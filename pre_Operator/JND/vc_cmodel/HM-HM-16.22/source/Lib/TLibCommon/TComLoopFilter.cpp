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

/** \file     TComLoopFilter.cpp
    \brief    deblocking filter
*/

#include "TComLoopFilter.h"
#include "TComSlice.h"
#include "TComMv.h"
#include "TComTU.h"
#include "TLibCommon/cvi_sigdump.h"
#include "cvi_pattern.h"
//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

//#define   EDGE_VER    0
//#define   EDGE_HOR    1

#define DEFAULT_INTRA_TC_OFFSET 2 ///< Default intra TC offset

// ====================================================================================================================
// Tables
// ====================================================================================================================

const UChar TComLoopFilter::sm_tcTable[MAX_QP + 1 + DEFAULT_INTRA_TC_OFFSET] =
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,5,5,6,6,7,8,9,10,11,13,14,16,18,20,22,24
};

const UChar TComLoopFilter::sm_betaTable[MAX_QP + 1] =
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,7,8,9,10,11,12,13,14,15,16,17,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64
};

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComLoopFilter::TComLoopFilter()
: m_uiNumPartitions(0)
, m_bLFCrossTileBoundary(true)
{
  for( Int edgeDir = 0; edgeDir < NUM_EDGE_DIR; edgeDir++ )
  {
    m_aapucBS       [edgeDir] = NULL;
    m_aapbEdgeFilter[edgeDir] = NULL;
  }
}

TComLoopFilter::~TComLoopFilter()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================
Void TComLoopFilter::setCfg( Bool bLFCrossTileBoundary )
{
  m_bLFCrossTileBoundary = bLFCrossTileBoundary;
}

Void TComLoopFilter::create( UInt uiMaxCUDepth )
{
  destroy();
  m_uiNumPartitions = 1 << ( uiMaxCUDepth<<1 );
  for( Int edgeDir = 0; edgeDir < NUM_EDGE_DIR; edgeDir++ )
  {
    m_aapucBS       [edgeDir] = new UChar[m_uiNumPartitions];
    m_aapbEdgeFilter[edgeDir] = new Bool [m_uiNumPartitions];
  }
}

Void TComLoopFilter::destroy()
{
  for( Int edgeDir = 0; edgeDir < NUM_EDGE_DIR; edgeDir++ )
  {
    if (m_aapucBS[edgeDir] != NULL)
    {
      delete [] m_aapucBS[edgeDir];
      m_aapucBS[edgeDir] = NULL;
    }

    if (m_aapbEdgeFilter[edgeDir])
    {
      delete [] m_aapbEdgeFilter[edgeDir];
      m_aapbEdgeFilter[edgeDir] = NULL;
    }
  }
}

/**
 - call deblocking function for every CU
 .
 \param  pcPic   picture class (TComPic) pointer
 */
Void TComLoopFilter::loopFilterPic( TComPic* pcPic )
{
  // Horizontal filtering
  for ( UInt ctuRsAddr = 0; ctuRsAddr < pcPic->getNumberOfCtusInFrame(); ctuRsAddr++ )
  {
    TComDataCU* pCtu = pcPic->getCtu( ctuRsAddr );

    ::memset( m_aapucBS       [EDGE_VER], 0, sizeof( UChar ) * m_uiNumPartitions );
    ::memset( m_aapbEdgeFilter[EDGE_VER], 0, sizeof( Bool  ) * m_uiNumPartitions );

    // CU-based deblocking
    //
#ifdef SIG_PPU
    if (g_sigdump.ppu)
    {
#ifdef SIG_PPU_OLD
      for (int i = 0; i < pCtu->getTotalNumPart(); i++)
      {
        //printf("(%3d ,%3d) ",  pCtu->getCUPelX() + g_auiRasterToPelX[i] ,  pCtu->getCUPelY() + g_auiRasterToPelY[i]);
        Pel *piSrc;
        Pel pixel_value[16];
        Int iStride = pCtu->getPic()->getPicYuvRec()->getStride(COMPONENT_Y);
        //std::cout<<"istride="<<iStride<<endl;
        piSrc = pCtu->getPic()->getPicYuvRec()->getAddr(COMPONENT_Y,  pCtu->getCtuRsAddr(),g_auiRasterToZscan[i]);
        //std::cout<<"z_index="<<g_auiRasterToZscan[i]<<"pisrc="<<piSrc<<endl;
        //std::cout<<"z_index_internal"<<g_auiRasterToZscan[i]<<"pisrc="<<piSrc+iStride<<endl;
        for (int i=0 ; i< 4 ;i++)
        {
          for (int j=0 ; j<4 ; j++) {
            pixel_value[i*4+j] = *(piSrc+j);
            // std::cout<<"pisrc="<<piSrc+j<<endl;
          }
          piSrc = piSrc+iStride ;
          // std::cout<<"reset_pisrc="<<piSrc<<endl;
        }

        Pel *piSrc_cb;
        Pel pixel_value_cb[4];
        Int iStride_cb = pCtu->getPic()->getPicYuvRec()->getStride(COMPONENT_Cb);
        // std::cout<<"istride_cb="<<iStride_cb<<endl;
        piSrc_cb = pCtu->getPic()->getPicYuvRec()->getAddr(COMPONENT_Cb,  pCtu->getCtuRsAddr(),g_auiRasterToZscan[i]);
        // std::cout<<"z_index="<<g_auiRasterToZscan[i]<<"pisrc_cb="<<piSrc_cb<<endl;
        // std::cout<<"z_index_internal"<<g_auiRasterToZscan[i]<<"pisrc="<<piSrc+iStride<<endl;
        for (int i=0 ; i< 2 ;i++)
        {
          for (int j=0 ; j<2 ; j++)
          {
            pixel_value_cb[i*2+j] = *(piSrc_cb+j);
            // std::cout<<"pisrc_cb="<<piSrc_cb+j<<endl;
          }
          piSrc_cb = piSrc_cb+iStride_cb ;
          // std::cout<<"reset_pisrc="<<piSrc<<endl;
        }

        Pel *piSrc_cr;
        Pel pixel_value_cr[4];
        Int iStride_cr = pCtu->getPic()->getPicYuvRec()->getStride(COMPONENT_Cr);
        // std::cout<<"istride_cr="<<iStride_cr<<endl;
        piSrc_cr = pCtu->getPic()->getPicYuvRec()->getAddr(COMPONENT_Cr,  pCtu->getCtuRsAddr(),g_auiRasterToZscan[i]);
        // std::cout<<"z_index="<<g_auiRasterToZscan[i]<<"pisrc_cr="<<piSrc_cr<<endl;
        //std::cout<<"z_index_internal"<<g_auiRasterToZscan[i]<<"pisrc="<<piSrc+iStride<<endl;
        for (int i=0 ; i< 2 ;i++)
        {
          for (int j=0 ; j<2 ; j++){
            pixel_value_cr[i*2+j] = *(piSrc_cr+j);
            // std::cout<<"pisrc_cr="<<piSrc_cr+j<<endl;
          }
          piSrc_cr = piSrc_cr+iStride_cr ;
          // std::cout<<"reset_pisrc="<<piSrc<<endl;
        }

        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"ctu_index=%d,",pCtu->getCtuRsAddr());
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"z_index=%d,",g_auiRasterToZscan[i]);
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"x=%d,",pCtu->getCUPelX() + g_auiRasterToPelX[i]);
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"y=%d,",pCtu->getCUPelY() + g_auiRasterToPelY[i]);
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p0=%x,",pixel_value[0] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p1=%x,",pixel_value[1] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p2=%x,",pixel_value[2] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p3=%x,",pixel_value[3] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p4=%x,",pixel_value[4] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p5=%x,",pixel_value[5] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p6=%x,",pixel_value[6] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p7=%x,",pixel_value[7] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p8=%x,",pixel_value[8] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p9=%x,",pixel_value[9] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p10=%x,",pixel_value[10] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p11=%x,",pixel_value[11] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p12=%x,",pixel_value[12] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p13=%x,",pixel_value[13] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p14=%x,",pixel_value[14] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"p15=%x,",pixel_value[15] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"cb_p0=%x,",pixel_value_cb[0] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"cb_p1=%x,",pixel_value_cb[1] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"cb_p2=%x,",pixel_value_cb[2] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"cb_p3=%x,",pixel_value_cb[3] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"cr_p0=%x,",pixel_value_cr[0] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"cr_p1=%x,",pixel_value_cr[1] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"cr_p2=%x,",pixel_value_cr[2] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"cr_p3=%x,",pixel_value_cr[3] );
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[0],"qp_value=%x\n",pCtu->getQP(g_auiRasterToZscan[i]));
      }

      // printf("\n---------------CTU Address(Raster Scan): %d-----------------\n", pCtu->getCtuRsAddr());
	    // printf("\n---------------CU starts--------------\n");
      for (int i = 0; i < pCtu->getTotalNumPart(); i++)
      {
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[2],"ctu_index=%d, z_index=%d, cu_depth=%d, tu_depth=%d\n ", pCtu->getCtuRsAddr(),g_auiRasterToZscan[i], pCtu->getDepth(g_auiRasterToZscan[i]) ,pCtu->getTransformIdx(g_auiRasterToZscan[i]) + pCtu->getDepth(g_auiRasterToZscan[i]) );
      }

      int iWidthInPart = (pCtu->getSlice()->getSPS()->getMaxCUWidth())>>2;
      int iCount = 0;
      // printf("\n---------------CU starts--------------\n");
      for (int i = 0; i < pCtu->getTotalNumPart(); i++)
      {
        if ( (iCount & (iWidthInPart - 1)) == 0)
        {
          // printf("\n");
          // printf("(%3d, %3d) ",g_auiRasterToZscan[i], pCtu->getTransformIdx(g_auiRasterToZscan[i]) + pCtu->getDepth(g_auiRasterToZscan[i]));
          // printf("(%3d, %3d) ",g_auiRasterToZscan[i], pCtu->getDepth(g_auiRasterToZscan[i]));
          iCount++;
        }
      }
      // printf("\n---------------CU ends--------------\n");
      // printf("\n---------------BS Value--------------\n");
      iCount = 0;
