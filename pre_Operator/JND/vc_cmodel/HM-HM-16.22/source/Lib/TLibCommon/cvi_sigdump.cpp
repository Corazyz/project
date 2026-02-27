
#include <stdio.h>
#include <stdarg.h>
#include <cstring>
#include <errno.h>
#include "CommonDef.h"
#include "cvi_ime.h"
#include "cvi_sigdump.h"
using namespace std;

sigdump_st g_sigdump;
sigpool_st g_sigpool;

string g_FileName;

string get_file_name()
{
  return g_FileName;
}

bool init_file_name(string FileName)
{
  size_t found = FileName.find_last_of("/\\");
  std::string path = FileName.substr(found + 1);
  size_t lastindex = path.find_last_of(".");
  std::string file_name = path.substr(0, lastindex);
  g_FileName = file_name;

  return true;
}

bool is_enable_cabac_hw_emulator()
{
   return ((g_sigdump.cabac || g_sigdump.cabac_prof) && g_sigpool.cabac_is_record);
}

void cabac_prof_add_bins(CABAC_SYNTAX_TYPE syntax_t, CABAC_BIN_TYPE bin_t, int bin_num) {
  if (!(g_sigdump.cabac_prof && g_sigpool.cabac_is_record)) {
    return;
  }
  if(syntax_t==NON_RESI) {
    if(bin_t==NORMAL) {
      g_sigpool.nonresi_cabac_normal_cycles += bin_num;
     }
     else { // BYPASS
      g_sigpool.nonresi_cabac_bypass_cycles += bin_num;
    }
  }
  else { // RESIDUAL
    if(bin_t==NORMAL) {
      g_sigpool.resi_cabac_normal_cycles += ((bin_num + g_sigdump.cabac_prof_resi_nor_bin_throughout - 1) /
                                            g_sigdump.cabac_prof_resi_nor_bin_throughout);
     }
     else { // BYPASS
      g_sigpool.resi_cabac_bypass_cycles += ((bin_num + g_sigdump.cabac_prof_resi_byp_bin_throughout - 1) /
                                            g_sigdump.cabac_prof_resi_byp_bin_throughout);
    }
  }
}

void sigdump_open_ctx_bin(sig_ctx *p_ctx, const string SigdumpFileName)
{
  if (!p_ctx)
    return;
  if (p_ctx->is_open == true) {
    printf("%s is reopen\n", SigdumpFileName.c_str());
    assert(0);
  }
  p_ctx->fp = fopen(SigdumpFileName.c_str(), "wb");
  if (p_ctx->fp == NULL) {
    printf("fopen %s fail, erro = %d\n", SigdumpFileName.c_str(), errno);
  }
  assert(p_ctx->fp != NULL);
  p_ctx->enable = true;
  p_ctx->is_open = true;
}

void sigdump_open_ctx_bin_r(sig_ctx *p_ctx, const string SigdumpFileName)
{
  if (!p_ctx)
    return;
  if (p_ctx->is_open == true) {
    printf("%s is reopen\n", SigdumpFileName.c_str());
    assert(0);
  }
  p_ctx->fp = fopen(SigdumpFileName.c_str(), "rb");
  if (p_ctx->fp == NULL) {
    printf("fopen %s fail, erro = %d\n", SigdumpFileName.c_str(), errno);
  }
  assert(p_ctx->fp != NULL);
  p_ctx->enable = true;
  p_ctx->is_open = true;
}

void sigdump_open_ctx_txt(sig_ctx *p_ctx, const string SigdumpFileName)
{
  if (!p_ctx)
    return;
  if (p_ctx->is_open == true) {
    printf("%s is reopen\n", SigdumpFileName.c_str());
    assert(0);
  }
  p_ctx->fp = fopen(SigdumpFileName.c_str(), "w");
  if (p_ctx->fp == NULL) {
    printf("fopen %s fail, erro = %d\n", SigdumpFileName.c_str(), errno);
  }
  assert(p_ctx->fp != NULL);
  p_ctx->enable = true;
  p_ctx->is_open = true;
}

void sigdump_open_ctx_txt_r(sig_ctx *p_ctx, const string SigdumpFileName)
{
  if (!p_ctx)
    return;
  if (p_ctx->is_open == true) {
    printf("%s is reopen\n", SigdumpFileName.c_str());
    assert(0);
  }
  p_ctx->fp = fopen(SigdumpFileName.c_str(), "rt");
  if (p_ctx->fp == NULL) {
    printf("fopen %s fail, erro = %d\n", SigdumpFileName.c_str(), errno);
  }
  assert(p_ctx->fp != NULL);
  p_ctx->enable = true;
  p_ctx->is_open = true;
}

void sigdump_safe_close(sig_ctx *p_ctx)
{
  if (p_ctx->fp)
  {
    fclose(p_ctx->fp);
    p_ctx->fp = NULL;
  }
  p_ctx->enable = false;
  p_ctx->is_open = false;
}

bool sigdump_enable(sigdump_st *g_sigdump)
{
  if ((!g_sigdump->rru) && g_sigdump->ccu)
    g_sigpool.rru_no_output = true;
  else
    g_sigpool.rru_no_output = false;

  if (g_sigdump->fpga)
    g_sigdump->top = true;

  if (g_sigdump->ccu)
  {
    g_sigdump->cabac = true;
    g_sigdump->rru = true;
    g_sigdump->ime_mvp_stats = true;
  }

  if (g_sigdump->iapu)
  {
    g_sigdump->ccu = true;
    g_sigdump->rru = true;
  }

  if (g_sigdump->rru || g_sigdump->cabac || g_sigdump->input_src)
    g_sigdump->resi = true;

  if (g_sigdump->top || g_sigdump->mc || g_sigdump->irpu || g_sigdump->ime ||
      g_sigdump->fme || g_sigdump->ime_mvp || g_sigdump->ccu || g_sigdump->smt)
  {
    g_sigdump->frm_mgr = true;
  }

  if (g_sigdump->ime_mvp || g_sigdump->ccu)
    g_sigdump->col = true;

  if (g_sigdump->mc || g_sigdump->fme || g_sigdump->ime || g_sigdump->input_src) {
    if (!g_sigdump->input_src)
      g_sigdump->input_src = 1;
    g_sigdump->rec_yuv = true;
  }

  if (g_sigdump->ime_mvp_stats) {
    string SigdumpFileName = g_FileName + "_QP" + to_string(g_sigpool.initQP) + "_ime_mvp_stats.csv";
    sigdump_open_ctx_txt(&g_sigpool.ime_mvp_stats_ctx, SigdumpFileName);
  }

  if(g_sigdump->bit_stats) {
    for(int c=0; c<2; c++) {
      string sz_s = (c==0) ? "y_" : "c_";
      string sig0_bit_file = sz_s + "sig0_bit_stats.csv";
      string sig1_bit_file = sz_s + "sig1_bit_stats.csv";
      string last_pos_bit_file = sz_s + "last_pos_bit_stats.csv";
      string cgf_bit_file = sz_s + "cgf_bit_stats.csv";
      sigdump_open_ctx_txt(&g_sigpool.sig0_bit_stats[c], sig0_bit_file);
      sigdump_open_ctx_txt(&g_sigpool.sig1_bit_stats[c], sig1_bit_file);
      sigdump_open_ctx_txt(&g_sigpool.last_pos_bit_stats[c], last_pos_bit_file);
      sigdump_open_ctx_txt(&g_sigpool.cgf_bit_stats[c], cgf_bit_file);
    }
  }

  if(g_sigdump->perc_show)
  {
    string SigdumpFileName;
    const string pat[5] = {"_madi.yuv", "_alpha.yuv", "_motion.yuv", "_foreground.yuv", "_dqp.yuv"};
    for(int idx=0; idx<5; idx++) {
      SigdumpFileName = g_FileName + pat[idx];
      sigdump_open_ctx_bin(&g_sigpool.perc_show[idx], SigdumpFileName);
    }
  }

  if(g_sigdump->cabac_prof) {
    string SigdumpFileName = g_FileName + "_cabac_profile.csv";
    sigdump_open_ctx_txt(&g_sigpool.cabac_profile, SigdumpFileName);
    sigdump_output_fprint(&g_sigpool.cabac_profile, "non-resi, normal, 1bin/T, bypass, 1bin/T\n");
    sigdump_output_fprint(&g_sigpool.cabac_profile, "residual, normal, %dbin/T, bypass, %dbin/T\n" \
    ,g_sigdump->cabac_prof_resi_nor_bin_throughout \
    ,g_sigdump->cabac_prof_resi_byp_bin_throughout);
    memset(g_sigpool.ostanding_hist, 0, sizeof(int)*64);
    g_sigpool.ostanding_bit_cnt = 0;
  }

#ifdef SIG_RESI
  if (g_sigdump->resi)
  {
    int size_y = g_sigpool.fb_pitch * g_sigpool.heightAlign8 * 8;
    for (int i = 0; i < 2; i++) {
      if (g_sigpool.p_resi_buf[i] == NULL)
        g_sigpool.p_resi_buf[i] = new short[size_y >> i];
    }
  }
#endif //~SIG_RESI
#ifdef SIG_RRU
  if (g_sigdump->rru)
  {
    for (int i = 0; i < 3; i++)
    {
      if (g_sigpool.p_rru_i4_resi_temp[i] == NULL)
        g_sigpool.p_rru_i4_resi_temp[i] = new int[16];   // 4x4
      if (g_sigpool.p_rru_i4_resi_final[i] == NULL)
        g_sigpool.p_rru_i4_resi_final[i] = new int[16];
    }

    if (g_sigpool.p_rru_intp_temp == NULL)
      g_sigpool.p_rru_intp_temp = new short[1024];    // 32x32
    if (g_sigpool.p_rru_intp_final == NULL)
      g_sigpool.p_rru_intp_final = new short[1024];
    if (g_sigpool.p_rru_iq_out == NULL)
      g_sigpool.p_rru_iq_out = new int[256];

    for (int i = 0; i < 2; i++)
    {
      if (g_sigpool.p_rru_dct_in_temp[i] == NULL)
        g_sigpool.p_rru_dct_in_temp[i] = new short[1024];
      if (g_sigpool.p_rru_dct_in_final[i] == NULL)
        g_sigpool.p_rru_dct_in_final[i] = new short[1024];
      if (g_sigpool.p_rru_dct_out_temp[i] == NULL)
        g_sigpool.p_rru_dct_out_temp[i] = new int[1024];
      if (g_sigpool.p_rru_dct_out_final[i] == NULL)
        g_sigpool.p_rru_dct_out_final[i] = new int[1024];

      if (g_sigpool.p_rru_idct_in_temp[i] == NULL)
        g_sigpool.p_rru_idct_in_temp[i] = new int[1024];
      if (g_sigpool.p_rru_idct_in_final[i] == NULL)
        g_sigpool.p_rru_idct_in_final[i] = new int[1024];
      if (g_sigpool.p_rru_idct_out_temp[i] == NULL)
        g_sigpool.p_rru_idct_out_temp[i] = new short[1024];
      if (g_sigpool.p_rru_idct_out_final[i] == NULL)
        g_sigpool.p_rru_idct_out_final[i] = new short[1024];
    }
#ifdef SIG_RRU_DEBUG
    g_sigdump->rru_debug = false;  // Default to open debug patterns
    if (g_sigdump->rru_debug)
    {
      for (int i = 0; i < 2; i++)
      {
        if (g_sigpool.p_rru_iq_in_temp[i] == NULL)
          g_sigpool.p_rru_iq_in_temp[i] = new int[1024];
        if (g_sigpool.p_rru_iq_in_final[i] == NULL)
          g_sigpool.p_rru_iq_in_final[i] = new int[1024];
      }
    }
#endif //~SIG_RRU_DEBUG
  }
#endif //~SIG_RRU

#ifdef SIG_PRU
  if (g_sigdump->pru)
  {
    g_sigpool.p_pru_st = new sig_pru_st;
    memset(g_sigpool.p_pru_st, 0, sizeof(sig_pru_st));
  }
#endif //~SIG_PRU

#ifdef SIG_CCU
  if (g_sigdump->ccu)
  {
    g_sigpool.p_ccu_rc_st = new sig_ccu_rc_st;
    memset(g_sigpool.p_ccu_rc_st, 0, sizeof(sig_ccu_rc_st));
  }
#endif //~SIG_CCU
#ifdef SIG_IAPU
  if (g_sigdump->iapu_ref_smp_mux){
    string SigdumpFileName;
    SigdumpFileName = g_FileName + "_iapu_rmd_ref_smp_mux_cml" + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.iapu_rmd_ref_smp_mux_ctx, SigdumpFileName);
  }
  if (g_sigdump->iapu){
    for (int i = 0; i < 3; i++)
    {
      if (g_sigpool.iapu_st.p_iapu_i4_coef_temp[i] == NULL)
        g_sigpool.iapu_st.p_iapu_i4_coef_temp[i] = new int[16];   // 4x4
      if (g_sigpool.iapu_st.p_iapu_i4_coef_final[i] == NULL)
        g_sigpool.iapu_st.p_iapu_i4_coef_final[i] = new int[16];
    }
  }
#endif //~SIG_IAPU

#ifdef SIG_PPU
  if (g_sigdump->ppu)
  {
    g_sigpool.p_ppu_st = new sig_ppu_st;
    memset(g_sigpool.p_ppu_st, 0, sizeof(sig_ppu_st));
  }
#endif //~SIG_PPU

#ifdef SIG_FPGA
  if (g_sigdump->fpga)
  {
    
    if (g_sigdump->testBigClipSmall == 1)
    {
      sigdump_open_ctx_txt(&g_sigpool.fpga_pat_info_ctx, "pat_info_big.txt");
      string SigdumpFileName;
      SigdumpFileName = g_FileName + ".md5";
      sigdump_open_ctx_txt(&g_sigpool.fpga_md5_ctx, SigdumpFileName);
    }
    else if (g_sigdump->testBigClipSmall == 2)
    {
      sigdump_open_ctx_txt(&g_sigpool.fpga_pat_info_ctx, "pat_info_small.txt");
      string SigdumpFileName;
      SigdumpFileName = g_FileName + ".md5";
      sigdump_open_ctx_txt(&g_sigpool.fpga_md5_ctx, SigdumpFileName);
    }
    else
    {
      sigdump_open_ctx_txt(&g_sigpool.fpga_pat_info_ctx, "pat_info.txt");
      string SigdumpFileName;
      SigdumpFileName = g_FileName + ".md5";
      sigdump_open_ctx_txt(&g_sigpool.fpga_md5_ctx, SigdumpFileName);
    }

    if (g_sigdump->cabac == false)
    {
      g_sigdump->cabac = true;
      g_sigdump->cabac_no_output = true;
    }

  }
