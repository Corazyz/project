// $Module: vc_enc $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Fri, 01 Mar 2024 03:22:40 PM $
//

#ifndef __REG_VC_ENC_STRUCT_H__
#define __REG_VC_ENC_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*ENC force P/B all INTRA CU;*/
		uint32_t reg_enc_force_all_intra:1;
		/*ENC force P/B all INTER CU
		priority, force inter > force intra;*/
		uint32_t reg_enc_force_all_inter:1;
		/*ENC force P/B all zero MV;*/
		uint32_t reg_enc_force_zero_mv:1;
		/*ENC force P/B skip enable
		priority force skip > force zero MV;*/
		uint32_t reg_enc_force_skip:1;
		/*ENC support two reference frames;*/
		uint32_t reg_enc_multi_ref_en:1;
		/*ENC constrained MV enable to avoid cross picture boundary for IME/FME;*/
		uint32_t reg_enc_cons_mv_en:1;
		/*ENC constrained MVD enable for IME to reduce candidates;*/
		uint32_t reg_enc_cons_mvd_en:1;
		/*ENC MV Clip enable for IME_AMVP;*/
		uint32_t reg_enc_mv_clip_en:1;
		/*ENC constrained frational part of MV 
		0: disable (no constraint)
		1: only support 0.5 (half) for any size of blocks.
		2, 3: Reserved
		4: only support 0.5 (half) for 16x16 blocks.
		5: only support integer for 8x8 blocks.;*/
		uint32_t reg_enc_cons_fme:4;
		/*ENC constrained Merge candidates enable;*/
		uint32_t reg_enc_cons_mrg:1;
		uint32_t rsv_13_29:17;
		/*ENC foreground cost enable;*/
		uint32_t reg_enc_fg_cost_en:1;
		/*ENC Disable I4 in P frame;*/
		uint32_t reg_enc_dis_inter_i4:1;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h00_C;
