// volatile 关键字告诉编译器，声明为 volatile 的变量的值可能在程序的控制之外发生变化，因此编译器不应该对其进行某些优化，主要用于处理硬件寄存器，中断服务程序和多线程等情况
// 优化：gcc -o bin volatile.c -lpthread -O3 -g
// gdb调试：gdb ./bin -q
// gdb调试线程：ps -A | grep "bin"
//             sudo gdb -p 1394322


// #include <stdio.h>

// volatile unsigned char buff;

// int main() {
//     while (buff) { // 等待 buff 不为0
//         // do nothing
//     }
//     return 0;
// }

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

volatile int wait = 1;
void* function(void *arg) {
    while (1) {
        sleep(1);
        wait = 0;
        printf("flag wait = %d in thread \n", wait);
    }
    return NULL;
}

int main(void) {
    pthread_t tid;
    pthread_create(&tid, NULL, function, NULL);

    while(wait);
    printf("flag wait = %d in main \n", wait);
    return 0;
}