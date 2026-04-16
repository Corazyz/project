#ifndef _CVI_MPI_IVE_H_
#define _CVI_MPI_IVE_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include "cvi_ive.h"

CVI_S32 CVI_MPI_IVE_DMA(IVE_HANDLE *pIveHandle, IVE_DATA_S *pstSrc,
	IVE_DST_DATA_S *pstDst, IVE_DMA_CTRL_S *pstDmaCtrl,CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Filter(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
    IVE_DST_IMAGE_S *pstDst, IVE_FILTER_CTRL_S *pstFltCtrl,CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_CSC(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
    IVE_DST_IMAGE_S *pstDst, IVE_CSC_CTRL_S *pstCscCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_FilterAndCSC(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
        IVE_DST_IMAGE_S *pstDst, IVE_FILTER_AND_CSC_CTRL_S *pstFltCscCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Sobel(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
        IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
        IVE_SOBEL_CTRL_S *pstSobelCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_MagAndAng(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
        IVE_DST_IMAGE_S *pstDstMag, IVE_DST_IMAGE_S *pstDstAng,
        IVE_MAG_AND_ANG_CTRL_S *pstMagAndAngCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Dilate(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
    IVE_DST_IMAGE_S *pstDst, IVE_DILATE_CTRL_S *pstDilateCtrl,CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Erode(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
    IVE_DST_IMAGE_S *pstDst, IVE_ERODE_CTRL_S *pstErodeCtrl,CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Thresh(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
    IVE_DST_IMAGE_S *pstDst, IVE_THRESH_CTRL_S *pstThrCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_And(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Sub(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst, IVE_SUB_CTRL_S *pstSubCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Or(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
    IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Integ(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
	IVE_DST_IMAGE_S *pstDst, IVE_INTEG_CTRL_S *pstIntegCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Hist(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
    IVE_DST_MEM_INFO_S *pstDst, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Thresh_S16(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
	IVE_DST_IMAGE_S *pstDst, IVE_THRESH_S16_CTRL_S *pstThrS16Ctrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Thresh_U16(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
	IVE_DST_IMAGE_S *pstDst, IVE_THRESH_U16_CTRL_S *pstThrU16Ctrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_16BitTo8Bit(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
	IVE_DST_IMAGE_S *pstDst, IVE_16BIT_TO_8BIT_CTRL_S *pst16BitTo8BitCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_OrdStatFilter(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
	IVE_DST_IMAGE_S *pstDst, IVE_ORD_STAT_FILTER_CTRL_S *pstOrdStatFltCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Map(IVE_HANDLE *pIveHandle,IVE_SRC_IMAGE_S *pstSrc,
	IVE_SRC_MEM_INFO_S *pstMap, IVE_DST_IMAGE_S *pstDst,IVE_MAP_CTRL_S *pstMapCtrl,CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_EqualizeHist(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
	IVE_DST_IMAGE_S *pstDst, IVE_EQUALIZE_HIST_CTRL_S *pstEqualizeHistCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Add(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
	IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst, IVE_ADD_CTRL_S *pstAddCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Xor(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
	IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_NCC(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
	IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_MEM_INFO_S *pstDst, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_CCL(IVE_HANDLE *pIveHandle, IVE_IMAGE_S *pstSrcDst,
	IVE_DST_MEM_INFO_S *pstBlob, IVE_CCL_CTRL_S *pstCclCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_GMM(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstFg,
	IVE_DST_IMAGE_S *pstBg, IVE_MEM_INFO_S *pstModel, IVE_GMM_CTRL_S *pstGmmCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_GMM2(IVE_HANDLE *pIveHandle,IVE_SRC_IMAGE_S *pstSrc,IVE_SRC_IMAGE_S *pstFactor,
	IVE_DST_IMAGE_S *pstFg,IVE_DST_IMAGE_S *pstBg,IVE_DST_IMAGE_S *pstMatchModelInfo,
	IVE_MEM_INFO_S  *pstModel,IVE_GMM2_CTRL_S *pstGmm2Ctrl,CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_CannyHysEdge(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstEdge,
       IVE_DST_MEM_INFO_S *pstStack, IVE_CANNY_HYS_EDGE_CTRL_S *pstCannyHysEdgeCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_CannyEdge(IVE_IMAGE_S *pstEdge, IVE_MEM_INFO_S *pstStack);

CVI_S32 CVI_MPI_IVE_LBP(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
	IVE_DST_IMAGE_S *pstDst, IVE_LBP_CTRL_S *pstLbpCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_NormGrad(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
	IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV, IVE_DST_IMAGE_S *pstDstHV,
	IVE_NORM_GRAD_CTRL_S *pstNormGradCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_LKOpticalFlowPyr(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S astSrcPrevPyr[], IVE_SRC_IMAGE_S astSrcNextPyr[],
    IVE_SRC_MEM_INFO_S *pstPrevPts, IVE_MEM_INFO_S *pstNextPts, IVE_DST_MEM_INFO_S *pstStatus, IVE_DST_MEM_INFO_S *pstErr,
    IVE_LK_OPTICAL_FLOW_PYR_CTRL_S *pstLkOptiFlowPyrCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_STCandiCorner(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstCandiCorner,
	IVE_ST_CANDI_CORNER_CTRL_S *pstStCandiCornerCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_STCorner(IVE_SRC_IMAGE_S * pstCandiCorner, IVE_DST_MEM_INFO_S *pstCorner,
    IVE_ST_CORNER_CTRL_S *pstStCornerCtrl);

CVI_S32 CVI_MPI_IVE_GradFg(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstBgDiffFg, IVE_SRC_IMAGE_S *pstCurGrad,
	IVE_SRC_IMAGE_S *pstBgGrad, IVE_DST_IMAGE_S *pstGradFg, IVE_GRAD_FG_CTRL_S *pstGradFgCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_MatchBgModel(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstCurImg, IVE_DATA_S *pstBgModel,
	IVE_IMAGE_S *pstFgFlag, IVE_DST_IMAGE_S *pstBgDiffFg, IVE_DST_IMAGE_S *pstFrmDiffFg, IVE_DST_MEM_INFO_S *pstStatData,
	IVE_MATCH_BG_MODEL_CTRL_S *pstMatchBgModelCtrl,	CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_UpdateBgModel(IVE_HANDLE *pIveHandle,	IVE_DATA_S *pstBgModel, IVE_IMAGE_S *pstFgFlag,
	IVE_DST_IMAGE_S *pstBgImg, IVE_DST_IMAGE_S *pstChgStaImg, IVE_DST_IMAGE_S *pstChgStaFg, IVE_DST_IMAGE_S *pstChgStaLife,
	IVE_DST_MEM_INFO_S *pstStatData, IVE_UPDATE_BG_MODEL_CTRL_S *pstUpdateBgModelCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_ANN_MLP_LoadModel(const CVI_CHAR *pchFileName, IVE_ANN_MLP_MODEL_S *pstAnnMlpModel);

CVI_VOID CVI_MPI_IVE_ANN_MLP_UnloadModel(IVE_ANN_MLP_MODEL_S *pstAnnMlpModel);

CVI_S32 CVI_MPI_IVE_ANN_MLP_Predict(IVE_HANDLE *pIveHandle, IVE_SRC_DATA_S *pstSrc,
    IVE_LOOK_UP_TABLE_S *pstActivFuncTab, IVE_ANN_MLP_MODEL_S *pstAnnMlpModel,
    IVE_DST_DATA_S *pstDst, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_SVM_LoadModel(const CVI_CHAR *pchFileName, IVE_SVM_MODEL_S *pstSvmModel);

CVI_VOID CVI_MPI_IVE_SVM_UnloadModel(IVE_SVM_MODEL_S *pstSvmModel);

CVI_S32 CVI_MPI_IVE_SVM_Predict(IVE_HANDLE *pIveHandle, IVE_SRC_DATA_S *pstSrc,
    IVE_LOOK_UP_TABLE_S *pstKernelTab, IVE_SVM_MODEL_S *pstSvmModel,
    IVE_DST_DATA_S *pstDstVote, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_SAD(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
	IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstSad,IVE_DST_IMAGE_S *pstThr,
	IVE_SAD_CTRL_S *pstSadCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_Resize(IVE_HANDLE *pIveHandle,IVE_SRC_IMAGE_S astSrc[],
	IVE_DST_IMAGE_S astDst[],IVE_RESIZE_CTRL_S *pstResizeCtrl,CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_CNN_LoadModel(const CVI_CHAR *pchFileName, IVE_CNN_MODEL_S *pstCnnModel);

CVI_VOID CVI_MPI_IVE_CNN_UnloadModel(IVE_CNN_MODEL_S *pstCnnModel);

CVI_S32 CVI_MPI_IVE_CNN_Predict(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S astSrc[],
	IVE_CNN_MODEL_S *pstCnnModel, IVE_DST_DATA_S *pstDst,
	IVE_CNN_CTRL_S *pstCnnCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_CNN_GetResult(IVE_SRC_DATA_S *pstSrc,  IVE_DST_MEM_INFO_S *pstDst,
	IVE_CNN_MODEL_S *pstCnnModel, IVE_CNN_CTRL_S *pstCnnCtrl);

CVI_S32 CVI_MPI_IVE_Query(IVE_HANDLE IveHandle, CVI_BOOL *pbFinish, CVI_BOOL bBlock);

CVI_S32 CVI_MPI_IVE_Bernsen(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
	IVE_DST_IMAGE_S *pstDst, IVE_BERNSEN_CTRL_S *pstLbpCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_AlphaBetaBlending(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
	IVE_SRC_IMAGE_S *pstSrc2, IVE_SRC_IMAGE_S *pstAlpha, IVE_SRC_IMAGE_S *pstBeta,
	IVE_DST_IMAGE_S *pstDst, IVE_STIT_MASK_U16_S *pstStitMaskCtrl, CVI_BOOL bInstant);

CVI_S32 CVI_MPI_IVE_AlphaBetaBlending_yuv444p(IVE_HANDLE *pIveHandle,
    IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
	IVE_SRC_IMAGE_S *pstAlpha, IVE_SRC_IMAGE_S *pstBeta,
	IVE_DST_IMAGE_S *pstDst, IVE_STIT_MASK_U16_S *pstStitMaskCtrl, CVI_BOOL bInstant);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif/*__MPI_IVE_H__*/
