#ifndef _CVI_IVE_H_
#define _CVI_IVE_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "cvi_comm_ive.h"

#define IVE_HIST_NUM		     256
#define IVE_MAP_NUM				 256
#define IVE_MAX_REGION_NUM       254
#define IVE_ST_MAX_CORNER_NUM    500

typedef enum _IVE_DMA_MODE_E
{
    IVE_DMA_MODE_DIRECT_COPY    =  0x0,
    IVE_DMA_MODE_INTERVAL_COPY  =  0x1,
    IVE_DMA_MODE_SET_3BYTE      =  0x2,
    IVE_DMA_MODE_SET_8BYTE      =  0x3,
    IVE_DMA_MODE_BUTT
}IVE_DMA_MODE_E;

typedef struct _IVE_DMA_CTRL_S
{
    IVE_DMA_MODE_E enMode;
    CVI_U64 u64Val;		    /*Used in memset mode*/
    CVI_U8 u8HorSegSize;		/*Used in interval-copy mode, every row was segmented by u8HorSegSize bytes, restricted in values of 2,3,4,8,16*/
    CVI_U8 u8ElemSize; 		/*Used in interval-copy mode, the valid bytes copied in front of every segment in a valid row, w_ch 0<u8ElemSize<u8HorSegSize*/
    CVI_U8 u8VerSegRows;		/*Used in interval-copy mode, copy one row in every u8VerSegRows*/
}IVE_DMA_CTRL_S;

typedef struct _IVE_FILTER_CTRL_S
{
    CVI_S8 as8Mask[25];        /*Template parameter filter coefficient*/
    CVI_U8 u8Norm;             /*Normalization parameter, by right s_ft*/
}IVE_FILTER_CTRL_S;

typedef enum _IVE_CSC_MODE_E
{
    IVE_CSC_MODE_VIDEO_BT601_YUV2RGB =  0x0,	/*CSC: YUV2RGB, video transfer mode, RGB value range [16, 235]*/
    IVE_CSC_MODE_VIDEO_BT709_YUV2RGB =  0x1,	/*CSC: YUV2RGB, video transfer mode, RGB value range [16, 235]*/
    IVE_CSC_MODE_PIC_BT601_YUV2RGB   =  0x2,	/*CSC: YUV2RGB, picture transfer mode, RGB value range [0, 255]*/
    IVE_CSC_MODE_PIC_BT709_YUV2RGB   =  0x3,	/*CSC: YUV2RGB, picture transfer mode, RGB value range [0, 255]*/

	IVE_CSC_MODE_PIC_BT601_YUV2HSV   =  0x4,	/*CSC: YUV2HSV, picture transfer mode, HSV value range [0, 255]*/
	IVE_CSC_MODE_PIC_BT709_YUV2HSV   =  0x5,	/*CSC: YUV2HSV, picture transfer mode, HSV value range [0, 255]*/

	IVE_CSC_MODE_PIC_BT601_YUV2LAB   =  0x6,	/*CSC: YUV2LAB, picture transfer mode, Lab value range [0, 255]*/
	IVE_CSC_MODE_PIC_BT709_YUV2LAB   =  0x7,	/*CSC: YUV2LAB, picture transfer mode, Lab value range [0, 255]*/

	IVE_CSC_MODE_VIDEO_BT601_RGB2YUV =  0x8,   	/*CSC: RGB2YUV, video transfer mode, YUV value range [0, 255]*/
	IVE_CSC_MODE_VIDEO_BT709_RGB2YUV =  0x9,	/*CSC: RGB2YUV, video transfer mode, YUV value range [0, 255]*/
	IVE_CSC_MODE_PIC_BT601_RGB2YUV   =  0xa,   	/*CSC: RGB2YUV, picture transfer mode, Y:[16, 235],U\V:[16, 240]*/
	IVE_CSC_MODE_PIC_BT709_RGB2YUV   =  0xb,	/*CSC: RGB2YUV, picture transfer mode, Y:[16, 235],U\V:[16, 240]*/

    IVE_CSC_MODE_BUTT
}IVE_CSC_MODE_E;

typedef enum _IVE_CSC_SRC_DST_FMT_E {
  IVE_CSC_SRC_DST_FMT_U8C3_PLANAR = 0x0,   /*U8C3 planar RGB*/
  IVE_CSC_SRC_DST_FMT_U8C3_PACKAGE = 0x1,  /*U8C3 planar RGB*/
  IVE_CSC_SRC_DST_FMT_YUV420SP = 0x2,      /*YUV420 semi-planar*/
  IVE_CSC_SRC_DST_FMT_YUV422SP = 0x3,      /*YUV420 semi-planar*/
  IVE_CSC_SRC_DST_FMT_YUV444_PLANAR = 0x4, /*YUV444 planar*/
} IVE_CSC_SRC_DST_FMT_E;

typedef struct _IVE_CSC_CTRL_S
{
    IVE_CSC_MODE_E    enMode; /*Working mode*/
}IVE_CSC_CTRL_S;

typedef struct _IVE_FILTER_AND_CSC_CTRL_S
{
    IVE_CSC_MODE_E		enMode;			/*CSC working mode*/
    CVI_S8				as8Mask[25];	/*Template parameter filter coefficient*/
    CVI_U8				u8Norm;			/*Normalization parameter, by right s_ft*/
}IVE_FILTER_AND_CSC_CTRL_S;

