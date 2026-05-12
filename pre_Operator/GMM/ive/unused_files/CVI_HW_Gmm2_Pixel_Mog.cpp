#include <stdio.h>
#include <string.h>
#include "cvi_ive_hw.h"

#define MAX2(a, b)  (( (a) > (b) )? (a) : (b) )
#define MIN2(a, b)  (( (a) < (b) )? (a) : (b) )
#define GMM2_MAX_MODEL_NUM    (7)

/*
https://github.com/opencv/opencv/blob/master/modules/video/src/bgfg_gaussmix2.cpp
MOG2Invoker -> operator() -> codes inside the (x,y) for loop
*/
CVI_U8 *CVI_HW_Gmm2_Pixel_Mog(
    CVI_U8 *pstSrc0, CVI_U8 *pstSrc1, CVI_U8 *pstSrc2,
    CVI_U8 *pstFg, CVI_U8 *pstBg0, CVI_U8 *pstBg1, CVI_U8 *pstBg2, CVI_U8 *pstMatchModelInfo,
    CVI_U16 u16LifeUpdateFactor, CVI_U8 u8SnsFactor, CVI_U8 *pstModel, CVI_U8 u8ModelNum,
    CVI_U16 u16VarRate, CVI_U16 u9q7MaxVar, CVI_U16 u9q7MinVar,
    CVI_U16 u16FreqReduFactor, CVI_U16 u16FreqThr, CVI_U16 u16FreqAddFactor, CVI_U16 u16FreqInitVal,
    CVI_U16 u16LifeThr, CVI_U8 Model_u16Stride, CVI_U8 IsU8C3)
{
	CVI_U8 *result;
	CVI_U8 curr_ptr = 0;
	CVI_S8 mode, mode1 = 0;
    int   i, tmp, flag, IsBg, Keep2Fg, KeepModel;
	CVI_U16 Life[10], Freq[10], mean[10], variance[10],mean_src[3 * GMM2_MAX_MODEL_NUM];
	CVI_U8  gmm[16];
	CVI_U16 var0, varnew, var;
	CVI_U32 newFreq0, newFreq;
	CVI_U32 newLife0, newLife;
	CVI_U32 dist2;
	CVI_U32 dData[3];
	CVI_U32 dData0_pow2, dData1_pow2, dData2_pow2;
	CVI_U16 data[3];
	CVI_U8 Bg0, Bg1, Bg2;

	CVI_ASSERT(u8ModelNum <= GMM2_MAX_MODEL_NUM);

	Bg0 = 0;
	Bg1 = 0;
	Bg2 = 0;
	Keep2Fg = 1;
	KeepModel = 0;

	for ( mode = 0; mode < u8ModelNum; ++mode )
	{
		curr_ptr = Model_u16Stride * mode;
		mean_src[3 * mode] = pstModel[curr_ptr] + (pstModel[curr_ptr + 1] << 8);
		if ( IsU8C3 )
		{
			mean_src[3 * mode + 1] = pstModel[curr_ptr + 2] + (pstModel[curr_ptr + 3] << 8);
			mean_src[3 * mode + 2] = pstModel[curr_ptr + 4] + (pstModel[curr_ptr + 5] << 8);
			variance[mode] = pstModel[curr_ptr + 6] + (pstModel[curr_ptr + 7] << 8);
			Freq[mode] = pstModel[curr_ptr + 8] + (pstModel[curr_ptr + 9] << 8);
    		Life[mode] = pstModel[curr_ptr + 10] + (pstModel[curr_ptr + 11] << 8);
		}
		else
		{
			variance[mode] = pstModel[curr_ptr + 2] + (pstModel[curr_ptr + 3] << 8);
			Freq[mode] = pstModel[curr_ptr + 4] + (pstModel[curr_ptr + 5] << 8);
    		Life[mode] = pstModel[curr_ptr + 6] + (pstModel[curr_ptr + 7] << 8);
		}
		mean[mode] = Freq[mode];
		gmm[mode] = mode;
	}

	mode = u8ModelNum - 2;
	flag = 1;
	while ( mode >= 0 && flag )
	{
		flag = 0;
		for ( i = 0; i <= mode; ++i )
		{
			if ( mean[i + 1] > mean[i] )
			{
				flag = 1;
				tmp = mean[i];  // swap mean[i] and mean[i+1]
				mean[i] = mean[i + 1];
				mean[i + 1] = tmp;
				tmp = gmm[i];   // Line: 674, swap(gmm[i], gmm[i+1])
				gmm[i] = gmm[i + 1];
				gmm[i + 1] = tmp;
			}
		}
		--mode;
	}

	for ( mode = 0; mode < u8ModelNum; ++mode )
	{
		curr_ptr = Model_u16Stride * mode;
		newFreq0 = ((CVI_U32)Freq[mode] * (CVI_U32)u16FreqReduFactor) >> 16;
        newFreq = MIN2(newFreq0, 0xFFFF);
		Freq[mode] = newFreq;
		memcpy(&pstModel[curr_ptr - 4 + Model_u16Stride], &Freq[mode], 2);
		if ( u8ModelNum > 1 )
		{
			newLife0 = (CVI_U32)Life[mode] + (CVI_U32)u16LifeUpdateFactor;
			newLife = MIN2(newLife0, 0xFFFF);
			Life[mode] = newLife;
			memcpy(&pstModel[curr_ptr - 2 + Model_u16Stride], &Life[mode], 2);
		}
	}

	for ( mode = 0; mode < u8ModelNum; ++mode )
	{
		mode1 = gmm[mode];
		newFreq0 = (CVI_U32)Freq[mode1];
		newLife0 = (CVI_U32)Life[mode1];
		var0     = (CVI_U16)variance[mode1];
		data[0]  = mean_src[3 * mode1];
		data[1]  = mean_src[3 * mode1 + 1];
		data[2]  = mean_src[3 * mode1 + 2];
		dData[0] = (CVI_U16)*pstSrc0 - (data[0] >> 7);
		dData0_pow2 = dData[0] * dData[0];
		if ( IsU8C3 )
		{
			dData[1] = (CVI_U16)*pstSrc1 - (data[1] >> 7);
			dData1_pow2 = dData[1] * dData[1];
			dData[2] = (CVI_U16)*pstSrc2 - (data[2] >> 7);
			dData2_pow2 = dData[2] * dData[2];
			dist2 = dData2_pow2 + dData1_pow2 + dData0_pow2;
			var = var0 >> 7;
			IsBg = dist2 < ((CVI_U32)u8SnsFactor * 3 * (CVI_U32)var);
		}
		else
		{
			dist2 = dData0_pow2;
			var = var0 >> 7;
			IsBg = dist2 < ((CVI_U32)u8SnsFactor * (CVI_U32)var);
		}
		IsBg &= u16FreqThr < newFreq0;
		if ( IsBg )
		{
			var0 += (dist2 - var) * u16VarRate;     // Line: 657, u16VarRate = alphaT/weight
            varnew = MAX2( MIN2( MAX2(var0, u9q7MinVar), u9q7MaxVar), u9q7MinVar);
			variance[mode1] = varnew;
			curr_ptr = Model_u16Stride * mode1;
			memcpy(&pstModel[(CVI_U8)(Model_u16Stride * mode1) - 6 + Model_u16Stride], &variance[mode1], 2);// gmm[mode].variance = varnew;
			mean_src[3 * mode1] += dData[0];
			if ( IsU8C3 )
			{
				// update src
				mean_src[3 * mode1 + 1] += dData[1];
				mean_src[3 * mode1 + 2] += dData[2];
				memcpy(&pstModel[curr_ptr], &mean_src[3 * mode1], 2);
				memcpy(&pstModel[curr_ptr + 2], &mean_src[3 * mode1 + 1], 2);
				memcpy(&pstModel[curr_ptr + 4], &mean_src[3 * mode1 + 2], 2);
			}
			else
			{
				memcpy(&pstModel[curr_ptr], &mean_src[3 * mode1], 2);
			}
			if ( u8ModelNum <= 1 )          newFreq0 = u16FreqInitVal;
			else
			{
				if ( u16FreqAddFactor )     newFreq0 += u16FreqAddFactor;
				else
				{
					if ( mode )				newFreq0 = u16FreqInitVal;
					else
					{
						if ( u16FreqInitVal + 10000 <= 65535 )
							                newFreq0 = u16FreqInitVal + 10000;
						else
							                newFreq0 = 65535;
					}
				}
                newFreq0 = MIN2(newFreq0, 0xFFFF);
			}
			Freq[mode1] = newFreq0;
			memcpy(&pstModel[curr_ptr - 4 + Model_u16Stride], &Freq[mode1], 2);
			KeepModel = 1;
			if ( u8ModelNum > 1 )
			{
				Keep2Fg = newLife0 < (int)u16LifeThr;
				break;
			}
			Keep2Fg = 0;
		}
	}
	if ( u8ModelNum <= 1 )
	{
		if ( (CVI_U16)Freq[0] < (int)u16FreqThr )
		{
			variance[0] = u9q7MaxVar;
			Freq[0] = u16FreqInitVal;
			memcpy(&pstModel[Model_u16Stride - 6], variance, 2);    // Init variance
			memcpy(&pstModel[Model_u16Stride - 4], Freq, 2);        // Init mean
			mean_src[3 * mode1] = *pstSrc0 << 7;                    // Init model
			if ( IsU8C3 )
			{
				mean_src[3 * mode1 + 1] = *pstSrc1 << 7;
				mean_src[3 * mode1 + 2] = *pstSrc2 << 7;
				memcpy(&pstModel[curr_ptr], &mean_src[3 * mode1], 2);
				memcpy(&pstModel[curr_ptr + 2], &mean_src[3 * mode1 + 1], 2);
				memcpy(&pstModel[curr_ptr + 4], &mean_src[3 * mode1 + 2], 2);
			}
			else
			{
				memcpy(&pstModel[curr_ptr], &mean_src[3 * mode1], 2);
			}
		}
	}
	else if ( !KeepModel )
	{
        mode1 = gmm[u8ModelNum-1];
		variance[mode1] = u9q7MaxVar;
		Freq[mode1] = u16FreqInitVal;
		Life[mode1] = 1;
		curr_ptr = Model_u16Stride * mode1;
		memcpy(&pstModel[(CVI_U8)(Model_u16Stride * mode1) - 6 + Model_u16Stride], &variance[mode1], 2);
		memcpy(&pstModel[curr_ptr - 4 + Model_u16Stride], &Freq[mode1], 2);
		memcpy(&pstModel[curr_ptr - 2 + Model_u16Stride], &Life[mode1], 2);
		if ( IsU8C3 )
		{
			mean_src[3 * mode1] = *pstSrc0 << 7;
			mean_src[3 * mode1 + 1] = *pstSrc1 << 7;
			mean_src[3 * mode1 + 2] = *pstSrc2 << 7;
			memcpy(&pstModel[curr_ptr], &mean_src[3 * mode1], 2);
			memcpy(&pstModel[curr_ptr + 2], &mean_src[3 * mode1 + 1], 2);
			memcpy(&pstModel[curr_ptr + 4], &mean_src[3 * mode1 + 2], 2);
		}
		else
		{
			mean_src[3 * mode1] = *pstSrc0 << 7;
			memcpy(&pstModel[curr_ptr], &mean_src[3 * mode1], 2);
		}
	}
	*pstFg = (Keep2Fg == 1)? 0xFF : 0; //-Keep2Fg;
	if ( u8ModelNum <= 1u )
	{
		Bg0 = (int)mean_src[0] >> 7;
		Bg1 = (int)mean_src[1] >> 7;
		Bg2 = (int)mean_src[2] >> 7;
	}
	else
	{
		for ( mode = 0; mode < (int)u8ModelNum; ++mode )
		{
			mode1 = gmm[mode];
			if ( (CVI_U16)Life[mode1] > (int)u16LifeThr )
			{
				Bg0 = (int)mean_src[3 * mode1] >> 7;
				Bg1 = (int)mean_src[3 * mode1 + 1] >> 7;
				Bg2 = (int)mean_src[3 * mode1 + 2] >> 7;
				break;
			}
		}
	}
	*pstBg0 = Bg0;
	if ( IsU8C3 )
	{
		*pstBg1 = Bg1;
		*pstBg2 = Bg2;
	}
	result = pstMatchModelInfo;
	*pstMatchModelInfo = (KeepModel & 0x1) + 2 * gmm[0];
	return result;
}