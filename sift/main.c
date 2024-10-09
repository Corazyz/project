#include "header.h"
#include <stdio.h>
#include<stdlib.h>
void sift_match();

int main(int argc, char** argv)
{
	sift_match();
	system("pause");
	return 0;
}



void sift_match()
{
	IplImage* img1, *img2, *stacked;
	Feature* feat1, *feat2, *feat;
	Feature** nbrs;
	KdNode* kd_root;
	Point pt1, pt2;
	double d0, d1;
	char* bmp1, *bmp2;
	int n1, n2, k, i, m = 0;
	FILE * fp1, *fp2;

	printf("load image\n");
	img1 = loadImage("./1.bmp");
	img2 = loadImage("./2.bmp");
	fp1 = fopen("./sift_point_img1.txt", "wt");
	fp2 = fopen("./sift_point_img2.txt", "wt");

	printf("start get sift\n");
	n1 = sift_features(img1, &feat1);
	n2 = sift_features(img2, &feat2);
	printf("numbers of sift in image 1: %d\n", n1);
	printf("numbers of sift in image 2: %d\n", n2);
	printf("create kd tree\n");
	kd_root = kdtree_build(feat2, n2);
	printf("start feature match\n");
	for (i = 0; i < n1; i++)
	{
		feat = feat1 + i;
		k = kdtree_bbf_knn(kd_root, feat, 2, &nbrs, KDTREE_BBF_MAX_NN_CHKS);
		if (k == 2)
		{
			d0 = descr_dist_sq(feat, nbrs[0]);
			d1 = descr_dist_sq(feat, nbrs[1]);
			if (d0 < d1 * NN_SQ_DIST_RATIO_THR)
			{
				pt1 = ipoint(iround(feat->x), iround(feat->y));
				fprintf(fp1, "%d : %d %d\n",(i+1), pt1.x, pt1.y);
				pt2 = ipoint(iround(nbrs[0]->x), iround(nbrs[0]->y));
				fprintf(fp2, "%d : %d %d\n", (i+1), pt2.x, pt2.y);
				pt2.y += img1->height;
				m++;
				feat1[i].fwd_match = nbrs[0];
			}
		}
		free(nbrs);
	}
	printf("feature match complete\n\n");
	printf("numbers of matched sift : %d\n\n",m);
	printf("please open sift_point.txt to get the matched coordinates!");
	fclose(fp1);
	fclose(fp2);
	releaseImage(&img1);
	releaseImage(&img2);
	kdtree_release(kd_root);
	free(feat1);
	free(feat2);
}
