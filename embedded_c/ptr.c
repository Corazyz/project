// 地址体现cpu的寻址能力
// 32位系统 -> 4byte
// 64位系统 -> 8byte
// 指针类型内存大小与数据类型没有关系
// 指针变量也是一种变量，大小跟cpu地址总线有关

#include <stdio.h>

struct abc {
    int a;
    int b;
    float c;
};

int main(void) {
    char *p1;
    int *p2;
    struct abc *p3;
    printf("size of p1: %ld, p2: %ld, p3: %ld\n", sizeof(*p1), sizeof(*p2), sizeof(*p3));

    int a = 10;
    int *p = &a;
    printf("symbol visit: a = %x, addr visit: a = %d, addr_a = %p, symbol_addr_a = %p\n", a, *p, p, &a);
    return 0;
}