typedef enum _IVE_SOBEL_OUT_CTRL_E
{
	IVE_SOBEL_OUT_CTRL_BOTH =  0x0, /*Output horizontal and vertical*/
	IVE_SOBEL_OUT_CTRL_HOR  =  0x1,	/*Output horizontal*/
	IVE_SOBEL_OUT_CTRL_VER  =  0x2,	/*Output vertical*/
	IVE_SOBEL_OUT_CTRL_BUTT
}IVE_SOBEL_OUT_CTRL_E;

typedef struct _IVE_SOBEL_CTRL_S
{
	IVE_SOBEL_OUT_CTRL_E enOutCtrl; /*Output format*/
    CVI_S8 as8Mask[25];			    /*Template parameter*/
}IVE_SOBEL_CTRL_S;

typedef enum _IVE_MAG_AND_ANG_OUT_CTRL_E
{
    IVE_MAG_AND_ANG_OUT_CTRL_MAG          =  0x0,      /*Only the magnitude is output.*/
    IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG  =  0x1,      /*The magnitude and angle are output.*/
    IVE_MAG_AND_ANG_OUT_CTRL_BUTT
}IVE_MAG_AND_ANG_OUT_CTRL_E;

typedef struct _IVE_MAG_AND_ANG_CTRL_S
{
    IVE_MAG_AND_ANG_OUT_CTRL_E enOutCtrl;
	CVI_U16 u16Thr;
    CVI_S8  as8Mask[25];         /*Template parameter.*/
}IVE_MAG_AND_ANG_CTRL_S;

typedef struct _IVE_DILATE_CTRL_S
{
    CVI_U8 au8Mask[25];         /*The template parameter value must be 0 or 255.*/
}IVE_DILATE_CTRL_S;

typedef struct _IVE_ERODE_CTRL_S
{
    CVI_U8 au8Mask[25];         /*The template parameter value must be 0 or 255.*/
}IVE_ERODE_CTRL_S;

typedef enum _IVE_THRESH_MODE_E
{
    IVE_THRESH_MODE_BINARY       =  0x0,  /*srcVal <= lowThr, dstVal = minVal; srcVal > lowThr, dstVal = maxVal.*/
    IVE_THRESH_MODE_TRUNC        =  0x1,  /*srcVal <= lowThr, dstVal = srcVal; srcVal > lowThr, dstVal = maxVal.*/
    IVE_THRESH_MODE_TO_MINVAL    =  0x2,  /*srcVal <= lowThr, dstVal = minVal; srcVal > lowThr, dstVal = srcVal.*/

	IVE_THRESH_MODE_MIN_MID_MAX  =  0x3,  /*srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= _ghThr, dstVal = midVal; srcVal > _ghThr, dstVal = maxVal.*/
	IVE_THRESH_MODE_ORI_MID_MAX  =  0x4,  /*srcVal <= lowThr, dstVal = srcVal;  lowThr < srcVal <= _ghThr, dstVal = midVal; srcVal > _ghThr, dstVal = maxVal.*/
	IVE_THRESH_MODE_MIN_MID_ORI  =  0x5,  /*srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= _ghThr, dstVal = midVal; srcVal > _ghThr, dstVal = srcVal.*/
	IVE_THRESH_MODE_MIN_ORI_MAX  =  0x6,  /*srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= _ghThr, dstVal = srcVal; srcVal > _ghThr, dstVal = maxVal.*/
	IVE_THRESH_MODE_ORI_MID_ORI  =  0x7,  /*srcVal <= lowThr, dstVal = srcVal;  lowThr < srcVal <= _ghThr, dstVal = midVal; srcVal > _ghThr, dstVal = srcVal.*/

	IVE_THRESH_MODE_BUTT
}IVE_THRESH_MODE_E;

typedef struct _IVE_THRESH_CTRL_S
{
    IVE_THRESH_MODE_E enMode;
    CVI_U8 u8LowThr;			/*user-defined threshold,  0<=u8LowThr<=255 */
	CVI_U8 u8HighThr;		/*user-defined threshold, if enMode<IVE_THRESH_MODE_MIN_MID_MAX, u8HighThr is not used, else 0<=u8LowThr<=u8HighThr<=255;*/
    CVI_U8 u8MinVal;			/*Minimum value when tri-level thresholding*/
    CVI_U8 u8MidVal;			/*Middle value when tri-level thresholding, if enMode<2, u32MidVal is not used; */
	CVI_U8 u8MaxVal;			/*Maxmum value when tri-level thresholding*/
}IVE_THRESH_CTRL_S;

typedef enum _IVE_SUB_MODE_E
{
    IVE_SUB_MODE_ABS	=  0x0,	  /*Absolute value of the difference*/
    IVE_SUB_MODE_SHIFT  =  0x1,   /*The output result is obtained by s_fting the result one digit right to reserve the signed bit.*/
    IVE_SUB_MODE_BUTT
}IVE_SUB_MODE_E;

typedef struct _IVE_SUB_CTRL_S
{
	IVE_SUB_MODE_E enMode;
}IVE_SUB_CTRL_S;

typedef enum _IVE_INTEG_OUT_CTRL_E
{
	IVE_INTEG_OUT_CTRL_COMBINE  =  0x0,
	IVE_INTEG_OUT_CTRL_SUM	    =  0x1,
	IVE_INTEG_OUT_CTRL_SQSUM    =  0x2,
	IVE_INTEG_OUT_CTRL_BUTT
}IVE_INTEG_OUT_CTRL_E;

