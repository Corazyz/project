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

/** \file     TEncSlice.cpp
    \brief    slice encoder class
*/

#include "TEncTop.h"
#include "TEncSlice.h"
#include "TLibCommon/cvi_rdo_bit_est.h"
#include "TLibCommon/cvi_rate_ctrl.h"
#include "TLibCommon/cvi_frm_buf_mgr.h"
#include "TLibCommon/cvi_pattern.h"
#include "TLibCommon/cvi_motion.h"
#include "TLibCommon/cvi_float_point.h"
#include "cvi_enc.h"
#include "cvi_enc_reg.h"
#include <math.h>

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSlice::TEncSlice()
 : m_encCABACTableIdx(I_SLICE)
{
}

TEncSlice::~TEncSlice()
{
  destroy();
}

static Void sigdump_slice_reg_info(sig_ctx *p_ctx, const TComPic* pcPic, const TComSlice* pcSlice, UInt startCtuTsAddr, TEncRateCtrl *p_encRateCtrl, Bool isUseRateCtrl)
{
  int force_all_inter = 0;
  if( pcSlice->getSliceType() != I_SLICE )
  {
    if (g_algo_cfg.DisablePfrmIntra)
      force_all_inter = true;
  }
  if (pcSlice->getSliceIdx() == 0) {
    sigdump_output_fprint(p_ctx, "#PIC = %d\n", g_sigpool.enc_count_pattern);
    sigdump_output_fprint(p_ctx, "reg_pic_width_ctu_m1 = 0x%x\n", g_sigpool.widthAlignCTB - 1);
    sigdump_output_fprint(p_ctx, "reg_pic_height_ctu_m1 = 0x%x\n", g_sigpool.heightAlignCTB - 1);
    sigdump_output_fprint(p_ctx, "reg_pic_width_cu_m1 = 0x%x\n", g_sigpool.widthAlign8 - 1);
    sigdump_output_fprint(p_ctx, "reg_pic_height_cu_m1 = 0x%x\n", g_sigpool.heightAlign8 - 1);
  }

  sigdump_output_fprint(p_ctx, "#Slice = %d\n", pcSlice->getSliceIdx());
  sigdump_output_fprint(p_ctx, "reg_i_slice = 0x%x\n", pcSlice->getSliceType() == I_SLICE ? 1 : 0);
  sigdump_output_fprint(p_ctx, "reg_p_slice = 0x%x\n", pcSlice->getSliceType() == P_SLICE ? 1 : 0);
  sigdump_output_fprint(p_ctx, "reg_b_slice = 0x%x\n", pcSlice->getSliceType() == B_SLICE ? 1 : 0);
  sigdump_output_fprint(p_ctx, "reg_ctu_st_x = 0x%x\n", startCtuTsAddr % pcPic->getPicSym()->getFrameWidthInCtus());
  sigdump_output_fprint(p_ctx, "reg_ctu_st_y = 0x%x\n", startCtuTsAddr / pcPic->getPicSym()->getFrameWidthInCtus());
  sigdump_output_fprint(p_ctx, "reg_ctu_end_x = 0x%x\n", (pcSlice->getSliceSegmentCurEndCtuTsAddr() - 1) % pcPic->getPicSym()->getFrameWidthInCtus());
  sigdump_output_fprint(p_ctx, "reg_ctu_end_y = 0x%x\n", (pcSlice->getSliceSegmentCurEndCtuTsAddr() - 1) / pcPic->getPicSym()->getFrameWidthInCtus());
  SliceType sliceType   = pcSlice->getSliceType();
  SliceType  encCABACTableIdx = pcSlice->getEncCABACTableIdx();
  Bool encCabacInitFlag = (sliceType!=encCABACTableIdx && encCABACTableIdx!=I_SLICE) ? true : false;
  Int cabacInitType;
  if (pcSlice->getSliceType() == I_SLICE) {
    cabacInitType = 0;
  } else if (pcSlice->getSliceType() == P_SLICE) {
    cabacInitType = encCabacInitFlag ? 2 : 1;
  } else {
    cabacInitType = encCabacInitFlag ? 1 : 2;
  }
  sigdump_output_fprint(p_ctx, "reg_cabac_init_type = 0x%x\n", cabacInitType);
  sigdump_output_fprint(p_ctx, "reg_num_ref_l0_act_m1 = 0x%x\n", pcSlice->getNumRefIdx(REF_PIC_LIST_0) - 1);
  sigdump_output_fprint(p_ctx, "reg_enc_muti_ref_en = 0x%x\n", pcSlice->getNumRefIdx(REF_PIC_LIST_0) > 1 ? 1 : 0);
  sigdump_output_fprint(p_ctx, "reg_tmp_mvp_flag = 0x%x\n", pcSlice->getEnableTMVPFlag());
  sigdump_output_fprint(p_ctx, "reg_wb_mv_flag = 0x%x\n", ((sliceType == I_SLICE) ? 0 : 1));
  sigdump_output_fprint(p_ctx, "reg_col_ref_idx = 0x%x\n", pcSlice->getColRefIdx());

  UInt col_fb_idx = 0;
  if (pcSlice->getNumRefIdx(REF_PIC_LIST_0))
    col_fb_idx = frm_buf_mgr_find_index_by_poc(pcSlice->getRefPic(REF_PIC_LIST_0, pcSlice->getColRefIdx())->getPOC());
  sigdump_output_fprint(p_ctx, "reg_nm_col_base = 0x%lx\n", get_hw_base("mv", col_fb_idx));
  sigdump_output_fprint(p_ctx, "reg_nm_col_pit = 0x%x\n", get_hw_pitch("mv"));
  sigdump_output_fprint(p_ctx, "reg_nm_mv_base = 0x%lx\n", get_hw_base("mv", frm_buf_mgr_find_index_by_poc(pcPic->getPOC())));
  sigdump_output_fprint(p_ctx, "reg_nm_b_base = 0x%lx\n", get_hw_base("nm_b"));
  sigdump_output_fprint(p_ctx, "reg_qpmap_base = 0x%lx\n",get_hw_base("qp_map"));
  sigdump_output_fprint(p_ctx, "reg_qpmap_pit = 0x%x\n", get_hw_pitch("qp_map"));

  sigdump_output_fprint(p_ctx, "reg_slice_qp = 0x%x\n", pcSlice->getSliceQp());
  sigdump_output_fprint(p_ctx, "reg_enc_fixed_qp = 0x%x\n",  isUseRateCtrl ? 0 : 1);
  sigdump_output_fprint(p_ctx, "reg_enc_qpmap_mode = 0x%x\n", getQpMapMode());
  sigdump_output_fprint(p_ctx, "reg_enc_qp_mode = 0x%x\n", getQpMode());
  sigdump_output_fprint(p_ctx, "reg_enc_qp_win_en = 0x%x\n", getQpWinEn());

  sigdump_output_fprint(p_ctx, "reg_enc_force_skip = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_force_all_zero = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_force_all_inter = 0x%x\n", force_all_inter);

  sigdump_output_fprint(p_ctx, "reg_enc_cons_qp = 0x%x\n", 1);
  sigdump_output_fprint(p_ctx, "reg_enc_cons_delta_qp = 0x%x\n", 1);
  sigdump_output_fprint(p_ctx, "reg_enc_max_qp = 0x%x\n", getRcMaxQp(sliceType));
  sigdump_output_fprint(p_ctx, "reg_enc_min_qp = 0x%x\n", getRcMinQp(sliceType));
  sigdump_output_fprint(p_ctx, "reg_enc_max_delta_qp = 0x%x\n", 31);
  sigdump_output_fprint(p_ctx, "reg_enc_min_delta_qp = 0x%x\n", -32);
  // CU cost penalty
  sigdump_output_fprint(p_ctx, "reg_enc_intra32_cost = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_intra16_cost = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_intra8_cost = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_intra4_cost = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_inter32_cost = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_inter16_cost = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_inter8_cost = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_inter4_cost = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_skip8_cost = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_skip16_cost = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_skip32_cost = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_intra32_bias = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_inter32_bias = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_intra16_bias = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_inter16_bias = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_intra8_bias = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_inter8_bias = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_intra4_bias = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_inter4_bias = 0x%x\n", 0);

  sigdump_output_fprint(p_ctx, "reg_enc_cons_mvd_en = 0x%x\n", g_cvi_enc_setting.enableConstrainedMVD);
  sigdump_output_fprint(p_ctx, "reg_enc_ime_mvdx_thr = 0x%x\n", g_cvi_enc_setting.mvd_thr_x);
  sigdump_output_fprint(p_ctx, "reg_enc_ime_mvdy_thr = 0x%x\n", g_cvi_enc_setting.mvd_thr_y);

  sigdump_output_fprint(p_ctx, "reg_enc_cons_mv_en = 0x%x\n", g_cvi_enc_setting.enableConstrainedMV);
  sigdump_output_fprint(p_ctx, "reg_enc_cons_pux_lbnd = 0x%x\n", g_cvi_enc_setting.mv_x_lbnd & 0x7fff);
  sigdump_output_fprint(p_ctx, "reg_enc_cons_pux_ubnd = 0x%x\n", g_cvi_enc_setting.mv_x_ubnd & 0x7fff);
  sigdump_output_fprint(p_ctx, "reg_enc_cons_puy_lbnd = 0x%x\n", g_cvi_enc_setting.mv_y_lbnd & 0x7fff);
  sigdump_output_fprint(p_ctx, "reg_enc_cons_puy_ubnd = 0x%x\n", g_cvi_enc_setting.mv_y_ubnd & 0x7fff);

  int row_avg_bits = 0;
  if (isUseRateCtrl)
    row_avg_bits = p_encRateCtrl->getRCPic()->getCtuRowAvgBit();

  sigdump_output_fprint(p_ctx, "reg_row_avg_bit = 0x%x\n", row_avg_bits);
  sigdump_output_fprint(p_ctx, "reg_err_comp_scl = 0x%x\n", getBitErrCompenScal());
  sigdump_output_fprint(p_ctx, "reg_ctu_num_norm_scl = 0x%x\n", getCtuRowNormScale());
  sigdump_output_fprint(p_ctx, "reg_row_qp_delta = 0x%x\n", getRowQpClip());
  sigdump_output_fprint(p_ctx, "reg_row_ovf_qp_delta = 0x%x\n", getInRowOverflowQpdelta());

  sigdump_output_fprint(p_ctx, "reg_slice_cb_qp_ofs = 0x%x\n", pcSlice->getPPS()->getQpOffset(COMPONENT_Cb));
  sigdump_output_fprint(p_ctx, "reg_slice_cr_qp_ofs = 0x%x\n", pcSlice->getPPS()->getQpOffset(COMPONENT_Cr));

  Int lr[2] = { 0 };
  for(Int l=0; l<2; l++)
    lr[l] = ((1 << LMS_LR_FRAC_BD)*g_significantScale_LR[l]);
  sigdump_output_fprint(p_ctx, "reg_lms_lrm = 0x%x\n", lr[0]);
  sigdump_output_fprint(p_ctx, "reg_lms_lrc = 0x%x\n", lr[1]);

  Int m_max = (1 << RESI_MDL_FRAC_BD) * g_significantScale_max;
  Int m_min = (1 << RESI_MDL_FRAC_BD) * g_significantScale_min;
  Int c_max = (1 << RESI_MDL_FRAC_BD) * g_significantBias_clip;
  Int c_min = -c_max;
  sigdump_output_fprint(p_ctx, "reg_lms_m_max = 0x%x\n", m_max);
  sigdump_output_fprint(p_ctx, "reg_lms_m_min = 0x%x\n", m_min);
  sigdump_output_fprint(p_ctx, "reg_lms_c_max = 0x%x\n", c_max);
  sigdump_output_fprint(p_ctx, "reg_lms_c_min = 0x%x\n", c_min);
  REG64_C lbase, ubase;
  lbase.val = get_hw_base("bs");
  ubase.val = lbase.val + get_hw_size("bs");
  sigdump_output_fprint(p_ctx, "reg_bs_out_lbase_lsb = 0x%08x\n", lbase.reg_lsb);
  sigdump_output_fprint(p_ctx, "reg_bs_out_lbase_msb = 0x%08x\n", lbase.reg_msb);
  sigdump_output_fprint(p_ctx, "reg_bs_out_ubase_lsb = 0x%08x\n", ubase.reg_lsb);
  sigdump_output_fprint(p_ctx, "reg_bs_out_ubase_msb = 0x%08x\n", ubase.reg_msb);
  sigdump_output_fprint(p_ctx, "reg_bs_space_lbase_lsb = 0x%08x\n", lbase.reg_lsb);
  sigdump_output_fprint(p_ctx, "reg_bs_space_lbase_msb = 0x%08x\n", lbase.reg_msb);
  sigdump_output_fprint(p_ctx, "reg_bs_space_ubase_lsb = 0x%08x\n", ubase.reg_lsb);
  sigdump_output_fprint(p_ctx, "reg_bs_space_ubase_msb = 0x%08x\n", ubase.reg_msb);

  sigdump_output_fprint(p_ctx, "reg_bs_slice_hdr_en = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_bs_slice_hdr_dat = 0x%x\n", 0);
  sigdump_output_fprint(p_ctx, "reg_enc_fg_cost_en = 0x%x\n", g_algo_cfg.CostPenaltyCfg.EnableForeground);
  sigdump_output_fprint(p_ctx, "reg_enc_dis_inter_i4 = 0x%x\n", g_algo_cfg.DisablePfrmIntra4 ? 1 : 0);
  sigdump_output_fprint(p_ctx, "reg_ml_mv_gain = 0x%x\n", get_fix_point_mv_gain());
  sigdump_output_fprint(p_ctx, "reg_ml_satd_gain = 0x%x\n", get_fix_point_sad_gain());
  sigdump_output_fprint(p_ctx, "reg_fg_th_gain = 0x%x\n", get_fix_point_fg_thr_gain());
  sigdump_output_fprint(p_ctx, "reg_fg_th_bias = 0x%x\n", get_fix_point_fg_thr_bias());
  int IsCurrRefLongTerm = 0;
  if (pcSlice->getNumRefIdx(REF_PIC_LIST_0))
    IsCurrRefLongTerm = pcSlice->getRefPic(REF_PIC_LIST_0, 0)->getIsLongTerm() ? 1 : 0;
  sigdump_output_fprint(p_ctx, "reg_ref_is_long = 0x%x\n", IsCurrRefLongTerm);

  // Smart Encode
  sigdump_output_fprint(p_ctx, "reg_ai_map_en = 0x%x\n", g_algo_cfg.EnableSmartEncAI ? 1 : 0);
  sigdump_output_fprint(p_ctx, "reg_ai_smooth_qp_en = 0x%x\n", g_algo_cfg.AISmoothMap ? 1 : 0);
  sigdump_output_fprint(p_ctx, "reg_ai_dqp_base = 0x%lx\n", get_hw_base("ai_dqp"));
  sigdump_output_fprint(p_ctx, "reg_ai_dqp_pit = 0x%x\n", get_hw_pitch("ai_dqp"));
  sigdump_output_fprint(p_ctx, "reg_conf_scale = 0x%x\n", g_ai_conf_scale);
  sigdump_output_fprint(p_ctx, "reg_tclip_min = 0x%x\n", g_ai_table_idx_min);
  sigdump_output_fprint(p_ctx, "reg_tclip_max = 0x%x\n", g_ai_table_idx_max);
  sigdump_output_fprint(p_ctx, "reg_isp_map_en = 0x%x\n",
                        (g_algo_cfg.EnableSmartEncISP && g_sigpool.enc_count > 0) ? 1 : 0);
  sigdump_output_fprint(p_ctx, "reg_skip_map_en = 0x%x\n", g_algo_cfg.EnableSmartEncISPSkipMap ? 1 : 0);
  sigdump_output_fprint(p_ctx, "reg_isp_dqp_base = 0x%lx\n", get_hw_base("isp_dqp"));
  sigdump_output_fprint(p_ctx, "reg_isp_dqp_pit = 0x%x\n", get_hw_pitch("isp_dqp"));
  sigdump_output_fprint(p_ctx, "reg_trans_qp = 0x%x\n", g_isp_trans_qp);
  sigdump_output_fprint(p_ctx, "reg_motion_qp = 0x%x\n", g_isp_motion_qp);
  sigdump_output_fprint(p_ctx, "reg_clip_delta_qp_min = 0x%x\n", (g_clip_delta_qp_min & 0x7f));
  sigdump_output_fprint(p_ctx, "reg_clip_delta_qp_max = 0x%x\n", (g_clip_delta_qp_max & 0x7f));
}

