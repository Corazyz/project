// #include <iostream>
// #include <stdio.h>

// using namespace std;
// int main() {
//     unsigned char* array;
//     array = (unsigned char*)malloc(1920*1080*3);
//     int array_size = 1920*1080*3;
//     if (array == NULL) {
//         std::cerr << "Memory allocation failed" << std::endl;
//         return -1;
//     }
//     int file_size;
//     FILE *file = fopen("/home/zyz/projects/pre_Operator/GMM/gmm_img/image_001.bin", "rb");
//     fseek(file, 0, SEEK_END);
//     file_size = ftell(file);
//     rewind(file);
//     cout << "file size: " << file_size << endl;
//     size_t read_count = fread(array, sizeof(unsigned char), array_size, file);
//     cout << "read_count: " << read_count << endl;
//     cout << "array content test: " << static_cast<int>(array[0]) << endl;

//     return 0;
// }


#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>

int main() {
    if (!std::filesystem::exists("output")) {
        std::filesystem::create_directory("output");
        std::cout << "📁 创建 output 文件夹成功。" << std::endl;
    }
    std::string video_path = "video.mp4"; // 请替换为你自己的视频文件路径

    // 检查文件是否存在
    if (!std::filesystem::exists(video_path)) {
        std::cerr << "❌ 错误：视频文件 \"" << video_path << "\" 不存在！" << std::endl;
        std::cerr << "请将视频文件放在程序同目录下，或修改代码中的路径。" << std::endl;
        std::cerr << "示例：将 'sample.mp4' 放入程序目录，然后修改 video_path = \"sample.mp4\";" << std::endl;
        return -1;
    }

    // 打开视频
    cv::VideoCapture cap(video_path);

    if (!cap.isOpened()) {
        std::cerr << "❌ 无法打开视频文件：" << video_path << std::endl;
        std::cerr << "可能原因：" << std::endl;
        std::cerr << "  - 视频格式不支持（如 HEVC/H.265）" << std::endl;
        std::cerr << "  - 缺少视频解码器（如 ffmpeg）" << std::endl;
        std::cerr << "  - OpenCV 未编译支持视频 I/O" << std::endl;
        return -1;
    }

    // 显示 OpenCV 版本
    std::cout << "✅ OpenCV 版本: " << CV_VERSION << std::endl;

    // 显示视频信息
    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap.get(cv::CAP_PROP_FPS);
    std::cout << "🎥 视频尺寸: " << width << "x" << height << std::endl;
    std::cout << "⏱️  帧率: " << fps << " fps" << std::endl;

    // 创建背景减除器
    cv::Ptr<cv::BackgroundSubtractorMOG2> mog2 =
        cv::createBackgroundSubtractorMOG2(500, 16, true);

    cv::Mat frame, fgmask;

    std::cout << "▶️ 正在播放视频：" << video_path << std::endl;
    std::cout << "按 ESC 键退出程序..." << std::endl;

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            std::cout << "🎬 视频播放结束。" << std::endl;
            break;
        }

        // 应用背景减除
        mog2->apply(frame, fgmask);

        static int frame_count = 0;
        cv::imwrite("output/frame_" + std::to_string(frame_count++) + ".png", fgmask);
    }

    cap.release();
    cv::destroyAllWindows();

    std::cout << "✅ 程序正常结束。" << std::endl;
    return 0;
}
