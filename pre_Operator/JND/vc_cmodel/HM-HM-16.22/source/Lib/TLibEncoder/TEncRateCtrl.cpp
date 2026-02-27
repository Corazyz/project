/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2020, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncRateCtrl.cpp
    \brief    Rate control manager class
*/
#include "TEncRateCtrl.h"
#include "cvi_rate_ctrl.h"
#include "cvi_pattern.h"
#include "cvi_motion.h"
#include "../TLibCommon/TComPic.h"
#include "../TLibCommon/TComChromaFormat.h"
#include "../TLibCommon/cvi_float_point.h"
#include "../TLibCommon/cvi_soft_float.h"

#ifdef CVI_RANDOM_ENCODE
#include "cvi_reg_statistic.h"
#endif

#include <cmath>

using namespace std;

static Int LambdaToQp(RC_Float lambda)
{
#if SOFT_FLOAT
  return CVI_FLOAT_TO_INT(CVI_FLOAT_ADD(CVI_FLOAT_MUL(FLOAT_VAL_4p2005, \
                    CVI_FLOAT_LOG(Float_to_SoftFloat(lambda))),FLOAT_VAL_13p7122));
#else
  return Int( ((RC_Float)4.2005 * log( lambda) ) + (RC_Float)13.7122 + (RC_Float)0.5 );
#endif
}

static RC_Float QpToLambda(RC_Float qp)
{
#if SOFT_FLOAT
  return SoftFloat_to_Float(CVI_FLOAT_EXP(CVI_FLOAT_DIV(CVI_FLOAT_SUB(Float_to_SoftFloat(qp) , FLOAT_VAL_13p7122) , FLOAT_VAL_4p2005)));
#else
  return exp(((RC_Float)(qp-(RC_Float)13.7122)/(RC_Float)4.2005));
#endif
}

//sequence level
TEncRCSeq::TEncRCSeq()
{
  m_totalFrames         = 0;
  m_targetRate          = 0;
  m_frameRate           = 0;
  m_targetBits          = 0;
  m_GOPSize             = 0;
  m_picWidth            = 0;
  m_picHeight           = 0;
  m_LCUWidth            = 0;
  m_LCUHeight           = 0;
  m_numberOfLevel       = 0;
  m_numberOfLCU         = 0;
  m_averageBits         = 0;
  m_bitsRatio           = NULL;
  m_GOPID2Level         = NULL;
  m_picPara             = NULL;
  m_LCUPara             = NULL;
  m_numberOfPixel       = 0;
  m_framesLeft          = 0;
  m_bitsLeft            = 0;
  m_useLCUSeparateModel = false;
  m_adaptiveBit         = 0;
  m_lastLambda          = 0.0;
  m_pictureIdx          = 0;
#if CVI_LOW_LUX_CONTROL
  m_LowLuxEnControl     = false;
  m_LuxValue            = 0;
#endif
}

TEncRCSeq::~TEncRCSeq()
{
  destroy();
}

Void TEncRCSeq::create( Int totalFrames, Int targetBitrate, Int frameRate, Int GOPSize, Int picWidth, Int picHeight, Int LCUWidth, Int LCUHeight, Int numberOfLevel, Bool useLCUSeparateModel, Int adaptiveBit, Int intraPeriod, Bool useLowLuxEnControl, Int LuxValue, Int intraQPoffset)
{
  destroy();
  m_totalFrames         = totalFrames;
  m_targetRate          = targetBitrate;
  m_frameRate           = frameRate;
  m_GOPSize             = GOPSize;
  m_picWidth            = picWidth;
  m_picHeight           = picHeight;
  m_LCUWidth            = LCUWidth;
  m_LCUHeight           = LCUHeight;
  m_numberOfLevel       = numberOfLevel;
  m_useLCUSeparateModel = useLCUSeparateModel;
  m_intraPeriod         = intraPeriod;
  m_numberOfPixel   = m_picWidth * m_picHeight;
  m_numberOfPixelAlign32 = cvi_mem_align(m_picWidth, 32) * cvi_mem_align(m_picHeight, 32);
  m_targetBits      = ((Int64)m_totalFrames * m_targetRate) / m_frameRate;
  m_seqTargetBpp = m_targetRate / (RC_Float)m_frameRate / (RC_Float)m_numberOfPixel;
  m_intraQPoffset = intraQPoffset;

#if CVI_LOW_LUX_CONTROL
  int idx;
  m_LowLuxEnControl = useLowLuxEnControl;
  m_LuxValue = LuxValue;
  for (idx = 0; idx < MAX_LOW_LUX_WIN; idx++){
    m_picLowLuxWindow[idx] = LOW_LUX_DEFAULT;
  }

  if((LuxValue > MINIMUM_LOW_LUX_VALUE) && (LuxValue < MAXIMUX_LOW_LUX_VALUE)){
    setLowLuxAdaptive(0);
  } 
  else
  {
    setLowLuxAdaptive(1);
  }

  setLowLuxAvg(LOW_LUX_DEFAULT); 
  setLowLuxLevel(0);
#endif

  if ( m_seqTargetBpp < 0.03 )
  {
    m_alphaUpdate = 0.01;
    m_betaUpdate  = 0.005;
  }
  else if ( m_seqTargetBpp < 0.08 )
  {
    m_alphaUpdate = 0.05;
    m_betaUpdate  = 0.025;
  }
  else if ( m_seqTargetBpp < 0.2 )
  {
    m_alphaUpdate = 0.1;
    m_betaUpdate  = 0.05;
  }
  else if ( m_seqTargetBpp < 0.5 )
  {
    m_alphaUpdate = 0.2;
    m_betaUpdate  = 0.1;
  }
  else
  {
    m_alphaUpdate = 0.4;
    m_betaUpdate  = 0.2;
  }

#ifdef CVI_STREAMING_RC
  m_averageBits = targetBitrate / m_frameRate;
  m_minPicBit =  m_numberOfPixel * g_RCMinPPicBpp;
  Int maxIPicBitByIPRatio = (Int)(((Int64)g_RCMaxIprop *m_intraPeriod * targetBitrate) / (m_frameRate *(g_RCMaxIprop + m_intraPeriod - 1)));
  Int maxIPicBitByMinPBudget = (Int)((Int64)m_intraPeriod * targetBitrate / m_frameRate) - (m_minPicBit*(m_intraPeriod-1));
  maxIPicBitByMinPBudget = max(maxIPicBitByMinPBudget, m_minPicBit);
  m_maxIPicBit = min(maxIPicBitByIPRatio, maxIPicBitByMinPBudget);
  //aka add 20240321
#if SOFT_FLOAT
  Soft_Float scale = CVI_FLOAT_POW(INT_TO_CVI_FLOAT(2), CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(-m_intraQPoffset), INT_TO_CVI_FLOAT(6)));
  m_maxIPicBit     = CVI_FLOAT_TO_INT_MODE (CVI_FLOAT_MUL(scale , INT_TO_CVI_FLOAT(m_maxIPicBit)), 2);
#else
   m_maxIPicBit = pow(2,(float)-m_intraQPoffset/6)*m_maxIPicBit;
#endif
  //aka add 20240321~
  m_minIPicBit = (Int)((g_RCMinIprop * m_intraPeriod * (Int64)targetBitrate) / (m_frameRate *(g_RCMinIprop + m_intraPeriod - 1)));
  m_StatFrameNum = max(g_StatTime, 1)*frameRate;
  // find bitrate constraint unit size in picture
  Int rcIterNum = 6;
  Int baseRcGopSize = max(min(m_intraPeriod, (Int)m_StatFrameNum)/rcIterNum, 1);
  m_rcGopSize = 1;
  for(Int size=baseRcGopSize; size<=m_intraPeriod; size++)
  {
    if(m_intraPeriod%size==0){
      m_rcGopSize = size;
      break;
    }
  }
  m_rcGopSize = ((m_rcGopSize + m_GOPSize - 1)/m_GOPSize)*m_GOPSize;
  m_neiIBitMargin = targetBitrate / 10;
  m_lastSliceType = m_curSliceType = NUMBER_OF_SLICE_TYPES;
  m_bitError = 0;
  m_lastIPicBit = -1;
  m_lastILambda = -1;
  m_lastIQp = g_RCInvalidQPValue;
#else
  m_averageBits     = (Int)(m_targetBits / totalFrames);
#endif
  Int picWidthInBU  = ( m_picWidth  % m_LCUWidth  ) == 0 ? m_picWidth  / m_LCUWidth  : m_picWidth  / m_LCUWidth  + 1;
  Int picHeightInBU = ( m_picHeight % m_LCUHeight ) == 0 ? m_picHeight / m_LCUHeight : m_picHeight / m_LCUHeight + 1;
  m_numberOfLCU     = picWidthInBU * picHeightInBU;

  m_bitsRatio   = new Int[m_GOPSize];
  for ( Int i=0; i<m_GOPSize; i++ )
  {
    m_bitsRatio[i] = 1;
  }

  m_GOPID2Level = new Int[m_GOPSize];
  for ( Int i=0; i<m_GOPSize; i++ )
  {
    m_GOPID2Level[i] = 1;
  }

  m_picPara = new TRCParameter[m_numberOfLevel];
  for ( Int i=0; i<m_numberOfLevel; i++ )
  {
    m_picPara[i].m_alpha = 0.0;
    m_picPara[i].m_beta  = 0.0;
#if JVET_K0390_RATE_CTRL
    m_picPara[i].m_validPix = -1;
#endif
#if JVET_M0600_RATE_CTRL
    m_picPara[i].m_skipRatio = 0.0;
#endif
  }

  if ( m_useLCUSeparateModel )
  {
    m_LCUPara = new TRCParameter*[m_numberOfLevel];
    for ( Int i=0; i<m_numberOfLevel; i++ )
    {
      m_LCUPara[i] = new TRCParameter[m_numberOfLCU];
      for ( Int j=0; j<m_numberOfLCU; j++)
      {
        m_LCUPara[i][j].m_alpha = 0.0;
        m_LCUPara[i][j].m_beta  = 0.0;
#if JVET_K0390_RATE_CTRL
        m_LCUPara[i][j].m_validPix = -1;
#endif
#if JVET_M0600_RATE_CTRL
        m_LCUPara[i][j].m_skipRatio = 0.0;
#endif
      }
    }
  }

  m_framesLeft = m_totalFrames;
  m_bitsLeft   = m_targetBits;
  m_adaptiveBit = adaptiveBit;
  m_lastLambda = 0.0;
  m_historyPicQpAccum = (RC_Float)g_RCInvalidQPValue;
}

Void TEncRCSeq::destroy()
{
  if (m_bitsRatio != NULL)
  {
    delete[] m_bitsRatio;
    m_bitsRatio = NULL;
  }

  if ( m_GOPID2Level != NULL )
  {
    delete[] m_GOPID2Level;
    m_GOPID2Level = NULL;
  }

  if ( m_picPara != NULL )
  {
    delete[] m_picPara;
    m_picPara = NULL;
  }

  if ( m_LCUPara != NULL )
  {
    for ( Int i=0; i<m_numberOfLevel; i++ )
    {
      delete[] m_LCUPara[i];
    }
    delete[] m_LCUPara;
    m_LCUPara = NULL;
  }
}

Void TEncRCSeq::initBitsRatio( Int bitsRatio[])
{
  for (Int i=0; i<m_GOPSize; i++)
  {
    m_bitsRatio[i] = bitsRatio[i];
  }
}

Void TEncRCSeq::initGOPID2Level( Int GOPID2Level[] )
{
  for ( Int i=0; i<m_GOPSize; i++ )
  {
    m_GOPID2Level[i] = GOPID2Level[i];
  }
}

Void TEncRCSeq::initPicPara( TRCParameter* picPara )
{
  assert( m_picPara != NULL );

  if ( picPara == NULL )
  {
    for ( Int i=0; i<m_numberOfLevel; i++ )
    {
      if (i>0)
      {
        m_picPara[i].m_alpha = 3.2003;
        m_picPara[i].m_beta  = -1.367;
      }
      else
      {
        m_picPara[i].m_alpha = ALPHA;
        m_picPara[i].m_beta  = BETA2;
      }
    }
  }
  else
  {
    for ( Int i=0; i<m_numberOfLevel; i++ )
    {
      m_picPara[i] = picPara[i];
    }
  }
}

Void TEncRCSeq::initLCUPara( TRCParameter** LCUPara )
{
  if ( m_LCUPara == NULL )
  {
    return;
  }
  if ( LCUPara == NULL )
  {
    for ( Int i=0; i<m_numberOfLevel; i++ )
    {
      for ( Int j=0; j<m_numberOfLCU; j++)
      {
        m_LCUPara[i][j].m_alpha = m_picPara[i].m_alpha;
        m_LCUPara[i][j].m_beta  = m_picPara[i].m_beta;
      }
    }
  }
  else
  {
    for ( Int i=0; i<m_numberOfLevel; i++ )
    {
      for ( Int j=0; j<m_numberOfLCU; j++)
      {
        m_LCUPara[i][j] = LCUPara[i][j];
      }
    }
  }
}

Void TEncRCSeq::updateAfterPic ( Int bits )
{
  m_bitsLeft -= bits;
  m_framesLeft--;
#ifdef CVI_STREAMING_RC
 #if CVI_LOW_LUX_CONTROL
  if (m_LowLuxEnControl){
    if (!getLowLuxAdaptive()){
      if ( m_LuxValue < LOW_LUX_STRONG_THRESHOLD)
      {
        m_bitError = Clip3(-MAX_BIT_ERROR, MAX_BIT_ERROR, (Int)(m_bitError + m_averageBits*LOW_LUX_CONTROL_STRONG / 10 - bits));
      }
      else if( m_LuxValue < LOW_LUX_MEDIUM_THRESHOLD )
      {
        m_bitError = Clip3(-MAX_BIT_ERROR, MAX_BIT_ERROR, (Int)(m_bitError + m_averageBits*LOW_LUX_CONTROL_MEDIUM / 10 - bits));
      }
      else
      {
        m_bitError = Clip3(-MAX_BIT_ERROR, MAX_BIT_ERROR, (Int)(m_bitError + m_averageBits - bits));
      }
    }
    else{
      if ( m_picLowLuxLevel == 2)
      {
        m_bitError = Clip3(-MAX_BIT_ERROR, MAX_BIT_ERROR, (Int)(m_bitError + m_averageBits*LOW_LUX_CONTROL_STRONG / 10 - bits));
      }
      else if( m_picLowLuxLevel == 1)
      {
        m_bitError = Clip3(-MAX_BIT_ERROR, MAX_BIT_ERROR, (Int)(m_bitError + m_averageBits*LOW_LUX_CONTROL_MEDIUM / 10 - bits));
      }
      else
      {
        m_bitError = Clip3(-MAX_BIT_ERROR, MAX_BIT_ERROR, (Int)(m_bitError + m_averageBits - bits));
      }
    }
  }
  else{
    m_bitError = Clip3(-MAX_BIT_ERROR, MAX_BIT_ERROR, (Int)(m_bitError + m_averageBits - bits));
  }
#endif

  if(m_curSliceType==I_SLICE) {
    m_lastIPicBit = bits;
  }
#if RC_DEBUG
  if(g_RCDebugLogEn) {
    printf("bitErr %d\n", (Int)m_bitError);
  }
#endif
#endif
}

