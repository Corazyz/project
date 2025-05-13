// #include <stdio.h>
#include <iostream>

int test_ptr_pass(int r, int g, int b){
    r = 30;
    g = 10;
    b = 50;
    printf("r = %d, g = %d, b = %d\n", r, g, b);
    return 0;
}
int main() {
    int r = 20;
    int g = 40;
    int b = 20;
    int ret = 0;
    ret = test_ptr_pass(r, g, b);
    printf("r = %d, g = %d, b = %d\n", r, g, b);
    return 0;
}