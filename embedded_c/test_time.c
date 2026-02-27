
#include <stdio.h>
//# 字符化
//## 连接符
//__FUNCTION__
//__LINE__
//__LINE__
//printf("%d %d \n", a, b);

// #define LOG_INFO(x) printf("x is %d \n", (x));
// #define FUNC(a, b) {((a) = (a)*(b)); (a++);} 
#define FUNC(a, b) do {((a) = (a)*(b)); (a++);} while(0)

#define DAY_DECLARE(x) int day##x
#define DAY(x) day##x
#define ABC 10+2

//#define LOG_INFO(x, ...) printf("%s: %s: %d: "x"",\
//		__FILE__, \
//		__FUNCTION__, \
//		__LINE__, \
//		##__VA_ARGS__)
#define LOG_INFO(x) do {printf("%s is %d \n", #x, (x));} while(0)
// int a = 10;

int main() {
//	if (0)
//		FUNC(a, 10);
//	else 
//		FUNC(a, a);
	DAY_DECLARE(1) = 100;
	DAY_DECLARE(21) = 200;
//	LOG_INFO("DAY1 = %d\n", DAY(1));
//	LOG_INFO("DAY21 = %d\n", DAY(21));
//	LOG_INFO("a = %d\n", a);
	LOG_INFO(DAY(1));
	printf("sizeof(10) = %ld\n", sizeof 100000000000);
//	a = ABC * 10;
//	printf("a is %d \n", a);
//	char a = 'a';
//	printf("a = %c ASCII = %d \n", a, a);
//	char b, c;
//	printf("input a char:");
//	scanf("%c%c", &b, &c);
//	char d = getchar();
//	char d;
//	scanf("%c", &d);
//	printf("input char are: %c, %c\n", b, c);
//	printf("d = %c\n", d);
//	printf("float: %ld, double: %ld \n", sizeof(float), sizeof(double));	
	char a = 115;
	if (++a > 127) {
		printf("char is unsigned, a = %c\n", a);
	} else {
		printf("char is signed, a = %c\n", a);
	}
	return 0;
}
