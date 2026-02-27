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

/** \file     TComPattern.cpp
    \brief    neighbouring pixel access classes
*/

#include "TComPic.h"
#include "TComPattern.h"
#include "TComDataCU.h"
#include "TComTU.h"
#include "Debug.h"
#include "TComPrediction.h"
#include "cvi_sigdump.h"

//! \ingroup TLibCommon
//! \{

// Forward declarations

/// padding of unavailable reference samples for intra prediction
Void fillReferenceSamples( const Int bitDepth, 
#if O0043_BEST_EFFORT_DECODING
                           const Int bitDepthDelta, 
#endif
                           const Pel* piRoiOrigin, 
                                 Pel* piIntraTemp,
                           const Bool* bNeighborFlags,
                           const Int iNumIntraNeighbor, 
                           const Int unitWidth, 
                           const Int unitHeight, 
                           const Int iAboveUnits, 
                           const Int iLeftUnits,
                           const UInt uiWidth, 
                           const UInt uiHeight, 
                           const Int iPicStride );

/// constrained intra prediction
Bool  isAboveLeftAvailable  ( const TComDataCU* pcCU, UInt uiPartIdxLT );
Int   isAboveAvailable      ( const TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT, Bool* bValidFlags );
Int   isLeftAvailable       ( const TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxLB, Bool* bValidFlags );
Int   isAboveRightAvailable ( const TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT, Bool* bValidFlags );
Int   isBelowLeftAvailable  ( const TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxLB, Bool* bValidFlags );

#ifdef CVI_INTRA_PRED
Int isCviAboveRightAvailable(const TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT, Bool *bValidFlags,
                             const TComRectangle rect);
Void cviFillReferenceSamples(const Int bitDepth,
                             const Pel* piRoiOrigin,
                                   Pel* piIntraTemp,
                             const Bool* bNeighborFlags,
                             const Int iNumIntraNeighbor,
                             const Int unitWidth,
                             const Int unitHeight,
                             const Int iAboveUnits,
                             const Int iLeftUnits,
                             const UInt uiWidth,
                             const UInt uiHeight,
                             const Int iPicStride,
                             const UInt posX,
                             const UInt posY,
                             const UInt ctuSize);
#endif

// ====================================================================================================================
// Public member functions (TComPattern)
// ====================================================================================================================

#if MCTS_ENC_CHECK
Void TComPattern::initPattern(Pel* piY,
                              Int roiWidth,
                              Int roiHeight,
                              Int stride,
                              Int bitDepthLuma,
                              Int roiPosX,
                              Int roiPosY)
#else
Void TComPattern::initPattern (Pel* piY,
                               Int roiWidth,
                               Int roiHeight,
                               Int stride,
                               Int bitDepthLuma)
#endif
{
  m_piROIOrigin = piY;
  m_roiWidth = roiWidth;
  m_roiHeight = roiHeight;
  m_patternStride = stride;
  m_bitDepth = bitDepthLuma;
#if MCTS_ENC_CHECK
  m_roiPosX       = roiPosX;
  m_roiPosY       = roiPosY;
#endif
}

#if MCTS_ENC_CHECK
Void TComPattern::setTileBorders(Int tileLeftTopPelPosX, Int tileLeftTopPelPosY, Int tileRightBottomPelPosX, Int tileRightBottomPelPosY)
{
  m_tileLeftTopPelPosX = tileLeftTopPelPosX;
  m_tileLeftTopPelPosY = tileLeftTopPelPosY;
  m_tileRightBottomPelPosX = tileRightBottomPelPosX;
  m_tileRightBottomPelPosY = tileRightBottomPelPosY;
}
#endif


