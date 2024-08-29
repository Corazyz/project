#include <stdio.h>
#include <string>
#include <iostream>
// typedef union djalhujiergh{
//     int i;
//     float f;
//     char c;
// } my_union;

// union djalhujiergh{
//     int i;
//     float f;
//     char c;
// };
// typedef union djalhujiergh my_union;

typedef struct abcd
{
    char name;
    int age;
    std::string addr;
} info;

// struct info
// {
//     char name;
//     int age;
//     char* addr;
// };


int main() {
    // my_union new_var;
    // new_var.i = 1;
    // printf("%d\n", new_var);
    // return 0;
    abcd my_info;
    my_info.addr = "xxx";
    // printf("%s", my_info.addr);
    std::cout<<my_info.addr<<std::endl;
    return 0;
}