#endif //~SIG_FPGA
#ifdef SIG_VBC
  if (g_sigdump->vbc)
  {
    string SigdumpFileName = g_FileName + "_vbe_frame_setting" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.vbe_frame_ctx, SigdumpFileName);
  }
#endif

#ifdef SIG_SMT
  if (g_sigdump->smt)
  {
    g_sigpool.p_smt_st = new sig_smt_st;
    memset(g_sigpool.p_smt_st, 0, sizeof(sig_smt_st));
    int width_in_16  = (g_sigpool.width + 15) >> 4;
    int height_in_16 = (g_sigpool.height + 15) >> 4;

    g_sigpool.p_smt_st->pp_smt_gld_data = new sig_smt_dat_st*[height_in_16];
    g_sigpool.p_smt_st->pp_smt_gld_data[0] = new sig_smt_dat_st[width_in_16 * height_in_16];

    for (int i = 1; i < height_in_16; i++)
      g_sigpool.p_smt_st->pp_smt_gld_data[i] = &g_sigpool.p_smt_st->pp_smt_gld_data[0][width_in_16 * i];
  }
#endif
  return true;
}

bool sigdump_disable(sigdump_st *g_sigdump)
{
  if (g_sigdump->ime_mvp_stats) {
    if (g_sigpool.ime_mvp_stats_ctx.fp)
      fclose(g_sigpool.ime_mvp_stats_ctx.fp);
  }

  if(g_sigdump->bit_stats)
  {
    for(int c=0; c<2; c++) {
      fclose(g_sigpool.sig0_bit_stats[c].fp);
      fclose(g_sigpool.sig1_bit_stats[c].fp);
      fclose(g_sigpool.last_pos_bit_stats[c].fp);
      fclose(g_sigpool.cgf_bit_stats[c].fp);
      g_sigpool.sig0_bit_stats[c].enable = false;
      g_sigpool.sig1_bit_stats[c].enable = false;
      g_sigpool.last_pos_bit_stats[c].enable = false;
      g_sigpool.cgf_bit_stats[c].enable = false;
    }
  }

  if(g_sigdump->pic_rc) {
    sigdump_safe_close(&g_sigpool.pic_rc_stats);
    sigdump_safe_close(&g_sigpool.pic_rc_golden);
  }

  if(g_sigdump->perc_show)
  {
    for(int idx=0; idx<5; idx++) {
      sigdump_safe_close(&g_sigpool.perc_show[idx]);
    }
  }

  if(g_sigdump->cabac_prof) {
    sigdump_output_fprint(&g_sigpool.cabac_profile, "\nos bit range,");
    for(int bin_i=0; bin_i<64; bin_i++) {
      sigdump_output_fprint(&g_sigpool.cabac_profile, "%d,", bin_i<<(g_sigdump->cabac_prof_os_bin_w));
    }
    int total_cnt = 0;
    sigdump_output_fprint(&g_sigpool.cabac_profile, "\nbin cnt,");
    for(int bin_i=0; bin_i<64; bin_i++) {
      sigdump_output_fprint(&g_sigpool.cabac_profile, "%d,", g_sigpool.ostanding_hist[bin_i]);
      total_cnt += g_sigpool.ostanding_hist[bin_i];
    }
    sigdump_output_fprint(&g_sigpool.cabac_profile, "\nbin prob.,");
    for(int bin_i=0; bin_i<64; bin_i++) {
      sigdump_output_fprint(&g_sigpool.cabac_profile, "%.5f,", g_sigpool.ostanding_hist[bin_i]/(double)total_cnt);
    }
    sigdump_output_fprint(&g_sigpool.cabac_profile, "\n");
    fclose(g_sigpool.cabac_profile.fp);
  }

#ifdef SIG_RESI
  if (g_sigdump->resi)
  {
    for (int i = 0; i < 2; i++)
    {
      if (g_sigpool.p_resi_buf[i])
      {
        delete[] g_sigpool.p_resi_buf[i];
        g_sigpool.p_resi_buf[i] = NULL;
      }
    }
  }
#endif //~SIG_RESI
#ifdef SIG_RRU
  if (g_sigdump->rru)
  {
    for (int i = 0; i < 3; i++)
    {
      if (g_sigpool.p_rru_i4_resi_temp[i])
      {
        delete[] g_sigpool.p_rru_i4_resi_temp[i];
        g_sigpool.p_rru_i4_resi_temp[i] = NULL;
      }
      if (g_sigpool.p_rru_i4_resi_final[i])
      {
        delete[] g_sigpool.p_rru_i4_resi_final[i];
        g_sigpool.p_rru_i4_resi_final[i] = NULL;
      }
    }

    if (g_sigpool.p_rru_intp_temp)
      delete[] g_sigpool.p_rru_intp_temp;
    if (g_sigpool.p_rru_intp_final)
      delete[] g_sigpool.p_rru_intp_final;
    if (g_sigpool.p_rru_iq_out)
      delete[] g_sigpool.p_rru_iq_out;

    for (int i = 0; i < 2; i++)
    {
      if (g_sigpool.p_rru_dct_in_temp[i])
      {
        delete[] g_sigpool.p_rru_dct_in_temp[i];
        g_sigpool.p_rru_dct_in_temp[i] = NULL;
      }
      if (g_sigpool.p_rru_dct_in_final[i])
      {
        delete[] g_sigpool.p_rru_dct_in_final[i];
        g_sigpool.p_rru_dct_in_final[i] = NULL;
      }
      if (g_sigpool.p_rru_dct_out_temp[i])
      {
        delete[] g_sigpool.p_rru_dct_out_temp[i];
        g_sigpool.p_rru_dct_out_temp[i] = NULL;
      }
      if (g_sigpool.p_rru_dct_out_final[i])
      {
        delete[] g_sigpool.p_rru_dct_out_final[i];
        g_sigpool.p_rru_dct_out_final[i] = NULL;
      }

      if (g_sigpool.p_rru_idct_in_temp[i])
      {
        delete[] g_sigpool.p_rru_idct_in_temp[i];
        g_sigpool.p_rru_idct_in_temp[i] = NULL;
      }
      if (g_sigpool.p_rru_idct_in_final[i])
      {
        delete[] g_sigpool.p_rru_idct_in_final[i];
        g_sigpool.p_rru_idct_in_final[i] = NULL;
      }
      if (g_sigpool.p_rru_idct_out_temp[i])
      {
        delete[] g_sigpool.p_rru_idct_out_temp[i];
        g_sigpool.p_rru_idct_out_temp[i] = NULL;
      }
      if (g_sigpool.p_rru_idct_out_final[i])
      {
        delete[] g_sigpool.p_rru_idct_out_final[i];
        g_sigpool.p_rru_idct_out_final[i] = NULL;
      }
    }
#ifdef SIG_RRU_DEBUG
    if (g_sigdump->rru_debug)
    {
      for (int i = 0; i < 2; i++)
      {
        if (g_sigpool.p_rru_iq_in_temp[i])
        {
          delete[] g_sigpool.p_rru_iq_in_temp[i];
          g_sigpool.p_rru_iq_in_temp[i] = NULL;
        }
        if (g_sigpool.p_rru_iq_in_final[i])
        {
          delete[] g_sigpool.p_rru_iq_in_final[i];
          g_sigpool.p_rru_iq_in_final[i] = NULL;
        }
      }
    }
#endif //~SIG_RRU_DEBUG
  }
#endif //~SIG_RRU

#ifdef SIG_PRU
  if (g_sigdump->pru)
  {
    if (g_sigpool.p_pru_st)
    {
      delete g_sigpool.p_pru_st;
      g_sigpool.p_pru_st = nullptr;
    }
  }
#endif //~SIG_PRU

#ifdef SIG_CCU
  if (g_sigdump->ccu)
  {
    if (g_sigpool.p_ccu_rc_st)
    {
      delete g_sigpool.p_ccu_rc_st;
      g_sigpool.p_ccu_rc_st = nullptr;
    }
  }
#endif //~SIG_CCU
#ifdef SIG_IAPU
  if (g_sigdump->iapu_ref_smp_mux){
    sigdump_safe_close(&g_sigpool.iapu_rmd_ref_smp_mux_ctx);
  } 
  if (g_sigdump->iapu){
    for (int i = 0; i < 3; i++)
    {
      if (g_sigpool.iapu_st.p_iapu_i4_coef_temp[i])
      {
        delete[] g_sigpool.iapu_st.p_iapu_i4_coef_temp[i];
        g_sigpool.iapu_st.p_iapu_i4_coef_temp[i] = NULL;
      }
      if (g_sigpool.iapu_st.p_iapu_i4_coef_final[i])
      {
        delete[] g_sigpool.iapu_st.p_iapu_i4_coef_final[i];
        g_sigpool.iapu_st.p_iapu_i4_coef_final[i] = NULL;
      }
    }
  } 
#endif //~SIG_IAPU

#ifdef SIG_PPU
  if (g_sigdump->ppu)
  {
    if (g_sigpool.p_ppu_st)
    {
      delete g_sigpool.p_ppu_st;
      g_sigpool.p_ppu_st = nullptr;
    }
  }
#endif //~SIG_PRU

#ifdef SIG_FPGA
  if (g_sigdump->fpga)
  {
    sigdump_output_fprint(&g_sigpool.fpga_pat_info_ctx, "%s\n", g_FileName.c_str());
    sigdump_output_fprint(&g_sigpool.fpga_pat_info_ctx, "width %d\n", g_sigpool.width);
    sigdump_output_fprint(&g_sigpool.fpga_pat_info_ctx, "height %d\n", g_sigpool.height);
    sigdump_output_fprint(&g_sigpool.fpga_pat_info_ctx, "enc_cnt %d\n", g_sigpool.enc_count);
    sigdump_output_fprint(&g_sigpool.fpga_pat_info_ctx, "frm_skip %d\n", g_sigpool.frame_skip);
    sigdump_safe_close(&g_sigpool.fpga_pat_info_ctx);
    sigdump_safe_close(&g_sigpool.fpga_md5_ctx);
  }
#endif //~SIG_FPGA
#ifdef SIG_VBC
  if (g_sigdump->vbc)
  {
    sigdump_safe_close(&g_sigpool.vbe_frame_ctx);
  }
#endif

#ifdef SIG_SMT
  if (g_sigdump->smt)
  {
    if (g_sigpool.p_smt_st)
    {
      delete[] g_sigpool.p_smt_st->pp_smt_gld_data[0];
      delete[] g_sigpool.p_smt_st->pp_smt_gld_data;
      delete g_sigpool.p_smt_st;
      g_sigpool.p_smt_st = nullptr;
    }
  }
#endif
  return true;
}