typedef struct _IVE_INTEG_CTRL_S
{
	IVE_INTEG_OUT_CTRL_E enOutCtrl;
}IVE_INTEG_CTRL_S;

typedef enum _IVE_THRESH_S16_MODE_E
{
    IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX  =  0x0,
    IVE_THRESH_S16_MODE_S16_TO_S8_MIN_ORI_MAX  =  0x1,
    IVE_THRESH_S16_MODE_S16_TO_U8_MIN_MID_MAX  =  0x2,
    IVE_THRESH_S16_MODE_S16_TO_U8_MIN_ORI_MAX  =  0x3,

    IVE_THRESH_S16_MODE_BUTT
}IVE_THRESH_S16_MODE_E;

typedef struct _IVE_THRESH_S16_CTRL_S
{
    IVE_THRESH_S16_MODE_E enMode;
    CVI_S16 s16LowThr;		/*User-defined threshold*/
    CVI_S16 s16HighThr;		/*User-defined threshold*/
    IVE_8BIT_U un8MinVal;	/*Minimum value when tri-level thresholding*/
    IVE_8BIT_U un8MidVal;	/*Middle value when tri-level thresholding*/
    IVE_8BIT_U un8MaxVal;	/*Maxmum value when tri-level thresholding*/
}IVE_THRESH_S16_CTRL_S;

typedef enum _IVE_THRESH_U16_MODE_E
{
    IVE_THRESH_U16_MODE_U16_TO_U8_MIN_MID_MAX  =  0x0,
    IVE_THRESH_U16_MODE_U16_TO_U8_MIN_ORI_MAX  =  0x1,

    IVE_THRESH_U16_MODE_BUTT
}IVE_THRESH_U16_MODE_E;

typedef struct _IVE_THRESH_U16_CTRL_S
{
    IVE_THRESH_U16_MODE_E enMode;
    CVI_U16 u16LowThr;
    CVI_U16 u16HighThr;
    CVI_U8  u8MinVal;
    CVI_U8  u8MidVal;
    CVI_U8  u8MaxVal;
}IVE_THRESH_U16_CTRL_S;

typedef enum _IVE_16BIT_TO_8BIT_MODE_E
{
	IVE_16BIT_TO_8BIT_MODE_S16_TO_S8		=  0x0,
	IVE_16BIT_TO_8BIT_MODE_S16_TO_U8_ABS	=  0x1,
	IVE_16BIT_TO_8BIT_MODE_S16_TO_U8_BIAS	=  0x2,
	IVE_16BIT_TO_8BIT_MODE_U16_TO_U8		=  0x3,

	IVE_16BIT_TO_8BIT_MODE_BUTT
}IVE_16BIT_TO_8BIT_MODE_E;

typedef struct _IVE_16BIT_TO_8BIT_CTRL_S
{
	IVE_16BIT_TO_8BIT_MODE_E enMode;
 	CVI_U16 u16Denominator;
	CVI_U8  u8Numerator;
	CVI_S8  s8Bias;
}IVE_16BIT_TO_8BIT_CTRL_S;

typedef enum _IVE_ORD_STAT_FILTER_MODE_E
{
    IVE_ORD_STAT_FILTER_MODE_MEDIAN  =  0x0,
    IVE_ORD_STAT_FILTER_MODE_MAX     =  0x1,
    IVE_ORD_STAT_FILTER_MODE_MIN     =  0x2,

    IVE_ORD_STAT_FILTER_MODE_BUTT
}IVE_ORD_STAT_FILTER_MODE_E;

typedef struct _IVE_ORD_STAT_FILTER_CTRL_S
{
    IVE_ORD_STAT_FILTER_MODE_E enMode;

}IVE_ORD_STAT_FILTER_CTRL_S;

typedef enum _IVE_MAP_MODE_E
{
    IVE_MAP_MODE_U8  =  0x0,
    IVE_MAP_MODE_S16 =  0x1,
    IVE_MAP_MODE_U16 =  0x2,

    IVE_MAP_MODE_BUTT
}IVE_MAP_MODE_E;

typedef struct _IVE_MAP_CTRL_S
{
    IVE_MAP_MODE_E enMode;
}IVE_MAP_CTRL_S;

typedef struct _IVE_MAP_U8BIT_LUT_MEM_S
{
    CVI_U8  au8Map[IVE_MAP_NUM];
}IVE_MAP_U8BIT_LUT_MEM_S;

typedef struct _IVE_MAP_U16BIT_LUT_MEM_S
{
    CVI_U16  au16Map[IVE_MAP_NUM];
}IVE_MAP_U16BIT_LUT_MEM_S;

typedef struct _IVE_MAP_S16BIT_LUT_MEM_S
{
    CVI_S16  as16Map[IVE_MAP_NUM];
}IVE_MAP_S16BIT_LUT_MEM_S;

typedef struct _IVE_EQUALIZE_HIST_CTRL_MEM_S
{
    CVI_U32 au32Hist[IVE_HIST_NUM];
    CVI_U8  au8Map[IVE_MAP_NUM];
}IVE_EQUALIZE_HIST_CTRL_MEM_S;

typedef struct _IVE_EQUALIZE_HIST_CTRL_S
{
    IVE_MEM_INFO_S stMem;
}IVE_EQUALIZE_HIST_CTRL_S;

