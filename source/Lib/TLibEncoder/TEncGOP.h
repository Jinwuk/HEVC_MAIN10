/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2014, ITU/ISO/IEC
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

/** \file     TEncGOP.h
    \brief    GOP encoder class (header)
*/

#ifndef __TENCGOP__
#define __TENCGOP__

#include <list>

#include <stdlib.h>

#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComLoopFilter.h"
#include "TLibCommon/AccessUnit.h"
#include "TEncSampleAdaptiveOffset.h"
#include "TEncSlice.h"
#include "TEncEntropy.h"
#include "TEncCavlc.h"
#include "TEncSbac.h"
#include "SEIwrite.h"

#include "TEncAnalyze.h"
#include "TEncRateCtrl.h"
#include <vector>


#include "TEncFrame.h"

//! \ingroup TLibEncoder
//! \{

class TEncTop;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// GOP encoder class
class TEncGOP
{
private:
  //  Data
  Bool                    m_bLongtermTestPictureHasBeenCoded;
  Bool                    m_bLongtermTestPictureHasBeenCoded2;
  UInt                    m_numLongTermRefPicSPS;
  UInt                    m_ltRefPicPocLsbSps[33];
  Bool                    m_ltRefPicUsedByCurrPicFlag[33];
  Int                     m_iLastIDR;
  Int                     m_iGopSize;
  Int                     m_iNumPicCoded;
  Bool                    m_bFirst;
#if ALLOW_RECOVERY_POINT_AS_RAP
  Int                     m_iLastRecoveryPicPOC;
#endif
  
  //  Access channel
  TEncTop*                m_pcEncTop;
  TEncCfg*                m_pcCfg;
  TEncSlice*              m_pcSliceEncoder;
  TComList<TComPic*>*     m_pcListPic;
  
  TEncEntropy*            m_pcEntropyCoder;
  TEncCavlc*              m_pcCavlcCoder;
  TEncSbac*               m_pcSbacCoder;
  TEncBinCABAC*           m_pcBinCABAC;
  TComLoopFilter*         m_pcLoopFilter;

  SEIWriter               m_seiWriter;
  
  //--Adaptive Loop filter
  TEncSampleAdaptiveOffset*  m_pcSAO;
  TComBitCounter*         m_pcBitCounter;
  TEncRateCtrl*           m_pcRateCtrl;
  // indicate sequence first
  Bool                    m_bSeqFirst;
  
  // clean decoding refresh
  Bool                    m_bRefreshPending;
  Int                     m_pocCRA;
  std::vector<Int>        m_storedStartCUAddrForEncodingSlice;
  std::vector<Int>        m_storedStartCUAddrForEncodingSliceSegment;
#if FIX1172
  NalUnitType             m_associatedIRAPType;
  Int                     m_associatedIRAPPOC;
#endif

  std::vector<Int> m_vRVM_RP;
  UInt                    m_lastBPSEI;
  UInt                    m_totalCoded;
  UInt                    m_cpbRemovalDelay;
  UInt                    m_tl0Idx;
  UInt                    m_rapIdx;
  Bool                    m_activeParameterSetSEIPresentInAU;
  Bool                    m_bufferingPeriodSEIPresentInAU;
  Bool                    m_pictureTimingSEIPresentInAU;
  Bool                    m_nestedBufferingPeriodSEIPresentInAU;
  Bool                    m_nestedPictureTimingSEIPresentInAU;

//======================================================================================
//	ETRI member Variables 
//======================================================================================
  TEncFrame*		em_pcFrameEncoder;

public:
  TEncGOP();
  virtual ~TEncGOP();
  
  Void  create      ();
  Void  destroy     ();
  
  Void  init        ( TEncTop* pcTEncTop );
  Void  xAttachSliceDataToNalUnit (OutputNALUnit& rNalu, TComOutputBitstream*& rpcBitstreamRedirect);

  
  Int   getGOPSize()          { return  m_iGopSize;  }
  
  TComList<TComPic*>*   getListPic()      { return m_pcListPic; }
  
