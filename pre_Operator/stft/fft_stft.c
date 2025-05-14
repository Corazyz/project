#include "stdio.h"
#include "math.h"
#include <stdlib.h>
#include <stdbool.h>
#include <fftw3.h>

# define M_PI		3.14159265358979323846	/* pi */
typedef struct {
    float real;
    float imag;
} Complex;

static void fft(float* in_real, float* in_imag, float* output_real, float* output_imag, int length, int step, bool forward);

void mystft(float *x, int xlen, float *win, int wLen, int hop, int nfft, float fs, Complex **STFT, float *f, float *t);
// Complex* calDFT(float* f, int N) {
//     Complex *F = (Complex*)malloc(N * sizeof(Complex));
//     if (F == NULL) {
//         printf("Memory allocation failed.\n");
//         return NULL;
//     }
//     for (int n = 0; n < N; n++) {
//         F[n].real = 0;
//         F[n].imag = 0;

//         for (int t = 0; t < N; t++) {
//             float angle = -2.0 * M_PI * t * n / N;
//             float cos_val = cos(angle);
//             float sin_val = sin(angle);

//             F[n].real += f[t] * cos_val;
//             F[n].imag += f[t] * sin_val;
//         }
//     }
//     return F;
// }

// float* calIDFT(Complex* F, int N) {
//     float* f = (float*)malloc(N * sizeof(float));
//     if (f == NULL) {
//         printf("Memory allocation failed.\n");
//         return NULL;
//     }
//     for (int t = 0; t < N; t++) {
//         f[t] = 0;

//         for (int n = 0; n < N; n++) {
//             float angle = 2.0 * M_PI * t * n / N;  // Note the positive sign for IDFT
//             float cos_val = cos(angle);
//             float sin_val = sin(angle);

//             f[t] += F[n].real * cos_val - F[n].imag * sin_val;
//         }

//         f[t] /= N;  // Normalize by dividing by N
//     }
//     return f;
// }
int main() {
    // int xlen = 400;
    // float in_signal[xlen];
    // for (int i = 0; i < xlen; i++) {
    //     if (i < xlen / 4) {
    //         in_signal[i] = 10 * sin(2 * M_PI * 5 * i * 4 / xlen);
    //     } else if (i < xlen / 2) {
    //         in_signal[i] = 20 * sin(2 * M_PI * 10 * i * 4 / xlen);
    //     } else if (i < xlen * 3 / 4) {
    //         in_signal[i] = 30 * sin(2 * M_PI * 15 * i * 4 / xlen);
    //     } else {
    //         in_signal[i] = 40 * sin(2 * M_PI * 20 * i * 4 / xlen);
    //     }
    // }

    int T = 4;
    int fs = 128;
    int xlen = T * fs;
    float in_signal[xlen];
    printf("xlen = %d\n", xlen);
    // for (int i = 0; i < xlen; i++) {
    //     if (i < xlen / 4) {
    //         in_signal[i] = 10 * sin(2 * M_PI * 5 * i / fs);
    //     } else if (i < xlen / 2) {
    //         in_signal[i] = 20 * sin(2 * M_PI * 10 * i / fs);
    //     } else if (i < xlen * 3 / 4) {
    //         in_signal[i] = 30 * sin(2 * M_PI * 15 * i / fs);
    //     } else {
    //         in_signal[i] = 40 * sin(2 * M_PI * 20 * i / fs);
    //     }
    // }
    for (int i = 0; i < xlen; i++) {
        if (i < xlen / 4) {
            in_signal[i] = sin(2 * M_PI * 5 * i / fs);
        } else if (i < xlen / 2) {
            in_signal[i] = sin(2 * M_PI * 10 * i / fs);
        } else if (i < xlen * 3 / 4) {
            in_signal[i] = sin(2 * M_PI * 15 * i / fs);
        } else {
            in_signal[i] = sin(2 * M_PI * 20 * i / fs);
        }
    }

    FILE *signal_file = fopen("signal.dat", "w");
    for (int i = 0; i < xlen; i++) {
        fprintf(signal_file, "%d %f\n", i, in_signal[i]);
    }
    fclose(signal_file);
    float *in_imag = (float *)calloc(xlen, sizeof(float));
    float *out_real = (float *)malloc(xlen * sizeof(float));
    float *out_imag = (float *)malloc(xlen * sizeof(float));

    // fft(in_signal, in_imag, out_real, out_imag, xlen, 1, true);
    // fftw_plan plan = fftw_plan_dft_1d(xlen, NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_complex *X = (fftw_complex *)malloc(xlen * sizeof(fftw_complex));
    fftw_plan plan = fftw_plan_dft_r2c_1d(xlen, in_signal, X, FFTW_ESTIMATE);
    fftw_execute(plan);
    FILE* file = fopen("stft_fft_res.txt", "w");
    if (file == NULL) {
        printf("Error opening file!\n");
        return -1;
    }
    for (int i = 0; i < xlen; i++) {
        float magnitude = sqrt(X[i][0] * X[i][0] + X[i][1] * X[i][1]);
        fprintf(file, "%d %f\n", i/4.0f, magnitude);
        // printf("i/4 = %f", i/4);
    }
    fclose(file);
    fftw_destroy_plan(plan);

    int wlen = 128;
    int hop = 128;
    int nfft = 128;

    float *x = (float*)malloc(xlen * sizeof(float));
    float *win = (float*)malloc(wlen * sizeof(float));

    for (int i = 0; i < wlen; i++) {
        win[i] = 0.5 * (1 - cos(2 * M_PI * i / (wlen - 1)));
    }

    int L = 1 + (xlen - wlen) / hop;

    Complex **STFT = (Complex **)malloc(L * sizeof(Complex *));
    for (int i = 0; i < L; i++) {
        STFT[i] = (Complex *)malloc(nfft * sizeof(float));
    }

    float *f = (float *)malloc(nfft * sizeof(float));
    float *t = (float *)malloc(L * sizeof(float));
    // int fs = 1000;

    mystft(in_signal, xlen, win, wlen, hop, nfft, fs, STFT, f, t);

    // for (int i = 0; i < 10; i++) {
    //     printf("%f, ", STFT[0][i].real);
    // }
    // printf("\n");
    // for (int i = 0; i < 10; i++) {
    //     printf("%f, ", STFT[0][i].imag);
    // }
    // printf("\n");

    for (int l = 0; l < L; l++) {
        char filename[50];
        sprintf(filename, "abs_output_%d.txt", l);
        FILE* file = fopen(filename, "w");
        if (file == NULL) {
            printf("Error opening file!\n");
            return -1;
        }
        for (int i = 0; i < nfft; i++) {
            float magnitude = sqrt(STFT[l][i].real * STFT[l][i].real + STFT[l][i].imag * STFT[l][i].imag);
            fprintf(file, "%d %f\n", i/4, magnitude);
            // printf("i/4 = %f", i/4);
        }
        fclose(file);
    }
    return 0;
}

