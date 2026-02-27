
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <memory.h>
#include "TComTrQuant.h"
#include "TComTU.h"
#include "cvi_ime.h"
#include "cvi_pattern.h"
#include "cvi_rdo_bit_est.h"
#include "cvi_cu_ctrl.h"
#include "cvi_rate_ctrl.h"
#include "cvi_motion.h"
#include "cvi_frm_buf_mgr.h"
#include "cvi_vbc.h"

static void get_madi_blk32(int blk32_x, int blk32_y, int madi_16[], int lum_16[] = NULL, int madi_8[] = NULL, int lum_8[] = NULL)
{
    int blk16_id = 0;
    int blk8_id = 0;
    for (int blk16_y = 0; blk16_y < 32; blk16_y += 16)
    {
        for (int blk16_x = 0; blk16_x < 32; blk16_x += 16)
        {
            int pos_x = blk32_x + blk16_x;
            int pos_y = blk32_y + blk16_y;
            madi_16[blk16_id] = g_blk_madi.get_data(pos_x, pos_y);
            if (lum_16)
                lum_16[blk16_id]  = g_blk_lum.get_data(pos_x, pos_y);
            blk16_id++;
            if (madi_8 || lum_8) {
                for (int blk8_y = 0; blk8_y < 16; blk8_y += 8)
                {
                    for (int blk8_x = 0; blk8_x < 16; blk8_x += 8)
                    {
                        pos_x = blk32_x + blk16_x + blk8_x;
                        pos_y = blk32_y + blk16_y + blk8_y;
                        if (madi_8)
                            madi_8[blk8_id] = g_blk8_madi.get_data(pos_x, pos_y);
                        if (lum_8)
                            lum_8[blk8_id]  = g_blk8_lum.get_data(pos_x, pos_y);
                        blk8_id++;
                    }
                }
            }
        }
    }
}

#ifdef SIG_RRU
void sig_rru_copy_intp_temp_buf(ComponentID compID, short *p_src, int src_stride, short *p_dst, int dst_stride, int size)
{
    short *p_src_buf = p_src;
    short *p_dst_buf = p_dst;

    if (compID == COMPONENT_Y)
    {
        int byte_size = sizeof(short) * size;
        for (int i = 0; i < size; i++)
        {
            memcpy(p_dst_buf, p_src_buf, byte_size);
            p_src_buf += src_stride;
            p_dst_buf += dst_stride;
        }
    }
    else
    {
        if (compID == COMPONENT_Cr)
            p_dst_buf++;

        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                *p_dst_buf = p_src_buf[i * src_stride + j];
                p_dst_buf += 2;
            }
        }
    }
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_output_intp(bool is_intra, int luma_pu_size, bool is_chroma)
{
    if (is_intra)
    {
        int pu_size = is_chroma ? (luma_pu_size == 16 ? 8 : 4) : luma_pu_size;
        int ctx_idx = sig_pat_blk_size((luma_pu_size << 1));
        sig_ctx *p_ctx = &g_sigpool.rru_i_ctx[ctx_idx];

        int count = (pu_size * pu_size) << (is_chroma ? 1 : 0);
        for (int i = 0; i < count; i++)
        {
            unsigned char temp = (g_sigpool.p_rru_intp_final[i] & 0xff);
            sigdump_output_bin(p_ctx, &temp, 1);
        }
    }
    else    // inter
    {
        int pu_size = is_chroma ? (luma_pu_size >> 1) : luma_pu_size;
        int ctx_idx = sig_pat_blk_size(luma_pu_size);
        sig_ctx *p_ctx = &g_sigpool.rru_pu_ctx[ctx_idx];

        if (luma_pu_size < 32)
        {
            int count = (pu_size * pu_size) << (is_chroma ? 1 : 0);
            for (int i = 0; i < count; i++)
            {
                unsigned char temp = (g_sigpool.p_rru_intp_final[i] & 0xff);
                sigdump_output_bin(p_ctx, &temp, 1);
            }
        }
        else
        {
            // Luma : 16x16block x4, Chroma(cbcr interleave) : 16x8block x4
            int tile_h = 16;
            int tile_w = 16;
            int pu_size_w = 32; // stride of p_rru_intp_final

            if (is_chroma)
                tile_h = 8;

            for (int i = 0; i < pu_size; i += tile_h)
            {
                for (int j = 0; j < pu_size_w; j += tile_w)
                {
                    Pel *p_src = g_sigpool.p_rru_intp_final + i * pu_size_w + j;

                    for (int k = 0; k < tile_h; k++)
                    {
                        for (int w = 0; w < tile_w; w++)
                        {
                            unsigned char temp = (p_src[w] & 0xff);
                            sigdump_output_bin(p_ctx, &temp, 1);
                        }
                        p_src += pu_size_w;
                    }
                }
            }
        }
    }
}
#endif //~SIG_RRU

