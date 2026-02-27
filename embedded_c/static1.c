#include <stdio.h>
extern int a;

int func1(void) {
    a++;
    printf("a = %d\n", a);

    return a;
}
