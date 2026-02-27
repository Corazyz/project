
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <cstdlib>
#include "math.h"
#include "TComPicYuv.h"
#include "TComBitStream.h"
#include "cvi_vbc.h"
#include "cvi_sigdump.h"

// #define VBC_32X16

#define MAX_SYMBOL 511
#define MAX_CODE_LEN 24
#define MAX_GP1_STR0_LEN 138
#define MAX_GP1_STR1_LEN 272

#define AVG_PREDICTOR 128

#define CTX_NUM 365
#define CTX_THR0 1
#define CTX_THR1 3  // BASIC_T1
#define CTX_THR2 7  // BASIC_T2
#define CTX_THR3 21 // BASIC_T3
#define MAX_L_VAL 6 // This is an implementation limit (theoretical limit is 32)

#define DPCM_MODE 1
#define PCM_MODE 8
#define CTX_MODE 9
#define FIXLEN_MODE 0

int cu_lossy_enable, lossy_truncate, lossy_cr, lossy_delay, lossy_tolerence, lossy, cu_total_cnt, acc_bits, target_bits;

enum CtxType
{
    CTX_1D = 0,
    CTX_2D = 1,
};

/* stream 0
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
  0,  0   1   3   4   5   6   7   8   9  10  11  12  13  14  15  16
  1,  2
  2, 17  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33
  3  18
*/
/* stream 1
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
  0,
  1,  X   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14
  2,
  3   X  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29
*/
const int stream0_order_luma[] =
    {
        0, 1, 16, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        32, 48, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
        45, 46, 47};

const int stream0_order_chroma[] =
    {
        0, 1, 2, 3, 16, 17, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        32, 33, 48, 49, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};