static void sigdump_slice_info(const TComPic* pcPic, const TComSlice* pcSlice, UInt startCtuTsAddr, TEncRateCtrl *p_encRateCtrl, Bool isUseRateCtrl)
{
  sigdump_slicewise_enable(pcPic->getPOC());

#ifdef SIG_IME_MVP
  if (g_sigdump.ime_mvp)
  {
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "reg_ctu_st_x = %x\n", startCtuTsAddr % pcPic->getPicSym()->getFrameWidthInCtus());
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "reg_ctu_st_y = %x\n", startCtuTsAddr / pcPic->getPicSym()->getFrameWidthInCtus());
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "reg_ctu_end_x = %x\n", (pcSlice->getSliceSegmentCurEndCtuTsAddr() - 1) % pcPic->getPicSym()->getFrameWidthInCtus());
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "reg_ctu_end_y = %x\n", (pcSlice->getSliceSegmentCurEndCtuTsAddr() - 1) / pcPic->getPicSym()->getFrameWidthInCtus());
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "reg_tmp_mvp_flag = %x\n", pcSlice->getEnableTMVPFlag());
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "reg_col_ref_idx = %x\n", pcSlice->getColRefIdx());
  }
#endif
#ifdef SIG_CCU
  if (g_sigdump.ccu)
  {
    sigdump_slice_reg_info(&g_sigpool.ccu_param_ctx, pcPic, pcSlice, startCtuTsAddr, p_encRateCtrl, isUseRateCtrl);
  }
#endif
#ifdef SIG_IRPU
  if (g_sigdump.irpu)
  {
    if (g_sigpool.slice_count == 0) {
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_col_ref_idx = %x\n", pcSlice->getColRefIdx());
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_col_l0_flag = %x\n", pcSlice->getColFromL0Flag());
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_tmp_mvp_flag = %x\n", pcSlice->getEnableTMVPFlag());
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_num_ref_l0_act_m1 = %x\n", pcSlice->getNumRefIdx(REF_PIC_LIST_0) - 1);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_num_ref_l1_act_m1 = %x\n", pcSlice->getNumRefIdx(REF_PIC_LIST_1) - 1);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_max_merge_cand_m1 = %x\n", pcSlice->getMaxNumMergeCand() - 1);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_mv_gain = %x\n", get_fix_point_mv_gain());
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_enc_cons_mrg = %d\n", g_cvi_enc_setting.enableConstrainedMergeCand ? 1 : 0);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_enc_mrg_mvx_thr = %x\n", g_cvi_enc_setting.merge_mv_thr_x);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_enc_mrg_mvy_thr = %x\n", g_cvi_enc_setting.merge_mv_thr_y);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_i_slice = 0x%x\n", pcSlice->getSliceType() == I_SLICE ? 1 : 0);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_slice_go = %d\n", 1);
    }
  }
#endif
#ifdef  SIG_BIT_EST
  if (g_sigdump.rdo_sse) {
    for (int blk_size = 0; blk_size < 4; blk_size++) {
      sig_ctx *rdo_ctx = &g_sigpool.rdo_sse_cmd_ctx[blk_size];
      sigdump_output_fprint(rdo_ctx, "chromaWeight = %x\n", g_sigpool.chromaWeight);
    }
  }
#endif
#ifdef  SIG_MC
  if (g_sigdump.mc) {
      for (int i = 0; i < 3; i++)
        sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[i], "reg_dist_chroma_weight = %d\n", g_sigpool.chromaWeight);
  }
#endif
}

Void TEncSlice::create( Int iWidth, Int iHeight, ChromaFormat chromaFormat, UInt iMaxCUWidth, UInt iMaxCUHeight, UChar uhTotalDepth )
{
  // create prediction picture
  m_picYuvPred.create( iWidth, iHeight, chromaFormat, iMaxCUWidth, iMaxCUHeight, uhTotalDepth, true );

  // create residual picture
  m_picYuvResi.create( iWidth, iHeight, chromaFormat, iMaxCUWidth, iMaxCUHeight, uhTotalDepth, true );

#ifdef CVI_ALGO_CFG
  m_pSrcPad = nullptr;
  if ((iWidth & 0x1f) || (iHeight & 0x1f))
  {
    int width_align32 = cvi_mem_align(iWidth, 32);
    int height_align32 = cvi_mem_align(iHeight, 32);

    m_pSrcPad = new Pel[width_align32 * height_align32];
  }
#endif
}

Void TEncSlice::destroy()
{
  m_picYuvPred.destroy();
  m_picYuvResi.destroy();

  // free lambda and QP arrays
  m_vdRdPicLambda.clear();
  m_vdRdPicQp.clear();
  m_viRdPicQp.clear();

#ifdef CVI_ALGO_CFG
  if (m_pSrcPad)
  {
    delete[] m_pSrcPad;
    m_pSrcPad = nullptr;
  }
#endif
}

Void TEncSlice::init( TEncTop* pcEncTop )
{
  m_pcCfg             = pcEncTop;
  m_pcListPic         = pcEncTop->getListPic();

  m_pcGOPEncoder      = pcEncTop->getGOPEncoder();
  m_pcCuEncoder       = pcEncTop->getCuEncoder();
  m_pcPredSearch      = pcEncTop->getPredSearch();

  m_pcEntropyCoder    = pcEncTop->getEntropyCoder();
  m_pcSbacCoder       = pcEncTop->getSbacCoder();
  m_pcBinCABAC        = pcEncTop->getBinCABAC();
  m_pcTrQuant         = pcEncTop->getTrQuant();

  m_pcRdCost          = pcEncTop->getRdCost();
  m_pppcRDSbacCoder   = pcEncTop->getRDSbacCoder();
  m_pcRDGoOnSbacCoder = pcEncTop->getRDGoOnSbacCoder();

  // create lambda and QP arrays
  m_vdRdPicLambda.resize(m_pcCfg->getDeltaQpRD() * 2 + 1 );
  m_vdRdPicQp.resize(    m_pcCfg->getDeltaQpRD() * 2 + 1 );
  m_viRdPicQp.resize(    m_pcCfg->getDeltaQpRD() * 2 + 1 );
  m_pcRateCtrl        = pcEncTop->getRateCtrl();
}

Void TEncSlice::updateLambda(TComSlice* pSlice, Double dQP)
{
  Int iQP = (Int)dQP;
  Double dLambda = calculateLambda(pSlice, m_gopID, pSlice->getDepth(), pSlice->getSliceQp(), dQP, iQP);

  setUpLambda(pSlice, dLambda, iQP);
}

