#include<iostream>
#include<opencv2/opencv.hpp>

using namespace std;
using namespace cv;
using namespace cv::ml;

int main(int argc, char** argv)
{
	Mat src = imread("E:/技能学习/opencv图像分割/test.jpg");
	if (src.empty())
	{
		cout << "could not load image!" << endl;
		return -1;
	}

	namedWindow("input image", WINDOW_AUTOSIZE);
	imshow("input image", src);

	//初始化
	int numCluster = 3;
	Scalar colorTab[] = {
		Scalar(0,0,255),
		Scalar(0,255,0),
		Scalar(255,0,0),
		Scalar(0,255,255), //红+绿 == 黄
	};

	int width = src.cols;
	int height = src.rows;
	int dims = src.channels();

	int numsamples = width * height; //样本总数

	Mat points(numsamples, dims, CV_64FC1); //样本矩阵  选择64位是为了精度更高
	Mat labels; //存放表示每个簇的标签
	Mat result = Mat::zeros(src.size(), CV_8UC3);

	//图像RGB像素数据转换为样本数据
	int index = 0;
	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			index = row * width + col;
			Vec3b rgb = src.at<Vec3b>(row, col);
			points.at<double>(index, 0) = static_cast<int>(rgb[0]);
			points.at<double>(index, 1) = static_cast<int>(rgb[1]);
			points.at<double>(index, 2) = static_cast<int>(rgb[2]);
		}
	}

	int begin_time = getTickCount();
	// EM Cluster Train
	Ptr<EM> em_model = EM::create();
	em_model->setClustersNumber(numCluster); //类别数
	em_model->setCovarianceMatrixType(EM::COV_MAT_SPHERICAL); //协方差矩阵的类型
	em_model->setTermCriteria(TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 100, 0.1)); //迭代停止的标准
	em_model->trainEM(points, noArray(), labels, noArray()); //训练

	//对每个像素标记颜色与显示
	Mat sample(1, dims, CV_64FC1);
	int end_time = getTickCount();

	int r = 0, g = 0, b = 0;

	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			index = row * width + col;

			/*
			//直接显示
			int label = labels.at<int>(index, 0);
			Scalar c = colorTab[label];
			result.at<Vec3b>(row, col)[0] = c[0];
			result.at<Vec3b>(row, col)[1] = c[1];
			result.at<Vec3b>(row, col)[2] = c[2];
			*/

			//预言的方式显示
			b = src.at<Vec3b>(row, col)[0];
			g = src.at<Vec3b>(row, col)[1];
			r = src.at<Vec3b>(row, col)[2];
			sample.at<double>(0) = b;
			sample.at<double>(1) = g;
			sample.at<double>(2) = r;

			int response = cvRound(em_model->predict2(sample, noArray())[1]);
			Scalar c = colorTab[response];

			result.at<Vec3b>(row, col)[0] = c[0];
			result.at<Vec3b>(row, col)[1] = c[1];
			result.at<Vec3b>(row, col)[2] = c[2];
		}
	}

	//cout << "execution time:" << (time - getTickCount()) / (getTickFrequency() * 1000) << endl;
	cout << "execution time:" << ((end_time - begin_time)/ getTickFrequency()) << endl;
	imshow("EM-Segmentation:",result);

	waitKey(0);
	destroyAllWindows();
	return 0;

}
