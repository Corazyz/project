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

/** \file     TComPicYuv.cpp
    \brief    picture YUV buffer class
*/

#include <cstdlib>
#include <assert.h>
#include <memory.h>

#ifdef __APPLE__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#include "TComPicYuv.h"
#include "Utilities/TVideoIOYuv.h"

//! \ingroup TLibCommon
//! \{

TComPicYuv::TComPicYuv()
{
  for(UInt i=0; i<MAX_NUM_COMPONENT; i++)
  {
    m_apiPicBuf[i]    = NULL;   // Buffer (including margin)
    m_piPicOrg[i]     = NULL;    // m_apiPicBufY + m_iMarginLuma*getStride() + m_iMarginLuma
  }

  for(UInt i=0; i<MAX_NUM_CHANNEL_TYPE; i++)
  {
    m_ctuOffsetInBuffer[i]=0;
    m_subCuOffsetInBuffer[i]=0;
  }

  m_bIsBorderExtended = false;
}




TComPicYuv::~TComPicYuv()
{
  destroy();
}



Void TComPicYuv::createWithoutCUInfo ( const Int picWidth,                 ///< picture width
                                       const Int picHeight,                ///< picture height
                                       const ChromaFormat chromaFormatIDC, ///< chroma format
                                       const Bool bUseMargin,              ///< if true, then a margin of uiMaxCUWidth+16 and uiMaxCUHeight+16 is created around the image.
                                       const UInt maxCUWidth,              ///< used for margin only
                                       const UInt maxCUHeight)             ///< used for margin only

{
  destroy();

  m_picWidth          = picWidth;
  m_picHeight         = picHeight;
  m_chromaFormatIDC   = chromaFormatIDC;
  m_marginX          = (bUseMargin?maxCUWidth:0) + 16;   // for 16-byte alignment
  m_marginY          = (bUseMargin?maxCUHeight:0) + 16;  // margin for 8-tap filter and infinite padding
  m_bIsBorderExtended = false;

  // assign the picture arrays and set up the ptr to the top left of the original picture
  for(UInt comp=0; comp<getNumberValidComponents(); comp++)
  {
    const ComponentID ch=ComponentID(comp);
    m_apiPicBuf[comp] = (Pel*)xMalloc( Pel, getStride(ch) * getTotalHeight(ch));
    m_piPicOrg[comp]  = m_apiPicBuf[comp] + (m_marginY >> getComponentScaleY(ch)) * getStride(ch) + (m_marginX >> getComponentScaleX(ch));
  }
  // initialize pointers for unused components to NULL
  for(UInt comp=getNumberValidComponents();comp<MAX_NUM_COMPONENT; comp++)
  {
    m_apiPicBuf[comp] = NULL;
    m_piPicOrg[comp]  = NULL;
  }

  for(Int chan=0; chan<MAX_NUM_CHANNEL_TYPE; chan++)
  {
    m_ctuOffsetInBuffer[chan]   = NULL;
    m_subCuOffsetInBuffer[chan] = NULL;
  }
}



Void TComPicYuv::create ( const Int picWidth,                 ///< picture width
                          const Int picHeight,                ///< picture height
                          const ChromaFormat chromaFormatIDC, ///< chroma format
                          const UInt maxCUWidth,              ///< used for generating offsets to CUs.
                          const UInt maxCUHeight,             ///< used for generating offsets to CUs.
                          const UInt maxCUDepth,              ///< used for generating offsets to CUs.
                          const Bool bUseMargin)              ///< if true, then a margin of uiMaxCUWidth+16 and uiMaxCUHeight+16 is created around the image.