const int stream1_order_luma[] =
    {
        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

const int stream1_order_chroma[] =
    {
        18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

const int stream0_order_half[] =
    {
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};
const int stream1_order_half[] =
    {
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

unsigned char *bsMetaOutPack;
int MetaOutPack_size;

int g_VBCVersion;
meta_st g_vbc_meta[2];
meta_st g_vbd_meta[2];
int8_t *m_ctx_quant_lut;
int8_t m_quantization_lut[512];
bool truncate_bit = true;
bool bsPack = true;
int PitchAlignSize = 256;
int VBCMetaPitch;

int get_vbc_meta_pitch_align_size()
{
    return PitchAlignSize;
}

class TComVBCOutputBitstream : public TComOutputBitstream
{

public:
    void writeVLC64(uint64_t uiBits, unsigned int uiNumberOfBits);
    void write64(uint64_t uiBits, unsigned int uiNumberOfBits);
    void writeVLC(unsigned int uiBits, unsigned int uiNumberOfBits);
    const std::vector<uint8_t> &getFifo() const { return m_fifo; }
    std::vector<uint8_t> &getFifo() { return m_fifo; }
};

void TComVBCOutputBitstream::writeVLC64(uint64_t uiBits, unsigned int uiNumberOfBits)
{
    unsigned int MSB = (unsigned int)((uiBits & 0xFFFFFFFF00000000LL) >> 32);
    unsigned int LSB = (unsigned int)(uiBits & 0xFFFFFFFFLL);
    writeVLC(LSB, uiNumberOfBits >= 32 ? 32 : uiNumberOfBits);
    if (uiNumberOfBits > 32)
        writeVLC(MSB, uiNumberOfBits - 32);
}

void TComVBCOutputBitstream::write64(uint64_t uiBits, unsigned int uiNumberOfBits)
{
    unsigned int MSB = (unsigned int)((uiBits & 0xFFFFFFFF00000000LL) >> 32);
    unsigned int LSB = (unsigned int)(uiBits & 0xFFFFFFFFLL);
    write(LSB, uiNumberOfBits >= 32 ? 32 : uiNumberOfBits);
    if (uiNumberOfBits > 32)
        write(MSB, uiNumberOfBits - 32);
}

void TComVBCOutputBitstream::writeVLC(unsigned int uiBits, unsigned int uiNumberOfBits)
{
    for (int bit = 0; bit < uiNumberOfBits; bit++)
        write((uiBits >> bit) & 1, 1);
}

TComVBCOutputBitstream bsOut[2];
TComVBCOutputBitstream bsOut1[2];
TComVBCOutputBitstream bsOut2[2];
TComVBCOutputBitstream bsmetacpx[2];
TComVBCOutputBitstream bsmetacpx_reorder[2];

uint8_t quantize_gradient_org(int di)
{
    if (di <= -CTX_THR3)
        return -4;
    if (di <= -CTX_THR2)
        return -3;
    if (di <= -CTX_THR1)
        return -2;
    if (di < -CTX_THR0)
        return -1;
    if (di <= CTX_THR0)
        return 0;
    if (di < CTX_THR1)
        return 1;
    if (di < CTX_THR2)
        return 2;
    if (di < CTX_THR3)
        return 3;
    return 4;
}

uint8_t byteInv(uint8_t byte)
{
    uint8_t out = 0;
    for (int i = 7; i >= 0; i--)
    {
        out |= ((byte & 1) << i);
        byte >>= 1;
    }
    return out;
}

void vbc_contexts_init()
{
    // context selection LUT
    int range = 1 << 8;
    for (int idx = 0; idx < (range * 2); idx++)
    {
        m_quantization_lut[idx] = quantize_gradient_org(-range + idx);
    }
    m_ctx_quant_lut = &m_quantization_lut[256];
}

void vbc_meta_init(int vbc_version, meta_st *p_in_meta, int pic_width, int pic_height, int vbc_shift)
{
    int ChType;
    bool is_vbc = p_in_meta == g_vbc_meta;
    g_VBCVersion = vbc_version;
    for (ChType = 0; ChType <= 1; ChType++)
    {
        meta_st *pvbc_meta = &p_in_meta[ChType];
        pvbc_meta->pic_width = (pic_width + 31) / 32 * 32;
        pvbc_meta->pic_height = ChType == 0 ? pic_height : pic_height / 2;
        if (is_vbc)
        {
            pvbc_meta->ycinterleave = 0;
            pvbc_meta->shift_pix = vbc_shift;
            if (ChType == 0)
            {
                if (g_VBCVersion >= 1)
                {
                    pvbc_meta->tile_w = 64;
                    pvbc_meta->tile_h = 64;
                    pvbc_meta->tile_sub_w = 32;
                    pvbc_meta->tile_sub_h = 32;
                }
                else
                {
                    pvbc_meta->tile_w = 64;
                    pvbc_meta->tile_h = 16;
                    pvbc_meta->tile_sub_w = 32;
                    pvbc_meta->tile_sub_h = 16;
                }
            }
            else
            {
                if (g_VBCVersion >= 1)
                {
                    pvbc_meta->tile_w = 64;
                    pvbc_meta->tile_h = 32;
                    pvbc_meta->tile_sub_w = 32;
                    pvbc_meta->tile_sub_h = 16;
                }
                else
                {
                    pvbc_meta->tile_w = 64;
                    pvbc_meta->tile_h = 8;
                    pvbc_meta->tile_sub_w = 32;
                    pvbc_meta->tile_sub_h = 8;
                }
            }
        }
        else
        {
            pvbc_meta->ycinterleave = 1;
            pvbc_meta->shift_pix = vbc_shift;
            pvbc_meta->tile_w = 64;
            pvbc_meta->tile_h = 4;
            pvbc_meta->tile_sub_w = 32;
            pvbc_meta->tile_sub_h = 4;
        }
        pvbc_meta->cu_w = 16;
        pvbc_meta->cu_h = 4;
        pvbc_meta->width = cvi_mem_align(pic_width, pvbc_meta->tile_w);
        if (g_VBCVersion >= 1)
            pvbc_meta->height = cvi_mem_align(ChType == 0 ? pic_height : pic_height / 2, pvbc_meta->tile_h / 4);
        else
            pvbc_meta->height = cvi_mem_align(ChType == 0 ? pic_height : pic_height / 2, pvbc_meta->tile_h);
        pvbc_meta->tile_cnt_x = cvi_mem_align_mult(pvbc_meta->width, pvbc_meta->tile_w);
        pvbc_meta->tile_cnt_y = cvi_mem_align_mult(pvbc_meta->height + pvbc_meta->shift_pix, pvbc_meta->tile_h);
        pvbc_meta->tile_sub_cnt_x = pvbc_meta->tile_w / pvbc_meta->tile_sub_w;
        pvbc_meta->tile_sub_cnt_y = pvbc_meta->tile_h / pvbc_meta->tile_sub_h;
        if (g_VBCVersion >= 1)
        {
            pvbc_meta->sub_tile_size = 32 + 6 * (pvbc_meta->tile_sub_w / pvbc_meta->cu_w) * (pvbc_meta->tile_sub_h / pvbc_meta->cu_h);
            pvbc_meta->tile_size = cvi_mem_align_mult(pvbc_meta->sub_tile_size * pvbc_meta->tile_sub_cnt_x * pvbc_meta->tile_sub_cnt_y, 8);
        }
        else
        {
            pvbc_meta->sub_tile_size = 6 * (pvbc_meta->tile_sub_w / pvbc_meta->cu_w) * (pvbc_meta->tile_sub_h / pvbc_meta->cu_h);
            pvbc_meta->tile_size = cvi_mem_align_mult(32 + pvbc_meta->sub_tile_size * pvbc_meta->tile_sub_cnt_x * pvbc_meta->tile_sub_cnt_y, 8);
        }
        pvbc_meta->meta_size = pvbc_meta->tile_size * pvbc_meta->tile_cnt_x * pvbc_meta->tile_cnt_y;
        pvbc_meta->tile_arry = (tile_meta_st *)malloc(pvbc_meta->tile_cnt_x * pvbc_meta->tile_cnt_y * sizeof(tile_meta_st));

        int tile_idx;
        for (tile_idx = 0; tile_idx < (pvbc_meta->tile_cnt_x * pvbc_meta->tile_cnt_y); tile_idx++)
        {
            pvbc_meta->tile_arry[tile_idx].sub_tile_arry = (sub_tile_meta_st *)malloc(pvbc_meta->tile_sub_cnt_x * pvbc_meta->tile_sub_cnt_y * sizeof(sub_tile_meta_st));
            int min_cu_cnt = pvbc_meta->tile_sub_w * pvbc_meta->tile_sub_h / (pvbc_meta->cu_w * pvbc_meta->cu_h);
            int sub_tile_idx;
            for (sub_tile_idx = 0; sub_tile_idx < (pvbc_meta->tile_sub_cnt_x * pvbc_meta->tile_sub_cnt_y); sub_tile_idx++)
            {
                pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry = (unsigned char *)malloc(min_cu_cnt);
            }
        }
    }

    int pitch = cvi_mem_align((p_in_meta[0].tile_size + p_in_meta[1].tile_size) * p_in_meta[0].tile_cnt_x, get_vbc_meta_pitch_align_size());
    p_in_meta[0].VBCMetaPitch = p_in_meta[1].VBCMetaPitch = pitch;
}

void vbc_meta_deinit(meta_st *p_in_meta)
{
    int ChType;
    for (ChType = 0; ChType <= 1; ChType++)
    {
        meta_st *pvbc_meta = &p_in_meta[ChType];
        int tile_idx;
        for (tile_idx = 0; tile_idx < (pvbc_meta->tile_cnt_x * pvbc_meta->tile_cnt_y); tile_idx++)
        {
            int sub_tile_idx;
            for (sub_tile_idx = 0; sub_tile_idx < (pvbc_meta->tile_sub_cnt_x * pvbc_meta->tile_sub_cnt_y); sub_tile_idx++)
            {
                free(pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry);
                pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry = NULL;
            }
            free(pvbc_meta->tile_arry[tile_idx].sub_tile_arry);
            pvbc_meta->tile_arry[tile_idx].sub_tile_arry = NULL;
        }

        free(pvbc_meta->tile_arry);
        pvbc_meta->tile_arry = NULL;
    }
}

void get_vbc_meta_info(int chType, int x, int y, unsigned int *address, unsigned int *data_length)
{
    meta_st *pvbc_meta = &g_vbc_meta[chType];
    int tile_x = x / pvbc_meta->tile_w;
    int tile_y = y / pvbc_meta->tile_h;
    int tile_idx = tile_x + tile_y * pvbc_meta->tile_cnt_x;
    int tile_sub_x = (x % pvbc_meta->tile_w) / pvbc_meta->tile_sub_w;
    int tile_sub_y = (y % pvbc_meta->tile_h) / pvbc_meta->tile_sub_h;
    int sub_tile_idx = tile_sub_x + tile_sub_y * pvbc_meta->tile_sub_cnt_x;

    int cu_x = (x % pvbc_meta->tile_sub_w) / pvbc_meta->cu_w;
    int cu_y = (y % pvbc_meta->tile_sub_h) / pvbc_meta->cu_h;
    int cu_idx = cu_x + cu_y * (pvbc_meta->tile_sub_w / pvbc_meta->cu_w);

    *address = pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].address;
    int i;
    unsigned char *cu_array = pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry;
    for (i = 0; i < cu_idx; i++)
    {
        *address += (cu_array[i] + 1);
    }
    *data_length = (cu_array[cu_idx] + cu_array[cu_idx + 1] + 2);
}

void get_vbc_meta_info_fake(int chType, int x, int y, unsigned int *address, unsigned int *data_length)
{
    meta_st *pvbc_meta = &g_vbc_meta[chType];
    int tile_x = x / pvbc_meta->tile_w;
    int tile_y = y / pvbc_meta->tile_h;
    int tile_idx = tile_x + tile_y * pvbc_meta->tile_cnt_x;
    int tile_sub_x = (x % pvbc_meta->tile_w) / pvbc_meta->tile_sub_w;
    int tile_sub_y = (y % pvbc_meta->tile_h) / pvbc_meta->tile_sub_h;
    int sub_tile_idx = tile_sub_x + tile_sub_y * pvbc_meta->tile_sub_cnt_x;

    int cu_y = (y % pvbc_meta->tile_sub_h) / pvbc_meta->cu_h;
    int cu_idx = cu_y * (pvbc_meta->tile_sub_w / pvbc_meta->cu_w);
    float tile_sz = (float)((pvbc_meta->total_compress_size) / (float)(pvbc_meta->pic_width * pvbc_meta->pic_height)) * (pvbc_meta->tile_w * pvbc_meta->tile_h);
    float sub_tile_sz = tile_sz / ((pvbc_meta->tile_w * pvbc_meta->tile_h) / (pvbc_meta->tile_sub_w * pvbc_meta->tile_sub_h));
    float cu_sz = tile_sz / ((pvbc_meta->tile_w * pvbc_meta->tile_h) / (pvbc_meta->cu_w * pvbc_meta->cu_h));
    float offset = tile_sz * tile_idx + sub_tile_idx * sub_tile_sz;
    int i;
    for (i = 0; i < cu_idx; i+=2)
    {
        offset += (2 * cu_sz);
    }
    *address = (unsigned int)offset;
    *data_length = (unsigned int)(cu_sz + cu_sz);
}

void get_vbc_meta_info_fake_reorder(int chType, int x, int y, unsigned int *address, unsigned int *data_length)
{
    meta_st *pvbc_meta = &g_vbc_meta[chType];
    if (x < pvbc_meta->tile_sub_w)
    {
        int tile_x = 0;
        int tile_y = y / pvbc_meta->tile_h;
        int tile_idx = tile_x + tile_y * pvbc_meta->tile_cnt_x;
        int tile_sub_y = (y % pvbc_meta->tile_h) / pvbc_meta->tile_sub_h;
        int sub_tile_idx = tile_sub_y * pvbc_meta->tile_sub_cnt_x;
        float tile_sz = (float)((pvbc_meta->total_compress_size) / (float)(pvbc_meta->pic_width * pvbc_meta->pic_height)) * (pvbc_meta->tile_w * pvbc_meta->tile_h);
        float sub_tile_sz = tile_sz / ((pvbc_meta->tile_w * pvbc_meta->tile_h) / (pvbc_meta->tile_sub_w * pvbc_meta->tile_sub_h));
        float cu_sz = tile_sz / ((pvbc_meta->tile_w * pvbc_meta->tile_h) / (pvbc_meta->cu_w * pvbc_meta->cu_h));

        float offset = tile_sz * tile_idx + sub_tile_idx * sub_tile_sz;

        int cu_y = (y % pvbc_meta->tile_sub_h) / pvbc_meta->cu_h;
        int cu_idx = cu_y * (pvbc_meta->tile_sub_w / pvbc_meta->cu_w);
        int i;
        for (i = 0; i < cu_idx; i+=2)
        {
            offset += (cu_sz * 2);
        }
        *address = (unsigned int)offset;
        *data_length = (unsigned int)(cu_sz + cu_sz);
    }
    else
    {
        float tile_sz = (float)((pvbc_meta->total_compress_size) / (float)(pvbc_meta->pic_width * pvbc_meta->pic_height)) * (pvbc_meta->tile_w * pvbc_meta->tile_h);
        float sub_tile_sz = tile_sz / ((pvbc_meta->tile_w * pvbc_meta->tile_h) / (pvbc_meta->tile_sub_w * pvbc_meta->tile_sub_h));
        float cu_sz = tile_sz / ((pvbc_meta->tile_w * pvbc_meta->tile_h) / (pvbc_meta->cu_w * pvbc_meta->cu_h));
        int tile_sub_x = (x % pvbc_meta->tile_w) / pvbc_meta->tile_sub_w;
        int tile_sub_y = (y % pvbc_meta->tile_h) / pvbc_meta->tile_sub_h;

        int tile_y = y / pvbc_meta->tile_h;
        int tile_idx = tile_y * pvbc_meta->tile_cnt_x;
        float offset = tile_sz * tile_idx + 2 * sub_tile_sz;

        if (tile_sub_y == 0)
        {
            int tile_x = (x - 32) / pvbc_meta->tile_w;
            offset += tile_x * tile_sz;
            if (tile_sub_x == 0)
                offset += sub_tile_sz;
        }
        else
        {
            int tile_x = (x - 32) / pvbc_meta->tile_w;
            offset += tile_x * tile_sz;
            offset += 2 * sub_tile_sz;
            if (tile_sub_x == 0)
                offset += sub_tile_sz;
        }
        int cu_y = (y % pvbc_meta->tile_sub_h) / pvbc_meta->cu_h;
        int cu_idx = cu_y * (pvbc_meta->tile_sub_w / pvbc_meta->cu_w);
        int i;
        for (i = 0; i < cu_idx; i+=2)
        {
            offset += (cu_sz * 2);
        }
        *address = (unsigned int)offset;
        *data_length = (unsigned int)(cu_sz + cu_sz);

    }
}


void get_vbc_1d_meta_buffer(int chType, int x, int y, unsigned int *meta_address, unsigned int *meta_length)
{
    if (g_VBCVersion >= 1)
    {
        if (chType == 0)
        {
            int tile_x = x / g_vbc_meta[chType].tile_w;
            int tile_y = y / g_vbc_meta[chType].tile_h;

            int tile_sub_x = (x % g_vbc_meta[chType].tile_w) / g_vbc_meta[chType].tile_sub_w;
            int tile_sub_y = (y % g_vbc_meta[chType].tile_h) / g_vbc_meta[chType].tile_sub_h;

            unsigned int tile_offset = (tile_y * g_vbc_meta[chType].tile_cnt_x + tile_x) * (g_vbc_meta[0].tile_size + g_vbc_meta[1].tile_size);
            unsigned int tile_sub_offset = (tile_sub_y * (g_vbc_meta[chType].tile_w / g_vbc_meta[chType].tile_sub_w) + tile_sub_x) * (g_vbc_meta[0].sub_tile_size + g_vbc_meta[1].sub_tile_size) / 8;

            *meta_address = tile_offset + tile_sub_offset;
            *meta_length = (g_vbc_meta[chType].sub_tile_size / 8);
        }
        else
        {
            y *= 2; //map to luma index
            int tile_x = x / g_vbc_meta[0].tile_w;
            int tile_y = y / g_vbc_meta[0].tile_h;

            int tile_sub_x = (x % g_vbc_meta[0].tile_w) / g_vbc_meta[0].tile_sub_w;
            int tile_sub_y = (y % g_vbc_meta[0].tile_h) / g_vbc_meta[0].tile_sub_h;

            unsigned int tile_offset = (tile_y * g_vbc_meta[0].tile_cnt_x + tile_x) * (g_vbc_meta[0].tile_size + g_vbc_meta[1].tile_size);
            unsigned int tile_sub_offset = (tile_sub_y * (g_vbc_meta[0].tile_w / g_vbc_meta[0].tile_sub_w) + tile_sub_x) * (g_vbc_meta[0].sub_tile_size + g_vbc_meta[1].sub_tile_size) / 8;

            tile_sub_offset += (g_vbc_meta[0].sub_tile_size / 8);
            *meta_address = tile_offset + tile_sub_offset;
            *meta_length = (g_vbc_meta[chType].sub_tile_size / 8);
        }
    }
    else
    {
        if (chType == 0)
        {
            int tile_x = x / g_vbc_meta[chType].tile_w;
            int tile_y = y / g_vbc_meta[chType].tile_h;
            unsigned int tile_offset = (tile_y * g_vbc_meta[chType].tile_cnt_x + tile_x) * (g_vbc_meta[0].tile_size + g_vbc_meta[1].tile_size);

            *meta_address = tile_offset;
            *meta_length = g_vbc_meta[chType].tile_size;
        }
        else
        {
            y *= 2; //map to luma index
            int tile_x = x / g_vbc_meta[0].tile_w;
            int tile_y = y / g_vbc_meta[0].tile_h;
            unsigned int tile_offset = (tile_y * g_vbc_meta[0].tile_cnt_x + tile_x) * (g_vbc_meta[0].tile_size + g_vbc_meta[1].tile_size);

            *meta_address = tile_offset;
            *meta_length = g_vbc_meta[chType].tile_size;
        }
    }
}

void writeOutPackBs(TComVBCOutputBitstream *bsPackOut, encCU_st *pCU, int mode_bit_sz)
{
    const int *stream0_order, *stream1_order;
    unsigned int stream0_cnt, stream1_cnt;

    if (g_VBCVersion >= 1)
    {
        stream0_order = stream0_order_half;
        stream0_cnt = sizeof(stream0_order_half) / sizeof(int);
        stream1_order = stream1_order_half;
        stream1_cnt = sizeof(stream1_order_half) / sizeof(int);
    }
    else
    {
        stream0_order = stream0_order_luma;
        stream0_cnt = sizeof(stream0_order_luma) / sizeof(int);
        stream1_order = stream1_order_luma;
        stream1_cnt = sizeof(stream1_order_luma) / sizeof(int);
    }

    int stream0_bit_sz = mode_bit_sz, stream1_bit_sz = 0;
    int s0_start = 0; // pCU->isMirrorCU ? (1 + pCU->chType) : 0;

    for (int i = s0_start; i < stream0_cnt; i++)
    {
        bsPackOut->writeVLC64(pCU->codeCU[stream0_order[i]], pCU->codeCULength[stream0_order[i]]);
        stream0_bit_sz += pCU->codeCULength[stream0_order[i]];
    }

    TComVBCOutputBitstream bsStream1Out;
    bsStream1Out.clear();
    // for (int i = (stream1_cnt - 1); i >= 0; i--)
    for (int i = 0; i < stream1_cnt; i++)
    {
        bsStream1Out.writeVLC64(pCU->codeCU[stream1_order[i]], pCU->codeCULength[stream1_order[i]]);
        stream1_bit_sz += pCU->codeCULength[stream1_order[i]];
    }
    uint8_t align_byte = 0;

    if ((stream1_bit_sz & 0x7) != 0)
    {
        bsStream1Out.writeAlignZero();
        align_byte = byteInv(bsStream1Out.getFifo().back());
        if (((stream1_bit_sz & 0x7) + (stream0_bit_sz & 0x7)) > 8)
        {
            bsPackOut->writeAlignZero();
            bsPackOut->write(align_byte, 8);
        }
        else
        {
            bsPackOut->write(align_byte, 8 - (stream0_bit_sz & 0x7));
        }
        bsStream1Out.getFifo().pop_back();
    }
    else
    {
        bsPackOut->writeAlignZero();
    }

    for (auto it = bsStream1Out.getFifo().rbegin(); it != bsStream1Out.getFifo().rend(); it++)
    {
        bsPackOut->write(byteInv(*it), 8);
    }
    if (s0_start != 0)
    {
        for (int i = 0; i < s0_start; i++)
        {
            bsPackOut->writeVLC64(pCU->codeCU[stream0_order[i]], pCU->codeCULength[stream0_order[i]]);
        }
    }
    assert(bsPackOut->getNumBitsUntilByteAligned() == 0);
}

void writeOutPackBs_onestream(TComVBCOutputBitstream *bsPackOut, TComVBCOutputBitstream *bsPackOut1, encCU_st *pCU, int mode_bit_sz, int *str0_sz, int *str1_sz)
{
    const int *stream0_order, *stream1_order;
    unsigned int stream0_cnt, stream1_cnt;
    if (g_VBCVersion >= 1)
    {
        stream0_order = stream0_order_half;
        stream0_cnt = sizeof(stream0_order_half) / sizeof(int);
        stream1_order = stream1_order_half;
        stream1_cnt = sizeof(stream1_order_half) / sizeof(int);
    }
    else
    {
        stream0_order = stream0_order_luma;
        stream0_cnt = sizeof(stream0_order_luma) / sizeof(int);
        stream1_order = stream1_order_luma;
        stream1_cnt = sizeof(stream1_order_luma) / sizeof(int);
    }

    int stream0_bit_sz = mode_bit_sz, stream1_bit_sz = 0;
    int s0_start = 0; // pCU->isMirrorCU ? (1 + pCU->chType) : 0;
    for (int i = s0_start; i < stream0_cnt; i++)
    {
        bsPackOut->writeVLC64(pCU->codeCU[stream0_order[i]], pCU->codeCULength[stream0_order[i]]);
        stream0_bit_sz += pCU->codeCULength[stream0_order[i]];
    }
    if ((stream0_bit_sz & 0x7) != 0)
        bsPackOut->writeAlignZero();
    for (int i = 0; i < stream1_cnt; i++)
    {
        bsPackOut1->writeVLC64(pCU->codeCU[stream1_order[i]], pCU->codeCULength[stream1_order[i]]);
        stream1_bit_sz += pCU->codeCULength[stream1_order[i]];
    }
    if ((stream1_bit_sz & 0x7) != 0)
        bsPackOut1->writeAlignZero();
    *str0_sz = stream0_bit_sz;
    *str1_sz = stream1_bit_sz;
}

bool pix_in_group(int x, int y, int group)
{
    if (group == 0)
    {
        return (x == 0 || y == 0);
    }
    else if (group == 1)
    {
        return !(x == 0 || y == 0);
    }
    else
        return true;
}

int get_group_size(encCU_st *encCU, int group)
{
    int gp0_sz = encCU->cu_w + encCU->cu_h - 1;

    if (group == 0)
    {
        return gp0_sz;
    }
    else
    {
        return (encCU->cu_w * encCU->cu_h - gp0_sz);
    }
}

int sign_to_unsign(int val)
{
    bool sign_i = val < 0;
    int abs_data_i = abs(val);
    return ((abs_data_i << 1) - sign_i);
}

int get_gr_max_symbol(int order_k, int max_code_len)
{
    int unary_q_bit = max_code_len - order_k;
    int max_remain = (1 << order_k) - 1;
    int unary_q = (unary_q_bit - 1) << order_k;

    int symbol = unary_q + max_remain;
    if (symbol > MAX_SYMBOL)
        return MAX_SYMBOL;
    return symbol;
}

int get_gr_max_unary_q_bit(int order_k, int max_code_len)
{
    int unary_q_bit = max_code_len - order_k;
    int max_group_i = MAX_SYMBOL >> order_k;
    if (unary_q_bit > max_group_i)
        return max_group_i + 1;
    return unary_q_bit;
}

uint64_t get_gr_code_word(int symbol, int order_k)
{
    uint64_t code = 0;
    if (order_k > 0)
    {
        uint8_t bit_mask = (1 << order_k) - 1;
        code = symbol & bit_mask;
    }
    int group_i = symbol >> order_k;
    int max_group_i = MAX_SYMBOL >> order_k;
    uint64_t unary_q = ((uint64_t)(group_i < max_group_i) << (group_i));
    code |= (unary_q << order_k);
    return code;
}

uint64_t get_gr_code_word_pack(int symbol, int order_k, int *len)
{
    uint64_t code = 0;
    if (order_k > 0)
    {
        int tmp_symbol = symbol;
        for (int i = 0; i < order_k; i++)
        {
            code |= tmp_symbol & 1;
            if ((i + 1) < order_k)
            {
                code <<= 1;
                tmp_symbol >>= 1;
            }
        }
    }
    int group_i = symbol >> order_k;
    int max_group_i = MAX_SYMBOL >> order_k;

    if (truncate_bit && ((group_i + 1) >= get_gr_max_unary_q_bit(order_k, MAX_CODE_LEN)))
    {
        int shift_code = get_gr_max_unary_q_bit(order_k, MAX_CODE_LEN) - 1;
        code = (code << shift_code);
        if (len)
            *len = (shift_code + order_k);
    }
    else
    {
        uint64_t unary_q = ((uint64_t)(group_i < max_group_i) << (group_i));
        if (group_i < max_group_i)
        {
            code = (code << (group_i + 1)) | unary_q;
            if (len)
                *len = (group_i + 1) + order_k;
        }
        else
        {
            code = (code << (max_group_i)) | unary_q;
            if (len)
                *len = max_group_i + order_k;
        }
    }
    return code;
}

int get_gr_bit_count(int symbol, int order_k)
{
    if (truncate_bit)
    {
        if (symbol > get_gr_max_symbol(order_k, MAX_CODE_LEN))
        {
            return (MAX_CODE_LEN + 1);
        }
        else
        {
            int len = 0;
            get_gr_code_word_pack(symbol, order_k, &len);
            return len;
        }
    }
    int group_i = symbol >> order_k;
    int max_group_i = MAX_SYMBOL >> order_k;
    return (group_i) + order_k + (group_i < max_group_i);
}

int get_mode_bs(TComVBCOutputBitstream *bsOut, int gp0_mode, int gp0_k, int gp1_mode, int gp1_k, int chType, bool write)
{
    int symbol = -1;
    int out_bs = 0;
    int bs_sz = 0;
    if (cu_lossy_enable)
    {
        if (gp0_mode == PCM_MODE && gp1_mode == PCM_MODE)
        {
            out_bs = 13 << 4;
            bs_sz = 8;
        }
        else
        {
            out_bs = 7 << 6;
            if (gp0_mode == PCM_MODE)
                out_bs += 0;
            else if (gp0_mode == DPCM_MODE)
                out_bs += ((gp0_k + 1) << 3);
            if (gp1_mode == PCM_MODE)
                out_bs += 0;
            else if (gp1_mode == DPCM_MODE)
                out_bs += (gp1_k + 1);
            bs_sz = 9;
        }
        if (write)
            bsOut->write(out_bs, bs_sz);
    }
    else if (gp0_mode == PCM_MODE && gp1_mode == PCM_MODE)
    {
        out_bs = 0;
        bs_sz = 0;
    }
    else if (gp0_mode == CTX_MODE)
    {
        if (gp1_mode == CTX_MODE)
        {
            out_bs = 10;
            bs_sz = 4;
        }
        else if (gp1_mode == FIXLEN_MODE)
        {
            out_bs = 11;
            bs_sz = 4;
        }
        else if (gp1_mode == DPCM_MODE)
        {
            if (gp1_k >= 0 && gp1_k <= 3)
            {
                out_bs = (12 << 2) + gp1_k;
                bs_sz = 6;
            }
            else
            {
                out_bs = (9 << 2) + gp1_k - 4;
                bs_sz = 6;
            }
        }
        else
        {
            out_bs = (9 << 2) + 3;
            bs_sz = 6;
        }
        if (write)
            bsOut->write(out_bs, bs_sz);
    }
    else
    {
        out_bs += gp0_mode;
        if (gp0_mode == DPCM_MODE)
            out_bs += gp0_k;
        if (write)
            bsOut->write(out_bs, 4);
        bs_sz = 5;
        if (gp1_mode == CTX_MODE)
        {
            out_bs |= (1 << 4);
            if (write)
                bsOut->write(1, 1);
        }
        else
        {
            if (write)
                bsOut->write(0, 1);
            int k = 0;
            int cur = gp1_mode == DPCM_MODE ? (gp1_mode + gp1_k) : gp1_mode;
            int pred = gp0_mode == DPCM_MODE ? (gp0_mode + gp0_k) : gp0_mode;
            symbol = sign_to_unsign(cur - pred);
            int gr_code_len = get_gr_bit_count(symbol, k);
            int gr_code_word = get_gr_code_word(symbol, k);
            out_bs |= (gr_code_word << 5);
            bs_sz += gr_code_len;
            if (write)
            {
                bsOut->writeVLC(gr_code_word, gr_code_len);
            }
        }
    }
    return bs_sz;
}

int getPixel(encCU_st *encCU, int x, int y)
{
    if ((x >= 0 && x < encCU->cu_w) && (y >= 0 && y < encCU->cu_h))
        return encCU->recCU[y * encCU->cu_w + x];
    return -1;
}

int getfixPixel(encCU_st *encCU, int x, int y)
{
    if ((x >= 0 && x < encCU->cu_w) && (y >= 0 && y < encCU->cu_h))
        return encCU->fixCU[y * encCU->cu_w + x];
    return -1;
}

void setPixel(encCU_st *encCU, int x, int y, int pix)
{
    if ((x >= 0 && x < encCU->cu_w) && (y >= 0 && y < encCU->cu_h))
        encCU->recCU[y * encCU->cu_w + x] = pix;
}

int getPredPixel(encCU_st *encCU, int x, int y)
{
    if ((x >= 0 && x < encCU->cu_w) && (y >= 0 && y < encCU->cu_h))
        return encCU->predCU[y * encCU->cu_w + x];
    return -1;
}
int getlossypred(encCU_st *encCU, int x, int y)
{
    if (x > 0)
        x = x - 1;
    else
        y = y - 1;
    if ((x >= 0 && x < encCU->cu_w) && (y >= 0 && y < encCU->cu_h))
        return encCU->recCU[y * encCU->cu_w + x];
    return -1;
}

void getNei(encCU_st *encCU, int x, int y, int *out_A, int *out_B, int *out_C, int *out_D = NULL)
{
    int neiA_val = -1, neiB_val = -1, neiC_val = -1, neiD_val = -1;
    int nei_offset_x, nei_offset_y = 1;
    nei_offset_x = 1;
    if (y == 0 && x > 0)
        neiA_val = getPixel(encCU, x - 3, y);
    else if (x == 0 && y > 0)
        neiA_val = getPixel(encCU, x, y - 3);
    else
        neiA_val = getPixel(encCU, x, y - nei_offset_y);
    if (x == 0 && y > 0)
        neiB_val = getPixel(encCU, x, y - 1);
    else
        neiB_val = getPixel(encCU, x - nei_offset_x, y);
    if (y == 0 && x > 0)
        neiC_val = getPixel(encCU, x - 2, y);
    else if (x == 0 && y > 0)
        neiC_val = getPixel(encCU, x, y - 2);
    else
        neiC_val = getPixel(encCU, x - nei_offset_x, y - nei_offset_y);
    neiD_val = getPixel(encCU, x + nei_offset_x, y - nei_offset_y);

    // right CU in cu pair
    if (encCU->leftPixel != -1)
    {
        if (x == 0 && y == 0)
            neiB_val = encCU->leftPixel;
        if (x == 0 && y == 1)
            neiC_val = encCU->leftPixel;
    }

    if (neiA_val == -1 && neiB_val == -1)
    {
        *out_A = *out_B = *out_C = AVG_PREDICTOR;
        if (out_D)
            *out_D = AVG_PREDICTOR;
        return;
    }

    if (neiA_val == -1)
        neiA_val = neiB_val;

    if (neiB_val == -1)
        neiB_val = neiA_val;

    if (neiC_val == -1)
    {
        neiC_val = neiA_val != -1 ? neiA_val : neiB_val;
    }
    if (neiD_val == -1)
    {
        neiD_val = neiA_val != -1 ? neiA_val : neiB_val;
    }
    assert(neiA_val != -1);
    assert(neiB_val != -1);
    assert(neiC_val != -1);
    assert(neiD_val != -1);
    *out_A = neiA_val;
    *out_B = neiB_val;
    *out_C = neiC_val;
    if (out_D)
        *out_D = neiD_val;
#ifdef SIG_VBC
    if (g_sigdump.vbc_low)
    {
        g_sigpool.A[x + 16 * y] = *out_A;
        g_sigpool.B[x + 16 * y] = *out_B;
        g_sigpool.C[x + 16 * y] = *out_C;
        if (out_D)
            g_sigpool.D[x + 16 * y] = *out_D;
        else
            g_sigpool.D[x + 16 * y] = -1;
    }
#endif
}

int getPredictor(int neiA_val, int neiB_val, int neiC_val)
{
    assert(neiA_val != -1);
    assert(neiB_val != -1);
    assert(neiC_val != -1);

    if (neiC_val >= max(neiA_val, neiB_val))
        return min(neiA_val, neiB_val);
    else if (neiC_val <= min(neiA_val, neiB_val))
        return max(neiA_val, neiB_val);
    else
        return (neiA_val + neiB_val - neiC_val);
}

int getCUPredictor(encCU_st *encCU, int x, int y)
{
    int neiA_val = -1, neiB_val = -1, neiC_val = -1;

    getNei(encCU, x, y, &neiA_val, &neiB_val, &neiC_val);
    if (x == 0 || y == 0)
        return neiB_val;
    else
        return getPredictor(neiA_val, neiB_val, neiC_val);
}

void init_cu(int cu_idx_x, int cu_idx_y, TComPicYuv *src, ComponentID srcPlane, encCU_st *encCU, meta_st *pvbc_meta)
{
    encCU->chType = srcPlane == COMPONENT_Y ? 0 : 1;
    encCU->cu_w = pvbc_meta->cu_w;
    encCU->cu_h = pvbc_meta->cu_h;
    encCU->predCU = (unsigned char *)malloc(encCU->cu_w * encCU->cu_h);
    encCU->recCU = (unsigned char *)malloc(encCU->cu_w * encCU->cu_h);
    encCU->fixCU = (unsigned char *)malloc(encCU->cu_w * encCU->cu_h);
    encCU->codeCU = (uint64_t *)malloc(sizeof(uint64_t) * encCU->cu_w * encCU->cu_h);
    encCU->codeCULength = (unsigned int *)malloc(sizeof(unsigned int) * encCU->cu_w * encCU->cu_h);
    encCU->leftPixel = -1;

    unsigned int width = src->getWidth(srcPlane);
    unsigned int height = src->getHeight(srcPlane);
    Pel *pSrc = (Pel *)src->getAddr(srcPlane);
    unsigned int strideSrc = src->getStride(srcPlane);

    Pel *pCur = pSrc + cu_idx_y * strideSrc + cu_idx_x;
    if (srcPlane == COMPONENT_Y)
    {
        encCU->isMirrorCU = ((cu_idx_x / encCU->cu_w) & 1) == 0;
        if (!encCU->isMirrorCU)
        {
            if (cu_idx_y >= height)
                encCU->leftPixel = 0;
            else if(cu_idx_x >= width)
                encCU->leftPixel = *(pSrc + cu_idx_y * strideSrc + ((int)width - 1));
            else
                encCU->leftPixel = *(pCur - 1);
        }
    }
    else if (srcPlane == COMPONENT_Cb)
    {
        encCU->isMirrorCU = true;
    }
    else
    {
        encCU->isMirrorCU = false;
    }
    for (int y = 0; y < encCU->cu_h; y++)
    {
        for (int x = 0; x < encCU->cu_w; x++)
        {
            int index_x = min(cu_idx_x + x, (int)width - 1);
            int index_y = min(cu_idx_y + y, (int)height - 1);
            Pel *pCur = pSrc + index_y * strideSrc + index_x;
            if (encCU->isMirrorCU)
                encCU->recCU[y * encCU->cu_w + encCU->cu_w - 1 - x] = (cu_idx_y + y >= height) ? 0 : *(pCur);
            else
            {
                encCU->recCU[y * encCU->cu_w + x] = (cu_idx_y + y >= height) ? 0 : *(pCur);
            }
        }
    }

    for (int y = 0; y < encCU->cu_h; y++)
    {
        for (int x = 0; x < encCU->cu_w; x++)
        {
            encCU->predCU[x + y * encCU->cu_w] = getCUPredictor(encCU, x, y);
        }
    }

    for (int y = 0; y < encCU->cu_h; y++)
    {
        for (int x = 0; x < encCU->cu_w; x++)
        {
            encCU->fixCU[x + y * encCU->cu_w] = getPixel(encCU, x, y);
        }
    }
}

void del_cu(encCU_st *encCU)
{
    free(encCU->predCU);
    free(encCU->recCU);
    free(encCU->fixCU);
    free(encCU->codeCU);
    free(encCU->codeCULength);
}

unsigned int test_fix_dpcm_mode_group(encCU_st *encCU, int group, bool write, int k, int *str0_sz, int *str1_sz)
{
    unsigned int code_len = 0;
    unsigned int gp1_str0_len = 0, gp1_str1_len = 0,gp0_str0_len = 0, gp0_str1_len = 0;
    for (int y = 0; y < encCU->cu_h; y++)
    {
        for (int x = 0; x < encCU->cu_w; x++)
        {
            if (pix_in_group(x, y, group))
            {
                int bits = 8;
                int pix = getPixel(encCU, x, y);
                if (cu_lossy_enable)
                {
                    pix = ((pix >> lossy_truncate) << lossy_truncate) + (1 << (lossy_truncate - 1));
                    bits = 8 - lossy_truncate;
                    setPixel(encCU, x, y, pix);
                }
#ifdef SIG_VBC
                if (g_sigdump.vbc_low)
                {
                    g_sigpool.codeCU_HW[7][0] = pix;        // PCM mode
                    g_sigpool.codeCULength_HW[7][0] = bits; // PCM mode
                }
#endif
                if (x == 0 && y == 0 && encCU->leftPixel == -1)
                {
                    code_len += bits;
                    gp0_str0_len = bits;
#ifdef SIG_VBC
                    if (g_sigdump.vbc_low)
                    {
                        g_sigpool.resi[0] = 0;
                        g_sigpool.codeCU_HW[k][0] = pix;
                        g_sigpool.codeCULength_HW[k][0] = bits;
                    }
#endif
                    if (write)
                    {
                        if (bsPack)
                        {
                            encCU->codeCU[x + y * encCU->cu_w] = pix;
                            if (cu_lossy_enable)
                                encCU->codeCU[x + y * encCU->cu_w] = (pix >> lossy_truncate);
                            encCU->codeCULength[x + y * encCU->cu_w] = bits;
                        }
                        else
                        {
                            bsOut[encCU->chType].write((unsigned char)pix, bits);
                            if (cu_lossy_enable)
                                bsOut[encCU->chType].write((unsigned char)pix, (pix >> lossy_truncate));
                        }
                    }
                }
                else
                {
                    int pred = getPredPixel(encCU, x, y);
                    if (cu_lossy_enable && !((x == 0 && y == 0)))
                        pred = getlossypred(encCU, x, y);
                    if (cu_lossy_enable)
                        pred = ((pred >> lossy_truncate) << lossy_truncate) + (1 << (lossy_truncate - 1));

                    int resi = pix - pred;
                    if (cu_lossy_enable)
                    {
                        setPixel(encCU, x, y, pix);
                        resi = (resi >> lossy_truncate);
                    }
                    int symbol = sign_to_unsign(resi);
                    int gr_code_len = get_gr_bit_count(symbol, k);
                    if (group == 1)
                    {
                        if (y == 2)
                            gp1_str0_len += gr_code_len;
                        else
                            gp1_str1_len += gr_code_len;
                    }
                    else
                    {
                        if (y == 2 || y == 0)
                            gp0_str0_len += gr_code_len;
                        else
                            gp0_str1_len += gr_code_len;
                    }
                    if (gr_code_len > MAX_CODE_LEN /*|| gp1_str0_len > MAX_GP1_STR0_LEN || gp1_str1_len > MAX_GP1_STR1_LEN*/)
                    {
                        code_len += 1000; // aka mark assert for hw, add code_len to avoid selected
                        // assert(!write);
                        // return MAX_INT;
                    }
                    code_len += gr_code_len;
#ifdef SIG_VBC
                    if (g_sigdump.vbc_low)
                    {
                        uint64_t gr_code_word;
                        if (bsPack)
                            gr_code_word = get_gr_code_word_pack(symbol, k, &gr_code_len);
                        else
                            gr_code_word = get_gr_code_word(symbol, k);
                        g_sigpool.resi[x + y * encCU->cu_w] = resi;
                        g_sigpool.codeCU_HW[k][x + y * encCU->cu_w] = gr_code_word;
                        g_sigpool.codeCULength_HW[k][x + y * encCU->cu_w] = gr_code_len;
                    }
#endif
                    if (write)
                    {
                        assert(gr_code_len <= MAX_CODE_LEN);
                        if (bsPack)
                        {
                            uint64_t gr_code_word = get_gr_code_word_pack(symbol, k, &gr_code_len);
                            encCU->codeCU[x + y * encCU->cu_w] = gr_code_word;
                            encCU->codeCULength[x + y * encCU->cu_w] = gr_code_len;
                        }
                        else
                        {
                            uint64_t gr_code_word = get_gr_code_word(symbol, k);
                            bsOut[encCU->chType].writeVLC64(gr_code_word, gr_code_len);
                        }
                    }
                }
            }
        }
    }
    *str0_sz = gp0_str0_len + gp1_str0_len;
    *str1_sz = gp0_str1_len + gp1_str1_len;

    return code_len;
}

//unsigned int test_dpcm_mode_group(encCU_st *encCU, int gp, int k)
unsigned int test_dpcm_mode_group(encCU_st *encCU, int gp, int k, int *str0_sz, int *str1_sz)
{
    int dpcm_bs;
    unsigned int best_bs = MAX_INT;
    {
        dpcm_bs = test_fix_dpcm_mode_group(encCU, gp, false, k,str0_sz,str1_sz);
        if (dpcm_bs < best_bs)
        {
            best_bs = dpcm_bs;
        }
        encCU->mode_len[gp][DPCM_MODE + k] = dpcm_bs;
    }
    return best_bs;
}

int get_context_key(CtxType ctxType, int grad1, int grad2, int grad3)
{
    if (ctxType == CTX_1D)
    {
        int q2 = m_ctx_quant_lut[grad2];
        int q3 = m_ctx_quant_lut[grad3];
        return (abs(q2) * abs(q2) + abs(q3) * abs(q3));
    }
    else
    {
        int q1 = m_ctx_quant_lut[grad1];
        int q2 = m_ctx_quant_lut[grad2];
        int q3 = m_ctx_quant_lut[grad3];
        return (abs(q1) * abs(q1) + abs(q2) * abs(q2) + abs(q3) * abs(q3));
    }
}

int get_ctx_1d_pred(encCU_st *encCU, int x, int y, int pix_buf[4])
{
    int last = AVG_PREDICTOR;
    int offset = 1;
    int valid_pix = 0;

    // cu in right pair and ref left cu pixel.
    if (y == 0 && encCU->leftPixel != -1)
    {
        last = encCU->leftPixel;
    }

    // get previous 4 pixel
    for (int i = 0; i < 4; i++)
    {
        int nei_x = x, nei_y = y;
        if (y == 0)
            nei_x = x - offset * (1 + i);
        else
            nei_y = y - (1 + i);

        int nei = getPixel(encCU, nei_x, nei_y);

        if (nei == -1)
            pix_buf[i] = last;
        else
        {
            pix_buf[i] = nei;
            valid_pix++;
        }
        last = pix_buf[i];
    }
    return valid_pix;
}

void get_ctx_id(CtxType ctxType, encCU_st *encCU, int x, int y, int *pred, int *ctx_id)
{
    int ctx_key;

    if (ctxType == CTX_2D)
    {
        int neiA_val = -1, neiB_val = -1, neiC_val = -1, neiD_val = -1;
        getNei(encCU, x, y, &neiA_val, &neiB_val, &neiC_val, &neiD_val);
        *pred = getPredictor(neiA_val, neiB_val, neiC_val);

        ctx_key = get_context_key(ctxType, (int)neiD_val - neiA_val,
                                  (int)neiA_val - neiC_val,
                                  (int)neiC_val - neiB_val);
    }
    else
    {
        int pix_buf[4] = {-1};
        get_ctx_1d_pred(encCU, x, y, pix_buf);
        *pred = pix_buf[0];

        ctx_key = get_context_key(ctxType, (int)pix_buf[3] - pix_buf[2],
                                  (int)pix_buf[2] - pix_buf[1], (int)pix_buf[1] - pix_buf[0]);
    }

    *ctx_id = ctx_key;
}

int get_fix_table_golomb_coding_parameter(CtxType ctxType, int chType, int id)
{
    if (ctxType == CTX_1D)
    {
        if (chType == 0)
        {
            if (id < 2)
                return 1;
            else if (id < 8)
                return 2;
            else if (id < 9)
                return 3;
            else if (id < 16)
                return 4;
            else
                return 5;
        }
        else
        {
            if (id < 1)
                return 0;
            else if (id < 3)
                return 1;
            else if (id < 9)
                return 2;
            else if (id < 16)
                return 3;
            else
                return 4;
        }
    }
    else
    {
        if (chType == 0)
        {
            if (id < 2)
                return 0;
            else if (id < 8)
                return 1;
            else if (id < 16)
                return 2;
            else if (id < 25)
                return 3;
            else if (id < 48)
                return 4;
            else
                return 5;
        }
        else
        {
            if (id < 5)
                return 0;
            else if (id < 13)
                return 1;
            else if (id < 22)
                return 2;
            else if (id < 34)
                return 3;
            else
                return 4;
        }
    }
}

void get_ctx_symbol(CtxType ctxType, encCU_st *encCU, int x, int y, int *symbol, int *k)
{
    int pix = getPixel(encCU, x, y);
    int pred;
    int ctx_id;
    get_ctx_id(ctxType, encCU, x, y, &pred, &ctx_id);
    if (cu_lossy_enable)
        pred = ((pred >> lossy_truncate) << lossy_truncate) + (1 << (lossy_truncate - 1));
    int resi = pix - pred;
    *symbol = sign_to_unsign(resi);
    *k = get_fix_table_golomb_coding_parameter(ctxType, encCU->chType, ctx_id);
}

unsigned int test_ctx_dpcm_mode_group(int chType, encCU_st *encCU, meta_st *pvbc_meta, int group, bool write, int *str0_sz, int *str1_sz)
{
    unsigned int code_len = 0;
    unsigned int gp1_str0_len = 0, gp1_str1_len = 0,gp0_str0_len = 0, gp0_str1_len = 0;
    bool overflow = false;

    for (int y = 0; y < encCU->cu_h; y++)
    {
        for (int x = 0; x < encCU->cu_w; x++)
        {
            if (pix_in_group(x, y, group))
            {
                if (x == 0 && y == 0 && encCU->leftPixel == -1)
                {
                    code_len += 8;
                    gp0_str0_len += 8;
#ifdef SIG_VBC
                    if (g_sigdump.vbc_low)
                    {
                        g_sigpool.codeCU_HW[8][0] = getPixel(encCU, x, y);
                        g_sigpool.codeCULength_HW[8][0] = 8;
                        g_sigpool.ctx_k[0] = 0;
                    }
#endif
                    if (write)
                    {
                        int pix = getPixel(encCU, x, y);
                        if (bsPack)
                        {
                            encCU->codeCU[x + y * encCU->cu_w] = pix;
                            encCU->codeCULength[x + y * encCU->cu_w] = 8;
                        }
                        else
                        {
                            bsOut[encCU->chType].write((unsigned char)pix, 8);
                        }
                    }
                }
                else
                {
                    int k, symbol;
                    get_ctx_symbol(group == 0 ? CTX_1D : CTX_2D, encCU, x, y, &symbol, &k);
                    int gr_code_len = get_gr_bit_count(symbol, k);
                    if (group == 1)
                    {
                        if (y == 2)
                            gp1_str0_len += gr_code_len;
                        else
                            gp1_str1_len += gr_code_len;
                    }
                    else
                        {
                            if (y == 2 || y == 0)
                                gp0_str0_len += gr_code_len;
                            else
                                gp0_str1_len += gr_code_len;
                        }
                    if (gr_code_len > MAX_CODE_LEN /*|| gp1_str0_len > MAX_GP1_STR0_LEN || gp1_str1_len > MAX_GP1_STR1_LEN*/)
                    {
                        overflow = true;
                    }
                    code_len += gr_code_len;
#ifdef SIG_VBC
                    if (g_sigdump.vbc_low)
                    {
                        uint64_t gr_code_word;
                        if (bsPack)
                            gr_code_word = get_gr_code_word_pack(symbol, k, &gr_code_len);
                        else
                            gr_code_word = get_gr_code_word(symbol, k);
                        g_sigpool.codeCU_HW[8][x + y * encCU->cu_w] = gr_code_word;
                        g_sigpool.codeCULength_HW[8][x + y * encCU->cu_w] = gr_code_len;
                        g_sigpool.ctx_k[x + y * encCU->cu_w] = k;
                    }
#endif
                    if (write)
                    {
                        assert(gr_code_len <= MAX_CODE_LEN);
                        if (bsPack)
                        {
                            uint64_t gr_code_word = get_gr_code_word_pack(symbol, k, &gr_code_len);
                            encCU->codeCU[x + y * encCU->cu_w] = gr_code_word;
                            encCU->codeCULength[x + y * encCU->cu_w] = gr_code_len;
                        }
                        else
                        {
                            uint64_t gr_code_word = get_gr_code_word(symbol, k);
                            bsOut[encCU->chType].writeVLC64(gr_code_word, gr_code_len);
                        }
                    }
                }
            }
        }
    }
    *str0_sz = gp0_str0_len + gp1_str0_len;
    *str1_sz = gp0_str1_len + gp1_str1_len;
    if (overflow == true)
        return MAX_INT;
    return code_len;
}

int test_fix_len_mode_group(encCU_st *encCU, int group, bool write, int *max_resi_bit)
{

    int max_symbol = 0;
    int comp_pix_cnt = 0;
    int code_len = 0;
    for (int y = 0; y < encCU->cu_h; y++)
    {
        for (int x = 0; x < encCU->cu_w; x++)
        {
            if (pix_in_group(x, y, group))
            {
                int pix = getPixel(encCU, x, y);
                if (x == 0 && y == 0 && encCU->leftPixel == -1)
                {
                    if (write)
                    {
                        if (bsPack)
                        {
                            encCU->codeCU[x + y * encCU->cu_w] = pix;
                            encCU->codeCULength[x + y * encCU->cu_w] = 8;
                        }
                        else
                        {
                            bsOut[encCU->chType].write((unsigned char)pix, 8);
                        }
                    }
                    code_len += 8;
                }
                else
                {
                    int pred = getPredPixel(encCU, x, y);
                    int resi = pix - pred;
                    int symbol = sign_to_unsign(resi);
                    if (symbol > max_symbol)
                        max_symbol = symbol;
                    if (write)
                    {
                        if (bsPack)
                        {
                            encCU->codeCU[x + y * encCU->cu_w] = symbol;
                            encCU->codeCULength[x + y * encCU->cu_w] = *max_resi_bit;
                        }
                        else
                        {
                            bsOut[encCU->chType].write(symbol, *max_resi_bit);
                        }
                    }
                    comp_pix_cnt++;
                }
            }
        }
    }

    if (max_resi_bit)
    {
        int i = 0;
        while (max_symbol)
        {
            i++;
            max_symbol >>= 1;
        }
        *max_resi_bit = i;
    }

    code_len += (*max_resi_bit) * comp_pix_cnt;
    if (*max_resi_bit <= 0)
        return code_len;
    else
        return MAX_INT;
}

void writeOutUncompressGroup(encCU_st *pCU, int gp, int cu_lossy_enable)
{
    int chtype = pCU->chType;
    for (int y = 0; y < pCU->cu_h; y++)
    {
        for (int x = 0; x < pCU->cu_w; x++)
        {
            if (pix_in_group(x, y, gp))
            {
                int pix = getPixel(pCU, x, y);
                if (bsPack)
                {
                    if (cu_lossy_enable)
                    {
                        int data = getfixPixel(pCU, x, y) >> lossy_truncate;
                        pCU->codeCU[x + y * pCU->cu_w] = data;
                        pCU->codeCULength[x + y * pCU->cu_w] = 8 - lossy_truncate;
                        data = (data << lossy_truncate) + (1 << (lossy_truncate - 1));
                        setPixel(pCU, x, y, data);
                    }
                    else
                    {
                        pCU->codeCU[x + y * pCU->cu_w] = pix;
                        pCU->codeCULength[x + y * pCU->cu_w] = 8;
                    }
                }
                else
                {
                    if (cu_lossy_enable)
                        bsOut[chtype].write(pix >> lossy_truncate, 8 - lossy_truncate);
                    else
                        bsOut[chtype].write(pix, 8);
                }
            }
        }
    }
}

void sig_vbe_bs_meta_ctx(int chType, int cu_idx_x, int cu_idx_y, int cu_best_bs, int org_len, int str0_sz, int str1_sz)
{
    unsigned char *bs_ptr = (unsigned char *)bsOut[chType].getByteStream() + org_len - g_sigpool.cu_best_bs[0];
    unsigned char x[2],y[2],cusize[2];
    unsigned char stream[128] = {0};
    unsigned char null[10] = {0};
    int i,bs_size;
    cu_idx_x = cu_idx_x-16;
    x[1] = cu_idx_x>>8;
    y[1] = cu_idx_y>>8;
    x[0] = cu_idx_x%256;
    y[0] = cu_idx_y%256;
    bs_size = g_sigpool.cu_best_bs[0] + g_sigpool.cu_best_bs[1];//cu
    cusize[1] = g_sigpool.cu_best_bs[1]-1;
    cusize[0] = g_sigpool.cu_best_bs[0]-1;

    memcpy(stream,bs_ptr,bs_size);
    for(i=0;i<2;i++)
        bsmetacpx[chType].write(x[i],8);
    for(i=0;i<2;i++)
        bsmetacpx[chType].write(y[i],8);
    for(i=0;i<2;i++)
        bsmetacpx[chType].write(cusize[i],8);
    for(i=0;i<10;i++)
        bsmetacpx[chType].write(null[i],8);
    for(i=0;i<128;i++)
        bsmetacpx[chType].write(stream[i],8);
}

void sig_vbe_bs_ctx(TComVBCOutputBitstream *bsOut, int chType, int cu_idx_x, int cu_idx_y, int cu_best_bs, int org_len, int str0_sz, int str1_sz)
{
    int i;
    unsigned char *bs_ptr = (unsigned char *)bsOut->getByteStream() + org_len;

    if (((cu_idx_x / 16) & 1) == 0)
    {
        g_sigpool.cu_best_bs[0] = cu_best_bs;
        g_sigpool.org_len = org_len;
        sigdump_output_fprint(&g_sigpool.vbe_bs_ctx, "# (x, y) = (%d, %d)\n", cu_idx_x, cu_idx_y);
        sigdump_output_fprint(&g_sigpool.vbe_bs_ctx, "# left\n");
    }
    else
    {
        sigdump_output_fprint(&g_sigpool.vbe_bs_ctx, "# right\n");
    }

    sigdump_output_fprint(&g_sigpool.vbe_bs_ctx, "# s0_sz = %d, s1_sz = %d, cu_sz = %d\n", str0_sz, str1_sz,
                          (str0_sz + str1_sz + 7) / 8 * 8);

    for (i = 0; i < cu_best_bs; i++)
    {
        sigdump_output_fprint(&g_sigpool.vbe_bs_ctx, "%02x", bs_ptr[i]);
        if (((i + 1) % 32) == 0)
            sigdump_output_fprint(&g_sigpool.vbe_bs_ctx, "\n");
    }
    if ((cu_best_bs % 32) != 0)
        sigdump_output_fprint(&g_sigpool.vbe_bs_ctx, "\n");
}

void sig_vbe_mode_ctx(encCU_st *encCU, int chType, int cu_idx_x, int cu_idx_y, int best_mode[2],int best_k[2])
{
    sigdump_output_fprint(&g_sigpool.vbe_mode_ctx, "# CU (x, y) = (%d, %d)\n", cu_idx_x, cu_idx_y);
    for (int gp = 0; gp <= 1; gp++)
    {
        // left
        if (encCU->isMirrorCU)
        {
            sigdump_output_fprint(&g_sigpool.vbe_mode_ctx, "# left gp %d\n", gp);
        }
        else
        {
            sigdump_output_fprint(&g_sigpool.vbe_mode_ctx, "# right gp %d\n", gp);
        }
        for (int i = 0; i <= 9; i++)
        {
            sigdump_output_fprint(&g_sigpool.vbe_mode_ctx, "mode%d_len = %x\n", i, encCU->mode_len[gp][i] > MAX_INT ? MAX_INT : encCU->mode_len[gp][i]);
        }
        if(best_mode[gp]==1)
            sigdump_output_fprint(&g_sigpool.vbe_mode_ctx, "mode_win = %d\n", best_mode[gp]+best_k[gp]);
        else
            sigdump_output_fprint(&g_sigpool.vbe_mode_ctx, "mode_win = %d\n", best_mode[gp]);
    }
}
void sig_vbe_strm_pack_out(encCU_st *pCU, int chType, int cu_idx_x, int cu_idx_y, int org_len1, int org_len2)
{
    int count1, count2, v[4];
    unsigned char *bs_ptr;
    bs_ptr = (unsigned char *)bsOut1[chType].getByteStream() + org_len1;
    count1 = bsOut1[chType].getByteStreamLength() - org_len1;
    if (count1 % 4)
        count2 = (count1 / 4 + 1) * 4;
    else
        count2 = count1;
    if (((cu_idx_x / 16) & 1) == 0)
    {
        sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "# CU (x, y) = (%d, %d)\n", cu_idx_x, cu_idx_y);
        sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "# enc_cu_cflag = %d, %d // Left cu ,Right cu\n", chType, chType);
        sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "# enc_cu_lossy = %d, %d // Left cu ,Right cu\n", cu_lossy_enable, cu_lossy_enable);
    }
    sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "#stream0\n");
    for (int i = 0; i < count2; i++)
    {
        if (((i) % 4) == 0)
            sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "0x");
        if (i < count1)
        {
            sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "%02x", bs_ptr[i]);
            v[i % 4] = 1;
        }
        else
        {
            sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "%02x", 0);
            v[i % 4] = 0;
        }
        if (((i + 1) % 4) == 0)
            sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "\n");
    }
    sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "1x%01x%01x%01x%01x0000\n", v[0], v[1], v[2], v[3]);
    bs_ptr = (unsigned char *)bsOut2[pCU->chType].getByteStream() + org_len2;
    count1 = bsOut2[pCU->chType].getByteStreamLength() - org_len2;
    if (count1 % 4)
        count2 = (count1 / 4 + 1) * 4;
    else
        count2 = count1;
    sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "#stream1\n");
    for (int i = 0; i < count2; i++)
    {
        if (((i) % 4) == 0)
            sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "0x");
        if (i < count1)
        {
            sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "%02x", bs_ptr[i]);
            v[i % 4] = 1;
        }
        else
        {
            sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "%02x", 0);
            v[i % 4] = 0;
        }
        if (((i + 1) % 4) == 0)
            sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "\n");
    }
    sigdump_output_fprint(&g_sigpool.vbe_strm_pack_out_ctx, "1x%01x%01x%01x%01x0000\n", v[0], v[1], v[2], v[3]);
}