#ifdef SIG_RRU
#ifndef SIG_RRU_TXT
void sig_rru_output_dct_bin(bool is_intra, int luma_pu_size, bool is_chroma)
{
    if (is_intra)
    {
        int pu_size = is_chroma ? (luma_pu_size == 16 ? 8 : 4) : luma_pu_size;
        int ctx_idx = sig_pat_blk_size((luma_pu_size << 1));

        //dct - idct
        int comp_idx = is_chroma ? 1 : 0;
        sig_ctx *p_ctx_in  = &g_sigpool.rru_intra_dct_ctx[ctx_idx][0][comp_idx];
        sig_ctx *p_ctx_out = &g_sigpool.rru_intra_dct_ctx[ctx_idx][1][comp_idx];

        sig_ctx *p_ctx_in_idct  = &g_sigpool.rru_intra_idct_ctx[ctx_idx][0][comp_idx];
        sig_ctx *p_ctx_out_idct = &g_sigpool.rru_intra_idct_ctx[ctx_idx][1][comp_idx];

        int buf_idx = is_chroma ? 1 : 0;
        short *p_dct_in = g_sigpool.p_rru_dct_in_final[buf_idx];
        int *p_dct_out = g_sigpool.p_rru_dct_out_final[buf_idx];
        int *p_idct_in = g_sigpool.p_rru_idct_in_final[buf_idx];
        short *p_idct_out = g_sigpool.p_rru_idct_out_final[buf_idx];
        int pixel_count = pu_size * pu_size * (buf_idx + 1);

        // Intra DCT in and IDCT out, data type is short
        sigdump_output_bin(p_ctx_in,       (unsigned char *)(p_dct_in),   sizeof(short) * pixel_count);
        for (int i = 0; i < pixel_count; i++)
        {
            short hw_clip = Clip3(-256, 255, (int)(p_idct_out[i]));
            sigdump_output_bin(p_ctx_out_idct, (unsigned char *)(&hw_clip), 2);
        }

        // Intra DCT out and IDCT in, data type is int
        for (int i = 0; i < pixel_count; i++)
        {
            unsigned short temp = (*p_dct_out) & 0xffff;
            sigdump_output_bin(p_ctx_out, (unsigned char *)(&temp), sizeof(unsigned short));
            p_dct_out++;

            temp = (*p_idct_in) & 0xffff;
            sigdump_output_bin(p_ctx_in_idct, (unsigned char *)(&temp), sizeof(unsigned short));
            p_idct_in++;
        }
    }
    else    // Inter
    {
        int pu_size = is_chroma ? (luma_pu_size >> 1) : luma_pu_size;
        int ctx_idx = sig_pat_blk_size(luma_pu_size);

        // inter DCT - IDCT
        int comp_idx = is_chroma ? 1 : 0;
        sig_ctx *p_ctx_in  = &g_sigpool.rru_inter_dct_ctx[ctx_idx][0][comp_idx];
        sig_ctx *p_ctx_out = &g_sigpool.rru_inter_dct_ctx[ctx_idx][1][comp_idx];

        sig_ctx *p_ctx_in_idct  = &g_sigpool.rru_inter_idct_ctx[ctx_idx][0][comp_idx];
        sig_ctx *p_ctx_out_idct = &g_sigpool.rru_inter_idct_ctx[ctx_idx][1][comp_idx];

        short numbers = (luma_pu_size == 32) ? 4 : 1;
        sigdump_output_bin(p_ctx_in,       (unsigned char *)(&numbers), 2);
        sigdump_output_bin(p_ctx_out,      (unsigned char *)(&numbers), 2);
        sigdump_output_bin(p_ctx_in_idct,  (unsigned char *)(&numbers), 2);
        sigdump_output_bin(p_ctx_out_idct, (unsigned char *)(&numbers), 2);

        short is_i_slice = (g_sigpool.slice_type == I_SLICE) ? 1 : 0;
        sigdump_output_bin(p_ctx_in,       (unsigned char *)(&is_i_slice), 2);
        sigdump_output_bin(p_ctx_out,      (unsigned char *)(&is_i_slice), 2);
        sigdump_output_bin(p_ctx_in_idct,  (unsigned char *)(&is_i_slice), 2);
        sigdump_output_bin(p_ctx_out_idct, (unsigned char *)(&is_i_slice), 2);

        unsigned char temp[10] = { 0 };
        sigdump_output_bin(p_ctx_in,       (unsigned char *)(temp), 4);
        sigdump_output_bin(p_ctx_out,      (unsigned char *)(temp), 4);
        sigdump_output_bin(p_ctx_in_idct,  (unsigned char *)(temp), 4);
        sigdump_output_bin(p_ctx_out_idct, (unsigned char *)(temp), 4);

        for (int comp = 0; comp < 3; comp++)
        {
            short qp = g_sigpool.rru_qp[comp] & 0xffff;
            sigdump_output_bin(p_ctx_in,       (unsigned char *)(&qp), 2);
            sigdump_output_bin(p_ctx_out,      (unsigned char *)(&qp), 2);
            sigdump_output_bin(p_ctx_in_idct,  (unsigned char *)(&qp), 2);
            sigdump_output_bin(p_ctx_out_idct, (unsigned char *)(&qp), 2);
        }

        sigdump_output_bin(p_ctx_in,       (unsigned char *)(temp), 10);
        sigdump_output_bin(p_ctx_out,      (unsigned char *)(temp), 10);
        sigdump_output_bin(p_ctx_in_idct,  (unsigned char *)(temp), 10);
        sigdump_output_bin(p_ctx_out_idct, (unsigned char *)(temp), 10);

#ifdef SIG_RRU_DEBUG
        sig_ctx *p_ctx_iq_in = NULL;
        if (g_sigdump.rru_debug)
        {
            p_ctx_iq_in  = &g_sigpool.rru_inter_iq_in_ctx[ctx_idx][comp_idx];
            sigdump_output_bin(p_ctx_iq_in, (unsigned char *)(&numbers), 2);
            sigdump_output_bin(p_ctx_iq_in, (unsigned char *)(&is_i_slice), 2);
            sigdump_output_bin(p_ctx_iq_in, (unsigned char *)(temp), 4);

            for (int comp = 0; comp < 3; comp++)
            {
                short qp = g_sigpool.rru_qp[comp] & 0xffff;
                sigdump_output_bin(p_ctx_iq_in, (unsigned char *)(&qp), 2);
            }

            sigdump_output_bin(p_ctx_iq_in, (unsigned char *)(temp), 10);
        }
#endif //~SIG_RRU_DEBUG

        // DCT-IDCT data
        // CU32 : 16x16x4
        // CU16, CU8 : 2Nx2N
        if (luma_pu_size == 32)
        {
            short *p_dct_in = NULL;
            int *p_dct_out = NULL;
            int *p_idct_in = NULL;
            short *p_idct_out = NULL;
#ifdef SIG_RRU_DEBUG
            int *p_iq_in = NULL;
#endif

            if (!is_chroma)
            {
                for (int n = 0; n < 4; n++)
                {
                    int idx_x = (n & 0x1);
                    int idx_y = (n / 2);
                    int data_offset = (16 * idx_x) + (32 * 16 * idx_y);

                    p_dct_in = g_sigpool.p_rru_dct_in_final[0] + data_offset;
                    p_dct_out = g_sigpool.p_rru_dct_out_final[0] + data_offset;
                    p_idct_in = g_sigpool.p_rru_idct_in_final[0] + data_offset;
                    p_idct_out = g_sigpool.p_rru_idct_out_final[0] + data_offset;
#ifdef SIG_RRU_DEBUG
                    if (g_sigdump.rru_debug)
                        p_iq_in = g_sigpool.p_rru_iq_in_final[0] + data_offset;
#endif

                    for (int i = 0; i < 16; i++)
                    {
                        for (int j = 0; j < 16; j++)
                        {
                            sigdump_output_bin(p_ctx_in,       (unsigned char *)(&p_dct_in[j]), 2);
                            sigdump_output_bin(p_ctx_out,      (unsigned char *)(&p_dct_out[j]), 2);
                            sigdump_output_bin(p_ctx_in_idct,  (unsigned char *)(&p_idct_in[j]), 2);

                            short hw_clip = Clip3(-256, 255, (int)(p_idct_out[j]));
                            sigdump_output_bin(p_ctx_out_idct, (unsigned char *)(&hw_clip), 2);
#ifdef SIG_RRU_DEBUG
                            if (g_sigdump.rru_debug)
                                sigdump_output_bin(p_ctx_iq_in, (unsigned char *)(&p_iq_in[j]), 2);
#endif
                        }
                        p_dct_in += 32;
                        p_dct_out += 32;
                        p_idct_in += 32;
                        p_idct_out += 32;
#ifdef SIG_RRU_DEBUG
                        if (g_sigdump.rru_debug)
                            p_iq_in += 32;
#endif
                    }
                }
            }
            else
            {
                int cb_size = pu_size * pu_size;

                for (int n = 0; n < 4; n++)
                {
                    // Cb
                    int idx_x = (n & 0x1);
                    int idx_y = (n / 2);
                    int data_offset = (8 * idx_x) + (16 * 8 * idx_y);

                    p_dct_in = g_sigpool.p_rru_dct_in_final[1] + data_offset;
                    p_dct_out = g_sigpool.p_rru_dct_out_final[1] + data_offset;
                    p_idct_in = g_sigpool.p_rru_idct_in_final[1] + data_offset;
                    p_idct_out = g_sigpool.p_rru_idct_out_final[1] + data_offset;
#ifdef SIG_RRU_DEBUG
                    if (g_sigdump.rru_debug)
                        p_iq_in = g_sigpool.p_rru_iq_in_final[1] + data_offset;
#endif

                    for (int i = 0; i < 8; i++)
                    {
                        for (int j = 0; j < 8; j++)
                        {
                            sigdump_output_bin(p_ctx_in,       (unsigned char *)(&p_dct_in[j]), 2);
                            sigdump_output_bin(p_ctx_out,      (unsigned char *)(&p_dct_out[j]), 2);
                            sigdump_output_bin(p_ctx_in_idct,  (unsigned char *)(&p_idct_in[j]), 2);

                            short hw_clip = Clip3(-256, 255, (int)(p_idct_out[j]));
                            sigdump_output_bin(p_ctx_out_idct, (unsigned char *)(&hw_clip), 2);
#ifdef SIG_RRU_DEBUG
                            if (g_sigdump.rru_debug)
                                sigdump_output_bin(p_ctx_iq_in, (unsigned char *)(&p_iq_in[j]), 2);
#endif
                        }
                        p_dct_in += 16;
                        p_dct_out += 16;
                        p_idct_in += 16;
                        p_idct_out += 16;
#ifdef SIG_RRU_DEBUG
                        if (g_sigdump.rru_debug)
                            p_iq_in += 16;
#endif
                    }

                    // Cr
                    p_dct_in = g_sigpool.p_rru_dct_in_final[1] + cb_size + data_offset;
                    p_dct_out = g_sigpool.p_rru_dct_out_final[1] + cb_size + data_offset;
                    p_idct_in = g_sigpool.p_rru_idct_in_final[1] + cb_size + data_offset;
                    p_idct_out = g_sigpool.p_rru_idct_out_final[1] + cb_size + data_offset;
#ifdef SIG_RRU_DEBUG
                    if (g_sigdump.rru_debug)
                        p_iq_in = g_sigpool.p_rru_iq_in_final[1] + cb_size + data_offset;
#endif

                    for (int i = 0; i < 8; i++)
                    {
                        for (int j = 0; j < 8; j++)
                        {
                            sigdump_output_bin(p_ctx_in,       (unsigned char *)(&p_dct_in[j]), 2);
                            sigdump_output_bin(p_ctx_out,      (unsigned char *)(&p_dct_out[j]), 2);
                            sigdump_output_bin(p_ctx_in_idct,  (unsigned char *)(&p_idct_in[j]), 2);

                            short hw_clip = Clip3(-256, 255, (int)(p_idct_out[j]));
                            sigdump_output_bin(p_ctx_out_idct, (unsigned char *)(&hw_clip), 2);
#ifdef SIG_RRU_DEBUG
                            if (g_sigdump.rru_debug)
                                sigdump_output_bin(p_ctx_iq_in, (unsigned char *)(&p_iq_in[j]), 2);
#endif
                        }
                        p_dct_in += 16;
                        p_dct_out += 16;
                        p_idct_in += 16;
                        p_idct_out += 16;
#ifdef SIG_RRU_DEBUG
                        if (g_sigdump.rru_debug)
                            p_iq_in += 16;
#endif
                    }
                }
            }
        }
        else
        {
            int buf_idx = is_chroma ? 1 : 0;
            short *p_dct_in = g_sigpool.p_rru_dct_in_final[buf_idx];
            int *p_dct_out = g_sigpool.p_rru_dct_out_final[buf_idx];
            int *p_idct_in = g_sigpool.p_rru_idct_in_final[buf_idx];
            short *p_idct_out = g_sigpool.p_rru_idct_out_final[buf_idx];
            int pixel_count = pu_size * pu_size * (buf_idx + 1);

            // Inter DCT in and IDCT out, data type is short
            sigdump_output_bin(p_ctx_in,       (unsigned char *)(p_dct_in),   sizeof(short) * pixel_count);
            for (int i = 0; i < pixel_count; i++)
            {
                short hw_clip = Clip3(-256, 255, (int)(p_idct_out[i]));
                sigdump_output_bin(p_ctx_out_idct, (unsigned char *)(&hw_clip), 2);
            }

            // Inter DCT out and IDCT in, data type is int
            for (int i = 0; i < pixel_count; i++)
            {
                unsigned short temp = (*p_dct_out) & 0xffff;
                sigdump_output_bin(p_ctx_out, (unsigned char *)(&temp), sizeof(unsigned short));
                p_dct_out++;

                temp = (*p_idct_in) & 0xffff;
                sigdump_output_bin(p_ctx_in_idct, (unsigned char *)(&temp), sizeof(unsigned short));
                p_idct_in++;
            }

#ifdef SIG_RRU_DEBUG
            if (g_sigdump.rru_debug)
            {
                int *p_iq_in = g_sigpool.p_rru_iq_in_final[buf_idx];

                // Inte IQ in, data type is int
                for (int i = 0; i < pixel_count; i++)
                {
                    unsigned short temp = (*p_iq_in) & 0xffff;
                    sigdump_output_bin(p_ctx_iq_in, (unsigned char *)(&temp), sizeof(unsigned short));
                    p_iq_in++;
                }
            }
#endif //~SIG_RRU_DEBUG
        }
    }
}
#endif //~!SIG_RRU_TXT
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_output_dct(bool is_intra, int luma_pu_size, bool is_chroma)
{
#ifndef SIG_RRU_TXT
    sig_rru_output_dct_bin(is_intra, luma_pu_size, is_chroma);
    return;
#endif

    if (is_intra)
    {
        int pu_size = is_chroma ? (luma_pu_size == 16 ? 8 : 4) : luma_pu_size;
        int ctx_idx = sig_pat_blk_size((luma_pu_size << 1));

        //dct - idct
        int comp_idx = is_chroma ? 1 : 0;
        sig_ctx *p_ctx_in  = &g_sigpool.rru_intra_dct_ctx[ctx_idx][0][comp_idx];
        sig_ctx *p_ctx_out = &g_sigpool.rru_intra_dct_ctx[ctx_idx][1][comp_idx];

        sig_ctx *p_ctx_in_idct  = &g_sigpool.rru_intra_idct_ctx[ctx_idx][0][comp_idx];
        sig_ctx *p_ctx_out_idct = &g_sigpool.rru_intra_idct_ctx[ctx_idx][1][comp_idx];

        if (!is_chroma)
        {
            short *p_dct_in = g_sigpool.p_rru_dct_in_final[0];
            int *p_dct_out = g_sigpool.p_rru_dct_out_final[0];
            int *p_idct_in = g_sigpool.p_rru_idct_in_final[0];
            short *p_idct_out = g_sigpool.p_rru_idct_out_final[0];

            for (int i = 0; i < pu_size; i++)
            {
                for (int j = 0; j < pu_size; j++)
                {
                    sigdump_output_fprint(p_ctx_in, "%4d ", *p_dct_in);
                    sigdump_output_fprint(p_ctx_out, "%4d ", *p_dct_out);
                    p_dct_in++;
                    p_dct_out++;

                    sigdump_output_fprint(p_ctx_in_idct, "%4d ", *p_idct_in);
                    sigdump_output_fprint(p_ctx_out_idct, "%4d ", *p_idct_out);
                    p_idct_in++;
                    p_idct_out++;
                }
                sigdump_output_fprint(p_ctx_in, "\n");
                sigdump_output_fprint(p_ctx_out, "\n");

                sigdump_output_fprint(p_ctx_in_idct, "\n");
                sigdump_output_fprint(p_ctx_out_idct, "\n");
            }
        }
        else
        {
            short *p_dct_in = g_sigpool.p_rru_dct_in_final[1];
            int *p_dct_out = g_sigpool.p_rru_dct_out_final[1];
            int *p_idct_in = g_sigpool.p_rru_idct_in_final[1];
            short *p_idct_out = g_sigpool.p_rru_idct_out_final[1];

            // cb
            sigdump_output_fprint(p_ctx_in, "# Cb\n");
            sigdump_output_fprint(p_ctx_out, "# Cb\n");
            sigdump_output_fprint(p_ctx_in_idct, "# Cb\n");
            sigdump_output_fprint(p_ctx_out_idct, "# Cb\n");

            sigdump_output_fprint(p_ctx_in, "#nr=%d\n", 0);
            sigdump_output_fprint(p_ctx_out, "#nr=%d\n", 0);
            sigdump_output_fprint(p_ctx_in_idct, "#nr=%d\n", 0);
            sigdump_output_fprint(p_ctx_out_idct, "#nr=%d\n", 0);

            for (int i = 0; i < pu_size; i++)
            {
                for (int j = 0; j < pu_size; j++)
                {
                    sigdump_output_fprint(p_ctx_in, "%4d ", *p_dct_in);
                    sigdump_output_fprint(p_ctx_out, "%4d ", *p_dct_out);
                    p_dct_in++;
                    p_dct_out++;

                    sigdump_output_fprint(p_ctx_in_idct, "%4d ", *p_idct_in);
                    sigdump_output_fprint(p_ctx_out_idct, "%4d ", *p_idct_out);
                    p_idct_in++;
                    p_idct_out++;
                }
                sigdump_output_fprint(p_ctx_in, "\n");
                sigdump_output_fprint(p_ctx_out, "\n");

                sigdump_output_fprint(p_ctx_in_idct, "\n");
                sigdump_output_fprint(p_ctx_out_idct, "\n");
            }

            // Cr
            int cb_size = pu_size * pu_size;
            p_dct_in = &g_sigpool.p_rru_dct_in_final[1][cb_size];
            p_dct_out = &g_sigpool.p_rru_dct_out_final[1][cb_size];
            p_idct_in = &g_sigpool.p_rru_idct_in_final[1][cb_size];
            p_idct_out = &g_sigpool.p_rru_idct_out_final[1][cb_size];

            sigdump_output_fprint(p_ctx_in, "# Cr\n");
            sigdump_output_fprint(p_ctx_out, "# Cr\n");
            sigdump_output_fprint(p_ctx_in_idct, "# Cr\n");
            sigdump_output_fprint(p_ctx_out_idct, "# Cr\n");

            sigdump_output_fprint(p_ctx_in, "#nr=%d\n", 0);
            sigdump_output_fprint(p_ctx_out, "#nr=%d\n", 0);
            sigdump_output_fprint(p_ctx_in_idct, "#nr=%d\n", 0);
            sigdump_output_fprint(p_ctx_out_idct, "#nr=%d\n", 0);

            for (int i = 0; i < pu_size; i++)
            {
                for (int j = 0; j < pu_size; j++)
                {
                    sigdump_output_fprint(p_ctx_in, "%4d ", *p_dct_in);
                    sigdump_output_fprint(p_ctx_out, "%4d ", *p_dct_out);
                    p_dct_in++;
                    p_dct_out++;

                    sigdump_output_fprint(p_ctx_in_idct, "%4d ", *p_idct_in);
                    sigdump_output_fprint(p_ctx_out_idct, "%4d ", *p_idct_out);
                    p_idct_in++;
                    p_idct_out++;
                }
                sigdump_output_fprint(p_ctx_in, "\n");
                sigdump_output_fprint(p_ctx_out, "\n");

                sigdump_output_fprint(p_ctx_in_idct, "\n");
                sigdump_output_fprint(p_ctx_out_idct, "\n");
            }
        }
    }
    else    // inter
    {
        int pu_size = is_chroma ? (luma_pu_size >> 1) : luma_pu_size;
        int ctx_idx = sig_pat_blk_size(luma_pu_size);

        // inter DCT - IDCT
        int comp_idx = is_chroma ? 1 : 0;
        sig_ctx *p_ctx_in  = &g_sigpool.rru_inter_dct_ctx[ctx_idx][0][comp_idx];
        sig_ctx *p_ctx_out = &g_sigpool.rru_inter_dct_ctx[ctx_idx][1][comp_idx];

        sig_ctx *p_ctx_in_idct  = &g_sigpool.rru_inter_idct_ctx[ctx_idx][0][comp_idx];
        sig_ctx *p_ctx_out_idct = &g_sigpool.rru_inter_idct_ctx[ctx_idx][1][comp_idx];

        int numbers = (luma_pu_size == 32) ? 4 : 1;

        sigdump_output_fprint(p_ctx_in, "numbers=%d\n", numbers);
        sigdump_output_fprint(p_ctx_out, "numbers=%d\n", numbers);
        sigdump_output_fprint(p_ctx_in_idct, "numbers=%d\n", numbers);
        sigdump_output_fprint(p_ctx_out_idct, "numbers=%d\n", numbers);

        sigdump_output_fprint(p_ctx_in, "i_slice=%d\n", (g_sigpool.slice_type == I_SLICE) ? 1 : 0);
        sigdump_output_fprint(p_ctx_out, "i_slice=%d\n", (g_sigpool.slice_type == I_SLICE) ? 1 : 0);
        sigdump_output_fprint(p_ctx_in_idct, "i_slice=%d\n", (g_sigpool.slice_type == I_SLICE) ? 1 : 0);
        sigdump_output_fprint(p_ctx_out_idct, "i_slice=%d\n", (g_sigpool.slice_type == I_SLICE) ? 1 : 0);

        if (!is_chroma)
        {
            int qp = g_sigpool.rru_qp[0];
            sigdump_output_fprint(p_ctx_in, "qp=%d\n", qp);
            sigdump_output_fprint(p_ctx_out, "qp=%d\n", qp);
            sigdump_output_fprint(p_ctx_in_idct, "qp=%d\n", qp);
            sigdump_output_fprint(p_ctx_out_idct, "qp=%d\n", qp);
        }
        else
        {
            int qp_cb = g_sigpool.rru_qp[1];
            int qp_cr = g_sigpool.rru_qp[2];
            sigdump_output_fprint(p_ctx_in,       "qp (Cb, Cr) = (%d, %d)\n", qp_cb, qp_cr);
            sigdump_output_fprint(p_ctx_out,      "qp (Cb, Cr) = (%d, %d)\n", qp_cb, qp_cr);
            sigdump_output_fprint(p_ctx_in_idct,  "qp (Cb, Cr) = (%d, %d)\n", qp_cb, qp_cr);
            sigdump_output_fprint(p_ctx_out_idct, "qp (Cb, Cr) = (%d, %d)\n", qp_cb, qp_cr);
        }

        // DCT-IDCT data
        // CU32 : 16x16x4
        // CU16, CU8 : 2Nx2N
        if (!is_chroma)
        {
            short *p_dct_in = g_sigpool.p_rru_dct_in_final[0];
            int *p_dct_out = g_sigpool.p_rru_dct_out_final[0];

            int *p_idct_in = g_sigpool.p_rru_idct_in_final[0];
            short *p_idct_out = g_sigpool.p_rru_idct_out_final[0];

            if (pu_size == 32)
            {
                for (int n = 0; n < 4; n++)
                {
                    sigdump_output_fprint(p_ctx_in, "#nr=%d\n", n);
                    sigdump_output_fprint(p_ctx_out, "#nr=%d\n", n);
                    sigdump_output_fprint(p_ctx_in_idct, "#nr=%d\n", n);
                    sigdump_output_fprint(p_ctx_out_idct, "#nr=%d\n", n);

                    int idx_x = (n & 0x1);
                    int idx_y = (n / 2);
                    int data_offset = (16 * idx_x) + (32 * 16 * idx_y);

                    p_dct_in = g_sigpool.p_rru_dct_in_final[0] + data_offset;
                    p_dct_out = g_sigpool.p_rru_dct_out_final[0] + data_offset;
                    p_idct_in = g_sigpool.p_rru_idct_in_final[0] + data_offset;
                    p_idct_out = g_sigpool.p_rru_idct_out_final[0] + data_offset;

                    for (int i = 0; i < 16; i++)
                    {
                        for (int j = 0; j < 16; j++)
                        {
                            sigdump_output_fprint(p_ctx_in, "%4d ", p_dct_in[j]);
                            sigdump_output_fprint(p_ctx_out, "%4d ", p_dct_out[j]);
                            sigdump_output_fprint(p_ctx_in_idct, "%4d ", p_idct_in[j]);
                            sigdump_output_fprint(p_ctx_out_idct, "%4d ", p_idct_out[j]);
                        }
                        p_dct_in += 32;
                        p_dct_out += 32;
                        p_idct_in += 32;
                        p_idct_out += 32;

                        sigdump_output_fprint(p_ctx_in, "\n");
                        sigdump_output_fprint(p_ctx_out, "\n");
                        sigdump_output_fprint(p_ctx_in_idct, "\n");
                        sigdump_output_fprint(p_ctx_out_idct, "\n");
                    }
                }
            }
            else
            {
                sigdump_output_fprint(p_ctx_in, "#nr=%d\n", 0);
                sigdump_output_fprint(p_ctx_out, "#nr=%d\n", 0);
                sigdump_output_fprint(p_ctx_in_idct, "#nr=%d\n", 0);
                sigdump_output_fprint(p_ctx_out_idct, "#nr=%d\n", 0);

                for (int i = 0; i < pu_size; i++)
                {
                    for (int j = 0; j < pu_size; j++)
                    {
                        sigdump_output_fprint(p_ctx_in, "%4d ", *p_dct_in);
                        sigdump_output_fprint(p_ctx_out, "%4d ", *p_dct_out);
                        p_dct_in++;
                        p_dct_out++;

                        sigdump_output_fprint(p_ctx_in_idct, "%4d ", *p_idct_in);
                        sigdump_output_fprint(p_ctx_out_idct, "%4d ", *p_idct_out);
                        p_idct_in++;
                        p_idct_out++;
                    }
                    sigdump_output_fprint(p_ctx_in, "\n");
                    sigdump_output_fprint(p_ctx_out, "\n");

                    sigdump_output_fprint(p_ctx_in_idct, "\n");
                    sigdump_output_fprint(p_ctx_out_idct, "\n");
                }
            }
        }
        else
        {
            short *p_dct_in = g_sigpool.p_rru_dct_in_final[1];
            int *p_dct_out = g_sigpool.p_rru_dct_out_final[1];
            int *p_idct_in = g_sigpool.p_rru_idct_in_final[1];
            short *p_idct_out = g_sigpool.p_rru_idct_out_final[1];
            int cb_size = pu_size * pu_size;

            if (luma_pu_size == 32)
            {
                for (int n = 0; n < 4; n++)
                {
                    // Cb
                    sigdump_output_fprint(p_ctx_in, "# Cb\n");
                    sigdump_output_fprint(p_ctx_out, "# Cb\n");
                    sigdump_output_fprint(p_ctx_in_idct, "# Cb\n");
                    sigdump_output_fprint(p_ctx_out_idct, "# Cb\n");
                    sigdump_output_fprint(p_ctx_in, "#nr=%d\n", n);
                    sigdump_output_fprint(p_ctx_out, "#nr=%d\n", n);
                    sigdump_output_fprint(p_ctx_in_idct, "#nr=%d\n", n);
                    sigdump_output_fprint(p_ctx_out_idct, "#nr=%d\n", n);

                    int idx_x = (n & 0x1);
                    int idx_y = (n / 2);
                    int data_offset = (8 * idx_x) + (16 * 8 * idx_y);

                    p_dct_in = g_sigpool.p_rru_dct_in_final[1] + data_offset;
                    p_dct_out = g_sigpool.p_rru_dct_out_final[1] + data_offset;
                    p_idct_in = g_sigpool.p_rru_idct_in_final[1] + data_offset;
                    p_idct_out = g_sigpool.p_rru_idct_out_final[1] + data_offset;

                    for (int i = 0; i < 8; i++)
                    {
                        for (int j = 0; j < 8; j++)
                        {
                            sigdump_output_fprint(p_ctx_in, "%4d ", p_dct_in[j]);
                            sigdump_output_fprint(p_ctx_out, "%4d ", p_dct_out[j]);
                            sigdump_output_fprint(p_ctx_in_idct, "%4d ", p_idct_in[j]);
                            sigdump_output_fprint(p_ctx_out_idct, "%4d ", p_idct_out[j]);
                        }
                        p_dct_in += 16;
                        p_dct_out += 16;
                        p_idct_in += 16;
                        p_idct_out += 16;

                        sigdump_output_fprint(p_ctx_in, "\n");
                        sigdump_output_fprint(p_ctx_out, "\n");
                        sigdump_output_fprint(p_ctx_in_idct, "\n");
                        sigdump_output_fprint(p_ctx_out_idct, "\n");
                    }

                    // Cr
                    sigdump_output_fprint(p_ctx_in, "# Cr\n");
                    sigdump_output_fprint(p_ctx_out, "# Cr\n");
                    sigdump_output_fprint(p_ctx_in_idct, "# Cr\n");
                    sigdump_output_fprint(p_ctx_out_idct, "# Cr\n");
                    sigdump_output_fprint(p_ctx_in, "#nr=%d\n", n);
                    sigdump_output_fprint(p_ctx_out, "#nr=%d\n", n);
                    sigdump_output_fprint(p_ctx_in_idct, "#nr=%d\n", n);
                    sigdump_output_fprint(p_ctx_out_idct, "#nr=%d\n", n);

                    p_dct_in = g_sigpool.p_rru_dct_in_final[1] + cb_size + data_offset;
                    p_dct_out = g_sigpool.p_rru_dct_out_final[1] + cb_size + data_offset;
                    p_idct_in = g_sigpool.p_rru_idct_in_final[1] + cb_size + data_offset;
                    p_idct_out = g_sigpool.p_rru_idct_out_final[1] + cb_size + data_offset;

                    for (int i = 0; i < 8; i++)
                    {
                        for (int j = 0; j < 8; j++)
                        {
                            sigdump_output_fprint(p_ctx_in, "%4d ", p_dct_in[j]);
                            sigdump_output_fprint(p_ctx_out, "%4d ", p_dct_out[j]);
                            sigdump_output_fprint(p_ctx_in_idct, "%4d ", p_idct_in[j]);
                            sigdump_output_fprint(p_ctx_out_idct, "%4d ", p_idct_out[j]);
                        }
                        p_dct_in += 16;
                        p_dct_out += 16;
                        p_idct_in += 16;
                        p_idct_out += 16;

                        sigdump_output_fprint(p_ctx_in, "\n");
                        sigdump_output_fprint(p_ctx_out, "\n");
                        sigdump_output_fprint(p_ctx_in_idct, "\n");
                        sigdump_output_fprint(p_ctx_out_idct, "\n");
                    }
                }
            }
            else
            {
                // Cb
                sigdump_output_fprint(p_ctx_in, "# Cb\n");
                sigdump_output_fprint(p_ctx_out, "# Cb\n");
                sigdump_output_fprint(p_ctx_in_idct, "# Cb\n");
                sigdump_output_fprint(p_ctx_out_idct, "# Cb\n");

                sigdump_output_fprint(p_ctx_in, "#nr=%d\n", 0);
                sigdump_output_fprint(p_ctx_out, "#nr=%d\n", 0);
                sigdump_output_fprint(p_ctx_in_idct, "#nr=%d\n", 0);
                sigdump_output_fprint(p_ctx_out_idct, "#nr=%d\n", 0);

                for (int i = 0; i < pu_size; i++)
                {
                    for (int j = 0; j < pu_size; j++)
                    {
                        sigdump_output_fprint(p_ctx_in, "%4d ", *p_dct_in);
                        sigdump_output_fprint(p_ctx_out, "%4d ", *p_dct_out);
                        p_dct_in++;
                        p_dct_out++;

                        sigdump_output_fprint(p_ctx_in_idct, "%4d ", *p_idct_in);
                        sigdump_output_fprint(p_ctx_out_idct, "%4d ", *p_idct_out);
                        p_idct_in++;
                        p_idct_out++;
                    }
                    sigdump_output_fprint(p_ctx_in, "\n");
                    sigdump_output_fprint(p_ctx_out, "\n");

                    sigdump_output_fprint(p_ctx_in_idct, "\n");
                    sigdump_output_fprint(p_ctx_out_idct, "\n");
                }

                // Cr
                sigdump_output_fprint(p_ctx_in, "# Cr\n");
                sigdump_output_fprint(p_ctx_out, "# Cr\n");
                sigdump_output_fprint(p_ctx_in_idct, "# Cr\n");
                sigdump_output_fprint(p_ctx_out_idct, "# Cr\n");

                sigdump_output_fprint(p_ctx_in, "#nr=%d\n", 0);
                sigdump_output_fprint(p_ctx_out, "#nr=%d\n", 0);
                sigdump_output_fprint(p_ctx_in_idct, "#nr=%d\n", 0);
                sigdump_output_fprint(p_ctx_out_idct, "#nr=%d\n", 0);

                p_dct_in = &g_sigpool.p_rru_dct_in_final[1][cb_size];
                p_dct_out = &g_sigpool.p_rru_dct_out_final[1][cb_size];
                p_idct_in = &g_sigpool.p_rru_idct_in_final[1][cb_size];
                p_idct_out = &g_sigpool.p_rru_idct_out_final[1][cb_size];

                for (int i = 0; i < pu_size; i++)
                {
                    for (int j = 0; j < pu_size; j++)
                    {
                        sigdump_output_fprint(p_ctx_in, "%4d ", *p_dct_in);
                        sigdump_output_fprint(p_ctx_out, "%4d ", *p_dct_out);
                        p_dct_in++;
                        p_dct_out++;

                        sigdump_output_fprint(p_ctx_in_idct, "%4d ", *p_idct_in);
                        sigdump_output_fprint(p_ctx_out_idct, "%4d ", *p_idct_out);
                        p_idct_in++;
                        p_idct_out++;
                    }
                    sigdump_output_fprint(p_ctx_in, "\n");
                    sigdump_output_fprint(p_ctx_out, "\n");

                    sigdump_output_fprint(p_ctx_in_idct, "\n");
                    sigdump_output_fprint(p_ctx_out_idct, "\n");
                }
            }
        }
    }
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_copy_dct_temp_buf(ComponentID compID, Pel *p_residual, int res_stride,
                               TCoeff *p_coeff, int size, int tu_x, int tu_y)
{
    Pel *p_res = p_residual;
    TCoeff *p_coe = p_coeff;
    int buf_idx = (compID == COMPONENT_Y) ? 0 : 1;

    Pel *p_dct_in  = g_sigpool.p_rru_dct_in_temp[buf_idx];
    TCoeff *p_dct_out = g_sigpool.p_rru_dct_out_temp[buf_idx];

    if (compID == COMPONENT_Cr)
    {
        int cb_size = size * size;
        if (g_sigpool.cu_width == 32)
            cb_size = (cb_size << 2);
        p_dct_in  = &g_sigpool.p_rru_dct_in_temp[buf_idx][cb_size];
        p_dct_out = &g_sigpool.p_rru_dct_out_temp[buf_idx][cb_size];
    }

    if (g_sigpool.cu_width == 32)
    {
        int stride = (compID == COMPONENT_Y) ? 32 : 16;
        int offset = size * stride;

        if (tu_x > 0)
        {
            p_dct_in += size;
            p_dct_out += size;
        }
        if (tu_y > 0)
        {
            p_dct_in += offset;
            p_dct_out += offset;
        }

        int copy_in_size = sizeof(short) * size;
        int copy_out_size = sizeof(int) * size;

        for (int i = 0; i < size; i++)
        {
            memcpy(p_dct_in, p_res, copy_in_size);
            p_res += res_stride;
            p_dct_in += stride;

            memcpy(p_dct_out, p_coe, copy_out_size);
            p_coe += size;
            p_dct_out += stride;
        }
    }
    else
    {
        // residual data, dct in
        int byte_size = sizeof(short) * size;
        for (int i = 0; i < size; i++)
        {
            memcpy(p_dct_in, p_res, byte_size);
            p_res += res_stride;
            p_dct_in += size;
        }

        // coefficient data, dct out
        byte_size = sizeof(int) * size * size;
        memcpy(p_dct_out, p_coe, byte_size);
    }
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_copy_idct_temp_buf(ComponentID compID, Pel *p_residual, int res_stride,
                                TCoeff *p_coeff, int size, int tu_x, int tu_y)
{
    Pel *p_res = p_residual;
    TCoeff *p_coe = p_coeff;
    int buf_idx = (compID == COMPONENT_Y) ? 0 : 1;
    TCoeff *p_idct_in  = g_sigpool.p_rru_idct_in_temp[buf_idx];
    Pel *p_idct_out = g_sigpool.p_rru_idct_out_temp[buf_idx];

    if (compID == COMPONENT_Cr)
    {
        int cb_size = size * size;
        if (g_sigpool.cu_width == 32)
            cb_size = (cb_size << 2);
        p_idct_in  = &g_sigpool.p_rru_idct_in_temp[buf_idx][cb_size];
        p_idct_out = &g_sigpool.p_rru_idct_out_temp[buf_idx][cb_size];
    }

    if (g_sigpool.cu_width == 32)
    {
        int stride = (compID == COMPONENT_Y) ? 32 : 16;
        int offset = size * stride;

        if (tu_x > 0)
        {
            p_idct_in += size;
            p_idct_out += size;
        }
        if (tu_y > 0)
        {
            p_idct_in += offset;
            p_idct_out += offset;
        }

        int copy_in_size = sizeof(int) * size;
        int copy_out_size = sizeof(short) * size;

        for (int i = 0; i < size; i++)
        {
            memcpy(p_idct_in, p_coe, copy_in_size);
            p_coe += size;
            p_idct_in += stride;

            memcpy(p_idct_out, p_res, copy_out_size);
            p_res += res_stride;
            p_idct_out += stride;
        }
    }
    else
    {
        // residual data, idct out
        int byte_size = sizeof(short) * size;
        for (int i = 0; i < size; i++)
        {
            memcpy(p_idct_out, p_res, byte_size);
            p_res += res_stride;
            p_idct_out += size;
        }

        // coefficient data, idct in
        byte_size = sizeof(int) * size * size;
        memcpy(p_idct_in, p_coe, byte_size);
    }
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_update_final_buf(bool is_chroma, bool is_intra, int size)
{
    int buf_idx = is_chroma ? 1 : 0;
    int pel_nr = size * size;

    if (is_chroma)
        pel_nr = (pel_nr << 1);

    // only intra needs to update intp
    if (is_intra)
    {
        memcpy(g_sigpool.p_rru_intp_final, g_sigpool.p_rru_intp_temp, sizeof(Pel) * pel_nr);

        if (is_chroma)
        {
            if (g_sigpool.luma_pu_size == 4)
            {
                memcpy(g_sigpool.p_rru_i4_resi_final[1], g_sigpool.p_rru_i4_resi_temp[1], 16 * sizeof(int));
                memcpy(g_sigpool.p_rru_i4_resi_final[2], g_sigpool.p_rru_i4_resi_temp[2], 16 * sizeof(int));
            }
        }
        else if (size == 4)
        {
            memcpy(g_sigpool.p_rru_i4_resi_final[0], g_sigpool.p_rru_i4_resi_temp[0], 16 * sizeof(int));
        }
    }

    memcpy(g_sigpool.p_rru_dct_in_final[buf_idx], g_sigpool.p_rru_dct_in_temp[buf_idx], sizeof(Pel) * pel_nr);
    memcpy(g_sigpool.p_rru_dct_out_final[buf_idx], g_sigpool.p_rru_dct_out_temp[buf_idx], sizeof(Int) * pel_nr);
    memcpy(g_sigpool.p_rru_idct_in_final[buf_idx], g_sigpool.p_rru_idct_in_temp[buf_idx], sizeof(Int) * pel_nr);
    memcpy(g_sigpool.p_rru_idct_out_final[buf_idx], g_sigpool.p_rru_idct_out_temp[buf_idx], sizeof(Pel) * pel_nr);

#ifdef SIG_RRU_DEBUG
    if (g_sigdump.rru_debug)
    {
        if (!is_intra)
            memcpy(g_sigpool.p_rru_iq_in_final[buf_idx], g_sigpool.p_rru_iq_in_temp[buf_idx], sizeof(Int) * pel_nr);
    }
#endif //~SIG_RRU_DEBUG
}
#endif //~SIGRRU

#ifdef SIG_RRU
int rru_get_last_pos_scale(double input)
{
    return (int)((input/16.0) * g_estBitPrec);
}

void sig_rru_output_cu_self_info(sig_ctx *p_ctx, int cu_size)
{
    // Self-information
    //g_hwEntropyBits[stateIdx][isLps];
    sigdump_output_fprint(p_ctx, "# Self-information\n");
    const string pred_s[] = {"inter", "intra"};
    const string color_s[] = {"luma", "chroma"};
    unsigned int bitest_sfi;
    for (int predID = 0; predID < 2; predID++) {
        for (int compID = 0; compID < 2; compID++) {
            if (cu_size == 4)
                bitest_sfi = (predID == 0) ? 0 : g_sigpool.rru_gold.rru_est_gold_4x4[compID][0].bitest_sfi;
            else
                bitest_sfi = g_sigpool.rru_gold.rru_est_gold[predID][compID][0].bitest_sfi;
            sigdump_output_fprint(p_ctx, "reg_%s_tu_bitest_%s_sfi = %d\n",     pred_s[predID].c_str(), color_s[compID].c_str(), bitest_sfi);
        }
        for (int compID = 0; compID < 2; compID++) {
            unsigned int *bitest_cbf_ctx = (cu_size == 4) ?
                g_sigpool.rru_gold.rru_est_gold_4x4[compID][0].bitest_cbf_ctx
                : g_sigpool.rru_gold.rru_est_gold[predID][compID][0].bitest_cbf_ctx;
            sigdump_output_fprint(p_ctx, "reg_%s_tu_bitest_%s_cbf_ctx = %d,%d\n", pred_s[predID].c_str(), color_s[compID].c_str(), bitest_cbf_ctx[0], bitest_cbf_ctx[1]);
        }
    }

    // g_lastPosBinScale_init[chType][isIntra][uiLog2BlockWidth-2]
    // Ratio for lastPosXY
    sigdump_output_fprint(p_ctx, "# Ratio for lastPosXY\n");
    for (int predID = 0; predID < 2; predID++)
        for (int compID = 0; compID < 2; compID++) {
            int log2_size_m2 = (cu_size == 4) ? 0 : (((cu_size == 8) ? 1 : 2) - compID);
            sigdump_output_fprint(p_ctx, "reg_%s_bitest_%s_ratio = %d\n", pred_s[predID].c_str(), color_s[compID].c_str(), rru_get_last_pos_scale(g_lastPosBinScale_init[compID][predID][log2_size_m2]));
        }
    sigdump_output_fprint(p_ctx, "# linear Model for SIG and GT1\n");
    for (int predID = 0; predID < 2; predID++) {
        for (int compID = 0; compID < 2; compID++) {
            int log2_size_m2 = (cu_size == 4) ? 0 : (((cu_size == 8) ? 1 : 2) - compID);
            const int *m = &g_significantScale[g_scale_D][compID][predID][log2_size_m2][0];
            const int *c = &g_significantBias[g_scale_D][compID][predID][log2_size_m2][0];
            sigdump_output_fprint(p_ctx, "reg_%s_tu_%s_m = %d, %d\n", pred_s[predID].c_str(), color_s[compID].c_str(), m[0], m[1]);
            sigdump_output_fprint(p_ctx, "reg_%s_tu_%s_c = %d, %d\n", pred_s[predID].c_str(), color_s[compID].c_str(), c[0], c[1]);
        }
    }
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_output_cu_winner(int cu_width)
{
    int ctx_idx = sig_pat_blk_size(cu_width);
    sig_ctx *p_ctx = &g_sigpool.rru_cu_ctx[ctx_idx];
    int is_win = 0;

    if (cu_width == 16 || (cu_width == 32 && g_sigpool.slice_type != I_SLICE))
    {
        if (g_sigpool.rru_cu_cost[0] <= g_sigpool.rru_cu_cost[1])
            is_win = 1;
    }
    sigdump_output_fprint(p_ctx, "is_win = %d\n", is_win);
    if (g_sigdump.ccu) {
        bool is_comp_split_intra_win = (cu_width == 32) ? 0 : g_sigpool.is_comp_split_intra_win[cu_width == 16 ? 1 : 0];
        sigdump_output_fprint(&g_sigpool.ccu_ctx[sig_pat_blk_size_4(cu_width)], "is_comp_split_intra_win = %d\n", is_comp_split_intra_win);
        sigdump_output_fprint(&g_sigpool.ccu_ctx[sig_pat_blk_size_4(cu_width)], "is_win = %d\n", is_win);
    }
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_output_cu_cmd(TComDataCU *p_cu, int cu_width)
{
    int ctx_idx = sig_pat_blk_size(cu_width);
    sigdump_output_fprint(&g_sigpool.rru_cu_ctx[ctx_idx], "qp = 0x%x, 0x%x, 0x%x\n",
                          g_sigpool.rru_qp[0], g_sigpool.rru_qp[1], g_sigpool.rru_qp[2]);

    int is_intra_win = p_cu->getPredictionMode(0) == MODE_INTRA ? 1 : 0;
    sigdump_output_fprint(&g_sigpool.rru_cu_ctx[ctx_idx], "is_intra_win = %d\n", is_intra_win);

    int is_skip_win = p_cu->isSkipped(0) ? 1 : 0;
    sigdump_output_fprint(&g_sigpool.rru_cu_ctx[ctx_idx], "is_skip_win = %d\n", is_skip_win);
    sigdump_output_fprint(&g_sigpool.rru_cu_ctx[ctx_idx], "is_mrg_win = %d\n", g_sigpool.rru_is_mrg_win);
    sigdump_output_fprint(&g_sigpool.rru_cu_ctx[ctx_idx], "mrg_cand_idx = %d\n", g_sigpool.rru_merge_cand);
    sigdump_output_fprint(&g_sigpool.rru_cu_ctx[ctx_idx], "scan_idx = %d, %d\n",
                          g_sigpool.rru_scan_idx[0], g_sigpool.rru_scan_idx[1]);

    sig_rru_output_cu_self_info(&g_sigpool.rru_cu_ctx[ctx_idx], cu_width);
    if (g_sigdump.ccu)
        sig_rru_output_cu_self_info(&g_sigpool.ccu_ctx[sig_pat_blk_size_4(cu_width)], cu_width);

    // is_comp_split_intra_win
    // if CU8, compare 4xI4 vs I8, 1: I8 win
    // if CU16, compare 4xI8 vs I16, 1: I16 win
    // if CU32, is_win = 0
    int is_win = 0;
    if (cu_width == 8)
    {
        if (g_sigpool.slice_type != I_SLICE && g_algo_cfg.DisablePfrmIntra4)
        {
            is_win = 1;
            g_sigpool.rru_intra_cost_i8x4 += g_sigpool.rru_intra_cost_wt[1];
        }
        else
        {
            if (g_sigpool.rru_intra_cost[1] <= g_sigpool.rru_intra_cost[0]) {
                is_win = 1; // 8x8 wins 4x4
            }
            g_sigpool.rru_intra_cost_i8x4 += (is_win == 1) ? g_sigpool.rru_intra_cost_wt[1] : g_sigpool.rru_intra_cost_wt[0];
        }

        sigdump_output_fprint(&g_sigpool.rru_cu_ctx[ctx_idx], "is_comp_split_intra_win = %d\n", is_win);
        if (g_sigdump.ccu) {
            g_sigpool.is_comp_split_intra_win[0] = is_win;
        }

        if ((g_sigpool.cu_idx_x & 0x8) > 0 && (g_sigpool.cu_idx_y & 0x8) > 0)
        {
            is_win = (g_sigpool.rru_intra_cost_wt[2] <= g_sigpool.rru_intra_cost_i8x4) ? 1 : 0;
            sigdump_output_fprint(&g_sigpool.rru_cu_ctx[1], "is_comp_split_intra_win = %d\n", is_win);
            if (g_sigdump.ccu) {
                g_sigpool.is_comp_split_intra_win[1] = is_win;
            }
        }
    }
    else if (cu_width == 32)
    {
        sigdump_output_fprint(&g_sigpool.rru_cu_ctx[2], "is_comp_split_intra_win = %d\n", 0);
    }
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_output_cu_iap(sig_ctx *p_ctx, const Pel *p_rec, int size, int is_b)
{
    unsigned char output_nb[size];
    memset(output_nb, 0, sizeof(unsigned char) * size);
    unsigned char *p_nb = &(output_nb[0]);

    int nb_start = 0;
    int nb_offset = 0;
    if (is_b)
    {
        nb_start  = (size - 1) * size;
        nb_offset = 1;
    }
    else
    {
        nb_start  = size - 1;
        nb_offset = size;
    }

    for (int i = 0; i < size; i++)
    {
        *p_nb = p_rec[nb_start + i * nb_offset];
        p_nb++;
    }
    sigdump_output_bin(p_ctx, output_nb, size);
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_output_cu_golden(int cu_width, SliceType slicetype, TComYuv *p_rec_yuv)
{
    int ctx_idx = sig_pat_blk_size(cu_width);
    sig_ctx *p_rru_ctx = &g_sigpool.rru_cu_gold_ctx[ctx_idx];
    int pred_bitest[2] = {0};
    int tu_count = (cu_width == 32) ? 4 : 1;
    unsigned int used_si_val[2][2][3] = {0};    //[is_intra][depth][comp]
    unsigned int tu_depth = 0;
    if (slicetype != I_SLICE)
    {
        int cbf_32x32[3] = {1, 0, 0};
        if (tu_count == 4)
        {
            //code 32x32 CB/CR cbf
            for (int comp = 1; comp < 3; comp++)
            {
                for (int tu_idx = 0; tu_idx < tu_count; tu_idx++) {
                    if (g_sigpool.rru_gold.rru_est_gold[0][comp][tu_idx].cbf > 0)
                        cbf_32x32[comp] = 1;
                }
                used_si_val[0][tu_depth][comp] += g_cbfScale[g_scale_D][1][0][cbf_32x32[comp]];
                //pred_bitest[0] += used_si_val[0][tu_depth][comp]; //add two cb/cr cbf
            }
            tu_depth = 1;
        } else {
            cbf_32x32[2] = cbf_32x32[1] = 1;
        }

        for (int tu_idx = 0; tu_idx < tu_count; tu_idx++)
        {
            for (int comp = 0; comp < 3; comp++)
            {
                unsigned int cbcr_cbf = g_sigpool.rru_gold.rru_est_gold[0][1][tu_idx].cbf | g_sigpool.rru_gold.rru_est_gold[0][2][tu_idx].cbf;
                if (cbf_32x32[comp])
                {
                    int depth = (tu_count == 4) ? 1 : 0;
                    int uiCtx = (comp == 0) ? ((depth == 0) ? 1 : 0) : depth;
                    unsigned int *si_val = g_cbfScale[g_scale_D][comp == 0 ? 0 : 1][uiCtx];
                    unsigned int cbf = g_sigpool.rru_gold.rru_est_gold[0][comp][tu_idx].cbf > 0 ? 1 : 0;
                    if (cbf > 0) {
                        pred_bitest[0] += g_sigpool.rru_gold.rru_est_gold[0][comp][tu_idx].bitest;
                    }
                    if (!(comp == 0 && tu_count == 1 && cbcr_cbf == 0)) {
                        used_si_val[0][tu_depth][comp] += si_val[cbf];
                        //pred_bitest[0] += si_val[cbf];
                    }
                }
            }
        }
    }

    if (cu_width != 32) {
        tu_depth = 0;
        //intra no split
        for (int comp = 0; comp < 3; comp++)
        {
            int uiCtx = (comp == 0) ? 1 : 0;
            unsigned int *si_val = g_cbfScale[g_scale_D][comp == 0 ? 0 : 1][uiCtx];
            unsigned int cbf = g_sigpool.rru_gold.rru_est_gold[1][comp][0].cbf > 0 ? 1 : 0;
            used_si_val[1][tu_depth][comp] += si_val[cbf];
            if (cbf > 0) {
                pred_bitest[1] += g_sigpool.rru_gold.rru_est_gold[1][comp][0].bitest;
            }
            //pred_bitest[1] += si_val[cbf];
        }
    }
    //pred_bitest[0] = pred_bitest[0] >> g_estBitFracBd;
    //pred_bitest[1] = pred_bitest[1] >> g_estBitFracBd;

    for (int tu_idx = 0; tu_idx < tu_count; tu_idx++)
    {
        std::vector<sig_ctx *> ctx_arr {p_rru_ctx};
        if (g_sigdump.ccu) {
            sig_ctx *p_ccu_ctx = &g_sigpool.ccu_ctx[sig_pat_blk_size_4(cu_width)];
            ctx_arr.push_back(p_ccu_ctx);
            sigdump_output_fprint(p_ccu_ctx, "# Estimation\n");
        }
        //for(auto p_ctx=v.begin(); p_ctx!=v.end(); ++p_ctx) {
        for (auto& p_ctx : ctx_arr) {
            sig_rru_est_gold_st *p_rru_est_gold;
            const string comp_s[] = {"luma", "cb", "cr"};
            const string pred_s[] = {"inter", "intra"};

            for (int pred_mode = 0; pred_mode < 2; pred_mode++) {
                sigdump_output_fprint(p_ctx, "%s_sse_val = 0x%x\n", pred_s[pred_mode].c_str(), g_sigpool.rru_gold.sse[pred_mode]);
                sigdump_output_fprint(p_ctx, "%s_bitest_val = 0x%x\n", pred_s[pred_mode].c_str(), pred_bitest[pred_mode]);
                sigdump_output_fprint(p_ctx, "%s_si_val = ", pred_s[pred_mode].c_str());
                for (int depth = 0; depth < 2; depth++)
                    for (int comp = 0; comp < 3; comp++)
                        sigdump_output_fprint(p_ctx, "0x%x,", used_si_val[pred_mode][depth][comp]);
                sigdump_output_fprint(p_ctx, "\n");
                for (int comp = 0; comp < 3; comp++) {
                    p_rru_est_gold = &g_sigpool.rru_gold.rru_est_gold[pred_mode][comp][tu_idx];
                    sigdump_output_fprint(p_ctx, "%s_%s_csbf = 0x%x\n", pred_s[pred_mode].c_str(), comp_s[comp].c_str(), p_rru_est_gold->csbf);
                    sigdump_output_fprint(p_ctx, "%s_%s_last_xy = 0x%x, 0x%x\n", pred_s[pred_mode].c_str(), comp_s[comp].c_str(), p_rru_est_gold->last_x, p_rru_est_gold->last_y);
                }
            }
        }
    }

    p_rru_ctx = &g_sigpool.rru_cu_iap_ctx[ctx_idx];
    for (int nb_b = 0; nb_b < 2; nb_b++)
    {
        for (int comp = 0; comp < 3; comp++)
        {
            ComponentID id = ComponentID(comp);
            int stride = p_rec_yuv->getStride(id);
            const Pel *p_rec = p_rec_yuv->getAddr(id, 0, stride);
            sig_rru_output_cu_iap(p_rru_ctx, p_rec, stride, nb_b);
        }
    }
}

void sig_ccu_output_cu_golden_4x4()
{
    int cu_width = 4;
    int pred_bitest[2] = {0};
    int tu_count = 4;
    unsigned int sse[2] = {0};
    unsigned int used_si_val[2][2][3] = {0};    //[is_intra][depth][comp]
    unsigned int tu_depth = 0;
    for (int tu_idx = 0; tu_idx < tu_count; tu_idx++)
    {
        for (int comp = 0; comp < (tu_idx == 0 ? 3 : 1); comp++)
        {
            tu_depth = comp == 0 ? 1 : 0;
            int uiCtx = 0; //Luma: !tu_depth , Chroma:tu_depth
            unsigned int *si_val = g_cbfScale[g_scale_D][comp == 0 ? 0 : 1][uiCtx];
            unsigned int cbf = g_sigpool.rru_gold.rru_est_gold_4x4[comp][tu_idx].cbf > 0 ? 1 : 0;
            used_si_val[1][tu_depth][comp] += si_val[cbf];
            if (g_sigpool.rru_gold.rru_est_gold_4x4[comp][tu_idx].cbf > 0) {
                pred_bitest[1] += g_sigpool.rru_gold.rru_est_gold_4x4[comp][tu_idx].bitest;
            }
            //pred_bitest[1] += used_si_val[1][tu_depth][comp];
        }
        sse[1] += g_sigpool.rru_gold.sse4x4[tu_idx];
    }
    //pred_bitest[1] = pred_bitest[1] >> g_estBitFracBd;
    sig_rru_est_gold_st *p_rru_est_gold;
    const string comp_s[] = {"luma", "cb", "cr"};
    const string pred_s[] = {"inter", "intra"};

    sig_ctx *p_ctx = &g_sigpool.ccu_ctx[sig_pat_blk_size_4(cu_width)];
    for (int tu_idx = 0; tu_idx < tu_count; tu_idx++)
    {
        sigdump_output_fprint(p_ctx, "# Estimation\n");
        for (int pred_mode = 0; pred_mode < 2; pred_mode++) {
            sigdump_output_fprint(p_ctx, "%s_sse_val = 0x%x\n", pred_s[pred_mode].c_str(), sse[pred_mode]);
            sigdump_output_fprint(p_ctx, "%s_bitest_val = 0x%x\n", pred_s[pred_mode].c_str(), pred_bitest[pred_mode]);
            sigdump_output_fprint(p_ctx, "%s_si_val = ", pred_s[pred_mode].c_str());
            for (int depth = 0; depth < 2; depth++)
                for (int comp = 0; comp < 3; comp++)
                    sigdump_output_fprint(p_ctx, "0x%x,", used_si_val[pred_mode][depth][comp]);
            sigdump_output_fprint(p_ctx, "\n");
            for (int comp = 0; comp < 3; comp++)
            {
                p_rru_est_gold = &g_sigpool.rru_gold.rru_est_gold_4x4[comp][tu_idx];
                sigdump_output_fprint(p_ctx, "%s_%s_csbf = 0x%x\n", pred_s[pred_mode].c_str(), comp_s[comp].c_str(), p_rru_est_gold->csbf);
                sigdump_output_fprint(p_ctx, "%s_%s_last_xy = 0x%x, 0x%x\n", pred_s[pred_mode].c_str(), comp_s[comp].c_str(), p_rru_est_gold->last_x, p_rru_est_gold->last_y);
            }
        }
    }
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_output_i4_rec(ComponentID comp, const Pel *p_rec_buf, int stride)
{
    if (comp == COMPONENT_Y)
    {
        unsigned char temp[64] = { 0 };
        int offset[4] = { 0, 4, (stride << 2), (stride << 2) + 4};
        int idx = 0;
        for (int blk = 0; blk < 4; blk++)
        {
            const Pel *p_rec = p_rec_buf + offset[blk];
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    temp[idx] = (p_rec[(i * stride) + j] & 0xff);
                    idx++;
                }
            }
        }
        sigdump_output_bin(&g_sigpool.rru_i4_rec_ctx, temp, 64);
    }
    else
    {
        unsigned char temp[16] = { 0 };
        for (int i = 0; i < 16; i++)
            temp[i] = (p_rec_buf[i] & 0xff);

        sigdump_output_bin(&g_sigpool.rru_i4_rec_ctx, temp, 16);
    }
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_output_intra(int puSize, const TComRectangle &rect, TComDataCU* pcCU)
{
    int ctx_idx = sig_pat_blk_size(puSize << 1);

    if (puSize != 4 || (puSize == 4 && rect.x0 == 0 && rect.y0 == 0))
    {
        int is_i_slice = (g_sigpool.slice_type == I_SLICE) ? 1 : 0;
        const QpParam cQP(*pcCU, COMPONENT_Y);
        const QpParam QP_cb(*pcCU, COMPONENT_Cb);
        const QpParam QP_cr(*pcCU, COMPONENT_Cr);

#ifndef SIG_RRU_TXT
        unsigned int ctb_x = g_sigpool.ctb_idx_x;
        unsigned int ctb_y = g_sigpool.ctb_idx_y;
        unsigned int cu_x  = g_sigpool.cu_idx_x;
        unsigned int cu_y  = g_sigpool.cu_idx_y;
        unsigned int tu_number[2] = { 0 };
        tu_number[0] = (puSize == 4) ? 4 : 1;
        tu_number[1] = 1;
        unsigned char temp[10] = { 0 };
        for (int io = 0; io < 2; io++)
        {
            for (int ch = 0; ch < 2; ch++)
            {
                // Intra DCT
                // # CTB (X, Y)
                sigdump_output_bin(&g_sigpool.rru_intra_dct_ctx[ctx_idx][io][ch], (unsigned char *)(&ctb_x), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_dct_ctx[ctx_idx][io][ch], (unsigned char *)(&ctb_y), 2);
                // # CU (X, Y)
                sigdump_output_bin(&g_sigpool.rru_intra_dct_ctx[ctx_idx][io][ch], (unsigned char *)(&cu_x), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_dct_ctx[ctx_idx][io][ch], (unsigned char *)(&cu_y), 2);
                // tu info
                sigdump_output_bin(&g_sigpool.rru_intra_dct_ctx[ctx_idx][io][ch], (unsigned char *)(&tu_number[ch]), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_dct_ctx[ctx_idx][io][ch], (unsigned char *)(&is_i_slice), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_dct_ctx[ctx_idx][io][ch], (unsigned char *)(temp), 4);
                sigdump_output_bin(&g_sigpool.rru_intra_dct_ctx[ctx_idx][io][ch], (unsigned char *)(&cQP.Qp), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_dct_ctx[ctx_idx][io][ch], (unsigned char *)(&QP_cb.Qp), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_dct_ctx[ctx_idx][io][ch], (unsigned char *)(&QP_cr.Qp), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_dct_ctx[ctx_idx][io][ch], (unsigned char *)(temp), 10);

                // Intra IDCT
                // # CTB (X, Y)
                sigdump_output_bin(&g_sigpool.rru_intra_idct_ctx[ctx_idx][io][ch], (unsigned char *)(&ctb_x), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_idct_ctx[ctx_idx][io][ch], (unsigned char *)(&ctb_y), 2);
                // # CU (X, Y)
                sigdump_output_bin(&g_sigpool.rru_intra_idct_ctx[ctx_idx][io][ch], (unsigned char *)(&cu_x), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_idct_ctx[ctx_idx][io][ch], (unsigned char *)(&cu_y), 2);
                // tu info
                sigdump_output_bin(&g_sigpool.rru_intra_idct_ctx[ctx_idx][io][ch], (unsigned char *)(&tu_number[ch]), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_idct_ctx[ctx_idx][io][ch], (unsigned char *)(&is_i_slice), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_idct_ctx[ctx_idx][io][ch], (unsigned char *)(temp), 4);
                sigdump_output_bin(&g_sigpool.rru_intra_idct_ctx[ctx_idx][io][ch], (unsigned char *)(&cQP.Qp), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_idct_ctx[ctx_idx][io][ch], (unsigned char *)(&QP_cb.Qp), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_idct_ctx[ctx_idx][io][ch], (unsigned char *)(&QP_cr.Qp), 2);
                sigdump_output_bin(&g_sigpool.rru_intra_idct_ctx[ctx_idx][io][ch], (unsigned char *)(temp), 10);
            }
        }
#endif //~!SIG_RRU_TXT

#ifdef SIG_RRU_TXT
        int dct_number = (puSize == 4) ? 4 : 1;
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][0][0], "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][1][0], "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][0][1], "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][1][1], "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][0][0], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][1][0], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][0][1], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][1][1], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][0][0], "numbers=%d\n", dct_number);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][1][0], "numbers=%d\n", dct_number);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][0][1], "numbers=%d\n", 1);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][1][1], "numbers=%d\n", 1);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][0][0], "i_slice=%d\n", is_i_slice);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][1][0], "i_slice=%d\n", is_i_slice);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][0][1], "i_slice=%d\n", is_i_slice);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][1][1], "i_slice=%d\n", is_i_slice);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][0][0], "qp=%d\n", cQP.Qp);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][1][0], "qp=%d\n", cQP.Qp);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][0][1],  "qp (Cb, Cr) = (%d, %d)\n", QP_cb.Qp, QP_cr.Qp);
        sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][1][1],  "qp (Cb, Cr) = (%d, %d)\n", QP_cb.Qp, QP_cr.Qp);

        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][0][0], "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][1][0], "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][0][1], "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][1][1], "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][0][0], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][1][0], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][0][1], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][1][1], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][0][0], "numbers=%d\n", dct_number);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][1][0], "numbers=%d\n", dct_number);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][0][1], "numbers=%d\n", 1);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][1][1], "numbers=%d\n", 1);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][0][0], "i_slice=%d\n", is_i_slice);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][1][0], "i_slice=%d\n", is_i_slice);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][0][1], "i_slice=%d\n", is_i_slice);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][1][1], "i_slice=%d\n", is_i_slice);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][0][0], "qp=%d\n", cQP.Qp);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][1][0], "qp=%d\n", cQP.Qp);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][0][1], "qp (Cb, Cr) = (%d, %d)\n", QP_cb.Qp, QP_cr.Qp);
        sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][1][1], "qp (Cb, Cr) = (%d, %d)\n", QP_cb.Qp, QP_cr.Qp);
