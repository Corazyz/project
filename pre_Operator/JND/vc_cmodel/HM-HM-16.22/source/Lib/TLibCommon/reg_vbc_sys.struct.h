
#ifndef __REG_VBC_SYS_STRUCT_H__
#define __REG_VBC_SYS_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*VBC SW Reset Core engine;*/
		uint32_t reg_vbc_sw_rst_core:1;
		uint32_t rsv_1_3:3;
		/*VBC frame start*/
		uint32_t reg_frame_start:1;
		uint32_t rsv_5_15:11;
		/*VBC SW Reset Core engine done*/
		uint32_t reg_vbc_sw_rst_core_done:1;
		uint32_t rsv_17_30:14;
		/*VBC Picture Done*/
		uint32_t reg_vbc_pic_done:1;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h00_C;
typedef union {
	struct {
		/*
		"VBC HW status
		0 : vbd exception event
		1 : vbe exceptioin event
		2: vbe pic done
		3: Clock is disable"
		*/
		uint32_t reg_vbc_hw_status:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h04_C;
typedef union {
	struct {
		/*VBC interrupt enable*/
		uint32_t reg_vbc_int_en:8;
		uint32_t rsv_8_14:7;
		/*VBC interrupt clear*/
		uint32_t reg_vbc_int_clr:1;
		/*
		"VBC interrupt vector
		0: vbd exception event
		1: vbe exception event
		2: vbe pic done
		3: Disable Clock"
		*/
		uint32_t reg_vbc_int_vec:16;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h08_C;
typedef union {
	struct {
		/* VBC clock gating enable for core engine */
		uint32_t reg_vbc_cg_en:1;
		/*VBC clock gating enable for local arbiter*/
		uint32_t reg_vbc_cg_larb_en:1;
		/*VBC clock  enable for VBD*/
		uint32_t reg_vbc_cg_vbd_en:1;
		uint32_t rsv_3_15:13;
		/*VBC clock wakeup*/
		uint32_t reg_vbc_ck_wakeup:1;
		uint32_t rsv_17_23:7;
		/*VBC clock status*/
		uint32_t reg_vbc_ck_status:8;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h0C_C;
typedef union {
	struct {
		/* Cache invalidate go */
		uint32_t reg_vbc_cc_inval_go:1;
		/*Cache SW reset*/
		uint32_t reg_vbc_cc_sw_rst:1;
		uint32_t rsv_2_3:2;
		/*Cache status clear*/
		uint32_t reg_vbc_cc_status_clr:1;
		uint32_t rsv_5_15:11;
		/*Cache invalidate done*/
		uint32_t reg_vbc_cc_inv_done:16;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h10_C;
typedef union {
	struct {
		/* VBE debug select */
		uint32_t reg_vbc_vbe_dbg_sel:8;
		/*VBD debug select */
		uint32_t reg_vbc_vbd_dbg_sel:8;
		/*
		"VBC AXI BW monitor status select
		0: wr counts of AXI WCH0
		1: wr latency of AXI WCH0
		2: rd counts of AXI RCH0
		3: rd latency of AXI RCH0
		4: wr counts of AXI WCH1
		5: wr latency of AXI WCH1
		6: rd counts of AXI RCH1
		7: rd latency of AXI RCH1"
		*/
		uint32_t reg_vbc_axi_mon_dgb_sel:8;
		/*TOP debug select*/
		uint32_t reg_vbc_top_dbg_sel:8;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h14_C;
typedef union {
	struct {
		/*
		"VBC debug output select
		0: VBE
		1: VBD
		2: AXI BW monitor
		255: TOP"
		*/
		uint32_t reg_vbc_dbg_out_sel:8;
		uint32_t rsv_8_30:23;
		/*VBC debug output go*/
		uint32_t reg_vbc_dbg_out_go:1;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h18_C;
typedef union {
	struct {
		/*VBC debug output*/
		uint32_t reg_vbc_dbg_out:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h1C_C;
typedef union {
	struct {
		/*VBC spare Registers*/
		uint32_t reg_vbc_spare:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h20_C;
typedef union {
	struct {
		/*VBC spare outputs*/
		uint32_t reg_vbc_spare_out:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h24_C;
typedef union {
	struct {
		/*VBC AXI0 Read ID*/
		uint32_t reg_vbc_axi_rid0:4;
		/*VBC AXI0 Write ID*/
		uint32_t reg_vbc_axi_wid0:4;
		/*VBC AXI1 Read ID*/
		uint32_t reg_vbc_axi_rid1:4;
		/*VBC AXI1 Write ID*/
		uint32_t reg_vbc_axi_wid1:4;
		uint32_t rsv_16_31:16;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h28_C;
typedef union {
	struct {
		/*
		"VBC AXI WR Client0 MAP
		b0: AXI master 0
		b1: AXI master 1
		b2: AXI master 2
		b3: AXI master 3"
		*/
		uint32_t reg_vbc_axi_wc0_map:4;
		/*VBC AXI WR Client1 MAP*/
		uint32_t reg_vbc_axi_wc1_map:4;
		/*VBC AXI WR Client2 MAP*/
		uint32_t reg_vbc_axi_wc2_map:4;
		uint32_t rsv_12_15:4;
		/*
		"VBC AXI RD Client0 MAP
		b0: AXI master 0
		b1: AXI master 1
		b2: AXI master 2
		b3: AXI master 3"
		*/
		uint32_t reg_vbc_axi_rc0_map:4;
		/*VBC AXI RD Client1 MAP*/
		uint32_t reg_vbc_axi_rc1_map:4;
		/*VBC AXI RD Client2 MAP*/
		uint32_t reg_vbc_axi_rc2_map:4;
		uint32_t rsv_28_31:4;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h2C_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*VBC AXI WR Client0 low bound address*/
		uint32_t reg_vbc_axi_wc0_lbnd_lsb:28;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h30_C;
typedef union {
	struct {
		/*VBC AXI WR Client0 low bound address*/
		uint32_t reg_vbc_axi_wc0_lbnd_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h34_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*VBC AXI WR Client1 low bound address*/
		uint32_t reg_vbc_axi_wc1_lbnd_lsb:28;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h38_C;
typedef union {
	struct {
		/*VBC AXI WR Client1 low bound address*/
		uint32_t reg_vbc_axi_wc1_lbnd_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h3C_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*VBC AXI WR Client2 low bound address*/
		uint32_t reg_vbc_axi_wc2_lbnd_lsb:28;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h40_C;
typedef union {
	struct {
		/*VBC AXI WR Client2 low bound address*/
		uint32_t reg_vbc_axi_wc2_lbnd_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h44_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*VBC AXI WR Client0 upper bound address*/
		uint32_t reg_vbc_axi_wc0_ubnd_lsb:28;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h48_C;
typedef union {
	struct {
		/*VBC AXI WR Client0 upper bound address*/
		uint32_t reg_vbc_axi_wc0_ubnd_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h4C_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*VBC AXI WR Client1 upper bound address*/
		uint32_t reg_vbc_axi_wc1_ubnd_lsb:28;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h50_C;
typedef union {
	struct {
		/*VBC AXI WR Client1 upper bound address*/
		uint32_t reg_vbc_axi_wc1_ubnd_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h54_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*VBC AXI WR Client2 upper bound address*/
		uint32_t reg_vbc_axi_wc2_ubnd_lsb:28;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h58_C;
typedef union {
	struct {
		/*VBC AXI WR Client2 upper bound address*/
		uint32_t reg_vbc_axi_wc2_ubnd_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h5C_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*VBC Encoder Meta base address (1KB)*/
		uint32_t reg_vbc_meta_base_lsb:22;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h60_C;
typedef union {
	struct {
		/*VBC Encoder Meta base address (1KB)*/
		uint32_t reg_vbc_meta_base_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h64_C;
typedef union {
	struct {
		uint32_t rsv_0_4:5;
		/*VBC Encoder Meta pitch (32B)*/
		uint32_t reg_vbc_meta_pit:10;
		uint32_t rsv_15_31:17;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h68_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*VBC Encoder Luma base address (1KB)*/
		uint32_t reg_vbc_luma_base_lsb:22;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h70_C;
typedef union {
	struct {
		/*VBC Encoder Luma base address (1KB)*/
		uint32_t reg_vbc_luma_base_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h74_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*VBC Encoder Chroma base address (1KB)*/
		uint32_t reg_vbc_chroma_base_lsb:22;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h78_C;
typedef union {
	struct {
		/*VBC Encoder Chroma base address (1KB)*/
		uint32_t reg_vbc_chroma_base_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h7C_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*VBC Decoder Meta base address (1KB)*/
		uint32_t reg_vbd_meta_base_lsb:22;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h80_C;
typedef union {
	struct {
		/*VBC Decoder Meta base address (1KB)*/
		uint32_t reg_vbd_meta_base_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h84_C;
typedef union {
	struct {
		uint32_t rsv_0_4:5;
		/*VBC Decoder Meta pitch (32B)*/
		uint32_t reg_vbd_meta_pit:10;
		uint32_t rsv_15_31:17;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h88_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*VBC Decoder Luma base address (1KB)*/
		uint32_t reg_vbd_luma_base_lsb:22;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h90_C;
typedef union {
	struct {
		/*VBC Decoder Luma base address (1KB)*/
		uint32_t reg_vbd_luma_base_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h94_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*VBC Decoder Chroma base address (1KB)*/
		uint32_t reg_vbd_chroma_base_lsb:22;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h98_C;
typedef union {
	struct {
		/*VBC Decoder Chroma base address (1KB)*/
		uint32_t reg_vbd_chroma_base_msb:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_h9C_C;
typedef union {
	struct {
		/*VBC lossy mode enable*/
		uint32_t reg_vbc_lossy_en:1;
		uint32_t rsv_1_3:3;
		/*
		"VBC lossy mode truncate bits
		0: 1bit
		1: 2bits
		2: 3bits
		3: 4bits"
		*/
		uint32_t reg_vbc_lossy_tb:2;
		uint32_t rsv_6_7:2;
		/*VBC lossy EU(32x4) target bits*/
		uint32_t reg_vbc_lossy_rc_tb:8;
		uint32_t rsv_16_30:15;
		/*Clear lossy counter*/
		uint32_t reg_vbc_lossy_cnt_clr:8;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_hA0_C;
typedef union {
	struct {
		uint32_t rsv_0_1:2;
		/*VBC RC goto lossy mode threshold*/
		uint32_t reg_vbc_rc_goto_thr:5;
		uint32_t rsv_7:1;
		/*VBC RC exit lossy mode threshold*/
		uint32_t reg_vbc_rc_exit_thr:6;
		uint32_t rsv_14_29:16;
		/*
		"VBC RC mode chane delay in EU unit
		0: 1
		1: 2
		2: 3
		3: 4"
		*/
		uint32_t reg_vbc_lossy_delay:2;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_hA4_C;
typedef union {
	struct {
		/*VBC lossy cu counts*/
		uint32_t reg_vbc_lossy_cu_cnt:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_hA8_C;
typedef union {
	struct {
		/*VBC lossy accumulated error*/
		uint32_t reg_vbc_lossy_acc_err:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_hAC_C;
typedef union {
	struct {
		/*Picture width in pixel minus 1*/
		uint32_t reg_pic_width_m1:13;
		uint32_t rsv_13_15:3;
		/*Picture height in pixel minus 1*/
		uint32_t reg_pic_height_m1:13;
		uint32_t rsv_29_31:3;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_hB0_C;

typedef union {
	struct {
		/* 0: HEVC mode: VBE 64 lines stream packing
		1: AVC mode: VBE 16 lines stream packing */
		uint32_t reg_avc_mode:1;
		/* VENC deblocking is disable */
		uint32_t reg_vbe_ve_db_dis:1;
		uint32_t reg_vbd_ve_db_dis:1;
		uint32_t rsv_3_3:1;
		/* VBC luma plane y coordinate offset line setting
		0: 0 line
		1: 4 lines
		2: 8 lines
		3: 12 lines */
		uint32_t reg_wbc_luma_y_offset:2;
		/* VBC chroma plane y coordinate offset line setting
		0: 0 line
		1: 4 lines
		2: 8 lines
		3: 12 lines */
		uint32_t reg_wbc_chroma_y_offset:2;
		uint32_t rsv_8_31:24;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_hB4_C;

typedef union {
	struct {
		/*Luma compressed buffer size, in byte unit*/
		uint32_t reg_luma_cpx_sz:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_hC0_C;

typedef union {
	struct {
		/*Chroma compressed buffer size, in byte unit*/
		uint32_t reg_chroma_cpx_sz:32;
	};
	uint32_t val;
} VBC_SYS_VBC_SYS_hC4_C;

typedef struct {
	volatile VBC_SYS_VBC_SYS_h00_C VBC_SYS_h00;
	volatile VBC_SYS_VBC_SYS_h04_C VBC_SYS_h04;
	volatile VBC_SYS_VBC_SYS_h08_C VBC_SYS_h08;
	volatile VBC_SYS_VBC_SYS_h0C_C VBC_SYS_h0C;
	volatile VBC_SYS_VBC_SYS_h10_C VBC_SYS_h10;
	volatile VBC_SYS_VBC_SYS_h14_C VBC_SYS_h14;
	volatile VBC_SYS_VBC_SYS_h18_C VBC_SYS_h18;
	volatile VBC_SYS_VBC_SYS_h1C_C VBC_SYS_h1C;
	volatile VBC_SYS_VBC_SYS_h20_C VBC_SYS_h20;
	volatile VBC_SYS_VBC_SYS_h24_C VBC_SYS_h24;
	volatile VBC_SYS_VBC_SYS_h28_C VBC_SYS_h28;
	volatile VBC_SYS_VBC_SYS_h2C_C VBC_SYS_h2C;
	volatile VBC_SYS_VBC_SYS_h30_C VBC_SYS_h30;
	volatile VBC_SYS_VBC_SYS_h34_C VBC_SYS_h34;
	volatile VBC_SYS_VBC_SYS_h38_C VBC_SYS_h38;
	volatile VBC_SYS_VBC_SYS_h3C_C VBC_SYS_h3C;
	volatile VBC_SYS_VBC_SYS_h40_C VBC_SYS_h40;
	volatile VBC_SYS_VBC_SYS_h44_C VBC_SYS_h44;
	volatile VBC_SYS_VBC_SYS_h48_C VBC_SYS_h48;
	volatile VBC_SYS_VBC_SYS_h4C_C VBC_SYS_h4C;
	volatile VBC_SYS_VBC_SYS_h50_C VBC_SYS_h50;
	volatile VBC_SYS_VBC_SYS_h54_C VBC_SYS_h54;
	volatile VBC_SYS_VBC_SYS_h58_C VBC_SYS_h58;
	volatile VBC_SYS_VBC_SYS_h5C_C VBC_SYS_h5C;
	volatile VBC_SYS_VBC_SYS_h60_C VBC_SYS_h60;
	volatile VBC_SYS_VBC_SYS_h64_C VBC_SYS_h64;
	volatile VBC_SYS_VBC_SYS_h68_C VBC_SYS_h68;
	volatile VBC_SYS_VBC_SYS_h70_C VBC_SYS_h70;
	volatile VBC_SYS_VBC_SYS_h74_C VBC_SYS_h74;
	volatile VBC_SYS_VBC_SYS_h78_C VBC_SYS_h78;
	volatile VBC_SYS_VBC_SYS_h7C_C VBC_SYS_h7C;
	volatile VBC_SYS_VBC_SYS_h80_C VBC_SYS_h80;
	volatile VBC_SYS_VBC_SYS_h84_C VBC_SYS_h84;
	volatile VBC_SYS_VBC_SYS_h88_C VBC_SYS_h88;
	volatile VBC_SYS_VBC_SYS_h90_C VBC_SYS_h90;
	volatile VBC_SYS_VBC_SYS_h94_C VBC_SYS_h94;
	volatile VBC_SYS_VBC_SYS_h98_C VBC_SYS_h98;
	volatile VBC_SYS_VBC_SYS_h9C_C VBC_SYS_h9C;
	volatile VBC_SYS_VBC_SYS_hA0_C VBC_SYS_hA0;
	volatile VBC_SYS_VBC_SYS_hA4_C VBC_SYS_hA4;
	volatile VBC_SYS_VBC_SYS_hA8_C VBC_SYS_hA8;
	volatile VBC_SYS_VBC_SYS_hAC_C VBC_SYS_hAC;
	volatile VBC_SYS_VBC_SYS_hB0_C VBC_SYS_hB0;
	volatile VBC_SYS_VBC_SYS_hB4_C VBC_SYS_hB4;
	volatile VBC_SYS_VBC_SYS_hC0_C VBC_SYS_hC0;
	volatile VBC_SYS_VBC_SYS_hC4_C VBC_SYS_hC4;
} VBC_SYS_C;


void init_vbc_sys_reg(VBC_SYS_C &vbc)
{
	memset(&vbc, 0, sizeof(VBC_SYS_C));

	vbc.VBC_SYS_h0C.reg_vbc_cg_en = 0x1;
	vbc.VBC_SYS_h0C.reg_vbc_cg_larb_en = 0x1;

	vbc.VBC_SYS_h20.reg_vbc_spare = 0xFFFF0000;
	vbc.VBC_SYS_h2C.reg_vbc_axi_rc0_map = 1;
	vbc.VBC_SYS_h2C.reg_vbc_axi_rc1_map = 1;
	vbc.VBC_SYS_h2C.reg_vbc_axi_rc2_map = 1;
	vbc.VBC_SYS_h2C.reg_vbc_axi_wc0_map = 1;
	vbc.VBC_SYS_h2C.reg_vbc_axi_wc1_map = 1;
	vbc.VBC_SYS_h2C.reg_vbc_axi_wc2_map = 1;
};
#endif //__REG_VBC_SYS_STRUCT_H__
