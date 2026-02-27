// $Module: vc_hdr $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 28 Jul 2021 07:42:53 PM $
//

#ifndef __REG_VC_HDR_STRUCT_H__
#define __REG_VC_HDR_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*Min CU size,
		0: 4, 1: 8, 2: 16, 3: 32. 4: 64, 5: 128;*/
		uint32_t reg_min_cu_sz:3;
		uint32_t rsv_3_3:1;
		/*CTU size,
		0: 4, 1: 8, 2: 16, 3: 32. 4: 64, 5: 128;*/
		uint32_t reg_ctu_sz:3;
		uint32_t rsv_7_7:1;
		/*Min TU size,
		0: 4, 1: 8, 2: 16, 3: 32. 4: 64, 5: 128;*/
		uint32_t reg_min_tu_sz:3;
		uint32_t rsv_11_11:1;
		/*Max TU size,
		0: 4, 1: 8, 2: 16, 3: 32. 4: 64, 5: 128;*/
		uint32_t reg_max_tu_sz:3;
		uint32_t rsv_15_15:1;
		/*Max Inter TU depth;*/
		uint32_t reg_max_tu_dep_inter_m1:2;
		uint32_t rsv_18_19:2;
		/*Max Intra TU depth;*/
		uint32_t reg_max_tu_dep_intra_m1:2;
	};
	uint32_t val;
} VC_HDR_SPS_REG_h00_C;
typedef union {
	struct {
		/*Luma Color Depth, 
		1: 8bits, 2: 9bits, 4: 10bits;*/
		uint32_t reg_luma_bitdepth:3;
		uint32_t rsv_3_3:1;
		/*Chroma Color Depth, 
		1: 8bits, 2: 9bits, 4: 10bits;*/
		uint32_t reg_chroma_bitdepth:3;
	};
	uint32_t val;
} VC_HDR_SPS_REG_h04_C;
typedef union {
	struct {
		/*PCM Luma Color Depth, 
		1: 8bits, 2: 9bits, 4: 10bits;*/
		uint32_t reg_pcm_luma_bitdepth:3;
		uint32_t rsv_3_3:1;
		/*PCM Chroma Color Depth, 
		1: 8bits, 2: 9bits, 4: 10bits;*/
		uint32_t reg_pcm_chroma_bitdepth:3;
		uint32_t rsv_7_7:1;
		/*PCM Min CU size,
		0: 4, 1: 8, 2: 16, 3: 32. 4: 64, 5: 128;*/
		uint32_t reg_pcm_min_cu_sz:3;
		uint32_t rsv_11_11:1;
		/*PCM CTU size,
		0: 4, 1: 8, 2: 16, 3: 32. 4: 64, 5: 128;*/
		uint32_t reg_pcm_ctu_sz:3;
	};
	uint32_t val;
} VC_HDR_SPS_REG_h08_C;
typedef union {
	struct {
		/*Asymetric Part Mode flag;*/
		uint32_t reg_amp_flag:1;
		/*SAO flag;*/
		uint32_t reg_sao_flag:1;
		/*PCM flag;*/
		uint32_t reg_pcm_flag:1;
		/*Strong Intra Smooth flag;*/
		uint32_t reg_sis_flag:1;
	};
	uint32_t val;
} VC_HDR_SPS_REG_h10_C;
typedef union {
	struct {
		/*Picture width in CU minus 1;*/
		uint32_t reg_pic_width_cu_m1:9;
		uint32_t rsv_9_15:7;
		/*Picture height in CU minus 1;*/
		uint32_t reg_pic_height_cu_m1:9;
	};
	uint32_t val;
} VC_HDR_SPS_REG_h14_C;
typedef union {
	struct {
		/*Picture width in CTU minus 1;*/
		uint32_t reg_pic_width_ctu_m1:9;
		uint32_t rsv_9_15:7;
		/*Picture height in CTU minus 1;*/
		uint32_t reg_pic_height_ctu_m1:9;
	};
	uint32_t val;
} VC_HDR_SPS_REG_h18_C;
typedef union {
	struct {
		/*Dependent Slice flag;*/
		uint32_t reg_dependent_slice_flag:1;
		/*CABAC Sign Hiding flag;*/
		uint32_t reg_signed_hiding_flag:1;
		/*Tile flag;*/
		uint32_t reg_tile_flag:1;
		/*Temporal MVP flag;*/
		uint32_t reg_tmp_mvp_flag:1;
		uint32_t rsv_4_5:2;
		/*Weight Prediction flag;*/
		uint32_t reg_weight_pred_flag:1;
		/*Weight BiPrediction flag;*/
		uint32_t reg_weight_bipred_flag:1;
		/*Use integer MV only, SCC extension;*/
		uint32_t reg_use_integer_mv:1;
		/*Transformation Bypass flag;*/
		uint32_t reg_trans_bypass_flag:1;
		/*Entropy coding sync flag;*/
		uint32_t reg_entropy_sync_flag:1;
		/*Constrained Intra Prediction;*/
		uint32_t reg_cons_intra_pred:1;
		/*Parallel merge level minus 2;*/
		uint32_t reg_parallel_mgr_lvl_m2:4;
		uint32_t rsv_16_30:15;
		/*AVC Entropy Coding mode, 0: CAVLC, 1: CABAC*/
		uint32_t reg_avc_entropy_coding:1;
	};
	uint32_t val;
} VC_HDR_PPS_REG_h80_C;
typedef union {
	struct {
		/*Current POC;*/
		uint32_t reg_cur_poc:32;
	};
	uint32_t val;
} VC_HDR_PPS_REG_h84_C;
typedef union {
	struct {
		/*Column number of tile minus 1;*/
		uint32_t reg_tile_col_m1:8;
		uint32_t rsv_8_15:8;
		/*Row number of tile minus 1;*/
		uint32_t reg_tile_row_m1:8;
	};
	uint32_t val;
} VC_HDR_PPS_REG_h88_C;
typedef union {
	struct {
		/*Program tileID index X;*/
		uint32_t reg_prg_tileid_x:8;
		uint32_t rsv_8_15:8;
		/*Program tileID index Y;*/
		uint32_t reg_prg_tileid_y:8;
		uint32_t rsv_24_28:5;
		/*Program tileID read data sel, 
		0: current write data, 1: tileID coordinates;*/
		uint32_t reg_prg_tileid_rd_sel:1;
		/*Program tileID read/write, 0: R, 1: W;*/
		uint32_t reg_prg_tileid_rw:1;
		/*Program tileID go, 1T pulse;*/
		uint32_t reg_prg_tileid_go:1;
	};
	uint32_t val;
} VC_HDR_PPS_REG_h8C_C;
typedef union {
	struct {
		/*Program tileID coordinate X start;*/
		uint32_t reg_prg_tileid_xst:8;
		uint32_t rsv_8_15:8;
		/*Program tileID coordinate X end;*/
		uint32_t reg_prg_tileid_xend:8;
	};
	uint32_t val;
} VC_HDR_PPS_REG_h90_C;
typedef union {
	struct {
		/*Program tileID coordinate Y start;*/
		uint32_t reg_prg_tileid_yst:8;
		uint32_t rsv_8_15:8;
		/*Program tileID coordinate Y end;*/
		uint32_t reg_prg_tileid_yend:8;
	};
	uint32_t val;
} VC_HDR_PPS_REG_h94_C;
typedef union {
	struct {
		/*Slice go;*/
		uint32_t reg_slice_go:1;
		/*CABAC initialization go;*/
		uint32_t reg_cabac_init_go:1;
		uint32_t rsv_2_7:6;
		/*Slice Done;*/
		uint32_t reg_slice_done:1;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h100_C;
typedef union {
	struct {
		/*I slice;*/
		uint32_t reg_i_slice:1;
		/*P slice;*/
		uint32_t reg_p_slice:1;
		/*B slice;*/
		uint32_t reg_b_slice:1;
		uint32_t rsv_3_3:1;
		/*CABAC Initialization Type;*/
		uint32_t reg_cabac_init_type:2;
		/*Col-located from L0 flag;*/
		uint32_t reg_col_l0_flag:1;
		/*Write Back MV flag;*/
		uint32_t reg_wb_mv_flag:1;
		/*MVD of L1 is zero flag;*/
		uint32_t reg_mvd_l1_zero_flag:1;
		uint32_t rsv_9_15:7;
		/*Col-located Buffer Read index;*/
		uint32_t reg_col_buf_ridx:4;
		/*Col-located Buffer Write index;*/
		uint32_t reg_col_buf_widx:4;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h104_C;
typedef union {
	struct {
		/*Number of reference L0 minus 1;*/
		uint32_t reg_num_ref_l0_act_m1:4;
		/*Number of reference L1 minus 1;*/
		uint32_t reg_num_ref_l1_act_m1:4;
		/*Col-located reference index;*/
		uint32_t reg_col_ref_idx:4;
		/*Max. merge candidates;*/
		uint32_t reg_max_merge_cand_m1:3;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h108_C;
typedef union {
	struct {
		/*Program Reference List;*/
		uint32_t reg_prg_ref_list_go:1;
		/*Program Reference List, 0: L0, 1: L1;*/
		uint32_t reg_prg_ref_list_idx:1;
		/*Program Reference List, 0: Read, 1: Write;*/
		uint32_t reg_prg_ref_list_rw:1;
		uint32_t rsv_3_15:13;
		/*Program Reference List Address;*/
		uint32_t reg_prg_ref_list_addr:8;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h10C_C;
typedef union {
	struct {
		/*Program Reference List Write Data;*/
		uint32_t reg_prg_ref_list_wd:32;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h110_C;
typedef union {
	struct {
		/*Program Reference List Read Data;*/
		uint32_t reg_prg_ref_list_rd:32;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h114_C;
typedef union {
	struct {
		/*Slice QP, 0 ~ 51;*/
		uint32_t reg_slice_qp:6;
		uint32_t rsv_6_7:2;
		/*Slice CB QP offset, -12 ~ 12;*/
		uint32_t reg_slice_cb_qp_ofs:5;
		uint32_t rsv_13_15:3;
		/*Slice CR QP offset, -12 ~ 12;*/
		uint32_t reg_slice_cr_qp_ofs:5;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h140_C;
typedef union {
	struct {
		/*Program Scaling List;*/
		uint32_t reg_prg_scl_list_go:1;
		uint32_t rsv_1_1:1;
		/*Program Scaling List, 0: Read, 1: Write;*/
		uint32_t reg_prg_scl_list_rw:1;
		uint32_t rsv_3_15:13;
		/*Program Scaling List Address;*/
		uint32_t reg_prg_scl_list_addr:8;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h144_C;
typedef union {
	struct {
		/*Program Scaling List Write Data;*/
		uint32_t reg_prg_scl_list_wd:32;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h148_C;
typedef union {
	struct {
		/*Program Scaling List Read Data;*/
		uint32_t reg_prg_scl_list_rd:32;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h14C_C;
typedef union {
	struct {
		/*Slice ILF disable;*/
		uint32_t reg_slice_ilf_dis:1;
		/*Slice ILF cross tile/slice disable;*/
		uint32_t reg_slice_ilf_cross_dis:1;
		uint32_t rsv_2_3:2;
		/*Slice Beta offset div 2, -6 ~ 6;*/
		uint32_t reg_slice_beta_ofs_div2:4;
		/*Slice TC offset div 2, -6 ~ 6;*/
		uint32_t reg_slice_tc_ofs_div2:4;
		uint32_t rsv_12_15:4;
		/*SAO Luma flag;*/
		uint32_t reg_sao_luma_flag:1;
		/*SAO Chroma flag;*/
		uint32_t reg_sao_chroma_flag:1;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h150_C;
typedef union {
	struct {
		/*Slice CTU start X;*/
		uint32_t reg_ctu_st_x:8;
		uint32_t rsv_8_15:8;
		/*Slice CTU start Y;*/
		uint32_t reg_ctu_st_y:8;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h160_C;
typedef union {
	struct {
		/*Slice CTU end X;*/
		uint32_t reg_ctu_end_x:8;
		uint32_t rsv_8_15:8;
		/*Slice CTU end Y;*/
		uint32_t reg_ctu_end_y:8;
	};
	uint32_t val;
} VC_HDR_SLICE_REG_h164_C;
typedef struct {
	volatile VC_HDR_SPS_REG_h00_C SPS_REG_h00;
	volatile VC_HDR_SPS_REG_h04_C SPS_REG_h04;
	volatile VC_HDR_SPS_REG_h08_C SPS_REG_h08;
	volatile VC_HDR_SPS_REG_h10_C SPS_REG_h10;
	volatile VC_HDR_SPS_REG_h14_C SPS_REG_h14;
	volatile VC_HDR_SPS_REG_h18_C SPS_REG_h18;
	volatile VC_HDR_PPS_REG_h80_C PPS_REG_h80;
	volatile VC_HDR_PPS_REG_h84_C PPS_REG_h84;
	volatile VC_HDR_PPS_REG_h88_C PPS_REG_h88;
	volatile VC_HDR_PPS_REG_h8C_C PPS_REG_h8C;
	volatile VC_HDR_PPS_REG_h90_C PPS_REG_h90;
	volatile VC_HDR_PPS_REG_h94_C PPS_REG_h94;
	volatile VC_HDR_SLICE_REG_h100_C SLICE_REG_h100;
	volatile VC_HDR_SLICE_REG_h104_C SLICE_REG_h104;
	volatile VC_HDR_SLICE_REG_h108_C SLICE_REG_h108;
	volatile VC_HDR_SLICE_REG_h10C_C SLICE_REG_h10C;
	volatile VC_HDR_SLICE_REG_h110_C SLICE_REG_h110;
	volatile VC_HDR_SLICE_REG_h114_C SLICE_REG_h114;
	volatile VC_HDR_SLICE_REG_h140_C SLICE_REG_h140;
	volatile VC_HDR_SLICE_REG_h144_C SLICE_REG_h144;
	volatile VC_HDR_SLICE_REG_h148_C SLICE_REG_h148;
	volatile VC_HDR_SLICE_REG_h14C_C SLICE_REG_h14C;
	volatile VC_HDR_SLICE_REG_h150_C SLICE_REG_h150;
	volatile VC_HDR_SLICE_REG_h160_C SLICE_REG_h160;
	volatile VC_HDR_SLICE_REG_h164_C SLICE_REG_h164;
} VC_HDR_C;
#define DEFINE_VC_HDR_C(X) \
	 VC_HDR_C X = \
{\
	.SPS_REG_h00.reg_min_cu_sz = 0x0,\
	.SPS_REG_h00.reg_ctu_sz = 0x0,\
	.SPS_REG_h00.reg_min_tu_sz = 0x0,\
	.SPS_REG_h00.reg_max_tu_sz = 0x0,\
	.SPS_REG_h00.reg_max_tu_dep_inter_m1 = 0x0,\
	.SPS_REG_h00.reg_max_tu_dep_intra_m1 = 0x0,\
	.SPS_REG_h04.reg_luma_bitdepth = 0x1,\
	.SPS_REG_h04.reg_chroma_bitdepth = 0x1,\
	.SPS_REG_h08.reg_pcm_luma_bitdepth = 0x1,\
	.SPS_REG_h08.reg_pcm_chroma_bitdepth = 0x1,\
	.SPS_REG_h08.reg_pcm_min_cu_sz = 0x0,\
	.SPS_REG_h08.reg_pcm_ctu_sz = 0x0,\
	.SPS_REG_h10.reg_amp_flag = 0x0,\
	.SPS_REG_h10.reg_sao_flag = 0x0,\
	.SPS_REG_h10.reg_pcm_flag = 0x0,\
	.SPS_REG_h10.reg_sis_flag = 0x0,\
	.SPS_REG_h14.reg_pic_width_cu_m1 = 0x1,\
	.SPS_REG_h14.reg_pic_height_cu_m1 = 0x1,\
	.SPS_REG_h18.reg_pic_width_ctu_m1 = 0x1,\
	.SPS_REG_h18.reg_pic_height_ctu_m1 = 0x1,\
	.PPS_REG_h80.reg_dependent_slice_flag = 0x0,\
	.PPS_REG_h80.reg_signed_hiding_flag = 0x0,\
	.PPS_REG_h80.reg_tile_flag = 0x0,\
	.PPS_REG_h80.reg_tmp_mvp_flag = 0x0,\
	.PPS_REG_h80.reg_weight_pred_flag = 0x0,\
	.PPS_REG_h80.reg_weight_bipred_flag = 0x0,\
	.PPS_REG_h80.reg_use_integer_mv = 0x0,\
	.PPS_REG_h80.reg_trans_bypass_flag = 0x0,\
	.PPS_REG_h80.reg_entropy_sync_flag = 0x0,\
	.PPS_REG_h80.reg_cons_intra_pred = 0x0,\
	.PPS_REG_h80.reg_parallel_mgr_lvl_m2 = 0x0,\
	.PPS_REG_h80.reg_avc_entropy_coding = 0x1,\
	.PPS_REG_h84.reg_cur_poc = 0x0,\
	.PPS_REG_h88.reg_tile_col_m1 = 0x0,\
	.PPS_REG_h88.reg_tile_row_m1 = 0x0,\
	.PPS_REG_h8C.reg_prg_tileid_x = 0x0,\
	.PPS_REG_h8C.reg_prg_tileid_y = 0x0,\
	.PPS_REG_h8C.reg_prg_tileid_rd_sel = 0x0,\
	.PPS_REG_h8C.reg_prg_tileid_rw = 0x0,\
	.PPS_REG_h8C.reg_prg_tileid_go = 0x0,\
	.PPS_REG_h90.reg_prg_tileid_xst = 0x0,\
	.PPS_REG_h90.reg_prg_tileid_xend = 0x0,\
	.PPS_REG_h94.reg_prg_tileid_yst = 0x0,\
	.PPS_REG_h94.reg_prg_tileid_yend = 0x0,\
	.SLICE_REG_h100.reg_slice_go = 0x0,\
	.SLICE_REG_h100.reg_cabac_init_go = 0x0,\
	.SLICE_REG_h100.reg_slice_done = 0x0,\
	.SLICE_REG_h104.reg_i_slice = 0x0,\
	.SLICE_REG_h104.reg_p_slice = 0x0,\
	.SLICE_REG_h104.reg_b_slice = 0x0,\
	.SLICE_REG_h104.reg_cabac_init_type = 0x0,\
	.SLICE_REG_h104.reg_col_l0_flag = 0x0,\
	.SLICE_REG_h104.reg_wb_mv_flag = 0x1,\
	.SLICE_REG_h104.reg_mvd_l1_zero_flag = 0x0,\
	.SLICE_REG_h104.reg_col_buf_ridx = 0x0,\
	.SLICE_REG_h104.reg_col_buf_widx = 0x0,\
	.SLICE_REG_h108.reg_num_ref_l0_act_m1 = 0x0,\
	.SLICE_REG_h108.reg_num_ref_l1_act_m1 = 0x0,\
	.SLICE_REG_h108.reg_col_ref_idx = 0x0,\
	.SLICE_REG_h108.reg_max_merge_cand_m1 = 0x0,\
	.SLICE_REG_h10C.reg_prg_ref_list_go = 0x0,\
	.SLICE_REG_h10C.reg_prg_ref_list_idx = 0x0,\
	.SLICE_REG_h10C.reg_prg_ref_list_rw = 0x0,\
	.SLICE_REG_h10C.reg_prg_ref_list_addr = 0x0,\
	.SLICE_REG_h110.reg_prg_ref_list_wd = 0x0,\
	.SLICE_REG_h114.reg_prg_ref_list_rd = 0x0,\
	.SLICE_REG_h140.reg_slice_qp = 0x0,\
	.SLICE_REG_h140.reg_slice_cb_qp_ofs = 0x0,\
	.SLICE_REG_h140.reg_slice_cr_qp_ofs = 0x0,\
	.SLICE_REG_h144.reg_prg_scl_list_go = 0x0,\
	.SLICE_REG_h144.reg_prg_scl_list_rw = 0x0,\
	.SLICE_REG_h144.reg_prg_scl_list_addr = 0x0,\
	.SLICE_REG_h148.reg_prg_scl_list_wd = 0x0,\
	.SLICE_REG_h14C.reg_prg_scl_list_rd = 0x0,\
	.SLICE_REG_h150.reg_slice_ilf_dis = 0x0,\
	.SLICE_REG_h150.reg_slice_ilf_cross_dis = 0x0,\
	.SLICE_REG_h150.reg_slice_beta_ofs_div2 = 0x0,\
	.SLICE_REG_h150.reg_slice_tc_ofs_div2 = 0x0,\
	.SLICE_REG_h150.reg_sao_luma_flag = 0x0,\
	.SLICE_REG_h150.reg_sao_chroma_flag = 0x0,\
	.SLICE_REG_h160.reg_ctu_st_x = 0x0,\
	.SLICE_REG_h160.reg_ctu_st_y = 0x0,\
	.SLICE_REG_h164.reg_ctu_end_x = 0x0,\
	.SLICE_REG_h164.reg_ctu_end_y = 0x0,\
};

void init_vc_hdr_reg(VC_HDR_C &hdr)
{
    hdr.SPS_REG_h00.reg_min_cu_sz = 0x0;
    hdr.SPS_REG_h00.reg_ctu_sz = 0x0;
    hdr.SPS_REG_h00.reg_min_tu_sz = 0x0;
    hdr.SPS_REG_h00.reg_max_tu_sz = 0x0;
    hdr.SPS_REG_h00.reg_max_tu_dep_inter_m1 = 0x0;
    hdr.SPS_REG_h00.reg_max_tu_dep_intra_m1 = 0x0;
    hdr.SPS_REG_h04.reg_luma_bitdepth = 0x1;
    hdr.SPS_REG_h04.reg_chroma_bitdepth = 0x1;
    hdr.SPS_REG_h08.reg_pcm_luma_bitdepth = 0x1;
    hdr.SPS_REG_h08.reg_pcm_chroma_bitdepth = 0x1;
    hdr.SPS_REG_h08.reg_pcm_min_cu_sz = 0x0;
    hdr.SPS_REG_h08.reg_pcm_ctu_sz = 0x0;
    hdr.SPS_REG_h10.reg_amp_flag = 0x0;
    hdr.SPS_REG_h10.reg_sao_flag = 0x0;
    hdr.SPS_REG_h10.reg_pcm_flag = 0x0;
    hdr.SPS_REG_h10.reg_sis_flag = 0x0;
    hdr.SPS_REG_h14.reg_pic_width_cu_m1 = 0x1;
    hdr.SPS_REG_h14.reg_pic_height_cu_m1 = 0x1;
    hdr.SPS_REG_h18.reg_pic_width_ctu_m1 = 0x1;
    hdr.SPS_REG_h18.reg_pic_height_ctu_m1 = 0x1;
    hdr.PPS_REG_h80.reg_dependent_slice_flag = 0x0;
    hdr.PPS_REG_h80.reg_signed_hiding_flag = 0x0;
    hdr.PPS_REG_h80.reg_tile_flag = 0x0;
    hdr.PPS_REG_h80.reg_tmp_mvp_flag = 0x0;
    hdr.PPS_REG_h80.reg_weight_pred_flag = 0x0;
    hdr.PPS_REG_h80.reg_weight_bipred_flag = 0x0;
    hdr.PPS_REG_h80.reg_use_integer_mv = 0x0;
    hdr.PPS_REG_h80.reg_trans_bypass_flag = 0x0;
    hdr.PPS_REG_h80.reg_entropy_sync_flag = 0x0;
    hdr.PPS_REG_h80.reg_cons_intra_pred = 0x0;
    hdr.PPS_REG_h80.reg_parallel_mgr_lvl_m2 = 0x0;
    hdr.PPS_REG_h80.reg_avc_entropy_coding = 0x1;
    hdr.PPS_REG_h84.reg_cur_poc = 0x0;
    hdr.PPS_REG_h88.reg_tile_col_m1 = 0x0;
    hdr.PPS_REG_h88.reg_tile_row_m1 = 0x0;
    hdr.PPS_REG_h8C.reg_prg_tileid_x = 0x0;
    hdr.PPS_REG_h8C.reg_prg_tileid_y = 0x0;
    hdr.PPS_REG_h8C.reg_prg_tileid_rd_sel = 0x0;
    hdr.PPS_REG_h8C.reg_prg_tileid_rw = 0x0;
    hdr.PPS_REG_h8C.reg_prg_tileid_go = 0x0;
    hdr.PPS_REG_h90.reg_prg_tileid_xst = 0x0;
    hdr.PPS_REG_h90.reg_prg_tileid_xend = 0x0;
    hdr.PPS_REG_h94.reg_prg_tileid_yst = 0x0;
    hdr.PPS_REG_h94.reg_prg_tileid_yend = 0x0;
    hdr.SLICE_REG_h100.reg_slice_go = 0x0;
    hdr.SLICE_REG_h100.reg_cabac_init_go = 0x0;
    hdr.SLICE_REG_h100.reg_slice_done = 0x0;
    hdr.SLICE_REG_h104.reg_i_slice = 0x0;
    hdr.SLICE_REG_h104.reg_p_slice = 0x0;
    hdr.SLICE_REG_h104.reg_b_slice = 0x0;
    hdr.SLICE_REG_h104.reg_cabac_init_type = 0x0;
    hdr.SLICE_REG_h104.reg_col_l0_flag = 0x0;
    hdr.SLICE_REG_h104.reg_wb_mv_flag = 0x1;
    hdr.SLICE_REG_h104.reg_mvd_l1_zero_flag = 0x0;
    hdr.SLICE_REG_h104.reg_col_buf_ridx = 0x0;
    hdr.SLICE_REG_h104.reg_col_buf_widx = 0x0;
    hdr.SLICE_REG_h108.reg_num_ref_l0_act_m1 = 0x0;
    hdr.SLICE_REG_h108.reg_num_ref_l1_act_m1 = 0x0;
    hdr.SLICE_REG_h108.reg_col_ref_idx = 0x0;
    hdr.SLICE_REG_h108.reg_max_merge_cand_m1 = 0x0;
    hdr.SLICE_REG_h10C.reg_prg_ref_list_go = 0x0;
    hdr.SLICE_REG_h10C.reg_prg_ref_list_idx = 0x0;
    hdr.SLICE_REG_h10C.reg_prg_ref_list_rw = 0x0;
    hdr.SLICE_REG_h10C.reg_prg_ref_list_addr = 0x0;
    hdr.SLICE_REG_h110.reg_prg_ref_list_wd = 0x0;
    hdr.SLICE_REG_h114.reg_prg_ref_list_rd = 0x0;
    hdr.SLICE_REG_h140.reg_slice_qp = 0x0;
    hdr.SLICE_REG_h140.reg_slice_cb_qp_ofs = 0x0;
    hdr.SLICE_REG_h140.reg_slice_cr_qp_ofs = 0x0;
    hdr.SLICE_REG_h144.reg_prg_scl_list_go = 0x0;
    hdr.SLICE_REG_h144.reg_prg_scl_list_rw = 0x0;
    hdr.SLICE_REG_h144.reg_prg_scl_list_addr = 0x0;
    hdr.SLICE_REG_h148.reg_prg_scl_list_wd = 0x0;
    hdr.SLICE_REG_h14C.reg_prg_scl_list_rd = 0x0;
    hdr.SLICE_REG_h150.reg_slice_ilf_dis = 0x0;
    hdr.SLICE_REG_h150.reg_slice_ilf_cross_dis = 0x0;
    hdr.SLICE_REG_h150.reg_slice_beta_ofs_div2 = 0x0;
    hdr.SLICE_REG_h150.reg_slice_tc_ofs_div2 = 0x0;
    hdr.SLICE_REG_h150.reg_sao_luma_flag = 0x0;
    hdr.SLICE_REG_h150.reg_sao_chroma_flag = 0x0;
    hdr.SLICE_REG_h160.reg_ctu_st_x = 0x0;
    hdr.SLICE_REG_h160.reg_ctu_st_y = 0x0;
    hdr.SLICE_REG_h164.reg_ctu_end_x = 0x0;
    hdr.SLICE_REG_h164.reg_ctu_end_y = 0x0;
};

#endif //__REG_VC_HDR_STRUCT_H__
