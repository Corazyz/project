#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int* data;
    int front;
    int rear;
    int capacity;
} Queue;

Queue* createQueue(int capacity) {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->data = (int*)malloc(capacity * sizeof(int));
    q->front = 0;
    q->rear = -1;
    q->capacity = capacity;
    return q;
}

void enqueue(Queue* q, int val) {
    if (q->rear + 1 >= q->capacity) return;
    q->data[++q->rear] = val;
}
int dequeue(Queue* q) {
    if (q->front > q->rear) return -1;
    return q->data[q->front++];
}
bool isEmpty(Queue* q) {
    return q->front > q->rear;
}
int size(Queue* q) {
    return q->rear - q->front + 1;
}
void freeQueue(Queue* q) {
    free(q->data);
    free(q);
}

// stack structure(realize using 2 queue)
typedef struct {
    Queue* q1;
    Queue* q2;
} MyStack;

MyStack* myStackCreate() {
    MyStack* obj = (MyStack*)malloc(sizeof(MyStack));
    obj->q1 = createQueue(100);
    obj->q2 = createQueue(100);
    return obj;
}

void myStackPush(MyStack* obj, int x) {
    if (isEmpty(obj->q2)) {
        enqueue(obj->q1, x);
    } else {
        enqueue(obj->q2, x);
    }
}

int myStackPop(MyStack* obj) {
    Queue* main = isEmpty(obj->q2) ? obj->q1 : obj->q2;
    Queue* helper = isEmpty(obj->q2) ? obj->q2 : obj->q1;

    int n = size(main) - 1;
    for (int i = 0; i < n; i++) {
        enqueue(helper, dequeue(main));
    }
    int val = dequeue(main);
    return val;
}

int myStackTop(MyStack* obj) {
    Queue* main = isEmpty(obj->q2)
}