#include <math.h>
#include <stdio.h>
#include <algorithm>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
// #include "TEncPreanalyzer.h"

using namespace std;
typedef       short               Pel;
typedef unsigned char uint8_t;
typedef signed short int int16_t;
typedef unsigned short int uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long int int64_t;

#define CLIP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define Q16_SHIFT   16
#define Q16_MULT    (1 << Q16_SHIFT)  // 65536
#define Q16_ROUND   (1 << (Q16_SHIFT - 1))
// #define PAD_SIZE             64
#define PAD_SIZE    2
#define QUANT_STEP 12
#define SCALE_FACTOR_TAN 256
#define NUM_BINS   30
#define NUM_BINS   30
#define HALF_BINS  (NUM_BINS/2)

#define FLT_MAX		__FLT_MAX__
#define DBL_MAX		__DBL_MAX__
#define LDBL_MAX	__LDBL_MAX__

int32_t **CurrJND;
unsigned char *PrevFrame;
unsigned char *MypicYuvPad;
uint8_t *Lc;

int Mask[5][5] =
{ 1, 1, 1, 1, 1,
1, 2, 2, 2, 1,
1, 2, 0, 2, 1,
1, 2, 2, 2, 1,
1, 1, 1, 1, 1
};
double Fh[3][3] =
{
	1.0 / 3, 1.0 / 3, 1.0 / 3,
	0, 0, 0,
	-1.0 / 3, -1.0 / 3, -1.0 / 3
};
// double Fh[3][3] =
// {
// 	-1.0 / 3, 0, 1.0 / 3,
// 	-1.0 / 3, 0, 1.0 / 3,
// 	-1.0 / 3, 0, 1.0 / 3
// };
double Fv[3][3] =
{
	1.0 / 3, 0, -1.0 / 3,
	1.0 / 3, 0, -1.0 / 3,
	1.0 / 3, 0, -1.0 / 3
};

// double Fv[3][3] =
// {
// 	-1.0 / 3, -1.0 / 3, -1.0 / 3,
// 	0, 0, 0,
// 	1.0 / 3, 1.0 / 3, 1.0 / 3
// };

static const int32_t COEFF_A   = (int32_t)(0.0000957 * Q16_MULT + 0.5);
static const int32_t COEFF_B   = (int32_t)(0.009     * Q16_MULT + 0.5);
static const int32_t COEFF_C   = (int32_t)(0.8       * Q16_MULT + 0.5);
static const int32_t COEFF_D   = (int32_t)(0.004     * Q16_MULT + 0.5);

static void expand_pix_border(unsigned char *pix, int i_width, int i_height, int i_padh, int i_padv, int b_pad_top, int b_pad_bottom)
{
#define PPIXEL_PAD(x, y) ( MypicYuvPad + (x) + (y)*(i_width+2*i_padh) )
#define PPIXEL(x, y) ( pix + (x) + (y)*i_width )
	int y;
	for (y = 0; y < i_height; y++)
	{
		/* left band */
		memset(PPIXEL_PAD(0, y + i_padv), PPIXEL(0, y)[0], i_padh);
		/* right band */
		memset(PPIXEL_PAD(i_width + i_padh, y + i_padv), PPIXEL(i_width - 1, y)[0], i_padh);
	}
	for (y = 0; y < i_height; y++)
	{
		/* center band */
		memcpy(PPIXEL_PAD(i_padh, y + i_padv), PPIXEL(0, y), i_width);
	}

	/* upper band */
	if (b_pad_top)
		for (y = 0; y < i_padv; y++)
			memcpy(PPIXEL_PAD(0, y), PPIXEL_PAD(0, i_padv), i_width + 2 * i_padh);
	/* lower band */
	if (b_pad_bottom)
		for (y = 0; y < i_padv; y++)
			memcpy(PPIXEL_PAD(0, i_height + i_padv + y), PPIXEL_PAD(0, i_height - 1 + i_padv), i_width + 2 * i_padh);
#undef PPIXEL
}

