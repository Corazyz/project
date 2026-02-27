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

/** \file     TEncRateCtrl.h
    \brief    Rate control manager class
*/

#ifndef __TENCRATECTRL__
#define __TENCRATECTRL__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComDataCU.h"
#include "cvi_sigdump.h"

#include <vector>
#include <algorithm>

using namespace std;

//! \ingroup TLibEncoder
//! \{

#include "../TLibEncoder/TEncCfg.h"
#include <list>
#include <cassert>

const Int g_RCInvalidQPValue = -999;
const Int g_RCSmoothWindowSize = 40;
const Int g_RCMaxPicListSize = 32;
const Int g_RCWeightPicTargetBitInGOP    = 75;
const Int g_RCWeightPicTargetBitInBuffer = 100 - g_RCWeightPicTargetBitInGOP;
const Int g_RCIterationNum = 20;
const RC_Float g_RCWeightHistoryLambda = 0.5;
const RC_Float g_RCWeightCurrentLambda = 1.0 - g_RCWeightHistoryLambda;
const Int g_RCLCUSmoothWindowSize = 4;

const RC_Float g_RCAlphaMinValue = 0.05;
const RC_Float g_RCAlphaMaxValue = 1000.0;
const RC_Float g_RCBetaMinValue  = -3.0;
const RC_Float g_RCBetaMaxValue  = -1.; // -0.1
const RC_Float g_RCIAlphaMinValue = 1.8;
const RC_Float g_RCIBetaMinValue  = 0.6;
const RC_Float g_RCIBetaMaxValue  = 5.;

const RC_Float g_RCMinPPicBpp = 0.00024;
const Int g_RCLastPicQpClip = 3;
const Int g_RCLevelPicQpClip = 3;

#if JVET_K0390_RATE_CTRL
const Int LAMBDA_PREC = 1000000;
#endif

#define ALPHA     6.7542
#define BETA1     1.2517
#define BETA2     1.7860

#define MAX_BIT_ERROR (1<<28)

struct TRCLCU
{
  Int m_actualBits;
  Int m_QP;     // QP of skip mode is set to g_RCInvalidQPValue
  Int m_targetBits;
  RC_Float m_lambda;
  RC_Float m_bitWeight;
  Int m_numberOfPixel;
  RC_Float m_costIntra;
  Int m_targetBitsLeft;
#if JVET_K0390_RATE_CTRL
  RC_Float m_actualSSE;
  RC_Float m_actualMSE;
#endif
};

struct TRCParameter
{
  RC_Float m_alpha;
  RC_Float m_beta;
#if JVET_K0390_RATE_CTRL
  Int    m_validPix;
#endif
#if JVET_M0600_RATE_CTRL
  RC_Float m_skipRatio;
#endif
};

class TEncRCSeq
{
public:
  TEncRCSeq();
  ~TEncRCSeq();

public:
  Void create( Int totalFrames, Int targetBitrate, Int frameRate, Int GOPSize, Int picWidth, Int picHeight, Int LCUWidth, Int LCUHeight, Int numberOfLevel, Bool useLCUSeparateModel, Int adaptiveBit, Int intraPeriod, Bool useLowLuxEnControl, Int LuxValue, Int intraQPoffset);
  Void destroy();
  Void initBitsRatio( Int bitsRatio[] );
  Void initGOPID2Level( Int GOPID2Level[] );
  Void initPicPara( TRCParameter* picPara  = NULL );    // NULL to initial with default value
  Void initLCUPara( TRCParameter** LCUPara = NULL );    // NULL to initial with default value
  Void updateAfterPic ( Int bits );
  Void setAllBitRatio( RC_Float basicLambda, RC_Float* equaCoeffA, RC_Float* equaCoeffB );

public:
  Int  getTotalFrames()                 { return m_totalFrames; }
  Int  getTargetRate()                  { return m_targetRate; }
  Int  getFrameRate()                   { return m_frameRate; }
  Int  getGOPSize()                     { return m_GOPSize; }
  Int  getPicWidth()                    { return m_picWidth; }
  Int  getPicHeight()                   { return m_picHeight; }
  Int  getLCUWidth()                    { return m_LCUWidth; }
  Int  getLCUHeight()                   { return m_LCUHeight; }
  Int  getNumberOfLevel()               { return m_numberOfLevel; }
  Int  getAverageBits()                 { return m_averageBits; }
  Int  getLeftAverageBits()             { assert( m_framesLeft > 0 ); return (Int)(m_bitsLeft / m_framesLeft); }
  Bool getUseLCUSeparateModel()         { return m_useLCUSeparateModel; }
#if CVI_LOW_LUX_CONTROL
  Bool getUseLowLuxCtrl()               { return m_LowLuxEnControl;     }
  Int  getLowLuxValue()                 { return m_LuxValue;   }
  Bool getLowLuxAdaptive()              { return m_picLowLuxAdaptive;   }
  Void setLowLuxAdaptive( Bool value )  { m_picLowLuxAdaptive = value;  }
  Int  getLowLuxLevel()                 { return m_picLowLuxLevel;   }
  Void setLowLuxLevel( Int level )      { m_picLowLuxLevel = level;     }
  Int  getLowLuxAvg()                   { return m_LuxAvrage;   }
  Void setLowLuxAvg( Int avg )          { m_LuxAvrage = avg;      }
  Int  getLowLuxWindow(Int num)         { return m_picLowLuxWindow[num]; }
  void setLowLuxWindow(Int value, Int num)   { m_picLowLuxWindow[num] = value; }
#endif

