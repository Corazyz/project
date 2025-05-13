#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define M_PI 3.14159265358979323846

int main() {
    int L = 6;
    int *F = (int*)malloc(L*sizeof(int));
    int i = 0;

    while (L % 5 == 0) {
        F[i++] = 5;
        L /= 5;
    }
    while (L % 3 == 0) {
        F[i++] = 3;
        L /= 3;
    }

    while (L % 2 == 0) {
        F[i++] = 2;
        L /= 2;
    }

    if (i > 1) {
        if (F[i - 1] == 2 && F[i - 2] == 2) {
            i--;
            F[i - 1] = 4;
        }
    }
    L = 6;
    int factorSize = i;
    int* plan_factors = (int*)malloc(factorSize * sizeof(int));
    printf("factor_size = %d, L = %d\n", factorSize, L);
    float* ER = (float*)malloc(L * sizeof(float));
    float* EI = (float*)malloc(L * sizeof(float));

    int j = 0;
    int k = 0;
    int m = 0;
    i = 0;
    double s, ang;

    for (i = 0; i < factorSize; ++i) {
        plan_factors[j] = F[i];
        // j++;
        L /= F[i];
        printf("L = %d, plan_factor[%d] = %d\n", L, j, plan_factors[j]);
        j++;
    }

    for (i = factorSize - 1; i > 0; --i) {
        L *= F[i];
        printf("L = %d\n", L);
        s = 2.0 * M_PI / (L * F[i - 1]);
        printf("L * F[i - 1] = %d\n", L * F[i - 1]);
        float cos_value[L * F[i - 1]];
        float sin_value[L * F[i - 1]];
        for (k = 0; k < L * F[i - 1]; ++k) {
            ang = - s * k;
            cos_value[k] = cos(ang);
            sin_value[k] = sin(ang);
        }
        for (j = 1; j < F[i - 1]; ++j) {
            for (k = 0; k < L; ++k) {
                ER[m] = (cos_value[(k * j) % (L * F[i - 1])]);
                EI[m] = (sin_value[(k * j) % (L * F[i - 1])]);
                printf("ER[%d] = %f, EI[%d] = %f\n", m, ER[m], m, EI[m]);
                m++;
            }
        }
    }
}