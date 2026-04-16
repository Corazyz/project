#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

typedef struct {
    int* data;
    int* minData;
    int top;
    int capacity;
} MinStack;

MinStack* minStackCreate() {
    MinStack* obj = (MinStack*)malloc(sizeof(MinStack));
    obj->capacity = 10000;
    obj->data = (int*)malloc(obj->capacity * sizeof(int));
    obj->minData = (int*)malloc(obj->capacity * sizeof(int));
    obj->top = -1;
    return obj;
}

void minStackPush(MinStack* obj, int val) {
    if (obj->top + 1 >= obj->capacity) return;
    obj->data[++obj->top] = val;
    int minVal = (obj->top == 0) ? val : (val < obj->minData[obj->top - 1] ? val : obj->minData[obj->top - 1]);
    obj->minData[obj->top] = minVal;
}

void minStackPop(MinStack* obj) {
    if (obj->top == -1) return;
    obj->top--;
}

int minStackTop(MinStack* obj) {
    if (obj->top == -1) return -1;
    return obj->data[obj->top];
}

int minStackGetMin(MinStack* obj) {
    if (obj->top == -1) return -1;
    return obj->minData[obj->top];
}

void minStackFree(MinStack* obj) {
    free(obj->data);
    free(obj->minData);
    free(obj);
}

// test
int main() {
    MinStack* stack = minStackCreate();
    minStackPush(stack, -2);
    minStackPush(stack, 1);
    minStackPush(stack, -3);
    printf("Min: %d\n", minStackGetMin(stack));
    minStackPop(stack);
    printf("Top: %d\n", minStackTop(stack));
    printf("Min: %d\n", minStackGetMin(stack));
    minStackFree(stack);
    return 0;
}