Void
TEncSlice::setUpLambda(TComSlice* slice, const Double dLambda, Int iQP)
{
  // store lambda
  m_pcRdCost ->setLambda( dLambda, sqrt(dLambda), slice->getSPS()->getBitDepths() );

  // for RDO
  // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
  Double dLambdas[MAX_NUM_COMPONENT] = { dLambda };
#ifdef CVI_FIX_POINT_RD_COST
  UChar chromaWeight = 0;
  if (g_algo_cfg.ForceChromaWeight == 0)
  {
      UInt compIdx = COMPONENT_Cb;
    const ComponentID compID = ComponentID(compIdx);
    Int chromaQPOffset = slice->getPPS()->getQpOffset(compID) + slice->getSliceChromaQpDelta(compID);
    Int qpc=(iQP + chromaQPOffset < 0) ? iQP : getScaledChromaQP(iQP + chromaQPOffset, m_pcCfg->getChromaFormatIdc());
#if SOFT_FLOAT
    RC_Float tmpWeight = SoftFloat_to_Float(CVI_FLOAT_POW(INT_TO_CVI_FLOAT(2), CVI_FLOAT_DIV( INT_TO_CVI_FLOAT(iQP-qpc), INT_TO_CVI_FLOAT(3))));  
#else
    RC_Float tmpWeight = pow( 2.0, (iQP-qpc)/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
#endif
    chromaWeight = Clip3(1, 0xff, (Int)(tmpWeight * (1 << CHROMA_WEIGHT_FRAC_BIT) + 0.5));
  }
  else
  {
    chromaWeight = (UChar)(g_algo_cfg.ForceChromaWeight);
  }

  m_pcRdCost->setDistortionWeight(chromaWeight);
  dLambdas[COMPONENT_Cb] = (dLambda * (1 << CHROMA_WEIGHT_FRAC_BIT) / chromaWeight);
  dLambdas[COMPONENT_Cr] = dLambdas[COMPONENT_Cb];
#else
  for(UInt compIdx=1; compIdx<MAX_NUM_COMPONENT; compIdx++)
  {
    const ComponentID compID=ComponentID(compIdx);
    Int chromaQPOffset = slice->getPPS()->getQpOffset(compID) + slice->getSliceChromaQpDelta(compID);
    Int qpc=(iQP + chromaQPOffset < 0) ? iQP : getScaledChromaQP(iQP + chromaQPOffset, m_pcCfg->getChromaFormatIdc());
    Double tmpWeight = pow( 2.0, (iQP-qpc)/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
    m_pcRdCost->setDistortionWeight(compID, tmpWeight);
    dLambdas[compIdx]=dLambda/tmpWeight;
  }
#endif

#if RDOQ_CHROMA_LAMBDA
// for RDOQ
  m_pcTrQuant->setLambdas( dLambdas );
#else
  m_pcTrQuant->setLambda( dLambda );
#endif

// For SAO
  slice->setLambdas( dLambdas );
}

#ifdef SIG_RRU
UChar TEncSlice::getChromaWeight()
{
  return m_pcRdCost->getDistortionWeight();
}
#endif

/**
 - non-referenced frame marking
 - QP computation based on temporal structure
 - lambda computation based on QP
 - set temporal layer ID and the parameter sets
 .
 \param pcPic         picture class
 \param pocLast       POC of last picture
 \param pocCurr       current POC
 \param iNumPicRcvd   number of received pictures
 \param iGOPid        POC offset for hierarchical structure
 \param rpcSlice      slice header class
 \param isField       true for field coding
 */

Void TEncSlice::initEncSlice( TComPic* pcPic, const Int pocLast, const Int pocCurr, const Int iGOPid, TComSlice*& rpcSlice, const Bool isField )
{
  Double dQP;
  Double dLambda;

  rpcSlice = pcPic->getSlice(0);
  rpcSlice->setSliceBits(0);
  rpcSlice->setPic( pcPic );
  rpcSlice->initSlice();
  rpcSlice->setPicOutputFlag( true );
  rpcSlice->setPOC( pocCurr );
  pcPic->setField(isField);
  m_gopID = iGOPid;

  // depth computation based on GOP size
  Int depth;
  {
    Int poc = rpcSlice->getPOC();
    if(isField)
    {
      poc = (poc/2) % (m_pcCfg->getGOPSize()/2);
    }
    else
    {
      poc = poc % m_pcCfg->getGOPSize();
    }

    if ( poc == 0 )
    {
      depth = 0;
    }
    else
    {
      Int step = m_pcCfg->getGOPSize();
      depth    = 0;
      for( Int i=step>>1; i>=1; i>>=1 )
      {
        for ( Int j=i; j<m_pcCfg->getGOPSize(); j+=step )
        {
          if ( j == poc )
          {
            i=0;
            break;
          }
        }
        step >>= 1;
        depth++;
      }
    }

    if(m_pcCfg->getHarmonizeGopFirstFieldCoupleEnabled() && poc != 0)
    {
      if (isField && ((rpcSlice->getPOC() % 2) == 1))
      {
        depth ++;
      }
    }
  }

  // slice type
  SliceType eSliceType;

  eSliceType=B_SLICE;
  if(!(isField && pocLast == 1) || !m_pcCfg->getEfficientFieldIRAPEnabled())
  {
    if(m_pcCfg->getDecodingRefreshType() == 3)
    {
      eSliceType = (pocLast == 0 || pocCurr % m_pcCfg->getIntraPeriod() == 0             || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;
    }
    else
    {
      eSliceType = (pocLast == 0 || (pocCurr - (isField ? 1 : 0)) % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;
    }
  }

  rpcSlice->setSliceType    ( eSliceType );

  // ------------------------------------------------------------------------------------------------------------------
  // Non-referenced frame marking
  // ------------------------------------------------------------------------------------------------------------------

  if(pocLast == 0)
  {
    rpcSlice->setTemporalLayerNonReferenceFlag(false);
  }
  else
  {
    rpcSlice->setTemporalLayerNonReferenceFlag(!m_pcCfg->getGOPEntry(iGOPid).m_refPic);
  }
  rpcSlice->setReferenced(true);

  // ------------------------------------------------------------------------------------------------------------------
  // QP setting
  // ------------------------------------------------------------------------------------------------------------------

  dQP = m_pcCfg->getQPForPicture(iGOPid, rpcSlice);

  // ------------------------------------------------------------------------------------------------------------------
  // Lambda computation
  // ------------------------------------------------------------------------------------------------------------------

  const Int temporalId=m_pcCfg->getGOPEntry(iGOPid).m_temporalId;
  Int iQP;
  Double dOrigQP = dQP;

  // pre-compute lambda and QP values for all possible QP candidates
  for ( Int iDQpIdx = 0; iDQpIdx < 2 * m_pcCfg->getDeltaQpRD() + 1; iDQpIdx++ )
  {
    // compute QP value
    dQP = dOrigQP + ((iDQpIdx+1)>>1)*(iDQpIdx%2 ? -1 : 1);
    dLambda = calculateLambda(rpcSlice, iGOPid, depth, dQP, dQP, iQP );
    m_vdRdPicLambda[iDQpIdx] = dLambda;
    m_vdRdPicQp    [iDQpIdx] = dQP;
    m_viRdPicQp    [iDQpIdx] = iQP;
  }

  // obtain dQP = 0 case
  dLambda = m_vdRdPicLambda[0];
  dQP     = m_vdRdPicQp    [0];
  iQP     = m_viRdPicQp    [0];

  if(rpcSlice->getPPS()->getSliceChromaQpFlag())
  {
    const Bool bUseIntraOrPeriodicOffset = rpcSlice->getSliceType()==I_SLICE || (m_pcCfg->getSliceChromaOffsetQpPeriodicity()!=0 && (rpcSlice->getPOC()%m_pcCfg->getSliceChromaOffsetQpPeriodicity())==0);
    Int cbQP = bUseIntraOrPeriodicOffset? m_pcCfg->getSliceChromaOffsetQpIntraOrPeriodic(false) : m_pcCfg->getGOPEntry(iGOPid).m_CbQPoffset;
    Int crQP = bUseIntraOrPeriodicOffset? m_pcCfg->getSliceChromaOffsetQpIntraOrPeriodic(true)  : m_pcCfg->getGOPEntry(iGOPid).m_CrQPoffset;

    cbQP = Clip3( -12, 12, cbQP + rpcSlice->getPPS()->getQpOffset(COMPONENT_Cb) ) - rpcSlice->getPPS()->getQpOffset(COMPONENT_Cb);
    crQP = Clip3( -12, 12, crQP + rpcSlice->getPPS()->getQpOffset(COMPONENT_Cr) ) - rpcSlice->getPPS()->getQpOffset(COMPONENT_Cr);
    rpcSlice->setSliceChromaQpDelta(COMPONENT_Cb, Clip3( -12, 12, cbQP));
    assert(rpcSlice->getSliceChromaQpDelta(COMPONENT_Cb)+rpcSlice->getPPS()->getQpOffset(COMPONENT_Cb)<=12 && rpcSlice->getSliceChromaQpDelta(COMPONENT_Cb)+rpcSlice->getPPS()->getQpOffset(COMPONENT_Cb)>=-12);
    rpcSlice->setSliceChromaQpDelta(COMPONENT_Cr, Clip3( -12, 12, crQP));
    assert(rpcSlice->getSliceChromaQpDelta(COMPONENT_Cr)+rpcSlice->getPPS()->getQpOffset(COMPONENT_Cr)<=12 && rpcSlice->getSliceChromaQpDelta(COMPONENT_Cr)+rpcSlice->getPPS()->getQpOffset(COMPONENT_Cr)>=-12);
  }
  else
  {
    rpcSlice->setSliceChromaQpDelta( COMPONENT_Cb, 0 );
    rpcSlice->setSliceChromaQpDelta( COMPONENT_Cr, 0 );
  }


  setUpLambda(rpcSlice, dLambda, iQP);

#ifdef CVI_HW_RC
  // Set lambda for fix QP mode
  g_qp_to_lambda_table[iQP] = m_pcRdCost->getLambda();
  g_qp_to_sqrt_lambda_table[iQP] = m_pcRdCost->getSqrtLambda();
#endif

  if (m_pcCfg->getFastMEForGenBLowDelayEnabled())
  {
    // restore original slice type

    if(!(isField && pocLast == 1) || !m_pcCfg->getEfficientFieldIRAPEnabled())
    {
      if(m_pcCfg->getDecodingRefreshType() == 3)
      {
        eSliceType = (pocLast == 0 || (pocCurr)                     % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;
      }
      else
      {
        eSliceType = (pocLast == 0 || (pocCurr - (isField ? 1 : 0)) % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;
      }
    }

    rpcSlice->setSliceType        ( eSliceType );
  }

  if (m_pcCfg->getUseRecalculateQPAccordingToLambda())
  {
    dQP = xGetQPValueAccordingToLambda( dLambda );
    iQP = max( -rpcSlice->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), min( MAX_QP, (Int) floor( dQP + 0.5 ) ) );
  }

  rpcSlice->setSliceQp           ( iQP );
#if ADAPTIVE_QP_SELECTION
  rpcSlice->setSliceQpBase       ( iQP );
#endif
  rpcSlice->setSliceQpDelta      ( 0 );
  rpcSlice->setUseChromaQpAdj( rpcSlice->getPPS()->getPpsRangeExtension().getChromaQpOffsetListEnabledFlag() );
  rpcSlice->setNumRefIdx(REF_PIC_LIST_0,m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive);
  rpcSlice->setNumRefIdx(REF_PIC_LIST_1,m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive);

  if ( m_pcCfg->getDeblockingFilterMetric() )
  {
    rpcSlice->setDeblockingFilterOverrideFlag(true);
    rpcSlice->setDeblockingFilterDisable(false);
    rpcSlice->setDeblockingFilterBetaOffsetDiv2( 0 );
    rpcSlice->setDeblockingFilterTcOffsetDiv2( 0 );
  }
  else if (rpcSlice->getPPS()->getDeblockingFilterControlPresentFlag())
  {
    rpcSlice->setDeblockingFilterOverrideFlag( rpcSlice->getPPS()->getDeblockingFilterOverrideEnabledFlag() );
    rpcSlice->setDeblockingFilterDisable( rpcSlice->getPPS()->getPPSDeblockingFilterDisabledFlag() );
    if ( !rpcSlice->getDeblockingFilterDisable())
    {
      if ( rpcSlice->getDeblockingFilterOverrideFlag() && eSliceType!=I_SLICE)
      {
        rpcSlice->setDeblockingFilterBetaOffsetDiv2( m_pcCfg->getGOPEntry(iGOPid).m_betaOffsetDiv2 + m_pcCfg->getLoopFilterBetaOffset()  );
        rpcSlice->setDeblockingFilterTcOffsetDiv2( m_pcCfg->getGOPEntry(iGOPid).m_tcOffsetDiv2 + m_pcCfg->getLoopFilterTcOffset() );
      }
      else
      {
        rpcSlice->setDeblockingFilterBetaOffsetDiv2( m_pcCfg->getLoopFilterBetaOffset() );
        rpcSlice->setDeblockingFilterTcOffsetDiv2( m_pcCfg->getLoopFilterTcOffset() );
      }
    }
  }
  else
  {
    rpcSlice->setDeblockingFilterOverrideFlag( false );
    rpcSlice->setDeblockingFilterDisable( false );
    rpcSlice->setDeblockingFilterBetaOffsetDiv2( 0 );
    rpcSlice->setDeblockingFilterTcOffsetDiv2( 0 );
  }

  rpcSlice->setDepth            ( depth );

  pcPic->setTLayer( temporalId );
  if(eSliceType==I_SLICE)
  {
    pcPic->setTLayer(0);
  }
  rpcSlice->setTLayer( pcPic->getTLayer() );

  pcPic->setPicYuvPred( &m_picYuvPred );
  pcPic->setPicYuvResi( &m_picYuvResi );
  rpcSlice->setSliceMode            ( m_pcCfg->getSliceMode()            );
  rpcSlice->setSliceArgument        ( m_pcCfg->getSliceArgument()        );
  rpcSlice->setSliceSegmentMode     ( m_pcCfg->getSliceSegmentMode()     );
  rpcSlice->setSliceSegmentArgument ( m_pcCfg->getSliceSegmentArgument() );
  rpcSlice->setMaxNumMergeCand      ( m_pcCfg->getMaxNumMergeCand()      );
}


Double TEncSlice::calculateLambda( const TComSlice* slice,
                                   const Int        GOPid, // entry in the GOP table
                                   const Int        depth, // slice GOP hierarchical depth.
                                   const Double     refQP, // initial slice-level QP
                                   const Double     dQP,   // initial double-precision QP
                                         Int       &iQP )  // returned integer QP.
{
  enum   SliceType eSliceType    = slice->getSliceType();
  const  Bool      isField       = slice->getPic()->isField();
  const  Int       NumberBFrames = ( m_pcCfg->getGOPSize() - 1 );
  const  Int       SHIFT_QP      = 12;
  const Int temporalId=m_pcCfg->getGOPEntry(GOPid).m_temporalId;
  const std::vector<Double> &intraLambdaModifiers=m_pcCfg->getIntraLambdaModifier();

#if FULL_NBIT
  Int    bitdepth_luma_qp_scale = 6 * (slice->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) - 8);
#else
  Int    bitdepth_luma_qp_scale = 0;
#endif
  Double qp_temp = dQP + bitdepth_luma_qp_scale - SHIFT_QP;
  // Case #1: I or P-slices (key-frame)
  Double dQPFactor = m_pcCfg->getGOPEntry(GOPid).m_QPFactor;
  if ( eSliceType==I_SLICE )
  {
    if (m_pcCfg->getIntraQpFactor()>=0.0 && m_pcCfg->getGOPEntry(GOPid).m_sliceType != I_SLICE)
    {
      dQPFactor=m_pcCfg->getIntraQpFactor();
    }
    else
    {
      if(m_pcCfg->getLambdaFromQPEnable())
      {
        dQPFactor=0.57;
      }
      else
      {
        Double dLambda_scale = 1.0 - Clip3( 0.0, 0.5, 0.05*(Double)(isField ? NumberBFrames/2 : NumberBFrames) );
        dQPFactor=0.57*dLambda_scale;
      }
    }
  }
  else if( m_pcCfg->getLambdaFromQPEnable() )
  {
    dQPFactor=0.57;
  }

  Double dLambda = dQPFactor*pow( 2.0, qp_temp/3.0 );

  if( !(m_pcCfg->getLambdaFromQPEnable()) && depth>0 )
  {
#if FULL_NBIT
      Double qp_temp_ref_orig = refQP - SHIFT_QP;
      dLambda *= Clip3( 2.00, 4.00, (qp_temp_ref_orig / 6.0) ); // (j == B_SLICE && p_cur_frm->layer != 0 )
#else
      Double qp_temp_ref = refQP + bitdepth_luma_qp_scale - SHIFT_QP;
      dLambda *= Clip3( 2.00, 4.00, (qp_temp_ref / 6.0) ); // (j == B_SLICE && p_cur_frm->layer != 0 )
#endif
  }

  // if hadamard is used in ME process
  if ( !m_pcCfg->getUseHADME() && slice->getSliceType( ) != I_SLICE )
  {
    dLambda *= 0.95;
  }

  Double lambdaModifier;
  if( eSliceType != I_SLICE || intraLambdaModifiers.empty())
  {
    lambdaModifier = m_pcCfg->getLambdaModifier( temporalId );
  }
  else
  {
    lambdaModifier = intraLambdaModifiers[ (temporalId < intraLambdaModifiers.size()) ? temporalId : (intraLambdaModifiers.size()-1) ];
  }
  dLambda *= lambdaModifier;

  iQP = max( -slice->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), min( MAX_QP, (Int) floor( dQP + 0.5 ) ) );

  // NOTE: the lambda modifiers that are sometimes applied later might be best always applied in here.
  return dLambda;
}

Void TEncSlice::resetQP( TComPic* pic, Int sliceQP, Double lambda )
{
  TComSlice* slice = pic->getSlice(0);

  // store lambda
  slice->setSliceQp( sliceQP );
#if ADAPTIVE_QP_SELECTION
  slice->setSliceQpBase ( sliceQP );
#endif
  setUpLambda(slice, lambda, sliceQP);
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

//! set adaptive search range based on poc difference
Void TEncSlice::setSearchRange( TComSlice* pcSlice )
{
  Int iCurrPOC = pcSlice->getPOC();
  Int iRefPOC;
  Int iGOPSize = m_pcCfg->getGOPSize();
  Int iOffset = (iGOPSize >> 1);
  Int iMaxSR = m_pcCfg->getSearchRange();
  Int iNumPredDir = pcSlice->isInterP() ? 1 : 2;

  for (Int iDir = 0; iDir < iNumPredDir; iDir++)
  {
    RefPicList  e = ( iDir ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
    for (Int iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(e); iRefIdx++)
    {
      iRefPOC = pcSlice->getRefPic(e, iRefIdx)->getPOC();
      Int newSearchRange = Clip3(m_pcCfg->getMinSearchWindow(), iMaxSR, (iMaxSR*ADAPT_SR_SCALE*abs(iCurrPOC - iRefPOC)+iOffset)/iGOPSize);
      m_pcPredSearch->setAdaptiveSearchRange(iDir, iRefIdx, newSearchRange);
    }
  }
}

/**
 Multi-loop slice encoding for different slice QP

 \param pcPic    picture class
 */
Void TEncSlice::precompressSlice( TComPic* pcPic )
{
  // if deltaQP RD is not used, simply return
  if ( m_pcCfg->getDeltaQpRD() == 0 )
  {
    return;
  }

  if ( m_pcCfg->getUseRateCtrl() )
  {
    printf( "\nMultiple QP optimization is not allowed when rate control is enabled." );
    assert(0);
    return;
  }

  TComSlice* pcSlice        = pcPic->getSlice(getSliceIdx());

  if (pcSlice->getDependentSliceSegmentFlag())
  {
    // if this is a dependent slice segment, then it was optimised
    // when analysing the entire slice.
    return;
  }

  if (pcSlice->getSliceMode()==FIXED_NUMBER_OF_BYTES)
  {
    // TODO: investigate use of average cost per CTU so that this Slice Mode can be used.
    printf( "\nUnable to optimise Slice-level QP if Slice Mode is set to FIXED_NUMBER_OF_BYTES\n" );
    assert(0);
    return;
  }

  Double     dPicRdCostBest = MAX_DOUBLE;
  UInt       uiQpIdxBest = 0;

  Double dFrameLambda;
#if FULL_NBIT
  Int    SHIFT_QP = 12 + 6 * (pcSlice->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) - 8);
#else
  Int    SHIFT_QP = 12;
#endif

  // set frame lambda
  if (m_pcCfg->getGOPSize() > 1)
  {
    dFrameLambda = 0.68 * pow (2, (m_viRdPicQp[0]  - SHIFT_QP) / 3.0) * (pcSlice->isInterB()? 2 : 1);
  }
  else
  {
    dFrameLambda = 0.68 * pow (2, (m_viRdPicQp[0] - SHIFT_QP) / 3.0);
  }
  m_pcRdCost      ->setFrameLambda(dFrameLambda);

  // for each QP candidate
  for ( UInt uiQpIdx = 0; uiQpIdx < 2 * m_pcCfg->getDeltaQpRD() + 1; uiQpIdx++ )
  {
    pcSlice       ->setSliceQp             ( m_viRdPicQp    [uiQpIdx] );
#if ADAPTIVE_QP_SELECTION
    pcSlice       ->setSliceQpBase         ( m_viRdPicQp    [uiQpIdx] );
#endif
    setUpLambda(pcSlice, m_vdRdPicLambda[uiQpIdx], m_viRdPicQp    [uiQpIdx]);

    // try compress
    compressSlice   ( pcPic, true, m_pcCfg->getFastDeltaQp());

    UInt64 uiPicDist        = m_uiPicDist; // Distortion, as calculated by compressSlice.
    // NOTE: This distortion is the chroma-weighted SSE distortion for the slice.
    //       Previously a standard SSE distortion was calculated (for the entire frame).
    //       Which is correct?

    // TODO: Update loop filter, SAO and distortion calculation to work on one slice only.
    // m_pcGOPEncoder->preLoopFilterPicAll( pcPic, uiPicDist );

    // compute RD cost and choose the best
    Double dPicRdCost = m_pcRdCost->calcRdCost( (Double)m_uiPicTotalBits, uiPicDist, DF_SSE_FRAME);

    if ( dPicRdCost < dPicRdCostBest )
    {
      uiQpIdxBest    = uiQpIdx;
      dPicRdCostBest = dPicRdCost;
    }
  }

  // set best values
  pcSlice       ->setSliceQp             ( m_viRdPicQp    [uiQpIdxBest] );
#if ADAPTIVE_QP_SELECTION
  pcSlice       ->setSliceQpBase         ( m_viRdPicQp    [uiQpIdxBest] );
#endif
  setUpLambda(pcSlice, m_vdRdPicLambda[uiQpIdxBest], m_viRdPicQp    [uiQpIdxBest]);
}

Void TEncSlice::calCostSliceI(TComPic* pcPic) // TODO: this only analyses the first slice segment. What about the others?
{
  Double            iSumHadSlice      = 0;
  TComSlice * const pcSlice           = pcPic->getSlice(getSliceIdx());
  const TComSPS    &sps               = *(pcSlice->getSPS());
  const Int         shift             = sps.getBitDepth(CHANNEL_TYPE_LUMA)-8;
  const Int         offset            = (shift>0)?(1<<(shift-1)):0;

  pcSlice->setSliceSegmentBits(0);

  UInt startCtuTsAddr, boundingCtuTsAddr;
  xDetermineStartAndBoundingCtuTsAddr ( startCtuTsAddr, boundingCtuTsAddr, pcPic );

  for( UInt ctuTsAddr = startCtuTsAddr, ctuRsAddr = pcPic->getPicSym()->getCtuTsToRsAddrMap( startCtuTsAddr);
       ctuTsAddr < boundingCtuTsAddr;
       ctuRsAddr = pcPic->getPicSym()->getCtuTsToRsAddrMap(++ctuTsAddr) )
  {
    // initialize CU encoder
    TComDataCU* pCtu = pcPic->getCtu( ctuRsAddr );
    pCtu->initCtu( pcPic, ctuRsAddr );

    Int height  = min( sps.getMaxCUHeight(),sps.getPicHeightInLumaSamples() - ctuRsAddr / pcPic->getFrameWidthInCtus() * sps.getMaxCUHeight() );
    Int width   = min( sps.getMaxCUWidth(), sps.getPicWidthInLumaSamples()  - ctuRsAddr % pcPic->getFrameWidthInCtus() * sps.getMaxCUWidth() );

    Int iSumHad = m_pcCuEncoder->updateCtuDataISlice(pCtu, width, height);

    (m_pcRateCtrl->getRCPic()->getLCU(ctuRsAddr)).m_costIntra=(iSumHad+offset)>>shift;
    iSumHadSlice += (m_pcRateCtrl->getRCPic()->getLCU(ctuRsAddr)).m_costIntra;

  }
  m_pcRateCtrl->getRCPic()->setTotalIntraCost(iSumHadSlice);
}

/** \param pcPic   picture class
 */
Void TEncSlice::compressSlice( TComPic* pcPic, const Bool bCompressEntireSlice, const Bool bFastDeltaQP )
{
  // if bCompressEntireSlice is true, then the entire slice (not slice segment) is compressed,
  //   effectively disabling the slice-segment-mode.

  UInt   startCtuTsAddr, numberOfWrittenBits;
  UInt   boundingCtuTsAddr;
  TComSlice* const pcSlice            = pcPic->getSlice(getSliceIdx());
  pcSlice->setSliceSegmentBits(0);
  xDetermineStartAndBoundingCtuTsAddr ( startCtuTsAddr, boundingCtuTsAddr, pcPic );
  if (bCompressEntireSlice)
  {
    boundingCtuTsAddr = pcSlice->getSliceCurEndCtuTsAddr();
    pcSlice->setSliceSegmentCurEndCtuTsAddr(boundingCtuTsAddr);
  }

  // initialize cost values - these are used by precompressSlice (they should be parameters).
  m_uiPicTotalBits  = 0;
  m_dPicRdCost      = 0; // NOTE: This is a write-only variable!
  m_uiPicDist       = 0;

#ifdef CVI_RC_USE_REAL_BIT
  m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
  m_pcEntropyCoder->setEntropyCoder   (m_pcSbacCoder);
  m_pcEntropyCoder->resetEntropy      ( pcSlice );
  TComBitCounter  dummyBitCounter;
  dummyBitCounter.resetBits();
#endif
#ifdef SIGDUMP
  g_sigpool.chromaWeight = m_pcRdCost->getChromaWeight();
  sigdump_slice_info(pcPic, pcSlice, startCtuTsAddr, m_pcRateCtrl, m_pcCfg->getUseRateCtrl());
#endif
  m_pcEntropyCoder->setEntropyCoder   ( m_pppcRDSbacCoder[0][CI_CURR_BEST] );
  m_pcEntropyCoder->resetEntropy      ( pcSlice );

  TEncBinCABAC* pRDSbacCoder = (TEncBinCABAC *) m_pppcRDSbacCoder[0][CI_CURR_BEST]->getEncBinIf();
  pRDSbacCoder->setBinCountingEnableFlag( false );
  pRDSbacCoder->setBinsCoded( 0 );

  TComBitCounter  tempBitCounter;
  const UInt      frameWidthInCtus = pcPic->getPicSym()->getFrameWidthInCtus();

  m_pcCuEncoder->setFastDeltaQp(bFastDeltaQP);

#ifdef CVI_SMART_ENC
  if(g_algo_cfg.EnableCalMapData)
    gp_isp_enc_param->is_entropy = 0;
#endif
  //------------------------------------------------------------------------------
  //  Weighted Prediction parameters estimation.
  //------------------------------------------------------------------------------
  // calculate AC/DC values for current picture
  if( pcSlice->getPPS()->getUseWP() || pcSlice->getPPS()->getWPBiPred() )
  {
    xCalcACDCParamSlice(pcSlice);
  }

  const Bool bWp_explicit = (pcSlice->getSliceType()==P_SLICE && pcSlice->getPPS()->getUseWP()) || (pcSlice->getSliceType()==B_SLICE && pcSlice->getPPS()->getWPBiPred());

  if ( bWp_explicit )
  {
    //------------------------------------------------------------------------------
    //  Weighted Prediction implemented at Slice level. SliceMode=2 is not supported yet.
    //------------------------------------------------------------------------------
    if ( pcSlice->getSliceMode()==FIXED_NUMBER_OF_BYTES || pcSlice->getSliceSegmentMode()==FIXED_NUMBER_OF_BYTES )
    {
      printf("Weighted Prediction is not supported with slice mode determined by max number of bins.\n"); exit(0);
    }

    xEstimateWPParamSlice( pcSlice, m_pcCfg->getWeightedPredictionMethod() );
    pcSlice->initWpScaling(pcSlice->getSPS());

    // check WP on/off
    xCheckWPEnable( pcSlice );
  }

#if ADAPTIVE_QP_SELECTION
  if( m_pcCfg->getUseAdaptQpSelect() && !(pcSlice->getDependentSliceSegmentFlag()))
  {
    // TODO: this won't work with dependent slices: they do not have their own QP. Check fix to mask clause execution with && !(pcSlice->getDependentSliceSegmentFlag())
    m_pcTrQuant->clearSliceARLCnt(); // TODO: this looks wrong for multiple slices - the results of all but the last slice will be cleared before they are used (all slices compressed, and then all slices encoded)
    if(pcSlice->getSliceType()!=I_SLICE)
    {
      Int qpBase = pcSlice->getSliceQpBase();
      pcSlice->setSliceQp(qpBase + m_pcTrQuant->getQpDelta(qpBase));
    }
  }
#endif

#ifdef CVI_ENABLE_RDO_BIT_EST
  if(getRDOBitEstMode()==CTX_ADAPTIVE) {
#ifdef SIGDUMP
    if ((pcSlice->getSliceType()==I_SLICE && g_sigdump.reset_resi_model == 1) || g_sigdump.reset_resi_model == 2)
      setupResiModel(0);
    else
#endif
      setupResiModel(pcSlice->getPOC());
  }
#endif
#ifdef SIG_CABAC
  if (g_sigdump.cabac)
  {
    if (pcSlice->getSliceIdx() == 0) {
      sig_cabac_syntax_init_tab();
    }
  }
#endif
#ifdef SIG_CCU
  if (g_sigdump.ccu)
  {
    if (pcSlice->getSliceIdx() == 0)
    {
      sig_ccu_output_init_tab();
      sig_ccu_output_qpmap();
    }
  }
#endif //~SIG_CCU

#ifdef SIG_TOP
  if (g_sigdump.top)
  {
    top_set_all_reg(pcSlice, m_pcCfg->getUseRateCtrl() ? m_pcRateCtrl : nullptr);

    if (!g_sigdump.fpga && pcSlice->getSliceIdx() == 0 && pcSlice->getSliceType() < I_SLICE)
    {
      // Reserve flexibility for mutiple ref and for B slice.
      int list_num = pcSlice->isInterP() ? 1 : 2;

      for (int list = 0; list < list_num; list++)
      {
        RefPicList eRefPicList = ( list ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
        int ref_num = pcSlice->getNumRefIdx(eRefPicList);

        for (int ref_idx = 0; ref_idx < ref_num; ref_idx++)
        {
          int ref_poc = pcSlice->getRefPic(eRefPicList, ref_idx)->getPOC();
          int fb_idx  = frm_buf_mgr_find_index_by_poc(ref_poc);
          sigdump_open_top_ref_ctx(fb_idx);

          TComPicYuv *p_ref_yuv = pcSlice->getRefPic(eRefPicList, ref_idx)->getPicYuvRec();
          if (g_sigpool.top_ref_ctx.fp)
          {
            p_ref_yuv->writeNV12(p_ref_yuv, g_sigpool.top_ref_ctx.fp, g_sigpool.fb_pitch, 0, 0);
          }
          sigdump_close_top_ref_ctx();
        }
      }
    }
  }
#endif //~SIG_TOP
#ifdef SIG_PPU
  if (g_sigdump.ppu)
  {
    sig_ppu_output_reg(pcSlice);
  }
#endif //~SIG_PPU

  // Adjust initial state if this is the start of a dependent slice.
  {
    const UInt      ctuRsAddr               = pcPic->getPicSym()->getCtuTsToRsAddrMap( startCtuTsAddr);
    const UInt      currentTileIdx          = pcPic->getPicSym()->getTileIdxMap(ctuRsAddr);
    const TComTile *pCurrentTile            = pcPic->getPicSym()->getTComTile(currentTileIdx);
    const UInt      firstCtuRsAddrOfTile    = pCurrentTile->getFirstCtuRsAddr();
    if( pcSlice->getDependentSliceSegmentFlag() && ctuRsAddr != firstCtuRsAddrOfTile )
    {
      // This will only occur if dependent slice-segments (m_entropyCodingSyncContextState=true) are being used.
      if( pCurrentTile->getTileWidthInCtus() >= 2 || !m_pcCfg->getEntropyCodingSyncEnabledFlag() )
      {
        m_pppcRDSbacCoder[0][CI_CURR_BEST]->loadContexts( &m_lastSliceSegmentEndContextState );
      }
    }
  }

#ifdef SIGDUMP
  g_sigpool.slice_type = pcSlice->getSliceType();
#endif

#ifdef CVI_FOREGROUND_QP
  if (g_algo_cfg.CostPenaltyCfg.EnableForeground && m_pcCfg->getUseRateCtrl())
  {
    set_foreground_qp_valid(false);
    if (pcSlice->getSliceType() != I_SLICE &&
        pcSlice->getRefPic(REF_PIC_LIST_0, 0)->getSlice(0)->getSliceType() != I_SLICE)
    {
      set_foreground_qp_valid(true);
    }
  }
#endif //~CVI_FOREGROUND_QP

  // for every CTU in the slice segment (may terminate sooner if there is a byte limit on the slice-segment)

  for( UInt ctuTsAddr = startCtuTsAddr; ctuTsAddr < boundingCtuTsAddr; ++ctuTsAddr )
  {
    const UInt ctuRsAddr = pcPic->getPicSym()->getCtuTsToRsAddrMap(ctuTsAddr);
    // initialize CTU encoder
    TComDataCU* pCtu = pcPic->getCtu( ctuRsAddr );
    pCtu->initCtu( pcPic, ctuRsAddr );
#ifdef SIGDUMP
    g_sigpool.ctb_idx_x = ( ctuRsAddr % pcPic->getFrameWidthInCtus() ) * pcPic->getPicSym()->getSPS().getMaxCUWidth();
    g_sigpool.ctb_idx_y = ( ctuRsAddr / pcPic->getFrameWidthInCtus() ) * pcPic->getPicSym()->getSPS().getMaxCUHeight();
    sigdump_ctb_start();
#endif
#ifdef SIG_IRPU
    if (g_sigdump.irpu && ctuTsAddr == startCtuTsAddr) {
      for (Int refIdx = 0; refIdx < pCtu->getSlice()->getNumRefIdx(REF_PIC_LIST_0); refIdx++) {
        sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "%x %x %x\n", refIdx,
        pCtu->getSlice()->getIsUsedAsLongTerm(REF_PIC_LIST_0, refIdx),
        pCtu->getSlice()->getRefPOC( REF_PIC_LIST_0, refIdx));
      }
    }
#endif

    // update CABAC state
    const UInt firstCtuRsAddrOfTile = pcPic->getPicSym()->getTComTile(pcPic->getPicSym()->getTileIdxMap(ctuRsAddr))->getFirstCtuRsAddr();
    const UInt tileXPosInCtus = firstCtuRsAddrOfTile % frameWidthInCtus;
    const UInt ctuXPosInCtus  = ctuRsAddr % frameWidthInCtus;

    if (ctuRsAddr == firstCtuRsAddrOfTile)
    {
      m_pppcRDSbacCoder[0][CI_CURR_BEST]->resetEntropy(pcSlice);
    }
    else if ( ctuXPosInCtus == tileXPosInCtus && m_pcCfg->getEntropyCodingSyncEnabledFlag())
    {
      // reset and then update contexts to the state at the end of the top-right CTU (if within current slice and tile).
      m_pppcRDSbacCoder[0][CI_CURR_BEST]->resetEntropy(pcSlice);
      // Sync if the Top-Right is available.
      TComDataCU *pCtuUp = pCtu->getCtuAbove();
      if ( pCtuUp && ((ctuRsAddr%frameWidthInCtus+1) < frameWidthInCtus)  )
      {
        TComDataCU *pCtuTR = pcPic->getCtu( ctuRsAddr - frameWidthInCtus + 1 );
        if ( pCtu->CUIsFromSameSliceAndTile(pCtuTR) )
        {
          // Top-Right is available, we use it.
          m_pppcRDSbacCoder[0][CI_CURR_BEST]->loadContexts( &m_entropyCodingSyncContextState );
        }
      }
    }

    // set go-on entropy coder (used for all trial encodings - the cu encoder and encoder search also have a copy of the same pointer)
    m_pcEntropyCoder->setEntropyCoder ( m_pcRDGoOnSbacCoder );
    m_pcEntropyCoder->setBitstream( &tempBitCounter );
    tempBitCounter.resetBits();
    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[0][CI_CURR_BEST] ); // this copy is not strictly necessary here, but indicates that the GoOnSbacCoder
                                                                     // is reset to a known state before every decision process.

    ((TEncBinCABAC*)m_pcRDGoOnSbacCoder->getEncBinIf())->setBinCountingEnableFlag(true);

    if(getRDOBitEstMode()==CTX_ADAPTIVE) {
      setResiModelState(RESI_MODEL_TEST);
    }

    Double oldLambda = m_pcRdCost->getLambda();
    Double oldSqrtLambda = m_pcRdCost->getSqrtLambda();
    if ( m_pcCfg->getUseRateCtrl() )
    {
      Int estQP        = pcSlice->getSliceQp();
      Double estLambda = -1.0;
      SliceType eSliceType = pcPic->getSlice( 0 )->getSliceType();
      if ( ( eSliceType == I_SLICE && m_pcCfg->getForceIntraQP() ) || !m_pcCfg->getLCULevelRC() )
      {
        estQP = pcSlice->getSliceQp();
      }
      else
      {
#ifdef CVI_HW_RC
        bool isCtuRowStart = ctuTsAddr % pcPic->getFrameWidthInCtus()==0;
        if(isCtuRowStart) {
          Int rowIdx = ctuTsAddr / pcPic->getFrameWidthInCtus();
          Int targetBit = m_pcRateCtrl->getRCPic()->hw_getCTULowTargetBit(rowIdx);
          m_pcRateCtrl->getRCPic()->hw_calcCTULowBaseQp(targetBit);
        }

        // perceptual
        estQP = m_pcRateCtrl->getRCPic()->hw_getCTUBaseQp();
        m_pcRateCtrl->getRCPic()->genAllCUQp(estQP, pCtu->getCUPelX(), pCtu->getCUPelY());
#else
        Double bpp = m_pcRateCtrl->getRCPic()->getLCUTargetBpp(eSliceType);
        if ( pcPic->getSlice( 0 )->getSliceType() == I_SLICE)
        {
          estLambda = m_pcRateCtrl->getRCPic()->getLCUEstLambdaAndQP(bpp, pcSlice->getSliceQp(), &estQP);
        }
        else
        {
          estLambda = m_pcRateCtrl->getRCPic()->getLCUEstLambda( bpp );
          estQP     = m_pcRateCtrl->getRCPic()->getLCUEstQP    ( estLambda, pcSlice->getSliceQp() );
        }
        estQP     = Clip3( -pcSlice->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, estQP );
#endif

        m_pcRdCost->setLambda(estLambda, sqrt(estLambda), pcSlice->getSPS()->getBitDepths());

#if RDOQ_CHROMA_LAMBDA
        // set lambda for RDOQ
        const Double chromaLambda = estLambda / m_pcRdCost->getChromaWeight();
        const Double lambdaArray[MAX_NUM_COMPONENT] = { estLambda, chromaLambda, chromaLambda };
        m_pcTrQuant->setLambdas( lambdaArray );
#else
        m_pcTrQuant->setLambda( estLambda );
#endif
      }

      m_pcRateCtrl->setRCQP( estQP );
#if ADAPTIVE_QP_SELECTION
      pCtu->getSlice()->setSliceQpBase( estQP );
#endif
    }

    // run CTU trial encoder
    m_pcCuEncoder->compressCtu( pCtu );

    // All CTU decisions have now been made. Restore entropy coder to an initial stage, ready to make a true encode,
    // which will result in the state of the contexts being correct. It will also count up the number of bits coded,
    // which is used if there is a limit of the number of bytes per slice-segment.

#ifdef CVI_RC_USE_REAL_BIT
    m_pppcRDSbacCoder[0][CI_CURR_BEST]->init( (TEncBinIf*)m_pcBinCABAC );
    m_pcEntropyCoder->setEntropyCoder ( m_pppcRDSbacCoder[0][CI_CURR_BEST] );
    m_pcEntropyCoder->setBitstream( &dummyBitCounter );
#else
    m_pcEntropyCoder->setEntropyCoder ( m_pppcRDSbacCoder[0][CI_CURR_BEST] );
    m_pcEntropyCoder->setBitstream( &tempBitCounter );
    m_pppcRDSbacCoder[0][CI_CURR_BEST]->resetBits();
#endif
    pRDSbacCoder->setBinCountingEnableFlag( true );
    pRDSbacCoder->setBinsCoded( 0 );

    // encode CTU and calculate the true bit counters.
    Int ctu_bit_pos = 0;
#ifdef CVI_RC_USE_REAL_BIT
    ctu_bit_pos = m_pcEntropyCoder->getNumberOfWrittenBits();
#endif
#ifdef CVI_HW_RC
    g_updata_stat_frm_en = true;
#endif
#ifdef SIG_CCU
    if (g_sigdump.ccu)
    {
      g_sigpool.ccu_is_record_cu_bits = true;
      g_sigpool.p_ccu_rc_st->cu32_start_bits = ctu_bit_pos;
    }
#endif
#ifdef SIG_RRU
    g_sigpool.rru_is_record_pos = true;
#endif
    m_pcCuEncoder->encodeCtu( pCtu );
#ifdef CVI_HW_RC
    g_updata_stat_frm_en = false;
#endif
#ifdef SIG_CCU
    g_sigpool.ccu_is_record_cu_bits = false;
#endif
#ifdef SIG_RRU
    g_sigpool.rru_is_record_pos = false;
#endif
    pRDSbacCoder->setBinCountingEnableFlag( false );

    numberOfWrittenBits = m_pcEntropyCoder->getNumberOfWrittenBits();

#ifdef CVI_HW_RC
    if (ctuTsAddr == startCtuTsAddr)
      numberOfWrittenBits -= HW_CABAC_FIRST_BIT;
    if (ctuTsAddr == boundingCtuTsAddr - 1)
      numberOfWrittenBits += HW_CABAC_SLICE_FIN_BIT;
#endif

#ifdef CVI_RC_USE_REAL_BIT
    numberOfWrittenBits -= ctu_bit_pos;
#endif

    // Calculate if this CTU puts us over slice bit size.
    // cannot terminate if current slice/slice-segment would be 0 Ctu in size,
    const UInt validEndOfSliceCtuTsAddr = ctuTsAddr + (ctuTsAddr == startCtuTsAddr ? 1 : 0);
    // Set slice end parameter
    if(pcSlice->getSliceMode()==FIXED_NUMBER_OF_BYTES && pcSlice->getSliceBits()+numberOfWrittenBits > (pcSlice->getSliceArgument()<<3))
    {
      pcSlice->setSliceSegmentCurEndCtuTsAddr(validEndOfSliceCtuTsAddr);
      pcSlice->setSliceCurEndCtuTsAddr(validEndOfSliceCtuTsAddr);
      boundingCtuTsAddr=validEndOfSliceCtuTsAddr;
    }
    else if((!bCompressEntireSlice) && pcSlice->getSliceSegmentMode()==FIXED_NUMBER_OF_BYTES && pcSlice->getSliceSegmentBits()+numberOfWrittenBits > (pcSlice->getSliceSegmentArgument()<<3))
    {
      pcSlice->setSliceSegmentCurEndCtuTsAddr(validEndOfSliceCtuTsAddr);
      boundingCtuTsAddr=validEndOfSliceCtuTsAddr;
    }

    if (boundingCtuTsAddr <= ctuTsAddr)
    {
      break;
    }

    pcSlice->setSliceBits( (UInt)(pcSlice->getSliceBits() + numberOfWrittenBits) );
    pcSlice->setSliceSegmentBits(pcSlice->getSliceSegmentBits()+numberOfWrittenBits);

    // Store probabilities of second CTU in line into buffer - used only if wavefront-parallel-processing is enabled.
    if ( ctuXPosInCtus == tileXPosInCtus+1 && m_pcCfg->getEntropyCodingSyncEnabledFlag())
    {
      m_entropyCodingSyncContextState.loadContexts(m_pppcRDSbacCoder[0][CI_CURR_BEST]);
    }


    if ( m_pcCfg->getUseRateCtrl() )
    {
      Int actualQP        = g_RCInvalidQPValue;
      Double actualLambda = m_pcRdCost->getLambda();
#ifdef CVI_RC_USE_REAL_BIT
      Int actualBits = numberOfWrittenBits;
#else
      Int actualBits = pCtu->getTotalBits();
#endif

      Int numberOfEffectivePixels    = 0;

#if JVET_M0600_RATE_CTRL
      Int numberOfSkipPixel = 0;
      for (Int idx = 0; idx < pcPic->getNumPartitionsInCtu(); idx++)
      {
        numberOfSkipPixel += 16 * pCtu->isSkipped(idx);
      }
#endif

      for ( Int idx = 0; idx < pcPic->getNumPartitionsInCtu(); idx++ )
      {
        if ( pCtu->getPredictionMode( idx ) != NUMBER_OF_PREDICTION_MODES && ( !pCtu->isSkipped( idx ) ) )
        {
          numberOfEffectivePixels = numberOfEffectivePixels + 16;
          break;
        }
      }

#if JVET_M0600_RATE_CTRL
      Double skipRatio = (Double)numberOfSkipPixel / m_pcRateCtrl->getRCPic()->getLCU(ctuTsAddr).m_numberOfPixel;
      m_pcRateCtrl->getRCPic()->m_numberOfSkipPixel += numberOfSkipPixel;
#endif

      if ( numberOfEffectivePixels == 0 )
      {
        actualQP = g_RCInvalidQPValue;
      }
      else
      {
        actualQP = pCtu->getQP( 0 );
      }
#if JVET_K0390_RATE_CTRL
      m_pcRateCtrl->getRCPic()->getLCU(ctuTsAddr).m_actualMSE = (Double)pCtu->getTotalDistortion() / (Double)m_pcRateCtrl->getRCPic()->getLCU(ctuTsAddr).m_numberOfPixel;
#endif
      m_pcRdCost->setLambda(oldLambda, oldSqrtLambda, pcSlice->getSPS()->getBitDepths());
#if JVET_M0600_RATE_CTRL
      m_pcRateCtrl->getRCPic()->updateAfterCTU(m_pcRateCtrl->getRCPic()->getLCUCoded(), actualBits, actualQP, actualLambda, skipRatio,
        pCtu->getSlice()->getSliceType() == I_SLICE ? 0 : m_pcCfg->getLCULevelRC());
#else
      m_pcRateCtrl->getRCPic()->updateAfterCTU( m_pcRateCtrl->getRCPic()->getLCUCoded(), actualBits, actualQP, actualLambda,
                                                pCtu->getSlice()->getSliceType() == I_SLICE ? 0 : m_pcCfg->getLCULevelRC() );
#endif
    }
#ifdef SIG_CCU
    if (g_sigdump.ccu)
    {
      if (!m_pcCfg->getUseRateCtrl())
      {
        sig_ccu_record_constqp(pCtu);
      }

      g_sigpool.p_ccu_rc_st->ctu_coded_bits = numberOfWrittenBits;
      sig_ccu_output_rc();
    }
#endif //~SIG_CCU
#ifdef SIG_PRU
    if (g_sigdump.pru)
    {
      sig_pru_output_madi_ctu(g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
    }
#endif //~SIG_PRU
#ifdef SIG_IAPU
    if (g_sigdump.iapu)
    {
        sig_iapu_output_ctu_bot_wb(pCtu);
    }
#endif
#ifdef CVI_RC_USE_REAL_BIT
    m_uiPicTotalBits += numberOfWrittenBits;
#else
    m_uiPicTotalBits += pCtu->getTotalBits();
#endif
    m_dPicRdCost     += pCtu->getTotalCost();
    m_uiPicDist      += pCtu->getTotalDistortion();
#ifdef SIGDUMP
    sigdump_ctb_end();
#endif
  }

  if(getRDOBitEstMode()==CTX_ADAPTIVE) {
    snapshotResiModel();
  }

  // store context state at the end of this slice-segment, in case the next slice is a dependent slice and continues using the CABAC contexts.
  if( pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag() )
  {
    m_lastSliceSegmentEndContextState.loadContexts( m_pppcRDSbacCoder[0][CI_CURR_BEST] );//ctx end of dep.slice
  }

  // stop use of temporary bit counter object.
  m_pppcRDSbacCoder[0][CI_CURR_BEST]->setBitstream(NULL);
  m_pcRDGoOnSbacCoder->setBitstream(NULL); // stop use of tempBitCounter.

#ifdef SIGDUMP
    sigdump_slicewise_disable();
#endif
  // TODO: optimise cabac_init during compress slice to improve multi-slice operation
  //if (pcSlice->getPPS()->getCabacInitPresentFlag() && !pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag())
  //{
  //  m_encCABACTableIdx = m_pcEntropyCoder->determineCabacInitIdx();
  //}
  //else
  //{
  //  m_encCABACTableIdx = pcSlice->getSliceType();
  //}
}

Void TEncSlice::encodeSlice   ( TComPic* pcPic, TComOutputBitstream* pcSubstreams, UInt &numBinsCoded )
{
  TComSlice *const pcSlice           = pcPic->getSlice(getSliceIdx());

  const UInt startCtuTsAddr          = pcSlice->getSliceSegmentCurStartCtuTsAddr();
  const UInt boundingCtuTsAddr       = pcSlice->getSliceSegmentCurEndCtuTsAddr();

  const UInt frameWidthInCtus        = pcPic->getPicSym()->getFrameWidthInCtus();
  const Bool depSliceSegmentsEnabled = pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag();
  const Bool wavefrontsEnabled       = pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag();

  // initialise entropy coder for the slice
  m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
  m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder );
  m_pcEntropyCoder->resetEntropy    ( pcSlice );

  numBinsCoded = 0;
  m_pcBinCABAC->setBinCountingEnableFlag( true );
  m_pcBinCABAC->setBinsCoded(0);

#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceEnable;
#endif
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tPOC: " );
  DTRACE_CABAC_V( pcPic->getPOC() );
  DTRACE_CABAC_T( "\n" );
#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceDisable;
#endif

  if (depSliceSegmentsEnabled)
  {
    // modify initial contexts with previous slice segment if this is a dependent slice.
    const UInt ctuRsAddr        = pcPic->getPicSym()->getCtuTsToRsAddrMap( startCtuTsAddr );
    const UInt currentTileIdx=pcPic->getPicSym()->getTileIdxMap(ctuRsAddr);
    const TComTile *pCurrentTile=pcPic->getPicSym()->getTComTile(currentTileIdx);
    const UInt firstCtuRsAddrOfTile = pCurrentTile->getFirstCtuRsAddr();

    if( pcSlice->getDependentSliceSegmentFlag() && ctuRsAddr != firstCtuRsAddrOfTile )
    {
      if( pCurrentTile->getTileWidthInCtus() >= 2 || !wavefrontsEnabled )
      {
        m_pcSbacCoder->loadContexts(&m_lastSliceSegmentEndContextState);
      }
    }
  }

  // for every CTU in the slice segment...
#ifdef SIG_CABAC
  if(g_sigdump.cabac || g_sigdump.cabac_prof) {
    g_sigpool.cabac_is_record = true;
  }
  if (g_sigdump.cabac)
  {
    if (startCtuTsAddr == 0) {
      sigdump_output_fprint(&g_sigpool.cabac_ctx, "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);
    }
    sigdump_output_fprint(&g_sigpool.cabac_ctx, "# [Slice info] Slice ID = %d\n", g_sigpool.slice_count - 1);
    sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, "# [Slice info] Slice ID = %d\n", g_sigpool.slice_count - 1);
    sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, "slice_qp_y = %d\n", pcSlice->getSliceQp());
    sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, "slice_type = %d\n", pcSlice->getSliceType());
    Bool encCabacInitFlag = (pcSlice->getSliceType()!=m_encCABACTableIdx && m_encCABACTableIdx!=I_SLICE) ? true : false;
    sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, "cabac_init_flag = %d\n", encCabacInitFlag);

    Int iniType = 0;
    if (pcSlice->getSliceType() == I_SLICE) {
      iniType = 0;
    } else if (pcSlice->getSliceType() == P_SLICE) {
      iniType = encCabacInitFlag ? 2 : 1;
    } else {
      iniType = encCabacInitFlag ? 1 : 2;
    }
    sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, "iniType = %d\n", iniType);

  }