#endif //~SIG_RRU_TXT
    }

#ifdef SIG_RRU_TXT
    int dct_nr = 0;
    if (puSize == 4)
    {
        if (rect.x0 != 0)
            dct_nr++;
        if (rect.y0 != 0)
            dct_nr += 2;
    }

    sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][0][0], "#nr=%d\n", dct_nr);
    sigdump_output_fprint(&g_sigpool.rru_intra_dct_ctx[ctx_idx][1][0], "#nr=%d\n", dct_nr);
    sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][0][0], "#nr=%d\n", dct_nr);
    sigdump_output_fprint(&g_sigpool.rru_intra_idct_ctx[ctx_idx][1][0], "#nr=%d\n", dct_nr);
#endif //~SIG_RRU_TXT
}
#endif //~SIG_RRU

#ifdef SIG_RESI
void sig_copy_resi_buf(ComponentID compID, TCoeff *p_coeff_buf, int pos_x, int pos_y, int tu_size)
{
    int buf_idx = (compID == COMPONENT_Y) ? 0 : 1;
    int buf_stride = g_sigpool.fb_pitch;
    int offset = pos_y * buf_stride + pos_x;
    if (compID == COMPONENT_Cr)
        offset++;   // NV12 format

    TCoeff *p_src = p_coeff_buf;
    short  *p_dst_start = g_sigpool.p_resi_buf[buf_idx] + offset;

    // copy tu coeff to frame buf
    int step = (compID == COMPONENT_Y) ? 1 : 2;
    for (int i = 0; i < tu_size; i++)
    {
        short *p_dst = p_dst_start;
        for (int j = 0; j < tu_size; j++)
        {
            *p_dst = (short)(*p_src);
            p_dst += step;
            p_src++;
        }
        p_dst_start += buf_stride;
    }
}
#endif //~SIG_RESI

#ifdef SIG_RESI
void sig_output_resi_buf()
{
    sig_ctx *p_ctx = &g_sigpool.resi_frm_ctx;
    int luma_size = (g_sigpool.heightAlign8 * 8) * g_sigpool.fb_pitch * sizeof(short);

    sigdump_output_bin(p_ctx, (unsigned char *)(g_sigpool.p_resi_buf[0]), luma_size);
    sigdump_output_bin(p_ctx, (unsigned char *)(g_sigpool.p_resi_buf[1]), (luma_size >> 1));
}
#endif //~SIG_RESI