typedef struct _IVE_ADD_CTRL_S
{
	CVI_U0Q16 u0q16X;		 /*x of "xA+yB"*/
	CVI_U0Q16 u0q16Y;         /*y of "xA+yB"*/
}IVE_ADD_CTRL_S;

typedef struct _IVE_NCC_DST_MEM_S
{
    CVI_U64 u64Numerator;
    CVI_U64 u64QuadSum1;
    CVI_U64 u64QuadSum2;
    CVI_U8  u8Reserved[8];
}IVE_NCC_DST_MEM_S;

typedef struct _IVE_REGION_S
{
    CVI_U32 u32Area;			   /*Represented by the pixel number*/
    CVI_U16 u16Left;            /*Circumscribed rectangle left border*/
    CVI_U16 u16Right;           /*Circumscribed rectangle right border*/
    CVI_U16 u16Top;             /*Circumscribed rectangle top border*/
    CVI_U16 u16Bottom;          /*Circumscribed rectangle bottom border*/
}IVE_REGION_S;

// sync with RTL
// typedef struct _IVE_REGION_S
// {
//     CVI_U16 u16Bottom;
//     CVI_U16 u16Top;
//     CVI_U16 u16Left;
//     CVI_U16 u16Right;
//     CVI_U32 u32Area;
// }IVE_REGION_S;

typedef struct _IVE_CCBLOB_S
{
    CVI_U16 u16CurAreaThr;                         /*Threshold of the result regions' area*/
    CVI_S8  s8LabelStatus;                         /*-1: Labeled failed ; 0: Labeled successfully*/
    CVI_U8  u8RegionNum;                           /*Number of valid region, non-continuous stored*/
    IVE_REGION_S astRegion[IVE_MAX_REGION_NUM];	  /*Valid regions with 'u32Area>0' and 'label = ArrayIndex+1'*/
}IVE_CCBLOB_S;

typedef enum _IVE_CCL_MODE_E
{
	IVE_CCL_MODE_4C  =  0x0,/*4-connected*/
	IVE_CCL_MODE_8C  =  0x1,/*8-connected*/

	IVE_CCL_MODE_BUTT
}IVE_CCL_MODE_E;

typedef struct _IVE_CCL_CTRL_S
{
	IVE_CCL_MODE_E enMode;	  /*Mode*/
    CVI_U16 u16InitAreaThr;    /*Init threshold of region area*/
    CVI_U16 u16Step;           /*Increase area step for once*/
}IVE_CCL_CTRL_S;

typedef struct _IVE_GMM_CTRL_S
{
    CVI_U22Q10    u22q10NoiseVar;        /*Initial noise Variance*/
    CVI_U22Q10    u22q10MaxVar;          /*Max  Variance*/
    CVI_U22Q10    u22q10MinVar;          /*Min  Variance*/
    CVI_U0Q16     u0q16LearnRate;        /*Learning rate*/
    CVI_U0Q16     u0q16BgRatio;			/*Background ratio*/
    CVI_U8Q8      u8q8VarThr;			/*Variance Threshold*/
    CVI_U0Q16     u0q16InitWeight;       /*Initial Weight*/
    CVI_U8        u8ModelNum;            /*Model number: 3 or 5*/
}IVE_GMM_CTRL_S;

typedef enum _IVE_GMM2_SNS_FACTOR_MODE_E
{
	IVE_GMM2_SNS_FACTOR_MODE_GLB   =  0x0,   /*Global sensitivity factor mode*/
	IVE_GMM2_SNS_FACTOR_MODE_PIX   =  0x1,   /*Pixel sensitivity factor mode*/

	IVE_GMM2_SNS_FACTOR_MODE_BUTT
}IVE_GMM2_SNS_FACTOR_MODE_E;

typedef enum _IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_E
{
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_GLB  =  0x0, /*Global life update factor mode*/
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_PIX  =  0x1, /*Pixel life update factor mode*/

	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_BUTT
}IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_E ;

typedef struct _IVE_GMM2_CTRL_S
{
	IVE_GMM2_SNS_FACTOR_MODE_E			enSnsFactorMode;		  /*Sensitivity factor mode*/
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_E	enLifeUpdateFactorMode;   /*Life update factor mode*/
	CVI_U16								u16GlbLifeUpdateFactor;   /*Global life update factor (default: 4)*/
	CVI_U16								u16LifeThr;               /*Life threshold (default: 5000)*/
	CVI_U16								u16FreqInitVal;           /*Initial frequency (default: 20000)*/
	CVI_U16								u16FreqReduFactor;        /*Frequency reduction factor (default: 0xFF00)*/
	CVI_U16								u16FreqAddFactor;         /*Frequency adding factor (default: 0xEF)*/
	CVI_U16								u16FreqThr;               /*Frequency threshold (default: 12000)*/
	CVI_U16								u16VarRate;               /*Variation update rate (default: 1)*/
	CVI_U9Q7								u9q7MaxVar;               /*Max variation (default: (16 * 16)<<7)*/
	CVI_U9Q7								u9q7MinVar;               /*Min variation (default: ( 8 *  8)<<7)*/
	CVI_U8								u8GlbSnsFactor;           /*Global sensitivity factor (default: 8)*/
	CVI_U8								u8ModelNum;               /*Model number (range: 1~5, default: 3)*/
}IVE_GMM2_CTRL_S;