bool sigdump_framewise_enable(void)
{
  bool enable = false;
  g_sigpool.slice_count = 0;
  g_sigpool.ctb_count = 0;
  for (int i = 0; i < 5; i++)
    g_sigpool.cu_count[i] = 0;
  g_sigpool.cu_chroma_4x4_count = 0;

  if (g_sigpool.enc_count >= g_sigdump.start_count && g_sigpool.enc_count <= g_sigdump.end_count)
    enable = true;
  if (!enable)
    return false;

  if (g_sigdump.ime_mvp_stats || g_sigdump.fpga)
  {
    g_sigpool.cu_16x16_1_cand_count = 0;
    g_sigpool.cu_16x16_2_cand_count = 0;
    g_sigpool.cu_16x16_2_cand_count_mod = 0;
    g_sigpool.abs_hor = 0;
    g_sigpool.abs_ver = 0;
    g_sigpool.cu_16x16_count = 0;
  }
#ifdef SIG_VBC
  if (g_sigdump.vbc_dbg)
  {
    for (int ch = 0; ch < 2; ch++)
    {
      string SigdumpFileName;
      string comp_type = ch == 0 ? "luma" : "chroma";
      SigdumpFileName = g_FileName + "_vbe_meta_dbg_frm_" + comp_type + "_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.vbe_meta_debug_ctx[ch], SigdumpFileName);
    }
  }
  if (g_sigdump.vbc)
  {
    string SigdumpFileName;
    int ch;
    if (g_sigdump.vbc_low)
    {
      SigdumpFileName = g_FileName + "_vbe_mode_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.vbe_mode_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_enc_info_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.vbe_enc_info_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_vbe_pu_rec_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.vbe_rec_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_vbe_bs_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.vbe_bs_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_vbe_pu_stream_in_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.vbe_strm_pack_out_ctx, SigdumpFileName);
    }
    SigdumpFileName = g_FileName + "_vbd_dma_meta_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.vbd_dma_meta, SigdumpFileName);

    for (ch = 0; ch < 2; ch++)
    {
        string comp_type = ch == 0 ? "luma" : "chroma";
        if (g_sigdump.vbc_low)
        {
          SigdumpFileName = g_FileName + "_vbe_src_frm_" + comp_type + "_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
          sigdump_open_ctx_txt(&g_sigpool.vbe_src_ctx[ch], SigdumpFileName);

          SigdumpFileName = g_FileName + "_vbd_dma_strm_pack_" + comp_type + "_"+ to_string(g_sigpool.enc_count_pattern) + ".txt";
          sigdump_open_ctx_txt(&g_sigpool.vbe_pack_out_ctx[ch], SigdumpFileName);

          SigdumpFileName = g_FileName + "_vbc_dma_cu_info_" + comp_type + "_"+ to_string(g_sigpool.enc_count_pattern) + ".txt";
          sigdump_open_ctx_txt(&g_sigpool.vbc_dma_cu_info[ch], SigdumpFileName);

          SigdumpFileName = g_FileName + "_vbe_pu_in_l_strm" + to_string(ch) + "_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
          sigdump_open_ctx_txt(&g_sigpool.vbe_pu_str_ctx[0][ch], SigdumpFileName);

          SigdumpFileName = g_FileName + "_vbe_pu_in_r_strm" + to_string(ch) + "_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
          sigdump_open_ctx_txt(&g_sigpool.vbe_pu_str_ctx[1][ch], SigdumpFileName);

          SigdumpFileName = g_FileName + "_vbe_pu_out_l_strm" + to_string(ch) + "_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
          sigdump_open_ctx_txt(&g_sigpool.vbe_pu_str_out_ctx[0][ch], SigdumpFileName);

          SigdumpFileName = g_FileName + "_vbe_pu_out_r_strm" + to_string(ch) + "_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
          sigdump_open_ctx_txt(&g_sigpool.vbe_pu_str_out_ctx[1][ch], SigdumpFileName);
        }

        comp_type = ch == 0 ? "Y" : "C";
        if (g_sigdump.vbc_low)
        {
          SigdumpFileName = g_FileName + "_vbe_lossy_src_" + comp_type + "_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
          sigdump_open_ctx_txt(&g_sigpool.vbe_lossy_src_ctx[ch], SigdumpFileName);
          SigdumpFileName = g_FileName + "_vbd_dma_meta_cpx_"+ comp_type + "_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
          sigdump_open_ctx_bin(&g_sigpool.vbd_dma_meta_cpx[ch], SigdumpFileName);
        }
        SigdumpFileName = g_FileName + "_vbd_dma_cpx_" + comp_type + "_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        sigdump_open_ctx_bin(&g_sigpool.vbd_dma_bs[ch], SigdumpFileName);

    }
    if (isEnableVBD())
    {
      string SigdumpFileName;
      SigdumpFileName = g_FileName + "_vbd_dma_cpx_input_nv12_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
      sigdump_open_ctx_bin(&g_sigpool.vbd_dma_bs_input_nv12, SigdumpFileName);
    }
  }
#endif
  if (g_sigdump.top || g_sigdump.fpga)
  {
    string SigdumpFileName = g_FileName + "_qpmap_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.top_qp_map, SigdumpFileName);
  }
  if (g_sigdump.top && !g_sigdump.fpga)
  {
      string SigdumpFileName;
      for (int i = 0; i < 2; i++)
      {
        string comp_type = i == 0 ? "y" : "c";
        SigdumpFileName = g_FileName + "_rec_" + comp_type + "_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        sigdump_open_ctx_bin(&g_sigpool.top_rec_ctx[i], SigdumpFileName);
      }
  }
  if (g_sigdump.input_src)
  {
      string format;
      string SigdumpFileName;
      SigdumpFileName = g_FileName + "_mc_src_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
      g_sigpool.input_f = fopen(SigdumpFileName.c_str(), "wb");

      if (getRotateAngleMode() != 0)
      {
        SigdumpFileName = g_FileName + "_srcrot_" + to_string(g_algo_cfg.EnableRotateAngle) + "_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        g_sigpool.input_rotate_f = fopen(SigdumpFileName.c_str(), "wb");
      }
      else if (g_algo_cfg.EnableMirror != 0)
      {
        if (g_algo_cfg.EnableMirror == 1)
          SigdumpFileName = g_FileName + "_src_hmirror_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        else
          SigdumpFileName = g_FileName + "_src_vmirror_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        g_sigpool.input_rotate_f = fopen(SigdumpFileName.c_str(), "wb");
      }
  }
  if (g_sigdump.irpu)
  {
      string SigdumpFileName;
      const string size[3] = {"8x8_", "16x16_", "32x32_"};
      for (int i = 0; i < 3; i++)
      {
        SigdumpFileName = g_FileName + "_irpu_cmd_" + size[i] + to_string(g_sigpool.enc_count_pattern) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.irpu_cmd_frm_ctx[i], SigdumpFileName);
        sigdump_output_fprint(&g_sigpool.irpu_cmd_frm_ctx[i], "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);

        SigdumpFileName = g_FileName + "_irpu_fme_mc_mdl_" + size[i] + to_string(g_sigpool.enc_count_pattern) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[i], SigdumpFileName);
        sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[i], "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);

        SigdumpFileName = g_FileName + "_irpu_golden_" + size[i] + to_string(g_sigpool.enc_count_pattern) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.irpu_golden_frm_ctx[i], SigdumpFileName);
        sigdump_output_fprint(&g_sigpool.irpu_golden_frm_ctx[i], "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);
      }
      SigdumpFileName = g_FileName + "_irpu_col_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.irpu_col_frm_ctx, SigdumpFileName);
      sigdump_output_fprint(&g_sigpool.irpu_col_frm_ctx, "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);

      SigdumpFileName = g_FileName + "_irpu_reg_init_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.irpu_reg_init_ctx, SigdumpFileName);
  }
  if (g_sigdump.fme)
  {
      string SigdumpFileName;
      const string size[2] = {"blk8_", "blk16_"};
      for (int i = 0; i < 2; i++)
      {
        SigdumpFileName = g_FileName + "_fme_" + size[i] + "cmd_in_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.fme_cmd_in_ctx[i], SigdumpFileName);
        sigdump_output_fprint(&g_sigpool.fme_cmd_in_ctx[i], "reg_pic_width_cu_m1 = %x\n", g_sigpool.widthAlign8 - 1);
        sigdump_output_fprint(&g_sigpool.fme_cmd_in_ctx[i], "reg_pic_height_cu_m1 = %x\n", g_sigpool.heightAlign8 - 1);
        sigdump_output_fprint(&g_sigpool.fme_cmd_in_ctx[i], "reg_fb_pit = %x\n", g_sigpool.fb_pitch);

        SigdumpFileName = g_FileName + "_fme_" + size[i] + "dat_out_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        sigdump_open_ctx_txt(&g_sigpool.fme_dat_out_ctx[i], SigdumpFileName);
        if (g_sigdump.fme_debug) {
          SigdumpFileName = g_FileName + "_fme_" + size[i] + "dat_out_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
          sigdump_open_ctx_txt(&g_sigpool.fme_dat_out_txt_ctx[i], SigdumpFileName);
        }
      }
      g_sigpool.fme_16_cnt = 0;
      g_sigpool.fme_8_cnt = 0;
  }
  if (g_sigdump.ime)
  {
      string SigdumpFileName;
      SigdumpFileName = g_FileName + "_ime_" + "cmd_in_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.ime_cmd_in_ctx, SigdumpFileName);
      sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "reg_pic_width_cu_m1 = %x\n", g_sigpool.widthAlign8 - 1);
      sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "reg_pic_height_cu_m1 = %x\n", g_sigpool.heightAlign8 - 1);
      sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "reg_fb_pit = %x\n", g_sigpool.fb_pitch);

      SigdumpFileName = g_FileName + "_ime_" + "dat_out_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.ime_dat_out_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_ime_" + "dat_out_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
      sigdump_open_ctx_bin(&g_sigpool.ime_dat_out_bin_ctx, SigdumpFileName);

      g_sigpool.ime_16_cnt = 0;
  }
  if (g_sigdump.ime_mvp)
  {
      string SigdumpFileName;
      SigdumpFileName = g_FileName + "_ime_mvp_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.ime_mvp_ctx, SigdumpFileName);
      sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);
      sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "reg_pic_width_ctu_m1 = %x\n", g_sigpool.widthAlignCTB - 1);
      sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "reg_pic_height_ctu_m1 = %x\n", g_sigpool.heightAlignCTB - 1);
      sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "reg_pic_width_cu_m1 = %x\n", g_sigpool.widthAlign8 - 1);
      sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "reg_pic_height_cu_m1 = %x\n", g_sigpool.heightAlign8 - 1);

      SigdumpFileName = g_FileName + "_ime_mvb_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
      sigdump_open_ctx_bin(&g_sigpool.ime_mvb_ctx, SigdumpFileName);
  }
  if (g_sigdump.col)
  {
      string SigdumpFileName;
      SigdumpFileName = g_FileName + "_ccu_col_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
      sigdump_open_ctx_bin(&g_sigpool.col_ctx, SigdumpFileName);
  }
  if (g_sigdump.mc)
  {
      string SigdumpFileName;
      const string size[3] = {"blk8_", "blk16_", "blk32_"};
      const string type[2] = {"lma_", "cma_"};
      for (int i = 0; i < 3; i++)
      {
        SigdumpFileName = g_FileName + "_mc_mrg_" + size[i] + "cmd_in_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.mc_mrg_ctx_cmd_in[i], SigdumpFileName);
        sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[i], "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);
        sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[i], "reg_pic_width_cu_m1 = %x\n", g_sigpool.widthAlign8 - 1);
        sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[i], "reg_pic_height_cu_m1 = %x\n", g_sigpool.heightAlign8 - 1);
        sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[i], "reg_fb_pit = %x\n", g_sigpool.fb_pitch);
        for (int j = 0; j < 2; j++)
        {
          SigdumpFileName = g_FileName + "_mc_mrg_" + type[j] + size[i] + "dat_out_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
          sigdump_open_ctx_bin(&g_sigpool.mc_mrg_ctx_dat_out[i][j], SigdumpFileName);
        }
        g_sigpool.mrg_cnt[i] = 0;
      }
  }
  if (g_sigdump.rec_yuv)
  {
      string SigdumpFileName = g_FileName + "_mc_ref_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
      sigdump_open_ctx_bin(&g_sigpool.mc_rec_ctx, SigdumpFileName);
  }
  if (g_sigdump.bit_est)
  {
    const string size[4] = {"blk4_", "blk8_", "blk16_", "blk32_"};
    for (int i = 0; i < 4; i++)
    {
      string SigdumpFileName = g_FileName + "_bit_est_cmd_" + size[i] + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.bit_est_cmd_ctx[i], SigdumpFileName);
      sigdump_output_fprint(&g_sigpool.bit_est_cmd_ctx[i], "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);

      SigdumpFileName = g_FileName + "_bit_est_gld_" + size[i] + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.bit_est_gld_ctx[i], SigdumpFileName);
      sigdump_output_fprint(&g_sigpool.bit_est_gld_ctx[i], "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);
      g_sigpool.bit_est_cnt[i] = 0;
    }
    g_sigpool.bit_est_chroma_4x4_cnt = 0;
  }
  if (g_sigdump.rdo_sse)
  {
    const string size[4] = {"blk4_", "blk8_", "blk16_", "blk32_"};
    for (int i = 0; i < 4; i++)
    {
      string SigdumpFileName = g_FileName + "_rdo_sse_cmd_" + size[i] + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.rdo_sse_cmd_ctx[i], SigdumpFileName);
      sigdump_output_fprint(&g_sigpool.rdo_sse_cmd_ctx[i], "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);

      SigdumpFileName = g_FileName + "_rdo_sse_gld_" + size[i] + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.rdo_sse_gld_ctx[i], SigdumpFileName);
      sigdump_output_fprint(&g_sigpool.rdo_sse_gld_ctx[i], "# [PIC info] PIC = %d\n", g_sigpool.enc_count_pattern);
      g_sigpool.rdo_sse_cnt[i] = 0;
    }
    g_sigpool.rdo_sse_chroma_4x4_cnt = 0;
  }
  if (g_sigdump.cabac)
  {
    if (g_sigdump.cabac_no_output == false)
    {
      string SigdumpFileName = g_FileName + "_cabac_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.cabac_ctx, SigdumpFileName);
      SigdumpFileName = g_FileName + "_cabac_syntax_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.cabac_syntax_ctx, SigdumpFileName);
      SigdumpFileName = g_FileName + "_cabac2ccu_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.cabac2ccu_ctx, SigdumpFileName);
      SigdumpFileName = g_FileName + "_cabac_para_update_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.cabac_para_update_ctx, SigdumpFileName);
      SigdumpFileName = g_FileName + "_cabac_bits_statis_tu_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.cabac_bits_statis_tu_ctx, SigdumpFileName);
      SigdumpFileName = g_FileName + "_slice_data_wo_prev_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
      sigdump_open_ctx_bin(&g_sigpool.slice_data_ctx[1], SigdumpFileName);
    }
    g_sigpool.bits_header = 0;
    g_sigpool.tu_start_bits_pos = 1;
    g_sigpool.cabac_is_record = false;
    g_sigpool.frm_hdr_size = 0;
    g_sigpool.frm_res_size = 0;
  }
  if (g_sigdump.cabac || g_sigdump.fpga)
  {
    string SigdumpFileName = g_FileName + "_slice_data_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.slice_data_ctx[0], SigdumpFileName);
  }
  if (g_sigdump.cabac_prof)
  {
    g_sigpool.ostanding_bit_cnt = 0;
    g_sigpool.nonresi_cabac_bypass_cycles = 0;
    g_sigpool.nonresi_cabac_normal_cycles = 0;
    g_sigpool.resi_cabac_bypass_cycles = 0;
    g_sigpool.resi_cabac_normal_cycles = 0;
  }

