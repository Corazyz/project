#ifndef __CVI_IAPU_REF_SMP_MUX_H__
#define __CVI_IAPU_REF_SMP_MUX_H__

#include<vector>
#include<cassert>

using namespace::std;


#include<cstdio>
#include<cstdint>
#include<vector>
#include<algorithm>
#include<cmath>
#include<cassert>
#include<iostream>

using namespace::std;


typedef struct{
    int     seg_idx;
    int     m_idx;
    int     sta_idx;
    int*    vref;
} l2_mux_info;

class iapu_mux_one_mode {
//{{{
    const int mux_width         = 4;
    const int statistics_cycle  = 4;
    const int pivot             = 18;


    public:

    int ang_mode;
    int block_size;
    int pixel_per_cycle;
    bool is_restructed;
 
    vector<vector<vector<vector<int> > > >  l1_tbl;
    vector<vector<vector<vector<int> > > >  p1_tbl, p2_tbl, f_tbl;
    vector<vector<l2_mux_info> >            p1_info, p2_info, f_info;

    iapu_mux_one_mode(const int mode);    
    ~iapu_mux_one_mode();    
    void restruct(const int block_size, const int pixel_per_cycle);
    int find_ref_smp_i(const int x, const int y);     // take only +1
    int find_ref_smp_f(const int x, const int y);
//}}}
};

class iapu_ref_mux_generator{
           /*0  1   2   3   4   5   6  7  8  9 10     11     12    13   14   15   16   17   18   19   20   21   22   23    24    25 26 27 28 29  30  31  32  33  34*/
                   /************************ horizontal ********************************/   /**************************** vertical *********************************/
    int A[35] = {0, 0, 32, 26, 21, 17, 13, 9, 5, 2, 0,    -2,    -5,   -9, -13, -17, -21, -26, -32, -26, -21, -17, -13,  -9,   -5,   -2, 0, 2, 5, 9, 13, 17, 21, 26, 32};
    int B[35] = {0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, -4096, -1638, -910,-630,-482,-390,-315,-256,-315,-390,-482,-630,-910,-1638,-4096, 0, 0, 0, 0,  0,  0,  0,  0,  0};
    
    const int mux_width         = 4;
    const int statistics_cycle  = 4;
    const int pivot_mode        = 18;


    // get i
    int get_i(const int ang_mode, const int x, const int y);
    // get f
    int get_f(const int ang_mode, const int x, const int y);
    
    
    void mux_unit_gen(const int ang_mode, const int seg_idx, const int m_idx, const int xinit, const int yinit, const int block_size, const int pixel_per_cycle, const int buf_align, vector<vector<int> >& l1_tbl, vector<vector<int> >& p1_tbl, vector<vector<int> >& p2_tbl, vector<vector<int> >& f_tbl, vector<vector<l2_mux_info> >& p1_info, vector<vector<l2_mux_info> >& p2_info, vector<vector<l2_mux_info> >& f_info);
    
    void mux_module_gen(const int ang_mode, const int block_size, const int pixel_per_cycle, const int buf_align, iapu_mux_one_mode& ref_smp_mux);

    public:

    iapu_mux_one_mode iapu_ref_smp_muxes[34+1];     // 0, 1 dummy

    
    iapu_ref_mux_generator():iapu_ref_smp_muxes{   // 0, 1 dummy
                             {0},
                             {1},  {2},  {3},  {4},  {5},  {6},  {7},  {8},  {9}, {10}, 
                            {11}, {12}, {13}, {14}, {15}, {16}, {17}, {18}, {19}, {20}, 
                            {21}, {22}, {23}, {24}, {25}, {26}, {27}, {28}, {29}, {30}, 
                            {31}, {32}, {33}, {34} 
                        }
    {
        return;
    }


    void generate(const int block_size, const int pixel_per_cycle, const int buf_align);
    
//    void print_table(const int block_size, const int pixel_per_cycle, const int max_l1, iapu_mux_one_mode ref_smp_muxes[]);
    
};



/********/
extern iapu_ref_mux_generator  iapu_i8_mux_gen;
extern iapu_ref_mux_generator  iapu_i4_mux_gen;
extern iapu_ref_mux_generator  iapu_i16_mux_gen;


#endif