void sig_vbe_pack_out(TComVBCOutputBitstream *bsOut, encCU_st *pCU, int chType)
{
    sig_ctx *vbe_pack_out_ctx;
    int count1, count2;
    int v[4];
    vbe_pack_out_ctx = &g_sigpool.vbe_pack_out_ctx[chType];
    count1 = g_sigpool.cu_best_bs[0] + g_sigpool.cu_best_bs[1];
    if (count1 % 4)
        count2 = (count1 / 4 + 1) * 4;
    else
        count2 = count1;
    unsigned char *bs_ptr = (unsigned char *)bsOut->getByteStream() + g_sigpool.org_len;
    sigdump_output_fprint(vbe_pack_out_ctx, "# vbe_cu_cpx_data\n");
    for (int i = 0; i < count2; i++)
    {
        if (((i) % 4) == 0)
            sigdump_output_fprint(vbe_pack_out_ctx, "0x");
        if (i < count1)
        {
            sigdump_output_fprint(vbe_pack_out_ctx, "%02x", bs_ptr[i]);
            v[i % 4] = 1;
        }
        else
        {
            sigdump_output_fprint(vbe_pack_out_ctx, "%02x", 0);
            v[i % 4] = 0;
        }
        if (((i + 1) % 4) == 0)
            sigdump_output_fprint(vbe_pack_out_ctx, "\n");
    }
    sigdump_output_fprint(vbe_pack_out_ctx, "1x%01x%01x%01x%01x0000\n", v[0], v[1], v[2], v[3]);
}

