/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2020, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncPreanalyzer.cpp
    \brief    source picture analyzer class
*/

#include <cfloat>
#include <algorithm>
#include <math.h>
#include "TEncPreanalyzer.h"

using namespace std;

//! \ingroup TLibEncoder
//! \{

/** Constructor
 */
TEncPreanalyzer::TEncPreanalyzer()
{
}

/** Destructor
 */
TEncPreanalyzer::~TEncPreanalyzer()
{
}

/** Analyze source picture and compute local image characteristics used for QP adaptation
 * \param pcEPic Picture object to be analyzed
 * \return Void
 */

#ifdef USE_JND_MODEL
//******JND Model***************
#define PAD_SIZE             64
#define T0 17
#define GAMMA 0.0234375
#define LANDA 1/2
#define C_TG 0.3
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
double Fv[3][3] =
{
	1.0 / 3, 0, -1.0 / 3,
	1.0 / 3, 0, -1.0 / 3,
	1.0 / 3, 0, -1.0 / 3
};

unsigned char *MypicYuvPad;

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

uint8_t *Lc;
double cal_back_luminace(int cur_i, int cur_j, unsigned char *psrc, int width)
{
	int k, m;
	double average_luminace = 0.0;
	for (m = 1; m <= 5; m++)
		for (k = 1; k <= 5; k++)
		{
		average_luminace += psrc[cur_i - 3 + k + (cur_j - 3 + m)*width] * Mask[m - 1][k - 1];
		}
	average_luminace /= 32;
	return average_luminace;
}
#ifdef HW_JND


#define QUANT_STEP 12
#define NUM_BINS   30
#define HALF_BINS  (NUM_BINS/2)
#define TAN_INF 1e6

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

