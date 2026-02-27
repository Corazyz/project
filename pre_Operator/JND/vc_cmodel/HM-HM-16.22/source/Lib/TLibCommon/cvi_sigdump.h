
#ifndef __CVI_SIGDUMP__
#define __CVI_SIGDUMP__

#include <iostream>
#include <fstream>
#include "CommonDef.h"

using namespace std;

#define SIGDUMP

#ifdef SIGDUMP

  #define SIG_TOP

  #define SIG_IRPU

  #define SIG_FME

  #define SIG_IME

  #define SIG_CCU

  #define SIG_MC

  #define SIG_BIT_EST

  #define SIG_IME_MVP

  #define SIG_CABAC

  #define SIG_RESI

  #define SIG_RRU
  #ifdef SIG_RRU
      //#define SIG_RRU_TXT   // for transform data debug.
      //#define SIG_RRU_DEBUG
  #endif
  #define SIG_PRU
  #define SIG_IAPU
    //#define SIG_IAPU_DEBUG
  #define SIG_PPU
    //#define SIG_PPU_OLD
  #define SIG_FPGA

  #define SIG_VBC
  #define SIG_SMT
#endif

typedef struct _sig_bit_est_st {
  unsigned int tu_x;
  unsigned int tu_y;
  unsigned int tu_width;
  unsigned int tu_height;
  unsigned int uiCbf;
  unsigned int lps;
  unsigned int sival[2];

  unsigned int csbf_map;
  unsigned int scan_idx;
  unsigned int is_intra;
  unsigned int last_x;
  unsigned int last_y;
  unsigned int last_xy_suf_bc;
  unsigned int csbf_bc;
  unsigned int gt2_bc;
  unsigned int sign_bc;
  unsigned int rem_bc;
  unsigned int int_bites;
  unsigned int sig_bc[2];
  unsigned int m_sig[2];
  int c_sig[2];
  unsigned int sig_bc_linear[2];
  unsigned int last_xy_pre_bc;
  unsigned int ratio;
  unsigned int frac_bites;
  unsigned int total_bites;
  int Resi[32][32];
} sig_bit_est_st;

typedef struct _sig_me_golden_st {
  int size_idx;
  unsigned long long SSE[2];
  unsigned long long SATD[2];
  unsigned long long BIT_CNT;
  unsigned long long BIN_CNT;
  unsigned long long LC_BIT_CNT;
  unsigned long long LC_BIN_CNT;
  unsigned long long BEST_BIN_CNT[2]; // MERGE / AMVP
  int refIdx;
  int isLongTerm;
  int amvp_list_idx;
  int mvd_x;
  int mvd_y;
  int mv_x;
  int mv_y;
  int poc_diff;
  unsigned char mv_level;
  int mv_x_ori;
  int mv_y_ori;
} sig_me_golden_st;

typedef struct _sig_ime_mvp_st {
  int mv_x;
  int mv_y;
  int refIdx;
} sig_ime_mvp_st;

typedef struct _sig_intra_golden_st {
  int size_idx;
  int luma_pred_mode[4];
  int chroma_pred_mode;
  int intra_chroma_pred_mode;
  int rem_luma_pred_mode[4];
  int mpm_idx[4];
  int prev_luma_pred_flag[4];
} sig_intra_golden_st;

typedef struct _sig_est_golden_st {
  unsigned long long EST_SSE[3][4]; //y cb cr, tu partition
  unsigned int EST_BIT[4]; //tu partition
  unsigned int EST_BAC;
  int TCoeff[3][4][16*16]; //y cb cr, tu partition, dct coeff
  int IQCoeff[3][4][16*16]; //y cb cr, tu partition, iq coeff
} sig_est_golden_st;

typedef struct _sig_est_golden_intra_st {
  unsigned long long EST_SSE; //y cb cr
  int TCoeff[16*16]; //y cb cr, dct coeff
  int IQCoeff[16*16]; //y cb cr, iq coeff
} sig_est_golden_intra_st;

typedef struct _sig_cabac2ccu_st {
  unsigned int CbfCtx[2][2]; //[y, c][]
  unsigned int PartSizeCtx;
  int m_c[2][9][2]; //[m or c][][]
} sig_cabac2ccu_st;

typedef struct _sig_ctx {
  bool enable;
  FILE *fp;
  bool is_open;
} sig_ctx;

