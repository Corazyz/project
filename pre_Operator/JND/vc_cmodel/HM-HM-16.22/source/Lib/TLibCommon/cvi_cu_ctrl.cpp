#include <stdlib.h>
#include <math.h>
#include "cvi_cu_ctrl.h"
#include "cvi_motion.h"
#include "cvi_rate_ctrl.h"
#include "cvi_sigdump.h"
#include "cvi_pattern.h"


int g_historyTsUnitCnt;
int g_historyCUTimeUnit;

// keep accumulating during the whole sequence
int g_historyCu32Cnt = 0;
int g_historyTermCU16Cnt = 0;
int g_historyTermCU8Cnt = 0;

// accumulate during one frame encoding
int g_picTsUnitCnt;

int g_picTermCU16Cnt;
int g_picTermCU8Cnt;

int g_picCU32Cnt;
int g_picCU32Num;

// accumulate in one CU32
int g_cuTermCU16Cnt = 0;
int g_cuTermCU8Cnt = 0;

int g_picMaxTargetTerm;
int g_picMinTargetTerm;
bool g_lastFrameIsI = true;
bool g_is_urgent;
int g_smooth_cu_thr = 8;

cvi_z_order_data<bool> g_isZeroResiMerge;

bool g_rd_buf_idx = 0, g_wrt_buf_idx = 0;

cvi_grid_data<int> g_cu_depth_info[2]; //16x16 grid read/write double buffer

bool g_unreliable_cu = false; //[cu32/cu16/cu8][cu_idx]

cvi_z_order_data<double> g_cu_rdcost;
// I4 termination
int g_madi8_hist[1<<MADI_BIN_PREC], g_madi8_hist_delay[1<<MADI_BIN_PREC]; // 256/4
int g_madi16_hist[1 << MADI_HIST_PREC], g_madi16_hist_delay[1 << MADI_HIST_PREC]; // 128 >> 2
int g_madi32_hist[1 << MADI_HIST_PREC], g_madi32_hist_delay[1 << MADI_HIST_PREC]; // 128 >> 2
cvi_grid_data<int> g_blk8_madi;
cvi_grid_data<int> g_blk8_lum;
int g_historyI4Cnt = 0;
int g_historyTermI4Cnt = 0;
int g_picTermI4Cnt = 0;
int g_picTargetI4Term = 0;
int g_i4_madi_thr = 8;

int g_pic_tc_accum, g_pic_madp_accum;
cvi_grid_data<int> g_blk_madi, g_blk_lum;
cvi_grid_data<int> g_blk_madp;

void setupFastCUEncEnv(int frame_width, int frame_height)
{
  for(Int idx=0; idx<2; idx++) {
    g_cu_depth_info[idx].create(frame_width, frame_height, 4);
  }
  g_isZeroResiMerge.create(32);
  g_cu_rdcost.create(32);
  // for I4 terminaiton
  int width_align32 = cvi_mem_align(frame_width, 32);
  int height_align32 = cvi_mem_align(frame_height, 32);
  g_blk8_madi.create(width_align32, height_align32, 3);
  g_blk8_lum.create(width_align32, height_align32, 3);

#ifdef CVI_FAST_CU_ENC_BY_COST
  if (isEnableFastCUEncByCost())
  {
    gp_fe_param = new FAST_ENC_PARAM;
    gp_fe_param->blk16_cost.create(width_align32, height_align32, 4);
    gp_fe_param->blk32_cost.create(width_align32, height_align32, 5);
  }
#endif //~CVI_FAST_CU_ENC_BY_COST
}

int calc_thr_by_hist(int targetCnt, int* hist, int bin_num, int bin_shift)
{
  int accumCnt = 0, last_accumCnt = 0;
  for(int bin_i=0; bin_i<bin_num; bin_i++)
  {
    accumCnt += hist[bin_i];
    if(accumCnt >= targetCnt) {
      int bin_incr = accumCnt - last_accumCnt;
      int thr_offset = (((targetCnt-last_accumCnt)<<bin_shift)+(bin_incr>>1)) / bin_incr;
      return (bin_i << bin_shift) + thr_offset;
    }
    last_accumCnt = accumCnt;
  }
  return 255;
}

void ccu_calc_i4_term_madi_thr(double termRatio, int total_cnt, bool is_first_frame)
{
  g_picTargetI4Term = (int)(termRatio * total_cnt);
  g_picTermI4Cnt = 0;
  if (is_first_frame || g_picTargetI4Term==0) {
    return;
  }
  g_i4_madi_thr = calc_thr_by_hist(g_picTargetI4Term, g_madi8_hist_delay, 1<<MADI_BIN_PREC, 8-MADI_BIN_PREC);
}

bool ccu_i4_term_decision(int pos_x, int pos_y)
{
  bool termEn = false;
  if (g_picTermI4Cnt < g_picTargetI4Term) {
    int madi = g_blk8_madi.get_data(pos_x, pos_y);
    termEn = madi <= g_i4_madi_thr;
    g_picTermI4Cnt += termEn;
    g_historyTermI4Cnt += termEn;
  }
  g_historyI4Cnt ++;
  return termEn;
}

void ccu_updateTermStats()
{
  if(g_picMaxTargetTerm==0&&g_picMinTargetTerm==0) {
    return;
  }
  g_picCU32Cnt ++;
  g_historyCu32Cnt++;
  g_historyTermCU16Cnt += g_cuTermCU16Cnt;
  g_historyTermCU8Cnt += (g_cuTermCU16Cnt==0) ? g_cuTermCU8Cnt : 0;

  g_picTermCU16Cnt += g_cuTermCU16Cnt;
  g_picTermCU8Cnt += (g_cuTermCU16Cnt==0) ? g_cuTermCU8Cnt : 0;
  int currTsUnit = (g_cuTermCU16Cnt>0)
                 ? (g_cuTermCU16Cnt*CU16_LAYER_TS_UNIT)
                 : (g_cuTermCU8Cnt*CU8_LAYER_TS_UNIT);
  g_picTsUnitCnt += currTsUnit;

  g_historyCUTimeUnit += CU32_LAYER_TIME_UNIT;
  g_historyTsUnitCnt += currTsUnit;
  g_cuTermCU16Cnt = 0;
  g_cuTermCU8Cnt = 0;
}

void ccu_updateCUStatus(int cu_x, int cu_y, int width, int depth)
{
  if(width==32) {
    ccu_updateTermStats();
    if(depth==0) {
      g_cu_depth_info[g_wrt_buf_idx].set_data(cu_x, cu_y, g_unreliable_cu);
      g_cu_depth_info[g_wrt_buf_idx].set_data(cu_x+16, cu_y, g_unreliable_cu);
      g_cu_depth_info[g_wrt_buf_idx].set_data(cu_x+16, cu_y+16, g_unreliable_cu);
      g_cu_depth_info[g_wrt_buf_idx].set_data(cu_x, cu_y+16, g_unreliable_cu);
    }
  }
  else if(width==16) {
    assert((depth+g_unreliable_cu)<=2);
    g_cu_depth_info[g_wrt_buf_idx].set_data(cu_x, cu_y, depth+g_unreliable_cu);
  }
}

void ccu_registerTermStats(int cu_size, int term_en)
{
  if(term_en){
   if(cu_size==32){
     g_cuTermCU16Cnt++;
   }
   else if(cu_size==16){
     g_cuTermCU8Cnt++;
   }
  }
}

void ccu_frame_init(bool isIframe, int poc, double targetTerm, int frame_width, int frame_height)
{
  g_picCU32Cnt = 0;
  g_picTsUnitCnt = 0;
  g_picTermCU16Cnt = 0;
  g_picTermCU8Cnt = 0;
  g_picMaxTargetTerm = 0;
  g_picMinTargetTerm = 0;
  g_rd_buf_idx = poc & 1;
  g_wrt_buf_idx = !g_rd_buf_idx;

  if(isIframe==false && g_lastFrameIsI==false) {
    g_picCU32Num = (frame_width>>5)*(frame_height>>5);
    g_picMaxTargetTerm = targetTerm * g_picCU32Num * CU32_LAYER_TIME_UNIT;
    g_picMinTargetTerm = targetTerm * g_picCU32Num * CU32_LAYER_TIME_UNIT;
    double normRatio = targetTerm * CU32_LAYER_TIME_UNIT/(CU16_LAYER_TS_UNIT);
    g_smooth_cu_thr = calc_thr_by_hist(g_picCU32Num*16*normRatio , g_madi8_hist_delay, (1<<MADI_BIN_PREC), 8-MADI_BIN_PREC);
  }
  g_lastFrameIsI = isIframe;
}

int ccu_get_i4_madi_thr() { return g_i4_madi_thr; };
int ccu_get_pic_target_i4_term() { return g_picTargetI4Term; };