#endif
  for( UInt ctuTsAddr = startCtuTsAddr; ctuTsAddr < boundingCtuTsAddr; ++ctuTsAddr )
  {
    const UInt ctuRsAddr = pcPic->getPicSym()->getCtuTsToRsAddrMap(ctuTsAddr);
    const TComTile &currentTile = *(pcPic->getPicSym()->getTComTile(pcPic->getPicSym()->getTileIdxMap(ctuRsAddr)));
    const UInt firstCtuRsAddrOfTile = currentTile.getFirstCtuRsAddr();
    const UInt tileXPosInCtus       = firstCtuRsAddrOfTile % frameWidthInCtus;
    const UInt tileYPosInCtus       = firstCtuRsAddrOfTile / frameWidthInCtus;
    const UInt ctuXPosInCtus        = ctuRsAddr % frameWidthInCtus;
    const UInt ctuYPosInCtus        = ctuRsAddr / frameWidthInCtus;
    const UInt uiSubStrm=pcPic->getSubstreamForCtuAddr(ctuRsAddr, true, pcSlice);
    TComDataCU* pCtu = pcPic->getCtu( ctuRsAddr );
#ifdef SIGDUMP
    sigdump_ctb_enc_start(ctuXPosInCtus * pcSlice->getSPS()->getMaxCUWidth(), ctuYPosInCtus * pcSlice->getSPS()->getMaxCUHeight());
#endif
    m_pcEntropyCoder->setBitstream( &pcSubstreams[uiSubStrm] );

    // set up CABAC contexts' state for this CTU
    if (ctuRsAddr == firstCtuRsAddrOfTile)
    {
      if (ctuTsAddr != startCtuTsAddr) // if it is the first CTU, then the entropy coder has already been reset
      {
        m_pcEntropyCoder->resetEntropy(pcSlice);
      }
    }
    else if (ctuXPosInCtus == tileXPosInCtus && wavefrontsEnabled)
    {
      // Synchronize cabac probabilities with upper-right CTU if it's available and at the start of a line.
      if (ctuTsAddr != startCtuTsAddr) // if it is the first CTU, then the entropy coder has already been reset
      {
        m_pcEntropyCoder->resetEntropy(pcSlice);
      }
      TComDataCU *pCtuUp = pCtu->getCtuAbove();
      if ( pCtuUp && ((ctuRsAddr%frameWidthInCtus+1) < frameWidthInCtus)  )
      {
        TComDataCU *pCtuTR = pcPic->getCtu( ctuRsAddr - frameWidthInCtus + 1 );
        if ( pCtu->CUIsFromSameSliceAndTile(pCtuTR) )
        {
          // Top-right is available, so use it.
          m_pcSbacCoder->loadContexts( &m_entropyCodingSyncContextState );
        }
      }
    }


    if ( pcSlice->getSPS()->getUseSAO() )
    {
      Bool bIsSAOSliceEnabled = false;
      Bool sliceEnabled[MAX_NUM_COMPONENT];
      for(Int comp=0; comp < MAX_NUM_COMPONENT; comp++)
      {
        ComponentID compId=ComponentID(comp);
        sliceEnabled[compId] = pcSlice->getSaoEnabledFlag(toChannelType(compId)) && (comp < pcPic->getNumberValidComponents());
        if (sliceEnabled[compId])
        {
          bIsSAOSliceEnabled=true;
        }
      }
      if (bIsSAOSliceEnabled)
      {
        SAOBlkParam& saoblkParam = (pcPic->getPicSym()->getSAOBlkParam())[ctuRsAddr];

        Bool leftMergeAvail = false;
        Bool aboveMergeAvail= false;
        //merge left condition
        Int rx = (ctuRsAddr % frameWidthInCtus);
        if(rx > 0)
        {
          leftMergeAvail = pcPic->getSAOMergeAvailability(ctuRsAddr, ctuRsAddr-1);
        }

        //merge up condition
        Int ry = (ctuRsAddr / frameWidthInCtus);
        if(ry > 0)
        {
          aboveMergeAvail = pcPic->getSAOMergeAvailability(ctuRsAddr, ctuRsAddr-frameWidthInCtus);
        }

        m_pcEntropyCoder->encodeSAOBlkParam(saoblkParam, pcPic->getPicSym()->getSPS().getBitDepths(), sliceEnabled, leftMergeAvail, aboveMergeAvail);
      }
    }

#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceEnable;
#endif
#ifdef SIG_RESI
      g_sigpool.resi_is_record_pos = true;
#endif //~SIG_RESI
      m_pcCuEncoder->encodeCtu( pCtu );
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceDisable;
#endif
#ifdef SIG_RESI
    g_sigpool.resi_is_record_pos = false;
#endif //~SIG_RESI

    //Store probabilities of second CTU in line into buffer
    if ( ctuXPosInCtus == tileXPosInCtus+1 && wavefrontsEnabled)
    {
      m_entropyCodingSyncContextState.loadContexts( m_pcSbacCoder );
    }

    // terminate the sub-stream, if required (end of slice-segment, end of tile, end of wavefront-CTU-row):
    if (ctuTsAddr+1 == boundingCtuTsAddr ||
         (  ctuXPosInCtus + 1 == tileXPosInCtus + currentTile.getTileWidthInCtus() &&
          ( ctuYPosInCtus + 1 == tileYPosInCtus + currentTile.getTileHeightInCtus() || wavefrontsEnabled)
         )
       )
    {
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        std::strncpy( g_sigpool.syntax_name, "end_of_slice_segment_flag", sizeof(g_sigpool.syntax_name) );
        sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0,14,0,1, 0);
      }
      cabac_prof_add_bins(NON_RESI, BYPASS, 1);
