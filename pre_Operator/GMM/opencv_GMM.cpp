#include <iostream>
#include <stdio.h>

using namespace std;
int main() {
    unsigned char* array;
    array = (unsigned char*)malloc(1920*1080*3);
    int array_size = 1920*1080*3;
    if (array == NULL) {
        std::cerr << "Memory allocation failed" << std::endl;
        return -1;
    }
    int file_size;
    FILE *file = fopen("/home/zyz/projects/pre_Operator/GMM/gmm_img/image_001.bin", "rb");
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);
    cout << "file size: " << file_size << endl;
    size_t read_count = fread(array, sizeof(unsigned char), array_size, file);
    cout << "read_count: " << read_count << endl;
    cout << "array content test: " << static_cast<int>(array[0]) << endl;

    return 0;
}