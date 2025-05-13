#include <stdio.h>
#include <unistd.h>
int main() {
    printf("hello\n");
    // fflush(stdout);
    // sleep(5);
    // printf(" world!\n");
    fork();
    return 0;
}