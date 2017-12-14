#include "LPC17xx.h"
#include "common.h"
#define Stop 0
extern unsigned int SystemCoreClock;
unsigned int PreScaleMillli(uint8_t timerPclkBit);

uint64_t gnTimeTicks = 0;

uint64_t getTimeTickCount()
{
    return gnTimeTicks;
}
void timer_initialise(void)
{
	SystemInit();
	printf("timer init\n");
    /* Powe on Timer 1 and 0 */
    LPC_SC->PCONP |= (1<<LPC_TIMER0) | (1<<LPC_TIMER1);

    LPC_TIM0->MCR  = (1<<SBIT_MR0I) | (1<<SBIT_MR0R);
    LPC_TIM0->PR   = PreScaleMillli(PCLK_TIMER0);
    LPC_TIM0->MR0  = TIME_IN_MILLI;
    LPC_TIM0->TCR  = (1 <<TIMER_ENABLE);
    NVIC_EnableIRQ(TIMER0_IRQn);

/*
    LPC_TIM1->MCR  = (1<<SBIT_MR0I) | (1<<SBIT_MR0R);
    LPC_TIM1->PR   = PreScaleMillli(PCLK_TIMER1);
    LPC_TIM1->MR0  = TIME_IN_MILLI;
    LPC_TIM1->TCR  = (1 <<TIMER_ENABLE);
    NVIC_EnableIRQ(TIMER1_IRQn);
*/

}
void TIMER0_IRQHandler(void)
{
    //printf("Timer 0 new code\n");
    unsigned int isrMask;
    isrMask = LPC_TIM0->IR;
    LPC_TIM0->IR = isrMask;         /* Clear the Interrupt Bit */
    //LPC_TIM0->IR = 0xFF;
    gnTimeTicks++;
    //printf("hello\n");

#if Stop
    char temp;
    int temp1;
    temp1 = parsePacket(0);
	if(temp1)
	{
		temp = read();
		if(temp == 'X')
			check_ack();
	}
	else
		printf("s/n");

#endif

}
void TIMER1_IRQHandler(void)
{
	//printf("Timer 1 new code");
    unsigned int isrMask;
    isrMask = LPC_TIM1->IR;
    LPC_TIM1->IR = isrMask;        /* Clear the Interrupt Bit */
#if Stop

#endif
}
unsigned int PreScaleMillli(uint8_t timerPclkBit)
{
    unsigned int pclk,prescalarForUs;
    pclk = (LPC_SC->PCLKSEL0 >> timerPclkBit) & 0x03;  /* get the pclk info for required timer */
    switch ( pclk )                                    /* Decode the bits to determine the pclk*/
    {
    case 0x00:
        pclk = SystemCoreClock/4;
        break;
    case 0x01:
        pclk = SystemCoreClock;
        break;
    case 0x02:
        pclk = SystemCoreClock/2;
        break;
    case 0x03:
        pclk = SystemCoreClock/8;
        break;
    default:
        pclk = SystemCoreClock/4;
        break;
    }
    /* Set theprescale for milli */
    prescalarForUs =pclk/1000 - 1;
    return prescalarForUs;

}