  Void  printOutSummary      ( UInt uiNumAllPicCoded, bool isField);
  Void  preLoopFilterPicAll  ( TComPic* pcPic, UInt64& ruiDist, UInt64& ruiBits );
  
  TEncSlice*  getSliceEncoder()   { return m_pcSliceEncoder; }
  NalUnitType getNalUnitType( Int pocCurr, Int lastIdr, Bool isField );
  Void arrangeLongtermPicturesInRPS(TComSlice *, TComList<TComPic*>& );

//======================================================================================
//	ETRI Public Functions 
//======================================================================================
//------------------------ For Data Interface -------------------------
TEncEntropy*	ETRI_getEntropyCoder	()	{return m_pcEntropyCoder;}
TEncCavlc*   	ETRI_getCavlcCoder		()	{return m_pcCavlcCoder;}
TEncSbac*  		ETRI_getSbacCoder		()	{return	m_pcSbacCoder;}
TEncBinCABAC* 	ETRI_getBinCABAC		()	{return	m_pcBinCABAC;}
TComBitCounter*	ETRI_getBitCounter		()	{return	m_pcBitCounter;}
SEIWriter*   	ETRI_getSEIWriter		()	{return &m_seiWriter;}

TComLoopFilter* ETRI_getLoopFilter  	()	{return m_pcLoopFilter;}
TEncSampleAdaptiveOffset*  ETRI_getSAO	()	{return m_pcSAO;}

Int  	ETRI_getiLastIDR				()	{return m_iLastIDR;}
Int  	ETRI_getassociatedIRAPPOC   	()	{return m_associatedIRAPPOC ;}	///< For ETRI_SetReferencePictureSetforSlice : Is it necessary to a member variable ??	@ 2015 5 24 by Seok
Int& 	ETRI_getiLastRecoveryPicPOC 	()	{return m_iLastRecoveryPicPOC;}	///< For ETRI_SetReferencePictureSetforSlice : Is it necessary to a member variable ??	@ 2015 5 24 by Seok 2015 5 24 by Seok
Int  	ETRI_getpocCRA					()	{return m_pocCRA;}				///< For ETRI_SetReferencePictureSetforSlice : Is it necessary to a member variable ??	@ 2015 5 24 by Seok 2015 5 24 by Seok

UInt&  	ETRI_getlastBPSEI				()	{return m_lastBPSEI;}			///< For in ETRI_setPictureTimingSEI @2015 5 24 by Seok
UInt   	ETRI_gettotalCoded				()	{return m_totalCoded;}  		///< For in ETRI_setPictureTimingSEI @2015 5 24 by Seok

Bool 	ETRI_getactiveParameterSetSEIPresentInAU	()	{return m_activeParameterSetSEIPresentInAU;}	///< For ETRI_writeHRDInfo @ 2015 5 24 by Seok
Bool& 	ETRI_getpictureTimingSEIPresentInAU 		()	{return m_pictureTimingSEIPresentInAU;}   		///< For ETRI_writeHRDInfo @ 2015 5 24 by Seok
Bool&	ETRI_getbufferingPeriodSEIPresentInAU 		()	{return m_bufferingPeriodSEIPresentInAU;}  		///< For ETRI_writeHRDInfo @ 2015 5 24 by Seok
Bool& 	ETRI_getnestedBufferingPeriodSEIPresentInAU	()	{return	m_nestedBufferingPeriodSEIPresentInAU;}	///< For ETRI_writeHRDInfo @ 2015 5 24 by Seok
UInt& 	ETRI_getcpbRemovalDelay  		()  {return m_cpbRemovalDelay;}   	///< For ETRI_writeHRDInfo @ 2015 5 24 by Seok

UInt  	ETRI_getnumLongTermRefPicSPS	()	{return m_numLongTermRefPicSPS;}
UInt*	ETRI_getltRefPicPocLsbSps		()	{return	m_ltRefPicPocLsbSps;}	///< Total Number of Ref Pic Order is 33. Look at the TEncGOP.h  @2015 5 24 by Seok
Bool*	ETRI_getltRefPicUsedByCurrPicFlag()	{return	m_ltRefPicUsedByCurrPicFlag;}///< Total Number of Ref Pic Order is 33. Look at the TEncGOP.h  @2015 5 24 by Seok
Bool 	ETRI_getbRefreshPending			()	{return m_bRefreshPending;}		///< For ETRI_SetReferencePictureSetforSlice : Is it necessary to a member variable ??	@ 2015 5 24 by Seok 2015 5 24 by Seok
Bool& 	ETRI_getbFirst  				()	{return m_bFirst;}

Void 	ETRI_setpocCRA					(Int ipocCRA) 			{m_pocCRA = ipocCRA;}	///< For ETRI_SetReferencePictureSetforSlice : Is it necessary to a member variable ??	@ 2015 5 24 by Seok 
Void 	ETRI_setbRefreshPending			(Bool bRefreshPending)	{m_bRefreshPending = bRefreshPending;}	///< For ETRI_SetReferencePictureSetforSlice : Is it necessary to a member variable ??	@ 2015 5 24 by Seok

NalUnitType ETRI_getassociatedIRAPType	()	{return m_associatedIRAPType;}	///Is it necessary to a member variable ??	@ 2015 5 24 by Seok

std::vector<Int>	*ETRI_getstoredStartCUAddrForEncodingSlice  		()	{return	&m_storedStartCUAddrForEncodingSlice;}	   		///< For Test @ 2015 5 24 by Seok		
std::vector<Int>	*ETRI_getstoredStartCUAddrForEncodingSliceSegment	()	{return &m_storedStartCUAddrForEncodingSliceSegment;}	///< For Test @ 2015 5 24 by Seok

//------------------------ For GOP Compression -------------------------
Void ETRI_EfficientFieldIRAPProc(Int iPOCLast, Int iNumPicRcvd, bool isField, Int& IRAPGOPid, Bool& IRAPtoReorder, Bool& swapIRAPForward, Bool bOperation);
void ETRI_xCalculateAddPSNR(TComPic* pcPic, TComList<TComPic*>& rcListPic, AccessUnit& accessUnit, Double dEncTime, Bool isField, Bool isTff); ///< 2015 5 13 by Seok:This Function is move to TEncFrame Class
Void ETRI_SetSEITemporalLevel0Index(TComSlice *pcSlice, SEITemporalLevel0Index& sei_temporal_level0_index);		///< TO Write Out SEI data,  Set the SEI data of TemporalLevel0Index @ 2015 5 25 by Seok

__inline Int ETRI_iGOPIRAPtoReorder_v1(Int iGOPid, Int IRAPGOPid, Bool IRAPtoReorder, Bool swapIRAPForward, Bool bOperation);
__inline Int ETRI_iGOPIRAPtoReorder_v2(Int iGOPid, Int IRAPGOPid, Bool& IRAPtoReorder, Bool swapIRAPForward, Bool bOperation);
__inline Int ETRI_iGOPIRAPtoReorder_v3(Int iGOPid, Int IRAPGOPid, Bool& IRAPtoReorder, Bool swapIRAPForward, Bool bOperation);
__inline Bool ETRI_FIrstPOCForFieldFrame(Int& pocCurr, Int& iTimeOffset, Int iPOCLast, Int iNumPicRcvd, Int iGOPid, Int IRAPGOPid, Bool& IRAPtoReorder, Bool swapIRAPForward, bool isField);	///< To TEncFrame

#if ETRI_MODIFICATION_V00
Void  ETRI_compressGOP ( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRec, std::list<AccessUnit>& accessUnitsInGOP, Bool isField, Bool isTff );
#else
Void  compressGOP ( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRec, std::list<AccessUnit>& accessUnitsInGOP, Bool isField, Bool isTff );
#endif
//======================================================================================
  TEncRateCtrl* getRateCtrl()       { return m_pcRateCtrl;  }		///< Original getRateCtrl() is a protected function. WHy ??? @ 2015 5 24 by Seok
  Void dblMetric( TComPic* pcPic, UInt uiNumSlices );			///< Original dblMetric is a protected function. WHy ??? @ 2015 5 24 by Seok
  Void xCreateLeadingSEIMessages (/*SEIMessages seiMessages,*/ AccessUnit &accessUnit, TComSPS *sps); ///< Original Function is a protected function. WHy ??? @ 2015 5 24 by Seok
  Int xGetFirstSeiLocation (AccessUnit &accessUnit);
  Void  xCalculateAddPSNR ( TComPic* pcPic, TComPicYuv* pcPicD, const AccessUnit&, Double dEncTime );
  Void  xCalculateInterlacedAddPSNR( TComPic* pcPicOrgTop, TComPic* pcPicOrgBottom, TComPicYuv* pcPicRecTop, TComPicYuv* pcPicRecBottom, const AccessUnit& accessUnit, Double dEncTime );

Void xResetNonNestedSEIPresentFlags()
{
  m_activeParameterSetSEIPresentInAU = false;
  m_bufferingPeriodSEIPresentInAU	 = false;
  m_pictureTimingSEIPresentInAU 	 = false;
}
Void xResetNestedSEIPresentFlags()
{
  m_nestedBufferingPeriodSEIPresentInAU    = false;
  m_nestedPictureTimingSEIPresentInAU	   = false;
}

Void  xGetBuffer		( TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, Int iNumPicRcvd, Int iTimeOffset, TComPic*& rpcPic, TComPicYuv*& rpcPicYuvRecOut, Int pocCurr, bool isField );


protected:
  
