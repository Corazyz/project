
#ifndef __CVI_FRM_BUF_MGR__
#define __CVI_FRM_BUF_MGR__

#include <iostream>

using namespace std;

#define MAX_FRAME_BUFFER 8

enum frm_buf_status
{
  AVAIL       = 0,
  ENCODED     = 1,
};

#define MALLOC_DRAM 1
#define MALLOC_SRAM 2

typedef union {
  struct {
     uint32_t reg_lsb:32;
     uint32_t reg_msb:32;
  };
  uint64_t val;
} REG64_C;

typedef struct _frm_buf {
  frm_buf_status status;
  int poc;
  int frm_cnt;
} frm_buf_st;

typedef struct _frm_buf_mgr {
    unsigned int max_frm_num;
    frm_buf_st *frm_buf;
} frm_buf_mgr_st;

typedef struct _dram_buf {
  uint64_t base;
  unsigned int pitch;
  unsigned int size;
  unsigned int mem_align;
} dram_st;

typedef struct _dram_arr_st {
  uint64_t base[MAX_FRAME_BUFFER];
  unsigned int pitch;
  unsigned int size;
  unsigned int mem_align;
} dram_arr_st;

typedef struct _dram_buf_mgr {
  uint64_t dram_base;
  uint64_t dram_end_base;
  uint64_t dram_cur_base;
  uint64_t sram_base;
  uint64_t sram_end_base;
  uint64_t sram_cur_base;
  unsigned int max_frm_num;
  dram_st bs;
  dram_st nm_b;
  dram_st nm_a;
  dram_st iapu;
  dram_st ilf_b;
  dram_st ilf_a;
  dram_st qp_map;
  dram_st cu_dep;
  dram_arr_st mv;
  dram_arr_st frm[2]; //luma, chroma
  dram_arr_st vbc_meta; //luma
  dram_st src[3];     //y, cb, cr
  dram_st ai_dqp;
  dram_st isp_dqp;
} dram_buf_mgr_st;

unsigned int get_hw_pitch(string name);
uint64_t get_hw_base(string name, int index = -1);
uint32_t get_hw_base_32(string name, string part, int index = -1);
unsigned int get_hw_size(string name, int index = -1);

void frm_buf_mgr_init(int max_frm_num, int width, int height, bool fpga);
void frm_buf_mgr_deinit();
int frm_buf_mgr_get(int frm_cnt, int poc);
int frm_buf_mgr_get_max_frm_num(void);
int frm_buf_mgr_release(int poc);
int frm_buf_mgr_find_index_by_poc( int poc);
int frm_buf_mgr_find_frm_cnt_by_poc( int poc);
extern uint64_t cfg_DRAM_BASE;
extern uint64_t cfg_SRAM_BASE;
extern unsigned int g_nmu_axi_map;
extern unsigned int g_iap_axi_map;
extern unsigned int g_ppu_axi_map;
extern bool set64bit_DRAM_BASE;
#endif /* __CVI_FRM_BUF_MGR__ */