void sig_vbe_src(encCU_st *encCU, int chType, int cu_idx_x, int cu_idx_y)
{
    int x, y;
    sig_ctx *vbe_src_ctx = &g_sigpool.vbe_src_ctx[chType];

    sigdump_output_fprint(vbe_src_ctx, "# CU (x, y) = (%d, %d)\n", cu_idx_x, cu_idx_y);
    sigdump_output_fprint(vbe_src_ctx, "# enc_cu_cflag = %d\n", chType);
    sigdump_output_fprint(vbe_src_ctx, "# reg_lossy_cr_ratio = %d\n", lossy_cr);
    sigdump_output_fprint(vbe_src_ctx, "# reg_lossy_tolerence = %d\n", lossy_tolerence);
    sigdump_output_fprint(vbe_src_ctx, "# reg_lossy_delay = %d\n", lossy_delay);
    sigdump_output_fprint(vbe_src_ctx, "# reg_lossy_truncate = %d\n", lossy_truncate);

    for (y = 0; y < encCU->cu_h; y++)
    {
        sigdump_output_fprint(vbe_src_ctx, "0x");
        for (x = 0; x < encCU->cu_w; x++)
        {
            if (encCU->isMirrorCU)
                sigdump_output_fprint(vbe_src_ctx, "%02x", getPixel(encCU, encCU->cu_w - 1 - x, y));
            else
                sigdump_output_fprint(vbe_src_ctx, "%02x", getPixel(encCU, x, y));
        }
        sigdump_output_fprint(vbe_src_ctx, "\n");
    }

    sigdump_output_fprint(vbe_src_ctx, "# enc_cu_lossy = %d\n", cu_lossy_enable);
    sigdump_output_fprint(vbe_src_ctx, "# enc_cu_tb = %d\n", lossy_truncate);
}