#ifdef SIG_RRU
  if (g_sigdump.rru && !g_sigpool.rru_no_output)
  {
    const string cu_size[3] = {"8", "16", "32"};
    const string i_size[3]  = {"4",  "8", "16"};
    const string io[2]  = {"in_", "out_"};
    const string yc[2]  = {"luma", "chroma"};

    string SigdumpFileName;
    for (int i = 0; i < 3; i++)
    {
      SigdumpFileName = g_FileName + "_rru_cu" + cu_size[i] + "_frm_" + to_string(g_sigpool.enc_count_pattern) +
                        "_slice_" + to_string(g_sigpool.slice_count) + "_cmd.txt";
      sigdump_open_ctx_txt(&g_sigpool.rru_cu_ctx[i], SigdumpFileName);

      SigdumpFileName = g_FileName + "_rru_pu" + cu_size[i] + "_intp_frm_" + to_string(g_sigpool.enc_count_pattern) +
                        "_slice_" + to_string(g_sigpool.slice_count) + ".bin";
      sigdump_open_ctx_bin(&g_sigpool.rru_pu_ctx[i], SigdumpFileName);

      SigdumpFileName = g_FileName + "_rru_i" + i_size[i] + "_intp_frm_" + to_string(g_sigpool.enc_count_pattern) +
                        "_slice_" + to_string(g_sigpool.slice_count) + ".bin";
      sigdump_open_ctx_bin(&g_sigpool.rru_i_ctx[i], SigdumpFileName);

      SigdumpFileName = g_FileName + "_rru_cu" + cu_size[i] + "_frm_" + to_string(g_sigpool.enc_count_pattern) +
                        "_slice_" + to_string(g_sigpool.slice_count) + "_gold.txt";
      sigdump_open_ctx_txt(&g_sigpool.rru_cu_gold_ctx[i], SigdumpFileName);

      SigdumpFileName = g_FileName + "_rru_cu" + cu_size[i] + "_iap_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
      sigdump_open_ctx_txt(&g_sigpool.rru_cu_iap_ctx[i], SigdumpFileName);

      for (int io_idx = 0; io_idx < 2; io_idx++)
      {
        for (int yc_idx = 0; yc_idx < 2; yc_idx++)
        {
          SigdumpFileName = g_FileName + "_rru_cu" + i_size[i] + "_intra_dct_" + io[io_idx] + yc[yc_idx] +
                            "_frm_" + to_string(g_sigpool.enc_count_pattern) +
                            "_slice_" + to_string(g_sigpool.slice_count) + ".bin";
          sigdump_open_ctx_txt(&(g_sigpool.rru_intra_dct_ctx[i][io_idx][yc_idx]), SigdumpFileName);

          SigdumpFileName = g_FileName + "_rru_cu" + i_size[i] + "_intra_idct_" + io[io_idx] + yc[yc_idx] +
                            "_frm_" + to_string(g_sigpool.enc_count_pattern) +
                            "_slice_" + to_string(g_sigpool.slice_count) + ".bin";
          sigdump_open_ctx_txt(&g_sigpool.rru_intra_idct_ctx[i][io_idx][yc_idx], SigdumpFileName);

          SigdumpFileName = g_FileName + "_rru_cu" + cu_size[i] + "_inter_dct_" + io[io_idx] + yc[yc_idx] +
                            "_frm_" + to_string(g_sigpool.enc_count_pattern) +
                            "_slice_" + to_string(g_sigpool.slice_count) + ".bin";
          sigdump_open_ctx_txt(&g_sigpool.rru_inter_dct_ctx[i][io_idx][yc_idx], SigdumpFileName);

          SigdumpFileName = g_FileName + "_rru_cu" + cu_size[i] + "_inter_idct_" + io[io_idx] + yc[yc_idx] +
                            "_frm_" + to_string(g_sigpool.enc_count_pattern) +
                            "_slice_" + to_string(g_sigpool.slice_count) + ".bin";
          sigdump_open_ctx_txt(&g_sigpool.rru_inter_idct_ctx[i][io_idx][yc_idx], SigdumpFileName);
        }
      }
    }

    SigdumpFileName = g_FileName + "_rru_rec_nv12_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_txt(&g_sigpool.rru_rec_frm_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_rru_cu_order_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.rru_cu_order_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_rru_i4_rec_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_txt(&g_sigpool.rru_i4_rec_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_rru_i4_resi_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_txt(&g_sigpool.rru_i4_resi_ctx, SigdumpFileName);

#ifdef SIG_RRU_DEBUG
    if (g_sigdump.rru_debug)
    {
      for (int i = 0; i < 3; i++)
      {
        for (int yc_idx = 0; yc_idx < 2; yc_idx++)
        {
          SigdumpFileName = g_FileName + "_rru_cu" + i_size[i] + "_inter_iq_in_" + yc[yc_idx] +
                            "_frm_" + to_string(g_sigpool.enc_count_pattern) +
                            "_slice_" + to_string(g_sigpool.slice_count) + ".bin";
          sigdump_open_ctx_txt(&(g_sigpool.rru_inter_iq_in_ctx[i][yc_idx]), SigdumpFileName);
        }
      }
    }
#endif //~SIG_RRU_DEBUG
  }
  else if (g_sigdump.rru)
  {
    string SigdumpFileName;
    SigdumpFileName = g_FileName + "_rru_cu_order_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.rru_cu_order_ctx, SigdumpFileName);
  }
#endif //~SIG_RRU

#ifdef SIG_PRU
  if (g_sigdump.pru)
  {
    sig_pru_st *p_pru_st = g_sigpool.p_pru_st;

    if (p_pru_st)
    {
      string SigdumpFileName = g_FileName + "_pru_lma_cmd_in_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&p_pru_st->pru_cmd_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_pru_lma_madi_out_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&p_pru_st->pru_madi_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_pru_lma_edg_det_out_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&p_pru_st->pru_edge_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_pru_hist_out_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&p_pru_st->pru_hist_ctx, SigdumpFileName);

      p_pru_st->blk16_id = 0;
      p_pru_st->disable_i4 = 0;
      p_pru_st->madi_blk32_id = 0;
      p_pru_st->last_stat_early_term = 0;
    }
  }
#endif //~SIG_PRU
#ifdef SIG_RESI
  if (g_sigdump.resi) {
    string SigdumpFileName = g_FileName + "_rru_resi_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    g_sigpool.resi_frm_ctx.fp = fopen(SigdumpFileName.c_str(), "w");
    g_sigpool.resi_frm_ctx.enable = true;

    int resi_buf_size = (g_sigpool.heightAlign8 * 8) * g_sigpool.fb_pitch * sizeof(short);
    for (int i = 0; i < 2; i++) {
      if (g_sigpool.p_resi_buf[i])
        memset(g_sigpool.p_resi_buf[i], 0, resi_buf_size >> i);
    }
  }
#endif //~SIG_RESI

#ifdef SIG_CCU
  if (g_sigdump.ccu)
  {
    string SigdumpFileName = g_FileName + "_ccu_init_tab_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_txt(&g_sigpool.ccu_init_tab_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_ccu_qpmap_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_txt(&g_sigpool.ccu_qpmap_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_ccu_resi_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.ccu_resi_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_ccu_resi_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.ccu_resi_txt_ctx, SigdumpFileName);
  }
  if (g_sigdump.ccu || g_sigdump.fpga)
  {
    string SigdumpFileName = g_FileName + "_ccu_stat_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_bin(&g_sigpool.ccu_stat_ctx, SigdumpFileName);
  }
#endif //~SIG_CCU

#ifdef SIG_IAPU
  if (g_sigdump.iapu){
    const string tu_size[3] = {"4", "8", "16"};
    string SigdumpFileName;
    for (int i = 0; i < 3; i++){
        SigdumpFileName = g_FileName + "_iapu_rmd_tu" + tu_size[i] + "_cmd_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.iapu_rmd_tu_cmd_frm_ctx[i], SigdumpFileName);
 
        //SigdumpFileName = g_FileName + "_iapu_rmd_tu" + tu_size[i] + "_src_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
        //sigdump_open_ctx_txt(&g_sigpool.iapu_rmd_tu_src_frm_ctx[i], SigdumpFileName);
        SigdumpFileName = g_FileName + "_iapu_rmd_tu" + tu_size[i] + "_src_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        sigdump_open_ctx_bin(&g_sigpool.iapu_rmd_tu_src_frm_ctx[i], SigdumpFileName);

        SigdumpFileName = g_FileName + "_iapu_rmd_tu" + tu_size[i] + "_cand_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[i], SigdumpFileName);

        //SigdumpFileName = g_FileName + "_iapu_rmd_tu" + tu_size[i] + "_pred_blk_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
        //sigdump_open_ctx_txt(&g_sigpool.iapu_rmd_tu_pred_blk_frm_ctx[i], SigdumpFileName);
        SigdumpFileName = g_FileName + "_iapu_rmd_tu" + tu_size[i] + "_pred_blk_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        sigdump_open_ctx_bin(&g_sigpool.iapu_rmd_tu_pred_blk_frm_ctx[i], SigdumpFileName);

        //
        SigdumpFileName = g_FileName + "_iapu_iap_tu" + tu_size[i] + "_cmd_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[i], SigdumpFileName);

        SigdumpFileName = g_FileName + "_iapu_iap_tu" + tu_size[i] + "_neb_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.iapu_iap_tu_neb_frm_ctx[i], SigdumpFileName);

        SigdumpFileName = g_FileName + "_iapu_iap_tu" + tu_size[i] + "_cand_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.iapu_iap_tu_cand_frm_ctx[i], SigdumpFileName);

        SigdumpFileName = g_FileName + "_iapu_iap_tu" + tu_size[i] + "_src_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        sigdump_open_ctx_bin(&g_sigpool.iapu_iap_tu_src_frm_ctx[i], SigdumpFileName);
    }

    SigdumpFileName = g_FileName + "_iapu_iap_tu4_coef_blk_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.iapu_iap_tu4_coef_blk_frm_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_iapu_iap_tu4_rec_blk_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.iapu_iap_tu4_rec_blk_frm_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_iapu_iap_tu8_rec_blk_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.iapu_iap_tu8_rec_blk_frm_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_iapu_iap_tu16_rec_blk_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.iapu_iap_tu16_rec_blk_frm_ctx, SigdumpFileName);

    //
    SigdumpFileName = g_FileName + "_iapu_iap_cu8_cmd_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.iapu_iap_cu8_cmd_frm_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_iapu_iap_cu8_rec_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.iapu_iap_cu8_rec_frm_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_iapu_iap_line_buffer_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.iapu_iap_line_buffer_frm_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_iapu_iap_tu4_pred_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.iapu_iap_tu4_pred_frm_ctx, SigdumpFileName);

#ifdef IAPU_IAP_SYNTAX_BIN
    SigdumpFileName = g_FileName + "_iapu_iap_syntax_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.iapu_iap_syntax, SigdumpFileName);
#else
    SigdumpFileName = g_FileName + "_iapu_iap_syntax_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.iapu_iap_syntax, SigdumpFileName);
#endif
  }
  if (g_sigdump.iapu_misc){
    const string tu_size[3] = {"4", "8", "16"};
    string SigdumpFileName;
    for (int i = 0; i < 3; i++){
        // misc , tmp
        SigdumpFileName = g_FileName + "_iapu_misc_tu" + tu_size[i] + "_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.iapu_misc_frm_ctx[i], SigdumpFileName);
    }
  }
#endif //~SIG_IAPU

 #ifdef SIG_PPU
  if (g_sigdump.ppu || g_sigdump.ccu)
  {
    string SigdumpFileName;
    SigdumpFileName = g_FileName + "_ppu_input_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.ppu_input_ctx, SigdumpFileName);
  }
  if (g_sigdump.ppu)
  {
    string SigdumpFileName;
  #ifdef SIG_PPU_OLD
    SigdumpFileName = g_FileName + "_ppu_input_raw_data_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.ppu_frm_ctx[0], SigdumpFileName);
    SigdumpFileName = g_FileName + "_ppu_cmd_order_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.ppu_frm_ctx[1], SigdumpFileName);
    SigdumpFileName = g_FileName + "_ppu_cu_tu_depth_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.ppu_frm_ctx[2], SigdumpFileName);
    SigdumpFileName = g_FileName + "_ppu_bs_verticle_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.ppu_frm_ctx[3], SigdumpFileName);
    SigdumpFileName = g_FileName + "_ppu_bs_horizontal_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
    sigdump_open_ctx_txt(&g_sigpool.ppu_frm_ctx[4], SigdumpFileName);
  #endif

    if (g_sigpool.p_ppu_st)
    {
      SigdumpFileName = g_FileName + "_ppu_param_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
      sigdump_open_ctx_bin(&g_sigpool.p_ppu_st->ppu_param_ctx, SigdumpFileName);
      SigdumpFileName = g_FileName + "_ppu_neb_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
      sigdump_open_ctx_bin(&g_sigpool.p_ppu_st->ppu_neb_ctx, SigdumpFileName);
      SigdumpFileName = g_FileName + "_ppu_reg_frm_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.p_ppu_st->ppu_reg_ctx, SigdumpFileName);

      if (g_sigdump.ppu_filter)
      {
        SigdumpFileName = g_FileName + "_ppu_filter_frm_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        sigdump_open_ctx_bin(&g_sigpool.p_ppu_st->ppu_filter_ctx, SigdumpFileName);
      }
    }
  }
#endif //~SIG_PPU

#ifdef SIG_SMT
  if (g_sigdump.smt)
  {
    if (g_sigpool.p_smt_st)
    {
      string SigdumpFileName;
      SigdumpFileName = g_FileName + "_isp_setting_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.p_smt_st->isp_set_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_isp_gld_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.p_smt_st->isp_gld_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_ai_setting_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.p_smt_st->ai_set_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_ai_gld_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.p_smt_st->ai_gld_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_isp_ai_setting_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.p_smt_st->isp_ai_set_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_isp_ai_gld_" + to_string(g_sigpool.enc_count_pattern) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.p_smt_st->isp_ai_gld_ctx, SigdumpFileName);
    }
  }

  if ((g_sigdump.fpga && isEnableSmartEncode()) || g_sigdump.smt)
  {
    string SigdumpFileName;
    SigdumpFileName = g_FileName + "_isp_src_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.isp_src_ctx, SigdumpFileName);

    SigdumpFileName = g_FileName + "_ai_src_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
    sigdump_open_ctx_bin(&g_sigpool.ai_src_ctx, SigdumpFileName);
  }
#endif //~SIG_SMT
  return true;
}

static int get_bit_est_cu_cnt(int blk_size, SliceType slice_type)
{
  if (slice_type == I_SLICE && blk_size == 3)
    return 0;
  else if (slice_type == P_SLICE) {
    if (blk_size == 0)
      return g_sigpool.cu_count[blk_size];
    else
      return g_sigpool.cu_count[blk_size] * ((blk_size != 3) ? 2 : 4);
  }
  return g_sigpool.cu_count[blk_size];
}

