
#ifndef __CVI_ALGO_CFG__
#define __CVI_ALGO_CFG__

#include <iostream>
#include <fstream>

using namespace std;

#define CVI_ALGO_CFG

#ifdef CVI_ALGO_CFG
  #define CVI_SEPERATE_MREGE_SKIP
  #define CVI_LC_RD_COST
  #define CVI_DISABLE_INTRA_CBF
  #define CVI_REDUCE_INTRA_MODE
  #define CVI_INTRA_PRED
  #define CVI_DISABLE_INTER_ZERO_RES_CHECK
  #define CVI_ENABLE_RDO_BIT_EST
  #define CVI_ENABLE_RDO_FAST_DISTORTION
  #define CVI_FIX_POINT_RD_COST
  #define CVI_STORE_IMV
  #define CVI_CACHE_MODEL
  #define CVI_FAST_CU_ENC
  #define CVI_FAST_CU_ENC_BY_COST
  #define CVI_RC_USE_REAL_BIT
  #define CVI_STREAMING_RC
  #define CVI_HW_RC
  #define CVI_QP_MAP
  #define CVI_MOTION_DETECT
//  #define CVI_FAST_16X4_AMVP_SAD
  #define CVI_CU_PENALTY
  #define CVI_LONG_TERM
  #define CVI_SAO_NEW_SETTING
  //#define CVI_FASTRDO_NEW_SETTING
  #define CVI_VBC_LOSSY
  #define CVI_SMART_ENC
  #define CVI_FAST_B_FRAME
#endif

#define CVI_MAX_CU_DEPTH 3

#ifdef CVI_FIX_POINT_RD_COST
  #define RD_COST_FRAC_BIT 2
  #define LAMBDA_FRAC_BIT 2
  #define LC_LAMBDA_FRAC_BIT 7
  #define MOTION_LAMBDA_FRAC_BIT 7
  #define CHROMA_WEIGHT_FRAC_BIT 5
  #define MAX_SSE_BD 26
  #define MAX_SSE_VAL ((1<<MAX_SSE_BD)-1)
  #define BIT_EST_FRAC_BIT      11
  #define I4_NUM_BIT_FRAC_BIT   2
#endif

#ifdef CVI_CU_PENALTY
  #define CVI_SPLIT_COST_WEIGHT
  #define FOREGROUND_FRAC_BIT   6
  #define MADI_GAIN_FRAC_BIT    4
#endif

#ifdef CVI_MOTION_DETECT
  //#define CVI_FOREGROUND_QP
#endif

#ifdef CVI_LONG_TERM
  #define CVI_MAX_LONG_TERM_PIC 1
#endif

#define CVI_LOW_LUX_CONTROL 1 ///< When enabled, use low lux control condition
#define LOW_LUX_CONTROL_STRONG      7
#define LOW_LUX_CONTROL_MEDIUM      8
#define LOW_LUX_STRONG_THRESHOLD    0
#define LOW_LUX_MEDIUM_THRESHOLD    800

#define MAX_LOW_LUX_WIN 5
#define LOW_LUX_DEFAULT 1000

#define MINIMUM_LOW_LUX_VALUE -500
#define MAXIMUX_LOW_LUX_VALUE 1500

#define CVI_RANDOM_ENCODE

#define USE_JND_MODEL
// #define HW_JND

typedef enum{
 CVI_RD_COST = 0,
 CVI_LC_COST = 1,
 CVI_LC_SAD_COST = 2,
} CVI_CostType;

typedef struct _cost_penalty_cfg_st {
  // CU pred cost penalty
  bool EnableCuCost;
  double  Inter32Cost;
  double  Inter16Cost;
  double  Inter8Cost;
  double  Intra32Cost;
  double  Intra16Cost;
  double  Intra8Cost;
  int  Inter32CostBias;
  int  Inter16CostBias;
  int  Inter8CostBias;
  int  Intra32CostBias;
  int  Intra16CostBias;
  int  Intra8CostBias;
  // Skip cost penalty
  double  Skip32Cost;
  double  Skip16Cost;
  double  Skip8Cost;
  int  Skip32CostBias;
  int  Skip16CostBias;
  int  Skip8CostBias;
  // Foreground Detection
  bool EnableForeground;
  bool EnableForegroundTc;
  bool EnableForegroundDQp;
  bool EnableForegroundHardTh;
  double ForegroundThGain;
  int ForegroundThBias;
  int ForegroundThOffset;
  int ForegroundDeltaQp;
  int BackgroundDeltaQp;
  double ForegroundSkipCost;
  double BackgroundSkipCost;
  double ForegroundIntraCost;
  double BackgroundIntraCost;
  double ForegroundInterCost;
  double BackgroundInterCost;
} cost_penalty_cfg_st;

