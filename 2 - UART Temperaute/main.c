/**
  ********************************************************************************
  * @brief   ΤΙ F28337S Launchpad Temperature Sensor
  * @date    Jul 2016
  * @version 1.0
  * @author  George Christidis
  ********************************************************************************
  * @details
			This program implements a communication between the TI F28337S
			Launchpad microcontroller and the computer via the FTDI chip and sends
			the microcontroller internal temperature when a character is sent.
	******************************************************************************
	*/

#include "F28x_Project.h"
#include <string.h>
#include <stdio.h>

__interrupt void scia_rx_isr(void);
void InitSCI(void), InitTemperatureSensor(void);
void uartputs(char *), uartputc(char);

char uartstring[30];

void main(void)
{
	//------- INTERNAL INIT------------//
    InitSysCtrl();
    DINT; // Disable CPU interrupts
    InitPieCtrl();
    IER = 0x0000; //Disable CPU interrupts and clear all CPU interrupt flags:
    IFR = 0x0000;
    InitPieVectTable();
    //------- END INTERNAL INIT--------//

	InitSCI();
	InitTemperatureSensor();

    sprintf(uartstring,"Hello World!\n");
    uartputs(uartstring);

    while(1)
    {

    }
}

__interrupt void scia_rx_isr(void)
{
	uint16_t temperature;

	char ch;
	ch = SciaRegs.SCIRXBUF.bit.SAR;

	AdcaRegs.ADCSOCFRC1.bit.SOC0 = 1; //Trigger ADC
	while (AdcaRegs.ADCINTFLG.bit.ADCINT1 != 1) ; //wait for new sample
	AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; // Clears respective flag bit in the ADCINTFLG register
	temperature = GetTemperatureC(AdcaResultRegs.ADCRESULT0);
	sprintf(uartstring,"Temperature: %d\n",temperature);
	uartputs(uartstring);

	PieCtrlRegs.PIEACK.all = PIEACK_GROUP9; //Acknowledge this interrupt to receive more interrupts from group 9
}

void uartputc(char ch)
{
	while (SciaRegs.SCICTL2.bit.TXRDY != 1) ;
	SciaRegs.SCITXBUF.all = ch;
}

void uartputs(char s[])
{
	int i=0;
	while (s[i] != '\0') {
		uartputc(s[i]);
		i++;
	}
}

void InitSCI()
{
    //GPIO85: Rx, GPIO84: Tx
    GPIO_SetupPinMux(84, GPIO_MUX_CPU1, 5);
    GPIO_SetupPinMux(85, GPIO_MUX_CPU1, 5);
    GPIO_SetupPinOptions(84, GPIO_OUTPUT, GPIO_PUSHPULL);
    GPIO_SetupPinOptions(85, GPIO_INPUT, GPIO_ASYNC);

    EALLOW;
    SciaRegs.SCICCR.bit.STOPBITS = 0; 		//1 stop bit
    SciaRegs.SCICCR.bit.PARITYENA = 0;		//no parity
    SciaRegs.SCICCR.bit.SCICHAR = 0x7; 		//8 bit characters
    SciaRegs.SCICTL1.bit.RXENA = 1; 		//Enable Rx
    SciaRegs.SCICTL1.bit.TXENA = 1; 		//Enable Tx
    SciaRegs.SCIHBAUD.all = 0x0000; 		//Baudrate 115200, Clock Frequency 200MHz
    SciaRegs.SCILBAUD.all = 0x0035;
    SciaRegs.SCICTL1.bit.SWRESET = 1; 		//Remove Reset

    SciaRegs.SCICTL2.bit.RXBKINTENA = 1; 	//Receiver-buffer/break interrupt enable
    PieVectTable.SCIA_RX_INT = &scia_rx_isr;
    IER |= M_INT9; 							//Enable CPU INT7
    PieCtrlRegs.PIEIER9.bit.INTx1 = 1;		//Interrupt Enable for INT9.1
    EINT;  									//Enable Global interrupt INTM
    ERTM;  									//Enable Global realtime interrupt DBGM
    EDIS;
}

void InitTemperatureSensor()
{
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    InitTempSensor(3.3); 					//Launchpad ADC voltage = 3.3V
	EALLOW;
	CpuSysRegs.PCLKCR13.bit.ADC_A = 1; 		//ADC_A Clock Enable on low power mode
	AdcaRegs.ADCCTL2.bit.PRESCALE = 6; 		//ADCCLK = Input Clock / 4.0 = 50MHz
	AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1; 		//All analog circuitry inside the core is powered up
	AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1; 	//Interrupt pulse generation occurs at the end of the conversion
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 0; 	//Software trigger;
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 13; 	//ADCIN13: temperature
	AdcaRegs.ADCSOC0CTL.bit.ACQPS = 40; 	//sample window is number + 1 SYSCLK cycles //min for tempsensor: 700ns
	AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0; 	//EOC0 is trigger for ADCINT1
	AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1; 	//Enable ADCINT1AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 0; 	//Software trigger
	EDIS;
}