#ifdef SIG_RRU
void sig_rru_output_i4_resi(bool is_chroma)
{
    sig_ctx *p_ctx = &g_sigpool.rru_i4_resi_ctx;
    short temp[16] = { 0 };

    int start_idx = 0;
    int end_idx = 1;
    if(is_chroma)
    {
        start_idx = 1;
        end_idx = 3;
    }

    for (int i = start_idx; i < end_idx; i++)
    {
        int *p_src = g_sigpool.p_rru_i4_resi_final[i];
        for (int j = 0; j < 16; j++)
            temp[j] = (short)(p_src[j]);

        sigdump_output_bin(p_ctx, (unsigned char *)(temp), sizeof(temp));
    }
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_output_i4term_temp_data()
{
    // If I4 is terminated, output temp data to pattern file.
    for (int i = 0; i < 3; i++)
        memset(g_sigpool.p_rru_i4_resi_final[i], 0, sizeof(int) * 16);

    sig_rru_output_i4_resi(false);
    sig_rru_output_i4_resi(false);
    sig_rru_output_i4_resi(false);
    sig_rru_output_i4_resi(false);
    sig_rru_output_i4_resi(true);

    unsigned char temp[96];   // luma(64)+cb(16)+cr(16)
    memset(temp, 0, sizeof(temp));
    sigdump_output_bin(&g_sigpool.rru_i4_rec_ctx, temp, sizeof(temp));

    memset(g_sigpool.p_rru_intp_final, 0, sizeof(short) * 32);
    sig_rru_output_intp(true, 4, false);
    sig_rru_output_intp(true, 4, false);
    sig_rru_output_intp(true, 4, false);
    sig_rru_output_intp(true, 4, false);
    sig_rru_output_intp(true, 4, true);
}
#endif //~SIG_RRU

#ifdef SIG_RRU
void sig_rru_copy_cu_cost(ForceDecision force_decison, TComDataCU *p_best_cu, TComDataCU *p_temp_cu)
{
    if (force_decison == FORCE_KEEP)  // force current cu win
    {
      g_sigpool.rru_cu_cost[0] = 0.0;
      g_sigpool.rru_cu_cost[1] = MAX_DOUBLE;
    }
    else if (force_decison==FORCE_CHANGE)
    {
      g_sigpool.rru_cu_cost[0] = MAX_DOUBLE;  // force sub cu win
      g_sigpool.rru_cu_cost[1] = 0.0;
    }
    else
    {
      g_sigpool.rru_cu_cost[0] = p_best_cu->getTotalCost();
      g_sigpool.rru_cu_cost[1] = p_temp_cu->getTotalCost();
    }
}
#endif //~SIG_RRU

#if defined(SIG_RRU) || defined(SIG_PPU)
void sig_rru_output_cu_order(sig_ctx *p_ctx, TComDataCU *pcCU, int cu_x, int cu_y, int cu_w, int cu_h,
                             UInt uiAbsPartIdx, UInt uiDepth)
{
    PredMode pred = pcCU->getPredictionMode(uiAbsPartIdx);
    PartSize part = pcCU->getPartitionSize(uiAbsPartIdx);
    int skip = (int)(pcCU->getSkipFlag(uiAbsPartIdx));

    sigdump_output_fprint(p_ctx, "CU(X, Y, W, H) = %d, %d, %d, %d\n", cu_x, cu_y, cu_w, cu_h);
    sigdump_output_fprint(p_ctx, "sz = 0x%x\n", cu_w);
    sigdump_output_fprint(p_ctx, "pred_mode = 0x%x\n", (int)(pred));
    sigdump_output_fprint(p_ctx, "part_mode = 0x%x\n", (part == SIZE_2Nx2N) ? 0 : 1);
    sigdump_output_fprint(p_ctx, "skip_flag = 0x%x\n", skip);

    // get decoder qp
    int qp_x = cu_x & 0x3f;
    int qp_y = cu_y & 0x3f;
    int qp = pcCU->getQP(uiAbsPartIdx);
    if (g_sigpool.is_enable_rc)
        qp = (cu_w == 32) ? g_cu32_qp.get_data(qp_x, qp_y) : g_blk_qp.get_data(qp_x, qp_y);

    if (cu_w == 32 || part == SIZE_NxN)
    {
        // split TU
        TComTURecurse rTu(pcCU, uiAbsPartIdx, uiDepth);
        TComTURecurse tuRecurseChild(rTu, true);
        Int decode_qp[4] = { 0 };
        int tu_id = 0;

        // Get CU QP form each TU
        do
        {
            const UInt uiAbsPartIdx = tuRecurseChild.GetAbsPartIdxTU();
            decode_qp[tu_id] = pcCU->getQP(uiAbsPartIdx);
            tu_id++;
        } while (tuRecurseChild.nextSection(rTu));

        // error checking
        for (int i = 1; i < 4; i++)
        {
            if (decode_qp[i] != decode_qp[i - 1])
                assert(0);
        }

        sigdump_output_fprint(p_ctx, "qp = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x,\n",
                              qp, decode_qp[0], decode_qp[1], decode_qp[2], decode_qp[3]);
    }
    else
    {
        Int decode_qp = pcCU->getQP(uiAbsPartIdx);

        sigdump_output_fprint(p_ctx, "qp = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x,\n",
                              qp, decode_qp, decode_qp, decode_qp, decode_qp);
    }

    if (p_ctx == &g_sigpool.ppu_input_ctx)
    {
        TComPic *p_pic = pcCU->getPic();
        int mvdx_gt4 = 0;
        int mvdy_gt4 = 0;

        if (pred == MODE_INTER)
        {
            int cu_w_in8 = cu_w / 8;
            int cu_h_in8 = cu_h / 8;
            int mvx_q = pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(uiAbsPartIdx).getHor();
            int mvy_q = pcCU->getCUMvField(REF_PIC_LIST_0)->getMv(uiAbsPartIdx).getVer();

            // a0-a3
            for (int i = 0; i < cu_h_in8; i++)
            {
                int cur_left_idx = g_auiRasterToZscan[g_auiZscanToRaster[uiAbsPartIdx] + (((i * 8) / p_pic->getMinCUWidth()) * p_pic->getNumPartInCtuWidth())];
                UInt ax_idx = 0;
                const TComDataCU *p_cu_ax = pcCU->getPULeft(ax_idx, cur_left_idx);
                Bool is_avail_ax = p_cu_ax && p_cu_ax->isInter(ax_idx);

                if (is_avail_ax)
                {
                    int mvx_p = p_cu_ax->getCUMvField(REF_PIC_LIST_0)->getMv(ax_idx).getHor();
                    int mvy_p = p_cu_ax->getCUMvField(REF_PIC_LIST_0)->getMv(ax_idx).getVer();

                    mvdx_gt4 |= ((abs(mvx_q - mvx_p) >= 4 ? 1 : 0) << i);
                    mvdy_gt4 |= ((abs(mvy_q - mvy_p) >= 4 ? 1 : 0) << i);
                }
            }

            // b0-b3
            for (int i = 0; i < cu_w_in8; i++)
            {
                int cur_top_idx = g_auiRasterToZscan[g_auiZscanToRaster[uiAbsPartIdx] + ((i * 8) / p_pic->getMinCUWidth())];
                UInt bx_idx = 0;
                const TComDataCU *p_cu_bx = pcCU->getPUAbove(bx_idx, cur_top_idx);
                Bool is_avail_bx = p_cu_bx && p_cu_bx->isInter(bx_idx);

                if (is_avail_bx)
                {
                    int mvx_p = p_cu_bx->getCUMvField(REF_PIC_LIST_0)->getMv(bx_idx).getHor();
                    int mvy_p = p_cu_bx->getCUMvField(REF_PIC_LIST_0)->getMv(bx_idx).getVer();

                    mvdx_gt4 |= ((abs(mvx_q - mvx_p) >= 4 ? 1 : 0) << (i + 4));
                    mvdy_gt4 |= ((abs(mvy_q - mvy_p) >= 4 ? 1 : 0) << (i + 4));
                }
            }
        }

        sigdump_output_fprint(p_ctx, "mvd_gt4 = 0x%x, 0x%x\n", mvdx_gt4, mvdy_gt4);

        int cbf[4] = { 0 };
        int cbf_num = (part == SIZE_NxN || cu_w == 32) ? 4 : 1;
        int tu_size = (part == SIZE_NxN || cu_w == 32) ? (cu_w >> 1) : cu_w;
        int tr_depth = (part == SIZE_NxN || cu_w == 32) ? 1 : 0;
        int tu_size_offset = tu_size / p_pic->getMinCUWidth();
        int stride = p_pic->getNumPartInCtuWidth();

        for (int i = 0; i < 4; i++)
        {
            if (i >= cbf_num)
            {
                cbf[i] = cbf[0];
                continue;
            }

            int tu_rs_idx = g_auiZscanToRaster[uiAbsPartIdx] + ((i & 0x1) * tu_size_offset) +
                            ((i >> 1) & 0x1) * tu_size_offset * stride;
            int tu_abs_idx = g_auiRasterToZscan[tu_rs_idx];
            cbf[i] = pcCU->getCbf(tu_abs_idx, COMPONENT_Y, tr_depth);
        }

        sigdump_output_fprint(p_ctx, "luma_cbf = 0x%x, 0x%x, 0x%x, 0x%x\n",
                              cbf[0], cbf[1], cbf[2], cbf[3]);
    }
}
#endif //~SIG_RRU || SIG_PPU

#ifdef SIG_RRU_DEBUG
void sig_rru_copy_iq_in_temp_buf(ComponentID compID, TCoeff *p_coeff, int size, int tu_x, int tu_y)
{
    TCoeff *p_coe = p_coeff;
    int buf_idx = (compID == COMPONENT_Y) ? 0 : 1;
    TCoeff *p_iq_in  = g_sigpool.p_rru_iq_in_temp[buf_idx];

    if (compID == COMPONENT_Cr)
    {
        int cb_size = size * size;
        if (g_sigpool.cu_width == 32)
            cb_size = (cb_size << 2);
        p_iq_in  = &g_sigpool.p_rru_iq_in_temp[buf_idx][cb_size];
    }

    if (g_sigpool.cu_width == 32)
    {
        int stride = (compID == COMPONENT_Y) ? 32 : 16;
        int offset = size * stride;

        if (tu_x > 0)
            p_iq_in += size;
        if (tu_y > 0)
            p_iq_in += offset;

        int copy_size = sizeof(int) * size;

        for (int i = 0; i < size; i++)
        {
            memcpy(p_iq_in, p_coe, copy_size);
            p_coe += size;
            p_iq_in += stride;
        }
    }
    else
    {
        int byte_size = sizeof(int) * size * size;
        memcpy(p_iq_in, p_coe, byte_size);
    }
}
#endif //SIG_RRU_DEBUG

#ifdef SIG_PRU
void sig_pru_output_cmd(int i4_thr, int i4_tar, bool is_i_slice)
{
    sig_pru_st *p_pru_st = g_sigpool.p_pru_st;

    if (p_pru_st)
    {
        sig_ctx *p_ctx = &p_pru_st->pru_cmd_ctx;
        sigdump_output_fprint(p_ctx, "reg_src_fmt = 0\n");
        sigdump_output_fprint(p_ctx, "reg_pic_width_m1 = %x\n", g_sigpool.width - 1);
        sigdump_output_fprint(p_ctx, "reg_pic_height_m1 = %x\n", g_sigpool.height - 1);
        int pitch = cvi_mem_align(g_sigpool.width, 32);
        sigdump_output_fprint(p_ctx, "reg_src_luma_pit = %x\n", pitch);
        sigdump_output_fprint(p_ctx, "reg_src_padding_en = %x\n", (g_sigpool.width == pitch) ? 0 : 1);
        sigdump_output_fprint(p_ctx, "reg_src_luma_pad_val = 0\n");
        sigdump_output_fprint(p_ctx, "reg_i4_madi_thr = %x\n", i4_thr);
        sigdump_output_fprint(p_ctx, "reg_pic_i4_term_target = %x\n", i4_tar);
        sigdump_output_fprint(p_ctx, "reg_i_slice = %x\n", (is_i_slice == true) ? 1 : 0);
    }
}
#endif //~SIG_PRU

#ifdef SIG_PRU
void sig_pru_output_madi_ctu(int blk64_x, int blk64_y)
{
    int frame_width  = g_sigpool.width;
    int frame_height = g_sigpool.height;
    sig_pru_st *p_pru_st = g_sigpool.p_pru_st;

    if (p_pru_st)
    {
        sig_ctx *p_ctx = &p_pru_st->pru_madi_ctx;
        int blk32_id = p_pru_st->madi_blk32_id;
        // CTU64
        for (int blk32_y = blk64_y; blk32_y < blk64_y + 64; blk32_y += 32)
        {
            for (int blk32_x = blk64_x; blk32_x < blk64_x + 64; blk32_x += 32)
            {
                if (blk32_y >= frame_height || blk32_x >= frame_width)
                    continue;

                sigdump_output_fprint(p_ctx, "BLK32(%d, %d) = #%d\n", blk32_x, blk32_y, blk32_id++);

                int madi_16[4] = { 0 };
                int lum_16[4] = { 0 };
                int madi_8[16] = { 0 };
                int lum_8[16] = { 0 };
                get_madi_blk32(blk32_x, blk32_y, madi_16, lum_16, madi_8, lum_8);

                sigdump_output_fprint(p_ctx, "// Mean\n");
                sigdump_output_fprint(p_ctx, "// BLK16\n");
                sigdump_output_fprint(p_ctx, "//%02x %02x %02x %02x\n",
                                      lum_16[0], lum_16[1], lum_16[2], lum_16[3]);

                for (int i = 0; i < 4; i++)
                {
                    int idx = (i << 2);
                    sigdump_output_fprint(p_ctx, "// BLK8 = #%d\n", i);
                    sigdump_output_fprint(p_ctx, "//%02x %02x %02x %02x\n",
                                          lum_8[idx], lum_8[idx + 1], lum_8[idx + 2], lum_8[idx + 3]);
                }

                sigdump_output_fprint(p_ctx, "// Madi\n");
                sigdump_output_fprint(p_ctx, "// BLK16\n");
                sigdump_output_fprint(p_ctx, "//%02x %02x %02x %02x\n",
                                      madi_16[0], madi_16[1], madi_16[2], madi_16[3]);

                for (int i = 0; i < 4; i++)
                {
                    int idx = (i << 2);
                    sigdump_output_fprint(p_ctx, "// BLK8 = #%d\n", i);
                    sigdump_output_fprint(p_ctx, "//%02x %02x %02x %02x\n",
                                          madi_8[idx], madi_8[idx + 1], madi_8[idx + 2], madi_8[idx + 3]);
                }

                int hist[4] = { 0 };
                int hist_idx = 0;

                for (int blk16_y = blk32_y; blk16_y < blk32_y + 32; blk16_y += 16)
                {
                    for (int blk16_x = blk32_x; blk16_x < blk32_x + 32; blk16_x += 16)
                    {
                        int idx = (((blk16_y & 0x3f) >> 4) << 2) + ((blk16_x & 0x3f) >> 4);
                        hist[hist_idx] = g_sigpool.p_pru_st->stat_early_term[idx];

                        if (hist[hist_idx] == 0xffffffff)
                            hist[hist_idx] = g_sigpool.p_pru_st->last_stat_early_term;
                        else
                            g_sigpool.p_pru_st->last_stat_early_term = hist[hist_idx];

                        hist_idx++;
                    }
                }

                sigdump_output_fprint(p_ctx, "// BLK8 Stat_Early_Term\n");
                sigdump_output_fprint(p_ctx, "//%02x %02x %02x %02x\n",
                                      hist[0], hist[1], hist[2], hist[3]);
            }
        }

        p_pru_st->madi_blk32_id = blk32_id;
    }
}
#endif //~SIG_PRU

#ifdef SIG_PRU
void sig_pru_output_hist()
{
    // MADI hist
    sig_ctx *p_ctx = &g_sigpool.p_pru_st->pru_hist_ctx;
    for (int i = 0; i < (1 << MADI_HIST_PREC); i++)
    {
        sigdump_output_fprint(p_ctx, "reg_hist_madi8_%d = %x\n", i, g_madi8_hist[i]);
    }
}
#endif //~SIG_PRU

#ifdef SIG_PRU
#define EDGE_INFO_SIZE         21
#define EDGE_INFO_BLK8_OFFSET   1
#define EDGE_INFO_BLK4_OFFSET   5
EDGE_DET_ST g_edge_info[EDGE_INFO_SIZE];
#endif //~SIG_PRU

#ifdef SIG_PRU
void reset_edge_info()
{
    memset(g_edge_info, 0xff, sizeof(g_edge_info));
}

void sig_pru_output_edge(int x, int y, int blk_size, EDGE_DET_ST *p_edge_st)
{
    if (blk_size == 16 ||
        ((g_sigpool.slice_type == I_SLICE) && (blk_size != 4)) ||
        ((g_sigpool.slice_type != I_SLICE) && (blk_size == 4)))
    {
        return;
    }

    int frame_width  = g_sigpool.width;
    int frame_height = g_sigpool.height;
    bool is_dump = false;
    int end_x = x + blk_size;
    int end_y = y + blk_size;

    if ( (((end_x & 0xf) == 0) || (end_x == frame_width)) &&
         (((end_y & 0xf) == 0) || (end_y == frame_height)))
    {
        is_dump = true;
    }

    sig_pru_st *p_pru_st = g_sigpool.p_pru_st;

    if (p_pru_st && is_dump)
    {
        sig_ctx *p_ctx = &p_pru_st->pru_edge_ctx;
        sigdump_output_fprint(p_ctx, "BLK16 = #%d\n", p_pru_st->blk16_id);
        p_pru_st->blk16_id++;

        sigdump_output_fprint(p_ctx, "// Edg_det_Mode\n");
        sigdump_output_fprint(p_ctx, "// I16 (%d, %d)\n", g_edge_info[0].x, g_edge_info[0].y);
        sigdump_output_fprint(p_ctx, "%02x\n", g_edge_info[0].index);
        sigdump_output_fprint(p_ctx, "// I8 (%d, %d) (%d, %d) (%d, %d) (%d, %d)\n",
                              g_edge_info[1].x, g_edge_info[1].y,
                              g_edge_info[2].x, g_edge_info[2].y,
                              g_edge_info[3].x, g_edge_info[3].y,
                              g_edge_info[4].x, g_edge_info[4].y);
        sigdump_output_fprint(p_ctx, "%02x %02x %02x %02x\n",
                              g_edge_info[1].index, g_edge_info[2].index,
                              g_edge_info[3].index, g_edge_info[4].index);

        for (int i = 0; i < 4; i++)
        {
            int idx = EDGE_INFO_BLK4_OFFSET + (i << 2);
            sigdump_output_fprint(p_ctx, "// I4 = #%d, (%d, %d) (%d, %d) (%d, %d) (%d, %d)\n", i,
                                  g_edge_info[idx    ].x, g_edge_info[idx    ].y,
                                  g_edge_info[idx + 1].x, g_edge_info[idx + 1].y,
                                  g_edge_info[idx + 2].x, g_edge_info[idx + 2].y,
                                  g_edge_info[idx + 3].x, g_edge_info[idx + 3].y);
            sigdump_output_fprint(p_ctx, "%02x %02x %02x %02x\n",
                                  g_edge_info[idx    ].index, g_edge_info[idx + 1].index,
                                  g_edge_info[idx + 2].index, g_edge_info[idx + 3].index);
        }

        sigdump_output_fprint(p_ctx, "// Edg_det_Value\n");
        sigdump_output_fprint(p_ctx, "// I16\n");
        sigdump_output_fprint(p_ctx, "%04x\n", g_edge_info[0].strength);
        sigdump_output_fprint(p_ctx, "// I8\n");
        sigdump_output_fprint(p_ctx, "%04x %04x %04x %04x\n",
                              g_edge_info[1].strength, g_edge_info[2].strength,
                              g_edge_info[3].strength, g_edge_info[4].strength);

        for (int i = 0; i < 4; i++)
        {
            int idx = EDGE_INFO_BLK4_OFFSET + (i << 2);
            sigdump_output_fprint(p_ctx, "// I4 = #%d\n", i);
            sigdump_output_fprint(p_ctx, "%04x %04x %04x %04x\n",
                                  g_edge_info[idx    ].strength, g_edge_info[idx + 1].strength,
                                  g_edge_info[idx + 2].strength, g_edge_info[idx + 3].strength);
        }

        reset_edge_info();
    }
}
#endif //~SIG_PRU

#ifdef SIG_PRU
void sig_pru_set_edge_info(int x, int y, int blk_size, int es_index, int es_strength)
{
    int info_idx = 0;
    int blk8_idx = ((x & 0x8) >> 3) + ((y & 0x8) >> 2);
    int blk4_idx = ((x & 0x4) >> 2) + ((y & 0x4) >> 1);

    if (x == 0 && y == 0 && blk_size == 16)
        reset_edge_info();

    if (blk_size == 8)
        info_idx = EDGE_INFO_BLK8_OFFSET + blk8_idx;
    else if (blk_size == 4)
        info_idx = EDGE_INFO_BLK4_OFFSET + (blk8_idx << 2) + blk4_idx;

    g_edge_info[info_idx].x        = x;
    g_edge_info[info_idx].y        = y;
    g_edge_info[info_idx].index    = es_index;
    g_edge_info[info_idx].strength = es_strength;

    sig_pru_output_edge(x, y, blk_size, g_edge_info);
}
#endif //~SIG_PRU

#ifdef SIG_PRU
void sig_pru_set_edge_info_i4_termination(int x, int y)
{
    int blk8_idx = ((x & 0x8) >> 3) + ((y & 0x8) >> 2);
    int blk4_idx = ((x & 0x4) >> 2) + ((y & 0x4) >> 1);
    int info_idx = EDGE_INFO_BLK4_OFFSET + (blk8_idx << 2) + blk4_idx;

    for (int b4_y = 0; b4_y < 8; b4_y += 4)
    {
        for (int b4_x = 0; b4_x < 8; b4_x += 4)
        {
            g_edge_info[info_idx].x        = x + b4_x;
            g_edge_info[info_idx].y        = y + b4_y;
            g_edge_info[info_idx].index    = 0x7;
            g_edge_info[info_idx].strength = 0;
            info_idx++;
        }
    }

    sig_pru_output_edge(x + 4, y + 4, 4, g_edge_info);
}
#endif //~SIG_PRU

#ifdef SIG_CCU
void sig_ccu_resi(TComTU &rTu)
{
  TComDataCU *pcCU = rTu.getCU();
  sigdump_output_bin(&g_sigpool.ccu_resi_ctx, (unsigned char *)g_sigpool.ccu_csbf, sizeof(g_sigpool.ccu_csbf));
  sigdump_output_bin(&g_sigpool.ccu_resi_ctx, g_sigpool.ccu_scan_idx, sizeof(g_sigpool.ccu_scan_idx));

  sig_output_tu_idx(&g_sigpool.ccu_resi_txt_ctx, rTu, COMPONENT_Y);
  sigdump_output_fprint(&g_sigpool.ccu_resi_txt_ctx, "csbf %x,%x,%x\n", g_sigpool.ccu_csbf[0], g_sigpool.ccu_csbf[1], g_sigpool.ccu_csbf[2]);
  sigdump_output_fprint(&g_sigpool.ccu_resi_txt_ctx, "scan_idx %x,%x\n", g_sigpool.ccu_scan_idx[0], g_sigpool.ccu_scan_idx[1]);

  for (UInt ch = 0; ch < 3; ch++)
  {
    ComponentID compID = ComponentID(ch);
    if (rTu.ProcessComponentSection(compID) && g_sigpool.ccu_last_subset[ch] != -1)
    {
      TCoeff *pcCoefTU = (pcCU->getCoeff(compID) + rTu.getCoefficientOffset(compID));
      for (Int iSubSet = g_sigpool.ccu_last_subset[ch]; iSubSet >= 0; iSubSet--)
      {
        TUEntropyCodingParameters codingParameters;
        getTUEntropyCodingParameters(codingParameters, rTu, compID);

        Int iCGBlkPos = codingParameters.scanCG[iSubSet];
        Int iCGPosY = iCGBlkPos / codingParameters.widthInGroups;
        Int iCGPosX = iCGBlkPos - (iCGPosY * codingParameters.widthInGroups);
        if (((g_sigpool.ccu_csbf[ch] >> iCGBlkPos) & 1) != 0)
        {
          TCoeff *pcCoefCG = pcCoefTU + (iCGPosX * MLS_CG_SIZE) + iCGPosY * MLS_CG_SIZE * rTu.getRect(compID).width;
          sigdump_output_fprint(&g_sigpool.ccu_resi_txt_ctx, "[comp%d][iSubSet %d]\n", compID, iSubSet);
          for (int idx_y = 0; idx_y < MLS_CG_SIZE; idx_y++)
          {
            for (int idx_x = 0; idx_x < MLS_CG_SIZE; idx_x++)
            {
              TCoeff *pcCoef = pcCoefCG + idx_x + idx_y * rTu.getRect(compID).width;
              sigdump_output_bin(&g_sigpool.ccu_resi_ctx, (unsigned char *)pcCoef, 2);
              sigdump_output_fprint(&g_sigpool.ccu_resi_txt_ctx, "%04x,", *pcCoef & 0xffff);
            }
            sigdump_output_fprint(&g_sigpool.ccu_resi_txt_ctx, "\n");
          }
        }
      }
    }
  }
}

void sig_ccu_dummy_resi(TComDataCU* pcCU, const UInt uiAbsPartIdx, UInt uiDepth)
{
    const UInt maxCUWidth  = pcCU->getSlice()->getSPS()->getMaxCUWidth();
    UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
    UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
    Int tu_cnt = 1;
    Int size = maxCUWidth >> uiDepth;
    if (size == 32) {
        tu_cnt = 4;
        size = 16;
    }

    g_sigpool.ccu_csbf[0] = g_sigpool.ccu_csbf[1] = g_sigpool.ccu_csbf[2] = 0;
    g_sigpool.ccu_scan_idx[0] = g_sigpool.ccu_scan_idx[1] = 0;

    for (int i = 0; i < tu_cnt; i++)
    {
        int idx_x = uiLPelX + (i & 1) * 16;
        int idx_y = uiTPelY + (i >> 1) * 16;
        sigdump_output_bin(&g_sigpool.ccu_resi_ctx, (unsigned char *)g_sigpool.ccu_csbf, sizeof(g_sigpool.ccu_csbf));
        sigdump_output_bin(&g_sigpool.ccu_resi_ctx, g_sigpool.ccu_scan_idx, sizeof(g_sigpool.ccu_scan_idx));

        sigdump_output_fprint(&g_sigpool.ccu_resi_txt_ctx, "# [TU info] (TU_X, TU_Y) = (%d, %d) (%d x %d)\n",
        idx_x, idx_y, size, size);
        sigdump_output_fprint(&g_sigpool.ccu_resi_txt_ctx, "csbf %x,%x,%x\n", g_sigpool.ccu_csbf[0], g_sigpool.ccu_csbf[1], g_sigpool.ccu_csbf[2]);
        sigdump_output_fprint(&g_sigpool.ccu_resi_txt_ctx, "scan_idx %x,%x\n", g_sigpool.ccu_scan_idx[0], g_sigpool.ccu_scan_idx[1]);
    }
}
#endif

#ifdef SIG_CABAC
void sig_output_tu_idx(const sig_ctx *ctx, TComTU &rTu, const ComponentID compID)
{
    TComDataCU *pcCU=rTu.getCU();
    const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU(compID);
    UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
    UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
    sigdump_output_fprint(ctx, "# [TU info] (TU_X, TU_Y) = (%d, %d) (%d x %d)\n",
    uiLPelX, uiTPelY, rTu.getRect(compID).width, rTu.getRect(compID).height);
}

void sig_cabac_bits_statis(TComTU &rTu, const ComponentID compID, UInt NumberOfWrittenBits, Bool bHaveACodedBlock)
{
    TComDataCU *pcCU=rTu.getCU();
    const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU();
    UInt bits_res = 0;
    sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "comp = %d, blk_sz = %d, is_intra = %d\n",
    compID, rTu.getRect(compID).width, pcCU->isIntra(uiAbsPartIdx));
    if (bHaveACodedBlock) {
        bits_res = NumberOfWrittenBits - g_sigpool.tu_start_bits_pos;
    } else {
        g_sigpool.bits_header = NumberOfWrittenBits - g_sigpool.tu_start_bits_pos;
    }

    if (g_sigpool.cabac_bits_statis_tu_ctx.fp)
        g_sigpool.cur_bits_offset = ftell(g_sigpool.cabac_bits_statis_tu_ctx.fp);

    sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "bits_header = %x\n", g_sigpool.bits_header);
    sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "bits_res = %x\n", bits_res);
    sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "bits_accu = %x\n", NumberOfWrittenBits - 1);

    g_sigpool.bits_header_tmp = g_sigpool.bits_header;
    g_sigpool.bits_res_tmp = bits_res;
    g_sigpool.bits_accu_tmp = NumberOfWrittenBits - 1;
    g_sigpool.bits_header = 0;
    g_sigpool.tu_start_bits_pos = NumberOfWrittenBits;
}

void sig_cabac_dummy_tu(TComDataCU* pcCU, UInt NumberOfWrittenBits, const UInt uiAbsPartIdx, UInt uiDepth)
{
    const UInt maxCUWidth  = pcCU->getSlice()->getSPS()->getMaxCUWidth();
    UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
    UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
    UInt tu_cnt = 1;
    Int size = maxCUWidth >> uiDepth;
    if (size == 32) {
        tu_cnt = 4;
        size = 16;
    }
    for (Int i = 0; i < tu_cnt; i++)
    {
        int idx_x = uiLPelX + (i & 1) * 16;
        int idx_y = uiTPelY + (i >> 1) * 16;
        for (Int ch = 0; ch < 3; ch++)
        {
            if (ch == 0)
            {
                sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, "# [TU info] (TU_X, TU_Y) = (%d, %d) (%d x %d)\n",
                idx_x, idx_y, size, size);
                sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, "blk_sz = %d, scan_idx = %d, comp = %d, is_intra = %d, last_x = %d, last_y = %d, csbf = %d\n",
                0, 0, 0, 0, 0, 0, 0);
            }
            Int blk_size = ch == 0 ? size : size / 2;
            sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "# [TU info] (TU_X, TU_Y) = (%d, %d) (%d x %d)\n",
            idx_x, idx_y, blk_size, blk_size);

            sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "comp = %d, blk_sz = %d, is_intra = %d\n", ch, blk_size, 0);
            g_sigpool.bits_header = NumberOfWrittenBits - g_sigpool.tu_start_bits_pos;

            if (g_sigpool.cabac_bits_statis_tu_ctx.fp)
                g_sigpool.cur_bits_offset = ftell(g_sigpool.cabac_bits_statis_tu_ctx.fp);

            sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "bits_header = %x\n", g_sigpool.bits_header);
            sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "bits_res = %x\n", 0);
            sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "bits_accu = %x\n", NumberOfWrittenBits - 1);
            g_sigpool.bits_header_tmp = g_sigpool.bits_header;
            g_sigpool.bits_res_tmp = 0;
            g_sigpool.bits_accu_tmp = NumberOfWrittenBits - 1;
            g_sigpool.bits_header = 0;
            g_sigpool.tu_start_bits_pos = NumberOfWrittenBits;
        }
    }
}

