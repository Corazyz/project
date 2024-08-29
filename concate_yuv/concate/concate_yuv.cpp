#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void write_file(char *data, int len) {
    static int m = 0;
    FILE *fp = nullptr;
    char filename[1024] = {0};
    snprintf(filename, sizeof(filename), "image%02d.yuv", m++);
    if ( (fp = fopen(filename, "wb+")) == NULL) {
        printf("failed to open output file!\n");
        return;
    }

    fwrite(data, 1, len, fp);
    fclose(fp);
}

void vertical_concate(char *buff_out, char *data1, char *data2, int width, int height) {
    memcpy(buff_out, data1, width * height);
    memcpy(buff_out + width * height, data2, width * height);
    memcpy(buff_out + width * height * 2, data1 + width * height, width * height / 2);
    memcpy(buff_out + width * height * 5 / 2, data2 + width * height, width * height / 2);

    write_file(buff_out, width * height * 3);
}

void horizontal_concate(char *buff_out, char *data1, char *data2, int width, int height) {
    int shift = 0;
    for (int i = 0; i < height; i++) {
        memcpy(buff_out + shift * width, data1 + i * width, width);
        shift += 1;
        memcpy(buff_out + shift * width, data2 + i * width, width);
        shift += 1;
    }
    shift = 0;
    for (int i = 0; i < height / 2; i++) {
        memcpy(buff_out + 2 * width * height + shift * width, data1 + width * height + i * width, width);
        shift += 1;
        memcpy(buff_out + 2 * width * height + shift * width, data2 + width * height + i * width, width);
        shift += 1;
    }
    write_file(buff_out, height * width * 3);
}
void extract_y_channel(char *buff_out, char *data, int width, int height) {
    for (int i = 0; i < height; i++) {
        memcpy(buff_out + i * width, data + i * width, width);
    }
    write_file(buff_out, height * width);
}

int realization() {
    FILE *yuv_info;
    yuv_info = fopen("input_1920_1080.yuv", "rb");
    if (yuv_info == nullptr) {
        printf("Open YUV file failed.\n");
        return -1;
    } else {
        printf("Open YUV file succeeded!\n");
    }

    int width = 1920, height = 1080;
    int len = width * height * 3 / 2;
    char *buff_info = (char *)malloc(len);
    char *tmp_buff_info = buff_info;
    if (fread(buff_info, 1, len, yuv_info) != len) {
        printf("Failed to read from YUV file!\n");
        free(buff_info);
        fclose(yuv_info);
        return -1;
    }

    char *new_buff_info = (char *)malloc(3 * width * height);
    char *tmp_new_buff_info1 = new_buff_info;
    char *tmp_new_buff_info2 = new_buff_info;
    char *tmp_new_buff_info3 = new_buff_info;
    vertical_concate(tmp_new_buff_info1, tmp_buff_info, tmp_buff_info, width, height);
    horizontal_concate(tmp_new_buff_info1, tmp_buff_info, tmp_buff_info, width, height);
    extract_y_channel(tmp_new_buff_info1, tmp_buff_info, width, height);

    // 写入新文件
    // FILE *fp = fopen("output_00.yuv", "wb+");
    // if (fp == nullptr) {
    //     printf("Failed to open output file!\n");
    //     free(new_buff_info);
    //     fclose(yuv_info);
    //     return -1;
    // }
    // fwrite(new_buff_info, 1, 3 * width * height, fp);
    // fclose(fp);

    // 释放内存
    free(buff_info);
    free(new_buff_info);
    fclose(yuv_info);

    return 0;
}

    // // 复制 Y 分量
    // for (int i = 0; i < height; i++) {
    //     memcpy(tmp_new_buff_info, tmp_buff_info, width);
    //     tmp_new_buff_info += width;
    //     memcpy(tmp_new_buff_info, tmp_buff_info, width);
    //     tmp_new_buff_info += width;
    //     tmp_buff_info += width;
    // }

    // // 复制 U 和 V 分量
    // for (int i = 0; i < height/2; i++) {
    //     memcpy(tmp_new_buff_info, tmp_buff_info, width);
    //     tmp_new_buff_info += width;
    //     memcpy(tmp_new_buff_info, tmp_buff_info, width);
    //     tmp_new_buff_info += width;
    //     tmp_buff_info += width;
    // }