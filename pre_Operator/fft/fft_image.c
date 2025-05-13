#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define M_PI 3.14159265358979323846

// 读取二进制文件
static void readBin(const char* path, unsigned char* input_data, int size)
{
    FILE *fp_src = fopen(path, "rb");
    if (!fp_src) {
        printf("Failed to open file: %s\n", path);
        exit(1);
    }
    if (fread((void *)input_data, 1, size, fp_src) < (unsigned int)size) {
        printf("File size is less than %d required bytes\n", size);
    }
    fclose(fp_src);
}

// 写入二进制文件
static void writeBin(const char * path, unsigned char* input_data, int size)
{
    FILE *fp_dst = fopen(path, "wb");
    if (!fp_dst) {
        printf("Failed to open file: %s\n", path);
        exit(1);
    }
    if (fwrite((void *)input_data, 1, size, fp_dst) < (unsigned int)size){
        printf("File size is less than %d required bytes\n", size);
    }
    fclose(fp_dst);
}

// 离散傅里叶变换 (DFT)
void dft(float* in_real, float* in_imag, float* output_real,
                float* output_imag, int length, bool forward)
{
    int i, j;
    double angle;

    for (i = 0; i < length; ++i) {
        output_real[i] = 0.f;
        output_imag[i] = 0.f;
        for (j = 0; j < length; ++j) {
            angle = (forward ? -2.0 : 2.0) * M_PI * i * j / length;
            output_real[i] += in_real[j] * cos(angle) - in_imag[j] * sin(angle);
            output_imag[i] += in_real[j] * sin(angle) + in_imag[j] * cos(angle);
        }
    }
}

// 归一化 FFT 结果
void normalize_fft(float* res_XR, float* res_XI, int frm_len, int num)
{
    float norm_fac;
    int i;

    norm_fac = 1.0f / frm_len;

    for (i = 0; i < num; ++i) {
        res_XR[i] *= norm_fac;
        res_XI[i] *= norm_fac;
    }
}

// 2D FFT
int cpu_fft_2d(float *in_real, float *in_imag, float *out_real,
                        float *out_imag, int M, int N, bool forward)
{
    int i, j;

    // 对每一行进行 DFT
    for (i = 0; i < M; ++i) {
        dft(&(in_real[i * N]), &(in_imag[i * N]), &(out_real[i * N]),
            &(out_imag[i * N]), N, forward);
    }
    if (!forward) {
        normalize_fft(out_real, out_imag, N, M * N);
    }

    // 转置矩阵
    float *in_out_real = (float*)malloc(M * N * sizeof(float));
    float *in_out_imag = (float*)malloc(M * N * sizeof(float));

    float *in_out_real2 = (float*)malloc(M * N * sizeof(float));
    float *in_out_imag2 = (float*)malloc(M * N * sizeof(float));

    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            in_out_real[j * M + i] = out_real[i * N + j];
            in_out_imag[j * M + i] = out_imag[i * N + j];
        }
    }

    // 对每一列进行 DFT
    for (i = 0; i < N; ++i) {
        dft(&(in_out_real[i * M]), &(in_out_imag[i * M]), &(in_out_real2[i * M]),
            &(in_out_imag2[i * M]), M, forward);
    }
    if (!forward) {
        normalize_fft(in_out_real2, in_out_imag2, M, M * N);
    }

    // 转置回原矩阵
    for (j = 0; j < N; j++) {
        for (i = 0; i < M; i++) {
            out_real[i * N + j] = in_out_real2[j * M + i];
            out_imag[i * N + j] = in_out_imag2[j * M + i];
        }
    }

    free(in_out_real);
    free(in_out_imag);
    free(in_out_real2);
    free(in_out_imag2);
    return 0;
}

// 主程序
int main(int argc, char* argv[])
{
    int width = 1920;
    int height = 1080;
    char* input_path = "/home/zyz/projects/test_project/pre_Operator/fft/1920x1080_gray.bin";
    char* output_path = "/home/zyz/projects/test_project/pre_Operator/fft/frequency.bin";
    // if (argc != 5) {
    //     printf("Usage: %s <input.raw> <output.raw> <width> <height>\n", argv[0]);
    //     return -1;
    // }
    if (argc > 1) input_path = argv[1];
    if (argc > 2) output_path = argv[2];
    if (argc > 3) width = atoi(argv[3]);
    if (argc > 4) height = atoi(argv[4]);

    int size = width * height;

    // 分配内存
    unsigned char* input_data = (unsigned char*)malloc(size * sizeof(unsigned char));
    float* real = (float*)malloc(size * sizeof(float));
    float* imag = (float*)malloc(size * sizeof(float));
    float* out_real = (float*)malloc(size * sizeof(float));
    float* out_imag = (float*)malloc(size * sizeof(float));
    unsigned char* output_data = (unsigned char*)malloc(size * sizeof(unsigned char));

    // 读取输入图像
    readBin(input_path, input_data, size);

    // 初始化实部和虚部
    for (int i = 0; i < size; ++i) {
        real[i] = (float)input_data[i];
        imag[i] = 0.0f;
    }

    // 执行 2D FFT
    cpu_fft_2d(real, imag, out_real, out_imag, height, width, true);

    // cpu_fft_2d(out_real, out_imag, real, imag, height, width, false);
    // 将频域结果转换为可视化图像
    // for (int i = 0; i < size; ++i) {
    //     output_data[i] = (unsigned char)(log(1.0f + sqrt(out_real[i] * out_real[i] + out_imag[i] * out_imag[i])) * 10);
    //     if (output_data[i] > 255) output_data[i] = 255; // 防止溢出
    // }
    for (int i = 0; i < size; ++i) {
        output_data[i] = (unsigned char)(real[i] + 0.5f); // 四舍五入
        if (output_data[i] > 255) output_data[i] = 255; // 防止溢出
        if (output_data[i] < 0) output_data[i] = 0;     // 防止负值
    }

    // 写入输出图像
    writeBin(output_path, output_data, size);

    // 释放内存
    free(input_data);
    free(real);
    free(imag);
    free(out_real);
    free(out_imag);
    free(output_data);

    printf("Frequency domain image saved to %s\n", output_path);

    return 0;
}
