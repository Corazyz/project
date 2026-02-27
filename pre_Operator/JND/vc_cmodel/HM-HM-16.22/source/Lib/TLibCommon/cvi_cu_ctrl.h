#ifndef __CVI_CU_CTRL__
#define __CVI_CU_CTRL__
#include <assert.h>
#include "memory.h"
#include "CommonDef.h"
#include "cvi_sigdump.h"
using namespace std;

/*
#define CU32_LAYER_TIME_UNIT 16
#define CU16_LAYER_TS_UNIT 8
#define CU8_LAYER_TS_UNIT 1
#define CU32_SUBCU_CHK_NUM 2
#define CU16_SUBCU_CHK_NUM 3
*/

#define CU32_LAYER_TIME_UNIT 40
#define CU16_LAYER_TS_UNIT 20
#define CU8_LAYER_TS_UNIT 3
#define CU32_SUBCU_CHK_NUM 2
#define CU16_SUBCU_CHK_NUM 2

#define MV_RELIABLE_THR 4 // in quadter pixels
#define MADI_BIN_PREC 6
#define MADI_HIST_PREC 5

enum ForceDecision
{
  FORCE_BYPASS = 0,
  FORCE_KEEP = 1,
  FORCE_CHANGE = 2
};

template <typename T>
class cvi_grid_data
{
private:
  T* m_data;
  int m_grid_size_shift;
  int m_horz_grid_num;
  int m_vert_grid_num;
  int m_data_num;
public:
  void create(int width, int height, int log2_grid_size) {
    m_grid_size_shift = log2_grid_size;
    m_horz_grid_num = (width+(1<<m_grid_size_shift)-1) >> m_grid_size_shift;
    m_vert_grid_num = (height+(1<<m_grid_size_shift)-1) >> m_grid_size_shift;
    m_data_num = m_horz_grid_num * m_vert_grid_num;
    m_data = new T[m_data_num]();
    memset(m_data, 0, sizeof(T) * m_data_num);
  };
  int get_grid_addr(int x, int y) {
    int grid_x = Clip3(0, m_horz_grid_num-1, x>>m_grid_size_shift);
    int grid_y = Clip3(0, m_vert_grid_num-1, y>>m_grid_size_shift);
    return (grid_y*m_horz_grid_num + grid_x);
  };
  void destroy() { delete(m_data); }
  int get_grid_size_shift() {return m_grid_size_shift; };
  void set_data(int x, int y, T val) {
    m_data[get_grid_addr(x, y)] = val;
  };
  void set_data(int index, T val) {
    index = Clip3(0, m_data_num - 1, index);
    m_data[index] = val;
  };
  T get_data(int x, int y) {
    return m_data[get_grid_addr(x, y)];
  };
  T get_data(int index) {
    index = Clip3(0, m_data_num - 1, index);
    return m_data[index];
  };
  int get_size() { return m_data_num; };
  void set_all(unsigned char value) { memset(m_data, value, sizeof(T) * m_data_num); };

  /*
  typedef void (*FunX)(void *);
  typedef void (*FunY)();
  void traverse_grid(int x, int y, int width, int height, FunX callbackX, FunY callbackY) {
    int grid_size = 1<<m_grid_size_shift;
    for(int grid_y=0; grid_y<height; grid_y+=grid_size) {
      for(int grid_x=0; grid_x<width; grid_x+=grid_size) {
        int data = get_data<int>(grid_x, grid_y);
        callbackX((void*)&(data));
      }
      callbackY();
    }
  }; */
};

// only support 64x64 CTU case
template <typename T>
class cvi_z_order_data
{
private:
  T** m_data;
  int m_depth;
  int m_baseNumPart;
public:
  void create(int base_depth_size) {
    m_depth = (base_depth_size==64)
            ? 4
            : (base_depth_size==16)
              ? 2
              : (base_depth_size==8)
                ? 1 : 3;
    m_data = new T*[m_depth];
    int num = 1;
    for(int d=0; d<m_depth; d++) {
      m_data[d] = new T[num]();
      num *= 4; // quadtree expansion
    };
    m_baseNumPart = num;
  }
  void destroy() { delete(m_data);}
  int get_z_addr(int zOrder, int depth) {
    int totalNumPart = m_baseNumPart>>(depth<<1);
    return (zOrder % m_baseNumPart) / totalNumPart;
  };

