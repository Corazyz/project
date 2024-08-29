// // 功能：代码 9-5 高斯混合背景建模
// // 作者：朱伟 zhu1988wei@163.com
// // 来源：《OpenCV图像处理编程实例》
// // 博客：http://blog.csdn.net/zhuwei1988
// // 更新：2016-8-1
// // 说明：版权所有，引用或摘录请联系作者，并按照上面格式注明出处，谢谢。//
// #include "opencv2/imgcodecs.hpp"
// #include "opencv2/imgproc.hpp"
// #include "opencv2/videoio.hpp"
// #include <opencv2/highgui.hpp>
// #include <opencv2/video.hpp>
// #include <iostream>
// #include <sstream>
// #include <stdio.h>

// using namespace cv;
// using namespace std;
// Mat frame;
// Mat fgMaskMOG2;
// Ptr<BackgroundSubtractor> pMOG2;
// int keyboard;
// void processVideo(string videoFilename)
// {
//     // 视频获取
//     VideoCapture capture(videoFilename);
//     if(!capture.isOpened())
//     {
//         // 输出视频文件打开错误信息
//         cerr << "Unable to open video file: " << videoFilename << endl;
//         exit(EXIT_FAILURE);
//     }
//     // 按下q键和esc退出
//     int count = 0;
//     // while( (char)keyboard != 'q' && (char)keyboard != 27 )
//     while (count < 150)
//     {
//         printf("current frame: %d\n", count++);
//         // 读取当前帧
//         if(!capture.read(frame))
//         {
//             // int f_num = 0;
//             // printf("num of frames: %d\n", f_num++);
//             cerr << "Unable to read next frame." << endl;
//             cerr << "Exiting..." << endl;
//             exit(EXIT_FAILURE);
//         }
//         // 图像尺寸缩小
//         cv::resize(frame, frame,cv::Size(), 0.25,0.25);
//         //  背景模型生成
//         pMOG2->apply(frame, fgMaskMOG2);
//         // 输出当前帧号
//         stringstream ss;
//         rectangle(frame, cv::Point(10, 2), cv::Point(100,20),
//             cv::Scalar(255,255,255), -1);
//         ss << capture.get(CAP_PROP_POS_FRAMES);
//         string frameNumberString = ss.str();
//         // 左上角显示帧号
//         putText(frame, frameNumberString.c_str(), cv::Point(15, 15),
//             FONT_HERSHEY_SIMPLEX, 0.5 , cv::Scalar(0,0,0));
//         // 输出结果
//         // imshow("Frame", frame);
//         // imshow("FG Mask MOG 2", fgMaskMOG2);
//         imwrite("/home/zyz/projects/test_picture_video/frame.jpg", frame);
//         imwrite("/home/zyz/projects/test_picture_video/FG_Mask_MOG_2.jpg", fgMaskMOG2);
//         keyboard = waitKey(30);
//     }
//     capture.release();
// }
// int main(int argc, char* argv[])
// {
//     // 创建背景建模类
//     pMOG2 = createBackgroundSubtractorMOG2();
//     string inputPath = "/home/zyz/projects/test_picture_video/DSCF1929_fish.AVI";
//     processVideo(inputPath);
//     return 0;
// }

#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>


int main()
{
	// Open the video file
    cv::VideoCapture capture("/home/zyz/projects/test_picture_video/DSCF1929_fish.AVI");
	// check if video successfully opened
	if (!capture.isOpened())
		return 0;

	// current video frame
	cv::Mat frame, frameGray;
	// foreground binary image
	cv::Mat foreground;

	// cv::namedWindow("Extracted Foreground");

	// The Mixture of Gaussian object
	// used with all default parameters
	// cv::BackgroundSubtractorMOG2 mog;
    cv::Ptr<cv::BackgroundSubtractorMOG2> mog = cv::createBackgroundSubtractorMOG2();
	bool stop(false);
	// for all frames in video
	while (!stop) {

		// read next frame if any
		if (!capture.read(frame))
			break;

		// update the background
		// and return the foreground
		cvtColor(frame, frameGray, cv::COLOR_BGR2GRAY);
		// mog(frameGray,foreground, 0.01);
        mog->apply(frameGray, foreground, 0.01);

		// show foreground
		// cv::imshow("Extracted Foreground",foreground);
		// cv::imshow("image",frame);
        cv::imwrite("/home/zyz/projects/test_picture_video/frame.jpg", frame);
        cv::imwrite("/home/zyz/projects/test_picture_video/FG_Mask_MOG_2.jpg", foreground);

		// introduce a delay
		// or press key to stop
		if (cv::waitKey(10)>=0)
				stop= true;
	}

	return 0;
}


// #pragma once
// #include <iostream>
// #include "opencv2/opencv.hpp"

// using namespace cv;
// using namespace std;

// #define GMM_MAX_COMPONT 6
// #define GMM_LEARN_ALPHA 0.005
// #define GMM_THRESHOLD_SUMW 0.7
// #define TRAIN_FRAMES 60

// class MOG_BGS {
// public:
// 	MOG_BGS(void);
// 	~MOG_BGS(void);

// 	void init(const Mat _image);
// 	void processFirstFrame(const Mat _image);
// 	void trainGMM(const Mat _image);
// 	void getFitNum(const Mat _image);
// 	void testGMM(const Mat _image);
// 	Mat getMast(void) {return m_mask;};

// private:
// 	Mat m_weight[GMM_MAX_COMPONT];
// 	Mat m_mean[GMM_MAX_COMPONT];
// 	Mat m_sigma[GMM_MAX_COMPONT];

// 	Mat m_mask;
// 	Mat m_fit_num;
// };