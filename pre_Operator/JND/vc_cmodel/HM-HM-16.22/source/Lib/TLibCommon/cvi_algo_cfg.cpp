
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "cvi_algo_cfg.h"
#include "cvi_cu_ctrl.h"
#include "CommonDef.h"

using namespace std;

algo_cfg_st g_algo_cfg;

unsigned int cvi_get_leading_one_pos(unsigned int val)
{
    unsigned int count = 0;
    while (val)
    {
        count++;
        val >>= 1;
    }
    return count;
}

bool isEnableUseIMEMV()
{
    return g_algo_cfg.EnableUseIMEMV;
}

bool isEnableLC_RdCostSATD(bool isinter)
{
    if (isinter)
        return g_algo_cfg.EnableLCRdCostSATDInter;
    else
        return g_algo_cfg.EnableLCRdCostSATDIntra;
}

bool isEnableLC_RdCostInter()
{
    return g_algo_cfg.EnableLCRdCostInter;
}

bool isForceChromaHad4x4()
{
  return g_algo_cfg.ForceChromaHad4x4;
}

bool isDisableInterZeroResCheck()
{
    return g_algo_cfg.DisableInterZeroResCheck;
}

int getRDOBitEstMode()
{
    return g_algo_cfg.RDOResiEstMd;
}

int getRDOSyntaxEstMode()
{
    return g_algo_cfg.RDOSyntaxEstMd;
}

int getRDOFastDistortion()
{
    return g_algo_cfg.EnableRDOFastDistortion;
}

bool isEnableFixPointRDCost()
{
    return g_algo_cfg.EnableFixPointRDCost;
}

bool isEnableIME_SR()
{
    return g_algo_cfg.EnableIME_SR;
}

bool isEnableCACHE_MODEL()
{
    return (g_algo_cfg.EnableCACHE_MODEL != 0);
}

bool isEnableCACHE_MODEL_FME()
{
    return (g_algo_cfg.EnableCACHE_MODEL == 2 || g_algo_cfg.EnableCACHE_MODEL == 3);
}

bool isEnableCACHE_MODEL_MC()
{
    return (g_algo_cfg.EnableCACHE_MODEL == 3);
}

int isEnableVBC()
{
    return g_algo_cfg.EnableVBC;
}

int isEnableVBD()
{
    return g_algo_cfg.EnableVBD;
}

int isEnableRGBConvert()
{
    return g_algo_cfg.EnableRGBConvert;
}

int getRotateAngleMode()
{
    if (g_algo_cfg.EnableRotateAngle == 90)
        return 3;
    else if (g_algo_cfg.EnableRotateAngle == 180)
        return 1;
    else if (g_algo_cfg.EnableRotateAngle == 270)
        return 2;
    else
        return 0;
}

int isEnable2STEP_IME()
{
    return g_algo_cfg.Enable2STEP_IME;
}

int isEnableScaleDown_FPS()
{
    return g_algo_cfg.EnableScaleDown_FPS;
}

bool isEnableIME_LCSAD()
{
    return g_algo_cfg.EnableIME_LCSAD;
}

bool isEnableLC_INTERPOLATION()
{
    return g_algo_cfg.EnableLC_INTERPOLATION;
}

bool isEnableFastCUEnc()
{
    return g_algo_cfg.EnableFastCUEnc;
}

bool isEnableFastCUEncByCost()
{
    return g_algo_cfg.EnableFastCUEncByCost;
}

bool isEnableFastEncMonitor()
{
    return g_algo_cfg.EnableFastEncMonitor;
}

int isEanbleFastIMEMVP()
{
    return g_algo_cfg.EnableFastIMEMVP;
}

bool isEnableTemporalFilter(bool isIFrame)
{
    return (g_algo_cfg.EnableDeNoise ||
        (g_algo_cfg.EnableDebreathe && isIFrame));
}

bool isEnableSmartEncode()
{
    return (g_algo_cfg.EnableSmartEncAI || g_algo_cfg.EnableSmartEncISP || g_algo_cfg.EnableJND);
}