  Int  getNumPixel()                    { return m_numberOfPixel; }
  Int  getNumPixelAlign32()             { return m_numberOfPixelAlign32; }
  Int64  getTargetBits()                { return m_targetBits; }
  Int  getNumberOfLCU()                 { return m_numberOfLCU; }
  Int* getBitRatio()                    { return m_bitsRatio; }
  Int  getBitRatio( Int idx )           { assert( idx<m_GOPSize); return m_bitsRatio[idx]; }
  Int* getGOPID2Level()                 { return m_GOPID2Level; }
  Int  getGOPID2Level( Int ID )         { assert( ID < m_GOPSize ); return m_GOPID2Level[ID]; }
  TRCParameter*  getPicPara()                                   { return m_picPara; }
  TRCParameter   getPicPara( Int level )                        { assert( level < m_numberOfLevel ); return m_picPara[level]; }
  Void           setPicPara( Int level, TRCParameter para )     { assert( level < m_numberOfLevel ); m_picPara[level] = para; }
  TRCParameter** getLCUPara()                                   { return m_LCUPara; }
  TRCParameter*  getLCUPara( Int level )                        { assert( level < m_numberOfLevel ); return m_LCUPara[level]; }
  TRCParameter   getLCUPara( Int level, Int LCUIdx )            { assert( LCUIdx  < m_numberOfLCU ); return getLCUPara(level)[LCUIdx]; }
  Void           setLCUPara( Int level, Int LCUIdx, TRCParameter para ) { assert( level < m_numberOfLevel ); assert( LCUIdx  < m_numberOfLCU ); m_LCUPara[level][LCUIdx] = para; }

  Int  getFramesLeft()                  { return m_framesLeft; }
  Int64  getBitsLeft()                  { return m_bitsLeft; }

  RC_Float getSeqBpp()                    { return m_seqTargetBpp; }
  RC_Float getAlphaUpdate()               { return m_alphaUpdate; }
  RC_Float getBetaUpdate()                { return m_betaUpdate; }

  Int    getAdaptiveBits()              { return m_adaptiveBit;  }
  RC_Float getLastLambda()                { return m_lastLambda;   }
  Void   setLastLambda( RC_Float lamdba ) { m_lastLambda = lamdba; }
  void resetHistoryPicQpAccum()           { m_historyPicQpAccum = 0; }
  void addHistoryPicQpAccum(double qp)    { m_historyPicQpAccum += qp; }
  RC_Float getHistoryAvgPicQp()          { return m_historyPicQpAccum/((RC_Float)max(m_intraPeriod-1, 1)); }
  Int64&  getBitError()                 { return m_bitError; }
  Int    getMaxIPicBit()               { return m_maxIPicBit; }
  Int    getMinIPicBit()               { return m_minIPicBit; }
  Int    getIntraPeriod()              { return m_intraPeriod; }
  Int    getMinPicBit()                { return m_minPicBit; }
  Int&   getLastIPicBit()              { return m_lastIPicBit; }
  RC_Float& getLastILambda()           { return m_lastILambda; }
  RC_Float& getLastIQp()               { return m_lastIQp; }

