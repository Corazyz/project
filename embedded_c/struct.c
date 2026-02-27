#include <stdio.h>
// struct abc {
//     char b;
//     char c;
//     int a;
// };

#pragma anon_unions

struct abc {
    int a;
    short b;
    long c;
} __attribute__((packed));

struct cfg {
    char e;
    int a;
    char c;
};

// union 共享首地址
union sensor {
    char b;
    int a;
};

int main(void) {
    // struct abc c = {
    //     .b = 'c',
    //     .a = 100,
    // };
    // struct abc c;
    // struct cfg d;
    // c.a = 100;
    // c.b = 'c';
    union sensor c = {
        .b = 1,
    };
    printf("a = %d b = %d \n", c.a, c.b);
    c.a = 0x3210;
    printf("a = 0x%x b = 0x%x \n", c.a, c.b);
    // printf("a = %d b = %c sizeof(c.a) = %ld sizeof(c.b) = %ld sizeof(c) = %ld \n", c.a, c.b, sizeof(c.a), sizeof(c.b), sizeof(c));
    // printf("sizeof(abc) = %ld sizeof(cfg) = %ld \n", sizeof(struct abc), sizeof(struct cfg));
    printf("sizeof int short long: %ld %ld %ld \n", sizeof(int), sizeof(short), sizeof(long));
    printf("sizeof struct abc: %ld \n", sizeof(struct abc));
    return 0;
}