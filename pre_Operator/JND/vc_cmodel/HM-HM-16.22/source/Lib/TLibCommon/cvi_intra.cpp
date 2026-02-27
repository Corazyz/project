#include <memory.h>
#include "cvi_intra.h"
#include "CommonDef.h"
#include "cvi_pattern.h"

using namespace std;

#define CVI_IAP_I8_MODES        6
#define CVI_IAP_I16_MODES       3
#define CVI_IAP_I4_REDUCE_MODE  4

int cviEdgeDetect(short *p_src_buf, int src_stride, int x, int y, int width, int height)
{
    int edge_strength[5] = { 0 }; // V, H, 45, 135, ND
    int blk4_es[5] = { 0 };
    int cx[4] = { 0 };  // c0~c3

    for (int i = 0; i < height; i += 4)
    {
        for (int j = 0; j < width; j += 4)
        {
            short *p_src_4x4 = p_src_buf + i * src_stride + j;
            int ci = 0;
            int sum = 0;

            // c0~c3
            for (int ch = 0; ch < 4; ch += 2)
            {
                for (int cw = 0; cw < 4; cw += 2)
                {
                    short *p_src = p_src_4x4 + (ch * src_stride) + cw;
                    sum = p_src[0];
                    sum += p_src[1];
                    p_src += src_stride;
                    sum += p_src[0];
                    sum += p_src[1];

                    cx[ci] = (sum + 2) >> 2;
                    ci++;
                }
            }

            // square2 = 1.4142136
            // 362/256 = 1.4140625
            int square2_factor = 362;
            int square2_shift = 8;

            // edge_strengthr
            blk4_es[0] = abs(cx[0] - cx[1] + cx[2] - cx[3]);
            blk4_es[1] = abs(cx[0] + cx[1] - cx[2] - cx[3]);
            blk4_es[2] = abs(((cx[0] - cx[3]) * square2_factor) >> square2_shift);
            blk4_es[3] = abs(((cx[1] - cx[2]) * square2_factor) >> square2_shift);
            blk4_es[4] = abs(cx[0] - cx[1] - cx[2] + cx[3]) << 1;

            edge_strength[0] += blk4_es[0];
            edge_strength[1] += blk4_es[1];
            edge_strength[2] += blk4_es[2];
            edge_strength[3] += blk4_es[3];
            edge_strength[4] += blk4_es[4];
#ifdef SIG_PRU
            if (g_sigdump.pru)
            {
                if ( width == 8 && g_sigpool.slice_type != I_SLICE)
                {
                    int max_es = 0;
                    int es_index = 0;
                    for (int i = 0; i < 5; i++)
                    {
                        if (max_es <= blk4_es[i])
                        {
                            max_es = blk4_es[i];
                            es_index = i;
                        }
                    }
                    sig_pru_set_edge_info(x + j, y + i, 4, es_index, max_es);
                }
            }
#endif //~SIG_PRU
        }
    }

    // pick max edge_strength
    int max_es = 0;
    int es_index = 0;
    for (int i = 0; i < 5; i++)
    {
        if (max_es <= edge_strength[i])
        {
            max_es = edge_strength[i];
            es_index = i;
        }
    }

#ifdef SIG_PRU
    if (g_sigdump.pru)
        sig_pru_set_edge_info(x, y, width, es_index, max_es);
#endif //~SIG_PRU

    return es_index;
}

int CVI_EDGE_MODE[5][3] =
{
    {23, 26, 29},   // V
    { 7, 10, 13},   // H
    { 3,  5, 33},   // 45
    {15, 18, 21},   // 135
    { 6, 18, 30}    // None angular
};

int CVI_EDGE_MODE_OFFSET[5][2] =
{
    {-1, 1},   // V
    {-1, 1},   // H
    {-1, 1},   // 45
    {-1, 1},   // 135
    {-4, 4}    // None angular
};

int CVI_EDGE_MODE_I16[5][5] =
{
    {0, 1, 23, 26, 29},   // V
    {0, 1,  7, 10, 13},   // H
    {0, 1,  3,  5, 33},   // 45
    {0, 1, 15, 18, 21},   // 135
    {0, 1,  6, 18, 30}    // None angular
};

