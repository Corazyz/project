#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    char* data;
    int top;
    int capacity;
} Stack;

// initailize stack
Stack* createStack(int capacity) {
    Stack* stack = (Stack*)malloc(sizeof(Stack));
    stack->data = (char*)malloc(capacity * sizeof(char));
    stack->top = -1;
    stack->capacity = capacity;
    return stack;
}

// push into stack
void push(Stack* stack, char c) {
    if (stack->top + 1 == stack->capacity) return;  //
    stack->data[++stack->top] = c;
}

// pop from stack
char pop(Stack* stack) {
    if (stack->top == -1) return '\0';
    return stack->data[stack->top--];
}

// stack top element
char peek(Stack* stack) {
    if (stack->top == -1) return '\0';
    return stack->data[stack->top];
}

// check if stack is null
bool isEmpty(Stack* stack) {
    return stack->top == -1;
}

// release stack
void freeStack(Stack* stack) {
    free(stack->data);
    free(stack);
}

// check whether left and rigth brace are paired
bool isMatch(char left, char right) {
    return (left == '(' && right == ')') ||
           (left == '[' && right == ']') ||
           (left == '{' && right == '}');
}

bool isValid(char* s) {
    int len = strlen(s);
    Stack *stack = createStack(len);    // len charactor to push in stack most

    for (int i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        if (c == '(' || c == '[' || c == '{') {
            push(stack, c);
        } else {
            if (isEmpty(stack)) {
                freeStack(stack);
                return false;
            }
            char top = pop(stack);
            if (!isMatch(top, c)) {
                freeStack(stack);
                return false;
            }
        }
    }

    bool result = isEmpty(stack);
    freeStack(stack);
    return result;
}

// test
int main() {
    char* test1 = "()[]{}";
    char* test2 = "(]";
    printf("%s -> %s\n", test1, isValid(test1) ? "true" : "false");
    printf("%s -> %s\n", test2, isValid(test2) ? "true" : "false");
    return 0;
}