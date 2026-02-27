#include "cvi_rc_kernel.h"

const int g_rcGopRemainBitWeight = 25;
const int g_rcLastPicQpClip = 3;
const int g_rcLevelPicQpClip = 3;
const int g_rcPicQpNormalClip = 10;


const RC_Float g_rcIAlphaMinValue = FLOAT_VAL_1p8;
const RC_Float g_rcIBetaMinValue = FLOAT_VAL_p6;
const RC_Float g_rcIBetaMaxValue = FLOAT_VAL_5;
const RC_Float g_rcAlphaMinValue = FLOAT_VAL_p05;
const RC_Float g_rcAlphaMaxValue = FLOAT_VAL_1000;
const RC_Float g_rcBetaMinValue = FLOAT_VAL_minus_3;
const RC_Float g_rcBetaMaxValue = FLOAT_VAL_minus_1;
const RC_Float g_rcMinPPicBpp = FLOAT_VAL_p00024;
const RC_Float g_rcALPHA_I = FLOAT_VAL_6p7542;
const RC_Float g_rcBETA_I = FLOAT_VAL_1p786;
const RC_Float g_rcALPHA_P = FLOAT_VAL_3p2003;
const RC_Float g_rcBETA_P = FLOAT_VAL_minus_1p367;
const RC_Float g_rcTcBETA = FLOAT_VAL_1p2517;
const RC_Float g_picAvgTc = FLOAT_VAL_12;

const RC_Float g_lambdaQpScale = FLOAT_VAL_4p2005;
const RC_Float g_lambdaQpBias = FLOAT_VAL_13p7122;

// ----------------------------------------------------------------

static int LambdaToQp(RC_Float lambda)
{
  return CVI_FLOAT_TO_INT(CVI_FLOAT_ADD(CVI_FLOAT_MUL(g_lambdaQpScale, CVI_FLOAT_LOG(lambda)), g_lambdaQpBias));
}

static RC_Float QpToLambda(RC_Float qp)
{
  return CVI_FLOAT_EXP(CVI_FLOAT_DIV(CVI_FLOAT_SUB(qp, g_lambdaQpBias), g_lambdaQpScale));
}

static RC_Float intraLambdaToBpp(RC_Float lambda, RC_Float alpha, RC_Float beta, RC_Float tc)
{
  RC_Float bpp_div_tc = CVI_FLOAT_POW(CVI_FLOAT_DIV(CVI_FLOAT_MUL(FLOAT_VAL_256, lambda), alpha), CVI_FLOAT_DIV(FLOAT_VAL_minus_1, beta));
  RC_Float bpp = CVI_FLOAT_MUL(tc, bpp_div_tc);
  return bpp;
}

static RC_Float lambdaToBpp(RC_Float lambda, RC_Float alpha, RC_Float beta)
{
  return CVI_FLOAT_POW(CVI_FLOAT_DIV(lambda, alpha), CVI_FLOAT_DIV(FLOAT_VAL_1, beta));
}

// ----------------------------------------------------------------

void rcModelUpdateParam_Init(stRcKernelInfo *info, RC_Float targetBpp)
{
  if (targetBpp < FLOAT_VAL_p03)
  {
    cviRcKernel_setRCModelUpdateStep(info, FLOAT_VAL_p01, FLOAT_VAL_p005);
  }
  else if (targetBpp < FLOAT_VAL_p08)
  {
    cviRcKernel_setRCModelUpdateStep(info, FLOAT_VAL_p05, FLOAT_VAL_p025);
  }
  else if (targetBpp < FLOAT_VAL_p2)
  {
    cviRcKernel_setRCModelUpdateStep(info, FLOAT_VAL_p1, FLOAT_VAL_p05);
  }
  else if (targetBpp < FLOAT_VAL_p5)
  {
    cviRcKernel_setRCModelUpdateStep(info, FLOAT_VAL_p2, FLOAT_VAL_p1);
  }
  else
  {
    cviRcKernel_setRCModelUpdateStep(info, FLOAT_VAL_p4, FLOAT_VAL_p2);
  }
}

// ----------------------------------------------------------------