#endif //~SIG_PPU_OLD

      if (g_sigpool.p_ppu_st)
      {
        // reset ppu_st for ppu_param_ctx
        memset(g_sigpool.p_ppu_st->blk8_info, 0, sizeof(g_sigpool.p_ppu_st->blk8_info));
        memset(g_sigpool.p_ppu_st->blk8_pq, 0, sizeof(g_sigpool.p_ppu_st->blk8_pq));
        memset(g_sigpool.p_ppu_st->blk8_pq_c, 0, sizeof(g_sigpool.p_ppu_st->blk8_pq_c));
      }
    }
#endif //~SIG_PPU

    xDeblockCU( pCtu, 0, 0, EDGE_VER );

#ifdef SIG_PPU
    if (g_sigdump.ppu)
    {
      if (g_sigpool.p_ppu_st)
      {
        sigdump_output_bin(&g_sigpool.p_ppu_st->ppu_param_ctx, (unsigned char *)(&g_sigpool.p_ppu_st->blk8_info[0]),
                           sizeof(sig_ppu_blk8_info_st) * 64);

        if (g_sigdump.ppu_filter)
        {
          sig_ctx *p_ctx = &g_sigpool.p_ppu_st->ppu_filter_ctx;
          unsigned char header[3] = { 0 };
          unsigned char header_c[2] = { 0 };
          for (int id = 0; id < 64; id++)
          {
            header[0] = (unsigned char)(g_sigpool.p_ppu_st->blk8_info[id].bs);
            header[1] = (unsigned char)(g_sigpool.p_ppu_st->blk8_info[id].tc);
            header[2] = (unsigned char)(g_sigpool.p_ppu_st->blk8_info[id].beta);
            header_c[0] = g_sigpool.p_ppu_st->blk8_info[id].chroma_tc[0];
            header_c[1] = g_sigpool.p_ppu_st->blk8_info[id].chroma_tc[1];

            for (int part = 0; part < 2; part++)
            {
              sigdump_output_bin(p_ctx, header, sizeof(header));

              unsigned char *p_out = &(g_sigpool.p_ppu_st->blk8_pq[id][part][0][0][0][0]);
              sigdump_output_bin(p_ctx, p_out, 64);

              for (int ch = 0; ch < 2; ch++)
              {
                sigdump_output_bin(p_ctx, &header_c[ch], 1);
                p_out = &(g_sigpool.p_ppu_st->blk8_pq_c[ch][id][part][0][0][0][0]);
                sigdump_output_bin(p_ctx, p_out, 16);
              }
            }
          }
        }
      }

#ifdef SIG_PPU_OLD
      // for (int i = 0; i < pCtu->getTotalNumPart(); i++)
      // {
      //   if ( (iCount & (iWidthInPart - 1)) == 0) {
      //     printf("\n");
      //     printf("(%3d, %3d) ",g_auiRasterToZscan[i], m_aapucBS[EDGE_VER][g_auiRasterToZscan[i]]);
      //     iCount++;
      //   }
      // }
      // printf("\n---------------BS ends--------------\n");
      for (int i = 0; i < pCtu->getTotalNumPart(); i++)
      {
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[3],"ctu_index=%d,z_index=%d,v_bs=%d\n",pCtu->getCtuRsAddr(), g_auiRasterToZscan[i], m_aapucBS[EDGE_VER][g_auiRasterToZscan[i]]        );
      }
#endif //~SIG_PPU_OLD
    }
#endif //~SIG_PPU
  }

  // Vertical filtering
  for ( UInt ctuRsAddr = 0; ctuRsAddr < pcPic->getNumberOfCtusInFrame(); ctuRsAddr++ )
  {
    TComDataCU* pCtu = pcPic->getCtu( ctuRsAddr );

    ::memset( m_aapucBS       [EDGE_HOR], 0, sizeof( UChar ) * m_uiNumPartitions );
    ::memset( m_aapbEdgeFilter[EDGE_HOR], 0, sizeof( Bool  ) * m_uiNumPartitions );

#ifdef SIG_PPU
    if (g_sigdump.ppu && g_sigpool.p_ppu_st)
    {
      memset(g_sigpool.p_ppu_st->blk8_info, 0, sizeof(g_sigpool.p_ppu_st->blk8_info));
      memset(g_sigpool.p_ppu_st->blk8_pq, 0, sizeof(g_sigpool.p_ppu_st->blk8_pq));
      memset(g_sigpool.p_ppu_st->blk8_pq_c, 0, sizeof(g_sigpool.p_ppu_st->blk8_pq_c));
    }
#endif

    // CU-based deblocking
    xDeblockCU( pCtu, 0, 0, EDGE_HOR );

#ifdef SIG_PPU
    if (g_sigdump.ppu)
    {
      if (g_sigpool.p_ppu_st)
      {
        sigdump_output_bin(&g_sigpool.p_ppu_st->ppu_param_ctx, (unsigned char *)(&g_sigpool.p_ppu_st->blk8_info[0]),
                           sizeof(sig_ppu_blk8_info_st) * 64);

        int rast_idx = 240;  // CTU64 bottom-left part index in raster scan order.
        unsigned char data[3];
        sig_ctx *p_ctx = &g_sigpool.p_ppu_st->ppu_neb_ctx;

        for (int i = 0; i < 16; i += 2)
        {
          int abs_idx = g_auiRasterToZscan[rast_idx + i];
          int cu_size = pCtu->getWidth(abs_idx);
          PredMode mode = pCtu->getPredictionMode(abs_idx);
          int tr_depth = 0;

          if (cu_size == 32 && mode == MODE_INTER)
          {
            bool is_skip = pCtu->getSkipFlag(abs_idx);
            if (is_skip == false)
              tr_depth = 1;
          }

          data[0] = pCtu->getQP(abs_idx);
          data[1] = mode;
          data[2] = pCtu->getCbf(abs_idx, COMPONENT_Y, tr_depth);
          sigdump_output_bin(p_ctx, data, sizeof(data));
        }

        if (g_sigdump.ppu_filter)
        {
          p_ctx = &g_sigpool.p_ppu_st->ppu_filter_ctx;
          unsigned char header[3] = { 0 };
          unsigned char header_c[2] = { 0 };
          for (int id = 0; id < 64; id++)
          {
            header[0] = (unsigned char)(g_sigpool.p_ppu_st->blk8_info[id].bs);
            header[1] = (unsigned char)(g_sigpool.p_ppu_st->blk8_info[id].tc);
            header[2] = (unsigned char)(g_sigpool.p_ppu_st->blk8_info[id].beta);
            header_c[0] = g_sigpool.p_ppu_st->blk8_info[id].chroma_tc[0];
            header_c[1] = g_sigpool.p_ppu_st->blk8_info[id].chroma_tc[1];

            for (int part = 0; part < 2; part++)
            {
              sigdump_output_bin(p_ctx, header, sizeof(header));

              unsigned char *p_out = &(g_sigpool.p_ppu_st->blk8_pq[id][part][0][0][0][0]);
              sigdump_output_bin(p_ctx, p_out, 64);

              for (int ch = 0; ch < 2; ch++)
              {
                sigdump_output_bin(p_ctx, &header_c[ch], 1);
                p_out = &(g_sigpool.p_ppu_st->blk8_pq_c[ch][id][part][0][0][0][0]);
                sigdump_output_bin(p_ctx, p_out, 16);
              }
            }
          }
        }
      }

#ifdef SIG_PPU_OLD
      int iWidthInPart = (pCtu->getSlice()->getSPS()->getMaxCUWidth())>>2;
      int iCount = 0;
      // printf("\n---------------BS start--------------\n");
      for (int i = 0; i < pCtu->getTotalNumPart(); i++)
      {
        if ( (iCount & (iWidthInPart - 1)) == 0)
        {
          // printf("\n");
          // printf("(%3d, %3d) ",g_auiRasterToZscan[i], m_aapucBS[EDGE_HOR][g_auiRasterToZscan[i]]);
          iCount++;
        }
      }
      // printf("\n---------------BS ends--------------\n");
      for (int i = 0; i < pCtu->getTotalNumPart(); i++)
      {
        sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[4],"ctu_index=%d,z_index=%d,h_bs=%d\n",pCtu->getCtuRsAddr(), g_auiRasterToZscan[i], m_aapucBS[EDGE_HOR][g_auiRasterToZscan[i]]        );
      }
#endif //~SIG_PPU_OLD
    }