///////////////////////////////////////////////////////////////////
// CU Weighting Control
///////////////////////////////////////////////////////////////////
#ifdef CVI_CU_PENALTY

int g_init_cost_wt[3][3][3];  // [background/foreground/cu_weight][CU32/16/8][intra/inter/skip]
int g_init_cost_bi[3][3][3];  // [background/foreground/cu_weight][CU32/16/8][intra/inter/skip]

int *gp_hw_cost[2][3];        // [is_foreground][CU32/16/8][intra/inter/skip]
int *gp_hw_bias[2][3];        // [is_foreground][CU32/16/8][intra/inter/skip]

UInt64 ccu_get_max_rd_cost() { return CVI_RDCOST_MAX; };

void cvi_set_hw_cost_wt_default()
{
  for (int mode = 0; mode < 3; mode++)
  {
    for (int cu = 0; cu < 3; cu++)
    {
      for (int pred = 0; pred < 3; pred++)
        g_init_cost_wt[mode][cu][pred] = CVI_HW_COST_WT_DEFAULT;
    }
  }
  memset(&g_init_cost_bi, 0, sizeof(g_init_cost_bi));
}

void cvi_set_hw_combined_cost_wt(int size, int pred, double cfg_fw, double cfg_bw, double cfg_cw, int cfg_bias)
{
  int mul = (1 << CVI_PEN_COST_FRAC_BIT);

  g_init_cost_wt[0][size][pred] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg_bw * cfg_cw * mul));
  g_init_cost_bi[0][size][pred] = Clip3(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, (int)(cfg_bias * cfg_bw + 0.5));

  g_init_cost_wt[1][size][pred] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg_fw * cfg_cw * mul));
  g_init_cost_bi[1][size][pred] = Clip3(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, (int)(cfg_bias * cfg_fw + 0.5));
}

void cvi_init_hw_cost_weight()
{
  cvi_set_hw_cost_wt_default();

  cost_penalty_cfg_st cfg = g_algo_cfg.CostPenaltyCfg;

  int mul = (1 << CVI_PEN_COST_FRAC_BIT);
  if (cfg.EnableCuCost)
  {
    // For I_Slice or (EnableForeground == false)
    g_init_cost_wt[2][0][0] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.Intra32Cost * mul));
    g_init_cost_wt[2][0][1] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.Inter32Cost * mul));
    g_init_cost_wt[2][0][2] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.Skip32Cost * mul));
    g_init_cost_wt[2][1][0] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.Intra16Cost * mul));
    g_init_cost_wt[2][1][1] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.Inter16Cost * mul));
    g_init_cost_wt[2][1][2] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.Skip16Cost * mul));
    g_init_cost_wt[2][2][0] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.Intra8Cost * mul));
    g_init_cost_wt[2][2][1] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.Inter8Cost * mul));
    g_init_cost_wt[2][2][2] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.Skip8Cost * mul));

    g_init_cost_bi[2][0][0] = Clip3(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, cfg.Intra32CostBias);
    g_init_cost_bi[2][0][1] = Clip3(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, cfg.Inter32CostBias);
    g_init_cost_bi[2][0][2] = Clip3(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, cfg.Skip32CostBias);
    g_init_cost_bi[2][1][0] = Clip3(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, cfg.Intra16CostBias);
    g_init_cost_bi[2][1][1] = Clip3(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, cfg.Inter16CostBias);
    g_init_cost_bi[2][1][2] = Clip3(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, cfg.Skip16CostBias);
    g_init_cost_bi[2][2][0] = Clip3(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, cfg.Intra8CostBias);
    g_init_cost_bi[2][2][1] = Clip3(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, cfg.Inter8CostBias);
    g_init_cost_bi[2][2][2] = Clip3(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, cfg.Skip8CostBias);
  }

  if (cfg.EnableForeground)
  {
    if (cfg.EnableCuCost)
    {
      // Combine CU weighting and foreground weighting
      cvi_set_hw_combined_cost_wt(0, 0, cfg.ForegroundIntraCost, cfg.BackgroundIntraCost, cfg.Intra32Cost, cfg.Intra32CostBias);
      cvi_set_hw_combined_cost_wt(0, 1, cfg.ForegroundInterCost, cfg.BackgroundInterCost, cfg.Inter32Cost, cfg.Inter32CostBias);
      cvi_set_hw_combined_cost_wt(0, 2, cfg.ForegroundSkipCost, cfg.BackgroundSkipCost, cfg.Skip32Cost, cfg.Skip32CostBias);

      cvi_set_hw_combined_cost_wt(1, 0, cfg.ForegroundIntraCost, cfg.BackgroundIntraCost, cfg.Intra16Cost, cfg.Intra16CostBias);
      cvi_set_hw_combined_cost_wt(1, 1, cfg.ForegroundInterCost, cfg.BackgroundInterCost, cfg.Inter16Cost, cfg.Inter16CostBias);
      cvi_set_hw_combined_cost_wt(1, 2, cfg.ForegroundSkipCost, cfg.BackgroundSkipCost, cfg.Skip16Cost, cfg.Skip16CostBias);

      cvi_set_hw_combined_cost_wt(2, 0, cfg.ForegroundIntraCost, cfg.BackgroundIntraCost, cfg.Intra8Cost, cfg.Intra8CostBias);
      cvi_set_hw_combined_cost_wt(2, 1, cfg.ForegroundInterCost, cfg.BackgroundInterCost, cfg.Inter8Cost, cfg.Inter8CostBias);
      cvi_set_hw_combined_cost_wt(2, 2, cfg.ForegroundSkipCost, cfg.BackgroundSkipCost, cfg.Skip8Cost, cfg.Skip8CostBias);
    }
    else
    {
      g_init_cost_wt[0][0][0] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.BackgroundIntraCost * mul));
      g_init_cost_wt[0][0][1] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.BackgroundInterCost * mul));
      g_init_cost_wt[0][0][2] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.BackgroundSkipCost * mul));
      g_init_cost_wt[1][0][0] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.ForegroundIntraCost * mul));
      g_init_cost_wt[1][0][1] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.ForegroundInterCost * mul));
      g_init_cost_wt[1][0][2] = Clip3(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, (int)(cfg.ForegroundSkipCost * mul));

      for (int size = 1; size <= 2; size++)
      {
        memcpy(g_init_cost_wt[0][size], g_init_cost_wt[0][0], sizeof(int) * 3);
        memcpy(g_init_cost_wt[1][size], g_init_cost_wt[1][0], sizeof(int) * 3);
      }
    }
  }
}

void cvi_random_hw_cost_weight()
{
  cvi_set_hw_cost_wt_default();

  cost_penalty_cfg_st cfg = g_algo_cfg.CostPenaltyCfg;

  if (cfg.EnableCuCost)
  {
    for (int blk = 0; blk < 3; blk++)
    {
      for (int pred = 0; pred < 3; pred++)
      {
        g_init_cost_wt[2][blk][pred] = cvi_random_range(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX);
        g_init_cost_bi[2][blk][pred] = cvi_random_range(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX);
      }
    }
  }

  if (cfg.EnableForeground)
  {
    if (cfg.EnableCuCost)
    {
      for (int fg = 0; fg < 2; fg++)
      {
        for (int blk = 0; blk < 3; blk++)
        {
          for (int pred = 0; pred < 3; pred++)
          {
            g_init_cost_wt[fg][blk][pred] = cvi_random_range(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX);
            g_init_cost_bi[fg][blk][pred] = cvi_random_range(CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX);
          }
        }
      }
    }
    else
    {
      for (int fg = 0; fg < 2; fg++)
      {
          for (int pred = 0; pred < 3; pred++)
          {
            g_init_cost_wt[fg][0][pred] = cvi_random_range(CVI_PEN_COST_MIN, CVI_PEN_COST_MAX);
          }
      }
      for (int size = 1; size <= 2; size++)
      {
        memcpy(g_init_cost_wt[0][size], g_init_cost_wt[0][0], sizeof(int) * 3);
        memcpy(g_init_cost_wt[1][size], g_init_cost_wt[1][0], sizeof(int) * 3);
      }
    }
  }
}

void cvi_set_pic_cost_weight(bool is_i_slice, bool is_fg_en)
{
  int bg_idx = 0;
  if (is_i_slice || (is_i_slice == false && is_fg_en == false))
    bg_idx = 2;

  for (int i = 0; i < 3; i++)
  {
    gp_hw_cost[0][i] = &g_init_cost_wt[bg_idx][i][0];
    gp_hw_bias[0][i] = &g_init_cost_bi[bg_idx][i][0];

    gp_hw_cost[1][i] = &g_init_cost_wt[1][i][0];
    gp_hw_bias[1][i] = &g_init_cost_bi[1][i][0];
  }
}