Void TEncRCSeq::setAllBitRatio( RC_Float basicLambda, RC_Float* equaCoeffA, RC_Float* equaCoeffB )
{
  Int* bitsRatio = new Int[m_GOPSize];
  for ( Int i=0; i<m_GOPSize; i++ )
  {
#if JVET_K0390_RATE_CTRL
    bitsRatio[i] = (Int)( equaCoeffA[i] * pow(basicLambda, equaCoeffB[i]) * getPicPara(getGOPID2Level(i)).m_validPix);
#else
    bitsRatio[i] = (Int)( equaCoeffA[i] * pow( basicLambda, equaCoeffB[i] ) * m_numberOfPixel );
#endif
  }
  initBitsRatio( bitsRatio );
  delete[] bitsRatio;
}

//GOP level
TEncRCGOP::TEncRCGOP()
{
  m_encRCSeq  = NULL;
  m_picTargetBitInGOP = NULL;
  m_numPic     = 0;
  m_targetBits = 0;
  m_picLeft    = 0;
  m_bitsLeft   = 0;
}

TEncRCGOP::~TEncRCGOP()
{
  destroy();
}

Void TEncRCGOP::create( TEncRCSeq* encRCSeq, Int numPic )
{
  destroy();
  Int targetBits = xEstGOPTargetBits( encRCSeq, numPic );

  if ( encRCSeq->getAdaptiveBits() > 0 && encRCSeq->getLastLambda() > 0.1 )
  {
    RC_Float targetBpp = (RC_Float)targetBits / encRCSeq->getNumPixel();
    RC_Float basicLambda = 0.0;
    RC_Float* lambdaRatio = new RC_Float[encRCSeq->getGOPSize()];
    RC_Float* equaCoeffA = new RC_Float[encRCSeq->getGOPSize()];
    RC_Float* equaCoeffB = new RC_Float[encRCSeq->getGOPSize()];

    if ( encRCSeq->getAdaptiveBits() == 1 )   // for GOP size =4, low delay case
    {
      if ( encRCSeq->getLastLambda() < 120.0 )
      {
        lambdaRatio[1] = 0.725 * log( encRCSeq->getLastLambda() ) + 0.5793;
        lambdaRatio[0] = 1.3 * lambdaRatio[1];
        lambdaRatio[2] = 1.3 * lambdaRatio[1];
        lambdaRatio[3] = 1.0;
      }
      else
      {
        lambdaRatio[0] = 5.0;
        lambdaRatio[1] = 4.0;
        lambdaRatio[2] = 5.0;
        lambdaRatio[3] = 1.0;
      }
    }
    else if ( encRCSeq->getAdaptiveBits() == 2 )  // for GOP size = 8, random access case
    {
      if ( encRCSeq->getLastLambda() < 90.0 )
      {
        lambdaRatio[0] = 1.0;
        lambdaRatio[1] = 0.725 * log( encRCSeq->getLastLambda() ) + 0.7963;
        lambdaRatio[2] = 1.3 * lambdaRatio[1];
        lambdaRatio[3] = 3.25 * lambdaRatio[1];
        lambdaRatio[4] = 3.25 * lambdaRatio[1];
        lambdaRatio[5] = 1.3  * lambdaRatio[1];
        lambdaRatio[6] = 3.25 * lambdaRatio[1];
        lambdaRatio[7] = 3.25 * lambdaRatio[1];
      }
      else
      {
        lambdaRatio[0] = 1.0;
        lambdaRatio[1] = 4.0;
        lambdaRatio[2] = 5.0;
        lambdaRatio[3] = 12.3;
        lambdaRatio[4] = 12.3;
        lambdaRatio[5] = 5.0;
        lambdaRatio[6] = 12.3;
        lambdaRatio[7] = 12.3;
      }
    }
#if JVET_K0390_RATE_CTRL
    else if (encRCSeq->getAdaptiveBits() == 3)  // for GOP size = 16, random access case
    {
      RC_Float hierarQp = 4.2005 * log(encRCSeq->getLastLambda()) + 13.7122;  //  the qp of POC16
      RC_Float qpLev2 = (hierarQp + 0.0) + 0.2016    * (hierarQp + 0.0) - 4.8848;
      RC_Float qpLev3 = (hierarQp + 3.0) + 0.22286 * (hierarQp + 3.0) - 5.7476;
      RC_Float qpLev4 = (hierarQp + 4.0) + 0.2333    * (hierarQp + 4.0) - 5.9;
      RC_Float qpLev5 = (hierarQp + 5.0) + 0.3            * (hierarQp + 5.0) - 7.1444;

      RC_Float lambdaLev1 = exp((hierarQp - 13.7122) / 4.2005);
      RC_Float lambdaLev2 = exp((qpLev2 - 13.7122) / 4.2005);
      RC_Float lambdaLev3 = exp((qpLev3 - 13.7122) / 4.2005);
      RC_Float lambdaLev4 = exp((qpLev4 - 13.7122) / 4.2005);
      RC_Float lambdaLev5 = exp((qpLev5 - 13.7122) / 4.2005);

      lambdaRatio[0] = 1.0;
      lambdaRatio[1] = lambdaLev2 / lambdaLev1;
      lambdaRatio[2] = lambdaLev3 / lambdaLev1;
      lambdaRatio[3] = lambdaLev4 / lambdaLev1;
      lambdaRatio[4] = lambdaLev5 / lambdaLev1;
      lambdaRatio[5] = lambdaLev5 / lambdaLev1;
      lambdaRatio[6] = lambdaLev4 / lambdaLev1;
      lambdaRatio[7] = lambdaLev5 / lambdaLev1;
      lambdaRatio[8] = lambdaLev5 / lambdaLev1;
      lambdaRatio[9] = lambdaLev3 / lambdaLev1;
      lambdaRatio[10] = lambdaLev4 / lambdaLev1;
      lambdaRatio[11] = lambdaLev5 / lambdaLev1;
      lambdaRatio[12] = lambdaLev5 / lambdaLev1;
      lambdaRatio[13] = lambdaLev4 / lambdaLev1;
      lambdaRatio[14] = lambdaLev5 / lambdaLev1;
      lambdaRatio[15] = lambdaLev5 / lambdaLev1;
#if JVET_M0600_RATE_CTRL
      const RC_Float qdfParaLev2A = 0.5847;
      const RC_Float qdfParaLev2B = -0.0782;
      const RC_Float qdfParaLev3A = 0.5468;
      const RC_Float qdfParaLev3B = -0.1364;
      const RC_Float qdfParaLev4A = 0.6539;
      const RC_Float qdfParaLev4B = -0.203;
      const RC_Float qdfParaLev5A = 0.8623;
      const RC_Float qdfParaLev5B = -0.4676;
      RC_Float qdfLev1Lev2 = Clip3<RC_Float>(0.12, 0.9, qdfParaLev2A * encRCSeq->getPicPara(2).m_skipRatio + qdfParaLev2B);
      RC_Float qdfLev1Lev3 = Clip3<RC_Float>(0.13, 0.9, qdfParaLev3A * encRCSeq->getPicPara(3).m_skipRatio + qdfParaLev3B);
      RC_Float qdfLev1Lev4 = Clip3<RC_Float>(0.15, 0.9, qdfParaLev4A * encRCSeq->getPicPara(4).m_skipRatio + qdfParaLev4B);
      RC_Float qdfLev1Lev5 = Clip3<RC_Float>(0.20, 0.9, qdfParaLev5A * encRCSeq->getPicPara(5).m_skipRatio + qdfParaLev5B);
      RC_Float qdfLev2Lev3 = Clip3<RC_Float>(0.09, 0.9, qdfLev1Lev3 * (1 - qdfLev1Lev2));
      RC_Float qdfLev2Lev4 = Clip3<RC_Float>(0.12, 0.9, qdfLev1Lev4 * (1 - qdfLev1Lev2));
      RC_Float qdfLev2Lev5 = Clip3<RC_Float>(0.14, 0.9, qdfLev1Lev5 * (1 - qdfLev1Lev2));
      RC_Float qdfLev3Lev4 = Clip3<RC_Float>(0.06, 0.9, qdfLev1Lev4 * (1 - qdfLev1Lev3));
      RC_Float qdfLev3Lev5 = Clip3<RC_Float>(0.09, 0.9, qdfLev1Lev5 * (1 - qdfLev1Lev3));
      RC_Float qdfLev4Lev5 = Clip3<RC_Float>(0.10, 0.9, qdfLev1Lev5 * (1 - qdfLev1Lev4));

      lambdaLev1 = 1 / (1 + 2 * (qdfLev1Lev2 + 2 * qdfLev1Lev3 + 4 * qdfLev1Lev4 + 8 * qdfLev1Lev5));
      lambdaLev2 = 1 / (1 + (3 * qdfLev2Lev3 + 5 * qdfLev2Lev4 + 8 * qdfLev2Lev5));
      lambdaLev3 = 1 / (1 + 2 * qdfLev3Lev4 + 4 * qdfLev3Lev5);
      lambdaLev4 = 1 / (1 + 2 * qdfLev4Lev5);
      lambdaLev5 = 1 / (1.0);

      lambdaRatio[0] = 1.0;
      lambdaRatio[1] = lambdaLev2 / lambdaLev1;
      lambdaRatio[2] = lambdaLev3 / lambdaLev1;
      lambdaRatio[3] = lambdaLev4 / lambdaLev1;
      lambdaRatio[4] = lambdaLev5 / lambdaLev1;
      lambdaRatio[5] = lambdaLev5 / lambdaLev1;
      lambdaRatio[6] = lambdaLev4 / lambdaLev1;
      lambdaRatio[7] = lambdaLev5 / lambdaLev1;
      lambdaRatio[8] = lambdaLev5 / lambdaLev1;
      lambdaRatio[9] = lambdaLev3 / lambdaLev1;
      lambdaRatio[10] = lambdaLev4 / lambdaLev1;
      lambdaRatio[11] = lambdaLev5 / lambdaLev1;
      lambdaRatio[12] = lambdaLev5 / lambdaLev1;
      lambdaRatio[13] = lambdaLev4 / lambdaLev1;
      lambdaRatio[14] = lambdaLev5 / lambdaLev1;
      lambdaRatio[15] = lambdaLev5 / lambdaLev1;
#endif
    }
#endif
    xCalEquaCoeff( encRCSeq, lambdaRatio, equaCoeffA, equaCoeffB, encRCSeq->getGOPSize() );
#if JVET_K0390_RATE_CTRL
    basicLambda = xSolveEqua(encRCSeq, targetBpp, equaCoeffA, equaCoeffB, encRCSeq->getGOPSize());
#else
    basicLambda = xSolveEqua( targetBpp, equaCoeffA, equaCoeffB, encRCSeq->getGOPSize() );
#endif
    encRCSeq->setAllBitRatio( basicLambda, equaCoeffA, equaCoeffB );

    delete []lambdaRatio;
    delete []equaCoeffA;
    delete []equaCoeffB;
  }

  m_picTargetBitInGOP = new Int[numPic];
  Int i;
  Int totalPicRatio = 0;
  Int currPicRatio = 0;
#ifndef CVI_STREAMING_RC
  for ( i=0; i<numPic; i++ )
  {
    totalPicRatio += encRCSeq->getBitRatio( i );
  }
#else
  totalPicRatio = numPic;
#endif

  for ( i=0; i<numPic; i++ )
  {
#ifndef CVI_STREAMING_RC
    currPicRatio = encRCSeq->getBitRatio( i );
#else
    currPicRatio = 1;
#endif
    m_picTargetBitInGOP[i] = (Int)( (targetBits * currPicRatio) / totalPicRatio );
  }

  m_encRCSeq    = encRCSeq;
  m_numPic       = numPic;
  m_targetBits   = targetBits;
  m_picLeft      = m_numPic;
  m_bitsLeft     = m_targetBits;
}

Void TEncRCGOP::xCalEquaCoeff( TEncRCSeq* encRCSeq, RC_Float* lambdaRatio, RC_Float* equaCoeffA, RC_Float* equaCoeffB, Int GOPSize )
{
  for ( Int i=0; i<GOPSize; i++ )
  {
    Int frameLevel = encRCSeq->getGOPID2Level(i);
    RC_Float alpha   = encRCSeq->getPicPara(frameLevel).m_alpha;
    RC_Float beta    = encRCSeq->getPicPara(frameLevel).m_beta;
    equaCoeffA[i] = pow( 1.0/alpha, 1.0/beta ) * pow( lambdaRatio[i], 1.0/beta );
    equaCoeffB[i] = 1.0/beta;
  }
}

#if JVET_K0390_RATE_CTRL
RC_Float TEncRCGOP::xSolveEqua(TEncRCSeq* encRCSeq, RC_Float targetBpp, RC_Float* equaCoeffA, RC_Float* equaCoeffB, Int GOPSize)
#else
RC_Float TEncRCGOP::xSolveEqua( RC_Float targetBpp, RC_Float* equaCoeffA, RC_Float* equaCoeffB, Int GOPSize )
#endif
{
  RC_Float solution = 100.0;
  RC_Float minNumber = 0.1;
  RC_Float maxNumber = 10000.0;
  for ( Int i=0; i<g_RCIterationNum; i++ )
  {
    RC_Float fx = 0.0;
    for ( Int j=0; j<GOPSize; j++ )
    {
#if JVET_K0390_RATE_CTRL
      RC_Float tmpBpp = equaCoeffA[j] * pow(solution, equaCoeffB[j]);
      RC_Float actualBpp = tmpBpp * encRCSeq->getPicPara(encRCSeq->getGOPID2Level(j)).m_validPix / (RC_Float)encRCSeq->getNumPixel();
      fx += actualBpp;
#else
      fx += equaCoeffA[j] * pow( solution, equaCoeffB[j] );
#endif
    }

    if ( fabs( fx - targetBpp ) < 0.000001 )
    {
      break;
    }

    if ( fx > targetBpp )
    {
      minNumber = solution;
      solution = ( solution + maxNumber ) / 2.0;
    }
    else
    {
      maxNumber = solution;
      solution = ( solution + minNumber ) / 2.0;
    }
  }

  solution = Clip3<RC_Float>( 0.1, 10000.0, solution );
  return solution;
}

Void TEncRCGOP::destroy()
{
  m_encRCSeq = NULL;
  if ( m_picTargetBitInGOP != NULL )
  {
    delete[] m_picTargetBitInGOP;
    m_picTargetBitInGOP = NULL;
  }
}

