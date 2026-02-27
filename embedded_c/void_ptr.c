#include <stdio.h>
#include <string.h>


// void:
// 1. 占位符：表示无或者空
// 2. void*：表示数据空间
// 3. 表示unused


void func(void) {
    return 10;
}

int main(void) {
    int a = 0;
    int b = 0x87654321;
    memcpy(&a, &b, 2);
    printf("a = %x \n", a);
    (void)a;
    return 0;
}