#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <complex.h>
#include <string.h>

# define PI		3.14159265358979323846

typedef struct {
    float real;
    float imag;
} Complex;

enum bdry{
    zeros = 0,
    constant,
    even,
    odd,
    none,
};

enum win{
    HANN = 0,
    HAMM = 1,
};

static void dft(float* in_real, float* in_imag, float* output_real, float* output_imag, int length, bool forward);

// void mystft(float *x, int xlen, float *win, int wLen, int hop, int nfft, float fs, Complex **STFT, float *f, float *t, float *t_main);
void mystft(float *x, int xlen, int fs, int window, int nperseg, int noverlap, int nfft, bool return_onesided, int boundary, bool padded, Complex **STFT, float *f, float *t);
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

void save_windowed_data(float *t, float *windowed_x, float *w_padded,
                         float *freq, float *X, int w_len, int nfft, int w_pos_sec) {
    char filename[100];

    // Save time domain windowed data
    snprintf(filename, sizeof(filename), "stft_windowed_time_pos_%d_sec.dat", w_pos_sec);
    FILE *fp_time = fopen(filename, "w");
    if (fp_time) {
        fprintf(fp_time, "# Time(sec)\tSignal\tWindow\n");
        for (int i = 0; i < w_len; i++) {
            fprintf(fp_time, "%f\t%f\t%f\n", t[i], windowed_x[i], w_padded[i]);
        }
        fclose(fp_time);
    }

    // Save frequency domain data
    snprintf(filename, sizeof(filename), "stft_windowed_freq_pos_%d_sec.dat", w_pos_sec);
    FILE *fp_freq = fopen(filename, "w");
    if (fp_freq) {
        fprintf(fp_freq, "# Frequency(Hz)\tMagnitude\n");
        for (int i = 0; i < nfft; i++) {
            fprintf(fp_freq, "%f\t%f\n", freq[i], X[i]);
        }
        fclose(fp_freq);
    }
}

