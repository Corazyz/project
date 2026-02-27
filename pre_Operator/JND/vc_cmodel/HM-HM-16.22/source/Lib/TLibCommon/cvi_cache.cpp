
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "cvi_algo_cfg.h"
#include "cvi_cache.h"
#include "cvi_sigdump.h"
#include "TComPicYuv.h"
#include "cvi_vbc.h"

using namespace std;

bool log_enable = false;
bool vbc_meta_1d_combine = true;

cache_1d_model_st vbc_1d_cache;
cache_1d_model_st vbc_1d_chroma_cache;
cache_1d_model_st vbc_meta_1d_cache;
cache_1d_model_st vbc_meta_1d_chroma_cache;
cache_model_st vbc_meta_cache;
cache_model_st vbc_meta_chroma_cache;
cache_model_st me_cache;
cache_model_st me_l2_cache;
cache_model_st mc_cache;
cache_model_st mc_chroma_cache;

int replace_LRU(cache_set_st *cache_set, int num_way)
{
    for (int way = 0; way < num_way; way++)
    {
        if (cache_set->entry[way].valid != true)
        {
            return way;
        }
    }
    if (num_way == 2)
    {
        return (cache_set->PLRU_BIT[0] == 0) ? 0 : 1;
    }

    if (num_way == 4)
    {
        if (cache_set->PLRU_BIT[0] == 0)
        {
            return cache_set->PLRU_BIT[1] == 0 ? 0 : 1;
        }
        else
        {
            return cache_set->PLRU_BIT[2] == 0 ? 2 : 3;
        }
    }

    if (num_way == 8)
    {
        if (cache_set->PLRU_BIT[0] == 0)
        {
            if (cache_set->PLRU_BIT[1] == 0)
            {
                return cache_set->PLRU_BIT[2] == 0 ? 0 : 1;
            }
            else
            {
                return cache_set->PLRU_BIT[3] == 0 ? 2 : 3;
            }
        }
        else
        {
            if (cache_set->PLRU_BIT[4] == 0)
            {
                return cache_set->PLRU_BIT[5] == 0 ? 4 : 5;
            }
            else
            {
                return cache_set->PLRU_BIT[6] == 0 ? 6 : 7;
            }
        }
    }
    else if (num_way == 16)
    {
        if (cache_set->PLRU_BIT[0] == 0)
        {
            if (cache_set->PLRU_BIT[1] == 0)
            {
                if (cache_set->PLRU_BIT[2] == 0)
                {
                    return cache_set->PLRU_BIT[3] == 0 ? 0 : 1;
                }
                else
                {
                    return cache_set->PLRU_BIT[4] == 0 ? 2 : 3;
                }
            }
            else
            {
                if (cache_set->PLRU_BIT[5] == 0)
                {
                    return cache_set->PLRU_BIT[6] == 0 ? 4 : 5;
                }
                else
                {
                    return cache_set->PLRU_BIT[7] == 0 ? 6 : 7;
                }
            }
        }
        else
        {
            if (cache_set->PLRU_BIT[8] == 0)
            {
                if (cache_set->PLRU_BIT[9] == 0)
                {
                    return cache_set->PLRU_BIT[10] == 0 ? 8 : 9;
                }
                else
                {
                    return cache_set->PLRU_BIT[11] == 0 ? 10 : 11;
                }
            }
            else
            {
                if (cache_set->PLRU_BIT[12] == 0)
                {
                    return cache_set->PLRU_BIT[13] == 0 ? 12 : 13;
                }
                else
                {
                    return cache_set->PLRU_BIT[14] == 0 ? 14 : 15;
                }
            }
        }
    }
    assert(0);
    return -1;
}

void update_LRU(int way, cache_set_st *cache_set, int num_way)
{
    if (num_way == 1)
    {
        cache_set->PLRU_BIT[0] = 0;
        cache_set->PLRU_BIT[1] = 0;
        cache_set->PLRU_BIT[2] = 0;
        return;
    }
    else if (num_way == 2)
    {
        cache_set->PLRU_BIT[0] = way == 0 ? 1 : 0;
        return;
    }
    else if (num_way == 4)
    {
        if (way == 0 || way == 1)
        {
            cache_set->PLRU_BIT[0] = 1;
            cache_set->PLRU_BIT[1] = way == 0 ? 1 : 0;
        }
        else if (way == 2 || way == 3)
        {
            cache_set->PLRU_BIT[0] = 0;
            cache_set->PLRU_BIT[2] = way == 2 ? 1 : 0;
        }
        return;
    }
    else if (num_way == 8)
    {
        //[0]
        cache_set->PLRU_BIT[0] = way < (num_way / 2) ? 1 : 0;
        way &= 0x3;
        if (cache_set->PLRU_BIT[0])
        {
            //[1]
            cache_set->PLRU_BIT[1] = way < (num_way / 4) ? 1 : 0;
            way &= 0x1;
            //[2] [3]
            if (cache_set->PLRU_BIT[1])
                cache_set->PLRU_BIT[2] = way < (num_way / 8) ? 1 : 0;
            else
                cache_set->PLRU_BIT[3] = way < (num_way / 8) ? 1 : 0;
        }
        else
        {
            //[4]
            cache_set->PLRU_BIT[4] = way < (num_way / 4) ? 1 : 0;
            way &= 0x1;
            //[5] [6]
            if (cache_set->PLRU_BIT[4])
                cache_set->PLRU_BIT[5] = way < (num_way / 8) ? 1 : 0;
            else
                cache_set->PLRU_BIT[6] = way < (num_way / 8) ? 1 : 0;
        }
    }
    else if (num_way == 16)
    {
        //[0]
        cache_set->PLRU_BIT[0] = way < (num_way / 2) ? 1 : 0;
        way &= 0x7;
        if (cache_set->PLRU_BIT[0])
        {
            //[1]
            cache_set->PLRU_BIT[1] = way < (num_way / 4) ? 1 : 0;
            way &= 0x3;
            if (cache_set->PLRU_BIT[1])
            {
                //[2]
                cache_set->PLRU_BIT[2] = way < (num_way / 8) ? 1 : 0;
                way &= 0x1;
                //[3] [4]
                if (cache_set->PLRU_BIT[2])
                {
                    cache_set->PLRU_BIT[3] = way < (num_way / 16) ? 1 : 0;
                }
                else
                {
                    cache_set->PLRU_BIT[4] = way < (num_way / 16) ? 1 : 0;
                }
            }
            else
            {
                //[5]
                cache_set->PLRU_BIT[5] = way < (num_way / 8) ? 1 : 0;
                way &= 0x1;
                //[6] [7]
                if (cache_set->PLRU_BIT[5])
                {
                    cache_set->PLRU_BIT[6] = way < (num_way / 16) ? 1 : 0;
                }
                else
                {
                    cache_set->PLRU_BIT[7] = way < (num_way / 16) ? 1 : 0;
                }
            }
        }
        else
        {
            //[8]
            cache_set->PLRU_BIT[8] = way < (num_way / 4) ? 1 : 0;
            way &= 0x3;
            if (cache_set->PLRU_BIT[8])
            {
                //[2]
                cache_set->PLRU_BIT[9] = way < (num_way / 8) ? 1 : 0;
                way &= 0x1;
                //[3] [4]
                if (cache_set->PLRU_BIT[9])
                {
                    cache_set->PLRU_BIT[10] = way < (num_way / 16) ? 1 : 0;
                }
                else
                {
                    cache_set->PLRU_BIT[11] = way < (num_way / 16) ? 1 : 0;
                }
            }
            else
            {
                //[5]
                cache_set->PLRU_BIT[12] = way < (num_way / 8) ? 1 : 0;
                way &= 0x1;
                //[6] [7]
                if (cache_set->PLRU_BIT[12])
                {
                    cache_set->PLRU_BIT[13] = way < (num_way / 16) ? 1 : 0;
                }
                else
                {
                    cache_set->PLRU_BIT[14] = way < (num_way / 16) ? 1 : 0;
                }
            }
        }
    }
}


