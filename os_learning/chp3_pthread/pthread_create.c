#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

struct Test {
    int num;
    int age;
};

void *callback(void *arg) {
    for(int i = 0; i < 5; i++) {
        printf("sub thread: i = %d\n", i);
    }
    printf("sub thread: %ld\n", pthread_self());
    struct Test *t = (struct Test*)arg;
    t->num = 100;
    t->age = 6;
    pthread_exit(&t);
    return NULL;
}
int main() {
    pthread_t tid;
    struct Test *t;
    pthread_create(&tid, NULL, callback, t);
    for (int i = 0; i < 5; i++) {
        printf("main thread: i = %d\n", i);
    }
    printf("main thread: %ld\n", pthread_self());
    pthread_detach(tid);
    // pthread_exit(NULL);
    // void *ptr;
    // pthread_join(tid, NULL);
    // struct Test* pt = (struct Test*)ptr;
    printf("num = %d, age = %d\n", t->num, t->age);
    pthread_exit(NULL);
    return 0;
}