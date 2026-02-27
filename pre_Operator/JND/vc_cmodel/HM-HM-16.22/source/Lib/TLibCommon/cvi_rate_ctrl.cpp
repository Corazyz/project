#include "cvi_rate_ctrl.h"
#include "cvi_sigdump.h"

// --- picture level rate control ---
// bit stability statistic
int g_RCDebugLogEn = 1;
int g_RCStatsBeginPicIdx = 50;
int g_RCStatsIPicCnt = 0;
int g_RCStatsPPicCnt = 0;
double g_RCStatsIPicBitErrAccum = 0;
double g_RCStatsPPicBitErrAccum = 0;
double g_RCStatsIPicBitErrMax = -1;
double g_RCStatsPPicBitErrMax = -1;

// cfg setting
bool g_RCFastConvergeEn = true;
int g_RCMdlUpdatType = 1;
int g_RCMaxIprop, g_RCMinIprop;
int g_RCMaxQp, g_RCMinQp;
int g_RCMaxIQp, g_RCMinIQp;
int g_CtuRowNormScale;
int g_StatTime = 2; // in second
int g_TcQpTuneEn, g_LumQpTuneEn;
int g_qp_mode = AVG;
bool g_random_rc = false;

RC_Float g_picAvgTc_delay = 12;
RC_Float g_RCMinMadi = 1;

// --- ctu row level rate control ---
int g_RowQpClip = 2;
int g_InRowOverflowQpdelta = 1;

cvi_grid_data<int> g_blk_qp, g_cu32_qp;
cvi_grid_data<double> g_cu32_lambda, g_cu32_sqrt_lambda;
int g_blk_delta_qp_accum = 0;

int getRcMaxQp(SliceType slice_type) { return (slice_type == I_SLICE) ? g_RCMaxIQp : g_RCMaxQp; }
int getRcMinQp(SliceType slice_type) { return (slice_type == I_SLICE) ? g_RCMinIQp : g_RCMinQp; }
int getCtuRowNormScale() { return g_CtuRowNormScale; }
int getBitErrCompenScal() { return g_bit_err_compen_scal; }
int getRowQpClip() { return g_RowQpClip; }
int getInRowOverflowQpdelta() { return g_InRowOverflowQpdelta; }
int getQpMode() { return (int)(g_qp_mode); }

unsigned char g_qp_win_en = 0;
unsigned char g_qp_win_count = 0;
unsigned char getQpWinEn() { return g_qp_win_en; }
unsigned char getQpWinCount() { return g_qp_win_count; }

#ifdef CVI_QP_MAP
int g_QpMapEn;
std::string g_QpMapFileName;
cvi_grid_data<int> g_blk16_qp_map;
QP_ROI g_qp_roi_cfg[QP_ROI_LIST_SIZE];
bool isQpMapEnable() { return ((g_QpMapEn > 0) ? true : false); }
int getQpMapMode() { return g_QpMapEn; }