void sig_vbe_lossy_src(encCU_st *encCU, int chType, int cu_idx_x, int cu_idx_y)
{
    int x, y;
    sig_ctx *vbe_loss_src_ctx = &g_sigpool.vbe_lossy_src_ctx[chType];

    sigdump_output_fprint(vbe_loss_src_ctx, "# CU (x, y) = (%d, %d)\n", cu_idx_x, cu_idx_y);
    for (y = 0; y < 4; y++)
    {
        sigdump_output_fprint(vbe_loss_src_ctx, "0x");
        for (x = 15; x >= 0; x--)
        {
            sigdump_output_fprint(vbe_loss_src_ctx, "%02x", getPixel(encCU, x, y));
        }
        sigdump_output_fprint(vbe_loss_src_ctx, "\n");
    }
}

void sig_vbe_pu_str(encCU_st *pCU, int cu_lossy_enable, int ori_lossy_cnt, int input_format, int cu_idx_x, int cu_idx_y)
{
    const int *stream_order[2];
    unsigned int stream_cnt[2];
    if (g_VBCVersion >= 1)
    {
        stream_order[0] = stream0_order_half;
        stream_cnt[0] = sizeof(stream0_order_half) / sizeof(int);
        stream_order[1] = stream1_order_half;
        stream_cnt[1] = sizeof(stream1_order_half) / sizeof(int);
    }
    else
    {
        stream_order[0] = stream0_order_luma;
        stream_cnt[0] = sizeof(stream0_order_luma) / sizeof(int);
        stream_order[1] = stream1_order_luma;
        stream_cnt[1] = sizeof(stream1_order_luma) / sizeof(int);
    }


    int isR = ((cu_idx_x / 16) & 1);
    int cuflag;
    if(input_format==2)
    {
        cuflag = cu_idx_x/32%3;
    }
    else
    {
        cuflag = pCU->chType;
    }

    for (int i = 0; i < 2; i++)
    {
        sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][i], "# CU (x, y) = (%d, %d)\n", cu_idx_x, cu_idx_y);
        sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][i], "# enc_cu_cflag = %d\n", cuflag);
        sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][i], "# enc_cu_lossy = %d\n", cu_lossy_enable);
        sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][i], "# reg_cu_lsy_acc = %d\n", g_sigpool.lossy_diff_sum[0]);
        sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][i], "# reg_cu_lsy_nu = %d\n", ori_lossy_cnt);
        sigdump_output_fprint(&g_sigpool.vbe_pu_str_out_ctx[isR][i], "# CU (x, y) = (%d, %d)\n", cu_idx_x, cu_idx_y);
        sigdump_output_fprint(&g_sigpool.vbe_pu_str_out_ctx[isR][i], "# pu_gen_len = %d %d %d %d\n", g_sigpool.best_size_HW[0][0], g_sigpool.best_size_HW[0][1], g_sigpool.best_size_HW[1][0], g_sigpool.best_size_HW[1][1]);
        sigdump_output_fprint(&g_sigpool.vbe_pu_str_out_ctx[isR][i], "# pu_gen_mode = %d %d %d %d\n", g_sigpool.best_mode_HW[0][0], g_sigpool.best_mode_HW[0][1], g_sigpool.best_mode_HW[1][0], g_sigpool.best_mode_HW[1][1]);
        sigdump_output_fprint(&g_sigpool.vbe_pu_str_out_ctx[isR][i], "# enc_cu_lossy = %d %d\n", cu_lossy_enable, cu_lossy_enable);
        sigdump_output_fprint(&g_sigpool.vbe_pu_str_out_ctx[isR][i], "# is_uncompress = %d %d\n", (g_sigpool.uncompress[0][0] || g_sigpool.uncompress[0][1]), (g_sigpool.uncompress[1][0] || g_sigpool.uncompress[1][1]));
    }

    for (int str = 0; str < 2; str++)
    {
        for (int i = 0; i < stream_cnt[str]; i++)
        {
            int x = stream_order[str][i] % pCU->cu_w;
            int y = stream_order[str][i] / pCU->cu_w;
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "# P(x,y) = (%d,%d)\n", stream_order[str][i] % 16, stream_order[str][i] / 16);
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "# enc_cu_strm_src = %d\n", getfixPixel(pCU, x, y));
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "# enc_cu_strm_rec = %d\n", getPixel(pCU, stream_order[str][i] % 16, stream_order[str][i] / 16));
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "# enc_cu_strm_neb = %d %d %d %d\n", g_sigpool.A[stream_order[str][i]], g_sigpool.B[stream_order[str][i]], g_sigpool.C[stream_order[str][i]], g_sigpool.D[stream_order[str][i]]);
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "# enc_cu_strm_symbol =");
            for (int j = 0; j < 9; j++)
                sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "%d ", g_sigpool.codeCU_HW[j][stream_order[str][i]]);
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "\n");
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "# enc_cu_strm_len =");
            for (int j = 0; j < 9; j++)
                sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "%d ", g_sigpool.codeCULength_HW[j][stream_order[str][i]]);
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "\n");
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "# enc_cu_strm_ctx_k = %d\n", g_sigpool.ctx_k[stream_order[str][i]]);
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_ctx[isR][str], "# enc_cu_strm_residual = %d\n", g_sigpool.resi[stream_order[str][i]]);
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_out_ctx[isR][str], "# P(x,y) = (%d,%d)\n", stream_order[str][i] % 16, stream_order[str][i] / 16);
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_out_ctx[isR][str], "# pu_bs_out_str = %x\n", pCU->codeCU[stream_order[str][i]]);
            sigdump_output_fprint(&g_sigpool.vbe_pu_str_out_ctx[isR][str], "# pu_bs_length_str = %d\n", pCU->codeCULength[stream_order[str][i]]);
        }
    }

}

void sig_vbe_rec(encCU_st *encCU, int chType, int cu_idx_x, int cu_idx_y)
{
    int x, y;
    sigdump_output_fprint(&g_sigpool.vbe_rec_ctx, "# CU (x, y) = (%d, %d)\n", cu_idx_x, cu_idx_y);
    sigdump_output_fprint(&g_sigpool.vbe_rec_ctx, "# enc_cu_cflag = %d\n", chType);
    sigdump_output_fprint(&g_sigpool.vbe_rec_ctx, "# enc_cu_lossy = %d\n", cu_lossy_enable);

    for (y = 0; y < encCU->cu_h; y++)
    {
        sigdump_output_fprint(&g_sigpool.vbe_rec_ctx, "0x");
        for (x = 0; x < encCU->cu_w; x++)
        {
            if (encCU->isMirrorCU)
                sigdump_output_fprint(&g_sigpool.vbe_rec_ctx, "%02x", getPixel(encCU, encCU->cu_w - 1 - x, y));
            else
                sigdump_output_fprint(&g_sigpool.vbe_rec_ctx, "%02x", getPixel(encCU, x, y));
        }
        sigdump_output_fprint(&g_sigpool.vbe_rec_ctx, "\n");
    }
}
#if 1
void cal_mean(encCU_st *pCU)
{
    int sum = 0, mean, diff_sum = 0, var = 0, k;
    for (k = 0; k < 2; k++)
        g_sigpool.lossy_diff_sum[k] = 0;
    for (int y = 0; y < 4; y++)
        for (int x = 15; x >= 0; x--)
            sum += getfixPixel(pCU, x, y);
    mean = sum / 64;

    const int *stream_order[2];
    unsigned int stream_cnt[2];
    if (g_VBCVersion >= 1)
    {
        stream_order[0] = stream0_order_half;
        stream_cnt[0] = sizeof(stream0_order_half) / sizeof(int);
        stream_order[1] = stream1_order_half;
        stream_cnt[1] = sizeof(stream1_order_half) / sizeof(int);
    }
    else
    {
        stream_order[0] = stream0_order_luma;
        stream_cnt[0] = sizeof(stream0_order_luma) / sizeof(int);
        stream_order[1] = stream1_order_luma;
        stream_cnt[1] = sizeof(stream1_order_luma) / sizeof(int);
    }


    for (int str = 0; str < 2; str++)
    {
        for (k = 0; k < stream_cnt[str]; k++)
        {
            int x = stream_order[str][k] % pCU->cu_w;
            int y = stream_order[str][k] / pCU->cu_w;
            int fix = getfixPixel(pCU, x, y);
            int update = getPixel(pCU, x, y);
            int diff = fix - update;
            g_sigpool.lossy_diff_sum[str] += diff;
            if (diff > g_sigpool.diff_max)
                g_sigpool.diff_max = diff;
            diff_sum += abs(diff);
            var += ((fix - mean) * (fix - mean));
        }
    }
    var /= 64;
    g_sigpool.diff_mean += diff_sum / 64;
    g_sigpool.var_mean += sqrt(var);
}
#endif

int compressOneCU(meta_st *pvbc_meta, encCU_st *encCU, int chType, int cu_idx_x, int cu_idx_y)
{
    int best_k[2] = {0, 0};
    int best_mode[2];
    int best_bs[2] = {MAX_INT, MAX_INT};
    int gp_max_resi_bit[2] = {0, 0};
    int gp_sz[2];
    int cu_best_bs = 0, gp;
    int cur_lossy = 0;
    static int lossy_enable_delay[10], pre_cu_idx_x = -1, pre_cu_idx_y = -1;
    int bsOut_type = pvbc_meta->ycinterleave ? 0 : chType;
    unsigned int org_len = bsOut[bsOut_type].getByteStreamLength();
    int stream0_bit_size[20],stream1_bit_size[20],final_str0_bit_size,final_str1_bit_size,gp1_str0_len,gp1_str1_len;
    memset(stream0_bit_size,0,20*sizeof(int));
    memset(stream1_bit_size,0,20*sizeof(int));
    if (acc_bits == 0)
    {
        pre_cu_idx_x = -1;
        pre_cu_idx_y = -1;
    }
    if (acc_bits == 0)
        memset(lossy_enable_delay, 0, 10 * sizeof(int));
    if (((cu_idx_x / 16) % 2 == 0 && chType == 0) || (!(pre_cu_idx_x == cu_idx_x && pre_cu_idx_y == cu_idx_y) && chType))
    {
        if (lossy)
        {
            if (acc_bits > (target_bits + lossy_tolerence))
            {
                cur_lossy = 1;
            }
            else
            {
                cur_lossy = 0;
            }
        }
        else
        {
            cur_lossy = 0;
        }
        for (int i = 9; i > 0; i--)
            lossy_enable_delay[i] = lossy_enable_delay[i - 1];
        lossy_enable_delay[0] = cur_lossy;
        cu_lossy_enable = lossy_enable_delay[lossy_delay];
    }

#ifdef SIG_VBC
    if (g_sigdump.vbc_low)
    {
        sig_vbe_src(encCU, chType, cu_idx_x, cu_idx_y);
    }
#endif

    pre_cu_idx_x = cu_idx_x;
    pre_cu_idx_y = cu_idx_y;
    for (gp = 0; gp < 2; gp++)
    {
        int k, best_dpcm = MAX_INT;
        gp_sz[gp] = get_group_size(encCU, gp);
        if (cu_lossy_enable)
        {
            int bits = 8 - lossy_truncate;
            best_bs[gp] = (gp_sz[gp] * bits);
            for (int y = 0; y < encCU->cu_h; y++)
                for (int x = 0; x < encCU->cu_w; x++)
                {
                    if (pix_in_group(x, y, gp))
                    {
                        int pix = getPixel(encCU, x, y);
                        pix = ((pix >> lossy_truncate) << lossy_truncate) + (1 << (lossy_truncate - 1));
                        setPixel(encCU, x, y, pix);
                    }
                }
        }
        else
            best_bs[gp] = (gp_sz[gp] * 8);
        best_mode[gp] = PCM_MODE;
        if(gp==0)
        {
            stream0_bit_size[10*gp+PCM_MODE] = 8 * 17;
            stream1_bit_size[10*gp+PCM_MODE] = 8 * 2;
        }
        else
        {
            stream0_bit_size[10*gp+PCM_MODE] = 8 * 15;
            stream1_bit_size[10*gp+PCM_MODE] = 8 * 30;
        }
        if (gp == 1)
        {
            int modebits;
            modebits = get_mode_bs(&bsOut[bsOut_type], best_mode[0], best_k[0], PCM_MODE, -1, chType, false);
            best_bs[gp] += modebits;
            if(best_mode[0] == DPCM_MODE)
            {
                final_str0_bit_size = stream0_bit_size[10*0+best_mode[0]+best_k[0]] + stream0_bit_size[10*1+PCM_MODE];
                final_str1_bit_size = stream1_bit_size[10*0+best_mode[0]+best_k[0]] + stream1_bit_size[10*1+PCM_MODE];
            }
            else
            {
                final_str0_bit_size = stream0_bit_size[10*0+best_mode[0]] + stream0_bit_size[10*1+PCM_MODE];
                final_str1_bit_size = stream1_bit_size[10*0+best_mode[0]] + stream1_bit_size[10*1+PCM_MODE];
            }
            gp1_str0_len = stream0_bit_size[10*1+PCM_MODE];
            gp1_str1_len = stream1_bit_size[10*1+PCM_MODE];
            if (g_VBCVersion >= 1)
            {
                if(modebits+final_str0_bit_size>264)
                    best_bs[gp] += 1000;
                if(final_str1_bit_size>256)
                    best_bs[gp] += 1000;
            }
            else if(g_VBCVersion==0)
            {
                if(gp1_str0_len > MAX_GP1_STR0_LEN || gp1_str1_len > MAX_GP1_STR1_LEN)
                    best_bs[gp] += 1000;
            }
        }

        encCU->mode_len[gp][PCM_MODE] = best_bs[gp];

        unsigned int dpcm_bs;
        for (k = 0; k < 7; k++)
        {
            dpcm_bs = test_dpcm_mode_group(encCU, gp, k, &stream0_bit_size[10*gp+k+DPCM_MODE],&stream1_bit_size[10*gp+k+DPCM_MODE]);
            if (gp == 1)
            {
                int modebits;
                modebits = get_mode_bs(&bsOut[bsOut_type], best_mode[0], best_k[0], DPCM_MODE, k, chType, false);
                 dpcm_bs += modebits;
                if(best_mode[0] == DPCM_MODE)
                {
                    final_str0_bit_size = stream0_bit_size[10*0+best_mode[0]+best_k[0]] + stream0_bit_size[10*1+DPCM_MODE+k];
                    final_str1_bit_size = stream1_bit_size[10*0+best_mode[0]+best_k[0]] + stream1_bit_size[10*1+DPCM_MODE+k];
                }
                else
                {
                    final_str0_bit_size = stream0_bit_size[10*0+best_mode[0]] + stream0_bit_size[10*1+DPCM_MODE+k];
                    final_str1_bit_size = stream1_bit_size[10*0+best_mode[0]] + stream1_bit_size[10*1+DPCM_MODE+k];
                }
                gp1_str0_len = stream0_bit_size[10*1+DPCM_MODE+k];
                gp1_str1_len = stream1_bit_size[10*1+DPCM_MODE+k];
                if (g_VBCVersion >= 1)
                {
                    if(modebits+final_str0_bit_size>264)
                        dpcm_bs += 1000;
                    if(final_str1_bit_size>256)
                        dpcm_bs += 1000;
                }
                else
                {
                    if(gp1_str0_len > MAX_GP1_STR0_LEN || gp1_str1_len > MAX_GP1_STR1_LEN)
                        dpcm_bs += 1000;
                }
            }
            if (dpcm_bs < best_dpcm)
            {
                best_dpcm = dpcm_bs;
                best_k[gp] = k;
            }
        }
        if (best_dpcm < best_bs[gp])
        {
            best_bs[gp] = best_dpcm;
            best_mode[gp] = DPCM_MODE;
        }

        unsigned int ctx_bs = test_ctx_dpcm_mode_group(chType, encCU, pvbc_meta, gp, false, &stream0_bit_size[10*gp+CTX_MODE],&stream1_bit_size[10*gp+CTX_MODE]);
        if (gp == 1)
        {
            int modebits;
            modebits= get_mode_bs(&bsOut[bsOut_type], best_mode[0], best_k[0], CTX_MODE, 0, chType, false);
            ctx_bs += modebits;
            if(best_mode[0] == DPCM_MODE)
            {
                final_str0_bit_size = stream0_bit_size[10*0+best_mode[0]+best_k[0]] + stream0_bit_size[10*1+CTX_MODE];
                final_str1_bit_size = stream1_bit_size[10*0+best_mode[0]+best_k[0]] + stream1_bit_size[10*1+CTX_MODE];
            }
            else
            {
                final_str0_bit_size = stream0_bit_size[10*0+best_mode[0]] + stream0_bit_size[10*1+CTX_MODE];
                final_str1_bit_size = stream1_bit_size[10*0+best_mode[0]] + stream1_bit_size[10*1+CTX_MODE];
            }
            gp1_str0_len = stream0_bit_size[10*1+CTX_MODE];
            gp1_str1_len = stream1_bit_size[10*1+CTX_MODE];
            if (g_VBCVersion >= 1)
            {
                if(modebits+final_str0_bit_size>264)
                    ctx_bs += 1000;
                if(final_str1_bit_size>256)
                    ctx_bs += 1000;
            }
            else
            {
                if(gp1_str0_len > MAX_GP1_STR0_LEN || gp1_str1_len > MAX_GP1_STR1_LEN)
                    dpcm_bs += 1000;
            }
        }
        if (ctx_bs < best_bs[gp] && !cu_lossy_enable)
        {
            best_bs[gp] = ctx_bs;
            best_mode[gp] = CTX_MODE;
        }

        encCU->mode_len[gp][CTX_MODE] = ctx_bs;

        unsigned int resi_fix_len_bs = test_fix_len_mode_group(encCU, gp, false, &gp_max_resi_bit[gp]);
        if (gp == 1)
            resi_fix_len_bs += get_mode_bs(&bsOut[bsOut_type], best_mode[0], best_k[0], FIXLEN_MODE, 0, chType, false);
        if (resi_fix_len_bs < best_bs[gp] && !cu_lossy_enable)
        {
            best_bs[gp] = resi_fix_len_bs;
            best_mode[gp] = FIXLEN_MODE;
        }
        encCU->mode_len[gp][FIXLEN_MODE] = resi_fix_len_bs;
#ifdef SIG_VBC
        if (g_sigdump.vbc_low && gp == 1)
        {
            sig_vbe_lossy_src(encCU, chType, cu_idx_x, cu_idx_y);
        }
#endif
    }

    if (cu_lossy_enable)
    {
        if ((((best_bs[1] + best_bs[0] + 7) / 8 * 8) >= ((gp_sz[0] + gp_sz[1]) * (8 - lossy_truncate) + 8)))
        {
            best_mode[0] = best_mode[1] = PCM_MODE;
            best_bs[0] = gp_sz[0] * (8 - lossy_truncate);
            best_bs[1] = gp_sz[1] * (8 - lossy_truncate) + 8;
        }
    }
    else
    {
        if (((best_bs[1] + best_bs[0] + 7) / 8) >= (gp_sz[0] + gp_sz[1]))
        {
            best_mode[0] = best_mode[1] = PCM_MODE;
            best_bs[0] = gp_sz[0] * 8;
            best_bs[1] = gp_sz[1] * 8;
        }
    }
    cu_best_bs = best_bs[0] + best_bs[1];
    cu_best_bs = (cu_best_bs + 7) / 8;
    int isR = ((cu_idx_x / 16) & 1);
    for (int gp = 0; gp <= 1; gp++)
    {
#ifdef SIG_VBC
        if (g_sigdump.vbc_low)
        {
            g_sigpool.best_size_HW[isR][gp] = best_bs[gp];
            g_sigpool.best_mode_HW[isR][gp] = best_mode[gp];
            g_sigpool.uncompress[isR][gp] = 0;
            g_sigpool.cu_best_bs[isR] = (cu_best_bs + 7) / 8;
        }
#endif
        if (best_mode[gp] == CTX_MODE)
            test_ctx_dpcm_mode_group(chType, encCU, pvbc_meta, gp, true, &stream0_bit_size[10*gp+CTX_MODE],&stream0_bit_size[10*gp+CTX_MODE]);
        else if (best_mode[gp] == FIXLEN_MODE)
            test_fix_len_mode_group(encCU, gp, true, &gp_max_resi_bit[gp]);
        else if (best_mode[gp] == DPCM_MODE)
        {
            test_fix_dpcm_mode_group(encCU, gp, true, best_k[gp], &stream0_bit_size[10*gp+best_k[gp]+DPCM_MODE],&stream1_bit_size[10*gp+best_k[gp]+DPCM_MODE]);
        }
        else
        {
            writeOutUncompressGroup(encCU, gp, cu_lossy_enable);
        }
    }
#ifdef SIG_VBC
    if (g_sigdump.vbc_low)
    {
        sig_vbe_mode_ctx(encCU, chType, cu_idx_x, cu_idx_y, best_mode,best_k);
        sig_vbe_pu_str(encCU, cu_lossy_enable, 0, 0, cu_idx_x, cu_idx_y);
    }
#endif
#ifdef SIG_VBC
    cal_mean(encCU);
#endif
    if (bsPack)
    {
#ifdef SIG_VBC
        if (g_sigdump.vbc_low)
        {
            if (((cu_idx_x / 16) & 1) == 0)
            {
                g_sigpool.cu_best_bs[0] = cu_best_bs;
                g_sigpool.org_len = org_len;
                sigdump_output_fprint(&g_sigpool.vbe_pack_out_ctx[chType], "# CU (x, y) = (%d, %d)\n", cu_idx_x, cu_idx_y);
                sigdump_output_fprint(&g_sigpool.vbe_pack_out_ctx[chType], "# enc_cu_cflag = %d, %d // Left cu ,Right cu\n", chType, chType);
                sigdump_output_fprint(&g_sigpool.vbe_pack_out_ctx[chType], "# enc_cu_lossy = %d, %d // Left cu ,Right cu\n", cu_lossy_enable, cu_lossy_enable);
                sigdump_output_fprint(&g_sigpool.vbe_pack_out_ctx[chType], "# enc_cu_tb = %d // truncate bits\n", lossy_truncate);
            }
            else
            {
                g_sigpool.cu_best_bs[1] = cu_best_bs;
                sigdump_output_fprint(&g_sigpool.vbe_pack_out_ctx[chType], "# cu_cpx_size = %d %d\n", g_sigpool.cu_best_bs[0], g_sigpool.cu_best_bs[1]);
            }
        }
#endif
        int mode_sz = get_mode_bs(&bsOut[bsOut_type], best_mode[0], best_k[0], best_mode[1], best_k[1], encCU->chType, true);
        writeOutPackBs(&bsOut[bsOut_type], encCU, mode_sz);
#ifdef SIG_VBC
        if (g_sigdump.vbc_low)
        {
            unsigned int org_len1 = bsOut1[chType].getByteStreamLength();
            unsigned int org_len2 = bsOut2[chType].getByteStreamLength();
            int str0_sz, str1_sz;
            get_mode_bs(&bsOut1[chType], best_mode[0], best_k[0], best_mode[1], best_k[1], encCU->chType, true);
            writeOutPackBs_onestream(&bsOut1[chType], &bsOut2[chType], encCU, mode_sz, &str0_sz, &str1_sz);
            sig_vbe_strm_pack_out(encCU, chType, cu_idx_x, cu_idx_y, org_len1, org_len2);
            sig_vbe_bs_ctx(&bsOut[bsOut_type], chType, cu_idx_x, cu_idx_y, cu_best_bs, org_len, str0_sz, str1_sz);
            if (((cu_idx_x / 16) & 1) != 0)
            {
                sig_vbe_pack_out(&bsOut[bsOut_type], encCU, chType);
                sig_vbe_bs_meta_ctx(chType, cu_idx_x, cu_idx_y, cu_best_bs, org_len, str0_sz, str1_sz);
            }
        }
#endif
    }
    acc_bits += cu_best_bs * 8;
    target_bits += 64 * 8 * lossy_cr / 100;
    if (cu_total_cnt % 24 == 23) // meta size add
    {
        acc_bits += 26 * 8;
    }
    cu_total_cnt++;

    return cu_best_bs;
}

