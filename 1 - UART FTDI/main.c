/**
  ********************************************************************************
  * @brief   ΤΙ F28337S Launchpad SCI (UART)
  * @date    Jul 2016
  * @version 1.0
  * @author  George Christidis
  ********************************************************************************
  * @details
			This program implements a communication between the TI F28337S
			Launchpad microcontroller and the computer via the FTDI chip. It
			contains the main functions to transmit a character or a string and
			receives a character via3 an interrupt.
	******************************************************************************
	*/

#include "F28x_Project.h"
#include <string.h>
#include <stdio.h>

__interrupt void scia_rx_isr(void);
void InitSCI(void);
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

    sprintf(uartstring,"Hello World!\n");
    uartputs(uartstring);

    while(1)
    {

    }
}

__interrupt void scia_rx_isr(void)
{
	char ch;

	ch = SciaRegs.SCIRXBUF.bit.SAR;
	uartputc(ch); //Loopback
	
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
