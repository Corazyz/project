#include <map>
#include <cmath>
#include <memory>
#include <random>
#include <vector>
#include <iostream>
#include <sys/time.h>
#include <unordered_map>

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "bmlib_runtime.h"
#include "tpu_api_protocol.h"
#include "kernel_module_data.h"

#define CLIP(val, max, min)             \
    val = (val <= min) ? min : val;     \
    val = (val >= max) ? max : val
#define TPU_KERNEL_MAX_IMAGE_DIM 3
#define TPU_KERNEL_MAX_IMAGE_CHANNELS 3

typedef enum tpu_kernel_image_t_format_ext_{
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
}  tpu_kernel_image_t_format_ext;

typedef enum {
    DT_INT8   = (0 << 1) | 1,
    DT_UINT8  = (0 << 1) | 0,
    DT_INT16  = (3 << 1) | 1,
    DT_UINT16 = (3 << 1) | 0,
    DT_FP16   = (1 << 1) | 1,
    DT_BFP16  = (5 << 1) | 1,
    DT_INT32  = (4 << 1) | 1,
    DT_UINT32 = (4 << 1) | 0,
    DT_FP32   = (2 << 1) | 1
} bm_data_type_t;

typedef struct tpu_kernel_image {
    bm_device_mem_t mem[TPU_KERNEL_MAX_IMAGE_CHANNELS];
    int height, width;
    bm_data_type_t dtype;
    tpu_kernel_image_t_format_ext format;
    unsigned stride[TPU_KERNEL_MAX_IMAGE_DIM];
} tpu_kernel_image_t;

typedef struct tpu_kernel_resize_s {
    int start_x;
    int start_y;
    int in_width;
    int in_height;
} tpu_kernel_resize_t;

typedef struct tpu_kernel_resize_image_s {
    tpu_kernel_resize_t *resize_img_attr;
    int roi_num;
    unsigned char stretch_fit;
    unsigned char padding_b;
    unsigned char padding_g;
    unsigned char padding_r;
    unsigned int interpolation;
} tpu_kernel_resize_image_t;

using std::make_shared;
using std::shared_ptr;
using namespace std;
static bm_handle_t handle;

class Blob {
    public:
    void *data      = NULL;
    int   byte_size = 0;

    explicit Blob(int size) {
        if (size == 0) {
            data      = NULL;
            byte_size = 0;
            return;
        }
        data      = malloc(size);
        byte_size = size;
    }
    ~Blob() {
        if (data)
            free(data);
    }
};

template <typename T>
shared_ptr<Blob> gen_random_gray_data(int image_n,
                                      int image_h,
                                      int image_w) {
    shared_ptr<Blob> output_ =
        make_shared<Blob>(image_n * image_h * image_w * sizeof(T));
    T *input_ptr = (T *)output_->data;
    for (int n = 0; n < image_n; n++) {
       for (int i = 0; i < image_h * image_w; i++) {
            input_ptr[n * image_h * image_w + i] = rand() % 256;
        }
    }
    return output_;
}

template <typename T>
shared_ptr<Blob> gen_random_bgr_data(int image_n,
                                     int image_h,
                                     int image_w) {
    shared_ptr<Blob> output_ =
        make_shared<Blob>(image_n * image_h * image_w * 3 * sizeof(T));
    T *input_ptr = (T *)output_->data;
    for (int n = 0; n < image_n; n++) {
       for (int i = 0; i < image_h * image_w; i++) {
            input_ptr[n * 3 * image_h * image_w + i] = rand() % 256;
            input_ptr[n * 3 * image_h * image_w + image_h * image_w + i] =
                rand() % 256;
            input_ptr[n * 3 * image_h * image_w + 2 * image_h * image_w + i] =
                rand() % 256;
        }
    }
    return output_;
}

template <typename T>
static shared_ptr<Blob> convert_unpack_1N_RGB_to_YUV444(shared_ptr<Blob> input_,
                                                        int              image_n,
                                                        int              image_h,
                                                        int              image_w) {
    shared_ptr<Blob> output_ =
        make_shared<Blob>(image_n * image_h * image_w * 3 * sizeof(T));

    T *raw_ptr       = (T *)input_->data;
    T *reference_ptr = (T *)output_->data;

    for (int n = 0; n < image_n; n++) {
        for (int h = 0; h < image_h; h++) {
            for (int w = 0; w < image_w; w++) {
                float b =
                    (float)raw_ptr[n * 3 * image_h * image_w + image_w * h + w];
                float g = (float)raw_ptr[n * 3 * image_h * image_w +
                                         image_h * image_w + image_w * h + w];
                float r =
                    (float)raw_ptr[n * 3 * image_h * image_w +
                                   2 * image_h * image_w + image_w * h + w];
                float y, u, v;
                y = (0.257 * r + 0.504 * g + 0.098 * b + 16);
                u = (-0.148 * r - 0.291 * g + 0.439 * b + 128);
                v = (0.439 * r - 0.368 * g - 0.071 * b + 128);
                CLIP(y, 255, 0);
                CLIP(u, 255, 0);
                CLIP(v, 255, 0);
                reference_ptr[n * image_h * image_w * 3 + image_w * h + w] =
                    (T)y;
                reference_ptr[n * image_h * image_w * 3 + image_h * image_w +
                              image_w * h + w]                         = (T)u;
                reference_ptr[n * image_h * image_w * 3 +
                              image_h * image_w * 2 + image_w * h + w] = (T)v;
            }
        }
    }
    return output_;
}

