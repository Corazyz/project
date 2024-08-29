#include <stdio.h>
#include <string.h>

int main() {
    char buffer[31];
    int num = 42;
    double pi = 3.14159;

    // 使用 snprintf 格式化字符串
    int result = snprintf(buffer, sizeof(buffer), "The number is %d and pi is %.2f", num, pi);
    printf("size of result: %d\n", result);
    if (result < sizeof(buffer)) {
        printf("Formatted string: %s\n", buffer);
    } else {
        printf("Buffer was too small. Needed at least %d characters.\n", result + 1);
    }

    return 0;
}