void sig_cabac_syntax_init_tab()
{
    sig_ctx *p_ctx = &g_sigpool.cabac_syntax_ctx;
    sigdump_output_fprint(p_ctx, "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);
    sigdump_output_fprint(p_ctx, "pic_w = %d\n", g_sigpool.width);
    sigdump_output_fprint(p_ctx, "pic_h = %d\n", g_sigpool.height);

    for (int predID = 0; predID < 2; predID++) {
        for (int is_c = 0; is_c < 2; is_c ++) {
            for (int compID = 0; compID < 2; compID++) {
                for (Int log2_size_m2 = 0; log2_size_m2 <= 2; log2_size_m2++) {
                    const int *tmp = is_c ? &g_significantBias[g_scale_D][compID][predID][log2_size_m2][0]
                                          : &g_significantScale[g_scale_D][compID][predID][log2_size_m2][0];
                    sigdump_output_fprint(p_ctx, "(%x, %x),", 0x3ffff & tmp[0], 0x3ffff & tmp[1]);
                }
                sigdump_output_fprint(p_ctx, "\n");
            }
        }
    }
    Int lr[2] = { 0 };
    for(Int l=0; l<2; l++)
        lr[l] = ((1 << LMS_LR_FRAC_BD)*g_significantScale_LR[l]);
    sigdump_output_fprint(p_ctx, "%x, %x\n", 0xffff & lr[0], 0xffff & lr[1]);

    Int m_max = (1 << RESI_MDL_FRAC_BD) * g_significantScale_max;
    Int m_min = (1 << RESI_MDL_FRAC_BD) * g_significantScale_min;
    Int c_max = (1 << RESI_MDL_FRAC_BD) * g_significantBias_clip;
    Int c_min = -c_max;
    sigdump_output_fprint(p_ctx, "%x,%x\n", 0x3ffff & m_max, 0x3ffff & m_min);
    sigdump_output_fprint(p_ctx, "%x,%x\n", 0x3ffff & c_max, 0x3ffff & c_min);

    sigdump_output_fprint(p_ctx, "# si_lut (mps, lps)\n");
    for (int i = 0; i < 16; i++)
    {
        UInt mps = (g_hwEntropyBits[i][0] & 0xfff);
        UInt lps = (g_hwEntropyBits[i][1] & 0x3fff);
        sigdump_output_fprint(p_ctx, "(%x, %x) # idx=%d\n", mps, lps, i);
    }
}

void sig_cabac_para_update(UInt tuSig0Bin, UInt tuSig1Bin, Int org_b0Scale, Int org_b1Scale, Int org_b0Bias, Int org_b1Bias, UInt tuSig0FracBit, UInt tuSig1FracBit)
{
    Int m[2], c[2], Rsig[2];
    UInt BCsig[2] = {tuSig0Bin, tuSig1Bin};
    for (int i = 0; i < 2; i++) {
        m[i] = (i == 0) ? org_b0Scale : org_b1Scale;
        c[i] = (i == 0) ? org_b0Bias : org_b1Bias;
        Rsig[i] = m[i] * BCsig[i] + c[i];
    }
    sig_ctx *p_ctx = &g_sigpool.cabac_para_update_ctx;
    sigdump_output_fprint(p_ctx, "BCsig0=%d, BCsig1=%d, Rsig0=%d, Rsig1=%d\n", BCsig[0], BCsig[1], Rsig[0], Rsig[1]);
    sigdump_output_fprint(p_ctx, "Rsival0=%d, Rsival1=%d\n", tuSig0FracBit, tuSig1FracBit);

    for (int predID = 0; predID < 2; predID++) {
        for (int is_c = 0; is_c < 2; is_c ++) {
            for (int compID = 0; compID < 2; compID++) {
                for (Int log2_size_m2 = 0; log2_size_m2 <= 2; log2_size_m2++) {
                    const int *tmp = is_c ? &g_significantBias[0][compID][predID][log2_size_m2][0]
                                          : &g_significantScale[0][compID][predID][log2_size_m2][0];
                    sigdump_output_fprint(p_ctx, "(%x, %x),", 0x3ffff & tmp[0], 0x3ffff & tmp[1]);
                }
                sigdump_output_fprint(p_ctx, "\n");
            }
        }
    }

}
#endif //~SIG_CABAC

#ifdef SIG_CCU
void sigdump_ccu_md_4x4(TComDataCU*& rpcBestCU)
{
  int ctx_idx = 0;
  sig_ctx *ccu_ctx = &g_sigpool.ccu_ctx[ctx_idx];
  sigdump_output_fprint(ccu_ctx, "# Mode Decision\n");
  sigdump_output_fprint(ccu_ctx, "is_intra_win = %d\n", rpcBestCU->isIntra(0));
  sigdump_output_fprint(ccu_ctx, "is_skip_win = %x\n", rpcBestCU->isSkipped(0));
  sigdump_output_fprint(ccu_ctx, "is_mrg_win = %x\n", rpcBestCU->getMergeFlag(0));
  sigdump_output_fprint(ccu_ctx, "mrg_cand_idx = %x\n", rpcBestCU->getMergeIndex(0));
  sigdump_output_fprint(ccu_ctx, "scan_idx = %d, %d, %d, %d, %d\n", g_sigpool.scan_idx_4x4[0], g_sigpool.scan_idx_4x4[1], g_sigpool.scan_idx_4x4[2], g_sigpool.scan_idx_4x4[3], g_sigpool.scan_idx_4x4[4]);

}

void sigdump_ccu_md(TComDataCU*& rpcBestCU, Bool isSkipWin)
{
  int ctx_idx = sig_pat_blk_size_4(rpcBestCU->getWidth(0));
  sig_ctx *ccu_ctx = &g_sigpool.ccu_ctx[ctx_idx];
  sigdump_output_fprint(ccu_ctx, "# Mode Decision\n");
  sigdump_output_fprint(ccu_ctx, "is_intra_win = %d\n", rpcBestCU->isIntra(0));
  sigdump_output_fprint(ccu_ctx, "is_skip_win = %x\n", isSkipWin ? 1 : 0);
  sigdump_output_fprint(ccu_ctx, "is_mrg_win = %x\n", g_sigpool.rru_is_mrg_win);
  sigdump_output_fprint(ccu_ctx, "mrg_cand_idx = %x\n", g_sigpool.rru_merge_cand);
  sigdump_output_fprint(ccu_ctx, "scan_idx = %d, %d\n", g_sigpool.scan_idx[0], g_sigpool.scan_idx[1]);
}

void sigdump_ccu_qp(TComDataCU *pCu, UInt uiWidth, Int64 Lambda, Int64 SqrtLambda)
{
  int blk_size = sig_pat_blk_size_4(uiWidth);
  int QP[3] = {0};
  for (int i = 0; i < pCu->getPic()->getNumberValidComponents(); i++)
  {
    const QpParam cQP(*pCu, ComponentID(i));
    QP[i] = cQP.Qp;
  }
  sigdump_output_fprint(&g_sigpool.ccu_ctx[blk_size], "# QP\ncur_qp = 0x%x,0x%x,0x%x\n", QP[2], QP[1], QP[0]);
  sigdump_output_fprint(&g_sigpool.ccu_ctx[blk_size], "cur_cu_lambda = 0x%x\n", (UInt)Lambda);
  sigdump_output_fprint(&g_sigpool.ccu_ctx[blk_size], "cur_cu_sqrt_lambda = 0x%x\n", (UInt)SqrtLambda);
}

void sigdump_irpu_cu_golden_start(string type)
{
  sig_ctx *irpu_golden_frm_ctx;
  int size_idx = g_sigpool.me_golden.size_idx - 1;
  assert(size_idx >= 0 && size_idx <= 2);
  irpu_golden_frm_ctx = &g_sigpool.irpu_golden_frm_ctx[size_idx];

  int refIdx = g_sigpool.me_golden.refIdx;
  int list_idx = g_sigpool.me_golden.amvp_list_idx;
  int mv_x = g_sigpool.amvp_list[refIdx][list_idx].mvx;
  int mv_y = g_sigpool.amvp_list[refIdx][list_idx].mvy;
  int AMVPrefIdx = g_sigpool.amvp_list[refIdx][list_idx].ref_idx;
  int mvdx_mvl = (type == "Merge") ? g_sigpool.me_golden.mv_level : (g_sigpool.me_golden.mvd_x & 0xffff);

  int nb_num = (1 << size_idx);
  int cur_mv_x = (type == "Merge") ? mv_x : g_sigpool.me_golden.mv_x_ori;
  int cur_mv_y = (type == "Merge") ? mv_y : g_sigpool.me_golden.mv_y_ori;
  int mvdx = 0;
  int mvdy = 0;
  int bs = 0;
  for (int i = 0; i < nb_num; i++)
  {
      for (int ab = 0; ab < 2; ab++)
      {
          if (g_sigpool.irpu_nb_mv[ab][i][0] == INT32_MAX)
            continue;

          mvdx = abs(cur_mv_x - g_sigpool.irpu_nb_mv[ab][i][0]);
          mvdy = abs(cur_mv_y - g_sigpool.irpu_nb_mv[ab][i][1]);
          bs |= (((mvdx >=4 || mvdy >= 4) ? 1 : 0) << (i + ab * 4));
      }
  }

  sigdump_output_fprint(irpu_golden_frm_ctx, "# [CU info] (CU_X, CU_Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
  sigdump_output_fprint(irpu_golden_frm_ctx, "$ %s, %llx, %llx, %02x, %02x, %02x, %02x, %04x, %04x, %02x, %04x, %04x, ",
    type.c_str(),
    g_sigpool.me_golden.SSE[0] + g_sigpool.me_golden.SSE[1],
    g_sigpool.me_golden.SATD[0] + g_sigpool.me_golden.SATD[1],
    g_sigpool.me_golden.poc_diff & 0xff, g_sigpool.me_golden.isLongTerm & 0xff,
    list_idx & 0xff, AMVPrefIdx & 0xff,
    g_sigpool.me_golden.mvd_y & 0xffff, mvdx_mvl, bs & 0xff,
    mv_y & 0xffff, mv_x & 0xffff);
  sigdump_output_fprint(irpu_golden_frm_ctx, "%s (%02x,%04x,%04x), %s (%02x,%04x,%04x), %s (%02x,%04x,%04x), %s (%02x,%04x,%04x), %02x\n",
    g_sigpool.amvp_list[0][0].dir, g_sigpool.amvp_list[0][0].ref_idx & 0xff, g_sigpool.amvp_list[0][0].mvy & 0xffff, g_sigpool.amvp_list[0][0].mvx & 0xffff,
    g_sigpool.amvp_list[0][1].dir, g_sigpool.amvp_list[0][1].ref_idx & 0xff, g_sigpool.amvp_list[0][1].mvy & 0xffff, g_sigpool.amvp_list[0][1].mvx & 0xffff,
    g_sigpool.amvp_list[1][0].dir, g_sigpool.amvp_list[1][0].ref_idx & 0xff, g_sigpool.amvp_list[1][0].mvy & 0xffff, g_sigpool.amvp_list[1][0].mvx & 0xffff,
    g_sigpool.amvp_list[1][1].dir, g_sigpool.amvp_list[1][1].ref_idx & 0xff, g_sigpool.amvp_list[1][1].mvy & 0xffff, g_sigpool.amvp_list[1][1].mvx & 0xffff,
    g_sigpool.me_golden.size_idx);
}
static void sigdump_fprint_ccu_intra(string type, sig_ctx *ccu_ctx, bool is4x4, const int *val)
{
    sigdump_output_fprint(ccu_ctx, "%s = 0x%x", type.c_str(), val[0]);
    if (is4x4) {
        sigdump_output_fprint(ccu_ctx, ",0x%x,0x%x,0x%x", val[1],val[2],val[3]);
    }
    sigdump_output_fprint(ccu_ctx, "\n");
}

void cvi_fill_scan_idx(bool is4x4, const TComDataCU *rpcTempCU)
{
    if (is4x4) {
        for (int i = 0; i < 4; i++) {
            g_sigpool.scan_idx_4x4[i] = rpcTempCU->getCoefScanIdx(i, 4, 4, COMPONENT_Y);
        }
        g_sigpool.scan_idx_4x4[4] = rpcTempCU->getCoefScanIdx(0, 4, 4, COMPONENT_Cb);
    } else {
        g_sigpool.scan_idx[0] = rpcTempCU->getCoefScanIdx(0, rpcTempCU->getWidth(0), rpcTempCU->getHeight(0), COMPONENT_Y);
        g_sigpool.scan_idx[1] = rpcTempCU->getCoefScanIdx(0, rpcTempCU->getWidth(0) / 2, rpcTempCU->getHeight(0) / 2, COMPONENT_Cb);
    }
}
void sigdump_ccu_cu_golden_start(string type)
{
    sig_ctx *ccu_ctx;
    string prefix = (type == "MERGE") ? "mrg" : ((type == "INTRA") ? "INTRA" : "fme");
    if (type.compare("INTRA") == 0) {
        int size_idx = g_sigpool.intra_golden.size_idx;
        ccu_ctx = &g_sigpool.ccu_ctx[size_idx];
        const sig_intra_golden_st *p_intra_gld = &g_sigpool.intra_golden;
        bool is4x4 = size_idx == 0;
        int chroma_pred_mode = p_intra_gld->chroma_pred_mode;
        if (chroma_pred_mode == DM_CHROMA_IDX) {
            chroma_pred_mode = p_intra_gld->luma_pred_mode[0];
        }
        sigdump_output_fprint(ccu_ctx, "# %s\n", type.c_str());
        sigdump_fprint_ccu_intra("luma_pred_mode", ccu_ctx, is4x4, p_intra_gld->luma_pred_mode);
        sigdump_output_fprint(ccu_ctx, "chroma_pred_mode = 0x%x\n", chroma_pred_mode);
        sigdump_output_fprint(ccu_ctx, "intra_chroma_pred_mode = 0x%x\n", p_intra_gld->intra_chroma_pred_mode);
        sigdump_fprint_ccu_intra("rem_luma_pred_mode", ccu_ctx, is4x4, p_intra_gld->rem_luma_pred_mode);
        sigdump_fprint_ccu_intra("mpm_idx", ccu_ctx, is4x4, p_intra_gld->mpm_idx);
        sigdump_fprint_ccu_intra("prev_luma_pred_flag", ccu_ctx, is4x4, p_intra_gld->prev_luma_pred_flag);
    } else {
        int size_idx = g_sigpool.me_golden.size_idx;
        ccu_ctx = &g_sigpool.ccu_ctx[size_idx];
        sig_me_golden_st *p_me_golden = &g_sigpool.me_golden;
        sigdump_output_fprint(ccu_ctx, "# %s\n", type.c_str());
        sigdump_output_fprint(ccu_ctx, "%s_mvxy = 0x%x,0x%x\n", prefix.c_str(), p_me_golden->mv_y, p_me_golden->mv_x);
        if (type.compare("MERGE") != 0)
            sigdump_output_fprint(ccu_ctx, "%s_mvdxy = 0x%x,0x%x\n", prefix.c_str(), p_me_golden->mvd_y, p_me_golden->mvd_x);
        sigdump_output_fprint(ccu_ctx, "%s_refidx = 0x%x\n", prefix.c_str(), p_me_golden->refIdx);
        sigdump_output_fprint(ccu_ctx, "%s_mvp_list = 0x%x\n", prefix.c_str(), p_me_golden->amvp_list_idx);
        sigdump_output_fprint(ccu_ctx, "%s_long_term = 0x%x\n", prefix.c_str(), p_me_golden->isLongTerm);
        sigdump_output_fprint(ccu_ctx, "%s_abs_poc = 0x%x\n", prefix.c_str(), p_me_golden->poc_diff);
        sigdump_output_fprint(ccu_ctx, "%s_sse = 0x%llx,0x%llx\n", prefix.c_str(), p_me_golden->SSE[0], p_me_golden->SSE[1]);
        sigdump_output_fprint(ccu_ctx, "%s_satd = 0x%llx,0x%llx\n", prefix.c_str(), p_me_golden->SATD[0], p_me_golden->SATD[1]);
        sigdump_output_fprint(ccu_ctx, "%s_bin_count = 0x%x\n", prefix.c_str(), p_me_golden->LC_BIN_CNT);
        if (type.compare("MERGE") == 0)
            sigdump_output_fprint(ccu_ctx, "%s_mvl = 0x%x\n", prefix.c_str(), p_me_golden->mv_level);

        if (type.compare("MERGE") == 0)
            p_me_golden->BEST_BIN_CNT[0] = p_me_golden->BIN_CNT;
        else if (type.compare("FME") == 0)
            p_me_golden->BEST_BIN_CNT[1] = p_me_golden->BIN_CNT;
        if (type.compare("FME") == 0) {
            if (g_sigpool.cu_width == 16) {
                g_sigpool.blk16_mv.mv_x = p_me_golden->mv_x;
                g_sigpool.blk16_mv.mv_y = p_me_golden->mv_y;
                g_sigpool.blk16_mv.refIdx = p_me_golden->refIdx;
            } else if (g_sigpool.cu_width == 8) {
                int idx = ((g_sigpool.cu_idx_x & 8) >> 3) + ((g_sigpool.cu_idx_y & 8) >> 2);
                g_sigpool.blk8_mv[idx].mv_x = p_me_golden->mv_x;
                g_sigpool.blk8_mv[idx].mv_y = p_me_golden->mv_y;
                g_sigpool.blk8_mv[idx].refIdx = p_me_golden->refIdx;
            }
        }
    }
}

void sig_ccu_madi(sig_ctx *ccu_ctx, int cu_x, int cu_y)
{
    int madi_16[4] = { 0 };
    int lum_16[4] = { 0 };
    get_madi_blk32(cu_x, cu_y, madi_16, lum_16);
    sigdump_output_fprint(ccu_ctx, "# MADI\ncur_blk32_madi = 0x%x, 0x%x, 0x%x, 0x%x\n",
        madi_16[0], madi_16[1], madi_16[2], madi_16[3]);
    sigdump_output_fprint(ccu_ctx, "cur_blk32_luma_avg = 0x%x, 0x%x, 0x%x, 0x%x\n",
        lum_16[0], lum_16[1], lum_16[2], lum_16[3]);
}

void sig_ccu_record_rc_blk16(int x, int y, int tc_qp_delta, int lum_qp_delta, int blk_qp)
{
    sig_ccu_rc_st *p_ccu_rc_st = g_sigpool.p_ccu_rc_st;
    int idx = p_ccu_rc_st->blk16_idx;
    if (idx >= 16)
      printf("[CCU][ERROR] p_ccu_rc_st->blk16_idx = %d\n", idx);

    p_ccu_rc_st->blk_x[idx]     = x;
    p_ccu_rc_st->blk_y[idx]     = y;
    p_ccu_rc_st->tc_q_ofs[idx]  = tc_qp_delta;
    p_ccu_rc_st->lum_q_ofs[idx] = lum_qp_delta;
    p_ccu_rc_st->qpmap[idx]     = (g_blk16_qp_map.get_data(x, y) & 0xff);
    p_ccu_rc_st->blk16_qp[idx]  = blk_qp;
    p_ccu_rc_st->blk16_idx++;
}
#endif //~SIG_CCU

#ifdef SIG_CCU
void sig_ccu_record_cu32_bits(int cu_x, int cu_y, int cu_w, int cu_h, int total_bits)
{
    int cu_end_x = cu_x + cu_w;
    int cu_end_y = cu_y + cu_h;

    if ( ((cu_end_x & 0x1f) == 0 || (cu_end_x == g_sigpool.width)) &&
         ((cu_end_y & 0x1f) == 0 || (cu_end_y == g_sigpool.height)))
    {
        int cu32_idx = 0;
        if ((cu_x & 0x3f) >= 32 )
          cu32_idx++;
        if ((cu_y & 0x3f) >= 32 )
          cu32_idx += 2;

        UInt start_bits = g_sigpool.p_ccu_rc_st->cu32_start_bits;
        g_sigpool.p_ccu_rc_st->cu32_coded_bits[cu32_idx] = total_bits - start_bits;
    }
}
#endif //~SIG_CCU

#ifdef SIG_CCU
void sig_ccu_record_constqp(TComDataCU *p_cu)
{
    sig_ccu_rc_st *p_ccu_rc_st = g_sigpool.p_ccu_rc_st;

    int const_qp = p_cu->getSlice()->getSliceQp();
    p_ccu_rc_st->ctu_qp = const_qp;
    p_ccu_rc_st->row_qp = const_qp;

    int ctu_posX = p_cu->getCUPelX();
    int ctu_posY = p_cu->getCUPelY();
    int width  = g_sigpool.width;
    int height = g_sigpool.height;

    // fill blk16
    Int idxToX[4] = {0, 1, 0, 1}, idxToY[4] = {0, 0, 1, 1};
    for(Int cu32_i=0; cu32_i<4; cu32_i++)
    {
        Int cu32_x = idxToX[cu32_i]<<5;
        Int cu32_y = idxToY[cu32_i]<<5;
        if((ctu_posX+cu32_x)>=width ||
           (ctu_posY+cu32_y)>=height)
        {
            continue;
        }

        p_ccu_rc_st->cu32_qp[(p_ccu_rc_st->blk16_idx >> 2)] = const_qp;
        for(Int blk_i=0; blk_i<4; blk_i++)
        {
            Int blk_x = ctu_posX + cu32_x + (idxToX[blk_i]<<4);
            Int blk_y = ctu_posY + cu32_y + (idxToY[blk_i]<<4);
            sig_ccu_record_rc_blk16(blk_x, blk_y, 0, 0, const_qp);
        }
    }
}
#endif //~SIG_CCU

#ifdef SIG_CCU
void sig_ccu_output_rc()
{
    sig_ctx *p_ctx = &g_sigpool.ccu_rc_ctx;
    sig_ccu_rc_st *p_ccu_rc_st = g_sigpool.p_ccu_rc_st;

    if (p_ccu_rc_st == nullptr)
        return;

    sigdump_output_fprint(p_ctx, "# CTU\n");
    sigdump_output_fprint(p_ctx, "ctu_coded_bits = 0x%x\n", p_ccu_rc_st->ctu_coded_bits);
    sigdump_output_fprint(p_ctx, "cu32_coded_bits_0 = 0x%x\n", p_ccu_rc_st->cu32_coded_bits[0]);
    sigdump_output_fprint(p_ctx, "cu32_coded_bits_1 = 0x%x\n", p_ccu_rc_st->cu32_coded_bits[1]);
    sigdump_output_fprint(p_ctx, "cu32_coded_bits_2 = 0x%x\n", p_ccu_rc_st->cu32_coded_bits[2]);
    sigdump_output_fprint(p_ctx, "cu32_coded_bits_3 = 0x%x\n", p_ccu_rc_st->cu32_coded_bits[3]);
    sigdump_output_fprint(p_ctx, "ctu_qp = 0x%x\n", p_ccu_rc_st->ctu_qp);
    sigdump_output_fprint(p_ctx, "is_row_ovf = 0x%x\n", (p_ccu_rc_st->is_row_ovf == true) ? 1 : 0);
    sigdump_output_fprint(p_ctx, "row_qp = 0x%x\n", p_ccu_rc_st->row_qp);
    sigdump_output_fprint(p_ctx, "ctu_row_target_bits = 0x%x\n", p_ccu_rc_st->ctu_row_target_bits);
    sigdump_output_fprint(p_ctx, "ctu_row_acc_bits = 0x%x\n", p_ccu_rc_st->ctu_row_acc_bits);

    for (int i = 0; i < p_ccu_rc_st->blk16_idx; i++)
    {
        int blk_x = p_ccu_rc_st->blk_x[i];
        int blk_y = p_ccu_rc_st->blk_y[i];
        sigdump_output_fprint(p_ctx, "# BLK16\n");
        sigdump_output_fprint(p_ctx, "(x, y) = (%d, %d)\n", blk_x, blk_y);
        sigdump_output_fprint(p_ctx, "tc_q_ofs = 0x%x\n", p_ccu_rc_st->tc_q_ofs[i]);
        sigdump_output_fprint(p_ctx, "lum_q_ofs = 0x%x\n", p_ccu_rc_st->lum_q_ofs[i]);
        sigdump_output_fprint(p_ctx, "qpmap = 0x%x\n", p_ccu_rc_st->qpmap[i]);
        sigdump_output_fprint(p_ctx, "blk16_qp = 0x%x\n", p_ccu_rc_st->blk16_qp[i]);

        // Get QP index
        int cu32_idx = (i >> 2);
        if ((i & 0x3) == 0)
            sigdump_output_fprint(p_ctx, "blk32_qp = 0x%x\n", p_ccu_rc_st->cu32_qp[cu32_idx]);

        // Get cbf_ctx index, it is different to QP index.
        cu32_idx  = (blk_x & 0x20) > 0 ? 1 : 0;
        cu32_idx += (blk_y & 0x20) > 0 ? 2 : 0;
        sigdump_output_fprint(p_ctx, "luma_cbf_ctx = 0x%x, 0x%x\n",
                              p_ccu_rc_st->cu32_cbf_ctx[cu32_idx][0][0],
                              p_ccu_rc_st->cu32_cbf_ctx[cu32_idx][0][1]);

        sigdump_output_fprint(p_ctx, "chroma_cbf_ctx = 0x%x, 0x%x\n",
                              p_ccu_rc_st->cu32_cbf_ctx[cu32_idx][1][0],
                              p_ccu_rc_st->cu32_cbf_ctx[cu32_idx][1][1]);

        if ((i & 0x3) == 0)
            sig_ccu_madi(p_ctx, blk_x, blk_y);
    }
}
#endif //~SIG_CCU

#ifdef SIG_CCU
void sig_ccu_output_init_tab()
{
    sig_ctx *p_ctx = &g_sigpool.ccu_init_tab_ctx;

    // Lambda
    for (int qp = 0; qp < 52; qp++)
    {
        int lambda = (int)(g_qp_to_lambda_table[qp] * (1 << LAMBDA_FRAC_BIT));
        sigdump_output_bin(p_ctx, (unsigned char *)(&lambda), sizeof(int));
    }
    // sqrt Lambda
    for (int qp = 0; qp < 52; qp++)
    {
        int lambda = (int)(g_qp_to_sqrt_lambda_table[qp] * (1 << LC_LAMBDA_FRAC_BIT));
        sigdump_output_bin(p_ctx, (unsigned char *)(&lambda), sizeof(int));
    }
    // ROWQ_IN table
    int entry_number = g_row_q_lut.get_entry_num();
    for (int i = 0; i < entry_number; i++)
    {
        int dump = g_row_q_lut.get_input_entry(i);
        sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(int));
    }
    // ROWQ_OUT table
    for (int i = 0; i < entry_number; i++)
    {
        unsigned char dump = (g_row_q_lut.get_output_entry(i) & 0xff);
        sigdump_output_bin(p_ctx, &dump, sizeof(unsigned char));
    }
    // TC_IN Table
    entry_number = g_tc_lut.get_entry_num();
    for (int i = 0; i < entry_number; i++)
    {
        unsigned short dump = (g_tc_lut.get_input_entry(i) & 0xffff);
        sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(unsigned short));
    }
    // TC_OUT Table
    entry_number++; // interval mode
    for (int i = 0; i < entry_number; i++)
    {
        unsigned short dump = (g_tc_lut.get_output_entry(i) & 0xffff);
        sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(unsigned short));
    }
    // LUM_IN table
    entry_number = g_lum_lut.get_entry_num();
    for (int i = 0; i < entry_number; i++)
    {
        unsigned short dump = (g_lum_lut.get_input_entry(i) & 0xffff);
        sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(unsigned short));
    }
    // LUM_OUT table
    for (int i = 0; i < entry_number; i++)
    {
        unsigned short dump = (g_lum_lut.get_output_entry(i) & 0xffff);
        sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(unsigned short));
    }
    // Self-Info LPS=0
    // Self-Info LPS=1
    int self_info_number = 16;
    for (int lps = 0; lps < 2; lps++)
    {
        for (int i = 0; i < self_info_number; i++)
        {
            unsigned short dump = (g_hwEntropyBits[i][lps] & 0xffff);
            sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(unsigned short));
        }
    }
    // M_SIG0
    // M_SIG1
    for (int bin = 0; bin < 2; bin++)
    {
        for (int ch = 0; ch < 2; ch++)
        {
            for (int pred = 0; pred < 2; pred++)
            {
                for (int size = 0; size < 4; size++)
                {
                    int dump = g_significantScale[0][ch][pred][size][bin];
                    sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(int));
                }
            }
        }
    }
    // C_SIG0
    // C_SIG1
    for (int bin = 0; bin < 2; bin++)
    {
        for (int ch = 0; ch < 2; ch++)
        {
            for (int pred = 0; pred < 2; pred++)
            {
                for (int size = 0; size < 4; size++)
                {
                    int dump = g_significantBias[0][ch][pred][size][bin];
                    sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(int));
                }
            }
        }
    }
    // Ratio
    for (int ch = 0; ch < 2; ch++)
    {
        for (int pred = 0; pred < 2; pred++)
        {
            for (int size = 0; size < 4; size++)
            {
                unsigned short dump = (unsigned short)(rru_get_last_pos_scale(g_lastPosBinScale_init[ch][pred][size]));
                sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(unsigned short));
            }
        }
    }
    // FG/BG Weight
    for (int fg = 0; fg < 2; fg++)
    {
        // Weight
        for (int mode = 0; mode < 3; mode++)
        {
            UInt dump = ((gp_hw_cost[fg][2][mode] & 0x3f) << 8) +
                        ((gp_hw_cost[fg][1][mode] & 0x3f) << 16) +
                        ((gp_hw_cost[fg][0][mode] & 0x3f) << 24);
            sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(unsigned int));
        }

        // Bias
        for (int mode = 0; mode < 3; mode++)
        {
            // CU8
            UInt dump = ((gp_hw_bias[fg][2][mode] & 0xffff) << 16);
            sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(unsigned int));
            // CU16, CU32
            dump = (gp_hw_bias[fg][1][mode] & 0xffff) +
                   ((gp_hw_bias[fg][0][mode] & 0xffff) << 16);
            sigdump_output_bin(p_ctx, (unsigned char *)(&dump), sizeof(unsigned int));
        }
    }

    // QP Map ROI
    for (int i = 0; i < QP_ROI_LIST_SIZE; i++)
    {
        int start_x = 0;
        int start_y = 0;
        int end_x = 0;
        int end_y = 0;
        int mode = 0;
        int qp = 0;
        int value = 0;

        if (g_qp_roi_cfg[i].is_enable == true)
        {
            int x = g_qp_roi_cfg[i].x;
            int y = g_qp_roi_cfg[i].y;
            int w = g_qp_roi_cfg[i].width;
            int h = g_qp_roi_cfg[i].height;

            start_x = (x >> 4) << 4;
            start_y = (y >> 4) << 4;
            end_x = ((x + w) >> 4) << 4;
            end_y = ((y + h) >> 4) << 4;
            mode = g_qp_roi_cfg[i].mode;
            qp = g_qp_roi_cfg[i].qp;
        }

        value = ((start_y & 0xffff) << 16) | (start_x & 0xffff);
        sigdump_output_bin(p_ctx, (unsigned char *)(&value), 4);

        value = ((qp & 0xff) << 8) | (mode & 0x1);
        sigdump_output_bin(p_ctx, (unsigned char *)(&value), 4);

        value = ((end_y & 0xffff) << 16) | (end_x & 0xffff);
        sigdump_output_bin(p_ctx, (unsigned char *)(&value), 4);
    }

#ifdef CVI_SMART_ENC
    // AI dqp table
    unsigned char dqp_table[AI_SI_TAB_COUNT][AI_SI_TAB_BIN];
    memset(&(dqp_table[0][0]), 0, sizeof(dqp_table));

    for (int i = 0; i < AI_SI_TAB_COUNT; i++)
    {
        for (int j = 0; j < AI_SI_TAB_BIN; j++)
            dqp_table[i][j] = (g_smart_enc_ai_table[i][j] & 0xff);
    }
    sigdump_output_bin(p_ctx, &(dqp_table[0][0]), sizeof(dqp_table));

    // AI object mapping table
    unsigned char obj_table[64];
    memset(obj_table, 0, sizeof(obj_table));
    int map_table_size = gp_ai_enc_param ? gp_ai_enc_param->mapping_table.size() : 0;

    for (int i = 0; i < map_table_size; i++)
    {
        char idx = gp_ai_enc_param->mapping_table[i].table_idx;
        if (idx >= 0)
            obj_table[i] = ((idx & 0x7) << 1) | 1;
    }
    sigdump_output_bin(p_ctx, obj_table, sizeof(obj_table));
#endif //~CVI_SMART_ENC
}
#endif //~SIG_CCU

#ifdef SIG_CCU
int SIGDUMP_CTU_CU16_POS[16][2] =
{
    { 0,  0}, {16,  0}, { 0, 16}, {16, 16},
    {32,  0}, {48,  0}, {32, 16}, {48, 16},
    { 0, 32}, {16, 32}, { 0, 48}, {16, 48},
    {32, 32}, {48, 32}, {32, 48}, {48, 48}
};

void sig_ccu_output_qpmap()
{
    sig_ctx *p_ctx = &g_sigpool.ccu_qpmap_ctx;
    int width = g_sigpool.width;
    int width_align128 = cvi_mem_align(width, 128);
    int height = g_sigpool.height;
    unsigned char map[16] = { 0 };

    for (int ctu_y = 0; ctu_y < height; ctu_y += 64)
    {
        for (int ctu_x = 0; ctu_x < width_align128; ctu_x += 64)
        {
            // pitch is 32 bytes, output temp 16 bytes.
            if (ctu_x >= width)
            {
                memset(map, 0, 16);
                sigdump_output_bin(p_ctx, map, 16);
                continue;
            }

            for (int i = 0; i < 16; i++)
            {
                int blk16_x = ctu_x + SIGDUMP_CTU_CU16_POS[i][0];
                int blk16_y = ctu_y + SIGDUMP_CTU_CU16_POS[i][1];
                map[i] = (g_blk16_qp_map.get_data(blk16_x, blk16_y) & 0xff);
            }
            sigdump_output_bin(p_ctx, map, 16);
        }
    }
}

void sig_ccu_copy_cbf_ctx()
{
    int idx = (g_sigpool.cu_idx_x & 0x20) > 0 ? 1 : 0;
    idx +=  (g_sigpool.cu_idx_y & 0x20) > 0 ? 2 : 0;
    for (int ch = 0; ch < 2; ch++)
    {
        for (int b = 0; b < 2; b++)
            g_sigpool.p_ccu_rc_st->cu32_cbf_ctx[idx][ch][b] = g_cbfState[g_scale_D][ch][b];
    }
}

void sigdump_ccu_ime_mvp(int cu_idx_x, int cu_idx_y)
{
    if (is_last_8x8_in_16x16(cu_idx_x, cu_idx_y)) {
        sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "# FME\nblk16_mvx = 0x%x\nblk16_mvy = 0x%x\nblk16_refidx = 0x%x\n",
            g_sigpool.blk16_mv.mv_x, g_sigpool.blk16_mv.mv_y, g_sigpool.blk16_mv.refIdx);
        sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "blk8_mvx = 0x%x, 0x%x, 0x%x, 0x%x\n",
            g_sigpool.blk8_mv[0].mv_x, g_sigpool.blk8_mv[1].mv_x, g_sigpool.blk8_mv[2].mv_x, g_sigpool.blk8_mv[3].mv_x);
        sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "blk8_mvy = 0x%x, 0x%x, 0x%x, 0x%x\n",
            g_sigpool.blk8_mv[0].mv_y, g_sigpool.blk8_mv[1].mv_y, g_sigpool.blk8_mv[2].mv_y, g_sigpool.blk8_mv[3].mv_y);
        sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "blk8_refidx = 0x%x, 0x%x, 0x%x, 0x%x\n",
            g_sigpool.blk8_mv[0].refIdx, g_sigpool.blk8_mv[1].refIdx, g_sigpool.blk8_mv[2].refIdx, g_sigpool.blk8_mv[3].refIdx);
    }
}