#endif //~SIG_PPU
  }
}


// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 Deblocking filter process in CU-based (the same function as conventional's)

 \param pcCU             Pointer to CTU/CU structure
 \param uiAbsZorderIdx   Position in CU
 \param uiDepth          Depth in CU
 \param edgeDir          the direction of the edge in block boundary (horizontal/vertical), which is added newly
*/
Void TComLoopFilter::xDeblockCU( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, DeblockEdgeDir edgeDir )
{
  if(pcCU->getPic()==0||pcCU->getPartitionSize(uiAbsZorderIdx)==NUMBER_OF_PART_SIZES)
  {
    return;
  }
  TComPic* pcPic     = pcCU->getPic();
  UInt uiCurNumParts = pcPic->getNumPartitionsInCtu() >> (uiDepth<<1);
  UInt uiQNumParts   = uiCurNumParts>>2;
  const TComSPS &sps = *(pcCU->getSlice()->getSPS());

  if( pcCU->getDepth(uiAbsZorderIdx) > uiDepth )
  {
    for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++, uiAbsZorderIdx+=uiQNumParts )
    {
      UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsZorderIdx] ];
      UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsZorderIdx] ];
      if( ( uiLPelX < sps.getPicWidthInLumaSamples() ) && ( uiTPelY < sps.getPicHeightInLumaSamples() ) )
      {
        xDeblockCU( pcCU, uiAbsZorderIdx, uiDepth+1, edgeDir );
      }
    }
    return;
  }
#ifdef SIG_PPU_OLD
  sigdump_output_fprint(&g_sigpool.ppu_frm_ctx[1],"start set CU filter: ctu_number=%d, z_index=%d, partitionnum=%d, edge=%d\n",pcCU->getCtuRsAddr(),uiAbsZorderIdx, uiCurNumParts, edgeDir );
#endif
  xSetLoopfilterParam( pcCU, uiAbsZorderIdx );
  TComTURecurse tuRecurse(pcCU, uiAbsZorderIdx);
  xSetEdgefilterTU   ( tuRecurse );
  xSetEdgefilterPU   ( pcCU, uiAbsZorderIdx );

  const UInt uiPelsInPart = sps.getMaxCUWidth() >> sps.getMaxTotalCUDepth();

  for( UInt uiPartIdx = uiAbsZorderIdx; uiPartIdx < uiAbsZorderIdx + uiCurNumParts; uiPartIdx++ )
  {
    UInt uiBSCheck;
    if( uiPelsInPart == 4 )
    {
      uiBSCheck = (edgeDir == EDGE_VER && uiPartIdx%2 == 0) || (edgeDir == EDGE_HOR && (uiPartIdx-((uiPartIdx>>2)<<2))/2 == 0);
    }
    else
    {
      uiBSCheck = 1;
    }

    if ( m_aapbEdgeFilter[edgeDir][uiPartIdx] && uiBSCheck )
    {
      xGetBoundaryStrengthSingle ( pcCU, edgeDir, uiPartIdx );
    }
  }

  UInt PartIdxIncr = DEBLOCK_SMALLEST_BLOCK / uiPelsInPart ? DEBLOCK_SMALLEST_BLOCK / uiPelsInPart : 1 ;

  UInt uiSizeInPU = pcPic->getNumPartInCtuWidth()>>(uiDepth);
  const ChromaFormat chFmt=pcPic->getChromaFormat();
  const UInt shiftFactor  = edgeDir == EDGE_VER ? pcPic->getComponentScaleX(COMPONENT_Cb) : pcPic->getComponentScaleY(COMPONENT_Cb);
  const Bool bAlwaysDoChroma=chFmt==CHROMA_444;

  for ( Int iEdge = 0; iEdge < uiSizeInPU ; iEdge+=PartIdxIncr)
  {
    xEdgeFilterLuma     ( pcCU, uiAbsZorderIdx, uiDepth, edgeDir, iEdge );
    if ( chFmt!=CHROMA_400 && (bAlwaysDoChroma ||
                               (uiPelsInPart>DEBLOCK_SMALLEST_BLOCK) ||
                               (iEdge % ( (DEBLOCK_SMALLEST_BLOCK<<shiftFactor)/uiPelsInPart ) ) == 0
                              )
       )
    {
      xEdgeFilterChroma   ( pcCU, uiAbsZorderIdx, uiDepth, edgeDir, iEdge );
    }
  }
}

Void TComLoopFilter::xSetEdgefilterMultiple( TComDataCU*    pcCU,
                                             UInt           uiAbsZorderIdx,
                                             UInt           uiDepth,
                                             DeblockEdgeDir edgeDir,
                                             Int            iEdgeIdx,
                                             Bool           bValue,
                                             UInt           uiWidthInBaseUnits,
                                             UInt           uiHeightInBaseUnits,
                                             const TComRectangle *rect)
{
  if ( uiWidthInBaseUnits == 0 )
  {
    uiWidthInBaseUnits  = pcCU->getPic()->getNumPartInCtuWidth () >> uiDepth;
  }
  if ( uiHeightInBaseUnits == 0 )
  {
    uiHeightInBaseUnits = pcCU->getPic()->getNumPartInCtuHeight() >> uiDepth;
  }
  const UInt uiNumElem = edgeDir == EDGE_VER ? uiHeightInBaseUnits : uiWidthInBaseUnits;
  assert( uiNumElem > 0 );
  assert( uiWidthInBaseUnits > 0 );
  assert( uiHeightInBaseUnits > 0 );
  for( UInt ui = 0; ui < uiNumElem; ui++ )
  {
    const UInt uiBsIdx = xCalcBsIdx( pcCU, uiAbsZorderIdx, edgeDir, iEdgeIdx, ui, rect );
    m_aapbEdgeFilter[edgeDir][uiBsIdx] = bValue;
    if (iEdgeIdx == 0)
    {
      m_aapucBS[edgeDir][uiBsIdx] = bValue;
    }
  }
}