void get_cu_cost_weighting(int size, int pred, int fg, int *p_hw_wt, int *p_hw_bi)
{
  *p_hw_wt = gp_hw_cost[fg][size][pred];
  *p_hw_bi = gp_hw_bias[fg][size][pred];
}

double calc_weighted_cu_cost(double cost, int weight, int bias, int wt_frac_bit)
{
  Int64 fix_point_cost = (Int64)(cost * (1 << RD_COST_FRAC_BIT));

  fix_point_cost = ((fix_point_cost * weight) >> wt_frac_bit) + bias * (1 << RD_COST_FRAC_BIT);
  fix_point_cost = Clip3(Int64(0), (Int64)(CVI_RDCOST_MAX), fix_point_cost);
  cost = (double)(fix_point_cost) / (1 << RD_COST_FRAC_BIT);

  return cost;
}

void calcCuPenaltyCost(CU_PENALTY_MODE mode, double &cost, int cu_width, int cu_x, int cu_y, bool is_i_slice)
{
  if (cost == MAX_DOUBLE)
    return;

  if (g_algo_cfg.CostPenaltyCfg.EnableCuCost == false && is_i_slice)
    return;

  int fg = 0;
  if (g_algo_cfg.CostPenaltyCfg.EnableForeground && !is_i_slice)
  {
    int fg_idx = (cu_width == 32) ? 1 : ((cu_width == 16) ? 0 : 2);
    fg = (g_foreground_map[fg_idx].get_data(cu_x, cu_y) > 0) ? 1 : 0;
  }

  int hw_weight = CVI_HW_COST_WT_DEFAULT;
  int hw_bias = 0;
  int size = (cu_width == 32) ? 0 : ((cu_width == 16) ? 1 : 2);
  get_cu_cost_weighting(size, (int)(mode), fg, &hw_weight, &hw_bias);
  cost = calc_weighted_cu_cost(cost, hw_weight, hw_bias, CVI_PEN_COST_FRAC_BIT);
}
#endif //~CVI_CU_PENALTY

#ifdef CVI_SPLIT_COST_WEIGHT
double calc_weighted_cu_cost_split(UInt bits, Distortion sse, UInt64 lambda,
                                   int weight, int bias, int wt_frac_bit)
{
  Int64 sse_cost  = ((sse * weight) << RD_COST_FRAC_BIT) >> wt_frac_bit;
  Int64 bit_cost  = (lambda * weight * bits) >> wt_frac_bit;
  Int64 bias_cost = bias << RD_COST_FRAC_BIT;
  Int64 final_cost = Clip3(Int64(0), (Int64)(CVI_RDCOST_MAX), (sse_cost + bit_cost + bias_cost));

  double cost = (double)(final_cost) / (1 << RD_COST_FRAC_BIT);
  return cost;
}
#endif //~CVI_SPLIT_COST_WEIGHT

#ifdef CVI_SPLIT_COST_WEIGHT
void calcCuPenaltyCostSplit(CU_PENALTY_MODE mode, double &cost, UInt bits, Distortion dist,
                            UInt64 lambda, int cu_width, int cu_x, int cu_y, bool is_i_slice)
{
  if (cost == MAX_DOUBLE)
    return;

  if (g_algo_cfg.CostPenaltyCfg.EnableCuCost == false && is_i_slice)
    return;

  int fg = 0;
  if (g_algo_cfg.CostPenaltyCfg.EnableForeground && !is_i_slice)
  {
    int fg_idx = (cu_width == 32) ? 1 : ((cu_width == 16) ? 0 : 2);
    fg = (g_foreground_map[fg_idx].get_data(cu_x, cu_y) > 0) ? 1 : 0;
  }

  int hw_weight = CVI_HW_COST_WT_DEFAULT;
  int hw_bias = 0;
  int size = (cu_width == 32) ? 0 : ((cu_width == 16) ? 1 : 2);
  get_cu_cost_weighting(size, (int)(mode), fg, &hw_weight, &hw_bias);
  cost = calc_weighted_cu_cost_split(bits, dist, lambda, hw_weight, hw_bias, CVI_PEN_COST_FRAC_BIT);
}
#endif //~CVI_SPLIT_COST_WEIGHT

#ifdef CVI_FOREGROUND_QP
#define USE_PREV_MOTION_MAP

int calcForegroundQpDelta(int cu_x, int cu_y, int cu_width)
{
  int fg_qp_delta = g_algo_cfg.CostPenaltyCfg.ForegroundDeltaQp;
  int bg_qp_delta = g_algo_cfg.CostPenaltyCfg.BackgroundDeltaQp;

  int idx = (cu_width == 32) ? 1 : ((cu_width == 16) ? 0 : 2);
  double fg_map = g_foreground_map[idx].get_data(cu_x, cu_y);
  int qp_delta = bg_qp_delta + (fg_qp_delta - bg_qp_delta) * fg_map + 0.5;

  return qp_delta;
}

void calcRcForegroundMap(int cu_x, int cu_y, int cu_width)
{
#ifdef USE_PREV_MOTION_MAP
  calcForegroundMap(cu_x, cu_y, cu_width);
#endif
}
#endif //~CVI_FOREGROUND_QP

#ifdef CVI_FAST_CU_ENC_BY_COST
FAST_ENC_PARAM *gp_fe_param;
int g_cu16_save_map[2][2];

void calc_cu_save_ratio(int width, int height, int cu32_target_ratio,
                        float *p_cu16_ratio, float *p_cu32_ratio,
                        UInt64 *p_cu16_thr, UInt64 *p_cu32_thr,
                        int *p_cu16_hist, int *p_cu32_hist, int hist_bin,
                        int cu16_scale_bin, int cu32_scale_bin)
{
  FAST_ENC_PARAM *p_param = gp_fe_param;
  FastEncMode fast_enc_mode = p_param->fast_enc_mode;

  int total_cu16_count = p_param->total_cu16_num;
  int total_cu32_count = p_param->total_cu32_num;
  int target_cnt = (cu32_target_ratio * total_cu16_count) / 100;
  int accum_cnt = 0, last_accum_cnt = 0;
  int cu32_thr_from_cu16 = 0;

  // Find CU32 threshold from CU16 hist.
  int scale_bin = cu16_scale_bin;
  for (int i = 0; i < hist_bin; i++)
  {
    accum_cnt += p_cu16_hist[i];
    if (accum_cnt >= target_cnt)
    {
      int bin_incr = accum_cnt - last_accum_cnt;
      int thr_offset = (((target_cnt - last_accum_cnt) << scale_bin) + (bin_incr >> 1)) / bin_incr;
      cu32_thr_from_cu16 =  (i << scale_bin) + thr_offset;
      break;
    }
    last_accum_cnt = accum_cnt;
  }

  int max_thr = 0;
  int cu32_thr = 0;
  if (fast_enc_mode == FAST_ENC_P_TC)
  {
    int max_thr = (fast_enc_mode == FAST_ENC_P_TC) ? 128 : MAX_INT;
    cu32_thr = min(max_thr, cu32_thr_from_cu16);
  }
  else if (fast_enc_mode == FAST_ENC_P_COST)
  {
    cu32_thr = (cu32_thr_from_cu16 << 2);
  }

  // Find CU32 ratio from CU32 threshold.
  scale_bin = cu32_scale_bin;
  accum_cnt = 0;

  for (int i = 0; i < hist_bin; i++)
  {
    if (((i+1)<<scale_bin) >= cu32_thr)
    {
      int cost_incr = cu32_thr - (i<<scale_bin);
      int bin_offset = cost_incr * p_cu32_hist[i] / (1 << scale_bin);
      accum_cnt = last_accum_cnt+bin_offset;
      break;
    }
    accum_cnt += p_cu32_hist[i];
    last_accum_cnt = accum_cnt;
  }

  float cu32_ratio = Clip3((float)(0.0), (float)(1.0), ((float)(accum_cnt) / total_cu32_count));

  *p_cu32_thr   = cu32_thr;
  *p_cu32_ratio = cu32_ratio;

  // Find CU16 ratio from CU32 ratio.
  float target_time_save = (float)(g_algo_cfg.FastEncTimeSaving) / 100.0;
  float cu32_time_save = cu32_ratio * 0.5;
  float cu16_time_save = target_time_save - cu32_time_save;
  float cu16_ratio = Clip3((float)(0.0), (float)(1.0), ((float)(cu16_time_save / 0.3) + cu32_ratio));

  // Find CU16 threshold from CU16 ratio.
  scale_bin = cu16_scale_bin;
  target_cnt = (cu16_ratio * total_cu16_count);
  accum_cnt = 0;
  last_accum_cnt = 0;
  int cu16_thr = max_thr;

  for (int i = 0; i < hist_bin; i++)
  {
    accum_cnt += p_cu16_hist[i];
    if (accum_cnt >= target_cnt)
    {
      int bin_incr = accum_cnt - last_accum_cnt;
      int thr_offset = (((target_cnt - last_accum_cnt) << scale_bin) + (bin_incr >> 1)) / bin_incr;
      cu16_thr =  (i << scale_bin) + thr_offset;
      break;
    }
    last_accum_cnt = accum_cnt;
  }

  *p_cu16_thr = cu16_thr;
  *p_cu16_ratio = cu16_ratio;
}

