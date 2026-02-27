
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "CommonDef.h"
//#include "cvi_sigdump.h"
#include "TComPicYuv.h"
#include "cvi_vbc.h"
#include "cvi_frm_buf_mgr.h"
#include "cvi_sigdump.h"

using namespace std;

frm_buf_mgr_st g_frm_buf_mgr;
dram_buf_mgr_st g_dram_buf_mgr;
bool set64bit_DRAM_BASE;
static const unsigned int BS_SIZE = 0xC00000;
uint64_t DRAM_BASE = 0;
uint64_t SRAM_BASE = 0;
uint64_t cfg_DRAM_BASE = 0;
uint64_t cfg_SRAM_BASE = 0;
unsigned int g_nmu_axi_map = 0;
unsigned int g_iap_axi_map = 0;
unsigned int g_ppu_axi_map = 0;

unsigned int get_hw_pitch(string name)
{
    if (!name.compare("mv"))
        return g_dram_buf_mgr.mv.pitch;
    else if (!name.compare("nm_b"))
        return g_dram_buf_mgr.nm_b.pitch;
    else if (!name.compare("nm_a"))
        return g_dram_buf_mgr.nm_a.pitch;
    else if (!name.compare("qp_map"))
        return g_dram_buf_mgr.qp_map.pitch;
    else if (!name.compare("luma_fb"))
        return g_dram_buf_mgr.frm[0].pitch;
    else if (!name.compare("vbc_meta"))
        return g_dram_buf_mgr.vbc_meta.pitch;
    else if (!name.compare("src_y"))
        return g_dram_buf_mgr.src[0].pitch;
    else if (!name.compare("src_cb"))
        return g_dram_buf_mgr.src[1].pitch;
    else if (!name.compare("src_cr"))
        return g_dram_buf_mgr.src[2].pitch;
    else if (!name.compare("ai_dqp"))
        return g_dram_buf_mgr.ai_dqp.pitch;
    else if (!name.compare("isp_dqp"))
        return g_dram_buf_mgr.isp_dqp.pitch;
    else
        return 0;
}

uint64_t get_hw_base(string name, int index)
{
    if (!name.compare("nm_b"))
        return g_dram_buf_mgr.nm_b.base;
    else if (!name.compare("nm_a"))
        return g_dram_buf_mgr.nm_a.base;
    else if (!name.compare("iapu"))
        return g_dram_buf_mgr.iapu.base;
    else if (!name.compare("ilf_b"))
        return g_dram_buf_mgr.ilf_b.base;
    else if (!name.compare("ilf_a"))
        return g_dram_buf_mgr.ilf_a.base;
    else if (!name.compare("qp_map"))
        return g_dram_buf_mgr.qp_map.base;
    else if (!name.compare("bs"))
        return g_dram_buf_mgr.bs.base;
    else if (index >= 0 && index < g_dram_buf_mgr.max_frm_num) {
        if (!name.compare("mv"))
            return g_dram_buf_mgr.mv.base[index];
        else if (!name.compare("luma_fb"))
            return g_dram_buf_mgr.frm[0].base[index];
        else if (!name.compare("chroma_fb"))
            return g_dram_buf_mgr.frm[1].base[index];
        else if (!name.compare("vbc_meta"))
            return g_dram_buf_mgr.vbc_meta.base[index];
        else
            return 0;
    }
    else if (!name.compare("src_y"))
        return g_dram_buf_mgr.src[0].base;
    else if (!name.compare("src_cb"))
        return g_dram_buf_mgr.src[1].base;
    else if (!name.compare("src_cr"))
        return g_dram_buf_mgr.src[2].base;
    else if (!name.compare("ai_dqp"))
        return g_dram_buf_mgr.ai_dqp.base;
    else if (!name.compare("isp_dqp"))
        return g_dram_buf_mgr.isp_dqp.base;
    else
        return 0;
}

uint32_t get_hw_base_32(string name, string part, int index)
{
    REG64_C base64;
    base64.val = get_hw_base(name, index);
    if (!part.compare("LSB") || !part.compare("lsb"))
    {
        return base64.reg_lsb;
    }
    else if (!part.compare("MSB") || !part.compare("msb"))
    {
        return base64.reg_msb;
    }
    else
    {
        return 0;
    }
}