// TODO: move this function to TComPrediction.cpp.
Void TComPrediction::initIntraPatternChType( TComTU &rTu, const ComponentID compID, const Bool bFilterRefSamples DEBUG_STRING_FN_DECLARE(sDebug)
#ifdef CVI_INTRA_PRED
                                            ,bool isRmdPred
#endif
)
{
  const ChannelType chType    = toChannelType(compID);

  TComDataCU *pcCU=rTu.getCU();
  const TComSPS &sps = *(pcCU->getSlice()->getSPS());
  const UInt uiZorderIdxInPart=rTu.GetAbsPartIdxTU();
  const UInt uiTuWidth        = rTu.getRect(compID).width;
  const UInt uiTuHeight       = rTu.getRect(compID).height;
  const UInt uiTuWidth2       = uiTuWidth  << 1;
  const UInt uiTuHeight2      = uiTuHeight << 1;

  const Int  iBaseUnitSize    = sps.getMaxCUWidth() >> sps.getMaxTotalCUDepth();
  const Int  iUnitWidth       = iBaseUnitSize  >> pcCU->getPic()->getPicYuvRec()->getComponentScaleX(compID);
  const Int  iUnitHeight      = iBaseUnitSize  >> pcCU->getPic()->getPicYuvRec()->getComponentScaleY(compID);
  const Int  iTUWidthInUnits  = uiTuWidth  / iUnitWidth;
  const Int  iTUHeightInUnits = uiTuHeight / iUnitHeight;
  const Int  iAboveUnits      = iTUWidthInUnits  << 1;
  const Int  iLeftUnits       = iTUHeightInUnits << 1;
  const Int  bitDepthForChannel = sps.getBitDepth(chType);

  assert(iTUHeightInUnits > 0 && iTUWidthInUnits > 0);

  const Int  iPartIdxStride   = pcCU->getPic()->getNumPartInCtuWidth();
  const UInt uiPartIdxLT      = pcCU->getZorderIdxInCtu() + uiZorderIdxInPart;
  const UInt uiPartIdxRT      = g_auiRasterToZscan[ g_auiZscanToRaster[ uiPartIdxLT ] +   iTUWidthInUnits  - 1                   ];
  const UInt uiPartIdxLB      = g_auiRasterToZscan[ g_auiZscanToRaster[ uiPartIdxLT ] + ((iTUHeightInUnits - 1) * iPartIdxStride)];

  Int   iPicStride = pcCU->getPic()->getStride(compID);
  Bool  bNeighborFlags[4 * MAX_NUM_PART_IDXS_IN_CTU_WIDTH + 1];
  Int   iNumIntraNeighbor = 0;

  #ifdef CVI_INTRA_PRED
  const TComRectangle &puRect = rTu.getRect(COMPONENT_Y);
  if (isRmdPred)
  {
    bNeighborFlags[iLeftUnits] = isAboveLeftAvailable( pcCU, uiPartIdxLT );
    iNumIntraNeighbor += bNeighborFlags[iLeftUnits] ? 1 : 0;
    iNumIntraNeighbor  += isAboveAvailable        ( pcCU, uiPartIdxLT, uiPartIdxRT, (bNeighborFlags + iLeftUnits + 1)                    );
    iNumIntraNeighbor  += isCviAboveRightAvailable( pcCU, uiPartIdxLT, uiPartIdxRT, (bNeighborFlags + iLeftUnits + 1 + iTUWidthInUnits ), puRect);
    iNumIntraNeighbor  += isLeftAvailable         ( pcCU, uiPartIdxLT, uiPartIdxLB, (bNeighborFlags + iLeftUnits - 1)                    );
    iNumIntraNeighbor  += isBelowLeftAvailable    ( pcCU, uiPartIdxLT, uiPartIdxLB, (bNeighborFlags + iLeftUnits - 1 - iTUHeightInUnits) );
  }
  else
  {
    bNeighborFlags[iLeftUnits] = isAboveLeftAvailable( pcCU, uiPartIdxLT );
    iNumIntraNeighbor += bNeighborFlags[iLeftUnits] ? 1 : 0;
    iNumIntraNeighbor  += isAboveAvailable     ( pcCU, uiPartIdxLT, uiPartIdxRT, (bNeighborFlags + iLeftUnits + 1)                    );
    iNumIntraNeighbor  += isAboveRightAvailable( pcCU, uiPartIdxLT, uiPartIdxRT, (bNeighborFlags + iLeftUnits + 1 + iTUWidthInUnits ) );
    iNumIntraNeighbor  += isLeftAvailable      ( pcCU, uiPartIdxLT, uiPartIdxLB, (bNeighborFlags + iLeftUnits - 1)                    );
    iNumIntraNeighbor  += isBelowLeftAvailable ( pcCU, uiPartIdxLT, uiPartIdxLB, (bNeighborFlags + iLeftUnits - 1 - iTUHeightInUnits) );
  }
#else
  bNeighborFlags[iLeftUnits] = isAboveLeftAvailable( pcCU, uiPartIdxLT );
  iNumIntraNeighbor += bNeighborFlags[iLeftUnits] ? 1 : 0;
  iNumIntraNeighbor  += isAboveAvailable     ( pcCU, uiPartIdxLT, uiPartIdxRT, (bNeighborFlags + iLeftUnits + 1)                    );
  iNumIntraNeighbor  += isAboveRightAvailable( pcCU, uiPartIdxLT, uiPartIdxRT, (bNeighborFlags + iLeftUnits + 1 + iTUWidthInUnits ) );
  iNumIntraNeighbor  += isLeftAvailable      ( pcCU, uiPartIdxLT, uiPartIdxLB, (bNeighborFlags + iLeftUnits - 1)                    );
  iNumIntraNeighbor  += isBelowLeftAvailable ( pcCU, uiPartIdxLT, uiPartIdxLB, (bNeighborFlags + iLeftUnits - 1 - iTUHeightInUnits) );
#endif

  const UInt         uiROIWidth  = uiTuWidth2+1;
  const UInt         uiROIHeight = uiTuHeight2+1;

  assert(uiROIWidth*uiROIHeight <= m_iYuvExtSize);

#if DEBUG_STRING
  std::stringstream ss(stringstream::out);
#endif

  {
    Pel *piIntraTemp   = m_piYuvExt[compID][PRED_BUF_UNFILTERED];
    Pel *piRoiOrigin = pcCU->getPic()->getPicYuvRec()->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu()+uiZorderIdxInPart);
#if O0043_BEST_EFFORT_DECODING
    const Int  bitDepthForChannelInStream = sps.getStreamBitDepth(chType);
    fillReferenceSamples (bitDepthForChannelInStream, bitDepthForChannelInStream - bitDepthForChannel,
#else
#ifdef CVI_INTRA_PRED
    if (isRmdPred)
    {
      Pel *piRoiOriginSrc = pcCU->getPic()->getPicYuvOrg()->getAddr(compID, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu()+uiZorderIdxInPart);
      const UInt ctuSize = pcCU->getSlice()->getSPS()->getMaxCUWidth();
      int posX = pcCU->getCUPelX() + puRect.x0;
      int posY = pcCU->getCUPelY() + puRect.y0;

      cviFillReferenceSamples(bitDepthForChannel,
                              piRoiOriginSrc, piIntraTemp, bNeighborFlags, iNumIntraNeighbor,  iUnitWidth, iUnitHeight, iAboveUnits, iLeftUnits,
                              uiROIWidth, uiROIHeight, iPicStride, posX, posY, ctuSize);
      #ifdef SIG_IAPU
      if (g_sigdump.iapu && (compID==COMPONENT_Y) && isRmdPred){
        int iapu_file_idx = (uiTuWidth==4)? 0: (uiTuWidth==8)? 1: 2;
        sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx], "# NEB_A\n");
        for (UInt i=1; i<uiROIHeight; i++)
        {
          sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx], "%3d ", piIntraTemp[i*uiROIWidth]);
        }
        sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx], "\n");

        sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx], "# NEB_B\n");
        for (UInt i=1; i<uiROIWidth; i++)
        {
          sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx], "%3d ", piIntraTemp[i]); 
        }
        sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx], "\n");

        sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx], "# NEB_C\n");
        sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx], "%3d", piIntraTemp[0]);
        sigdump_output_fprint(&g_sigpool.iapu_rmd_tu_cand_frm_ctx[iapu_file_idx], "\n");
      }
      #endif //~SIG_IAPU

    }
    else{
#endif //~CVI_INTRA_PRED
    fillReferenceSamples (bitDepthForChannel,
#endif
                          piRoiOrigin, piIntraTemp, bNeighborFlags, iNumIntraNeighbor,  iUnitWidth, iUnitHeight, iAboveUnits, iLeftUnits,
                          uiROIWidth, uiROIHeight, iPicStride);
#ifdef CVI_INTRA_PRED
      #ifdef SIG_IAPU
      if (g_sigdump.iapu && !isRmdPred){
        g_sigpool.iapu_st.neb_w[compID] = uiROIWidth;
        g_sigpool.iapu_st.neb_h[compID] = uiROIHeight;
        for (UInt i=1; i<uiROIHeight; i++)
            g_sigpool.iapu_st.neb_a[compID][i]  = piIntraTemp[i*uiROIWidth];
        for (UInt i=1; i<uiROIWidth; i++)
            g_sigpool.iapu_st.neb_b[compID][i]  = piIntraTemp[i];
        g_sigpool.iapu_st.neb_c[compID]     = piIntraTemp[0];
      }
      #endif //~SIG_IAPU
    }
