#include <stdio.h>

void func1(void) {
    printf("This is f1\n");
}

void func2(void) {
    printf("This is f2\n");
}

int main(void) {
    void (*p)(void);    // 函数指针，只能指向无参无返回值的函数
    void (*P)(int, int);    // 另一个函数指针，能够指向两个整形参数
    // 无返回值的函数

    char (*p2)(int); // 函数指针，指向有1个整形参数，一个返回值的函数
    void *(*p3)(int *); // 只能指向这样的函数：

    // 函数指针赋值，必须要格式（函数头）相同
    p = func1;  // 让p这个指针指向func1的整个函数
    // 函数实际上是一个代码块，整个代码块执行到最后会返回
    p();    // 函数指针的运行，实际就是它指向的对象的运行（简写的方法）
    (*p)();     // 原始的运行方法
    p = func2;
    p();

    // 代码唯一的好处是p可以随意指向某个函数
}