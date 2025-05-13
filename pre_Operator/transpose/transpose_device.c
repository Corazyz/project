#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdbool.h"

#define LOCAL_MEM_SIZE 64
#define NPU_NUM 32
typedef enum {
    TRANS_GENERAL = 0,
    TRANS_NPU_N_SWITCH_W,
    TRANS_GDMA_NCH,
    TRANS_NPU_H_SWITCH_W,
} trans_axis;

typedef struct {
    trans_axis trans_method;
    int N;
    int C;
    int H;
    int W;
    int max_trans_counts;
} trans_info_t;

#define GET_OPTIMIZED_FACTORIZATION(factor, polynomial)         \
    {                                                           \
        factor = 0;                                             \
        if(polynomial > NPU_NUM){                               \
            for(int i = NPU_NUM; i >= (NPU_NUM / 2); i--) {     \
                if(polynomial % i == 0) {                       \
                    factor = i;                                 \
                    break;                                      \
                }                                               \
            }                                                   \
        }                                                       \
    }

trans_info_t sg_get_transpose_info_(int fixed_dim,
                                 int left_dim,
                                 int right_dim,
                                 int type_len) {
    trans_info_t trans_info;
    int factor = 0;
    bool use_left_dim = false;
    bool use_right_dim = false;
    int min_transpose_size = 0;
    float overflow = 0.f;
    if (left_dim > right_dim) {
        GET_OPTIMIZED_FACTORIZATION(factor, left_dim)
        if ((factor > 0) && (factor != left_dim) && (fixed_dim < left_dim)) {
             use_left_dim = true;
        } else {
            GET_OPTIMIZED_FACTORIZATION(factor, right_dim)
            if ((factor > 0) && (factor != right_dim) && (fixed_dim == 1)) {
                use_right_dim = true;
            }
        }
    } else {
        GET_OPTIMIZED_FACTORIZATION(factor, right_dim)
        if ((factor > 0) && (factor != right_dim) && (fixed_dim < right_dim)) {
            use_right_dim = true;
        } else {
            GET_OPTIMIZED_FACTORIZATION(factor, left_dim)
            if ((factor > 0) && (factor != left_dim) && (fixed_dim == 1)) {
                use_left_dim = true;
            }
        }
    }
    /* Y(left_dim) and Z(right_dim) cannot be factored on the C
     * or X(fixed_dim) is greater than Y/Z (because the transpose may require too many instructions when X is large)
     * decompose X to N and C, Y to H, Z to W, do YZ transpose of HW
     */
    if ((use_left_dim == false && use_right_dim == false) ||
        (fixed_dim >= left_dim && fixed_dim >= right_dim)) {
        GET_OPTIMIZED_FACTORIZATION(factor, fixed_dim)
        if (factor > 0) {
            trans_info.N = 1; // fixed_dim / factor;
            trans_info.C = factor;
            trans_info.H = left_dim;
            trans_info.W = right_dim;
            min_transpose_size = trans_info.H * trans_info.W  * type_len;
            // cal whether the 1/2 local mem size can accommodate the min transpose size
            overflow = ((float)((float)min_transpose_size) / (float)(LOCAL_MEM_SIZE / 2));
            // cal the max trans counts
            trans_info.max_trans_counts = fixed_dim / factor;
        } else if (fixed_dim <= NPU_NUM) {
            trans_info.N = 1;
            trans_info.C = fixed_dim;
            trans_info.H = left_dim;
            trans_info.W = right_dim;
            min_transpose_size = trans_info.H * trans_info.W  * type_len;
            overflow = ((float)((float)min_transpose_size) / (float)(LOCAL_MEM_SIZE / 2));
            trans_info.max_trans_counts = 1;
        }
        trans_info.trans_method = TRANS_NPU_H_SWITCH_W;

    /* X is large and can be factored on the C
     * X is decomposed into C and H, Z is put on W
     * do YZ transpose of NW
     */
    } else if (use_left_dim) {
        trans_info.N = 1;
        trans_info.C = factor;
        trans_info.H = left_dim / factor;
        trans_info.W = right_dim;
        trans_info.trans_method = TRANS_NPU_N_SWITCH_W;
        min_transpose_size = trans_info.H * trans_info.W * type_len;
        overflow = ((float)((float)min_transpose_size) / (float)(LOCAL_MEM_SIZE / 2));
        trans_info.max_trans_counts = fixed_dim;
    /* Z is large and can be factored on the C
     * decompose Z onto the C and H, and put X onto the N
     * do YZ transpose of NW
     */
    } else if (use_right_dim) {
        trans_info.N = left_dim;
        trans_info.C = factor;
        trans_info.H = right_dim / factor;
        trans_info.W = 1;
        trans_info.trans_method = TRANS_NPU_N_SWITCH_W;
        min_transpose_size = trans_info.H * trans_info.N * type_len;
        overflow = ((float)((float)min_transpose_size) / (float)(LOCAL_MEM_SIZE / 2));
        trans_info.max_trans_counts = fixed_dim;
    }
    /* the min trans data of the TRANS_NPU_* causes the local mem to overflow
      use the TRANS_GENERAL */
    if (overflow > 1.0f || overflow == 0.f) {
        memset(&trans_info, 0, sizeof(trans_info));
        trans_info.trans_method = TRANS_GENERAL;
    }
    return trans_info;
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
    }
    memset(output, 0, num);

    int order[4];
    order[0] = 0;
    order[1] = 1;
    order[2] = 3;
    order[3] = 2;
    int raw_order[4];
    raw_order[0] = 0;
    raw_order[1] = 1;
    raw_order[2] = 2;
    raw_order[3] = 3;
    int fixed_dim = 1;
    int real_input_shape[4];
    real_input_shape[0] = 1;
    real_input_shape[1] = channel;
    real_input_shape[2] = height;
    real_input_shape[3] = width;

    int real_dims = 4;
    int left_dim = 1;
    int right_dim = 1;
    int tmp_order[4];

    int type_len = 1;
    int steps[4][4];
    int step_num = 0;

    int trans_total_times[4] = {0};
    int trans_method[4] = {0};
    for (int i = 0; i < real_dims; i++) {
        if (order[i] == raw_order[i]) {
            fixed_dim *= real_input_shape[order[i]];
            continue;
        }
        int pivot = 0;
        for (int j = i + 1; j < real_dims; j++) {
            if (order[i] == raw_order[j]) {
                pivot = j - i;
                break;
            }
        }

        left_dim = 1;
        right_dim = 1;
        for (int j = i; j < real_dims; j++) {
            if (j < pivot + i) {
                left_dim *= real_input_shape[raw_order[j]];
            } else {
                right_dim *= real_input_shape[raw_order[j]];
            }
            tmp_order[j] = (j + pivot < real_dims) ? raw_order[j + pivot] : raw_order[i + j + pivot - real_dims];
        }
        for (int j = i; j < real_dims; j++)
            raw_order[j] = tmp_order[j];
        if (left_dim != 1 && right_dim != 1) {
            trans_info_t trans_info = sg_get_transpose_info_(fixed_dim,
                                                          left_dim,
                                                          right_dim,
                                                          type_len);
            if (trans_info.trans_method == TRANS_GENERAL) {
                steps[step_num][0] = fixed_dim;
                steps[step_num][1] = left_dim;
                steps[step_num][2] = right_dim;
            } else {
                steps[step_num][0] = trans_info.N;
                steps[step_num][1] = trans_info.C;
                steps[step_num][2] = trans_info.H;
                steps[step_num][3] = trans_info.W;
                trans_total_times[step_num] = trans_info.max_trans_counts;
                trans_method[step_num] = trans_info.trans_method;
            }
            step_num++;
        }
        fixed_dim *= real_input_shape[order[i]];
    }
}
