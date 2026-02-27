#ifndef __CVI_RATE_CTRL__
#define __CVI_RATE_CTRL__
using namespace std;
#include "CommonDef.h"
#include "TLibCommon/TComDataCU.h"
#include "cvi_cu_ctrl.h"
#include <stdlib.h>

#define QP_NUM 52
#define ROW_Q_LUT_SIZE 9
#define TC_LUT_SIZE 16
#define LUM_LUT_SIZE 5
#define RC_NORM_SCALE_BD 8
#define MAX_CTU_ROW_BIT ((1<<21)-1)
#define MAX_CTU_ROW_BIT_ACCUM ((1<<22)-1)
#define MAX_CTU_CODED_BIT ((1<<16)-1)
#define MAX_CTU_BIT ((1<<15)-1)
#define MIN_BIT_ERR (-(1<<23))
#define MAX_BIT_ERR ((1<<23)-1)
#define ROW_AVG_BIT_BD 20
#define ROW_Q_LUT_IN_BD 15
#define MAX_BLK_DQP_ACCUM ((1<<23)-1)
#define MIN_BLK_DQP_ACCUM (-(1<<23))
#define ROW_ACCUM_BIT_DELAY_CTU 1
#define RC_DEBUG 1

#define HW_CABAC_FIRST_BIT          1
#define HW_CABAC_SLICE_FIN_BIT      10

enum qp_mode {
    MAX = 0,
    MIN = 1,
    AVG = 2
};

extern int g_RCDebugLogEn;
extern int g_StatTime;
extern int g_RCMaxIprop, g_RCMinIprop;
extern int g_RCMaxQp, g_RCMinQp;
extern int g_RCMaxIQp, g_RCMinIQp;
extern int g_CtuRowNormScale;
extern int g_TcQpTuneEn, g_LumQpTuneEn;
extern int g_qp_mode;
extern bool g_random_rc;

extern int g_RCStatsBeginPicIdx;
extern int g_RCStatsIPicCnt, g_RCStatsPPicCnt;
extern double g_RCStatsIPicBitErrAccum;
extern double g_RCStatsPPicBitErrAccum;
extern double g_RCStatsIPicBitErrMax;
extern double g_RCPStatsPicBitErrMax;

extern bool g_RCFastConvergeEn;
extern int g_RCMdlUpdatType;
extern int g_RowQpClip, g_InRowOverflowQpdelta;
extern RC_Float g_picAvgTc_delay;

extern int g_bit_error_accum, g_bit_err_smooth_factor, g_bit_err_compen_scal;

extern cvi_grid_data<int> g_blk_qp, g_cu32_qp;
extern cvi_grid_data<double> g_cu32_lambda, g_cu32_sqrt_lambda;
extern int g_TcThrd[TC_LUT_SIZE], g_LumThrd[LUM_LUT_SIZE], g_LumQpDelta[LUM_LUT_SIZE];
extern int g_directionThrd;
extern int g_blk_delta_qp_accum;

#ifdef CVI_FOREGROUND_QP
extern int g_FgTcThrd[TC_LUT_SIZE];
#endif

enum lut_mode {
 NEAREST = 1,
 INTERVAL = 2,
 LUT_MODE_NUM = 3
};

class hw_lut
{
private:
 int m_entry_num;
 lut_mode m_mode;
 int* m_input;
 int* m_output;
 int m_center_idx;
public:

 void lut_init(int entry_num, lut_mode mode);
 int lut_lookup(int target);
 int get_entry_num() { return m_entry_num;};
 int get_center_idx() { return m_center_idx;};
 void set_input_entry(int idx, int val) { m_input[idx] = val;};
 void set_output_entry(int idx, int val) { m_output[idx] = val;};
 int get_input_entry(int idx) { return m_input[idx];};
 int get_output_entry(int idx) { return m_output[idx];};
 int* get_input_lut_ptr() { return m_input;};
 int* get_output_lut_ptr() { return m_output;};
};

