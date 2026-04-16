// #include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip> // for setw, setfill

using namespace cv;
using namespace std;

// 高斯分布结构体
struct Gaussian {
    Vec3d mean;       // 均值（BGR）
    Vec3d var;        // 方差（BGR）
    double weight;    // 权重
    double alpha;     // 学习率（用于更新）

    Gaussian() : mean(0,0,0), var(10,10,10), weight(0.0), alpha(0.001) {}

    // 判断像素是否匹配该高斯分布（2.5σ准则）
    bool match(const Vec3b& pixel) const {
        for (int c = 0; c < 3; ++c) {
            double diff = abs(pixel[c] - mean[c]);
            if (diff > 2.5 * sqrt(var[c])) {
                return false;
            }
        }
        return true;
    }

    // 更新高斯分布参数
    void update(const Vec3b& pixel, double alpha) {
        for (int c = 0; c < 3; ++c) {
            double diff = pixel[c] - mean[c];
            double rho = alpha / sqrt(var[c] + 1e-6);  // 防止除零
            mean[c] += rho * diff;
            var[c] += rho * (diff * diff - var[c]);
        }
        weight += alpha * (1.0 - weight);  // 权重更新
    }

    // 用新像素初始化高斯分布
    void initialize(const Vec3b& pixel, double alpha) {
        for (int c = 0; c < 3; ++c) {
            mean[c] = pixel[c];
            var[c] = 10.0;  // 初始方差
        }
        weight = 0.1;  // 初始权重 0.1
        this->alpha = alpha;
    }

    // 计算 w / σ（用于排序）
    double getSortKey() const {
        double sum = 0.0;
        for (int c = 0; c < 3; ++c) {
            sum += weight / (sqrt(var[c]) + 1e-6);
        }
        return sum / 3.0;
    }
};

// MOG2 背景减除器类
class BackgroundSubtractorMOG2Custom {
private:
    int K;              // 每个像素的高斯分布数量
    double alpha;       // 学习率
    double varThreshold; // 匹配阈值（用于判断是否匹配）
    bool detectShadows; // 是否检测阴影
    double shadowThreshold; // 阴影检测阈值（亮度变化但色度不变）
    int history;        // 背景模型历史帧数（未实现，可扩展）

    vector<vector<vector<Gaussian>>> model; // [height][width][K]

public:
    BackgroundSubtractorMOG2Custom(int K = 5, double alpha = 0.001, double varThreshold = 16.0, bool detectShadows = true)
        : K(K), alpha(alpha), varThreshold(varThreshold), detectShadows(detectShadows), shadowThreshold(0.5) {}

    void initialize(const Mat& frame) {
        int h = frame.rows, w = frame.cols;
        model.resize(h, vector<vector<Gaussian>>(w, vector<Gaussian>(K)));
        // 初始化每个像素的第一个高斯分布
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                Vec3b pixel = frame.at<Vec3b>(y, x);
                model[y][x][0].initialize(pixel, alpha);
                model[y][x][0].weight = 0.1; // 第一个高斯权重设为1
            }
        }
    }

    Mat apply(const Mat& frame) {
        Mat mask = Mat::zeros(frame.size(), CV_8U);
        int h = frame.rows, w = frame.cols;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                Vec3b pixel = frame.at<Vec3b>(y, x);
                vector<Gaussian>& gaussians = model[y][x];
                int matchedIdx = -1;

                // 步骤1：查找匹配的高斯分布
                for (int i = 0; i < K; ++i) {
                    if (gaussians[i].match(pixel)) {
                        matchedIdx = i;
                        break;
                    }
                }

                if (matchedIdx != -1) {
                    // 更新匹配的高斯分布
                    gaussians[matchedIdx].update(pixel, alpha);
                } else {
                    // 替换权重最小的高斯分布
                    int minIdx = 0;
                    for (int i = 1; i < K; ++i) {
                        if (gaussians[i].weight < gaussians[minIdx].weight) {
                            minIdx = i;
                        }
                    }
                    gaussians[minIdx].initialize(pixel, alpha);
                }

                // 步骤2：排序高斯分布（按 w/σ）
                sort(gaussians.begin(), gaussians.end(), [](const Gaussian& a, const Gaussian& b) {
                    return a.getSortKey() > b.getSortKey();
                });

                // 步骤3：判断前景/背景
                double cumWeight = 0.0;
                bool isBackground = false;
                for (int i = 0; i < K; ++i) {
                    cumWeight += gaussians[i].weight;
                    if (cumWeight >= 0.7) { // 背景模型阈值 T=0.7
                        if (gaussians[i].match(pixel)) {
                            isBackground = true;
                            break;
                        }
                    }
                }

                if (!isBackground) {
                    // 判断是否为阴影
                    if (detectShadows && isShadow(pixel, gaussians)) {
                        mask.at<uchar>(y, x) = 127; // 阴影
                    } else {
                        mask.at<uchar>(y, x) = 255; // 前景
                    }
                } else {
                    mask.at<uchar>(y, x) = 0; // 背景
                }
            }
        }

        return mask;
    }