void mystft(float *x, int xlen, float *win, int wlen, int hop, int nfft, float fs, Complex **STFT, float *f, float *t) {
    int L = 1 + (xlen - wlen) / hop;

    for (int l = 0; l < L; l++) {
        float *xw = (float *)malloc(wlen * sizeof(float));
        for (int i = 0; i < wlen; i++) {
            xw[i] = x[l * hop + i] * win[i];
        }

        // Zero-padding
        float *xw_padded = (float *)calloc(nfft, sizeof(float));
        for (int i = 0; i < wlen; i++) {
            xw_padded[i] = xw[i];
        }

        // Prepare input/output arrays for FFT
        float *in_real = (float *)malloc(nfft * sizeof(float));
        float *in_imag = (float *)calloc(nfft, sizeof(float));
        float *out_real = (float *)malloc(nfft * sizeof(float));
        float *out_imag = (float *)malloc(nfft * sizeof(float));

        // Copy padded window to input
        for (int i = 0; i < nfft; i++) {
            in_real[i] = xw_padded[i];
        }

        // Perform FFT
        fft(in_real, in_imag, out_real, out_imag, nfft, 1, true);

        // Shift and store the result
        for (int i = 0; i < nfft; i++) {
            // int shifted_index = (i + nfft / 2) % nfft;
            STFT[l][i].real = out_real[i];
            STFT[l][i].imag = out_imag[i];
        }

    // // Calculate frequency vector
    // for (int i = 0; i < nfft; i++) {
    //     f[i] = (i - nfft / 2) * (fs / nfft);
    // }

    // // Calculate time vector
    // for (int l = 0; l < L; l++) {
    //     t[l] = (wlen / 2.0f + l * hop) / fs;
    // }

        free(xw);
        free(xw_padded);
        free(in_real);
        free(in_imag);
        free(out_real);
        free(out_imag);
    }
}

static void fft(float* in_real, float* in_imag, float* output_real, float* output_imag, int length, int step, bool forward) {
    int i;

    if (length == 1) {
        output_real[0] = in_real[0];
        output_imag[0] = in_imag[0];
        return;
    }

    fft(in_real, in_imag, output_real, output_imag, length / 2, 2 * step, forward);
    fft(in_real + step, in_imag + step, output_real + length / 2, output_imag + length / 2, length / 2, 2 * step, forward);

    for (i = 0; i < length / 2; ++i) {
        float angle = forward ? -2 * M_PI * i / length : 2 * M_PI * i / length;
        float wr = cosf(angle);
        float wi = sinf(angle);

        float tr = wr * output_real[i + length / 2] - wi * output_imag[i + length / 2];
        float ti = wr * output_imag[i + length / 2] + wi * output_real[i + length / 2];

        output_real[i + length / 2] = output_real[i] - tr;
        output_imag[i + length / 2] = output_imag[i] - ti;
        output_real[i] += tr;
        output_imag[i] += ti;
    }
}