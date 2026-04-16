#include <stdio.h>
#include <string.h>
#include "cvi_ive_hw.h"

#define MAX2(a, b)  (( (a) > (b) )? (a) : (b) )
#define MIN2(a, b)  (( (a) < (b) )? (a) : (b) )

/*
https://github.com/opencv/opencv/blob/2.4/modules/video/src/bgfg_gaussmix.cpp
*/
int CVI_HW_RGBGMM(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride, CVI_U16 Src_u16Width, CVI_U16 Src_u16Height,
    CVI_U8 *pstModel, CVI_U8 u8ModelNum,
    CVI_U8 *pstFg, CVI_U16 Fg_u16Stride, CVI_U8 *pstBg, CVI_U16 Bg_u16Stride,
    CVI_U16 u0q16LearnRate, CVI_U16 u0q16BgRatio, CVI_U16 u8q8VarThr,
    CVI_U32 u22q10NoiseVar, CVI_U32 u22q10MaxVar, CVI_U32 u22q10MinVar, CVI_U16 u0q16InitWeight,
    CVI_U8 enDetectShadow, CVI_U8 u0q8ShadowThr, CVI_U8 u8SnsFactor)
{
	int result = 0;
	int min_k_Km1;
	CVI_U8 swapbuf[11];
	CVI_U32 One_div_wscale, sortKey_k1p1, sortKey_k1;
	CVI_S64 diff_c0, diff_c1, diff_c2;
	int k1, k, kForeground, kHit;
	CVI_U8 pix_c0, pix_c1, pix_c2;
	CVI_U32 dw, wsum;
	CVI_U64 dist2, var;
	CVI_U16 x, y, w, mu_c0, mu_c1, mu_c2;
	CVI_U8 *pstBg0;
	CVI_U8 *mptr;
	CVI_U8 *pstFg0;
	CVI_U8 *pstSrc0;

	pstSrc0 = pstSrc;
	pstFg0 = pstFg;
	pstBg0 = pstBg;
	for ( y = 0; y < Src_u16Height ; ++y )
	{
		result = y;
		for ( x = 0; x < Src_u16Width ; ++x )
		{
			kHit = -1;
			kForeground = -1;
			pix_c0 = pstSrc0[3 * x];
			pix_c1 = pstSrc0[3 * x + 1];
			pix_c2 = pstSrc0[3 * x + 2];
			wsum = 0;
			mptr = pstModel;
			for ( k = 0; k < u8ModelNum; ++k )
			{
				mptr = &pstModel[11 * k];
				w = mptr[0] + (mptr[1] << 8);
				wsum += w;
				if ( !w )
					break;  // break condition #1: k will be less than u8ModelNum if break
				mu_c0 = mptr[2] + (mptr[3] << 8);
				mu_c1 = mptr[4] + (mptr[5] << 8);
				mu_c2 = mptr[6] + (mptr[7] << 8);
				var = mptr[8] + (mptr[9] << 8) + (mptr[10] << 16);
				diff_c0 = (pix_c0 << 8) - mu_c0;
				diff_c1 = (pix_c1 << 8) - mu_c1;
				diff_c2 = (pix_c2 << 8) - mu_c2;
				dist2 = (diff_c2 * diff_c2 + diff_c1 * diff_c1 + diff_c0 * diff_c0 + 32) >> 6;
				if ( dist2 < (u8q8VarThr * var + 128) >> 8 )
				{
					wsum -= w;
					dw = ((0xFFFF - w) * (CVI_U32)u0q16LearnRate + 0x8000) >> 16;
					w += dw;
					*(CVI_U16 *)mptr = w;
					mu_c0 += ((CVI_U32)u0q16LearnRate * diff_c0) >> 16;
					mu_c1 += ((CVI_U32)u0q16LearnRate * diff_c1) >> 16;
					mu_c2 += ((CVI_U32)u0q16LearnRate * diff_c2) >> 16;
					*((CVI_U16 *)mptr + 1) = mu_c0;
					*((CVI_U16 *)mptr + 2) = mu_c1;
					*((CVI_U16 *)mptr + 3) = mu_c2;
					var = ((var << 16) + (u0q16LearnRate * (dist2 - var))) >> 16;
					var = MIN2(var, u22q10MaxVar);
					var = MAX2(var, u22q10MinVar);
					mptr[8] = (CVI_U8)(var & 0xFF);
					mptr[9] = (CVI_U8)((var >> 8) & 0xFF);
					mptr[10] = (CVI_U8)((var >> 16) & 0xFF);
					for ( k1 = k - 1; k1 >= 0; --k1 )
					{
						// if( mptr[k1].sortKey >= mptr[k1+1].sortKey )
						//     break;
						// std::swap( mptr[k1], mptr[k1+1] );
						//
						sortKey_k1 = pstModel[11 * k1] + (pstModel[11 * k1 + 1] << 8);
						sortKey_k1p1 = pstModel[11 * k1 + 11] + (pstModel[11 * k1 + 12] << 8);
						if ( sortKey_k1 > sortKey_k1p1 )
							break;
						memcpy(swapbuf, &pstModel[11 * k1], sizeof(swapbuf));
						memcpy(&pstModel[11 * k1], &pstModel[11 * k1 + 11], 0xB);
						memcpy(&pstModel[11 * k1 + 11], swapbuf, 0xB);
					}
					kHit = k1 + 1;
					break;
				}
			}
			// k < u8ModelNum: break condition #1
			if ( kHit >= 0 )
			{
				while ( k < u8ModelNum )
				{
					// wsum += mptr[k].weight;
					w = pstModel[11 * k] + (pstModel[11 * k + 1] << 8);
					wsum += w;
					++k;
				}
			}
			else
			{
				// kHit = k = std::min(k, K-1);
				// wsum += w0 - mptr[k].weight;
				// mptr[k].weight = w0;
				// mptr[k].mean = pix;
				// mptr[k].var = var0;
				// mptr[k].sortKey = sk0;
				//
				if ( k <= u8ModelNum - 1 )
					min_k_Km1 = k;
				else
					min_k_Km1 = u8ModelNum - 1;
				k = min_k_Km1;
				kHit = min_k_Km1;
				w = pstModel[11 * min_k_Km1] + (pstModel[11 * min_k_Km1 + 1] << 8);
				wsum += u0q16InitWeight - w;
				pstModel[11 * k] = u0q16InitWeight;
				pstModel[11 * k + 1] = (u0q16InitWeight >> 8);
				pstModel[11 * k + 3] = pix_c0;
				pstModel[11 * k + 2] = 0;
				pstModel[11 * k + 5] = pix_c1;
				pstModel[11 * k + 4] = 0;
				pstModel[11 * k + 7] = pix_c2;
				pstModel[11 * k + 6] = 0;
				var = 4 * u22q10NoiseVar;
				pstModel[11 * k + 10] = (CVI_U8)((var >> 16) & 0xFF);
				pstModel[11 * k + 9]  = (CVI_U8)((var >> 8) & 0xFF);
				pstModel[11 * k + 8]  = (CVI_U8)(var & 0xFF);
			}
			One_div_wscale = wsum;
			wsum = 0;
			for ( k = 0; k < u8ModelNum; ++k )
			{
				// wsum += mptr[k].weight *= wscale;
				w = pstModel[11 * k] + (pstModel[11 * k + 1] << 8);
				w = 0xFFFF * (CVI_U32)w / One_div_wscale;
				*(CVI_U16 *)&pstModel[11 * k] = w;
				wsum += w;
				if ( wsum > u0q16BgRatio && kForeground < 0 )
					kForeground = k + 1;
			}
			if ( enDetectShadow )
			{
				pstFg0[x] = (kHit < kForeground) - 1;
				if ( (CVI_U8)pstFg0[x] == 255
				        && CVI_HW_DetectGMMShadow(
				            (CVI_U8 *)&pstSrc0[3 * x],
				            pstModel,
				            u8ModelNum,
				            1,
				            u0q16BgRatio,
				            u0q8ShadowThr,
				            u8SnsFactor) )
				{
					pstFg0[x] = 127;
				}
			}
			else
			{
				pstFg0[x] = (kHit >= kForeground)? 0xFF : 0;
			}
			pstBg0[3 * x] = pstModel[3];
			pstBg0[3 * x + 1] = pstModel[5];
			pstBg0[3 * x + 2] = pstModel[7];
			pstModel += 11 * u8ModelNum;
		}
		pstSrc0 += 3 * Src_u16Stride;
		pstFg0 += Fg_u16Stride;
		pstBg0 += 3 * Bg_u16Stride;
	}
	return result;
}
