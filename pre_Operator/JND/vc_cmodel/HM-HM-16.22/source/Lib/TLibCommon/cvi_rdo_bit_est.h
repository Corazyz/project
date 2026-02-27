
#ifndef __CVI_RDO_BIT_EST__
#define __CVI_RDO_BIT_EST__

#include "cvi_sigdump.h"
#include "cvi_algo_cfg.h"

using namespace std;

#define LMS_LR_FRAC_BD      16
#define LMS_MAX_ERR_BD      21
#define RESI_MDL_FRAC_BD    15
#define SCALE_MDL_DELAY     1   //  Model delay CU32 number
#define SCALE_MDL_BUF_SIZE  (SCALE_MDL_DELAY + 1)

#define DUMP_BIT_STATS 0

enum RdoBitEstMode
{
  HM_ORIGINAL = 0,
  BIN_BASE = 1,
  CTX_ADAPTIVE = 2
};

enum ResiModelState
{
  RESI_MODEL_TEST = 0,
  RESI_MODEL_TRAIN = 1,
  RESI_MODEL_STATE_NUM = 2
};

const extern unsigned int g_ctxSubSplShift;
const extern unsigned int g_stateEntropy[128];
extern int g_estBitFracBd;
extern int g_estBitPrec;
extern unsigned int g_hwEntropyBits[64][2];

#ifdef CVI_RANDOM_ENCODE
extern bool g_is_cvi_rand_rdo_en;
extern bool g_is_cvi_rand_qp_map;
extern unsigned int g_hwEntropyBitsRand[64][2];
#endif

extern const double g_significantScale_init[2][2][4][2];
extern double g_lastPosBinScale_init[2][2][4];
extern const double g_significantLms_lr[2][4][2]; // [ch][td][bin]

extern double g_significantBias_clip;
extern double g_significantScale_min;
extern double g_significantScale_max;
extern double g_significantScale_LR[2];

extern int g_significantScale[SCALE_MDL_BUF_SIZE][2][2][4][2];  // [Delay][ch][md][td][bin]
extern int g_significantBias[SCALE_MDL_BUF_SIZE][2][2][4][2];   // [Delay][ch][md][td][bin]
extern int g_significantScale_snapshot[2][2][4][2]; // [ch][md][td][bin]
extern int g_significantBias_snapshot[2][2][4][2];
extern unsigned int g_uiResiModelState;

extern unsigned int g_scale_D;
extern unsigned int g_cuSplitFlagScale[SCALE_MDL_BUF_SIZE][2]; // [T][bit]
extern unsigned int g_skipFlagScale[SCALE_MDL_BUF_SIZE][2];
extern unsigned int g_cuPredModeScale[SCALE_MDL_BUF_SIZE][2];
extern unsigned int g_cuMergeIdxScale[SCALE_MDL_BUF_SIZE][2];
extern unsigned int g_cuMergeFlagScale[SCALE_MDL_BUF_SIZE][2];
extern unsigned int g_cuPartFlagScale[SCALE_MDL_BUF_SIZE][2];
extern unsigned int g_mvpIdxScale[SCALE_MDL_BUF_SIZE][2];
extern unsigned int g_mvdScale[SCALE_MDL_BUF_SIZE][2];
extern unsigned int g_refIdxScale[SCALE_MDL_BUF_SIZE][2];
extern unsigned int g_intraPredScale[SCALE_MDL_BUF_SIZE][2];
extern unsigned int g_chmaIntraPredScale[SCALE_MDL_BUF_SIZE][2];
extern unsigned int g_cbfScale[SCALE_MDL_BUF_SIZE][2][5][2]; // [2][NUM_QT_CBF_CTX_SETS][NUM_QT_CBF_CTX_PER_SET][2]
#ifdef SIGDUMP
extern unsigned int g_cuPartFlagIsLps[SCALE_MDL_BUF_SIZE][2]; // for sigdump
extern unsigned int g_cuPartFlagState[SCALE_MDL_BUF_SIZE][2]; // for sigdump
extern unsigned int g_cbfisLps[SCALE_MDL_BUF_SIZE][2][5][2];  // for sigdump
extern unsigned int g_cbfState[SCALE_MDL_BUF_SIZE][2][5];     // for sigdump
#endif
extern unsigned int g_rootCbfScale[SCALE_MDL_BUF_SIZE][2];

extern unsigned int g_syntaxType;

void setupBitEstEnv();
void setupResiModel(int poc);
void setResiModelState(unsigned int state);
void delayResiModel();
void snapshotResiModel();

void linear_lms(int x, int y, int m, int c, int lr[],
                int *o_m, int *o_c, int paraBd, int yBd, int lrBd);
#endif /* __CVI_RDO_BIT_EST__ */
