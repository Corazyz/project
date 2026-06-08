#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __unix
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
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

/*
 * 3-channel GMM model layout per component (11 bytes):
 *   [0..1]  weight      uint16_t  Q0.16
 *   [2..3]  mu channel0 uint16_t  Q8.8
 *   [4..5]  mu channel1 uint16_t  Q8.8
 *   [6..7]  mu channel2 uint16_t  Q8.8
 *   [8..10] variance    uint24    Q22.10  (shared across channels)
 */
#define GMM_C3_MODEL_BYTES 11

int CVI_HW_SingleGMM_C3(
    uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint8_t *pstFg, uint16_t Fg_u16Stride, uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar,
    uint16_t u0q16InitWeight);

int32_t CVI_MPI_IVE_GMM_C3(
    uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstFg, uint16_t Fg_u16Stride,
    uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar,
    uint16_t u0q16InitWeight);

int32_t GMM_Sample_U8C3(int show, int compare);

/* ===== mpi_ive.cpp Implementation ===== */

#define MAX2(a, b)  (( (a) > (b) )? (a) : (b) )
#define MIN2(a, b)  (( (a) < (b) )? (a) : (b) )

int CVI_HW_SingleGMM_C3(
    uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint8_t *pstFg, uint16_t Fg_u16Stride, uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar,
    uint16_t u0q16InitWeight)
{
    int result = 0;
    int min_k_Km1;
    uint8_t swapbuf[GMM_C3_MODEL_BYTES];
    uint32_t One_div_wscale, sortKey_k1p1, sortKey_k1;
    int64_t diff0, diff1, diff2;
    int k1, k, kForeground, kHit;
    uint8_t pix0, pix1, pix2;
    uint32_t dw, wsum;
    uint64_t dist2, var;
    uint16_t x, y, w, mu0, mu1, mu2;
    uint8_t *pstBg0;
    uint8_t *mptr;
    uint8_t *pstFg0;
    uint8_t *pstSrc0;

    pstSrc0 = pstSrc;
    pstFg0  = pstFg;
    pstBg0  = pstBg;

    for (y = 0; y < Src_u16Height; ++y)
    {
        result = y;
        for (x = 0; x < Src_u16Width; ++x)
        {
            kHit        = -1;
            kForeground = -1;
            pix0 = pstSrc0[x * 3 + 0];
            pix1 = pstSrc0[x * 3 + 1];
            pix2 = pstSrc0[x * 3 + 2];
            wsum = 0;

            for (k = 0; k < u8ModelNum; ++k)
            {
                mptr = &pstModel[GMM_C3_MODEL_BYTES * k];
                w    = mptr[0] + (mptr[1] << 8);
                wsum += w;
                if (!w)
                    break;

                mu0 = mptr[2] + (mptr[3] << 8);
                mu1 = mptr[4] + (mptr[5] << 8);
                mu2 = mptr[6] + (mptr[7] << 8);
                var = mptr[8] + (mptr[9] << 8) + ((uint32_t)mptr[10] << 16);

                diff0 = ((int64_t)pix0 << 8) - mu0;
                diff1 = ((int64_t)pix1 << 8) - mu1;
                diff2 = ((int64_t)pix2 << 8) - mu2;

                /* Average squared Mahalanobis distance across 3 channels.
                 * Dividing by 3 keeps the same scale as the C1 single-channel
                 * dist2, so all threshold parameters remain compatible. */
                dist2 = ((diff0 * diff0 + diff1 * diff1 + diff2 * diff2) / 3 + 32) >> 6;

                if (dist2 < ((uint64_t)u8q8VarThr * var + 128) >> 8)
                {
                    wsum -= w;
                    dw = ((0xFFFF - w) * (uint32_t)u0q16LearnRate + 0x8000) >> 16;
                    w += dw;
                    *(uint16_t *)mptr = w;

                    mu0 += (uint16_t)((u0q16LearnRate * diff0) >> 16);
                    mu1 += (uint16_t)((u0q16LearnRate * diff1) >> 16);
                    mu2 += (uint16_t)((u0q16LearnRate * diff2) >> 16);
                    *((uint16_t *)mptr + 1) = mu0;
                    *((uint16_t *)mptr + 2) = mu1;
                    *((uint16_t *)mptr + 3) = mu2;

                    var = ((var << 16) + (u0q16LearnRate * (dist2 - var))) >> 16;
                    var = MIN2(var, u22q10MaxVar);
                    var = MAX2(var, u22q10MinVar);
                    mptr[8]  = (uint8_t)(var & 0xFF);
                    mptr[9]  = (uint8_t)((var >> 8) & 0xFF);
                    mptr[10] = (uint8_t)((var >> 16) & 0xFF);

                    for (k1 = k - 1; k1 >= 0; --k1)
                    {
                        sortKey_k1   = pstModel[GMM_C3_MODEL_BYTES * k1]
                                     + (pstModel[GMM_C3_MODEL_BYTES * k1 + 1] << 8);
                        sortKey_k1p1 = pstModel[GMM_C3_MODEL_BYTES * (k1 + 1)]
                                     + (pstModel[GMM_C3_MODEL_BYTES * (k1 + 1) + 1] << 8);
                        if (sortKey_k1 > sortKey_k1p1)
                            break;
                        memcpy(swapbuf,
                               &pstModel[GMM_C3_MODEL_BYTES * k1],
                               GMM_C3_MODEL_BYTES);
                        memcpy(&pstModel[GMM_C3_MODEL_BYTES * k1],
                               &pstModel[GMM_C3_MODEL_BYTES * (k1 + 1)],
                               GMM_C3_MODEL_BYTES);
                        memcpy(&pstModel[GMM_C3_MODEL_BYTES * (k1 + 1)],
                               swapbuf,
                               GMM_C3_MODEL_BYTES);
                    }
                    kHit = k1 + 1;
                    break;
                }
            }

            if (kHit >= 0)
            {
                while (k < u8ModelNum)
                {
                    w = pstModel[GMM_C3_MODEL_BYTES * k]
                      + (pstModel[GMM_C3_MODEL_BYTES * k + 1] << 8);
                    wsum += w;
                    ++k;
                }
            }
            else
            {
                if (k <= u8ModelNum - 1)
                    min_k_Km1 = k;
                else
                    min_k_Km1 = u8ModelNum - 1;
                k    = min_k_Km1;
                kHit = min_k_Km1;

                w = pstModel[GMM_C3_MODEL_BYTES * min_k_Km1]
                  + (pstModel[GMM_C3_MODEL_BYTES * min_k_Km1 + 1] << 8);
                wsum += u0q16InitWeight - w;

                pstModel[GMM_C3_MODEL_BYTES * k + 0] = (uint8_t)(u0q16InitWeight & 0xFF);
                pstModel[GMM_C3_MODEL_BYTES * k + 1] = (uint8_t)(u0q16InitWeight >> 8);
                /* mu stored as Q8.8: integer part in high byte, fraction = 0 */
                pstModel[GMM_C3_MODEL_BYTES * k + 2] = 0;
                pstModel[GMM_C3_MODEL_BYTES * k + 3] = pix0;
                pstModel[GMM_C3_MODEL_BYTES * k + 4] = 0;
                pstModel[GMM_C3_MODEL_BYTES * k + 5] = pix1;
                pstModel[GMM_C3_MODEL_BYTES * k + 6] = 0;
                pstModel[GMM_C3_MODEL_BYTES * k + 7] = pix2;
                var = 4 * u22q10NoiseVar;
                pstModel[GMM_C3_MODEL_BYTES * k + 8]  = (uint8_t)(var & 0xFF);
                pstModel[GMM_C3_MODEL_BYTES * k + 9]  = (uint8_t)((var >> 8) & 0xFF);
                pstModel[GMM_C3_MODEL_BYTES * k + 10] = (uint8_t)((var >> 16) & 0xFF);
            }

            One_div_wscale = wsum;
            wsum = 0;
            for (k = 0; k < u8ModelNum; ++k)
            {
                w = pstModel[GMM_C3_MODEL_BYTES * k]
                  + (pstModel[GMM_C3_MODEL_BYTES * k + 1] << 8);
                w = (uint16_t)(0xFFFF * (uint32_t)w / One_div_wscale);
                *(uint16_t *)&pstModel[GMM_C3_MODEL_BYTES * k] = w;
                wsum += w;
                if (wsum > u0q16BgRatio && kForeground < 0)
                    kForeground = k + 1;
            }

            pstFg0[x] = (kHit >= kForeground) ? 0xFF : 0;

            /* Background: high byte of each channel's mean from the dominant component */
            pstBg0[x * 3 + 0] = pstModel[3];
            pstBg0[x * 3 + 1] = pstModel[5];
            pstBg0[x * 3 + 2] = pstModel[7];

            pstModel += GMM_C3_MODEL_BYTES * u8ModelNum;
        }
        pstSrc0 += Src_u16Stride;
        pstFg0  += Fg_u16Stride;
        pstBg0  += Bg_u16Stride;
    }
    return result;
}

