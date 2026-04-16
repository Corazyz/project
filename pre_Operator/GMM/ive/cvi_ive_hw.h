#include "cvi_ive.h"
#include "cvi_comm_ive.h"

int CVI_HW_Add(
    CVI_U8 *pstSrc1, CVI_U16 Src1_u16Stride,
    CVI_U8 *pstSrc2, CVI_U16 Src2_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_U0Q16 u0q16X, CVI_U0Q16 u0q16Y);

int CVI_HW_AlphaBetaBlending(
    IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2, 
	IVE_SRC_IMAGE_S *pstAlpha, IVE_SRC_IMAGE_S *pstBeta, 
	IVE_DST_IMAGE_S *pstDst);

int CVI_HW_AlphaBetaBlending_v2(
    IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2, 
	IVE_SRC_IMAGE_S *pstAlpha, IVE_SRC_IMAGE_S *pstBeta, 
	IVE_DST_IMAGE_S *pstDst);

int CVI_HW_AlphaBetaBlending_v3(
    IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2, 
	IVE_SRC_IMAGE_S *pstAlpha, IVE_SRC_IMAGE_S *pstBeta,
    IVE_DST_IMAGE_S *pstDst, CVI_U16 *bxl, CVI_U16 *bxr);

int CVI_HW_AlphaBetaBlending_yuv444p(
    IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2, 
	IVE_SRC_IMAGE_S *pstAlpha, IVE_SRC_IMAGE_S *pstBeta,
    IVE_DST_IMAGE_S *pstOut, CVI_U16 *bxl, CVI_U16 *bxr);

    
int CVI_HW_And(
    CVI_U8 *pstSrc1, CVI_U16 Src1_u16Stride,
    CVI_U8 *pstSrc2, CVI_U16 Src2_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height);

int CVI_HW_Or(
    CVI_U8 *pstSrc1, CVI_U16 Src1_u16Stride,
    CVI_U8 *pstSrc2, CVI_U16 Src2_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height);

int CVI_HW_Sub(
    CVI_U8 *pstSrc1, CVI_U16 Src1_u16Stride,
    CVI_U8 *pstSrc2, CVI_U16 Src2_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_U8 enMode);

int CVI_HW_Xor(
    CVI_U8 *pstSrc1, CVI_U16 Src1_u16Stride,
    CVI_U8 *pstSrc2, CVI_U16 Src2_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height);

int CVI_HW_Thresh(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_U8 lowThr, CVI_U8 highThr,
    CVI_U8 minVal, CVI_U8 midVal, CVI_U8 maxVal,
    IVE_THRESH_MODE_E enMode);

int CVI_HW_Thresh_S16(
    CVI_S16 *pstSrc, CVI_U16 Src_u16Stride,
    IVE_8BIT_U *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_S16 lowThr, CVI_S16 highThr,
    IVE_8BIT_U minVal, IVE_8BIT_U midVal, IVE_8BIT_U maxVal,
    IVE_THRESH_S16_MODE_E enMode);

int CVI_HW_Thresh_U16(
    CVI_U16 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_U16 lowThr, CVI_U16 highThr,
    CVI_U8 minVal, CVI_U8 midVal, CVI_U8 maxVal,
    IVE_THRESH_U16_MODE_E enMode);

int CVI_HW_DMA(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U8 u8HorSegSize, CVI_U8 u8ElemSize, CVI_U8 u8VerSegRows,
    CVI_U32 u64Val_L32b, CVI_U32 u64Val_H32b,
    CVI_U16 u16Width, CVI_U16 u16Height, CVI_U8 enMode);

int CVI_HW_16BitTo8Bit(
    CVI_U16 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_U8 enMode, CVI_U16 u8Num_div_u16Den, CVI_S8 s8Bias);

int CVI_HW_CSC(
    CVI_U8 **pstSrc, CVI_U16 *Src_u16Stride,
    CVI_U8 **pstDst, CVI_U16 *Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_U8 enSrcType, CVI_U8 enMode, CVI_U8 enDstType);