  T get_data(int zOrder, int depth) {
    int innerDepth = depth - (4 - m_depth);
    assert(innerDepth>=0);
    int addr = get_z_addr(zOrder, innerDepth);
    return m_data[innerDepth][addr];
  };
  void set_data(int zOrder, int depth, T val) {
    int innerDepth = depth - (4 - m_depth);
    assert(innerDepth>=0);
    int addr = get_z_addr(zOrder, innerDepth);
    m_data[innerDepth][addr] = val;
  };
};

extern int g_historyCu32Cnt;
extern int g_historyTermCU16Cnt;
extern int g_historyTermCU8Cnt;

extern int g_historyTsUnitCnt;
extern int g_historyCUTimeUnit;
extern int g_picTsUnitCnt;

extern int g_picTermCU16Cnt;
extern int g_picTermCU8Cnt;
extern int g_picCU32Cnt;
extern int g_picCU32Num;

extern int g_cuTermCU16Cnt;
extern int g_cuTermCU8Cnt;
extern bool g_is_urgent;
extern int g_smooth_cu_thr;

extern cvi_z_order_data<bool> g_isZeroResiMerge;
extern cvi_z_order_data<double> g_cu_rdcost;

extern bool g_lastFrameIsI;
extern bool g_rd_buf_idx, g_wrt_buf_idx;

extern int g_picMaxTargetTerm;
extern int g_picMinTargetTerm;

extern cvi_grid_data<int> g_cu_depth_info[2];

extern bool g_unreliable_cu;

extern cvi_grid_data<int> g_blk8_madi;
extern cvi_grid_data<int> g_blk8_lum;
extern int g_madi8_hist[1<<MADI_BIN_PREC], g_madi8_hist_delay[1<<MADI_BIN_PREC];
extern int g_madi16_hist[1 << MADI_HIST_PREC], g_madi16_hist_delay[1 << MADI_HIST_PREC];
extern int g_madi32_hist[1 << MADI_HIST_PREC], g_madi32_hist_delay[1 << MADI_HIST_PREC];
extern int g_i4_madi_thr;
extern int g_historyI4Cnt;
extern int g_historyTermI4Cnt;
extern int g_picTermI4Cnt;
extern int g_picTargetI4Term;

extern cvi_grid_data<int> g_blk_madi, g_blk_lum;
extern int g_pic_tc_accum, g_pic_madp_accum;
extern cvi_grid_data<int> g_blk_madp;

void setupFastCUEncEnv(int frame_width, int frame_height);
void ccu_calc_i4_term_madi_thr(double termRatio, int total_cnt, bool is_first_frame);
bool ccu_i4_term_decision(int pos_x, int pox_y);
void ccu_registerTermStats(int cu_size, int term_en);
void ccu_updateTermStats();
void ccu_updateCUStatus(int cu_x, int cu_y, int width, int depth);
void ccu_frame_init(bool isIframe, int poc, double targetTerm, int frame_width, int frame_height);

int ccu_get_i4_madi_thr();
int ccu_get_pic_target_i4_term();
UInt64 ccu_get_max_rd_cost();

#ifdef CVI_CU_PENALTY
typedef enum _CU_PENALTY_MODE_
{
  CU_PENALTY_INTRA = 0,
  CU_PENALTY_INTER,
  CU_PENALTY_SKIP,
  CU_PENALTY_MODE_TOTAL
} CU_PENALTY_MODE;