Void TEncRCGOP::updateAfterPicture( Int bitsCost )
{
  m_bitsLeft -= bitsCost;
  m_picLeft--;
}

Int TEncRCGOP::xEstGOPTargetBits( TEncRCSeq* encRCSeq, Int GOPSize )
{
#ifdef CVI_STREAMING_RC
  Int picAvgBit = encRCSeq->getAverageBits();

#if CVI_LOW_LUX_CONTROL
  if (encRCSeq->getUseLowLuxCtrl()){
    if (!encRCSeq->getLowLuxAdaptive()){
      if ( encRCSeq->getLowLuxValue() < 0)
      {
        picAvgBit = picAvgBit * LOW_LUX_CONTROL_STRONG / 10;
      }
      else if( encRCSeq->getLowLuxValue() < 800 )
      {
        picAvgBit = picAvgBit * LOW_LUX_CONTROL_MEDIUM / 10;
      }
      else
      {
        picAvgBit = picAvgBit;
      }
    }
    else{
      if ( encRCSeq->getLowLuxAvg() < 0)
      {
        picAvgBit = picAvgBit * LOW_LUX_CONTROL_STRONG / 10;
        encRCSeq->setLowLuxLevel(2);
      }
      else if( encRCSeq->getLowLuxAvg() < 800 )
      {
        picAvgBit = picAvgBit * LOW_LUX_CONTROL_MEDIUM / 10;
        encRCSeq->setLowLuxLevel(1);
      }
      else
      {
        picAvgBit = picAvgBit;
        encRCSeq->setLowLuxLevel(0);
      }
    }
  }
#endif

  Int picIdxInIPeriod = encRCSeq->getPictureIdx() % encRCSeq->getIntraPeriod();

  Int smoothWinSize = max((Int)encRCSeq->getStatFrameNum() - picIdxInIPeriod + 1, encRCSeq->getFrameRate());
  Int picTargetBit = max(encRCSeq->getMinPicBit(), (Int)(picAvgBit + (encRCSeq->getBitError() / smoothWinSize)));
  // smooth consecutive gop allocated bit
  if(picIdxInIPeriod > 1) {
    picTargetBit = (encRCSeq->getLastGopAvgBit()*25 + picTargetBit*75)/100;
  }
  encRCSeq->getLastGopAvgBit() = picTargetBit;
  Int targetBits = picTargetBit * GOPSize;
#else
  Int realInfluencePicture = min( g_RCSmoothWindowSize, encRCSeq->getFramesLeft() );
  Int averageTargetBitsPerPic = (Int)( encRCSeq->getTargetBits() / encRCSeq->getTotalFrames() );
  Int currentTargetBitsPerPic = (Int)( ( encRCSeq->getBitsLeft() - averageTargetBitsPerPic * (encRCSeq->getFramesLeft() - realInfluencePicture) ) / realInfluencePicture );
  Int targetBits = currentTargetBitsPerPic * GOPSize;
  if ( targetBits < 200 )
  {
    targetBits = 200;   // at least allocate 200 bits for one GOP
  }
#endif
  return targetBits;
}

//picture level
TEncRCPic::TEncRCPic()
{
  m_encRCSeq = NULL;
  m_encRCGOP = NULL;

  m_frameLevel    = 0;
  m_numberOfPixel = 0;
  m_numberOfLCU   = 0;
  m_targetBits    = 0;
  m_estHeaderBits = 0;
  m_estPicQP      = 0;
  m_estPicLambda  = 0.0;

  m_LCULeft       = 0;
  m_bitsLeft      = 0;
  m_pixelsLeft    = 0;

  m_LCUs         = NULL;
  m_picActualHeaderBits = 0;
  m_picActualBits       = 0;
  m_picQP               = 0;
  m_picLambda           = 0.0;
#if JVET_K0390_RATE_CTRL
  m_picMSE = 0.0;
  m_validPixelsInPic = 0;
#endif
}

TEncRCPic::~TEncRCPic()
{
  destroy();
}

Int TEncRCPic::xEstPicTargetBits( TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP )
{
  Int targetBits        = 0;
  Int GOPbitsLeft       = max(0, encRCGOP->getBitsLeft());
  Int currPicPosition = encRCGOP->getNumPic()-encRCGOP->getPicLeft();
#ifndef CVI_STREAMING_RC
  Int currPicRatio    = encRCSeq->getBitRatio( currPicPosition );
  Int totalPicRatio   = 0;
  for ( Int i=currPicPosition; i<encRCGOP->getNumPic(); i++ )
  {
    totalPicRatio += encRCSeq->getBitRatio( i );
  }
#else
  Int totalPicRatio   = encRCGOP->getPicLeft();
  Int currPicRatio    = 1;
#endif

  targetBits  = (GOPbitsLeft * currPicRatio) / totalPicRatio;

  if ( targetBits < encRCSeq->getMinPicBit() )
  {
    targetBits = encRCSeq->getMinPicBit();   // at least allocate 100 bits for one picture
  }

#ifndef CVI_STREAMING_RC
  if ( m_encRCSeq->getFramesLeft() > 16 )
#endif
  {
    targetBits = (g_RCWeightPicTargetBitInBuffer * targetBits + g_RCWeightPicTargetBitInGOP * m_encRCGOP->getTargetBitInGOP(currPicPosition))/100;
  }

  return targetBits;
}

Int TEncRCPic::xEstPicHeaderBits( list<TEncRCPic*>& listPreviousPictures, Int frameLevel )
{
  Int numPreviousPics   = 0;
  Int totalPreviousBits = 0;

  list<TEncRCPic*>::iterator it;
  for ( it = listPreviousPictures.begin(); it != listPreviousPictures.end(); it++ )
  {
    if ( (*it)->getFrameLevel() == frameLevel )
    {
      totalPreviousBits += (*it)->getPicActualHeaderBits();
      numPreviousPics++;
    }
  }

  Int estHeaderBits = 0;
  if ( numPreviousPics > 0 )
  {
    estHeaderBits = totalPreviousBits / numPreviousPics;
  }

  return estHeaderBits;
}

Int TEncRCPic::xEstPicLowerBound(TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP)
{
#ifdef CVI_STREAMING_RC
  return encRCSeq->getMinPicBit();
#endif
  Int lowerBound = 0;
  Int GOPbitsLeft = encRCGOP->getBitsLeft();

  const Int nextPicPosition = (encRCGOP->getNumPic() - encRCGOP->getPicLeft() + 1) % encRCGOP->getNumPic();
  const Int nextPicRatio = encRCSeq->getBitRatio(nextPicPosition);

  Int totalPicRatio = 0;
  for (Int i = nextPicPosition; i < encRCGOP->getNumPic(); i++)
  {
    totalPicRatio += encRCSeq->getBitRatio(i);
  }

  if (nextPicPosition == 0)
  {
    GOPbitsLeft = encRCGOP->getTargetBits();
  }
  else
  {
    GOPbitsLeft -= m_targetBits;
  }

  lowerBound = Int(GOPbitsLeft * nextPicRatio) / totalPicRatio;

  if (lowerBound < 100)
  {
    lowerBound = 100;   // at least allocate 100 bits for one picture
  }

#ifndef CVI_STREAMING_RC
  if ( m_encRCSeq->getFramesLeft() > 16 )
#endif
  {
    lowerBound = (g_RCWeightPicTargetBitInBuffer * lowerBound + g_RCWeightPicTargetBitInGOP * m_encRCGOP->getTargetBitInGOP(nextPicPosition))/100;
  }

  return lowerBound;
}

Void TEncRCPic::addToPictureLsit( list<TEncRCPic*>& listPreviousPictures )
{
  if ( listPreviousPictures.size() > g_RCMaxPicListSize )
  {
    TEncRCPic* p = listPreviousPictures.front();
    listPreviousPictures.pop_front();
    p->destroy();
    delete p;
  }

  listPreviousPictures.push_back( this );
}

#if CVI_LOW_LUX_CONTROL
Int TEncRCPic::calc_avg_lowlux()
{
  int winSize = MAX_LOW_LUX_WIN;
  assert( winSize > 0 );
  // trimmed average
  int accum = 0, idx = 0;
  for (idx = 0; idx < winSize; idx++) {
    accum += m_encRCSeq->getLowLuxWindow(idx % winSize);
  }
  return Clip3(MINIMUM_LOW_LUX_VALUE, MAXIMUX_LOW_LUX_VALUE, (accum / winSize));
}

Void TEncRCPic::cviEnc_LowLux_PicCtrl(TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP)
{
  m_encRCSeq = encRCSeq;
  m_encRCGOP = encRCGOP;
  Int picLowLuxValue=LOW_LUX_DEFAULT;//needs to achieve from  ISP
  picLowLuxValue = Clip3(MINIMUM_LOW_LUX_VALUE, MAXIMUX_LOW_LUX_VALUE, picLowLuxValue);

  m_encRCSeq->setLowLuxWindow(picLowLuxValue, m_encRCSeq->getPictureIdx() % MAX_LOW_LUX_WIN);
  Int LowLuxAvg= calc_avg_lowlux();
  m_encRCSeq->setLowLuxAvg(LowLuxAvg); 
}
#endif

Void TEncRCPic::create( TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP, Int frameLevel, list<TEncRCPic*>& listPreviousPictures )
{
  destroy();
  m_encRCSeq = encRCSeq;
  m_encRCGOP = encRCGOP;

  Int targetBits    = xEstPicTargetBits( encRCSeq, encRCGOP );
#ifndef CVI_STREAMING_RC
  Int estHeaderBits = xEstPicHeaderBits( listPreviousPictures, frameLevel );
#else
  Int estHeaderBits = 0;
#endif
  if ( targetBits < estHeaderBits + encRCSeq->getMinPicBit() )
  {
    targetBits = estHeaderBits + encRCSeq->getMinPicBit();   // at least allocate 100 bits for picture data
  }

#if CVI_LOW_LUX_CONTROL
  if (m_encRCSeq->getUseLowLuxCtrl()&& encRCSeq->getLowLuxAdaptive()) {
    cviEnc_LowLux_PicCtrl(encRCSeq,encRCGOP);
  }
#endif

  m_frameLevel       = frameLevel;
  m_numberOfPixel    = encRCSeq->getNumPixel();
  m_numberOfPixelAlign32 = encRCSeq->getNumPixelAlign32();
  m_numberOfLCU      = encRCSeq->getNumberOfLCU();
  m_estPicLambda     = 100.0;
  m_targetBits       = targetBits;
  m_estHeaderBits    = estHeaderBits;
  m_bitsLeft         = m_targetBits;
  Int picWidth       = encRCSeq->getPicWidth();
  Int picHeight      = encRCSeq->getPicHeight();
  Int LCUWidth       = encRCSeq->getLCUWidth();
  Int LCUHeight      = encRCSeq->getLCUHeight();
  Int picWidthInLCU  = ( picWidth  % LCUWidth  ) == 0 ? picWidth  / LCUWidth  : picWidth  / LCUWidth  + 1;
  Int picHeightInLCU = ( picHeight % LCUHeight ) == 0 ? picHeight / LCUHeight : picHeight / LCUHeight + 1;
  m_lowerBound       = xEstPicLowerBound( encRCSeq, encRCGOP );

  m_LCULeft         = m_numberOfLCU;
  m_bitsLeft       -= m_estHeaderBits;
  m_pixelsLeft      = m_numberOfPixel;

  m_numOfPelInCTU = LCUWidth * LCUHeight;
  m_ctuNumInRow = picWidthInLCU;
  m_numOfCTURow = picHeightInLCU;

  m_LCUs           = new TRCLCU[m_numberOfLCU];
  Int i, j;
  Int LCUIdx;
  for ( i=0; i<picWidthInLCU; i++ )
  {
    for ( j=0; j<picHeightInLCU; j++ )
    {
      LCUIdx = j*picWidthInLCU + i;
      m_LCUs[LCUIdx].m_actualBits = 0;
#if JVET_K0390_RATE_CTRL
      m_LCUs[LCUIdx].m_actualSSE = 0.0;
      m_LCUs[LCUIdx].m_actualMSE = 0.0;
#endif
      m_LCUs[LCUIdx].m_QP         = 0;
      m_LCUs[LCUIdx].m_lambda     = 0.0;
      m_LCUs[LCUIdx].m_targetBits = 0;
      m_LCUs[LCUIdx].m_bitWeight  = 1.0;
      Int currWidth  = ( (i == picWidthInLCU -1) ? picWidth  - LCUWidth *(picWidthInLCU -1) : LCUWidth  );
      Int currHeight = ( (j == picHeightInLCU-1) ? picHeight - LCUHeight*(picHeightInLCU-1) : LCUHeight );
      m_LCUs[LCUIdx].m_numberOfPixel = currWidth * currHeight;
    }
  }
  m_picActualHeaderBits = 0;
  m_picActualBits       = 0;
  m_picQP               = 0;
  m_picLambda           = 0.0;

  m_numberOfSkipPixel = 0;
}

Void TEncRCPic::destroy()
{
  if( m_LCUs != NULL )
  {
    delete[] m_LCUs;
    m_LCUs = NULL;
  }
  m_encRCSeq = NULL;
  m_encRCGOP = NULL;
}

RC_Float TEncRCPic::getPicTextCplx()
{
#ifdef CVI_HW_RC
#if SOFT_FLOAT
  Soft_Float picAvgTc_delay  =  Float_to_SoftFloat(g_picAvgTc_delay);
  return SoftFloat_to_Float(CVI_FLOAT_POW(CVI_FLOAT_MAX(picAvgTc_delay, FLOAT_VAL_1), SOFT_FLOAT_BETA));
#else
  return pow((RC_Float)g_picAvgTc_delay, (RC_Float)BETA1);
#endif
#else
  return pow(m_totalCostIntra/(RC_Float)m_numberOfPixel, BETA1);
#endif
}

