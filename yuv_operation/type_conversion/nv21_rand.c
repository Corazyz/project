#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static void fillNV21(unsigned char* nv21Buffer, int width, int height) {
    // 填充Y分量
    for (int j = 0; j < height; j++) {
        for (int k = 0; k < width; k++) {
            unsigned char yValue = rand() % 256;
            nv21Buffer[j * width + k] = yValue;
        }
    }
    // printf("Aborted error\n");
    // 填充VU分量
    int vuIndex = width * height; // VU分量开始的索引
    // printf("width = %d, height = %d\n", width, height);
    for (int j = 0; j < height / 2; j++) {
        for (int k = 0; k < width / 2; k++) {
            unsigned char uValue = 1;
            unsigned char vValue = 0;

            // 由于VU是交错存储，所以每次跳过两个字节来存储
            nv21Buffer[vuIndex++] = vValue;
            nv21Buffer[vuIndex++] = uValue;
            // printf("vuIndex = %d\n", vuIndex);
        }
    }
}

int main() {
    unsigned char* nv21_0 = (unsigned char*)malloc(150*sizeof(unsigned char));
    fillNV21(nv21_0, 10, 10);
    for (int i = 0; i < 200; i++) {
        printf("%d, ", nv21_0[i]);
        if ((i+1) % 10 == 0) {
            printf("\n");
        }
        if ((i+1) % 100 == 0) {
            printf("\n");
        }
    }
    free(nv21_0);
    return 0;
}
