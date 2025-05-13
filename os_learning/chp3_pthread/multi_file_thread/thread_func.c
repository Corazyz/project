#include <stdio.h>
#include <pthread.h>
#include "thread_func.h"
// void* process_file(void* arg) {
//     Thread_param* param = (Thread_param*)arg;
//     printf("Processing file: %s\n", param->file_name);
//     return NULL;
// }
int thread_call_func(Thread_param* param){
    printf("thread_call_func: param.filename = %s\n", param->file_name);
    return 0;
}