RC_Float TEncRCPic::estimatePicLambda( list<TEncRCPic*>& listPreviousPictures, SliceType eSliceType)
{
  RC_Float alpha         = m_encRCSeq->getPicPara( m_frameLevel ).m_alpha;
  RC_Float beta          = m_encRCSeq->getPicPara( m_frameLevel ).m_beta;
  RC_Float bpp       = m_targetBits/(RC_Float)m_numberOfPixel;

#if JVET_K0390_RATE_CTRL
  Int lastPicValPix = 0;
  if (listPreviousPictures.size() > 0)
  {
    lastPicValPix = m_encRCSeq->getPicPara(m_frameLevel).m_validPix;
  }
  if (lastPicValPix > 0)
  {
    bpp = m_targetBits / (RC_Float)lastPicValPix;
  }
#endif

  RC_Float estLambda;
 if (eSliceType == I_SLICE)
  {
#if SOFT_FLOAT
    estLambda = calculateLambdaIntra(Float_to_SoftFloat(alpha), Float_to_SoftFloat(beta), Float_to_SoftFloat(getPicTextCplx()), Float_to_SoftFloat(bpp));
#else
    estLambda = calculateLambdaIntra(alpha, beta, getPicTextCplx(), bpp);
#endif
  }
  else
  {
#if SOFT_FLOAT
    estLambda = SoftFloat_to_Float(CVI_FLOAT_MUL(Float_to_SoftFloat(alpha), CVI_FLOAT_POW(Float_to_SoftFloat(bpp),Float_to_SoftFloat(beta))));
#else
    estLambda = alpha * pow( bpp, beta );
#endif
  }

  RC_Float lastLevelLambda = -1.0;
  RC_Float lastPicLambda   = -1.0;
  RC_Float lastValidLambda = -1.0;
  list<TEncRCPic*>::iterator it;
  for ( it = listPreviousPictures.begin(); it != listPreviousPictures.end(); it++ )
  {
    if ( (*it)->getFrameLevel() == m_frameLevel )
    {
      lastLevelLambda = (*it)->getPicActualLambda();
    }
    lastPicLambda     = (*it)->getPicActualLambda();

    if ( lastPicLambda > 0.0 )
    {
      lastValidLambda = lastPicLambda;
    }
  }

#ifdef CVI_STREAMING_RC
  if(eSliceType == I_SLICE) {
    lastLevelLambda = (getRCSequence()->getPictureIdx()>getRCSequence()->getIntraPeriod())
                    ? getRCSequence()->getLastILambda()
                    : -1;
  }
  Bool isFirstPPic = getRCSequence()->isFirstPPic();
#if SOFT_FLOAT
  RC_Float lastNormalLambdaScaleLow = SoftFloat_to_Float(CVI_FLOAT_POW(INT_TO_CVI_FLOAT(2), CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(-10) , INT_TO_CVI_FLOAT(3))));
  RC_Float lastNormalLambdaScaleHigh = SoftFloat_to_Float(CVI_FLOAT_POW(INT_TO_CVI_FLOAT(2), CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(10) , INT_TO_CVI_FLOAT(3))));
#endif
  if ( lastLevelLambda > 0.0 && !isFirstPPic)
  {
    estLambda = Clip3<RC_Float>( lastLevelLambda * (pow( 2.0, -g_RCLevelPicQpClip/3.0 )), lastLevelLambda * (pow( 2.0, g_RCLevelPicQpClip/3.0 )), estLambda );
  }
#if SOFT_FLOAT
 if( lastPicLambda > 0.0 && isFirstPPic)
  {
    estLambda = Clip3<RC_Float>( lastPicLambda * pow( 2.0, -g_RCLastPicQpClip/3.0 ), lastPicLambda * lastNormalLambdaScaleHigh, estLambda );
  }
  else if ( lastPicLambda > 0.0 && eSliceType != I_SLICE)
  {
    estLambda = Clip3<RC_Float>( lastPicLambda * pow( 2.0, -g_RCLastPicQpClip/3.0 ), lastPicLambda * pow( 2.0, g_RCLastPicQpClip/3.0 ), estLambda );
  }  
  else if ( lastValidLambda > 0.0 )
  {
    estLambda = Clip3<RC_Float>( lastValidLambda * lastNormalLambdaScaleLow, lastValidLambda * lastNormalLambdaScaleHigh, estLambda );
  }
#else

  if( lastPicLambda > 0.0 && isFirstPPic)
  {
    estLambda = Clip3<RC_Float>( lastPicLambda * pow( RC_Float(2.0), RC_Float(-g_RCLastPicQpClip)/RC_Float(3.0) ), lastPicLambda * pow( RC_Float(2.0), RC_Float(10)/RC_Float(3.0) ), estLambda );
  }
  else if ( lastPicLambda > 0.0 && eSliceType != I_SLICE)
  {
    estLambda = Clip3<RC_Float>( lastPicLambda * pow( RC_Float(2.0), RC_Float(-g_RCLastPicQpClip)/RC_Float(3.0) ), lastPicLambda * pow(RC_Float(2.0), RC_Float(g_RCLastPicQpClip) / RC_Float(3.0) ), estLambda );
  }
  else if ( lastValidLambda > 0.0 )
  {
    estLambda = Clip3<RC_Float>( lastValidLambda * pow(RC_Float(2.0), RC_Float(-10.0)/RC_Float(3.0)), lastValidLambda * pow(RC_Float(2.0), RC_Float(10.0)/RC_Float(3.0)), estLambda );
  }

#endif  
  Int maxQp = (eSliceType == I_SLICE) ? g_RCMaxIQp : g_RCMaxQp;
  Int minQp = (eSliceType == I_SLICE) ? g_RCMinIQp : g_RCMinQp;
  estLambda = Clip3(QpToLambda(minQp), QpToLambda(maxQp), estLambda);
#else
  if ( lastLevelLambda > 0.0 )
  {
    lastLevelLambda = Clip3( 0.1, 10000.0, lastLevelLambda );
    estLambda = Clip3( lastLevelLambda * pow( 2.0, -3/3.0 ), lastLevelLambda * pow( 2.0, 3/3.0 ), estLambda );
  }
  if ( lastPicLambda > 0.0 && eSliceType != I_SLICE)
  {
    lastPicLambda = Clip3( 0.1, 2000.0, lastPicLambda );
    estLambda = Clip3( lastPicLambda * pow( 2.0, -3/3.0 ), lastPicLambda * pow( 2.0, 3/3.0 ), estLambda );
  }
  else if ( lastValidLambda > 0.0 )
  {
    lastValidLambda = Clip3( 0.1, 2000.0, lastValidLambda );
    estLambda = Clip3( lastValidLambda * pow(2.0, -10.0/3.0), lastValidLambda * pow(2.0, 10.0/3.0), estLambda );
  }
  else
  {
    estLambda = Clip3( 0.1, 10000.0, estLambda );
  }
  if ( estLambda < 0.1 )
  {
    estLambda = 0.1;
  }
#endif
#ifndef CVI_STREAMING_RC
#if JVET_K0390_RATE_CTRL
  //Avoid different results in different platforms. The problem is caused by the different results of pow() in different platforms.
  estLambda = RC_Float(int64_t(estLambda * (RC_Float)LAMBDA_PREC + 0.5)) / (RC_Float)LAMBDA_PREC;
#endif
#endif
  m_estPicLambda = estLambda;
  RC_Float totalWeight = 0.0;
  // initial BU bit allocation weight
  for ( Int i=0; i<m_numberOfLCU; i++ )
  {
    RC_Float alphaLCU, betaLCU;
    if ( m_encRCSeq->getUseLCUSeparateModel() )
    {
      alphaLCU = m_encRCSeq->getLCUPara( m_frameLevel, i ).m_alpha;
      betaLCU  = m_encRCSeq->getLCUPara( m_frameLevel, i ).m_beta;
    }
    else
    {
      alphaLCU = m_encRCSeq->getPicPara( m_frameLevel ).m_alpha;
      betaLCU  = m_encRCSeq->getPicPara( m_frameLevel ).m_beta;
    }

    m_LCUs[i].m_bitWeight =  m_LCUs[i].m_numberOfPixel * pow( estLambda/alphaLCU, 1.0/betaLCU );

    if ( m_LCUs[i].m_bitWeight < 0.01 )
    {
      m_LCUs[i].m_bitWeight = 0.01;
    }
    totalWeight += m_LCUs[i].m_bitWeight;
  }
  for ( Int i=0; i<m_numberOfLCU; i++ )
  {
    RC_Float BUTargetBits = m_targetBits * m_LCUs[i].m_bitWeight / totalWeight;
    m_LCUs[i].m_bitWeight = BUTargetBits;
  }

  return estLambda;
}

Int TEncRCPic::estimatePicQP( RC_Float lambda, list<TEncRCPic*>& listPreviousPictures)
{
  Int QP = LambdaToQp(lambda);
  Int lastLevelQP = g_RCInvalidQPValue;
  Int lastPicQP   = g_RCInvalidQPValue;
  Int lastValidQP = g_RCInvalidQPValue;
  list<TEncRCPic*>::iterator it;
  for ( it = listPreviousPictures.begin(); it != listPreviousPictures.end(); it++ )
  {
    if ( (*it)->getFrameLevel() == m_frameLevel )
    {
      lastLevelQP = (*it)->getPicActualQP();
    }
    lastPicQP = (*it)->getPicActualQP();
    if ( lastPicQP > g_RCInvalidQPValue )
    {
      lastValidQP= lastPicQP;
    }
  }

  SliceType eSliceType = getRCSequence()->getSliceType();
#ifdef CVI_STREAMING_RC
  if(eSliceType == I_SLICE) {
    lastLevelQP = (getRCSequence()->getPictureIdx()>getRCSequence()->getIntraPeriod())
                ? getRCSequence()->getLastIQp()
                : g_RCInvalidQPValue;
  }
  Bool isFirstPPic = getRCSequence()->isFirstPPic();
  if ( lastLevelQP > g_RCInvalidQPValue && !isFirstPPic)
  {
    QP = Clip3( lastLevelQP - g_RCLevelPicQpClip, lastLevelQP + g_RCLevelPicQpClip, QP );
  }
  if( lastPicQP > g_RCInvalidQPValue && isFirstPPic)
  {
    QP = Clip3( lastPicQP - g_RCLastPicQpClip, lastPicQP + 10, QP );
  }
  else if( lastPicQP > g_RCInvalidQPValue && eSliceType != I_SLICE)
  {
    QP = Clip3( lastPicQP - g_RCLastPicQpClip, lastPicQP + g_RCLastPicQpClip, QP );
  }
  else if( lastValidQP > g_RCInvalidQPValue )
  {
    QP = Clip3( lastValidQP - 10, lastValidQP + 10, QP );
  }
  Int maxQp = (eSliceType == I_SLICE) ? g_RCMaxIQp : g_RCMaxQp;
  Int minQp = (eSliceType == I_SLICE) ? g_RCMinIQp : g_RCMinQp;
  QP = Clip3( minQp, maxQp, QP );
#else
  if ( lastLevelQP > g_RCInvalidQPValue )
  {
    QP = Clip3( lastLevelQP - 3, lastLevelQP + 3, QP );
  }

  if( lastPicQP > g_RCInvalidQPValue && eSliceType != I_SLICE)
  {
    QP = Clip3( lastPicQP - 3, lastPicQP + 3, QP );
  }
  else if( lastValidQP > g_RCInvalidQPValue )
  {
    QP = Clip3( lastValidQP - 10, lastValidQP + 10, QP );
  }
#endif
  return QP;
}

#ifdef CVI_HW_RC
void TEncRCPic::fillHwQpLut (SliceType eSliceType, Int qpOffset)
{
  RC_Float alpha = m_encRCSeq->getPicPara(m_frameLevel).m_alpha;
  RC_Float beta = m_encRCSeq->getPicPara(m_frameLevel).m_beta;
  RC_Float bpp = (eSliceType==I_SLICE)
              ? intraLambdaToBpp(m_estPicLambda, alpha, beta)
              : lambdaToBpp(m_estPicLambda, alpha, beta);
  Int centerQp = LambdaToQp(m_estPicLambda);
  Int centerEntry = g_row_q_lut.get_center_idx();
  Int iterNum = g_row_q_lut.get_entry_num() >> 1;
  RC_Float upperLambda = m_estPicLambda, lowerLambda = m_estPicLambda;
#if SOFT_FLOAT  
  RC_Float lambdaStepScale = SoftFloat_to_Float(FLOAT_VAL_LAMBDA_STEP_SCALE);
#else
  RC_Float lambdaStepScale = exp(1/4.2005);
#endif  
  for(Int iter=0; iter<iterNum; iter++)
  {
   Int upperEntry = centerEntry + iter + 1;
   Int lowerEntry = centerEntry - iter - 1;
   upperLambda *= lambdaStepScale;
   lowerLambda /= lambdaStepScale;
   RC_Float upperBpp = (eSliceType==I_SLICE)
                  ? intraLambdaToBpp(upperLambda, alpha, beta)
                  : lambdaToBpp(upperLambda, alpha, beta);
   RC_Float lowerBpp = (eSliceType==I_SLICE)
                  ? intraLambdaToBpp(lowerLambda, alpha, beta)
                  : lambdaToBpp(lowerLambda, alpha, beta);
   g_row_q_lut.set_input_entry(upperEntry, Clip3(lowerEntry, (1<<ROW_Q_LUT_IN_BD)-1, (Int)(upperBpp*m_numOfPelInCTU)));
   g_row_q_lut.set_input_entry(lowerEntry, Clip3(upperEntry, (1<<ROW_Q_LUT_IN_BD)-1, (Int)(lowerBpp*m_numOfPelInCTU)));
   g_row_q_lut.set_output_entry(upperEntry, Clip3(0, 51, LambdaToQp(upperLambda) - qpOffset));
   g_row_q_lut.set_output_entry(lowerEntry, Clip3(0, 51, LambdaToQp(lowerLambda) - qpOffset));
  }
  g_row_q_lut.set_input_entry(centerEntry, Clip3(centerEntry,  (1<<ROW_Q_LUT_IN_BD)-1, (Int)(bpp*m_numOfPelInCTU)));
  g_row_q_lut.set_output_entry(centerEntry, Clip3(0, 51, centerQp - qpOffset));

  // qp to lambda table
  RC_Float lambdaScale = m_estPicLambda/QpToLambda(centerQp);
  for (Int qp = 0; qp < 52; qp++)
  {
    if (qp >= m_blkMinQp && qp <= m_blkMaxQp)
    {
      RC_Float lambda = lambdaScale*QpToLambda(qp);
#if SOFT_FLOAT
      RC_Float sqrtLambda = SoftFloat_to_Float(CVI_FLOAT_POW(Float_to_SoftFloat(lambda), FLOAT_VAL_p5));
#else
      RC_Float sqrtLambda = pow(lambda, 0.5);
#endif
      g_qp_to_lambda_table[qp] = lambda;
      g_qp_to_sqrt_lambda_table[qp] = sqrtLambda;
    }
    else
    {
      g_qp_to_lambda_table[qp] = 0;
      g_qp_to_sqrt_lambda_table[qp] = 0;
    }
  }
#ifdef CVI_RANDOM_ENCODE
  if (g_is_cvi_rand_rc_en)
  {
    int max_bits = (1<<ROW_Q_LUT_IN_BD)-1;
    for(Int iter=0; iter<g_row_q_lut.get_entry_num(); iter++)
    {
      Int random_bits = cvi_random_range(0, max_bits);
      g_row_q_lut.set_input_entry(iter, random_bits);

      Int random_qp = cvi_random_range(0, 51);
      g_row_q_lut.set_output_entry(iter, random_qp);
    }

    int frac = (1 << LAMBDA_FRAC_BIT);
    int sqrt_frac = (1 << LC_LAMBDA_FRAC_BIT);
    for (int i = 0; i < QP_NUM; i++)
    {
      double lambda = cvi_random_range(0, 65535);
      g_qp_to_lambda_table_rand[i] = lambda / frac;

      double sqrt_lambda = cvi_random_range(0, 32767);
      g_qp_to_sqrt_lambda_table_rand[i] = sqrt_lambda / sqrt_frac;
    }

    // replace qp to lambda tables by random values.
    memcpy(g_qp_to_lambda_table, g_qp_to_lambda_table_rand, sizeof(g_qp_to_lambda_table));
    memcpy(g_qp_to_sqrt_lambda_table, g_qp_to_sqrt_lambda_table_rand, sizeof(g_qp_to_sqrt_lambda_table));
  }
#endif
#if RC_DEBUG
  if(g_RCDebugLogEn) {
    printf("r-q lut\n");
    for(Int entry_i=0; entry_i<g_row_q_lut.get_entry_num(); entry_i++) {
      printf("%4d %d\n", g_row_q_lut.get_input_entry(entry_i) , g_row_q_lut.get_output_entry(entry_i));
    }
    printf("lambda scale %.2f \n", lambdaScale);
  }
#endif
}

