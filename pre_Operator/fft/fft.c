#include "stdio.h"
#include "math.h"
#include <stdlib.h>

# define M_PI		3.14159265358979323846	/* pi */
typedef struct {
    double real;
    double imag;
} Complex;

void show(Complex C) {
    if (fabs(C.real) < 0.0001) C.real = 0;
    if (fabs(C.imag) < 0.0001) C.imag = 0;

    if (C.imag > 0) {
        printf("%.4f + %.4fi\n", C.real, C.imag);
    } else if (C.imag == 0) {
        printf("%.4f\n", C.real);
    } else {
        printf("%.4f %.4fi\n", C.real, C.imag);
    }
}

Complex* calDFT(float* f, int N) {
    Complex *F = (Complex*)malloc(N * sizeof(Complex));
    if (F == NULL) {
        printf("Memory allocation failed.\n");
        return NULL;
    }
    for (int n = 0; n < N; n++) {
        F[n].real = 0;
        F[n].imag = 0;

        for (int t = 0; t < N; t++) {
            float angle = -2.0 * M_PI * t * n / N;
            float cos_val = cos(angle);
            float sin_val = sin(angle);

            F[n].real += f[t] * cos_val;
            F[n].imag += f[t] * sin_val;
        }
    }
    return F;
}

float* calIDFT(Complex* F, int N) {
    float* f = (float*)malloc(N * sizeof(float));
    if (f == NULL) {
        printf("Memory allocation failed.\n");
        return NULL;
    }
    for (int t = 0; t < N; t++) {
        f[t] = 0;

        for (int n = 0; n < N; n++) {
            float angle = 2.0 * M_PI * t * n / N;  // Note the positive sign for IDFT
            float cos_val = cos(angle);
            float sin_val = sin(angle);

            f[t] += F[n].real * cos_val - F[n].imag * sin_val;
        }

        f[t] /= N;  // Normalize by dividing by N
    }
    return f;
}
int main() {
    // Complex c1 = {3.0, 4.0};
    // Complex c2 = {3.0, -4.0};
    // Complex c3 = {3.0, 0.0};
    // Complex c4 = {0.0, 0.0};

    // float in_signal[100];
    // float t = M_PI / 50;
    // printf("t = %f\n", t);
    // for (int i = 0; i < 100; i++) {
    //     in_signal[i] = sin(i * t)+ sin(3 * i * t);
    //     printf("in_signal[%d] = %f,\n", i, in_signal[i]);
    // }
    // for (int i = 0; i < 50; i++) {
    //     in_signal[i + 50] = in_signal[i];
    //     printf("in_signal[%d] = %f\n", i+50, in_signal[i+50]);
    // }
    int N = 400;
    float in_signal[N];
    for (int i = 0; i < N; i++) {
        if (i < N / 4) {
            in_signal[i] = 10 * sin(2 * M_PI * 10 * i / N);
        } else if (i < N / 2) {
            in_signal[i] = 20 * sin(2 * M_PI * 20 * i / N);
        } else if (i < N * 3 / 4) {
            in_signal[i] = 30 * sin(2 * M_PI * 30 * i / N);
        } else {
            in_signal[i] = 40 * sin(2 * M_PI * 40 * i / N);
        }
    }


    Complex *F;
    F = calDFT(in_signal, 400);
    float abs_F[400];
    for (int i = 0; i < 400; i++) {
        abs_F[i] = F[i].real * F[i].real + F[i].imag * F[i].imag;
        printf("abs_F[%d] = %f\n", i, abs_F[i]);
    }

    FILE* file = fopen("abs_F_output.txt", "w");
    if (file == NULL) {
        printf("Error opening file!\n");
        return 1;
    }
    for (int i = 0; i < 400; i++) {
        fprintf(file, "%d %f\n", i, abs_F[i]);
    }
    fclose(file);

    // float *f;
    // f = calIDFT(F, 100);
    // FILE* file1 = fopen("f_output.txt", "w");
    // if (file1 == NULL) {
    //     printf("Error opening file!\n");
    //     return 1;
    // }
    // for (int i = 0; i < 100; i++) {
    //     fprintf(file1, "%d %f\n", i, f[i]);
    // }
    // fclose(file1);

    printf("Data written to abs_F_output.txt\n");
    return 0;
}