typedef struct _sig_amvp_list_ctx {
  char dir[128];
  short mvx;
  short mvy;
  short ref_idx;
  int poc_diff;
} sig_amvp_list_ctx;

#ifdef SIG_RRU
typedef struct _sig_rru_est_gold_st {
  unsigned int bitest;
  unsigned int csbf;
  unsigned int last_x;
  unsigned int last_y;
  unsigned int cbf;
  unsigned int bitest_sfi;        // cbf sfi
  unsigned int bitest_cbf_ctx[2]; // tu depth 0 1
  unsigned int bitest_pm_sfi;     // part_mode sfi
} sig_rru_est_gold_st;

typedef struct _sig_rru_gold_st {
  unsigned int sse4x4[4];           // chroma sse is in the first element.
  unsigned int sse[2];              // [is_intra]

  sig_rru_est_gold_st rru_est_gold[2][3][4]; // [is_intra][compID][TUs], TUs: CU32 has 4 TUs
  sig_rru_est_gold_st rru_est_gold_4x4[3][4]; // [compID][4x4blk]
} sig_rru_gold_st;
#endif //~SIG_RRU

#ifdef SIG_PRU
typedef struct _sig_pru_st {
  sig_ctx pru_cmd_ctx;
  sig_ctx pru_madi_ctx;
  sig_ctx pru_edge_ctx;
  sig_ctx pru_hist_ctx;
  int blk16_id;
  int disable_i4;
  int madi_blk32_id;
  int stat_early_term[16]; // blk16 in a CTU
  int last_stat_early_term;
} sig_pru_st;
#endif //~SIG_PRU

#ifdef SIG_CCU
typedef struct _sig_ccu_rc_st {
  int ctu_x;
  int ctu_y;
  int ctu_coded_bits;
  bool is_row_ovf;    // Delay
  int ctu_qp;
  int row_qp;
  int ctu_row_target_bits;
  int ctu_row_acc_bits;
  int blk_x[16];      // 16 blk16 in a CUT64
  int blk_y[16];
  int tc_q_ofs[16];
  int lum_q_ofs[16];
  char qpmap[16];
  char blk16_qp[16];
  int blk16_idx;
  unsigned int cu32_start_bits;
  unsigned int cu32_coded_bits[4];
  int cu32_qp[4];
  unsigned int cu32_cbf_ctx[4][2][2];
} sig_ccu_rc_st;
#endif //~SIG_CCU

#ifdef SIG_IAPU
typedef struct _sig_iapu_st {
  unsigned int  neb_b[3][32+1];
  unsigned int  neb_a[3][32+1];
  unsigned int  neb_c[3];
  unsigned int  neb_w[3], neb_h[3];
  unsigned int  rec_y[16*16];
  unsigned int  rec_c[16*16];
  unsigned int  lbf_y[64];
  unsigned int  lbf_c[64];
  Bool          neb_avail_a[2];
  Bool          neb_avail_b[3];
  Bool          neb_avail_a_iap[3][2];      //{cu8/cu16/cu32}*2
  Bool          neb_avail_b_iap[3][3];      //{cu8/cu16/cu32}*3
  Double        iapu_cu_cost[2];            //copy from RRU // cu cost of [current CU cost / split CUs total cost]
  Double        iapu_intra_cost[3];         //copy from RRU // intra cost of [i4/i8/i16]
  Double        iapu_intra_cost_i8x4;       //copy from RRU 
  Int           iapu_intra_dir[3];
  UInt          lambda;
  Int           qp[3];
  unsigned int  bitest_sfi[4][2];
  unsigned int  bitest_ratio[4][2];
  unsigned int  m[4][2][2];
  unsigned int  c[4][2][2];
  unsigned int  bitest_pm_sfi;

  UInt          c_mode_list[3][9];          // {i4/i8/i16}*max
  Int64         c_mode_cost[3][9];          // {i4/i8/i16}*max
  UInt64        c_mode_sad [3][9][2];       // {i4/i8/i16}*max*{cb/cr}
  Int64         c_mode_lambda[3];           // {i4/i8/i16}

  Int64         c_mode_distortion[5];       // chroma i4 of Luma i4x4
  UInt64        c_mode_bins[5];             // chroma i4 of Luma i4x4
  Int           c_mode_bits[5];             // chroma i4 of Luma i4x4
  UInt64        c_mode_bits_est_val[5];     // chroma i4 of Luma i4x4
  UInt          c_mode_best;                // chroma i4 of Luma i4x4
  Int64         c_mode_best_cost;           // chroma i4 of Luma i4x4

  int           *p_iapu_i4_coef_temp[3];     // copy from RRU // [y/cb/cr]
  int           *p_iapu_i4_coef_final[3];    // copy from RRU // [y/cb/cr]

  UChar         tu4_pred_blk[3][6][4*4];        // l 6 modes ; c 5 modes

  UInt64        bin_count;
#ifdef SIG_IAPU_DEBUG
  int           rdo_mode_idx;
#endif
} sig_iapu_st;
#endif //~SIG_IAPU

