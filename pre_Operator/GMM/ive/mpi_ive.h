#ifndef _MPI_IVE_H_
#define _MPI_IVE_H_

#include "cvi_ive_common.h"

int CVI_HW_DetectGMMShadow(
    uint8_t *pix_data, uint8_t *pstModel, uint8_t u8ModelNum, uint8_t IsU8C3,
    uint16_t u0q16BgRatio, uint8_t u0q8ShadowThr, uint8_t u8SnsFactor);


int CVI_HW_RGBGMM(
    uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint8_t *pstFg, uint16_t Fg_u16Stride, uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar, uint16_t u0q16InitWeight,
    uint8_t enDetectShadow, uint8_t u0q8ShadowThr, uint8_t u8SnsFactor);

int CVI_HW_SingleGMM(
    uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint8_t *pstFg, uint16_t Fg_u16Stride, uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar, uint16_t u0q16InitWeight,
    uint8_t enDetectShadow, uint8_t u0q8ShadowThr, uint8_t u8SnsFactor);


int32_t CVI_MPI_IVE_GMM(uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
	uint8_t *pstFg, uint16_t Fg_u16Stride,
	uint8_t *pstBg, uint16_t Bg_u16Stride,
	uint8_t *pstModel, uint8_t u8ModelNum,
	uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
	uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar,
	uint16_t u0q16InitWeight, uint8_t IsU8C3);

int32_t CVI_MPI_IVE_GMM2(uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t *pstFactor, uint16_t Factor_u16Stride,
	uint8_t *pstFg, uint16_t Fg_u16Stride,
	uint8_t *pstBg, uint16_t Bg_u16Stride,
	uint8_t *pstMatchModelInfo, uint16_t MatchModelInfo_u16Stride, uint8_t *pstModel,
	uint8_t u8ModelNum, uint16_t u16LifeThr,
	uint16_t u16FreqInitVal, uint16_t u16FreqReduFactor, uint16_t u16FreqAddFactor, uint16_t u16FreqThr,
	uint16_t u16VarRate, uint16_t u9q7MaxVar, uint16_t u9q7MinVar,
	uint8_t u8GlbSnsFactor, uint8_t enSnsFactorMode, uint16_t u16GlbLifeUpdateFactor, uint8_t enLifeUpdateFactorMode,
	uint16_t ncols, uint16_t nrows, uint8_t IsU8C3);

int CVI_HW_GMM(
    uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint8_t *pstFg, uint16_t Fg_u16Stride, uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar, uint16_t u0q16InitWeight,
    uint8_t enDetectShadow, uint8_t u0q8ShadowThr, uint8_t u8SnsFactor, uint8_t IsU8C3);

int CVI_HW_GMM2(
    uint8_t *pstSrc, uint16_t Src_u16Stride,
    uint16_t *pstFactor, uint16_t Factor_u16Stride,
    uint8_t *pstFg, uint16_t Fg_u16Stride,
    uint8_t *pstBg, uint16_t FgBg_u16Stride,
    uint8_t *pstMatchModelInfo, uint16_t MatchModelInfo_u16Stride, uint8_t *pstModel,
    uint8_t u8ModelNum, uint16_t u16LifeThr,
    uint16_t u16FreqInitVal, uint16_t u16FreqReduFactor, uint16_t u16FreqAddFactor, uint16_t u16FreqThr,
    uint16_t u16VarRate, uint16_t u9q7MaxVar, uint16_t u9q7MinVar,
    uint8_t u8GlbSnsFactor, uint8_t enSnsFactorMode, uint16_t u16GlbLifeUpdateFactor, uint8_t enLifeUpdateFactorMode,
    uint16_t ncols, uint16_t nrows, uint8_t IsU8C3);

#endif