void updateAlphaBetaIntra(RC_Float *alpha, RC_Float *beta, RC_Float tc, int targetBits, int encodedBits)
{
  RC_Float lnbpp = CVI_FLOAT_MAX(FLOAT_VAL_1, CVI_FLOAT_LOG(tc));
  RC_Float diffLambda = CVI_FLOAT_MUL(*beta, CVI_FLOAT_SUB(CVI_FLOAT_LOG(INT_TO_CVI_FLOAT(encodedBits)), CVI_FLOAT_LOG(INT_TO_CVI_FLOAT(targetBits))));
  diffLambda = CVI_FLOAT_CLIP(FLOAT_VAL_minus_p125, FLOAT_VAL_p25, CVI_FLOAT_MUL(FLOAT_VAL_p25, diffLambda));
  *alpha = CVI_FLOAT_CLIP(g_rcIAlphaMinValue, g_rcAlphaMaxValue, CVI_FLOAT_MUL(*alpha, CVI_FLOAT_EXP(diffLambda)));
  *beta = CVI_FLOAT_CLIP(g_rcIBetaMinValue, g_rcIBetaMaxValue, CVI_FLOAT_ADD(*beta, CVI_FLOAT_DIV(diffLambda, lnbpp)));
}

void rcGopBitAlloc(stRcKernelInfo *info, int picIdx)
{
  int picIdxInIPeriod = picIdx % info->intraPeriod;
  int smoothWinSize = MAX(info->statFrameNum - picIdxInIPeriod + 1, info->framerate);
  int picTargetBit = MAX(info->minPicBit, info->picAvgBit + (info->bitError / smoothWinSize));
  // smooth consecutive rc-gop allocated bit
  if (info->isLastPicI == 0)
  {
    picTargetBit = (info->gopPicAvgBit * 25 + picTargetBit * 75) / 100;
  }
  info->gopPicAvgBit = picTargetBit;
  int gopSize = ((picIdx / info->rcGopSize) + ((picIdx % info->rcGopSize)!=0)) * info->rcGopSize + 1 - picIdx;
  info->gopPicLeft = gopSize;
  info->gopBitLeft = picTargetBit * gopSize;
}

int getAvgPFrameQp(stRcKernelInfo *info)
{
  if(info->pPicCnt > 0) {
    return (((CVI_FLOAT_TO_INT(info->pPicQpAccum) + (info->pPicCnt>>1)) /  info->pPicCnt)) + info->ipQpDelta;
  }
  else {
    return info->lastLevelQp[0];
  }
}

int estPicBitByModel(stRcKernelInfo *info, RC_Float lambda, int isIPic)
{
  RC_Float alpha = info->rqModel[isIPic == 0].alpha;
  RC_Float beta = info->rqModel[isIPic == 0].beta;
  RC_Float bpp;
  if (isIPic)
  {
    bpp = intraLambdaToBpp(lambda, alpha, beta, info->picTextCplx);
  }
  else
  {
    bpp = lambdaToBpp(lambda, alpha, beta);
  }
  bpp = CVI_FLOAT_MIN(bpp, FLOAT_VAL_12);
  int bpp_frac = CVI_FLOAT_TO_FRAC_INT(bpp, FIX_POINT_FRAC_BIT);
  return ((long long int)bpp_frac * info->numOfPixel)>>FIX_POINT_FRAC_BIT;
}

int rcIPicBitAlloc(stRcKernelInfo *info, int qp)
{
  RC_Float lambda = QpToLambda(INT_TO_CVI_FLOAT(qp));
  int intraOrgBits = estPicBitByModel(info, lambda, 1);
  int intraBits = CLIP(info->minIPicBit, info->maxIPicBit, intraOrgBits);
  if (info->lastIPicBit > 0)
  {
    int upperBitLimit = info->lastIPicBit + info->neiIBitMargin;
    int lowerBitLimit = info->lastIPicBit - info->neiIBitMargin;
    if (intraBits > upperBitLimit)
    {
      intraBits = (2 * intraBits + 8 * upperBitLimit) / 10;
    }
    else if (intraBits < lowerBitLimit)
    {
      intraBits = (2 * intraBits + 8 * lowerBitLimit) / 10;
    }
  }
  return intraBits;
}

