#include<opencv2/opencv.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/highgui/highgui.hpp>
#include <iostream>
using namespace std;
using namespace cv;
int main()
{
    std::cout << "hello world" << std::endl;
    auto mat = cv::imread("crown.png");
    cv::Mat graymat;
    cv::cvtColor(mat, graymat, cv::COLOR_BGR2GRAY);
    cv::imwrite("grey.jpg", graymat);
    return 0;
}

// #include <opencv4/opencv2/opencv.hpp>

// int main( int argc, char** argv) {
//     cv::Mat imgOriginal;
//     imgOriginal = cv::imread("/home/zyz/projects/src/crown.png");
//     if( imgOrigibal.empty()) return -1;
//     cv::nameWindow("Example1", cv::WINDOW_AUTOSIZE);
//     cv::imshow("Example1", imgOriginal);
//     cv::waitKey(0);
//     cv::destroyWindow("Example1");
//     return 0;
// }
