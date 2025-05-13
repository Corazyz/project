#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>
// #include <stdbool.h>

#define FS 128
#define DURATION 9
#define N (FS * DURATION)
#define PI 3.14159265358979323846

void generate_signal(double *t, double *x) {
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

void save_time_domain(double *t, double *x) {
    FILE *fp = fopen("time_domain.dat", "w");
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

void save_frequency_domain(double *freq, double *X, int n) {
    FILE *fp = fopen("frequency_domain.dat", "w");
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

void save_windowed_data(double *t, double *windowed_x, double *w_padded,
                       double *freq, double *X, int n, int w_pos_sec) {
    char filename[100];

    // Save time domain windowed data
    snprintf(filename, sizeof(filename), "windowed_time_pos_%d_sec.dat", w_pos_sec);
    FILE *fp_time = fopen(filename, "w");
    if (fp_time) {
        fprintf(fp_time, "# Time(sec)\tSignal\tWindow\n");
        for (int i = 0; i < 2*n; i++) {
            fprintf(fp_time, "%f\t%f\t%f\n", t[i], windowed_x[i], w_padded[i]);
        }
        fclose(fp_time);
    }

    // Save frequency domain data
    snprintf(filename, sizeof(filename), "windowed_freq_pos_%d_sec.dat", w_pos_sec);
    FILE *fp_freq = fopen(filename, "w");
    if (fp_freq) {
        fprintf(fp_freq, "# Frequency(Hz)\tMagnitude\n");
        for (int i = 0; i < n; i++) {
            fprintf(fp_freq, "%f\t%f\n", freq[i], X[i]);
        }
        fclose(fp_freq);
    }
}

void windowed_ft(double *t, double *x, int w_pos_sec, int w_len) {
    // Apply Hann window
    double *windowed_x = (double *)calloc(w_len, sizeof(double));
    double *w_padded = (double *)calloc(w_len, sizeof(double));

    int w_pos = FS * w_pos_sec;

    // Create Hann window
    for (int i = 0; i < w_len; i++) {
        if (w_pos + i < N) {
            w_padded[i] = 0.5 * (1 - cos(2 * PI * i / (w_len - 1)));
            windowed_x[i] = x[w_pos + i] * w_padded[i];
        }
    }

    // Compute FFT
    fftw_complex *in = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * w_len);
    fftw_complex *out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * w_len);
    fftw_plan plan = fftw_plan_dft_1d(w_len, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    for (int i = 0; i < w_len; i++) {
        in[i] = windowed_x[i] + 0.0 * I;
    }

    fftw_execute(plan);

    // Calculate magnitude spectrum
    double *X = (double *)malloc((w_len/2) * sizeof(double));
    double *freq = (double *)malloc((w_len/2) * sizeof(double));

    for (int i = 0; i < w_len/2; i++) {
        X[i] = cabs(out[i]) / FS;
        freq[i] = (double)i * FS / w_len;
    }

    // Save data to files
    save_windowed_data(t, windowed_x, w_padded, freq, X, w_len/2, w_pos_sec);

    // Clean up
    free(windowed_x);
    free(w_padded);
    free(X);
    free(freq);
}

int main() {
    double t[N];
    double x[N];
    printf("7/2 = %d", 7/2);
    // Generate signal
    generate_signal(t, x);

    // Save time domain signal
    save_time_domain(t, x);

    // Compute and save frequency domain
    fftw_complex *in = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
    fftw_plan plan = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    for (int i = 0; i < N; i++) {
        in[i] = x[i] + 0.0 * I;
    }

    fftw_execute(plan);

    double *X = (double *)malloc((N/2) * sizeof(double));
    double *freq = (double *)malloc((N/2) * sizeof(double));

    for (int i = 0; i < N/2; i++) {
        X[i] = cabs(out[i]) / FS;
        freq[i] = (double)i * FS / N;
    }

    save_frequency_domain(freq, X, N/2);

    // Perform windowed analysis
    windowed_ft(t, x, 1, FS);
    windowed_ft(t, x, 2, FS);
    windowed_ft(t, x, 3, FS);
    windowed_ft(t, x, 4, FS);
    windowed_ft(t, x, 5, FS);
    windowed_ft(t, x, 6, FS);
    windowed_ft(t, x, 7, FS);
    // windowed_ft(t, x, 2.5, FS);

    // Clean up
    free(X);
    free(freq);

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

// static void fft_rec(const float *in_r, const float *in_i,
//                     float       *out_r, float       *out_i,
//                     int n, int stride, bool inverse)
// {
//     if (n == 1) {                         /* 递归基 */
//         out_r[0] = in_r[0];
//         out_i[0] = in_i[0];
//         return;
//     }

//     int half = n >> 1;

//     /* 偶数下标 → out[0 .. half-1]，奇数下标 → out[half .. n-1] */
//     fft_rec(in_r,           in_i,
//             out_r,          out_i,
//             half, stride << 1, inverse);
//     fft_rec(in_r + stride,  in_i + stride,
//             out_r + half,   out_i + half,
//             half, stride << 1, inverse);

//     /* 合并蝶形 */
//     const float ang_sign = inverse ? 2.0f : -2.0f;
//     for (int k = 0; k < half; ++k) {
//         float ang  = ang_sign * PI * k / n;
//         float w_r  = cosf(ang);
//         float w_i  = sinf(ang);

//         /*  t = W_N^k * out[half + k]  */
//         float t_r  = w_r * out_r[half + k] - w_i * out_i[half + k];
//         float t_i  = w_r * out_i[half + k] + w_i * out_r[half + k];

//         float u_r  = out_r[k];
//         float u_i  = out_i[k];

//         /*  butterfly */
//         out_r[k]        = u_r + t_r;
//         out_i[k]        = u_i + t_i;
//         out_r[half + k] = u_r - t_r;
//         out_i[half + k] = u_i - t_i;
//     }
// }

// /* -----------对外包装-------------- */
// static int fft(const float *in_r, const float *in_i,
//                    float *out_r, float *out_i,
//                    int n, bool inverse)
// {
//     /* 简易合法性检查：必须是 2 的幂 */
//     if (n <= 0 || (n & (n - 1)))
//         printf("invalid length\n");
//         return -1;

//     fft_rec(in_r, in_i, out_r, out_i, n, 1, inverse);

//     if (inverse) {
//         float scale = 1.0f / n;
//         for (int i = 0; i < n; ++i) {
//             out_r[i] *= scale;
//             out_i[i] *= scale;
//         }
//     }
//     return 0;
// }