{
  createWithoutCUInfo(picWidth, picHeight, chromaFormatIDC, bUseMargin, maxCUWidth, maxCUHeight);


  const Int numCuInWidth  = m_picWidth  / maxCUWidth  + (m_picWidth  % maxCUWidth  != 0);
  const Int numCuInHeight = m_picHeight / maxCUHeight + (m_picHeight % maxCUHeight != 0);
  for(Int chan=0; chan<MAX_NUM_CHANNEL_TYPE; chan++)
  {
    const ChannelType ch= ChannelType(chan);
    const Int ctuHeight = maxCUHeight>>getChannelTypeScaleY(ch);
    const Int ctuWidth  = maxCUWidth>>getChannelTypeScaleX(ch);
    const Int stride    = getStride(ch);

    m_ctuOffsetInBuffer[chan] = new Int[numCuInWidth * numCuInHeight];

    for (Int cuRow = 0; cuRow < numCuInHeight; cuRow++)
    {
      for (Int cuCol = 0; cuCol < numCuInWidth; cuCol++)
      {
        m_ctuOffsetInBuffer[chan][cuRow * numCuInWidth + cuCol] = stride * cuRow * ctuHeight + cuCol * ctuWidth;
      }
    }

    m_subCuOffsetInBuffer[chan] = new Int[(size_t)1 << (2 * maxCUDepth)];

    const Int numSubBlockPartitions=(1<<maxCUDepth);
    const Int minSubBlockHeight    =(ctuHeight >> maxCUDepth);
    const Int minSubBlockWidth     =(ctuWidth  >> maxCUDepth);

    for (Int buRow = 0; buRow < numSubBlockPartitions; buRow++)
    {
      for (Int buCol = 0; buCol < numSubBlockPartitions; buCol++)
      {
        m_subCuOffsetInBuffer[chan][(buRow << maxCUDepth) + buCol] = stride  * buRow * minSubBlockHeight + buCol * minSubBlockWidth;
      }
    }
  }
}

Void TComPicYuv::destroy()
{
  for(Int comp=0; comp<MAX_NUM_COMPONENT; comp++)
  {
    m_piPicOrg[comp] = NULL;

    if( m_apiPicBuf[comp] )
    {
      xFree( m_apiPicBuf[comp] );
      m_apiPicBuf[comp] = NULL;
    }
  }

  for(UInt chan=0; chan<MAX_NUM_CHANNEL_TYPE; chan++)
  {
    if (m_ctuOffsetInBuffer[chan])
    {
      delete[] m_ctuOffsetInBuffer[chan];
      m_ctuOffsetInBuffer[chan] = NULL;
    }
    if (m_subCuOffsetInBuffer[chan])
    {
      delete[] m_subCuOffsetInBuffer[chan];
      m_subCuOffsetInBuffer[chan] = NULL;
    }
  }
}

static Void writeTile64x64(unsigned char* src, unsigned char* tile_64x64, UInt src_stride, int in_tile_w, int in_tile_h)
{
  const int out_tile_w = 64, out_tile_h = 64;
  unsigned char* out_ptr = tile_64x64;

  for (int j = 0; j < out_tile_h; j += in_tile_h)
  {
      unsigned char* src_ptr = src + src_stride * j;
      for (int n = 0; n < out_tile_w; n += in_tile_w)
      {
        src_ptr += n;
        for (int m = 0; m < in_tile_h; m ++, out_ptr += in_tile_w)
        {
          memcpy(out_ptr, src_ptr + m * src_stride, sizeof(unsigned char) * in_tile_w);
        }
      }
  }
}