bool cache_access(int x_align, int y_align, int poc, cache_model_st *cache_model, cache_model_st *cache_l1_model = NULL)
{
    cache_set_st *cache_set;
    cache_entry_st *entry;
    unsigned int index_x = x_align & cache_model->idx_x_mask;
    unsigned int index_y = y_align & cache_model->idx_y_mask;
    cache_set = &cache_model->cvi_cache_set[index_y][index_x];
    for (int i = 0; i < cache_model->num_way; i++)
    {
        entry = &cache_set->entry[i];
        if (entry->valid && entry->poc == poc && entry->x == x_align && entry->y == y_align)
        {
            cache_model->hit_cnt++;
            //if (log_enable)
             //   fprintf(cache_model->cache_log_fp, "[HIT]%d,%d, way %d, index %d,%d\n", x_align, y_align, i, index_x, index_y);
            update_LRU(i, cache_set, cache_model->num_way);
            if (cache_model->cache_fp && cache_model->dump_enable) {
                unsigned char tmp[6] = {0};
                for (int k = 0; k < (cache_model->num_way - 1); k++)
                    tmp[3] = ((cache_set->PLRU_BIT[k] & 1) << (k + 1)) | tmp[3];
                tmp[5] = i;
                tmp[4] = (index_x & 0xf) | ((index_y & 0xf) << 4);
                tmp[3] |= 1; //hit
                tmp[0] = x_align & 0xff;
                tmp[1] = y_align & 0xff;
                tmp[2] = poc & 0xff;
                fwrite(tmp, 1, 6, cache_model->cache_fp);
                //if ((cache_model->hit_cnt + cache_model->miss_cnt) > 25610 && (cache_model->hit_cnt + cache_model->miss_cnt) < 25630)
                //if ((cache_model->hit_cnt + cache_model->miss_cnt) < 100)
                    //printf("[HIT][%d]%d,%d, way %d, index %d,%d, PLRU %d\n", (cache_model->hit_cnt + cache_model->miss_cnt), x_align, y_align, i, index_x, index_y, tmp[3]);
            }
            return true;
        }
    }

    //miss
    int nWay = replace_LRU(cache_set, cache_model->num_way);
    update_LRU(nWay, cache_set, cache_model->num_way);
    cache_model->miss_cnt++;
    entry = &cache_set->entry[nWay];
    entry->valid = true;
    entry->poc = poc;
    entry->x = x_align;
    entry->y = y_align;
    cache_set->way_cnt[nWay]++;

    if (log_enable)
    {
        if (cache_l1_model == &me_cache)
            fprintf(cache_model->cache_log_fp, "[MISS]%d,%d, way %d, index %d,%d (ME)\n", x_align, y_align, nWay, index_x, index_y);
        else
            fprintf(cache_model->cache_log_fp, "[MISS]%d,%d, way %d, index %d,%d (MC)\n", x_align, y_align, nWay, index_x, index_y);
    }
    if (cache_model->cache_fp && cache_model->dump_enable) {
        unsigned char tmp[6] = {0};
        for (int k = 0; k < (cache_model->num_way - 1); k++)
            tmp[3] = ((cache_set->PLRU_BIT[k] & 1) << (k + 1)) | tmp[3];
        tmp[5] = nWay;
        tmp[4] = (index_x & 0xf) | ((index_y & 0xf) << 4);
        tmp[3] |= 0; //miss
        tmp[0] = x_align & 0xff;
        tmp[1] = y_align & 0xff;
        tmp[2] = poc & 0xff;
        fwrite(tmp, 1, 6, cache_model->cache_fp);
        //if ((cache_model->hit_cnt + cache_model->miss_cnt) > 25610 && (cache_model->hit_cnt + cache_model->miss_cnt) < 25630)
        //if ((cache_model->hit_cnt + cache_model->miss_cnt) < 100)
            //printf("[MISS][%d]%d,%d, way %d, index %d,%d PLRU %d\n", (cache_model->hit_cnt + cache_model->miss_cnt), x_align, y_align, nWay, index_x, index_y, tmp[3]);
    }

    if ((cache_model == &mc_cache || cache_model == &me_cache) && (me_l2_cache.num_way != 0))
    {
        bool l2_hit = cache_access(x_align, y_align, poc, &me_l2_cache, cache_model);
        if (l2_hit)
            cache_model->l2_hit_cnt++;
        else
            cache_model->l2_miss_cnt++;
    }

    return false;
}

bool cache_1d_access(int x, int y, unsigned int address, int poc, cache_1d_model_st *cache_model)
{
    cache_entry_st *entry;
    unsigned int index = (address / cache_model->cache_line) & cache_model->idx_mask;
    cache_set_st *cache_set = &cache_model->cvi_cache_set[index];
    for (int i = 0; i < cache_model->num_way; i++)
    {
        entry = &cache_set->entry[i];
        if (entry->valid && entry->poc == poc && entry->x == address)
        {
            cache_model->hit_cnt++;
            //if (log_enable)
              //  fprintf(cache_model->cache_log_fp, "[HIT](%d,%d), %d, way %d, index %d\n", x, y, address, i, index);
            update_LRU(i, cache_set, cache_model->num_way);
            if (cache_model->cache_fp && cache_model->dump_enable) {
                unsigned char tmp[6] = {0};
                for (int k = 0; k < (cache_model->num_way - 1); k++)
                    tmp[3] = ((cache_set->PLRU_BIT[k] & 1) << (k + 1)) | tmp[3];
                tmp[5] = i;
                tmp[4] = (index & 0xf);
                tmp[3] |= 1; //hit
                tmp[0] = address & 0xff;
                tmp[1] = 0;
                tmp[2] = poc & 0xff;
                fwrite(tmp, 1, 6, cache_model->cache_fp);
                //if ((cache_model->hit_cnt + cache_model->miss_cnt) > 25610 && (cache_model->hit_cnt + cache_model->miss_cnt) < 25630)
                //if ((cache_model->hit_cnt + cache_model->miss_cnt) < 100)
                    //printf("[HIT][%d]%d,%d, way %d, index %d,%d, PLRU %d\n", (cache_model->hit_cnt + cache_model->miss_cnt), x_align, y_align, i, index_x, index_y, tmp[3]);
            }
            return true;
        }
    }

    //miss
    int nWay = replace_LRU(cache_set, cache_model->num_way);
    update_LRU(nWay, cache_set, cache_model->num_way);

    cache_model->miss_cnt++;
    entry = &cache_set->entry[nWay];
    if (log_enable)
    {
        fprintf(cache_model->cache_log_fp, "[MISS](%d, %d) %d, way %d, index %d (vaild = %d)\n", x, y, address, nWay, index, entry->valid);
    }
    entry->valid = true;
    entry->poc = poc;
    entry->x = address;

    if (cache_model->cache_fp && cache_model->dump_enable) {
        unsigned char tmp[6] = {0};
        for (int k = 0; k < (cache_model->num_way - 1); k++)
            tmp[3] = ((cache_set->PLRU_BIT[k] & 1) << (k + 1)) | tmp[3];
        tmp[5] = nWay;
        tmp[4] = (index & 0xf);
        tmp[3] |= 0; //miss
        tmp[0] = address & 0xff;
        tmp[1] = 0;
        tmp[2] = poc & 0xff;
        fwrite(tmp, 1, 6, cache_model->cache_fp);
        //if ((cache_model->hit_cnt + cache_model->miss_cnt) > 25610 && (cache_model->hit_cnt + cache_model->miss_cnt) < 25630)
        //if ((cache_model->hit_cnt + cache_model->miss_cnt) < 100)
            //printf("[MISS][%d]%d,%d, way %d, index %d,%d PLRU %d\n", (cache_model->hit_cnt + cache_model->miss_cnt), x_align, y_align, nWay, index_x, index_y, tmp[3]);
    }
    return false;
}