template <typename T>
static shared_ptr<Blob> convert_YUV444_to_YUV420SP(shared_ptr<Blob> input_,
                                                   int              format,
                                                   int              image_n,
                                                   int              image_h,
                                                   int              image_w) {
    shared_ptr<Blob> output_ =
        make_shared<Blob>(image_n * image_h * image_w * 3 / 2 * sizeof(T));
    T *raw_ptr = (T *)input_->data;
    T *out     = (T *)output_->data;

    for (int n = 0; n < image_n; n++) {
        memcpy(out + n * image_h * image_w * 3 / 2,
               raw_ptr + n * image_h * image_w * 3,
               image_h * image_w * sizeof(T));
        for (int h = 0; h < image_h / 2; h++) {
            for (int w = 0; w < image_w / 2; w++) {
                float u = raw_ptr[n * image_h * image_w * 3 + image_h * image_w +
                                image_w * h * 2 + w * 2] +
                        raw_ptr[n * image_h * image_w * 3 + image_h * image_w +
                                image_w * (h * 2 + 1) + w * 2 + 1] +
                        raw_ptr[n * image_h * image_w * 3 + image_h * image_w +
                                image_w * h * 2 + w * 2 + 1] +
                        raw_ptr[n * image_h * image_w * 3 + image_h * image_w +
                                image_w * (h * 2 + 1) + w * 2];

                float v =
                    raw_ptr[n * image_h * image_w * 3 + 2 * image_h * image_w +
                            image_w * h * 2 + w * 2] +
                    raw_ptr[n * image_h * image_w * 3 + 2 * image_h * image_w +
                            image_w * (h * 2 + 1) + w * 2 + 1] +
                    raw_ptr[n * image_h * image_w * 3 + 2 * image_h * image_w +
                            image_w * h * 2 + w * 2 + 1] +
                    raw_ptr[n * image_h * image_w * 3 + 2 * image_h * image_w +
                            image_w * (h * 2 + 1) + w * 2];
                u                            = u / 4;
                v                            = v / 4;
                if(format == 3){
                    out[n * image_h * image_w * 3 / 2 + image_h * image_w +
                        image_w * h + w * 2]     = (T)u;
                    out[n * image_h * image_w * 3 / 2 + image_h * image_w +
                        image_w * h + w * 2 + 1] = (T)v;
                } else if(format == 4){
                    out[n * image_h * image_w * 3 / 2 + image_h * image_w +
                        image_w * h + w * 2]     = (T)v;
                    out[n * image_h * image_w * 3 / 2 + image_h * image_w +
                        image_w * h + w * 2 + 1] = (T)u;
                }
            }
        }
    }
    return output_;
}

static shared_ptr<Blob> convert_YUV444_to_YUV420SP_float(shared_ptr<Blob> input_,
                                                     int              format,
                                                     int              image_n,
                                                     int              image_h,
                                                     int              image_w) {
    shared_ptr<Blob> output_ =
        make_shared<Blob>(image_n * image_h * image_w * 3 / 2 * sizeof(float));

    float *raw_ptr = (float *)input_->data;
    float *out     = (float *)output_->data;

    for (int n = 0; n < image_n; n++) {
        memcpy(out + n * image_h * image_w * 3 / 2,
               raw_ptr + n * image_h * image_w * 3,
               image_h * image_w * sizeof(float));
        for (int h = 0; h < image_h / 2; h++) {
            for (int w = 0; w < image_w / 2; w++) {
                float u = raw_ptr[n * image_h * image_w * 3 + image_h * image_w +
                                image_w * h * 2 + w * 2] +
                        raw_ptr[n * image_h * image_w * 3 + image_h * image_w +
                                image_w * (h * 2 + 1) + w * 2 + 1] +
                        raw_ptr[n * image_h * image_w * 3 + image_h * image_w +
                                image_w * h * 2 + w * 2 + 1] +
                        raw_ptr[n * image_h * image_w * 3 + image_h * image_w +
                                image_w * (h * 2 + 1) + w * 2];

                float v =
                    raw_ptr[n * image_h * image_w * 3 + 2 * image_h * image_w +
                            image_w * h * 2 + w * 2] +
                    raw_ptr[n * image_h * image_w * 3 + 2 * image_h * image_w +
                            image_w * (h * 2 + 1) + w * 2 + 1] +
                    raw_ptr[n * image_h * image_w * 3 + 2 * image_h * image_w +
                            image_w * h * 2 + w * 2 + 1] +
                    raw_ptr[n * image_h * image_w * 3 + 2 * image_h * image_w +
                            image_w * (h * 2 + 1) + w * 2];
                u                            = u / 4;
                v                            = v / 4;
                if(format == 3){
                    out[n * image_h * image_w * 3 / 2 + image_h * image_w +
                        image_w * h + w * 2]     = (float)u;
                    out[n * image_h * image_w * 3 / 2 + image_h * image_w +
                        image_w * h + w * 2 + 1] = (float)v;
                } else if (format == 4){
                    out[n * image_h * image_w * 3 / 2 + image_h * image_w +
                        image_w * h + w * 2]     = (float)v;
                    out[n * image_h * image_w * 3 / 2 + image_h * image_w +
                        image_w * h + w * 2 + 1] = (float)u;
                }
            }
        }
    }
    return output_;
}

