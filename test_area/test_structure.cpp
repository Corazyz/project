#include <iostream>

using namespace std;

typedef struct test_struct {
    int feature1;
    int feature2;
    int feature3;
    float* feature4[3];
    float* feature5[3];
} test1;

void load_params(float* array, int size) {
    float test_data[] = {0.1, 0.2, 0.1, 0.1, 0.3, 0.4};
    for (int i = 0; i < size; i++) {
        array[i] = test_data[i];
    }
}

void parse_params(test1* model) {   // void* model
    test1 *param = model;       //REVIEW 为什么一定要声明（test1*） 因为函数声明为void* model，所以要类型转换
    param->feature1 = 1;
    param->feature2 = 2;
    param->feature3 = 3;
    for (int i = 0; i < 3; i++) {
        param->feature4[i] = (float*)malloc(param->feature2 * param->feature3 * sizeof(float));
    }
    for (int j = 0; j < 3; j++) {
        load_params(param->feature4[j], param->feature2 * param->feature3);
    }
}

int main() {
    test1* model;
    model = (test1*)calloc(1, sizeof(test1));
    parse_params(model);
    cout << "model->feature1: " << model->feature1 << endl;
    cout << "model->feature2: " << model->feature2 << endl;
    cout << "model->feature3: " << model->feature3 << endl;
    for (int i=0; i<3; i++) {
        cout << "model->feature4: " << model->feature4[i] << endl;
    }
    for (int i = 0; i < 3; i ++ ) {
        for (int j = 0; j < 6; j ++) {
            cout << "model->feature4; " << model->feature4[i][j] << ",";
        }
        cout << endl;
    }
}