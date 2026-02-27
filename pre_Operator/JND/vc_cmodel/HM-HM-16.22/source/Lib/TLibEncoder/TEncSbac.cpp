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

/** \file     TEncSbac.cpp
    \brief    SBAC encoder class
*/

#include "TEncTop.h"
#include "TEncSbac.h"
#include "TLibCommon/TComTU.h"

#include <map>
#include <algorithm>

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
#include "../TLibCommon/Debug.h"
#endif

#include "cvi_pattern.h"
extern int rate_nocabac;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSbac::TEncSbac()
// new structure here
: m_pcBitIf                            ( NULL )
, m_pcBinIf                            ( NULL )
, m_numContextModels                   ( 0 )
, m_cCUSplitFlagSCModel                ( 1,             1,                      NUM_SPLIT_FLAG_CTX                   , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSkipFlagSCModel                 ( 1,             1,                      NUM_SKIP_FLAG_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMergeFlagExtSCModel             ( 1,             1,                      NUM_MERGE_FLAG_EXT_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMergeIdxExtSCModel              ( 1,             1,                      NUM_MERGE_IDX_EXT_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUPartSizeSCModel                 ( 1,             1,                      NUM_PART_SIZE_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUPredModeSCModel                 ( 1,             1,                      NUM_PRED_MODE_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUIntraPredSCModel                ( 1,             1,                      NUM_INTRA_PREDICT_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUChromaPredSCModel               ( 1,             1,                      NUM_CHROMA_PRED_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUDeltaQpSCModel                  ( 1,             1,                      NUM_DELTA_QP_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUInterDirSCModel                 ( 1,             1,                      NUM_INTER_DIR_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCURefPicSCModel                   ( 1,             1,                      NUM_REF_NO_CTX                       , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMvdSCModel                      ( 1,             1,                      NUM_MV_RES_CTX                       , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUQtCbfSCModel                    ( 1,             NUM_QT_CBF_CTX_SETS,    NUM_QT_CBF_CTX_PER_SET               , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUTransSubdivFlagSCModel          ( 1,             1,                      NUM_TRANS_SUBDIV_FLAG_CTX            , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUQtRootCbfSCModel                ( 1,             1,                      NUM_QT_ROOT_CBF_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSigCoeffGroupSCModel            ( 1,             2,                      NUM_SIG_CG_FLAG_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSigSCModel                      ( 1,             1,                      NUM_SIG_FLAG_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCuCtxLastX                        ( 1,             NUM_CTX_LAST_FLAG_SETS, NUM_CTX_LAST_FLAG_XY                 , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCuCtxLastY                        ( 1,             NUM_CTX_LAST_FLAG_SETS, NUM_CTX_LAST_FLAG_XY                 , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUOneSCModel                      ( 1,             1,                      NUM_ONE_FLAG_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUAbsSCModel                      ( 1,             1,                      NUM_ABS_FLAG_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cMVPIdxSCModel                     ( 1,             1,                      NUM_MVP_IDX_CTX                      , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoMergeSCModel                   ( 1,             1,                      NUM_SAO_MERGE_FLAG_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoTypeIdxSCModel                 ( 1,             1,                      NUM_SAO_TYPE_IDX_CTX                 , m_contextModels + m_numContextModels, m_numContextModels)
, m_cTransformSkipSCModel              ( 1,             MAX_NUM_CHANNEL_TYPE,   NUM_TRANSFORMSKIP_FLAG_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_CUTransquantBypassFlagSCModel      ( 1,             1,                      NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX    , m_contextModels + m_numContextModels, m_numContextModels)
, m_explicitRdpcmFlagSCModel           ( 1,             MAX_NUM_CHANNEL_TYPE,   NUM_EXPLICIT_RDPCM_FLAG_CTX          , m_contextModels + m_numContextModels, m_numContextModels)
, m_explicitRdpcmDirSCModel            ( 1,             MAX_NUM_CHANNEL_TYPE,   NUM_EXPLICIT_RDPCM_DIR_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCrossComponentPredictionSCModel   ( 1,             1,                      NUM_CROSS_COMPONENT_PREDICTION_CTX   , m_contextModels + m_numContextModels, m_numContextModels)
, m_ChromaQpAdjFlagSCModel             ( 1,             1,                      NUM_CHROMA_QP_ADJ_FLAG_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_ChromaQpAdjIdcSCModel              ( 1,             1,                      NUM_CHROMA_QP_ADJ_IDC_CTX            , m_contextModels + m_numContextModels, m_numContextModels)
{
  assert( m_numContextModels <= MAX_NUM_CTX_MOD );
}

TEncSbac::~TEncSbac()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncSbac::getCbfStateMps(const ComponentID compID, UInt depth, UInt *StateMps)
{
  ContextModel *rcCtxModel = &m_cCUQtCbfSCModel.get( 0, toChannelType(compID), depth);
  *StateMps = rcCtxModel->getHwEntropyState();
}

Void TEncSbac::getCUPartSizeMps(UInt *StateMps)
{
  ContextModel *rcCtxModel = &m_cCUPartSizeSCModel.get( 0, 0, 0);
  *StateMps = rcCtxModel->getHwEntropyState();
}

Void TEncSbac::resetEntropy           (const TComSlice *pSlice)
{
  Int  iQp              = pSlice->getSliceQp();
  SliceType eSliceType  = pSlice->getSliceType();

  SliceType encCABACTableIdx = pSlice->getEncCABACTableIdx();
  if (!pSlice->isIntra() && (encCABACTableIdx==B_SLICE || encCABACTableIdx==P_SLICE) && pSlice->getPPS()->getCabacInitPresentFlag())
  {
    eSliceType = encCABACTableIdx;
  }

  m_cCUSplitFlagSCModel.initBuffer                ( eSliceType, iQp, (UChar*)INIT_SPLIT_FLAG );
  m_cCUSkipFlagSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_SKIP_FLAG );
  m_cCUMergeFlagExtSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_MERGE_FLAG_EXT);
  m_cCUMergeIdxExtSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_MERGE_IDX_EXT);
  m_cCUPartSizeSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_PART_SIZE );
  m_cCUPredModeSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_PRED_MODE );
  m_cCUIntraPredSCModel.initBuffer                ( eSliceType, iQp, (UChar*)INIT_INTRA_PRED_MODE );
  m_cCUChromaPredSCModel.initBuffer               ( eSliceType, iQp, (UChar*)INIT_CHROMA_PRED_MODE );
  m_cCUInterDirSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_INTER_DIR );
  m_cCUMvdSCModel.initBuffer                      ( eSliceType, iQp, (UChar*)INIT_MVD );
  m_cCURefPicSCModel.initBuffer                   ( eSliceType, iQp, (UChar*)INIT_REF_PIC );
  m_cCUDeltaQpSCModel.initBuffer                  ( eSliceType, iQp, (UChar*)INIT_DQP );
  m_cCUQtCbfSCModel.initBuffer                    ( eSliceType, iQp, (UChar*)INIT_QT_CBF );
  m_cCUQtRootCbfSCModel.initBuffer                ( eSliceType, iQp, (UChar*)INIT_QT_ROOT_CBF );
  m_cCUSigCoeffGroupSCModel.initBuffer            ( eSliceType, iQp, (UChar*)INIT_SIG_CG_FLAG );
  m_cCUSigSCModel.initBuffer                      ( eSliceType, iQp, (UChar*)INIT_SIG_FLAG );
  m_cCuCtxLastX.initBuffer                        ( eSliceType, iQp, (UChar*)INIT_LAST );
  m_cCuCtxLastY.initBuffer                        ( eSliceType, iQp, (UChar*)INIT_LAST );
  m_cCUOneSCModel.initBuffer                      ( eSliceType, iQp, (UChar*)INIT_ONE_FLAG );
  m_cCUAbsSCModel.initBuffer                      ( eSliceType, iQp, (UChar*)INIT_ABS_FLAG );
  m_cMVPIdxSCModel.initBuffer                     ( eSliceType, iQp, (UChar*)INIT_MVP_IDX );
  m_cCUTransSubdivFlagSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
  m_cSaoMergeSCModel.initBuffer                   ( eSliceType, iQp, (UChar*)INIT_SAO_MERGE_FLAG );
  m_cSaoTypeIdxSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_SAO_TYPE_IDX );
  m_cTransformSkipSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_TRANSFORMSKIP_FLAG );
  m_CUTransquantBypassFlagSCModel.initBuffer      ( eSliceType, iQp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG );
  m_explicitRdpcmFlagSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_EXPLICIT_RDPCM_FLAG);
  m_explicitRdpcmDirSCModel.initBuffer            ( eSliceType, iQp, (UChar*)INIT_EXPLICIT_RDPCM_DIR);
  m_cCrossComponentPredictionSCModel.initBuffer   ( eSliceType, iQp, (UChar*)INIT_CROSS_COMPONENT_PREDICTION  );
  m_ChromaQpAdjFlagSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_CHROMA_QP_ADJ_FLAG );
  m_ChromaQpAdjIdcSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_CHROMA_QP_ADJ_IDC );

  for (UInt statisticIndex = 0; statisticIndex < RExt__GOLOMB_RICE_ADAPTATION_STATISTICS_SETS ; statisticIndex++)
  {
    m_golombRiceAdaptationStatistics[statisticIndex] = 0;
  }

  m_pcBinIf->start();

  return;
}

/** The function does the following:
 * If current slice type is P/B then it determines the distance of initialisation type 1 and 2 from the current CABAC states and
 * stores the index of the closest table.  This index is used for the next P/B slice when cabac_init_present_flag is true.
 */
SliceType TEncSbac::determineCabacInitIdx(const TComSlice *pSlice)
{
  Int  qp              = pSlice->getSliceQp();

  if (!pSlice->isIntra())
  {
    SliceType aSliceTypeChoices[] = {B_SLICE, P_SLICE};

    UInt bestCost             = MAX_UINT;
    SliceType bestSliceType   = aSliceTypeChoices[0];
    for (UInt idx=0; idx<2; idx++)
    {
      UInt curCost          = 0;
      SliceType curSliceType  = aSliceTypeChoices[idx];

      curCost  = m_cCUSplitFlagSCModel.calcCost                ( curSliceType, qp, (UChar*)INIT_SPLIT_FLAG );
      curCost += m_cCUSkipFlagSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_SKIP_FLAG );
      curCost += m_cCUMergeFlagExtSCModel.calcCost             ( curSliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT);
      curCost += m_cCUMergeIdxExtSCModel.calcCost              ( curSliceType, qp, (UChar*)INIT_MERGE_IDX_EXT);
      curCost += m_cCUPartSizeSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_PART_SIZE );
      curCost += m_cCUPredModeSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_PRED_MODE );
      curCost += m_cCUIntraPredSCModel.calcCost                ( curSliceType, qp, (UChar*)INIT_INTRA_PRED_MODE );
      curCost += m_cCUChromaPredSCModel.calcCost               ( curSliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE );
      curCost += m_cCUInterDirSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_INTER_DIR );
      curCost += m_cCUMvdSCModel.calcCost                      ( curSliceType, qp, (UChar*)INIT_MVD );
      curCost += m_cCURefPicSCModel.calcCost                   ( curSliceType, qp, (UChar*)INIT_REF_PIC );
      curCost += m_cCUDeltaQpSCModel.calcCost                  ( curSliceType, qp, (UChar*)INIT_DQP );
      curCost += m_cCUQtCbfSCModel.calcCost                    ( curSliceType, qp, (UChar*)INIT_QT_CBF );
      curCost += m_cCUQtRootCbfSCModel.calcCost                ( curSliceType, qp, (UChar*)INIT_QT_ROOT_CBF );
      curCost += m_cCUSigCoeffGroupSCModel.calcCost            ( curSliceType, qp, (UChar*)INIT_SIG_CG_FLAG );
      curCost += m_cCUSigSCModel.calcCost                      ( curSliceType, qp, (UChar*)INIT_SIG_FLAG );
      curCost += m_cCuCtxLastX.calcCost                        ( curSliceType, qp, (UChar*)INIT_LAST );
      curCost += m_cCuCtxLastY.calcCost                        ( curSliceType, qp, (UChar*)INIT_LAST );
      curCost += m_cCUOneSCModel.calcCost                      ( curSliceType, qp, (UChar*)INIT_ONE_FLAG );
      curCost += m_cCUAbsSCModel.calcCost                      ( curSliceType, qp, (UChar*)INIT_ABS_FLAG );
      curCost += m_cMVPIdxSCModel.calcCost                     ( curSliceType, qp, (UChar*)INIT_MVP_IDX );
      curCost += m_cCUTransSubdivFlagSCModel.calcCost          ( curSliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
      curCost += m_cSaoMergeSCModel.calcCost                   ( curSliceType, qp, (UChar*)INIT_SAO_MERGE_FLAG );
      curCost += m_cSaoTypeIdxSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_SAO_TYPE_IDX );
      curCost += m_cTransformSkipSCModel.calcCost              ( curSliceType, qp, (UChar*)INIT_TRANSFORMSKIP_FLAG );
      curCost += m_CUTransquantBypassFlagSCModel.calcCost      ( curSliceType, qp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG );
      curCost += m_explicitRdpcmFlagSCModel.calcCost           ( curSliceType, qp, (UChar*)INIT_EXPLICIT_RDPCM_FLAG);
      curCost += m_explicitRdpcmDirSCModel.calcCost            ( curSliceType, qp, (UChar*)INIT_EXPLICIT_RDPCM_DIR);
      curCost += m_cCrossComponentPredictionSCModel.calcCost   ( curSliceType, qp, (UChar*)INIT_CROSS_COMPONENT_PREDICTION );
      curCost += m_ChromaQpAdjFlagSCModel.calcCost             ( curSliceType, qp, (UChar*)INIT_CHROMA_QP_ADJ_FLAG );
      curCost += m_ChromaQpAdjIdcSCModel.calcCost              ( curSliceType, qp, (UChar*)INIT_CHROMA_QP_ADJ_IDC );

      if (curCost < bestCost)
      {
        bestSliceType = curSliceType;
        bestCost      = curCost;
      }
    }
    return bestSliceType;
  }
  else
  {
    return I_SLICE;
  }
}

Void TEncSbac::codeVPS( const TComVPS* /*pcVPS*/ )
{
  assert (0);
  return;
}

Void TEncSbac::codeSPS( const TComSPS* /*pcSPS*/ )
{
  assert (0);
  return;
}

Void TEncSbac::codePPS( const TComPPS* /*pcPPS*/ )
{
  assert (0);
  return;
}

Void TEncSbac::codeSliceHeader( TComSlice* /*pcSlice*/ )
{
  assert (0);
  return;
}

Void TEncSbac::codeTilesWPPEntryPoint( TComSlice* /*pSlice*/ )
{
  assert (0);
  return;
}

Void TEncSbac::codeTerminatingBit( UInt uilsLast )
{
  m_pcBinIf->encodeBinTrm( uilsLast );
}

Void TEncSbac::codeSliceFinish()
{
  m_pcBinIf->finish();
}

Void TEncSbac::xWriteUnarySymbol( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset )
{
  m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[0] );

  if( 0 == uiSymbol)
  {
    return;
  }

  while( uiSymbol-- )
  {
    m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[ iOffset ] );
  }

  return;
}

Void TEncSbac::xWriteUnaryMaxSymbol( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol )
{
  if (uiMaxSymbol == 0)
  {
    return;
  }
#ifdef SIG_CABAC
  char syntax_name[512] = "NULL_BIN";
  if (g_sigdump.cabac && g_sigpool.cabac_is_record)
    std::strncpy( syntax_name, g_sigpool.syntax_name, sizeof(syntax_name));
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
  m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[ 0 ] );

  if ( uiSymbol == 0 )
  {
    return;
  }

  Bool bCodeLast = ( uiMaxSymbol > uiSymbol );

  while( --uiSymbol )
  {
#ifdef SIG_CABAC
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);

  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, syntax_name, sizeof(g_sigpool.syntax_name) );
  }
#endif
    m_pcBinIf->encodeBin( 1, pcSCModel[ iOffset ] );
  }
  if( bCodeLast )
  {
#ifdef SIG_CABAC
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, syntax_name, sizeof(g_sigpool.syntax_name) );
  }
#endif
    m_pcBinIf->encodeBin( 0, pcSCModel[ iOffset ] );
  }

  return;
}

Void TEncSbac::xWriteEpExGolomb( UInt uiSymbol, UInt uiCount )
{
  UInt bins = 0;
  Int numBins = 0;

  while( uiSymbol >= (UInt)(1<<uiCount) )
  {
    bins = 2 * bins + 1;
    numBins++;
    uiSymbol -= 1 << uiCount;
    uiCount  ++;
  }
  bins = 2 * bins + 0;
  numBins++;

  bins = (bins << uiCount) | uiSymbol;
  numBins += uiCount;

  assert( numBins <= 32 );
  m_pcBinIf->encodeBinsEP( bins, numBins );
#ifdef SIG_CABAC
  cabac_prof_add_bins(NON_RESI, BYPASS, numBins);
#endif
}


/** Coding of coeff_abs_level_minus3
 * \param symbol                  value of coeff_abs_level_minus3
 * \param rParam                  reference to Rice parameter
 * \param useLimitedPrefixLength
 * \param maxLog2TrDynamicRange
 */
Void TEncSbac::xWriteCoefRemainExGolomb ( UInt symbol, UInt &rParam, const Bool useLimitedPrefixLength, const Int maxLog2TrDynamicRange )
{
  Int codeNumber  = (Int)symbol;
  UInt length;

#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    sigdump_cabac_syntax_info("coeff_abs_level_remaining", 3, 7, 0, codeNumber, 0);
  }
#endif
  if (codeNumber < (COEF_REMAIN_BIN_REDUCTION << rParam))
  {
    length = codeNumber>>rParam;
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "coeff_abs_level_remaining", sizeof(g_sigpool.syntax_name) );
    }
#endif
    m_pcBinIf->encodeBinsEP( (1<<(length+1))-2 , length+1);
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "coeff_abs_level_remaining", sizeof(g_sigpool.syntax_name) );
    }
    cabac_prof_add_bins(RESIDUAL, BYPASS, length+1+rParam);