#ifdef SIG_PPU
typedef struct _sig_ppu_blk8_info_st {
  int x;
  int y;
  int bs;
  int beta;
  int tc;
  int chroma_tc[2];
} sig_ppu_blk8_info_st;

typedef struct _sig_ppu_st {
  sig_ctx ppu_param_ctx;
  sig_ctx ppu_neb_ctx;
  sig_ctx ppu_reg_ctx;
  sig_ctx ppu_filter_ctx;

  sig_ppu_blk8_info_st blk8_info[64];             // [blk8_idx]
  unsigned char blk8_pq[64][2][2][2][4][4];       // [blk8_idx][part][ori/filtered][pq][row/col][0 1 2 3]
  unsigned char blk8_pq_c[2][64][2][2][2][2][2];  // [cb/cr][blk8_idx][part][ori/filtered][pq][row/col][0 1]
} sig_ppu_st;
#endif //~SIG_PPU

#ifdef SIG_SMT
typedef struct _sig_smt_dat_st_ {
  // ISP
  char still_flag;
  char still_num;
  char trans_num;
  char motion_num;
  char isp_dqp;
  char skip_flag;
  // AI
  char ena;
  char obj;
  char dqp_tab_idx;
  char idx;
  char bin;
  char ai_dqp;
} sig_smt_dat_st;

typedef struct _sig_smt_st {
  sig_ctx isp_set_ctx;
  sig_ctx isp_gld_ctx;
  sig_ctx ai_set_ctx;
  sig_ctx ai_gld_ctx;
  sig_ctx isp_ai_set_ctx;
  sig_ctx isp_ai_gld_ctx;

  int b16_x;
  int b16_y;
  sig_smt_dat_st **pp_smt_gld_data;
} sig_smt_st;
#endif

typedef struct _sigdump_st {
  unsigned int start_count;
  unsigned int end_count;
  bool top;
  bool irpu;
  bool ime_mvp;
  bool ime;
  bool fme;
  bool fme_debug;
  bool col;
  bool mc;
  bool frm_mgr;
  bool ccu;
  bool bit_est;
  bool rdo_sse;
  bool cabac;
  bool cabac_prof;
  int  cabac_prof_os_bin_w;
  int  cabac_prof_resi_nor_bin_throughout;
  int  cabac_prof_resi_byp_bin_throughout;
  int  cabac_no_output;
  bool rec_yuv;
  bool rru;
#ifdef SIG_RRU_DEBUG
  bool rru_debug;
#endif
  bool pru;
  unsigned int input_src;
  bool resi;

  bool bit_stats;
  bool perc_show;
  bool ime_mvp_stats;
  bool hm_rc_yuv;
  bool pic_rc;

  bool iapu;
  bool iapu_misc;
  bool iapu_ref_smp_mux;

  unsigned int reset_resi_model;
  bool ppu;
  bool ppu_filter;
  bool fpga;
  bool vbc;
  bool vbc_low;
  bool vbc_dbg;
  bool smt;
  UInt smt_rand;
  unsigned int testBsOverflow;
  unsigned int testBigClipSmall;
} sigdump_st;