#define LUT_BITS   10
#define SHIFT_BITS 6
#define FIXED_SCALE 256
#define LUT_SIZE (1 << LUT_BITS)
uint16_t sqrt_lut[LUT_SIZE];

void InitSqrtLUT() {
    for (int i = 0; i < LUT_SIZE; i++) {
        float S = (i << SHIFT_BITS) + 0.5f;
        float val = sqrtf(S);
        sqrt_lut[i] = (uint16_t)(val * FIXED_SCALE);
    }
}
// int count_Lc = 0;
uint8_t CalculateLc(int16_t Gh, int16_t Gv, int pos_x, int pos_y) {
    if (pos_x == 10 && pos_y == 3) {
        printf("CalculateLc: Gh = %d, Gv = %d\n", Gh, Gv);
    }
    uint32_t S = (Gh * Gh + Gv * Gv) / 2;

    uint32_t index = S >> SHIFT_BITS;
    uint32_t frac = S & ((1 << SHIFT_BITS) - 1);

    if (index >= (LUT_SIZE - 1)) {
        return sqrt_lut[LUT_SIZE - 1] >> 8;
    }

    uint32_t val_low = sqrt_lut[index];
    uint32_t val_high = sqrt_lut[index + 1];
    uint32_t result = (val_low * ((1 << SHIFT_BITS) - frac) + val_high * frac) >> SHIFT_BITS;

    return (result + FIXED_SCALE / 2) >> 8;
}

#define SCALE_FACTOR_TAN 256
#define TAN_INF          1e6
#define TAN_INF_FIXED    ((int16_t)(TAN_INF * SCALE_FACTOR_TAN))
int16_t tan_edges_fixed[NUM_BINS + 1];

void InitOrientationLUT() {
    for (int i = 0; i <= NUM_BINS; i++) {
        float angle_deg = i * QUANT_STEP;
        if (fabsf(angle_deg - 90.0f) < 0.1f || fabsf(angle_deg - 270.0f) < 0.1f) {
            tan_edges_fixed[i] = TAN_INF_FIXED;
        } else {
            float angle_mod = fmodf(angle_deg, 180.0f);
            float tan_val = tanf(angle_mod * M_PI / 180.0f);
            tan_edges_fixed[i] = (int16_t)(tan_val * SCALE_FACTOR_TAN);
        }
    }
    tan_edges_fixed[NUM_BINS] = tan_edges_fixed[0];
}