int CVI_HW_UnpackUV(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst,
    CVI_U16 u16Width, CVI_U16 u16Height);

int CVI_HW_RepackUV(
    CVI_U8 *pstSrc,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height);

int CVI_HW_Filter(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_S8 *au8Mask, CVI_U8 u8Norm);

int CVI_HW_Hist(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U32 *pstDst, CVI_U16 u16Width, CVI_U16 u16Height);

int CVI_HW_Map(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U8 *pstMap, IVE_MAP_MODE_E enMode,
    CVI_U16 u16Width, CVI_U16 u16Height);

int CVI_HW_NCC(
    CVI_U8 *pstSrc1, CVI_U16 Src1_u16Stride,
    CVI_U8 *pstSrc2, CVI_U16 Src2_u16Stride,
    CVI_U64 *u64Numerator, CVI_U64 *u64QuadSum1, CVI_U64 *u64QuadSum2,
    CVI_U16 SrcWidth, CVI_U16 SrcHeight);

int CVI_HW_OrdStatFilter(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    IVE_ORD_STAT_FILTER_MODE_E enMode);

void CVI_HW_Resize(
    CVI_U8 **pstSrc, CVI_U16 *Src_u16Stride,
    CVI_U8 **pstDst, CVI_U16 *Dst_u16Stride,
    CVI_U16 Src_u16Width, CVI_U16 Src_u16Height,
    CVI_U16 Dst_u16Width, CVI_U16 Dst_u16Height,
    CVI_U16 scale_x, CVI_U16 scale_y,
    CVI_U8 nChannels, IVE_RESIZE_MODE_E enMode);

void CVI_HW_ResizeUnit(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 Src_u16Width, CVI_U16 Src_u16Height,
    CVI_U16 Dst_u16Width, CVI_U16 Dst_u16Height,
    CVI_U16 scale_x, CVI_U16 scale_y, CVI_U8 enMode);

CVI_U16 CVI_HW_Resize_ComputeResizeAreaTab(
    CVI_U16 ssize, CVI_U16 dsize, CVI_U16 scale, CVI_U16 *tab);

int CVI_HW_Resize_Bilinear(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U16 Src_u16Width, CVI_U16 Src_u16Height,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 Dst_u16Width, CVI_U16 Dst_u16Height,
    CVI_U16 xmax, CVI_U16 *alpha, CVI_U16 *beta,
    CVI_U16 *xofs, CVI_U16 *yofs, int ksize, CVI_U32 *buffer_data);

int CVI_HW_Resize_Bilinear_Hor(
    CVI_U8 *pstSrc[], CVI_U32 *pstDst[],
    int count, CVI_U16 *xofs, CVI_U16 *alpha, int Src_u16Width,
    CVI_U16 Dst_u16Width, CVI_U16 xmax);

int CVI_HW_Resize_Bilinear_Ver(
    CVI_U32 *pstSrc[], CVI_U8 *pstDst, CVI_U16 *beta, CVI_U16 u16Width);

int CVI_HW_Resize_Area(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U16 Src_u16Width, CVI_U16 Src_u16Height,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 Dst_u16Width, CVI_U16 Dst_u16Height,
    CVI_U16 *xtab, unsigned int xtab_size,
    CVI_U16 *ytab, int ytab_size,
    CVI_U16 j_start, CVI_U16 j_end, CVI_U16 *buf);

int CVI_HW_Resize_AreaFast(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U16 Src_u16Width, CVI_U16 Src_u16Height,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 Dst_u16Width, CVI_U16 Dst_u16Height,
    CVI_U16 *ofs, CVI_U16 *xofs, CVI_U16 scale_x, CVI_U32 scale_y, CVI_U16 area);

int CVI_HW_CannyHysEdge(
    CVI_U8 *pstSrcAng, CVI_U16 SrcAng_u16Stride,
    CVI_U16 *pstSrcMag, CVI_U16 SrcMag_u16Stride,
    CVI_U8 *pstEdge, CVI_U16 EdgeStride,
    CVI_U16 *pstStack, CVI_U32 *u32StackSize,
    CVI_U16 u16LowThr, CVI_U16 u16HighThr,
    CVI_U16 Src_u16Width, CVI_U16 Src_u16Height);