#endif
    m_pcBinIf->encodeBinsEP((codeNumber%(1<<rParam)),rParam);
  }
  else if (useLimitedPrefixLength)
  {
    const UInt maximumPrefixLength = (32 - (COEF_REMAIN_BIN_REDUCTION + maxLog2TrDynamicRange));

    UInt prefixLength = 0;
    UInt suffixLength = MAX_UINT;
    UInt codeValue    = (symbol >> rParam) - COEF_REMAIN_BIN_REDUCTION;

    if (codeValue >= ((1 << maximumPrefixLength) - 1))
    {
      prefixLength = maximumPrefixLength;
      suffixLength = maxLog2TrDynamicRange - rParam;
    }
    else
    {
      while (codeValue > ((2 << prefixLength) - 2))
      {
        prefixLength++;
      }

      suffixLength = prefixLength + 1; //+1 for the separator bit
    }

    const UInt suffix = codeValue - ((1 << prefixLength) - 1);

    const UInt totalPrefixLength = prefixLength + COEF_REMAIN_BIN_REDUCTION;
    const UInt prefix            = (1 << totalPrefixLength) - 1;
    const UInt rParamBitMask     = (1 << rParam) - 1;
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "coeff_abs_level_remaining", sizeof(g_sigpool.syntax_name) );
    }
#endif
    m_pcBinIf->encodeBinsEP(  prefix,                                        totalPrefixLength      ); //prefix
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "coeff_abs_level_remaining", sizeof(g_sigpool.syntax_name) );
    }
    cabac_prof_add_bins(RESIDUAL, BYPASS, totalPrefixLength+(suffixLength + rParam));
#endif
    m_pcBinIf->encodeBinsEP(((suffix << rParam) | (symbol & rParamBitMask)), (suffixLength + rParam)); //separator, suffix, and rParam bits
  }
  else
  {
    length = rParam;
    codeNumber  = codeNumber - ( COEF_REMAIN_BIN_REDUCTION << rParam);

    while (codeNumber >= (1<<length))
    {
      codeNumber -=  (1<<(length++));
    }
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "coeff_abs_level_remaining", sizeof(g_sigpool.syntax_name) );
    }
#endif
    m_pcBinIf->encodeBinsEP((1<<(COEF_REMAIN_BIN_REDUCTION+length+1-rParam))-2,COEF_REMAIN_BIN_REDUCTION+length+1-rParam);
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "coeff_abs_level_remaining", sizeof(g_sigpool.syntax_name) );
    }
    cabac_prof_add_bins(RESIDUAL, BYPASS, COEF_REMAIN_BIN_REDUCTION+length+1-rParam+length);
#endif
    m_pcBinIf->encodeBinsEP(codeNumber,length);
  }
}

// SBAC RD
Void  TEncSbac::load ( const TEncSbac* pSrc)
{
  this->xCopyFrom(pSrc);
}

Void  TEncSbac::loadIntraDirMode( const TEncSbac* pSrc, const ChannelType chType )
{
  m_pcBinIf->copyState( pSrc->m_pcBinIf );
  if (isLuma(chType))
  {
    this->m_cCUIntraPredSCModel      .copyFrom( &pSrc->m_cCUIntraPredSCModel       );
  }
  else
  {
    this->m_cCUChromaPredSCModel     .copyFrom( &pSrc->m_cCUChromaPredSCModel      );
  }
}


Void  TEncSbac::store( TEncSbac* pDest) const
{
  pDest->xCopyFrom( this );
}


Void TEncSbac::xCopyFrom( const TEncSbac* pSrc )
{
  m_pcBinIf->copyState( pSrc->m_pcBinIf );
  xCopyContextsFrom(pSrc);
}

Void TEncSbac::codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  Int iSymbol = pcCU->getMVPIdx(eRefList, uiAbsPartIdx);
  Int iNum = AMVP_MAX_NUM_CANDS;
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    string out_st = eRefList == REF_PIC_LIST_0 ? "mvp_l0_flag" : "mvp_l1_flag";
    std::strncpy( g_sigpool.syntax_name, out_st.c_str(), sizeof(g_sigpool.syntax_name) );
    sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0, 7, 9, iSymbol, 0);
  }
#endif
  xWriteUnaryMaxSymbol(iSymbol, m_cMVPIdxSCModel.get(0), 1, iNum-1);
}

Void TEncSbac::codePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  PartSize eSize         = pcCU->getPartitionSize( uiAbsPartIdx );
  const UInt log2DiffMaxMinCodingBlockSize = pcCU->getSlice()->getSPS()->getLog2DiffMaxMinCodingBlockSize();

  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
    if( uiDepth == log2DiffMaxMinCodingBlockSize )
    {
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        std::strncpy( g_sigpool.syntax_name, "part_mode", sizeof(g_sigpool.syntax_name) );
        sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0, 3, 3, eSize==SIZE_2Nx2N ? 0 : 1, 0);
      }
      cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
      m_pcBinIf->encodeBin(eSize==SIZE_2Nx2N ? 1 : 0, m_cCUPartSizeSCModel.get(0, 0, 0), g_cuPartFlagScale[g_scale_D]);
#ifdef SIG_RRU
      if (g_sigdump.rru && g_sigpool.rru_is_record_si == true)
      {
        int isIntra = pcCU->isIntra(uiAbsPartIdx);
        if (isIntra)
        {
          int part_mode = (eSize==SIZE_2Nx2N ? 1 : 0);
          int isLps = g_cuPartFlagIsLps[g_scale_D][part_mode];
          int si_val_0 = g_cuPartFlagScale[g_scale_D][0];
          int si_val_1 = g_cuPartFlagScale[g_scale_D][1];

          int b0 = part_mode ^ isLps;
          int b1_14  = ((b0 == 0) ? si_val_1 : si_val_0) & 0x3FFF;
          int b15_26 = ((b0 == 0) ? si_val_0 : si_val_1) & 0xFFF;
          int si_val = b0 | (b1_14 << 1) | (b15_26 << 15);

          g_sigpool.rru_temp_gold.rru_est_gold[isIntra][0][0].bitest_pm_sfi = si_val;
        }
      }
#endif //~SIG_RRU
#else
      m_pcBinIf->encodeBin( eSize == SIZE_2Nx2N? 1 : 0, m_cCUPartSizeSCModel.get( 0, 0, 0 ) );
#endif
    }
    return;
  }

#ifdef SIG_CABAC
  // only 2Nx2N for INTER
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, "part_mode", sizeof(g_sigpool.syntax_name) );
    sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0,3,3,eSize==SIZE_2Nx2N ? 0 : 1, 0);
  }
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
  switch(eSize)
  {
    case SIZE_2Nx2N:
    {
#ifdef CVI_ENABLE_RDO_BIT_EST
      m_pcBinIf->encodeBin(1, m_cCUPartSizeSCModel.get(0, 0, 0), g_cuPartFlagScale[g_scale_D]);
#else
      m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 0) );
#endif
      break;
    }
    case SIZE_2NxN:
    case SIZE_2NxnU:
    case SIZE_2NxnD:
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 1) );
      if ( pcCU->getSlice()->getSPS()->getUseAMP() && uiDepth < log2DiffMaxMinCodingBlockSize )
      {
        if (eSize == SIZE_2NxN)
        {
          m_pcBinIf->encodeBin(1, m_cCUPartSizeSCModel.get( 0, 0, 3 ));
        }
        else
        {
          m_pcBinIf->encodeBin(0, m_cCUPartSizeSCModel.get( 0, 0, 3 ));
          m_pcBinIf->encodeBinEP((eSize == SIZE_2NxnU? 0: 1));
        }
      }
      break;
    }
    case SIZE_Nx2N:
    case SIZE_nLx2N:
    case SIZE_nRx2N:
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 1) );

      if( uiDepth == log2DiffMaxMinCodingBlockSize && !( pcCU->getWidth(uiAbsPartIdx) == 8 && pcCU->getHeight(uiAbsPartIdx) == 8 ) )
      {
        m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 2) );
      }

      if ( pcCU->getSlice()->getSPS()->getUseAMP() && uiDepth < log2DiffMaxMinCodingBlockSize )
      {
        if (eSize == SIZE_Nx2N)
        {
          m_pcBinIf->encodeBin(1, m_cCUPartSizeSCModel.get( 0, 0, 3 ));
        }
        else
        {
          m_pcBinIf->encodeBin(0, m_cCUPartSizeSCModel.get( 0, 0, 3 ));
          m_pcBinIf->encodeBinEP((eSize == SIZE_nLx2N? 0: 1));
        }
      }
      break;
    }
    case SIZE_NxN:
    {
      if( uiDepth == log2DiffMaxMinCodingBlockSize && !( pcCU->getWidth(uiAbsPartIdx) == 8 && pcCU->getHeight(uiAbsPartIdx) == 8 ) )
      {
        m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
        m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 1) );
        m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 2) );
      }
      break;
    }
    default:
    {
      assert(0);
      break;
    }
  }
}


/** code prediction mode
 * \param pcCU
 * \param uiAbsPartIdx
 * \returns Void
 */
Void TEncSbac::codePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, "pred_mode_flag", sizeof(g_sigpool.syntax_name) );
    sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0,2,2,pcCU->isIntra( uiAbsPartIdx ) ? 1 : 0, 0);
  }
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
  m_pcBinIf->encodeBin( pcCU->isIntra( uiAbsPartIdx ) ? 1 : 0, m_cCUPredModeSCModel.get( 0, 0, 0), g_cuPredModeScale[g_scale_D]);
#else
  m_pcBinIf->encodeBin( pcCU->isIntra( uiAbsPartIdx ) ? 1 : 0, m_cCUPredModeSCModel.get( 0, 0, 0 ) );
#endif
}

Void TEncSbac::codeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiSymbol = pcCU->getCUTransquantBypass(uiAbsPartIdx);
  m_pcBinIf->encodeBin( uiSymbol, m_CUTransquantBypassFlagSCModel.get( 0, 0, 0 ) );
}

/** code skip flag
 * \param pcCU
 * \param uiAbsPartIdx
 * \returns Void
 */
Void TEncSbac::codeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
  UInt uiSymbol = pcCU->isSkipped( uiAbsPartIdx ) ? 1 : 0;
  UInt uiCtxSkip = pcCU->getCtxSkipFlag( uiAbsPartIdx ) ;

#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, "cu_skip_flag", sizeof(g_sigpool.syntax_name) );
    sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0,1,1,uiSymbol, uiCtxSkip);
  }
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
  m_pcBinIf->encodeBin( uiSymbol, m_cCUSkipFlagSCModel.get( 0, 0, uiCtxSkip), g_skipFlagScale[g_scale_D]);
#else
  m_pcBinIf->encodeBin( uiSymbol, m_cCUSkipFlagSCModel.get( 0, 0, uiCtxSkip ) );
#endif
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tSkipFlag" );
  DTRACE_CABAC_T( "\tuiCtxSkip: ");
  DTRACE_CABAC_V( uiCtxSkip );
  DTRACE_CABAC_T( "\tuiSymbol: ");
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\n");
}

/** code merge flag
 * \param pcCU
 * \param uiAbsPartIdx
 * \returns Void
 */
Void TEncSbac::codeMergeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  const UInt uiSymbol = pcCU->getMergeFlag( uiAbsPartIdx ) ? 1 : 0;
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, "merge_flag", sizeof(g_sigpool.syntax_name) );
    sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0,5,7,uiSymbol, 0);
  }
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
  m_pcBinIf->encodeBin( uiSymbol, *m_cCUMergeFlagExtSCModel.get( 0 ), g_cuMergeFlagScale[g_scale_D]);
#else
  m_pcBinIf->encodeBin( uiSymbol, *m_cCUMergeFlagExtSCModel.get( 0 ) );
#endif

  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tMergeFlag: " );
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\tAddress: " );
  DTRACE_CABAC_V( pcCU->getCtuRsAddr() );
  DTRACE_CABAC_T( "\tuiAbsPartIdx: " );
  DTRACE_CABAC_V( uiAbsPartIdx );
  DTRACE_CABAC_T( "\n" );
}

/** code merge index
 * \param pcCU
 * \param uiAbsPartIdx
 * \returns Void
 */
Void TEncSbac::codeMergeIndex( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiUnaryIdx = pcCU->getMergeIndex( uiAbsPartIdx );
  UInt uiNumCand = pcCU->getSlice()->getMaxNumMergeCand();
  if ( uiNumCand > 1 )
  {
    for( UInt ui = 0; ui < uiNumCand - 1; ++ui )
    {
      const UInt uiSymbol = ui == uiUnaryIdx ? 0 : 1;
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        std::strncpy( g_sigpool.syntax_name, "merge_idx", sizeof(g_sigpool.syntax_name) );
        sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0,4,6,uiSymbol, 0);
      }
#endif
      if ( ui==0 )
      {
#ifdef CVI_ENABLE_RDO_BIT_EST
        m_pcBinIf->encodeBin( uiSymbol, m_cCUMergeIdxExtSCModel.get( 0, 0, 0), g_cuMergeIdxScale[g_scale_D]);
#else
        m_pcBinIf->encodeBin( uiSymbol, m_cCUMergeIdxExtSCModel.get( 0, 0, 0 ) );
#endif
#ifdef SIG_CABAC
        cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
      }
      else
      {
        m_pcBinIf->encodeBinEP( uiSymbol );
#ifdef SIG_CABAC
        cabac_prof_add_bins(NON_RESI, BYPASS, 1);
#endif
      }
      if( uiSymbol == 0 )
      {
        break;
      }
    }
  }
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tparseMergeIndex()" );
  DTRACE_CABAC_T( "\tuiMRGIdx= " );
  DTRACE_CABAC_V( pcCU->getMergeIndex( uiAbsPartIdx ) );
  DTRACE_CABAC_T( "\n" );
}

Void TEncSbac::codeSplitFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == pcCU->getSlice()->getSPS()->getLog2DiffMaxMinCodingBlockSize() )
  {
    return;
  }

  UInt uiCtx           = pcCU->getCtxSplitFlag( uiAbsPartIdx, uiDepth );
  UInt uiCurrSplitFlag = ( pcCU->getDepth( uiAbsPartIdx ) > uiDepth ) ? 1 : 0;

  assert( uiCtx < 3 );
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, "split_cu_flag", sizeof(g_sigpool.syntax_name) );
    assert(uiDepth < CVI_MAX_CU_DEPTH);
    if (uiDepth == 0) {
      g_sigpool.split_cnt = 1;
      for (int i = 0; i < CVI_MAX_CU_DEPTH; i++)
        g_sigpool.bin_inc[i] = -1;
    } else {
      for (int i = 0; i < (CVI_MAX_CU_DEPTH - 1); i++)
        g_sigpool.bin_inc[i] = g_sigpool.bin_inc[i+1];
      if (uiCurrSplitFlag)
        g_sigpool.split_cnt = (g_sigpool.split_cnt << 1) + 1;
    }
    g_sigpool.bin_inc[CVI_MAX_CU_DEPTH - 1] = uiCtx;
    if (uiCurrSplitFlag == 0 || uiDepth == (CVI_MAX_CU_DEPTH - 1)) {
      sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx,
        "%s grp_id = %d, stx_id = %d,ctx_idx = %d, stx_val = %d, depth = %d, ctx_inc = ",
        g_sigpool.syntax_name, 0,0,0,g_sigpool.split_cnt, uiDepth);
      for (Int i = 0; i < CVI_MAX_CU_DEPTH; i++) {
        sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, " %d,", g_sigpool.bin_inc[i]);
        g_sigpool.bin_inc[i] = -1;
      }
      sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, "\n");
      g_sigpool.split_cnt = 0;
    }
  }
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
  m_pcBinIf->encodeBin(uiCurrSplitFlag, m_cCUSplitFlagSCModel.get( 0, 0, uiCtx), g_cuSplitFlagScale[g_scale_D]);
