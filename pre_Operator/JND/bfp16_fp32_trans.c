#include "stdio.h"
#include "string.h"
#include "math.h"

#define M_PI 3.14159265358979323846



static short fp32_to_bfp16(float value) {
    unsigned int bits;
    memcpy(&bits, &value, sizeof(value));
    return (bits >> 16) & 0xFFFF;
}

float bfp16_to_fp32(short bfp16_value) {
    unsigned int fp32_bits = ((unsigned int)bfp16_value) << 16;
    float result;
    memcpy(&result, &fp32_bits, sizeof(result));
    return result;
}

int main() {
    short a[8] = {0x42fd, 0x4309, 0x4314, 0x430c, 0x42f2, 0x4308, 0x430a, 0x4309};
    short c[16] = {0x3883, 0x38b0, 0x3824, 0x39f1, 0x3a87, 0x3aa4, 0x3a9e, 0x3aec, 0x3dcd, 0x3e1e, 0x3e56, 0x3f3c, 0x4292, 0x4311, 0x430e, 0x4368};
    short m[4] = {0x4051, 0x4073, 0x4077, 0x4084};
                            // 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 5,
                            // 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 5, 5,
                            // 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                            // 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                            // 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                            // 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                            // 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                            // 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                            // 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                            // 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                            // 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                            // 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                            // 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                            // 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                            // 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                            // 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
    float b[8];
    float d[16];
    float n[4];
    printf("angle_rad(0.052734) = %f\n", asinf(0.052734));
    for (int i = 0; i < 8; i++) {
        b[i] = bfp16_to_fp32(a[i]);
        // printf("%f, %f, %f", b[i], b[i] / 32, d[i]);
        printf("%.6f, ", b[i]);
    }
    printf("\n");
    for (int i = 0; i < 16; i++) {
        d[i] = bfp16_to_fp32(c[i]);
        // printf("%f, %f, %f", b[i], b[i] / 32, d[i]);
        printf("%.1f, ", d[i]);
    }
    printf("\n");
    for (int i = 0; i < 4; i++) {
        n[i] = bfp16_to_fp32(m[i]);
        // printf("%f, %f, %f", b[i], b[i] / 32, d[i]);
        printf("%f, ", n[i]);
    }
    printf("\n");
    // 180/pi
    short pi;
    pi = fp32_to_bfp16((180.f / M_PI));
    short b1;
    b1 = fp32_to_bfp16(1.0);
    printf("b1 = %x\n", b1);
    printf("pi = %x\n", pi);

    // 180 deg, 360 deg
    short d180 = 0x4334;
    float d_180 = bfp16_to_fp32(d180);
    printf("d_180 = %f\n", d_180);
    short d360 = 0x43B4;
    float d_360 = bfp16_to_fp32(d360);
    printf("d_360 = %f\n", d_360);

    // orient param
    float orient_pram = 0.4999;
    short bf_opram = fp32_to_bfp16(orient_pram);
    printf("bf_oparm = %x\n", bf_opram);

    // atan
    float Gv = -13.5;
    float Gh = 22.7;
    float atan_res = atan(Gv / Gh);
    printf("atan(Gv/Gh) = %f\n", atan_res);

    // random bfp number
    short num = 0x3a83;
    float fp_num = bfp16_to_fp32(num);
    printf("num = %f\n", fp_num);

    // random fp number
    // float f = log(exp(1));
    float f = 0.001;
    short bf = fp32_to_bfp16(f);
    printf("bf = %x\n", bf);

    // floor
    float fl = 2.0;
    fl = floor(fl + 0.49999);
    printf("%f\n", fl);
    return 0;
}