  Void  xInitGOP          ( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, bool isField );
//  Void  xGetBuffer        ( TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, Int iNumPicRcvd, Int iTimeOffset, TComPic*& rpcPic, TComPicYuv*& rpcPicYuvRecOut, Int pocCurr, bool isField );
  UInt64 xFindDistortionFrame (TComPicYuv* pcPic0, TComPicYuv* pcPic1);
  Double xCalculateRVM();
  SEIActiveParameterSets* xCreateSEIActiveParameterSets (TComSPS *sps);
  SEIFramePacking*        xCreateSEIFramePacking();
  SEIDisplayOrientation*  xCreateSEIDisplayOrientation();

  SEIToneMappingInfo*     xCreateSEIToneMappingInfo();


//--------------------------------------------------------------------------------------------------------
//	Originaly the foloowing Functions defined as protected function. However, we chaged the chrateristics of the functions as public
//	2015 5 25 by Seok ETRI
//--------------------------------------------------------------------------------------------------------
#if !ETRI_MODIFICATION_V00
#if 0
  Void xResetNonNestedSEIPresentFlags()
  {
    m_activeParameterSetSEIPresentInAU = false;
    m_bufferingPeriodSEIPresentInAU    = false;
    m_pictureTimingSEIPresentInAU      = false;
  }
  Void xResetNestedSEIPresentFlags()
  {
    m_nestedBufferingPeriodSEIPresentInAU    = false;
    m_nestedPictureTimingSEIPresentInAU      = false;
  }

  TEncRateCtrl* getRateCtrl() 	  { return m_pcRateCtrl;  } 	  
  Int xGetFirstSeiLocation (AccessUnit &accessUnit);
  Void xCreateLeadingSEIMessages (/*SEIMessages seiMessages,*/ AccessUnit &accessUnit, TComSPS *sps);
  Void dblMetric( TComPic* pcPic, UInt uiNumSlices );
#endif
#endif
};// END CLASS DEFINITION TEncGOP

// ====================================================================================================================
// Enumeration
// ====================================================================================================================
enum PROCESSING_STATE
{
  EXECUTE_INLOOPFILTER,
  ENCODE_SLICE
};

enum SCALING_LIST_PARAMETER
{
  SCALING_LIST_OFF,
  SCALING_LIST_DEFAULT,
  SCALING_LIST_FILE_READ
};

//! \}

#endif // __TENCGOP__