extern hw_lut g_row_q_lut, g_tc_lut, g_lum_lut;
extern double g_qp_to_lambda_table[QP_NUM], g_qp_to_sqrt_lambda_table[QP_NUM];
extern bool g_updata_stat_frm_en;
extern int g_qp_hist[QP_NUM];
extern int g_stat_frm_max_qp;
extern int g_stat_frm_min_qp;
extern int g_stat_frm_qp_sum;

#define STAT_CU_IDX_TOTAL   19
extern const int STAT_CU_IDX[3][4]; // [Intra/Inter/Skip][CU4/8/16/32]
extern int g_stat_cu_count[STAT_CU_IDX_TOTAL];

#ifdef CVI_FOREGROUND_QP
extern hw_lut g_fg_tc_lut;
#endif
#ifdef CVI_RANDOM_ENCODE
extern bool g_is_cvi_rand_rc_en;
extern int g_tc_qp_delta_rand[TC_LUT_SIZE+1];
extern double g_qp_to_lambda_table_rand[QP_NUM];
extern double g_qp_to_sqrt_lambda_table_rand[QP_NUM];
#endif

void cal8x8MadiAndLum(Short *p_srcBase, int stride, int *p_iMad, int *p_pLum);
void rc_setPicAvgTc(int picTcAccum, int picPelNum);
void rc_update_pic_bit_stats(int targetBit, int actBit, int picIdx, int picType);
void rc_show_pic_bit_stats();
void update_frm_statistic(TComDataCU *pcCU, int uiAbsPartIdx, int cu_x, int cu_y, int cu_w, bool is_rc_en);
void reset_frm_statistic();

#ifdef CVI_QP_MAP

#define QP_MAP_MODE_DRAM    1
#define QP_MAP_MODE_ROI     2
#define QP_ROI_LIST_SIZE    8

typedef enum _QP_MAP_DELTA_MODE_
{
    QP_MAP_RELATIVE = 0,
    QP_MAP_ABSOLUTE
} QP_MAP_DELTA_MODE;

typedef struct _QP_ROI_
{
    bool is_enable;
    int x;
    int y;
    int width;
    int height;
    int skip;
    QP_MAP_DELTA_MODE mode;
    int qp;
} QP_ROI;

extern int g_QpMapEn;
extern std::string g_QpMapFileName;
extern cvi_grid_data<int> g_blk16_qp_map;
extern QP_ROI g_qp_roi_cfg[QP_ROI_LIST_SIZE];
bool isQpMapEnable();
int getQpMapMode();
int packQpMapByte(int skip, int mode, int qp);
void unpackQpMapByte(int map, int *p_skip, QP_MAP_DELTA_MODE *p_mode, int *p_qp);
void unpackQpMapByteAI(int map, int *p_skip, int *p_qp);
bool parseQpRoiCfg(int frame_width, int frame_height);
void setFrameQpMap(int frame_width, int frame_height);
extern void clearQpMapBoundarySkip(int frame_width, int frame_height);
#ifdef CVI_RANDOM_ENCODE
void setRandomFrameQpMap(int frame_width, int frame_height);
void setRandomQpMapCfg();
#endif //~CVI_RANDOM_ENCODE
void getQpMapBlk16(int x, int y, int *p_skip, QP_MAP_DELTA_MODE *p_mode, int *p_qp);
ForceDecision checkQpMapCuDecision(int cu_x, int cu_y, int cu_width);
#endif //~CVI_QP_MAP

int getRcMaxQp(SliceType slice_type);
int getRcMinQp(SliceType slice_type);
int getCtuRowNormScale();
int getBitErrCompenScal();
int getRowQpClip();
int getInRowOverflowQpdelta();
int getQpMode();
unsigned char getQpWinEn();
unsigned char getQpWinCount();
#endif /* __CVI_RATE_CTRL__ */