#else
  m_pcBinIf->encodeBin( uiCurrSplitFlag, m_cCUSplitFlagSCModel.get( 0, 0, uiCtx ) );
#endif

  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tSplitFlag\n" )
  return;
}

Void TEncSbac::codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, "split_transform_flag", sizeof(g_sigpool.syntax_name) );
    sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0,9,11,uiSymbol, 0);
  }
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
  m_pcBinIf->encodeBin( uiSymbol, m_cCUTransSubdivFlagSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseTransformSubdivFlag()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiSymbol )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\n" )
}

Void TEncSbac::codeIntraDirLumaAng( TComDataCU* pcCU, UInt absPartIdx, Bool isMultiple)
{
  UInt dir[4],j;
  Int preds[4][NUM_MOST_PROBABLE_MODES] = {{-1, -1, -1},{-1, -1, -1},{-1, -1, -1},{-1, -1, -1}};
  Int predIdx[4] ={ -1,-1,-1,-1};
  PartSize mode = pcCU->getPartitionSize( absPartIdx );
  UInt partNum = isMultiple?(mode==SIZE_NxN?4:1):1;
  UInt partOffset = ( pcCU->getPic()->getNumPartitionsInCtu() >> ( pcCU->getDepth(absPartIdx) << 1 ) ) >> 2;

#ifdef SIG_CCU
    if (g_sigdump.ccu && g_sigpool.ccu_is_record) {
      memset(&g_sigpool.intra_golden, 0x0, sizeof(sig_intra_golden_st));
    }
#endif

  for (j=0;j<partNum;j++)
  {
    dir[j] = pcCU->getIntraDir( CHANNEL_TYPE_LUMA, absPartIdx+partOffset*j );
    pcCU->getIntraDirPredictor(absPartIdx+partOffset*j, preds[j], COMPONENT_Y);
    for(UInt i = 0; i < NUM_MOST_PROBABLE_MODES; i++)
    {
      if(dir[j] == preds[j][i])
      {
        predIdx[j] = i;
      }
    }
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "prev_intra_luma_pred_flag", sizeof(g_sigpool.syntax_name) );
    }
    cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
#ifdef SIG_CCU
    if (g_sigdump.ccu && g_sigpool.ccu_is_record) {
      g_sigpool.intra_golden.luma_pred_mode[j] = dir[j];
      g_sigpool.intra_golden.prev_luma_pred_flag[j] = (predIdx[j]!=-1)? 1 : 0;
    }
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
    m_pcBinIf->encodeBin((predIdx[j]!=-1)? 1 : 0, m_cCUIntraPredSCModel.get(0, 0, 0), g_intraPredScale[g_scale_D]);
#else
    m_pcBinIf->encodeBin((predIdx[j] != -1)? 1 : 0, m_cCUIntraPredSCModel.get( 0, 0, 0 ) );
#endif
  }

#ifdef SIG_CABAC
  unsigned int prev_intra_luma_pred_flag_grp = 0, intra_luma_pred_mode_grp = 0;
#endif
  for (j=0;j<partNum;j++)
  {
    if(predIdx[j] != -1)
    {
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        std::strncpy( g_sigpool.syntax_name, "mpm_idx", sizeof(g_sigpool.syntax_name) );
        prev_intra_luma_pred_flag_grp |= (1 << j);
        intra_luma_pred_mode_grp |= ((predIdx[j] & 0x3) << (5 * j));
      }
      cabac_prof_add_bins(NON_RESI, BYPASS, 1);
#endif
#ifdef SIG_CCU
      if (g_sigdump.ccu && g_sigpool.ccu_is_record) {
        g_sigpool.intra_golden.mpm_idx[j] = predIdx[j];
      }
#endif
      m_pcBinIf->encodeBinEP( predIdx[j] ? 1 : 0 );
      if (predIdx[j])
      {
#ifdef SIG_CABAC
        if (g_sigdump.cabac && g_sigpool.cabac_is_record)
          std::strncpy( g_sigpool.syntax_name, "mpm_idx", sizeof(g_sigpool.syntax_name) );
        cabac_prof_add_bins(NON_RESI, BYPASS, 1);
#endif
        m_pcBinIf->encodeBinEP( predIdx[j]-1 );
      }
    }
    else
    {
      if (preds[j][0] > preds[j][1])
      {
        std::swap(preds[j][0], preds[j][1]);
      }
      if (preds[j][0] > preds[j][2])
      {
        std::swap(preds[j][0], preds[j][2]);
      }
      if (preds[j][1] > preds[j][2])
      {
        std::swap(preds[j][1], preds[j][2]);
      }
      for(Int i = (Int(NUM_MOST_PROBABLE_MODES) - 1); i >= 0; i--)
      {
        dir[j] = dir[j] > preds[j][i] ? dir[j] - 1 : dir[j];
      }
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        std::strncpy( g_sigpool.syntax_name, "rem_intra_luma_pred_mode", sizeof(g_sigpool.syntax_name) );
        intra_luma_pred_mode_grp |= ((dir[j] & 0x1f) << (5 * j));
      }
      cabac_prof_add_bins(NON_RESI, BYPASS, 5);
#endif
#ifdef SIG_CCU
      if (g_sigdump.ccu && g_sigpool.ccu_is_record) {
        g_sigpool.intra_golden.rem_luma_pred_mode[j] = dir[j];
      }
#endif
      m_pcBinIf->encodeBinsEP( dir[j], 5 );
    }
  }
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    sigdump_cabac_syntax_info("prev_intra_luma_pred_flag_grp", 1,0,4, prev_intra_luma_pred_flag_grp, 0);
    sigdump_cabac_syntax_info("intra_luma_pred_mode_grp", 1,1,0, intra_luma_pred_mode_grp, 0);
  }
#endif
  return;
}

Void TEncSbac::codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiIntraDirChroma = pcCU->getIntraDir( CHANNEL_TYPE_CHROMA, uiAbsPartIdx );

#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record)
    std::strncpy( g_sigpool.syntax_name, "intra_chroma_pred_mode", sizeof(g_sigpool.syntax_name) );
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
#ifdef SIG_CCU
  if (g_sigdump.ccu && g_sigpool.ccu_is_record) {
    g_sigpool.intra_golden.chroma_pred_mode = uiIntraDirChroma;
  }
#endif
  if( uiIntraDirChroma == DM_CHROMA_IDX )
  {
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      sigdump_cabac_syntax_info(g_sigpool.syntax_name, 1,2,5, 4, 0);
    }
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
    m_pcBinIf->encodeBin( 0, m_cCUChromaPredSCModel.get(0, 0, 0), g_chmaIntraPredScale[g_scale_D]);
#else
    m_pcBinIf->encodeBin( 0, m_cCUChromaPredSCModel.get( 0, 0, 0 ) );
#endif
#ifdef SIG_CCU
    if (g_sigdump.ccu && g_sigpool.ccu_is_record) {
      g_sigpool.intra_golden.intra_chroma_pred_mode = 4;
    }
#endif
  }
  else
  {
#ifdef CVI_ENABLE_RDO_BIT_EST
    m_pcBinIf->encodeBin( 1, m_cCUChromaPredSCModel.get(0, 0, 0), g_chmaIntraPredScale[g_scale_D]);
#else
    m_pcBinIf->encodeBin( 1, m_cCUChromaPredSCModel.get( 0, 0, 0 ) );
#endif

    UInt uiAllowedChromaDir[ NUM_CHROMA_MODE ];
    pcCU->getAllowedChromaDir( uiAbsPartIdx, uiAllowedChromaDir );

    for( Int i = 0; i < NUM_CHROMA_MODE - 1; i++ )
    {
      if( uiIntraDirChroma == uiAllowedChromaDir[i] )
      {
        uiIntraDirChroma = i;
        break;
      }
    }
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "intra_chroma_pred_mode", sizeof(g_sigpool.syntax_name) );
      sigdump_cabac_syntax_info(g_sigpool.syntax_name, 1,2,5, uiIntraDirChroma, 0);
    }
    cabac_prof_add_bins(NON_RESI, BYPASS, 2);
#endif
    m_pcBinIf->encodeBinsEP( uiIntraDirChroma, 2 );
#ifdef SIG_CCU
  if (g_sigdump.ccu && g_sigpool.ccu_is_record) {
    g_sigpool.intra_golden.intra_chroma_pred_mode = uiIntraDirChroma;
  }
#endif
  }

  return;
}


Void TEncSbac::codeInterDir( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  const UInt uiInterDir = pcCU->getInterDir( uiAbsPartIdx ) - 1;
  const UInt uiCtx      = pcCU->getCtxInterDir( uiAbsPartIdx );
  ContextModel *pCtx    = m_cCUInterDirSCModel.get( 0 );

  if (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N || pcCU->getHeight(uiAbsPartIdx) != 8 )
  {
    m_pcBinIf->encodeBin( uiInterDir == 2 ? 1 : 0, *( pCtx + uiCtx ) );
  }

  if (uiInterDir < 2)
  {
    m_pcBinIf->encodeBin( uiInterDir, *( pCtx + 4 ) );
  }

  return;
}

Void TEncSbac::codeRefFrmIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  Int iRefFrame = pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx );
  ContextModel *pCtx = m_cCURefPicSCModel.get( 0 );
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    string out_st = eRefList == REF_PIC_LIST_0 ? "ref_idx_l0" : "ref_idx_l1";
    std::strncpy( g_sigpool.syntax_name, out_st.c_str(), sizeof(g_sigpool.syntax_name) );
    sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0,6,8,( iRefFrame == 0 ? 0 : 1 ), 0);
  }
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
  m_pcBinIf->encodeBin( ( iRefFrame == 0 ? 0 : 1 ), *pCtx );

  if( iRefFrame > 0 )
  {
    UInt uiRefNum = pcCU->getSlice()->getNumRefIdx( eRefList ) - 2;
    pCtx++;
    iRefFrame--;
    for( UInt ui = 0; ui < uiRefNum; ++ui )
    {
      const UInt uiSymbol = ui == iRefFrame ? 0 : 1;
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        string out_st = eRefList == REF_PIC_LIST_0 ? "ref_idx_l0" : "ref_idx_l1";
        std::strncpy( g_sigpool.syntax_name, out_st.c_str(), sizeof(g_sigpool.syntax_name) );
        sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0,6,8,uiSymbol, 0);
      }
#endif
      if( ui == 0 )
      {
        m_pcBinIf->encodeBin( uiSymbol, *pCtx );
#ifdef SIG_CABAC
        cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
      }
      else
      {
        m_pcBinIf->encodeBinEP( uiSymbol );
#ifdef SIG_CABAC
        cabac_prof_add_bins(NON_RESI, BYPASS, 1);
#endif
      }
      if( uiSymbol == 0 )
      {
        break;
      }
    }
  }
  return;
}

Void TEncSbac::codeMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  if(pcCU->getSlice()->getMvdL1ZeroFlag() && eRefList == REF_PIC_LIST_1 && pcCU->getInterDir(uiAbsPartIdx)==3)
  {
    return;
  }

  const TComCUMvField* pcCUMvField = pcCU->getCUMvField( eRefList );
  const Int iHor = pcCUMvField->getMvd( uiAbsPartIdx ).getHor();
  const Int iVer = pcCUMvField->getMvd( uiAbsPartIdx ).getVer();
  ContextModel* pCtx = m_cCUMvdSCModel.get( 0 );

#ifdef CVI_ENABLE_RDO_BIT_EST
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, "abs_mvd_greater0_flag_0", sizeof(g_sigpool.syntax_name) );
    sigdump_cabac_syntax_info("mvdx", 2,0,15,iHor, 0);
  }
#endif
  m_pcBinIf->encodeBin( iHor != 0 ? 1 : 0, *pCtx, g_mvdScale[g_scale_D]);
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, "abs_mvd_greater0_flag_1", sizeof(g_sigpool.syntax_name) );
    sigdump_cabac_syntax_info("mvdy", 2,1,15,iVer, 0);
  }
  cabac_prof_add_bins(NON_RESI, NORMAL, 2);
#endif
  m_pcBinIf->encodeBin( iVer != 0 ? 1 : 0, *pCtx, g_mvdScale[g_scale_D]);
#else
  m_pcBinIf->encodeBin( iHor != 0 ? 1 : 0, *pCtx );
  m_pcBinIf->encodeBin( iVer != 0 ? 1 : 0, *pCtx );
#endif
  const Bool bHorAbsGr0 = iHor != 0;
  const Bool bVerAbsGr0 = iVer != 0;
  const UInt uiHorAbs   = 0 > iHor ? -iHor : iHor;
  const UInt uiVerAbs   = 0 > iVer ? -iVer : iVer;
  pCtx++;

  if( bHorAbsGr0 )
  {
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "abs_mvd_greater1_flag_0", sizeof(g_sigpool.syntax_name) );
    }
    cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
    m_pcBinIf->encodeBin( uiHorAbs > 1 ? 1 : 0, *pCtx );
  }

  if( bVerAbsGr0 )
  {
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record)
      std::strncpy( g_sigpool.syntax_name, "abs_mvd_greater1_flag_1", sizeof(g_sigpool.syntax_name) );
    cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
    m_pcBinIf->encodeBin( uiVerAbs > 1 ? 1 : 0, *pCtx );
  }

  if( bHorAbsGr0 )
  {
    if( uiHorAbs > 1 )
    {
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record)
        std::strncpy( g_sigpool.syntax_name, "abs_mvd_minus2_0", sizeof(g_sigpool.syntax_name) );
#endif
      xWriteEpExGolomb( uiHorAbs-2, 1 );
    }
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record)
      std::strncpy( g_sigpool.syntax_name, "mvd_sign_flag_0", sizeof(g_sigpool.syntax_name) );
    cabac_prof_add_bins(NON_RESI, BYPASS, 1);
#endif
    m_pcBinIf->encodeBinEP( 0 > iHor ? 1 : 0 );
  }

  if( bVerAbsGr0 )
  {
    if( uiVerAbs > 1 )
    {
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record)
        std::strncpy( g_sigpool.syntax_name, "abs_mvd_minus2_1", sizeof(g_sigpool.syntax_name) );
#endif
      xWriteEpExGolomb( uiVerAbs-2, 1 );
    }
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record)
      std::strncpy( g_sigpool.syntax_name, "mvd_sign_flag_1", sizeof(g_sigpool.syntax_name) );
    cabac_prof_add_bins(NON_RESI, BYPASS, 1);
#endif
    m_pcBinIf->encodeBinEP( 0 > iVer ? 1 : 0 );
  }

  return;
}

Void TEncSbac::codeCrossComponentPrediction( TComTU &rTu, ComponentID compID )
{
  TComDataCU *pcCU = rTu.getCU();

  if( isLuma(compID) || !pcCU->getSlice()->getPPS()->getPpsRangeExtension().getCrossComponentPredictionEnabledFlag() )
  {
    return;
  }

  const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();

  if (!pcCU->isIntra(uiAbsPartIdx) || (pcCU->getIntraDir( CHANNEL_TYPE_CHROMA, uiAbsPartIdx ) == DM_CHROMA_IDX))
  {
    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T("\tparseCrossComponentPrediction()")
    DTRACE_CABAC_T( "\tAddr=" )
    DTRACE_CABAC_V( compID )
    DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
    DTRACE_CABAC_V( uiAbsPartIdx )

    Int alpha = pcCU->getCrossComponentPredictionAlpha( uiAbsPartIdx, compID );
    ContextModel *pCtx = m_cCrossComponentPredictionSCModel.get(0, 0) + ((compID == COMPONENT_Cr) ? (NUM_CROSS_COMPONENT_PREDICTION_CTX >> 1) : 0);
    m_pcBinIf->encodeBin(((alpha != 0) ? 1 : 0), pCtx[0]);

    if (alpha != 0)
    {
      static const Int log2AbsAlphaMinus1Table[8] = { 0, 1, 1, 2, 2, 2, 3, 3 };
      assert(abs(alpha) <= 8);

      if (abs(alpha)>1)
      {
        m_pcBinIf->encodeBin(1, pCtx[1]);
        xWriteUnaryMaxSymbol( log2AbsAlphaMinus1Table[abs(alpha) - 1] - 1, (pCtx + 2), 1, 2 );
      }
      else
      {
        m_pcBinIf->encodeBin(0, pCtx[1]);
      }
      m_pcBinIf->encodeBin( ((alpha < 0) ? 1 : 0), pCtx[4] );
    }
    DTRACE_CABAC_T( "\tAlpha=" )
    DTRACE_CABAC_V( pcCU->getCrossComponentPredictionAlpha( uiAbsPartIdx, compID ) )
    DTRACE_CABAC_T( "\n" )
  }
}