bool tileCUIsVaild(int ChType, int x, int y, int width, int height)
{
    if (y < 0 || x < 0)
        return false;
    if (ChType == 0)
        return ((x < width) && (y < height));
    else
        return ((x < width) && (y < (height / 2)));
}

int getPackMetaBsOffset(meta_st *p_in_meta, ChannelType chType, int subBlkX, int subBlkY)
{
    int offset;
    int i;
    int tile_num_subtile[2];
    int subtile_offset_delta[2];
    int tile_delta;

    for (i = 0; i < 2; i++)
    {
        int tile_sub_w = p_in_meta[i].tile_sub_w * 2;
        subtile_offset_delta[i] = (tile_sub_w / 32) * (p_in_meta[i].tile_sub_h / 4) * 12 / 8 + 4;
        tile_num_subtile[i] = p_in_meta[i].tile_w * p_in_meta[i].tile_h / (tile_sub_w * p_in_meta[i].tile_sub_h);
    }

    offset = 0;
    int tileX = (subBlkX / p_in_meta[chType].tile_w);
    int tileY = (subBlkY / p_in_meta[chType].tile_h);
    tile_delta = tile_num_subtile[0] * subtile_offset_delta[0] + tile_num_subtile[1] * subtile_offset_delta[1];
    offset += tileY * p_in_meta[chType].VBCMetaPitch + tileX * tile_delta; // tile offset
    if (chType == CHANNEL_TYPE_CHROMA)
    {
        offset += tile_num_subtile[0] * subtile_offset_delta[0];
        if ((subBlkX / 32) & 1)
            offset += subtile_offset_delta[1];
    }

    return offset;
}

void packMetaBs_tile_32x32(meta_st *p_in_meta, int subBlkX, int subBlkY, int tile_idx, int subtile_idx, unsigned int &offset)
{
    unsigned char *pMetaPack = bsMetaOutPack + offset;
    int w_idx = 0;
    for (int chType = 0; chType < 2; chType++)
    {
        meta_st *pvbc_meta = &p_in_meta[chType];
        tile_meta_st *tile_arry = &pvbc_meta->tile_arry[tile_idx];
        TComVBCOutputBitstream bsSubTile;
        bsSubTile.clear();
        int CuCnt = (pvbc_meta->tile_sub_w / pvbc_meta->cu_w) * (pvbc_meta->tile_sub_h / pvbc_meta->cu_h);
        int cu_idx = 0;
        sub_tile_meta_st *sub_tile_arry = &tile_arry->sub_tile_arry[subtile_idx];
        bsSubTile.write_LSB(sub_tile_arry->address, 32);
        for (int cu_y = 0; cu_y < CuCnt / 2; cu_y++)
        {
            if (((subBlkX / 16) < pvbc_meta->pic_width / 16) && (((subBlkY + cu_y * pvbc_meta->cu_h) / 16) < pvbc_meta->height * (1 + chType) / 16))
            {
                //left cu
                int cu_bs_size_m1_l = max(sub_tile_arry->cu_sz_arry[cu_idx] - 1, 0);
                bsSubTile.write_LSB(cu_bs_size_m1_l, 6);

                //right cu
                int cu_bs_size_m1_r = max(sub_tile_arry->cu_sz_arry[cu_idx + 1] - 1, 0);
                bsSubTile.write_LSB(cu_bs_size_m1_r, 6);
                sig_ctx *vbc_dma_cu_info = &g_sigpool.vbc_dma_cu_info[chType];
                sigdump_output_fprint(vbc_dma_cu_info, "# CU (x,y) = (%d,%d)\n",
                                    0 + subBlkX / 16, cu_y + subBlkY / 4);
                // sigdump_output_fprint(vbc_dma_cu_info, "# CU (x,y) = (%d,%d)\n",0 , cu_y);
                sigdump_output_fprint(vbc_dma_cu_info, "# meta_eu_start_pos = %08x\n", offset);
                sigdump_output_fprint(vbc_dma_cu_info, "# meta data = %02x %02x\n", cu_bs_size_m1_l, cu_bs_size_m1_r);
                sigdump_output_fprint(vbc_dma_cu_info, "# cpx_eu_start_pos = %08x\n", sub_tile_arry->address);
            }
            else
            {
                bsSubTile.write_LSB(0, 6);
                bsSubTile.write_LSB(0, 6);
            }
            cu_idx += 2;
        }

        for (auto it = bsSubTile.getFifo().begin(); it != bsSubTile.getFifo().end(); it++)
        {
            pMetaPack[w_idx++] = *it;
        }
        offset += bsSubTile.getFifo().size();
    }
}

void packMetaBs(meta_st *p_in_meta, ChannelType chType, tile_meta_st *tile_arry, int subBlkX, int subBlkY)
{
    meta_st *pvbc_meta = &p_in_meta[chType];
    int offset = getPackMetaBsOffset(p_in_meta, chType, subBlkX, subBlkY + pvbc_meta->shift_pix);
    int i = 0;
    unsigned char *pMetaPack = bsMetaOutPack + offset;
    TComVBCOutputBitstream bsSubTile;
    bsSubTile.clear();
    int CuCnt = chType == CHANNEL_TYPE_LUMA ? 16 : 8;
    int start_offset = tile_arry->sub_tile_arry[0].address;
    bsSubTile.write_LSB(start_offset, 32);
    for (int cu_tile_x = 0; cu_tile_x < 2; cu_tile_x++)
    {
        int cu_idx = 0;
        sub_tile_meta_st *sub_tile_arry = &tile_arry->sub_tile_arry[cu_tile_x];
        for (int cu_y = 0; cu_y < CuCnt / 4; cu_y++)
        {

            int cu_bs_size_m1_l = max(sub_tile_arry->cu_sz_arry[cu_idx] - 1, 0);
            bsSubTile.write_LSB(cu_bs_size_m1_l, 6);

            if ((cu_tile_x * 2 + subBlkX / 16) < pvbc_meta->pic_width / 16)
            {
                int cu_bs_size_m1_r = max(sub_tile_arry->cu_sz_arry[cu_idx + 1] - 1, 0);
                bsSubTile.write_LSB(cu_bs_size_m1_r, 6);
                sig_ctx *vbc_dma_cu_info = &g_sigpool.vbc_dma_cu_info[chType];
                sigdump_output_fprint(vbc_dma_cu_info, "# CU (x,y) = (%d,%d)\n",
                                    0 + cu_tile_x * 2 + subBlkX / 16, cu_y + subBlkY / 4);
                // sigdump_output_fprint(vbc_dma_cu_info, "# CU (x,y) = (%d,%d)\n",0 , cu_y);
                sigdump_output_fprint(vbc_dma_cu_info, "# meta_eu_start_pos = %08x\n", offset);
                sigdump_output_fprint(vbc_dma_cu_info, "# meta data = %02x %02x\n", cu_bs_size_m1_l, cu_bs_size_m1_r);
                sigdump_output_fprint(vbc_dma_cu_info, "# cpx_eu_start_pos = %08x\n", start_offset);
            }
            else
            {
                bsSubTile.write_LSB(0, 6);
            }
            cu_idx += 2;
        }
    }

    for (auto it = bsSubTile.getFifo().begin(); it != bsSubTile.getFifo().end(); it++)
    {
        pMetaPack[i] = *it;
        i++;
    }
}

