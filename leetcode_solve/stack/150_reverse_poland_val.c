#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

typedef struct {
    int* data;
    int top;
    int capacity;
} Stack;

Stack* createStack(int capacity) {
    Stack* stack = (Stack*)malloc(sizeof(stack));
    stack->data = (int*)malloc(capacity * sizeof(int));
    stack->top = -1;
    stack->capacity = capacity;
    return stack;
}

void push(Stack* stack, int val) {
    if (stack->top + 1 >= stack->capacity) return;
    stack->data[++stack->top] = val;
}
int pop(Stack* stack) {
    if (stack->top == -1) return 0;
    return (stack->data[stack->top--]);
}
int top(Stack* stack) {
    if (stack->top == -1) return 0;
    return (stack->data[stack->top]);
}
bool isEmpty(Stack* stack) {
    return stack->top == -1;
}
bool freeStack(Stack* stack) {
    free(stack->data);
    free(stack);
}

// ckeck whether item is a number(support negative num)
bool isNumber(char* token) {
    if (token[0] == '-' && strlen(token) > 1) return true;
    for (int i = 0; token[i] != '\0'; i++) {
        if (!isdigit(token[i])) return false;
    }
    return true;
}

int evalRPN(char** tokens, int tokensSize) {
    Stack* stack = createStack(tokensSize);

    for (int i = 0; i < tokensSize; i++) {
        char* token = tokens[i];
        if (isNumber(token)) {
            int num = atoi(token);
            push(stack, num);
        } else {
            int b = pop(stack);
            int a = pop(stack);
            int result;
            if (strcmp(token, "+") == 0) result = a + b;
            else if (strcmp(token, "-") == 0) result = a - b;
            else if (strcmp(token, "*") == 0) result = a * b;
            else if (strcmp(token, "/") == 0) result = a / b;
            push(stack, result);
        }
    }
    int result = top(stack);
    freeStack(stack);
    return result;
}

// test
int main() {
    char* tokens1[] = {"2", "1", "+", "3", "*"};
    int size1 = 5;
    printf("result1: %d\n", evalRPN(tokens1, size1));

    char* tokens2[] = {"-2", "1", "*", "5", "*"};
    int size2 = 5;
    printf("result2: %d\n", evalRPN(tokens2, size2));

    char* tokens3[] = {"4", "13", "-5", "/", "+"};
    int size3 = 5;
    printf("result3: %d\n", evalRPN(tokens3, size3));

    return 0;
}