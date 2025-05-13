#include "stdio.h"


void main() {
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
    real_input_shape[1] = 3;
    real_input_shape[2] = 300;
    real_input_shape[3] = 500;

    int tmp_order[4];
    int real_dims = 4;
    for (int i = 0; i < 4; i++) {
        if (order[i] == raw_order[i]) {
            fixed_dim *= real_input_shape[order[i]];
            continue;
        }
        printf("i = %d\n", i);
        printf("fixed_dim = %d\n", fixed_dim);
        int pivot = 0;
        int count = 0;
        for (int j = i+1; j < real_dims; j++) {
            if (order[i] == raw_order[j]) {
                pivot = j - i;
                printf("pivot = %d\n", pivot);
                break;
            }
        }

        int left_dim = 1;
        int right_dim = 1;
        for (int j = i; j < real_dims; j++) {
            if (j < pivot + i) {
                left_dim *= real_input_shape[raw_order[j]];
                printf("if: j = %d, left_dim = %d\n", j, left_dim);
            } else {
                right_dim *= real_input_shape[raw_order[j]];
                printf("else: j = %d, right_dim = %d\n", j, right_dim);
            }
            tmp_order[j] = (j + pivot < real_dims) ? raw_order[j + pivot] : raw_order[i + j + pivot - real_dims];
            printf("tmp_order[%d] = %d\n", j, tmp_order[j]);
        }
    }
}
