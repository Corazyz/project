// $Module: vc_sys $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Tue, 22 Jun 2021 02:48:53 PM $
//

#ifndef __REG_VC_SYS_STRUCT_H__
#define __REG_VC_SYS_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		uint32_t reg_vc_sw_rst_core:1;
		uint32_t reg_vc_sw_rst_fab:1;
		uint32_t reg_vc_sw_rst_sys:1;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h00_C;
typedef union {
	struct {
		uint32_t reg_vc_sw_rst_core_done:1;
		uint32_t reg_vc_sw_rst_fab_done:1;
		uint32_t reg_vc_sw_rst_sys_done:1;
		uint32_t rsv_3_29:27;
		uint32_t reg_vc_slice_done:1;
		uint32_t reg_vc_pic_done:1;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h04_C;
typedef union {
	struct {
		uint32_t reg_vc_int_en:16; //14:BS ring buffer full
		uint32_t rsv_16_30:15;
		uint32_t reg_vc_int_clr:1;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h10_C;
typedef union {
	struct {
		uint32_t reg_vc_int_vec:32;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h14_C;
typedef union {
	struct {
		uint32_t reg_vc_cac_en:1;
		uint32_t reg_vc_cg_en:1;
		uint32_t reg_vc_cg_larb_en:1;
		uint32_t rsv_3_15:13;
		uint32_t reg_vc_cg_inter_en:1;
		uint32_t reg_vc_cg_intra_en:1;
		uint32_t reg_vc_cg_cavlc_en:1;
		uint32_t reg_vc_cg_cabac_en:1;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h20_C;
typedef union {
	struct {
		uint32_t reg_vc_cg_sip_en[2];
	};
	uint32_t val[2];
} VC_SYS_VC_SYS_h24_C;
typedef union {
	struct {
		uint32_t reg_vc_cg_sip_status:32;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h2C_C;
typedef union {
	struct {
		uint32_t reg_vc_ck_wakeup:1;
		uint32_t rsv_1_7:7;
		uint32_t reg_vc_ck_status:24;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h40_C;
typedef union {
	struct {
		uint32_t reg_vc_cac_mode:1;
		uint32_t reg_vc_cac_sw_mode:3;
		uint32_t reg_vc_cac_sw_upd:1;
		uint32_t rsv_5_15:11;
		uint32_t reg_vc_cac_sw_div:4;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h80_C;
typedef union {
	struct {
		uint32_t reg_vc_cac_hw_gear0:20;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h84_C;
typedef union {
	struct {
		uint32_t reg_vc_cac_hw_gear1:20;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h88_C;
typedef union {
	struct {
		uint32_t reg_vc_cac_hw_gear2:20;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h8C_C;
typedef union {
	struct {
		uint32_t reg_vc_cac_hw_gear3:20;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h90_C;
typedef union {
	struct {
		uint32_t reg_vc_cac_hw_gear4:20;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h94_C;
typedef union {
	struct {
		uint32_t reg_vc_cc_inval_go:1;
		uint32_t reg_vc_cc_sw_rst:1;
		uint32_t rsv_2_3:2;
		uint32_t reg_vc_cc_status_clr:1;
		uint32_t rsv_5_15:11;
		uint32_t reg_vc_cc_inv_done:16;
	};
	uint32_t val;
} VC_SYS_VC_SYS_hA0_C;
typedef union {
	struct {
		uint32_t reg_vc_ccu_dbg_sel:8;
		uint32_t reg_vc_irpu_dbg_sel:8;
		uint32_t reg_vc_iapu_dbg_sel:8;
		uint32_t reg_vc_rru_dbg_sel:8;
	};
	uint32_t val;
} VC_SYS_VC_SYS_hC0_C;
typedef union {
	struct {
		uint32_t reg_vc_ppu_dbg_sel:8;
		uint32_t reg_vc_pru_dbg_sel:8;
		uint32_t rsv_16_23:8;
		uint32_t reg_vc_top_dbg_sel:8;
	};
	uint32_t val;
} VC_SYS_VC_SYS_hC4_C;
typedef union {
	struct {
		uint32_t reg_vc_dbg_out_sel:8;
	};
	uint32_t val;
} VC_SYS_VC_SYS_hD0_C;
typedef union {
	struct {
		uint32_t reg_vc_dbg_out:32;
	};
	uint32_t val;
} VC_SYS_VC_SYS_hD4_C;
typedef union {
	struct {
		uint32_t reg_vc_axi_rid0:4;
		uint32_t reg_vc_axi_wid0:4;
		uint32_t reg_vc_axi_rid1:4;
		uint32_t reg_vc_axi_wid1:4;
		uint32_t reg_vc_axi_rid2:4;
		uint32_t reg_vc_axi_wid2:4;
		uint32_t reg_vc_axi_rid3:4;
		uint32_t reg_vc_axi_wid3:4;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h100_C;
typedef union {
	struct {
		uint32_t reg_vc_axi_wc0_map:4;
		uint32_t reg_vc_axi_wc1_map:4;
		uint32_t reg_vc_axi_wc2_map:4;
		uint32_t reg_vc_axi_wc3_map:4;
		uint32_t reg_vc_axi_wc4_map:4;
		uint32_t reg_vc_axi_wc5_map:4;
		uint32_t reg_vc_axi_wc6_map:4;
		uint32_t reg_vc_axi_wc7_map:4;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h104_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc0_lbnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h110_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc1_lbnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h118_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc2_lbnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h120_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc3_lbnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h128_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc4_lbnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h130_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc5_lbnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h138_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc6_lbnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h140_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc7_lbnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h148_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc0_ubnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h1A0_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc1_ubnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h1A8_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc2_ubnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h1B0_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc3_ubnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h1B8_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc4_ubnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h1C0_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc5_ubnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h1C8_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc6_ubnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h1D0_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		uint32_t reg_vc_axi_wc7_ubnd:28;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h1D8_C;
typedef union {
	struct {
		uint32_t reg_vc_axi_rc0_map:4;
		uint32_t reg_vc_axi_rc1_map:4;
		uint32_t reg_vc_axi_rc2_map:4;
		uint32_t reg_vc_axi_rc3_map:4;
		uint32_t reg_vc_axi_rc4_map:4;
		uint32_t reg_vc_axi_rc5_map:4;
		uint32_t reg_vc_axi_rc6_map:4;
		uint32_t reg_vc_axi_rc7_map:4;
	};
	uint32_t val;
} VC_SYS_VC_SYS_h240_C;
typedef struct {
	volatile VC_SYS_VC_SYS_h00_C VC_SYS_h00;
	volatile VC_SYS_VC_SYS_h04_C VC_SYS_h04;
	volatile VC_SYS_VC_SYS_h10_C VC_SYS_h10;
	volatile VC_SYS_VC_SYS_h14_C VC_SYS_h14;
	volatile VC_SYS_VC_SYS_h20_C VC_SYS_h20;
	volatile VC_SYS_VC_SYS_h24_C VC_SYS_h24;
	volatile VC_SYS_VC_SYS_h2C_C VC_SYS_h2C;
	volatile VC_SYS_VC_SYS_h40_C VC_SYS_h40;
	volatile VC_SYS_VC_SYS_h80_C VC_SYS_h80;
	volatile VC_SYS_VC_SYS_h84_C VC_SYS_h84;
	volatile VC_SYS_VC_SYS_h88_C VC_SYS_h88;
	volatile VC_SYS_VC_SYS_h8C_C VC_SYS_h8C;
	volatile VC_SYS_VC_SYS_h90_C VC_SYS_h90;
	volatile VC_SYS_VC_SYS_h94_C VC_SYS_h94;
	volatile VC_SYS_VC_SYS_hA0_C VC_SYS_hA0;
	volatile VC_SYS_VC_SYS_hC0_C VC_SYS_hC0;
	volatile VC_SYS_VC_SYS_hC4_C VC_SYS_hC4;
	volatile VC_SYS_VC_SYS_hD0_C VC_SYS_hD0;
	volatile VC_SYS_VC_SYS_hD4_C VC_SYS_hD4;
	volatile VC_SYS_VC_SYS_h100_C VC_SYS_h100;
	volatile VC_SYS_VC_SYS_h104_C VC_SYS_h104;
	volatile VC_SYS_VC_SYS_h110_C VC_SYS_h110;
	volatile VC_SYS_VC_SYS_h118_C VC_SYS_h118;
	volatile VC_SYS_VC_SYS_h120_C VC_SYS_h120;
	volatile VC_SYS_VC_SYS_h128_C VC_SYS_h128;
	volatile VC_SYS_VC_SYS_h130_C VC_SYS_h130;
	volatile VC_SYS_VC_SYS_h138_C VC_SYS_h138;
	volatile VC_SYS_VC_SYS_h140_C VC_SYS_h140;
	volatile VC_SYS_VC_SYS_h148_C VC_SYS_h148;
	volatile VC_SYS_VC_SYS_h1A0_C VC_SYS_h1A0;
	volatile VC_SYS_VC_SYS_h1A8_C VC_SYS_h1A8;
	volatile VC_SYS_VC_SYS_h1B0_C VC_SYS_h1B0;
	volatile VC_SYS_VC_SYS_h1B8_C VC_SYS_h1B8;
	volatile VC_SYS_VC_SYS_h1C0_C VC_SYS_h1C0;
	volatile VC_SYS_VC_SYS_h1C8_C VC_SYS_h1C8;
	volatile VC_SYS_VC_SYS_h1D0_C VC_SYS_h1D0;
	volatile VC_SYS_VC_SYS_h1D8_C VC_SYS_h1D8;
	volatile VC_SYS_VC_SYS_h240_C VC_SYS_h240;
} VC_SYS_C;
#define DEFINE_VC_SYS_C(X) \
	 VC_SYS_C X = \
{\
	.VC_SYS_h00.reg_vc_sw_rst_core = 0x0,\
	.VC_SYS_h00.reg_vc_sw_rst_fab = 0x0,\
	.VC_SYS_h00.reg_vc_sw_rst_sys = 0x0,\
	.VC_SYS_h04.reg_vc_sw_rst_core_done = 0x0,\
	.VC_SYS_h04.reg_vc_sw_rst_fab_done = 0x0,\
	.VC_SYS_h04.reg_vc_sw_rst_sys_done = 0x0,\
	.VC_SYS_h04.reg_vc_slice_done = 0x0,\
	.VC_SYS_h04.reg_vc_pic_done = 0x0,\
	.VC_SYS_h10.reg_vc_int_en = 0x0,\
	.VC_SYS_h10.reg_vc_int_clr = 0x0,\
	.VC_SYS_h14.reg_vc_int_vec = 0x0,\
	.VC_SYS_h20.reg_vc_cac_en = 0x0,\
	.VC_SYS_h20.reg_vc_cg_en = 0x1,\
	.VC_SYS_h20.reg_vc_cg_larb_en = 0x1,\
	.VC_SYS_h20.reg_vc_cg_inter_en = 0x1,\
	.VC_SYS_h20.reg_vc_cg_intra_en = 0x1,\
	.VC_SYS_h20.reg_vc_cg_cavlc_en = 0x0,\
	.VC_SYS_h20.reg_vc_cg_cabac_en = 0x0,\
	.VC_SYS_h24.reg_vc_cg_sip_en = 0x0,\
	.VC_SYS_h2C.reg_vc_cg_sip_status = 0x0,\
	.VC_SYS_h40.reg_vc_ck_wakeup = 0x0,\
	.VC_SYS_h40.reg_vc_ck_status = 0x0,\
	.VC_SYS_h80.reg_vc_cac_mode = 0x0,\
	.VC_SYS_h80.reg_vc_cac_sw_mode = 0x7,\
	.VC_SYS_h80.reg_vc_cac_sw_upd = 0x0,\
	.VC_SYS_h80.reg_vc_cac_sw_div = 0xF,\
	.VC_SYS_h84.reg_vc_cac_hw_gear0 = 0x0,\
	.VC_SYS_h88.reg_vc_cac_hw_gear1 = 0x0,\
	.VC_SYS_h8C.reg_vc_cac_hw_gear2 = 0x0,\
	.VC_SYS_h90.reg_vc_cac_hw_gear3 = 0x0,\
	.VC_SYS_h94.reg_vc_cac_hw_gear4 = 0x0,\
	.VC_SYS_hA0.reg_vc_cc_inval_go = 0x0,\
	.VC_SYS_hA0.reg_vc_cc_sw_rst = 0x0,\
	.VC_SYS_hA0.reg_vc_cc_status_clr = 0x0,\
	.VC_SYS_hA0.reg_vc_cc_inv_done = 0x0,\
	.VC_SYS_hC0.reg_vc_ccu_dbg_sel = 0x0,\
	.VC_SYS_hC0.reg_vc_irpu_dbg_sel = 0x0,\
	.VC_SYS_hC0.reg_vc_iapu_dbg_sel = 0x0,\
	.VC_SYS_hC0.reg_vc_rru_dbg_sel = 0x0,\
	.VC_SYS_hC4.reg_vc_ppu_dbg_sel = 0x0,\
	.VC_SYS_hC4.reg_vc_pru_dbg_sel = 0x0,\
	.VC_SYS_hC4.reg_vc_top_dbg_sel = 0x0,\
	.VC_SYS_hD0.reg_vc_dbg_out_sel = 0x0,\
	.VC_SYS_hD4.reg_vc_dbg_out = 0x0,\
	.VC_SYS_h100.reg_vc_axi_rid0 = 0x0,\
	.VC_SYS_h100.reg_vc_axi_wid0 = 0x0,\
	.VC_SYS_h100.reg_vc_axi_rid1 = 0x0,\
	.VC_SYS_h100.reg_vc_axi_wid1 = 0x0,\
	.VC_SYS_h100.reg_vc_axi_rid2 = 0x0,\
	.VC_SYS_h100.reg_vc_axi_wid2 = 0x0,\
	.VC_SYS_h100.reg_vc_axi_rid3 = 0x0,\
	.VC_SYS_h100.reg_vc_axi_wid3 = 0x0,\
	.VC_SYS_h104.reg_vc_axi_wc0_map = 0x1,\
	.VC_SYS_h104.reg_vc_axi_wc1_map = 0x1,\
	.VC_SYS_h104.reg_vc_axi_wc2_map = 0x1,\
	.VC_SYS_h104.reg_vc_axi_wc3_map = 0x1,\
	.VC_SYS_h104.reg_vc_axi_wc4_map = 0x1,\
	.VC_SYS_h104.reg_vc_axi_wc5_map = 0x1,\
	.VC_SYS_h104.reg_vc_axi_wc6_map = 0x1,\
	.VC_SYS_h104.reg_vc_axi_wc7_map = 0x1,\
	.VC_SYS_h110.reg_vc_axi_wc0_lbnd = 0x0,\
	.VC_SYS_h118.reg_vc_axi_wc1_lbnd = 0x0,\
	.VC_SYS_h120.reg_vc_axi_wc2_lbnd = 0x0,\
	.VC_SYS_h128.reg_vc_axi_wc3_lbnd = 0x0,\
	.VC_SYS_h130.reg_vc_axi_wc4_lbnd = 0x0,\
	.VC_SYS_h138.reg_vc_axi_wc5_lbnd = 0x0,\
	.VC_SYS_h140.reg_vc_axi_wc6_lbnd = 0x0,\
	.VC_SYS_h148.reg_vc_axi_wc7_lbnd = 0x0,\
	.VC_SYS_h1A0.reg_vc_axi_wc0_ubnd = 0xFFFFFFF,\
	.VC_SYS_h1A8.reg_vc_axi_wc1_ubnd = 0xFFFFFFF,\
	.VC_SYS_h1B0.reg_vc_axi_wc2_ubnd = 0xFFFFFFF,\
	.VC_SYS_h1B8.reg_vc_axi_wc3_ubnd = 0xFFFFFFF,\
	.VC_SYS_h1C0.reg_vc_axi_wc4_ubnd = 0xFFFFFFF,\
	.VC_SYS_h1C8.reg_vc_axi_wc5_ubnd = 0xFFFFFFF,\
	.VC_SYS_h1D0.reg_vc_axi_wc6_ubnd = 0xFFFFFFF,\
	.VC_SYS_h1D8.reg_vc_axi_wc7_ubnd = 0xFFFFFFF,\
	.VC_SYS_h240.reg_vc_axi_rc0_map = 0x1,\
	.VC_SYS_h240.reg_vc_axi_rc1_map = 0x1,\
	.VC_SYS_h240.reg_vc_axi_rc2_map = 0x1,\
	.VC_SYS_h240.reg_vc_axi_rc3_map = 0x1,\
	.VC_SYS_h240.reg_vc_axi_rc4_map = 0x1,\
	.VC_SYS_h240.reg_vc_axi_rc5_map = 0x1,\
	.VC_SYS_h240.reg_vc_axi_rc6_map = 0x1,\
	.VC_SYS_h240.reg_vc_axi_rc7_map = 0x1,\
};

void init_vc_sys_reg(VC_SYS_C &sys)
{
    sys.VC_SYS_h00.reg_vc_sw_rst_core = 0x0;
    sys.VC_SYS_h00.reg_vc_sw_rst_fab = 0x0;
    sys.VC_SYS_h00.reg_vc_sw_rst_sys = 0x0;
    sys.VC_SYS_h04.reg_vc_sw_rst_core_done = 0x0;
    sys.VC_SYS_h04.reg_vc_sw_rst_fab_done = 0x0;
    sys.VC_SYS_h04.reg_vc_sw_rst_sys_done = 0x0;
    sys.VC_SYS_h04.reg_vc_slice_done = 0x0;
    sys.VC_SYS_h04.reg_vc_pic_done = 0x0;
    sys.VC_SYS_h10.reg_vc_int_en = 0x0;
    sys.VC_SYS_h10.reg_vc_int_clr = 0x0;
    sys.VC_SYS_h14.reg_vc_int_vec = 0x0;
    sys.VC_SYS_h20.reg_vc_cac_en = 0x0;
    sys.VC_SYS_h20.reg_vc_cg_en = 0x1;
    sys.VC_SYS_h20.reg_vc_cg_larb_en = 0x1;
    sys.VC_SYS_h20.reg_vc_cg_inter_en = 0x1;
    sys.VC_SYS_h20.reg_vc_cg_intra_en = 0x1;
    sys.VC_SYS_h20.reg_vc_cg_cavlc_en = 0x0;
    sys.VC_SYS_h20.reg_vc_cg_cabac_en = 0x0;
    sys.VC_SYS_h24.reg_vc_cg_sip_en[0] = 0x0;
	sys.VC_SYS_h24.reg_vc_cg_sip_en[1] = 0x0;
    sys.VC_SYS_h2C.reg_vc_cg_sip_status = 0x0;
    sys.VC_SYS_h40.reg_vc_ck_wakeup = 0x0;
    sys.VC_SYS_h40.reg_vc_ck_status = 0x0;
    sys.VC_SYS_h80.reg_vc_cac_mode = 0x0;
    sys.VC_SYS_h80.reg_vc_cac_sw_mode = 0x7;
    sys.VC_SYS_h80.reg_vc_cac_sw_upd = 0x0;
    sys.VC_SYS_h80.reg_vc_cac_sw_div = 0xF;
    sys.VC_SYS_h84.reg_vc_cac_hw_gear0 = 0x0;
    sys.VC_SYS_h88.reg_vc_cac_hw_gear1 = 0x0;
    sys.VC_SYS_h8C.reg_vc_cac_hw_gear2 = 0x0;
    sys.VC_SYS_h90.reg_vc_cac_hw_gear3 = 0x0;
    sys.VC_SYS_h94.reg_vc_cac_hw_gear4 = 0x0;
    sys.VC_SYS_hA0.reg_vc_cc_inval_go = 0x0;
    sys.VC_SYS_hA0.reg_vc_cc_sw_rst = 0x0;
    sys.VC_SYS_hA0.reg_vc_cc_status_clr = 0x0;
    sys.VC_SYS_hA0.reg_vc_cc_inv_done = 0x0;
    sys.VC_SYS_hC0.reg_vc_ccu_dbg_sel = 0x0;
    sys.VC_SYS_hC0.reg_vc_irpu_dbg_sel = 0x0;
    sys.VC_SYS_hC0.reg_vc_iapu_dbg_sel = 0x0;
    sys.VC_SYS_hC0.reg_vc_rru_dbg_sel = 0x0;
    sys.VC_SYS_hC4.reg_vc_ppu_dbg_sel = 0x0;
    sys.VC_SYS_hC4.reg_vc_pru_dbg_sel = 0x0;
    sys.VC_SYS_hC4.reg_vc_top_dbg_sel = 0x0;
    sys.VC_SYS_hD0.reg_vc_dbg_out_sel = 0x0;
    sys.VC_SYS_hD4.reg_vc_dbg_out = 0x0;
    sys.VC_SYS_h100.reg_vc_axi_rid0 = 0x0;
    sys.VC_SYS_h100.reg_vc_axi_wid0 = 0x0;
    sys.VC_SYS_h100.reg_vc_axi_rid1 = 0x0;
    sys.VC_SYS_h100.reg_vc_axi_wid1 = 0x0;
    sys.VC_SYS_h100.reg_vc_axi_rid2 = 0x0;
    sys.VC_SYS_h100.reg_vc_axi_wid2 = 0x0;
    sys.VC_SYS_h100.reg_vc_axi_rid3 = 0x0;
    sys.VC_SYS_h100.reg_vc_axi_wid3 = 0x0;
    sys.VC_SYS_h104.reg_vc_axi_wc0_map = 0x1;
    sys.VC_SYS_h104.reg_vc_axi_wc1_map = 0x1;
    sys.VC_SYS_h104.reg_vc_axi_wc2_map = 0x1;
    sys.VC_SYS_h104.reg_vc_axi_wc3_map = 0x1;
    sys.VC_SYS_h104.reg_vc_axi_wc4_map = 0x1;
    sys.VC_SYS_h104.reg_vc_axi_wc5_map = 0x1;
    sys.VC_SYS_h104.reg_vc_axi_wc6_map = 0x1;
    sys.VC_SYS_h104.reg_vc_axi_wc7_map = 0x1;
    sys.VC_SYS_h110.reg_vc_axi_wc0_lbnd = 0x0;
    sys.VC_SYS_h118.reg_vc_axi_wc1_lbnd = 0x0;
    sys.VC_SYS_h120.reg_vc_axi_wc2_lbnd = 0x0;
    sys.VC_SYS_h128.reg_vc_axi_wc3_lbnd = 0x0;
    sys.VC_SYS_h130.reg_vc_axi_wc4_lbnd = 0x0;
    sys.VC_SYS_h138.reg_vc_axi_wc5_lbnd = 0x0;
    sys.VC_SYS_h140.reg_vc_axi_wc6_lbnd = 0x0;
    sys.VC_SYS_h148.reg_vc_axi_wc7_lbnd = 0x0;
    sys.VC_SYS_h1A0.reg_vc_axi_wc0_ubnd = 0xFFFFFFF;
    sys.VC_SYS_h1A8.reg_vc_axi_wc1_ubnd = 0xFFFFFFF;
    sys.VC_SYS_h1B0.reg_vc_axi_wc2_ubnd = 0xFFFFFFF;
    sys.VC_SYS_h1B8.reg_vc_axi_wc3_ubnd = 0xFFFFFFF;
    sys.VC_SYS_h1C0.reg_vc_axi_wc4_ubnd = 0xFFFFFFF;
    sys.VC_SYS_h1C8.reg_vc_axi_wc5_ubnd = 0xFFFFFFF;
    sys.VC_SYS_h1D0.reg_vc_axi_wc6_ubnd = 0xFFFFFFF;
    sys.VC_SYS_h1D8.reg_vc_axi_wc7_ubnd = 0xFFFFFFF;
    sys.VC_SYS_h240.reg_vc_axi_rc0_map = 0x1;
    sys.VC_SYS_h240.reg_vc_axi_rc1_map = 0x1;
    sys.VC_SYS_h240.reg_vc_axi_rc2_map = 0x1;
    sys.VC_SYS_h240.reg_vc_axi_rc3_map = 0x1;
    sys.VC_SYS_h240.reg_vc_axi_rc4_map = 0x1;
    sys.VC_SYS_h240.reg_vc_axi_rc5_map = 0x1;
    sys.VC_SYS_h240.reg_vc_axi_rc6_map = 0x1;
    sys.VC_SYS_h240.reg_vc_axi_rc7_map = 0x1;
};
#endif //__REG_VC_SYS_STRUCT_H__
