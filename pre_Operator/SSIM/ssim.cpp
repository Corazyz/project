#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cmath>
#include <cstring>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cfloat>
#include <iomanip>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// 使用命名空间简化
using namespace std;

// 图像结构体
struct Image {
    int width;
    int height;
    int channels;
    vector<float> data;  // 使用 vector 管理内存

    Image(int w, int h, int c) : width(w), height(h), channels(c), data(w * h * c, 0.0f) {}

    // 复制构造函数
    Image(const Image& other) : width(other.width), height(other.height), channels(other.channels), data(other.data) {}

    // 赋值运算符
    Image& operator=(const Image& other) {
        if (this != &other) {
            width = other.width;
            height = other.height;
            channels = other.channels;
            data = other.data;
        }
        return *this;
    }

    // 获取指定通道的指针（用于通道分离）
    float* channel_ptr(int c) {
        return data.data() + c;
    }

    // 获取像素值（线性索引）
    float& operator[](int idx) { return data[idx]; }
    const float& operator[](int idx) const { return data[idx]; }
};

// 创建图像（C++风格）
unique_ptr<Image> create_image(int width, int height, int channels) {
    return make_unique<Image>(width, height, channels);
}

// 高斯分离核生成
int get_gaussian_sep_kernel(int n, float sigma, vector<float>& k_sep) {
    const int SMALL_GAUSSIAN_SIZE = 3;
    static const float small_gaussian_tab[3] = {0.25f, 0.5f, 0.25f};
    const float* fixed_kernel = (n % 2 == 1 && n <= SMALL_GAUSSIAN_SIZE && sigma <= 0) ? small_gaussian_tab : nullptr;
    float sigmaX = sigma > 0 ? sigma : ((n - 1) * 0.5 - 1) * 0.3 + 0.8;
    float scale2X = -0.5f / (sigmaX * sigmaX);
    float sum = 0.0f;

    k_sep.resize(n);
    for (int i = 0; i < n; i++) {
        float x = i - (n - 1) * 0.5f;
        float t = fixed_kernel ? fixed_kernel[i] : expf(scale2X * x * x);
        k_sep[i] = t;
        sum += k_sep[i];
    }
    sum = 1.0f / sum;
    for (int i = 0; i < n; i++) {
        k_sep[i] *= sum;
    }
    return 0;
}

// 生成二维高斯核
void create_gaussian_kernel(vector<float>& kernel, int kw, int kh, float sigma1, float sigma2) {
    if (sigma2 <= 0) sigma2 = sigma1;

    if (kw <= 0 && sigma1 > 0) kw = static_cast<int>(roundf(sigma1 * 3 * 2 + 1)) | 1;
    if (kh <= 0 && sigma2 > 0) kh = static_cast<int>(roundf(sigma2 * 3 * 2 + 1)) | 1;
    sigma1 = sigma1 < 0 ? 0 : sigma1;
    sigma2 = sigma2 < 0 ? 0 : sigma2;

    vector<float> k_sep_x(kw), k_sep_y(kh);

    get_gaussian_sep_kernel(kw, sigma1, k_sep_x);
    if (kh == kw && abs(sigma1 - sigma2) < FLT_EPSILON) {
        get_gaussian_sep_kernel(kw, sigma1, k_sep_y);
    } else {
        get_gaussian_sep_kernel(kh, sigma2, k_sep_y);
    }

    kernel.resize(kh * kw);
    for (int i = 0; i < kh; i++) {
        for (int j = 0; j < kw; j++) {
            kernel[i * kw + j] = k_sep_y[i] * k_sep_x[j];
        }
    }
}

// 高斯滤波（反射边界）
void gaussian_filter(const vector<float>& src, vector<float>& dst, int width, int height, float sigma) {
    int r = static_cast<int>(3.5f * sigma + 0.5f);
    int win_size = 2 * r + 1;
    vector<float> kernel;
    create_gaussian_kernel(kernel, win_size, win_size, sigma, sigma);

    dst.assign(width * height, 0.0f);  // 初始化输出

    for (int y = 0; y < height - win_size + 1; y++) {
        for (int x = 0; x < width - win_size + 1; x++) {
            float val = 0.0f;
            for (int ky = 0; ky < win_size; ky++) {
                for (int kx = 0; kx < win_size; kx++) {
                    val += src[(y + ky) * width + (x + kx)] * kernel[ky * win_size + kx];
                }
            }
            dst[(y + r) * width + (x + r)] = val;
        }
    }
}

