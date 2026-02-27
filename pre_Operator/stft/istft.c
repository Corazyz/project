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

void normalize_fft(float* res_XR, float* res_XI, int frm_len, int num)
{
    float norm_fac;
    int i;

    norm_fac = 1.0f / frm_len;

    for (i = 0; i < num; ++i) {
        res_XR[i] *= norm_fac;
        res_XI[i] *= norm_fac;
    }
}

int cpu_fft(float* in_real, float* in_imag, float* output_real,
                float* output_imag, int length, int batch, bool forward)
{
    int i;

    for (i = 0; i < batch; ++i) {
        dft(&(in_real[i * length]), &(in_imag[i * length]), &(output_real[i * length]),
            &(output_imag[i * length]), length, forward);
    }

    if (!forward) {
        normalize_fft(output_real, output_imag, length, batch * length);
    }

    return 0;
}

void myistft(Complex **STFT, int xlen, int L, int fs, int boundary, int window, int nperseg, int noverlap, int nfft, float *x_reconstructed) {
    int hop = nperseg - noverlap;

    // Allocate window function
    float* win = (float*)malloc(nperseg * sizeof(float));
    switch (window) {
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

    // Calculate the normalization factor (sum of squared windows)
    float *win_squared = (float *)malloc(nperseg * sizeof(float));
    for (int i = 0; i < nperseg; i++) {
        win_squared[i] = win[i] * win[i];
    }

    // Allocate output buffer (maximum possible length)
    int max_len = (L - 1) * hop + nperseg;
    float *x_out = (float *)calloc(max_len, sizeof(float));
    float *norm = (float *)calloc(max_len, sizeof(float));

    // Process each frame
    for (int l = 0; l < L; l++) {
        // Prepare input/output arrays for IFFT
        float *in_real = (float *)malloc(nfft * sizeof(float));
        float *in_imag = (float *)malloc(nfft * sizeof(float));
        float *out_real = (float *)malloc(nfft * sizeof(float));
        float *out_imag = (float *)malloc(nfft * sizeof(float));

        // Copy STFT data to input
        for (int i = 0; i < nfft; i++) {
            in_real[i] = STFT[l][i].real;
            in_imag[i] = STFT[l][i].imag;
        }

        // Perform IFFT
        dft(in_real, in_imag, out_real, out_imag, nfft, false);

        // Extract the real part and scale by nfft
        float *frame = (float *)malloc(nperseg * sizeof(float));
        for (int i = 0; i < nperseg; i++) {
            frame[i] = out_real[i] / nfft;
        }

        // Overlap-add reconstruction
        int start = l * hop;
        for (int i = 0; i < nperseg; i++) {
            if (start + i < (L - 1) * hop + nperseg) {
                x_out[start + i] += frame[i] * win[i];
                norm[start + i] += win_squared[i];
            }
        }
        free(in_real);
        free(in_imag);
        free(out_real);
        free(out_imag);
        free(frame);
    }
    // Normalize by the window sum
    for (int i = 0; i < max_len; i++) {
        if (norm[i] > 1e-10) {  // Avoid division by zero
            x_out[i] /= norm[i];
        }
    }

    // Determine the actual length of the reconstructed signal

    // Copy to output
    if (boundary != none) {
        memcpy(x_reconstructed, x_out + nperseg / 2, xlen * sizeof(float));
    } else {
        memcpy(x_reconstructed, x_out, xlen * sizeof(float));
    }

    // Free memory
    free(win);
    free(win_squared);
    free(x_out);
    free(norm);
}

int main() {
    // idft
    FILE *input = fopen("fft_Y_Out.dat", "r");
    if (!input) {
        perror("Error opening fft_Y_Out.dat");
        return -1;
    }
    int N = 0;
    char line[256];
    while (fgets(line, sizeof(line), input)) {
        if (line[0] != '#') {
            N++;
        }
    }
    rewind(input);
    float *YR = (float*)malloc(N * sizeof(float));
    float *YI = (float*)malloc(N * sizeof(float));
    float *xr = (float*)malloc(N * sizeof(float));
    float *xi = (float*)malloc(N * sizeof(float));

    int i = 0;
    while (fgets(line, sizeof(line), input)) {
        if (line[0] != '#') {
            sscanf(line, "%f %f", &YR[i], &YI[i]);
            i++;
        }
    }
    fclose(input);
    dft(YR, YI, xr, xi, N, false);

    FILE *output = fopen("idft_result.dat", "w");
    if (!output) {
        perror("Error opening idft_result.dat");
        return -1;
    }
    fprintf(output, "# Index\tReal\tImag\n");
    for (int n = 0; n < N; n++) {
        fprintf(output, "%d\t%f\t%f\n", n, xr[n], xi[n]);
    }
    fclose(output);

    // istft
    int xlen, xlen_, fs, nperseg, noverlap, nfft, window, boundary;
    FILE *stft_param = fopen("stft_param.dat", "r");
    if (!stft_param) {
        perror("Error opening stft_param.dat");
        return -1;
    }

    int result = fscanf(stft_param, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", &xlen, &xlen_, &fs, &nperseg, &noverlap, &nfft, &window, &boundary);
    fclose(stft_param);
    if (result != 8) {
        fprintf(stderr, "Error reading parameters from file.\n");
        return -1;
    }

    int hop = nperseg - noverlap;
    int L = 1 + (xlen_ - nperseg) / hop;

    Complex **STFT = (Complex **)malloc(L * sizeof(Complex *));
    for (int l = 0; l < L; l++) {
        STFT[l] = (Complex *)malloc(nfft * sizeof(Complex));
    }

    FILE *stft_out = fopen("stft_Y_Out.dat", "r");
    if (!stft_out) {
        perror("Error opening stft_Y_Out.dat");
        return -1;
    }
    char header[256];
    if (!fgets(header, sizeof(header), stft_out)) {
        fprintf(stderr, "Error reading header\n");
        fclose(stft_out);
        return -1;
    }

    // Read data
    for (int l = 0; l < L; l++) {
        for (int i = 0; i < nfft; i++) {
            if (fscanf(stft_out, "%f\t%f", &STFT[l][i].real, &STFT[l][i].imag) != 2) {
                fprintf(stderr, "Error reading STFT data at frame %d, bin %d\n", l, i);
            }
        }
    }
    fclose(stft_out);

    float *x_reconstructed = (float*)malloc(xlen * sizeof(float));
    myistft(STFT, xlen, L, fs, boundary, window, nperseg, noverlap, nfft, x_reconstructed);
    FILE *recon_file = fopen("reconstructed_signal.txt", "w");
    for (int i = 0; i < xlen; i++) {
        fprintf(recon_file, "%f\n", x_reconstructed[i]);
    }
    fclose(recon_file);

    // Clean up
    for (int l = 0; l < L; l++) {
        free(STFT[l]);
    }
    free(STFT);
    free(YR);
    free(YI);
    free(xr);
    free(xi);
    free(x_reconstructed);

    return 0;
}