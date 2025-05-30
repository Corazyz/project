#include "header.h"
#include<math.h>

int sift_features(IplImage* img, Feature** feat)
{
	return _sift_features(img, feat, SIFT_INTVLS, SIFT_SIGMA, SIFT_CONTR_THR, SIFT_CURV_THR, SIFT_IMG_DBL, SIFT_DESCR_WIDTH, SIFT_DESCR_HIST_BINS);
}


int _sift_features(IplImage* img, Feature** feat, int intvls, double sigma, double contr_thr, int curv_thr, int img_dbl, int descr_width, int descr_hist_bins)
{
	IplImage* init_img;
	IplImage*** gauss_pyr, *** dog_pyr;
	MemStorage* storage;
	Seq* features;
	int octvs, i, n;

	init_img = create_init_img(img, img_dbl, sigma);
	octvs = log(MIN(init_img->width, init_img->height)) / log(2) - 2;

	gauss_pyr = build_gauss_pyr(init_img, octvs, intvls, sigma);
	dog_pyr = build_dog_pyr(gauss_pyr, octvs, intvls);

	storage = createMemStorage(0);
	features = scale_space_extrema(dog_pyr, octvs, intvls, contr_thr, curv_thr, storage);
	calc_feature_scales(features, sigma, intvls);
	if (img_dbl)
		adjust_for_img_dbl(features);
	calc_feature_oris(features, gauss_pyr);
	compute_descriptors(features, gauss_pyr, descr_width, descr_hist_bins);

	/* sort features by decreasing scale and move from Seq to array */
	seqSort(features, (CmpFunc)feature_cmp, NULL);
	n = features->total;

	*feat = calloc(n, sizeof(Feature));

	*feat = tSeqToArray(features, *feat, CV_WHOLE_SEQ);
	for (i = 0; i < n; i++)
	{
		free((*feat)[i].feature_data);
		(*feat)[i].feature_data = NULL;
	}

	releaseMemStorage(&storage);
	releaseImage(&init_img);
	release_pyr(&gauss_pyr, octvs, intvls + 3);
	release_pyr(&dog_pyr, octvs, intvls + 2);
	return n;
}


/************************ Functions prototyped here **************************/

/*
  Converts an image to 8-bit grayscale and Gaussian-smooths it.  The image is
  optionally doubled in size prior to smoothing.

  @param img input image
  @param img_dbl if true, image is doubled in size prior to smoothing
  @param sigma total std of Gaussian smoothing
*/
//tristan
static IplImage* create_init_img(IplImage* img, int img_dbl, double sigma)
{
	IplImage* gray, *dbl;
	double sig_diff;

	gray = convert_to_gray32(img);
	sig_diff = sqrt(sigma * sigma - SIFT_INIT_SIGMA * SIFT_INIT_SIGMA * 4);
	dbl = createImage(isize(img->width * 2, img->height * 2),IPL_DEPTH_32F, 1);
	resizeImg(gray, dbl);

	GaussianSmooth(dbl,dbl,sig_diff);
	releaseImage(&gray);
	return dbl;
}

float pixval32f(IplImage* img, int r, int c)
{
	return ((float*)(img->imageData + img->widthStep*r))[c];
}

/*
  Converts an image to 32-bit grayscale

  @param img a 3-channel 8-bit color (BGR) or 8-bit gray image

  @return Returns a 32-bit grayscale image
*/
//tristan
Size getSize(IplImage* img)
{
	Size s;
	s.height = img->height;
	s.width = img->width;
	return s;
}
//tristan
static IplImage* convert_to_gray32(IplImage* img)
{
	IplImage* gray8, *gray32;

	Size size = getSize(img);
	gray32 = createImage(size, IPL_DEPTH_32F, 1);
	gray8 = rgb2gray(img);
	convertScale(gray8, gray32, 1.0 / 255.0);

	releaseImage(&gray8);
	return gray32;
}