void verifyQpRoiCfg(int frame_width, int frame_height, int frame_width_align16, int frame_height_align16)
{
  g_qp_win_en = 0;
  g_qp_win_count = 0;
  for (int i = 0; i < QP_ROI_LIST_SIZE; i ++)
  {
    bool is_invalid = false;
    if (g_qp_roi_cfg[i].is_enable == false)
      continue;

    if (g_qp_roi_cfg[i].x < 0 || g_qp_roi_cfg[i].x >= frame_width)
    {
      is_invalid = true;
      printf("[QpROICfg][ERROR] idx = %d, x = %d error\n", i, g_qp_roi_cfg[i].x);
    }
    if (g_qp_roi_cfg[i].y < 0 || g_qp_roi_cfg[i].y >= frame_height)
    {
      is_invalid = true;
      printf("[QpROICfg][ERROR] idx = %d, y = %d error\n", i, g_qp_roi_cfg[i].y);
    }
    if (g_qp_roi_cfg[i].width < 0 || g_qp_roi_cfg[i].x + g_qp_roi_cfg[i].width > frame_width_align16)
    {
      is_invalid = true;
      printf("[QpROICfg][ERROR] idx = %d, x = %d, width = %d error\n", i, g_qp_roi_cfg[i].x, g_qp_roi_cfg[i].width);
    }
    if (g_qp_roi_cfg[i].height < 0 || g_qp_roi_cfg[i].y + g_qp_roi_cfg[i].height > frame_height_align16)
    {
      is_invalid = true;
      printf("[QpROICfg][ERROR] idx = %d, y = %d, height = %d error\n", i, g_qp_roi_cfg[i].y, g_qp_roi_cfg[i].height);
    }
    if (g_qp_roi_cfg[i].skip < 0 || g_qp_roi_cfg[i].skip > 1)
    {
      is_invalid = true;
      printf("[QpROICfg][ERROR] idx = %d, skip = %d error\n", i, g_qp_roi_cfg[i].skip);
    }
    if (g_qp_roi_cfg[i].mode != QP_MAP_RELATIVE && g_qp_roi_cfg[i].mode != QP_MAP_ABSOLUTE)
    {
      is_invalid = true;
      printf("[QpROICfg][ERROR] idx = %d, mode = %d error\n", i, (int)(g_qp_roi_cfg[i].mode));
    }

    if (g_qp_roi_cfg[i].mode == QP_MAP_RELATIVE)
    {
      if (g_qp_roi_cfg[i].qp < -32 || g_qp_roi_cfg[i].qp > 31)
      {
        is_invalid = true;
        printf("[QpROICfg][ERROR] idx = %d, mode = %d, qp = %d error\n", i, (int)(g_qp_roi_cfg[i].mode), g_qp_roi_cfg[i].qp);
      }
    }
    else if (g_qp_roi_cfg[i].mode == QP_MAP_ABSOLUTE)
    {
      if (g_qp_roi_cfg[i].qp < 0 || g_qp_roi_cfg[i].qp > 51)
      {
        is_invalid = true;
        printf("[QpROICfg][ERROR] idx = %d, mode = %d, qp = %d error\n", i, (int)(g_qp_roi_cfg[i].mode), g_qp_roi_cfg[i].qp);
      }
    }

    if (is_invalid)
    {
      g_qp_roi_cfg[i].is_enable = false;
    }
    else
    {
      g_qp_win_en |= (1 << i);
      g_qp_win_count++;
    }
  }
}

void alignQpRoi()
{
  for (int i = 0; i < QP_ROI_LIST_SIZE; i ++)
  {
    if (g_qp_roi_cfg[i].is_enable == false)
      continue;

    int start_x = (g_qp_roi_cfg[i].x >> 4) << 4;
    int start_y = (g_qp_roi_cfg[i].y >> 4) << 4;
    int end_x = ((g_qp_roi_cfg[i].x + g_qp_roi_cfg[i].width + 15) >> 4) << 4;
    int end_y = ((g_qp_roi_cfg[i].y + g_qp_roi_cfg[i].height + 15) >> 4) << 4;

    g_qp_roi_cfg[i].x = start_x;
    g_qp_roi_cfg[i].y = start_y;
    g_qp_roi_cfg[i].width = end_x - start_x;
    g_qp_roi_cfg[i].height = end_y - start_y;
  }
}