typedef struct _IVE_CANNY_HYS_EDGE_CTRL_S
{
    IVE_MEM_INFO_S stMem;
    CVI_U16 u16LowThr;
    CVI_U16 u16HighThr;
    CVI_S8 as8Mask[25];
} IVE_CANNY_HYS_EDGE_CTRL_S;

typedef struct _IVE_CANNY_STACK_SIZE_S
{
    CVI_U32 u32StackSize;   /*Stack size for output*/
    CVI_U8 u8Reserved[12];  /*For 16 byte align*/
}IVE_CANNY_STACK_SIZE_S;

typedef enum _IVE_LBP_CMP_MODE_E
{
    IVE_LBP_CMP_MODE_NORMAL = 0x0,	/* P(x)-P(center)>= un8BitThr.s8Val, s(x)=1; else s(x)=0; */
    IVE_LBP_CMP_MODE_ABS = 0x1,		/* Abs(P(x)-P(center))>=un8BitThr.u8Val, s(x)=1; else s(x)=0; */

    IVE_LBP_CMP_MODE_BUTT
}IVE_LBP_CMP_MODE_E;

typedef struct _IVE_LBP_CTRL_S
{
    IVE_LBP_CMP_MODE_E enMode;
    IVE_8BIT_U un8BitThr;
}IVE_LBP_CTRL_S;

typedef enum _IVE_NORM_GRAD_OUT_CTRL_E
{
	IVE_NORM_GRAD_OUT_CTRL_HOR_AND_VER  =  0x0,
	IVE_NORM_GRAD_OUT_CTRL_HOR		    =  0x1,
	IVE_NORM_GRAD_OUT_CTRL_VER			=  0x2,
	IVE_NORM_GRAD_OUT_CTRL_COMBINE      =  0x3,

	IVE_NORM_GRAD_OUT_CTRL_BUTT
}IVE_NORM_GRAD_OUT_CTRL_E;

typedef struct _IVE_NORM_GRAD_CTRL_S
{
	IVE_NORM_GRAD_OUT_CTRL_E enOutCtrl;
	CVI_S8 as8Mask[25];
	CVI_U8 u8Norm;
}IVE_NORM_GRAD_CTRL_S;

typedef enum _IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_E
{
	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_NONE   = 0,	/*Output none*/
	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_STATUS = 1,	/*Output status*/
	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_BOTH   = 2,	/*Output status and err*/

	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_BUTT
}IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_E;

typedef struct _IVE_LK_OPTICAL_FLOW_PYR_CTRL_S
{
	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_E enOutMode;
    CVI_BOOL     bUseInitFlow;		/*where to use initial flow*/
    CVI_U16	    u16PtsNum;		    /*Number of the feature points,<=500*/
    CVI_U8       u8MaxLevel;         /*0<=u8MaxLevel<=3*/
    CVI_U0Q8     u0q8MinEigThr;		/*Minimum eigenvalue threshold*/
    CVI_U8	    u8IterCnt;          /*Maximum iteration times, <=20*/
    CVI_U0Q8     u0q8Eps;            /*Used for exit criteria: dx^2 + dy^2 < u0q8Eps */
}IVE_LK_OPTICAL_FLOW_PYR_CTRL_S;

typedef struct _IVE_ST_MAX_EIG_S
{
    CVI_U16 u16MaxEig;           /*S_-Tomasi second step output MaxEig*/
    CVI_U8  u8Reserved[14];      /*For 16 byte align*/
}IVE_ST_MAX_EIG_S;

typedef struct _IVE_ST_CANDI_CORNER_CTRL_S
{
	IVE_MEM_INFO_S stMem;
	CVI_U0Q8 u0q8QualityLevel;
}IVE_ST_CANDI_CORNER_CTRL_S;

typedef struct _IVE_ST_CORNER_INFO_S
{
    CVI_U16 u16CornerNum;
    IVE_POINT_U16_S astCorner[IVE_ST_MAX_CORNER_NUM];
}IVE_ST_CORNER_INFO_S;

typedef struct _IVE_ST_CORNER_CTRL_S
{
	CVI_U16 u16MaxCornerNum;
	CVI_U16 u16MinDist;
}IVE_ST_CORNER_CTRL_S;

typedef enum _IVE_GRAD_FG_MODE_E
{
    IVE_GRAD_FG_MODE_USE_CUR_GRAD  =  0x0,
    IVE_GRAD_FG_MODE_FIND_MIN_GRAD =  0x1,

    IVE_GRAD_FG_MODE_BUTT
}IVE_GRAD_FG_MODE_E;

typedef struct _IVE_GRAD_FG_CTRL_S
{
    IVE_GRAD_FG_MODE_E enMode;		/*Calculation mode*/
    CVI_U16 u16EdwFactor;			/*Edge width adjustment factor (range: 500 to 2000; default: 1000)*/
    CVI_U8 u8CrlCoefThr;				/*Gradient vector correlation coefficient threshold (ranges: 50 to 100; default: 80)*/
    CVI_U8 u8MagCrlThr;				/*Gradient amplitude threshold (range: 0 to 20; default: 4)*/
    CVI_U8 u8MinMagDiff;				/*Gradient magnitude difference threshold (range: 2 to 8; default: 2)*/
    CVI_U8 u8NoiseVal;				/*Gradient amplitude noise threshold (range: 1 to 8; default: 1)*/
    CVI_U8 u8EdwDark;				/*Black pixels enable flag (range: 0 (no), 1 (yes); default: 1)*/
}IVE_GRAD_FG_CTRL_S;