int32_t CVI_MPI_IVE_GMM_C3(
    uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstFg, uint16_t Fg_u16Stride,
    uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar,
    uint16_t u0q16InitWeight)
{
    CVI_HW_SingleGMM_C3(
        pstSrc, Src_u16Stride,
        Src_u16Width, Src_u16Height,
        pstModel, u8ModelNum,
        pstFg, Fg_u16Stride,
        pstBg, Bg_u16Stride,
        u0q16LearnRate, u0q16BgRatio, u8q8VarThr,
        u22q10NoiseVar, u22q10MaxVar, u22q10MinVar,
        u0q16InitWeight);
    return 0;
}

/* ===== GMM.cpp Implementation (No OpenCV) ===== */

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

static int32_t Common_CreateImage(COMMON_IMAGE_S* pstImg, IVE_IMAGE_TYPE_E enType,
                                  uint32_t u32Width, uint32_t u32Height)
{
    uint32_t u32Stride = (enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) ? u32Width * 3 : u32Width;
    uint32_t u32Bytes  = u32Stride * u32Height;

    pstImg->au64VirAddr[0] = (uint64_t)malloc(u32Bytes);
    if (!pstImg->au64VirAddr[0]) return CVI_FAILURE;
    memset((void*)pstImg->au64VirAddr[0], 0, u32Bytes);
    pstImg->au64PhyAddr[0] = 0;
    pstImg->au32Stride[0]  = u32Stride;
    pstImg->u32Width       = u32Width;
    pstImg->u32Height      = u32Height;
    pstImg->enType         = enType;
    pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[2] = 0;
    pstImg->au64VirAddr[1] = pstImg->au64VirAddr[2] = 0;
    pstImg->au32Stride[1]  = pstImg->au32Stride[2]  = 0;
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

static int32_t CVI_Common_GMM_C3(COMMON_IMAGE_S *pstSrc, COMMON_IMAGE_S *pstFg,
    COMMON_IMAGE_S *pstBg, COMMON_MEM_INFO_S *pstModel, COMMON_GMM_CTRL_S *pstCtrl)
{
    return CVI_MPI_IVE_GMM_C3(
        (uint8_t*)pstSrc->au64VirAddr[0], (uint16_t)pstSrc->au32Stride[0],
        (uint16_t)pstSrc->u32Width, (uint16_t)pstSrc->u32Height,
        (uint8_t*)pstFg->au64VirAddr[0],  (uint16_t)pstFg->au32Stride[0],
        (uint8_t*)pstBg->au64VirAddr[0],  (uint16_t)pstBg->au32Stride[0],
        pstModel->pData, pstCtrl->u8ModelNum,
        pstCtrl->u0q16LearnRate, pstCtrl->u0q16BgRatio, pstCtrl->u8q8VarThr,
        pstCtrl->u22q10NoiseVar, pstCtrl->u22q10MaxVar, pstCtrl->u22q10MinVar,
        pstCtrl->u0q16InitWeight);
}

// Simple PNG writer (no compression, uncompressed PNG)
#include <zlib.h>

static uint32_t crc32_table[256];
static int crc_table_initialized = 0;

static void init_crc_table() {
    if (crc_table_initialized) return;
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) {
            if (c & 1) c = 0xEDB88320L ^ (c >> 1);
            else c = c >> 1;
        }
        crc32_table[n] = c;
    }
    crc_table_initialized = 1;
}

