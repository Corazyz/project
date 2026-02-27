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

/** \file     TComRdCost.h
    \brief    RD cost computation classes (header)
*/

#ifndef __TCOMRDCOST__
#define __TCOMRDCOST__


#include "CommonDef.h"
#include "TComPattern.h"
#include "TComMv.h"

#include "TComSlice.h"
#include "TComRdCostWeightPrediction.h"
#include "cvi_algo_cfg.h"

//! \ingroup TLibCommon
//! \{

class DistParam;
class TComPattern;

// ====================================================================================================================
// Type definition
// ====================================================================================================================

// for function pointer
typedef Distortion (*FpDistFunc) (DistParam*); // TODO: can this pointer be replaced with a reference? - there are no NULL checks on pointer.

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// distortion parameter class
class DistParam
{
public:
  const Pel*            pOrg;
  const Pel*            pCur;
  Int                   iStrideOrg;
  Int                   iStrideCur;
  Int                   iRows;
  Int                   iCols;
  Int                   iStep;
  FpDistFunc            DistFunc;
  Int                   bitDepth;

  Bool                  bApplyWeight;     // whether weighted prediction is used or not
  Bool                  bIsBiPred;

#ifdef CVI_LC_RD_COST
  Bool                  bDisable8x8;
#endif

  const WPScalingParam *wpCur;           // weighted prediction scaling parameters for current ref
  ComponentID           compIdx;
  Distortion            m_maximumDistortionForEarlyExit; /// During cost calculations, if distortion exceeds this value, cost calculations may early-terminate.

  // (vertical) subsampling shift (for reducing complexity)
  // - 0 = no subsampling, 1 = even rows, 2 = every 4th, etc.
  Int             iSubShift;

  DistParam()
   : pOrg(NULL),
     pCur(NULL),
     iStrideOrg(0),
     iStrideCur(0),
     iRows(0),
     iCols(0),
     iStep(1),
     DistFunc(NULL),
     bitDepth(0),
     bApplyWeight(false),
     bIsBiPred(false),
     wpCur(NULL),
     compIdx(MAX_NUM_COMPONENT),
     m_maximumDistortionForEarlyExit(std::numeric_limits<Distortion>::max()),
     iSubShift(0)
  { }
};

/// RD cost computation class
class TComRdCost
{
private:
  // for distortion

  FpDistFunc              m_afpDistortFunc[DF_TOTAL_FUNCTIONS]; // [eDFunc]
  CostMode                m_costMode;
#ifdef CVI_FIX_POINT_RD_COST
  UChar                   m_distortionWeight; // only chroma values are used, cb and cr share one.
  Distortion              m_cviCbDist;
  Distortion              m_cviCbDistOrg;
  Int                     m_cbBits;
#else
  Double                  m_distortionWeight[MAX_NUM_COMPONENT]; // only chroma values are used.
#endif
  Double                  m_dLambda;
  Double                  m_sqrtLambda;
  Double                  m_dLambdaMotionSAD[2 /* 0=standard, 1=for transquant bypass when mixed-lossless cost evaluation enabled*/];
  Double                  m_dLambdaMotionSSE[2 /* 0=standard, 1=for transquant bypass when mixed-lossless cost evaluation enabled*/];
  Double                  m_dFrameLambda;

  // for motion cost
  TComMv                  m_mvPredictor;
  Double                  m_motionLambda;
  Int                     m_iCostScale;

public:
  TComRdCost();
  virtual ~TComRdCost();