void sigdump_ccu_rd_cost(TComDataCU* bestCU, Int cu_width, UInt uiDepth,
                         Double totalRDcost_inter, Double totalRDcost_inter_wt)
{
    sig_ctx *ccu_ctx = &g_sigpool.ccu_ctx[sig_pat_blk_size_4(cu_width)];

    int is_foreground = 0;
    if (g_algo_cfg.CostPenaltyCfg.EnableForeground && bestCU->getSlice()->getSliceType() != I_SLICE)
    {
        int cu_x = bestCU->getCUPelX();
        int cu_y = bestCU->getCUPelY();
        int fg_idx = (cu_width == 32) ? 1 : ((cu_width == 16) ? 0 : 2);
        is_foreground = (g_foreground_map[fg_idx].get_data(cu_x, cu_y) > 0) ? 1 : 0;
    }

    if (cu_width != 4) {
        unsigned int inter_bin_cnt = bestCU->isSkipped(0) ? 2 : g_sigpool.me_golden.BEST_BIN_CNT[bestCU->getMergeFlag(0) ? 0 : 1];
        if( !(bestCU->getMergeFlag( 0 ) && bestCU->getPartitionSize(0) == SIZE_2Nx2N ) ) {
            inter_bin_cnt += 1; // QtRootCbf
        }
        UInt64 intra_cost    = (UInt64)(g_sigpool.rru_intra_cost[sig_pat_blk_size_4(cu_width)] * (1 << RD_COST_FRAC_BIT));
        UInt64 intra_cost_wt = (UInt64)(g_sigpool.rru_intra_cost_wt[sig_pat_blk_size_4(cu_width)] * (1 << RD_COST_FRAC_BIT));
        UInt intra_bin_count = g_sigpool.intra_bin_count[1];

        if (cu_width == 8 && !g_algo_cfg.DisablePfrmIntra4)
        {
            if (g_sigpool.rru_intra_cost[0] < g_sigpool.rru_intra_cost[1])
            {
                intra_cost    = (UInt64)(g_sigpool.rru_intra_cost[0] * (1 << RD_COST_FRAC_BIT));
                intra_cost_wt = (UInt64)(g_sigpool.rru_intra_cost_wt[0] * (1 << RD_COST_FRAC_BIT));
                intra_bin_count = g_sigpool.intra_bin_count[0];
            }
        }
        sigdump_output_fprint(ccu_ctx, "inter_rd_cost = 0x%llx,0x%llx,0x%x\n",
                              (UInt64)(totalRDcost_inter * 4), (UInt64)(totalRDcost_inter_wt * 4), is_foreground);
        sigdump_output_fprint(ccu_ctx, "inter_bin_count = 0x%x\n", inter_bin_cnt);

        sigdump_output_fprint(ccu_ctx, "intra_rd_cost = 0x%llx,0x%llx,0x%x\n",
                              intra_cost, intra_cost_wt, is_foreground);
        sigdump_output_fprint(ccu_ctx, "intra_bin_count = 0x%x\n", intra_bin_count);

        UInt64 best_rd_cost = (UInt64)(bestCU->getTotalCost() * 4);
        sigdump_output_fprint(ccu_ctx, "best_rd_cost = 0x%llx,0x%llx,0x%x\n",
                              best_rd_cost, best_rd_cost, is_foreground);

        unsigned int split_flag = (uiDepth == bestCU->getSlice()->getSPS()->getLog2DiffMaxMinCodingBlockSize()) ? 0 : 1;
        unsigned int best_bin_count = bestCU->isIntra(0) ?
                                      ((bestCU->getPartitionSize(0) == SIZE_NxN) ? g_sigpool.intra_bin_count[0] : g_sigpool.intra_bin_count[1] + split_flag) :
                                      inter_bin_cnt + split_flag;
        sigdump_output_fprint(ccu_ctx, "best_bin_count = 0x%x\n", best_bin_count);

        sigdump_output_fprint(&g_sigpool.ccu_cost_ctx, "cu%d best_rd_cost = 0x%x\n", cu_width, (unsigned int)(bestCU->getTotalCost() * 4));
    } else {
        sigdump_output_fprint(ccu_ctx, "inter_rd_cost = 0x0,0x0,0x%x\n", is_foreground);
        sigdump_output_fprint(ccu_ctx, "inter_bin_count = 0x%x\n", 0);
        sigdump_output_fprint(ccu_ctx, "intra_rd_cost = 0x%x,0x%x,0x%x\n",
                              (unsigned int)(g_sigpool.rru_intra_cost[0] * 4),
                              (unsigned int)(g_sigpool.rru_intra_cost_wt[0] * 4), is_foreground);
        sigdump_output_fprint(ccu_ctx, "intra_bin_count = 0x%x\n", g_sigpool.intra_bin_count[0]);
        sigdump_output_fprint(ccu_ctx, "best_rd_cost = 0x%x,0x%x,0x%x\n",
                              (unsigned int)(g_sigpool.rru_intra_cost[0] * 4),
                              (unsigned int)(g_sigpool.rru_intra_cost_wt[0] * 4), is_foreground);
        sigdump_output_fprint(ccu_ctx, "best_bin_count = 0x%x\n", g_sigpool.intra_bin_count[0]);

        sigdump_output_fprint(&g_sigpool.ccu_cost_ctx, "cu%d best_rd_cost = 0x%x\n", cu_width, (unsigned int)(g_sigpool.rru_intra_cost[0] * 4));
    }
}

void sig_ccu_output_i4term_temp_data()
{
    sig_ctx *p_ctx = &g_sigpool.ccu_ctx[0];
    sigdump_output_fprint(p_ctx, "# CU (x, y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    sigdump_output_fprint(p_ctx, "# QP\ncur_qp = 0x0,0x0,0x0\n");
    sigdump_output_fprint(p_ctx, "cur_cu_lambda = 0x0\n");
    sigdump_output_fprint(p_ctx, "cur_cu_sqrt_lambda = 0x0\n");

    // sigdump_ccu_cu_golden_start()
    sigdump_output_fprint(p_ctx, "# INTRA\n");
    sigdump_output_fprint(p_ctx, "luma_pred_mode = 0x0, 0x0, 0x0, 0x0\n");
    sigdump_output_fprint(p_ctx, "chroma_pred_mode = 0x0\n");
    sigdump_output_fprint(p_ctx, "intra_chroma_pred_mode = 0x0\n");
    sigdump_output_fprint(p_ctx, "rem_luma_pred_mode = 0x0, 0x0, 0x0, 0x0\n");
    sigdump_output_fprint(p_ctx, "mpm_idx = 0x0, 0x0, 0x0, 0x0\n");
    sigdump_output_fprint(p_ctx, "prev_luma_pred_flag = 0x0, 0x0, 0x0, 0x0\n");

    // sig_ccu_output_cu_golden_4x4()
    const string comp_s[] = {"luma", "cb", "cr"};
    const string pred_s[] = {"inter", "intra"};
    int tu_count = 4;

    for (int tu_idx = 0; tu_idx < tu_count; tu_idx++)
    {
        sigdump_output_fprint(p_ctx, "# Estimation\n");
        for (int pred_mode = 0; pred_mode < 2; pred_mode++)
        {
            sigdump_output_fprint(p_ctx, "%s_sse_val = 0x0\n", pred_s[pred_mode].c_str());
            sigdump_output_fprint(p_ctx, "%s_bitest_val = 0x\n", pred_s[pred_mode].c_str());
            sigdump_output_fprint(p_ctx, "%s_si_val = ", pred_s[pred_mode].c_str());
            for (int depth = 0; depth < 2; depth++)
            {
                for (int comp = 0; comp < 3; comp++)
                    sigdump_output_fprint(p_ctx, "0x0,");
            }
            sigdump_output_fprint(p_ctx, "\n");
            for (int comp = 0; comp < 3; comp++)
            {
                sigdump_output_fprint(p_ctx, "%s_%s_csbf = 0x0\n", pred_s[pred_mode].c_str(), comp_s[comp].c_str());
                sigdump_output_fprint(p_ctx, "%s_%s_last_xy = 0x0, 0x0\n", pred_s[pred_mode].c_str(), comp_s[comp].c_str());
            }
        }
    }

    // sigdump_ccu_md_4x4()
    sigdump_output_fprint(p_ctx, "# Mode Decision\n");
    sigdump_output_fprint(p_ctx, "is_intra_win = 1\n");
    sigdump_output_fprint(p_ctx, "is_skip_win = 0\n");
    sigdump_output_fprint(p_ctx, "is_mrg_win = 0\n");
    sigdump_output_fprint(p_ctx, "mrg_cand_idx = 0\n");
    sigdump_output_fprint(p_ctx, "scan_idx = 0, 0, 0, 0, 0\n");

    // sig_rru_output_cu_self_info()
    sigdump_output_fprint(p_ctx, "# Self-information\n");
    const string color_s[] = {"luma", "chroma"};

    for (int predID = 0; predID < 2; predID++) {
        for (int compID = 0; compID < 2; compID++) {
            sigdump_output_fprint(p_ctx, "reg_%s_tu_bitest_%s_sfi = 0\n", pred_s[predID].c_str(), color_s[compID].c_str());
        }
        for (int compID = 0; compID < 2; compID++) {
            sigdump_output_fprint(p_ctx, "reg_%s_tu_bitest_%s_cbf_ctx = 0,0\n", pred_s[predID].c_str(), color_s[compID].c_str());
        }
    }

    sigdump_output_fprint(p_ctx, "# Ratio for lastPosXY\n");
    for (int predID = 0; predID < 2; predID++) {
        for (int compID = 0; compID < 2; compID++) {
            sigdump_output_fprint(p_ctx, "reg_%s_bitest_%s_ratio = 0\n", pred_s[predID].c_str(), color_s[compID].c_str());
        }
    }
    sigdump_output_fprint(p_ctx, "# linear Model for SIG and GT1\n");
    for (int predID = 0; predID < 2; predID++) {
        for (int compID = 0; compID < 2; compID++) {
            sigdump_output_fprint(p_ctx, "reg_%s_tu_%s_m = 0, 0\n", pred_s[predID].c_str(), color_s[compID].c_str());
            sigdump_output_fprint(p_ctx, "reg_%s_tu_%s_c = 0, 0\n", pred_s[predID].c_str(), color_s[compID].c_str());
        }
    }

    // sigdump_ccu_rd_cost()
    UInt64 max_cost = ccu_get_max_rd_cost();

    sigdump_output_fprint(p_ctx, "inter_rd_cost = 0x0,0x0,0x0\n");
    sigdump_output_fprint(p_ctx, "inter_bin_count = 0x0\n");
    sigdump_output_fprint(p_ctx, "intra_rd_cost = 0x%llx,0x%llx,0x0\n", max_cost, max_cost);
    sigdump_output_fprint(p_ctx, "intra_bin_count = 0x0\n");
    sigdump_output_fprint(p_ctx, "best_rd_cost = 0x%llx,0x%llx,0x0\n", max_cost, max_cost);
    sigdump_output_fprint(p_ctx, "best_bin_count = 0x0\n");
    sigdump_output_fprint(p_ctx, "is_comp_split_intra_win = %d\n", 0);
    sigdump_output_fprint(p_ctx, "is_win = %d\n", 0);

    sigdump_output_fprint(&g_sigpool.ccu_cost_ctx, "cu%d best_rd_cost = 0x%x\n", 4, max_cost);
}
#endif //~SIG_CCU

#ifdef SIG_IME_MVP
void sigdump_nei_mvb(TComDataCU* pCtu)
{
  int idx_x = g_sigpool.ctb_idx_x;
  int idx_y = g_sigpool.ctb_idx_y + g_sigpool.ctb_size - 16;

  unsigned short refIdx_buf;
  short mvx[8] = {0};
  short mvy[8] = {0};
  unsigned char dummy[14] = {0};
  for (int i = 0; i < 4; i ++) {
    int refIdx = -1;
    pCtu->get_nei_real_mv(REF_PIC_LIST_0, idx_x + 16 * i, idx_y, mvx[2 * i], mvy[2 * i], refIdx);
    if (refIdx == -1)
      refIdx = 3;
    assert(refIdx <= 3);
    refIdx_buf |= (refIdx << (4 * i));
    refIdx_buf |= (refIdx << (4 * i + 2));

    mvx[2 * i + 1] = mvx[2 * i];
    mvy[2 * i + 1] = mvy[2 * i];
  }

  sigdump_output_bin(&g_sigpool.ime_mvb_ctx, dummy, sizeof(dummy));
  sigdump_output_bin(&g_sigpool.ime_mvb_ctx, (unsigned char *)&refIdx_buf, sizeof(refIdx_buf));
  sigdump_output_bin(&g_sigpool.ime_mvb_ctx, (unsigned char *)mvx, sizeof(mvx));
  sigdump_output_bin(&g_sigpool.ime_mvb_ctx, (unsigned char *)mvy, sizeof(mvy));
}
#endif

#ifdef SIG_IRPU
void sigdump_col_mv(TComDataCU* pCtu)
{
  int col_cnt = g_sigpool.ctb_size / 16;
//  int cnt = 0;
//  int offset = 16 / 4;
  unsigned char isIntra_long_buf[16] = {0};
  char POCdiff_buf[16] = {0};
  short mvx[16] = {0};
  short mvy[16] = {0};

  // 16x16 Z-SCAN in CTB
  for (int col_idx = 0; col_idx < (col_cnt * col_cnt); col_idx++)
  {
    TComMv cColMv, cColScaledMv;
    Int isIntra = 0;
    Int islong = 0;
    Int POCdiff = 0;
    Int ColRefIdx = -1;
    //int pel_x = g_auiRasterToPelX[g_auiZscanToRaster[col_idx * 16]];
    //int pel_y = g_auiRasterToPelY[g_auiZscanToRaster[col_idx * 16]];

    pCtu->xGetColMVP_cvi(REF_PIC_LIST_0, pCtu->getCtuRsAddr(), col_idx * 16, cColMv, cColScaledMv, isIntra, islong, POCdiff, ColRefIdx);

    if (g_sigdump.irpu)
      sigdump_output_fprint(&g_sigpool.irpu_col_frm_ctx, "c%d %x %x %x %x %x\n", col_idx, isIntra, islong, POCdiff, cColMv.getVer() & 0xffff, cColMv.getHor() & 0xffff);
    if (g_sigdump.col) {
      if (isIntra)
        isIntra_long_buf[col_idx] |= 1;
      if (islong)
        isIntra_long_buf[col_idx] |= 2;
      POCdiff_buf[col_idx] = POCdiff;
      mvx[col_idx] = cColMv.getHor() & 0xffff;
      mvy[col_idx] = cColMv.getVer() & 0xffff;
    }
  }


  if (g_sigdump.col) {
    //col buffer is align two ctb.
    int ctb_col_cnt = 1;
    int last_ctb_idx = (g_sigpool.widthAlignCTB - 1) * g_sigpool.ctb_size;
    if ((g_sigpool.widthAlignCTB & 0x1) && (g_sigpool.ctb_idx_x  == last_ctb_idx))
      ctb_col_cnt++;
    for (int i = 0; i < ctb_col_cnt; i++) {
      sigdump_output_bin(&g_sigpool.col_ctx, (unsigned char *)POCdiff_buf, sizeof(POCdiff_buf));
      sigdump_output_bin(&g_sigpool.col_ctx, (unsigned char *)isIntra_long_buf, sizeof(isIntra_long_buf));
      for (int mv_cnt = 0; mv_cnt < 16; mv_cnt++) {
        sigdump_output_bin(&g_sigpool.col_ctx, (unsigned char *)(mvx + mv_cnt), 2);
        sigdump_output_bin(&g_sigpool.col_ctx, (unsigned char *)(mvy + mv_cnt), 2);
      }
    }
  }
}

void sigdump_irpu_cu_cmd(TComDataCU*& rpcTempCU, TComMvField* pcMvFieldNeighbours)
{
  int size_idx = (rpcTempCU->getWidth( 0 ) == 32) ? 2 : (rpcTempCU->getWidth( 0 ) == 16) ? 1 : 0;
  sig_ctx *irpu_cmd_frm_ctx = &g_sigpool.irpu_cmd_frm_ctx[size_idx];

  for (int i = 0; i < 2; i++) {
    g_sigpool.amvp_list[0][i].mvx = pcMvFieldNeighbours[0 + 2*i].getMv().getHor();
    g_sigpool.amvp_list[0][i].mvy = pcMvFieldNeighbours[0 + 2*i].getMv().getVer();
    if (strcmp(g_sigpool.amvp_list[0][i].dir, "T0_SCALING") == 0
    || strcmp(g_sigpool.amvp_list[0][i].dir, "T0") == 0
    || strcmp(g_sigpool.amvp_list[0][i].dir, "T1_SCALING") == 0
    || strcmp(g_sigpool.amvp_list[0][i].dir, "T1") == 0) {
        g_sigpool.amvp_list[0][i].ref_idx = rpcTempCU->getSlice()->getColRefIdx();
      } else {
      g_sigpool.amvp_list[0][i].ref_idx = pcMvFieldNeighbours[0 + 2 * i].getRefIdx();
    }
  }

  sigdump_output_fprint(irpu_cmd_frm_ctx, "# [CU info] (CU_X, CU_Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);

  //get nebA
  rpcTempCU->getCuNeighbor_cvi( 0, 0);
}

void sigdump_me_cu_golden_backup(TComDataCU*& rpcTempCU, int list_idx)
{
  int size_idx = sig_pat_blk_size_4(rpcTempCU->getWidth( 0 ));
  g_sigpool.me_golden.size_idx = size_idx;

  const TComCUMvField* pcCUMvField = rpcTempCU->getCUMvField( REF_PIC_LIST_0 );

  g_sigpool.me_golden.isLongTerm = rpcTempCU->getSlice()->getRefPic(REF_PIC_LIST_0, pcCUMvField->getRefIdx(0))->getIsLongTerm() ? 1 : 0;
  g_sigpool.me_golden.SSE[CHANNEL_TYPE_LUMA] = rpcTempCU->getMcDistortion(CHANNEL_TYPE_LUMA);
  g_sigpool.me_golden.SSE[CHANNEL_TYPE_CHROMA] = rpcTempCU->getMcDistortion(CHANNEL_TYPE_CHROMA);
  g_sigpool.me_golden.SATD[CHANNEL_TYPE_LUMA] = rpcTempCU->getMcLcSADCost(CHANNEL_TYPE_LUMA);
  g_sigpool.me_golden.SATD[CHANNEL_TYPE_CHROMA] = rpcTempCU->getMcLcSADCost(CHANNEL_TYPE_CHROMA);

  //g_sigpool.me_golden.refIdx = frm_buf_mgr_find_index_by_poc(rpcTempCU->getSlice()->getRefPOC( REF_PIC_LIST_0, pcCUMvField->getRefIdx(0)));
  g_sigpool.me_golden.refIdx = pcCUMvField->getRefIdx(0);
  g_sigpool.me_golden.amvp_list_idx = list_idx;
  g_sigpool.me_golden.mv_x_ori = pcCUMvField->getMv( 0 ).getHor();
  g_sigpool.me_golden.mv_y_ori = pcCUMvField->getMv( 0 ).getVer();
  g_sigpool.me_golden.mv_x = pcCUMvField->getMv( 0 ).getHor() & 0xffff;
  g_sigpool.me_golden.mv_y = pcCUMvField->getMv( 0 ).getVer() & 0xffff;
  g_sigpool.me_golden.mvd_x = pcCUMvField->getMvd( 0 ).getHor() & 0xffff;
  g_sigpool.me_golden.mvd_y = pcCUMvField->getMvd( 0 ).getVer() & 0xffff;
  const string Cand_dir(g_sigpool.amvp_list[pcCUMvField->getRefIdx(0)][list_idx].dir);
  if (Cand_dir == "T0" || Cand_dir == "T1" || Cand_dir == "T0_SCALING" || Cand_dir == "T1_SCALING") {
    g_sigpool.me_golden.poc_diff = g_sigpool.amvp_list[pcCUMvField->getRefIdx(0)][list_idx].poc_diff;
  } else {
    g_sigpool.me_golden.poc_diff = Clip3( -128, 127,
      rpcTempCU->getPic()->getPOC() - rpcTempCU->getSlice()->getRefPOC(REF_PIC_LIST_0, pcCUMvField->getRefIdx(0)));
  }
  g_sigpool.me_golden.BIT_CNT = rpcTempCU->getNoCoeffBits();
  g_sigpool.me_golden.BIN_CNT = rpcTempCU->getNoCoeffBins();
  g_sigpool.me_golden.LC_BIT_CNT = rpcTempCU->getLcNoCoeffBits();
  g_sigpool.me_golden.LC_BIN_CNT = rpcTempCU->getLcNoCoeffBins();
}

#endif

#ifdef SIG_IAPU
void sig_iapu_ref_mux_cnt(){
    stringstream js;

    js << "{";
    for(int sz=0; sz<3; sz++){
        js << "\"" << sz << "\"";
        js <<":[";
        for(int md=0; md<=34; md++){
            js << g_sigpool.iapu_rmd_ref_smp_mux_cnt[sz][md];
            if(md+1 <= 34)
                js<<",";
        }
        js <<"]";
        if(sz+1<3)
            js<<",";
    }
    js<<"}";
    sigdump_output_fprint(&g_sigpool.iapu_rmd_ref_smp_mux_ctx,  "%s\n", js.str().c_str() );
}
#endif //~SIG_IAPU
#ifdef SIG_IAPU
void sig_iapu_output_ctu_idx(int sz){
    int iapu_file_idx = (sz==4)? 0: (sz==8)? 1: 2;
    sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cmd_frm_ctx[iapu_file_idx],    "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
    sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx],   "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
    sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx],          "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx],    "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx],    "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx],   "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
}
void sig_iapu_output_cu_idx(int sz){
    int iapu_file_idx = (sz==4)? 0: (sz==8)? 1: 2;
    sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cmd_frm_ctx[iapu_file_idx],    "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx],   "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx],          "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx],    "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx],    "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx],   "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
}
void sig_iapu_output_neb(sig_ctx *p_ctx, int sz, ComponentID comp){
    int     iapu_file_idx = (sz==4)? 0: (sz==8)? 1: 2;
    char    prefix[3][3] = {"L", "CB", "CR"};

    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx], "# %s_NEB_A\n", prefix[comp]);
    for (UInt i=1; i<g_sigpool.iapu_st.neb_h[comp]; i++)
      sigdump_output_fprint(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx], "%3d ", g_sigpool.iapu_st.neb_a[comp][i]);
    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx], "\n");

    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx], "# %s_NEB_B\n", prefix[comp]);
    for (UInt i=1; i<g_sigpool.iapu_st.neb_w[comp]; i++)
      sigdump_output_fprint(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx], "%3d ", g_sigpool.iapu_st.neb_b[comp][i]);
    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx], "\n");

    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx], "# %s_NEB_C\n", prefix[comp]);
    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx], "%3d", g_sigpool.iapu_st.neb_c[comp]);
    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx], "\n");
}
void sig_iapu_output_i4_rec(ComponentID comp, const Pel *p_rec_buf, int stride)  //copy from sig_rru_output_i4_rec
{
    if (comp == COMPONENT_Y)
    {
        unsigned char temp[64] = { 0 };
        int offset[4] = { 0, 4, (stride << 2), (stride << 2) + 4};
        int idx = 0;
        for (int blk = 0; blk < 4; blk++)
        {
            const Pel *p_rec = p_rec_buf + offset[blk];
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    temp[idx] = (p_rec[(i * stride) + j] & 0xff);
                    idx++;
                }
            }
        }
        sigdump_output_bin(&g_sigpool.iapu_iap_tu4_rec_blk_frm_ctx, temp, 64*sizeof(unsigned char));
    }
    else
    {
        unsigned char temp[16] = { 0 };
        for (int i = 0; i < 16; i++)
            temp[i] = (p_rec_buf[i] & 0xff);

        sigdump_output_bin(&g_sigpool.iapu_iap_tu4_rec_blk_frm_ctx, temp, 16*sizeof(unsigned char));
    }
}

#ifdef SIG_IAPU
void sig_iapu_output_rec(ComponentID comp, const Pel *p_rec_buf, int tu_size)
{
    sig_ctx *p_ctx = nullptr;
    int buf_size = tu_size * tu_size;
    unsigned char temp[buf_size];

    if (comp == COMPONENT_Y)
    {
        p_ctx = (tu_size == 16) ?
                (&g_sigpool.iapu_iap_tu16_rec_blk_frm_ctx) :
                (&g_sigpool.iapu_iap_tu8_rec_blk_frm_ctx);
    }
    else
    {
        p_ctx = (tu_size == 8) ?
                (&g_sigpool.iapu_iap_tu16_rec_blk_frm_ctx) :
                (&g_sigpool.iapu_iap_tu8_rec_blk_frm_ctx);
    }

    for (int i = 0; i < buf_size; i++)
        temp[i] = (p_rec_buf[i] & 0xff);

    sigdump_output_bin(p_ctx, temp, buf_size);
}
#endif //~SIG_IAPU

#ifdef SIG_IAPU
void sig_iapu_output_cu_winner(TComDataCU *p_best_cu, int cu_x, int cu_y, int cu_width)
{
    if (cu_width == 8)
    {
        sigdump_output_fprint(&g_sigpool.iapu_iap_cu8_cmd_frm_ctx, "# CU (X, Y) = (%d, %d)\n", cu_x, cu_y);

        // 8x8 x4 bottom-left, wait for CU16
        if ((cu_x & 0x8) != 0 && (cu_y & 0x8) != 0)
            return;

        sig_iapu_output_cu_cmd(p_best_cu, cu_width, 0);
    }
    else if (cu_width == 16)
    {
        // 16x16 x4 bottom-left, wait for CU32
        if (g_sigpool.slice_type != I_SLICE && (cu_x & 0x10) != 0 && (cu_y & 0x10) != 0)
        {
            return;
        }
        sig_iapu_output_cu_cmd(p_best_cu, cu_width, 12);
    }
    else if (cu_width == 32)
    {
        if (g_sigpool.slice_type != I_SLICE)
        {
            sig_iapu_output_cu_cmd(p_best_cu, cu_width, 60);
        }
    }
}
#endif //~SIG_IAPU

#ifdef SIG_IAPU
void sig_iapu_output_cu_cmd(TComDataCU *p_cu, int cu_width, int abs_idx)
{
    sig_ctx *p_ctx = &g_sigpool.iapu_iap_cu8_cmd_frm_ctx;

    int depth = p_cu->getDepth(abs_idx);
    sigdump_output_fprint(&g_sigpool.iapu_iap_cu8_cmd_frm_ctx, "cur_cu_upd_sz = %d\n", 3 - depth);

    int part_mode = (p_cu->getPartitionSize(abs_idx) == SIZE_NxN) ? 1 : 0;
    sigdump_output_fprint(p_ctx, "cur_cu_par_mode = %d\n", part_mode);

    UChar mode[4] = { 63, 63, 63, 63 };
    if (p_cu->getPredictionMode(abs_idx) == MODE_INTRA)
    {
        UInt uiInitTrDepth = (part_mode == 1) ? 1 : 0;
        TComTURecurse tuRecurseCU(p_cu, abs_idx);
        TComTURecurse tuRecurseWithPU(tuRecurseCU, false, (uiInitTrDepth==0)?TComTU::DONT_SPLIT : TComTU::QUAD_SPLIT);

        if (uiInitTrDepth)
        {
            UInt i = 0;
            do {
                const UInt uiPartOffset = tuRecurseWithPU.GetAbsPartIdxTU();
                mode[i] = p_cu->getIntraDir(CHANNEL_TYPE_LUMA)[uiPartOffset];
                i++;
            } while(tuRecurseWithPU.nextSection(tuRecurseCU));
            assert (i == 4);
        }
        else
        {
            const UInt uiPartOffset = tuRecurseWithPU.GetAbsPartIdxTU();
            UChar intra_mode = p_cu->getIntraDir(CHANNEL_TYPE_LUMA)[uiPartOffset];
            memset(mode, intra_mode, 4);
        }
    }
    sigdump_output_fprint(p_ctx, "cur_cu_intra_pred_mode = %d,%d,%d,%d\n", mode[0], mode[1], mode[2], mode[3]);

    sigdump_output_fprint(p_ctx, "cur_cu8_b = 0x%x\n", g_sigpool.iapu_st.neb_avail_b_iap[0][2]<<2 | g_sigpool.iapu_st.neb_avail_b_iap[0][1]<<1 | g_sigpool.iapu_st.neb_avail_b_iap[0][0]);
    sigdump_output_fprint(p_ctx, "cur_cu16_b = 0x%x\n", g_sigpool.iapu_st.neb_avail_b_iap[1][2]<<2 | g_sigpool.iapu_st.neb_avail_b_iap[1][1]<<1 | g_sigpool.iapu_st.neb_avail_b_iap[1][0]);
    sigdump_output_fprint(p_ctx, "cur_cu32_b = 0x%x\n", g_sigpool.iapu_st.neb_avail_b_iap[2][2]<<2 | g_sigpool.iapu_st.neb_avail_b_iap[2][1]<<1 | g_sigpool.iapu_st.neb_avail_b_iap[2][0]);
    sigdump_output_fprint(p_ctx, "cur_cu8_a = 0x%x\n", g_sigpool.iapu_st.neb_avail_a_iap[0][1]<<1 | g_sigpool.iapu_st.neb_avail_a_iap[0][0]);
    sigdump_output_fprint(p_ctx, "cur_cu16_a = 0x%x\n", g_sigpool.iapu_st.neb_avail_a_iap[1][1]<<1 | g_sigpool.iapu_st.neb_avail_a_iap[1][0]);
    sigdump_output_fprint(p_ctx, "cur_cu32_a = 0x%x\n", g_sigpool.iapu_st.neb_avail_a_iap[2][1]<<1 | g_sigpool.iapu_st.neb_avail_a_iap[2][0]);

    sigdump_output_fprint(p_ctx, "cur_blk16_lambda = 0x%x\n", g_sigpool.iapu_st.lambda);
    sigdump_output_fprint(p_ctx, "qp = 0x%x,0x%x,0x%x\n", g_sigpool.iapu_st.qp[0], g_sigpool.iapu_st.qp[1], g_sigpool.iapu_st.qp[2]);

    sig_iapu_output_cu_self_info(p_ctx);
    sigdump_output_fprint(p_ctx, "reg_dist_chroma_weight = %d\n", g_sigpool.dist_chroma_weight);
    sigdump_output_fprint(p_ctx, "reg_i_slice = %d\n", (p_cu->getSlice()->getSliceType()==I_SLICE));
    sigdump_output_fprint(p_ctx, "reg_enc_dis_inter_i4 = %d\n", g_algo_cfg.DisablePfrmIntra4 ? 1 : 0);
}
#endif

