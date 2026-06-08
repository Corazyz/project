#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#ifdef __unix
#include <unistd.h>
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/* ===== From cvi_type.h ===== */

#ifndef NULL
#define NULL                0L
#endif

#define CVI_NULL            0L
#define CVI_NULL_PTR        0L

#define CVI_SUCCESS         0
#define CVI_FAILURE         (-1)
#define CVI_ASSERT(a)       assert(a)
#define UNUSED(x)           (void)(x)

#define CVI_INVALID_HANDLE  (-1)
#define CVI_DBG_ERR         3

typedef int32_t IVE_HANDLE;
typedef int32_t CLS_HANDLE;
typedef int32_t FD_CHN;
typedef int32_t MD_CHN;

/* ===== From cvi_errno.h ===== */

#define CVI_ERR_APPID  (0x80000000L + 0x20000000L)

typedef enum hiERR_LEVEL_E
{
	EN_ERR_LEVEL_DEBUG = 0,
	EN_ERR_LEVEL_INFO,
	EN_ERR_LEVEL_NOTICE,
	EN_ERR_LEVEL_WARNING,
	EN_ERR_LEVEL_ERROR,
	EN_ERR_LEVEL_CRIT,
	EN_ERR_LEVEL_ALERT,
	EN_ERR_LEVEL_FATAL,
	EN_ERR_LEVEL_BUTT
} ERR_LEVEL_E;

#define CVI_DEF_ERR( module, level, errid) \
	((CVI_S32)( (CVI_ERR_APPID) | ((module) << 16 ) | ((level)<<13) | (errid) ))

typedef enum _EN_ERR_CODE_E
{
    EN_ERR_INVALID_DEVID = 1,
    EN_ERR_INVALID_CHNID = 2,
    EN_ERR_ILLEGAL_PARAM = 3,
    EN_ERR_EXIST         = 4,
    EN_ERR_UNEXIST       = 5,
    EN_ERR_NULL_PTR      = 6,
    EN_ERR_NOT_CONFIG    = 7,
    EN_ERR_NOT_SUPPORT   = 8,
    EN_ERR_NOT_PERM      = 9,
    EN_ERR_NOMEM         = 12,
    EN_ERR_NOBUF         = 13,
    EN_ERR_BUF_EMPTY     = 14,
    EN_ERR_BUF_FULL      = 15,
    EN_ERR_SYS_NOTREADY  = 16,
    EN_ERR_BADADDR       = 17,
    EN_ERR_BUSY          = 18,
    EN_ERR_BUTT          = 63,
} EN_ERR_CODE_E;

typedef enum _MOD_ID_E
{
    CVI_ID_CMPI    = 0,
    CVI_ID_VB      = 1,
    CVI_ID_SYS     = 2,
    CVI_ID_VALG    = 3,
    CVI_ID_CHNL    = 4,
    CVI_ID_VDEC    = 5,
    CVI_ID_GROUP   = 6,
    CVI_ID_VENC    = 7,
    CVI_ID_VPSS    = 8,
    CVI_ID_VDA     = 9,
    CVI_ID_H264E   = 10,
    CVI_ID_JPEGE   = 11,
    CVI_ID_MPEG4E  = 12,
    CVI_ID_H264D   = 13,
    CVI_ID_JPEGD   = 14,
    CVI_ID_VOU     = 15,
    CVI_ID_VIU     = 16,
    CVI_ID_DSU     = 17,
    CVI_ID_RGN     = 18,
    CVI_ID_RC      = 19,
    CVI_ID_SIO     = 20,
    CVI_ID_AI      = 21,
    CVI_ID_AO      = 22,
    CVI_ID_AENC    = 23,
    CVI_ID_ADEC    = 24,
    CVI_ID_AVENC   = 25,
    CVI_ID_PCIV    = 26,
    CVI_ID_PCIVFMW = 27,
    CVI_ID_ISP     = 28,
    CVI_ID_IVE     = 29,
    CVI_ID_DCCM    = 31,
    CVI_ID_DCCS    = 32,
    CVI_ID_PROC    = 33,
    CVI_ID_LOG     = 34,
    CVI_ID_MST_LOG = 35,
    CVI_ID_VD      = 36,
    CVI_ID_VCMP    = 38,
    CVI_ID_FB      = 39,
    CVI_ID_HDMI    = 40,
    CVI_ID_VOIE    = 41,
    CVI_ID_TDE     = 42,
    CVI_ID_USR     = 43,
    CVI_ID_VEDU    = 44,
    CVI_ID_FD      = 47,
    CVI_ID_ODT     = 48,
    CVI_ID_VQA     = 49,
    CVI_ID_LPR     = 50,
    CVI_ID_BUTT,
} MOD_ID_E;

