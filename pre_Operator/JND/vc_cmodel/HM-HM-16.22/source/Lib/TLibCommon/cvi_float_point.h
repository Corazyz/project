
#ifndef __CVI_FLOAT_POINT__
#define __CVI_FLOAT_POINT__

#include <stdint.h>
#include "cvi_soft_float.h"

typedef sw_float float32;
float SoftFloat_to_Float(unsigned int v);
unsigned int Float_to_SoftFloat(float v);

float32 INT_TO_CVI_FLOAT(int32_t a);
int32_t CVI_FLOAT_TO_INT(float32 a);
int32_t CVI_FLOAT_TO_INT_MODE(float32 a, int mode);

float32 CVI_FLOAT_ADD(float32 a, float32 b);
float32 CVI_FLOAT_SUB(float32 a, float32 b);
float32 CVI_FLOAT_MUL(float32 a, float32 b);
float32 CVI_FLOAT_EXP(float32 a);
float32 CVI_FLOAT_LOG(float32 a);
float32 CVI_FLOAT_POW(float32 a, float32 b);
float32 CVI_FLOAT_DIV(float32 a, float32 b);

uint32_t CVI_FLOAT_EQ(float32 a, float32 b);
uint32_t CVI_FLOAT_LT(float32 a, float32 b);
uint32_t CVI_FLOAT_LE(float32 a, float32 b);
uint32_t CVI_FLOAT_GT(float32 a, float32 b);
uint32_t CVI_FLOAT_GE(float32 a, float32 b);

float32 CVI_FLOAT_MIN(float32 a, float32 b);
float32 CVI_FLOAT_MAX(float32 a, float32 b);
float32 CVI_FLOAT_CLIP(float32 low, float32 high, float32 a);
#endif
