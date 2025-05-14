#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <complex.h>
#include <string.h>

# define PI		3.14159265358979323846

static void dft(float* in_real, float* in_imag, float* output_real, float* output_imag, int length, bool forward);

void save_time_domain(float *t, float *x, int x_len) {
    FILE *fp = fopen("stft_time_domain.dat", "w");
    if (!fp) {
        perror("Error opening time_domain.dat");
        return;
    }
    fprintf(fp, "# Time(sec)\tAmplitude\n");
    for (int i = 0; i < x_len; i++) {
        fprintf(fp, "%f\t%f\n", t[i], x[i]);
    }
    fclose(fp);
}

void save_frequency_domain(float *freq, float *X, int n) {
    FILE *fp = fopen("stft_frequency_domain.dat", "w");
    if (!fp) {
        perror("Error opening frequency_domain.dat");
        return;
    }
    fprintf(fp, "# Frequency(Hz)\tMagnitude\n");
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%f\t%f\n", freq[i], X[i]);
    }
    fclose(fp);
}

int main() {
    int xlen = 256;
    int fs = 32;
    float *t = (float*)malloc(xlen * sizeof(float));

    float *in_real = (float *)malloc(xlen * sizeof(float));
    float *in_imag = (float *)calloc(xlen, sizeof(float));
    float *out_real = (float *)malloc(xlen * sizeof(float));
    float *out_imag = (float *)malloc(xlen * sizeof(float));

    for (int i = 0; i < xlen; i++) {
        t[i] = (float)i / fs;
        if (i < xlen/4) in_real[i] = sin(2 * PI * 5 * t[i]);
        else in_real[i] = sin(2 * PI * 10 * t[i]);
    }
    save_time_domain(t, in_real, xlen);

    dft(in_real, in_imag, out_real, out_imag, xlen, true);
    float *X = (float *)malloc(xlen/2 * sizeof(float));
    float *freq = (float *)malloc(xlen/2 * sizeof(float));
    for (int i = 0; i < xlen/2; i++) {
        X[i] = sqrtf(out_real[i] * out_real[i] + out_imag[i] * out_imag[i]) / fs;
        freq[i] = (float)i * fs / xlen;
    }
    save_frequency_domain(freq, X, xlen/2);

    // Clean up
    free(t);
    free(in_real);
    free(in_imag);
    free(out_real);
    free(out_imag);
    free(X);
    free(freq);
    return 0;
}

static void dft(float* in_real, float* in_imag, float* output_real,
                float* output_imag, int length, bool forward)
{
    int i, j;
    double angle;

    for (i = 0; i < length; ++i) {
        output_real[i] = 0.f;
        output_imag[i] = 0.f;
        for (j = 0; j < length; ++j) {
            angle = (forward ? -2.0 : 2.0) * PI * i * j / length;
            output_real[i] += in_real[j] * cos(angle) - in_imag[j] * sin(angle);
            output_imag[i] += in_real[j] * sin(angle) + in_imag[j] * cos(angle);
        }
    }
}