#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define FS 128
#define DURATION 9
#define N (FS * DURATION)
#define PI 3.14159265358979323846

static void dft(float* in_real, float* in_imag, float* output_real,
               float* output_imag, int length, bool forward);

void generate_signal(float *t, float *x) {
    for (int i = 0; i < N; i++) {
        t[i] = (float)i / FS;
        if (i < N/4) {
            x[i] = sin(2 * PI * 5 * t[i]);
        } else if (i < N / 2) {
            x[i] = 0.5 * sin(2 * PI * 10 * t[i]);
        } else if (i < 3 * N / 4) {
            x[i] = 0.7 * sin(2 * PI * 15 * t[i]);
        } else {
            x[i] = 1.2 * sin(2 * PI * 20 * t[i]);
        }
    }
}

void save_time_domain(float *t, float *x) {
    FILE *fp = fopen("stft_time_domain.dat", "w");
    if (!fp) {
        perror("Error opening time_domain.dat");
        return;
    }
    fprintf(fp, "# Time(sec)\tAmplitude\n");
    for (int i = 0; i < N; i++) {
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

void save_windowed_data(float *t, float *windowed_x, float *w_padded,
                         float *freq, float *X, int n, int w_pos_sec) {
    char filename[100];

    // Save time domain windowed data
    snprintf(filename, sizeof(filename), "stft_windowed_time_pos_%d_sec.dat", w_pos_sec);
    FILE *fp_time = fopen(filename, "w");
    if (fp_time) {
        fprintf(fp_time, "# Time(sec)\tSignal\tWindow\n");
        for (int i = 0; i < 2*n; i++) {
            fprintf(fp_time, "%f\t%f\t%f\n", t[i], windowed_x[i], w_padded[i]);
        }
        fclose(fp_time);
    }

    // Save frequency domain data
    snprintf(filename, sizeof(filename), "stft_windowed_freq_pos_%d_sec.dat", w_pos_sec);
    FILE *fp_freq = fopen(filename, "w");
    if (fp_freq) {
        fprintf(fp_freq, "# Frequency(Hz)\tMagnitude\n");
        for (int i = 0; i < n; i++) {
            fprintf(fp_freq, "%f\t%f\n", freq[i], X[i]);
        }
        fclose(fp_freq);
    }
}

void mystft(float *t, float *x, int x_len, int w_len, int hop, int nfft, int fs) {
    int L = 1 + (x_len - w_len) / hop;
    float *freq = (float *)malloc(nfft * sizeof(float));
    float *t_sec = (float *)malloc(L * sizeof(float));

    // Calculate frequency vector
    // for (int i = 0; i < nfft; i++) {
    //     freq[i] = (i - nfft / 2) * (fs / nfft);
    // }
    // Calculate time vector
    for (int l = 0; l < L; l++) {
        t_sec[l] = (w_len / 2.0f + l * hop) / fs;
    }

    // Create Hann window
    float *w_padded = (float *)calloc(w_len, sizeof(float));
    for (int i = 0; i < w_len; i++) {
        w_padded[i] = 0.5 * (1 - cos(2 * PI * i / (w_len - 1)));
    }
    for (int l = 0; l < L; l++) {
        float *wx = (float *)malloc(w_len * sizeof(float));
        for (int i = 0; i < w_len; i++) {
            wx[i] = x[l * hop + i] * w_padded[i];
        }
        // Zero-padding
        float *wx_padded = (float *)calloc(nfft, sizeof(float));
        for (int i = 0; i < w_len; i++) {
            wx_padded[i] = wx[i];
        }

        // Compute FFt
        float *in_imag = (float *)calloc(w_len, sizeof(float));
        float *out_real = (float *)malloc(w_len * sizeof(float));
        float *out_imag = (float *)malloc(w_len * sizeof(float));
        dft(wx_padded, in_imag, out_real, out_imag, w_len, true);

        float *X = (float *)malloc((w_len/2) * sizeof(float));
        for (int i = 0; i < w_len/2; i++) {
            X[i] = sqrtf(out_real[i] * out_real[i] + out_imag[i] * out_imag[i]) / FS;
            freq[i] = (float)i * FS / w_len;
        }

        // Save data to files
        save_windowed_data(t, wx_padded, w_padded, freq, X, w_len/2, l);
        free(wx);
        free(wx_padded);
        free(in_imag);
        free(out_real);
        free(out_imag);
    }

    free(t_sec);
    free(freq);
    free(w_padded);
}

int main() {
    float t[N];
    float x[N];

    // Generate signal
    generate_signal(t, x);

    // Save time domain signal
    save_time_domain(t, x);

    // Compute and save frequency domain
    float *in_imag = (float *)calloc(N, sizeof(float));
    float *out_real = (float *)malloc(N * sizeof(float));
    float *out_imag = (float *)malloc(N * sizeof(float));
    dft(x, in_imag, out_real, out_imag, N, true);

    float *X = (float *)malloc((N) * sizeof(float));
    float *freq = (float *)malloc((N) * sizeof(float));
    for (int i = 0; i < N; i++) {
        X[i] = sqrtf(out_real[i] * out_real[i] + out_imag[i] * out_imag[i]) / FS;
        freq[i] = (float)i * FS / N;
    }

    printf("N = %d\n", N);
    printf("freq[1152] = %f\n", freq[1151]);
    save_frequency_domain(freq, X, N);

    // Perform windowed analysis
    mystft(t, x, N, 128, 64, 128, 128);
    // Clean up
    free(in_imag);
    free(out_imag);
    free(out_real);
    free(X);
    free(freq);

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

// static void fft(float* in_real, float* in_imag, float* output_real, float* output_imag, int length, int step, bool forward) {
//     int i;

//     if (length == 1) {
//         output_real[0] = in_real[0];
//         output_imag[0] = in_imag[0];
//         return;
//     }

//     fft(in_real, in_imag, output_real, output_imag, length / 2, 2 * step, forward);
//     fft(in_real + step, in_imag + step, output_real + length / 2, output_imag + length / 2, length / 2, 2 * step, forward);

//     for (i = 0; i < length / 2; ++i) {
//         float angle = forward ? -2 * PI * i / length : 2 * PI * i / length;
//         float wr = cosf(angle);
//         float wi = sinf(angle);

//         float tr = wr * output_real[i + length / 2] - wi * output_imag[i + length / 2];
//         float ti = wr * output_imag[i + length / 2] + wi * output_real[i + length / 2];

//         output_real[i + length / 2] = output_real[i] - tr;
//         output_imag[i + length / 2] = output_imag[i] - ti;
//         output_real[i] += tr;
//         output_imag[i] += ti;
//     }
// }
