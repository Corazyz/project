// #include <stdio.h>

// int main() {
//     int b = 10;
//     int *a = &b;
//     printf("*a = %d, *&b = %d\n", *a, *&b);
//     printf("address of b is: a: %p, &b: %p\n", a, &b);
//     printf("address of a is: &a: %p\n", &a);
// }

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

void parse_pointer_params(test1* model) {   // void* model
    test1 *param = model;
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
    test1 member1;
    // &member1 = (test1*)calloc(1, sizeof(test1));
    // parse_object_params(member1);
    parse_pointer_params(&member1);

    cout << "member1.feature1: " << member1.feature1 << endl;
    cout << "member1.feature2: " << member1.feature2 << endl;
    cout << "member1.feature3: " << member1.feature3 << endl;
    for (int i=0; i<3; i++) {
        cout << "member1.feature4: " << member1.feature4[i] << endl;
    }
    for (int i = 0; i < 3; i ++ ) {
        for (int j = 0; j < 6; j ++) {
            cout << "member1.feature4; " << member1.feature4[i][j] << ",";
        }
        cout << endl;
    }

    // test1 *member2;
    // member2 = (test1*)calloc(1, sizeof(test1));
    // parse_pointer_params(member2);

    // cout << "member2->feature1: " << member2->feature1 << endl;
    // cout << "member2->feature2: " << member2->feature2 << endl;
    // cout << "member2->feature3: " << member2->feature3 << endl;
    // for (int i=0; i<3; i++) {
    //     cout << "member2->feature4: " << member2->feature4[i] << endl;
    // }
    // for (int i = 0; i < 3; i ++ ) {
    //     for (int j = 0; j < 6; j ++) {
    //         cout << "member2->feature4; " << member2->feature4[i][j] << ",";
    //     }
    //     cout << endl;
    // }
}