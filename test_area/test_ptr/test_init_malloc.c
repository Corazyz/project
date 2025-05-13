#include "stdio.h"
#include "stdlib.h"

int main() {
    float *init_val = (float*)malloc(3*sizeof(float));
    for (int i = 0; i < 3; i++) {
        printf("initial value of init_val[%d] = %f\n", i, init_val[i]);
    }
    return 0;
}