void sig_iapu_output_cu_self_info(sig_ctx *p_ctx)
{
    const string pred_s[] = {"inter", "intra"};
    const string color_s[] = {"luma", "chroma"};
    int predID = 1;
    int compID;

    for(int blk4_idx=0; blk4_idx<4; blk4_idx++){
        compID = 0;
        sigdump_output_fprint(p_ctx, "reg_%s_tu_bitest_%s_sfi = %d\n",  pred_s[predID].c_str(), color_s[compID].c_str(), g_sigpool.iapu_st.bitest_sfi[blk4_idx][compID]);
        compID = 1;
        sigdump_output_fprint(p_ctx, "reg_%s_tu_bitest_%s_sfi = %d\n",  pred_s[predID].c_str(), color_s[compID].c_str(), g_sigpool.iapu_st.bitest_sfi[blk4_idx][compID]);

        sigdump_output_fprint(p_ctx, "reg_%s_tu_pm_sfi = %d\n",  pred_s[predID].c_str(), g_sigpool.iapu_st.bitest_pm_sfi);

        compID = 0;
        sigdump_output_fprint(p_ctx, "reg_%s_bitest_%s_ratio = %d\n", pred_s[predID].c_str(), color_s[compID].c_str(), g_sigpool.iapu_st.bitest_ratio[blk4_idx][compID]);
        compID = 1;
        sigdump_output_fprint(p_ctx, "reg_%s_bitest_%s_ratio = %d\n", pred_s[predID].c_str(), color_s[compID].c_str(), g_sigpool.iapu_st.bitest_ratio[blk4_idx][compID]);

        compID = 0;
        sigdump_output_fprint(p_ctx, "reg_%s_tu_%s_m = %d, %d\n", pred_s[predID].c_str(), color_s[compID].c_str(), g_sigpool.iapu_st.m[blk4_idx][compID][0], g_sigpool.iapu_st.m[blk4_idx][compID][1]);
        sigdump_output_fprint(p_ctx, "reg_%s_tu_%s_c = %d, %d\n", pred_s[predID].c_str(), color_s[compID].c_str(), g_sigpool.iapu_st.c[blk4_idx][compID][0], g_sigpool.iapu_st.c[blk4_idx][compID][1]);
        compID = 1;
        sigdump_output_fprint(p_ctx, "reg_%s_tu_%s_m = %d, %d\n", pred_s[predID].c_str(), color_s[compID].c_str(), g_sigpool.iapu_st.m[blk4_idx][compID][0], g_sigpool.iapu_st.m[blk4_idx][compID][1]);
        sigdump_output_fprint(p_ctx, "reg_%s_tu_%s_c = %d, %d\n", pred_s[predID].c_str(), color_s[compID].c_str(), g_sigpool.iapu_st.c[blk4_idx][compID][0], g_sigpool.iapu_st.c[blk4_idx][compID][1]);

    }
}
void sig_iapu_save_blk4_self_info(int blk4_y_idx, int blk4_c_idx)         // reference to sig_rru_output_cu_self_info
{
    unsigned int bitest_sfi;
    int predID = 1;
    int compID;
//    sigdump_output_fprint(&g_sigpool.iapu_iap_cu8_cmd_frm_ctx, "[DBG] y_idx=%0d, c_idx=%0d\n", blk4_y_idx, blk4_c_idx);

    if(blk4_y_idx>=0){
        compID = 0;
        bitest_sfi = (predID == 0) ? 0 : g_sigpool.rru_gold.rru_est_gold_4x4[compID][0].bitest_sfi;
        g_sigpool.iapu_st.bitest_sfi[blk4_y_idx][compID] = bitest_sfi;
        int log2_size_m2 = 0;
        g_sigpool.iapu_st.bitest_ratio[blk4_y_idx][compID] = rru_get_last_pos_scale(g_lastPosBinScale_init[compID][predID][log2_size_m2]);
        const int *m = &g_significantScale[g_scale_D][compID][predID][log2_size_m2][0];
        const int *c = &g_significantBias[g_scale_D][compID][predID][log2_size_m2][0];
        g_sigpool.iapu_st.m[blk4_y_idx][compID][0] = m[0];
        g_sigpool.iapu_st.m[blk4_y_idx][compID][1] = m[1];
        g_sigpool.iapu_st.c[blk4_y_idx][compID][0] = c[0];
        g_sigpool.iapu_st.c[blk4_y_idx][compID][1] = c[1];
        g_sigpool.iapu_st.bitest_pm_sfi = g_sigpool.rru_gold.rru_est_gold_4x4[compID][0].bitest_pm_sfi;
    }
    if(blk4_c_idx>=0){
        compID = 1;
        bitest_sfi = (predID == 0) ? 0 : g_sigpool.rru_gold.rru_est_gold_4x4[compID][0].bitest_sfi;
        g_sigpool.iapu_st.bitest_sfi[blk4_c_idx][compID] = bitest_sfi;
        int log2_size_m2 = 0;
        g_sigpool.iapu_st.bitest_ratio[blk4_c_idx][compID] = rru_get_last_pos_scale(g_lastPosBinScale_init[compID][predID][log2_size_m2]);
        const int *m = &g_significantScale[g_scale_D][compID][predID][log2_size_m2][0];
        const int *c = &g_significantBias[g_scale_D][compID][predID][log2_size_m2][0];
        g_sigpool.iapu_st.m[blk4_c_idx][compID][0] = m[0];
        g_sigpool.iapu_st.m[blk4_c_idx][compID][1] = m[1];
        g_sigpool.iapu_st.c[blk4_c_idx][compID][0] = c[0];
        g_sigpool.iapu_st.c[blk4_c_idx][compID][1] = c[1];
    }
}

void sig_iapu_output_cu_neb_wb(sig_ctx *p_ctx, const Pel *p_rec, int size, int is_b, ComponentID id)    // copy from sig_rru_output_cu_iap
{
    unsigned char y_nb[32]={0}, c_nb[16]={0};
    unsigned char *output_nb = NULL;
    unsigned char *p_nb = NULL;

    if(id == COMPONENT_Y){
        output_nb = y_nb;
    }else{
        output_nb = c_nb;
    }
    p_nb = &(output_nb[0]);

    int nb_start = 0;
    int nb_offset = 0;
    if (is_b)
    {
        nb_start  = (size - 1) * size;                                                          //mj: bottom
        nb_offset = 1;
    }
    else
    {
        nb_start  = size - 1;                                                                   //mj: right
        nb_offset = size;
    }

    for (int i = 0; i < size; i++)
    {
        *p_nb = p_rec[nb_start + i * nb_offset];
        p_nb++;
    }

    if(id == COMPONENT_Y){
        sigdump_output_bin(p_ctx, output_nb, 32);
    }else{
        sigdump_output_bin(p_ctx, output_nb, 16);
    }
}
void sig_iapu_output_ctu_bot_wb(TComDataCU* pCtu)
{
    uint8_t y_bot[64]={0};
    uint8_t c_bot[64]={0};
    Pel*  piSrc;
    UInt  uiSrcStride;

    piSrc       = pCtu->getPic()->getPicYuvRec()->getAddr( COMPONENT_Y, pCtu->getCtuRsAddr() );
    uiSrcStride = pCtu->getPic()->getPicYuvRec()->getStride( COMPONENT_Y );

    for( UInt uiX = 0; uiX < 64; uiX++ )
        y_bot[uiX] = piSrc[63*uiSrcStride + uiX];

    piSrc       = pCtu->getPic()->getPicYuvRec()->getAddr( COMPONENT_Cb, pCtu->getCtuRsAddr() );
    uiSrcStride = pCtu->getPic()->getPicYuvRec()->getStride( COMPONENT_Cb );
    for( UInt uiX = 0; uiX < 32; uiX++ )
        c_bot[uiX*2] = piSrc[32*uiSrcStride + uiX];

    piSrc       = pCtu->getPic()->getPicYuvRec()->getAddr( COMPONENT_Cr, pCtu->getCtuRsAddr() );
    uiSrcStride = pCtu->getPic()->getPicYuvRec()->getStride( COMPONENT_Cr );
    for( UInt uiX = 0; uiX < 32; uiX++ )
        c_bot[uiX*2+1] = piSrc[32*uiSrcStride + uiX];

    sigdump_output_bin(&g_sigpool.iapu_iap_line_buffer_frm_ctx, y_bot, 64*sizeof(uint8_t));
    sigdump_output_bin(&g_sigpool.iapu_iap_line_buffer_frm_ctx, c_bot, 64*sizeof(uint8_t));
}

void sig_iapu_update_final_buf(bool is_chroma, bool is_intra, int size)
{
    // only intra needs to update intp
    if (is_intra)
    {
        if (is_chroma)
        {
            if (g_sigpool.luma_pu_size == 4)
            {
                memcpy(g_sigpool.iapu_st.p_iapu_i4_coef_final[1], g_sigpool.iapu_st.p_iapu_i4_coef_temp[1], 16 * sizeof(int));
                memcpy(g_sigpool.iapu_st.p_iapu_i4_coef_final[2], g_sigpool.iapu_st.p_iapu_i4_coef_temp[2], 16 * sizeof(int));
            }
        }
        else if (size == 4)
        {
            memcpy(g_sigpool.iapu_st.p_iapu_i4_coef_final[0], g_sigpool.iapu_st.p_iapu_i4_coef_temp[0], 16 * sizeof(int));
        }
    }
}

void sig_iapu_output_i4_resi(bool is_chroma)
{
    sig_ctx *p_ctx = &g_sigpool.iapu_iap_tu4_coef_blk_frm_ctx;
    short temp[16] = { 0 };

    int start_idx = 0;
    int end_idx = 1;
    if(is_chroma)
    {
        start_idx = 1;
        end_idx = 3;
    }

    for (int i = start_idx; i < end_idx; i++)
    {
        int *p_src = g_sigpool.iapu_st.p_iapu_i4_coef_final[i];
        for (int j = 0; j < 16; j++)
            temp[j] = (short)(p_src[j]);

        sigdump_output_bin(p_ctx, (unsigned char *)(temp), sizeof(temp));
    }
}
void sig_iapu_output_i4_pred()
{
    sig_ctx *p_ctx = &g_sigpool.iapu_iap_tu4_pred_frm_ctx;

    for(int mode=0; mode<6; mode++){
        sigdump_output_bin(p_ctx, g_sigpool.iapu_st.tu4_pred_blk[COMPONENT_Y][mode], 16*sizeof(UChar));
    }
    for(int mode=0; mode<5; mode++){
        sigdump_output_bin(p_ctx, g_sigpool.iapu_st.tu4_pred_blk[COMPONENT_Cb][mode], 16*sizeof(UChar));
    }
    for(int mode=0; mode<5; mode++){
        sigdump_output_bin(p_ctx, g_sigpool.iapu_st.tu4_pred_blk[COMPONENT_Cr][mode], 16*sizeof(UChar));
    }
}
#endif //~SIG_IAPU