#endif
      m_pcEntropyCoder->encodeTerminatingBit(1);
      m_pcEntropyCoder->encodeSliceFinish();
#ifdef SIG_CABAC
      if(g_sigdump.cabac && g_sigpool.cabac_is_record) {
        sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "bits_accu = %x\n", pcSubstreams[uiSubStrm].getNumberOfWrittenBits() + 1);
        g_sigpool.hdr_bits_num += (HW_CABAC_SLICE_FIN_BIT - HW_CABAC_FIRST_BIT);
      }
#endif
      // Byte-alignment in slice_data() when new tile
      pcSubstreams[uiSubStrm].writeByteAlignment();

      // write sub-stream size
      if (ctuTsAddr+1 != boundingCtuTsAddr)
      {
        pcSlice->addSubstreamSize( (pcSubstreams[uiSubStrm].getNumberOfWrittenBits() >> 3) + pcSubstreams[uiSubStrm].countStartCodeEmulations() );
      }
    }
#ifdef SIGDUMP
    sigdump_ctb_enc_end();
#endif
  } // CTU-loop

#ifdef SIG_CABAC
  if(g_sigdump.cabac || g_sigdump.cabac_prof)
  {
    g_sigpool.cabac_is_record = false;
  }
#endif
  if( depSliceSegmentsEnabled )
  {
    m_lastSliceSegmentEndContextState.loadContexts( m_pcSbacCoder );//ctx end of dep.slice
  }

