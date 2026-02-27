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

#include <iostream>
#include <cstring>
#include "TEncBinCoderCABAC.h"
#include "TLibCommon/TComRom.h"
#include "TLibCommon/Debug.h"
#include "TLibCommon/cvi_sigdump.h"

//! \ingroup TLibEncoder
//! \{


TEncBinCABAC::TEncBinCABAC()
: m_pcTComBitIf( 0 )
, m_binCountIncrement( 0 )
#if FAST_BIT_EST
, m_fracBits( 0 )
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
, m_estFracBits(0)
, m_actFracBits(0)
, m_bBitEstIncr(true)
, m_curCompID(0)
#endif
{
#ifdef CVI_ENABLE_RDO_BIT_EST
  memset(&m_estCbfBits[0], 0, sizeof(m_estCbfBits));
#endif
}

TEncBinCABAC::~TEncBinCABAC()
{
}

Void TEncBinCABAC::init( TComBitIf* pcTComBitIf )
{
  m_pcTComBitIf = pcTComBitIf;
}

Void TEncBinCABAC::uninit()
{
  m_pcTComBitIf = 0;
}

Void TEncBinCABAC::start()
{
  m_uiLow            = 0;
  m_uiLow_cvi        = 0;
  m_uiRange          = 510;
  m_bitsLeft         = 23;
  m_numBufferedBytes = 0;
  m_bufferedByte     = 0xff;
#if FAST_BIT_EST
  m_fracBits         = 0;
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
  m_estFracBits      = 0;
  m_actFracBits      = 0;
#endif
}

Void TEncBinCABAC::finish()
{
  if ( m_uiLow >> ( 32 - m_bitsLeft ) )
  {
    //assert( m_numBufferedBytes > 0 );
    //assert( m_bufferedByte != 0xff );
    m_pcTComBitIf->write( m_bufferedByte + 1, 8 );
    while ( m_numBufferedBytes > 1 )
    {
      m_pcTComBitIf->write( 0x00, 8 );
      m_numBufferedBytes--;
    }
    m_uiLow -= 1 << ( 32 - m_bitsLeft );
  }
  else
  {
    if ( m_numBufferedBytes > 0 )
    {
      m_pcTComBitIf->write( m_bufferedByte, 8 );
    }
    while ( m_numBufferedBytes > 1 )
    {
      m_pcTComBitIf->write( 0xff, 8 );
      m_numBufferedBytes--;
    }
  }
  m_pcTComBitIf->write( m_uiLow >> 8, 24 - m_bitsLeft );
}

Void TEncBinCABAC::flush()
{
  encodeBinTrm(1);
  finish();
  m_pcTComBitIf->write(1, 1);
  m_pcTComBitIf->writeAlignZero();

  start();
}

/** Reset BAC register and counter values.
 * \returns Void
 */
Void TEncBinCABAC::resetBac()
{
  start();
}

/** Encode PCM alignment zero bits.
 * \returns Void
 */
Void TEncBinCABAC::encodePCMAlignBits()
{
  finish();
  m_pcTComBitIf->write(1, 1);
  m_pcTComBitIf->writeAlignZero(); // pcm align zero
}

/** Write a PCM code.
 * \param uiCode code value
 * \param uiLength code bit-depth
 * \returns Void
 */
Void TEncBinCABAC::xWritePCMCode(UInt uiCode, UInt uiLength)
{
  m_pcTComBitIf->write(uiCode, uiLength);
}

Void TEncBinCABAC::copyState( const TEncBinIf* pcTEncBinIf )
{
  const TEncBinCABAC* pcTEncBinCABAC = pcTEncBinIf->getTEncBinCABAC();
  m_uiLow           = pcTEncBinCABAC->m_uiLow;
  m_uiRange         = pcTEncBinCABAC->m_uiRange;
  m_bitsLeft        = pcTEncBinCABAC->m_bitsLeft;
  m_bufferedByte    = pcTEncBinCABAC->m_bufferedByte;
  m_numBufferedBytes = pcTEncBinCABAC->m_numBufferedBytes;
#if FAST_BIT_EST
  m_fracBits = pcTEncBinCABAC->m_fracBits;
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
  m_estFracBits = pcTEncBinCABAC->m_estFracBits;
  m_actFracBits = pcTEncBinCABAC->m_actFracBits;
#endif
}