bool parseQpRoiCfg(int frame_width, int frame_height)
{
  FILE *p_roi_fp = fopen(g_QpMapFileName.c_str(), "rt");

  if (p_roi_fp == NULL)
  {
    printf("[ParseQpROI][ERROR] open %s fail\n", g_QpMapFileName.c_str());
    return false;
  }

  char type[512] = { 0 };
  QP_ROI *p_target_roi = &g_qp_roi_cfg[0];
  memset(g_qp_roi_cfg, 0, sizeof(g_qp_roi_cfg));
  int frame_width_align16 = ((frame_width + 15) >> 4) << 4;
  int frame_height_align16 = ((frame_height + 15) >> 4) << 4;

  while (1)
  {
    if (fscanf(p_roi_fp, "%s", type) != 1)
    {
      break;
    }

    if (!strcmp (type, "EOF"))
    {
      break;
    }
    else if (!strcmp (type, "#"))
    {
      // change line to skip cmd example
      if (fscanf(p_roi_fp, "%[^\n]", type) != 1)
      {
        break;
      }
    }
    else if (!strcmp (type, "index"))
    {
      int index = 0;
      if (fscanf(p_roi_fp, "%d", &index) != 1)
        break;
      if (index < 0 || index >= 8)
      {
        printf("[ParseQpROI][ERROR] ROI index %d error\n", index);
        break;
      }

      p_target_roi = &g_qp_roi_cfg[index];
      memset(p_target_roi, 0, sizeof(QP_ROI));
      p_target_roi->is_enable = true;
    }
    else if (!strcmp (type, "roi_x"))
    {
      int roi_x = 0;
      if (fscanf(p_roi_fp, "%d", &roi_x) != 1)
        break;

      p_target_roi->x = roi_x;
    }
    else if (!strcmp (type, "roi_y"))
    {
      int roi_y = 0;
      if (fscanf(p_roi_fp, "%d", &roi_y) != 1)
        break;

      p_target_roi->y = roi_y;
    }
    else if (!strcmp (type, "roi_w"))
    {
      int roi_w = 0;
      if (fscanf(p_roi_fp, "%d", &roi_w) != 1)
        break;

      p_target_roi->width = roi_w;
    }
    else if (!strcmp (type, "roi_h"))
    {
      int roi_h = 0;
      if (fscanf(p_roi_fp, "%d", &roi_h) != 1)
        break;

      p_target_roi->height = roi_h;
    }
    else if (!strcmp (type, "skip"))
    {
      int skip = 0;
      if (fscanf(p_roi_fp, "%d", &skip) != 1)
        break;

      p_target_roi->skip = (getQpMapMode() == QP_MAP_MODE_ROI) ? 0 : skip;
    }
    else if (!strcmp (type, "mode"))
    {
      int mode = 0;
      if (fscanf(p_roi_fp, "%d", &mode) != 1)
        break;

      p_target_roi->mode = (QP_MAP_DELTA_MODE)(mode);
    }
    else if (!strcmp (type, "qp"))
    {
      int qp = 0;
      if (fscanf(p_roi_fp, "%d", &qp) != 1)
        break;

      p_target_roi->qp = qp;
    }
  }

  verifyQpRoiCfg(frame_width, frame_height, frame_width_align16, frame_height_align16);
  alignQpRoi(); // align roi(x,y,w,h) to 16.

  if (p_roi_fp)
    fclose(p_roi_fp);

  return true;
}

#define QP_MAP_INIT             0
#define QP_MAP_SKIP_OFFSET      7
#define QP_MAP_BIT_MODE_OFFSET  6
#define QP_MAP_BIT_QP_OFFSET    0

int packQpMapByte(int skip, int mode, int qp)
{
  int info = ((skip & 0x01) << QP_MAP_SKIP_OFFSET) |
             ((mode & 0x01) << QP_MAP_BIT_MODE_OFFSET) |
             ((qp   & 0x3f) << QP_MAP_BIT_QP_OFFSET);
  return info;
}

void unpackQpMapByte(int map, int *p_skip, QP_MAP_DELTA_MODE *p_mode, int *p_qp)
{
  *p_skip = (map >> QP_MAP_SKIP_OFFSET) & 0x01;
  *p_mode = (QP_MAP_DELTA_MODE)((map >> QP_MAP_BIT_MODE_OFFSET) & 0x01);
  *p_qp = (map >> QP_MAP_BIT_QP_OFFSET) & 0x3f;
  if (*p_mode == QP_MAP_RELATIVE)
  {
    if ((*p_qp & 0x20) > 0)
      *p_qp |= 0xffffffc0;
  }
}

