#include <iostream>
#include "stdio.h"
extern "C"
{
#include "libavcodec/avcodec.h"
}
int main() {
    printf("%s\n", avcodec_configuration());
    return 0;
}