#include "stdio.h"
#include "math.h"
#define ALIGN(x, a) (((x) + (a) & ~ (a)))
int main() {
    int local_mem = 262144;
    int kernel_size[] = {9, 15, 21, 27};
    int mem_avail;
    int width_max[4];
    for (int i = 0; i < 4; i++) {
        mem_avail = local_mem - ALIGN(kernel_size[i] * kernel_size[i] * 2, 64);
        mem_avail = (mem_avail / 4)/64 * 64;
        width_max[i] = (mem_avail / ((kernel_size[i] + 3) * 2)) - (kernel_size[i] - 1);
        printf("width_max[%d] = %d\n", i, width_max[i]);
    }


}

// #include <stdio.h>
// #include <stdint.h>

// // 定义 float16 结构
// typedef union {
//     unsigned short bits;
//     struct {
//         unsigned short frac : 10; // mantissa
//         unsigned short exp  : 5;  // exponent
//         unsigned short sign : 1;  // sign
//     } format;
// } float16;

// // 定义 scalar_t 联合体
// typedef union {
//     char           s4;
//     unsigned char  u4;
//     char           s8;
//     unsigned char  u8;
//     short          s16;
//     unsigned short u16;
//     float16        f16;
//     // bfloat16       bf16; // 假设 bfloat16 已经定义
//     int            s32;
//     unsigned int   u32;
//     float          f32;
// } scalar_t;

// // 将 float 转换为 float16
// float16 float_to_float16(float value) {
//     const float FLOAT16_MAX = 65504.0f;
//     const float FLOAT16_MIN_NORMAL = 6.103515625e-5f;
//     const float FLOAT16_MIN_DENORMAL = 5.9604644775390625e-8f;

//     if (value == 0.0f) {
//         return (float16){ .bits = 0 };
//     }

//     if (value >= FLOAT16_MAX || value <= -FLOAT16_MAX) {
//         return (float16){ .bits = 0x7FFF }; // Inf
//     }

//     if (value < FLOAT16_MIN_NORMAL && value > -FLOAT16_MIN_NORMAL) {
//         if (value >= -FLOAT16_MIN_DENORMAL && value <= FLOAT16_MIN_DENORMAL) {
//             return (float16){ .bits = 0 }; // Denormal
//         }
//     }

//     union {
//         float f;
//         uint32_t i;
//     } u = { .f = value };

//     int sign = (u.i >> 31) & 0x1;
//     int exp = ((u.i >> 23) & 0xFF) - 127 + 15;
//     int frac = (u.i >> 13) & 0x3FF;

//     if (exp >= 31) {
//         return (float16){ .bits = 0x7FFF }; // Inf
//     }

//     if (exp < 0) {
//         if (exp < -10) {
//             return (float16){ .bits = 0 }; // Zero
//         }
//         frac >>= (-exp - 1);
//         exp = 0;
//     } else {
//         frac |= 0x400;
//     }

//     return (float16){
//         .bits = (sign << 15) | (exp << 10) | frac
//     };
// }

// int main() {
//     // 计算 (1/81)
//     float value = 0.9f;

//     // 创建 scalar_t 实例
//     scalar_t scalar;

//     // 使用 float 存储 (1/81)
//     scalar.f32 = value;
//     printf("f32: %f\n", scalar.f32);

//     // 使用 float16 存储 (1/81)
//     scalar.f16 = float_to_float16(value);
//     printf("f16: %u\n", scalar.f16.bits); // 打印浮点数的二进制表示

//     return 0;
// }