static int bit_est_cu_cnt_check(const string ctx_type, int blk_size, SliceType slice_type)
{
    sig_ctx *ctx;
    int pattern_cnt, pattern_4x4_cnt;
    if (ctx_type.compare("bit_est") == 0) {
      ctx = &g_sigpool.bit_est_cmd_ctx[blk_size];
      pattern_cnt = g_sigpool.bit_est_cnt[blk_size];
      pattern_4x4_cnt = g_sigpool.bit_est_chroma_4x4_cnt;
    } else if (ctx_type.compare("rdo_sse") == 0) {
      ctx = &g_sigpool.rdo_sse_cmd_ctx[blk_size];
      pattern_cnt = g_sigpool.rdo_sse_cnt[blk_size];
      pattern_4x4_cnt = g_sigpool.rdo_sse_chroma_4x4_cnt;
    } else {
      printf("bit_est_cu_cnt_check erro type %s\n", ctx_type.c_str());
      return -1;
    }
    if (pattern_cnt != get_bit_est_cu_cnt(blk_size, slice_type)) {
      fseek(ctx->fp, 0, SEEK_SET);
      printf("[ERROR] pattern cnt missmatch %d, %d\n", pattern_cnt, get_bit_est_cu_cnt(blk_size, slice_type));
      sigdump_output_fprint(ctx, "[ERROR] pattern cnt missmatch %d, %d\n", pattern_cnt, get_bit_est_cu_cnt(blk_size, slice_type));
    }
    if (blk_size == 0 && pattern_4x4_cnt != g_sigpool.cu_chroma_4x4_count) {
      fseek(ctx->fp, 0, SEEK_SET);
      printf("[ERROR][chroma] 4x4 pattern cnt missmatch %d, %d\n", pattern_4x4_cnt, g_sigpool.cu_chroma_4x4_count);
      sigdump_output_fprint(ctx, "[ERROR][chroma] 4x4 pattern cnt missmatch %d, %d\n", pattern_4x4_cnt, g_sigpool.cu_chroma_4x4_count);
    }
    return 0;
}

bool sigdump_framewise_disable(SliceType slice_type)
{
  if (g_sigdump.ime_mvp_stats) {
    if (g_sigpool.cu_16x16_2_cand_count != 0) {
      sigdump_output_fprint(&g_sigpool.ime_mvp_stats_ctx, "%d,%d,%d,%d,%d,%d,%d\n",
      g_sigpool.enc_count_pattern, g_sigpool.cu_16x16_count, g_sigpool.cu_16x16_1_cand_count, g_sigpool.cu_16x16_2_cand_count, g_sigpool.cu_16x16_2_cand_count_mod,
      g_sigpool.abs_hor / g_sigpool.cu_16x16_2_cand_count,
      g_sigpool.abs_ver / g_sigpool.cu_16x16_2_cand_count);
    }
  }

  if (g_sigdump.cabac_prof)
  {
    sigdump_output_fprint(&g_sigpool.cabac_profile, "frame %d, cycles\n", g_sigpool.enc_count_pattern);
    sigdump_output_fprint(&g_sigpool.cabac_profile, "non-resi nor, %d\n", g_sigpool.nonresi_cabac_normal_cycles);
    sigdump_output_fprint(&g_sigpool.cabac_profile, "non-resi byp, %d\n", g_sigpool.nonresi_cabac_bypass_cycles);
    sigdump_output_fprint(&g_sigpool.cabac_profile, "resi normal, %d\n", g_sigpool.resi_cabac_normal_cycles);
    sigdump_output_fprint(&g_sigpool.cabac_profile, "resi bypass, %d\n", g_sigpool.resi_cabac_bypass_cycles);
  }

  g_sigpool.enc_count++;
  if(g_sigdump.testBigClipSmall == 1) {
    g_sigpool.enc_count_pattern = g_sigpool.enc_count * 2;
  }
  else if(g_sigdump.testBigClipSmall == 2) {
    g_sigpool.enc_count_pattern = g_sigpool.enc_count * 2 + 1;
  }
  else if(g_sigdump.testBigClipSmall == 0) {
    g_sigpool.enc_count_pattern = g_sigpool.enc_count;
  } 

#ifdef SIG_TOP
  if (g_sigdump.top)
  {
    sigdump_safe_close(&g_sigpool.top_qp_map);
  }
  if (g_sigdump.top && !g_sigdump.fpga)
  {
    for (int i = 0; i < 2; i++)
      sigdump_safe_close(&g_sigpool.top_rec_ctx[i]);
  }
#endif //~SIG_TOP
#ifdef SIG_VBC
  if (g_sigdump.vbc)
  {
    int ch;
    if (g_sigdump.vbc_low)
    {
      sigdump_safe_close(&g_sigpool.vbe_mode_ctx);
      sigdump_safe_close(&g_sigpool.vbe_enc_info_ctx);
      sigdump_safe_close(&g_sigpool.vbe_rec_ctx);
      sigdump_safe_close(&g_sigpool.vbe_bs_ctx);
      sigdump_safe_close(&g_sigpool.vbe_strm_pack_out_ctx);
    }
    sigdump_safe_close(&g_sigpool.vbd_dma_meta);
    for (ch = 0; ch < 2; ch++)
    {
      if (g_sigdump.vbc_low)
      {
        sigdump_safe_close(&g_sigpool.vbe_src_ctx[ch]);
        sigdump_safe_close(&g_sigpool.vbe_lossy_src_ctx[ch]);
        sigdump_safe_close(&g_sigpool.vbe_pack_out_ctx[ch]);
        sigdump_safe_close(&g_sigpool.vbc_dma_cu_info[ch]);
        sigdump_safe_close(&g_sigpool.vbe_pu_str_ctx[0][ch]);
        sigdump_safe_close(&g_sigpool.vbe_pu_str_ctx[1][ch]);
        sigdump_safe_close(&g_sigpool.vbe_pu_str_out_ctx[0][ch]);
        sigdump_safe_close(&g_sigpool.vbe_pu_str_out_ctx[1][ch]);
        sigdump_safe_close(&g_sigpool.vbd_dma_meta_cpx[ch]);
      }
      sigdump_safe_close(&g_sigpool.vbd_dma_bs[ch]);
    }
    if (isEnableVBD())
    {
      sigdump_safe_close(&g_sigpool.vbd_dma_bs_input_nv12);
    }
  }
  if (g_sigdump.vbc_dbg)
  {
    for (int ch = 0; ch < 2; ch++)
    {
      sigdump_safe_close(&g_sigpool.vbe_meta_debug_ctx[ch]);
    }
  }
#endif
  if (g_sigdump.input_src)
  {
    if (g_sigpool.input_f)
      fclose(g_sigpool.input_f);
    g_sigpool.input_f = NULL;

    if (g_sigpool.input_rotate_f)
      fclose(g_sigpool.input_rotate_f);
    g_sigpool.input_rotate_f = NULL;
  }
  if (g_sigdump.irpu)
  {
      for (int i = 0; i < 3; i++)
      {
        sigdump_safe_close(&g_sigpool.irpu_cmd_frm_ctx[i]);
        sigdump_safe_close(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[i]);
        sigdump_safe_close(&g_sigpool.irpu_golden_frm_ctx[i]);
      }
      sigdump_safe_close(&g_sigpool.irpu_col_frm_ctx);
      sigdump_safe_close(&g_sigpool.irpu_reg_init_ctx);
  }
  if (g_sigdump.ime_mvp)
  {
    sigdump_safe_close(&g_sigpool.ime_mvp_ctx);
    sigdump_safe_close(&g_sigpool.ime_mvb_ctx);
  }
  if (g_sigdump.col)
  {
    sigdump_safe_close(&g_sigpool.col_ctx);
  }
  if (g_sigdump.fme)
  {
      for (int i = 0; i < 2; i++)
      {
        sigdump_safe_close(&g_sigpool.fme_cmd_in_ctx[i]);
        sigdump_safe_close(&g_sigpool.fme_dat_out_ctx[i]);

        if (g_sigdump.fme_debug) {
          sigdump_safe_close(&g_sigpool.fme_dat_out_txt_ctx[i]);
        }
      }
  }
  if (g_sigdump.ime)
  {
    sigdump_safe_close(&g_sigpool.ime_cmd_in_ctx);
    sigdump_safe_close(&g_sigpool.ime_dat_out_ctx);
    sigdump_safe_close(&g_sigpool.ime_dat_out_bin_ctx);
  }
  if (g_sigdump.mc)
  {
      for (int i = 0; i < 3; i++)
      {
        sigdump_safe_close(&g_sigpool.mc_mrg_ctx_cmd_in[i]);
        for (int j = 0; j < 2; j++)
        {
          sigdump_safe_close(&g_sigpool.mc_mrg_ctx_dat_out[i][j]);
        }
      }
  }
  if (g_sigdump.rec_yuv)
  {
    sigdump_safe_close(&g_sigpool.mc_rec_ctx);
  }
  if (g_sigdump.bit_est)
  {
    for (int i = 0; i < 4; i++)
    {
      bit_est_cu_cnt_check("bit_est", i, slice_type);
      sigdump_safe_close(&g_sigpool.bit_est_cmd_ctx[i]);
      sigdump_safe_close(&g_sigpool.bit_est_gld_ctx[i]);
    }
  }

  if (g_sigdump.rdo_sse)
  {
    for (int i = 0; i < 4; i++)
    {
      sigdump_safe_close(&g_sigpool.rdo_sse_cmd_ctx[i]);
      sigdump_safe_close(&g_sigpool.rdo_sse_gld_ctx[i]);
    }
  }

  if (g_sigdump.cabac && g_sigdump.cabac_no_output == false)
  {
    sigdump_safe_close(&g_sigpool.cabac_ctx);
    sigdump_safe_close(&g_sigpool.cabac_syntax_ctx);
    sigdump_output_fprint(&g_sigpool.cabac2ccu_ctx, "bs_sz_wo_prev = %x\n", g_sigpool.slice_data_size[1]);
    sigdump_output_fprint(&g_sigpool.cabac2ccu_ctx, "bs_sz = %x\n", g_sigpool.slice_data_size[0]);
    sigdump_output_fprint(&g_sigpool.cabac2ccu_ctx, "bs_sz_w_hdr = %x\n", g_sigpool.slice_data_size[0] + g_sigpool.slice_header_size);
    sigdump_safe_close(&g_sigpool.cabac2ccu_ctx);
    sigdump_safe_close(&g_sigpool.cabac_para_update_ctx);
    sigdump_safe_close(&g_sigpool.cabac_bits_statis_tu_ctx);
    sigdump_safe_close(&g_sigpool.slice_data_ctx[1]);
  }

  if (g_sigdump.cabac || g_sigdump.fpga)
  {
      sigdump_safe_close(&g_sigpool.slice_data_ctx[0]);
  }
#ifdef SIG_RESI
  if (g_sigdump.resi)
    sigdump_safe_close(&g_sigpool.resi_frm_ctx);
#endif //~SIG_RESI
#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    for (int i = 0; i < 3; i++)
    {
      sigdump_safe_close(&g_sigpool.rru_cu_ctx[i]);
      sigdump_safe_close(&g_sigpool.rru_pu_ctx[i]);
      sigdump_safe_close(&g_sigpool.rru_i_ctx[i]);
      sigdump_safe_close(&g_sigpool.rru_cu_gold_ctx[i]);
      sigdump_safe_close(&g_sigpool.rru_cu_iap_ctx[i]);

      for (int j = 0; j < 2; j ++)
      {
        for (int k = 0; k < 2; k ++)
        {
          sigdump_safe_close(&g_sigpool.rru_intra_dct_ctx[i][j][k]);
          sigdump_safe_close(&g_sigpool.rru_intra_idct_ctx[i][j][k]);
          sigdump_safe_close(&g_sigpool.rru_inter_dct_ctx[i][j][k]);
          sigdump_safe_close(&g_sigpool.rru_inter_idct_ctx[i][j][k]);
        }
      }
    }
    sigdump_safe_close(&g_sigpool.rru_rec_frm_ctx);
    sigdump_safe_close(&g_sigpool.rru_cu_order_ctx);
    sigdump_safe_close(&g_sigpool.rru_i4_rec_ctx);
    sigdump_safe_close(&g_sigpool.rru_i4_resi_ctx);

#ifdef SIG_RRU_DEBUG
    if (g_sigdump.rru_debug)
    {
      for (int i = 0; i < 3; i++)
      {
        for (int yc_idx = 0; yc_idx < 2; yc_idx++)
          sigdump_safe_close(&g_sigpool.rru_inter_iq_in_ctx[i][yc_idx]);
      }
    }
#endif
  }
#endif //~SIG_RRU

#ifdef SIG_PRU
  if (g_sigdump.pru)
  {
    sig_pru_st *p_pru_st = g_sigpool.p_pru_st;
    if (p_pru_st)
    {
      sigdump_safe_close(&p_pru_st->pru_cmd_ctx);
      sigdump_safe_close(&p_pru_st->pru_madi_ctx);
      sigdump_safe_close(&p_pru_st->pru_edge_ctx);
      sigdump_safe_close(&p_pru_st->pru_hist_ctx);
    }
  }
#endif //~SIG_PRU

#ifdef SIG_CCU
  if (g_sigdump.ccu)
  {
    sigdump_safe_close(&g_sigpool.ccu_init_tab_ctx);
    sigdump_safe_close(&g_sigpool.ccu_qpmap_ctx);
    sigdump_safe_close(&g_sigpool.ccu_resi_ctx);
    sigdump_safe_close(&g_sigpool.ccu_resi_txt_ctx);
  }
  if (g_sigdump.ccu || g_sigdump.fpga)
  {
    sigdump_safe_close(&g_sigpool.ccu_stat_ctx);
  }
#endif //~SIG_CCU

