
#ifndef __CVI_IME__
#define __CVI_IME__

#include <iostream>

using namespace std;

const int cvi_sr[] = {16, 16, 16, 16};


typedef struct _cvi_var_st {
  unsigned int ime_mvp_list_refidx[5];
  short ime_mvp_mvx[2][2]; //blk 16x16 mvx [list][ref_idx]
  short ime_mvp_mvy[2][2]; //blk 16x16 mvy [list][ref_idx]
} cvi_var_st;

typedef struct _cvi_mv_st {
  short m_iHor;     ///< horizontal component of motion vector
  short m_iVer;     ///< vertical component of motion vector
  int m_irefPoc;
  int m_irefIdx;
} cvi_mv_st;

typedef struct _cvi_16x16_mv_st {
  //var for cvi encoder
  cvi_mv_st cMvPred16x16;
  cvi_mv_st cMvPred8x8[2][2]; // four 8x8 mv in 16x16
} cvi_16x16_mv_st;

typedef struct _cvi_ctb_mv_st {
  cvi_16x16_mv_st **cvi_16x16_mv;
} cvi_ctb_mv_st;

typedef struct _cvi_frm_mv_array_st {
  cvi_ctb_mv_st *cvi_ctb_mv;
} cvi_frm_mv_array_st;

extern cvi_frm_mv_array_st cvi_frm_mv_array[2];
extern cvi_var_st g_cvi_var;

bool is_8x8_store_frm_mv(int width, int height, int cu_x, int cu_y);
void init_frm_mv_array(int ctb_cnt_x, int ctb_cnt_y, int ctb_size);
void deinit_frm_mv_array(int ctb_cnt_x, int ctb_cnt_y);
void store_frm_mv(bool is16x16, int width, int height, int ctb_cnt, int ctb_size, int cu_x, int cu_y, short Hor, short Ver, int refPoc, int refIdx, int list);
void get_frm_mv(int list, bool is16x16, int ctb_cnt, int index_x, int index_y, short &Hor, short &Ver, int &refPoc, int &refIdx);
void get_real_mv(int index_x, int index_y, short &Hor, short &Ver, int &refPoc, int &refIdx);
bool is_use_8x8_ime_mvp(int x, int y, int w, int h);

#endif /* __CVI_IME__ */
