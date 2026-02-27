
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "TEncTop.h"
#include "TEncSlice.h"
#include "TLibCommon/cvi_frm_buf_mgr.h"
#include "TLibCommon/reg_vc_mm.struct.h"
#include "TLibCommon/reg_vc_mem.struct.h"
#include "TLibCommon/reg_vc_hdr.struct.h"
#include "TLibCommon/reg_vc_enc.struct.h"
#include "TLibCommon/reg_vc_stat.struct.h"
#include "TLibCommon/reg_vc_sys.struct.h"
#include "TLibCommon/reg_vbc_sys.struct.h"
#include "TLibCommon/reg_vbd_sys.struct.h"
#include "cvi_enc.h"
#include "cvi_enc_reg.h"
#include "cvi_rate_ctrl.h"
#include "cvi_pattern.h"
#include "cvi_motion.h"
#include "cvi_reg_statistic.h"
#include "cvi_cu_ctrl.h"

using namespace std;

#define PRG_TAB_IDX_QP_MAP_ROI  0
#define PRG_TAB_IDX_TCL_LLUT    4
#define PRG_TAB_IDX_QLLUT       5
#define PRG_TAB_IDX_SI_TAB      8
#define PRG_TAB_IDX_RATIO       9
#define PRG_TAB_IDX_MC_TAB      10
#define PRG_TAB_IDX_ROW_QP      12
#define PRG_TAB_IDX_BG_FG       13
#define PRG_TAB_IDX_DQP_TAB     14
#define PRG_TAB_IDX_OBJ_TAB     15

VC_HDR_C vc_hdr_reg;
VC_MM_C vc_mm_reg;
VC_ENC_C vc_enc_reg;
VC_STAT_C vc_stat_reg;
VC_MEM_C vc_mem_reg;
VC_SYS_C vc_sys_reg;
VBC_SYS_C vbc_sys_reg;
VBD_SYS_C vbd_sys_reg;

void top_reg_init()
{
  init_vc_hdr_reg(vc_hdr_reg);
  init_vc_mm_reg(vc_mm_reg);
  init_vc_enc_reg(vc_enc_reg);
  init_vc_stat_reg(vc_stat_reg);
  init_vc_mem_reg(vc_mem_reg);
  init_vc_sys_reg(vc_sys_reg);
  init_vbc_sys_reg(vbc_sys_reg);
  init_vbd_sys_reg(vbd_sys_reg);
}

void top_vc_enc_prg_tab_write(int idx, int table_addr, UInt value)
{
  vc_enc_reg.VC_ENC_h80.reg_enc_prg_tab_go = 1;
  vc_enc_reg.VC_ENC_h80.reg_enc_prg_tab_rw = 1;
  vc_enc_reg.VC_ENC_h80.reg_enc_prg_tab_idx = idx;
  vc_enc_reg.VC_ENC_h80.reg_enc_prg_tab_addr = table_addr;
  vc_enc_reg.VC_ENC_h84.reg_enc_prg_tab_wd = value;

  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x84, vc_enc_reg.VC_ENC_h84.val);  // 0x84 first
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x80, vc_enc_reg.VC_ENC_h80.val);

  p_ctx = &g_sigpool.top_fw_cmdq_ctx;
  sig_top_reg_cmdq_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x84, vc_enc_reg.VC_ENC_h84.val,0);  // 0x84 first
  sig_top_reg_cmdq_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x80, vc_enc_reg.VC_ENC_h80.val,0);
}

int top_vc_enc_prg_tab_write_pack(int table_idx, int entry_num, int entry_byte, int *p_lut, int start_addr)
{
  if (entry_byte == 0)
    return 0;
  int pack_num = (4 / entry_byte);
  int addr = start_addr;
  int shift = (entry_byte << 3);
  int mask = (entry_byte == 1) ? 0xff : 0xffff;

  for (int i = 0; i < entry_num; i += pack_num)
  {
    uint32_t reg_value = 0;
    for (int j = 0; j < pack_num; j++)
    {
      if (i + j == entry_num)
        break;
      reg_value |= ((p_lut[i + j] & mask) << (j * shift));
    }
    top_vc_enc_prg_tab_write(table_idx, addr, reg_value);
    addr++;
  }

  return addr;
}

UInt top_vc_enc_pack_ratio(UInt high, UInt low)
{
  return (((high & 0xfff) << 16) + (low & 0xfff));
}

void top_vc_enc_prg_tab_write_ratio()
{
  // a0: {inter_L_TU8, inter_L_TU4},
  // a1: {inter_L_TU32, inter_L_TU16}
  // a2: {inter_C_TU8, inter_C_TU4},
  // a3: {N/A, N/A}
  // a4: {intra_L_TU8, intra_L_TU4},
  // a5: {intra_L_TU32, intra_L_TU16}
  // a6: {intra_C_TU8, intra_C_TU4},
  // a7: {N/A, N/A}
  // {High WORD(12bits), Low WORD(12bits)}

  int addr = 0;
  for (int is_intra = 0; is_intra <2; is_intra++)
  {
    UInt high = (UInt)(rru_get_last_pos_scale(g_lastPosBinScale_init[0][is_intra][1]));
    UInt low  = (UInt)(rru_get_last_pos_scale(g_lastPosBinScale_init[0][is_intra][0]));
    UInt reg_value = top_vc_enc_pack_ratio(high, low);
    top_vc_enc_prg_tab_write(PRG_TAB_IDX_RATIO, addr, reg_value);
    addr++;

    high = (UInt)(rru_get_last_pos_scale(g_lastPosBinScale_init[0][is_intra][3]));
    low  = (UInt)(rru_get_last_pos_scale(g_lastPosBinScale_init[0][is_intra][2]));
    reg_value = top_vc_enc_pack_ratio(high, low);
    top_vc_enc_prg_tab_write(PRG_TAB_IDX_RATIO, addr, reg_value);
    addr++;

    high = (UInt)(rru_get_last_pos_scale(g_lastPosBinScale_init[1][is_intra][1]));
    low  = (UInt)(rru_get_last_pos_scale(g_lastPosBinScale_init[1][is_intra][0]));
    reg_value = top_vc_enc_pack_ratio(high, low);
    top_vc_enc_prg_tab_write(PRG_TAB_IDX_RATIO, addr, reg_value);
    addr = 4;
  }
}