void miu_access_vbc_meta(int x, int y, int w, int h, int poc, bool isChroma)
{
    cache_model_st *cache_model = isChroma ? &vbc_meta_chroma_cache : &vbc_meta_cache;

    if (x < 0)
    {
        w += x;
        x = 0;
        if (w <= 0)
            w = 1;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
        if (h <= 0)
            h = 1;
    }
    if (x >= cache_model->image_w)
    {
        x = cache_model->image_w - 1;
        w = 1;
    }
    if (y >= cache_model->image_h)
    {
        y = cache_model->image_h - 1;
        h = 1;
    }
    unsigned int x_align = x / cache_model->cache_line_width;
    unsigned int y_align = y / cache_model->cache_line_height;
    unsigned int x_end_align = (x + w + cache_model->cache_line_width - 1) / cache_model->cache_line_width * cache_model->cache_line_width;
    unsigned int y_end_align = (y + h + cache_model->cache_line_height - 1) / cache_model->cache_line_height * cache_model->cache_line_height;
    if (x_end_align >= cache_model->image_w)
        x_end_align = cache_model->image_w;
    if (y_end_align >= cache_model->image_h)
        y_end_align = cache_model->image_h;

    x_end_align /= cache_model->cache_line_width;
    y_end_align /= cache_model->cache_line_height;
    unsigned int x_cnt = x_end_align - x_align;
    unsigned int y_cnt = y_end_align - y_align;
    if (x_cnt == 0)
        x_cnt = 1;
    if (y_cnt == 0)
        y_cnt = 1;
    for (int j = 0; j < x_cnt; j++)
    {
        for (int i = 0; i < y_cnt; i++)
        {
            cache_access(x_align + j, y_align + i, poc, cache_model);
        }
    }
}

void miu_access_vbc_meta_1d(int x, int y, int w, int h, int poc, bool isChroma)
{
    cache_1d_model_st *cache_model = &vbc_meta_1d_cache;
    if (!vbc_meta_1d_combine && isChroma)
        cache_model = &vbc_meta_1d_chroma_cache;
    const meta_st *pvbc_meta = &g_vbc_meta[isChroma];

    if (x < 0)
    {
        w += x;
        x = 0;
        if (w <= 0)
            w = 1;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
        if (h <= 0)
            h = 1;
    }
    if (x >= cache_model->image_w)
    {
        x = cache_model->image_w - 1;
        w = 1;
    }
    if (y >= cache_model->image_h)
    {
        y = cache_model->image_h - 1;
        h = 1;
    }

    if (g_VBCVersion >= 1)
    {
        unsigned int x_align = x / pvbc_meta->tile_sub_w * pvbc_meta->tile_sub_w;
        unsigned int y_align = y / pvbc_meta->tile_sub_h * pvbc_meta->tile_sub_h;
        unsigned int x_end_align = (x + w + pvbc_meta->tile_sub_w - 1) / pvbc_meta->tile_sub_w * pvbc_meta->tile_sub_w;
        unsigned int y_end_align = (y + h + pvbc_meta->tile_sub_h - 1) / pvbc_meta->tile_sub_h * pvbc_meta->tile_sub_h;
        if (x_end_align >= cache_model->image_w)
            x_end_align = cache_model->image_w;
        if (y_end_align >= cache_model->image_h)
            y_end_align = cache_model->image_h;

        unsigned int meta_address;
        unsigned int meta_length;
        for (int j = x_align; j < x_end_align; j += pvbc_meta->tile_sub_w)
        {
            for (int i = y_align; i < y_end_align; i += pvbc_meta->tile_sub_h)
            {
                get_vbc_1d_meta_buffer(isChroma, j, i, &meta_address, &meta_length);

                unsigned int align_addr = meta_address / cache_model->cache_line * cache_model->cache_line;
                unsigned int align_size = ((meta_address + meta_length + cache_model->cache_line - 1) / cache_model->cache_line * cache_model->cache_line) - align_addr;

                for (int z = align_addr; z < align_addr + align_size; z += cache_model->cache_line)
                {
                    cache_1d_access(j, i, z, poc, cache_model);
                }
                //cache_access(x_align + j, y_align + i, poc, cache_model);
            }
        }
    }
    else
    {
        unsigned int x_align = x / pvbc_meta->tile_w * pvbc_meta->tile_w;
        unsigned int y_align = y / pvbc_meta->tile_h * pvbc_meta->tile_h;
        unsigned int x_end_align = (x + w + pvbc_meta->tile_w - 1) / pvbc_meta->tile_w * pvbc_meta->tile_w;
        unsigned int y_end_align = (y + h + pvbc_meta->tile_h - 1) / pvbc_meta->tile_h * pvbc_meta->tile_h;
        if (x_end_align >= cache_model->image_w)
            x_end_align = cache_model->image_w;
        if (y_end_align >= cache_model->image_h)
            y_end_align = cache_model->image_h;

        unsigned int meta_address;
        unsigned int meta_length;
        for (int j = x_align; j < x_end_align; j += pvbc_meta->tile_w)
        {
            for (int i = y_align; i < y_end_align; i += pvbc_meta->tile_h)
            {
                get_vbc_1d_meta_buffer(isChroma, j, i, &meta_address, &meta_length);

                unsigned int align_addr = meta_address / cache_model->cache_line * cache_model->cache_line;
                unsigned int align_size = ((meta_address + meta_length + cache_model->cache_line - 1) / cache_model->cache_line * cache_model->cache_line) - align_addr;

                for (int z = align_addr; z < align_addr + align_size; z += cache_model->cache_line)
                {
                    cache_1d_access(j, i, z, poc, cache_model);
                }
                //cache_access(x_align + j, y_align + i, poc, cache_model);
            }
        }
    }
}

void miu_access_vbc(int x, int y, int w, int h, int poc, bool isChroma)
{
    cache_1d_model_st *cache_model = isChroma ? &vbc_1d_chroma_cache : &vbc_1d_cache;
    const meta_st *pvbc_meta = &g_vbc_meta[isChroma];

    if (x < 0)
    {
        w += x;
        x = 0;
        if (w <= 0)
            w = 1;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
        if (h <= 0)
            h = 1;
    }
    if (x >= cache_model->image_w)
    {
        x = cache_model->image_w - 1;
        w = 1;
    }
    if (y >= cache_model->image_h)
    {
        y = cache_model->image_h - 1;
        h = 1;
    }
    unsigned int min_w = pvbc_meta->tile_sub_w;
    unsigned int min_h = pvbc_meta->cu_h;
    unsigned int x_align = x / min_w * min_w;
    unsigned int y_align = y / min_h * min_h;
    unsigned int x_end_align = (x + w + min_w - 1) / min_w * min_w;
    unsigned int y_end_align = (y + h + min_h - 1) / min_h * min_h;
    if (x_end_align >= cache_model->image_w)
        x_end_align = cache_model->image_w;
    if (y_end_align >= cache_model->image_h)
        y_end_align = cache_model->image_h;

    unsigned int address;
    unsigned int length;
    for (int j = x_align; j < x_end_align; j += min_w)
    {
        for (int i = y_align; i < y_end_align; i += min_h)
        {
            get_vbc_meta_info(isChroma, j, i, &address, &length);
            //get_vbc_meta_info_fake_reorder(isChroma, j, i, &address, &length);
            //get_vbc_meta_info_fake(isChroma, j, i, &address, &length);
            unsigned int align_addr = address / cache_model->cache_line * cache_model->cache_line;
            unsigned int align_size = ((address + length + cache_model->cache_line - 1) / cache_model->cache_line * cache_model->cache_line) - align_addr;

            for (int z = 0; z < align_size; z += cache_model->cache_line)
            {
                cache_1d_access(j, i, align_addr + z, poc, cache_model);
            }
            //cache_access(x_align + j, y_align + i, poc, cache_model);
        }
    }
}