int CVI_HW_Integ(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U32 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height, IVE_INTEG_OUT_CTRL_E enOutCtrl);

int CVI_HW_LBP(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    IVE_8BIT_U un8BitThr, CVI_U8 enMode);

int CVI_HW_NormGrad(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_S8 *pstDstH, CVI_U16 DstH_u16Stride,
    CVI_S8 *pstDstV, CVI_U16 DstV_u16Stride,
    CVI_S8 *pstDstHV, CVI_U16 DstHV_u16Stride,
    CVI_S8 *as8Mask, CVI_U16 Src_u16Width, CVI_U16 Src_u16Height,
    CVI_U8 u8Norm, IVE_NORM_GRAD_OUT_CTRL_E enOutCtrl);

int CVI_HW_STBoxFltAndEigCalc(
	CVI_S16 *pstGrad_S8C2, CVI_U16 Grad_u16Stride,
	CVI_U16 *pstEigen, CVI_U16 Eigen_u16Stride,
	CVI_U16 Src_u16Width, CVI_U16 Src_u16Height,
	CVI_U16 *u16MaxEig);

int CVI_HW_STCandiCorner(
    CVI_U8 *pstPrev, CVI_U16 Prev_u16Stride,
    CVI_U8 *pstCurr, CVI_U16 Curr_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 Src_u16Width, CVI_U16 Src_u16Height);

int CVI_HW_Sobel(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U16 *pstDstMag, CVI_U16 DstMag_u16Stride,
    CVI_U8 *pstDstAng, CVI_U16 DstAng_u16Stride,
    CVI_U16 *pstDstH, CVI_U16 DstH_u16Stride,
    CVI_U16 *pstDstV, CVI_U16 DstV_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_S8 *as8Mask, CVI_U16 u16Thr);

int CVI_HW_MagAndAng_Top(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U16 *pstDstMag, CVI_U16 DstMag_u16Stride,
    CVI_U8 *pstDstAng, CVI_U16 DstAng_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_S8 *as8Mask, CVI_U16 u16Thr, IVE_MAG_AND_ANG_OUT_CTRL_E enOutCtrl);

int CVI_HW_Bernsen(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_U8 u8WinSize, CVI_U8 u8Thr, CVI_U8 u8ContrastThreshold,
    IVE_BERNSEN_MODE_E enMode);

CVI_U16 *CVI_HW_CCL(
    CVI_U8 *pstSrcDst, CVI_U16 SrcDst_u16Stride,
    CVI_U16 SrcDst_u16Width, CVI_U16 SrcDst_u16Height,
    IVE_REGION_S *pstRegion, CVI_U16 *u16CurAreaThr,
    CVI_U16 u16InitAreaThr, CVI_U16 u16Step,
    CVI_S8 *s8LabelStatus, CVI_U8 *u8RegionNum, CVI_U8 enMode);

int CVI_HW_Dilate(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_U8 *au8Mask);

int CVI_HW_Erode(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride,
    CVI_U8 *pstDst, CVI_U16 Dst_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_U8 *au8Mask);

int *CVI_HW_MatchBgModel(
    CVI_U16 pstFgFlag_u16Stride,
    CVI_U8 *pstCurImg,
    CVI_U8 *pstBgModel,
    int a4,
    CVI_U16 pstBgDiffFg_u16Stride,
    CVI_U8 *pstFgFlag,
    CVI_S8 *pstBgDiffFg,
    CVI_S8 *pstFrmDiffFg,
    CVI_U16 BgModel_u16Stride,
    CVI_U16 u16Width,
    CVI_U16 u16Height,
    CVI_U32 *pstStatData,
    CVI_U32 u32CurFrmNum,
    CVI_U32 u32PreFrmNum,
    CVI_U16 u16TimeThr,
    CVI_U8 u8DiffThrCrlCoef,
    CVI_U8 u8DiffMaxThr,
    CVI_U8 u8DiffMinThr,
    CVI_U8 u8DiffThrInc,
    CVI_U8 u8FastLearnRate,
    CVI_U8 u8DetChgRegion);

