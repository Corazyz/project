#include <stdio.h>
int a;
extern int func1();
int func(void) {
    static int b = 10;
    b = b + 1;
    return b;
}

int main(void) {
    printf("b = %d\n", func());
    printf("b = %d\n", func());
    printf("a = %d\n", func1());
    printf("a = %d\n", func1());
    return 0;
}