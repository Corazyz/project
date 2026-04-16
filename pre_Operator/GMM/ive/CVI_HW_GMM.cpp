#include "cvi_ive_hw.h"

int CVI_HW_GMM(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride, CVI_U16 Src_u16Width, CVI_U16 Src_u16Height, 
    CVI_U8 *pstModel, CVI_U8 u8ModelNum, 
    CVI_U8 *pstFg, CVI_U16 Fg_u16Stride, CVI_U8 *pstBg, CVI_U16 Bg_u16Stride, 
    CVI_U16 u0q16LearnRate, CVI_U16 u0q16BgRatio, CVI_U16 u8q8VarThr, 
    CVI_U32 u22q10NoiseVar, CVI_U32 u22q10MaxVar, CVI_U32 u22q10MinVar, CVI_U16 u0q16InitWeight, 
    CVI_U8 enDetectShadow, CVI_U8 u0q8ShadowThr, CVI_U8 u8SnsFactor, CVI_U8 IsU8C3)
{
	int result;

	if ( IsU8C3 )
		result = CVI_HW_RGBGMM(
		             pstSrc,
		             Src_u16Stride,
		             Src_u16Width,
		             Src_u16Height,
		             pstModel,
		             u8ModelNum,
		             pstFg,
		             Fg_u16Stride,
		             pstBg,
		             Bg_u16Stride,
		             u0q16LearnRate,
		             u0q16BgRatio,
		             u8q8VarThr,
		             u22q10NoiseVar,
		             u22q10MaxVar,
		             u22q10MinVar,
		             u0q16InitWeight,
		             enDetectShadow,
		             u0q8ShadowThr,
		             u8SnsFactor);
	else
		result = CVI_HW_SingleGMM(
		             pstSrc,
		             Src_u16Stride,
		             Src_u16Width,
		             Src_u16Height,
		             pstModel,
		             u8ModelNum,
		             pstFg,
		             Fg_u16Stride,
		             pstBg,
		             Bg_u16Stride,
		             u0q16LearnRate,
		             u0q16BgRatio,
		             u8q8VarThr,
		             u22q10NoiseVar,
		             u22q10MaxVar,
		             u22q10MinVar,
		             u0q16InitWeight,
		             enDetectShadow,
		             u0q8ShadowThr,
		             u8SnsFactor);
	return result;
}