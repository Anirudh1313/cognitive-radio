/*
===============================================================================
 Name        : RF_Handshaking.c
 Description : RF and PWM code for LPC1769

 CTI One Corporation released for Dr. Harry Li for CMPE 245 Class use ONLY!
===============================================================================
*/

//#include "FreeRTOS.h"
//#include "FreeRTOS.h"
#ifdef __USE_CMSIS
//#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <stdio.h>
#include <stdbool.h>
#include "LoRa.h"
#include"common.h"
#include "timer.h"

#include <string.h>


#define RF_Receive 1
#define RF_Transmit 0
#define TransmittACk 0
#define ack_start_stop 0
int rfInit(void);

char receiveData=0;
int packetSize;


static int snP = DEF_POWER;
static int snSF = DEF_SF;

#define LOGD(...)
//printf(__VA_ARGS__)
#define LOGV(...) printf(__VA_ARGS__)


/**************************************************************************************************
* @brief	wait for ms amount of milliseconds
* @param	ms : Time to wait in milliseconds
**************************************************************************************************/
static void delay_ms(unsigned int ms)
{
    unsigned int i,j;
    for(i=0;i<ms;i++)
        for(j=0;j<50000;j++);
}

/**************************************************************************************************
* @brief	wait for delayInMs amount of milliseconds
* @param	delayInMs : Time to wait in milliseconds
**************************************************************************************************/
static void delay(uint32_t delayInMs)
{
	LPC_TIM0->TCR = 0x02;		/* reset timer */
	LPC_TIM0->PR  = 0x00;		/* set prescaler to zero */
	LPC_TIM0->MR0 = delayInMs * (9000000 / 1000-1);
	LPC_TIM0->IR  = 0xff;		/* reset all interrrupts */
	LPC_TIM0->MCR = 0x04;		/* stop timer on match */
	LPC_TIM0->TCR = 0x01;		/* start timer */

	/* wait until delay time has elapsed */
	while (LPC_TIM0->TCR & 0x01);
}