int *CVI_HW_UpdateBgModel(
    CVI_U8 *pstBgModel,
    CVI_U16 BgModel_u16Stride,
    CVI_U8 *pstFgFlag,
    CVI_U8 *pstBgImg,
    CVI_U8 *pstChgStaImg,
    CVI_S8 *pstChgStaFg,
    CVI_U16 FgFlag_u16Stride,
    CVI_U16 *pstChgStaLife,
    CVI_U16 ChgStaLife_u16Stride,
    CVI_U16 u16Width,
    CVI_U16 u16Height,
    CVI_U32 *pstStatData,
    CVI_U32 u32CurFrmNum,
    CVI_U32 u32PreChkTime,
    CVI_U32 u32FrmChkPeriod,
    CVI_U32 u32InitMinTime,
    CVI_U32 u32StyBgMinBlendTime,
    CVI_U32 u32StyBgMaxBlendTime,
    CVI_U32 u32DynBgMinBlendTime,
    CVI_U16 u16FgMaxFadeTime,
    CVI_U16 u16BgMaxFadeTime,
    CVI_U32 u32StaticDetMinTime,
    CVI_U8 u8StyBgAccTimeRateThr,
    CVI_U8 u8ChgBgAccTimeRateThr,
    CVI_U8 u8DynBgAccTimeThr,
    CVI_U8 u8DynBgDepth,
    CVI_U8 u8BgEffStaRateThr,
    CVI_U8 u8AcceBgLearn,
    CVI_U8 u8DetChgRegion);

int CVI_HW_GMM(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride, CVI_U16 Src_u16Width, CVI_U16 Src_u16Height,
    CVI_U8 *pstModel, CVI_U8 u8ModelNum,
    CVI_U8 *pstFg, CVI_U16 Fg_u16Stride, CVI_U8 *pstBg, CVI_U16 Bg_u16Stride,
    CVI_U16 u0q16LearnRate, CVI_U16 u0q16BgRatio, CVI_U16 u8q8VarThr,
    CVI_U32 u22q10NoiseVar, CVI_U32 u22q10MaxVar, CVI_U32 u22q10MinVar, CVI_U16 u0q16InitWeight,
    CVI_U8 enDetectShadow, CVI_U8 u0q8ShadowThr, CVI_U8 u8SnsFactor, CVI_U8 IsU8C3);

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
    CVI_U16 ncols, CVI_U16 nrows, CVI_U8 IsU8C3);

CVI_U8 *CVI_HW_Gmm2_Pixel_Mog(
    CVI_U8 *pstSrc0, CVI_U8 *pstSrc1, CVI_U8 *pstSrc2,
    CVI_U8 *pstFg, CVI_U8 *pstBg0, CVI_U8 *pstBg1, CVI_U8 *pstBg2, CVI_U8 *pstMatchModelInfo,
    CVI_U16 u16LifeUpdateFactor, CVI_U8 u8SnsFactor, CVI_U8 *pstModel, CVI_U8 u8ModelNum,
    CVI_U16 u16VarRate, CVI_U16 varMax, CVI_U16 varMin,
    CVI_U16 u16FreqReduFactor, CVI_U16 u16FreqThr, CVI_U16 u16FreqAddFactor, CVI_U16 u16FreqInitVal,
    CVI_U16 u16LifeThr, CVI_U8 Model_u16Stride, CVI_U8 IsU8C3);