Bool  TComPicYuv::writeTilePlane(int sub_w, int sub_h)
{
  const int out_tile_w = 64, out_tile_h = 64;
  ComponentID compID = COMPONENT_Y;
  Pel* src = getAddr(compID);
  UInt stride = getStride(compID);
  UInt width = getWidth(compID);
  UInt height = getHeight(compID);

  for (int n = 0; n < height; n += out_tile_h)
  {
    for (int m = 0; m < width; m += out_tile_w)
    {
      unsigned char src_64x64[64 * 64] = {0};
      unsigned char tile_64x64[64 * 64] = {0};
      int max_h = out_tile_h, max_w = out_tile_w;
      if ((n + out_tile_h) > height)
        max_h = height - n;
      if ((m + out_tile_w) > width)
        max_w = width - m;
      for (int i = 0; i < max_h; i++)
        for (int j = 0; j < max_w; j++)
          src_64x64[64 * i + j] = (unsigned char)*(src + (n + i) * stride + m + j);

      writeTile64x64(src_64x64, tile_64x64, 64, sub_w, sub_h);
      sigdump_output_bin(&g_sigpool.top_rec_ctx[0], tile_64x64, 64 * 64);
    }
  }

  Pel* src_cb = getAddr(COMPONENT_Cb);
  Pel* src_cr = getAddr(COMPONENT_Cr);
  stride = getStride(COMPONENT_Cb);
  width = getWidth(COMPONENT_Cb);
  height = getHeight(COMPONENT_Cb);

  for (int n = 0; n < height; n += out_tile_h)
  {
    for (int m = 0; m < width; m += (out_tile_w / 2))
    {
      unsigned char src_64x64[64 * 64] = {0};
      unsigned char tile_64x64[64 * 64] = {0};
      int max_h = out_tile_h, max_w = out_tile_w / 2;
      if ((n + out_tile_h) > height)
        max_h = height - n;
      if ((m + out_tile_w / 2) > width)
        max_w = width - m;
      for (int i = 0; i < max_h; i++)
        for (int j = 0; j < max_w; j++)
        {
          src_64x64[64 * i + 2 * j] = (unsigned char)*(src_cb + (n + i) * stride + m + j);
          src_64x64[64 * i + 2 * j + 1] = (unsigned char)*(src_cr + (n + i) * stride + m + j);
        }
      writeTile64x64(src_64x64, tile_64x64, 64, sub_w, sub_h);
      sigdump_output_bin(&g_sigpool.top_rec_ctx[1], tile_64x64, 64 * 64);
    }
  }

  return true;
}

Void  TComPicYuv::copyToPic (TComPicYuv*  pcPicYuvDst) const
{
  assert( m_chromaFormatIDC == pcPicYuvDst->getChromaFormat() );

  for(Int comp=0; comp<getNumberValidComponents(); comp++)
  {
    const ComponentID compId=ComponentID(comp);
    const Int width     = getWidth(compId);
    const Int height    = getHeight(compId);
    const Int strideSrc = getStride(compId);
    assert(pcPicYuvDst->getWidth(compId) == width);
    assert(pcPicYuvDst->getHeight(compId) == height);
    if (strideSrc==pcPicYuvDst->getStride(compId))
    {
      ::memcpy ( pcPicYuvDst->getBuf(compId), getBuf(compId), sizeof(Pel)*strideSrc*getTotalHeight(compId));
    }
    else
    {
      const Pel *pSrc       = getAddr(compId);
            Pel *pDest      = pcPicYuvDst->getAddr(compId);
      const UInt strideDest = pcPicYuvDst->getStride(compId);

      for(Int y=0; y<height; y++, pSrc+=strideSrc, pDest+=strideDest)
      {
        ::memcpy(pDest, pSrc, width*sizeof(Pel));
      }
    }
  }
}


