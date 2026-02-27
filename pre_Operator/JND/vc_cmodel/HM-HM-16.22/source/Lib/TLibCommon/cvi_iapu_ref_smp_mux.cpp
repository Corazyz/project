#include<cstdio>
#include<cstdint>
#include<vector>
#include<algorithm>
#include<cmath>
#include<cassert>

#include "cvi_iapu_ref_smp_mux.h"

using namespace::std;

iapu_mux_one_mode::iapu_mux_one_mode(const int mode):ang_mode(mode), block_size(-1), pixel_per_cycle(-1), is_restructed(false){
}

iapu_mux_one_mode::~iapu_mux_one_mode(){
    
}

void iapu_mux_one_mode::restruct(const int block_size=-1, const int pixel_per_cycle=-1){
//{{{
    assert(block_size>0);
    assert(pixel_per_cycle>0);

    if(this->block_size==block_size && this->pixel_per_cycle==pixel_per_cycle)
        return;

    this->block_size         = block_size;
    this->pixel_per_cycle    = pixel_per_cycle;

    if(is_restructed)
        return;
    is_restructed = true;

    int iter_size   = static_cast<int>(ceil(block_size*1.0/pixel_per_cycle)*block_size);
    int seg_num     = iter_size/block_size;

    l1_tbl.resize(seg_num);
    p1_tbl.resize(seg_num);
    p2_tbl.resize(seg_num);
    f_tbl.resize(seg_num);

    for(int seg_idx=0; seg_idx<seg_num; seg_idx++){
        int m_num = max(1, pixel_per_cycle/mux_width);

        l1_tbl[seg_idx].resize(m_num);
        p1_tbl[seg_idx].resize(m_num);
        p2_tbl[seg_idx].resize(m_num);
        f_tbl[seg_idx].resize(m_num);

        for(int m_idx=0; m_idx<m_num; m_idx++){
            l1_tbl[seg_idx][m_idx].resize(block_size/statistics_cycle);
            p1_tbl[seg_idx][m_idx].resize(block_size);
            p2_tbl[seg_idx][m_idx].resize(block_size);
            f_tbl[seg_idx][m_idx].resize(block_size);

            for(int b_idx=0; b_idx<block_size; b_idx++){
                p1_tbl[seg_idx][m_idx][b_idx].resize(mux_width);
                p2_tbl[seg_idx][m_idx][b_idx].resize(mux_width);
                f_tbl[seg_idx][m_idx][b_idx].resize(mux_width);
            }
        }
    }



    //
    p1_info.resize(block_size);
    p2_info.resize(block_size);
    f_info.resize(block_size);
    for(int i=0; i<block_size; i++){
        p1_info[i].resize(block_size);
        p2_info[i].resize(block_size);
        f_info[i].resize(block_size);

        for(int j=0; j<block_size; j++){
            p1_info[i][j] = {0};
            p2_info[i][j] = {0};
            f_info[i][j]  = {0};
        }
    }

    return;
//}}}
}

int iapu_mux_one_mode::find_ref_smp_i(const int x, const int y){     // take only +1
    int l2_p1_idx   = *p1_info[y][x].vref;
    int l1_seg_idx  = p1_info[y][x].seg_idx;
    int l1_m_idx    = p1_info[y][x].m_idx;
    int l1_sta_idx  = p1_info[y][x].sta_idx;
    int l1_p1_idx   = l1_tbl[l1_seg_idx][l1_m_idx][l1_sta_idx][l2_p1_idx];

    //! remember to add offset

    return l1_p1_idx;
}
int iapu_mux_one_mode::find_ref_smp_f(const int x, const int y){
    return  *f_info[y][x].vref;
    
}

/************/

int iapu_ref_mux_generator::get_i(const int ang_mode, const int x, const int y){
    if(ang_mode>=pivot_mode){
        return ((y+1)*A[ang_mode])>>5;
    }else{
        return ((x+1)*A[ang_mode])>>5;
    }
    return -100;
}

int iapu_ref_mux_generator::get_f(const int ang_mode, const int x, const int y){
    if(ang_mode>=pivot_mode){
        return ((y+1)*A[ang_mode])&31;
    }else{
        return ((x+1)*A[ang_mode])&31;
    }
    return -100;
}