#ifdef SIG_IAPU
  if (g_sigdump.iapu){
    for (int i = 0; i < 3; i++){
      sigdump_safe_close(&g_sigpool.iapu_rmd_tu_cmd_frm_ctx[i]);
      sigdump_safe_close(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[i]);
      sigdump_safe_close(&g_sigpool.iapu_rmd_tu_src_frm_ctx[i]);
      sigdump_safe_close(&g_sigpool.iapu_rmd_tu_pred_blk_frm_ctx[i]);
      //
      sigdump_safe_close(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[i]);
      sigdump_safe_close(&g_sigpool.iapu_iap_tu_neb_frm_ctx[i]);
      sigdump_safe_close(&g_sigpool.iapu_iap_tu_cand_frm_ctx[i]);
      sigdump_safe_close(&g_sigpool.iapu_iap_tu_src_frm_ctx[i]);
    }
    sigdump_safe_close(&g_sigpool.iapu_iap_tu4_coef_blk_frm_ctx);
    sigdump_safe_close(&g_sigpool.iapu_iap_tu4_rec_blk_frm_ctx);
    sigdump_safe_close(&g_sigpool.iapu_iap_tu8_rec_blk_frm_ctx);
    sigdump_safe_close(&g_sigpool.iapu_iap_tu16_rec_blk_frm_ctx);
    //
    sigdump_safe_close(&g_sigpool.iapu_iap_cu8_cmd_frm_ctx);
    sigdump_safe_close(&g_sigpool.iapu_iap_cu8_rec_frm_ctx);
    sigdump_safe_close(&g_sigpool.iapu_iap_line_buffer_frm_ctx);
    sigdump_safe_close(&g_sigpool.iapu_iap_tu4_pred_frm_ctx);
    sigdump_safe_close(&g_sigpool.iapu_iap_syntax);

    //! trick : copy iap_tu8_src to iap_tu4_src
    FILE *istream;
    FILE *ostream;
    if(g_sigdump.testBigClipSmall == 0)
    {
      istream = fopen(std::string(g_FileName + "_iapu_iap_tu8_src_frm_" + to_string(g_sigpool.enc_count_pattern-1) + ".bin").c_str(), "rb");
      ostream = fopen(std::string(g_FileName + "_iapu_iap_tu4_src_frm_" + to_string(g_sigpool.enc_count_pattern-1) + ".bin").c_str(), "wb");
    }
    else
    {
      istream = fopen(std::string(g_FileName + "_iapu_iap_tu8_src_frm_" + to_string(g_sigpool.enc_count_pattern-2) + ".bin").c_str(), "rb");
      ostream = fopen(std::string(g_FileName + "_iapu_iap_tu4_src_frm_" + to_string(g_sigpool.enc_count_pattern-2) + ".bin").c_str(), "wb");
    }

    if (istream != 0 && ostream != 0){
        const int len = 4096;
        char buf[4096];
        while(1)
        {
            if( feof(istream))
                break;
            int nBytesRead = fread(buf, 1, len, istream);
            if(nBytesRead <= 0)
                break;
            fwrite(buf, 1, nBytesRead, ostream);
        }
        fclose(istream);
        fclose(ostream);
    }
  }
  if (g_sigdump.iapu_misc){
    for (int i = 0; i < 3; i++){
      sigdump_safe_close(&g_sigpool.iapu_misc_frm_ctx[i]);
    }
  }
#endif //~SIG_IAPU

#ifdef SIG_PPU
  if (g_sigdump.ppu || g_sigdump.ccu)
  {
    sigdump_safe_close(&g_sigpool.ppu_input_ctx);
  }
  if (g_sigdump.ppu)
  {
#ifdef SIG_PPU_OLD
    sigdump_safe_close(&g_sigpool.ppu_frm_ctx[0]);
    sigdump_safe_close(&g_sigpool.ppu_frm_ctx[1]);
    sigdump_safe_close(&g_sigpool.ppu_frm_ctx[2]);
    sigdump_safe_close(&g_sigpool.ppu_frm_ctx[3]);
    sigdump_safe_close(&g_sigpool.ppu_frm_ctx[4]);
#endif

    if (g_sigpool.p_ppu_st)
    {
      sigdump_safe_close(&g_sigpool.p_ppu_st->ppu_param_ctx);
      sigdump_safe_close(&g_sigpool.p_ppu_st->ppu_neb_ctx);
      sigdump_safe_close(&g_sigpool.p_ppu_st->ppu_reg_ctx);

      if (g_sigdump.ppu_filter)
        sigdump_safe_close(&g_sigpool.p_ppu_st->ppu_filter_ctx);
    }
  }
#endif //~SIG_PPU

#ifdef SIG_SMT
  if (g_sigdump.smt)
  {
    sig_smt_st *p_smt_st = g_sigpool.p_smt_st;
    if (p_smt_st)
    {
      sigdump_safe_close(&p_smt_st->isp_set_ctx);
      sigdump_safe_close(&p_smt_st->isp_gld_ctx);
      sigdump_safe_close(&p_smt_st->ai_set_ctx);
      sigdump_safe_close(&p_smt_st->ai_gld_ctx);
      sigdump_safe_close(&p_smt_st->isp_ai_set_ctx);
      sigdump_safe_close(&p_smt_st->isp_ai_gld_ctx);
    }
  }

  sigdump_safe_close(&g_sigpool.isp_src_ctx);
  sigdump_safe_close(&g_sigpool.ai_src_ctx);
#endif
  return true;
}

bool sigdump_slicewise_enable(int pocCurr)
{
  bool enable = false;

  if (g_sigpool.enc_count >= g_sigdump.start_count && g_sigpool.enc_count <= g_sigdump.end_count)
    enable = true;
  if (!enable)
    return false;

  if (g_sigdump.bit_est)
  {
    for (int i = 0; i < 4; i++) {
      sigdump_output_fprint(&g_sigpool.bit_est_cmd_ctx[i], "# [SLICE info] SLICE ID = %d\n", g_sigpool.slice_count);
      sigdump_output_fprint(&g_sigpool.bit_est_gld_ctx[i], "# [SLICE info] SLICE ID = %d\n", g_sigpool.slice_count);
    }
  }
  if (g_sigdump.rdo_sse)
  {
    for (int i = 0; i < 4; i++) {
      sigdump_output_fprint(&g_sigpool.rdo_sse_cmd_ctx[i], "# [SLICE info] SLICE ID = %d\n", g_sigpool.slice_count);
      sigdump_output_fprint(&g_sigpool.rdo_sse_gld_ctx[i], "# [SLICE info] SLICE ID = %d\n", g_sigpool.slice_count);
    }
  }
  if (g_sigdump.irpu)
  {
    for (int i = 0; i < 3; i++)
    {
      sigdump_output_fprint(&g_sigpool.irpu_cmd_frm_ctx[i], "# [SLICE info] SLICE ID = %d\n", g_sigpool.slice_count);
      sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[i], "# [SLICE info] SLICE ID = %d\n", g_sigpool.slice_count);
      sigdump_output_fprint(&g_sigpool.irpu_golden_frm_ctx[i], "# [SLICE info] SLICE ID = %d\n", g_sigpool.slice_count);
    }
    if (g_sigpool.slice_count == 0) {
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_cur_poc = %x\n", pocCurr);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_pic_width_ctu_m1 = %x\n", g_sigpool.widthAlignCTB - 1);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_pic_height_ctu_m1 = %x\n", g_sigpool.heightAlignCTB - 1);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_pic_width_cu_m1 = %x\n", g_sigpool.widthAlign8 - 1);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_pic_height_cu_m1 = %x\n", g_sigpool.heightAlign8 - 1);
      sigdump_output_fprint(&g_sigpool.irpu_reg_init_ctx, "reg_ctu_sz = %x\n", g_sigpool.ctb_size == 64 ? 4 : (g_sigpool.ctb_size == 32) ? 3 : 2);
    }
    sigdump_output_fprint(&g_sigpool.irpu_col_frm_ctx, "# [SLICE info] SLICE ID = %d\n", g_sigpool.slice_count);
  }
  if (g_sigdump.ime_mvp)
  {
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "# Slice = %d\n", g_sigpool.slice_count);
  }
  if (g_sigdump.ccu)
  {
      string SigdumpFileName;
      const string size[4] = {"blk4_", "blk8_", "blk16_", "blk32_"};
      for (int i = 0; i < 4; i++)
      {
        SigdumpFileName = g_FileName + "_ccu_" + size[i] + "frm_" + to_string(g_sigpool.enc_count_pattern) + "_slice_" + to_string(g_sigpool.slice_count) + ".txt";
        sigdump_open_ctx_txt(&g_sigpool.ccu_ctx[i], SigdumpFileName);
      }
      SigdumpFileName = g_FileName + "_ccu_param_frm_" + to_string(g_sigpool.enc_count_pattern) + "_slice_" + to_string(g_sigpool.slice_count) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.ccu_param_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_ccu_cost_frm_" + to_string(g_sigpool.enc_count_pattern) + "_slice_" + to_string(g_sigpool.slice_count) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.ccu_cost_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_ccu_ime_frm_" + to_string(g_sigpool.enc_count_pattern) + "_slice_" + to_string(g_sigpool.slice_count) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.ccu_ime_ctx, SigdumpFileName);

      SigdumpFileName = g_FileName + "_ccu_rc_frm_" + to_string(g_sigpool.enc_count_pattern) + "_slice_" + to_string(g_sigpool.slice_count) + ".txt";
      sigdump_open_ctx_txt(&g_sigpool.ccu_rc_ctx, SigdumpFileName);
  }

#ifdef SIG_TOP
  if (g_sigdump.top)
  {
    string SigdumpFileName = g_FileName + "_frm_" + to_string(g_sigpool.enc_count_pattern) + "_slice_" + to_string(g_sigpool.slice_count) + "_fw.txt";
    sigdump_open_ctx_txt(&g_sigpool.top_fw_ctx, SigdumpFileName);
    SigdumpFileName = g_FileName + "_frm_" + to_string(g_sigpool.enc_count) + "_slice_" + to_string(g_sigpool.slice_count) + "_fw_cmdq.bin";
    sigdump_open_ctx_txt(&g_sigpool.top_fw_cmdq_ctx, SigdumpFileName);
    SigdumpFileName = g_FileName + "_frm_" + to_string(g_sigpool.enc_count) + "_slice_" + to_string(g_sigpool.slice_count) + "_fw_cmdq.txt";
    sigdump_open_ctx_txt(&g_sigpool.top_fw_cmdq_addr_ctx, SigdumpFileName);
  }
#endif //~SIG_TOP

  return true;
}

bool sigdump_slicewise_disable(void)
{
  if (g_sigdump.ccu)
  {
    for (int i = 0; i < 4; i++)
    {
      sigdump_safe_close(&g_sigpool.ccu_ctx[i]);
    }
    sigdump_safe_close(&g_sigpool.ccu_cost_ctx);
    sigdump_safe_close(&g_sigpool.ccu_param_ctx);
    sigdump_safe_close(&g_sigpool.ccu_ime_ctx);
    sigdump_safe_close(&g_sigpool.ccu_rc_ctx);
  }

#ifdef SIG_TOP
  if (g_sigdump.top)
  {
    sigdump_safe_close(&g_sigpool.top_fw_ctx);
    sigdump_safe_close(&g_sigpool.top_fw_cmdq_ctx);
    sigdump_safe_close(&g_sigpool.top_fw_cmdq_addr_ctx);
  }
#endif //~SIG_TOP

  return true;
}