int CalPixOrientation(int pos_x, int pos_y, unsigned char *src, int stride, int width) {
    int i, j;
    float Gh = 0, Gv = 0;
    // for (j = 1; j <= 3; j++) {
    //     for (i = 1; i <= 3; i++) {
    //         int idx = (pos_y - 1 + j) * stride + pos_x - 1 + i;
    //         Gh += src[idx] * Fh[j - 1][i - 1];
    //         Gv += src[idx] * Fv[j - 1][i - 1];
    //     }
    // }
    for (j = 1; j <= 3; j++) {
        for (i = 1; i <= 3; i++) {
            int idx = (pos_y - 2 + j) * stride + pos_x - 2 + i;		//REVIEW - pos_y = 0 的时候，idx < 0
            Gh += src[idx] * Fh[j - 1][i - 1];
            Gv += src[idx] * Fv[j - 1][i - 1];
            if (pos_x == 10 && pos_y == 3) {
                printf("src[%d] = %d, ", idx, src[idx]);
            }
        }
    }
    if (pos_y==3 && pos_x == 10) {
        printf("Gh = %f, Gv = %f\n", Gh, Gv);
    }

    *(Lc + pos_x + pos_y * width) = CalculateLc(Gh, Gv, pos_x, pos_y);
    // if (pos_y == 0) {
    //     printf("Lc[%d]=%d\n", pos_x + pos_y * width, Lc[pos_x + pos_y * width]);
    // }

    if (fabsf(Gh) < 1e-5) {
        return (Gv > 0) ? (90 / QUANT_STEP) : (270 / QUANT_STEP);
    }
    float ratio = Gv / Gh;
    int16_t ratio_fixed = (int16_t)(ratio * SCALE_FACTOR_TAN);
    // if (pos_y == 0) {
    //     printf("ratio_fixed=%d\n", ratio_fixed);
    // }
    int gh_sign = (Gh > 0) ? 1 : -1;
    int gv_sign = (Gv > 0) ? 1 : -1;

    int start, end;

    if (gh_sign > 0 && gv_sign > 0) {       //bin0 ~ bin7
        start = 0; end = 7;
    } else if (gh_sign < 0 && gv_sign > 0) { //bin8 ~ bin14
        start = 8; end = 14;
    } else if (gh_sign < 0 && gv_sign < 0) { //bin15 ~ bin22
        start = 15; end = 22;
    } else {                                //bin23 ~ bin29
        start = 23; end = 29;
    }

    bool positive_quadrant = (gh_sign > 0 && gv_sign > 0) || (gh_sign < 0 && gv_sign < 0);
    // if(pos_x == 0 && pos_y == 0) {
    //     printf("tan_edges_fixed:\n");
    //     for (int i = 0; i < 32; i++) {
    //         printf("%d, ", tan_edges_fixed[i]);
    //     }
    //     printf("\n");
    // }
    for (int bin = start; bin <= end; bin++) {
        int16_t lower = tan_edges_fixed[bin];
        int16_t upper = tan_edges_fixed[bin + 1];
        bool in_bin;
        if (positive_quadrant) {
            in_bin = (ratio_fixed >= lower) && (ratio_fixed < upper);
        } else {
            in_bin = (ratio_fixed <= lower) && (ratio_fixed > upper);
        }
        if (bin == 7 || bin == 22) {
            if (positive_quadrant) {
                in_bin = ratio_fixed >= lower;
            } else {
                in_bin = ratio_fixed <= upper;
            }
        }
        if (in_bin) return bin;
    }

    return start;
}

int cal_back_luminace_fixed(int cur_i, int cur_j, unsigned char *psrc, int width)
{
	int k, m;
	int average_luminace = 0.0;
	for (m = 1; m <= 5; m++)
		for (k = 1; k <= 5; k++)
		{
		  average_luminace += psrc[cur_i - 3 + k + (cur_j - 3 + m)*width] * Mask[m - 1][k - 1];
		}
	average_luminace /= 32;         //REVIEW - 为什么这里要除以32
	return average_luminace;
}

#define Q6_SHIFT 6
#define Q6_MULT (1 << Q6_SHIFT) // 64
#define LUT_SIZE_LA  128
static uint16_t sqrt_la_lut[LUT_SIZE_LA];
const float N_LUT[9] = {
    /* N=1 */ 0.150000f *Q6_MULT,  // 0.3*1^2.7/(1+1)   = 0.15
    /* N=2 */ 0.389883f *Q6_MULT,  // 0.3*6.498052/5    = 0.389883
    /* N=3 */ 0.582571f *Q6_MULT,  // 0.3*19.419017/10  = 0.582571
    /* N=4 */ 0.745135f *Q6_MULT,  // 0.3*42.224253/17  = 0.745135
    /* N=5 */ 0.890106f *Q6_MULT,  // 0.3*77.142683/26  = 0.890106
    /* N=6 */ 1.217451f *Q6_MULT,  // 0.3*150.152436/37 = 1.217451
    /* N=7 */ 1.363366f *Q6_MULT,  // 0.3*227.227739/50 = 1.363366
    /* N=8 */ 1.670932f *Q6_MULT,  // 0.3*362.035227/65 = 1.670932
    /* N=9 */ 2.022455f *Q6_MULT   // 0.3*552.804419/82 = 2.022455
};
void InitLuminanceAdaptLUT() {
    const float sqrt127 = sqrtf(127.0f);
    for (int B = 0; B < LUT_SIZE_LA; B++) {
        float val = sqrtf((float)B) / sqrt127;
        sqrt_la_lut[B] = (uint16_t)(val * Q6_MULT);
    }
}