typedef struct _IVE_CANDI_BG_PIX_S
{
    CVI_U8Q4F4 u8q4f4Mean;			/*Candidate background grays value */
    CVI_U16 u16StartTime;			/*Candidate Background start time */
    CVI_U16 u16SumAccessTime;		/*Candidate Background cumulative access time */
    CVI_U16 u16ShortKeepTime;		/*Candidate background short hold time*/
    CVI_U8 u8ChgCond;				/*Time condition for candidate background into the changing state*/
    CVI_U8 u8PotenBgLife;			/*Potential background cumulative access time */
}IVE_CANDI_BG_PIX_S;

typedef struct _IVE_WORK_BG_PIX_S
{
    CVI_U8Q4F4 u8q4f4Mean;			/*0# background grays value */
    CVI_U16 u16AccTime;				/*Background cumulative access time */
    CVI_U8 u8PreGray;				/*Gray value of last pixel */
    CVI_U5Q3 u5q3DiffThr;			/*Differential threshold */
    CVI_U8 u8AccFlag;				/*Background access flag */
    CVI_U8 u8BgGray[3];				/*1# ~ 3# background grays value */
}IVE_WORK_BG_PIX_S;

typedef struct _IVE_BG_LIFE_S
{
    CVI_U8 u8WorkBgLife[3];			/*1# ~ 3# background vitality */
    CVI_U8 u8CandiBgLife;			/*Candidate background vitality */
}IVE_BG_LIFE_S;

typedef struct _IVE_BG_MODEL_PIX_S
{
    IVE_WORK_BG_PIX_S	stWorkBgPixel;	/*Working background */
    IVE_CANDI_BG_PIX_S	stCandiPixel;	/*Candidate background */
    IVE_BG_LIFE_S		stBgLife;		/*Background vitality */
}IVE_BG_MODEL_PIX_S;

typedef struct _IVE_FG_STAT_DATA_S
{
    CVI_U32 u32PixNum;
    CVI_U32 u32SumLum;
    CVI_U8  u8Reserved[8];
}IVE_FG_STAT_DATA_S;

typedef struct _IVE_BG_STAT_DATA_S
{
    CVI_U32 u32PixNum;
    CVI_U32 u32SumLum;
    CVI_U8  u8Reserved[8];
}IVE_BG_STAT_DATA_S;

typedef struct _IVE_MATCH_BG_MODEL_CTRL_S
{
    CVI_U32 u32CurFrmNum;		/*Current frame timestamp, in frame units */
    CVI_U32 u32PreFrmNum;		/*Previous frame timestamp, in frame units */
    CVI_U16 u16TimeThr;			/*Potential background replacement time threshold (range: 2 to 100 frames; default: 20) */

    CVI_U8 u8DiffThrCrlCoef;		/*Correlation coefficients between differential threshold and gray value (range: 0 to 5; default: 0) */
    CVI_U8 u8DiffMaxThr;			/*Maximum of background differential threshold (range: 3 to 15; default: 6) */
    CVI_U8 u8DiffMinThr;			/*Minimum of background differential threshold (range: 3 to 15; default: 4) */
    CVI_U8 u8DiffThrInc;			/*Dynamic Background differential threshold increment (range: 0 to 6; default: 0) */
    CVI_U8 u8FastLearnRate;		/*Quick background learning rate (range: 0 to 4; default: 2) */
    CVI_U8 u8DetChgRegion;		/*Whether to detect change region (range: 0 (no), 1 (yes); default: 0) */
}IVE_MATCH_BG_MODEL_CTRL_S;

typedef struct _IVE_UPDATE_BG_MODEL_CTRL_S
{
    CVI_U32 u32CurFrmNum;			/*Current frame timestamp, in frame units */
    CVI_U32 u32PreChkTime;			/*The last time when background status is checked */
    CVI_U32 u32FrmChkPeriod;			/*Background status checking period (range: 0 to 2000 frames; default: 50) */

    CVI_U32 u32InitMinTime;			/*Background initialization shortest time (range: 20 to 6000 frames; default: 100)*/
    CVI_U32 u32StyBgMinBlendTime;	/*Steady background integration shortest time (range: 20 to 6000 frames; default: 200)*/
    CVI_U32 u32StyBgMaxBlendTime;	/*Steady background integration longest time (range: 20 to 40000 frames; default: 1500)*/
    CVI_U32 u32DynBgMinBlendTime;	/*Dynamic background integration shortest time (range: 0 to 6000 frames; default: 0)*/
    CVI_U32 u32StaticDetMinTime;		/*Still detection shortest time (range: 20 to 6000 frames; default: 80)*/
    CVI_U16 u16FgMaxFadeTime;		/*Foreground disappearing longest time (range: 1 to 255 seconds; default: 15)*/
    CVI_U16 u16BgMaxFadeTime;		/*Background disappearing longest time (range: 1 to 255  seconds ; default: 60)*/

    CVI_U8 u8StyBgAccTimeRateThr;	/*Steady background access time ratio threshold (range: 10 to 100; default: 80)*/
    CVI_U8 u8ChgBgAccTimeRateThr;	/*Change background access time ratio threshold (range: 10 to 100; default: 60)*/
    CVI_U8 u8DynBgAccTimeThr;		/*Dynamic background access time ratio threshold (range: 0 to 50; default: 0)*/
    CVI_U8 u8DynBgDepth;				/*Dynamic background depth (range: 0 to 3; default: 3)*/
    CVI_U8 u8BgEffStaRateThr;		/*Background state time ratio threshold when initializing (range: 90 to 100; default: 90)*/

    CVI_U8 u8AcceBgLearn;			/*Whether to accelerate background learning (range: 0 (no), 1 (yes); default: 0)*/
    CVI_U8 u8DetChgRegion;			/*Whether to detect change region (range: 0 (no), 1 (yes); default: 0)*/
} IVE_UPDATE_BG_MODEL_CTRL_S;

