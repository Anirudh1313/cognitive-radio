/*
 * @file main.c
 * @brief The main application file;
 * 1) GPIO (I/O test) code under the macro: GPIO_TEST
 * @date Created on: 24-Sep-2017
 * 2) RF Module test under macros: RF_RX_TEST, RF_TX_TEST
 * 3) LISA test code under macros: LISA_TX, LISA_RX
 * @author(s): Unnikrishnan, Pratap, Rahul, Anirudh
 */


//#include "chip_lpc175x_6x.h"
//#include "gpio_17xx_40xx.h"
#include "FreeRTOS.h"
#include "task.h"

#include "board.h"

#include "api.h"

#include "imp_LISA.h"
#include "imp_scrambler_descrambler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "gpio_17xx_40xx.h"

/** CMPE 245 Assignments
 * @{ */
/** SW-LED Assignment: */
#define CMPE245_GPIO_TEST
/** @} */

typedef struct
{
    int port;
    int pin;
    int portLED;
    int pinLED;
}tSwitchInfo;

static void vSwitchListener(void* apvParams)
{
    tSwitchInfo* pSWInfo = (tSwitchInfo*)apvParams;
    /** poll the input switch */
    do {
#if 0
        uint32_t* pGPIO_FIO0PIN0 = (uint32_t*)(0x2009C014);
        uint8_t nBitVal = *pGPIO_FIO0PIN0 & (0x01UL << 2);
        ///DEBUGOUT("nBitVal=%d pin=%d %x\n", nBitVal, pSWInfo->pin, *pGPIO_FIO0PIN0);
#endif
        //DEBUGOUT("port=%d pin=%d\n", pSWInfo->port, pSWInfo->pin);
        bool nBit;
        if(nBit = Chip_GPIO_ReadPortBit(LPC_GPIO, pSWInfo->port, pSWInfo->pin))
        {
            DEBUGOUT("SW pressed\n");
            /** turn on the LED */
            unsigned int const CLEAR_P0_3 = (1 << 3);
            Chip_GPIO_SetPinToggle(LPC_GPIO, pSWInfo->portLED, pSWInfo->pinLED);
        }
        else
        {
            //DEBUGOUT("SW not pressed");
        }
        /** delay 1s */
    } while(1);
}

static tSwitchInfo sw = {0, 2, 0, 3};
int main_app()
{
#ifdef CMPE245_GPIO_TEST
    /** set P0.2 (J2-21); Switch PIN to Input */
    Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 2);
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, IOCON_MODE_PULLDOWN);
    //LPC_IOCON_T* pIOCON = (LPC_IOCON_T*)LPC_IOCON;
    //pIOCON->PINMODE[0] |= (3UL << 4);

#if 0
    uint32_t* pPINMODE0 = (uint32_t*)(0x4002C040);
    *pPINMODE0 = *pPINMODE0 | (3UL << 4);
#endif

    /** set P0.3 (J2-22); LED PIN to Output */
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 3);
    unsigned int const CLEAR_P0_3 = (1 << 3);
    Chip_GPIO_SetPortOutHigh(LPC_GPIO, 0, CLEAR_P0_3);


    /** create the Switch Listener Thread */

    xTaskCreate(vSwitchListener, (char*)"SW-Listener", (512 * 4), (void*)(&sw), (tskIDLE_PRIORITY + 1UL), NULL);

#endif /**< CMPE245_GPIO_TEST */

    return 1;
}

static void vDataTransmitter(void* apvParams)
{
    /** set P0.3 (J2-22); LED PIN to Output */
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 3);
    unsigned int const CLEAR_P0_3 = (1 << 3);
    Chip_GPIO_SetPortOutHigh(LPC_GPIO, 0, CLEAR_P0_3);

    /** set P2.6 (J2-48); RF TX */
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 2, 6);
    unsigned int const CLEAR_P2_6 = (1 << 6);
    //Chip_GPIO_SetPortOutHigh(LPC_GPIO2, 0, CLEAR_P2_6);

    char nOut = 0;
    while(1)
    {
        DEBUGOUT("sending %d\n", nOut);
        if(nOut)
        {
            /** active low LED */
            Chip_GPIO_SetPortOutLow(LPC_GPIO, 0, CLEAR_P0_3);
            Chip_GPIO_SetPortOutHigh(LPC_GPIO, 2, CLEAR_P2_6);
        }
        else
        {
            Chip_GPIO_SetPortOutHigh(LPC_GPIO, 0, CLEAR_P0_3);
            Chip_GPIO_SetPortOutLow(LPC_GPIO, 2, CLEAR_P2_6);
        }
        nOut = !nOut;
        vTaskDelay(5000);
    }

}

int main_tx_test()
{
    xTaskCreate(vDataTransmitter, (char*)"DATA-Transmitter", (512 * 4), NULL, (tskIDLE_PRIORITY + 1UL), NULL);
    return 1;
}

static void vDataReceiver(void* apvParams)
{
    /** set P0.2 (J2-21); Switch PIN to Input */
    Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 2);
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, IOCON_MODE_PULLDOWN);

    DEBUGOUT("PIN configured\n");

    do {
        //DEBUGOUT("port=%d pin=%d\n", pSWInfo->port, pSWInfo->pin);
        bool nBit;
        if((nBit = Chip_GPIO_ReadPortBit(LPC_GPIO, 0, 2)))
        {
            DEBUGOUT("SW pressed\n");
            /** turn on the LED */

        }
        else
        {
            //DEBUGOUT("SW not pressed\n");
        }
        /** delay 1s */
    } while(1);
}