Void TEncSbac::codeDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
#ifdef CVI_ENABLE_RDO_BIT_EST
  //bit est does not calculate codeDeltaQP.
  m_pcBinIf->setBitEstIncr(false);
#endif
  Int iDQp  = pcCU->getQP( uiAbsPartIdx ) - pcCU->getRefQP( uiAbsPartIdx );

  Int qpBdOffsetY =  pcCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA);
  iDQp = (iDQp + 78 + qpBdOffsetY + (qpBdOffsetY/2)) % (52 + qpBdOffsetY) - 26 - (qpBdOffsetY/2);

  UInt uiAbsDQp = (UInt)((iDQp > 0)? iDQp  : (-iDQp));
  UInt TUValue = min((Int)uiAbsDQp, CU_DQP_TU_CMAX);
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    sigdump_cabac_syntax_info("cu_qp_delta", 0,13,14,iDQp, 0);
    std::strncpy( g_sigpool.syntax_name, "cu_qp_delta_abs", sizeof(g_sigpool.syntax_name) );
  }
#endif
  xWriteUnaryMaxSymbol( TUValue, &m_cCUDeltaQpSCModel.get( 0, 0, 0 ), 1, CU_DQP_TU_CMAX);
  if( uiAbsDQp >= CU_DQP_TU_CMAX )
  {
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, "cu_qp_delta_abs", sizeof(g_sigpool.syntax_name) );
  }
    xWriteEpExGolomb( uiAbsDQp - CU_DQP_TU_CMAX, CU_DQP_EG_k );
  }

  if ( uiAbsDQp > 0)
  {
#ifdef SIG_CABAC
    cabac_prof_add_bins(NON_RESI, BYPASS, 1);
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "cu_qp_delta_sign", sizeof(g_sigpool.syntax_name) );
    }
#endif
    UInt uiSign = (iDQp > 0 ? 0 : 1);
    m_pcBinIf->encodeBinEP(uiSign);
  }
#ifdef CVI_ENABLE_RDO_BIT_EST
  m_pcBinIf->setBitEstIncr(true);
#endif
  return;
}

/** code chroma qp adjustment, converting from the internal table representation
 * \returns Void
 */
Void TEncSbac::codeChromaQpAdjustment( TComDataCU* cu, UInt absPartIdx )
{
#ifdef CVI_ENABLE_RDO_BIT_EST
  //bit est does not calculate codeChromaQpAdjustment.
  m_pcBinIf->setBitEstIncr(false);
#endif
  Int internalIdc = cu->getChromaQpAdj( absPartIdx );
  Int chromaQpOffsetListLen = cu->getSlice()->getPPS()->getPpsRangeExtension().getChromaQpOffsetListLen();
  /* internal_idc == 0 => flag = 0
   * internal_idc > 1 => code idc value (if table size warrents) */
  m_pcBinIf->encodeBin( internalIdc > 0, m_ChromaQpAdjFlagSCModel.get( 0, 0, 0 ) );

  if (internalIdc > 0 && chromaQpOffsetListLen > 1)
  {
    xWriteUnaryMaxSymbol( internalIdc - 1, &m_ChromaQpAdjIdcSCModel.get( 0, 0, 0 ), 0, chromaQpOffsetListLen - 1 );
  }
#ifdef CVI_ENABLE_RDO_BIT_EST
  m_pcBinIf->setBitEstIncr(true);
#endif
}

Void TEncSbac::codeQtCbf( TComTU &rTu, const ComponentID compID, const Bool lowestLevel )
{
  TComDataCU* pcCU = rTu.getCU();

  const UInt absPartIdx   = rTu.GetAbsPartIdxTU(compID);
  const UInt TUDepth      = rTu.GetTransformDepthRel();
        UInt uiCtx        = pcCU->getCtxQtCbf( rTu, toChannelType(compID) );
  const UInt contextSet   = toChannelType(compID);
  const UInt width        = rTu.getRect(compID).width;
  const UInt height       = rTu.getRect(compID).height;
  const Bool canQuadSplit = (width >= (MIN_TU_SIZE * 2)) && (height >= (MIN_TU_SIZE * 2));

  //             Since the CBF for chroma is coded at the highest level possible, if sub-TUs are
  //             to be coded for a 4x8 chroma TU, their CBFs must be coded at the highest 4x8 level
  //             (i.e. where luma TUs are 8x8 rather than 4x4)
  //    ___ ___
  //   |   |   | <- 4 x (8x8 luma + 4x8 4:2:2 chroma)
  //   |___|___|    each quadrant has its own chroma CBF
  //   |   |   | _ _ _ _
  //   |___|___|        |
  //   <--16--->        V
  //                   _ _
  //                  |_|_| <- 4 x 4x4 luma + 1 x 4x8 4:2:2 chroma
  //                  |_|_|    no chroma CBF is coded - instead the parent CBF is inherited
  //                  <-8->    if sub-TUs are present, their CBFs had to be coded at the parent level

  const UInt lowestTUDepth = TUDepth + ((!lowestLevel && !canQuadSplit) ? 1 : 0); //unsplittable TUs inherit their parent's CBF

  if ((width != height) && (lowestLevel || !canQuadSplit)) //if sub-TUs are present
  {
    const UInt subTUDepth        = lowestTUDepth + 1;                      //if this is the lowest level of the TU-tree, the sub-TUs are directly below. Otherwise, this must be the level above the lowest level (as specified above)
    const UInt partIdxesPerSubTU = rTu.GetAbsPartIdxNumParts(compID) >> 1;

    for (UInt subTU = 0; subTU < 2; subTU++)
    {
      const UInt subTUAbsPartIdx = absPartIdx + (subTU * partIdxesPerSubTU);
      const UInt uiCbf           = pcCU->getCbf(subTUAbsPartIdx, compID, subTUDepth);

      m_pcBinIf->encodeBin(uiCbf, m_cCUQtCbfSCModel.get(0, contextSet, uiCtx));

      DTRACE_CABAC_VL( g_nSymbolCounter++ )
      DTRACE_CABAC_T( "\tparseQtCbf()" )
      DTRACE_CABAC_T( "\tsub-TU=" )
      DTRACE_CABAC_V( subTU )
      DTRACE_CABAC_T( "\tsymbol=" )
      DTRACE_CABAC_V( uiCbf )
      DTRACE_CABAC_T( "\tctx=" )
      DTRACE_CABAC_V( uiCtx )
      DTRACE_CABAC_T( "\tetype=" )
      DTRACE_CABAC_V( compID )
      DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
      DTRACE_CABAC_V( subTUAbsPartIdx )
      DTRACE_CABAC_T( "\n" )
    }
  }
  else
  {
    const UInt uiCbf = pcCU->getCbf( absPartIdx, compID, lowestTUDepth );
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      string cbf_str = (compID == COMPONENT_Y) ? "cbf_luma" : (compID == COMPONENT_Cb) ? "cbf_cb" : "cbf_cr";
      std::strncpy( g_sigpool.syntax_name, cbf_str.c_str(), sizeof(g_sigpool.syntax_name) );
      g_sigpool.cur_depth = TUDepth;
      if (compID == COMPONENT_Y) {
        sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0, 12, 13 , uiCbf, 0);
      } else if (compID == COMPONENT_Cb) {
        sigdump_cabac_syntax_info("cbf_cb", 0, 10, 12, pcCU->getCbf( absPartIdx, COMPONENT_Cb, lowestTUDepth ), 0);
      } else if (compID == COMPONENT_Cr) {
        sigdump_cabac_syntax_info("cbf_cr", 0, 11, 12, pcCU->getCbf( absPartIdx, COMPONENT_Cr, lowestTUDepth ), 0);
      }
    }
    cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif

#ifdef CVI_ENABLE_RDO_BIT_EST
    m_pcBinIf->setEstCurrentType(compID);
    m_pcBinIf->setEstCbfBits(0, compID);
    m_pcBinIf->encodeBin( uiCbf , m_cCUQtCbfSCModel.get( 0, contextSet, uiCtx), g_cbfScale[g_scale_D][contextSet][uiCtx] );
#else
    m_pcBinIf->encodeBin( uiCbf , m_cCUQtCbfSCModel.get( 0, contextSet, uiCtx ) );
#endif

#ifdef SIG_BIT_EST
    if (g_sigdump.bit_est && g_sigpool.enable_bit_est)
    {
      Int pred_type = pcCU->isIntra(absPartIdx) ? 0 : g_sigpool.is_merge ? 1 : 2;
      sig_bit_est_st *pbit_est;
      if (g_sigpool.cu_width < 32)
        pbit_est = &g_sigpool.bit_est_golden[pred_type == 1 ? 0 : 1][pred_type][compID];
      else
        pbit_est = &g_sigpool.bit_est_golden_32[0][rTu.GetSectionNumber()][compID];
      pbit_est->tu_x = rTu.getRect(compID).x0;
      pbit_est->tu_y = rTu.getRect(compID).y0;
      pbit_est->tu_width = rTu.getRect(compID).width;
      pbit_est->tu_height = rTu.getRect(compID).height;
      pbit_est->uiCbf = uiCbf;
      pbit_est->lps = g_cbfisLps[g_scale_D][contextSet][uiCtx][uiCbf];
      pbit_est->sival[0] = g_cbfScale[g_scale_D][contextSet][uiCtx][0];
      pbit_est->sival[1] = g_cbfScale[g_scale_D][contextSet][uiCtx][1];
    }
#endif
#ifdef SIG_RRU
    if (g_sigdump.rru && g_sigpool.rru_is_record_si == true)
    {
      int isIntra = pcCU->isIntra(absPartIdx);
      // si_val
      int cbf = uiCbf;
      int isLps = g_cbfisLps[g_scale_D][contextSet][uiCtx][uiCbf];
      int si_val_0 = g_cbfScale[g_scale_D][contextSet][uiCtx][0];
      int si_val_1 = g_cbfScale[g_scale_D][contextSet][uiCtx][1];

      int b0 = cbf ^ isLps;
      int b1_14  = ((b0 == 0) ? si_val_1 : si_val_0) & 0x3FFF;
      int b15_26 = ((b0 == 0) ? si_val_0 : si_val_1) & 0xFFF;
      int si_val = b0 | (b1_14 << 1) | (b15_26 << 15);

      //int state = m_cCUQtCbfSCModel.get( 0, contextSet, uiCtx).getHwEntropyState();
      sig_rru_est_gold_st *p_rru_est_gold = &g_sigpool.rru_temp_gold.rru_est_gold[isIntra][compID][rTu.GetSectionNumber()];
      p_rru_est_gold->cbf = cbf;
      p_rru_est_gold->bitest_cbf_ctx[0] = g_cbfState[g_scale_D][contextSet][0];
      p_rru_est_gold->bitest_cbf_ctx[1] = g_cbfState[g_scale_D][contextSet][1];
      p_rru_est_gold->bitest_sfi = si_val;

      // reset coefficient related golden.
      p_rru_est_gold->bitest = 0;
      p_rru_est_gold->csbf = 0;
      p_rru_est_gold->last_x = 0;
      p_rru_est_gold->last_y = 0;
    }
#endif
    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tparseQtCbf()" )
    DTRACE_CABAC_T( "\tsymbol=" )
    DTRACE_CABAC_V( uiCbf )
    DTRACE_CABAC_T( "\tctx=" )
    DTRACE_CABAC_V( uiCtx )
    DTRACE_CABAC_T( "\tetype=" )
    DTRACE_CABAC_V( compID )
    DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
    DTRACE_CABAC_V( rTu.GetAbsPartIdxTU(compID) )
    DTRACE_CABAC_T( "\n" )
  }
}


Void TEncSbac::codeTransformSkipFlags (TComTU &rTu, ComponentID component )
{
  TComDataCU* pcCU=rTu.getCU();
  const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU();

  if (pcCU->getCUTransquantBypass(uiAbsPartIdx))
  {
    return;
  }

  if (!TUCompRectHasAssociatedTransformSkipFlag(rTu.getRect(component), pcCU->getSlice()->getPPS()->getPpsRangeExtension().getLog2MaxTransformSkipBlockSize()))
  {
    return;
  }

  UInt useTransformSkip = pcCU->getTransformSkip( uiAbsPartIdx,component);
  m_pcBinIf->encodeBin( useTransformSkip, m_cTransformSkipSCModel.get( 0, toChannelType(component), 0 ) );

  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T("\tparseTransformSkip()");
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( useTransformSkip )
  DTRACE_CABAC_T( "\tAddr=" )
  DTRACE_CABAC_V( pcCU->getCtuRsAddr() )
  DTRACE_CABAC_T( "\tetype=" )
  DTRACE_CABAC_V( component )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( rTu.GetAbsPartIdxTU() )
  DTRACE_CABAC_T( "\n" )
}


/** Code I_PCM information.
 * \param pcCU pointer to CU
 * \param uiAbsPartIdx CU index
 * \returns Void
 */
Void TEncSbac::codeIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiIPCM = (pcCU->getIPCMFlag(uiAbsPartIdx) == true)? 1 : 0;

  Bool writePCMSampleFlag = pcCU->getIPCMFlag(uiAbsPartIdx);

  m_pcBinIf->encodeBinTrm (uiIPCM);

  if (writePCMSampleFlag)
  {
    m_pcBinIf->encodePCMAlignBits();

    const UInt minCoeffSizeY = pcCU->getPic()->getMinCUWidth() * pcCU->getPic()->getMinCUHeight();
    const UInt offsetY       = minCoeffSizeY * uiAbsPartIdx;
    for (UInt ch=0; ch < pcCU->getPic()->getNumberValidComponents(); ch++)
    {
      const ComponentID compID = ComponentID(ch);
      const UInt offset = offsetY >> (pcCU->getPic()->getComponentScaleX(compID) + pcCU->getPic()->getComponentScaleY(compID));
      Pel * pPCMSample  = pcCU->getPCMSample(compID) + offset;
      const UInt width  = pcCU->getWidth (uiAbsPartIdx) >> pcCU->getPic()->getComponentScaleX(compID);
      const UInt height = pcCU->getHeight(uiAbsPartIdx) >> pcCU->getPic()->getComponentScaleY(compID);
      const UInt sampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepth(toChannelType(compID));
      for (UInt y=0; y<height; y++)
      {
        for (UInt x=0; x<width; x++)
        {
          UInt sample = pPCMSample[x];
          m_pcBinIf->xWritePCMCode(sample, sampleBits);
        }
        pPCMSample += width;
      }
    }

    m_pcBinIf->resetBac();
  }
}

Void TEncSbac::codeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiCbf = pcCU->getQtRootCbf( uiAbsPartIdx );
  UInt uiCtx = 0;

#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    std::strncpy( g_sigpool.syntax_name, "rqt_root_cbf", sizeof(g_sigpool.syntax_name) );
    sigdump_cabac_syntax_info(g_sigpool.syntax_name, 0,8,10,uiCbf, 0);
  }
  cabac_prof_add_bins(NON_RESI, NORMAL, 1);
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
  m_pcBinIf->encodeBin( uiCbf , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx), g_rootCbfScale[g_scale_D]);
#else
  m_pcBinIf->encodeBin( uiCbf , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx ) );
#endif
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtRootCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiCbf )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
}

Void TEncSbac::codeQtCbfZero( TComTU & rTu, const ChannelType chType )
{
  // this function is only used to estimate the bits when cbf is 0
  // and will never be called when writing the bistream. do not need to write log
  UInt uiCbf = 0;
  UInt uiCtx = rTu.getCU()->getCtxQtCbf( rTu, chType );

  m_pcBinIf->encodeBin( uiCbf , m_cCUQtCbfSCModel.get( 0, chType, uiCtx ) );
}

Void TEncSbac::codeQtRootCbfZero( )
{
  // this function is only used to estimate the bits when cbf is 0
  // and will never be called when writing the bistream. do not need to write log
  UInt uiCbf = 0;
  UInt uiCtx = 0;
  m_pcBinIf->encodeBin( uiCbf , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx ) );
}

/** Encode (X,Y) position of the last significant coefficient
 * \param uiPosX     X component of last coefficient
 * \param uiPosY     Y component of last coefficient
 * \param width      Block width
 * \param height     Block height
 * \param component  chroma component ID
 * \param uiScanIdx  scan type (zig-zag, hor, ver)
 * This method encodes the X and Y component within a block of the last significant coefficient.
 */
