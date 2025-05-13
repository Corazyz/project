#include <stdio.h>

namespace myname {
    // int var = 42;
    float func(float);
}

extern "C" {
    float _Z4funcf;
};

int main() {
    printf("%f\n", _Z4funcf);
    return 0;
}


// #include <stdio.h>

// namespace myname {
//     int var = 42;

//     // 提供一个外部可见的接口
//     extern "C" int get_var() {
//         return myname::var;
//     }
// }

// extern "C" int get_var();

// int main() {
//     printf("%d\n", get_var());
//     return 0;
// }