template <typename T>
static shared_ptr<Blob> convert_NV12_to_YUV420P(shared_ptr<Blob> input_,
                                                int              IMAGE_N,
                                                int              IMAGE_H,
                                                int              IMAGE_W) {
    shared_ptr<Blob> output_ =
        make_shared<Blob>(IMAGE_N * IMAGE_H * IMAGE_W * 3 / 2 * sizeof(T));
    T *raw_ptr = (T *)input_->data;
    T *out     = (T *)output_->data;

    for (int n = 0; n < IMAGE_N; n++) {
        memcpy(out + n * IMAGE_H * IMAGE_W * 3 / 2,
               raw_ptr + n * IMAGE_H * IMAGE_W * 3 / 2,
               IMAGE_H * IMAGE_W * sizeof(T));
        for (int i = 0; i < IMAGE_H * IMAGE_W / 4; i++) {

            T u = raw_ptr[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W +
                           2 * i];
            T v = raw_ptr[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W +
                           2 * i + 1];

            out[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W + i] = (T)u;
            out[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W +
                IMAGE_H * IMAGE_W / 4 + i]                             = (T)v;
        }
    }
    return output_;
}

static shared_ptr<Blob> convert_NV12_to_YUV420P_float(shared_ptr<Blob> input_,
                                                      int              IMAGE_N,
                                                      int              IMAGE_H,
                                                      int              IMAGE_W) {
    shared_ptr<Blob> output_ =
        make_shared<Blob>(IMAGE_N * IMAGE_H * IMAGE_W * 3 / 2 * sizeof(float));

    float *raw_ptr = (float *)input_->data;
    float *out     = (float *)output_->data;

    for (int n = 0; n < IMAGE_N; n++) {
        memcpy(out + n * IMAGE_H * IMAGE_W * 3 / 2,
               raw_ptr + n * IMAGE_H * IMAGE_W * 3 / 2,
               IMAGE_H * IMAGE_W * sizeof(float));
        for (int i = 0; i < IMAGE_H * IMAGE_W / 4; i++) {
            float u = raw_ptr[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W +
                           2 * i];
            float v = raw_ptr[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W +
                           2 * i + 1];

            out[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W + i] = (float)u;
            out[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W +
                IMAGE_H * IMAGE_W / 4 + i]                             = (float)v;
        }
    }
    return output_;
}

template <typename T>
static shared_ptr<Blob> convert_YUV420P_to_NV12(shared_ptr<Blob> input_,
                                                int              IMAGE_N,
                                                int              IMAGE_H,
                                                int              IMAGE_W) {
    shared_ptr<Blob> output_ =
        make_shared<Blob>(IMAGE_N * IMAGE_H * IMAGE_W * 3 * sizeof(T));

    T *raw_ptr = (T *)input_->data;
    T *out     = (T *)output_->data;

    for (int n = 0; n < IMAGE_N; n++) {
        memcpy(out + n * IMAGE_H * IMAGE_W * 3 / 2,
               raw_ptr + n * IMAGE_H * IMAGE_W * 3 / 2,
               IMAGE_H * IMAGE_W * sizeof(T));
        for (int i = 0; i < IMAGE_H * IMAGE_W / 4; i++) {
            T u =
                raw_ptr[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W + i];
            T v = raw_ptr[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W +
                           IMAGE_H * IMAGE_W / 4 + i];

            out[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W + i * 2] =
                (T)u;
            out[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W + i * 2 + 1] =
                (T)v;
        }
    }
    return output_;
}

template <typename T>
static shared_ptr<Blob> convert_NV12_to_YUV444(shared_ptr<Blob> input_,
                                               int              IMAGE_N,
                                               int              IMAGE_H,
                                               int              IMAGE_W) {
    shared_ptr<Blob> output_ =
        make_shared<Blob>(IMAGE_N * IMAGE_H * IMAGE_W * 3 * sizeof(T));

    T *raw_ptr = (T *)input_->data;
    T *out     = (T *)output_->data;

    for (int n = 0; n < IMAGE_N; n++) {
        memcpy(out + n * IMAGE_H * IMAGE_W * 3,
               raw_ptr + n * IMAGE_H * IMAGE_W * 3 / 2,
               IMAGE_H * IMAGE_W * sizeof(T));
        for (int h = 0; h < IMAGE_H; h++) {
            for (int w = 0; w < IMAGE_W; w++) {
                T u =
                    raw_ptr[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W +
                            IMAGE_W * (h / 2) + (w / 2) * 2];
                T v =
                    raw_ptr[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W +
                            IMAGE_W * (h / 2) + (w / 2) * 2 + 1];

                out[n * IMAGE_H * IMAGE_W * 3 + IMAGE_H * IMAGE_W +
                    IMAGE_W * h + w] = (T)u;
                out[n * IMAGE_H * IMAGE_W * 3 + IMAGE_H * IMAGE_W * 2 +
                    IMAGE_W * h + w] = (T)v;
            }
        }
    }
    return output_;
}

template <typename T>
static shared_ptr<Blob> convert_NV21_to_YUV444(shared_ptr<Blob> input_,
                                               int              IMAGE_N,
                                               int              IMAGE_H,
                                               int              IMAGE_W) {
    shared_ptr<Blob> output_ =
        make_shared<Blob>(IMAGE_N * IMAGE_H * IMAGE_W * 3 * sizeof(T));

    T *raw_ptr = (T *)input_->data;
    T *out     = (T *)output_->data;

    for (int n = 0; n < IMAGE_N; n++) {
        memcpy(out + n * IMAGE_H * IMAGE_W * 3,
               raw_ptr + n * IMAGE_H * IMAGE_W * 3 / 2,
               IMAGE_H * IMAGE_W * sizeof(T));
        for (int h = 0; h < IMAGE_H; h++) {
            for (int w = 0; w < IMAGE_W; w++) {
                T v =
                    raw_ptr[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W +
                            IMAGE_W * (h / 2) + (w / 2) * 2];
                T u =
                    raw_ptr[n * IMAGE_H * IMAGE_W * 3 / 2 + IMAGE_H * IMAGE_W +
                            IMAGE_W * (h / 2) + (w / 2) * 2 + 1];

                out[n * IMAGE_H * IMAGE_W * 3 + IMAGE_H * IMAGE_W +
                    IMAGE_W * h + w] = (T)u;
                out[n * IMAGE_H * IMAGE_W * 3 + IMAGE_H * IMAGE_W * 2 +
                    IMAGE_W * h + w] = (T)v;
            }
        }
    }
    return output_;
}

