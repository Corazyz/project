#include <stdio.h>
#include <math.h>

unsigned char fp32_to_u8(float fp32) {
    // 通过指针将浮点数的二进制表示解析为整数表示
    union {
        float f;
        unsigned int i;
    } u;
    u.f = fp32;

    // 提取符号、指数和尾数
    unsigned int sign = (u.i >> 31) & 0x1;       // 符号位
    unsigned int exponent = (u.i >> 23) & 0xFF; // 指数部分
    unsigned int fraction = u.i & 0x7FFFFF;     // 尾数部分（小数部分）

    // 如果指数为0，说明是非规格化数或0，直接映射为0
    if (exponent == 0) {
        return 0;
    }

    // 计算浮点数的真实值
    float value = pow(-1, sign) * pow(2, exponent - 127) * (1 + fraction / pow(2, 23));

    // 映射到u8范围（0-255）
    if (value < 0) value = 0;               // 限制最小值
    if (value > 255) value = 255;           // 限制最大值

    return (unsigned char)(value + 0.5f);         // 四舍五入并返回
}

int main() {
    float a = 1.5;
    unsigned char b = fp32_to_u8(a);
    printf("b = %d\n", b);
    // unsigned int argb_pixel = 0x1234;   // A = 0x1, R = 0x2, G = 0x3, B = 0x4
    // unsigned int a = (argb_pixel >> 12) & 0x0F;
    // unsigned int r = (argb_pixel >> 8) & 0x0F;
    // unsigned int g = (argb_pixel >> 4) & 0x0F;
    // unsigned int b = argb_pixel & 0x0F;

    // printf("a = 0x%X, r = 0x%X, g = 0x%X, b = 0x%X\n", a, r, g, b);
    // a = (a << 4) | a;
    // r = (r << 4) | r;
    // g = (g << 4) | g;
    // b = (b << 4) | b;
    // printf("a = 0x%X, r = 0x%X, g = 0x%X, b = 0x%X\n", a, r, g, b);

    // unsigned int rgba = (a << 24) | (b << 16) | (g << 8) | r;

    // printf("a = 0x%X, r = 0x%X, g = 0x%X, b = 0x%X\n", a, r, g, b);
    // printf("ARGB: 0x%X\n", argb_pixel);
    // printf("RGBA: 0x%X\n", rgba);

    // char *c = (char*)&rgba;
    // printf("value of start addr of rgba: 0x%X\n", *c);
}