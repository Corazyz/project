//

#include <stdio.h>

// int main(void) {
//     int a = 2;
//     if (a == 1) {
//         printf("hello 1\n");
//     } else if (2 == a) {
//         printf("hello 2\n");
//     } else {
//         printf("hello a = %d\n", a);
//     }
//     return 0;
// }

int main(void) {
    int a = 2;
    switch (a) {
    case 1:
        printf("hello 1\n");
        break;
    case 2:
        printf("hello 2\n");
        // break;
    case 3:
        printf("hello 3\n");
        break;
    default:
        printf("hello a = %d\n", a);
    }
    return 0;
}