Void TComLoopFilter::xSetEdgefilterTU(  TComTU &rTu )
{
  TComDataCU* pcCU  = rTu.getCU();
  UInt uiTransDepthTotal = rTu.GetTransformDepthTotal();

  if( pcCU->getTransformIdx( rTu.GetAbsPartIdxTU() ) + pcCU->getDepth( rTu.GetAbsPartIdxTU()) > uiTransDepthTotal )
  {
    TComTURecurse tuChild(rTu, false);
    do
    {
      xSetEdgefilterTU( tuChild );
    } while (tuChild.nextSection(rTu));
    return;
  }

  const TComRectangle &rect = rTu.getRect(COMPONENT_Y);
  const TComSPS &sps=*(pcCU->getSlice()->getSPS());

  const UInt uiWidthInBaseUnits  = rect.width  / (sps.getMaxCUWidth()  >> sps.getMaxTotalCUDepth());
  const UInt uiHeightInBaseUnits = rect.height / (sps.getMaxCUHeight() >> sps.getMaxTotalCUDepth());

  xSetEdgefilterMultiple( pcCU, rTu.GetAbsPartIdxCU(), uiTransDepthTotal, EDGE_VER, 0, m_stLFCUParam.bInternalEdge, uiWidthInBaseUnits, uiHeightInBaseUnits, &rect );
  xSetEdgefilterMultiple( pcCU, rTu.GetAbsPartIdxCU(), uiTransDepthTotal, EDGE_HOR, 0, m_stLFCUParam.bInternalEdge, uiWidthInBaseUnits, uiHeightInBaseUnits, &rect );
}

Void TComLoopFilter::xSetEdgefilterPU( TComDataCU* pcCU, UInt uiAbsZorderIdx )
{
  const UInt uiDepth = pcCU->getDepth( uiAbsZorderIdx );
  const UInt uiWidthInBaseUnits  = pcCU->getPic()->getNumPartInCtuWidth () >> uiDepth;
  const UInt uiHeightInBaseUnits = pcCU->getPic()->getNumPartInCtuHeight() >> uiDepth;
  const UInt uiHWidthInBaseUnits  = uiWidthInBaseUnits  >> 1;
  const UInt uiHHeightInBaseUnits = uiHeightInBaseUnits >> 1;
  const UInt uiQWidthInBaseUnits  = uiWidthInBaseUnits  >> 2;
  const UInt uiQHeightInBaseUnits = uiHeightInBaseUnits >> 2;

  xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_VER, 0, m_stLFCUParam.bLeftEdge );
  xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_HOR, 0, m_stLFCUParam.bTopEdge );

  switch ( pcCU->getPartitionSize( uiAbsZorderIdx ) )
  {
    case SIZE_2Nx2N:
    {
      break;
    }
    case SIZE_2NxN:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_HOR, uiHHeightInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
    case SIZE_Nx2N:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_VER, uiHWidthInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
    case SIZE_NxN:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_VER, uiHWidthInBaseUnits, m_stLFCUParam.bInternalEdge );
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_HOR, uiHHeightInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
    case SIZE_2NxnU:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_HOR, uiQHeightInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
    case SIZE_2NxnD:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_HOR, uiHeightInBaseUnits - uiQHeightInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
    case SIZE_nLx2N:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_VER, uiQWidthInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
    case SIZE_nRx2N:
    {
      xSetEdgefilterMultiple( pcCU, uiAbsZorderIdx, uiDepth, EDGE_VER, uiWidthInBaseUnits - uiQWidthInBaseUnits, m_stLFCUParam.bInternalEdge );
      break;
    }
    default:
    {
      break;
    }
  }
}


Void TComLoopFilter::xSetLoopfilterParam( TComDataCU* pcCU, UInt uiAbsZorderIdx )
{
  UInt uiX           = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[ uiAbsZorderIdx ] ];
  UInt uiY           = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[ uiAbsZorderIdx ] ];

  UInt        uiTempPartIdx;

  m_stLFCUParam.bInternalEdge = ! pcCU->getSlice()->getDeblockingFilterDisable();

  if ( (uiX == 0) || pcCU->getSlice()->getDeblockingFilterDisable() )
  {
    m_stLFCUParam.bLeftEdge = false;
  }
  else
  {
    m_stLFCUParam.bLeftEdge = true;
  }
  if ( m_stLFCUParam.bLeftEdge )
  {
    const TComDataCU* pcTempCU = pcCU->getPULeft( uiTempPartIdx, uiAbsZorderIdx, !pcCU->getSlice()->getLFCrossSliceBoundaryFlag(), !m_bLFCrossTileBoundary);

    if ( pcTempCU != NULL )
    {
      m_stLFCUParam.bLeftEdge = true;
    }
    else
    {
      m_stLFCUParam.bLeftEdge = false;
    }
  }

  if ( (uiY == 0 ) || pcCU->getSlice()->getDeblockingFilterDisable() )
  {
    m_stLFCUParam.bTopEdge = false;
  }
  else
  {
    m_stLFCUParam.bTopEdge = true;
  }
  if ( m_stLFCUParam.bTopEdge )
  {
    const TComDataCU* pcTempCU = pcCU->getPUAbove( uiTempPartIdx, uiAbsZorderIdx, !pcCU->getSlice()->getLFCrossSliceBoundaryFlag(), false, !m_bLFCrossTileBoundary);

    if ( pcTempCU != NULL )
    {
      m_stLFCUParam.bTopEdge = true;
    }
    else
    {
      m_stLFCUParam.bTopEdge = false;
    }
  }
}

