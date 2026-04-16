#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi_ive.h"
#include "cvi_ive_hw.h"

CVI_S32 CVI_MPI_IVE_GMM(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstFg,
	IVE_DST_IMAGE_S *pstBg, IVE_MEM_INFO_S *pstModel, IVE_GMM_CTRL_S *pstGmmCtrl, CVI_BOOL bInstant)
{
	UNUSED(pIveHandle);
	UNUSED(bInstant);

	//printf("CVI_MPI_IVE_GMM\n");

	//
	// TODO: Here shall be real MPI IVE driver code, which fill in register command, then pass to hw/cmodel
	// FIXME: call CVI_HW_XXX() directly for now
	//
	CVI_HW_GMM(
			(CVI_U8 *)pstSrc->au64VirAddr[0], (CVI_U16)pstSrc->au32Stride[0],
			(CVI_U16)pstSrc->u32Width, (CVI_U16)pstSrc->u32Height,
			(CVI_U8 *)pstModel->u64VirAddr, pstGmmCtrl->u8ModelNum,
			(CVI_U8 *)pstFg->au64VirAddr[0], (CVI_U16)pstFg->au32Stride[0],
			(CVI_U8 *)pstBg->au64VirAddr[0], (CVI_U16)pstBg->au32Stride[0],
			pstGmmCtrl->u0q16LearnRate, pstGmmCtrl->u0q16BgRatio, pstGmmCtrl->u8q8VarThr,
			pstGmmCtrl->u22q10NoiseVar, pstGmmCtrl->u22q10MaxVar, pstGmmCtrl->u22q10MinVar,
			pstGmmCtrl->u0q16InitWeight,
			0, // enDetectShadow
			0, // u0q8ShadowThr
			8, // u8SnsFactor, as from GMM2 sample
			(CVI_U8)((pstSrc->enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) ? 1 : 0));
	return CVI_SUCCESS;
}

CVI_S32 CVI_MPI_IVE_GMM2(IVE_HANDLE *pIveHandle,IVE_SRC_IMAGE_S *pstSrc,IVE_SRC_IMAGE_S *pstFactor,
	IVE_DST_IMAGE_S *pstFg,IVE_DST_IMAGE_S *pstBg,IVE_DST_IMAGE_S *pstMatchModelInfo,
	IVE_MEM_INFO_S  *pstModel,IVE_GMM2_CTRL_S *pstGmm2Ctrl,CVI_BOOL bInstant)
{
	UNUSED(pIveHandle);
	UNUSED(bInstant);

	//printf("CVI_MPI_IVE_GMM2\n");

	//
	// TODO: Here shall be real MPI IVE driver code, which fill in register command, then pass to hw/cmodel
	// FIXME: call CVI_HW_XXX() directly for now
	//
	CVI_HW_GMM2(
			(CVI_U8 *)pstSrc->au64VirAddr[0], (CVI_U16)pstSrc->au32Stride[0],
			(CVI_U16 *)pstFactor->au64VirAddr[0], (CVI_U16)pstFactor->au32Stride[0],
			(CVI_U8 *)pstFg->au64VirAddr[0], (CVI_U16)pstFg->au32Stride[0],
			(CVI_U8 *)pstBg->au64VirAddr[0], (CVI_U16)pstBg->au32Stride[0],
			(CVI_U8 *)pstMatchModelInfo->au64VirAddr[0], (CVI_U16)pstMatchModelInfo->au32Stride[0],
			(CVI_U8 *)pstModel->u64VirAddr,
			pstGmm2Ctrl->u8ModelNum, pstGmm2Ctrl->u16LifeThr,
			pstGmm2Ctrl->u16FreqInitVal, pstGmm2Ctrl->u16FreqReduFactor,
			pstGmm2Ctrl->u16FreqAddFactor, pstGmm2Ctrl->u16FreqThr,
			pstGmm2Ctrl->u16VarRate, pstGmm2Ctrl->u9q7MaxVar, pstGmm2Ctrl->u9q7MinVar,
			pstGmm2Ctrl->u8GlbSnsFactor, pstGmm2Ctrl->enSnsFactorMode,
			pstGmm2Ctrl->u16GlbLifeUpdateFactor, pstGmm2Ctrl->enLifeUpdateFactorMode,
			(CVI_U16)pstSrc->u32Width, (CVI_U16)pstSrc->u32Height,
			(CVI_U8)((pstSrc->enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) ? 1 : 0));
	return CVI_SUCCESS;
}
