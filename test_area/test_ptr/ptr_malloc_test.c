#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"

// typedef struct {
//     float a1;
//     float a2;
//     float a3;
// } a_member;
// void main() {
//     // a_member* a = (a_member*)malloc(3 * sizeof(a_member));
//     // a[1].a1 = 0.1;
//     // a[0].a1 = 0.0;
//     // a[2].a1 = 0.2;
//     // for (int i = 0; i < 3; i++) {
//     //     a[i].a1 += 1.0;
//     //     printf("%f, ", a[i].a1);
//     // }
//     // int arr[5] = {1, 2, 3, 4, 5};
//     // printf("&arr[0] == arr ? %d, arr[0] = %d\n", &arr[0] == arr, arr[0]);
//     // printf("\n");
//     // free(a);
// }
typedef struct {
    int member1;
    int member2;
} mem;
int main() {
    mem b[4] = {0};
    b[3].member1 = 2;
    b[2].member2 = 3;
    for (int i = 0; i < 4; i++) {
        printf("b[%d].m1 = %d, b[%d].m2 = %d\n", i, (b+i)->member1, i, (b+i)->member2);
    }
    mem *a[5];
    a[0] = (mem*)malloc(6 * sizeof(mem));
    for (int i = 0; i < 6; i++) {
        a[0][i].member1 = i;
        a[0][i].member2 = i * 2;
    }
    printf("a[0][0].mb1 = %d\n", a[0][0].member1);
}