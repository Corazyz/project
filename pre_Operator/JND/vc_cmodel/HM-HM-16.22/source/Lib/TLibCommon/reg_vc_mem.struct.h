// $Module: vc_mem $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 28 Jul 2021 07:42:22 PM $
//

#ifndef __REG_VC_MEM_STRUCT_H__
#define __REG_VC_MEM_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*Neighbor Management B base address (1KB);*/
		uint32_t reg_nm_b_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h00_C;
typedef union {
	struct {
		/*Neighbor Management B base address (1KB);*/
		uint32_t reg_nm_b_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h04_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*Neighbor Management A base address (1KB);*/
		uint32_t reg_nm_a_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h08_C;
typedef union {
	struct {
		/*Neighbor Management A base address (1KB);*/
		uint32_t reg_nm_a_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h0C_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*Neighbor Management Col-located base address (1KB);*/
		uint32_t reg_nm_col_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h10_C;
typedef union {
	struct {
		/*Neighbor Management Col-located base address (1KB);*/
		uint32_t reg_nm_col_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h14_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*Neighbor Management write back MV base address (1KB);*/
		uint32_t reg_nm_mv_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h18_C;
typedef union {
	struct {
		/*Neighbor Management write back MV base address (1KB);*/
		uint32_t reg_nm_mv_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h1C_C;
typedef union {
	struct {
		uint32_t rsv_0_4:5;
		/*Neighbor Management Col-located pitch (32B);*/
		uint32_t reg_nm_col_pit:10;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h20_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*Intra Prediction Base address (1KB);*/
		uint32_t reg_iapu_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h30_C;
typedef union {
	struct {
		/*Intra Prediction Base address (1KB);*/
		uint32_t reg_iapu_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h34_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*In-loop filter B Luma base address (1KB);*/
		uint32_t reg_ilf_b_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h40_C;
typedef union {
	struct {
		/*In-loop filter B Luma base address (1KB);*/
		uint32_t reg_ilf_b_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h44_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*In-loop filter  A Luma base address (1KB);*/
		uint32_t reg_ilf_a_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h48_C;
typedef union {
	struct {
		/*In-loop filter  A Luma base address (1KB);*/
		uint32_t reg_ilf_a_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h4C_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*Current decoded frame buffer base address (1KB);*/
		uint32_t reg_cur_fb_luma_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h60_C;
typedef union {
	struct {
		uint32_t reg_cur_fb_luma_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h64_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*Current decoded frame buffer base address (1KB);*/
		uint32_t reg_cur_fb_chroma_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h68_C;
typedef union {
	struct {
		uint32_t reg_cur_fb_chroma_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h6C_C;
typedef union {
	struct {
		uint32_t rsv_0_4:5;
		/*Current decoded frame buffer pitch (32B);*/
		uint32_t reg_cur_fb_pit:10;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h70_C;
typedef union {
	struct {
		/*Program DPB Base;*/
		uint32_t reg_prg_dpb_base_go:1;
		/*Program DPB Base index, 0: Luma, 1: Chroma;*/
		uint32_t reg_prg_dpb_base_idx:1;
		/*Program DPB Base, 0: Read, 1: Write;*/
		uint32_t reg_prg_dpb_base_rw:1;
		/*Program DPB Base type, 0: org size, 1: subsample size;*/
		uint32_t reg_prg_dpb_base_type:1;
		uint32_t rsv_4_15:12;
		/*Program DPB Base Address;*/
		uint32_t reg_prg_dpb_base_addr:4;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h80_C;
typedef union {
	struct {
		/*Program DPB Base Write Data;*/
		uint32_t reg_prg_dpb_base_wd_lsb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h88_C;
typedef union {
	struct {
		/*Program DPB Base Write Data;*/
		uint32_t reg_prg_dpb_base_wd_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h8C_C;
typedef union {
	struct {
		/*Program DPB Base Read Data;*/
		uint32_t reg_prg_dpb_base_rd_lsb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h90_C;
typedef union {
	struct {
		/*Program DPB Base Read Data;*/
		uint32_t reg_prg_dpb_base_rd_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_h94_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*QP/SKIP Map base address (1KB);*/
		uint32_t reg_qpmap_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_hA0_C;
typedef union {
	struct {
		uint32_t reg_qpmap_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_hA4_C;
typedef union {
	struct {
		uint32_t rsv_0_4:5;
		/*QP/SKIP Map pitch (32B);*/
		uint32_t reg_qpmap_pit:10;
	};
	uint32_t val;
} VC_MEM_VC_MEM_hA8_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*CU Depth Map base address (1KB);*/
		uint32_t reg_cudep_base:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_hB0_C;
typedef union {
	struct {
		uint32_t rsv_0_4:5;
		/*CU Depth Map pitch (32B);*/
		uint32_t reg_cudep_pit:10;
	};
	uint32_t val;
} VC_MEM_VC_MEM_hB8_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*AI dqp base address*/
		uint32_t reg_ai_dqp_src_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_hC0_C;
typedef union {
	struct {
		uint32_t reg_ai_dqp_src_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_hC4_C;
typedef union {
	struct {
		uint32_t rsv_0_4:5;
		/*AI dqp pitch*/
		uint32_t reg_qpmap_pit:10;
	};
	uint32_t val;
} VC_MEM_VC_MEM_hC8_C;
typedef union {
	struct {
		uint32_t rsv_0_9:10;
		/*ISP dqp base address*/
		uint32_t reg_isp_dqp_src_base_lsb:22;
	};
	uint32_t val;
} VC_MEM_VC_MEM_hD0_C;
typedef union {
	struct {
		uint32_t reg_isp_dqp_src_base_msb:32;
	};
	uint32_t val;
} VC_MEM_VC_MEM_hD4_C;
typedef union {
	struct {
		/*ISP dqp pitch*/
		uint32_t reg_qpmap_pit:14;
	};
	uint32_t val;
} VC_MEM_VC_MEM_hD8_C;
typedef struct {
	volatile VC_MEM_VC_MEM_h00_C VC_MEM_h00;
	volatile VC_MEM_VC_MEM_h04_C VC_MEM_h04;
	volatile VC_MEM_VC_MEM_h08_C VC_MEM_h08;
	volatile VC_MEM_VC_MEM_h0C_C VC_MEM_h0C;
	volatile VC_MEM_VC_MEM_h10_C VC_MEM_h10;
	volatile VC_MEM_VC_MEM_h14_C VC_MEM_h14;
	volatile VC_MEM_VC_MEM_h18_C VC_MEM_h18;
	volatile VC_MEM_VC_MEM_h1C_C VC_MEM_h1C;
	volatile VC_MEM_VC_MEM_h20_C VC_MEM_h20;
	volatile VC_MEM_VC_MEM_h30_C VC_MEM_h30;
	volatile VC_MEM_VC_MEM_h34_C VC_MEM_h34;
	volatile VC_MEM_VC_MEM_h40_C VC_MEM_h40;
	volatile VC_MEM_VC_MEM_h44_C VC_MEM_h44;
	volatile VC_MEM_VC_MEM_h48_C VC_MEM_h48;
	volatile VC_MEM_VC_MEM_h4C_C VC_MEM_h4C;
	volatile VC_MEM_VC_MEM_h60_C VC_MEM_h60;
	volatile VC_MEM_VC_MEM_h64_C VC_MEM_h64;
	volatile VC_MEM_VC_MEM_h68_C VC_MEM_h68;
	volatile VC_MEM_VC_MEM_h6C_C VC_MEM_h6C;
	volatile VC_MEM_VC_MEM_h70_C VC_MEM_h70;
	volatile VC_MEM_VC_MEM_h80_C VC_MEM_h80;
	volatile VC_MEM_VC_MEM_h88_C VC_MEM_h88;
	volatile VC_MEM_VC_MEM_h8C_C VC_MEM_h8C;
	volatile VC_MEM_VC_MEM_h90_C VC_MEM_h90;
	volatile VC_MEM_VC_MEM_h94_C VC_MEM_h94;
	volatile VC_MEM_VC_MEM_hA0_C VC_MEM_hA0;
	volatile VC_MEM_VC_MEM_hA4_C VC_MEM_hA4;
	volatile VC_MEM_VC_MEM_hA8_C VC_MEM_hA8;
	volatile VC_MEM_VC_MEM_hB0_C VC_MEM_hB0;
	volatile VC_MEM_VC_MEM_hB8_C VC_MEM_hB8;
	volatile VC_MEM_VC_MEM_hC0_C VC_MEM_hC0;
	volatile VC_MEM_VC_MEM_hC4_C VC_MEM_hC4;
	volatile VC_MEM_VC_MEM_hC8_C VC_MEM_hC8;
	volatile VC_MEM_VC_MEM_hD0_C VC_MEM_hD0;
	volatile VC_MEM_VC_MEM_hD4_C VC_MEM_hD4;
	volatile VC_MEM_VC_MEM_hD8_C VC_MEM_hD8;
} VC_MEM_C;
#define DEFINE_VC_MEM_C(X) \
	 VC_MEM_C X = \
{\
	.VC_MEM_h00.reg_nm_b_base_lsb = 0x0,\
	.VC_MEM_h04.reg_nm_b_base_msb = 0x0,\
	.VC_MEM_h08.reg_nm_a_base_lsb = 0x0,\
	.VC_MEM_h0C.reg_nm_a_base_msb = 0x0,\
	.VC_MEM_h10.reg_nm_col_base_lsb = 0x0,\
	.VC_MEM_h1C.reg_nm_col_base_msb = 0x0,\
	.VC_MEM_h18.reg_nm_mv_base_lsb = 0x0,\
	.VC_MEM_h1C.reg_nm_mv_base_msb = 0x0,\
	.VC_MEM_h20.reg_nm_col_pit = 0x0,\
	.VC_MEM_h30.reg_iapu_base_lsb = 0x0,\
	.VC_MEM_h34.reg_iapu_base_msb = 0x0,\
	.VC_MEM_h40.reg_ilf_b_base_lsb = 0x0,\
	.VC_MEM_h44.reg_ilf_b_base_msb = 0x0,\
	.VC_MEM_h48.reg_ilf_a_base_lsb = 0x0,\
	.VC_MEM_h4C.reg_ilf_a_base_msb = 0x0,\
	.VC_MEM_h60.reg_cur_fb_luma_base_lsb = 0x0,\
	.VC_MEM_h64.reg_cur_fb_luma_base_msb = 0x0,\
	.VC_MEM_h68.reg_cur_fb_chroma_base_lsb = 0x0,\
	.VC_MEM_h6C.reg_cur_fb_chroma_base_msb = 0x0,\
	.VC_MEM_h70.reg_cur_fb_pit = 0x0,\
	.VC_MEM_h80.reg_prg_dpb_base_go = 0x0,\
	.VC_MEM_h80.reg_prg_dpb_base_idx = 0x0,\
	.VC_MEM_h80.reg_prg_dpb_base_rw = 0x0,\
	.VC_MEM_h80.reg_prg_dpb_base_type = 0x0,\
	.VC_MEM_h80.reg_prg_dpb_base_addr = 0x0,\
	.VC_MEM_h88.reg_prg_dpb_base_wd_lsb = 0x0,\
	.VC_MEM_h8C.reg_prg_dpb_base_wd_msb = 0x0,\
	.VC_MEM_h90.reg_prg_dpb_base_rd_lsb = 0x0,\
	.VC_MEM_h94.reg_prg_dpb_base_rd_msb = 0x0,\
	.VC_MEM_hA0.reg_qpmap_base_lsb = 0x0,\
	.VC_MEM_hA4.reg_qpmap_base_msb = 0x0,\
	.VC_MEM_hA8.reg_qpmap_pit = 0x0,\
	.VC_MEM_hB0.reg_cudep_base = 0x0,\
	.VC_MEM_hB8.reg_cudep_pit = 0x0,\
};

void init_vc_mem_reg(VC_MEM_C &mem)
{
    mem.VC_MEM_h00.reg_nm_b_base_lsb = 0x0;
    mem.VC_MEM_h04.reg_nm_b_base_msb = 0x0;
    mem.VC_MEM_h08.reg_nm_a_base_lsb = 0x0;
    mem.VC_MEM_h0C.reg_nm_a_base_msb = 0x0;
    mem.VC_MEM_h10.reg_nm_col_base_lsb = 0x0;
    mem.VC_MEM_h14.reg_nm_col_base_msb = 0x0;
    mem.VC_MEM_h18.reg_nm_mv_base_lsb = 0x0;
    mem.VC_MEM_h1C.reg_nm_mv_base_msb = 0x0;
    mem.VC_MEM_h20.reg_nm_col_pit = 0x0;
    mem.VC_MEM_h30.reg_iapu_base_lsb = 0x0;
    mem.VC_MEM_h34.reg_iapu_base_msb = 0x0;
    mem.VC_MEM_h40.reg_ilf_b_base_lsb = 0x0;
    mem.VC_MEM_h44.reg_ilf_b_base_msb = 0x0;
    mem.VC_MEM_h48.reg_ilf_a_base_lsb = 0x0;
    mem.VC_MEM_h4C.reg_ilf_a_base_msb = 0x0;
    mem.VC_MEM_h60.reg_cur_fb_luma_base_lsb = 0x0;
    mem.VC_MEM_h64.reg_cur_fb_luma_base_msb = 0x0;
    mem.VC_MEM_h68.reg_cur_fb_chroma_base_lsb = 0x0;
    mem.VC_MEM_h6C.reg_cur_fb_chroma_base_msb = 0x0;
    mem.VC_MEM_h70.reg_cur_fb_pit = 0x0;
    mem.VC_MEM_h80.reg_prg_dpb_base_go = 0x0;
    mem.VC_MEM_h80.reg_prg_dpb_base_idx = 0x0;
    mem.VC_MEM_h80.reg_prg_dpb_base_rw = 0x0;
    mem.VC_MEM_h80.reg_prg_dpb_base_type = 0x0;
    mem.VC_MEM_h80.reg_prg_dpb_base_addr = 0x0;
    mem.VC_MEM_h88.reg_prg_dpb_base_wd_lsb = 0x0;
    mem.VC_MEM_h8C.reg_prg_dpb_base_wd_msb = 0x0;
    mem.VC_MEM_h90.reg_prg_dpb_base_rd_lsb = 0x0;
    mem.VC_MEM_h94.reg_prg_dpb_base_rd_msb = 0x0;
    mem.VC_MEM_hA0.reg_qpmap_base_lsb = 0x0;
    mem.VC_MEM_hA4.reg_qpmap_base_msb = 0x0;
    mem.VC_MEM_hA8.reg_qpmap_pit = 0x0;
    mem.VC_MEM_hB0.reg_cudep_base = 0x0;
    mem.VC_MEM_hB8.reg_cudep_pit = 0x0;
    mem.VC_MEM_hC0.val = 0x0;
    mem.VC_MEM_hC4.val = 0x0;
    mem.VC_MEM_hC8.val = 0x0;
    mem.VC_MEM_hD0.val = 0x0;
    mem.VC_MEM_hD4.val = 0x0;
    mem.VC_MEM_hD8.val = 0x0;
};
#endif //__REG_VC_MEM_STRUCT_H__