void top_set_reg_vc_mm(TComSlice *p_slice)
{
  vc_mm_reg.VC_MM_h10.reg_avc_mode = 0;

  vc_mm_reg.VC_MM_h10.reg_vbc_ena = isEnableVBC() ? 1 : 0;
  vc_mm_reg.VC_MM_h10.reg_src_vbc_ena = isEnableVBD() ? 1 : 0;

  vc_mm_reg.VC_MM_h80.reg_bs_slice_hdr_en = 0;
  // Source Format
  // 0: I420
  // 1: NV12
  // 2: NV21
  // 3: Reserved
  // 4: Reserved
  // 5: Tile32x32
  // 6: Tile32x16
  // 7: Tile16x32
  vc_mm_reg.VC_MM_h80.reg_src_fmt = g_sigdump.input_src == 2 ? 6 : 1;

  vc_mm_reg.VC_MM_h80.reg_src_rotate = getRotateAngleMode();
  if (vc_mm_reg.VC_MM_h80.reg_src_rotate == 0)
  {
    //reg_src_rotate must be 0
    vc_mm_reg.VC_MM_h80.reg_src_h_mirror = g_algo_cfg.EnableMirror == 1 ? 1 : 0;
    vc_mm_reg.VC_MM_h80.reg_src_v_mirror = g_algo_cfg.EnableMirror == 2 ? 1 : 0;
  }

  if (g_algo_cfg.enableCviRGBDebugMode)
  {
    vc_mm_reg.VC_MM_h80.reg_src_rgb_ena = 0;
    vc_mm_reg.VC_MM_h80.reg_src_rgb_mode = 0;
    vc_mm_reg.VC_MM_h80.reg_src_rgb2yuv_sel = 0;
  }
  else
  {
    vc_mm_reg.VC_MM_h80.reg_src_rgb_ena = isEnableRGBConvert() ? 1 : 0;
    vc_mm_reg.VC_MM_h80.reg_src_rgb_mode = g_algo_cfg.RGBFormat;
    vc_mm_reg.VC_MM_h80.reg_src_rgb2yuv_sel = isEnableRGBConvert() ? (g_algo_cfg.EnableRGBConvert - 1) : 0;
  }
  vc_mm_reg.VC_MM_h88.val = get_hw_base_32("src_y", "LSB");
  vc_mm_reg.VC_MM_h8C.val = get_hw_base_32("src_y", "MSB");
  vc_mm_reg.VC_MM_h90.val = get_hw_base_32("src_cb", "LSB");
  vc_mm_reg.VC_MM_h94.val = get_hw_base_32("src_cb", "MSB");
  vc_mm_reg.VC_MM_h98.val = 0;
  vc_mm_reg.VC_MM_h9C.val = 0;

  vc_mm_reg.VC_MM_hA0.reg_src_width_m1   = g_sigpool.width - 1;
  vc_mm_reg.VC_MM_hA0.reg_src_height_m1  = g_sigpool.height - 1;
  vc_mm_reg.VC_MM_hA4.reg_src_luma_pit   = (get_hw_pitch("src_y") >> 4);
  vc_mm_reg.VC_MM_hA4.reg_src_chroma_pit = (get_hw_pitch("src_cb") >> 4);

  // find slice start-end (x, y) align8 by ts addr
  TComPic *p_pic              = p_slice->getPic();
  const TComPicSym *p_pic_sym = p_pic->getPicSym();
  int slice_ctu_rs_start      = p_pic_sym->getCtuTsToRsAddrMap(p_slice->getSliceCurStartCtuTsAddr());
  int slice_ctu_rs_end        = p_pic_sym->getCtuTsToRsAddrMap(p_slice->getSliceCurEndCtuTsAddr());
  int ctu_width               = p_pic_sym->getSPS().getMaxCUWidth();
  int frm_width_in_ctu        = p_pic->getFrameWidthInCtus();
  int frm_width               = p_pic->getPicYuvOrg()->getWidth(COMPONENT_Y);
  int frm_height              = p_pic->getPicYuvOrg()->getHeight(COMPONENT_Y);

  int slice_start_x = (slice_ctu_rs_start % frm_width_in_ctu) * ctu_width;
  int slice_start_y = (slice_ctu_rs_start / frm_width_in_ctu) * ctu_width;
  int slice_end_x   = ((slice_ctu_rs_end - 1) % frm_width_in_ctu) * ctu_width;
  int slice_end_y   = ((slice_ctu_rs_end - 1) / frm_width_in_ctu) * ctu_width;

  slice_start_x = cvi_mem_align(slice_start_x, 8);
  slice_start_y = cvi_mem_align(slice_start_y, 8);
  slice_end_x   = cvi_mem_align(min(slice_end_x + ctu_width, frm_width), 8);
  slice_end_y   = cvi_mem_align(min(slice_end_y + ctu_width, frm_height), 8);

  // minus 1
  vc_mm_reg.VC_MM_hA8.reg_src_st_x8 = (slice_start_x == 0) ? 0 : ((slice_start_x >> 3) - 1);
  vc_mm_reg.VC_MM_hA8.reg_src_st_y8 = (slice_start_x == 0) ? 0 : ((slice_start_x >> 3) - 1);
  vc_mm_reg.VC_MM_hAC.reg_src_end_x8 = (slice_end_x >> 3) - 1;
  vc_mm_reg.VC_MM_hAC.reg_src_end_y8 = (slice_end_y >> 3) - 1;

  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_ctx *p_cmdq_ctx = &g_sigpool.top_fw_cmdq_ctx;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0x10, vc_mm_reg.VC_MM_h10.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0x80, vc_mm_reg.VC_MM_h80.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0x88, vc_mm_reg.VC_MM_h88.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0x8C, vc_mm_reg.VC_MM_h8C.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0x90, vc_mm_reg.VC_MM_h90.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0x94, vc_mm_reg.VC_MM_h94.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0x98, vc_mm_reg.VC_MM_h98.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0x9C, vc_mm_reg.VC_MM_h9C.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xa0, vc_mm_reg.VC_MM_hA0.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xa4, vc_mm_reg.VC_MM_hA4.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xa8, vc_mm_reg.VC_MM_hA8.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xac, vc_mm_reg.VC_MM_hAC.val);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0x10, vc_mm_reg.VC_MM_h10.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0x80, vc_mm_reg.VC_MM_h80.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0x88, vc_mm_reg.VC_MM_h88.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0x8C, vc_mm_reg.VC_MM_h8C.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0x90, vc_mm_reg.VC_MM_h90.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0x94, vc_mm_reg.VC_MM_h94.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0x98, vc_mm_reg.VC_MM_h98.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0x9C, vc_mm_reg.VC_MM_h9C.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xa0, vc_mm_reg.VC_MM_hA0.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xa4, vc_mm_reg.VC_MM_hA4.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xa8, vc_mm_reg.VC_MM_hA8.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xac, vc_mm_reg.VC_MM_hAC.val,0);

  // Set width and height before firing source buffer.
  const TComSPS *p_sps = p_slice->getSPS();
  int min_cu_size = (1 << p_sps->getLog2MinCodingBlockSize());
  vc_hdr_reg.SPS_REG_h14.reg_pic_width_cu_m1 = cvi_mem_align_mult(p_sps->getPicWidthInLumaSamples(), min_cu_size) - 1;
  vc_hdr_reg.SPS_REG_h14.reg_pic_height_cu_m1 = cvi_mem_align_mult(p_sps->getPicHeightInLumaSamples(), min_cu_size) - 1;
  vc_hdr_reg.SPS_REG_h18.reg_pic_width_ctu_m1 = p_pic->getFrameWidthInCtus() - 1;
  vc_hdr_reg.SPS_REG_h18.reg_pic_height_ctu_m1 = p_pic->getFrameHeightInCtus() - 1;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x14, vc_hdr_reg.SPS_REG_h14.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x18, vc_hdr_reg.SPS_REG_h18.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x14, vc_hdr_reg.SPS_REG_h14.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x18, vc_hdr_reg.SPS_REG_h18.val,0);

  // Set VC_ENC_h00 before firing source buffer.
  vc_enc_reg.VC_ENC_h00.reg_enc_force_all_intra = 0;
  vc_enc_reg.VC_ENC_h00.reg_enc_force_all_inter = (p_slice->getSliceType() == P_SLICE) ? g_algo_cfg.DisablePfrmIntra : 0;
  vc_enc_reg.VC_ENC_h00.reg_enc_force_zero_mv   = 0;
  vc_enc_reg.VC_ENC_h00.reg_enc_force_skip      = 0;
  vc_enc_reg.VC_ENC_h00.reg_enc_multi_ref_en    = 0;
  vc_enc_reg.VC_ENC_h00.reg_enc_cons_mv_en      = g_cvi_enc_setting.enableConstrainedMV;
  vc_enc_reg.VC_ENC_h00.reg_enc_cons_mvd_en     = g_cvi_enc_setting.enableConstrainedMVD;
  vc_enc_reg.VC_ENC_h00.reg_enc_mv_clip_en      = 0;
  vc_enc_reg.VC_ENC_h00.reg_enc_cons_fme        = 0;
  vc_enc_reg.VC_ENC_h00.reg_enc_cons_mrg        = g_cvi_enc_setting.enableConstrainedMergeCand;
  vc_enc_reg.VC_ENC_h00.reg_enc_fg_cost_en      = g_algo_cfg.CostPenaltyCfg.EnableForeground;
  vc_enc_reg.VC_ENC_h00.reg_enc_dis_inter_i4    = g_algo_cfg.DisablePfrmIntra4 ? 1 : 0;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x00, vc_enc_reg.VC_ENC_h00.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x00, vc_enc_reg.VC_ENC_h00.val,0);

  // Set i4 early termination before reg_read_src_go.
  vc_enc_reg.VC_ENC_h4C.reg_i4_madi_thr        = ccu_get_i4_madi_thr();
  vc_enc_reg.VC_ENC_h4C.reg_i4_early_term_en   = (g_algo_cfg.I4TermRatio == 0) ? 0x0 : 0x1;
  vc_enc_reg.VC_ENC_h50.reg_pic_i4_term_target = ccu_get_pic_target_i4_term();
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x4c, vc_enc_reg.VC_ENC_h4C.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x50, vc_enc_reg.VC_ENC_h50.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x4c, vc_enc_reg.VC_ENC_h4C.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x50, vc_enc_reg.VC_ENC_h50.val,0);

  vc_mm_reg.VC_MM_hB0.reg_read_src_go = 1;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xb0, vc_mm_reg.VC_MM_hB0.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xb0, vc_mm_reg.VC_MM_hB0.val,0);

  REG64_C bs_base, bs_end_base;
  bs_base.val = get_hw_base("bs");
  bs_end_base.val = bs_base.val + get_hw_size("bs");

  vc_mm_reg.VC_MM_hD0.val = bs_base.reg_lsb;
  vc_mm_reg.VC_MM_hD4.val = bs_base.reg_msb;
  vc_mm_reg.VC_MM_hD8.val = bs_end_base.reg_lsb;
  vc_mm_reg.VC_MM_hDC.val = bs_end_base.reg_msb;
  //current frame bit stream buffer
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xd0, vc_mm_reg.VC_MM_hD0.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xd4, vc_mm_reg.VC_MM_hD4.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xd8, vc_mm_reg.VC_MM_hD8.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xdc, vc_mm_reg.VC_MM_hDC.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xd0, vc_mm_reg.VC_MM_hD0.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xd4, vc_mm_reg.VC_MM_hD4.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xd8, vc_mm_reg.VC_MM_hD8.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xdc, vc_mm_reg.VC_MM_hDC.val,0);

  //overall bit stream space
  vc_mm_reg.VC_MM_hF4.val = bs_base.reg_lsb;
  vc_mm_reg.VC_MM_hF8.val = bs_base.reg_msb;
  vc_mm_reg.VC_MM_hFC.val = bs_end_base.reg_lsb;
  vc_mm_reg.VC_MM_h100.val = bs_end_base.reg_msb;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xf4, vc_mm_reg.VC_MM_hF4.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xf8, vc_mm_reg.VC_MM_hF8.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xfc, vc_mm_reg.VC_MM_hFC.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0x100, vc_mm_reg.VC_MM_h100.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xf4, vc_mm_reg.VC_MM_hF4.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xf8, vc_mm_reg.VC_MM_hF8.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xfc, vc_mm_reg.VC_MM_hFC.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0x100, vc_mm_reg.VC_MM_h100.val,0);


  //vc_mm_reg.VC_MM_hE0.reg_bs_slice_hdr_dat[0] = ?;
  //vc_mm_reg.VC_MM_hE0.reg_bs_slice_hdr_dat[1] = ?;
  //vc_mm_reg.VC_MM_hE0.reg_bs_slice_hdr_dat[2] = ?;
  //vc_mm_reg.VC_MM_hE0.reg_bs_slice_hdr_dat[3] = ?;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xe0, vc_mm_reg.VC_MM_hE0.val[0]);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xe4, vc_mm_reg.VC_MM_hE0.val[1]);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xe8, vc_mm_reg.VC_MM_hE0.val[2]);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MM, 0xec, vc_mm_reg.VC_MM_hE0.val[3]);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xe0, vc_mm_reg.VC_MM_hE0.val[0],0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xe4, vc_mm_reg.VC_MM_hE0.val[1],0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xe8, vc_mm_reg.VC_MM_hE0.val[2],0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MM, 0xec, vc_mm_reg.VC_MM_hE0.val[3],0);
}