#define CVI_HW_COST_WT_DEFAULT  0x10
#define CVI_PEN_COST_FRAC_BIT   4
#define CVI_PEN_COST_MAX        0x3f
#define CVI_PEN_COST_MIN        1
#define CVI_PEN_COST_BIAS_BIT   15
#define CVI_PEN_COST_BIAS_MAX   ((1 << CVI_PEN_COST_BIAS_BIT) - 1)
#define CVI_PEN_COST_BIAS_MIN   (-(1 << CVI_PEN_COST_BIAS_BIT))
#define CVI_RDCOST_BIT          33  // 31.2
#define CVI_RDCOST_MAX          ((__UINT64_C(1) << CVI_RDCOST_BIT) - 1)

extern int g_init_cost_wt[3][3][3];  // [background/foreground/cu_weight][CU32/16/8][intra/inter/skip]
extern int g_init_cost_bi[3][3][3];  // [background/foreground/cu_weight][CU32/16/8][intra/inter/skip]

extern int *gp_hw_cost[2][3];
extern int *gp_hw_bias[2][3];

void cvi_init_hw_cost_weight();
void cvi_random_hw_cost_weight();
void cvi_set_pic_cost_weight(bool is_i_slice, bool is_fg_en);
void calcCuPenaltyCost(CU_PENALTY_MODE mode, double &cost, int cu_size, int cu_x, int cu_y, bool is_i_slice);

#ifdef CVI_SPLIT_COST_WEIGHT
void calcCuPenaltyCostSplit(CU_PENALTY_MODE mode, double &cost, UInt bits, Distortion dist,
                            UInt64 lambda, int cu_width, int cu_x, int cu_y, bool is_i_slice);
#endif

#endif //~CVI_CU_PENALTY

#ifdef CVI_FOREGROUND_QP
void calcRcForegroundMap(int cu_x, int cu_y, int cu_width);
int calcForegroundQpDelta(int cu_x, int cu_y, int cu_width);
#endif

#ifdef CVI_FAST_CU_ENC_BY_COST

#define CU_COST_HIST_DEPTH  6
#define CU_COST_HIST_BIN    (1 << CU_COST_HIST_DEPTH)

#define FAST_ENC_CU32_TIME        40
#define FAST_ENC_CU32_TIME_SAVE   20
#define FAST_ENC_CU16_TIME_SAVE   3

#define FAST_ENC_THR_AMP_DEPTH    5
#define FAST_ENC_THR_AMP_DEFAULT  32  // (1 << FAST_ENC_THR_AMP_DEPTH)
#define FAST_ENC_THR_AMP_MIN      35
#define FAST_ENC_THR_AMP_MAX      39

enum FastEncMode
{
  FAST_ENC_I = 0,
  FAST_ENC_P_TC,
  FAST_ENC_P_COST,
  FAST_ENC_Total_Mode
};

enum FastEncCuIdx
{
  FE_CU8 = 0, // not used
  FE_CU16,
  FE_CU32,
  FE_CU_IDX_TOTAL
};

enum FastEncLevel
{
  FE_LEVEL_0 = 0, // not used
  FE_LEVEL_1,
  FE_LEVEL_2,
  FE_LEVEL_TOTAL
};

typedef struct _FAST_ENC_PARAM_
{
  int total_cu32_num;
  int total_cu16_num;
  FastEncMode fast_enc_mode;
  int cu_cost_target_ratio[FE_CU_IDX_TOTAL];

  int enc_cu32_cnt;
  int cu32_save_cnt;
  int cu16_save_cnt;
  int total_time_unit;
  int target_save_unit;
  int current_save_unit;

  int cu32_time;
  int cu32_time_save;
  int cu16_time_save;

  int thr_amplifier;
  FastEncLevel time_save_level;

  cvi_grid_data<UInt64> blk16_cost;
  cvi_grid_data<UInt64> blk32_cost;
  UInt64 cu_cost_accum[FE_CU_IDX_TOTAL];
  UInt64 cu_cost_avg[FE_CU_IDX_TOTAL];
  int cu_cost_scale_log2[FE_CU_IDX_TOTAL];
  int cu_cost_hist[FE_CU_IDX_TOTAL][CU_COST_HIST_BIN];
  UInt64 cu_cost_thr[FE_CU_IDX_TOTAL];
  int cu_tc_thr[FE_CU_IDX_TOTAL];
} FAST_ENC_PARAM;