#if ADAPTIVE_QP_SELECTION
  if( m_pcCfg->getUseAdaptQpSelect() )
  {
    m_pcTrQuant->storeSliceQpNext(pcSlice); // TODO: this will only be storing the adaptive QP state of the very last slice-segment that is not dependent in the frame... Perhaps this should be moved to the compress slice loop.
  }
#endif

  if (pcSlice->getPPS()->getCabacInitPresentFlag() && !pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag())
  {
    m_encCABACTableIdx = m_pcEntropyCoder->determineCabacInitIdx(pcSlice);
  }
  else
  {
    m_encCABACTableIdx = pcSlice->getSliceType();
  }
  if(g_algo_cfg.EnableCalMapData)
  {
    printf("skip, nonskip bits %d %d\n", gp_isp_enc_param->skip_bits, gp_isp_enc_param->nonskip_bits);
  }

  numBinsCoded = m_pcBinCABAC->getBinsCoded();
}

Void TEncSlice::calculateBoundingCtuTsAddrForSlice(UInt &startCtuTSAddrSlice, UInt &boundingCtuTSAddrSlice, Bool &haveReachedTileBoundary,
                                                   TComPic* pcPic, const Int sliceMode, const Int sliceArgument)
{
  TComSlice* pcSlice = pcPic->getSlice(getSliceIdx());
  const UInt numberOfCtusInFrame = pcPic->getNumberOfCtusInFrame();
  const TComPPS &pps=*(pcSlice->getPPS());
  boundingCtuTSAddrSlice=0;
  haveReachedTileBoundary=false;

  switch (sliceMode)
  {
    case FIXED_NUMBER_OF_CTU:
      {
        UInt ctuAddrIncrement    = sliceArgument;
        boundingCtuTSAddrSlice  = ((startCtuTSAddrSlice + ctuAddrIncrement) < numberOfCtusInFrame) ? (startCtuTSAddrSlice + ctuAddrIncrement) : numberOfCtusInFrame;
      }
      break;
    case FIXED_NUMBER_OF_BYTES:
      boundingCtuTSAddrSlice  = numberOfCtusInFrame; // This will be adjusted later if required.
      break;
    case FIXED_NUMBER_OF_TILES:
      {
        const UInt tileIdx        = pcPic->getPicSym()->getTileIdxMap( pcPic->getPicSym()->getCtuTsToRsAddrMap(startCtuTSAddrSlice) );
        const UInt tileTotalCount = (pcPic->getPicSym()->getNumTileColumnsMinus1()+1) * (pcPic->getPicSym()->getNumTileRowsMinus1()+1);
        UInt ctuAddrIncrement   = 0;

        for(UInt tileIdxIncrement = 0; tileIdxIncrement < sliceArgument; tileIdxIncrement++)
        {
          if((tileIdx + tileIdxIncrement) < tileTotalCount)
          {
            UInt tileWidthInCtus   = pcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileWidthInCtus();
            UInt tileHeightInCtus  = pcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileHeightInCtus();
            ctuAddrIncrement    += (tileWidthInCtus * tileHeightInCtus);
          }
        }

        boundingCtuTSAddrSlice  = ((startCtuTSAddrSlice + ctuAddrIncrement) < numberOfCtusInFrame) ? (startCtuTSAddrSlice + ctuAddrIncrement) : numberOfCtusInFrame;
      }
      break;
    default:
      boundingCtuTSAddrSlice    = numberOfCtusInFrame;
      break;
  }

  // Adjust for tiles and wavefronts.
  const Bool wavefrontsAreEnabled = pps.getEntropyCodingSyncEnabledFlag();

  if ((sliceMode == FIXED_NUMBER_OF_CTU || sliceMode == FIXED_NUMBER_OF_BYTES) &&
      (pps.getNumTileRowsMinus1() > 0 || pps.getNumTileColumnsMinus1() > 0))
  {
    const UInt ctuRSAddr                  = pcPic->getPicSym()->getCtuTsToRsAddrMap(startCtuTSAddrSlice);
    const UInt startTileIdx               = pcPic->getPicSym()->getTileIdxMap(ctuRSAddr);

    const TComTile *pStartingTile         = pcPic->getPicSym()->getTComTile(startTileIdx);
    const UInt tileStartTsAddr            = pcPic->getPicSym()->getCtuRsToTsAddrMap(pStartingTile->getFirstCtuRsAddr());
    const UInt tileStartWidth             = pStartingTile->getTileWidthInCtus();
    const UInt tileStartHeight            = pStartingTile->getTileHeightInCtus();
    const UInt tileLastTsAddr_excl        = tileStartTsAddr + tileStartWidth*tileStartHeight;
    const UInt tileBoundingCtuTsAddrSlice = tileLastTsAddr_excl;

    const UInt ctuColumnOfStartingTile    = ((startCtuTSAddrSlice-tileStartTsAddr)%tileStartWidth);
    if (wavefrontsAreEnabled && ctuColumnOfStartingTile!=0)
    {
      // WPP: if a slice does not start at the beginning of a CTB row, it must end within the same CTB row
      const UInt numberOfCTUsToEndOfRow            = tileStartWidth - ctuColumnOfStartingTile;
      const UInt wavefrontTileBoundingCtuAddrSlice = startCtuTSAddrSlice + numberOfCTUsToEndOfRow;
      if (wavefrontTileBoundingCtuAddrSlice < boundingCtuTSAddrSlice)
      {
        boundingCtuTSAddrSlice = wavefrontTileBoundingCtuAddrSlice;
      }
    }

    if (tileBoundingCtuTsAddrSlice < boundingCtuTSAddrSlice)
    {
      boundingCtuTSAddrSlice = tileBoundingCtuTsAddrSlice;
      haveReachedTileBoundary = true;
    }
  }
  else if ((sliceMode == FIXED_NUMBER_OF_CTU || sliceMode == FIXED_NUMBER_OF_BYTES) && wavefrontsAreEnabled && ((startCtuTSAddrSlice % pcPic->getFrameWidthInCtus()) != 0))
  {
    // Adjust for wavefronts (no tiles).
    // WPP: if a slice does not start at the beginning of a CTB row, it must end within the same CTB row
    boundingCtuTSAddrSlice = min(boundingCtuTSAddrSlice, startCtuTSAddrSlice - (startCtuTSAddrSlice % pcPic->getFrameWidthInCtus()) + (pcPic->getFrameWidthInCtus()));
  }
}

