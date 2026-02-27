#include "cvi_motion.h"
#include "cvi_sigdump.h"
#include "cvi_rate_ctrl.h"

void *g_pPrevPicYuvRec = NULL;

double g_mvGain = 2.;
double g_motAlpha = 0.5;
double g_sadGain = 1.0;
double g_mdMadiGain = 0.5;
int g_mdBias = 12;
double g_nrMadiGain = 1.25;
int g_nrBias = -2;

cvi_grid_data<UChar> g_alpha_map;
cvi_grid_data<UChar> g_motion_map[3]; // 0:blk16, 1:blk32, 2:blk8
int g_pic_motion_level = 0;

cvi_grid_data<double> g_foreground_map[3];  // 0:blk16, 1:blk32, 2:blk8
int g_pic_foreground_cnt = 0;


#ifdef CVI_FOREGROUND_QP
cvi_grid_data<UChar> g_fg_dqp_map;    // 16x16
bool g_is_foreground_qp_valid = false;
void set_foreground_qp_valid(bool is_valid) { g_is_foreground_qp_valid = is_valid; }
bool get_foreground_qp_valid() { return g_is_foreground_qp_valid; }
#endif

int get_fix_point_mv_gain()
{
  return Clip3(0, MV_SAD_GAIN_MAX, (int)(g_mvGain * (1 << MV_SAD_GAIN_FRAC_BIT)));
}

int get_fix_point_sad_gain()
{
  return Clip3(0, MV_SAD_GAIN_MAX, (int)(g_sadGain * (1 << MV_SAD_GAIN_FRAC_BIT)));
}

void motion_detect_by_mv_and_sad(int cu_x, int cu_y, int sad, short mv_x[], short mv_y[], int mv_num, int size, bool is_first_pic)
{
  short result_x = average(mv_x, mv_num)/4;
  short result_y = average(mv_y, mv_num)/4;

  int fix_point_mv_gain = get_fix_point_mv_gain();
  int fix_point_sad_gain = get_fix_point_sad_gain();
  int mvl = Clip3(0, 255, ((int)((abs(result_x) + abs(result_y)) * fix_point_mv_gain) >> MV_SAD_GAIN_FRAC_BIT));
  int mot = Clip3(0, 255, ((int)(sad * fix_point_sad_gain) >> MV_SAD_GAIN_FRAC_BIT) + mvl);

  if(!is_first_pic) {
    int prev_mot = g_motion_map[0].get_data(cu_x, cu_y);
    if(size==32) {
      // IIR with four blk16 motion
      prev_mot += (g_motion_map[0].get_data(cu_x+16, cu_y) +
                  g_motion_map[0].get_data(cu_x, cu_y+16) +
                  g_motion_map[0].get_data(cu_x+16, cu_y+16));
      prev_mot >>= 2;
    }
    mot = Clip3(0, 255, (int)((mot*g_motAlpha) + (prev_mot*(1.0-g_motAlpha))));
  }
  int map_idx = (size==32) ? 1 : ((size==16) ? 0 : 2);
  g_motion_map[map_idx].set_data(cu_x, cu_y, mot);

  calcForegroundMap(cu_x, cu_y, size);

#ifdef SIG_IRPU
  g_sigpool.me_golden.mv_level = (unsigned char)(mvl);
#endif
}

void motion_fuse_with_cur_blk_diff(Pel* pPrevPtr, Int iWidth, Int iHeight, Int iPrevStride)
{
  for(int y=0; y<iHeight; y+=16)
  {
    for(int x=0; x<iWidth; x+=16)
    {
      int ref_avg = blkSum<Pel, int>(&pPrevPtr[x], 16, 16, iPrevStride)>>8;
      int cur_avg = g_blk_lum.get_data(x, y);
      int blk_diff = abs(cur_avg-ref_avg);
      int mot = g_motion_map[0].get_data(x, y);
      g_motion_map[0].set_data(x, y, max(blk_diff, mot));
    }
    pPrevPtr += (iPrevStride*16);
  }
}