void frm_init_fast_cu_enc_by_cost(int width, int height, int slice_type)
{
  FAST_ENC_PARAM *p_param = gp_fe_param;

  p_param->total_cu32_num = (width >> 5) * (height >> 5);
  p_param->total_cu16_num = (width >> 4) * (height >> 4);
  p_param->total_time_unit   = FAST_ENC_CU32_TIME * p_param->total_cu32_num;
  p_param->target_save_unit  = (p_param->total_time_unit * g_algo_cfg.FastEncTimeSaving) / 100;
  p_param->current_save_unit = 0;

  p_param->cu32_time      = FAST_ENC_CU32_TIME;
  p_param->cu32_time_save = FAST_ENC_CU32_TIME_SAVE;
  p_param->cu16_time_save = FAST_ENC_CU16_TIME_SAVE;

  p_param->time_save_level   = FE_LEVEL_0;
  p_param->thr_amplifier     = FAST_ENC_THR_AMP_DEFAULT;

  if (slice_type == I_SLICE)
  {
    p_param->fast_enc_mode = FAST_ENC_I;
  }
  else if (slice_type == P_SLICE)
  {
    if (p_param->fast_enc_mode == FAST_ENC_I)
      p_param->fast_enc_mode = FAST_ENC_P_TC;
    else
      p_param->fast_enc_mode = FAST_ENC_P_COST;

    p_param->cu_cost_avg[FE_CU16] = p_param->cu_cost_accum[FE_CU16] / p_param->total_cu16_num;
    p_param->cu_cost_scale_log2[FE_CU16] = (int)(log2((p_param->cu_cost_avg[FE_CU16] >> (CU_COST_HIST_DEPTH - 1))) + 0.5);

    if (p_param->fast_enc_mode == FAST_ENC_P_TC)
    {
      p_param->cu_cost_avg[FE_CU32] = 0;  // not used
      p_param->cu_cost_scale_log2[FE_CU32] = p_param->cu_cost_scale_log2[FE_CU16] + 2;
    }
    else
    {
      p_param->cu_cost_avg[FE_CU32] = p_param->cu_cost_accum[FE_CU32] / p_param->total_cu32_num;
      p_param->cu_cost_scale_log2[FE_CU32] = (int)(log2((p_param->cu_cost_avg[FE_CU32] >> (CU_COST_HIST_DEPTH - 1))) + 0.5);
    }
  }

  float cu16_save_ratio_max = (float)(p_param->cu16_time_save << 2) / p_param->cu32_time;

  g_algo_cfg.FastEncRatioCu16 = ((float)g_algo_cfg.FastEncTimeSaving/cu16_save_ratio_max + 0.5);
  g_algo_cfg.FastEncRatioCu16 = min(100, g_algo_cfg.FastEncRatioCu16);

  int FastEncRatioCu32 = g_algo_cfg.FastEncRatioCu32;
  if (g_algo_cfg.FastEncTimeSaving > 20)
  {
    FastEncRatioCu32 += ((g_algo_cfg.FastEncTimeSaving - 20) * 2);
    FastEncRatioCu32 = min(100, FastEncRatioCu32);
  }

  p_param->cu_cost_target_ratio[FE_CU8]  = 0;  // Not used.
  p_param->cu_cost_target_ratio[FE_CU16] = g_algo_cfg.FastEncRatioCu16;
  p_param->cu_cost_target_ratio[FE_CU32] = FastEncRatioCu32;

  float cu16_ratio = 0.0;
  float cu32_ratio = 0.0;
  if (p_param->fast_enc_mode == FAST_ENC_P_TC)
  {
    UInt64 cu16_thr = 0;
    UInt64 cu32_thr = 0;
    int madi_scale_bin = 7 - MADI_HIST_PREC;

    calc_cu_save_ratio(width, height, p_param->cu_cost_target_ratio[FE_CU32],
                       &cu16_ratio, &cu32_ratio, &cu16_thr, &cu32_thr,
                       g_madi16_hist_delay, g_madi32_hist_delay,
                       (1 << MADI_HIST_PREC), madi_scale_bin, madi_scale_bin);

    p_param->cu_tc_thr[FE_CU16] = (int)(cu16_thr);
    p_param->cu_tc_thr[FE_CU32] = (int)(cu32_thr);
  }
  else if (p_param->fast_enc_mode == FAST_ENC_P_COST)
  {
    UInt64 cu16_thr = 0;
    UInt64 cu32_thr = 0;

    calc_cu_save_ratio(width, height, p_param->cu_cost_target_ratio[FE_CU32],
                       &cu16_ratio, &cu32_ratio, &cu16_thr, &cu32_thr,
                       p_param->cu_cost_hist[FE_CU16],
                       p_param->cu_cost_hist[FE_CU32],
                       CU_COST_HIST_BIN,
                       p_param->cu_cost_scale_log2[FE_CU16],
                       p_param->cu_cost_scale_log2[FE_CU32]);

    p_param->cu_cost_thr[FE_CU16] = cu16_thr;
    p_param->cu_cost_thr[FE_CU32] = cu32_thr;
  }

  /////////////////////////////////////////////////////////////////////////////
  // HW reset.
  p_param->enc_cu32_cnt  = 0;
  p_param->cu32_save_cnt = 0;
  p_param->cu16_save_cnt = 0;
  p_param->blk16_cost.set_all(0);
  p_param->blk32_cost.set_all(0);
  memset(p_param->cu_cost_accum, 0, sizeof(p_param->cu_cost_accum));
  memset(p_param->cu_cost_hist, 0, sizeof(p_param->cu_cost_hist));
  /////////////////////////////////////////////////////////////////////////////

  printf("[FastEnc][frm%d] ==== Init ========================\n", g_sigpool.enc_count_pattern);
  printf("[FastEnc] Mode = %d, CU32 ratio = %d, CU16 ratio = %d\n",
         (int)(p_param->fast_enc_mode), p_param->cu_cost_target_ratio[2], p_param->cu_cost_target_ratio[1]);

  if (p_param->fast_enc_mode == FAST_ENC_P_TC)
  {
    printf("[FastEnc] CU32 madi ratio = %f, CU16 madi ratio = %f\n",
           cu32_ratio, cu16_ratio);
    printf("[FastEnc] CU32 madi threshold = %d, CU16 madi threshold = %d\n",
           p_param->cu_tc_thr[2], p_param->cu_tc_thr[1]);
  }
  else if (p_param->fast_enc_mode == FAST_ENC_P_COST)
  {
    printf("[FastEnc] CU32 cost threshold = %lld, CU16 cost threshold = %lld\n",
           p_param->cu_cost_thr[2], p_param->cu_cost_thr[1]);
  }
}

void store_current_cu_cost(int x, int y, int size, double cost)
{
  if (cost == MAX_DOUBLE)
    return;

  UInt64 int64_cost = (UInt64)(cost);

  if (size == 32)
    gp_fe_param->blk32_cost.set_data(x, y, int64_cost);
  else if (size == 16)
    gp_fe_param->blk16_cost.set_data(x, y, int64_cost);
}

void frm_update_fast_cu_enc_by_cost(int width, int height)
{
  FAST_ENC_PARAM *p_param = gp_fe_param;
  FastEncMode fast_enc_mode = p_param->fast_enc_mode;

  if (fast_enc_mode == FAST_ENC_P_TC || fast_enc_mode == FAST_ENC_P_COST)
  {
    int width_in_32  = max(1, width >> 5);
    int height_in_32 = max(1, height >> 5);
    int cu32_total = width_in_32 * height_in_32;

    int cu32_enc_time = p_param->cu32_time;
    int cu32_time_save = p_param->cu32_time_save;
    int cu16_time_save = p_param->cu16_time_save;

    static int count=0;
    static double save_ratio_sum=0;
    int cu32_save_cnt = gp_fe_param->cu32_save_cnt;
    int cu16_save_cnt = gp_fe_param->cu16_save_cnt;

    double save_ratio = ((cu32_save_cnt * cu32_time_save) + (cu16_save_cnt * cu16_time_save)) / (double)(cu32_total * cu32_enc_time);
    save_ratio_sum += save_ratio;
    count += 1;
    g_historyTsUnitCnt =save_ratio_sum/count*1000;
    g_historyCUTimeUnit = 1000;

    printf("[FastEnc][COST] CU32 target ratio = %d\n", p_param->cu_cost_target_ratio[FE_CU32]);
    printf("[FastEnc][COST] CU16 target ratio = %d\n", p_param->cu_cost_target_ratio[FE_CU16]);
    printf("[FastEnc][COST] CU32 save count = %d, ratio = %lf\n", cu32_save_cnt, cu32_save_cnt / (double)(cu32_total));
    printf("[FastEnc][COST] CU16 save count(ignore cu32) = %d\n", cu16_save_cnt);
    printf("[FastEnc][COST] save_ratio = %lf\n", save_ratio);
    printf("[FastEnc][COST] save_ratio mean= %lf\n", save_ratio_sum/count);
  }
}

