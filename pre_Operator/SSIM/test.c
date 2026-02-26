// #include <stdio.h>
// #include <stdlib.h>
// #include <math.h>
// #include <string.h>
// #include <float.h>
// #include <time.h>

// typedef struct {
//     int width;
//     int height;
//     int channels;
//     double* data;
// } Image;


// int read_image(const char* filename, double* image, int rows, int cols) {
//     FILE* f = fopen(filename, "r");
//     if (!f) {
//         printf("错误：无法打开文件 %s\n", filename);
//         return -1;
//     }

//     int count = 0;
//     for (int i = 0; i < rows; i++) {
//         for (int j = 0; j < cols; j++) {
//             int ret = fscanf(f, "%lf", &image[i * cols + j]);
//             if (ret != 1) {
//                 printf("错误：读取第 %d 行第 %d 列失败 (ret=%d)\n", i+1, j+1, ret);
//                 fclose(f);
//                 return -1;
//             }
//             count++;
//         }
//     }

//     printf("成功读取 %d 个数值\n", count);  // 👈 添加这行
//     fclose(f);
//     return 0;
// }


// Image* create_image(int width, int height, int channels) {
//     Image *img = (Image*)malloc(sizeof(Image));
//     img->width = width;
//     img->height = height;
//     img->channels = channels;
//     img->data = (double*)calloc(width * height * channels, sizeof(double));
//     return img;
// }

// int main() {
//     int width = 3830;
//     int height = 2150;
//     Image *im = create_image(width, height, 1);

//     if (read_image("pad_S_c.txt", im->data, height, width) != 0) {
//         printf("读取 image.txt 失败\n");
//         return 1;
//     }

//     struct timespec start, end;
//     double total = 0.0f;
//     int count = 0;

//     clock_gettime(CLOCK_MONOTONIC, &start);  // 高精度计时开始

//     for (int y = 0; y < height; y++) {
//         for (int x = 0; x < width; x++) {
//             total += im->data[y * width + x];
//             count++;
//         }
//     }

//     clock_gettime(CLOCK_MONOTONIC, &end);  // 结束

//     double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
//     printf("计算耗时: %.9f 秒\n", elapsed);
//     printf("total = %.6lf\n", total);
//     total /= count;
//     printf("mean = %.6lf\n", total);
// }

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <time.h>
#include <stdint.h>

// int test_ptr(int *ptr) {
//     int a = 100;
//     *ptr = a;
//     return 0;
// }
void malloc_a_ptr(int **a) {
    (*a) = (int*)malloc(10 * sizeof(int));
}
void malloc_a(unsigned long *a) {
    // malloc_a_ptr(&a);
    int *b = (int*)malloc(10 * sizeof(int));
    a[0] = (unsigned long)((uintptr_t)b);
    for (int i = 0; i < 10; i++) {
        b[i] = i;
    }
}

int main() {
    // int value = 0;
    // int *ptr = &value;
    // int ret = test_ptr(ptr);
    // printf("*ptr = %d\n", *ptr);
    // int *ptr = NULL;
    // if (&ptr) {
    //     printf("ptr valid\n");
    // }

    // for (int i = 0; i < 10; i++) {
    //     for (int i = 0; i < 5; i++) {
    //         printf("inner i = %d\n", i);
    //     }
    //     printf("outer i = %d\n", i);
    // }
    // int a = 32;
    // printf("float a = %.1f\n", (float)a);
    // unsigned char b = 147;
    // unsigned char c = 215;
    // unsigned char d = 0;
    // d = b * c;
    // int e = b * c;
    // printf("u8 d = %u\n", d);
    // printf("int d = %d\n", d);
    // printf("int e = %d\n", e);
    // printf("\n");

    // printf("u8 b = %u\n", b);
    // printf("int b = %d\n", b);
    // printf("sizeof(unsigned int) = %ld\n", sizeof(unsigned int));
    // printf("float b = %.1f\n", b);
    unsigned long *a = (unsigned long*)malloc(10 * sizeof(unsigned long));
    malloc_a(a);
    int *b = NULL;
    b = (int*)(uintptr_t)a[0];
    printf("sizeof a = %ld\n", sizeof(a));
    for (int i = 0; i < 10; i++) {
        printf("%d, ", b[i]);
    }
    printf("\n");
    return 0;
}