static uint32_t calc_crc32(const uint8_t *buf, size_t len) {
    init_crc_table();
    uint32_t c = 0xFFFFFFFF;
    for (size_t n = 0; n < len; n++) {
        c = crc32_table[(c ^ buf[n]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFF;
}

static void write_png_chunk(FILE *fp, const char *type, const uint8_t *data, size_t len) {
    uint32_t len_be = __builtin_bswap32((uint32_t)len);
    fwrite(&len_be, 4, 1, fp);
    uint8_t chunk_type[4] = {type[0], type[1], type[2], type[3]};
    fwrite(chunk_type, 1, 4, fp);
    if (len > 0) fwrite(data, 1, len, fp);
    uint8_t type_data[4 + len];
    memcpy(type_data, chunk_type, 4);
    if (len > 0) memcpy(type_data + 4, data, len);
    uint32_t crc = calc_crc32(type_data, 4 + len);
    uint32_t crc_be = __builtin_bswap32(crc);
    fwrite(&crc_be, 4, 1, fp);
}

static void writePNG(const char* filename, uint8_t* data, int width, int height) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) return;

    uint8_t signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    fwrite(signature, 1, 8, fp);

    uint8_t ihdr[13];
    uint32_t w_be = __builtin_bswap32(width);
    uint32_t h_be = __builtin_bswap32(height);
    memcpy(ihdr, &w_be, 4);
    memcpy(ihdr + 4, &h_be, 4);
    ihdr[8]  = 8;  // bit depth
    ihdr[9]  = 2;  // color type: RGB
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;
    write_png_chunk(fp, "IHDR", ihdr, 13);

    int rowSize = width * 3;
    size_t rawSize = (size_t)(rowSize + 1) * height;
    uint8_t *rawData = (uint8_t*)malloc(rawSize);
    if (!rawData) { fclose(fp); return; }

    for (int y = 0; y < height; y++) {
        rawData[y * (rowSize + 1)] = 0;
        memcpy(rawData + y * (rowSize + 1) + 1, data + y * rowSize, rowSize);
    }

    uLongf compressedSize = compressBound(rawSize);
    uint8_t *compressed = (uint8_t*)malloc(compressedSize);
    if (!compressed) { free(rawData); fclose(fp); return; }

    compress(compressed, &compressedSize, rawData, rawSize);
    write_png_chunk(fp, "IDAT", compressed, compressedSize);
    free(rawData);
    free(compressed);

    write_png_chunk(fp, "IEND", NULL, 0);
    fclose(fp);
}

// Convert grayscale to RGB
static void grayToRgb(uint8_t* gray, uint8_t* rgb, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        rgb[i * 3 + 0] = gray[i];
        rgb[i * 3 + 1] = gray[i];
        rgb[i * 3 + 2] = gray[i];
    }
}

