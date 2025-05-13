#include "stdio.h"

#define NUMBER 3

int main() {
    int a = 10;
#ifdef DEBUG
    printf("define debug\n");
#endif
    for (int i = 0; i < 3; i++) {
        printf("Hello, gcc\n");
    }
    return 0;
}