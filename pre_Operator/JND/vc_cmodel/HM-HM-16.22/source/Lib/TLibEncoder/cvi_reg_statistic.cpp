#include "cvi_cu_ctrl.h"
#include "cvi_reg_statistic.h"

bool g_en_fpga_stat = false;
CviRegStatistic g_fpga_stat;

RegStat::RegStat(string name, UInt addr, int min, int max, int step)
    :m_name(name)
    ,m_address(addr)
    ,m_val_min(min)
    ,m_val_max(max)
    ,m_step(step)
{
    if (m_step <= 0)
        m_step = 1;

    int total_idx = ((max - min) / step) + 1;

    if (total_idx < 1)
        return;

    if (m_step != 1)
        total_idx += 2;           // separate the maximum and the minimum.
    m_total_idx = total_idx;
    m_scale.clear();                // scale will not be used in RS_STEP mode
    m_stat.clear();
    m_stat.resize(total_idx + 1);   // extra 1 for invalid input
    std::fill(m_stat.begin(), m_stat.end(), 0);
    m_mode = RS_STEP;
}

RegStat::RegStat(string name, UInt addr, int *p_scale, int number)
    :m_name(name)
    ,m_address(addr)
    ,m_val_min(0)
    ,m_val_max(0)
    ,m_step(0)
{
    m_total_idx = number;
    m_scale.clear();
    m_scale.resize(number + 1);   // extra 1 for invalid input
    for (int i = 0; i < number; i++)
        m_scale[i] = p_scale[i];
    m_scale[number] = -1;

    m_stat.clear();
    m_stat.resize(number + 1);   // extra 1 for invalid input
    std::fill(m_stat.begin(), m_stat.end(), 0);
    m_mode = RS_FIX;
}

RegStat::~RegStat()
{
    m_stat.clear();
}

void RegStat::AddCount(int val)
{
    if (m_mode == RS_STEP)
    {
        UInt idx = m_total_idx;
        if (val >= m_val_min && val <= m_val_max)
        {
            idx = (val - m_val_min) / m_step;
        }
        if (m_step != 1)
        {
            if (val == m_val_min)
            {
                m_stat[0]++;
            }
            else if (val == m_val_max)
            {
                m_stat[1]++;
            }
            idx += 2;
        }
        assert(idx < m_stat.size());
        m_stat[idx]++;
    }
    else // (m_mode == RS_FIX)
    {
        int idx = m_total_idx;
        for (int i = 0; i < m_total_idx; i++)
        {
            if (val == m_scale[i])
            {
                idx = i;
                break;
            }
        }
        assert(idx < m_stat.size());
        m_stat[idx]++;
    }
}


CviRegStatistic::CviRegStatistic()
{
    m_reg_stat.clear();
}

CviRegStatistic::~CviRegStatistic()
{
    m_reg_stat.clear();
}

RegStat* CviRegStatistic::GetRegStat(string name)
{
    for (vector<RegStat>::iterator it = m_reg_stat.begin() ; it != m_reg_stat.end(); it++)
    {
        if (name == it->GetName())
            return &m_reg_stat[it - m_reg_stat.begin()];
    }

    return nullptr;
}

bool CviRegStatistic::AddNewItem(string name, UInt addr, int min, int max, int step)
{
    if (GetRegStat(name) != nullptr)
    {
        printf("[FPGA][STAT][AddNewItem] Duplicated Item Name : %s\n", name.c_str());
        return false;
    }
    
    RegStat stat(name, addr, min, max, step);
    m_reg_stat.push_back(stat);
    return true;
}

bool CviRegStatistic::AddNewItem(string name, int *p_scale, int number)
{
    if (GetRegStat(name) != nullptr)
    {
        printf("[FPGA][STAT][AddNewItem] Duplicated Item Name : %s\n", name.c_str());
        return false;
    }

    RegStat stat(name, 0, p_scale, number);
    m_reg_stat.push_back(stat);

    return true;
}