#endif //~CVI_INTRA_PRED


#if DEBUG_STRING
    if (DebugOptionList::DebugString_Pred.getInt()&DebugStringGetPredModeMask(MODE_INTRA))
    {
      ss << "###: generating Ref Samples for channel " << compID << " and " << rTu.getRect(compID).width << " x " << rTu.getRect(compID).height << "\n";
      for (UInt y=0; y<uiROIHeight; y++)
      {
        ss << "###: - ";
        for (UInt x=0; x<uiROIWidth; x++)
        {
          if (x==0 || y==0)
          {
            ss << piIntraTemp[y*uiROIWidth + x] << ", ";
//          if (x%16==15) ss << "\nPart size: ~ ";
          }
        }
        ss << "\n";
      }
    }
#endif
    #ifdef SIG_IAPU
    if (g_sigdump.iapu_misc && (compID==COMPONENT_Y) && isRmdPred){
      int iapu_file_idx = (uiTuWidth==4)? 0: (uiTuWidth==8)? 1: 2;
      sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx], "# Filtered=%d\n", bFilterRefSamples);
    }
    #endif //~SIG_IAPU

    if (bFilterRefSamples)
    {
      // generate filtered intra prediction samples

            Int          stride    = uiROIWidth;
      const Pel         *piSrcPtr  = piIntraTemp                           + (stride * uiTuHeight2); // bottom left
            Pel         *piDestPtr = m_piYuvExt[compID][PRED_BUF_FILTERED] + (stride * uiTuHeight2); // bottom left

      //------------------------------------------------

      Bool useStrongIntraSmoothing = isLuma(chType) && sps.getUseStrongIntraSmoothing();
      const Pel bottomLeft = piIntraTemp[stride * uiTuHeight2];
      const Pel topLeft    = piIntraTemp[0];
      const Pel topRight   = piIntraTemp[uiTuWidth2];

      if (useStrongIntraSmoothing)
      {
#if O0043_BEST_EFFORT_DECODING
        const Int  threshold     = 1 << (bitDepthForChannelInStream - 5);
#else
        const Int  threshold     = 1 << (bitDepthForChannel - 5);
#endif
        const Bool bilinearLeft  = abs((bottomLeft + topLeft ) - (2 * piIntraTemp[stride * uiTuHeight])) < threshold; //difference between the
        const Bool bilinearAbove = abs((topLeft    + topRight) - (2 * piIntraTemp[         uiTuWidth ])) < threshold; //ends and the middle
        if ((uiTuWidth < 32) || (!bilinearLeft) || (!bilinearAbove))
        {
          useStrongIntraSmoothing = false;
        }
      }

      *piDestPtr = *piSrcPtr; // bottom left is not filtered
      piDestPtr -= stride;
      piSrcPtr  -= stride;

      //------------------------------------------------

      //left column (bottom to top)

      if (useStrongIntraSmoothing)
      {
        const Int shift = g_aucConvertToBit[uiTuHeight] + 3; //log2(uiTuHeight2)

        for(UInt i=1; i<uiTuHeight2; i++, piDestPtr-=stride)
        {
          *piDestPtr = (((uiTuHeight2 - i) * bottomLeft) + (i * topLeft) + uiTuHeight) >> shift;
        }

        piSrcPtr -= stride * (uiTuHeight2 - 1);
      }
      else
      {
        for(UInt i=1; i<uiTuHeight2; i++, piDestPtr-=stride, piSrcPtr-=stride)
        {
          *piDestPtr = ( piSrcPtr[stride] + 2*piSrcPtr[0] + piSrcPtr[-stride] + 2 ) >> 2;
        }
      }

      //------------------------------------------------

      //top-left

      if (useStrongIntraSmoothing)
      {
        *piDestPtr = piSrcPtr[0];
      }
      else
      {
        *piDestPtr = ( piSrcPtr[stride] + 2*piSrcPtr[0] + piSrcPtr[1] + 2 ) >> 2;
      }
      piDestPtr += 1;
      piSrcPtr  += 1;

      //------------------------------------------------

      //top row (left-to-right)

      if (useStrongIntraSmoothing)
      {
        const Int shift = g_aucConvertToBit[uiTuWidth] + 3; //log2(uiTuWidth2)

        for(UInt i=1; i<uiTuWidth2; i++, piDestPtr++)
        {
          *piDestPtr = (((uiTuWidth2 - i) * topLeft) + (i * topRight) + uiTuWidth) >> shift;
        }

        piSrcPtr += uiTuWidth2 - 1;
      }
      else
      {
        for(UInt i=1; i<uiTuWidth2; i++, piDestPtr++, piSrcPtr++)
        {
          *piDestPtr = ( piSrcPtr[1] + 2*piSrcPtr[0] + piSrcPtr[-1] + 2 ) >> 2;
        }
      }

      //------------------------------------------------

      *piDestPtr=*piSrcPtr; // far right is not filtered

      #ifdef SIG_IAPU
      if (g_sigdump.iapu_misc && (compID==COMPONENT_Y) && isRmdPred){
        int iapu_file_idx = (uiTuWidth==4)? 0: (uiTuWidth==8)? 1: 2;
        sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx], "# NEB_A FLT\n");
        for (UInt i=1; i<uiROIHeight; i++)
        {
          sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx], "%3d ", m_piYuvExt[compID][PRED_BUF_FILTERED][i*uiROIWidth + 0]);
        }
        sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx], "\n");

        sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx], "# NEB_B FLT\n");
        for (UInt i=1; i<uiROIWidth; i++)
        {
          sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx], "%3d ", m_piYuvExt[compID][PRED_BUF_FILTERED][0*uiROIWidth + i]); 
        }
        sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx], "\n");

        sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx], "# NEB_C FLT\n");
        sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx], "%3d", m_piYuvExt[compID][PRED_BUF_FILTERED][0*uiROIWidth + 0]);
        sigdump_output_fprint(&g_sigpool.iapu_misc_frm_ctx[iapu_file_idx], "\n");
      }
      #endif //~SIG_IAPU


