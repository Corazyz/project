#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <float.h>

typedef struct {
    int width;
    int height;
    int channels;
    float* data;
} Image;

Image* create_image(int width, int height, int channels) {
    Image *img = (Image*)malloc(sizeof(Image));
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->data = (float*)calloc(width * height * channels, sizeof(float));
    return img;
}

void free_image(Image *img) {
    if (img) {
        free(img->data);
        free(img);
    }
}

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

// 高斯滤波（反射边界）
void gaussian_filter(float* src, float* dst, int width, int height, float sigma) {
    int r = (int)(3.5f * sigma + 0.5f);
    int win_size = 2 * r + 1;
    float *kernel = (float*)malloc(win_size * win_size * sizeof(float));
    float sum = 0.0f;
    int pad = win_size / 2;

    // for (int i = 0; i < win_size; i++) {
    //     for (int j = 0; j < win_size; j++) {
    //         int x = i - r;
    //         int y = j - r;
    //         float val = expf(-(x*x + y*y) / (2.0f * sigma * sigma));
    //         kernel[i * win_size + j] = val;
    //         sum += val;
    //     }
    // }

    // // 归一化
    // for (int i = 0; i < win_size * win_size; i++) {
    //     kernel[i] /= sum;
    // }
    create_gaussian_kernel(kernel, win_size, win_size, sigma, sigma);

    // 卷积（反射边界）
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float val = 0.0f;
            for (int ky = 0; ky < win_size; ky++) {
                for (int kx = 0; kx < win_size; kx++) {
                    int iy = y + ky - r;
                    int ix = x + kx - r;

                    // 反射边界
                    if (iy < 0) iy = -iy;
                    if (iy >= height) iy = 2 * height - 1 - iy;
                    if (ix < 0) ix = -ix;
                    if (ix >= width) ix = 2 * width - 1 - ix;

                    val += src[iy * width + ix] * kernel[ky * win_size + kx];
                }
            }
            dst[y * width + x] = val;
        }
    }
    free(kernel);
}

// 均值滤波（反射边界）
void uniform_filter(float* src, float* dst, int width, int height, int win_size) {
    int pad = win_size / 2;
    float norm = 1.0f / (win_size * win_size);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float sum = 0.0f;
            for (int dy = -pad; dy <= pad; dy++) {
                for (int dx = -pad; dx <= pad; dx++) {
                    int iy = y + dy;
                    int ix = x + dx;
                    // 反射边界
                    if (iy < 0) iy = -iy;
                    if (iy >= height) iy = 2 * height - 1 - iy;
                    if (ix < 0) ix = -ix;
                    if (ix >= width) ix = 2 * width - 1 - ix;
                    sum += src[iy * width + ix];
                }
            }
            dst[y * width + x] = sum * norm;
        }
    }
}