#ifdef SIG_IAPU
void sig_iap_output_i4term_temp_data()
{
    // coeff
    for (int i = 0; i < 3; i++)
        memset(g_sigpool.iapu_st.p_iapu_i4_coef_final[i], 0, sizeof(int) * 16);

    sig_iapu_output_i4_resi(false);
    sig_iapu_output_i4_resi(false);
    sig_iapu_output_i4_resi(false);
    sig_iapu_output_i4_resi(false);
    sig_iapu_output_i4_resi(true);

    // predict
    memset(g_sigpool.iapu_st.tu4_pred_blk, 0, sizeof(g_sigpool.iapu_st.tu4_pred_blk));
    sig_iapu_output_i4_pred();

    // reconstruct, 4x4x4
    unsigned char temp[64];
    memset(temp, 0, sizeof(temp));
    sigdump_output_bin(&g_sigpool.iapu_iap_tu4_rec_blk_frm_ctx, temp, sizeof(temp));             // y
    sigdump_output_bin(&g_sigpool.iapu_iap_tu4_rec_blk_frm_ctx, temp, 32*sizeof(unsigned char)); // cb+cr

    // rmd predict, 4 x (4x4 x 3 rmd modes)
    for (int i = 0; i < 4; i++) {
        sigdump_output_bin(&g_sigpool.iapu_rmd_tu_pred_blk_frm_ctx[0], temp, 48);
    }

    // rmd source
    sigdump_output_bin(&g_sigpool.iapu_rmd_tu_src_frm_ctx[0], temp, sizeof(temp));

    sig_ctx *p_ctx = NULL;
    for (int blk4 = 0; blk4 < 4; blk4++)
    {
        // temp rmd tu4 cand
        p_ctx = &g_sigpool.iapu_rmd_tu_cand_frm_ctx[0];
        sigdump_output_fprint(p_ctx, "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(p_ctx, "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(p_ctx, "# NEB_A\n");
        for (UInt i = 1; i < 8; i++) {
            sigdump_output_fprint(p_ctx, "  0 ");
        }
        sigdump_output_fprint(p_ctx, "\n# NEB_B\n");
        for (UInt i = 1; i < 8; i++) {
            sigdump_output_fprint(p_ctx, "  0 ");
        }
        sigdump_output_fprint(p_ctx, "\n# NEB_C\n");
        sigdump_output_fprint(p_ctx, "  0 \n");

        for (int i = 0; i < 3; i++)
        {
            sigdump_output_fprint(p_ctx, "# MODE=0\n");
            sigdump_output_fprint(p_ctx, "# SAD=0\n");
        }
        sigdump_output_fprint(p_ctx, "# CANDIDATES\n");
        sigdump_output_fprint(p_ctx, "63 63 63\n");

        // temp rmd tu4 cmd
        p_ctx = &g_sigpool.iapu_rmd_tu_cmd_frm_ctx[0];
        sigdump_output_fprint(p_ctx, "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(p_ctx, "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(p_ctx, "# EDGE=7\n");
    }

    // iap syntax
    p_ctx = &g_sigpool.iapu_iap_syntax;
    for (int blk4 = 0; blk4 < 4; blk4++)
    {
        sigdump_output_fprint(p_ctx, "cu4_prev_flag = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_mpm_idx = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_rem_pred_mode = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_intra_chroma_pred_mode = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_luma_pred_mode = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_chroma_pred_mode = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_luma_scan_idx = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_chroma_scan_idx = 0\n");

        sigdump_output_fprint(p_ctx, "cu4_best_rdo = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_best_sse = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_luma_csbf = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_luma_last_xy = 0, 0\n");
        sigdump_output_fprint(p_ctx, "cu4_cb_csbf = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_cb_last_xy = 0, 0\n");
        sigdump_output_fprint(p_ctx, "cu4_cr_csbf = 0\n");
        sigdump_output_fprint(p_ctx, "cu4_cr_last_xy = 0, 0\n");
    }

    // iap i4 cand
    p_ctx = &g_sigpool.iapu_iap_tu_cand_frm_ctx[0];
    for (int blk4 = 0; blk4 < 4; blk4++)
    {
        sigdump_output_fprint(p_ctx, "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(p_ctx, "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        for (int mode = 0; mode < 6; mode++)
        {
            sigdump_output_fprint(p_ctx, "L_MODE = 0\n");
            sigdump_output_fprint(p_ctx, "RDO_COST = 0,0,0,0,0\n");
        }
        sigdump_output_fprint(p_ctx, "L_BEST_MODE = 0\n");
    }
    for (int mode = 0; mode < 5; mode++)
    {
        sigdump_output_fprint(p_ctx, "C_MODE = 0\n");
        sigdump_output_fprint(p_ctx, "RDO_COST = 0,0,0,0,0\n");
    }
    sigdump_output_fprint(p_ctx, "C_BEST_MODE = 0\n");
    sigdump_output_fprint(p_ctx, "CU_BEST_RDO = 0\n");

    // iap i4 cmd
    p_ctx = &g_sigpool.iapu_iap_tu_cmd_frm_ctx[0];
    for (int blk4 = 0; blk4 < 4; blk4++)
    {
        sigdump_output_fprint(p_ctx, "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(p_ctx, "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(p_ctx, "# RMD_CAND\n0 0 0\n");
        sigdump_output_fprint(p_ctx, "# MPM\n0 0 0\n");
        sigdump_output_fprint(p_ctx, "# LUMA_CAND\n0 0 0 0 0\n");
        sigdump_output_fprint(p_ctx, "# L_QP = 0\n");
        sigdump_output_fprint(p_ctx, "# CB_QP = 0\n");
        sigdump_output_fprint(p_ctx, "# CR_QP = 0\n");
    }

    // iap i4 neb
    p_ctx = &g_sigpool.iapu_iap_tu_neb_frm_ctx[0];
    for (int blk4 = 0; blk4 < 4; blk4++)
    {
        sigdump_output_fprint(p_ctx, "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(p_ctx, "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(p_ctx, "# L_NEB_A\n  0   0   0   0   0   0   0   0\n");
        sigdump_output_fprint(p_ctx, "# L_NEB_B\n  0   0   0   0   0   0   0   0\n");
        sigdump_output_fprint(p_ctx, "# L_NEB_C\n  0\n");
    }
    sigdump_output_fprint(p_ctx, "# CB_NEB_A\n  0   0   0   0   0   0   0   0\n");
    sigdump_output_fprint(p_ctx, "# CB_NEB_B\n  0   0   0   0   0   0   0   0\n");
    sigdump_output_fprint(p_ctx, "# CB_NEB_C\n  0\n");
    sigdump_output_fprint(p_ctx, "# CR_NEB_A\n  0   0   0   0   0   0   0   0\n");
    sigdump_output_fprint(p_ctx, "# CR_NEB_B\n  0   0   0   0   0   0   0   0\n");
    sigdump_output_fprint(p_ctx, "# CR_NEB_C\n  0\n");
}
#endif //~SIG_IAPU

#ifdef SIG_IAPU
void sig_iapu_output_iap_syntax(TComDataCU *p_cu, PartSize part_size, TComRdCost *p_rd)
{
    sig_ctx *p_ctx = &g_sigpool.iapu_iap_syntax;
    int cu_x = p_cu->getCUPelX();
    int cu_y = p_cu->getCUPelY();
    int cu_width = p_cu->getWidth(0);
    int part_num = (part_size == SIZE_NxN) ? 4 : 1;
    int tu_size = (part_size == SIZE_NxN) ? 4 : cu_width;
    int tu_size_c = (tu_size >> 1);
    UInt64 i4_cost = 0;
    UInt64 i4_dist = 0;

#ifdef IAPU_IAP_SYNTAX_BIN
    unsigned char output_buf[9] = { 0 };
    if (part_size == SIZE_2Nx2N)
    {
        if (!(cu_width == 8 && (cu_x & 0xf) == 0 && (cu_y & 0xf) == 0))
        {
            sigdump_output_bin(p_ctx, (unsigned char *)(&cu_x), 4);
            sigdump_output_bin(p_ctx, (unsigned char *)(&cu_y), 4);
        }
    }
    else
    {
        i4_cost = p_rd->getFixpointRdCost(p_cu->getTotalCost());
    }

    output_buf[7] = p_cu->getCoefScanIdx(0, tu_size_c, tu_size_c, COMPONENT_Cb);
    for (int i = 0; i < part_num; i++)
    {
        output_buf[0] = (unsigned char)(g_sigpool.intra_golden.prev_luma_pred_flag[i]);
        output_buf[1] = (unsigned char)(g_sigpool.intra_golden.mpm_idx[i]);
        output_buf[2] = (unsigned char)(g_sigpool.intra_golden.rem_luma_pred_mode[i]);
        output_buf[3] = (unsigned char)(g_sigpool.intra_golden.intra_chroma_pred_mode);
        output_buf[4] = (unsigned char)(g_sigpool.intra_golden.luma_pred_mode[i]);
        output_buf[5] = (unsigned char)(g_sigpool.intra_golden.chroma_pred_mode);
        output_buf[6] = p_cu->getCoefScanIdx(i, tu_size, tu_size, COMPONENT_Y);
        sigdump_output_bin(p_ctx, output_buf, sizeof(unsigned char) * 8);

        if (part_size == SIZE_NxN)
        {
            sigdump_output_bin(p_ctx, (unsigned char *)(&i4_cost), sizeof(UInt64));

            output_buf[0] = (unsigned char)(g_sigpool.rru_gold.rru_est_gold_4x4[0][i].csbf);
            output_buf[1] = (unsigned char)(g_sigpool.rru_gold.rru_est_gold_4x4[0][i].last_x);
            output_buf[2] = (unsigned char)(g_sigpool.rru_gold.rru_est_gold_4x4[0][i].last_y);
            output_buf[3] = (unsigned char)(g_sigpool.rru_gold.rru_est_gold_4x4[1][0].csbf);
            output_buf[4] = (unsigned char)(g_sigpool.rru_gold.rru_est_gold_4x4[1][0].last_x);
            output_buf[5] = (unsigned char)(g_sigpool.rru_gold.rru_est_gold_4x4[1][0].last_y);
            output_buf[6] = (unsigned char)(g_sigpool.rru_gold.rru_est_gold_4x4[2][0].csbf);
            output_buf[7] = (unsigned char)(g_sigpool.rru_gold.rru_est_gold_4x4[2][0].last_x);
            output_buf[8] = (unsigned char)(g_sigpool.rru_gold.rru_est_gold_4x4[2][0].last_y);
            sigdump_output_bin(p_ctx, output_buf, sizeof(output_buf));
        }
    }
#else
    if (part_size == SIZE_2Nx2N)
    {
        if (!(cu_width == 8 && (cu_x & 0xf) == 0 && (cu_y & 0xf) == 0))
        {
            sigdump_output_fprint(p_ctx, "# CU (X, Y) = (%d, %d)\n", cu_x, cu_y);
        }
    }
    else
    {
        i4_cost = p_rd->getFixpointRdCost(p_cu->getTotalCost());
        i4_dist = p_cu->getTotalDistortion();
    }

    UChar prefix[3][5] = { "cu16", "cu8", "cu4"};
    int idx = (cu_width == 16) ? 0 : ((part_size == SIZE_2Nx2N) ? 1 : 2);
    UChar scan_idx_c = p_cu->getCoefScanIdx(0, tu_size_c, tu_size_c, COMPONENT_Cb);
    for (int i = 0; i < part_num; i++)
    {
        UChar scan_idx_y = p_cu->getCoefScanIdx(i, tu_size, tu_size, COMPONENT_Y);
        sigdump_output_fprint(p_ctx, "%s_prev_flag = %d\n", prefix[idx], (unsigned char)(g_sigpool.intra_golden.prev_luma_pred_flag[i]));
        sigdump_output_fprint(p_ctx, "%s_mpm_idx = %d\n", prefix[idx], (unsigned char)(g_sigpool.intra_golden.mpm_idx[i]));
        sigdump_output_fprint(p_ctx, "%s_rem_pred_mode = %d\n", prefix[idx], (unsigned char)(g_sigpool.intra_golden.rem_luma_pred_mode[i]));
        sigdump_output_fprint(p_ctx, "%s_intra_chroma_pred_mode = %d\n", prefix[idx], (unsigned char)(g_sigpool.intra_golden.intra_chroma_pred_mode));
        sigdump_output_fprint(p_ctx, "%s_luma_pred_mode = %d\n", prefix[idx], (unsigned char)(g_sigpool.intra_golden.luma_pred_mode[i]));
        sigdump_output_fprint(p_ctx, "%s_chroma_pred_mode = %d\n", prefix[idx], (unsigned char)(g_sigpool.intra_golden.chroma_pred_mode));
        sigdump_output_fprint(p_ctx, "%s_luma_scan_idx = %d\n", prefix[idx], scan_idx_y);
        sigdump_output_fprint(p_ctx, "%s_chroma_scan_idx = %d\n", prefix[idx], scan_idx_c);

        if (part_size == SIZE_NxN)
        {
            sigdump_output_fprint(p_ctx, "cu4_best_rdo = %llu\n", i4_cost);
            sigdump_output_fprint(p_ctx, "cu4_best_sse = %llu\n", i4_dist);
            sigdump_output_fprint(p_ctx, "cu4_luma_csbf = %d\n", g_sigpool.rru_gold.rru_est_gold_4x4[0][i].csbf);
            sigdump_output_fprint(p_ctx, "cu4_luma_last_xy = %d, %d\n",
                                  g_sigpool.rru_gold.rru_est_gold_4x4[0][i].last_x,
                                  g_sigpool.rru_gold.rru_est_gold_4x4[0][i].last_y);
            sigdump_output_fprint(p_ctx, "cu4_cb_csbf = %d\n", g_sigpool.rru_gold.rru_est_gold_4x4[1][0].csbf);
            sigdump_output_fprint(p_ctx, "cu4_cb_last_xy = %d, %d\n",
                                  g_sigpool.rru_gold.rru_est_gold_4x4[1][0].last_x,
                                  g_sigpool.rru_gold.rru_est_gold_4x4[1][0].last_y);
            sigdump_output_fprint(p_ctx, "cu4_cr_csbf = %d\n", g_sigpool.rru_gold.rru_est_gold_4x4[2][0].csbf);
            sigdump_output_fprint(p_ctx, "cu4_cr_last_xy = %d, %d\n",
                                  g_sigpool.rru_gold.rru_est_gold_4x4[2][0].last_x,
                                  g_sigpool.rru_gold.rru_est_gold_4x4[2][0].last_y);
        }
    }
#endif
}
#endif //~SIG_IAPU

#ifdef SIG_TOP
void sig_top_reg_write(sig_ctx *p_ctx, unsigned int base, unsigned int addr, unsigned int value)
{
    sigdump_output_fprint(p_ctx, "WR 0x%08x 0x%08x\n", (base + addr), value);
}

void sig_top_reg_cmdq_write(sig_ctx *p_ctx, unsigned int base, unsigned int addr, unsigned int value, int end)
{
    unsigned char temp;
    //sigdump_output_fprint(p_ctx, "0x0 0x%d 0xf 0x%08x 0x%08x\n", end, (base + addr), value);
    int i=0;
    for(i=0;i<4;i++)
    {
        temp = (value>>(i*8))&0xff;
        sigdump_output_bin(p_ctx, &temp, 1);
    }
    for(i=0;i<2;i++)
    {
        temp = ((base + addr)>>(i*8+2))&0xff;
        sigdump_output_bin(p_ctx, &temp, 1);
    }
    i = 2;
    temp = ((base + addr)>>(i*8+2))&0xff;
    temp |= 0xf0;
    sigdump_output_bin(p_ctx, &temp, 1);
    temp = end;
    sigdump_output_bin(p_ctx, &temp, 1);
}

void sig_top_reg_read(sig_ctx *p_ctx, unsigned int base, unsigned int addr, unsigned int value, string comment)
{
    sigdump_output_fprint(p_ctx, "RD 0x%08x 0x%08x #%s\n", (base + addr), value, comment.c_str());
}

const int QPMAP_BLK_X[16] = {0, 16, 0, 16, 32, 48, 32, 48,
                             0, 16, 0, 16, 32, 48, 32, 48};
const int QPMAP_BLK_Y[16] = {0, 0, 16, 16, 0, 0, 16, 16,
                             32, 32, 48, 48, 32, 32, 48, 48};

void sig_top_output_qpmap()
{
    int width = g_sigpool.width;
    int height = g_sigpool.height;
    int pitch = cvi_mem_align(width, 512);  // 512 aligned
    sig_ctx *p_ctx = &g_sigpool.top_qp_map;

    unsigned char ctu_data[16];
    for (int ctu_y = 0; ctu_y < height; ctu_y += 64)
    {
        for (int ctu_x = 0; ctu_x < pitch; ctu_x += 64)
        {
            for (int idx = 0; idx < 16; idx++)
            {
                int blk_x = ctu_x + QPMAP_BLK_X[idx];
                int blk_y = ctu_y + QPMAP_BLK_Y[idx];

                if ((blk_x < width) && (blk_y < height))
                    ctu_data[idx] = (unsigned char)(g_blk16_qp_map.get_data(blk_x, blk_y));
                else
                    ctu_data[idx] = 0;
            }
            sigdump_output_bin(p_ctx, ctu_data, 16);
        }
    }
}

char *read_file_into_buf (char **filebuf, long *fplen, FILE *fp)
{
    fseek (fp, 0, SEEK_END);
    if ((*fplen = ftell (fp)) == -1) {  /* get file length */
        fprintf (stderr, "error: unable to determine file length.\n");
        return NULL;
    }
    fseek (fp, 0, SEEK_SET);  /* allocate memory for file */
    if (!(*filebuf = (char *)calloc (*fplen, sizeof *filebuf))) {
        fprintf (stderr, "error: virtual memory exhausted.\n");
        return NULL;
    }

    /* read entire file into filebuf */
    if (!fread (*filebuf, sizeof *filebuf, *fplen, fp)) {
        fprintf (stderr, "error: file read failed.\n");
        return NULL;
    }

    return *filebuf;
}

unsigned find_replace_text (const char *find, const char *rep, char *buf, long sz)
{
    long i;
    unsigned rpc = 0;
    size_t j, flen, rlen;

    flen = strlen (find);
    rlen = strlen (rep);

    for (i = 0; i < sz; i++) {
        /* if char doesn't match first in find, continue */
        if (buf[i] != *find) continue;

        /* if find found, replace with rep */
        if (strncmp (&buf[i], find, flen) == 0) {
            for (j = 0; buf[i + j] && j < rlen; j++)
                buf[i + j] = rep[j];
            if (buf[i + j])
              rpc++;
        }
    }

    return rpc;
}

int sig_overwrite_bs_buffer(int mode, int slice_idx)
{
    string SigdumpFileName = g_FileName + "_frm_" + to_string(g_sigpool.enc_count_pattern) + "_slice_" + to_string(slice_idx) + "_fw.txt";
    FILE *fw_fp = fopen(SigdumpFileName.c_str(), "r");
    char *filebuf = NULL;
    long int fplen = 0;

    if (!read_file_into_buf (&filebuf, &fplen, fw_fp)) return 1;
    if (fplen < 1 || fplen >= INT_MAX) { /* validate file length */
        fprintf (stderr, "error: length of file invalid for fwrite use.\n");
        return 1;
    }
    if (fw_fp != stdin) fclose (fw_fp);

    REG64_C bs_base, bs_end_base;
    char VC_MM_hD8[128];
    char VC_MM_hDC[128];
    //overflow test
    if (mode == 1)
    {

        bs_base.val = get_hw_base("bs");
        bs_end_base.val = bs_base.val + cvi_mem_align(g_sigpool.slice_data_size[0] * 7 / 10, 128);
        sprintf(VC_MM_hD8, "WR 0x0b0200d8 0x%08x", bs_end_base.reg_lsb);
        sprintf(VC_MM_hDC, "WR 0x0b0200dc 0x%08x", bs_end_base.reg_msb);

        /* find/replace text in filebuf */
        if (!find_replace_text ("WR 0x0b0200d8", VC_MM_hD8, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
        if (!find_replace_text ("WR 0x0b0200dc", VC_MM_hDC, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }

        string SigdumpFileName = g_FileName + "_frm_" + to_string(g_sigpool.enc_count_pattern) + "_slice_" + to_string(slice_idx) + "_overflow.txt";
        FILE *overflow_fp = fopen(SigdumpFileName.c_str(), "w");
        bs_base.val = bs_end_base.val;
        bs_end_base.val = get_hw_base("bs") + get_hw_size("bs");
        fprintf(overflow_fp, "WR 0x0b0200d0 0x%08x\n", bs_base.reg_lsb);
        fprintf(overflow_fp, "WR 0x0b0200d4 0x%08x\n", bs_base.reg_msb);
        fprintf(overflow_fp, "WR 0x0b0200d8 0x%08x\n", bs_end_base.reg_lsb);
        fprintf(overflow_fp, "WR 0x0b0200dc 0x%08x\n", bs_end_base.reg_msb);
        fclose(overflow_fp);
    }
    else if (mode == 2)
    {
        //ring buffer
        bs_base.val = get_hw_base("bs");
        bs_end_base.val = bs_base.val + get_hw_size("bs");
        bs_base.val = bs_end_base.val - cvi_mem_align(g_sigpool.slice_data_size[0] * 7 / 10, 128) + 128;
        bs_end_base.val = get_hw_base("bs") + cvi_mem_align(g_sigpool.slice_data_size[0], 128);
        char VC_MM_hD0[128];
        char VC_MM_hD4[128];
        char VC_MM_hD8[128];
        char VC_MM_hDC[128];
        sprintf(VC_MM_hD0, "WR 0x0b0200d0 0x%08x", bs_base.reg_lsb);
        sprintf(VC_MM_hD4, "WR 0x0b0200d4 0x%08x", bs_base.reg_msb);
        sprintf(VC_MM_hD8, "WR 0x0b0200d8 0x%08x", bs_end_base.reg_lsb);
        sprintf(VC_MM_hDC, "WR 0x0b0200dc 0x%08x", bs_end_base.reg_msb);

        /* find/replace text in filebuf */
        if (!find_replace_text ("WR 0x0b0200d0", VC_MM_hD0, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
        if (!find_replace_text ("WR 0x0b0200d4", VC_MM_hD4, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
        /* find/replace text in filebuf */
        if (!find_replace_text ("WR 0x0b0200d8", VC_MM_hD8, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
        if (!find_replace_text ("WR 0x0b0200dc", VC_MM_hDC, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
    }
    else if (mode >= 3)
    {

        bs_base.val = get_hw_base("bs");
        bs_end_base.val = bs_base.val + cvi_mem_align(g_sigpool.slice_data_size[0] * 7 / 10, 128);
        sprintf(VC_MM_hD8, "WR 0x0b0200d8 0x%08x", bs_end_base.reg_lsb);
        sprintf(VC_MM_hDC, "WR 0x0b0200dc 0x%08x", bs_end_base.reg_msb);

        /* find/replace text in filebuf */
        if (!find_replace_text ("WR 0x0b0200d8", VC_MM_hD8, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
        if (!find_replace_text ("WR 0x0b0200dc", VC_MM_hDC, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }

        string SigdumpFileName = g_FileName + "_frm_" + to_string(g_sigpool.enc_count_pattern) + "_slice_" + to_string(slice_idx) + "_overflow.txt";
        FILE *overflow_fp = fopen(SigdumpFileName.c_str(), "w");
        bs_base.val = bs_end_base.val + (g_sigdump.testBsOverflow - 2)*128;  // (testBsOverflow-2)x128, align128
        bs_end_base.val = get_hw_base("bs") + get_hw_size("bs");
        fprintf(overflow_fp, "WR 0x0b0200d0 0x%08x\n", bs_base.reg_lsb);
        fprintf(overflow_fp, "WR 0x0b0200d4 0x%08x\n", bs_base.reg_msb);
        fprintf(overflow_fp, "WR 0x0b0200d8 0x%08x\n", bs_end_base.reg_lsb);
        fprintf(overflow_fp, "WR 0x0b0200dc 0x%08x\n", bs_end_base.reg_msb);
        fclose(overflow_fp);
    }
    /* open file for writing (default stdout) */
    fw_fp = fopen(SigdumpFileName.c_str(), "w");

    /* write modified filebuf back to filename */
    if (fwrite (filebuf, sizeof *filebuf, (size_t)fplen, fw_fp) != (size_t)fplen) {
        fprintf (stderr, "error: file write failed.\n");
        return 1;
    }
    if (fw_fp != stdout) 
        if (fclose (fw_fp) == EOF) {
            fprintf (stderr, "error: fclose() returned EOF\n");
            return 1;
        }

    free (filebuf);

    return 0;
}

int sig_ccu_overwrite_bs_buffer(int mode, int slice_idx)
{
    string SigdumpFileName = g_FileName + "_ccu_param_frm_" + to_string(g_sigpool.enc_count_pattern) + "_slice_" + to_string(slice_idx) + ".txt";
    FILE *fw_fp = fopen(SigdumpFileName.c_str(), "r");
    char *filebuf = NULL;
    long int fplen = 0;

    if (!read_file_into_buf (&filebuf, &fplen, fw_fp)) return 1;
    if (fplen < 1 || fplen >= INT_MAX) { /* validate file length */
        fprintf (stderr, "error: length of file invalid for fwrite use.\n");
        return 1;
    }
    if (fw_fp != stdin) fclose (fw_fp);


    REG64_C bs_base, bs_end_base;
    //overflow test
    if (mode == 1 || mode >=3)
    {
        char VC_MM_hD8[128];
        char VC_MM_hDC[128];

        bs_base.val = get_hw_base("bs");
        bs_end_base.val = bs_base.val + cvi_mem_align(g_sigpool.slice_data_size[0] * 7 / 10, 128);

        sprintf(VC_MM_hD8, "reg_bs_out_ubase_lsb = 0x%08x", bs_end_base.reg_lsb);
        sprintf(VC_MM_hDC, "reg_bs_out_ubase_msb = 0x%08x", bs_end_base.reg_msb);
        /* find/replace text in filebuf */
        if (!find_replace_text ("reg_bs_out_ubase_lsb", VC_MM_hD8, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
        if (!find_replace_text ("reg_bs_out_ubase_msb", VC_MM_hDC, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
    }
    else if (mode == 2)
    {
        //ring buffer
        bs_base.val = get_hw_base("bs");
        bs_end_base.val = bs_base.val + get_hw_size("bs");
        bs_base.val = bs_end_base.val - cvi_mem_align(g_sigpool.slice_data_size[0] * 7 / 10, 128) + 128;
        bs_end_base.val = get_hw_base("bs") + cvi_mem_align(g_sigpool.slice_data_size[0], 128);
        char VC_MM_hD0[128];
        char VC_MM_hD4[128];
        char VC_MM_hD8[128];
        char VC_MM_hDC[128];

        sprintf(VC_MM_hD0, "reg_bs_out_base_lsb = 0x%08x", bs_base.reg_lsb);
        sprintf(VC_MM_hD4, "reg_bs_out_base_msb = 0x%08x", bs_base.reg_msb);
        sprintf(VC_MM_hD8, "reg_bs_out_ubase_lsb = 0x%08x", bs_end_base.reg_lsb);
        sprintf(VC_MM_hDC, "reg_bs_out_ubase_msb = 0x%08x", bs_end_base.reg_msb);

        /* find/replace text in filebuf */
        if (!find_replace_text ("reg_bs_out_base_lsb", VC_MM_hD0, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
        if (!find_replace_text ("reg_bs_out_base_msb", VC_MM_hD4, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
        /* find/replace text in filebuf */
        if (!find_replace_text ("reg_bs_out_ubase_lsb", VC_MM_hD8, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
        if (!find_replace_text ("reg_bs_out_ubase_msb", VC_MM_hDC, filebuf, fplen)) {
            printf ("no replacements made.\n");
            return 0;
        }
    }

    /* open file for writing (default stdout) */
    fw_fp = fopen(SigdumpFileName.c_str(), "w");

    /* write modified filebuf back to filename */
    if (fwrite (filebuf, sizeof *filebuf, (size_t)fplen, fw_fp) != (size_t)fplen) {
        fprintf (stderr, "error: file write failed.\n");
        return 1;
    }
    if (fw_fp != stdout) 
        if (fclose (fw_fp) == EOF) {
            fprintf (stderr, "error: fclose() returned EOF\n");
            return 1;
        }

    free (filebuf);

    return 0;
}
#endif //~SIG_TOP

#ifdef SIG_PPU
void sig_ppu_output_reg(TComSlice *p_slice)
{
    sig_ctx *p_ctx = &g_sigpool.p_ppu_st->ppu_reg_ctx;
    const TComSPS *p_sps = p_slice->getSPS();
    int min_cu_size = (1 << p_sps->getLog2MinCodingBlockSize());
    int pic_w_cu_m1 = cvi_mem_align_mult(p_sps->getPicWidthInLumaSamples(), min_cu_size) - 1;
    int pic_h_cu_m1 = cvi_mem_align_mult(p_sps->getPicHeightInLumaSamples(), min_cu_size) - 1;

    sigdump_output_fprint(p_ctx, "reg_pic_width_cu_m1 = %d\n", pic_w_cu_m1);
    sigdump_output_fprint(p_ctx, "reg_pic_height_cu_m1 = %d\n", pic_h_cu_m1);

    sigdump_output_fprint(p_ctx, "reg_slice_ilf_dis = %d\n", p_slice->getDeblockingFilterDisable());
    sigdump_output_fprint(p_ctx, "reg_slice_ilf_cross_dis = %d\n", p_slice->getLFCrossSliceBoundaryFlag());
    sigdump_output_fprint(p_ctx, "reg_slice_beta_ofs_div2 = %d\n", p_slice->getDeblockingFilterBetaOffsetDiv2());
    sigdump_output_fprint(p_ctx, "reg_slice_tc_ofs_div2 = %d\n", p_slice->getDeblockingFilterTcOffsetDiv2());
    sigdump_output_fprint(p_ctx, "reg_slice_cb_qp_ofs = %d\n", p_slice->getPPS()->getQpOffset(COMPONENT_Cb));
    sigdump_output_fprint(p_ctx, "reg_slice_cr_qp_ofs = %d\n", p_slice->getPPS()->getQpOffset(COMPONENT_Cr));
}
#endif //~SIG_PPU

#ifdef SIG_VBC
void sig_vbc_frm_setting(int cfg_lossy,int cfg_lossy_cr,int cfg_lossy_tolerence,int cfg_lossy_delay)
{
    sigdump_output_fprint(&g_sigpool.vbe_frame_ctx, "# reg_lossy_cr = %d\n",cfg_lossy_cr);
    sigdump_output_fprint(&g_sigpool.vbe_frame_ctx, "# reg_lossy_tolerence = %d\n",cfg_lossy_tolerence);
    sigdump_output_fprint(&g_sigpool.vbe_frame_ctx, "# reg_lossy_delay = %d\n",cfg_lossy_delay);
    sigdump_output_fprint(&g_sigpool.vbe_frame_ctx, "# reg_lossy_truncate = %d\n",(int)(8-8*cfg_lossy_cr/100));
    sigdump_output_fprint(&g_sigpool.vbe_frame_ctx, "# reg_meta_pit = %d\n", g_vbc_meta[0].VBCMetaPitch);
}

#endif //~SIG_VBC

#ifdef SIG_SMT
void sig_smt_isp_src(char *p_src, int width, int height)
{
    if (p_src == nullptr)
        return;

    int width_in_4 = (width >> 2);
    int height_in_4 = (height >> 2);
    int width_in_4_aligned = (cvi_mem_align(width, 256)) >> 2;  // 1 byte is block16x4, 16 bytes aligned

    int b4y = 0, b4x = 0, x = 0;
    unsigned char byte = 0;
    sig_ctx *p_ctx = &g_sigpool.isp_src_ctx;
    char *p_motion_map = p_src;

    for (b4y = 0; b4y < height_in_4; b4y++)
    {
        for (b4x = 0; b4x < width_in_4_aligned; b4x += 4)
        {
            byte = 0;
            for (x = 0; x < 4; x++)
            {
                if ((b4x + x) >= width_in_4)
                    continue;

                byte |= (((*p_motion_map) & 0x03) << (x << 1));
                p_motion_map++;
            }
            sigdump_output_bin(p_ctx, &byte, 1);
        }
    }
}

void sig_smt_isp_set()
{
    sig_ctx *p_ctx = &g_sigpool.p_smt_st->isp_set_ctx;
    sigdump_output_fprint(p_ctx, "reg_trans_qp = %d\n", g_isp_trans_qp);
    sigdump_output_fprint(p_ctx, "reg_motion_qp = %d\n", g_isp_motion_qp);
}

void sig_smt_isp_save_blk_data(int x, int y, int skip, int still, int trans, int motion, int dqp)
{
    int b16_x = (x >> 4);
    int b16_y = (y >> 4);
    sig_smt_dat_st *p_isp_dat = &g_sigpool.p_smt_st->pp_smt_gld_data[b16_y][b16_x];

    p_isp_dat->still_flag = (still == 16) ? 1 : 0;
    p_isp_dat->still_num = still;
    p_isp_dat->trans_num = trans;
    p_isp_dat->motion_num = motion;
    p_isp_dat->isp_dqp = dqp;
    p_isp_dat->skip_flag = 0;
}

void sig_smt_isp_save_blk_skip(int x, int y, int skip)
{
    int b16_x = (x >> 4);
    int b16_y = (y >> 4);
    g_sigpool.p_smt_st->pp_smt_gld_data[b16_y][b16_x].skip_flag = skip;
}

unsigned char CU16_SCAN_POS[16][2] =
{
    {  0,  0 }, { 16,  0 }, {  0, 16 }, { 16, 16 },
    { 32,  0 }, { 48,  0 }, { 32, 16 }, { 48, 16 },
    {  0, 32 }, { 16, 32 }, {  0, 48 }, { 16, 48 },
    { 32, 32 }, { 48, 32 }, { 32, 48 }, { 48, 48 }
};

void sig_smt_isp_gld(int width, int height)
{
    sig_ctx *p_ctx = &g_sigpool.p_smt_st->isp_gld_ctx;

    for (int ctu_y = 0; ctu_y < height; ctu_y += 64)
    {
        for (int ctu_x = 0; ctu_x < width; ctu_x += 64)
        {
            for (int i = 0; i < 16; i++)
            {
                int blk_x = ctu_x + CU16_SCAN_POS[i][0];
                int blk_y = ctu_y + CU16_SCAN_POS[i][1];

                if (blk_x >= width || blk_y >= height)
                    continue;

                int b16_x = (blk_x >> 4);
                int b16_y = (blk_y >> 4);
                sig_smt_dat_st *p_isp_dat = &g_sigpool.p_smt_st->pp_smt_gld_data[b16_y][b16_x];

                if ((blk_x + 16) > width || (blk_y + 16) > height)
                    p_isp_dat->skip_flag = 0;

                sigdump_output_fprint(p_ctx, "(x, y) = (%d, %d)\n", blk_x, blk_y);
                sigdump_output_fprint(p_ctx, "still_flag = %d\n", p_isp_dat->still_flag);
                sigdump_output_fprint(p_ctx, "still_trans_sum = %d\n", (p_isp_dat->still_num + p_isp_dat->trans_num));
                sigdump_output_fprint(p_ctx, "motion = %d\n", p_isp_dat->motion_num);
                sigdump_output_fprint(p_ctx, "dqp = %d\n", p_isp_dat->isp_dqp);
                sigdump_output_fprint(p_ctx, "skip_flag = %d\n", p_isp_dat->skip_flag);
            }
        }
    }
}

void sig_smt_ai_src(AI_ENC_PARAM *p_ai_param)
{
    int width = p_ai_param->src_width;
    int height = p_ai_param->src_height;
    cvi_grid_data<AI_SEG_BLK> *p_seg_map = &p_ai_param->seg_map;
    sig_ctx *p_ctx = &g_sigpool.ai_src_ctx;
    unsigned short data = 0;
    int width_align_pitch = cvi_mem_align(width, 256);  // 128 bytes align

    for (int ctu_y = 0; ctu_y < height; ctu_y += 64)
    {
        for (int ctu_x = 0; ctu_x < width_align_pitch; ctu_x += 64)
        {
            for (int i = 0; i < 16; i++)
            {
                int blk_x = ctu_x + CU16_SCAN_POS[i][0];
                int blk_y = ctu_y + CU16_SCAN_POS[i][1];

                if (blk_x >= width || blk_y >= height)
                {
                    data = 0;
                    sigdump_output_bin(p_ctx, (unsigned char *)(&data), 2);
                    continue;
                }

                // src
                AI_SEG_BLK seg = p_seg_map->get_data(blk_x, blk_y);
                data = ((seg.idx & 0x3f) | ((seg.level & 0x7f) << 8));
                sigdump_output_bin(p_ctx, (unsigned char *)(&data), 2);
            }
        }
    }
}

void sig_smt_ai_src_gld_empty_blk(int blk_x, int blk_y)
{
    sig_ctx *p_ctx = &g_sigpool.ai_src_ctx;
    unsigned short data = 0;
    sigdump_output_bin(p_ctx, (unsigned char *)(&data), 2);

    sig_ctx *p_gld_ctx = &g_sigpool.p_smt_st->ai_gld_ctx;
    sigdump_output_fprint(p_gld_ctx, "(x, y) = (%d, %d)\n", blk_x, blk_y);
    sigdump_output_fprint(p_gld_ctx, "ena = 0\n");
    sigdump_output_fprint(p_gld_ctx, "obj = 0\n");
    sigdump_output_fprint(p_gld_ctx, "dqp_tab_idx = 0\n");
    sigdump_output_fprint(p_gld_ctx, "idx = 0\n");
    sigdump_output_fprint(p_gld_ctx, "bin = 0\n");
    sigdump_output_fprint(p_gld_ctx, "dqp = 0\n");
    sigdump_output_fprint(p_gld_ctx, "sm_dqp = 0\n");
}

void sig_smt_ai_src_gld(AI_ENC_PARAM *p_ai_param)
{
    int width = p_ai_param->src_width;
    int height = p_ai_param->src_height;
    cvi_grid_data<AI_SEG_BLK> *p_seg_map = &p_ai_param->seg_map;
    sig_ctx *p_ctx = &g_sigpool.ai_src_ctx;
    sig_ctx *p_gld_ctx = &g_sigpool.p_smt_st->ai_gld_ctx;
    unsigned short data = 0;
    int width_align_pitch = cvi_mem_align(width, 256);  // 128 bytes align
    int skip = 0;
    int sm_dqp = 0;

    for (int ctu_y = 0; ctu_y < height; ctu_y += 64)
    {
        for (int ctu_x = 0; ctu_x < width_align_pitch; ctu_x += 64)
        {
            for (int i = 0; i < 16; i++)
            {
                int blk_x = ctu_x + CU16_SCAN_POS[i][0];
                int blk_y = ctu_y + CU16_SCAN_POS[i][1];

                if (blk_x >= width || blk_y >= height)
                {
                    sig_smt_ai_src_gld_empty_blk(blk_x, blk_y);
                    continue;
                }

                // src
                AI_SEG_BLK seg = p_seg_map->get_data(blk_x, blk_y);
                data = ((seg.idx & 0x3f) | ((seg.level & 0x7f) << 8));
                sigdump_output_bin(p_ctx, (unsigned char *)(&data), 2);

                // golden
                sig_smt_dat_st *p_smt_dat = &g_sigpool.p_smt_st->pp_smt_gld_data[blk_y >> 4][blk_x >> 4];
                sigdump_output_fprint(p_gld_ctx, "(x, y) = (%d, %d)\n", blk_x, blk_y);
                sigdump_output_fprint(p_gld_ctx, "ena = %d\n", p_smt_dat->ena);
                sigdump_output_fprint(p_gld_ctx, "obj = %d\n", p_smt_dat->obj);
                sigdump_output_fprint(p_gld_ctx, "dqp_tab_idx = %d\n", p_smt_dat->dqp_tab_idx);
                sigdump_output_fprint(p_gld_ctx, "idx = %d\n", p_smt_dat->idx);
                sigdump_output_fprint(p_gld_ctx, "bin = %d\n", p_smt_dat->bin);
                sigdump_output_fprint(p_gld_ctx, "dqp = %d\n", p_smt_dat->ai_dqp);

                getQpMapBlk16(blk_x, blk_y, &skip, NULL, &sm_dqp);
                sigdump_output_fprint(p_gld_ctx, "sm_dqp = %d\n", sm_dqp);
            }
        }
    }
}

void sig_smt_ai_set(AI_ENC_PARAM *p_ai_param)
{
    if (p_ai_param == nullptr)
        return;

    sig_ctx *p_ctx = &g_sigpool.p_smt_st->ai_set_ctx;
    sigdump_output_fprint(p_ctx, "reg_conf_scale = %d\n", g_ai_conf_scale);
    sigdump_output_fprint(p_ctx, "reg_tclip_min = %d\n", g_ai_table_idx_min);
    sigdump_output_fprint(p_ctx, "reg_tclip_max = %d\n", g_ai_table_idx_max);

    for (int i = 0; i < AI_SI_TAB_COUNT; i++)
    {
        sigdump_output_fprint(p_ctx, "dqp_table_%d = ", i);
        for (int j = 0; j < AI_SI_TAB_BIN - 1; j++)
        {
            sigdump_output_fprint(p_ctx, "0x%02x, ", g_smart_enc_ai_table[i][j]);
        }
        sigdump_output_fprint(p_ctx, "0x%02x\n", g_smart_enc_ai_table[i][AI_SI_TAB_BIN - 1]);
    }

    int map_table_size = p_ai_param->mapping_table.size();
    for (int i = 0; i < map_table_size; i++)
    {
        char idx = p_ai_param->mapping_table[i].table_idx;
        if (idx >= 0)
            idx = ((idx & 0x7) << 1) | 1;
        else
            idx = 0;
        sigdump_output_fprint(p_ctx, "obj_tab_%d = 0x%x\n", i, idx);
    }
    for (int i = map_table_size; i < 64; i++)
    {
        sigdump_output_fprint(p_ctx, "obj_tab_%d = 0x0\n", i);
    }
}

void sig_smt_rand_set(AI_ENC_PARAM *p_ai_param)
{
    if (p_ai_param == nullptr)
        return;

    for (int i = 0; i < AI_SI_TAB_COUNT; i++)
    {
        for (int j = 0; j < AI_SI_TAB_BIN; j++)
            g_smart_enc_ai_table[i][j] = cvi_random_range(-51, 51);
    }

    int map_table_size = p_ai_param->mapping_table.size();
    for (int i = 0; i < map_table_size; i++)
    {
        p_ai_param->mapping_table[i].idx = i;
        p_ai_param->mapping_table[i].table_idx = cvi_random_range(0, 7);
    }

    g_ai_conf_scale = cvi_random_range(0, 63);
    g_ai_table_idx_min = cvi_random_range(0, 15);
    g_ai_table_idx_max = cvi_random_range(0, 15);
    if (g_ai_table_idx_min > g_ai_table_idx_max)
        std::swap(g_ai_table_idx_min, g_ai_table_idx_max);

    g_isp_motion_qp = cvi_random_range(-51, 51);
    g_isp_trans_qp = cvi_random_range(-51, 51);

    g_clip_delta_qp_min = cvi_random_range(-51, 51);
    g_clip_delta_qp_max = cvi_random_range(-51, 51);
    if (g_clip_delta_qp_min > g_clip_delta_qp_max)
        std::swap(g_clip_delta_qp_min, g_clip_delta_qp_max);
}

void sig_smt_isp_ai_set(int width, int height, bool is_ai_en, bool is_isp_en)
{
    sig_ctx *p_ctx = &g_sigpool.p_smt_st->isp_ai_set_ctx;
    sigdump_output_fprint(p_ctx, "pic_w = %d\n", width);
    sigdump_output_fprint(p_ctx, "pic_h = %d\n", height);
    sigdump_output_fprint(p_ctx, "reg_avc_mode = 0\n");
    sigdump_output_fprint(p_ctx, "reg_isp_map_en = %d\n", (is_isp_en ? 1 : 0));
    sigdump_output_fprint(p_ctx, "reg_ai_map_en = %d\n", (is_ai_en ? 1 : 0));
    sigdump_output_fprint(p_ctx, "reg_ai_smooth_qp_en = %d\n", g_algo_cfg.AISmoothMap ? 1 : 0);
    sigdump_output_fprint(p_ctx, "reg_skip_map_en = %d\n", g_algo_cfg.EnableSmartEncISPSkipMap ? 1 : 0);
    sigdump_output_fprint(p_ctx, "reg_clip_delta_qp_min = %d\n", g_clip_delta_qp_min);
    sigdump_output_fprint(p_ctx, "reg_clip_delta_qp_max = %d\n", g_clip_delta_qp_max);

    sigdump_output_fprint(p_ctx, "reg_ai_dqp_base = %lu\n", get_hw_base("ai_dqp"));
    sigdump_output_fprint(p_ctx, "reg_ai_dqp_pit = %u\n", get_hw_pitch("ai_dqp"));
    sigdump_output_fprint(p_ctx, "reg_isp_dqp_base = %lu\n", get_hw_base("isp_dqp"));
    sigdump_output_fprint(p_ctx, "reg_isp_dqp_pit = %u\n", get_hw_pitch("isp_dqp"));
}

void sig_smt_isp_ai_gld(int width, int height)
{
    sig_ctx *p_ctx = &g_sigpool.p_smt_st->isp_ai_gld_ctx;
    int width_align_pitch = cvi_mem_align(width, 256);  // 128 bytes align
    int skip = 0;
    int delta_qp = 0;

    for (int ctu_y = 0; ctu_y < height; ctu_y += 64)
    {
        for (int ctu_x = 0; ctu_x < width_align_pitch; ctu_x += 64)
        {
            for (int i = 0; i < 16; i++)
            {
                int blk_x = ctu_x + CU16_SCAN_POS[i][0];
                int blk_y = ctu_y + CU16_SCAN_POS[i][1];
                getQpMapBlk16(blk_x, blk_y, &skip, NULL, &delta_qp);

                if (blk_x >= width || blk_y >= height)
                {
                    skip = 0;
                    delta_qp = Clip3(g_clip_delta_qp_min, g_clip_delta_qp_max, 0);
                }

                sigdump_output_fprint(p_ctx, "(x, y) = (%d, %d)\n", blk_x, blk_y);
                sigdump_output_fprint(p_ctx, "skip_flag = %d\n", skip);
                sigdump_output_fprint(p_ctx, "dqp = %d\n", delta_qp);
            }
        }
    }
}
#endif //~SIG_SMT