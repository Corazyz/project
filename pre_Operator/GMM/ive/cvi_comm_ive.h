#ifndef _CVI_COMM_IVE_H_
#define _CVI_COMM_IVE_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "cvi_type.h"
#include "cvi_errno.h"

typedef unsigned char           CVI_U0Q8;
typedef unsigned char           CVI_U1Q7;
typedef unsigned char           CVI_U5Q3;
typedef unsigned short          CVI_U0Q16;
typedef unsigned short          CVI_U4Q12;
typedef unsigned short          CVI_U6Q10;
typedef unsigned short          CVI_U8Q8;
typedef unsigned short          CVI_U9Q7;
typedef unsigned short          CVI_U12Q4;
typedef unsigned short          CVI_U14Q2;
typedef unsigned short			CVI_U5Q11;
typedef short                   CVI_S9Q7;
typedef short                   CVI_S14Q2;
typedef short                   CVI_S1Q15;
typedef unsigned int            CVI_U22Q10;
typedef unsigned int            CVI_U25Q7;
typedef unsigned int			CVI_U21Q11;
typedef int                     CVI_S25Q7;
typedef int                     CVI_S16Q16;
typedef unsigned short          CVI_U8Q4F4;
typedef float                   CVI_FLOAT;
typedef double                  CVI_DOUBLE;

typedef enum _IVE_IMAGE_TYPE_E
{
	IVE_IMAGE_TYPE_U8C1           =  0x0,
	IVE_IMAGE_TYPE_S8C1           =  0x1,

	IVE_IMAGE_TYPE_YUV420SP       =  0x2,		/*YUV420 SemiPlanar*/
	IVE_IMAGE_TYPE_YUV422SP       =  0x3,		/*YUV422 SemiPlanar*/
	IVE_IMAGE_TYPE_YUV420P        =  0x4,		/*YUV420 Planar */
	IVE_IMAGE_TYPE_YUV422P		  =	 0x5,       /*YUV422 planar */

	IVE_IMAGE_TYPE_S8C2_PACKAGE   =  0x6,
	IVE_IMAGE_TYPE_S8C2_PLANAR    =  0x7,

	IVE_IMAGE_TYPE_S16C1          =  0x8,
	IVE_IMAGE_TYPE_U16C1          =  0x9,

	IVE_IMAGE_TYPE_U8C3_PACKAGE   =  0xa,
	IVE_IMAGE_TYPE_U8C3_PLANAR    =  0xb,

	IVE_IMAGE_TYPE_S32C1          =  0xc,
	IVE_IMAGE_TYPE_U32C1          =  0xd,

	IVE_IMAGE_TYPE_S64C1          =  0xe,
	IVE_IMAGE_TYPE_U64C1          =  0xf,

    IVE_IMAGE_TYPE_YUV444P        =  0x10,		/*YUV444 Planar */

	IVE_IMAGE_TYPE_BUTT

}IVE_IMAGE_TYPE_E;

typedef struct _IVE_IMAGE_S
{
	CVI_U64  au64PhyAddr[3];
	CVI_U64  au64VirAddr[3];
	CVI_U32  au32Stride[3];
	CVI_U32  u32Width;
	CVI_U32  u32Height;
	IVE_IMAGE_TYPE_E  enType;
}IVE_IMAGE_S;
typedef IVE_IMAGE_S IVE_SRC_IMAGE_S;
typedef IVE_IMAGE_S IVE_DST_IMAGE_S;

typedef struct _IVE_MEM_INFO_S
{
	CVI_U64  u64PhyAddr;
	CVI_U64  u64VirAddr;
	CVI_U32  u32Size;
}IVE_MEM_INFO_S;
typedef IVE_MEM_INFO_S IVE_SRC_MEM_INFO_S;
typedef IVE_MEM_INFO_S IVE_DST_MEM_INFO_S;

typedef struct _IVE_DATA_S
{
	CVI_U64  u64PhyAddr;  /*Physical address of the data*/
	CVI_U64  u64VirAddr;

	CVI_U32  u32Stride;   /*2D data stride by byte*/
	CVI_U32  u32Width;    /*2D data width by byte*/
	CVI_U32  u32Height;   /*2D data height*/

    CVI_U32 u32Reserved;
}IVE_DATA_S;
typedef IVE_DATA_S IVE_SRC_DATA_S;
typedef IVE_DATA_S IVE_DST_DATA_S;

typedef union _IVE_8BIT_U
{
	CVI_S8 s8Val;
	CVI_U8 u8Val;
}IVE_8BIT_U;

typedef struct _IVE_POINT_U16_S
{
    CVI_U16 u16X;
    CVI_U16 u16Y;
}IVE_POINT_U16_S;

typedef struct _IVE_POINT_S16_S
{
	CVI_U16 s16X;
	CVI_U16 s16Y;
}IVE_POINT_S16_S;

typedef struct _IVE_POINT_S25Q7_S
{
    CVI_S25Q7     s25q7X;                /*X coordinate*/
    CVI_S25Q7     s25q7Y;                /*Y coordinate*/
}IVE_POINT_S25Q7_S;

typedef struct _IVE_RECT_U16_S
{
    CVI_U16 u16X;
    CVI_U16 u16Y;
    CVI_U16 u16Width;
    CVI_U16 u16Height;
}IVE_RECT_U16_S;

typedef struct _IVE_STIT_MASK_U16_S
{
    CVI_U16 u16PX;
    CVI_U16 u16PY;
	CVI_U16 u16XL;
	CVI_U16 u16XR;
    CVI_U16 u16Width;
    CVI_U16 u16Height;
	CVI_U16 u16BlendInfo[20];
}IVE_STIT_MASK_U16_S;

