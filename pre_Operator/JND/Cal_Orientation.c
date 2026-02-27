#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"
#include "time.h"
#include "stdbool.h"

# define M_PI		3.14159265358979323846	/* pi */
# define QUANT_STEP 12
# define NUM_BINS 30
#define SCALE_FACTOR_TAN 256
#define TAN_INF          1e6
#define TAN_INF_FIXED    ((int16_t)(TAN_INF * SCALE_FACTOR_TAN))
short tan_edges_fixed[NUM_BINS + 1];


void InitOrientationLUT() {
    for (int i = 0; i <= NUM_BINS; i++) {
        float angle_deg = i * QUANT_STEP;
        if (fabsf(angle_deg - 90.0f) < 0.1f || fabsf(angle_deg - 270.0f) < 0.1f) {
            tan_edges_fixed[i] = TAN_INF_FIXED;
        } else {
            float angle_mod = fmodf(angle_deg, 180.0f);
            float tan_val = tanf(angle_mod * M_PI / 180.0f);
            tan_edges_fixed[i] = (short)(tan_val * SCALE_FACTOR_TAN);
        }
    }
    tan_edges_fixed[NUM_BINS] = tan_edges_fixed[0];
}

int run_sample(float Gv, float Gh, const char* tag) {
    // asin
    float angle_deg;
    float angle_rad;
    float abs_Gv = fabsf(Gv);

    float abs_Gh = fabsf(Gh);
    float mid_v = Gv * Gv;
    float mid_h = Gh * Gh;
    float mid_s = mid_v + mid_h;
    float hypotenuse = sqrtf(Gv * Gv + Gh * Gh);   // 计算斜边
    float mid_d = abs_Gv / hypotenuse;

    // float angle_rad = asinf(fabsf(Gv) / hypotenuse);  // 使用绝对值计算锐角
    if (hypotenuse == 0.0f) {
        angle_rad = 0.0f;  // 处理除零情况
    } else {
        angle_rad = asinf(abs_Gv / hypotenuse);
    }

    angle_deg = angle_rad * (180.0f / M_PI);  // 转换为角度

    // 根据 Gh 的符号调整角度范围
    if (Gh < 0) {
        angle_deg = 180.0f - angle_deg;  // 第二或第三象限
    }
    // 根据 Gv 的符号调整最终角度
        if (Gv < 0) {
            angle_deg = 360.0f - angle_deg;  // 第三或第四象限
        }
    // int sin_bin = (int)((angle_deg + 0.4999) / QUANT_STEP) % NUM_BINS;
    int sin_bin = (int)((angle_deg) / QUANT_STEP) % NUM_BINS;

    // atan
    float Orient = 0;
    Orient = atan2f(Gv, Gh);
    Orient = Orient * 180.0f / M_PI;
    if (Orient < 0.0f) {
        Orient += 360.0f;
    }
    int tan_bin = ((int)(Orient + 0.4999) / QUANT_STEP) % NUM_BINS;

    if (fabsf(Gh) < 1e-5) {
        return (Gv > 0) ? (90 / QUANT_STEP) : (270 / QUANT_STEP);
    }
    float ratio = Gv / Gh;
    short ratio_fixed = (short)(ratio * SCALE_FACTOR_TAN);

    int gh_sign = (Gh > 0) ? 1 : -1;
    int gv_sign = (Gv > 0) ? 1 : -1;

    int start, end;

    if (gh_sign > 0 && gv_sign > 0) {       //bin0 ~ bin7
        start = 0; end = 7;
    } else if (gh_sign < 0 && gv_sign > 0) { //bin8 ~ bin14
        start = 8; end = 14;
    } else if (gh_sign < 0 && gv_sign < 0) { //bin15 ~ bin22
        start = 15; end = 22;
    } else {                                //bin23 ~ bin29
        start = 23; end = 29;
    }

    bool positive_quadrant = (gh_sign > 0 && gv_sign > 0) || (gh_sign < 0 && gv_sign < 0);

    // for (int bin = start; bin <= end; bin++) {
    //     short lower = tan_edges_fixed[bin];
    //     short upper = tan_edges_fixed[bin + 1];
    //     bool in_bin;
    //     if (positive_quadrant) {
    //         in_bin = (ratio_fixed >= lower) && (ratio_fixed < upper);
    //     } else {
    //         in_bin = (ratio_fixed <= lower) && (ratio_fixed > upper);
    //     }
    //     if (bin == 7 || bin == 22) {
    //         if (positive_quadrant) {
    //             in_bin = ratio_fixed >= lower;
    //         } else {
    //             in_bin = ratio_fixed <= upper;
    //         }
    //     }
    //     if (in_bin) return bin;
    // }

    // int tan_bin = ((int)Orient / QUANT_STEP) % NUM_BINS;
    // printf("sin_bin = %d, tan_bin = %d", sin_bin, tan_bin);
    if (tan_bin != sin_bin) {
        printf("Gv = %f, Gh = %f, sin_bin = %d, tan_bin = %d\n", Gv, Gh, sin_bin, tan_bin);
        return 1;
    }
    return 0;
}