void CviRegStatistic::InitStatItems()
{
    int resolution[6] = {176, 352, 704, 1280, 1920, 4096};
    AddNewItem("src_width", resolution, 6);

    AddNewItem("reg_i_slice", 0, 0, 1, 1);
    AddNewItem("reg_enc_fixed_qp", 0, 0, 1, 1);
    AddNewItem("reg_enc_qp_mode", 0, 0, 2, 1);
    AddNewItem("reg_slice_qp", 0, 0, MAX_QP, 1);
    AddNewItem("reg_slice_cb_qp_ofs", 0, -12, 12, 1);
    AddNewItem("reg_slice_cr_qp_ofs", 0, -12, 12, 1);
    AddNewItem("reg_enc_max_qp", 0, 1, MAX_QP, 1);
    AddNewItem("reg_enc_min_qp", 0, 0, MAX_QP - 1, 1);
    AddNewItem("reg_row_avg_bit", 0, 0, 0xfffff, 1024);

    AddNewItem("reg_err_comp_scl:BitErrSmoothFactor", 0, 1, 32, 1);
    AddNewItem("reg_row_qp_delta", 0, 0, 15, 1);
    AddNewItem("reg_row_ovf_qp_delta", 0, 0, 3, 1);
    AddNewItem("reg_lms_lrm", 0, 0, 1023, 4);
    AddNewItem("reg_lms_lrc", 0, 0, 1023, 4);
    AddNewItem("reg_lms_m_max", 0, 1, 131071, 1024); // 0x1ffff
    AddNewItem("reg_lms_m_min", 0, 0, 131071 - 1, 1024);
    AddNewItem("reg_lms_c_max", 0, 0, 131071, 1024);
    AddNewItem("reg_lms_c_min", 0, -131071, 0, 1024); // -reg_lms_c_max
    AddNewItem("reg_dist_chroma_weight", 0, 1, 255, 1);

    AddNewItem("g_tc_lut:in", 0, 0, 255, 1);
    AddNewItem("g_tc_lut:out", 0, -16, 16, 1);
    AddNewItem("g_qp_to_lambda_table", 0, 0, 65535, 256);
    AddNewItem("g_qp_to_sqrt_lambda_table", 0, 0, 32767, 128);
    AddNewItem("SI_TAB_0:g_hwEntropyBits[0]", 0, 0, 4095, 16);
    AddNewItem("SI_TAB_1:g_hwEntropyBits[1]", 0, 0, 16383, 64);

    // ToDo:
    // 9. RATIO
    AddNewItem("g_row_q_lut_in", 0, 0, 32767, 128);
    AddNewItem("g_row_q_lut_out", 0, 0, 51, 1);
    AddNewItem("ratio_luma_inter_4", 0, 0, 4095, 16);
    AddNewItem("ratio_luma_inter_8", 0, 0, 4095, 16);
    AddNewItem("ratio_luma_inter_16", 0, 0, 4095, 16);
    AddNewItem("ratio_luma_inter_32", 0, 0, 4095, 16);
    AddNewItem("ratio_chroma_inter_4", 0, 0, 4095, 16);
    AddNewItem("ratio_chroma_inter_8", 0, 0, 4095, 16);

    AddNewItem("ratio_luma_intra_4", 0, 0, 4095, 16);
    AddNewItem("ratio_luma_intra_8", 0, 0, 4095, 16);
    AddNewItem("ratio_luma_intra_16", 0, 0, 4095, 16);
    AddNewItem("ratio_luma_intra_32", 0, 0, 4095, 16);
    AddNewItem("ratio_chroma_intra_8", 0, 0, 4095, 16);
    AddNewItem("ratio_chroma_intra_16", 0, 0, 4095, 16);

    // 10. M/C
    for (int ch = 0; ch < 2; ch++)
    {
        string ch_s = ch == 0 ? "luma" : "chroma";
        for (int pred = 0; pred < 2; pred++)
        {
            string pred_s = pred == 0 ? "inter" : "intra";
            for (int size = 0; size < 4; size++)
            {
                string size_s = "blk" + std::to_string(4 * (1 << size));
                // M, C
                for (int bin = 0; bin < 2; bin++)
                {
                    string out_s = ch_s + "_" + pred_s + "_" + size_s + "_bin" + std::to_string(bin);
                    AddNewItem("M_" + out_s, 0, 0, 0x1ffff, 2048);
                    AddNewItem("C_" + out_s, 0, -131072, 131071, 2048);
                }
            }
        }
    }

    AddNewItem("reg_tmp_mvp_flag", 0, 0, 1, 1);
    AddNewItem("reg_slice_ilf_dis", 0, 0, 1, 1);
    AddNewItem("reg_slice_beta_ofs_div2", 0, -6, 6, 1);
    AddNewItem("reg_slice_tc_ofs_div2", 0, -6, 6, 1);

    AddNewItem("reg_enc_cons_mvd_en", 0, 0, 1, 1);
    AddNewItem("reg_enc_ime_mvdx_thr", 0, 0, 255, 1);
    AddNewItem("reg_enc_ime_mvdy_thr", 0, 0, 255, 1);
    AddNewItem("reg_i4_early_term_en", 0, 0, 1, 1);
    AddNewItem("reg_i4_madi_thr", 0, 0, 255, 1);
    AddNewItem("reg_pic_i4_term_target (I4TermRatio)", 0, 0, 100, 5);

    AddNewItem("reg_enc_fg_cost_en", 0, 0, 1, 1);
    AddNewItem("reg_ml_mv_gain", 0, 0, 63, 1);
    AddNewItem("reg_ml_satd_gain", 0, 0, 63, 1);
    AddNewItem("reg_fg_th_gain", 0, 0, 63, 1);
    AddNewItem("reg_fg_th_bias", 0, -128, 127, 1);

    AddNewItem("reg_enc_qpmap_mode", 0, 0, 2, 1);
    AddNewItem("reg_enc_qp_win_en (roi count)", 0, 0, 8, 1);

    {
        const char* type[] = {"BG", "FG", "cuWt"};
        const char* mode[] = {"Intra", "Inter", "Skip"};
        for (int i = 0; i < 3; i++)
        {
            int blk_sz, pred;
            for (pred = 0; pred < 3; pred++)
            {
                for (blk_sz = 32; blk_sz >= 8; blk_sz >>= 1)
                {
                    char name[128];
                    sprintf(name, "%s_%s%dCost", type[i], mode[pred], blk_sz);
                    AddNewItem(name, 0, CVI_PEN_COST_MIN, CVI_PEN_COST_MAX, 1);
                    sprintf(name, "%s_%s%dBias", type[i], mode[pred], blk_sz);
                    AddNewItem(name, 0, CVI_PEN_COST_BIAS_MIN, CVI_PEN_COST_BIAS_MAX, 16);
                }
            }
        }
    }

    AddNewItem("reg_enc_cons_mrg", 0, 0, 1, 1);
    AddNewItem("reg_enc_mrg_mvx_thr", 0, 0, 15, 1);
    AddNewItem("reg_enc_mrg_mvy_thr", 0, 0, 15, 1);
}

