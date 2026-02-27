#ifndef __CVI_MOTION__
#define __CVI_MOTION__
#include <assert.h>
#include "cvi_cu_ctrl.h"
#include "CommonDef.h"
using namespace std;

#define MOT_STABLE_PIC_CNT      10
#define MV_SAD_GAIN_FRAC_BIT    4
#define MV_SAD_GAIN_MAX         0x3f

// cfg paramters
extern double g_mvGain;
extern double g_motAlpha;
extern double g_sadGain;

extern void *g_pPrevPicYuvRec;

extern cvi_grid_data<UChar> g_alpha_map;
extern cvi_grid_data<UChar> g_motion_map[3];
extern int g_pic_motion_level;

// Foreground Detect
extern cvi_grid_data<double> g_foreground_map[3];
extern int g_pic_foreground_cnt;

#ifdef CVI_FOREGROUND_QP
extern cvi_grid_data<UChar> g_fg_dqp_map;
extern bool g_is_foreground_qp_valid;
void set_foreground_qp_valid(bool is_valid);
bool get_foreground_qp_valid();
#endif

int get_fix_point_mv_gain();
int get_fix_point_sad_gain();
int get_fix_point_fg_thr_gain();
int get_fix_point_fg_thr_bias();

void motion_detect_by_mv_and_sad(int cu_x, int cu_y, int sad, short mv_x[], short mv_y[], int mv_num, int size, bool is_first_pic);

void motion_fuse_with_cur_blk_diff(Pel* pPrevPtr, Int iWidth, Int iHeight, Int iPrevStride);

void temporal_filter_with_ref(Pel* pSrcBase, Pel* pPrevBase, Int  iWidth, Int  iHeight, Int iSrcStride, Int iPrevStride, SliceType sliceType);

void calc_pic_motion_level(int picWidth, int picHeight);

void calcForegroundMap(int cu_x, int cu_y, int cu_width);
#endif /* __CVI_MOTION__ */