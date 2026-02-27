
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <cstdlib>
#include "cvi_enc.h"
#include "cvi_algo_cfg.h"
#include "cvi_cu_ctrl.h"
#include "cvi_motion.h"
#include "cvi_sigdump.h"
#include "cvi_rdo_bit_est.h"
#include "cvi_reg_statistic.h"

using namespace std;

#define DEFAULT_CONSTRAINED_MVD_THR_X 0x15
#define DEFAULT_CONSTRAINED_MVD_THR_Y 0x15

#define DEFAULT_CONSTRAINED_MV_LBND (-16384)
#define DEFAULT_CONSTRAINED_MV_UBND (16383)


cvi_enc_setting_st g_cvi_enc_setting;

bool get_avg_col_mv(const TComPic* pColPic)
{
    g_cvi_enc_setting.last_avg_mv_x = g_cvi_enc_setting.last_avg_mv_y = 0;

    g_cvi_enc_setting.last_max_mv_x = -MAX_INT;
    g_cvi_enc_setting.last_min_mv_x = MAX_INT;
    g_cvi_enc_setting.last_max_mv_y = -MAX_INT;
    g_cvi_enc_setting.last_min_mv_y = MAX_INT;

    const UInt maxCUWidth = pColPic->getPicSym()->getSPS().getMaxCUWidth();
    const UInt ctb_cnt = pColPic->getFrameWidthInCtus() * pColPic->getFrameHeightInCtus();
    int col_cnt = maxCUWidth / 16;
    int mv_cnt = 0;
    for( UInt ctuRsAddr = 0; ctuRsAddr < ctb_cnt; ++ctuRsAddr )
    {
        if (pColPic->getPicSym()->hasDPBPerCtuData())
        {
            const TComPicSym::DPBPerCtuData * const pColDpbCtu = &(pColPic->getPicSym()->getDPBPerCtuData(ctuRsAddr));
            // 16x16 Z-SCAN in CTB
            for (int col_idx = 0; col_idx < (col_cnt * col_cnt); col_idx++)
            {
                if (pColDpbCtu->isInter(col_idx * 16)) {
                    const TComMv &cColMv = pColDpbCtu->getCUMvField(REF_PIC_LIST_0)->getMv(col_idx * 16);
                    g_cvi_enc_setting.last_avg_mv_x += cColMv.getHor();
                    g_cvi_enc_setting.last_avg_mv_y += cColMv.getVer();
                    g_cvi_enc_setting.last_max_mv_x = max(g_cvi_enc_setting.last_max_mv_x, cColMv.getHor());
                    g_cvi_enc_setting.last_max_mv_y = max(g_cvi_enc_setting.last_max_mv_y, cColMv.getVer());
                    g_cvi_enc_setting.last_min_mv_x = min(g_cvi_enc_setting.last_min_mv_x, cColMv.getHor());
                    g_cvi_enc_setting.last_min_mv_y = min(g_cvi_enc_setting.last_min_mv_y, cColMv.getVer());
                    mv_cnt++;
                }
            }
        }
    }
    if (mv_cnt == 0)
        return false;

    g_cvi_enc_setting.last_avg_mv_x /= mv_cnt;
    g_cvi_enc_setting.last_avg_mv_y /= mv_cnt;
    return true;
}

void cvi_default_enc_config()
{
    g_cvi_enc_setting.enableConstrainedMVD = isEanbleFastIMEMVP() ? true : false;
    g_cvi_enc_setting.mvd_thr_x = isEanbleFastIMEMVP() ? isEanbleFastIMEMVP() : DEFAULT_CONSTRAINED_MVD_THR_X;
    g_cvi_enc_setting.mvd_thr_y = isEanbleFastIMEMVP() ? isEanbleFastIMEMVP() : DEFAULT_CONSTRAINED_MVD_THR_Y;
}

void cvi_init_enc_config()
{
    cvi_default_enc_config();
#ifdef CVI_CU_PENALTY
    cvi_init_hw_cost_weight();
#endif
    if (g_cvi_enc_setting.enableConstrainedMergeCand)
    {
        g_cvi_enc_setting.merge_cnt = 0;
        g_cvi_enc_setting.merge_2_cand_cnt = 0;
        g_cvi_enc_setting.merge_reduce_cnt = 0;
    }
}

void cvi_enc_update_config(const TComPic* pcPic)
{
  if (g_cvi_enc_setting.enableCviRGBChange)
  {
    g_algo_cfg.EnableRGBConvert++;
    if (g_algo_cfg.EnableRGBConvert > 7)
        g_algo_cfg.EnableRGBConvert = 1;
    g_algo_cfg.RGBFormat++;
    if (g_algo_cfg.RGBFormat > 3)
        g_algo_cfg.RGBFormat = 0;
  }
}

