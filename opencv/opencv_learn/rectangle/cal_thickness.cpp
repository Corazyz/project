#include <iostream>
#include <math.h>
// #include "precomp.hpp"
#include "/home/zyz/projects/test_project/opencv/opencv-4.10.0/modules/core/include/opencv2/core/fast_math.hpp"

int main() {
    int x1 = 100;
    int y1 = 200;
    int x2 = 600;
    int y2 = 400;
    double INV_XY_ONE = (1.52587890625E-5);
    double dx = (x2 - x1) * INV_XY_ONE;
    double dy = (y2 - y1) * INV_XY_ONE;
    double r = dx * dx + dy * dy;
    int thickness = 2;
    int oddThickness = thickness & 1;
    int XY_ONE = 65536;
    r = (thickness + oddThickness*XY_ONE*0.5)/std::sqrt(r);
    printf("r = %f\n", r);
    int x = cvRound(dy * r);
    int y = cvRound(dx * r);
    x1 = x1 + x;
    y1 = y1 + y;
    x2 = x2 - x;
    y2 = y2 - y;
    printf("x1 = %d, y1 = %d, x2 = %d, y2 = %d\n", x1, y1, x2, y2);
    return 0;
}