void miu_access(int x, int y, int w, int h, int poc, bool isMC, bool isChroma)
{
    cache_model_st *cache_model = &me_cache;
    if (isMC)
    {
        if (isChroma)
        {
            cache_model = &mc_chroma_cache;
        }
        else
        {
            if (isEnableCACHE_MODEL_MC())
                cache_model = &mc_cache;
            else
                cache_model = &me_l2_cache;
        }
    }
    else
    {
#if 1
        if (isEnableVBC())
        {
            assert(isChroma == false);
            miu_access_vbc_meta(x, y, w, h, poc, isChroma);
            if (isEnableVBC() >= 2)
                miu_access_vbc_meta_1d(x, y, w, h, poc, isChroma);
            if (isEnableVBC() >= 3)
                miu_access_vbc(x, y, w, h, poc, isChroma);
        }
#endif
    }
    if (x < 0)
    {
        w += x;
        x = 0;
        if (w <= 0)
            w = 1;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
        if (h <= 0)
            h = 1;
    }
    if (x >= cache_model->image_w)
    {
        x = cache_model->image_w - 1;
        w = 1;
    }
    if (y >= cache_model->image_h)
    {
        y = cache_model->image_h - 1;
        h = 1;
    }

    unsigned int x_align = x / cache_model->cache_line_width;
    unsigned int y_align = y / cache_model->cache_line_height;
    unsigned int x_end_align = (x + w + cache_model->cache_line_width - 1) / cache_model->cache_line_width * cache_model->cache_line_width;
    unsigned int y_end_align = (y + h + cache_model->cache_line_height - 1) / cache_model->cache_line_height * cache_model->cache_line_height;
    if (x_end_align >= cache_model->image_w)
        x_end_align = cache_model->image_w;
    if (y_end_align >= cache_model->image_h)
        y_end_align = cache_model->image_h;

    x_end_align /= cache_model->cache_line_width;
    y_end_align /= cache_model->cache_line_height;
    unsigned int x_cnt = x_end_align - x_align;
    unsigned int y_cnt = y_end_align - y_align;
    if (x_cnt == 0)
        x_cnt = 1;
    if (y_cnt == 0)
        y_cnt = 1;
    for (int j = 0; j < x_cnt; j++)
        for (int i = 0; i < y_cnt; i++)
        {
            cache_access(x_align + j, y_align + i, poc, cache_model);
        }
}

void miu_access_fme(int x, int y, int w, int h, int poc)
{

    if (w == 16 && h == 16)
    {
        // two 8x16
        miu_access(x - 4, y - 4, 8 + 8, 16 + 8, poc);
        miu_access(x + 8 - 4, y - 4, 8 + 8, 16 + 8, poc);
    }
    else if (w == 8 && h == 8)
    {
        miu_access(x - 4, y - 4, w + 8, h + 8, poc);
    }
    else
    {
        assert(0);
    }
}

void get_vbc_meta(TComDataCU*& rpcTempCU, bool isChroma)
{

    TComMv rcMv = rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->getMv( 0 );
    int index_x = rpcTempCU->getCUPelX();
    int index_y = rpcTempCU->getCUPelY();
    int refPOC = rpcTempCU->getSlice()->getRefPOC( REF_PIC_LIST_0, rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( 0 ));
    int w = rpcTempCU->getWidth(0);
    int h = rpcTempCU->getHeight(0);

    // Hor: always padding, Ver: padding only if fract mv
    if (isChroma)
    {
        w = w >> 1;
        h = h >> 1;
        index_x = index_x >> 1;
        index_y = index_y >> 1;

        index_x += (rcMv.getHor() >> 3);
        index_y += (rcMv.getVer() >> 3);
        index_x -= 1;
        w += (1 + 2);
        if ((rcMv.getVer() & 7) != 0)
        {
            index_y -= 1;
            h += (1 + 2);
        }
        w *= 2; //CbCr
        index_x *= 2;
    }
    else
    {
        index_x += (rcMv.getHor() >> 2);
        index_y += (rcMv.getVer() >> 2);
        index_x -= 3;
        w += (3 + 4);
        if ((rcMv.getVer() & 3) != 0)
        {
            index_y -= 3;
            h += (3 + 4);
        }
    }

    miu_access_vbc_meta(index_x, index_y, w, h, refPOC, isChroma);
    if (isEnableVBC() >= 2)
    //if (isEnableVBC() >= 2 && !isChroma && w > 32 )
    {
        miu_access_vbc_meta_1d(index_x, index_y, w, h, refPOC, isChroma);
        if (isEnableVBC() >= 3)
            miu_access_vbc(index_x, index_y, w, h, refPOC, isChroma);
    }

}

void miu_access_mc(TComDataCU*& rpcTempCU, bool isChroma)
{

    TComMv rcMv = rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->getMv( 0 );
    int index_x = rpcTempCU->getCUPelX();
    int index_y = rpcTempCU->getCUPelY();
    int refPOC = rpcTempCU->getSlice()->getRefPOC( REF_PIC_LIST_0, rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( 0 ));
    int w = rpcTempCU->getWidth(0);
    int h = rpcTempCU->getHeight(0);

    // Hor: always padding, Ver: padding only if fract mv
    if (isChroma)
    {
        w = w >> 1;
        h = h >> 1;
        index_x = index_x >> 1;
        index_y = index_y >> 1;

        index_x += (rcMv.getHor() >> 3);
        index_y += (rcMv.getVer() >> 3);
        index_x -= 1;
        w += (1 + 2);
        if ((rcMv.getVer() & 7) != 0)
        {
            index_y -= 1;
            h += (1 + 2);
        }
    }
    else
    {
        index_x += (rcMv.getHor() >> 2);
        index_y += (rcMv.getVer() >> 2);
        index_x -= 3;
        w += (3 + 4);
        if ((rcMv.getVer() & 3) != 0)
        {
            index_y -= 3;
            h += (3 + 4);
        }
    }
    miu_access(index_x, index_y, w, h, refPOC, true, isChroma);

    if (isEnableVBC())
        get_vbc_meta(rpcTempCU, isChroma);
}

void cache_1d_malloc(cache_1d_model_st *cache_1d_model, string cache_type)
{
    int dimWay = cache_1d_model->num_way;
    int dimX = cache_1d_model->cache_size / cache_1d_model->cache_line;
    cache_1d_model->idx_mask = dimX - 1;
    cache_1d_model->cvi_cache_set = new cache_set_st[dimX];
    for(int x = 0; x < dimX; ++x)
    {
        cache_1d_model->cvi_cache_set[x].entry = new cache_entry_st[dimWay];
        cache_1d_model->cvi_cache_set[x].PLRU_BIT = new int[dimWay];
        for(int z = 0; z < dimWay; ++z) {
            cache_1d_model->cvi_cache_set[x].entry[z].valid = false;
            cache_1d_model->cvi_cache_set[x].entry[z].x = 0;
            cache_1d_model->cvi_cache_set[x].entry[z].poc = 0;
            cache_1d_model->cvi_cache_set[x].PLRU_BIT[z] = 0;
        }
    }

    string SigdumpFileName = get_file_name() + "_" + cache_type + "_cache_statistics.csv";
    cache_1d_model->cache_statistics.cache_statistics_fp = fopen(SigdumpFileName.c_str(), "w");

    cache_1d_model->cache_statistics.min_byte_cnt = MAX_UINT;
    cache_1d_model->cache_statistics.max_byte_cnt = 0;

    cache_1d_model->cache_statistics.min_byte_ratio = MAX_UINT;
    cache_1d_model->cache_statistics.max_byte_ratio = 0;
}

