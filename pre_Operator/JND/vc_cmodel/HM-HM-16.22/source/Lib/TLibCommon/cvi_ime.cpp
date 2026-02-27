
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "cvi_ime.h"

using namespace std;

cvi_var_st g_cvi_var;

cvi_frm_mv_array_st cvi_frm_mv_array[2];

void* new2d(int h, int w, int size)
{
    int i;
    void **p;

    p = (void**)new char[h*sizeof(void*) + h*w*size];
    for(i = 0; i < h; i++)
        p[i] = ((char *)(p + h)) + i*w*size;

    return p;
}

void init_frm_mv_array(int ctb_cnt_x, int ctb_cnt_y, int ctb_size)
{
    for (int list = 0; list < 2; list++)
    {
        cvi_frm_mv_array[list].cvi_ctb_mv = (cvi_ctb_mv_st *)malloc(sizeof(cvi_ctb_mv_st) * ctb_cnt_x * ctb_cnt_y);
        for (int i = 0; i < ctb_cnt_x * ctb_cnt_y; i++) {
            cvi_ctb_mv_st *cvi_ctb_mv = &cvi_frm_mv_array[list].cvi_ctb_mv[i];
            cvi_ctb_mv->cvi_16x16_mv = (cvi_16x16_mv_st **)new2d(ctb_size / 16, ctb_size / 16, sizeof(cvi_16x16_mv_st));
            for (int y = 0; y < ctb_size / 16; y++)
                for (int x = 0; x < ctb_size / 16; x++)
                {
                    memset(&cvi_ctb_mv->cvi_16x16_mv[y][x], 0x0, sizeof(cvi_16x16_mv_st));
                    cvi_ctb_mv->cvi_16x16_mv[y][x].cMvPred16x16.m_irefIdx = -1;
                }
        }
    }
}

bool is_8x8_store_frm_mv(int width, int height, int cu_x, int cu_y)
{
    bool is_right_bound = (cu_x + 8) == width && ((width % 16) != 0);
    bool is_bottom_bound = (cu_y + 8) == height && ((height % 16) != 0);
    bool is_right_bound_b2 = is_right_bound && (!is_bottom_bound) && ((cu_y % 16) != 0);
    bool is_bottom_bound_b0 = is_bottom_bound && (!is_right_bound) && ((cu_x % 16) == 0);
    bool is_bottom_right_bound_b0 = is_bottom_bound && is_right_bound;

    return (is_right_bound_b2 || is_bottom_bound_b0 || is_bottom_right_bound_b0);
}
void store_frm_mv(bool is16x16, int width, int height, int ctb_cnt, int ctb_size, int cu_x, int cu_y, short Hor, short Ver, int refPoc, int refIdx, int list)
{
    cvi_ctb_mv_st *cvi_ctb_mv = &cvi_frm_mv_array[list].cvi_ctb_mv[ctb_cnt];
    int index_x = cu_x % ctb_size;
    int index_y = cu_y % ctb_size;
    if (is16x16)
    {
        cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_iHor = Hor;
        cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_iVer = Ver;
        cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_irefPoc = refPoc;
        cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_irefIdx = refIdx;
    }
    else
    {
        cvi_16x16_mv_st *cvi_16x16_mv = &cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16];
        int idx8_x = (index_x & 0x8) >> 3;
        int idx8_y = (index_y & 0x8) >> 3;
        cvi_16x16_mv->cMvPred8x8[idx8_y][idx8_x].m_iHor = Hor;
        cvi_16x16_mv->cMvPred8x8[idx8_y][idx8_x].m_iVer = Ver;
        cvi_16x16_mv->cMvPred8x8[idx8_y][idx8_x].m_irefPoc = refPoc;
        cvi_16x16_mv->cMvPred8x8[idx8_y][idx8_x].m_irefIdx = refIdx;

        //check if cu is boundary
        if (is_8x8_store_frm_mv(width, height, cu_x, cu_y)) {
            cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_iHor = Hor;
            cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_iVer = Ver;
            cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_irefPoc = refPoc;
            cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_irefIdx = refIdx;
        }
    }
}

void get_frm_mv(int list, bool is16x16, int ctb_cnt, int index_x, int index_y, short &Hor, short &Ver, int &refPoc, int &refIdx)
{
    cvi_ctb_mv_st *cvi_ctb_mv = &cvi_frm_mv_array[list].cvi_ctb_mv[ctb_cnt];
    if (is16x16)
    {
        Hor = cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_iHor;
        Ver = cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_iVer;
        refPoc = cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_irefPoc;
        refIdx = cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16].cMvPred16x16.m_irefIdx;
    }
    else
    {
        cvi_16x16_mv_st *cvi_16x16_mv = &cvi_ctb_mv->cvi_16x16_mv[index_y / 16][index_x / 16];
        int idx8_x = (index_x & 0x8) >> 3;
        int idx8_y = (index_y & 0x8) >> 3;
        Hor = cvi_16x16_mv->cMvPred8x8[idx8_y][idx8_x].m_iHor;
        Ver = cvi_16x16_mv->cMvPred8x8[idx8_y][idx8_x].m_iVer;
        refPoc = cvi_16x16_mv->cMvPred8x8[idx8_y][idx8_x].m_irefPoc;
        refIdx = cvi_16x16_mv->cMvPred8x8[idx8_y][idx8_x].m_irefIdx;
    }
}

void deinit_frm_mv_array(int ctb_cnt_x, int ctb_cnt_y)
{
    for (int list = 0; list < 2; list++)
    {
        for (int i = 0; i < ctb_cnt_x * ctb_cnt_y; i++)
        {
            delete [] cvi_frm_mv_array[list].cvi_ctb_mv[i].cvi_16x16_mv;
        }
        free(cvi_frm_mv_array[list].cvi_ctb_mv);
    }
}

bool is_use_8x8_ime_mvp(int x, int y, int w, int h)
{
  bool is_first_8x8 = (x % 16) == 0 && (y % 16) == 0;
  bool is_width_not_align = (x + 8) == w;
  bool is_height_not_align = (y + 8) == h;
  return is_first_8x8 && (is_width_not_align || is_height_not_align);
}
