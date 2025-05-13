#include "thread.h"
#include "stdio.h"
void T_a() { while (1) {printf("a");}}
void T_b() { while (1) {printf("b");}}

int main() {
    create(T_a);
    create(T_b);
}