#if DEBUG_STRING
    if (DebugOptionList::DebugString_Pred.getInt()&DebugStringGetPredModeMask(MODE_INTRA))
    {
      ss << "###: filtered result for channel " << compID <<"\n";
      for (UInt y=0; y<uiROIHeight; y++)
      {
        ss << "###: - ";
        for (UInt x=0; x<uiROIWidth; x++)
        {
          if (x==0 || y==0)
          {
            ss << m_piYuvExt[compID][PRED_BUF_FILTERED][y*uiROIWidth + x] << ", ";
//          if (x%16==15) ss << "\nPart size: ~ ";
          }
        }
        ss << "\n";
      }
    }
#endif


    }
  }
  DEBUG_STRING_APPEND(sDebug, ss.str())
}

Void fillReferenceSamples( const Int bitDepth, 
#if O0043_BEST_EFFORT_DECODING
                           const Int bitDepthDelta, 
#endif
                           const Pel* piRoiOrigin, 
                                 Pel* piIntraTemp,
                           const Bool* bNeighborFlags,
                           const Int iNumIntraNeighbor, 
                           const Int unitWidth, 
                           const Int unitHeight, 
                           const Int iAboveUnits, 
                           const Int iLeftUnits,
                           const UInt uiWidth, 
                           const UInt uiHeight, 
                           const Int iPicStride )
{
  const Pel* piRoiTemp;
  Int  i, j;
  Int  iDCValue = 1 << (bitDepth - 1);
  const Int iTotalUnits = iAboveUnits + iLeftUnits + 1; //+1 for top-left

  if (iNumIntraNeighbor == 0)
  {
    // Fill border with DC value
    for (i=0; i<uiWidth; i++)
    {
      piIntraTemp[i] = iDCValue;
    }
    for (i=1; i<uiHeight; i++)
    {
      piIntraTemp[i*uiWidth] = iDCValue;
    }
  }
  else if (iNumIntraNeighbor == iTotalUnits)
  {
    // Fill top-left border and top and top right with rec. samples
    piRoiTemp = piRoiOrigin - iPicStride - 1;

    for (i=0; i<uiWidth; i++)
    {
#if O0043_BEST_EFFORT_DECODING
      piIntraTemp[i] = piRoiTemp[i] << bitDepthDelta;
#else
      piIntraTemp[i] = piRoiTemp[i];
#endif
    }

    // Fill left and below left border with rec. samples
    piRoiTemp = piRoiOrigin - 1;

    for (i=1; i<uiHeight; i++)
    {
#if O0043_BEST_EFFORT_DECODING
      piIntraTemp[i*uiWidth] = (*(piRoiTemp)) << bitDepthDelta;
#else
      piIntraTemp[i*uiWidth] = *(piRoiTemp);
#endif
      piRoiTemp += iPicStride;
    }
  }
  else // reference samples are partially available
  {
    // all above units have "unitWidth" samples each, all left/below-left units have "unitHeight" samples each
    const Int  iTotalSamples = (iLeftUnits * unitHeight) + ((iAboveUnits + 1) * unitWidth);
    Pel  piIntraLine[5 * MAX_CU_SIZE];
    Pel  *piIntraLineTemp;
    const Bool *pbNeighborFlags;


    // Initialize
    for (i=0; i<iTotalSamples; i++)
    {
      piIntraLine[i] = iDCValue;
    }

    // Fill top-left sample
    piRoiTemp = piRoiOrigin - iPicStride - 1;
    piIntraLineTemp = piIntraLine + (iLeftUnits * unitHeight);
    pbNeighborFlags = bNeighborFlags + iLeftUnits;
    if (*pbNeighborFlags)
    {
#if O0043_BEST_EFFORT_DECODING
      Pel topLeftVal=piRoiTemp[0] << bitDepthDelta;
#else
      Pel topLeftVal=piRoiTemp[0];
#endif
      for (i=0; i<unitWidth; i++)
      {
        piIntraLineTemp[i] = topLeftVal;
      }
    }

    // Fill left & below-left samples (downwards)
    piRoiTemp += iPicStride;
    piIntraLineTemp--;
    pbNeighborFlags--;

    for (j=0; j<iLeftUnits; j++)
    {
      if (*pbNeighborFlags)
      {
        for (i=0; i<unitHeight; i++)
        {
#if O0043_BEST_EFFORT_DECODING
          piIntraLineTemp[-i] = piRoiTemp[i*iPicStride] << bitDepthDelta;
#else
          piIntraLineTemp[-i] = piRoiTemp[i*iPicStride];
#endif
        }
      }
      piRoiTemp += unitHeight*iPicStride;
      piIntraLineTemp -= unitHeight;
      pbNeighborFlags--;
    }

    // Fill above & above-right samples (left-to-right) (each unit has "unitWidth" samples)
    piRoiTemp = piRoiOrigin - iPicStride;
    // offset line buffer by iNumUints2*unitHeight (for left/below-left) + unitWidth (for above-left)
    piIntraLineTemp = piIntraLine + (iLeftUnits * unitHeight) + unitWidth;
    pbNeighborFlags = bNeighborFlags + iLeftUnits + 1;
    for (j=0; j<iAboveUnits; j++)
    {
      if (*pbNeighborFlags)
      {
        for (i=0; i<unitWidth; i++)
        {
#if O0043_BEST_EFFORT_DECODING
          piIntraLineTemp[i] = piRoiTemp[i] << bitDepthDelta;
#else
          piIntraLineTemp[i] = piRoiTemp[i];
#endif
        }
      }
      piRoiTemp += unitWidth;
      piIntraLineTemp += unitWidth;
      pbNeighborFlags++;
    }

    // Pad reference samples when necessary
    Int iCurrJnit = 0;
    Pel  *piIntraLineCur   = piIntraLine;
    const UInt piIntraLineTopRowOffset = iLeftUnits * (unitHeight - unitWidth);

    if (!bNeighborFlags[0])
    {
      // very bottom unit of bottom-left; at least one unit will be valid.
      {
        Int   iNext = 1;
        while (iNext < iTotalUnits && !bNeighborFlags[iNext])
        {
          iNext++;
        }
        Pel *piIntraLineNext = piIntraLine + ((iNext < iLeftUnits) ? (iNext * unitHeight) : (piIntraLineTopRowOffset + (iNext * unitWidth)));
        const Pel refSample = *piIntraLineNext;
        // Pad unavailable samples with new value
        Int iNextOrTop = std::min<Int>(iNext, iLeftUnits);
        // fill left column
        while (iCurrJnit < iNextOrTop)
        {
          for (i=0; i<unitHeight; i++)
          {
            piIntraLineCur[i] = refSample;
          }
          piIntraLineCur += unitHeight;
          iCurrJnit++;
        }
        // fill top row
        while (iCurrJnit < iNext)
        {
          for (i=0; i<unitWidth; i++)
          {
            piIntraLineCur[i] = refSample;
          }
          piIntraLineCur += unitWidth;
          iCurrJnit++;
        }
      }
    }

    // pad all other reference samples.
    while (iCurrJnit < iTotalUnits)
    {
      if (!bNeighborFlags[iCurrJnit]) // samples not available
      {
        {
          const Int numSamplesInCurrUnit = (iCurrJnit >= iLeftUnits) ? unitWidth : unitHeight;
          const Pel refSample = *(piIntraLineCur-1);
          for (i=0; i<numSamplesInCurrUnit; i++)
          {
            piIntraLineCur[i] = refSample;
          }
          piIntraLineCur += numSamplesInCurrUnit;
          iCurrJnit++;
        }
      }
      else
      {
        piIntraLineCur += (iCurrJnit >= iLeftUnits) ? unitWidth : unitHeight;
        iCurrJnit++;
      }
    }

    // Copy processed samples

    piIntraLineTemp = piIntraLine + uiHeight + unitWidth - 2;
    // top left, top and top right samples
    for (i=0; i<uiWidth; i++)
    {
      piIntraTemp[i] = piIntraLineTemp[i];
    }

    piIntraLineTemp = piIntraLine + uiHeight - 1;
    for (i=1; i<uiHeight; i++)
    {
      piIntraTemp[i*uiWidth] = piIntraLineTemp[-i];
    }
  }
}

