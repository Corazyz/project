#ifndef __CVI_ERRNO_H__
#define __CVI_ERRNO_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define CVI_ERR_APPID  (0x80000000L + 0x20000000L)
typedef enum hiERR_LEVEL_E
{
	EN_ERR_LEVEL_DEBUG = 0,  /* debug-level                                  */
	EN_ERR_LEVEL_INFO,       /* informational                                */
	EN_ERR_LEVEL_NOTICE,     /* normal but significant condition             */
	EN_ERR_LEVEL_WARNING,    /* warning conditions                           */
	EN_ERR_LEVEL_ERROR,      /* error conditions                             */
	EN_ERR_LEVEL_CRIT,       /* critical conditions                          */
	EN_ERR_LEVEL_ALERT,      /* action must be taken immediately             */
	EN_ERR_LEVEL_FATAL,      /* just for compatibility with previous version */
	EN_ERR_LEVEL_BUTT
}ERR_LEVEL_E;

#define CVI_DEF_ERR( module, level, errid) \
	((CVI_S32)( (CVI_ERR_APPID) | ((module) << 16 ) | ((level)<<13) | (errid) ))

typedef enum _EN_ERR_CODE_E
{
    EN_ERR_INVALID_DEVID = 1, /* invlalid device ID                           */
    EN_ERR_INVALID_CHNID = 2, /* invlalid channel ID                          */
    EN_ERR_ILLEGAL_PARAM = 3, /* at lease one parameter is illagal
                               * eg, an illegal enumeration value             */
    EN_ERR_EXIST         = 4, /* resource exists                              */
    EN_ERR_UNEXIST       = 5, /* resource unexists                            */

    EN_ERR_NULL_PTR      = 6, /* using a NULL point                           */

    EN_ERR_NOT_CONFIG    = 7, /* try to enable or initialize system, device
                              ** or channel, before configing attribute       */

    EN_ERR_NOT_SUPPORT   = 8, /* operation or type is not supported by NOW    */
    EN_ERR_NOT_PERM      = 9, /* operation is not permitted
                              ** eg, try to change static attribute           */

    EN_ERR_NOMEM         = 12,/* failure caused by malloc memory              */
    EN_ERR_NOBUF         = 13,/* failure caused by malloc buffer              */

    EN_ERR_BUF_EMPTY     = 14,/* no data in buffer                            */
    EN_ERR_BUF_FULL      = 15,/* no buffer for new data                       */

    EN_ERR_SYS_NOTREADY  = 16,/* System is not ready,maybe not initialed or
                              ** loaded. Returning the error code when opening
                              ** a device file failed.                        */

    EN_ERR_BADADDR       = 17,/* bad address,
                              ** eg. used for copy_from_user & copy_to_user   */

    EN_ERR_BUSY          = 18,/* resource is busy,
                              ** eg. destroy a venc chn without unregister it */

    EN_ERR_BUTT          = 63,/* maxium code, private error code of all modules
                              ** must be greater than it                      */
}EN_ERR_CODE_E;

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
    CVI_ID_RGN	  = 18,
    CVI_ID_RC      = 19,

    CVI_ID_SIO     = 20,
    CVI_ID_AI      = 21,
    CVI_ID_AO      = 22,
    CVI_ID_AENC    = 23,
    CVI_ID_ADEC    = 24,

    CVI_ID_AVENC   = 25,

    CVI_ID_PCIV    = 26,
    CVI_ID_PCIVFMW = 27,

    CVI_ID_ISP	  = 28,

    CVI_ID_IVE	  = 29,
    /* there is a hole */

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

    CVI_ID_FD	  = 47,
    CVI_ID_ODT	  = 48, //Object detection trace
    CVI_ID_VQA	  = 49,//Video quality  analysis
    CVI_ID_LPR 	  = 50, // License Plate Recognition

    CVI_ID_BUTT,
} MOD_ID_E;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __CVI_ERRNO_H__ */