Void TComPicYuv::extendPicBorder ()
{
  if ( m_bIsBorderExtended )
  {
    return;
  }

  for(Int comp=0; comp<getNumberValidComponents(); comp++)
  {
    const ComponentID compId=ComponentID(comp);
    Pel *piTxt=getAddr(compId); // piTxt = point to (0,0) of image within bigger picture.
    const Int stride=getStride(compId);
    const Int width=getWidth(compId);
    const Int height=getHeight(compId);
    const Int marginX=getMarginX(compId);
    const Int marginY=getMarginY(compId);

    Pel*  pi = piTxt;
    // do left and right margins
    for (Int y = 0; y < height; y++)
    {
      for (Int x = 0; x < marginX; x++ )
      {
        pi[ -marginX + x ] = pi[0];
        pi[    width + x ] = pi[width-1];
      }
      pi += stride;
    }

    // pi is now the (0,height) (bottom left of image within bigger picture
    pi -= (stride + marginX);
    // pi is now the (-marginX, height-1)
    for (Int y = 0; y < marginY; y++ )
    {
      ::memcpy( pi + (y+1)*stride, pi, sizeof(Pel)*(width + (marginX<<1)) );
    }

    // pi is still (-marginX, height-1)
    pi -= ((height-1) * stride);
    // pi is now (-marginX, 0)
    for (Int y = 0; y < marginY; y++ )
    {
      ::memcpy( pi - (y+1)*stride, pi, sizeof(Pel)*(width + (marginX<<1)) );
    }
  }

  m_bIsBorderExtended = true;
}



// NOTE: This function is never called, but may be useful for developers.
Void TComPicYuv::dump (const std::string &fileName, const BitDepths &bitDepths, const Bool bAppend, const Bool bForceTo8Bit) const
{
  FILE *pFile = fopen (fileName.c_str(), bAppend?"ab":"wb");

  Bool is16bit=false;
  for(Int comp = 0; comp < getNumberValidComponents() && !bForceTo8Bit; comp++)
  {
    if (bitDepths.recon[toChannelType(ComponentID(comp))]>8)
    {
      is16bit=true;
    }
  }

  for(Int comp = 0; comp < getNumberValidComponents(); comp++)
  {
    const ComponentID  compId = ComponentID(comp);
    const Pel         *pi     = getAddr(compId);
    const Int          stride = getStride(compId);
    const Int          height = getHeight(compId);
    const Int          width  = getWidth(compId);

    if (is16bit)
    {
      for (Int y = 0; y < height; y++ )
      {
        for (Int x = 0; x < width; x++ )
        {
          UChar uc = (UChar)((pi[x]>>0) & 0xff);
          fwrite( &uc, sizeof(UChar), 1, pFile );
          uc = (UChar)((pi[x]>>8) & 0xff);
          fwrite( &uc, sizeof(UChar), 1, pFile );
        }
        pi += stride;
      }
    }
    else
    {
      const Int shift  = bitDepths.recon[toChannelType(compId)] - 8;
      const Int offset = (shift>0)?(1<<(shift-1)):0;
      for (Int y = 0; y < height; y++ )
      {
        for (Int x = 0; x < width; x++ )
        {
          UChar uc = (UChar)Clip3<Pel>(0, 255, (pi[x]+offset)>>shift);
          fwrite( &uc, sizeof(UChar), 1, pFile );
        }
        pi += stride;
      }
    }
  }

  fclose(pFile);
}

