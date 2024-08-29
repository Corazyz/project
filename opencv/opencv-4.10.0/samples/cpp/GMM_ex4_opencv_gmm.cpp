#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/video/tracking.hpp"
#include <opencv2/video/background_segm.hpp>
#include <iostream>
#include <numeric>
#include <vector>
using namespace cv;
using namespace std;

struct GaussianDistribution
{
	float w;// 权重
	float u;// 期望
	float sigma;// 标准差
};

struct PixelGMM
{
	int num;// 高斯模型最大数量
	int index;// 当前使用的高斯模型数量
	GaussianDistribution* gd;// 高斯模型数组指针
};
bool modelInit = false;
const int GAUSSIAN_MODULE_NUMS = 5;// 模型数量
const float ALPHA = 0.005;// 学习速率
const float SIGMA = 30;// 标准差初始值
const float WEIGHT = 0.05;// 高斯模型权重初始值
const float T = 0.7;// 有效高斯分布阈值
int rows, cols;
PixelGMM* ppgmm;

int main()
{
	Mat image;
	Mat imageGray, imageFG, imageMog;
	// cv::BackgroundSubtractorMOG mog;
    cv::Ptr<cv::BackgroundSubtractorMOG2> mog = cv::createBackgroundSubtractorMOG2();

	// 打开视频文件
	VideoCapture cap("/home/zyz/projects/test_picture_video/DSCF1929_fish.AVI");
	if(!cap.isOpened())
	{
		cout<<"cannot open avi file"<<endl;
		return -1;
	}

	// double fps = cap.get(CV_CAP_PROP_FPS);// 获取图像帧率
	// int pauseTime = (int)(1000.f/fps);
	// namedWindow("video");

	while(1)
	{
		if(!cap.read(image))
			break;
		cvtColor(image, imageGray, COLOR_BGR2GRAY);// 转化为灰度图处理
        // cvtColor(frame, frameGray, cv::COLOR_BGR2GRAY);
		mog->apply(imageGray,imageMog,0.005);
        // mog->apply(frameGray, foreground, 0.01);

		// 初始化各个像素的高斯分布
		if(!modelInit)
		{

			/* 高斯分布参数分配空间*/
			rows = image.rows;
			cols = image.cols;
			ppgmm = (PixelGMM*)malloc(rows*cols*sizeof(PixelGMM));
			for(int i = 0; i < rows*cols; i++)
			{
				ppgmm[i].num = GAUSSIAN_MODULE_NUMS;
				ppgmm[i].index = 0;
				ppgmm[i].gd = (GaussianDistribution*)malloc(GAUSSIAN_MODULE_NUMS*sizeof(GaussianDistribution));
			}

			/* 初始化高斯分布参数 */

			for(int i = 0; i < rows; i++)
			{
				for(int j = 0; j < cols; j++)
				{
					for(int n = 0; n < GAUSSIAN_MODULE_NUMS; n++)
					{
						ppgmm[i*cols + j].gd[n].w = 0;
						ppgmm[i*cols + j].gd[n].u = 0;
						ppgmm[i*cols + j].gd[n].sigma = SIGMA;

					}
				}
			}

			imageFG.create(rows, cols, CV_8UC1);
			modelInit = true;
		}
		{
			// 结果图像初始化为全黑
			imageFG.setTo(Scalar(0));

			for(int i = 0; i < rows; i++)
			{
				for(int j = 0; j < cols; j++)
				{
					int kHit = -1;// 匹配高斯模型索引
					int gray = imageGray.at<unsigned char>(i,j);
					//判断是否属于当前高斯模型
					for(int m = 0; m < ppgmm[i*cols + j].index; m++)
					{
						if(fabs(gray - ppgmm[i*cols + j].gd[m].u) < 2.5*ppgmm[i*cols + j].gd[m].sigma)// 满足分布
						{
							// 更新该高斯分布的标准差、期望、权值，标准差变小，权值增加
							if(ppgmm[i*cols + j].gd[m].w > 1)
								ppgmm[i*cols + j].gd[m].w = 1;
							else
								ppgmm[i*cols + j].gd[m].w = (1-ALPHA)*ppgmm[i*cols + j].gd[m].w + ALPHA*1;
							ppgmm[i*cols + j].gd[m].u = (1-ALPHA)*ppgmm[i*cols + j].gd[m].u + ALPHA*gray;
							if(ppgmm[i*cols + j].gd[m].sigma < SIGMA/2)
								ppgmm[i*cols + j].gd[m].sigma = SIGMA;
							else
								ppgmm[i*cols + j].gd[m].sigma = sqrt((1-ALPHA)*ppgmm[i*cols + j].gd[m].sigma
								*ppgmm[i*cols + j].gd[m].sigma + ALPHA*(gray - ppgmm[i*cols + j].gd[m].u)
								*(gray - ppgmm[i*cols + j].gd[m].u));// 若同一高斯分布被重复匹配多次，其标准差会一直下降，这里需要设置最低阈值

							// 根据w/sigma降序排序
							int n;
							for(n = m-1; n >= 0; n--)
							{
								if(ppgmm[i*cols + j].gd[n].w/ppgmm[i*cols + j].gd[n].sigma <= ppgmm[i*cols + j].gd[n+1].w/ppgmm[i*cols + j].gd[n+1].sigma)
								{
									float temp;
									temp = ppgmm[i*cols + j].gd[n].sigma;
									ppgmm[i*cols + j].gd[n].sigma = ppgmm[i*cols + j].gd[n+1].sigma;
									ppgmm[i*cols + j].gd[n+1].sigma = temp;
									temp = ppgmm[i*cols + j].gd[n].u;
									ppgmm[i*cols + j].gd[n].u = ppgmm[i*cols + j].gd[n+1].u;
									ppgmm[i*cols + j].gd[n+1].u = temp;
									temp = ppgmm[i*cols + j].gd[n].w;
									ppgmm[i*cols + j].gd[n].w = ppgmm[i*cols + j].gd[n+1].w;
									ppgmm[i*cols + j].gd[n+1].w = temp;

								}
								else
								{
									break;
								}
							}
							kHit = n + 1;// 匹配高斯分布的最终索引
							break;
						}
						else
						{
							// 没有被匹配到的高斯分布权重降低
							ppgmm[i*cols + j].gd[m].w *= (1-ALPHA);
						}
					}

					// 增加新的高斯分布，属于前景
					if(kHit == -1)
					{

						// 需要去除影响最小的高斯分布
						if(ppgmm[i*cols + j].index == GAUSSIAN_MODULE_NUMS)
						{
							ppgmm[i*cols + j].gd[ppgmm[i*cols + j].index - 1].sigma = SIGMA;
							ppgmm[i*cols + j].gd[ppgmm[i*cols + j].index - 1].u = gray;
							ppgmm[i*cols + j].gd[ppgmm[i*cols + j].index - 1].w = WEIGHT;

						}
						else
						{
							// 增加新的高斯分布
							ppgmm[i*cols + j].gd[ppgmm[i*cols + j].index].sigma = SIGMA;
							ppgmm[i*cols + j].gd[ppgmm[i*cols + j].index].u = gray;
							ppgmm[i*cols + j].gd[ppgmm[i*cols + j].index].w = WEIGHT;
							ppgmm[i*cols + j].index++;
						}
					}

					// 高斯分布权值归一化
					float weightSum = 0;
					for(int n = 0; n < ppgmm[i*cols + j].index; n++)
					{
						weightSum += ppgmm[i*cols + j].gd[n].w;
					}
					float weightScale = 1.f/weightSum;

					// 根据T得到有效高斯分布的截止索引
					weightSum = 0;
					int kForeground = -1;
					for(int n = 0; n < ppgmm[i*cols + j].index; n++)
					{
						ppgmm[i*cols + j].gd[n].w *= weightScale;
						weightSum += ppgmm[i*cols + j].gd[n].w;
						if(weightSum > T && kForeground < 0)
						{
							kForeground = n + 1;
						}
					}

					// 属于前景点的判断条件
					if((kHit > 0&&kHit >= +66666666666
					 kForeground) || kHit == -1)
					{
						imageFG.at<unsigned char>(i,j) = 255;

					}
				}

			}
		}

        cv::imwrite("/home/zyz/projects/python_projects/GMM_project/GMM_output/imageMog.jpg", imageMog);
        cv::imwrite("/home/zyz/projects/python_projects/GMM_project/GMM_output/video.jpg", imageGray);
        cv::imwrite("/home/zyz/projects/python_projects/GMM_project/GMM_output/imageFG.jpg", imageFG);
		// imshow("imageMog", imageMog);
		// imshow("video", imageGray);
		// imshow("imageFG", imageFG);
		// waitKey(10);
	}


	return 0;
}