bool cviSkipRmdMode(int pu_size, int mode, int edge_index)
{
    if (pu_size == 4 || pu_size == 8)
    {
        // 3 angular modes
        for (int i = 0; i < 3; i++)
        {
            if (mode == CVI_EDGE_MODE[edge_index][i])
                return false;
        }
    }
    else if (pu_size == 16)
    {
        // Planar, DC and 3 angular modes
        for (int i = 0; i < 5; i++)
        {
            if (mode == CVI_EDGE_MODE_I16[edge_index][i])
                return false;
        }
    }

    return true;
}

bool cviSkipIapMode(int mode, unsigned int rmd_modes[35], int rmd_number)
{
    for (int i = 0; i < rmd_number; i++)
    {
        if (mode == rmd_modes[i])
            return false;
    }
    return true;
}

void cviFillIntraMode(int pu_size, unsigned int select_mode[35], int &mode_number, int edge_index,
                      int mpm_number, int mpm_modes[3])
{
    if (pu_size == 4 || pu_size == 8)
    {
        if (g_algo_cfg.EnableIapI4ReduceMode && pu_size == 4)
            mode_number = CVI_IAP_I4_REDUCE_MODE;
        else
            mode_number = CVI_IAP_I8_MODES;

        int rmd_best_mode_0 = select_mode[0];
        int rmd_best_mode_1 = select_mode[1];

        select_mode[0] = PLANAR_IDX;
        select_mode[1] = DC_IDX;
        select_mode[2] = rmd_best_mode_0;
        select_mode[3] = rmd_best_mode_0 + CVI_EDGE_MODE_OFFSET[edge_index][0];
        select_mode[4] = rmd_best_mode_0 + CVI_EDGE_MODE_OFFSET[edge_index][1];
        select_mode[5] = rmd_best_mode_1;

        // insert mpm
        int replace_idx = 5;
        for (int i = 0; i < mpm_number; i++)
        {
            if (mpm_modes[i] <= DC_IDX)
                continue;

            bool insert_neighbor = true;
            for (int j = 2; j <= replace_idx; j++)
            {
                if (select_mode[j] == mpm_modes[i])
                {
                    insert_neighbor = false;
                    break;
                }
            }

            if (insert_neighbor == true)
            {
                select_mode[replace_idx] = mpm_modes[i];
                replace_idx--;
            }
        }
    }
    else if (pu_size == 16)
    {
        mode_number = CVI_IAP_I16_MODES;

        // priority: mpm first
        int replace_idx = 2;
        for (int i = 0; i < mpm_number; i++)
        {
            bool insert_neighbor = true;
            for (int j = 0; j <= replace_idx; j++)
            {
                if (select_mode[j] == mpm_modes[i])
                {
                    insert_neighbor = false;
                    break;
                }
            }

            if (insert_neighbor == true)
            {
                select_mode[replace_idx] = mpm_modes[i];
                replace_idx--;
            }
        }
    }
}

UInt CHROMA_BASIC_MODE[4] = {PLANAR_IDX, VER_IDX, HOR_IDX, DC_IDX};
UInt g_cviChromaCand[CVI_CHROMA_CAND_SIZE];
UInt g_cviChromaCandNumber = 0;

bool isChromaBasic(UInt mode)
{
    if (mode == PLANAR_IDX || mode == VER_IDX || mode == HOR_IDX || mode == DC_IDX)
        return true;

    return false;
}

void cviSetChromaCandidate(UInt *p_mode, UInt mode_number)
{
    if (p_mode == nullptr)
        return;

    g_cviChromaCandNumber = 0;
    memset(g_cviChromaCand, 0, sizeof(g_cviChromaCand));

    // Set chroma basic candidates
    for (int i = 0; i < 4; i++)
        g_cviChromaCand[i] = CHROMA_BASIC_MODE[i];

    g_cviChromaCand[4] = 34;
    g_cviChromaCandNumber = 5;

    for (int i = 0; i < mode_number; i++)
    {
        if (isChromaBasic(p_mode[i]) == false && p_mode[i] != 34)
        {
            g_cviChromaCand[g_cviChromaCandNumber] = p_mode[i];
            g_cviChromaCandNumber++;
        }
    }
}

void cviGetChromaCandidate(UInt *p_mode, UInt *p_number)
{
    if (p_mode == nullptr)
        return;

    *p_number = g_cviChromaCandNumber;

    for (int i = 0; i < g_cviChromaCandNumber; i++)
        p_mode[i] = g_cviChromaCand[i];
}