void top_set_reg_vc_hdr(TComSlice *p_slice)
{
  const TComSPS *p_sps = p_slice->getSPS();
  const TComPPS *p_pps = p_slice->getPPS();
  const TComPic *p_pic = p_slice->getPic();
  int slice_idx = p_slice->getSliceIdx();
  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_ctx *p_cmdq_ctx = &g_sigpool.top_fw_cmdq_ctx;

  if (slice_idx == 0)
  {
    // SPS
    vc_hdr_reg.SPS_REG_h00.reg_min_cu_sz = p_sps->getLog2MinCodingBlockSize() - 2;
    vc_hdr_reg.SPS_REG_h00.reg_ctu_sz = p_sps->getLog2DiffMaxMinCodingBlockSize() +
                                        vc_hdr_reg.SPS_REG_h00.reg_min_cu_sz;
    vc_hdr_reg.SPS_REG_h00.reg_min_tu_sz = p_sps->getQuadtreeTULog2MinSize() - 2;
    vc_hdr_reg.SPS_REG_h00.reg_max_tu_sz = p_sps->getQuadtreeTULog2MaxSize() - 2;
    vc_hdr_reg.SPS_REG_h00.reg_max_tu_dep_inter_m1 = p_sps->getQuadtreeTUMaxDepthInter() - 1;
    vc_hdr_reg.SPS_REG_h00.reg_max_tu_dep_intra_m1 = p_sps->getQuadtreeTUMaxDepthIntra() - 1;

    vc_hdr_reg.SPS_REG_h04.reg_luma_bitdepth = (1 << (p_sps->getBitDepth(CHANNEL_TYPE_LUMA) - 8));
    vc_hdr_reg.SPS_REG_h04.reg_chroma_bitdepth = (1 << (p_sps->getBitDepth(CHANNEL_TYPE_CHROMA) - 8));
    vc_hdr_reg.SPS_REG_h08.reg_pcm_luma_bitdepth = (1 << (p_sps->getPCMBitDepth(CHANNEL_TYPE_LUMA) - 8));
    vc_hdr_reg.SPS_REG_h08.reg_pcm_chroma_bitdepth = (1 << (p_sps->getPCMBitDepth(CHANNEL_TYPE_CHROMA) - 8));
    vc_hdr_reg.SPS_REG_h08.reg_pcm_min_cu_sz = p_sps->getPCMLog2MinSize();
    vc_hdr_reg.SPS_REG_h08.reg_pcm_ctu_sz = p_sps->getPCMLog2MaxSize();
    vc_hdr_reg.SPS_REG_h10.reg_amp_flag = p_sps->getUseAMP();
    vc_hdr_reg.SPS_REG_h10.reg_sao_flag = p_sps->getUseSAO();
    vc_hdr_reg.SPS_REG_h10.reg_pcm_flag = !p_sps->getPCMFilterDisableFlag();
    vc_hdr_reg.SPS_REG_h10.reg_sis_flag = p_sps->getUseStrongIntraSmoothing();

    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x00, vc_hdr_reg.SPS_REG_h00.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x04, vc_hdr_reg.SPS_REG_h04.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x08, vc_hdr_reg.SPS_REG_h08.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x10, vc_hdr_reg.SPS_REG_h10.val);

    sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x00, vc_hdr_reg.SPS_REG_h00.val,0);
    sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x04, vc_hdr_reg.SPS_REG_h04.val,0);
    sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x08, vc_hdr_reg.SPS_REG_h08.val,0);
    sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x10, vc_hdr_reg.SPS_REG_h10.val,0);

    // PPS
    vc_hdr_reg.PPS_REG_h80.reg_dependent_slice_flag = p_pps->getDependentSliceSegmentsEnabledFlag();
    vc_hdr_reg.PPS_REG_h80.reg_signed_hiding_flag   = p_pps->getSignDataHidingEnabledFlag();
    vc_hdr_reg.PPS_REG_h80.reg_tile_flag            = p_pps->getTilesEnabledFlag();
    vc_hdr_reg.PPS_REG_h80.reg_tmp_mvp_flag         = (p_slice->getSliceType() == I_SLICE) ? 0 : p_slice->getEnableTMVPFlag();
    vc_hdr_reg.PPS_REG_h80.reg_weight_pred_flag     = p_pps->getUseWP();
    vc_hdr_reg.PPS_REG_h80.reg_weight_bipred_flag   = p_pps->getWPBiPred();
    vc_hdr_reg.PPS_REG_h80.reg_use_integer_mv       = 0;  //cmodel not support "use_integer_mv_flag"
    vc_hdr_reg.PPS_REG_h80.reg_trans_bypass_flag    = p_pps->getTransquantBypassEnabledFlag();
    vc_hdr_reg.PPS_REG_h80.reg_entropy_sync_flag    = p_pps->getEntropyCodingSyncEnabledFlag();
    vc_hdr_reg.PPS_REG_h80.reg_cons_intra_pred      = p_pps->getConstrainedIntraPred();
    vc_hdr_reg.PPS_REG_h80.reg_parallel_mgr_lvl_m2  = p_pps->getLog2ParallelMergeLevelMinus2();
    vc_hdr_reg.PPS_REG_h80.reg_avc_entropy_coding   = 1;
    vc_hdr_reg.PPS_REG_h84.reg_cur_poc              = p_slice->getPOC();

    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x80, vc_hdr_reg.PPS_REG_h80.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x84, vc_hdr_reg.PPS_REG_h84.val);

    sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x80, vc_hdr_reg.PPS_REG_h80.val,0);
    sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x84, vc_hdr_reg.PPS_REG_h84.val,0);
  }

  // Slice
  SliceType slice_type = p_slice->getSliceType();
  SliceType encCABACTableIdx = p_slice->getEncCABACTableIdx();
  Bool encCabacInitFlag = (slice_type != encCABACTableIdx && encCABACTableIdx != I_SLICE) ? true : false;
  Int cabac_init_type;
  if (slice_type == I_SLICE) {
    cabac_init_type = 0;
  } else if (slice_type == P_SLICE) {
    cabac_init_type = encCabacInitFlag ? 2 : 1;
  } else {
    cabac_init_type = encCabacInitFlag ? 1 : 2;
  }
  vc_hdr_reg.SLICE_REG_h104.reg_i_slice = (slice_type == I_SLICE) ? 1 : 0;
  vc_hdr_reg.SLICE_REG_h104.reg_p_slice = (slice_type == P_SLICE) ? 1 : 0;
  vc_hdr_reg.SLICE_REG_h104.reg_b_slice = (slice_type == B_SLICE) ? 1 : 0;
  vc_hdr_reg.SLICE_REG_h104.reg_cabac_init_type = cabac_init_type;
  vc_hdr_reg.SLICE_REG_h104.reg_col_l0_flag = p_slice->getColFromL0Flag();
  vc_hdr_reg.SLICE_REG_h104.reg_wb_mv_flag = (slice_type == I_SLICE) ? 0 : 1;

  vc_hdr_reg.SLICE_REG_h108.reg_num_ref_l0_act_m1 = p_slice->getNumRefIdx(REF_PIC_LIST_0) - 1;
  vc_hdr_reg.SLICE_REG_h108.reg_num_ref_l1_act_m1 = p_slice->getNumRefIdx(REF_PIC_LIST_1) - 1;
  vc_hdr_reg.SLICE_REG_h108.reg_col_ref_idx       = p_slice->getColRefIdx();
  vc_hdr_reg.SLICE_REG_h108.reg_max_merge_cand_m1 = p_slice->getMaxNumMergeCand() - 1;

  vc_hdr_reg.SLICE_REG_h140.reg_slice_qp = p_slice->getSliceQp();
  vc_hdr_reg.SLICE_REG_h140.reg_slice_cb_qp_ofs = p_slice->getPPS()->getQpOffset(COMPONENT_Cb);
  vc_hdr_reg.SLICE_REG_h140.reg_slice_cr_qp_ofs = p_slice->getPPS()->getQpOffset(COMPONENT_Cr);

  vc_hdr_reg.SLICE_REG_h150.reg_slice_ilf_dis       = p_slice->getDeblockingFilterDisable();
  vc_hdr_reg.SLICE_REG_h150.reg_slice_ilf_cross_dis = p_slice->getLFCrossSliceBoundaryFlag();
  vc_hdr_reg.SLICE_REG_h150.reg_slice_beta_ofs_div2 = p_slice->getDeblockingFilterBetaOffsetDiv2();
  vc_hdr_reg.SLICE_REG_h150.reg_slice_tc_ofs_div2   = p_slice->getDeblockingFilterTcOffsetDiv2();
  vc_hdr_reg.SLICE_REG_h150.reg_sao_luma_flag       = p_slice->getSaoEnabledFlag(CHANNEL_TYPE_LUMA);
  vc_hdr_reg.SLICE_REG_h150.reg_sao_chroma_flag     = p_slice->getSaoEnabledFlag(CHANNEL_TYPE_CHROMA);

  int frame_width_in_ctus = p_pic->getPicSym()->getFrameWidthInCtus();
  int slice_ts_addr_start = p_slice->getSliceSegmentCurStartCtuTsAddr();  // not support tile, ts is equal to rs
  int slice_ts_addr_end   = p_slice->getSliceSegmentCurEndCtuTsAddr();
  vc_hdr_reg.SLICE_REG_h160.reg_ctu_st_x = slice_ts_addr_start % frame_width_in_ctus;
  vc_hdr_reg.SLICE_REG_h160.reg_ctu_st_y = slice_ts_addr_start / frame_width_in_ctus;
  vc_hdr_reg.SLICE_REG_h164.reg_ctu_end_x = (slice_ts_addr_end - 1) % frame_width_in_ctus;
  vc_hdr_reg.SLICE_REG_h164.reg_ctu_end_y = (slice_ts_addr_end - 1) / frame_width_in_ctus;

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x104, vc_hdr_reg.SLICE_REG_h104.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x108, vc_hdr_reg.SLICE_REG_h108.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x140, vc_hdr_reg.SLICE_REG_h140.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x150, vc_hdr_reg.SLICE_REG_h150.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x160, vc_hdr_reg.SLICE_REG_h160.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x164, vc_hdr_reg.SLICE_REG_h164.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x104, vc_hdr_reg.SLICE_REG_h104.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x108, vc_hdr_reg.SLICE_REG_h108.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x140, vc_hdr_reg.SLICE_REG_h140.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x150, vc_hdr_reg.SLICE_REG_h150.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x160, vc_hdr_reg.SLICE_REG_h160.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x164, vc_hdr_reg.SLICE_REG_h164.val,0);
}

void top_set_reg_vc_mem(TComSlice *p_slice)
{
  int curr_poc = p_slice->getPic()->getPOC();
  int curr_fb_index = frm_buf_mgr_find_index_by_poc(curr_poc);
  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_ctx *p_cmdq_ctx = &g_sigpool.top_fw_cmdq_ctx;

  // 1. current frame buffer
  vc_mem_reg.VC_MEM_h60.reg_cur_fb_luma_base_lsb   = (get_hw_base_32("luma_fb", "LSB", curr_fb_index) >> 10);
  vc_mem_reg.VC_MEM_h64.reg_cur_fb_luma_base_msb   = get_hw_base_32("luma_fb", "MSB", curr_fb_index);
  vc_mem_reg.VC_MEM_h68.reg_cur_fb_chroma_base_lsb = (get_hw_base_32("chroma_fb", "LSB", curr_fb_index)) >> 10;
  vc_mem_reg.VC_MEM_h6C.reg_cur_fb_chroma_base_msb   = get_hw_base_32("chroma_fb", "MSB", curr_fb_index);
  vc_mem_reg.VC_MEM_h70.reg_cur_fb_pit         = (get_hw_pitch("luma_fb") >> 5);

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x60, vc_mem_reg.VC_MEM_h60.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x64, vc_mem_reg.VC_MEM_h64.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x68, vc_mem_reg.VC_MEM_h68.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x6C, vc_mem_reg.VC_MEM_h6C.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x70, vc_mem_reg.VC_MEM_h70.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x60, vc_mem_reg.VC_MEM_h60.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x64, vc_mem_reg.VC_MEM_h64.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x68, vc_mem_reg.VC_MEM_h68.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x6C, vc_mem_reg.VC_MEM_h6C.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x70, vc_mem_reg.VC_MEM_h70.val,0);

  // 2. DPB buffer
  if (p_slice->getSliceType() < I_SLICE)
  {
    int list_num = p_slice->isInterP() ? 1 : 2;
    for (int list = 0; list < list_num; list++)
    {
      RefPicList eRefPicList = ( list ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
      int ref_num = p_slice->getNumRefIdx(eRefPicList);

      for (int ref_idx = 0; ref_idx < ref_num; ref_idx++)
      {
        int ref_poc = p_slice->getRefPic(eRefPicList, ref_idx)->getPOC();
        int fb_index = frm_buf_mgr_find_index_by_poc(ref_poc);
        REG64_C fb_base[2];
        fb_base[0].val = get_hw_base("luma_fb", fb_index);
        fb_base[1].val = get_hw_base("chroma_fb", fb_index);

        for (int ch = 0; ch < 2; ch++)
        {
          vc_mem_reg.VC_MEM_h88.reg_prg_dpb_base_wd_lsb = fb_base[ch].reg_lsb;
          vc_mem_reg.VC_MEM_h8C.reg_prg_dpb_base_wd_msb = fb_base[ch].reg_msb;

          vc_mem_reg.VC_MEM_h80.reg_prg_dpb_base_go = 1;
          vc_mem_reg.VC_MEM_h80.reg_prg_dpb_base_idx = ch;
          vc_mem_reg.VC_MEM_h80.reg_prg_dpb_base_rw = 1;
          vc_mem_reg.VC_MEM_h80.reg_prg_dpb_base_type = 0;
          vc_mem_reg.VC_MEM_h80.reg_prg_dpb_base_addr = fb_index;

          sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x88, vc_mem_reg.VC_MEM_h88.val);
          sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x8C, vc_mem_reg.VC_MEM_h8C.val);
          sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x80, vc_mem_reg.VC_MEM_h80.val);

          sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x88, vc_mem_reg.VC_MEM_h88.val,0);
          sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x8C, vc_mem_reg.VC_MEM_h8C.val,0);
          sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x80, vc_mem_reg.VC_MEM_h80.val,0);
        }
      }
    }
  }

  // 3. ref list
  if (p_slice->getSliceType() < I_SLICE)
  {
    int list_num = p_slice->isInterP() ? 1 : 2;
    for (int list = 0; list < list_num; list++)
    {
      RefPicList eRefPicList = ( list ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
      int ref_num = p_slice->getNumRefIdx(eRefPicList);

      for (int ref_idx = 0; ref_idx < ref_num; ref_idx++)
      {
        int ref_poc = p_slice->getRefPic(eRefPicList, ref_idx)->getPOC();
        int is_lt = (int)(p_slice->getRefPic(eRefPicList, ref_idx)->getIsLongTerm());
        int fb_index = frm_buf_mgr_find_index_by_poc(ref_poc);

        // POC
        vc_hdr_reg.SLICE_REG_h110.reg_prg_ref_list_wd = ref_poc;
        vc_hdr_reg.SLICE_REG_h10C.reg_prg_ref_list_go = 1;
        vc_hdr_reg.SLICE_REG_h10C.reg_prg_ref_list_idx = list;
        vc_hdr_reg.SLICE_REG_h10C.reg_prg_ref_list_rw = 1;
        vc_hdr_reg.SLICE_REG_h10C.reg_prg_ref_list_addr = (ref_idx << 1);
        sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x110, vc_hdr_reg.SLICE_REG_h110.val);
        sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x10c, vc_hdr_reg.SLICE_REG_h10C.val);

        sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x110, vc_hdr_reg.SLICE_REG_h110.val,0);
        sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x10c, vc_hdr_reg.SLICE_REG_h10C.val,0);

        // FB_idx
        vc_hdr_reg.SLICE_REG_h110.reg_prg_ref_list_wd = ((fb_index << 8) | is_lt);
        vc_hdr_reg.SLICE_REG_h10C.reg_prg_ref_list_addr = (ref_idx << 1) + 1;
        sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x110, vc_hdr_reg.SLICE_REG_h110.val);
        sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x10c, vc_hdr_reg.SLICE_REG_h10C.val);

        sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x110, vc_hdr_reg.SLICE_REG_h110.val,0);
        sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_HDR, 0x10c, vc_hdr_reg.SLICE_REG_h10C.val,0);
      }
    }
  }

  // Not support scaling list 0x144 0x148 0x14c

  // 4. HW buffer
  int col_fb_idx = 0;
  if (p_slice->getSliceType() < I_SLICE)
  {
    if (p_slice->getNumRefIdx(REF_PIC_LIST_0) > 0)
      col_fb_idx = frm_buf_mgr_find_index_by_poc(p_slice->getRefPic(REF_PIC_LIST_0, p_slice->getColRefIdx())->getPOC());
  }

  vc_mem_reg.VC_MEM_h00.val = get_hw_base_32("nm_b", "LSB");
  vc_mem_reg.VC_MEM_h04.val = get_hw_base_32("nm_b", "MSB");
  vc_mem_reg.VC_MEM_h08.val = get_hw_base_32("nm_a", "LSB");
  vc_mem_reg.VC_MEM_h0C.val = get_hw_base_32("nm_a", "MSB");
  vc_mem_reg.VC_MEM_h10.val = get_hw_base_32("mv",  "LSB", col_fb_idx);
  vc_mem_reg.VC_MEM_h14.val = get_hw_base_32("mv",  "MSB", col_fb_idx);
  vc_mem_reg.VC_MEM_h18.val = get_hw_base_32("mv", "LSB", curr_fb_index);
  vc_mem_reg.VC_MEM_h1C.val = get_hw_base_32("mv", "MSB", curr_fb_index);
  vc_mem_reg.VC_MEM_h20.reg_nm_col_pit = (get_hw_pitch("mv") >> 5);
  vc_mem_reg.VC_MEM_h30.val = get_hw_base_32("iapu", "LSB");
  vc_mem_reg.VC_MEM_h34.val = get_hw_base_32("iapu", "MSB");
  vc_mem_reg.VC_MEM_h40.val = get_hw_base_32("ilf_b", "LSB");
  vc_mem_reg.VC_MEM_h44.val = get_hw_base_32("ilf_b", "MSB");
  vc_mem_reg.VC_MEM_h48.val = get_hw_base_32("ilf_a", "LSB");
  vc_mem_reg.VC_MEM_h4C.val = get_hw_base_32("ilf_a", "MSB");
  vc_mem_reg.VC_MEM_hC0.val = get_hw_base_32("ai_dqp", "LSB");
  vc_mem_reg.VC_MEM_hC4.val = get_hw_base_32("ai_dqp", "MSB");
  vc_mem_reg.VC_MEM_hC8.val = get_hw_pitch("ai_dqp");
  vc_mem_reg.VC_MEM_hD0.val = get_hw_base_32("isp_dqp", "LSB");
  vc_mem_reg.VC_MEM_hD4.val = get_hw_base_32("isp_dqp", "MSB");
  vc_mem_reg.VC_MEM_hD8.val = get_hw_pitch("isp_dqp");

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x00, vc_mem_reg.VC_MEM_h00.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x04, vc_mem_reg.VC_MEM_h04.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x08, vc_mem_reg.VC_MEM_h08.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x0C, vc_mem_reg.VC_MEM_h0C.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x10, vc_mem_reg.VC_MEM_h10.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x14, vc_mem_reg.VC_MEM_h14.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x18, vc_mem_reg.VC_MEM_h18.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x1C, vc_mem_reg.VC_MEM_h1C.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x20, vc_mem_reg.VC_MEM_h20.val);

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x30, vc_mem_reg.VC_MEM_h30.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x34, vc_mem_reg.VC_MEM_h34.val);

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x40, vc_mem_reg.VC_MEM_h40.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x44, vc_mem_reg.VC_MEM_h44.val);

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x48, vc_mem_reg.VC_MEM_h48.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0x4C, vc_mem_reg.VC_MEM_h4C.val);

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0xC0, vc_mem_reg.VC_MEM_hC0.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0xC4, vc_mem_reg.VC_MEM_hC4.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0xC8, vc_mem_reg.VC_MEM_hC8.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0xD0, vc_mem_reg.VC_MEM_hD0.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0xD4, vc_mem_reg.VC_MEM_hD4.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0xD8, vc_mem_reg.VC_MEM_hD8.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x00, vc_mem_reg.VC_MEM_h00.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x04, vc_mem_reg.VC_MEM_h04.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x08, vc_mem_reg.VC_MEM_h08.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x0C, vc_mem_reg.VC_MEM_h0C.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x10, vc_mem_reg.VC_MEM_h10.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x14, vc_mem_reg.VC_MEM_h14.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x18, vc_mem_reg.VC_MEM_h18.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x1C, vc_mem_reg.VC_MEM_h1C.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x20, vc_mem_reg.VC_MEM_h20.val,0);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x30, vc_mem_reg.VC_MEM_h30.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x34, vc_mem_reg.VC_MEM_h34.val,0);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x40, vc_mem_reg.VC_MEM_h40.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x44, vc_mem_reg.VC_MEM_h44.val,0);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x48, vc_mem_reg.VC_MEM_h48.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0x4C, vc_mem_reg.VC_MEM_h4C.val,0);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0xC0, vc_mem_reg.VC_MEM_hC0.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0xC4, vc_mem_reg.VC_MEM_hC4.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0xC8, vc_mem_reg.VC_MEM_hC8.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0xD0, vc_mem_reg.VC_MEM_hD0.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0xD4, vc_mem_reg.VC_MEM_hD4.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0xD8, vc_mem_reg.VC_MEM_hD8.val,0);
}