/*
  Builds Gaussian scale space pyramid from an image

  @param base base image of the pyramid
  @param octvs number of octaves of scale space
  @param intvls number of intervals per octave
  @param sigma amount of Gaussian smoothing per octave

  @return Returns a Gaussian scale space pyramid as an octvs x (intvls + 3)
    array
*/
//#define _intvls 10
//tristan
static IplImage*** build_gauss_pyr(IplImage* base, int octvs,
	int intvls, double sigma)
{
	IplImage*** gauss_pyr;
	const int _intvls = intvls;

	double* sig = (double*)malloc(sizeof(double)*(_intvls + 3));

	double k;
	int i, o;

	gauss_pyr = calloc(octvs, sizeof(IplImage**));

	for (i = 0; i < octvs; i++)
	{
		gauss_pyr[i] = calloc(intvls + 3, sizeof(IplImage *));
	}

	/*
	  precompute Gaussian sigmas using the following formula:

	  \sigma_{total}^2 = \sigma_{i}^2 + \sigma_{i-1}^2

	  sig[i] is the incremental sigma value needed to compute
	  the actual sigma of level i. Keeping track of incremental
	  sigmas vs. total sigmas keeps the gaussian kernel small.
	  */
	k = pow(2.0, 1.0 / intvls);
	sig[0] = sigma;
	sig[1] = sigma * sqrt(k*k - 1);
	for (i = 2; i < intvls + 3; i++)
		sig[i] = sig[i - 1] * k;

	for (o = 0; o < octvs; o++)
		for (i = 0; i < intvls + 3; i++)
		{
			if (o == 0 && i == 0)
				gauss_pyr[o][i] = cloneImage(base);

			/* base of new octvave is halved image from end of previous octave */
			else if (i == 0)
				gauss_pyr[o][i] = downsample(gauss_pyr[o - 1][intvls]);

			/* blur the current octave's last image to create the next one */
			else
			{
				gauss_pyr[o][i] = createImage(getSize(gauss_pyr[o][i - 1]),
					IPL_DEPTH_32F, 1);
				//cvSmooth(gauss_pyr[o][i - 1], gauss_pyr[o][i],
				//	CV_GAUSSIAN, 0, 0, sig[i], sig[i]);
				GaussianSmooth(gauss_pyr[o][i - 1], gauss_pyr[o][i],sig[i]);
			}
		}

	return gauss_pyr;
}


/*
  Downsamples an image to a quarter of its size (half in each dimension)
  using nearest-neighbor interpolation

  @param img an image

  @return Returns an image whose dimensions are half those of img
*/
//tristan
static IplImage* downsample(IplImage* img)
{
	IplImage* smaller = createImage(isize(img->width / 2, img->height / 2),
		img->depth, img->nChannels);
	resizeImg(img, smaller);

	return smaller;
}



/*
  Builds a difference of Gaussians scale space pyramid by subtracting adjacent
  intervals of a Gaussian pyramid

  @param gauss_pyr Gaussian scale-space pyramid
  @param octvs number of octaves of scale space
  @param intvls number of intervals per octave

  @return Returns a difference of Gaussians scale space pyramid as an
    octvs x (intvls + 2) array
*/
//tristan
static IplImage*** build_dog_pyr(IplImage*** gauss_pyr, int octvs, int intvls)
{
	IplImage*** dog_pyr;
	int i, o;

	dog_pyr = calloc(octvs, sizeof(IplImage**));

	for (i = 0; i < octvs; i++)
	{
		dog_pyr[i] = calloc(intvls + 2, sizeof(IplImage*));
	}

	for (o = 0; o < octvs; o++)
		for (i = 0; i < intvls + 2; i++)
		{
			dog_pyr[o][i] = createImage(getSize(gauss_pyr[o][i]), IPL_DEPTH_32F, 1);
			imgSub(gauss_pyr[o][i + 1], gauss_pyr[o][i], dog_pyr[o][i]);
		}

	return dog_pyr;
}



/*
  Detects features at extrema in DoG scale space.  Bad features are discarded
  based on contrast and ratio of principal curvatures.

  @param dog_pyr DoG scale space pyramid
  @param octvs octaves of scale space represented by dog_pyr
  @param intvls intervals per octave
  @param contr_thr low threshold on feature contrast
  @param curv_thr high threshold on feature ratio of principal curvatures
  @param storage memory storage in which to store detected features

  @return Returns an array of detected features whose scales, orientations,
    and descriptors are yet to be determined.
*/
//tristan
static Seq* scale_space_extrema(IplImage*** dog_pyr, int octvs, int intvls,
	double contr_thr, int curv_thr,
	MemStorage* storage)
{
	Seq* features;
	double prelim_contr_thr = 0.5 * contr_thr / intvls;
	Feature* feat;
	DetectionData* ddata;
	int o, i, r, c;

	features = createSeq(0, sizeof(Seq), sizeof(Feature), storage);
	for (o = 0; o < octvs; o++)
		for (i = 1; i <= intvls; i++)
			for (r = SIFT_IMG_BORDER; r < dog_pyr[o][0]->height - SIFT_IMG_BORDER; r++)
				for (c = SIFT_IMG_BORDER; c < dog_pyr[o][0]->width - SIFT_IMG_BORDER; c++)
					/* perform preliminary check on contrast */
					if (ABS(pixval32f(dog_pyr[o][i], r, c)) > prelim_contr_thr)
						if (is_extremum(dog_pyr, o, i, r, c))
						{
							feat = interp_extremum(dog_pyr, o, i, r, c, intvls, contr_thr);
							if (feat)
							{
								ddata = feat_detection_data(feat);
								if (!is_too_edge_like(dog_pyr[ddata->octv][ddata->intvl],
									ddata->r, ddata->c, curv_thr))
								{
									seqPush(features, feat);
								}
								else
									free(ddata);
								free(feat);
							}
						}

	return features;
}