bool sigdump_cu_start(void)
{
  g_sigpool.cu_count[sig_pat_blk_size_4(g_sigpool.cu_width)]++;
#ifdef SIG_IRPU
  if (g_sigdump.irpu) {
    int blk_size = sig_pat_blk_size_8(g_sigpool.cu_width);
    sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[blk_size], "# [CU info] (CU_X, CU_Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
  }
#endif
#ifdef SIG_CCU
  if (g_sigdump.ccu) {
    bool is_use_8x8 = is_use_8x8_ime_mvp(g_sigpool.cu_idx_x, g_sigpool.cu_idx_y, g_sigpool.width, g_sigpool.height);
    int blk_size = sig_pat_blk_size_4(g_sigpool.cu_width);
    sigdump_output_fprint(&g_sigpool.ccu_ctx[blk_size], "# CU (x, y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);

    if (g_sigpool.cu_width >= 16)
      sigdump_output_fprint(&g_sigpool.ccu_cost_ctx, "# CU (x, y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);

    if ((g_sigpool.cu_idx_x % 16) == 0 && (g_sigpool.cu_idx_y % 16) == 0 && (g_sigpool.cu_width == 16 || is_use_8x8)) {
      memset(&g_sigpool.blk16_mv, 0x0, sizeof(sig_ime_mvp_st));
      memset(&g_sigpool.blk8_mv, 0x0, sizeof(sig_ime_mvp_st) * 4);
    }
    if (g_sigpool.cu_width == 16 || is_use_8x8) {
      sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "# CU (x, y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    }
  }
#endif

#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    int idx = sig_pat_blk_size(g_sigpool.cu_width);
    sigdump_output_fprint(&g_sigpool.rru_cu_ctx[idx], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    sigdump_output_fprint(&g_sigpool.rru_cu_gold_ctx[idx], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);

    for (int j = 0; j < 2; j++)
    {
      for (int k = 0; k < 2; k++)
      {
#ifdef SIG_RRU_TXT
        // inter
        sigdump_output_fprint(&g_sigpool.rru_inter_dct_ctx[idx][j][k], "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_inter_dct_ctx[idx][j][k], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_inter_idct_ctx[idx][j][k], "# CTB (X, Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
        sigdump_output_fprint(&g_sigpool.rru_inter_idct_ctx[idx][j][k], "# CU (X, Y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
#else
        sigdump_output_bin(&g_sigpool.rru_inter_dct_ctx[idx][j][k], (unsigned char *)(&g_sigpool.ctb_idx_x), 2);
        sigdump_output_bin(&g_sigpool.rru_inter_dct_ctx[idx][j][k], (unsigned char *)(&g_sigpool.ctb_idx_y), 2);
        sigdump_output_bin(&g_sigpool.rru_inter_dct_ctx[idx][j][k], (unsigned char *)(&g_sigpool.cu_idx_x), 2);
        sigdump_output_bin(&g_sigpool.rru_inter_dct_ctx[idx][j][k], (unsigned char *)(&g_sigpool.cu_idx_y), 2);

        sigdump_output_bin(&g_sigpool.rru_inter_idct_ctx[idx][j][k], (unsigned char *)(&g_sigpool.ctb_idx_x), 2);
        sigdump_output_bin(&g_sigpool.rru_inter_idct_ctx[idx][j][k], (unsigned char *)(&g_sigpool.ctb_idx_y), 2);
        sigdump_output_bin(&g_sigpool.rru_inter_idct_ctx[idx][j][k], (unsigned char *)(&g_sigpool.cu_idx_x), 2);
        sigdump_output_bin(&g_sigpool.rru_inter_idct_ctx[idx][j][k], (unsigned char *)(&g_sigpool.cu_idx_y), 2);
#endif
      }
    }

    memset(&g_sigpool.rru_temp_gold, 0, sizeof(sig_rru_gold_st));
    memset(&g_sigpool.rru_gold, 0, sizeof(sig_rru_gold_st));
    if (g_sigpool.cu_width == 32)
    {
      memset(g_sigpool.rru_intra_cost, 0, sizeof(g_sigpool.rru_intra_cost));
      memset(g_sigpool.rru_intra_cost_wt, 0, sizeof(g_sigpool.rru_intra_cost_wt));
    }
    else if (g_sigpool.cu_width == 16)
    {
      g_sigpool.rru_intra_cost_i8x4 = 0.0;
    }

    g_sigpool.rru_is_mrg_win = 0;
    g_sigpool.rru_merge_cand = 0;
    g_sigpool.rru_scan_idx[0] = g_sigpool.rru_scan_idx[1] = 0;
  }
#endif //~SIG_RRU
#ifdef SIG_IAPU
  if (g_sigdump.iapu)
  {
    if (g_sigpool.cu_width == 32)
      memset(g_sigpool.iapu_st.iapu_intra_cost, 0, sizeof(g_sigpool.iapu_st.iapu_intra_cost));
    else if (g_sigpool.cu_width == 16)
      g_sigpool.iapu_st.iapu_intra_cost_i8x4 = 0.0;
  }
#endif //~SIG_IAPU  
  return true;
}

bool sigdump_ctb_enc_start(const unsigned int idx_x, const unsigned int idx_y)
{
  if (g_sigdump.cabac)
  {
    sigdump_output_fprint(&g_sigpool.cabac_ctx, "# [CTU info] (CTU_X, CTU_Y) = (%d, %d)\n", idx_x, idx_y);
    sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, "# [CTU info] (CTU_X, CTU_Y) = (%d, %d)\n", idx_x, idx_y);
    sigdump_output_fprint(&g_sigpool.cabac2ccu_ctx, "# [CTU info] (CTU_X, CTU_Y) = (%d, %d)\n", idx_x, idx_y);
    g_sigpool.hdr_bits_num = 0;
    g_sigpool.res_bits_num = 0;
  }
  return true;
}

bool sigdump_ctb_enc_end(void)
{
  if (g_sigdump.cabac)
  {
    sigdump_output_fprint(&g_sigpool.cabac2ccu_ctx, "hdr_bits_nr = %x\n", g_sigpool.hdr_bits_num);
    sigdump_output_fprint(&g_sigpool.cabac2ccu_ctx, "res_bits_nr = %x\n", g_sigpool.res_bits_num);
    g_sigpool.frm_hdr_size += g_sigpool.hdr_bits_num;
    g_sigpool.frm_res_size += g_sigpool.res_bits_num;
  }
  return true;
}

bool sigdump_ctb_start(void)
{
  if (g_sigdump.ime_mvp)
  {
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "# CTU (x, y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
  }

  if (g_sigdump.irpu)
  {
    for (int i = 0; i < 3; i++)
    {
      sigdump_output_fprint(&g_sigpool.irpu_cmd_frm_ctx[i], "# [CTU info] (CTU_X, CTU_Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
      sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[i], "# [CTU info] (CTU_X, CTU_Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
      sigdump_output_fprint(&g_sigpool.irpu_golden_frm_ctx[i], "# [CTU info] (CTU_X, CTU_Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
    }
    sigdump_output_fprint(&g_sigpool.irpu_col_frm_ctx, "# [CTU info] (CTU_X, CTU_Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
  }

#ifdef SIG_CCU
  if (g_sigdump.ccu)
  {
    memset(g_sigpool.p_ccu_rc_st, 0, sizeof(sig_ccu_rc_st));
  }
#endif //~SIG_CCU
  if (g_sigdump.bit_est)
  {
    for (int i = 0; i < 4; i++) {
      sigdump_output_fprint(&g_sigpool.bit_est_cmd_ctx[i], "# [CTU info] (CTU_X, CTU_Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
      sigdump_output_fprint(&g_sigpool.bit_est_gld_ctx[i], "# [CTU info] (CTU_X, CTU_Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
    }
  }
  if (g_sigdump.rdo_sse)
  {
    for (int i = 0; i < 4; i++) {
      sigdump_output_fprint(&g_sigpool.rdo_sse_cmd_ctx[i], "# [CTU info] (CTU_X, CTU_Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
      sigdump_output_fprint(&g_sigpool.rdo_sse_gld_ctx[i], "# [CTU info] (CTU_X, CTU_Y) = (%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
    }
  }

#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    if (g_sigpool.ctb_count == 0)
    {
      for (int i = 0; i < 3; i++)
      {
        sigdump_output_fprint(&g_sigpool.rru_cu_ctx[i], "# PIC = %d\n", g_sigpool.enc_count_pattern);
        sigdump_output_fprint(&g_sigpool.rru_cu_ctx[i], "# reg_src_width_m1 = %d\n", g_sigpool.width - 1);
        sigdump_output_fprint(&g_sigpool.rru_cu_ctx[i], "# reg_src_height_m1 = %d\n", g_sigpool.height - 1);
        sigdump_output_fprint(&g_sigpool.rru_cu_ctx[i], "# reg_src_luma_pit = %d\n", g_sigpool.width);
        sigdump_output_fprint(&g_sigpool.rru_cu_ctx[i], "# reg_src_chroma_pit = %d\n", (g_sigpool.width >> 1));
        sigdump_output_fprint(&g_sigpool.rru_cu_ctx[i], "# reg_src_fmt = 1\n");
        sigdump_output_fprint(&g_sigpool.rru_cu_ctx[i], "# reg_i_slice = %d\n", (g_sigpool.slice_type == 2) ? 1 : 0);
        sigdump_output_fprint(&g_sigpool.rru_cu_ctx[i], "# reg_dist_chroma_weight = %d\n", g_sigpool.dist_chroma_weight);

        sigdump_output_fprint(&g_sigpool.rru_cu_gold_ctx[i], "# PIC = %d\n", g_sigpool.enc_count_pattern);
      }
    }

    sigdump_output_fprint(&g_sigpool.rru_cu_order_ctx, "# CTB(%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
  }
#endif //~SIG_RRU

#ifdef SIG_PPU
  if (g_sigdump.ppu || g_sigdump.ccu)
  {
    sigdump_output_fprint(&g_sigpool.ppu_input_ctx, "# CTB(%d, %d)\n", g_sigpool.ctb_idx_x, g_sigpool.ctb_idx_y);
  }
#endif //~SIG_PPU

#ifdef SIG_PRU
  if (g_sigdump.pru)
  {
    if (g_sigpool.p_pru_st)
      memset(g_sigpool.p_pru_st->stat_early_term, 0xff, sizeof(g_sigpool.p_pru_st->stat_early_term));
  }
#endif //~SIG_PRU

  return true;
}

bool sigdump_ctb_end(void)
{
  g_sigpool.ctb_count++;
  return true;
}

bool sigdump_output_bin(const sig_ctx *ctx, unsigned char * buf_ptr, unsigned int byte_size)
{
  if (!ctx->enable)
    return false;

  assert(ctx->fp != NULL);
  size_t write_size = fwrite(buf_ptr, 1, byte_size, ctx->fp);

  if (write_size != byte_size)
    return false;

  return true;
}

bool sigdump_output_fprint(const sig_ctx *ctx, const char *format, ...)
{
  if (!ctx->enable)
    return false;

  assert(ctx->fp != NULL);
  va_list args;

  va_start(args, format);
  vfprintf(ctx->fp, format, args);
  va_end(args);
  //fflush(ctx->fp);
  return true;
}

int sigdump_vfscanf(const sig_ctx *ctx, const char * format, ...)
{
  if (!ctx->enable)
    return 0;

  assert(ctx->fp != NULL);

  int result;
  va_list arglist;
  va_start(arglist, format);
  result = vfscanf(ctx->fp, format, arglist);
  va_end(arglist);
  return result;
}

#ifdef SIG_BIT_EST
void sigdump_output_rdo_sse_inter(int blk_size)
{
  sig_ctx *rdo_ctx = &g_sigpool.rdo_sse_gld_ctx[blk_size];
  sig_est_golden_st *pbest_est_golden = &g_sigpool.est_golden[blk_size][1];
  //tu partition
  const int tu_cnt = (blk_size  == 3) ? 4 : 1;

  int tu_size = (blk_size >= 2) ? 16 : (blk_size == 1) ? 8 : 4;

  for (int tu = 0; tu < tu_cnt; tu++) {
    g_sigpool.rdo_sse_cnt[blk_size] ++;
    sigdump_output_fprint(rdo_ctx, "#(cu_x, cu_y = %d, %d), (tu_x, tu_y = %d, %d)\n",
      g_sigpool.cu_idx_x, g_sigpool.cu_idx_y, (tu & 1) * tu_size, (tu >> 1) * tu_size);

    Distortion CVI_SSE = pbest_est_golden->EST_SSE[0][tu];
    Distortion CVI_DIST = CVI_SSE;
    sigdump_output_fprint(rdo_ctx, "%llx, %llx\n", CVI_SSE, CVI_DIST);

    //cb
    CVI_SSE = pbest_est_golden->EST_SSE[1][tu];
    CVI_DIST = ((Distortion) ((g_sigpool.chromaWeight * CVI_SSE) >> CHROMA_WEIGHT_FRAC_BIT));
    sigdump_output_fprint(rdo_ctx, "%llx, %llx\n", CVI_SSE, CVI_DIST);

    //(cb + cr)
    CVI_SSE += pbest_est_golden->EST_SSE[2][tu];
    CVI_DIST = ((Distortion) ((g_sigpool.chromaWeight * CVI_SSE) >> CHROMA_WEIGHT_FRAC_BIT));
    sigdump_output_fprint(rdo_ctx, "%llx, %llx\n", CVI_SSE, CVI_DIST);
  }

  rdo_ctx = &g_sigpool.rdo_sse_cmd_ctx[blk_size];
  //tu partition
  for (int tu = 0; tu < tu_cnt; tu++) {
    int tu_size = (blk_size >= 2) ? 16 : (blk_size == 1) ? 8 : 4;
    sigdump_output_fprint(rdo_ctx, "#(cu_x, cu_y = %d, %d), (tu_x, tu_y = %d, %d)\n",
    g_sigpool.cu_idx_x, g_sigpool.cu_idx_y, (tu & 1) * tu_size, (tu >> 1) * tu_size);
    for (int compID = 0; compID < 3; compID++) {
      if (compID == 1)
        tu_size /= 2;
      for (int coeff_type = 0; coeff_type < 2; coeff_type++) {
        int *pCoeff = (coeff_type == 0) ? pbest_est_golden->TCoeff[compID][tu]
          : pbest_est_golden->IQCoeff[compID][tu];

        sigdump_output_fprint(rdo_ctx, "#compID = %d, %s\n", compID, coeff_type == 0 ? "DCT" : "IQ");
        for (int i = 0; i < tu_size; i++) {
          for (int j = 0; j < tu_size; j++) {
            sigdump_output_fprint(rdo_ctx, "%4d,", pCoeff[i * tu_size + j]);
          }
          sigdump_output_fprint(rdo_ctx, "\n");
        }
      }
    }
  }
}

void sigdump_output_rdo_sse_intra(int blk_size, unsigned int uiPartOffset, int comp)
{
  sig_est_golden_intra_st *pbest_est_golden_intra = g_sigpool.est_golden_intra[1];
  sig_ctx *rdo_ctx = &g_sigpool.rdo_sse_gld_ctx[blk_size];
  int idx_x = g_sigpool.cu_idx_x + 4 * (uiPartOffset & 1);
  int idx_y = g_sigpool.cu_idx_y + 2 * (uiPartOffset & 2);
  if (comp == 0) {
    sigdump_output_fprint(rdo_ctx, "#(cu_x, cu_y = %d, %d), (tu_x, tu_y = %d, %d)\n",
      idx_x, idx_y, idx_x, idx_y);
    g_sigpool.rdo_sse_cnt[blk_size] ++;
  } else if (comp == 1 && blk_size == 0) {
    g_sigpool.rdo_sse_chroma_4x4_cnt ++;
  }
  Distortion CVI_SSE = pbest_est_golden_intra[comp].EST_SSE;
  Distortion CVI_DIST = CVI_SSE;

  if (comp != COMPONENT_Y)
  {
    if (comp == COMPONENT_Cr && blk_size != 0)
    {
      //cb + cr
      CVI_SSE += pbest_est_golden_intra[1].EST_SSE;
    }
    CVI_DIST = ((Distortion)((g_sigpool.chromaWeight * CVI_SSE) >> CHROMA_WEIGHT_FRAC_BIT));
  }
  sigdump_output_fprint(rdo_ctx, "%llx, %llx\n", CVI_SSE, CVI_DIST);

  rdo_ctx = &g_sigpool.rdo_sse_cmd_ctx[blk_size];
  if (comp == 0)
    sigdump_output_fprint(rdo_ctx, "#(cu_x, cu_y = %d, %d), (tu_x, tu_y = %d, %d)\n",
      idx_x, idx_y, idx_x, idx_y);


  int tu_size = (blk_size >= 2) ? 16 : (blk_size == 1) ? 8 : 4;
  if (comp != 0 && tu_size != 4)
    tu_size /= 2;

  for (int coeff_type = 0; coeff_type < 2; coeff_type++) {
    int *pCoeff = (coeff_type == 0) ? pbest_est_golden_intra[comp].TCoeff
      : pbest_est_golden_intra[comp].IQCoeff;

    sigdump_output_fprint(rdo_ctx, "#compID = %d, %s\n", comp, coeff_type == 0 ? "DCT" : "IQ");
    for (int i = 0; i < tu_size; i++) {
      for (int j = 0; j < tu_size; j++) {
        sigdump_output_fprint(rdo_ctx, "%4d,", pCoeff[i * tu_size + j]);
      }
      sigdump_output_fprint(rdo_ctx, "\n");
    }
  }
}
#endif

void sigdump_reset_amvp_list(int ref_idx)
{
    for (int ui = 0; ui < 5; ui++) {
      strcpy(g_sigpool.amvp_list[ref_idx][ui].dir, "NULL");
      g_sigpool.amvp_list[ref_idx][ui].mvx = 0;
      g_sigpool.amvp_list[ref_idx][ui].mvy = 0;
      g_sigpool.amvp_list[ref_idx][ui].ref_idx = 0;
      g_sigpool.amvp_list[ref_idx][ui].poc_diff = 0;
    }
}

void sigdump_bit_est(int blk_size, int pred_mode, int compNum)
{
  sig_ctx *cmd_ctx = &g_sigpool.bit_est_cmd_ctx[blk_size];
  sig_ctx *gld_ctx = &g_sigpool.bit_est_gld_ctx[blk_size];
  int tu_cnt = g_sigpool.cu_width == 32 ? 4 : 1;
  for (int tu = 0; tu < tu_cnt; tu++)
  {
    sig_bit_est_st *pbit_est;
    g_sigpool.bit_est_cnt[blk_size] ++;
    for (int compID = 0; compID < compNum; compID++) {
      if (g_sigpool.cu_width == 32)
        pbit_est = &g_sigpool.bit_est_golden_32[1][tu][compID];
      else
        pbit_est = &g_sigpool.bit_est_golden[1][pred_mode][compID];
      if (compID == 1 && blk_size == 0)
          g_sigpool.bit_est_chroma_4x4_cnt ++;
      sigdump_output_fprint(cmd_ctx, "#compID = %d, (cu_x, cu_y = %d, %d), (tu_x, tu_y = %d, %d)\n", compID, g_sigpool.cu_idx_x, g_sigpool.cu_idx_y, pbit_est->tu_x, pbit_est->tu_y);
      sigdump_output_fprint(gld_ctx, "#compID = %d, (cu_x, cu_y = %d, %d), (tu_x, tu_y = %d, %d)\n", compID, g_sigpool.cu_idx_x, g_sigpool.cu_idx_y, pbit_est->tu_x, pbit_est->tu_y);
      sigdump_output_fprint(cmd_ctx, "cbf = %d\n", pbit_est->uiCbf);
      sigdump_output_fprint(gld_ctx, "cbf = %d\n", pbit_est->uiCbf);
      sigdump_output_fprint(cmd_ctx, "color = %d, scan_idx = %d, is_intra = %d, last_x = %d, last_y = %d\n", compID, pbit_est->scan_idx, pbit_est->is_intra, pbit_est->last_x, pbit_est->last_y);
      sigdump_output_fprint(cmd_ctx, "m_sig_0 = %x, c_sig_0 = %x, ", pbit_est->m_sig[0], pbit_est->c_sig[0]);
      sigdump_output_fprint(cmd_ctx, "m_sig_1 = %x, c_sig_1 = %x, ", pbit_est->m_sig[1], pbit_est->c_sig[1]);
      sigdump_output_fprint(cmd_ctx, "ratio = %x, lps = %x, sival = %x, %x\n", pbit_est->ratio, pbit_est->lps, pbit_est->sival[0], pbit_est->sival[1]);
      if (pbit_est->uiCbf) {
        sigdump_output_fprint(gld_ctx, "last_x = %d, last_y = %d, csbf_map = %x\n", pbit_est->last_x, pbit_est->last_y, pbit_est->csbf_map);
        for (int i = 0; i < pbit_est->tu_height; i++) {
          sigdump_output_fprint(cmd_ctx, "$ ");
          for (int j = 0; j < pbit_est->tu_width; j++) {
            sigdump_output_fprint(cmd_ctx, "%04x,", (pbit_est->Resi[i][j] & 0xffff));
          }
          sigdump_output_fprint(cmd_ctx, "\n");
        }
        sigdump_output_fprint(gld_ctx, "last_xy_suf_bc = %d\n", pbit_est->last_xy_suf_bc);
        sigdump_output_fprint(gld_ctx, "csbf_bc = %d\n", pbit_est->csbf_bc);
        sigdump_output_fprint(gld_ctx, "gt2_bc = %d\n", pbit_est->gt2_bc);
        sigdump_output_fprint(gld_ctx, "sign_bc = %d\n", pbit_est->sign_bc);
        sigdump_output_fprint(gld_ctx, "rem_bc = %d\n", pbit_est->rem_bc);
        sigdump_output_fprint(gld_ctx, "int_bites = %d\n", pbit_est->int_bites);
        sigdump_output_fprint(gld_ctx, "sig1_bc = %d, m_sig_1 = %x, c_sig_1 = %x, sig1_bc_linear = %x\n", pbit_est->sig_bc[1], pbit_est->m_sig[1], pbit_est->c_sig[1], pbit_est->sig_bc_linear[1]);
        sigdump_output_fprint(gld_ctx, "sig0_bc = %d, m_sig_0 = %x, c_sig_0 = %x, sig0_bc_linear = %x\n", pbit_est->sig_bc[0], pbit_est->m_sig[0], pbit_est->c_sig[0], pbit_est->sig_bc_linear[0]);
        sigdump_output_fprint(gld_ctx, "last_xy_pre_bc = %x, ratio = %x, last_xy_pre_bc_ratio = %x\n", pbit_est->last_xy_pre_bc, pbit_est->ratio, pbit_est->last_xy_pre_bc*pbit_est->ratio);
        sigdump_output_fprint(gld_ctx, "lps = %x, sival = %x, %x\n", pbit_est->lps, pbit_est->sival[0], pbit_est->sival[1]);
        sigdump_output_fprint(gld_ctx, "frac_bites = %x\n", pbit_est->frac_bites);
        sigdump_output_fprint(gld_ctx, "total_bites = %x\n", pbit_est->total_bites);
      } else {
        sigdump_output_fprint(gld_ctx, "lps = %x, sival = %x, %x\n", pbit_est->lps, pbit_est->sival[0], pbit_est->sival[1]);
        sigdump_output_fprint(gld_ctx, "frac_bites = %x\n", pbit_est->sival[0]);
        sigdump_output_fprint(gld_ctx, "total_bites = %x\n", pbit_est->sival[0]);
      }
    }
  }
}

void add_cabac_bit_cnt(void)
{
  if (strncmp(g_sigpool.syntax_name, "last_sig", sizeof("last_sig") - 1) == 0
    || strcmp(g_sigpool.syntax_name, "sig_coeff_flag") == 0
    || strncmp(g_sigpool.syntax_name, "coeff_", sizeof("coeff_") - 1) == 0
    || strcmp(g_sigpool.syntax_name, "coded_sub_block_flag") == 0) {
      g_sigpool.res_bits_num += g_sigpool.numBits;
  } else {
      g_sigpool.hdr_bits_num += g_sigpool.numBits;
  }
}

void sigdump_output_cabac_info(void)
{
  if (!(g_sigdump.cabac && g_sigpool.cabac_is_record)) {
    return;
  }
  if (strcmp(g_sigpool.syntax_name, "NULL_BIN") == 0
    || strcmp(g_sigpool.syntax_name, "NULL_EP") == 0
    || strcmp(g_sigpool.syntax_name, "NULL_sEP") == 0
    || strcmp(g_sigpool.syntax_name, "NULL_Trm") == 0
    || strcmp(g_sigpool.syntax_name, "R") == 0) {
      printf("[ERROR]cabac dump syntax name error %s\n", g_sigpool.syntax_name);
      assert(0);
    }
  sigdump_output_fprint(&g_sigpool.cabac_ctx, "%s %x %x %x %x %x %x %x %x %x %x %x\n",
    g_sigpool.syntax_name, g_sigpool.range, g_sigpool.low & 0x3ff, g_sigpool.bin_type, g_sigpool.bin_value,
    g_sigpool.state[0], g_sigpool.state[1], g_sigpool.mps[0], g_sigpool.mps[1], 0, g_sigpool.put, g_sigpool.numBits);

  add_cabac_bit_cnt();

}

void sigdump_cabac_syntax_info(const char *syntax_name, int grp_id, int stx_id, int ctx_idx, int stx_val, int ctx_inc)
{
  sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx,
    "%s grp_id = %x, stx_id = %x,ctx_idx = %x, stx_val = %x, depth = %d, ctx_inc = %x\n",
    syntax_name, grp_id, stx_id, ctx_idx, stx_val, g_sigpool.cur_depth, ctx_inc);

}

bool is_last_8x8_in_16x16(int cu_idx_x, int cu_idx_y)
{
  int cu_idx_in_16x16 = ((cu_idx_x % 16) >> 3) + ((cu_idx_y % 16) >> 2);
  bool is_last = false;
  bool right_bnd = (cu_idx_x + 8) >= g_sigpool.width;
  bool bot_bnd = (cu_idx_y + 8) >= g_sigpool.height;
  if (right_bnd && bot_bnd)
    is_last = true;
  else if (right_bnd && cu_idx_in_16x16 == 2)
    is_last = true;
  else if (bot_bnd && cu_idx_in_16x16 == 1)
    is_last = true;
  else if (cu_idx_in_16x16 == 3)
    is_last = true;
  return is_last;
}

void sigdump_open_top_ref_ctx(int fb_idx)
{
  string SigdumpFileName;
  SigdumpFileName = g_FileName + "_ref_frm_" + to_string(g_sigpool.enc_count_pattern) + "_" + to_string(fb_idx) + ".bin";
  sigdump_open_ctx_bin(&g_sigpool.top_ref_ctx, SigdumpFileName);
}

void sigdump_close_top_ref_ctx()
{
  sigdump_safe_close(&g_sigpool.top_ref_ctx);
}

void sigdump_pic_rc_init(int targetRate, int frameNum, int frameRate, int intraPeriod, int statTime, \
                        int ipQpDelta, int numOfPixel, int rcMaxIprop, int rcMinIprop, \
                        int rcMaxQp, int rcMinQp, int rcMaxIQp, int rcMinIQp, \
                        int firstFrmstartQp, int rcMdlUpdatType)
{
  sigdump_open_ctx_txt(&g_sigpool.pic_rc_golden, g_FileName + "_picrc_golden.csv");
  sigdump_open_ctx_txt(&g_sigpool.pic_rc_stats, g_FileName + "_picrc_stats.csv");
  sigdump_output_fprint(&g_sigpool.pic_rc_stats, "codec, frameNum, targetBitrate, framerate,\
  intraPeriod, statTime, ipQpDelta, numOfPixel, maxIprop, minIprop,\
  maxQp, minQp, maxIQp, minIQp, firstFrmstartQp, rcMdlUpdatType\n");
  sigdump_output_fprint(&g_sigpool.pic_rc_stats, "2, %d, %d, %d,\
                        %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d \n",\
                        frameNum, targetRate, frameRate, intraPeriod, statTime, ipQpDelta,\
                        numOfPixel, rcMaxIprop, rcMinIprop, rcMaxQp, rcMinQp, rcMaxIQp, rcMinIQp,\
                        firstFrmstartQp, rcMdlUpdatType);
  sigdump_output_fprint(&g_sigpool.pic_rc_stats, "targetBit, picQp, picLambda, encodedBit,\
  encodedQp, encodedLambda, madi, mse, skipRatio, alpha, beta\n");
}


#ifdef CVI_RANDOM_ENCODE

#define UPPER_MASK        0x80000000
#define LOWER_MASK        0x7fffffff
#define TEMPERING_MASK_B  0x9d2c5680
#define TEMPERING_MASK_C  0xefc60000

#define STATE_VECTOR_LENGTH 624
#define STATE_VECTOR_M      397 /* changes to STATE_VECTOR_LENGTH also require changes to this */

typedef struct tagMTRand {
  unsigned long mt[STATE_VECTOR_LENGTH];
  int index;
} MTRand;

MTRand cvi_gRandSeed;

inline static void m_seedRand(MTRand* rand, unsigned long seed) {
  /* set initial seeds to mt[STATE_VECTOR_LENGTH] using the generator
   * from Line 25 of Table 1 in: Donald Knuth, "The Art of Computer
   * Programming," Vol. 2 (2nd Ed.) pp.102.
   */
  rand->mt[0] = seed & 0xffffffff;
  for(rand->index=1; rand->index<STATE_VECTOR_LENGTH; rand->index++) {
    rand->mt[rand->index] = (6069 * rand->mt[rand->index-1]) & 0xffffffff;
  }
}

/**
* Creates a new random number generator from a given seed.
*/
void cvi_srand(unsigned int seed)
{
  m_seedRand(&cvi_gRandSeed, seed);
}

/**
 * Generates a pseudo-randomly generated long.
 */
int cvi_rand()
{
  MTRand* rand = &cvi_gRandSeed;
  unsigned long y;

  static unsigned long mag[2] = {0x0, 0x9908b0df}; /* mag[x] = x * 0x9908b0df for x = 0,1 */
  if(rand->index >= STATE_VECTOR_LENGTH || rand->index < 0) {
    /* generate STATE_VECTOR_LENGTH words at a time */
    int kk;
    if(rand->index >= STATE_VECTOR_LENGTH+1 || rand->index < 0) {
      m_seedRand(rand, 4357);
    }
    for(kk=0; kk<STATE_VECTOR_LENGTH-STATE_VECTOR_M; kk++) {
      y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk+1] & LOWER_MASK);
      rand->mt[kk] = rand->mt[kk+STATE_VECTOR_M] ^ (y >> 1) ^ mag[y & 0x1];
    }
    for(; kk<STATE_VECTOR_LENGTH-1; kk++) {
      y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk+1] & LOWER_MASK);
      rand->mt[kk] = rand->mt[kk+(STATE_VECTOR_M-STATE_VECTOR_LENGTH)] ^ (y >> 1) ^ mag[y & 0x1];
    }
    y = (rand->mt[STATE_VECTOR_LENGTH-1] & UPPER_MASK) | (rand->mt[0] & LOWER_MASK);
    rand->mt[STATE_VECTOR_LENGTH-1] = rand->mt[STATE_VECTOR_M-1] ^ (y >> 1) ^ mag[y & 0x1];
    rand->index = 0;
  }
  y = rand->mt[rand->index++];
  y ^= (y >> 11);
  y ^= (y << 7) & TEMPERING_MASK_B;
  y ^= (y << 15) & TEMPERING_MASK_C;
  y ^= (y >> 18);
  return (y & 0x7fffffff);
}

int cvi_random_range(int min, int max)
{
  int x = cvi_rand() % (max - min + 1) + min;
  return x;
}

int cvi_random_range_step(int min, int max, int step)
{
  int x = cvi_rand() % (max - min + 1) + min;
  if (x - step <= min)
    x = min;
  if (x + step >= max)
    x = max;

  return x;
}

int cvi_random_equal()
{
  int x = (cvi_random_range(0, 127) >= 64) ? 1 : 0;
  return x;
}

bool cvi_random_probability(int prob)
{
  assert(prob > 0 && prob < 100);
  int rand = cvi_random_range(1, 100);
  return (rand <= prob);
}
#endif //~CVI_RANDOM_ENCODE