extern FAST_ENC_PARAM *gp_fe_param;
extern int g_cu16_save_map[2][2];

void frm_init_fast_cu_enc_by_cost(int width, int height, int slice_type);
void store_current_cu_cost(int x, int y, int size, double cost);
void frm_update_fast_cu_enc_by_cost(int width, int height);
void update_fast_enc_status();
UInt64 fast_enc_thr_amplifier(UInt64 threshold);
#endif //~CVI_FAST_CU_ENC_BY_COST


////////////////////////////////////////////////////////////////////////////
// AI-ISP Smart Encode
////////////////////////////////////////////////////////////////////////////

#ifdef CVI_SMART_ENC

#define SCENE_IDX_TOTAL         8
#define AI_SI_TAB_COUNT         8
#define AI_SI_TAB_BIN           16
#define AI_DELTA_QP_MIN         (-51)
#define AI_DELTA_QP_MAX         51
#define AI_CONF_NORM_SCALE_BD   8
#define AI_INVALID_IDX          (-1)

extern int g_smart_enc_ai_table[AI_SI_TAB_COUNT][AI_SI_TAB_BIN];
extern string g_ai_pattern_path;
extern string g_ai_scene_pattern_name;
extern int g_ai_table_idx_min;
extern int g_ai_table_idx_max;
extern int g_ai_conf_scale;
extern int g_ai_blk_size;

extern string g_isp_pattern_path;
extern int g_isp_motion_qp;
extern int g_isp_trans_qp;
extern int g_clip_delta_qp_min;
extern int g_clip_delta_qp_max;

typedef struct _AI_SEG_BLK_
{
  int idx;
  int level;
} AI_SEG_BLK;

typedef struct _AI_MAP_TAB_
{
  int idx;
  string label_s;
  int table_idx;
} AI_MAP_TAB;

typedef struct _AI_ENC_PARAM_
{
  sig_ctx scene_ctx;
  sig_ctx object_ctx;
  sig_ctx mapping_table_ctx;
  // Scene
  int src_width;
  int src_height;
  int frame_idx;
  int current_scene_idx;
  int current_scene_level;
  int scene_hist[SCENE_IDX_TOTAL];
  int score_accum[SCENE_IDX_TOTAL];
  //int scene_change_count;
  //std::vector<int> scene_change_pos;

  // Segmentation
  std::vector<AI_MAP_TAB> mapping_table;
  cvi_grid_data<AI_SEG_BLK> seg_map;
  int frame_il_bd;
  int roi_delta_qp;
} AI_ENC_PARAM;

extern AI_ENC_PARAM *gp_ai_enc_param;

typedef struct _ISP_ENC_PARAM_
{
  sig_ctx motion_ctx;
  sig_ctx edge0_ctx;
  sig_ctx edge45_ctx;
  sig_ctx edge90_ctx;
  sig_ctx edge135_ctx;
  sig_ctx madi_ctx;
  int src_width;
  int src_height;
  int skip_bits;
  int nonskip_bits;
  int is_entropy;
  int map_dqp_accum;
  int preP_QP;
  int curI_QP;
  int slice_type;
  int intra_period;
} ISP_ENC_PARAM;

extern ISP_ENC_PARAM *gp_isp_enc_param;

void enable_smart_enc_sequence(int src_width, int src_height);
void close_smart_enc_sequence();
void process_smart_enc_frame(int frame_idx, int frame_width, int frame_height);
void frame_read_ai_scene(int frame_idx);
void frame_read_ai_info(int frame_idx);
void calc_smart_enc_qp_map();
void frame_read_isp_info(int frame_idx);
void frame_clip_slice_qp_bound(int frame_width, int frame_height);
#endif //~CVI_SMART_ENC

#ifdef USE_JND_MODEL
void xPreanalyzeFrameJND(Pel* pY, int iWidth, int iHeight, int iStride, Int frameIdx, Int FrameLeft);
Void CalculateJNDQP(int iWidth, int iHeight, int frame_Idx, int SliceQP);
#endif

#endif /* __CVI_CU_CTRL__ */