void vbc_encode_NV12(meta_st *p_in_meta, int tile_x, int tile_y, int tile_idx, TComPicYuv *src)
{
    int channel = 1;
    int sub_tile_idx = 0;
    int cu_idx = 0;
    int cusize[2][4] = {0};
    sub_tile_idx = 0;
    /* subtile 64 x 16, cu 16x4
    0  1  8  9
    2  3 10 11
    4  5 12 13
    6  7 14 15
    */

    if (g_sigdump.vbc_dbg)
    {
        sigdump_output_fprint(&g_sigpool.vbe_meta_debug_ctx[0], "# tile %d, (x,y) = (%d,%d)\n", tile_idx, tile_x, tile_y);
    }
    if ((tile_y / p_in_meta[0].tile_sub_h) & 1)
        channel = 2;
    int start_offset = bsOut[0].getByteStreamLength();
    int cu_sz_cnt[2] = {0};
    for (UInt sub_y = 0; sub_y < p_in_meta[0].tile_h; sub_y += p_in_meta[0].tile_sub_h)
    {
        for (UInt sub_x = 0; sub_x < p_in_meta[0].tile_w; sub_x += p_in_meta[0].tile_sub_w, sub_tile_idx++)
        {
            for (int chType = 0; chType < channel; chType++)
            {
                meta_st *pvbc_meta = &p_in_meta[0];
                pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].address = pvbc_meta->offset;
                cu_idx = 0;
                UInt cu_x, cu_y;
                if (sub_x == 0 && chType == 0)
                {
                    for (int i = 0; i < 3 * channel; i++)
                    {
                        bsOut[0].write(0, 8);
                        bsOut[1].write(0, 8);
                    }
                    //start_offset = subTileBlk.pMeta->start_offset;
                }
                for (cu_x = 0; cu_x < pvbc_meta->tile_sub_w; cu_x += (2 * pvbc_meta->cu_w))
                {
                    for (cu_y = 0; cu_y < pvbc_meta->tile_sub_h; cu_y += pvbc_meta->cu_h)
                    {
                        // 16x4 pair
                        for (int i = 0; i < 2; i++)
                        {
                            unsigned int org_len = bsOut[0].getByteStreamLength();
                            int cu_idx_x = cu_x + i * pvbc_meta->cu_w + sub_x + tile_x;
                            int cu_idx_y = cu_y + sub_y + tile_y;
                            if (tileCUIsVaild(chType, cu_idx_x, cu_idx_y, pvbc_meta->pic_width, pvbc_meta->pic_height))
                            {
                                int size = 0;
                                encCU_st encCU;
                                if (chType == 0)
                                {
                                    init_cu(cu_idx_x, cu_idx_y, src, COMPONENT_Y, &encCU, pvbc_meta);
                                    size = compressOneCU(pvbc_meta, &encCU, chType, cu_idx_x, cu_idx_y);
                                }
                                else
                                {
                                    if (i == 0)
                                    {
                                        init_cu(cu_idx_x / 2, cu_idx_y, src, COMPONENT_Cb, &encCU, pvbc_meta);
                                        size = compressOneCU(pvbc_meta, &encCU, chType, cu_idx_x, cu_idx_y);
                                    }
                                    else
                                    {
                                        init_cu((cu_idx_x - pvbc_meta->cu_w) / 2, cu_idx_y, src, COMPONENT_Cr, &encCU, pvbc_meta);
                                        size = compressOneCU(pvbc_meta, &encCU, chType, cu_idx_x, cu_idx_y);
                                    }
                                }
                                if (g_sigdump.vbc_low)
                                    sig_vbe_rec(&encCU, chType, cu_idx_x, cu_idx_y);
                                cusize[chType][cu_sz_cnt[chType]++] = size - 1;
                                pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = size;
                                if (g_sigdump.vbc_low)
                                {
                                    sigdump_output_fprint(&g_sigpool.vbe_enc_info_ctx, "[%s]cu %4d, %4d, size = %2d, offset %d,\n",
                                                          encCU.chType == 0 ? "Y" : "C", cu_idx_x, cu_idx_y, size, org_len);
                                }
                                if (g_sigdump.vbc_dbg)
                                {
                                    sigdump_output_fprint(&g_sigpool.vbe_meta_debug_ctx[chType], "# CU (x,y) = (%d,%d)\n", cu_idx_x, cu_idx_y);
                                }
                                pvbc_meta->offset = bsOut[0].getByteStreamLength();
                                del_cu(&encCU);
                            }
                            else if (cu_idx_y < 0)
                            {
                                pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = 0;
                            }
                        }
                    }
                }
            } // chType
        }
    }
    int align_res = bsOut[0].getByteStreamLength() % PitchAlignSize;
    if (align_res && (tile_x / p_in_meta[0].tile_w) == (p_in_meta[0].tile_cnt_x - 1))
    {
        for (int i = 0; i < PitchAlignSize - align_res; i++)
        {
            bsOut[0].write(0, 8);
        }
    }
    //packMetaBs(p_in_meta, (ChannelType)0, &p_in_meta[0].tile_arry[tile_idx], tile_x, tile_y);

    {
        TComVBCOutputBitstream bsSubTile;
        bsSubTile.clear();
        int i = 0;
        unsigned char *pMetaPack;

        if (tile_x == 0)
        {
            pMetaPack = bsMetaOutPack + tile_y; // byte for each row
            bsSubTile.write_LSB(start_offset, 32);
            for (auto it = bsSubTile.getFifo().begin(); it != bsSubTile.getFifo().end(); it++)
            {
                pMetaPack[i] = *it;
                i++;
            }
        }

        int rowsize;
        rowsize = p_in_meta[0].tile_cnt_x * 3 * 2; // tile_cnt_x* 3 byte * 2 channel
        pMetaPack = bsMetaOutPack + p_in_meta[0].height / 4 * p_in_meta[0].tile_sub_h;
        pMetaPack += (3 * channel) * tile_x / p_in_meta[0].tile_w + tile_y / p_in_meta[0].tile_h * rowsize;

        bsSubTile.clear();
        i = 0;
        for (int chType = 0; chType < channel; chType++)
        {
            for (int cu_idx = 0; cu_idx < 4; cu_idx++)
            {
                bsSubTile.write_LSB(cusize[chType][cu_idx], 6);
            }
        }
        for (auto it = bsSubTile.getFifo().begin(); it != bsSubTile.getFifo().end(); it++)
        {
            pMetaPack[i] = *it;
            i++;
        }

        for (i = 0; i < 3 * channel; i++)
        {
            bsOut[0].m_fifo[start_offset + i] = pMetaPack[i];
        }
    }
}

void vbc_encode_luma_subtile(meta_st *p_in_meta, int tile_sub_x, int tile_sub_y, int sub_tile_idx, int tile_idx, TComPicYuv *src)
{
    int chType = 0;
    meta_st *pvbc_meta = &p_in_meta[chType];
    int cu_idx = 0;

    /* subtile 64 x 16, cu 16x4
    0  1  8  9
    2  3 10 11
    4  5 12 13
    6  7 14 15
    */
    int cu_y_order[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    if (pvbc_meta->shift_pix == 0)
    {
        cu_y_order[0] = 0;
        cu_y_order[1] = 3;
        cu_y_order[2] = 1;
        cu_y_order[3] = 2;
        cu_y_order[4] = 4 + 0;
        cu_y_order[5] = 4 + 3;
        cu_y_order[6] = 4 + 1;
        cu_y_order[7] = 4 + 2;
    }

    if (g_sigdump.vbc_dbg)
    {
        sigdump_output_fprint(&g_sigpool.vbe_meta_debug_ctx[chType], "# tile %d, sub tile %d, (x,y) = (%d,%d), address = 0x%x\n", tile_idx, sub_tile_idx, tile_sub_x, tile_sub_y, pvbc_meta->offset);
    }

    pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].address = pvbc_meta->offset;
    cu_idx = 0;
    UInt cu_x, cu_y;

    for (cu_y = 0; cu_y < pvbc_meta->tile_sub_h; cu_y += pvbc_meta->cu_h)
    {
        int modify_cu_y;
        if ((tile_sub_x + pvbc_meta->tile_sub_w) >= pvbc_meta->pic_width)
            modify_cu_y = cu_y;
        else
            modify_cu_y = cu_y_order[cu_y / pvbc_meta->cu_h] * pvbc_meta->cu_h;
        for (cu_x = 0; cu_x < pvbc_meta->tile_sub_w; cu_x+=pvbc_meta->cu_w)
        {
            int cu_idx_x = cu_x + tile_sub_x;
            int cu_idx_y = modify_cu_y + tile_sub_y;
            if (tileCUIsVaild(0, cu_idx_x, cu_idx_y, pvbc_meta->pic_width, pvbc_meta->height))
            {
                int size = 0;
                encCU_st encCU;
                init_cu(cu_idx_x, cu_idx_y, src, COMPONENT_Y, &encCU, pvbc_meta);
                size = compressOneCU(pvbc_meta, &encCU, chType, cu_idx_x, cu_idx_y);
                if (g_sigdump.vbc_low)
                    sig_vbe_rec(&encCU, chType, cu_idx_x, cu_idx_y);
                pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = size;
                if (g_sigdump.vbc_low)
                {
                    sigdump_output_fprint(&g_sigpool.vbe_enc_info_ctx, "[%s]cu %4d, %4d, size = %2d, offset %d,\n",
                                            encCU.chType == 0 ? "Y" : "C", cu_idx_x, cu_idx_y, size, pvbc_meta->offset);
                }
                if (g_sigdump.vbc_dbg)
                {
                    sigdump_output_fprint(&g_sigpool.vbe_meta_debug_ctx[chType], "# CU (x,y) = (%d,%d), size = 0x%x, offset 0x%x,\n", cu_idx_x, cu_idx_y, size, pvbc_meta->offset);
                }
                pvbc_meta->offset += size;
                del_cu(&encCU);
            }
            else
            {
                pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = 0;
            }
        }
    }
    //packMetaBs(p_in_meta, CHANNEL_TYPE_LUMA, &pvbc_meta->tile_arry[tile_idx], tile_sub_x, tile_sub_y);
    pvbc_meta->total_compress_size = pvbc_meta->offset;
}

void vbc_encode_luma(meta_st *p_in_meta, int tile_x, int tile_y, int tile_idx, TComPicYuv *src)
{
    int chType = 0;
    meta_st *pvbc_meta = &p_in_meta[chType];
    int sub_tile_idx = 0;
    int cu_idx = 0;

    sub_tile_idx = 0;
    /* subtile 64 x 16, cu 16x4
    0  1  8  9
    2  3 10 11
    4  5 12 13
    6  7 14 15
    */
    int cu_y_order[4] = {0, 1, 2, 3};
    if (pvbc_meta->shift_pix == 0)
    {
        cu_y_order[1] = 3;
        cu_y_order[2] = 1;
        cu_y_order[3] = 2;
    }
    if (g_sigdump.vbc_dbg)
    {
        sigdump_output_fprint(&g_sigpool.vbe_meta_debug_ctx[chType], "# tile %d, (x,y) = (%d,%d)\n", tile_idx, tile_x, tile_y);
    }

    for (UInt sub_y = 0; sub_y < pvbc_meta->tile_h; sub_y += pvbc_meta->tile_sub_h)
    {
        for (UInt sub_x = 0; sub_x < pvbc_meta->tile_w; sub_x += pvbc_meta->tile_sub_w, sub_tile_idx++)
        {
            pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].address = pvbc_meta->offset;
            cu_idx = 0;
            UInt cu_x, cu_y;
            for (cu_x = 0; cu_x < pvbc_meta->tile_sub_w; cu_x += (2 * pvbc_meta->cu_w))
            {
                for (cu_y = 0; cu_y < pvbc_meta->tile_sub_h; cu_y += pvbc_meta->cu_h)
                {
                    int modify_cu_y;
                    if ((cu_x + sub_x + tile_x + pvbc_meta->tile_sub_w) >= pvbc_meta->pic_width)
                        modify_cu_y = cu_y;
                    else
                        modify_cu_y = cu_y_order[cu_y / pvbc_meta->cu_h] * pvbc_meta->cu_h;
                    for (int i = 0; i < 2; i++)
                    {
                        int cu_idx_x = cu_x + i * pvbc_meta->cu_w + sub_x + tile_x;
                        int cu_idx_y = modify_cu_y + sub_y + tile_y;
                        if (tileCUIsVaild(0, cu_idx_x, cu_idx_y, pvbc_meta->pic_width, pvbc_meta->height))
                        {
                            int size = 0;
                            encCU_st encCU;
                            init_cu(cu_idx_x, cu_idx_y, src, COMPONENT_Y, &encCU, pvbc_meta);
                            size = compressOneCU(pvbc_meta, &encCU, chType, cu_idx_x, cu_idx_y);
                            if (g_sigdump.vbc_low)
                                sig_vbe_rec(&encCU, chType, cu_idx_x, cu_idx_y);
                            pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = size;
                            if (g_sigdump.vbc_low)
                            {
                                sigdump_output_fprint(&g_sigpool.vbe_enc_info_ctx, "[%s]cu %4d, %4d, size = %2d, offset %d,\n",
                                                      encCU.chType == 0 ? "Y" : "C", cu_idx_x, cu_idx_y, size, pvbc_meta->offset);
                            }
                            if (g_sigdump.vbc_dbg)
                            {
                                sigdump_output_fprint(&g_sigpool.vbe_meta_debug_ctx[chType], "# CU (x,y) = (%d,%d)\n", cu_idx_x, cu_idx_y);
                            }
                            pvbc_meta->offset += size;
                            del_cu(&encCU);
                        }
                        else
                        {
                            pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = 0;
                        }
                    }
                }
            }
        }
        packMetaBs(p_in_meta, CHANNEL_TYPE_LUMA, &pvbc_meta->tile_arry[tile_idx], tile_x, tile_y);
    }
    pvbc_meta->total_compress_size = pvbc_meta->offset;
}

void vbc_encode_chroma_subtile(meta_st *p_in_meta, int tile_sub_x, int tile_sub_y, int sub_tile_idx, int tile_idx, TComPicYuv *src)
{
    int chType = 1;
    meta_st *pvbc_meta = &p_in_meta[chType];

    int cu_idx = 0;

    if (g_sigdump.vbc_dbg)
    {
        sigdump_output_fprint(&g_sigpool.vbe_meta_debug_ctx[chType], "# tile %d, sub tile %d, (x,y) = (%d,%d)\n", tile_idx, sub_tile_idx, tile_sub_x, tile_sub_y);
    }

    UInt cu_y;
    pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].address = pvbc_meta->offset;
    cu_idx = 0;

    for (cu_y = 0; cu_y < pvbc_meta->tile_sub_h; cu_y += pvbc_meta->cu_h)
    {
        int cu_idx_x = tile_sub_x;
        int cu_idx_y = cu_y + tile_sub_y;
        if (tileCUIsVaild(0, cu_idx_x, cu_idx_y, pvbc_meta->pic_width, pvbc_meta->height))
        {
            encCU_st encCU;
            init_cu(cu_idx_x / 2, cu_idx_y, src, COMPONENT_Cb, &encCU, pvbc_meta);
            int size_cb = compressOneCU(pvbc_meta, &encCU, chType, cu_idx_x, cu_idx_y);
            if (g_sigdump.vbc_low)
                sig_vbe_rec(&encCU, chType, cu_idx_x, cu_idx_y);
            del_cu(&encCU);
            init_cu(cu_idx_x / 2, cu_idx_y, src, COMPONENT_Cr, &encCU, pvbc_meta);
            int size_cr = compressOneCU(pvbc_meta, &encCU, chType, cu_idx_x + 16, cu_idx_y);
            pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = size_cb;
            pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = size_cr;
            if (g_sigdump.vbc_low)
            {
                sig_vbe_rec(&encCU, chType, cu_idx_x + 16, cu_idx_y);
                sigdump_output_fprint(&g_sigpool.vbe_enc_info_ctx, "[%s]cu %4d, %4d, size = %2d, offset %d,\n",
                                        encCU.chType == 0 ? "Y" : "C", cu_idx_x, cu_idx_y, size_cb, pvbc_meta->offset);
                sigdump_output_fprint(&g_sigpool.vbe_enc_info_ctx, "[%s]cu %4d, %4d, size = %2d, offset %d,\n",
                                        encCU.chType == 0 ? "Y" : "C", cu_idx_x + pvbc_meta->cu_w, cu_idx_y, size_cr, pvbc_meta->offset + size_cb);
            }
            if (g_sigdump.vbc_dbg)
            {
                sigdump_output_fprint(&g_sigpool.vbe_meta_debug_ctx[chType], "# CU (x,y) = (%d,%d) (%d,%d)\n", cu_idx_x, cu_idx_y, cu_idx_x + pvbc_meta->cu_w, cu_idx_y);
            }
            pvbc_meta->offset += size_cb;
            pvbc_meta->offset += size_cr;
            del_cu(&encCU);
        }
        else
        {
            pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = 0;
            pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = 0;
        }
    }

    pvbc_meta->total_compress_size = pvbc_meta->offset;
}

void vbc_encode_chroma(meta_st *p_in_meta, int tile_x, int tile_y, int tile_idx, TComPicYuv *src)
{
    int chType = 1;
    meta_st *pvbc_meta = &p_in_meta[chType];
    int sub_tile_idx = 0;
    int cu_idx = 0;

    if (g_sigdump.vbc_dbg)
    {
        sigdump_output_fprint(&g_sigpool.vbe_meta_debug_ctx[chType], "# tile %d, (x,y) = (%d,%d)\n", tile_idx, tile_x, tile_y);
    }
    sub_tile_idx = 0;
    for (UInt sub_y = 0; sub_y < pvbc_meta->tile_h; sub_y += pvbc_meta->tile_sub_h)
    {
        for (UInt sub_x = 0; sub_x < pvbc_meta->tile_w; sub_x += pvbc_meta->tile_sub_w, sub_tile_idx++)
        {
            UInt cu_x, cu_y;
            pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].address = pvbc_meta->offset;
            cu_idx = 0;
            for (cu_x = 0; cu_x < pvbc_meta->tile_sub_w / 2; cu_x += pvbc_meta->cu_w)
            {
                for (cu_y = 0; cu_y < pvbc_meta->tile_sub_h; cu_y += pvbc_meta->cu_h)
                {
                    int cu_idx_x = cu_x + sub_x + tile_x;
                    int cu_idx_y = cu_y + sub_y + tile_y;
                    if (tileCUIsVaild(0, cu_idx_x, cu_idx_y, pvbc_meta->pic_width, pvbc_meta->height))
                    {
                        encCU_st encCU;
                        init_cu(cu_idx_x / 2, cu_idx_y, src, COMPONENT_Cb, &encCU, pvbc_meta);
                        int size_cb = compressOneCU(pvbc_meta, &encCU, chType, cu_idx_x, cu_idx_y);
                        if (g_sigdump.vbc_low)
                            sig_vbe_rec(&encCU, chType, cu_idx_x, cu_idx_y);
                        del_cu(&encCU);
                        init_cu(cu_idx_x / 2, cu_idx_y, src, COMPONENT_Cr, &encCU, pvbc_meta);
                        int size_cr = compressOneCU(pvbc_meta, &encCU, chType, cu_idx_x + 16, cu_idx_y);
                        pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = size_cb;
                        pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = size_cr;
                        if (g_sigdump.vbc_low)
                        {
                            sig_vbe_rec(&encCU, chType, cu_idx_x + 16, cu_idx_y);
                            sigdump_output_fprint(&g_sigpool.vbe_enc_info_ctx, "[%s]cu %4d, %4d, size = %2d, offset %d,\n",
                                                  encCU.chType == 0 ? "Y" : "C", cu_idx_x, cu_idx_y, size_cb, pvbc_meta->offset);
                            sigdump_output_fprint(&g_sigpool.vbe_enc_info_ctx, "[%s]cu %4d, %4d, size = %2d, offset %d,\n",
                                                  encCU.chType == 0 ? "Y" : "C", cu_idx_x + pvbc_meta->cu_w, cu_idx_y, size_cr, pvbc_meta->offset + size_cb);
                        }
                        if (g_sigdump.vbc_dbg)
                        {
                            sigdump_output_fprint(&g_sigpool.vbe_meta_debug_ctx[chType], "# CU (x,y) = (%d,%d) (%d,%d)\n", cu_idx_x, cu_idx_y, cu_idx_x + pvbc_meta->cu_w, cu_idx_y);
                        }
                        pvbc_meta->offset += size_cb;
                        pvbc_meta->offset += size_cr;
                        del_cu(&encCU);
                    }
                    else
                    {
                        pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = 0;
                        pvbc_meta->tile_arry[tile_idx].sub_tile_arry[sub_tile_idx].cu_sz_arry[cu_idx++] = 0;
                    }
                }
            }
        }
        packMetaBs(p_in_meta, CHANNEL_TYPE_CHROMA, &pvbc_meta->tile_arry[tile_idx], tile_x, tile_y);
    }

    pvbc_meta->total_compress_size = pvbc_meta->offset;
}