void update_fast_enc_status()
{
  FAST_ENC_PARAM *p_param = gp_fe_param;

  int cu32_save_cnt = p_param->cu32_save_cnt;
  int cu16_save_cnt = p_param->cu16_save_cnt;

  int remain_cu32_num = p_param->total_cu32_num - p_param->enc_cu32_cnt;
  p_param->current_save_unit = cu32_save_cnt * p_param->cu32_time_save +
                               cu16_save_cnt * p_param->cu16_time_save;

  int pred_remain_save_unit = remain_cu32_num * (p_param->current_save_unit + p_param->cu32_time) / (p_param->enc_cu32_cnt + 1);
  int pred_save_unit = p_param->current_save_unit + pred_remain_save_unit;
  p_param->enc_cu32_cnt++;

  if (p_param->time_save_level == FE_LEVEL_2)
    return;

  if (pred_save_unit < p_param->target_save_unit)
  {
    if ((remain_cu32_num * p_param->cu32_time_save) < (p_param->target_save_unit - p_param->current_save_unit))
    {
      p_param->time_save_level = FE_LEVEL_2;
    }
    else
    {
      p_param->time_save_level = FE_LEVEL_1;
      int lack_ratio = (int)(100 * (1.0 - ((float)(pred_save_unit) / p_param->target_save_unit)));
      p_param->thr_amplifier = FAST_ENC_THR_AMP_DEFAULT;

      if (lack_ratio >= 3)
      {
        p_param->thr_amplifier = Clip3(FAST_ENC_THR_AMP_MIN, FAST_ENC_THR_AMP_MAX,
                                           (1 << FAST_ENC_THR_AMP_DEPTH) + lack_ratio);
      }
    }
  }
  else
  {
    p_param->time_save_level = FE_LEVEL_0;
    p_param->thr_amplifier = FAST_ENC_THR_AMP_DEFAULT;
  }
}

UInt64 fast_enc_thr_amplifier(UInt64 threshold)
{
  UInt64 thr_amp = ((threshold * gp_fe_param->thr_amplifier) >> FAST_ENC_THR_AMP_DEPTH);
  return thr_amp;
}
#endif //~CVI_FAST_CU_ENC_BY_COST


////////////////////////////////////////////////////////////////////////////
// AI-ISP Smart Encode
////////////////////////////////////////////////////////////////////////////
#ifdef CVI_SMART_ENC
AI_ENC_PARAM *gp_ai_enc_param = nullptr;
ISP_ENC_PARAM *gp_isp_enc_param = nullptr;

// AI segmentation map to delta qp table
// 5 bits delta qp : [-16, 15]
int g_smart_enc_ai_table[AI_SI_TAB_COUNT][AI_SI_TAB_BIN] =
{
  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,   0,   0,   0 },
  {  0,  0,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,   2,   3,   3,   3 },
  {  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,   1,   1,   1,   1 },
  {  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1,  -1,  -1,  -1,  -1 },
  {  0,  0, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2,  -2,  -3,  -3,  -3 },
  {  0, -1, -1, -1, -2, -2, -2, -3, -3, -3, -3, -4,  -4,  -4,  -5,  -5 },
  {  0, -1, -1, -2, -2, -3, -3, -4, -4, -4, -5, -5,  -6,  -6,  -7,  -7 },
  { -1, -1, -2, -3, -3, -4, -4, -5, -6, -6, -7, -8,  -8,  -9,  -9, -10 }
};

// AI importance level table
int g_smart_enc_ai_citable_init[AI_SI_TAB_COUNT][AI_SI_TAB_BIN] =
{
  { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3 },
  { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4 },
  { 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5 },
  { 0, 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6 },
  { 0, 1, 1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7 },
  { 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 }
};

string g_ai_pattern_path;
string g_ai_scene_pattern_name;
int g_ai_table_idx_min = 0;
int g_ai_table_idx_max = 15;
int g_ai_conf_scale    = 41;  // (16/100) ~= (41/256)
int g_ai_blk_size      = 16;

string g_isp_pattern_path;
int g_isp_motion_qp      = -3;
int g_isp_trans_qp       = -7;
int g_clip_delta_qp_min  = -51;
int g_clip_delta_qp_max  = 51;

void parse_mapping_table_blk16()
{
  AI_ENC_PARAM *p_ai_param = gp_ai_enc_param;
  sig_ctx *p_mt_ctx = &p_ai_param->mapping_table_ctx;
  AI_MAP_TAB map_data;

#ifdef SIG_SMT
  if (g_sigdump.smt_rand)
  {
    for (int i = 0; i < 12; i++)
    {
      map_data.idx = 0;
      map_data.label_s = "";
      map_data.table_idx = 0;

      p_ai_param->mapping_table.push_back(map_data);
    }

    return;
  }
#endif

  string file_name = "./smart_enc/AI_Mapping_Table.txt";
  sigdump_open_ctx_txt_r(p_mt_ctx, file_name);

  char temp_char[512];
  memset (temp_char, 0, sizeof(temp_char));

  // Read delta qp table
  int d2 = 0;
  int d1 = 0;
  int data = 0;
  sigdump_vfscanf(p_mt_ctx, "%[^\n]\n", temp_char);
  sigdump_vfscanf(p_mt_ctx, "%[^\n]\n", temp_char);
  sigdump_vfscanf(p_mt_ctx, "%s %d %s\n", temp_char, &d2, temp_char);
  sigdump_vfscanf(p_mt_ctx, "%s %d %s\n", temp_char, &d1, temp_char);

  for (int i = 0; i < d2; i++)
  {
    sigdump_vfscanf(p_mt_ctx, "%s", temp_char);
    for (int j = 0; j < d1; j++)
    {
      sigdump_vfscanf(p_mt_ctx, "%d%c \n", &data, temp_char);
      g_smart_enc_ai_table[i][j] = data;
    }
    sigdump_vfscanf(p_mt_ctx, "%[^\n]\n", temp_char);
  }

  // Read Object label mapping
  p_ai_param->mapping_table.clear();
  int label_count = 0;

  sigdump_vfscanf(p_mt_ctx, "%[^\n]\n", temp_char);
  sigdump_vfscanf(p_mt_ctx, "%s %d %s\n", temp_char, &label_count, temp_char);

  for (int i = 0; i < label_count; i++)
  {
    sigdump_vfscanf(p_mt_ctx, "%d %s %d\n", &map_data.idx, temp_char, &map_data.table_idx);
    map_data.label_s = temp_char;

    if (map_data.table_idx < 0 || map_data.table_idx >= AI_SI_TAB_COUNT)
      map_data.table_idx = AI_INVALID_IDX;

    p_ai_param->mapping_table.push_back(map_data);
  }
}

void parse_mapping_table_blk64()
{
  AI_ENC_PARAM *p_ai_param = gp_ai_enc_param;
  sig_ctx *p_mt_ctx = &p_ai_param->mapping_table_ctx;
  string file_name = "./smart_enc/AI_Mapping_Table_blk64.txt";
  sigdump_open_ctx_txt_r(p_mt_ctx, file_name);

  char temp_char[512];
  memset (temp_char, 0, sizeof(temp_char));

  // Copy CITable
  memcpy(&g_smart_enc_ai_table[0][0], &g_smart_enc_ai_citable_init[0][0], sizeof(g_smart_enc_ai_citable_init));

  // Read Object label mapping
  p_ai_param->mapping_table.clear();
  AI_MAP_TAB map_data;
  int label_count = 0;

  sigdump_vfscanf(p_mt_ctx, "%[^\n]\n", temp_char);
  sigdump_vfscanf(p_mt_ctx, "%[^\n]\n", temp_char);
  sigdump_vfscanf(p_mt_ctx, "%[^\n]\n", temp_char);
  sigdump_vfscanf(p_mt_ctx, "%s %d %s\n", temp_char, &label_count, temp_char);

  int max_dqp = 0;

  for (int i = 0; i < label_count; i++)
  {
    sigdump_vfscanf(p_mt_ctx, "%d %s %d\n", &map_data.idx, temp_char, &map_data.table_idx);
    map_data.label_s = temp_char;
    p_ai_param->mapping_table.push_back(map_data);

    if (map_data.table_idx < 0 || map_data.table_idx > 16)
      map_data.table_idx = AI_INVALID_IDX;

    max_dqp = max(max_dqp, map_data.table_idx);
  }

  int frame_il_bd = (max_dqp > 0) ? (((max_dqp + 7) >> 3) - 1) : 0;
  p_ai_param->frame_il_bd = Clip3(0, 1, frame_il_bd);
  p_ai_param->roi_delta_qp = p_ai_param->frame_il_bd + 1;
}