// Convert RGB to BGR
static void rgbToBgr(uint8_t* rgb, uint8_t* bgr, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        bgr[i * 3 + 0] = rgb[i * 3 + 2];
        bgr[i * 3 + 1] = rgb[i * 3 + 1];
        bgr[i * 3 + 2] = rgb[i * 3 + 0];
    }
}

// Horizontal concatenate three RGB images into one
static void hconcat3(uint8_t* src1, uint8_t* src2, uint8_t* src3,
                     uint8_t* dst, int width, int height) {
    int dstWidth = width * 3;
    for (int y = 0; y < height; y++) {
        uint8_t* dstRow = dst + y * dstWidth * 3;
        memcpy(dstRow,              src1 + y * width * 3, width * 3);
        memcpy(dstRow + width * 3,  src2 + y * width * 3, width * 3);
        memcpy(dstRow + width * 6,  src3 + y * width * 3, width * 3);
    }
}

int32_t GMM_Sample_U8C3(int show, int compare)
{
	int32_t  s32Ret = CVI_SUCCESS;
	int32_t s32CompareError = 0;
	const char *pchRaw    = "/home/zyz/projects/test_project/pre_Operator/GMM/ive_C1/data/result/raw_frames.raw";
	const char *pchOutDir = "/home/zyz/projects/test_project/pre_Operator/GMM/ive_c3/data/result/frame";

	COMMON_IMAGE_S stCommImg, stCommFg, stCommBg;
	COMMON_MEM_INFO_S stModel;
	COMMON_GMM_CTRL_S stCtrl;
	uint16_t u16Width = 352, u16Height = 288;
	int32_t s32FrmCnt = 0;
	const int32_t totalFrames = 35;

	FILE* fpRaw = fopen(pchRaw, "rb");
	if (!fpRaw) {
		printf("Failed to open raw file: %s\n", pchRaw);
		return CVI_FAILURE;
	}

	stCtrl.u0q16BgRatio    = 45875;
	stCtrl.u0q16InitWeight = 3277;
	stCtrl.u22q10NoiseVar  = 225 * 1024;
	stCtrl.u22q10MaxVar    = 2000 * 1024;
	stCtrl.u22q10MinVar    = 200 * 1024;
	stCtrl.u8q8VarThr      = (uint16_t)(256 * 6.25);
	stCtrl.u8ModelNum      = 3;

	/* src: 3-channel packed RGB; fg: 1-channel mask; bg: 3-channel packed RGB */
	if (Common_CreateImage(&stCommImg, IVE_IMAGE_TYPE_U8C3_PACKAGE, u16Width, u16Height) != CVI_SUCCESS) {
		fclose(fpRaw);
		return CVI_FAILURE;
	}
	if (Common_CreateImage(&stCommBg, IVE_IMAGE_TYPE_U8C3_PACKAGE, u16Width, u16Height) != CVI_SUCCESS) {
		Common_DestroyImage(&stCommImg);
		fclose(fpRaw);
		return CVI_FAILURE;
	}
	if (Common_CreateImage(&stCommFg, IVE_IMAGE_TYPE_U8C1, u16Width, u16Height) != CVI_SUCCESS) {
		Common_DestroyImage(&stCommBg);
		Common_DestroyImage(&stCommImg);
		fclose(fpRaw);
		return CVI_FAILURE;
	}

	if (Common_CreateMem(&stModel,
	        (uint32_t)stCtrl.u8ModelNum * GMM_C3_MODEL_BYTES * u16Width * u16Height) != CVI_SUCCESS) {
		Common_DestroyImage(&stCommFg);
		Common_DestroyImage(&stCommBg);
		Common_DestroyImage(&stCommImg);
		fclose(fpRaw);
		return CVI_FAILURE;
	}

	createDirectory(pchOutDir);

	int frameSize = u16Width * u16Height * 3;
	uint8_t* rgbBuf  = (uint8_t*)malloc(frameSize);
	uint8_t* fgRgb   = (uint8_t*)malloc(frameSize);
	uint8_t* bgrBuf  = (uint8_t*)malloc(frameSize);
	uint8_t* dispRgb = (uint8_t*)malloc(frameSize * 3);

	for (s32FrmCnt = 0; s32FrmCnt < totalFrames; s32FrmCnt++)
	{
		size_t readBytes = fread(rgbBuf, 1, frameSize, fpRaw);
		if (readBytes != (size_t)frameSize) {
			printf("Warning: failed to read frame %d, got %zu bytes\n", s32FrmCnt, readBytes);
			break;
		}

		/* Copy RGB directly into the 3-channel src image */
		memcpy((void*)stCommImg.au64VirAddr[0], rgbBuf, frameSize);

		stCtrl.u0q16LearnRate = (s32FrmCnt >= 500) ? 131 : (65535 / (s32FrmCnt + 1));

		s32Ret = CVI_Common_GMM_C3(&stCommImg, &stCommFg, &stCommBg, &stModel, &stCtrl);
		if (s32Ret != CVI_SUCCESS) break;

		/* fg mask is grayscale -> expand to RGB for display */
		grayToRgb((uint8_t*)stCommFg.au64VirAddr[0], fgRgb, u16Width, u16Height);

		/* Convert src RGB to BGR for the display panel */
		rgbToBgr(rgbBuf, bgrBuf, u16Width, u16Height);

		/* Concatenate: src(BGR) | bg(RGB from model) | fg(gray->RGB) */
		hconcat3(bgrBuf, (uint8_t*)stCommBg.au64VirAddr[0], fgRgb,
		         dispRgb, u16Width, u16Height);

		char filename[512];
		snprintf(filename, sizeof(filename), "%s/frame_%06d.png", pchOutDir, s32FrmCnt);
		writePNG(filename, dispRgb, u16Width * 3, u16Height);
		printf("Saved frame %d to %s\n", s32FrmCnt, filename);
	}

	if (dispRgb) free(dispRgb);
	if (bgrBuf)  free(bgrBuf);
	if (fgRgb)   free(fgRgb);
	if (rgbBuf)  free(rgbBuf);
	Common_DestroyMem(&stModel);
	Common_DestroyImage(&stCommFg);
	Common_DestroyImage(&stCommBg);
	Common_DestroyImage(&stCommImg);
	if (fpRaw) fclose(fpRaw);
	return (s32CompareError == 0) ? s32Ret : CVI_FAILURE;
}

/* ===== main.cpp Implementation ===== */

int main(int argc, char* const* argv)
{
	int32_t s32FinalResult = CVI_SUCCESS;
	int32_t s32Result;

	int show = 1, compare = 1;
	int c;
	opterr = 0;
	while ((c = getopt(argc, argv, "s:c:")) != -1)
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
			abort();
		}
	}

	printf("%s show gmm sample\n", argv[0]);

	printf("RGB 3-channel GMM sample start...\n");
	s32Result = GMM_Sample_U8C3(show, compare);
	printf("-->end!\n");
	if (s32Result != CVI_SUCCESS) {
		printf("GMM_Sample_U8C3 failed, ret = %d\n", s32Result);
		s32FinalResult = s32Result;
	}

	printf("gmm sample done\n");
	return s32FinalResult;
}