unsigned int get_hw_size(string name, int index)
{
    if (!name.compare("nm_b"))
        return g_dram_buf_mgr.nm_b.size;
    else if (!name.compare("nm_a"))
        return g_dram_buf_mgr.nm_a.size;
    else if (!name.compare("qp_map"))
        return g_dram_buf_mgr.qp_map.size;
    else if (!name.compare("bs"))
        return g_dram_buf_mgr.bs.size;
    else if (index >= 0 && index < g_dram_buf_mgr.max_frm_num) {
        if (!name.compare("mv"))
            return g_dram_buf_mgr.mv.size;
        else if (!name.compare("luma_fb"))
            return g_dram_buf_mgr.frm[0].size;
        else if (!name.compare("chroma_fb"))
            return g_dram_buf_mgr.frm[1].size;
        else
            return 0;
    }
    else if (!name.compare("src_y"))
        return g_dram_buf_mgr.src[0].size;
    else if (!name.compare("src_cb"))
        return g_dram_buf_mgr.src[1].size;
    else if (!name.compare("src_cr"))
        return g_dram_buf_mgr.src[2].size;
    else if (!name.compare("ai_dqp"))
        return g_dram_buf_mgr.ai_dqp.size;
    else if (!name.compare("isp_dqp"))
        return g_dram_buf_mgr.isp_dqp.size;
    else
        return 0;
}

bool cvi_get_dram_buf(string name, uint64_t *addr, unsigned int size, unsigned int mem_align = 1024)
{
    if (size == 0)
        return true;
    uint64_t align_base = cvi_mem_align64(g_dram_buf_mgr.dram_cur_base, (uint64_t)mem_align);
    if ((align_base + size) >= g_dram_buf_mgr.dram_end_base) {
        printf("[ERROR][%s]get DRAM error, curr base: 0x%lx, max_end_base: 0x%lx, size: 0x%x\n",
            name.c_str(), g_dram_buf_mgr.dram_cur_base, g_dram_buf_mgr.dram_end_base, size);
        return false;
    }
    printf("[DRAM][%12s]base:0x%lx, size: 0x%x\n", name.c_str(), align_base, size);
    *addr = align_base;
    g_dram_buf_mgr.dram_cur_base = align_base + size;
    return true;
}

bool cvi_get_sram_buf(string name, uint64_t *addr, unsigned int size, unsigned int mem_align = 1024)
{
    if (size == 0)
        return true;
    uint64_t align_base = cvi_mem_align64(g_dram_buf_mgr.sram_cur_base, (uint64_t)mem_align);
    if ((align_base + size) >= g_dram_buf_mgr.sram_end_base) {
        printf("[ERROR][%s]get SRAM error, curr base: 0x%lx, max_end_base: 0x%lx, size: 0x%x\n",
            name.c_str(), g_dram_buf_mgr.sram_cur_base, g_dram_buf_mgr.sram_end_base, size);
        return false;
    }
    printf("[SRAM][%12s]base:0x%lx, size: 0x%x\n", name.c_str(), align_base, size);
    *addr = align_base;
    g_dram_buf_mgr.sram_cur_base = align_base + size;
    return true;
}

bool cvi_get_memory_buf(int type, string name, uint64_t *addr, unsigned int size, unsigned int mem_align = 1024)
{
    if (type == MALLOC_DRAM)
        return cvi_get_dram_buf(name, addr, size, mem_align);
    else
        return cvi_get_sram_buf(name, addr, size, mem_align);
}

