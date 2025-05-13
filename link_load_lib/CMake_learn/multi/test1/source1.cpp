#include <stdio.h>
#include "calc.h"
int main() {
    printf("This is test1\n");
    int a = 10;
    int b = 5;
    printf("a = %d, b = %d\n", a, b);
    printf("a + b = %d\n", add(a, b));
    printf("a - b = %d\n", substract(a, b));
    printf("a * b = %d\n", multiply(a, b));
    printf("a / b = %f\n", divide(a, b));
    return 0;
}