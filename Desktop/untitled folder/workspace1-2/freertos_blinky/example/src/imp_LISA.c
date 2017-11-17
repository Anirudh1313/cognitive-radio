/** 
 * @file imp_LISA.c
 *
 * @brief LISA Implementation file
 *
 * @date created Sep 16 2017
 *
 * @features:
 * 1) Creating a DAT file with 1024 bit LISA protocol specific
 * data: - argv[1] == 0
 * a) Random GARBAGE
 * b) SYNC FIELD (with or without corruption) - use argv[2] to specify 
 *                                      corruption amount (example = 1/32 = 0.03125
 *                                      will corrupt one of the 32 Sync Fields)
 * c) Payload (8-bit Length + the payload given by user at argv[3])
 * 2) Reading the DAT file and printing the payload
 * See the major function prototypes at imp_LISA.h
 *
 * @author(s) Unnikrishnan Sreekumar<unnikrishnankgs@gmail.com>, Rahul D G, Prathap, Anirudh
 * 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

//#define DEBUG_E
//#define VERBOSE_E

#include "debug_e.h"

#include "imp_LISA.h"

//#define TEST_GARBAGE_BYTE_ALIGNED

//#define PRINT_HEXDUMP

/**
 * @brief
 * NOTE: bit index starts at 0
 */
void put_bit_value(unsigned char* apcBuffer, int anBitLoc, char anBitVal)
{
    unsigned char* pBuf = apcBuffer + (anBitLoc/8);
    LOGD("bit pos %d val=%d %x\n\n", anBitLoc, anBitVal, *pBuf);
    int nLeftOverBits = anBitLoc - ((anBitLoc/8)*8);
    if(nLeftOverBits)
    {
        /** write at the nLeftOverBits'th position of pBuf */
        *pBuf &= (~(0x01 << (8 - nLeftOverBits - 1)));
        *pBuf |= anBitVal << (8 - nLeftOverBits - 1);
        
    }
    else
    {
        /** exactly at byte boundary */
        /** clear that bit */
        *pBuf &= (~(0x01 << 7));
        *pBuf |= (anBitVal << 7);
    }

}

void put_byte(unsigned char* apcBuffer, int anBitLoc, unsigned char anByte)
{
    if(!apcBuffer)
        return;
    for(int i = 0; i < 8; i++)
        put_bit_value(apcBuffer, anBitLoc + i, ((anByte >> (8 - i - 1)) & 0x01));
}

/**
 * @fn int create_data_file(tLISADataRaw* apLISADataRaw)
 * @brief Function to create DAT_FILE with RAW data 
 * RAW data will have sync-field, payload and other random 
 * data
 * @param apLISADataRaw [IN] LISA network packets to be written
 * into a file
 * @return SC_SUCCESS on successfully writing the file 
 */