bool cvi_alloc_dram_buf(int max_frm_num, int width, int height)
{
    assert(max_frm_num <= MAX_FRAME_BUFFER);
    memset(&g_dram_buf_mgr, 0x0, sizeof(dram_buf_mgr_st));
    g_dram_buf_mgr.max_frm_num = max_frm_num;
    g_dram_buf_mgr.dram_base = g_dram_buf_mgr.dram_cur_base = DRAM_BASE;
    g_dram_buf_mgr.dram_end_base = g_dram_buf_mgr.dram_base + (MAX_UINT64 - DRAM_BASE);

    g_dram_buf_mgr.sram_base = g_dram_buf_mgr.sram_cur_base = SRAM_BASE;
    g_dram_buf_mgr.sram_end_base = g_dram_buf_mgr.sram_base + (MAX_UINT64 - SRAM_BASE);

    //IAP
    g_dram_buf_mgr.iapu.size =  ((width + 63) / 64) * 128;
    g_dram_buf_mgr.iapu.mem_align = 1024;
    if(!cvi_get_memory_buf(g_iap_axi_map, "IAP", &g_dram_buf_mgr.iapu.base, g_dram_buf_mgr.iapu.size)) return false;

    //PPU A / B
    g_dram_buf_mgr.ilf_b.size =  ((width + 63) / 64) * 512;
    g_dram_buf_mgr.ilf_b.mem_align = 1024;
    g_dram_buf_mgr.ilf_a.size = 0;//No need if DONOT support TILE
    g_dram_buf_mgr.ilf_a.mem_align = 1024;
    if(!cvi_get_memory_buf(g_ppu_axi_map, "ILF B", &g_dram_buf_mgr.ilf_b.base, g_dram_buf_mgr.ilf_b.size)) return false;
    if(!cvi_get_memory_buf(g_ppu_axi_map, "ILF A", &g_dram_buf_mgr.ilf_a.base, g_dram_buf_mgr.ilf_a.size)) return false;

    //neighbor A / B
    g_dram_buf_mgr.nm_b.size = (width + 255) / 256 * 4 * 48;
    g_dram_buf_mgr.nm_b.mem_align = 1024;
    g_dram_buf_mgr.nm_a.size = 0;//No need if DONOT support TILE
    g_dram_buf_mgr.nm_a.mem_align = 1024;
    if(!cvi_get_memory_buf(g_nmu_axi_map, "nei B", &g_dram_buf_mgr.nm_b.base, g_dram_buf_mgr.nm_b.size)) return false;
    if(!cvi_get_memory_buf(g_nmu_axi_map, "nei A", &g_dram_buf_mgr.nm_a.base, g_dram_buf_mgr.nm_a.size)) return false;

    //Col MV, col buf size is 96 byte per CTB, and align to TWO CTBs.
    g_dram_buf_mgr.mv.pitch = ((width + 127) / 128) * 96 * 2;
    g_dram_buf_mgr.mv.size = g_dram_buf_mgr.mv.pitch * ((height + 63) / 64);
    g_dram_buf_mgr.mv.mem_align = 1024;
    for (int frm_cnt = 0; frm_cnt < max_frm_num; frm_cnt++) {
        string mv_name = "MV[" + to_string(frm_cnt) + "]";
        if(!cvi_get_dram_buf(mv_name, &g_dram_buf_mgr.mv.base[frm_cnt], g_dram_buf_mgr.mv.size))
            return false;
    }

    //BS
    g_dram_buf_mgr.bs.size = BS_SIZE;
    g_dram_buf_mgr.bs.mem_align = 1024;
    if(!cvi_get_dram_buf("BS", &g_dram_buf_mgr.bs.base, g_dram_buf_mgr.bs.size)) return false;

    //frame buffer
    g_dram_buf_mgr.frm[0].pitch = cvi_mem_align(width, 64);
    g_dram_buf_mgr.frm[0].size = g_dram_buf_mgr.frm[0].pitch * cvi_mem_align(height, 64);
    g_dram_buf_mgr.frm[0].mem_align = 1024;
    //Chroma
    g_dram_buf_mgr.frm[1].pitch = cvi_mem_align(width, 64);
    g_dram_buf_mgr.frm[1].size = g_dram_buf_mgr.frm[1].pitch * cvi_mem_align(height / 2, 64);
    g_dram_buf_mgr.frm[1].mem_align = 1024;

    //VBC META
    g_dram_buf_mgr.vbc_meta.pitch = g_vbc_meta[0].VBCMetaPitch;
    g_dram_buf_mgr.vbc_meta.size = g_dram_buf_mgr.vbc_meta.pitch * g_vbc_meta[0].tile_cnt_y;
    g_dram_buf_mgr.vbc_meta.mem_align = 1024;

    for (int frm_cnt = 0; frm_cnt < max_frm_num; frm_cnt++) {
        //Luma
        string frm_name = "Luma[" + to_string(frm_cnt) + "]";
        if(!cvi_get_dram_buf(frm_name, &g_dram_buf_mgr.frm[0].base[frm_cnt], g_dram_buf_mgr.frm[0].size))
            return false;
        //Chroma
        frm_name = "Chroma[" + to_string(frm_cnt) + "]";
        if(!cvi_get_dram_buf(frm_name, &g_dram_buf_mgr.frm[1].base[frm_cnt], g_dram_buf_mgr.frm[1].size))
            return false;
        //VBC META
        frm_name = "Vbc_meta[" + to_string(frm_cnt) + "]";
        if(!cvi_get_dram_buf(frm_name, &g_dram_buf_mgr.vbc_meta.base[frm_cnt], g_dram_buf_mgr.vbc_meta.size))
            return false;

    }

    if (isEnableRGBConvert() && !g_algo_cfg.enableCviRGBDebugMode)
    {
        // source buffer, RGBX
        int pitch = cvi_mem_align(width * 4, 32);
        g_dram_buf_mgr.src[0].pitch = pitch;
        g_dram_buf_mgr.src[0].size  = cvi_mem_align(pitch * height, 16384);
        g_dram_buf_mgr.src[0].mem_align = 16384;
        if(!cvi_get_dram_buf("SRC Y", &g_dram_buf_mgr.src[0].base, g_dram_buf_mgr.src[0].size)) return false;

        g_dram_buf_mgr.src[1].pitch = 0;
        g_dram_buf_mgr.src[1].size  = 0;
        g_dram_buf_mgr.src[1].mem_align = 16384;
    }
    else
    {
        if (g_algo_cfg.EnableRotateAngle == 90 || g_algo_cfg.EnableRotateAngle == 270)
        {
            // source buffer, nv12
            int pitch = cvi_mem_align(height, 32);
            g_dram_buf_mgr.src[0].pitch = pitch;
            g_dram_buf_mgr.src[0].size  = cvi_mem_align(pitch * width, 16384);
            g_dram_buf_mgr.src[0].mem_align = 16384;
            if(!cvi_get_dram_buf("SRC Y", &g_dram_buf_mgr.src[0].base, g_dram_buf_mgr.src[0].size)) return false;

            g_dram_buf_mgr.src[1].pitch = pitch;
            g_dram_buf_mgr.src[1].size  = cvi_mem_align(pitch * (width >> 1), 16384);
            g_dram_buf_mgr.src[1].mem_align = 16384;
            if(!cvi_get_dram_buf("SRC Cb", &g_dram_buf_mgr.src[1].base, g_dram_buf_mgr.src[1].size)) return false;
        }
        else
        {
            // source buffer, nv12
            int pitch = cvi_mem_align(width, 32);
            g_dram_buf_mgr.src[0].pitch = pitch;
            g_dram_buf_mgr.src[0].size  = cvi_mem_align(pitch * height, 16384);
            g_dram_buf_mgr.src[0].mem_align = 16384;
            if(!cvi_get_dram_buf("SRC Y", &g_dram_buf_mgr.src[0].base, g_dram_buf_mgr.src[0].size)) return false;

            g_dram_buf_mgr.src[1].pitch = pitch;
            g_dram_buf_mgr.src[1].size  = cvi_mem_align(pitch * (height >> 1), 16384);
            g_dram_buf_mgr.src[1].mem_align = 16384;
            if(!cvi_get_dram_buf("SRC Cb", &g_dram_buf_mgr.src[1].base, g_dram_buf_mgr.src[1].size)) return false;
        }
    }

    //QP MAP
    g_dram_buf_mgr.qp_map.pitch = ((width + 511) / 512) * 16 * 8;
    g_dram_buf_mgr.qp_map.size =  g_dram_buf_mgr.qp_map.pitch * ((height + 63) / 64);
    g_dram_buf_mgr.qp_map.mem_align = 16384;
    if(!cvi_get_dram_buf("QP MAP", &g_dram_buf_mgr.qp_map.base, g_dram_buf_mgr.qp_map.size)) return false;

    // AI dram buffer
    int height_in_ctu64 = cvi_mem_align_mult(height, 64);
    g_dram_buf_mgr.ai_dqp.pitch = cvi_mem_align_mult(width, 256) * 128;
    g_dram_buf_mgr.ai_dqp.size = g_dram_buf_mgr.ai_dqp.pitch * height_in_ctu64;
    g_dram_buf_mgr.ai_dqp.mem_align = 16384;
    if(!cvi_get_dram_buf("AI DQP", &g_dram_buf_mgr.ai_dqp.base, g_dram_buf_mgr.ai_dqp.size)) return false;

    // ISP dram buffer
    g_dram_buf_mgr.isp_dqp.pitch = cvi_mem_align_mult(width, 256) * 16;
    g_dram_buf_mgr.isp_dqp.size = g_dram_buf_mgr.isp_dqp.pitch * (height >> 2);
    g_dram_buf_mgr.isp_dqp.mem_align = 16384;
    if(!cvi_get_dram_buf("ISP DQP", &g_dram_buf_mgr.isp_dqp.base, g_dram_buf_mgr.isp_dqp.size)) return false;

    return true;
}