void iapu_ref_mux_generator::mux_unit_gen(const int ang_mode, const int seg_idx, const int m_idx, const int xinit, const int yinit, const int block_size, const int pixel_per_cycle, const int buf_align, vector<vector<int> >& l1_tbl, vector<vector<int> >& p1_tbl, vector<vector<int> >& p2_tbl, vector<vector<int> >& f_tbl, vector<vector<l2_mux_info> >& p1_info, vector<vector<l2_mux_info> >& p2_info, vector<vector<l2_mux_info> >& f_info){
//{{{

//    int xinit, yinit;
    int xstep;
    int iter    = 0;

    int x, y, w, i, f;

    int ref_idx_p1, ref_idx_p2, ref_idx_ali;
    vector<int> ref_idx_buf;
    vector<int> ref_ali_buf;

    
    
    iter    = 0;

    do{
        xstep = 1;

        for(w=0; w<mux_width; w++){
            x = xinit+ w*xstep;
            y = yinit+ (iter*max(1, pixel_per_cycle/block_size))%block_size;

            i = get_i(ang_mode, x, y);

            if(ang_mode>=pivot_mode){
                ref_idx_p1 = x+i+1;
                ref_idx_p2 = x+i+2;
            }else{
                ref_idx_p1 = y+i+1;
                ref_idx_p2 = y+i+2;
            }
 
            p1_tbl[iter][x-xinit] = ref_idx_p1;
            p2_tbl[iter][x-xinit] = ref_idx_p2;


            ref_idx_buf.push_back(ref_idx_p1);
            ref_idx_buf.push_back(ref_idx_p2);
        }


        if((iter+1)% statistics_cycle==0){
            sort( ref_idx_buf.begin(), ref_idx_buf.end() );            
            ref_idx_buf.erase( unique( ref_idx_buf.begin(), ref_idx_buf.end() ), ref_idx_buf.end() );

            ref_idx_ali = floor(ref_idx_buf[0]*1.0/buf_align)*buf_align;            //align to
            ref_ali_buf.push_back(ref_idx_ali);                                     //save alignment

            w = ref_idx_buf[0] - ref_idx_ali;
            for(i=0; i< w; i++){
                ref_idx_buf.insert(ref_idx_buf.begin(), 0);
            }
            l1_tbl[iter/statistics_cycle]= ref_idx_buf;

            ref_idx_buf.clear();
        }

        iter++;
    }while(iter < block_size ); 


    //
    iter    = 0;

    do{
        xstep = 1;

        for(w=0; w<mux_width; w++){
            x = xinit+ w*xstep;
            y = yinit+ (iter*max(1, pixel_per_cycle/block_size))%block_size;

            f = get_f(ang_mode, x, y);
            f_tbl[iter][x-xinit] = f;
            
            p1_tbl[iter][x-xinit] -= ref_ali_buf[iter/statistics_cycle];
            p2_tbl[iter][x-xinit] -= ref_ali_buf[iter/statistics_cycle];
            ref_idx_p1 = p1_tbl[iter][x-xinit];
            ref_idx_p2 = p2_tbl[iter][x-xinit];

            //
            p1_info[y][x].seg_idx   = seg_idx;
            p1_info[y][x].m_idx     = m_idx;
            p1_info[y][x].sta_idx   = iter/statistics_cycle;
            p1_info[y][x].vref      = &p1_tbl[iter][x-xinit];
            p2_info[y][x].seg_idx   = seg_idx;
            p2_info[y][x].m_idx     = m_idx;
            p2_info[y][x].sta_idx   = iter/statistics_cycle;
            p2_info[y][x].vref      = &p2_tbl[iter][x-xinit];
            f_info[y][x].seg_idx    = seg_idx;
            f_info[y][x].m_idx      = m_idx;
            f_info[y][x].sta_idx    = iter/statistics_cycle;
            f_info[y][x].vref       = &f_tbl[iter][x-xinit];

        }

        iter++;
    }while(iter < block_size ); 



    
    return;
//}}}    
}

void iapu_ref_mux_generator::mux_module_gen(const int ang_mode, const int block_size, const int pixel_per_cycle, const int buf_align, iapu_mux_one_mode& ref_smp_mux){
//{{{

    int i;
    int iter_size;
    int seg_idx, seg_num;
    int m_idx, m_num;
    int max_l1;
    int xinit, yinit;

    vector<vector<int> > l1_tbl;

    
    //
    iter_size   = static_cast<int>(ceil(block_size*1.0/pixel_per_cycle)*block_size);
    seg_num     = iter_size/block_size;

    for(seg_idx=0; seg_idx<seg_num; seg_idx++){
        m_num = max(1, pixel_per_cycle/mux_width);  
        for(m_idx=0; m_idx<m_num; m_idx++){

            xinit   = m_idx*mux_width % block_size + seg_idx*pixel_per_cycle;
            yinit   = m_idx*mux_width / block_size;
            mux_unit_gen(ang_mode, seg_idx, m_idx, xinit, yinit, block_size, pixel_per_cycle, buf_align, ref_smp_mux.l1_tbl[seg_idx][m_idx], ref_smp_mux.p1_tbl[seg_idx][m_idx], ref_smp_mux.p2_tbl[seg_idx][m_idx], ref_smp_mux.f_tbl[seg_idx][m_idx], ref_smp_mux.p1_info, ref_smp_mux.p2_info, ref_smp_mux.f_info);
           
            for(i=0; i<static_cast<int>(l1_tbl.size()); i++){
                if(static_cast<int>(l1_tbl[i].size())>max_l1){
                    max_l1 = l1_tbl[i].size();
                }
            }
        }
    }

    return;
//}}}    
}