void top_set_reg_vc_enc(TComSlice *p_slice, TEncRateCtrl *p_rc)
{
  bool is_en_rc = (p_rc == nullptr) ? false : true;
  SliceType slice_type = p_slice->getSliceType();

  // VC_ENC_h00 had been set at top_set_reg_vc_mm() before firing source buffer.
  // Start from h04
  vc_enc_reg.VC_ENC_h04.reg_enc_cons_pux_lbnd = (g_cvi_enc_setting.mv_x_lbnd & 0x7fff);
  vc_enc_reg.VC_ENC_h04.reg_enc_cons_pux_ubnd = (g_cvi_enc_setting.mv_x_ubnd & 0x7fff);
  vc_enc_reg.VC_ENC_h08.reg_enc_cons_puy_lbnd = (g_cvi_enc_setting.mv_y_lbnd & 0x7fff);
  vc_enc_reg.VC_ENC_h08.reg_enc_cons_puy_ubnd = (g_cvi_enc_setting.mv_y_ubnd & 0x7fff);

  /*ENC IME X search range, HW only support default (-32, 31)
		0: -8, 7
		1: -16, 15
		2: -32, 31
		3: -64, 63;*/
  vc_enc_reg.VC_ENC_h0C.reg_enc_ime_xsr       = 2;
  vc_enc_reg.VC_ENC_h0C.reg_enc_ime_ysr       = 2;
  vc_enc_reg.VC_ENC_h0C.reg_enc_mrg_mvx_thr   = g_cvi_enc_setting.merge_mv_thr_x;
  vc_enc_reg.VC_ENC_h0C.reg_enc_mrg_mvy_thr   = g_cvi_enc_setting.merge_mv_thr_y;
  vc_enc_reg.VC_ENC_h0C.reg_enc_ime_mvdx_thr  = g_cvi_enc_setting.mvd_thr_x;
  vc_enc_reg.VC_ENC_h0C.reg_enc_ime_mvdy_thr  = g_cvi_enc_setting.mvd_thr_y;

  vc_enc_reg.VC_ENC_h14.reg_enc_fixed_qp      = (is_en_rc == false) ? 1 : 0;
  vc_enc_reg.VC_ENC_h14.reg_enc_qpmap_mode    = getQpMapMode();
  vc_enc_reg.VC_ENC_h14.reg_enc_qp_mode       = getQpMode();
  vc_enc_reg.VC_ENC_h14.reg_enc_qp_win_en     = getQpWinEn();
  vc_enc_reg.VC_ENC_h18.reg_enc_cons_qp       = 1;
  vc_enc_reg.VC_ENC_h18.reg_enc_cons_delta_qp = 1;

  vc_enc_reg.VC_ENC_h40.reg_row_avg_bit        = is_en_rc ? p_rc->getRCPic()->getCtuRowAvgBit() : 0;
  vc_enc_reg.VC_ENC_h44.reg_err_comp_scl       = getBitErrCompenScal();
  vc_enc_reg.VC_ENC_h44.reg_ctu_num_norm_scl   = getCtuRowNormScale();
  vc_enc_reg.VC_ENC_h48.reg_row_qp_delta       = getRowQpClip();
  vc_enc_reg.VC_ENC_h48.reg_row_ovf_qp_delta   = getInRowOverflowQpdelta();

  vc_enc_reg.VC_ENC_h54.reg_lms_lrm   = (uint16_t)((1 << LMS_LR_FRAC_BD) * g_significantScale_LR[0]);
  vc_enc_reg.VC_ENC_h54.reg_lms_lrc   = (uint16_t)((1 << LMS_LR_FRAC_BD) * g_significantScale_LR[1]);
  vc_enc_reg.VC_ENC_h58.reg_lms_m_max = (uint32_t)((1 << RESI_MDL_FRAC_BD) * g_significantScale_max);
  vc_enc_reg.VC_ENC_h5C.reg_lms_m_min = (uint32_t)((1 << RESI_MDL_FRAC_BD) * g_significantScale_min);
  vc_enc_reg.VC_ENC_h60.reg_lms_c_max = (int32_t)((1 << RESI_MDL_FRAC_BD) * g_significantBias_clip);
  vc_enc_reg.VC_ENC_h64.reg_lms_c_min = (int32_t)(-(1 << RESI_MDL_FRAC_BD) * g_significantBias_clip);

  //
  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_ctx *p_cmdq_ctx = &g_sigpool.top_fw_cmdq_ctx;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x00, vc_enc_reg.VC_ENC_h00.val);  // output h00 again
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x04, vc_enc_reg.VC_ENC_h04.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x08, vc_enc_reg.VC_ENC_h08.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x0c, vc_enc_reg.VC_ENC_h0C.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x14, vc_enc_reg.VC_ENC_h14.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x18, vc_enc_reg.VC_ENC_h18.val);

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x40, vc_enc_reg.VC_ENC_h40.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x44, vc_enc_reg.VC_ENC_h44.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x48, vc_enc_reg.VC_ENC_h48.val);

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x54, vc_enc_reg.VC_ENC_h54.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x58, vc_enc_reg.VC_ENC_h58.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x5c, vc_enc_reg.VC_ENC_h5C.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x60, vc_enc_reg.VC_ENC_h60.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x64, vc_enc_reg.VC_ENC_h64.val);
  
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x00, vc_enc_reg.VC_ENC_h00.val,0);  // output h00 again
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x04, vc_enc_reg.VC_ENC_h04.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x08, vc_enc_reg.VC_ENC_h08.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x0c, vc_enc_reg.VC_ENC_h0C.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x14, vc_enc_reg.VC_ENC_h14.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x18, vc_enc_reg.VC_ENC_h18.val,0);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x40, vc_enc_reg.VC_ENC_h40.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x44, vc_enc_reg.VC_ENC_h44.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x48, vc_enc_reg.VC_ENC_h48.val,0);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x54, vc_enc_reg.VC_ENC_h54.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x58, vc_enc_reg.VC_ENC_h58.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x5c, vc_enc_reg.VC_ENC_h5C.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x60, vc_enc_reg.VC_ENC_h60.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x64, vc_enc_reg.VC_ENC_h64.val,0);

  //-------------program enc table 0x84 0x80 start---------------
  UInt reg_value = 0;
  int addr = 0;

  if (is_en_rc)
  {
    // 4. TCL/LumaLUT
    int entry_number = g_tc_lut.get_entry_num();
    addr = top_vc_enc_prg_tab_write_pack(PRG_TAB_IDX_TCL_LLUT, entry_number,       1, g_tc_lut.get_input_lut_ptr(), addr);
    addr = top_vc_enc_prg_tab_write_pack(PRG_TAB_IDX_TCL_LLUT, (entry_number + 1), 1, g_tc_lut.get_output_lut_ptr(), addr);

    entry_number = g_lum_lut.get_entry_num();
    addr = top_vc_enc_prg_tab_write_pack(PRG_TAB_IDX_TCL_LLUT, entry_number, 1, g_lum_lut.get_input_lut_ptr(), addr);
    addr = top_vc_enc_prg_tab_write_pack(PRG_TAB_IDX_TCL_LLUT, entry_number, 1, g_lum_lut.get_output_lut_ptr(), addr);
  }

  // 5. QLLUT
  for (int qp = 0; qp < 52; qp++)
  {
    UInt lambda = (UInt)(g_qp_to_lambda_table[qp] * (1 << LAMBDA_FRAC_BIT));
    UInt sqrt_lambda = (UInt)(g_qp_to_sqrt_lambda_table[qp] * (1 << LC_LAMBDA_FRAC_BIT));
    reg_value = ((sqrt_lambda & 0x7fff) << 16) + (lambda & 0xffff);
    top_vc_enc_prg_tab_write(PRG_TAB_IDX_QLLUT, qp, reg_value);
  }

  // 8. SI_TAB
  for (int i = 0; i < 16; i++)
  {
    UInt mps = g_hwEntropyBits[i][0];
    UInt lps = g_hwEntropyBits[i][1];
    reg_value = ((mps & 0xfff) << 16) + (lps & 0x3fff);
    top_vc_enc_prg_tab_write(PRG_TAB_IDX_SI_TAB, i, reg_value);
  }

  // 9. RATIO
  top_vc_enc_prg_tab_write_ratio();

  // 10. M/C
  addr = 0;
  for (int ch = 0; ch < 2; ch++)
  {
    for (int pred = 0; pred < 2; pred++)
    {
      for (int size = 0; size < 4; size++)
      {
        // M
        for (int bin = 0; bin < 2; bin++)
        {
          reg_value = (g_significantScale[0][ch][pred][size][bin] & 0x3ffff);  // 18 bits
          top_vc_enc_prg_tab_write(PRG_TAB_IDX_MC_TAB, addr, reg_value);
          addr++;
        }
        // C
        for (int bin = 0; bin < 2; bin++)
        {
          reg_value = (g_significantBias[0][ch][pred][size][bin] & 0x3ffff);  // 18 bits
          top_vc_enc_prg_tab_write(PRG_TAB_IDX_MC_TAB, addr, reg_value);
          addr++;
        }
      }
    }
  }

  // 12 ROW QP
  if (is_en_rc)
  {
    // ROWQ_IN table
    int entry_number = g_row_q_lut.get_entry_num();
    for (int i = 0; i < entry_number; i++)
    {
      reg_value = (UInt)(g_row_q_lut.get_input_entry(i));
      top_vc_enc_prg_tab_write(PRG_TAB_IDX_ROW_QP, i, reg_value);
    }
    // ROWQ_OUT table
    addr = entry_number;
    top_vc_enc_prg_tab_write_pack(PRG_TAB_IDX_ROW_QP, entry_number, 1, g_row_q_lut.get_output_lut_ptr(), addr);
  }
  // 13 BG/FG
  addr = 0;
  for (int fg = 0; fg < 2; fg++)
  {
    // Weight
    for (int mode = 0; mode < 3; mode++)
    {
      reg_value = ((gp_hw_cost[fg][2][mode] & 0x3f) << 8) +
                  ((gp_hw_cost[fg][1][mode] & 0x3f) << 16) +
                  ((gp_hw_cost[fg][0][mode] & 0x3f) << 24);
      top_vc_enc_prg_tab_write(PRG_TAB_IDX_BG_FG, addr, reg_value);
      addr++;
    }

    // Bias
    for (int mode = 0; mode < 3; mode++)
    {
      // CU8
      reg_value = ((gp_hw_bias[fg][2][mode] & 0xffff) << 16);
      top_vc_enc_prg_tab_write(PRG_TAB_IDX_BG_FG, addr, reg_value);
      addr++;

      // CU16, CU32
      reg_value = (gp_hw_bias[fg][1][mode] & 0xffff) +
                  ((gp_hw_bias[fg][0][mode] & 0xffff) << 16);
      top_vc_enc_prg_tab_write(PRG_TAB_IDX_BG_FG, addr, reg_value);
      addr++;
    }
  }

  // 14. AI Delta QP table
  addr = 0;
  int entry_number = AI_SI_TAB_COUNT * AI_SI_TAB_BIN;
  top_vc_enc_prg_tab_write_pack(PRG_TAB_IDX_DQP_TAB, entry_number, 1, &g_smart_enc_ai_table[0][0], addr);

  // 15. AI Object Table
  if (gp_ai_enc_param)
  {
    addr = 0;
    int size = gp_ai_enc_param->mapping_table.size();
    int enable = 0;
    int table_idx = 0;
    int obj_pack[4];

    for (int i = 0; i < size; i += 4)
    {
      memset(obj_pack, 0, sizeof(obj_pack));

      for (int j = i; j < i + 4; j++)
      {
        if (j >= size)
          break;

        table_idx = gp_ai_enc_param->mapping_table[j].table_idx;
        enable = (table_idx != AI_INVALID_IDX) ? 1 : 0;
        obj_pack[(j % 4)] = enable ? (((table_idx & 0x7) << 1) | enable) : 0;
      }

      addr = top_vc_enc_prg_tab_write_pack(PRG_TAB_IDX_OBJ_TAB, 4, 1, obj_pack, addr);
    }
  }

  //-------------program enc table end---------------

  vc_enc_reg.VC_ENC_h1C.reg_enc_max_qp         = getRcMaxQp(slice_type);
  vc_enc_reg.VC_ENC_h1C.reg_enc_min_qp         = getRcMinQp(slice_type);
  vc_enc_reg.VC_ENC_h1C.reg_enc_max_delta_qp   = 0x1f; // 31
  vc_enc_reg.VC_ENC_h1C.reg_enc_min_delta_qp   = 0x20; // -32
  vc_enc_reg.VC_ENC_h70.reg_dist_chroma_weight = g_sigpool.dist_chroma_weight;
  vc_enc_reg.VC_ENC_h70.reg_ml_mv_gain         = get_fix_point_mv_gain();
  vc_enc_reg.VC_ENC_h70.reg_ml_satd_gain       = get_fix_point_sad_gain();
  vc_enc_reg.VC_ENC_h74.reg_fg_th_gain         = get_fix_point_fg_thr_gain();
  vc_enc_reg.VC_ENC_h74.reg_fg_th_ofs          = 0; // reserved
  vc_enc_reg.VC_ENC_h74.reg_fg_th_bias         = get_fix_point_fg_thr_bias();

  vc_enc_reg.VC_ENC_h8C.reg_ai_map_en          = g_algo_cfg.EnableSmartEncAI ? 1 : 0;
  vc_enc_reg.VC_ENC_h8C.reg_ai_smooth_qp_en    = g_algo_cfg.AISmoothMap ? 1 : 0;
  vc_enc_reg.VC_ENC_h8C.reg_conf_scale         = (g_ai_conf_scale & 0x3f);
  vc_enc_reg.VC_ENC_h8C.reg_tclip_min          = (g_ai_table_idx_min & 0x1f);
  vc_enc_reg.VC_ENC_h8C.reg_tclip_max          = (g_ai_table_idx_max & 0x1f);
  vc_enc_reg.VC_ENC_h8C.reg_isp_map_en         = (g_algo_cfg.EnableSmartEncISP && g_sigpool.enc_count > 0) ? 1 : 0;
  vc_enc_reg.VC_ENC_h8C.reg_skip_map_en        = g_algo_cfg.EnableSmartEncISPSkipMap ? 1 : 0;
  vc_enc_reg.VC_ENC_h8C.reg_trans_qp           = (g_isp_trans_qp & 0x7f);
  vc_enc_reg.VC_ENC_h8C.reg_motion_qp          = (g_isp_motion_qp & 0x7f);
  vc_enc_reg.VC_ENC_h90.reg_clip_delta_qp_min  = (g_clip_delta_qp_min & 0x7f);
  vc_enc_reg.VC_ENC_h90.reg_clip_delta_qp_max  = (g_clip_delta_qp_max & 0x7f);

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x1c, vc_enc_reg.VC_ENC_h1C.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x70, vc_enc_reg.VC_ENC_h70.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x74, vc_enc_reg.VC_ENC_h74.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x8c, vc_enc_reg.VC_ENC_h8C.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_ENC, 0x90, vc_enc_reg.VC_ENC_h90.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x1c, vc_enc_reg.VC_ENC_h1C.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x70, vc_enc_reg.VC_ENC_h70.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x74, vc_enc_reg.VC_ENC_h74.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x8c, vc_enc_reg.VC_ENC_h8C.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_ENC, 0x90, vc_enc_reg.VC_ENC_h90.val,0);
}