/* ===== From cvi_ive.h ===== */

typedef enum {
    CVI_FALSE    = 0,
    CVI_TRUE     = 1,
} CVI_BOOL;

typedef struct _IVE_MEM_INFO_S
{
	uint64_t  u64PhyAddr;
	uint64_t  u64VirAddr;
	uint32_t  u32Size;
} IVE_MEM_INFO_S;

typedef IVE_MEM_INFO_S IVE_SRC_MEM_INFO_S;
typedef IVE_MEM_INFO_S IVE_DST_MEM_INFO_S;

typedef struct _IVE_GMM_CTRL_S
{
    unsigned int    u22q10NoiseVar;
    unsigned int    u22q10MaxVar;
    unsigned int    u22q10MinVar;
    unsigned short  u0q16LearnRate;
    unsigned short  u0q16BgRatio;
    unsigned short  u8q8VarThr;
    unsigned short  u0q16InitWeight;
    uint8_t         u8ModelNum;
} IVE_GMM_CTRL_S;

typedef enum _IVE_GMM2_SNS_FACTOR_MODE_E
{
	IVE_GMM2_SNS_FACTOR_MODE_GLB   =  0x0,
	IVE_GMM2_SNS_FACTOR_MODE_PIX   =  0x1,
	IVE_GMM2_SNS_FACTOR_MODE_BUTT
} IVE_GMM2_SNS_FACTOR_MODE_E;

typedef enum _IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_E
{
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_GLB  =  0x0,
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_PIX  =  0x1,
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_BUTT
} IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_E;

typedef struct _IVE_GMM2_CTRL_S
{
	IVE_GMM2_SNS_FACTOR_MODE_E			enSnsFactorMode;
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_E	enLifeUpdateFactorMode;
	uint16_t							u16GlbLifeUpdateFactor;
	uint16_t							u16LifeThr;
	uint16_t							u16FreqInitVal;
	uint16_t							u16FreqReduFactor;
	uint16_t							u16FreqAddFactor;
	uint16_t							u16FreqThr;
	uint16_t							u16VarRate;
	unsigned short						u9q7MaxVar;
	unsigned short						u9q7MinVar;
	uint8_t								u8GlbSnsFactor;
	uint8_t								u8ModelNum;
} IVE_GMM2_CTRL_S;

typedef enum _IVE_IMAGE_TYPE_E
{
	IVE_IMAGE_TYPE_U8C1           =  0x0,
	IVE_IMAGE_TYPE_U8C3_PACKAGE   =  0xa,
	IVE_IMAGE_TYPE_BUTT
} IVE_IMAGE_TYPE_E;

typedef struct _COMMON_IMAGE_S
{
	uint64_t  au64PhyAddr[3];
	uint64_t  au64VirAddr[3];
	uint32_t  au32Stride[3];
	uint32_t  u32Width;
	uint32_t  u32Height;
	IVE_IMAGE_TYPE_E  enType;
} COMMON_IMAGE_S;

typedef COMMON_IMAGE_S IVE_IMAGE_S;
typedef COMMON_IMAGE_S IVE_SRC_IMAGE_S;
typedef COMMON_IMAGE_S IVE_DST_IMAGE_S;

/* ===== From sample_define.h ===== */

#define _MAX_FNAME NAME_MAX

#define CVI_CHECK_ET_RET(srcData,value,ret)\
do{\
	if((value) == (srcData))\
	{\
		return (ret);\
	}\
}while(0)

