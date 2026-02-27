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

/** \file     TEncCu.cpp
    \brief    Coding Unit (CU) encoder class
*/

#include <stdio.h>
#include "TEncTop.h"
#include "TEncCu.h"
#include "TEncAnalyze.h"
#include "TLibCommon/Debug.h"
#include "TLibCommon/cvi_sigdump.h"
#include "TLibCommon/cvi_algo_cfg.h"
#include "TLibCommon/cvi_cache.h"
#include "TLibCommon/cvi_frm_buf_mgr.h"
#include "cvi_enc.h"
#include <cmath>
#include <algorithm>
#include "TLibCommon/cvi_pattern.h"
#include "TLibCommon/cvi_ime.h"
#include "TLibCommon/cvi_motion.h"

using namespace std;
extern int diff_QP;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

/**
 \param    uhTotalDepth  total number of allowable depth
 \param    uiMaxWidth    largest CU width
 \param    uiMaxHeight   largest CU height
 \param    chromaFormat  chroma format
 */
Void TEncCu::create(UChar uhTotalDepth, UInt uiMaxWidth, UInt uiMaxHeight, ChromaFormat chromaFormat)
{
  Int i;

  m_uhTotalDepth   = uhTotalDepth + 1;
  m_ppcBestCU      = new TComDataCU*[m_uhTotalDepth-1];
  m_ppcTempCU      = new TComDataCU*[m_uhTotalDepth-1];

  m_ppcPredYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcResiYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcRecoYuvBest = new TComYuv*[m_uhTotalDepth-1];
  m_ppcPredYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcResiYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcRecoYuvTemp = new TComYuv*[m_uhTotalDepth-1];
  m_ppcOrigYuv     = new TComYuv*[m_uhTotalDepth-1];

  UInt uiNumPartitions;
  for( i=0 ; i<m_uhTotalDepth-1 ; i++)
  {
    uiNumPartitions = 1<<( ( m_uhTotalDepth - i - 1 )<<1 );
    UInt uiWidth  = uiMaxWidth  >> i;
    UInt uiHeight = uiMaxHeight >> i;

    m_ppcBestCU[i] = new TComDataCU; m_ppcBestCU[i]->create( chromaFormat, uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1) );
    m_ppcTempCU[i] = new TComDataCU; m_ppcTempCU[i]->create( chromaFormat, uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1) );

    m_ppcPredYuvBest[i] = new TComYuv; m_ppcPredYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcResiYuvBest[i] = new TComYuv; m_ppcResiYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcRecoYuvBest[i] = new TComYuv; m_ppcRecoYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);

    m_ppcPredYuvTemp[i] = new TComYuv; m_ppcPredYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcResiYuvTemp[i] = new TComYuv; m_ppcResiYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);
    m_ppcRecoYuvTemp[i] = new TComYuv; m_ppcRecoYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);

    m_ppcOrigYuv    [i] = new TComYuv; m_ppcOrigYuv    [i]->create(uiWidth, uiHeight, chromaFormat);
  }

  m_bEncodeDQP                     = false;
  m_stillToCodeChromaQpOffsetFlag  = false;
  m_cuChromaQpOffsetIdxPlus1       = 0;
  m_bFastDeltaQP                   = false;

  // initialize partition order.
  UInt* piTmp = &g_auiZscanToRaster[0];
  initZscanToRaster( m_uhTotalDepth, 1, 0, piTmp);
  initRasterToZscan( uiMaxWidth, uiMaxHeight, m_uhTotalDepth );

  // initialize conversion matrix from partition index to pel
  initRasterToPelXY( uiMaxWidth, uiMaxHeight, m_uhTotalDepth );
}

Void TEncCu::destroy()
{
  Int i;

  for( i=0 ; i<m_uhTotalDepth-1 ; i++)
  {
    if(m_ppcBestCU[i])
    {
      m_ppcBestCU[i]->destroy();      delete m_ppcBestCU[i];      m_ppcBestCU[i] = NULL;
    }
    if(m_ppcTempCU[i])
    {
      m_ppcTempCU[i]->destroy();      delete m_ppcTempCU[i];      m_ppcTempCU[i] = NULL;
    }
    if(m_ppcPredYuvBest[i])
    {
      m_ppcPredYuvBest[i]->destroy(); delete m_ppcPredYuvBest[i]; m_ppcPredYuvBest[i] = NULL;
    }
    if(m_ppcResiYuvBest[i])
    {
      m_ppcResiYuvBest[i]->destroy(); delete m_ppcResiYuvBest[i]; m_ppcResiYuvBest[i] = NULL;
    }
    if(m_ppcRecoYuvBest[i])
    {
      m_ppcRecoYuvBest[i]->destroy(); delete m_ppcRecoYuvBest[i]; m_ppcRecoYuvBest[i] = NULL;
    }
    if(m_ppcPredYuvTemp[i])
    {
      m_ppcPredYuvTemp[i]->destroy(); delete m_ppcPredYuvTemp[i]; m_ppcPredYuvTemp[i] = NULL;
    }
    if(m_ppcResiYuvTemp[i])
    {
      m_ppcResiYuvTemp[i]->destroy(); delete m_ppcResiYuvTemp[i]; m_ppcResiYuvTemp[i] = NULL;
    }
    if(m_ppcRecoYuvTemp[i])
    {
      m_ppcRecoYuvTemp[i]->destroy(); delete m_ppcRecoYuvTemp[i]; m_ppcRecoYuvTemp[i] = NULL;
    }
    if(m_ppcOrigYuv[i])
    {
      m_ppcOrigYuv[i]->destroy();     delete m_ppcOrigYuv[i];     m_ppcOrigYuv[i] = NULL;
    }
  }
  if(m_ppcBestCU)
  {
    delete [] m_ppcBestCU;
    m_ppcBestCU = NULL;
  }
  if(m_ppcTempCU)
  {
    delete [] m_ppcTempCU;
    m_ppcTempCU = NULL;
  }

  if(m_ppcPredYuvBest)
  {
    delete [] m_ppcPredYuvBest;
    m_ppcPredYuvBest = NULL;
  }
  if(m_ppcResiYuvBest)
  {
    delete [] m_ppcResiYuvBest;
    m_ppcResiYuvBest = NULL;
  }
  if(m_ppcRecoYuvBest)
  {
    delete [] m_ppcRecoYuvBest;
    m_ppcRecoYuvBest = NULL;
  }
  if(m_ppcPredYuvTemp)
  {
    delete [] m_ppcPredYuvTemp;
    m_ppcPredYuvTemp = NULL;
  }
  if(m_ppcResiYuvTemp)
  {
    delete [] m_ppcResiYuvTemp;
    m_ppcResiYuvTemp = NULL;
  }
  if(m_ppcRecoYuvTemp)
  {
    delete [] m_ppcRecoYuvTemp;
    m_ppcRecoYuvTemp = NULL;
  }
  if(m_ppcOrigYuv)
  {
    delete [] m_ppcOrigYuv;
    m_ppcOrigYuv = NULL;
  }
}

/** \param    pcEncTop      pointer of encoder class
 */
Void TEncCu::init( TEncTop* pcEncTop )
{
  m_pcEncCfg           = pcEncTop;
  m_pcPredSearch       = pcEncTop->getPredSearch();
  m_pcTrQuant          = pcEncTop->getTrQuant();
  m_pcRdCost           = pcEncTop->getRdCost();

  m_pcEntropyCoder     = pcEncTop->getEntropyCoder();
  m_pcBinCABAC         = pcEncTop->getBinCABAC();

  m_pppcRDSbacCoder    = pcEncTop->getRDSbacCoder();
  m_pcRDGoOnSbacCoder  = pcEncTop->getRDGoOnSbacCoder();

  m_pcRateCtrl         = pcEncTop->getRateCtrl();
  m_lumaQPOffset       = 0;
  initLumaDeltaQpLUT();
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 \param  pCtu pointer of CU data class
 */
Void TEncCu::compressCtu( TComDataCU* pCtu )
{
  // initialize CU data
  m_ppcBestCU[0]->initCtu( pCtu->getPic(), pCtu->getCtuRsAddr() );
  m_ppcTempCU[0]->initCtu( pCtu->getPic(), pCtu->getCtuRsAddr() );

  // analysis of CU
  DEBUG_STRING_NEW(sDebug)

#if defined(SIG_IRPU) || defined(SIG_IME_MVP)
    if (!pCtu->getSlice()->isIntra())
    {
      if (g_sigdump.irpu || g_sigdump.col)
        sigdump_col_mv(pCtu);
    }
#endif
  xCompressCU( m_ppcBestCU[0], m_ppcTempCU[0], 0 DEBUG_STRING_PASS_INTO(sDebug) );
  DEBUG_STRING_OUTPUT(std::cout, sDebug)

#if defined(SIG_IME_MVP)
    if (!pCtu->getSlice()->isIntra())
    {
      if (g_sigdump.ime_mvp)
        sigdump_nei_mvb(pCtu);
    }
#endif

#if ADAPTIVE_QP_SELECTION
  if( m_pcEncCfg->getUseAdaptQpSelect() )
  {
    if(pCtu->getSlice()->getSliceType()!=I_SLICE) //IIII
    {
      xCtuCollectARLStats( pCtu );
    }
  }
#endif
}
/** \param  pCtu  pointer of CU data class
 */
Void TEncCu::encodeCtu ( TComDataCU* pCtu )
{
  if ( pCtu->getSlice()->getPPS()->getUseDQP() )
  {
    setdQPFlag(true);
  }

  if ( pCtu->getSlice()->getUseChromaQpAdj() )
  {
    setCodeChromaQpAdjFlag(true);
  }

  // Encode CU data
  xEncodeCU( pCtu, 0, 0 );
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TEncCu::initLumaDeltaQpLUT()
{
  const LumaLevelToDeltaQPMapping &mapping=m_pcEncCfg->getLumaLevelToDeltaQPMapping();

  if ( !mapping.isEnabled() )
  {
    return;
  }

  // map the sparse LumaLevelToDeltaQPMapping.mapping to a fully populated linear table.

  Int         lastDeltaQPValue=0;
  std::size_t nextSparseIndex=0;
  for(Int index=0; index<LUMA_LEVEL_TO_DQP_LUT_MAXSIZE; index++)
  {
    while (nextSparseIndex < mapping.mapping.size() && index>=mapping.mapping[nextSparseIndex].first)
    {
      lastDeltaQPValue=mapping.mapping[nextSparseIndex].second;
      nextSparseIndex++;
    }
    m_lumaLevelToDeltaQPLUT[index]=lastDeltaQPValue;
  }
}

Int TEncCu::calculateLumaDQP(TComDataCU *pCU, const UInt absPartIdx, const TComYuv * pOrgYuv)
{
  const Pel *pY = pOrgYuv->getAddr(COMPONENT_Y, absPartIdx);
  const Int stride  = pOrgYuv->getStride(COMPONENT_Y);
  Int width = pCU->getWidth(absPartIdx);
  Int height = pCU->getHeight(absPartIdx);
  Double avg = 0;

  // limit the block by picture size
  const TComSPS* pSPS = pCU->getSlice()->getSPS();
  if ( pCU->getCUPelX() + width > pSPS->getPicWidthInLumaSamples() )
  {
    width = pSPS->getPicWidthInLumaSamples() - pCU->getCUPelX();
  }
  if ( pCU->getCUPelY() + height > pSPS->getPicHeightInLumaSamples() )
  {
    height = pSPS->getPicHeightInLumaSamples() - pCU->getCUPelY();
  }

  // Get QP offset derived from Luma level
  if ( m_pcEncCfg->getLumaLevelToDeltaQPMapping().mode == LUMALVL_TO_DQP_AVG_METHOD )
  {
    // Use avg method
    Int sum = 0;
    for (Int y = 0; y < height; y++)
    {
      for (Int x = 0; x < width; x++)
      {
        sum += pY[x];
      }
      pY += stride;
    }
    avg = (Double)sum/(width*height);
  }
  else
  {
    // Use maximum luma value
    Int maxVal = 0;
    for (Int y = 0; y < height; y++)
    {
      for (Int x = 0; x < width; x++)
      {
        if (pY[x] > maxVal)
        {
          maxVal = pY[x];
        }
      }
      pY += stride;
    }
    // use a percentage of the maxVal
    avg = (Double)maxVal * m_pcEncCfg->getLumaLevelToDeltaQPMapping().maxMethodWeight;
  }

  Int lumaIdx = Clip3<Int>(0, Int(LUMA_LEVEL_TO_DQP_LUT_MAXSIZE)-1, Int(avg+0.5) );
  Int QP = m_lumaLevelToDeltaQPLUT[lumaIdx];
  return QP;
}

//! Derive small set of test modes for AMP encoder speed-up
#if AMP_ENC_SPEEDUP
#if AMP_MRG
Void TEncCu::deriveTestModeAMP (TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver, Bool &bTestMergeAMP_Hor, Bool &bTestMergeAMP_Ver)
#else
Void TEncCu::deriveTestModeAMP (TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver)
#endif
{
  if ( pcBestCU->getPartitionSize(0) == SIZE_2NxN )
  {
    bTestAMP_Hor = true;
  }
  else if ( pcBestCU->getPartitionSize(0) == SIZE_Nx2N )
  {
    bTestAMP_Ver = true;
  }
  else if ( pcBestCU->getPartitionSize(0) == SIZE_2Nx2N && pcBestCU->getMergeFlag(0) == false && pcBestCU->isSkipped(0) == false )
  {
    bTestAMP_Hor = true;
    bTestAMP_Ver = true;
  }

#if AMP_MRG
  //! Utilizing the partition size of parent PU
  if ( eParentPartSize >= SIZE_2NxnU && eParentPartSize <= SIZE_nRx2N )
  {
    bTestMergeAMP_Hor = true;
    bTestMergeAMP_Ver = true;
  }

  if ( eParentPartSize == NUMBER_OF_PART_SIZES ) //! if parent is intra
  {
    if ( pcBestCU->getPartitionSize(0) == SIZE_2NxN )
    {
      bTestMergeAMP_Hor = true;
    }
    else if ( pcBestCU->getPartitionSize(0) == SIZE_Nx2N )
    {
      bTestMergeAMP_Ver = true;
    }
  }

  if ( pcBestCU->getPartitionSize(0) == SIZE_2Nx2N && pcBestCU->isSkipped(0) == false )
  {
    bTestMergeAMP_Hor = true;
    bTestMergeAMP_Ver = true;
  }

  if ( pcBestCU->getWidth(0) == 64 )
  {
    bTestAMP_Hor = false;
    bTestAMP_Ver = false;
  }
#else
  //! Utilizing the partition size of parent PU
  if ( eParentPartSize >= SIZE_2NxnU && eParentPartSize <= SIZE_nRx2N )
  {
    bTestAMP_Hor = true;
    bTestAMP_Ver = true;
  }

  if ( eParentPartSize == SIZE_2Nx2N )
  {
    bTestAMP_Hor = false;
    bTestAMP_Ver = false;
  }
#endif
}
#endif


// ====================================================================================================================
// Protected member functions
// ====================================================================================================================
/** Compress a CU block recursively with enabling sub-CTU-level delta QP
 *  - for loop of QP value to compress the current CU with all possible QP
*/
#if AMP_ENC_SPEEDUP
Void TEncCu::xCompressCU( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, const UInt uiDepth DEBUG_STRING_FN_DECLARE(sDebug_), PartSize eParentPartSize )
#else
Void TEncCu::xCompressCU( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, const UInt uiDepth )
#endif
{
  TComPic* pcPic = rpcBestCU->getPic();
  DEBUG_STRING_NEW(sDebug)
  const TComPPS &pps=*(rpcTempCU->getSlice()->getPPS());
  const TComSPS &sps=*(rpcTempCU->getSlice()->getSPS());

  // These are only used if getFastDeltaQp() is true
  const UInt fastDeltaQPCuMaxSize    = Clip3(sps.getMaxCUHeight()>>sps.getLog2DiffMaxMinCodingBlockSize(), sps.getMaxCUHeight(), 32u);

  // get Original YUV data from picture
  m_ppcOrigYuv[uiDepth]->copyFromPicYuv( pcPic->getPicYuvOrg(), rpcBestCU->getCtuRsAddr(), rpcBestCU->getZorderIdxInCtu() );

  // variable for Cbf fast mode PU decision
  Bool    doNotBlockPu = true;
  Bool    earlyDetectionSkipMode = false;

  const UInt uiLPelX   = rpcBestCU->getCUPelX();
  const UInt uiRPelX   = uiLPelX + rpcBestCU->getWidth(0)  - 1;
  const UInt uiTPelY   = rpcBestCU->getCUPelY();
  const UInt uiBPelY   = uiTPelY + rpcBestCU->getHeight(0) - 1;
  const UInt uiWidth   = rpcBestCU->getWidth(0);

  Int iBaseQP = xComputeQP( rpcBestCU, uiDepth );
  Int iMinQP;
  Int iMaxQP;
  Bool isAddLowestQP = false;

  const UInt numberValidComponents = rpcBestCU->getPic()->getNumberValidComponents();

#ifdef SIGDUMP
  g_sigpool.cu_idx_x = uiLPelX;
  g_sigpool.cu_idx_y = uiTPelY;
  g_sigpool.cu_width = rpcBestCU->getWidth(0);
  g_sigpool.cu_height = rpcBestCU->getHeight(0);
  Int best_inter_mode = 0;
#endif
#ifdef SIG_CCU
  Double totalRDcost_inter = 0.0;
  Double totalRDcost_inter_wt = 0.0;
#endif
#ifdef CVI_ALGO_CFG
#ifdef CVI_SEPERATE_MREGE_SKIP
  UInt BestMergeCand;
#endif
  Bool DisableCU64 = (uiWidth == 64) && g_algo_cfg.DisableCU64;
  Bool DisableAmvp32 = (uiWidth >= 32) && g_algo_cfg.DisableAmvp32;
  Bool DisableIntra4;
  Bool DisableIntra = false;

  if ((uiWidth >= 32) && g_algo_cfg.DisableIntra32)
    DisableIntra = true;
  if( rpcBestCU->getSlice()->getSliceType() != I_SLICE )
  {
    DisableIntra4 = (uiWidth == 8) && (g_algo_cfg.DisableIntra4 || g_algo_cfg.DisablePfrmIntra4);
    if (g_algo_cfg.DisablePfrmIntra)
      DisableIntra = true;
  }
  else
  {
    DisableIntra4 = (uiWidth == 8) && g_algo_cfg.DisableIntra4;
  }
#endif

  if( uiDepth <= pps.getMaxCuDQPDepth() )
  {
    Int idQP = m_pcEncCfg->getMaxDeltaQP();
    iMinQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP+idQP );
  }
  else
  {
    iMinQP = rpcTempCU->getQP(0);
    iMaxQP = rpcTempCU->getQP(0);
  }

  if ( m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() )
  {
    if ( uiDepth <= pps.getMaxCuDQPDepth() )
    {
      // keep using the same m_QP_LUMA_OFFSET in the same CTU
      m_lumaQPOffset = calculateLumaDQP(rpcTempCU, 0, m_ppcOrigYuv[uiDepth]);
    }
    iMinQP = Clip3(-sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP - m_lumaQPOffset);
    iMaxQP = iMinQP; // force encode choose the modified QO
  }

  if ( m_pcEncCfg->getUseRateCtrl() )
  {
#ifdef CVI_HW_RC
    if(m_pcEncCfg->getLCULevelRC()) {
      Int iRcQp = m_pcRateCtrl->getRCQP();
      if(rpcBestCU->getWidth(0)==32) {
        Double dsqrtLambda = g_cu32_sqrt_lambda.get_data(rpcBestCU->getCUPelX()%64, rpcBestCU->getCUPelY()%64);
        iRcQp = g_cu32_qp.get_data(rpcBestCU->getCUPelX()%64, rpcBestCU->getCUPelY()%64);
        m_pcRdCost->setLambda(g_cu32_lambda.get_data(rpcBestCU->getCUPelX()%64, rpcBestCU->getCUPelY()%64), dsqrtLambda, sps.getBitDepths());
      }
      else if(rpcBestCU->getWidth(0)==16) {
        iRcQp = g_blk_qp.get_data(rpcBestCU->getCUPelX()%64, rpcBestCU->getCUPelY()%64);
        Double dsqrtLambda = g_qp_to_sqrt_lambda_table[iRcQp];
        m_pcRdCost->setLambda(g_qp_to_lambda_table[iRcQp], dsqrtLambda, sps.getBitDepths());
      }
      m_pcRateCtrl->setRCQP(iRcQp);
      //aka add 20240111
      if(g_algo_cfg.EnableDeBreath == 1)
      if(( rpcBestCU->getSlice()->getSliceType() == I_SLICE ) && rpcBestCU->getSlice()->getPOC()>0)
      if(rpcBestCU->getWidth(0)==16 || rpcBestCU->getWidth(0)==32)
      {
        if(rpcBestCU->getCUPelX()==0 && rpcBestCU->getCUPelY()==0)
        {
          printf("diff_QP = %d\n",diff_QP);
        }
        if(rpcBestCU->getWidth(0)==32)
          iRcQp = g_cu32_qp.get_data(rpcBestCU->getCUPelX()%64, rpcBestCU->getCUPelY()%64);
        else if(rpcBestCU->getWidth(0)==16)
          iRcQp = g_blk_qp.get_data(rpcBestCU->getCUPelX()%64, rpcBestCU->getCUPelY()%64);
        int update_IQ = iRcQp + diff_QP/2;
        Double dsqrtLambda = g_qp_to_sqrt_lambda_table[update_IQ];
        if(rpcBestCU->getWidth(0)==32)
          m_pcRdCost->setLambda(g_cu32_lambda.get_data(rpcBestCU->getCUPelX()%64, rpcBestCU->getCUPelY()%64), dsqrtLambda, sps.getBitDepths());
        else if(rpcBestCU->getWidth(0)==16)
          m_pcRdCost->setLambda(g_qp_to_lambda_table[update_IQ], dsqrtLambda, sps.getBitDepths());
        m_pcRateCtrl->setRCQP(update_IQ);
      }
      //aka add 20240111~
    }
#endif
    iMinQP = m_pcRateCtrl->getRCQP();
    iMaxQP = m_pcRateCtrl->getRCQP();
  }

  // transquant-bypass (TQB) processing loop variable initialisation ---

  const Int lowestQP = iMinQP; // For TQB, use this QP which is the lowest non TQB QP tested (rather than QP'=0) - that way delta QPs are smaller, and TQB can be tested at all CU levels.

  if ( (pps.getTransquantBypassEnabledFlag()) )
  {
    isAddLowestQP = true; // mark that the first iteration is to cost TQB mode.
    iMinQP = iMinQP - 1;  // increase loop variable range by 1, to allow testing of TQB mode along with other QPs
    if ( m_pcEncCfg->getCUTransquantBypassFlagForceValue() )
    {
      iMaxQP = iMinQP;
    }
  }

#ifdef CVI_ENABLE_RDO_BIT_EST
  if(uiWidth==32 && getRDOSyntaxEstMode()!=HM_ORIGINAL)
  {
    if (uiLPelX == 0 && uiTPelY == 0)
    {
      m_pppcRDSbacCoder[uiDepth][CI_CVI_CU32]->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
    }
    m_pppcRDSbacCoder[uiDepth][CI_CVI_CU32]->cxtScalingSnapshot_delay();
    m_pppcRDSbacCoder[uiDepth][CI_CVI_CU32]->cxtScalingSnapshot(rpcTempCU, uiDepth);
    m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_CVI_CU32]);

#ifdef SIG_CCU
    if (g_sigdump.ccu)
      sig_ccu_copy_cbf_ctx();
#endif //~SIG_CCU
  }