void enable_smart_enc_sequence(int src_width, int src_height)
{
  if (g_algo_cfg.EnableSmartEncAI)
  {
    if (gp_ai_enc_param == nullptr)
    {
      gp_ai_enc_param = new AI_ENC_PARAM;
      memset(gp_ai_enc_param, 0, sizeof(AI_ENC_PARAM));
    }

    AI_ENC_PARAM *p_ai_param = gp_ai_enc_param;
    p_ai_param->src_width = src_width;
    p_ai_param->src_height = src_height;

    if (g_ai_blk_size == 16)
      parse_mapping_table_blk16();
    else
      parse_mapping_table_blk64();

    if (g_algo_cfg.EnableSmartEncAI == 2)
    {
      sig_ctx *p_scene_ctx = &p_ai_param->scene_ctx;
      string file_name = g_ai_pattern_path + "/SCENE_CLA/" + g_FileName + ".txt";
      sigdump_open_ctx_txt_r(p_scene_ctx, file_name);

      char temp_char[512];
      memset (temp_char, 0, sizeof(temp_char));

      sigdump_vfscanf(p_scene_ctx, "%s", temp_char);
      g_ai_scene_pattern_name = temp_char;

      int pat_width;
      int pat_height;
      sigdump_vfscanf(p_scene_ctx, "%d\n%d\n", &pat_width, &pat_height);

      if (pat_width != p_ai_param->src_width || pat_height != p_ai_param->src_height)
      {
        printf("[SmartEnc][Error] Scene pattern width/height mismatch.\n");
      }
    }

    int src_w_align16 = cvi_mem_align(p_ai_param->src_width, 16);
    int src_h_align16 = cvi_mem_align(p_ai_param->src_height, 16);
    p_ai_param->seg_map.create(src_w_align16, src_h_align16, 4);
  }

  if(g_algo_cfg.EnableSmartEncISP || g_algo_cfg.EnableDeBreath == 2)
  {
    if (gp_isp_enc_param == nullptr)
    {
      gp_isp_enc_param = new ISP_ENC_PARAM;
      memset(gp_isp_enc_param, 0, sizeof(ISP_ENC_PARAM));
    }
    ISP_ENC_PARAM *p_isp_param = gp_isp_enc_param;
    p_isp_param->src_width = src_width;
    p_isp_param->src_height = src_height;
  }

  // For both AI and ISP
  g_clip_delta_qp_min = -51;
  g_clip_delta_qp_max = 51;
}

void close_smart_enc_sequence()
{
  if(g_algo_cfg.EnableSmartEncAI)
  {
    if (g_algo_cfg.EnableSmartEncAI == 2)
      sigdump_safe_close(&gp_ai_enc_param->scene_ctx);

    gp_ai_enc_param->seg_map.destroy();
    delete gp_ai_enc_param;
  }
  if(g_algo_cfg.EnableSmartEncISP || g_algo_cfg.EnableDeBreath == 2)
  {
    delete gp_isp_enc_param;
  }
}

void process_smart_enc_frame(int frame_idx, int frame_width, int frame_height)
{
  bool is_ai_enable = g_algo_cfg.EnableSmartEncAI ? true : false;
  bool is_isp_enable = (g_algo_cfg.EnableSmartEncISP && frame_idx != 0) ? true : false;

#ifdef SIG_SMT
  if (g_sigdump.smt_rand)
    sig_smt_rand_set(gp_ai_enc_param);
#endif

  if (is_ai_enable)
  {
    frame_read_ai_info(frame_idx);
    calc_smart_enc_qp_map();
  }

  if (is_isp_enable || g_algo_cfg.EnableDeBreath == 2)
  {
    frame_read_isp_info(frame_idx);
  }

  if (is_ai_enable || is_isp_enable || g_algo_cfg.EnableJND)
    frame_clip_slice_qp_bound(frame_width, frame_height);

#ifdef SIG_SMT
  if (g_sigdump.smt)
  {
    if (is_isp_enable)
      sig_smt_isp_set();

    if (is_ai_enable)
      sig_smt_ai_set(gp_ai_enc_param);

    sig_smt_isp_ai_set(g_sigpool.width, g_sigpool.height, is_ai_enable, is_isp_enable);
    sig_smt_isp_ai_gld(frame_width, frame_height);
  }
#endif
}

void frame_read_ai_scene(int frame_idx)
{
  if (g_algo_cfg.EnableSmartEncAI < 2)
    return;

  AI_ENC_PARAM *p_ai_param = gp_ai_enc_param;
  sig_ctx *p_scene_ctx = &p_ai_param->scene_ctx;

  char temp_char[32];
  char prefix_char[16];
  string prefix_pattern = "#Frame";
  int frm_idx = 0;
  int top_idx = 0;
  float top_level = 0.0;

  sigdump_vfscanf(p_scene_ctx, "%s %d%c %s %c %d%c %s %c %f\n",
                  prefix_char, &frm_idx, temp_char, temp_char, temp_char, &top_idx,
                  temp_char, temp_char, temp_char, &top_level);

  if ((prefix_pattern != prefix_char) || (frm_idx != frame_idx))
  {
    printf("[SmartEnc][Error][%d] parse scene info error\n", frame_idx);
  }

  p_ai_param->frame_idx = frm_idx;
  p_ai_param->current_scene_idx = top_idx;
  p_ai_param->current_scene_level = (int)(top_level * 100 + 0.5);
  p_ai_param->scene_hist[top_idx]++;
  p_ai_param->score_accum[top_idx] += p_ai_param->current_scene_level;

  // skip unused lines.
  for (int i = 0; i < 15; i++)
  {
    sigdump_vfscanf(p_scene_ctx, "%[^\n]\n", temp_char);
  }
}

void frame_read_ai_info(int frame_idx)
{
  AI_ENC_PARAM *p_ai_param = gp_ai_enc_param;
  char temp_char[32];

  ///////////////////////////////////////////////////
  // Read scene info
  ///////////////////////////////////////////////////
  if (g_algo_cfg.EnableSmartEncAI == 2)
    frame_read_ai_scene(frame_idx);

  ///////////////////////////////////////////////////
  // Read object info
  ///////////////////////////////////////////////////
  int blk_x = 0;
  int blk_y = 0;
  AI_SEG_BLK data;
  cvi_grid_data<AI_SEG_BLK> *p_seg_map = &p_ai_param->seg_map;
  p_seg_map->set_all(0);

  int blk_size = g_ai_blk_size;
  int src_w_align = cvi_mem_align(p_ai_param->src_width, blk_size);
  int src_h_align = cvi_mem_align(p_ai_param->src_height, blk_size);
  int blk_count = (src_w_align / blk_size) * (src_h_align / blk_size);

#ifdef SIG_SMT
  if (g_sigdump.smt_rand != 0)
  {
    int obj_size = p_ai_param->mapping_table.size() - 1;

    for (int i = 0; i < blk_count; i++)
    {
      data.idx = cvi_random_range(0, obj_size);
      data.level = cvi_random_range(1, 99);
      p_seg_map->set_data(i, data);
    }

    return;
  }
#endif

  sig_ctx *p_obj_ctx = &p_ai_param->object_ctx;

  char idx_string[5];
  sprintf(idx_string, "%04d", frame_idx);
  string file_name = g_ai_pattern_path + g_FileName + "/" + g_FileName + "_" + idx_string + ".txt";
  sigdump_open_ctx_txt_r(p_obj_ctx, file_name);

  // skip unused lines.
  for (int i = 0; i < 3; i++)
  {
    sigdump_vfscanf(p_obj_ctx, "%[^\n]\n", temp_char);
  }

  for (int i = 0; i < blk_count; i++)
  {
    sigdump_vfscanf(p_obj_ctx, "%c%d%c%d%c\n",
                    temp_char, &blk_x, temp_char, &blk_y, temp_char);

    int result = sigdump_vfscanf(p_obj_ctx, "%d\n%d\n", &data.idx, &data.level);
    if (result == -1)
    {
      printf("[SmartEnc][Error][%d] parse obj info error\n", frame_idx);
      break;
    }

    p_seg_map->set_data(blk_x, blk_y, data);

    if (blk_size == 64)
    {
      for (int blk16_y = blk_y; blk16_y < blk_y + 64; blk16_y += 16)
      {
        for (int blk16_x = blk_x; blk16_x < blk_x + 64; blk16_x += 16)
          p_seg_map->set_data(blk16_x, blk16_y, data);
      }
    }
  }

  sigdump_safe_close(p_obj_ctx);
}