void vbc_encode_frame(meta_st *p_in_meta, TComPicYuv *src, int frm_cnt, int cfg_lossy, int cfg_lossy_cr, int cfg_lossy_tolerence, int cfg_lossy_delay)
{
#ifdef CVI_VBC_LOSSY
    lossy = cfg_lossy;
    lossy_cr = cfg_lossy_cr;
    lossy_tolerence = cfg_lossy_tolerence;
    lossy_delay = cfg_lossy_delay;
    lossy_truncate = (int)(8 - 8 * lossy_cr / 100);
#endif
    acc_bits = 0, target_bits = 0;
    meta_st *pvbc_meta = &p_in_meta[0];
    int tile_idx = 0;
    cu_total_cnt = 0;
    for (int chType = 0; chType < 2; chType++)
    {
        bsOut[chType].clear();
        bsOut1[chType].clear();
        bsOut2[chType].clear();
        bsmetacpx[chType].clear();
        bsmetacpx_reorder[chType].clear();
    }

    MetaOutPack_size = g_vbc_meta[0].VBCMetaPitch * g_vbc_meta[0].tile_cnt_y;
    bsMetaOutPack = (unsigned char *) malloc(MetaOutPack_size);
    memset(bsMetaOutPack, 0, MetaOutPack_size);

    p_in_meta[0].offset = p_in_meta[1].offset = 0;

    int x, y;
    if (g_VBCVersion == 0)
    {
        for (y = (-pvbc_meta->shift_pix); y < pvbc_meta->height; y += (4 * pvbc_meta->tile_h))
        {
            //last two rows
            if ((pvbc_meta->shift_pix != 0) && ((y + 4 * pvbc_meta->tile_h + pvbc_meta->shift_pix) == pvbc_meta->height) && ((pvbc_meta->height % (4 * pvbc_meta->tile_h)) == 0))
                break;
            for (x = 0; x < pvbc_meta->width; x += pvbc_meta->tile_w)
            {
                for (int sub_y = y, cnt = 0; (sub_y < pvbc_meta->height) && (cnt < 4); sub_y += pvbc_meta->tile_h, cnt++)
                {
                    vbc_encode_luma(p_in_meta, x, sub_y, tile_idx, src);
                    vbc_encode_chroma(p_in_meta, x, (sub_y + pvbc_meta->shift_pix) / 2 - pvbc_meta->shift_pix, tile_idx, src);
                    tile_idx++;
                }
            }
        }

        //combine last two row: 64x16 + 64x4.
        if ((pvbc_meta->shift_pix != 0) && (pvbc_meta->height % (4 * pvbc_meta->tile_h)) == 0)
        {
            for (x = 0; x < pvbc_meta->width; x += pvbc_meta->tile_w)
            {
                for (int last_y = y; last_y < pvbc_meta->height; last_y += pvbc_meta->tile_h)
                {
                    vbc_encode_luma(p_in_meta, x, last_y, tile_idx, src);
                    vbc_encode_chroma(p_in_meta, x, (last_y + pvbc_meta->shift_pix) / 2 - pvbc_meta->shift_pix, tile_idx, src);
                    tile_idx++;
                }
            }
        }
    }
    else if (g_VBCVersion >= 1)
    {
        for (y = (-pvbc_meta->shift_pix); y < pvbc_meta->height; y +=  pvbc_meta->tile_h)
        {
            x = 0;
            int sub_y;
            int cnt;
            for (sub_y = y, cnt = 0; ((sub_y + pvbc_meta->shift_pix) < pvbc_meta->height) && (cnt < 2); sub_y += pvbc_meta->tile_sub_h, cnt++)
            {
                tile_idx = (x / pvbc_meta->tile_w) + (pvbc_meta->tile_cnt_x * ((sub_y + pvbc_meta->shift_pix) / pvbc_meta->tile_h));
                vbc_encode_luma_subtile(p_in_meta, x, sub_y, cnt, tile_idx, src);
                vbc_encode_chroma_subtile(p_in_meta, x, (sub_y + pvbc_meta->shift_pix) / 2 - pvbc_meta->shift_pix, cnt, tile_idx, src);
            }
            //last 4 bytes
            if ((pvbc_meta->shift_pix != 0) && ((sub_y + pvbc_meta->shift_pix) == pvbc_meta->height))
            {
                tile_idx = (x / pvbc_meta->tile_w) + (pvbc_meta->tile_cnt_x * ((sub_y + pvbc_meta->shift_pix) / pvbc_meta->tile_h));
                vbc_encode_luma_subtile(p_in_meta, x, sub_y, cnt == 1 ? 1 : 0, tile_idx, src);
                vbc_encode_chroma_subtile(p_in_meta, x, (sub_y + pvbc_meta->shift_pix) / 2 - pvbc_meta->shift_pix, cnt == 1 ? 1 : 0, tile_idx, src);
            }
            x += pvbc_meta->tile_sub_w;
            for (;x < pvbc_meta->pic_width; x += pvbc_meta->tile_w)
            {
                for (sub_y = y, cnt = 0; ((sub_y + pvbc_meta->shift_pix) < pvbc_meta->height) && (cnt < 2); sub_y += pvbc_meta->tile_sub_h, cnt++)
                {
                    int max_x;
                    if ((x + pvbc_meta->tile_w + pvbc_meta->tile_sub_w) == pvbc_meta->pic_width)
                        max_x = pvbc_meta->pic_width;
                    else
                        max_x = min(x + pvbc_meta->tile_w, pvbc_meta->pic_width);

                    int last_y = sub_y + pvbc_meta->tile_sub_h;
                    for (int sub_x = x;sub_x < max_x; sub_x += pvbc_meta->tile_sub_w)
                    {
                        int subtile_idx = (((sub_x % pvbc_meta->tile_w) / pvbc_meta->tile_sub_w) % pvbc_meta->tile_sub_cnt_x)
                            + (pvbc_meta->tile_sub_cnt_x * (((sub_y + pvbc_meta->shift_pix) % pvbc_meta->tile_h) / pvbc_meta->tile_sub_h));
                        if (subtile_idx == 1)
                            subtile_idx = 2;
                        else if (subtile_idx ==2)
                            subtile_idx = 1;
                        tile_idx = (sub_x / pvbc_meta->tile_w) + (pvbc_meta->tile_cnt_x * ((sub_y + pvbc_meta->shift_pix) / pvbc_meta->tile_h));
                        vbc_encode_luma_subtile(p_in_meta, sub_x, sub_y, subtile_idx, tile_idx, src);
                        vbc_encode_chroma_subtile(p_in_meta, sub_x, (sub_y + pvbc_meta->shift_pix) / 2 - pvbc_meta->shift_pix, subtile_idx, tile_idx, src);

                        //last 4 bytes
                        if ((pvbc_meta->shift_pix != 0) && ((last_y + pvbc_meta->shift_pix) == pvbc_meta->height))
                        {
                            subtile_idx = (((sub_x % pvbc_meta->tile_w) / pvbc_meta->tile_sub_w) % pvbc_meta->tile_sub_cnt_x)
                                + (pvbc_meta->tile_sub_cnt_x * (((last_y + pvbc_meta->shift_pix) % pvbc_meta->tile_h) / pvbc_meta->tile_sub_h));
                            if (subtile_idx == 1)
                                subtile_idx = 2;
                            else if (subtile_idx ==2)
                                subtile_idx = 1;
                            tile_idx = (sub_x / pvbc_meta->tile_w) + (pvbc_meta->tile_cnt_x * ((last_y + pvbc_meta->shift_pix) / pvbc_meta->tile_h));
                            vbc_encode_luma_subtile(p_in_meta, sub_x, last_y, subtile_idx, tile_idx, src);
                            vbc_encode_chroma_subtile(p_in_meta, sub_x, (last_y + pvbc_meta->shift_pix) / 2 - pvbc_meta->shift_pix, subtile_idx, tile_idx, src);
                        }
                    }
                }
                if ((x + pvbc_meta->tile_w + pvbc_meta->tile_sub_w) == pvbc_meta->pic_width)
                    x += pvbc_meta->tile_sub_w;
            }
            if ((pvbc_meta->shift_pix != 0) && ((y + pvbc_meta->tile_h + pvbc_meta->shift_pix) == pvbc_meta->height))
                break;
        }

        //pack meta
        tile_idx = 0;
        for (int tile_y = 0; tile_y < pvbc_meta->tile_cnt_y; tile_y++)
        {
            unsigned int offset = g_vbc_meta[0].VBCMetaPitch * tile_y;
            for (int tile_x = 0; tile_x < pvbc_meta->tile_cnt_x; tile_x++)
            {
                int subtile_idx = 0;
                for (int sub_x = 0; sub_x < pvbc_meta->tile_sub_cnt_x; sub_x++)
                {
                    for (int sub_y = 0; sub_y < pvbc_meta->tile_sub_cnt_y; sub_y++)
                    {
                        int subBlkX = sub_x * pvbc_meta->tile_sub_w + tile_x * pvbc_meta->tile_w;
                        int subBlkY = sub_y * pvbc_meta->tile_sub_h + tile_y * pvbc_meta->tile_h - pvbc_meta->shift_pix;
                        if (subBlkY < p_in_meta->height)
                        {
                            packMetaBs_tile_32x32(p_in_meta, subBlkX, subBlkY, tile_idx, subtile_idx, offset);
                            assert(offset <= MetaOutPack_size);
                        }
                        subtile_idx++;
                    }
                }
                tile_idx++;
            }
        }
    }
#ifdef SIG_VBC
    if (g_sigdump.vbc)
    {
        sigdump_output_bin(&g_sigpool.vbd_dma_meta, bsMetaOutPack, MetaOutPack_size);
        unsigned char tmp_buf[32] = {0};
        for (int ChType = 0; ChType < 2; ChType++)
        {
            sigdump_output_bin(&g_sigpool.vbd_dma_bs[ChType], (unsigned char *)bsOut[ChType].getByteStream(), bsOut[ChType].getByteStreamLength());
            if ((bsOut[ChType].getByteStreamLength() % 32) != 0)
                sigdump_output_bin(&g_sigpool.vbd_dma_bs[ChType], tmp_buf, (32 - (bsOut[ChType].getByteStreamLength() % 32)));
        }
    }
    if(g_sigdump.vbc_low)
    {
        for (int chType = 0; chType < 2; chType++)
        {
            sig_ctx *meta_cpx_fw = &g_sigpool.vbd_dma_meta_cpx[chType];
            int frame_size = bsmetacpx[chType].getFifo().size();
            unsigned char *reorder_meta_cpx_frame = new unsigned char[frame_size]();

            for (auto it = bsmetacpx[chType].getFifo().begin(); it != bsmetacpx[chType].getFifo().end(); it+=144)
            {
                int x = *(it + 0)+(*(it + 1)<<8);
                int y = *(it + 2)+(*(it + 3)<<8);
                unsigned char *out_ptr = reorder_meta_cpx_frame+(x/32*144)+(y/4*pvbc_meta->pic_width/32*144);
                std::copy( it, it + 144, out_ptr );
            }

            sigdump_output_bin(meta_cpx_fw, reorder_meta_cpx_frame, frame_size);
            delete [] reorder_meta_cpx_frame;
        }
    }
#endif

    double ratio[2];

    for (int ChType = 0; ChType < 2; ChType++)
    {
        meta_st *pvbc_meta = &p_in_meta[ChType];
        unsigned int pic_size = pvbc_meta->pic_width * pvbc_meta->pic_height;
        ratio[ChType] = (double)(pvbc_meta->total_compress_size + pvbc_meta->meta_size) / pic_size;
    }
    printf("\n[%d]YCR, %.3f, CCR, %.3f", frm_cnt, ratio[0], ratio[1]);

    if (bsMetaOutPack)
        free(bsMetaOutPack);
}

void vbc_encode_nv12_frame(meta_st *p_in_meta, TComPicYuv *src, int frm_cnt, int cfg_lossy, int cfg_lossy_cr, int cfg_lossy_tolerence, int cfg_lossy_delay)
{
    bool org_vbc = g_sigdump.vbc;
    bool org_vbc_dbg = g_sigdump.vbc_dbg;
    g_sigdump.vbc = g_sigdump.vbc_dbg = false;
#ifdef CVI_VBC_LOSSY
    lossy = cfg_lossy;
    lossy_cr = cfg_lossy_cr;
    lossy_tolerence = cfg_lossy_tolerence;
    lossy_delay = cfg_lossy_delay;
    lossy_truncate = (int)(8 - 8 * lossy_cr / 100);
#endif
    acc_bits = 0, target_bits = 0;
    meta_st *pvbc_meta = &p_in_meta[0];
    int tile_idx = 0;
    cu_total_cnt = 0;
    for (int chType = 0; chType < 2; chType++)
    {
        bsOut[chType].clear();
        bsOut1[chType].clear();
        bsOut2[chType].clear();
    }
    for (int i = 0; i < (pvbc_meta->height + 255) / 256 * 256; i++)
    {
        bsOut[0].write(0, 8);
    }

    int align_w = (pvbc_meta->width + 64 - 1) / 64;
    int align_h = (pvbc_meta->height + 16 - 1) / 16;
    MetaOutPack_size = align_w * align_h * 208; // Y: 64x16=12*2*4+32, C:64x8 = 12*2*2+32
    bsMetaOutPack = (unsigned char *) malloc(MetaOutPack_size);
    memset(bsMetaOutPack, 0, MetaOutPack_size);

    p_in_meta[0].offset = p_in_meta[1].offset = 0;
    for (int y = 0; y < pvbc_meta->height; y += pvbc_meta->tile_h)
    {
        for (int x = 0; x < pvbc_meta->width; x += pvbc_meta->tile_w)
        {
            vbc_encode_NV12(p_in_meta, x, y, tile_idx, src);
            tile_idx++;
        }
    }

    {
        int size, i;
        size = (pvbc_meta->height + 255) / 256 * 256;
        for(i = 0; i < size; i++)
            bsOut[0].m_fifo[i] = bsMetaOutPack[i];
    }

#ifdef SIG_VBC
    if (isEnableVBD())
    {
        unsigned char tmp_buf[32] = {0};
        //sigdump_output_bin(&g_sigpool.vbd_dma_meta_input_nv12, bsMetaOutPack, MetaOutPack_size);

        sigdump_output_bin(&g_sigpool.vbd_dma_bs_input_nv12, (unsigned char *)bsOut[0].getByteStream(), bsOut[0].getByteStreamLength());
        if ((bsOut[0].getByteStreamLength() % 32) != 0)
            sigdump_output_bin(&g_sigpool.vbd_dma_bs_input_nv12, tmp_buf, (32 - (bsOut[0].getByteStreamLength() % 32)));
    }
#endif

    double ratio[2];

    for (int ChType = 0; ChType < 2; ChType++)
    {
        meta_st *pvbc_meta = &p_in_meta[ChType];
        unsigned int pic_size = pvbc_meta->pic_width * pvbc_meta->pic_height;
        pvbc_meta->total_compress_size = bsOut[ChType].getByteStreamLength();
        ratio[ChType] = (double)(pvbc_meta->total_compress_size) / pic_size;
    }
    printf("\n[%d]YCR, %.3f, CCR, %.3f", frm_cnt, ratio[0], ratio[1]);

    if (bsMetaOutPack)
        free(bsMetaOutPack);

    g_sigdump.vbc = org_vbc;
    g_sigdump.vbc_dbg = org_vbc_dbg;
}