int create_data_file(tLISACtx* apCtx, tLISADataRaw* apLISADataRaw)
{
    int nMaxGarbageLenBits = 0;
    int nRandGarbageLenBits = 0;
    int nIdx = 0; /**< current position in pcBuffer */

    int nSFStart = 0;


    LOGD("DEBUGME\n");
    if(!apCtx || !apLISADataRaw)
        return EC_FAILURE;
    LOGD("DEBUGME\n");

    /** 1) Generate random garbage
     *  a) Max length of garbage = BUFFER_SIZE - (sizeof(SF) + sizeof(payload))
     */
    #ifdef TEST_WITH_INIT_GARBAGE
    nMaxGarbageLenBits = BUFFER_SIZE - (LISA_SYNC_FIELD_LEN_BITS + LISA_PAYLOAD_LEN_FIELD_BITS + (apLISADataRaw->pPayload->nLen * 8));
    if(nMaxGarbageLenBits < 0)
        return EC_FAILURE;

    nRandGarbageLenBits = ((rand() % nMaxGarbageLenBits));

    #ifdef TEST_GARBAGE_BYTE_ALIGNED
    /** Test code; not compiled; forces the garbage bits to be byte aligned */
    nRandGarbageLenBits = ((rand() % nMaxGarbageLenBits)/8)*8;
    #endif /**< TEST_GARBAGE_BYTE_ALIGNED */

    for(nIdx = 0; nIdx < nRandGarbageLenBits/8; nIdx++)
    {
        apCtx->pcBuffer[nIdx] = (unsigned char)rand();
    }

    {
        int nLeftOverBits = nRandGarbageLenBits - ((nRandGarbageLenBits/8) * 8);
        if(nLeftOverBits)
        {
            unsigned char nKernel = (2 << (nLeftOverBits+1)) - 1; /**< 2^N - 1 is all 1's */
            nKernel = nKernel << (8 - nLeftOverBits);
            apCtx->pcBuffer[nIdx++] = nKernel & (unsigned char)rand();
        }
    }
    LOGD("nRandGarbageLenBits=%d\n", nRandGarbageLenBits);
    #endif

    /** 2) Sync field
     *  a) Corrupt the bits based on fSFCorrruption */
    nSFStart = nIdx = nRandGarbageLenBits;
    #if 0
    for(int i = 0; i < (LISA_SYNC_FIELD_LEN_BITS/8)/2; i++, nIdx += 8)
        put_byte((unsigned char*)apCtx->pcBuffer, nIdx, (0x05 << 4) | (i & 0x0f));
    for(int i = 0; i < (LISA_SYNC_FIELD_LEN_BITS/8)/2; i++, nIdx += 8)
        put_byte((unsigned char*)apCtx->pcBuffer, nIdx, (0x0a << 4) | (i & 0x0f));
    #endif
    for(int i = 0; i < LISA_SYNC_FIELD_LEN_BITS/8; i++, nIdx += 8)
    {
        put_byte((unsigned char*)apCtx->pcBuffer, nIdx, KERNEL_STRING[i]);
        
    }

#ifdef TEST_WITH_CORRUPTION
    /** corrupt bits */
    //int nBytesToCorrupt = apLISADataRaw->fSFCorrruption * LISA_SYNC_FIELD_LEN_BYTES;
    LOGV("nBytesToCorrupt=%d nSFStart=%d\n", apLISADataRaw->nBytesToCorrupt, nSFStart);
    for(int i = 0; i < apLISADataRaw->nBytesToCorrupt; i++)
    {
        int nRandBitPos = (nSFStart + (i*8) + (rand() % 7));
        LOGD("nRandBitPos=%d bot_val=%d %x\n", nRandBitPos, get_bit_value(apCtx->pcBuffer, nRandBitPos), apCtx->pcBuffer[nRandBitPos/8]);
        put_bit_value(apCtx->pcBuffer, nRandBitPos, (get_bit_value(apCtx->pcBuffer, nRandBitPos) ? 0x00 : 0x01));
        LOGD("corrupted=%x\n", apCtx->pcBuffer[nRandBitPos/8]);
    }
#endif
    
    /** 3) Payload */
    /** a) write length */
    LOGD("len=%d payload=[%s]\n", apLISADataRaw->pPayload->nLen, apLISADataRaw->pPayload->pcPayload);
    put_byte((unsigned char*)apCtx->pcBuffer, nIdx, apLISADataRaw->pPayload->nLen);
    nIdx += 8;

    for(int i = 0; i < apLISADataRaw->pPayload->nLen; i++)
    {
        put_byte((unsigned char*)apCtx->pcBuffer, nIdx, apLISADataRaw->pPayload->pcPayload[i]);
        nIdx += 8;
    }

    LOGV("bits in buff=%d\n", nIdx);

#ifdef WRITE_FILE
    FILE* fpDATA = fopen(DAT_FILE, "wb");
    fwrite(apCtx->pcBuffer, sizeof(unsigned char), BUFFER_SIZE/8, fpDATA);
    fclose(fpDATA);
#endif

    return SC_SUCCESS;
}

unsigned char get_bit_value(unsigned char* apcBuffer, int anBitLoc)
{
    unsigned char* pBuf = apcBuffer + (anBitLoc/8);
    int nLeftOverBits = anBitLoc - ((anBitLoc/8)*8);
    if(nLeftOverBits)
    {
        return ((*pBuf >> (8 - nLeftOverBits - 1)) & 0x01);
        
    }
    else
    {
        /** exactly at byte boundary */
        return ((*pBuf >> 7) & 0x01);
    }
}

unsigned char get_byte(unsigned char* apcBuffer, int anBitLoc)
{
    unsigned char nByte = 0;
    for(int i = 0; i < 8; i++)
        nByte |= ((get_bit_value(apcBuffer, anBitLoc+i) << (8 - i -1)));
    return nByte;
}

int match_window_kernel_at(unsigned char* apcBuffer, int anBitLoc, unsigned char* apcKernel, int anKernelSizeBytes)
{
    int nMatch = 0;
    /** match the kernel bits and return score */
    for(int i = 0; i < anKernelSizeBytes; i++)
    {
        LOGD("anBitLoc=%d i =%d %d %x\n", anBitLoc + i*8, i, anKernelSizeBytes, apcKernel[i]);
        int nM = match_window_kernelbyte_at(apcBuffer, anBitLoc + i*8, apcKernel[i]);
        nMatch += nM;
        if(nM != 8)
            break;
    }

    LOGD("nMatch=%d\n", nMatch);
    return nMatch;
}