double jnd_lumina(unsigned char *src, int cur_i, int cur_j, int stride) {
    int bg = cal_back_luminace_fixed(cur_i, cur_j, src, stride);
    double jnd_l;

    if (bg <= 127) {
        int B = (int)(bg + 0.5);
        B = (B < 0) ? 0 : (B > 127) ? 127 : B;
        int sqrt_val = sqrt_la_lut[B];
        jnd_l = 17  * (64 - sqrt_val) + (3 << Q6_SHIFT);
    } else {
        jnd_l = ((3/128)<< Q6_SHIFT) * (bg - 127) + (3 << Q6_SHIFT);
    }
    return jnd_l;
}

int CalPixOrientDiff(int x, int y, int *PixOrient, int width, int height)
{
    int center_bin = PixOrient[y*width + x];
    int bins[NUM_BINS] = {0};

    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    for (int i = 0; i < 8; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;

        int neighbor_bin = PixOrient[ny*width + nx];
        int raw_diff = abs(neighbor_bin - center_bin);
        int circular_diff = (raw_diff > HALF_BINS) ? (NUM_BINS - raw_diff) : raw_diff;

        if (circular_diff > 0) {
            int quant_diff = (circular_diff * QUANT_STEP) / QUANT_STEP;
            bins[quant_diff] = 1;
        }
    }

    int N = 0;
    for (int i = 0; i < NUM_BINS; i++) {
        N += bins[i];
    }
    return N;
}

#define LUT_LC_SIZE 256
int lmLUT[LUT_LC_SIZE];
#define LUT_LC_SIZE 256
#define MAX_LC 255.0

void init_lmLUT() {
    for (int i = 0; i < LUT_LC_SIZE; i++) {
        double lc = (MAX_LC * i) / (LUT_LC_SIZE - 1);
        lmLUT[i] = (1.84 * pow(lc, 2.4) / (pow(lc, 2) + 676))*Q6_MULT;
    }
}

int calculate_vm_lc(uint8_t Lc) {
    double pos = Lc  / MAX_LC * (LUT_LC_SIZE - 1);
    int idx = (int) pos;
    if (idx >= LUT_LC_SIZE - 1)
        return lmLUT[LUT_LC_SIZE - 1];
    double frac = pos - idx;
    return lmLUT[idx] * (1 - frac) + lmLUT[idx + 1] * frac;
}

int calculate_vm(uint8_t Lc, int N) {
    if (N < 1 || N > 9) return 0;
    int vm_lc = calculate_vm_lc(Lc);
    int vm_n = N_LUT[N - 1];
    return vm_lc * vm_n;
}

int32_t CalFidl_Fixed(int i, int j, uint8_t *src, uint8_t *src_pre, int width, int height, int stride) {
    int32_t idl, Fidl;
    int lum_td, bg_t, bg_pret;

    lum_td = (int)(*(src + j*width + i) - *(src_pre + j*width + i));
    bg_t   = cal_back_luminace_fixed(i, j, src, stride);
    bg_pret= cal_back_luminace_fixed(i, j, src_pre, stride);

    idl = (lum_td + bg_t - bg_pret) >> 1;

    if (idl < 0) {
        int64_t term1 = (int64_t)COEFF_A * (int64_t)idl * (int64_t)idl;
        int64_t term2 = (int64_t)COEFF_B * (int64_t)idl;
        Fidl = (int32_t)(term1 + term2 + COEFF_C);
    } else {
        int64_t term = (int64_t)COEFF_D * (int64_t)idl;
        Fidl = (int32_t)(term + COEFF_C);
    }

    return Fidl;
}

