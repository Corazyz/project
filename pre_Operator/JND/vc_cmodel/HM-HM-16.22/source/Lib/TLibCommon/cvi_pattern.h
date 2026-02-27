#ifndef __CVI_SIG_PAT__
#define __CVI_SIG_PAT__

#include "cvi_sigdump.h"
#include "CommonDef.h"
#include "TComPic.h"
#include "cvi_cu_ctrl.h"
#include "cvi_intra.h"

#ifdef SIG_PRU
typedef struct _EDGE_DET_ST
{
    int x;
    int y;
    int index;
    int strength;
} EDGE_DET_ST;
#endif //~SIG_PRU

#ifdef SIG_RRU
void sig_rru_copy_intp_temp_buf(ComponentID compID, short *p_src, int src_stride, short *p_dst, int dst_stride, int size);
void sig_rru_output_intp(bool is_intra, int luma_pu_size, bool is_chroma);
void sig_rru_output_dct(bool is_intra, int luma_pu_size, bool is_chroma);

void sig_rru_copy_dct_temp_buf(ComponentID compID, Pel *p_residual, int res_stride,
                               TCoeff *p_coeff, int size, int tu_x, int tu_y);
void sig_rru_copy_idct_temp_buf(ComponentID compID, Pel *p_residual, int res_stride,
                                TCoeff *p_coeff, int size, int tu_x, int tu_y);
void sig_rru_update_final_buf(bool is_chroma, bool is_intra, int size);

int rru_get_last_pos_scale(double input);
void sig_rru_output_cu_self_info(sig_ctx *p_ctx, int cu_size);
void sig_rru_output_cu_winner(int cu_width);
void sig_rru_output_cu_cmd(TComDataCU *p_cu, int cu_width);
void sig_rru_output_cu_golden(int cu_width, SliceType slicetype, TComYuv *p_rec_yuv);
void sig_rru_output_i4_rec(ComponentID comp, const Pel *p_rec_buf, int stride);
void sig_rru_output_i4_resi(bool is_chroma);
void sig_rru_output_i4term_temp_data();
void sig_rru_copy_cu_cost(ForceDecision force_decison, TComDataCU *p_best_cu, TComDataCU *p_temp_cu);
void sig_rru_output_cu_order(sig_ctx *p_ctx, TComDataCU *pcCU, int cu_x, int cu_y, int cu_w, int cu_h,
                             UInt uiAbsPartIdx, UInt uiDepth);
void sig_rru_output_intra(int puSize, const TComRectangle &rect, TComDataCU* pcCU);
#endif //~SIG_RRU
#ifdef SIG_RESI
void sig_copy_resi_buf(ComponentID compID, TCoeff *p_coeff_buf, int pos_x, int pos_y, int tu_size);
void sig_output_resi_buf();
#endif //~SIG_RESI
#ifdef SIG_RRU_DEBUG
void sig_rru_copy_iq_in_temp_buf(ComponentID compID, TCoeff *p_coeff, int size, int tu_x, int tu_y);
#endif //~SIG_RRU_DEBUG
#ifdef SIG_PRU
void sig_pru_output_cmd(int i4_thr, int i4_tar, bool is_i_slice);
void sig_pru_output_madi_ctu(int blk64_x, int blk64_y);
void sig_pru_output_hist();
void sig_pru_set_edge_info(int x, int y, int blk_size, int es_index, int es_strength);
void sig_pru_set_edge_info_i4_termination(int x, int y);
#endif //~SIG_PRU
#ifdef SIG_CABAC
void sig_output_tu_idx(const sig_ctx *ctx, TComTU &rTu, const ComponentID compID);
void sig_cabac_bits_statis(TComTU &rTu, const ComponentID compID, UInt NumberOfWrittenBits, Bool bHaveACodedBlock);
void sig_cabac_dummy_tu(TComDataCU* pcCU, UInt NumberOfWrittenBits, const UInt uiAbsPartIdx, UInt uiDepth);
void sig_cabac_syntax_init_tab();
void sig_cabac_para_update(UInt tuSig0Bin, UInt tuSig1Bin, Int org_b0Scale, Int org_b1Scale, Int org_b0Bias, Int org_b1Bias, UInt tuSig0FracBit, UInt tuSig1FracBit);
#endif //~SIG_CABAC
#ifdef SIG_CCU
void sig_ccu_resi(TComTU &rTu);
void sig_ccu_dummy_resi(TComDataCU* pcCU, const UInt uiAbsPartIdx, UInt uiDepth);
void cvi_fill_scan_idx(bool is4x4, const TComDataCU *rpcTempCU);
void sigdump_ccu_md_4x4(TComDataCU*& rpcBestCU);
void sigdump_ccu_md(TComDataCU*& rpcBestCU, Bool isSkipWin);
void sigdump_ccu_qp(TComDataCU *pCu, UInt uiWidth, Int64 Lambda, Int64 SqrtLambda);
void sigdump_irpu_cu_golden_start(string type);
void sigdump_ccu_cu_golden_start(string type);
void sigdump_ccu_ime_mvp(int cu_idx_x, int cu_idx_y);
void sigdump_ccu_rd_cost(TComDataCU* bestCU, Int cu_width, UInt uiDepth,
                         Double totalRDcost_inter, Double totalRDcost_inter_wt);
