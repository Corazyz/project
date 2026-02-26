// gaussian_no_padding.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

// 辅助宏
#define MAX_DIM 5
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


static int get_gaussian_sep_kernel(int n, float sigma, float *k_sep) {
    const int SMALL_GAUSSIAN_SIZE = 3;
    static const float small_gaussian_tab[3] = {0.25f, 0.5f, 0.25f};
    const float* fixed_kernel = n % 2 == 1 && n <= SMALL_GAUSSIAN_SIZE && sigma <= 0 ? small_gaussian_tab : 0;
    float sigmaX = sigma > 0 ? sigma : ((n - 1) * 0.5 - 1) * 0.3 + 0.8;
    float scale2X = -0.5 / (sigmaX * sigmaX);
    float sum = 0;
    int i;

    for (i = 0; i < n; i++) {
        float x = i - (n - 1) * 0.5;
        float t = fixed_kernel ? fixed_kernel[i] : exp(scale2X * x * x);
        k_sep[i] = t;
        sum += k_sep[i];
    }
    sum = 1./sum;
    for (i = 0; i < n; i++) {
        k_sep[i] = k_sep[i] * sum;
    }
    return 0;
}

static void create_gaussian_kernel(float* kernel, int kw, int kh, float sigma1, float sigma2) {
    float* k_sep_x = (float* )malloc(sizeof(float) * kw);
    float* k_sep_y = (float* )malloc(sizeof(float) * kh);

    if(sigma2 <= 0) sigma2 = sigma1;
    // automatic detection of kernel size from sigma
    if (kw <= 0 && sigma1 > 0 ) kw = (int)round(sigma1 * 3 * 2 + 1) | 1;
    if (kh <= 0 && sigma2 > 0 ) kh = (int)round(sigma2 * 3 * 2 + 1) | 1;
    sigma1 = sigma1 < 0 ? 0 : sigma1;
    sigma2 = sigma2 < 0 ? 0 : sigma2;
    get_gaussian_sep_kernel(kw, sigma1, k_sep_x);
    if (kh == kw && abs(sigma1 - sigma2) < DBL_EPSILON) {
        get_gaussian_sep_kernel(kw, sigma1, k_sep_y);
    } else {
        get_gaussian_sep_kernel(kh, sigma2, k_sep_y);
    }
    for (int i = 0; i < kh; i++) {
        for (int j = 0; j < kw; j++) {
            kernel[i * kw + j] = k_sep_y[i] * k_sep_x[j];
        }
    }
    free(k_sep_x);
    free(k_sep_y);
}


void gaussian_filter(double* src, double* dst, int width, int height, float sigma) {
    int r = (int)(3.5f * sigma + 0.5f);
    int win_size = 2 * r + 1;
    printf("sigma = %.2f, r = %d, win_size = %d\n", sigma, r, win_size);
    float *kernel = (float*)malloc(win_size * win_size * sizeof(float));
    float sum = 0.0f;

    // for (int i = 0; i < win_size; i++) {
    //     for (int j = 0; j < win_size; j++) {
    //         int x = i - r;
    //         int y = j - r;
    //         float val = expf(-(x*x + y*y) / (2.0f * sigma * sigma));
    //         kernel[i * win_size + j] = val;
    //         sum += val;
    //     }
    // }
    // for (int i = 0; i < win_size * win_size; i++) {
    //     kernel[i] /= sum;
    //     printf("%.6f, ", kernel[i]);
    //     if ((i+1) % win_size == 0){
    //         printf("\n");
    //     }
    // }

    // float* tpu_kernel = (float*)malloc(sizeof(float) * win_size * win_size);
    create_gaussian_kernel(kernel, win_size, win_size, sigma, sigma);
    // for (int i = 0; i < win_size * win_size; i++) {
        // printf("%.6f, ", kernel[i]);
        // if ((i+1) % win_size == 0){
        //     printf("\n");
    //     }
    // }

    // 卷积（反射边界）
    for (int y = 0; y < height-win_size+1; y++) {
        for (int x = 0; x < width-win_size+1; x++) {
            float val = 0.0f;
            for (int ky = 0; ky < win_size; ky++) {
                for (int kx = 0; kx < win_size; kx++) {
                    val += src[(y + ky) * width + (x + kx)] * kernel[ky * win_size + kx];
                }
            }
            dst[(y+(win_size-1)/2) * width + (x+(win_size-1)/2)] = val;
            // printf("%.1f, ", val);
        }
        // printf("\n");
    }
    free(kernel);
}

// 读取文件
int read_image(const char* filename, double* image, int rows, int cols) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("错误：无法打开文件 %s\n", filename);
        return -1;
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (fscanf(f, "%lf", &image[i * cols + j]) != 1) {
                printf("错误：读取第 %d 行第 %d 列失败\n", i+1, j+1);
                fclose(f);
                return -1;
            }
        }
    }

    fclose(f);
    return 0;
}


// 写入文件
int write_image(const char* filename, double* image, int rows, int cols) {
    FILE* f = fopen(filename, "w");
    if (!f) return -1;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(f, "%.2f ", image[i * cols + j]);
        }
        fprintf(f, "\n");
    }
    fclose(f);
    return 0;
}

int main() {
    int shape[2] = {64, 64};
    double image[4096];
    double output[4096];
    double sigma[2] = {1.5, 1.5};
    double truncate = 3.5;

    // 读取图像
    if (read_image("image.txt", image, 64, 64) != 0) {
        printf("读取 image.txt 失败\n");
        return 1;
    }

    // 高斯滤波
    gaussian_filter(image, output, 64, 64, 1.5);

    // 输出结果
    write_image("output_c.txt", output, 64, 64);

    printf("C 代码滤波结果已保存到 output_c.txt\n");
    return 0;
}