#define CVI_CHECK_ET_NULL_RET(ptr,ret) CVI_CHECK_ET_RET(ptr,NULL,ret)

#define CVI_FCLOSE(fp)\
do{\
	if (NULL != (fp))\
	{\
		fclose((fp));\
		(fp) = NULL;\
	}\
} while (0)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

/* ===== Function Declarations ===== */

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
    uint16_t u0q16InitWeight);

int32_t GMM_Sample_U8C1(int show, int compare);

/* ===== mpi_ive.cpp Implementation ===== */

#define MAX2(a, b)  (( (a) > (b) )? (a) : (b) )
#define MIN2(a, b)  (( (a) < (b) )? (a) : (b) )

int CVI_HW_SingleGMM(
    uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint8_t *pstFg, uint16_t Fg_u16Stride, uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar, uint16_t u0q16InitWeight,
    uint8_t enDetectShadow, uint8_t u0q8ShadowThr, uint8_t u8SnsFactor)
{
    int result = 0;
    int min_k_Km1;
    uint8_t swapbuf[7];
    uint32_t One_div_wscale, sortKey_k1p1, sortKey_k1;
    int64_t diff;
    int k1, k, kForeground, kHit;
    uint8_t pix;
    uint32_t dw, wsum;
    uint64_t dist2, var;
    uint16_t x, y, w, mu;
    uint8_t *pstBg0;
    uint8_t *mptr;
    uint8_t *pstFg0;
    uint8_t *pstSrc0;

    pstSrc0 = pstSrc;
    pstFg0 = pstFg;
    pstBg0 = pstBg;
    for ( y = 0; y < Src_u16Height ; ++y )
    {
        result = y;
        for ( x = 0; x < Src_u16Width ; ++x )
        {
            kHit = -1;
            kForeground = -1;
            pix = pstSrc0[x];
            wsum = 0;
            mptr = pstModel;
            for ( k = 0; k < u8ModelNum; ++k )
            {
                mptr = &pstModel[7 * k];
                w = mptr[0] + (mptr[1] << 8);
                wsum += w;
                if ( !w )
                    break;
                mu = mptr[2] + (mptr[3] << 8);
                var = mptr[4] + (mptr[5] << 8) + (mptr[6] << 16);
                diff = (pix << 8) - mu;
                dist2 = (diff * diff + 32) >> 6;
                if ( dist2 < (u8q8VarThr * var + 128) >> 8 )
                {
                    wsum -= w;
                    dw = ((0xFFFF - w) * (uint32_t)u0q16LearnRate + 0x8000) >> 16;
                    w += dw;
                    *(uint16_t *)mptr = w;
                    mu += ((u0q16LearnRate * diff) >> 16);
                    *((uint16_t *)mptr + 1) = mu;
                    var = ((var << 16) + (u0q16LearnRate * (dist2 - var))) >> 16;
                    var = MIN2(var, u22q10MaxVar);
                    var = MAX2(var, u22q10MinVar);
                    mptr[4] = (uint8_t)(var & 0xFF);
                    mptr[5] = (uint8_t)((var >> 8) & 0xFF);
                    mptr[6] = (uint8_t)((var >> 16) & 0xFF);
                    for ( k1 = k - 1; k1 >= 0; --k1 )
                    {
                        sortKey_k1   = pstModel[7 * k1] + (pstModel[7 * k1 + 1] << 8);
                        sortKey_k1p1 = pstModel[7 * k1 + 7] + (pstModel[7 * k1 + 8] << 8);
                        if ( sortKey_k1 > sortKey_k1p1 )
                            break;
                        memcpy(swapbuf, &pstModel[7 * k1], 7);
                        memcpy(&pstModel[7 * k1], &pstModel[7 * k1 + 7], 7);
                        memcpy(&pstModel[7 * k1 + 7], swapbuf, 7);
                    }
                    kHit = k1 + 1;
                    break;
                }
            }
            if ( kHit >= 0 )
            {
                while ( k < u8ModelNum )
                {
                    w = pstModel[7 * k] + (pstModel[7 * k + 1] << 8);
                    wsum += w;
                    ++k;
                }
            }
            else
            {
                if ( k <= u8ModelNum - 1 )
                    min_k_Km1 = k;
                else
                    min_k_Km1 = u8ModelNum - 1;
                k = min_k_Km1;
                kHit = min_k_Km1;
                w = pstModel[7 * min_k_Km1] + (pstModel[7 * min_k_Km1 + 1] << 8);
                wsum += u0q16InitWeight - w;
                pstModel[7 * k] = u0q16InitWeight;
                pstModel[7 * k + 1] = (u0q16InitWeight >> 8);
                pstModel[7 * k + 3] = pix;
                pstModel[7 * k + 2] = 0;
                var = 4 * u22q10NoiseVar;
                pstModel[7 * k + 6] = (uint8_t)((var >> 16) & 0xFF);
                pstModel[7 * k + 5]  = (uint8_t)((var >> 8) & 0xFF);
                pstModel[7 * k + 4]  = (uint8_t)(var & 0xFF);
            }
            One_div_wscale = wsum;
            wsum = 0;
            for ( k = 0; k < u8ModelNum; ++k )
            {
                w = pstModel[7 * k] + (pstModel[7 * k + 1] << 8);
                w = 0xFFFF * (uint32_t)w / One_div_wscale;
                *(uint16_t *)&pstModel[7 * k] = w;
                wsum += w;
                if ( wsum > u0q16BgRatio && kForeground < 0 )
                    kForeground = k + 1;
            }
            if ( enDetectShadow )
            {
            }
            else
            {
                pstFg0[x] = (kHit >= kForeground)? 0xFF : 0;
            }
            pstBg0[x] = pstModel[3];
            pstModel += 7 * u8ModelNum;
        }
        pstSrc0 += Src_u16Stride;
        pstFg0 += Fg_u16Stride;
        pstBg0 += Bg_u16Stride;
    }
    return result;
}