float CalFidl(int i, int j, unsigned char *src, unsigned char *src_pre, int width, int height, int stride)
{
	double idl;
	double Fidl;

	int lum_td, bg_t, bg_pret;

	lum_td = *(src + j*width + i) - *(src_pre + j*width + i);
	bg_t = cal_back_luminace_fixed(i, j, src, stride);
	bg_pret = cal_back_luminace_fixed(i, j, src_pre, stride);
	idl = (lum_td + bg_t - bg_pret)*0.5;

	if (idl < 0)
	{
		Fidl = 0.0000957* idl * idl +0.009 * idl + 0.8;
	}
	else
	{
		Fidl = 0.004*idl + 0.8;
	}

	return Fidl;

}

void CalculateJNDQP(int iWidth, int iHeight, int frame_Idx, int SliceQP)
{

	double dJndActivity, dVarianceJnd, dAverageJnd;
	double dSumJNDAct = 0.0;

	double dSumAct = 0.0;

	int uiAQPartWidth = 1 << 4;
	int uiAQPartHeight = 1 << 4;
	int LCU_NUM = 0;
	int x, y;
	double *AQ_ArryCUJND16 = (double*)malloc(8160*sizeof(double));
	for (y = 0; y < iHeight; y += uiAQPartHeight)
	{
		const int uiCurrAQPartHeight = min(uiAQPartHeight, iHeight - y);
		for (x = 0; x < iWidth; x += uiAQPartWidth)
		{
			const int uiCurrAQPartWidth = min(uiAQPartWidth, iWidth - x);
			double uiSumJnd = 0;
			double uiSumSqJnd = 0;

			int uiNumPixInAQPart = 0;
			for (int by = 0; by < uiCurrAQPartHeight; by++)
			{
				for (int bx = 0; bx < uiCurrAQPartWidth; bx++, uiNumPixInAQPart++)
				{

					uiSumJnd += (int)CurrJND[y+by][x+bx];
				}
			}

			double dMinVar = DBL_MAX;

			// for (int i = 0; i < 1; i++)
			// {
				dAverageJnd = (uiSumJnd) / uiNumPixInAQPart;
				for (int by = 0; by < uiCurrAQPartHeight; by++)
				{
					for (int bx = 0; bx < uiCurrAQPartWidth; bx++, uiNumPixInAQPart++)
					{
						uiSumSqJnd += ((int)CurrJND[y + by][x + bx] -dAverageJnd)*((int)CurrJND[y + by][x + bx] -dAverageJnd);
					}
				}
				dVarianceJnd = (uiSumSqJnd) / uiNumPixInAQPart;
				dVarianceJnd = min(DBL_MAX, dVarianceJnd);
			    dJndActivity = 4096 + (int)(dVarianceJnd);
			// }

			const double dActivity = 1.0 + dMinVar;

			AQ_ArryCUJND16[LCU_NUM] = dJndActivity;
			dSumJNDAct += dJndActivity;
			LCU_NUM++;
			dSumAct += dActivity;
		}
	}

	for (int k = 0; k < iHeight;k++)
	{
		free(*(CurrJND + k));
	}


	int mb_width_num = (iWidth % 16 == 0) ? (iWidth / 16) : iWidth / 16 + 1;
	int mb_height_num = (iHeight % 16 == 0) ? (iHeight / 16) : iHeight / 16 + 1;
	double deltaQP_MB[8160];
	double dCUAct;
	double sumVariance, sumDeltaQP,averageQP;

	sumVariance = 0;
	sumDeltaQP = 0;

	for (int i = 0; i < mb_width_num*mb_height_num;i++)
	{
		dCUAct = AQ_ArryCUJND16[i];
#ifdef HW_JND
		deltaQP_MB[i] = Q16_16_TO_FLOAT(compute_log2_fixed(dCUAct));
#else
		deltaQP_MB[i] =log(dCUAct) / log(2.0F) - 12;
#endif
		sumVariance += deltaQP_MB[i] * deltaQP_MB[i];
		sumDeltaQP += deltaQP_MB[i];
	}
	averageQP = sumDeltaQP / (mb_width_num*mb_height_num);
	double deltaQP_frame = averageQP;
	int avgQP = 0;

	for (int i = 0; i < mb_height_num; i++)
	{
		for (int j = 0; j < mb_width_num; j++)
		{
			double weight = /*g_algo_cfg.JNDweight*/0.8;
			double qp = weight *(deltaQP_MB[i*mb_width_num + j] - deltaQP_frame);

			qp = (floor(qp + 0.49999));
			avgQP += qp;

		}

	}
	avgQP = avgQP / (mb_width_num*mb_height_num);
	free(AQ_ArryCUJND16);
}