/**************************************************************************************************
* main : Main program entry
**************************************************************************************************/
int main(void)
{
	LOGD("System clock is %d\n",SystemCoreClock);
	uint64_t nTimeElapsedBetweenPackets = 0;
	uint64_t nTimeTickCount1 = 0;
	double fPerformanceIndex = 100.0;
	double fDelayPerformanceDrop = 0.0;
	double fCorruptionDrop = 0.0;
	// Working frequency range from 724 MHz to 1040 MHz.
	//LoRabegin(1040000000);
	//LoRabegin(1020000000);
	//LoRabegin(724000000);
	//LoRabegin(750000000);
	//LoRabegin(790000000);
	//LoRabegin(800000000);
	//LoRabegin(845000000);
	//LoRabegin(850000000);
	//LoRabegin(910000000);
	//LoRabegin(868000000);
	LoRabegin(869000000);

	//LoRabegin(915000000);
	int counter =0;
	int ind = 0;
	timer_initialise();
	uint64_t nCount = 0;
	uint64_t nRcvCount = 0;

	char en = '*';// = {'*', '*', '*', '$\*', '%'};
	/* Start the tasks running. */
	//vTaskStartScheduler();

	/* If all is well we will never reach here as the scheduler will now be
	running.  If we do reach here then it is likely that there was insufficient
	heap available for the idle task to be created. */
#if RF_Receive
	while(1)
	{

#if 1
	    if((getTimeTickCount() - nCount) >= 10)
	    {
	        //nTimeElapsedBetweenPackets = nTimeElapsedBetweenPackets / nRcvCount;
	        //change the config
	        LOGV("changing SF=%d, power=%d perf=%f RSSI=%d SNR=%f nTimeEl=%d br=%d\n",
	                snSF, snP, fPerformanceIndex, packetRssi(), packetSnr(),
	                nTimeElapsedBetweenPackets,
	                getBitrate());
	        nTimeElapsedBetweenPackets = 0;
	        setTxPower(snP);
	        setSpreadingFactor(snSF);

	        nCount = getTimeTickCount();
	        char buffer[1024];
	        char Acknowledgement;
	        int nSendSize = snprintf(buffer, 1024, "%f", fPerformanceIndex);
	        Acknowledgement = 'A';
	        LOGD("Start Sending data \n");
	        //delay_ms(1000);
	        LoRabeginPacket(0);
	        //writebyte(Acknowledgement);
	        write(buffer, nSendSize);
	        LoRaendPacket();
	        LOGD("Data sent [%s]\n", buffer);
	    }
#endif
	    rxModeCheck();
	    //delay_ms(1000);

	    //LOGD("parsePacket:\n");
		packetSize = parsePacket(0);
		LOGD("parsePacket size=%d %llu\n", packetSize, getTimeTickCount());
		if (packetSize)
		{
					double entropy;
		    int count = 0;
			counter = 0;
			//NVIC_EnableIRQ(TIMER0_IRQn);
			//received a packet
			//LOGD("Received packet \n");
			// read packet
			if(nTimeTickCount1)
			{
			    nTimeElapsedBetweenPackets += (getTimeTickCount() - nTimeTickCount1);
			}

            counter = 1;
            char pcRecvBuffer[1024] = {0};
            int nIdxRcvBuffer = 0;
            int bGotSemi = 0;
            nTimeTickCount1 = getTimeTickCount();
			while (available() && (++count <= packetSize))
			{
			    pcRecvBuffer[nIdxRcvBuffer++] = receiveData = read();
				//LOGD("%c\n",receiveData);
				//return 0;
				// print RSSI of packet
				LOGD("Received packet '");
				LOGD("%c",receiveData);
				LOGD("' with RSSI ");
				LOGD("%d; nTimeElapsedS=%d\n",packetRssi(), nTimeElapsedBetweenPackets);

				if(receiveData == ';')
				{
				    bGotSemi = counter;
				}
				if(bGotSemi)
				{
				    continue;
				}
				if(counter%5 == 0)
				{
				    if(receiveData != en)
				    {
				        LOGD("no match in water mark index : %d, count = %d\n", ind, counter);
				    }
				    else
				    {
				        LOGD("****  Yes! match in water mark index : %d, count = %d\n", ind, counter);
				        ind++;
				    }
				}

				counter++;

			}
			if(count == packetSize)
			{
			    nRcvCount++;
			    /*Understand the param config from TX and configure local params
			     * ;P=%dSF=%d */
			    LOGV("buffer=[%s]\n", pcRecvBuffer);
			    //split by semi colon
			    // blah1;blah2;blah3
			    char* token = strtok(pcRecvBuffer, ";");
			    while(token = strtok(NULL, ";"))
			    {
			        LOGV("data=[%s]\n", token);
			        //this is P=%dSF=%d
			        sscanf(token, "P=%dSF=%d", &snP, &snSF);
			    }
			}
			entropy = ((packetSize - ind)*1.0) /packetSize;

			int nAsterixCountReq = bGotSemi / 5;
			if(nAsterixCountReq - ind > 0)
			{
			    LOGD("we did not get %d *'s\n", nAsterixCountReq - ind);
			    int nAsterixWeDropped = nAsterixCountReq - ind;
			    fCorruptionDrop = (nAsterixWeDropped * 50.0) / nAsterixCountReq;
			}


			/** for each second more of the time between packets,
			 * reduce 5% of the performance index
			 *  */
			if(nTimeElapsedBetweenPackets > 3)
			{
			    fDelayPerformanceDrop += (5.0 * (nTimeElapsedBetweenPackets /3));
			    if(fDelayPerformanceDrop >= 50.0)
			    {
			        fDelayPerformanceDrop = 50.0;
			    }
			}
			else
			{
			    fDelayPerformanceDrop = 0.0;
			}

			fPerformanceIndex = 100.0 - fDelayPerformanceDrop - fCorruptionDrop;


		}

	}


#if TransmittACk
	const char buffer[] = "Data from LPC1769";
	char Acknowledgement;
	Acknowledgement = 'A';
	while(1)
	{
		LOGD("Start Sending data \n");
		delay_ms(1000);
		LoRabeginPacket(0);
		//writebyte(Acknowledgement);
		write(buffer, sizeof(buffer));
		LoRaendPacket();
		LOGD("Data sent \n");

	}

#endif

#endif
#if RF_Transmit
    const char buffer[] = "Data from LPC1769";
    char Acknowledgement;
    Acknowledgement = 'A';
    while(1)
    {
        LOGD("Start Sending data \n");
        delay_ms(1000);
        LOGD("delay done\n");
        LoRabeginPacket(0);
        //writebyte(Acknowledgement);
        LOGD("begin packet\n");
       size_t ret = write(buffer, sizeof(buffer));
        LOGD("write done %d \n",ret);
        LoRaendPacket();
        LOGD("Data sent \n");

    }

#endif
}

void check_ack()
{
	LOGD("1 \n");

//		   NVIC_EnableIRQ(TIMER0_IRQn);
		//NVIC_DisableIRQ(TIMER0_IRQn);
}
