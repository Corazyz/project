#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static int cpu_transpose(void* input, void* output, int channel, int height,
                        int width, int stride, int dtype)
{
    int i, j, k;

    if (input == NULL || output == NULL) {
        printf("the input or the output is null!\n");
        return -1;
    }
    if (dtype == 4) {
        float* input_tmp = (float*)input;
        float* output_tmp = (float*)output;

        for (i = 0; i < channel; ++i) {
            for (j = 0; j < width; ++j) {
                for (k = 0; k < height; ++k) {
                    output_tmp[i * height * width + j * height + k] = \
                                                    input_tmp[i * height * stride + k * stride + j];
                }
            }
        }
    } else if (dtype == 1) {
        unsigned char* input_tmp = (unsigned char*)input;
        unsigned char* output_tmp = (unsigned char*)output;

        for (i = 0; i < channel; ++i) {
            for (j = 0; j < width; ++j) {
                for (k = 0; k < height; ++k) {
                    output_tmp[i * height * width + j * height + k] = \
                                                    input_tmp[i * height * stride + k * stride + j];
                }
            }
        }
    }

    return 0;
}

void main() {
    int type = 1;
    int channel = 3;
    int height = 3;
    int width = 2;
    int num = channel * height * width;
    int ret = 0;
    unsigned char* input = (unsigned char*)malloc(channel * height * width * sizeof(unsigned char));
    unsigned char* output = (unsigned char*)malloc(channel * height * width * sizeof(unsigned char));
    for (int i = 0; i < num; i++) {
        input[i] = (unsigned char)(rand()%9);
        printf("%d, ", input[i]);
        if ((i+1) % width == 0) {
            printf("\n");
            if ((i+1) % (width * height) == 0){
                printf("\n");
            }
        }
    }

    printf("\n");
    for (int i = 0; i < num; i++) {
        printf("%d, ", input[i]);
    }
    printf("\n");

    memset(output, 0, num);
    ret = cpu_transpose(input, output, channel, height, width, width, type);
    for (int i = 0; i < num; i++) {
        printf("%d, ", output[i]);
        if ((i+1) % (height) == 0) {
            printf("\n");
            if ((i+1) % (width * height) == 0){
                printf("\n");
            }
        }
    }
    printf("\n");

    for (int i = 0; i < num; i++) {
        printf("%d, ", output[i]);
    }
    printf("\n");
}