Void TEncBinCABAC::resetBits()
{
  m_uiLow            = 0;
  m_uiLow_cvi        = 0;
  m_bitsLeft         = 23;
  m_numBufferedBytes = 0;
  m_bufferedByte     = 0xff;
  if ( m_binCountIncrement )
  {
    m_uiBinsCoded = 0;
  }
#if FAST_BIT_EST
  m_fracBits &= 32767;
#endif
#ifdef CVI_ENABLE_RDO_BIT_EST
  m_estFracBits = 0;
  m_actFracBits = 0;
  memset(&m_estCbfBits[0], 0, sizeof(m_estCbfBits));
#endif
}

UInt TEncBinCABAC::getNumWrittenBits()
{
  return m_pcTComBitIf->getNumberOfWrittenBits() + 8 * m_numBufferedBytes + 23 - m_bitsLeft;
}

#ifdef CVI_ENABLE_RDO_BIT_EST
Void TEncBinCABAC::encodeBin( UInt  uiBin,  ContextModel& rcCtxModel, UInt *scale)
{
  encodeBin(uiBin, rcCtxModel);
}
#endif

static void profile_chk_outstandingbit()
{
  if (g_sigdump.cabac_prof && g_sigpool.cabac_is_record) {
    int msb_pos = 5;
    int low_msb = g_sigpool.put;
    for(int iter=0; iter<g_sigpool.numBits; iter++) {
      Bool os_detect = (((low_msb>>msb_pos)&0x03)==0x1);
      if(os_detect) {
        g_sigpool.ostanding_bit_cnt ++;
        int sub_msb_mask = (1<<msb_pos);
        low_msb -= sub_msb_mask;
      }
      else {
        int hist_max = (1 << (6+g_sigdump.cabac_prof_os_bin_w)) - 1;
        int ostanding_hist_idx = min(hist_max, ((int)g_sigpool.ostanding_bit_cnt))>>g_sigdump.cabac_prof_os_bin_w;
        g_sigpool.ostanding_hist[ostanding_hist_idx] += 1;
        g_sigpool.ostanding_bit_cnt = 0;
      }
      msb_pos--;
    }
  }
}

static UInt updateLow_cvi_BP(UInt low)
{
  profile_chk_outstandingbit();
  return ((low & 0x400) == 0x400) ? (low & 0x3ff) : (low & 0x1ff);
}

static UInt updateLow_cvi(UInt low, UInt putBits)
{
  profile_chk_outstandingbit();
  UInt retLow = 0;
  switch(putBits)
  {
    case 0 : retLow =   low;
      break;
    case 1 : retLow = ((low & 0x200) == 0x200) ? ((low & 0x1ff) << 1) : ((low & 0xff) << 1);
      break;
    case 2 : retLow = ((low & 0x300) == 0x300) ? ((low & 0xff) << 2) : ((low & 0x7f) << 2);
      break;
    case 3 : retLow = ((low & 0x380) == 0x380) ? ((low & 0x7f) << 3) : ((low & 0x3f) << 3);
      break;
    case 4 : retLow = ((low & 0x3C0) == 0x3C0) ? ((low & 0x3f) << 4) : ((low & 0x1f) << 4);
      break;
    case 5 : retLow = ((low & 0x3E0) == 0x3E0) ? ((low & 0x1f) << 5) : ((low & 0xf) << 5);
      break;
    default : retLow = ((low & 0x3F0) == 0x3F0) ? ((low & 0xf) << 6) : ((low & 0x7) << 6);
  }
  return retLow;
}

/**
 * \brief Encode bin
 *
 * \param binValue   bin value
 * \param rcCtxModel context model
 */