void cache_malloc(cache_model_st *cache_model, string cache_type)
{
    int dimWay = cache_model->num_way;
    int dimY = cache_model->cache_height / cache_model->cache_line_height;
    int dimX = cache_model->cache_width / cache_model->cache_line_width;
    cache_model->idx_x_mask = dimX - 1;
    cache_model->idx_y_mask = dimY - 1;

    cache_model->cvi_cache_set = new cache_set_st*[dimY];
    for(int y = 0; y < dimY; ++y) {
        cache_model->cvi_cache_set[y] = new cache_set_st[dimX];
        for(int x = 0; x < dimX; ++x) {
            cache_model->cvi_cache_set[y][x].entry = new cache_entry_st[dimWay];
            cache_model->cvi_cache_set[y][x].PLRU_BIT = new int[dimWay];
            for(int z = 0; z < dimWay; ++z) {
                cache_model->cvi_cache_set[y][x].entry[z].valid = false;
                cache_model->cvi_cache_set[y][x].entry[z].x = 0;
                cache_model->cvi_cache_set[y][x].entry[z].y = 0;
                cache_model->cvi_cache_set[y][x].entry[z].poc = 0;
                cache_model->cvi_cache_set[y][x].PLRU_BIT[z] = 0;
                cache_model->cvi_cache_set[y][x].way_cnt[z] = 0;
            }
        }
    }

    string SigdumpFileName = get_file_name() + "_" + cache_type + "_cache_statistics.csv";
    cache_model->cache_statistics.cache_statistics_fp = fopen(SigdumpFileName.c_str(), "w");

    cache_model->cache_statistics.min_byte_cnt = MAX_UINT;
    cache_model->cache_statistics.max_byte_cnt = 0;

    cache_model->cache_statistics.min_byte_ratio = MAX_UINT;
    cache_model->cache_statistics.max_byte_ratio = 0;
}

void cache_cfg(int w, int h)
{
    char cache_type[512], type[512];
    FILE *cache_cfg_fp = fopen(g_algo_cfg.m_CacheCfgFileName, "r");
    cache_model_st *cache_model = NULL;
    cache_1d_model_st *cache_1d_model = NULL;
    if (cache_cfg_fp == NULL) {
        printf("[ERROR] open %s fail\n", g_algo_cfg.m_CacheCfgFileName);
        return;
    }
    while (1)
    {
        cache_model = NULL;
        cache_1d_model = NULL;
        if (fscanf(cache_cfg_fp, "%s", cache_type) != 1)
        {
            break;
        }
        if (!strcmp (cache_type, "EOF"))
        {
            break;
        }
        else if (!strcmp (cache_type, "ME"))
        {
            cache_model = &me_cache;
        }
        else if (!strcmp (cache_type, "ME_L2"))
        {
            cache_model = &me_l2_cache;
        }
        else if (!strcmp (cache_type, "MC"))
        {
            cache_model = &mc_cache;
        }
        else if (!strcmp (cache_type, "MC_CHROMA"))
        {
            cache_model = &mc_chroma_cache;
        }
        else if (!strcmp (cache_type, "VBC_META"))
        {
            cache_model = &vbc_meta_cache;
        }
        else if (!strcmp (cache_type, "VBC_META_1D"))
        {
            cache_1d_model = &vbc_meta_1d_cache;
        }
        else if (!strcmp (cache_type, "VBC_1D"))
        {
            cache_1d_model = &vbc_1d_cache;
        }
        else if (!strcmp (cache_type, "VBC_1D_CHROMA"))
        {
            cache_1d_model = &vbc_1d_chroma_cache;
        }
        else if (!strcmp (cache_type, "VBC_META_CHROMA"))
        {
            cache_model = &vbc_meta_chroma_cache;
        }
        else if (!strcmp (cache_type, "VBC_META_1D_CHROMA"))
        {
            cache_1d_model = &vbc_meta_1d_chroma_cache;
        }
        else
        {
            printf("Err type %s\n", cache_type);
            assert(0);
            break;
        }

        if (cache_model)
        {
            if (fscanf(cache_cfg_fp, "%s %d", type, &cache_model->num_way) != 2)
                break;
            if (fscanf(cache_cfg_fp, "%s %d", type, &cache_model->cache_width) != 2)
                break;
            if (fscanf(cache_cfg_fp, "%s %d", type, &cache_model->cache_height) != 2)
                break;
            if (fscanf(cache_cfg_fp, "%s %d", type, &cache_model->cache_line_width) != 2)
                break;
            if (fscanf(cache_cfg_fp, "%s %d", type, &cache_model->cache_line_height) != 2)
                break;
            if (fscanf(cache_cfg_fp, "%s %d", type, &cache_model->dump_enable) != 2)
                break;

            cache_model->miss_cnt = 0;
            cache_model->hit_cnt = 0;
            cache_model->l2_miss_cnt = 0;
            cache_model->l2_hit_cnt = 0;
            cache_model->image_w = (w + 31) / 32 * 32;
            cache_model->image_h = (h + 15) / 16 * 16;
            if (!isEnableVBC() && (!strcmp (cache_type, "VBC_META")))
            {
                cache_model->num_way = 0;
                break;
            }
            if (!strcmp (cache_type, "MC_CHROMA"))
            {
                cache_model->image_w /= 2;
                cache_model->image_h /= 2;
            }
            cache_malloc(cache_model, cache_type);
        }
        else if (cache_1d_model)
        {
            if (fscanf(cache_cfg_fp, "%s %d", type, &cache_1d_model->num_way) != 2)
                break;
            if (fscanf(cache_cfg_fp, "%s %d", type, &cache_1d_model->cache_size) != 2)
                break;
            if (fscanf(cache_cfg_fp, "%s %d", type, &cache_1d_model->cache_line) != 2)
                break;
            if (fscanf(cache_cfg_fp, "%s %d", type, &cache_1d_model->dump_enable) != 2)
                break;

            if ((isEnableVBC() == 1) && ((!strcmp (cache_type, "VBC_1D")) || (!strcmp (cache_type, "VBC_1D_CHROMA"))
                || (!strcmp (cache_type, "VBC_META_1D")) || (!strcmp (cache_type, "VBC_META_1D_CHROMA"))))
            {
                cache_1d_model->num_way = 0;
            }
            else if ((isEnableVBC() == 2) && ((!strcmp (cache_type, "VBC_1D")) || (!strcmp (cache_type, "VBC_1D_CHROMA"))))
            {
                cache_1d_model->num_way = 0;
            }
            else
            {
                cache_1d_model->miss_cnt = 0;
                cache_1d_model->hit_cnt = 0;
                cache_1d_model->l2_miss_cnt = 0;
                cache_1d_model->l2_hit_cnt = 0;
                cache_1d_model->image_w = (w + 31) / 32 * 32;
                cache_1d_model->image_h = (h + 15) / 16 * 16;
                if (!strcmp (cache_type, "VBC_1D_CHROMA"))
                {
                    cache_1d_model->image_h /= 2;
                }
                cache_1d_malloc(cache_1d_model, cache_type);
            }
        }
    }

    if (cache_cfg_fp)
        fclose(cache_cfg_fp);
}