/*
  Determines whether a pixel is a scale-space extremum by comparing it to it's
  3x3x3 pixel neighborhood.

  @param dog_pyr DoG scale space pyramid
  @param octv pixel's scale space octave
  @param intvl pixel's within-octave interval
  @param r pixel's image row
  @param c pixel's image col

  @return Returns 1 if the specified pixel is an extremum (max or min) among
    it's 3x3x3 pixel neighborhood.
*/
//tristan
static int is_extremum(IplImage*** dog_pyr, int octv, int intvl, int r, int c)
{
	double val = pixval32f(dog_pyr[octv][intvl], r, c);
	int i, j, k;

	/* check for maximum */
	if (val > 0)
	{
		for (i = -1; i <= 1; i++)
			for (j = -1; j <= 1; j++)
				for (k = -1; k <= 1; k++)
					if (val < pixval32f(dog_pyr[octv][intvl + i], r + j, c + k))
						return 0;
	}

	/* check for minimum */
	else
	{
		for (i = -1; i <= 1; i++)
			for (j = -1; j <= 1; j++)
				for (k = -1; k <= 1; k++)
					if (val > pixval32f(dog_pyr[octv][intvl + i], r + j, c + k))
						return 0;
	}

	return 1;
}



/*
  Interpolates a scale-space extremum's location and scale to subpixel
  accuracy to form an image feature.  Rejects features with low contrast.
  Based on Section 4 of Lowe's paper.

  @param dog_pyr DoG scale space pyramid
  @param octv feature's octave of scale space
  @param intvl feature's within-octave interval
  @param r feature's image row
  @param c feature's image column
  @param intvls total intervals per octave
  @param contr_thr threshold on feature contrast

  @return Returns the feature resulting from interpolation of the given
    parameters or NULL if the given location could not be interpolated or
    if contrast at the interpolated loation was too low.  If a feature is
    returned, its scale, orientation, and descriptor are yet to be determined.
*/
//tristan
static Feature* interp_extremum(IplImage*** dog_pyr, int octv,
	int intvl, int r, int c, int intvls,
	double contr_thr)
{
	Feature* feat;
	DetectionData* ddata;
	double xi, xr, xc, contr;
	int i = 0;

	while (i < SIFT_MAX_INTERP_STEPS)
	{
		interp_step(dog_pyr, octv, intvl, r, c, &xi, &xr, &xc);
		if (ABS(xi) < 0.5  &&  ABS(xr) < 0.5  &&  ABS(xc) < 0.5)
			break;

		c += iround(xc);
		r += iround(xr);
		intvl += iround(xi);

		if (intvl < 1 ||
			intvl > intvls ||
			c < SIFT_IMG_BORDER ||
			r < SIFT_IMG_BORDER ||
			c >= dog_pyr[octv][0]->width - SIFT_IMG_BORDER ||
			r >= dog_pyr[octv][0]->height - SIFT_IMG_BORDER)
		{
			return NULL;
		}

		i++;
	}

	/* ensure convergence of interpolation */
	if (i >= SIFT_MAX_INTERP_STEPS)
		return NULL;

	contr = interp_contr(dog_pyr, octv, intvl, r, c, xi, xr, xc);
	if (ABS(contr) < contr_thr / intvls)
		return NULL;

	feat = new_feature();
	ddata = feat_detection_data(feat);
	feat->img_pt.x = feat->x = (c + xc) * pow(2.0, octv);
	feat->img_pt.y = feat->y = (r + xr) * pow(2.0, octv);
	ddata->r = r;
	ddata->c = c;
	ddata->octv = octv;
	ddata->intvl = intvl;
	ddata->subintvl = xi;

	return feat;
}



/*
  Performs one step of extremum interpolation.  Based on Eqn. (3) in Lowe's
  paper.

  @param dog_pyr difference of Gaussians scale space pyramid
  @param octv octave of scale space
  @param intvl interval being interpolated
  @param r row being interpolated
  @param c column being interpolated
  @param xi output as interpolated subpixel increment to interval
  @param xr output as interpolated subpixel increment to row
  @param xc output as interpolated subpixel increment to col
*/
//tristan
static void interp_step(IplImage*** dog_pyr, int octv, int intvl, int r, int c,
	double* xi, double* xr, double* xc)
{
	Mat* dD, *H, *H_inv, X;
	double x[3] = { 0 };

	dD = deriv_3D(dog_pyr, octv, intvl, r, c);
	H = hessian_3D(dog_pyr, octv, intvl, r, c);
	H_inv = createMat(3, 3, CV_64FC1);
	invert(H, H_inv, 3);
	initMatHeader(&X, 3, 1, CV_64FC1, x, CV_AUTOSTEP);
	GEMM(H_inv, dD, -1, &X, 0);
	releaseMat(&dD);
	releaseMat(&H);
	releaseMat(&H_inv);

	*xi = x[2];
	*xr = x[1];
	*xc = x[0];
}



