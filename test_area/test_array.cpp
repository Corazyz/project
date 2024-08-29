#include <iostream>

using namespace std;

int main() {
    // unsigned char test[5] = {0};
    // cout << "test output: " << &test << endl;
    // return 0;
    int frame = 1920*1080;
    // cout << "test in_ptr: " << static_cast<int>(*(in_ptr[0]+1)) << endl;
    cout << "get test_cpu successfully! " << endl;
    float *mask[5];
    // float mask[5][frame];
    // float* mask[5] = malloc
    // cout << "test error: " << endl;
    for (int i = 0; i < 5; i++) {
        mask[i] = (float*)malloc(frame*sizeof(float));
        // memset(mask[i], 0, frame_size*sizeof(float));
        for (int j = 0; j < frame; j++) {
            mask[i][j] = 0;
        }
        cout << "test mask: " << mask[i][0] << endl;
    }

    // free(mask);
    return 0;
}