void frm_buf_mgr_init(int max_frm_num, int width, int height, bool fpga)
{
    if (fpga) {
        DRAM_BASE = 0x81600000;
        
        if (g_sigdump.testBigClipSmall==1)
        {
            DRAM_BASE = 0x81600000;     // Big picture
        }
        else if(g_sigdump.testBigClipSmall==2)
        {
            DRAM_BASE = 0x85600000;     // Small picture 
        }
    }
    else
    {
        DRAM_BASE = cfg_DRAM_BASE;
        if (set64bit_DRAM_BASE)
            DRAM_BASE |= 0x700000000;
    }
    SRAM_BASE = cfg_SRAM_BASE;

    memset(&g_frm_buf_mgr, 0x0, sizeof(frm_buf_mgr_st));

    g_frm_buf_mgr.max_frm_num = max_frm_num;

    g_frm_buf_mgr.frm_buf = new frm_buf_st[max_frm_num];
    for (int i = 0; i < max_frm_num; i++)
    {
        g_frm_buf_mgr.frm_buf[i].poc = -1;
        g_frm_buf_mgr.frm_buf[i].status = AVAIL;
    }
    cvi_alloc_dram_buf(max_frm_num, width, height);
}

void frm_buf_mgr_deinit()
{
    delete [] g_frm_buf_mgr.frm_buf;
}