  Int    getNeiIBitMargin()            { return m_neiIBitMargin; }
  void   setSliceType(SliceType sliceType)  { m_lastSliceType = m_curSliceType; m_curSliceType = sliceType; }
  void   setPictureIdx(Int picIdx)     { m_pictureIdx = picIdx;}
  Int    getPictureIdx()               { return m_pictureIdx;}
  SliceType getSliceType()             { return m_curSliceType; }
  Bool      isFirstPPic()              { return m_lastSliceType==I_SLICE && m_curSliceType==P_SLICE && m_pictureIdx > m_intraPeriod; }
  UInt      getRcGopSize()             { return m_rcGopSize; }
  UInt      getStatFrameNum()          { return m_StatFrameNum; }
  UInt&     getLastGopAvgBit()         { return m_LastGopAvgBit; }
private:
  Int m_totalFrames;
  Int m_targetRate;
  Int m_frameRate;
  Int m_GOPSize;
  Int m_picWidth;
  Int m_picHeight;
  Int m_LCUWidth;
  Int m_LCUHeight;
  Int m_numberOfLevel;
  Int m_averageBits;
  Int m_intraPeriod;
  Int m_numberOfPixel;
  Int m_numberOfPixelAlign32;
  Int64 m_targetBits;
  Int m_numberOfLCU;
  Int* m_bitsRatio;
  Int* m_GOPID2Level;
  TRCParameter*  m_picPara;
  TRCParameter** m_LCUPara;

  Int m_framesLeft;
  Int64 m_bitsLeft;
  RC_Float m_seqTargetBpp;
  RC_Float m_alphaUpdate;
  RC_Float m_betaUpdate;
  Bool m_useLCUSeparateModel;
#if CVI_LOW_LUX_CONTROL
  Bool m_LowLuxEnControl;
  Bool m_picLowLuxAdaptive;
  Int m_LuxValue;
  Int m_picLowLuxLevel;
  Int m_picLowLuxWindow[MAX_LOW_LUX_WIN];
  Int m_LuxAvrage;
#endif
  Int m_adaptiveBit;
  RC_Float m_lastLambda;
  RC_Float m_historyPicQpAccum;
  Int64  m_bitError;
  Int    m_pictureIdx;
  Int m_maxIPicBit;
  Int m_minIPicBit;
  Int m_minPicBit;
  Int m_lastIPicBit;
  Int m_intraQPoffset;
  RC_Float m_lastILambda;
  RC_Float m_lastIQp;
  UInt      m_StatFrameNum;
  UInt      m_rcGopSize;
  UInt      m_LastGopAvgBit;
  SliceType m_curSliceType;
  SliceType m_lastSliceType;
  Int       m_neiIBitMargin;
};

class TEncRCGOP
{
public:
  TEncRCGOP();
  ~TEncRCGOP();

public:
  Void create( TEncRCSeq* encRCSeq, Int numPic );
  Void destroy();
  Void updateAfterPicture( Int bitsCost );

private:
  Int  xEstGOPTargetBits( TEncRCSeq* encRCSeq, Int GOPSize );
  Void   xCalEquaCoeff( TEncRCSeq* encRCSeq, RC_Float* lambdaRatio, RC_Float* equaCoeffA, RC_Float* equaCoeffB, Int GOPSize );
#if JVET_K0390_RATE_CTRL
  RC_Float xSolveEqua(TEncRCSeq* encRCSeq, RC_Float targetBpp, RC_Float* equaCoeffA, RC_Float* equaCoeffB, Int GOPSize);
#else
  RC_Float xSolveEqua( RC_Float targetBpp, RC_Float* equaCoeffA, RC_Float* equaCoeffB, Int GOPSize );
#endif
public:
  TEncRCSeq* getEncRCSeq()        { return m_encRCSeq; }
  Int  getNumPic()                { return m_numPic;}
  Int  getTargetBits()            { return m_targetBits; }
  Int  getPicLeft()               { return m_picLeft; }
  Int  getBitsLeft()              { return m_bitsLeft; }
  Int  getTargetBitInGOP( Int i ) { return m_picTargetBitInGOP[i]; }

private:
  TEncRCSeq* m_encRCSeq;
  Int* m_picTargetBitInGOP;
  Int m_numPic;
  Int m_targetBits;
  Int m_picLeft;
  Int m_bitsLeft;
};

class TEncRCPic
{
public:
  TEncRCPic();
  ~TEncRCPic();

public:
  Void create( TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP, Int frameLevel, list<TEncRCPic*>& listPreviousPictures );
  Void destroy();

  Int    estimatePicQP    ( RC_Float lambda, list<TEncRCPic*>& listPreviousPictures);
  Int    getRefineBitsForIntra(Int orgBits, Int qp);
  RC_Float intraLambdaToBpp(RC_Float lambda, RC_Float alpha, RC_Float beta);
  RC_Float lambdaToBpp(RC_Float lambda, RC_Float alpha, RC_Float beta);
  RC_Float getPicTextCplx();