double get_pix_mot_beta(Int in, Int *inNode, Int *outNode)
{
  double beta;
  if(in<inNode[0]) {
    beta = 0.;
  }
  else if(in<inNode[1]){
    beta = (double)(in - inNode[0])*outNode[0]/((inNode[1]-inNode[0]));
  }
  else {
    beta = outNode[0] + (double)((in - inNode[1])*(outNode[1] - outNode[0])/(255-inNode[0]));
  }
  return beta/256.;
}

void temporal_filter_with_ref(Pel* pSrcBase, Pel* pPrevBase, Int iWidth, Int iHeight, Int iSrcStride, Int iPrevStride, SliceType sliceType)
{
  Pel *srcPtr = pSrcBase, *prevPtr = pPrevBase;
  bool blkMotionDeblkEn = false;
  bool pixMotionEn = true;
  bool motionSmoothEn = false;
  double alpha_base[2] = {0.3125, 0.375};
  double alpha_gain[2] = {0.025, 0.025};
  int pixMotInNode[2] = {16, 32};
  int pixMotOutNode[2] = {16, 48};
  Int param_idx = (sliceType==I_SLICE) ? 0 : 1;
  for(int y=0; y<iHeight; y++)
  {
    for(int x=0; x<iWidth; x++)
    {
      int madi = g_blk_madi.get_data(x, y);
      int motCoring = (int)(madi*g_nrMadiGain) + g_nrBias;
      int motion = 0;
      if(blkMotionDeblkEn) {
        // bilinear up-sampling
        int grid_idx_x0 = max(0, ((x - 8) / 16)*16 + 8);
        int grid_idx_y0 = max(0, ((y - 8) / 16)*16 + 8);
        int grid_idx_x1 = min(iWidth-1, ((x + 8) / 16)*16 + 8);
        int grid_idx_y1 = min(iHeight-1, ((y + 8) / 16)*16 + 8);
        int mot_00 = g_motion_map[0].get_data(grid_idx_x0, grid_idx_y0);
        int mot_01 = g_motion_map[0].get_data(grid_idx_x0, grid_idx_y1);
        int mot_10 = g_motion_map[0].get_data(grid_idx_x1, grid_idx_y0);
        int mot_11 = g_motion_map[0].get_data(grid_idx_x1, grid_idx_y1);
        motion += mot_00 * (grid_idx_x1 - x)*(grid_idx_y1 - y);
        motion += mot_10 * (x - grid_idx_x0)*(grid_idx_y1 - y);
        motion += mot_01 * (grid_idx_x1 - x)*(y - grid_idx_y0);
        motion += mot_11 * (x - grid_idx_x0)*(y - grid_idx_y0);
        motion >>= 8;
      }
      else if(motionSmoothEn) {
        // 3x3 low-pass
        for(Int dy=-16; dy<=16; dy+=16) {
          for(Int dx=-16; dx<=16; dx+=16) {
            motion += g_motion_map[0].get_data(Clip3(0, iWidth-1, x + dx), Clip3(0, iHeight-1, y + dy));
          }
        }
        motion = motion/9;
      }
      else {
        motion = g_motion_map[0].get_data(x, y);
      }
      if (pixMotionEn) {
        int pix_diff = abs(srcPtr[x] - prevPtr[x]);
        double beta = get_pix_mot_beta(pix_diff, pixMotInNode, pixMotOutNode);
        motion = (255 * beta) + motion*(1 - beta);
      }
      motion = max(0, motion - motCoring);
      double alpha = min(1.0, (motion*alpha_gain[param_idx]) + alpha_base[param_idx]);
      Pel filtered = srcPtr[x]*alpha + prevPtr[x]*(1-alpha);
      srcPtr[x] = Clip3((Pel)0, (Pel)255, filtered);
      g_alpha_map.set_data(x, y, (unsigned char)(alpha*255));
    }
    prevPtr += iPrevStride;
    srcPtr += iSrcStride;
  }
}

bool is_motion_block(int x, int y)
{
  return g_motion_map[0].get_data(x, y) > ((int)(g_blk_madi.get_data(x, y)*g_mdMadiGain) + g_mdBias);
}