typedef enum _IVE_ANN_MLP_ACTIV_FUNC_E
{
    IVE_ANN_MLP_ACTIV_FUNC_IDENTITY     = 0x0,
    IVE_ANN_MLP_ACTIV_FUNC_SIGMOID_SYM  = 0x1,
    IVE_ANN_MLP_ACTIV_FUNC_GAUSSIAN     = 0x2,

    IVE_ANN_MLP_ACTIV_FUNC_BUTT
}IVE_ANN_MLP_ACTIV_FUNC_E;

typedef enum _IVE_ANN_MLP_ACCURATE_E
{
    IVE_ANN_MLP_ACCURATE_SRC16_WGT16    = 0x0,  /*input decimals' accurate 16 bit, weight 16bit*/
    IVE_ANN_MLP_ACCURATE_SRC14_WGT20    = 0x1,  /*input decimals' accurate 14 bit, weight 20bit*/

    IVE_ANN_MLP_ACCURATE_BUTT
}IVE_ANN_MLP_ACCURATE_E;

typedef struct _IVE_ANN_MLP_MODEL_S
{
    IVE_ANN_MLP_ACTIV_FUNC_E enActivFunc;
    IVE_ANN_MLP_ACCURATE_E   enAccurate;
    IVE_MEM_INFO_S stWeight;
    CVI_U32 u32TotalWeightSize;

    CVI_U16 au16LayerCount[8];    /*8 layers, including input and output layer*/
    CVI_U16 u16MaxCount;          /*MaxCount<=1024*/
    CVI_U8 u8LayerNum;		     /*2<layerNum<=8*/
    CVI_U8 u8Reserved;
}IVE_ANN_MLP_MODEL_S;

typedef enum _IVE_SVM_TYPE_E
{
    IVE_SVM_TYPE_C_SVC   = 0x0,
    IVE_SVM_TYPE_NU_SVC  = 0x1,

    IVE_SVM_TYPE_BUTT
}IVE_SVM_TYPE_E;

typedef enum _IVE_SVM_KERNEL_TYPE_E
{
    IVE_SVM_KERNEL_TYPE_LINEAR  = 0x0,
    IVE_SVM_KERNEL_TYPE_POLY    = 0x1,
    IVE_SVM_KERNEL_TYPE_RBF     = 0x2,
    IVE_SVM_KERNEL_TYPE_SIGMOID = 0x3,

    IVE_SVM_KERNEL_TYPE_BUTT
}IVE_SVM_KERNEL_TYPE_E;

typedef struct _IVE_SVM_MODEL_S
{
    IVE_SVM_TYPE_E enType;
    IVE_SVM_KERNEL_TYPE_E enKernelType;

    IVE_MEM_INFO_S  stSv;       /*SV memory*/
    IVE_MEM_INFO_S  stDf;       /*Decision functions memory*/
    CVI_U32 u32TotalDfSize;      /*All decision functions coef size in byte*/

    CVI_U16 u16FeatureDim;
    CVI_U16 u16SvTotal;
    CVI_U8  u8ClassCount;
}IVE_SVM_MODEL_S;

typedef enum _IVE_SAD_MODE_E
{
	IVE_SAD_MODE_MB_4X4		= 0x0, /*4x4*/
	IVE_SAD_MODE_MB_8X8		= 0x1, /*8x8*/
	IVE_SAD_MODE_MB_16X16	= 0x2, /*16x16*/

	IVE_SAD_MODE_BUTT
}IVE_SAD_MODE_E;

typedef enum _IVE_SAD_OUT_CTRL_E
{
	IVE_SAD_OUT_CTRL_16BIT_BOTH	= 0x0, /*Output 16 bit sad and thresh*/
	IVE_SAD_OUT_CTRL_8BIT_BOTH	= 0x1, /*Output 8 bit sad and thresh*/
	IVE_SAD_OUT_CTRL_16BIT_SAD	= 0x2, /*Output 16 bit sad*/
	IVE_SAD_OUT_CTRL_8BIT_SAD	= 0x3, /*Output 8 bit sad*/
	IVE_SAD_OUT_CTRL_THRESH		= 0x4, /*Output thresh,16 bits sad */

	IVE_SAD_OUT_CTRL_BUTT
}IVE_SAD_OUT_CTRL_E;

typedef struct _IVE_SAD_CTRL_S
{
	IVE_SAD_MODE_E enMode;
	IVE_SAD_OUT_CTRL_E enOutCtrl;
	CVI_U16 u16Thr;				/*srcVal <= u16Thr, dstVal = minVal; srcVal > u16Thr, dstVal = maxVal.*/
	CVI_U8 u8MinVal;				/*Min value*/
	CVI_U8 u8MaxVal;				/*Max value*/
}IVE_SAD_CTRL_S;

typedef enum _IVE_RESIZE_MODE_E
{
	IVE_RESIZE_MODE_LINEAR   =  0x0, /*Bilinear interpolation*/
	IVE_RESIZE_MODE_AREA	 =  0x1, /*Area-based (or super) interpolation*/

	IVE_RESIZE_MODE_BUTT
}IVE_RESIZE_MODE_E;