void TEncRCPic::setupHwRcParam(SliceType eSliceType, bool isFirstFrame)
{
  // re-allocate picture target bitrate
  RC_Float alpha = m_encRCSeq->getPicPara(m_frameLevel).m_alpha;
  RC_Float beta = m_encRCSeq->getPicPara(m_frameLevel).m_beta;
  Int targetBitByModel = ((eSliceType==I_SLICE)
                        ? intraLambdaToBpp(m_estPicLambda, alpha, beta)
                        : lambdaToBpp(m_estPicLambda, alpha, beta)) * m_numberOfPixel;
  m_targetBits = max((m_targetBits + targetBitByModel)/2, 0);
  m_ctuRowAvgBit = min(m_targetBits/m_numOfCTURow, (1<<ROW_AVG_BIT_BD)-1);
  // ctu-row q LUT compensation
  Int picAvgQpDelta = 0, lastPicAvgQpDelta = 0;
  if(!isFirstFrame) {
    lastPicAvgQpDelta = g_blk_delta_qp_accum / (m_numberOfPixel>>8);
    picAvgQpDelta = lastPicAvgQpDelta;
  }
  m_blkMaxQp = (eSliceType == I_SLICE) ? g_RCMaxIQp : g_RCMaxQp;
  m_blkMinQp = (eSliceType == I_SLICE) ? g_RCMinIQp : g_RCMinQp;
  fillHwQpLut(eSliceType, picAvgQpDelta);

  g_CtuRowNormScale = (1<<RC_NORM_SCALE_BD) / m_ctuNumInRow;
  assert(g_CtuRowNormScale < (1<<8));
  // bit error compensation scale adjustment by average mad
  g_bit_err_compen_scal = (1<<RC_NORM_SCALE_BD) / max(1, g_bit_err_smooth_factor);
  assert(g_bit_err_compen_scal < (1<<9));
#if RC_DEBUG
  if(g_RCDebugLogEn) {
    printf("picAvgQpDelta last %d cur %d \n", lastPicAvgQpDelta, picAvgQpDelta);
    printf("bit_err_compen_scal %d\n",  g_bit_err_compen_scal);
  }
#endif

}

void TEncRCPic::hw_picInit()
{
  // HW variable init
  Int center_idx = g_row_q_lut.get_center_idx();
  m_rowBaseQp = g_row_q_lut.get_output_entry(center_idx);
  m_ctuRowAccumBit = g_row_q_lut.get_input_entry(center_idx);
  m_ctuRowAccumBit_delay = m_ctuRowAccumBit;
  g_bit_error_accum = 0;
  g_blk_delta_qp_accum = 0;
  m_isRowOverflow = false;
  m_isRowOverflow_delay = false;
}

Void TEncRCPic::hw_calcCTUOvfTargetBit()
{
  m_isRowOverflow = false;
  Int bitErrCompensate = (g_bit_error_accum*g_bit_err_compen_scal) >> RC_NORM_SCALE_BD;
  m_ctuRowOvfTargetBit = Clip3(0, MAX_CTU_ROW_BIT, m_ctuRowAvgBit + bitErrCompensate);
}

Int TEncRCPic::hw_getCTULowTargetBit(Int rowIdx)
{
  if (rowIdx == 0)
    hw_calcCTUOvfTargetBit();

  m_ctuRowTargetBit = m_ctuRowOvfTargetBit;
  Int ctuTargetBit = min(MAX_CTU_BIT, (m_ctuRowTargetBit * g_CtuRowNormScale) >> RC_NORM_SCALE_BD);
#if RC_DEBUG
  if(g_RCDebugLogEn) {
    printf("row %2d %5d %4d", rowIdx, g_bit_error_accum, ctuTargetBit);
  }
#endif
  return ctuTargetBit;
}

void TEncRCPic::hw_calcCTULowBaseQp(Int targetBit)
{
  Int lastRowBaseQp = m_rowBaseQp;
  Int baseQp = g_row_q_lut.lut_lookup(targetBit);
  m_rowBaseQp = Clip3(lastRowBaseQp-g_RowQpClip, lastRowBaseQp+g_RowQpClip, baseQp);
#if RC_DEBUG
  if(g_RCDebugLogEn) {
    printf(" q %d %d\n", baseQp, m_rowBaseQp);
  }
#endif
}

Int TEncRCPic::hw_getCTUBaseQp()
{
#ifdef SIG_CCU
  if (g_sigdump.ccu && g_sigpool.p_ccu_rc_st)
  {
    g_sigpool.p_ccu_rc_st->is_row_ovf          = m_isRowOverflow_delay;
    g_sigpool.p_ccu_rc_st->ctu_row_target_bits = m_ctuRowTargetBit;
    g_sigpool.p_ccu_rc_st->ctu_row_acc_bits    = m_ctuRowAccumBit_delay;
  }
#endif //~SIG_CCU
  return m_rowBaseQp + ((m_isRowOverflow_delay) ? g_InRowOverflowQpdelta : 0);
}

Int TEncRCPic::hw_blockQpDecision(Int baseQp, Int blkPosX, Int blkPosY) {
  Int blkMadi = g_blk_madi.get_data(blkPosX, blkPosY);
  Int tc_qp_delta = g_tc_lut.lut_lookup(blkMadi);

#ifdef CVI_FOREGROUND_QP
  // Apply foreground tc table
  if (g_algo_cfg.CostPenaltyCfg.EnableForeground)
  {
    if (get_foreground_qp_valid())
    {
      calcRcForegroundMap(blkPosX, blkPosY, 16);

      if (g_algo_cfg.CostPenaltyCfg.EnableForegroundTc)
      {
        int tc_delta_ori = tc_qp_delta;
        double fg_map = g_foreground_map[0].get_data(blkPosX, blkPosY);

        if (g_algo_cfg.CostPenaltyCfg.EnableForegroundHardTh)
        {
          if (fg_map > 0)
            tc_qp_delta = g_fg_tc_lut.lut_lookup(blkMadi);
        }
        else
        {
          int fg_tc_qp_delta = g_fg_tc_lut.lut_lookup(blkMadi);
          tc_qp_delta = tc_qp_delta + ((fg_tc_qp_delta - tc_qp_delta) * fg_map);
        }
        g_fg_dqp_map.set_data(blkPosX, blkPosY, tc_delta_ori - tc_qp_delta);  // for evaluation
      }
    }
  }
#endif //~CVI_FOREGROUND_QP

  Int blkLum = g_blk_lum.get_data(blkPosX, blkPosY);
  Int lum_qp_delta = g_lum_lut.lut_lookup(blkLum);
  Int blk_qp = baseQp + tc_qp_delta + lum_qp_delta;

#ifdef CVI_QP_MAP
  Int qp_map_delta = 0;
  int blk_qp_ori = blk_qp;

#ifdef CVI_FOREGROUND_QP
  if (g_algo_cfg.CostPenaltyCfg.EnableForeground && g_algo_cfg.CostPenaltyCfg.EnableForegroundDQp)
  {
    if (get_foreground_qp_valid())
    {
      int fg_delta_qp = calcForegroundQpDelta(blkPosX, blkPosY, 16);
      blk_qp = blk_qp + fg_delta_qp;
      qp_map_delta = fg_delta_qp;
    }
  }
#endif //~CVI_FOREGROUND_QP

  int pic_w = getRCSequence()->getPicWidth();
  int pic_h = getRCSequence()->getPicHeight();

  if (isQpMapEnable() && (blkPosX < pic_w) && (blkPosY < pic_h))
  {
    int skip = 0;
    QP_MAP_DELTA_MODE delta_mode = QP_MAP_RELATIVE;
    int map_qp = 0;
    getQpMapBlk16(blkPosX, blkPosY, &skip, &delta_mode, &map_qp);

    if (delta_mode == QP_MAP_RELATIVE)
    {
      blk_qp = blk_qp + map_qp;
      qp_map_delta = map_qp;
    }
    else
    {
      blk_qp = map_qp;
      qp_map_delta = blk_qp - blk_qp_ori;
    }
  }
#endif //~CVI_QP_MAP

  blk_qp = Clip3(m_blkMinQp, m_blkMaxQp, blk_qp);

#ifdef CVI_SMART_ENC
  if (isEnableSmartEncode())
  {
    int skip = 0;
    QP_MAP_DELTA_MODE delta_mode = QP_MAP_RELATIVE;
    int map_qp = 0;

    if ((blkPosX < pic_w) && (blkPosY < pic_h))
    {
      getQpMapBlk16(blkPosX, blkPosY, &skip, &delta_mode, &map_qp);
    }
    else
    {
      map_qp = Clip3(g_clip_delta_qp_min, g_clip_delta_qp_max, map_qp);
    }

    blk_qp = blk_qp + map_qp;
    qp_map_delta = map_qp;

    blk_qp = Clip3(m_blkMinQp, m_blkMaxQp, blk_qp);
  }
#endif //~CVI_SMART_ENC

  g_blk_delta_qp_accum = Clip3(MIN_BLK_DQP_ACCUM, MAX_BLK_DQP_ACCUM,
                               g_blk_delta_qp_accum+tc_qp_delta+lum_qp_delta+qp_map_delta);

#ifdef SIG_CCU
  if (g_sigdump.ccu)
  {
    sig_ccu_record_rc_blk16(blkPosX, blkPosY, tc_qp_delta, lum_qp_delta, blk_qp);
  }
#endif //~SIG_CCU
  return blk_qp;
}

Void TEncRCPic::genAllCUQp(Int baseQp, Int ctu_posX, Int ctu_posY)
{
#ifdef SIG_CCU
  if (g_sigdump.ccu)
  {
    g_sigpool.p_ccu_rc_st->ctu_x = ctu_posX;
    g_sigpool.p_ccu_rc_st->ctu_y = ctu_posY;
    g_sigpool.p_ccu_rc_st->ctu_qp = baseQp;
    g_sigpool.p_ccu_rc_st->row_qp = m_rowBaseQp;
  }
#endif //~SIG_CCU

  Int idxToX[4] = {0, 1, 0, 1}, idxToY[4] = {0, 0, 1, 1};
  for(Int cu32_i=0; cu32_i<4; cu32_i++) {
    Int cu32_x = idxToX[cu32_i]<<5;
    Int cu32_y = idxToY[cu32_i]<<5;
    if((ctu_posX+cu32_x)>=getRCSequence()->getPicWidth() ||
       (ctu_posY+cu32_y)>=getRCSequence()->getPicHeight()) {
      continue;
    }
    Int blk_qp_accum = 0, blk_qp_min = MAX_INT, blk_qp_max = -1;
    Int blk_lambda_accum = 0, blk_sqrt_lambda_accum = 0;
    for(Int blk_i=0; blk_i<4; blk_i++) {
      Int blk_x = cu32_x + (idxToX[blk_i]<<4);
      Int blk_y = cu32_y + (idxToY[blk_i]<<4);
      Int blk_qp = hw_blockQpDecision(baseQp, blk_x+ctu_posX, blk_y+ctu_posY);
      g_blk_qp.set_data(blk_x, blk_y, blk_qp);

      // CU32 Qp/Lambda aggregatation
      blk_qp_accum += blk_qp;
      blk_lambda_accum += (Int)(g_qp_to_lambda_table[blk_qp]*(1<<LAMBDA_FRAC_BIT));
      blk_sqrt_lambda_accum += (Int)(g_qp_to_sqrt_lambda_table[blk_qp]*(1<<LC_LAMBDA_FRAC_BIT));
      if(blk_qp < blk_qp_min) {
        blk_qp_min = blk_qp;
      }
      if(blk_qp > blk_qp_max) {
        blk_qp_max = blk_qp;
      }
    }
    Int cu32_qp = (g_qp_mode==AVG)
                ? (blk_qp_accum + 2) >> 2
                : (g_qp_mode==MIN)
                  ? blk_qp_min : blk_qp_max;
    g_cu32_qp.set_data(cu32_x, cu32_y, cu32_qp);
    g_cu32_lambda.set_data(cu32_x, cu32_y, (blk_lambda_accum>>2)/(RC_Float)(1<<LAMBDA_FRAC_BIT));
    g_cu32_sqrt_lambda.set_data(cu32_x, cu32_y, (blk_sqrt_lambda_accum>>2)/(RC_Float)(1<<LC_LAMBDA_FRAC_BIT));

#ifdef SIG_CCU
    if (g_sigdump.ccu)
    {
      if (g_sigpool.p_ccu_rc_st)
        g_sigpool.p_ccu_rc_st->cu32_qp[(g_sigpool.p_ccu_rc_st->blk16_idx - 4) >> 2] = cu32_qp;
    }
#endif //~SIG_CCU
  }
}
#endif

RC_Float TEncRCPic::getLCUTargetBpp(SliceType eSliceType)
{
  Int   LCUIdx    = getLCUCoded();
  RC_Float bpp      = -1.0;
  Int avgBits     = 0;

  if (eSliceType == I_SLICE)
  {
    Int noOfLCUsLeft = m_numberOfLCU - LCUIdx + 1;
    Int bitrateWindow = min(4,noOfLCUsLeft);
    RC_Float MAD      = getLCU(LCUIdx).m_costIntra;

    if (m_remainingCostIntra > 0.1 )
    {
      RC_Float weightedBitsLeft = (m_bitsLeft*bitrateWindow+(m_bitsLeft-getLCU(LCUIdx).m_targetBitsLeft)*noOfLCUsLeft)/(RC_Float)bitrateWindow;
      avgBits = Int( MAD*weightedBitsLeft/m_remainingCostIntra );
    }
    else
    {
      avgBits = Int( m_bitsLeft / m_LCULeft );
    }
    m_remainingCostIntra -= MAD;
  }
  else
  {
    RC_Float totalWeight = 0;
    for ( Int i=LCUIdx; i<m_numberOfLCU; i++ )
    {
      totalWeight += m_LCUs[i].m_bitWeight;
    }
    Int realInfluenceLCU = min( g_RCLCUSmoothWindowSize, getLCULeft() );
    avgBits = (Int)( m_LCUs[LCUIdx].m_bitWeight - ( totalWeight - m_bitsLeft ) / realInfluenceLCU + 0.5 );
  }

  if ( avgBits < 1 )
  {
    avgBits = 1;
  }

  bpp = ( RC_Float )avgBits/( RC_Float )m_LCUs[ LCUIdx ].m_numberOfPixel;
  m_LCUs[ LCUIdx ].m_targetBits = avgBits;

  return bpp;
}