#endif

  TComSlice * pcSlice = rpcTempCU->getPic()->getSlice(rpcTempCU->getPic()->getCurrSliceIdx());

  const Bool bBoundary = !( uiRPelX < sps.getPicWidthInLumaSamples() && uiBPelY < sps.getPicHeightInLumaSamples() );

#ifdef CVI_ALGO_CFG
  if ( !bBoundary && !DisableCU64)
#else
  if ( !bBoundary )
#endif
  {
#ifdef SIGDUMP
    sigdump_cu_start();
#endif
    for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
    {
      const Bool bIsLosslessMode = isAddLowestQP && (iQP == iMinQP);

      if (bIsLosslessMode)
      {
        iQP = lowestQP;
      }
      if ( m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() && uiDepth <= pps.getMaxCuDQPDepth() )
      {
        getSliceEncoder()->updateLambda(pcSlice, iQP);
      }

      m_cuChromaQpOffsetIdxPlus1 = 0;
      if (pcSlice->getUseChromaQpAdj())
      {
        /* Pre-estimation of chroma QP based on input block activity may be performed
         * here, using for example m_ppcOrigYuv[uiDepth] */
        /* To exercise the current code, the index used for adjustment is based on
         * block position
         */
        Int lgMinCuSize = sps.getLog2MinCodingBlockSize() +
                          std::max<Int>(0, sps.getLog2DiffMaxMinCodingBlockSize()-Int(pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth()));
        m_cuChromaQpOffsetIdxPlus1 = ((uiLPelX >> lgMinCuSize) + (uiTPelY >> lgMinCuSize)) % (pps.getPpsRangeExtension().getChromaQpOffsetListLen() + 1);
      }

      rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

#if defined(SIG_CCU) || defined(SIG_BIT_EST)
      if (g_sigdump.ccu || g_sigdump.rdo_sse) {
        sigdump_ccu_qp(rpcTempCU, uiWidth, m_pcRdCost->getFixpointFracLambda(), m_pcRdCost->getFixpointSADSqrtLambda());

        int blk_size = sig_pat_blk_size_4(uiWidth);
        for (int i = 0; i < 2; i++) {
          for (int j = 0; j < 4; j++) {
            g_sigpool.est_golden[blk_size][i].EST_BIT[j] = 0;
            for (int compID = 0; compID < 3; compID++) {
              g_sigpool.est_golden[blk_size][i].EST_SSE[compID][j] = 0;
              memset(g_sigpool.est_golden[blk_size][i].TCoeff[compID][j], 0x0, sizeof(int) * 256);
              memset(g_sigpool.est_golden[blk_size][i].IQCoeff[compID][j], 0x0, sizeof(int) * 256);
            }
          }
          g_sigpool.est_golden[blk_size][i].EST_BAC = 0;
        }
      }
#endif
#ifdef SIG_RRU
      if (g_sigdump.rru)
      {
        for (int i = 0; i < rpcTempCU->getPic()->getNumberValidComponents(); i++)
        {
          const QpParam cQP(*rpcTempCU, ComponentID(i));
          g_sigpool.rru_qp[i] = cQP.Qp;
        }
      }
#endif
      // do inter modes, SKIP and 2Nx2N
      if( rpcBestCU->getSlice()->getSliceType() != I_SLICE )
      {
        // 2Nx2N
        if(m_pcEncCfg->getUseEarlySkipDetection())
        {
          xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug) );
          rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );//by Competition for inter_2Nx2N
        }
#ifdef CVI_SEPERATE_MREGE_SKIP
#ifdef SIGDUMP
        g_sigpool.is_merge = 1;
#endif
        // Merge mode only
        xCheckRDCostMerge2Nx2N_CVI( rpcBestCU, rpcTempCU DEBUG_STRING_PASS_INTO(sDebug), &earlyDetectionSkipMode, BestMergeCand);//by Merge for inter_2Nx2N
#ifdef SIGDUMP
        g_sigpool.is_merge = 0;
#endif
        rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
        // Skip mode only
        //xCheckRDCostSkip2Nx2N_CVI( rpcBestCU, rpcTempCU DEBUG_STRING_PASS_INTO(sDebug), &earlyDetectionSkipMode );//by Skip for inter_2Nx2N
        //rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#else
        // SKIP
        xCheckRDCostMerge2Nx2N( rpcBestCU, rpcTempCU DEBUG_STRING_PASS_INTO(sDebug), &earlyDetectionSkipMode );//by Merge for inter_2Nx2N
        rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
#endif
#ifdef CVI_ALGO_CFG
        if(!m_pcEncCfg->getUseEarlySkipDetection() && !DisableAmvp32)
#else
        if(!m_pcEncCfg->getUseEarlySkipDetection())
#endif
        {
          // 2Nx2N, NxN
          xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug) );
          rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
          if(m_pcEncCfg->getUseCbfFastMode())
          {
            doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
          }
        }
#ifdef SIG_RRU
        g_sigpool.rru_is_mrg_win = rpcBestCU->getMergeFlag(0);
        if (g_sigpool.rru_is_mrg_win)
          g_sigpool.rru_merge_cand = rpcBestCU->getMergeIndex(0);
#endif //~SIG_RRU

#ifdef CVI_HW_RC
        if (uiWidth == 16)
        {
          // use SATD as madp
          int madp = (int)(rpcBestCU->getTotalLcSADCost());
          g_blk_madp.set_data(uiLPelX, uiTPelY, madp);
          g_pic_madp_accum += madp;
        }
      }
#endif

      if (bIsLosslessMode) // Restore loop variable if lossless mode was searched.
      {
        iQP = iMinQP;
      }
    }

    if(!earlyDetectionSkipMode)
    {
      for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
      {
        const Bool bIsLosslessMode = isAddLowestQP && (iQP == iMinQP); // If lossless, then iQP is irrelevant for subsequent modules.
        if (bIsLosslessMode)
        {
          iQP = lowestQP;
        }

        rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

        // do inter modes, NxN, 2NxN, and Nx2N
        if( rpcBestCU->getSlice()->getSliceType() != I_SLICE )
        {
          // 2Nx2N, NxN
          if(!( (rpcBestCU->getWidth(0)==8) && (rpcBestCU->getHeight(0)==8) ))
          {
            if( uiDepth == sps.getLog2DiffMaxMinCodingBlockSize() && doNotBlockPu)
            {
              xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_NxN DEBUG_STRING_PASS_INTO(sDebug)   );
              rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
            }
          }
#ifndef CVI_ALGO_CFG
          if(doNotBlockPu)
          {
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_Nx2N DEBUG_STRING_PASS_INTO(sDebug)  );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
            if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_Nx2N )
            {
              doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
            }
          }

          if(doNotBlockPu)
          {
            xCheckRDCostInter      ( rpcBestCU, rpcTempCU, SIZE_2NxN DEBUG_STRING_PASS_INTO(sDebug)  );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
            if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxN)
            {
              doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
            }
          }

          //! Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
          if(sps.getUseAMP() && uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() )
          {
#if AMP_ENC_SPEEDUP
            Bool bTestAMP_Hor = false, bTestAMP_Ver = false;

#if AMP_MRG
            Bool bTestMergeAMP_Hor = false, bTestMergeAMP_Ver = false;

            deriveTestModeAMP (rpcBestCU, eParentPartSize, bTestAMP_Hor, bTestAMP_Ver, bTestMergeAMP_Hor, bTestMergeAMP_Ver);
#else
            deriveTestModeAMP (rpcBestCU, eParentPartSize, bTestAMP_Hor, bTestAMP_Ver);
#endif

            //! Do horizontal AMP
            if ( bTestAMP_Hor )
            {
              if(doNotBlockPu)
              {
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU DEBUG_STRING_PASS_INTO(sDebug) );
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
              if(doNotBlockPu)
              {
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD DEBUG_STRING_PASS_INTO(sDebug) );
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
            }
#if AMP_MRG
            else if ( bTestMergeAMP_Hor )
            {
              if(doNotBlockPu)
              {
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU DEBUG_STRING_PASS_INTO(sDebug), true );
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
              if(doNotBlockPu)
              {
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD DEBUG_STRING_PASS_INTO(sDebug), true );
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
            }
#endif

            //! Do horizontal AMP
            if ( bTestAMP_Ver )
            {
              if(doNotBlockPu)
              {
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N DEBUG_STRING_PASS_INTO(sDebug) );
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
              if(doNotBlockPu)
              {
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N DEBUG_STRING_PASS_INTO(sDebug) );
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
              }
            }
#if AMP_MRG
            else if ( bTestMergeAMP_Ver )
            {
              if(doNotBlockPu)
              {
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N DEBUG_STRING_PASS_INTO(sDebug), true );
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
                if(m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N )
                {
                  doNotBlockPu = rpcBestCU->getQtRootCbf( 0 ) != 0;
                }
              }
              if(doNotBlockPu)
              {
                xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N DEBUG_STRING_PASS_INTO(sDebug), true );
                rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
              }
            }
#endif

#else
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnU );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_2NxnD );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nLx2N );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

            xCheckRDCostInter( rpcBestCU, rpcTempCU, SIZE_nRx2N );
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

#endif
          }
#endif // ~!CVI_ALGO_CFG
        }
#ifdef SIGDUMP
        best_inter_mode = rpcBestCU->getMergeFlag(0) ? 1 : 2;
#endif

#ifdef SIG_CCU
        totalRDcost_inter = rpcBestCU->getTotalCost();
        totalRDcost_inter_wt = rpcBestCU->getTotalCost();
#endif
#ifdef CVI_CU_PENALTY
        if (rpcTempCU->getSlice()->getSliceType() != I_SLICE &&
            (g_algo_cfg.CostPenaltyCfg.EnableCuCost || g_algo_cfg.CostPenaltyCfg.EnableForeground))
        {
          Double &cost = rpcBestCU->getTotalCost();
          int cu_width = rpcBestCU->getWidth(0);
          int cu_x = rpcBestCU->getCUPelX();
          int cu_y = rpcBestCU->getCUPelY();
#ifdef CVI_SPLIT_COST_WEIGHT
          UInt bits = rpcBestCU->getTotalBits();
          Distortion dist = rpcBestCU->getTotalDistortion();
          UInt64 lambda = m_pcRdCost->getFixpointFracLambda();
          calcCuPenaltyCostSplit(CU_PENALTY_INTER, cost, bits, dist, lambda,
                                 cu_width, cu_x, cu_y, false);
#else
          calcCuPenaltyCost(CU_PENALTY_INTER, cost, cu_width, cu_x, cu_y, false);
#endif
#ifdef SIG_CCU
          totalRDcost_inter_wt = rpcBestCU->getTotalCost();
#endif
        }
#endif //~CVI_CU_PENALTY

        // do normal intra modes
        // speedup for inter frames
#if MCTS_ENC_CHECK
        if ( m_pcEncCfg->getTMCTSSEITileConstraint() || (rpcBestCU->getSlice()->getSliceType() == I_SLICE) ||
             ((!m_pcEncCfg->getDisableIntraPUsInInterSlices()) && (
             (rpcBestCU->getCbf(0, COMPONENT_Y) != 0) ||
             ((rpcBestCU->getCbf(0, COMPONENT_Cb) != 0) && (numberValidComponents > COMPONENT_Cb)) ||
             ((rpcBestCU->getCbf(0, COMPONENT_Cr) != 0) && (numberValidComponents > COMPONENT_Cr))  // avoid very complex intra if it is unlikely
            )))
        {
#else
#ifdef CVI_DISABLE_INTRA_CBF
        if((rpcBestCU->getSlice()->getSliceType() == I_SLICE)                                        ||
            (!m_pcEncCfg->getDisableIntraPUsInInterSlices()))
#else
        if((rpcBestCU->getSlice()->getSliceType() == I_SLICE)                                        ||
            ((!m_pcEncCfg->getDisableIntraPUsInInterSlices()) && (
              (rpcBestCU->getCbf( 0, COMPONENT_Y  ) != 0)                                            ||
             ((rpcBestCU->getCbf( 0, COMPONENT_Cb ) != 0) && (numberValidComponents > COMPONENT_Cb)) ||
             ((rpcBestCU->getCbf( 0, COMPONENT_Cr ) != 0) && (numberValidComponents > COMPONENT_Cr))  // avoid very complex intra if it is unlikely
            )))
#endif
        {
#endif

#ifdef CVI_ALGO_CFG
          if (!DisableIntra)
#endif
          {
#ifdef SIGDUMP
            g_sigpool.is_intra = 1;
#endif
            xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug) );

            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
            if( uiDepth == sps.getLog2DiffMaxMinCodingBlockSize() )
            {
              if( rpcTempCU->getWidth(0) > ( 1 << sps.getQuadtreeTULog2MinSize() ) )
              {
#ifdef CVI_ALGO_CFG
#ifdef CVI_FAST_CU_ENC
                if (!DisableIntra4) {
                  DisableIntra4 |= ccu_i4_term_decision(uiLPelX, uiTPelY);
                }

                if (DisableIntra4)
                {
#ifdef SIG_RRU
                  if (g_sigdump.rru) {
                    g_sigpool.rru_intra_cost[0] = MAX_DOUBLE;
                    sig_rru_output_i4term_temp_data();
                  }
#endif //~SIG_RRU
#ifdef SIG_IAPU
                  if (g_sigdump.iapu) {
                    sig_iap_output_i4term_temp_data();
                  }
#endif //~SIG_IAPU
#ifdef SIG_CCU
                  if (g_sigdump.ccu) {
                    sig_ccu_output_i4term_temp_data();
                  }
#endif //~SIG_CCU
                }
#endif //~CVI_FAST_CU_ENC

#ifdef SIG_PRU
                if (g_sigdump.pru)
                {
                  g_sigpool.p_pru_st->disable_i4 = DisableIntra4;
                  if (DisableIntra4 == true)
                    sig_pru_set_edge_info_i4_termination(uiLPelX, uiTPelY);

                  int idx = (((uiTPelY & 0x3f) >> 4) << 2) + ((uiLPelX & 0x3f) >> 4);
                  g_sigpool.p_pru_st->stat_early_term[idx] = g_picTermI4Cnt;
                }
#endif //~SIG_PRU

                if (!(rpcBestCU->getWidth(0) == 8 && DisableIntra4))
#endif
                {
#ifdef SIG_CCU
                  if (g_sigdump.ccu) {  //blk 4x4
                    sigdump_output_fprint(&g_sigpool.ccu_ctx[0], "# CU (x, y) = (%d, %d)\n",
                      rpcBestCU->getCUPelX(), rpcBestCU->getCUPelY());
                    sigdump_ccu_qp(rpcTempCU, 4, m_pcRdCost->getFixpointFracLambda(), m_pcRdCost->getFixpointSADSqrtLambda());
                  }
#endif
                  xCheckRDCostIntra( rpcBestCU, rpcTempCU, SIZE_NxN DEBUG_STRING_PASS_INTO(sDebug)   );

#ifdef SIG_CCU
                  if (g_sigdump.ccu)
                  {
                    sig_ctx *p_ctx = &g_sigpool.ccu_ctx[0];
                    sig_ccu_output_cu_golden_4x4();
                    sigdump_ccu_md_4x4(rpcBestCU);
                    sig_rru_output_cu_self_info(p_ctx, 4);
                    sigdump_ccu_rd_cost(rpcBestCU, 4, uiDepth, 0, 0);
                    sigdump_output_fprint(p_ctx, "is_comp_split_intra_win = %d\n", 0);
                    sigdump_output_fprint(p_ctx, "is_win = %d\n", 0);
                  }
#endif
                  rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
                }
              }
            }
#ifdef SIGDUMP
            g_sigpool.is_intra = 0;
#endif
          }
        }

