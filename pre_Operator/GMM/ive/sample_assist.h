#ifndef _SAMPLE_ASSIST_H_
#define _SAMPLE_ASSIST_H_

#include "cvi_comm_ive.h"
#include "mpi_ive.h"

#define CLIP(a, maxv, minv)		 (((a)>(maxv)) ? (maxv) : (((a) < (minv)) ? (minv) : (a)))

CVI_U32 CVI_CalcStride(CVI_U32 u32Width, CVI_U32 u32Align);
CVI_S32 CVI_CreateIveImage(IVE_IMAGE_S *pstImage,IVE_IMAGE_TYPE_E enType, CVI_U32 u32Width, CVI_U32 u32Height);
CVI_S32 CVI_DestroyIveImage(IVE_IMAGE_S *pstImage);
void dump_data_u8(const CVI_U8 *data, CVI_S32 count, const CVI_CHAR *desc);
CVI_S32 CVI_CompareIveImage(IVE_IMAGE_S *pstImage1,IVE_IMAGE_S *pstImage2);
CVI_S32 CVI_ResetIveImage(IVE_IMAGE_S *pstImage,CVI_U8 val);
CVI_S32 CVI_CompareIveMem(IVE_MEM_INFO_S *pstMem1,IVE_MEM_INFO_S *pstMem2);
CVI_S32 CVI_ResetIveMem(IVE_MEM_INFO_S *pstMem,CVI_U8 val);
CVI_S32 CVI_CompareIveData(IVE_DATA_S *pstData1,IVE_DATA_S *pstData2);
CVI_S64 CVI_GetTickCount(CVI_VOID);
CVI_DOUBLE CVI_GetTickFrequency(CVI_VOID);
CVI_S32 CVI_GenRand(CVI_S32 s32Max,CVI_S32 s32Min);

#endif /*_SAMPLE_ASSIST_H_*/