void unpackQpMapByteAI(int map, int *p_skip, int *p_qp)
{
  *p_skip = (map >> QP_MAP_SKIP_OFFSET) & 0x01;
  *p_qp = map & 0x7f;

  if ((*p_qp & 0x40) > 0)
    *p_qp |= 0xffffff80;
}

void setQpRoi(int x, int y, int width, int height, int skip, QP_MAP_DELTA_MODE mode, int qp)
{
  int map = packQpMapByte(skip, mode, qp);
  int start_x = (x >> 4) << 4;
  int start_y = (y >> 4) << 4;
  int end_x = cvi_mem_align(x + width, 16);
  int end_y = cvi_mem_align(y + height, 16);

  for (int blk_y = start_y; blk_y < end_y; blk_y += 16)
  {
    for (int blk_x = start_x; blk_x < end_x; blk_x += 16)
      g_blk16_qp_map.set_data(blk_x, blk_y, map);
  }
}

// If frame w/h is not 16-aligned, set skip flag in boundary to be 0.
void clearQpMapBoundarySkip(int frame_width, int frame_height)
{
  if (!(getQpMapMode() == 1 || isEnableSmartEncode()))
    return;

  int map = 0;
  if ((frame_width & 0xf) != 0)
  {
    int x = (frame_width >> 4) << 4;
    for (int y = 0; y < frame_height; y += 16)
    {
      map = (g_blk16_qp_map.get_data(x, y) & 0x7f);
      g_blk16_qp_map.set_data(x, y, map);
    }
  }

  if ((frame_height & 0xf) != 0)
  {
    int y = (frame_height >> 4) << 4;
    for (int x = 0; x < frame_width; x += 16)
    {
      map = (g_blk16_qp_map.get_data(x, y) & 0x7f);
      g_blk16_qp_map.set_data(x, y, map);
    }
  }
}

void setFrameQpMap(int frame_width, int frame_height)
{
  g_blk16_qp_map.set_all(QP_MAP_INIT);

  for (int i = 0; i < QP_ROI_LIST_SIZE; i++)
  {
    if (g_qp_roi_cfg[i].is_enable == true)
    {
      setQpRoi(g_qp_roi_cfg[i].x, g_qp_roi_cfg[i].y,
               g_qp_roi_cfg[i].width, g_qp_roi_cfg[i].height,
               g_qp_roi_cfg[i].skip, g_qp_roi_cfg[i].mode, g_qp_roi_cfg[i].qp);

      printf("[QP ROI][%d] (x, y, w, h) = (%d, %d, %d, %d)\n", i,
             g_qp_roi_cfg[i].x, g_qp_roi_cfg[i].y,
             g_qp_roi_cfg[i].width, g_qp_roi_cfg[i].height);
      printf("[QP ROI][%d] skip = %d, mode = %d, qp = %d\n", i,
             g_qp_roi_cfg[i].skip, g_qp_roi_cfg[i].mode, g_qp_roi_cfg[i].qp);
    }
  }

  clearQpMapBoundarySkip(frame_width, frame_height);
}

#ifdef CVI_RANDOM_ENCODE
void setRandomFrameQpMap(int frame_width, int frame_height)
{
  g_blk16_qp_map.set_all(QP_MAP_INIT);

  int map_size = g_blk16_qp_map.get_size();
  for (int i = 0; i < map_size; i++)
  {
    int skip = cvi_random_equal();
    int mode = cvi_random_equal();
    int qp = (QP_MAP_DELTA_MODE)(mode) == QP_MAP_ABSOLUTE ? cvi_random_range(0, 51) : cvi_random_range(-32, 31);
    int map = packQpMapByte(skip, mode, qp);
    g_blk16_qp_map.set_data(i, map);
  }

  clearQpMapBoundarySkip(frame_width, frame_height);
}

