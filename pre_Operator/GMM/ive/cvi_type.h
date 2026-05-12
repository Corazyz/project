#ifndef __CVI_TYPE_H__
#define __CVI_TYPE_H__

#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

typedef enum {
    CVI_FALSE    = 0,
    CVI_TRUE     = 1,
} CVI_BOOL;

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
#define CVI_DBG_ERR         3    /* error conditions, sdk code: CVI_debug.h */

typedef int32_t IVE_HANDLE;
typedef int32_t CLS_HANDLE;
typedef int32_t FD_CHN;
typedef int32_t MD_CHN;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __CVI_TYPE_H__ */