void cache_init(int w, int h)
{
    //reset all cache model
    memset(&me_cache, 0x0, sizeof(cache_model_st));
    memset(&me_l2_cache, 0x0, sizeof(cache_model_st));
    memset(&mc_cache, 0x0, sizeof(cache_model_st));
    if (strcmp (g_algo_cfg.m_CacheCfgFileName, ""))
    {
        cache_cfg(w, h);
    }
    else
    {
        me_cache.num_way = 8;
        me_cache.cache_height = 16;
        me_cache.cache_width = 32;
        me_cache.cache_line_height = 8;
        me_cache.cache_line_width = 16;
        me_cache.miss_cnt = 0;
        me_cache.hit_cnt = 0;
        me_cache.l2_miss_cnt = 0;
        me_cache.l2_hit_cnt = 0;
        me_cache.image_w = (w + 31) / 32 * 32;
        me_cache.image_h = (h + 15) / 16 * 16;
        cache_malloc(&me_cache, "ME");

        me_l2_cache.num_way = 8;
        me_l2_cache.cache_height = 16;
        me_l2_cache.cache_width = 64;
        me_l2_cache.cache_line_height = 8;
        me_l2_cache.cache_line_width = 16;
        me_l2_cache.miss_cnt = 0;
        me_l2_cache.hit_cnt = 0;
        me_l2_cache.l2_miss_cnt = 0;
        me_l2_cache.l2_hit_cnt = 0;
        me_l2_cache.image_w = (w + 31) / 32 * 32;
        me_l2_cache.image_h = (h + 15) / 16 * 16;
        cache_malloc(&me_l2_cache, "ME_L2");

        mc_cache.num_way = 8;
        mc_cache.cache_height = 16;
        mc_cache.cache_width = 32;
        mc_cache.cache_line_height = 8;
        mc_cache.cache_line_width = 16;
        mc_cache.miss_cnt = 0;
        mc_cache.hit_cnt = 0;
        mc_cache.l2_miss_cnt = 0;
        mc_cache.l2_hit_cnt = 0;
        mc_cache.image_w = (w + 31) / 32 * 32;
        mc_cache.image_h = (h + 15) / 16 * 16;
        cache_malloc(&mc_cache, "MC");

        mc_chroma_cache.num_way = 8;
        mc_chroma_cache.cache_height = 8;
        mc_chroma_cache.cache_width = 16;
        mc_chroma_cache.cache_line_height = 8;
        mc_chroma_cache.cache_line_width = 8;
        mc_chroma_cache.miss_cnt = 0;
        mc_chroma_cache.hit_cnt = 0;
        mc_chroma_cache.l2_miss_cnt = 0;
        mc_chroma_cache.l2_hit_cnt = 0;
        mc_chroma_cache.image_w = (w + 31) / 32 * 32 / 2;
        mc_chroma_cache.image_h = (h + 15) / 16 * 16 / 2;
        cache_malloc(&mc_chroma_cache, "MC_CHROMA");

        if (isEnableVBC())
        {
            vbc_meta_cache.num_way = 8;
            vbc_meta_cache.cache_height = 32;
            vbc_meta_cache.cache_width = 64;
            vbc_meta_cache.cache_line_height = 32;
            vbc_meta_cache.cache_line_width = 32;
            vbc_meta_cache.miss_cnt = 0;
            vbc_meta_cache.hit_cnt = 0;
            vbc_meta_cache.l2_miss_cnt = 0;
            vbc_meta_cache.l2_hit_cnt = 0;
            vbc_meta_cache.image_w = (w + 31) / 32 * 32;
            vbc_meta_cache.image_h = (h + 15) / 16 * 16;
            cache_malloc(&vbc_meta_cache, "VBC_META");

            vbc_meta_chroma_cache.num_way = 8;
            vbc_meta_chroma_cache.cache_height = 32;
            vbc_meta_chroma_cache.cache_width = 64;
            vbc_meta_chroma_cache.cache_line_height = 32;
            vbc_meta_chroma_cache.cache_line_width = 32;
            vbc_meta_chroma_cache.miss_cnt = 0;
            vbc_meta_chroma_cache.hit_cnt = 0;
            vbc_meta_chroma_cache.l2_miss_cnt = 0;
            vbc_meta_chroma_cache.l2_hit_cnt = 0;
            vbc_meta_chroma_cache.image_w = (w + 31) / 32 * 32;
            vbc_meta_chroma_cache.image_h = (h / 2 + 15) / 16 * 16;
            cache_malloc(&vbc_meta_chroma_cache, "VBC_META_CHROMA");
        }
        if (isEnableVBC() >= 2)
        {
            vbc_meta_1d_cache.num_way = 8;
            vbc_meta_1d_cache.cache_size = 32;
            vbc_meta_1d_cache.cache_line = 16;
            vbc_meta_1d_cache.miss_cnt = 0;
            vbc_meta_1d_cache.hit_cnt = 0;
            vbc_meta_1d_cache.l2_miss_cnt = 0;
            vbc_meta_1d_cache.l2_hit_cnt = 0;
            vbc_meta_1d_cache.image_w = (w + 31) / 32 * 32;
            vbc_meta_1d_cache.image_h = (h + 15) / 16 * 16;
            cache_1d_malloc(&vbc_meta_1d_cache, "VBC_META_1D");

            vbc_meta_1d_chroma_cache.num_way = 8;
            vbc_meta_1d_chroma_cache.cache_size = 32;
            vbc_meta_1d_chroma_cache.cache_line = 16;
            vbc_meta_1d_chroma_cache.miss_cnt = 0;
            vbc_meta_1d_chroma_cache.hit_cnt = 0;
            vbc_meta_1d_chroma_cache.l2_miss_cnt = 0;
            vbc_meta_1d_chroma_cache.l2_hit_cnt = 0;
            vbc_meta_1d_chroma_cache.image_w = (w + 31) / 32 * 32;
            vbc_meta_1d_chroma_cache.image_h = (h / 2 + 15) / 16 * 16;
            cache_1d_malloc(&vbc_meta_1d_chroma_cache, "VBC_META_1D_CHROMA");
        }
        if (isEnableVBC() >= 3)
        {
            cache_1d_model_st *p1d_cache = &vbc_1d_cache;
            p1d_cache->num_way = 8;
            p1d_cache->cache_size = 64;
            p1d_cache->cache_line = 16;
            p1d_cache->miss_cnt = 0;
            p1d_cache->hit_cnt = 0;
            p1d_cache->l2_miss_cnt = 0;
            p1d_cache->l2_hit_cnt = 0;
            p1d_cache->image_w = (w + 31) / 32 * 32;
            p1d_cache->image_h = (h + 15) / 16 * 16;
            cache_1d_malloc(p1d_cache, "VBC_1D");

            p1d_cache = &vbc_1d_chroma_cache;
            p1d_cache->num_way = 8;
            p1d_cache->cache_size = 64;
            p1d_cache->cache_line = 16;
            p1d_cache->miss_cnt = 0;
            p1d_cache->hit_cnt = 0;
            p1d_cache->l2_miss_cnt = 0;
            p1d_cache->l2_hit_cnt = 0;
            p1d_cache->image_w = (w + 31) / 32 * 32;
            p1d_cache->image_h = (h / 2 + 15) / 16 * 16;
            cache_1d_malloc(p1d_cache, "VBC_1D_CHROMA");
        }
    }
}

void cache_1d_free(cache_1d_model_st *cache_model, string cache_type)
{
    int dimX = cache_model->cache_size / cache_model->cache_line;

    cache_statistics_st cache_statistics = cache_model->cache_statistics;
    if (cache_statistics.cache_statistics_fp)
    {
        printf("%s, max %u, %u%%, min %u, %u%%, avg %u, %u%%\n", cache_type.c_str(),
            cache_statistics.max_byte_cnt, cache_statistics.max_byte_ratio,
            cache_statistics.min_byte_cnt, cache_statistics.min_byte_ratio,
            cache_statistics.avg_byte_cnt / cache_statistics.frm_cnt, cache_statistics.avg_byte_ratio / cache_statistics.frm_cnt);
        fprintf(cache_statistics.cache_statistics_fp, "%u,%u,%u,%u,%u,%u\n",
            cache_statistics.max_byte_cnt, cache_statistics.max_byte_ratio,
            cache_statistics.min_byte_cnt, cache_statistics.min_byte_ratio,
            cache_statistics.avg_byte_cnt / cache_statistics.frm_cnt, cache_statistics.avg_byte_ratio / cache_statistics.frm_cnt);

        fclose(cache_statistics.cache_statistics_fp);
    }
    for(int x = 0; x < dimX; ++x)
    {
        delete cache_model->cvi_cache_set[x].entry;
        delete cache_model->cvi_cache_set[x].PLRU_BIT;
    }
    delete cache_model->cvi_cache_set;
}