#ifdef SIG_BIT_EST
        if (g_sigdump.bit_est || g_sigdump.rdo_sse) {
          if( rpcBestCU->getSlice()->getSliceType() != I_SLICE) {
            if (g_sigdump.bit_est)
              sigdump_bit_est(g_aucConvertToBit[g_sigpool.cu_width], best_inter_mode, 3);
            if (g_sigdump.rdo_sse)
              sigdump_output_rdo_sse_inter(g_aucConvertToBit[g_sigpool.cu_width]);
          }
        }
#endif
#ifdef CVI_SEPERATE_MREGE_SKIP
        if( rpcBestCU->getSlice()->getSliceType() != I_SLICE )
        {
          // Skip mode only
          xCheckRDCostSkip2Nx2N_CVI( rpcBestCU, rpcTempCU DEBUG_STRING_PASS_INTO(sDebug), &earlyDetectionSkipMode, BestMergeCand);//by Skip for inter_2Nx2N
#ifdef SIG_CCU
          if (totalRDcost_inter_wt > g_sigpool.skip_rd_cost_wt)
          {
            totalRDcost_inter    = g_sigpool.skip_rd_cost;
            totalRDcost_inter_wt = g_sigpool.skip_rd_cost_wt;
          }
#endif
          rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
        }
#endif
        // test PCM
        if(sps.getUsePCM()
          && rpcTempCU->getWidth(0) <= (1<<sps.getPCMLog2MaxSize())
          && rpcTempCU->getWidth(0) >= (1<<sps.getPCMLog2MinSize()) )
        {
          UInt uiRawBits = getTotalBits(rpcBestCU->getWidth(0), rpcBestCU->getHeight(0), rpcBestCU->getPic()->getChromaFormat(), sps.getBitDepths().recon);
          UInt uiBestBits = rpcBestCU->getTotalBits();
          if((uiBestBits > uiRawBits) || (rpcBestCU->getTotalCost() > m_pcRdCost->calcRdCost(uiRawBits, 0)))
          {
            xCheckIntraPCM (rpcBestCU, rpcTempCU);
            rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
          }
        }

        if (bIsLosslessMode) // Restore loop variable if lossless mode was searched.
        {
          iQP = iMinQP;
        }
      }
    }

    if( rpcBestCU->getTotalCost()!=MAX_DOUBLE )
    {
      m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);
#ifdef CVI_HW_RC
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeSplitFlag( rpcBestCU, 0, uiDepth, true );
      Int splitBits = m_pcEntropyCoder->getNumberOfWrittenBits();
      Double splitBitsCost = m_pcRdCost->calcRdCost( splitBits, 0 );
      rpcBestCU->getTotalCost() += splitBitsCost;
 //aka set 20240906
  if(rpcBestCU->getWidth(0)==32 && g_algo_cfg.DisableCU32)
    rpcBestCU->getTotalCost() = MAX_DOUBLE;

  //aka set 20240906~
#else
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeSplitFlag( rpcBestCU, 0, uiDepth, true );
      rpcBestCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
      rpcBestCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
      rpcBestCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcBestCU->getTotalBits(), rpcBestCU->getTotalDistortion() );
#endif
      m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);
    }

#ifdef SIG_RRU
    if (g_sigdump.rru)
    {
      // Output RRU CU cmd and golden
      int cu_width = g_sigpool.cu_width;
      sig_rru_output_cu_golden(cu_width, rpcBestCU->getSlice()->getSliceType(), m_ppcRecoYuvBest[uiDepth]);
      if (g_sigdump.ccu)
        sigdump_ccu_md(rpcBestCU, totalRDcost_inter_wt == g_sigpool.skip_rd_cost_wt);
      sig_rru_output_cu_cmd(rpcBestCU, cu_width);
    }
#endif
#if defined(SIG_CCU)
    if (g_sigdump.ccu) {
      sigdump_ccu_rd_cost(rpcBestCU, rpcBestCU->getWidth(0), uiDepth, totalRDcost_inter, totalRDcost_inter_wt);
    }
#endif
#ifdef SIG_CCU
    if (g_sigdump.ccu && rpcBestCU->getWidth(0) == 8) {
      sigdump_ccu_ime_mvp(g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
    }
#endif
  }

  // copy original YUV samples to PCM buffer
  if( rpcBestCU->getTotalCost()!=MAX_DOUBLE && rpcBestCU->isLosslessCoded(0) && (rpcBestCU->getIPCMFlag(0) == false))
  {
    xFillPCMBuffer(rpcBestCU, m_ppcOrigYuv[uiDepth]);
  }

  if( uiDepth == pps.getMaxCuDQPDepth() )
  {
    Int idQP = m_pcEncCfg->getMaxDeltaQP();
    iMinQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP-idQP );
    iMaxQP = Clip3( -sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP+idQP );
  }
  else if( uiDepth < pps.getMaxCuDQPDepth() )
  {
    iMinQP = iBaseQP;
    iMaxQP = iBaseQP;
  }
  else
  {
    const Int iStartQP = rpcTempCU->getQP(0);
    iMinQP = iStartQP;
    iMaxQP = iStartQP;
  }

  if ( m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() )
  {
    iMinQP = Clip3(-sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP - m_lumaQPOffset);
    iMaxQP = iMinQP;
  }

  if ( m_pcEncCfg->getUseRateCtrl() )
  {
    iMinQP = m_pcRateCtrl->getRCQP();
    iMaxQP = m_pcRateCtrl->getRCQP();
  }

  if ( m_pcEncCfg->getCUTransquantBypassFlagForceValue() )
  {
    iMaxQP = iMinQP; // If all TUs are forced into using transquant bypass, do not loop here.
  }

  const Bool bSubBranch = bBoundary || !( m_pcEncCfg->getUseEarlyCU() && rpcBestCU->getTotalCost()!=MAX_DOUBLE && rpcBestCU->isSkipped(0) );

  if( bSubBranch && uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() && (!getFastDeltaQp() || uiWidth > fastDeltaQPCuMaxSize || bBoundary))
  {
    // further split
    Double splitTotalCost = 0;

    for (Int iQP=iMinQP; iQP<=iMaxQP; iQP++)
    {
      const Bool bIsLosslessMode = false; // False at this level. Next level down may set it to true.

      rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );

      UChar       uhNextDepth         = uiDepth+1;
      TComDataCU* pcSubBestPartCU     = m_ppcBestCU[uhNextDepth];
      TComDataCU* pcSubTempPartCU     = m_ppcTempCU[uhNextDepth];
      DEBUG_STRING_NEW(sTempDebug)
#ifdef CVI_HW_RC
      Double depthLambda = m_pcRdCost->getLambda();
      Double depthsqrtLambda = m_pcRdCost->getSqrtLambda();
#endif

      for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++ )
      {
        pcSubBestPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.
        pcSubTempPartCU->initSubCU( rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP );           // clear sub partition datas or init.

        if( ( pcSubBestPartCU->getCUPelX() < sps.getPicWidthInLumaSamples() ) && ( pcSubBestPartCU->getCUPelY() < sps.getPicHeightInLumaSamples() ) )
        {
          if ( 0 == uiPartUnitIdx) //initialize RD with previous depth buffer
          {
            m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
          }
          else
          {
            m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
          }

#if AMP_ENC_SPEEDUP
          DEBUG_STRING_NEW(sChild)
          if ( !(rpcBestCU->getTotalCost()!=MAX_DOUBLE && rpcBestCU->isInter(0)) )
          {
            xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth DEBUG_STRING_PASS_INTO(sChild), NUMBER_OF_PART_SIZES );
          }
          else
          {

            xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth DEBUG_STRING_PASS_INTO(sChild), rpcBestCU->getPartitionSize(0) );
          }
          DEBUG_STRING_APPEND(sTempDebug, sChild)
#else
          xCompressCU( pcSubBestPartCU, pcSubTempPartCU, uhNextDepth );
#endif

          rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );         // Keep best part data to current temporary data.
          xCopyYuv2Tmp( pcSubBestPartCU->getTotalNumPart()*uiPartUnitIdx, uhNextDepth );

#ifndef CVI_HW_RC
          if ((m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() && pps.getMaxCuDQPDepth() >= 1))
#endif
          {
            splitTotalCost += pcSubBestPartCU->getTotalCost();
          }
        }
        else
        {
          pcSubBestPartCU->copyToPic( uhNextDepth );
          rpcTempCU->copyPartFrom( pcSubBestPartCU, uiPartUnitIdx, uhNextDepth );
        }
      }

      m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
      if( !bBoundary )
      {
#ifdef CVI_HW_RC
        if (!DisableCU64) {
          m_pcRdCost->setLambda(depthLambda, depthsqrtLambda, sps.getBitDepths());
        }
#endif
        m_pcEntropyCoder->resetBits();
        m_pcEntropyCoder->encodeSplitFlag( rpcTempCU, 0, uiDepth, true );

#ifndef CVI_HW_RC
        if ((m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() && pps.getMaxCuDQPDepth() >= 1))
#endif
        {
          Int splitBits = m_pcEntropyCoder->getNumberOfWrittenBits();
          Double splitBitCost = m_pcRdCost->calcRdCost( splitBits, 0 );
#if defined(SIG_CCU)
          if (g_sigdump.ccu && m_pcEncCfg->getUseRateCtrl()) {
            sigdump_output_fprint(&g_sigpool.ccu_cost_ctx, "(4*cu%d cost) 0x%x = 0x%x + 0x%x (split flag)\n",
              rpcTempCU->getWidth(0), (UInt)(splitTotalCost * 4 + splitBitCost * 4), (UInt)(splitTotalCost * 4), (UInt)(splitBitCost * 4));
          }
#endif
          splitTotalCost += splitBitCost;
        }

        rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
        rpcTempCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
      }

#ifndef CVI_HW_RC
      if ((m_pcEncCfg->getLumaLevelToDeltaQPMapping().isEnabled() && pps.getMaxCuDQPDepth() >= 1))
#endif
      {
        rpcTempCU->getTotalCost() = splitTotalCost;
      }
#ifndef CVI_HW_RC
      else
      {
        rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
      }
#endif

      if( uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
      {
        Bool hasResidual = false;
        for( UInt uiBlkIdx = 0; uiBlkIdx < rpcTempCU->getTotalNumPart(); uiBlkIdx ++)
        {
          if( (     rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Y)
                || (rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Cb) && (numberValidComponents > COMPONENT_Cb))
                || (rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Cr) && (numberValidComponents > COMPONENT_Cr)) ) )
          {
            hasResidual = true;
            break;
          }
        }

        if ( hasResidual )
        {
#ifdef CVI_HW_RC
          m_pcEntropyCoder->resetBits();
          m_pcEntropyCoder->encodeQP( rpcTempCU, 0, false );
          Int qpBits = m_pcEntropyCoder->getNumberOfWrittenBits();
          rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
          rpcTempCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
          Double qpBitsCost = m_pcRdCost->calcRdCost( qpBits, 0 );
          rpcTempCU->getTotalCost() += qpBitsCost;
#else
          m_pcEntropyCoder->resetBits();
          m_pcEntropyCoder->encodeQP( rpcTempCU, 0, false );
          rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
          rpcTempCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
          rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
#endif
          Bool foundNonZeroCbf = false;
          rpcTempCU->setQPSubCUs( rpcTempCU->getRefQP( 0 ), 0, uiDepth, foundNonZeroCbf );
          assert( foundNonZeroCbf );
        }
        else
        {
          rpcTempCU->setQPSubParts( rpcTempCU->getRefQP( 0 ), 0, uiDepth ); // set QP to default QP
        }
      }

      m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

      // If the configuration being tested exceeds the maximum number of bytes for a slice / slice-segment, then
      // a proper RD evaluation cannot be performed. Therefore, termination of the
      // slice/slice-segment must be made prior to this CTU.
      // This can be achieved by forcing the decision to be that of the rpcTempCU.
      // The exception is each slice / slice-segment must have at least one CTU.
      if (rpcBestCU->getTotalCost()!=MAX_DOUBLE)
      {
        const Bool isEndOfSlice        =    pcSlice->getSliceMode()==FIXED_NUMBER_OF_BYTES
                                         && ((pcSlice->getSliceBits()+rpcBestCU->getTotalBits())>pcSlice->getSliceArgument()<<3)
                                         && rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceCurStartCtuTsAddr())
                                         && rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceSegmentCurStartCtuTsAddr());
        const Bool isEndOfSliceSegment =    pcSlice->getSliceSegmentMode()==FIXED_NUMBER_OF_BYTES
                                         && ((pcSlice->getSliceSegmentBits()+rpcBestCU->getTotalBits()) > pcSlice->getSliceSegmentArgument()<<3)
                                         && rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceSegmentCurStartCtuTsAddr());
                                             // Do not need to check slice condition for slice-segment since a slice-segment is a subset of a slice.
        if(isEndOfSlice||isEndOfSliceSegment)
        {
          rpcBestCU->getTotalCost()=MAX_DOUBLE;
        }
      }
#ifdef CVI_FAST_CU_ENC
      ForceDecision decision = FORCE_BYPASS;
      if(rpcBestCU->getSlice()->getSliceType() != I_SLICE)
      {
        decision = TermByPartialCost(rpcBestCU, rpcTempCU, rpcBestCU->getWidth(0), pcSubBestPartCU->getTotalNumPart());
        if(decision==FORCE_BYPASS) {
          decision = TerminateDecision(rpcBestCU, rpcTempCU, rpcBestCU->getWidth(0), pcSubBestPartCU->getTotalNumPart());
        }
      }

#ifdef CVI_FAST_CU_ENC_BY_COST
      if(rpcBestCU->getSlice()->getSliceType() != I_SLICE)
      {
        if (isEnableFastCUEncByCost())
        {
          decision = TerminateDecision(rpcBestCU);

          if (uiWidth == 32)
          {
            if (g_cu16_save_map[0][0] == 2)
            {
              gp_fe_param->cu32_save_cnt++;
            }
            else
            {
              int *p_save_map = &(g_cu16_save_map[0][0]);
              for (int i = 0; i < 4; i++)
              {
                if (*p_save_map == 1)
                  gp_fe_param->cu16_save_cnt++;
                p_save_map++;
              }
            }
          }
        }
      }
#endif //~CVI_FAST_CU_ENC_BY_COST

#ifdef CVI_QP_MAP
      // Support QP MAP CU16 skip flag
      // Force to change partition by considering skip flag in CU32 and in CU16
      if ((isQpMapEnable() || isEnableSmartEncode()) && pcSlice->getSliceType() != I_SLICE)
      {
        UInt cu_x = rpcBestCU->getCUPelX();
        UInt cu_y = rpcBestCU->getCUPelY();
        UInt cu_width = rpcBestCU->getWidth(0);
        UInt pic_w = pcPic->getPicYuvRec()->getWidth(COMPONENT_Y);
        UInt pic_h = pcPic->getPicYuvRec()->getHeight(COMPONENT_Y);

        if ((cu_x + cu_width) <= pic_w && (cu_y + cu_width) <= pic_h)
        {
          ForceDecision skip_decision = checkQpMapCuDecision(cu_x, cu_y, cu_width);
          if(skip_decision != FORCE_BYPASS)
            decision = skip_decision;
        }
      }
#endif
#if defined(SIG_CCU)
      if (g_sigdump.ccu && uiWidth < 64) {
        sigdump_output_fprint(&g_sigpool.ccu_cost_ctx, "[final]cu%d, cost = 0x%x, cu%d, cost = 0x%x\n",
          rpcBestCU->getWidth(0), (UInt)(rpcBestCU->getTotalCost() * 4),
          rpcBestCU->getWidth(0) / 2, (UInt)(rpcTempCU->getTotalCost() * 4));
      }
#endif