void smooth_qp_map(int width, int height)
{
  cvi_grid_data<int> dqp_ori;
  dqp_ori.create(width, height, 4);

  int delta_qp;
  int skip;
  cvi_grid_data<int> *p_qp_map = &g_blk16_qp_map;

  for (int y = 0; y < height; y += 16)
  {
    for (int x = 0; x < width; x += 16)
    {
      getQpMapBlk16(x, y, &skip, NULL, &delta_qp);
      dqp_ori.set_data(x, y, delta_qp);
    }
  }

  for (int y = 0; y < height; y += 16)
  {
    for (int x = 0; x < width; x += 16)
    {
      int qp_cur = dqp_ori.get_data(x, y);
      int qp_num = 0;

      int ctu_size = 64;
      int ctu_y_num = y >> 6;
      int ctu_x_num = x >> 6;
      int CTU_upper_boundary = ctu_y_num*ctu_size;
      int CTU_lower_boundary = (ctu_y_num + 1)*ctu_size;
      int CTU_left_boundary = ctu_x_num*ctu_size;
      int CTU_right_boundary = (ctu_x_num+ 1)*ctu_size;
      int ai_max_qp = AI_DELTA_QP_MIN;
      int ai_min_qp = AI_DELTA_QP_MAX;

      if (qp_cur < 0)
        continue;

      for (int nb_y = (y - 16); nb_y <= y + 16; nb_y += 16)
      {
        for (int nb_x = (x - 16); nb_x <= x + 16; nb_x += 16)
        {
          if (nb_x < 0 || nb_y < 0 || nb_x >= width || nb_y >= height)
            continue;

          int nb_qp = dqp_ori.get_data(nb_x, nb_y);

          if(nb_x < CTU_right_boundary && nb_x >= CTU_left_boundary && nb_y < CTU_lower_boundary && nb_y >= CTU_upper_boundary)
          {
            if(nb_qp < 0)
            {
              if(ai_max_qp < nb_qp)
              {
                ai_max_qp = nb_qp;
              }
              if(ai_min_qp > nb_qp)
              {
                ai_min_qp = nb_qp;
              }
              qp_num++;
            }
          }
        }
      }

      if (qp_num > 0)
      {
        int dqp_new = (ai_max_qp + ai_min_qp) >> 1;
        int map = (((skip & 0x1) << 7) | (dqp_new & 0x7f));
        p_qp_map->set_data(x, y, map);
      }
    }
  }

  dqp_ori.destroy();
}

// return true : delta_qp is valid
bool lookup_seg_delta_qp(int obj_idx, int level, int *p_delta_qp)
{
  AI_ENC_PARAM *p_ai_param = gp_ai_enc_param;

  int tclip_min = g_ai_table_idx_min;
  int tclip_max = g_ai_table_idx_max;
  int conf_scale = g_ai_conf_scale;
  int size = p_ai_param->mapping_table.size();

  if (g_ai_blk_size == 16)
  {
    for (int i = 0; i < size; i++)
    {
      if (obj_idx == p_ai_param->mapping_table[i].idx)
      {
        int enable = (p_ai_param->mapping_table[i].table_idx == AI_INVALID_IDX) ? 0 : 1;
        int dqi = enable ? p_ai_param->mapping_table[i].table_idx : 0;
        int idx = (level * conf_scale) >> AI_CONF_NORM_SCALE_BD;
        int bin = Clip3(tclip_min, tclip_max, idx);
        *p_delta_qp = enable ? g_smart_enc_ai_table[dqi][bin] : 0;

#ifdef SIG_SMT
        if (g_sigdump.smt)
        {
          sig_smt_st *p_smt_st = g_sigpool.p_smt_st;
          int b16_x = p_smt_st->b16_x;
          int b16_y = p_smt_st->b16_y;
          sig_smt_dat_st *p_smt_dat = &p_smt_st->pp_smt_gld_data[b16_y][b16_x];
          p_smt_dat->ena = 1;
          p_smt_dat->obj = obj_idx;
          p_smt_dat->dqp_tab_idx = dqi;
          p_smt_dat->idx = idx;
          p_smt_dat->bin = bin;
          p_smt_dat->ai_dqp = *p_delta_qp;
        }
#endif

        return enable ? true : false;
      }
    }
  }
  else
  {
    for (int i = 0; i < size; i++)
    {
      if (obj_idx == p_ai_param->mapping_table[i].idx)
      {
        if (p_ai_param->mapping_table[i].table_idx == AI_INVALID_IDX)
          return false;

        int dqi = (p_ai_param->mapping_table[i].table_idx - 1) >> p_ai_param->frame_il_bd;
        int idx = (level * conf_scale) >> AI_CONF_NORM_SCALE_BD;
        int bin = Clip3(tclip_min, tclip_max, idx);
        int importance_level = g_smart_enc_ai_table[dqi][bin];
        *p_delta_qp = -(p_ai_param->roi_delta_qp * importance_level);
        return true;
      }
    }
  }

  return false;
}

void calc_smart_enc_qp_map()
{
  AI_ENC_PARAM *p_ai_param = gp_ai_enc_param;
  int src_w_align16 = cvi_mem_align(p_ai_param->src_width, 16);
  int src_h_align16 = cvi_mem_align(p_ai_param->src_height, 16);
  cvi_grid_data<AI_SEG_BLK> *p_seg_map = &p_ai_param->seg_map;
  cvi_grid_data<int> *p_qp_map = &g_blk16_qp_map;

  for (int y = 0; y < src_h_align16; y += 16)
  {
    for (int x = 0; x < src_w_align16; x += 16)
    {
      AI_SEG_BLK seg = p_seg_map->get_data(x, y);
      int delta_qp = 0;

#ifdef SIG_SMT
      if (g_sigdump.smt)
      {
        sig_smt_st *p_smt_st = g_sigpool.p_smt_st;
        int b16_x = (x >> 4);
        int b16_y = (y >> 4);
        p_smt_st->b16_x = b16_x;
        p_smt_st->b16_y = b16_y;
        sig_smt_dat_st *p_smt_dat = &p_smt_st->pp_smt_gld_data[b16_y][b16_x];
        p_smt_dat->ena = 0;
        p_smt_dat->obj = 0;
        p_smt_dat->dqp_tab_idx = 0;
        p_smt_dat->idx = 0;
        p_smt_dat->bin = 0;
        p_smt_dat->ai_dqp = 0;
      }
#endif

      bool is_use_delta_qp = lookup_seg_delta_qp(seg.idx, seg.level, &delta_qp);

      if (is_use_delta_qp)
      {
        if (delta_qp < AI_DELTA_QP_MIN || delta_qp > AI_DELTA_QP_MAX)
        {
          printf("[SmartEnc][Error] lookup delta_qp = %d\n", delta_qp);
          delta_qp =Clip3(AI_DELTA_QP_MIN, AI_DELTA_QP_MAX, delta_qp);
        }

        int map = (delta_qp & 0x7f);
        p_qp_map->set_data(x, y, map);
      }
    }
  }

  if (g_algo_cfg.AISmoothMap)
    smooth_qp_map(src_w_align16, src_h_align16);

#ifdef SIG_SMT
  if (g_sigdump.smt)
  {
    sig_smt_ai_src_gld(p_ai_param);
  }
  else if (g_sigdump.fpga)
  {
    sig_smt_ai_src(p_ai_param);
  }
#endif //~SIG_SMT
}