RC_Float TEncRCPic::getLCUEstLambda( RC_Float bpp )
{
  Int   LCUIdx = getLCUCoded();
  RC_Float alpha;
  RC_Float beta;
  if ( m_encRCSeq->getUseLCUSeparateModel() )
  {
    alpha = m_encRCSeq->getLCUPara( m_frameLevel, LCUIdx ).m_alpha;
    beta  = m_encRCSeq->getLCUPara( m_frameLevel, LCUIdx ).m_beta;
  }
  else
  {
    alpha = m_encRCSeq->getPicPara( m_frameLevel ).m_alpha;
    beta  = m_encRCSeq->getPicPara( m_frameLevel ).m_beta;
  }

  RC_Float estLambda = alpha * pow( bpp, beta );
  //for Lambda clip, picture level clip
  RC_Float clipPicLambda = m_estPicLambda;

  //for Lambda clip, LCU level clip
  RC_Float clipNeighbourLambda = -1.0;
  for ( Int i=LCUIdx - 1; i>=0; i-- )
  {
    if ( m_LCUs[i].m_lambda > 0 )
    {
      clipNeighbourLambda = m_LCUs[i].m_lambda;
      break;
    }
  }

  if ( clipNeighbourLambda > 0.0 )
  {
    estLambda = Clip3<RC_Float>( clipNeighbourLambda * pow( 2.0, -1.0/3.0 ), clipNeighbourLambda * (pow( 2.0, 1.0/3.0 )), estLambda );
  }

  if ( clipPicLambda > 0.0 )
  {
    estLambda = Clip3<RC_Float>( clipPicLambda *pow( 2.0, -2.0/3.0 ), clipPicLambda * pow( 2.0, 2.0/3.0 ), estLambda );
  }
  else
  {
    estLambda = Clip3<RC_Float>( 10.0, 1000.0, estLambda );
  }

  if ( estLambda < 0.1 )
  {
    estLambda = 0.1;
  }
#if JVET_K0390_RATE_CTRL
  //Avoid different results in different platforms. The problem is caused by the different results of pow() in different platforms.
  estLambda = RC_Float(int64_t(estLambda * (RC_Float)LAMBDA_PREC + 0.5)) / (RC_Float)LAMBDA_PREC;
#endif
  return estLambda;
}

Int TEncRCPic::getLCUEstQP( RC_Float lambda, Int clipPicQP )
{
  Int LCUIdx = getLCUCoded();
  Int estQP = LambdaToQp(lambda);

  //for Lambda clip, LCU level clip
  Int clipNeighbourQP = g_RCInvalidQPValue;
  for ( Int i=LCUIdx - 1; i>=0; i-- )
  {
    if ( (getLCU(i)).m_QP > g_RCInvalidQPValue )
    {
      clipNeighbourQP = getLCU(i).m_QP;
      break;
    }
  }

  if ( clipNeighbourQP > g_RCInvalidQPValue )
  {
    estQP = Clip3( clipNeighbourQP - 1, clipNeighbourQP + 1, estQP );
  }

  estQP = Clip3( clipPicQP - 2, clipPicQP + 2, estQP );

  return estQP;
}

#if JVET_M0600_RATE_CTRL
Void TEncRCPic::updateAfterCTU(Int LCUIdx, Int bits, Int QP, RC_Float lambda, RC_Float skipRatio, Bool updateLCUParameter)
#else
Void TEncRCPic::updateAfterCTU( Int LCUIdx, Int bits, Int QP, RC_Float lambda, Bool updateLCUParameter )
#endif
{
  m_LCUs[LCUIdx].m_actualBits = bits;
  m_LCUs[LCUIdx].m_QP         = QP;
  m_LCUs[LCUIdx].m_lambda     = lambda;
#if JVET_K0390_RATE_CTRL
  m_LCUs[LCUIdx].m_actualSSE = m_LCUs[LCUIdx].m_actualMSE * m_LCUs[LCUIdx].m_numberOfPixel;
#endif

  m_LCULeft--;
  m_bitsLeft   -= bits;
  m_pixelsLeft -= m_LCUs[LCUIdx].m_numberOfPixel;

#ifdef CVI_RANDOM_ENCODE
  if (!g_random_rc)
  {
    assert(bits <= MAX_CTU_CODED_BIT);
  }
#endif

#ifdef CVI_HW_RC
  // update current CTU comsumed bit
  bits = min(bits, MAX_CTU_CODED_BIT);
  m_ctuRowAccumBit_delay = m_ctuRowAccumBit;
  m_ctuRowAccumBit = min(m_ctuRowAccumBit + bits, MAX_CTU_ROW_BIT_ACCUM);
  m_isRowOverflow_delay = m_isRowOverflow;
  m_isRowOverflow |= (m_ctuRowAccumBit >= m_ctuRowOvfTargetBit);

  // update bit accumuated error within frame
  if (LCUIdx % m_ctuNumInRow==(m_ctuNumInRow - 1 - ROW_ACCUM_BIT_DELAY_CTU)) {
    g_bit_error_accum = Clip3(MIN_BIT_ERR, MAX_BIT_ERR, g_bit_error_accum +
                              m_ctuRowAvgBit - m_ctuRowAccumBit);
    m_ctuRowAccumBit = 0;
    hw_calcCTUOvfTargetBit();
  }
#endif //~CVI_HW_RC

  if ( !updateLCUParameter )
  {
    return;
  }

  if ( !m_encRCSeq->getUseLCUSeparateModel() )
  {
    return;
  }

  RC_Float alpha = m_encRCSeq->getLCUPara( m_frameLevel, LCUIdx ).m_alpha;
  RC_Float beta  = m_encRCSeq->getLCUPara( m_frameLevel, LCUIdx ).m_beta;

  Int LCUActualBits   = m_LCUs[LCUIdx].m_actualBits;
  Int LCUTotalPixels  = m_LCUs[LCUIdx].m_numberOfPixel;
  RC_Float bpp         = LCUActualBits/( RC_Float )LCUTotalPixels;
  RC_Float calLambda   = alpha * pow( bpp, beta );
  RC_Float inputLambda = m_LCUs[LCUIdx].m_lambda;

  if( inputLambda < 0.01 || calLambda < 0.01 || bpp < 0.0001 )
  {
    alpha *= ( 1.0 - m_encRCSeq->getAlphaUpdate() / 2.0 );
    beta  *= ( 1.0 - m_encRCSeq->getBetaUpdate() / 2.0 );

    alpha = Clip3( g_RCAlphaMinValue, g_RCAlphaMaxValue, alpha );
    beta  = Clip3( g_RCBetaMinValue,  g_RCBetaMaxValue,  beta  );

    TRCParameter rcPara;
    rcPara.m_alpha = alpha;
    rcPara.m_beta  = beta;
#if JVET_M0600_RATE_CTRL
    rcPara.m_skipRatio = skipRatio;
#endif
#if JVET_K0390_RATE_CTRL
    if (QP == g_RCInvalidQPValue && m_encRCSeq->getAdaptiveBits() == 1)
    {
      rcPara.m_validPix = 0;
    }
    else
    {
      rcPara.m_validPix = LCUTotalPixels;
    }

    RC_Float MSE = m_LCUs[LCUIdx].m_actualMSE;
    RC_Float updatedK = bpp * inputLambda / MSE;
    RC_Float updatedC = MSE / pow(bpp, -updatedK);
    rcPara.m_alpha = updatedC * updatedK;
    rcPara.m_beta = -updatedK - 1.0;

    if (bpp > 0 && updatedK > 0.0001)
    {
      m_encRCSeq->setLCUPara(m_frameLevel, LCUIdx, rcPara);
    }
    else
    {
      rcPara.m_alpha = Clip3<RC_Float>(0.0001, g_RCAlphaMaxValue, rcPara.m_alpha);
      m_encRCSeq->setLCUPara(m_frameLevel, LCUIdx, rcPara);
    }
#else
    m_encRCSeq->setLCUPara( m_frameLevel, LCUIdx, rcPara );
#endif
    return;
  }

  calLambda = Clip3<RC_Float>( inputLambda /10.0, inputLambda * 10.0, calLambda );
  alpha += m_encRCSeq->getAlphaUpdate() * ( log( inputLambda ) - log( calLambda ) ) * alpha;
  RC_Float lnbpp = log( bpp );
  lnbpp = Clip3<RC_Float>( -5.0, -0.1, lnbpp );
  beta  += m_encRCSeq->getBetaUpdate() * ( log( inputLambda ) - log( calLambda ) ) * lnbpp;

  alpha = Clip3( g_RCAlphaMinValue, g_RCAlphaMaxValue, alpha );
  beta  = Clip3( g_RCBetaMinValue,  g_RCBetaMaxValue,  beta  );

  TRCParameter rcPara;
  rcPara.m_alpha = alpha;
  rcPara.m_beta  = beta;
#if JVET_M0600_RATE_CTRL
  rcPara.m_skipRatio = skipRatio;
#endif
#if JVET_K0390_RATE_CTRL
  if (QP == g_RCInvalidQPValue && m_encRCSeq->getAdaptiveBits() == 1)
  {
    rcPara.m_validPix = 0;
  }
  else
  {
    rcPara.m_validPix = LCUTotalPixels;
  }

  RC_Float MSE = m_LCUs[LCUIdx].m_actualMSE;
  RC_Float updatedK = bpp * inputLambda / MSE;
  RC_Float updatedC = MSE / pow(bpp, -updatedK);
  rcPara.m_alpha = updatedC * updatedK;
  rcPara.m_beta = -updatedK - 1.0;

  if (bpp > 0 && updatedK > 0.0001)
  {
    m_encRCSeq->setLCUPara(m_frameLevel, LCUIdx, rcPara);
  }
  else
  {
    rcPara.m_alpha = Clip3<RC_Float>(0.0001, g_RCAlphaMaxValue, rcPara.m_alpha);
    m_encRCSeq->setLCUPara(m_frameLevel, LCUIdx, rcPara);
  }
#else
  m_encRCSeq->setLCUPara( m_frameLevel, LCUIdx, rcPara );
#endif
}
Void TEncRCPic::calAvgLambdaAndQpByHist(RC_Float* p_lambda, RC_Float* p_qp)
{
  Int qp_accum = 0, qp_cnt = 0;
  for(Int qp=m_blkMinQp; qp<=m_blkMaxQp; qp++) {
    qp_accum += g_qp_hist[qp]*qp;
    qp_cnt += g_qp_hist[qp];
  }
  RC_Float avgQP = qp_accum / (RC_Float)qp_cnt;
  *p_qp = avgQP;
  *p_lambda = QpToLambda(avgQP);
}

RC_Float TEncRCPic::calAverageQP()
{
  Int totalQPs = 0;
  Int numTotalLCUs = 0;

  Int i;
  for ( i=0; i<m_numberOfLCU; i++ )
  {
    if ( m_LCUs[i].m_QP > 0 )
    {
      totalQPs += m_LCUs[i].m_QP;
      numTotalLCUs++;
    }
  }

  RC_Float avgQP = 0.0;
  if ( numTotalLCUs == 0 )
  {
    avgQP = g_RCInvalidQPValue;
  }
  else
  {
    avgQP = ((RC_Float)totalQPs) / ((RC_Float)numTotalLCUs);
  }
  return avgQP;
}

RC_Float TEncRCPic::calAverageLambda()
{
  RC_Float totalLambdas = 0.0;
  Int numTotalLCUs = 0;
#if JVET_K0390_RATE_CTRL
  RC_Float totalSSE = 0.0;
  Int totalPixels = 0;
#endif
  Int i;
  for ( i=0; i<m_numberOfLCU; i++ )
  {
    if ( m_LCUs[i].m_lambda > 0.01 )
    {
#if JVET_K0390_RATE_CTRL
      if (m_LCUs[i].m_QP > 0 || m_encRCSeq->getAdaptiveBits() != 1)
      {
        m_validPixelsInPic += m_LCUs[i].m_numberOfPixel;
        totalLambdas += log(m_LCUs[i].m_lambda);
        numTotalLCUs++;
      }
#else
      totalLambdas += log( m_LCUs[i].m_lambda );
      numTotalLCUs++;
#endif

#if JVET_K0390_RATE_CTRL
      if (m_LCUs[i].m_QP > 0 || m_encRCSeq->getAdaptiveBits() != 1)
      {
        totalSSE += m_LCUs[i].m_actualSSE;
        totalPixels += m_LCUs[i].m_numberOfPixel;
      }
#endif
    }
  }
#if JVET_K0390_RATE_CTRL
  setPicMSE(totalPixels > 0 ? totalSSE / (RC_Float)totalPixels : 1.0); //1.0 is useless in the following process, just to make sure the divisor not be 0
#endif

  RC_Float avgLambda;
  if( numTotalLCUs == 0 )
  {
    avgLambda = -1.0;
  }
  else
  {
    avgLambda = pow( 2.7183, totalLambdas / numTotalLCUs );
  }
  return avgLambda;
}