int estPicTargetBits(stRcKernelInfo *info)
{
  int gopRemainAvgBit = MAX(MAX(0, info->gopBitLeft) / MAX(1, info->gopPicLeft), info->minPicBit);
  int targetBits = (g_rcGopRemainBitWeight * gopRemainAvgBit + ((100 - g_rcGopRemainBitWeight) * info->gopPicAvgBit)) / 100;
  return targetBits;
}

RC_Float calculateLambdaIntra(RC_Float alpha, RC_Float beta, RC_Float picTextCplx, RC_Float bitsPerPixel)
{
  return CVI_FLOAT_MUL(FRAC_INT_TO_CVI_FLOAT((CVI_FLOAT_TO_FRAC_INT(alpha, FIX_POINT_FRAC_BIT)>>8), FIX_POINT_FRAC_BIT), \
        CVI_FLOAT_POW(CVI_FLOAT_DIV(picTextCplx, bitsPerPixel), beta));
}

RC_Float estPicLambda(stRcKernelInfo *info, int targetBits, int isIPic, int picIdx)
{
  int picLv = isIPic == 0;
  RC_Float alpha = info->rqModel[picLv].alpha;
  RC_Float beta = info->rqModel[picLv].beta;
  RC_Float bpp = FRAC_INT_TO_CVI_FLOAT(((long long int)targetBits<<FIX_POINT_FRAC_BIT) / info->numOfPixel, FIX_POINT_FRAC_BIT);
  RC_Float estLambda;
  if (isIPic)
  {
    estLambda = calculateLambdaIntra(alpha, beta, info->picTextCplx, bpp);
  }
  else
  {
    estLambda = CVI_FLOAT_MUL(alpha, CVI_FLOAT_POW(bpp, beta));
  }

  RC_Float lastLevelLambda = info->lastLevelLambda[picLv];
  RC_Float lastPicLambda = info->lastPicLambda;
  int isFirstPPic = (info->isCurPicI == 0) && (info->isLastPicI == 1) && (picIdx > info->intraPeriod);
  int levelClip = (info->isCurPicI == 1) ? (picIdx > info->intraPeriod) : (info->isLastPicI == 0);
  if (CVI_FLOAT_GT(lastLevelLambda, FLOAT_VAL_0) && levelClip == 1)
  {
    RC_Float low = CVI_FLOAT_MUL(lastLevelLambda, info->lastLevelLambdaScaleLow);
    RC_Float high = CVI_FLOAT_MUL(lastLevelLambda, info->lastLevelLambdaScaleHigh);
    estLambda = CVI_FLOAT_CLIP(low, high, estLambda);
  }
  if (CVI_FLOAT_GT(lastPicLambda, FLOAT_VAL_0) && isFirstPPic == 1)
  {
    RC_Float low = CVI_FLOAT_MUL(lastPicLambda, info->lastPicLambdaScaleLow);
    RC_Float high = CVI_FLOAT_MUL(lastPicLambda, info->lastNormalLambdaScaleHigh);
    estLambda = CVI_FLOAT_CLIP(low, high, estLambda);
  }
  else if (CVI_FLOAT_GT(lastPicLambda, FLOAT_VAL_0) && isIPic == 0)
  {
    RC_Float low = CVI_FLOAT_MUL(lastPicLambda, info->lastPicLambdaScaleLow);
    RC_Float high = CVI_FLOAT_MUL(lastPicLambda, info->lastPicLambdaScaleHigh);
    estLambda = CVI_FLOAT_CLIP(low, high, estLambda);
  }
  else if (CVI_FLOAT_GT(lastPicLambda, FLOAT_VAL_0))
  {
    RC_Float low = CVI_FLOAT_MUL(lastPicLambda, info->lastNormalLambdaScaleLow);
    RC_Float high = CVI_FLOAT_MUL(lastPicLambda, info->lastNormalLambdaScaleHigh);
    estLambda = CVI_FLOAT_CLIP(low, high, estLambda);
  }
  RC_Float maxLambda = (info->isCurPicI) ? info->maxILambda : info->maxLambda;
  RC_Float minLambda = (info->isCurPicI) ? info->minILambda : info->minLambda;
  estLambda = CVI_FLOAT_CLIP(minLambda, maxLambda, estLambda);
  return estLambda;
}

