/**
  ********************************************************************************
  * @brief   ΤΙ F28337S Sine Wave Generator and PLL
  * @date    Jul 2016
  * @version 1.0
  * @author  George Christidis
  ********************************************************************************
  * @details
			This program uses a DAC to output a sinusoidal wave 0-3V, 50Hz. This
			wave is then fed externally to an ADC converter. A PLL is used to
			determine the phase of the PLL. Using an oscilloscope, the original
			sinusoidal output together with the PLL output can be compared.
	******************************************************************************
	*/

#include "F28x_Project.h"
#include "Solar_F.h"

#define PI 3.14159

__interrupt void epwm3_isr(void), adcb1_isr(void);
void InitPWM3(void), InitDACA(void), InitDACC(void),InitADC(void);

SPLL_1ph_F spll1;
SPLL_1ph_F_NOTCH_COEFF spll_notch_coef1;

void main(void)
{
	//------- INTERNAL INIT------------//
    InitSysCtrl();
    DINT; //Disable CPU interrupts
    InitPieCtrl();
    IER = 0x0000; //Disable CPU interrupts and clear all CPU interrupt flags:
    IFR = 0x0000;
    InitPieVectTable();
    //------- END INTERNAL INIT--------//

	InitPWM3();
	InitDACA();
	InitDACC();
	InitADC();

    //PLL 1PHASE//
    SPLL_1ph_F_init(50,((float)(1.0/200000.0F)), &spll1);
    SPLL_1ph_F_notch_coeff_update(((float)(1.0/50000.0F)), (float)(2*PI*50*2),(float)0.00001,(float)0.1, &spll1);

    while(1)
    {

    }
}

__interrupt void epwm3_isr(void)
{
    static int daccounter=0;
    float wt, sinwt;
    uint16_t dacoutv;

    if (daccounter<1000) daccounter++; else daccounter=0;
    wt = daccounter / 1000.0F;
    sinwt = __sinpuf32(wt);
    dacoutv = (sinwt + 1.0F) * 4095.0F / 2.0F;
    DacaRegs.DACVALS.bit.DACVALS = dacoutv;

    EPwm3Regs.ETCLR.bit.INT = 1; //Clear INT flag for this timer
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3; //Acknowledge this interrupt to receive more interrupts from group 3
}

__interrupt void adcb1_isr(void)
{
	spll1.AC_input = (float32)(((int16_t)AdcbResultRegs.ADCRESULT0-2048)/2048.0F);
	SPLL_1ph_F_FUNC(&spll1);

	DaccRegs.DACVALS.bit.DACVALS = spll1.sin[0] * 2047 + 2047;

	AdcbRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //clear INT1 flag
	PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; // Acknowledge this interrupt to receive more interrupts from group 1
}

void InitPWM3(void)
{
    EALLOW;
    //PWMFeq=100MHz
    CpuSysRegs.PCLKCR2.bit.EPWM3=1; 			//Disable Clock Gating
    EPwm3Regs.TBCTL.bit.CTRMODE = 00; 			//Up count mode
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = 0x0;		//Clock ratio to SYSCLKOUT
    EPwm3Regs.TBCTL.bit.CLKDIV = 0x0;
    EPwm3Regs.TBPRD = 1999; 					//freq=50kHz
    EPwm3Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;	//Select INT on Zero event
    EPwm3Regs.ETSEL.bit.INTEN = 1;				//Enable INT
    EPwm3Regs.ETPS.bit.INTPRD = ET_1ST;         //Generate INT on 1st event
    PieVectTable.EPWM3_INT = &epwm3_isr;
    IER |= M_INT3; 								//Enable CPU INT3 which is connected to EPWM1-3 INT:
    PieCtrlRegs.PIEIER3.bit.INTx3 = 1;			//Interrupt Enable for INT3.7
    EINT;  										//Enable Global interrupt INTM
    ERTM;  										//Enable Global realtime interrupt DBGM
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1; 		//start PWM counters
    EDIS;
}