Void TEncSbac::codeLastSignificantXY( UInt uiPosX, UInt uiPosY, Int width, Int height, ComponentID component, UInt uiScanIdx
#ifdef CVI_ENABLE_RDO_BIT_EST
, UInt uiBinCnt[]
, UInt uiFractBitCnt[]
#endif
)
{
  // swap
  if( uiScanIdx == SCAN_VER )
  {
    swap( uiPosX, uiPosY );
    swap( width,  height );
  }

  UInt uiCtxLast;
  UInt uiGroupIdxX    = g_uiGroupIdx[ uiPosX ];
  UInt uiGroupIdxY    = g_uiGroupIdx[ uiPosY ];

  ContextModel *pCtxX = m_cCuCtxLastX.get( 0, toChannelType(component) );
  ContextModel *pCtxY = m_cCuCtxLastY.get( 0, toChannelType(component) );

  Int blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY;
  getLastSignificantContextParameters(component, width, height, blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY);

  //------------------
#ifdef CVI_ENABLE_RDO_BIT_EST
  UInt x_bit_pos = m_pcBinIf->getSimEncBits();
  UInt x_bin_pos = m_pcBinIf->getSimEncBits(2);
#endif
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    sigdump_cabac_syntax_info("last_sig_grp", 3,0,0,(uiPosY << 4) | uiPosX, 0);
  }
#endif
  // posX
  for( uiCtxLast = 0; uiCtxLast < uiGroupIdxX; uiCtxLast++ )
  {
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "last_sig_coeff_x_prefix", sizeof(g_sigpool.syntax_name) );
    }
#endif
    m_pcBinIf->encodeBin( 1, *( pCtxX + blkSizeOffsetX + (uiCtxLast >>shiftX) ) );
  }
  if( uiGroupIdxX < g_uiGroupIdx[ width - 1 ])
  {
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "last_sig_coeff_x_prefix", sizeof(g_sigpool.syntax_name) );
    }
#endif
    m_pcBinIf->encodeBin( 0, *( pCtxX + blkSizeOffsetX + (uiCtxLast >>shiftX) ) );
  }

#ifdef CVI_ENABLE_RDO_BIT_EST
  UInt y_bit_pos = m_pcBinIf->getSimEncBits();
  UInt y_bin_pos = m_pcBinIf->getSimEncBits(2);
  uiFractBitCnt[0] = y_bit_pos - x_bit_pos;
  uiBinCnt[0] = y_bin_pos - x_bin_pos;
#endif

  // posY

  for( uiCtxLast = 0; uiCtxLast < uiGroupIdxY; uiCtxLast++ )
  {
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "last_sig_coeff_y_prefix", sizeof(g_sigpool.syntax_name) );
    }
#endif
    m_pcBinIf->encodeBin( 1, *( pCtxY + blkSizeOffsetY + (uiCtxLast >>shiftY) ) );
  }
  if( uiGroupIdxY < g_uiGroupIdx[ height - 1 ])
  {
#ifdef SIG_CABAC
    if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
      std::strncpy( g_sigpool.syntax_name, "last_sig_coeff_y_prefix", sizeof(g_sigpool.syntax_name) );
    }
#endif
    m_pcBinIf->encodeBin( 0, *( pCtxY + blkSizeOffsetY + (uiCtxLast >>shiftY) ) );
  }

#ifdef CVI_ENABLE_RDO_BIT_EST
  uiBinCnt[1] = m_pcBinIf->getSimEncBits(2) - y_bin_pos;
  uiFractBitCnt[1] = m_pcBinIf->getSimEncBits() - y_bit_pos;
#endif

#ifdef SIG_CABAC
  cabac_prof_add_bins(RESIDUAL, NORMAL, uiGroupIdxX + (uiGroupIdxX < g_uiGroupIdx[width - 1]));
  cabac_prof_add_bins(RESIDUAL, NORMAL, uiGroupIdxY + (uiGroupIdxY < g_uiGroupIdx[height - 1]));
#endif
  // EP-coded part

  if ( uiGroupIdxX > 3 )
  {
    UInt uiCount = ( uiGroupIdxX - 2 ) >> 1;
    uiPosX       = uiPosX - g_uiMinInGroup[ uiGroupIdxX ];
    for (Int i = uiCount - 1 ; i >= 0; i-- )
    {
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        std::strncpy( g_sigpool.syntax_name, "last_sig_coeff_x_suffix", sizeof(g_sigpool.syntax_name) );
      }
#endif
      m_pcBinIf->encodeBinEP( ( uiPosX >> i ) & 1 );
    }
#ifdef SIG_CABAC
    cabac_prof_add_bins(RESIDUAL, BYPASS, uiCount);
#endif
  }
  if ( uiGroupIdxY > 3 )
  {
    UInt uiCount = ( uiGroupIdxY - 2 ) >> 1;
    uiPosY       = uiPosY - g_uiMinInGroup[ uiGroupIdxY ];
    for ( Int i = uiCount - 1 ; i >= 0; i-- )
    {
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        std::strncpy( g_sigpool.syntax_name, "last_sig_coeff_y_suffix", sizeof(g_sigpool.syntax_name) );
      }
#endif
      m_pcBinIf->encodeBinEP( ( uiPosY >> i ) & 1 );
    }
#ifdef SIG_CABAC
    cabac_prof_add_bins(RESIDUAL, BYPASS, uiCount);
#endif
  }
}


Void TEncSbac::codeCoeffNxN( TComTU &rTu, TCoeff* pcCoef, const ComponentID compID )
{
  TComDataCU* pcCU=rTu.getCU();
  const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU(compID);
  const TComRectangle &tuRect=rTu.getRect(compID);
  const UInt uiWidth=tuRect.width;
  const UInt uiHeight=tuRect.height;
  const TComSPS &sps=*(pcCU->getSlice()->getSPS());

  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseCoeffNxN()\teType=" )
  DTRACE_CABAC_V( compID )
  DTRACE_CABAC_T( "\twidth=" )
  DTRACE_CABAC_V( uiWidth )
  DTRACE_CABAC_T( "\theight=" )
  DTRACE_CABAC_V( uiHeight )
  DTRACE_CABAC_T( "\tdepth=" )
//  DTRACE_CABAC_V( rTu.GetTransformDepthTotalAdj(compID) )
  DTRACE_CABAC_V( rTu.GetTransformDepthTotal() )
  DTRACE_CABAC_T( "\tabspartidx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\ttoCU-X=" )
  DTRACE_CABAC_V( pcCU->getCUPelX() )
  DTRACE_CABAC_T( "\ttoCU-Y=" )
  DTRACE_CABAC_V( pcCU->getCUPelY() )
  DTRACE_CABAC_T( "\tCU-addr=" )
  DTRACE_CABAC_V(  pcCU->getCtuRsAddr() )
  DTRACE_CABAC_T( "\tinCU-X=" )
//  DTRACE_CABAC_V( g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_V( g_auiRasterToPelX[ g_auiZscanToRaster[rTu.GetAbsPartIdxTU(compID)] ] )
  DTRACE_CABAC_T( "\tinCU-Y=" )
// DTRACE_CABAC_V( g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_V( g_auiRasterToPelY[ g_auiZscanToRaster[rTu.GetAbsPartIdxTU(compID)] ] )
  DTRACE_CABAC_T( "\tpredmode=" )
  DTRACE_CABAC_V(  pcCU->getPredictionMode( uiAbsPartIdx ) )
  DTRACE_CABAC_T( "\n" )

  //--------------------------------------------------------------------------------------------------

  if( uiWidth > sps.getMaxTrSize() )
  {
    std::cerr << "ERROR: codeCoeffNxN was passed a TU with dimensions larger than the maximum allowed size" << std::endl;
    assert(false);
    exit(1);
  }

  // compute number of significant coefficients
  UInt uiNumSig = TEncEntropy::countNonZeroCoeffs(pcCoef, uiWidth * uiHeight);

  if ( uiNumSig == 0 )
  {
    std::cerr << "ERROR: codeCoeffNxN called for empty TU!" << std::endl;
    assert(false);
    exit(1);
  }
  //--------------------------------------------------------------------------------------------------

  //set parameters

  const ChannelType  chType            = toChannelType(compID);
  const UInt         uiLog2BlockWidth  = g_aucConvertToBit[ uiWidth  ] + 2;
  const UInt         uiLog2BlockHeight = g_aucConvertToBit[ uiHeight ] + 2;

  const ChannelType  channelType       = toChannelType(compID);
  const Bool         extendedPrecision = sps.getSpsRangeExtension().getExtendedPrecisionProcessingFlag();

  const Bool         alignCABACBeforeBypass = sps.getSpsRangeExtension().getCabacBypassAlignmentEnabledFlag();
  const Int          maxLog2TrDynamicRange  = sps.getMaxLog2TrDynamicRange(channelType);

  Bool beValid;

#ifdef SIG_RESI
  if (g_sigdump.resi && g_sigpool.resi_is_record_pos)
  {
    int  pos_x = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
    int  pos_y = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];

    if (compID != COMPONENT_Y)
    {
      pos_y = (pos_y >> 1);
    }
    sig_copy_resi_buf(compID, pcCoef, pos_x, pos_y, uiWidth);
  }
#endif //~SIG_RESI

#ifdef CVI_ENABLE_RDO_BIT_EST
  g_syntaxType = 1;
  UInt tuSig0FracBit = 0, tuSig1FracBit = 0, tuCgfFracBit = 0, estRemainCoeffBit = 0;
  UInt tuSig0Bin = 0, tuGT1Bin = 0, tuCgfBin = 0, tuGT2Bin = 0;
  UInt tuSig1Bin = uiNumSig;
  if(getRDOBitEstMode()==CTX_ADAPTIVE) {
    m_pcBinIf->setBitEstIncr(g_uiResiModelState==RESI_MODEL_TRAIN);
    m_pcBinIf->setEstCurrentType(compID);
  }