Bool TComPicYuv::writeTile( TComPicYuv* src, FILE *f_tile, int sub_w, int sub_h)
{
  const int out_tile_w = 64, out_tile_h = 64;
  ComponentID srcPlane = COMPONENT_Y;
  UInt width=src->getWidth(srcPlane);
  UInt height=src->getHeight(srcPlane);
  Pel *pSrc=src->getAddr(srcPlane);
  UInt strideSrc=src->getStride(srcPlane);

  for (int n = 0; n < height; n += out_tile_h)
  {
    for (int m = 0; m < width; m += out_tile_w)
    {
      unsigned char src_64x64[64 * 64] = {0};
      unsigned char tile_64x64[64 * 64] = {0};
      int max_h = out_tile_h, max_w = out_tile_w;
      if ((n + out_tile_h) > height)
        max_h = height - n;
      if ((m + out_tile_w) > width)
        max_w = width - m;
      for (int i = 0; i < max_h; i++)
        for (int j = 0; j < max_w; j++)
          src_64x64[64 * i + j] = (unsigned char)*(pSrc + (n + i) * strideSrc + m + j);

      writeTile64x64(src_64x64, tile_64x64, 64, sub_w, sub_h);
      fwrite(tile_64x64, 1, 64 * 64, f_tile);
    }
  }

  srcPlane = COMPONENT_Cb;
  width=src->getWidth(srcPlane);
  height=src->getHeight(srcPlane);
  strideSrc=src->getStride(srcPlane);
  Pel *pSrcCb=src->getAddr(COMPONENT_Cb);
  Pel *pSrcCr=src->getAddr(COMPONENT_Cr);
  for (int n = 0; n < height; n += out_tile_h)
  {
    for (int m = 0; m < width; m += (out_tile_w / 2))
    {
      unsigned char src_64x64[64 * 64] = {0};
      unsigned char tile_64x64[64 * 64] = {0};
      int max_h = out_tile_h, max_w = out_tile_w / 2;
      if ((n + out_tile_h) > height)
        max_h = height - n;
      if ((m + out_tile_w / 2) > width)
        max_w = width - m;
      for (int i = 0; i < max_h; i++)
        for (int j = 0; j < max_w; j++)
        {
          src_64x64[64 * i + 2 * j] = (unsigned char)*(pSrcCb + (n + i) * strideSrc + m + j);
          src_64x64[64 * i + 2 * j + 1] = (unsigned char)*(pSrcCr + (n + i) * strideSrc + m + j);
        }
      writeTile64x64(src_64x64, tile_64x64, 64, sub_w, sub_h);
      fwrite(tile_64x64, 1, 64 * 64, f_tile);
    }
  }

  return true;
}