void xPreanalyzeFrameJND(Pel* pY, int iWidth, int iHeight, int iStride, int frameIdx, int FrameLeft)
{
    printf("img_width = %d, img_height = %d, img_stride = %d, frameIdx = %d, frame_left = %d\n", iWidth, iHeight, iStride, frameIdx, FrameLeft);
// #ifdef HW_JND
	// InitSqrtLUT();
    // for (int i = 0; i < 10; i++) {
    //     printf("%d, ", sqrt_lut[i]);
    // }
    // printf("\n");
	// InitOrientationLUT();
    // for (int i = 0; i < NUM_BINS + 1; i++) {
    //     printf("%d, ", tan_edges_fixed[i]);
    // }
    // printf("\n");
	// InitLuminanceAdaptLUT();
    // for (int i = 0; i < 128; i++) {
    //     printf("%d, ", sqrt_la_lut[i]);
    //     if ((i+1) % 16 == 0) {
    //         printf("\n");
    //     }
    // }
	// init_lmLUT();
    // for (int i = 0; i < 256; i++) {
    //     printf("%d, ", lmLUT[i]);
    //     if ((i+1) % 16 == 0) {
    //         printf("\n");
    //     }
    // }
// #endif

	double *LA = (double*)malloc(iWidth*iHeight*sizeof(double));
	unsigned char *CurrFrame = (unsigned char*)malloc(iWidth*iHeight*sizeof(unsigned char));

	if(frameIdx == 0)
		PrevFrame = (unsigned char*)malloc(iWidth*iHeight*sizeof(unsigned char));

	MypicYuvPad = (unsigned char*)malloc((iWidth + 2 * PAD_SIZE)*(iHeight + 2 * PAD_SIZE)*sizeof(unsigned char));

	CurrJND = (int32_t**)malloc(iHeight*sizeof(int32_t*));
	for (int k = 0; k < iHeight; k++)
	{
		*(CurrJND + k) = (int32_t*)malloc(iWidth*sizeof(int32_t));
	}

	int *PixOrient = (int*)malloc(iWidth*iHeight*sizeof(int));
	int   NumOrient;
	double LcPix, Vm, LAPix;
	int i, j;
	int PadSride = iWidth + 2 * PAD_SIZE;
	Lc = (uint8_t*)malloc(iWidth*iHeight*sizeof(uint8_t));

	for (j = 0; j < iHeight; j++)
	{
		for (i = 0; i < iWidth; i++)
		{
      		CurrFrame[j*iWidth + i] = (unsigned char)pY[j*iStride+i];
		}
	}

    // printf("CurrFrame:\n");
    // for (int i = 0; i < iWidth * iHeight; i++) {
    //     printf("%d, ", CurrFrame[i]);
    //     if ((i+1) % iWidth == 0) {
    //         printf("\n");
    //     }
    // }
	expand_pix_border(CurrFrame, iWidth, iHeight, PAD_SIZE, PAD_SIZE, PAD_SIZE, PAD_SIZE);
    printf("padded_frame[%d]:\n", frameIdx);
    for (int i = 0; i < (iWidth + 2 * PAD_SIZE) * (iHeight + 2 * PAD_SIZE); i++) {
        printf("%d, ", MypicYuvPad[i]);
        if ((i+1) % (iWidth + 2 * PAD_SIZE) == 0) {
            printf("\n");
        }
    }
    // printf("PixOrient:\n");
	for (j = 0; j < iHeight; j++)
	{
		for (i = 0; i < iWidth; i++)
		{
            // *(PixOrient + j*iWidth + i) = CalPixOrientation(i, j, MypicYuvPad + PAD_SIZE + (PAD_SIZE-1)*PadSride - 1, PadSride, iWidth);
            *(PixOrient + j*iWidth + i) = CalPixOrientation(i, j, MypicYuvPad + PAD_SIZE + PAD_SIZE*PadSride, PadSride, iWidth);
            // printf("%d, ", PixOrient[j*iWidth + i]);
		}
        // printf("\n");
	}

    // printf("LA:\n");
	for (j = 0; j < iHeight; j++)
	{
		for (i = 0; i < iWidth; i++)
		{
			*(LA + j*iWidth + i) = jnd_lumina(MypicYuvPad + PAD_SIZE + PAD_SIZE*PadSride, i, j, PadSride);

            // printf("%f, ", LA[j*iWidth + i]);
		}
	}
	double sum_jnd = 0;
    int count = 0;
	for (j = 0; j < iHeight; j++)
	{
		for (i = 0; i < iWidth; i++)
		{
			LcPix = *(Lc + j*iWidth + i);
			LAPix = *(LA + j*iWidth + i);
			NumOrient = CalPixOrientDiff(i, j, PixOrient, iWidth, iHeight);
            // if (count == 0) {
            //     printf("NumOrient: %d\n", NumOrient);
            //     printf("LcPix = %f\n", LcPix);
            //     printf("LAPix = %f\n", LAPix);
            // }
#ifdef HW_JND
            Vm = calculate_vm(LcPix, NumOrient)/64;
#else
			double Vm= 1.84*pow(LcPix, 2.4)*0.3*pow(NumOrient*1.0, 2.7) / (pow(LcPix, 2) + 26 * 26) / (NumOrient*NumOrient + 1);
#endif
            // if (count == 0) {
            //     // printf("Vm_org: %f\n", Vm_org);
            //     printf("Vm: %f\n", Vm);
            // }
			CurrJND[j][i] = (LAPix + Vm - 0.3*min(LAPix, Vm));
            // if (count == 0) {
            //     printf("CurrJND: %d\n", CurrJND[j][i]);
            // }
			if(frameIdx>0 && i > 3 && i < iWidth - 3 && j > 3 && j < iHeight - 3)       //REVIEW - 这里为什么是3
			{
// #ifdef HW_JND
				int32_t fidl = CalFidl_Fixed(i, j, CurrFrame, PrevFrame, iWidth, iHeight, iWidth);
// #else
				// double fidl = CalFidl(i, j, CurrFrame, PrevFrame, iWidth, iHeight, iWidth);
// #endif
                // if (count == 0) {
                // printf("fidl:\n");
                // printf("%f, ", fidl);
                // }
				CurrJND[j][i] = fidl*CurrJND[j][i] / Q16_MULT;
				sum_jnd += CurrJND[j][i];

			}
        count += 1;
		}
	}
    // printf("sum_jnd: %f\n", sum_jnd);
	sum_jnd = sum_jnd / (iWidth*iHeight);

	for (j = 0; j < iHeight; j++)
	{
		for (i = 0; i < iWidth; i++)
		{
      		PrevFrame[j*iWidth + i] = CurrFrame[j*iWidth + i];
		}
	}

	free(LA);
	free(CurrFrame);
	if(FrameLeft == 1)
		free(PrevFrame);
	free(MypicYuvPad);

	free(PixOrient);
	free(Lc);
}