#endif

  {
    Int uiIntraMode = -1;
    const Bool       bIsLuma = isLuma(compID);
    Int isIntra = pcCU->isIntra(uiAbsPartIdx) ? 1 : 0;
    if ( isIntra )
    {
      uiIntraMode = pcCU->getIntraDir( toChannelType(compID), uiAbsPartIdx );

      const UInt partsPerMinCU = 1<<(2*(sps.getMaxTotalCUDepth() - sps.getLog2DiffMaxMinCodingBlockSize()));
      uiIntraMode = (uiIntraMode==DM_CHROMA_IDX && !bIsLuma) ? pcCU->getIntraDir(CHANNEL_TYPE_LUMA, getChromasCorrespondingPULumaIdx(uiAbsPartIdx, rTu.GetChromaFormat(), partsPerMinCU)) : uiIntraMode;
      uiIntraMode = ((rTu.GetChromaFormat() == CHROMA_422) && !bIsLuma) ? g_chroma422IntraAngleMappingTable[uiIntraMode] : uiIntraMode;
    }

    Int transformSkip = pcCU->getTransformSkip( uiAbsPartIdx,compID) ? 1 : 0;
    Bool rdpcm_lossy = ( transformSkip && isIntra && ( (uiIntraMode == HOR_IDX) || (uiIntraMode == VER_IDX) ) ) && pcCU->isRDPCMEnabled(uiAbsPartIdx);

    if ( (pcCU->getCUTransquantBypass(uiAbsPartIdx)) || rdpcm_lossy )
    {
      beValid = false;
      if ( (!pcCU->isIntra(uiAbsPartIdx)) && pcCU->isRDPCMEnabled(uiAbsPartIdx))
      {
        codeExplicitRdpcmMode( rTu, compID);
      }
    }
    else
    {
      beValid = pcCU->getSlice()->getPPS()->getSignDataHidingEnabledFlag();
    }
  }

  //--------------------------------------------------------------------------------------------------

  if(pcCU->getSlice()->getPPS()->getUseTransformSkip())
  {
    codeTransformSkipFlags(rTu, compID);
    if(pcCU->getTransformSkip(uiAbsPartIdx, compID) && !pcCU->isIntra(uiAbsPartIdx) && pcCU->isRDPCMEnabled(uiAbsPartIdx))
    {
      //  This TU has coefficients and is transform skipped. Check whether is inter coded and if yes encode the explicit RDPCM mode
      codeExplicitRdpcmMode( rTu, compID);

      if(pcCU->getExplicitRdpcmMode(compID, uiAbsPartIdx) != RDPCM_OFF)
      {
        //  Sign data hiding is avoided for horizontal and vertical explicit RDPCM modes
        beValid = false;
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  const Bool  bUseGolombRiceParameterAdaptation = sps.getSpsRangeExtension().getPersistentRiceAdaptationEnabledFlag();
        UInt &currentGolombRiceStatistic        = m_golombRiceAdaptationStatistics[rTu.getGolombRiceStatisticsIndex(compID)];

  //select scans
  TUEntropyCodingParameters codingParameters;
  getTUEntropyCodingParameters(codingParameters, rTu, compID);

  //----- encode significance map -----

  // Find position of last coefficient
  Int scanPosLast = -1;
  Int posLast;

  UInt uiSigCoeffGroupFlag[ MLS_GRP_NUM ];

  memset( uiSigCoeffGroupFlag, 0, sizeof(UInt) * MLS_GRP_NUM );
  do
  {
    posLast = codingParameters.scan[ ++scanPosLast ];

    if( pcCoef[ posLast ] != 0 )
    {
      // get L1 sig map
      UInt uiPosY   = posLast >> uiLog2BlockWidth;
      UInt uiPosX   = posLast - ( uiPosY << uiLog2BlockWidth );

      UInt uiBlkIdx = (codingParameters.widthInGroups * (uiPosY >> MLS_CG_LOG2_HEIGHT)) + (uiPosX >> MLS_CG_LOG2_WIDTH);
      uiSigCoeffGroupFlag[ uiBlkIdx ] = 1;

      uiNumSig--;
    }
  } while ( uiNumSig > 0 );

  // Code position of last coefficient
  Int posLastY = posLast >> uiLog2BlockWidth;
  Int posLastX = posLast - ( posLastY << uiLog2BlockWidth );
#ifdef SIG_CABAC
  if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
    sigdump_output_fprint(&g_sigpool.cabac_syntax_ctx, "blk_sz = %d, scan_idx = %d, comp = %d, is_intra = %d, last_x = %d, last_y = %d, csbf = %d\n",
      uiWidth, codingParameters.scanType, compID, pcCU->isIntra(uiAbsPartIdx), posLastX, posLastY, TEncEntropy::count4x4NonZeroCoeffs(pcCoef, uiWidth, uiHeight));
  }
#endif
#ifdef SIG_CCU
  if (g_sigdump.ccu && g_sigpool.ccu_is_record_cu_bits)
  {
    g_sigpool.ccu_csbf[compID] = TEncEntropy::count4x4NonZeroCoeffs(pcCoef, uiWidth, uiHeight);
    g_sigpool.ccu_scan_idx[chType] = codingParameters.scanType;
  }
#endif //~SIG_CCU
#ifdef CVI_ENABLE_RDO_BIT_EST
  UInt lastPosRegBitXY[2], lastPosRegBinXY[2];
  codeLastSignificantXY(posLastX, posLastY, uiWidth, uiHeight, compID, codingParameters.scanType, lastPosRegBinXY, lastPosRegBitXY);
#else
  codeLastSignificantXY(posLastX, posLastY, uiWidth, uiHeight, compID, codingParameters.scanType );
#endif

#ifdef CVI_ENABLE_RDO_BIT_EST
  // last position bit estimation
  UInt csbfBin = scanPosLast >> (MLS_CG_SIZE);
  UInt uiGroupIdxX = g_uiGroupIdx[posLastX];
  UInt uiGroupIdxY = g_uiGroupIdx[posLastY];
  UInt uMax = (uiLog2BlockWidth<<1) - 1;
  UInt regLastPosBin = std::min<UInt>(uiGroupIdxX + 1, uMax) + std::min<UInt>(uiGroupIdxY + 1, uMax);
  UInt remainLastPosBin = ((uiGroupIdxX > 3) ? (uiGroupIdxX - 2) >> 1 : 0)
                        + ((uiGroupIdxY > 3) ? (uiGroupIdxY- 2) >> 1 : 0);
  UInt lastPosScale = (g_lastPosBinScale_init[chType][pcCU->isIntra(uiAbsPartIdx)][uiLog2BlockWidth-2]/16.0)*g_estBitPrec;
  UInt estLastPosBit = regLastPosBin*lastPosScale + (remainLastPosBin<<g_estBitFracBd);

#if (DUMP_BIT_STATS)
  if(g_uiResiModelState==RESI_MODEL_TRAIN){
    Int isIntra = pcCU->isIntra(uiAbsPartIdx) ? 1 : 0;
    for(Int idx=0; idx<2; idx++){
      if(lastPosRegBinXY[idx]>0){
        sigdump_output_fprint(&g_sigpool.last_pos_bit_stats[chType], "%d, %d, %d, %d\n", uiWidth, isIntra, lastPosRegBitXY[idx]>>(EST_BIT_FRAC_BD-4), lastPosRegBinXY[idx]);
      }
    }
  }
#endif
#endif
  //===== code significance flag =====
  ContextModel * const baseCoeffGroupCtx = m_cCUSigCoeffGroupSCModel.get( 0, chType );
  ContextModel * const baseCtx = m_cCUSigSCModel.get( 0, 0 ) + getSignificanceMapContextOffset(compID);

  const Int  iLastScanSet  = scanPosLast >> MLS_CG_SIZE;

  UInt c1                  = 1;
  UInt uiGoRiceParam       = 0;
  Int  iScanPosSig         = scanPosLast;
#ifdef SIG_CABAC
  int csbfContiZeroCnt = 0;
#endif
#ifdef SIG_CCU
  if (g_sigdump.ccu && g_sigpool.ccu_is_record_cu_bits)
  {
    g_sigpool.ccu_last_subset[compID] = iLastScanSet;
  }
#endif //~SIG_CCU
  for( Int iSubSet = iLastScanSet; iSubSet >= 0; iSubSet-- )
  {
    Int numNonZero = 0;
    Int  iSubPos   = iSubSet << MLS_CG_SIZE;
    uiGoRiceParam  = currentGolombRiceStatistic / RExt__GOLOMB_RICE_INCREMENT_DIVISOR;
    Bool updateGolombRiceStatistics = bUseGolombRiceParameterAdaptation; //leave the statistics at 0 when not using the adaptation system
    UInt coeffSigns = 0;

    Int absCoeff[1 << MLS_CG_SIZE];

    Int lastNZPosInCG  = -1;
    Int firstNZPosInCG = 1 << MLS_CG_SIZE;

    Bool escapeDataPresentInGroup = false;

    if( iScanPosSig == scanPosLast )
    {
      absCoeff[ 0 ] = Int(abs( pcCoef[ posLast ] ));
      coeffSigns    = ( pcCoef[ posLast ] < 0 );
      numNonZero    = 1;
      lastNZPosInCG  = iScanPosSig;
      firstNZPosInCG = iScanPosSig;
      iScanPosSig--;
    }

    // encode significant_coeffgroup_flag
    Int iCGBlkPos = codingParameters.scanCG[ iSubSet ];
    Int iCGPosY   = iCGBlkPos / codingParameters.widthInGroups;
    Int iCGPosX   = iCGBlkPos - (iCGPosY * codingParameters.widthInGroups);

    if( iSubSet == iLastScanSet || iSubSet == 0)
    {
      uiSigCoeffGroupFlag[ iCGBlkPos ] = 1;
    }
    else
    {
#ifdef CVI_ENABLE_RDO_BIT_EST
      UInt64 cfg_bit_status = getSimEncBits();
#endif
      UInt uiSigCoeffGroup   = (uiSigCoeffGroupFlag[ iCGBlkPos ] != 0);
      UInt uiCtxSig  = TComTrQuant::getSigCoeffGroupCtxInc( uiSigCoeffGroupFlag, iCGPosX, iCGPosY, codingParameters.widthInGroups, codingParameters.heightInGroups );
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        std::strncpy( g_sigpool.syntax_name, "coded_sub_block_flag", sizeof(g_sigpool.syntax_name) );
        sigdump_cabac_syntax_info("coded_sub_block_flag", 3,5,5,uiSigCoeffGroup, 0);
      }
      if(uiSigCoeffGroup || iSubSet==1) {
        cabac_prof_add_bins(RESIDUAL, NORMAL, csbfContiZeroCnt+1);
        csbfContiZeroCnt = 0;
      }
      else {
        csbfContiZeroCnt ++;
      }
#endif
      m_pcBinIf->encodeBin( uiSigCoeffGroup, baseCoeffGroupCtx[ uiCtxSig ] );

#ifdef CVI_ENABLE_RDO_BIT_EST
      tuCgfBin++;
      tuCgfFracBit += (getSimEncBits() - cfg_bit_status);
#endif
    }

    // encode significant_coeff_flag
    if( uiSigCoeffGroupFlag[ iCGBlkPos ] )
    {
      const Int patternSigCtx = TComTrQuant::calcPatternSigCtx(uiSigCoeffGroupFlag, iCGPosX, iCGPosY, codingParameters.widthInGroups, codingParameters.heightInGroups);

#ifdef CVI_ENABLE_RDO_BIT_EST
      UInt sig0Cnt = 0, sig0FracBit = 0, sig1FracBit = 0;
#endif
#ifdef SIG_CABAC
      int sig_cnt = 0;
      UInt uiSig_arr = 0;
#endif
      UInt uiBlkPos, uiSig, uiCtxSig;
      for( ; iScanPosSig >= iSubPos; iScanPosSig-- )
      {
        uiBlkPos  = codingParameters.scan[ iScanPosSig ];
        uiSig     = (pcCoef[ uiBlkPos ] != 0);
        if( iScanPosSig > iSubPos || iSubSet == 0 || numNonZero )
        {
#ifdef CVI_ENABLE_RDO_BIT_EST
          UInt64 sig_bit_status = getSimEncBits();
#endif

          uiCtxSig  = TComTrQuant::getSigCtxInc( patternSigCtx, codingParameters, iScanPosSig, uiLog2BlockWidth, uiLog2BlockHeight, chType );
#ifdef SIG_CABAC
          if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
            std::strncpy( g_sigpool.syntax_name, "sig_coeff_flag", sizeof(g_sigpool.syntax_name) );
            uiSig_arr |= (uiSig << sig_cnt);
          }
          sig_cnt ++;
#endif
          m_pcBinIf->encodeBin( uiSig, baseCtx[ uiCtxSig ] );

#ifdef CVI_ENABLE_RDO_BIT_EST
          UInt64 fracBit = getSimEncBits() - sig_bit_status;
          sig0FracBit += (uiSig==0) ? fracBit : 0;
          sig1FracBit += (uiSig==1) ? fracBit : 0;
          sig0Cnt += (uiSig==0) ? 1 : 0;
#endif
        }
        if( uiSig )
        {
          absCoeff[ numNonZero ] = Int(abs( pcCoef[ uiBlkPos ] ));
          coeffSigns = 2 * coeffSigns + ( pcCoef[ uiBlkPos ] < 0 );
          numNonZero++;
          if( lastNZPosInCG == -1 )
          {
            lastNZPosInCG = iScanPosSig;
          }
          firstNZPosInCG = iScanPosSig;
        }
      }
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record && uiSig_arr != 0) {
        sigdump_cabac_syntax_info("sig_coeff_flag_grp", 3,2,2,uiSig_arr, 0);
      }
      cabac_prof_add_bins(RESIDUAL, NORMAL, sig_cnt);
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
      tuSig0FracBit += sig0FracBit;
      tuSig1FracBit += sig1FracBit;
      tuSig0Bin += sig0Cnt;
#endif
    }
    else
    {
      iScanPosSig = iSubPos - 1;
    }

    if( numNonZero > 0 )
    {
      Bool signHidden = ( lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD );

      const UInt uiCtxSet = getContextSetIndex(compID, iSubSet, (c1 == 0));
      c1 = 1;

      ContextModel *baseCtxMod = m_cCUOneSCModel.get( 0, 0 ) + (NUM_ONE_FLAG_CTX_PER_SET * uiCtxSet);

      Int numC1Flag = min(numNonZero, C1FLAG_NUMBER);
      Int firstC2FlagIdx = -1;

#ifdef CVI_ENABLE_RDO_BIT_EST
      UInt64 gt1_bit_status = getSimEncBits();
      for( Int idx = 0; idx < numNonZero; idx++ )
      {
        UInt baseLevel = pcCU->isIntra(uiAbsPartIdx) ? 3 : 2;
        tuGT1Bin += (absCoeff[idx]>1);
        if(absCoeff[ idx ] >= baseLevel){
            estRemainCoeffBit += getEstRemainCoeffBit(absCoeff[idx] - baseLevel);
        }
      }
#endif
#ifdef SIG_CABAC
      cabac_prof_add_bins(RESIDUAL, NORMAL, numC1Flag);
      UInt uiGT1_arr = 0;
#endif
      for( Int idx = 0; idx < numC1Flag; idx++ )
      {
        UInt uiSymbol = absCoeff[ idx ] > 1;
#ifdef SIG_CABAC
        if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
          std::strncpy( g_sigpool.syntax_name, "coeff_abs_level_greater1_flag", sizeof(g_sigpool.syntax_name) );
          uiGT1_arr |= (uiSymbol << idx);
        }
#endif
        m_pcBinIf->encodeBin( uiSymbol, baseCtxMod[c1] );
        if( uiSymbol )
        {
          c1 = 0;
          if (firstC2FlagIdx == -1)
          {
            firstC2FlagIdx = idx;
          }
          else //if a greater-than-one has been encountered already this group
          {
            escapeDataPresentInGroup = true;
          }
        }
        else if( (c1 < 3) && (c1 > 0) )
        {
          c1++;
        }
      }
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        sigdump_cabac_syntax_info("coeff_abs_level_greater1_flag_grp", 3,3,3,uiGT1_arr, 0);
      }
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
      tuSig1FracBit += (getSimEncBits() - gt1_bit_status);
#endif
      if (c1 == 0)
      {
        baseCtxMod = m_cCUAbsSCModel.get( 0, 0 ) + (NUM_ABS_FLAG_CTX_PER_SET * uiCtxSet);
        if ( firstC2FlagIdx != -1)
        {
          UInt symbol = absCoeff[ firstC2FlagIdx ] > 2;
#ifdef SIG_CABAC
          if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
            std::strncpy( g_sigpool.syntax_name, "coeff_abs_level_greater2_flag", sizeof(g_sigpool.syntax_name) );
            sigdump_cabac_syntax_info("coeff_abs_level_greater2_flag", 3,4,4,symbol, 0);
          }
          cabac_prof_add_bins(RESIDUAL, NORMAL, 1);
#endif
          m_pcBinIf->encodeBin( symbol, baseCtxMod[0] );
          if (symbol != 0)
          {
            escapeDataPresentInGroup = true;
          }
#ifdef CVI_ENABLE_RDO_BIT_EST
          tuGT2Bin++;
#endif
        }
      }

      escapeDataPresentInGroup = escapeDataPresentInGroup || (numNonZero > C1FLAG_NUMBER);

      if (escapeDataPresentInGroup && alignCABACBeforeBypass)
      {
        m_pcBinIf->align();
      }
#ifdef SIG_CABAC
      if (g_sigdump.cabac && g_sigpool.cabac_is_record) {
        std::strncpy( g_sigpool.syntax_name, "coeff_sign_flag", sizeof(g_sigpool.syntax_name) );
        UInt coeffSigns_re = 0;
        for (int i = 0; i < numNonZero; i++) {
          coeffSigns_re |= ((coeffSigns >> (numNonZero - 1 - i) & 1) << i);
        }
        sigdump_cabac_syntax_info("coeff_sign_flag", 3,6,0,coeffSigns_re, 0);
      }
      cabac_prof_add_bins(RESIDUAL, BYPASS, numNonZero);
#endif
      if( beValid && signHidden )
      {
        m_pcBinIf->encodeBinsEP( (coeffSigns >> 1), numNonZero-1 );
      }
      else
      {
        m_pcBinIf->encodeBinsEP( coeffSigns, numNonZero );
      }

      Int iFirstCoeff2 = 1;
      if (escapeDataPresentInGroup)
      {
        for ( Int idx = 0; idx < numNonZero; idx++ )
        {
          UInt baseLevel  = (idx < C1FLAG_NUMBER)? (2 + iFirstCoeff2 ) : 1;
          if( absCoeff[ idx ] >= baseLevel)
          {
            const UInt escapeCodeValue = absCoeff[idx] - baseLevel;

            xWriteCoefRemainExGolomb( escapeCodeValue, uiGoRiceParam, extendedPrecision, maxLog2TrDynamicRange );

            if (absCoeff[idx] > (3 << uiGoRiceParam))
            {
              uiGoRiceParam = bUseGolombRiceParameterAdaptation ? (uiGoRiceParam + 1) : (std::min<UInt>((uiGoRiceParam + 1), 4));
            }

            if (updateGolombRiceStatistics)
            {
              const UInt initialGolombRiceParameter = currentGolombRiceStatistic / RExt__GOLOMB_RICE_INCREMENT_DIVISOR;

              if (escapeCodeValue >= (3 << initialGolombRiceParameter))
              {
                currentGolombRiceStatistic++;
              }
              else if (((escapeCodeValue * 2) < (1 << initialGolombRiceParameter)) && (currentGolombRiceStatistic > 0))
              {
                currentGolombRiceStatistic--;
              }

              updateGolombRiceStatistics = false;
            }

          }

          if(absCoeff[ idx ] >= 2)
          {
            iFirstCoeff2 = 0;
          }
        }
      }
    }
  }
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  printSBACCoeffData(posLastX, posLastY, uiWidth, uiHeight, compID, uiAbsPartIdx, codingParameters.scanType, pcCoef, pcCU->getSlice()->getFinalized());
#endif

#ifdef CVI_ENABLE_RDO_BIT_EST
#if (DUMP_BIT_STATS)
  if(g_uiResiModelState==RESI_MODEL_TRAIN){
    Int isIntra = pcCU->isIntra(uiAbsPartIdx) ? 1 : 0;
    if(tuSig0Bin>0){
      sigdump_output_fprint(&g_sigpool.sig0_bit_stats[chType], "%d, %d, %d, %d\n", uiWidth, isIntra, tuSig0FracBit>>EST_BIT_FRAC_BD, tuSig0Bin);
    }
    if(tuSig1Bin>0){
      sigdump_output_fprint(&g_sigpool.sig1_bit_stats[chType], "%d, %d, %d, %d\n", uiWidth, isIntra, tuSig1FracBit>>EST_BIT_FRAC_BD, tuSig1Bin);
    }
    if(tuCgfBin>0){
      sigdump_output_fprint(&g_sigpool.cgf_bit_stats[chType], "%d, %d, %d, %d\n", uiWidth, isIntra, tuCgfFracBit>>(EST_BIT_FRAC_BD-4), tuCgfBin);
    }
  }