#ifdef CVI_FAST_CU_ENC_BY_COST
      if (isEnableFastCUEncByCost())
      {
        if (uiWidth == 16 || uiWidth == 32)
        {
          store_current_cu_cost(uiLPelX, uiTPelY, uiWidth, rpcBestCU->getTotalCost());

          if (uiWidth == 32 && decision == FORCE_KEEP)
          {
            Double cu16_cost = (Double)(((UInt64)(rpcBestCU->getTotalCost())) >> 2);
            for (int by = uiTPelY; by < uiTPelY + 32; by += 16)
            {
              for (int bx = uiLPelX; bx < uiLPelX + 32; bx += 16)
                store_current_cu_cost(bx, by, 16, cu16_cost);
            }
          }
        }

        int hist_level_max = CU_COST_HIST_BIN - 1;

        if (pcSlice->getSliceType() == I_SLICE)
        {
          if (uiWidth == 16)
          {
            UInt64 cost = gp_fe_param->blk16_cost.get_data(uiLPelX, uiTPelY);
            gp_fe_param->cu_cost_accum[FE_CU16] += cost;

            int scale_log2 = gp_fe_param->cu_cost_scale_log2[FE_CU16];
            int level = min(hist_level_max, (int)(cost >> scale_log2));
            gp_fe_param->cu_cost_hist[FE_CU16][level]++;
          }
        }
        else
        {
          if (uiWidth == 32)
          {
            UInt64 cost = 0;
            int scale_log2 = 0;
            int level = 0;

            if ((uiLPelX + 32) <= pcPic->getPicYuvRec()->getWidth(COMPONENT_Y) &&
                (uiTPelY + 32) <= pcPic->getPicYuvRec()->getHeight(COMPONENT_Y))
            {
              cost = gp_fe_param->blk32_cost.get_data(uiLPelX, uiTPelY);
              gp_fe_param->cu_cost_accum[FE_CU32] += cost;

              scale_log2 = gp_fe_param->cu_cost_scale_log2[FE_CU32];
              level = min(hist_level_max, (int)(cost >> scale_log2));
              gp_fe_param->cu_cost_hist[FE_CU32][level]++;
            }

            scale_log2 = gp_fe_param->cu_cost_scale_log2[FE_CU16];
            for (int by = uiTPelY; by < uiTPelY + 32; by += 16)
            {
              for (int bx = uiLPelX; bx < uiLPelX + 32; bx += 16)
              {
                if ((bx + 16) <= pcPic->getPicYuvRec()->getWidth(COMPONENT_Y) &&
                    (by + 16) <= pcPic->getPicYuvRec()->getHeight(COMPONENT_Y))
                {
                  cost = gp_fe_param->blk16_cost.get_data(bx, by);
                  gp_fe_param->cu_cost_accum[FE_CU16] += cost;

                  level = min(hist_level_max, (int)(cost >> scale_log2));
                  gp_fe_param->cu_cost_hist[FE_CU16][level]++;
                }
              }
            }
          }
        }
      }
#endif //~CVI_FAST_CU_ENC_BY_COST

      xCheckBestModeOrForceDecision(rpcBestCU, rpcTempCU, uiDepth, false, decision);
#ifdef CVI_FAST_CU_ENC_BY_COST
      if (uiWidth == 16 || uiWidth == 32)
      {
        if (isEnableFastEncMonitor() && pcSlice->getSliceType() != I_SLICE)
        {
          int pic_height = sps.getPicHeightInLumaSamples();
          if (uiWidth == 32 && rpcBestCU->getCUPelY() + uiWidth < pic_height)
          {
            update_fast_enc_status();
          }
        }
      }
#endif //~CVI_FAST_CU_ENC_BY_COST

      ccu_registerTermStats(rpcBestCU->getWidth(0), decision==FORCE_KEEP);
#else
      xCheckBestMode( rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTempDebug) DEBUG_STRING_PASS_INTO(false) ); // RD compare current larger prediction
#endif //~CVI_FAST_CU_ENC
    }
  }

#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    if (uiWidth < 64)
    {
      if (uiRPelX < g_sigpool.width && uiBPelY < g_sigpool.height)
        sig_rru_output_cu_winner(uiWidth);
    }
  }
#endif //~SIG_RRU

#ifdef SIG_IAPU
  if (g_sigdump.iapu)
  {
    if (uiWidth < 64)
    {
      if (uiRPelX < g_sigpool.width && uiBPelY < g_sigpool.height)
        sig_iapu_output_cu_winner(rpcBestCU, uiLPelX, uiTPelY, uiWidth);

//        printf("[DBG2] (cu_x, cu_y)=(%d, %d)\n", g_sigpool.cu_idx_x, g_sigpool.cu_idx_y);
        int best_width  = rpcBestCU->getWidth(0);
        int depth       = (best_width==32)? 1: (best_width==16)? 2: 3;

//        printf("[DBG2] best width=%d\n", rpcBestCU->getWidth(0));
//        printf("[DBG2] depth=%d\n", depth);

        TComYuv *p_rec_yuv = m_ppcRecoYuvBest[depth];
        for (int nb_b = 1; nb_b >= 0; nb_b--)
        {
          for (int comp = 0; comp < 3; comp++)
          {
              ComponentID id = ComponentID(comp);
              int stride = p_rec_yuv->getStride(id);
              const Pel *p_rec = p_rec_yuv->getAddr(id, 0, stride);
//        printf("[DBG2] stride=%d\n", stride);
              sig_iapu_output_cu_neb_wb(&g_sigpool.iapu_iap_cu8_rec_frm_ctx, p_rec, stride, nb_b, id);
          }
        }
    }
  }
#endif //~SIG_RRU

  DEBUG_STRING_APPEND(sDebug_, sDebug);

  rpcBestCU->copyToPic(uiDepth);                                                     // Copy Best data to Picture for next partition prediction.

  xCopyYuv2Pic( rpcBestCU->getPic(), rpcBestCU->getCtuRsAddr(), rpcBestCU->getZorderIdxInCtu(), uiDepth, uiDepth );   // Copy Yuv data to picture Yuv

#ifdef CVI_ENABLE_RDO_BIT_EST
  if(uiWidth==32 && getRDOBitEstMode()==CTX_ADAPTIVE)
  {
    m_pcEntropyCoder->resetBits();
    m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CVI_CU32]);
    delayResiModel();
    setResiModelState(RESI_MODEL_TRAIN);
    xEncodeCU( rpcBestCU, 0, uiDepth);
    setResiModelState(RESI_MODEL_TEST);
    m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_CVI_CU32]);
#ifdef SIG_CABAC
    if (g_sigdump.cabac) {
      sig_store_cabac2ccu(rpcBestCU);
    }
#endif
  }
#endif

  if (bBoundary)
  {
    return;
  }

#ifdef CVI_FAST_CU_ENC
  if(isEnableFastCUEnc() &&
    rpcBestCU->getSlice()->getSliceType() != I_SLICE)
  {
    ccu_updateCUStatus(uiLPelX, uiTPelY, uiWidth, rpcBestCU->getDepth(0)-1);
    if(uiWidth <= 32) {
      g_cu_rdcost.set_data(rpcBestCU->getZorderIdxInCtu(), uiDepth, rpcBestCU->getTotalCost());
    }
  }
#endif

#if defined(SIG_IRPU)
  if (g_sigdump.irpu && uiWidth <= 32)
  {
    int is_win = 1;
    int refIdx = -1;
    TComMv refMv(0, 0);
    if (!rpcBestCU->isIntra( 0 )) {
      refIdx = rpcBestCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( 0 );
      refMv = rpcBestCU->getCUMvField( REF_PIC_LIST_0 )->getMv(0);
    } else {
      refIdx = 3;
    }
    if(uiWidth == 16 || uiWidth == 32)
      is_win = (rpcBestCU->getWidth(0) == uiWidth) ? 1 : 0;
    sigdump_output_fprint(&g_sigpool.irpu_cmd_frm_ctx[sig_pat_blk_size_8(uiWidth)], "(%x, %x, %x, %x)\n", is_win, refIdx, refMv.getVer(), refMv.getHor());
  }
#endif

  // Assert if Best prediction mode is NONE
  // Selected mode's RD-cost must be not MAX_DOUBLE.
  assert( rpcBestCU->getPartitionSize ( 0 ) != NUMBER_OF_PART_SIZES       );
  assert( rpcBestCU->getPredictionMode( 0 ) != NUMBER_OF_PREDICTION_MODES );
  assert( rpcBestCU->getTotalCost     (   ) != MAX_DOUBLE                 );
}

/** finish encoding a cu and handle end-of-slice conditions
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TEncCu::finishCU( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  TComPic* pcPic = pcCU->getPic();
  TComSlice * pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());

  //Calculate end address
  const Int  currentCTUTsAddr = pcPic->getPicSym()->getCtuRsToTsAddrMap(pcCU->getCtuRsAddr());
  const Bool isLastSubCUOfCtu = pcCU->isLastSubCUOfCtu(uiAbsPartIdx);
  if ( isLastSubCUOfCtu )
  {
    // The 1-terminating bit is added to all streams, so don't add it here when it's 1.
    // i.e. when the slice segment CurEnd CTU address is the current CTU address+1.
    if (pcSlice->getSliceSegmentCurEndCtuTsAddr() != currentCTUTsAddr+1)
    {
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        std::strncpy( g_sigpool.syntax_name, "end_of_slice_segment_flag", sizeof(g_sigpool.syntax_name) );
        sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0, 14, 0, 0, 0);
      }
      cabac_prof_add_bins(NON_RESI, BYPASS, 1);
#endif
      m_pcEntropyCoder->encodeTerminatingBit( 0 );
    }
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      if ((m_pcEntropyCoder->getNumberOfWrittenBits() - 1) != g_sigpool.bits_accu_tmp) {
        UInt endBits = m_pcEntropyCoder->getNumberOfWrittenBits() - 1 - g_sigpool.bits_accu_tmp;

        if (g_sigpool.cabac_bits_statis_tu_ctx.enable)
        {
          fseek(g_sigpool.cabac_bits_statis_tu_ctx.fp, g_sigpool.cur_bits_offset, SEEK_SET);
          sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "bits_header = %x\n", g_sigpool.bits_header_tmp + endBits);
          sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "bits_res = %x\n", g_sigpool.bits_res_tmp);
          sigdump_output_fprint(&g_sigpool.cabac_bits_statis_tu_ctx, "bits_accu = %x\n", m_pcEntropyCoder->getNumberOfWrittenBits() - 1);
        }
        g_sigpool.bits_header = 0;
        g_sigpool.tu_start_bits_pos = m_pcEntropyCoder->getNumberOfWrittenBits();
      }
    }
#endif
  }
}

/** Compute QP for each CU
 * \param pcCU Target CU
 * \param uiDepth CU depth
 * \returns quantization parameter
 */
Int TEncCu::xComputeQP( TComDataCU* pcCU, UInt uiDepth )
{
  Int iBaseQp = pcCU->getSlice()->getSliceQp();
  Int iQpOffset = 0;
  if ( m_pcEncCfg->getUseAdaptiveQP() )
  {
    TEncPic* pcEPic = dynamic_cast<TEncPic*>( pcCU->getPic() );
    UInt uiAQDepth = min( uiDepth, pcEPic->getMaxAQDepth()-1 );
    TEncPicQPAdaptationLayer* pcAQLayer = pcEPic->getAQLayer( uiAQDepth );
    UInt uiAQUPosX = pcCU->getCUPelX() / pcAQLayer->getAQPartWidth();
    UInt uiAQUPosY = pcCU->getCUPelY() / pcAQLayer->getAQPartHeight();
    UInt uiAQUStride = pcAQLayer->getAQPartStride();
    TEncQPAdaptationUnit* acAQU = pcAQLayer->getQPAdaptationUnit();

    Double dMaxQScale = pow(2.0, m_pcEncCfg->getQPAdaptationRange()/6.0);
    Double dAvgAct = pcAQLayer->getAvgActivity();
    Double dCUAct = acAQU[uiAQUPosY * uiAQUStride + uiAQUPosX].getActivity();
    Double dNormAct = (dMaxQScale*dCUAct + dAvgAct) / (dCUAct + dMaxQScale*dAvgAct);
    Double dQpOffset = log(dNormAct) / log(2.0) * 6.0;
    iQpOffset = Int(floor( dQpOffset + 0.49999 ));
  }

  return Clip3(-pcCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQp+iQpOffset );
}

/** encode a CU block recursively
 * \param pcCU
 * \param uiAbsPartIdx
 * \param uiDepth
 * \returns Void
 */
Void TEncCu::xEncodeCU( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
        TComPic   *const pcPic   = pcCU->getPic();
        TComSlice *const pcSlice = pcCU->getSlice();
  const TComSPS   &sps =*(pcSlice->getSPS());
  const TComPPS   &pps =*(pcSlice->getPPS());

  const UInt maxCUWidth  = sps.getMaxCUWidth();
  const UInt maxCUHeight = sps.getMaxCUHeight();

        Bool bBoundary = false;
        UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
  const UInt uiRPelX   = uiLPelX + (maxCUWidth>>uiDepth)  - 1;
        UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
  const UInt uiBPelY   = uiTPelY + (maxCUHeight>>uiDepth) - 1;

#ifdef CVI_SMART_ENC
  int temp = 0;
  int isForceSkip = 0;
  int startbits = 0;
  if(g_algo_cfg.EnableCalMapData)
    startbits = m_pcEntropyCoder->getNumberOfWrittenBits();
#endif

#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record)
  {
    sigdump_output_fprint(&g_sigpool.cabac_ctx, "# [CU info] (CU_X, CU_Y) = (%d, %d) (%d x %d)\n",
      uiLPelX, uiTPelY, (maxCUWidth>>uiDepth), (maxCUHeight>>uiDepth));
    sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, "# [CU info] (CU_X, CU_Y) = (%d, %d) (%d x %d)\n",
      uiLPelX, uiTPelY, (maxCUWidth>>uiDepth), (maxCUHeight>>uiDepth));
      g_sigpool.cur_depth = uiDepth;
  }
#endif
#ifdef SIG_CCU
  if (g_sigdump.ccu)
  {
    if (g_sigpool.ccu_is_record_cu_bits && uiDepth == 1)
    {
      if (((uiLPelX & 0x3f) != 0) || ((uiTPelY & 0x3f) != 0))
        g_sigpool.p_ccu_rc_st->cu32_start_bits = m_pcEntropyCoder->getNumberOfWrittenBits();
    }
  }
#endif //~SIG_CCU

  if( ( uiRPelX < sps.getPicWidthInLumaSamples() ) && ( uiBPelY < sps.getPicHeightInLumaSamples() ) )
  {
    m_pcEntropyCoder->encodeSplitFlag( pcCU, uiAbsPartIdx, uiDepth );
  }
  else
  {
    bBoundary = true;
  }

#ifdef CVI_SMART_ENC
  if(g_algo_cfg.EnableCalMapData)
  {
    ISP_ENC_PARAM* p_isp_enc_param = gp_isp_enc_param;
    if (p_isp_enc_param->is_entropy == 1)//split flag
    {
      if (uiDepth == 1 || uiDepth == 2) {
        getQpMapBlk16(uiLPelX, uiTPelY, &isForceSkip, (QP_MAP_DELTA_MODE*)(&temp), &temp);
        if (isForceSkip > 0) {
          p_isp_enc_param->skip_bits += m_pcEntropyCoder->getNumberOfWrittenBits() - startbits;
        } else {
          p_isp_enc_param->nonskip_bits += m_pcEntropyCoder->getNumberOfWrittenBits() - startbits;
        }
      } else {
        p_isp_enc_param->nonskip_bits += m_pcEntropyCoder->getNumberOfWrittenBits() - startbits;
      }
    }
  }
#endif
  if( ( ( uiDepth < pcCU->getDepth( uiAbsPartIdx ) ) && ( uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() ) ) || bBoundary )
  {
    UInt uiQNumParts = ( pcPic->getNumPartitionsInCtu() >> (uiDepth<<1) )>>2;
    if( uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
    {
      setdQPFlag(true);
    }

    if( uiDepth == pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcSlice->getUseChromaQpAdj())
    {
      setCodeChromaQpAdjFlag(true);
    }

    for ( UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx+=uiQNumParts )
    {
      uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
      uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
      if( ( uiLPelX < sps.getPicWidthInLumaSamples() ) && ( uiTPelY < sps.getPicHeightInLumaSamples() ) )
      {
        xEncodeCU( pcCU, uiAbsPartIdx, uiDepth+1 );
      }
    }
    return;
  }

#ifdef CVI_HW_RC
  if (g_updata_stat_frm_en)
  {
    update_frm_statistic(pcCU, uiAbsPartIdx, uiLPelX, uiTPelY, (maxCUWidth>>uiDepth), m_pcEncCfg->getUseRateCtrl());
  }
#endif

#if defined(SIG_RRU) || defined(SIG_PPU)
  if (g_sigdump.rru || g_sigdump.ppu || g_sigdump.ccu)
  {
    if (g_sigpool.rru_is_record_pos)
    {
      int cu_x = uiLPelX;
      int cu_y = uiTPelY;
      int cu_w = (maxCUWidth>>uiDepth);
      int cu_h = (maxCUHeight>>uiDepth);

      if (g_sigdump.rru)
      {
        sig_ctx *p_ctx = &g_sigpool.rru_cu_order_ctx;
        sig_rru_output_cu_order(p_ctx, pcCU, cu_x, cu_y, cu_w, cu_h, uiAbsPartIdx, uiDepth);
      }
      if (g_sigdump.ppu || g_sigdump.ccu)
      {
        sig_ctx *p_ctx = &g_sigpool.ppu_input_ctx;
        sig_rru_output_cu_order(p_ctx, pcCU, cu_x, cu_y, cu_w, cu_h, uiAbsPartIdx, uiDepth);
      }
    }
  }
#endif //~SIG_RRU || SIG_PPU

#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    UInt idx_x   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
    UInt idx_y   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
    if ((idx_x % 32) == 0 && (idx_y % 32) == 0) {
      sig_dump_cabac2ccu(idx_x, idx_y);
    }
  }
#endif

#ifdef CVI_SMART_ENC
  if(g_algo_cfg.EnableCalMapData)
    startbits = m_pcEntropyCoder->getNumberOfWrittenBits();
#endif

  if( uiDepth <= pps.getMaxCuDQPDepth() && pps.getUseDQP())
  {
    setdQPFlag(true);
  }

  if( uiDepth <= pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcSlice->getUseChromaQpAdj())
  {
    setCodeChromaQpAdjFlag(true);
  }

  if (pps.getTransquantBypassEnabledFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( pcCU, uiAbsPartIdx );
  }

  if( !pcSlice->isIntra() )
  {
    m_pcEntropyCoder->encodeSkipFlag( pcCU, uiAbsPartIdx );
  }

  if( pcCU->isSkipped( uiAbsPartIdx ) )
  {
    m_pcEntropyCoder->encodeMergeIndex( pcCU, uiAbsPartIdx );
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      sig_cabac_dummy_tu(pcCU, m_pcEntropyCoder->getNumberOfWrittenBits(), uiAbsPartIdx, uiDepth);
    }
#endif //~SIG_CABAC
#ifdef SIG_CCU
    if (g_sigdump.ccu && g_sigpool.ccu_is_record_cu_bits)
    {
      sig_ccu_dummy_resi(pcCU, uiAbsPartIdx, uiDepth);
    }
#endif
    finishCU(pcCU,uiAbsPartIdx);