void frame_read_isp_info(int frame_idx)
{
    char temp_char[32];
    ISP_ENC_PARAM *p_isp_param = gp_isp_enc_param;

    int frame_width = p_isp_param->src_width;
    int frame_height = p_isp_param->src_height;
    int blk_4_x_num = frame_width / 4;
    int blk_4_y_num = frame_height / 4;
    int blk_4_num = blk_4_x_num * blk_4_y_num;
    int MotionQP = g_isp_motion_qp;
    int TransQP = g_isp_trans_qp;

    p_isp_param->map_dqp_accum = 0;
    cvi_grid_data<int> still_ori;
    still_ori.create(frame_width, frame_height, 4);

    if(g_algo_cfg.EnableCalMapData)
    {
      gp_isp_enc_param->skip_bits = 0;
      gp_isp_enc_param->nonskip_bits = 0;
    }
    int diff_qp = 0;
    if(g_algo_cfg.EnableDeBreath)
    {
      diff_qp = p_isp_param->preP_QP - p_isp_param->curI_QP;
    }

    char *data = (char *)malloc(blk_4_num);
    memset(data, 0, blk_4_num);

#ifdef SIG_SMT
    if (g_sigdump.smt_rand == 0) {
#endif
      sig_ctx *fr_motion = &p_isp_param->motion_ctx;
      char idx_string[5];
      sprintf(idx_string, "%03d", frame_idx);
      string motion_file_name = g_isp_pattern_path + g_FileName + "/" + "MotionStatus_PostSP_" + idx_string + ".ppm"; 
      sigdump_open_ctx_bin_r(fr_motion, motion_file_name);
      // skip unused lines.
      for (int i = 0; i < 4; i++)
      {
        if(fgets(temp_char, 32, fr_motion->fp)== NULL)
          printf("[SmartEnc][Error] Failed to read ISP motion status");
      }

      if (fread(data, sizeof(unsigned char), blk_4_num, fr_motion->fp) != blk_4_num)
        printf("[SmartEnc][Error] Failed to read ISP motion status\n");

      sigdump_safe_close(fr_motion);
#ifdef SIG_SMT
    }
    else
    {
      for (int i = 0; i < blk_4_num; i++)
        data[i] = cvi_random_range(0, 2);

      int blk16_num = (cvi_mem_align(frame_width, 16) * cvi_mem_align(frame_height, 16)) >> 8;
      int skip_num = blk16_num >> 4;
      int stride = cvi_mem_align(frame_width, 4) >> 2;
      int height_in_4 = cvi_mem_align(frame_height, 4) >> 2;

      for (int i = 0; i < skip_num; i++)
      {
        int center_x = (cvi_random_range(0, frame_width) >> 4) << 4;
        int center_y = (cvi_random_range(0, frame_height) >> 4) << 4;

        int start_x = max(0, center_x - 16) >> 2;
        int start_y = max(0, center_y - 16) >> 2;
        int end_x = min(start_x + 12, stride);
        int end_y = min(start_y + 12, height_in_4);

        for (int y = start_y; y < end_y; y++)
        {
          for (int x = start_x; x < end_x; x++)
          {
            data[y * stride + x] = 0;
          }
        }
      }
    }
#endif

    for (int y = 0; y < frame_height; y += 16)
    {
      int blk_16_y = y / 4;
      for (int x = 0; x < frame_width; x += 16)
      {
        int still_data = 0, trans_data = 0, motion_data = 0;
        int skip = 0;
        int qp = 0;
#ifdef SIG_SMT
        int isp_qp = 0;
#endif
        int blk_16_x = x / 4;
        int blk_motionmap_num = 0;

        for (int n = 0; n < 4; n++)
        {
          for (int m = 0; m < 4; m++)
          {
            int isp_data = data[(blk_16_y + n) * blk_4_x_num + (blk_16_x + m)];
            int blk_motionmap_x = x + m * 4;
            int blk_motionmap_y = y + n * 4;

            if ((blk_motionmap_x >= frame_width || blk_motionmap_y >= frame_height) || (isp_data == 0))
              still_data += 1;
            else if (isp_data == 1)
              trans_data += 1;
            else if (isp_data == 2)
              motion_data += 1;

            blk_motionmap_num += 1;
          }
        }

        int ai_delta_qp = 0;
        QP_MAP_DELTA_MODE mode1;
        getQpMapBlk16(x, y, &skip, &mode1, &ai_delta_qp);

        if(g_algo_cfg.EnableDeBreath == 2)
        {
          if(p_isp_param->slice_type != I_SLICE && frame_idx >=p_isp_param->intra_period)
          {
            int still_area = still_data + trans_data;
            int motion_area = motion_data;

            qp = (float)diff_qp*((float)motion_area/16) - (float)diff_qp*((float)still_area/16);
            qp = Clip3(-3, 3, qp);
            qp = qp + ai_delta_qp;
            int map = (((skip & 0x1) << 7) | (qp & 0x7f));
            g_blk16_qp_map.set_data(x, y, map);
          }
        }
        if(g_algo_cfg.EnableSmartEncISP)
        {
          if (still_data != blk_motionmap_num)
          {
            int trans_area = trans_data + still_data;
            int motion_area = motion_data;

            qp = (TransQP * trans_area + MotionQP * motion_area) >> 4;

#ifdef SIG_SMT
            isp_qp = qp;
#endif
            if((ai_delta_qp < 0 && qp < 0) || (ai_delta_qp > 0 && qp > 0))
              qp = (qp + ai_delta_qp) >> 1;
            else
              qp = qp + ai_delta_qp;
          }
          else
          {
            if(g_algo_cfg.EnableSmartEncISPSkipMap)
            {
              skip = 1;
              still_ori.set_data(x, y, skip);
            }
            else
            {
              skip = 0;
            }
            qp = ai_delta_qp;
          }
          int map = (((skip & 0x1) << 7) | (qp & 0x7f));
          g_blk16_qp_map.set_data(x, y, map);
          p_isp_param->map_dqp_accum += qp;

#ifdef SIG_SMT
          if (g_sigdump.smt)
            sig_smt_isp_save_blk_data(x, y, skip, still_data, trans_data, motion_data, isp_qp);
#endif
        }
      }
    }

    if(g_algo_cfg.EnableSmartEncISPSkipMap)
    {
      for (int y = 0; y < frame_height; y += 16) {
        for (int x = 0; x < frame_width; x += 16) {

          QP_MAP_DELTA_MODE mode1;
          int delta_qp = 0;
          int skip_cur = still_ori.get_data(x, y);
          int ctu_size = 64;
          int ctu_y_num = y >> 6;
          int ctu_x_num = x >> 6;
          int CTU_upper_boundary = ctu_y_num*ctu_size;
          int CTU_lower_boundary = (ctu_y_num + 1)*ctu_size;
          int CTU_left_boundary = ctu_x_num*ctu_size;
          int CTU_right_boundary = (ctu_x_num+ 1)*ctu_size;
          int skip_num = 0;
          if(skip_cur == 0)
            continue;

          for (int nb_y = (y - 16); nb_y <= y + 16; nb_y += 16) {
            for (int nb_x = (x - 16); nb_x <= x + 16; nb_x += 16) {

              if (nb_x < 0 || nb_y < 0 || nb_x >= frame_width || nb_y >= frame_height)
                continue;

              if(nb_x < CTU_right_boundary && nb_x >= CTU_left_boundary && nb_y < CTU_lower_boundary && nb_y >= CTU_upper_boundary)
              {
                int skip_nb = still_ori.get_data(nb_x, nb_y);
                if(skip_nb == 0)
                {
                  skip_num++;
                }
              }
            }
          }
          if(skip_num !=0)
          {
            int skip_tmp = 0;
            skip_cur = 0;
            getQpMapBlk16(x, y, &skip_tmp, &mode1, &delta_qp);
            int map = (((skip_cur & 0x1) << 7) | (delta_qp & 0x7f));
            g_blk16_qp_map.set_data(x, y, map);
          }

#ifdef SIG_SMT
          if (g_sigdump.smt)
            sig_smt_isp_save_blk_skip(x, y, skip_cur);
#endif
        }
      }
    }

#ifdef SIG_SMT
  if (g_sigdump.smt)
  {
    sig_smt_isp_src(data, frame_width, frame_height);
    sig_smt_isp_gld(frame_width, frame_height);
  }
  else if (g_sigdump.fpga)
  {
    sig_smt_isp_src(data, frame_width, frame_height);
  }
#endif

    clearQpMapBoundarySkip(frame_width, frame_height);
    free(data);
    still_ori.destroy();
}

void frame_clip_slice_qp_bound(int frame_width, int frame_height)
{
  int clip_delta_qp_max = g_clip_delta_qp_max;
  int clip_delta_qp_min = g_clip_delta_qp_min;
  cvi_grid_data<int> *p_qp_map = &g_blk16_qp_map;
  int skip = 0;
  int delta_qp = 0;

  for (int y = 0; y < frame_height; y += 16)
  {
    for (int x = 0; x < frame_width; x += 16)
    {
      getQpMapBlk16(x, y, &skip, NULL, &delta_qp);
      delta_qp = Clip3(clip_delta_qp_min, clip_delta_qp_max, delta_qp);
      int map = (((skip & 0x1) << 7) | (delta_qp & 0x7f));
      p_qp_map->set_data(x, y, map);
    }
  }
}
#endif //~CVI_SMART_ENC
