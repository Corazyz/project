// #include "stdafx.h"
#include "stdio.h"

extern int gdata10;
extern int sum(int, int);

int gdata1 = 10;
int gdata2 = 0;
int gdata3;

static int gdata4 = 11;
static int gdata5 = 0;
static int gdata6;

int main() {
    int a = 12;
    int b = 0;
    int c = gdata10;
    sum(a, c);
    // printf("%d\n", sum(a, c));

    static int d = 13;
    static int e = 0;
    static int f;

    getchar();
    getchar();
    return 0;
}