/** Determines the starting and bounding CTU address of current slice / dependent slice
 * \param [out] startCtuTsAddr
 * \param [out] boundingCtuTsAddr
 * \param [in]  pcPic

 * Updates startCtuTsAddr, boundingCtuTsAddr with appropriate CTU address
 */
Void TEncSlice::xDetermineStartAndBoundingCtuTsAddr  ( UInt& startCtuTsAddr, UInt& boundingCtuTsAddr, TComPic* pcPic )
{
  TComSlice* pcSlice                 = pcPic->getSlice(getSliceIdx());

  // Non-dependent slice
  UInt startCtuTsAddrSlice           = pcSlice->getSliceCurStartCtuTsAddr();
  Bool haveReachedTileBoundarySlice  = false;
  UInt boundingCtuTsAddrSlice;
  calculateBoundingCtuTsAddrForSlice(startCtuTsAddrSlice, boundingCtuTsAddrSlice, haveReachedTileBoundarySlice, pcPic,
                                     m_pcCfg->getSliceMode(), m_pcCfg->getSliceArgument());
  pcSlice->setSliceCurEndCtuTsAddr(   boundingCtuTsAddrSlice );
  pcSlice->setSliceCurStartCtuTsAddr( startCtuTsAddrSlice    );

  // Dependent slice
  UInt startCtuTsAddrSliceSegment          = pcSlice->getSliceSegmentCurStartCtuTsAddr();
  Bool haveReachedTileBoundarySliceSegment = false;
  UInt boundingCtuTsAddrSliceSegment;
  calculateBoundingCtuTsAddrForSlice(startCtuTsAddrSliceSegment, boundingCtuTsAddrSliceSegment, haveReachedTileBoundarySliceSegment, pcPic,
                                     m_pcCfg->getSliceSegmentMode(), m_pcCfg->getSliceSegmentArgument());
  if (boundingCtuTsAddrSliceSegment>boundingCtuTsAddrSlice)
  {
    boundingCtuTsAddrSliceSegment = boundingCtuTsAddrSlice;
  }
  pcSlice->setSliceSegmentCurEndCtuTsAddr( boundingCtuTsAddrSliceSegment );
  pcSlice->setSliceSegmentCurStartCtuTsAddr(startCtuTsAddrSliceSegment);

  // Make a joint decision based on reconstruction and dependent slice bounds
  startCtuTsAddr    = max(startCtuTsAddrSlice   , startCtuTsAddrSliceSegment   );
  boundingCtuTsAddr = boundingCtuTsAddrSliceSegment;
}