typedef struct _sigpool_st {
  sig_ctx ime_mvp_stats_ctx;
  unsigned int cu_16x16_count;
  unsigned int cu_16x16_1_cand_count;
  unsigned int cu_16x16_2_cand_count;
  unsigned int cu_16x16_2_cand_count_mod;
  unsigned int abs_hor;
  unsigned int abs_ver;

  unsigned int frame_skip;
  unsigned int enc_count;
  unsigned int enc_count_pattern;

  unsigned int write_frm_count;
  unsigned int slice_count;
  unsigned int ctb_count;
  int cu_count[5];  //[4x4, 8x8, 16x16 32x32 64x64]
  int cu_chroma_4x4_count;
  bool is_enable_rc;
  unsigned int initQP;
  unsigned int width;
  unsigned int height;
  unsigned int widthAlignCTB;
  unsigned int heightAlignCTB;
  unsigned int widthAlign8;
  unsigned int heightAlign8;
  unsigned int fb_pitch;
  unsigned int rgb_pitch;
  unsigned int slice_type;
  unsigned int ctb_size;
  unsigned int ctb_idx_x;
  unsigned int ctb_idx_y;
  unsigned int cu_idx_x;
  unsigned int cu_idx_y;
  unsigned int cu_width;
  unsigned int cu_height;
#ifdef SIG_FME
  unsigned int fme_8_cnt;
  unsigned int fme_16_cnt;
  int fme_cur_is_16_blk;
#endif
#ifdef SIG_IME
  unsigned int ime_16_cnt;
#endif
#ifdef SIG_MC
  unsigned int mrg_cnt[3];  //0:8, 1:16, 2:32
#endif
  int is_merge;
  int is_intra;
  bool ccu_is_record;
  int intra_bit_count[2]; //0: 4x4, 1: others
  int intra_bin_count[2]; //0: 4x4, 1: others
  int intra_rd_count[2]; //0: 4x4, 1: others
  sig_intra_golden_st intra_golden;
  sig_me_golden_st me_golden;
  sig_est_golden_st est_golden[4][2]; //[blk size][0:temp 1: best]
  sig_est_golden_intra_st est_golden_intra[2][3]; //[0:temp 1: best] [y cb cr]
#ifdef SIG_IRPU
  sig_amvp_list_ctx amvp_list[2][5]; //[ref_idx][cand]
  int irpu_nb_mv[2][4][2];           //[ab][idx][xy]
#endif

  //--SIG_TOP
  sig_ctx top_rec_ctx[2]; //frame level sigdump tile rec yuv, 0: Y, 1: UV
  sig_ctx top_fw_ctx;
  sig_ctx top_fw_cmdq_ctx;
  sig_ctx top_fw_cmdq_addr_ctx;
  sig_ctx top_ref_ctx;
  sig_ctx top_qp_map;
  //--SIG_TOP end

  //--SIG_IRPU
  sig_ctx irpu_cmd_frm_ctx[3];  //frame level sigdump 8x8, 16x16, 32x32
  sig_ctx irpu_fme_mc_mdl_frm_ctx[3]; //frame level sigdump 8x8, 16x16, 32x32
  sig_ctx irpu_golden_frm_ctx[3];  //frame level sigdump 8x8, 16x16, 32x32
  sig_ctx irpu_col_frm_ctx;  //frame level sigdump col
  sig_ctx irpu_reg_init_ctx;  //frame level sigdump reg
  //--SIG_IRPU end

  //--SIG_IME
  sig_ctx ime_cmd_in_ctx;  //frame level sigdump 8x8, 16x16
  sig_ctx ime_dat_out_ctx;  //frame level sigdump 8x8, 16x16
  sig_ctx ime_dat_out_bin_ctx;  //frame level sigdump 8x8, 16x16
  //--SIG_IME end

  //--SIG_FME
  sig_ctx fme_cmd_in_ctx[2];  //frame level sigdump 8x8, 16x16
  sig_ctx fme_dat_out_ctx[2];  //frame level sigdump 8x8, 16x16
  sig_ctx fme_dat_out_txt_ctx[2];  //frame level sigdump 8x8, 16x16
  //--SIG_FME end

  //--SIG_IME_MVP
  sig_ctx ime_mvp_ctx;  //frame level sigdump
  sig_ctx ime_mvb_ctx;  //frame level sigdump
  //--SIG_IME_MVP end

  //--SIG_COL
  sig_ctx col_ctx;  //frame level sigdump
  //--SIG_COL end

  //--SIG_CCU
  double skip_rd_cost;
  double skip_rd_cost_wt;
  int scan_idx_4x4[5];
  int scan_idx[2];
  sig_ime_mvp_st blk8_mv[4];
  sig_ime_mvp_st blk16_mv;
  bool rru_no_output;
  bool ccu_is_record_cu_bits;
  unsigned int ccu_csbf[3]; //y/u/v
  unsigned char ccu_scan_idx[2]; //y/uv
  int ccu_last_subset[3]; //y/uv
  sig_ctx ccu_cost_ctx;
  sig_ctx ccu_ctx[4];       //slice level sigdump 4x4, 8x8, 16x16, 32x32
  sig_ctx ccu_param_ctx;    //slice level sigdump
  sig_ctx ccu_ime_ctx;      //slice level sigdump
  sig_ctx ccu_rc_ctx;       //slice level sigdump
  sig_ctx ccu_init_tab_ctx; //frame level sigdump
  sig_ctx ccu_qpmap_ctx;    //frame level sigdump
  sig_ctx ccu_resi_ctx;     //frame level sigdump
  sig_ctx ccu_resi_txt_ctx; //frame level sigdump
  sig_ctx ccu_stat_ctx;     //frame level sigdump
  sig_ccu_rc_st *p_ccu_rc_st;
  bool is_comp_split_intra_win[2];  //8x8, 16x16
  //--SIG_CCU end

  //--SIG_BIT_EST
  sig_bit_est_st bit_est_golden[2][3][3]; //[0:temp 1: best] [0: intra 1: MERGE 2: AMVP][Y/Cb/Cr]
  sig_bit_est_st bit_est_golden_32[2][4][3]; //[0:temp 1: best] [tu partition][Y/Cb/Cr]
  bool enable_bit_est;
  sig_ctx bit_est_cmd_ctx[4];  //slice level sigdump, [4x4, 8x8, 16x16 32x32]
  sig_ctx bit_est_gld_ctx[4];  //slice level sigdump, [4x4, 8x8, 16x16 32x32]
  sig_ctx rdo_sse_cmd_ctx[4];  //slice level sigdump, [4x4, 8x8, 16x16 32x32]
  sig_ctx rdo_sse_gld_ctx[4];  //slice level sigdump, [4x4, 8x8, 16x16 32x32]
  int bit_est_cnt[4];  //slice level sigdump, [4x4, 8x8, 16x16 32x32]
  int bit_est_chroma_4x4_cnt;
  int rdo_sse_cnt[4];  //slice level sigdump, [4x4, 8x8, 16x16 32x32]
  int rdo_sse_chroma_4x4_cnt;
  unsigned char chromaWeight;
  //--SIG_BIT_EST end

  //--SIG_CABAC
  unsigned int tu_start_bits_pos;
  unsigned int bits_header;

  unsigned int cur_bits_offset;
  unsigned int bits_header_tmp;
  unsigned int bits_res_tmp;
  unsigned int bits_accu_tmp;
  bool cabac_is_record;
  unsigned int slice_header_size;
  unsigned int slice_data_size[2];  //[w/wo 0x3]
  sig_ctx cabac_ctx;  //frame level sigdump
  sig_ctx cabac_syntax_ctx;  //frame level sigdump
  sig_ctx cabac2ccu_ctx;  //frame level sigdump
  sig_ctx slice_data_ctx[2];  //frame level sigdump, [w/wo 0x3]
  sig_ctx cabac_para_update_ctx;  //frame level sigdump
  sig_ctx cabac_bits_statis_tu_ctx;  //frame level sigdump
  int split_cnt;
  int bin_inc[CVI_MAX_CU_DEPTH]; //lastest idx store lastest split
  char syntax_name[512];
  unsigned int range;
  unsigned int low;
  unsigned int bin_type;
  unsigned int bin_value;
  unsigned int state[2];  // ber/aft
  unsigned int mps[2];  // ber/aft
  unsigned int ctx_inc;
  unsigned int put;
  unsigned int numBits;
  int cur_depth;

  sig_ctx cabac_profile;
  unsigned int ostanding_hist[64];
  unsigned int ostanding_bit_cnt;
  unsigned int nonresi_cabac_bypass_cycles;
  unsigned int nonresi_cabac_normal_cycles;
  unsigned int resi_cabac_bypass_cycles;
  unsigned int resi_cabac_normal_cycles;
  unsigned int hdr_bits_num;
  unsigned int res_bits_num;
  unsigned int frm_hdr_size;
  unsigned int frm_res_size;
  //--SIG_CABAC end

  FILE *input_f; //frame level input yuv dump.
  FILE *input_rotate_f; //frame level input yuv dump.
  sig_ctx mc_mrg_ctx_cmd_in[3];  //frame level sigdump merge []8x8, 16x16 32x32
  sig_ctx mc_mrg_ctx_dat_out[3][2];  //frame level sigdump merge []8x8, 16x16 32x32 []Y/UV
  sig_ctx mc_rec_ctx;  //frame level sigdump NV12

  // dump bit statisitc for rdo bit estimation evaluation
  sig_ctx sig0_bit_stats[2]; // [luma/chroma]
  sig_ctx sig1_bit_stats[2]; // [luma/chroma]
  sig_ctx last_pos_bit_stats[2]; // [luma/chroma]
  sig_ctx cgf_bit_stats[2];
  // encoder dump feature visualization
  sig_ctx perc_show[5]; // [madi/alpha/motion/foreground/foreground_dqp]

  // picture level rc statistic for RC Lib verification
  sig_ctx pic_rc_stats;
  sig_ctx pic_rc_golden;
#ifdef SIG_RRU
  // --SIG_RRU
  sig_ctx rru_cu_ctx[3];        // CU level sigdump, [CU8 / CU16 / CU32]
  sig_ctx rru_pu_ctx[3];        // PU level sigdump of Inter block, [PU8 / PU16 / PU32]
  sig_ctx rru_i_ctx[3];         // PU level sigdump of Intra block, [I4 / I8 / I16]
  sig_ctx rru_cu_gold_ctx[3];   // CU level sigdump of CU golden, [CU8 / CU16 / CU32]

  sig_ctx rru_intra_dct_ctx[3][2][2];   // TU level sigdump of Intra block, [I4 / I8 / I16][In / Out][Luma / Chroma]
  sig_ctx rru_intra_idct_ctx[3][2][2];  // TU level sigdump of Intra block, [I4 / I8 / I16][In / Out][Luma / Chroma]
  sig_ctx rru_inter_dct_ctx[3][2][2];   // TU level sigdump of Inter block, [TU8 / TU16 / TU32][In / Out][Luma / Chroma]
  sig_ctx rru_inter_idct_ctx[3][2][2];  // TU level sigdump of Inter block, [TU8 / TU16 / TU32][In / Out][Luma / Chroma]

  sig_ctx rru_rec_frm_ctx;
  sig_ctx rru_cu_order_ctx;
  sig_ctx rru_i4_rec_ctx;
  sig_ctx rru_i4_resi_ctx;      // quantization output data
  sig_ctx rru_cu_iap_ctx[3];    // CU level sigdump, [CU8 / CU16 / CU32]

  short *p_rru_intp_temp;
  short *p_rru_intp_final;

  int *p_rru_i4_resi_temp[3];     // [y/cb/cr]
  int *p_rru_i4_resi_final[3];    // [y/cb/cr]

  short *p_rru_dct_in_temp[2];    // [Luma / Chroma]
  short *p_rru_dct_in_final[2];
  int *p_rru_dct_out_temp[2];
  int *p_rru_dct_out_final[2];

  int *p_rru_iq_out;
  int *p_rru_idct_in_temp[2];
  int *p_rru_idct_in_final[2];
  short *p_rru_idct_out_temp[2];
  short *p_rru_idct_out_final[2];

  int luma_pu_size;
  int rru_qp[3];
  unsigned char dist_chroma_weight;
  Double rru_cu_cost[2];     // cu cost of [current CU cost / split CUs total cost]
  Double rru_intra_cost[3];  // intra cost of [i4/i8/i16]
  Double rru_intra_cost_i8x4;
  Double rru_intra_cost_wt[3];  // intra weighted cost [i4/i8/i16]

  int rru_is_mrg_win;
  int rru_merge_cand;
  int rru_scan_idx[2];
  bool rru_is_record_si;
  bool rru_is_record_pos;
  bool is_skip;

  sig_rru_gold_st rru_temp_gold;
  sig_rru_gold_st rru_gold;

#ifdef SIG_RRU_DEBUG
  sig_ctx rru_inter_iq_in_ctx[3][2];   // [TU8 / TU16 / TU32][Luma / Chroma]
  int *p_rru_iq_in_temp[2];
  int *p_rru_iq_in_final[2];
#endif //~SIG_RRU_DEBUG
#endif //~SIG_RRU

  // --SIG_RRU end

  // --SIG_PRU start
#ifdef SIG_PRU
  sig_pru_st *p_pru_st;
#endif //~SIG_PRU
  // --SIG_PRU end
#ifdef SIG_RESI
  //for RRU & CABAC residual buffer
  bool resi_is_record_pos;
  sig_ctx resi_frm_ctx;
  short *p_resi_buf[2];       // [Luma / Chroma]
#endif //~SIG_RESI

#ifdef SIG_IAPU
  // --SIG_IAPU
//{{{
  sig_ctx       iapu_rmd_tu_cmd_frm_ctx[3];
  sig_ctx       iapu_rmd_tu_src_frm_ctx[3];
  sig_ctx       iapu_rmd_tu_cand_frm_ctx[3];
  sig_ctx       iapu_rmd_tu_pred_blk_frm_ctx[3];

  sig_ctx       iapu_iap_tu_cmd_frm_ctx[3];
  sig_ctx       iapu_iap_tu_neb_frm_ctx[3];
  sig_ctx       iapu_iap_tu_cand_frm_ctx[3];
  sig_ctx       iapu_iap_tu4_coef_blk_frm_ctx;
  sig_ctx       iapu_iap_tu4_rec_blk_frm_ctx;
  sig_ctx       iapu_iap_tu8_rec_blk_frm_ctx;
  sig_ctx       iapu_iap_tu16_rec_blk_frm_ctx;
  sig_ctx       iapu_iap_tu_src_frm_ctx[3];

  sig_ctx       iapu_iap_cu8_cmd_frm_ctx;
  sig_ctx       iapu_iap_cu8_rec_frm_ctx;
  sig_ctx       iapu_iap_line_buffer_frm_ctx;
  sig_ctx       iapu_iap_tu4_pred_frm_ctx;
  sig_ctx       iapu_iap_syntax;

  sig_ctx       iapu_misc_frm_ctx[3];

  int           flag_iapu_luma_stage;
  int           flag_iapu_chroma_stage;
  sig_iapu_st   iapu_st;

  sig_ctx       iapu_rmd_ref_smp_mux_ctx;
  int           iapu_rmd_ref_smp_mux_cnt[3][35];
//}}}
#endif
#ifdef SIG_PPU
#ifdef SIG_PPU_OLD
  sig_ctx ppu_frm_ctx[5];
#endif
  sig_ctx ppu_input_ctx;
  sig_ppu_st *p_ppu_st;
#endif //~SIG_PPU
  //--SIG_FPGA
  sig_ctx fpga_pat_info_ctx;
  sig_ctx fpga_md5_ctx;
  sig_ctx fpga_hdr_ctx;

  // for FPGA statistic
  int fpga_frame_skip;
  int fpga_intra_period;
  int fpga_frame_rate;
  int fpga_bitrate;
  //--SIG_TOP end

  //VBC
  sig_ctx vbe_meta_debug_ctx[2];
  sig_ctx vbe_mode_ctx;
  sig_ctx vbe_enc_info_ctx;
  sig_ctx vbe_frame_ctx;
  sig_ctx vbe_rec_ctx;
  sig_ctx vbe_bs_ctx;
  sig_ctx vbd_dma_meta;
  sig_ctx vbd_dma_meta_cpx[2];
  sig_ctx vbe_strm_pack_out_ctx;
  sig_ctx vbe_src_ctx[2];
  sig_ctx vbe_lossy_src_ctx[2];
  sig_ctx vbd_dma_bs[2];
  sig_ctx vbe_pack_out_ctx[2];
  sig_ctx vbc_dma_cu_info[2];
  sig_ctx vbe_pu_str_ctx[2][2]; //[left, right] [str0 str1]
  sig_ctx vbe_pu_str_out_ctx[2][2]; //[left, right] [str0 str1]
  int cu_best_bs[2];
  int org_len; //left cu
  int lossy_diff_sum[2];
  int best_mode_HW[2][2];//[l/r][gp]
  int best_size_HW[2][2];//[l/r][gp]
  int uncompress[2][2];//[l/r][gp]
  int var_mean;
  int diff_mean;
  int diff_max;
  int A[64];
  int B[64];
  int C[64];
  int D[64];
  int ctx_k[64];
  int resi[64];
  uint64_t codeCU_HW[10][64];//[mode][cusize]
  uint64_t codeCULength_HW[10][64];//[mode][cusize]
  sig_ctx vbd_dma_bs_input_nv12;

#ifdef SIG_SMT
  // Smart Enc
  sig_smt_st *p_smt_st;
  sig_ctx isp_src_ctx;
  sig_ctx ai_src_ctx;
#endif
} sigpool_st;