bool CviRegStatistic::AddItemCount(string name, int val)
{
    RegStat *p_reg_stat = GetRegStat(name);
    if (p_reg_stat == nullptr)
        return false;

    p_reg_stat->AddCount(val);
    return true;
}

void CviRegStatistic::Export()
{
    FILE *p_fp_csv = fopen("Fpga_Statistic.csv", "wt");
    FILE *p_fp_dat = fopen("Fpga_Statistic.dat", "wb");

    for (vector<RegStat>::iterator it = m_reg_stat.begin() ; it != m_reg_stat.end(); it++)
    {
        string name = it->GetName();
        int val_min = it->GetValMin();
        int step    = it->GetStep();
        vector<int> vec_stat = it->GetStat();
        int size = vec_stat.size();
        RegStatMode mode = it->GetMode();

        fprintf(p_fp_csv, "%s", name.c_str());
        if (mode == RS_STEP)
        {
            int i = 0;
            if (step != 1)
            {
                fprintf(p_fp_csv, ",min %d,max %d", val_min, it->GetValMax());
                i = 2;
            }
            for (int val = val_min; i < size; i++, val += step)
                fprintf(p_fp_csv, ",%d", val);
        }
        else
        {
            vector<int> vec_scale = it->GetScale();
            for (int i = 0; i < vec_scale.size(); i++)
                fprintf(p_fp_csv, ",%d", vec_scale[i]);
        }
        fprintf(p_fp_csv, "\n");

        for (int i = 0; i < size; i++)
        {
            fprintf(p_fp_csv, ",%d", vec_stat[i]);
        }
        fprintf(p_fp_csv, "\n");

        fwrite(vec_stat.data(), sizeof(int), size, p_fp_dat);
    }

    fclose(p_fp_csv);
    fclose(p_fp_dat);
    Report();
}

