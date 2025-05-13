#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <time.h>
#include <pthread.h>

// template <typename T>
void fill_img_float(float* input, int size) {
    float count = (float)(10);
    for (int i = 0; i < size; i++) {
        input[i] = count;
        count++;
        if (count == float(256)) {
            count = float(10);
        }
    }
}

static void writeBin_float(const char * path, float* input_data, int size) {

    FILE *fp_dst = fopen(path, "wb");
    // if (fp_dst == NULL) {
    //     perror("Failed to open file");
    //     return;
    // }
    printf("--->test for fault_write_bin\n");
    if (fwrite(input_data, sizeof(float), size, fp_dst) < size) {
        printf("file size is less than %d required bytes\n", size);
    }

    fclose(fp_dst);
}

int main() {
    int width = 1920;
    int height = 1080;
    float *input_data1 = (float *)malloc(width * height * 3);
    float *input_data2 = (float *)malloc(width * height * 3);
    fill_img_float(input_data1, width * height * 3);
    writeBin_float("./test_input1_data", input_data1, width * height * 3);
    fill_img_float(input_data2, width * height * 3);
    writeBin_float("./test_input2_data", input_data2, width * height * 3);
}