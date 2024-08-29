#include <iostream>
using namespace std;

int main() {
    float a[] = {0, 1, 0, 1, 0, 1};

    float b[3][2] = {{0, 1}, {0, 1}, {0, 1}};  // 多维数组的初始化
    cout << "size of a: " << sizeof(a)/sizeof(float) << endl;
    cout << "size of b[0]: " << sizeof(b[0])/sizeof(float) << endl;
    cout << "size of b[1]: " << sizeof(b[1])/sizeof(float) << endl;
    cout << "size of b: " << sizeof(b)/sizeof(b[0]) << endl;
    return 0;
}