Void TComLoopFilter::xGetBoundaryStrengthSingle ( TComDataCU* pCtu, DeblockEdgeDir edgeDir, UInt uiAbsPartIdx4x4BlockWithinCtu )
{
  TComSlice * const pcSlice = pCtu->getSlice();

  const Bool lfCrossSliceBoundaryFlag=pCtu->getSlice()->getLFCrossSliceBoundaryFlag();

  const UInt uiPartQ = uiAbsPartIdx4x4BlockWithinCtu;
  TComDataCU* const pcCUQ = pCtu;

  UInt uiPartP;
  const TComDataCU* pcCUP;
  UInt uiBs = 0;

  //-- Calculate Block Index
  if (edgeDir == EDGE_VER)
  {
    pcCUP = pcCUQ->getPULeft(uiPartP, uiPartQ, !lfCrossSliceBoundaryFlag, !m_bLFCrossTileBoundary);
  }
  else  // (edgeDir == EDGE_HOR)
  {
    pcCUP = pcCUQ->getPUAbove(uiPartP, uiPartQ, !pCtu->getSlice()->getLFCrossSliceBoundaryFlag(), false, !m_bLFCrossTileBoundary);
  }

  //-- Set BS for Intra MB : BS = 4 or 3
  if ( pcCUP->isIntra(uiPartP) || pcCUQ->isIntra(uiPartQ) )
  {
    uiBs = 2;
  }

  //-- Set BS for not Intra MB : BS = 2 or 1 or 0
  if ( !pcCUP->isIntra(uiPartP) && !pcCUQ->isIntra(uiPartQ) )
  {
    UInt nsPartQ = uiPartQ;
    UInt nsPartP = uiPartP;

    if ( m_aapucBS[edgeDir][uiAbsPartIdx4x4BlockWithinCtu] && (pcCUQ->getCbf( nsPartQ, COMPONENT_Y, pcCUQ->getTransformIdx(nsPartQ)) != 0 || pcCUP->getCbf( nsPartP, COMPONENT_Y, pcCUP->getTransformIdx(nsPartP) ) != 0) )
    {
      uiBs = 1;
    }
    else
    {
      if (pcSlice->isInterB() || pcCUP->getSlice()->isInterB())
      {
        Int iRefIdx;
        iRefIdx = pcCUP->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartP);
        const TComPic *piRefP0 = (iRefIdx < 0) ? NULL : pcCUP->getSlice()->getRefPic(REF_PIC_LIST_0, iRefIdx);
        iRefIdx = pcCUP->getCUMvField(REF_PIC_LIST_1)->getRefIdx(uiPartP);
        const TComPic *piRefP1 = (iRefIdx < 0) ? NULL : pcCUP->getSlice()->getRefPic(REF_PIC_LIST_1, iRefIdx);
        iRefIdx = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartQ);
        const TComPic *piRefQ0 = (iRefIdx < 0) ? NULL : pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx);
        iRefIdx = pcCUQ->getCUMvField(REF_PIC_LIST_1)->getRefIdx(uiPartQ);
        const TComPic *piRefQ1 = (iRefIdx < 0) ? NULL : pcSlice->getRefPic(REF_PIC_LIST_1, iRefIdx);

        TComMv pcMvP0 = pcCUP->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartP);
        TComMv pcMvP1 = pcCUP->getCUMvField(REF_PIC_LIST_1)->getMv(uiPartP);
        TComMv pcMvQ0 = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartQ);
        TComMv pcMvQ1 = pcCUQ->getCUMvField(REF_PIC_LIST_1)->getMv(uiPartQ);

        if (piRefP0 == NULL)
        {
          pcMvP0.setZero();
        }
        if (piRefP1 == NULL)
        {
          pcMvP1.setZero();
        }
        if (piRefQ0 == NULL)
        {
          pcMvQ0.setZero();
        }
        if (piRefQ1 == NULL)
        {
          pcMvQ1.setZero();
        }

        if ( ((piRefP0==piRefQ0)&&(piRefP1==piRefQ1)) || ((piRefP0==piRefQ1)&&(piRefP1==piRefQ0)) )
        {
          if ( piRefP0 != piRefP1 )   // Different L0 & L1
          {
            if ( piRefP0 == piRefQ0 )
            {
              uiBs  = ((abs(pcMvQ0.getHor() - pcMvP0.getHor()) >= 4) ||
                       (abs(pcMvQ0.getVer() - pcMvP0.getVer()) >= 4) ||
                       (abs(pcMvQ1.getHor() - pcMvP1.getHor()) >= 4) ||
                       (abs(pcMvQ1.getVer() - pcMvP1.getVer()) >= 4)) ? 1 : 0;
            }
            else
            {
              uiBs  = ((abs(pcMvQ1.getHor() - pcMvP0.getHor()) >= 4) ||
                       (abs(pcMvQ1.getVer() - pcMvP0.getVer()) >= 4) ||
                       (abs(pcMvQ0.getHor() - pcMvP1.getHor()) >= 4) ||
                       (abs(pcMvQ0.getVer() - pcMvP1.getVer()) >= 4)) ? 1 : 0;
            }
          }
          else    // Same L0 & L1
          {
            uiBs  = ((abs(pcMvQ0.getHor() - pcMvP0.getHor()) >= 4) ||
                     (abs(pcMvQ0.getVer() - pcMvP0.getVer()) >= 4) ||
                     (abs(pcMvQ1.getHor() - pcMvP1.getHor()) >= 4) ||
                     (abs(pcMvQ1.getVer() - pcMvP1.getVer()) >= 4)) &&
                    ((abs(pcMvQ1.getHor() - pcMvP0.getHor()) >= 4) ||
                     (abs(pcMvQ1.getVer() - pcMvP0.getVer()) >= 4) ||
                     (abs(pcMvQ0.getHor() - pcMvP1.getHor()) >= 4) ||
                     (abs(pcMvQ0.getVer() - pcMvP1.getVer()) >= 4)) ? 1 : 0;
          }
        }
        else // for all different Ref_Idx
        {
          uiBs = 1;
        }
      }
      else  // pcSlice->isInterP()
      {
        Int iRefIdx;
        iRefIdx = pcCUP->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartP);
        const TComPic *piRefP0 = (iRefIdx < 0) ? NULL : pcCUP->getSlice()->getRefPic(REF_PIC_LIST_0, iRefIdx);
        iRefIdx = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getRefIdx(uiPartQ);
        const TComPic *piRefQ0 = (iRefIdx < 0) ? NULL : pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx);
        TComMv pcMvP0 = pcCUP->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartP);
        TComMv pcMvQ0 = pcCUQ->getCUMvField(REF_PIC_LIST_0)->getMv(uiPartQ);

        if (piRefP0 == NULL)
        {
          pcMvP0.setZero();
        }
        if (piRefQ0 == NULL)
        {
          pcMvQ0.setZero();
        }

        uiBs  = ((piRefP0 != piRefQ0) ||
                 (abs(pcMvQ0.getHor() - pcMvP0.getHor()) >= 4) ||
                 (abs(pcMvQ0.getVer() - pcMvP0.getVer()) >= 4)) ? 1 : 0;
      }
    }   // enf of "if( one of BCBP == 0 )"
  }   // enf of "if( not Intra )"

  m_aapucBS[edgeDir][uiAbsPartIdx4x4BlockWithinCtu] = uiBs;
}