void cache_free(cache_model_st *cache_model, string cache_type)
{
    int dimY = cache_model->cache_height / cache_model->cache_line_height;
    int dimX = cache_model->cache_width / cache_model->cache_line_width;
#if 0
    if (!strcmp (cache_type.c_str(), "ME") || !strcmp (cache_type.c_str(), "ME_L2"))
    {
        for(int y = 0; y < dimY; ++y) {
            for(int x = 0; x < dimX; ++x) {
                printf("%s, set(x%d, y%d): ", cache_type.c_str(), x, y);
                for(int z = 0; z < cache_model->num_way; ++z) {
                    printf("%d,", cache_model->cvi_cache_set[y][x].way_cnt[z]);
                }
                printf("\n");
            }
        }
    }
#endif
    cache_statistics_st cache_statistics = cache_model->cache_statistics;
    if (cache_statistics.cache_statistics_fp)
    {
        printf("%s, max %u, %u%%, min %u, %u%%, avg %u, %u%%\n", cache_type.c_str(),
            cache_statistics.max_byte_cnt, cache_statistics.max_byte_ratio,
            cache_statistics.min_byte_cnt, cache_statistics.min_byte_ratio,
            cache_statistics.avg_byte_cnt / cache_statistics.frm_cnt, cache_statistics.avg_byte_ratio / cache_statistics.frm_cnt);
        fprintf(cache_statistics.cache_statistics_fp, "%u,%u,%u,%u,%u,%u\n",
            cache_statistics.max_byte_cnt, cache_statistics.max_byte_ratio,
            cache_statistics.min_byte_cnt, cache_statistics.min_byte_ratio,
            cache_statistics.avg_byte_cnt / cache_statistics.frm_cnt, cache_statistics.avg_byte_ratio / cache_statistics.frm_cnt);

        fclose(cache_statistics.cache_statistics_fp);
    }
    for(int y = 0; y < dimY; ++y)
    {
        for(int x = 0; x < dimX; ++x)
        {
            delete cache_model->cvi_cache_set[y][x].entry;
            delete cache_model->cvi_cache_set[y][x].PLRU_BIT;
        }
        delete cache_model->cvi_cache_set[y];
    }
}

void cache_frm_open(cache_model_st *cache_model, const char* cache_type)
{
    int dimY = cache_model->cache_height / cache_model->cache_line_height;
    int dimX = cache_model->cache_width / cache_model->cache_line_width;

    strncpy(cache_model->cache_type, cache_type, sizeof(cache_model->cache_type));
    cache_model->hit_cnt = 0;
    cache_model->miss_cnt = 0;
    cache_model->l2_hit_cnt = 0;
    cache_model->l2_miss_cnt = 0;
    for(int y = 0; y < dimY; ++y) {
        for(int x = 0; x < dimX; ++x) {
            for(int z = 0; z < cache_model->num_way; ++z) {
                cache_model->cvi_cache_set[y][x].entry[z].poc = 0;
                cache_model->cvi_cache_set[y][x].entry[z].x = 0;
                cache_model->cvi_cache_set[y][x].entry[z].y = 0;
                cache_model->cvi_cache_set[y][x].entry[z].valid = false;
            }
            for (int n = 0; n < (cache_model->num_way - 1); n++)
            cache_model->cvi_cache_set[y][x].PLRU_BIT[n] = 0;
        }
    }

    if (cache_model->dump_enable)
    {
        string SigdumpFileName = get_file_name() + "_" + cache_type + "_cache_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        cache_model->cache_fp = fopen(SigdumpFileName.c_str(), "wb");
    }
    if (log_enable)
    {
        string SigdumpFileName = get_file_name() + "_" + cache_type + "_cache_" + to_string(g_sigpool.enc_count_pattern) + ".log";
        cache_model->cache_log_fp = fopen(SigdumpFileName.c_str(), "w");
    }
}

void cache_frm_open(cache_1d_model_st *cache_model, const char* cache_type)
{
    int dim = cache_model->cache_size / cache_model->cache_line;

    strncpy(cache_model->cache_type, cache_type, sizeof(cache_model->cache_type));
    cache_model->hit_cnt = 0;
    cache_model->miss_cnt = 0;
    cache_model->l2_hit_cnt = 0;
    cache_model->l2_miss_cnt = 0;
    for(int y = 0; y < dim; ++y) {
        for(int z = 0; z < cache_model->num_way; ++z) {
            cache_model->cvi_cache_set[y].entry[z].poc = 0;
            cache_model->cvi_cache_set[y].entry[z].x = 0;
            cache_model->cvi_cache_set[y].entry[z].y = 0;
            cache_model->cvi_cache_set[y].entry[z].valid = false;
        }
        for (int n = 0; n < (cache_model->num_way - 1); n++)
        cache_model->cvi_cache_set[y].PLRU_BIT[n] = 0;
    }

    if (cache_model->dump_enable)
    {
        string SigdumpFileName = get_file_name() + "_" + cache_type + "_cache_" + to_string(g_sigpool.enc_count_pattern) + ".bin";
        cache_model->cache_fp = fopen(SigdumpFileName.c_str(), "wb");
    }
    if (log_enable)
    {
        string SigdumpFileName = get_file_name() + "_" + cache_type + "_cache_" + to_string(g_sigpool.enc_count_pattern) + ".log";
        cache_model->cache_log_fp = fopen(SigdumpFileName.c_str(), "w");
    }
}

void cache_frm_enable()
{
    if (me_cache.num_way != 0)
        cache_frm_open(&me_cache, "ME");

    if (me_l2_cache.num_way != 0)
        cache_frm_open(&me_l2_cache, "ME_L2");

    if (mc_cache.num_way != 0)
        cache_frm_open(&mc_cache, "MC");

    if (mc_chroma_cache.num_way != 0)
        cache_frm_open(&mc_chroma_cache, "MC_CHROMA");

    if (vbc_meta_cache.num_way != 0)
        cache_frm_open(&vbc_meta_cache, "VBC_META");

    if (vbc_meta_chroma_cache.num_way != 0)
        cache_frm_open(&vbc_meta_chroma_cache, "VBC_META_CHROMA");

    if (vbc_meta_1d_cache.num_way != 0)
        cache_frm_open(&vbc_meta_1d_cache, "VBC_META_1D");

    if (vbc_meta_1d_chroma_cache.num_way != 0)
        cache_frm_open(&vbc_meta_1d_chroma_cache, "VBC_META_1D_CHROMA");

    if (vbc_1d_cache.num_way != 0)
        cache_frm_open(&vbc_1d_cache, "VBC_1D");

    if (vbc_1d_chroma_cache.num_way != 0)
        cache_frm_open(&vbc_1d_chroma_cache, "VBC_1D_CHROMA");

}