  Void   fillHwQpLut (SliceType eSliceType, Int qpOffset=0);
  Void   setupHwRcParam(SliceType eSliceType, bool isFirstFrame);
  Void   hw_picInit();
  Void   hw_calcCTUOvfTargetBit();
  Int    hw_getCTULowTargetBit(Int rowIdx);
  Void   hw_calcCTULowBaseQp(Int targetBit);
  Int    hw_getCTUBaseQp();
  Int    hw_blockQpDecision(Int baseQp, Int blkPosX, Int blkPosY);
  Void   genAllCUQp(Int baseQp, Int ctuX, Int ctuY);

  RC_Float calculateLambdaIntra(RC_Float alpha, RC_Float beta, RC_Float MADPerPixel, RC_Float bitsPerPixel);
#if SOFT_FLOAT 
  RC_Float calculateLambdaIntra(Soft_Float alpha, Soft_Float beta, Soft_Float MADPerPixel, Soft_Float bitsPerPixel);
#endif
  RC_Float estimatePicLambda( list<TEncRCPic*>& listPreviousPictures, SliceType eSliceType);

  Void   updateAlphaBetaIntra(RC_Float *alpha, RC_Float *beta);

  RC_Float getLCUTargetBpp(SliceType eSliceType);
  RC_Float getLCUEstLambdaAndQP(RC_Float bpp, Int clipPicQP, Int *estQP);
  RC_Float getLCUEstLambda( RC_Float bpp );
  Int    getLCUEstQP( RC_Float lambda, Int clipPicQP );
#if JVET_M0600_RATE_CTRL
  void updateAfterCTU( Int LCUIdx, Int bits, Int QP, RC_Float lambda, RC_Float skipRatio, Bool updateLCUParameter = true);
#else
  Void updateAfterCTU( Int LCUIdx, Int bits, Int QP, RC_Float lambda, Bool updateLCUParameter = true );
#endif
  Void updateAfterPicture( Int actualHeaderBits, Int actualTotalBits, RC_Float averageQP, RC_Float averageLambda, SliceType eSliceType);

  Void addToPictureLsit( list<TEncRCPic*>& listPreviousPictures );
  RC_Float calAverageQP();
  RC_Float calAverageLambda();
  Void calAvgLambdaAndQpByHist(RC_Float* lambda, RC_Float* qp);

  int getCtuRowAvgBit() { return m_ctuRowAvgBit; };

#if CVI_LOW_LUX_CONTROL
  Void cviEnc_LowLux_PicCtrl(TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP);
  Int calc_avg_lowlux();
#endif

private:
  Int xEstPicTargetBits( TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP );
  Int xEstPicHeaderBits( list<TEncRCPic*>& listPreviousPictures, Int frameLevel );
  Int xEstPicLowerBound( TEncRCSeq* encRCSeq, TEncRCGOP* encRCGOP );

public:
  TEncRCSeq*      getRCSequence()                         { return m_encRCSeq; }
  TEncRCGOP*      getRCGOP()                              { return m_encRCGOP; }

  Int  getFrameLevel()                                    { return m_frameLevel; }
  Int  getNumberOfPixel()                                 { return m_numberOfPixel; }
  Int  getNumberOfPixelAlign32()                          { return m_numberOfPixelAlign32; }
  Int  getNumberOfLCU()                                   { return m_numberOfLCU; }
  Int  getTargetBits()                                    { return m_targetBits; }
  Int  getEstHeaderBits()                                 { return m_estHeaderBits; }
  Int  getLCULeft()                                       { return m_LCULeft; }
  Int  getBitsLeft()                                      { return m_bitsLeft; }
  Int  getPixelsLeft()                                    { return m_pixelsLeft; }
  Int  getBitsCoded()                                     { return m_targetBits - m_estHeaderBits - m_bitsLeft; }
  Int  getLCUCoded()                                      { return m_numberOfLCU - m_LCULeft; }
  Int  getLowerBound()                                    { return m_lowerBound; }
  TRCLCU* getLCU()                                        { return m_LCUs; }
  TRCLCU& getLCU( Int LCUIdx )                            { return m_LCUs[LCUIdx]; }
  Int  getPicActualHeaderBits()                           { return m_picActualHeaderBits; }
  Void setBitLeft(Int bits)                               { m_bitsLeft = bits; }
  Void setTargetBits( Int bits )                          { m_targetBits = bits; m_bitsLeft = bits;}
  Void setTotalIntraCost(RC_Float cost)                     { m_totalCostIntra = cost; }
  Void getLCUInitTargetBits();