Void TEncRCPic::updateAfterPicture( Int actualHeaderBits, Int actualTotalBits, RC_Float averageQP, RC_Float averageLambda, SliceType eSliceType)
{
  m_picActualHeaderBits = actualHeaderBits;
  m_picActualBits       = actualTotalBits;
  if ( averageQP > 0.0 )
  {
    m_picQP             = Int( averageQP + 0.5 );
  }
  else
  {
    m_picQP             = g_RCInvalidQPValue;
  }
  m_picLambda           = averageLambda;

  RC_Float alpha = m_encRCSeq->getPicPara( m_frameLevel ).m_alpha;
  RC_Float beta  = m_encRCSeq->getPicPara( m_frameLevel ).m_beta;
#if JVET_M0600_RATE_CTRL //calculate the skipRatio of picture
  RC_Float skipRatio = m_numberOfSkipPixel/(RC_Float)m_numberOfPixel;
#endif
  if (eSliceType == I_SLICE)
  {
    updateAlphaBetaIntra(&alpha, &beta);
  }
#ifndef CVI_STREAMING_RC
  else
#else
  else if(g_RCMdlUpdatType==0) // LMS algorithm
#endif
  {
    // update parameters
    RC_Float picActualBits = (RC_Float) m_picActualBits;
    RC_Float picActualBpp  = picActualBits/(RC_Float)m_numberOfPixel;
    RC_Float calLambda     = alpha * pow( picActualBpp, beta );
    RC_Float inputLambda   = m_picLambda;
#ifndef CVI_STREAMING_RC
    if ( inputLambda < 0.01 || calLambda < 0.01 || picActualBpp < 0.0001 )
    {
      alpha *= ( 1.0 - m_encRCSeq->getAlphaUpdate() / 2.0 );
      beta  *= ( 1.0 - m_encRCSeq->getBetaUpdate() / 2.0 );

      alpha = Clip3( g_RCAlphaMinValue, g_RCAlphaMaxValue, alpha );
      beta  = Clip3( g_RCBetaMinValue,  g_RCBetaMaxValue,  beta  );

      TRCParameter rcPara;
      rcPara.m_alpha = alpha;
      rcPara.m_beta  = beta;
#if JVET_M0600_RATE_CTRL
      rcPara.m_skipRatio = skipRatio;
#endif
      m_encRCSeq->setPicPara( m_frameLevel, rcPara );

      return;
    }
#endif
    calLambda = Clip3<RC_Float>( inputLambda / 10.0, inputLambda *10.0, calLambda );
    alpha += m_encRCSeq->getAlphaUpdate() * ( log( inputLambda ) - log( calLambda ) ) * alpha;
    RC_Float lnbpp = log( picActualBpp );
    lnbpp = Clip3<RC_Float>(-5.0, -0.1, lnbpp );
    beta  += m_encRCSeq->getBetaUpdate() * ( log( inputLambda ) - log( calLambda ) ) * lnbpp;
    alpha = Clip3( g_RCAlphaMinValue, g_RCAlphaMaxValue, alpha );
    beta  = Clip3( g_RCBetaMinValue,  g_RCBetaMaxValue,  beta  );
  }
#ifdef CVI_STREAMING_RC
  else  // RD function analytics
  {
    RC_Float picActualBpp = m_picActualBits / (RC_Float)m_numberOfPixel;
    RC_Float avgMSE = getPicMSE();
    RC_Float updatedK = picActualBpp * averageLambda / avgMSE;
#if SOFT_FLOAT
    RC_Float updatedC = avgMSE /SoftFloat_to_Float(CVI_FLOAT_POW(Float_to_SoftFloat(picActualBpp) , Float_to_SoftFloat(-updatedK)));
#else
    RC_Float updatedC = avgMSE / pow(picActualBpp, -updatedK);
#endif
    RC_Float updateScale = Clip3<RC_Float>(0.1, 0.9 , (1-skipRatio));
    RC_Float newAlpha = updatedC * updatedK;
    RC_Float newBeta = -updatedK - 1.0;
    RC_Float updateAlpha = updateScale*newAlpha + (1-updateScale)*alpha;
    RC_Float updateBeta = updateScale*newBeta +  (1-updateScale)*beta;
    alpha = Clip3(g_RCAlphaMinValue, g_RCAlphaMaxValue, updateAlpha);
    beta = Clip3(g_RCBetaMinValue,  g_RCBetaMaxValue, updateBeta);
  }
#endif
  TRCParameter rcPara;
  rcPara.m_alpha = alpha;
  rcPara.m_beta  = beta;
  m_validPixelsInPic = m_numberOfPixel;
  rcPara.m_validPix = m_validPixelsInPic;
#if JVET_M0600_RATE_CTRL
  rcPara.m_skipRatio = skipRatio;
#endif
  m_encRCSeq->setPicPara( m_frameLevel, rcPara );

#if RC_DEBUG
  if(g_RCDebugLogEn) {
    printf("\n%.3f %.3f\n", rcPara.m_alpha, rcPara.m_beta);
  }
#endif
#ifdef SIGDUMP
  if(g_sigdump.pic_rc)
  {
    TRCParameter *m_picPara = m_encRCSeq->getPicPara();
    sigdump_output_fprint(&g_sigpool.pic_rc_golden, "%u, %u, %u, %u, %u\n", Float_to_SoftFloat(averageLambda), Float_to_SoftFloat(m_picPara[0].m_alpha), Float_to_SoftFloat(m_picPara[1].m_alpha), 
      Float_to_SoftFloat(m_picPara[0].m_beta), Float_to_SoftFloat(m_picPara[1].m_beta));
    printf("%d, averageLambda = %.8f, alpha, %.8f, %.8f, beta, %.8f, %.8f\n", g_sigpool.enc_count, averageLambda, m_picPara[0].m_alpha, m_picPara[1].m_alpha, 
      m_picPara[0].m_beta, m_picPara[1].m_beta);
  }
#endif
  if (m_frameLevel>0)
  {
    // only for rc-gop adaptive bit allocation
    RC_Float currLambda = Clip3<RC_Float>( 0.1, 10000.0, m_picLambda );
    RC_Float updateLastLambda = g_RCWeightHistoryLambda * m_encRCSeq->getLastLambda() + g_RCWeightCurrentLambda * currLambda;
    m_encRCSeq->setLastLambda( updateLastLambda );
#ifdef CVI_STREAMING_RC
    m_encRCSeq->addHistoryPicQpAccum(averageQP);
  }
  else if(m_frameLevel==0) {
    m_encRCSeq->getLastILambda() = m_picLambda;
    m_encRCSeq->getLastIQp()  = m_picQP;
    m_encRCSeq->resetHistoryPicQpAccum();
#endif
  }
  // texture complexity for I frame model
  rc_setPicAvgTc(g_pic_tc_accum, m_numberOfPixelAlign32);

#ifdef SIGDUMP
  if(g_sigdump.pic_rc) {
    if (sizeof(RC_Float) == sizeof(double))
    {
      sigdump_output_fprint(&g_sigpool.pic_rc_stats, "%d, %d, %f, %d,\
      %f, %f, %f, %f,  %f,  %f,  %f\n", m_targetBits, m_estPicQP, m_estPicLambda, m_picActualBits, \
      averageQP, m_picLambda, g_picAvgTc_delay, getPicMSE(), skipRatio, alpha, beta);
    }
    else
    {
      sigdump_output_fprint(&g_sigpool.pic_rc_stats, "%d, %d, %u, %d,\
      %u, %u, %u, %u,  %u,  %u,  %u\n", m_targetBits, m_estPicQP,  Float_to_SoftFloat(m_estPicLambda), m_picActualBits, \
      Float_to_SoftFloat(averageQP), Float_to_SoftFloat(m_picLambda), Float_to_SoftFloat(g_picAvgTc_delay), Float_to_SoftFloat(getPicMSE()), Float_to_SoftFloat(skipRatio), Float_to_SoftFloat(alpha), Float_to_SoftFloat(beta));
    }
  }
#endif
}

RC_Float TEncRCPic::intraLambdaToBpp(RC_Float lambda, RC_Float alpha, RC_Float beta)
{
  RC_Float MADPerPixel = getPicTextCplx();
#if SOFT_FLOAT
  Soft_Float beta_soft = CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(-1), Float_to_SoftFloat(beta));

  RC_Float bpp = MADPerPixel*  SoftFloat_to_Float (CVI_FLOAT_POW(CVI_FLOAT_DIV( CVI_FLOAT_MUL(INT_TO_CVI_FLOAT(256) , \
                                                                  Float_to_SoftFloat(lambda)) , Float_to_SoftFloat(alpha)) , beta_soft));
#else
  RC_Float bpp = MADPerPixel*pow((((RC_Float)(256.0)*lambda)/alpha), -1/beta);
#endif
  return bpp;
}

RC_Float TEncRCPic::lambdaToBpp(RC_Float lambda, RC_Float alpha, RC_Float beta)
{
#if SOFT_FLOAT
  Soft_Float beta_soft = CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(1), Float_to_SoftFloat(beta));
  return SoftFloat_to_Float(CVI_FLOAT_POW(CVI_FLOAT_DIV(Float_to_SoftFloat(lambda) , Float_to_SoftFloat(alpha)) , beta_soft));
#else
  return pow(lambda/alpha, 1/beta);
#endif
}

Int TEncRCPic::getRefineBitsForIntra( Int orgBits, Int qp)
{
  Int iIntraBits = 0;
#ifdef CVI_STREAMING_RC
  RC_Float alpha = m_encRCSeq->getPicPara(0).m_alpha;
  RC_Float beta = m_encRCSeq->getPicPara(0).m_beta;
  RC_Float Lambda = QpToLambda(qp);
  Int iIntraOrgBits = intraLambdaToBpp(Lambda, alpha, beta)*m_numberOfPixel;

  #if CVI_LOW_LUX_CONTROL
  if (m_encRCSeq->getUseLowLuxCtrl()&&(!m_encRCSeq->getLowLuxAdaptive())){
      if ( m_encRCSeq->getLowLuxValue() < LOW_LUX_STRONG_THRESHOLD )
      {
        Int MinIPicBit = m_encRCSeq->getMinIPicBit();
        MinIPicBit = MinIPicBit * LOW_LUX_CONTROL_STRONG / 10;
        iIntraBits = Clip3(MinIPicBit, m_encRCSeq->getMaxIPicBit(), iIntraOrgBits);
      }
      else if ( m_encRCSeq->getLowLuxValue() < LOW_LUX_MEDIUM_THRESHOLD )
      {
        Int MinIPicBit = m_encRCSeq->getMinIPicBit();
        MinIPicBit = MinIPicBit * LOW_LUX_CONTROL_MEDIUM / 10;
        iIntraBits = Clip3(MinIPicBit, m_encRCSeq->getMaxIPicBit(), iIntraOrgBits);
      }
      else
      {
        iIntraBits = Clip3(m_encRCSeq->getMinIPicBit(), m_encRCSeq->getMaxIPicBit(), iIntraOrgBits);
      }
  }
  else
  {
    iIntraBits = Clip3(m_encRCSeq->getMinIPicBit(), m_encRCSeq->getMaxIPicBit(), iIntraOrgBits);
  }
#endif

  if(m_encRCSeq->getLastIPicBit() > 0) {
    Int lastIPicBit = m_encRCSeq->getLastIPicBit();
    Int neiIBitMargin = m_encRCSeq->getNeiIBitMargin();
    Int upperBitLimit = lastIPicBit + neiIBitMargin;
    Int lowerBitLimit = lastIPicBit - neiIBitMargin;
    if(iIntraBits > upperBitLimit) {
      iIntraBits = (2 * iIntraBits + 8 * upperBitLimit) / 10;
    }
    else if(iIntraBits < lowerBitLimit) {
      iIntraBits = (2 * iIntraBits + 8 * lowerBitLimit) / 10;
    }
  }
#if RC_DEBUG
  if(g_RCDebugLogEn) {
    printf("IOrgPic target %d\n", iIntraOrgBits);
  }
#endif
#else
  RC_Float alpha=0.25, beta=0.5582;
  if (orgBits*40 < m_numberOfPixel)
  {
    alpha=0.25;
  }
  else
  {
    alpha=0.30;
  }

  iIntraBits = (Int)(alpha* pow(m_totalCostIntra*4.0/(RC_Float)orgBits, beta)*(RC_Float)orgBits+0.5);
#endif
  return iIntraBits;
}
#if SOFT_FLOAT
RC_Float TEncRCPic::calculateLambdaIntra(Soft_Float alpha, Soft_Float beta, Soft_Float MADPerPixel, Soft_Float bitsPerPixel)
{
  return SoftFloat_to_Float(CVI_FLOAT_MUL(CVI_FLOAT_DIV(alpha , INT_TO_CVI_FLOAT(256)),CVI_FLOAT_POW(CVI_FLOAT_DIV(MADPerPixel, bitsPerPixel), beta)));
}
#endif
RC_Float TEncRCPic::calculateLambdaIntra(RC_Float alpha, RC_Float beta, RC_Float MADPerPixel, RC_Float bitsPerPixel)
{
  return ( (alpha/256.0) * pow( MADPerPixel/bitsPerPixel, beta ) );
}

Void TEncRCPic::updateAlphaBetaIntra(RC_Float *alpha, RC_Float *beta)
{
#if SOFT_FLOAT
  Soft_Float lnbpp = CVI_FLOAT_MAX(FLOAT_VAL_1, CVI_FLOAT_LOG( Float_to_SoftFloat(getPicTextCplx())));
  Soft_Float diffLambda = CVI_FLOAT_MUL(Float_to_SoftFloat (*beta), CVI_FLOAT_SUB(CVI_FLOAT_LOG(INT_TO_CVI_FLOAT(m_picActualBits)), CVI_FLOAT_LOG(INT_TO_CVI_FLOAT(m_targetBits))));
  diffLambda = CVI_FLOAT_CLIP(FLOAT_VAL_minus_p125, FLOAT_VAL_p25, CVI_FLOAT_MUL(FLOAT_VAL_p25, diffLambda));
  *alpha = Clip3(g_RCIAlphaMinValue, g_RCAlphaMaxValue, SoftFloat_to_Float(CVI_FLOAT_MUL(Float_to_SoftFloat(*alpha), CVI_FLOAT_EXP(diffLambda))));
  *beta = Clip3(g_RCIBetaMinValue, g_RCIBetaMaxValue, SoftFloat_to_Float( CVI_FLOAT_ADD(Float_to_SoftFloat(*beta), CVI_FLOAT_DIV(diffLambda, lnbpp))));
#else
  RC_Float lnbpp = std::max(RC_Float(1.0), log(getPicTextCplx()));
  RC_Float diffLambda = (*beta)*(log((RC_Float)m_picActualBits)-log((RC_Float)m_targetBits));
  diffLambda = Clip3(-0.125, 0.25, 0.25*diffLambda);
  *alpha = Clip3(g_RCIAlphaMinValue, g_RCAlphaMaxValue, (*alpha) * exp(diffLambda));
  *beta = Clip3(g_RCIBetaMinValue, g_RCIBetaMaxValue, (*beta) + diffLambda / lnbpp);
#endif


}

Void TEncRCPic::getLCUInitTargetBits()
{
  Int iAvgBits     = 0;

  m_remainingCostIntra = m_totalCostIntra;
  for (Int i=m_numberOfLCU-1; i>=0; i--)
  {
    iAvgBits += Int(m_targetBits * getLCU(i).m_costIntra/m_totalCostIntra);
    getLCU(i).m_targetBitsLeft = iAvgBits;
  }
}


