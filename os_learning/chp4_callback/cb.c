#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

void *cb(void *arg) {
    printf("call back running %lu\n", pthread_self());
    return NULL;
}

int main(void) {
    pthread_t tid;
    pthread_create(&tid, NULL, cb, NULL);
    printf("tid = %lu\n", tid);
    pthread_t tid2;
    pthread_create(&tid2, NULL, cb, NULL);

    pthread_join(tid, NULL);
    pthread_join(tid2, NULL);
    return 0;
}