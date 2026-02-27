
#ifndef __CVI_VBC__
#define __CVI_VBC__

typedef struct _sub_tile_meta_st {
    unsigned int address;
    unsigned char *cu_sz_arry;
} sub_tile_meta_st;

typedef struct _tile_meta_st {
    sub_tile_meta_st *sub_tile_arry;
} tile_meta_st;

typedef struct _meta_st {
    int pic_width;
    int pic_height;
    int width;
    int height;
    int tile_w;
    int tile_h;
    int tile_sub_w;
    int tile_sub_h;
    int tile_cnt_x;
    int tile_cnt_y;
    int tile_sub_cnt_x;
    int tile_sub_cnt_y;
    int cu_w;
    int cu_h;
    int sub_tile_size;
    int tile_size;
    int meta_size;
    tile_meta_st *tile_arry;
    int frm_cnt;
    unsigned int total_compress_size;
    unsigned int offset;
    int shift_pix;
    int VBCMetaPitch;
    int MetaOutPack_size;
    int ycinterleave;
} meta_st;

typedef struct _encCU_st {
    int chType;
    int cu_w;
    int cu_h;
    int leftPixel;
    bool isMirrorCU;
    unsigned char *recCU;
    unsigned char *predCU;
    unsigned int mode_len[2][10];
    uint64_t *codeCU;
    unsigned int *codeCULength;
    unsigned char *fixCU;
} encCU_st;

extern int g_VBCVersion;
extern meta_st g_vbc_meta[2];
extern meta_st g_vbd_meta[2]; //for input compressed src.
int get_vbc_meta_pitch_align_size();
void vbc_meta_init(int vbc_version, meta_st *p_in_meta, int pic_width, int pic_height, int vbc_shift);
void vbc_meta_deinit(meta_st *p_in_meta);
void get_vbc_1d_meta_buffer(int chType, int x, int y, unsigned int *meta_address, unsigned int *meta_length);
void get_vbc_meta_info(int chType, int x, int y, unsigned int *address, unsigned int *data_length);
void get_vbc_meta_info_fake(int chType, int x, int y, unsigned int *address, unsigned int *data_length);
void get_vbc_meta_info_fake_reorder(int chType, int x, int y, unsigned int *address, unsigned int *data_length);
//void init_cu(int cu_idx_x, int cu_idx_y, TComPicYuv* src, ComponentID srcPlane, encCU_st *encCU, meta_st *pvbc_meta);
//void del_cu(encCU_st *encCU);
void vbc_contexts_init();
void vbc_encode_frame(meta_st *p_in_meta, TComPicYuv* src, int frm_cnt,int cfg_lossy,int cfg_lossy_cr,int cfg_lossy_tolerence,int cfg_lossy_delay);
void vbc_encode_nv12_frame(meta_st *p_in_meta, TComPicYuv *src, int frm_cnt, int cfg_lossy, int cfg_lossy_cr, int cfg_lossy_tolerence, int cfg_lossy_delay);
#endif //~__CVI_VBC__
