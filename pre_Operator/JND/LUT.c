// #include <cfloat>
// #include <algorithm>
#include <math.h>
#include <stdio.h>

// #include "TEncPreanalyzer.h"
typedef signed char __int8_t;
typedef unsigned char uint8_t;
typedef signed short int int16_t;
typedef unsigned short int uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;

#define LUT_BITS   10
#define SHIFT_BITS 6
#define FIXED_SCALE 256
#define LUT_SIZE (1 << LUT_BITS)
uint16_t sqrt_lut[LUT_SIZE];

void InitSqrtLUT() {
    for (int i = 0; i < LUT_SIZE; i++) {
        float S = (i << SHIFT_BITS) + 0.5f;
        float val = sqrtf(S);
        sqrt_lut[i] = (uint16_t)(val * FIXED_SCALE);
    }
}

uint8_t CalculateLc(int16_t Gh, int16_t Gv) {
    uint32_t S = (Gh * Gh + Gv * Gv) / 2;

    uint32_t index = S >> SHIFT_BITS;
    uint32_t frac = S & ((1 << SHIFT_BITS) - 1);

    if (index >= (LUT_SIZE - 1)) {
        return sqrt_lut[LUT_SIZE - 1] >> 8;
    }

    uint32_t val_low = sqrt_lut[index];
    uint32_t val_high = sqrt_lut[index + 1];
    uint32_t result = (val_low * ((1 << SHIFT_BITS) - frac) + val_high * frac) >> SHIFT_BITS;

    return (result + FIXED_SCALE / 2) >> 8;
}

int main() {
    InitSqrtLUT();
    printf("%d\n", LUT_SIZE);
    for (int i = 0; i < LUT_SIZE; i++) {
        printf("%d, ", sqrt_lut[i]);
        if ((i+1)%16==0) {
            printf("\n");
        }
    }
    printf("\n");
    printf("sqrt_lut[LUT_SIZE - 1] >> 8 = %d")
}