void setRandomQpMapCfg()
{
  int roi_count = cvi_random_range(1, QP_ROI_LIST_SIZE);
  int width = g_sigpool.width;
  int height = g_sigpool.height;
  memset(g_qp_roi_cfg, 0, sizeof(g_qp_roi_cfg));

  for (int i = 0; i < roi_count; i++)
  {
    int roi_w = cvi_random_range(16, width);
    int roi_h = cvi_random_range(16, height);
    int roi_x = cvi_random_range(0, width - roi_w);
    int roi_y = cvi_random_range(0, height - roi_h);

    g_qp_roi_cfg[i].is_enable = true;
    g_qp_roi_cfg[i].x = roi_x;
    g_qp_roi_cfg[i].y = roi_y;
    g_qp_roi_cfg[i].width  = roi_w;
    g_qp_roi_cfg[i].height = roi_h;
    g_qp_roi_cfg[i].skip = 0; // ROI not support
    g_qp_roi_cfg[i].mode = (QP_MAP_DELTA_MODE)(cvi_random_equal());

    if (g_qp_roi_cfg[i].mode == 0)
      g_qp_roi_cfg[i].qp = cvi_random_range(-32, 31);
    else if (g_qp_roi_cfg[i].mode == 1)
      g_qp_roi_cfg[i].qp = cvi_random_range(0, 51);
  }

  int width_align16 = ((width + 15) >> 4) << 4;
  int height_align16 = ((height + 15) >> 4) << 4;
  verifyQpRoiCfg(width, height, width_align16, height_align16);
  alignQpRoi(); // align roi(x,y,w,h) to 16.
}
#endif //~CVI_RANDOM_ENCODE

void getQpMapBlk16(int x, int y, int *p_skip, QP_MAP_DELTA_MODE *p_mode, int *p_qp)
{
  int map = g_blk16_qp_map.get_data(x, y);
  if (isEnableSmartEncode())
    unpackQpMapByteAI(map, p_skip, p_qp);
  else
    unpackQpMapByte(map, p_skip, p_mode, p_qp);
}

void setQpMapBlk16(int x, int y, int map)
{
  g_blk16_qp_map.set_data(x, y, map);
}

ForceDecision checkQpMapCuDecision(int cu_x, int cu_y, int cu_width)
{
  ForceDecision decision = FORCE_BYPASS;

  if (cu_width == 32)
  {
    int is_skip = 0;
    int total_skip = 0;
    int temp = 0;

    for (int y = cu_y; y < cu_y + 32; y += 16)
    {
      for (int x = cu_x; x < cu_x + 32; x += 16)
      {
        getQpMapBlk16(x, y, &is_skip, (QP_MAP_DELTA_MODE *)(&temp), &temp);
        total_skip += is_skip;
      }
    }

    if (total_skip == 4)
// aka marked skip 20240906
      if(g_algo_cfg.DisableCU32)
        decision = FORCE_BYPASS;
      else
        decision = FORCE_KEEP;
// aka marked skip 20240906~
    else if (total_skip > 0)
      decision = FORCE_CHANGE;
  }
  else if (cu_width == 16)
  {
    int is_skip = 0;
    int temp = 0;
    getQpMapBlk16(cu_x, cu_y, &is_skip, (QP_MAP_DELTA_MODE *)(&temp), &temp);

    if (is_skip == 1)
      decision = FORCE_KEEP;
  }

  return decision;
}

#endif //~CVI_QP_MAP

// -------------- HW rate control unit --------------
hw_lut g_row_q_lut; // input: bit per ctu, output: row base qp
hw_lut g_tc_lut;    // input: tc(madi), output: tc delta qp
hw_lut g_lum_lut;   // input: luminance(blk_avg), output: lum delta qp
int g_bit_error_accum = 0;
int g_bit_err_smooth_factor;
int g_bit_err_compen_scal = RC_NORM_SCALE_BD / 4;
double g_qp_to_lambda_table[QP_NUM], g_qp_to_sqrt_lambda_table[QP_NUM];
bool g_updata_stat_frm_en;
int g_qp_hist[QP_NUM];
int g_stat_frm_max_qp;
int g_stat_frm_min_qp;
int g_stat_frm_qp_sum;
const int STAT_CU_IDX[3][4] = { { 0,  1,  2, -1},
                                {-1,  9, 10, 11},
                                {-1, 16, 17, 18} };
