#include <stdio.h>

// 使用enum的地方基本可以用宏替代

enum abc {
    A = 0,
    B = 1,
    C = 3,
    D
};

enum efg {
    E = 0x123456789,
    F,
    G
};

int main(void) {
    printf("a = %d, b = %d, c = %d, d = %d \n", A, B, C, D);
    printf("sizeof a = %ld, abc = %ld \n", sizeof(A), sizeof(enum abc));
    printf("sizeof E = %ld, efg = %ld \n", sizeof(E), sizeof(enum efg));
    return 0;
}