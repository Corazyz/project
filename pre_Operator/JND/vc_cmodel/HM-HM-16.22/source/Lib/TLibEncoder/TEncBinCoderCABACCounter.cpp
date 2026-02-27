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

/** \file     TEncBinCoderCABAC.cpp
    \brief    binary entropy encoder of CABAC
*/

#include "TEncBinCoderCABACCounter.h"
#include "TLibCommon/TComRom.h"
#include "TLibCommon/Debug.h"


#if FAST_BIT_EST

//! \ingroup TLibEncoder
//! \{


TEncBinCABACCounter::TEncBinCABACCounter()
{
}

TEncBinCABACCounter::~TEncBinCABACCounter()
{
}

Void TEncBinCABACCounter::finish()
{
  m_pcTComBitIf->write(0, UInt(m_fracBits >> 15) );
  m_fracBits &= 32767;

#ifdef CVI_ENABLE_RDO_BIT_EST
  m_estFracBits = 0;
  m_actFracBits = 0;
#endif
}

#ifdef CVI_ENABLE_RDO_BIT_EST
UInt64 TEncBinCABACCounter::getSimEncBits(UInt type=0)
{
  return (type==0) ? m_actFracBits : (type==1) ? m_estFracBits : m_uiBinsCoded;
}
#endif

UInt TEncBinCABACCounter::getNumWrittenBits()
{
#ifdef CVI_ENABLE_RDO_BIT_EST
  if (getRDOBitEstMode()!=HM_ORIGINAL || getRDOSyntaxEstMode()!=HM_ORIGINAL)
  {
    return UInt(m_estFracBits >> g_estBitFracBd);
  }
  else
#endif
    return m_pcTComBitIf->getNumberOfWrittenBits() + UInt( m_fracBits >> 15 );
}

#ifdef CVI_ENABLE_RDO_BIT_EST
Void TEncBinCABACCounter::encodeBin( UInt  uiBin, ContextModel& rcCtxModel, UInt *scaleBit)
{
  if(m_bBitEstIncr){
    UInt64 fracBit;
    if(g_syntaxType==0)
    {
      fracBit = (getRDOSyntaxEstMode()==BIN_BASE)
                    ? g_estBitPrec
                    : (getRDOSyntaxEstMode()==HM_ORIGINAL)
                     ? rcCtxModel.getEntropyBits(uiBin)
                     : scaleBit[uiBin];
    }
    else // residual
    {
      fracBit = (getRDOBitEstMode()==BIN_BASE)
                    ? g_estBitPrec
                    : (getRDOBitEstMode()==HM_ORIGINAL)
                     ? rcCtxModel.getEntropyBits(uiBin)
                     : 0;
    }
    m_estFracBits += fracBit;
    m_estCbfBits[m_curCompID] += fracBit;
  }
  m_actFracBits += rcCtxModel.getHwEntropyBits(uiBin);
  m_uiBinsCoded += m_binCountIncrement;
  m_fracBits += rcCtxModel.getEntropyBits(uiBin);
  rcCtxModel.update(uiBin);
}
#endif

/**
 * \brief Encode bin
 *
 * \param binValue   bin value
 * \param rcCtxModel context model
 */
Void TEncBinCABACCounter::encodeBin( UInt binValue, ContextModel &rcCtxModel )
{
#if DEBUG_ENCODER_SEARCH_BINS
  const UInt64 startingFracBits = m_fracBits;
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
   if(m_bBitEstIncr){
    UInt64 fracBit;
    if(g_syntaxType==0){ // syntax
      fracBit = (getRDOSyntaxEstMode()==HM_ORIGINAL)
                    ?  rcCtxModel.getEntropyBits(binValue)
                    : g_estBitPrec;
    }
    else // residual
    {
      fracBit = (getRDOBitEstMode()==BIN_BASE)
                    ? g_estBitPrec
                    : (getRDOBitEstMode()==HM_ORIGINAL)
                     ? rcCtxModel.getEntropyBits(binValue)
                     : 0;
    }
    m_estFracBits += fracBit;
    m_estCbfBits[m_curCompID] += fracBit;
  }
  m_actFracBits += rcCtxModel.getHwEntropyBits( binValue );
#endif
  m_uiBinsCoded += m_binCountIncrement;
  m_fracBits += rcCtxModel.getEntropyBits( binValue );
  rcCtxModel.update( binValue );

#if DEBUG_ENCODER_SEARCH_BINS
  if ((g_debugCounter + debugEncoderSearchBinWindow) >= debugEncoderSearchBinTargetLine)
  {
    std::cout << g_debugCounter << ": coding bin value " << binValue << ", fracBits = [" << startingFracBits << "->" << m_fracBits << "]\n";
  }

  if (g_debugCounter >= debugEncoderSearchBinTargetLine)
  {
    UChar breakPointThis;
    breakPointThis = 7;
  }
  if (g_debugCounter >= (debugEncoderSearchBinTargetLine + debugEncoderSearchBinWindow))
  {
    exit(0);
  }
  g_debugCounter++;
#endif
}

/**
 * \brief Encode equiprobable bin
 *
 * \param binValue bin value
 */
Void TEncBinCABACCounter::encodeBinEP( UInt /*binValue*/ )
{
  m_uiBinsCoded += m_binCountIncrement;
  m_fracBits += 32768;

#ifdef CVI_ENABLE_RDO_BIT_EST
  m_estFracBits += (m_bBitEstIncr) ? g_estBitPrec : 0;
  m_actFracBits += g_estBitPrec;
#endif
}

/**
 * \brief Encode equiprobable bins
 *
 * \param binValues bin values
 * \param numBins number of bins
 */
Void TEncBinCABACCounter::encodeBinsEP( UInt /*binValues*/, Int numBins )
{
  m_uiBinsCoded += numBins & -m_binCountIncrement;
  m_fracBits += 32768 * numBins;
#ifdef CVI_ENABLE_RDO_BIT_EST
  m_estFracBits += (m_bBitEstIncr) ? g_estBitPrec * numBins : 0;
  m_actFracBits += g_estBitPrec * numBins;
#endif
}

/**
 * \brief Encode terminating bin
 *
 * \param binValue bin value
 */
Void TEncBinCABACCounter::encodeBinTrm( UInt binValue )
{
  m_uiBinsCoded += m_binCountIncrement;
  m_fracBits += ContextModel::getEntropyBitsTrm( binValue );

#ifdef CVI_ENABLE_RDO_BIT_EST
  m_estFracBits += (m_bBitEstIncr) ? g_estBitPrec : 0;
#endif
}

Void TEncBinCABACCounter::align()
{
  m_fracBits = (m_fracBits + 32767) & (~32767);
}
//! \}
#endif

