#include <stdio.h>

// 先声明，再使用

// extern void (*step)(void);

// void cb_install(void (*p)(int));
// void mystep_cb(int i) {
//     printf("second %d\n", i);
// }

// int main(void) {
//     // step = mystep_cb;
//     cb_install(mystep_cb);
//     run();
// }

void cb_install(int (*p)(int));
int mystep_cb(int i) {
    printf("second %d\n", i);
    return 2025;
}

int main(void) {
    // step = mystep_cb;
    cb_install(mystep_cb);
    run();
}