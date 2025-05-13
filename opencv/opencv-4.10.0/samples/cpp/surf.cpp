#include<opencv2/opencv.hpp>
#include<iostream>
#include<opencv2/xfeatures2d.hpp>
#include<vector>

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;
Mat src, gray_src;
const char* output_title = "SURF";
int main(int argc, char**argv) {
	src = imread("/home/zyz/projects/test_project/test_picture_video/frame.jpg");
	if (src.empty()) {
		printf("could not load image...\n");
		return -1;
	}
	namedWindow("input image", CV_WINDOW_AUTOSIZE);
	imshow("input image", src);

	//SURF特征检测
	int minHession = 400;
	Ptr<SURF> detector = SURF::create(minHession);
	vector<KeyPoint> keypoints;
	detector->detect(src, keypoints, Mat());

	//绘制关键点
	Mat keypoint_img;
	drawKeypoints(src, keypoints, keypoint_img, Scalar::all(-1),DrawMatchesFlags::DEFAULT);



	imshow(output_title, keypoint_img);
	waitKey(0);
	return 0;
}


// #include <opencv2/opencv.hpp>
// #include <iostream>

// int main() {
//     std::cout << "OpenCV version: " << CV_VERSION << std::endl;
//     return 0;
// }