void top_fire_hw_cabac_init()
{
  vc_hdr_reg.SLICE_REG_h100.val = 0;
  vc_hdr_reg.SLICE_REG_h100.reg_cabac_init_go = 1;

  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x100, vc_hdr_reg.SLICE_REG_h100.val);
  p_ctx = &g_sigpool.top_fw_cmdq_ctx;
  sig_top_reg_cmdq_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x100, vc_hdr_reg.SLICE_REG_h100.val,0);
}

void top_fire_hw_slice()
{
  vc_hdr_reg.SLICE_REG_h100.val = 0;
  vc_hdr_reg.SLICE_REG_h100.reg_slice_go = 1;

  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x100, vc_hdr_reg.SLICE_REG_h100.val);
  p_ctx = &g_sigpool.top_fw_cmdq_ctx;
  sig_top_reg_cmdq_write(p_ctx, CALFDOZER_REG_BASE_HDR, 0x100, vc_hdr_reg.SLICE_REG_h100.val,1);
}

#ifdef CVI_RANDOM_ENCODE

int mask = 0xffffffff;
int convert_unsigned_to_int(int val, int bits)
{
  if (val >> (bits - 1))
  {
    val = ((mask >> bits) << bits) | val;
  }
  return val;
}

void top_calc_fpga_statistic()
{
  if (g_sigpool.enc_count == 0)
  {
    g_fpga_stat.InitStatItems();
    g_fpga_stat.Import();
  }
  g_fpga_stat.AddItemCount("src_width", g_sigpool.width);
  g_fpga_stat.AddItemCount("reg_i_slice", vc_hdr_reg.SLICE_REG_h104.reg_i_slice);
  g_fpga_stat.AddItemCount("reg_enc_fixed_qp", vc_enc_reg.VC_ENC_h14.reg_enc_fixed_qp);
  g_fpga_stat.AddItemCount("reg_enc_qp_mode", vc_enc_reg.VC_ENC_h14.reg_enc_qp_mode);
  g_fpga_stat.AddItemCount("reg_slice_qp", vc_hdr_reg.SLICE_REG_h140.reg_slice_qp);
  g_fpga_stat.AddItemCount("reg_slice_cb_qp_ofs", convert_unsigned_to_int(vc_hdr_reg.SLICE_REG_h140.reg_slice_cb_qp_ofs, 5));
  g_fpga_stat.AddItemCount("reg_slice_cr_qp_ofs", convert_unsigned_to_int(vc_hdr_reg.SLICE_REG_h140.reg_slice_cr_qp_ofs, 5));

  g_fpga_stat.AddItemCount("reg_err_comp_scl:BitErrSmoothFactor", g_bit_err_smooth_factor);
  g_fpga_stat.AddItemCount("reg_enc_max_qp", vc_enc_reg.VC_ENC_h1C.reg_enc_max_qp);
  g_fpga_stat.AddItemCount("reg_enc_min_qp", vc_enc_reg.VC_ENC_h1C.reg_enc_min_qp);
  g_fpga_stat.AddItemCount("reg_row_avg_bit", vc_enc_reg.VC_ENC_h40.reg_row_avg_bit);

  g_fpga_stat.AddItemCount("reg_row_qp_delta", vc_enc_reg.VC_ENC_h48.reg_row_qp_delta);
  g_fpga_stat.AddItemCount("reg_row_ovf_qp_delta", vc_enc_reg.VC_ENC_h48.reg_row_ovf_qp_delta);
  g_fpga_stat.AddItemCount("reg_lms_lrm", vc_enc_reg.VC_ENC_h54.reg_lms_lrm);
  g_fpga_stat.AddItemCount("reg_lms_lrc", vc_enc_reg.VC_ENC_h54.reg_lms_lrc);

  g_fpga_stat.AddItemCount("reg_lms_m_max", vc_enc_reg.VC_ENC_h58.reg_lms_m_max);
  g_fpga_stat.AddItemCount("reg_lms_m_min", vc_enc_reg.VC_ENC_h5C.reg_lms_m_min);
  g_fpga_stat.AddItemCount("reg_lms_c_max", vc_enc_reg.VC_ENC_h60.reg_lms_c_max);
  g_fpga_stat.AddItemCount("reg_lms_c_min", convert_unsigned_to_int(vc_enc_reg.VC_ENC_h64.reg_lms_c_min, 18));
  g_fpga_stat.AddItemCount("reg_dist_chroma_weight", vc_enc_reg.VC_ENC_h70.reg_dist_chroma_weight);

  // RC Tables
  for (int i = 0; i < g_tc_lut.get_entry_num(); i++)
  {
    g_fpga_stat.AddItemCount("g_tc_lut:in", g_tc_lut.get_input_entry(i));
    g_fpga_stat.AddItemCount("g_tc_lut:out", g_tc_lut.get_output_entry(i));
  }
  g_fpga_stat.AddItemCount("g_tc_lut:out", g_tc_lut.get_output_entry(g_tc_lut.get_entry_num()));

  int lambda_frac = (1 << LAMBDA_FRAC_BIT);
  int sqrt_frac = (1 << LC_LAMBDA_FRAC_BIT);
  for (int i = 0; i < QP_NUM; i++)
  {
    g_fpga_stat.AddItemCount("g_qp_to_lambda_table", g_qp_to_lambda_table[i] * lambda_frac);
    g_fpga_stat.AddItemCount("g_qp_to_sqrt_lambda_table", g_qp_to_sqrt_lambda_table[i] * sqrt_frac);
  }

  for (int i = 0; i < 16; i++)
  {
    g_fpga_stat.AddItemCount("SI_TAB_0:g_hwEntropyBits[0]", g_hwEntropyBits[i][0]);
    g_fpga_stat.AddItemCount("SI_TAB_1:g_hwEntropyBits[1]", g_hwEntropyBits[i][1]);
  }

  for (int i = 0; i < g_row_q_lut.get_entry_num(); i++)
  {
    g_fpga_stat.AddItemCount("g_row_q_lut_in", g_row_q_lut.get_input_entry(i));
    g_fpga_stat.AddItemCount("g_row_q_lut_out", g_row_q_lut.get_output_entry(i));
  }

  g_fpga_stat.AddItemCount("ratio_luma_inter_4", rru_get_last_pos_scale(g_lastPosBinScale_init[0][0][0]));
  g_fpga_stat.AddItemCount("ratio_luma_inter_8", rru_get_last_pos_scale(g_lastPosBinScale_init[0][0][1]));
  g_fpga_stat.AddItemCount("ratio_luma_inter_16", rru_get_last_pos_scale(g_lastPosBinScale_init[0][0][2]));
  g_fpga_stat.AddItemCount("ratio_luma_inter_32", rru_get_last_pos_scale(g_lastPosBinScale_init[0][0][3]));
  g_fpga_stat.AddItemCount("ratio_chroma_inter_4", rru_get_last_pos_scale(g_lastPosBinScale_init[1][0][0]));
  g_fpga_stat.AddItemCount("ratio_chroma_inter_8", rru_get_last_pos_scale(g_lastPosBinScale_init[1][0][1]));

  g_fpga_stat.AddItemCount("ratio_luma_intra_4", rru_get_last_pos_scale(g_lastPosBinScale_init[0][1][0]));
  g_fpga_stat.AddItemCount("ratio_luma_intra_8", rru_get_last_pos_scale(g_lastPosBinScale_init[0][1][1]));
  g_fpga_stat.AddItemCount("ratio_luma_intra_16", rru_get_last_pos_scale(g_lastPosBinScale_init[0][1][2]));
  g_fpga_stat.AddItemCount("ratio_luma_intra_32", rru_get_last_pos_scale(g_lastPosBinScale_init[0][1][3]));
  g_fpga_stat.AddItemCount("ratio_chroma_intra_4", rru_get_last_pos_scale(g_lastPosBinScale_init[1][1][0]));
  g_fpga_stat.AddItemCount("ratio_chroma_intra_8", rru_get_last_pos_scale(g_lastPosBinScale_init[1][1][1]));

  //ratio
  for (int ch = 0; ch < 2; ch++)
  {
    string ch_s = ch == 0 ? "luma" : "chroma";
    for (int pred = 0; pred < 2; pred++)
    {
      string pred_s = pred == 0 ? "inter" : "intra";
      for (int size = 0; size < 4; size++)
      {
        string size_s = "blk" + std::to_string(4 * (1 << size));
        // M, C
        for (int bin = 0; bin < 2; bin++)
        {
          string out_s = ch_s + "_" + pred_s + "_" + size_s + "_bin" + std::to_string(bin);
          g_fpga_stat.AddItemCount("M_" + out_s, g_significantScale[0][ch][pred][size][bin]);
          g_fpga_stat.AddItemCount("C_" + out_s, g_significantBias[0][ch][pred][size][bin]);
        }
      }
    }
  }

  g_fpga_stat.AddItemCount("reg_tmp_mvp_flag", vc_hdr_reg.PPS_REG_h80.reg_tmp_mvp_flag);
  g_fpga_stat.AddItemCount("reg_slice_ilf_dis", vc_hdr_reg.SLICE_REG_h150.reg_slice_ilf_dis);
  g_fpga_stat.AddItemCount("reg_slice_beta_ofs_div2", convert_unsigned_to_int(vc_hdr_reg.SLICE_REG_h150.reg_slice_beta_ofs_div2, 4));
  g_fpga_stat.AddItemCount("reg_slice_tc_ofs_div2", convert_unsigned_to_int(vc_hdr_reg.SLICE_REG_h150.reg_slice_tc_ofs_div2, 4));

  g_fpga_stat.AddItemCount("reg_enc_cons_mvd_en", vc_enc_reg.VC_ENC_h00.reg_enc_cons_mvd_en);
  g_fpga_stat.AddItemCount("reg_enc_ime_mvdx_thr", vc_enc_reg.VC_ENC_h0C.reg_enc_ime_mvdx_thr);
  g_fpga_stat.AddItemCount("reg_enc_ime_mvdy_thr", vc_enc_reg.VC_ENC_h0C.reg_enc_ime_mvdy_thr);
  g_fpga_stat.AddItemCount("reg_i4_madi_thr", vc_enc_reg.VC_ENC_h4C.reg_i4_madi_thr);
  g_fpga_stat.AddItemCount("reg_i4_early_term_en", 1);
  g_fpga_stat.AddItemCount("reg_pic_i4_term_target (I4TermRatio)", (int)(g_algo_cfg.I4TermRatio * 100));

  g_fpga_stat.AddItemCount("reg_enc_fg_cost_en", vc_enc_reg.VC_ENC_h00.reg_enc_fg_cost_en);
  g_fpga_stat.AddItemCount("reg_ml_mv_gain", vc_enc_reg.VC_ENC_h70.reg_ml_mv_gain);
  g_fpga_stat.AddItemCount("reg_ml_satd_gain", vc_enc_reg.VC_ENC_h70.reg_ml_satd_gain);
  g_fpga_stat.AddItemCount("reg_fg_th_gain", vc_enc_reg.VC_ENC_h74.reg_fg_th_gain);
  g_fpga_stat.AddItemCount("reg_fg_th_bias", convert_unsigned_to_int(vc_enc_reg.VC_ENC_h74.reg_fg_th_bias, 8));

  g_fpga_stat.AddItemCount("reg_enc_qpmap_mode", vc_enc_reg.VC_ENC_h14.reg_enc_qpmap_mode);
  g_fpga_stat.AddItemCount("reg_enc_qp_win_en (roi count)", getQpWinCount());

  {
      const char* type[] = {"BG", "FG", "cuWt"};
      const char* mode[] = {"Intra", "Inter", "Skip"};
      const int blk_sz[] = {32, 16, 8};
      for (int i = 0; i < 3; i++)
      {
          int pred, blk;
          for (pred = 0; pred < 3; pred++)
          {
              for (blk = 0; blk < 3; blk++)
              {
                  char name[128];
                  sprintf(name, "%s_%s%dCost", type[i], mode[pred], blk_sz[blk]);
                  g_fpga_stat.AddItemCount(name, g_init_cost_wt[i][blk][pred]);
                  sprintf(name, "%s_%s%dBias", type[i], mode[pred], blk_sz[blk]);
                  g_fpga_stat.AddItemCount(name, g_init_cost_bi[i][blk][pred]);
              }
          }
      }
  }

  g_fpga_stat.AddItemCount("reg_enc_cons_mrg", vc_enc_reg.VC_ENC_h00.reg_enc_cons_mrg);
  g_fpga_stat.AddItemCount("reg_enc_mrg_mvx_thr", vc_enc_reg.VC_ENC_h0C.reg_enc_mrg_mvx_thr);
  g_fpga_stat.AddItemCount("reg_enc_mrg_mvy_thr", vc_enc_reg.VC_ENC_h0C.reg_enc_mrg_mvy_thr);

}
#endif //~CVI_RANDOM_ENCODE