Void TEncBinCABAC::encodeBin( UInt binValue, ContextModel &rcCtxModel )
{
  //{
  //  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  //  DTRACE_CABAC_T( "\tstate=" )
  //  DTRACE_CABAC_V( ( rcCtxModel.getState() << 1 ) + rcCtxModel.getMps() )
  //  DTRACE_CABAC_T( "\tsymbol=" )
  //  DTRACE_CABAC_V( binValue )
  //  DTRACE_CABAC_T( "\n" )
  //}

#if DEBUG_CABAC_BINS
  const UInt startingRange = m_uiRange;
#endif

  m_uiBinsCoded += m_binCountIncrement;
  rcCtxModel.setBinsCoded( 1 );

  UInt  uiLPS   = TComCABACTables::sm_aucLPSTable[ rcCtxModel.getState() ][ ( m_uiRange >> 6 ) & 3 ];
#ifdef SIG_CABAC
  if (is_enable_cabac_hw_emulator())
  {
    g_sigpool.range = m_uiRange;
    g_sigpool.low = m_uiLow_cvi;
    g_sigpool.bin_type = 0;
    g_sigpool.bin_value = binValue;
    g_sigpool.state[0] = rcCtxModel.getState();
    g_sigpool.mps[0] = rcCtxModel.getMps();
  }
#endif
  m_uiRange    -= uiLPS;

  if( binValue != rcCtxModel.getMps() )
  {
    Int numBits = TComCABACTables::sm_aucRenormTable[ uiLPS >> 3 ];

    m_uiLow     = ( m_uiLow + m_uiRange ) << numBits;
#ifdef SIG_CABAC
    if (is_enable_cabac_hw_emulator())
    {
      m_uiLow_cvi = m_uiLow_cvi + m_uiRange;
      g_sigpool.numBits = numBits;
      g_sigpool.put = (m_uiLow_cvi >> 3) & 0x7f;
    }
#endif
    m_uiRange   = uiLPS << numBits;
    rcCtxModel.updateLPS();
    m_bitsLeft -= numBits;
    testAndWriteOut();
  }
  else
  {
    rcCtxModel.updateMPS();

    if ( m_uiRange < 256 )
    {
#ifdef SIG_CABAC
      if (is_enable_cabac_hw_emulator())
      {
        g_sigpool.numBits = 1;
        g_sigpool.put = (m_uiLow_cvi >> 3) & 0x7f;
      }
#endif
      m_uiLow <<= 1;
      m_uiRange <<= 1;
      m_bitsLeft--;
      testAndWriteOut();
    }
#ifdef SIG_CABAC
    else  if (is_enable_cabac_hw_emulator())
    {
      g_sigpool.numBits = 0;
      g_sigpool.put = (m_uiLow_cvi >> 3) & 0x7f;
    }
#endif
  }

#ifdef SIG_CABAC
  if (is_enable_cabac_hw_emulator())
  {
    m_uiLow_cvi = updateLow_cvi(m_uiLow_cvi, g_sigpool.numBits);
    g_sigpool.state[1] = rcCtxModel.getState();
    g_sigpool.mps[1] = rcCtxModel.getMps();
    sigdump_output_cabac_info();
    std::strncpy( g_sigpool.syntax_name, "NULL_BIN", sizeof(g_sigpool.syntax_name) );
  }
#endif
#if DEBUG_CABAC_BINS
  if ((g_debugCounter + debugCabacBinWindow) >= debugCabacBinTargetLine)
  {
    std::cout << g_debugCounter << ": coding bin value " << binValue << ", range = [" << startingRange << "->" << m_uiRange << "]\n";
  }

  if (g_debugCounter >= debugCabacBinTargetLine)
  {
    UChar breakPointThis;
    breakPointThis = 7;
  }
  if (g_debugCounter >= (debugCabacBinTargetLine + debugCabacBinWindow))
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
Void TEncBinCABAC::encodeBinEP( UInt binValue )
{
  if (false)
  {
    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tEPsymbol=" )
    DTRACE_CABAC_V( binValue )
    DTRACE_CABAC_T( "\n" )
  }

  m_uiBinsCoded += m_binCountIncrement;
#ifdef SIG_CABAC
  UInt uiRange_cvi = m_uiRange;
  if (is_enable_cabac_hw_emulator())
  {
    g_sigpool.low = m_uiLow_cvi;
    g_sigpool.range = uiRange_cvi;
    g_sigpool.bin_type = 1;
    m_uiLow_cvi <<= 1;
    if( binValue ) {
      m_uiLow_cvi += uiRange_cvi;
    }
    g_sigpool.bin_value = binValue;
    g_sigpool.numBits = 1;
    g_sigpool.put = (m_uiLow_cvi >> 4) & 0x7f;
    m_uiLow_cvi = updateLow_cvi_BP(m_uiLow_cvi);
    sigdump_output_cabac_info();
    std::strncpy( g_sigpool.syntax_name, "NULL_EP", sizeof(g_sigpool.syntax_name) );
  }
#endif
  if (m_uiRange == 256)
  {
    encodeAlignedBinsEP(binValue, 1);
    return;
  }

  m_uiLow <<= 1;
  if( binValue )
  {
    m_uiLow += m_uiRange;
  }
  m_bitsLeft--;

  testAndWriteOut();
}

/**
 * \brief Encode equiprobable bins
 *
 * \param binValues bin values
 * \param numBins number of bins
 */
Void TEncBinCABAC::encodeBinsEP( UInt binValues, Int numBins )
{
  m_uiBinsCoded += numBins & -m_binCountIncrement;

  if (false)
  {
    for ( Int i = 0; i < numBins; i++ )
    {
      DTRACE_CABAC_VL( g_nSymbolCounter++ )
      DTRACE_CABAC_T( "\tEPsymbol=" )
      DTRACE_CABAC_V( ( binValues >> ( numBins - 1 - i ) ) & 1 )
      DTRACE_CABAC_T( "\n" )
    }
  }
#ifdef SIG_CABAC
  UInt uiRange_cvi = m_uiRange;

  if (is_enable_cabac_hw_emulator())
  {
    UInt binsRemaining = numBins;

    g_sigpool.range = uiRange_cvi;
    g_sigpool.bin_type = 1;
    while(binsRemaining)
    {
      const UInt binsToCode = std::min<UInt>(binsRemaining, 8);
      for (Int i = 0; i < binsToCode; i++) {
        UInt binValue = (binValues >> (binsRemaining - 1 - i)) & 1;
        g_sigpool.low = m_uiLow_cvi;

        m_uiLow_cvi <<= 1;
        if( binValue ) {
          m_uiLow_cvi += uiRange_cvi;
        }

        g_sigpool.bin_value = binValue;
        g_sigpool.numBits = 1;
        g_sigpool.put = (m_uiLow_cvi >> 4) & 0x7f;
        m_uiLow_cvi = updateLow_cvi_BP(m_uiLow_cvi);
        sigdump_output_cabac_info();
      }
      binsRemaining -= binsToCode;
    }
    if(g_sigdump.cabac)
      std::strncpy( g_sigpool.syntax_name, "NULL_sEP", sizeof(g_sigpool.syntax_name) );
  }
#endif
  if (m_uiRange == 256)
  {
    encodeAlignedBinsEP(binValues, numBins);
    return;
  }

  while ( numBins > 8 )
  {
    numBins -= 8;
    UInt pattern = binValues >> numBins;
    m_uiLow <<= 8;
    m_uiLow += m_uiRange * pattern;
    binValues -= pattern << numBins;
    m_bitsLeft -= 8;

    testAndWriteOut();
  }

  m_uiLow <<= numBins;
  m_uiLow += m_uiRange * binValues;
  m_bitsLeft -= numBins;

  testAndWriteOut();
}

Void TEncBinCABAC::align()
{
  m_uiRange = 256;
}

Void TEncBinCABAC::encodeAlignedBinsEP( UInt binValues, Int numBins )
{
  Int binsRemaining = numBins;

  assert(m_uiRange == 256); //aligned encode only works when range = 256

  while (binsRemaining > 0)
  {
    const UInt binsToCode = std::min<UInt>(binsRemaining, 8); //code bytes if able to take advantage of the system's byte-write function
    const UInt binMask    = (1 << binsToCode) - 1;

    const UInt newBins = (binValues >> (binsRemaining - binsToCode)) & binMask;

    //The process of encoding an EP bin is the same as that of coding a normal
    //bin where the symbol ranges for 1 and 0 are both half the range:
    //
    //  low = (low + range/2) << 1       (to encode a 1)
    //  low =  low            << 1       (to encode a 0)
    //
    //  i.e.
    //  low = (low + (bin * range/2)) << 1
    //
    //  which is equivalent to:
    //
    //  low = (low << 1) + (bin * range)
    //
    //  this can be generalised for multiple bins, producing the following expression:
    //
    m_uiLow = (m_uiLow << binsToCode) + (newBins << 8); //range is known to be 256

    binsRemaining -= binsToCode;
    m_bitsLeft    -= binsToCode;

    testAndWriteOut();
  }
}

/**
 * \brief Encode terminating bin
 *
 * \param binValue bin value
 */
Void TEncBinCABAC::encodeBinTrm( UInt binValue )
{
  m_uiBinsCoded += m_binCountIncrement;
#ifdef SIG_CABAC
  if (is_enable_cabac_hw_emulator())
  {
    g_sigpool.low = m_uiLow_cvi;
    g_sigpool.range = m_uiRange;
    g_sigpool.bin_type = 2;
    g_sigpool.bin_value = binValue;
  }
#endif
  m_uiRange -= 2;
  if( binValue )
  {
    m_uiLow  += m_uiRange;
    m_uiLow <<= 7;
    m_uiRange = 2 << 7;
    m_bitsLeft -= 7;
  }
  else if ( m_uiRange >= 256 )
  {
#ifdef SIG_CABAC
    if (is_enable_cabac_hw_emulator())
    {
      g_sigpool.numBits = 0;
      g_sigpool.put = (m_uiLow_cvi >> 3) & 0x7f;
      m_uiLow_cvi = updateLow_cvi(m_uiLow_cvi, g_sigpool.numBits);
      sigdump_output_cabac_info();
      std::strncpy( g_sigpool.syntax_name, "NULL_Trm", sizeof(g_sigpool.syntax_name) );
    }
#endif
    return;
  }
  else
  {
    m_uiLow   <<= 1;
    m_uiRange <<= 1;
    m_bitsLeft--;
  }
#ifdef SIG_CABAC
  if (is_enable_cabac_hw_emulator())
  {
    g_sigpool.numBits = 1;
    g_sigpool.put = (m_uiLow_cvi >> 3) & 0x7f;
    m_uiLow_cvi = updateLow_cvi(m_uiLow_cvi, g_sigpool.numBits);
    sigdump_output_cabac_info();
    std::strncpy( g_sigpool.syntax_name, "NULL_Trm", sizeof(g_sigpool.syntax_name) );
  }
#endif
  testAndWriteOut();
}

Void TEncBinCABAC::testAndWriteOut()
{
  if ( m_bitsLeft < 12 )
  {
    writeOut();
  }
}

/**
 * \brief Move bits from register into bitstream
 */
Void TEncBinCABAC::writeOut()
{
  UInt leadByte = m_uiLow >> (24 - m_bitsLeft);
  m_bitsLeft += 8;
  m_uiLow &= 0xffffffffu >> m_bitsLeft;

  if ( leadByte == 0xff )
  {
    m_numBufferedBytes++;
  }
  else
  {
    if ( m_numBufferedBytes > 0 )
    {
      UInt carry = leadByte >> 8;
      UInt byte = m_bufferedByte + carry;
      m_bufferedByte = leadByte & 0xff;
      m_pcTComBitIf->write( byte, 8 );

      byte = ( 0xff + carry ) & 0xff;
      while ( m_numBufferedBytes > 1 )
      {
        m_pcTComBitIf->write( byte, 8 );
        m_numBufferedBytes--;
      }
    }
    else
    {
      m_numBufferedBytes = 1;
      m_bufferedByte = leadByte;
    }
  }
}

Void TEncBinCABAC::setEstCurrentType(Int compID)
{
  m_curCompID = compID;
}
//! \}
