#ifndef _SAMPLE_ASSIST_H_
#define _SAMPLE_ASSIST_H_

#include "cvi_comm_ive.h"
#include "mpi_ive.h"

#define CLIP(a, maxv, minv)		 (((a)>(maxv)) ? (maxv) : (((a) < (minv)) ? (minv) : (a)))

uint32_t CVI_CalcStride(uint32_t u32Width, uint32_t u32Align);
int32_t CVI_CreateIveImage(COMMON_IMAGE_S *pstImage,IVE_IMAGE_TYPE_E enType, uint32_t u32Width, uint32_t u32Height);
int32_t CVI_DestroyIveImage(COMMON_IMAGE_S *pstImage);
void dump_data_u8(const uint8_t *data, int32_t count, const char *desc);
int32_t CVI_CompareIveImage(COMMON_IMAGE_S *pstImage1,COMMON_IMAGE_S *pstImage2);
int32_t CVI_ResetIveImage(COMMON_IMAGE_S *pstImage,uint8_t val);
int32_t CVI_CompareIveMem(IVE_MEM_INFO_S *pstMem1,IVE_MEM_INFO_S *pstMem2);
int32_t CVI_ResetIveMem(IVE_MEM_INFO_S *pstMem,uint8_t val);
int32_t CVI_CompareIveData(IVE_DATA_S *pstData1,IVE_DATA_S *pstData2);
int64_t CVI_GetTickCount(void);
CVI_DOUBLE CVI_GetTickFrequency(void);
int32_t CVI_GenRand(int32_t s32Max,int32_t s32Min);

#endif /*_SAMPLE_ASSIST_H_*/