#endif
  g_syntaxType = 0;
  if(getRDOBitEstMode()!=CTX_ADAPTIVE)
    return;

  Int isIntra = pcCU->isIntra(uiAbsPartIdx) ? 1 : 0;

  if(g_uiResiModelState==RESI_MODEL_TEST) {
    Int b0Scale = g_significantScale[g_scale_D][chType][isIntra][uiLog2BlockWidth-2][0];
    Int b1Scale = g_significantScale[g_scale_D][chType][isIntra][uiLog2BlockWidth-2][1];
    Int b0Bias = g_significantBias[g_scale_D][chType][isIntra][uiLog2BlockWidth-2][0];
    Int b1Bias = g_significantBias[g_scale_D][chType][isIntra][uiLog2BlockWidth-2][1];
    assert(b0Scale>=0 && b1Scale>=0);
    UInt paraShift = RESI_MDL_FRAC_BD-g_estBitFracBd;
    UInt tuEstFracBit = estLastPosBit
                  + (csbfBin<<g_estBitFracBd)
                  + std::max<Int>((b0Scale>>paraShift) * tuSig0Bin + (b0Bias>>paraShift), 0)
                  + std::max<Int>((b1Scale>>paraShift) * tuSig1Bin + (b1Bias>>paraShift), 0) // significant and greater_than_one
                  + (tuSig1Bin<<g_estBitFracBd) // sign
                  + (estRemainCoeffBit<<g_estBitFracBd) // remain
                  + (tuGT1Bin<<g_estBitFracBd); // greater_than2_flag
                  //+ (tuGT2Bin<<g_estBitFracBd); // greater_than2_flag
    m_pcBinIf->addEstFracBits(tuEstFracBit);
    m_pcBinIf->setBitEstIncr(true);
#ifdef SIG_BIT_EST
    if (g_sigdump.bit_est && g_sigpool.enable_bit_est)
    {
      int pred_mode = pcCU->isIntra(uiAbsPartIdx) ? 0 : g_sigpool.is_merge ? 1 : 2;
      sig_bit_est_st *pbit_est;
      if (g_sigpool.cu_width < 32)
        pbit_est = &g_sigpool.bit_est_golden[pred_mode == 1 ? 0 : 1][pred_mode][compID];
      else
        pbit_est = &g_sigpool.bit_est_golden_32[0][rTu.GetSectionNumber()][compID];
      pbit_est->csbf_map = TEncEntropy::count4x4NonZeroCoeffs(pcCoef, uiWidth, uiHeight);
      pbit_est->scan_idx = codingParameters.scanType;
      pbit_est->is_intra = pcCU->isIntra(uiAbsPartIdx);
      pbit_est->last_x = posLastX;
      pbit_est->last_y = posLastY;
      pbit_est->last_xy_suf_bc = remainLastPosBin << g_estBitFracBd;
      pbit_est->csbf_bc = csbfBin;
      pbit_est->gt2_bc = tuGT1Bin;
      pbit_est->sign_bc = tuSig1Bin<<g_estBitFracBd;
      pbit_est->rem_bc = estRemainCoeffBit<<g_estBitFracBd;
      pbit_est->int_bites = remainLastPosBin + csbfBin + tuGT1Bin + tuSig1Bin + estRemainCoeffBit;
      for (int i = 0; i < 2; i++) {
        pbit_est->sig_bc[i] = i == 0 ? tuSig0Bin : tuSig1Bin;
        pbit_est->m_sig[i] = i == 0 ? (b0Scale>>paraShift) : (b1Scale>>paraShift);
        pbit_est->c_sig[i] = i == 0 ? (b0Bias>>paraShift) : (b1Bias>>paraShift);
        pbit_est->sig_bc_linear[i] = std::max<Int>(pbit_est->m_sig[i] * pbit_est->sig_bc[i] + pbit_est->c_sig[i], 0);
      }
      pbit_est->last_xy_pre_bc = regLastPosBin;
      pbit_est->ratio = lastPosScale;
      pbit_est->total_bites = m_pcBinIf->getEstCbfBits(compID) + tuEstFracBit;
      pbit_est->frac_bites = pbit_est->total_bites - (pbit_est->int_bites << g_estBitFracBd);
    }
#endif
#ifdef SIG_RRU
    if (g_sigdump.rru && (g_sigpool.rru_is_record_si || pcCU->isIntra(uiAbsPartIdx)))
    {
      UInt csbf = TEncEntropy::count4x4NonZeroCoeffs(pcCoef, uiWidth, uiHeight);
      sig_rru_est_gold_st *p_rru_est_gold = &g_sigpool.rru_temp_gold.rru_est_gold[isIntra][compID][rTu.GetSectionNumber()];

      p_rru_est_gold->bitest = (UInt)tuEstFracBit;
      p_rru_est_gold->last_x = posLastX;
      p_rru_est_gold->last_y = posLastY;
      p_rru_est_gold->csbf = csbf;
    }
#endif //~SIG_RRU
  }
  else// RESI_MODEL_TRAIN
  {
    Int org_b0Scale = g_significantScale[0][chType][isIntra][uiLog2BlockWidth-2][0];
    Int org_b1Scale = g_significantScale[0][chType][isIntra][uiLog2BlockWidth-2][1];
    Int org_b0Bias = g_significantBias[0][chType][isIntra][uiLog2BlockWidth-2][0];
    Int org_b1Bias = g_significantBias[0][chType][isIntra][uiLog2BlockWidth-2][1];
    Int bias_clip = (1<<RESI_MDL_FRAC_BD)*g_significantBias_clip;
    Int min_scale = (1<<RESI_MDL_FRAC_BD)*g_significantScale_min;
    Int max_scale = (1<<RESI_MDL_FRAC_BD)*g_significantScale_max;
    Int lr[2], update_Scale, update_bias;
    for(Int l=0; l<2; l++){
        lr[l] = ((1 << LMS_LR_FRAC_BD)*g_significantScale_LR[l]);
    }
    if(tuSig0Bin > 0){
      linear_lms(tuSig0Bin, tuSig0FracBit, org_b0Scale, org_b0Bias, lr, &update_Scale,
                 &update_bias, RESI_MDL_FRAC_BD, g_estBitFracBd, LMS_LR_FRAC_BD);
      g_significantScale[0][chType][isIntra][uiLog2BlockWidth-2][0] = Clip3(min_scale, max_scale, update_Scale);
      g_significantBias[0][chType][isIntra][uiLog2BlockWidth-2][0] = Clip3(-bias_clip, bias_clip, update_bias);
    }
    if(tuSig1Bin > 0){
      linear_lms(tuSig1Bin, tuSig1FracBit, org_b1Scale, org_b1Bias, lr, &update_Scale,
                 &update_bias, RESI_MDL_FRAC_BD, g_estBitFracBd, LMS_LR_FRAC_BD);
      g_significantScale[0][chType][isIntra][uiLog2BlockWidth-2][1] = Clip3(min_scale, max_scale, update_Scale);
      g_significantBias[0][chType][isIntra][uiLog2BlockWidth-2][1] = Clip3(-bias_clip, bias_clip, update_bias);
    }
#ifdef SIG_CABAC
    if (g_sigdump.cabac) {
      sig_ctx *p_ctx = &g_sigpool.cabac_para_update_ctx;
      sig_output_tu_idx(p_ctx, rTu, compID);
      sigdump_output_fprint(p_ctx, "comp=%d, blk_sz=%d, is_intra=%d\n", compID, uiLog2BlockWidth-2, isIntra);
      sig_cabac_para_update(tuSig0Bin, tuSig1Bin, org_b0Scale, org_b1Scale, org_b0Bias, org_b1Bias, tuSig0FracBit, tuSig1FracBit);

      unsigned int CbfCtx[2][2];
      for (int chType = 0; chType <= 1; chType++)
        for (int i = 0; i < 2; i++)
          getCbfStateMps(ComponentID(chType), i, &CbfCtx[chType][i]);

      sigdump_output_fprint(p_ctx, "cbf_luma_ctx = %x, %x\n", CbfCtx[0][0], CbfCtx[0][1]);
      sigdump_output_fprint(p_ctx, "cbf_chroma_ctx = %x, %x\n", CbfCtx[1][0], CbfCtx[1][1]);
    }
#endif
  }
#endif
}

/** code SAO offset sign
 * \param code sign value
 */
Void TEncSbac::codeSAOSign( UInt code )
{
#ifdef CVI_SAO_NEW_SETTING
  rate_nocabac++;
#endif
  m_pcBinIf->encodeBinEP( code );
}

Void TEncSbac::codeSaoMaxUvlc    ( UInt code, UInt maxSymbol )
{
  if (maxSymbol == 0)
  {
    return;
  }

  Int i;
  Bool bCodeLast = ( maxSymbol > code );

  if ( code == 0 )
  {
#ifdef CVI_SAO_NEW_SETTING
    rate_nocabac++;
#endif
    m_pcBinIf->encodeBinEP( 0 );
  }
  else
  {
#ifdef CVI_SAO_NEW_SETTING
    rate_nocabac++;
#endif
    m_pcBinIf->encodeBinEP( 1 );
    for ( i=0; i<code-1; i++ )
    {
#ifdef CVI_SAO_NEW_SETTING
    rate_nocabac++;
#endif
      m_pcBinIf->encodeBinEP( 1 );
    }
    if( bCodeLast )
    {
#ifdef CVI_SAO_NEW_SETTING
    rate_nocabac++;
#endif
      m_pcBinIf->encodeBinEP( 0 );
    }
  }
}

/** Code SAO EO class or BO band position
 */
Void TEncSbac::codeSaoUflc       ( UInt uiLength, UInt uiCode )
{
#ifdef CVI_SAO_NEW_SETTING
    rate_nocabac++;
#endif
  m_pcBinIf->encodeBinsEP ( uiCode, uiLength );
}

/** Code SAO merge flags
 */
Void TEncSbac::codeSaoMerge       ( UInt uiCode )
{
#ifdef CVI_SAO_NEW_SETTING
    rate_nocabac++;
#endif
  m_pcBinIf->encodeBin(((uiCode == 0) ? 0 : 1),  m_cSaoMergeSCModel.get( 0, 0, 0 ));
}

/** Code SAO type index
 */
Void TEncSbac::codeSaoTypeIdx       ( UInt uiCode)
{
  if (uiCode == 0)
  {
#ifdef CVI_SAO_NEW_SETTING
    rate_nocabac++;
#endif
    m_pcBinIf->encodeBin( 0, m_cSaoTypeIdxSCModel.get( 0, 0, 0 ) );
  }
  else
  {
#ifdef CVI_SAO_NEW_SETTING
    rate_nocabac++;
#endif
    m_pcBinIf->encodeBin( 1, m_cSaoTypeIdxSCModel.get( 0, 0, 0 ) );
#ifdef CVI_SAO_NEW_SETTING
    rate_nocabac++;
#endif
    m_pcBinIf->encodeBinEP( uiCode == 1 ? 0 : 1 );
  }
}

Void TEncSbac::codeSAOOffsetParam(ComponentID compIdx, SAOOffset& ctbParam, Bool sliceEnabled, const Int channelBitDepth)
{
  UInt uiSymbol;
  if(!sliceEnabled)
  {
    assert(ctbParam.modeIdc == SAO_MODE_OFF);
    return;
  }
  const Bool bIsFirstCompOfChType = (getFirstComponentOfChannel(toChannelType(compIdx)) == compIdx);

  //type
  if(bIsFirstCompOfChType)
  {
    //sao_type_idx_luma or sao_type_idx_chroma
    if(ctbParam.modeIdc == SAO_MODE_OFF)
    {
      uiSymbol =0;
    }
    else if(ctbParam.typeIdc == SAO_TYPE_BO) //BO
    {
      uiSymbol = 1;
    }
    else
    {
      assert(ctbParam.typeIdc < SAO_TYPE_START_BO); //EO
      uiSymbol = 2;
    }
    codeSaoTypeIdx(uiSymbol);
  }

  if(ctbParam.modeIdc == SAO_MODE_NEW)
  {
    Int numClasses = (ctbParam.typeIdc == SAO_TYPE_BO)?4:NUM_SAO_EO_CLASSES;
    Int offset[4];
    Int k=0;
    for(Int i=0; i< numClasses; i++)
    {
      if(ctbParam.typeIdc != SAO_TYPE_BO && i == SAO_CLASS_EO_PLAIN)
      {
        continue;
      }
      Int classIdx = (ctbParam.typeIdc == SAO_TYPE_BO)?(  (ctbParam.typeAuxInfo+i)% NUM_SAO_BO_CLASSES   ):i;
      offset[k] = ctbParam.offset[classIdx];
      k++;
    }

    const Int  maxOffsetQVal = TComSampleAdaptiveOffset::getMaxOffsetQVal(channelBitDepth);
    for(Int i=0; i< 4; i++)
    {
      codeSaoMaxUvlc((offset[i]<0)?(-offset[i]):(offset[i]),  maxOffsetQVal ); //sao_offset_abs
    }


    if(ctbParam.typeIdc == SAO_TYPE_BO)
    {
      for(Int i=0; i< 4; i++)
      {
        if(offset[i] != 0)
        {
          codeSAOSign((offset[i]< 0)?1:0);
        }
      }

      codeSaoUflc(NUM_SAO_BO_CLASSES_LOG2, ctbParam.typeAuxInfo ); //sao_band_position
    }
    else //EO
    {
      if(bIsFirstCompOfChType)
      {
        assert(ctbParam.typeIdc - SAO_TYPE_START_EO >=0);
        codeSaoUflc(NUM_SAO_EO_TYPES_LOG2, ctbParam.typeIdc - SAO_TYPE_START_EO ); //sao_eo_class_luma or sao_eo_class_chroma
      }
    }

  }
}


Void TEncSbac::codeSAOBlkParam(SAOBlkParam& saoBlkParam, const BitDepths &bitDepths
                              , Bool* sliceEnabled
                              , Bool leftMergeAvail
                              , Bool aboveMergeAvail
                              , Bool onlyEstMergeInfo // = false
                              )
{

  Bool isLeftMerge = false;
  Bool isAboveMerge= false;

  if(leftMergeAvail)
  {
    isLeftMerge = ((saoBlkParam[COMPONENT_Y].modeIdc == SAO_MODE_MERGE) && (saoBlkParam[COMPONENT_Y].typeIdc == SAO_MERGE_LEFT));
    codeSaoMerge( isLeftMerge?1:0  ); //sao_merge_left_flag
  }

  if( aboveMergeAvail && !isLeftMerge)
  {
    isAboveMerge = ((saoBlkParam[COMPONENT_Y].modeIdc == SAO_MODE_MERGE) && (saoBlkParam[COMPONENT_Y].typeIdc == SAO_MERGE_ABOVE));
    codeSaoMerge( isAboveMerge?1:0  ); //sao_merge_left_flag
  }

  if(onlyEstMergeInfo)
  {
    return; //only for RDO
  }

  if(!isLeftMerge && !isAboveMerge) //not merge mode
  {
    for(Int compIdx=0; compIdx < MAX_NUM_COMPONENT; compIdx++)
    {
      codeSAOOffsetParam(ComponentID(compIdx), saoBlkParam[compIdx], sliceEnabled[compIdx], bitDepths.recon[toChannelType(ComponentID(compIdx))]);
    }
  }
}

/*!
 ****************************************************************************
 * \brief
 *   estimate bit cost for CBP, significant map and significant coefficients
 ****************************************************************************
 */
Void TEncSbac::estBit( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType, COEFF_SCAN_TYPE scanType )
{
  estCBFBit( pcEstBitsSbac );

  estSignificantCoeffGroupMapBit( pcEstBitsSbac, chType );

  // encode significance map
  estSignificantMapBit( pcEstBitsSbac, width, height, chType );

  // encode last significant position
  estLastSignificantPositionBit( pcEstBitsSbac, width, height, chType, scanType );

  // encode significant coefficients
  estSignificantCoefficientsBit( pcEstBitsSbac, chType );

  memcpy(pcEstBitsSbac->golombRiceAdaptationStatistics, m_golombRiceAdaptationStatistics, (sizeof(UInt) * RExt__GOLOMB_RICE_ADAPTATION_STATISTICS_SETS));
}

/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost for each CBP bit
 ****************************************************************************
 */
Void TEncSbac::estCBFBit( estBitsSbacStruct* pcEstBitsSbac )
{
  ContextModel *pCtx = m_cCUQtCbfSCModel.get( 0 );

  for( UInt uiCtxInc = 0; uiCtxInc < (NUM_QT_CBF_CTX_SETS * NUM_QT_CBF_CTX_PER_SET); uiCtxInc++ )
  {
    pcEstBitsSbac->blockCbpBits[ uiCtxInc ][ 0 ] = pCtx[ uiCtxInc ].getEntropyBits( 0 );
    pcEstBitsSbac->blockCbpBits[ uiCtxInc ][ 1 ] = pCtx[ uiCtxInc ].getEntropyBits( 1 );
  }

  pCtx = m_cCUQtRootCbfSCModel.get( 0 );

  for( UInt uiCtxInc = 0; uiCtxInc < 4; uiCtxInc++ )
  {
    pcEstBitsSbac->blockRootCbpBits[ uiCtxInc ][ 0 ] = pCtx[ uiCtxInc ].getEntropyBits( 0 );
    pcEstBitsSbac->blockRootCbpBits[ uiCtxInc ][ 1 ] = pCtx[ uiCtxInc ].getEntropyBits( 1 );
  }
}


/*!
 ****************************************************************************
 * \brief
 *    estimate SAMBAC bit cost for significant coefficient group map
 ****************************************************************************
 */
Void TEncSbac::estSignificantCoeffGroupMapBit( estBitsSbacStruct* pcEstBitsSbac, ChannelType chType )
{
  Int firstCtx = 0, numCtx = NUM_SIG_CG_FLAG_CTX;

  for ( Int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++ )
  {
    for( UInt uiBin = 0; uiBin < 2; uiBin++ )
    {
      pcEstBitsSbac->significantCoeffGroupBits[ ctxIdx ][ uiBin ] = m_cCUSigCoeffGroupSCModel.get(  0, chType, ctxIdx ).getEntropyBits( uiBin );
    }
  }
}


/*!
 ****************************************************************************
 * \brief
 *    estimate SAMBAC bit cost for significant coefficient map
 ****************************************************************************
 */