Bool TComPrediction::filteringIntraReferenceSamples(const ComponentID compID, UInt uiDirMode, UInt uiTuChWidth, UInt uiTuChHeight, const ChromaFormat chFmt, const Bool intraReferenceSmoothingDisabled)
{
  Bool bFilter;

  if (!filterIntraReferenceSamples(toChannelType(compID), chFmt, intraReferenceSmoothingDisabled))
  {
    bFilter=false;
  }
  else
  {
    assert(uiTuChWidth>=4 && uiTuChHeight>=4 && uiTuChWidth<128 && uiTuChHeight<128);

    if (uiDirMode == DC_IDX)
    {
      bFilter=false; //no smoothing for DC or LM chroma
    }
    else
    {
      Int diff = min<Int>(abs((Int) uiDirMode - HOR_IDX), abs((Int)uiDirMode - VER_IDX));
      UInt sizeIndex=g_aucConvertToBit[uiTuChWidth];
      assert(sizeIndex < MAX_INTRA_FILTER_DEPTHS);
      bFilter = diff > m_aucIntraFilter[toChannelType(compID)][sizeIndex];
    }
  }
  return bFilter;
}

Bool isAboveLeftAvailable( const TComDataCU* pcCU, UInt uiPartIdxLT )
{
  Bool bAboveLeftFlag;
  UInt uiPartAboveLeft;
  const TComDataCU* pcCUAboveLeft = pcCU->getPUAboveLeft( uiPartAboveLeft, uiPartIdxLT );
  if(pcCU->getSlice()->getPPS()->getConstrainedIntraPred())
  {
    bAboveLeftFlag = ( pcCUAboveLeft && pcCUAboveLeft->isIntra( uiPartAboveLeft ) );
  }
  else
  {
    bAboveLeftFlag = (pcCUAboveLeft ? true : false);
  }
  #ifdef SIG_IAPU
  if(g_sigdump.iapu)
    g_sigpool.iapu_st.neb_avail_b[2] = bAboveLeftFlag;
  #endif //~SIG_IAPU  

  return bAboveLeftFlag;
}

