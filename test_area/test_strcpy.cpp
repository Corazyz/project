#include <cstring>  // 或 #include <string.h> for strcpy
#include <iostream>

int main() {
    char source[100] = "Hello, World!";
    char destination[100] = "Original content.";

    std::cout << "strcmp result: " << strcmp(source, destination) << " further result: " << (strcmp(source, destination) == 0) << std::endl;

    // 检查目标缓冲区的大小
    if (sizeof(destination) >= strlen(source) + 1) {
        // 使用 strcpy 复制字符串
        strcpy(destination, source);
    } else {
        std::cerr << "Destination buffer is too small." << std::endl;
        return 1;
    }
    // 输出结果
    std::cout << "Source: " << source << std::endl;
    std::cout << "Destination: " << destination << std::endl;

    return 0;
}