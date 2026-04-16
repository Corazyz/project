#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    int fd = open("/dev/chrdevbase1", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    char buf[100];
    write(fd, "Hello from user", 15);
    read(fd, buf, 100);
    printf("Read from kernel: %s\n", buf);

    close(fd);
    return 0;
}