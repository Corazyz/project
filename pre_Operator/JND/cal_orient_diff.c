#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"

#define NUM_BINS 30
#define HALF_BINS 15
#define QUANT_STEP 12

int CalPixOrientDiff(int x, int y, int *PixOrient, int width, int height)
{
    int center_bin = PixOrient[y*width + x];
    int bins[NUM_BINS] = {0};

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    for (int i = 0; i < 8; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;

        int neighbor_bin = PixOrient[ny*width + nx];
        int raw_diff = abs(neighbor_bin - center_bin);
        int circular_diff = (raw_diff > HALF_BINS) ? (NUM_BINS - raw_diff) : raw_diff;

        if (circular_diff > 0) {
            int quant_diff = (circular_diff * QUANT_STEP) / QUANT_STEP;
            bins[quant_diff] = 1;       // 为什么不是 += 1
        }
    }

    int N = 0;
    for (int i = 0; i < NUM_BINS; i++) {
        N += bins[i];
    }
    return N;
}

int main()
{
    int width = 5;
    int height = 5;
    int PixOrient[25] = {
        1, 1, 3, 5, 7,    // 第0行
        1, 2, 4, 6, 7,    // 第1行
        0, 2, 3, 5, 7,    // 第2行
        0, 1, 4, 6, 0,    // 第3行
        2, 3, 5, 7, 1     // 第4行
    };

    // 测试中心点(2,2)
    int x = 2, y = 2;
    int result = CalPixOrientDiff(x, y, PixOrient, width, height);
    printf("中心点(%d,%d)的差异计数: %d\n", x, y, result);

    // 测试边缘点(0,0)
    x = 0, y = 0;
    result = CalPixOrientDiff(x, y, PixOrient, width, height);
    printf("边缘点(%d,%d)的差异计数: %d\n", x, y, result);

    // 测试角点(4,4)
    x = 4, y = 4;
    result = CalPixOrientDiff(x, y, PixOrient, width, height);
    printf("角点(%d,%d)的差异计数: %d\n", x, y, result);

    return 0;
}