int main() {
    srand(time(NULL));
    int width = 64;
    int height = 64;
    int stride = width;
    InitSqrtLUT();
    InitOrientationLUT();
    InitLuminanceAdaptLUT();
    init_lmLUT();
    int total_frames = 1;
    for (int frame = 0; frame < total_frames; frame++) {
        // printf("\nGenerating frame %d pattern...\n", frame);

        Pel* pY = (Pel*)malloc(width * height * sizeof(Pel));

        // 基础模式参数，随时间缓慢变化
        float time_factor = frame * 0.2f;  // 控制随时间变化的因子

        switch(frame) {
            case 0: // 初始帧 - 水平渐变
				// printf("Pattern: Initial Horizontal Gradient\n");
				for (int i = 0; i < height; i++) {
					for (int j = 0; j < width; j++) {
						// pY[i * stride + j] = (j * 255) / (width - 1);
                        pY[i * stride + j] = ((i + j) * 255) / (height + width - 2);
                        printf("%d, ", (unsigned char)pY[i * stride + j]);
					}
                    printf("\n");
				}
				break;

			case 1: // 第1帧 - 添加垂直渐变分量
				// printf("Pattern: Horizontal + Vertical Gradient (Frame 1)\n");
				for (int i = 0; i < height; i++) {
					for (int j = 0; j < width; j++) {
						// int base = (j * 255) / (width - 1);
						// int vertical = (i * 20) / (height - 1); // 添加轻微的垂直渐变
						// pY[i * stride + j] = (base + vertical) > 255 ? 255 : (base + vertical);
                        // pY[i * stride + j] = (j * 255) / (width - 1);
                        pY[i * stride + j] = ((i + j) * 255) / (height + width - 2);
                        pY[i * stride + j] = 1.2 * pY[i * stride + j];
                        printf("%d, ", (unsigned char)pY[i * stride + j]);
					}
                    printf("\n");
				}
				break;

			case 2: // 第2帧 - 添加对角线波浪效果
				// printf("Pattern: Diagonal Wave (Frame 2)\n");
				for (int i = 0; i < height; i++) {
					for (int j = 0; j < width; j++) {
						// int base = (j * 255) / (width - 1);
						// int wave = 20 * sin((i + j) * 0.1); // 对角线波浪效果
						// pY[i * stride + j] = (base + wave) > 255 ? 255 :
						// 				(base + wave) < 0 ? 0 : (base + wave);
                        pY[i * stride + j] = (j * 255) / (width - 1);
                        pY[i * stride + j] = 0.8 * pY[i * stride + j];
                        printf("%d, ", (unsigned char)pY[i * stride + j]);
					}
                    printf("\n");
				}
				break;

			case 3: // 第3帧 - 添加水平移动效果
				// printf("Pattern: Moving Gradient (Frame 3)\n");
				for (int i = 0; i < height; i++) {
					for (int j = 0; j < width; j++) {
						// int pos = (j + frame - 1) % width; // 随时间水平移动
						// pY[i * stride + j] = (pos * 255) / (width - 1);
                        pY[i * stride + j] = (j * 255) / (width - 1);
                        pY[i * stride + j] = 0.6 * pY[i * stride + j];
                        printf("%d, ", (unsigned char)pY[i * stride + j]);
					}
                    printf("\n");
				}
				break;

			case 4: // 第4帧 - 添加脉冲效果
				// printf("Pattern: Pulsing Gradient (Frame 4)\n");
				for (int i = 0; i < height; i++) {
					for (int j = 0; j < width; j++) {
						// int base = (j * 255) / (width - 1);
						// int pulse = 30 * sin((frame - 1) * 0.2); // 脉冲效果
						// pY[i * stride + j] = (base + pulse) > 255 ? 255 :
						// 				(base + pulse) < 0 ? 0 : (base + pulse);
                        pY[i * stride + j] = (j * 255) / (width - 1);
                        pY[i * stride + j] = 0.4 * pY[i * stride + j];
                        printf("%d, ", (unsigned char)pY[i * stride + j]);
					}
                    printf("\n");
				}
				break;

        }

        // 调用JND分析函数
        printf("Processing frame %d...\n", frame);
        xPreanalyzeFrameJND(pY, width, height, stride, frame, total_frames - frame);
        CalculateJNDQP(width, height, 0, 30);
        // 打印部分JND值用于观察
        printf("Sample JND values (center region):\n");
        for (int i = height/2 - 2; i <= height/2 + 2; i++) {
            for (int j = width/2 - 2; j <= width/2 + 2; j++) {
                printf("%4d ", CurrJND[i][j]);
            }
            printf("\n");
        }

        free(pY);
    }
    return 0;
}