//void iapu_ref_mux_generator::print_table(const int block_size, const int pixel_per_cycle, const int max_l1, iapu_mux_one_mode ref_smp_muxes[]){
////{{{
//    int i; 
//    int iter_size;
//    int seg_idx, seg_num;
//    int m_idx, m_num;
//    int mode, mode_idx;
//    int x, y;
//
//    printf("print_table\n");
//
//    printf("mux_L1 table\n");
//    for(mode_idx=0, mode=2; mode<=34; mode_idx++, mode++){
//        iter_size   = static_cast<int>(ceil(block_size*1.0/pixel_per_cycle)*block_size);
//        seg_num     = iter_size/block_size;
//
//        for(seg_idx=0; seg_idx<seg_num; seg_idx++){
//            for(y=0; y < block_size/statistics_cycle; y++){
//                printf("mode[%2d], ", mode);
//                m_num = pixel_per_cycle/mux_width;
//                for(m_idx=0; m_idx<m_num; m_idx++){
//                    for(x=0; x< static_cast<int>(ref_smp_muxes[mode].l1_tbl[seg_idx][m_idx][y].size()); x++){
//                        printf("%3d, ", ref_smp_muxes[mode].l1_tbl[seg_idx][m_idx][y][x]);
//                    }
//                    for(i=0; i<max_l1-x; i++){
//                        printf("%3s, ", " ");
//                    }
//                    printf(" , ");
//                }
//                printf("\n");
//            }
//        }
//
//    }
//    printf("\n");
//
//
//    printf("mux_L2 table\n");
//    printf("        , ");
//            
//    m_num = max(1, pixel_per_cycle/mux_width);  
//    for(m_idx=0; m_idx<m_num; m_idx++){
//        for(i=0; i<mux_width; i++){
//            printf("%3d, %3d, ", i, i);
//        }
//        printf(" , ");
//    }
//    printf("\n");
//    for(mode_idx=0, mode=2; mode<=34; mode_idx++, mode++){
//        iter_size   = static_cast<int>(ceil(block_size*1.0/pixel_per_cycle)*block_size);
//        seg_num     = iter_size/block_size;
//
//        for(seg_idx=0; seg_idx<seg_num; seg_idx++){
//            for(y=0; y < block_size; y++){
//                printf("mode[%2d], ", mode);
//            
//                m_num = max(1, pixel_per_cycle/mux_width);                  
//                for(m_idx=0; m_idx < m_num; m_idx++){
//                    for(x=0; x < mux_width; x++){
//                        printf("%3d, %3d, ", ref_smp_muxes[mode].p1_tbl[seg_idx][m_idx][y][x], ref_smp_muxes[mode].p2_tbl[seg_idx][m_idx][y][x] );
//                    }
//                    printf(" , ");
//                }
//                printf("\n");
//            }
//        }
//    }
//    printf("\n");
//
//    printf("f table\n");
//    printf("        , ");
//    for(m_idx=0; m_idx<pixel_per_cycle/mux_width; m_idx++){
//        for(i=0; i<mux_width; i++){
//            printf("%3d, %3d, ", i, i);
//        }
//        printf(" , ");
//    }
//    printf("\n");
//    for(mode_idx=0, mode=2; mode<=34; mode_idx++, mode++){
//        iter_size   = static_cast<int>(ceil(block_size*1.0/pixel_per_cycle)*block_size);
//        seg_num     = iter_size/block_size;
//
//        for(seg_idx=0; seg_idx<seg_num; seg_idx++){
//
//            for(y=0; y<block_size; y++){
//                printf("mode[%2d], ", mode);
//            
//                m_num = max(1, pixel_per_cycle/mux_width);                
//                for(m_idx=0; m_idx<m_num; m_idx++){
//                    for(x=0; x<mux_width; x++){
//                        printf("%3d, %3d, ", 32-ref_smp_muxes[mode].f_tbl[seg_idx][m_idx][y][x], ref_smp_muxes[mode].f_tbl[seg_idx][m_idx][y][x]);
//                    }
//                    printf(" , ");
//                }             
//                printf("\n"); 
//            }                 
//        }                     
//    }                        
//    printf("\n");
////}}}
//}

void iapu_ref_mux_generator::generate(const int block_size=-1, const int pixel_per_cycle=-1, const int buf_align=-1){
//{{{
    assert(block_size>0);
    assert(pixel_per_cycle>0);
    assert(buf_align>0);
    for(int mode=2; mode<=34; mode++){
        iapu_ref_smp_muxes[mode].restruct(block_size, pixel_per_cycle);
        mux_module_gen(mode, block_size, pixel_per_cycle, buf_align, iapu_ref_smp_muxes[mode]);
    }

    return;
//}}}    
}





/********/
iapu_ref_mux_generator  iapu_i8_mux_gen;
iapu_ref_mux_generator  iapu_i4_mux_gen;
iapu_ref_mux_generator  iapu_i16_mux_gen;