Bool TComPicYuv::writeNV12( TComPicYuv* src, FILE *f_nv12, UInt out_pitch, int rotate_mode, int mirror)
{
  ComponentID srcPlane = COMPONENT_Y;
  UInt width=src->getWidth(srcPlane);
  UInt height=src->getHeight(srcPlane);
  Pel *pSrc=src->getAddr(srcPlane);
  UInt strideSrc=src->getStride(srcPlane);
  Pel padding_pix = 0x0;
  int x, y;
  if (mirror == 0)
  {
    //0 angle degrees
    if (rotate_mode == 0)
    {
      for (y = 0; y < height; y++)
      {
        for (x = 0; x < out_pitch; x++)
        {
            fwrite((x < width) ? (pSrc + y * strideSrc + x) : &padding_pix, 1, 1, f_nv12);
        }
      }
    }
    else if (rotate_mode == 1) //180 angle degrees
    {
      for (y = height - 1; y >= 0; y--)
      {
        for (x = width - 1; x >= 0; x--)
        {
            fwrite(pSrc + y * strideSrc + x, 1, 1, f_nv12);
        }
        for (x = width; x < out_pitch; x++)
        {
            fwrite(&padding_pix, 1, 1, f_nv12);
        }
      }
    }
    else if (rotate_mode  == 3) // 90 degrees
    {
        int rotate_pitch = cvi_mem_align(height, 32);
        for (x = 0; x < width; x++)
        {
          for (y = height - 1; y >= 0; y--)
          {
            fwrite((pSrc + y * strideSrc + x), 1, 1, f_nv12);
          }
          for (y = height; y < rotate_pitch; y++)
            fwrite(&padding_pix, 1, 1, f_nv12);
        }
    }
    else if (rotate_mode  == 2) // 270 degrees
    {
        int rotate_pitch = cvi_mem_align(height, 32);
        for (x = width - 1; x >= 0; x--)
        {
          for (y = 0; y < height; y++)
          {
            fwrite(pSrc + y * strideSrc + x, 1, 1, f_nv12);
          }
          for (y = height; y < rotate_pitch; y++)
            fwrite(&padding_pix, 1, 1, f_nv12);
        }
    }
  }
  else
  {
    // h mirror
    if (mirror == 1)
    {
      for (y = 0; y < height; y++)
      {
        for (x = width - 1; x >= 0; x--)
        {
            fwrite(pSrc + y * strideSrc + x, 1, 1, f_nv12);
        }
        for (x = width; x < out_pitch; x++)
        {
            fwrite(&padding_pix, 1, 1, f_nv12);
        }
      }
    }
    else //v mirror
    {
      for (y = height - 1; y >= 0; y--)
      {
        for (x = 0; x < out_pitch; x++)
        {
            fwrite((x < width) ? (pSrc + y * strideSrc + x) : &padding_pix, 1, 1, f_nv12);
        }
      }
    }
  }
  srcPlane = COMPONENT_Cb;
  width=src->getWidth(srcPlane);
  height=src->getHeight(srcPlane);
  strideSrc=src->getStride(srcPlane);


  Pel *pSrcCb=src->getAddr(COMPONENT_Cb);
  Pel *pSrcCr=src->getAddr(COMPONENT_Cr);

  if (mirror == 0)
  {
    if (rotate_mode == 0)
    {
      for (y = 0; y < height; y++)
      {
        for (x = 0; x < out_pitch / 2; x++)
        {
          fwrite((x < width) ? (pSrcCb + y * strideSrc + x) : &padding_pix, 1, 1, f_nv12);
          fwrite((x < width) ? (pSrcCr + y * strideSrc + x) : &padding_pix, 1, 1, f_nv12);
        }
      }
    }
    else if (rotate_mode == 1)
    {
      for (y = height - 1; y >= 0; y--)
      {
        for (x = width - 1; x >= 0; x--)
        {
          fwrite(pSrcCb + y * strideSrc + x, 1, 1, f_nv12);
          fwrite(pSrcCr + y * strideSrc + x, 1, 1, f_nv12);
        }
        for (x = 2 * width; x < out_pitch; x++)
        {
          fwrite(&padding_pix, 1, 1, f_nv12);
        }
      }
    }
    else if (rotate_mode  == 3) // 90 degrees
    {
        int rotate_pitch = cvi_mem_align(2 * height, 32);
        for (x = 0; x < width; x++)
        {
          for (y = height - 1; y >= 0; y--)
          {
            fwrite((pSrcCb + y * strideSrc + x), 1, 1, f_nv12);
            fwrite((pSrcCr + y * strideSrc + x), 1, 1, f_nv12);
          }
          for (y = 2 * height; y < rotate_pitch; y++)
            fwrite(&padding_pix, 1, 1, f_nv12);
        }
    }
    else if (rotate_mode  == 2) // 270 degrees
    {
        int rotate_pitch = cvi_mem_align(2 * height, 32);
        for (x = width - 1; x >= 0; x--)
        {
          for (y = 0; y < height; y++)
          {
            fwrite(pSrcCb + y * strideSrc + x, 1, 1, f_nv12);
            fwrite(pSrcCr + y * strideSrc + x, 1, 1, f_nv12);
          }
          for (y = 2 * height; y < rotate_pitch; y++)
            fwrite(&padding_pix, 1, 1, f_nv12);
        }
    }
  }
  else
  {
    // h mirror
    if (mirror == 1)
    {
      for (y = 0; y < height; y++)
      {
        for (x = width - 1; x >= 0; x--)
        {
          fwrite(pSrcCb + y * strideSrc + x, 1, 1, f_nv12);
          fwrite(pSrcCr + y * strideSrc + x, 1, 1, f_nv12);
        }
        for (x = 2 * width; x < out_pitch; x++)
        {
          fwrite(&padding_pix, 1, 1, f_nv12);
        }
      }
    }
    else //v mirror
    {
      for (y = height - 1; y >= 0; y--)
      {
        for (x = 0; x < out_pitch / 2; x++)
        {
          fwrite((x < width) ? (pSrcCb + y * strideSrc + x) : &padding_pix, 1, 1, f_nv12);
          fwrite((x < width) ? (pSrcCr + y * strideSrc + x) : &padding_pix, 1, 1, f_nv12);
        }
      }
    }
  }
  return true;
}

//! \}