Void TComLoopFilter::xEdgeFilterLuma( TComDataCU* const pcCU, const UInt uiAbsZorderIdx, const UInt uiDepth, const DeblockEdgeDir edgeDir, const Int iEdge  )
{
        TComPicYuv *pcPicYuvRec                    = pcCU->getPic()->getPicYuvRec();
        Pel        *piSrc                          = pcPicYuvRec->getAddr(COMPONENT_Y, pcCU->getCtuRsAddr(), uiAbsZorderIdx );
        Pel        *piTmpSrc                       = piSrc;
  const TComSPS    &sps                            = *(pcCU->getSlice()->getSPS());
  const Bool        ppsTransquantBypassEnabledFlag = pcCU->getSlice()->getPPS()->getTransquantBypassEnabledFlag();
  const Int         bitDepthLuma                   = sps.getBitDepth(CHANNEL_TYPE_LUMA);
  const Bool        lfCrossSliceBoundaryFlag       = pcCU->getSlice()->getLFCrossSliceBoundaryFlag();

  Int  iStride = pcPicYuvRec->getStride(COMPONENT_Y);
  Int iQP = 0;
  Int iQP_P = 0;
  Int iQP_Q = 0;
  UInt uiNumParts = pcCU->getPic()->getNumPartInCtuWidth()>>uiDepth;

  UInt  uiPelsInPart = sps.getMaxCUWidth() >> sps.getMaxTotalCUDepth();
  UInt  uiBsAbsIdx = 0, uiBs = 0;
  Int   iOffset, iSrcStep;

  Bool  bPCMFilter = (sps.getUsePCM() && sps.getPCMFilterDisableFlag())? true : false;
  Bool  bPartPNoFilter = false;
  Bool  bPartQNoFilter = false;
  UInt  uiPartPIdx = 0;
  UInt  uiPartQIdx = 0;
  const TComDataCU* pcCUP = pcCU;
  TComDataCU* pcCUQ = pcCU;
  Int  betaOffsetDiv2 = pcCUQ->getSlice()->getDeblockingFilterBetaOffsetDiv2();
  Int  tcOffsetDiv2 = pcCUQ->getSlice()->getDeblockingFilterTcOffsetDiv2();

  if (edgeDir == EDGE_VER)
  {
    iOffset = 1;
    iSrcStep = iStride;
    piTmpSrc += iEdge*uiPelsInPart;
  }
  else  // (edgeDir == EDGE_HOR)
  {
    iOffset = iStride;
    iSrcStep = 1;
    piTmpSrc += iEdge*uiPelsInPart*iStride;
  }

  const Int iBitdepthScale = 1 << (bitDepthLuma-8);

  for ( UInt iIdx = 0; iIdx < uiNumParts; iIdx++ )
  {
    uiBsAbsIdx = xCalcBsIdx( pcCU, uiAbsZorderIdx, edgeDir, iEdge, iIdx);
    uiBs = m_aapucBS[edgeDir][uiBsAbsIdx];

#ifdef SIG_PPU
    bool is_rec_ppu_bs = false;
    int ppu_idx = 0;
    int part_idx = iIdx % 2;
    if (g_sigdump.ppu)
    {
      int x = g_auiRasterToPelX[ g_auiZscanToRaster[uiBsAbsIdx] ];
      int y = g_auiRasterToPelY[ g_auiZscanToRaster[uiBsAbsIdx] ];
      is_rec_ppu_bs = ((x & 0x4) == 0 && (y & 0x4) == 0);
      ppu_idx = (x / 8) + (y / 8) * 8;

      if (is_rec_ppu_bs)
      {
        g_sigpool.p_ppu_st->blk8_info[ppu_idx].x = pcCU->getCUPelX() + x;
        g_sigpool.p_ppu_st->blk8_info[ppu_idx].y = pcCU->getCUPelY() + y;
        g_sigpool.p_ppu_st->blk8_info[ppu_idx].bs = uiBs;
      }
    }
#endif //~SIG_PPU

    if ( uiBs )
    {
      iQP_Q = pcCU->getQP( uiBsAbsIdx );
      uiPartQIdx = uiBsAbsIdx;
      // Derive neighboring PU index
      if (edgeDir == EDGE_VER)
      {
        pcCUP = pcCUQ->getPULeft (uiPartPIdx, uiPartQIdx,!lfCrossSliceBoundaryFlag, !m_bLFCrossTileBoundary);
      }
      else  // (iDir == EDGE_HOR)
      {
        pcCUP = pcCUQ->getPUAbove(uiPartPIdx, uiPartQIdx,!lfCrossSliceBoundaryFlag, false, !m_bLFCrossTileBoundary);
      }

      iQP_P = pcCUP->getQP(uiPartPIdx);
      iQP = (iQP_P + iQP_Q + 1) >> 1;

      Int iIndexTC = Clip3(0, MAX_QP+DEFAULT_INTRA_TC_OFFSET, Int(iQP + DEFAULT_INTRA_TC_OFFSET*(uiBs-1) + (tcOffsetDiv2 << 1)));
      Int iIndexB = Clip3(0, MAX_QP, iQP + (betaOffsetDiv2 << 1));

      Int iTc =  sm_tcTable[iIndexTC]*iBitdepthScale;
      Int iBeta = sm_betaTable[iIndexB]*iBitdepthScale;
      Int iSideThreshold = (iBeta+(iBeta>>1))>>3;
      Int iThrCut = iTc*10;

#ifdef SIG_PPU
      if (g_sigdump.ppu)
      {
        if (is_rec_ppu_bs)
        {
          g_sigpool.p_ppu_st->blk8_info[ppu_idx].beta = iBeta;
          g_sigpool.p_ppu_st->blk8_info[ppu_idx].tc = iTc;
        }

        for (int i = 0; i < 4; i++)
        {
          Pel *p_src = piTmpSrc + iSrcStep * (iIdx * uiPelsInPart + i);

          // source pq
          for (int pq = 0; pq < 4; pq++)
          {
            g_sigpool.p_ppu_st->blk8_pq[ppu_idx][part_idx][0][1][i][pq] = (unsigned char)(p_src[pq * iOffset]);
            g_sigpool.p_ppu_st->blk8_pq[ppu_idx][part_idx][0][0][i][pq] = (unsigned char)(p_src[(-1 - pq) * iOffset]);
          }
        }
      }
#endif //~SIG_PPU

      UInt  uiBlocksInPart = uiPelsInPart / 4 ? uiPelsInPart / 4 : 1;
      for (UInt iBlkIdx = 0; iBlkIdx<uiBlocksInPart; iBlkIdx ++)
      {
        Int dp0 = xCalcDP( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*4+0), iOffset);
        Int dq0 = xCalcDQ( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*4+0), iOffset);
        Int dp3 = xCalcDP( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*4+3), iOffset);
        Int dq3 = xCalcDQ( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*4+3), iOffset);
        Int d0 = dp0 + dq0;
        Int d3 = dp3 + dq3;

        Int dp = dp0 + dp3;
        Int dq = dq0 + dq3;
        Int d =  d0 + d3;

        if (bPCMFilter || ppsTransquantBypassEnabledFlag)
        {
          // Check if each of PUs is I_PCM with LF disabling
          bPartPNoFilter = (bPCMFilter && pcCUP->getIPCMFlag(uiPartPIdx));
          bPartQNoFilter = (bPCMFilter && pcCUQ->getIPCMFlag(uiPartQIdx));

          // check if each of PUs is lossless coded
          bPartPNoFilter = bPartPNoFilter || (pcCUP->isLosslessCoded(uiPartPIdx) );
          bPartQNoFilter = bPartQNoFilter || (pcCUQ->isLosslessCoded(uiPartQIdx) );
        }

        if (d < iBeta)
        {
          Bool bFilterP = (dp < iSideThreshold);
          Bool bFilterQ = (dq < iSideThreshold);

          Bool sw =  xUseStrongFiltering( iOffset, 2*d0, iBeta, iTc, piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*4+0))
          && xUseStrongFiltering( iOffset, 2*d3, iBeta, iTc, piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*4+3));

          for ( Int i = 0; i < DEBLOCK_SMALLEST_BLOCK/2; i++)
          {
            xPelFilterLuma( piTmpSrc+iSrcStep*(iIdx*uiPelsInPart+iBlkIdx*4+i), iOffset, iTc, sw, bPartPNoFilter, bPartQNoFilter, iThrCut, bFilterP, bFilterQ, bitDepthLuma);
          }
        }
      }
#ifdef SIG_PPU
      if (g_sigdump.ppu)
      {
        for (int i = 0; i < 4; i++)
        {
          Pel *p_src = piTmpSrc + iSrcStep * (iIdx * uiPelsInPart + i);

          // filtered pq
          for (int pq = 0; pq < 4; pq++)
          {
            g_sigpool.p_ppu_st->blk8_pq[ppu_idx][part_idx][1][1][i][pq] = (unsigned char)(p_src[pq * iOffset]);
            g_sigpool.p_ppu_st->blk8_pq[ppu_idx][part_idx][1][0][i][pq] = (unsigned char)(p_src[(-1 - pq) * iOffset]);
          }
        }
      }
#endif //~SIG_PPU
    }
  }
}


