#include "cvi_ive_hw.h"

/*
https://github.com/openhumanoids/opencv-pod/blob/master/opencv/modules/video/src/bgfg_gaussmix2.cpp
*/
int CVI_HW_DetectGMMShadow(
	CVI_U8 *pix_data, CVI_U8 *pstModel, CVI_U8 u8ModelNum, CVI_U8 IsU8C3, 
	CVI_U16 u0q16BgRatio, CVI_U8 u0q8ShadowThr, CVI_U8 u8SnsFactor)
{
	CVI_U64 dist2a;
	CVI_S32 dD;
	CVI_U8  a, model_stride;
	CVI_U32 c, i, j;
	CVI_U32 nchannels;
	CVI_U32 denominator;
	CVI_U32 numerator;
	CVI_U16 tWeight;
	CVI_U8  mean[12];
	CVI_U32 g_variance;
	CVI_U16 g_weight;

	tWeight = 0;
	if ( IsU8C3 )
	{
		nchannels = 3;
		model_stride = 11;
	}
	else
	{
		nchannels = 1;
		model_stride = 7;
	}
	for ( i = 0; i < u8ModelNum; ++i )
	{
		numerator = 0;
		denominator = 0;
		for ( j = 0; j < nchannels; ++j )
		{
			mean[j] = pstModel[2 * j + 3];
			numerator += (CVI_U8)mean[j] * pix_data[j];
			denominator += (CVI_U8)mean[j] * (CVI_U8)mean[j];
		}
		if ( !denominator )
			return 0;
		if ( numerator <= denominator && numerator << 8 >= denominator * u0q8ShadowThr )
		{
			a = ((denominator >> 1) + 255 * (CVI_U64)numerator) / denominator;
			dist2a = 0;
			for ( c = 0; c < nchannels; ++c )
			{
				dD = (CVI_U8)mean[c] * a - (pix_data[c] << 8);
				if ( dD < 0 )
					dD = (pix_data[c] << 8) - (CVI_U8)mean[c] * a;
				dist2a += dD * dD;
			}
			g_variance = pstModel[2 * nchannels + 2]
			             + (pstModel[2 * nchannels + 3] << 8)
			             + (pstModel[2 * nchannels + 4] << 16);
			if ( dist2a < (u8SnsFactor * (CVI_U64)g_variance * a * a) >> 8 )
				return 1;
		}
		g_weight = *pstModel + (pstModel[1] << 8);
		tWeight += g_weight;
		if ( tWeight > (int)u0q16BgRatio )
			return 0;
		pstModel += model_stride;
	}
	return 0;
}