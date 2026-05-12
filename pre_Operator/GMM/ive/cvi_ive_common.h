#ifndef __CVI_IVE_COMMON_H__
#define __CVI_IVE_COMMON_H__

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>

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

#endif /* __CVI_IVE_COMMON_H__ */
