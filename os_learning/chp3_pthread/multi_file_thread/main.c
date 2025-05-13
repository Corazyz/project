#include <stdio.h>
#include <pthread.h>
#include "thread_func.h"
#include <stdlib.h>

int g_1 = 4;
int g_2 = 5;
int g_3 = 6;
// typedef struct {
//     int a;
//     int b;
//     int c;
//     char* file_name;
// } Thread_param;

void* process_file(void* arg) {
    Thread_param* param = (Thread_param*)arg;
    printf("Processing file: %s\n", param->file_name);
    thread_call_func(param);

    return NULL;
}

int main() {
    pthread_t threads[3];
    char *files[] = {"file2.txt", "file2.txt", "file3.txt"};
    Thread_param *Param = (Thread_param*)malloc(sizeof(Thread_param));
    Param->a = 3;
    Param->b = 4;
    Param->c = 5;
    Param->file_name = "file.txt";
    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, process_file, (void*)Param);
    }
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}