int estPicQP(stRcKernelInfo *info, RC_Float lambda, int isIPic, int picIdx)
{
  int qp = LambdaToQp(lambda);
  int picLv = isIPic == 0;
  int lastLevelQP = info->lastLevelQp[picLv];
  int lastPicQP = info->lastPicQp;
  int isFirstPPic = (info->isCurPicI == 0) && (info->isLastPicI == 1) && (picIdx > info->intraPeriod);
  int levelClip = (info->isCurPicI == 1) ? (picIdx > info->intraPeriod) : (info->isLastPicI == 0);
  if (lastLevelQP > 0 && levelClip == 1)
  {
    qp = CLIP(lastLevelQP - info->levelPicQpClip, lastLevelQP + info->levelPicQpClip, qp);
  }
  if (lastPicQP > 0 && isFirstPPic == 1)
  {
    qp = CLIP(lastPicQP - info->lastPicQpClip, lastPicQP + g_rcPicQpNormalClip, qp);
  }
  else if (lastPicQP > 0 && isIPic == 0)
  {
    qp = CLIP(lastPicQP - info->lastPicQpClip, lastPicQP + info->lastPicQpClip, qp);
  }
  else if (lastPicQP > 0)
  {
    qp = CLIP(lastPicQP - info->picQpNormalClip, lastPicQP + info->picQpNormalClip, qp);
  }
  int maxQp = (info->isCurPicI) ? info->maxIQp : info->maxQp;
  int minQp = (info->isCurPicI) ? info->minIQp : info->minQp;
  return CLIP(minQp, maxQp, qp);
}

// --------------------- CVI RC kernel Set property ---------------------

void cviRcKernel_setTextCplx(stRcKernelInfo *info, RC_Float madi)
{
  if(CVI_FLOAT_GT(madi, FLOAT_VAL_0)) {
   info->picTextCplx = CVI_FLOAT_POW(CVI_FLOAT_MAX(madi, FLOAT_VAL_1), g_rcTcBETA);
  }
}

void cviRcKernel_setMinMaxQp(stRcKernelInfo *info, int minQp, int maxQp, int isIntra)
{
  if(isIntra==1) {
    info->maxIQp = maxQp;
    info->minIQp = minQp;
    info->maxILambda = QpToLambda(INT_TO_CVI_FLOAT(info->maxIQp));
    info->minILambda = QpToLambda(INT_TO_CVI_FLOAT(info->minIQp));
  }
  else {
    info->maxQp = maxQp;
    info->minQp = minQp;
    info->maxLambda = QpToLambda(INT_TO_CVI_FLOAT(info->maxQp));
    info->minLambda = QpToLambda(INT_TO_CVI_FLOAT(info->minQp));
  }
}

void cviRcKernel_setBitrateAndFrameRate(stRcKernelInfo *info, int targetBitrate, RC_Float frameRate)
{
  if(targetBitrate>0) {
    info->targetBitrate = targetBitrate;
  }
  if(CVI_FLOAT_GT(frameRate, FLOAT_VAL_0)) {
    info->framerate = CVI_FLOAT_TO_INT(frameRate);
    info->statFrameNum = MAX(info->statTime * info->framerate, info->framerate);
  }
  // picture bit allocation param init
  RC_Float fPicAvgBit = CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(info->targetBitrate), frameRate);
  info->picAvgBit = CVI_FLOAT_TO_INT(fPicAvgBit);
  info->neiIBitMargin = info->targetBitrate / 10;
  int maxIPicBitByIPRatio = CVI_FLOAT_TO_INT(CVI_FLOAT_MUL(CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(info->maxIprop * info->intraPeriod), INT_TO_CVI_FLOAT(info->maxIprop + info->intraPeriod - 1)), fPicAvgBit));
  int maxIPicBitByMinPBudget = CVI_FLOAT_TO_INT(CVI_FLOAT_MUL(INT_TO_CVI_FLOAT(info->intraPeriod), fPicAvgBit)) - (info->minPicBit * (info->intraPeriod - 1));
  info->minIPicBit = CVI_FLOAT_TO_INT(CVI_FLOAT_MUL(CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(info->minIprop * info->intraPeriod), INT_TO_CVI_FLOAT(info->minIprop + info->intraPeriod - 1)), fPicAvgBit));
  info->maxIPicBit = MIN(maxIPicBitByIPRatio, maxIPicBitByMinPBudget);

  if (info->rcMdlUpdatType == 0) {
    RC_Float targetBpp = CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(info->targetBitrate), CVI_FLOAT_MUL(frameRate, INT_TO_CVI_FLOAT(info->numOfPixel)));
    rcModelUpdateParam_Init(info, targetBpp);
  }
  info->bitError = 0;
}

