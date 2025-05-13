#include "stdio.h"
#include "string.h"
// int* get(int a, int b) {
//     int c = 0;
//     c = a + b;
//     int *d = &c;
//     printf("%p\n", &c);
//     return d;
//     // return &c;
// }

int *get(int *a, int *b) {
    *a = *a + *b;
    return a;
}

int main(void) {
    int a = 4;
    int *p = &a;
    int b = 5;
    int *q = &b;

    printf("%p\n", p);
    printf("%d\n", *p);

    printf("%p\n", q);
    printf("%d\n", *q);
    int *res = get(p, q);
    printf("%p\n", res);
    printf("%d\n", *res);
    // int *p = get(4, 5);
    // printf("%p\n", p);
    // printf("%d\n", *p);
    return 0;
}
// #include <stdio.h>
// int *global = NULL;
// int *f(int c) {
//     int b = c;
//     int *p1 = &b;
//     global = &b;
//     return p1;
// }

// int main() {
//     int *p = f(6);
//     printf("p_addr: %p\n", p);
//     printf("p_num: %d\n", *p);
//     printf("global_addr: %p\n", global);
//     printf("global_num: %d\n", *global);
//     printf("*f(7) return: %d\n", *f(7));
//     printf("p_addr: %p\n", p);
//     printf("p_num: %d\n", *p);
//     printf("global_addr: %p\n", global);
//     printf("global_num: %d\n", *global);
//     return 0;
// }