typedef struct _IVE_LOOK_UP_TABLE_S
{
    IVE_MEM_INFO_S stTable;
    CVI_U16 u16ElemNum;          /*LUT's elements number*/

    CVI_U8 u8TabInPreci;
    CVI_U8 u8TabOutNorm;

    CVI_S32 s32TabInLower;       /*LUT's original input lower limit*/
    CVI_S32 s32TabInUpper;       /*LUT's original input upper limit*/
}IVE_LOOK_UP_TABLE_S;


typedef enum _EN_IVE_ERR_CODE_E
{
    ERR_IVE_SYS_TIMEOUT    = 0x40,   /* IVE process timeout */
    ERR_IVE_QUERY_TIMEOUT  = 0x41,   /* IVE query timeout */
    ERR_IVE_OPEN_FILE      = 0x42,   /* IVE open file error */
    ERR_IVE_READ_FILE      = 0x43,   /* IVE read file error */
    ERR_IVE_WRITE_FILE     = 0x44,   /* IVE write file error */

    ERR_IVE_BUTT
}EN_IVE_ERR_CODE_E;

typedef enum _EN_FD_ERR_CODE_E
{
	ERR_FD_SYS_TIMEOUT   = 0x40,   /* FD process timeout */
	ERR_FD_CFG           = 0x41,   /* FD configuration error */
	ERR_FD_FACE_NUM_OVER = 0x42,   /* FD candidate face number over*/
	ERR_FD_OPEN_FILE     = 0x43,   /* FD open file error */
	ERR_FD_READ_FILE     = 0x44,   /* FD read file error */
	ERR_FD_WRITE_FILE    = 0x45,   /* FD write file error */

	ERR_FD_BUTT
}EN_FD_ERR_CODE_E;

#define CVI_ERR_IVE_INVALID_DEVID     CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
#define CVI_ERR_IVE_INVALID_CHNID     CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define CVI_ERR_IVE_ILLEGAL_PARAM     CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define CVI_ERR_IVE_EXIST             CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
#define CVI_ERR_IVE_UNEXIST           CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
#define CVI_ERR_IVE_NULL_PTR          CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define CVI_ERR_IVE_NOT_CONFIG        CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
#define CVI_ERR_IVE_NOT_SURPPORT      CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
#define CVI_ERR_IVE_NOT_PERM          CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define CVI_ERR_IVE_NOMEM             CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
#define CVI_ERR_IVE_NOBUF             CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
#define CVI_ERR_IVE_BUF_EMPTY         CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
#define CVI_ERR_IVE_BUF_FULL          CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
#define CVI_ERR_IVE_NOTREADY          CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define CVI_ERR_IVE_BADADDR           CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
#define CVI_ERR_IVE_BUSY              CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define CVI_ERR_IVE_SYS_TIMEOUT       CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_SYS_TIMEOUT)
#define CVI_ERR_IVE_QUERY_TIMEOUT     CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_QUERY_TIMEOUT)
#define CVI_ERR_IVE_OPEN_FILE         CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_OPEN_FILE)
#define CVI_ERR_IVE_READ_FILE         CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_READ_FILE)
#define CVI_ERR_IVE_WRITE_FILE        CVI_DEF_ERR(CVI_ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_WRITE_FILE)

#define CVI_ERR_FD_INVALID_DEVID     CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
#define CVI_ERR_FD_INVALID_CHNID     CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define CVI_ERR_FD_ILLEGAL_PARAM     CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define CVI_ERR_FD_EXIST             CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
#define CVI_ERR_FD_UNEXIST           CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
#define CVI_ERR_FD_NULL_PTR          CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define CVI_ERR_FD_NOT_CONFIG        CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
#define CVI_ERR_FD_NOT_SURPPORT      CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
#define CVI_ERR_FD_NOT_PERM          CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define CVI_ERR_FD_NOMEM             CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
#define CVI_ERR_FD_NOBUF             CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
#define CVI_ERR_FD_BUF_EMPTY         CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
#define CVI_ERR_FD_BUF_FULL          CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
#define CVI_ERR_FD_NOTREADY          CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define CVI_ERR_FD_BADADDR           CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
#define CVI_ERR_FD_BUSY              CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define CVI_ERR_FD_SYS_TIMEOUT       CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_SYS_TIMEOUT)
#define CVI_ERR_FD_CFG               CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_CFG)
#define CVI_ERR_FD_FACE_NUM_OVER     CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_FACE_NUM_OVER)
#define CVI_ERR_FD_OPEN_FILE         CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_OPEN_FILE)
#define CVI_ERR_FD_READ_FILE         CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_READ_FILE)
#define CVI_ERR_FD_WRITE_FILE        CVI_DEF_ERR(CVI_ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_WRITE_FILE)

#define CVI_ERR_ODT_INVALID_CHNID    CVI_DEF_ERR(CVI_ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define CVI_ERR_ODT_EXIST			 CVI_DEF_ERR(CVI_ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
#define CVI_ERR_ODT_UNEXIST			 CVI_DEF_ERR(CVI_ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
#define CVI_ERR_ODT_NOT_PERM         CVI_DEF_ERR(CVI_ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define CVI_ERR_ODT_NOTREADY		 CVI_DEF_ERR(CVI_ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define CVI_ERR_ODT_BUSY			 CVI_DEF_ERR(CVI_ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif/*__CVI_COMM_IVE_H__*/
