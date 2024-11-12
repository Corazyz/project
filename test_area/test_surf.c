#include "stdio.h"
#include "math.h"
#include "string.h"
#include "stdlib.h"

typedef struct {
	int **response_filter;
} rl_Kernel;

typedef struct {
	rl_Kernel kernels[3];
} rl_Kernel_Layer;

typedef struct {
	rl_Kernel_Layer layers[12];
} rl_response_kernel;

int main() {
    int filter = 21;
    // rl_Kernel_Layer myBoxFilter;
    rl_response_kernel myBoxFilter;
    // float a = 1.0/3;
    // printf("%f\n", a);

    for (int i = 0; i < 12; i++) {
        for (int D = 0; D < 3; D++) {
            myBoxFilter.layers[i].kernels[D].response_filter = (int**)malloc(filter * sizeof(int *));
        }
        for (int j = 0; j < filter; j++) {
            for (int D = 0; D < 3; D++) {
                    myBoxFilter.layers[i].kernels[D].response_filter[j] = (int*)malloc(filter * sizeof(int *));
            }
            for (int k = 0; k < filter; k++) {
                if (j >= (1.0/3*filter + 1)/2 && j < (5.0/3*filter- 1)/2 && k >= 1.0/3*filter && k < 2.0/3*filter) {
                // if (j >= (filter/3 + 1)/2 && j < (filter*5.0/3 - 1)/2 && k >= filter/3 && k < filter*2/3) {
                    // printf("test for j&k value: j = %d, k = %d\n", j, k);
                    myBoxFilter.layers[i].kernels[0].response_filter[j][k] = -2;
                } else if (j >= (1.0/3*filter + 1)/2 && j < (5.0/3*filter - 1)/2) {
                    // printf("test for j&k value: j = %d, k = %d\n", j, k);
                    myBoxFilter.layers[i].kernels[0].response_filter[j][k] = 1;
                } else {
                    myBoxFilter.layers[i].kernels[0].response_filter[j][k] = 0;
                }
                if (j >= filter / 3 && j < filter * 2/3 && k >= (filter/3 + 1)/2 && k < (filter*5/3 - 1)/2) {
                    myBoxFilter.layers[i].kernels[1].response_filter[j][k] = -2;
                } else if (k >= (filter/3 + 1)/2 && k < (filter * 5/3 - 1)/2) {
                    myBoxFilter.layers[i].kernels[1].response_filter[j][k] = 1;
                } else {
                    myBoxFilter.layers[i].kernels[1].response_filter[j][k] = 0;
                }
                if ((j >= (filter/3 - 1)/2 && j < (filter - 1)/2 && k >= (filter/3 - 1)/2 && k < (filter - 1)/2) || (j >=  (filter + 1)/2 && j < (filter*5/3 + 1)/2 && k >= (filter + 1)/2 && k < (filter*5/3 + 1)/2)) {
                    myBoxFilter.layers[i].kernels[2].response_filter[j][k] = 1;
                } else if ((j >= (filter/3 - 1)/2 && j < (filter - 1)/2 && k >= (filter + 1)/2 && k < (filter *5/3 + 1)/2) || (j >=  (filter + 1)/2 && j < (filter*5/3 + 1)/2 && k >= (filter/3 - 1)/2 && k < (filter - 1)/2)) {
                    myBoxFilter.layers[i].kernels[2].response_filter[j][k] = -1;
                } else {
                    myBoxFilter.layers[i].kernels[2].response_filter[j][k] = 0;
                }
            }
        }
    }



    // for (int D = 0; D < 3; D++) {
	// 	myBoxFilter.kernels[D].response_filter = (int**)malloc(filter * sizeof(int *));
	// }
	// for (int j = 0; j < filter; j++) {
	// 	for (int D = 0; D < 3; D++) {
	// 			myBoxFilter.kernels[D].response_filter[j] = (int*)malloc(filter * sizeof(int *));
	// 	}
	// 	for (int k = 0; k < filter; k++) {
	// 		if (j >= (filter / 3 + 1)/2 && j < (filter * 5/3 - 1)/2 && k >= filter / 3 && k < filter * 2/3) {
    //             printf("test for j&k value: j = %d, k = %d\n", j, k);
	// 			myBoxFilter.kernels[0].response_filter[j][k] = -2;
	// 		} else if (j >= (filter/3 + 1)/2 && j < (filter *5/3 - 1)/2) {
	// 			myBoxFilter.kernels[0].response_filter[j][k] = 1;
	// 		} else {
	// 			myBoxFilter.kernels[0].response_filter[j][k] = 0;
	// 		}
	// 		if (j >= filter / 3 && j < filter * 2/3 && k >= (filter/3 + 1)/2 && k < (filter*5/3 - 1)/2) {
	// 			myBoxFilter.kernels[1].response_filter[j][k] = -2;
	// 		} else if (k >= (filter/3 + 1)/2 && k < (filter * 5/3 - 1)/2) {
	// 			myBoxFilter.kernels[1].response_filter[j][k] = 1;
	// 		} else {
	// 			myBoxFilter.kernels[1].response_filter[j][k] = 0;
	// 		}
	// 		if ((j >= (filter/3 - 1)/2 && j < (filter - 1)/2 && k >= (filter/3 - 1)/2 && k < (filter - 1)/2) || (j >=  (filter + 1)/2 && j < (filter*5/3 + 1)/2 && k >= (filter + 1)/2 && k < (filter*5/3 + 1)/2)) {
	// 			myBoxFilter.kernels[2].response_filter[j][k] = 1;
	// 		} else if ((j >= (filter/3 - 1)/2 && j < (filter - 1)/2 && k >= (filter + 1)/2 && k < (filter *5/3 + 1)/2) || (j >=  (filter + 1)/2 && j < (filter*5/3 + 1)/2 && k >= (filter/3 - 1)/2 && k < (filter - 1)/2)) {
	// 			myBoxFilter.kernels[2].response_filter[j][k] = -1;
	// 		} else {
	// 			myBoxFilter.kernels[2].response_filter[j][k] = 0;
	// 		}
	// 	}
	// }



    for (int i = 0; i < filter; i++) {
        for (int j = 0; j < filter; j++) {
            if (myBoxFilter.layers[1].kernels[0].response_filter[i][j] != -2) {
                printf("%d  ", myBoxFilter.layers[1].kernels[0].response_filter[i][j]);
            } else {printf("%d ", myBoxFilter.layers[1].kernels[0].response_filter[i][j]);}
        }
        printf("\n");
    }

    printf("\n");
    printf("\n");
    for (int i = 0; i < filter; i++) {
        for (int j = 0; j < filter; j++) {
            if (myBoxFilter.layers[1].kernels[1].response_filter[i][j] != -2) {
                printf("%d  ", myBoxFilter.layers[1].kernels[1].response_filter[i][j]);
            } else {printf("%d ", myBoxFilter.layers[1].kernels[1].response_filter[i][j]);}
        }
        printf("\n");
    }
    printf("\n");
    printf("\n");
    for (int i = 0; i < filter; i++) {
        for (int j = 0; j < filter; j++) {
            if (myBoxFilter.layers[1].kernels[2].response_filter[i][j] != -1) {
                printf("%d  ", myBoxFilter.layers[1].kernels[2].response_filter[i][j]);
            } else {printf("%d ", myBoxFilter.layers[1].kernels[2].response_filter[i][j]);}
        }
        printf("\n");
    }



    // for (int i = 0; i < filter; i++) {
    //     for (int j = 0; j < filter; j++) {
    //         if (myBoxFilter.layers[0].kernels[0].response_filter[i][j] != -2) {
    //             printf("%d  ", myBoxFilter.layers[0].kernels[0].response_filter[i][j]);
    //         } else {printf("%d ", myBoxFilter.layers[0].kernels[0].response_filter[i][j]);}
    //     }
    //     printf("\n");
    // }

    // printf("\n");
    // printf("\n");
    // for (int i = 0; i < filter; i++) {
    //     for (int j = 0; j < filter; j++) {
    //         if (myBoxFilter.layers[0].kernels[1].response_filter[i][j] != -2) {
    //             printf("%d  ", myBoxFilter.layers[0].kernels[1].response_filter[i][j]);
    //         } else {printf("%d ", myBoxFilter.layers[0].kernels[1].response_filter[i][j]);}
    //     }
    //     printf("\n");
    // }
    // printf("\n");
    // printf("\n");
    // for (int i = 0; i < filter; i++) {
    //     for (int j = 0; j < filter; j++) {
    //         if (myBoxFilter.layers[0].kernels[2].response_filter[i][j] != -1) {
    //             printf("%d  ", myBoxFilter.layers[0].kernels[2].response_filter[i][j]);
    //         } else {printf("%d ", myBoxFilter.layers[0].kernels[2].response_filter[i][j]);}
    //     }
    //     printf("\n");
    // }

    for (int i = 0; i < 12; i++) {
        for (int D = 0; D < 3; D++) {
            for (int j = 0; j < filter; j++) {
                // for (int k = 0; k < filter; k++) {
                //     free(myBoxFilter.layers[0].kernels[D].response_filter[j][k]);  // 释放最内层的内存
                // }
                free(myBoxFilter.layers[i].kernels[D].response_filter[j]);  // 释放中间层的内存
            }
            free(myBoxFilter.layers[i].kernels[D].response_filter);  // 释放外层的内存
        }
    }
}
