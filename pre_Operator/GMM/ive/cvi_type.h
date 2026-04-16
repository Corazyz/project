#ifndef __CVI_TYPE_H__
#define __CVI_TYPE_H__

#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

typedef uint8_t           CVI_U8;
typedef uint8_t           CVI_UCHAR;
typedef uint16_t          CVI_U16;
typedef uint32_t          CVI_U32;
typedef int8_t            CVI_S8;
typedef int16_t           CVI_S16;
typedef int32_t           CVI_S32;
typedef uint64_t          CVI_U64;
typedef int64_t           CVI_S64;
typedef char              CVI_CHAR;
//typedef void              CVI_VOID;
#define CVI_VOID          void
typedef unsigned long     CVI_SIZE_T;
typedef unsigned long     CVI_LENGTH_T;

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

typedef CVI_S32 IVE_HANDLE;
typedef CVI_S32 CLS_HANDLE;
typedef CVI_S32 FD_CHN;
typedef CVI_S32 MD_CHN;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __CVI_TYPE_H__ */

