/*
*********************************************************************************************

   Copyright (c) 2006 Electronics and Telecommunications Research Institute (ETRI) All Rights Reserved.

   Following acts are STRICTLY PROHIBITED except when a specific prior written permission is obtained from 
   ETRI or a separate written agreement with ETRI stipulates such permission specifically:

      a) Selling, distributing, sublicensing, renting, leasing, transmitting, redistributing or otherwise transferring 
          this software to a third party;
      b) Copying, transforming, modifying, creating any derivatives of, reverse engineering, decompiling, 
          disassembling, translating, making any attempt to discover the source code of, the whole or part of 
          this software in source or binary form; 
      c) Making any copy of the whole or part of this software other than one copy for backup purposes only; and 
      d) Using the name, trademark or logo of ETRI or the names of contributors in order to endorse or promote 
          products derived from this software.

   This software is provided "AS IS," without a warranty of any kind. ALL EXPRESS OR IMPLIED CONDITIONS, 
   REPRESENTATIONS AND WARRANTIES, INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS 
   FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED. IN NO EVENT WILL ETRI 
   (OR ITS LICENSORS, IF ANY) BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA, OR FOR DIRECT, 
   INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER CAUSED AND 
   REGARDLESS OF THE THEORY OF LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE USE 
   OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF ETRI HAS BEEN ADVISED OF THE POSSIBILITY OF 
   SUCH DAMAGES.

   Any permitted redistribution of this software must retain the copyright notice, conditions, and disclaimer 
   as specified above.

*********************************************************************************************
*/
/** 
	\file   	TEncFrame.h
   	\brief    	Tile encoder class (header)
	\author		Jinwuk Seok 
	\date		2015.05.22
*/

#ifndef __TENCFRAME__
#define __TENCFRAME__

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
#include "TEncGOP.h"

#include <vector>


//! \ingroup TLibEncoder
//! \{

class TEncTop;
class TEncGOP;
// ====================================================================================================================
// Class definition
// ====================================================================================================================

class TEncFrame 
{
private:
//		TComPicYuv*	em_pcPicYuvRecOut;
		TEncTop* 		em_pcEncTop;
		TEncGOP* 		em_pcGOPEncoder;					///< For Internal Configure Only Use it in the Init @ 2015 5 23 by Seok
		TEncSlice*		em_pcSliceEncoder;
		TEncSearch*		em_pcSearch;
		TEncRateCtrl*  	em_pcRateCtrl;
		TComLoopFilter*	em_pcLoopFilter;
		TEncSampleAdaptiveOffset*  em_pcSAO;

		SEIWriter*		em_pcseiWriter;

		TEncEntropy*   	em_pcEntropyCoder;		
		TEncCavlc* 	  	em_pcCavlcCoder;

		TEncSbac* 	 	em_pcSbacCoder;
		TEncBinCABAC*	em_pcBinCABAC;
		TComBitCounter*	em_pcBitCounter;
		TComRdCost*  	em_pcRdCost;

		TComList<TComPic*>*  	em_rcListPic;
		TComOutputBitstream*	em_pcBitstreamRedirect;

		AccessUnit*		em_pcAU;	///< 2015 5 23 by Seok ???
//		TComList<TComPicYuv*>*	rcListPicYuvRecOut;	///< 2015 5 23 by Seok : ???
	
		Int    		em_iMTFrameIdx;
		Int    		em_iGOPid;
		Int    		em_iPOCLast;
		Int    		em_iNumPicRcvd;
		Int			em_iNumSubstreams;
		Int			em_IRAPGOPid;

///----- For Frame Compression : according to the member variables in TEncGOP @ 2015 5 24 by Seok : with 2 Tabs
		Int    		em_iGopSize;
		Int    		em_iLastIDR;			///< 2015 5 24 by Seok : Int  ETRI_getiLastIDR		()	{return m_iLastIDR;}

		NalUnitType	em_associatedIRAPType;	///< Is it necessary to a member variable ??  @ 2015 5 24 by Seok
		Int    		em_associatedIRAPPOC ;	///< Is it necessary to a member variable ??  @ 2015 5 24 by Seok
		Int			em_iLastRecoveryPicPOC;

		///< ----- Clean Decoding Refresh @ 2015 5 24 by Seok
		Int    		em_pocCRA;
		Bool   		em_bRefreshPending;
		std::vector<Int>	*em_storedStartCUAddrForEncodingSlice;
		std::vector<Int>	*em_storedStartCUAddrForEncodingSliceSegment;

		Bool 		em_bLastGOP;
		Bool   		em_bisField;
		Bool   		em_bisTff;
		Bool  		em_bFirst;   				///< Some what serious. Don't get it from GOP Encoder. If you do that, Speed would be slow. @2015 5 26 by Seok		

		UInt 			em_cpbRemovalDelay;		///< Some what serious. Don't get it from GOP Encoder. If you do that, Speed would be slow. @2015 5 26 by Seok

		UInt*		accumBitsDU;
		UInt*		accumNalsDU;
		Double 		em_dEncTime;

		SEIScalableNesting em_scalableNestingSEI;

#if ETRI_DLL_INTERFACE	// 2013 10 23 by Seok
		UInt	FrameTypeInGOP;
		UInt	FramePOC;
		UInt	FrameEncodingOrder;
#endif

public:
	TEncFrame();
	virtual ~TEncFrame();	

