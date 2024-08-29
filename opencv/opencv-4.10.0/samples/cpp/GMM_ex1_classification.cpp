#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace cv::ml;
using namespace std;

int main(int argc, char** argv) {
	Mat img(500, 500, CV_8UC3);
	RNG rng(12345);
	vector<int> vi = {1,2,3,4,5,6};
	randShuffle(vi, 1, &rng);
	for (size_t i=0; i<vi.size(); i++) {
		cout << vi[i] << endl;
	}

	Scalar colorTab[] = {
		Scalar(0, 0, 255),
		Scalar(0, 255, 0),
		Scalar(255, 0, 0),
		Scalar(0, 255, 255),
		Scalar(255, 0, 255)
	};

	int numCluster = rng.uniform(2, 10);
	printf("number of clusters: %d\n", numCluster);
	int sampleCount = rng.uniform(5, 100);
	Mat points(sampleCount, 2, CV_32GC1, Scalar(10, 1));
	
	return 0;
}









// int main(int argc, char** argv) {
// 	//产生随机的点集
// 	Mat img(500, 500, CV_8UC3);
// 	RNG rng(12345);
// 	vector<int> vi = {1,2,3,4,5,6};
// 	randShuffle(vi, 1, &rng);
// 	for (size_t i = 0; i < vi.size(); i++)
// 	{
// 		cout << vi[i] << endl;
// 	}

// 	Scalar colorTab[] = {
// 		Scalar(0, 0, 255),
// 		Scalar(0, 255, 0),
// 		Scalar(255, 0, 0),
// 		Scalar(0, 255, 255),
// 		Scalar(255, 0, 255)
// 	};

// 	int numCluster = rng.uniform(2, 10);
// 	printf("number of clusters : %d\n", numCluster);

// 	int sampleCount = rng.uniform(5, 100);
// 	//1，Mat(row,col,type) 定义一个sampleCount行 ，2列的数据
// 	Mat points(sampleCount, 2, CV_32FC1,Scalar(10,1));

// 	Mat labels;
// 	Mat centers;
// 	cout << points << endl;
// 	// 生成随机数
// 	for (int k = 0; k < numCluster; k++) {
// 		Point center;
// 		center.x = rng.uniform(0, img.cols);
// 		center.y = rng.uniform(0, img.rows);
// 		//为矩阵的指定行区间创建一个矩阵头 参数1,从0开始的行间距索引;  参数2,终止索引（把points的第n,到n+1行数据放到这里）
// 		Mat pointChunk = points.rowRange(k*sampleCount / numCluster,
// 			k == numCluster - 1 ? sampleCount : (k + 1)*sampleCount / numCluster);

// 		/*用随机数填充矩阵 ,
// 		InputOutputArray                    输入输出矩阵，最多支持4通道，超过4通道先用reshape()改变结构

// 		int distType                             UNIFORM 或 NORMAL，表示均匀分布和高斯分布

// 		InputArray a                           disType是UNIFORM,a表示为下限

// 		InputArray b                           disType是UNIFORM,b表示为上限

// 		bool saturateRange=false     只针对均匀分布有效。当为真的时候，会先把产生随机数的范围变换到数据类型的范围，再产生随机数；
// 		                             如果为假，会先产生随机数，再进行截断到数据类型的有效区间。请看以下fillM1和fillM2的例子并观察结果 */
// 		rng.fill(pointChunk, RNG::NORMAL, Scalar(center.x, center.y), Scalar(img.cols*0.05, img.rows*0.05));

// 	}

// 	randShuffle(points, 1, &rng);


// 	Ptr<EM> em_model = EM::create();
// 	em_model->setClustersNumber(numCluster);
// 	em_model->setCovarianceMatrixType(EM::COV_MAT_SPHERICAL);
// 	em_model->setTermCriteria(TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 100, 0.1));
// 	em_model->trainEM(points, noArray(), labels, noArray());
// 	//em_model 模型
// 	// classify every image pixels(像素)
// 	Mat sample(1, 2, CV_32FC1);
// 	for (int row = 0; row < img.rows; row++) {
// 		for(int col = 0; col < img.cols; col++) {
// 			sample.at<float>(0) = (float)col;
// 			sample.at<float>(1) = (float)row;
// 			//cvRound()：四舍五入；
// 			// predict2：EM预言 sample:位置
// 			int response = cvRound(em_model->predict2(sample, noArray())[1]);
// 			/*typedef struct Scalar
// 			{
// 				double val[4];
// 			}Scalar*/
// 			Scalar c = colorTab[response];
// 			circle(img, Point(col, row), 1, c*0.75, -1);
// 		}
// 	}

// 	// draw the clusters
// 	for (int i = 0; i < sampleCount; i++) {
// 		Point p(cvRound(points.at<float>(i, 0)), points.at<float>(i, 1));
// 		circle(img, p, 3, colorTab[labels.at<int>(i)], -1);
// 	}

// 	// imshow("GMM-EM Demo", img);
//     imwrite("/home/zyz/projects/test_picture_video/GMM-EM-Demo.jpg", img);

// 	waitKey(0);
// 	return 0;
// }