int match_window_kernels_for_confidence_level(tLISACtx* apCtx)
{
    int nPayloadStart = -1;
    /** return payload start bit location if the confidence level match
     * else return -1 */
    if(!apCtx)
        return nPayloadStart;

    int nNumOfSFBytesToCheck = apCtx->nConfidence;
    int nNumOfTotalChecks = 32 - apCtx->nConfidence + 1;
    int nMatch = 0;
    LOGV("nNumOfSFBytesToCheck=%d\n", apCtx->nConfidence);
    for(int i = 0; i < BUFFER_SIZE; i++) //starting at ith bit in the buffer
    {
        nMatch = 0;
        LOGD("i=%d\n", i);
        for(int j = 0; j + nNumOfSFBytesToCheck <= 32; j++)
        {
            LOGD("j=%d bit=%d byte=%d\n", j, i+(j*8), (i+(j*8))/8);
            print_hex(&apCtx->pcBuffer[(i+(j*8))/8], BUFFER_SIZE - (i+(j*8))/8);
            int nM = match_window_kernel_at((unsigned char*)apCtx->pcBuffer, i + j*8, (unsigned char*)(&KERNEL_STRING[j]), nNumOfSFBytesToCheck);
            nMatch += nM;
            if(nM  == nNumOfSFBytesToCheck*8)
            {
                LOGV("match perfect; iteration=%d [%s]\n", j, &KERNEL_STRING[j]);
                nPayloadStart = i + 32*8;
                return nPayloadStart;
            }
        }
        #if 0
        if(nMatch/8 == nNumOfSFBytesToCheck)
        {
            nPayloadStart = i + 32*8;
            return nPayloadStart;
        }
        #endif
    }

    LOGD("nMatch=%d nMatchBytes=%d nNumOfSFBytesToCheck=%d\n", nMatch, nMatch/8, nNumOfSFBytesToCheck);

    return nPayloadStart;
}

int match_window_kernelbyte_at(unsigned char* apcBuffer, int anBitLoc, unsigned char anKernel)
{
    int nMatch = 0;
    unsigned char nBitVal = 0;
    if(!apcBuffer)
        return nMatch;

    /**  */
    for(int i = 0; i < 8; i++)
    {
        nBitVal = get_bit_value(apcBuffer, anBitLoc + i);
        LOGD("pos=%d nBitVal=%d %d\n", anBitLoc + i, nBitVal, ((anKernel >> (8 - i - 1)) & 0x01));
        if(nBitVal == ((anKernel >> (8 - i - 1)) & 0x01))
        {
            nMatch++;
        }
    }

    return nMatch;
}

void print_hex(unsigned char* apcBuffer, int anLen)
{
    #ifdef PRINT_HEXDUMP
    LOGD("apcBuffer=%p anLen=%d\n", apcBuffer, anLen);
    for(int i = 0; i < anLen; i++)
    {
        LOGE("%x ", apcBuffer[i]);
        if(i % 16 == 0)
            LOGE("\n");
    }
    LOGE("\n");
    #endif /**< PRINT_HEXDUMP */
}

int get_byte_match_pos(unsigned char* apcBuffer, int anStartLoc, int anLen, unsigned char anByte)
{
        for(int nIdx = anStartLoc; nIdx < anLen; nIdx++)
        {
            int nMatch = match_window_kernelbyte_at(apcBuffer, nIdx, anByte);
            LOGD("nMatch=%d\n", nMatch);
            if(nMatch == 8)
            {
                LOGD("full match %d anByte=%x\n", nIdx, anByte);
                return nIdx;
            }
        }
        return -1;
}


int get_payload_start_pos(tLISACtx* apCtx, unsigned char* apcBuffer, int* apnPayloadStart, int anLen)
{
    int nConfidence = 0;
    int nIdx = 0;
    int nLastFullMatchIdx = 0;
    
    for(int i = 0; i < LISA_SYNC_FIELD_LEN_BYTES; i++)
    {
        unsigned char nKernel = (i < (LISA_SYNC_FIELD_LEN_BYTES/2)) ? 0x50 | i : 0xa0 | (i - (LISA_SYNC_FIELD_LEN_BYTES/2));
        if((nIdx = get_byte_match_pos(apcBuffer, 0, anLen, nKernel)) != -1)
        {
            *apnPayloadStart = nIdx + ((LISA_SYNC_FIELD_LEN_BYTES-i)*8);
            nConfidence++;
            nLastFullMatchIdx = nIdx;
            if((((double)nConfidence) / 32) >= apCtx->fSFConfidence)
            {
                LOGD("required confidence achieved at %d\n", nIdx);
                break;
            }
        }
    }

    return nConfidence;
}