void InitDACA(void)
{
	//pin 27 on board//
    EALLOW;
    DacaRegs.DACCTL.bit.SYNCSEL = 2; 	//PWMSYNC3
    DacaRegs.DACCTL.bit.DACREFSEL = 1; 	//ADC VREFHI/VREFLO are the reference voltages
    DacaRegs.DACVALS.bit.DACVALS = 0; 	//init with 0V
    DacaRegs.DACOUTEN.bit.DACOUTEN = 1; //Enable DAC
    EDIS;
}

void InitDACC(void)
{
    //pin 24 on board//
    EALLOW;
    DaccRegs.DACCTL.bit.SYNCSEL = 2; 	//PWMSYNC3
    DaccRegs.DACCTL.bit.DACREFSEL = 1; 	//ADC VREFHI/VREFLO are the reference voltages
    DaccRegs.DACVALS.bit.DACVALS = 0; 	//init with 0V
    DaccRegs.DACOUTEN.bit.DACOUTEN = 1; //Enable DAC
    EDIS;
}

void InitADC(void)
{
    //ADCINB2//
    //pin 26 on board//
    AdcSetMode(ADC_ADCB, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    EALLOW;
    CpuSysRegs.PCLKCR13.bit.ADC_B = 1; 		//ADC_B Clock Enable
    AdcbRegs.ADCCTL2.bit.PRESCALE = 6; 		//ADCCLK = Input Clock / 4.0 = 50MHz
    AdcbRegs.ADCCTL1.bit.ADCPWDNZ = 1; 		//All analog circuitry inside the core is powered up
    AdcbRegs.ADCCTL1.bit.INTPULSEPOS = 1; 	//Interrupt pulse generation occurs at the end of the conversion

    AdcbRegs.ADCSOC0CTL.bit.TRIGSEL = 0x0B;	//ADCTRIG11 - ePWM4, ADCSOCA trigger;
    AdcbRegs.ADCSOC0CTL.bit.CHSEL = 2; 		//ADCIN2: pin 26 on baord
    AdcbRegs.ADCSOC0CTL.bit.ACQPS = 40; 	//sample window is number + 1 SYSCLK cycles
    AdcbRegs.ADCINTSEL1N2.bit.INT1SEL = 0; 	//EOC0 is trigger for ADCINT1
    AdcbRegs.ADCINTSEL1N2.bit.INT1E = 1; 	//Enable ADCINT1
    PieVectTable.ADCB1_INT = &adcb1_isr; 	//int 1.2
    IER |= M_INT1; 							//Enable group 1 interrupts
    EINT;  									// Enable Global interrupt INTM
    ERTM;  									// Enable Global realtime interrupt DBGM
    PieCtrlRegs.PIEIER1.bit.INTx2 = 1; 		//Interrupt Enable for INT1.2

    ClkCfgRegs.PERCLKDIVSEL.bit.EPWMCLKDIV = 1; //PWMFeq=100MHz
    CpuSysRegs.PCLKCR2.bit.EPWM4=1; 			//Disable Clock Gating
    EPwm4Regs.TBCTL.bit.CTRMODE = 00; 			//Up count mode
    EPwm4Regs.TBCTL.bit.HSPCLKDIV = 0x0;   		//Clock ratio to SYSCLKOUT = 1
    EPwm4Regs.TBCTL.bit.CLKDIV = 0x0;
    EPwm4Regs.TBPRD = 2000; 					//freq: 50kHz
    EPwm4Regs.ETSEL.bit.SOCASEL = 1;			// Enable event time-base counter equal to zero
    EPwm4Regs.ETPS.bit.SOCAPRD = 1; 			// 1 Event has occured for SOCA
    EPwm4Regs.ETSEL.bit.SOCAEN = 1;				// Enable SOCA
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1; 		//start PWM counters
    EDIS;
}
