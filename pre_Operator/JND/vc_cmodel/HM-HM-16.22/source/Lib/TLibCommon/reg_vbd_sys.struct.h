
#ifndef __REG_VBD_SYS_STRUCT_H__
#define __REG_VBD_SYS_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*VBC SW Reset Core engine;*/
		uint32_t reg_vbc_sw_rst_core:1;
		uint32_t rsv_1_3:3;
		/*VBC frame start*/
		uint32_t reg_vbc_frame_start:1;
		uint32_t rsv_5_7:3;
		/*VBD SA (Start Address) DMA go*/
		uint32_t reg_vbc_sa_init_go:1;
		/*Enable SA prefetch*/
		uint32_t reg_vbc_sa_prefetch_en:1;
		uint32_t rsv_10_15:6;
		/*"Multi-DMA active number minus 1
		4'b0000:: 1 CU Row
		4'b0011:  4 CU Rows
		4'b1111:  16 CU Rows
		others: reserved" */
		uint32_t reg_vbc_ndma_act_num_m1:4;
		uint32_t rsv_20_23:4;
		/*VBC SW Reset Core engine done*/
		uint32_t reg_vbd_sw_rst_core_done:1;
		uint32_t rsv_25_30:6;
		/*VBD frame done stutus*/
		uint32_t reg_vbd_frm_done:1;
	};
	uint32_t val;
} VBD_SYS_h00_C;
typedef union {
	struct {
		/* Enable VBD clock gating */
		uint32_t reg_vbd_gc_ena:1;
		uint32_t rsv_1_7:7;
		uint32_t reg_vbd_ck_status:8;
		uint32_t rsv_16_31:16;
	};
	uint32_t val;
} VBD_SYS_h14_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*VBC Encoder SA base address, 1KB alignment*/
		uint32_t reg_vbc_sa_base_lsb:22;
	};
	uint32_t val;
} VBD_SYS_h20_C;
typedef union {
	struct {
		/*VBC Encoder SA base address, 1KB alignment*/
		uint32_t reg_vbc_sa_base_msb:32;
	};
	uint32_t val;
} VBD_SYS_h24_C;
typedef union {
	struct {
		uint32_t rsv_0_7:8;
		/*VBC Encoder Stream base address, 256B alignment*/
		uint32_t reg_vbc_cpx_base_lsb:24;
	};
	uint32_t val;
} VBD_SYS_h28_C;
typedef union {
	struct {
		/*VBC Encoder Stream base address, 256B alignment*/
		uint32_t reg_vbc_cpx_base_msb:32;
	};
	uint32_t val;
} VBD_SYS_h2C_C;
typedef union {
	struct {
		/* "AXI source 0: from JPG 1: from VBD" */
		uint32_t reg_vbc_ena:1;
		uint32_t rsv_1_7:7;
		/*VBD AXI idle status*/
		uint32_t reg_vbc_axi_idle:1;
		uint32_t rsv_9_15:7;
		/*VBD AXI Read arbiter staus clear*/
		uint32_t reg_vbc_axir_arb_sts_clr:1;
		uint32_t rsv_17_31:15;
	};
	uint32_t val;
} VBD_SYS_h30_C;
typedef union {
	struct {
		/* "Color format 0: nv12 1: rgb888" */
		uint32_t reg_vbc_color_fmt:1;
		/*"0: NV12 8b mode 1: NV12 10b mode" */
		uint32_t reg_vbc_nv12_10b:1;
		uint32_t rsv_2_31:30;
	};
	uint32_t val;
} VBD_SYS_h34_C;
typedef union {
	struct {
		/* Picture width in pixel minus 1 */
		uint32_t reg_vbc_pic_width_m1:16;
		/* Picture height in pixel minus 1 */
		uint32_t reg_vbc_pic_height_m1:16;
	};
	uint32_t val;
} VBD_SYS_h38_C;
typedef union {
	struct {
		uint32_t rsv_0_4:5;
		/*Picture Crop X32 start*/
		uint32_t reg_vbc_pic_crop_xst32:11;
		uint32_t rsv_16_17:2;
		/*Picture Crop Y4 start*/
		uint32_t reg_vbc_pic_crop_yst4:14;
	};
	uint32_t val;
} VBD_SYS_h3C_C;
typedef union {
	struct {
		/* "VBC lossy mode truncate bits
		0: 1bit
		1: 2bits
		2: 3bits
		3: 4bits" */
		uint32_t reg_vbc_lossy_tb:2;
		uint32_t rsv_2_31:30;
	};
	uint32_t val;
} VBD_SYS_h50_C;

typedef struct {
	volatile VBD_SYS_h00_C VBD_SYS_h00;
	volatile VBD_SYS_h14_C VBD_SYS_h14;
	volatile VBD_SYS_h20_C VBD_SYS_h20;
	volatile VBD_SYS_h24_C VBD_SYS_h24;
	volatile VBD_SYS_h28_C VBD_SYS_h28;
	volatile VBD_SYS_h2C_C VBD_SYS_h2C;
	volatile VBD_SYS_h30_C VBD_SYS_h30;
	volatile VBD_SYS_h34_C VBD_SYS_h34;
	volatile VBD_SYS_h38_C VBD_SYS_h38;
	volatile VBD_SYS_h3C_C VBD_SYS_h3C;
	volatile VBD_SYS_h50_C VBD_SYS_h50;
} VBD_SYS_C;

void init_vbd_sys_reg(VBD_SYS_C &vbd)
{
	memset(&vbd, 0, sizeof(VBD_SYS_C));
};

#endif //__REG_VBD_SYS_STRUCT_H__