typedef union {
	struct {
		/*ENC constrained PUX low bound, pixel integer part from 0
		Note: 2's complement S14;*/
		uint32_t reg_enc_cons_pux_lbnd:15;
		uint32_t rsv_15_15:1;
		/*ENC constrained PUX upper bound, pixel integer part from 0;*/
		uint32_t reg_enc_cons_pux_ubnd:15;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h04_C;
typedef union {
	struct {
		/*ENC constrained PUY low bound, pixel integer part from 0;*/
		uint32_t reg_enc_cons_puy_lbnd:15;
		uint32_t rsv_15_15:1;
		/*ENC constrained PUY upper bound, pixel integer part from 0;*/
		uint32_t reg_enc_cons_puy_ubnd:15;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h08_C;
typedef union {
	struct {
		/*ENC IME X search range,
		0: -8, 7 
		1: -16, 15
		2: -32, 31
		3: -64, 63;*/
		uint32_t reg_enc_ime_xsr:2;
		uint32_t rsv_2_3:2;
		/*ENC IME Y search range,
		0: -8, 7 
		1: -16, 15
		2: -32, 31
		3: -64, 63;*/
		uint32_t reg_enc_ime_ysr:2;
		uint32_t rsv_6_6:1;
		/*ENC AVC IME TMP enable;*/
		uint32_t reg_avc_ime_tmp_en:1;
		/*ENC constrained Merge MVX to reduce candidates;*/
		uint32_t reg_enc_mrg_mvx_thr:4;
		/*ENC constrained Merge MVY to reduce candidates;*/
		uint32_t reg_enc_mrg_mvy_thr:4;
		/*ENC constrained integer MVDX for IME to reduce candidates
		if abs(mvdx) < mvdx_thr and abs(mvdy) < mvdy_thr
		then reduce candidates to Single;*/
		uint32_t reg_enc_ime_mvdx_thr:8;
		/*ENC constrained integer MVDY for IME to reduce candidates
		if abs(mvdx) < mvdx_thr and abs(mvdy) < mvdy_thr
		then reduce candidates to Single;*/
		uint32_t reg_enc_ime_mvdy_thr:8;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h0C_C;
typedef union {
	struct {
		/*ENC fixed QP flag;*/
		uint32_t reg_enc_fixed_qp:1;
		/*ENC QPMap enable
		0: Disable
		1: QPMAP in DRAM
		2: 8 Windows
		3: Reserved;*/
		uint32_t reg_enc_qpmap_mode:2;
		uint32_t rsv_3_3:1;
		/*ENC QP mode if CU size is great than 16
		0: max
		1: min
		2: average
		3: reserved;*/
		uint32_t reg_enc_qp_mode:2;
		uint32_t rsv_6_7:2;
		/*ENC QP offset fuse
		0: MADI_QP_OFS
		1: 0.75 * MADI_QP_OFS + 0.25 AVG_QP_OFS
		2: 0.5 * MADI_QP_OFS + 0.5 AVG_QP_OFS
		3: 0.25 * MADI_QP_OFS + 0.75 AVG_QP_OFS
		4: AVG_QP_OFS;*/
		uint32_t reg_enc_qp_ofs_fuse:3;
		uint32_t rsv_11_15:5;
		/*ENC enable QP for 8 windows;*/
		uint32_t reg_enc_qp_win_en:8;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h14_C;
typedef union {
	struct {
		/*ENC QP Update, 0: CTU, 1: CU;*/
		uint32_t reg_enc_qp_upd:1;
		/*ENC constrain QP clip(QP_MAX, QP_MIN, CurQP);*/
		uint32_t reg_enc_cons_qp:1;
		/*ENC constrain DQP clip(DQP_MAX, DQP_MIN, CurDQP);*/
		uint32_t reg_enc_cons_delta_qp:1;
		/*ENC enable to output MAD information;*/
		uint32_t reg_enc_mad_out_en:1;
		/*ENC enable to read CU depth information;*/
		uint32_t reg_enc_cudep_en:1;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h18_C;
typedef union {
	struct {
		/*ENC Max QP, 63;*/
		uint32_t reg_enc_max_qp:6;
		uint32_t rsv_6_7:2;
		/*ENC Min QP, 0;*/
		uint32_t reg_enc_min_qp:6;
		uint32_t rsv_14_15:2;
		/*ENC Max delta QP, S5, 31;*/
		uint32_t reg_enc_max_delta_qp:6;
		uint32_t rsv_22_23:2;
		/*ENC Min delta QP, S5, -32;*/
		uint32_t reg_enc_min_delta_qp:6;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h1C_C;
typedef union {
	struct {
		/*ENC MV lower boundary, default = -256.0 + SR = -1024 + 67 = -957;*/
		uint32_t reg_enc_mv_range_lbnd:16;
		/*ENC MV upper boundary, default = 255.75 - SR = 1023 - 67 = 956;*/
		uint32_t reg_enc_mv_range_ubnd:16;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h20_C;
typedef union {
	struct {
		/*ENC MVD lower boundary, default = -512. 0 + SR = -2048 + 67 = -1981;*/
		uint32_t reg_enc_mvd_range_lbnd:16;
		/*ENC MVD upper boundary, default = 511.75 - SR = 2047 - 67 = 1980;*/
		uint32_t reg_enc_mvd_range_ubnd:16;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h24_C;
typedef union {
	struct {
		/*Row average bit bumber;*/
		uint32_t reg_row_avg_bit:20;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h40_C;
typedef union {
	struct {
		/*Error Compensation scale;*/
		uint32_t reg_err_comp_scl:10;
		uint32_t rsv_10_15:6;
		/*Division normalized scale;*/
		uint32_t reg_ctu_num_norm_scl:10;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h44_C;
typedef union {
	struct {
		/*Row QP delta;*/
		uint32_t reg_row_qp_delta:4;
		uint32_t rsv_4_7:4;
		/*Row overflow QP delta;*/
		uint32_t reg_row_ovf_qp_delta:2;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h48_C;
typedef union {
	struct {
		/*I4 MADI early terminated threshold;*/
		uint32_t reg_i4_madi_thr:8;
		uint32_t rsv_8_30:23;
		/*I4 MADI early terminated enable;*/
		uint32_t reg_i4_early_term_en:1;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h4C_C;
typedef union {
	struct {
		/*I4 MADI early terminated target;*/
		uint32_t reg_pic_i4_term_target:22;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h50_C;
typedef union {
	struct {
		/*LMS LR M, (Q.16) << 6, left fraction part;*/
		uint32_t reg_lms_lrm:10;
		uint32_t rsv_10_15:6;
		/*LMS LR C, (Q.16) << 6, left fraction part;*/
		uint32_t reg_lms_lrc:10;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h54_C;
typedef union {
	struct {
		/*LMS M max, Q3.15;*/
		uint32_t reg_lms_m_max:18;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h58_C;
typedef union {
	struct {
		/*LMS M max, Q3.15;*/
		uint32_t reg_lms_m_min:18;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h5C_C;
typedef union {
	struct {
		/*LMS C max, Q3.15;*/
		uint32_t reg_lms_c_max:18;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h60_C;
typedef union {
	struct {
		/*LMS C min, Q3.15;*/
		uint32_t reg_lms_c_min:18;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h64_C;
typedef union {
	struct {
		/*ENC Distortion Chroma Weight Q3.5;*/
		uint32_t reg_dist_chroma_weight:8;
		/*ENC Motion level MV gain Q2.4;*/
		uint32_t reg_ml_mv_gain:6;
		uint32_t rsv_14_15:2;
		/*ENC Motion level SATD gain Q2.4;*/
		uint32_t reg_ml_satd_gain:6;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h70_C;
typedef union {
	struct {
		/*ENC foreground threshold gain;*/
		uint32_t reg_fg_th_gain:6;
		uint32_t rsv_6_7:2;
		/*ENC foreground threshold offset;*/
		uint32_t reg_fg_th_ofs:7;
		uint32_t rsv_15_15:1;
		/*ENC foreground threshold bias;*/
		uint32_t reg_fg_th_bias:8;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h74_C;
typedef union {
	struct {
		/*ENC Program Table go;*/
		uint32_t reg_enc_prg_tab_go:1;
		/*ENC Program Table read/write;*/
		uint32_t reg_enc_prg_tab_rw:1;
		uint32_t rsv_2_3:2;
		/*ENC Program Table index
		0: ROI WIN START POS, ROI WIN QP, ROI WIN END POS
		4: TCL/LumaLUT
		5: QLLUT
		8: SI_TAB (Self-Information Table)
		9: Ratio (Bit estimation for LastPosXY)
		10: M/C table (Bit estimation for SIG and GT1)
		12: Row QP
		13: BG/FG
		14: ai dqp tab
		15: ai object tab;*/
		uint32_t reg_enc_prg_tab_idx:4;
		uint32_t rsv_8_15:8;
		/*ENC Program Table read/write address
		4: TC/LumaLUT
		  total 12 entry
		8: 16 entry.
		9: Ratio
		  a0: {inter_L_TU8, inter_L_TU4}, 
		  a1: {inter_L_TU32, inter_L_TU16}
		  a2: {inter_C_TU8, inter_C_TU4}, 
		  a3: {N/A, N/A}
		  a4: {intra_L_TU8, intra_L_TU4}, 
		  a5: {intra_L_TU32, intra_L_TU16}
		  a6: {intra_C_TU8, intra_C_TU4}, 
		  a7: {N/A, N/A}
		10: M/C
		  a0~a3, luma inter m_sig0/1, c_sig0/1, tu4
		  a4~a15,  luma inter m_sig0/1, c_sig0/1, tu8/16/32
		  a16~a31,  luma intra m_sig0/1, c_sig0/1, tu4/8/16/32
		  a32 ~ a63, chroma inter/intra m/c sig0/1
		13: BG/FG
		  a0: intra32/16/8/4 BG_weight
		  a1: inter32/16/8/4 BG_weight
		  a2: skip32/16/8 BG_weight
		  a3, 4: intra32/16/8 BG_Bias
		  a5, 6: inter32/16/8 BG_Bias
		  a7, 8: inter32/16/8 BG_Bias
		  a9: intra32/16/8/4 FG_weight
		  a10: inter32/16/8/4 FG_weight
		  a11: skip32/16/8 FG_weight
		  a12, 13: intra32/16/8 FG_Bias
		  a14, 15: inter32/16/8 FG_Bias
		  a16: skip32/16/8 FG_Bias
		14: ai dqp tab 8*16 entry
		15: ai obj tab 64 entry;*/
		uint32_t reg_enc_prg_tab_addr:8;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h80_C;
typedef union {
	struct {
		/*ENC Program Table write data
		0: ROI WIN POS, 
		  bit31~16, Y Start align 16pixels
		  bit15~0,   X Start align 16pixels
		1: ROI WIN QP
		  bit0: ABS QP,
		  bit15~bit8, QP
		2: ROI WIN W/H
		  bit31~bit16, Y end align 16pixels
		  bit15~bit0, X end align 16pixels
		4: TCLUT
		    {Check FW programming guide}
		5: QLLUT
		   {SQRT_LAMBDA(15bits), LAMBAD(16bits)}
		8: {MPS (16bits), LPS (16bits)}
		9: {High WORD(16bits), Low WORD(16bits)}
		10: 18bits
		14: dqp tab 5bits
		15: obj tab 4bits {tab_idx, enable};*/
		uint32_t reg_enc_prg_tab_wd:32;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h84_C;
typedef union {
	struct {
		/*ENC Program Table read data;*/
		uint32_t reg_enc_prg_tab_rd:32;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h88_C;
typedef union {
	struct {
		/*smt enc ai dqp enable;*/
		uint32_t reg_ai_map_en:1;
		/*smt enc ai dqp smooth enable;*/
		uint32_t reg_ai_smooth_qp_en:1;
		/*smt enc ai confidence scale (default = 41);*/
		uint32_t reg_conf_scale:6;
		/*smt enc ai clip3 min (default = 0);*/
		uint32_t reg_tclip_min:4;
		/*smt enc ai clip3 max (default = 15);*/
		uint32_t reg_tclip_max:4;
		/*smt enc isp dqp enable;*/
		uint32_t reg_isp_map_en:1;
		/*smt enc isp skip map enable;*/
		uint32_t reg_skip_map_en:1;
		/*smt enc isp trans value (default = -7);*/
		uint32_t reg_trans_qp:7;
		/*smt enc isp trans value (default = -3);*/
		uint32_t reg_motion_qp:7;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h8C_C;
typedef union {
	struct {
		/*smt enc ai/isp dqp clip min (default = -10);*/
		uint32_t reg_clip_delta_qp_min:7;
		uint32_t rsv_5_7:1;
		/*smt enc ai/isp dqp clip min (default = 10);*/
		uint32_t reg_clip_delta_qp_max:7;
	};
	uint32_t val;
} VC_ENC_VC_ENC_h90_C;

typedef struct {
	volatile VC_ENC_VC_ENC_h00_C VC_ENC_h00;
	volatile VC_ENC_VC_ENC_h04_C VC_ENC_h04;
	volatile VC_ENC_VC_ENC_h08_C VC_ENC_h08;
	volatile VC_ENC_VC_ENC_h0C_C VC_ENC_h0C;
	volatile VC_ENC_VC_ENC_h14_C VC_ENC_h14;
	volatile VC_ENC_VC_ENC_h18_C VC_ENC_h18;
	volatile VC_ENC_VC_ENC_h1C_C VC_ENC_h1C;
	volatile VC_ENC_VC_ENC_h20_C VC_ENC_h20;
	volatile VC_ENC_VC_ENC_h24_C VC_ENC_h24;
	volatile VC_ENC_VC_ENC_h40_C VC_ENC_h40;
	volatile VC_ENC_VC_ENC_h44_C VC_ENC_h44;
	volatile VC_ENC_VC_ENC_h48_C VC_ENC_h48;
	volatile VC_ENC_VC_ENC_h4C_C VC_ENC_h4C;
	volatile VC_ENC_VC_ENC_h50_C VC_ENC_h50;
	volatile VC_ENC_VC_ENC_h54_C VC_ENC_h54;
	volatile VC_ENC_VC_ENC_h58_C VC_ENC_h58;
	volatile VC_ENC_VC_ENC_h5C_C VC_ENC_h5C;
	volatile VC_ENC_VC_ENC_h60_C VC_ENC_h60;
	volatile VC_ENC_VC_ENC_h64_C VC_ENC_h64;
	volatile VC_ENC_VC_ENC_h70_C VC_ENC_h70;
	volatile VC_ENC_VC_ENC_h74_C VC_ENC_h74;
	volatile VC_ENC_VC_ENC_h80_C VC_ENC_h80;
	volatile VC_ENC_VC_ENC_h84_C VC_ENC_h84;
	volatile VC_ENC_VC_ENC_h88_C VC_ENC_h88;
	volatile VC_ENC_VC_ENC_h8C_C VC_ENC_h8C;
	volatile VC_ENC_VC_ENC_h90_C VC_ENC_h90;
} VC_ENC_C;
#define DEFINE_VC_ENC_C(X) \
	 VC_ENC_C X = \
{\
	.VC_ENC_h00.reg_enc_force_all_intra = 0x0,\
	.VC_ENC_h00.reg_enc_force_all_inter = 0x0,\
	.VC_ENC_h00.reg_enc_force_zero_mv = 0x0,\
	.VC_ENC_h00.reg_enc_force_skip = 0x0,\
	.VC_ENC_h00.reg_enc_multi_ref_en = 0x0,\
	.VC_ENC_h00.reg_enc_cons_mv_en = 0x0,\
	.VC_ENC_h00.reg_enc_cons_mvd_en = 0x0,\
	.VC_ENC_h00.reg_enc_mv_clip_en = 0x0,\
	.VC_ENC_h00.reg_enc_cons_fme = 0x0,\
	.VC_ENC_h00.reg_enc_cons_mrg = 0x0,\
	.VC_ENC_h00.reg_enc_fg_cost_en = 0x0,\
	.VC_ENC_h00.reg_enc_dis_inter_i4 = 0x0,\
	.VC_ENC_h04.reg_enc_cons_pux_lbnd = 0x0,\
	.VC_ENC_h04.reg_enc_cons_pux_ubnd = 0x3FFF,\
	.VC_ENC_h08.reg_enc_cons_puy_lbnd = 0x0,\
	.VC_ENC_h08.reg_enc_cons_puy_ubnd = 0x3FFF,\
	.VC_ENC_h0C.reg_enc_ime_xsr = 0x2,\
	.VC_ENC_h0C.reg_enc_ime_ysr = 0x2,\
	.VC_ENC_h0C.reg_avc_ime_tmp_en = 0x1,\
	.VC_ENC_h0C.reg_enc_mrg_mvx_thr = 0x4,\
	.VC_ENC_h0C.reg_enc_mrg_mvy_thr = 0x4,\
	.VC_ENC_h0C.reg_enc_ime_mvdx_thr = 0x15,\
	.VC_ENC_h0C.reg_enc_ime_mvdy_thr = 0x15,\
	.VC_ENC_h14.reg_enc_fixed_qp = 0x0,\
	.VC_ENC_h14.reg_enc_qpmap_mode = 0x0,\
	.VC_ENC_h14.reg_enc_qp_mode = 0x0,\
	.VC_ENC_h14.reg_enc_qp_ofs_fuse = 0x0,\
	.VC_ENC_h14.reg_enc_qp_win_en = 0x0,\
	.VC_ENC_h18.reg_enc_qp_upd = 0x0,\
	.VC_ENC_h18.reg_enc_cons_qp = 0x1,\
	.VC_ENC_h18.reg_enc_cons_delta_qp = 0x1,\
	.VC_ENC_h18.reg_enc_mad_out_en = 0x0,\
	.VC_ENC_h18.reg_enc_cudep_en = 0x0,\
	.VC_ENC_h1C.reg_enc_max_qp = 0x3F,\
	.VC_ENC_h1C.reg_enc_min_qp = 0x0,\
	.VC_ENC_h1C.reg_enc_max_delta_qp = 0x1F,\
	.VC_ENC_h1C.reg_enc_min_delta_qp = 0x20,\
	.VC_ENC_h20.reg_enc_mv_range_lbnd = 0xfc43,\
	.VC_ENC_h20.reg_enc_mv_range_ubnd = 0x3bc,\
	.VC_ENC_h24.reg_enc_mvd_range_lbnd = 0xf843,\
	.VC_ENC_h24.reg_enc_mvd_range_ubnd = 0x7bc,\
	.VC_ENC_h40.reg_row_avg_bit = 0x0,\
	.VC_ENC_h44.reg_err_comp_scl = 0x0,\
	.VC_ENC_h44.reg_ctu_num_norm_scl = 0x0,\
	.VC_ENC_h48.reg_row_qp_delta = 0x0,\
	.VC_ENC_h48.reg_row_ovf_qp_delta = 0x0,\
	.VC_ENC_h4C.reg_i4_madi_thr = 0x0,\
	.VC_ENC_h4C.reg_i4_early_term_en = 0x0,\
	.VC_ENC_h50.reg_pic_i4_term_target = 0x0,\
	.VC_ENC_h54.reg_lms_lrm = 0x0,\
	.VC_ENC_h54.reg_lms_lrc = 0x0,\
	.VC_ENC_h58.reg_lms_m_max = 0x0,\
	.VC_ENC_h5C.reg_lms_m_min = 0x0,\
	.VC_ENC_h60.reg_lms_c_max = 0x0,\
	.VC_ENC_h64.reg_lms_c_min = 0x0,\
	.VC_ENC_h70.reg_dist_chroma_weight = 0x20,\
	.VC_ENC_h70.reg_ml_mv_gain = 0x10,\
	.VC_ENC_h70.reg_ml_satd_gain = 0x10,\
	.VC_ENC_h74.reg_fg_th_gain = 0x10,\
	.VC_ENC_h74.reg_fg_th_ofs = 0x00,\
	.VC_ENC_h74.reg_fg_th_bias = 0x10,\
	.VC_ENC_h80.reg_enc_prg_tab_go = 0x0,\
	.VC_ENC_h80.reg_enc_prg_tab_rw = 0x0,\
	.VC_ENC_h80.reg_enc_prg_tab_idx = 0x0,\
	.VC_ENC_h80.reg_enc_prg_tab_addr = 0x0,\
	.VC_ENC_h84.reg_enc_prg_tab_wd = 0x0,\
	.VC_ENC_h88.reg_enc_prg_tab_rd = 0x0,\
	.VC_ENC_h8C.reg_ai_map_en = 0x0,\
	.VC_ENC_h8C.reg_ai_smooth_qp_en = 0x0,\
	.VC_ENC_h8C.reg_conf_scale = 0x29,\
	.VC_ENC_h8C.reg_tclip_min = 0x0,\
	.VC_ENC_h8C.reg_tclip_max = 0xf,\
	.VC_ENC_h8C.reg_isp_map_en = 0x0,\
	.VC_ENC_h8C.reg_skip_map_en = 0x0,\
	.VC_ENC_h8C.reg_trans_qp = 0x79,\
	.VC_ENC_h8C.reg_motion_qp = 0x7d,\
	.VC_ENC_h90.reg_clip_delta_qp_min = 0x4d,\
	.VC_ENC_h90.reg_clip_delta_qp_max = 0x33,\
};

void init_vc_enc_reg(VC_ENC_C &enc)
{
    enc.VC_ENC_h00.reg_enc_force_all_intra = 0x0;
    enc.VC_ENC_h00.reg_enc_force_all_inter = 0x0;
    enc.VC_ENC_h00.reg_enc_force_zero_mv = 0x0;
    enc.VC_ENC_h00.reg_enc_force_skip = 0x0;
    enc.VC_ENC_h00.reg_enc_multi_ref_en = 0x0;
    enc.VC_ENC_h00.reg_enc_cons_mv_en = 0x0;
    enc.VC_ENC_h00.reg_enc_cons_mvd_en = 0x0;
    enc.VC_ENC_h00.reg_enc_mv_clip_en = 0x0;
    enc.VC_ENC_h00.reg_enc_cons_fme = 0x0;
    enc.VC_ENC_h00.reg_enc_cons_mrg = 0x0;
    enc.VC_ENC_h00.reg_enc_fg_cost_en = 0x0;
    enc.VC_ENC_h00.reg_enc_dis_inter_i4 = 0x0;
    enc.VC_ENC_h04.reg_enc_cons_pux_lbnd = 0x0;
    enc.VC_ENC_h04.reg_enc_cons_pux_ubnd = 0x3FFF;
    enc.VC_ENC_h08.reg_enc_cons_puy_lbnd = 0x0;
    enc.VC_ENC_h08.reg_enc_cons_puy_ubnd = 0x3FFF;
    enc.VC_ENC_h0C.reg_enc_ime_xsr = 0x2;
    enc.VC_ENC_h0C.reg_enc_ime_ysr = 0x2;
    enc.VC_ENC_h0C.reg_avc_ime_tmp_en = 0x1;
    enc.VC_ENC_h0C.reg_enc_mrg_mvx_thr = 0x4;
    enc.VC_ENC_h0C.reg_enc_mrg_mvy_thr = 0x4;
    enc.VC_ENC_h0C.reg_enc_ime_mvdx_thr = 0x15;
    enc.VC_ENC_h0C.reg_enc_ime_mvdy_thr = 0x15;
    enc.VC_ENC_h14.reg_enc_fixed_qp = 0x0;
    enc.VC_ENC_h14.reg_enc_qpmap_mode = 0x0;
    enc.VC_ENC_h14.reg_enc_qp_mode = 0x0;
    enc.VC_ENC_h14.reg_enc_qp_ofs_fuse = 0x0;
    enc.VC_ENC_h14.reg_enc_qp_win_en = 0x0;
    enc.VC_ENC_h18.reg_enc_qp_upd = 0x0;
    enc.VC_ENC_h18.reg_enc_cons_qp = 0x1;
    enc.VC_ENC_h18.reg_enc_cons_delta_qp = 0x1;
    enc.VC_ENC_h18.reg_enc_mad_out_en = 0x0;
    enc.VC_ENC_h18.reg_enc_cudep_en = 0x0;
    enc.VC_ENC_h1C.reg_enc_max_qp = 0x3F;
    enc.VC_ENC_h1C.reg_enc_min_qp = 0x0;
    enc.VC_ENC_h1C.reg_enc_max_delta_qp = 0x1F;
    enc.VC_ENC_h1C.reg_enc_min_delta_qp = 0x20;
    enc.VC_ENC_h20.reg_enc_mv_range_lbnd = 0xfc43;
    enc.VC_ENC_h20.reg_enc_mv_range_ubnd = 0x3bc;
    enc.VC_ENC_h24.reg_enc_mvd_range_lbnd = 0xf843;
    enc.VC_ENC_h24.reg_enc_mvd_range_ubnd = 0x7bc;
    enc.VC_ENC_h40.reg_row_avg_bit = 0x0;
    enc.VC_ENC_h44.reg_err_comp_scl = 0x0;
    enc.VC_ENC_h44.reg_ctu_num_norm_scl = 0x0;
    enc.VC_ENC_h48.reg_row_qp_delta = 0x0;
    enc.VC_ENC_h48.reg_row_ovf_qp_delta = 0x0;
    enc.VC_ENC_h4C.reg_i4_madi_thr = 0x0;
    enc.VC_ENC_h4C.reg_i4_early_term_en = 0x0;
    enc.VC_ENC_h50.reg_pic_i4_term_target = 0x0;
    enc.VC_ENC_h54.reg_lms_lrm = 0x0;
    enc.VC_ENC_h54.reg_lms_lrc = 0x0;
    enc.VC_ENC_h58.reg_lms_m_max = 0x0;
    enc.VC_ENC_h5C.reg_lms_m_min = 0x0;
    enc.VC_ENC_h60.reg_lms_c_max = 0x0;
    enc.VC_ENC_h64.reg_lms_c_min = 0x0;
    enc.VC_ENC_h70.reg_dist_chroma_weight = 0x20;
    enc.VC_ENC_h70.reg_ml_mv_gain = 0x10;
    enc.VC_ENC_h70.reg_ml_satd_gain = 0x10;
    enc.VC_ENC_h74.reg_fg_th_gain = 0x10;
    enc.VC_ENC_h74.reg_fg_th_ofs = 0x00;
    enc.VC_ENC_h74.reg_fg_th_bias = 0x10;
    enc.VC_ENC_h80.reg_enc_prg_tab_go = 0x0;
    enc.VC_ENC_h80.reg_enc_prg_tab_rw = 0x0;
    enc.VC_ENC_h80.reg_enc_prg_tab_idx = 0x0;
    enc.VC_ENC_h80.reg_enc_prg_tab_addr = 0x0;
    enc.VC_ENC_h84.reg_enc_prg_tab_wd = 0x0;
    enc.VC_ENC_h88.reg_enc_prg_tab_rd = 0x0;
    enc.VC_ENC_h8C.reg_ai_map_en = 0x0;
    enc.VC_ENC_h8C.reg_ai_smooth_qp_en = 0x0;
    enc.VC_ENC_h8C.reg_conf_scale = 0x29;
    enc.VC_ENC_h8C.reg_tclip_min = 0x0;
    enc.VC_ENC_h8C.reg_tclip_max = 0xf;
    enc.VC_ENC_h8C.reg_isp_map_en = 0x0;
    enc.VC_ENC_h8C.reg_skip_map_en = 0x0;
    enc.VC_ENC_h8C.reg_trans_qp = 0x79;
    enc.VC_ENC_h8C.reg_motion_qp = 0x7d;
    enc.VC_ENC_h90.reg_clip_delta_qp_min = 0x4d;
    enc.VC_ENC_h90.reg_clip_delta_qp_max = 0x33;
};
#endif //__REG_VC_ENC_STRUCT_H__
