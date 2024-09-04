#include <iostream>
#include <stdio.h>



static int get_image_size(int format, int width, int height){
    int size = 0;
    switch (format){
        case 0:
            size = width * height + (height * width / 4) * 2;
            break;
        case 1:
            size = width * height + (height * width / 2) * 2;
            break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            size = width * height * 3;
            break;
        case 9:
        case 10:
            size = width * height + width * height / 2;
            break;
        default:
            printf("image format error \n");
            break;
    }
    return size;
}

int main() {
    int format = 4;
    int res = get_image_size(format, 10, 20);
    printf("size = %d\n", res);
    return 0;
}