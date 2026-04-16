
#include <stdio.h>

#ifndef SAMPLE_DEFINE_H_
#define SAMPLE_DEFINE_H_

#define IVE_STRIDE_ALIGN 16
#define SAMPLE_PRT(...)\
do{\
	printf("[Func]:%s [Line]:%d [Info]:", __FUNCTION__, __LINE__);\
	printf(__VA_ARGS__);\
}while(0)

#define CVI_CHECK_ET_RET(srcData,value,ret)\
do{\
	if((value) == (srcData))\
	{\
		return (ret);\
	}\
}while(0)

#define CVI_CHECK_ET_RET_VOID(srcData,value)\
do{\
	if((value) == (srcData))\
	{\
		return ;\
	}\
}while(0)

#define CVI_CHECK_NET_RET(srcData,value,ret)\
do{\
	if((value) != (srcData))\
	{\
		return (ret);\
	}\
}while(0)

#define CVI_CHECK_NET_RET_VOID(srcData,value)\
do{\
	if((value) != (srcData))\
	{\
		return ;\
	}\
}while(0)

#define CVI_CHECK_GT_RET(srcData,value,ret)\
do{\
	if((srcData) > (value))\
	{\
		return (ret);\
	}\
}while(0)


#define CVI_CHECK_GT_AND_ET_RET(srcData,value,ret)\
do{\
	if((srcData) >= (value))\
	{\
		return (ret);\
	}\
}while(0)

#define CVI_CHECK_LT_RET(srcData,value,ret)\
do{\
	if((srcData) < (value))\
	{\
		return (ret);\
	}\
}while(0)

#define CVI_CHECK_LT_AND_ET_RET(srcData,value,ret)\
do{\
	if((srcData) <= (value))\
	{\
		return (ret);\
	}\
}while(0)

#define CVI_CHECK_NET_GOTO(srcData,value,label)\
do{\
	if((value) != (srcData))\
	{\
		goto label;\
	}\
}while(0)

#define CVI_CHECK_ET_GOTO(srcData,value,label)\
do{\
	if((value) == (srcData))\
	{\
		goto label;\
	}\
}while(0)

#define CVI_CHECK_ET_NULL_RET_VOID(ptr) CVI_CHECK_ET_RET_VOID(ptr,NULL)
#define CVI_CHECK_ET_NULL_RET(ptr,ret) CVI_CHECK_ET_RET(ptr,NULL,ret)

#define CVI_CHECK_ET_RET_PRT(srcData,value,ret, ...)\
do{\
	if((value) == (srcData))\
	{\
		SAMPLE_PRT(__VA_ARGS__);\
		return (ret);\
	}\
}while(0)

#define CVI_CHECK_ET_RET_VOID_PRT(srcData,value, ...)\
do{\
	if((value) == (srcData))\
	{\
		SAMPLE_PRT(__VA_ARGS__);\
		return ;\
	}\
}while(0)

#define CVI_CHECK_NET_RET_PRT(srcData,value,ret, ...)\
do{\
	if((value) != (srcData))\
	{\
		SAMPLE_PRT(__VA_ARGS__);\
		return (ret);\
	}\
}while(0)

#define CVI_CHECK_NET_RET_VOID_PRT(srcData,value, ...)\
do{\
	if((value) != (srcData))\
	{\
		SAMPLE_PRT(__VA_ARGS__);\
		return ;\
	}\
}while(0)

#define CVI_CHECK_GT_RET_PRT(srcData,value,ret, ...)\
do{\
	if((srcData) > (value))\
	{\
		SAMPLE_PRT(__VA_ARGS__);\
		return (ret);\
	}\
}while(0)


#define CVI_CHECK_GT_AND_ET_RET_PRT(srcData,value,ret, ...)\
do{\
	if((srcData) >= (value))\
	{\
		SAMPLE_PRT(__VA_ARGS__);\
		return (ret);\
	}\
}while(0)

#define CVI_CHECK_LT_RET_PRT(srcData,value,ret, ...)\
do{\
	if((srcData) < (value))\
	{\
		SAMPLE_PRT(__VA_ARGS__);\
		return (ret);\
	}\
}while(0)