#ifdef CVI_SMART_ENC
    if (g_algo_cfg.EnableCalMapData) {
      ISP_ENC_PARAM* p_isp_enc_param = gp_isp_enc_param;
      if (p_isp_enc_param->is_entropy == 1) {
        if (uiDepth == 1 || uiDepth == 2) {
          getQpMapBlk16(uiLPelX, uiTPelY, &isForceSkip, (QP_MAP_DELTA_MODE*)(&temp), &temp);
          if (isForceSkip > 0) {
            p_isp_enc_param->skip_bits += m_pcEntropyCoder->getNumberOfWrittenBits() - startbits;
          } else {
            p_isp_enc_param->nonskip_bits += m_pcEntropyCoder->getNumberOfWrittenBits() - startbits;
          }
        } else {
          p_isp_enc_param->nonskip_bits += m_pcEntropyCoder->getNumberOfWrittenBits() - startbits;
        }
      }
    }
#endif
#ifdef SIG_CCU
    if (g_sigdump.ccu)
    {
      if (g_sigpool.ccu_is_record_cu_bits)
      {
        int cu_w = (maxCUWidth>>uiDepth);
        int cu_h = (maxCUHeight>>uiDepth);
        int bits = m_pcEntropyCoder->getNumberOfWrittenBits();
        sig_ccu_record_cu32_bits(uiLPelX, uiTPelY, cu_w, cu_h, bits);
      }
    }
#endif //~SIG_CCU
    return;
  }

  m_pcEntropyCoder->encodePredMode( pcCU, uiAbsPartIdx );
  m_pcEntropyCoder->encodePartSize( pcCU, uiAbsPartIdx, uiDepth );

  if (pcCU->isIntra( uiAbsPartIdx ) && pcCU->getPartitionSize( uiAbsPartIdx ) == SIZE_2Nx2N )
  {
    m_pcEntropyCoder->encodeIPCMInfo( pcCU, uiAbsPartIdx );

    if(pcCU->getIPCMFlag(uiAbsPartIdx))
    {
      // Encode slice finish
      finishCU(pcCU,uiAbsPartIdx);
      return;
    }
  }

  // prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
  m_pcEntropyCoder->encodePredInfo( pcCU, uiAbsPartIdx );

  // Encode Coefficients
  Bool bCodeDQP = getdQPFlag();
  Bool codeChromaQpAdj = getCodeChromaQpAdjFlag();
  m_pcEntropyCoder->encodeCoeff( pcCU, uiAbsPartIdx, uiDepth, bCodeDQP, codeChromaQpAdj );
  setCodeChromaQpAdjFlag( codeChromaQpAdj );
  setdQPFlag( bCodeDQP );

  // --- write terminating bit ---
  finishCU(pcCU,uiAbsPartIdx);

#ifdef SIG_CCU
  if (g_sigdump.ccu)
  {
    if (g_sigpool.ccu_is_record_cu_bits)
    {
      int cu_w = (maxCUWidth>>uiDepth);
      int cu_h = (maxCUHeight>>uiDepth);
      int bits = m_pcEntropyCoder->getNumberOfWrittenBits();
      sig_ccu_record_cu32_bits(uiLPelX, uiTPelY, cu_w, cu_h, bits);
    }
  }
#endif //~SIG_CCU

#ifdef CVI_SMART_ENC
  if(g_algo_cfg.EnableCalMapData){
    ISP_ENC_PARAM* p_isp_enc_param = gp_isp_enc_param;
    if (p_isp_enc_param->is_entropy == 1)
    {
      p_isp_enc_param->nonskip_bits += m_pcEntropyCoder->getNumberOfWrittenBits() - startbits;
    }
  }
#endif
}

Int xCalcHADs8x8_ISlice(Pel *piOrg, Int iStrideOrg)
{
  Int k, i, j, jj;
  Int diff[64], m1[8][8], m2[8][8], m3[8][8], iSumHad = 0;

  for( k = 0; k < 64; k += 8 )
  {
    diff[k+0] = piOrg[0] ;
    diff[k+1] = piOrg[1] ;
    diff[k+2] = piOrg[2] ;
    diff[k+3] = piOrg[3] ;
    diff[k+4] = piOrg[4] ;
    diff[k+5] = piOrg[5] ;
    diff[k+6] = piOrg[6] ;
    diff[k+7] = piOrg[7] ;

    piOrg += iStrideOrg;
  }

  //horizontal
  for (j=0; j < 8; j++)
  {
    jj = j << 3;
    m2[j][0] = diff[jj  ] + diff[jj+4];
    m2[j][1] = diff[jj+1] + diff[jj+5];
    m2[j][2] = diff[jj+2] + diff[jj+6];
    m2[j][3] = diff[jj+3] + diff[jj+7];
    m2[j][4] = diff[jj  ] - diff[jj+4];
    m2[j][5] = diff[jj+1] - diff[jj+5];
    m2[j][6] = diff[jj+2] - diff[jj+6];
    m2[j][7] = diff[jj+3] - diff[jj+7];

    m1[j][0] = m2[j][0] + m2[j][2];
    m1[j][1] = m2[j][1] + m2[j][3];
    m1[j][2] = m2[j][0] - m2[j][2];
    m1[j][3] = m2[j][1] - m2[j][3];
    m1[j][4] = m2[j][4] + m2[j][6];
    m1[j][5] = m2[j][5] + m2[j][7];
    m1[j][6] = m2[j][4] - m2[j][6];
    m1[j][7] = m2[j][5] - m2[j][7];

    m2[j][0] = m1[j][0] + m1[j][1];
    m2[j][1] = m1[j][0] - m1[j][1];
    m2[j][2] = m1[j][2] + m1[j][3];
    m2[j][3] = m1[j][2] - m1[j][3];
    m2[j][4] = m1[j][4] + m1[j][5];
    m2[j][5] = m1[j][4] - m1[j][5];
    m2[j][6] = m1[j][6] + m1[j][7];
    m2[j][7] = m1[j][6] - m1[j][7];
  }

  //vertical
  for (i=0; i < 8; i++)
  {
    m3[0][i] = m2[0][i] + m2[4][i];
    m3[1][i] = m2[1][i] + m2[5][i];
    m3[2][i] = m2[2][i] + m2[6][i];
    m3[3][i] = m2[3][i] + m2[7][i];
    m3[4][i] = m2[0][i] - m2[4][i];
    m3[5][i] = m2[1][i] - m2[5][i];
    m3[6][i] = m2[2][i] - m2[6][i];
    m3[7][i] = m2[3][i] - m2[7][i];

    m1[0][i] = m3[0][i] + m3[2][i];
    m1[1][i] = m3[1][i] + m3[3][i];
    m1[2][i] = m3[0][i] - m3[2][i];
    m1[3][i] = m3[1][i] - m3[3][i];
    m1[4][i] = m3[4][i] + m3[6][i];
    m1[5][i] = m3[5][i] + m3[7][i];
    m1[6][i] = m3[4][i] - m3[6][i];
    m1[7][i] = m3[5][i] - m3[7][i];

    m2[0][i] = m1[0][i] + m1[1][i];
    m2[1][i] = m1[0][i] - m1[1][i];
    m2[2][i] = m1[2][i] + m1[3][i];
    m2[3][i] = m1[2][i] - m1[3][i];
    m2[4][i] = m1[4][i] + m1[5][i];
    m2[5][i] = m1[4][i] - m1[5][i];
    m2[6][i] = m1[6][i] + m1[7][i];
    m2[7][i] = m1[6][i] - m1[7][i];
  }

  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      iSumHad += abs(m2[i][j]);
    }
  }
  iSumHad -= abs(m2[0][0]);
  iSumHad =(iSumHad+2)>>2;
  return(iSumHad);
}

Int  TEncCu::updateCtuDataISlice(TComDataCU* pCtu, Int width, Int height)
{
  Int  xBl, yBl;
  const Int iBlkSize = 8;

  Pel* pOrgInit   = pCtu->getPic()->getPicYuvOrg()->getAddr(COMPONENT_Y, pCtu->getCtuRsAddr(), 0);
  Int  iStrideOrig = pCtu->getPic()->getPicYuvOrg()->getStride(COMPONENT_Y);
  Pel  *pOrg;

  Int iSumHad = 0;
  for ( yBl=0; (yBl+iBlkSize)<=height; yBl+= iBlkSize)
  {
    for ( xBl=0; (xBl+iBlkSize)<=width; xBl+= iBlkSize)
    {
      pOrg = pOrgInit + iStrideOrig*yBl + xBl;
      iSumHad += xCalcHADs8x8_ISlice(pOrg, iStrideOrig);
    }
  }
  return(iSumHad);
}

#ifdef CVI_SEPERATE_MREGE_SKIP
/** check RD costs for a CU block encoded with merge mode only
 * \param rpcBestCU
 * \param rpcTempCU
 * \param earlyDetectionSkipMode
 */
Void TEncCu::xCheckRDCostMerge2Nx2N_CVI( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU DEBUG_STRING_FN_DECLARE(sDebug), Bool *earlyDetectionSkipMode, UInt &BestMergeCand)
{
  assert( rpcTempCU->getSlice()->getSliceType() != I_SLICE );
  if(getFastDeltaQp())
  {
    return;   // never check merge in fast deltaqp mode
  }
  TComMvField  cMvFieldNeighbours[2 * MRG_MAX_NUM_CANDS]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  Int numValidMergeCand = 0;
  const Bool bTransquantBypassFlag = rpcTempCU->getCUTransquantBypass(0);

  for( UInt ui = 0; ui < rpcTempCU->getSlice()->getMaxNumMergeCand(); ++ui )
  {
    uhInterDirNeighbours[ui] = 0;
  }
  UChar uhDepth = rpcTempCU->getDepth( 0 );
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
#if MCTS_ENC_CHECK
  UInt numSpatialMergeCandidates = 0;
  rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, numSpatialMergeCandidates );
#else
  rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );
#endif

#if MCTS_ENC_CHECK
  if (m_pcEncCfg->getTMCTSSEITileConstraint() && rpcTempCU->isLastColumnCTUInTile())
  {
    numValidMergeCand = numSpatialMergeCandidates;
  }
#endif

  DEBUG_STRING_NEW(bestStr)

  if (cMvFieldNeighbours[0 + 2*0].getRefIdx() == cMvFieldNeighbours[0 + 2*1].getRefIdx()
      && cMvFieldNeighbours[0 + 2*0].getMv() == cMvFieldNeighbours[0 + 2*1].getMv()
      && cMvFieldNeighbours[1 + 2*0].getRefIdx() == cMvFieldNeighbours[1 + 2*1].getRefIdx()
      && cMvFieldNeighbours[1 + 2*0].getMv() == cMvFieldNeighbours[1 + 2*1].getMv()) {
    numValidMergeCand = 1;
#ifdef SIG_IRPU
    if (g_sigdump.irpu)
    {
      strcpy(g_sigpool.amvp_list[0][numValidMergeCand].dir, "NULL");
    }
#endif
  }

#ifdef SIG_IRPU
  if (g_sigdump.irpu)
  {
    sigdump_irpu_cu_cmd(rpcTempCU, cMvFieldNeighbours);
  }
#endif

  Bool DisableMerge = (rpcBestCU->getWidth(0) >= 32) && g_algo_cfg.DisableMerge32;

  if (!DisableMerge)
  {
    Double MergeCandCost = MAX_DOUBLE;
#ifdef CVI_ENC_SETTING
    if (g_cvi_enc_setting.enableConstrainedMergeCand && rpcBestCU->getWidth(0) == 8)
    {
      g_cvi_enc_setting.merge_cnt++;
      if (numValidMergeCand >= 2)
      {
        g_cvi_enc_setting.merge_2_cand_cnt++;
        int l0_mvx_diff = abs(cMvFieldNeighbours[0 + 2].getHor() - cMvFieldNeighbours[0].getHor());
        int l0_mvy_diff = abs(cMvFieldNeighbours[0 + 2].getVer() - cMvFieldNeighbours[0].getVer());
        int l1_mvx_diff = abs(cMvFieldNeighbours[1 + 2].getHor() - cMvFieldNeighbours[1].getHor());
        int l1_mvy_diff = abs(cMvFieldNeighbours[1 + 2].getVer() - cMvFieldNeighbours[1].getVer());
        if ((l0_mvx_diff < g_cvi_enc_setting.merge_mv_thr_x) && (l0_mvy_diff < g_cvi_enc_setting.merge_mv_thr_y)
            && (l1_mvx_diff < g_cvi_enc_setting.merge_mv_thr_x) && (l1_mvy_diff < g_cvi_enc_setting.merge_mv_thr_y))
        {
          numValidMergeCand = 1;
          g_cvi_enc_setting.merge_reduce_cnt++;
        }
      }
    }
#endif
#ifdef SIG_MC
    if (g_sigdump.mc) {
      int blk_size = sig_pat_blk_size_8(rpcBestCU->getWidth(0));
      sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[blk_size], "BLK%d #%x\n", rpcBestCU->getWidth(0), g_sigpool.mrg_cnt[blk_size]++);
      sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[blk_size], "blk_cur_x = %x\n", g_sigpool.cu_idx_x);
      sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[blk_size], "blk_cur_y = %x\n", g_sigpool.cu_idx_y);
      sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[blk_size], "candidate_num = %x\n", numValidMergeCand);
    }
#endif

    for( UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand )
    {
      DEBUG_STRING_NEW(tmpStr)
      // set MC parameters
      rpcTempCU->setPredModeSubParts( MODE_INTER, 0, uhDepth ); // interprets depth relative to CTU level
      rpcTempCU->setCUTransquantBypassSubParts( bTransquantBypassFlag, 0, uhDepth );
      rpcTempCU->setChromaQpAdjSubParts( bTransquantBypassFlag ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uhDepth );
      rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
      rpcTempCU->setMergeFlagSubParts( true, 0, 0, uhDepth ); // interprets depth relative to CTU level
      rpcTempCU->setMergeIndexSubParts( uiMergeCand, 0, 0, uhDepth ); // interprets depth relative to CTU level
      rpcTempCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth ); // interprets depth relative to CTU level
      rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level
      rpcTempCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level
#ifdef SIG_MC
      if (g_sigdump.mc) {
        int blk_size = sig_pat_blk_size_8(rpcBestCU->getWidth(0));
        sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[blk_size], "candidate #%x\n", uiMergeCand);
        sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[blk_size], "blk_refidx = %x\n", rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( 0 ));
        TComMv rcMv = rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->getMv( 0 );
        sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[blk_size], "blk_fmvx = %x\n", rcMv.getHor());
        sigdump_output_fprint(&g_sigpool.mc_mrg_ctx_cmd_in[blk_size], "blk_fmvy = %x\n", rcMv.getVer());

        for (Int comp=0; comp < 2; comp++) {
          const ComponentID component = ComponentID(comp);
          const Int uiPartWidth =rpcBestCU->getWidth(0)>>rpcTempCU->getPic()->getComponentScaleX(component);
          const Int uiPartHeight=rpcBestCU->getWidth(0)>>rpcTempCU->getPic()->getComponentScaleY(component);
          const Pel* piSrc = m_ppcOrigYuv    [uhDepth]->getAddr( component, 0, uiPartWidth );

          for (int y = 0; y < uiPartHeight; y++) {
            for (int x = 0; x < uiPartWidth; x++) {
              UInt curStride = m_ppcOrigYuv[uhDepth]->getStride(component);
              sigdump_output_bin(&g_sigpool.mc_mrg_ctx_dat_out[blk_size][comp], (unsigned char *)(piSrc + y * curStride + x), 1);
              if (comp == 1) {
                const Pel* piSrc_cr = m_ppcOrigYuv    [uhDepth]->getAddr( ComponentID(2), 0, uiPartWidth );
                sigdump_output_bin(&g_sigpool.mc_mrg_ctx_dat_out[blk_size][comp], (unsigned char *)(piSrc_cr + y * curStride + x), 1);
              }
            }
          }
        }
      }
#endif
#if MCTS_ENC_CHECK
      if ( m_pcEncCfg->getTMCTSSEITileConstraint () && (!(m_pcPredSearch->checkTMctsMvp(rpcTempCU))))
      {
        continue;
      }

#endif
#ifdef CVI_CACHE_MODEL
      if (isEnableCACHE_MODEL())
      {
        miu_access_mc(rpcTempCU);
      }
#endif
      // do MC
      m_pcPredSearch->motionCompensation ( rpcTempCU, m_ppcPredYuvTemp[uhDepth] );
      // estimate residual and encode everything
      m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU,
                                                  m_ppcOrigYuv    [uhDepth],
                                                  m_ppcPredYuvTemp[uhDepth],
                                                  m_ppcResiYuvTemp[uhDepth],
                                                  m_ppcResiYuvBest[uhDepth],
                                                  m_ppcRecoYuvTemp[uhDepth],
                                                  false DEBUG_STRING_PASS_INTO(tmpStr) );

#if DEBUG_STRING
      DebugInterPredResiReco(tmpStr, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif
#ifdef SIG_IRPU
      if (g_sigdump.irpu) {
        int blk_size = sig_pat_blk_size_8(rpcBestCU->getWidth(0));
        sigdump_output_fprint(&g_sigpool.irpu_fme_mc_mdl_frm_ctx[blk_size], "%x\n", uiMergeCand);
      }
#endif
      if ((isEnableLC_RdCostInter() ? rpcTempCU->getTotalLcSADCost() : rpcTempCU->getTotalCost()) < MergeCandCost) {
        BestMergeCand = uiMergeCand;
        MergeCandCost = isEnableLC_RdCostInter() ? rpcTempCU->getTotalLcSADCost() : rpcTempCU->getTotalCost();
#if defined(SIG_IRPU) || defined(SIG_CCU)
        if (g_sigdump.irpu || g_sigdump.ccu)
        {
          sigdump_me_cu_golden_backup(rpcTempCU, uiMergeCand);
        }
        if (g_sigdump.ccu || g_sigdump.rdo_sse)
        {
          int blk_size = sig_pat_blk_size_4(g_sigpool.cu_width);
          memcpy(&g_sigpool.est_golden[blk_size][1], &g_sigpool.est_golden[blk_size][0], sizeof(sig_est_golden_st));
        }
#endif
#ifdef SIG_BIT_EST
        if (g_sigdump.bit_est) {
          for (int ch = 0; ch < 3; ch ++) {
            if (g_sigpool.cu_width < 32)
              memcpy(&g_sigpool.bit_est_golden[1][g_sigpool.is_merge][ch], &g_sigpool.bit_est_golden[0][g_sigpool.is_merge][ch], sizeof(sig_bit_est_st));
            else {
              for (int tu = 0; tu < 4; tu ++)
                memcpy(&g_sigpool.bit_est_golden_32[1][tu][ch], &g_sigpool.bit_est_golden_32[0][tu][ch], sizeof(sig_bit_est_st));
            }
          }
        }
#endif
      }
      Int orgQP = rpcTempCU->getQP( 0 );
      xCheckDQP( rpcTempCU );
      xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(bestStr) DEBUG_STRING_PASS_INTO(tmpStr), isEnableLC_RdCostInter() ? CVI_LC_SAD_COST : CVI_RD_COST);
      rpcTempCU->initEstData( uhDepth, orgQP, bTransquantBypassFlag );
    }