// 均值滤波（反射边界）
void uniform_filter(const vector<float>& src, vector<float>& dst, int width, int height, int win_size) {
    int pad = win_size / 2;
    float norm = 1.0f / (win_size * win_size);
    dst.assign(width * height, 0.0f);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float sum = 0.0f;
            for (int dy = -pad; dy <= pad; dy++) {
                for (int dx = -pad; dx <= pad; dx++) {
                    int iy = y + dy;
                    int ix = x + dx;
                    // 反射边界
                    if (iy < 0) iy = -iy;
                    if (iy >= height) iy = 2 * height - 1 - iy;
                    if (ix < 0) ix = -ix;
                    if (ix >= width) ix = 2 * width - 1 - ix;
                    sum += src[iy * width + ix];
                }
            }
            dst[y * width + x] = sum * norm;
        }
    }
}

// 单通道 SSIM
float structural_similarity_channel(
    const vector<float>& im1,
    const vector<float>& im2,
    int width,
    int height,
    int win_size,
    bool gaussian_weights,
    float data_range,
    bool full,
    vector<float>* ssim_map_out
) {
    if (win_size % 2 == 0) {
        throw runtime_error("Error: win_size must be odd.");
    }

    float K1 = 0.01f;
    float K2 = 0.03f;
    float sigma = 1.5f;

    // 临时缓冲区
    vector<float> ux(width * height), uy(width * height);
    vector<float> uxx(width * height), uyy(width * height), uxy(width * height);
    vector<float> vx(width * height), vy(width * height), vxy(width * height);
    vector<float> S(width * height);

    // 计算均值
    if (gaussian_weights) {
        int r = static_cast<int>(3.5f * sigma + 0.5f);
        win_size = 2 * r + 1;

        gaussian_filter(im1, ux, width, height, sigma);
        gaussian_filter(im2, uy, width, height, sigma);

        vector<float> im1_sq(width * height);
        for (int i = 0; i < width * height; i++) im1_sq[i] = im1[i] * im1[i];
        gaussian_filter(im1_sq, uxx, width, height, sigma);

        vector<float> im2_sq(width * height);
        for (int i = 0; i < width * height; i++) im2_sq[i] = im2[i] * im2[i];
        gaussian_filter(im2_sq, uyy, width, height, sigma);

        vector<float> im1_im2(width * height);
        for (int i = 0; i < width * height; i++) im1_im2[i] = im1[i] * im2[i];
        gaussian_filter(im1_im2, uxy, width, height, sigma);
    } else {
        uniform_filter(im1, ux, width, height, win_size);
        uniform_filter(im2, uy, width, height, win_size);

        vector<float> im1_sq(width * height);
        for (int i = 0; i < width * height; i++) im1_sq[i] = im1[i] * im1[i];
        uniform_filter(im1_sq, uxx, width, height, win_size);

        vector<float> im2_sq(width * height);
        for (int i = 0; i < width * height; i++) im2_sq[i] = im2[i] * im2[i];
        uniform_filter(im2_sq, uyy, width, height, win_size);

        vector<float> im1_im2(width * height);
        for (int i = 0; i < width * height; i++) im1_im2[i] = im1[i] * im2[i];
        uniform_filter(im1_im2, uxy, width, height, win_size);
    }

    // 计算方差与协方差
    int np = win_size * win_size;
    float cov_norm = np / (np - 1.0f);  // 无偏估计

    for (int i = 0; i < width * height; i++) {
        vx[i] = cov_norm * (uxx[i] - ux[i] * ux[i]);
        vy[i] = cov_norm * (uyy[i] - uy[i] * uy[i]);
        vxy[i] = cov_norm * (uxy[i] - ux[i] * uy[i]);
    }

    // 计算 SSIM
    float C1 = (K1 * data_range) * (K1 * data_range);
    float C2 = (K2 * data_range) * (K2 * data_range);

    for (int i = 0; i < width * height; i++) {
        float A1 = 2.0f * ux[i] * uy[i] + C1;
        float A2 = 2.0f * vxy[i] + C2;
        float B1 = ux[i] * ux[i] + uy[i] * uy[i] + C1;
        float B2 = vx[i] + vy[i] + C2;
        float D = B1 * B2;

        if (D == 0.0f) {
            S[i] = 1.0f;
        } else {
            S[i] = (A1 * A2) / D;
        }
    }

    // 计算平均 SSIM（忽略边缘）
    int pad = (win_size - 1) / 2;
    double mssim = 0.0;
    int count = 0;

    for (int y = pad; y < height - pad; y++) {
        for (int x = pad; x < width - pad; x++) {
            mssim += S[y * width + x];
            count++;
        }
    }
    mssim /= count;

    // 保存 SSIM 图到文件
    {
        ofstream fp("mssim_c.txt");
        if (fp.is_open()) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    fp << fixed << setprecision(2) << S[y * width + x] << " ";
                }
                fp << "\n";
            }
            cout << "mssim values saved to 'mssim_c.txt'\n";
        }
    }

    // 保存裁剪后的 SSIM 图
    {
        int new_width = width - 2 * pad;
        int new_height = height - 2 * pad;
        vector<float> pad_S(new_width * new_height);

        int idx = 0;
        for (int y = pad; y < height - pad; y++) {
            for (int x = pad; x < width - pad; x++) {
                pad_S[idx++] = S[y * width + x];
            }
        }

        ofstream fp("pad_S_c.txt");
        if (fp.is_open()) {
            for (int y = 0; y < new_height; y++) {
                for (int x = 0; x < new_width; x++) {
                    fp << fixed << setprecision(2) << pad_S[y * new_width + x] << " ";
                }
                fp << "\n";
            }
            cout << "pad_S_c values saved to 'pad_S_c.txt'\n";
        }

        double cmp_mssim = 0.0;
        for (float val : pad_S) cmp_mssim += val;
        cmp_mssim /= pad_S.size();
        cout << "cmp_mssim = " << fixed << setprecision(6) << cmp_mssim << ", count = " << pad_S.size() << endl;
    }

    // 输出完整 SSIM 图
    if (full && ssim_map_out) {
        ssim_map_out->assign(S.begin(), S.end());
    }

    return static_cast<float>(mssim);
}

