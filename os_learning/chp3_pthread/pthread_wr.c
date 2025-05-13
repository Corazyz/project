#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define MAX 50
int number;
pthread_rwlock_t rwlock;

void *writeNum(void *arg) {
    for (int i = 0; i < MAX; i++) {
        pthread_rwlock_wrlock(&rwlock);
        int cur = number;
        cur++;
        number = cur;
        printf("thread write, number = %d, tid = %ld\n", number, pthread_self());
        pthread_rwlock_unlock(&rwlock);
    }
    return NULL;
}
void *readNum(void *arg) {

    for (int i = 0; i < MAX; i++) {
        pthread_rwlock_rdlock(&rwlock);
        // int cur = number;
        // cur++;
        // number = cur;
        printf("thread read, id = %lu, number = %d\n", pthread_self(), number);
        pthread_rwlock_unlock(&rwlock);
        usleep(rand()%5);
    }
    return NULL;
}

int main(int argc, const char* argv[]){
    pthread_t p1[5], p2[3];
    pthread_rwlock_init(&rwlock, NULL);

    for (int i = 0; i < 5; i++) {
        pthread_create(&p1[i], NULL, readNum, NULL);
    }
    for (int i = 0; i < 3; i++) {
        pthread_create(&p2[i], NULL, writeNum, NULL);
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(p1[i], NULL);
    }
    for (int i = 0; i < 3; i++) {
        pthread_join(p2[i], NULL);
    }

    pthread_rwlock_destroy(&rwlock);

    return 0;
}