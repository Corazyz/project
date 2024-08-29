#include <cstring>
#include <iostream>

int main() {
    char* dst = (char*)malloc(100);  // 分配内存
    int count = 40;  // 填充长度
    int val = 0;  // 填充值
    printf("sizeof(long): %ld\n", sizeof(long));
    printf("(unsigned long)dst: %ld\n", (unsigned long)dst);
    printf("%d\n", ((unsigned long)dst % sizeof(long))==0);

    //REVIEW - 为什么转换了地址之后能判断是否会溢出
    if (((unsigned long)dst % sizeof(long)) == 0 &&
        (count % sizeof(long)) == 0 &&
        (val == 0)) {
        // 使用 memset 快速清零内存区域
        memset(dst, 0, count);
    } else {
        // 使用循环逐字节填充
        for (int i = 0; i < count; ++i) {
            dst[i] = val;
        }
    }

    // 输出填充后的内存内容
    for (int i = 0; i < count; ++i) {
        std::cout << "dst[" << i << "] = " << (int)dst[i] << std::endl;
    }

    free(dst);  // 释放内存

    return 0;
}