// 计算单通道 SSIM
float structural_similarity_channel(
    float* im1, float* im2,
    int width, int height,
    int win_size,
    int gaussian_weights,
    float data_range,
    int full,
    float* ssim_map
) {
    if (win_size % 2 == 0) {
        fprintf(stderr, "Error: win_size must be odd.\n");
        return -1.0f;
    }

    float K1 = 0.01f;
    float K2 = 0.03f;
    float sigma = 1.5f;  // 默认高斯 sigma

    // 分配临时缓冲区
    float* ux = (float*)calloc(width * height, sizeof(float));
    float* uy = (float*)calloc(width * height, sizeof(float));
    float* uxx = (float*)calloc(width * height, sizeof(float));
    float* uyy = (float*)calloc(width * height, sizeof(float));
    float* uxy = (float*)calloc(width * height, sizeof(float));
    float* vx = (float*)calloc(width * height, sizeof(float));
    float* vy = (float*)calloc(width * height, sizeof(float));
    float* vxy = (float*)calloc(width * height, sizeof(float));
    // float* S = (float*)calloc(width * height, sizeof(float));
    // ssim_map = (float*)calloc(width * height, sizeof(float));

    // 计算均值
    if (gaussian_weights) {
        int r = (int)(3.5f * sigma + 0.5f);
        win_size = 2 * r + 1;
        gaussian_filter(im1, ux, width, height, sigma);
        gaussian_filter(im2, uy, width, height, sigma);

        float* im1_sq = (float*)malloc(width * height * sizeof(float));
        for (int i = 0; i < width * height; i++) im1_sq[i] = im1[i] * im1[i];
        gaussian_filter(im1_sq, uxx, width, height, sigma);
        free(im1_sq);

        float* im2_sq = (float*)malloc(width * height * sizeof(float));
        for (int i = 0; i < width * height; i++) im2_sq[i] = im2[i] * im2[i];
        gaussian_filter(im2_sq, uyy, width, height, sigma);
        free(im2_sq);

        float* im1_im2 = (float*)malloc(width * height * sizeof(float));
        for (int i = 0; i < width * height; i++) im1_im2[i] = im1[i] * im2[i];
        gaussian_filter(im1_im2, uxy, width, height, sigma);
        free(im1_im2);
    } else {
        uniform_filter(im1, ux, width, height, win_size);
        uniform_filter(im2, uy, width, height, win_size);

        float* im1_sq = (float*)malloc(width * height * sizeof(float));
        for (int i = 0; i < width * height; i++) im1_sq[i] = im1[i] * im1[i];
        uniform_filter(im1_sq, uxx, width, height, win_size);
        free(im1_sq);

        float* im2_sq = (float*)malloc(width * height * sizeof(float));
        for (int i = 0; i < width * height; i++) im2_sq[i] = im2[i] * im2[i];
        uniform_filter(im2_sq, uyy, width, height, win_size);
        free(im2_sq);

        float* im1_im2 = (float*)malloc(width * height * sizeof(float));
        for (int i = 0; i < width * height; i++) im1_im2[i] = im1[i] * im2[i];
        uniform_filter(im1_im2, uxy, width, height, win_size);
        free(im1_im2);
    }

    // 计算方差与协方差
    printf("win_size = %d\n", win_size);
    int np = win_size * win_size;
    float cov_norm = np / (np - 1.0f);  // 无偏估计
    // float cov_norm = 1.0f;                 // 有偏估计

    for (int i = 0; i < width * height; i++) {
        vx[i] = cov_norm * (uxx[i] - ux[i] * ux[i]);
        vy[i] = cov_norm * (uyy[i] - uy[i] * uy[i]);
        vxy[i] = cov_norm * (uxy[i] - ux[i] * uy[i]);
    }

    // 计算 SSIM
    float C1 = (K1 * data_range) * (K1 * data_range);
    float C2 = (K2 * data_range) * (K2 * data_range);

    for (int i = 0; i < width * height; i++) {
        float A1 = 2.0f * ux[i] * uy[i] + C1;
        float A2 = 2.0f * vxy[i] + C2;
        float B1 = ux[i] * ux[i] + uy[i] * uy[i] + C1;
        float B2 = vx[i] + vy[i] + C2;
        float D = B1 * B2;

        if (D == 0.0f) {
            ssim_map[i] = 1.0f;
        } else {
            ssim_map[i] = (A1 * A2) / D;
        }
    }

    // 计算平均 SSIM（忽略边缘）
    int pad = (win_size - 1) / 2;
    double mssim = 0.0f;
    int count = 0;
    printf("width = %d, height = %d\n", width, height);
    float *pad_S = (float*)malloc((width-2*pad) * (height-2*pad) * sizeof(float));
    for (int y = pad; y < height - pad; y++) {
        for (int x = pad; x < width - pad; x++) {
            mssim += ssim_map[y * width + x];
            pad_S[(y - pad) * (width - 2*pad) + (x - pad)] = ssim_map[y * width + x];
            count++;
        }
    }
    mssim /= count;

    FILE* fp = fopen("mssim_c.txt", "w");
    if (fp == NULL) {
        fprintf(stderr, "Error: Cannot open file 'mssim_c.txt' for writing.\n");
    } else {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;
                fprintf(fp, "%.2f ", ssim_map[idx]);
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
        printf("mssim values saved to 'mssim_c.txt'\n");
    }

    int new_count = 0;
    float cmp_mssim = 0.0f;
    fp = fopen("pad_S_c.txt", "w");
    if (fp == NULL) {
        fprintf(stderr, "Error: Cannot open file 'ux_values.txt' for writing.\n");
    } else {
        for (int y = 0; y < height-2*pad; y++) {
            for (int x = 0; x < width-2*pad; x++) {
                int idx = y * (width-2*pad) + x;
                cmp_mssim += pad_S[idx];
                new_count++;
                fprintf(fp, "%.2f ", pad_S[idx]);
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
        printf("pad_S_c values saved to 'pad_S_c.txt'\n");
    }
    printf("mssim = %.6f\n", cmp_mssim);
    cmp_mssim /= new_count;
    printf("cmp_mssim = %.6f, count = %d\n", cmp_mssim, new_count);
    // float mssim = 0.0f;
    // int count = width * height;
    // for (int i = 0; i < width * height; i++) {
    //     mssim += S[i];
    // }
    // mssim /= count;

    // 输出完整 SSIM 图


    free(ux); free(uy); free(uxx); free(uyy); free(uxy);
    free(vx); free(vy); free(vxy);

    return mssim;
}

// 多通道 SSIM（RGB）
float structural_similarity_multi(
    Image* im1, Image* im2,
    int win_size,
    int gaussian_weights,
    float data_range,
    int full,
    float* ssim_map
) {
    if (im1->width != im2->width || im1->height != im2->height || im1->channels != im2->channels) {
        fprintf(stderr, "Error: Images must have same dimensions and channels.\n");
        return -1.0f;
    }

    int width = im1->width;
    int height = im1->height;
    int channels = im1->channels;

    float total_ssim = 0.0f;

    for (int c = 0; c < channels; c++) {
        float* ch1 = im1->data + c;
        float* ch2 = im2->data + c;

        // 为每个通道创建临时数组（步长为 channels）
        float* tmp1 = (float*)malloc(width * height * sizeof(float));
        float* tmp2 = (float*)malloc(width * height * sizeof(float));

        for (int i = 0; i < width * height; i++) {
            tmp1[i] = ch1[i * channels];
            tmp2[i] = ch2[i * channels];
        }
        int is_first = (c == 0 && full && ssim_map);
        float *local_ssim_map = NULL;

        float ssim_ch = structural_similarity_channel(
            tmp1, tmp2,
            width, height,
            win_size,
            gaussian_weights,
            data_range,
            is_first ? 1 : 0,  // 不输出完整图（多通道时通常不输出）
            is_first ? ssim_map : NULL
        );

        total_ssim += ssim_ch;

        free(tmp1);
        free(tmp2);
    }

    return total_ssim / channels;
}

int read_image(const char* filename, float* image, int rows, int cols) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("错误：无法打开文件 %s\n", filename);
        return -1;
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (fscanf(f, "%f", &image[i * cols + j]) != 1) {
                printf("错误：读取第 %d 行第 %d 列失败\n", i+1, j+1);
                fclose(f);
                return -1;
            }
        }
    }

    fclose(f);
    return 0;
}

