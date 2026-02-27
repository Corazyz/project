
#ifndef __CVI_ENC__
#define __CVI_ENC__

#include <iostream>

#include "CommonDef.h"
#include "TComPic.h"

using namespace std;

#define CVI_ENC_SETTING

typedef struct _cvi_enc_setting_st {
  int last_avg_mv_x;
  int last_avg_mv_y;

  int last_max_mv_x;
  int last_min_mv_x;
  int last_max_mv_y;
  int last_min_mv_y;

  bool enableCviRGBChange;
  bool enableCviRandomEnc;
  int RandomGroup;
  //ENC constrained MVD enable for IME to reduce candidates
  bool enableConstrainedMVD;
  int mvd_thr_x;
  int mvd_thr_y;

  //ENC constrained MV enable for IME/FME
  bool enableConstrainedMV;
  int mv_x_lbnd;
  int mv_x_ubnd;
  int mv_y_lbnd;
  int mv_y_ubnd;

  //ENC Constrained MV cand for Merge mode
  bool enableConstrainedMergeCand;
  int merge_mv_thr_x;
  int merge_mv_thr_y;
  unsigned int merge_cnt;
  unsigned int merge_2_cand_cnt;
  unsigned int merge_reduce_cnt;
} cvi_enc_setting_st;

extern cvi_enc_setting_st g_cvi_enc_setting;

void cvi_init_enc_config();
void cvi_enc_config(const TComPic* pcPic);
void cvi_enc_update_config(const TComPic* pcPic);
void cvi_random_frame_enc_config(const TComPic* pcPic);
#endif /* __CVI_ENC__ */