int g_stat_cu_count[STAT_CU_IDX_TOTAL];
int g_TcThrd[TC_LUT_SIZE] = {0, 0, 0, 0, 3, 3, 5, 5, 8, 8, 8, 15, 15, 20, 25, 25};
int g_LumThrd[LUM_LUT_SIZE] = {30, 100, 150, 175, 200};
int g_LumQpDelta[LUM_LUT_SIZE] = {0, -1, 0, 1, 1};
int g_directionThrd = 8;

#ifdef CVI_FOREGROUND_QP
hw_lut g_fg_tc_lut; // foreground tc lut
// g_FgTcThrd needs to be optimized
int g_FgTcThrd[TC_LUT_SIZE] = {4, 5, 6, 6, 7, 7, 8, 8, 8, 8, 8, 15, 15, 20, 25, 25};
#endif
#ifdef CVI_RANDOM_ENCODE
bool g_is_cvi_rand_rc_en = false;
int g_tc_qp_delta_rand[TC_LUT_SIZE+1] = { 0 };
double g_qp_to_lambda_table_rand[QP_NUM] = { 0.0 };
double g_qp_to_sqrt_lambda_table_rand[QP_NUM] = { 0.0 };
#endif

void hw_lut:: lut_init(int entry_num, lut_mode mode)
{
  m_entry_num = entry_num;
  m_mode = mode;
  m_center_idx = entry_num>>1;
  m_input = new int[entry_num]();
  m_output = new int[(mode==INTERVAL) ? entry_num+1 : entry_num]();
}

int hw_lut:: lut_lookup(int target)
{
  int target_entry = -1;
  if(m_mode==NEAREST) {
    int min_diff = MAX_INT;
    for(int entry_i=0; entry_i<m_entry_num; entry_i++) {
      int diff = abs(target - m_input[entry_i]);
      if((diff < min_diff) ||
         ((diff==min_diff) && (abs(entry_i-m_center_idx) < abs(target_entry-m_center_idx)))) {
        target_entry = entry_i;
        min_diff = diff;
      }
    }
    assert(target_entry >= 0 && target_entry < m_entry_num);
  }
  else { // INTERVAL
    if(m_input[m_center_idx-1]<=target && target<=m_input[m_center_idx]) {
      target_entry = m_center_idx;
    }
    else if(target<=m_input[m_center_idx]) {
      target_entry = 0;
      for(int entry_i=m_center_idx-2; entry_i>=0; entry_i--) {
        if(m_input[entry_i]<=target && target<m_input[entry_i+1]) {
          target_entry = entry_i + 1;
          break;
        }
      }
    }
    else {
      target_entry = m_entry_num;
      for(int entry_i=m_center_idx; entry_i<(m_entry_num-1); entry_i++) {
        if(m_input[entry_i]<target && target<=m_input[entry_i+1]) {
          target_entry = entry_i + 1;
          break;
        }
      }
    }
    assert(target_entry >= 0 && target_entry <= m_entry_num);
  }
  return m_output[target_entry];
}
// ------------------------------------------------------------

void cal8x8MadiAndLum(Short *p_srcBase, int stride, int *p_iMad, int *p_pLum)
{
  int lum = blkSum<Short, int>(p_srcBase, 8, 8, stride)>>6;
  *p_pLum = lum;
  int madi = 0;
  Short *srcPtr = p_srcBase;
  for(int y=0; y<8; y++)
  {
    for(int x=0; x<8; x++)
    {
      madi += abs(lum - srcPtr[x]);
      // [next change] configuralbe madi to distinguish strong edge and texture
      // madi += min(abs(lum - srcPtr[x]), T);
    }
    srcPtr += stride;
  }
  *p_iMad = madi>>6;
}