Double TEncSlice::xGetQPValueAccordingToLambda ( Double lambda )
{
  return 4.2005*log(lambda) + 13.7122;
}

Void TEncSlice::calFrameMadi(TComPic* pcPic, bool isFirstFrame)
{
  Pel* pSrcBase = pcPic->getPicYuvOrg()->getAddr(COMPONENT_Y);
  Int  iStride = pcPic->getPicYuvOrg()->getStride(COMPONENT_Y);
  Int  iWidth = pcPic->getPicYuvOrg()->getWidth(COMPONENT_Y);
  Int  iHeight = pcPic->getPicYuvOrg()->getHeight(COMPONENT_Y);
#ifdef CVI_FAST_CU_ENC
  memcpy(g_madi8_hist_delay, g_madi8_hist, (1<<MADI_BIN_PREC)*sizeof(g_madi8_hist[0]));
  memset(g_madi8_hist, 0, (1<<MADI_BIN_PREC)*sizeof(g_madi8_hist[0]));

  memcpy(g_madi16_hist_delay, g_madi16_hist, sizeof(g_madi16_hist));
  memset(g_madi16_hist, 0, sizeof(g_madi16_hist));

  memcpy(g_madi32_hist_delay, g_madi32_hist, sizeof(g_madi32_hist));
  memset(g_madi32_hist, 0, sizeof(g_madi32_hist));
#endif
  g_pic_tc_accum = 0;
  g_pic_foreground_cnt = 0;

  int iWidth_align32 = cvi_mem_align(iWidth, 32);
  int iHeight_align32 = cvi_mem_align(iHeight, 32);

#ifdef CVI_ALGO_CFG
  if (iWidth_align32 != iWidth || iHeight_align32 != iHeight)
  {
    if (m_pSrcPad)
    {
      Pel *p_pad = m_pSrcPad;
      for (int i = 0; i < iHeight; i++)
      {
        memcpy(p_pad, &pSrcBase[i * iStride], sizeof(Pel) * iWidth);

        // Pad right boundary
        Pel bonud_x = p_pad[iWidth - 1];
        for (int x = iWidth; x < iWidth_align32; x++)
          p_pad[x] = bonud_x;

        p_pad += iWidth_align32;
      }

      // Pad bottom boundary
      Pel *p_bound_y = p_pad - iWidth_align32;
      for (int j = iHeight; j < iHeight_align32; j++)
      {
        memcpy(p_pad, p_bound_y, sizeof(Pel) * iWidth_align32);
        p_pad += iWidth_align32;
      }

      pSrcBase = m_pSrcPad;
      iStride = iWidth_align32;
    }
  }
#endif

  // 16x16 grid traversal
  for (Int gridPosY=0; gridPosY<iHeight_align32; gridPosY+=16)
  {
    for (Int gridPosX=0; gridPosX<iWidth_align32; gridPosX+=16)
    {
      Int Madi = 0, Lum = 0;
      for (Int blkY=0; blkY<2; blkY++)
      {
        for (Int blkX=0; blkX<2; blkX++)
        {
          Int posY = gridPosY + (blkY<<3);
          Int posX = gridPosX + (blkX<<3);
          Pel *src = pSrcBase + iStride*posY + posX;
          Int blkMadi, blkLum;
          cal8x8MadiAndLum(src, iStride, &blkMadi, &blkLum);
          Madi += blkMadi;
          Lum += blkLum;

#ifdef CVI_FAST_CU_ENC
          g_blk8_madi.set_data(posX, posY, blkMadi);
          g_blk8_lum.set_data(posX, posY, blkLum);
          if (posX < iWidth && posY < iHeight)
            g_madi8_hist[blkMadi>>(8-MADI_BIN_PREC)] ++;
#endif
        }
      }
      Madi >>= 2; Lum >>= 2;

      g_blk_madi.set_data(gridPosX, gridPosY, Madi);
      g_blk_lum.set_data(gridPosX, gridPosY, Lum);
#ifdef CVI_FAST_CU_ENC
      if (Madi < 128)
        g_madi16_hist[Madi >> (7 - MADI_HIST_PREC)]++;
#endif
      g_pic_tc_accum += Madi;
    }
  }

#ifdef CVI_FAST_CU_ENC_BY_COST
  // Madi 32 hist for fast encode of first P
  if (pcPic->getSlice(0)->getSliceType() == I_SLICE)
  {
    for (Int gridPosY=0; gridPosY<iHeight_align32; gridPosY+=32)
    {
      for (Int gridPosX=0; gridPosX<iWidth_align32; gridPosX+=32)
      {
        Int Madi = 0;
        for (Int blkY=0; blkY<4; blkY++)
        {
          for (Int blkX=0; blkX<4; blkX++)
          {
            Int posY = gridPosY + (blkY<<3);
            Int posX = gridPosX + (blkX<<3);
            Madi += g_blk_madi.get_data(posX, posY);
          }
        }
        Madi >>= 4;

        if (Madi < 128)
          g_madi32_hist[Madi >> (7 - MADI_HIST_PREC)]++;
      }
    }
  }
#endif //~CVI_FAST_CU_ENC_BY_COST

  // -- dump vidusualized grid madi/luminance frame --
  if(!g_sigdump.perc_show) {
    return;
  }
  static unsigned char *madi_line, *mot_line, *fg_line;
#ifdef CVI_FOREGROUND_QP
  static unsigned char *dqp_line;
#endif

  if(g_sigpool.enc_count==0) {
    madi_line = new unsigned char[iWidth];
    mot_line = new unsigned char[iWidth];
    fg_line = new unsigned char[iWidth];
#ifdef CVI_FOREGROUND_QP
    dqp_line = new unsigned char[iWidth];
#endif
  }
  for (Int gridPosY=0; gridPosY<iHeight; gridPosY+=16)
  {
    for (Int gridPosX=0; gridPosX<iWidth; gridPosX+=16)
    {
      Int Madi = Clip3(0, 255, g_blk_madi.get_data(gridPosX, gridPosY)<<1); // mul 2 for better visualization
      UChar motion = Clip3(0, 255, g_motion_map[0].get_data(gridPosX, gridPosY)<<2);
      UChar foreground = Clip3(0, 255, (int)((g_foreground_map[0].get_data(gridPosX, gridPosY) * 255)));
      memset(&madi_line[gridPosX], Madi, 16);
      memset(&mot_line[gridPosX], motion, 16);
      memset(&fg_line[gridPosX], foreground, 16);

#ifdef CVI_FOREGROUND_QP
      UChar dqp = Clip3(0, 255, (int)((g_fg_dqp_map.get_data(gridPosX, gridPosY) * 40)));
      memset(&dqp_line[gridPosX], dqp, 16);
#endif
    }
    Int lineNum = min(16, iHeight - gridPosY);
    for(Int line=0; line<lineNum; line++)
    {
      sigdump_output_bin(&g_sigpool.perc_show[0], madi_line, iWidth);
      if(g_sigpool.enc_count>0) {
        sigdump_output_bin(&g_sigpool.perc_show[2], mot_line, iWidth);
        sigdump_output_bin(&g_sigpool.perc_show[3], fg_line, iWidth);
#ifdef CVI_FOREGROUND_QP
        sigdump_output_bin(&g_sigpool.perc_show[4], dqp_line, iWidth);
#endif
      }
    }
  }
  if(g_sigpool.enc_count>0) {
    for (Int pos_y=0; pos_y<iHeight; pos_y++) {
      for (Int pos_x=0; pos_x<iWidth; pos_x++) {
        UChar g_alpha = Clip3((unsigned char)0,  (unsigned char)255, g_alpha_map.get_data(pos_x, pos_y));
        sigdump_output_bin(&g_sigpool.perc_show[1], (unsigned char*)&g_alpha, 1);
      }
    }
  }
  // chroma
  memset(madi_line, 128, iWidth);
  for(Int idx=0; idx<5; idx++)
  {
    if(g_sigpool.enc_count==0 && idx>0) {
      break;
    }
    for(Int line=0; line<(iHeight>>1); line++) {
      sigdump_output_bin(&g_sigpool.perc_show[idx], madi_line, iWidth);
    }
  }
}
//! \}
