// $Module: vc_mm $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 28 Jul 2021 07:43:01 PM $
//

#ifndef __REG_VC_MM_STRUCT_H__
#define __REG_VC_MM_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*HW Encoder information
		b0: Support AVC Encoder
		b1: Support HEVC Encoder
		b2: Support VVC Encoder;*/
		uint32_t reg_hw_enc_info:32;
	};
	uint32_t val;
} VC_MM_VC_MM_h0_C;
typedef union {
	struct {
		/*HW Decoder information
		b0: Support AVC Decoder
		b1: Support HEVC Decoder
		b2: Support VVC Decoder;*/
		uint32_t reg_hw_dec_info:32;
	};
	uint32_t val;
} VC_MM_VC_MM_h4_C;
typedef union {
	struct {
		/*HW Peripheral information
		b0: Support internal Line Buffer
		b1: Support Frame Buffer Codec;*/
		uint32_t reg_hw_peri_info:32;
	};
	uint32_t val;
} VC_MM_VC_MM_h8_C;
typedef union {
	struct {
		/*AVC Mode*/
		uint32_t reg_avc_mode : 1;
		uint32_t rsv_1_15 : 15;
		uint32_t reg_vbc_ena : 1;
		uint32_t reg_src_vbc_ena : 1;
		uint32_t rsv_17_31 : 14;
	};
	uint32_t val;
} VC_MM_VC_MM_h10_C;
typedef union {
	struct {
		/*AVC Decoder;*/
		uint32_t reg_avc_dec:1;
		/*HEVC Decoder;*/
		uint32_t reg_hevc_dec:1;
		/*VVC Decoder;*/
		uint32_t reg_vvc_dec:1;
	};
	uint32_t val;
} VC_MM_VC_MM_h14_C;
typedef union {
	struct {
		/*Bounding Information;*/
		uint32_t reg_bonding_info:32;
	};
	uint32_t val;
} VC_MM_VC_MM_h40_C;
typedef union {
	struct {
		/*Timestamp Low word;*/
		uint32_t reg_timestamp_lsb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_h44_C;
typedef union {
	struct {
		/*Timestamp high word;*/
		uint32_t reg_timestamp_msb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_h48_C;
typedef union {
	struct {
		/*Source Format
		0: I420
		1: NV12
		2: NV21
		3: Reserved
		4: Reserved
		5: Tile32x32
		6: Tile32x16
		7: Tile16x32;*/
		uint32_t reg_src_fmt:3;
		uint32_t rsv_3_7:5;
		/*Source Image PADDING enable;*/
		uint32_t reg_src_padding_en:1;
		uint32_t rsv_9_15:7;
		/*Write Back Slice Data Header enable;*/
		uint32_t reg_bs_slice_hdr_en:1;
		uint32_t rsv_17_19:3;
		/*"NV12 source rotation setting
		0: rotate 0
		1: rotate 180
		2: rotate 270
		3: rotate 90" */
		uint32_t reg_src_rotate:2;
		uint32_t reg_src_h_mirror:1; //NV12 source horizontal mirror (reg_src_rotate must be 0)
		uint32_t reg_src_v_mirror:1; //NV12 source vertical flip (reg_src_rotate must be 0)
		uint32_t reg_src_rgb_ena:1;//Source format is RGB
		/*"RGB format
		0: XRGB8888
		1: XBGR8888
		2: RGBX8888
		3: BGRX8888"*/
		uint32_t reg_src_rgb_mode:2;
		uint32_t rsv_27:1;
		/*"RGB2YUV formula selection
		0: BT601  (rgb=[0,255]) -> (y=[16,235], uv=[16,240])
		1: BT601  (rgb=[0,255]) -> (y=[0,255], uv=[0,255])
		2:  BT601  (rgb=[0,219]) -> (y=[16,235], uv=[16,240])
		3: BT601  (rgb=[16,235]) -> (y=[16,235], uv=[18.5,237.5])
		4: BT709  (rgb=[0,255]) -> (y=[16,235], uv=[16,240])
		5. BT709  (rgb=[0,255]) -> (y=[0,255], uv=[0,255])
		6. BT709  (rgb=[16,235]) -> (y=[16,235], uv=[18.5,237.5])"*/
		uint32_t reg_src_rgb2yuv_sel:3;
		uint32_t rsv_31:1;
	};
	uint32_t val;
} VC_MM_VC_MM_h80_C;
typedef union {
	struct {
		/*Source Image LUMA PADDING;*/
		uint32_t reg_src_luma_pad_val:8;
		uint32_t rsv_8_15:8;
		/*Source Image CHROMA PADDING;*/
		uint32_t reg_src_chroma_pad_val:8;
	};
	uint32_t val;
} VC_MM_VC_MM_h84_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*Source Luma Base address (16byte);*/
		uint32_t reg_src_luma_base_lsb:28;
	};
	uint32_t val;
} VC_MM_VC_MM_h88_C;

typedef union {
	struct {
		uint32_t reg_src_luma_base_msb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_h8C_C;

typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*Source Chroma/CB Base address (16byte);*/
		uint32_t reg_src_cb_base_lsb:28;
	};
	uint32_t val;
} VC_MM_VC_MM_h90_C;

typedef union {
	struct {
		/*Source Chroma/CB Base address (16byte);*/
		uint32_t reg_src_cb_base_msb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_h94_C;

typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*Source Cr Base address (16byte);*/
		uint32_t reg_src_cr_base_lsb:28;
	};
	uint32_t val;
} VC_MM_VC_MM_h98_C;

typedef union {
	struct {
		/*Source Cr Base address (16byte);*/
		uint32_t reg_src_cr_base_msb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_h9C_C;

typedef union {
	struct {
		/*Source Width minus 1, 1 pixel alignment;*/
		uint32_t reg_src_width_m1:14;
		uint32_t rsv_14_15:2;
		/*Source Height minus 1, 1 pixel alignment;*/
		uint32_t reg_src_height_m1:14;
	};
	uint32_t val;
} VC_MM_VC_MM_hA0_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*Source Luma Pitch (16byte);*/
		uint32_t reg_src_luma_pit:12;
		uint32_t rsv_16_19:4;
		/*Source Chroma Pitch (16byte);*/
		uint32_t reg_src_chroma_pit:10;
		uint32_t rsv_30_31:2;
	};
	uint32_t val;
} VC_MM_VC_MM_hA4_C;
typedef union {
	struct {
		uint32_t rsv_0_2:3;
		/*Source Slice Start X align 8pixels;*/
		uint32_t reg_src_st_x8:11;
		uint32_t rsv_14_18:5;
		/*Source Slice Start Y align 8pixels;*/
		uint32_t reg_src_st_y8:11;
	};
	uint32_t val;
} VC_MM_VC_MM_hA8_C;
typedef union {
	struct {
		uint32_t rsv_0_2:3;
		/*Source Slice End X align 8pixels;*/
		uint32_t reg_src_end_x8:11;
		uint32_t rsv_14_18:5;
		/*Source Slice End X align 8pixels;*/
		uint32_t reg_src_end_y8:11;
	};
	uint32_t val;
} VC_MM_VC_MM_hAC_C;
typedef union {
	struct {
		/*Start to read SRC data for encode;*/
		uint32_t reg_read_src_go:1;
		uint32_t rsv_1_29:29;
		/*Read SRC slice done;*/
		uint32_t reg_read_src_done:1;
		/*Read SRC frame idle;*/
		uint32_t reg_read_src_idle:1;
	};
	uint32_t val;
} VC_MM_VC_MM_hB0_C;
typedef union {
	struct {
		/*NAL Base address (byte) for decoder;*/
		uint32_t reg_bs_in_base:32;
	};
	uint32_t val;
} VC_MM_VC_MM_hC0_C;
typedef union {
	struct {
		/*NAL size (byte), max. 1GB for decoder;*/
		uint32_t reg_bs_in_sz:30;
		uint32_t rsv_30_30:1;
		/*Start to read NAL data;*/
		uint32_t reg_bs_in_go:1;
	};
	uint32_t val;
} VC_MM_VC_MM_hC8_C;
typedef union {
	struct {
		/*Bistream Buffer Base low address (byte) for encoder;*/
		uint32_t reg_bs_out_base_lsb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_hD0_C;
typedef union {
	struct {
		/*Bistream Buffer Base high address (byte) for encoder;*/
		uint32_t reg_bs_out_base_msb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_hD4_C;
typedef union {
	struct {
		uint32_t rsv_0_3:4;
		/*Bistream Buffer Base upper low address (16byte) for encoder;*/
		uint32_t reg_bs_out_ubase_lsb:28;
	};
	uint32_t val;
} VC_MM_VC_MM_hD8_C;
typedef union {
	struct {
		/*Bistream Buffer Base upper high address (16byte) for encoder;*/
		uint32_t reg_bs_out_ubase_msb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_hDC_C;
typedef union {
	struct {
		/*Slice Data Header, max 16bytes;*/
		uint32_t reg_bs_slice_hdr_dat[4];
	};
	uint32_t val[4];
} VC_MM_VC_MM_hE0_C;
typedef union {
	struct {
		/*Bitstream output buffer overflow mechanism
		0: wrap around and overwrite from beginning.
		1: discard and no-more wirte to output buffer.
		2: Continue to write back bitstream and address is still counting.
		3: Stall Encoder and interrupt to wait SW trigger
		4. Encoder early termination.;*/
		uint32_t reg_bs_out_ovf_mode:3;
		uint32_t rsv_3_7:5;
		/*SW trigger when HW Stall;*/
		uint32_t reg_bs_ovf_sw_trig:1;
		uint32_t rsv_9_30:22;
		/*Bitstream overflow flag;*/
		uint32_t reg_bs_ovf_flag:1;
	};
	uint32_t val;
} VC_MM_VC_MM_hF0_C;
typedef union {
	struct {
		/*Bistream space low base low address (byte) for encoder;*/
		uint32_t reg_bs_space_lbase_lsb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_hF4_C;
typedef union {
	struct {
		/*Bistream space low base high address (byte) for encoder;*/
		uint32_t reg_bs_space_lbase_msb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_hF8_C;
typedef union {
	struct {
		/*Bistream space high base low address (byte) for encoder;*/
		uint32_t reg_bs_space_ubase_lsb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_hFC_C;
typedef union {
	struct {
		/*Bistream space high base high address (byte) for encoder;*/
		uint32_t reg_bs_space_ubase_msb:32;
	};
	uint32_t val;
} VC_MM_VC_MM_h100_C;
typedef struct {
	volatile VC_MM_VC_MM_h0_C VC_MM_h0;
	volatile VC_MM_VC_MM_h4_C VC_MM_h4;
	volatile VC_MM_VC_MM_h8_C VC_MM_h8;
	volatile VC_MM_VC_MM_h10_C VC_MM_h10;
	volatile VC_MM_VC_MM_h14_C VC_MM_h14;
	volatile VC_MM_VC_MM_h40_C VC_MM_h40;
	volatile VC_MM_VC_MM_h44_C VC_MM_h44;
	volatile VC_MM_VC_MM_h48_C VC_MM_h48;
	volatile VC_MM_VC_MM_h80_C VC_MM_h80;
	volatile VC_MM_VC_MM_h84_C VC_MM_h84;
	volatile VC_MM_VC_MM_h88_C VC_MM_h88;
	volatile VC_MM_VC_MM_h8C_C VC_MM_h8C;
	volatile VC_MM_VC_MM_h90_C VC_MM_h90;
	volatile VC_MM_VC_MM_h94_C VC_MM_h94;
	volatile VC_MM_VC_MM_h98_C VC_MM_h98;
	volatile VC_MM_VC_MM_h9C_C VC_MM_h9C;
	volatile VC_MM_VC_MM_hA0_C VC_MM_hA0;
	volatile VC_MM_VC_MM_hA4_C VC_MM_hA4;
	volatile VC_MM_VC_MM_hA8_C VC_MM_hA8;
	volatile VC_MM_VC_MM_hAC_C VC_MM_hAC;
	volatile VC_MM_VC_MM_hB0_C VC_MM_hB0;
	volatile VC_MM_VC_MM_hC0_C VC_MM_hC0;
	volatile VC_MM_VC_MM_hC8_C VC_MM_hC8;
	volatile VC_MM_VC_MM_hD0_C VC_MM_hD0;
	volatile VC_MM_VC_MM_hD4_C VC_MM_hD4;
	volatile VC_MM_VC_MM_hD8_C VC_MM_hD8;
	volatile VC_MM_VC_MM_hDC_C VC_MM_hDC;
	volatile VC_MM_VC_MM_hE0_C VC_MM_hE0;
	volatile VC_MM_VC_MM_hF0_C VC_MM_hF0;
	volatile VC_MM_VC_MM_hF4_C VC_MM_hF4;
	volatile VC_MM_VC_MM_hF8_C VC_MM_hF8;
	volatile VC_MM_VC_MM_hFC_C VC_MM_hFC;
	volatile VC_MM_VC_MM_h100_C VC_MM_h100;
} VC_MM_C;
#define DEFINE_VC_MM_C(X) \
	 VC_MM_C X = \
{\
	.VC_MM_h0.reg_hw_enc_info = 0x0,\
	.VC_MM_h4.reg_hw_dec_info = 0x0,\
	.VC_MM_h8.reg_hw_peri_info = 0x0,\
	.VC_MM_h10.reg_avc_mode = 0x0,\
	.VC_MM_h10.reg_vbc_ena = 0x0,\
	.VC_MM_h10.reg_src_vbc_ena = 0x0,\
	.VC_MM_h14.reg_avc_dec = 0x0,\
	.VC_MM_h14.reg_hevc_dec = 0x0,\
	.VC_MM_h14.reg_vvc_dec = 0x0,\
	.VC_MM_h40.reg_bonding_info = 0x0,\
	.VC_MM_h44.reg_timestamp_lsb = 0x0,\
	.VC_MM_h48.reg_timestamp_msb = 0x0,\
	.VC_MM_h80.reg_src_fmt = 0x0,\
	.VC_MM_h80.reg_src_padding_en = 0x1,\
	.VC_MM_h80.reg_bs_slice_hdr_en = 0x1,\
	.VC_MM_h80.reg_src_rotate = 0x0, \
	.VC_MM_h80.reg_src_h_mirror = 0x0, \
	.VC_MM_h80.reg_src_v_mirror = 0x0, \
	.VC_MM_h80.reg_src_rgb_ena = 0x0, \
	.VC_MM_h84.reg_src_luma_pad_val = 0x0,\
	.VC_MM_h84.reg_src_chroma_pad_val = 0x0,\
	.VC_MM_h88.reg_src_luma_base_lsb = 0x0,\
	.VC_MM_h8C.reg_src_luma_base_msb = 0x0,\
	.VC_MM_h90.reg_src_cb_base_lsb = 0x0,\
	.VC_MM_h94.reg_src_cb_base_msb = 0x0,\
	.VC_MM_h98.reg_src_cr_base_lsb = 0x0,\
	.VC_MM_h9C.reg_src_cr_base_msb = 0x0,\
	.VC_MM_hA0.reg_src_width_m1 = 0x0,\
	.VC_MM_hA0.reg_src_height_m1 = 0x0,\
	.VC_MM_hA4.reg_src_luma_pit = 0x0,\
	.VC_MM_hA4.reg_src_chroma_pit = 0x0,\
	.VC_MM_hA8.reg_src_st_x8 = 0x0,\
	.VC_MM_hA8.reg_src_st_y8 = 0x0,\
	.VC_MM_hAC.reg_src_end_x8 = 0x0,\
	.VC_MM_hAC.reg_src_end_y8 = 0x0,\
	.VC_MM_hB0.reg_read_src_go = 0x0,\
	.VC_MM_hB0.reg_read_src_done = 0x0,\
	.VC_MM_hB0.reg_read_src_idle = 0x0,\
	.VC_MM_hC0.reg_bs_in_base = 0x0,\
	.VC_MM_hC8.reg_bs_in_sz = 0x0,\
	.VC_MM_hC8.reg_bs_in_go = 0x0,\
	.VC_MM_hD0.reg_bs_out_base_lsb = 0x0,\
	.VC_MM_hD4.reg_bs_out_base_msb = 0x0,\
	.VC_MM_hD8.reg_bs_out_ubase_lsb = 0x0,\
	.VC_MM_hDC.reg_bs_out_ubase_msb = 0x0,\
	.VC_MM_hE0.reg_bs_slice_hdr_dat = 0x0,\
	.VC_MM_hF0.reg_bs_out_ovf_mode = 0x0,\
	.VC_MM_hF0.reg_bs_ovf_sw_trig = 0x0,\
	.VC_MM_hF4.reg_bs_space_lbase_lsb = 0x0,\
	.VC_MM_hF8.reg_bs_space_lbase_msb = 0x0,\
	.VC_MM_hFC.reg_bs_space_ubase_lsb = 0xFFFFFFFF,\
	.VC_MM_h100.reg_bs_space_ubase_msb = 0xFFFFFFFF,\
};

void init_vc_mm_reg(VC_MM_C &mm)
{
    mm.VC_MM_h0.reg_hw_enc_info = 0x0;
    mm.VC_MM_h4.reg_hw_dec_info = 0x0;
    mm.VC_MM_h8.reg_hw_peri_info = 0x0;
    mm.VC_MM_h10.reg_avc_mode = 0x0;
    mm.VC_MM_h10.reg_vbc_ena = 0x0;
    mm.VC_MM_h14.reg_avc_dec = 0x0;
    mm.VC_MM_h14.reg_hevc_dec = 0x0;
    mm.VC_MM_h14.reg_vvc_dec = 0x0;
    mm.VC_MM_h40.reg_bonding_info = 0x0;
    mm.VC_MM_h44.reg_timestamp_lsb = 0x0;
    mm.VC_MM_h48.reg_timestamp_msb = 0x0;
    mm.VC_MM_h80.reg_src_fmt = 0x0;
    mm.VC_MM_h80.reg_src_padding_en = 0x1;
    mm.VC_MM_h80.reg_bs_slice_hdr_en = 0x1;
    mm.VC_MM_h80.reg_src_rotate = 0x0;
    mm.VC_MM_h80.reg_src_h_mirror = 0x0;
    mm.VC_MM_h80.reg_src_v_mirror = 0x0;
    mm.VC_MM_h80.reg_src_rgb_ena = 0x0;
    mm.VC_MM_h84.reg_src_luma_pad_val = 0x0;
    mm.VC_MM_h84.reg_src_chroma_pad_val = 0x0;
    mm.VC_MM_h88.reg_src_luma_base_lsb = 0x0;
    mm.VC_MM_h8C.reg_src_luma_base_msb = 0x0;
    mm.VC_MM_h90.reg_src_cb_base_lsb = 0x0;
    mm.VC_MM_h94.reg_src_cb_base_msb = 0x0;
    mm.VC_MM_h98.reg_src_cr_base_lsb = 0x0;
    mm.VC_MM_h9C.reg_src_cr_base_msb = 0x0;
    mm.VC_MM_hA0.reg_src_width_m1 = 0x0;
    mm.VC_MM_hA0.reg_src_height_m1 = 0x0;
    mm.VC_MM_hA4.reg_src_luma_pit = 0x0;
    mm.VC_MM_hA4.reg_src_chroma_pit = 0x0;
    mm.VC_MM_hA8.reg_src_st_x8 = 0x0;
    mm.VC_MM_hA8.reg_src_st_y8 = 0x0;
    mm.VC_MM_hAC.reg_src_end_x8 = 0x0;
    mm.VC_MM_hAC.reg_src_end_y8 = 0x0;
    mm.VC_MM_hB0.reg_read_src_go = 0x0;
    mm.VC_MM_hB0.reg_read_src_done = 0x0;
    mm.VC_MM_hB0.reg_read_src_idle = 0x0;
    mm.VC_MM_hC0.reg_bs_in_base = 0x0;
    mm.VC_MM_hC8.reg_bs_in_sz = 0x0;
    mm.VC_MM_hC8.reg_bs_in_go = 0x0;
    mm.VC_MM_hD0.reg_bs_out_base_lsb = 0x0;
    mm.VC_MM_hD4.reg_bs_out_base_msb = 0x0;
    mm.VC_MM_hD8.reg_bs_out_ubase_lsb = 0x0;
    mm.VC_MM_hDC.reg_bs_out_ubase_msb = 0x0;
    mm.VC_MM_hE0.reg_bs_slice_hdr_dat[0] = 0x0;
    mm.VC_MM_hE0.reg_bs_slice_hdr_dat[1] = 0x0;
    mm.VC_MM_hE0.reg_bs_slice_hdr_dat[2] = 0x0;
    mm.VC_MM_hE0.reg_bs_slice_hdr_dat[3] = 0x0;
    mm.VC_MM_hF0.reg_bs_out_ovf_mode = 0x0;
    mm.VC_MM_hF0.reg_bs_ovf_sw_trig = 0x0;
    mm.VC_MM_hF4.reg_bs_space_lbase_lsb = 0x0;
    mm.VC_MM_hF8.reg_bs_space_lbase_msb = 0x0;
    mm.VC_MM_hFC.reg_bs_space_ubase_lsb = 0xFFFFFFFF;
    mm.VC_MM_h100.reg_bs_space_ubase_msb = 0xFFFFFFFF;
};
#endif //__REG_VC_MM_STRUCT_H__