shared_ptr<Blob> convert_to_float_data(shared_ptr<Blob> u8_input,
                                       int image_n,
                                       int image_h,
                                       int image_w,
                                       int is_gray) {
    shared_ptr<Blob> output_ =
        make_shared<Blob>(image_n * image_h * image_w * (is_gray ? 1 : 3) * sizeof(float));
    float *input_ptr = (float *)output_->data;
    u8 *u8_input_ptr = (u8 *)u8_input->data;
    for (int n = 0; n < image_n; n++) {
        for (int i = 0; i < image_h * image_w; i++) {
            input_ptr[n * 3 * image_h * image_w + i] =
                    (float)(u8_input_ptr[n * 3 * image_h * image_w + i]);
            if(!is_gray) {
                input_ptr[n * 3 * image_h * image_w + image_h * image_w + i] =
                        (float)(u8_input_ptr[n * 3 * image_h * image_w + image_h * image_w + i]);
                input_ptr[n * 3 * image_h * image_w + 2 * image_h * image_w + i] =
                        (float)(u8_input_ptr[n * 3 * image_h * image_w + 2 * image_h * image_w + i]);
            }
        }
    }
    return output_;
}

shared_ptr<Blob> convert_from_float_data(shared_ptr<Blob> float_input,
                                         int len) {
    shared_ptr<Blob> u8_output_ =
        make_shared<Blob>(len * sizeof(u8));
    float *input_ptr = (float *)float_input->data;
    u8 *u8_output_ptr = (u8 *)u8_output_->data;
    for (int i = 0; i < len; i++) {
        u8_output_ptr[i] = (u8)round(input_ptr[i] );
    }
    return u8_output_;
}

shared_ptr<Blob> image_resize_bilinear_float(shared_ptr<Blob> input,
                                             int src_h,
                                             int src_w,
                                             int dst_h,
                                             int dst_w,
                                             int is_gray) {
    shared_ptr<Blob> output = make_shared<Blob>(dst_h * dst_w * (is_gray ? 1 : 3) * sizeof(float));
    float* dataDst = (float *)output->data;
    float* dataSrc = (float *)input->data;
    int stepDst = dst_h * dst_w;
    int stepSrc = src_h * src_w;
    float scale_x = (float)src_w / dst_w;
    float scale_y = (float)src_h / dst_h;

    for (int k = 0; k < (is_gray ? 1 : 3); ++k) {
        int sx_plus_1;
        int sy_plus_1;

        for (int j = 0; j < dst_h; ++j) {
            float fy = (float)((j + 0.5) * scale_y - 0.5);
            int sy = floor(fy);
            fy -= sy;

            sy_plus_1 = sy + 1;
            if (sy >= src_h - 1) {
                sy = src_h - 1;
                sy_plus_1 = sy;
            }
            if (sy < 0) {
                sy = 0;
                sy_plus_1 = 0;
                fy = 0;
            }

            float cbufy[2];
            cbufy[0] = 1.f - fy;
            cbufy[1] = fy;

            for (int i = 0; i < dst_w; ++i) {
                float fx = (float)((i + 0.5) * scale_x - 0.5);
                int sx = floor(fx);
                fx -= sx;

                sx_plus_1 = sx + 1;
                if (sx >= src_w - 1) {
                    sx = src_w -1;
                    sx_plus_1 = sx;
                }
                if (sx < 0) {
                    fx = 0, sx = 0;
                    sx_plus_1 = 1;
                }

                float cbufx[2];
                cbufx[0] = 1.f - fx;
                cbufx[1] = fx;
                *(dataDst + k * stepDst + j * dst_w + i) =
                        (*(dataSrc + k * stepSrc + sy * src_w + sx) * cbufx[0] * cbufy[0] +
                        *(dataSrc + k * stepSrc + sy_plus_1 * src_w + sx) * cbufx[0] * cbufy[1] +
                        *(dataSrc + k * stepSrc + sy * src_w + sx_plus_1) * cbufx[1] * cbufy[0] +
                        *(dataSrc + k * stepSrc + sy_plus_1 * src_w + sx_plus_1) * cbufx[1] * cbufy[1]);
            }
        }
    }
    return output;
}

