#include <opencv2/opencv.hpp>
#include "iostream"
#include "math.h"
#include <opencv2/imgproc/types_c.h>
using namespace cv;
using namespace std;

int main() {
    // 生成方式1
    Mat src = imread("/home/zyz/projects/test_project/test_picture/crown.png");
    if (src.empty()) {
        cout << "could not load image" << endl;
        return -1;
    }
    // imwrite("dst.bin", src);

    // 拷贝生成
    Mat dst1 = src;
    Mat dst2(src);
    Mat dst3, dst4;
    dst3 = src.clone();
    src.copyTo(dst4);

    Mat dst5 = Mat::zeros(src.size(), src.type());

    Mat dst6 = Mat::zeros(10, 10, CV_8UC1);
    Mat dst7 = Mat::eye(10, 10, CV_8UC3);
    Mat dst8;
    dst8.create(4, 14, CV_8UC3);
    dst8 = Scalar(10, 11, 12);
    Mat dst9;
    dst9.create(dst8.size(), CV_8UC1);
    dst9 = Scalar(55);

    cout << "dst8" << dst8 << endl;
    cout << "dst9" << dst9 << endl;

    waitKey(0);
    return 0;
}