#ifdef SIG_RRU
    if (g_sigdump.rru)
    {
      for (int i = 0; i < MAX_NUM_COMPONENT; i++)
      {
        ComponentID compID = (ComponentID)(i);
        int pu_size = (compID == COMPONENT_Y) ? g_sigpool.cu_width : (g_sigpool.cu_width >> 1);
        Pel *p_pred_buf = m_ppcPredYuvBest[uhDepth]->getAddr(compID, 0);
        int stride = m_ppcPredYuvBest[uhDepth]->getStride(compID);

        sig_rru_copy_intp_temp_buf(compID, p_pred_buf, stride, g_sigpool.p_rru_intp_final, pu_size, pu_size);

        if (compID == COMPONENT_Y)
        {
          sig_rru_output_intp(false, g_sigpool.cu_width, false);

          if (g_sigpool.cu_width == 32)
            sig_rru_output_dct(false, g_sigpool.cu_width, false);
        }
        if (compID == COMPONENT_Cr)
        {
          sig_rru_output_intp(false, g_sigpool.cu_width, true);

          if (g_sigpool.cu_width == 32)
            sig_rru_output_dct(false, g_sigpool.cu_width, true);
        }
      }
    }
#endif

#ifdef CVI_FAST_CU_ENC
    if(isEnableFastCUEnc()) {
      if(rpcTempCU->getWidth(0)==32 || rpcTempCU->getWidth(0)==16) {
        g_isZeroResiMerge.set_data(rpcBestCU->getZorderIdxInCtu(), uhDepth, rpcBestCU->getQtRootCbf(0)==0);
      }
    }
#endif

#ifdef CVI_MOTION_DETECT
    if(rpcTempCU->getWidth(0)==16 || rpcTempCU->getWidth(0)==32 || rpcTempCU->getWidth(0)==8) {
      short mv[2][MRG_MAX_NUM_CANDS] = {0};
      for(Int mv_i=0; mv_i<numValidMergeCand; mv_i++) {
        mv[0][mv_i] = cMvFieldNeighbours[mv_i*2].getHor();
        mv[1][mv_i] = cMvFieldNeighbours[mv_i*2].getVer();
      }

      int width = rpcTempCU->getWidth(0);
      int dist_shift = (width == 16) ? 8 : ((width == 32) ? 10 : 6);
      int dist = ((Int)MergeCandCost)>>dist_shift;
      motion_detect_by_mv_and_sad(rpcTempCU->getCUPelX(), rpcTempCU->getCUPelY(), dist, mv[0], mv[1], numValidMergeCand, rpcTempCU->getWidth(0), rpcTempCU->getPic()->getPOC()==0);
    }
#endif

#if defined(SIG_IRPU) || defined(SIG_CCU)
    if (g_sigdump.ccu)
      sigdump_ccu_cu_golden_start("MERGE");
    if (g_sigdump.irpu)
      sigdump_irpu_cu_golden_start("Merge");
#endif
  }
  DEBUG_STRING_APPEND(sDebug, bestStr)
}

/** check RD costs for a CU block encoded with skip mode
 * \param rpcBestCU
 * \param rpcTempCU
 * \param earlyDetectionSkipMode
 */
Void TEncCu::xCheckRDCostSkip2Nx2N_CVI( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU DEBUG_STRING_FN_DECLARE(sDebug), Bool *earlyDetectionSkipMode, UInt BestMergeCand)
{
  assert( rpcTempCU->getSlice()->getSliceType() != I_SLICE );
  if(getFastDeltaQp())
  {
    return;   // never check merge in fast deltaqp mode
  }
  TComMvField  cMvFieldNeighbours[2 * MRG_MAX_NUM_CANDS]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  Int numValidMergeCand = 0;
  const Bool bTransquantBypassFlag = rpcTempCU->getCUTransquantBypass(0);

  for( UInt ui = 0; ui < rpcTempCU->getSlice()->getMaxNumMergeCand(); ++ui )
  {
    uhInterDirNeighbours[ui] = 0;
  }
  UChar uhDepth = rpcTempCU->getDepth( 0 );
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
#if MCTS_ENC_CHECK
  UInt numSpatialMergeCandidates = 0;
  rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, numSpatialMergeCandidates );
#else
  rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );
#endif

#if MCTS_ENC_CHECK
  if (m_pcEncCfg->getTMCTSSEITileConstraint() && rpcTempCU->isLastColumnCTUInTile())
  {
    numValidMergeCand = numSpatialMergeCandidates;
  }
#endif

  DEBUG_STRING_NEW(bestStr)

  Bool DisableSkip = (rpcBestCU->getWidth(0) >= 32) && g_algo_cfg.DisableSkip32;

  if (!DisableSkip)
  {
    UInt uiMergeCand = BestMergeCand;
    {
      DEBUG_STRING_NEW(tmpStr)
      // set MC parameters
      rpcTempCU->setPredModeSubParts( MODE_INTER, 0, uhDepth ); // interprets depth relative to CTU level
      rpcTempCU->setCUTransquantBypassSubParts( bTransquantBypassFlag, 0, uhDepth );
      rpcTempCU->setChromaQpAdjSubParts( bTransquantBypassFlag ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uhDepth );
      rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
      rpcTempCU->setMergeFlagSubParts( true, 0, 0, uhDepth ); // interprets depth relative to CTU level
      rpcTempCU->setMergeIndexSubParts( uiMergeCand, 0, 0, uhDepth ); // interprets depth relative to CTU level
      rpcTempCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth ); // interprets depth relative to CTU level
      rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level
      rpcTempCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level

#if MCTS_ENC_CHECK
      if ( m_pcEncCfg->getTMCTSSEITileConstraint () && (!(m_pcPredSearch->checkTMctsMvp(rpcTempCU))))
      {
        continue;
      }

#endif
#ifdef CVI_CACHE_MODEL
      if (isEnableCACHE_MODEL())
      {
        miu_access_mc(rpcTempCU);
      }
#endif
      // do MC
      m_pcPredSearch->motionCompensation ( rpcTempCU, m_ppcPredYuvTemp[uhDepth] );
      // estimate residual and encode everything
      m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU,
                                                  m_ppcOrigYuv    [uhDepth],
                                                  m_ppcPredYuvTemp[uhDepth],
                                                  m_ppcResiYuvTemp[uhDepth],
                                                  m_ppcResiYuvBest[uhDepth],
                                                  m_ppcRecoYuvTemp[uhDepth],
                                                  true DEBUG_STRING_PASS_INTO(tmpStr) );

#if DEBUG_STRING
      DebugInterPredResiReco(tmpStr, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif

#ifdef SIG_CCU
      g_sigpool.skip_rd_cost = rpcTempCU->getTotalCost();
      g_sigpool.skip_rd_cost_wt = rpcTempCU->getTotalCost();
#endif
#ifdef CVI_CU_PENALTY
      if (g_algo_cfg.CostPenaltyCfg.EnableCuCost || g_algo_cfg.CostPenaltyCfg.EnableForeground)
      {
        Double &cost = rpcTempCU->getTotalCost();
        int cu_x = rpcTempCU->getCUPelX();
        int cu_y = rpcTempCU->getCUPelY();
        int cu_width = rpcTempCU->getWidth(0);
#ifdef CVI_SPLIT_COST_WEIGHT
        UInt bits = rpcTempCU->getTotalBits();
        Distortion dist = rpcTempCU->getTotalDistortion();
        UInt64 lambda = m_pcRdCost->getFixpointFracLambda();
        calcCuPenaltyCostSplit(CU_PENALTY_SKIP, cost, bits, dist, lambda,
                               cu_width, cu_x, cu_y, false);
#else
        calcCuPenaltyCost(CU_PENALTY_SKIP, cost, cu_width, cu_x, cu_y, false);
#endif
#ifdef SIG_CCU
        g_sigpool.skip_rd_cost_wt = rpcTempCU->getTotalCost();
#endif
      }
#endif //~CVI_CU_PENALTY

      Int orgQP = rpcTempCU->getQP( 0 );
      xCheckDQP( rpcTempCU );
#ifdef SIG_RRU
      g_sigpool.is_skip = true;
#endif
#ifdef CVI_QP_MAP
      // Support QP MAP CU16 skip flag
      // Force to change skip mode if the top-left skip flag is 1 in CU32 and in CU16.
      int isForceSkip = 0;
      if (isQpMapEnable() || isEnableSmartEncode())
      {
        UInt cu_width = rpcTempCU->getWidth(0);
        if (cu_width == 32 || cu_width == 16)
        {
          UInt cu_x = rpcTempCU->getCUPelX();
          UInt cu_y = rpcTempCU->getCUPelY();
          int temp = 0;
          getQpMapBlk16(cu_x, cu_y, &isForceSkip, (QP_MAP_DELTA_MODE *)(&temp), &temp);
        }
      }


      if (isForceSkip > 0)
        xCheckBestModeOrForceDecision(rpcBestCU, rpcTempCU, uhDepth, false, FORCE_CHANGE);
      else
#endif
      xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(bestStr) DEBUG_STRING_PASS_INTO(tmpStr), CVI_RD_COST, true);
#ifdef SIG_RRU
      g_sigpool.is_skip = false;
#endif
      rpcTempCU->initEstData( uhDepth, orgQP, bTransquantBypassFlag );
    }
  }
  DEBUG_STRING_APPEND(sDebug, bestStr)
}
#endif //CVI_ALGO_CFG

/** check RD costs for a CU block encoded with merge
 * \param rpcBestCU
 * \param rpcTempCU
 * \param earlyDetectionSkipMode
 */
Void TEncCu::xCheckRDCostMerge2Nx2N( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU DEBUG_STRING_FN_DECLARE(sDebug), Bool *earlyDetectionSkipMode )
{
  assert( rpcTempCU->getSlice()->getSliceType() != I_SLICE );
  if(getFastDeltaQp())
  {
    return;   // never check merge in fast deltaqp mode
  }
  TComMvField  cMvFieldNeighbours[2 * MRG_MAX_NUM_CANDS]; // double length for mv of both lists
  UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
  Int numValidMergeCand = 0;
  const Bool bTransquantBypassFlag = rpcTempCU->getCUTransquantBypass(0);

  for( UInt ui = 0; ui < rpcTempCU->getSlice()->getMaxNumMergeCand(); ++ui )
  {
    uhInterDirNeighbours[ui] = 0;
  }
  UChar uhDepth = rpcTempCU->getDepth( 0 );
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
#if MCTS_ENC_CHECK
  UInt numSpatialMergeCandidates = 0;
  rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand, numSpatialMergeCandidates );
#else
  rpcTempCU->getInterMergeCandidates( 0, 0, cMvFieldNeighbours,uhInterDirNeighbours, numValidMergeCand );
#endif

  Int mergeCandBuffer[MRG_MAX_NUM_CANDS];
  for( UInt ui = 0; ui < numValidMergeCand; ++ui )
  {
    mergeCandBuffer[ui] = 0;
  }
#if MCTS_ENC_CHECK
  if (m_pcEncCfg->getTMCTSSEITileConstraint() && rpcTempCU->isLastColumnCTUInTile())
  {
    numValidMergeCand = numSpatialMergeCandidates;
  }
#endif

  Bool bestIsSkip = false;

  UInt iteration;
  if ( rpcTempCU->isLosslessCoded(0))
  {
    iteration = 1;
  }
  else
  {
    iteration = 2;
  }
  DEBUG_STRING_NEW(bestStr)

#ifdef CVI_ALGO_CFG
  Bool DisableMerge = (rpcBestCU->getWidth(0) >= 32) && g_algo_cfg.DisableMerge32;
  Bool DisableSkip = (rpcBestCU->getWidth(0) >= 32) && g_algo_cfg.DisableSkip32;
#endif
  for( UInt uiNoResidual = 0; uiNoResidual < iteration; ++uiNoResidual )
  {
#ifdef CVI_ALGO_CFG
    //uiNoResidual = 0: merge mode, uiNoResidual = 1: skip mode
    if (DisableMerge && uiNoResidual == 0)
      continue;
    if (DisableSkip && uiNoResidual == 1)
      continue;
#endif
    for( UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand )
    {
      if(!(uiNoResidual==1 && mergeCandBuffer[uiMergeCand]==1))
      {
        if( !(bestIsSkip && uiNoResidual == 0) )
        {
          DEBUG_STRING_NEW(tmpStr)
          // set MC parameters
          rpcTempCU->setPredModeSubParts( MODE_INTER, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setCUTransquantBypassSubParts( bTransquantBypassFlag, 0, uhDepth );
          rpcTempCU->setChromaQpAdjSubParts( bTransquantBypassFlag ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uhDepth );
          rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setMergeFlagSubParts( true, 0, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setMergeIndexSubParts( uiMergeCand, 0, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->setInterDirSubParts( uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth ); // interprets depth relative to CTU level
          rpcTempCU->getCUMvField( REF_PIC_LIST_0 )->setAllMvField( cMvFieldNeighbours[0 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level
          rpcTempCU->getCUMvField( REF_PIC_LIST_1 )->setAllMvField( cMvFieldNeighbours[1 + 2*uiMergeCand], SIZE_2Nx2N, 0, 0 ); // interprets depth relative to rpcTempCU level

#if MCTS_ENC_CHECK
          if ( m_pcEncCfg->getTMCTSSEITileConstraint () && (!(m_pcPredSearch->checkTMctsMvp(rpcTempCU))))
          {
            continue;
          }

#endif
          // do MC
          m_pcPredSearch->motionCompensation ( rpcTempCU, m_ppcPredYuvTemp[uhDepth] );
          // estimate residual and encode everything
          m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU,
                                                     m_ppcOrigYuv    [uhDepth],
                                                     m_ppcPredYuvTemp[uhDepth],
                                                     m_ppcResiYuvTemp[uhDepth],
                                                     m_ppcResiYuvBest[uhDepth],
                                                     m_ppcRecoYuvTemp[uhDepth],
                                                     (uiNoResidual != 0) DEBUG_STRING_PASS_INTO(tmpStr) );

#if DEBUG_STRING
          DebugInterPredResiReco(tmpStr, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif

          if ((uiNoResidual == 0) && (rpcTempCU->getQtRootCbf(0) == 0))
          {
            // If no residual when allowing for one, then set mark to not try case where residual is forced to 0
            mergeCandBuffer[uiMergeCand] = 1;
          }

          Int orgQP = rpcTempCU->getQP( 0 );
          xCheckDQP( rpcTempCU );
          xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(bestStr) DEBUG_STRING_PASS_INTO(tmpStr));

          rpcTempCU->initEstData( uhDepth, orgQP, bTransquantBypassFlag );

          if( m_pcEncCfg->getUseFastDecisionForMerge() && !bestIsSkip )
          {
            bestIsSkip = rpcBestCU->getQtRootCbf(0) == 0;
          }
        }
      }
    }

    if(uiNoResidual == 0 && m_pcEncCfg->getUseEarlySkipDetection())
    {
      if(rpcBestCU->getQtRootCbf( 0 ) == 0)
      {
        if( rpcBestCU->getMergeFlag( 0 ))
        {
          *earlyDetectionSkipMode = true;
        }
        else if(m_pcEncCfg->getMotionEstimationSearchMethod() != MESEARCH_SELECTIVE)
        {
          Int absoulte_MV=0;
          for ( UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++ )
          {
            if ( rpcBestCU->getSlice()->getNumRefIdx( RefPicList( uiRefListIdx ) ) > 0 )
            {
              TComCUMvField* pcCUMvField = rpcBestCU->getCUMvField(RefPicList( uiRefListIdx ));
              Int iHor = pcCUMvField->getMvd( 0 ).getAbsHor();
              Int iVer = pcCUMvField->getMvd( 0 ).getAbsVer();
              absoulte_MV+=iHor+iVer;
            }
          }

          if(absoulte_MV == 0)
          {
            *earlyDetectionSkipMode = true;
          }
        }
      }
    }
  }
  DEBUG_STRING_APPEND(sDebug, bestStr)
}


#if AMP_MRG
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize DEBUG_STRING_FN_DECLARE(sDebug), Bool bUseMRG)
#else
Void TEncCu::xCheckRDCostInter( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize )
#endif
{
  DEBUG_STRING_NEW(sTest)

  if(getFastDeltaQp())
  {
    const TComSPS &sps=*(rpcTempCU->getSlice()->getSPS());
    const UInt fastDeltaQPCuMaxSize = Clip3(sps.getMaxCUHeight()>>(sps.getLog2DiffMaxMinCodingBlockSize()), sps.getMaxCUHeight(), 32u);
    if(ePartSize != SIZE_2Nx2N || rpcTempCU->getWidth( 0 ) > fastDeltaQPCuMaxSize)
    {
      return; // only check necessary 2Nx2N Inter in fast deltaqp mode
    }
  }
  // prior to this, rpcTempCU will have just been reset using rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
  UChar uhDepth = rpcTempCU->getDepth( 0 );

  rpcTempCU->setPartSizeSubParts  ( ePartSize,  0, uhDepth );
  rpcTempCU->setPredModeSubParts  ( MODE_INTER, 0, uhDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uhDepth );

#if MCTS_ENC_CHECK
  rpcTempCU->setTMctsMvpIsValid(true);
#endif

#if AMP_MRG
  rpcTempCU->setMergeAMP (true);
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] DEBUG_STRING_PASS_INTO(sTest), false, bUseMRG );
#else
  m_pcPredSearch->predInterSearch ( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] );
#endif

#if AMP_MRG
  if ( !rpcTempCU->getMergeAMP() )
  {
    return;
  }
#endif

#if MCTS_ENC_CHECK
  if (m_pcEncCfg->getTMCTSSEITileConstraint() && (!rpcTempCU->getTMctsMvpIsValid()))
  {
    return;
  }
#endif

  m_pcPredSearch->encodeResAndCalcRdInterCU( rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvBest[uhDepth], m_ppcRecoYuvTemp[uhDepth], false DEBUG_STRING_PASS_INTO(sTest) );
  rpcTempCU->getTotalCost()  = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