int32_t CVI_MPI_IVE_GMM(uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstFg, uint16_t Fg_u16Stride,
    uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar,
    uint16_t u0q16InitWeight)
{
    CVI_HW_SingleGMM(
            pstSrc, Src_u16Stride,
            Src_u16Width, Src_u16Height,
            pstModel, u8ModelNum,
            pstFg, Fg_u16Stride,
            pstBg, Bg_u16Stride,
            u0q16LearnRate, u0q16BgRatio, u8q8VarThr,
            u22q10NoiseVar, u22q10MaxVar, u22q10MinVar,
            u0q16InitWeight,
            0, 0, 8);
    return 0;
}

/* ===== GMM.cpp Implementation ===== */

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(path) mkdir(path, 0777)
#endif

#ifdef __unix
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),  (mode)))==NULL
#endif

bool directoryExists(const char* path) {
    #ifdef _WIN32
        struct _stat info;
        return (_stat(path, &info) == 0 && (info.st_mode & _S_IFDIR));
    #else
        struct stat info;
        return (stat(path, &info) == 0 && (info.st_mode & S_IFDIR));
    #endif
}

void createDirectory(const char* path) {
    if (!directoryExists(path)) {
        MKDIR(path);
    }
}

using namespace std;
using namespace cv;

enum {
	CVGRAY_COMMON_U8C1 = 0,
	COMMON_U8C1_CVGRAY = 3,
};

typedef struct _COMMON_MEM_INFO_S {
	uint8_t* pData;
	uint32_t u32Size;
} COMMON_MEM_INFO_S;

typedef struct _COMMON_GMM_CTRL_S {
	uint16_t u0q16LearnRate;
	uint16_t u0q16BgRatio;
	uint16_t u8q8VarThr;
	uint32_t u22q10NoiseVar;
	uint32_t u22q10MaxVar;
	uint32_t u22q10MinVar;
	uint16_t u0q16InitWeight;
	uint8_t  u8ModelNum;
} COMMON_GMM_CTRL_S;

