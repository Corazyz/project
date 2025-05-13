char* str = "Hello world!\n";

void print() {
    asm("movl $13, %%edx \n\t"
        "movl %0, %%ecx \n\t"
        "movl $0, %%ebx \n\t"
        "movl $4, %%eax \n\t"
        "int $0x80 \n\t"
        ::"r"(str):"edx","ecx","ebx");
}

void exit() {
    asm("movl $42, %ebx \n\t"
        "movl $1, %eax \n\t"
        "int $0x80 \n\t");
}

void nomain() {
    print();
    exit();
}

// char* str = "Hello world!\n";

// void print() {
//     asm(
//         "movl $13, %%edx \n\t"  // 字符串长度
//         "movl %0, %%ecx \n\t"   // 缓冲区地址
//         "movl $0, %%ebx \n\t"   // 文件描述符（标准输出）
//         "movl $4, %%eax \n\t"   // 系统调用号（sys_write）
//         "int $0x80 \n\t"        // 触发中断0x80
//         :: "r"(str) : "edx", "ecx", "ebx"
//     );
// }

// void exit() {
//     asm(
//         "movl $42, %%ebx \n\t"  // 退出码
//         "movl $1, %%eax \n\t"   // 系统调用号（sys_exit）
//         "int $0x80 \n\t"        // 触发中断0x80
//     );
// }

// void nomain() {
//     print();
//     exit();
// }