typedef struct _IVE_RESIZE_CTRL_S
{
	IVE_RESIZE_MODE_E enMode;
    IVE_MEM_INFO_S stMem;
	CVI_U16  u16Num;
}IVE_RESIZE_CTRL_S;

typedef enum _IVE_CNN_ACTIV_FUNC_E
{
    IVE_CNN_ACTIV_FUNC_NONE	   = 0x0,  /*Do not taking a activation, equivalent f(x)=x*/
    IVE_CNN_ACTIV_FUNC_RELU    = 0x1,  /*f(x)=max(0, x)*/
    IVE_CNN_ACTIV_FUNC_SIGMOID = 0x2,  /*f(x)=1/(1+exp(-x)), not support*/

    IVE_CNN_ACTIV_FUNC_BUTT
}IVE_CNN_ACTIV_FUNC_E;

typedef enum _IVE_CNN_POOLING_E
{
    IVE_CNN_POOLING_NONE =0x0, /*Do not taking a pooling action*/
    IVE_CNN_POOLING_MAX  =0x1, /*Using max value of every pooling area*/
    IVE_CNN_POOLING_AVG  =0x2, /*Using average value of every pooling area*/

    IVE_CNN_POOLING_BUTT
}IVE_CNN_POOLING_E;

typedef struct _IVE_CNN_CONV_POOLING_S
{
    IVE_CNN_ACTIV_FUNC_E enActivFunc; /*Type of activation function*/
    IVE_CNN_POOLING_E    enPooling;   /*Mode of pooling method*/

	CVI_U8  u8FeatureMapNum;	    /*Number of feature maps*/
    CVI_U8  u8KernelSize;		/*Kernel size, only support 3 currently*/
    CVI_U8  u8ConvStep;			/*Convolution step, only support 1 currently*/

    CVI_U8  u8PoolSize;			/*Pooling size, only support 2 currently*/
    CVI_U8  u8PoolStep;			/*Pooling step, only support 2 currently*/
    CVI_U8  u8Reserved[3];

}IVE_CNN_CONV_POOLING_S;

typedef struct _IVE_CNN_FULL_CONNECT_S
{
    CVI_U16 au16LayerCnt[8];		/*Neuron number of every fully connected layers*/
    CVI_U16 u16MaxCnt;			/*Max neuron number in all fully connected layers*/
    CVI_U8 u8LayerNum;			/*Number of fully connected layer*/
    CVI_U8 u8Reserved;
}IVE_CNN_FULL_CONNECT_S;

typedef struct _IVE_CNN_MODEL_S
{
    IVE_CNN_CONV_POOLING_S astConvPool[8];  /*Conv-ReLU-Pooling layers info*/
    IVE_CNN_FULL_CONNECT_S stFullConnect;   /*Fully connected layers info*/

    IVE_MEM_INFO_S stConvKernelBias;	    /*Conv-ReLU-Pooling layers' kernels and bias*/
    CVI_U32 u32ConvKernelBiasSize;           /*Size of Conv-ReLU-Pooling layer' kernels and bias*/

    IVE_MEM_INFO_S stFCLWgtBias;		    /*Fully Connection Layers' weights and bias*/
    CVI_U32 u32FCLWgtBiasSize;               /*Size of fully connection layers weights and bias*/

    CVI_U32 u32TotalMemSize;                 /*Total memory size of all kernels, weights, bias*/

    IVE_IMAGE_TYPE_E enType;		        /*Image type used for the CNN model*/
    CVI_U32 u32Width;                        /*Image width used for the model*/
    CVI_U32 u32Height;                       /*Image height used for the model*/

	CVI_U16 u16ClassCount;                   /*Number of classes*/
    CVI_U8  u8ConvPoolLayerNum;              /*Number of Conv-ReLU-Pooling layers*/
    CVI_U8  u8Reserved;
}IVE_CNN_MODEL_S;

typedef struct _IVE_CNN_CTRL_S
{
	IVE_MEM_INFO_S stMem;   /*Assist memory*/
    CVI_U32 u32Num;			/*Input image number*/
}IVE_CNN_CTRL_S;

typedef struct _IVE_CNN_RESULT_S
{
    CVI_S32 s32ClassIdx;     /*The most possible index of the classification*/
    CVI_S32 s32Confidence;   /*The confidence of the classification*/
}IVE_CNN_RESULT_S;

typedef enum _IVE_BERNSEN_MODE_E
{
    IVE_BERNSEN_MODE_NORMAL = 0x0,          /*Simple Bernsen thresh*/
    IVE_BERNSEN_MODE_THRESH = 0x1,          /*Thresh based on the global threshold and local Bernsen threshold*/
    IVE_BERNSEN_MODE_PAPER  = 0x2,          /*This method is same with original paper*/
    IVE_BERNSEN_MODE_BUTT
} IVE_BERNSEN_MODE_E;

typedef struct _IVE_BERNSEN_CTRL_S
{
    IVE_BERNSEN_MODE_E enMode;
    CVI_U8 u8WinSize; /* 3x3 or 5x5 */
    CVI_U8 u8Thr;
    CVI_U8 u8ContrastThreshold; // compare with midgray
}IVE_BERNSEN_CTRL_S;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif/*_CVI_IVE_H_*/
