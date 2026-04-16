#include "GMM.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef __unix
#include <unistd.h>
#else
#include "sample_getopt.h"
#endif

int main( int argc, char* const* argv )
{
	CVI_S32 s32FinalResult = CVI_SUCCESS;
	CVI_S32 s32Result;

	int show = 1, compare = 1;
	int c;
	opterr = 0;
	while ((c = getopt (argc, argv, "s:c:")) != -1)
	{
    	switch (c)
		{
		case 's':
			show = atoi(optarg);
			break;
		case 'c':
    	    compare = atoi(optarg);
    	    break;
		default:
			abort ();
		}
	}

	printf("%s show gmm sample\n",argv[0]);

    printf("RGB gmm sample start...\n");
    s32Result = GMM_Sample_U8C3_PACKAGE(show, compare);
    printf("-->end!\n");
	if (s32Result != CVI_SUCCESS) {
		printf("GMM_Sample_U8C3_PACKAGE failed, ret = %d\n", s32Result);
		s32FinalResult = s32Result;
	}

    printf("Gray gmm sample start...\n");
    s32Result = GMM_Sample_U8C1(show, compare);
	printf("-->end!\n");
	if (s32Result != CVI_SUCCESS) {
		printf("GMM_Sample_U8C1 failed, ret = %d\n", s32Result);
		s32FinalResult = s32Result;
	}

	printf("gmm sample done\n");

	//printf("\npress any key to exit...\n");
	//getchar();
	return s32FinalResult;
}