void cviRcKernel_setLevelPicQpClip(stRcKernelInfo *info, int levelPicQpClip)
{
  info->levelPicQpClip = levelPicQpClip;
  info->lastLevelLambdaScaleLow = CVI_FLOAT_POW(FLOAT_VAL_2, CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(-levelPicQpClip), FLOAT_VAL_3));
  info->lastLevelLambdaScaleHigh = CVI_FLOAT_POW(FLOAT_VAL_2, CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(levelPicQpClip), FLOAT_VAL_3));
}

void cviRcKernel_setLastPicQpClip(stRcKernelInfo *info, int lastPicQpClip)
{
  info->lastPicQpClip = lastPicQpClip;
  info->lastPicLambdaScaleLow = CVI_FLOAT_POW(FLOAT_VAL_2, CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(-lastPicQpClip), FLOAT_VAL_3));
  info->lastPicLambdaScaleHigh = CVI_FLOAT_POW(FLOAT_VAL_2, CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(lastPicQpClip), FLOAT_VAL_3));
}
void cviRcKernel_setpPicQpNormalClip(stRcKernelInfo *info, int picQpNormalClip)
{
  info->picQpNormalClip = picQpNormalClip;
  info->lastNormalLambdaScaleLow = CVI_FLOAT_POW(FLOAT_VAL_2, CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(-picQpNormalClip), FLOAT_VAL_3));
  info->lastNormalLambdaScaleHigh= CVI_FLOAT_POW(FLOAT_VAL_2, CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(picQpNormalClip), FLOAT_VAL_3));
}

void cviRcKernel_setRCModelParam(stRcKernelInfo *info, RC_Float alpha, RC_Float beta, int model_idx)
{
  stRLModel *rqModel = info->rqModel;
  int mdl_i = CLIP(0, MAX_T_LAYER, model_idx);
  rqModel[mdl_i].alpha = alpha;
  rqModel[mdl_i].beta = beta;
}

void cviRcKernel_setRCModelUpdateStep(stRcKernelInfo *info, RC_Float alphaStep, RC_Float betaStep)
{
  info->alphaStep = alphaStep;
  info->betaStep = betaStep;
}
// --------------------- CVI RC kernel main flow control ---------------------

void cviRcKernel_init(stRcKernelInfo *info, stRcKernelCfg *cfg)
{
  info->intraPeriod = cfg->intraPeriod;
  info->numOfPixel = cfg->numOfPixel;
  info->ipQpDelta = cfg->ipQpDelta;
  info->firstFrmstartQp = cfg->firstFrmstartQp;
  info->rcMdlUpdatType = cfg->rcMdlUpdatType;
  info->statTime = cfg->statTime;
  info->maxIprop = cfg->maxIprop;
  info->minIprop = cfg->minIprop;
  info->minPicBit = CVI_FLOAT_TO_INT(CVI_FLOAT_MUL(INT_TO_CVI_FLOAT(info->numOfPixel), g_rcMinPPicBpp));

  cviRcKernel_setMinMaxQp(info, cfg->minIQp, cfg->maxIQp, 1);
  cviRcKernel_setMinMaxQp(info, cfg->minQp, cfg->maxQp, 0);
  // should be called after above params. are assigned
  cviRcKernel_setBitrateAndFrameRate(info, cfg->targetBitrate, cfg->framerate);
  cviRcKernel_setTextCplx(info, g_picAvgTc);

  // find a proper smooth bitrate-constraint unit size
  int rcIterNum = 6;
  int baseRcGopSize = MAX(MIN(info->intraPeriod, info->statFrameNum) / rcIterNum, 1);
  info->rcGopSize = 1;
  for (int size = baseRcGopSize; size <= info->intraPeriod; size++)
  {
    if (info->intraPeriod % size == 0) {
      info->rcGopSize = size;
      break;
    }
  }

  // pre-calculate common floating point constant
  cviRcKernel_setLastPicQpClip(info, g_rcLastPicQpClip);
  cviRcKernel_setLevelPicQpClip(info, g_rcLevelPicQpClip);
  cviRcKernel_setpPicQpNormalClip(info, g_rcPicQpNormalClip);

  // RC model param init
  cviRcKernel_setRCModelParam(info, g_rcALPHA_I, g_rcBETA_I, 0);
  cviRcKernel_setRCModelParam(info, g_rcALPHA_P, g_rcBETA_P, 1);

  // variable reset
  info->bitError = 0;
  info->lastIPicBit = -1;
  info->isCurPicI = 1;
  info->isLastPicI = 0;
  info->pPicQpAccum = FLOAT_VAL_0;
  info->pPicCnt = 0;
  info->isFirstPic = 1;
  info->lastLevelLambda[0] = info->lastLevelLambda[1] = FLOAT_VAL_minus_1;
  info->lastPicLambda = FLOAT_VAL_minus_1;
  info->lastLevelQp[0] = info->lastLevelQp[1] = -1;
  info->lastPicQp = -1;
}

