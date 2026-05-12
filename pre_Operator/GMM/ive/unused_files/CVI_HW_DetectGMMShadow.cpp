#include "cvi_ive_hw.h"

/*
https://github.com/openhumanoids/opencv-pod/blob/master/opencv/modules/video/src/bgfg_gaussmix2.cpp
*/
int CVI_HW_DetectGMMShadow(
	uint8_t *pix_data, uint8_t *pstModel, uint8_t u8ModelNum, uint8_t IsU8C3,
	uint16_t u0q16BgRatio, uint8_t u0q8ShadowThr, uint8_t u8SnsFactor)
{
	uint64_t dist2a;
	int32_t dD;
	uint8_t  a, model_stride;
	uint32_t c, i, j;
	uint32_t nchannels;
	uint32_t denominator;
	uint32_t numerator;
	uint16_t tWeight;
	uint8_t  mean[12];
	uint32_t g_variance;
	uint16_t g_weight;

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
			numerator += (uint8_t)mean[j] * pix_data[j];
			denominator += (uint8_t)mean[j] * (uint8_t)mean[j];
		}
		if ( !denominator )
			return 0;
		if ( numerator <= denominator && numerator << 8 >= denominator * u0q8ShadowThr )
		{
			a = ((denominator >> 1) + 255 * (uint64_t)numerator) / denominator;
			dist2a = 0;
			for ( c = 0; c < nchannels; ++c )
			{
				dD = (uint8_t)mean[c] * a - (pix_data[c] << 8);
				if ( dD < 0 )
					dD = (pix_data[c] << 8) - (uint8_t)mean[c] * a;
				dist2a += dD * dD;
			}
			g_variance = pstModel[2 * nchannels + 2]
			             + (pstModel[2 * nchannels + 3] << 8)
			             + (pstModel[2 * nchannels + 4] << 16);
			if ( dist2a < (u8SnsFactor * (uint64_t)g_variance * a * a) >> 8 )
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