int frm_buf_mgr_release(int poc)
{
    for (int i = 0; i < g_frm_buf_mgr.max_frm_num; i++)
    {
        if (g_frm_buf_mgr.frm_buf[i].poc == poc)
        {
            g_frm_buf_mgr.frm_buf[i].status = AVAIL;
            g_frm_buf_mgr.frm_buf[i].poc = -1;
            g_frm_buf_mgr.frm_buf[i].frm_cnt = -1;
            return i;
        }
    }
    printf("[ERR]not find frmbuf(poc %d) to release\n", poc);
    assert(0);
    return -1;
}

int frm_buf_mgr_get_max_frm_num(void)
{
    return g_frm_buf_mgr.max_frm_num;
}

int frm_buf_mgr_get(int frm_cnt, int poc)
{
    //check no conflict frm index
    for (int i = 0; i < g_frm_buf_mgr.max_frm_num; i++)
    {
        if (g_frm_buf_mgr.frm_buf[i].status == ENCODED)
        {
            if (g_frm_buf_mgr.frm_buf[i].frm_cnt == frm_cnt)
            {
                printf("[ERR]This frm idx:%d already in frm bufer\n", frm_cnt);
                assert(0);
                return -1;
            }
        }
    }
    for (int i = 0; i < g_frm_buf_mgr.max_frm_num; i++)
    {
        if (g_frm_buf_mgr.frm_buf[i].status == AVAIL)
        {
            g_frm_buf_mgr.frm_buf[i].status = ENCODED;
            g_frm_buf_mgr.frm_buf[i].poc = poc;
            g_frm_buf_mgr.frm_buf[i].frm_cnt = frm_cnt;
            return i;
        }
    }
    printf("[ERR]not find frmbuf(poc %d) to encode\n", poc);
    assert(0);
    return -1;
}

int frm_buf_mgr_find_index_by_poc( int poc)
{
    for (int i = 0; i < g_frm_buf_mgr.max_frm_num; i++)
    {
        if (g_frm_buf_mgr.frm_buf[i].status == ENCODED && g_frm_buf_mgr.frm_buf[i].poc == poc)
        {
            return i;
        }
    }
    printf("[ERR]not find frmbuf(poc %d)\n", poc);
    assert(0);
    return -1;
}

int frm_buf_mgr_find_frm_cnt_by_poc( int poc)
{
    for (int i = 0; i < g_frm_buf_mgr.max_frm_num; i++)
    {
        if (g_frm_buf_mgr.frm_buf[i].status == ENCODED && g_frm_buf_mgr.frm_buf[i].poc == poc)
        {
            return g_frm_buf_mgr.frm_buf[i].frm_cnt;
        }
    }
    printf("[ERR]not find frmbuf(poc %d)\n", poc);
    assert(0);
    return -1;
}