Void TEncSbac::estSignificantMapBit( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType )
{
  //--------------------------------------------------------------------------------------------------

  //set up the number of channels and context variables

  const UInt firstComponent = ((isLuma(chType)) ? (COMPONENT_Y) : (COMPONENT_Cb));
  const UInt lastComponent  = ((isLuma(chType)) ? (COMPONENT_Y) : (COMPONENT_Cb));

  //----------------------------------------------------------

  Int firstCtx = MAX_INT;
  Int numCtx   = MAX_INT;

  if      ((width == 4) && (height == 4))
  {
    firstCtx = significanceMapContextSetStart[chType][CONTEXT_TYPE_4x4];
    numCtx   = significanceMapContextSetSize [chType][CONTEXT_TYPE_4x4];
  }
  else if ((width == 8) && (height == 8))
  {
    firstCtx = significanceMapContextSetStart[chType][CONTEXT_TYPE_8x8];
    numCtx   = significanceMapContextSetSize [chType][CONTEXT_TYPE_8x8];
  }
  else
  {
    firstCtx = significanceMapContextSetStart[chType][CONTEXT_TYPE_NxN];
    numCtx   = significanceMapContextSetSize [chType][CONTEXT_TYPE_NxN];
  }

  //--------------------------------------------------------------------------------------------------

  //fill the data for the significace map

  for (UInt component = firstComponent; component <= lastComponent; component++)
  {
    const UInt contextOffset = getSignificanceMapContextOffset(ComponentID(component));

    if (firstCtx > 0)
    {
      for( UInt bin = 0; bin < 2; bin++ ) //always get the DC
      {
        pcEstBitsSbac->significantBits[ contextOffset ][ bin ] = m_cCUSigSCModel.get( 0, 0, contextOffset ).getEntropyBits( bin );
      }
    }

    // This could be made optional, but would require this function to have knowledge of whether the
    // TU is transform-skipped or transquant-bypassed and whether the SPS flag is set
    for( UInt bin = 0; bin < 2; bin++ )
    {
      const Int ctxIdx = significanceMapContextSetStart[chType][CONTEXT_TYPE_SINGLE];
      pcEstBitsSbac->significantBits[ contextOffset + ctxIdx ][ bin ] = m_cCUSigSCModel.get( 0, 0, (contextOffset + ctxIdx) ).getEntropyBits( bin );
    }

    for ( Int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++ )
    {
      for( UInt uiBin = 0; uiBin < 2; uiBin++ )
      {
        pcEstBitsSbac->significantBits[ contextOffset + ctxIdx ][ uiBin ] = m_cCUSigSCModel.get(  0, 0, (contextOffset + ctxIdx) ).getEntropyBits( uiBin );
      }
    }
  }

  //--------------------------------------------------------------------------------------------------
}


/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost of significant coefficient
 ****************************************************************************
 */

Void TEncSbac::estLastSignificantPositionBit( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType, COEFF_SCAN_TYPE scanType )
{
  if (scanType == SCAN_VER)
  {
    swap(width, height);
  }

  //--------------------------------------------------------------------------------------------------.

  //set up the number of channels

  const UInt firstComponent = ((isLuma(chType)) ? (COMPONENT_Y) : (COMPONENT_Cb));
  const UInt lastComponent  = ((isLuma(chType)) ? (COMPONENT_Y) : (COMPONENT_Cb));

  //--------------------------------------------------------------------------------------------------

  //fill the data for the last-significant-coefficient position

  for (UInt componentIndex = firstComponent; componentIndex <= lastComponent; componentIndex++)
  {
    const ComponentID component = ComponentID(componentIndex);

    Int iBitsX = 0, iBitsY = 0;

    Int blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY;
    getLastSignificantContextParameters(ComponentID(component), width, height, blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY);

    Int ctx;

    const ChannelType channelType = toChannelType(ComponentID(component));

    ContextModel *const pCtxX = m_cCuCtxLastX.get( 0, channelType );
    ContextModel *const pCtxY = m_cCuCtxLastY.get( 0, channelType );
    Int          *const lastXBitsArray = pcEstBitsSbac->lastXBits[channelType];
    Int          *const lastYBitsArray = pcEstBitsSbac->lastYBits[channelType];

    //------------------------------------------------

    //X-coordinate

    for (ctx = 0; ctx < g_uiGroupIdx[ width - 1 ]; ctx++)
    {
      Int ctxOffset = blkSizeOffsetX + (ctx >>shiftX);
      lastXBitsArray[ ctx ] = iBitsX + pCtxX[ ctxOffset ].getEntropyBits( 0 );
      iBitsX += pCtxX[ ctxOffset ].getEntropyBits( 1 );
    }

    lastXBitsArray[ctx] = iBitsX;

    //------------------------------------------------

    //Y-coordinate

    for (ctx = 0; ctx < g_uiGroupIdx[ height - 1 ]; ctx++)
    {
      Int ctxOffset = blkSizeOffsetY + (ctx >>shiftY);
      lastYBitsArray[ ctx ] = iBitsY + pCtxY[ ctxOffset ].getEntropyBits( 0 );
      iBitsY += pCtxY[ ctxOffset ].getEntropyBits( 1 );
    }

    lastYBitsArray[ctx] = iBitsY;

  } //end of component loop

  //--------------------------------------------------------------------------------------------------
}


/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost of significant coefficient
 ****************************************************************************
 */
Void TEncSbac::estSignificantCoefficientsBit( estBitsSbacStruct* pcEstBitsSbac, ChannelType chType )
{
  ContextModel *ctxOne = m_cCUOneSCModel.get(0, 0);
  ContextModel *ctxAbs = m_cCUAbsSCModel.get(0, 0);

  const UInt oneStartIndex = ((isLuma(chType)) ? (0)                     : (NUM_ONE_FLAG_CTX_LUMA));
  const UInt oneStopIndex  = ((isLuma(chType)) ? (NUM_ONE_FLAG_CTX_LUMA) : (NUM_ONE_FLAG_CTX));
  const UInt absStartIndex = ((isLuma(chType)) ? (0)                     : (NUM_ABS_FLAG_CTX_LUMA));
  const UInt absStopIndex  = ((isLuma(chType)) ? (NUM_ABS_FLAG_CTX_LUMA) : (NUM_ABS_FLAG_CTX));

  for (Int ctxIdx = oneStartIndex; ctxIdx < oneStopIndex; ctxIdx++)
  {
    pcEstBitsSbac->m_greaterOneBits[ ctxIdx ][ 0 ] = ctxOne[ ctxIdx ].getEntropyBits( 0 );
    pcEstBitsSbac->m_greaterOneBits[ ctxIdx ][ 1 ] = ctxOne[ ctxIdx ].getEntropyBits( 1 );
  }

  for (Int ctxIdx = absStartIndex; ctxIdx < absStopIndex; ctxIdx++)
  {
    pcEstBitsSbac->m_levelAbsBits[ ctxIdx ][ 0 ] = ctxAbs[ ctxIdx ].getEntropyBits( 0 );
    pcEstBitsSbac->m_levelAbsBits[ ctxIdx ][ 1 ] = ctxAbs[ ctxIdx ].getEntropyBits( 1 );
  }
}

/**
 - Initialize our context information from the nominated source.
 .
 \param pSrc From where to copy context information.
 */
Void TEncSbac::xCopyContextsFrom( const TEncSbac* pSrc )
{
  memcpy(m_contextModels, pSrc->m_contextModels, m_numContextModels*sizeof(m_contextModels[0]));
  memcpy(m_golombRiceAdaptationStatistics, pSrc->m_golombRiceAdaptationStatistics, (sizeof(UInt) * RExt__GOLOMB_RICE_ADAPTATION_STATISTICS_SETS));
}

Void  TEncSbac::loadContexts ( const TEncSbac* pSrc)
{
  xCopyContextsFrom(pSrc);
}

/** Performs CABAC encoding of the explicit RDPCM mode
 * \param rTu current TU data structure
 * \param compID component identifier
 */
Void TEncSbac::codeExplicitRdpcmMode( TComTU &rTu, const ComponentID compID )
{
  TComDataCU *cu = rTu.getCU();
  const TComRectangle &rect = rTu.getRect(compID);
  const UInt absPartIdx   = rTu.GetAbsPartIdxTU(compID);
  const UInt tuHeight = g_aucConvertToBit[rect.height];
  const UInt tuWidth  = g_aucConvertToBit[rect.width];

  assert(tuHeight == tuWidth);
  assert(tuHeight < 4);

  UInt explicitRdpcmMode = cu->getExplicitRdpcmMode(compID, absPartIdx);

  if( explicitRdpcmMode == RDPCM_OFF )
  {
    m_pcBinIf->encodeBin (0, m_explicitRdpcmFlagSCModel.get (0, toChannelType(compID), 0));
  }
  else if( explicitRdpcmMode == RDPCM_HOR || explicitRdpcmMode == RDPCM_VER )
  {
    m_pcBinIf->encodeBin (1, m_explicitRdpcmFlagSCModel.get (0, toChannelType(compID), 0));
    if(explicitRdpcmMode == RDPCM_HOR)
    {
      m_pcBinIf->encodeBin ( 0, m_explicitRdpcmDirSCModel.get(0, toChannelType(compID), 0));
    }
    else
    {
      m_pcBinIf->encodeBin ( 1, m_explicitRdpcmDirSCModel.get(0, toChannelType(compID), 0));
    }
  }
  else
  {
    assert(0);
  }
}

#ifdef CVI_ENABLE_RDO_BIT_EST
Void TEncSbac::cxtScalingSnapshot(TComDataCU* pcCU, UInt uiDepth)
{
  UInt oneInFractBit = g_estBitPrec;
  if(getRDOSyntaxEstMode()==CTX_ADAPTIVE)
  {
    g_scale_D = SCALE_MDL_DELAY; // delay 1 CU32 to sync HW
    for (int b=0; b<2; b++)
    {
      //UInt uiCtx  = pcCU->getCtxSplitFlag(0, uiDepth);
      g_cuSplitFlagScale[0][b] = oneInFractBit; //m_cCUSplitFlagSCModel.get(0, 0, uiCtx).getHwEntropyBits(b);
      //g_cuSplitFlagScale[0][b] = m_cCUSplitFlagSCModel.get(0, 0, uiCtx).getHwEntropyBits(b);
      // UInt uiCtxSkip = pcCU->getCtxSkipFlag(0);
      g_skipFlagScale[0][b] = oneInFractBit;//m_cCUSkipFlagSCModel.get(0, 0, uiCtxSkip).getHwEntropyBits(b);
      g_cuMergeFlagScale[0][b] = oneInFractBit; //m_cCUMergeFlagExtSCModel.get(0, 0, 0).getHwEntropyBits(b);
      g_cuMergeIdxScale[0][b] = oneInFractBit; //m_cCUMergeIdxExtSCModel.get(0, 0, 0).getHwEntropyBits(b);
      g_cuPredModeScale[0][b] = oneInFractBit; //m_cCUPredModeSCModel.get(0, 0, 0).getHwEntropyBits(b);
      g_cuPartFlagScale[0][b] = m_cCUPartSizeSCModel.get(0, 0, 0).getHwEntropyBits(b);
#ifdef SIGDUMP
      g_cuPartFlagIsLps[0][b] = m_cCUPartSizeSCModel.get(0, 0, 0).getHwEntropyIsLps(b);
      g_cuPartFlagState[0][b] = m_cCUPartSizeSCModel.get(0, 0, 0).getHwEntropyState();
#endif
      g_mvpIdxScale[0][b] = oneInFractBit; //m_cMVPIdxSCModel.get(0, 0, 0).getHwEntropyBits(b);
      g_mvdScale[0][b] = oneInFractBit; //m_cCUMvdSCModel.get(0, 0, 0).getHwEntropyBits(b);
      g_refIdxScale[0][b] = oneInFractBit; //m_cCURefPicSCModel.get(0, 0, 0).getHwEntropyBits(b);
      g_intraPredScale[0][b] = oneInFractBit; //m_cCUIntraPredSCModel.get(0, 0, 0).getHwEntropyBits(b);
      g_chmaIntraPredScale[0][b] = oneInFractBit; //m_cCUChromaPredSCModel.get(0, 0, 0).getHwEntropyBits(b);
      for (int s=0; s<NUM_QT_CBF_CTX_SETS; s++)
      {
        for (int p=0; p<NUM_QT_CBF_CTX_PER_SET; p++)
        {
          g_cbfScale[0][s][p][b] = m_cCUQtCbfSCModel.get(0, s, p).getHwEntropyBits(b);
#ifdef SIGDUMP
          g_cbfisLps[0][s][p][b] = m_cCUQtCbfSCModel.get(0, s, p).getHwEntropyIsLps(b);
          g_cbfState[0][s][p] = m_cCUQtCbfSCModel.get(0, s, p).getHwEntropyState();
#endif
        }
      }
      g_rootCbfScale[0][b] = oneInFractBit; // m_cCUQtRootCbfSCModel.get(0, 0, 0).getHwEntropyBits(b);
    }
  }
  else if(getRDOSyntaxEstMode()==BIN_BASE)
  {
    for (int b=0; b<2; b++)
    {
      g_cuSplitFlagScale[0][b] = oneInFractBit;
      g_skipFlagScale[0][b] = oneInFractBit;
      g_cuPredModeScale[0][b] = oneInFractBit;
      g_cuMergeIdxScale[0][b] = oneInFractBit;
      g_cuMergeFlagScale[0][b] = oneInFractBit;
      g_cuPartFlagScale[0][b] = oneInFractBit;
      g_mvpIdxScale[0][b] = oneInFractBit;
      g_mvdScale[0][b] = oneInFractBit;
      g_refIdxScale[0][b] = oneInFractBit;
      g_intraPredScale[0][b] = oneInFractBit;
      g_chmaIntraPredScale[0][b] = oneInFractBit;
      for (int s=0; s<NUM_QT_CBF_CTX_SETS; s++)
      {
        for (int p=0; p<NUM_QT_CBF_CTX_PER_SET; p++)
        {
          g_cbfScale[0][s][p][b] = oneInFractBit;
        }
      }
      g_rootCbfScale[0][b] = oneInFractBit;
    }
  }

  if (pcCU->getCUPelX() == 0 && pcCU->getCUPelY() == 0)
  {
    for (int delay = 0; delay < g_scale_D; delay++)
      cxtScalingSnapshot_delay();
  }
}

// fix k = 0 estimation, hybrid gr and exp-golomb
UInt TEncSbac::getEstRemainCoeffBit(UInt symbol)
{
  Int codeNumber  = (Int)symbol;
  if (codeNumber < (COEF_REMAIN_BIN_REDUCTION)) // GR
  {
    return codeNumber+1;
  }
  else // exp-Golomb
  {
    UInt length = 0;
    codeNumber  = codeNumber - COEF_REMAIN_BIN_REDUCTION;
    while (codeNumber >= (1<<length))
    {
      codeNumber -=  (1<<(length++));
    }
    return COEF_REMAIN_BIN_REDUCTION+(length<<1)+1;
  }
}

Void TEncSbac::cxtScalingSnapshot_delay()
{
  for (int scale_d = g_scale_D; scale_d > 0; scale_d--)
  {
    int prev_scale = scale_d - 1;

    for (int b=0; b<2; b++)
    {
      g_cuSplitFlagScale[scale_d][b] = g_cuSplitFlagScale[prev_scale][b];
      g_skipFlagScale[scale_d][b] = g_skipFlagScale[prev_scale][b];
      g_cuPredModeScale[scale_d][b] = g_cuPredModeScale[prev_scale][b];
      g_cuMergeIdxScale[scale_d][b] = g_cuMergeIdxScale[prev_scale][b];
      g_cuMergeFlagScale[scale_d][b] = g_cuMergeFlagScale[prev_scale][b];
      g_cuPartFlagScale[scale_d][b] = g_cuPartFlagScale[prev_scale][b];
#ifdef SIGDUMP
      g_cuPartFlagIsLps[scale_d][b] = g_cuPartFlagIsLps[0][b];
      g_cuPartFlagState[scale_d][b] = g_cuPartFlagState[0][b];
#endif
      g_mvpIdxScale[scale_d][b] = g_mvpIdxScale[prev_scale][b];
      g_mvdScale[scale_d][b] = g_mvdScale[prev_scale][b];
      g_refIdxScale[scale_d][b] = g_refIdxScale[prev_scale][b];
      g_intraPredScale[scale_d][b] = g_intraPredScale[prev_scale][b];
      g_chmaIntraPredScale[scale_d][b] = g_chmaIntraPredScale[prev_scale][b];

      for (int s=0; s<NUM_QT_CBF_CTX_SETS; s++)
      {
        for (int p=0; p<NUM_QT_CBF_CTX_PER_SET; p++)
        {
          g_cbfScale[scale_d][s][p][b] = g_cbfScale[prev_scale][s][p][b];
#ifdef SIGDUMP
          g_cbfisLps[scale_d][s][p][b] = g_cbfisLps[prev_scale][s][p][b];
          g_cbfState[scale_d][s][p] = g_cbfState[prev_scale][s][p];
#endif
        }
      }
      g_rootCbfScale[scale_d][b] = g_rootCbfScale[prev_scale][b];
    }
  }
}
#endif
//! \}