void cviRcKernel_estimatePic(stRcKernelInfo *info, stRcKernelPicOut *out, int isIPic, int picIdx)
{
  info->isCurPicI = isIPic;
  int picTargetBit;
  int qp = info->firstFrmstartQp;
  if ((isIPic && info->intraPeriod != 1) || info->isFirstPic == 1)
  {
    if (info->isFirstPic == 0)
    {
      qp = getAvgPFrameQp(info);
    }
    picTargetBit = rcIPicBitAlloc(info, qp);
  }
  else
  {
    if ((info->isLastPicI == 1) || (picIdx % info->rcGopSize == 1) || (info->rcGopSize == 1))
    {
      rcGopBitAlloc(info, picIdx);
    }
    picTargetBit = estPicTargetBits(info);
  }

  RC_Float lambda = estPicLambda(info, picTargetBit, isIPic, picIdx);
  if (info->isFirstPic != 1 || info->intraPeriod == 1) {
    qp = estPicQP(info, lambda, isIPic, picIdx);
  }

  // re-allocate picture target bitrate
  int targetBitByModel = estPicBitByModel(info, lambda, isIPic);
  info->picTargetBit = MAX((picTargetBit + targetBitByModel) / 2, 0);

  // output parameters
  out->targetBit = info->picTargetBit;
  out->qp = qp;
  out->lambda = lambda;
}