void CviRegStatistic::Report()
{
    FILE *p_fp_csv = fopen("Fpga_Statistic_Report.csv", "wt");
    for (vector<RegStat>::iterator it = m_reg_stat.begin() ; it != m_reg_stat.end(); it++)
    {
        string name = it->GetName();
        int val_min = it->GetValMin();
        int step    = it->GetStep();
        vector<int> vec_stat = it->GetStat();
        int size = vec_stat.size();
        RegStatMode mode = it->GetMode();

        if (mode == RS_STEP)
        {
            bool first_find = true;
            int i = 0;
            if (step != 1)
            {
                if (vec_stat[0] == 0 || vec_stat[1] == 0)
                {
                    int max_val = val_min + (size - 2) * step;
                    fprintf(p_fp_csv, "\n%s (%d to %d)loss:,", it->GetName().c_str(), val_min, max_val);
                    if (vec_stat[0] == 0)
                        fprintf(p_fp_csv, "min,");
                    if (vec_stat[1] == 0)
                        fprintf(p_fp_csv, "max,");
                    first_find = false;
                }
                i += 2;
            }

            for (int val = val_min; i < (size - 1); i++, val += step)
            {
                if (vec_stat[i] == 0)
                {
                    if (first_find)
                    {
                        int max_val = val_min + (size - 2) * step;
                        fprintf(p_fp_csv, "\n%s (%d to %d)loss:,", it->GetName().c_str(), val_min, max_val);
                    }
                    fprintf(p_fp_csv, "%d,", val);
                    first_find = false;
                }
            }
        }
        else
        {
            bool first_find = true;
            vector<int> vec_scale = it->GetScale();
            for (int i = 0; i < vec_scale.size() - 1; i++)
            {
                if (vec_stat[i] == 0)
                {
                    if (first_find)
                    {
                        fprintf(p_fp_csv, "\n%s loss:,", it->GetName().c_str());
                    }
                    fprintf(p_fp_csv, "%d,", vec_scale[i]);
                    first_find = false;
                }
            }
        }
    }
    fclose(p_fp_csv);
}

void CviRegStatistic::Import()
{
    FILE *p_fp_dat = fopen("Fpga_Statistic.dat", "rb");

    if (p_fp_dat == nullptr)
        return;

    for (vector<RegStat>::iterator it = m_reg_stat.begin() ; it != m_reg_stat.end(); it++)
    {
        vector<int> vec_stat = it->GetStat();
        int size = vec_stat.size();
        int *p_data = it->GetData();
        size_t sz = fread(p_data, sizeof(int), size, p_fp_dat);
        if (sz != size)
            printf("err\n");
    }

    fclose(p_fp_dat);
}
