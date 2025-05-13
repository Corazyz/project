#include <stdio.h>
extern int shared;

void swap(int* a, int* b);
int main() {
    int a = 100;
    printf("before swap: a = %d, shared = %d\n", a, shared);
    swap(&a, &shared);
    printf("after swap: a = %d, shared = %d\n", a, shared);
}