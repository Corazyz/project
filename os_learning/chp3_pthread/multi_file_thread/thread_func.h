#ifndef _thread_func_h
#define _thread_func_h
// void* process_file(void* arg);
typedef struct {
    int a;
    int b;
    int c;
    char* file_name;
} Thread_param;

int thread_call_func(Thread_param* param);
#endif