int main_rx_test()
{
    xTaskCreate(vDataReceiver, (char*)"DATA-Receiver", (512 * 4), NULL, (tskIDLE_PRIORITY + 1UL), NULL);
    return 1;
}

#define portAPP_MSTOTICK(t) (t / (portTICK_RATE_MS))

void Chip_GPIO_SendBit(LPC_GPIO_T *pGPIO, uint8_t port, uint32_t pin, uint8_t nBit)
{
    if(nBit)
    {
        pGPIO[port].SET = (1 << pin);
    }
    else
    {
        pGPIO[port].CLR = (1 << pin);
    }
}

uint8_t Chip_GPIO_ReadBit(LPC_GPIO_T *pGPIO, uint8_t port, uint32_t pin)
{
    return ((pGPIO[port].PIN >> pin) & 1);
}

void left_shift_by(char* apcBuffer, int anSize, int anBy)
{
    for(int i = 0; i < anSize; i++)
    {

    }
}

void vLISADataTxRx(void* apvParams)
{
    tLISACtx* pCtx = (tLISACtx*)apvParams;
    int i = 0;

    /** set P0.3 (J2-22); Tx PIN to Output */
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 3);
    unsigned int const CLEAR_P0_3 = (1 << 3);
    //Chip_GPIO_SetPortOutHigh(LPC_GPIO, 0, CLEAR_P0_3);

    /** set P0.2 (J2-21); Rx to Input */
    Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 2);
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, IOCON_MODE_PULLDOWN);

    uint8_t payload[] = "CMPE245";
    uint8_t payload_scrambled[1024];

#ifdef LISA_TX
    pCtx->pLISADataRaw->pPayload->pcPayload = payload;//DEFAULT_PAYLOAD;
    pCtx->pLISADataRaw->pPayload->nLen = strlen(pCtx->pLISADataRaw->pPayload->pcPayload) + 1;
    /** scramble */
    gScrambleData(5, pCtx->pLISADataRaw->pPayload->pcPayload,
            pCtx->pLISADataRaw->pPayload->nLen, payload_scrambled);
    pCtx->pLISADataRaw->pPayload->pcPayload = payload_scrambled;
    /** generate payload in pCtx->pcBuffer */
    create_data_file(pCtx, pCtx->pLISADataRaw);
#endif
#ifdef LISA_RX
#endif

    portTickType nT1, nT2;

    while(1)
    {
        //nT1 = xTaskGetTickCount();
        if(0)//i > 1023)
        {
            vTaskDelay(portAPP_MSTOTICK(1000));
            continue;
        }
        uint8_t nBit;
#ifdef LISA_TX
        /** test send  */
        nBit = get_bit_value(pCtx->pcBuffer, i);
        Chip_GPIO_SendBit(LPC_GPIO, 0, 3, nBit);
        //Chip_GPIO_SetPortOutLow(LPC_GPIO, 0, 3);
        //DEBUGOUT("sending %d %d\n", nBit, i);
#endif /**< LISA_TX */
#ifdef LISA_RX
        /** left shift the buffer by 1 */
        //left_shift_by(pCtx->pcBuffer, 1);
        nBit = Chip_GPIO_ReadPortBit(LPC_GPIO, 0, 2);
        //nBit = get_bit_value(pCtx->pcBuffer, i);
        put_bit_value(pCtx->pcBuffer, i, nBit);
        //DEBUGOUT("reading %d %d\n", nBit, i);
        if(i == 1023)
        {
            int ret = read_data_from_file(pCtx, pCtx->pLISADataRaw);

            DEBUGOUT("ret=%d nLen=%d\n", ret, pCtx->pLISADataRaw->pPayload->nLen);
            gDeScrambleData(5, pCtx->pLISADataRaw->pPayload->pcPayload,
                    pCtx->pLISADataRaw->pPayload->nLen,
                    payload_scrambled
                    );
            DEBUGOUT("payload=[%s]\n", payload_scrambled);

            //print_hex(pCtx->pcBuffer, BUFFER_SIZE);
        }
#endif /**< LISA_RX */
        i++;
        /** Configuration for this board says:
         * portTICK_RATE_MS = 1000 / 1000
         * means each TICK = 1 millisecond
         *
         * Let's say our datarate = 1kbps
         * 1000 bits in a second
         * So, each bit = 1ms
         *  */
        portTickType nTicks = portAPP_MSTOTICK(10);
        //DEBUGOUT("nTicks=%d\n", nTicks);

        //nT2 = xTaskGetTickCount();
        //DEBUGOUT("nT2-nT1 = %d\n", nT2 - nT1);
        vTaskDelay(nTicks);
    }

    DEBUGOUT("PIN configured\n");

}

int main_lisa()
{
    tLISACtx* pCtx = (tLISACtx*)calloc(1, sizeof(tLISACtx));
    tLISADataRaw* pLISADataRaw = (tLISADataRaw*)calloc(1, sizeof(tLISADataRaw) + sizeof(tLISAPayload));
    pLISADataRaw->nBufferSize = BUFFER_SIZE;
    pLISADataRaw->pPayload = (tLISAPayload*)(pLISADataRaw + 1);

    pCtx->fSFConfidence = 100.0;
    pCtx->nConfidence = 20;//(int)lround((pCtx->fSFConfidence / 100) * 32);
    if(pCtx->nConfidence < 0 || pCtx->nConfidence > 32)
        pCtx->nConfidence = 32;

    pCtx->pLISADataRaw = pLISADataRaw;

    xTaskCreate(vLISADataTxRx, (signed char*)"LISA-TxRx", (512 * 4), (void*)pCtx, (tskIDLE_PRIORITY + 1UL), NULL);
    return 1;
}