typedef struct _algo_cfg_st {
  //cfg for cvi encoder
  bool DisableCU64;
  bool DisableCU32;
  bool DisableIntra32;
  bool DisableMerge32;
  bool DisableSkip32;
  bool DisableAmvp32;
  bool DisableAmvpNon2Nx2N;
  bool DisableIntra4;
  bool DisablePfrmIntra;
  bool DisablePfrmIntra4;
  bool EnableLCRdCostSATDIntra;
  bool EnableLCRdCostSATDInter;
  bool EnableLCRdCostInter;
  bool ForceChromaHad4x4;
  cost_penalty_cfg_st CostPenaltyCfg;
  bool EnableCviIntraPred;
  bool EnableIntraRmdFakeNb;
  bool EnableIntraRmdSAD;
  bool EnableIapI4ReduceMode;
  bool DisableInterZeroResCheck;
  int  RDOResiEstMd;
  int  RDOSyntaxEstMd;
  int  EnableRDOFastDistortion;
  bool EnableFixPointRDCost;
  bool EnableUseIMEMV;
  bool EnableIME_SR;
  int Enable2STEP_IME;
  int EnableScaleDown_FPS;
  int EnableCACHE_MODEL;
  int EnableVBC;
  int EnableVBD;
  int VBCVersion;
  int EnableIME_LCSAD;
  bool EnableLC_INTERPOLATION;
  char m_CacheCfgFileName[512];
  bool EnableFastCUEnc;
  double MaxCUTermRatio;
  double I4TermRatio;
  bool EnableFastCUEncByCost;
  bool EnableFastEncMonitor;
  int FastEncTimeSaving;
  int FastEncRatioCu32;
  int FastEncRatioCu16;
  int EnableFastIMEMVP;
  int ForceChromaWeight;
  bool EnableDebreathe;
  bool EnableDeNoise;
  int EnableSmartEncAI;
  bool AISmoothMap;
  bool EnableSmartEncISP;
  bool EnableCalMapData;
  bool EnableSmartEncISPSkipMap;
  int EnableRGBConvert;
  int RGBFormat;
  int EnableRotateAngle;
  int EnableMirror;
  int EnableDeBreath;
  bool enableCviRGBDebugMode;
  double JNDweight;
  bool EnableJND;
} algo_cfg_st;

extern algo_cfg_st g_algo_cfg;

bool isEnableLC_RdCostSATD(bool isinter);
bool isEnableLC_RdCostInter();
bool isForceChromaHad4x4();
bool isDisableInterZeroResCheck();
int  getRDOBitEstMode();
int  getRDOSyntaxEstMode();
int  getRDOFastDistortion();
bool isEnableFixPointRDCost();
bool isEnableUseIMEMV();
bool isEnableIME_SR();
bool isEnableCACHE_MODEL();
bool isEnableCACHE_MODEL_FME();
bool isEnableCACHE_MODEL_MC();
int isEnableVBC();
int isEnable2STEP_IME();
int isEnableScaleDown_FPS();
bool isEnableIME_LCSAD();
bool isEnableLC_INTERPOLATION();
bool isEnableFastCUEnc();
bool isEnableFastCUEncByCost();
bool isEnableFastEncMonitor();
int isEanbleFastIMEMVP();
bool isEnableTemporalFilter(bool isinter);
bool isEnableSmartEncode();
int isEnableVBD();
int isEnableRGBConvert();
int getRotateAngleMode();

static __inline double fixed_to_float(unsigned int input, bool is_sqrt)
{
  unsigned int lambda_frac_bit = is_sqrt ? LC_LAMBDA_FRAC_BIT : LAMBDA_FRAC_BIT;
  return ((double)input / (double)(1 << lambda_frac_bit));
}

static __inline unsigned int float_to_fixed(double input, bool is_sqrt)
{
  unsigned int lambda_frac_bit = is_sqrt ? LC_LAMBDA_FRAC_BIT : LAMBDA_FRAC_BIT;
  return (unsigned int)(input*((unsigned int)1 << lambda_frac_bit));
}

unsigned int cvi_get_leading_one_pos(unsigned int val);

static __inline unsigned int cvi_mem_align(unsigned int val, unsigned int align)
{
  return (val + align - 1) / align * align;
}

static __inline uint64_t cvi_mem_align64(uint64_t val, uint64_t align)
{
  return (val + align - 1) / align * align;
}

static __inline unsigned int cvi_mem_align_mult(unsigned int val, unsigned int align)
{
  return (val + align - 1) / align;
}

static __inline uint64_t cvi_mem_align_mult64(uint64_t val, uint64_t align)
{
  return (val + align - 1) / align;
}


#endif /* __CVI_ALGO_CFG__ */