#define CVI_CHECK_LT_AND_ET_RET_PRT(srcData,value,ret, ...)\
do{\
	if((srcData) <= (value))\
	{\
		SAMPLE_PRT(__VA_ARGS__);\
		return (ret);\
	}\
}while(0)

#define CVI_CHECK_NET_GOTO_PRT(srcData,value,label, ...)\
do{\
	if((value) != (srcData))\
	{\
		SAMPLE_PRT(__VA_ARGS__);\
		goto label;\
	}\
}while(0)

#define CVI_CHECK_ET_GOTO_PRT(srcData,value,label, ...)\
do{\
	if((value) == (srcData))\
	{\
		SAMPLE_PRT(__VA_ARGS__);\
		goto label;\
	}\
}while(0)

#define CVI_CHECK_ET_NULL_RET_VOID_PRT(ptr, ...) CVI_CHECK_ET_RET_VOID_PRT(ptr,NULL,__VA_ARGS__)
#define CVI_CHECK_ET_NULL_RET_PRT(ptr,ret, ...) CVI_CHECK_ET_RET_PRT(ptr,NULL,ret,__VA_ARGS__)

#define CVI_FREE(p)\
do{\
	if (NULL != (p))\
	{\
		free((p));\
		(p) = NULL;\
	}\
} while (0)

#define CVI_FCLOSE(fp)\
do{\
	if (NULL != (fp))\
	{\
		fclose((fp));\
		(fp) = NULL;\
	}\
} while (0)

#define CVI_FREE(p)\
do{\
	if (NULL != (p))\
	{\
		free((p));\
		(p) = NULL;\
	}\
} while (0)

#define CVI_FREE_64(p)\
do{\
	if (0 != (p))\
	{\
		free(((CVI_VOID*)p));\
		(p) = 0;\
	}\
} while (0)

#define CVI_ERROR(err)                           \
do                                              \
{                                               \
    printf("\n%s, at:>File name: %s"            \
    " >Function : %s >Line No. : %d\n",         \
    err, __FILE__,__FUNCTION__,                 \
    __LINE__);                                  \
    _exit(-1);                                  \
}while(0)


#define CVI_CHECK_CONDITION_RET(expr, name, ret) \
do                                              \
{                                               \
    if (expr)                                   \
    {                                           \
        printf("%s, at %s: LINE: %d !\n",       \
        name, __FUNCTION__, __LINE__);          \
        return (ret);                           \
    }                                           \
}while(0)

#define CVI_CHECK_CONDITION_RET_VOID(expr, name) \
do                                              \
{                                               \
    if (expr)                                   \
    {                                           \
        printf("%s, at %s: LINE: %d !\n",       \
        name, __FUNCTION__, __LINE__);          \
        return;                                 \
    }                                           \
}while(0)

#define CVI_CLIP(a, maxv, minv)		 (((a)>(maxv)) ? (maxv) : (((a) < (minv)) ? (minv) : (a)))

//stride align byte
#define CVI_IVE2_STRIDE_ALIGN 16
//max file length
#define CVI_MAX_FILE_LEN  256
//image width 704
#define CVI_IMAGE_WIDTH_704  704
//image height 576
#define CVI_IMAGE_HEIGHT_576 576
//image width 352
#define CVI_IMAGE_WIDTH_352 352
//image height 288
#define CVI_IMAGE_HEIGHT_288 288
//print info len
#define CVI_INFO_LEN   100

//U8/S8/U16/S16 max and  min value define
#define CVI_MAX_U8 255
#define CVI_MIN_U8 0
#define CVI_MAX_S8 127
#define CVI_MIN_S8 -128
#define CVI_MAX_U16 65535
#define CVI_MIN_U16 0
#define CVI_MAX_S16 32767
#define CVI_MIN_S16 -32768

#ifndef WIN32
#include "limits.h"
#define _MAX_FNAME NAME_MAX
#define strcpy_s(dest,destsz,src) strncpy(dest,src,destsz)
#endif

#endif