#if DEBUG_STRING
  DebugInterPredResiReco(sTest, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif
#if defined(SIG_IRPU) || defined(SIG_CCU)
  if (g_sigdump.irpu || g_sigdump.ccu)
  {
    sigdump_me_cu_golden_backup(rpcTempCU, rpcTempCU->getMVPIdx(REF_PIC_LIST_0, 0));
    if (g_sigdump.ccu)
      sigdump_ccu_cu_golden_start("FME");
    if (g_sigdump.irpu)
      sigdump_irpu_cu_golden_start("AMVP");
  }
  if (g_sigdump.ccu || g_sigdump.rdo_sse)
  {
    int blk_size = g_aucConvertToBit[rpcTempCU->getWidth(0)];
    if (rpcTempCU->getTotalLcCost() < rpcBestCU->getTotalLcCost()) {
      memcpy(&g_sigpool.est_golden[blk_size][1], &g_sigpool.est_golden[blk_size][0], sizeof(sig_est_golden_st));
    }
  }
#endif

#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    for (int i = 0; i < MAX_NUM_COMPONENT; i++)
    {
      ComponentID compID = (ComponentID)(i);
      int pu_size = (compID == COMPONENT_Y) ? g_sigpool.cu_width : (g_sigpool.cu_width >> 1);
      Pel *p_pred_buf = m_ppcPredYuvTemp[uhDepth]->getAddr(compID, 0);
      int stride = m_ppcPredYuvTemp[uhDepth]->getStride(compID);

      sig_rru_copy_intp_temp_buf(compID, p_pred_buf, stride, g_sigpool.p_rru_intp_final, pu_size, pu_size);

      if (compID == COMPONENT_Y)
      {
        sig_rru_output_intp(false, g_sigpool.cu_width, false);
      }
      if (compID == COMPONENT_Cr)
      {
        sig_rru_output_intp(false, g_sigpool.cu_width, true);
      }
    }
  }
#endif

  xCheckDQP( rpcTempCU );
  xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTest), isEnableLC_RdCostInter() ? CVI_LC_COST : CVI_RD_COST);

#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    for (int i = 0; i < MAX_NUM_COMPONENT; i++)
    {
      ComponentID compID = (ComponentID)(i);
      if (compID == COMPONENT_Y)
        sig_rru_output_dct(false, g_sigpool.cu_width, false);
      else if (compID == COMPONENT_Cr)
        sig_rru_output_dct(false, g_sigpool.cu_width, true);
    }
  }
#endif

#ifdef CVI_CACHE_MODEL
  if (isEnableCACHE_MODEL())
  {
    if( !rpcBestCU->getMergeFlag(0) && !rpcBestCU->getSkipFlag(0) && rpcBestCU->getPredictionMode(0) == MODE_INTER)
    {
      miu_access_mc(rpcBestCU);
    }
    miu_access_mc(rpcBestCU, true); // for Chroma
  }
#endif

}

Void TEncCu::xCheckRDCostIntra( TComDataCU *&rpcBestCU,
                                TComDataCU *&rpcTempCU,
                                PartSize     eSize
                                DEBUG_STRING_FN_DECLARE(sDebug) )
{
  DEBUG_STRING_NEW(sTest)

  if(getFastDeltaQp())
  {
    const TComSPS &sps=*(rpcTempCU->getSlice()->getSPS());
    const UInt fastDeltaQPCuMaxSize = Clip3(sps.getMaxCUHeight()>>(sps.getLog2DiffMaxMinCodingBlockSize()), sps.getMaxCUHeight(), 32u);
    if(rpcTempCU->getWidth( 0 ) > fastDeltaQPCuMaxSize)
    {
      return; // only check necessary 2Nx2N Intra in fast deltaqp mode
    }
  }

  UInt uiDepth = rpcTempCU->getDepth( 0 );

  rpcTempCU->setSkipFlagSubParts( false, 0, uiDepth );

  rpcTempCU->setPartSizeSubParts( eSize, 0, uiDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTRA, 0, uiDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uiDepth );

  Pel resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE];
#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    g_sigpool.rru_is_record_si = true;
  }
#endif
#ifdef SIG_IAPU
  if (g_sigdump.iapu_ref_smp_mux){          //HW MUX compare
    iapu_i8_mux_gen.generate(8, 8, 4);
    iapu_i4_mux_gen.generate(4, 8, 4);
    iapu_i16_mux_gen.generate(16, 8, 8);
  }
#endif

  m_pcPredSearch->estIntraPredLumaQT( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], resiLuma DEBUG_STRING_PASS_INTO(sTest) );

  m_ppcRecoYuvTemp[uiDepth]->copyToPicComponent(COMPONENT_Y, rpcTempCU->getPic()->getPicYuvRec(), rpcTempCU->getCtuRsAddr(), rpcTempCU->getZorderIdxInCtu() );

  if (rpcBestCU->getPic()->getChromaFormat()!=CHROMA_400)
  {
    m_pcPredSearch->estIntraPredChromaQT( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], resiLuma DEBUG_STRING_PASS_INTO(sTest) );
  }
#ifdef SIG_RRU
  g_sigpool.rru_is_record_si = false;
#endif

  m_pcEntropyCoder->resetBits();

  if ( rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnabledFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( rpcTempCU, 0,          true );
  }
#ifdef SIG_CCU
  if (g_sigdump.ccu) {
    g_sigpool.ccu_is_record = true;
  }
#endif
  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePredMode( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePartSize( rpcTempCU, 0, uiDepth, true );
  m_pcEntropyCoder->encodePredInfo( rpcTempCU, 0 );
  m_pcEntropyCoder->encodeIPCMInfo(rpcTempCU, 0, true );
#ifdef SIG_CCU
  if (g_sigdump.ccu) {
    UInt ruiNoCoeffBits = m_pcEntropyCoder->getNumberOfWrittenBits();
    UInt ruiNoCoeffBins = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    g_sigpool.intra_bit_count[eSize == SIZE_NxN ? 0 : 1] = ruiNoCoeffBits;
    g_sigpool.intra_bin_count[eSize == SIZE_NxN ? 0 : 1] = ruiNoCoeffBins;
    g_sigpool.intra_golden.size_idx = eSize == SIZE_NxN ? 0 : sig_pat_blk_size_4(rpcTempCU->getWidth( 0 ));
    g_sigpool.ccu_is_record = false;
  }
#endif
#ifdef SIG_IAPU
  if (g_sigdump.iapu)
  {
    sig_iapu_output_iap_syntax(rpcTempCU, eSize, m_pcRdCost);
  }
#endif
  // Encode Coefficients
  Bool bCodeDQP = getdQPFlag();
  Bool codeChromaQpAdjFlag = getCodeChromaQpAdjFlag();
#ifdef SIG_BIT_EST
  if (g_sigdump.bit_est)
    g_sigpool.enable_bit_est = true;
#endif
  m_pcEntropyCoder->encodeCoeff( rpcTempCU, 0, uiDepth, bCodeDQP, codeChromaQpAdjFlag );
#ifdef SIG_BIT_EST
  if (g_sigdump.bit_est)
    g_sigpool.enable_bit_est = false;
#endif
  setCodeChromaQpAdjFlag( codeChromaQpAdjFlag );
  setdQPFlag( bCodeDQP );

  m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

#ifdef CVI_INTRA_PRED
  if (eSize != SIZE_NxN)
  {
    rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
    rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
    rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
  }
#else
  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
  rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
  rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );
#endif

  xCheckDQP( rpcTempCU );

#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    int idx = 0;
    if (eSize != SIZE_NxN)
    {
      g_sigpool.rru_gold.sse[1] = rpcTempCU->getTotalDistortion();
      idx = (g_sigpool.cu_width == 8) ? 1 : 2;
    }
    g_sigpool.rru_intra_cost[idx]    = rpcTempCU->getTotalCost();
    g_sigpool.rru_intra_cost_wt[idx] = rpcTempCU->getTotalCost();

    if (eSize == SIZE_NxN)
    {
      TComYuv *p_rec_yuv = m_ppcRecoYuvTemp[uiDepth];

      for (int comp = 0; comp < 3; comp++)
      {
        ComponentID id = ComponentID(comp);
        int width  = p_rec_yuv->getWidth(id);
        int stride = p_rec_yuv->getStride(id);
        const Pel* p_rec = p_rec_yuv->getAddr(id, 0, width);
        sig_rru_output_i4_rec(id, p_rec, stride);
      }
    }
  }
#endif //~SIG_RRU
#ifdef SIG_CCU
  if (g_sigdump.ccu) {
    cvi_fill_scan_idx(eSize == SIZE_NxN, rpcTempCU);
    sigdump_ccu_cu_golden_start("INTRA");
  }
#endif

#ifdef CVI_CU_PENALTY
  if (g_algo_cfg.CostPenaltyCfg.EnableCuCost || g_algo_cfg.CostPenaltyCfg.EnableForeground)
  {
    Double &cost = rpcTempCU->getTotalCost();
    rpcTempCU->getUnweightedTotalCost() = cost;
    int cu_x = rpcTempCU->getCUPelX();
    int cu_y = rpcTempCU->getCUPelY();
    int cu_width = rpcTempCU->getWidth(0);
    calcCuPenaltyCost(CU_PENALTY_INTRA, cost, cu_width, cu_x, cu_y,
                      (rpcTempCU->getSlice()->getSliceType() == I_SLICE));

#ifdef SIG_RRU
    if (g_sigdump.rru)
    {
      int idx = (cu_width > 8) ? 2 : eSize == SIZE_2Nx2N ? 1 : 0;
      g_sigpool.rru_intra_cost_wt[idx] = cost;
    }
#endif
  }
#endif //~CVI_CU_PENALTY

#ifdef SIG_IAPU
  if (g_sigdump.iapu)           //copy from RRU above
  {
    int idx = 0;
    if (eSize != SIZE_NxN)
    {
      idx = (g_sigpool.cu_width == 8) ? 1 : 2;
    }
    g_sigpool.iapu_st.iapu_intra_cost[idx]  = rpcTempCU->getTotalCost();
    g_sigpool.iapu_st.iapu_intra_dir[idx]   = rpcTempCU->getIntraDir(CHANNEL_TYPE_LUMA)[0];

    TComYuv *p_rec_yuv = m_ppcRecoYuvTemp[uiDepth];

    for (int comp = 0; comp < 3; comp++)
    {
      ComponentID id = ComponentID(comp);
      int width  = p_rec_yuv->getWidth(id);
      int stride = p_rec_yuv->getStride(id);
      const Pel* p_rec = p_rec_yuv->getAddr(id, 0, width);

      if (eSize == SIZE_NxN)
        sig_iapu_output_i4_rec(id, p_rec, stride);
      else
        sig_iapu_output_rec(id, p_rec, width);
    }

    if (eSize == SIZE_NxN)
    {
      sig_iapu_output_i4_pred();
    }
  }
#endif //~SIG_IAPU

#ifdef CVI_CU_PENALTY
  if (g_algo_cfg.CostPenaltyCfg.EnableCuCost || g_algo_cfg.CostPenaltyCfg.EnableForeground)
  {
    ForceDecision decision = FORCE_BYPASS;
    int best_cu_mode  = rpcBestCU->getPredictionMode(0);

    if (eSize == SIZE_NxN && best_cu_mode == MODE_INTRA)
    {
      Double best_uwt_cost = rpcBestCU->getUnweightedTotalCost();
      Double Temp_uwt_cost = rpcTempCU->getUnweightedTotalCost();

      if (Temp_uwt_cost < best_uwt_cost)
        decision = FORCE_CHANGE;
    }
    xCheckBestModeOrForceDecision(rpcBestCU, rpcTempCU, uiDepth, false, decision);
    return;
  }
#endif

  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTest));
}


/** Check R-D costs for a CU with PCM mode.
 * \param rpcBestCU pointer to best mode CU data structure
 * \param rpcTempCU pointer to testing mode CU data structure
 * \returns Void
 *
 * \note Current PCM implementation encodes sample values in a lossless way. The distortion of PCM mode CUs are zero. PCM mode is selected
 if the best mode yields bits greater than that of PCM mode.
 */
Void TEncCu::xCheckIntraPCM( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU )
{
  if(getFastDeltaQp())
  {
    const TComSPS &sps=*(rpcTempCU->getSlice()->getSPS());
    const UInt fastDeltaQPCuMaxPCMSize = Clip3((UInt)1<<sps.getPCMLog2MinSize(), (UInt)1<<sps.getPCMLog2MaxSize(), 32u);
    if (rpcTempCU->getWidth( 0 ) > fastDeltaQPCuMaxPCMSize)
    {
      return;   // only check necessary PCM in fast deltaqp mode
    }
  }

  UInt uiDepth = rpcTempCU->getDepth( 0 );

  rpcTempCU->setSkipFlagSubParts( false, 0, uiDepth );

  rpcTempCU->setIPCMFlag(0, true);
  rpcTempCU->setIPCMFlagSubParts (true, 0, rpcTempCU->getDepth(0));
  rpcTempCU->setPartSizeSubParts( SIZE_2Nx2N, 0, uiDepth );
  rpcTempCU->setPredModeSubParts( MODE_INTRA, 0, uiDepth );
  rpcTempCU->setTrIdxSubParts ( 0, 0, uiDepth );
  rpcTempCU->setChromaQpAdjSubParts( rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uiDepth );

  m_pcPredSearch->IPCMSearch( rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth]);

  m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

  m_pcEntropyCoder->resetBits();

  if ( rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnabledFlag())
  {
    m_pcEntropyCoder->encodeCUTransquantBypassFlag( rpcTempCU, 0,          true );
  }

  m_pcEntropyCoder->encodeSkipFlag ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePredMode ( rpcTempCU, 0,          true );
  m_pcEntropyCoder->encodePartSize ( rpcTempCU, 0, uiDepth, true );
  m_pcEntropyCoder->encodeIPCMInfo ( rpcTempCU, 0, true );

  m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

  rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
  rpcTempCU->getTotalBins() = ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
  rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost( rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion() );

  xCheckDQP( rpcTempCU );
  DEBUG_STRING_NEW(a)
  DEBUG_STRING_NEW(b)
  xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(a) DEBUG_STRING_PASS_INTO(b));
}

/** check whether current try is the best with identifying the depth of current try
 * \param rpcBestCU
 * \param rpcTempCU
 * \param uiDepth
 */
Void TEncCu::xCheckBestMode( TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth DEBUG_STRING_FN_DECLARE(sParent) DEBUG_STRING_FN_DECLARE(sTest) DEBUG_STRING_PASS_INTO(Bool bAddSizeInfo), CVI_CostType cost_type, Bool change_include_eq_cost)
{
#ifdef CVI_LC_RD_COST
  Double       TotalCost = 0;
  Double       TotalBestCost = 0;

  if (cost_type == CVI_RD_COST) {
    TotalCost = rpcTempCU->getTotalCost();
    TotalBestCost = rpcBestCU->getTotalCost();
  } else if (cost_type == CVI_LC_COST) {
    TotalCost = rpcTempCU->getTotalLcCost();
    TotalBestCost = rpcBestCU->getTotalLcCost();
  } else if (cost_type == CVI_LC_SAD_COST) {
    TotalCost = rpcTempCU->getTotalLcSADCost();
    TotalBestCost = rpcBestCU->getTotalLcSADCost();
  }

  if( (change_include_eq_cost && (TotalCost == TotalBestCost)) || (TotalCost < TotalBestCost) )
#else
  if( rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost() )
#endif
  {
    TComYuv* pcYuv;
    // Change Information data
    TComDataCU* pcCU = rpcBestCU;
    rpcBestCU = rpcTempCU;
    rpcTempCU = pcCU;

    // Change Prediction data
    pcYuv = m_ppcPredYuvBest[uiDepth];
    m_ppcPredYuvBest[uiDepth] = m_ppcPredYuvTemp[uiDepth];
    m_ppcPredYuvTemp[uiDepth] = pcYuv;

    // Change Reconstruction data
    pcYuv = m_ppcRecoYuvBest[uiDepth];
    m_ppcRecoYuvBest[uiDepth] = m_ppcRecoYuvTemp[uiDepth];
    m_ppcRecoYuvTemp[uiDepth] = pcYuv;

    pcYuv = NULL;
    pcCU  = NULL;

    // store temp best CI for next CU coding
    m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]->store(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);


#if DEBUG_STRING
    DEBUG_STRING_SWAP(sParent, sTest)
    const PredMode predMode=rpcBestCU->getPredictionMode(0);
    if ((DebugOptionList::DebugString_Structure.getInt()&DebugStringGetPredModeMask(predMode)) && bAddSizeInfo)
    {
      std::stringstream ss(stringstream::out);
      ss <<"###: " << (predMode==MODE_INTRA?"Intra   ":"Inter   ") << partSizeToString[rpcBestCU->getPartitionSize(0)] << " CU at " << rpcBestCU->getCUPelX() << ", " << rpcBestCU->getCUPelY() << " width=" << UInt(rpcBestCU->getWidth(0)) << std::endl;
      sParent+=ss.str();
    }
#endif

#ifdef SIG_RRU
    if (g_sigdump.rru && g_sigpool.is_skip == false)
    {
      int isInter = rpcBestCU->isInter(0);
      int cu_size = g_sigpool.cu_width;
      if (isInter)
      {
        sig_rru_update_final_buf(false, false, cu_size);
        sig_rru_update_final_buf(true, false, (cu_size >> 1));

        g_sigpool.rru_gold.sse[0] = g_sigpool.rru_temp_gold.sse[0];

        for (int i = 0; i < 3; i++)
        {
          int tu_count = (cu_size == 32) ? 4 : 1;
          for (int j = 0; j < tu_count; j++)
          {
            g_sigpool.rru_gold.rru_est_gold[0][i][j] = g_sigpool.rru_temp_gold.rru_est_gold[0][i][j];
          }
        }
      }
    }
#endif
  }
}

#ifdef CVI_FAST_CU_ENC
// force_decison 0:bypass force decision, 1: force change mode, -1: force unchange
Void TEncCu::xCheckBestModeOrForceDecision(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth,
                                           Bool isEnableLC_RdCost, ForceDecision force_decison)
{
#ifdef SIG_RRU
  if (g_sigdump.rru)
  {
    if (g_sigpool.is_skip == false)
      sig_rru_copy_cu_cost(force_decison, rpcBestCU, rpcTempCU);
  }
#endif //~SIG_RRU

  if (force_decison==FORCE_KEEP) {
    return;
  }
  else if(force_decison==FORCE_CHANGE) {
    TComYuv* pcYuv;
    TComDataCU* pcCU = rpcBestCU;
    rpcBestCU = rpcTempCU;
    rpcTempCU = pcCU;
    pcYuv = m_ppcPredYuvBest[uiDepth];
    m_ppcPredYuvBest[uiDepth] = m_ppcPredYuvTemp[uiDepth];
    m_ppcPredYuvTemp[uiDepth] = pcYuv;
    pcYuv = m_ppcRecoYuvBest[uiDepth];
    m_ppcRecoYuvBest[uiDepth] = m_ppcRecoYuvTemp[uiDepth];
    m_ppcRecoYuvTemp[uiDepth] = pcYuv;
    pcYuv = NULL;
    pcCU  = NULL;
    m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]->store(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);
  }
  else { // FORCE_BYPASS
    xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(a) DEBUG_STRING_PASS_INTO(b));
  }
}