RC_Float TEncRCPic::getLCUEstLambdaAndQP(RC_Float bpp, Int clipPicQP, Int *estQP)
{
  Int   LCUIdx = getLCUCoded();

  RC_Float   alpha = m_encRCSeq->getPicPara( m_frameLevel ).m_alpha;
  RC_Float   beta  = m_encRCSeq->getPicPara( m_frameLevel ).m_beta;

  RC_Float costPerPixel = getLCU(LCUIdx).m_costIntra/(RC_Float)getLCU(LCUIdx).m_numberOfPixel;
  costPerPixel = pow(costPerPixel, BETA1);
  RC_Float estLambda = calculateLambdaIntra(alpha, beta, costPerPixel, bpp);

  Int clipNeighbourQP = g_RCInvalidQPValue;
  for (Int i=LCUIdx-1; i>=0; i--)
  {
    if ((getLCU(i)).m_QP > g_RCInvalidQPValue)
    {
      clipNeighbourQP = getLCU(i).m_QP;
      break;
    }
  }

  Int minQP = clipPicQP - 2;
  Int maxQP = clipPicQP + 2;

  if ( clipNeighbourQP > g_RCInvalidQPValue )
  {
    maxQP = min(clipNeighbourQP + 1, maxQP);
    minQP = max(clipNeighbourQP - 1, minQP);
  }

  RC_Float maxLambda=exp(((maxQP+0.49)-13.7122)/4.2005);
  RC_Float minLambda=exp(((minQP-0.49)-13.7122)/4.2005);

  estLambda = Clip3(minLambda, maxLambda, estLambda);
#if JVET_K0390_RATE_CTRL
  //Avoid different results in different platforms. The problem is caused by the different results of pow() in different platforms.
  estLambda = RC_Float(int64_t(estLambda * (RC_Float)LAMBDA_PREC + 0.5)) / (RC_Float)LAMBDA_PREC;
#endif
  *estQP = LambdaToQp(estLambda);
  *estQP = Clip3(minQP, maxQP, *estQP);

  return estLambda;
}

TEncRateCtrl::TEncRateCtrl()
{
  m_encRCSeq = NULL;
  m_encRCGOP = NULL;
  m_encRCPic = NULL;
}

TEncRateCtrl::~TEncRateCtrl()
{
  destroy();
}

Void TEncRateCtrl::destroy()
{
  if ( m_encRCSeq != NULL )
  {
    delete m_encRCSeq;
    m_encRCSeq = NULL;
  }
  if ( m_encRCGOP != NULL )
  {
    delete m_encRCGOP;
    m_encRCGOP = NULL;
  }
  while ( m_listRCPictures.size() > 0 )
  {
    TEncRCPic* p = m_listRCPictures.front();
    m_listRCPictures.pop_front();
    delete p;
  }
}

Void TEncRateCtrl::init( Int totalFrames, Int targetBitrate, Int frameRate, Int GOPSize, Int picWidth, Int picHeight, Int LCUWidth, Int LCUHeight, Int keepHierBits, Bool useLCUSeparateModel, GOPEntry  GOPList[MAX_GOP], Int intraPeriod, Bool useLowLuxEnControl, Int LuxValue, Int intraQPoffset)
{
  destroy();

  Bool isLowdelay = true;
  for ( Int i=0; i<GOPSize-1; i++ )
  {
    if ( GOPList[i].m_POC > GOPList[i+1].m_POC )
    {
      isLowdelay = false;
      break;
    }
  }

  Int numberOfLevel = 1;
  Int adaptiveBit = 0;
  if ( keepHierBits > 0 )
  {
    numberOfLevel = Int( log(GOPSize)/log(2.0) + 0.5 ) + 1;
  }
#if JVET_K0390_RATE_CTRL
  if (!isLowdelay && (GOPSize == 16 || GOPSize == 8))
#else
  if ( !isLowdelay && GOPSize == 8 )
#endif
  {
    numberOfLevel = Int( log(GOPSize)/log(2.0) + 0.5 ) + 1;
  }
  numberOfLevel++;    // intra picture
  numberOfLevel++;    // non-reference picture


  Int* bitsRatio;
  bitsRatio = new Int[ GOPSize ];
  for ( Int i=0; i<GOPSize; i++ )
  {
    bitsRatio[i] = 10;
    if ( !GOPList[i].m_refPic )
    {
      bitsRatio[i] = 2;
    }
  }

  if ( keepHierBits > 0 )
  {
    RC_Float bpp = (RC_Float)( targetBitrate / (RC_Float)( frameRate*picWidth*picHeight ) );
    if ( GOPSize == 4 && isLowdelay )
    {
      if ( bpp > 0.2 )
      {
        bitsRatio[0] = 2;
        bitsRatio[1] = 3;
        bitsRatio[2] = 2;
        bitsRatio[3] = 6;
      }
      else if( bpp > 0.1 )
      {
        bitsRatio[0] = 2;
        bitsRatio[1] = 3;
        bitsRatio[2] = 2;
        bitsRatio[3] = 10;
      }
      else if ( bpp > 0.05 )
      {
        bitsRatio[0] = 2;
        bitsRatio[1] = 3;
        bitsRatio[2] = 2;
        bitsRatio[3] = 12;
      }
      else
      {
        bitsRatio[0] = 2;
        bitsRatio[1] = 3;
        bitsRatio[2] = 2;
        bitsRatio[3] = 14;
      }

      if ( keepHierBits == 2 )
      {
        adaptiveBit = 1;
      }
    }
    else if ( GOPSize == 8 && !isLowdelay )
    {
      if ( bpp > 0.2 )
      {
        bitsRatio[0] = 15;
        bitsRatio[1] = 5;
        bitsRatio[2] = 4;
        bitsRatio[3] = 1;
        bitsRatio[4] = 1;
        bitsRatio[5] = 4;
        bitsRatio[6] = 1;
        bitsRatio[7] = 1;
      }
      else if ( bpp > 0.1 )
      {
        bitsRatio[0] = 20;
        bitsRatio[1] = 6;
        bitsRatio[2] = 4;
        bitsRatio[3] = 1;
        bitsRatio[4] = 1;
        bitsRatio[5] = 4;
        bitsRatio[6] = 1;
        bitsRatio[7] = 1;
      }
      else if ( bpp > 0.05 )
      {
        bitsRatio[0] = 25;
        bitsRatio[1] = 7;
        bitsRatio[2] = 4;
        bitsRatio[3] = 1;
        bitsRatio[4] = 1;
        bitsRatio[5] = 4;
        bitsRatio[6] = 1;
        bitsRatio[7] = 1;
      }
      else
      {
        bitsRatio[0] = 30;
        bitsRatio[1] = 8;
        bitsRatio[2] = 4;
        bitsRatio[3] = 1;
        bitsRatio[4] = 1;
        bitsRatio[5] = 4;
        bitsRatio[6] = 1;
        bitsRatio[7] = 1;
      }

      if ( keepHierBits == 2 )
      {
        adaptiveBit = 2;
      }
    }
#if JVET_K0390_RATE_CTRL
    else if (GOPSize == 16 && !isLowdelay)
    {
      if (bpp > 0.2)
      {
        bitsRatio[0] = 10;
        bitsRatio[1] = 8;
        bitsRatio[2] = 4;
        bitsRatio[3] = 2;
        bitsRatio[4] = 1;
        bitsRatio[5] = 1;
        bitsRatio[6] = 2;
        bitsRatio[7] = 1;
        bitsRatio[8] = 1;
        bitsRatio[9] = 4;
        bitsRatio[10] = 2;
        bitsRatio[11] = 1;
        bitsRatio[12] = 1;
        bitsRatio[13] = 2;
        bitsRatio[14] = 1;
        bitsRatio[15] = 1;
      }
      else if (bpp > 0.1)
      {
        bitsRatio[0] = 15;
        bitsRatio[1] = 9;
        bitsRatio[2] = 4;
        bitsRatio[3] = 2;
        bitsRatio[4] = 1;
        bitsRatio[5] = 1;
        bitsRatio[6] = 2;
        bitsRatio[7] = 1;
        bitsRatio[8] = 1;
        bitsRatio[9] = 4;
        bitsRatio[10] = 2;
        bitsRatio[11] = 1;
        bitsRatio[12] = 1;
        bitsRatio[13] = 2;
        bitsRatio[14] = 1;
        bitsRatio[15] = 1;
      }
      else if (bpp > 0.05)
      {
        bitsRatio[0] = 40;
        bitsRatio[1] = 17;
        bitsRatio[2] = 7;
        bitsRatio[3] = 2;
        bitsRatio[4] = 1;
        bitsRatio[5] = 1;
        bitsRatio[6] = 2;
        bitsRatio[7] = 1;
        bitsRatio[8] = 1;
        bitsRatio[9] = 7;
        bitsRatio[10] = 2;
        bitsRatio[11] = 1;
        bitsRatio[12] = 1;
        bitsRatio[13] = 2;
        bitsRatio[14] = 1;
        bitsRatio[15] = 1;
      }
      else
      {
        bitsRatio[0] = 40;
        bitsRatio[1] = 15;
        bitsRatio[2] = 6;
        bitsRatio[3] = 3;
        bitsRatio[4] = 1;
        bitsRatio[5] = 1;
        bitsRatio[6] = 3;
        bitsRatio[7] = 1;
        bitsRatio[8] = 1;
        bitsRatio[9] = 6;
        bitsRatio[10] = 3;
        bitsRatio[11] = 1;
        bitsRatio[12] = 1;
        bitsRatio[13] = 3;
        bitsRatio[14] = 1;
        bitsRatio[15] = 1;
      }

      if (keepHierBits == 2)
      {
        adaptiveBit = 3;
      }
    }
#endif
    else
    {
      printf( "\n hierarchical bit allocation is not support for the specified coding structure currently.\n" );
    }
  }

  Int* GOPID2Level = new Int[ GOPSize ];
  for ( Int i=0; i<GOPSize; i++ )
  {
    GOPID2Level[i] = 1;
    if ( !GOPList[i].m_refPic )
    {
      GOPID2Level[i] = 2;
    }
  }

  if ( keepHierBits > 0 )
  {
    if ( GOPSize == 4 && isLowdelay )
    {
      GOPID2Level[0] = 3;
      GOPID2Level[1] = 2;
      GOPID2Level[2] = 3;
      GOPID2Level[3] = 1;
    }
    else if ( GOPSize == 8 && !isLowdelay )
    {
      GOPID2Level[0] = 1;
      GOPID2Level[1] = 2;
      GOPID2Level[2] = 3;
      GOPID2Level[3] = 4;
      GOPID2Level[4] = 4;
      GOPID2Level[5] = 3;
      GOPID2Level[6] = 4;
      GOPID2Level[7] = 4;
    }
#if JVET_K0390_RATE_CTRL
    else if (GOPSize == 16 && !isLowdelay)
    {
      GOPID2Level[0] = 1;
      GOPID2Level[1] = 2;
      GOPID2Level[2] = 3;
      GOPID2Level[3] = 4;
      GOPID2Level[4] = 5;
      GOPID2Level[5] = 5;
      GOPID2Level[6] = 4;
      GOPID2Level[7] = 5;
      GOPID2Level[8] = 5;
      GOPID2Level[9] = 3;
      GOPID2Level[10] = 4;
      GOPID2Level[11] = 5;
      GOPID2Level[12] = 5;
      GOPID2Level[13] = 4;
      GOPID2Level[14] = 5;
      GOPID2Level[15] = 5;
    }
#endif
  }

  if ( !isLowdelay && GOPSize == 8 )
  {
    GOPID2Level[0] = 1;
    GOPID2Level[1] = 2;
    GOPID2Level[2] = 3;
    GOPID2Level[3] = 4;
    GOPID2Level[4] = 4;
    GOPID2Level[5] = 3;
    GOPID2Level[6] = 4;
    GOPID2Level[7] = 4;
  }
#if JVET_K0390_RATE_CTRL
  else if (GOPSize == 16 && !isLowdelay)
  {
    GOPID2Level[0] = 1;
    GOPID2Level[1] = 2;
    GOPID2Level[2] = 3;
    GOPID2Level[3] = 4;
    GOPID2Level[4] = 5;
    GOPID2Level[5] = 5;
    GOPID2Level[6] = 4;
    GOPID2Level[7] = 5;
    GOPID2Level[8] = 5;
    GOPID2Level[9] = 3;
    GOPID2Level[10] = 4;
    GOPID2Level[11] = 5;
    GOPID2Level[12] = 5;
    GOPID2Level[13] = 4;
    GOPID2Level[14] = 5;
    GOPID2Level[15] = 5;
  }
#endif

  m_encRCSeq = new TEncRCSeq;
  m_encRCSeq->create( totalFrames, targetBitrate, frameRate, GOPSize, picWidth, picHeight, LCUWidth, LCUHeight, numberOfLevel, useLCUSeparateModel, adaptiveBit, intraPeriod, useLowLuxEnControl, LuxValue, intraQPoffset);
  m_encRCSeq->initBitsRatio( bitsRatio );
  m_encRCSeq->initGOPID2Level( GOPID2Level );
  m_encRCSeq->initPicPara();
  if ( useLCUSeparateModel )
  {
    m_encRCSeq->initLCUPara();
  }
  m_CpbSaturationEnabled = false;
  m_cpbSize              = targetBitrate;
  m_cpbState             = (UInt)(m_cpbSize*0.5f);
  m_bufferingRate        = (Int)(targetBitrate / frameRate);

  delete[] bitsRatio;
  delete[] GOPID2Level;
}

Void TEncRateCtrl::initRCPic( Int frameLevel )
{
  m_encRCPic = new TEncRCPic;
  m_encRCPic->create( m_encRCSeq, m_encRCGOP, frameLevel, m_listRCPictures );
}

Void TEncRateCtrl::initRCGOP( Int numberOfPictures )
{
  m_encRCGOP = new TEncRCGOP;
  m_encRCGOP->create( m_encRCSeq, numberOfPictures );
}

Int  TEncRateCtrl::updateCpbState(Int actualBits)
{
  Int cpbState = 1;

  m_cpbState -= actualBits;
  if (m_cpbState < 0)
  {
    cpbState = -1;
  }

  m_cpbState += m_bufferingRate;
  if (m_cpbState > m_cpbSize)
  {
    cpbState = 0;
  }

  return cpbState;
}

Void TEncRateCtrl::initHrdParam(const TComHRD* pcHrd, Int iFrameRate, RC_Float fInitialCpbFullness)
{
  m_CpbSaturationEnabled = true;
  m_cpbSize = (pcHrd->getCpbSizeValueMinus1(0, 0, 0) + 1) << (4 + pcHrd->getCpbSizeScale());
  m_cpbState = (UInt)(m_cpbSize*fInitialCpbFullness);
  m_bufferingRate = (UInt)(((pcHrd->getBitRateValueMinus1(0, 0, 0) + 1) << (6 + pcHrd->getBitRateScale())) / iFrameRate);
  printf("\nHRD - [Initial CPB state %6d] [CPB Size %6d] [Buffering Rate %6d]\n", m_cpbState, m_cpbSize, m_bufferingRate);
}

Void TEncRateCtrl::destroyRCGOP()
{
  delete m_encRCGOP;
  m_encRCGOP = NULL;
}
