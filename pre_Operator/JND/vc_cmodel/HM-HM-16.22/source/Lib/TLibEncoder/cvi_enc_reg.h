#ifndef __CVI_ENC_REG__
#define __CVI_ENC_REG__
using namespace std;

#include "../TLibCommon/TComSlice.h"

#define CALFDOZER_REG_BASE_MM       0x0B020000
#define CALFDOZER_REG_BASE_SYS      0x0B021000
#define CALFDOZER_REG_BASE_MEM      0x0B022000
#define CALFDOZER_REG_BASE_STAT     0x0B022400
#define CALFDOZER_REG_BASE_ENC      0x0B022C00
#define CALFDOZER_REG_BASE_HDR      0x0B026C00

#define CALFDOZER_REG_BASE_VBC      0x0B010000
#define CALFDOZER_REG_BASE_VBD      0x0B018000

typedef struct _HwStatistic_
{
    unsigned int frm_mse_sum;
    unsigned char frm_max_qp;
    unsigned char frm_min_qp;
    int frm_qp_sum;
    unsigned char frm_qp_hist_count;
    int *p_frm_qp_hist;
    unsigned int frm_madi_sum;
    unsigned int frm_madp_sum;
    unsigned char frm_madi_hist_count;
    int *p_frm_madi_hist;
    unsigned int frm_early_term;
    unsigned int frm_fg_cu16_cnt;
    unsigned int frm_bso_blen;
    unsigned int frm_hdr_sum;
    unsigned int frm_res_sum;
    unsigned int bit_err_accum;
    unsigned int blk_qp_accum;
    unsigned char cu_info_count;
    int *p_cu_info;
} HwStatistic;

void top_reg_init();
void top_set_all_reg(TComSlice *p_slice, TEncRateCtrl *p_rc);
void top_set_reg_vc_mm(TComSlice *p_slice);
void top_set_reg_vc_hdr(TComSlice *p_slice);
void top_set_reg_vc_mem(TComSlice *p_slice);
void top_set_reg_vc_enc(TComSlice *p_slice, TEncRateCtrl *p_rc);
void top_set_reg_qp_map_buf();
void top_set_reg_qp_map();
void top_set_hw_wakeup();
void top_set_axi_map();
void top_set_interrupt(uint32_t val);
void top_fill_hw_stat_golden(HwStatistic *p_hw_stat, bool is_rc_en, TComPic *pcPic, TEncSlice *pcSliceEncoder);
void top_get_vc_stat(HwStatistic *p_hw_stat);

//VBC
void top_set_vbc_wakeup();

#endif /* __CVI_ENC_REG__ */
