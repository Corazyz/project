
#ifndef __CVI_CACHE__
#define __CVI_CACHE__

#include "TLibCommon/TComDataCU.h"

using namespace std;

typedef struct _cache_entry_st {
    bool valid;
    unsigned int x;
    unsigned int y;
    int poc;
} cache_entry_st;

void cache_init(int w, int h);
void cache_deinit();

void cache_frm_enable();
void cache_frm_disable();
void miu_access(int x, int y, int w, int h, int poc, bool isMC = false, bool isChroma = false);
void miu_access_fme(int x, int y, int w, int h, int poc);
void miu_access_mc(TComDataCU*& rpcTempCU, bool isChroma = false);
// 
typedef struct cache_statistics_st {
    FILE *cache_statistics_fp; //statistics dump
    unsigned int min_byte_cnt;
    unsigned int max_byte_cnt;
    unsigned int avg_byte_cnt;
    unsigned int min_byte_ratio; //for 1D VBC
    unsigned int max_byte_ratio; //for 1D VBC
    unsigned int avg_byte_ratio; //for 1D VBC
    unsigned int frm_cnt;
} cache_statistics_st;

typedef struct cache_set_st {
    cache_entry_st *entry; // 8 way
    int *PLRU_BIT;
    int way_cnt[16];
} cache_set_st;

typedef struct _cache_model_st {
    char cache_type[128];
    cache_set_st **cvi_cache_set;
    int num_way;
    int cache_width; //byte
    int cache_height; //byte
    int cache_line_width; //byte
    int cache_line_height;
    int idx_x_mask;
    int idx_y_mask;
    int hit_cnt;
    int miss_cnt;
    int l2_hit_cnt;
    int l2_miss_cnt;
    int image_w;
    int image_h;
    FILE *cache_log_fp; //log dump
    FILE *cache_fp; //pattern dump
    cache_statistics_st cache_statistics;
    int dump_enable;
} cache_model_st;

typedef struct _cache_1d_model_st {
    char cache_type[128];
    cache_set_st *cvi_cache_set;
    int num_way;
    int cache_size; //byte
    int cache_line; //byte
    int idx_mask;
    int hit_cnt;
    int miss_cnt;
    int l2_hit_cnt;
    int l2_miss_cnt;
    int image_w;
    int image_h;
    FILE *cache_log_fp; //log dump
    FILE *cache_fp; //pattern dump
    cache_statistics_st cache_statistics;
    int dump_enable;
} cache_1d_model_st;

extern cache_model_st me_cache;

#endif /* __CVI_CACHE__ */