/*
  Computes the partial derivatives in x, y, and scale of a pixel in the DoG
  scale space pyramid.

  @param dog_pyr DoG scale space pyramid
  @param octv pixel's octave in dog_pyr
  @param intvl pixel's interval in octv
  @param r pixel's image row
  @param c pixel's image col

  @return Returns the vector of partial derivatives for pixel I
    { dI/dx, dI/dy, dI/ds }^T as a Mat*
*/
//tristan
static Mat* deriv_3D(IplImage*** dog_pyr, int octv, int intvl, int r, int c)
{
	Mat* dI;
	double dx, dy, ds;

	dx = (pixval32f(dog_pyr[octv][intvl], r, c + 1) -
		pixval32f(dog_pyr[octv][intvl], r, c - 1)) / 2.0;
	dy = (pixval32f(dog_pyr[octv][intvl], r + 1, c) -
		pixval32f(dog_pyr[octv][intvl], r - 1, c)) / 2.0;
	ds = (pixval32f(dog_pyr[octv][intvl + 1], r, c) -
		pixval32f(dog_pyr[octv][intvl - 1], r, c)) / 2.0;

	dI = createMat(3, 1, CV_64FC1);
	mSet(dI, 0, 0, dx);
	mSet(dI, 1, 0, dy);
	mSet(dI, 2, 0, ds);

	return dI;
}



/*
  Computes the 3D Hessian matrix for a pixel in the DoG scale space pyramid.

  @param dog_pyr DoG scale space pyramid
  @param octv pixel's octave in dog_pyr
  @param intvl pixel's interval in octv
  @param r pixel's image row
  @param c pixel's image col

  @return Returns the Hessian matrix (below) for pixel I as a Mat*

  / Ixx  Ixy  Ixs \ <BR>
  | Ixy  Iyy  Iys | <BR>
  \ Ixs  Iys  Iss /
*/
//tristan
static Mat* hessian_3D(IplImage*** dog_pyr, int octv, int intvl, int r,
	int c)
{
	Mat* H;
	double v, dxx, dyy, dss, dxy, dxs, dys;

	v = pixval32f(dog_pyr[octv][intvl], r, c);
	dxx = (pixval32f(dog_pyr[octv][intvl], r, c + 1) +
		pixval32f(dog_pyr[octv][intvl], r, c - 1) - 2 * v);
	dyy = (pixval32f(dog_pyr[octv][intvl], r + 1, c) +
		pixval32f(dog_pyr[octv][intvl], r - 1, c) - 2 * v);
	dss = (pixval32f(dog_pyr[octv][intvl + 1], r, c) +
		pixval32f(dog_pyr[octv][intvl - 1], r, c) - 2 * v);
	dxy = (pixval32f(dog_pyr[octv][intvl], r + 1, c + 1) -
		pixval32f(dog_pyr[octv][intvl], r + 1, c - 1) -
		pixval32f(dog_pyr[octv][intvl], r - 1, c + 1) +
		pixval32f(dog_pyr[octv][intvl], r - 1, c - 1)) / 4.0;
	dxs = (pixval32f(dog_pyr[octv][intvl + 1], r, c + 1) -
		pixval32f(dog_pyr[octv][intvl + 1], r, c - 1) -
		pixval32f(dog_pyr[octv][intvl - 1], r, c + 1) +
		pixval32f(dog_pyr[octv][intvl - 1], r, c - 1)) / 4.0;
	dys = (pixval32f(dog_pyr[octv][intvl + 1], r + 1, c) -
		pixval32f(dog_pyr[octv][intvl + 1], r - 1, c) -
		pixval32f(dog_pyr[octv][intvl - 1], r + 1, c) +
		pixval32f(dog_pyr[octv][intvl - 1], r - 1, c)) / 4.0;

	H = createMat(3, 3, CV_64FC1);
	mSet(H, 0, 0, dxx);
	mSet(H, 0, 1, dxy);
	mSet(H, 0, 2, dxs);
	mSet(H, 1, 0, dxy);
	mSet(H, 1, 1, dyy);
	mSet(H, 1, 2, dys);
	mSet(H, 2, 0, dxs);
	mSet(H, 2, 1, dys);
	mSet(H, 2, 2, dss);

	return H;
}