extern sigdump_st g_sigdump;
extern sigpool_st g_sigpool;

extern string g_FileName;

void sigdump_open_ctx_bin(sig_ctx *p_ctx, const string SigdumpFileName);
void sigdump_open_ctx_bin_r(sig_ctx *p_ctx, const string SigdumpFileName);
void sigdump_open_ctx_txt(sig_ctx *p_ctx, const string SigdumpFileName);
void sigdump_open_ctx_txt_r(sig_ctx *p_ctx, const string SigdumpFileName);
void sigdump_safe_close(sig_ctx *p_ctx);

string get_file_name(void);
bool init_file_name(string FileName);
bool sigdump_enable(sigdump_st *g_sigdump);
bool sigdump_disable(sigdump_st *g_sigdump);
bool sigdump_framewise_enable(void);
bool sigdump_framewise_disable(SliceType slice_type);
bool sigdump_slicewise_enable(int pocCurr);
bool sigdump_slicewise_disable(void);
bool sigdump_cu_start(void);
bool sigdump_ctb_start(void);
bool sigdump_ctb_end(void);
bool sigdump_ctb_enc_start(const unsigned int idx_x, const unsigned int idx_y);
bool sigdump_ctb_enc_end(void);
bool sigdump_output_bin(const sig_ctx *ctx, unsigned char * buf_ptr, unsigned int byte_size);
bool sigdump_output_fprint(const sig_ctx *ctx, const char *format, ...);
int sigdump_vfscanf(const sig_ctx *ctx, const char * format, ...);
void sigdump_reset_amvp_list(int ref_idx);
void sigdump_output_cabac_info(void);
bool is_enable_cabac_hw_emulator();
void cabac_prof_add_bins(CABAC_SYNTAX_TYPE syntax_t, CABAC_BIN_TYPE bin_t, int bin_num);
void sigdump_output_rdo_sse_inter(int blk_size);
void sigdump_output_rdo_sse_intra(int blk_size, unsigned int uiPartOffset, int comp);
static __inline int sig_pat_blk_size_8(int cu_width)
{
  return (cu_width == 32) ? 2 : (cu_width == 16) ? 1 : 0;
}
static __inline int sig_pat_blk_size_4(int cu_width)
{
  return (cu_width == 64) ? 4 : (cu_width == 32) ? 3 : (cu_width == 16) ? 2 : (cu_width == 8) ? 1 : 0;
}
static __inline int sig_pat_blk_size(int cu_width)
{
  return sig_pat_blk_size_8(cu_width);
}

void sigdump_bit_est(int blk_size, int pred_mode, int compNum);
void sigdump_cabac_syntax_info(const char *syntax_name, int grp_id, int stx_id, int ctx_idx, int stx_val, int ctx_inc);
bool is_last_8x8_in_16x16(int cu_idx_x, int cu_idx_y);

void sigdump_open_top_ref_ctx(int fb_idx);
void sigdump_close_top_ref_ctx();
void sigdump_pic_rc_init(int targetRate, int frameNum, int frameRate, int intraPeriod, int statTime, \
                        int ipQpDelta, int numOfPixel, int rcMaxIprop, int rcMinIprop, \
                        int rcMaxQp, int rcMinQp, int rcMaxIQp, int rcMinIQp, \
                        int firstFrmstartQp, int rcMdlUpdatType);


#ifdef CVI_RANDOM_ENCODE
void cvi_srand(unsigned int seed);
int cvi_rand();
int cvi_random_range(int min, int max);
int cvi_random_range_step(int min, int max, int step);
int cvi_random_equal();
bool cvi_random_probability(int prob);
#endif //~CVI_RANDOM_ENCODE

#endif /* __CVI_SIGDUMP__ */