void top_set_reg_vbd(TComSlice *p_slice)
{
    const TComSPS *p_sps = p_slice->getSPS();
    sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
    //sig_ctx *p_cmdq_ctx = &g_sigpool.top_fw_cmdq_ctx;

    vbd_sys_reg.VBD_SYS_h14.reg_vbd_gc_ena = 1;
    REG64_C src_y;
    src_y.val = get_hw_base("src_y");
    vbd_sys_reg.VBD_SYS_h20.reg_vbc_sa_base_lsb = (src_y.reg_lsb >> 10);
    vbd_sys_reg.VBD_SYS_h24.reg_vbc_sa_base_msb = src_y.reg_msb;

    vbd_sys_reg.VBD_SYS_h28.reg_vbc_cpx_base_lsb = (src_y.reg_lsb >> 8);
    vbd_sys_reg.VBD_SYS_h2C.reg_vbc_cpx_base_msb = src_y.reg_msb;

    vbd_sys_reg.VBD_SYS_h30.reg_vbc_ena = 1;    //1: from VBD
    vbd_sys_reg.VBD_SYS_h34.reg_vbc_color_fmt = 0;
    vbd_sys_reg.VBD_SYS_h34.reg_vbc_nv12_10b = p_sps->getBitDepth(CHANNEL_TYPE_LUMA) > 8 ? 1 : 0;
    vbd_sys_reg.VBD_SYS_h38.reg_vbc_pic_width_m1 = p_sps->getPicWidthInLumaSamples() - 1;
    vbd_sys_reg.VBD_SYS_h38.reg_vbc_pic_height_m1 = p_sps->getPicHeightInLumaSamples() - 1;
    vbd_sys_reg.VBD_SYS_h50.reg_vbc_lossy_tb = 0;//pVbcEnc->lossy ? pVbcEnc->lossy_truncate : 0;

    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBD, 0x14, vbd_sys_reg.VBD_SYS_h14.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBD, 0x20, vbd_sys_reg.VBD_SYS_h20.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBD, 0x24, vbd_sys_reg.VBD_SYS_h24.val);

    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBD, 0x28, vbd_sys_reg.VBD_SYS_h28.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBD, 0x2C, vbd_sys_reg.VBD_SYS_h2C.val);

    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBD, 0x30, vbd_sys_reg.VBD_SYS_h30.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBD, 0x34, vbd_sys_reg.VBD_SYS_h34.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBD, 0x38, vbd_sys_reg.VBD_SYS_h38.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBD, 0x3C, vbd_sys_reg.VBD_SYS_h3C.val);

    //fire vbd
    vbd_sys_reg.VBD_SYS_h00.reg_vbc_ndma_act_num_m1 = 0xF;//hevc = 15, avc = 3
    vbd_sys_reg.VBD_SYS_h00.reg_vbc_sa_prefetch_en = 1;
    vbd_sys_reg.VBD_SYS_h00.reg_vbc_frame_start = 1;
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBD, 0x00, vbd_sys_reg.VBD_SYS_h00.val);
}