/*
  Calculates interpolated pixel contrast.  Based on Eqn. (3) in Lowe's
  paper.

  @param dog_pyr difference of Gaussians scale space pyramid
  @param octv octave of scale space
  @param intvl within-octave interval
  @param r pixel row
  @param c pixel column
  @param xi interpolated subpixel increment to interval
  @param xr interpolated subpixel increment to row
  @param xc interpolated subpixel increment to col

  @param Returns interpolated contrast.
*/
//tristan
static double interp_contr(IplImage*** dog_pyr, int octv, int intvl, int r,
	int c, double xi, double xr, double xc)
{
	Mat* dD, X, T;
	double t[1], x[3] = { xc, xr, xi };

	initMatHeader(&X, 3, 1, CV_64FC1, x, CV_AUTOSTEP);
	initMatHeader(&T, 1, 1, CV_64FC1, t, CV_AUTOSTEP);
	dD = deriv_3D(dog_pyr, octv, intvl, r, c);
	GEMM(dD, &X, 1, &T, 1);
	double tst = T.data.db[0];
	releaseMat(&dD);

	return pixval32f(dog_pyr[octv][intvl], r, c) + t[0] * 0.5;
}


/*
  Allocates and initializes a new feature

  @return Returns a pointer to the new feature
*/
//tristan
static Feature* new_feature(void)
{
	Feature* feat;
	DetectionData* ddata;


	feat = malloc(sizeof(Feature));
	memset(feat, 0, sizeof(Feature));
	ddata = malloc(sizeof(DetectionData));
	memset(ddata, 0, sizeof(DetectionData));
	feat->feature_data = ddata;
	feat->type = FEATURE_LOWE;

	return feat;
}



/*
  Determines whether a feature is too edge like to be stable by computing the
  ratio of principal curvatures at that feature.  Based on Section 4.1 of
  Lowe's paper.

  @param dog_img image from the DoG pyramid in which feature was detected
  @param r feature row
  @param c feature col
  @param curv_thr high threshold on ratio of principal curvatures

  @return Returns 0 if the feature at (r,c) in dog_img is sufficiently
    corner-like or 1 otherwise.
*/
//tristan
static int is_too_edge_like(IplImage* dog_img, int r, int c, int curv_thr)
{
	double d, dxx, dyy, dxy, tr, det;

	/* principal curvatures are computed using the trace and det of Hessian */
	d = pixval32f(dog_img, r, c);
	dxx = pixval32f(dog_img, r, c + 1) + pixval32f(dog_img, r, c - 1) - 2 * d;
	dyy = pixval32f(dog_img, r + 1, c) + pixval32f(dog_img, r - 1, c) - 2 * d;
	dxy = (pixval32f(dog_img, r + 1, c + 1) - pixval32f(dog_img, r + 1, c - 1) -
		pixval32f(dog_img, r - 1, c + 1) + pixval32f(dog_img, r - 1, c - 1)) / 4.0;
	tr = dxx + dyy;
	det = dxx * dyy - dxy * dxy;

	/* negative determinant -> curvatures have different signs; reject feature */
	if (det <= 0)
		return 1;

	if (tr * tr / det < (curv_thr + 1.0)*(curv_thr + 1.0) / curv_thr)
		return 0;
	return 1;
}



/*
  Calculates characteristic scale for each feature in an array.

  @param features array of features
  @param sigma amount of Gaussian smoothing per octave of scale space
  @param intvls intervals per octave of scale space
*/
//tristan
static void calc_feature_scales(Seq* features, double sigma, int intvls)
{
	Feature* feat;
	DetectionData* ddata;
	double intvl;
	int i, n;

	n = features->total;
	for (i = 0; i < n; i++)
	{
		feat = getSeqElem((Seq*)(features), i);
		ddata = feat_detection_data(feat);
		intvl = ddata->intvl + ddata->subintvl;
		feat->scl = sigma * pow(2.0, ddata->octv + intvl / intvls);
		ddata->scl_octv = sigma * pow(2.0, intvl / intvls);
	}
}



/*
  Halves feature coordinates and scale in case the input image was doubled
  prior to scale space construction.

  @param features array of features
*/
//tristan
static void adjust_for_img_dbl(Seq* features)
{
	Feature* feat;
	int i, n;

	n = features->total;
	for (i = 0; i < n; i++)
	{
		feat = getSeqElem((Seq*)(features), i);
		feat->x /= 2.0;
		feat->y /= 2.0;
		feat->scl /= 2.0;
		feat->img_pt.x /= 2.0;
		feat->img_pt.y /= 2.0;
	}
}