int main() {
    // int xlen = 1024;
    int fs = 200;
    int duration = 9;
    int xlen = fs * duration;
    int wlen = 256;
    int noverlap = 128;
    int hop = wlen - noverlap;
    int nfft = 300;
    int boundary = even;
    bool padded = false;

    float *x = (float*)malloc(xlen * sizeof(float));
    float *win = (float*)malloc(wlen * sizeof(float));
    float *t = (float*)malloc(xlen * sizeof(float));

    // 生成信号
    for (int i = 0; i < xlen; i++) {
        t[i] = (float)i / fs;
        if (i < xlen/4) {
            x[i] = sin(2 * PI * 5 * t[i]);
        } else if (i < xlen/2) {
            x[i] = sin(2 * PI * 10 * t[i]);
        } else if (i < 3*xlen/4) {
            x[i] = sin(2 * PI * 15 * t[i]);
        } else {
            x[i] = sin(2 * PI * 20 * t[i]);
        }
    }

    save_time_domain(t, x, xlen);

    int xlen_ = 0;
    if (boundary == zeros || boundary == constant || boundary == even || boundary == odd) {
        xlen_ = xlen + 2 * hop;
    } else {
        xlen_ = xlen;
    }
    int padded_len = 0;
    if (padded) {
        padded_len = (xlen_ - wlen) % hop == 0 ? 0 : hop - (xlen_ - wlen) % hop;
    }
    xlen_ += padded_len;

    int L = 1 + (xlen_ - wlen) / hop;
    // int L = 1 + (xlen - wlen) / hop;

    Complex **STFT = (Complex **)malloc(L * sizeof(Complex *));
    for (int i = 0; i < L; i++) {
        STFT[i] = (Complex *)malloc(nfft * sizeof(Complex));
    }
    // Complex *STFT = (Complex *)malloc(xlen * sizeof(Complex));

    float *f = (float *)malloc(nfft * sizeof(float));
    float *t_sec = (float *)malloc(L * sizeof(float));
    for (int i = 0; i < wlen; i++) {
        win[i] = 0.5 * (1 - cos(2 * PI * i / (wlen - 1)));
    }
    // float *in_real = (float *)malloc(xlen * sizeof(float));
    float *in_imag = (float *)calloc(xlen, sizeof(float));
    float *out_real = (float *)malloc(xlen * sizeof(float));
    float *out_imag = (float *)malloc(xlen * sizeof(float));
    dft(x, in_imag, out_real, out_imag, xlen, true);

    float *X = (float *)malloc(xlen * sizeof(float));
    float *freq = (float *)malloc(xlen * sizeof(float));
    for (int i = 0; i < xlen; i++) {
        X[i] = sqrtf(out_real[i] * out_real[i] + out_imag[i] * out_imag[i]) / fs;
        freq[i] = (float)i * fs / xlen;
    }

    save_frequency_domain(freq, X, xlen);
    mystft(x, xlen, fs, HANN, wlen, noverlap, nfft, false, zeros, false, STFT, f, t_sec);

    FILE *input_file = fopen("input_signal.txt", "w");
    for (int i = 0; i < xlen; i++) {
        // fprintf(input_file, "%f %f\n", i/fs, x[i]);
        fprintf(input_file, "%f\n", x[i]);
    }
    fclose(input_file);

    // Save STFT magnitude (for visualization)
    FILE *stft_file = fopen("stft_magnitude.txt", "w");
    for (int l = 0; l < L; l++) {
        for (int i = 0; i < nfft; i++) {
            float magnitude = sqrtf(STFT[l][i].real * STFT[l][i].real +
                                  STFT[l][i].imag * STFT[l][i].imag);
            fprintf(stft_file, "%f ", magnitude);
        }
        fprintf(stft_file, "\n");
    }
    fclose(stft_file);

    // Save frequency vector
    FILE *freq_file = fopen("frequency_vector.txt", "w");
    for (int i = 0; i < nfft; i++) {
        fprintf(freq_file, "%f\n", f[i]);
    }
    fclose(freq_file);

    // Save time vector
    FILE *time_file = fopen("time_vector.txt", "w");
    for (int i = 0; i < L; i++) {
        fprintf(time_file, "%f\n", t_sec[i]);
    }
    fclose(time_file);

    printf("Data saved to files:\n");
    printf("- input_signal.txt\n");
    printf("- stft_magnitude.txt\n");
    printf("- frequency_vector.txt\n");
    printf("- time_vector.txt\n");

    // Clean up
    free(x);
    free(win);
    free(in_imag);
    free(out_imag);
    free(out_real);
    free(X);
    free(freq);
    for (int i = 0; i < L; i++) {
        free(STFT[i]);
    }
    free(STFT);
    free(t);
    free(f);
    free(t_sec);

    return 0;
}