shared_ptr<Blob> image_resize_padding_float( shared_ptr<Blob> src_format_input,
                                              tpu_kernel_image_t_format_ext dst_format,
                                              int src_h,
                                              int src_w,
                                              int dst_h,
                                              int dst_w,
                                              int padding_r,
                                              int padding_g,
                                              int padding_b,
                                              int is_gray){
    shared_ptr<Blob> output = make_shared<Blob>(dst_h * dst_w * (is_gray ? 1 : 3) * sizeof(float));
    float r = std::min(float(dst_w) / src_w, float(dst_h) / src_h);
    int inside_w = round(src_w * r);
    int inside_h = round(src_h * r);
    shared_ptr<Blob> inline_resized;
    inline_resized = image_resize_bilinear_float(src_format_input, src_h, src_w, inside_h, inside_w, is_gray);

    float *tmp_output = (float*)output->data;
    float *in_resize = (float*)inline_resized->data;
    for(int i = 0; i < dst_h; i++){
        for(int j = 0; j < dst_w; j++){
            if(dst_format == FORMAT_GRAY){
                if((i < inside_h) && (j < inside_w)){
                    tmp_output[i * dst_w + j] = in_resize[i * inside_w + j];
                } else {
                    tmp_output[i * dst_w + j] = padding_b;
                }
            }else{
                if((i < inside_h) && (j < inside_w)){
                    tmp_output[i * dst_w + j] = in_resize[i * inside_w + j];
                    tmp_output[dst_h * dst_w + i * dst_w + j] = in_resize[inside_w * inside_h + i * inside_w + j];
                    tmp_output[2 * dst_h * dst_w + i * dst_w + j] = in_resize[2 * inside_w * inside_h + i * inside_w + j];
                } else {
                    if( (dst_format == FORMAT_YUV420P) || (dst_format == FORMAT_NV12) ||
                        (dst_format == FORMAT_NV21)){
                            float y = (0.257 * padding_r + 0.504 * padding_g + 0.098 * padding_b + 16);
                            float u = (-0.148 * padding_r - 0.291 * padding_g + 0.439 * padding_b + 128);
                            float v = (0.439 * padding_r - 0.368 * padding_g - 0.071 * padding_b + 128);
                            CLIP(y, 255, 0);
                            CLIP(u, 255, 0);
                            CLIP(v, 255, 0);
                            tmp_output[i * dst_w + j] = y;
                            tmp_output[dst_h * dst_w + i * dst_w + j] = u;
                            tmp_output[2 * dst_h * dst_w + i * dst_w + j] = v;
                    } else if (dst_format == FORMAT_BGR_PLANAR){
                        tmp_output[i * dst_w + j] = padding_b;
                        tmp_output[dst_h * dst_w + i * dst_w + j] = padding_g;
                        tmp_output[2 * dst_h * dst_w + i * dst_w + j] = padding_r;
                    }
                }
            }
        }
    }
    return output;
}

template <typename T, typename D>
vector<shared_ptr<Blob> > convert_resize_native( shared_ptr<Blob> src_format_input,
                                                 tpu_kernel_image_t_format_ext src_format,
                                                 tpu_kernel_image_t_format_ext dst_format,
                                                 int  s_data_type,
                                                 int  d_data_type,
                                                 int  input_num,
                                                 int* start_x,
                                                 int* start_y,
                                                 int  src_h,
                                                 int  src_w,
                                                 int  dst_h,
                                                 int  dst_w,
                                                 int  if_padding,
                                                 int  padding_r,
                                                 int  padding_g,
                                                 int  padding_b) {
    shared_ptr<Blob> converted_output = src_format_input;
    shared_ptr<Blob> resized_output;
    vector<shared_ptr<Blob> > resized_outputs;
    int is_gray = (src_format == FORMAT_GRAY) ? 1 : 0;
    if(src_format == FORMAT_YUV420P){
        converted_output = convert_YUV420P_to_NV12<T>(converted_output, input_num, src_h, src_w);
        converted_output = convert_NV12_to_YUV444<T>(converted_output, input_num, src_h, src_w);
    } else if(src_format == FORMAT_NV12){
        converted_output = convert_NV12_to_YUV444<T>(converted_output, input_num, src_h, src_w);
    } else if(src_format == FORMAT_NV21){
        converted_output = convert_NV21_to_YUV444<T>(converted_output, input_num, src_h, src_w);
    }
    if (s_data_type == 0)
    {
        converted_output = convert_to_float_data(converted_output, input_num, src_h, src_w, is_gray);
    }
    if(!if_padding){
        // padding
        resized_output = image_resize_padding_float(converted_output, dst_format, src_h, src_w, dst_h,
                                                    dst_w, padding_r, padding_g, padding_b, is_gray);
    } else {
        resized_output = image_resize_bilinear_float(converted_output, src_h, src_w, dst_h, dst_w, is_gray);
    }
    int dst_data_num = 0;
    if(dst_format == FORMAT_YUV420P){
        dst_data_num = dst_h * dst_w + dst_h * dst_w / 2;
        resized_output = convert_YUV444_to_YUV420SP_float(resized_output, 3,input_num, dst_h, dst_w);
        resized_output = convert_NV12_to_YUV420P_float(resized_output, input_num, dst_h, dst_w);
    } else if((dst_format == FORMAT_NV12) || (dst_format == FORMAT_NV21)){
        dst_data_num = dst_h * dst_w + dst_h * dst_w / 2;
        resized_output = convert_YUV444_to_YUV420SP_float(resized_output, dst_format, input_num, dst_h, dst_w);
    } else if(dst_format == FORMAT_BGR_PLANAR){
        dst_data_num = dst_h * dst_w * 3;
    } else if (dst_format == FORMAT_GRAY){
        dst_data_num = dst_h * dst_w;
    }
    if(d_data_type == 0){
        resized_output = convert_from_float_data(resized_output, dst_data_num);
    }
    resized_outputs.push_back(resized_output);
    if((int)resized_outputs.size() != input_num) {
        std::cout << "Number of resized outputs doesn't match number of input's." << std::endl;
        exit(-1);
    }

    return resized_outputs;
}