Void TComLoopFilter::xEdgeFilterChroma( TComDataCU* const pcCU, const UInt uiAbsZorderIdx, const UInt uiDepth, const DeblockEdgeDir edgeDir, const Int iEdge )
{
        TComPicYuv *pcPicYuvRec    = pcCU->getPic()->getPicYuvRec();
        Int         iStride        = pcPicYuvRec->getStride(COMPONENT_Cb);
        Pel        *piSrcCb        = pcPicYuvRec->getAddr( COMPONENT_Cb, pcCU->getCtuRsAddr(), uiAbsZorderIdx );
        Pel        *piSrcCr        = pcPicYuvRec->getAddr( COMPONENT_Cr, pcCU->getCtuRsAddr(), uiAbsZorderIdx );
  const TComSPS    &sps            = *(pcCU->getSlice()->getSPS());
  const Int         bitDepthChroma = sps.getBitDepth(CHANNEL_TYPE_CHROMA);

  const UInt  uiPelsInPartChromaH = sps.getMaxCUWidth() >> (sps.getMaxTotalCUDepth()+pcPicYuvRec->getComponentScaleX(COMPONENT_Cb));
  const UInt  uiPelsInPartChromaV = sps.getMaxCUHeight() >> (sps.getMaxTotalCUDepth()+pcPicYuvRec->getComponentScaleY(COMPONENT_Cb));

  Int iQP = 0;
  Int iQP_P = 0;
  Int iQP_Q = 0;

  Int   iOffset, iSrcStep;
  UInt  uiLoopLength;

  const UInt uiCtuWidthInBaseUnits = pcCU->getPic()->getNumPartInCtuWidth();

  Bool  bPCMFilter = (pcCU->getSlice()->getSPS()->getUsePCM() && pcCU->getSlice()->getSPS()->getPCMFilterDisableFlag())? true : false;
  Bool  bPartPNoFilter = false;
  Bool  bPartQNoFilter = false;
  TComDataCU* pcCUQ = pcCU;
  Int tcOffsetDiv2 = pcCU->getSlice()->getDeblockingFilterTcOffsetDiv2();

  // Vertical Position
  UInt uiEdgeNumInCtuVert = g_auiZscanToRaster[uiAbsZorderIdx]%uiCtuWidthInBaseUnits + iEdge;
  UInt uiEdgeNumInCtuHor = g_auiZscanToRaster[uiAbsZorderIdx]/uiCtuWidthInBaseUnits + iEdge;

  if ( (uiPelsInPartChromaH < DEBLOCK_SMALLEST_BLOCK) && (uiPelsInPartChromaV < DEBLOCK_SMALLEST_BLOCK) &&
       (
         ( (uiEdgeNumInCtuVert%(DEBLOCK_SMALLEST_BLOCK/uiPelsInPartChromaH)) && (edgeDir==EDGE_VER) ) ||
         ( (uiEdgeNumInCtuHor %(DEBLOCK_SMALLEST_BLOCK/uiPelsInPartChromaV)) && (edgeDir==EDGE_HOR) )
       )
     )
  {
    return;
  }


  const Bool lfCrossSliceBoundaryFlag=pcCU->getSlice()->getLFCrossSliceBoundaryFlag();

  UInt  uiNumParts = pcCU->getPic()->getNumPartInCtuWidth()>>uiDepth;

  UInt  uiBsAbsIdx;
  UChar ucBs;

  Pel* piTmpSrcCb = piSrcCb;
  Pel* piTmpSrcCr = piSrcCr;

  if (edgeDir == EDGE_VER)
  {
    iOffset   = 1;
    iSrcStep  = iStride;
    piTmpSrcCb += iEdge*uiPelsInPartChromaH;
    piTmpSrcCr += iEdge*uiPelsInPartChromaH;
    uiLoopLength=uiPelsInPartChromaV;
  }
  else  // (edgeDir == EDGE_HOR)
  {
    iOffset   = iStride;
    iSrcStep  = 1;
    piTmpSrcCb += iEdge*iStride*uiPelsInPartChromaV;
    piTmpSrcCr += iEdge*iStride*uiPelsInPartChromaV;
    uiLoopLength=uiPelsInPartChromaH;
  }

  const Int iBitdepthScale = 1 << (pcCU->getSlice()->getSPS()->getBitDepth(CHANNEL_TYPE_CHROMA)-8);

#ifdef SIG_PPU
  int ppu_idx = 0;
  int part_idx = 0;
#endif //~SIG_PPU

  for ( UInt iIdx = 0; iIdx < uiNumParts; iIdx++ )
  {
    uiBsAbsIdx = xCalcBsIdx( pcCU, uiAbsZorderIdx, edgeDir, iEdge, iIdx);
    ucBs = m_aapucBS[edgeDir][uiBsAbsIdx];

#ifdef SIG_PPU
    part_idx = (iIdx % 2);
    if (g_sigdump.ppu)
    {
      if (part_idx == 0)
      {
        int x = g_auiRasterToPelX[ g_auiZscanToRaster[uiBsAbsIdx] ];
        int y = g_auiRasterToPelY[ g_auiZscanToRaster[uiBsAbsIdx] ];
        ppu_idx = (x / 8) + (y / 8) * 8;
      }
    }
#endif //~SIG_PPU

    if ( ucBs > 1)
    {
      iQP_Q = pcCU->getQP( uiBsAbsIdx );
      UInt  uiPartQIdx = uiBsAbsIdx;
      // Derive neighboring PU index
      const TComDataCU* pcCUP;
      UInt  uiPartPIdx;

      if (edgeDir == EDGE_VER)
      {
        pcCUP = pcCUQ->getPULeft (uiPartPIdx, uiPartQIdx,!lfCrossSliceBoundaryFlag, !m_bLFCrossTileBoundary);
      }
      else  // (edgeDir == EDGE_HOR)
      {
        pcCUP = pcCUQ->getPUAbove(uiPartPIdx, uiPartQIdx,!lfCrossSliceBoundaryFlag, false, !m_bLFCrossTileBoundary);
      }

      iQP_P = pcCUP->getQP(uiPartPIdx);

      if (bPCMFilter || pcCU->getSlice()->getPPS()->getTransquantBypassEnabledFlag())
      {
        // Check if each of PUs is I_PCM with LF disabling
        bPartPNoFilter = (bPCMFilter && pcCUP->getIPCMFlag(uiPartPIdx));
        bPartQNoFilter = (bPCMFilter && pcCUQ->getIPCMFlag(uiPartQIdx));

        // check if each of PUs is lossless coded
        bPartPNoFilter = bPartPNoFilter || (pcCUP->isLosslessCoded(uiPartPIdx));
        bPartQNoFilter = bPartQNoFilter || (pcCUQ->isLosslessCoded(uiPartQIdx));
      }

      for ( UInt chromaIdx = 0; chromaIdx < 2; chromaIdx++ )
      {
        Int chromaQPOffset  = pcCU->getSlice()->getPPS()->getQpOffset(ComponentID(chromaIdx + 1));
        Pel* piTmpSrcChroma = (chromaIdx == 0) ? piTmpSrcCb : piTmpSrcCr;

        iQP = ((iQP_P + iQP_Q + 1) >> 1) + chromaQPOffset;
        if (iQP >= chromaQPMappingTableSize)
        {
          if (pcPicYuvRec->getChromaFormat()==CHROMA_420)
          {
            iQP -=6;
          }
          else if (iQP>51)
          {
            iQP=51;
          }
        }
        else if (iQP >= 0 )
        {
          iQP = getScaledChromaQP(iQP, pcPicYuvRec->getChromaFormat());
        }

        Int iIndexTC = Clip3(0, MAX_QP+DEFAULT_INTRA_TC_OFFSET, iQP + DEFAULT_INTRA_TC_OFFSET*(ucBs - 1) + (tcOffsetDiv2 << 1));
        Int iTc =  sm_tcTable[iIndexTC]*iBitdepthScale;

#ifdef SIG_PPU
        if (g_sigdump.ppu)
        {
          g_sigpool.p_ppu_st->blk8_info[ppu_idx].chroma_tc[chromaIdx] = iTc;

          for (int i = 0; i < 2; i++)
          {
            Pel *p_src = piTmpSrcChroma + iSrcStep * (i + iIdx * uiLoopLength);
            // source q
            g_sigpool.p_ppu_st->blk8_pq_c[chromaIdx][ppu_idx][part_idx][0][1][i][0] = (unsigned char)(p_src[0]);
            g_sigpool.p_ppu_st->blk8_pq_c[chromaIdx][ppu_idx][part_idx][0][1][i][1] = (unsigned char)(p_src[iOffset]);
            // source p
            g_sigpool.p_ppu_st->blk8_pq_c[chromaIdx][ppu_idx][part_idx][0][0][i][0] = (unsigned char)(p_src[-iOffset]);
            g_sigpool.p_ppu_st->blk8_pq_c[chromaIdx][ppu_idx][part_idx][0][0][i][1] = (unsigned char)(p_src[-(iOffset * 2)]);
          }
        }
#endif //~SIG_PPU

        for ( UInt uiStep = 0; uiStep < uiLoopLength; uiStep++ )
        {
          xPelFilterChroma( piTmpSrcChroma + iSrcStep*(uiStep+iIdx*uiLoopLength), iOffset, iTc , bPartPNoFilter, bPartQNoFilter, bitDepthChroma);
        }
#ifdef SIG_PPU
        if (g_sigdump.ppu)
        {
          for (int i = 0; i < 2; i++)
          {
            Pel *p_src = piTmpSrcChroma + iSrcStep * (i + iIdx * uiLoopLength);
            // filtered q
            g_sigpool.p_ppu_st->blk8_pq_c[chromaIdx][ppu_idx][part_idx][1][1][i][0] = (unsigned char)(p_src[0]);
            g_sigpool.p_ppu_st->blk8_pq_c[chromaIdx][ppu_idx][part_idx][1][1][i][1] = (unsigned char)(p_src[iOffset]);
            // filtered p
            g_sigpool.p_ppu_st->blk8_pq_c[chromaIdx][ppu_idx][part_idx][1][0][i][0] = (unsigned char)(p_src[-iOffset]);
            g_sigpool.p_ppu_st->blk8_pq_c[chromaIdx][ppu_idx][part_idx][1][0][i][1] = (unsigned char)(p_src[-(iOffset * 2)]);
          }
        }
#endif //~SIG_PPU
      }
    }
  }
}