void cvi_enc_config(const TComPic* pcPic)
{
    cvi_default_enc_config();

    g_cvi_enc_setting.enableConstrainedMV = true;
#ifdef CVI_RANDOM_ENCODE
    if (g_cvi_enc_setting.enableCviRandomEnc == false || g_cvi_enc_setting.RandomGroup < 4)
#endif
    {
        g_cvi_enc_setting.mv_x_lbnd = -15;
        g_cvi_enc_setting.mv_x_ubnd = pcPic->getPicSym()->getSPS().getPicWidthInLumaSamples() + 15;
        g_cvi_enc_setting.mv_y_lbnd = -15;
        g_cvi_enc_setting.mv_y_ubnd = pcPic->getPicSym()->getSPS().getPicHeightInLumaSamples() + 15;
    }
    else
    {
        const TComPic *const pColPic = pcPic->getSlice(0)->getRefPic( RefPicList(pcPic->getSlice(0)->isInterB() ? 1-pcPic->getSlice(0)->getColFromL0Flag() : 0), pcPic->getSlice(0)->getColRefIdx());
        g_cvi_enc_setting.enableConstrainedMVD = false;
        if (pColPic)
        {
            g_cvi_enc_setting.enableConstrainedMVD = cvi_random_probability(90);
            if (get_avg_col_mv(pColPic) && g_cvi_enc_setting.enableConstrainedMVD && cvi_random_probability(30))
            {
                g_cvi_enc_setting.mvd_thr_x = g_cvi_enc_setting.last_max_mv_x - g_cvi_enc_setting.last_min_mv_x + cvi_random_range(-8, 8);
                g_cvi_enc_setting.mvd_thr_y = g_cvi_enc_setting.last_max_mv_y - g_cvi_enc_setting.last_min_mv_y + cvi_random_range(-8, 8);
                g_cvi_enc_setting.mvd_thr_x = min(max(0, g_cvi_enc_setting.mvd_thr_x), 0xff);
                g_cvi_enc_setting.mvd_thr_y = min(max(0, g_cvi_enc_setting.mvd_thr_y), 0xff);
            }
            else
            {
                cvi_default_enc_config();
            }
        }
        g_cvi_enc_setting.enableConstrainedMV = true;
        Int width = pcPic->getPicSym()->getSPS().getPicWidthInLumaSamples();
        Int height = pcPic->getPicSym()->getSPS().getPicHeightInLumaSamples();
        g_cvi_enc_setting.mv_x_lbnd = cvi_random_range( -15, 64);
        g_cvi_enc_setting.mv_x_ubnd = width - 1 + cvi_random_range( -64, 15);
        g_cvi_enc_setting.mv_y_lbnd = cvi_random_range( -15, 64);
        g_cvi_enc_setting.mv_y_ubnd = height - 1 + cvi_random_range( -64, 15);
        g_cvi_enc_setting.mv_x_lbnd = max(DEFAULT_CONSTRAINED_MV_LBND, g_cvi_enc_setting.mv_x_lbnd);
        g_cvi_enc_setting.mv_y_lbnd = max(DEFAULT_CONSTRAINED_MV_LBND, g_cvi_enc_setting.mv_y_lbnd);
        g_cvi_enc_setting.mv_x_ubnd = min(DEFAULT_CONSTRAINED_MV_UBND, g_cvi_enc_setting.mv_x_ubnd);
        g_cvi_enc_setting.mv_y_ubnd = min(DEFAULT_CONSTRAINED_MV_UBND, g_cvi_enc_setting.mv_y_ubnd);

        assert(g_cvi_enc_setting.mv_x_lbnd <= g_cvi_enc_setting.mv_x_ubnd);
        assert(g_cvi_enc_setting.mv_y_lbnd <= g_cvi_enc_setting.mv_y_ubnd);

    }

#ifdef CVI_CU_PENALTY
    cvi_set_pic_cost_weight((pcPic->getSlice(0)->getSliceType() == I_SLICE),
                            g_algo_cfg.CostPenaltyCfg.EnableForeground);
#endif
}

void cvi_random_frame_enc_config(const TComPic* pcPic)
{
    if (g_cvi_enc_setting.RandomGroup >= 2)
    {
        bool is_random_lr = (cvi_random_range(1, 100)) > 50 ? true : false;
        if (is_random_lr)
        {
            double frac = (1 << LMS_LR_FRAC_BD);
            int lrm = cvi_random_range(0, 1023);
            g_significantScale_LR[0] = (lrm / frac);
            int lrc = cvi_random_range(0, 1023);
            g_significantScale_LR[1] = (lrc / frac);

            // lrm min and lrm max registers are 18 bits, but hw only support unsigned Q2.15
            int lrm_max = cvi_random_range_step(1, 0x1ffff, 1024);
            int lrm_min = cvi_random_range_step(0, 0x1ffff - 1, 1024);
            if (lrm_max < lrm_min)
            std::swap(lrm_max, lrm_min);

            frac = (1 << RESI_MDL_FRAC_BD);
            g_significantScale_max = lrm_max / frac;
            g_significantScale_min = lrm_min / frac;

            // 18 bits, signed Q3.15
            // : lrc_clip is a 17 bits signed value.
            int lrc_clip = cvi_random_range_step(0, 0x1ffff, 1024);
            g_significantBias_clip = lrc_clip / frac;
        }

        bool is_random_cw = (cvi_random_equal() > 0) ? true : false;
        g_algo_cfg.ForceChromaWeight = is_random_cw ? cvi_random_range(1, 255) : 0;
    }

    if (g_cvi_enc_setting.RandomGroup >= 4 && g_algo_cfg.I4TermRatio != 0)
    {
        g_algo_cfg.I4TermRatio = ((double)(cvi_random_range(0, 100)) / 100);
    }

  //////////////////////////////////////////////////////////////////////////////////////
  // RG5, Cu Weighting
  double frac = 16.0;
  int cost_min = 0;
  int cost_max = 63;

  if (g_cvi_enc_setting.RandomGroup >= 5 && g_algo_cfg.CostPenaltyCfg.EnableForeground)
  {
    g_mvGain = (double)(cvi_random_range(cost_min, cost_max)) / frac;
    g_sadGain = (double)(cvi_random_range(cost_min, cost_max)) / frac;
    g_algo_cfg.CostPenaltyCfg.ForegroundThGain = (double)(cvi_random_range(cost_min, cost_max)) / frac;
    g_algo_cfg.CostPenaltyCfg.ForegroundThBias = cvi_random_range(-128, 127);
  }

  cvi_random_hw_cost_weight();
}
