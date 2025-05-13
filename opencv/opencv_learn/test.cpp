#include "opencv2/opencv.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string img_file = "./Soil.jpg";
    cv::Mat mat = cv::imread(img_file, cv::IMREAD_AVFRAME, 0);
    cv::Mat resize_mat;
    cv::resize(mat, resize_mat, cv::Size(1920, 1080));
    int count = 0;
    std::string name = "resize" + std::to_string(count++) + ".jpg";
    cv::imwrite(name, resize_mat);
}