// 多通道 SSIM（RGB）
float structural_similarity_multi(
    const Image& im1,
    const Image& im2,
    int win_size,
    bool gaussian_weights,
    float data_range,
    bool full,
    vector<float>* ssim_map_out
) {
    if (im1.width != im2.width || im1.height != im2.height || im1.channels != im2.channels) {
        throw runtime_error("Error: Images must have same dimensions and channels.");
    }

    int width = im1.width;
    int height = im1.height;
    int channels = im1.channels;

    float total_ssim = 0.0f;

    for (int c = 0; c < channels; c++) {
        vector<float> tmp1(width * height);
        vector<float> tmp2(width * height);

        for (int i = 0; i < width * height; i++) {
            tmp1[i] = im1.data[i * channels + c];
            tmp2[i] = im2.data[i * channels + c];
        }

        bool is_first = (c == 0 && full && ssim_map_out);
        vector<float> local_ssim_map;

        float ssim_ch = structural_similarity_channel(
            tmp1, tmp2,
            width, height,
            win_size,
            gaussian_weights,
            data_range,
            is_first,
            is_first ? &local_ssim_map : nullptr
        );

        if (is_first && ssim_map_out) {
            ssim_map_out->swap(local_ssim_map);
        }

        total_ssim += ssim_ch;
    }

    return total_ssim / channels;
}

// 读取文本图像（每行每列一个浮点数）
int read_image(const string& filename, vector<float>& image, int rows, int cols) {
    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "错误：无法打开文件 " << filename << endl;
        return -1;
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (!(f >> image[i * cols + j])) {
                cerr << "错误：读取第 " << i+1 << " 行第 " << j+1 << " 列失败\n";
                return -1;
            }
        }
    }

    return 0;
}

// 读取二进制图像（raw）
int read_bin(const string& input_path, vector<unsigned char>& input_data, int width, int height, int channel) {
    ifstream fp_src(input_path, ios::binary);
    if (!fp_src.is_open()) {
        cout << "Can not open file! " << input_path << endl;
        return -1;
    }

    input_data.resize(width * height * channel);
    fp_src.read(reinterpret_cast<char*>(input_data.data()), input_data.size());
    if (fp_src.gcount() != input_data.size()) {
        cout << "读取数据不完整！\n";
        return -1;
    }
    cout << "read image success\n";
    return 0;
}

// 主函数
int main() {
    try {
        int width1 = 3840;
        int height1 = 2160;

        auto im1 = create_image(width1, height1, 1);
        auto im2 = create_image(width1, height1, 1);

        if (read_image("gray1_py.txt", im1->data, height1, width1) != 0) {
            throw runtime_error("读取 gray1_py.txt 失败");
        }

        if (read_image("gray2_py.txt", im2->data, height1, width1) != 0) {
            throw runtime_error("读取 gray2_py.txt 失败");
        }

        // 计算 SSIM
        vector<float> ssim_map;
        float ssim_value = structural_similarity_multi(
            *im1, *im2,
            7, false, 255.0f, true, &ssim_map
        );

        cout << "Mean SSIM (RGB): " << fixed << setprecision(6) << ssim_value << endl;

        // 转换为 unsigned char 用于保存 PNG
        vector<unsigned char> ssim_img(width1 * height1);
        for (int i = 0; i < width1 * height1; i++) {
            float val = ssim_map[i];
            if (val < 0.0f) val = 0.0f;
            if (val > 1.0f) val = 1.0f;
            ssim_img[i] = static_cast<unsigned char>(val * 255.0f);
        }

        // 保存为 PNG
        int success = stbi_write_png("ssim_map.png", width1, height1, 1, ssim_img.data(), width1);
        if (success) {
            cout << "SSIM map saved as 'ssim_map.png'\n";
        } else {
            cerr << "Error: Failed to save SSIM map.\n";
        }

    } catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}