private:
    // 判断是否为阴影（亮度变化但色度不变）
    bool isShadow(const Vec3b& pixel, const vector<Gaussian>& gaussians) const {
        // 找到最匹配的背景高斯分布
        int bestIdx = -1;
        double minDist = 1e9;
        for (int i = 0; i < K; ++i) {
            double dist = 0.0;
            for (int c = 0; c < 3; ++c) {
                dist += abs(pixel[c] - gaussians[i].mean[c]);
            }
            if (dist < minDist) {
                minDist = dist;
                bestIdx = i;
            }
        }

        if (bestIdx == -1) return false;

        // 计算亮度变化比例
        double bgLum = (gaussians[bestIdx].mean[0] + gaussians[bestIdx].mean[1] + gaussians[bestIdx].mean[2]) / 3.0;
        double pixelLum = (pixel[0] + pixel[1] + pixel[2]) / 3.0;
        double lumRatio = pixelLum / (bgLum + 1e-6);

        // 如果亮度降低（<1）且色度变化小，则认为是阴影
        if (lumRatio < 1.0 && lumRatio > 0.5) {
            double colorDiff = 0.0;
            for (int c = 0; c < 3; ++c) {
                colorDiff += abs(pixel[c] - gaussians[bestIdx].mean[c]);
            }
            if (colorDiff < 30.0) { // 色度变化小
                return true;
            }
        }

        return false;
    }
};

// 📸 保存图像到文件（带序号）
void saveFrame(const Mat& frame, int frameIdx, const string& prefix = "frame") {
    stringstream ss;
    ss << prefix << "_" << setfill('0') << setw(4) << frameIdx << ".png";
    imwrite(ss.str(), frame);
    cout << "保存: " << ss.str() << endl;
}

// 🧪 测试主函数（保存结果到文件）
int main() {
    VideoCapture cap("video.mp4"); // 替换为你的视频路径
    if (!cap.isOpened()) {
        cout << "无法打开视频文件" << endl;
        return -1;
    }

    Mat frame, mask;
    BackgroundSubtractorMOG2Custom mog2(5, 0.001, 16.0, true);

    // 初始化模型（用第一帧）
    cap >> frame;
    if (frame.empty()) return -1;
    mog2.initialize(frame);

    int frameIdx = 0;
    while (cap.read(frame)) {
        mask = mog2.apply(frame);

        // 保存前景掩码
        saveFrame(mask, frameIdx, "output/foreground_mask");

        // 可选：保存原图（用于对比）
        // saveFrame(frame, frameIdx, "original");

        frameIdx++;
    }

    cap.release();
    cout << "处理完成，共保存 " << frameIdx << " 帧。" << endl;
    return 0;
}