int main() {
    float Gv = 1;
    float Gh = 0;
    InitOrientationLUT();
    run_sample(-1.43051147e-06f, 43.3333321f, "+Gh axis (0°)");
    srand((unsigned)time(NULL));

    // 1) 零与纯轴向
    run_sample(0.0f, 0.0f, "zero");
    run_sample(1.0f, 0.0f, "+Gv axis (90°)");
    run_sample(-1.0f, 0.0f, "-Gv axis (270°)");
    run_sample(0.0f, 1.0f, "+Gh axis (0°)");
    run_sample(0.0f, -1.0f, "-Gh axis (180°)");

    // 2) 标准象限内典型角度
    run_sample((float)sqrt(3.0), 1.0f, "~60°");
    run_sample(1.0f, (float)sqrt(3.0), "~30°");
    run_sample(1.0f, 1.0f, "45°");
    run_sample(-1.0f, 1.0f, "135°");
    run_sample(-1.0f, -1.0f, "225°");
    run_sample(1.0f, -1.0f, "315°");

    // 3) 接近轴与数值稳定性
    run_sample(1e-7f, 1.0f, "near 0°");
    run_sample(1.0f, 1e-7f, "near 90°");
    run_sample(-1e-7f, -1.0f, "near 180°");
    run_sample(-1.0f, -1e-7f, "near 270°");
    run_sample(1e-7f, -1.0f, "near 360°");

    // 4) 象限边界附近微扰
    run_sample(1.0f, 1.0f + 1e-6f, "<45°");
    run_sample(1.0f + 1e-6f, 1.0f, ">45°");
    run_sample(-1.0f, 1.0f + 1e-6f, "<135°");
    run_sample(-1.0f - 1e-6f, 1.0f, ">135°");
    run_sample(-1.0f, -1.0f - 1e-6f, ">225°");
    run_sample(-1.0f - 1e-6f, -1.0f, "<225°");
    run_sample(1.0f, -1.0f - 1e-6f, ">315°");
    run_sample(1.0f + 1e-6f, -1.0f, "<315°");

    // 5) 缩放不变性
    run_sample(2.0f, 2.0f, "45°");
    run_sample(200.0f, 200.0f, "45° (scaled)");
    run_sample(-3.0f, 6.0f, "atan2≈333.435°");
    run_sample(6.0f, -3.0f, "atan2≈116.565°");

    int N = 200; // 可调整
    int count = 0;
    for (int i = 0; i < N; ++i) {
        // 范围：[-10, 10]
        Gv = ((float)rand() / RAND_MAX) * 20.0f - 10.0f;
        Gh = ((float)rand() / RAND_MAX) * 20.0f - 10.0f;
        // 避免 (0,0)
        if (fabsf(Gv) < 1e-12f && fabsf(Gh) < 1e-12f) Gh = 1e-6f;
        int ret = run_sample(Gv, Gh, "rand");
        if (ret != 0) {
            printf("Gv = %f, Gh = %f\n", Gv, Gh);
        }
        count += ret;
    }
    printf("diff count = %d\n", count);
    return 0;
}