Int isAboveAvailable( const TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT, Bool *bValidFlags )
{
  const UInt uiRasterPartBegin = g_auiZscanToRaster[uiPartIdxLT];
  const UInt uiRasterPartEnd = g_auiZscanToRaster[uiPartIdxRT]+1;
  const UInt uiIdxStep = 1;
  Bool *pbValidFlags = bValidFlags;
  Int iNumIntra = 0;

  for ( UInt uiRasterPart = uiRasterPartBegin; uiRasterPart < uiRasterPartEnd; uiRasterPart += uiIdxStep )
  {
    UInt uiPartAbove;
    const TComDataCU* pcCUAbove = pcCU->getPUAbove( uiPartAbove, g_auiRasterToZscan[uiRasterPart] );
    if(pcCU->getSlice()->getPPS()->getConstrainedIntraPred())
    {
      if ( pcCUAbove && pcCUAbove->isIntra( uiPartAbove ) )
      {
        iNumIntra++;
        *pbValidFlags = true;
      }
      else
      {
        *pbValidFlags = false;
      }
    }
    else
    {
      if (pcCUAbove)
      {
        iNumIntra++;
        *pbValidFlags = true;
      }
      else
      {
        *pbValidFlags = false;
      }
    }
    pbValidFlags++;
    #ifdef SIG_IAPU
    if(g_sigdump.iapu){
      if(uiRasterPart==uiRasterPartBegin){
        if(pcCU->getSlice()->getPPS()->getConstrainedIntraPred())
          if ( pcCUAbove && pcCUAbove->isIntra( uiPartAbove ) )
            g_sigpool.iapu_st.neb_avail_b[1] = true;
          else
            g_sigpool.iapu_st.neb_avail_b[1] = false;
        else
          if (pcCUAbove)
            g_sigpool.iapu_st.neb_avail_b[1] = true;
          else
            g_sigpool.iapu_st.neb_avail_b[1] = false;
      }
    }
    #endif //~SIG_IAPU  
  }
  return iNumIntra;
}

Int isLeftAvailable( const TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxLB, Bool *bValidFlags )
{
  const UInt uiRasterPartBegin = g_auiZscanToRaster[uiPartIdxLT];
  const UInt uiRasterPartEnd = g_auiZscanToRaster[uiPartIdxLB]+1;
  const UInt uiIdxStep = pcCU->getPic()->getNumPartInCtuWidth();
  Bool *pbValidFlags = bValidFlags;
  Int iNumIntra = 0;

  for ( UInt uiRasterPart = uiRasterPartBegin; uiRasterPart < uiRasterPartEnd; uiRasterPart += uiIdxStep )
  {
    UInt uiPartLeft;
    const TComDataCU* pcCULeft = pcCU->getPULeft( uiPartLeft, g_auiRasterToZscan[uiRasterPart] );
    if(pcCU->getSlice()->getPPS()->getConstrainedIntraPred())
    {
      if ( pcCULeft && pcCULeft->isIntra( uiPartLeft ) )
      {
        iNumIntra++;
        *pbValidFlags = true;
      }
      else
      {
        *pbValidFlags = false;
      }
    }
    else
    {
      if ( pcCULeft )
      {
        iNumIntra++;
        *pbValidFlags = true;
      }
      else
      {
        *pbValidFlags = false;
      }
    }
    pbValidFlags--; // opposite direction
    #ifdef SIG_IAPU
    if(g_sigdump.iapu){
      if(uiRasterPart==uiRasterPartBegin){
        if(pcCU->getSlice()->getPPS()->getConstrainedIntraPred())
          if ( pcCULeft && pcCULeft->isIntra( uiPartLeft ) )
            g_sigpool.iapu_st.neb_avail_a[1] = true;
          else
            g_sigpool.iapu_st.neb_avail_a[1] = false;
        else
          if ( pcCULeft )
            g_sigpool.iapu_st.neb_avail_a[1] = true;
          else
            g_sigpool.iapu_st.neb_avail_a[1] = false;
      }
    }
    #endif //~SIG_IAPU  
  }
  return iNumIntra;
}

Int isAboveRightAvailable( const TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT, Bool *bValidFlags )
{
  const UInt uiNumUnitsInPU = g_auiZscanToRaster[uiPartIdxRT] - g_auiZscanToRaster[uiPartIdxLT] + 1;
  Bool *pbValidFlags = bValidFlags;
  Int iNumIntra = 0;

  for ( UInt uiOffset = 1; uiOffset <= uiNumUnitsInPU; uiOffset++ )
  {
    UInt uiPartAboveRight;
    const TComDataCU* pcCUAboveRight = pcCU->getPUAboveRight( uiPartAboveRight, uiPartIdxRT, uiOffset );
    if(pcCU->getSlice()->getPPS()->getConstrainedIntraPred())
    {
      if ( pcCUAboveRight && pcCUAboveRight->isIntra( uiPartAboveRight ) )
      {
        iNumIntra++;
        *pbValidFlags = true;
      }
      else
      {
        *pbValidFlags = false;
      }
    }
    else
    {
      if ( pcCUAboveRight )
      {
        iNumIntra++;
        *pbValidFlags = true;
      }
      else
      {
        *pbValidFlags = false;
      }
    }
    pbValidFlags++;
    #ifdef SIG_IAPU
    if(g_sigdump.iapu){
      if(uiOffset == 1){
        if(pcCU->getSlice()->getPPS()->getConstrainedIntraPred())
          if ( pcCUAboveRight && pcCUAboveRight->isIntra( uiPartAboveRight ) )
            g_sigpool.iapu_st.neb_avail_b[0] = true;
          else
            g_sigpool.iapu_st.neb_avail_b[0] = false;
        else
          if ( pcCUAboveRight )
            g_sigpool.iapu_st.neb_avail_b[0] = true;
          else
            g_sigpool.iapu_st.neb_avail_b[0] = false;
      }
    }
    #endif //~SIG_IAPU    
  }
  return iNumIntra;
}

