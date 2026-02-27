
#include <stdio.h>
#include <cstring>
#include "cvi_rdo_bit_est.h"

const double g_significantScale_init[2][2][4][2] = {
  {
    {{-1, -1}, {0.25626, 2.5}, {0.1618, 2.838}, {-1, -1}}, // [luma][INTER]
    {{0.7186, 1.66}, {0.4794, 1.84}, {0.4919, 1.87}, {-1, -1}}, // [luma][INTRA]
  },
  {
    {{0.2581, 1.645}, {0.207, 2.33}, {-1, -1}, {-1, -1}}, // [chroma][INTER]
    {{0.39, 1.605}, {0.34, 2.302}, {-1, -1}, {-1, -1}}, // [chroma][INTRA]
  }
};

double g_lastPosBinScale_init[2][2][4] = {
  {{-1, 12.2256, 11.85, -1}, // [luma][INTER]
   {13.7167, 12.1383, 9.502, -1}}, // [luma][INTRA]
  {{15.1, 16, -1, -1}, // [chroma][INTER]
   {13.2, 15.6, -1, -1}} // [chroma][INTRA]
};

const double g_significantLms_lr[2][4][2] = {
  {{0.04, 0.05}, {0.003, 0.01}, {0.0004, 0.001}, {-1, -1}}, // [luma][bin]
  {{0.05, 0.1}, {0.006, 0.08}, {-1, -1}, {-1, -1}} // [chroma][bin]
};

const unsigned int g_ctxSubSplShift = 2;
const unsigned int g_stateEntropy[128] =
{
  0x08000, 0x08000, 0x076da, 0x089a0, 0x06e92, 0x09340, 0x0670a, 0x09cdf, 0x06029, 0x0a67f, 0x059dd, 0x0b01f, 0x05413, 0x0b9bf, 0x04ebf, 0x0c35f,
  0x049d3, 0x0ccff, 0x04546, 0x0d69e, 0x0410d, 0x0e03e, 0x03d22, 0x0e9de, 0x0397d, 0x0f37e, 0x03619, 0x0fd1e, 0x032ee, 0x106be, 0x02ffa, 0x1105d,
  0x02d37, 0x119fd, 0x02aa2, 0x1239d, 0x02836, 0x12d3d, 0x025f2, 0x136dd, 0x023d1, 0x1407c, 0x021d2, 0x14a1c, 0x01ff2, 0x153bc, 0x01e2f, 0x15d5c,
  0x01c87, 0x166fc, 0x01af7, 0x1709b, 0x0197f, 0x17a3b, 0x0181d, 0x183db, 0x016d0, 0x18d7b, 0x01595, 0x1971b, 0x0146c, 0x1a0bb, 0x01354, 0x1aa5a,
  0x0124c, 0x1b3fa, 0x01153, 0x1bd9a, 0x01067, 0x1c73a, 0x00f89, 0x1d0da, 0x00eb7, 0x1da79, 0x00df0, 0x1e419, 0x00d34, 0x1edb9, 0x00c82, 0x1f759,
  0x00bda, 0x200f9, 0x00b3c, 0x20a99, 0x00aa5, 0x21438, 0x00a17, 0x21dd8, 0x00990, 0x22778, 0x00911, 0x23118, 0x00898, 0x23ab8, 0x00826, 0x24458,
  0x007ba, 0x24df7, 0x00753, 0x25797, 0x006f2, 0x26137, 0x00696, 0x26ad7, 0x0063f, 0x27477, 0x005ed, 0x27e17, 0x0059f, 0x287b6, 0x00554, 0x29156,
  0x0050e, 0x29af6, 0x004cc, 0x2a497, 0x0048d, 0x2ae35, 0x00451, 0x2b7d6, 0x00418, 0x2c176, 0x003e2, 0x2cb15, 0x003af, 0x2d4b5, 0x0037f, 0x2de55
};

int g_estBitFracBd;
int g_estBitPrec;
unsigned int g_hwEntropyBits[64][2];

#ifdef CVI_RANDOM_ENCODE
bool g_is_cvi_rand_rdo_en = false;
bool g_is_cvi_rand_qp_map = false;
unsigned int g_hwEntropyBitsRand[64][2] = { 0 };
#endif

double g_significantBias_clip = 2.0;
double g_significantScale_min = 0.2731;
double g_significantScale_max = 3.97;
#ifdef CVI_FASTRDO_NEW_SETTING
  double g_significantScale_LR[2] = {0.0002, 0.0001};
#else
  double g_significantScale_LR[2] = {0.00005, 0.0001};
#endif

int g_significantScale[SCALE_MDL_BUF_SIZE][2][2][4][2]; // [Delay][ch][md][td][bin]
int g_significantBias[SCALE_MDL_BUF_SIZE][2][2][4][2];  // [Delay][ch][md][td][bin]
int g_significantScale_snapshot[2][2][4][2];            // [ch][md][td][bin]
int g_significantBias_snapshot[2][2][4][2];

unsigned int g_uiResiModelState;

