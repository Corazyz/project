#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>

// #define MAX 50
// int number;
// pthread_mutex_t mutex;

// void *funcA_num(void *arg) {
//     for (int i = 0; i < MAX; i++) {
//         pthread_mutex_lock(&mutex);
//         int cur = number;
//         cur++;
//         // usleep(100);
//         number = cur;
//         printf("thread A, id = %ld, number = %d\n", pthread_self(), number);
//         pthread_mutex_unlock(&mutex);
//     }
//     return NULL;
// }
// void *funcB_num(void *arg) {

//     for (int i = 0; i < MAX; i++) {
//         pthread_mutex_lock(&mutex);
//         int cur = number;
//         cur++;
//         number = cur;
//         printf("thread B, id = %ld, number = %d\n", pthread_self(), number);
//         pthread_mutex_unlock(&mutex);
//         usleep(5);
//     }
//     return NULL;
// }

// int main(int argc, const char* argv[]){
//     pthread_t p1, p2;
//     pthread_mutex_init(&mutex, NULL);

//     pthread_create(&p1, NULL, funcA_num, NULL);
//     pthread_create(&p2, NULL, funcB_num, NULL);

//     pthread_join(p1, NULL);
//     pthread_join(p2, NULL);

//     pthread_mutex_destroy(&mutex);

//     return 0;
// }