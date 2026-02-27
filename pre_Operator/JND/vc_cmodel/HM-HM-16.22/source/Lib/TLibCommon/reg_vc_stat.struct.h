// $Module: vc_stat $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 28 Jul 2021 07:42:49 PM $
//

#ifndef __REG_VC_STAT_STRUCT_H__
#define __REG_VC_STAT_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*Report MSE SUM of current frame;*/
		uint32_t reg_stat_frm_mse_sum:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h00_C;
typedef union {
	struct {
		/*Report MAX QP of current frame;*/
		uint32_t reg_stat_frm_max_qp:6;
		uint32_t rsv_6_7:2;
		/*Report MIN QP of current frame;*/
		uint32_t reg_stat_frm_min_qp:6;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h10_C;
typedef union {
	struct {
		/*Report QP SUM of current frame;*/
		uint32_t reg_stat_frm_qp_sum:25;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h14_C;
typedef union {
	struct {
		/*Report QP histogram of current frame go;*/
		uint32_t reg_stat_frm_qp_hist_go:1;
		uint32_t rsv_1_7:7;
		/*Report QP histogram index of current frame;*/
		uint32_t reg_stat_frm_qp_hist_idx:6;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h18_C;
typedef union {
	struct {
		/*Report MADP SUM of current frame (unit CTU);*/
		uint32_t reg_stat_frm_qp_hist_rd:20;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h1C_C;
typedef union {
	struct {
		/*Report MADI SUM of current frame (unit CTU);*/
		uint32_t reg_stat_frm_madi_sum:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h30_C;
typedef union {
	struct {
		/*Report MADP SUM of current frame (unit CTU);*/
		uint32_t reg_stat_frm_madp_sum:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h34_C;
typedef union {
	struct {
		/*Report MADI histogram of current frame go;*/
		uint32_t reg_stat_frm_madi_hist_go:1;
		/*Report MADI histogram index of current frame;*/
		uint32_t reg_stat_frm_madi_hist_idx:7;
		/*Report MADI histogram of current frame;*/
		uint32_t reg_stat_frm_madi_hist_rd:24;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h38_C;
typedef union {
	struct {
		/*Report I4 early terminated count;*/
		uint32_t reg_stat_frm_early_term:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h3C_C;
typedef union {
	struct {
		/*Report foreground CU16 count;*/
		uint32_t reg_stat_frm_fg_cu16_cnt:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h40_C;
typedef union {
	struct {
		/*Report bitstream output byte length;*/
		uint32_t reg_stat_frm_bso_blen:25;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h4C_C;
typedef union {
	struct {
		/*Report Header bits SUM of current frame;*/
		uint32_t reg_stat_frm_hdr_sum:20;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h50_C;
typedef union {
	struct {
		/*Report Residual bits SUM of current frame;*/
		uint32_t reg_stat_frm_res_sum:24;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h54_C;
typedef union {
	struct {
		/*Bit Error accumulator;*/
		uint32_t reg_bit_err_accum:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h58_C;
typedef union {
	struct {
		/*Block QP accumulator;*/
		uint32_t reg_blk_qp_accum:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h5C_C;
typedef union {
	struct {
		/*IMV 2Cand Original Count;*/
		uint32_t reg_ime_2cand_org_cnt:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h70_C;

typedef union {
	struct {
		/*IMV 2Cand Count;*/
		uint32_t reg_ime_2cand_cnt:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h74_C;


typedef union {
	struct {
		/*IMV 2Cand sum of ABS MVDX;*/
		uint32_t reg_ime_2cand_abs_mvdx:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h78_C;

typedef union {
	struct {
		/*IMV 2Cand sum of ABS MVDY;*/
		uint32_t reg_ime_2cand_abs_mvdy:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h7C_C;
typedef union {
	struct {
		/*Set/Get SSE information of specified regions;*/
		uint32_t reg_stat_frm_sse_go:1;
		/*Set/Get SSE information of specified regions
		0: read SSE windows settings
		1: set SSE windows corrdinates;*/
		uint32_t reg_stat_frm_sse_rw:1;
		uint32_t rsv_2_3:2;
		/*Set/Get SSE index of regions;*/
		uint32_t reg_stat_frm_sse_idx:3;
		uint32_t rsv_7_15:9;
		/*Set/Get SSE Read type
		0: read SSE
		1: read X start/end
		2: read Y start/end;*/
		uint32_t reg_stat_frm_sse_rtype:3;
		uint32_t rsv_19_23:5;
		/*Set/Get SSE Region enable
		8 bits for 8 regions;*/
		uint32_t reg_stat_frm_sse_ena:8;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h80_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*XST of SSE regions, unit 16pixels;*/
		uint32_t reg_stat_frm_sse_xst:12;
		uint32_t rsv_16_19:4;
		/*XEND of SSE regions, unit 16pixels;*/
		uint32_t reg_stat_frm_sse_xend:12;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h84_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*YST of SSE regions, unit 16pixels;*/
		uint32_t reg_stat_frm_sse_yst:12;
		uint32_t rsv_16_19:4;
		/*YEND of SSE regions, unit 16pixels;*/
		uint32_t reg_stat_frm_sse_yend:12;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h88_C;
typedef union {
	struct {
		/*SSE information of sepcified region;*/
		uint32_t reg_stat_frm_sse_info:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h8C_C;
typedef union {
	struct {
		/*Get frame level M/C information;*/
		uint32_t reg_stat_frm_mc_go:1;
		uint32_t rsv_1_7:7;
		/*M/C index;*/
		uint32_t reg_stat_frm_mc_idx:8;
	};
	uint32_t val;
} VC_STAT_VC_STAT_hA0_C;
typedef union {
	struct {
		/*frame level M/C information;*/
		uint32_t reg_stat_frm_mc_info:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_hA4_C;
typedef union {
	struct {
		/*Get coding unit information go;*/
		uint32_t reg_stat_frm_cu_go:1;
		uint32_t rsv_1_7:7;
		/*Get coding unit information index
		0: count of I4
		1: count of I8
		2: count of I16
		3: count of I32
		4: count of I64
		5: count of I128
		8: count of P/B4
		9: count of P/B8
		10: count of P/B16
		11: count of P/B32
		12: count of P/B64
		13: count of P/B128
		16: count of P/SKIP8
		17: count of P/SKIP16
		18: count of P/SKIP32;*/
		uint32_t reg_stat_frm_cu_idx:8;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h100_C;
typedef union {
	struct {
		/*Coding unit information of current frame;*/
		uint32_t reg_stat_frm_cu_info:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h104_C;
typedef union {
	struct {
		/*Get Performance information go;*/
		uint32_t reg_stat_frm_perf_go:1;
		uint32_t rsv_1_7:7;
		/*Get Performance information index
		0: frame encoding/decoding ticks
		1: frame encoding/decoding bandwidth
		2: decoding frame bit rate
		7~3: Reserved
		8: encoding/decoding bin rate
		9: ME/MC luma bandwidth
		10: ME/MC chroma bandwidth
		Others: Reserved;*/
		uint32_t reg_stat_frm_perf_idx:8;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h110_C;
typedef union {
	struct {
		/*Performance information of current frame;*/
		uint32_t reg_stat_frm_perf_info:32;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h114_C;
typedef union {
	struct {
		/*Current HW CTU X;*/
		uint32_t reg_stat_ctu_x:7;
		uint32_t rsv_7_15:9;
		/*Current HW CTU Y;*/
		uint32_t reg_stat_ctu_y:7;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h120_C;
typedef union {
	struct {
		/*Current HW CU X;*/
		uint32_t reg_stat_cu_x:4;
		/*Current HW CU Y;*/
		uint32_t reg_stat_cu_y:4;
	};
	uint32_t val;
} VC_STAT_VC_STAT_h124_C;
typedef struct {
	volatile VC_STAT_VC_STAT_h00_C VC_STAT_h00;
	volatile VC_STAT_VC_STAT_h10_C VC_STAT_h10;
	volatile VC_STAT_VC_STAT_h14_C VC_STAT_h14;
	volatile VC_STAT_VC_STAT_h18_C VC_STAT_h18;
	volatile VC_STAT_VC_STAT_h1C_C VC_STAT_h1C;
	volatile VC_STAT_VC_STAT_h30_C VC_STAT_h30;
	volatile VC_STAT_VC_STAT_h34_C VC_STAT_h34;
	volatile VC_STAT_VC_STAT_h38_C VC_STAT_h38;
	volatile VC_STAT_VC_STAT_h3C_C VC_STAT_h3C;
	volatile VC_STAT_VC_STAT_h40_C VC_STAT_h40;
	volatile VC_STAT_VC_STAT_h4C_C VC_STAT_h4C;
	volatile VC_STAT_VC_STAT_h50_C VC_STAT_h50;
	volatile VC_STAT_VC_STAT_h54_C VC_STAT_h54;
	volatile VC_STAT_VC_STAT_h58_C VC_STAT_h58;
	volatile VC_STAT_VC_STAT_h5C_C VC_STAT_h5C;
	volatile VC_STAT_VC_STAT_h70_C VC_STAT_h70;
	volatile VC_STAT_VC_STAT_h74_C VC_STAT_h74;
	volatile VC_STAT_VC_STAT_h78_C VC_STAT_h78;
	volatile VC_STAT_VC_STAT_h7C_C VC_STAT_h7C;
	volatile VC_STAT_VC_STAT_h80_C VC_STAT_h80;
	volatile VC_STAT_VC_STAT_h84_C VC_STAT_h84;
	volatile VC_STAT_VC_STAT_h88_C VC_STAT_h88;
	volatile VC_STAT_VC_STAT_h8C_C VC_STAT_h8C;
	volatile VC_STAT_VC_STAT_hA0_C VC_STAT_hA0;
	volatile VC_STAT_VC_STAT_hA4_C VC_STAT_hA4;
	volatile VC_STAT_VC_STAT_h100_C VC_STAT_h100;
	volatile VC_STAT_VC_STAT_h104_C VC_STAT_h104;
	volatile VC_STAT_VC_STAT_h110_C VC_STAT_h110;
	volatile VC_STAT_VC_STAT_h114_C VC_STAT_h114;
	volatile VC_STAT_VC_STAT_h120_C VC_STAT_h120;
	volatile VC_STAT_VC_STAT_h124_C VC_STAT_h124;
} VC_STAT_C;
#define DEFINE_VC_STAT_C(X) \
	 VC_STAT_C X = \
{\
	.VC_STAT_h00.reg_stat_frm_mse_sum = 0x0,\
	.VC_STAT_h10.reg_stat_frm_max_qp = 0x0,\
	.VC_STAT_h10.reg_stat_frm_min_qp = 0x0,\
	.VC_STAT_h14.reg_stat_frm_qp_sum = 0x0,\
	.VC_STAT_h18.reg_stat_frm_qp_hist_go = 0x0,\
	.VC_STAT_h18.reg_stat_frm_qp_hist_idx = 0x0,\
	.VC_STAT_h1C.reg_stat_frm_qp_hist_rd = 0x0,\
	.VC_STAT_h30.reg_stat_frm_madi_sum = 0x0,\
	.VC_STAT_h34.reg_stat_frm_madp_sum = 0x0,\
	.VC_STAT_h38.reg_stat_frm_madi_hist_go = 0x0,\
	.VC_STAT_h38.reg_stat_frm_madi_hist_idx = 0x0,\
	.VC_STAT_h38.reg_stat_frm_madi_hist_rd = 0x0,\
	.VC_STAT_h3C.reg_stat_frm_early_term = 0x0,\
	.VC_STAT_h40.reg_stat_frm_fg_cu16_cnt = 0x0,\
	.VC_STAT_h4C.reg_stat_frm_bso_blen = 0x0,\
	.VC_STAT_h50.reg_stat_frm_hdr_sum = 0x0,\
	.VC_STAT_h54.reg_stat_frm_res_sum = 0x0,\
	.VC_STAT_h58.reg_bit_err_accum = 0x0,\
	.VC_STAT_h5C.reg_blk_qp_accum = 0x0,\
	.VC_STAT_h70.reg_ime_2cand_org_cnt = 0x0, \
	.VC_STAT_h74.reg_ime_2cand_cnt = 0x0, \
	.VC_STAT_h78.reg_ime_2cand_abs_mvdx = 0x0, \
	.VC_STAT_h7C.reg_ime_2cand_abs_mvdy = 0x0, \
	.VC_STAT_h80.reg_stat_frm_sse_go = 0x0,\
	.VC_STAT_h80.reg_stat_frm_sse_rw = 0x0,\
	.VC_STAT_h80.reg_stat_frm_sse_idx = 0x0,\
	.VC_STAT_h80.reg_stat_frm_sse_rtype = 0x0,\
	.VC_STAT_h80.reg_stat_frm_sse_ena = 0x0,\
	.VC_STAT_h84.reg_stat_frm_sse_xst = 0x0,\
	.VC_STAT_h84.reg_stat_frm_sse_xend = 0x0,\
	.VC_STAT_h88.reg_stat_frm_sse_yst = 0x0,\
	.VC_STAT_h88.reg_stat_frm_sse_yend = 0x0,\
	.VC_STAT_h8C.reg_stat_frm_sse_info = 0x0,\
	.VC_STAT_hA0.reg_stat_frm_mc_go = 0x0,\
	.VC_STAT_hA0.reg_stat_frm_mc_idx = 0x0,\
	.VC_STAT_hA4.reg_stat_frm_mc_info = 0x0,\
	.VC_STAT_h100.reg_stat_frm_cu_go = 0x0,\
	.VC_STAT_h100.reg_stat_frm_cu_idx = 0x0,\
	.VC_STAT_h104.reg_stat_frm_cu_info = 0x0,\
	.VC_STAT_h110.reg_stat_frm_perf_go = 0x0,\
	.VC_STAT_h110.reg_stat_frm_perf_idx = 0x0,\
	.VC_STAT_h114.reg_stat_frm_perf_info = 0x0,\
	.VC_STAT_h120.reg_stat_ctu_x = 0x0,\
	.VC_STAT_h120.reg_stat_ctu_y = 0x0,\
	.VC_STAT_h124.reg_stat_cu_x = 0x0,\
	.VC_STAT_h124.reg_stat_cu_y = 0x0,\
};

void init_vc_stat_reg(VC_STAT_C &stat)
{
    stat.VC_STAT_h00.reg_stat_frm_mse_sum = 0x0;
    stat.VC_STAT_h10.reg_stat_frm_max_qp = 0x0;
    stat.VC_STAT_h10.reg_stat_frm_min_qp = 0x0;
    stat.VC_STAT_h14.reg_stat_frm_qp_sum = 0x0;
    stat.VC_STAT_h18.reg_stat_frm_qp_hist_go = 0x0;
    stat.VC_STAT_h18.reg_stat_frm_qp_hist_idx = 0x0;
    stat.VC_STAT_h1C.reg_stat_frm_qp_hist_rd = 0x0;
    stat.VC_STAT_h30.reg_stat_frm_madi_sum = 0x0;
    stat.VC_STAT_h34.reg_stat_frm_madp_sum = 0x0;
    stat.VC_STAT_h38.reg_stat_frm_madi_hist_go = 0x0;
    stat.VC_STAT_h38.reg_stat_frm_madi_hist_idx = 0x0;
    stat.VC_STAT_h38.reg_stat_frm_madi_hist_rd = 0x0;
    stat.VC_STAT_h3C.reg_stat_frm_early_term = 0x0;
    stat.VC_STAT_h40.reg_stat_frm_fg_cu16_cnt = 0x0;
    stat.VC_STAT_h4C.reg_stat_frm_bso_blen = 0x0;
    stat.VC_STAT_h50.reg_stat_frm_hdr_sum = 0x0;
    stat.VC_STAT_h54.reg_stat_frm_res_sum = 0x0;
    stat.VC_STAT_h58.reg_bit_err_accum = 0x0;
    stat.VC_STAT_h5C.reg_blk_qp_accum = 0x0;
    stat.VC_STAT_h80.reg_stat_frm_sse_go = 0x0;
    stat.VC_STAT_h80.reg_stat_frm_sse_rw = 0x0;
    stat.VC_STAT_h80.reg_stat_frm_sse_idx = 0x0;
    stat.VC_STAT_h80.reg_stat_frm_sse_rtype = 0x0;
    stat.VC_STAT_h80.reg_stat_frm_sse_ena = 0x0;
    stat.VC_STAT_h84.reg_stat_frm_sse_xst = 0x0;
    stat.VC_STAT_h84.reg_stat_frm_sse_xend = 0x0;
    stat.VC_STAT_h88.reg_stat_frm_sse_yst = 0x0;
    stat.VC_STAT_h88.reg_stat_frm_sse_yend = 0x0;
    stat.VC_STAT_h8C.reg_stat_frm_sse_info = 0x0;
    stat.VC_STAT_hA0.reg_stat_frm_mc_go = 0x0;
    stat.VC_STAT_hA0.reg_stat_frm_mc_idx = 0x0;
    stat.VC_STAT_hA4.reg_stat_frm_mc_info = 0x0;
    stat.VC_STAT_h100.reg_stat_frm_cu_go = 0x0;
    stat.VC_STAT_h100.reg_stat_frm_cu_idx = 0x0;
    stat.VC_STAT_h104.reg_stat_frm_cu_info = 0x0;
    stat.VC_STAT_h110.reg_stat_frm_perf_go = 0x0;
    stat.VC_STAT_h110.reg_stat_frm_perf_idx = 0x0;
    stat.VC_STAT_h114.reg_stat_frm_perf_info = 0x0;
    stat.VC_STAT_h120.reg_stat_ctu_x = 0x0;
    stat.VC_STAT_h120.reg_stat_ctu_y = 0x0;
    stat.VC_STAT_h124.reg_stat_cu_x = 0x0;
    stat.VC_STAT_h124.reg_stat_cu_y = 0x0;
};
#endif //__REG_VC_STAT_STRUCT_H__
