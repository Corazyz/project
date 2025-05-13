#include <unistd.h>
#include <stdio.h>
// 希望在run循环里，每隔1秒运行一个函数，这个函数由main来指定（函数放在mian里）

// void (*step)(int) = NULL; // 定义一个函数指针，在循环里运行

// // 算秒的函数，每秒自增1
// void run(void) {
//     int c = 0;
//     while(1) {
//         if (step != NULL)
//             step(c);
//         c++;
//         sleep(1);
//     }
// }


// void cb_install(void (*p)(int)) {
//     step = p;
// }

int (*step)(int) = NULL; // 定义一个函数指针，在循环里运行

// 算秒的函数，每秒自增1
void run(void) {
    int c = 0;
    while(1) {
        if (step != NULL) {
            int r = step(c);
            printf("r = %d\n", r);
        }

        c++;
        sleep(1);
    }
}


void cb_install(int (*p)(int)) {
    step = p;
}