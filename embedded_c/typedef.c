// typedef关键字 是给数据类型起一个别名，特别是复杂的类型（如函数指针）
// typedef 数据类型 别名

#include <stdio.h>

// typedef struct abc {
//     int a;
//     int b;
//     float c;
// } abc_t;

// typedef int len_t;
// typedef unsigned char uint8_t;
// typedef void (*func)(void);

// int main(void) {
//     len_t a = 0;
//     uint8_t b;
//     abc_t c;

//     return 0;
// }

#define uchar_p unsigned char *
typedef unsigned char * uint8_p;

int main(void) {
    uchar_p a, *b;
    uint8_p c, d;

    a = "hello";
    b = "hello";
    c = "hello";
    d = "hello";

    return 0;
}
