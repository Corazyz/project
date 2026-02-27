#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"
#define NUM_BINS 30
#define QUANT_STEP 12
#define SCALE_FACTOR_TAN 256
#define TAN_INF          1e6
#define TAN_INF_FIXED    ((int16_t)(TAN_INF * SCALE_FACTOR_TAN))
typedef signed short int int16_t;
int16_t tan_edges_fixed[NUM_BINS + 1];
# define M_PI		3.14159265358979323846	/* pi */

void InitOrientationLUT() {
    for (int i = 0; i <= NUM_BINS; i++) {
        float angle_deg = i * QUANT_STEP;
        if (fabsf(angle_deg - 90.0f) < 0.1f || fabsf(angle_deg - 270.0f) < 0.1f) {
            tan_edges_fixed[i] = TAN_INF_FIXED;
        } else {
            float angle_mod = fmodf(angle_deg, 180.0f);
            printf("NUM_BINS: %d, angle_deg: %f, angle_mod: %f\n", i, angle_deg, angle_mod);
            float tan_val = tanf(angle_mod * M_PI / 180.0f);
            tan_edges_fixed[i] = (int16_t)(tan_val * SCALE_FACTOR_TAN);
        }
    }
    tan_edges_fixed[NUM_BINS] = tan_edges_fixed[0];
    float Gv = 0.0002;
    float Gh = 0.00002;
    float angle_deg = asinf(Gv/sqrtf(Gv*Gv+Gh*Gh));
    printf("angle_deg = %f\n", angle_deg * (180.0f / M_PI));
}

int main() {
    InitOrientationLUT();
    printf("TAN_INF_FIXED = %d\n", TAN_INF_FIXED);
    for (int i = 0; i < (NUM_BINS+1); i++) {
        printf("%d, ", tan_edges_fixed[i]);
        if ((i+1)%6==0) {
            printf("\n");
        }
    }
    printf("\n");
}