// subCU layer partial cost check
ForceDecision TEncCu:: TermByPartialCost(TComDataCU* pCurCU, TComDataCU* pSubCU, Int width, Int subPartition)
{
  if(!(width==32 || width==16)) {
    return FORCE_BYPASS;
  }
  double subCUPartCost = 0;
  Int chkSubCuNum = (width==32) ? CU32_SUBCU_CHK_NUM : CU16_SUBCU_CHK_NUM;
  for(Int partIdx=0; partIdx<chkSubCuNum; partIdx++)
  {
    Int idx = pCurCU->getZorderIdxInCtu() + subPartition * partIdx;
    subCUPartCost += g_cu_rdcost.get_data(idx, pSubCU->getDepth(0));
  }
  if(subCUPartCost >= pCurCU->getTotalCost()) {
    return FORCE_KEEP;
  }
  else {
    return FORCE_BYPASS;
  }
}

ForceDecision TEncCu:: TerminateDecision(TComDataCU* pCurCU, TComDataCU* pSubCU, Int width, Int subPartition)
{
  if((!isEnableFastCUEnc()) || (!(width==32 || width==16))) {
    return FORCE_BYPASS;
  }

  Int cur_cu_x = pCurCU->getCUPelX();
  Int cur_cu_y = pCurCU->getCUPelY();
  Int img_w = pCurCU->getSlice()->getSPS()->getPicWidthInLumaSamples();
  Int img_h = pCurCU->getSlice()->getSPS()->getPicHeightInLumaSamples();
  g_unreliable_cu = false;
  g_is_urgent = false;

  if(((cur_cu_x+width)>img_w) || ((cur_cu_y+width)>img_h) || (g_picTsUnitCnt>=g_picMaxTargetTerm)) {
    return FORCE_BYPASS;
  }

  Int chkSubCuNum = (width==32) ? CU32_SUBCU_CHK_NUM : CU16_SUBCU_CHK_NUM;
  bool isAllSubCUNotSplit = true;
  bool isAllSubSkip = true;
  for(Int partCnt=0; partCnt<chkSubCuNum; partCnt++) {
    Int partIdx = subPartition * partCnt;
    isAllSubSkip &= pSubCU->isSkipped(partIdx);
    if(width==32) {
      isAllSubCUNotSplit &= pSubCU->getDepth(partIdx)==(pCurCU->getDepth(0)+1);
    }
    else {
      isAllSubCUNotSplit &= pSubCU->getPartitionSize(partIdx)==SIZE_2Nx2N;
    }
  }

  // colocate 3x3 CU window depth check
  cvi_grid_data<int> *cu_info_depth_info_rd = &g_cu_depth_info[g_rd_buf_idx];
  int cur_depth = (width==32) ? 0 : 1;
  int col_win_depth_cnt = 0;
  Int grid_size_shift = (width==32) ? 5 : 4;
  for(Int y=-1; y<=1; y++)
  {
    for(Int x=-1; x<=1; x++)
    {
      Int info_x = cur_cu_x + (x<<grid_size_shift);
      Int info_y = cur_cu_y + (y<<grid_size_shift);
      if(info_x<0 || info_x>=img_w || info_y<0 || info_y>=img_h) {
        col_win_depth_cnt ++;
      }
      else {
        col_win_depth_cnt += (cu_info_depth_info_rd->get_data(info_x, info_y) > cur_depth);
      }
    }
  }

  bool termByDepthChk = false;
  if(col_win_depth_cnt<=0) {
    termByDepthChk |= true;
  }

  // degenerate Merge check
  bool isCurSkip = pCurCU->isSkipped(0);
  bool isMergeZeroResi = g_isZeroResiMerge.get_data(pCurCU->getZorderIdxInCtu(), pCurCU->getDepth(0));
  bool is_smooth = (width==16)&&(g_blk_madi.get_data(cur_cu_x, cur_cu_y) < g_smooth_cu_thr);
  bool isStaticCU = isCurSkip&&isAllSubSkip&&(isMergeZeroResi||width==16);
  if (!isAllSubCUNotSplit) {
    return FORCE_BYPASS;
  }
  if(isStaticCU) {
    return FORCE_KEEP;
  }
  else if(termByDepthChk)
  {
    g_unreliable_cu = true;
    return FORCE_KEEP;
  }
  // urgent state check
  int remain_cu32_cnt = g_picCU32Num - g_picCU32Cnt;
  int predRemainTs = remain_cu32_cnt*(g_picTsUnitCnt+CU32_LAYER_TIME_UNIT)/(g_picCU32Cnt+1);
  int lackTs = g_picMinTargetTerm - g_picTsUnitCnt;
  if(predRemainTs<lackTs) {
    g_is_urgent = true;
  }
  if (g_is_urgent&&is_smooth) {
    g_unreliable_cu = true;
    return FORCE_KEEP;
  }
  return FORCE_BYPASS;
}
#endif

#ifdef CVI_FAST_CU_ENC_BY_COST
ForceDecision TEncCu::TerminateDecision(TComDataCU * p_cu)
{
  int width = p_cu->getWidth(0);
  int cu_pel_x = p_cu->getCUPelX();
  int cu_pel_y = p_cu->getCUPelY();
  int cu16_x = (cu_pel_x & 0x1f) >> 4;
  int cu16_y = (cu_pel_y & 0x1f) >> 4;

  if (width == 16 && cu16_x == 0 && cu16_y == 0)
    memset(g_cu16_save_map, 0, sizeof(g_cu16_save_map));

  if (!(width == 32 || width == 16))
    return FORCE_BYPASS;

  double cu_cost_frac = p_cu->getTotalCost();
  if (cu_cost_frac == MAX_DOUBLE)
      return FORCE_BYPASS;

  // Decision
  FastEncCuIdx idx = (width == 32) ? FE_CU32 : FE_CU16;

  if (isEnableFastEncMonitor())
  {
    if (idx == FE_CU32 && gp_fe_param->time_save_level == FE_LEVEL_2)
    {
      // Force to terminate all CU32.
      g_cu16_save_map[0][0] = 2;
      return FORCE_KEEP;
    }

    if (gp_fe_param->current_save_unit > gp_fe_param->target_save_unit)
    {
      return FORCE_BYPASS;
    }
  }

  if (gp_fe_param->fast_enc_mode == FAST_ENC_P_TC)
  {
    UInt64 cu_tc_thr = gp_fe_param->cu_tc_thr[idx];
    bool is_static_cu = true;
    int non_static_cnt = 0;
    int non_static_thr = (width >> 4) << 1;

    if (isEnableFastEncMonitor())
      cu_tc_thr = fast_enc_thr_amplifier(cu_tc_thr);

    for (int y = cu_pel_y; y < cu_pel_y + width; y += 8)
    {
      for (int x = cu_pel_x; x < cu_pel_x + width; x += 8)
      {
        int madi = g_blk8_madi.get_data(x, y);
        if (madi > cu_tc_thr)
          non_static_cnt++;
      }
    }

    if (non_static_cnt > non_static_thr)
      is_static_cu = false;

    if (is_static_cu)
    {
      if (width == 16)
        g_cu16_save_map[cu16_y][cu16_x] = 1;
      else
        g_cu16_save_map[0][0] = 2;

      return FORCE_KEEP;
    }
  }
  else if (gp_fe_param->fast_enc_mode == FAST_ENC_P_COST)
  {
    UInt64 cu_cost_thr = gp_fe_param->cu_cost_thr[idx];
    UInt64 cu_cost = (UInt64)(p_cu->getTotalCost());

    if (isEnableFastEncMonitor())
      cu_cost_thr = fast_enc_thr_amplifier(cu_cost_thr);

    //printf("[FastEnc][COST](%d, %d, %d), thr = %lld, cost = %lld\n", cu_pel_x, cu_pel_y, width, cu_cost_thr, cu_cost);

    if (cu_cost <= cu_cost_thr)
    {
      if (width == 16)
        g_cu16_save_map[cu16_y][cu16_x] = 1;
      else
        g_cu16_save_map[0][0] = 2;

      return FORCE_KEEP;
    }
  }

  return FORCE_BYPASS;
}
#endif //~CVI_FAST_CU_ENC_BY_COST

Void TEncCu::xCheckDQP( TComDataCU* pcCU )
{
  UInt uiDepth = pcCU->getDepth( 0 );

  const TComPPS &pps = *(pcCU->getSlice()->getPPS());
  if ( pps.getUseDQP() && uiDepth <= pps.getMaxCuDQPDepth() )
  {
    if ( pcCU->getQtRootCbf( 0) )
    {
      m_pcEntropyCoder->resetBits();
      m_pcEntropyCoder->encodeQP( pcCU, 0, false );
      pcCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
      pcCU->getTotalBins() += ((TEncBinCABAC *)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
      pcCU->getTotalCost() = m_pcRdCost->calcRdCost( pcCU->getTotalBits(), pcCU->getTotalDistortion() );
    }
    else
    {
      pcCU->setQPSubParts( pcCU->getRefQP( 0 ), 0, uiDepth ); // set QP to default QP
    }
  }
}

Void TEncCu::xCopyAMVPInfo (AMVPInfo* pSrc, AMVPInfo* pDst)
{
  pDst->iN = pSrc->iN;
  for (Int i = 0; i < pSrc->iN; i++)
  {
    pDst->m_acMvCand[i] = pSrc->m_acMvCand[i];
  }
}
Void TEncCu::xCopyYuv2Pic(TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsPartIdx, UInt uiDepth, UInt uiSrcDepth )
{
  UInt uiAbsPartIdxInRaster = g_auiZscanToRaster[uiAbsPartIdx];
  UInt uiSrcBlkWidth = rpcPic->getNumPartInCtuWidth() >> (uiSrcDepth);
  UInt uiBlkWidth    = rpcPic->getNumPartInCtuWidth() >> (uiDepth);
  UInt uiPartIdxX = ( ( uiAbsPartIdxInRaster % rpcPic->getNumPartInCtuWidth() ) % uiSrcBlkWidth) / uiBlkWidth;
  UInt uiPartIdxY = ( ( uiAbsPartIdxInRaster / rpcPic->getNumPartInCtuWidth() ) % uiSrcBlkWidth) / uiBlkWidth;
  UInt uiPartIdx = uiPartIdxY * ( uiSrcBlkWidth / uiBlkWidth ) + uiPartIdxX;
  m_ppcRecoYuvBest[uiSrcDepth]->copyToPicYuv( rpcPic->getPicYuvRec (), uiCUAddr, uiAbsPartIdx, uiDepth - uiSrcDepth, uiPartIdx);

  m_ppcPredYuvBest[uiSrcDepth]->copyToPicYuv( rpcPic->getPicYuvPred (), uiCUAddr, uiAbsPartIdx, uiDepth - uiSrcDepth, uiPartIdx);
}

Void TEncCu::xCopyYuv2Tmp( UInt uiPartUnitIdx, UInt uiNextDepth )
{
  UInt uiCurrDepth = uiNextDepth - 1;
  m_ppcRecoYuvBest[uiNextDepth]->copyToPartYuv( m_ppcRecoYuvTemp[uiCurrDepth], uiPartUnitIdx );
  m_ppcPredYuvBest[uiNextDepth]->copyToPartYuv( m_ppcPredYuvBest[uiCurrDepth], uiPartUnitIdx);
}

/** Function for filling the PCM buffer of a CU using its original sample array
 * \param pCU pointer to current CU
 * \param pOrgYuv pointer to original sample array
 */
Void TEncCu::xFillPCMBuffer     ( TComDataCU* pCU, TComYuv* pOrgYuv )
{
  const ChromaFormat format = pCU->getPic()->getChromaFormat();
  const UInt numberValidComponents = getNumberValidComponents(format);
  for (UInt componentIndex = 0; componentIndex < numberValidComponents; componentIndex++)
  {
    const ComponentID component = ComponentID(componentIndex);

    const UInt width  = pCU->getWidth(0)  >> getComponentScaleX(component, format);
    const UInt height = pCU->getHeight(0) >> getComponentScaleY(component, format);

    Pel *source      = pOrgYuv->getAddr(component, 0, width);
    Pel *destination = pCU->getPCMSample(component);

    const UInt sourceStride = pOrgYuv->getStride(component);

    for (Int line = 0; line < height; line++)
    {
      for (Int column = 0; column < width; column++)
      {
        destination[column] = source[column];
      }

      source      += sourceStride;
      destination += width;
    }
  }
}

#if ADAPTIVE_QP_SELECTION
/** Collect ARL statistics from one block
  */
Int TEncCu::xTuCollectARLStats(TCoeff* rpcCoeff, TCoeff* rpcArlCoeff, Int NumCoeffInCU, Double* cSum, UInt* numSamples )
{
  for( Int n = 0; n < NumCoeffInCU; n++ )
  {
    TCoeff u = abs( rpcCoeff[ n ] );
    TCoeff absc = rpcArlCoeff[ n ];

    if( u != 0 )
    {
      if( u < LEVEL_RANGE )
      {
        cSum[ u ] += ( Double )absc;
        numSamples[ u ]++;
      }
      else
      {
        cSum[ LEVEL_RANGE ] += ( Double )absc - ( Double )( u << ARL_C_PRECISION );
        numSamples[ LEVEL_RANGE ]++;
      }
    }
  }

  return 0;
}

//! Collect ARL statistics from one CTU
Void TEncCu::xCtuCollectARLStats(TComDataCU* pCtu )
{
  Double cSum[ LEVEL_RANGE + 1 ];     //: the sum of DCT coefficients corresponding to data type and quantization output
  UInt numSamples[ LEVEL_RANGE + 1 ]; //: the number of coefficients corresponding to data type and quantization output

  TCoeff* pCoeffY = pCtu->getCoeff(COMPONENT_Y);
  TCoeff* pArlCoeffY = pCtu->getArlCoeff(COMPONENT_Y);
  const TComSPS &sps = *(pCtu->getSlice()->getSPS());

  const UInt uiMinCUWidth = sps.getMaxCUWidth() >> sps.getMaxTotalCUDepth(); // NOTE: ed - this is not the minimum CU width. It is the square-root of the number of coefficients per part.
  const UInt uiMinNumCoeffInCU = 1 << uiMinCUWidth;                          // NOTE: ed - what is this?

  memset( cSum, 0, sizeof( Double )*(LEVEL_RANGE+1) );
  memset( numSamples, 0, sizeof( UInt )*(LEVEL_RANGE+1) );

  // Collect stats to cSum[][] and numSamples[][]
  for(Int i = 0; i < pCtu->getTotalNumPart(); i ++ )
  {
    UInt uiTrIdx = pCtu->getTransformIdx(i);

    if(pCtu->isInter(i) && pCtu->getCbf( i, COMPONENT_Y, uiTrIdx ) )
    {
      xTuCollectARLStats(pCoeffY, pArlCoeffY, uiMinNumCoeffInCU, cSum, numSamples);
    }//Note that only InterY is processed. QP rounding is based on InterY data only.

    pCoeffY  += uiMinNumCoeffInCU;
    pArlCoeffY  += uiMinNumCoeffInCU;
  }

  for(Int u=1; u<LEVEL_RANGE;u++)
  {
    m_pcTrQuant->getSliceSumC()[u] += cSum[ u ] ;
    m_pcTrQuant->getSliceNSamples()[u] += numSamples[ u ] ;
  }
  m_pcTrQuant->getSliceSumC()[LEVEL_RANGE] += cSum[ LEVEL_RANGE ] ;
  m_pcTrQuant->getSliceNSamples()[LEVEL_RANGE] += numSamples[ LEVEL_RANGE ] ;
}
#endif

#ifdef SIG_CABAC
Void TEncCu::sig_store_cabac2ccu( TComDataCU*  pcCU)
{
  if (pcCU->getCUPelX() == 0 && pcCU->getCUPelY() == 0)
    storeCabac2CCU.clear();

  sig_cabac2ccu_st cabac2ccu;

  for (int chType = 0; chType <= 1; chType++)
    for (int i = 0; i < 2; i++)
      m_pcEntropyCoder->getCbfCtx(ComponentID(chType), i, &cabac2ccu.CbfCtx[chType][i]);

  m_pcEntropyCoder->getPartSizeCtx(&cabac2ccu.PartSizeCtx);

  for (int m_c_type = 0; m_c_type <= 1; m_c_type++) {
    int m_c_cnt = 0;
    for (int chType = 0; chType <= 1; chType++) {
      for (int isIntra = 0; isIntra <= 1; isIntra++) {
        for (int Log2BlockWidth = (isIntra ? 2 : 3); Log2BlockWidth <= 4; Log2BlockWidth++) {
          if ((Log2BlockWidth - 2 - chType) < 0)
            continue;
          Int *out = (m_c_type == 0) ?
              g_significantScale[0][chType][isIntra][Log2BlockWidth - 2 - chType]
              : g_significantBias[0][chType][isIntra][Log2BlockWidth - 2 - chType];
          cabac2ccu.m_c[m_c_type][m_c_cnt][0] = out[0];
          cabac2ccu.m_c[m_c_type][m_c_cnt][1] = out[1];
          m_c_cnt++;
        }
      }
    }
  }
  storeCabac2CCU.push_back(cabac2ccu);
}

Void TEncCu::sig_dump_cabac2ccu(Int idx_x, Int idx_y)
{
  int blk32_cnt = ((idx_x % 64) == 0 ? 0 : 1) +  ((idx_y % 64) == 0 ? 0 : 2);
  sigdump_output_fprint(&g_sigpool.cabac2ccu_ctx,"# [BLK32_%d info] (%d, %d)\n", blk32_cnt, idx_x, idx_y);

  assert(storeCabac2CCU.size() > 0);
  sig_cabac2ccu_st vec = storeCabac2CCU.front();

  sigdump_output_fprint(&g_sigpool.cabac2ccu_ctx,"cbf_luma_ctx = %x, %x\n", vec.CbfCtx[0][0], vec.CbfCtx[0][1]);
  sigdump_output_fprint(&g_sigpool.cabac2ccu_ctx,"cbf_chroma_ctx = %x, %x\n", vec.CbfCtx[1][0], vec.CbfCtx[1][1]);
  sigdump_output_fprint(&g_sigpool.cabac2ccu_ctx,"part_mode_ctx = %x\n", vec.PartSizeCtx);

  for (int type = 0; type <= 1; type++) {
    for (int cnt = 0; cnt <= 8; cnt++) {
      sigdump_output_fprint(&g_sigpool.cabac2ccu_ctx,"%s = %x, %x\n",
        type == 0 ? "m" : "c", vec.m_c[type][cnt][0], vec.m_c[type][cnt][1]);
    }
  }
  storeCabac2CCU.erase(storeCabac2CCU.begin());

}
#endif
//! \}
