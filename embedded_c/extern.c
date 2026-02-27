#include <stdio.h>

extern "C" int func(void);

int main(void) {
    func();
    return 0;
}