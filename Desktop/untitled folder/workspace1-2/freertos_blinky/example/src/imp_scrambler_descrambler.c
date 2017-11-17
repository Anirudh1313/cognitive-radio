/**
 * @file imp_scrambler_descrambler.c
 *
 * @brief Scrambler/De-scrambler implementation
 *
 * @date created Oct 20 2017
 *
 * @author(s) Unnikrishnan Sreekumar
 *
 */

#include "imp_scrambler_descrambler.h"
#include "imp_LISA.h"

bool gFindDXY(uint32_t N, uint32_t* apX, uint32_t* apY)
{
    /** x + y = N;
     * x = y+1 */
    *apX = *apY = 0;
    /** N shall be ODD */
    if((N % 2) == 0)
        return false;
    /** y+1 + y = N;
     * y = (N-1)/2 */
    *apY = (N-1)/2;
    *apX = *apY + 1;
    return true;
}


bool gScrambleData(uint32_t N, uint8_t* apcData, uint32_t anSize, uint8_t* apcDataOut)
{
    bool ret = false;
    uint8_t DX[MAX_X] = {0};
    uint8_t DY[MAX_Y] = {0};
    if(!apcData || !apcDataOut || N > MAX_N)
        return ret;

    uint32_t x,y;
    if(gFindDXY(N, &x, &y) == false)
    {
        return ret;
    }

    /** scramble bit by bit */

    uint8_t S = 0, S3 = 0, S5 = 0, S6 = 0, S0 = 0, T1 = 0;
    for(int i = 0; i < anSize * 8; i++)
    {
        S3 = DX[x-1];
        S5 = DY[y-1];
        //get each bit
        S = get_bit_value(apcData, i);
        S6 = S3 ^ S5;
        S0 = S ^ S6;


        for(int j = x-1; j > 0; j--)
        {
            DX[j] = DX[j-1];
        }
        DX[0] = S0;


        for(int j = y-1; j > 0; j--)
        {
            DY[j] = DY[j-1];
        }
        DY[0] = S3;

        T1 = S0;
        put_bit_value(apcDataOut, i, T1);
    }

    ret = true;
    return ret;
}

bool gDeScrambleData(uint32_t N, uint8_t* apcData, uint32_t anSize, uint8_t* apcDataOut)
{
    bool ret = false;
    uint8_t DX[MAX_X] = {0};
    uint8_t DY[MAX_Y] = {0};
    if(!apcData || !apcDataOut || N > MAX_N)
        return ret;

    uint32_t x,y;
    if(gFindDXY(N, &x, &y) == false)
    {
        return ret;
    }

    /** scramble bit by bit */

    uint8_t S = 0, S3 = 0, S5 = 0, S6 = 0, S0 = 0, R = 0;
    for(int i = 0; i < anSize * 8; i++)
    {
        S3 = DX[x-1];
        S5 = DY[y-1];
        //get each bit
        S = get_bit_value(apcData, i);
        S6 = S3 ^ S5;
        S0 = S;

        for(int j = x-1; j > 0; j--)
        {
            DX[j] = DX[j-1];
        }
        DX[0] = S0;


        for(int j = y-1; j > 0; j--)
        {
            DY[j] = DY[j-1];
        }
        DY[0] = S3;

        R = S6 ^ S0;
        put_bit_value(apcDataOut, i, R);
    }

    ret = true;
    return ret;
}
