// 被修饰的变量是 read only的，编译器尽量不让修改，const修饰的变量技术上能改，但是不要去改
// const 数据类型 变量名 or 数据类型 const 变量名
// gcc -O2 -g -o bin const.c 打开优化，可以将const修饰变量视作立即数

// #include <stdio.h>


// int main(void) {
//     int a = 10;
//     int const b = 20;
//     int *p = &a;

//     p[1] = 30;
//     printf("a = %p, b = %p\n", &a, &b);
//     printf("a = %d, b = %d\n", a, b);
//     return 0;
// }


// #include <stdio.h>

// const int *a;
// int const *b;
// int * const c;
// const int * const d;

// int main() {
//     a = 1;
//     b = 1;
//     *c = 1;
//     return 0;
// }

#include <stdio.h>

int main() {
    int shadow_ABC = 10;
    const int ABC = 20;
    printf("ABC = %p, shadow_ABC = %p\n", &ABC, &shadow_ABC);
    int *p = &shadow_ABC;
    p[1] = 30;
    int a = ABC;
    printf("a = %d\n", a);
    return 0;
}