void mystft(float *x, int xlen, int fs, int window, int nperseg, int noverlap, int nfft, bool return_onesided, int boundary, bool padded, Complex **STFT, float *f, float *t) {
    int hop = nperseg - noverlap;
    int xlen_ = 0;
    if (boundary == zeros || boundary == constant || boundary == even || boundary == odd) {
        xlen_ = xlen + 2 * hop;
    } else {
        xlen_ = xlen;
    }
    int padded_len = 0;
    if (padded) {
        padded_len = (xlen_ - nperseg) % hop == 0 ? 0 : hop - (xlen_ - nperseg) % hop;
    }
    xlen_ += padded_len;

    int L = 1 + (xlen_ - nperseg) / hop;

    float *x_ = (float *)malloc(xlen_ * sizeof(float));

    switch (boundary) {
    case zeros:
        // Zero-padding at both ends
        for (int i = 0; i < hop; i++) {
            x_[i] = 0.0f;                     // Left zero-padding
            x_[xlen_ - padded_len - hop + i] = 0.0f; // Right zero-padding
        }
        // Copy original signal to the center
        memcpy(x_ + hop, x, xlen * sizeof(float));
        break;
    case constant:
        for (int i = 0; i < hop; i++) {
            x_[i] = x[hop];
            x_[xlen_ - padded_len - hop + i] = x[xlen-1];
        }
        memcpy(x_ + hop, x, xlen * sizeof(float));
        break;
    case even:
        // Even symmetry padding (mirroring with repetition of last element)
        for (int i = 0; i < hop; i++) {
            // Left padding: mirror from start
            x_[i] = x[hop - 1 - i];
            // Right padding: mirror from end
            x_[xlen_ - padded_len - hop + i] = x[xlen - 1 - i];
        }
        // Copy original signal to the center
        memcpy(x_ + hop, x, xlen * sizeof(float));
        break;

    case odd:
        // Odd symmetry padding (mirroring with sign flip)
        for (int i = 0; i < hop; i++) {
            // Left padding: mirror with sign flip
            x_[i] = -x[hop - i];  // Note: different indexing for odd symmetry
            // Right padding: mirror with sign flip
            x_[xlen_ - padded_len - hop + i] = -x[xlen - 2 - i];
        }
        // Copy original signal to the center
        memcpy(x_ + hop, x, xlen * sizeof(float));
        break;
    case none:
        memcpy(x_, x, xlen * sizeof(float));
        break;
    default:
        printf("Unsupported boundary value! Use original signal.\n");
        memcpy(x_, x, xlen * sizeof(float));
        break;
    }

    for (int i = 0; i < padded_len; i++) {
        x_[xlen_ - padded_len + i] = 0;
    }
    for (int i = 0; i < nfft; i++) {
        f[i] = (i - nfft / 2) * ((float)fs / nfft);
    }

    for (int l = 0; l < L; l++) {
        t[l] = (nperseg / 2.0 + l * hop) / fs;
    }

    float* win = (float*)malloc(nperseg * sizeof(float));
    switch (window)
    {
    case HANN:
        for (int i = 0; i < nperseg; i++) {
            win[i] = 0.5 * (1 - cos(2 * PI * (float)i / (nperseg - 1)));
        }
        break;
    case HAMM:
        for (int i = 0; i < nperseg; i++) {
            win[i] = 0.54 - 0.46 * cos(2 * PI * (float)i / (nperseg - 1));
        }
        break;
    default:
        printf("Unsupported window type! Using default HANN window.\n");
        for (int i = 0; i < nperseg; i++) {
            win[i] = 0.5 * (1 - cos(2 * PI * (float)i / (nperseg - 1)));
        }
        break;
    }

    for (int l = 0; l < L; l++) {
        float *xw = (float *)malloc(nperseg * sizeof(float));
        for (int i = 0; i < nperseg; i++) {
            xw[i] = x_[l * hop + i] * win[i];
        }

        // Zero-padding
        float *xw_padded = (float *)calloc(nfft, sizeof(float));
        for (int i = 0; i < nperseg; i++) {
            xw_padded[i] = xw[i];
        }

        // Prepare input/output arrays for FFT
        float *in_real = (float *)calloc(nfft, sizeof(float));
        float *in_imag = (float *)calloc(nfft, sizeof(float));
        float *out_real = (float *)malloc(nfft * sizeof(float));
        float *out_imag = (float *)malloc(nfft * sizeof(float));

        // Copy padded window to input
        for (int i = 0; i < nfft; i++) {
            in_real[i] = xw_padded[i];
        }

        // Perform FFT

        dft(in_real, in_imag, out_real, out_imag, nfft, true);
        float *X = (float *)malloc((nfft/2) * sizeof(float));
        float *freq = (float *)malloc(nfft * sizeof(float));
        for (int i = 0; i < nfft/2; i++) {
            X[i] = sqrtf(out_real[i] * out_real[i] + out_imag[i] * out_imag[i]) / fs;
            freq[i] = (float)i * fs / nfft;
        }

        float *t_axes = (float *)malloc(nperseg * sizeof(float));
        for (int i = 0; i < nperseg; i++) {
            t_axes[i] = (nperseg / 2.0 + l * hop) / fs + (float)i / fs;
        }
        // // Save data to files
        save_windowed_data(t_axes, xw_padded, win, freq, X, nperseg, nfft/2, l);
        // Shift and store the result
        for (int i = 0; i < nfft; i++) {
            int shifted_index = (i + nfft / 2) % nfft;
            // printf("shifted_index = %d\n", shifted_index);
            STFT[l][shifted_index].real = out_real[i];
            STFT[l][shifted_index].imag = out_imag[i];
        }
        free(X);
        free(xw);
        free(xw_padded);
        free(in_real);
        free(in_imag);
        free(out_real);
        free(out_imag);
        free(freq);
        free(t_axes);
    }
    free(x_);
    free(win);
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