void calc_pic_motion_level(int picWidth, int picHeight)
{
  Int pic_motion_sum = 0;
  for(int y=0; y<picHeight; y+=16)
  {
    for(int x=0; x<picWidth; x+=16)
    {
      pic_motion_sum += is_motion_block(x, y);
    }
  }
  g_pic_motion_level = min(255, (255 * pic_motion_sum*256)/(picWidth*picHeight));
}

#define FOREGROUND_TH_BIAS_BIT  7
#define FOREGROUND_TH_BIAS_MAX  ((1 << FOREGROUND_TH_BIAS_BIT) - 1)
#define FOREGROUND_TH_BIAS_MIN  (-(1 << FOREGROUND_TH_BIAS_BIT))
#define FOREGROUND_TH_OFF_MAX   FOREGROUND_TH_BIAS_MAX
#define FOREGROUND_TH_OFF_MIN   0

int get_fix_point_fg_thr_gain()
{
  return (int)(g_algo_cfg.CostPenaltyCfg.ForegroundThGain * (1 << MADI_GAIN_FRAC_BIT));
}

int get_fix_point_fg_thr_bias()
{
  return g_algo_cfg.CostPenaltyCfg.ForegroundThBias;
}

void calcForegroundMap(int cu_x, int cu_y, int cu_width)
{
  // Madi
  int madi =g_blk_madi.get_data(cu_x, cu_y);
  if (cu_width == 32)
  {
    madi += g_blk_madi.get_data(cu_x + 16, cu_y);
    madi += g_blk_madi.get_data(cu_x, cu_y + 16);
    madi += g_blk_madi.get_data(cu_x + 16, cu_y + 16);
    madi >>= 2;
  }
  else if (cu_width == 8)
  {
    madi = g_blk_madi.get_data(cu_x, cu_y);
  }

  // calc foreground threshold
  double madiGain = g_algo_cfg.CostPenaltyCfg.ForegroundThGain;
  int mot_bias = g_algo_cfg.CostPenaltyCfg.ForegroundThBias;
  mot_bias = Clip3(FOREGROUND_TH_BIAS_MIN, FOREGROUND_TH_BIAS_MAX, mot_bias);

  int fg_frac_bits = MADI_GAIN_FRAC_BIT;
  int fix_point_madiGain = madiGain * (1 << fg_frac_bits);
  int mot_thr = max(0, ((madi * fix_point_madiGain) >> fg_frac_bits) + mot_bias);

  // calc foreground map
  double fg_map = 0.0;
  int idx = (cu_width == 32) ? 1 : ((cu_width == 16) ? 0 : 2);
  int mot = (int)(g_motion_map[idx].get_data(cu_x, cu_y));

  if (g_algo_cfg.CostPenaltyCfg.EnableForegroundHardTh)
  {
    if (mot > mot_thr)
      fg_map = 1.0;
  }
  else
  {
    mot_thr = max(1, mot_thr);  // avoid to divide zero.

    int coring_offset = g_algo_cfg.CostPenaltyCfg.ForegroundThOffset;
    coring_offset = Clip3(FOREGROUND_TH_OFF_MIN, FOREGROUND_TH_OFF_MAX, coring_offset);
    int fg_frac_bits = FOREGROUND_FRAC_BIT;
    int fg_bias = min((1 << fg_frac_bits), ((max(0, mot - coring_offset) * (1 << fg_frac_bits)) / mot_thr));
    fg_map = fg_bias / (double)(1 << fg_frac_bits);
  }
  g_foreground_map[idx].set_data(cu_x, cu_y, fg_map);

  if (cu_width == 16)
  {
    int is_skip = 0;
    if (isQpMapEnable() || isEnableSmartEncode())
    {
      int temp = 0;
      getQpMapBlk16(cu_x, cu_y, &is_skip, (QP_MAP_DELTA_MODE *)(&temp), &temp);
    }

    if (is_skip == 0)
      g_pic_foreground_cnt += (int)(fg_map);
  }
}
