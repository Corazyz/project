#ifndef __CVI_INTRA__
#define __CVI_INTRA__

#include "CommonDef.h"

#define CVI_CHROMA_CAND_SIZE    9   // Planar, DC, Ver, Hor, 34, Luma[4]

int cviEdgeDetect(short *p_src_buf, int src_stride, int x, int y, int width, int height);
bool cviSkipRmdMode(int pu_size, int mode, int edge_index);
bool cviSkipIapMode(int mode, unsigned int rmd_modes[35], int rmd_number);
void cviFillIntraMode(int pu_size, unsigned int select_mode[35], int &mode_number, int edge_index,
                      int mpm_number, int mpm_modes[3]);

bool isChromaBasic(UInt mode);
void cviSetChromaCandidate(UInt *p_mode, UInt mode_number);
void cviGetChromaCandidate(UInt *p_mode, UInt *p_number);
#endif