void top_set_vbc_wakeup()
{
  vbc_sys_reg.VBC_SYS_h0C.reg_vbc_cg_en = 0x1;
  vbc_sys_reg.VBC_SYS_h0C.reg_vbc_cg_larb_en = 0x1;
  vbc_sys_reg.VBC_SYS_h0C.reg_vbc_cg_vbd_en = 0x1;
  vbc_sys_reg.VBC_SYS_h0C.reg_vbc_ck_wakeup = 1;

  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x0C, vbc_sys_reg.VBC_SYS_h0C.val);
}

void top_fire_vbc()
{
  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  vbc_sys_reg.VBC_SYS_h00.val = 0;
  vbc_sys_reg.VBC_SYS_h00.reg_frame_start = 1;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x00, vbc_sys_reg.VBC_SYS_h00.val);
  vbc_sys_reg.VBC_SYS_h00.reg_frame_start = 0;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x00, vbc_sys_reg.VBC_SYS_h00.val);
}

void top_set_reg_vbc(TComSlice *p_slice)
{
  int curr_poc = p_slice->getPic()->getPOC();
  int curr_fb_index = frm_buf_mgr_find_index_by_poc(curr_poc);
  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;

  vbc_sys_reg.VBC_SYS_h60.reg_vbc_meta_base_lsb = (get_hw_base_32("vbc_meta", "LSB", curr_fb_index) >> 10);
  vbc_sys_reg.VBC_SYS_h64.reg_vbc_meta_base_msb = get_hw_base_32("vbc_meta", "MSB", curr_fb_index);
  vbc_sys_reg.VBC_SYS_h68.reg_vbc_meta_pit = (get_hw_pitch("vbc_meta") >> 5);

  vbc_sys_reg.VBC_SYS_h70.reg_vbc_luma_base_lsb = (get_hw_base_32("luma_fb", "LSB", curr_fb_index) >> 10);
  vbc_sys_reg.VBC_SYS_h74.reg_vbc_luma_base_msb = get_hw_base_32("luma_fb", "MSB", curr_fb_index);
  vbc_sys_reg.VBC_SYS_h78.reg_vbc_chroma_base_lsb = (get_hw_base_32("chroma_fb", "LSB", curr_fb_index) >> 10);
  vbc_sys_reg.VBC_SYS_h7C.reg_vbc_chroma_base_msb = get_hw_base_32("chroma_fb", "MSB", curr_fb_index);

  vbc_sys_reg.VBC_SYS_hB4.reg_avc_mode = 0;
  vbc_sys_reg.VBC_SYS_hB4.reg_vbe_ve_db_dis = p_slice->getDeblockingFilterDisable();
  if (p_slice->getSliceType() < I_SLICE)
    vbc_sys_reg.VBC_SYS_hB4.reg_vbd_ve_db_dis = p_slice->getRefPic(REF_PIC_LIST_0, 0)->getSlice(0)->getDeblockingFilterDisable();
  else
    vbc_sys_reg.VBC_SYS_hB4.reg_vbd_ve_db_dis = p_slice->getDeblockingFilterDisable();
  vbc_sys_reg.VBC_SYS_hB4.reg_wbc_luma_y_offset = 1;
  vbc_sys_reg.VBC_SYS_hB4.reg_wbc_chroma_y_offset = 1;

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x60, vbc_sys_reg.VBC_SYS_h60.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x64, vbc_sys_reg.VBC_SYS_h64.val);

  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x68, vbc_sys_reg.VBC_SYS_h68.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x70, vbc_sys_reg.VBC_SYS_h70.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x74, vbc_sys_reg.VBC_SYS_h74.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x78, vbc_sys_reg.VBC_SYS_h78.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x7C, vbc_sys_reg.VBC_SYS_h7C.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0xB4, vbc_sys_reg.VBC_SYS_hB4.val);
  if (p_slice->getSliceType() < I_SLICE)
  {
      int ref_idx = 0;
      RefPicList eRefPicList = REF_PIC_LIST_0;
      int ref_poc = p_slice->getRefPic(eRefPicList, ref_idx)->getPOC();
      int fb_index = frm_buf_mgr_find_index_by_poc(ref_poc);
      vbc_sys_reg.VBC_SYS_h80.reg_vbd_meta_base_lsb = get_hw_base_32("vbc_meta", "LSB", fb_index) >> 10;
      vbc_sys_reg.VBC_SYS_h84.reg_vbd_meta_base_msb = get_hw_base_32("vbc_meta", "MSB", fb_index);

      vbc_sys_reg.VBC_SYS_h88.reg_vbd_meta_pit = (get_hw_pitch("vbc_meta") >> 5);

      vbc_sys_reg.VBC_SYS_h90.reg_vbd_luma_base_lsb = (get_hw_base_32("luma_fb", "LSB", fb_index) >> 10);
      vbc_sys_reg.VBC_SYS_h94.reg_vbd_luma_base_msb = get_hw_base_32("luma_fb", "MSB", fb_index);
      vbc_sys_reg.VBC_SYS_h98.reg_vbd_chroma_base_lsb = (get_hw_base_32("chroma_fb", "LSB", fb_index) >> 10);
      vbc_sys_reg.VBC_SYS_h9C.reg_vbd_chroma_base_msb = get_hw_base_32("chroma_fb", "MSB", fb_index);

    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x80, vbc_sys_reg.VBC_SYS_h80.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x84, vbc_sys_reg.VBC_SYS_h84.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x88, vbc_sys_reg.VBC_SYS_h88.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x90, vbc_sys_reg.VBC_SYS_h90.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x94, vbc_sys_reg.VBC_SYS_h94.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x98, vbc_sys_reg.VBC_SYS_h98.val);
    sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0x9C, vbc_sys_reg.VBC_SYS_h9C.val);
  }

  vbc_sys_reg.VBC_SYS_hB0.reg_pic_width_m1 = p_slice->getSPS()->getPicWidthInLumaSamples() - 1;
  vbc_sys_reg.VBC_SYS_hB0.reg_pic_height_m1 = p_slice->getSPS()->getPicHeightInLumaSamples() - 1;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_VBC, 0xB0, vbc_sys_reg.VBC_SYS_hB0.val);
}

void top_set_all_reg(TComSlice *p_slice, TEncRateCtrl *p_rc)
{
  top_reg_init();
  top_set_hw_wakeup();
  if (g_sigdump.testBsOverflow)
    top_set_interrupt(0x2000); //enable buf overflow interrupt
  top_set_axi_map();
  if (isEnableVBC() || g_sigdump.fpga)
  {
    top_set_vbc_wakeup();
    top_set_reg_vbc(p_slice);
    if (isEnableVBD())
      top_set_reg_vbd(p_slice);
  }
  top_set_reg_vc_mm(p_slice);
  top_set_reg_vc_mem(p_slice);
  top_set_reg_vc_hdr(p_slice);

  // fire cabac init after slice data done.
  top_fire_hw_cabac_init();

  top_set_reg_qp_map();
  top_set_reg_vc_enc(p_slice, p_rc);

  // fire hw
  if (isEnableVBC() || g_sigdump.fpga)
    top_fire_vbc();

  top_fire_hw_slice();

#ifdef CVI_RANDOM_ENCODE
  if (g_en_fpga_stat)
    top_calc_fpga_statistic();
#endif //~CVI_RANDOM_ENCODE
}

void top_set_reg_qp_map_buf()
{
  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_ctx *p_cmdq_ctx = &g_sigpool.top_fw_cmdq_ctx;
  vc_mem_reg.VC_MEM_hA0.val = get_hw_base_32("qp_map", "LSB");
  vc_mem_reg.VC_MEM_hA4.val = get_hw_base_32("qp_map", "MSB");
  vc_mem_reg.VC_MEM_hA8.reg_qpmap_pit  = (get_hw_pitch("qp_map") >> 5);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0xA0, vc_mem_reg.VC_MEM_hA0.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0xA4, vc_mem_reg.VC_MEM_hA4.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_MEM, 0xA8, vc_mem_reg.VC_MEM_hA8.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0xA0, vc_mem_reg.VC_MEM_hA0.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0xA4, vc_mem_reg.VC_MEM_hA4.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_MEM, 0xA8, vc_mem_reg.VC_MEM_hA8.val,0);
}

void top_set_reg_qp_map_roi()
{
  unsigned int reg_value = 0;

  for (int i = 0; i < QP_ROI_LIST_SIZE; i++)
  {
    if (g_qp_roi_cfg[i].is_enable == false)
      continue;

    int x = g_qp_roi_cfg[i].x;
    int y = g_qp_roi_cfg[i].y;
    int w = g_qp_roi_cfg[i].width;
    int h = g_qp_roi_cfg[i].height;

    int start_x = (x >> 4) << 4;
    int start_y = (y >> 4) << 4;
    int end_x = ((x + w) >> 4) << 4;
    int end_y = ((y + h) >> 4) << 4;

    int addr = (i * 3);
    reg_value = ((start_y & 0xffff) << 16) | (start_x & 0xffff);
    top_vc_enc_prg_tab_write(PRG_TAB_IDX_QP_MAP_ROI, addr, reg_value);

    reg_value = ((g_qp_roi_cfg[i].qp & 0xff) << 8) | ((int)(g_qp_roi_cfg[i].mode) & 0x1);
    top_vc_enc_prg_tab_write(PRG_TAB_IDX_QP_MAP_ROI, addr + 1, reg_value);

    reg_value = ((end_y & 0xffff) << 16) | (end_x & 0xffff);
    top_vc_enc_prg_tab_write(PRG_TAB_IDX_QP_MAP_ROI, addr + 2, reg_value);
  }
}

void top_set_reg_qp_map()
{
  int qp_map_mode = getQpMapMode();
  if (qp_map_mode == 0)
  {
    // Disable
    return;
  }
  else if (qp_map_mode == 1)
  {
    // Dram
    top_set_reg_qp_map_buf();
    sig_top_output_qpmap();
  }
  else if (qp_map_mode == 2)
  {
    top_set_reg_qp_map_roi();
  }
}

void top_set_hw_wakeup()
{
  vc_sys_reg.VC_SYS_h40.reg_vc_ck_wakeup = 1;

  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_ctx *p_cmdq_ctx = &g_sigpool.top_fw_cmdq_ctx;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_SYS, 0x40, vc_sys_reg.VC_SYS_h40.val);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_SYS, 0x40, vc_sys_reg.VC_SYS_h40.val,0);
}

void top_set_axi_map()
{
  //read client
  vc_sys_reg.VC_SYS_h240.reg_vc_axi_rc0_map = 1; //DRAM
  vc_sys_reg.VC_SYS_h240.reg_vc_axi_rc1_map = g_nmu_axi_map;
  vc_sys_reg.VC_SYS_h240.reg_vc_axi_rc2_map = 1;
  vc_sys_reg.VC_SYS_h240.reg_vc_axi_rc3_map = g_iap_axi_map;
  vc_sys_reg.VC_SYS_h240.reg_vc_axi_rc4_map = g_ppu_axi_map;
  vc_sys_reg.VC_SYS_h240.reg_vc_axi_rc5_map = 1;
  vc_sys_reg.VC_SYS_h240.reg_vc_axi_rc6_map = 1;
  vc_sys_reg.VC_SYS_h240.reg_vc_axi_rc7_map = 1;

  //write client
  vc_sys_reg.VC_SYS_h104.reg_vc_axi_wc0_map = g_nmu_axi_map;
  vc_sys_reg.VC_SYS_h104.reg_vc_axi_wc1_map = 1;
  vc_sys_reg.VC_SYS_h104.reg_vc_axi_wc2_map = g_iap_axi_map;
  vc_sys_reg.VC_SYS_h104.reg_vc_axi_wc3_map = g_ppu_axi_map;
  vc_sys_reg.VC_SYS_h104.reg_vc_axi_wc4_map = 1;
  vc_sys_reg.VC_SYS_h104.reg_vc_axi_wc5_map = 1;
  vc_sys_reg.VC_SYS_h104.reg_vc_axi_wc6_map = 1;
  vc_sys_reg.VC_SYS_h104.reg_vc_axi_wc7_map = 1;

  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_ctx *p_cmdq_ctx = &g_sigpool.top_fw_cmdq_ctx;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_SYS, 0x104, vc_sys_reg.VC_SYS_h104.val);
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_SYS, 0x240, vc_sys_reg.VC_SYS_h240.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_SYS, 0x104, vc_sys_reg.VC_SYS_h104.val,0);
  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_SYS, 0x240, vc_sys_reg.VC_SYS_h240.val,0);
}

