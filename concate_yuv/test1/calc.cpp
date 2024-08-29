#include <stdio.h>
#include "head.h"

int main() {
    int a = 20;
    int b = 12;
    printf("a = %d, b = %d", a, b);
    printf("a + b = %d\n", add(a, b));
    printf("a - b = %d\n", subtract(a, b));
    printf("a * b = %d\n", multiply(a, b));
    printf("a / b = %f\n", div(a, b));
    return 0;
}
