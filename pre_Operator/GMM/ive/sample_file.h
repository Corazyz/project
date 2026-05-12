#ifndef SAMPLE_FILE_H_
#define SAMPLE_FILE_H_

#include "cvi_comm_ive.h"
#include <stdio.h>

#ifdef __unix
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),  (mode)))==NULL
#endif

int32_t CVI_ReadFile(IVE_SRC_IMAGE_S *pstImage,FILE *fp);
int32_t CVI_WriteFile(IVE_SRC_IMAGE_S *pstImage,FILE *fp);
int32_t CVI_MemReadFile(IVE_MEM_INFO_S *pstMem,FILE *fp);
int32_t CVI_MemWriteFile(IVE_MEM_INFO_S *pstMem,FILE *fp);
int32_t CVI_DataReadFile(IVE_DATA_S *pstData,FILE *fp);
int32_t CVI_DataWriteFile(IVE_DATA_S *pstData,FILE *fp);

#endif