int read_data_from_file(tLISACtx* apCtx, tLISADataRaw* apLISADataRaw)
{
    int nIdx = 0;
    int nPayloadStart = 0;
    if(!apCtx || !apLISADataRaw)
        return EC_FAILURE;

#ifdef WRITE_FILE
    /** read from file */
    FILE* fpDATA = fopen(DAT_FILE, "rb");
    fread(apCtx->pcBuffer, sizeof(unsigned char), BUFFER_SIZE/8, fpDATA);
    fclose(fpDATA);
#endif

    print_hex(apCtx->pcBuffer, BUFFER_SIZE/8);

    #if 0
    get_payload_start_pos(apCtx, apCtx->pcBuffer, &nPayloadStart, BUFFER_SIZE);
    #else
    nPayloadStart = match_window_kernels_for_confidence_level(apCtx);
    #endif
    if(nPayloadStart)
    {
        LOGD("payload start at %d bit\n", nPayloadStart);
    }



    //nIdx += (LISA_SYNC_FIELD_LEN_BITS);
    nIdx = nPayloadStart;

    apLISADataRaw->pPayload->nLen = get_byte(apCtx->pcBuffer, nIdx);
    nIdx += 8;

    apLISADataRaw->pPayload->pcPayload = (char*)malloc(apLISADataRaw->pPayload->nLen);
    LOGV("payload len=%d nIdx=%d\n", apLISADataRaw->pPayload->nLen, nIdx);
    for(int i = 0; i < apLISADataRaw->pPayload->nLen; i++)
    {
        apLISADataRaw->pPayload->pcPayload[i] = get_byte(apCtx->pcBuffer, nIdx);
        nIdx += 8;
    }


    if(nPayloadStart == -1)
    {
        LOGE("payload unavailable; try better confidence-percentage\n");
        return EC_FAILURE;
    }

    LOGE("payload=[%s]\n", apLISADataRaw->pPayload->pcPayload);

    return SC_SUCCESS;
}

#if 1
int xLISA(int argc, char* argv[])
{
    tLISACtx*     pLISACtx     = NULL;
    tLISADataRaw* pLISADataRaw = NULL;
    int nOption = 0;

    srand(time(NULL));
    /**
     * bin [0, 1] [data]
     * 0: generate DAT file [fSFCorrruption payload_string]
     * 1: read from the DAT file and print the payload []
     * fSFCorrruption: Sync Field corruption;
     * We have 32 * 8 bits in SF,
     * so corruption can be on any bit;
     * Lets model this in bytes.
     * 32 bytes; if corruption is 1/32 - one of the SF byte is corrupted.
     */
    LOGD("argc=%d\n", argc);
    if(argc < 2)
    {
        return EC_FAILURE;
    }
    nOption = atoi(argv[1]);
    pLISACtx     = (tLISACtx*)calloc(1, sizeof(tLISACtx));
    pLISADataRaw = (tLISADataRaw*)calloc(1, sizeof(tLISADataRaw) + sizeof(tLISAPayload));
    pLISADataRaw->fSFCorrruption = 0.0;
    pLISADataRaw->nBufferSize = BUFFER_SIZE;
    pLISADataRaw->pPayload = (tLISAPayload*)(pLISADataRaw + 1);

    LOGD("nOption=%d\n", nOption);
    switch(nOption)
    {
        case 0:
            pLISADataRaw->fSFCorrruption = (argv[2] && argc > 2) ? atof(argv[2]) : 0.0;
            pLISADataRaw->nBytesToCorrupt = (int)lround((pLISADataRaw->fSFCorrruption / 100) * 32);
            pLISADataRaw->pPayload->pcPayload = (char*)((argv[3] && argc > 2) ? argv[3] : DEFAULT_PAYLOAD);
            pLISADataRaw->pPayload->nLen = strlen(pLISADataRaw->pPayload->pcPayload) + 1;
            create_data_file(pLISACtx, pLISADataRaw);
        break;
        case 1:
            pLISACtx->fSFConfidence = (argv[2] && argc > 2) ? atof(argv[2]) : 100.0;
            pLISACtx->nConfidence = (int)lround((pLISACtx->fSFConfidence / 100) * 32);
            if(pLISACtx->nConfidence < 0 || pLISACtx->nConfidence > 32)
                pLISACtx->nConfidence = 32;
            read_data_from_file(pLISACtx, pLISADataRaw);
        break;
        default:
            return EC_FAILURE;
    }
    
    
    return 0;
}
#endif