/*
  Computes a canonical orientation for each image feature in an array.  Based
  on Section 5 of Lowe's paper.  This function adds features to the array when
  there is more than one dominant orientation at a given feature location.

  @param features an array of image features
  @param gauss_pyr Gaussian scale space pyramid
*/
//tristan
static void calc_feature_oris(Seq* features, IplImage*** gauss_pyr)
{
	Feature* feat;
	DetectionData* ddata;
	double* hist;
	double omax;
	int i, j, n = features->total;

	for (i = 0; i < n; i++)
	{

		feat = malloc(sizeof(Feature));
		seqPopFront(features, feat);
		ddata = feat_detection_data(feat);
		hist = ori_hist(gauss_pyr[ddata->octv][ddata->intvl],
			ddata->r, ddata->c, SIFT_ORI_HIST_BINS,
			iround(SIFT_ORI_RADIUS * ddata->scl_octv),
			SIFT_ORI_SIG_FCTR * ddata->scl_octv);
		for (j = 0; j < SIFT_ORI_SMOOTH_PASSES; j++)
			smooth_ori_hist(hist, SIFT_ORI_HIST_BINS);
		omax = dominant_ori(hist, SIFT_ORI_HIST_BINS);
		add_good_ori_features(features, hist, SIFT_ORI_HIST_BINS,
			omax * SIFT_ORI_PEAK_RATIO, feat);
		free(ddata);
		free(feat);
		free(hist);
	}
}


/*
  Computes a gradient orientation histogram at a specified pixel.

  @param img image
  @param r pixel row
  @param c pixel col
  @param n number of histogram bins
  @param rad radius of region over which histogram is computed
  @param sigma std for Gaussian weighting of histogram entries

  @return Returns an n-element array containing an orientation histogram
    representing orientations between 0 and 2 PI.
*/
//tristan
static double* ori_hist(IplImage* img, int r, int c, int n, int rad,
	double sigma)
{
	double* hist;
	double mag, ori, w, exp_denom, PI2 = PI * 2.0;
	int bin, i, j;

	hist = calloc(n, sizeof(double));
	exp_denom = 2.0 * sigma * sigma;
	for (i = -rad; i <= rad; i++)
		for (j = -rad; j <= rad; j++)
			if (calc_grad_mag_ori(img, r + i, c + j, &mag, &ori))
			{
				w = exp(-(i*i + j*j) / exp_denom);
				bin = iround(n * (ori + PI) / PI2);
				bin = (bin < n) ? bin : 0;
				hist[bin] += w * mag;
			}

	return hist;
}



/*
  Calculates the gradient magnitude and orientation at a given pixel.

  @param img image
  @param r pixel row
  @param c pixel col
  @param mag output as gradient magnitude at pixel (r,c)
  @param ori output as gradient orientation at pixel (r,c)

  @return Returns 1 if the specified pixel is a valid one and sets mag and
    ori accordingly; otherwise returns 0
*/
//tristan
static int calc_grad_mag_ori(IplImage* img, int r, int c, double* mag,
	double* ori)
{
	double dx, dy;

	if (r > 0 && r < img->height - 1 && c > 0 && c < img->width - 1)
	{
		dx = pixval32f(img, r, c + 1) - pixval32f(img, r, c - 1);
		dy = pixval32f(img, r - 1, c) - pixval32f(img, r + 1, c);
		*mag = sqrt(dx*dx + dy*dy);
		*ori = atan2(dy, dx);
		return 1;
	}

	else
		return 0;
}



/*
  Gaussian smooths an orientation histogram.

  @param hist an orientation histogram
  @param n number of bins
*/
//tristan
static void smooth_ori_hist(double* hist, int n)
{
	double prev, tmp, h0 = hist[0];
	int i;

	prev = hist[n - 1];
	for (i = 0; i < n; i++)
	{
		tmp = hist[i];
		hist[i] = 0.25 * prev + 0.5 * hist[i] +
			0.25 * ((i + 1 == n) ? h0 : hist[i + 1]);
		prev = tmp;
	}
}



/*
  Finds the magnitude of the dominant orientation in a histogram

  @param hist an orientation histogram
  @param n number of bins

  @return Returns the value of the largest bin in hist
*/
//tristan
static double dominant_ori(double* hist, int n)
{
	double omax;

	omax = hist[0];
	for (int i = 1; i < n; i++)
		if (hist[i] > omax)
		{
			omax = hist[i];
		}
	return omax;
}



/*
  Interpolates a histogram peak from left, center, and right values
*/




/*
  Adds features to an array for every orientation in a histogram greater than
  a specified threshold.

  @param features new features are added to the end of this array
  @param hist orientation histogram
  @param n number of bins in hist
  @param mag_thr new features are added for entries in hist greater than this
  @param feat new features are clones of this with different orientations
*/
//tristan
static void add_good_ori_features(Seq* features, double* hist, int n,
	double mag_thr, Feature* feat)
{
	Feature* new_feat;
	double bin, PI2 = PI * 2.0;
	int l, r, i;

	for (i = 0; i < n; i++)
	{
		l = (i == 0) ? n - 1 : i - 1;
		r = (i + 1) % n;

		if (hist[i] > hist[l] && hist[i] > hist[r] && hist[i] >= mag_thr)
		{
			bin = i + interp_hist_peak(hist[l], hist[i], hist[r]);
			bin = (bin < 0) ? n + bin : (bin >= n) ? bin - n : bin;
			new_feat = clone_feature(feat);
			new_feat->ori = ((PI2 * bin) / n) - PI;
			seqPush(features, new_feat);
			free(new_feat);
		}
	}
}