Int isBelowLeftAvailable( const TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxLB, Bool *bValidFlags )
{
  const UInt uiNumUnitsInPU = (g_auiZscanToRaster[uiPartIdxLB] - g_auiZscanToRaster[uiPartIdxLT]) / pcCU->getPic()->getNumPartInCtuWidth() + 1;
  Bool *pbValidFlags = bValidFlags;
  Int iNumIntra = 0;

  for ( UInt uiOffset = 1; uiOffset <= uiNumUnitsInPU; uiOffset++ )
  {
    UInt uiPartBelowLeft;
    const TComDataCU* pcCUBelowLeft = pcCU->getPUBelowLeft( uiPartBelowLeft, uiPartIdxLB, uiOffset );
    if(pcCU->getSlice()->getPPS()->getConstrainedIntraPred())
    {
      if ( pcCUBelowLeft && pcCUBelowLeft->isIntra( uiPartBelowLeft ) )
      {
        iNumIntra++;
        *pbValidFlags = true;
      }
      else
      {
        *pbValidFlags = false;
      }
    }
    else
    {
      if ( pcCUBelowLeft )
      {
        iNumIntra++;
        *pbValidFlags = true;
      }
      else
      {
        *pbValidFlags = false;
      }
    }
    pbValidFlags--; // opposite direction
    #ifdef SIG_IAPU
    if(g_sigdump.iapu){
      if(uiOffset == 1){
        if(pcCU->getSlice()->getPPS()->getConstrainedIntraPred())
          if ( pcCUBelowLeft && pcCUBelowLeft->isIntra( uiPartBelowLeft ) )
            g_sigpool.iapu_st.neb_avail_a[0] = true;
          else
            g_sigpool.iapu_st.neb_avail_a[0] = false;
        else
          if ( pcCUBelowLeft )
            g_sigpool.iapu_st.neb_avail_a[0] = true;
          else
            g_sigpool.iapu_st.neb_avail_a[0] = false;
      }
    }
    #endif //~SIG_IAPU    
  }
  return iNumIntra;
}

#ifdef CVI_INTRA_PRED
Int isCviAboveRightAvailable(const TComDataCU* pcCU, UInt uiPartIdxLT, UInt uiPartIdxRT, Bool *bValidFlags,
                             const TComRectangle rect)
{
  const UInt uiNumUnitsInPU = g_auiZscanToRaster[uiPartIdxRT] - g_auiZscanToRaster[uiPartIdxLT] + 1;
  Bool *pbValidFlags = bValidFlags;
  Int iNumIntra = 0;
  Int ctuSize = pcCU->getSlice()->getSPS()->getMaxCUWidth();
  Int mask = ctuSize - 1;
  Int pos_x = pcCU->getCUPelX() + rect.x0;
  Int pos_y = pcCU->getCUPelY() + rect.y0;

  for ( UInt uiOffset = 1; uiOffset <= uiNumUnitsInPU; uiOffset++ )
  {
    UInt uiPartAboveRight;
    const TComDataCU* pcCUAboveRight = pcCU->getPUAboveRight( uiPartAboveRight, uiPartIdxRT, uiOffset );

    if ( pcCUAboveRight )
    {
      int above_right_x = pos_x + rect.width;
      int frm_width = pcCU->getSlice()->getSPS()->getPicWidthInLumaSamples();

      // CTU top right PU
      if (((pos_y & mask) == 0) && (((pos_x & mask) + rect.width) == ctuSize))
      {
        *pbValidFlags = false;
      }
      else if ((above_right_x + rect.width) > frm_width)
      {
        *pbValidFlags = false;
      }
      else
      {
        iNumIntra++;
        *pbValidFlags = true;
      }
    }
    else
    {
      *pbValidFlags = false;
    }
    pbValidFlags++;
  }

  return iNumIntra;
}