// -------------------------------------------------------
void rc_setPicAvgTc(int picTcSum, int picPelNum)
{
  g_picAvgTc_delay = std::max(g_RCMinMadi, (((RC_Float) picTcSum*(RC_Float)1.6 * (RC_Float)256) / ((RC_Float)picPelNum)));
}
// -------------------------------------------------------
void rc_update_pic_bit_stats(int targetBit, int actBit, int picIdx, int picType)
{
  double bitErrPercentage = abs(targetBit - actBit) / (double)targetBit;
  printf("\n%d, %d, %.3f", targetBit, actBit, bitErrPercentage);
  if(picIdx < g_RCStatsBeginPicIdx) return;
  if(picType==I_SLICE)
  {
    g_RCStatsIPicBitErrAccum += bitErrPercentage;
    if(bitErrPercentage>g_RCStatsIPicBitErrMax){
      g_RCStatsIPicBitErrMax = bitErrPercentage;
    }
    g_RCStatsIPicCnt++;
  }
  else
  {
    g_RCStatsPPicBitErrAccum += bitErrPercentage;
    if(bitErrPercentage > g_RCStatsPPicBitErrMax){
      g_RCStatsPPicBitErrMax = bitErrPercentage;
    }
    g_RCStatsPPicCnt++;
  }
}

void rc_show_pic_bit_stats()
{
  if(g_RCStatsIPicCnt > 0){
    printf( "\n\nI Picture Bit Error --------------------------------------------\n" );
    printf("avg, %.2f, max, %.2f\n", g_RCStatsIPicBitErrAccum/g_RCStatsIPicCnt, g_RCStatsIPicBitErrMax);
  }
  if(g_RCStatsPPicCnt > 0){
    printf( "  \nP Picture Bit Error --------------------------------------------\n" );
    printf("avg, %.2f, max, %.2f\n", g_RCStatsPPicBitErrAccum/g_RCStatsPPicCnt, g_RCStatsPPicBitErrMax);
  }
}

void update_frm_statistic(TComDataCU *pcCU, int uiAbsPartIdx, int cu_x, int cu_y, int cu_w, bool is_rc_en)
{
  // QP
  if ((cu_x & 0xf) == 0 && (cu_y & 0xf) == 0)
  {
    int enc_qp = pcCU->getQP(uiAbsPartIdx);
    if (is_rc_en)
    {
      int pos_x = cu_x & 0x3f;
      int pos_y = cu_y & 0x3f;
      enc_qp = (cu_w == 32) ? g_cu32_qp.get_data(pos_x, pos_y) : g_blk_qp.get_data(pos_x, pos_y);
    }

    if (cu_w == 32)
    {
      g_qp_hist[enc_qp]+=4;
      g_stat_frm_qp_sum += (enc_qp << 2);
    }
    else
    {
      g_qp_hist[enc_qp]++;
      g_stat_frm_qp_sum += enc_qp;
    }

    if (g_stat_frm_max_qp < enc_qp)
      g_stat_frm_max_qp = enc_qp;

    if (g_stat_frm_min_qp > enc_qp)
      g_stat_frm_min_qp = enc_qp;
  }

  // CU count
  int mode = pcCU->isSkipped(uiAbsPartIdx) ? 2 : ((int)(pcCU->getPredictionMode(uiAbsPartIdx)) + 1) & 0x1;
  int pu_w = (mode == 0 && (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_NxN)) ? 4 : cu_w;
  int size = (pu_w == 4)  ? 0 :
             (pu_w == 8)  ? 1 :
             (pu_w == 16) ? 2 : 3;
  int idx = STAT_CU_IDX[mode][size];
  g_stat_cu_count[idx]++;
}

void reset_frm_statistic()
{
  memset(g_qp_hist, 0, sizeof(g_qp_hist));
  g_stat_frm_max_qp = 0;
  g_stat_frm_min_qp = 51;
  g_stat_frm_qp_sum = 0;
  memset(g_stat_cu_count, 0, sizeof(g_stat_cu_count));
  g_pic_madp_accum = 0;
  g_blk_madp.set_all(0);
}