  Int  getPicActualBits()                                 { return m_picActualBits; }
  Int  getPicActualQP()                                   { return m_picQP; }
  RC_Float getPicActualLambda()                             { return m_picLambda; }
  Int  getPicEstQP()                                      { return m_estPicQP; }
  Void setPicEstQP( Int QP )                              { m_estPicQP = QP; }
  RC_Float getPicEstLambda()                                { return m_estPicLambda; }
  Void setPicEstLambda( RC_Float lambda )                   { m_picLambda = lambda; }

#if JVET_K0390_RATE_CTRL
  RC_Float getPicMSE()                                      { return m_picMSE; }
  void  setPicMSE(RC_Float avgMSE)                           { m_picMSE = avgMSE; }
#endif

  Int m_numberOfSkipPixel;
private:
  TEncRCSeq* m_encRCSeq;
  TEncRCGOP* m_encRCGOP;

  Int m_frameLevel;
  Int m_numberOfPixel;
  Int m_numberOfPixelAlign32;
  Int m_numberOfLCU;
  Int m_targetBits;
  Int m_estHeaderBits;
  Int m_estPicQP;
  Int m_lowerBound;
  RC_Float m_estPicLambda;

  Int m_LCULeft;
  Int m_bitsLeft;
  Int m_pixelsLeft;

  TRCLCU* m_LCUs;
  Int m_picActualHeaderBits;    // only SH and potential APS
  RC_Float m_totalCostIntra;
  RC_Float m_remainingCostIntra;
  Int m_picActualBits;          // the whole picture, including header
  Int m_picQP;                  // in integer form
  RC_Float m_picLambda;
#if JVET_K0390_RATE_CTRL
  RC_Float m_picMSE;
  Int m_validPixelsInPic;
#endif
  Int m_numOfPelInCTU;
  Int m_numOfCTURow;
  Int m_ctuNumInRow;
  Int m_rowBaseQp;
  Int m_ctuRowAvgBit;
  Int m_ctuRowTargetBit;
  Int m_ctuRowOvfTargetBit;
  Int m_ctuRowAccumBit;
  Int m_blkMinQp;
  Int m_blkMaxQp;
  Bool m_isRowOverflow;
  Bool m_isRowOverflow_delay; // delay 1 ctu to sync HW behavior

  Int m_ctuRowAccumBit_delay; // delay 1 ctu to sync HW behavior, for verification
};

class TEncRateCtrl
{
public:
  TEncRateCtrl();
  ~TEncRateCtrl();

public:
  Void init( Int totalFrames, Int targetBitrate, Int frameRate, Int GOPSize, Int picWidth, Int picHeight, Int LCUWidth, Int LCUHeight, Int keepHierBits, Bool useLCUSeparateModel, GOPEntry GOPList[MAX_GOP], Int intraPeriod, Bool useLowLuxEnControl, Int LuxValue, Int intraQPoffset);
  Void destroy();
  Void initRCPic( Int frameLevel );
  Void initRCGOP( Int numberOfPictures );
  Void destroyRCGOP();

public:
  Void       setRCQP ( Int QP ) { m_RCQP = QP;   }
  Int        getRCQP ()         { return m_RCQP; }
  TEncRCSeq* getRCSeq()          { assert ( m_encRCSeq != NULL ); return m_encRCSeq; }
  TEncRCGOP* getRCGOP()          { assert ( m_encRCGOP != NULL ); return m_encRCGOP; }
  TEncRCPic* getRCPic()          { assert ( m_encRCPic != NULL ); return m_encRCPic; }
  list<TEncRCPic*>& getPicList() { return m_listRCPictures; }
  Bool       getCpbSaturationEnabled()  { return m_CpbSaturationEnabled;  }
  UInt       getCpbState()              { return m_cpbState;       }
  UInt       getCpbSize()               { return m_cpbSize;        }
  UInt       getBufferingRate()         { return m_bufferingRate;  }
  Int        updateCpbState(Int actualBits);
  Void       initHrdParam(const TComHRD* pcHrd, Int iFrameRate, RC_Float fInitialCpbFullness);
private:
  TEncRCSeq* m_encRCSeq;
  TEncRCGOP* m_encRCGOP;
  TEncRCPic* m_encRCPic;
  list<TEncRCPic*> m_listRCPictures;
  Int        m_RCQP;
  Bool       m_CpbSaturationEnabled;    // Enable target bits saturation to avoid CPB overflow and underflow
  Int        m_cpbState;                // CPB State 
  UInt       m_cpbSize;                 // CPB size
  UInt       m_bufferingRate;           // Buffering rate
};

#endif