/*
  Makes a deep copy of a feature

  @param feat feature to be cloned

  @return Returns a deep copy of feat
*/
//tristan
static Feature* clone_feature(Feature* feat)
{
	Feature* new_feat;
	DetectionData* ddata;

	new_feat = new_feature();
	ddata = feat_detection_data(new_feat);
	memcpy(new_feat, feat, sizeof(Feature));
	memcpy(ddata, feat_detection_data(feat), sizeof(DetectionData));
	new_feat->feature_data = ddata;

	return new_feat;
}


/*
  Computes feature descriptors for features in an array.  Based on Section 6
  of Lowe's paper.

  @param features array of features
  @param gauss_pyr Gaussian scale space pyramid
  @param d width of 2D array of orientation histograms
  @param n number of bins per orientation histogram
*/
//tristan
static void compute_descriptors(Seq* features, IplImage*** gauss_pyr, int d,
	int n)
{
	Feature* feat;
	DetectionData* ddata;
	double*** hist;
	int i, k = features->total;

	for (i = 0; i < k; i++)
	{
		feat = getSeqElem((Seq*)(features), i);
		ddata = feat_detection_data(feat);
		hist = descr_hist(gauss_pyr[ddata->octv][ddata->intvl], ddata->r,
			ddata->c, feat->ori, ddata->scl_octv, d, n);
		hist_to_descr(hist, d, n, feat);
		release_descr_hist(&hist, d);
	}
}



/*
  Computes the 2D array of orientation histograms that form the feature
  descriptor.  Based on Section 6.1 of Lowe's paper.

  @param img image used in descriptor computation
  @param r row coord of center of orientation histogram array
  @param c column coord of center of orientation histogram array
  @param ori canonical orientation of feature whose descr is being computed
  @param scl scale relative to img of feature whose descr is being computed
  @param d width of 2d array of orientation histograms
  @param n bins per orientation histogram

  @return Returns a d x d array of n-bin orientation histograms.
*/
//tristan
static double*** descr_hist(IplImage* img, int r, int c, double ori,
	double scl, int d, int n)
{
	double*** hist;
	double cos_t, sin_t, hist_width, exp_denom, r_rot, c_rot, grad_mag,
		grad_ori, w, rbin, cbin, obin, bins_per_rad, PI2 = 2.0 * PI;
	int radius, i, j;

	hist = calloc(d, sizeof(double**));
	for (i = 0; i < d; i++)
	{
		hist[i] = calloc(d, sizeof(double*));
		for (j = 0; j < d; j++)
		{
			hist[i][j] = calloc(n, sizeof(double));
		}
	}

	cos_t = cos(ori);
	sin_t = sin(ori);
	bins_per_rad = n / PI2;
	exp_denom = d * d * 0.5;
	hist_width = SIFT_DESCR_SCL_FCTR * scl;
	radius = hist_width * sqrt(2) * (d + 1.0) * 0.5 + 0.5;
	for (i = -radius; i <= radius; i++)
		for (j = -radius; j <= radius; j++)
		{
			/*
			  Calculate sample's histogram array coords rotated relative to ori.
			  Subtract 0.5 so samples that fall e.g. in the center of row 1 (i.e.
			  r_rot = 1.5) have full weight placed in row 1 after interpolation.
			  */
			c_rot = (j * cos_t - i * sin_t) / hist_width;
			r_rot = (j * sin_t + i * cos_t) / hist_width;
			rbin = r_rot + d / 2 - 0.5;
			cbin = c_rot + d / 2 - 0.5;

			if (rbin > -1.0  &&  rbin < d  &&  cbin > -1.0  &&  cbin < d)
				if (calc_grad_mag_ori(img, r + i, c + j, &grad_mag, &grad_ori))
				{
					grad_ori -= ori;
					while (grad_ori < 0.0)
						grad_ori += PI2;
					while (grad_ori >= PI2)
						grad_ori -= PI2;

					obin = grad_ori * bins_per_rad;
					w = exp(-(c_rot * c_rot + r_rot * r_rot) / exp_denom);
					interp_hist_entry(hist, rbin, cbin, obin, grad_mag * w, d, n);
				}
		}

	return hist;
}