static int read_bin(const char *input_path, unsigned char *input_data, int width, int height, int channel) {
    FILE *fp_src = fopen(input_path, "rb");
    if (fp_src == NULL)
    {
        printf("Can not open file! %s\n", input_path);
        return -1;
    }
    if(fread(input_data, sizeof(unsigned char), width * height * channel, fp_src) != 0)
        printf("read image success\n");
    fclose(fp_src);
    return 0;
}

int main() {
    int width1, height1, channel1;
    int width2, height2, channel2;

    /* decoded img */
    // // 读取灰度图
    // unsigned char* img1_rgb = stbi_load("10350+.jpg", &width1, &height1, &channel1, 3);
    // unsigned char* img2_rgb = stbi_load("background.jpg", &width2, &height2, &channel2, 3);

    // if (!img1_rgb || !img2_rgb) {
    //     fprintf(stderr, "Error: Failed to load images.\n");
    //     return -1;
    // }

    // if (width1 != width2 || height1 != height2) {
    //     fprintf(stderr, "Error: Images must have same dimensions.\n");
    //     stbi_image_free(img1_rgb);
    //     stbi_image_free(img2_rgb);
    //     return -1;
    // }

    // unsigned char *img1_data = (unsigned char*)malloc(width1 * height1 * sizeof(unsigned char));
    // unsigned char *img2_data = (unsigned char*)malloc(width2 * height2 * sizeof(unsigned char));

    // for (int i = 0; i < width1 * height1; i++) {
    //     unsigned char r1 = img1_rgb[i * 3 + 0];
    //     unsigned char g1 = img1_rgb[i * 3 + 1];
    //     unsigned char b1 = img1_rgb[i * 3 + 2];

    //     unsigned char r2 = img2_rgb[i * 3 + 0];
    //     unsigned char g2 = img2_rgb[i * 3 + 1];
    //     unsigned char b2 = img2_rgb[i * 3 + 2];

    //     float gray1 = 0.299f * r1 + 0.587f * g1 + 0.114f * b1;
    //     img1_data[i] = (unsigned char)(gray1 + 0.5f);

    //     float gray2 = 0.299f * r2 + 0.587f * g2 + 0.114f * b2;
    //     img2_data[i] = (unsigned char)(gray2 + 0.5f);
    // }


    // // 创建单通道图像
    // Image *im1 = create_image(width1, height1, 1);
    // Image *im2 = create_image(width2, height2, 1);

    // // 转换为 float
    // for (int i = 0; i < width1 * height1; i++) {
    //     im1->data[i] = img1_data[i];
    //     im2->data[i] = img2_data[i];
    // }



    /* customized data */
    width1 = 3840;
    height1 = 2160;

    Image *im1 = create_image(width1, height1, 1);
    Image *im2 = create_image(width1, height1, 1);

    if (read_image("gray1_py.txt", im1->data, width1, height1) != 0) {
        printf("读取 image.txt 失败\n");
        return 1;
    }

    if (read_image("gray2_py.txt", im2->data, width1, height1) != 0) {
        printf("读取 image.txt 失败\n");
        return 1;
    }




    // /* raw img data */
    // width1 = 1920;
    // height1 = 1080;
    // Image *im1 = create_image(width1, height1, 1);
    // Image *im2 = create_image(width1, height1, 1);
    // unsigned char *img1_data = (unsigned char*)malloc(width1 * height1 * sizeof(unsigned char));
    // unsigned char *img2_data = (unsigned char*)malloc(width1 * height1 * sizeof(unsigned char));
    // if (read_bin("gray.bin", img1_data, width1, height1, 1) != 0) {
    //     printf("读取 gray.bin 失败\n");
    //     return 1;
    // }
    // if (read_bin("gray_out.bin", img2_data, width1, height1, 1) != 0) {
    //     printf("读取 gray.bin 失败\n");
    //     return 1;
    // }
    // // 转换为 float
    // for (int i = 0; i < width1 * height1; i++) {
    //     im1->data[i] = img1_data[i];
    //     im2->data[i] = img2_data[i];
    // }




    /* save input data */
    // FILE* fp = fopen("gray1_c.txt", "w");
    // if (fp == NULL) {
    //     fprintf(stderr, "Error: Cannot open file 'ux_values.txt' for writing.\n");
    // } else {
    //     for (int y = 0; y < height1; y++) {
    //         for (int x = 0; x < width1; x++) {
    //             int idx = y * width1 + x;
    //             fprintf(fp, "%.2f ", im1->data[idx]);
    //         }
    //         fprintf(fp, "\n");
    //     }
    //     fclose(fp);
    //     printf("gray1 values saved to 'gray1_c.txt'\n");
    // }

    // fp = fopen("gray2_c.txt", "w");
    // if (fp == NULL) {
    //     fprintf(stderr, "Error: Cannot open file 'ux_values.txt' for writing.\n");
    // } else {
    //     for (int y = 0; y < height1; y++) {
    //         for (int x = 0; x < width1; x++) {
    //             int idx = y * width1 + x;
    //             fprintf(fp, "%.2f ", im2->data[idx]);
    //         }
    //         fprintf(fp, "\n");
    //     }
    //     fclose(fp);
    //     printf("gray2 values saved to 'gray2_c.txt'\n");
    // }

    // 计算 SSIM
    float *ssim_map = (float*)calloc(width1 * height1, sizeof(float));

    double ssim_value = structural_similarity_multi(
        im1, im2,
        7, 1, 255.0f, 1, ssim_map
    );

    printf("Mean SSIM (RGB): %.6lf\n", ssim_value);

    unsigned char* ssim_img = (unsigned char*)malloc(width1 * height1 * sizeof(unsigned char));
    for (int i = 0; i < width1 * height1; i++) {
        float val = ssim_map[i];
        // 限制在 [0,1]
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;
        ssim_img[i] = (unsigned char)(val * 255.0f);
    }

    // 保存 SSIM 图为 PNG（使用 stb_image_write）
    // 保存为 PNG
    int success = stbi_write_png("ssim_map.png", width1, height1, 1, ssim_img, width1);
    if (success) {
        printf("SSIM map saved as 'ssim_map.png'\n");
    } else {
        fprintf(stderr, "Error: Failed to save SSIM map.\n");
    }

    // 释放资源
    free_image(im1);
    free_image(im2);
    // stbi_image_free(img1_data);
    // stbi_image_free(img2_data);
    free(ssim_map);
    free(ssim_img);

    return 0;
}