uint8_t CalculateLc(int16_t Gh, int16_t Gv) {
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
    for (j = 1; j <= 3; j++) {
        for (i = 1; i <= 3; i++) {
            int idx = (pos_y - 2 + j) * stride + pos_x - 2 + i;		//REVIEW - pos_y = 0 的时候，idx < 0
            Gh += src[idx] * Fh[j - 1][i - 1];
            Gv += src[idx] * Fv[j - 1][i - 1];
        }
    }

    *(Lc + pos_x + pos_y * width) = CalculateLc(Gh, Gv);

    if (fabsf(Gh) < 1e-5) {
        return (Gv > 0) ? (90 / QUANT_STEP) : (270 / QUANT_STEP);
    }
    float ratio = Gv / Gh;
    int16_t ratio_fixed = (int16_t)(ratio * SCALE_FACTOR_TAN);

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

#define Q6_SHIFT 6
#define Q6_MULT (1 << Q6_SHIFT) // 64
typedef uint32_t q6_t;

#define FLOAT_TO_Q6(x) ((q6_t)((x) * Q6_MULT + 0.5))
#define Q6_TO_FLOAT(x) ((double)(x) / Q6_MULT)
#define LUT_SIZE_LA  128
static uint16_t sqrt_la_lut[LUT_SIZE_LA];

void InitLuminanceAdaptLUT() {
    const float sqrt127 = sqrtf(127.0f);
    for (int B = 0; B < LUT_SIZE_LA; B++) {
        float val = sqrtf((float)B) / sqrt127;
        sqrt_la_lut[B] = (uint16_t)(val * Q6_MULT);
    }
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
	average_luminace /= 32;
	return average_luminace;
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
#else
int CalPixOrientation(int pos_x, int pos_y, unsigned char *src, int stride, int width)
{
	int i, j;
	double Gh, Gv;
	int Orient = 0;
	Gh = Gv = 0;
	for (j = 1; j <= 3; j++)
	{
		for (i = 1; i <= 3; i++)
		{
			Gh += src[pos_x - 2 + i + (pos_y - 2 + j)*stride] * Fh[j - 1][i - 1];
			Gv += src[pos_x - 2 + i + (pos_y - 2 + j)*stride] * Fv[j - 1][i - 1];
		}
	}
	Orient = ((atan(Gv / Gh)) * 180 / 3.1415926 + 0.4999);
	*(Lc + pos_x + pos_y*width) = sqrt((Gh*Gh + Gv*Gv) / 2);
	return Orient;
}
int cmp(const void *a, const void *b)
{
	return(*(int *)a - *(int *)b);
}
int CalPixOrientDiff(int pos_x, int pos_y, int *OrientOrg, int width, int height)
{
	int    Diff[9];
	int    QDiff[9];
	int    Count = 0;
	int i, j, k, temp_i;
	int Step = 12;
	int *Orient = OrientOrg + pos_x + pos_y*width;
	if (pos_x == 0 && pos_y == 0)
	{
		int dif1 = (*(Orient)-*(Orient + 1)) / Step;
		int dif2 = (*(Orient)-*(Orient + width)) / Step;
		if (dif1 != dif2)
		{
			Count++;
		}
	} //0

	else if (pos_x == 0 && pos_y<height - 1 && pos_y != 0)
	{
		int dif1 = *(Orient) / Step - *(Orient + 1) / Step;
		int dif2 = *(Orient) / Step - *(Orient - width) / Step;
		int dif3 = *(Orient) / Step - *(Orient + width) / Step;
		if (dif1 != dif2)
		{
			Count++;
		}
		if (dif1 != dif3)
		{
			Count++;
		}
		if (dif2 != dif3)
		{
			Count++;
		}
	} //1

	else if (pos_x == 0 && pos_y == height - 1)
	{
		int dif1 = (*(Orient)-*(Orient + 1)) / Step;
		int dif2 = (*(Orient) != *(Orient - width)) / Step;
		if (dif1 != dif2)
		{
			Count++;
		}
	}// 2

	else if (pos_x>0 && pos_x<width - 1 && pos_y == 0)
	{
		int dif1 = *(Orient) / Step - *(Orient + 1) / Step;
		int dif2 = *(Orient) / Step - *(Orient - 1) / Step;
		int dif3 = *(Orient) / Step - *(Orient + width) / Step;
		if (dif1 != dif2)
		{
			Count++;
		}
		if (dif1 != dif3)
		{
			Count++;
		}
		if (dif2 != dif3)
		{
			Count++;
		}
	} //3

	else if (pos_x == width - 1 && pos_y == 0)
	{
		int dif1 = (*(Orient)-*(Orient - 1)) / Step;
		int dif2 = (*(Orient)-*(Orient + width)) / Step;
		if (dif1 != dif2)
		{
			Count++;
		}
	}//4

	else if (pos_x == width - 1 && pos_y>0 && pos_y<height - 1)
	{
		int dif1 = *(Orient) / Step - *(Orient - 1) / Step;
		int dif2 = *(Orient) / Step - *(Orient - width) / Step;
		int dif3 = *(Orient) / Step - *(Orient + width) / Step;
		if (dif1 != dif2)
		{
			Count++;
		}
		if (dif1 != dif3)
		{
			Count++;
		}
		if (dif2 != dif3)
		{
			Count++;
		}
	}//5

	else if (pos_x>0 && pos_x<width - 1 && pos_y == height - 1)
	{
		int dif1 = *(Orient) / Step - *(Orient + 1) / Step;
		int dif2 = *(Orient) / Step - *(Orient - 1) / Step;
		int dif3 = *(Orient) / Step - *(Orient - width) / Step;
		if (dif1 != dif2)
		{
			Count++;
		}
		if (dif1 != dif3)
		{
			Count++;
		}
		if (dif2 != dif3)
		{
			Count++;
		}
	}//6

	else if (pos_x == width - 1 && pos_y == height - 1)
	{
		int dif1 = (*(Orient)-*(Orient - 1)) / Step;
		int dif2 = (*(Orient)-*(Orient - width)) / Step;
		if (dif1 != dif2)
		{
			Count++;
		}
	}//7


	else //8
	{
		k = 0;
		for (j = 1; j <= 3; j++)
		{
			for (i = 1; i <= 3; i++)
			{
				Diff[k++] = *(Orient)-*(Orient - 2 + i + (-2 + j)*width);
			}
		}
		for (k = 0; k<9; k++)
		{
			QDiff[k] = (Diff[k] * 1.0 / Step + 0.4999);
		}
		qsort(QDiff, 9, sizeof(int), cmp);
		temp_i = 0;
		for (i = temp_i; i<9; i++)
		{
			for (j = i + 1; j<9; j++)
			{
				if (QDiff[i] == QDiff[j])
				{
					continue;
				}
				else
				{
					Count++;
					i = j;
				}
			}
		}
	}
	return Count;
}


double jnd_lumina(unsigned char *src, int cur_i, int cur_j, int stride)
{
	// int i, j;
	double bg;
	double jnd_l;

	bg = cal_back_luminace(cur_i, cur_j, src, stride);
	if (bg <= 127)
	{
		jnd_l = T0*(1 - sqrt(bg / 127)) + 3;
	}
	else
	{
		jnd_l = GAMMA*(bg - 127) + 3;
	}
	return jnd_l;
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
#endif

#define Q16_SHIFT   16
#define Q16_MULT    (1 << Q16_SHIFT)  // 65536
#define Q16_ROUND   (1 << (Q16_SHIFT - 1))

static const int32_t COEFF_A   = (int32_t)(0.0000957 * Q16_MULT + 0.5);
static const int32_t COEFF_B   = (int32_t)(0.009     * Q16_MULT + 0.5);
static const int32_t COEFF_C   = (int32_t)(0.8       * Q16_MULT + 0.5);
static const int32_t COEFF_D   = (int32_t)(0.004     * Q16_MULT + 0.5);

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
#ifdef HW_JND
#define LUMINANCE_MAX 255
#define SMALL_REGION_THRES 26
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

#define LUT_LC_SIZE 256
#define MAX_LC 255.0

int lmLUT[LUT_LC_SIZE];

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
#endif

int32_t **CurrJND;
unsigned char *PrevFrame;
void xPreanalyzeFrameJND(Pel* pY, int iWidth, int iHeight, int iStride, Int frameIdx, Int FrameLeft)
{

#ifdef HW_JND
	InitSqrtLUT();
	InitOrientationLUT();
	InitLuminanceAdaptLUT();
	init_lmLUT();
#endif

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
	expand_pix_border(CurrFrame, iWidth, iHeight, PAD_SIZE, PAD_SIZE, PAD_SIZE, PAD_SIZE);

	for (j = 0; j < iHeight; j++)
	{
		for (i = 0; i < iWidth; i++)
		{
			*(PixOrient + j*iWidth + i) = CalPixOrientation(i, j, MypicYuvPad + PAD_SIZE + PAD_SIZE*PadSride, PadSride, iWidth);
		}
	}

	for (j = 0; j < iHeight; j++)
	{
		for (i = 0; i < iWidth; i++)
		{
			*(LA + j*iWidth + i) = jnd_lumina(MypicYuvPad + PAD_SIZE + PAD_SIZE*PadSride, i, j, PadSride);
		}
	}
	double sum_jnd = 0;
	for (j = 0; j < iHeight; j++)
	{
		for (i = 0; i < iWidth; i++)
		{
			LcPix = *(Lc + j*iWidth + i);
			LAPix = *(LA + j*iWidth + i);
			NumOrient = CalPixOrientDiff(i, j, PixOrient, iWidth, iHeight);
#ifdef HW_JND
            Vm = calculate_vm(LcPix, NumOrient)/64;
#else
			double Vm_org= 1.84*pow(LcPix, 2.4)*0.3*pow(NumOrient*1.0, 2.7) / (pow(LcPix, 2) + 26 * 26) / (NumOrient*NumOrient + 1);
#endif
			CurrJND[j][i] = (LAPix + Vm - 0.3*min(LAPix, Vm));
			if(frameIdx>0 && i > 3 && i < iWidth - 3 && j > 3 && j < iHeight - 3)
			{
#ifdef HW_JND
				int32_t fidl = CalFidl_Fixed(i, j, CurrFrame, PrevFrame, iWidth, iHeight, iWidth);
#else
				double fidl = CalFidl(i, j, CurrFrame, PrevFrame, iWidth, iHeight, iWidth);
#endif
				CurrJND[j][i] = fidl*CurrJND[j][i] / Q16_MULT;
				sum_jnd += CurrJND[j][i];
			}
		}
	}
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

typedef int32_t q16_16_t;
#define FLOAT_TO_Q16_16(x) ((q16_16_t)((x) * 65536.0f)) //For Debug
#define Q16_16_TO_FLOAT(x) ((float)(x) / 65536.0f) //For Debug
typedef enum {
    SEGMENT_0,  //[0.0, 0.25)
    SEGMENT_1,  //[0.25, 0.5)
    SEGMENT_2,  //[0.5, 0.75)
    SEGMENT_3   //[0.75, 1.0)
} Segment;

const q16_16_t a_coeffs[4] = {
    FLOAT_TO_Q16_16(-0.115f),   // SEGMENT_0
    FLOAT_TO_Q16_16(-0.302f),   // SEGMENT_1
    FLOAT_TO_Q16_16(-0.501f),   // SEGMENT_2
    FLOAT_TO_Q16_16(-0.723f)    // SEGMENT_3
};

const q16_16_t b_coeffs[4] = {
    FLOAT_TO_Q16_16(1.442f),    // SEGMENT_0
    FLOAT_TO_Q16_16(1.396f),    // SEGMENT_1
    FLOAT_TO_Q16_16(1.321f),    // SEGMENT_2
    FLOAT_TO_Q16_16(1.211f)     // SEGMENT_3
};

int find_msb(uint32_t x) {
    if (x == 0) return -1;
    int k = 0;
    while (x >>= 1) k++;
    return k;
}
q16_16_t compute_log2_fixed(uint32_t dCUAct) {
    int k = find_msb(dCUAct);
    uint32_t normalized = dCUAct << (31 - k);
    q16_16_t f = (normalized >> 15) & 0xFFFF;

    Segment seg;
    if (f < 0x4000) {         // 0.25 in Q16.16: 0.25 * 65536 = 16384 (0x4000)
        seg = SEGMENT_0;
    } else if (f < 0x8000) {  // 0.5 in Q16.16: 32768 (0x8000)
        seg = SEGMENT_1;
    } else if (f < 0xC000) {  // 0.75 in Q16.16: 49152 (0xC000)
        seg = SEGMENT_2;
    } else {
        seg = SEGMENT_3;
    }

    q16_16_t a = a_coeffs[seg];
    q16_16_t b = b_coeffs[seg];

    q16_16_t f_sq = (q16_16_t)(((int64_t)f * f) >> 16);
    // 锟斤拷锟斤拷 a*f2 + b*f
    q16_16_t term_a = (q16_16_t)(((int64_t)a * f_sq) >> 16);
    q16_16_t term_b = (q16_16_t)(((int64_t)b * f) >> 16);
    q16_16_t log_f = term_a + term_b;

    return ((k - 12) << 16) + log_f;
}


extern void setQpMapBlk16(int x, int y, int map);
Void CalculateJNDQP(int iWidth, int iHeight, int frame_Idx, int SliceQP)
{

	double dJndActivity, dVarianceJnd, dAverageJnd;
	double dSumJNDAct = 0.0;

	double dSumAct = 0.0;

	Int uiAQPartWidth = 1 << 4;
	Int uiAQPartHeight = 1 << 4;
	int LCU_NUM = 0;
	Int x, y;
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

			for (int i = 0; i < 1; i++)
			{
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
			}

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
			double weight = g_algo_cfg.JNDweight;
			double qp = weight *(deltaQP_MB[i*mb_width_num + j] - deltaQP_frame);

			qp = (floor(qp + 0.49999));

			int delta_qp_bound = 54 - SliceQP;
			qp  = max((-1)*delta_qp_bound, min((int)qp, delta_qp_bound));
			qp  = max(-5, min((int)qp, 5));

			int map = (((0 & 0x1) << 7) | ((int)qp & 0x7f));
			setQpMapBlk16(j<<4, i<<4, map);
			avgQP += qp;

		}

	}
	avgQP = avgQP / (mb_width_num*mb_height_num);
	free(AQ_ArryCUJND16);
}
#endif

Void TEncPreanalyzer::xPreanalyze( TEncPic* pcEPic )
{
  TComPicYuv* pcPicYuv = pcEPic->getPicYuvOrg();
  const Int iWidth = pcPicYuv->getWidth(COMPONENT_Y);
  const Int iHeight = pcPicYuv->getHeight(COMPONENT_Y);
  const Int iStride = pcPicYuv->getStride(COMPONENT_Y);

  for ( UInt d = 0; d < pcEPic->getMaxAQDepth(); d++ )
  {
    const Pel* pLineY = pcPicYuv->getAddr(COMPONENT_Y);
    TEncPicQPAdaptationLayer* pcAQLayer = pcEPic->getAQLayer(d);
    const UInt uiAQPartWidth = pcAQLayer->getAQPartWidth();
    const UInt uiAQPartHeight = pcAQLayer->getAQPartHeight();
    TEncQPAdaptationUnit* pcAQU = pcAQLayer->getQPAdaptationUnit();

    Double dSumAct = 0.0;
    for ( UInt y = 0; y < iHeight; y += uiAQPartHeight )
    {
      const UInt uiCurrAQPartHeight = min(uiAQPartHeight, iHeight-y);
      for ( UInt x = 0; x < iWidth; x += uiAQPartWidth, pcAQU++ )
      {
        const UInt uiCurrAQPartWidth = min(uiAQPartWidth, iWidth-x);
        const Pel* pBlkY = &pLineY[x];
        UInt64 uiSum[4] = {0, 0, 0, 0};
        UInt64 uiSumSq[4] = {0, 0, 0, 0};
        UInt by = 0;
        for ( ; by < uiCurrAQPartHeight>>1; by++ )
        {
          UInt bx = 0;
          for ( ; bx < uiCurrAQPartWidth>>1; bx++ )
          {
            uiSum  [0] += pBlkY[bx];
            uiSumSq[0] += pBlkY[bx] * pBlkY[bx];
          }
          for ( ; bx < uiCurrAQPartWidth; bx++ )
          {
            uiSum  [1] += pBlkY[bx];
            uiSumSq[1] += pBlkY[bx] * pBlkY[bx];
          }
          pBlkY += iStride;
        }
        for ( ; by < uiCurrAQPartHeight; by++ )
        {
          UInt bx = 0;
          for ( ; bx < uiCurrAQPartWidth>>1; bx++ )
          {
            uiSum  [2] += pBlkY[bx];
            uiSumSq[2] += pBlkY[bx] * pBlkY[bx];
          }
          for ( ; bx < uiCurrAQPartWidth; bx++ )
          {
            uiSum  [3] += pBlkY[bx];
            uiSumSq[3] += pBlkY[bx] * pBlkY[bx];
          }
          pBlkY += iStride;
        }

        assert ((uiCurrAQPartWidth&1)==0);
        assert ((uiCurrAQPartHeight&1)==0);
        const UInt pixelWidthOfQuadrants  = uiCurrAQPartWidth >>1;
        const UInt pixelHeightOfQuadrants = uiCurrAQPartHeight>>1;
        const UInt numPixInAQPart         = pixelWidthOfQuadrants * pixelHeightOfQuadrants;

        Double dMinVar = DBL_MAX;
        if (numPixInAQPart!=0)
        {
          for ( Int i=0; i<4; i++)
          {
            const Double dAverage = Double(uiSum[i]) / numPixInAQPart;
            const Double dVariance = Double(uiSumSq[i]) / numPixInAQPart - dAverage * dAverage;
            dMinVar = min(dMinVar, dVariance);
          }
        }
        else
        {
          dMinVar = 0.0;
        }
        const Double dActivity = 1.0 + dMinVar;
        pcAQU->setActivity( dActivity );
        dSumAct += dActivity;
      }
      pLineY += iStride * uiCurrAQPartHeight;
    }

    const Double dAvgAct = dSumAct / (pcAQLayer->getNumAQPartInWidth() * pcAQLayer->getNumAQPartInHeight());
    pcAQLayer->setAvgActivity( dAvgAct );
  }
}
//! \}