/*
  Interpolates an entry into the array of orientation histograms that form
  the feature descriptor.

  @param hist 2D array of orientation histograms
  @param rbin sub-bin row coordinate of entry
  @param cbin sub-bin column coordinate of entry
  @param obin sub-bin orientation coordinate of entry
  @param mag size of entry
  @param d width of 2D array of orientation histograms
  @param n number of bins per orientation histogram
*/
//tristan
static void interp_hist_entry(double*** hist, double rbin, double cbin,
	double obin, double mag, int d, int n)
{
	double d_r, d_c, d_o, v_r, v_c, v_o;
	double** row, *h;
	int r0, c0, o0, rb, cb, ob, r, c, o;

	r0 = ifloor(rbin);
	c0 = ifloor(cbin);
	o0 = ifloor(obin);
	d_r = rbin - r0;
	d_c = cbin - c0;
	d_o = obin - o0;

	/*
	  The entry is distributed into up to 8 bins.  Each entry into a bin
	  is multiplied by a weight of 1 - d for each dimension, where d is the
	  distance from the center value of the bin measured in bin units.
	  */
	for (r = 0; r <= 1; r++)
	{
		rb = r0 + r;
		if (rb >= 0 && rb < d)
		{
			v_r = mag * ((r == 0) ? 1.0 - d_r : d_r);
			row = hist[rb];
			for (c = 0; c <= 1; c++)
			{
				cb = c0 + c;
				if (cb >= 0 && cb < d)
				{
					v_c = v_r * ((c == 0) ? 1.0 - d_c : d_c);
					h = row[cb];
					for (o = 0; o <= 1; o++)
					{
						ob = (o0 + o) % n;
						v_o = v_c * ((o == 0) ? 1.0 - d_o : d_o);
						h[ob] += v_o;
					}
				}
			}
		}
	}
}



/*
  Converts the 2D array of orientation histograms into a feature's descriptor
  vector.

  @param hist 2D array of orientation histograms
  @param d width of hist
  @param n bins per histogram
  @param feat feature into which to store descriptor
*/
//tristan
static void hist_to_descr(double*** hist, int d, int n, Feature* feat)
{
	int int_val, i, r, c, o, k = 0;

	for (r = 0; r < d; r++)
		for (c = 0; c < d; c++)
			for (o = 0; o < n; o++)
				feat->descr[k++] = hist[r][c][o];

	feat->d = k;
	normalize_descr(feat);
	for (i = 0; i < k; i++)
		if (feat->descr[i] > SIFT_DESCR_MAG_THR)
			feat->descr[i] = SIFT_DESCR_MAG_THR;
	normalize_descr(feat);

	/* convert floating-point descriptor to integer valued descriptor */
	for (i = 0; i < k; i++)
	{
		int_val = SIFT_INT_DESCR_FCTR * feat->descr[i];
		feat->descr[i] = MIN(255, int_val);
	}
}



/*
  Normalizes a feature's descriptor vector to unitl length

  @param feat feature
*/
//tristan
static void normalize_descr(Feature* feat)
{
	double cur, len_inv, len_sq = 0.0;
	int i, d = feat->d;

	for (i = 0; i < d; i++)
	{
		cur = feat->descr[i];
		len_sq += cur*cur;
	}
	len_inv = 1.0 / sqrt(len_sq);
	for (i = 0; i < d; i++)
		feat->descr[i] *= len_inv;
}



/*
  Compares features for a decreasing-scale ordering.  Intended for use with
  seqSort

  @param feat1 first feature
  @param feat2 second feature
  @param param unused

  @return Returns 1 if feat1's scale is greater than feat2's, -1 if vice versa,
    and 0 if their scales are equal
*/
//tristan
static int feature_cmp(void* feat1, void* feat2, void* param)
{
	Feature* f1 = (Feature*)feat1;
	Feature* f2 = (Feature*)feat2;

	if (f1->scl < f2->scl)
		return 1;
	if (f1->scl > f2->scl)
		return -1;
	return 0;
}



/*
  De-allocates memory held by a descriptor histogram

  @param hist pointer to a 2D array of orientation histograms
  @param d width of hist
*/
//tristan
static void release_descr_hist(double**** hist, int d)
{
	int i, j;

	for (i = 0; i < d; i++)
	{
		for (j = 0; j < d; j++)
			free((*hist)[i][j]);
		free((*hist)[i]);
	}
	free(*hist);
	*hist = NULL;
}


/*
  De-allocates memory held by a scale space pyramid

  @param pyr scale space pyramid
  @param octvs number of octaves of scale space
  @param n number of images per octave
*/
//tristan
void release_pyr(IplImage**** pyr, int octvs, int n)
{
	int i, j;
	for (i = 0; i < octvs; i++)
	{
		for (j = 0; j < n; j++)
			releaseImage(&(*pyr)[i][j]);
		free((*pyr)[i]);
	}
	free(*pyr);
	*pyr = NULL;
}


extern IplImage* stack_imgs(IplImage* img1, IplImage* img2)
{
	IplImage* stacked = createImage(isize(MAX(img1->width, img2->width),
		img1->height + img2->height),
		IPL_DEPTH_8U, 3);
	setZero(stacked);
	setImageROI(stacked, irect(0, 0, img1->width, img1->height));
	setImageROI(stacked, irect(0, img1->height, img2->width, img2->height));
	resetImageROI(stacked);

	return stacked;
}
