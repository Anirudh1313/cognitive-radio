/*
 * spi.cpp
 *
 *  Created on: Oct 29, 2016
 *      Author: Bharat
 */

#include "ssp.h"

//#define CHIP_SELECT			(LPC_GPIO0->FIOCLR |= (0x1<<6))
//#define CHIP_DESELECT		(LPC_GPIO0->FIOSET |= (0x1<<6))
#define SSP_BUFSIZE		128

/* statistics of all the interrupts */
volatile uint32_t interrupt0RxStat = 0;
volatile uint32_t interrupt0OverRunStat = 0;
volatile uint32_t interrupt0RxTimeoutStat = 0;
volatile uint32_t interrupt1RxStat = 0;
volatile uint32_t interrupt1OverRunStat = 0;
volatile uint32_t interrupt1RxTimeoutStat = 0;
uint8_t sendBuffer[SSP_BUFSIZE];



/****************************************************************************************
*SSP1_IRQHandler - SSP1 interrupt handler
****************************************************************************************/
void SSP1_IRQHandler(void)
{
	uint32_t regValue;

	regValue = LPC_SSP1->MIS;

	if ( regValue & SSPMIS_RORMIS )	/* Receive overrun interrupt */
	{
		interrupt1OverRunStat++;
		LPC_SSP1->ICR = SSPICR_RORIC;		/* clear interrupt */
	}

	if ( regValue & SSPMIS_RTMIS )	/* Receive timeout interrupt */
	{
		interrupt1RxTimeoutStat++;
		LPC_SSP1->ICR = SSPICR_RTIC;		/* clear interrupt */
	}

	/* please be aware that, in main and ISR, CurrentRxIndex and CurrentTxIndex
	are shared as global variables. It may create some race condition that main
	and ISR manipulate these variables at the same time. SSPSR_BSY checking (polling)
	in both main and ISR could prevent this kind of race condition */
	if ( regValue & SSPMIS_RXMIS )	/* Rx at least half full */
	{
		interrupt1RxStat++;		/* receive until it's empty */
	}
	return;
}

/****************************************************************************************
* SSP1Init - Initialize SSP1 module
****************************************************************************************/
void SSP1Init()
{
	// Enable SSP1
	LPC_SC->PCONP |= (1 << SSP1_PCONP_ENABLE);

	// Clock selection for SSP1
	LPC_SC->PCLKSEL0 |= (3<<20);

	/* PIN select P0.6 - SSP1 SSEL1, P0.7 - SSP1 SCK1
				  P0.8 - SSP1 MISO1, P0.9 - SSP1 MOSI1 */

	LPC_PINCON->PINSEL0 &= ~((0x3<<12)|(0x3<<14)|(0x3<<16)|(0x3<<18));
	LPC_PINCON->PINSEL0 |= ((0x2<<12)|(0x2<<14)|(0x2<<16)|(0x2<<18));

	LPC_PINCON->PINSEL0 &= ~(3<<12);	//P0.6 as gpio
	LPC_GPIO0->FIODIR |= (1<<6);        	// SSP0 P0.6 defined as Outputs

	// DSS data to 8-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 15
	LPC_SSP1->CR0 = 0x0707;

	// SSP1 CPSR clock pre-scale register, master mode, minimum divisor is 0x02
	LPC_SSP1->CPSR = 0x2;

	/* Enable the SSP Interrupt */
	NVIC_EnableIRQ(SSP1_IRQn);

	/* Master mode */
	LPC_SSP1->CR1 &= ~(SSPCR1_MS);

	/* SSP Enable */
	LPC_SSP1->CR1 = SSPCR1_SSE;

	LPC_SSP1->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;

}

/****************************************************************************************
* ssp1Send - send data over SSP1
****************************************************************************************/
uint8_t ssp1Send(uint8_t *buf, uint32_t length)
{
	uint32_t i=0;
	uint8_t Dummy = 0;

	for( i = 0 ; i < length ; i++ )
	{
		/* Move on only if NOT busy and TX FIFO not full. */
		while((LPC_SSP1->SR & (SSP_STAT_TNF|SSP_STAT_BSY)) != SSP_STAT_TNF);
		LPC_SSP1->DR = *buf;
		buf++;

		while((LPC_SSP1->SR & (SSP_STAT_BSY|SSP_STAT_RNE)) != SSP_STAT_RNE);
		/* Whenever a byte is written, MISO FIFO counter increments, Clear FIFO
		on MISO. Otherwise, when SSP1Receive() is called, previous data byte
		is left in the FIFO. */
		Dummy = LPC_SSP1->DR;
	}

	return Dummy;
}

/****************************************************************************************
* ssp1Transfer - Transmit and receive byte on SSP1
****************************************************************************************/

uint8_t ssp1Transfer(uint8_t dataByte)
{
	uint8_t data=0;
	LPC_SSP1->DR = dataByte;
	while(LPC_SSP1->SR & (1<<4));
	data = LPC_SSP1->DR;
	return data;
}
/*
uint8_t ssp1Transfer(uint8_t dataByte)
{
	uint8_t dummy=0;
	sendBuffer[0] = dataByte;
	dummy = ssp1Send((uint8_t *)sendBuffer, 1 );
	return dummy;
}
*/