void cache_1d_frm_report(cache_1d_model_st *cache_model, string cache_type)
{
    unsigned int miu_byte_cnt = cache_model->miss_cnt * cache_model->cache_line;
    int is_chroma = !strcmp (cache_type.c_str(), "MC_CHROMA") ? 2 : 1; //uv interleave
    miu_byte_cnt = miu_byte_cnt * is_chroma;

    unsigned int byte_ratio = 0;

    if (!strcmp (cache_type.c_str(), "VBC_META_1D"))
    {
        byte_ratio = (miu_byte_cnt * 100) / (g_vbc_meta[0].meta_size + g_vbc_meta[1].meta_size);
        printf("[%s] hit cnt: %d, miss cnt: %d, ", cache_type.c_str(), cache_model->hit_cnt, cache_model->miss_cnt);
        printf("miu byte cnt %d, %d%%\n", miu_byte_cnt, byte_ratio);
    }
    else if (!strcmp (cache_type.c_str(), "VBC_META_1D_CHROMA"))
    {
        int isChroma = !strcmp (cache_type.c_str(), "VBC_META_1D_CHROMA") ? 1 : 0;
        byte_ratio = (miu_byte_cnt * 100) / (g_vbc_meta[isChroma].meta_size);
        printf("[%s] hit cnt: %d, miss cnt: %d, ", cache_type.c_str(), cache_model->hit_cnt, cache_model->miss_cnt);
        printf("miu byte cnt %d, %d%%\n", miu_byte_cnt, byte_ratio);
    }
    else if (!strcmp (cache_type.c_str(), "VBC_1D") || !strcmp (cache_type.c_str(), "VBC_1D_CHROMA"))
    {
        int isChroma = !strcmp (cache_type.c_str(), "VBC_1D_CHROMA") ? 1 : 0;
        printf("[%s] hit cnt: %d, miss cnt: %d, ", cache_type.c_str(), cache_model->hit_cnt, cache_model->miss_cnt);
        if (g_vbc_meta[isChroma].total_compress_size == 0)
        {
            printf("[ERROR]No enable VBC Model, compress size = 0\n");
        }
        else
        {
            byte_ratio = (miu_byte_cnt * 100) / (g_vbc_meta[isChroma].total_compress_size);
            printf("miu byte cnt %d, %d%%\n", miu_byte_cnt, byte_ratio);
        }
    }
    cache_model->cache_statistics.avg_byte_cnt += miu_byte_cnt;
    cache_model->cache_statistics.avg_byte_ratio += byte_ratio;

    if (miu_byte_cnt > cache_model->cache_statistics.max_byte_cnt)
        cache_model->cache_statistics.max_byte_cnt = miu_byte_cnt;
    if (miu_byte_cnt < cache_model->cache_statistics.min_byte_cnt)
        cache_model->cache_statistics.min_byte_cnt = miu_byte_cnt;

    if (byte_ratio > cache_model->cache_statistics.max_byte_ratio)
        cache_model->cache_statistics.max_byte_ratio = byte_ratio;
    if (byte_ratio < cache_model->cache_statistics.min_byte_ratio)
        cache_model->cache_statistics.min_byte_ratio = byte_ratio;

    if (cache_model->dump_enable)
    {
        if (cache_model->cache_fp)
            fclose(cache_model->cache_fp);
    }
    if (log_enable)
    {
        if (cache_model->cache_log_fp)
            fclose(cache_model->cache_log_fp);
    }
    cache_model->cache_statistics.frm_cnt++;
}

void cache_frm_report(cache_model_st *cache_model, string cache_type)
{
    unsigned int miu_byte_cnt = cache_model->miss_cnt * cache_model->cache_line_height * cache_model->cache_line_width;
    int is_chroma = !strcmp (cache_type.c_str(), "MC_CHROMA") ? 2 : 1; //uv interleave
    miu_byte_cnt = miu_byte_cnt * is_chroma;
    unsigned int byte_ratio = 0;

    if (!strcmp (cache_type.c_str(), "VBC_META") || !strcmp (cache_type.c_str(), "VBC_META_CHROMA"))
    {
        int isChroma = !strcmp (cache_type.c_str(), "VBC_META_CHROMA") ? 1 : 0;
        unsigned int cache_line_size = cache_model->cache_line_height * cache_model->cache_line_width / g_vbc_meta[isChroma].tile_sub_w / g_vbc_meta[isChroma].tile_sub_h;
        miu_byte_cnt = cache_model->miss_cnt * cache_line_size * g_vbc_meta[isChroma].sub_tile_size;
        byte_ratio = (miu_byte_cnt * 100) / (g_vbc_meta[isChroma].meta_size);
        printf("[%s] hit cnt: %d, miss cnt: %d, ", cache_type.c_str(), cache_model->hit_cnt, cache_model->miss_cnt);
        printf("miu byte cnt %d, %d%%\n", miu_byte_cnt, byte_ratio);
    }
    else
    {
        byte_ratio = (miu_byte_cnt * 100) / (cache_model->image_w * cache_model->image_h * is_chroma);
        printf("[%s] hit cnt: %d, miss cnt: %d, ", cache_type.c_str(), cache_model->hit_cnt, cache_model->miss_cnt);
        if (cache_model->l2_miss_cnt)
            printf("(to L2 miss cnt: %d), ", cache_model->l2_miss_cnt);
        printf("miu byte cnt %d, %d%%\n", miu_byte_cnt, byte_ratio);
    }
    cache_model->cache_statistics.avg_byte_cnt += miu_byte_cnt;
    cache_model->cache_statistics.avg_byte_ratio += byte_ratio;

    if (miu_byte_cnt > cache_model->cache_statistics.max_byte_cnt)
        cache_model->cache_statistics.max_byte_cnt = miu_byte_cnt;
    if (miu_byte_cnt < cache_model->cache_statistics.min_byte_cnt)
        cache_model->cache_statistics.min_byte_cnt = miu_byte_cnt;

    if (byte_ratio > cache_model->cache_statistics.max_byte_ratio)
        cache_model->cache_statistics.max_byte_ratio = byte_ratio;
    if (byte_ratio < cache_model->cache_statistics.min_byte_ratio)
        cache_model->cache_statistics.min_byte_ratio = byte_ratio;

    if (cache_model->dump_enable)
    {
        if (cache_model->cache_fp)
            fclose(cache_model->cache_fp);
    }
    if (log_enable)
    {
        if (cache_model->cache_log_fp)
            fclose(cache_model->cache_log_fp);
    }
    cache_model->cache_statistics.frm_cnt++;
}

void cache_frm_disable()
{
    if (me_cache.num_way != 0)
        cache_frm_report(&me_cache, "ME");

    if (me_l2_cache.num_way != 0)
        cache_frm_report(&me_l2_cache, "ME_L2");

    if (mc_cache.num_way != 0)
        cache_frm_report(&mc_cache, "MC");

    if (mc_chroma_cache.num_way != 0)
        cache_frm_report(&mc_chroma_cache, "MC_CHROMA");

    if (vbc_meta_cache.num_way != 0)
        cache_frm_report(&vbc_meta_cache, "VBC_META");

    if (vbc_meta_chroma_cache.num_way != 0)
        cache_frm_report(&vbc_meta_chroma_cache, "VBC_META_CHROMA");

    if (vbc_meta_1d_cache.num_way != 0)
        cache_1d_frm_report(&vbc_meta_1d_cache, "VBC_META_1D");

    if (vbc_meta_1d_chroma_cache.num_way != 0)
        cache_1d_frm_report(&vbc_meta_1d_chroma_cache, "VBC_META_1D_CHROMA");

    if (vbc_1d_cache.num_way != 0)
        cache_1d_frm_report(&vbc_1d_cache, "VBC_1D");

    if (vbc_1d_chroma_cache.num_way != 0)
        cache_1d_frm_report(&vbc_1d_chroma_cache, "VBC_1D_CHROMA");
}

void cache_deinit()
{
    if (me_cache.num_way != 0)
        cache_free(&me_cache, "ME");

    if (me_l2_cache.num_way != 0)
        cache_free(&me_l2_cache, "ME_L2");

    if (mc_cache.num_way != 0)
        cache_free(&mc_cache, "MC");

    if (mc_chroma_cache.num_way != 0)
        cache_free(&mc_chroma_cache, "MC_CHROMA");

    if (vbc_meta_cache.num_way != 0)
        cache_free(&vbc_meta_cache, "VBC_META");

    if (vbc_meta_chroma_cache.num_way != 0)
        cache_free(&vbc_meta_chroma_cache, "VBC_META_CHROMA");

    if (vbc_meta_1d_cache.num_way != 0)
        cache_1d_free(&vbc_meta_1d_cache, "VBC_META_1D");

    if (vbc_meta_1d_chroma_cache.num_way != 0)
        cache_1d_free(&vbc_meta_1d_chroma_cache, "VBC_META_1D_CHROMA");

    if (vbc_1d_cache.num_way != 0)
        cache_1d_free(&vbc_1d_cache, "VBC_1D");

    if (vbc_1d_chroma_cache.num_way != 0)
        cache_1d_free(&vbc_1d_chroma_cache, "VBC_1D_CHROMA");

}

