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

/** \file     TEncSearch.cpp
 \brief    encoder search class
 */

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComRom.h"
#include "TLibCommon/TComMotionInfo.h"
#include "TEncSearch.h"
#include "TLibCommon/TComTU.h"
#include "TLibCommon/Debug.h"
#include <math.h>
#include <limits>

#include "TLibCommon/cvi_sigdump.h"
#include "TLibCommon/cvi_algo_cfg.h"
#include "TLibCommon/cvi_cache.h"
#include "TLibCommon/cvi_frm_buf_mgr.h"
#include "TLibCommon/cvi_rate_ctrl.h"
#include "TLibCommon/cvi_intra.h"
#include "TLibCommon/cvi_pattern.h"
#include "TLibCommon/cvi_ime.h"
#include "cvi_enc.h"

//! \ingroup TLibEncoder
//! \{

static const TComMv s_acMvRefineH[9] =
{
  TComMv(  0,  0 ), // 0
  TComMv(  0, -1 ), // 1
  TComMv(  0,  1 ), // 2
  TComMv( -1,  0 ), // 3
  TComMv(  1,  0 ), // 4
  TComMv( -1, -1 ), // 5
  TComMv(  1, -1 ), // 6
  TComMv( -1,  1 ), // 7
  TComMv(  1,  1 )  // 8
};

static const TComMv s_acMvRefineQ[9] =
{
  TComMv(  0,  0 ), // 0
  TComMv(  0, -1 ), // 1
  TComMv(  0,  1 ), // 2
  TComMv( -1, -1 ), // 5
  TComMv(  1, -1 ), // 6
  TComMv( -1,  0 ), // 3
  TComMv(  1,  0 ), // 4
  TComMv( -1,  1 ), // 7
  TComMv(  1,  1 )  // 8
};

static Void offsetSubTUCBFs(TComTU &rTu, const ComponentID compID)
{
        TComDataCU *pcCU              = rTu.getCU();
  const UInt        uiTrDepth         = rTu.GetTransformDepthRel();
  const UInt        uiAbsPartIdx      = rTu.GetAbsPartIdxTU(compID);
  const UInt        partIdxesPerSubTU = rTu.GetAbsPartIdxNumParts(compID) >> 1;

  //move the CBFs down a level and set the parent CBF

  UChar subTUCBF[2];
  UChar combinedSubTUCBF = 0;

  for (UInt subTU = 0; subTU < 2; subTU++)
  {
    const UInt subTUAbsPartIdx = uiAbsPartIdx + (subTU * partIdxesPerSubTU);

    subTUCBF[subTU]   = pcCU->getCbf(subTUAbsPartIdx, compID, uiTrDepth);
    combinedSubTUCBF |= subTUCBF[subTU];
  }

  for (UInt subTU = 0; subTU < 2; subTU++)
  {
    const UInt subTUAbsPartIdx = uiAbsPartIdx + (subTU * partIdxesPerSubTU);
    const UChar compositeCBF = (subTUCBF[subTU] << 1) | combinedSubTUCBF;

    pcCU->setCbfPartRange((compositeCBF << uiTrDepth), compID, subTUAbsPartIdx, partIdxesPerSubTU);
  }
}


TEncSearch::TEncSearch()
: m_puhQTTempTrIdx(NULL)
, m_pcQTTempTComYuv(NULL)
, m_pcEncCfg (NULL)
, m_pcTrQuant (NULL)
, m_pcRdCost (NULL)
, m_pcEntropyCoder (NULL)
, m_iSearchRange (0)
, m_bipredSearchRange (0)
, m_motionEstimationSearchMethod (MESEARCH_FULL)
, m_pppcRDSbacCoder (NULL)
, m_pcRDGoOnSbacCoder (NULL)
, m_pTempPel (NULL)
, m_isInitialized (false)
{
  for (UInt ch=0; ch<MAX_NUM_COMPONENT; ch++)
  {
    m_ppcQTTempCoeff[ch]                           = NULL;
#if ADAPTIVE_QP_SELECTION
    m_ppcQTTempArlCoeff[ch]                        = NULL;
#endif
    m_puhQTTempCbf[ch]                             = NULL;
    m_phQTTempCrossComponentPredictionAlpha[ch]    = NULL;
    m_pSharedPredTransformSkip[ch]                 = NULL;
    m_pcQTTempTUCoeff[ch]                          = NULL;
#if ADAPTIVE_QP_SELECTION
    m_ppcQTTempTUArlCoeff[ch]                      = NULL;
#endif
    m_puhQTTempTransformSkipFlag[ch]               = NULL;
  }

  for (Int i=0; i<MAX_NUM_REF_LIST_ADAPT_SR; i++)
  {
    memset (m_aaiAdaptSR[i], 0, MAX_IDX_ADAPT_SR * sizeof (Int));
  }
  for (Int i=0; i<AMVP_MAX_NUM_CANDS+1; i++)
  {
    memset (m_auiMVPIdxCost[i], 0, (AMVP_MAX_NUM_CANDS+1) * sizeof (UInt) );
  }

  setWpScalingDistParam( NULL, -1, REF_PIC_LIST_X );
}


Void TEncSearch::destroy()
{
  assert (m_isInitialized);
  if ( m_pTempPel )
  {
    delete [] m_pTempPel;
    m_pTempPel = NULL;
  }

  if ( m_pcEncCfg )
  {
    const UInt uiNumLayersAllocated = m_pcEncCfg->getQuadtreeTULog2MaxSize()-m_pcEncCfg->getQuadtreeTULog2MinSize()+1;

    for (UInt ch=0; ch<MAX_NUM_COMPONENT; ch++)
    {
      for (UInt layer = 0; layer < uiNumLayersAllocated; layer++)
      {
        delete[] m_ppcQTTempCoeff[ch][layer];
#if ADAPTIVE_QP_SELECTION
        delete[] m_ppcQTTempArlCoeff[ch][layer];
#endif
      }
      delete[] m_ppcQTTempCoeff[ch];
      delete[] m_puhQTTempCbf[ch];
#if ADAPTIVE_QP_SELECTION
      delete[] m_ppcQTTempArlCoeff[ch];
#endif
    }

    for( UInt layer = 0; layer < uiNumLayersAllocated; layer++ )
    {
      m_pcQTTempTComYuv[layer].destroy();
    }
  }

  delete[] m_puhQTTempTrIdx;
  delete[] m_pcQTTempTComYuv;

  for (UInt ch=0; ch<MAX_NUM_COMPONENT; ch++)
  {
    delete[] m_pSharedPredTransformSkip[ch];
    delete[] m_pcQTTempTUCoeff[ch];
#if ADAPTIVE_QP_SELECTION
    delete[] m_ppcQTTempTUArlCoeff[ch];
#endif
    delete[] m_phQTTempCrossComponentPredictionAlpha[ch];
    delete[] m_puhQTTempTransformSkipFlag[ch];
  }
  m_pcQTTempTransformSkipTComYuv.destroy();

  m_tmpYuvPred.destroy();
  m_isInitialized = false;
}

TEncSearch::~TEncSearch()
{
  if (m_isInitialized)
  {
    destroy();
  }
}




Void TEncSearch::init(TEncCfg*       pcEncCfg,
                      TComTrQuant*   pcTrQuant,
                      Int            iSearchRange,
                      Int            bipredSearchRange,
                      MESearchMethod motionEstimationSearchMethod,
                      const UInt     maxCUWidth,
                      const UInt     maxCUHeight,
                      const UInt     maxTotalCUDepth,
                      TEncEntropy*   pcEntropyCoder,
                      TComRdCost*    pcRdCost,
                      TEncSbac***    pppcRDSbacCoder,
                      TEncSbac*      pcRDGoOnSbacCoder
                      )
{
  assert (!m_isInitialized);
  m_pcEncCfg                     = pcEncCfg;
  m_pcTrQuant                    = pcTrQuant;
  m_iSearchRange                 = iSearchRange;
  m_bipredSearchRange            = bipredSearchRange;
  m_motionEstimationSearchMethod = motionEstimationSearchMethod;
  m_pcEntropyCoder               = pcEntropyCoder;
  m_pcRdCost                     = pcRdCost;

  m_pppcRDSbacCoder              = pppcRDSbacCoder;
  m_pcRDGoOnSbacCoder            = pcRDGoOnSbacCoder;

  for (UInt iDir = 0; iDir < MAX_NUM_REF_LIST_ADAPT_SR; iDir++)
  {
    for (UInt iRefIdx = 0; iRefIdx < MAX_IDX_ADAPT_SR; iRefIdx++)
    {
      m_aaiAdaptSR[iDir][iRefIdx] = iSearchRange;
    }
  }

  // initialize motion cost
  for( Int iNum = 0; iNum < AMVP_MAX_NUM_CANDS+1; iNum++)
  {
    for( Int iIdx = 0; iIdx < AMVP_MAX_NUM_CANDS; iIdx++)
    {
      if (iIdx < iNum)
      {
#ifdef CVI_ENABLE_RDO_BIT_EST
        m_auiMVPIdxCost[iIdx][iNum] = 1;
#else
        m_auiMVPIdxCost[iIdx][iNum] = xGetMvpIdxBits(iIdx, iNum);
#endif
      }
      else
      {
        m_auiMVPIdxCost[iIdx][iNum] = MAX_INT;
      }
    }
  }

  const ChromaFormat cform=pcEncCfg->getChromaFormatIdc();
  initTempBuff(cform);

  m_pTempPel = new Pel[maxCUWidth*maxCUHeight];

  const UInt uiNumLayersToAllocate = pcEncCfg->getQuadtreeTULog2MaxSize()-pcEncCfg->getQuadtreeTULog2MinSize()+1;
  const UInt uiNumPartitions = 1<<(maxTotalCUDepth<<1);
  for (UInt ch=0; ch<MAX_NUM_COMPONENT; ch++)
  {
    const UInt csx=::getComponentScaleX(ComponentID(ch), cform);
    const UInt csy=::getComponentScaleY(ComponentID(ch), cform);
    m_ppcQTTempCoeff[ch] = new TCoeff* [uiNumLayersToAllocate];
#if ADAPTIVE_QP_SELECTION
    m_ppcQTTempArlCoeff[ch]  = new TCoeff*[uiNumLayersToAllocate];
#endif
    m_puhQTTempCbf[ch] = new UChar  [uiNumPartitions];

    for (UInt layer = 0; layer < uiNumLayersToAllocate; layer++)
    {
      m_ppcQTTempCoeff[ch][layer] = new TCoeff[(maxCUWidth*maxCUHeight)>>(csx+csy)];
#if ADAPTIVE_QP_SELECTION
      m_ppcQTTempArlCoeff[ch][layer]  = new TCoeff[(maxCUWidth*maxCUHeight)>>(csx+csy) ];
#endif
    }

    m_phQTTempCrossComponentPredictionAlpha[ch]    = new SChar  [uiNumPartitions];
    m_pSharedPredTransformSkip[ch]                 = new Pel   [MAX_CU_SIZE*MAX_CU_SIZE];
    m_pcQTTempTUCoeff[ch]                          = new TCoeff[MAX_CU_SIZE*MAX_CU_SIZE];
#if ADAPTIVE_QP_SELECTION
    m_ppcQTTempTUArlCoeff[ch]                      = new TCoeff[MAX_CU_SIZE*MAX_CU_SIZE];
#endif
    m_puhQTTempTransformSkipFlag[ch]               = new UChar [uiNumPartitions];
  }
  m_puhQTTempTrIdx   = new UChar  [uiNumPartitions];
  m_pcQTTempTComYuv  = new TComYuv[uiNumLayersToAllocate];
  for( UInt ui = 0; ui < uiNumLayersToAllocate; ++ui )
  {
    m_pcQTTempTComYuv[ui].create( maxCUWidth, maxCUHeight, pcEncCfg->getChromaFormatIdc() );
  }
  m_pcQTTempTransformSkipTComYuv.create( maxCUWidth, maxCUHeight, pcEncCfg->getChromaFormatIdc() );
  m_tmpYuvPred.create(MAX_CU_SIZE, MAX_CU_SIZE, pcEncCfg->getChromaFormatIdc());
  m_isInitialized = true;
}


__inline Void TEncSearch::xTZSearchHelp( const TComPattern* const pcPatternKey, IntTZSearchStruct& rcStruct, const Int iSearchX, const Int iSearchY, const UChar ucPointNr, const UInt uiDistance )
{
  Distortion  uiSad = 0;

  const Pel* const  piRefSrch = rcStruct.piRefY + iSearchY * rcStruct.iYStride + iSearchX;

  //-- jclee for using the SAD function pointer
  m_pcRdCost->setDistParam( pcPatternKey, piRefSrch, rcStruct.iYStride,  m_cDistParam );

  setDistParamComp(COMPONENT_Y);

  // distortion
  m_cDistParam.bitDepth = pcPatternKey->getBitDepthY();
  m_cDistParam.m_maximumDistortionForEarlyExit = rcStruct.uiBestSad;

  if((m_pcEncCfg->getRestrictMESampling() == false) && m_pcEncCfg->getMotionEstimationSearchMethod() == MESEARCH_SELECTIVE)
  {
    Int isubShift = 0;
    // motion cost
    Distortion uiBitCost = m_pcRdCost->getCostOfVectorWithPredictor( iSearchX, iSearchY );

    // Skip search if bit cost is already larger than best SAD
    if (uiBitCost < rcStruct.uiBestSad)
    {
      if ( m_cDistParam.iRows > 32 )
      {
        m_cDistParam.iSubShift = 4;
      }
      else if ( m_cDistParam.iRows > 16 )
      {
        m_cDistParam.iSubShift = 3;
      }
      else if ( m_cDistParam.iRows > 8 )
      {
        m_cDistParam.iSubShift = 2;
      }
      else
      {
        m_cDistParam.iSubShift = 1;
      }

      Distortion uiTempSad = m_cDistParam.DistFunc( &m_cDistParam );
      if((uiTempSad + uiBitCost) < rcStruct.uiBestSad)
      {
        uiSad += uiTempSad >>  m_cDistParam.iSubShift;
        while(m_cDistParam.iSubShift > 0)
        {
          isubShift         = m_cDistParam.iSubShift -1;
          m_cDistParam.pOrg = pcPatternKey->getROIY() + (pcPatternKey->getPatternLStride() << isubShift);
          m_cDistParam.pCur = piRefSrch + (rcStruct.iYStride << isubShift);
          uiTempSad = m_cDistParam.DistFunc( &m_cDistParam );
          uiSad += uiTempSad >>  m_cDistParam.iSubShift;
          if(((uiSad << isubShift) + uiBitCost) > rcStruct.uiBestSad)
          {
            break;
          }

          m_cDistParam.iSubShift--;
        }

        if(m_cDistParam.iSubShift == 0)
        {
          uiSad += uiBitCost;
          if( uiSad < rcStruct.uiBestSad )
          {
            rcStruct.uiBestSad      = uiSad;
            rcStruct.iBestX         = iSearchX;
            rcStruct.iBestY         = iSearchY;
            rcStruct.uiBestDistance = uiDistance;
            rcStruct.uiBestRound    = 0;
            rcStruct.ucPointNr      = ucPointNr;
            m_cDistParam.m_maximumDistortionForEarlyExit = uiSad;
          }
        }
      }
    }
  }
  else
  {
    // fast encoder decision: use subsampled SAD when rows > 8 for integer ME
    if ( m_pcEncCfg->getFastInterSearchMode()==FASTINTERSEARCH_MODE1 || m_pcEncCfg->getFastInterSearchMode()==FASTINTERSEARCH_MODE3 )
    {
      if ( m_cDistParam.iRows > 8 )
      {
        m_cDistParam.iSubShift = 1;
      }
    }

    uiSad = m_cDistParam.DistFunc( &m_cDistParam );

    // only add motion cost if uiSad is smaller than best. Otherwise pointless
    // to add motion cost.
    if( uiSad < rcStruct.uiBestSad )
    {
      // motion cost
      uiSad += m_pcRdCost->getCostOfVectorWithPredictor( iSearchX, iSearchY );

      if( uiSad < rcStruct.uiBestSad )
      {
        rcStruct.uiBestSad      = uiSad;
        rcStruct.iBestX         = iSearchX;
        rcStruct.iBestY         = iSearchY;
        rcStruct.uiBestDistance = uiDistance;
        rcStruct.uiBestRound    = 0;
        rcStruct.ucPointNr      = ucPointNr;
        m_cDistParam.m_maximumDistortionForEarlyExit = uiSad;
      }
    }
  }
}

__inline Void TEncSearch::xTZ2PointSearch( const TComPattern* const pcPatternKey, IntTZSearchStruct& rcStruct, const TComMv* const pcMvSrchRngLT, const TComMv* const pcMvSrchRngRB )
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();

  // 2 point search,                   //   1 2 3
  // check only the 2 untested points  //   4 0 5
  // around the start point            //   6 7 8
  Int iStartX = rcStruct.iBestX;
  Int iStartY = rcStruct.iBestY;
  switch( rcStruct.ucPointNr )
  {
    case 1:
    {
      if ( (iStartX - 1) >= iSrchRngHorLeft )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY, 0, 2 );
      }
      if ( (iStartY - 1) >= iSrchRngVerTop )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY - 1, 0, 2 );
      }
    }
      break;
    case 2:
    {
      if ( (iStartY - 1) >= iSrchRngVerTop )
      {
        if ( (iStartX - 1) >= iSrchRngHorLeft )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY - 1, 0, 2 );
        }
        if ( (iStartX + 1) <= iSrchRngHorRight )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY - 1, 0, 2 );
        }
      }
    }
      break;
    case 3:
    {
      if ( (iStartY - 1) >= iSrchRngVerTop )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY - 1, 0, 2 );
      }
      if ( (iStartX + 1) <= iSrchRngHorRight )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY, 0, 2 );
      }
    }
      break;
    case 4:
    {
      if ( (iStartX - 1) >= iSrchRngHorLeft )
      {
        if ( (iStartY + 1) <= iSrchRngVerBottom )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY + 1, 0, 2 );
        }
        if ( (iStartY - 1) >= iSrchRngVerTop )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY - 1, 0, 2 );
        }
      }
    }
      break;
    case 5:
    {
      if ( (iStartX + 1) <= iSrchRngHorRight )
      {
        if ( (iStartY - 1) >= iSrchRngVerTop )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY - 1, 0, 2 );
        }
        if ( (iStartY + 1) <= iSrchRngVerBottom )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY + 1, 0, 2 );
        }
      }
    }
      break;
    case 6:
    {
      if ( (iStartX - 1) >= iSrchRngHorLeft )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY , 0, 2 );
      }
      if ( (iStartY + 1) <= iSrchRngVerBottom )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY + 1, 0, 2 );
      }
    }
      break;
    case 7:
    {
      if ( (iStartY + 1) <= iSrchRngVerBottom )
      {
        if ( (iStartX - 1) >= iSrchRngHorLeft )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX - 1, iStartY + 1, 0, 2 );
        }
        if ( (iStartX + 1) <= iSrchRngHorRight )
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY + 1, 0, 2 );
        }
      }
    }
      break;
    case 8:
    {
      if ( (iStartX + 1) <= iSrchRngHorRight )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX + 1, iStartY, 0, 2 );
      }
      if ( (iStartY + 1) <= iSrchRngVerBottom )
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iStartY + 1, 0, 2 );
      }
    }
      break;
    default:
    {
      assert( false );
    }
      break;
  } // switch( rcStruct.ucPointNr )
}




__inline Void TEncSearch::xTZ8PointSquareSearch( const TComPattern* const pcPatternKey, IntTZSearchStruct& rcStruct, const TComMv* const pcMvSrchRngLT, const TComMv* const pcMvSrchRngRB, const Int iStartX, const Int iStartY, const Int iDist )
{
  const Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  const Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  const Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  const Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();

  // 8 point search,                   //   1 2 3
  // search around the start point     //   4 0 5
  // with the required  distance       //   6 7 8
  assert( iDist != 0 );
  const Int iTop        = iStartY - iDist;
  const Int iBottom     = iStartY + iDist;
  const Int iLeft       = iStartX - iDist;
  const Int iRight      = iStartX + iDist;
  rcStruct.uiBestRound += 1;

  if ( iTop >= iSrchRngVerTop ) // check top
  {
    if ( iLeft >= iSrchRngHorLeft ) // check top left
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iTop, 1, iDist );
    }
    // top middle
    xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 2, iDist );

    if ( iRight <= iSrchRngHorRight ) // check top right
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iRight, iTop, 3, iDist );
    }
  } // check top
  if ( iLeft >= iSrchRngHorLeft ) // check middle left
  {
    xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 4, iDist );
  }
  if ( iRight <= iSrchRngHorRight ) // check middle right
  {
    xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 5, iDist );
  }
  if ( iBottom <= iSrchRngVerBottom ) // check bottom
  {
    if ( iLeft >= iSrchRngHorLeft ) // check bottom left
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iBottom, 6, iDist );
    }
    // check bottom middle
    xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist );

    if ( iRight <= iSrchRngHorRight ) // check bottom right
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iRight, iBottom, 8, iDist );
    }
  } // check bottom
}




__inline Void TEncSearch::xTZ8PointDiamondSearch( const TComPattern*const  pcPatternKey,
                                                  IntTZSearchStruct& rcStruct,
                                                  const TComMv*const  pcMvSrchRngLT,
                                                  const TComMv*const  pcMvSrchRngRB,
                                                  const Int iStartX,
                                                  const Int iStartY,
                                                  const Int iDist,
                                                  const Bool bCheckCornersAtDist1 )
{
  const Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  const Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  const Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  const Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();

  // 8 point search,                   //   1 2 3
  // search around the start point     //   4 0 5
  // with the required  distance       //   6 7 8
  assert ( iDist != 0 );
  const Int iTop        = iStartY - iDist;
  const Int iBottom     = iStartY + iDist;
  const Int iLeft       = iStartX - iDist;
  const Int iRight      = iStartX + iDist;
  rcStruct.uiBestRound += 1;

  if ( iDist == 1 )
  {
    if ( iTop >= iSrchRngVerTop ) // check top
    {
      if (bCheckCornersAtDist1)
      {
        if ( iLeft >= iSrchRngHorLeft) // check top-left
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iTop, 1, iDist );
        }
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 2, iDist );
        if ( iRight <= iSrchRngHorRight ) // check middle right
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iRight, iTop, 3, iDist );
        }
      }
      else
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 2, iDist );
      }
    }
    if ( iLeft >= iSrchRngHorLeft ) // check middle left
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 4, iDist );
    }
    if ( iRight <= iSrchRngHorRight ) // check middle right
    {
      xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 5, iDist );
    }
    if ( iBottom <= iSrchRngVerBottom ) // check bottom
    {
      if (bCheckCornersAtDist1)
      {
        if ( iLeft >= iSrchRngHorLeft) // check top-left
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iBottom, 6, iDist );
        }
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist );
        if ( iRight <= iSrchRngHorRight ) // check middle right
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iRight, iBottom, 8, iDist );
        }
      }
      else
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist );
      }
    }
  }
  else
  {
    if ( iDist <= 8 )
    {
      const Int iTop_2      = iStartY - (iDist>>1);
      const Int iBottom_2   = iStartY + (iDist>>1);
      const Int iLeft_2     = iStartX - (iDist>>1);
      const Int iRight_2    = iStartX + (iDist>>1);

      if (  iTop >= iSrchRngVerTop && iLeft >= iSrchRngHorLeft &&
          iRight <= iSrchRngHorRight && iBottom <= iSrchRngVerBottom ) // check border
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX,  iTop,      2, iDist    );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2,  iTop_2,    1, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iTop_2,    3, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft,    iStartY,   4, iDist    );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight,   iStartY,   5, iDist    );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2,  iBottom_2, 6, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iBottom_2, 8, iDist>>1 );
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX,  iBottom,   7, iDist    );
      }
      else // check border
      {
        if ( iTop >= iSrchRngVerTop ) // check top
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 2, iDist );
        }
        if ( iTop_2 >= iSrchRngVerTop ) // check half top
        {
          if ( iLeft_2 >= iSrchRngHorLeft ) // check half left
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2, iTop_2, 1, (iDist>>1) );
          }
          if ( iRight_2 <= iSrchRngHorRight ) // check half right
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iTop_2, 3, (iDist>>1) );
          }
        } // check half top
        if ( iLeft >= iSrchRngHorLeft ) // check left
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 4, iDist );
        }
        if ( iRight <= iSrchRngHorRight ) // check right
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 5, iDist );
        }
        if ( iBottom_2 <= iSrchRngVerBottom ) // check half bottom
        {
          if ( iLeft_2 >= iSrchRngHorLeft ) // check half left
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iLeft_2, iBottom_2, 6, (iDist>>1) );
          }
          if ( iRight_2 <= iSrchRngHorRight ) // check half right
          {
            xTZSearchHelp( pcPatternKey, rcStruct, iRight_2, iBottom_2, 8, (iDist>>1) );
          }
        } // check half bottom
        if ( iBottom <= iSrchRngVerBottom ) // check bottom
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 7, iDist );
        }
      } // check border
    }
    else // iDist > 8
    {
      if ( iTop >= iSrchRngVerTop && iLeft >= iSrchRngHorLeft &&
          iRight <= iSrchRngHorRight && iBottom <= iSrchRngVerBottom ) // check border
      {
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop,    0, iDist );
        xTZSearchHelp( pcPatternKey, rcStruct, iLeft,   iStartY, 0, iDist );
        xTZSearchHelp( pcPatternKey, rcStruct, iRight,  iStartY, 0, iDist );
        xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 0, iDist );
        for ( Int index = 1; index < 4; index++ )
        {
          const Int iPosYT = iTop    + ((iDist>>2) * index);
          const Int iPosYB = iBottom - ((iDist>>2) * index);
          const Int iPosXL = iStartX - ((iDist>>2) * index);
          const Int iPosXR = iStartX + ((iDist>>2) * index);
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYT, 0, iDist );
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYT, 0, iDist );
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYB, 0, iDist );
          xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYB, 0, iDist );
        }
      }
      else // check border
      {
        if ( iTop >= iSrchRngVerTop ) // check top
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iTop, 0, iDist );
        }
        if ( iLeft >= iSrchRngHorLeft ) // check left
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iLeft, iStartY, 0, iDist );
        }
        if ( iRight <= iSrchRngHorRight ) // check right
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iRight, iStartY, 0, iDist );
        }
        if ( iBottom <= iSrchRngVerBottom ) // check bottom
        {
          xTZSearchHelp( pcPatternKey, rcStruct, iStartX, iBottom, 0, iDist );
        }
        for ( Int index = 1; index < 4; index++ )
        {
          const Int iPosYT = iTop    + ((iDist>>2) * index);
          const Int iPosYB = iBottom - ((iDist>>2) * index);
          const Int iPosXL = iStartX - ((iDist>>2) * index);
          const Int iPosXR = iStartX + ((iDist>>2) * index);

          if ( iPosYT >= iSrchRngVerTop ) // check top
          {
            if ( iPosXL >= iSrchRngHorLeft ) // check left
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYT, 0, iDist );
            }
            if ( iPosXR <= iSrchRngHorRight ) // check right
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYT, 0, iDist );
            }
          } // check top
          if ( iPosYB <= iSrchRngVerBottom ) // check bottom
          {
            if ( iPosXL >= iSrchRngHorLeft ) // check left
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXL, iPosYB, 0, iDist );
            }
            if ( iPosXR <= iSrchRngHorRight ) // check right
            {
              xTZSearchHelp( pcPatternKey, rcStruct, iPosXR, iPosYB, 0, iDist );
            }
          } // check bottom
        } // for ...
      } // check border
    } // iDist <= 8
  } // iDist == 1
}

Distortion TEncSearch::xPatternRefinement( TComPattern* pcPatternKey,
                                           TComMv baseRefMv,
                                           Int iFrac, TComMv& rcMvFrac,
                                           Bool bAllowUseOfHadamard
                                         )
{
  Distortion  uiDist;
  Distortion  uiDistBest  = std::numeric_limits<Distortion>::max();
  UInt        uiDirecBest = 0;

  Pel*  piRefPos;
  Int iRefStride = m_filteredBlock[0][0].getStride(COMPONENT_Y);

  m_pcRdCost->setDistParam( pcPatternKey, m_filteredBlock[0][0].getAddr(COMPONENT_Y), iRefStride, 1, m_cDistParam, m_pcEncCfg->getUseHADME() && bAllowUseOfHadamard );

  const TComMv* pcMvRefine = (iFrac == 2 ? s_acMvRefineH : s_acMvRefineQ);

#if MCTS_ENC_CHECK
  UInt maxRefinements = 9;
  Int mvShift = 2;

  // filter length of sub-sample generation filter to be considered
  const UInt LumaLTSampleOffset = 3;
  const UInt LumaRBSampleOffset = 4;

  if (m_pcEncCfg->getTMCTSSEITileConstraint())
  {
    // if close to tile borders
    if ( pcPatternKey->getROIYPosX() + (baseRefMv.getHor() >> mvShift ) < pcPatternKey->getTileLeftTopPelPosX() +  LumaLTSampleOffset ||
         pcPatternKey->getROIYPosY() + (baseRefMv.getVer() >> mvShift ) < pcPatternKey->getTileLeftTopPelPosY() +  LumaLTSampleOffset ||
         pcPatternKey->getROIYPosX() + (baseRefMv.getHor() >> mvShift)  > pcPatternKey->getTileRightBottomPelPosX() - pcPatternKey->getROIYWidth() - LumaRBSampleOffset ||
         pcPatternKey->getROIYPosY() + (baseRefMv.getVer() >> mvShift)  > pcPatternKey->getTileRightBottomPelPosY() - pcPatternKey->getROIYHeight() - LumaRBSampleOffset
       )
    {
      // only allow full pel positions to avoid filter dependency
      maxRefinements = 1;
    }
  }

  for (UInt i = 0; i < maxRefinements; i++)
#else
#ifdef SIG_FME
  vector <Distortion> SATD_sum(9);
  vector <Distortion> MV_cost(9);
#endif
  for (UInt i = 0; i < 9; i++)
#endif
  {
    TComMv cMvTest = pcMvRefine[i];
    cMvTest += baseRefMv;

    Int horVal = cMvTest.getHor() * iFrac;
    Int verVal = cMvTest.getVer() * iFrac;
    piRefPos = m_filteredBlock[ verVal & 3 ][ horVal & 3 ].getAddr(COMPONENT_Y);
    if ( horVal == 2 && ( verVal & 1 ) == 0 )
    {
      piRefPos += 1;
    }
    if ( ( horVal & 1 ) == 0 && verVal == 2 )
    {
      piRefPos += iRefStride;
    }
    cMvTest = pcMvRefine[i];
    cMvTest += rcMvFrac;

    setDistParamComp(COMPONENT_Y);

    m_cDistParam.pCur = piRefPos;
    m_cDistParam.bitDepth = pcPatternKey->getBitDepthY();
    uiDist = m_cDistParam.DistFunc( &m_cDistParam );
#ifdef SIG_FME
    if (g_sigdump.fme) {
      sig_ctx *ctx_dat_out = &g_sigpool.fme_dat_out_ctx[g_sigpool.fme_cur_is_16_blk];
      sig_ctx *ctx_dat_txt_out = &g_sigpool.fme_dat_out_txt_ctx[g_sigpool.fme_cur_is_16_blk];
      sigdump_output_fprint(ctx_dat_txt_out, "//%s,%d,%d i = %d\n", iFrac == 2 ? "HALF" : "QUART", cMvTest.getHor(), cMvTest.getVer(), i);
      //Interpolation_dat
      for (int m = 0; m < pcPatternKey->getROIYHeight(); m++) {
        Pel* src_p = piRefPos + m * iRefStride;
        for (int n = 0; n < pcPatternKey->getROIYWidth(); n++) {
          sigdump_output_bin(ctx_dat_out, (unsigned char *)(src_p + n), 1);
          sigdump_output_fprint(ctx_dat_txt_out,"%02x", *(src_p + n));
        }
        sigdump_output_fprint(ctx_dat_txt_out,"\n");
      }
      SATD_sum[i] = uiDist;
      MV_cost[i] = m_pcRdCost->getCostOfVectorWithPredictor( cMvTest.getHor(), cMvTest.getVer() );
    }
#endif
    uiDist += m_pcRdCost->getCostOfVectorWithPredictor( cMvTest.getHor(), cMvTest.getVer() );

    if ( uiDist < uiDistBest )
    {
      uiDistBest  = uiDist;
      uiDirecBest = i;
      m_cDistParam.m_maximumDistortionForEarlyExit = uiDist;
    }
  }

#ifdef SIG_FME
  if (g_sigdump.fme) {
    sig_ctx *ctx_cmd_in = &g_sigpool.fme_cmd_in_ctx[g_sigpool.fme_cur_is_16_blk];
    if (iFrac == 2)
      sigdump_output_fprint(ctx_cmd_in, "sqrt_lambda = %x\n", m_pcRdCost->getFixpointSADSqrtLambda());
    sigdump_output_fprint(ctx_cmd_in, "%s_SATD_sum = ", iFrac == 2 ? "HALF" : "QUART");
    for (int i = 0; i < SATD_sum.size(); i++)
      sigdump_output_fprint(ctx_cmd_in, "%x%s", SATD_sum[i], (i == SATD_sum.size() - 1) ? "" : ",");
    sigdump_output_fprint(ctx_cmd_in, "\n%s_MV_cost = ", iFrac == 2 ? "HALF" : "QUART");
    for (int i = 0; i < MV_cost.size(); i++)
      sigdump_output_fprint(ctx_cmd_in, "%x%s", MV_cost[i], (i == MV_cost.size() - 1) ? "" : ",");
    sigdump_output_fprint(ctx_cmd_in, "\n%s_Best_Pos = %d\n", iFrac == 2 ? "HALF" : "QUART", uiDirecBest);
    if (iFrac != 2) {
      sigdump_output_fprint(ctx_cmd_in, "Best_fmvx = %x\n", (rcMvFrac.getHor() + pcMvRefine[uiDirecBest].getHor()) * iFrac);
      sigdump_output_fprint(ctx_cmd_in, "Best_fmvy = %x\n", (rcMvFrac.getVer() + pcMvRefine[uiDirecBest].getVer()) * iFrac);
    }
    sigdump_output_fprint(&g_sigpool.fme_dat_out_txt_ctx[g_sigpool.fme_cur_is_16_blk], "%s_Best_Pos = %d\n", iFrac == 2 ? "HALF" : "QUART", uiDirecBest);
  }
#endif
  rcMvFrac = pcMvRefine[uiDirecBest];

  return uiDistBest;
}



Void
TEncSearch::xEncSubdivCbfQT(TComTU      &rTu,
                            Bool         bLuma,
                            Bool         bChroma )
{
  TComDataCU* pcCU=rTu.getCU();
  const UInt uiAbsPartIdx         = rTu.GetAbsPartIdxTU();
  const UInt uiTrDepth            = rTu.GetTransformDepthRel();
  const UInt uiTrMode             = pcCU->getTransformIdx( uiAbsPartIdx );
  const UInt uiSubdiv             = ( uiTrMode > uiTrDepth ? 1 : 0 );
  const UInt uiLog2LumaTrafoSize  = rTu.GetLog2LumaTrSize();

  if( pcCU->isIntra(0) && pcCU->getPartitionSize(0) == SIZE_NxN && uiTrDepth == 0 )
  {
    assert( uiSubdiv );
  }
  else if( uiLog2LumaTrafoSize > pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() )
  {
    assert( uiSubdiv );
  }
  else if( uiLog2LumaTrafoSize == pcCU->getSlice()->getSPS()->getQuadtreeTULog2MinSize() )
  {
    assert( !uiSubdiv );
  }
  else if( uiLog2LumaTrafoSize == pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
  {
    assert( !uiSubdiv );
  }
  else
  {
    assert( uiLog2LumaTrafoSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );
    if( bLuma )
    {
      m_pcEntropyCoder->encodeTransformSubdivFlag( uiSubdiv, 5 - uiLog2LumaTrafoSize );
    }
  }

  if ( bChroma )
  {
    const UInt numberValidComponents = getNumberValidComponents(rTu.GetChromaFormat());
    for (UInt ch=COMPONENT_Cb; ch<numberValidComponents; ch++)
    {
      const ComponentID compID=ComponentID(ch);
      if( rTu.ProcessingAllQuadrants(compID) && (uiTrDepth==0 || pcCU->getCbf( uiAbsPartIdx, compID, uiTrDepth-1 ) ))
      {
        m_pcEntropyCoder->encodeQtCbf(rTu, compID, (uiSubdiv == 0));
      }
    }
  }

  if( uiSubdiv )
  {
    TComTURecurse tuRecurse(rTu, false);
    do
    {
      xEncSubdivCbfQT( tuRecurse, bLuma, bChroma );
    } while (tuRecurse.nextSection(rTu));
  }
  else
  {
    //===== Cbfs =====
    if( bLuma )
    {
      m_pcEntropyCoder->encodeQtCbf( rTu, COMPONENT_Y, true );
    }
  }
}




Void
TEncSearch::xEncCoeffQT(TComTU &rTu,
                        const ComponentID  component,
                        Bool         bRealCoeff )
{
  TComDataCU* pcCU=rTu.getCU();
  const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();
  const UInt uiTrDepth=rTu.GetTransformDepthRel();

  const UInt  uiTrMode        = pcCU->getTransformIdx( uiAbsPartIdx );
  const UInt  uiSubdiv        = ( uiTrMode > uiTrDepth ? 1 : 0 );

  if( uiSubdiv )
  {
    TComTURecurse tuRecurseChild(rTu, false);
    do
    {
      xEncCoeffQT( tuRecurseChild, component, bRealCoeff );
    } while (tuRecurseChild.nextSection(rTu) );
  }
  else if (rTu.ProcessComponentSection(component))
  {
    //===== coefficients =====
    const UInt  uiLog2TrafoSize = rTu.GetLog2LumaTrSize();
    UInt    uiCoeffOffset   = rTu.getCoefficientOffset(component);
    UInt    uiQTLayer       = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrafoSize;
    TCoeff* pcCoeff         = bRealCoeff ? pcCU->getCoeff(component) : m_ppcQTTempCoeff[component][uiQTLayer];

    if (isChroma(component) && (pcCU->getCbf( rTu.GetAbsPartIdxTU(), COMPONENT_Y, uiTrMode ) != 0) && pcCU->getSlice()->getPPS()->getPpsRangeExtension().getCrossComponentPredictionEnabledFlag() )
    {
      m_pcEntropyCoder->encodeCrossComponentPrediction( rTu, component );
    }

    m_pcEntropyCoder->encodeCoeffNxN( rTu, pcCoeff+uiCoeffOffset, component );
  }
}




Void
TEncSearch::xEncIntraHeader( TComDataCU*  pcCU,
                            UInt         uiTrDepth,
                            UInt         uiAbsPartIdx,
                            Bool         bLuma,
                            Bool         bChroma )
{
  if( bLuma )
  {
    // CU header
    if( uiAbsPartIdx == 0 )
    {
      if( !pcCU->getSlice()->isIntra() )
      {
        if (pcCU->getSlice()->getPPS()->getTransquantBypassEnabledFlag())
        {
          m_pcEntropyCoder->encodeCUTransquantBypassFlag( pcCU, 0, true );
        }
        m_pcEntropyCoder->encodeSkipFlag( pcCU, 0, true );
        m_pcEntropyCoder->encodePredMode( pcCU, 0, true );
      }
      m_pcEntropyCoder  ->encodePartSize( pcCU, 0, pcCU->getDepth(0), true );

      if (pcCU->isIntra(0) && pcCU->getPartitionSize(0) == SIZE_2Nx2N )
      {
        m_pcEntropyCoder->encodeIPCMInfo( pcCU, 0, true );

        if ( pcCU->getIPCMFlag (0))
        {
          return;
        }
      }
    }
    // luma prediction mode
    if( pcCU->getPartitionSize(0) == SIZE_2Nx2N )
    {
      if (uiAbsPartIdx==0)
      {
        m_pcEntropyCoder->encodeIntraDirModeLuma ( pcCU, 0 );
      }
    }
    else
    {
      UInt uiQNumParts = pcCU->getTotalNumPart() >> 2;
      if (uiTrDepth>0 && (uiAbsPartIdx%uiQNumParts)==0)
      {
        m_pcEntropyCoder->encodeIntraDirModeLuma ( pcCU, uiAbsPartIdx );
      }
    }
  }

  if( bChroma )
  {
    if( pcCU->getPartitionSize(0) == SIZE_2Nx2N || !enable4ChromaPUsInIntraNxNCU(pcCU->getPic()->getChromaFormat()))
    {
      if(uiAbsPartIdx==0)
      {
         m_pcEntropyCoder->encodeIntraDirModeChroma ( pcCU, uiAbsPartIdx );
      }
    }
    else
    {
      UInt uiQNumParts = pcCU->getTotalNumPart() >> 2;
      assert(uiTrDepth>0);
      if ((uiAbsPartIdx%uiQNumParts)==0)
      {
        m_pcEntropyCoder->encodeIntraDirModeChroma ( pcCU, uiAbsPartIdx );
      }
    }
  }
}




UInt
TEncSearch::xGetIntraBitsQT(TComTU &rTu,
                            Bool         bLuma,
                            Bool         bChroma,
                            Bool         bRealCoeff /* just for test */ )
{
  TComDataCU* pcCU=rTu.getCU();
  const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();
  const UInt uiTrDepth=rTu.GetTransformDepthRel();
  m_pcEntropyCoder->resetBits();
  xEncIntraHeader ( pcCU, uiTrDepth, uiAbsPartIdx, bLuma, bChroma );

#ifdef SIG_IAPU
  g_sigpool.iapu_st.bin_count = m_pcEntropyCoder->getNumberOfWrittenBitsFrac();
#endif

  xEncSubdivCbfQT ( rTu, bLuma, bChroma );

  if( bLuma )
  {
    xEncCoeffQT   ( rTu, COMPONENT_Y,      bRealCoeff );
  }
  if( bChroma )
  {
    xEncCoeffQT   ( rTu, COMPONENT_Cb,  bRealCoeff );
    xEncCoeffQT   ( rTu, COMPONENT_Cr,  bRealCoeff );
  }

#ifdef CVI_ENABLE_RDO_BIT_EST
  if (getRDOBitEstMode()!=HM_ORIGINAL && getRDOSyntaxEstMode()!= HM_ORIGINAL)
  {
    PartSize part = rTu.getCU()->getPartitionSize(0);
    if (part == SIZE_NxN)
    {
      UInt frac_bits = (UInt)(m_pcEntropyCoder->getNumberOfWrittenBitsFrac());
      return frac_bits;
    }
  }
#endif
  UInt   uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
  return uiBits;
}

UInt TEncSearch::xGetIntraBitsQTChroma(TComTU &rTu,
                                       ComponentID compID,
                                       Bool         bRealCoeff /* just for test */ )
{
  m_pcEntropyCoder->resetBits();
  xEncCoeffQT   ( rTu, compID,  bRealCoeff );
  UInt   uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
  return uiBits;
}

Void TEncSearch::xIntraCodingTUBlock(       TComYuv*    pcOrgYuv,
                                            TComYuv*    pcPredYuv,
                                            TComYuv*    pcResiYuv,
                                            Pel         resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE],
                                      const Bool        checkCrossCPrediction,
                                            Distortion& ruiDist,
                                      const ComponentID compID,
                                            TComTU&     rTu
                                      DEBUG_STRING_FN_DECLARE(sDebug)
                                           ,Int         default0Save1Load2
                                     )
{
  if (!rTu.ProcessComponentSection(compID))
  {
    return;
  }
  const Bool           bIsLuma          = isLuma(compID);
  const TComRectangle &rect             = rTu.getRect(compID);
        TComDataCU    *pcCU             = rTu.getCU();
  const UInt           uiAbsPartIdx     = rTu.GetAbsPartIdxTU();
  const TComSPS       &sps              = *(pcCU->getSlice()->getSPS());

  const UInt           uiTrDepth        = rTu.GetTransformDepthRelAdj(compID);
  const UInt           uiFullDepth      = rTu.GetTransformDepthTotal();
  const UInt           uiLog2TrSize     = rTu.GetLog2LumaTrSize();
  const ChromaFormat   chFmt            = pcOrgYuv->getChromaFormat();
  const ChannelType    chType           = toChannelType(compID);
  const Int            bitDepth         = sps.getBitDepth(chType);

  const UInt           uiWidth          = rect.width;
  const UInt           uiHeight         = rect.height;
  const UInt           uiStride         = pcOrgYuv ->getStride (compID);
        Pel           *piOrg            = pcOrgYuv ->getAddr( compID, uiAbsPartIdx );
        Pel           *piPred           = pcPredYuv->getAddr( compID, uiAbsPartIdx );
        Pel           *piResi           = pcResiYuv->getAddr( compID, uiAbsPartIdx );
        Pel           *piReco           = pcPredYuv->getAddr( compID, uiAbsPartIdx );
  const UInt           uiQTLayer        = sps.getQuadtreeTULog2MaxSize() - uiLog2TrSize;
        Pel           *piRecQt          = m_pcQTTempTComYuv[ uiQTLayer ].getAddr( compID, uiAbsPartIdx );
  const UInt           uiRecQtStride    = m_pcQTTempTComYuv[ uiQTLayer ].getStride(compID);
  const UInt           uiZOrder         = pcCU->getZorderIdxInCtu() + uiAbsPartIdx;
        Pel           *piRecIPred       = pcCU->getPic()->getPicYuvRec()->getAddr( compID, pcCU->getCtuRsAddr(), uiZOrder );
        UInt           uiRecIPredStride = pcCU->getPic()->getPicYuvRec()->getStride  ( compID );
        TCoeff        *pcCoeff          = m_ppcQTTempCoeff[compID][uiQTLayer] + rTu.getCoefficientOffset(compID);
        Bool           useTransformSkip = pcCU->getTransformSkip(uiAbsPartIdx, compID);

#if ADAPTIVE_QP_SELECTION
        TCoeff        *pcArlCoeff       = m_ppcQTTempArlCoeff[compID][ uiQTLayer ] + rTu.getCoefficientOffset(compID);
#endif

  const UInt           uiChPredMode     = pcCU->getIntraDir( chType, uiAbsPartIdx );
  const UInt           partsPerMinCU    = 1<<(2*(sps.getMaxTotalCUDepth() - sps.getLog2DiffMaxMinCodingBlockSize()));
  const UInt           uiChCodedMode    = (uiChPredMode==DM_CHROMA_IDX && !bIsLuma) ? pcCU->getIntraDir(CHANNEL_TYPE_LUMA, getChromasCorrespondingPULumaIdx(uiAbsPartIdx, chFmt, partsPerMinCU)) : uiChPredMode;
  const UInt           uiChFinalMode    = ((chFmt == CHROMA_422)       && !bIsLuma) ? g_chroma422IntraAngleMappingTable[uiChCodedMode] : uiChCodedMode;

  const Int            blkX                                 = g_auiRasterToPelX[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
  const Int            blkY                                 = g_auiRasterToPelY[ g_auiZscanToRaster[ uiAbsPartIdx ] ];
  const Int            bufferOffset                         = blkX + (blkY * MAX_CU_SIZE);
        Pel  *const    encoderLumaResidual                  = resiLuma[RESIDUAL_ENCODER_SIDE ] + bufferOffset;
        Pel  *const    reconstructedLumaResidual            = resiLuma[RESIDUAL_RECONSTRUCTED] + bufferOffset;
  const Bool           bUseCrossCPrediction                 = isChroma(compID) && (uiChPredMode == DM_CHROMA_IDX) && checkCrossCPrediction;
  const Bool           bUseReconstructedResidualForEstimate = m_pcEncCfg->getUseReconBasedCrossCPredictionEstimate();
        Pel *const     lumaResidualForEstimate              = bUseReconstructedResidualForEstimate ? reconstructedLumaResidual : encoderLumaResidual;

#if DEBUG_STRING
  const Int debugPredModeMask=DebugStringGetPredModeMask(MODE_INTRA);
#endif

  //===== init availability pattern =====
  DEBUG_STRING_NEW(sTemp)

#if !DEBUG_STRING
  if( default0Save1Load2 != 2 )
#endif
  {
    const Bool bUseFilteredPredictions=TComPrediction::filteringIntraReferenceSamples(compID, uiChFinalMode, uiWidth, uiHeight, chFmt, sps.getSpsRangeExtension().getIntraSmoothingDisabledFlag());

    initIntraPatternChType( rTu, compID, bUseFilteredPredictions DEBUG_STRING_PASS_INTO(sDebug) );

    //===== get prediction signal =====
    predIntraAng( compID, uiChFinalMode, piOrg, uiStride, piPred, uiStride, rTu, bUseFilteredPredictions );

    // save prediction
    if( default0Save1Load2 == 1 )
    {
      Pel*  pPred   = piPred;
      Pel*  pPredBuf = m_pSharedPredTransformSkip[compID];
      Int k = 0;
      for( UInt uiY = 0; uiY < uiHeight; uiY++ )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          pPredBuf[ k ++ ] = pPred[ uiX ];
        }
        pPred += uiStride;
      }
    }
  }
#if !DEBUG_STRING
  else
  {
    // load prediction
    Pel*  pPred   = piPred;
    Pel*  pPredBuf = m_pSharedPredTransformSkip[compID];
    Int k = 0;
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pPred[ uiX ] = pPredBuf[ k ++ ];
      }
      pPred += uiStride;
    }
  }
#endif

#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    sig_rru_copy_intp_temp_buf(compID, piPred, uiStride, g_sigpool.p_rru_intp_temp, uiWidth, uiWidth);
  }
#endif //~SIG_RRU

  //===== get residual signal =====
  {
    // get residual
    Pel*  pOrg    = piOrg;
    Pel*  pPred   = piPred;
    Pel*  pResi   = piResi;

    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        pResi[ uiX ] = pOrg[ uiX ] - pPred[ uiX ];
      }

      pOrg  += uiStride;
      pResi += uiStride;
      pPred += uiStride;
    }
  }

  if (pcCU->getSlice()->getPPS()->getPpsRangeExtension().getCrossComponentPredictionEnabledFlag())
  {
    if (bUseCrossCPrediction)
    {
      if (xCalcCrossComponentPredictionAlpha( rTu, compID, lumaResidualForEstimate, piResi, uiWidth, uiHeight, MAX_CU_SIZE, uiStride ) == 0)
      {
        return;
      }
      TComTrQuant::crossComponentPrediction ( rTu, compID, reconstructedLumaResidual, piResi, piResi, uiWidth, uiHeight, MAX_CU_SIZE, uiStride, uiStride, false );
    }
    else if (isLuma(compID) && !bUseReconstructedResidualForEstimate)
    {
      xStoreCrossComponentPredictionResult( encoderLumaResidual, piResi, rTu, 0, 0, MAX_CU_SIZE, uiStride );
    }
  }

  //===== transform and quantization =====
  //--- init rate estimation arrays for RDOQ ---
  if( useTransformSkip ? m_pcEncCfg->getUseRDOQTS() : m_pcEncCfg->getUseRDOQ() )
  {
    COEFF_SCAN_TYPE scanType = COEFF_SCAN_TYPE(pcCU->getCoefScanIdx(uiAbsPartIdx, uiWidth, uiHeight, compID));
    m_pcEntropyCoder->estimateBit( m_pcTrQuant->m_pcEstBitsSbac, uiWidth, uiHeight, chType, scanType );
  }

  //--- transform and quantization ---
  TCoeff uiAbsSum = 0;
  if (bIsLuma)
  {
    pcCU       ->setTrIdxSubParts ( uiTrDepth, uiAbsPartIdx, uiFullDepth );
  }

  const QpParam cQP(*pcCU, compID);

#if RDOQ_CHROMA_LAMBDA
  m_pcTrQuant->selectLambda     (compID);
#endif

  m_pcTrQuant->transformNxN     ( rTu, compID, piResi, uiStride, pcCoeff,
#if ADAPTIVE_QP_SELECTION
    pcArlCoeff,
#endif
    uiAbsSum, cQP
    );

#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    const TComRectangle luma_tu_rect = rTu.getRect(COMPONENT_Y);
    if (luma_tu_rect.width == 4)
    {
      int idx = (int)(compID);
      int *p_dst = g_sigpool.p_rru_i4_resi_temp[idx];
      memcpy(p_dst, pcCoeff, 16 * sizeof(TCoeff));
    }
    memset(g_sigpool.p_rru_iq_out, 0, sizeof(int) * 256);
  }
#endif //~SIG_RRU
#ifdef SIG_IAPU
  if (g_sigdump.iapu)   //reference to rru above
  {
    const TComRectangle luma_tu_rect = rTu.getRect(COMPONENT_Y);
    if (luma_tu_rect.width == 4){
        int idx = (int)(compID);
        int *p_dst = g_sigpool.iapu_st.p_iapu_i4_coef_temp[idx];
        memcpy(p_dst, pcCoeff, 16 * sizeof(TCoeff));
    }
  }
#endif //~SIG_IAPU

  //--- inverse transform ---
#ifdef CVI_ENABLE_RDO_FAST_DISTORTION
  Distortion CVI_SSE = 0;
#endif
#if DEBUG_STRING
  if ( (uiAbsSum > 0) || (DebugOptionList::DebugString_InvTran.getInt()&debugPredModeMask) )
#else
  if ( uiAbsSum > 0 )
#endif
  {
    m_pcTrQuant->invTransformNxN ( rTu, compID, piResi, uiStride, pcCoeff, cQP DEBUG_STRING_PASS_INTO_OPTIONAL(&sDebug, (DebugOptionList::DebugString_InvTran.getInt()&debugPredModeMask)) );
  }
  else
  {
    Pel* pResi = piResi;
    memset( pcCoeff, 0, sizeof( TCoeff ) * uiWidth * uiHeight );
    for( UInt uiY = 0; uiY < uiHeight; uiY++ )
    {
      memset( pResi, 0, sizeof( Pel ) * uiWidth );
      pResi += uiStride;
    }
#ifdef SIG_BIT_EST
    if (g_sigdump.rdo_sse) {
      int *pIQCoeff = g_sigpool.is_intra ? g_sigpool.est_golden_intra[0][compID].IQCoeff :
        g_sigpool.est_golden[sig_pat_blk_size_4(g_sigpool.cu_width)][0].IQCoeff[compID][uiAbsPartIdx / 16];

      for (int i = 0; i < rTu.getRect(compID).height; i++) {
        for (int j = 0; j < rTu.getRect(compID).width; j++) {
          pIQCoeff[rTu.getRect(compID).width * i + j] = 0;
        }
      }
    }
#endif
  }

#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    const TComRectangle tu_rect = rTu.getRect(compID);
    sig_rru_copy_idct_temp_buf(compID, piResi, uiStride, g_sigpool.p_rru_iq_out, uiWidth, tu_rect.x0, tu_rect.y0);
  }
#endif //~SIG_RRU


#ifdef CVI_ENABLE_RDO_FAST_DISTORTION
  if (getRDOFastDistortion() != 0) {
    m_pcTrQuant->GetCVI_SSE( rTu, compID, uiAbsSum, &CVI_SSE);
#if defined(SIG_BIT_EST)
    if (g_sigdump.rdo_sse) {
      g_sigpool.est_golden_intra[0][compID].EST_SSE = CVI_SSE;
      TCoeff* m_plTempCoeff = m_pcTrQuant->getplTempCoeff_cvi();
      int *pTCoeff = g_sigpool.est_golden_intra[0][compID].TCoeff;

      for (int i = 0; i < rTu.getRect(compID).height; i++) {
        for (int j = 0; j < rTu.getRect(compID).width; j++) {
          pTCoeff[i * rTu.getRect(compID).width + j]
            = m_plTempCoeff[rTu.getRect(compID).width * i + j];
        }
      }
    }
#endif
  }
#endif
  //===== reconstruction =====
  {
    Pel* pPred      = piPred;
    Pel* pResi      = piResi;
    Pel* pReco      = piReco;
    Pel* pRecQt     = piRecQt;
    Pel* pRecIPred  = piRecIPred;

    if (pcCU->getSlice()->getPPS()->getPpsRangeExtension().getCrossComponentPredictionEnabledFlag())
    {
      if (bUseCrossCPrediction)
      {
        TComTrQuant::crossComponentPrediction( rTu, compID, reconstructedLumaResidual, piResi, piResi, uiWidth, uiHeight, MAX_CU_SIZE, uiStride, uiStride, true );
      }
      else if (isLuma(compID))
      {
        xStoreCrossComponentPredictionResult( reconstructedLumaResidual, piResi, rTu, 0, 0, MAX_CU_SIZE, uiStride );
      }
    }

 #if DEBUG_STRING
    std::stringstream ss(stringstream::out);
    const Bool bDebugPred=((DebugOptionList::DebugString_Pred.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));
    const Bool bDebugResi=((DebugOptionList::DebugString_Resi.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));
    const Bool bDebugReco=((DebugOptionList::DebugString_Reco.getInt()&debugPredModeMask) && DEBUG_STRING_CHANNEL_CONDITION(compID));

    if (bDebugPred || bDebugResi || bDebugReco)
    {
      ss << "###: " << "CompID: " << compID << " pred mode (ch/fin): " << uiChPredMode << "/" << uiChFinalMode << " absPartIdx: " << rTu.GetAbsPartIdxTU() << "\n";
      for( UInt uiY = 0; uiY < uiHeight; uiY++ )
      {
        ss << "###: ";
        if (bDebugPred)
        {
          ss << " - pred: ";
          for( UInt uiX = 0; uiX < uiWidth; uiX++ )
          {
            ss << pPred[ uiX ] << ", ";
          }
        }
        if (bDebugResi)
        {
          ss << " - resi: ";
        }
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          if (bDebugResi)
          {
            ss << pResi[ uiX ] << ", ";
          }
          pReco    [ uiX ] = Pel(ClipBD<Int>( Int(pPred[uiX]) + Int(pResi[uiX]), bitDepth ));
          pRecQt   [ uiX ] = pReco[ uiX ];
          pRecIPred[ uiX ] = pReco[ uiX ];
        }
        if (bDebugReco)
        {
          ss << " - reco: ";
          for( UInt uiX = 0; uiX < uiWidth; uiX++ )
          {
            ss << pReco[ uiX ] << ", ";
          }
        }
        pPred     += uiStride;
        pResi     += uiStride;
        pReco     += uiStride;
        pRecQt    += uiRecQtStride;
        pRecIPred += uiRecIPredStride;
        ss << "\n";
      }
      DEBUG_STRING_APPEND(sDebug, ss.str())
    }
    else
#endif
    {

      for( UInt uiY = 0; uiY < uiHeight; uiY++ )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          pReco    [ uiX ] = Pel(ClipBD<Int>( Int(pPred[uiX]) + Int(pResi[uiX]), bitDepth ));
          pRecQt   [ uiX ] = pReco[ uiX ];
          pRecIPred[ uiX ] = pReco[ uiX ];
        }
        pPred     += uiStride;
        pResi     += uiStride;
        pReco     += uiStride;
        pRecQt    += uiRecQtStride;
        pRecIPred += uiRecIPredStride;
      }
    }
  }

  //===== update distortion =====
#ifdef CVI_ENABLE_RDO_FAST_DISTORTION
  if (getRDOFastDistortion() != 0) {
    if (compID != COMPONENT_Y && rTu.getRect(COMPONENT_Y).width == 4) {
      ruiDist += m_pcRdCost->getCVI_DistPart(compID, CVI_SSE, false);
    }
    else {
      ruiDist += m_pcRdCost->getCVI_DistPart(compID, CVI_SSE);
    }
  } else {
    ruiDist +=  m_pcRdCost->getDistPart( bitDepth, piReco, uiStride, piOrg, uiStride, uiWidth, uiHeight, compID );
  }
#else
  ruiDist += m_pcRdCost->getDistPart( bitDepth, piReco, uiStride, piOrg, uiStride, uiWidth, uiHeight, compID );
#endif
}




Void
TEncSearch::xRecurIntraCodingLumaQT(TComYuv*    pcOrgYuv,
                                    TComYuv*    pcPredYuv,
                                    TComYuv*    pcResiYuv,
                                    Pel         resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE],
                                    Distortion& ruiDistY,
#if HHI_RQT_INTRA_SPEEDUP
                                    Bool        bCheckFirst,
#endif
                                    Double&     dRDCost,
                                    TComTU&     rTu
                                    DEBUG_STRING_FN_DECLARE(sDebug))
{
  TComDataCU   *pcCU          = rTu.getCU();
  const UInt    uiAbsPartIdx  = rTu.GetAbsPartIdxTU();
  const UInt    uiFullDepth   = rTu.GetTransformDepthTotal();
  const UInt    uiTrDepth     = rTu.GetTransformDepthRel();
  const UInt    uiLog2TrSize  = rTu.GetLog2LumaTrSize();
        Bool    bCheckFull    = ( uiLog2TrSize  <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() );
        Bool    bCheckSplit   = ( uiLog2TrSize  >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );

        Pel     resiLumaSplit [NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE];
        Pel     resiLumaSingle[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE];

        Bool    bMaintainResidual[NUMBER_OF_STORED_RESIDUAL_TYPES];
        for (UInt residualTypeIndex = 0; residualTypeIndex < NUMBER_OF_STORED_RESIDUAL_TYPES; residualTypeIndex++)
        {
          bMaintainResidual[residualTypeIndex] = true; //assume true unless specified otherwise
        }

        bMaintainResidual[RESIDUAL_ENCODER_SIDE] = !(m_pcEncCfg->getUseReconBasedCrossCPredictionEstimate());

#if HHI_RQT_INTRA_SPEEDUP
  Int maxTuSize = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
  Int isIntraSlice = (pcCU->getSlice()->getSliceType() == I_SLICE);
  // don't check split if TU size is less or equal to max TU size
  Bool noSplitIntraMaxTuSize = bCheckFull;
  if(m_pcEncCfg->getRDpenalty() && ! isIntraSlice)
  {
    // in addition don't check split if TU size is less or equal to 16x16 TU size for non-intra slice
    noSplitIntraMaxTuSize = ( uiLog2TrSize  <= min(maxTuSize,4) );

    // if maximum RD-penalty don't check TU size 32x32
    if(m_pcEncCfg->getRDpenalty()==2)
    {
      bCheckFull    = ( uiLog2TrSize  <= min(maxTuSize,4));
    }
  }
  if( bCheckFirst && noSplitIntraMaxTuSize )

  {
    bCheckSplit = false;
  }
#else
  Int maxTuSize = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize();
  Int isIntraSlice = (pcCU->getSlice()->getSliceType() == I_SLICE);
  // if maximum RD-penalty don't check TU size 32x32
  if((m_pcEncCfg->getRDpenalty()==2)  && !isIntraSlice)
  {
    bCheckFull    = ( uiLog2TrSize  <= min(maxTuSize,4));
  }
#endif
  Double     dSingleCost                        = MAX_DOUBLE;
  Distortion uiSingleDistLuma                   = 0;
  UInt       uiSingleCbfLuma                    = 0;
  Bool       checkTransformSkip  = pcCU->getSlice()->getPPS()->getUseTransformSkip();
  Int        bestModeId[MAX_NUM_COMPONENT] = { 0, 0, 0};
  checkTransformSkip           &= TUCompRectHasAssociatedTransformSkipFlag(rTu.getRect(COMPONENT_Y), pcCU->getSlice()->getPPS()->getPpsRangeExtension().getLog2MaxTransformSkipBlockSize());
  checkTransformSkip           &= (!pcCU->getCUTransquantBypass(0));

  assert (rTu.ProcessComponentSection(COMPONENT_Y));
  const UInt totalAdjustedDepthChan   = rTu.GetTransformDepthTotalAdj(COMPONENT_Y);

  if ( m_pcEncCfg->getUseTransformSkipFast() )
  {
    checkTransformSkip       &= (pcCU->getPartitionSize(uiAbsPartIdx)==SIZE_NxN);
  }

  if( bCheckFull )
  {
    if(checkTransformSkip == true)
    {
      //----- store original entropy coding status -----
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );

      Distortion singleDistTmpLuma                    = 0;
      UInt       singleCbfTmpLuma                     = 0;
      Double     singleCostTmp                        = 0;
      Int        firstCheckId                         = 0;

      for(Int modeId = firstCheckId; modeId < 2; modeId ++)
      {
        DEBUG_STRING_NEW(sModeString)
        Int  default0Save1Load2 = 0;
        singleDistTmpLuma=0;
        if(modeId == firstCheckId)
        {
          default0Save1Load2 = 1;
        }
        else
        {
          default0Save1Load2 = 2;
        }


        pcCU->setTransformSkipSubParts ( modeId, COMPONENT_Y, uiAbsPartIdx, totalAdjustedDepthChan );
        xIntraCodingTUBlock( pcOrgYuv, pcPredYuv, pcResiYuv, resiLumaSingle, false, singleDistTmpLuma, COMPONENT_Y, rTu DEBUG_STRING_PASS_INTO(sModeString), default0Save1Load2 );

        singleCbfTmpLuma = pcCU->getCbf( uiAbsPartIdx, COMPONENT_Y, uiTrDepth );

        //----- determine rate and r-d cost -----
        if(modeId == 1 && singleCbfTmpLuma == 0)
        {
          //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
          singleCostTmp = MAX_DOUBLE;
        }
        else
        {
          UInt uiSingleBits = xGetIntraBitsQT( rTu, true, false, false );
          singleCostTmp     = m_pcRdCost->calcRdCost( uiSingleBits, singleDistTmpLuma );
        }
        if(singleCostTmp < dSingleCost)
        {
          DEBUG_STRING_SWAP(sDebug, sModeString)
          dSingleCost   = singleCostTmp;
          uiSingleDistLuma = singleDistTmpLuma;
          uiSingleCbfLuma = singleCbfTmpLuma;

          bestModeId[COMPONENT_Y] = modeId;
          if(bestModeId[COMPONENT_Y] == firstCheckId)
          {
            xStoreIntraResultQT(COMPONENT_Y, rTu );
            m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_TEMP_BEST ] );
          }

          if (pcCU->getSlice()->getPPS()->getPpsRangeExtension().getCrossComponentPredictionEnabledFlag())
          {
            const Int xOffset = rTu.getRect( COMPONENT_Y ).x0;
            const Int yOffset = rTu.getRect( COMPONENT_Y ).y0;
            for (UInt storedResidualIndex = 0; storedResidualIndex < NUMBER_OF_STORED_RESIDUAL_TYPES; storedResidualIndex++)
            {
              if (bMaintainResidual[storedResidualIndex])
              {
                xStoreCrossComponentPredictionResult(resiLuma[storedResidualIndex], resiLumaSingle[storedResidualIndex], rTu, xOffset, yOffset, MAX_CU_SIZE, MAX_CU_SIZE);
              }
            }
          }
        }
        if (modeId == firstCheckId)
        {
          m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
        }
      }

      pcCU ->setTransformSkipSubParts ( bestModeId[COMPONENT_Y], COMPONENT_Y, uiAbsPartIdx, totalAdjustedDepthChan );

      if(bestModeId[COMPONENT_Y] == firstCheckId)
      {
        xLoadIntraResultQT(COMPONENT_Y, rTu );
        pcCU->setCbfSubParts  ( uiSingleCbfLuma << uiTrDepth, COMPONENT_Y, uiAbsPartIdx, rTu.GetTransformDepthTotalAdj(COMPONENT_Y) );

        m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiFullDepth ][ CI_TEMP_BEST ] );
      }
    }
    else
    {
      //----- store original entropy coding status -----
      if( bCheckSplit )
      {
        m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
      }
      //----- code luma/chroma block with given intra prediction mode and store Cbf-----
      dSingleCost   = 0.0;

      pcCU ->setTransformSkipSubParts ( 0, COMPONENT_Y, uiAbsPartIdx, totalAdjustedDepthChan );
      xIntraCodingTUBlock( pcOrgYuv, pcPredYuv, pcResiYuv, resiLumaSingle, false, uiSingleDistLuma, COMPONENT_Y, rTu DEBUG_STRING_PASS_INTO(sDebug));

      if( bCheckSplit )
      {
        uiSingleCbfLuma = pcCU->getCbf( uiAbsPartIdx, COMPONENT_Y, uiTrDepth );
      }
      //----- determine rate and r-d cost -----
      UInt uiSingleBits = xGetIntraBitsQT( rTu, true, false, false );

      if(m_pcEncCfg->getRDpenalty() && (uiLog2TrSize==5) && !isIntraSlice)
      {
        uiSingleBits=uiSingleBits*4;
      }

#ifdef CVI_ENABLE_RDO_BIT_EST
      if ((getRDOBitEstMode()!=HM_ORIGINAL && getRDOSyntaxEstMode()!=HM_ORIGINAL) &&
          pcCU->getPartitionSize(0) == SIZE_NxN)
      {
        dSingleCost = m_pcRdCost->calcFracBitsRdCost(uiSingleBits, uiSingleDistLuma, I4_NUM_BIT_FRAC_BIT);
      }
      else
      {
        dSingleCost = m_pcRdCost->calcRdCost(uiSingleBits, uiSingleDistLuma );
      }
#else
      dSingleCost       = m_pcRdCost->calcRdCost(uiSingleBits, uiSingleDistLuma );
#endif

      if (pcCU->getSlice()->getPPS()->getPpsRangeExtension().getCrossComponentPredictionEnabledFlag())
      {
        const Int xOffset = rTu.getRect( COMPONENT_Y ).x0;
        const Int yOffset = rTu.getRect( COMPONENT_Y ).y0;
        for (UInt storedResidualIndex = 0; storedResidualIndex < NUMBER_OF_STORED_RESIDUAL_TYPES; storedResidualIndex++)
        {
          if (bMaintainResidual[storedResidualIndex])
          {
            xStoreCrossComponentPredictionResult(resiLuma[storedResidualIndex], resiLumaSingle[storedResidualIndex], rTu, xOffset, yOffset, MAX_CU_SIZE, MAX_CU_SIZE);
          }
        }
      }
    }
  }

  if( bCheckSplit )
  {
    //----- store full entropy coding status, load original entropy coding status -----
    if( bCheckFull )
    {
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_TEST ] );
      m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
    }
    else
    {
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
    }
    //----- code splitted block -----
    Double     dSplitCost      = 0.0;
    Distortion uiSplitDistLuma = 0;
    UInt       uiSplitCbfLuma  = 0;

    TComTURecurse tuRecurseChild(rTu, false);
    DEBUG_STRING_NEW(sSplit)
    do
    {
      DEBUG_STRING_NEW(sChild)
#if HHI_RQT_INTRA_SPEEDUP
      xRecurIntraCodingLumaQT( pcOrgYuv, pcPredYuv, pcResiYuv, resiLumaSplit, uiSplitDistLuma, bCheckFirst, dSplitCost, tuRecurseChild DEBUG_STRING_PASS_INTO(sChild) );
#else
      xRecurIntraCodingLumaQT( pcOrgYuv, pcPredYuv, pcResiYuv, resiLumaSplit, uiSplitDistLuma, dSplitCost, tuRecurseChild DEBUG_STRING_PASS_INTO(sChild) );
#endif
      DEBUG_STRING_APPEND(sSplit, sChild)
      uiSplitCbfLuma |= pcCU->getCbf( tuRecurseChild.GetAbsPartIdxTU(), COMPONENT_Y, tuRecurseChild.GetTransformDepthRel() );
    } while (tuRecurseChild.nextSection(rTu) );

    UInt    uiPartsDiv     = rTu.GetAbsPartIdxNumParts();
    {
      if (uiSplitCbfLuma)
      {
        const UInt flag=1<<uiTrDepth;
        UChar *pBase=pcCU->getCbf( COMPONENT_Y );
        for( UInt uiOffs = 0; uiOffs < uiPartsDiv; uiOffs++ )
        {
          pBase[ uiAbsPartIdx + uiOffs ] |= flag;
        }
      }
    }
    //----- restore context states -----
    m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );

    //----- determine rate and r-d cost -----
    UInt uiSplitBits = xGetIntraBitsQT( rTu, true, false, false );
    dSplitCost       = m_pcRdCost->calcRdCost( uiSplitBits, uiSplitDistLuma );

    //===== compare and set best =====
    if( dSplitCost < dSingleCost )
    {
      //--- update cost ---
      DEBUG_STRING_SWAP(sSplit, sDebug)
      ruiDistY += uiSplitDistLuma;
      dRDCost  += dSplitCost;

      if (pcCU->getSlice()->getPPS()->getPpsRangeExtension().getCrossComponentPredictionEnabledFlag())
      {
        const Int xOffset = rTu.getRect( COMPONENT_Y ).x0;
        const Int yOffset = rTu.getRect( COMPONENT_Y ).y0;
        for (UInt storedResidualIndex = 0; storedResidualIndex < NUMBER_OF_STORED_RESIDUAL_TYPES; storedResidualIndex++)
        {
          if (bMaintainResidual[storedResidualIndex])
          {
            xStoreCrossComponentPredictionResult(resiLuma[storedResidualIndex], resiLumaSplit[storedResidualIndex], rTu, xOffset, yOffset, MAX_CU_SIZE, MAX_CU_SIZE);
          }
        }
      }

      return;
    }

    //----- set entropy coding status -----
    m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_TEST ] );

    //--- set transform index and Cbf values ---
    pcCU->setTrIdxSubParts( uiTrDepth, uiAbsPartIdx, uiFullDepth );
    const TComRectangle &tuRect=rTu.getRect(COMPONENT_Y);
    pcCU->setCbfSubParts  ( uiSingleCbfLuma << uiTrDepth, COMPONENT_Y, uiAbsPartIdx, totalAdjustedDepthChan );
    pcCU ->setTransformSkipSubParts  ( bestModeId[COMPONENT_Y], COMPONENT_Y, uiAbsPartIdx, totalAdjustedDepthChan );

    //--- set reconstruction for next intra prediction blocks ---
    const UInt  uiQTLayer   = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    const UInt  uiZOrder    = pcCU->getZorderIdxInCtu() + uiAbsPartIdx;
    const UInt  uiWidth     = tuRect.width;
    const UInt  uiHeight    = tuRect.height;
    Pel*  piSrc       = m_pcQTTempTComYuv[ uiQTLayer ].getAddr( COMPONENT_Y, uiAbsPartIdx );
    UInt  uiSrcStride = m_pcQTTempTComYuv[ uiQTLayer ].getStride  ( COMPONENT_Y );
    Pel*  piDes       = pcCU->getPic()->getPicYuvRec()->getAddr( COMPONENT_Y, pcCU->getCtuRsAddr(), uiZOrder );
    UInt  uiDesStride = pcCU->getPic()->getPicYuvRec()->getStride  ( COMPONENT_Y );

    for( UInt uiY = 0; uiY < uiHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
    {
      for( UInt uiX = 0; uiX < uiWidth; uiX++ )
      {
        piDes[ uiX ] = piSrc[ uiX ];
      }
    }
  }
  ruiDistY += uiSingleDistLuma;
  dRDCost  += dSingleCost;
}


Void
TEncSearch::xSetIntraResultLumaQT(TComYuv* pcRecoYuv, TComTU &rTu)
{
  TComDataCU *pcCU        = rTu.getCU();
  const UInt uiTrDepth    = rTu.GetTransformDepthRel();
  const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if(  uiTrMode == uiTrDepth )
  {
    UInt uiLog2TrSize = rTu.GetLog2LumaTrSize();
    UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    //===== copy transform coefficients =====

    const TComRectangle &tuRect=rTu.getRect(COMPONENT_Y);
    const UInt coeffOffset = rTu.getCoefficientOffset(COMPONENT_Y);
    const UInt numCoeffInBlock = tuRect.width * tuRect.height;

    if (numCoeffInBlock!=0)
    {
      const TCoeff* srcCoeff = m_ppcQTTempCoeff[COMPONENT_Y][uiQTLayer] + coeffOffset;
      TCoeff* destCoeff      = pcCU->getCoeff(COMPONENT_Y) + coeffOffset;
      ::memcpy( destCoeff, srcCoeff, sizeof(TCoeff)*numCoeffInBlock );
#if ADAPTIVE_QP_SELECTION
      const TCoeff* srcArlCoeff = m_ppcQTTempArlCoeff[COMPONENT_Y][ uiQTLayer ] + coeffOffset;
      TCoeff* destArlCoeff      = pcCU->getArlCoeff (COMPONENT_Y)               + coeffOffset;
      ::memcpy( destArlCoeff, srcArlCoeff, sizeof( TCoeff ) * numCoeffInBlock );
#endif
      m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartComponent( COMPONENT_Y, pcRecoYuv, uiAbsPartIdx, tuRect.width, tuRect.height );
    }

  }
  else
  {
    TComTURecurse tuRecurseChild(rTu, false);
    do
    {
      xSetIntraResultLumaQT( pcRecoYuv, tuRecurseChild );
    } while (tuRecurseChild.nextSection(rTu));
  }
}


Void
TEncSearch::xStoreIntraResultQT(const ComponentID compID, TComTU &rTu )
{
  TComDataCU *pcCU=rTu.getCU();
  const UInt uiTrDepth = rTu.GetTransformDepthRel();
  const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();
  const UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if ( compID==COMPONENT_Y || uiTrMode == uiTrDepth )
  {
    assert(uiTrMode == uiTrDepth);
    const UInt uiLog2TrSize = rTu.GetLog2LumaTrSize();
    const UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    if (rTu.ProcessComponentSection(compID))
    {
      const TComRectangle &tuRect=rTu.getRect(compID);

      //===== copy transform coefficients =====
      const UInt uiNumCoeff    = tuRect.width * tuRect.height;
      TCoeff* pcCoeffSrc = m_ppcQTTempCoeff[compID] [ uiQTLayer ] + rTu.getCoefficientOffset(compID);
      TCoeff* pcCoeffDst = m_pcQTTempTUCoeff[compID];

      ::memcpy( pcCoeffDst, pcCoeffSrc, sizeof( TCoeff ) * uiNumCoeff );
#if ADAPTIVE_QP_SELECTION
      TCoeff* pcArlCoeffSrc = m_ppcQTTempArlCoeff[compID] [ uiQTLayer ] + rTu.getCoefficientOffset(compID);
      TCoeff* pcArlCoeffDst = m_ppcQTTempTUArlCoeff[compID];
      ::memcpy( pcArlCoeffDst, pcArlCoeffSrc, sizeof( TCoeff ) * uiNumCoeff );
#endif
      //===== copy reconstruction =====
      m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartComponent( compID, &m_pcQTTempTransformSkipTComYuv, uiAbsPartIdx, tuRect.width, tuRect.height );
    }
  }
}


Void
TEncSearch::xLoadIntraResultQT(const ComponentID compID, TComTU &rTu)
{
  TComDataCU *pcCU=rTu.getCU();
  const UInt uiTrDepth = rTu.GetTransformDepthRel();
  const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();
  const UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if ( compID==COMPONENT_Y || uiTrMode == uiTrDepth )
  {
    assert(uiTrMode == uiTrDepth);
    const UInt uiLog2TrSize = rTu.GetLog2LumaTrSize();
    const UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    const UInt uiZOrder     = pcCU->getZorderIdxInCtu() + uiAbsPartIdx;

    if (rTu.ProcessComponentSection(compID))
    {
      const TComRectangle &tuRect=rTu.getRect(compID);

      //===== copy transform coefficients =====
      const UInt uiNumCoeff = tuRect.width * tuRect.height;
      TCoeff* pcCoeffDst = m_ppcQTTempCoeff[compID] [ uiQTLayer ] + rTu.getCoefficientOffset(compID);
      TCoeff* pcCoeffSrc = m_pcQTTempTUCoeff[compID];

      ::memcpy( pcCoeffDst, pcCoeffSrc, sizeof( TCoeff ) * uiNumCoeff );
#if ADAPTIVE_QP_SELECTION
      TCoeff* pcArlCoeffDst = m_ppcQTTempArlCoeff[compID] [ uiQTLayer ] + rTu.getCoefficientOffset(compID);
      TCoeff* pcArlCoeffSrc = m_ppcQTTempTUArlCoeff[compID];
      ::memcpy( pcArlCoeffDst, pcArlCoeffSrc, sizeof( TCoeff ) * uiNumCoeff );
#endif
      //===== copy reconstruction =====
      m_pcQTTempTransformSkipTComYuv.copyPartToPartComponent( compID, &m_pcQTTempTComYuv[ uiQTLayer ], uiAbsPartIdx, tuRect.width, tuRect.height );

      Pel*    piRecIPred        = pcCU->getPic()->getPicYuvRec()->getAddr( compID, pcCU->getCtuRsAddr(), uiZOrder );
      UInt    uiRecIPredStride  = pcCU->getPic()->getPicYuvRec()->getStride (compID);
      Pel*    piRecQt           = m_pcQTTempTComYuv[ uiQTLayer ].getAddr( compID, uiAbsPartIdx );
      UInt    uiRecQtStride     = m_pcQTTempTComYuv[ uiQTLayer ].getStride  (compID);
      UInt    uiWidth           = tuRect.width;
      UInt    uiHeight          = tuRect.height;
      Pel* pRecQt               = piRecQt;
      Pel* pRecIPred            = piRecIPred;
      for( UInt uiY = 0; uiY < uiHeight; uiY++ )
      {
        for( UInt uiX = 0; uiX < uiWidth; uiX++ )
        {
          pRecIPred[ uiX ] = pRecQt   [ uiX ];
        }
        pRecQt    += uiRecQtStride;
        pRecIPred += uiRecIPredStride;
      }
    }
  }
}

Void
TEncSearch::xStoreCrossComponentPredictionResult(       Pel    *pResiDst,
                                                  const Pel    *pResiSrc,
                                                        TComTU &rTu,
                                                  const Int     xOffset,
                                                  const Int     yOffset,
                                                  const Int     strideDst,
                                                  const Int     strideSrc )
{
  const Pel *pSrc = pResiSrc + yOffset * strideSrc + xOffset;
        Pel *pDst = pResiDst + yOffset * strideDst + xOffset;

  for( Int y = 0; y < rTu.getRect( COMPONENT_Y ).height; y++ )
  {
    ::memcpy( pDst, pSrc, sizeof(Pel) * rTu.getRect( COMPONENT_Y ).width );
    pDst += strideDst;
    pSrc += strideSrc;
  }
}

SChar
TEncSearch::xCalcCrossComponentPredictionAlpha(       TComTU &rTu,
                                                const ComponentID compID,
                                                const Pel*        piResiL,
                                                const Pel*        piResiC,
                                                const Int         width,
                                                const Int         height,
                                                const Int         strideL,
                                                const Int         strideC )
{
  const Pel *pResiL = piResiL;
  const Pel *pResiC = piResiC;

        TComDataCU *pCU = rTu.getCU();
  const Int  absPartIdx = rTu.GetAbsPartIdxTU( compID );
  const Int diffBitDepth = pCU->getSlice()->getSPS()->getDifferentialLumaChromaBitDepth();

  SChar alpha = 0;
  Int SSxy  = 0;
  Int SSxx  = 0;

  for( UInt uiY = 0; uiY < height; uiY++ )
  {
    for( UInt uiX = 0; uiX < width; uiX++ )
    {
      const Pel scaledResiL = rightShift( pResiL[ uiX ], diffBitDepth );
      SSxy += ( scaledResiL * pResiC[ uiX ] );
      SSxx += ( scaledResiL * scaledResiL   );
    }

    pResiL += strideL;
    pResiC += strideC;
  }

  if( SSxx != 0 )
  {
    Double dAlpha = SSxy / Double( SSxx );
    alpha = SChar(Clip3<Int>(-16, 16, (Int)(dAlpha * 16)));

    static const SChar alphaQuant[17] = {0, 1, 1, 2, 2, 2, 4, 4, 4, 4, 4, 4, 8, 8, 8, 8, 8};

    alpha = (alpha < 0) ? -alphaQuant[Int(-alpha)] : alphaQuant[Int(alpha)];
  }
  pCU->setCrossComponentPredictionAlphaPartRange( alpha, compID, absPartIdx, rTu.GetAbsPartIdxNumParts( compID ) );

  return alpha;
}

Void
TEncSearch::xRecurIntraChromaCodingQT(TComYuv*    pcOrgYuv,
                                      TComYuv*    pcPredYuv,
                                      TComYuv*    pcResiYuv,
                                      Pel         resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE],
                                      Distortion& ruiDist,
                                      TComTU&     rTu
                                      DEBUG_STRING_FN_DECLARE(sDebug))
{
  TComDataCU         *pcCU                  = rTu.getCU();
  const UInt          uiTrDepth             = rTu.GetTransformDepthRel();
  const UInt          uiAbsPartIdx          = rTu.GetAbsPartIdxTU();
  const ChromaFormat  format                = rTu.GetChromaFormat();
  UInt                uiTrMode              = pcCU->getTransformIdx( uiAbsPartIdx );
  const UInt          numberValidComponents = getNumberValidComponents(format);

  if(  uiTrMode == uiTrDepth )
  {
    if (!rTu.ProcessChannelSection(CHANNEL_TYPE_CHROMA))
    {
      return;
    }

    const UInt uiFullDepth = rTu.GetTransformDepthTotal();

    Bool checkTransformSkip = pcCU->getSlice()->getPPS()->getUseTransformSkip();
    checkTransformSkip &= TUCompRectHasAssociatedTransformSkipFlag(rTu.getRect(COMPONENT_Cb), pcCU->getSlice()->getPPS()->getPpsRangeExtension().getLog2MaxTransformSkipBlockSize());

    if ( m_pcEncCfg->getUseTransformSkipFast() )
    {
      checkTransformSkip &= TUCompRectHasAssociatedTransformSkipFlag(rTu.getRect(COMPONENT_Y), pcCU->getSlice()->getPPS()->getPpsRangeExtension().getLog2MaxTransformSkipBlockSize());

      if (checkTransformSkip)
      {
        Int nbLumaSkip = 0;
        const UInt maxAbsPartIdxSub=uiAbsPartIdx + (rTu.ProcessingAllQuadrants(COMPONENT_Cb)?1:4);
        for(UInt absPartIdxSub = uiAbsPartIdx; absPartIdxSub < maxAbsPartIdxSub; absPartIdxSub ++)
        {
          nbLumaSkip += pcCU->getTransformSkip(absPartIdxSub, COMPONENT_Y);
        }
        checkTransformSkip &= (nbLumaSkip > 0);
      }
    }


    for (UInt ch=COMPONENT_Cb; ch<numberValidComponents; ch++)
    {
      const ComponentID compID = ComponentID(ch);
      DEBUG_STRING_NEW(sDebugBestMode)

      //use RDO to decide whether Cr/Cb takes TS
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[uiFullDepth][CI_QT_TRAFO_ROOT] );

      const Bool splitIntoSubTUs = rTu.getRect(compID).width != rTu.getRect(compID).height;

      TComTURecurse TUIterator(rTu, false, (splitIntoSubTUs ? TComTU::VERTICAL_SPLIT : TComTU::DONT_SPLIT), true, compID);

      const UInt partIdxesPerSubTU = TUIterator.GetAbsPartIdxNumParts(compID);

      do
      {
        const UInt subTUAbsPartIdx   = TUIterator.GetAbsPartIdxTU(compID);

        Double     dSingleCost               = MAX_DOUBLE;
        Int        bestModeId                = 0;
        Distortion singleDistC               = 0;
        UInt       singleCbfC                = 0;
        Distortion singleDistCTmp            = 0;
        Double     singleCostTmp             = 0;
        UInt       singleCbfCTmp             = 0;
        SChar      bestCrossCPredictionAlpha = 0;
        Int        bestTransformSkipMode     = 0;

        const Bool checkCrossComponentPrediction =    (pcCU->getIntraDir(CHANNEL_TYPE_CHROMA, subTUAbsPartIdx) == DM_CHROMA_IDX)
                                                   &&  pcCU->getSlice()->getPPS()->getPpsRangeExtension().getCrossComponentPredictionEnabledFlag()
                                                   && (pcCU->getCbf(subTUAbsPartIdx,  COMPONENT_Y, uiTrDepth) != 0);

        const Int  crossCPredictionModesToTest = checkCrossComponentPrediction ? 2 : 1;
        const Int  transformSkipModesToTest    = checkTransformSkip            ? 2 : 1;
        const Int  totalModesToTest            = crossCPredictionModesToTest * transformSkipModesToTest;
              Int  currModeId                  = 0;
              Int  default0Save1Load2          = 0;

        for(Int transformSkipModeId = 0; transformSkipModeId < transformSkipModesToTest; transformSkipModeId++)
        {
          for(Int crossCPredictionModeId = 0; crossCPredictionModeId < crossCPredictionModesToTest; crossCPredictionModeId++)
          {
            pcCU->setCrossComponentPredictionAlphaPartRange(0, compID, subTUAbsPartIdx, partIdxesPerSubTU);
            DEBUG_STRING_NEW(sDebugMode)
            pcCU->setTransformSkipPartRange( transformSkipModeId, compID, subTUAbsPartIdx, partIdxesPerSubTU );
            currModeId++;

            const Bool isOneMode  = (totalModesToTest == 1);
            const Bool isLastMode = (currModeId == totalModesToTest); // currModeId is indexed from 1

            if (isOneMode)
            {
              default0Save1Load2 = 0;
            }
            else if (!isOneMode && (transformSkipModeId == 0) && (crossCPredictionModeId == 0))
            {
              default0Save1Load2 = 1; //save prediction on first mode
            }
            else
            {
              default0Save1Load2 = 2; //load it on subsequent modes
            }

            singleDistCTmp = 0;

            xIntraCodingTUBlock( pcOrgYuv, pcPredYuv, pcResiYuv, resiLuma, (crossCPredictionModeId != 0), singleDistCTmp, compID, TUIterator DEBUG_STRING_PASS_INTO(sDebugMode), default0Save1Load2);
            singleCbfCTmp = pcCU->getCbf( subTUAbsPartIdx, compID, uiTrDepth);

            if (  ((crossCPredictionModeId == 1) && (pcCU->getCrossComponentPredictionAlpha(subTUAbsPartIdx, compID) == 0))
               || ((transformSkipModeId    == 1) && (singleCbfCTmp == 0))) //In order not to code TS flag when cbf is zero, the case for TS with cbf being zero is forbidden.
            {
              singleCostTmp = MAX_DOUBLE;
            }
            else if (!isOneMode)
            {
              UInt bitsTmp = xGetIntraBitsQTChroma( TUIterator, compID, false );
              singleCostTmp  = m_pcRdCost->calcRdCost( bitsTmp, singleDistCTmp);
            }

            if(singleCostTmp < dSingleCost)
            {
              DEBUG_STRING_SWAP(sDebugBestMode, sDebugMode)
              dSingleCost               = singleCostTmp;
              singleDistC               = singleDistCTmp;
              bestCrossCPredictionAlpha = (crossCPredictionModeId != 0) ? pcCU->getCrossComponentPredictionAlpha(subTUAbsPartIdx, compID) : 0;
              bestTransformSkipMode     = transformSkipModeId;
              bestModeId                = currModeId;
              singleCbfC                = singleCbfCTmp;

              if (!isOneMode && !isLastMode)
              {
                xStoreIntraResultQT(compID, TUIterator);
                m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiFullDepth ][ CI_TEMP_BEST ] );
              }
            }

            if (!isOneMode && !isLastMode)
            {
              m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiFullDepth ][ CI_QT_TRAFO_ROOT ] );
            }
          }
        }

        if(bestModeId < totalModesToTest)
        {
          xLoadIntraResultQT(compID, TUIterator);
          pcCU->setCbfPartRange( singleCbfC << uiTrDepth, compID, subTUAbsPartIdx, partIdxesPerSubTU );

          m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiFullDepth ][ CI_TEMP_BEST ] );
        }

        DEBUG_STRING_APPEND(sDebug, sDebugBestMode)
        pcCU ->setTransformSkipPartRange                ( bestTransformSkipMode,     compID, subTUAbsPartIdx, partIdxesPerSubTU );
        pcCU ->setCrossComponentPredictionAlphaPartRange( bestCrossCPredictionAlpha, compID, subTUAbsPartIdx, partIdxesPerSubTU );
        ruiDist += singleDistC;
      } while (TUIterator.nextSection(rTu));

      if (splitIntoSubTUs)
      {
        offsetSubTUCBFs(rTu, compID);
      }
    }
  }
  else
  {
    UInt    uiSplitCbf[MAX_NUM_COMPONENT] = {0,0,0};

    TComTURecurse tuRecurseChild(rTu, false);
    const UInt uiTrDepthChild   = tuRecurseChild.GetTransformDepthRel();
    do
    {
      DEBUG_STRING_NEW(sChild)

      xRecurIntraChromaCodingQT( pcOrgYuv, pcPredYuv, pcResiYuv, resiLuma, ruiDist, tuRecurseChild DEBUG_STRING_PASS_INTO(sChild) );

      DEBUG_STRING_APPEND(sDebug, sChild)
      const UInt uiAbsPartIdxSub=tuRecurseChild.GetAbsPartIdxTU();

      for(UInt ch=COMPONENT_Cb; ch<numberValidComponents; ch++)
      {
        uiSplitCbf[ch] |= pcCU->getCbf( uiAbsPartIdxSub, ComponentID(ch), uiTrDepthChild );
      }
    } while ( tuRecurseChild.nextSection(rTu) );


    UInt uiPartsDiv = rTu.GetAbsPartIdxNumParts();
    for(UInt ch=COMPONENT_Cb; ch<numberValidComponents; ch++)
    {
      if (uiSplitCbf[ch])
      {
        const UInt flag=1<<uiTrDepth;
        ComponentID compID=ComponentID(ch);
        UChar *pBase=pcCU->getCbf( compID );
        for( UInt uiOffs = 0; uiOffs < uiPartsDiv; uiOffs++ )
        {
          pBase[ uiAbsPartIdx + uiOffs ] |= flag;
        }
      }
    }
  }
}




Void
TEncSearch::xSetIntraResultChromaQT(TComYuv*    pcRecoYuv, TComTU &rTu)
{
  if (!rTu.ProcessChannelSection(CHANNEL_TYPE_CHROMA))
  {
    return;
  }
  TComDataCU *pcCU=rTu.getCU();
  const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();
  const UInt uiTrDepth   = rTu.GetTransformDepthRel();
  UInt uiTrMode     = pcCU->getTransformIdx( uiAbsPartIdx );
  if(  uiTrMode == uiTrDepth )
  {
    UInt uiLog2TrSize = rTu.GetLog2LumaTrSize();
    UInt uiQTLayer    = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    //===== copy transform coefficients =====
    const TComRectangle &tuRectCb=rTu.getRect(COMPONENT_Cb);
    UInt uiNumCoeffC    = tuRectCb.width*tuRectCb.height;//( pcCU->getSlice()->getSPS()->getMaxCUWidth() * pcCU->getSlice()->getSPS()->getMaxCUHeight() ) >> ( uiFullDepth << 1 );
    const UInt offset = rTu.getCoefficientOffset(COMPONENT_Cb);

    const UInt numberValidComponents = getNumberValidComponents(rTu.GetChromaFormat());
    for (UInt ch=COMPONENT_Cb; ch<numberValidComponents; ch++)
    {
      const ComponentID component = ComponentID(ch);
      const TCoeff* src           = m_ppcQTTempCoeff[component][uiQTLayer] + offset;//(uiNumCoeffIncC*uiAbsPartIdx);
      TCoeff* dest                = pcCU->getCoeff(component) + offset;//(uiNumCoeffIncC*uiAbsPartIdx);
      ::memcpy( dest, src, sizeof(TCoeff)*uiNumCoeffC );
#if ADAPTIVE_QP_SELECTION
      TCoeff* pcArlCoeffSrc = m_ppcQTTempArlCoeff[component][ uiQTLayer ] + offset;//( uiNumCoeffIncC * uiAbsPartIdx );
      TCoeff* pcArlCoeffDst = pcCU->getArlCoeff(component)                + offset;//( uiNumCoeffIncC * uiAbsPartIdx );
      ::memcpy( pcArlCoeffDst, pcArlCoeffSrc, sizeof( TCoeff ) * uiNumCoeffC );
#endif
    }

    //===== copy reconstruction =====

    m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartComponent( COMPONENT_Cb, pcRecoYuv, uiAbsPartIdx, tuRectCb.width, tuRectCb.height );
    m_pcQTTempTComYuv[ uiQTLayer ].copyPartToPartComponent( COMPONENT_Cr, pcRecoYuv, uiAbsPartIdx, tuRectCb.width, tuRectCb.height );
  }
  else
  {
    TComTURecurse tuRecurseChild(rTu, false);
    do
    {
      xSetIntraResultChromaQT( pcRecoYuv, tuRecurseChild );
    } while (tuRecurseChild.nextSection(rTu));
  }
}



Void
TEncSearch::estIntraPredLumaQT(TComDataCU* pcCU,
                               TComYuv*    pcOrgYuv,
                               TComYuv*    pcPredYuv,
                               TComYuv*    pcResiYuv,
                               TComYuv*    pcRecoYuv,
                               Pel         resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE]
                               DEBUG_STRING_FN_DECLARE(sDebug))
{
  const UInt         uiDepth               = pcCU->getDepth(0);
  const UInt         uiInitTrDepth         = pcCU->getPartitionSize(0) == SIZE_2Nx2N ? 0 : 1;
  const UInt         uiNumPU               = 1<<(2*uiInitTrDepth);
  const UInt         uiQNumParts           = pcCU->getTotalNumPart() >> 2;
  const UInt         uiWidthBit            = pcCU->getIntraSizeIdx(0);
  const ChromaFormat chFmt                 = pcCU->getPic()->getChromaFormat();
  const UInt         numberValidComponents = getNumberValidComponents(chFmt);
  const TComSPS     &sps                   = *(pcCU->getSlice()->getSPS());
  const TComPPS     &pps                   = *(pcCU->getSlice()->getPPS());
        Distortion   uiOverallDistY        = 0;
#ifdef CVI_INTRA_PRED
        double       overallCostY          = 0;
#endif
        UInt         CandNum;
        Double       CandCostList[ FAST_UDI_MAX_RDMODE_NUM ];
        Pel          resiLumaPU[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE];

        Bool    bMaintainResidual[NUMBER_OF_STORED_RESIDUAL_TYPES];
        for (UInt residualTypeIndex = 0; residualTypeIndex < NUMBER_OF_STORED_RESIDUAL_TYPES; residualTypeIndex++)
        {
          bMaintainResidual[residualTypeIndex] = true; //assume true unless specified otherwise
        }

        bMaintainResidual[RESIDUAL_ENCODER_SIDE] = !(m_pcEncCfg->getUseReconBasedCrossCPredictionEstimate());

  // Lambda calculation at equivalent Qp of 4 is recommended because at that Qp, the quantisation divisor is 1.
#if FULL_NBIT
  const Double sqrtLambdaForFirstPass= (m_pcEncCfg->getCostMode()==COST_MIXED_LOSSLESS_LOSSY_CODING && pcCU->getCUTransquantBypass(0)) ?
                sqrt(0.57 * pow(2.0, ((LOSSLESS_AND_MIXED_LOSSLESS_RD_COST_TEST_QP_PRIME - 12) / 3.0)))
              : m_pcRdCost->getSqrtLambda();
#else
  const Double sqrtLambdaForFirstPass= (m_pcEncCfg->getCostMode()==COST_MIXED_LOSSLESS_LOSSY_CODING && pcCU->getCUTransquantBypass(0)) ?
                sqrt(0.57 * pow(2.0, ((LOSSLESS_AND_MIXED_LOSSLESS_RD_COST_TEST_QP_PRIME - 12 - 6 * (sps.getBitDepth(CHANNEL_TYPE_LUMA) - 8)) / 3.0)))
              : m_pcRdCost->getSqrtLambda();
#endif

  //===== set QP and clear Cbf =====
  if ( pps.getUseDQP() == true)
  {
    pcCU->setQPSubParts( pcCU->getQP(0), 0, uiDepth );
  }
  else
  {
    pcCU->setQPSubParts( pcCU->getSlice()->getSliceQp(), 0, uiDepth );
  }

  //===== loop over partitions =====
  TComTURecurse tuRecurseCU(pcCU, 0);
  TComTURecurse tuRecurseWithPU(tuRecurseCU, false, (uiInitTrDepth==0)?TComTU::DONT_SPLIT : TComTU::QUAD_SPLIT);

  do
  {
    const UInt uiPartOffset=tuRecurseWithPU.GetAbsPartIdxTU();
//  for( UInt uiPU = 0, uiPartOffset=0; uiPU < uiNumPU; uiPU++, uiPartOffset += uiQNumParts )
  //{
    //===== init pattern for luma prediction =====
    DEBUG_STRING_NEW(sTemp2)

    //===== determine set of modes to be tested (using prediction signal only) =====
    Int numModesAvailable     = 35; //total number of Intra modes
    UInt uiRdModeList[FAST_UDI_MAX_RDMODE_NUM];
    Int numModesForFullRD = m_pcEncCfg->getFastUDIUseMPMEnabled()?g_aucIntraModeNumFast_UseMPM[ uiWidthBit ] : g_aucIntraModeNumFast_NotUseMPM[ uiWidthBit ];
#ifdef CVI_INTRA_PRED
    const TComRectangle &rect = tuRecurseWithPU.getRect(COMPONENT_Y);
    int edgeIndex = 0;
    int mpmModes[NUM_MOST_PROBABLE_MODES];
    memset(mpmModes, 0, NUM_MOST_PROBABLE_MODES * sizeof(int));
    int mpmNumber = 0;
    UInt puSize = tuRecurseWithPU.getRect(COMPONENT_Y).width;
    bool isCviIntraPred = g_algo_cfg.EnableCviIntraPred;
    bool isEnableIntraRmdSAD = g_algo_cfg.EnableIntraRmdSAD;
    if (isCviIntraPred && puSize == 16)
      numModesForFullRD = 5;
#endif //~CVI_INTRA_PRED

#ifdef SIGDUMP
    g_sigpool.luma_pu_size = puSize;
#endif
#ifdef SIG_IAPU
    if(g_sigdump.iapu){
        sig_iapu_output_ctu_idx(puSize);
        sig_iapu_output_cu_idx(puSize);
        g_sigpool.iapu_st.lambda = (UInt)(m_pcRdCost->getFixpointFracLambda());
        const QpParam cQP(*pcCU, COMPONENT_Y);
        const QpParam QP_cb(*pcCU, COMPONENT_Cb);
        const QpParam QP_cr(*pcCU, COMPONENT_Cr);
        g_sigpool.iapu_st.qp[COMPONENT_Y]   = cQP.Qp;
        g_sigpool.iapu_st.qp[COMPONENT_Cb]  = QP_cb.Qp;
        g_sigpool.iapu_st.qp[COMPONENT_Cr]  = QP_cr.Qp;
    }
#endif //~SIG_IAPU

#ifdef SIG_RRU
    if (g_sigdump.rru)
      sig_rru_output_intra(puSize, rect, pcCU);
#endif //~SIG_RRU

    // this should always be true
    assert (tuRecurseWithPU.ProcessComponentSection(COMPONENT_Y));

#ifdef CVI_INTRA_PRED
    if (isCviIntraPred && g_algo_cfg.EnableIntraRmdFakeNb)
      initIntraPatternChType( tuRecurseWithPU, COMPONENT_Y, true DEBUG_STRING_PASS_INTO(sTemp2), true);
    else
      initIntraPatternChType( tuRecurseWithPU, COMPONENT_Y, true DEBUG_STRING_PASS_INTO(sTemp2) );
#else
    initIntraPatternChType( tuRecurseWithPU, COMPONENT_Y, true DEBUG_STRING_PASS_INTO(sTemp2) );
#endif //~CVI_INTRA_PRED

    Bool doFastSearch = (numModesForFullRD != numModesAvailable);
    if (doFastSearch)
    {
      #ifdef SIG_IAPU
      if(g_sigdump.iapu){
        g_sigpool.flag_iapu_luma_stage = 0;
      }
      #endif //~SIG_IAPU

      assert(numModesForFullRD < numModesAvailable);

      for( Int i=0; i < numModesForFullRD; i++ )
      {
        CandCostList[ i ] = MAX_DOUBLE;
      }
      CandNum = 0;

      const TComRectangle &puRect=tuRecurseWithPU.getRect(COMPONENT_Y);
      const UInt uiAbsPartIdx=tuRecurseWithPU.GetAbsPartIdxTU();

      Pel* piOrg         = pcOrgYuv ->getAddr( COMPONENT_Y, uiAbsPartIdx );
      Pel* piPred        = pcPredYuv->getAddr( COMPONENT_Y, uiAbsPartIdx );
      UInt uiStride      = pcPredYuv->getStride( COMPONENT_Y );
      DistParam distParam;

      #ifdef SIG_IAPU
      if(g_sigdump.iapu){
          const TComRectangle &rect        = tuRecurseWithPU.getRect(isLuma(COMPONENT_Y) ? COMPONENT_Y : COMPONENT_Cb);
          const Int            iWidth      = rect.width;
          const Int            iHeight     = rect.height;
          int iapu_file_idx = (iWidth==4)? 0: (iWidth==8)? 1: 2;

          for (Int y=0;y<iHeight;y++){
              for (Int x=0;x<iWidth;x++){
                  sigdump_output_bin(&g_sigpool.iapu_rmd_tu_src_frm_ctx[iapu_file_idx], reinterpret_cast<unsigned char*>(&piOrg[y*uiStride+x]), 1);
                  if(iWidth==16 || iWidth==8)
                      sigdump_output_bin(&g_sigpool.iapu_iap_tu_src_frm_ctx[iapu_file_idx], reinterpret_cast<unsigned char*>(&piOrg[y*uiStride+x]), 1);
              }
          }
      }
      #endif //~SIG_IAPU

#ifdef CVI_INTRA_PRED
      Bool bUseHadamard = (pcCU->getCUTransquantBypass(0) == 0) && isEnableLC_RdCostSATD(false);

      if (isCviIntraPred && isEnableIntraRmdSAD)
      {
        if (puSize == 8 || puSize == 16)
          bUseHadamard = false;
      }
#else
      const Bool bUseHadamard=pcCU->getCUTransquantBypass(0) == 0;
#endif
      m_pcRdCost->setDistParam(distParam, sps.getBitDepth(CHANNEL_TYPE_LUMA), piOrg, uiStride, piPred, uiStride, puRect.width, puRect.height, bUseHadamard);
      distParam.bApplyWeight = false;

#ifdef CVI_INTRA_PRED
      if (isCviIntraPred)
      {
        const UInt cuPelX   = pcCU->getCUPelX();
        const UInt cuPelY   = pcCU->getCUPelY();
        int tu_x = cuPelX + rect.x0;
        int tu_y = cuPelY + rect.y0;

        // Get edge
        edgeIndex = cviEdgeDetect(piOrg, uiStride, tu_x, tu_y, rect.width, rect.height);
        #ifdef SIG_IAPU
        int iapu_file_idx = (puSize==4)? 0: (puSize==8)? 1: 2;
        if(g_sigdump.iapu){
            sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cmd_frm_ctx[iapu_file_idx],   "# EDGE=%d\n", edgeIndex);
        }
        #endif //~SIG_IAPU

        // Get MPM mode
        pcCU->getIntraDirPredictor(uiPartOffset, mpmModes, COMPONENT_Y, &mpmNumber);
      }
#endif //~CVI_INTRA_PRED

      for( Int modeIdx = 0; modeIdx < numModesAvailable; modeIdx++ )
      {
        UInt       uiMode = modeIdx;
        Distortion uiSad  = 0;

        const Bool bUseFilter=TComPrediction::filteringIntraReferenceSamples(COMPONENT_Y, uiMode, puRect.width, puRect.height, chFmt, sps.getSpsRangeExtension().getIntraSmoothingDisabledFlag());

#ifdef CVI_INTRA_PRED
        if (isCviIntraPred)
        {
          bool isSkip = cviSkipRmdMode(puSize, modeIdx, edgeIndex);
          if (isSkip)
            continue;
        }
#endif //~CVI_INTRA_PRED
        predIntraAng( COMPONENT_Y, uiMode, piOrg, uiStride, piPred, uiStride, tuRecurseWithPU, bUseFilter, TComPrediction::UseDPCMForFirstPassIntraEstimation(tuRecurseWithPU, uiMode) );

        #ifdef SIG_IAPU
        if(g_sigdump.iapu){
            const TComRectangle &rect        = tuRecurseWithPU.getRect(isLuma(COMPONENT_Y) ? COMPONENT_Y : COMPONENT_Cb);
            const Int            iWidth      = rect.width;
            const Int            iHeight     = rect.height;
            int iapu_file_idx = (iWidth==4)? 0: (iWidth==8)? 1: 2;

            sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx],       "# MODE=%d\n", uiMode);

            for (Int y=0;y<iHeight;y++){
                for (Int x=0;x<iWidth;x++){
                    sigdump_output_bin(&g_sigpool.iapu_rmd_tu_pred_blk_frm_ctx[iapu_file_idx], reinterpret_cast<unsigned char*>(&piPred[y*uiStride+x]), 1);
                }
            }
        }
        #endif //~SIG_IAPU

        // use hadamard transform here
        uiSad+=distParam.DistFunc(&distParam);

#ifdef CVI_INTRA_PRED
        Double cost = 0.0;
        if (isCviIntraPred)
        {
          cost = (Double)uiSad;
        }
        else
        {
          UInt iModeBits = 0;
          iModeBits+=xModeBitsIntra( pcCU, uiMode, uiPartOffset, uiDepth, CHANNEL_TYPE_LUMA );
          cost      = (Double)uiSad + (Double)iModeBits * sqrtLambdaForFirstPass;
        }
#else
        UInt   iModeBits = 0;
        // NB xModeBitsIntra will not affect the mode for chroma that may have already been pre-estimated.
        iModeBits+=xModeBitsIntra( pcCU, uiMode, uiPartOffset, uiDepth, CHANNEL_TYPE_LUMA );
        Double cost      = (Double)uiSad + (Double)iModeBits * sqrtLambdaForFirstPass;
#endif //~CVI_INTRA_PRED

#if DEBUG_INTRA_SEARCH_COSTS
        std::cout << "1st pass mode " << uiMode << " SAD = " << uiSad << ", mode bits = " << iModeBits << ", cost = " << cost << "\n";
#endif
        #ifdef SIG_IAPU
        if(g_sigdump.iapu){
            int iapu_file_idx = (puSize==4)? 0: (puSize==8)? 1: 2;
            sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx],   "# SAD=%d\n", uiSad);
        }
        #endif //~SIG_IAPU

        CandNum += xUpdateCandList( uiMode, cost, numModesForFullRD, uiRdModeList, CandCostList );
      }

      #ifdef SIG_IAPU
      if(g_sigdump.iapu){
          int iapu_file_idx = (puSize==4)? 0: (puSize==8)? 1: 2;
          // RMD :: CANDIDATES
          sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx],     "# CANDIDATES\n");
          for(int mode_idx=0; mode_idx<CandNum; mode_idx++)
              sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx], "%d ", uiRdModeList[mode_idx]);
          sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx],     "\n");
          // IAP :: RMD_CAND
          sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx],     "# RMD_CAND\n");
          for(int mode_idx=0; mode_idx<CandNum; mode_idx++)
              sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx], "%d ", uiRdModeList[mode_idx]);
          sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx],     "\n");
          // IAP :: MPM
          sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx],     "# MPM\n");
          for(int mode_idx=0; mode_idx<NUM_MOST_PROBABLE_MODES; mode_idx++)
              sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx], "%d ", mpmModes[mode_idx]);
          sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx],     "\n");

      }
      #endif //~SIG_IAPU


      #ifdef CVI_INTRA_PRED
      ////////////////////////////////////////////
      // IAP stage, Hadamard transform to i8 and i16
      #ifdef SIG_IAPU
      if(g_sigdump.iapu){
        g_sigpool.flag_iapu_luma_stage = 1;
      }
      #endif //~SIG_IAPU

      if (isCviIntraPred && (puSize == 16 || puSize == 8))
      {
        int iapModeNumber = 0;
        UInt iapModes[numModesAvailable];
        memset(iapModes, 0, numModesAvailable * sizeof(UInt));

        // Restore true neighbor
        if (isCviIntraPred && g_algo_cfg.EnableIntraRmdFakeNb){
          initIntraPatternChType( tuRecurseWithPU, COMPONENT_Y, true DEBUG_STRING_PASS_INTO(sTemp2));
          #ifdef SIG_IAPU
          if (g_sigdump.iapu ){
            sig_iapu_output_neb(g_sigpool.iapu_iap_tu_neb_frm_ctx, tuRecurseWithPU.getRect(COMPONENT_Y).width, COMPONENT_Y);
            int ctx_idx = sig_pat_blk_size(puSize);
            memcpy(g_sigpool.iapu_st.neb_avail_b_iap[ctx_idx], g_sigpool.iapu_st.neb_avail_b, sizeof(Bool)*3);
            memcpy(g_sigpool.iapu_st.neb_avail_a_iap[ctx_idx], g_sigpool.iapu_st.neb_avail_a, sizeof(Bool)*2);
          }
          #endif //~SIG_IAPU
        }

        // Restore cost function to hadamard
        if (isCviIntraPred && isEnableIntraRmdSAD)
        {
          bUseHadamard = true;
          m_pcRdCost->setDistParam(distParam, sps.getBitDepth(CHANNEL_TYPE_LUMA), piOrg, uiStride, piPred, uiStride, puRect.width, puRect.height, bUseHadamard);
        }

        for( Int i = 0; i < numModesForFullRD; i++ )
        {
          iapModes[i] = uiRdModeList[i];
          CandCostList[i] = MAX_DOUBLE;
        }

        cviFillIntraMode(puSize, iapModes, iapModeNumber, edgeIndex, mpmNumber, mpmModes);
        #ifdef SIG_IAPU
        if(g_sigdump.iapu){
            int iapu_file_idx = (puSize==8)? 1: 2;
            // IAP :: LUMA_CAND for tu8, tu16
            sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx],     "# LUMA_CAND\n");
            for(int mode_idx=0; mode_idx<iapModeNumber; mode_idx++)
                sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx], "%d ", iapModes[mode_idx]);
            sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx],     "\n");
        }
        #endif //~SIG_IAPU

        // Get chroma i8 candidates from luma rmd
        if (puSize == 16 || puSize == 8)
        {
          cviSetChromaCandidate(iapModes, iapModeNumber);
        }

        for( Int modeIdx = 0; modeIdx < numModesAvailable; modeIdx++ )
        {
          UInt       uiMode = modeIdx;
          Distortion uiSad  = 0;
          const Bool bUseFilter=TComPrediction::filteringIntraReferenceSamples(COMPONENT_Y, uiMode, puRect.width, puRect.height, chFmt, sps.getSpsRangeExtension().getIntraSmoothingDisabledFlag());

          bool isSkip = cviSkipIapMode(modeIdx, iapModes, iapModeNumber);
          if (isSkip)
            continue;

          predIntraAng( COMPONENT_Y, uiMode, piOrg, uiStride, piPred, uiStride, tuRecurseWithPU, bUseFilter, TComPrediction::UseDPCMForFirstPassIntraEstimation(tuRecurseWithPU, uiMode) );

          // use hadamard transform here
          uiSad+=distParam.DistFunc(&distParam);
          UInt   iModeBits = 0;

          // NB xModeBitsIntra will not affect the mode for chroma that may have already been pre-estimated.
          iModeBits += xModeBitsIntra( pcCU, uiMode, uiPartOffset, uiDepth, CHANNEL_TYPE_LUMA );
          Double cost = (Double)uiSad + m_pcRdCost->calcRdCost(iModeBits, 0, DF_SAD);
          CandNum += xUpdateCandList( uiMode, cost, numModesForFullRD, uiRdModeList, CandCostList );
          #ifdef SIG_IAPU
          if(g_sigdump.iapu){
              int iapu_file_idx = (puSize==8)? 1: 2;
              sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx],     "L_MODE = %d\n", uiMode);
              sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx],     "SAD = %lu\n", uiSad);
              sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx],     "MODEBITS = %d\n", iModeBits);
              sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx],     "COST = %ld\n", m_pcRdCost->getFixpointRdCost(cost));
              sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx],     "LAMBDA = %ld\n", m_pcRdCost->getFixpointSADSqrtLambda());
          }
          #endif //~SIG_IAPU
        }
      }
      ////////////////////////////////////////////
#endif //~CVI_INTRA_PRED

#ifdef CVI_INTRA_PRED
      if (m_pcEncCfg->getFastUDIUseMPMEnabled() && isCviIntraPred == false)
#else
      if (m_pcEncCfg->getFastUDIUseMPMEnabled())
#endif
      {
        Int uiPreds[NUM_MOST_PROBABLE_MODES] = {-1, -1, -1};

        Int iMode = -1;
        pcCU->getIntraDirPredictor( uiPartOffset, uiPreds, COMPONENT_Y, &iMode );

        const Int numCand = ( iMode >= 0 ) ? iMode : Int(NUM_MOST_PROBABLE_MODES);

        for( Int j=0; j < numCand; j++)
        {
          Bool mostProbableModeIncluded = false;
          Int mostProbableMode = uiPreds[j];

          for( Int i=0; i < numModesForFullRD; i++)
          {
            mostProbableModeIncluded |= (mostProbableMode == uiRdModeList[i]);
          }
          if (!mostProbableModeIncluded)
          {
            uiRdModeList[numModesForFullRD++] = mostProbableMode;
          }
        }
      }
    }
    else
    {
      for( Int i=0; i < numModesForFullRD; i++)
      {
        uiRdModeList[i] = i;
      }
    }

    //===== check modes (using r-d costs) =====
#if HHI_RQT_INTRA_SPEEDUP_MOD
    UInt   uiSecondBestMode  = MAX_UINT;
    Double dSecondBestPUCost = MAX_DOUBLE;
#endif
    DEBUG_STRING_NEW(sPU)
    UInt       uiBestPUMode  = 0;
    Distortion uiBestPUDistY = 0;
    Double     dBestPUCost   = MAX_DOUBLE;

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
    UInt max=numModesForFullRD;

    if (DebugOptionList::ForceLumaMode.isSet())
    {
      max=0;  // we are forcing a direction, so don't bother with mode check
    }
    for ( UInt uiMode = 0; uiMode < max; uiMode++)
#else

#ifdef CVI_INTRA_PRED
    if (isCviIntraPred)
    {
      if (puSize == 4)
      {
        int modeListNumber = 0;
        cviFillIntraMode(puSize, uiRdModeList, modeListNumber, edgeIndex, mpmNumber, mpmModes);
        numModesForFullRD = modeListNumber;
        #ifdef SIG_IAPU
        if(g_sigdump.iapu){
            int iapu_file_idx = 0;
            // IAP :: LUMA_CAND for tu4
            sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx],     "# LUMA_CAND\n");
            for(int mode_idx=0; mode_idx<modeListNumber; mode_idx++)
                sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx], "%d ", uiRdModeList[mode_idx]);
            sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx],     "\n");
        }
        #endif //~SIG_IAPU
      }
      else if (puSize == 16 || puSize == 8)
      {
        numModesForFullRD = 1;
      }
    }
#endif //~CVI_INTRA_PRED

    for( UInt uiMode = 0; uiMode < numModesForFullRD; uiMode++ )
#endif
    {
#ifdef SIG_IAPU_DEBUG
      g_sigpool.iapu_st.rdo_mode_idx = uiMode;
#endif

      // set luma prediction mode
      UInt uiOrgMode = uiRdModeList[uiMode];

      pcCU->setIntraDirSubParts ( CHANNEL_TYPE_LUMA, uiOrgMode, uiPartOffset, uiDepth + uiInitTrDepth );

      DEBUG_STRING_NEW(sMode)
      // set context models
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );

      // determine residual for partition
      Distortion uiPUDistY = 0;
      Double     dPUCost   = 0.0;
#if HHI_RQT_INTRA_SPEEDUP
      xRecurIntraCodingLumaQT( pcOrgYuv, pcPredYuv, pcResiYuv, resiLumaPU, uiPUDistY, true, dPUCost, tuRecurseWithPU DEBUG_STRING_PASS_INTO(sMode) );
#else
      xRecurIntraCodingLumaQT( pcOrgYuv, pcPredYuv, pcResiYuv, resiLumaPU, uiPUDistY, dPUCost, tuRecurseWithPU DEBUG_STRING_PASS_INTO(sMode) );
#endif

#if DEBUG_INTRA_SEARCH_COSTS
      std::cout << "2nd pass [luma,chroma] mode [" << Int(pcCU->getIntraDir(CHANNEL_TYPE_LUMA, uiPartOffset)) << "," << Int(pcCU->getIntraDir(CHANNEL_TYPE_CHROMA, uiPartOffset)) << "] cost = " << dPUCost << "\n";
#endif
#ifdef SIG_IAPU
      if(g_sigdump.iapu && (puSize==4)){
        int iapu_file_idx = 0;
        Int64 fix_point_cost = m_pcRdCost->getFixpointRdCost(dPUCost);
        sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "L_MODE = %d\n", Int(pcCU->getIntraDir(CHANNEL_TYPE_LUMA, uiPartOffset)) );
        sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "RDO_COST = %lld,%llu,%llu,%u,%llu\n",
                              fix_point_cost, uiPUDistY, g_sigpool.iapu_st.bin_count,
                              m_pcEntropyCoder->getNumberOfWrittenBits(), m_pcEntropyCoder->getNumberOfWrittenBitsFrac() );

        Pel* piPred   = pcPredYuv->getAddr( COMPONENT_Y, uiPartOffset );
        UInt uiStride = pcPredYuv->getStride( COMPONENT_Y );

        for (Int y=0;y<4;y++){
          for (Int x=0;x<4;x++){
            g_sigpool.iapu_st.tu4_pred_blk[COMPONENT_Y][uiMode][y*4+x] = static_cast<UChar>(piPred[y*uiStride+x]&0xff);
          }
        }
      }
#endif //~SIG_IAPU

      // check r-d cost
      if( dPUCost < dBestPUCost )
      {
        DEBUG_STRING_SWAP(sPU, sMode)
#if HHI_RQT_INTRA_SPEEDUP_MOD
        uiSecondBestMode  = uiBestPUMode;
        dSecondBestPUCost = dBestPUCost;
#endif
        uiBestPUMode  = uiOrgMode;
        uiBestPUDistY = uiPUDistY;
        dBestPUCost   = dPUCost;

        xSetIntraResultLumaQT( pcRecoYuv, tuRecurseWithPU );

        if (pps.getPpsRangeExtension().getCrossComponentPredictionEnabledFlag())
        {
          const Int xOffset = tuRecurseWithPU.getRect( COMPONENT_Y ).x0;
          const Int yOffset = tuRecurseWithPU.getRect( COMPONENT_Y ).y0;
          for (UInt storedResidualIndex = 0; storedResidualIndex < NUMBER_OF_STORED_RESIDUAL_TYPES; storedResidualIndex++)
          {
            if (bMaintainResidual[storedResidualIndex])
            {
              xStoreCrossComponentPredictionResult(resiLuma[storedResidualIndex], resiLumaPU[storedResidualIndex], tuRecurseWithPU, xOffset, yOffset, MAX_CU_SIZE, MAX_CU_SIZE );
            }
          }
        }

#ifdef SIG_BIT_EST
        if (g_sigdump.rdo_sse) {
          memcpy(&g_sigpool.est_golden_intra[1][0], &g_sigpool.est_golden_intra[0][0], sizeof(sig_est_golden_intra_st));
        }
#endif
        UInt uiQPartNum = tuRecurseWithPU.GetAbsPartIdxNumParts();

        ::memcpy( m_puhQTTempTrIdx,  pcCU->getTransformIdx()       + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        for (UInt component = 0; component < numberValidComponents; component++)
        {
          const ComponentID compID = ComponentID(component);
          ::memcpy( m_puhQTTempCbf[compID], pcCU->getCbf( compID  ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
          ::memcpy( m_puhQTTempTransformSkipFlag[compID],  pcCU->getTransformSkip(compID)  + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        }
#ifdef SIG_RRU
        if (g_sigdump.rru)
        {
          // update final buffer
          sig_rru_update_final_buf(false, true, puSize);
          if (puSize > 4)
          {
            g_sigpool.rru_gold.rru_est_gold[1][0][0] = g_sigpool.rru_temp_gold.rru_est_gold[1][0][0];
          }
          else
          {
            int tu_id = tuRecurseWithPU.GetSectionNumber();
            g_sigpool.rru_gold.rru_est_gold_4x4[0][tu_id] = g_sigpool.rru_temp_gold.rru_est_gold[1][0][tu_id];
          }
        }
#endif //~SIG_RRU
        #ifdef SIG_IAPU
        if (g_sigdump.iapu)
        {
          // update final buffer
          sig_iapu_update_final_buf(false, true, puSize);
        }
        #endif //~SIG_IAPU
      }
#if HHI_RQT_INTRA_SPEEDUP_MOD
      else if( dPUCost < dSecondBestPUCost )
      {
        uiSecondBestMode  = uiOrgMode;
        dSecondBestPUCost = dPUCost;
      }
#endif
    } // Mode loop
#ifdef SIG_BIT_EST
    if (g_sigdump.rdo_sse)
      sigdump_output_rdo_sse_intra(g_aucConvertToBit[puSize], uiPartOffset, 0);
#endif
#ifdef SIG_RRU
    if (g_sigdump.rru)
    {
      sig_rru_output_intp(true, puSize, false);
      sig_rru_output_dct(true, puSize, false);

      if (puSize == 4)
      {
        sig_rru_output_i4_resi(false);
      }
    }
#endif //~SIG_RRU
#ifdef SIG_IAPU
    if(g_sigdump.iapu){
        int iapu_file_idx = (puSize==4)? 0: (puSize==8)? 1: 2;
        sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "L_BEST_MODE = %d\n", uiBestPUMode);
    }
    if(g_sigdump.iapu && (puSize==4)){
        int iapu_file_idx = 0;
        sig_iapu_output_neb(g_sigpool.iapu_iap_tu_neb_frm_ctx, puSize, COMPONENT_Y);
        //
        sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx], "# L_QP = %d\n",   g_sigpool.iapu_st.qp[COMPONENT_Y]);
        sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx], "# CB_QP = %d\n",  g_sigpool.iapu_st.qp[COMPONENT_Cb]);
        sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cmd_frm_ctx[iapu_file_idx], "# CR_QP = %d\n",  g_sigpool.iapu_st.qp[COMPONENT_Cr]);
        //
        sig_iapu_output_i4_resi(false);
    }
#endif //~SIG_IAPU

#if HHI_RQT_INTRA_SPEEDUP
#if HHI_RQT_INTRA_SPEEDUP_MOD
    for( UInt ui =0; ui < 2; ++ui )
#endif
    {
#if HHI_RQT_INTRA_SPEEDUP_MOD
      UInt uiOrgMode   = ui ? uiSecondBestMode  : uiBestPUMode;
      if( uiOrgMode == MAX_UINT )
      {
        break;
      }
#else
      UInt uiOrgMode = uiBestPUMode;
#endif

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
      if (DebugOptionList::ForceLumaMode.isSet())
      {
        uiOrgMode = DebugOptionList::ForceLumaMode.getInt();
      }
#endif

      pcCU->setIntraDirSubParts ( CHANNEL_TYPE_LUMA, uiOrgMode, uiPartOffset, uiDepth + uiInitTrDepth );
      DEBUG_STRING_NEW(sModeTree)

      // set context models
      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST] );

      // determine residual for partition
      Distortion uiPUDistY = 0;
      Double     dPUCost   = 0.0;

      xRecurIntraCodingLumaQT( pcOrgYuv, pcPredYuv, pcResiYuv, resiLumaPU, uiPUDistY, false, dPUCost, tuRecurseWithPU DEBUG_STRING_PASS_INTO(sModeTree));

      // check r-d cost
      if( dPUCost < dBestPUCost )
      {
        DEBUG_STRING_SWAP(sPU, sModeTree)
        uiBestPUMode  = uiOrgMode;
        uiBestPUDistY = uiPUDistY;
        dBestPUCost   = dPUCost;

        xSetIntraResultLumaQT( pcRecoYuv, tuRecurseWithPU );

        if (pps.getPpsRangeExtension().getCrossComponentPredictionEnabledFlag())
        {
          const Int xOffset = tuRecurseWithPU.getRect( COMPONENT_Y ).x0;
          const Int yOffset = tuRecurseWithPU.getRect( COMPONENT_Y ).y0;
          for (UInt storedResidualIndex = 0; storedResidualIndex < NUMBER_OF_STORED_RESIDUAL_TYPES; storedResidualIndex++)
          {
            if (bMaintainResidual[storedResidualIndex])
            {
              xStoreCrossComponentPredictionResult(resiLuma[storedResidualIndex], resiLumaPU[storedResidualIndex], tuRecurseWithPU, xOffset, yOffset, MAX_CU_SIZE, MAX_CU_SIZE );
            }
          }
        }

        const UInt uiQPartNum = tuRecurseWithPU.GetAbsPartIdxNumParts();
        ::memcpy( m_puhQTTempTrIdx,  pcCU->getTransformIdx()       + uiPartOffset, uiQPartNum * sizeof( UChar ) );

        for (UInt component = 0; component < numberValidComponents; component++)
        {
          const ComponentID compID = ComponentID(component);
          ::memcpy( m_puhQTTempCbf[compID], pcCU->getCbf( compID  ) + uiPartOffset, uiQPartNum * sizeof( UChar ) );
          ::memcpy( m_puhQTTempTransformSkipFlag[compID],  pcCU->getTransformSkip(compID)  + uiPartOffset, uiQPartNum * sizeof( UChar ) );
        }
      }
    } // Mode loop
#endif

    DEBUG_STRING_APPEND(sDebug, sPU)

    //--- update overall distortion ---
    uiOverallDistY += uiBestPUDistY;
#ifdef CVI_INTRA_PRED
    overallCostY += dBestPUCost;
#endif
#ifdef SIG_RRU
    if (puSize == 4 && g_sigdump.rru) {
      g_sigpool.rru_gold.sse4x4[tuRecurseWithPU.GetSectionNumber()] = uiBestPUDistY;
    }
#endif
#ifdef SIG_IAPU
    if (puSize == 4 && g_sigdump.iapu) {
        sig_iapu_save_blk4_self_info(tuRecurseWithPU.GetSectionNumber(), -1);
    }
#endif
    //--- update transform index and cbf ---
    const UInt uiQPartNum = tuRecurseWithPU.GetAbsPartIdxNumParts();
    ::memcpy( pcCU->getTransformIdx()       + uiPartOffset, m_puhQTTempTrIdx,  uiQPartNum * sizeof( UChar ) );
    for (UInt component = 0; component < numberValidComponents; component++)
    {
      const ComponentID compID = ComponentID(component);
      ::memcpy( pcCU->getCbf( compID  ) + uiPartOffset, m_puhQTTempCbf[compID], uiQPartNum * sizeof( UChar ) );
      ::memcpy( pcCU->getTransformSkip( compID  ) + uiPartOffset, m_puhQTTempTransformSkipFlag[compID ], uiQPartNum * sizeof( UChar ) );
    }

    //--- set reconstruction for next intra prediction blocks ---
    if( !tuRecurseWithPU.IsLastSection() )
    {
      const TComRectangle &puRect=tuRecurseWithPU.getRect(COMPONENT_Y);
      const UInt  uiCompWidth   = puRect.width;
      const UInt  uiCompHeight  = puRect.height;

      const UInt  uiZOrder      = pcCU->getZorderIdxInCtu() + uiPartOffset;
            Pel*  piDes         = pcCU->getPic()->getPicYuvRec()->getAddr( COMPONENT_Y, pcCU->getCtuRsAddr(), uiZOrder );
      const UInt  uiDesStride   = pcCU->getPic()->getPicYuvRec()->getStride( COMPONENT_Y);
      const Pel*  piSrc         = pcRecoYuv->getAddr( COMPONENT_Y, uiPartOffset );
      const UInt  uiSrcStride   = pcRecoYuv->getStride( COMPONENT_Y);

      for( UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
      {
        for( UInt uiX = 0; uiX < uiCompWidth; uiX++ )
        {
          piDes[ uiX ] = piSrc[ uiX ];
        }
      }
    }
#ifdef SIGDUMP
    if (tuRecurseWithPU.getRect(COMPONENT_Y).width == 4) {
      g_sigpool.cu_count[0]++;
    }
#endif
    //=== update PU data ====
    pcCU->setIntraDirSubParts     ( CHANNEL_TYPE_LUMA, uiBestPUMode, uiPartOffset, uiDepth + uiInitTrDepth );
  } while (tuRecurseWithPU.nextSection(tuRecurseCU));


  if( uiNumPU > 1 )
  { // set Cbf for all blocks
    UInt uiCombCbfY = 0;
    UInt uiCombCbfU = 0;
    UInt uiCombCbfV = 0;
    UInt uiPartIdx  = 0;
    for( UInt uiPart = 0; uiPart < 4; uiPart++, uiPartIdx += uiQNumParts )
    {
      uiCombCbfY |= pcCU->getCbf( uiPartIdx, COMPONENT_Y,  1 );
      uiCombCbfU |= pcCU->getCbf( uiPartIdx, COMPONENT_Cb, 1 );
      uiCombCbfV |= pcCU->getCbf( uiPartIdx, COMPONENT_Cr, 1 );
    }
    for( UInt uiOffs = 0; uiOffs < 4 * uiQNumParts; uiOffs++ )
    {
      pcCU->getCbf( COMPONENT_Y  )[ uiOffs ] |= uiCombCbfY;
      pcCU->getCbf( COMPONENT_Cb )[ uiOffs ] |= uiCombCbfU;
      pcCU->getCbf( COMPONENT_Cr )[ uiOffs ] |= uiCombCbfV;
    }
  }

  //===== reset context models =====
  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

  //===== set distortion (rate and r-d costs are determined later) =====
  pcCU->getTotalDistortion() = uiOverallDistY;
#ifdef CVI_INTRA_PRED
  pcCU->getTotalCost() = overallCostY;
#endif
}




Void
TEncSearch::estIntraPredChromaQT(TComDataCU* pcCU,
                                 TComYuv*    pcOrgYuv,
                                 TComYuv*    pcPredYuv,
                                 TComYuv*    pcResiYuv,
                                 TComYuv*    pcRecoYuv,
                                 Pel         resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE]
                                 DEBUG_STRING_FN_DECLARE(sDebug))
{
  const UInt    uiInitTrDepth  = pcCU->getPartitionSize(0) != SIZE_2Nx2N && enable4ChromaPUsInIntraNxNCU(pcOrgYuv->getChromaFormat()) ? 1 : 0;

  TComTURecurse tuRecurseCU(pcCU, 0);
  TComTURecurse tuRecurseWithPU(tuRecurseCU, false, (uiInitTrDepth==0)?TComTU::DONT_SPLIT : TComTU::QUAD_SPLIT);
  const UInt    uiQNumParts    = tuRecurseWithPU.GetAbsPartIdxNumParts();
  const UInt    uiDepthCU=tuRecurseWithPU.getCUDepth();
  const UInt    numberValidComponents = pcCU->getPic()->getNumberValidComponents();

  do
  {
    UInt       uiBestMode  = 0;
    Distortion uiBestDist  = 0;
    Double     dBestCost   = MAX_DOUBLE;

    //----- init mode list -----
    if (tuRecurseWithPU.ProcessChannelSection(CHANNEL_TYPE_CHROMA))
    {
      UInt uiModeList[FAST_UDI_MAX_RDMODE_NUM];
      const UInt  uiQPartNum     = uiQNumParts;
      const UInt  uiPartOffset   = tuRecurseWithPU.GetAbsPartIdxTU();
      {
        UInt  uiMinMode = 0;
        UInt  uiMaxMode = NUM_CHROMA_MODE;

        //----- check chroma modes -----
        pcCU->getAllowedChromaDir( uiPartOffset, uiModeList );
#ifdef SIGDUMP
        if (g_sigpool.luma_pu_size == 4) {
          g_sigpool.cu_chroma_4x4_count++;
        }
#endif
#ifdef CVI_INTRA_PRED
        bool chromaEnableCviIntraPred = g_algo_cfg.EnableCviIntraPred;
#endif

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
        if (DebugOptionList::ForceChromaMode.isSet())
        {
          uiMinMode=DebugOptionList::ForceChromaMode.getInt();
          if (uiModeList[uiMinMode]==34)
          {
            uiMinMode=4; // if the fixed mode has been renumbered because DM_CHROMA covers it, use DM_CHROMA.
          }
          uiMaxMode=uiMinMode+1;
        }
#endif

        DEBUG_STRING_NEW(sPU)

#ifdef CVI_INTRA_PRED
        if (chromaEnableCviIntraPred)
        {
          // CVI intra chroma mode decision
          //    Chroma 8x8 (luma 16x16): Use Hadamard transform
          //    Chroma 4x4 (Luma 8x8)  : Use Hadamard transform
          //    Chroma 4x4 (LUma 4x4)  : Use RDO
          int puSizeLuma = pcCU->getPartitionSize(0) == SIZE_NxN ?
                           4 : tuRecurseWithPU.getRect(COMPONENT_Y).width;
          UChar lumaMode = pcCU->getIntraDir(CHANNEL_TYPE_LUMA, uiPartOffset);

          int listSize = CVI_CHROMA_CAND_SIZE;
          UInt modeList[listSize];
          Double costList[listSize];
          for (int i = 0 ; i < listSize; i++)
          {
            modeList[i] = 0;
            costList[i] = MAX_DOUBLE;
          }

          if (puSizeLuma == 16 || puSizeLuma == 8)
            cviGetChromaCandidate(uiModeList, &uiMaxMode);

          Bool bUseHadamard = true;
          for( UInt uiMode = uiMinMode; uiMode < uiMaxMode; uiMode++ )
          {
            // Skip chroma I4 of PU NxN
            if (puSizeLuma == 4)
              break;

            Distortion uiSad  = 0;
            #ifdef SIG_IAPU
            if(g_sigdump.iapu && (g_sigpool.luma_pu_size==16 || g_sigpool.luma_pu_size==8)){
                int iapu_file_idx = (g_sigpool.luma_pu_size==8)? 1: 2;
                g_sigpool.iapu_st.c_mode_list[iapu_file_idx][uiMode] = uiModeList[uiMode];
            }
            #endif //~SIG_IAPU

            for (UInt ch=COMPONENT_Cb; ch<numberValidComponents; ch++)
            {
              const ComponentID compID    = ComponentID(ch);
              Pel* piOrg         = pcOrgYuv ->getAddr( compID, uiPartOffset );
              Pel* piPred        = pcPredYuv->getAddr( compID, uiPartOffset );
              UInt uiStride      = pcPredYuv->getStride( compID );
              UInt width = tuRecurseWithPU.getRect(compID).width;
              UInt height = tuRecurseWithPU.getRect(compID).height;
              DistParam distParam;
              m_pcRdCost->setDistParam(distParam, pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_CHROMA),
                                       piOrg, uiStride, piPred, uiStride, width, height, bUseHadamard);
              distParam.bApplyWeight = false;

              const Bool bUseFilter=TComPrediction::filteringIntraReferenceSamples(compID, uiModeList[uiMode], width, height, pcCU->getPic()->getChromaFormat(), pcCU->getSlice()->getSPS()->getSpsRangeExtension().getIntraSmoothingDisabledFlag());
              initIntraPatternChType( tuRecurseWithPU, compID, bUseFilter DEBUG_STRING_PASS_INTO(sDebug));

              const UInt uiChCodedMode = (uiModeList[uiMode] == DM_CHROMA_IDX) ? lumaMode : uiModeList[uiMode];
              predIntraAng(compID, uiChCodedMode, piOrg, uiStride, piPred, uiStride, tuRecurseWithPU, bUseFilter,
                           TComPrediction::UseDPCMForFirstPassIntraEstimation(tuRecurseWithPU, uiModeList[uiMode]));

              // use hadamard transform here
              uiSad+=distParam.DistFunc(&distParam);
              #ifdef SIG_IAPU
              if(g_sigdump.iapu && (g_sigpool.luma_pu_size==16 || g_sigpool.luma_pu_size==8)){
                  int iapu_file_idx = (g_sigpool.luma_pu_size==8)? 1: 2;
                  g_sigpool.iapu_st.c_mode_sad[iapu_file_idx][uiMode][ch-COMPONENT_Cb] = static_cast<UInt64>(distParam.DistFunc(&distParam));
              }
              #endif //~SIG_IAPU
            }

            modeList[uiMode] = uiModeList[uiMode];
            costList[uiMode] = (Double)(uiSad);
          }
          #ifdef SIG_IAPU
          if(g_sigdump.iapu && (g_sigpool.luma_pu_size==16 || g_sigpool.luma_pu_size==8)){
            int iapu_file_idx = (g_sigpool.luma_pu_size==8)? 1: 2;
            Pel* piOrg      = pcOrgYuv ->getAddr( COMPONENT_Cb, uiPartOffset );
            UInt uiStride   = pcOrgYuv->getStride( COMPONENT_Cb );
            Int  iWidth     = tuRecurseWithPU.getRect(COMPONENT_Cb).width;
            Int  iHeight    = tuRecurseWithPU.getRect(COMPONENT_Cb).height;
            for (Int y=0;y<iHeight;y++){
                for (Int x=0;x<iWidth;x++){
                    sigdump_output_bin(&g_sigpool.iapu_iap_tu_src_frm_ctx[iapu_file_idx], reinterpret_cast<unsigned char*>(&piOrg[y*uiStride+x]), 1);
                }
            }
            piOrg       = pcOrgYuv ->getAddr( COMPONENT_Cr, uiPartOffset );
            uiStride    = pcOrgYuv->getStride( COMPONENT_Cr );
            iWidth      = tuRecurseWithPU.getRect(COMPONENT_Cr).width;
            iHeight     = tuRecurseWithPU.getRect(COMPONENT_Cr).height;
            for (Int y=0;y<iHeight;y++){
                for (Int x=0;x<iWidth;x++){
                    sigdump_output_bin(&g_sigpool.iapu_iap_tu_src_frm_ctx[iapu_file_idx], reinterpret_cast<unsigned char*>(&piOrg[y*uiStride+x]), 1);
                }
            }
          }
          if(g_sigdump.iapu){
              int iapu_file_idx = (g_sigpool.luma_pu_size==4)? 0: (g_sigpool.luma_pu_size==8)? 1: 2;
              sig_iapu_output_neb(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx], g_sigpool.luma_pu_size, COMPONENT_Cb);
              sig_iapu_output_neb(&g_sigpool.iapu_iap_tu_neb_frm_ctx[iapu_file_idx], g_sigpool.luma_pu_size, COMPONENT_Cr);
          }
          #endif //~SIG_IAPU

          // verify chroma mode candidates.
          // if luma mode is not 1 of 4 basic modes, chroma mode cannot be 34
          UInt possible_modes[5] = { PLANAR_IDX, VER_IDX, HOR_IDX, DC_IDX, 34};
          if (lumaMode != PLANAR_IDX &&
              lumaMode != VER_IDX &&
              lumaMode != HOR_IDX &&
              lumaMode != DC_IDX)
          {
            possible_modes[4] = lumaMode;
          }

          if (puSizeLuma == 16 || puSizeLuma == 8)
          {
            // update mode cost
            UInt temp_list[CVI_CHROMA_CAND_SIZE] = { 0 };
            Double temp_cost[CVI_CHROMA_CAND_SIZE] = { 0 };
            UInt bits = 0;
            #ifdef SIG_IAPU
            if(g_sigdump.iapu){
                int iapu_file_idx = (g_sigpool.luma_pu_size==8)? 1: 2;
                g_sigpool.iapu_st.c_mode_lambda[iapu_file_idx] = m_pcRdCost->getFixpointSADSqrtLambda();
            }
            #endif //~SIG_IAPU

            for (int i = 0; i < CVI_CHROMA_CAND_SIZE; i++)
            {
              temp_cost[i] = MAX_DOUBLE;
            }

            for (int i = 0; i < uiMaxMode; i++)
            {
              UInt chroma_mode = modeList[i];
              double cost = costList[i];
              #ifdef SIG_IAPU
              UInt cmode_save = chroma_mode;
              if(g_sigdump.iapu){
                int iapu_file_idx = (g_sigpool.luma_pu_size==8)? 1: 2;
                int old_idx = std::find(g_sigpool.iapu_st.c_mode_list[iapu_file_idx], g_sigpool.iapu_st.c_mode_list[iapu_file_idx]+9, chroma_mode) - g_sigpool.iapu_st.c_mode_list[iapu_file_idx];
                g_sigpool.iapu_st.c_mode_cost[iapu_file_idx][old_idx] = m_pcRdCost->getFixpointRdCost(cost);
              }
              #endif //~SIG_IAPU
              for (int j = 0; j < 5; j++)
              {
                if (chroma_mode == possible_modes[j])
                {
                  if (chroma_mode == lumaMode)
                    chroma_mode = DM_CHROMA_IDX;

                  bits = xModeBitsIntra( pcCU, chroma_mode, uiPartOffset, uiDepthCU, CHANNEL_TYPE_CHROMA );

                  // add (bits * sqrt_lambda) to RD Cost
                  cost += m_pcRdCost->calcRdCost(bits, 0, DF_SAD);

                  #ifdef SIG_IAPU
                  if(g_sigdump.iapu){
                    int iapu_file_idx = (g_sigpool.luma_pu_size==8)? 1: 2;
                    int old_idx = std::find(g_sigpool.iapu_st.c_mode_list[iapu_file_idx], g_sigpool.iapu_st.c_mode_list[iapu_file_idx]+9, cmode_save) - g_sigpool.iapu_st.c_mode_list[iapu_file_idx];
                    g_sigpool.iapu_st.c_mode_cost[iapu_file_idx][old_idx] = m_pcRdCost->getFixpointRdCost(cost);
                  }
                  #endif //~SIG_IAPU

                  xUpdateCandList(chroma_mode, cost, uiMaxMode, temp_list, temp_cost);
                  break;
                }
              }
            }
            #ifdef SIG_IAPU
            if(g_sigdump.iapu){
                int iapu_file_idx = (g_sigpool.luma_pu_size==8)? 1: 2;
                for (int i = uiMinMode; i < uiMaxMode; i++){
                    sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "C_MODE = %u\n", static_cast<UInt>(g_sigpool.iapu_st.c_mode_list[iapu_file_idx][i]));
                    if(g_sigpool.luma_pu_size==4){
                    }else{
                        sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "SAD_CB = %d\n", static_cast<Int>(g_sigpool.iapu_st.c_mode_sad[iapu_file_idx][i][0]));
                        sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "SAD_CR = %d\n", static_cast<Int>(g_sigpool.iapu_st.c_mode_sad[iapu_file_idx][i][1]));
                        sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "COST = %d\n",  static_cast<Int>(g_sigpool.iapu_st.c_mode_cost[iapu_file_idx][i]));
                    }
                }
                sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "LAMBDA = %d\n", static_cast<Int>(g_sigpool.iapu_st.c_mode_lambda[iapu_file_idx]));
                sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "C_BEST_MODE = %u\n", static_cast<UInt>(temp_list[0]));
            }
            #endif //~SIG_IAPU

            uiMaxMode = 1;
            uiModeList[0] = temp_list[0];
          }
          else  // puSizeLuma == 4
          {
            for (int i = 0; i < uiMaxMode; i++)
            {
              uiModeList[i] = possible_modes[i];
              if (uiModeList[i] == lumaMode)
                uiModeList[i] = DM_CHROMA_IDX;
            }
          }
        }
#endif //~CVI_INTRA_PRED

        for( UInt uiMode = uiMinMode; uiMode < uiMaxMode; uiMode++ )
        {
          //----- restore context models -----
          m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepthCU][CI_CURR_BEST] );

          DEBUG_STRING_NEW(sMode)
          //----- chroma coding -----
          Distortion uiDist = 0;
          pcCU->setIntraDirSubParts  ( CHANNEL_TYPE_CHROMA, uiModeList[uiMode], uiPartOffset, uiDepthCU+uiInitTrDepth );
          xRecurIntraChromaCodingQT       ( pcOrgYuv, pcPredYuv, pcResiYuv, resiLuma, uiDist, tuRecurseWithPU DEBUG_STRING_PASS_INTO(sMode) );

          if( pcCU->getSlice()->getPPS()->getUseTransformSkip() )
          {
            m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepthCU][CI_CURR_BEST] );
          }

          UInt    uiBits = xGetIntraBitsQT( tuRecurseWithPU, false, true, false );
#ifdef CVI_ENABLE_RDO_BIT_EST
          Double dCost = 0.0;
          if ((getRDOBitEstMode()!=HM_ORIGINAL && getRDOSyntaxEstMode()!=HM_ORIGINAL) &&
              pcCU->getPartitionSize(0) == SIZE_NxN)
          {
            dCost = m_pcRdCost->calcFracBitsRdCost(uiBits, uiDist, I4_NUM_BIT_FRAC_BIT);
          }
          else
          {
            dCost = m_pcRdCost->calcRdCost( uiBits, uiDist );
          }
#else
          Double  dCost  = m_pcRdCost->calcRdCost( uiBits, uiDist );
#endif
          //----- compare -----
          if( dCost < dBestCost )
          {
            DEBUG_STRING_SWAP(sPU, sMode);
            dBestCost   = dCost;
            uiBestDist  = uiDist;
            uiBestMode  = uiModeList[uiMode];

            xSetIntraResultChromaQT( pcRecoYuv, tuRecurseWithPU );
            for (UInt componentIndex = COMPONENT_Cb; componentIndex < numberValidComponents; componentIndex++)
            {
              const ComponentID compID = ComponentID(componentIndex);
              ::memcpy( m_puhQTTempCbf[compID], pcCU->getCbf( compID )+uiPartOffset, uiQPartNum * sizeof( UChar ) );
              ::memcpy( m_puhQTTempTransformSkipFlag[compID], pcCU->getTransformSkip( compID )+uiPartOffset, uiQPartNum * sizeof( UChar ) );
              ::memcpy( m_phQTTempCrossComponentPredictionAlpha[compID], pcCU->getCrossComponentPredictionAlpha(compID)+uiPartOffset, uiQPartNum * sizeof( SChar ) );
#ifdef SIG_BIT_EST
              if (g_sigdump.bit_est) {
                memcpy(&g_sigpool.bit_est_golden[1][0][componentIndex], &g_sigpool.bit_est_golden[0][0][componentIndex], sizeof(sig_bit_est_st));
              }
              if (g_sigdump.rdo_sse) {
                memcpy(&g_sigpool.est_golden_intra[1][componentIndex], &g_sigpool.est_golden_intra[0][componentIndex], sizeof(sig_est_golden_intra_st));
              }
#endif
            }
#ifdef SIG_RRU
            if (g_sigdump.rru)
            {
              // update final buffer
              int puSize = tuRecurseWithPU.getRect(COMPONENT_Cb).width;
              sig_rru_update_final_buf(true, true, puSize);
              for (int i = 1; i <= 2; i++) {
                if (g_sigpool.luma_pu_size > 4) {
                  g_sigpool.rru_gold.rru_est_gold[1][i][0] = g_sigpool.rru_temp_gold.rru_est_gold[1][i][0];
                }
                else {
                  g_sigpool.rru_gold.rru_est_gold_4x4[i][0] = g_sigpool.rru_temp_gold.rru_est_gold[1][i][0];
                }
              }
            }
#endif //~SIG_RRU
#ifdef SIG_IAPU
            if (g_sigdump.iapu)
            {
              // update final buffer
              int puSize = tuRecurseWithPU.getRect(COMPONENT_Cb).width;
              sig_iapu_update_final_buf(true, true, puSize);
            }
#endif //~SIG_IAPU
          }

#ifdef SIG_IAPU
          if (g_sigdump.iapu)
          {
            if(g_sigpool.luma_pu_size==4) {
                int iapu_file_idx = 0;
                g_sigpool.iapu_st.c_mode_list[iapu_file_idx][uiMode] = uiModeList[uiMode];
                g_sigpool.iapu_st.c_mode_cost[iapu_file_idx][uiMode] = m_pcRdCost->getFixpointRdCost(dCost);
                g_sigpool.iapu_st.c_mode_distortion[uiMode] = uiDist;
                g_sigpool.iapu_st.c_mode_bins[uiMode] = g_sigpool.iapu_st.bin_count;
                g_sigpool.iapu_st.c_mode_bits[uiMode] = uiBits;
                g_sigpool.iapu_st.c_mode_bits_est_val[uiMode] = m_pcEntropyCoder->getNumberOfWrittenBitsFrac();
                g_sigpool.iapu_st.c_mode_best = uiBestMode;
                g_sigpool.iapu_st.c_mode_best_cost = m_pcRdCost->getFixpointRdCost(dBestCost);
                //tag

                for (UInt componentIndex = COMPONENT_Cb; componentIndex < numberValidComponents; componentIndex++){
                    Pel* piPred        = pcPredYuv->getAddr(ComponentID(componentIndex), uiPartOffset );
                    UInt uiStride      = pcPredYuv->getStride(ComponentID(componentIndex));

                    for (Int y=0;y<4;y++){
                        for (Int x=0;x<4;x++){
                            g_sigpool.iapu_st.tu4_pred_blk[componentIndex][uiMode][y*4+x] = static_cast<UChar>(piPred[y*uiStride+x]&0xff);
                        }
                    }
                }
            }
          }
#endif //~SIG_IAPU
        }

#ifdef SIG_RRU
        if (g_sigdump.rru)
        {
          sig_rru_output_intp(true, g_sigpool.luma_pu_size, true);
          sig_rru_output_dct(true, g_sigpool.luma_pu_size, true);

          if (g_sigpool.luma_pu_size == 4)
          {
            sig_rru_output_i4_resi(true);
          }

          //scan_idx
          if (g_sigpool.luma_pu_size >= 8)
          {
            g_sigpool.rru_scan_idx[0] = pcCU->getCoefScanIdx(0, tuRecurseWithPU.getRect(COMPONENT_Y).width,
                                                             tuRecurseWithPU.getRect(COMPONENT_Y).height,
                                                             COMPONENT_Y);
            g_sigpool.rru_scan_idx[1] = pcCU->getCoefScanIdx(0, tuRecurseWithPU.getRect(COMPONENT_Cb).width,
                                                             tuRecurseWithPU.getRect(COMPONENT_Cb).height,
                                                             COMPONENT_Cb);
          }
        }
#endif //~SIG_RRU
#ifdef SIG_IAPU
        if (g_sigdump.iapu)
        {
          if(g_sigpool.luma_pu_size==4) {
            sig_iapu_output_i4_resi(true);
            int iapu_file_idx = 0;
            for (int i = uiMinMode; i < uiMaxMode; i++){
              sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "C_MODE = %u\n",
                                    static_cast<UInt>(g_sigpool.iapu_st.c_mode_list[iapu_file_idx][i]));
              sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "RDO_COST = %lld,%lld,%llu,%d,%llu\n",
                                    g_sigpool.iapu_st.c_mode_cost[iapu_file_idx][i],
                                    g_sigpool.iapu_st.c_mode_distortion[i],
                                    g_sigpool.iapu_st.c_mode_bins[i], g_sigpool.iapu_st.c_mode_bits[i],
                                    g_sigpool.iapu_st.c_mode_bits_est_val[i]);
            }
            sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[iapu_file_idx], "C_BEST_MODE = %u\n",
                                  static_cast<UInt>(g_sigpool.iapu_st.c_mode_best));
          }
        }
#endif //~SIG_IAPU

        DEBUG_STRING_APPEND(sDebug, sPU)

        //----- set data -----
        for (UInt componentIndex = COMPONENT_Cb; componentIndex < numberValidComponents; componentIndex++)
        {
          const ComponentID compID = ComponentID(componentIndex);
          ::memcpy( pcCU->getCbf( compID )+uiPartOffset, m_puhQTTempCbf[compID], uiQPartNum * sizeof( UChar ) );
          ::memcpy( pcCU->getTransformSkip( compID )+uiPartOffset, m_puhQTTempTransformSkipFlag[compID], uiQPartNum * sizeof( UChar ) );
          ::memcpy( pcCU->getCrossComponentPredictionAlpha(compID)+uiPartOffset, m_phQTTempCrossComponentPredictionAlpha[compID], uiQPartNum * sizeof( SChar ) );
#ifdef SIG_BIT_EST
          if (g_sigdump.rdo_sse)
            sigdump_output_rdo_sse_intra(g_aucConvertToBit[g_sigpool.luma_pu_size], 0, componentIndex);
#endif
        }
      }

      if( ! tuRecurseWithPU.IsLastSection() )
      {
        for (UInt ch=COMPONENT_Cb; ch<numberValidComponents; ch++)
        {
          const ComponentID compID    = ComponentID(ch);
          const TComRectangle &tuRect = tuRecurseWithPU.getRect(compID);
          const UInt  uiCompWidth     = tuRect.width;
          const UInt  uiCompHeight    = tuRect.height;
          const UInt  uiZOrder        = pcCU->getZorderIdxInCtu() + tuRecurseWithPU.GetAbsPartIdxTU();
                Pel*  piDes           = pcCU->getPic()->getPicYuvRec()->getAddr( compID, pcCU->getCtuRsAddr(), uiZOrder );
          const UInt  uiDesStride     = pcCU->getPic()->getPicYuvRec()->getStride( compID);
          const Pel*  piSrc           = pcRecoYuv->getAddr( compID, uiPartOffset );
          const UInt  uiSrcStride     = pcRecoYuv->getStride( compID);

          for( UInt uiY = 0; uiY < uiCompHeight; uiY++, piSrc += uiSrcStride, piDes += uiDesStride )
          {
            for( UInt uiX = 0; uiX < uiCompWidth; uiX++ )
            {
              piDes[ uiX ] = piSrc[ uiX ];
            }
          }
        }
      }

      pcCU->setIntraDirSubParts( CHANNEL_TYPE_CHROMA, uiBestMode, uiPartOffset, uiDepthCU+uiInitTrDepth );
      pcCU->getTotalDistortion      () += uiBestDist;
#ifdef CVI_INTRA_PRED
      pcCU->getTotalCost() += dBestCost;
#endif //~CVI_INTRA_PRED
#ifdef SIG_IAPU
      if (g_sigdump.iapu)
      {
        if (g_sigpool.luma_pu_size == 4)
        {
          double i4_best_cost = pcCU->getTotalCost();
          sigdump_output_fprint(&g_sigpool.iapu_iap_tu_cand_frm_ctx[0], "CU_BEST_RDO = %lld\n",
                                m_pcRdCost->getFixpointRdCost(i4_best_cost));

          sig_iapu_save_blk4_self_info(-1, tuRecurseWithPU.GetSectionNumber());
        }
      }
#endif //~SIG_IAPU
#ifdef SIG_RRU
      //chroma sse add to first luma 4x4
      if (g_sigpool.luma_pu_size == 4 && g_sigdump.rru)
        g_sigpool.rru_gold.sse4x4[0] += uiBestDist;
#endif
    }
  } while (tuRecurseWithPU.nextSection(tuRecurseCU));

  //----- restore context models -----

  if( uiInitTrDepth != 0 )
  { // set Cbf for all blocks
    UInt uiCombCbfU = 0;
    UInt uiCombCbfV = 0;
    UInt uiPartIdx  = 0;
    for( UInt uiPart = 0; uiPart < 4; uiPart++, uiPartIdx += uiQNumParts )
    {
      uiCombCbfU |= pcCU->getCbf( uiPartIdx, COMPONENT_Cb, 1 );
      uiCombCbfV |= pcCU->getCbf( uiPartIdx, COMPONENT_Cr, 1 );
    }
    for( UInt uiOffs = 0; uiOffs < 4 * uiQNumParts; uiOffs++ )
    {
      pcCU->getCbf( COMPONENT_Cb )[ uiOffs ] |= uiCombCbfU;
      pcCU->getCbf( COMPONENT_Cr )[ uiOffs ] |= uiCombCbfV;
    }
  }

  m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[uiDepthCU][CI_CURR_BEST] );
}




/** Function for encoding and reconstructing luma/chroma samples of a PCM mode CU.
 * \param pcCU pointer to current CU
 * \param uiAbsPartIdx part index
 * \param pOrg pointer to original sample arrays
 * \param pPCM pointer to PCM code arrays
 * \param pPred pointer to prediction signal arrays
 * \param pResi pointer to residual signal arrays
 * \param pReco pointer to reconstructed sample arrays
 * \param uiStride stride of the original/prediction/residual sample arrays
 * \param uiWidth block width
 * \param uiHeight block height
 * \param compID texture component type
 */
Void TEncSearch::xEncPCM (TComDataCU* pcCU, UInt uiAbsPartIdx, Pel* pOrg, Pel* pPCM, Pel* pPred, Pel* pResi, Pel* pReco, UInt uiStride, UInt uiWidth, UInt uiHeight, const ComponentID compID )
{
  const UInt uiReconStride   = pcCU->getPic()->getPicYuvRec()->getStride(compID);
  const UInt uiPCMBitDepth   = pcCU->getSlice()->getSPS()->getPCMBitDepth(toChannelType(compID));
  const Int  channelBitDepth = pcCU->getSlice()->getSPS()->getBitDepth(toChannelType(compID));
  Pel* pRecoPic = pcCU->getPic()->getPicYuvRec()->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu()+uiAbsPartIdx);

  const Int pcmShiftRight=(channelBitDepth - Int(uiPCMBitDepth));

  assert(pcmShiftRight >= 0);

  for( UInt uiY = 0; uiY < uiHeight; uiY++ )
  {
    for( UInt uiX = 0; uiX < uiWidth; uiX++ )
    {
      // Reset pred and residual
      pPred[uiX] = 0;
      pResi[uiX] = 0;
      // Encode
      pPCM[uiX] = (pOrg[uiX]>>pcmShiftRight);
      // Reconstruction
      pReco   [uiX] = (pPCM[uiX]<<(pcmShiftRight));
      pRecoPic[uiX] = pReco[uiX];
    }
    pPred += uiStride;
    pResi += uiStride;
    pPCM += uiWidth;
    pOrg += uiStride;
    pReco += uiStride;
    pRecoPic += uiReconStride;
  }
}


//!  Function for PCM mode estimation.
Void TEncSearch::IPCMSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv* pcPredYuv, TComYuv* pcResiYuv, TComYuv* pcRecoYuv )
{
  UInt              uiDepth      = pcCU->getDepth(0);
  const Distortion  uiDistortion = 0;
  UInt              uiBits;

  Double dCost;

  for (UInt ch=0; ch < pcCU->getPic()->getNumberValidComponents(); ch++)
  {
    const ComponentID compID  = ComponentID(ch);
    const UInt width  = pcCU->getWidth(0)  >> pcCU->getPic()->getComponentScaleX(compID);
    const UInt height = pcCU->getHeight(0) >> pcCU->getPic()->getComponentScaleY(compID);
    const UInt stride = pcPredYuv->getStride(compID);

    Pel * pOrig    = pcOrgYuv->getAddr  (compID, 0, width);
    Pel * pResi    = pcResiYuv->getAddr(compID, 0, width);
    Pel * pPred    = pcPredYuv->getAddr(compID, 0, width);
    Pel * pReco    = pcRecoYuv->getAddr(compID, 0, width);
    Pel * pPCM     = pcCU->getPCMSample (compID);

    xEncPCM ( pcCU, 0, pOrig, pPCM, pPred, pResi, pReco, stride, width, height, compID );

  }

  m_pcEntropyCoder->resetBits();
  xEncIntraHeader ( pcCU, uiDepth, 0, true, false);
  uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();

  dCost = m_pcRdCost->calcRdCost( uiBits, uiDistortion );

  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

  pcCU->getTotalBits()       = uiBits;
  pcCU->getTotalCost()       = dCost;
  pcCU->getTotalDistortion() = uiDistortion;

  pcCU->copyToPic(uiDepth);
}




Void TEncSearch::xGetInterPredictionError( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPartIdx, Distortion& ruiErr, Bool /*bHadamard*/ )
{
  motionCompensation( pcCU, &m_tmpYuvPred, REF_PIC_LIST_X, iPartIdx );

  UInt uiAbsPartIdx = 0;
  Int iWidth = 0;
  Int iHeight = 0;
  pcCU->getPartIndexAndSize( iPartIdx, uiAbsPartIdx, iWidth, iHeight );

  DistParam cDistParam;

  cDistParam.bApplyWeight = false;


  m_pcRdCost->setDistParam( cDistParam, pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA),
                            pcYuvOrg->getAddr( COMPONENT_Y, uiAbsPartIdx ), pcYuvOrg->getStride(COMPONENT_Y),
                            m_tmpYuvPred .getAddr( COMPONENT_Y, uiAbsPartIdx ), m_tmpYuvPred.getStride(COMPONENT_Y),
                            iWidth, iHeight, m_pcEncCfg->getUseHADME() && (pcCU->getCUTransquantBypass(iPartIdx) == 0) );

  ruiErr = cDistParam.DistFunc( &cDistParam );
}

//! estimation of best merge coding
Void TEncSearch::xMergeEstimation( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPUIdx, UInt& uiInterDir, TComMvField* pacMvField, UInt& uiMergeIndex, Distortion& ruiCost, TComMvField* cMvFieldNeighbours, UChar* uhInterDirNeighbours, Int& numValidMergeCand )
{
  UInt uiAbsPartIdx = 0;
  Int iWidth = 0;
  Int iHeight = 0;

  pcCU->getPartIndexAndSize( iPUIdx, uiAbsPartIdx, iWidth, iHeight );
  UInt uiDepth = pcCU->getDepth( uiAbsPartIdx );

  PartSize partSize = pcCU->getPartitionSize( 0 );
  if ( pcCU->getSlice()->getPPS()->getLog2ParallelMergeLevelMinus2() && partSize != SIZE_2Nx2N && pcCU->getWidth( 0 ) <= 8 )
  {
    if ( iPUIdx == 0 )
    {
      pcCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth ); // temporarily set
#if MCTS_ENC_CHECK
      UInt numSpatialMergeCandidates = 0;
      pcCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, numSpatialMergeCandidates );
      if (m_pcEncCfg->getTMCTSSEITileConstraint() && pcCU->isLastColumnCTUInTile())
      {
        numValidMergeCand = numSpatialMergeCandidates;
      }
#else
      pcCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );
#endif
      pcCU->setPartSizeSubParts( partSize, 0, uiDepth ); // restore
    }
  }
  else
  {
#if MCTS_ENC_CHECK
    UInt numSpatialMergeCandidates = 0;
    pcCU->getInterMergeCandidates( uiAbsPartIdx, iPUIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, numSpatialMergeCandidates );
    if (m_pcEncCfg->getTMCTSSEITileConstraint() && pcCU->isLastColumnCTUInTile())
    {
      numValidMergeCand = numSpatialMergeCandidates;
    }
#else
    pcCU->getInterMergeCandidates( uiAbsPartIdx, iPUIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand );
#endif
  }

  xRestrictBipredMergeCand( pcCU, iPUIdx, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand );

  ruiCost = std::numeric_limits<Distortion>::max();
  for( UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand )
  {
    Distortion uiCostCand = std::numeric_limits<Distortion>::max();
    UInt       uiBitsCand = 0;

    PartSize ePartSize = pcCU->getPartitionSize( 0 );

    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand], ePartSize, uiAbsPartIdx, 0, iPUIdx );
    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand], ePartSize, uiAbsPartIdx, 0, iPUIdx );

    xGetInterPredictionError( pcCU, pcYuvOrg, iPUIdx, uiCostCand, m_pcEncCfg->getUseHADME() );
    uiBitsCand = uiMergeCand + 1;
    if (uiMergeCand == m_pcEncCfg->getMaxNumMergeCand() -1)
    {
        uiBitsCand--;
    }
    uiCostCand = uiCostCand + m_pcRdCost->getCost( uiBitsCand );
    if ( uiCostCand < ruiCost )
    {
      ruiCost = uiCostCand;
      pacMvField[0] = cMvFieldNeighbours[0 + 2*uiMergeCand];
      pacMvField[1] = cMvFieldNeighbours[1 + 2*uiMergeCand];
      uiInterDir = uhInterDirNeighbours[uiMergeCand];
      uiMergeIndex = uiMergeCand;
    }
  }
}

/** convert bi-pred merge candidates to uni-pred
 * \param pcCU
 * \param puIdx
 * \param mvFieldNeighbours
 * \param interDirNeighbours
 * \param numValidMergeCand
 * \returns Void
 */
Void TEncSearch::xRestrictBipredMergeCand( TComDataCU* pcCU, UInt puIdx, TComMvField* mvFieldNeighbours, UChar* interDirNeighbours, Int numValidMergeCand )
{
  if ( pcCU->isBipredRestriction(puIdx) )
  {
    for( UInt mergeCand = 0; mergeCand < numValidMergeCand; ++mergeCand )
    {
      if ( interDirNeighbours[mergeCand] == 3 )
      {
        interDirNeighbours[mergeCand] = 1;
        mvFieldNeighbours[(mergeCand << 1) + 1].setMvField(TComMv(0,0), -1);
      }
    }
  }
}

//! search of the best candidate for inter prediction
#if AMP_MRG
Void TEncSearch::predInterSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv* pcPredYuv, TComYuv* pcResiYuv, TComYuv* pcRecoYuv DEBUG_STRING_FN_DECLARE(sDebug), Bool bUseRes, Bool bUseMRG )
#else
Void TEncSearch::predInterSearch( TComDataCU* pcCU, TComYuv* pcOrgYuv, TComYuv* pcPredYuv, TComYuv* pcResiYuv, TComYuv* pcRecoYuv, Bool bUseRes )
#endif
{
  for(UInt i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_acYuvPred[i].clear();
  }
  m_cYuvPredTemp.clear();
  pcPredYuv->clear();

  if ( !bUseRes )
  {
    pcResiYuv->clear();
  }

  pcRecoYuv->clear();

  TComMv       cMvSrchRngLT;
  TComMv       cMvSrchRngRB;

  TComMv       cMvZero;
  TComMv       TempMv; //kolya

  TComMv       cMv[2];
  TComMv       cMvBi[2];
  TComMv       cMvTemp[2][33];

  Int          iNumPart    = pcCU->getNumPartitions();
  Int          iNumPredDir = pcCU->getSlice()->isInterP() ? 1 : 2;

  TComMv       cMvPred[2][33];

  TComMv       cMvPredBi[2][33];
  Int          aaiMvpIdxBi[2][33];

  Int          aaiMvpIdx[2][33];
  Int          aaiMvpNum[2][33];

  AMVPInfo     aacAMVPInfo[2][33];

  Int          iRefIdx[2]={0,0}; //If un-initialized, may cause SEGV in bi-directional prediction iterative stage.
  Int          iRefIdxBi[2];

  UInt         uiPartAddr;
  Int          iRoiWidth, iRoiHeight;

  UInt         uiMbBits[3] = {1, 1, 0};

  UInt         uiLastMode = 0;
  Int          iRefStart, iRefEnd;

  PartSize     ePartSize = pcCU->getPartitionSize( 0 );

  Int          bestBiPRefIdxL1 = 0;
  Int          bestBiPMvpL1 = 0;
  Distortion   biPDistTemp = std::numeric_limits<Distortion>::max();

  TComMvField cMvFieldNeighbours[MRG_MAX_NUM_CANDS << 1]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  Int numValidMergeCand = 0 ;
#ifdef CVI_STORE_IMV
  TComMv cBestIMEMv[2]; //L0, L1
#endif
#ifdef CVI_FAST_B_FRAME
TComMv IME_list[2] = {TComMv(0,0)};
#endif
  for ( Int iPartIdx = 0; iPartIdx < iNumPart; iPartIdx++ )
  {
    Distortion   uiCost[2] = { std::numeric_limits<Distortion>::max(), std::numeric_limits<Distortion>::max() };
    Distortion   uiCostBi  =   std::numeric_limits<Distortion>::max();
    Distortion   uiCostTemp;

    UInt         uiBits[3];
    UInt         uiBitsTemp;
    Distortion   bestBiPDist = std::numeric_limits<Distortion>::max();

    Distortion   uiCostTempL0[MAX_NUM_REF];
    for (Int iNumRef=0; iNumRef < MAX_NUM_REF; iNumRef++)
    {
      uiCostTempL0[iNumRef] = std::numeric_limits<Distortion>::max();
    }
    UInt         uiBitsTempL0[MAX_NUM_REF];

    TComMv       mvValidList1;
    Int          refIdxValidList1 = 0;
    UInt         bitsValidList1 = MAX_UINT;
    Distortion   costValidList1 = std::numeric_limits<Distortion>::max();

    xGetBlkBits( ePartSize, pcCU->getSlice()->isInterP(), iPartIdx, uiLastMode, uiMbBits);

    pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iRoiWidth, iRoiHeight );

#if AMP_MRG
    Bool bTestNormalMC = true;

    if ( bUseMRG && pcCU->getWidth( 0 ) > 8 && iNumPart == 2 )
    {
      bTestNormalMC = false;
    }

    if (bTestNormalMC)
    {
#endif
    //  Uni-directional prediction
    for ( Int iRefList = 0; iRefList < iNumPredDir; iRefList++ )
    {
      RefPicList  eRefPicList = ( iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );

      for ( Int iRefIdxTemp = 0; iRefIdxTemp < pcCU->getSlice()->getNumRefIdx(eRefPicList); iRefIdxTemp++ )
      {
        uiBitsTemp = uiMbBits[iRefList];
        if ( pcCU->getSlice()->getNumRefIdx(eRefPicList) > 1 )
        {
          uiBitsTemp += iRefIdxTemp+1;
          if ( iRefIdxTemp == pcCU->getSlice()->getNumRefIdx(eRefPicList)-1 )
          {
            uiBitsTemp--;
          }
        }

#ifdef CVI_STORE_IMV
        xEstimateMvPredAMVP( pcCU, pcOrgYuv, iPartIdx, eRefPicList, iRefIdxTemp, cMvPred[iRefList][iRefIdxTemp], false, &biPDistTemp, &cBestIMEMv[eRefPicList]);
        //8x8 use 16x16 IME MVP
        if ((iRoiWidth == 16 && iRoiHeight == 16) || is_use_8x8_ime_mvp(pcCU->getCUPelX(), pcCU->getCUPelY(),
          pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples(), pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples())) {
          g_cvi_var.ime_mvp_mvx[eRefPicList][iRefIdxTemp] = cBestIMEMv[eRefPicList].getHor();
          g_cvi_var.ime_mvp_mvy[eRefPicList][iRefIdxTemp] = cBestIMEMv[eRefPicList].getVer();
        } else {
          cBestIMEMv[eRefPicList].set(g_cvi_var.ime_mvp_mvx[eRefPicList][iRefIdxTemp], g_cvi_var.ime_mvp_mvy[eRefPicList][iRefIdxTemp]);
        }
#else
        xEstimateMvPredAMVP( pcCU, pcOrgYuv, iPartIdx, eRefPicList, iRefIdxTemp, cMvPred[iRefList][iRefIdxTemp], false, &biPDistTemp);
#endif

        aaiMvpIdx[iRefList][iRefIdxTemp] = pcCU->getMVPIdx(eRefPicList, uiPartAddr);
        aaiMvpNum[iRefList][iRefIdxTemp] = pcCU->getMVPNum(eRefPicList, uiPartAddr);

        if(pcCU->getSlice()->getMvdL1ZeroFlag() && iRefList==1 && biPDistTemp < bestBiPDist)
        {
          bestBiPDist = biPDistTemp;
          bestBiPMvpL1 = aaiMvpIdx[iRefList][iRefIdxTemp];
          bestBiPRefIdxL1 = iRefIdxTemp;
        }

        uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdx[iRefList][iRefIdxTemp]][AMVP_MAX_NUM_CANDS];

        if ( m_pcEncCfg->getFastMEForGenBLowDelayEnabled() && iRefList == 1 )    // list 1
        {
          if ( pcCU->getSlice()->getList1IdxToList0Idx( iRefIdxTemp ) >= 0 )
          {
            cMvTemp[1][iRefIdxTemp] = cMvTemp[0][pcCU->getSlice()->getList1IdxToList0Idx( iRefIdxTemp )];
            uiCostTemp = uiCostTempL0[pcCU->getSlice()->getList1IdxToList0Idx( iRefIdxTemp )];
            /*first subtract the bit-rate part of the cost of the other list*/
            uiCostTemp -= m_pcRdCost->getCost( uiBitsTempL0[pcCU->getSlice()->getList1IdxToList0Idx( iRefIdxTemp )] );
            /*correct the bit-rate part of the current ref*/
            m_pcRdCost->setPredictor  ( cMvPred[iRefList][iRefIdxTemp] );
            uiBitsTemp += m_pcRdCost->getBitsOfVectorWithPredictor( cMvTemp[1][iRefIdxTemp].getHor(), cMvTemp[1][iRefIdxTemp].getVer() );
            /*calculate the correct cost*/
            uiCostTemp += m_pcRdCost->getCost( uiBitsTemp );
          }
          else
          {
            xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp, false, &cBestIMEMv[iRefList] );
          }
        }
        else
        {
#ifdef CVI_STORE_IMV
          xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp, false, &cBestIMEMv[iRefList], &IME_list[iRefList]);
#else
          xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPred[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp );
#endif
        }
        xCopyAMVPInfo(pcCU->getCUMvField(eRefPicList)->getAMVPInfo(), &aacAMVPInfo[iRefList][iRefIdxTemp]); // must always be done ( also when AMVP_MODE = AM_NONE )
        xCheckBestMVP(pcCU, eRefPicList, cMvTemp[iRefList][iRefIdxTemp], cMvPred[iRefList][iRefIdxTemp], aaiMvpIdx[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp);

        if ( iRefList == 0 )
        {
          uiCostTempL0[iRefIdxTemp] = uiCostTemp;
          uiBitsTempL0[iRefIdxTemp] = uiBitsTemp;
        }
        if ( uiCostTemp < uiCost[iRefList] )
        {
          uiCost[iRefList] = uiCostTemp;
          uiBits[iRefList] = uiBitsTemp; // storing for bi-prediction

          // set motion
          cMv[iRefList]     = cMvTemp[iRefList][iRefIdxTemp];
          iRefIdx[iRefList] = iRefIdxTemp;
        }

        if ( iRefList == 1 && uiCostTemp < costValidList1 && pcCU->getSlice()->getList1IdxToList0Idx( iRefIdxTemp ) < 0 )
        {
          costValidList1 = uiCostTemp;
          bitsValidList1 = uiBitsTemp;

          // set motion
          mvValidList1     = cMvTemp[iRefList][iRefIdxTemp];
          refIdxValidList1 = iRefIdxTemp;
        }
      }
    }

    //  Bi-predictive Motion estimation
    if ( (pcCU->getSlice()->isInterB()) && (pcCU->isBipredRestriction(iPartIdx) == false) )
    {

      cMvBi[0] = cMv[0];            cMvBi[1] = cMv[1];
      iRefIdxBi[0] = iRefIdx[0];    iRefIdxBi[1] = iRefIdx[1];

      ::memcpy(cMvPredBi, cMvPred, sizeof(cMvPred));
      ::memcpy(aaiMvpIdxBi, aaiMvpIdx, sizeof(aaiMvpIdx));

      UInt uiMotBits[2];

      if(pcCU->getSlice()->getMvdL1ZeroFlag())
      {
        xCopyAMVPInfo(&aacAMVPInfo[1][bestBiPRefIdxL1], pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
        pcCU->setMVPIdxSubParts( bestBiPMvpL1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        aaiMvpIdxBi[1][bestBiPRefIdxL1] = bestBiPMvpL1;
        cMvPredBi[1][bestBiPRefIdxL1]   = pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo()->m_acMvCand[bestBiPMvpL1];

        cMvBi[1] = cMvPredBi[1][bestBiPRefIdxL1];
        iRefIdxBi[1] = bestBiPRefIdxL1;
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMv( cMvBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllRefIdx( iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
        TComYuv* pcYuvPred = &m_acYuvPred[REF_PIC_LIST_1];
        motionCompensation( pcCU, pcYuvPred, REF_PIC_LIST_1, iPartIdx );

        uiMotBits[0] = uiBits[0] - uiMbBits[0];
        uiMotBits[1] = uiMbBits[1];

        if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1) > 1 )
        {
          uiMotBits[1] += bestBiPRefIdxL1+1;
          if ( bestBiPRefIdxL1 == pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_1)-1 )
          {
            uiMotBits[1]--;
          }
        }

        uiMotBits[1] += m_auiMVPIdxCost[aaiMvpIdxBi[1][bestBiPRefIdxL1]][AMVP_MAX_NUM_CANDS];

        uiBits[2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];

        cMvTemp[1][bestBiPRefIdxL1] = cMvBi[1];
      }
      else
      {
        uiMotBits[0] = uiBits[0] - uiMbBits[0];
        uiMotBits[1] = uiBits[1] - uiMbBits[1];
        uiBits[2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];
      }

      // 4-times iteration (default)
      Int iNumIter = 4;

      // fast encoder setting: only one iteration
      if ( m_pcEncCfg->getFastInterSearchMode()==FASTINTERSEARCH_MODE1 || m_pcEncCfg->getFastInterSearchMode()==FASTINTERSEARCH_MODE2 || pcCU->getSlice()->getMvdL1ZeroFlag() )
      {
        iNumIter = 1;
      }
#ifdef CVI_FAST_B_FRAME   
      if(m_pcEncCfg->getFastBiPred() == 1 || m_pcEncCfg->getFastBiPred() == 2){
        iNumIter = 0;
      }

      if(m_pcEncCfg->getFastBiPred() == 2){
      cMvBi[0] = IME_list[0];
      cMvBi[1] = IME_list[1];

      m_pcRdCost->setCostScale( 0 );
      
      m_pcRdCost->setPredictor  ( cBestIMEMv[0] );
      uiMotBits[0] = m_pcRdCost->getBitsOfVectorWithPredictor( cMvBi[0].getHor(), cMvBi[0].getVer() ) + 1;
      
      m_pcRdCost->setPredictor  ( cBestIMEMv[1] );
      uiMotBits[1] = m_pcRdCost->getBitsOfVectorWithPredictor( cMvBi[1].getHor(), cMvBi[1].getVer() ) + 1;
      
      uiBits[2] = uiMbBits[2] + uiMotBits[0] + uiMotBits[1];

      pcCU->getCUMvField(RefPicList(0))->setAllMv( cMvBi[0], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(RefPicList(0))->setAllRefIdx( iRefIdx[0], ePartSize, uiPartAddr, 0, iPartIdx );

      pcCU->getCUMvField(RefPicList(1))->setAllMv( cMvBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(RefPicList(1))->setAllRefIdx( iRefIdx[1], ePartSize, uiPartAddr, 0, iPartIdx );
        

      motionCompensation ( pcCU, pcPredYuv, REF_PIC_LIST_X, iPartIdx );

      DistParam cDtParam;
      const Int predSizeX = iRoiWidth, predSizeY = iRoiHeight;
      m_pcRdCost->setDistParam( predSizeX, predSizeY, DF_SAD, cDtParam );

      cDtParam.pCur       = pcOrgYuv->getAddr(COMPONENT_Y, uiPartAddr);
      cDtParam.iStrideCur = pcOrgYuv->getStride(COMPONENT_Y);
    
      //pred buffer
      cDtParam.pOrg       = pcPredYuv->getAddr(COMPONENT_Y, uiPartAddr);
      cDtParam.iStrideOrg = pcPredYuv->getStride(COMPONENT_Y);
      
      cDtParam.iSubShift  = 0;
      cDtParam.bApplyWeight = false;
      cDtParam.compIdx      = MAX_NUM_COMPONENT; // just for assert: to be sure it was set before use
      cDtParam.bitDepth     = pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA);

      uiCostBi  = m_pcRdCost->xGetBiSAD(&cDtParam, 0);
      uiCostBi += m_pcRdCost->getCost( uiBits[2] );

      iRefIdxBi[0] = 0;
      iRefIdxBi[1] = 0;

      if ( uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1] )
      {
        xCopyAMVPInfo(&aacAMVPInfo[0][iRefIdxBi[0]], pcCU->getCUMvField(REF_PIC_LIST_0)->getAMVPInfo());
        xCheckBestMVP(pcCU, REF_PIC_LIST_0, cMvBi[0], cMvPredBi[0][iRefIdxBi[0]], aaiMvpIdxBi[0][iRefIdxBi[0]], uiBits[2], uiCostBi);
        if(!pcCU->getSlice()->getMvdL1ZeroFlag())
        {
          xCopyAMVPInfo(&aacAMVPInfo[1][iRefIdxBi[1]], pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
          xCheckBestMVP(pcCU, REF_PIC_LIST_1, cMvBi[1], cMvPredBi[1][iRefIdxBi[1]], aaiMvpIdxBi[1][iRefIdxBi[1]], uiBits[2], uiCostBi);
        }
      }

      pcPredYuv->clear();


      }
#endif

      for ( Int iIter = 0; iIter < iNumIter; iIter++ )
      {
        Int         iRefList    = iIter % 2;

        if ( m_pcEncCfg->getFastInterSearchMode()==FASTINTERSEARCH_MODE1 || m_pcEncCfg->getFastInterSearchMode()==FASTINTERSEARCH_MODE2 )
        {
          if( uiCost[0] <= uiCost[1] )
          {
            iRefList = 1;
          }
          else
          {
            iRefList = 0;
          }
        }
        else if ( iIter == 0 )
        {
          iRefList = 0;
        }
        if ( iIter == 0 && !pcCU->getSlice()->getMvdL1ZeroFlag())
        {
          pcCU->getCUMvField(RefPicList(1-iRefList))->setAllMv( cMv[1-iRefList], ePartSize, uiPartAddr, 0, iPartIdx );
          pcCU->getCUMvField(RefPicList(1-iRefList))->setAllRefIdx( iRefIdx[1-iRefList], ePartSize, uiPartAddr, 0, iPartIdx );
          TComYuv*  pcYuvPred = &m_acYuvPred[1-iRefList];
          motionCompensation ( pcCU, pcYuvPred, RefPicList(1-iRefList), iPartIdx );
        }

        RefPicList  eRefPicList = ( iRefList ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );

        if(pcCU->getSlice()->getMvdL1ZeroFlag())
        {
          iRefList = 0;
          eRefPicList = REF_PIC_LIST_0;
        }

        Bool bChanged = false;

        iRefStart = 0;
        iRefEnd   = pcCU->getSlice()->getNumRefIdx(eRefPicList)-1;

        for ( Int iRefIdxTemp = iRefStart; iRefIdxTemp <= iRefEnd; iRefIdxTemp++ )
        {
          uiBitsTemp = uiMbBits[2] + uiMotBits[1-iRefList];
          if ( pcCU->getSlice()->getNumRefIdx(eRefPicList) > 1 )
          {
            uiBitsTemp += iRefIdxTemp+1;
            if ( iRefIdxTemp == pcCU->getSlice()->getNumRefIdx(eRefPicList)-1 )
            {
              uiBitsTemp--;
            }
          }
          uiBitsTemp += m_auiMVPIdxCost[aaiMvpIdxBi[iRefList][iRefIdxTemp]][AMVP_MAX_NUM_CANDS];
          // call ME
#ifdef CVI_STORE_IMV
          xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPredBi[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp, true, &cBestIMEMv[iRefList]);
#else
          xMotionEstimation ( pcCU, pcOrgYuv, iPartIdx, eRefPicList, &cMvPredBi[iRefList][iRefIdxTemp], iRefIdxTemp, cMvTemp[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp, true );
#endif
          xCopyAMVPInfo(&aacAMVPInfo[iRefList][iRefIdxTemp], pcCU->getCUMvField(eRefPicList)->getAMVPInfo());
          xCheckBestMVP(pcCU, eRefPicList, cMvTemp[iRefList][iRefIdxTemp], cMvPredBi[iRefList][iRefIdxTemp], aaiMvpIdxBi[iRefList][iRefIdxTemp], uiBitsTemp, uiCostTemp);

          if ( uiCostTemp < uiCostBi )
          {
            bChanged = true;

            cMvBi[iRefList]     = cMvTemp[iRefList][iRefIdxTemp];
            iRefIdxBi[iRefList] = iRefIdxTemp;

            uiCostBi            = uiCostTemp;
            uiMotBits[iRefList] = uiBitsTemp - uiMbBits[2] - uiMotBits[1-iRefList];
            uiBits[2]           = uiBitsTemp;

            if(iNumIter!=1)
            {
              //  Set motion
              pcCU->getCUMvField( eRefPicList )->setAllMv( cMvBi[iRefList], ePartSize, uiPartAddr, 0, iPartIdx );
              pcCU->getCUMvField( eRefPicList )->setAllRefIdx( iRefIdxBi[iRefList], ePartSize, uiPartAddr, 0, iPartIdx );

              TComYuv* pcYuvPred = &m_acYuvPred[iRefList];
              motionCompensation( pcCU, pcYuvPred, eRefPicList, iPartIdx );
            }
          }
        } // for loop-iRefIdxTemp

        if ( !bChanged )
        {
          if ( uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1] )
          {
            xCopyAMVPInfo(&aacAMVPInfo[0][iRefIdxBi[0]], pcCU->getCUMvField(REF_PIC_LIST_0)->getAMVPInfo());
            xCheckBestMVP(pcCU, REF_PIC_LIST_0, cMvBi[0], cMvPredBi[0][iRefIdxBi[0]], aaiMvpIdxBi[0][iRefIdxBi[0]], uiBits[2], uiCostBi);
            if(!pcCU->getSlice()->getMvdL1ZeroFlag())
            {
              xCopyAMVPInfo(&aacAMVPInfo[1][iRefIdxBi[1]], pcCU->getCUMvField(REF_PIC_LIST_1)->getAMVPInfo());
              xCheckBestMVP(pcCU, REF_PIC_LIST_1, cMvBi[1], cMvPredBi[1][iRefIdxBi[1]], aaiMvpIdxBi[1][iRefIdxBi[1]], uiBits[2], uiCostBi);
            }
          }
          break;
        }
      } // for loop-iter
    } // if (B_SLICE)

#if AMP_MRG
    } //end if bTestNormalMC
#endif
    //  Clear Motion Field
    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField( TComMvField(), ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );
    pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,       ePartSize, uiPartAddr, 0, iPartIdx );

    pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

    UInt uiMEBits = 0;
    // Set Motion Field_
    cMv[1] = mvValidList1;
    iRefIdx[1] = refIdxValidList1;
    uiBits[1] = bitsValidList1;
    uiCost[1] = costValidList1;

#if AMP_MRG
    if (bTestNormalMC)
    {
#endif
    if ( uiCostBi <= uiCost[0] && uiCostBi <= uiCost[1])
    {
      uiLastMode = 2;
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMv( cMvBi[0], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx( iRefIdxBi[0], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMv( cMvBi[1], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx( iRefIdxBi[1], ePartSize, uiPartAddr, 0, iPartIdx );

      TempMv = cMvBi[0] - cMvPredBi[0][iRefIdxBi[0]];
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );

      TempMv = cMvBi[1] - cMvPredBi[1][iRefIdxBi[1]];
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );

      pcCU->setInterDirSubParts( 3, uiPartAddr, iPartIdx, pcCU->getDepth(0) );

      pcCU->setMVPIdxSubParts( aaiMvpIdxBi[0][iRefIdxBi[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[0][iRefIdxBi[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPIdxSubParts( aaiMvpIdxBi[1][iRefIdxBi[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[1][iRefIdxBi[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

      uiMEBits = uiBits[2];
    }
    else if ( uiCost[0] <= uiCost[1] )
    {
      uiLastMode = 0;
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMv( cMv[0], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllRefIdx( iRefIdx[0], ePartSize, uiPartAddr, 0, iPartIdx );

      TempMv = cMv[0] - cMvPred[0][iRefIdx[0]];
      pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );

      pcCU->setInterDirSubParts( 1, uiPartAddr, iPartIdx, pcCU->getDepth(0) );

      pcCU->setMVPIdxSubParts( aaiMvpIdx[0][iRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[0][iRefIdx[0]], REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

      uiMEBits = uiBits[0];
    }
    else
    {
      uiLastMode = 1;
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMv( cMv[1], ePartSize, uiPartAddr, 0, iPartIdx );
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllRefIdx( iRefIdx[1], ePartSize, uiPartAddr, 0, iPartIdx );

      TempMv = cMv[1] - cMvPred[1][iRefIdx[1]];
      pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( TempMv,                 ePartSize, uiPartAddr, 0, iPartIdx );

      pcCU->setInterDirSubParts( 2, uiPartAddr, iPartIdx, pcCU->getDepth(0) );

      pcCU->setMVPIdxSubParts( aaiMvpIdx[1][iRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      pcCU->setMVPNumSubParts( aaiMvpNum[1][iRefIdx[1]], REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));

      uiMEBits = uiBits[1];
    }
#if AMP_MRG
    } // end if bTestNormalMC
#endif

    if ( pcCU->getPartitionSize( uiPartAddr ) != SIZE_2Nx2N )
    {
      UInt uiMRGInterDir = 0;
      TComMvField cMRGMvField[2];
      UInt uiMRGIndex = 0;

      UInt uiMEInterDir = 0;
      TComMvField cMEMvField[2];

      m_pcRdCost->selectMotionLambda( true, 0, pcCU->getCUTransquantBypass(uiPartAddr) );

#if AMP_MRG
      // calculate ME cost
      Distortion uiMEError = std::numeric_limits<Distortion>::max();
      Distortion uiMECost  = std::numeric_limits<Distortion>::max();

      if (bTestNormalMC)
      {
        xGetInterPredictionError( pcCU, pcOrgYuv, iPartIdx, uiMEError, m_pcEncCfg->getUseHADME() );
        uiMECost = uiMEError + m_pcRdCost->getCost( uiMEBits );
      }
#else
      // calculate ME cost
      Distortion uiMEError = std::numeric_limits<Distortion>::max();
      xGetInterPredictionError( pcCU, pcOrgYuv, iPartIdx, uiMEError, m_pcEncCfg->getUseHADME() );
      Distortion uiMECost = uiMEError + m_pcRdCost->getCost( uiMEBits );
#endif
      // save ME result.
      uiMEInterDir = pcCU->getInterDir( uiPartAddr );
      TComDataCU::getMvField( pcCU, uiPartAddr, REF_PIC_LIST_0, cMEMvField[0] );
      TComDataCU::getMvField( pcCU, uiPartAddr, REF_PIC_LIST_1, cMEMvField[1] );

      // find Merge result
      Distortion uiMRGCost = std::numeric_limits<Distortion>::max();

      xMergeEstimation( pcCU, pcOrgYuv, iPartIdx, uiMRGInterDir, cMRGMvField, uiMRGIndex, uiMRGCost, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);

      if ( uiMRGCost < uiMECost )
      {
        // set Merge result
        pcCU->setMergeFlagSubParts ( true,          uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->setMergeIndexSubParts( uiMRGIndex,    uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->setInterDirSubParts  ( uiMRGInterDir, uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMRGMvField[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMRGMvField[1], ePartSize, uiPartAddr, 0, iPartIdx );

        pcCU->getCUMvField(REF_PIC_LIST_0)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUMvField(REF_PIC_LIST_1)->setAllMvd    ( cMvZero,            ePartSize, uiPartAddr, 0, iPartIdx );

        pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_0, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPIdxSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
        pcCU->setMVPNumSubParts( -1, REF_PIC_LIST_1, uiPartAddr, iPartIdx, pcCU->getDepth(uiPartAddr));
      }
      else
      {
        // set ME result
        pcCU->setMergeFlagSubParts( false,        uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->setInterDirSubParts ( uiMEInterDir, uiPartAddr, iPartIdx, pcCU->getDepth( uiPartAddr ) );
        pcCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMEMvField[0], ePartSize, uiPartAddr, 0, iPartIdx );
        pcCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMEMvField[1], ePartSize, uiPartAddr, 0, iPartIdx );
      }
    }

#if MCTS_ENC_CHECK
    if (m_pcEncCfg->getTMCTSSEITileConstraint() && (!checkTMctsMvp(pcCU, iPartIdx)))
    {
      pcCU->setTMctsMvpIsValid(false);
      return;
    }
#endif

    //  MC
    motionCompensation ( pcCU, pcPredYuv, REF_PIC_LIST_X, iPartIdx );

  } //  end of for ( Int iPartIdx = 0; iPartIdx < iNumPart; iPartIdx++ )

  setWpScalingDistParam( pcCU, -1, REF_PIC_LIST_X );

  return;
}


Void TEncSearch::ConstrainedAMVP(TComDataCU* pcCU, TComMv& cMvPred)
{
    Int offset_x = pcCU->getCUPelX() + (cMvPred.getHor() >> 2);
    Int offset_y = pcCU->getCUPelY() + (cMvPred.getVer() >> 2);

    //check left/right bound
    if (offset_x < g_cvi_enc_setting.mv_x_lbnd)
      cMvPred.setHor((g_cvi_enc_setting.mv_x_lbnd - pcCU->getCUPelX()) << 2);
    else if (offset_x > g_cvi_enc_setting.mv_x_ubnd)
      cMvPred.setHor((g_cvi_enc_setting.mv_x_ubnd - pcCU->getCUPelX()) << 2);

    //check top/bot bound
    if (offset_y < g_cvi_enc_setting.mv_y_lbnd)
      cMvPred.setVer((g_cvi_enc_setting.mv_y_lbnd - pcCU->getCUPelY()) << 2);
    else if (offset_y > g_cvi_enc_setting.mv_y_ubnd)
      cMvPred.setVer((g_cvi_enc_setting.mv_y_ubnd - pcCU->getCUPelY()) << 2);
}

Void TEncSearch::xCVIGetImeMVP( TComDataCU* pcCU, TComYuv* pcOrgYuv, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv* cBestIMEMv, AMVPInfo& AMVPInfo_cvi)
{
  Int        iRoiWidth, iRoiHeight;
  Int        i;
  UInt       uiPartAddr = 0;
  pcCU->getPartIndexAndSize( uiPartIdx, uiPartAddr, iRoiWidth, iRoiHeight );

  Distortion uiBestCost = std::numeric_limits<Distortion>::max();
  unsigned int ime_mvp_list_idx = 0;
  int candidate_num = 2;
  if (AMVPInfo_cvi.m_acMvCand[0].getHor() == AMVPInfo_cvi.m_acMvCand[1].getHor()
    && AMVPInfo_cvi.m_acMvCand[0].getVer() == AMVPInfo_cvi.m_acMvCand[1].getVer()
    && g_cvi_var.ime_mvp_list_refidx[0] == g_cvi_var.ime_mvp_list_refidx[1]) {
      candidate_num = 1;
  }

  if (candidate_num == 2)
    g_sigpool.cu_16x16_2_cand_count++;
  else
    g_sigpool.cu_16x16_1_cand_count++;

#ifdef SIG_CCU
  if (g_sigdump.ccu) {
    sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "# IME_AMVP\nime_amvp_cand_nb = 0x%x\n", candidate_num);
    for (Int i = 0; i < candidate_num; i++) {
      sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "ime_amvp_mvxy = 0x%x,0x%x,0x%x\n", i, AMVPInfo_cvi.m_acMvCand[i].getVer(), AMVPInfo_cvi.m_acMvCand[i].getHor());
      sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "ime_amvp_refidx = 0x%x,0x%x\n", i, g_cvi_var.ime_mvp_list_refidx[i]);
    }
  }
#endif

  if (g_cvi_enc_setting.enableConstrainedMVD && candidate_num == 2)
  {
    int abs_hor = abs(AMVPInfo_cvi.m_acMvCand[0].getHor() - AMVPInfo_cvi.m_acMvCand[1].getHor());
    int abs_ver = abs(AMVPInfo_cvi.m_acMvCand[0].getVer() - AMVPInfo_cvi.m_acMvCand[1].getVer());

    int max_abs_hor = g_cvi_enc_setting.mvd_thr_x * 4;
    int max_abs_ver = g_cvi_enc_setting.mvd_thr_y * 4;

    if ((abs_hor < max_abs_hor) && (abs_ver < max_abs_ver)) {
        candidate_num = 1;
        AMVPInfo_cvi.m_acMvCand[0].setHor((AMVPInfo_cvi.m_acMvCand[0].getHor() + AMVPInfo_cvi.m_acMvCand[1].getHor())>>3<<2);
        AMVPInfo_cvi.m_acMvCand[0].setVer((AMVPInfo_cvi.m_acMvCand[0].getVer() + AMVPInfo_cvi.m_acMvCand[1].getVer())>>3<<2);
    }

    g_sigpool.abs_hor += abs_hor;
    g_sigpool.abs_ver += abs_ver;
  }

#ifdef CVI_ENC_SETTING
  if (g_cvi_enc_setting.enableConstrainedMV)
  {
    for (Int i = 0; i < candidate_num; i++)
    {
#if 0
        //Constrain mv_x mv_y range relative to current CU
        Int mv_x = (AMVPInfo_cvi.m_acMvCand[i].getHor() >> 2) + pcCU->getCUPelX();
        Int mv_y = (AMVPInfo_cvi.m_acMvCand[i].getVer() >> 2) + pcCU->getCUPelY();

        Int Constrained_mv_x = min(g_cvi_enc_setting.mv_x_ubnd, max(mv_x, g_cvi_enc_setting.mv_x_lbnd));
        Int Constrained_mv_y = min(g_cvi_enc_setting.mv_y_ubnd, max(mv_y, g_cvi_enc_setting.mv_y_lbnd));
        AMVPInfo_cvi.m_acMvCand[i].set((Constrained_mv_x - pcCU->getCUPelX()) << 2, (Constrained_mv_y - pcCU->getCUPelY()) << 2);
#endif
        ConstrainedAMVP(pcCU, AMVPInfo_cvi.m_acMvCand[i]);
    }
  }
  if (candidate_num == 2 && AMVPInfo_cvi.m_acMvCand[0].getHor() == AMVPInfo_cvi.m_acMvCand[1].getHor()
    && AMVPInfo_cvi.m_acMvCand[0].getVer() == AMVPInfo_cvi.m_acMvCand[1].getVer()
    && g_cvi_var.ime_mvp_list_refidx[0] == g_cvi_var.ime_mvp_list_refidx[1]) {
      candidate_num = 1;
  }
#ifdef SIG_CCU
  if (g_sigdump.ccu) {
    sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "cons_ime_amvp_cand_nb = 0x%x\n", candidate_num);
    for (Int i = 0; i < candidate_num; i++) {
      sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "cons_ime_amvp_mvxy = 0x%x,0x%x,0x%x\n", i, AMVPInfo_cvi.m_acMvCand[i].getVer(), AMVPInfo_cvi.m_acMvCand[i].getHor());
      sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "cons_ime_amvp_refidx = 0x%x,0x%x\n", i, g_cvi_var.ime_mvp_list_refidx[i]);
    }
  }
#endif
#endif

#ifdef SIG_IME
  if (g_sigdump.ime) {
    sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "BLK16  #%x\n", g_sigpool.ime_16_cnt);
    sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "candidate_num = %x\n", candidate_num);
    sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "blk_cur_x = %x\n", g_sigpool.cu_idx_x);
    sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "blk_cur_y = %x\n", g_sigpool.cu_idx_y);
    sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "sqrt_lambda = %x\n", m_pcRdCost->getFixpointSADSqrtLambda());
  }
#endif

  if (candidate_num == 1) {
    i = 0;
    *cBestIMEMv   = AMVPInfo_cvi.m_acMvCand[i];
    ime_mvp_list_idx = i;
#ifdef SIG_IME
    if (g_sigdump.ime) {
      sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "candidate #%x\n", i);
      sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "blk_cand_idx = %x\n", i);
      sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "blk_refidx = %x\n", g_cvi_var.ime_mvp_list_refidx[i]);
      sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "blk_imvx = %x\n", AMVPInfo_cvi.m_acMvCand[i].getHor());
      sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "blk_imvy = %x\n", AMVPInfo_cvi.m_acMvCand[i].getVer());
    }
#endif
  } else {
    g_sigpool.cu_16x16_2_cand_count_mod++;
    //-- Check Minimum Cost.
    for ( i = 0 ; i < candidate_num; i++)
    {
#ifdef SIG_IME
      if (g_sigdump.ime) {
        sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "candidate #%x\n", i);
        sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "blk_cand_idx = %x\n", i);
        sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "blk_refidx = %x\n", g_cvi_var.ime_mvp_list_refidx[i]);
        sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "blk_imvx = %x\n", AMVPInfo_cvi.m_acMvCand[i].getHor());
        sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "blk_imvy = %x\n", AMVPInfo_cvi.m_acMvCand[i].getVer());
      }
#endif
      Distortion uiTmpCost, uiSadCost;
      uiSadCost = xGetIMEAMVPSadCost( pcCU, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, AMVPInfo_cvi.m_acMvCand[i],
        i, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
      uiTmpCost = (UInt) m_pcRdCost->calcRdCost( m_auiMVPIdxCost[i][AMVP_MAX_NUM_CANDS], uiSadCost, DF_SAD );
#ifdef SIG_IME
      if (g_sigdump.ime) {
        sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "sad = %x\n", uiSadCost);
        sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "cost = %x\n", uiTmpCost);
      }
#endif

      if ( uiBestCost > uiTmpCost )
      {
        uiBestCost = uiTmpCost;
        *cBestIMEMv   = AMVPInfo_cvi.m_acMvCand[i];
        ime_mvp_list_idx = i;
      }
    }
  }
#ifdef SIG_IME_MVP
  if (g_sigdump.ime_mvp) {
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "BLK16  #%x\n", g_sigpool.ime_16_cnt);
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "(x, y) = (%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "ime_mvp = %x %x %x\n",
      g_cvi_var.ime_mvp_list_refidx[ime_mvp_list_idx], cBestIMEMv->getVer(), cBestIMEMv->getHor());
    sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "ime_mvp_cand = %x\n", candidate_num);
  }
#endif
#ifdef SIG_IME
  if (g_sigdump.ime) {
    sigdump_output_fprint(&g_sigpool.ime_dat_out_ctx, "BLK16  #%x\nMVP_info\n", g_sigpool.ime_16_cnt);
    sigdump_output_fprint(&g_sigpool.ime_dat_out_ctx, "best_sad %x\n", uiBestCost);
    sigdump_output_fprint(&g_sigpool.ime_dat_out_ctx, "best_mvp_x %x\n", cBestIMEMv->getHor());
    sigdump_output_fprint(&g_sigpool.ime_dat_out_ctx, "best_mvp_y %x\n", cBestIMEMv->getVer());
  }
  g_sigpool.ime_16_cnt++;
#endif
}
// AMVP
Void TEncSearch::xEstimateMvPredAMVP( TComDataCU* pcCU, TComYuv* pcOrgYuv, UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, TComMv& rcMvPred, Bool bFilled, Distortion* puiDistBiP, TComMv* cBestIMEMv)
{
  AMVPInfo*  pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();

  TComMv     cBestMv;
  Int        iBestIdx   = 0;
  TComMv     cZeroMv;
  TComMv     cMvPred;
  Distortion uiBestCost = std::numeric_limits<Distortion>::max();
  UInt       uiPartAddr = 0;
  Int        iRoiWidth, iRoiHeight;
  Int        i;
  Int        minMVPCand;
  Int        maxMVPCand;
#ifdef CVI_STORE_IMV
  AMVPInfo  AMVPInfo_cvi;
  AMVPInfo_cvi.iN = 0;
#endif
  pcCU->getPartIndexAndSize( uiPartIdx, uiPartAddr, iRoiWidth, iRoiHeight );
  // Fill the MV Candidates
  if (!bFilled)
  {
    pcCU->fillMvpCand( uiPartIdx, uiPartAddr, eRefPicList, iRefIdx, pcAMVPInfo );
#ifdef CVI_STORE_IMV
    if (isEnableUseIMEMV())
    {
      pcCU->fillMvpCand_cvi( uiPartIdx, uiPartAddr, eRefPicList, iRefIdx, &AMVPInfo_cvi );
    }
#endif
  }
  // initialize Mvp index & Mvp
#if MCTS_ENC_CHECK
  if (m_pcEncCfg->getTMCTSSEITileConstraint() && pcCU->isLastColumnCTUInTile() && (pcAMVPInfo->numSpatialMVPCandidates < pcAMVPInfo->iN))
  {
    iBestIdx    = (pcAMVPInfo->numSpatialMVPCandidates == 0) ? 1 : 0;
    cBestMv     = pcAMVPInfo->m_acMvCand[(pcAMVPInfo->numSpatialMVPCandidates == 0) ? 1 : 0];
    minMVPCand  = (pcAMVPInfo->numSpatialMVPCandidates == 0) ? 1 : 0;
    maxMVPCand  = (pcAMVPInfo->numSpatialMVPCandidates == 0) ? pcAMVPInfo->iN : 1;
  }
  else
  {
    iBestIdx = 0;
    cBestMv  = pcAMVPInfo->m_acMvCand[0];
    minMVPCand  = 0;
    maxMVPCand  = pcAMVPInfo->iN;
  }
#else
  iBestIdx = 0;
  cBestMv  = pcAMVPInfo->m_acMvCand[0];
  minMVPCand  = 0;
  maxMVPCand  = pcAMVPInfo->iN;
#endif
  if (pcAMVPInfo->iN <= 1)
  {
    rcMvPred = cBestMv;

    pcCU->setMVPIdxSubParts( iBestIdx, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
    pcCU->setMVPNumSubParts( pcAMVPInfo->iN, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));

    if(pcCU->getSlice()->getMvdL1ZeroFlag() && eRefPicList==REF_PIC_LIST_1)
    {
      (*puiDistBiP) = xGetTemplateCost( pcCU, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, rcMvPred, 0, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
    }
    return;
  }

  if (bFilled)
  {
    assert(pcCU->getMVPIdx(eRefPicList,uiPartAddr) >= 0);
    rcMvPred = pcAMVPInfo->m_acMvCand[pcCU->getMVPIdx(eRefPicList,uiPartAddr)];
    return;
  }

  m_cYuvPredTemp.clear();
  //-- Check Minimum Cost.
  for ( i = minMVPCand ; i < maxMVPCand; i++)
  {
    Distortion uiTmpCost;
    uiTmpCost = xGetTemplateCost( pcCU, uiPartAddr, pcOrgYuv, &m_cYuvPredTemp, pcAMVPInfo->m_acMvCand[i], i, AMVP_MAX_NUM_CANDS, eRefPicList, iRefIdx, iRoiWidth, iRoiHeight);
    if ( uiBestCost > uiTmpCost )
    {
      uiBestCost = uiTmpCost;
      cBestMv   = pcAMVPInfo->m_acMvCand[i];
      iBestIdx  = i;
      (*puiDistBiP) = uiTmpCost;
    }
  }
  m_cYuvPredTemp.clear();

#ifdef CVI_STORE_IMV
  if (isEnableUseIMEMV() && ((iRoiWidth == 16 && iRoiHeight == 16) ||
    is_use_8x8_ime_mvp(pcCU->getCUPelX(), pcCU->getCUPelY(), pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples(), pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples())))
  {
    xCVIGetImeMVP(pcCU, pcOrgYuv, uiPartIdx, eRefPicList, iRefIdx, cBestIMEMv, AMVPInfo_cvi);
    m_cYuvPredTemp.clear();
  }
#endif

  // Setting Best MVP
  rcMvPred = cBestMv;
  pcCU->setMVPIdxSubParts( iBestIdx, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
  pcCU->setMVPNumSubParts( pcAMVPInfo->iN, eRefPicList, uiPartAddr, uiPartIdx, pcCU->getDepth(uiPartAddr));
  return;
}

UInt TEncSearch::xGetMvpIdxBits(Int iIdx, Int iNum)
{
  assert(iIdx >= 0 && iNum >= 0 && iIdx < iNum);

  if (iNum == 1)
  {
    return 0;
  }

  UInt uiLength = 1;
  Int iTemp = iIdx;
  if ( iTemp == 0 )
  {
    return uiLength;
  }

  Bool bCodeLast = ( iNum-1 > iTemp );

  uiLength += (iTemp-1);

  if( bCodeLast )
  {
    uiLength++;
  }

  return uiLength;
}

Void TEncSearch::xGetBlkBits( PartSize eCUMode, Bool bPSlice, Int iPartIdx, UInt uiLastMode, UInt uiBlkBit[3])
{
  if ( eCUMode == SIZE_2Nx2N )
  {
    uiBlkBit[0] = (! bPSlice) ? 3 : 1;
    uiBlkBit[1] = 3;
    uiBlkBit[2] = 5;
  }
  else if ( (eCUMode == SIZE_2NxN || eCUMode == SIZE_2NxnU) || eCUMode == SIZE_2NxnD )
  {
    UInt aauiMbBits[2][3][3] = { { {0,0,3}, {0,0,0}, {0,0,0} } , { {5,7,7}, {7,5,7}, {9-3,9-3,9-3} } };
    if ( bPSlice )
    {
      uiBlkBit[0] = 3;
      uiBlkBit[1] = 0;
      uiBlkBit[2] = 0;
    }
    else
    {
      ::memcpy( uiBlkBit, aauiMbBits[iPartIdx][uiLastMode], 3*sizeof(UInt) );
    }
  }
  else if ( (eCUMode == SIZE_Nx2N || eCUMode == SIZE_nLx2N) || eCUMode == SIZE_nRx2N )
  {
    UInt aauiMbBits[2][3][3] = { { {0,2,3}, {0,0,0}, {0,0,0} } , { {5,7,7}, {7-2,7-2,9-2}, {9-3,9-3,9-3} } };
    if ( bPSlice )
    {
      uiBlkBit[0] = 3;
      uiBlkBit[1] = 0;
      uiBlkBit[2] = 0;
    }
    else
    {
      ::memcpy( uiBlkBit, aauiMbBits[iPartIdx][uiLastMode], 3*sizeof(UInt) );
    }
  }
  else if ( eCUMode == SIZE_NxN )
  {
    uiBlkBit[0] = (! bPSlice) ? 3 : 1;
    uiBlkBit[1] = 3;
    uiBlkBit[2] = 5;
  }
  else
  {
    printf("Wrong!\n");
    assert( 0 );
  }
}

Void TEncSearch::xCopyAMVPInfo (AMVPInfo* pSrc, AMVPInfo* pDst)
{
  pDst->iN = pSrc->iN;
  for (Int i = 0; i < pSrc->iN; i++)
  {
    pDst->m_acMvCand[i] = pSrc->m_acMvCand[i];
  }
}

Void TEncSearch::xCheckBestMVP ( TComDataCU* pcCU, RefPicList eRefPicList, TComMv cMv, TComMv& rcMvPred, Int& riMVPIdx, UInt& ruiBits, Distortion& ruiCost )
{
  AMVPInfo* pcAMVPInfo = pcCU->getCUMvField(eRefPicList)->getAMVPInfo();

  assert(pcAMVPInfo->m_acMvCand[riMVPIdx] == rcMvPred);

  if (pcAMVPInfo->iN < 2)
  {
    return;
  }

  m_pcRdCost->selectMotionLambda( true, 0, pcCU->getCUTransquantBypass(0) );
  m_pcRdCost->setCostScale ( 0    );

  Int iBestMVPIdx = riMVPIdx;

  m_pcRdCost->setPredictor( rcMvPred );
  Int iOrgMvBits  = m_pcRdCost->getBitsOfVectorWithPredictor(cMv.getHor(), cMv.getVer());
  iOrgMvBits += m_auiMVPIdxCost[riMVPIdx][AMVP_MAX_NUM_CANDS];
  Int iBestMvBits = iOrgMvBits;

#if MCTS_ENC_CHECK
  Int minMVPCand = 0;
  Int maxMVPCand = pcAMVPInfo->iN;

  if (m_pcEncCfg->getTMCTSSEITileConstraint() && pcCU->isLastColumnCTUInTile())
  {
    minMVPCand = (pcAMVPInfo->numSpatialMVPCandidates == 0) ? 1 : 0;
    maxMVPCand = (pcAMVPInfo->numSpatialMVPCandidates == 0) ? pcAMVPInfo->iN : 1;
  }
  for (Int iMVPIdx = minMVPCand; iMVPIdx < maxMVPCand; iMVPIdx++)
#else
  for (Int iMVPIdx = 0; iMVPIdx < pcAMVPInfo->iN; iMVPIdx++)
#endif
  {

    if (iMVPIdx == riMVPIdx)
    {
      continue;
    }

    m_pcRdCost->setPredictor( pcAMVPInfo->m_acMvCand[iMVPIdx] );

    Int iMvBits = m_pcRdCost->getBitsOfVectorWithPredictor(cMv.getHor(), cMv.getVer());
    iMvBits += m_auiMVPIdxCost[iMVPIdx][AMVP_MAX_NUM_CANDS];

#ifdef CVI_STORE_IMV
    if ((iMvBits < iBestMvBits) || (isEnableUseIMEMV() && (iMvBits == iBestMvBits) && (iBestMVPIdx == 1) ))
#else
    if (iMvBits < iBestMvBits)
#endif
    {
      iBestMvBits = iMvBits;
      iBestMVPIdx = iMVPIdx;
    }
  }

  if (iBestMVPIdx != riMVPIdx)  //if changed
  {
    rcMvPred = pcAMVPInfo->m_acMvCand[iBestMVPIdx];

    riMVPIdx = iBestMVPIdx;
    UInt uiOrgBits = ruiBits;
    ruiBits = uiOrgBits - iOrgMvBits + iBestMvBits;
    ruiCost = (ruiCost - m_pcRdCost->getCost( uiOrgBits ))  + m_pcRdCost->getCost( ruiBits );
  }
}


Distortion TEncSearch::xGetTemplateCost( TComDataCU* pcCU,
                                         UInt        uiPartAddr,
                                         TComYuv*    pcOrgYuv,
                                         TComYuv*    pcTemplateCand,
                                         TComMv      cMvCand,
                                         Int         iMVPIdx,
                                         Int         iMVPNum,
                                         RefPicList  eRefPicList,
                                         Int         iRefIdx,
                                         Int         iSizeX,
                                         Int         iSizeY)
{
  Distortion uiCost = std::numeric_limits<Distortion>::max();

  TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec();

  pcCU->clipMv( cMvCand );

  // prediction pattern
  if ( pcCU->getSlice()->testWeightPred() && pcCU->getSlice()->getSliceType()==P_SLICE )
  {
    xPredInterBlk( COMPONENT_Y, pcCU, pcPicYuvRef, uiPartAddr, &cMvCand, iSizeX, iSizeY, pcTemplateCand, true, pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) );
  }
  else
  {
    xPredInterBlk( COMPONENT_Y, pcCU, pcPicYuvRef, uiPartAddr, &cMvCand, iSizeX, iSizeY, pcTemplateCand, false, pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) );
  }

  if ( pcCU->getSlice()->testWeightPred() && pcCU->getSlice()->getSliceType()==P_SLICE )
  {
    xWeightedPredictionUni( pcCU, pcTemplateCand, uiPartAddr, iSizeX, iSizeY, eRefPicList, pcTemplateCand, iRefIdx );
  }

  // calc distortion
  uiCost = m_pcRdCost->getDistPart( pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA), pcTemplateCand->getAddr(COMPONENT_Y, uiPartAddr), pcTemplateCand->getStride(COMPONENT_Y), pcOrgYuv->getAddr(COMPONENT_Y, uiPartAddr), pcOrgYuv->getStride(COMPONENT_Y), iSizeX, iSizeY, COMPONENT_Y, DF_SAD );
  uiCost = (UInt) m_pcRdCost->calcRdCost( m_auiMVPIdxCost[iMVPIdx][iMVPNum], uiCost, DF_SAD );
  return uiCost;
}

Distortion TEncSearch::xGetIMEAMVPSadCost( TComDataCU* pcCU,
                                           UInt        uiPartAddr,
                                           TComYuv*    pcOrgYuv,
                                           TComYuv*    pcTemplateCand,
                                           TComMv      cMvCand,
                                           Int         iMVPIdx,
                                           Int         iMVPNum,
                                           RefPicList  eRefPicList,
                                           Int         iRefIdx,
                                           Int         iSizeX,
                                           Int         iSizeY)
{
  Distortion uiCost = std::numeric_limits<Distortion>::max();

  TComPicYuv* pcPicYuvRef = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPicYuvRec();
  const Int predSizeX = 16, predSizeY = 16;

  pcCU->clipMv( cMvCand );

  // prediction pattern
  if ( pcCU->getSlice()->testWeightPred() && pcCU->getSlice()->getSliceType()==P_SLICE )
  {
    xPredInterBlk( COMPONENT_Y, pcCU, pcPicYuvRef, uiPartAddr, &cMvCand, predSizeX, predSizeY, pcTemplateCand, true, pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) );
  }
  else
  {
    xPredInterBlk( COMPONENT_Y, pcCU, pcPicYuvRef, uiPartAddr, &cMvCand, predSizeX, predSizeY, pcTemplateCand, false, pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) );
  }

  if ( pcCU->getSlice()->testWeightPred() && pcCU->getSlice()->getSliceType()==P_SLICE )
  {
    xWeightedPredictionUni( pcCU, pcTemplateCand, uiPartAddr, predSizeX, predSizeY, eRefPicList, pcTemplateCand, iRefIdx );
  }

  DistParam cDtParam;
  m_pcRdCost->setDistParam( predSizeX, predSizeY, DF_SAD, cDtParam );
  //Curr buffer
  Pel *tmp_buffer = NULL;
  if (iSizeX != 16 || iSizeY != 16) {
    tmp_buffer = new Pel[predSizeX * predSizeY];
    memset(tmp_buffer, 0x0, sizeof(Pel) * predSizeX * predSizeY);

    TComPicYuv *pcPicYuvOrg = pcCU->getSlice()->getPic()->getPicYuvOrg();
    Pel *Org = pcPicYuvOrg->getAddr(COMPONENT_Y, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + uiPartAddr);
    int copy_x = ((g_sigpool.cu_idx_x + 16) <= g_sigpool.width) ? 16 : 8;
    int copy_y = ((g_sigpool.cu_idx_y + 16) <= g_sigpool.height) ? 16 : 8;
    for (int i = 0; i < copy_y; i++) {
      for (int j = 0; j < copy_x; j++) {
        tmp_buffer[i * predSizeX + j] = *(Org + pcPicYuvOrg->getStride(COMPONENT_Y) * i + j);
      }
    }

    //padding right
    if (copy_x == 8)
    {
      for (int i = 0; i < copy_y; i++)
      {
        for (int j = 8; j < 16; j++)
        {
          tmp_buffer[i * predSizeX + j] = tmp_buffer[i * predSizeX + j - 1];
        }
      }
    }
    //padding below
    if (copy_y == 8)
    {
      for (int i = 8; i < 16; i++)
      {
        for (int j = 0; j < copy_x; j++)
        {
          tmp_buffer[i * predSizeX + j] = tmp_buffer[(i - 1) * predSizeX + j];
        }
      }
    }

    if (copy_x == 8 && copy_y == 8)
    {
      for (int i = 8; i < 16; i++)
      {
        for (int j = 8; j < 16; j++)
        {
          tmp_buffer[i * predSizeX + j] = tmp_buffer[7 * predSizeX + 7];
        }
      }
    }
    cDtParam.pCur = tmp_buffer;
    cDtParam.iStrideCur = predSizeX;
  } else {
    cDtParam.pCur       = pcOrgYuv->getAddr(COMPONENT_Y, uiPartAddr);
    cDtParam.iStrideCur = pcOrgYuv->getStride(COMPONENT_Y);
  }
  //pred buffer
  cDtParam.pOrg       = pcTemplateCand->getAddr(COMPONENT_Y, uiPartAddr);
  cDtParam.iStrideOrg = pcTemplateCand->getStride(COMPONENT_Y);

  cDtParam.iSubShift  = 1;
  cDtParam.bApplyWeight = false;
  cDtParam.compIdx      = MAX_NUM_COMPONENT; // just for assert: to be sure it was set before use
  cDtParam.bitDepth     = pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA);
#ifndef CVI_FAST_16X4_AMVP_SAD
  //uiCost = m_pcRdCost->xGetSAD_SHIFT(&cDtParam, 2);
  uiCost = m_pcRdCost->xGetBiSAD(&cDtParam, 0);
#else
  Distortion uiSum = 0;
  const int y_cnt = 8;
  const int y_sub_cnt = 4;
  const int shift = 2;
  for (int i = 0; i < predSizeY; i+=y_cnt) {
    for (int i_sub = 0; i_sub < y_sub_cnt; i_sub++) {
      for (int j = 0; j < predSizeX; j+=1) {
        uiSum += abs( (*(cDtParam.pOrg + j + (i + i_sub) * cDtParam.iStrideOrg) >> shift) - (*(cDtParam.pCur + j + (i + i_sub) * cDtParam.iStrideCur) >> shift) );
      }
    }
  }
  uiCost = (uiSum * (y_cnt / y_sub_cnt)) << shift;
#endif
#ifdef SIG_IME
  if (g_sigdump.ime) {
    for (int i = 0; i < predSizeY; i++) {
      for (int j = 0; j < predSizeX; j++) {
        sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "%02x", *(cDtParam.pOrg + j + i * cDtParam.iStrideOrg));
      }
      sigdump_output_fprint(&g_sigpool.ime_cmd_in_ctx, "\n");
    }
  }
#endif
#ifdef CVI_CACHE_MODEL
  if (isEnableCACHE_MODEL())
  {
    int index_x = pcCU->getCUPelX() + (cMvCand.getHor() >> 2);
    int index_y = pcCU->getCUPelY() + (cMvCand.getVer() >> 2);
    miu_access(index_x, index_y, predSizeX, predSizeY, pcCU->getSlice()->getRefPic( eRefPicList, iRefIdx )->getPOC());
  }
#endif
  if (tmp_buffer)
    delete [] tmp_buffer;

  return uiCost;
}
#ifdef SIG_IME
static Void sigdump_ime_serach(TComDataCU* pcCU, Pel* piRefY, Int iRefStride, TComMv* cBestIMEMv, Int iRoiWidth, Int iRoiHeight)
{
  if (iRoiWidth == 16 || is_use_8x8_ime_mvp(g_sigpool.cu_idx_x, g_sigpool.cu_idx_y, g_sigpool.width, g_sigpool.height)) {
    for ( Int y = - cvi_sr[0]; y < (cvi_sr[1] + 16 - 1); y++ )
    {
      for ( Int x = - cvi_sr[2]; x < (cvi_sr[3] + 16 - 1); x++ )
      {
        TComMv      tmpMv((x << 2) + cBestIMEMv->getHor(), (y << 2) + cBestIMEMv->getVer());
        pcCU->clipMv( tmpMv );
        tmpMv.divideByPowerOf2(2);
        sigdump_output_bin(&g_sigpool.ime_dat_out_bin_ctx, (unsigned char *)(piRefY + tmpMv.getHor() + (tmpMv.getVer() * iRefStride)), 1);
        //sigdump_output_fprint(&g_sigpool.ime_dat_out_ctx, "%02x", *(piRefY + tmpMv.getHor() + (tmpMv.getVer() * iRefStride)));
      }
      //sigdump_output_fprint(&g_sigpool.ime_dat_out_ctx, "\n");
    }
  }
}
#endif

Void TEncSearch::xMotionEstimation( TComDataCU* pcCU, TComYuv* pcYuvOrg, Int iPartIdx, RefPicList eRefPicList, TComMv* pcMvPred, Int iRefIdxPred, TComMv& rcMv, UInt& ruiBits, Distortion& ruiCost, Bool bBi, TComMv* cBestIMEMv , TComMv* IME_list)
{
  UInt          uiPartAddr;
  Int           iRoiWidth;
  Int           iRoiHeight;

  TComMv        cMvHalf, cMvQter;
  TComMv        cMvSrchRngLT;
  TComMv        cMvSrchRngRB;

  TComYuv*      pcYuv = pcYuvOrg;

  assert(eRefPicList < MAX_NUM_REF_LIST_ADAPT_SR && iRefIdxPred<Int(MAX_IDX_ADAPT_SR));
  m_iSearchRange = m_aaiAdaptSR[eRefPicList][iRefIdxPred];

  Int           iSrchRng      = ( bBi ? m_bipredSearchRange : m_iSearchRange );
  TComPattern   cPattern;

  Double        fWeight       = 1.0;

  pcCU->getPartIndexAndSize( iPartIdx, uiPartAddr, iRoiWidth, iRoiHeight );

  if ( bBi ) // Bipredictive ME
  {
    TComYuv*  pcYuvOther = &m_acYuvPred[1-(Int)eRefPicList];
    pcYuv                = &m_cYuvPredTemp;

    pcYuvOrg->copyPartToPartYuv( pcYuv, uiPartAddr, iRoiWidth, iRoiHeight );

    pcYuv->removeHighFreq( pcYuvOther, uiPartAddr, iRoiWidth, iRoiHeight, pcCU->getSlice()->getSPS()->getBitDepths().recon, m_pcEncCfg->getClipForBiPredMeEnabled() );

    fWeight = 0.5;
  }
  m_cDistParam.bIsBiPred = bBi;

  //  Search key pattern initialization
#if MCTS_ENC_CHECK
  Int roiPosX, roiPosY;
  Int roiW, roiH;
  pcCU->getPartPosition(iPartIdx, roiPosX, roiPosY, roiW, roiH);
  assert(roiW == iRoiWidth);
  assert(roiH == iRoiHeight);
  cPattern.initPattern( pcYuv->getAddr(COMPONENT_Y, uiPartAddr),
                        iRoiWidth,
                        iRoiHeight,
                        pcYuv->getStride(COMPONENT_Y),
                        pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA),
                        roiPosX,
                        roiPosY);
  xInitTileBorders(pcCU, &cPattern);
#else
  cPattern.initPattern( pcYuv->getAddr  ( COMPONENT_Y, uiPartAddr ),
                        iRoiWidth,
                        iRoiHeight,
                        pcYuv->getStride(COMPONENT_Y),
                        pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_LUMA) );
#endif

  Pel*        piRefY      = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdxPred )->getPicYuvRec()->getAddr( COMPONENT_Y, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + uiPartAddr );
  Int         iRefStride  = pcCU->getSlice()->getRefPic( eRefPicList, iRefIdxPred )->getPicYuvRec()->getStride(COMPONENT_Y);

  TComMv      cMvPred = *pcMvPred;

#ifdef CVI_STORE_IMV
  if (isEnableUseIMEMV()) {
    cMvPred = *cBestIMEMv;
  }
#endif
  if ( bBi )
  {
#if MCTS_ENC_CHECK
    xSetSearchRange(pcCU, rcMv, iSrchRng, cMvSrchRngLT, cMvSrchRngRB, &cPattern);
#else
    xSetSearchRange(pcCU, rcMv, iSrchRng, cMvSrchRngLT, cMvSrchRngRB);
#endif
  }
  else
  {
#if MCTS_ENC_CHECK
    xSetSearchRange(pcCU, cMvPred, iSrchRng, cMvSrchRngLT, cMvSrchRngRB, &cPattern);
#else
    xSetSearchRange(pcCU, cMvPred, iSrchRng, cMvSrchRngLT, cMvSrchRngRB);
#endif
  }

  m_pcRdCost->selectMotionLambda( true, 0, pcCU->getCUTransquantBypass(uiPartAddr) );

  m_pcRdCost->setPredictor  ( *pcMvPred );
  m_pcRdCost->setCostScale  ( 2 );

  setWpScalingDistParam( pcCU, iRefIdxPred, eRefPicList );
  //  Do integer search
  if ( (m_motionEstimationSearchMethod==MESEARCH_FULL) || bBi )
  {
    TComMv      rcsubMv;

    if (isEnableUseIMEMV())
      m_pcRdCost->setPredictor  ( cMvPred );
#ifdef SIG_IME
    if (g_sigdump.ime) {
      sigdump_ime_serach(pcCU, piRefY, iRefStride, cBestIMEMv, iRoiWidth, iRoiHeight);
    }
#endif
    xPatternSearch      ( &cPattern, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost , rcsubMv, pcCU->getCUPelX(), pcCU->getCUPelY(), pcCU->getSlice()->getRefPOC( eRefPicList, iRefIdxPred));

    if (isEnableUseIMEMV())
    {
      store_frm_mv(iRoiWidth == 16 && iRoiHeight == 16, pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples(), pcCU->getSlice()->getSPS()->getPicHeightInLumaSamples(),
        pcCU->getCtuRsAddr(), pcCU->getSlice()->getSPS()->getMaxCUHeight(), pcCU->getCUPelX(),pcCU->getCUPelY(),
        rcMv.getHor() << 2, rcMv.getVer() << 2, pcCU->getSlice()->getRefPOC( eRefPicList, iRefIdxPred), iRefIdxPred, eRefPicList);
#ifdef CVI_FAST_B_FRAME
      (*IME_list).set(rcMv.getHor() << 2, rcMv.getVer() << 2);
#endif
#ifdef SIG_IME_MVP
      if (g_sigdump.ime_mvp && iRoiWidth == 16 && iRoiHeight == 16) {
        sigdump_output_fprint(&g_sigpool.ime_mvp_ctx, "ime_wb_mv = %x %x %x\n", iRefIdxPred, rcMv.getVer() << 2, rcMv.getHor() << 2);
      }
#endif
#ifdef SIG_CCU
      if (g_sigdump.ccu && (iRoiWidth == 16 || is_8x8_store_frm_mv(g_sigpool.width, g_sigpool.height, g_sigpool.cu_idx_x, g_sigpool.cu_idx_y)))
      {
        sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "# IME\nime_mvxy = 0x%x,0x%x\nime_refidx = 0x%x\n", rcMv.getVer() << 2, rcMv.getHor() << 2, iRefIdxPred);
        sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "reg_ime_2cand_org_cnt = %d\n", g_sigpool.cu_16x16_2_cand_count);
        sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "reg_ime_2cand_cnt = %d\n", g_sigpool.cu_16x16_2_cand_count_mod);
        sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "reg_ime_2cand_abs_mvdx = %d\n", g_sigpool.abs_hor);
        sigdump_output_fprint(&g_sigpool.ccu_ime_ctx, "reg_ime_2cand_abs_mvdy = %d\n", g_sigpool.abs_ver);
      }
#endif
    }
  }
  else
  {
    rcMv = *pcMvPred;
    const TComMv *pIntegerMv2Nx2NPred=0;
    if (pcCU->getPartitionSize(0) != SIZE_2Nx2N || pcCU->getDepth(0) != 0)
    {
      pIntegerMv2Nx2NPred = &(m_integerMv2Nx2N[eRefPicList][iRefIdxPred]);
    }
    xPatternSearchFast  ( pcCU, &cPattern, piRefY, iRefStride, &cMvSrchRngLT, &cMvSrchRngRB, rcMv, ruiCost, pIntegerMv2Nx2NPred );
    if (pcCU->getPartitionSize(0) == SIZE_2Nx2N)
    {
      m_integerMv2Nx2N[eRefPicList][iRefIdxPred] = rcMv;
    }
  }
#ifdef CVI_CACHE_MODEL
        if (isEnableCACHE_MODEL_FME())
        {
          int index_x = pcCU->getCUPelX() + rcMv.getHor();
          int index_y = pcCU->getCUPelY() + rcMv.getVer();
          //twice for half & quart
          miu_access_fme(index_x, index_y, iRoiWidth, iRoiHeight, pcCU->getSlice()->getRefPOC( eRefPicList, iRefIdxPred));
          miu_access_fme(index_x, index_y, iRoiWidth, iRoiHeight, pcCU->getSlice()->getRefPOC( eRefPicList, iRefIdxPred));
        }
#endif

  m_pcRdCost->selectMotionLambda( true, 0, pcCU->getCUTransquantBypass(uiPartAddr) );
  m_pcRdCost->setCostScale ( 1 );

  const Bool bIsLosslessCoded = pcCU->getCUTransquantBypass(uiPartAddr) != 0;
  xPatternSearchFracDIF( bIsLosslessCoded, &cPattern, piRefY, iRefStride, &rcMv, cMvHalf, cMvQter, ruiCost );

  m_pcRdCost->setCostScale( 0 );
  rcMv <<= 2;
  rcMv += (cMvHalf <<= 1);
  rcMv +=  cMvQter;

  UInt uiMvBits = m_pcRdCost->getBitsOfVectorWithPredictor( rcMv.getHor(), rcMv.getVer() );

  ruiBits      += uiMvBits;
  ruiCost       = (Distortion)( floor( fWeight * ( (Double)ruiCost - (Double)m_pcRdCost->getCost( uiMvBits ) ) ) + (Double)m_pcRdCost->getCost( ruiBits ) );
}

#if MCTS_ENC_CHECK
Void TEncSearch::xInitTileBorders(const TComDataCU* const pcCU, TComPattern* pcPatternKey)
{
  if (m_pcEncCfg->getTMCTSSEITileConstraint())
  {
    UInt  tileXPosInCtus = 0;
    UInt  tileYPosInCtus = 0;
    UInt  tileWidthtInCtus = 0;
    UInt  tileHeightInCtus = 0;

    getTilePosition(pcCU, tileXPosInCtus, tileYPosInCtus, tileWidthtInCtus, tileHeightInCtus);

    const Int  ctuLength = pcCU->getPic()->getPicSym()->getSPS().getMaxCUWidth();

    // tile position in full pels
    const Int tileLeftTopPelPosX = ctuLength * tileXPosInCtus;
    const Int tileLeftTopPelPosY = ctuLength * tileYPosInCtus;
    const Int tileRightBottomPelPosX = ((tileWidthtInCtus + tileXPosInCtus) * ctuLength) - 1;
    const Int tileRightBottomPelPosY = ((tileHeightInCtus + tileYPosInCtus) * ctuLength) - 1;

    pcPatternKey->setTileBorders (tileLeftTopPelPosX,tileLeftTopPelPosY,tileRightBottomPelPosX,tileRightBottomPelPosY);
  }
}
#endif

Void TEncSearch::xSetSearchRange ( const TComDataCU* const pcCU, const TComMv& cMvPred, const Int iSrchRng,
#if MCTS_ENC_CHECK
                                   TComMv& rcMvSrchRngLT, TComMv& rcMvSrchRngRB, const TComPattern* const pcPatternKey )
#else
                                   TComMv& rcMvSrchRngLT, TComMv& rcMvSrchRngRB )
#endif
{
  Int  iMvShift = 2;
  TComMv cTmpMvPred = cMvPred;

#if MCTS_ENC_CHECK
  if (m_pcEncCfg->getTMCTSSEITileConstraint())
  {
    const Int lRangeXLeft = max(cTmpMvPred.getHor() - (iSrchRng << iMvShift), (pcPatternKey->getTileLeftTopPelPosX() - pcPatternKey->getROIYPosX()) << iMvShift);
    const Int lRangeYTop = max(cTmpMvPred.getVer() - (iSrchRng << iMvShift), (pcPatternKey->getTileLeftTopPelPosY() - pcPatternKey->getROIYPosY()) << iMvShift);
    const Int lRangeXRight = min(cTmpMvPred.getHor() + (iSrchRng << iMvShift), (pcPatternKey->getTileRightBottomPelPosX() - (pcPatternKey->getROIYPosX() + pcPatternKey->getROIYWidth())) << iMvShift);
    const Int lRangeYBottom = min(cTmpMvPred.getVer() + (iSrchRng << iMvShift), (pcPatternKey->getTileRightBottomPelPosY() - (pcPatternKey->getROIYPosY() + pcPatternKey->getROIYHeight())) << iMvShift);

    rcMvSrchRngLT.setHor(lRangeXLeft);
    rcMvSrchRngLT.setVer(lRangeYTop);

    rcMvSrchRngRB.setHor(lRangeXRight);
    rcMvSrchRngRB.setVer(lRangeYBottom);
  }
  else
  {
    rcMvSrchRngLT.setHor(cTmpMvPred.getHor() - (iSrchRng << iMvShift));
    rcMvSrchRngLT.setVer(cTmpMvPred.getVer() - (iSrchRng << iMvShift));

    rcMvSrchRngRB.setHor( cTmpMvPred.getHor() + (iSrchRng << iMvShift));
    rcMvSrchRngRB.setVer( cTmpMvPred.getVer() + (iSrchRng << iMvShift) );
  }
#else
#ifdef CVI_ALGO_CFG
  if (isEnableIME_SR())
  {
    Int margin = 32;
    rcMvSrchRngLT.setHor( cTmpMvPred.getHor() - (cvi_sr[0] << iMvShift) );
    rcMvSrchRngLT.setVer( cTmpMvPred.getVer() - (cvi_sr[2] << iMvShift) );

    rcMvSrchRngRB.setHor( cTmpMvPred.getHor() + (cvi_sr[1] << iMvShift));
    rcMvSrchRngRB.setVer( cTmpMvPred.getVer() + (cvi_sr[3] << iMvShift) );
    pcCU->clipMv( rcMvSrchRngLT, margin);
    pcCU->clipMv( rcMvSrchRngRB, margin);
  }
  else
#endif
  {
    pcCU->clipMv( cTmpMvPred );

    rcMvSrchRngLT.setHor( cTmpMvPred.getHor() - (iSrchRng << iMvShift) );
    rcMvSrchRngLT.setVer( cTmpMvPred.getVer() - (iSrchRng << iMvShift) );

    rcMvSrchRngRB.setHor( cTmpMvPred.getHor() + (iSrchRng << iMvShift));
    rcMvSrchRngRB.setVer( cTmpMvPred.getVer() + (iSrchRng << iMvShift) );

    pcCU->clipMv        ( rcMvSrchRngLT );
    pcCU->clipMv        ( rcMvSrchRngRB );
  }
#endif


#if ME_ENABLE_ROUNDING_OF_MVS
  rcMvSrchRngLT.divideByPowerOf2(iMvShift);
  rcMvSrchRngRB.divideByPowerOf2(iMvShift);
#else
  rcMvSrchRngLT >>= iMvShift;
  rcMvSrchRngRB >>= iMvShift;
#endif
}


Void TEncSearch::xPatternSearch( const TComPattern* const pcPatternKey,
                                 const Pel*               piRefY,
                                 const Int                iRefStride,
                                 const TComMv* const      pcMvSrchRngLT,
                                 const TComMv* const      pcMvSrchRngRB,
                                 TComMv&      rcMv,
                                 Distortion&  ruiSAD,
                                 TComMv&      rcsubMv,
                                 UInt         m_uiCUPelX,
                                 UInt         m_uiCUPelY,
                                 Int          refPOC)
{
  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();

  Distortion  uiSad;
  Distortion  uiSadBest = std::numeric_limits<Distortion>::max();
  Int         iBestX = 0;
  Int         iBestY = 0;

  //-- jclee for using the SAD function pointer
  m_pcRdCost->setDistParam( pcPatternKey, piRefY, iRefStride,  m_cDistParam );
  // fast encoder decision: use subsampled SAD for integer ME
  if ( m_pcEncCfg->getFastInterSearchMode()==FASTINTERSEARCH_MODE1 || m_pcEncCfg->getFastInterSearchMode()==FASTINTERSEARCH_MODE3 )
  {
    if ( m_cDistParam.iRows > 8 )
    {
      m_cDistParam.iSubShift = 1;
    }
  }

  piRefY += (iSrchRngVerTop * iRefStride);

  for ( Int y = iSrchRngVerTop; y < iSrchRngVerBottom; y++ )
  {
    for ( Int x = iSrchRngHorLeft; x < iSrchRngHorRight; x++ )
    {
#ifdef CVI_CACHE_MODEL
      if (isEnableCACHE_MODEL())
      {
        int index_x = m_uiCUPelX + x;
        int index_y = m_uiCUPelY + y;
        miu_access(index_x, index_y, m_cDistParam.iCols, m_cDistParam.iRows, refPOC);
      }
#endif
      //  find min. distortion position
      m_cDistParam.pCur = piRefY + x;
      setDistParamComp(COMPONENT_Y);
      m_cDistParam.bitDepth = pcPatternKey->getBitDepthY();
      uiSad = 0;
#if 0
      //row & col index jump 2
      m_cDistParam.iSubShift = 1;
      if (isEnableIME_LCSAD())
        uiSad = m_pcRdCost->xGetSAD_SHIFT_JUMP(&m_cDistParam, 2);
      else
        uiSad = m_pcRdCost->xGetSAD_SHIFT(&m_cDistParam, 2);
#else
      uiSad = m_pcRdCost->xGetBiSAD(&m_cDistParam, 0);
#endif
      //only support 16x16 & 8x8 now.
      assert(m_cDistParam.iCols == 16 || m_cDistParam.iCols == 8);
      // motion cost
      uiSad += m_pcRdCost->getCostOfVectorWithPredictor( x, y );

      if ( uiSad < uiSadBest )
      {
        uiSadBest = uiSad;
        iBestX    = x;
        iBestY    = y;
        m_cDistParam.m_maximumDistortionForEarlyExit = uiSad;
      }
    }
    piRefY += iRefStride;
  }

  rcMv.set( iBestX, iBestY );

  ruiSAD = uiSadBest - m_pcRdCost->getCostOfVectorWithPredictor( iBestX, iBestY );
#ifdef SIG_IME
  if (g_sigdump.ime) {
    if (m_cDistParam.iCols == 8)
      sigdump_output_fprint(&g_sigpool.ime_dat_out_ctx, "BLK%d  #%x\n", m_cDistParam.iCols, (g_sigpool.cu_idx_x % 16) / 8 + 2 * (g_sigpool.cu_idx_y % 16) / 8);
    sigdump_output_fprint(&g_sigpool.ime_dat_out_ctx, "best_mv_cost = %x\n", uiSadBest - ruiSAD);
    sigdump_output_fprint(&g_sigpool.ime_dat_out_ctx, "best_sad %x\n", ruiSAD);
    sigdump_output_fprint(&g_sigpool.ime_dat_out_ctx, "best_imvx %x\n", iBestX << 2);
    sigdump_output_fprint(&g_sigpool.ime_dat_out_ctx, "best_imvy %x\n", iBestY << 2);
  }
#endif

  return;
}


Void TEncSearch::xPatternSearchFast( const TComDataCU* const  pcCU,
                                     const TComPattern* const pcPatternKey,
                                     const Pel* const         piRefY,
                                     const Int                iRefStride,
                                     const TComMv* const      pcMvSrchRngLT,
                                     const TComMv* const      pcMvSrchRngRB,
                                     TComMv&                  rcMv,
                                     Distortion&              ruiSAD,
                                     const TComMv* const      pIntegerMv2Nx2NPred )
{
  assert (MD_LEFT < NUM_MV_PREDICTORS);
  pcCU->getMvPredLeft       ( m_acMvPredictors[MD_LEFT] );
  assert (MD_ABOVE < NUM_MV_PREDICTORS);
  pcCU->getMvPredAbove      ( m_acMvPredictors[MD_ABOVE] );
  assert (MD_ABOVE_RIGHT < NUM_MV_PREDICTORS);
  pcCU->getMvPredAboveRight ( m_acMvPredictors[MD_ABOVE_RIGHT] );

  switch ( m_motionEstimationSearchMethod )
  {
    case MESEARCH_DIAMOND:
      xTZSearch( pcCU, pcPatternKey, piRefY, iRefStride, pcMvSrchRngLT, pcMvSrchRngRB, rcMv, ruiSAD, pIntegerMv2Nx2NPred, false );
      break;

    case MESEARCH_SELECTIVE:
      xTZSearchSelective( pcCU, pcPatternKey, piRefY, iRefStride, pcMvSrchRngLT, pcMvSrchRngRB, rcMv, ruiSAD, pIntegerMv2Nx2NPred );
      break;

    case MESEARCH_DIAMOND_ENHANCED:
      xTZSearch( pcCU, pcPatternKey, piRefY, iRefStride, pcMvSrchRngLT, pcMvSrchRngRB, rcMv, ruiSAD, pIntegerMv2Nx2NPred, true );
      break;

    case MESEARCH_FULL: // shouldn't get here.
    default:
      break;
  }
}


Void TEncSearch::xTZSearch( const TComDataCU* const pcCU,
                            const TComPattern* const pcPatternKey,
                            const Pel* const         piRefY,
                            const Int                iRefStride,
                            const TComMv* const      pcMvSrchRngLT,
                            const TComMv* const      pcMvSrchRngRB,
                            TComMv&                  rcMv,
                            Distortion&              ruiSAD,
                            const TComMv* const      pIntegerMv2Nx2NPred,
                            const Bool               bExtendedSettings)
{
  const Bool bUseAdaptiveRaster                      = bExtendedSettings;
  const Int  iRaster                                 = 5;
  const Bool bTestOtherPredictedMV                   = bExtendedSettings;
  const Bool bTestZeroVector                         = true;
  const Bool bTestZeroVectorStart                    = bExtendedSettings;
  const Bool bTestZeroVectorStop                     = false;
  const Bool bFirstSearchDiamond                     = true;  // 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch
  const Bool bFirstCornersForDiamondDist1            = bExtendedSettings;
  const Bool bFirstSearchStop                        = m_pcEncCfg->getFastMEAssumingSmootherMVEnabled();
  const UInt uiFirstSearchRounds                     = 3;     // first search stop X rounds after best match (must be >=1)
  const Bool bEnableRasterSearch                     = true;
  const Bool bAlwaysRasterSearch                     = bExtendedSettings;  // true: BETTER but factor 2 slower
  const Bool bRasterRefinementEnable                 = false; // enable either raster refinement or star refinement
  const Bool bRasterRefinementDiamond                = false; // 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch
  const Bool bRasterRefinementCornersForDiamondDist1 = bExtendedSettings;
  const Bool bStarRefinementEnable                   = true;  // enable either star refinement or raster refinement
  const Bool bStarRefinementDiamond                  = true;  // 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch
  const Bool bStarRefinementCornersForDiamondDist1   = bExtendedSettings;
  const Bool bStarRefinementStop                     = false;
  const UInt uiStarRefinementRounds                  = 2;  // star refinement stop X rounds after best match (must be >=1)
  const Bool bNewZeroNeighbourhoodTest               = bExtendedSettings;

  UInt uiSearchRange = m_iSearchRange;
  pcCU->clipMv( rcMv );
#if ME_ENABLE_ROUNDING_OF_MVS
  rcMv.divideByPowerOf2(2);
#else
  rcMv >>= 2;
#endif
  // init TZSearchStruct
  IntTZSearchStruct cStruct;
  cStruct.iYStride    = iRefStride;
  cStruct.piRefY      = piRefY;
  cStruct.uiBestSad   = MAX_UINT;

  // set rcMv (Median predictor) as start point and as best point
  xTZSearchHelp( pcPatternKey, cStruct, rcMv.getHor(), rcMv.getVer(), 0, 0 );

  // test whether one of PRED_A, PRED_B, PRED_C MV is better start point than Median predictor
  if ( bTestOtherPredictedMV )
  {
    for ( UInt index = 0; index < NUM_MV_PREDICTORS; index++ )
    {
      TComMv cMv = m_acMvPredictors[index];
      pcCU->clipMv( cMv );
#if ME_ENABLE_ROUNDING_OF_MVS
      cMv.divideByPowerOf2(2);
#else
      cMv >>= 2;
#endif
      if (cMv != rcMv && (cMv.getHor() != cStruct.iBestX && cMv.getVer() != cStruct.iBestY))
      {
        // only test cMV if not obviously previously tested.
        xTZSearchHelp( pcPatternKey, cStruct, cMv.getHor(), cMv.getVer(), 0, 0 );
      }
    }
  }

  // test whether zero Mv is better start point than Median predictor
  if ( bTestZeroVector )
  {
    if ((rcMv.getHor() != 0 || rcMv.getVer() != 0) &&
        (0 != cStruct.iBestX || 0 != cStruct.iBestY))
    {
      // only test 0-vector if not obviously previously tested.
      xTZSearchHelp( pcPatternKey, cStruct, 0, 0, 0, 0 );
    }
  }

  Int   iSrchRngHorLeft   = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight  = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop    = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom = pcMvSrchRngRB->getVer();

  if (pIntegerMv2Nx2NPred != 0)
  {
    TComMv integerMv2Nx2NPred = *pIntegerMv2Nx2NPred;
    integerMv2Nx2NPred <<= 2;
    pcCU->clipMv( integerMv2Nx2NPred );
#if ME_ENABLE_ROUNDING_OF_MVS
    integerMv2Nx2NPred.divideByPowerOf2(2);
#else
    integerMv2Nx2NPred >>= 2;
#endif
    if ((rcMv != integerMv2Nx2NPred) &&
        (integerMv2Nx2NPred.getHor() != cStruct.iBestX || integerMv2Nx2NPred.getVer() != cStruct.iBestY))
    {
      // only test integerMv2Nx2NPred if not obviously previously tested.
      xTZSearchHelp(pcPatternKey, cStruct, integerMv2Nx2NPred.getHor(), integerMv2Nx2NPred.getVer(), 0, 0);
    }

    // reset search range
    TComMv cMvSrchRngLT;
    TComMv cMvSrchRngRB;
    Int iSrchRng = m_iSearchRange;
    TComMv currBestMv(cStruct.iBestX, cStruct.iBestY );
    currBestMv <<= 2;
#if MCTS_ENC_CHECK
    xSetSearchRange(pcCU, currBestMv, iSrchRng, cMvSrchRngLT, cMvSrchRngRB, pcPatternKey);
#else
    xSetSearchRange(pcCU, currBestMv, iSrchRng, cMvSrchRngLT, cMvSrchRngRB);
#endif
    iSrchRngHorLeft   = cMvSrchRngLT.getHor();
    iSrchRngHorRight  = cMvSrchRngRB.getHor();
    iSrchRngVerTop    = cMvSrchRngLT.getVer();
    iSrchRngVerBottom = cMvSrchRngRB.getVer();
  }

  // start search
  Int  iDist = 0;
  Int  iStartX = cStruct.iBestX;
  Int  iStartY = cStruct.iBestY;

  const Bool bBestCandidateZero = (cStruct.iBestX == 0) && (cStruct.iBestY == 0);

  // first search around best position up to now.
  // The following works as a "subsampled/log" window search around the best candidate
  for ( iDist = 1; iDist <= (Int)uiSearchRange; iDist*=2 )
  {
    if ( bFirstSearchDiamond == 1 )
    {
      xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist, bFirstCornersForDiamondDist1 );
    }
    else
    {
      xTZ8PointSquareSearch  ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
    }

    if ( bFirstSearchStop && ( cStruct.uiBestRound >= uiFirstSearchRounds ) ) // stop criterion
    {
      break;
    }
  }

  if (!bNewZeroNeighbourhoodTest)
  {
    // test whether zero Mv is a better start point than Median predictor
    if ( bTestZeroVectorStart && ((cStruct.iBestX != 0) || (cStruct.iBestY != 0)) )
    {
      xTZSearchHelp( pcPatternKey, cStruct, 0, 0, 0, 0 );
      if ( (cStruct.iBestX == 0) && (cStruct.iBestY == 0) )
      {
        // test its neighborhood
        for ( iDist = 1; iDist <= (Int)uiSearchRange; iDist*=2 )
        {
          xTZ8PointDiamondSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, 0, 0, iDist, false );
          if ( bTestZeroVectorStop && (cStruct.uiBestRound > 0) ) // stop criterion
          {
            break;
          }
        }
      }
    }
  }
  else
  {
    // Test also zero neighbourhood but with half the range
    // It was reported that the original (above) search scheme using bTestZeroVectorStart did not
    // make sense since one would have already checked the zero candidate earlier
    // and thus the conditions for that test would have not been satisfied
    if (bTestZeroVectorStart == true && bBestCandidateZero != true)
    {
      for ( iDist = 1; iDist <= ((Int)uiSearchRange >> 1); iDist*=2 )
      {
        xTZ8PointDiamondSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, 0, 0, iDist, false );
        if ( bTestZeroVectorStop && (cStruct.uiBestRound > 2) ) // stop criterion
        {
          break;
        }
      }
    }
  }

  // calculate only 2 missing points instead 8 points if cStruct.uiBestDistance == 1
  if ( cStruct.uiBestDistance == 1 )
  {
    cStruct.uiBestDistance = 0;
    xTZ2PointSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB );
  }

  // raster search if distance is too big
  if (bUseAdaptiveRaster)
  {
    Int   iWindowSize = iRaster;
    Int   iSrchRngRasterLeft   = iSrchRngHorLeft;
    Int   iSrchRngRasterRight  = iSrchRngHorRight;
    Int   iSrchRngRasterTop    = iSrchRngVerTop;
    Int   iSrchRngRasterBottom = iSrchRngVerBottom;

    if (!(bEnableRasterSearch && ( ((Int)(cStruct.uiBestDistance) > iRaster))))
    {
      iWindowSize ++;
      iSrchRngRasterLeft /= 2;
      iSrchRngRasterRight /= 2;
      iSrchRngRasterTop /= 2;
      iSrchRngRasterBottom /= 2;
    }
    cStruct.uiBestDistance = iWindowSize;
    for ( iStartY = iSrchRngRasterTop; iStartY <= iSrchRngRasterBottom; iStartY += iWindowSize )
    {
      for ( iStartX = iSrchRngRasterLeft; iStartX <= iSrchRngRasterRight; iStartX += iWindowSize )
      {
        xTZSearchHelp( pcPatternKey, cStruct, iStartX, iStartY, 0, iWindowSize );
      }
    }
  }
  else
  {
    if ( bEnableRasterSearch && ( ((Int)(cStruct.uiBestDistance) > iRaster) || bAlwaysRasterSearch ) )
    {
      cStruct.uiBestDistance = iRaster;
      for ( iStartY = iSrchRngVerTop; iStartY <= iSrchRngVerBottom; iStartY += iRaster )
      {
        for ( iStartX = iSrchRngHorLeft; iStartX <= iSrchRngHorRight; iStartX += iRaster )
        {
          xTZSearchHelp( pcPatternKey, cStruct, iStartX, iStartY, 0, iRaster );
        }
      }
    }
  }

  // raster refinement

  if ( bRasterRefinementEnable && cStruct.uiBestDistance > 0 )
  {
    while ( cStruct.uiBestDistance > 0 )
    {
      iStartX = cStruct.iBestX;
      iStartY = cStruct.iBestY;
      if ( cStruct.uiBestDistance > 1 )
      {
        iDist = cStruct.uiBestDistance >>= 1;
        if ( bRasterRefinementDiamond == 1 )
        {
          xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist, bRasterRefinementCornersForDiamondDist1 );
        }
        else
        {
          xTZ8PointSquareSearch  ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
        }
      }

      // calculate only 2 missing points instead 8 points if cStruct.uiBestDistance == 1
      if ( cStruct.uiBestDistance == 1 )
      {
        cStruct.uiBestDistance = 0;
        if ( cStruct.ucPointNr != 0 )
        {
          xTZ2PointSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB );
        }
      }
    }
  }

  // star refinement
  if ( bStarRefinementEnable && cStruct.uiBestDistance > 0 )
  {
    while ( cStruct.uiBestDistance > 0 )
    {
      iStartX = cStruct.iBestX;
      iStartY = cStruct.iBestY;
      cStruct.uiBestDistance = 0;
      cStruct.ucPointNr = 0;
      for ( iDist = 1; iDist < (Int)uiSearchRange + 1; iDist*=2 )
      {
        if ( bStarRefinementDiamond == 1 )
        {
          xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist, bStarRefinementCornersForDiamondDist1 );
        }
        else
        {
          xTZ8PointSquareSearch  ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
        }
        if ( bStarRefinementStop && (cStruct.uiBestRound >= uiStarRefinementRounds) ) // stop criterion
        {
          break;
        }
      }

      // calculate only 2 missing points instead 8 points if cStrukt.uiBestDistance == 1
      if ( cStruct.uiBestDistance == 1 )
      {
        cStruct.uiBestDistance = 0;
        if ( cStruct.ucPointNr != 0 )
        {
          xTZ2PointSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB );
        }
      }
    }
  }

  // write out best match
  rcMv.set( cStruct.iBestX, cStruct.iBestY );
  ruiSAD = cStruct.uiBestSad - m_pcRdCost->getCostOfVectorWithPredictor( cStruct.iBestX, cStruct.iBestY );
}


Void TEncSearch::xTZSearchSelective( const TComDataCU* const   pcCU,
                                     const TComPattern* const  pcPatternKey,
                                     const Pel* const          piRefY,
                                     const Int                 iRefStride,
                                     const TComMv* const       pcMvSrchRngLT,
                                     const TComMv* const       pcMvSrchRngRB,
                                     TComMv                   &rcMv,
                                     Distortion               &ruiSAD,
                                     const TComMv* const       pIntegerMv2Nx2NPred )
{
  const Bool bTestOtherPredictedMV    = true;
  const Bool bTestZeroVector          = true;
  const Bool bEnableRasterSearch      = true;
  const Bool bAlwaysRasterSearch      = false;  // 1: BETTER but factor 15x slower
  const Bool bStarRefinementEnable    = true;   // enable either star refinement or raster refinement
  const Bool bStarRefinementDiamond   = true;   // 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch
  const Bool bStarRefinementStop      = false;
  const UInt uiStarRefinementRounds   = 2;  // star refinement stop X rounds after best match (must be >=1)
  const UInt uiSearchRange            = m_iSearchRange;
  const Int  uiSearchRangeInitial     = m_iSearchRange >> 2;
  const Int  uiSearchStep             = 4;
  const Int  iMVDistThresh            = 8;

  Int   iSrchRngHorLeft         = pcMvSrchRngLT->getHor();
  Int   iSrchRngHorRight        = pcMvSrchRngRB->getHor();
  Int   iSrchRngVerTop          = pcMvSrchRngLT->getVer();
  Int   iSrchRngVerBottom       = pcMvSrchRngRB->getVer();
  Int   iFirstSrchRngHorLeft    = 0;
  Int   iFirstSrchRngHorRight   = 0;
  Int   iFirstSrchRngVerTop     = 0;
  Int   iFirstSrchRngVerBottom  = 0;
  Int   iStartX                 = 0;
  Int   iStartY                 = 0;
  Int   iBestX                  = 0;
  Int   iBestY                  = 0;
  Int   iDist                   = 0;

  pcCU->clipMv( rcMv );
#if ME_ENABLE_ROUNDING_OF_MVS
  rcMv.divideByPowerOf2(2);
#else
  rcMv >>= 2;
#endif
  // init TZSearchStruct
  IntTZSearchStruct cStruct;
  cStruct.iYStride    = iRefStride;
  cStruct.piRefY      = piRefY;
  cStruct.uiBestSad   = MAX_UINT;
  cStruct.iBestX = 0;
  cStruct.iBestY = 0;


  // set rcMv (Median predictor) as start point and as best point
  xTZSearchHelp( pcPatternKey, cStruct, rcMv.getHor(), rcMv.getVer(), 0, 0 );

  // test whether one of PRED_A, PRED_B, PRED_C MV is better start point than Median predictor
  if ( bTestOtherPredictedMV )
  {
    for ( UInt index = 0; index < NUM_MV_PREDICTORS; index++ )
    {
      TComMv cMv = m_acMvPredictors[index];
      pcCU->clipMv( cMv );
#if ME_ENABLE_ROUNDING_OF_MVS
      cMv.divideByPowerOf2(2);
#else
      cMv >>= 2;
#endif
      xTZSearchHelp( pcPatternKey, cStruct, cMv.getHor(), cMv.getVer(), 0, 0 );
    }
  }

  // test whether zero Mv is better start point than Median predictor
  if ( bTestZeroVector )
  {
    xTZSearchHelp( pcPatternKey, cStruct, 0, 0, 0, 0 );
  }

  if ( pIntegerMv2Nx2NPred != 0 )
  {
    TComMv integerMv2Nx2NPred = *pIntegerMv2Nx2NPred;
    integerMv2Nx2NPred <<= 2;
    pcCU->clipMv( integerMv2Nx2NPred );
#if ME_ENABLE_ROUNDING_OF_MVS
    integerMv2Nx2NPred.divideByPowerOf2(2);
#else
    integerMv2Nx2NPred >>= 2;
#endif
    xTZSearchHelp(pcPatternKey, cStruct, integerMv2Nx2NPred.getHor(), integerMv2Nx2NPred.getVer(), 0, 0);

    // reset search range
    TComMv cMvSrchRngLT;
    TComMv cMvSrchRngRB;
    Int iSrchRng = m_iSearchRange;
    TComMv currBestMv(cStruct.iBestX, cStruct.iBestY );
    currBestMv <<= 2;
#if MCTS_ENC_CHECK
    xSetSearchRange(pcCU, currBestMv, iSrchRng, cMvSrchRngLT, cMvSrchRngRB, pcPatternKey);
#else
    xSetSearchRange(pcCU, currBestMv, iSrchRng, cMvSrchRngLT, cMvSrchRngRB);
#endif
    iSrchRngHorLeft   = cMvSrchRngLT.getHor();
    iSrchRngHorRight  = cMvSrchRngRB.getHor();
    iSrchRngVerTop    = cMvSrchRngLT.getVer();
    iSrchRngVerBottom = cMvSrchRngRB.getVer();
  }

  // Initial search
  iBestX = cStruct.iBestX;
  iBestY = cStruct.iBestY;
  iFirstSrchRngHorLeft    = ((iBestX - uiSearchRangeInitial) > iSrchRngHorLeft)   ? (iBestX - uiSearchRangeInitial) : iSrchRngHorLeft;
  iFirstSrchRngVerTop     = ((iBestY - uiSearchRangeInitial) > iSrchRngVerTop)    ? (iBestY - uiSearchRangeInitial) : iSrchRngVerTop;
  iFirstSrchRngHorRight   = ((iBestX + uiSearchRangeInitial) < iSrchRngHorRight)  ? (iBestX + uiSearchRangeInitial) : iSrchRngHorRight;
  iFirstSrchRngVerBottom  = ((iBestY + uiSearchRangeInitial) < iSrchRngVerBottom) ? (iBestY + uiSearchRangeInitial) : iSrchRngVerBottom;

  for ( iStartY = iFirstSrchRngVerTop; iStartY <= iFirstSrchRngVerBottom; iStartY += uiSearchStep )
  {
    for ( iStartX = iFirstSrchRngHorLeft; iStartX <= iFirstSrchRngHorRight; iStartX += uiSearchStep )
    {
      xTZSearchHelp( pcPatternKey, cStruct, iStartX, iStartY, 0, 0 );
      xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, 1, false );
      xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, 2, false );
    }
  }

  Int iMaxMVDistToPred = (abs(cStruct.iBestX - iBestX) > iMVDistThresh || abs(cStruct.iBestY - iBestY) > iMVDistThresh);

  //full search with early exit if MV is distant from predictors
  if ( bEnableRasterSearch && (iMaxMVDistToPred || bAlwaysRasterSearch) )
  {
    for ( iStartY = iSrchRngVerTop; iStartY <= iSrchRngVerBottom; iStartY += 1 )
    {
      for ( iStartX = iSrchRngHorLeft; iStartX <= iSrchRngHorRight; iStartX += 1 )
      {
        xTZSearchHelp( pcPatternKey, cStruct, iStartX, iStartY, 0, 1 );
      }
    }
  }
  //Smaller MV, refine around predictor
  else if ( bStarRefinementEnable && cStruct.uiBestDistance > 0 )
  {
    // start refinement
    while ( cStruct.uiBestDistance > 0 )
    {
      iStartX = cStruct.iBestX;
      iStartY = cStruct.iBestY;
      cStruct.uiBestDistance = 0;
      cStruct.ucPointNr = 0;
      for ( iDist = 1; iDist < (Int)uiSearchRange + 1; iDist*=2 )
      {
        if ( bStarRefinementDiamond == 1 )
        {
          xTZ8PointDiamondSearch ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist, false );
        }
        else
        {
          xTZ8PointSquareSearch  ( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB, iStartX, iStartY, iDist );
        }
        if ( bStarRefinementStop && (cStruct.uiBestRound >= uiStarRefinementRounds) ) // stop criterion
        {
          break;
        }
      }

      // calculate only 2 missing points instead 8 points if cStrukt.uiBestDistance == 1
      if ( cStruct.uiBestDistance == 1 )
      {
        cStruct.uiBestDistance = 0;
        if ( cStruct.ucPointNr != 0 )
        {
          xTZ2PointSearch( pcPatternKey, cStruct, pcMvSrchRngLT, pcMvSrchRngRB );
        }
      }
    }
  }

  // write out best match
  rcMv.set( cStruct.iBestX, cStruct.iBestY );
  ruiSAD = cStruct.uiBestSad - m_pcRdCost->getCostOfVectorWithPredictor( cStruct.iBestX, cStruct.iBestY );

}


Void TEncSearch::xPatternSearchFracDIF(
                                       Bool         bIsLosslessCoded,
                                       TComPattern* pcPatternKey,
                                       Pel*         piRefY,
                                       Int          iRefStride,
                                       TComMv*      pcMvInt,
                                       TComMv&      rcMvHalf,
                                       TComMv&      rcMvQter,
                                       Distortion&  ruiCost
                                      )
{
  //  Reference pattern initialization (integer scale)
  TComPattern cPatternRoi;
  Int         iOffset    = pcMvInt->getHor() + pcMvInt->getVer() * iRefStride;
  cPatternRoi.initPattern(piRefY + iOffset,
                          pcPatternKey->getROIYWidth(),
                          pcPatternKey->getROIYHeight(),
                          iRefStride,
#if MCTS_ENC_CHECK
                          pcPatternKey->getBitDepthY(),
                          pcPatternKey->getROIYPosX(),
                          pcPatternKey->getROIYPosY());
#else
                          pcPatternKey->getBitDepthY());
#endif
#if MCTS_ENC_CHECK
  cPatternRoi.setTileBorders(pcPatternKey->getTileLeftTopPelPosX(), pcPatternKey->getTileLeftTopPelPosY(), pcPatternKey->getTileRightBottomPelPosX(), pcPatternKey->getTileRightBottomPelPosY());
#endif

#ifdef SIG_FME
  if (g_sigdump.fme) {
    g_sigpool.fme_cur_is_16_blk = (pcPatternKey->getROIYWidth() == 16) ? 1 : 0;
    sig_ctx *ctx_cmd_in = &g_sigpool.fme_cmd_in_ctx[g_sigpool.fme_cur_is_16_blk];
    sig_ctx *ctx_dat_out = &g_sigpool.fme_dat_out_ctx[g_sigpool.fme_cur_is_16_blk];
    sig_ctx *ctx_dat_txt_out = &g_sigpool.fme_dat_out_txt_ctx[g_sigpool.fme_cur_is_16_blk];
    if (g_sigpool.fme_cur_is_16_blk) {
      sigdump_output_fprint(ctx_cmd_in, "BLK16 #%x\n", g_sigpool.fme_16_cnt);
      sigdump_output_fprint(ctx_dat_txt_out, "BLK16 #%x, %x,%x\n", g_sigpool.fme_16_cnt++, g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    } else {
      sigdump_output_fprint(ctx_cmd_in, "BLK8 #%x\n", g_sigpool.fme_8_cnt);
      sigdump_output_fprint(ctx_dat_txt_out, "BLK8 #%x, %x,%x\n", g_sigpool.fme_8_cnt++, g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    }
    sigdump_output_fprint(ctx_cmd_in, "early_terminate = %x\n", 0);
    sigdump_output_fprint(ctx_cmd_in, "blk_mvpx = %x\n", m_pcRdCost->getPredictor().getHor());
    sigdump_output_fprint(ctx_cmd_in, "blk_mvpy = %x\n", m_pcRdCost->getPredictor().getVer());
    sigdump_output_fprint(ctx_cmd_in, "blk_imvx = %x\n", pcMvInt->getHor() << 2);
    sigdump_output_fprint(ctx_cmd_in, "blk_imvy = %x\n", pcMvInt->getVer() << 2);
    sigdump_output_fprint(ctx_cmd_in, "blk_cur_x = %x\n", g_sigpool.cu_idx_x);
    sigdump_output_fprint(ctx_cmd_in, "blk_cur_y = %x\n", g_sigpool.cu_idx_y);
    sigdump_output_fprint(ctx_dat_txt_out, "//source data\n");
    //source data
    for (int i = 0; i < pcPatternKey->getROIYHeight(); i++) {
      Pel* src_p = pcPatternKey->getROIY() + i * pcPatternKey->getPatternLStride();
      for (int j = 0; j < pcPatternKey->getROIYWidth(); j++) {
         sigdump_output_bin(ctx_dat_out, (unsigned char *)(src_p + j), 1);
         sigdump_output_fprint(ctx_dat_txt_out, "%02x", *(src_p + j));
      }
      sigdump_output_fprint(ctx_dat_txt_out, "\n");
    }
    sigdump_output_fprint(ctx_dat_txt_out, "//reference data\n");
    //reference data
    for (int i = -4; i < cPatternRoi.getROIYHeight() + 4; i++) {
      Pel* src_p = cPatternRoi.getROIY() + i * cPatternRoi.getPatternLStride();
      for (int j = -4; j < cPatternRoi.getROIYWidth() + 4; j++) {
         sigdump_output_bin(ctx_dat_out, (unsigned char *)(src_p + j), 1);
         sigdump_output_fprint(ctx_dat_txt_out, "%02x", *(src_p + j));
      }
      sigdump_output_fprint(ctx_dat_txt_out, "\n");
    }
  }
#endif
  //  Half-pel refinement
  xExtDIFUpSamplingH ( &cPatternRoi );

  rcMvHalf = *pcMvInt;   rcMvHalf <<= 1;    // for mv-cost
  TComMv baseRefMv(0, 0);
  ruiCost = xPatternRefinement( pcPatternKey, baseRefMv, 2, rcMvHalf, !bIsLosslessCoded );

  m_pcRdCost->setCostScale( 0 );

  xExtDIFUpSamplingQ ( &cPatternRoi, rcMvHalf );
  baseRefMv = rcMvHalf;
  baseRefMv <<= 1;

  rcMvQter = *pcMvInt;   rcMvQter <<= 1;    // for mv-cost
  rcMvQter += rcMvHalf;  rcMvQter <<= 1;
  ruiCost = xPatternRefinement( pcPatternKey, baseRefMv, 1, rcMvQter, !bIsLosslessCoded );
}


//! encode residual and calculate rate-distortion for a CU block
Void TEncSearch::encodeResAndCalcRdInterCU( TComDataCU* pcCU, TComYuv* pcYuvOrg, TComYuv* pcYuvPred,
                                            TComYuv* pcYuvResi, TComYuv* pcYuvResiBest, TComYuv* pcYuvRec,
                                            Bool bSkipResidual DEBUG_STRING_FN_DECLARE(sDebug) )
{
  assert ( !pcCU->isIntra(0) );

  const UInt cuWidthPixels      = pcCU->getWidth ( 0 );
  const UInt cuHeightPixels     = pcCU->getHeight( 0 );
  const Int  numValidComponents = pcCU->getPic()->getNumberValidComponents();
  const TComSPS &sps=*(pcCU->getSlice()->getSPS());

  // The pcCU is not marked as skip-mode at this point, and its m_pcTrCoeff, m_pcArlCoeff, m_puhCbf, m_puhTrIdx will all be 0.
  // due to prior calls to TComDataCU::initEstData(  );

  if ( bSkipResidual ) //  No residual coding : SKIP mode
  {
    pcCU->setSkipFlagSubParts( true, 0, pcCU->getDepth(0) );

    pcYuvResi->clear();

    pcYuvPred->copyToPartYuv( pcYuvRec, 0 );
    Distortion distortion = 0;

    for (Int comp=0; comp < numValidComponents; comp++)
    {
      const ComponentID compID=ComponentID(comp);
      const UInt csx=pcYuvOrg->getComponentScaleX(compID);
      const UInt csy=pcYuvOrg->getComponentScaleY(compID);

      distortion += m_pcRdCost->getDistPart( sps.getBitDepth(toChannelType(compID)), pcYuvRec->getAddr(compID), pcYuvRec->getStride(compID), pcYuvOrg->getAddr(compID),
                                               pcYuvOrg->getStride(compID), cuWidthPixels >> csx, cuHeightPixels >> csy, compID);
    }

    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_CURR_BEST]);
    m_pcEntropyCoder->resetBits();

    if (pcCU->getSlice()->getPPS()->getTransquantBypassEnabledFlag())
    {
      m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
    }

    m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
    m_pcEntropyCoder->encodeMergeIndex( pcCU, 0, true );

    UInt uiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
    pcCU->getTotalBits()       = uiBits;
    pcCU->getTotalDistortion() = distortion;
    pcCU->getTotalCost()       = m_pcRdCost->calcRdCost( uiBits, distortion );

    m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_TEMP_BEST]);

#if DEBUG_STRING
    pcYuvResiBest->clear(); // Clear the residual image, if we didn't code it.
    for(UInt i=0; i<MAX_NUM_COMPONENT+1; i++)
    {
      sDebug+=debug_reorder_data_inter_token[i];
    }
#endif

    return;
  }

  //  Residual coding.

   pcYuvResi->subtract( pcYuvOrg, pcYuvPred, 0, cuWidthPixels );

#ifdef CVI_LC_RD_COST
  if (isEnableLC_RdCostInter()) {
    pcCU->getTotalLcCost() = 0;
    pcCU->getTotalLcSADCost() = 0;
    Distortion lc_cost[3];
#if defined(SIG_MC) || defined(SIG_IRPU) || defined(SIG_CCU)
    Distortion mc_distortion[2] = {0};
#endif
    for (Int comp=0; comp < numValidComponents; comp++) {
      const ComponentID component = ComponentID(comp);
      const Int uiPartWidth =cuWidthPixels>>pcCU->getPic()->getComponentScaleX(component);
      const Int uiPartHeight=cuWidthPixels>>pcCU->getPic()->getComponentScaleY(component);
      const Pel* piSrc = pcYuvOrg->getAddr( component, 0, uiPartWidth );
      const Pel* piPred = pcYuvPred->getAddr( component, 0, uiPartWidth );
      DistParam distParam;
      const Bool bUseHadamard = isEnableLC_RdCostSATD(true);
      Bool bDisable8x8 = isForceChromaHad4x4() && (comp > COMPONENT_Y);
      m_pcRdCost->setDistParam(distParam, sps.getBitDepth(comp == 0 ? CHANNEL_TYPE_LUMA : CHANNEL_TYPE_CHROMA), piSrc, pcYuvOrg->getStride(component),
      piPred, pcYuvPred->getStride(component), uiPartWidth, uiPartHeight, bUseHadamard, bDisable8x8);
      distParam.bApplyWeight = false;
      lc_cost[comp] = distParam.DistFunc(&distParam);
      pcCU->getTotalLcCost() += lc_cost[comp];
#if defined(SIG_MC) || defined(SIG_IRPU) || defined(SIG_CCU)
      if (g_sigdump.mc || g_sigdump.irpu || g_sigdump.ccu) {
        mc_distortion[comp == 0 ? 0 : 1] += m_pcRdCost->getDistPart(sps.getBitDepth(comp == 0 ? CHANNEL_TYPE_LUMA : CHANNEL_TYPE_CHROMA),
          piSrc, pcYuvOrg->getStride(component), piPred, pcYuvPred->getStride(component), uiPartWidth, uiPartHeight, component);
      }
#endif
    }
#if defined(SIG_MC) || defined(SIG_IRPU) || defined(SIG_CCU)
    if (g_sigdump.mc || g_sigdump.irpu || g_sigdump.ccu) {
      pcCU->getMcDistortion(CHANNEL_TYPE_LUMA) = mc_distortion[CHANNEL_TYPE_LUMA];
      pcCU->getMcDistortion(CHANNEL_TYPE_CHROMA) = mc_distortion[CHANNEL_TYPE_CHROMA];
      pcCU->getMcLcSADCost(CHANNEL_TYPE_LUMA) = lc_cost[0];
      pcCU->getMcLcSADCost(CHANNEL_TYPE_CHROMA) = lc_cost[1] + lc_cost[2];
    }
#endif
#ifdef SIG_IRPU
    if (g_sigdump.irpu) {
      int blk_size = sig_pat_blk_size_8(cuWidthPixels);
      if (g_sigpool.is_merge) {
        sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[blk_size], "$MERGE %05x ", mc_distortion[0] + mc_distortion[1]);
        sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[blk_size], "%05x ", lc_cost[0] + lc_cost[1] + lc_cost[2]);
        sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[blk_size], "%x ", blk_size);
      } else {
        const TComCUMvField* pcCUMvField = pcCU->getCUMvField( REF_PIC_LIST_0 );
        //int ref_idx = frm_buf_mgr_find_index_by_poc(pcCU->getSlice()->getRefPOC( REF_PIC_LIST_0, pcCUMvField->getRefIdx(0)));
        sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[blk_size], "$AMVP %x ", pcCUMvField->getRefIdx(0));
        sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[blk_size], "%x ", pcCUMvField->getMv(0).getVer());
        sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[blk_size], "%x ", pcCUMvField->getMv(0).getHor());
        sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[blk_size], "%x\n", blk_size);
      }
    }
#endif
#ifdef SIG_MC
    if (g_sigdump.mc && g_sigpool.is_merge) {
      int blk_size = sig_pat_blk_size_8(cuWidthPixels);
      sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[blk_size], "SATD_sum = %05x,%05x\n", pcCU->getMcLcSADCost(CHANNEL_TYPE_LUMA), pcCU->getMcLcSADCost(CHANNEL_TYPE_CHROMA));
      sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[blk_size], "SSE_sum = %05x,%05x\n", pcCU->getMcDistortion(CHANNEL_TYPE_LUMA), pcCU->getMcDistortion(CHANNEL_TYPE_CHROMA));
    }
#endif
    pcCU->getTotalLcSADCost() = (Distortion)pcCU->getTotalLcCost();
  }
#endif

#if defined(SIG_CCU) || defined(SIG_BIT_EST)
  if (g_sigdump.ccu || g_sigdump.rdo_sse) {
    memset(&g_sigpool.est_golden[sig_pat_blk_size_4(cuWidthPixels)][0], 0, sizeof(sig_est_golden_st));
  }
#endif
  TComTURecurse tuLevel0(pcCU, 0);

  Double     nonZeroCost       = 0;
  UInt       nonZeroBits       = 0;
  Distortion nonZeroDistortion = 0;
  Distortion zeroDistortion    = 0;

  m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ pcCU->getDepth( 0 ) ][ CI_CURR_BEST ] );

  xEstimateInterResidualQT( pcYuvResi,  nonZeroCost, nonZeroBits, nonZeroDistortion, &zeroDistortion, tuLevel0 DEBUG_STRING_PASS_INTO(sDebug) );

#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    g_sigpool.rru_temp_gold.sse[0] = nonZeroDistortion;
  }
#endif //~SIG_RRU

#if defined(SIG_CCU) || defined(SIG_BIT_EST)
  if (g_sigdump.ccu || g_sigdump.rdo_sse) {
    g_sigpool.est_golden[sig_pat_blk_size_4(g_sigpool.cu_width)][0].EST_BAC = nonZeroBits;
  }
#endif
  // -------------------------------------------------------
  // set the coefficients in the pcCU, and also calculates the residual data.
  // If a block full of 0's is efficient, then just use 0's.
  // The costs at this point do not include header bits.

  m_pcEntropyCoder->resetBits();
  m_pcEntropyCoder->encodeQtRootCbfZero( );
  const UInt   zeroResiBits = m_pcEntropyCoder->getNumberOfWrittenBits();
  const Double zeroCost     = (pcCU->isLosslessCoded( 0 )) ? (nonZeroCost+1) : (m_pcRdCost->calcRdCost( zeroResiBits, zeroDistortion ));
#ifdef CVI_DISABLE_INTER_ZERO_RES_CHECK
  if ( (zeroCost < nonZeroCost && !isDisableInterZeroResCheck()) || !pcCU->getQtRootCbf(0) )
#else
  if ( zeroCost < nonZeroCost || !pcCU->getQtRootCbf(0) )
#endif
  {
    const UInt uiQPartNum = tuLevel0.GetAbsPartIdxNumParts();
    ::memset( pcCU->getTransformIdx()     , 0, uiQPartNum * sizeof(UChar) );
    for (Int comp=0; comp < numValidComponents; comp++)
    {
      const ComponentID component = ComponentID(comp);
      ::memset( pcCU->getCbf( component ) , 0, uiQPartNum * sizeof(UChar) );
      ::memset( pcCU->getCrossComponentPredictionAlpha(component), 0, ( uiQPartNum * sizeof(SChar) ) );
    }
    static const UInt useTS[MAX_NUM_COMPONENT]={0,0,0};
    pcCU->setTransformSkipSubParts ( useTS, 0, pcCU->getDepth(0) );
#if DEBUG_STRING
    sDebug.clear();
    for(UInt i=0; i<MAX_NUM_COMPONENT+1; i++)
    {
      sDebug+=debug_reorder_data_inter_token[i];
    }
#endif
  }
  else
  {
    xSetInterResidualQTData( NULL, false, tuLevel0); // Call first time to set coefficients.
  }

  // all decisions now made. Fully encode the CU, including the headers:
  m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[pcCU->getDepth(0)][CI_CURR_BEST] );

  UInt finalBits = 0;

  xAddSymbolBitsInter( pcCU, finalBits );

  // we've now encoded the pcCU, and so have a valid bit cost

  if ( !pcCU->getQtRootCbf( 0 ) )
  {
    pcYuvResiBest->clear(); // Clear the residual image, if we didn't code it.
  }
  else
  {
    xSetInterResidualQTData( pcYuvResiBest, true, tuLevel0 ); // else set the residual image data pcYUVResiBest from the various temp images.
  }
  m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ pcCU->getDepth( 0 ) ][ CI_TEMP_BEST ] );

  pcYuvRec->addClip ( pcYuvPred, pcYuvResiBest, 0, cuWidthPixels, sps.getBitDepths() );

  // update with clipped distortion and cost (previously unclipped reconstruction values were used)
  Distortion finalDistortion = 0;

#ifdef CVI_ENABLE_RDO_FAST_DISTORTION
  finalDistortion = nonZeroDistortion;
#else
  for(Int comp=0; comp<numValidComponents; comp++)
  {
    const ComponentID compID=ComponentID(comp);
    finalDistortion += m_pcRdCost->getDistPart( sps.getBitDepth(toChannelType(compID)), pcYuvRec->getAddr(compID ), pcYuvRec->getStride(compID ), pcYuvOrg->getAddr(compID ), pcYuvOrg->getStride(compID), cuWidthPixels >> pcYuvOrg->getComponentScaleX(compID), cuHeightPixels >> pcYuvOrg->getComponentScaleY(compID), compID);
  }
#endif
  pcCU->getTotalBits()       = finalBits;
  pcCU->getTotalDistortion() = finalDistortion;
  pcCU->getTotalCost()       = m_pcRdCost->calcRdCost( finalBits, finalDistortion );

#ifdef CVI_LC_RD_COST
  if (isEnableLC_RdCostInter())
    pcCU->getTotalLcCost() = m_pcRdCost->calcRdCost(pcCU->getLcNoCoeffBits(), pcCU->getTotalLcCost() , DF_SAD);
#endif
}



Void TEncSearch::xEstimateInterResidualQT( TComYuv    *pcResi,
                                           Double     &rdCost,
                                           UInt       &ruiBits,
                                           Distortion &ruiDist,
                                           Distortion *puiZeroDist,
                                           TComTU     &rTu
                                           DEBUG_STRING_FN_DECLARE(sDebug) )
{
  TComDataCU *pcCU        = rTu.getCU();
  const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();
  const UInt uiDepth      = rTu.GetTransformDepthTotal();
  const UInt uiTrMode     = rTu.GetTransformDepthRel();
  const UInt subTUDepth   = uiTrMode + 1;
  const UInt numValidComp = pcCU->getPic()->getNumberValidComponents();
  DEBUG_STRING_NEW(sSingleStringComp[MAX_NUM_COMPONENT])

  assert( pcCU->getDepth( 0 ) == pcCU->getDepth( uiAbsPartIdx ) );
  const UInt uiLog2TrSize = rTu.GetLog2LumaTrSize();

  UInt SplitFlag = ((pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && pcCU->isInter(uiAbsPartIdx) && ( pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N ));
#if DEBUG_STRING
  const Int debugPredModeMask = DebugStringGetPredModeMask(pcCU->getPredictionMode(uiAbsPartIdx));
#endif

  Bool bCheckFull;

  if ( SplitFlag && uiDepth == pcCU->getDepth(uiAbsPartIdx) && ( uiLog2TrSize >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) ) )
  {
    bCheckFull = false;
  }
  else
  {
    bCheckFull =  ( uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() );
  }

  const Bool bCheckSplit  = ( uiLog2TrSize >  pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) );

  assert( bCheckFull || bCheckSplit );

  // code full block
  Double     dSingleCost = MAX_DOUBLE;
  UInt       uiSingleBits                                                                                                        = 0;
  Distortion uiSingleDistComp            [MAX_NUM_COMPONENT][2/*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = {{0,0},{0,0},{0,0}};
  Distortion uiSingleDist                                                                                                        = 0;
  TCoeff     uiAbsSum                    [MAX_NUM_COMPONENT][2/*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = {{0,0},{0,0},{0,0}};
  UInt       uiBestTransformMode         [MAX_NUM_COMPONENT][2/*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = {{0,0},{0,0},{0,0}};
  //  Stores the best explicit RDPCM mode for a TU encoded without split
  UInt       bestExplicitRdpcmModeUnSplit[MAX_NUM_COMPONENT][2/*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = {{3,3}, {3,3}, {3,3}};
  SChar      bestCrossCPredictionAlpha   [MAX_NUM_COMPONENT][2/*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/] = {{0,0},{0,0},{0,0}};

  m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );

  if( bCheckFull )
  {
    Double minCost[MAX_NUM_COMPONENT][2/*0 = top (or whole TU for non-4:2:2) sub-TU, 1 = bottom sub-TU*/];
    Bool checkTransformSkip[MAX_NUM_COMPONENT];
    pcCU->setTrIdxSubParts( uiTrMode, uiAbsPartIdx, uiDepth );

    m_pcEntropyCoder->resetBits();

    memset( m_pTempPel, 0, sizeof( Pel ) * rTu.getRect(COMPONENT_Y).width * rTu.getRect(COMPONENT_Y).height ); // not necessary needed for inside of recursion (only at the beginning)

    const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
    TCoeff *pcCoeffCurr[MAX_NUM_COMPONENT];
#if ADAPTIVE_QP_SELECTION
    TCoeff *pcArlCoeffCurr[MAX_NUM_COMPONENT];
#endif

    for(UInt i=0; i<numValidComp; i++)
    {
      minCost[i][0] = MAX_DOUBLE;
      minCost[i][1] = MAX_DOUBLE;
    }

    Pel crossCPredictedResidualBuffer[ MAX_TU_SIZE * MAX_TU_SIZE ];

    for(UInt i=0; i<numValidComp; i++)
    {
      checkTransformSkip[i]=false;
      const ComponentID compID=ComponentID(i);
      const Int channelBitDepth=pcCU->getSlice()->getSPS()->getBitDepth(toChannelType(compID));
      pcCoeffCurr[compID]    = m_ppcQTTempCoeff[compID][uiQTTempAccessLayer] + rTu.getCoefficientOffset(compID);
#if ADAPTIVE_QP_SELECTION
      pcArlCoeffCurr[compID] = m_ppcQTTempArlCoeff[compID ][uiQTTempAccessLayer] +  rTu.getCoefficientOffset(compID);
#endif

      if(rTu.ProcessComponentSection(compID))
      {
        const QpParam cQP(*pcCU, compID);

        checkTransformSkip[compID] = pcCU->getSlice()->getPPS()->getUseTransformSkip() &&
                                     TUCompRectHasAssociatedTransformSkipFlag(rTu.getRect(compID), pcCU->getSlice()->getPPS()->getPpsRangeExtension().getLog2MaxTransformSkipBlockSize()) &&
                                     (!pcCU->isLosslessCoded(0));

        const Bool splitIntoSubTUs = rTu.getRect(compID).width != rTu.getRect(compID).height;

        TComTURecurse TUIterator(rTu, false, (splitIntoSubTUs ? TComTU::VERTICAL_SPLIT : TComTU::DONT_SPLIT), true, compID);

        const UInt partIdxesPerSubTU = TUIterator.GetAbsPartIdxNumParts(compID);

        do
        {
          const UInt           subTUIndex             = TUIterator.GetSectionNumber();
          const UInt           subTUAbsPartIdx        = TUIterator.GetAbsPartIdxTU(compID);
          const TComRectangle &tuCompRect             = TUIterator.getRect(compID);
          const UInt           subTUBufferOffset      = tuCompRect.width * tuCompRect.height * subTUIndex;

                TCoeff        *currentCoefficients    = pcCoeffCurr[compID] + subTUBufferOffset;
#if ADAPTIVE_QP_SELECTION
                TCoeff        *currentARLCoefficients = pcArlCoeffCurr[compID] + subTUBufferOffset;
#endif
          const Bool isCrossCPredictionAvailable      =    isChroma(compID)
                                                         && pcCU->getSlice()->getPPS()->getPpsRangeExtension().getCrossComponentPredictionEnabledFlag()
                                                         && (pcCU->getCbf(subTUAbsPartIdx, COMPONENT_Y, uiTrMode) != 0);

          SChar preCalcAlpha = 0;
          const Pel *pLumaResi = m_pcQTTempTComYuv[uiQTTempAccessLayer].getAddrPix( COMPONENT_Y, rTu.getRect( COMPONENT_Y ).x0, rTu.getRect( COMPONENT_Y ).y0 );

          if (isCrossCPredictionAvailable)
          {
            const Bool bUseReconstructedResidualForEstimate = m_pcEncCfg->getUseReconBasedCrossCPredictionEstimate();
            const Pel  *const lumaResidualForEstimate       = bUseReconstructedResidualForEstimate ? pLumaResi                                                     : pcResi->getAddrPix(COMPONENT_Y, tuCompRect.x0, tuCompRect.y0);
            const UInt        lumaResidualStrideForEstimate = bUseReconstructedResidualForEstimate ? m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(COMPONENT_Y) : pcResi->getStride(COMPONENT_Y);

            preCalcAlpha = xCalcCrossComponentPredictionAlpha(TUIterator,
                                                              compID,
                                                              lumaResidualForEstimate,
                                                              pcResi->getAddrPix(compID, tuCompRect.x0, tuCompRect.y0),
                                                              tuCompRect.width,
                                                              tuCompRect.height,
                                                              lumaResidualStrideForEstimate,
                                                              pcResi->getStride(compID));
          }

          const Int transformSkipModesToTest    = checkTransformSkip[compID] ? 2 : 1;
          const Int crossCPredictionModesToTest = (preCalcAlpha != 0)        ? 2 : 1; // preCalcAlpha cannot be anything other than 0 if isCrossCPredictionAvailable is false

          const Bool isOneMode                  = (crossCPredictionModesToTest == 1) && (transformSkipModesToTest == 1);

          for (Int transformSkipModeId = 0; transformSkipModeId < transformSkipModesToTest; transformSkipModeId++)
          {
            pcCU->setTransformSkipPartRange(transformSkipModeId, compID, subTUAbsPartIdx, partIdxesPerSubTU);

            for (Int crossCPredictionModeId = 0; crossCPredictionModeId < crossCPredictionModesToTest; crossCPredictionModeId++)
            {
              const Bool isFirstMode          = (transformSkipModeId == 0) && (crossCPredictionModeId == 0);
              const Bool bUseCrossCPrediction = crossCPredictionModeId != 0;

              m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
              m_pcEntropyCoder->resetBits();

              pcCU->setTransformSkipPartRange(transformSkipModeId, compID, subTUAbsPartIdx, partIdxesPerSubTU);
              pcCU->setCrossComponentPredictionAlphaPartRange((bUseCrossCPrediction ? preCalcAlpha : 0), compID, subTUAbsPartIdx, partIdxesPerSubTU );

              if ((compID != COMPONENT_Cr) && ((transformSkipModeId == 1) ? m_pcEncCfg->getUseRDOQTS() : m_pcEncCfg->getUseRDOQ()))
              {
                COEFF_SCAN_TYPE scanType = COEFF_SCAN_TYPE(pcCU->getCoefScanIdx(uiAbsPartIdx, tuCompRect.width, tuCompRect.height, compID));
                m_pcEntropyCoder->estimateBit(m_pcTrQuant->m_pcEstBitsSbac, tuCompRect.width, tuCompRect.height, toChannelType(compID), scanType);
              }

#if RDOQ_CHROMA_LAMBDA
              m_pcTrQuant->selectLambda(compID);
#endif

              Pel *pcResiCurrComp = m_pcQTTempTComYuv[uiQTTempAccessLayer].getAddrPix(compID, tuCompRect.x0, tuCompRect.y0);
              UInt resiStride     = m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(compID);

              TCoeff bestCoeffComp   [MAX_TU_SIZE*MAX_TU_SIZE];
              Pel    bestResiComp    [MAX_TU_SIZE*MAX_TU_SIZE];

#if ADAPTIVE_QP_SELECTION
              TCoeff bestArlCoeffComp[MAX_TU_SIZE*MAX_TU_SIZE];
#endif
              TCoeff     currAbsSum   = 0;
              UInt       currCompBits = 0;
              Distortion currCompDist = 0;
              Double     currCompCost = 0;
              UInt       nonCoeffBits = 0;
              Distortion nonCoeffDist = 0;
              Double     nonCoeffCost = 0;

              if(!isOneMode && !isFirstMode)
              {
                memcpy(bestCoeffComp,    currentCoefficients,    (sizeof(TCoeff) * tuCompRect.width * tuCompRect.height));
#if ADAPTIVE_QP_SELECTION
                memcpy(bestArlCoeffComp, currentARLCoefficients, (sizeof(TCoeff) * tuCompRect.width * tuCompRect.height));
#endif
                for(Int y = 0; y < tuCompRect.height; y++)
                {
                  memcpy(&bestResiComp[y * tuCompRect.width], (pcResiCurrComp + (y * resiStride)), (sizeof(Pel) * tuCompRect.width));
                }
              }

              if (bUseCrossCPrediction)
              {
                TComTrQuant::crossComponentPrediction(TUIterator,
                                                      compID,
                                                      pLumaResi,
                                                      pcResi->getAddrPix(compID, tuCompRect.x0, tuCompRect.y0),
                                                      crossCPredictedResidualBuffer,
                                                      tuCompRect.width,
                                                      tuCompRect.height,
                                                      m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(COMPONENT_Y),
                                                      pcResi->getStride(compID),
                                                      tuCompRect.width,
                                                      false);

                m_pcTrQuant->transformNxN(TUIterator, compID, crossCPredictedResidualBuffer, tuCompRect.width, currentCoefficients,
#if ADAPTIVE_QP_SELECTION
                                          currentARLCoefficients,
#endif
                                          currAbsSum, cQP);
              }
              else
              {
                m_pcTrQuant->transformNxN(TUIterator, compID, pcResi->getAddrPix( compID, tuCompRect.x0, tuCompRect.y0 ), pcResi->getStride(compID), currentCoefficients,
#if ADAPTIVE_QP_SELECTION
                                          currentARLCoefficients,
#endif
                                          currAbsSum, cQP);
              }

              if(isFirstMode || (currAbsSum == 0))
              {
                if (bUseCrossCPrediction)
                {
                  TComTrQuant::crossComponentPrediction(TUIterator,
                                                        compID,
                                                        pLumaResi,
                                                        m_pTempPel,
                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getAddrPix(compID, tuCompRect.x0, tuCompRect.y0),
                                                        tuCompRect.width,
                                                        tuCompRect.height,
                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(COMPONENT_Y),
                                                        tuCompRect.width,
                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(compID),
                                                        true);

                  nonCoeffDist = m_pcRdCost->getDistPart( channelBitDepth, m_pcQTTempTComYuv[uiQTTempAccessLayer].getAddrPix( compID, tuCompRect.x0, tuCompRect.y0 ),
                                                          m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride( compID ), pcResi->getAddrPix( compID, tuCompRect.x0, tuCompRect.y0 ),
                                                          pcResi->getStride(compID), tuCompRect.width, tuCompRect.height, compID, DF_SSE, false); // initialized with zero residual distortion
                }
                else
                {
                  nonCoeffDist = m_pcRdCost->getDistPart( channelBitDepth, m_pTempPel, tuCompRect.width, pcResi->getAddrPix( compID, tuCompRect.x0, tuCompRect.y0 ),
                                                          pcResi->getStride(compID), tuCompRect.width, tuCompRect.height, compID, DF_SSE, false); // initialized with zero residual distortion
                }
#ifdef CVI_ENABLE_RDO_FAST_DISTORTION
                if (getRDOFastDistortion() != 0) {
                  Distortion CVI_SSE = 0;
                  m_pcTrQuant->GetCVI_SSE( rTu, compID, 0, &CVI_SSE);
                  Distortion cvi_dist = 0;
#ifdef CVI_FIX_POINT_RD_COST
                  cvi_dist = m_pcRdCost->getCVI_DistPart(compID, CVI_SSE, false);
#else
                  cvi_dist = m_pcRdCost->getCVI_DistPart(compID, CVI_SSE);
#endif
                  //printf("(nonCoeff)(Comp%d) (%d x %d)Dist = %d CVI Dist = %d\n", compID, tuCompRect.width, tuCompRect.height, (int)(nonCoeffDist), (int)(cvi_dist));
                  nonCoeffDist = cvi_dist;
#if defined(SIG_CCU) || defined(SIG_BIT_EST)
                  if (g_sigdump.ccu || g_sigdump.rdo_sse) {
                    g_sigpool.est_golden[sig_pat_blk_size_4(g_sigpool.cu_width)][0].EST_SSE[compID][rTu.GetSectionNumber()] = CVI_SSE;
                  }
#endif
#ifdef SIG_BIT_EST
                  if (g_sigdump.rdo_sse) {
                    //sig_ctx *rdo_ctx = &g_sigpool.rdo_sse_cmd_ctx[g_aucConvertToBit[g_sigpool.cu_width]];
                    TCoeff* m_plTempCoeff = m_pcTrQuant->getplTempCoeff_cvi();
                    int *pTCoeff = g_sigpool.est_golden[sig_pat_blk_size_4(g_sigpool.cu_width)][0].TCoeff[compID][rTu.GetSectionNumber()];

                    for (int i = 0; i < rTu.getRect(compID).height; i++) {
                      for (int j = 0; j < rTu.getRect(compID).width; j++) {
                        pTCoeff[i * rTu.getRect(compID).width + j]
                          = m_plTempCoeff[rTu.getRect(compID).width * i + j];
                      }
                    }
                  }
#endif
                }
#endif
                m_pcEntropyCoder->encodeQtCbfZero( TUIterator, toChannelType(compID) );

                if ( isCrossCPredictionAvailable )
                {
                  m_pcEntropyCoder->encodeCrossComponentPrediction( TUIterator, compID );
                }

                nonCoeffBits = m_pcEntropyCoder->getNumberOfWrittenBits();
                nonCoeffCost = m_pcRdCost->calcRdCost( nonCoeffBits, nonCoeffDist );
#if defined(SIG_CCU) || defined(SIG_BIT_EST)
                if (g_sigdump.ccu || g_sigdump.rdo_sse) {
                  g_sigpool.est_golden[sig_pat_blk_size_4(g_sigpool.cu_width)][0].EST_BIT[rTu.GetSectionNumber()] += nonCoeffBits;
                }
#endif
              }

              if((puiZeroDist != NULL) && isFirstMode)
              {
                *puiZeroDist += nonCoeffDist; // initialized with zero residual distortion
              }

              DEBUG_STRING_NEW(sSingleStringTest)

#ifdef SIG_RRU
              if (g_sigdump.rru)
              {
                memset(g_sigpool.p_rru_iq_out, 0, sizeof(int) * 256);
              }
#endif

              if( currAbsSum > 0 ) //if non-zero coefficients are present, a residual needs to be derived for further prediction
              {
                if (isFirstMode)
                {
                  m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
                  m_pcEntropyCoder->resetBits();
                }

                m_pcEntropyCoder->encodeQtCbf( TUIterator, compID, true );

                if (isCrossCPredictionAvailable)
                {
                  m_pcEntropyCoder->encodeCrossComponentPrediction( TUIterator, compID );
                }

                m_pcEntropyCoder->encodeCoeffNxN( TUIterator, currentCoefficients, compID );
                currCompBits = m_pcEntropyCoder->getNumberOfWrittenBits();

                pcResiCurrComp = m_pcQTTempTComYuv[uiQTTempAccessLayer].getAddrPix( compID, tuCompRect.x0, tuCompRect.y0 );

                m_pcTrQuant->invTransformNxN( TUIterator, compID, pcResiCurrComp, m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(compID), currentCoefficients, cQP DEBUG_STRING_PASS_INTO_OPTIONAL(&sSingleStringTest, (DebugOptionList::DebugString_InvTran.getInt()&debugPredModeMask)) );

                if (bUseCrossCPrediction)
                {
                  TComTrQuant::crossComponentPrediction(TUIterator,
                                                        compID,
                                                        pLumaResi,
                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getAddrPix(compID, tuCompRect.x0, tuCompRect.y0),
                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getAddrPix(compID, tuCompRect.x0, tuCompRect.y0),
                                                        tuCompRect.width,
                                                        tuCompRect.height,
                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(COMPONENT_Y),
                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(compID     ),
                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(compID     ),
                                                        true);
                }

                currCompDist = m_pcRdCost->getDistPart( channelBitDepth, m_pcQTTempTComYuv[uiQTTempAccessLayer].getAddrPix( compID, tuCompRect.x0, tuCompRect.y0 ),
                                                        m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(compID),
                                                        pcResi->getAddrPix( compID, tuCompRect.x0, tuCompRect.y0 ),
                                                        pcResi->getStride(compID),
                                                        tuCompRect.width, tuCompRect.height, compID, DF_SSE, false);
#ifdef CVI_ENABLE_RDO_FAST_DISTORTION
                if (getRDOFastDistortion() != 0) {
                  Distortion CVI_SSE = 0;
                  m_pcTrQuant->GetCVI_SSE( rTu, compID, currAbsSum, &CVI_SSE);
                  Distortion cvi_dist = 0;
                  cvi_dist = m_pcRdCost->getCVI_DistPart(compID, CVI_SSE);
                  //printf("(Inter)(Comp%d) (%d x %d)Dist = %d CVI Dist = %d\n", compID, tuCompRect.width, tuCompRect.height, (int)(currCompDist), (int)(cvi_dist));
                  currCompDist = cvi_dist;
#if defined(SIG_CCU) || defined(SIG_BIT_EST)
                  if (g_sigdump.ccu || g_sigdump.rdo_sse) {
                    g_sigpool.est_golden[sig_pat_blk_size_4(g_sigpool.cu_width)][0].EST_SSE[compID][rTu.GetSectionNumber()] = CVI_SSE;
                    g_sigpool.est_golden[sig_pat_blk_size_4(g_sigpool.cu_width)][0].EST_BIT[rTu.GetSectionNumber()] = currCompBits;
                  }
#endif
                }
#endif

#ifdef CVI_ENABLE_RDO_FAST_DISTORTION
                if (getRDOFastDistortion() != 0)
                {
                  if (compID == COMPONENT_Y)
                  {
                    currCompCost = m_pcRdCost->calcRdCost(currCompBits, currCompDist);
                  }
                  else if (compID == COMPONENT_Cb)
                  {
                    m_pcRdCost->setCbNumBits(currCompBits);
                    currCompCost = 0;
                  }
                  else
                  {
                    Int cbBits = m_pcRdCost->getCbNumBits();
                    currCompCost = m_pcRdCost->calcRdCost(currCompBits + cbBits, currCompDist);
                  }
                }
                else
                {
                  currCompCost = m_pcRdCost->calcRdCost(currCompBits, currCompDist);
                }
#else
                currCompCost = m_pcRdCost->calcRdCost(currCompBits, currCompDist);
#endif
                if (pcCU->isLosslessCoded(0))
                {
                  nonCoeffCost = MAX_DOUBLE;
                }
              }
              else if ((transformSkipModeId == 1) && !bUseCrossCPrediction)
              {
                currCompCost = MAX_DOUBLE;
              }
              else
              {
                currCompBits = nonCoeffBits;
                currCompDist = nonCoeffDist;
                currCompCost = nonCoeffCost;

#ifdef CVI_ENABLE_RDO_FAST_DISTORTION
                if (getRDOFastDistortion() != 0)
                {
                  if (compID == COMPONENT_Cb)
                  {
                    Distortion CVI_SSE = 0;
                    m_pcTrQuant->GetCVI_SSE( rTu, compID, 0, &CVI_SSE);
                    m_pcRdCost->getCVI_DistPart(compID, CVI_SSE); // store cb sse
                    m_pcRdCost->setCbNumBits(currCompBits);       // store cb bits
                    currCompDist = 0;
                  }
                  else if (compID == COMPONENT_Cr)
                  {
                    Distortion CVI_SSE = 0;
                    m_pcTrQuant->GetCVI_SSE( rTu, compID, 0, &CVI_SSE);
                    currCompDist = m_pcRdCost->getCVI_DistPart(compID, CVI_SSE);
                    Int cbBits = m_pcRdCost->getCbNumBits();
                    currCompCost = m_pcRdCost->calcRdCost(currCompBits + cbBits, currCompDist); // cb+cr cost
                    m_pcRdCost->setCbNumBits(0);  // reset cb bits to 0.
                  }
                }
#endif
              }

              // evaluate
              if ((currCompCost < minCost[compID][subTUIndex]) || ((transformSkipModeId == 1) && (currCompCost == minCost[compID][subTUIndex])))
              {
                bestExplicitRdpcmModeUnSplit[compID][subTUIndex] = pcCU->getExplicitRdpcmMode(compID, subTUAbsPartIdx);

#ifdef CVI_FIX_POINT_RD_COST
                if(isFirstMode && compID == COMPONENT_Y)
#else
                if(isFirstMode) //check for forced null
#endif
                {
#ifdef CVI_DISABLE_INTER_ZERO_RES_CHECK
                  if(((nonCoeffCost < currCompCost) && !isDisableInterZeroResCheck()) || (currAbsSum == 0))
#else
                  if((nonCoeffCost < currCompCost) || (currAbsSum == 0))
#endif
                  {
                    memset(currentCoefficients, 0, (sizeof(TCoeff) * tuCompRect.width * tuCompRect.height));

                    currAbsSum   = 0;
                    currCompBits = nonCoeffBits;
                    currCompDist = nonCoeffDist;
                    currCompCost = nonCoeffCost;
                  }
                }

#if DEBUG_STRING
                if (currAbsSum > 0)
                {
                  DEBUG_STRING_SWAP(sSingleStringComp[compID], sSingleStringTest)
                }
                else
                {
                  sSingleStringComp[compID].clear();
                }
#endif

                uiAbsSum                 [compID][subTUIndex] = currAbsSum;
                uiSingleDistComp         [compID][subTUIndex] = currCompDist;
                minCost                  [compID][subTUIndex] = currCompCost;
                uiBestTransformMode      [compID][subTUIndex] = transformSkipModeId;
                bestCrossCPredictionAlpha[compID][subTUIndex] = (crossCPredictionModeId == 1) ? pcCU->getCrossComponentPredictionAlpha(subTUAbsPartIdx, compID) : 0;

                if (uiAbsSum[compID][subTUIndex] == 0)
                {
                  if (bUseCrossCPrediction)
                  {
                    TComTrQuant::crossComponentPrediction(TUIterator,
                                                          compID,
                                                          pLumaResi,
                                                          m_pTempPel,
                                                          m_pcQTTempTComYuv[uiQTTempAccessLayer].getAddrPix(compID, tuCompRect.x0, tuCompRect.y0),
                                                          tuCompRect.width,
                                                          tuCompRect.height,
                                                          m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(COMPONENT_Y),
                                                          tuCompRect.width,
                                                          m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(compID),
                                                          true);
                  }
                  else
                  {
                    pcResiCurrComp = m_pcQTTempTComYuv[uiQTTempAccessLayer].getAddrPix(compID, tuCompRect.x0, tuCompRect.y0);
                    const UInt uiStride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(compID);
                    for(UInt uiY = 0; uiY < tuCompRect.height; uiY++)
                    {
                      memset(pcResiCurrComp, 0, (sizeof(Pel) * tuCompRect.width));
                      pcResiCurrComp += uiStride;
                    }
                  }
                }
#ifdef SIG_RRU
                if (g_sigdump.rru)
                {
                  int res_stride = m_pcQTTempTComYuv[uiQTTempAccessLayer].getStride(compID);
                  const TComRectangle tu_rect = rTu.getRect(compID);
                  Pel *p_residual_buf = m_pcQTTempTComYuv[uiQTTempAccessLayer].getAddrPix(compID, tuCompRect.x0, tuCompRect.y0);
                  sig_rru_copy_idct_temp_buf(compID, p_residual_buf, res_stride, g_sigpool.p_rru_iq_out, tuCompRect.width, tu_rect.x0, tu_rect.y0);
#ifdef SIG_RRU_DEBUG
                  if (g_sigdump.rru_debug)
                  {
                    sig_rru_copy_iq_in_temp_buf(compID, currentCoefficients, tuCompRect.width, tu_rect.x0, tu_rect.y0);
                  }
#endif
                }
#endif //~SIG_RRU
              }
              else
              {
                // reset
                memcpy(currentCoefficients,    bestCoeffComp,    (sizeof(TCoeff) * tuCompRect.width * tuCompRect.height));
#if ADAPTIVE_QP_SELECTION
                memcpy(currentARLCoefficients, bestArlCoeffComp, (sizeof(TCoeff) * tuCompRect.width * tuCompRect.height));
#endif
                for (Int y = 0; y < tuCompRect.height; y++)
                {
                  memcpy((pcResiCurrComp + (y * resiStride)), &bestResiComp[y * tuCompRect.width], (sizeof(Pel) * tuCompRect.width));
                }
              }
            }
          }

          pcCU->setExplicitRdpcmModePartRange            (   bestExplicitRdpcmModeUnSplit[compID][subTUIndex],                            compID, subTUAbsPartIdx, partIdxesPerSubTU);
          pcCU->setTransformSkipPartRange                (   uiBestTransformMode         [compID][subTUIndex],                            compID, subTUAbsPartIdx, partIdxesPerSubTU );
          pcCU->setCbfPartRange                          ((((uiAbsSum                    [compID][subTUIndex] > 0) ? 1 : 0) << uiTrMode), compID, subTUAbsPartIdx, partIdxesPerSubTU );
          pcCU->setCrossComponentPredictionAlphaPartRange(   bestCrossCPredictionAlpha   [compID][subTUIndex],                            compID, subTUAbsPartIdx, partIdxesPerSubTU );
        } while (TUIterator.nextSection(rTu)); //end of sub-TU loop
      } // processing section
    } // component loop

    for(UInt ch = 0; ch < numValidComp; ch++)
    {
      const ComponentID compID = ComponentID(ch);
      if (rTu.ProcessComponentSection(compID) && (rTu.getRect(compID).width != rTu.getRect(compID).height))
      {
        offsetSubTUCBFs(rTu, compID); //the CBFs up to now have been defined for two sub-TUs - shift them down a level and replace with the parent level CBF
      }
    }

    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
    m_pcEntropyCoder->resetBits();

    if( uiLog2TrSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
    {
      m_pcEntropyCoder->encodeTransformSubdivFlag( 0, 5 - uiLog2TrSize );
    }

#ifdef SIG_BIT_EST
    if (g_sigdump.bit_est)
      g_sigpool.enable_bit_est = true;
#endif
#ifdef SIG_RRU
    g_sigpool.rru_is_record_si = true;
#endif
    for(UInt ch = 0; ch < numValidComp; ch++)
    {
      const UInt chOrderChange = ((ch + 1) == numValidComp) ? 0 : (ch + 1);
      const ComponentID compID=ComponentID(chOrderChange);
      if( rTu.ProcessComponentSection(compID) )
      {
        m_pcEntropyCoder->encodeQtCbf( rTu, compID, true );
      }
    }

    for(UInt ch = 0; ch < numValidComp; ch++)
    {
      const ComponentID compID=ComponentID(ch);
      if (rTu.ProcessComponentSection(compID))
      {
        if(isChroma(compID) && (uiAbsSum[COMPONENT_Y][0] != 0))
        {
          m_pcEntropyCoder->encodeCrossComponentPrediction( rTu, compID );
        }

        m_pcEntropyCoder->encodeCoeffNxN( rTu, pcCoeffCurr[compID], compID );
#ifdef SIG_BIT_EST
        if (g_sigdump.bit_est) {
          int pred_mode = pcCU->isIntra(uiAbsPartIdx) ? 0 : g_sigpool.is_merge ? 1 : 2;
          sig_bit_est_st *pbit_est;
          if (g_sigpool.cu_width < 32)
            pbit_est = &g_sigpool.bit_est_golden[pred_mode == 2 ? 1 : 0][pred_mode][compID];
          else
            pbit_est = &g_sigpool.bit_est_golden_32[0][rTu.GetSectionNumber()][compID];

          TCoeff* presi = pcCoeffCurr[compID];
          for (int i = 0; i < rTu.getRect(compID).height; i++)
            for (int j = 0; j < rTu.getRect(compID).width; j++) {
              pbit_est->Resi[i][j] = presi[i * rTu.getRect(compID).width + j];
            }
        }
#endif
        for (UInt subTUIndex = 0; subTUIndex < 2; subTUIndex++)
        {
          uiSingleDist += uiSingleDistComp[compID][subTUIndex];
        }
      }
    }
#ifdef SIG_BIT_EST
    if (g_sigdump.bit_est)
      g_sigpool.enable_bit_est = false;
#endif
#ifdef SIG_RRU
    g_sigpool.rru_is_record_si = false;
#endif
    uiSingleBits = m_pcEntropyCoder->getNumberOfWrittenBits();

    dSingleCost = m_pcRdCost->calcRdCost( uiSingleBits, uiSingleDist );
  } // check full

  // code sub-blocks
  if( bCheckSplit )
  {
    if( bCheckFull )
    {
      m_pcRDGoOnSbacCoder->store( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_TEST ] );
      m_pcRDGoOnSbacCoder->load ( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
    }
    Distortion uiSubdivDist = 0;
    UInt       uiSubdivBits = 0;
    Double     dSubdivCost = 0.0;

    //save the non-split CBFs in case we need to restore them later

    UInt bestCBF     [MAX_NUM_COMPONENT];
    UInt bestsubTUCBF[MAX_NUM_COMPONENT][2];
    for(UInt ch = 0; ch < numValidComp; ch++)
    {
      const ComponentID compID=ComponentID(ch);

      if (rTu.ProcessComponentSection(compID))
      {
        bestCBF[compID] = pcCU->getCbf(uiAbsPartIdx, compID, uiTrMode);

        const TComRectangle &tuCompRect = rTu.getRect(compID);
        if (tuCompRect.width != tuCompRect.height)
        {
          const UInt partIdxesPerSubTU = rTu.GetAbsPartIdxNumParts(compID) >> 1;

          for (UInt subTU = 0; subTU < 2; subTU++)
          {
            bestsubTUCBF[compID][subTU] = pcCU->getCbf ((uiAbsPartIdx + (subTU * partIdxesPerSubTU)), compID, subTUDepth);
          }
        }
      }
    }


    TComTURecurse tuRecurseChild(rTu, false);
    const UInt uiQPartNumSubdiv = tuRecurseChild.GetAbsPartIdxNumParts();

    DEBUG_STRING_NEW(sSplitString[MAX_NUM_COMPONENT])

    do
    {
      DEBUG_STRING_NEW(childString)
      xEstimateInterResidualQT( pcResi, dSubdivCost, uiSubdivBits, uiSubdivDist, bCheckFull ? NULL : puiZeroDist,  tuRecurseChild DEBUG_STRING_PASS_INTO(childString));
#if DEBUG_STRING
      // split the string by component and append to the relevant output (because decoder decodes in channel order, whereas this search searches by TU-order)
      std::size_t lastPos=0;
      const std::size_t endStrng=childString.find(debug_reorder_data_inter_token[MAX_NUM_COMPONENT], lastPos);
      for(UInt ch = 0; ch < numValidComp; ch++)
      {
        if (lastPos!=std::string::npos && childString.find(debug_reorder_data_inter_token[ch], lastPos)==lastPos)
        {
          lastPos+=strlen(debug_reorder_data_inter_token[ch]); // skip leading string
        }
        std::size_t pos=childString.find(debug_reorder_data_inter_token[ch+1], lastPos);
        if (pos!=std::string::npos && pos>endStrng)
        {
          lastPos=endStrng;
        }
        sSplitString[ch]+=childString.substr(lastPos, (pos==std::string::npos)? std::string::npos : (pos-lastPos) );
        lastPos=pos;
      }
#endif
    } while ( tuRecurseChild.nextSection(rTu) ) ;

    UInt uiCbfAny=0;
    for(UInt ch = 0; ch < numValidComp; ch++)
    {
      UInt uiYUVCbf = 0;
      for( UInt ui = 0; ui < 4; ++ui )
      {
        uiYUVCbf |= pcCU->getCbf( uiAbsPartIdx + ui * uiQPartNumSubdiv, ComponentID(ch),  uiTrMode + 1 );
      }
      UChar *pBase=pcCU->getCbf( ComponentID(ch) );
      const UInt flags=uiYUVCbf << uiTrMode;
      for( UInt ui = 0; ui < 4 * uiQPartNumSubdiv; ++ui )
      {
        pBase[uiAbsPartIdx + ui] |= flags;
      }
      uiCbfAny|=uiYUVCbf;
    }

    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_ROOT ] );
    m_pcEntropyCoder->resetBits();

    // when compID isn't a channel, code Cbfs:
    xEncodeInterResidualQT( MAX_NUM_COMPONENT, rTu );
    for(UInt ch = 0; ch < numValidComp; ch++)
    {
      xEncodeInterResidualQT( ComponentID(ch), rTu );
    }

    uiSubdivBits = m_pcEntropyCoder->getNumberOfWrittenBits();
    dSubdivCost  = m_pcRdCost->calcRdCost( uiSubdivBits, uiSubdivDist );

    if (!bCheckFull || (uiCbfAny && (dSubdivCost < dSingleCost)))
    {
      rdCost += dSubdivCost;
      ruiBits += uiSubdivBits;
      ruiDist += uiSubdivDist;
#if DEBUG_STRING
      for(UInt ch = 0; ch < numValidComp; ch++)
      {
        DEBUG_STRING_APPEND(sDebug, debug_reorder_data_inter_token[ch])
        DEBUG_STRING_APPEND(sDebug, sSplitString[ch])
      }
#endif
    }
    else
    {
      rdCost  += dSingleCost;
      ruiBits += uiSingleBits;
      ruiDist += uiSingleDist;

      //restore state to unsplit

      pcCU->setTrIdxSubParts( uiTrMode, uiAbsPartIdx, uiDepth );

      for(UInt ch = 0; ch < numValidComp; ch++)
      {
        const ComponentID compID=ComponentID(ch);

        DEBUG_STRING_APPEND(sDebug, debug_reorder_data_inter_token[ch])
        if (rTu.ProcessComponentSection(compID))
        {
          DEBUG_STRING_APPEND(sDebug, sSingleStringComp[compID])

          const Bool splitIntoSubTUs   = rTu.getRect(compID).width != rTu.getRect(compID).height;
          const UInt numberOfSections  = splitIntoSubTUs ? 2 : 1;
          const UInt partIdxesPerSubTU = rTu.GetAbsPartIdxNumParts(compID) >> (splitIntoSubTUs ? 1 : 0);

          for (UInt subTUIndex = 0; subTUIndex < numberOfSections; subTUIndex++)
          {
            const UInt  uisubTUPartIdx = uiAbsPartIdx + (subTUIndex * partIdxesPerSubTU);

            if (splitIntoSubTUs)
            {
              const UChar combinedCBF = (bestsubTUCBF[compID][subTUIndex] << subTUDepth) | (bestCBF[compID] << uiTrMode);
              pcCU->setCbfPartRange(combinedCBF, compID, uisubTUPartIdx, partIdxesPerSubTU);
            }
            else
            {
              pcCU->setCbfPartRange((bestCBF[compID] << uiTrMode), compID, uisubTUPartIdx, partIdxesPerSubTU);
            }

            pcCU->setCrossComponentPredictionAlphaPartRange(bestCrossCPredictionAlpha[compID][subTUIndex], compID, uisubTUPartIdx, partIdxesPerSubTU);
            pcCU->setTransformSkipPartRange(uiBestTransformMode[compID][subTUIndex], compID, uisubTUPartIdx, partIdxesPerSubTU);
            pcCU->setExplicitRdpcmModePartRange(bestExplicitRdpcmModeUnSplit[compID][subTUIndex], compID, uisubTUPartIdx, partIdxesPerSubTU);
          }
        }
      }

      m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[ uiDepth ][ CI_QT_TRAFO_TEST ] );
    }
  }
  else
  {
    rdCost  += dSingleCost;
    ruiBits += uiSingleBits;
    ruiDist += uiSingleDist;
#if DEBUG_STRING
    for(UInt ch = 0; ch < numValidComp; ch++)
    {
      const ComponentID compID=ComponentID(ch);
      DEBUG_STRING_APPEND(sDebug, debug_reorder_data_inter_token[compID])

      if (rTu.ProcessComponentSection(compID))
      {
        DEBUG_STRING_APPEND(sDebug, sSingleStringComp[compID])
      }
    }
#endif
  }
  DEBUG_STRING_APPEND(sDebug, debug_reorder_data_inter_token[MAX_NUM_COMPONENT])
}



Void TEncSearch::xEncodeInterResidualQT( const ComponentID compID, TComTU &rTu )
{
  TComDataCU* pcCU=rTu.getCU();
  const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU();
  const UInt uiCurrTrMode = rTu.GetTransformDepthRel();
  assert( pcCU->getDepth( 0 ) == pcCU->getDepth( uiAbsPartIdx ) );
  const UInt uiTrMode = pcCU->getTransformIdx( uiAbsPartIdx );

  const Bool bSubdiv = uiCurrTrMode != uiTrMode;

  const UInt uiLog2TrSize = rTu.GetLog2LumaTrSize();

  if (compID==MAX_NUM_COMPONENT)  // we are not processing a channel, instead we always recurse and code the CBFs
  {
    if( uiLog2TrSize <= pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() && uiLog2TrSize > pcCU->getQuadtreeTULog2MinSizeInCU(uiAbsPartIdx) )
    {
      if((pcCU->getSlice()->getSPS()->getQuadtreeTUMaxDepthInter() == 1) && (pcCU->getPartitionSize(uiAbsPartIdx) != SIZE_2Nx2N))
      {
        assert(bSubdiv); // Inferred splitting rule - see derivation and use of interSplitFlag in the specification.
      }
      else
      {
        m_pcEntropyCoder->encodeTransformSubdivFlag( bSubdiv, 5 - uiLog2TrSize );
      }
    }

    assert( !pcCU->isIntra(uiAbsPartIdx) );

    const Bool bFirstCbfOfCU = uiCurrTrMode == 0;

    for (UInt ch=COMPONENT_Cb; ch<pcCU->getPic()->getNumberValidComponents(); ch++)
    {
      const ComponentID compIdInner=ComponentID(ch);
      if( bFirstCbfOfCU || rTu.ProcessingAllQuadrants(compIdInner) )
      {
        if( bFirstCbfOfCU || pcCU->getCbf( uiAbsPartIdx, compIdInner, uiCurrTrMode - 1 ) )
        {
          m_pcEntropyCoder->encodeQtCbf( rTu, compIdInner, !bSubdiv );
        }
      }
      else
      {
        assert( pcCU->getCbf( uiAbsPartIdx, compIdInner, uiCurrTrMode ) == pcCU->getCbf( uiAbsPartIdx, compIdInner, uiCurrTrMode - 1 ) );
      }
    }

    if (!bSubdiv)
    {
      m_pcEntropyCoder->encodeQtCbf( rTu, COMPONENT_Y, true );
    }
  }

  if( !bSubdiv )
  {
    if (compID != MAX_NUM_COMPONENT) // we have already coded the CBFs, so now we code coefficients
    {
      if (rTu.ProcessComponentSection(compID))
      {
        if (isChroma(compID) && (pcCU->getCbf(uiAbsPartIdx, COMPONENT_Y, uiTrMode) != 0))
        {
          m_pcEntropyCoder->encodeCrossComponentPrediction(rTu, compID);
        }

        if (pcCU->getCbf(uiAbsPartIdx, compID, uiTrMode) != 0)
        {
          const UInt uiQTTempAccessLayer = pcCU->getSlice()->getSPS()->getQuadtreeTULog2MaxSize() - uiLog2TrSize;
          TCoeff *pcCoeffCurr = m_ppcQTTempCoeff[compID][uiQTTempAccessLayer] + rTu.getCoefficientOffset(compID);
          m_pcEntropyCoder->encodeCoeffNxN( rTu, pcCoeffCurr, compID );
        }
      }
    }
  }
  else
  {
    if( compID==MAX_NUM_COMPONENT || pcCU->getCbf( uiAbsPartIdx, compID, uiCurrTrMode ) )
    {
      TComTURecurse tuRecurseChild(rTu, false);
      do
      {
        xEncodeInterResidualQT( compID, tuRecurseChild );
      } while (tuRecurseChild.nextSection(rTu));
    }
  }
}




Void TEncSearch::xSetInterResidualQTData( TComYuv* pcResi, Bool bSpatial, TComTU &rTu ) // TODO: turn this into two functions for bSpatial=true and false.
{
  TComDataCU* pcCU=rTu.getCU();
  const UInt uiCurrTrMode=rTu.GetTransformDepthRel();
  const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU();
  assert( pcCU->getDepth( 0 ) == pcCU->getDepth( uiAbsPartIdx ) );
  const UInt uiTrMode = pcCU->getTransformIdx( uiAbsPartIdx );
  const TComSPS *sps=pcCU->getSlice()->getSPS();

  if( uiCurrTrMode == uiTrMode )
  {
    const UInt uiLog2TrSize = rTu.GetLog2LumaTrSize();
    const UInt uiQTTempAccessLayer = sps->getQuadtreeTULog2MaxSize() - uiLog2TrSize;

    if( bSpatial )
    {
      // Data to be copied is in the spatial domain, i.e., inverse-transformed.

      for(UInt i=0; i<pcResi->getNumberValidComponents(); i++)
      {
        const ComponentID compID=ComponentID(i);
        if (rTu.ProcessComponentSection(compID))
        {
          const TComRectangle &rectCompTU(rTu.getRect(compID));
          m_pcQTTempTComYuv[uiQTTempAccessLayer].copyPartToPartComponentMxN    ( compID, pcResi, rectCompTU );
        }
      }
    }
    else
    {
      for (UInt ch=0; ch < getNumberValidComponents(sps->getChromaFormatIdc()); ch++)
      {
        const ComponentID compID   = ComponentID(ch);
        if (rTu.ProcessComponentSection(compID))
        {
          const TComRectangle &rectCompTU(rTu.getRect(compID));
          const UInt numCoeffInBlock    = rectCompTU.width * rectCompTU.height;
          const UInt offset             = rTu.getCoefficientOffset(compID);
          TCoeff* dest                  = pcCU->getCoeff(compID)                        + offset;
          const TCoeff* src             = m_ppcQTTempCoeff[compID][uiQTTempAccessLayer] + offset;
          ::memcpy( dest, src, sizeof(TCoeff)*numCoeffInBlock );

#if ADAPTIVE_QP_SELECTION
          TCoeff* pcArlCoeffSrc            = m_ppcQTTempArlCoeff[compID][uiQTTempAccessLayer] + offset;
          TCoeff* pcArlCoeffDst            = pcCU->getArlCoeff(compID)                        + offset;
          ::memcpy( pcArlCoeffDst, pcArlCoeffSrc, sizeof( TCoeff ) * numCoeffInBlock );
#endif
        }
      }
    }
  }
  else
  {

    TComTURecurse tuRecurseChild(rTu, false);
    do
    {
      xSetInterResidualQTData( pcResi, bSpatial, tuRecurseChild );
    } while (tuRecurseChild.nextSection(rTu));
  }
}




UInt TEncSearch::xModeBitsIntra( TComDataCU* pcCU, UInt uiMode, UInt uiPartOffset, UInt uiDepth, const ChannelType chType )
{
  // Reload only contexts required for coding intra mode information
  m_pcRDGoOnSbacCoder->loadIntraDirMode( m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST], chType );

  // Temporarily set the intra dir being tested, and only
  // for absPartIdx, since encodeIntraDirModeLuma/Chroma only use
  // the entry at absPartIdx.

  UChar &rIntraDirVal=pcCU->getIntraDir( chType )[uiPartOffset];
  UChar origVal=rIntraDirVal;
  rIntraDirVal = uiMode;
  //pcCU->setIntraDirSubParts ( chType, uiMode, uiPartOffset, uiDepth + uiInitTrDepth );

  m_pcEntropyCoder->resetBits();
  if (isLuma(chType))
  {
    m_pcEntropyCoder->encodeIntraDirModeLuma ( pcCU, uiPartOffset);
  }
  else
  {
    m_pcEntropyCoder->encodeIntraDirModeChroma ( pcCU, uiPartOffset);
  }

  rIntraDirVal = origVal; // restore

  return m_pcEntropyCoder->getNumberOfWrittenBits();
}




UInt TEncSearch::xUpdateCandList( UInt uiMode, Double uiCost, UInt uiFastCandNum, UInt * CandModeList, Double * CandCostList )
{
  UInt i;
  UInt shift=0;

  while ( shift<uiFastCandNum && uiCost<CandCostList[ uiFastCandNum-1-shift ] )
  {
    shift++;
  }

  if( shift!=0 )
  {
    for(i=1; i<shift; i++)
    {
      CandModeList[ uiFastCandNum-i ] = CandModeList[ uiFastCandNum-1-i ];
      CandCostList[ uiFastCandNum-i ] = CandCostList[ uiFastCandNum-1-i ];
    }
    CandModeList[ uiFastCandNum-shift ] = uiMode;
    CandCostList[ uiFastCandNum-shift ] = uiCost;
    return 1;
  }

  return 0;
}





/** add inter-prediction syntax elements for a CU block
 * \param pcCU
 * \param uiQp
 * \param uiTrMode
 * \param ruiBits
 * \returns Void
 */
Void  TEncSearch::xAddSymbolBitsInter( TComDataCU* pcCU, UInt& ruiBits)
{
  if(pcCU->getMergeFlag( 0 ) && pcCU->getPartitionSize( 0 ) == SIZE_2Nx2N && !pcCU->getQtRootCbf( 0 ))
  {
    pcCU->setSkipFlagSubParts( true, 0, pcCU->getDepth(0) );

    m_pcEntropyCoder->resetBits();
    if(pcCU->getSlice()->getPPS()->getTransquantBypassEnabledFlag())
    {
      m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
    }
    m_pcEntropyCoder->encodeSkipFlag(pcCU, 0, true);
    m_pcEntropyCoder->encodeMergeIndex(pcCU, 0, true);
    ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();

#ifdef CVI_LC_RD_COST
    pcCU->getNoCoeffBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
    pcCU->getNoCoeffBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();

    //LC compare use no-skip bin cnt because HW dose not know the CBF in this stage.
    m_pcEntropyCoder->resetBits();
    if(pcCU->getSlice()->getPPS()->getTransquantBypassEnabledFlag())
    {
      m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
    }

    m_pcEntropyCoder->encodeSkipFlag ( pcCU, 0, true );
    m_pcEntropyCoder->encodePredMode( pcCU, 0, true );
    UInt64 ps_bits_frac = m_pcEntropyCoder->getNumberOfWrittenBitsFrac();
    m_pcEntropyCoder->encodePartSize( pcCU, 0, pcCU->getDepth(0), true );
    ps_bits_frac = m_pcEntropyCoder->getNumberOfWrittenBitsFrac() - ps_bits_frac;
    m_pcEntropyCoder->encodePredInfo( pcCU, 0 );

    // In LC cost, change use 1 bit to instead of fractional bits of part_size.
    pcCU->getLcNoCoeffBits() = ((m_pcEntropyCoder->getNumberOfWrittenBitsFrac() - ps_bits_frac) >> g_estBitFracBd) + 1;
    pcCU->getLcNoCoeffBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
#endif
  }
  else
  {
    m_pcEntropyCoder->resetBits();

    if(pcCU->getSlice()->getPPS()->getTransquantBypassEnabledFlag())
    {
      m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, 0, true);
    }
    m_pcEntropyCoder->encodeSkipFlag ( pcCU, 0, true );
    m_pcEntropyCoder->encodePredMode( pcCU, 0, true );
#ifdef CVI_LC_RD_COST
    UInt64 ps_bits_frac = m_pcEntropyCoder->getNumberOfWrittenBitsFrac();
    m_pcEntropyCoder->encodePartSize( pcCU, 0, pcCU->getDepth(0), true );
    ps_bits_frac = m_pcEntropyCoder->getNumberOfWrittenBitsFrac() - ps_bits_frac;
#else
    m_pcEntropyCoder->encodePartSize( pcCU, 0, pcCU->getDepth(0), true );
#endif
    m_pcEntropyCoder->encodePredInfo( pcCU, 0 );
#ifdef CVI_LC_RD_COST
    pcCU->getNoCoeffBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
    pcCU->getNoCoeffBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    // In LC cost, change to use 1 bit to instead of fractional bits of part_size.
    pcCU->getLcNoCoeffBits() = ((m_pcEntropyCoder->getNumberOfWrittenBitsFrac() - ps_bits_frac) >> g_estBitFracBd) + 1;
    pcCU->getLcNoCoeffBins() = pcCU->getNoCoeffBins();
#endif
    Bool codeDeltaQp = false;
    Bool codeChromaQpAdj = false;
    m_pcEntropyCoder->encodeCoeff   ( pcCU, 0, pcCU->getDepth(0), codeDeltaQp, codeChromaQpAdj );

    ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
  }
}





/**
 * \brief Generate half-sample interpolated block
 *
 * \param pattern Reference picture ROI
 * \param biPred    Flag indicating whether block is for biprediction
 */
Void TEncSearch::xExtDIFUpSamplingH( TComPattern* pattern )
{
  Int width      = pattern->getROIYWidth();
  Int height     = pattern->getROIYHeight();
  Int srcStride  = pattern->getPatternLStride();

  Int intStride = m_filteredBlockTmp[0].getStride(COMPONENT_Y);
  Int dstStride = m_filteredBlock[0][0].getStride(COMPONENT_Y);
  Pel *intPtr;
  Pel *dstPtr;
  Int filterSize = NTAPS_LUMA;
  Int halfFilterSize = (filterSize>>1);
  Pel *srcPtr = pattern->getROIY() - halfFilterSize*srcStride - 1;

  const ChromaFormat chFmt = m_filteredBlock[0][0].getChromaFormat();

  m_if.filterHor(COMPONENT_Y, srcPtr, srcStride, m_filteredBlockTmp[0].getAddr(COMPONENT_Y), intStride, width+1, height+filterSize, 0, false, chFmt, pattern->getBitDepthY());
  if (isEnableLC_INTERPOLATION())
    m_if.cvi_filter(false, true, false, srcPtr, srcStride, m_filteredBlockTmp[2].getAddr(COMPONENT_Y), intStride, width+1, height+filterSize, 2);
  else
    m_if.filterHor(COMPONENT_Y, srcPtr, srcStride, m_filteredBlockTmp[2].getAddr(COMPONENT_Y), intStride, width+1, height+filterSize, 2, false, chFmt, pattern->getBitDepthY());

  intPtr = m_filteredBlockTmp[0].getAddr(COMPONENT_Y) + halfFilterSize * intStride + 1;
  dstPtr = m_filteredBlock[0][0].getAddr(COMPONENT_Y);
  m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width+0, height+0, 0, false, true, chFmt, pattern->getBitDepthY());

  intPtr = m_filteredBlockTmp[0].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride + 1;
  dstPtr = m_filteredBlock[2][0].getAddr(COMPONENT_Y);
  if (isEnableLC_INTERPOLATION())
    m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width+0, height+1, 2);
  else
    m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width+0, height+1, 2, false, true, chFmt, pattern->getBitDepthY());

  intPtr = m_filteredBlockTmp[2].getAddr(COMPONENT_Y) + halfFilterSize * intStride;
  dstPtr = m_filteredBlock[0][2].getAddr(COMPONENT_Y);
  if (isEnableLC_INTERPOLATION())
    m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width+1, height+0, 0);
  else
    m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width+1, height+0, 0, false, true, chFmt, pattern->getBitDepthY());

  intPtr = m_filteredBlockTmp[2].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[2][2].getAddr(COMPONENT_Y);
  if (isEnableLC_INTERPOLATION())
    m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width+1, height+1, 2);
  else
    m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width+1, height+1, 2, false, true, chFmt, pattern->getBitDepthY());
}





/**
 * \brief Generate quarter-sample interpolated blocks
 *
 * \param pattern    Reference picture ROI
 * \param halfPelRef Half-pel mv
 * \param biPred     Flag indicating whether block is for biprediction
 */
Void TEncSearch::xExtDIFUpSamplingQ( TComPattern* pattern, TComMv halfPelRef )
{
  Int width      = pattern->getROIYWidth();
  Int height     = pattern->getROIYHeight();
  Int srcStride  = pattern->getPatternLStride();

  Pel *srcPtr;
  Int intStride = m_filteredBlockTmp[0].getStride(COMPONENT_Y);
  Int dstStride = m_filteredBlock[0][0].getStride(COMPONENT_Y);
  Pel *intPtr;
  Pel *dstPtr;
  Int filterSize = NTAPS_LUMA;

  Int halfFilterSize = (filterSize>>1);

  Int extHeight = (halfPelRef.getVer() == 0) ? height + filterSize : height + filterSize-1;

  const ChromaFormat chFmt = m_filteredBlock[0][0].getChromaFormat();

  // Horizontal filter 1/4
  srcPtr = pattern->getROIY() - halfFilterSize * srcStride - 1;
  intPtr = m_filteredBlockTmp[1].getAddr(COMPONENT_Y);
  if (halfPelRef.getVer() > 0)
  {
    srcPtr += srcStride;
  }
  if (halfPelRef.getHor() >= 0)
  {
    srcPtr += 1;
  }
  if (isEnableLC_INTERPOLATION())
    m_if.cvi_filter(false, true, false, srcPtr, srcStride, intPtr, intStride, width, extHeight, 1);
  else
    m_if.filterHor(COMPONENT_Y, srcPtr, srcStride, intPtr, intStride, width, extHeight, 1, false, chFmt, pattern->getBitDepthY());

  // Horizontal filter 3/4
  srcPtr = pattern->getROIY() - halfFilterSize*srcStride - 1;
  intPtr = m_filteredBlockTmp[3].getAddr(COMPONENT_Y);
  if (halfPelRef.getVer() > 0)
  {
    srcPtr += srcStride;
  }
  if (halfPelRef.getHor() > 0)
  {
    srcPtr += 1;
  }
  if (isEnableLC_INTERPOLATION())
    m_if.cvi_filter(false, true, false, srcPtr, srcStride, intPtr, intStride, width, extHeight, 3);
  else
    m_if.filterHor(COMPONENT_Y, srcPtr, srcStride, intPtr, intStride, width, extHeight, 3, false, chFmt, pattern->getBitDepthY());

  // Generate @ 1,1
  intPtr = m_filteredBlockTmp[1].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[1][1].getAddr(COMPONENT_Y);
  if (halfPelRef.getVer() == 0)
  {
    intPtr += intStride;
  }
  if (isEnableLC_INTERPOLATION())
    m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 1);
  else
    m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true, chFmt, pattern->getBitDepthY());

  // Generate @ 3,1
  intPtr = m_filteredBlockTmp[1].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[3][1].getAddr(COMPONENT_Y);
  if (isEnableLC_INTERPOLATION())
    m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 3);
  else
    m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true, chFmt, pattern->getBitDepthY());

  if (halfPelRef.getVer() != 0)
  {
    // Generate @ 2,1
    intPtr = m_filteredBlockTmp[1].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[2][1].getAddr(COMPONENT_Y);
    if (halfPelRef.getVer() == 0)
    {
      intPtr += intStride;
    }
    if (isEnableLC_INTERPOLATION())
      m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 2);
    else
      m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 2, false, true, chFmt, pattern->getBitDepthY());

    // Generate @ 2,3
    intPtr = m_filteredBlockTmp[3].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[2][3].getAddr(COMPONENT_Y);
    if (halfPelRef.getVer() == 0)
    {
      intPtr += intStride;
    }
    if (isEnableLC_INTERPOLATION())
      m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 2);
    else
      m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 2, false, true, chFmt, pattern->getBitDepthY());
  }
  else
  {
    // Generate @ 0,1
    intPtr = m_filteredBlockTmp[1].getAddr(COMPONENT_Y) + halfFilterSize * intStride;
    dstPtr = m_filteredBlock[0][1].getAddr(COMPONENT_Y);
    if (isEnableLC_INTERPOLATION())
      m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 0);
    else
      m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 0, false, true, chFmt, pattern->getBitDepthY());

    // Generate @ 0,3
    intPtr = m_filteredBlockTmp[3].getAddr(COMPONENT_Y) + halfFilterSize * intStride;
    dstPtr = m_filteredBlock[0][3].getAddr(COMPONENT_Y);
    if (isEnableLC_INTERPOLATION())
      m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 0);
    else
      m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 0, false, true, chFmt, pattern->getBitDepthY());
  }

  if (halfPelRef.getHor() != 0)
  {
    // Generate @ 1,2
    intPtr = m_filteredBlockTmp[2].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[1][2].getAddr(COMPONENT_Y);
    if (halfPelRef.getHor() > 0)
    {
      intPtr += 1;
    }
    if (halfPelRef.getVer() >= 0)
    {
      intPtr += intStride;
    }
    if (isEnableLC_INTERPOLATION())
      m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 1);
    else
      m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true, chFmt, pattern->getBitDepthY());

    // Generate @ 3,2
    intPtr = m_filteredBlockTmp[2].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride;
    dstPtr = m_filteredBlock[3][2].getAddr(COMPONENT_Y);
    if (halfPelRef.getHor() > 0)
    {
      intPtr += 1;
    }
    if (halfPelRef.getVer() > 0)
    {
      intPtr += intStride;
    }
    if (isEnableLC_INTERPOLATION())
      m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 3);
    else
      m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true, chFmt, pattern->getBitDepthY());
  }
  else
  {
    // Generate @ 1,0
    intPtr = m_filteredBlockTmp[0].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride + 1;
    dstPtr = m_filteredBlock[1][0].getAddr(COMPONENT_Y);
    if (halfPelRef.getVer() >= 0)
    {
      intPtr += intStride;
    }
    if (isEnableLC_INTERPOLATION())
      m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 1);
    else
      m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true, chFmt, pattern->getBitDepthY());

    // Generate @ 3,0
    intPtr = m_filteredBlockTmp[0].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride + 1;
    dstPtr = m_filteredBlock[3][0].getAddr(COMPONENT_Y);
    if (halfPelRef.getVer() > 0)
    {
      intPtr += intStride;
    }
    if (isEnableLC_INTERPOLATION())
      m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 3);
    else
      m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true, chFmt, pattern->getBitDepthY());
  }

  // Generate @ 1,3
  intPtr = m_filteredBlockTmp[3].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[1][3].getAddr(COMPONENT_Y);
  if (halfPelRef.getVer() == 0)
  {
    intPtr += intStride;
  }
  if (isEnableLC_INTERPOLATION())
    m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 1);
  else
    m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 1, false, true, chFmt, pattern->getBitDepthY());

  // Generate @ 3,3
  intPtr = m_filteredBlockTmp[3].getAddr(COMPONENT_Y) + (halfFilterSize-1) * intStride;
  dstPtr = m_filteredBlock[3][3].getAddr(COMPONENT_Y);
  if (isEnableLC_INTERPOLATION())
    m_if.cvi_filter(true, false, true, intPtr, intStride, dstPtr, dstStride, width, height, 3);
  else
    m_if.filterVer(COMPONENT_Y, intPtr, intStride, dstPtr, dstStride, width, height, 3, false, true, chFmt, pattern->getBitDepthY());
}





//! set wp tables
Void  TEncSearch::setWpScalingDistParam( TComDataCU* pcCU, Int iRefIdx, RefPicList eRefPicListCur )
{
  if ( iRefIdx<0 )
  {
    m_cDistParam.bApplyWeight = false;
    return;
  }

  TComSlice       *pcSlice  = pcCU->getSlice();
  WPScalingParam  *wp0 , *wp1;

  m_cDistParam.bApplyWeight = ( pcSlice->getSliceType()==P_SLICE && pcSlice->testWeightPred() ) || ( pcSlice->getSliceType()==B_SLICE && pcSlice->testWeightBiPred() ) ;

  if ( !m_cDistParam.bApplyWeight )
  {
    return;
  }

  Int iRefIdx0 = ( eRefPicListCur == REF_PIC_LIST_0 ) ? iRefIdx : (-1);
  Int iRefIdx1 = ( eRefPicListCur == REF_PIC_LIST_1 ) ? iRefIdx : (-1);

  getWpScaling( pcCU, iRefIdx0, iRefIdx1, wp0 , wp1 );

  if ( iRefIdx0 < 0 )
  {
    wp0 = NULL;
  }
  if ( iRefIdx1 < 0 )
  {
    wp1 = NULL;
  }

  m_cDistParam.wpCur  = NULL;

  if ( eRefPicListCur == REF_PIC_LIST_0 )
  {
    m_cDistParam.wpCur = wp0;
  }
  else
  {
    m_cDistParam.wpCur = wp1;
  }
}

//! \}