void cviRcKernel_updatePic(stRcKernelInfo *info, stRcKernelPicIn *stats, int isIPic)
{
  if(CVI_FLOAT_LE(stats->encodedLambda, FLOAT_VAL_0)) {
    info->lastPicLambda = QpToLambda(stats->encodedQp);
  }
  else {
    info->lastPicLambda = stats->encodedLambda;
  }
  info->lastPicQp = CVI_FLOAT_TO_INT(stats->encodedQp);

  int picLv = isIPic == 0;
  // update bitrate status
  info->bitError = CLIP(-MAX_BIT_ERROR, MAX_BIT_ERROR, info->bitError + info->picAvgBit - stats->encodedBit);
  info->gopBitLeft -= stats->encodedBit;
  info->gopPicLeft -= 1;

  if(isIPic==0 && stats->skipRatio==FLOAT_VAL_minus_1) {
    goto MDL_PARAM_UPDATE_END;
  }

  // update model parameters
  RC_Float alpha = info->rqModel[picLv].alpha;
  RC_Float beta = info->rqModel[picLv].beta;
  if (isIPic == 1)
  {
    updateAlphaBetaIntra(&alpha, &beta, info->picTextCplx, info->picTargetBit, stats->encodedBit);
  }
  else if (info->rcMdlUpdatType == 0)
  {
    // LMS algorithm
    RC_Float picActualBpp = FRAC_INT_TO_CVI_FLOAT(((long long int)stats->encodedBit<<FIX_POINT_FRAC_BIT) / info->numOfPixel, FIX_POINT_FRAC_BIT);
    RC_Float calLambda = CVI_FLOAT_MUL(alpha, CVI_FLOAT_POW(picActualBpp, beta));
    RC_Float inputLambda = info->lastPicLambda;
    calLambda = CVI_FLOAT_CLIP(CVI_FLOAT_DIV(inputLambda, FLOAT_VAL_10), CVI_FLOAT_MUL(inputLambda, FLOAT_VAL_10), calLambda);
    RC_Float logLambdaDiff = CVI_FLOAT_SUB(CVI_FLOAT_LOG(inputLambda), CVI_FLOAT_LOG(calLambda));
    RC_Float alpha_incr = CVI_FLOAT_MUL(CVI_FLOAT_MUL(info->alphaStep, logLambdaDiff), alpha);
    alpha = CVI_FLOAT_ADD(alpha, alpha_incr);
    RC_Float lnbpp = CVI_FLOAT_LOG(picActualBpp);
    lnbpp = CVI_FLOAT_CLIP(FLOAT_VAL_minus_5, FLOAT_VAL_minus_p01, lnbpp);
    RC_Float beta_incr = CVI_FLOAT_MUL(CVI_FLOAT_MUL(info->betaStep, logLambdaDiff), lnbpp);
    beta = CVI_FLOAT_ADD(beta, beta_incr);
    alpha = CVI_FLOAT_CLIP(g_rcAlphaMinValue, g_rcAlphaMaxValue, alpha);
    beta = CVI_FLOAT_CLIP(g_rcBetaMinValue, g_rcBetaMaxValue, beta);
  }
  else
  { // RD function analytics
    RC_Float picActualBpp = FRAC_INT_TO_CVI_FLOAT(((long long int)stats->encodedBit<<FIX_POINT_FRAC_BIT) / info->numOfPixel, FIX_POINT_FRAC_BIT);
    RC_Float updatedK = CVI_FLOAT_DIV(CVI_FLOAT_MUL(picActualBpp, info->lastPicLambda), stats->mse);
    RC_Float negative_updateK = CVI_FLOAT_MUL(FLOAT_VAL_minus_1, updatedK);
    RC_Float updatedC = CVI_FLOAT_DIV(stats->mse, CVI_FLOAT_POW(picActualBpp, negative_updateK));
    RC_Float updateScale = CVI_FLOAT_CLIP(FLOAT_VAL_p1, FLOAT_VAL_p9, CVI_FLOAT_SUB(FLOAT_VAL_1, stats->skipRatio));
    RC_Float newAlpha = CVI_FLOAT_MUL(updatedC, updatedK);
    RC_Float newBeta = CVI_FLOAT_SUB(FLOAT_VAL_0, CVI_FLOAT_ADD(updatedK, FLOAT_VAL_1));
    RC_Float oneMinusUpdateScale = CVI_FLOAT_SUB(FLOAT_VAL_1, updateScale);
    RC_Float updateAlpha = CVI_FLOAT_ADD(CVI_FLOAT_MUL(updateScale, newAlpha), CVI_FLOAT_MUL(oneMinusUpdateScale, alpha));
    RC_Float updateBeta = CVI_FLOAT_ADD(CVI_FLOAT_MUL(updateScale, newBeta), CVI_FLOAT_MUL(oneMinusUpdateScale, beta));
    alpha = CVI_FLOAT_CLIP(g_rcAlphaMinValue, g_rcAlphaMaxValue, updateAlpha);
    beta = CVI_FLOAT_CLIP(g_rcBetaMinValue, g_rcBetaMaxValue, updateBeta);
  }

  info->rqModel[picLv].alpha = alpha;
  info->rqModel[picLv].beta = beta;
  info->lastLevelLambda[picLv] = info->lastPicLambda;
  info->lastLevelQp[picLv] = info->lastPicQp;
  if (isIPic == 0) {
    info->pPicQpAccum = CVI_FLOAT_ADD(info->pPicQpAccum, stats->encodedQp);
    info->pPicCnt += 1;
  }

MDL_PARAM_UPDATE_END :
  if (isIPic == 1)
  {
    info->lastIPicBit = stats->encodedBit;
    info->pPicQpAccum = 0;
    info->pPicCnt = 0;
  }
  info->isLastPicI = info->isCurPicI;
  info->isFirstPic = 0;
  // texture complexity for I frame model
  cviRcKernel_setTextCplx(info, stats->madi);
}