  Double calcRdCost( Double numBits, Distortion distortion, DFunc eDFunc = DF_DEFAULT );

#ifdef CVI_ENABLE_RDO_BIT_EST
  Double calcFracBitsRdCost( UInt estBits, Distortion distortion, int round_frac_bits);
#endif

#ifdef CVI_FIX_POINT_RD_COST
  Void    setDistortionWeight  ( const UChar distortionWeight ) { m_distortionWeight = distortionWeight; }
  UChar   getDistortionWeight  () { return m_distortionWeight; }
#else
  Void    setDistortionWeight  ( const ComponentID compID, const Double distortionWeight ) { m_distortionWeight[compID] = distortionWeight; }
#endif
  Void    setLambda( Double dLambda, Double dsqrtLambda, const BitDepths &bitDepths );
  Void    setFrameLambda ( Double dLambda ) { m_dFrameLambda = dLambda; }

  Double  &getSqrtLambda ()   { return m_sqrtLambda; }

  Double  getLambda() { return m_dLambda; }
#ifdef CVI_FIX_POINT_RD_COST
  Int64   getFixpointSADSqrtLambda()
  {
    Int64 frac_Lambda = m_sqrtLambda*((Int64)1<<LC_LAMBDA_FRAC_BIT);
    return frac_Lambda;
  }

  Int64 getFixpointFracLambda()
  {
      Int64 frac_Lambda = m_dLambda*((Int64)1<<LAMBDA_FRAC_BIT);
      return frac_Lambda;
  }

  Int64 getFixpointRdCost(double cost)
  {
    Int64 fix_point_cost = (Int64)(cost * (1 << RD_COST_FRAC_BIT));
    return fix_point_cost;
  }
#endif
#ifdef CVI_FIX_POINT_RD_COST
  UChar  getChromaWeight () { return m_distortionWeight; }
#else
  Double  getChromaWeight () { return ((m_distortionWeight[COMPONENT_Cb] + m_distortionWeight[COMPONENT_Cr]) / 2.0); }
#endif

  Void    setCostMode(CostMode   m )    { m_costMode = m; }

  // Distortion Functions
  Void    init();

  Void    setDistParam( UInt uiBlkWidth, UInt uiBlkHeight, DFunc eDFunc, DistParam& rcDistParam );
  Void    setDistParam( const TComPattern* const pcPatternKey, const Pel* piRefY, Int iRefStride,            DistParam& rcDistParam );
  Void    setDistParam( const TComPattern* const pcPatternKey, const Pel* piRefY, Int iRefStride, Int iStep, DistParam& rcDistParam, Bool bHADME=false );
  Void    setDistParam( DistParam& rcDP, Int bitDepth, const Pel* p1, Int iStride1, const Pel* p2, Int iStride2, Int iWidth, Int iHeight, Bool bHadamard = false
#ifdef CVI_LC_RD_COST
, Bool bDisable8x8 = false
#endif
  );
  Distortion calcHAD(Int bitDepth, const Pel* pi0, Int iStride0, const Pel* pi1, Int iStride1, Int iWidth, Int iHeight );

  // for motion cost
  static UInt    xGetExpGolombNumberOfBits( Int iVal );
  Void    selectMotionLambda( Bool bSad, Int iAdd, Bool bIsTransquantBypass ) { m_motionLambda = (bSad ? m_dLambdaMotionSAD[(bIsTransquantBypass && m_costMode==COST_MIXED_LOSSLESS_LOSSY_CODING) ?1:0] + iAdd : m_dLambdaMotionSSE[(bIsTransquantBypass && m_costMode==COST_MIXED_LOSSLESS_LOSSY_CODING)?1:0] + iAdd); }
  Void    setPredictor( TComMv& rcMv )
  {
    m_mvPredictor = rcMv;
  }
  TComMv getPredictor()
  {
    return m_mvPredictor;
  }
  Void    setCostScale( Int iCostScale )    { m_iCostScale = iCostScale; }
  Distortion getCost( UInt b )
  {
 #ifdef CVI_FIX_POINT_RD_COST
    if(isEnableFixPointRDCost())
    {
      Int64 lambda = m_motionLambda*((Int64)1<<MOTION_LAMBDA_FRAC_BIT);
      return Distortion((lambda * b) >> MOTION_LAMBDA_FRAC_BIT);
    }
    else
#endif
    {
      return Distortion(( m_motionLambda * b ) / 65536.0);
    }
  }