	// -------------------------------------------------------------------------------------------------------------------
	// Creation and Initilization 
	// -------------------------------------------------------------------------------------------------------------------
	Void 	create	();
	Void 	init  	(TEncTop* pcCfg, TEncGOP* pcGOPEncoder, Int iFrameIdx);
	Void 	destroy	();
	// -------------------------------------------------------------------------------------------------------------------
	// member access functions
	// -------------------------------------------------------------------------------------------------------------------
	Void   	ETRI_setFrameParameter(Int iGOPid,  Int pocLast, Int iNumPicRcvd, Int IRAPGOPid, int iLastIDR, UInt* uiaccumBitsDU, UInt* uiaccumNalsDU, TComOutputBitstream*& pcBitstreamRedirect, Bool isField, Bool isTff);
	Void 	ETRI_ResetFrametoGOPParameter();
	Double 	ETRI_getdEncTime  	 		()	{return em_dEncTime;}
	AccessUnit* ETRI_getAccessUnitFrame	()	{return em_pcAU;}
	// -------------------------------------------------------------------------------------------------------------------
	// Auxiliary Compression functions (Employee in ETRI_CompressFrame)
	// -------------------------------------------------------------------------------------------------------------------
	UInt ETRI_Select_UiColDirection(Int iGOPid, UInt uiColDir); 																		
	Void ETRI_SliceDataInitialization(TComPic* pcPic, TComSlice* pcSlice);																
	Void ETRI_SetReferencePictureSetforSlice(TComPic* pcPic, TComSlice* pcSlice, Int iGOPid, Int pocCurr, Bool isField, TComList<TComPic*>& rcListPic);
	Void ETRI_refPicListModification(TComSlice* pcSlice, TComRefPicListModification* refPicListModification, TComList<TComPic*>& rcListPic, Int iGOPid, UInt& uiColDir);
	Void ETRI_NoBackPred_TMVPset(TComSlice* pcSlice, Int iGOPid);
	Void ETRI_setMvdL1ZeroFlag(TComPic* pcPic, TComSlice* pcSlice);
	Void ETRI_RateControlSlice(TComPic* pcPic, TComSlice* pcSlice, int iGOPid, ETRI_SliceInfo& ReturnValue);

	Void ETRI_setCUAddressinFrame(TComPic* pcPic, TComSlice* pcSlice, ETRI_SliceInfo& ReturnValue);
	Void ETRI_EvalCodingOrderMAPandInverseCOMAP(TComPic* pcPic);
	void ETRI_setStartCUAddr(TComSlice* pcSlice, ETRI_SliceInfo& ReturnValue);
	void ETRI_SetNextSlice_with_IF(TComPic* pcPic, TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue);
	void ETRI_LoopFilter(TComPic* pcPic, TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue);

	void ETRI_WriteSeqHeader(TComPic* pcPic, TComSlice*& pcSlice, AccessUnit& accessUnit, Int& actualTotalBits);
	void ETRI_WriteSOPDescriptionInSEI(Int iGOPid, Int pocCurr, TComSlice*& pcSlice, AccessUnit& accessUnit, Bool& writeSOP, Bool isField); 
	void ETRI_setPictureTimingSEI(TComSlice*& pcSlice, SEIPictureTiming& pictureTimingSEI, Int IRAPGOPid, ETRI_SliceInfo& SliceInfo);	
	void ETRI_writeHRDInfo(TComSlice*& pcSlice, AccessUnit& accessUnit, SEIScalableNesting& scalableNestingSEI);						
	void ETRI_Ready4WriteSlice(TComPic* pcPic,  Int pocCurr, TComSlice*& pcSlice, AccessUnit& accessUnit, ETRI_SliceInfo& SliceInfo);
	void ETRI_ReInitSliceData(TComPic* pcPic, TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue);										
	Void ETRI_ResetSliceBoundaryData(TComPic* pcPic, TComSlice*& pcSlice, Bool& skippedSlice, Bool& bRtValue, ETRI_SliceInfo& ReturnValue);
	void ETRI_SetSliceEncoder (TComPic* pcPic, TComSlice*& pcSlice, TComOutputBitstream* pcSubstreamsOut, TComOutputBitstream*& pcBitstreamRedirect, 
										TEncSbac* pcSbacCoders,  OutputNALUnit& nalu, ETRI_SliceInfo& ReturnValue); 					
	void ETRI_WriteOutSlice(TComPic* pcPic, TComSlice*& pcSlice, TComOutputBitstream* pcSubstreamsOut, TComOutputBitstream*& pcBitstreamRedirect, 
						TEncSbac* pcSbacCoders, AccessUnit& accessUnit, OutputNALUnit& nalu, ETRI_SliceInfo& ReturnValue);				

	void ETRI_SAO_Process(TComPic* pcPic, TComSlice* pcSlice, ETRI_SliceInfo& ReturnValue); 											
	void ETRI_WriteOutSEI(TComPic* pcPic, TComSlice*& pcSlice, AccessUnit& accessUnit); 												
	void ETRI_RateControlForGOP(TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue);														
	void ETRI_WriteOutHRDModel(TComSlice*& pcSlice, SEIPictureTiming& pictureTimingSEI, AccessUnit& accessUnit, ETRI_SliceInfo& ReturnValue);

	__inline Void ETRI_UpdateSliceAfterCompression(TComPic*& pcPic, TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue);							

	// -------------------------------------------------------------------------------------------------------------------
	// Compression functions
	// -------------------------------------------------------------------------------------------------------------------
	Void 	ETRI_CompressFrame(Int iTimeOffset, Int pocCurr, TComPic* pcPic, TComPicYuv* pcPicYuvRecOut, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsInGOP);

// 	Void ETRI_UpdateSliceAfterCompression(TComPic*& pcPic, TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue);					
//	void ETRI_xCalculateAddPSNR(TComPic* pcPic, TComList<TComPic*>& rcListPic, AccessUnit& accessUnit, Double dEncTime, Bool isField, Bool isTff); 


};
//! \}
#endif
