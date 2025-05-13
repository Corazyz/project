#include <stdio.h>

// void foobar(int i) __attribute__((visibility("default")));

void foobar(int i) {
    printf("Printing from Lib.so %d\n", i);
    sleep(-1);
}