int CVI_HW_GradFg(
    CVI_S8 *pstBgDiffFg, CVI_U16 BgDiffFg_u16Stride,
    CVI_S8 *pstCurGrad, CVI_U16 CurGrad_u16Stride,
    CVI_S8 *pstBgGrad, CVI_U16 BgGrad_u16Stride,
    CVI_U8 *pstGradFg, CVI_U16 GradFg_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    IVE_GRAD_FG_MODE_E enMode, CVI_U16 u16EdwFactor,
    CVI_U8 u8CrlCoefThr, CVI_U8 u8MagCrlThr,
    CVI_U8 u8MinMagDiff, CVI_U8 u8NoiseVal, CVI_U8 u8EdwDark);

int CVI_HW_SAD(
    CVI_U8 *pstSrc1, CVI_U16 Src1_u16Stride,
    CVI_U8 *pstSrc2, CVI_U16 Src2_u16Stride,
    CVI_U16 *pstSad, CVI_U16 Sad_u16Stride,
    CVI_U8 *pstThr, CVI_U16 Thr_u16Stride,
    CVI_U16 u16Width, CVI_U16 u16Height,
    CVI_U16 u16Thr, CVI_U8 u8MinVal, CVI_U8 u8MaxVal,
    IVE_SAD_OUT_CTRL_E enOutCtrl, IVE_SAD_MODE_E enMode);

CVI_U8 *CVI_HW_Sort3ElemAscend(CVI_U8 *Min, CVI_U8 *Mid, CVI_U8 *Max);

int CVI_HW_DetectGMMShadow(
    CVI_U8 *pix_data, CVI_U8 *pstModel, CVI_U8 u8ModelNum, CVI_U8 IsU8C3,
    CVI_U16 u0q16BgRatio, CVI_U8 u0q8ShadowThr, CVI_U8 u8SnsFactor);

int CVI_HW_RGBGMM(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride, CVI_U16 Src_u16Width, CVI_U16 Src_u16Height,
    CVI_U8 *pstModel, CVI_U8 u8ModelNum,
    CVI_U8 *pstFg, CVI_U16 Fg_u16Stride, CVI_U8 *pstBg, CVI_U16 Bg_u16Stride,
    CVI_U16 u0q16LearnRate, CVI_U16 u0q16BgRatio, CVI_U16 u8q8VarThr,
    CVI_U32 u22q10NoiseVar, CVI_U32 u22q10MaxVar, CVI_U32 u22q10MinVar, CVI_U16 u0q16InitWeight,
    CVI_U8 enDetectShadow, CVI_U8 u0q8ShadowThr, CVI_U8 u8SnsFactor);

int CVI_HW_SingleGMM(
    CVI_U8 *pstSrc, CVI_U16 Src_u16Stride, CVI_U16 Src_u16Width, CVI_U16 Src_u16Height,
    CVI_U8 *pstModel, CVI_U8 u8ModelNum,
    CVI_U8 *pstFg, CVI_U16 Fg_u16Stride, CVI_U8 *pstBg, CVI_U16 Bg_u16Stride,
    CVI_U16 u0q16LearnRate, CVI_U16 u0q16BgRatio, CVI_U16 u8q8VarThr,
    CVI_U32 u22q10NoiseVar, CVI_U32 u22q10MaxVar, CVI_U32 u22q10MinVar, CVI_U16 u0q16InitWeight,
    CVI_U8 enDetectShadow, CVI_U8 u0q8ShadowThr, CVI_U8 u8SnsFactor);

int CVI_HW_LKOpticalFlowPyr(
    CVI_U8 **astSrcPrevPyr, CVI_U16 *SrcPrevPyr_u16Stride,
    CVI_U8 **astSrcNextPyr, CVI_U16 *SrcNextPyr_u16Stride,
    CVI_U16 Src_u16Width, CVI_U16 Src_u16Height, CVI_U8 u8MaxLevel,
    IVE_POINT_S25Q7_S *pstPrevPts, IVE_POINT_S25Q7_S *pstNextPts,
    CVI_U8 *pstStatus, CVI_U16 *pstErr,
    CVI_U16 u16PtsNum, CVI_U8 u8IterCnt, CVI_U8 u0q8Eps, CVI_U8 u0q8MinEigThr,
    CVI_U8 bUseInitFlow, CVI_U8 enOutMode);

