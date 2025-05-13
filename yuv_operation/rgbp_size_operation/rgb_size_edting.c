#include <stdio.h>
#include <stdlib.h>

typedef enum image_format_ext_ {
    FORMAT_YUV420P,
    FORMAT_YUV422P,
    FORMAT_YUV444P,
    FORMAT_NV12,
    FORMAT_NV21,
    FORMAT_NV16,
    FORMAT_NV61,
    FORMAT_NV24,
    FORMAT_RGB_PLANAR,
    FORMAT_BGR_PLANAR,
    FORMAT_RGB_PACKED,
    FORMAT_BGR_PACKED,
    FORMAT_RGBP_SEPARATE,
    FORMAT_BGRP_SEPARATE,
    FORMAT_GRAY,
    FORMAT_COMPRESSED,
    FORMAT_HSV_PLANAR,
    FORMAT_ARGB_PACKED,
    FORMAT_ABGR_PACKED,
    FORMAT_YUV444_PACKED,
    FORMAT_YVU444_PACKED,
    FORMAT_YUV422_YUYV,
    FORMAT_YUV422_YVYU,
    FORMAT_YUV422_UYVY,
    FORMAT_YUV422_VYUY,
    FORMAT_RGBYP_PLANAR,
    FORMAT_HSV180_PACKED,
    FORMAT_HSV256_PACKED,
    FORMAT_BAYER,
    FORMAT_BAYER_RG8,
} image_format_ext;

static void readbin(const char *input_path, unsigned char *input_data, int width, int height, int channel) {
    FILE *fp_src = fopen(input_path, "rb");
    if (fp_src == NULL) {
        printf("Can not open input_file %s\n", input_path);
        return;
    }
    if (fread(input_data, sizeof(unsigned char), width * height * channel, fp_src) != 0)
        printf("read image success\n");
    fclose(fp_src);
}

static void writebin(const char *output_path, unsigned char *data, int byte_size) {
    FILE *fp_dst = fopen(output_path, "wb");
    if (!fp_dst) {
        perror("failed to open destination file\n");
        // free(data);
    }
    if (fwrite((void*)data, 1, byte_size, fp_dst) < (unsigned int)byte_size) {
        printf("Failed to write all required bytes\n");
    }
    fclose(fp_dst);
}

static void crop_img(unsigned char* input_data, unsigned char* crop_data, int input_width, int input_height, int crop_width, int crop_height, int format) {
    switch (format)
    {
    case FORMAT_RGB_PACKED:
        for (int i = 0; i < crop_height; i++) {
            for (int j = 0; j < crop_width; j++) {
                int crop_data_idx = (i * crop_width + j) * 3;
                int input_data_idx = (i * input_width + j) * 3;
                // 剪裁起始点：input_data_idx = (i * input_width + j + x) * 3 + y * input_width * 3;
                crop_data[crop_data_idx] = input_data[input_data_idx];
                crop_data[crop_data_idx + 1] = input_data[input_data_idx + 1];
                crop_data[crop_data_idx + 2] = input_data[input_data_idx + 2];
            }
        }
        break;
    case FORMAT_RGB_PLANAR:
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < crop_height; j++) {
                for (int k = 0; k < crop_width; k++) {
                    crop_data[j*crop_width + k + crop_width * crop_height * i] = input_data[j*input_width + k + input_width * input_height * i];
                }
            }
        }
    default:
        break;
    }
}
void main(int argc, char* args[]) {
    const char* input_path = "/home/zyz/projects/test_project/yuv_operation/rgbp_size_operation/lake_1920x1080.rgb";
    const char* output_path = "/home/zyz/projects/test_project/yuv_operation/rgbp_size_operation/rgb_planar_opt.bin";
    int width = 1920;
    int height = 1080;
    int output_width = 500;
    int output_height = 500;
    int format = FORMAT_RGB_PACKED;
    if (argc > 1) input_path = args[1];
    if (argc > 2) output_path = args[2];
    if (argc > 3) width = atoi(args[3]);
    if (argc > 4) height = atoi(args[4]);
    if (argc > 5) output_width = atoi(args[5]);
    if (argc > 6) output_height = atoi(args[6]);
    if (argc > 7) format = atoi(args[7]);
    unsigned char *input_data = (unsigned char*)malloc(width*height*3*sizeof(unsigned char));
    unsigned char *output_data = (unsigned char*)malloc(output_width*output_height*3*sizeof(unsigned char));
    readbin(input_path, input_data, width, height, 3);
    crop_img(input_data, output_data, width, height, output_width, output_height, format);
    writebin(output_path, output_data, output_width * output_height * 3);
}