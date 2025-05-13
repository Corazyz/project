#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fftw3.h>

void mystft(double *x, int xlen, double *win, int wLen, int hop, int nfft, double fs, fftw_complex **STFT, double *f, double *t);

int main() {
    int xlen = 1024;
    int wlen = 256;
    int hop = 128;
    int nfft = 512;
    double fs = 1000;

    double *x = (double*)malloc(xlen * sizeof(double));
    double *win = (double*)malloc(wlen * sizeof(double));
    for (int i = 0; i < xlen; i++) {
        x[i] = sin(2 * M_PI * 50 * i / fs);
    }

    for (int i = 50; i <= 150; i++) {
        x[i] += sin(2 * M_PI * 120 * i / fs); // 叠加 120 Hz 的信号
    }

    for (int i = 0; i < wlen; i++) {
        win[i] = 0.5 * (1 - cos(2 * M_PI * i / (wlen - 1)));
    }

    int L = 1 + (xlen - wlen) / hop;

    fftw_complex **STFT = (fftw_complex **)malloc(L * sizeof(fftw_complex *));
    for (int i = 0; i < L; i++) {
        STFT[i] = (fftw_complex *)malloc(nfft * sizeof(fftw_complex));
    }

    double *f = (double *)malloc(nfft * sizeof(double));
    double *t = (double *)malloc(L * sizeof(double));

    mystft(x, xlen, win, wlen, hop, nfft, fs, STFT, f, t);

    FILE *input_file = fopen("input_signal.txt", "w");
    for (int i = 0; i < xlen; i++) {
        fprintf(input_file, "%f\n", x[i]);
    }
    fclose(input_file);

    FILE *stft_file = fopen("stft_result.txt", "w");
    for (int l = 0; l < L; l++) {
        for (int i = 0; i < nfft; i++) {
            double magnitude = sqrt(STFT[l][i][0] * STFT[l][i][0] + STFT[l][i][1] * STFT[l][i][1]);
            fprintf(stft_file, "%f ", magnitude);
        }
        fprintf(stft_file, "\n");
    }
    fclose(stft_file);

    FILE *freq_file = fopen("frequency_vector.txt", "w");
    for (int i = 0; i < nfft; i++) {
        fprintf(freq_file, "%f\n", f[i]);
    }
    fclose(freq_file);

    FILE *time_file = fopen("time_vector.txt", "w");
    for (int i = 0; i < L; i++) {
        fprintf(time_file, "%f\n", t[i]);
    }
    fclose(time_file);

    printf("Finished STFT computation.\n");
    printf("freq array:\n");
    for (int i = 0; i < nfft; i++) {
        printf("%f ", f[i]);
    }
    printf("\n");
    printf("time array:\n");
    for (int i = 0; i < L; i++) {
        printf("%f ", t[i]);
    }
    printf("\n");
    free(x);
    free(win);
    for (int i = 0; i < L; i++) {
        free(STFT[i]);
    }
    free(STFT);
    free(f);
    free(t);

    return 0;
}

void mystft(double *x, int xlen, double *win, int wlen, int hop, int nfft, double fs, fftw_complex **STFT, double *f, double *t) {
    int L = 1 + (xlen - wlen) / hop;

    for (int i = 0; i < nfft; i++) {
        f[i] = (i - nfft / 2) * (fs / nfft);
    }

    for (int l = 0; l < L; l++) {
        t[l] = (wlen / 2.0 + l * hop) / fs;
    }

    fftw_plan plan = fftw_plan_dft_1d(nfft, NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE);

    for (int l = 0; l < L; l++) {
        double *xw = (double *)malloc(wlen * sizeof(double));
        for (int i = 0; i < wlen; i++) {
            xw[i] = x[l * hop + i] * win[i];
        }

        double *xw_padded = (double *)calloc(nfft, sizeof(double));
        for (int i = 0; i < wlen; i++) {
            xw_padded[i] = xw[i];
        }

        fftw_complex *X = (fftw_complex *)malloc(nfft * sizeof(fftw_complex));
        fftw_plan plan = fftw_plan_dft_r2c_1d(nfft, xw_padded, X, FFTW_ESTIMATE);
        fftw_execute(plan);

        for (int i = 0; i < nfft; i++) {
            int shifted_index = (i + nfft / 2) % nfft;
            STFT[l][i][0] = X[shifted_index][0];
            STFT[l][i][1] = X[shifted_index][1];
        }

        fftw_destroy_plan(plan);
        free(xw);
        free(xw_padded);
        free(X);
    }
    fftw_destroy_plan(plan);
}