Void cviFillReferenceSamples(const Int bitDepth,
                             const Pel* piRoiOrigin,
                                   Pel* piIntraTemp,
                             const Bool* bNeighborFlags,
                             const Int iNumIntraNeighbor,
                             const Int unitWidth,
                             const Int unitHeight,
                             const Int iAboveUnits,
                             const Int iLeftUnits,
                             const UInt uiWidth,
                             const UInt uiHeight,
                             const Int iPicStride,
                             const UInt posX,
                             const UInt posY,
                             const UInt ctuSize)
{
  const Pel* piRoiTemp;
  Int  i, j;
  Int  iDCValue = 1 << (bitDepth - 1);
  const Int iTotalUnits = iAboveUnits + iLeftUnits + 1; //+1 for top-left

  const UInt mask = ctuSize - 1;
  const bool isCtuTop  = (posY & mask) == 0 ? true : false;
  const bool isCtuLeft = (posX & mask) == 0 ? true : false;

  if (iNumIntraNeighbor == 0)
  {
    // Fill border with DC value
    for (i=0; i<uiWidth; i++)
    {
      piIntraTemp[i] = iDCValue;
    }
    for (i=1; i<uiHeight; i++)
    {
      piIntraTemp[i*uiWidth] = iDCValue;
    }
  }
  else if (iNumIntraNeighbor == iTotalUnits)
  {
    // Fill top-left border and top and top right with rec. samples
    if (isCtuTop)
      piRoiTemp = piRoiOrigin - 1;
    else
      piRoiTemp = piRoiOrigin - iPicStride - 1;

    for (i=0; i<uiWidth; i++)
    {
      piIntraTemp[i] = piRoiTemp[i];
    }

    if (isCtuLeft || isCtuTop)
      piIntraTemp[0] = piRoiOrigin[0]; // top-left use pu(0, 0) pixel

    // Fill left and below left border with rec. samples
    if (isCtuLeft)
    {
      piRoiTemp = piRoiOrigin;
    }
    else
    {
      piRoiTemp = piRoiOrigin - 1;
    }

    for (i=1; i<uiHeight; i++)
    {
      piIntraTemp[i*uiWidth] = *(piRoiTemp);
      piRoiTemp += iPicStride;
    }
  }
  else // reference samples are partially available
  {
    // all above units have "unitWidth" samples each, all left/below-left units have "unitHeight" samples each
    const Int  iTotalSamples = (iLeftUnits * unitHeight) + ((iAboveUnits + 1) * unitWidth);
    Pel  piIntraLine[5 * MAX_CU_SIZE];
    Pel  *piIntraLineTemp;
    const Bool *pbNeighborFlags;

    // Initialize
    for (i=0; i<iTotalSamples; i++)
    {
      piIntraLine[i] = iDCValue;
    }

    // Fill top-left sample
    piRoiTemp = piRoiOrigin - iPicStride - 1;
    piIntraLineTemp = piIntraLine + (iLeftUnits * unitHeight);
    pbNeighborFlags = bNeighborFlags + iLeftUnits;
    if (*pbNeighborFlags)
    {
      Pel topLeftVal=piRoiTemp[0];
      if (isCtuTop || isCtuLeft)
      {
          topLeftVal = piRoiOrigin[0];
      }

      for (i=0; i<unitWidth; i++)
      {
        piIntraLineTemp[i] = topLeftVal;
      }
    }

    // Fill left & below-left samples (downwards)
    if (isCtuLeft)
      piRoiTemp = piRoiOrigin;
    else
      piRoiTemp += iPicStride;
    piIntraLineTemp--;
    pbNeighborFlags--;

    for (j=0; j<iLeftUnits; j++)
    {
      if (*pbNeighborFlags)
      {
        for (i=0; i<unitHeight; i++)
        {
          piIntraLineTemp[-i] = piRoiTemp[i*iPicStride];
        }
      }
      piRoiTemp += unitHeight*iPicStride;
      piIntraLineTemp -= unitHeight;
      pbNeighborFlags--;
    }

    // Fill above & above-right samples (left-to-right) (each unit has "unitWidth" samples)
    if (isCtuTop)
      piRoiTemp = piRoiOrigin;
    else
      piRoiTemp = piRoiOrigin - iPicStride;
    // offset line buffer by iNumUints2*unitHeight (for left/below-left) + unitWidth (for above-left)
    piIntraLineTemp = piIntraLine + (iLeftUnits * unitHeight) + unitWidth;
    pbNeighborFlags = bNeighborFlags + iLeftUnits + 1;
    for (j=0; j<iAboveUnits; j++)
    {
      if (*pbNeighborFlags)
      {
        for (i=0; i<unitWidth; i++)
        {
          piIntraLineTemp[i] = piRoiTemp[i];
        }
      }
      piRoiTemp += unitWidth;
      piIntraLineTemp += unitWidth;
      pbNeighborFlags++;
    }

    // Pad reference samples when necessary
    Int iCurrJnit = 0;
    Pel  *piIntraLineCur   = piIntraLine;
    const UInt piIntraLineTopRowOffset = iLeftUnits * (unitHeight - unitWidth);

    if (!bNeighborFlags[0])
    {
      // very bottom unit of bottom-left; at least one unit will be valid.
      {
        Int   iNext = 1;
        while (iNext < iTotalUnits && !bNeighborFlags[iNext])
        {
          iNext++;
        }
        Pel *piIntraLineNext = piIntraLine + ((iNext < iLeftUnits) ? (iNext * unitHeight) : (piIntraLineTopRowOffset + (iNext * unitWidth)));
        const Pel refSample = *piIntraLineNext;
        // Pad unavailable samples with new value
        Int iNextOrTop = std::min<Int>(iNext, iLeftUnits);
        // fill left column
        while (iCurrJnit < iNextOrTop)
        {
          for (i=0; i<unitHeight; i++)
          {
            piIntraLineCur[i] = refSample;
          }
          piIntraLineCur += unitHeight;
          iCurrJnit++;
        }
        // fill top row
        while (iCurrJnit < iNext)
        {
          for (i=0; i<unitWidth; i++)
          {
            piIntraLineCur[i] = refSample;
          }
          piIntraLineCur += unitWidth;
          iCurrJnit++;
        }
      }
    }

    // pad all other reference samples.
    while (iCurrJnit < iTotalUnits)
    {
      if (!bNeighborFlags[iCurrJnit]) // samples not available
      {
        {
          const Int numSamplesInCurrUnit = (iCurrJnit >= iLeftUnits) ? unitWidth : unitHeight;
          const Pel refSample = *(piIntraLineCur-1);
          for (i=0; i<numSamplesInCurrUnit; i++)
          {
            piIntraLineCur[i] = refSample;
          }
          piIntraLineCur += numSamplesInCurrUnit;
          iCurrJnit++;
        }
      }
      else
      {
        piIntraLineCur += (iCurrJnit >= iLeftUnits) ? unitWidth : unitHeight;
        iCurrJnit++;
      }
    }

    // Copy processed samples

    piIntraLineTemp = piIntraLine + uiHeight + unitWidth - 2;
    // top left, top and top right samples
    for (i=0; i<uiWidth; i++)
    {
      piIntraTemp[i] = piIntraLineTemp[i];
    }

    piIntraLineTemp = piIntraLine + uiHeight - 1;
    for (i=1; i<uiHeight; i++)
    {
      piIntraTemp[i*uiWidth] = piIntraLineTemp[-i];
    }
  }
}
#endif //~CVI_INTRA_PRED

//! \}