void sig_ccu_madi(sig_ctx *ccu_ctx, int cu_x, int cu_y);
void sig_ccu_output_cu_golden_4x4();
void sig_ccu_record_rc_blk16(int x, int y, int tc_qp_delta, int lum_qp_delta, int blk_qp);
void sig_ccu_record_cu32_bits(int cu_x, int cu_y, int cu_w, int cu_h, int total_bits);
void sig_ccu_record_constqp(TComDataCU *p_cu);
void sig_ccu_output_rc();
void sig_ccu_output_init_tab();
void sig_ccu_output_qpmap();
void sig_ccu_copy_cbf_ctx();
void sig_ccu_output_i4term_temp_data();
#endif //~SIG_CCU
#ifdef SIG_IME_MVP
void sigdump_nei_mvb(TComDataCU* pCtu);
#endif //~SIG_IME_MVP
#ifdef SIG_IRPU
void sigdump_col_mv(TComDataCU* pCtu);
void sigdump_irpu_cu_cmd(TComDataCU*& rpcTempCU, TComMvField* pcMvFieldNeighbours);
void sigdump_me_cu_golden_backup(TComDataCU*& rpcTempCU, int list_idx);
#endif //~SIG_IRPU

#ifdef SIG_IAPU
#include "cvi_iapu_ref_smp_mux.h"
extern iapu_ref_mux_generator  g_i8_p8_mux_gen;
extern iapu_ref_mux_generator  g_i4_p8_mux_gen;
extern iapu_ref_mux_generator  g_i16_p8_mux_gen;
void sig_iapu_ref_mux_cnt();
//
void sig_iapu_output_ctu_idx(int sz);
void sig_iapu_output_cu_idx(int sz);
void sig_iapu_output_neb(sig_ctx *p_ctx, int sz, ComponentID comp);
void sig_iapu_output_i4_rec(ComponentID comp, const Pel *p_rec_buf, int stride);//copy from sig_rru_output_i4_rec
void sig_iapu_output_rec(ComponentID comp, const Pel *p_rec_buf, int tu_size);
void sig_iapu_output_cu_winner(TComDataCU *p_best_cu, int cu_x, int cu_y, int cu_width);
void sig_iapu_output_cu_cmd(TComDataCU *p_cu, int cu_width, int abs_idx);
void sig_iapu_output_cu_neb_wb(sig_ctx *p_ctx, const Pel *p_rec, int size, int is_b, ComponentID id);   // copy from sig_rru_output_cu_iap
void sig_iapu_output_ctu_bot_wb(TComDataCU* pCtu);
void sig_iapu_output_cu_self_info(sig_ctx *p_ctx);  //copy from sig_rru_output_cu_self_info
void sig_iapu_save_blk4_self_info(int blk4_y_idx, int blk4_c_idx);
void sig_iapu_update_final_buf(bool is_chroma, bool is_intra, int size); //copy from sig_rru_update_final_buf
void sig_iapu_output_i4_resi(bool is_chroma);   //copy from sig_rru_output_i4_resi
void sig_iapu_output_i4_pred();
void sig_iap_output_i4term_temp_data();
void sig_iapu_output_iap_syntax(TComDataCU *p_cu, PartSize part_size, TComRdCost *p_rd);
#endif //~SIG_IAPU

#ifdef SIG_TOP
void sig_top_reg_write(sig_ctx *p_ctx, unsigned int base, unsigned int addr, unsigned int value);
void sig_top_reg_cmdq_write(sig_ctx *p_ctx, unsigned int base, unsigned int addr, unsigned int value,int end);
void sig_top_reg_read(sig_ctx *p_ctx, unsigned int base, unsigned int addr, unsigned int value, string comment);
void sig_top_output_qpmap();
int sig_overwrite_bs_buffer(int mode, int slice_idx);
int sig_ccu_overwrite_bs_buffer(int mode, int slice_idx);
#endif // SIG_TOP

#ifdef SIG_PPU
void sig_ppu_output_reg(TComSlice *p_slice);
#endif //~SIG_PPU

#ifdef SIG_VBC
void sig_vbc_frm_setting(int cfg_lossy,int cfg_lossy_cr,int cfg_lossy_tolerence,int cfg_lossy_delay);
#endif //~SIG_VBC

#ifdef SIG_SMT
void sig_smt_isp_src(char *p_src, int width, int height);
void sig_smt_isp_set();
void sig_smt_isp_save_blk_data(int x, int y, int skip, int still, int trans, int motion, int dqp);
void sig_smt_isp_save_blk_skip(int x, int y, int skip);
void sig_smt_isp_gld(int width, int height);
void sig_smt_ai_src(AI_ENC_PARAM *p_ai_param);
void sig_smt_ai_src_gld(AI_ENC_PARAM *p_ai_param);
void sig_smt_ai_set(AI_ENC_PARAM *p_ai_param);
void sig_smt_rand_set(AI_ENC_PARAM *p_ai_param);
void sig_smt_isp_ai_set(int width, int height, bool is_ai_en, bool is_isp_en);
void sig_smt_isp_ai_gld(int width, int height);
#endif //~SIG_SMT
#endif