unsigned int g_scale_D;
unsigned int g_cuSplitFlagScale[SCALE_MDL_BUF_SIZE][2]; // [T][bit]
unsigned int g_skipFlagScale[SCALE_MDL_BUF_SIZE][2];
unsigned int g_cuPredModeScale[SCALE_MDL_BUF_SIZE][2];
unsigned int g_cuMergeIdxScale[SCALE_MDL_BUF_SIZE][2];
unsigned int g_cuMergeFlagScale[SCALE_MDL_BUF_SIZE][2];
unsigned int g_cuPartFlagScale[SCALE_MDL_BUF_SIZE][2];
unsigned int g_mvpIdxScale[SCALE_MDL_BUF_SIZE][2];
unsigned int g_mvdScale[SCALE_MDL_BUF_SIZE][2];
unsigned int g_refIdxScale[SCALE_MDL_BUF_SIZE][2];
unsigned int g_intraPredScale[SCALE_MDL_BUF_SIZE][2];
unsigned int g_chmaIntraPredScale[SCALE_MDL_BUF_SIZE][2];
unsigned int g_cbfScale[SCALE_MDL_BUF_SIZE][2][5][2];
#ifdef SIGDUMP
unsigned int g_cuPartFlagIsLps[SCALE_MDL_BUF_SIZE][2];  // for sigdump
unsigned int g_cuPartFlagState[SCALE_MDL_BUF_SIZE][2];  // for sigdump
unsigned int g_cbfisLps[SCALE_MDL_BUF_SIZE][2][5][2];   // for sigdump
unsigned int g_cbfState[SCALE_MDL_BUF_SIZE][2][5];      // for sigdump
#endif
unsigned int g_rootCbfScale[SCALE_MDL_BUF_SIZE][2];

unsigned int g_syntaxType; // 0: syntax 1: residual

void setResiModelState(unsigned int state) {g_uiResiModelState = state;};

void linear_lms(int x, int y, int m, int c, int lr[],
                int *o_m, int *o_c, int paraBd, int yBd, int lrBd)
{
  int err = (y<<(paraBd-yBd)) - m * x - c;
  err = Clip3(-(1<<LMS_MAX_ERR_BD), (1<<LMS_MAX_ERR_BD)-1, err);
#ifdef CVI_FASTRDO_NEW_SETTING
  *o_m = m + (((UInt64)(lr[0])*err)>>lrBd);
#else
  *o_m = m + (((UInt64)(lr[0])*err*x)>>lrBd);
#endif
  *o_c = c + (((UInt64)(lr[1])*err)>>lrBd);
}

void setupBitEstEnv()
{
  g_estBitFracBd = (getRDOBitEstMode()==HM_ORIGINAL || getRDOSyntaxEstMode()==HM_ORIGINAL)
                 ? 15
                 : 11;
  g_estBitPrec = 1 << g_estBitFracBd;
  // setup bit entropy LUT
  int hwEntryNum = 64>>g_ctxSubSplShift;
  int avgNum = 1<<g_ctxSubSplShift;
  int fracShift = 15 - g_estBitFracBd;
  memset(g_hwEntropyBits, 0, sizeof(g_hwEntropyBits));
  for(int s=0; s<hwEntryNum; s++)
  {
    for(int idx=0; idx<avgNum; idx++)
    {
      int stateIdx = ((s<<g_ctxSubSplShift) + idx)<<1;
      g_hwEntropyBits[s][0] += g_stateEntropy[stateIdx];
      g_hwEntropyBits[s][1] += g_stateEntropy[stateIdx + 1];
    }
    g_hwEntropyBits[s][0] >>= (g_ctxSubSplShift+fracShift);
    g_hwEntropyBits[s][1] >>= (g_ctxSubSplShift+fracShift);
  }
#ifdef CVI_RANDOM_ENCODE
  if (g_is_cvi_rand_rdo_en)
  {
    memcpy(g_hwEntropyBits, g_hwEntropyBitsRand, sizeof(g_hwEntropyBits));
  }
#endif
}

void delayResiModel()
{
  for (int scale_d = g_scale_D; scale_d > 0; scale_d--)
  {
    memcpy(g_significantScale[scale_d], g_significantScale[scale_d - 1], sizeof(g_significantScale[0]));
    memcpy(g_significantBias[scale_d], g_significantBias[scale_d - 1], sizeof(g_significantScale[0]));
  }
}

void snapshotResiModel()
{
  memcpy(g_significantScale_snapshot, g_significantScale[0], sizeof(g_significantScale_snapshot));
  memcpy(g_significantBias_snapshot, g_significantBias[0], sizeof(g_significantBias_snapshot));
}

void setupResiModel(int poc)
{
  g_scale_D = SCALE_MDL_DELAY; // delay 1 CU32 to sync HW

  if(poc==0)
  {
    memset(g_significantBias, 0, sizeof(g_significantBias));
    for (int ch=0; ch<2; ch++) {
      for(int md=0; md<2; md++) {
        for (int ti_i=0; ti_i<4; ti_i++) {
          for (int b=0; b<2; b++) {
            g_significantScale[0][ch][md][ti_i][b] = g_significantScale_init[ch][md][ti_i][b] * (1<<RESI_MDL_FRAC_BD);
          }
        }
      }
    }

    if (g_is_cvi_rand_rdo_en)
    {
      for (int ch=0; ch<2; ch++) {
        for(int md=0; md<2; md++) {
          for (int ti_i=0; ti_i<4; ti_i++) {
            double rand_val = (double)(cvi_random_range(0, 4095) * 16) / g_estBitPrec;
            g_lastPosBinScale_init[ch][md][ti_i] = rand_val;

            for (int b=0; b<2; b++) {
              g_significantScale[0][ch][md][ti_i][b] = cvi_random_range(0, 0x1ffff);
              g_significantBias[0][ch][md][ti_i][b] = cvi_random_range(-65536, 65535);
            }
          }
        }
      }
    }
  }
  else
  {
    memcpy(g_significantScale[0], g_significantScale_snapshot, sizeof(g_significantScale_snapshot));
    memcpy(g_significantBias[0], g_significantBias_snapshot, sizeof(g_significantBias_snapshot));
  }
  for (int i = 0; i < g_scale_D; i++)
    delayResiModel();
}