void top_set_interrupt(uint32_t val)
{
  vc_sys_reg.VC_SYS_h10.reg_vc_int_en = val;
  sig_ctx *p_ctx = &g_sigpool.top_fw_ctx;
  sig_ctx *p_cmdq_ctx = &g_sigpool.top_fw_cmdq_ctx;
  sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_SYS, 0x10, vc_sys_reg.VC_SYS_h10.val);

  sig_top_reg_cmdq_write(p_cmdq_ctx, CALFDOZER_REG_BASE_SYS, 0x10, vc_sys_reg.VC_SYS_h10.val,0);
}

void top_fill_hw_stat_golden(HwStatistic *p_hw_stat, bool is_rc_en, TComPic *pcPic, TEncSlice *pcSliceEncoder)
{
  unsigned char frm_max_qp = g_stat_frm_max_qp;
  unsigned char frm_min_qp = g_stat_frm_min_qp;
  int frm_qp_sum = g_stat_frm_qp_sum;
  if (is_rc_en == false)
  {
    frm_max_qp = (unsigned char)(pcPic->getSlice(0)->getSliceQp());
    frm_min_qp = frm_max_qp;
    int pic_w_in_16 = (g_sigpool.width + 15) >> 4;
    int pic_h_in_16 = (g_sigpool.height + 15) >> 4;
    frm_qp_sum = pic_w_in_16 * pic_h_in_16 * frm_max_qp;
  }

  p_hw_stat->frm_mse_sum = (unsigned int)(pcSliceEncoder->getPicDist());
  p_hw_stat->frm_max_qp = frm_max_qp;
  p_hw_stat->frm_min_qp = frm_min_qp;
  p_hw_stat->frm_qp_sum = frm_qp_sum;
  p_hw_stat->frm_qp_hist_count = QP_NUM;
  p_hw_stat->p_frm_qp_hist = &(g_qp_hist[0]);
  p_hw_stat->frm_madi_sum = g_pic_tc_accum;
  p_hw_stat->frm_madp_sum = g_pic_madp_accum;
  p_hw_stat->frm_madi_hist_count = (1 << MADI_HIST_PREC);
  p_hw_stat->p_frm_madi_hist = &(g_madi8_hist[0]);
  p_hw_stat->frm_early_term = g_picTermI4Cnt;
  p_hw_stat->frm_fg_cu16_cnt = g_pic_foreground_cnt;
  p_hw_stat->frm_bso_blen = g_sigpool.slice_data_size[0];
  p_hw_stat->frm_hdr_sum = g_sigpool.frm_hdr_size - 1;  // sync hw behavior
  p_hw_stat->frm_res_sum = g_sigpool.frm_res_size;
  p_hw_stat->bit_err_accum = g_bit_error_accum;
  p_hw_stat->blk_qp_accum = g_blk_delta_qp_accum;
  p_hw_stat->cu_info_count = STAT_CU_IDX_TOTAL;
  p_hw_stat->p_cu_info = &(g_stat_cu_count[0]);
}

void top_get_vc_stat(HwStatistic *p_hw_stat)
{
  sig_ctx *p_ctx = &g_sigpool.ccu_stat_ctx;
  vc_stat_reg.VC_STAT_h00.reg_stat_frm_mse_sum = p_hw_stat->frm_mse_sum;
  vc_stat_reg.VC_STAT_h10.reg_stat_frm_max_qp  = p_hw_stat->frm_max_qp;
  vc_stat_reg.VC_STAT_h10.reg_stat_frm_min_qp  = p_hw_stat->frm_min_qp;
  vc_stat_reg.VC_STAT_h14.reg_stat_frm_qp_sum  = p_hw_stat->frm_qp_sum;

  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x00, vc_stat_reg.VC_STAT_h00.val, "frm_mse_sum");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x10, vc_stat_reg.VC_STAT_h10.val, "frm_max_min_qp");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x14, vc_stat_reg.VC_STAT_h14.val, "frm_qp_sum");

  if (p_hw_stat->p_frm_qp_hist)
  {
    for (int i = 0; i < p_hw_stat->frm_qp_hist_count; i++)
    {
      vc_stat_reg.VC_STAT_h18.reg_stat_frm_qp_hist_go = 1;
      vc_stat_reg.VC_STAT_h18.reg_stat_frm_qp_hist_idx = i;
      sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_STAT, 0x18, vc_stat_reg.VC_STAT_h18.val);

      string cmt = "frm_qp_hist" + std::to_string(i);
      vc_stat_reg.VC_STAT_h1C.reg_stat_frm_qp_hist_rd = p_hw_stat->p_frm_qp_hist[i];
      sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x1c, vc_stat_reg.VC_STAT_h1C.val, cmt);
    }
  }

  vc_stat_reg.VC_STAT_h30.reg_stat_frm_madi_sum = p_hw_stat->frm_madi_sum;
  vc_stat_reg.VC_STAT_h34.reg_stat_frm_madp_sum = p_hw_stat->frm_madp_sum;
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x30, vc_stat_reg.VC_STAT_h30.val, "frm_madi_sum");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x34, vc_stat_reg.VC_STAT_h34.val, "frm_madp_sum");

  if (p_hw_stat->p_frm_madi_hist)
  {
    for (int i = 0; i < p_hw_stat->frm_madi_hist_count; i++)
    {
      vc_stat_reg.VC_STAT_h38.reg_stat_frm_madi_hist_go = 1;
      vc_stat_reg.VC_STAT_h38.reg_stat_frm_madi_hist_idx = i;
      vc_stat_reg.VC_STAT_h38.reg_stat_frm_madi_hist_rd = 0;
      sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_STAT, 0x38, vc_stat_reg.VC_STAT_h38.val);

      string cmt = "frm_madi_hist" + std::to_string(i);
      vc_stat_reg.VC_STAT_h38.reg_stat_frm_madi_hist_go = 0;
      vc_stat_reg.VC_STAT_h38.reg_stat_frm_madi_hist_rd = p_hw_stat->p_frm_madi_hist[i];
      sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x38, vc_stat_reg.VC_STAT_h38.val, cmt);
    }
  }

  vc_stat_reg.VC_STAT_h3C.reg_stat_frm_early_term  = p_hw_stat->frm_early_term;
  vc_stat_reg.VC_STAT_h40.reg_stat_frm_fg_cu16_cnt = p_hw_stat->frm_fg_cu16_cnt;
  vc_stat_reg.VC_STAT_h4C.reg_stat_frm_bso_blen    = p_hw_stat->frm_bso_blen;
  vc_stat_reg.VC_STAT_h50.reg_stat_frm_hdr_sum     = p_hw_stat->frm_hdr_sum;
  vc_stat_reg.VC_STAT_h54.reg_stat_frm_res_sum     = p_hw_stat->frm_res_sum;
  vc_stat_reg.VC_STAT_h58.reg_bit_err_accum        = p_hw_stat->bit_err_accum;
  vc_stat_reg.VC_STAT_h5C.reg_blk_qp_accum         = p_hw_stat->blk_qp_accum;

  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x3c, vc_stat_reg.VC_STAT_h3C.val, "frm_early_term");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x40, vc_stat_reg.VC_STAT_h40.val, "frm_fg_cu16_cnt");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x4c, vc_stat_reg.VC_STAT_h4C.val, "frm_bso_blen");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x50, vc_stat_reg.VC_STAT_h50.val, "frm_hdr_sum");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x54, vc_stat_reg.VC_STAT_h54.val, "frm_res_sum");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x58, vc_stat_reg.VC_STAT_h58.val, "bit_error_accum");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x5c, vc_stat_reg.VC_STAT_h5C.val, "blk_qp_accum");

  vc_stat_reg.VC_STAT_h70.reg_ime_2cand_org_cnt = g_sigpool.cu_16x16_2_cand_count;
  vc_stat_reg.VC_STAT_h74.reg_ime_2cand_cnt = g_sigpool.cu_16x16_2_cand_count_mod;
  vc_stat_reg.VC_STAT_h78.reg_ime_2cand_abs_mvdx = g_cvi_enc_setting.enableConstrainedMVD ? (g_sigpool.abs_hor >> 2) : 0;
  vc_stat_reg.VC_STAT_h7C.reg_ime_2cand_abs_mvdy = g_cvi_enc_setting.enableConstrainedMVD ? (g_sigpool.abs_ver >> 2) : 0;

  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x70, vc_stat_reg.VC_STAT_h70.val, "frm_ime_2cand_org_cnt");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x74, vc_stat_reg.VC_STAT_h74.val, "frm_ime_2cand_cnt");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x78, vc_stat_reg.VC_STAT_h78.val, "frm_ime_2cand_abs_mvdx");
  sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x7c, vc_stat_reg.VC_STAT_h7C.val, "frm_ime_2cand_abs_mvdy");

  int addr = 0;
  for (int ch = 0; ch < 2; ch++)
  {
    for (int pred = 0; pred < 2; pred++)
    {
      for (int size = 0; size < 4; size++)
      {
        if ((addr == 12 || addr == 28 || addr == 44 || addr == 60) || // skip Luma/Chroma, Inter/Intra tu32
            (addr == 0                                           ) || // skip Luma, inter tu4
            (addr == 40 || addr == 56))                               // skip Chroma, Inter/Intra tu16
        {
          addr += 4;
          continue;
        }

        // M
        for (int bin = 0; bin < 2; bin++)
        {
          vc_stat_reg.VC_STAT_hA0.reg_stat_frm_mc_go = 1;
          vc_stat_reg.VC_STAT_hA0.reg_stat_frm_mc_idx = addr;
          sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_STAT, 0xA0, vc_stat_reg.VC_STAT_hA0.val);

          string cmt = "mc_idx_" + std::to_string(addr);
          vc_stat_reg.VC_STAT_hA4.reg_stat_frm_mc_info = (g_significantScale[0][ch][pred][size][bin] & 0x3ffff);  // 18 bits
          sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0xA4, vc_stat_reg.VC_STAT_hA4.val, cmt);
          addr++;
        }
        // C
        for (int bin = 0; bin < 2; bin++)
        {
          vc_stat_reg.VC_STAT_hA0.reg_stat_frm_mc_go = 1;
          vc_stat_reg.VC_STAT_hA0.reg_stat_frm_mc_idx = addr;
          sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_STAT, 0xA0, vc_stat_reg.VC_STAT_hA0.val);

          string cmt = "mc_idx_" + std::to_string(addr);
          vc_stat_reg.VC_STAT_hA4.reg_stat_frm_mc_info = (g_significantBias[0][ch][pred][size][bin] & 0x3ffff);  // 18 bits
          sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0xA4, vc_stat_reg.VC_STAT_hA4.val, cmt);
          addr++;
        }
      }
    }
  }

  for (int mode = 0; mode < 3; mode++)
  {
    for (int size = 0; size < 4; size++)
    {
      int cu_idx = STAT_CU_IDX[mode][size];
      if (cu_idx < 0 || cu_idx >= p_hw_stat->cu_info_count)
      {
        continue;
      }

      vc_stat_reg.VC_STAT_h100.reg_stat_frm_cu_go = 1;
      vc_stat_reg.VC_STAT_h100.reg_stat_frm_cu_idx = cu_idx;
      sig_top_reg_write(p_ctx, CALFDOZER_REG_BASE_STAT, 0x100, vc_stat_reg.VC_STAT_h100.val);

      string cmt = "cu_info_idx_" + std::to_string(cu_idx);
      vc_stat_reg.VC_STAT_h104.reg_stat_frm_cu_info = p_hw_stat->p_cu_info[cu_idx];
      sig_top_reg_read(p_ctx, CALFDOZER_REG_BASE_STAT, 0x104, vc_stat_reg.VC_STAT_h104.val, cmt);
    }
  }
}