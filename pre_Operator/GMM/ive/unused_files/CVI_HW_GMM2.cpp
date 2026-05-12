#include "cvi_ive_hw.h"

/*
https://github.com/opencv/opencv/blob/master/modules/video/src/bgfg_gaussmix2.cpp
MOG2Invoker -> operator()
*/
int CVI_HW_GMM2(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride, 
    CVI_U16 *pstFactor, CVI_U16 Factor_u16Stride, 
    CVI_U8 *pstFg, CVI_U16 Fg_u16Stride, 
    CVI_U8 *pstBg, CVI_U16 FgBg_u16Stride, 
    CVI_U8 *pstMatchModelInfo, CVI_U16 MatchModelInfo_u16Stride, CVI_U8 *pstModel, 
    CVI_U8 u8ModelNum, CVI_U16 u16LifeThr, 
    CVI_U16 u16FreqInitVal, CVI_U16 u16FreqReduFactor, CVI_U16 u16FreqAddFactor, CVI_U16 u16FreqThr, 
    CVI_U16 u16VarRate, CVI_U16 u9q7MaxVar, CVI_U16 u9q7MinVar, 
    CVI_U8 u8GlbSnsFactor, CVI_U8 enSnsFactorMode, CVI_U16 u16GlbLifeUpdateFactor, CVI_U8 enLifeUpdateFactorMode, 
    CVI_U16 ncols, CVI_U16 nrows, CVI_U8 IsU8C3)
{
	int result;
	CVI_U16 *pstFactor0;
	CVI_U8 *pstMatchModelInfo0;
	CVI_U8 *pstFg0;
	CVI_U16 u16LifeUpdateFactor;
	CVI_U8 u8SnsFactor;
	unsigned int y;
	unsigned int x;

	for ( y = 0; ; ++y )
	{
		result = nrows;
		if ( y >= nrows )
			break;
		for ( x = 0; x < ncols; ++x )
		{
			pstFg0 = &pstFg[y * Fg_u16Stride + x];
			pstMatchModelInfo0 = &pstMatchModelInfo[y * MatchModelInfo_u16Stride + x];
			pstFactor0 = &pstFactor[y * Factor_u16Stride + x];
			if ( enSnsFactorMode )
				u8SnsFactor = *pstFactor0;
			else
				u8SnsFactor = u8GlbSnsFactor;
			if ( enLifeUpdateFactorMode )
				u16LifeUpdateFactor = (*pstFactor0 & 0xFF00) >> 8;
			else
				u16LifeUpdateFactor = u16GlbLifeUpdateFactor;
			if ( IsU8C3 )
				CVI_HW_Gmm2_Pixel_Mog(
				    &pstSrc[3 * x + 3 * y * Src_u16Stride],
				    &pstSrc[3 * x + 1 + 3 * y * Src_u16Stride],
				    &pstSrc[3 * x + 2 + 3 * y * Src_u16Stride],
				    pstFg0,
				    &pstBg[3 * x + 3 * y * FgBg_u16Stride],
				    &pstBg[3 * x + 1 + 3 * y * FgBg_u16Stride],
				    &pstBg[3 * x + 2 + 3 * y * FgBg_u16Stride],
				    pstMatchModelInfo0,
				    u16LifeUpdateFactor,
				    u8SnsFactor,
				    &pstModel[12 * u8ModelNum * (x + y * ncols)],
				    u8ModelNum,
				    u16VarRate,
				    u9q7MaxVar,
				    u9q7MinVar,
				    u16FreqReduFactor,
				    u16FreqThr,
				    u16FreqAddFactor,
				    u16FreqInitVal,
				    u16LifeThr,
				    12u,
				    IsU8C3);
			else
				CVI_HW_Gmm2_Pixel_Mog(
				    &pstSrc[y * Src_u16Stride + x],
				    0,
				    0,
				    pstFg0,
				    &pstBg[y * FgBg_u16Stride + x],
				    0,
				    0,
				    pstMatchModelInfo0,
				    u16LifeUpdateFactor,
				    u8SnsFactor,
				    &pstModel[8 * u8ModelNum * (x + y * ncols)],
				    u8ModelNum,
				    u16VarRate,
				    u9q7MaxVar,
				    u9q7MinVar,
				    u16FreqReduFactor,
				    u16FreqThr,
				    u16FreqAddFactor,
				    u16FreqInitVal,
				    u16LifeThr,
				    8u,
				    0);
		}
	}
	return result;
}