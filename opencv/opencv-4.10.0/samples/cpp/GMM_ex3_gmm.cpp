#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <glob.h>
#include <string>

using namespace cv;
using namespace std;

const int GMM_MAX_COMPONT = 5;
const float SIGMA = 30.0f;
const float INITIAL_WEIGHT = 0.05f;  // 初始权重
const float T = 0.7f;
const float alpha = 0.005f;
const float eps = pow(10, -10);

vector<Mat> m_weight(GMM_MAX_COMPONT * 3);
vector<Mat> m_mean(GMM_MAX_COMPONT * 3);
vector<Mat> m_sigma(GMM_MAX_COMPONT * 3);
Mat m_fit_num;

void init(const Mat& img) {
    int rows = img.rows;
    int cols = img.cols;
    int channels = img.channels();

    for (int i = 0; i < GMM_MAX_COMPONT * channels; i++) {
        m_weight[i] = Mat::ones(rows, cols, CV_32FC1) * INITIAL_WEIGHT;
        m_mean[i] = Mat::zeros(rows, cols, CV_32FC1);  // 初始均值设置为0，但可以考虑使用图像的全局平均
        m_sigma[i] = Mat::ones(rows, cols, CV_32FC1) * SIGMA;
    }

    m_fit_num = Mat::zeros(rows, cols, CV_32SC1);
}

void train_gmm(const Mat& imgs) {
    vector<Mat> bgr;
    split(imgs, bgr);
    imwrite("/home/zyz/projects/python_projects/GMM_project/test_split_r.png", bgr[0]);
    imwrite("/home/zyz/projects/python_projects/GMM_project/test_split_g.png", bgr[1]);
    imwrite("/home/zyz/projects/python_projects/GMM_project/test_split_b.png", bgr[2]);
    int rows = imgs.rows;
    int cols = imgs.cols;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            for (int c = 0; c < 3; c++) {
                Mat img = bgr[c];
                vector<pair<float, int>> components;  // 用于存储权重和索引

                // 更新每个高斯分量
                for (int k = c * GMM_MAX_COMPONT; k < (c + 1) * GMM_MAX_COMPONT; k++) {
                    float weight = m_weight[k].at<float>(i, j);
                    float mean = m_mean[k].at<float>(i, j);
                    float sigma = m_sigma[k].at<float>(i, j);
                    float pixel = img.at<uchar>(i, j);
                    float delta = abs(pixel - mean);
                    if (delta < 2.5 * sigma) {
                        // 更新存在的高斯分量
                        float new_weight = (1 - alpha) * weight + alpha;
                        float new_mean = (1 - alpha) * mean + alpha * pixel;
                        float diff = pixel - new_mean;
                        float new_sigma = sqrt((1 - alpha) * sigma * sigma + alpha * diff * diff);

                        m_weight[k].at<float>(i, j) = new_weight;
                        m_mean[k].at<float>(i, j) = new_mean;
                        m_sigma[k].at<float>(i, j) = new_sigma;
                    } else {
                        m_weight[k].at<float>(i, j) *= (1 - alpha);
                    }
                    components.push_back(make_pair(weight / (sigma + eps), k));  // 记录权重/方差比
                }

                // 排序找到最小的高斯分量
                sort(components.begin(), components.end());

                // 如果没有匹配的高斯分量，替换最不可能的分量
                if (components.front().first == 0) {
                    int replace_index = components.front().second;
                    m_weight[replace_index].at<float>(i, j) = INITIAL_WEIGHT;
                    m_mean[replace_index].at<float>(i, j) = img.at<uchar>(i, j);
                    m_sigma[replace_index].at<float>(i, j) = SIGMA;
                }

                // 归一化权重
                float weight_sum = 0;
                for (int nn = c * GMM_MAX_COMPONT; nn < (c + 1) * GMM_MAX_COMPONT; nn++) {
                    weight_sum += m_weight[nn].at<float>(i, j);
                }

                if (weight_sum > eps) {
                    for (int nn = c * GMM_MAX_COMPONT; nn < (c + 1) * GMM_MAX_COMPONT; nn++) {
                        m_weight[nn].at<float>(i, j) /= weight_sum;
                    }
                }
            }
        }
    }
}

Mat test_img(const Mat& imgs) {
    int rows = imgs.rows;
    int cols = imgs.cols;
    vector<Mat> bgr;
    split(imgs, bgr);
    Mat m_mask = Mat::ones(rows, cols, CV_8UC1) * 255;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int cnt = 0;
            for (int c = 0; c < imgs.channels(); c++) {
                Mat img = bgr[c];
                float weight_sum = 0;
                for (int nn = c * GMM_MAX_COMPONT; nn < (c + 1) * GMM_MAX_COMPONT; nn++) {
                    float weight = m_weight[nn].at<float>(i, j);
                    float mean = m_mean[nn].at<float>(i, j);
                    float sigma = m_sigma[nn].at<float>(i, j);
                    float pixel = img.at<uchar>(i, j);
                    if (abs(pixel - mean) < 2 * sigma) {
                        cnt++;
                        break;
                    }
                    weight_sum += weight;
                    if (weight_sum > T) {
                        break;
                    }
                }
            }

            if (cnt == imgs.channels()) {
                m_mask.at<uchar>(i, j) = 0;
            }
        }
    }
    imwrite("/home/zyz/projects/python_projects/GMM_project/test.png", m_mask);

    // medianBlur(m_mask, m_mask, 7);
    // Mat kernel_d = getStructuringElement(MORPH_RECT, Size(5, 5));
    // dilate(m_mask, m_mask, kernel_d);

    return m_mask;
}
vector<string> glob_vector(const string& pattern) {
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));

    int return_value = glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_result);
    vector<string> filenames;
    if (return_value == 0) {
        for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
            filenames.push_back(string(glob_result.gl_pathv[i]));
        }
    }
    globfree(&glob_result);
    return filenames;
}

int main() {
    string path = "/home/zyz/projects/python_projects/GMM_project/frame/b*.bmp";
    vector<string> files = glob_vector(path);

    int i = 0;
    for (const string& file : files) {
        Mat img = imread(file);
        if (i == 0) {
            init(img);
        }

        if (i <= 200) {
            double t1 = (double)getTickCount();
            train_gmm(img);
            double t2 = (double)getTickCount();
            cout << "Time elapsed: " << (t2 - t1) / getTickFrequency() << " seconds." << endl;
        }

        if (i == 286) {
            int j = 0;
            for (const string& temp_file : files) {
                Mat temp_img = imread(temp_file);
                temp_img = test_img(temp_img);
                imwrite("/home/zyz/projects/python_projects/GMM_project/gray/" + to_string(j) + ".jpg", temp_img);
                j++;
            }
        }
        i++;
    }

    return 0;
}