  Distortion getCostOfVectorWithPredictor( const Int x, const Int y )
  {
 #ifdef CVI_FIX_POINT_RD_COST
   if(isEnableFixPointRDCost()) {
    Int64 lambda = m_motionLambda*((Int64)1<<MOTION_LAMBDA_FRAC_BIT);
    return Distortion((lambda * getBitsOfVectorWithPredictor(x, y)) >> MOTION_LAMBDA_FRAC_BIT);
   }
   else
 #endif
   {
    return Distortion((m_motionLambda * getBitsOfVectorWithPredictor(x, y)) / 65536.0);
   }
  }
  UInt getBitsOfVectorWithPredictor( const Int x, const Int y )
  {
    return xGetExpGolombNumberOfBits((x << m_iCostScale) - m_mvPredictor.getHor())
    +      xGetExpGolombNumberOfBits((y << m_iCostScale) - m_mvPredictor.getVer());
  }

private:

  static Distortion xGetSSE           ( DistParam* pcDtParam );
  static Distortion xGetSSE4          ( DistParam* pcDtParam );
  static Distortion xGetSSE8          ( DistParam* pcDtParam );
  static Distortion xGetSSE16         ( DistParam* pcDtParam );
  static Distortion xGetSSE32         ( DistParam* pcDtParam );
  static Distortion xGetSSE64         ( DistParam* pcDtParam );
  static Distortion xGetSSE16N        ( DistParam* pcDtParam );

  static Distortion xGetSAD           ( DistParam* pcDtParam );
  static Distortion xGetSAD4          ( DistParam* pcDtParam );
  static Distortion xGetSAD8          ( DistParam* pcDtParam );
  static Distortion xGetSAD16         ( DistParam* pcDtParam );
  static Distortion xGetSAD32         ( DistParam* pcDtParam );
  static Distortion xGetSAD64         ( DistParam* pcDtParam );
  static Distortion xGetSAD16N        ( DistParam* pcDtParam );

  static Distortion xGetSAD12         ( DistParam* pcDtParam );
  static Distortion xGetSAD24         ( DistParam* pcDtParam );
  static Distortion xGetSAD48         ( DistParam* pcDtParam );

  static Distortion xGetHADs          ( DistParam* pcDtParam );
  static Distortion xCalcHADs2x2      ( const Pel *piOrg, const Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  static Distortion xCalcHADs4x4      ( const Pel *piOrg, const Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep );
  static Distortion xCalcHADs8x8      ( const Pel *piOrg, const Pel *piCurr, Int iStrideOrg, Int iStrideCur, Int iStep
#if VECTOR_CODING__DISTORTION_CALCULATIONS && (RExt__HIGH_BIT_DEPTH_SUPPORT==0)
                                      , Int bitDepth
#endif
                                      );

public:
  Distortion   xGetBiSAD( DistParam* pcDtParam, Int shift);
  Distortion   xGetSAD_SHIFT( DistParam* pcDtParam, Int shift);
  Distortion   xGetSAD_SHIFT_JUMP( DistParam* pcDtParam, Int shift);
  Distortion   getDistPart(Int bitDepth, const Pel* piCur, Int iCurStride, const Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, const ComponentID compID, DFunc eDFunc = DF_SSE, bool isChromaWeightAfterSum = true );
  Distortion   getCVI_DistPart(const ComponentID compID, Distortion CVI_SSE, bool isChromaWeightAfterSum = true);

#ifdef CVI_ENABLE_RDO_FAST_DISTORTION
  void setCbNumBits(int bits) { m_cbBits = bits; }
  Int  getCbNumBits() { return m_cbBits; }
#endif
};// END CLASS DEFINITION TComRdCost

//! \}

#endif // __TCOMRDCOST__