bm_status_t tpu_kernel_image_resize( bm_handle_t handle,
                                     tpu_kernel_resize_image_t resize_attr,
                                     tpu_kernel_image_t input,
                                     tpu_kernel_image_t output){
    u64 input_dev_addr[3];
    for(int i = 0; i < 3; i++)
        input_dev_addr[i] = bm_mem_get_device_addr(input.mem[i]);

    u64 output_dev_addr[3];
    for(int i = 0; i < 3; i++)
        output_dev_addr[i] = bm_mem_get_device_addr(output.mem[i]);

    bm_device_mem_t yuv444_mem;
    bm_device_mem_t resize_mem;
    int per_src_size = input.height * input.width * 3 * sizeof(float);
    int per_dst_size = output.height * output.width * 3 * sizeof(float);
    if(BM_SUCCESS != bm_malloc_device_byte(handle, &yuv444_mem, per_src_size)){
        printf("bm_image_alloc_dev_mem error \r\n");
        exit(-1);
    }

    if(BM_SUCCESS != bm_malloc_device_byte(handle, &resize_mem, per_dst_size)){
        printf("bm_image_alloc_dev_mem error \r\n");
        exit(-1);
    }
    tpu_api_image_resize_t api;
    api.src_h = input.height;
    api.src_w = input.width;
    api.dst_h = output.height;
    api.dst_w = output.width;
    api.roi_n = 1;
    api.padding_b = resize_attr.padding_b;
    api.padding_g = resize_attr.padding_g;
    api.padding_r = resize_attr.padding_r;
    api.stretch_fit = resize_attr.stretch_fit;
    api.src_format = input.format;
    api.src_data_type = input.dtype;
    api.dst_format = output.format;
    api.dst_data_type = output.dtype;
    api.start_x[0] = resize_attr.resize_img_attr->start_x;
    api.start_y[0] = resize_attr.resize_img_attr->start_y;
    api.yuv444_global_offset[0] = bm_mem_get_device_addr(yuv444_mem);
    api.resize_global_offset[0] = bm_mem_get_device_addr(resize_mem);
    api.input_global_offset[0] = input_dev_addr[0];
    api.aux_input0_global_offset[0] = input_dev_addr[1];
    api.aux_input1_global_offset[0] = input_dev_addr[2];
    api.output_global_offset[0] = output_dev_addr[0];
    api.aux_output0_global_offset[0] = output_dev_addr[1];
    api.aux_output1_global_offset[0] = output_dev_addr[2];
    bm_status_t ret = BM_SUCCESS;

    tpu_kernel_module_t tpu_module;
    tpu_kernel_function_t func_id;
    const unsigned int *p = kernel_module_data;
    size_t length = sizeof(kernel_module_data);
    // Load device module
    tpu_module = tpu_kernel_load_module(handle, (const char *)p, length);
    func_id = tpu_kernel_get_function(handle, tpu_module, "cv_image_resize");

    struct timeval t1, t2;
    for(int i = 0; i < 2; i++) {
        gettimeofday(&t1, NULL);
        ret = tpu_kernel_launch(handle, func_id, &api, sizeof(api));
        gettimeofday(&t2, NULL);
        bm_thread_sync(handle);
    }
    cout << "--using time = " << ((t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec) << "(us)--" << endl;


    bm_free_device(handle, yuv444_mem);
    bm_free_device(handle, resize_mem);
    return ret;
}

static int array_cmp_float(float *p_exp, float*p_got, int len, const char *info_label, float delta){
    int idx = 0;
    for (idx = 0; idx < len; idx++){
        if (fabs(p_exp[idx] - p_got[idx]) > delta){
            printf("%s abs error at index %d exp %f got %f\n",
                   info_label,
                   idx,
                   p_exp[idx],
                   p_got[idx]);
            return -1;
        }
    }
    return 0;
}

static int array_cmp_u8(u8 *p_exp,
                        u8 *p_got,
                        int len,
                        const char *info_label,
                        u8 delta) {
    int idx = 0;
    for (idx = 0; idx < len; idx++) {
        if ((int)fabs(p_exp[idx] - (int)p_got[ idx]) > delta) {
            printf("%s abs error at index %d exp %d got %d\n",
                   info_label,
                   idx,
                   p_exp[idx],
                   p_got[idx]);
            return -1;
        }
    }
    return 0;
}


template <typename T, typename D>
static void test_image_resize(int _src_format,
                              int _dst_format,
                              int src_data_type,
                              int dst_data_type,
                              int start_x,
                              int start_y,
                              int src_h,
                              int src_w,
                              int dst_h,
                              int dst_w,
                              int if_padding,
                              int p_r,
                              int p_g,
                              int p_b){
    tpu_kernel_image_t_format_ext src_format = FORMAT_GRAY;
    tpu_kernel_image_t_format_ext dst_format = FORMAT_GRAY;
    switch (_src_format) {
        case 0:
            src_format = FORMAT_YUV420P;
            break;
        case 3:
            src_format = FORMAT_NV12;
            break;
        case 4:
            src_format = FORMAT_NV21;
            break;
        case 9:
            src_format = FORMAT_BGR_PLANAR;
            break;
        case 14:
            src_format = FORMAT_GRAY;
            break;
        default:
            break;
    };

    switch (_dst_format) {
        case 0:
            dst_format = FORMAT_YUV420P;
            break;
        case 3:
            dst_format = FORMAT_NV12;
            break;
        case 4:
            dst_format = FORMAT_NV21;
            break;
        case 9:
            dst_format = FORMAT_BGR_PLANAR;
            break;
        case 14:
            dst_format = FORMAT_GRAY;
            break;
        default:
            break;
    };

    /*check resize shape, must uniform scale(for single side padding), YUV shape must divided by 4*/
    if (!(dst_h >= src_h && dst_w >= src_w) && !(src_h >= dst_h && src_w >= dst_w)){
        std::cout << "resize must be uniform scale" << std::endl;
        exit(-1);
    }
    if ((src_format == FORMAT_YUV420P || src_format == FORMAT_NV12 || src_format == FORMAT_NV21) &&
         (src_h % 2 != 0 || src_w % 2 != 0 || dst_h % 2 != 0 || dst_w % 2 != 0)){
        std::cout << "  ! ! YUV shape must divided by 4 ! !  " << std::endl;
        exit(-1);
    }

    cout << "---------------parameter------------- " << endl;
    cout << "src_format: " << src_format << endl;
    cout << "dst_format: " << dst_format << endl;
    if (src_data_type == 0)
        cout << "src_data_type: " << "u8" << endl;
    else if (src_data_type == 1)
        cout << "src_data_type: " << "FP32" << endl;
    if (dst_data_type == 0)
        cout << "dst_data_type: " << "u8" << endl;
    else if (dst_data_type == 1)
        cout << "dst_data_type: " << "FP32" << endl;
    cout << "src_h: " << src_h << endl;
    cout << "src_w: " << src_w << endl;
    cout << "dst_h: " << dst_h << endl;
    cout << "dst_w: " << dst_w << endl;
    cout << "if_padding: " << if_padding << endl;
    cout << "-------------------------------------" << endl;

    // fill input data
    shared_ptr<Blob> src_format_input;
    if( (src_format == FORMAT_NV12) || (src_format == FORMAT_NV21)){
        shared_ptr<Blob> rgb_input = gen_random_bgr_data<T>(1, src_h, src_w);
        shared_ptr<Blob> yuv444_input = convert_unpack_1N_RGB_to_YUV444<T>(rgb_input, 1, src_h, src_w);
        src_format_input = convert_YUV444_to_YUV420SP<T>(yuv444_input, src_format, 1, src_h, src_w);
    } else if( src_format == FORMAT_YUV420P){
        shared_ptr<Blob> rgb_input = gen_random_bgr_data<T>(1, src_h, src_w);
        shared_ptr<Blob> yuv444_input = convert_unpack_1N_RGB_to_YUV444<T>(rgb_input, 1, src_h, src_w);
        src_format_input = convert_YUV444_to_YUV420SP<T>(yuv444_input, 3, 1, src_h, src_w);
        src_format_input = convert_NV12_to_YUV420P<T>(src_format_input, 1, src_h, src_w);
    } else if (src_format == FORMAT_GRAY){
        src_format_input = gen_random_gray_data<T>(1, src_h, src_w);
    } else if( src_format == FORMAT_BGR_PLANAR){
        src_format_input =  gen_random_bgr_data<T>(1, src_h, src_w);
    } else {
        cout << "Src format not supported." << endl;
        exit(-1);
    }

    // calculate reference
    vector<shared_ptr<Blob> > out_ref = convert_resize_native<T, D>(
        src_format_input, src_format, dst_format, src_data_type, dst_data_type, 1, 0, 0,
            src_h, src_w,  dst_h, dst_w, if_padding, p_r,  p_g, p_b);
    // calculate with TPUKernel code
    bm_data_type_t src_data_format = (src_data_type == 0) ? DT_UINT8 : DT_FP32;
    bm_data_type_t dst_data_format = (dst_data_type == 0) ? DT_UINT8 : DT_FP32;

    tpu_kernel_image_t src_input;
    int input_size[3] = {0};
    int channel = 0;

    if(src_format == FORMAT_GRAY){
        input_size[0] = src_h * src_w;
        channel = 1;
    }
    else if(src_format == FORMAT_YUV420P){
        input_size[0] = src_h * src_w;
        input_size[1] = src_h * src_w / 4;
        input_size[2] = src_h * src_w / 4;
        channel = 3;
    }else if (src_format == FORMAT_NV12 || src_format == FORMAT_NV21){
        input_size[0] = src_h * src_w;
        input_size[1] = src_h * src_w / 2;
        channel = 2;
    }else if (src_format == FORMAT_BGR_PLANAR){
        input_size[0] = 3 * src_h * src_w;
        channel = 1;
    }
    bm_device_mem_t src_dev_mem[channel];

    T *input_host_ptr = (T *)src_format_input->data;
    T *input_image_ptr[3] = { input_host_ptr,
                              input_host_ptr + input_size[0],
                              input_host_ptr + input_size[0] + input_size[1]};
    for(int i = 0; i < channel; i++){
        if (BM_SUCCESS != bm_malloc_device_byte(handle, &src_dev_mem[i], input_size[i] * sizeof(T))){
            std::cout << "malloc input fail" << std::endl;
            exit(-1);
        }
        if (BM_SUCCESS != bm_memcpy_s2d(handle, src_dev_mem[i], input_image_ptr[i])){
            std::cout << "copy input to device fail" << std::endl;
            for(int c = 0; c < i; c++)
                bm_free_device(handle, src_dev_mem[c]);
            exit(-1);
        }
        src_input.mem[i] = src_dev_mem[i];
    }
    src_input.height = src_h;
    src_input.width = src_w;
    src_input.dtype = src_data_format;
    src_input.format = src_format;

    tpu_kernel_image_t dst_output;
    bm_device_mem_t dst_dev_mem[channel];
    int dst_size[3] = {0};

    if(dst_format == FORMAT_GRAY)
        dst_size[0] = dst_h * dst_w;
    else if(dst_format == FORMAT_YUV420P){
        dst_size[0] = dst_h * dst_w;
        dst_size[1] = dst_h * dst_w / 4;
        dst_size[2] = dst_h * dst_w / 4;
    }else if (dst_format == FORMAT_NV12 || dst_format == FORMAT_NV21){
        dst_size[0] = dst_h * dst_w;
        dst_size[1] = dst_h * dst_w / 2;
    }else if (dst_format == FORMAT_BGR_PLANAR)
        dst_size[0] = 3 * dst_h * dst_w;

    int total_dst_len = dst_size[0] + dst_size[1] + dst_size[2];
    vector<shared_ptr<Blob> > output_blobs;
    shared_ptr<Blob> output_blob_ =
                make_shared<Blob>(total_dst_len * sizeof(D));

    D *output_host_ptr = (D *)output_blob_->data;
    D *output_image_ptr[3] = { output_host_ptr,
                               output_host_ptr + dst_size [0],
                               output_host_ptr + dst_size[0] + dst_size[1]};
    for(int i = 0; i < channel; i++){
        if (BM_SUCCESS != bm_malloc_device_byte(handle, &dst_dev_mem[i], dst_size[i] * sizeof(D))){
            std::cout << "malloc output fail" << std::endl;
            for(int j = 0; j < channel; j++)
                bm_free_device(handle, src_dev_mem[j]);
            for(int k = 0; k < i; k++)
                bm_free_device(handle, dst_dev_mem[k]);
            exit(-1);
        }
        dst_output.mem[i] = dst_dev_mem[i];
    }

    dst_output.height = dst_h;
    dst_output.width = dst_w;
    dst_output.dtype = dst_data_format;
    dst_output.format = dst_format;

    tpu_kernel_resize_t resize_info;
    tpu_kernel_resize_image_t  resize_image;
    resize_info.start_x = start_x;
    resize_info.start_y = start_y;
    resize_info.in_height = src_h;
    resize_info.in_width = src_w;
    resize_image.resize_img_attr = &resize_info;
    resize_image.roi_num = 1;
    resize_image.stretch_fit = if_padding;
    resize_image.padding_b = p_b;
    resize_image.padding_g = p_g;
    resize_image.padding_r = p_r;

    bm_status_t status = tpu_kernel_image_resize(handle, resize_image, src_input, dst_output);
    if(BM_SUCCESS != status){
        printf("run tpu_image_resize failed status = %d \n", status);
        for(int i = 0; i < channel; i++){
            bm_free_device(handle, src_dev_mem[i]);
            bm_free_device(handle, dst_dev_mem[i]);
        }
        exit(-1);
    }

    for(int i = 0; i < channel; i++){
        if(BM_SUCCESS != bm_memcpy_d2s(handle, output_image_ptr[i], dst_dev_mem[i])){
            std::cout << "bm_memcpy_d2s fail" << std::endl;
            for(int i = 0; i < channel; i++){
                bm_free_device(handle, src_dev_mem[i]);
                bm_free_device(handle, dst_dev_mem[i]);
            }
            exit(-1);
        }
    }
    output_blobs.push_back(output_blob_);

    for(int i = 0; i < channel; i++){
        bm_free_device(handle, src_dev_mem[i]);
        bm_free_device(handle, dst_dev_mem[i]);
    }
    int ret = 0;
    if(dst_data_format == DT_FP32){
        ret = array_cmp_float( (float *)out_ref[0]->data,
                               (float *)output_blobs[0]->data,
                               total_dst_len,
                               "tpu_image_resize",
                               0.1);
    } else{
        ret = array_cmp_u8( (u8*) out_ref[0]->data,
                            (u8*) output_blobs[0]->data,
                            total_dst_len,
                            "tpu_image_resize",
                            1);
        }
     if(ret != 0){
            cout << "------[IMAGE_RESIZE TEST FAILED!]------" << endl;
            exit(-1);
    }
}

int main(int argc, char* argv[]){
    int src_format = 3; // 0, 3, 4, 9, 14
    int dst_format = 3; // 0, 3, 4, 9, 14
    int src_data_type = 0;  // 0, 1
    int dst_data_type = 1;  // 0, 1
    int src_h = 1024;
    int src_w = 1920;
    int dst_h = 4096;
    int dst_w = 4096;
    int if_padding = 1; // 0 1
    int padding_r = 9;
    int padding_g = 8;
    int padding_b = 7;

    if(argc > 1)    src_format = atoi(argv[1]);
    if(argc > 1)    dst_format = atoi(argv[1]);
    if(argc > 2)    src_h = atoi(argv[2]);
    if(argc > 3)    src_w = atoi(argv[3]);
    if(argc > 4)    dst_h = atoi(argv[4]);
    if(argc > 5)    dst_w = atoi(argv[5]);
    if(argc > 6)    if_padding = atoi(argv[6]);
    if(argc > 7)    padding_r = atoi(argv[7]);
    if(argc > 8)    padding_g = atoi(argv[8]);
    if(argc > 9)    padding_b = atoi(argv[9]);

    int start_x = 0;
    int start_y = 0;

    int dev_id = 0;
    bm_status_t status = bm_dev_request(&handle, dev_id);
    if (status != BM_SUCCESS) {
        printf("Create bm handle failed. ret = %d\n", status);
        exit(-1);
    }
    test_image_resize<u8, float>(src_format,
                                 dst_format,
                                 src_data_type,
                                 dst_data_type,
                                 start_x,
                                 start_y,
                                 src_h,
                                 src_w,
                                 dst_h,
                                 dst_w,
                                 if_padding,
                                 padding_r,
                                 padding_g,
                                 padding_b);
    cout << "------[IMAGE_RESIZE TEST PASSED!]------" << endl;
    bm_dev_free(handle);
    return 0;
}