int32_t CVI_ReadFile(IVE_SRC_IMAGE_S *pstImage,FILE *fp)
{
	uint16_t y;
	int32_t u32Succ;
	uint8_t *pData;

	CVI_CHECK_ET_NULL_RET(pstImage,CVI_FAILURE);
	CVI_CHECK_ET_NULL_RET(fp,CVI_FAILURE);

	if (feof(fp))
	{
		fseek(fp, 0 , SEEK_SET);
	}

	u32Succ = CVI_SUCCESS;

	switch(pstImage->enType)
	{
	case IVE_IMAGE_TYPE_U8C1:
		{
			pData = (uint8_t*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0])
			{
				if (1 != fread(pData,pstImage->u32Width,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	default:
		u32Succ = CVI_FAILURE;
		break;
	}

	return u32Succ;
}

static int32_t Common_CreateImage(COMMON_IMAGE_S* pstImg, IVE_IMAGE_TYPE_E enType, uint32_t u32Width, uint32_t u32Height)
{
	uint32_t u32Stride = u32Width;
	pstImg->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height);
	if (!pstImg->au64VirAddr[0]) return CVI_FAILURE;
	memset((void*)pstImg->au64VirAddr[0], 0, u32Stride * u32Height);
	pstImg->au64PhyAddr[0] = 0;
	pstImg->au32Stride[0] = u32Stride;
	pstImg->u32Width = u32Width;
	pstImg->u32Height = u32Height;
	pstImg->enType = enType;
	pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[2] = 0;
	pstImg->au64VirAddr[1] = pstImg->au64VirAddr[2] = 0;
	pstImg->au32Stride[1] = pstImg->au32Stride[2] = 0;
	return CVI_SUCCESS;
}

static void Common_DestroyImage(COMMON_IMAGE_S* pstImg)
{
	if (pstImg->au64VirAddr[0]) {
		free((void*)pstImg->au64VirAddr[0]);
		pstImg->au64VirAddr[0] = 0;
	}
}

static int32_t Common_CreateMem(COMMON_MEM_INFO_S* pstMem, uint32_t u32Size)
{
	pstMem->pData = (uint8_t*)malloc(u32Size);
	if (!pstMem->pData) return CVI_FAILURE;
	memset(pstMem->pData, 0, u32Size);
	pstMem->u32Size = u32Size;
	return CVI_SUCCESS;
}

static void Common_DestroyMem(COMMON_MEM_INFO_S* pstMem)
{
	if (pstMem->pData) {
		free(pstMem->pData);
		pstMem->pData = NULL;
	}
}

static int32_t CVI_Common_GMM(COMMON_IMAGE_S *pstSrc, COMMON_IMAGE_S *pstFg,
	COMMON_IMAGE_S *pstBg, COMMON_MEM_INFO_S *pstModel, COMMON_GMM_CTRL_S *pstCtrl)
{
	return CVI_MPI_IVE_GMM(
			(uint8_t*)pstSrc->au64VirAddr[0], (uint16_t)pstSrc->au32Stride[0],
			(uint16_t)pstSrc->u32Width, (uint16_t)pstSrc->u32Height,
			(uint8_t*)pstFg->au64VirAddr[0], (uint16_t)pstFg->au32Stride[0],
			(uint8_t*)pstBg->au64VirAddr[0], (uint16_t)pstBg->au32Stride[0],
			pstModel->pData, pstCtrl->u8ModelNum,
			pstCtrl->u0q16LearnRate, pstCtrl->u0q16BgRatio, pstCtrl->u8q8VarThr,
			pstCtrl->u22q10NoiseVar, pstCtrl->u22q10MaxVar, pstCtrl->u22q10MinVar,
			pstCtrl->u0q16InitWeight);
}

static int32_t Common_CompareImage(COMMON_IMAGE_S *pstImg, const char* fileName)
{
	FILE *fp;
	int32_t open = fopen_s(&fp, fileName, "rb");
	if (open != CVI_SUCCESS) {
		printf("%s not exist\n", fileName);
		return false;
	}

	COMMON_IMAGE_S stRef;
	Common_CreateImage(&stRef, pstImg->enType, pstImg->u32Width, pstImg->u32Height);
	CVI_ReadFile(&stRef, fp);
	CVI_FCLOSE(fp);

	uint32_t strideBytesA = pstImg->au32Stride[0];
	uint32_t strideBytesB = stRef.au32Stride[0];
	bool match = true;

	for (uint32_t y = 0; y < pstImg->u32Height; y++) {
		uint8_t *pA = (uint8_t*)(pstImg->au64VirAddr[0] + y * strideBytesA);
		uint8_t *pB = (uint8_t*)(stRef.au64VirAddr[0] + y * strideBytesB);
		if (memcmp(pA, pB, pstImg->u32Width) != 0) {
			match = false;
			for (uint32_t x = 0; x < pstImg->u32Width; x++) {
				if (pA[x] != pB[x]) {
					printf("  first diff at y=%u x=%u A=0x%02x B=0x%02x\n", y, x, pA[x], pB[x]);
					break;
				}
			}
			break;
		}
	}
	Common_DestroyImage(&stRef);

	if (match) {
		printf("Compare against %s passed\n", fileName);
		return false;
	} else {
		printf("Compare against %s failed\n", fileName);
		return true;
	}
}

static void mat2CommonImg(Mat *src, COMMON_IMAGE_S *pstDst, int type)
{
	if(src->cols != (int)pstDst->u32Width || src->rows != (int)pstDst->u32Height) {
		printf("width or height didn't equal, line:%d\n",__LINE__);
	}

	uint8_t *pBase = (uint8_t*)pstDst->au64VirAddr[0];
	uint32_t strideBytes = pstDst->au32Stride[0];

	switch(type) {
	case CVGRAY_COMMON_U8C1:
		for(int r=0; r<src->rows; r++) {
			uint8_t *ptm = src->ptr(r);
			uint8_t *pti = pBase + r * strideBytes;
			memcpy(pti, ptm, pstDst->u32Width);
		}
		break;
	default:
		printf("not support type for mat2CommonImg!\n");
	}
}

static void commonImg2Mat(COMMON_IMAGE_S *pstSrc, Mat *dst, int type)
{
	if(dst->cols != (int)pstSrc->u32Width || dst->rows != (int)pstSrc->u32Height) {
		printf("width or height didn't equal,line:%d\n",__LINE__);
		return;
	}

	uint8_t *pBase = (uint8_t*)pstSrc->au64VirAddr[0];
	uint32_t strideBytes = pstSrc->au32Stride[0];

	switch(type) {
	case COMMON_U8C1_CVGRAY:
		for(int r=0; r<dst->rows; r++) {
			uint8_t *ptm = dst->ptr(r);
			uint8_t *pti = pBase + r * strideBytes;
			memcpy(ptm, pti, dst->cols);
		}
		break;
	default:
		printf("not support type for commonImg2Mat!\n");
	}
}

int32_t GMM_Sample_U8C1(int show, int compare)
{
	int32_t  s32Ret = CVI_SUCCESS;
	int32_t s32CompareError = 0;
	// const char *pchAvi = "./data/avi/campus.avi";
	// const char *pchRes = "./data/avi/GMM_Sample_U8C1.avi";
    const char *pchAvi = "./data/avi/小区监控-src_hd_爱给网_aigei_com.mp4";
    const char *pchRes = "./data/avi/GMM_Sample_U8C1.avi";

	COMMON_IMAGE_S stCommImg, stCommFg, stCommBg;
	COMMON_MEM_INFO_S stModel;
	COMMON_GMM_CTRL_S stCtrl;
	uint16_t u16Width, u16Height;
	int32_t s32FrmCnt = 0;

	Mat cvImg, cvFg, cvBg, cvImgGray, cvFgBGR, cvBgBGR, cvDispImg;
	VideoCapture cvCap;
	VideoWriter  cvWte;
	cv::VideoWriter videoWriter;
	bool isVideoInitialized = false;
	int cvGap;
	Size cvSz;
    int fourcc;

	cvCap.open(pchAvi);
	int cvWidth  = (int32_t)cvCap.get(cv::CAP_PROP_FRAME_WIDTH);
	int cvHeight = (int32_t)cvCap.get(cv::CAP_PROP_FRAME_HEIGHT);

	u16Width  = cvWidth & (~1);
	u16Height = cvHeight & (~1);
    printf("width = %d, height = %d\n", u16Width, u16Height);

	stCtrl.u0q16BgRatio		= 45875;
	stCtrl.u0q16InitWeight	= 3277;
	stCtrl.u22q10NoiseVar	= 225 * 1024;
	stCtrl.u22q10MaxVar		= 2000 * 1024;
	stCtrl.u22q10MinVar		= 200 * 1024;
	stCtrl.u8q8VarThr		= (uint16_t)(256 * 6.25);
	stCtrl.u8ModelNum		= 3;

	if (Common_CreateImage(&stCommImg, IVE_IMAGE_TYPE_U8C1, u16Width, u16Height) != CVI_SUCCESS)goto FAIL_U8C1_0;
	if (Common_CreateImage(&stCommBg, IVE_IMAGE_TYPE_U8C1, u16Width, u16Height) != CVI_SUCCESS)goto FAIL_U8C1_1;
	if (Common_CreateImage(&stCommFg, IVE_IMAGE_TYPE_U8C1, u16Width, u16Height) != CVI_SUCCESS)goto FAIL_U8C1_2;

	cvBg	= Mat::zeros(u16Height, u16Width, CV_8UC1);
	cvBgBGR = Mat::zeros(u16Height, u16Width, CV_8UC3);
	cvFg    = Mat::zeros(u16Height, u16Width, CV_8UC1);
	cvFgBGR = Mat::zeros(u16Height, u16Width, CV_8UC3);

	if (Common_CreateMem(&stModel, stCtrl.u8ModelNum * 7 * u16Width * u16Height) != CVI_SUCCESS)goto FAIL_U8C1_3;

	cvGap = 10;
	cvSz.width  = stCommImg.u32Width  + stCommBg.u32Width  + stCommFg.u32Width  + 4 * cvGap;
	cvSz.height = max(max(stCommImg.u32Height, stCommBg.u32Height), stCommFg.u32Height) + 2 * cvGap;
    fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
    cvWte.open(pchRes, fourcc, cvCap.get(cv::CAP_PROP_FPS), cvSz);

	for(;;) {
		s32FrmCnt = (int32_t)cvCap.get(cv::CAP_PROP_POS_FRAMES);
		cvCap >> cvImg;
		if(cvImg.empty()) break;

		if(cvImg.rows != (int)stCommImg.u32Height || cvImg.cols != (int)stCommImg.u32Width)
			resize(cvImg, cvImg, Size(stCommImg.u32Width, stCommImg.u32Height));

		cvtColor(cvImg, cvImgGray, cv::COLOR_BGR2GRAY);
		mat2CommonImg(&cvImgGray, &stCommImg, CVGRAY_COMMON_U8C1);

		stCtrl.u0q16LearnRate = (s32FrmCnt >= 500) ? 131 : (65535/(s32FrmCnt+1));

		s32Ret = CVI_Common_GMM(&stCommImg, &stCommFg, &stCommBg, &stModel, &stCtrl);
		if (s32Ret != CVI_SUCCESS) goto FAIL_U8C1_4;

		if (compare && (s32FrmCnt == 0 || s32FrmCnt == 1 || s32FrmCnt == 512)) {
			char fileName[_MAX_FNAME];

			snprintf(fileName, _MAX_FNAME, "./data/result/sample_GMM_U8C1_bg_%d.yuv", s32FrmCnt);
			if (Common_CompareImage(&stCommBg, fileName))
				s32CompareError++;

			snprintf(fileName, _MAX_FNAME, "./data/result/sample_GMM_U8C1_fg_%d.yuv", s32FrmCnt);
			if (Common_CompareImage(&stCommFg, fileName))
				s32CompareError++;
		}

		if (show) {
            commonImg2Mat(&stCommBg, &cvBg, COMMON_U8C1_CVGRAY);
            commonImg2Mat(&stCommFg, &cvFg, COMMON_U8C1_CVGRAY);
            cvtColor(cvBg, cvBgBGR, cv::COLOR_GRAY2BGR);
            cvtColor(cvFg, cvFgBGR, cv::COLOR_GRAY2BGR);

            hconcat(cvImg, cvBgBGR, cvDispImg);
            hconcat(cvDispImg, cvFgBGR, cvDispImg);

            putText(cvDispImg, "srcImg", Point(5,15), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255,0,0));
            putText(cvDispImg, "bgImg", Point(cvImg.cols + 5,15), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255,0,0));
            putText(cvDispImg, "fgImg", Point(cvImg.cols + cvBgBGR.cols + 5,15), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255,0,0));

            if (!isVideoInitialized && cvDispImg.rows > 0 && cvDispImg.cols > 0) {
                const char* videoPath = "./data/result/output_video_u8c1.mp4";
                int vfourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
                double fps = 25.0;
                cv::Size frameSize(cvDispImg.cols, cvDispImg.rows);
                videoWriter.open(videoPath, vfourcc, fps, frameSize);
                if (videoWriter.isOpened()) isVideoInitialized = true;
            }

            if (isVideoInitialized) videoWriter.write(cvDispImg);

            char filename[256];
            snprintf(filename, sizeof(filename), "./data/result/frame/frame_%06d.png", s32FrmCnt);
            createDirectory("./data/result/frame/");
            imwrite(filename, cvDispImg);
        }

        // Save first 35 decoded frames as raw file
        if (s32FrmCnt < 35) {
            FILE *rawFp = nullptr;
            char rawFilename[256];
            snprintf(rawFilename, sizeof(rawFilename), "./data/result/raw_frames.raw");
            createDirectory("./data/result/");
            if (s32FrmCnt == 0) {
                rawFp = fopen(rawFilename, "wb");
            } else {
                rawFp = fopen(rawFilename, "ab");
            }
            if (rawFp) {
                // Save raw BGR data (cvImg is the original decoded frame)
                size_t written = fwrite(cvImg.data, 1, cvImg.total() * cvImg.elemSize(), rawFp);
                if (written != cvImg.total() * cvImg.elemSize()) {
                    printf("Warning: failed to write all raw data for frame %d\n", s32FrmCnt);
                }
                fclose(rawFp);
            }
        }
	}