/**
 - Deblocking for the luminance component with strong or weak filter
 .
 \param piSrc           pointer to picture data
 \param iOffset         offset value for picture data
 \param tc              tc value
 \param sw              decision strong/weak filter
 \param bPartPNoFilter  indicator to disable filtering on partP
 \param bPartQNoFilter  indicator to disable filtering on partQ
 \param iThrCut         threshold value for weak filter decision
 \param bFilterSecondP  decision weak filter/no filter for partP
 \param bFilterSecondQ  decision weak filter/no filter for partQ
 \param bitDepthLuma    luma bit depth
*/
__inline Void TComLoopFilter::xPelFilterLuma( Pel* piSrc, Int iOffset, Int tc, Bool sw, Bool bPartPNoFilter, Bool bPartQNoFilter, Int iThrCut, Bool bFilterSecondP, Bool bFilterSecondQ, const Int bitDepthLuma)
{
  Int delta;

  Pel m4  = piSrc[0];
  Pel m3  = piSrc[-iOffset];
  Pel m5  = piSrc[ iOffset];
  Pel m2  = piSrc[-iOffset*2];
  Pel m6  = piSrc[ iOffset*2];
  Pel m1  = piSrc[-iOffset*3];
  Pel m7  = piSrc[ iOffset*3];
  Pel m0  = piSrc[-iOffset*4];

  if (sw)
  {
    piSrc[-iOffset]   = Clip3(m3-2*tc, m3+2*tc, ((m1 + 2*m2 + 2*m3 + 2*m4 + m5 + 4) >> 3));
    piSrc[0]          = Clip3(m4-2*tc, m4+2*tc, ((m2 + 2*m3 + 2*m4 + 2*m5 + m6 + 4) >> 3));
    piSrc[-iOffset*2] = Clip3(m2-2*tc, m2+2*tc, ((m1 + m2 + m3 + m4 + 2)>>2));
    piSrc[ iOffset]   = Clip3(m5-2*tc, m5+2*tc, ((m3 + m4 + m5 + m6 + 2)>>2));
    piSrc[-iOffset*3] = Clip3(m1-2*tc, m1+2*tc, ((2*m0 + 3*m1 + m2 + m3 + m4 + 4 )>>3));
    piSrc[ iOffset*2] = Clip3(m6-2*tc, m6+2*tc, ((m3 + m4 + m5 + 3*m6 + 2*m7 +4 )>>3));
  }
  else
  {
    /* Weak filter */
    delta = (9*(m4-m3) -3*(m5-m2) + 8)>>4 ;

    if ( abs(delta) < iThrCut )
    {
      delta = Clip3(-tc, tc, delta);
      piSrc[-iOffset] = ClipBD((m3+delta), bitDepthLuma);
      piSrc[0] = ClipBD((m4-delta), bitDepthLuma);

      Int tc2 = tc>>1;
      if(bFilterSecondP)
      {
        Int delta1 = Clip3(-tc2, tc2, (( ((m1+m3+1)>>1)- m2+delta)>>1));
        piSrc[-iOffset*2] = ClipBD((m2+delta1), bitDepthLuma);
      }
      if(bFilterSecondQ)
      {
        Int delta2 = Clip3(-tc2, tc2, (( ((m6+m4+1)>>1)- m5-delta)>>1));
        piSrc[ iOffset] = ClipBD((m5+delta2), bitDepthLuma);
      }
    }
  }

  if(bPartPNoFilter)
  {
    piSrc[-iOffset] = m3;
    piSrc[-iOffset*2] = m2;
    piSrc[-iOffset*3] = m1;
  }
  if(bPartQNoFilter)
  {
    piSrc[0] = m4;
    piSrc[ iOffset] = m5;
    piSrc[ iOffset*2] = m6;
  }
}

/**
 - Deblocking of one line/column for the chrominance component
 .
 \param piSrc           pointer to picture data
 \param iOffset         offset value for picture data
 \param tc              tc value
 \param bPartPNoFilter  indicator to disable filtering on partP
 \param bPartQNoFilter  indicator to disable filtering on partQ
 \param bitDepthChroma  chroma bit depth
 */
__inline Void TComLoopFilter::xPelFilterChroma( Pel* piSrc, Int iOffset, Int tc, Bool bPartPNoFilter, Bool bPartQNoFilter, const Int bitDepthChroma)
{
  Int delta;

  Pel m4  = piSrc[0];
  Pel m3  = piSrc[-iOffset];
  Pel m5  = piSrc[ iOffset];
  Pel m2  = piSrc[-iOffset*2];

  delta = Clip3(-tc,tc, (((( m4 - m3 ) << 2 ) + m2 - m5 + 4 ) >> 3) );
  piSrc[-iOffset] = ClipBD((m3+delta), bitDepthChroma);
  piSrc[0] = ClipBD((m4-delta), bitDepthChroma);

  if(bPartPNoFilter)
  {
    piSrc[-iOffset] = m3;
  }
  if(bPartQNoFilter)
  {
    piSrc[0] = m4;
  }
}

/**
 - Decision between strong and weak filter
 .
 \param offset         offset value for picture data
 \param d               d value
 \param beta            beta value
 \param tc              tc value
 \param piSrc           pointer to picture data
 */
__inline Bool TComLoopFilter::xUseStrongFiltering( Int offset, Int d, Int beta, Int tc, Pel* piSrc)
{
  Pel m4  = piSrc[0];
  Pel m3  = piSrc[-offset];
  Pel m7  = piSrc[ offset*3];
  Pel m0  = piSrc[-offset*4];

  Int d_strong = abs(m0-m3) + abs(m7-m4);

  return ( (d_strong < (beta>>3)) && (d<(beta>>2)) && ( abs(m3-m4) < ((tc*5+1)>>1)) );
}

__inline Int TComLoopFilter::xCalcDP( Pel* piSrc, Int iOffset)
{
  return abs( piSrc[-iOffset*3] - 2*piSrc[-iOffset*2] + piSrc[-iOffset] ) ;
}

__inline Int TComLoopFilter::xCalcDQ( Pel* piSrc, Int iOffset)
{
  return abs( piSrc[0] - 2*piSrc[iOffset] + piSrc[iOffset*2] );
}
//! \}
//
Void TComLoopFilter::printTU( TComDataCU* pcCU, UInt uiAbsZorderIdx, UInt uiDepth, DeblockEdgeDir edgeDir )

//! \}

{

  if(pcCU->getPic()==0||pcCU->getPartitionSize(uiAbsZorderIdx)==NUMBER_OF_PART_SIZES)
  {
    return;
  }
  TComPic* pcPic     = pcCU->getPic();
  UInt uiCurNumParts = pcPic->getNumPartitionsInCtu() >> (uiDepth<<1);
  UInt uiQNumParts   = uiCurNumParts>>2;
  const TComSPS &sps = *(pcCU->getSlice()->getSPS());
  if( (pcCU->getDepth(uiAbsZorderIdx)+pcCU->getTransformIdx(uiAbsZorderIdx)) > uiDepth )
  {
    for ( UInt uiPartIdx = 0; uiPartIdx < 4; uiPartIdx++, uiAbsZorderIdx+=uiQNumParts )
    {
	    UInt uiLPelX   = pcCU->getCUPelX() + g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsZorderIdx] ];
      	    UInt uiTPelY   = pcCU->getCUPelY() + g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsZorderIdx] ];
      if( ( uiLPelX < sps.getPicWidthInLumaSamples() ) && ( uiTPelY < sps.getPicHeightInLumaSamples() ) )
      {
        printTU( pcCU, uiAbsZorderIdx, uiDepth+1, edgeDir );
        //printf("start set TU filter: z_index=%d, partitionnum=%d\n",uiAbsZorderIdx, uiCurNumParts );
      }
    }
    return;
  }
}
