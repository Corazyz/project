#include "stdio.h"
#include "stdlib.h"

typedef struct bmcv_border {
    int border_num;
    int width;
    int height;
} bmcv_border;

int main() {
    int rect_num = 5;
    int draw_num = 0;
    draw_num = (rect_num >> 2) + 1;
    bmcv_border *border_cfg = (bmcv_border *)malloc(sizeof(bmcv_border) * draw_num);
    for (int j = 0; j < draw_num; j++) {
        int draw_num_current = (j == (draw_num - 1)) ? (rect_num % 4) : 4;
        border_cfg[j].border_num = draw_num_current;
        for (int k = 0; k < draw_num_current; k++) {
            int draw_idx = ((j << 2) + k);
            printf("rect_num = %d, draw_num = %d, draw_num_current = %d, border_num = %d, draw_idx = %d\n", rect_num, draw_num, draw_num_current, border_cfg[j].border_num, draw_idx);
        }
    }
    return 0;
}