FAIL_U8C1_4:
    cvWte.release();
    if (isVideoInitialized) { videoWriter.release(); std::cout << "U8C1 Video file saved successfully." << std::endl; }
FAIL_U8C1_3:
    Common_DestroyMem(&stModel);
FAIL_U8C1_2:
    Common_DestroyImage(&stCommFg);
FAIL_U8C1_1:
    Common_DestroyImage(&stCommBg);
FAIL_U8C1_0:
    Common_DestroyImage(&stCommImg);
    destroyAllWindows();
    return (s32CompareError == 0) ? s32Ret : CVI_FAILURE;
}

/* ===== main.cpp Implementation ===== */

int main( int argc, char* const* argv )
{
	int32_t s32FinalResult = CVI_SUCCESS;
	int32_t s32Result;

	int show = 1, compare = 1;
	int c;
	opterr = 0;
	while ((c = getopt (argc, argv, "s:c:")) != -1)
	{
    	switch (c)
		{
		case 's':
			show = atoi(optarg);
			break;
		case 'c':
    	    compare = atoi(optarg);
    	    break;
		default:
			abort ();
		}
	}

	printf("%s show gmm sample\n",argv[0]);

    printf("Gray gmm sample start...\n");
    s32Result = GMM_Sample_U8C1(show, compare);
	printf("-->end!\n");
	if (s32Result != CVI_SUCCESS) {
		printf("GMM_Sample_U8C1 failed, ret = %d\n", s32Result);
		s32FinalResult = s32Result;
	}

	printf("gmm sample done\n");

	return s32FinalResult;
}
