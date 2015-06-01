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
	\file   	TEncFrame.cpp
   	\brief    	Frame encoder class (Source)
*/

#include "TEncTop.h"
#include "TEncFrame.h"
#include "libmd5/MD5.h"
#include "NALwrite.h"

#include <time.h>

//! \ingroup TLibEncoder
//! \{


// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================
TEncFrame::TEncFrame()
{
	em_pcEncTop      	= nullptr;			
	em_pcSliceEncoder 	= nullptr;

	em_pcGOPEncoder  	= nullptr;		
	em_pcRateCtrl   	= nullptr;
	em_pcLoopFilter  	= nullptr;
	em_pcSAO     		= nullptr;

	em_pcEntropyCoder	= nullptr;		
	em_pcSbacCoder   	= nullptr;
	em_pcBinCABAC   	= nullptr;
	em_pcBitCounter  	= nullptr;

	em_pcBitstreamRedirect = nullptr;
	em_pcAU     		= nullptr;

	em_iMTFrameIdx   	= 0;
	em_iGOPid   		= 0;
	em_iPOCLast   		= 0;
	em_iNumPicRcvd   	= 0;
	em_iNumSubstreams   = 0;
	em_IRAPGOPid    	= 0;		///How about -1 to keep consistency to TEncGOP ? : 2015 5 23 by Seok

	em_bLastGOP 		= false;
	em_bisField  		= false;
	em_bisTff   		= false;

	em_iGopSize   		= 0;
	em_iLastIDR   		= 0;

	accumBitsDU  		= nullptr;
	accumNalsDU 		= nullptr;

	em_dEncTime  		= 0.0;
#if ETRI_DLL_INTERFACE	// 2013 10 23 by Seok
	FrameTypeInGOP		= 0;
	FramePOC			= 0;
	FrameEncodingOrder	= 0;
#endif

}

TEncFrame::~TEncFrame()
{

}

Void TEncFrame::create()
{

}

Void TEncFrame::destroy()
{

}
// ====================================================================================================================
// Data and Coder Setting
// ====================================================================================================================
Void TEncFrame::init(TEncTop* pcCfg, TEncGOP* pcGOPEncoder, Int iFrameIdx)
{
	em_pcEncTop     	= pcCfg;
	em_pcGOPEncoder 	= pcGOPEncoder;
	em_pcSliceEncoder 	= pcGOPEncoder->getSliceEncoder();

	//-------- Encoder filter -------
	em_pcSbacCoder		= pcGOPEncoder->ETRI_getSbacCoder();
	em_pcBinCABAC		= pcGOPEncoder->ETRI_getBinCABAC();
	em_pcEntropyCoder 	= pcGOPEncoder->ETRI_getEntropyCoder();
	em_pcCavlcCoder		= pcGOPEncoder->ETRI_getCavlcCoder();
	em_pcBitCounter		= pcGOPEncoder->ETRI_getBitCounter();		///For SAO Process in Frame Class 2015 5 25 by Seok

	//----- Adaptive Loop filter ----
	em_pcSAO 			= pcGOPEncoder->ETRI_getSAO();
	em_pcLoopFilter		= pcGOPEncoder->ETRI_getLoopFilter();

	em_pcRateCtrl  		= pcGOPEncoder->getRateCtrl();
	em_pcseiWriter 		= pcGOPEncoder->ETRI_getSEIWriter();

//	em_storedStartCUAddrForEncodingSlice = new std::vector<Int>; 		///<P : 2015 5 25 by Seok
//	em_storedStartCUAddrForEncodingSliceSegment = new std::vector<Int>; ///<P : 2015 5 25 by Seok

	em_iMTFrameIdx 	= iFrameIdx;
	em_bFirst			= pcGOPEncoder->ETRI_getbFirst();				///Important Variable :  2015 5 26 by Seok
	em_cpbRemovalDelay = 0;										///Important Variable :  2015 5 26 by Seok


#if ETRI_E265_PH01
	EDPRINTF(stderr, "------------------------------------------ \n");
	EDPRINTF(stderr, " Compiled @%s  [%s] \n\n", __DATE__, __TIME__);
#endif

}

// ====================================================================================================================
// ETRI Compress Frame Functions @ 2015 5 11 by Seok
// ====================================================================================================================
/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Select uiColDir. This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
UInt	TEncFrame::ETRI_Select_UiColDirection(Int iGOPid, UInt	uiColDir)
{
	//select uiColDir
	Int iRef = 0;
	Int iCloseLeft=1, iCloseRight=-1;
	for(Int i = 0; i<em_pcEncTop->getGOPEntry(iGOPid).m_numRefPics; i++)
	{
		iRef = em_pcEncTop->getGOPEntry(iGOPid).m_referencePics[i];
		if  	(iRef>0&&(iRef<iCloseRight||iCloseRight==-1))	{iCloseRight=iRef;}
		else if(iRef<0&&(iRef>iCloseLeft||iCloseLeft==1)) 	{iCloseLeft=iRef;}
	}

	if(iCloseRight>-1){iCloseRight=iCloseRight+em_pcEncTop->getGOPEntry(iGOPid).m_POC-1;}
	if(iCloseLeft<1){
		iCloseLeft=iCloseLeft+em_pcEncTop->getGOPEntry(iGOPid).m_POC-1;
		while(iCloseLeft<0)	{iCloseLeft+=em_iGopSize;}
	}

	Int iLeftQP=0, iRightQP=0;
	for(Int i=0; i<em_iGopSize; i++)
	{
		if(em_pcEncTop->getGOPEntry(i).m_POC==(iCloseLeft%em_iGopSize)+1)
		iLeftQP= em_pcEncTop->getGOPEntry(i).m_QPOffset;

		if (em_pcEncTop->getGOPEntry(i).m_POC==(iCloseRight%em_iGopSize)+1)
		iRightQP=em_pcEncTop->getGOPEntry(i).m_QPOffset;
	}

	return ((iCloseRight>-1&&iRightQP<iLeftQP)? 0 : uiColDir);  	/// Return Value is uiColDir

}

/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Set Default Slice Level data to the same as SPS level  flag. 
	           It is not so important, so as it is possible to remove the codes with ETRI_SCALING_LIST_OPTIMIZATION
	           This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_SliceDataInitialization(TComPic* pcPic, TComSlice* pcSlice)
{
	//set default slice level flag to the same as SPS level flag
	pcSlice->setLFCrossSliceBoundaryFlag(  pcSlice->getPPS()->getLoopFilterAcrossSlicesEnabledFlag()  );
#if !ETRI_SCALING_LIST_OPTIMIZATION
	pcSlice->setScalingList ( em_pcEncTop->getScalingList()	);
	if(em_pcEncTop->getUseScalingListId() == SCALING_LIST_OFF)
	{
		em_pcEncTop->getTrQuant()->setFlatScalingList();
		em_pcEncTop->getTrQuant()->setUseScalingList(false);
		em_pcEncTop->getSPS()->setScalingListPresentFlag(false);
		em_pcEncTop->getPPS()->setScalingListPresentFlag(false);
	}
	else if(em_pcEncTop->getUseScalingListId() == SCALING_LIST_DEFAULT)
	{
		pcSlice->setDefaultScalingList ();
		em_pcEncTop->getSPS()->setScalingListPresentFlag(false);
		em_pcEncTop->getPPS()->setScalingListPresentFlag(false);
		em_pcEncTop->getTrQuant()->setScalingList(pcSlice->getScalingList());
		em_pcEncTop->getTrQuant()->setUseScalingList(true);
	}
	else if(em_pcEncTop->getUseScalingListId() == SCALING_LIST_FILE_READ)
	{
		if(pcSlice->getScalingList()->xParseScalingList(em_pcEncTop->getScalingListFile()))
		{
			pcSlice->setDefaultScalingList ();
		}
		pcSlice->getScalingList()->checkDcOfMatrix();
		em_pcEncTop->getSPS()->setScalingListPresentFlag(pcSlice->checkDefaultScalingList());
		em_pcEncTop->getPPS()->setScalingListPresentFlag(false);
		em_pcEncTop->getTrQuant()->setScalingList(pcSlice->getScalingList());
		em_pcEncTop->getTrQuant()->setUseScalingList(true);
	}
	else
	{
		printf("error : ScalingList == %d no support\n",em_pcEncTop->getUseScalingListId());
		assert(0);
	}
#endif

}



/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Set RPS data for Slice Here.  
			It is so important that, when we make Frame Paralleization, we should fix and add the codes for RPS providing Frame Parallelization
			I mark the location for RPS amenment in this function
	           	This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_SetReferencePictureSetforSlice(TComPic* pcPic, TComSlice* pcSlice, Int iGOPid, Int pocCurr, Bool isField, TComList<TComPic*>& rcListPic)
{

	if(pcSlice->getSliceType()==B_SLICE&&em_pcEncTop->getGOPEntry(iGOPid).m_sliceType=='P')
	{
		pcSlice->setSliceType(P_SLICE);
	}
	if(pcSlice->getSliceType()==B_SLICE&&em_pcEncTop->getGOPEntry(iGOPid).m_sliceType=='I')
	{
		pcSlice->setSliceType(I_SLICE);
	}

	// Set the nal unit type
	pcSlice->setNalUnitType(em_pcGOPEncoder->getNalUnitType(pocCurr, em_iLastIDR, isField));
	if(pcSlice->getTemporalLayerNonReferenceFlag())
	{
		if (pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_TRAIL_R &&
		!(em_iGopSize == 1 && pcSlice->getSliceType() == I_SLICE))
		// Add this condition to avoid POC issues with encoder_intra_main.cfg configuration (see #1127 in bug tracker)
		{
			pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TRAIL_N);
		}
		if(pcSlice->getNalUnitType()==NAL_UNIT_CODED_SLICE_RADL_R)
		{
			pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_RADL_N);
		}
		if(pcSlice->getNalUnitType()==NAL_UNIT_CODED_SLICE_RASL_R)
		{
			pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_RASL_N);
		}
	}

#if EFFICIENT_FIELD_IRAP
#if FIX1172
	if ( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA )  // IRAP picture
	{
		em_associatedIRAPType = pcSlice->getNalUnitType();
		em_associatedIRAPPOC = pocCurr;
	}
	pcSlice->setAssociatedIRAPType(em_associatedIRAPType);
	pcSlice->setAssociatedIRAPPOC(em_associatedIRAPPOC);
#endif
#endif

	// Do decoding refresh marking if any
	pcSlice->decodingRefreshMarking(em_pocCRA, em_bRefreshPending, rcListPic);
	em_pcEncTop->selectReferencePictureSet(pcSlice, pocCurr, iGOPid);
	pcSlice->getRPS()->setNumberOfLongtermPictures(0);
#if EFFICIENT_FIELD_IRAP
#else
#if FIX1172
	if ( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_LP
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_W_RADL
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA_N_LP
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA )  // IRAP picture
	{
	em_associatedIRAPType = pcSlice->getNalUnitType();
	em_associatedIRAPPOC = pocCurr;
	}
	pcSlice->setAssociatedIRAPType(em_associatedIRAPType);
	pcSlice->setAssociatedIRAPPOC(em_associatedIRAPPOC);
#endif
#endif

#if ALLOW_RECOVERY_POINT_AS_RAP
	if ((pcSlice->checkThatAllRefPicsAreAvailable(rcListPic, pcSlice->getRPS(), false, em_iLastRecoveryPicPOC, em_pcEncTop->getDecodingRefreshType() == 3) != 0) || (pcSlice->isIRAP()) 
#if EFFICIENT_FIELD_IRAP
	|| (isField && pcSlice->getAssociatedIRAPType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && pcSlice->getAssociatedIRAPType() <= NAL_UNIT_CODED_SLICE_CRA && pcSlice->getAssociatedIRAPPOC() == pcSlice->getPOC()+1)
#endif
	)
	{
		/*-------------------------------------------------------------------------
			If you want RPS amendment, Please add the code here	@ 2015 5 14 by Seok 
			For example : 
				//ETRI_V10 change, no need by yhee 2013.10.16
				#if (!ETRI_MTFrameRPS && ETRI_MTLevel3Frame)
				if(em_iGOPDepth !=3 || em_bLastGOP){
					pcSlice->createExplicitReferencePictureSetFromReference(rcListPic, pcSlice->getRPS());
				}
				#endif
		---------------------------------------------------------------------------*/
		pcSlice->createExplicitReferencePictureSetFromReference(rcListPic, pcSlice->getRPS(), pcSlice->isIRAP(), em_iLastRecoveryPicPOC, em_pcEncTop->getDecodingRefreshType() == 3);
	}
#else
	if ((pcSlice->checkThatAllRefPicsAreAvailable(rcListPic, pcSlice->getRPS(), false) != 0) || (pcSlice->isIRAP()))
	{
	pcSlice->createExplicitReferencePictureSetFromReference(rcListPic, pcSlice->getRPS(), pcSlice->isIRAP());
	}
#endif
	pcSlice->applyReferencePictureSet(rcListPic, pcSlice->getRPS());

	if(pcSlice->getTLayer() > 0 
	&&  !( pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_N	  // Check if not a leading picture
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL_R
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_N
	|| pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RASL_R )
	)
	{
		if(pcSlice->isTemporalLayerSwitchingPoint(rcListPic) || pcSlice->getSPS()->getTemporalIdNestingFlag())
		{
			if(pcSlice->getTemporalLayerNonReferenceFlag())
			{
				pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TSA_N);
			}
			else
			{
				pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_TSA_R);
			}
		}
		else if(pcSlice->isStepwiseTemporalLayerSwitchingPointCandidate(rcListPic))
		{
			Bool isSTSA=true;
			for(Int ii=iGOPid+1;(ii<em_pcEncTop->getGOPSize() && isSTSA==true);ii++)
			{
				Int lTid= em_pcEncTop->getGOPEntry(ii).m_temporalId;
				if(lTid==pcSlice->getTLayer())
				{
					TComReferencePictureSet* nRPS = pcSlice->getSPS()->getRPSList()->getReferencePictureSet(ii);
					for(Int jj=0;jj<nRPS->getNumberOfPictures();jj++)
					{
						if(nRPS->getUsed(jj))
						{
							Int tPoc=em_pcEncTop->getGOPEntry(ii).m_POC+nRPS->getDeltaPOC(jj);
							Int kk=0;
							for(kk=0;kk<em_pcEncTop->getGOPSize();kk++)
							{
								if(em_pcEncTop->getGOPEntry(kk).m_POC==tPoc)
								break;
							}
							Int tTid=em_pcEncTop->getGOPEntry(kk).m_temporalId;
							if(tTid >= pcSlice->getTLayer())
							{
								isSTSA=false;
								break;
							}
						}
					}
				}
			}
			if(isSTSA==true)
			{
				if(pcSlice->getTemporalLayerNonReferenceFlag())
				{
					pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_N);
				}
				else
				{
					pcSlice->setNalUnitType(NAL_UNIT_CODED_SLICE_STSA_R);
				}
			}
		}
	}

	em_pcGOPEncoder->arrangeLongtermPicturesInRPS(pcSlice, rcListPic);
}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Set RefPicList based on RPS generated by ETRI_SetReferencePictureSetforSlice
			Remenber the RefList for each Picture is Evaluated HERE !!!
	           	This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
Void TEncFrame::ETRI_refPicListModification(TComSlice* pcSlice, TComRefPicListModification* refPicListModification, TComList<TComPic*>& rcListPic, Int iGOPid, UInt& uiColDir)
{

	refPicListModification->setRefPicListModificationFlagL0(0);
	refPicListModification->setRefPicListModificationFlagL1(0);
	pcSlice->setNumRefIdx(REF_PIC_LIST_0,min(em_pcEncTop->getGOPEntry(iGOPid).m_numRefPicsActive,pcSlice->getRPS()->getNumberOfPictures()));
	pcSlice->setNumRefIdx(REF_PIC_LIST_1,min(em_pcEncTop->getGOPEntry(iGOPid).m_numRefPicsActive,pcSlice->getRPS()->getNumberOfPictures()));
	
#if ADAPTIVE_QP_SELECTION
	pcSlice->setTrQuant( em_pcEncTop->getTrQuant() );
#endif
	
	//	Set reference list
	pcSlice->setRefPicList ( rcListPic );
	
	//	Slice info. refinement
	if ( (pcSlice->getSliceType() == B_SLICE) && (pcSlice->getNumRefIdx(REF_PIC_LIST_1) == 0) )
	{
	  pcSlice->setSliceType ( P_SLICE );
	}
	
	if (pcSlice->getSliceType() == B_SLICE)
	{
	  pcSlice->setColFromL0Flag(1-uiColDir);
	  Bool bLowDelay = true;
	  Int  iCurrPOC  = pcSlice->getPOC();
	  Int iRefIdx = 0;
	  
	  for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_0) && bLowDelay; iRefIdx++)
	  {
		if ( pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx)->getPOC() > iCurrPOC )
		{
		  bLowDelay = false;
		}
	  }
	  for (iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(REF_PIC_LIST_1) && bLowDelay; iRefIdx++)
	  {
		if ( pcSlice->getRefPic(REF_PIC_LIST_1, iRefIdx)->getPOC() > iCurrPOC )
		{
		  bLowDelay = false;
		}
	  }
	  
	  pcSlice->setCheckLDC(bLowDelay);
	}
	else
	{
	  pcSlice->setCheckLDC(true);
	}
	
	uiColDir = 1-uiColDir;

}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Set TMVPset based on Evaluated POC for Slice
	           	This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
Void TEncFrame::ETRI_NoBackPred_TMVPset(TComSlice* pcSlice, Int iGOPid)
{
	pcSlice->setRefPOCList();
	pcSlice->setList1IdxToList0Idx();		///L0034_COMBINED_LIST_CLEANUP is Auto Active (Default) @ 2015 5 14 by Seok

	if (em_pcEncTop->getTMVPModeId() == 2)
	{
		if (iGOPid == 0) // first picture in SOP (i.e. forward B)
		{
			pcSlice->setEnableTMVPFlag(0);
		}
		else
		{	// Note: pcSlice->getColFromL0Flag() is assumed to be always 0 and getcolRefIdx() is always 0.
			pcSlice->setEnableTMVPFlag(1);
		}
		pcSlice->getSPS()->setTMVPFlagsPresent(1);
	}
	else if (em_pcEncTop->getTMVPModeId() == 1)
	{
		pcSlice->getSPS()->setTMVPFlagsPresent(1);
		pcSlice->setEnableTMVPFlag(1);
	}
	else
	{
		pcSlice->getSPS()->setTMVPFlagsPresent(0);
		pcSlice->setEnableTMVPFlag(0);
	}
}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Set MVdL1Zero Flag 
	           	This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
Void TEncFrame::ETRI_setMvdL1ZeroFlag(TComPic* pcPic, TComSlice* pcSlice)
{
	Bool bGPBcheck=false;
	if ( pcSlice->getSliceType() == B_SLICE)
	{
		if ( pcSlice->getNumRefIdx(RefPicList( 0 ) ) == pcSlice->getNumRefIdx(RefPicList( 1 ) ) )
		{
			bGPBcheck=true;
			for (Int i=0; i < pcSlice->getNumRefIdx(RefPicList( 1 ) ); i++ )
			{
				if ( pcSlice->getRefPOC(RefPicList(1), i) != pcSlice->getRefPOC(RefPicList(0), i) )
				{
					bGPBcheck=false;
					break;
				}
			}
		}
	}
 
	pcSlice->setMvdL1ZeroFlag(bGPBcheck);
 	pcPic->getSlice(pcSlice->getSliceIdx())->setMvdL1ZeroFlag(pcSlice->getMvdL1ZeroFlag());

}



/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Rate Control For Slice : Alternative Name is ETRI_RateControlLambdaDomain in the Previous Version
	           	This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
Void TEncFrame::ETRI_RateControlSlice(TComPic* pcPic, TComSlice* pcSlice, int iGOPid, ETRI_SliceInfo& ReturnValue)
{
	Double* lambda            = ReturnValue.lambda;
	Int* actualHeadBits      = ReturnValue.actualHeadBits;
	Int* actualTotalBits      = ReturnValue.actualTotalBits;
	Int* estimatedBits        = ReturnValue.estimatedBits;
	Int tmpBitsBeforeWriting = 0;

	*lambda = 0.0; *actualHeadBits = *actualTotalBits = *estimatedBits = 0;

	if ( em_pcEncTop->getUseRateCtrl() )
	{
		Int frameLevel = em_pcRateCtrl->getRCSeq()->getGOPID2Level( iGOPid );
		if ( pcPic->getSlice(0)->getSliceType() == I_SLICE )
		{
			frameLevel = 0;
		}
		em_pcRateCtrl->initRCPic( frameLevel );
		*estimatedBits = em_pcRateCtrl->getRCPic()->getTargetBits();

		Int sliceQP = em_pcEncTop->getInitialQP();
		if ( ( pcSlice->getPOC() == 0 && em_pcEncTop->getInitialQP() > 0 ) || ( frameLevel == 0 && em_pcEncTop->getForceIntraQP() ) ) // QP is specified
		{
			Int    NumberBFrames = ( em_pcEncTop->getGOPSize() - 1 );
			Double dLambda_scale = 1.0 - Clip3( 0.0, 0.5, 0.05*(Double)NumberBFrames );
			Double dQPFactor     = 0.57*dLambda_scale;
			Int    SHIFT_QP      = 12;
			Int    bitdepth_luma_qp_scale = 0;
			Double qp_temp = (Double) sliceQP + bitdepth_luma_qp_scale - SHIFT_QP;
			*lambda = dQPFactor*pow( 2.0, qp_temp/3.0 );
		}
		else if ( frameLevel == 0 )   // intra case, but use the model
		{
			em_pcSliceEncoder->calCostSliceI(pcPic);
			if ( em_pcEncTop->getIntraPeriod() != 1 )   // do not refine allocated bits for all intra case
			{
				Int bits = em_pcRateCtrl->getRCSeq()->getLeftAverageBits();
				bits = em_pcRateCtrl->getRCPic()->getRefineBitsForIntra( bits );
				if ( bits < 200 ){bits = 200;}
				em_pcRateCtrl->getRCPic()->setTargetBits( bits );
			}

			list<TEncRCPic*> listPreviousPicture = em_pcRateCtrl->getPicList();
			em_pcRateCtrl->getRCPic()->getLCUInitTargetBits();
			*lambda  = em_pcRateCtrl->getRCPic()->estimatePicLambda( listPreviousPicture, pcSlice->getSliceType());
			sliceQP = em_pcRateCtrl->getRCPic()->estimatePicQP( *lambda, listPreviousPicture );
		}
		else    // normal case
		{
			list<TEncRCPic*> listPreviousPicture = em_pcRateCtrl->getPicList();
			*lambda  = em_pcRateCtrl->getRCPic()->estimatePicLambda( listPreviousPicture, pcSlice->getSliceType());
			sliceQP = em_pcRateCtrl->getRCPic()->estimatePicQP( *lambda, listPreviousPicture );
		}

		sliceQP = Clip3( -pcSlice->getSPS()->getQpBDOffsetY(), MAX_QP, sliceQP );
		em_pcRateCtrl->getRCPic()->setPicEstQP( sliceQP );

		em_pcSliceEncoder->resetQP( pcPic, sliceQP, *lambda );
	}

}

/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Set CU Address in Slice or Frame (Picture) This Function is compbined with ETRI_setStartCUAddr
	           	This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
Void TEncFrame::ETRI_setCUAddressinFrame(TComPic* pcPic, TComSlice* pcSlice, ETRI_SliceInfo& ReturnValue)
{
	UInt* uiNumSlices   		= ReturnValue.uiNumSlices;
	UInt* uiInternalAddress 	= ReturnValue.uiInternalAddress;
	UInt* uiExternalAddress 	= ReturnValue.uiExternalAddress;
	UInt* uiPosX  				= ReturnValue.uiPosX;
	UInt* uiPosY  				= ReturnValue.uiPosY;
	UInt* uiWidth   			= ReturnValue.uiWidth;
	UInt* uiHeight   			= ReturnValue.uiHeight;
	UInt* uiRealEndAddress   	= ReturnValue.uiRealEndAddress;

	*uiNumSlices = 1;
	*uiInternalAddress = pcPic->getNumPartInCU()-4;
	*uiExternalAddress = pcPic->getPicSym()->getNumberOfCUsInFrame()-1;
	*uiPosX = ( *uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[*uiInternalAddress] ];
	*uiPosY = ( *uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[*uiInternalAddress] ];
	*uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
	*uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();

	while(*uiPosX>=*uiWidth||*uiPosY>=*uiHeight)
	{
		(*uiInternalAddress)--;
		*uiPosX = ( *uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[*uiInternalAddress] ];
		*uiPosY = ( *uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[*uiInternalAddress] ];
	}
	(*uiInternalAddress)++;
	if(*uiInternalAddress==pcPic->getNumPartInCU())
	{
		*uiInternalAddress = 0;
		(*uiExternalAddress)++;
	}

	*uiRealEndAddress = *uiExternalAddress * pcPic->getNumPartInCU() + *uiInternalAddress;

}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Generating Codig Order Map and Inverse Coding Order Map :
			It is very Important Function for Parallel Processing in HEVC Encoding using WPP and Tiles
	           	This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
Void TEncFrame::ETRI_EvalCodingOrderMAPandInverseCOMAP(TComPic* pcPic)
{
    //generate the Coding Order Map and Inverse Coding Order Map
    UInt uiEncCUAddr, p;
    for(p=0, uiEncCUAddr=0; p<pcPic->getPicSym()->getNumberOfCUsInFrame(); p++, uiEncCUAddr = pcPic->getPicSym()->xCalculateNxtCUAddr(uiEncCUAddr))
    {
      pcPic->getPicSym()->setCUOrderMap(p, uiEncCUAddr);
      pcPic->getPicSym()->setInverseCUOrderMap(uiEncCUAddr, p);
    }
    pcPic->getPicSym()->setCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());
    pcPic->getPicSym()->setInverseCUOrderMap(pcPic->getPicSym()->getNumberOfCUsInFrame(), pcPic->getPicSym()->getNumberOfCUsInFrame());
}
 

/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Generating Start CU Address for Slice 
	           	This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_setStartCUAddr(TComSlice* pcSlice, ETRI_SliceInfo& ReturnValue)
{
	UInt* startCUAddrSliceIdx   		= ReturnValue.startCUAddrSliceIdx; 			
	UInt* startCUAddrSlice	 			= ReturnValue.startCUAddrSlice;   			
	UInt* startCUAddrSliceSegmentIdx	= ReturnValue.startCUAddrSliceSegmentIdx; 	
	UInt* startCUAddrSliceSegment	 	= ReturnValue.startCUAddrSliceSegment;    	
	UInt* nextCUAddr					= ReturnValue.nextCUAddr;

	*startCUAddrSliceIdx 		= 0; 	///used to index "m_uiStoredStartCUAddrForEncodingSlice" containing locations of slice boundaries
	*startCUAddrSlice	 		= 0; 	///used to keep track of current slice's starting CU addr.
	*startCUAddrSliceSegmentIdx = 0; ///used to index "m_uiStoredStartCUAddrForEntropyEncodingSlice" containing locations of slice boundaries
	*startCUAddrSliceSegment	= 0;	///used to keep track of current Dependent slice's starting CU addr.
	*nextCUAddr 				= 0;

	pcSlice->setSliceCurStartCUAddr( *startCUAddrSlice ); 				///Setting "start CU addr" for current slice
	em_storedStartCUAddrForEncodingSlice->clear();

	pcSlice->setSliceSegmentCurStartCUAddr( *startCUAddrSliceSegment ); 	///Setting "start CU addr" for current Dependent slice
	em_storedStartCUAddrForEncodingSliceSegment->clear();

	em_storedStartCUAddrForEncodingSlice->push_back (*nextCUAddr);
	(*startCUAddrSliceIdx)++;
	em_storedStartCUAddrForEncodingSliceSegment->push_back(*nextCUAddr);
	(*startCUAddrSliceSegmentIdx)++;

}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Set Data such as CU Address for compression of Next Slice 
	           	This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_SetNextSlice_with_IF(TComPic* pcPic, TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue)
{
	Bool bNoBinBitConstraintViolated = (!pcSlice->isNextSlice() && !pcSlice->isNextSliceSegment());

	UInt* startCUAddrSlice  			= ReturnValue.startCUAddrSlice;
	UInt* startCUAddrSliceIdx 			= ReturnValue.startCUAddrSliceIdx;
	UInt* startCUAddrSliceSegmentIdx 	= ReturnValue.startCUAddrSliceSegmentIdx;
	UInt* startCUAddrSliceSegment 		= ReturnValue.startCUAddrSliceSegment;
	UInt* uiRealEndAddress  			= ReturnValue.uiRealEndAddress;
	UInt* uiNumSlices 					= ReturnValue.uiNumSlices;
	UInt* nextCUAddr 					= ReturnValue.nextCUAddr;
	
	if (pcSlice->isNextSlice() || (bNoBinBitConstraintViolated && em_pcEncTop->getSliceMode()==FIXED_NUMBER_OF_LCU))
	{
	  *startCUAddrSlice = pcSlice->getSliceCurEndCUAddr();
	  // Reconstruction slice
	  em_storedStartCUAddrForEncodingSlice->push_back(*startCUAddrSlice);
	  (*startCUAddrSliceIdx)++;
	  // Dependent slice
	  if (*startCUAddrSliceSegmentIdx>0 && (*em_storedStartCUAddrForEncodingSliceSegment)[(*startCUAddrSliceSegmentIdx) - 1] != *startCUAddrSlice)
	  { 
		em_storedStartCUAddrForEncodingSliceSegment->push_back(*startCUAddrSlice);
		(*startCUAddrSliceSegmentIdx)++;
	  }
	  
	  if (*startCUAddrSlice < *uiRealEndAddress)
	  {
		pcPic->allocateNewSlice();
		pcPic->setCurrSliceIdx					( (*startCUAddrSliceIdx) -1 );
		em_pcSliceEncoder->setSliceIdx			( (*startCUAddrSliceIdx) -1 );
		pcSlice = pcPic->getSlice				( (*startCUAddrSliceIdx) -1 );
		pcSlice->copySliceInfo					( pcPic->getSlice(0)	);
		pcSlice->setSliceIdx					( (*startCUAddrSliceIdx) -1 );
		pcSlice->setSliceCurStartCUAddr			( *startCUAddrSlice	);
		pcSlice->setSliceSegmentCurStartCUAddr	( *startCUAddrSlice	);
		pcSlice->setSliceBits(0);
		(*uiNumSlices) ++;
	  }
	}
	else if (pcSlice->isNextSliceSegment() || (bNoBinBitConstraintViolated && em_pcEncTop->getSliceSegmentMode()==FIXED_NUMBER_OF_LCU))
	{
	  *startCUAddrSliceSegment	  = pcSlice->getSliceSegmentCurEndCUAddr();
	  em_storedStartCUAddrForEncodingSliceSegment->push_back(*startCUAddrSliceSegment);
	  (*startCUAddrSliceSegmentIdx)++;
	  pcSlice->setSliceSegmentCurStartCUAddr( *startCUAddrSliceSegment );
	}
	else
	{
	  *startCUAddrSlice  		= pcSlice->getSliceCurEndCUAddr();
	  *startCUAddrSliceSegment 	= pcSlice->getSliceSegmentCurEndCUAddr();
	}
	
	*nextCUAddr = (*startCUAddrSlice > *startCUAddrSliceSegment) ? *startCUAddrSlice : *startCUAddrSliceSegment;

}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Loop Filter and Gather the statistical Information of SAO Process
			In Comparison to the previous Version, this routine looks very simple.
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_LoopFilter(TComPic* pcPic, TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue)
{
    // SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
    if( pcSlice->getSPS()->getUseSAO() && em_pcEncTop->getSaoLcuBoundary()){
		em_pcSAO->getPreDBFStatistics(pcPic);
	}

    //-- Loop filter
    Bool bLFCrossTileBoundary = pcSlice->getPPS()->getLoopFilterAcrossTilesEnabledFlag();
    em_pcLoopFilter->setCfg(bLFCrossTileBoundary);
    if ( em_pcEncTop->getDeblockingFilterMetric() )
    {
      em_pcGOPEncoder->dblMetric(pcPic, *ReturnValue.uiNumSlices);
    }
    em_pcLoopFilter->loopFilterPic( pcPic );

}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Writeout VPS, SPS, PPS at the First Frame of Sequence. If you want IDR Parallelization, set m_bSeqFirst at the other side of encoder.
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_WriteSeqHeader(TComPic* pcPic, TComSlice*& pcSlice, AccessUnit& accessUnit, Int& actualTotalBits)
{

	// Set entropy coder
	em_pcEntropyCoder->setEntropyCoder	( em_pcCavlcCoder, pcSlice );

	/* write various header sets. */
	if (em_bFirst)
	{
		OutputNALUnit nalu(NAL_UNIT_VPS);
		em_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
		em_pcEntropyCoder->encodeVPS(em_pcEncTop->getVPS());
		writeRBSPTrailingBits(nalu.m_Bitstream);
		accessUnit.push_back(new NALUnitEBSP(nalu));
		actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;

		nalu = NALUnit(NAL_UNIT_SPS);
		em_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

//		if (em_bFirst)
//		{
			UInt	e_numLongTermRefPicSPS   	= em_pcGOPEncoder->ETRI_getnumLongTermRefPicSPS();
			UInt*	e_ltRefPicPocLsbSps  		= em_pcGOPEncoder->ETRI_getltRefPicPocLsbSps();			///Total Number of Ref Pic Order is 33. Look at the TEncGOP.h  @2015 5 24 by Seok
			Bool*	e_ltRefPicUsedByCurrPicFlag	= em_pcGOPEncoder->ETRI_getltRefPicUsedByCurrPicFlag();	///Total Number of Ref Pic Order is 33. Look at the TEncGOP.h  @2015 5 24 by Seok

			pcSlice->getSPS()->setNumLongTermRefPicSPS(e_numLongTermRefPicSPS);
			for (Int k = 0; k < e_numLongTermRefPicSPS; k++)
			{
				pcSlice->getSPS()->setLtRefPicPocLsbSps(k, e_ltRefPicPocLsbSps[k]);
				pcSlice->getSPS()->setUsedByCurrPicLtSPSFlag(k, e_ltRefPicUsedByCurrPicFlag[k]);
			}
//		}

		if( em_pcEncTop->getPictureTimingSEIEnabled() || em_pcEncTop->getDecodingUnitInfoSEIEnabled() )
		{
			UInt maxCU = em_pcEncTop->getSliceArgument() >> ( pcSlice->getSPS()->getMaxCUDepth() << 1);
			UInt numDU = ( em_pcEncTop->getSliceMode() == 1 ) ? ( pcPic->getNumCUsInFrame() / maxCU ) : ( 0 );
			if( pcPic->getNumCUsInFrame() % maxCU != 0 || numDU == 0 )
			{
				numDU ++;
			}
			pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->setNumDU( numDU );
			pcSlice->getSPS()->setHrdParameters( em_pcEncTop->getFrameRate(), numDU, em_pcEncTop->getTargetBitrate(), ( em_pcEncTop->getIntraPeriod() > 0 ) );
		}

		if( em_pcEncTop->getBufferingPeriodSEIEnabled() || em_pcEncTop->getPictureTimingSEIEnabled() || em_pcEncTop->getDecodingUnitInfoSEIEnabled() )
		{
			pcSlice->getSPS()->getVuiParameters()->setHrdParametersPresentFlag( true );
		}

		em_pcEntropyCoder->encodeSPS(pcSlice->getSPS());
		writeRBSPTrailingBits(nalu.m_Bitstream);
		accessUnit.push_back(new NALUnitEBSP(nalu));
		actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;

		nalu = NALUnit(NAL_UNIT_PPS);
		em_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
		em_pcEntropyCoder->encodePPS(pcSlice->getPPS());
		writeRBSPTrailingBits(nalu.m_Bitstream);
		accessUnit.push_back(new NALUnitEBSP(nalu));
		actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;

		em_pcGOPEncoder->xCreateLeadingSEIMessages(accessUnit, pcSlice->getSPS());

//		em_pcGOPEncoder->ETRI_getbFirst() = false;
	}

}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Writeout SOP in SEI at the First part of GOP.  
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_WriteSOPDescriptionInSEI(Int iGOPid, Int pocCurr, TComSlice*& pcSlice, AccessUnit& accessUnit, Bool& writeSOP, Bool isField)
{
    if (writeSOP) // write SOP description SEI (if enabled) at the beginning of GOP
    {
      Int SOPcurrPOC = pocCurr;
      
      OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
      em_pcEntropyCoder->setEntropyCoder(em_pcCavlcCoder, pcSlice);
      em_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
      
      SEISOPDescription SOPDescriptionSEI;
      SOPDescriptionSEI.m_sopSeqParameterSetId = pcSlice->getSPS()->getSPSId();
      
      UInt i = 0;
      UInt prevEntryId = iGOPid;
      for (UInt j = iGOPid; j < em_iGopSize; j++)
      {
        Int deltaPOC = em_pcEncTop->getGOPEntry(j).m_POC - em_pcEncTop->getGOPEntry(prevEntryId).m_POC;
        if ((SOPcurrPOC + deltaPOC) < em_pcEncTop->getFramesToBeEncoded())
        {
          SOPcurrPOC += deltaPOC;
          SOPDescriptionSEI.m_sopDescVclNaluType[i] = em_pcGOPEncoder->getNalUnitType(SOPcurrPOC, em_iLastIDR, isField);
          SOPDescriptionSEI.m_sopDescTemporalId[i] = em_pcEncTop->getGOPEntry(j).m_temporalId;
          SOPDescriptionSEI.m_sopDescStRpsIdx[i] = em_pcEncTop->getReferencePictureSetIdxForSOP(pcSlice, SOPcurrPOC, j);
          SOPDescriptionSEI.m_sopDescPocDelta[i] = deltaPOC;
          
          prevEntryId = j;
          i++;
        }
      }
      
      SOPDescriptionSEI.m_numPicsInSopMinus1 = i - 1;
      
      em_pcseiWriter->writeSEImessage( nalu.m_Bitstream, SOPDescriptionSEI, pcSlice->getSPS());
      writeRBSPTrailingBits(nalu.m_Bitstream);
      accessUnit.push_back(new NALUnitEBSP(nalu));
      
      writeSOP = false;
    }
}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Set Picture Timing Information in SEI 
			CpbRemovalDelay is Evaluate Here Correctly. Look at the code about pictureTimingSEI
			This code can hinder the Frame Parallelism, if this information is write out to all header of pictures.
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_setPictureTimingSEI(TComSlice*& pcSlice, SEIPictureTiming& pictureTimingSEI, Int IRAPGOPid, ETRI_SliceInfo& SliceInfo)
{
	if( ( em_pcEncTop->getPictureTimingSEIEnabled() || em_pcEncTop->getDecodingUnitInfoSEIEnabled() ) &&
	( pcSlice->getSPS()->getVuiParametersPresentFlag() ) &&
	( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() )
	|| ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) )
	{
		if( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getSubPicCpbParamsPresentFlag() )
		{
			UInt numDU = pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNumDU();
			pictureTimingSEI.m_numDecodingUnitsMinus1     = ( numDU - 1 );
			pictureTimingSEI.m_duCommonCpbRemovalDelayFlag = false;

			if( pictureTimingSEI.m_numNalusInDuMinus1 == NULL )
			{
				pictureTimingSEI.m_numNalusInDuMinus1       = new UInt[ numDU ];
			}
			if( pictureTimingSEI.m_duCpbRemovalDelayMinus1  == NULL )
			{
				pictureTimingSEI.m_duCpbRemovalDelayMinus1  = new UInt[ numDU ];
			}
			if( SliceInfo.accumBitsDU == NULL )
			{
				SliceInfo.accumBitsDU                                  = new UInt[ numDU ];
			}
			if( SliceInfo.accumNalsDU == NULL )
			{
				SliceInfo.accumNalsDU                                  = new UInt[ numDU ];
			}
		}

		UInt 	e_totalCoded	= em_pcGOPEncoder->ETRI_gettotalCoded();
		UInt  	e_lastBPSEI  	= em_pcGOPEncoder->ETRI_getlastBPSEI();

		pictureTimingSEI.m_auCpbRemovalDelay = std::min<Int>(std::max<Int>(1, e_totalCoded - e_lastBPSEI), static_cast<Int>(pow(2, static_cast<double>(pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getCpbRemovalDelayLengthMinus1()+1)))); // Syntax element signalled as minus, hence the .
		pictureTimingSEI.m_picDpbOutputDelay = pcSlice->getSPS()->getNumReorderPics(pcSlice->getSPS()->getMaxTLayers()-1) + pcSlice->getPOC() - e_totalCoded;
#if EFFICIENT_FIELD_IRAP
		// if pictures have been swapped there is likely one more picture delay on their tid. Very rough approximation
		if(IRAPGOPid > 0 && IRAPGOPid < em_iGopSize)
		{
			pictureTimingSEI.m_picDpbOutputDelay ++;
		}
#endif
		Int factor = pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getTickDivisorMinus2() + 2;
		pictureTimingSEI.m_picDpbOutputDuDelay = factor * pictureTimingSEI.m_picDpbOutputDelay;
		if( em_pcEncTop->getDecodingUnitInfoSEIEnabled() )
		{
			*SliceInfo.picSptDpbOutputDuDelay = factor * pictureTimingSEI.m_picDpbOutputDelay;
		}
	}

}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Set HRD Information in VUI
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_writeHRDInfo(TComSlice*& pcSlice, AccessUnit& accessUnit, SEIScalableNesting& scalableNestingSEI)
{
	UInt j;

	if( ( em_pcEncTop->getBufferingPeriodSEIEnabled() ) && ( pcSlice->getSliceType() == I_SLICE ) &&
	( pcSlice->getSPS()->getVuiParametersPresentFlag() ) &&
	( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() )
	|| ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) )
	{
		OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
		em_pcEntropyCoder->setEntropyCoder(em_pcCavlcCoder, pcSlice);
		em_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

		SEIBufferingPeriod sei_buffering_period;

		UInt uiInitialCpbRemovalDelay = (90000/2);                      // 0.5 sec
		sei_buffering_period.m_initialCpbRemovalDelay      [0][0]     = uiInitialCpbRemovalDelay;
		sei_buffering_period.m_initialCpbRemovalDelayOffset[0][0]     = uiInitialCpbRemovalDelay;
		sei_buffering_period.m_initialCpbRemovalDelay      [0][1]     = uiInitialCpbRemovalDelay;
		sei_buffering_period.m_initialCpbRemovalDelayOffset[0][1]     = uiInitialCpbRemovalDelay;

		Double dTmp = (Double)pcSlice->getSPS()->getVuiParameters()->getTimingInfo()->getNumUnitsInTick() / (Double)pcSlice->getSPS()->getVuiParameters()->getTimingInfo()->getTimeScale();

		UInt uiTmp = (UInt)( dTmp * 90000.0 );
		uiInitialCpbRemovalDelay -= uiTmp;
		uiInitialCpbRemovalDelay -= uiTmp / ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getTickDivisorMinus2() + 2 );
		sei_buffering_period.m_initialAltCpbRemovalDelay      [0][0]  = uiInitialCpbRemovalDelay;
		sei_buffering_period.m_initialAltCpbRemovalDelayOffset[0][0]  = uiInitialCpbRemovalDelay;
		sei_buffering_period.m_initialAltCpbRemovalDelay      [0][1]  = uiInitialCpbRemovalDelay;
		sei_buffering_period.m_initialAltCpbRemovalDelayOffset[0][1]  = uiInitialCpbRemovalDelay;

		sei_buffering_period.m_rapCpbParamsPresentFlag              = 0;
		//for the concatenation, it can be set to one during splicing.
		sei_buffering_period.m_concatenationFlag = 0;
		//since the temporal layer HRD is not ready, we assumed it is fixed
		sei_buffering_period.m_auCpbRemovalDelayDelta = 1;
		sei_buffering_period.m_cpbDelayOffset = 0;
		sei_buffering_period.m_dpbDelayOffset = 0;

		em_pcseiWriter->writeSEImessage( nalu.m_Bitstream, sei_buffering_period, pcSlice->getSPS());
		writeRBSPTrailingBits(nalu.m_Bitstream);

		Bool e_activeParameterSetSEIPresentInAU = em_pcGOPEncoder->ETRI_getactiveParameterSetSEIPresentInAU();
		Bool e_pictureTimingSEIPresentInAU = em_pcGOPEncoder->ETRI_getpictureTimingSEIPresentInAU();
		{
			UInt seiPositionInAu = em_pcGOPEncoder->xGetFirstSeiLocation(accessUnit);
			UInt offsetPosition = e_activeParameterSetSEIPresentInAU;   // Insert BP SEI after APS SEI
			AccessUnit::iterator it;
			for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
			{
				it++;
			}
			accessUnit.insert(it, new NALUnitEBSP(nalu));
			em_pcGOPEncoder->ETRI_getbufferingPeriodSEIPresentInAU() = true;
		}

		if (em_pcEncTop->getScalableNestingSEIEnabled())
		{
			OutputNALUnit naluTmp(NAL_UNIT_PREFIX_SEI);
			em_pcEntropyCoder->setEntropyCoder(em_pcCavlcCoder, pcSlice);
			em_pcEntropyCoder->setBitstream(&naluTmp.m_Bitstream);
			scalableNestingSEI.m_nestedSEIs.clear();
			scalableNestingSEI.m_nestedSEIs.push_back(&sei_buffering_period);
			em_pcseiWriter->writeSEImessage( naluTmp.m_Bitstream, scalableNestingSEI, pcSlice->getSPS());
			writeRBSPTrailingBits(naluTmp.m_Bitstream);
			UInt seiPositionInAu = em_pcGOPEncoder->xGetFirstSeiLocation(accessUnit);
			UInt offsetPosition = e_activeParameterSetSEIPresentInAU + em_pcGOPEncoder->ETRI_getbufferingPeriodSEIPresentInAU() + e_pictureTimingSEIPresentInAU;   // Insert BP SEI after non-nested APS, BP and PT SEIs
			AccessUnit::iterator it;
			for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
			{
				it++;
			}
			accessUnit.insert(it, new NALUnitEBSP(naluTmp));
			em_pcGOPEncoder->ETRI_getnestedBufferingPeriodSEIPresentInAU() = true;
		}
		em_pcGOPEncoder->ETRI_getlastBPSEI() = em_pcGOPEncoder->ETRI_gettotalCoded();	///In TEncGOP : m_lastBPSEI = m_totalCoded @ 2015 5 24 by Seok
		em_cpbRemovalDelay = 0;
	}
	em_cpbRemovalDelay += 1;
}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Gradual decoding refresh SEI  and Ready for Write out Slice
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_Ready4WriteSlice(TComPic* pcPic,  Int pocCurr, TComSlice*& pcSlice, AccessUnit& accessUnit, ETRI_SliceInfo& SliceInfo)
{
	if( ( em_pcEncTop->getRecoveryPointSEIEnabled() ) && ( pcSlice->getSliceType() == I_SLICE ) )
	{
		if( em_pcEncTop->getGradualDecodingRefreshInfoEnabled() && !pcSlice->getRapPicFlag() )
		{
			// Gradual decoding refresh SEI
			OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
			em_pcEntropyCoder->setEntropyCoder(em_pcCavlcCoder, pcSlice);
			em_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

			SEIGradualDecodingRefreshInfo seiGradualDecodingRefreshInfo;
			seiGradualDecodingRefreshInfo.m_gdrForegroundFlag = true; // Indicating all "foreground"

			em_pcseiWriter->writeSEImessage( nalu.m_Bitstream, seiGradualDecodingRefreshInfo, pcSlice->getSPS() );
			writeRBSPTrailingBits(nalu.m_Bitstream);
			accessUnit.push_back(new NALUnitEBSP(nalu));
		}
		// Recovery point SEI
		OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
		em_pcEntropyCoder->setEntropyCoder(em_pcCavlcCoder, pcSlice);
		em_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

		SEIRecoveryPoint sei_recovery_point;
		sei_recovery_point.m_recoveryPocCnt    = 0;
		sei_recovery_point.m_exactMatchingFlag = ( pcSlice->getPOC() == 0 ) ? (true) : (false);
		sei_recovery_point.m_brokenLinkFlag    = false;
#if ALLOW_RECOVERY_POINT_AS_RAP
		if(em_pcEncTop->getDecodingRefreshType() == 3)
		{
			em_pcGOPEncoder->ETRI_getiLastRecoveryPicPOC() = pocCurr;
		}
#endif
		em_pcseiWriter->writeSEImessage( nalu.m_Bitstream, sei_recovery_point, pcSlice->getSPS() );
		writeRBSPTrailingBits(nalu.m_Bitstream);
		accessUnit.push_back(new NALUnitEBSP(nalu));
	}

	/* use the main bitstream buffer for storing the marshalled picture */
	em_pcEntropyCoder->setBitstream(NULL);

	*SliceInfo.startCUAddrSliceIdx = 0;
	*SliceInfo.startCUAddrSlice    = 0;

	*SliceInfo.startCUAddrSliceSegmentIdx = 0;
	*SliceInfo.startCUAddrSliceSegment    = 0;
	*SliceInfo.nextCUAddr                 = 0;
	pcSlice = pcPic->getSlice(*SliceInfo.startCUAddrSliceIdx);

}



/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Slice Data Re-Initilization for Encode Slice, Especially, SetRPS is conducted in this function. 
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_ReInitSliceData(TComPic* pcPic, TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue)
{
	UInt* 	nextCUAddr 			= ReturnValue.nextCUAddr;
	UInt*	startCUAddrSliceIdx	= ReturnValue.startCUAddrSliceIdx;
	UInt*	startCUAddrSliceSegmentIdx = ReturnValue.startCUAddrSliceSegmentIdx;

	  pcSlice->setNextSlice 	  ( false ); 
	  pcSlice->setNextSliceSegment( false );
	  if (*nextCUAddr == (*em_storedStartCUAddrForEncodingSlice)[*startCUAddrSliceIdx])
	  {
		pcSlice = pcPic->getSlice(*startCUAddrSliceIdx);
		if(*startCUAddrSliceIdx > 0 && pcSlice->getSliceType()!= I_SLICE)
		{
		  pcSlice->checkColRefIdx(*startCUAddrSliceIdx, pcPic);
		}
		pcPic->setCurrSliceIdx(*startCUAddrSliceIdx);
		em_pcSliceEncoder->setSliceIdx(*startCUAddrSliceIdx);
		assert(*startCUAddrSliceIdx == pcSlice->getSliceIdx());
		// Reconstruction slice
		pcSlice->setSliceCurStartCUAddr( *nextCUAddr );	// to be used in encodeSlice() + context restriction
		pcSlice->setSliceCurEndCUAddr  ( (*em_storedStartCUAddrForEncodingSlice)[*startCUAddrSliceIdx + 1 ] );
		// Dependent slice
		pcSlice->setSliceSegmentCurStartCUAddr( *nextCUAddr );  // to be used in encodeSlice() + context restriction
		pcSlice->setSliceSegmentCurEndCUAddr  ( (*em_storedStartCUAddrForEncodingSliceSegment)[*startCUAddrSliceSegmentIdx + 1 ] );
		
		pcSlice->setNextSlice		( true );
		
		(*startCUAddrSliceIdx)++;
		(*startCUAddrSliceSegmentIdx)++;
	  }
	  else if (*nextCUAddr == (*em_storedStartCUAddrForEncodingSliceSegment)[*startCUAddrSliceSegmentIdx])
	  {
		// Dependent slice
		pcSlice->setSliceSegmentCurStartCUAddr( *nextCUAddr );  // to be used in encodeSlice() + context restriction
		pcSlice->setSliceSegmentCurEndCUAddr  ( (*em_storedStartCUAddrForEncodingSliceSegment)[*startCUAddrSliceSegmentIdx+1 ] );
		
		pcSlice->setNextSliceSegment( true );
		
		(*startCUAddrSliceSegmentIdx)++;
	  }
	  
	  pcSlice->setRPS(pcPic->getSlice(0)->getRPS());
	  pcSlice->setRPSidx(pcPic->getSlice(0)->getRPSidx());

}




/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Reset Slice Boundary Data. 
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
Void TEncFrame::ETRI_ResetSliceBoundaryData(TComPic* pcPic, TComSlice*& pcSlice, Bool& skippedSlice, Bool& bRtValue, ETRI_SliceInfo& ReturnValue)
{
	  UInt uiDummyStartCUAddr, uiDummyBoundingCUAddr;

	  UInt* uiInternalAddress		  	= ReturnValue.uiInternalAddress;
	  UInt* uiExternalAddress  			= ReturnValue.uiExternalAddress;
	  UInt* uiPosX					  	= ReturnValue.uiPosX;
	  UInt* uiPosY					 	= ReturnValue.uiPosY;
	  UInt* uiWidth 			  		= ReturnValue.uiWidth;
	  UInt* uiHeight			  		= ReturnValue.uiHeight;
	  UInt* nextCUAddr  				= ReturnValue.nextCUAddr;
	  UInt* startCUAddrSliceIdx 	  	= ReturnValue.startCUAddrSliceIdx;
	  UInt* startCUAddrSliceSegmentIdx 	= ReturnValue.startCUAddrSliceSegmentIdx;

	  em_pcSliceEncoder->xDetermineStartAndBoundingCUAddr(uiDummyStartCUAddr,uiDummyBoundingCUAddr,pcPic,true);

	  *uiInternalAddress = pcPic->getPicSym()->getPicSCUAddr(pcSlice->getSliceSegmentCurEndCUAddr()-1) % pcPic->getNumPartInCU();
	  *uiExternalAddress = pcPic->getPicSym()->getPicSCUAddr(pcSlice->getSliceSegmentCurEndCUAddr()-1) / pcPic->getNumPartInCU();
	  *uiPosX = ( *uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[*uiInternalAddress] ];
	  *uiPosY = ( *uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[*uiInternalAddress] ];
	  *uiWidth = pcSlice->getSPS()->getPicWidthInLumaSamples();
	  *uiHeight = pcSlice->getSPS()->getPicHeightInLumaSamples();

	  while(*uiPosX >= *uiWidth||*uiPosY >= *uiHeight)
	  {
		(*uiInternalAddress)--;
		*uiPosX = ( *uiExternalAddress % pcPic->getFrameWidthInCU() ) * g_uiMaxCUWidth+ g_auiRasterToPelX[ g_auiZscanToRaster[*uiInternalAddress] ];
		*uiPosY = ( *uiExternalAddress / pcPic->getFrameWidthInCU() ) * g_uiMaxCUHeight+ g_auiRasterToPelY[ g_auiZscanToRaster[*uiInternalAddress] ];
	  }

	  (*uiInternalAddress)++;
	  if(*uiInternalAddress==pcPic->getNumPartInCU())
	  {
		*uiInternalAddress = 0;
		*uiExternalAddress = pcPic->getPicSym()->getCUOrderMap(pcPic->getPicSym()->getInverseCUOrderMap(*uiExternalAddress)+1);
	  }

	  UInt endAddress = pcPic->getPicSym()->getPicSCUEncOrder(*uiExternalAddress * pcPic->getNumPartInCU()+ *uiInternalAddress);
	  if(endAddress<=pcSlice->getSliceSegmentCurStartCUAddr())
	  {
		UInt boundingAddrSlice, boundingAddrSliceSegment;
		boundingAddrSlice		   	= (*em_storedStartCUAddrForEncodingSlice)[*startCUAddrSliceIdx];
		boundingAddrSliceSegment 	= (*em_storedStartCUAddrForEncodingSliceSegment)[*startCUAddrSliceSegmentIdx];
		*nextCUAddr   			= min(boundingAddrSlice, boundingAddrSliceSegment);
		if(pcSlice->isNextSlice())
		{
		  skippedSlice=true;
		}
		bRtValue = true;	return;
	  }

	  if(skippedSlice)
	  {
		pcSlice->setNextSlice		( true );
		pcSlice->setNextSliceSegment( false );
	  }
	  skippedSlice=false;

	bRtValue = false; 	/// return Value
}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Set Slice Encoder. Set Substream, CAVLC, CABAC Coder and Bitstream for Slice
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_SetSliceEncoder (TComPic* pcPic, TComSlice*& pcSlice, TComOutputBitstream* pcSubstreamsOut, TComOutputBitstream*& pcBitstreamRedirect, 
									TEncSbac* pcSbacCoders,  OutputNALUnit& nalu, ETRI_SliceInfo& ReturnValue)
{
	Int*  	iNumSubstreams  			= ReturnValue.iNumSubstreams;
	Int*  	tmpBitsBeforeWriting		= ReturnValue.tmpBitsBeforeWriting;
	Int*  	actualHeadBits  			= ReturnValue.actualHeadBits;
	UInt* 	uiOneBitstreamPerSliceLength= ReturnValue.uiOneBitstreamPerSliceLength;


	pcSlice->allocSubstreamSizes( *iNumSubstreams );
	for ( UInt ui = 0 ; ui < *iNumSubstreams; ui++ )
	{
		pcSubstreamsOut[ui].clear();
	}

	em_pcEntropyCoder->setEntropyCoder   ( em_pcCavlcCoder, pcSlice );
	em_pcEntropyCoder->resetEntropy	  ();
	/* start slice NALunit */
	//			OutputNALUnit nalu( pcSlice->getNalUnitType(), pcSlice->getTLayer() );
	Bool sliceSegment = (!pcSlice->isNextSlice());
	if (!sliceSegment)
	{
		*uiOneBitstreamPerSliceLength = 0; // start of a new slice
	}
	em_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);

#if SETTING_NO_OUT_PIC_PRIOR
	pcSlice->setNoRaslOutputFlag(false);
	if (pcSlice->isIRAP())
	{
		if (pcSlice->getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && pcSlice->getNalUnitType() <= NAL_UNIT_CODED_SLICE_IDR_N_LP)
		{
			pcSlice->setNoRaslOutputFlag(true);
		}
		//the inference for NoOutputPriorPicsFlag
		// KJS: This cannot happen at the encoder
		if (! em_bFirst && pcSlice->isIRAP() && pcSlice->getNoRaslOutputFlag())
		{
			if (pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA)
			pcSlice->setNoOutputPriorPicsFlag(true);
		}
	}
#endif

	*tmpBitsBeforeWriting = em_pcEntropyCoder->getNumberOfWrittenBits();
	em_pcEntropyCoder->encodeSliceHeader(pcSlice);
	*actualHeadBits += ( em_pcEntropyCoder->getNumberOfWrittenBits() - *tmpBitsBeforeWriting );

	// is it needed?
	{
		if (!sliceSegment)
		{
			pcBitstreamRedirect->writeAlignOne();
		}
		else
		{
		// We've not completed our slice header info yet, do the alignment later.
		}
#if !ETRI_Remove_Redundant_EntoropyLoader	///<만일 Slice 병렬화에서도 이 부분이 필요 없다면  ! ETRI_Remove_Redundant_EntoropyLoader 하여 해당 코드를 삭제한다. @2015 5 12 by Seok
		em_pcSbacCoder->init( (TEncBinIf*)em_pcBinCABAC );
		em_pcEntropyCoder->setEntropyCoder ( em_pcSbacCoder, pcSlice );
		em_pcEntropyCoder->resetEntropy	  ();
		for ( UInt ui = 0 ; ui < pcSlice->getPPS()->getNumSubstreams() ; ui++ )
		{
			em_pcEntropyCoder->setEntropyCoder ( &pcSbacCoders[ui], pcSlice );
			em_pcEntropyCoder->resetEntropy	();
		}
#endif
	}

	if(pcSlice->isNextSlice())
	{
		// set entropy coder for writing
		em_pcSbacCoder->init( (TEncBinIf*)em_pcBinCABAC );
		{
		for ( UInt ui = 0 ; ui < pcSlice->getPPS()->getNumSubstreams() ; ui++ )
		{
			em_pcEntropyCoder->setEntropyCoder ( &pcSbacCoders[ui], pcSlice );
			em_pcEntropyCoder->resetEntropy	  ();
		}
		pcSbacCoders[0].load(em_pcSbacCoder);
		em_pcEntropyCoder->setEntropyCoder ( &pcSbacCoders[0], pcSlice );	//ALF is written in substream #0 with CABAC coder #0 (see ALF param encoding below)
		}
		em_pcEntropyCoder->resetEntropy	  ();

		// File writing
#if !ETRI_Remove_Redundant_EntoropyLoader	///만일 Slice 병렬화에서도 이 부분이 필요 없다면  ! ETRI_Remove_Redundant_EntoropyLoader 하여 해당 코드를 삭제한다. @2015 5 12 by Seok
		TComOutputBitstream* e_pcActiveBitstream = (sliceSegment)?&nalu.m_Bitstream : pcBitstreamRedirect;
		em_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
#endif
 		// for now, override the TILES_DECODER setting in order to write substreams.
		em_pcEntropyCoder->setBitstream	  ( &pcSubstreamsOut[0] );

	}
	pcSlice->setFinalized(true);


}



/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Writeout Slice Data Including SLice NALU. Espesically, WPP params here.
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_WriteOutSlice(TComPic* pcPic, TComSlice*& pcSlice, TComOutputBitstream* pcSubstreamsOut, TComOutputBitstream*& pcBitstreamRedirect, 
									TEncSbac* pcSbacCoders, AccessUnit& accessUnit, OutputNALUnit& nalu, ETRI_SliceInfo& ReturnValue)
{
	Int*		iNumSubstreams 	= ReturnValue.iNumSubstreams;
	UInt*	uiOneBitstreamPerSliceLength = ReturnValue.uiOneBitstreamPerSliceLength;

	em_pcSbacCoder->load( &pcSbacCoders[0] );

	pcSlice->setTileOffstForMultES( *uiOneBitstreamPerSliceLength );
	pcSlice->setTileLocationCount ( 0 );
	em_pcSliceEncoder->encodeSlice(pcPic, pcSubstreamsOut);

	{
	// Construct the final bitstream by flushing and concatenating substreams.
	// The final bitstream is either nalu.m_Bitstream or pcBitstreamRedirect;
	UInt* puiSubstreamSizes = pcSlice->getSubstreamSizes();
	UInt uiTotalCodedSize = 0; // for padding calcs.
	UInt uiNumSubstreamsPerTile = *iNumSubstreams;

	if (*iNumSubstreams > 1)
		uiNumSubstreamsPerTile /= pcPic->getPicSym()->getNumTiles();

	for ( UInt ui = 0 ; ui < *iNumSubstreams; ui++ )
	{
		// Flush all substreams -- this includes empty ones.
		// Terminating bit and flush.
		em_pcEntropyCoder->setEntropyCoder   ( &pcSbacCoders[ui], pcSlice );
		em_pcEntropyCoder->setBitstream      (  &pcSubstreamsOut[ui] );
		em_pcEntropyCoder->encodeTerminatingBit( 1 );
		em_pcEntropyCoder->encodeSliceFinish();

		pcSubstreamsOut[ui].writeByteAlignment();   // Byte-alignment in slice_data() at end of sub-stream
		// Byte alignment is necessary between tiles when tiles are independent.
		uiTotalCodedSize += pcSubstreamsOut[ui].getNumberOfWrittenBits();

		Bool bNextSubstreamInNewTile = ((ui+1) < *iNumSubstreams)&& ((ui+1)%uiNumSubstreamsPerTile == 0);
		if (bNextSubstreamInNewTile)
		{
			pcSlice->setTileLocation(ui/uiNumSubstreamsPerTile, pcSlice->getTileOffstForMultES()+(uiTotalCodedSize>>3));
		}
		if (ui+1 < pcSlice->getPPS()->getNumSubstreams())
		{
			puiSubstreamSizes[ui] = pcSubstreamsOut[ui].getNumberOfWrittenBits() + (pcSubstreamsOut[ui].countStartCodeEmulations()<<3);
		}
	}

	// Complete the slice header info.
	em_pcEntropyCoder->setEntropyCoder   ( em_pcCavlcCoder, pcSlice );
	em_pcEntropyCoder->setBitstream(&nalu.m_Bitstream);
	em_pcEntropyCoder->encodeTilesWPPEntryPoint( pcSlice );

	// Substreams...
	TComOutputBitstream *pcOut = pcBitstreamRedirect;
	Int offs = 0;
	Int nss = pcSlice->getPPS()->getNumSubstreams();
	if (pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag())
	{
		// 1st line present for WPP.
		offs = pcSlice->getSliceSegmentCurStartCUAddr()/pcSlice->getPic()->getNumPartInCU()/pcSlice->getPic()->getFrameWidthInCU();
		nss  = pcSlice->getNumEntryPointOffsets()+1;
	}

	for ( UInt ui = 0 ; ui < nss; ui++ )
	pcOut->addSubstream(&pcSubstreamsOut[ui+offs]);

	}

	UInt boundingAddrSlice, boundingAddrSliceSegment;
	boundingAddrSlice        = (*em_storedStartCUAddrForEncodingSlice)[*ReturnValue.startCUAddrSliceIdx];
	boundingAddrSliceSegment = (*em_storedStartCUAddrForEncodingSliceSegment)[*ReturnValue.startCUAddrSliceSegmentIdx];
	*ReturnValue.nextCUAddr               = min(boundingAddrSlice, boundingAddrSliceSegment);
	// If current NALU is the first NALU of slice (containing slice header) and more NALUs exist (due to multiple dependent slices) then buffer it.
	// If current NALU is the last NALU of slice and a NALU was buffered, then (a) Write current NALU (b) Update an write buffered NALU at approproate location in NALU list.
	Bool bNALUAlignedWrittenToList    = false; // used to ensure current NALU is not written more than once to the NALU list.
	em_pcGOPEncoder->xAttachSliceDataToNalUnit(nalu, pcBitstreamRedirect);
	accessUnit.push_back(new NALUnitEBSP(nalu));
	*ReturnValue.actualTotalBits += UInt(accessUnit.back()->m_nalUnitData.str().size()) * 8;
	bNALUAlignedWrittenToList = true;
	*uiOneBitstreamPerSliceLength += nalu.m_Bitstream.getNumberOfWrittenBits(); // length of bitstream after byte-alignment

	if (!bNALUAlignedWrittenToList)
	{
		nalu.m_Bitstream.writeAlignZero();
		accessUnit.push_back(new NALUnitEBSP(nalu));
		*uiOneBitstreamPerSliceLength += nalu.m_Bitstream.getNumberOfWrittenBits() + 24; // length of bitstream after byte-alignment + 3 byte startcode 0x000001
	}

	if( ( em_pcEncTop->getPictureTimingSEIEnabled() || em_pcEncTop->getDecodingUnitInfoSEIEnabled() ) &&
	( pcSlice->getSPS()->getVuiParametersPresentFlag() ) &&
	( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() )
	|| ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) &&
	( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getSubPicCpbParamsPresentFlag() ) )
	{
		UInt numNalus = 0;
		UInt numRBSPBytes = 0;
		for (AccessUnit::const_iterator it = accessUnit.begin(); it != accessUnit.end(); it++)
		{
			UInt numRBSPBytes_nal = UInt((*it)->m_nalUnitData.str().size());
			if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
			{
				numRBSPBytes += numRBSPBytes_nal;
				numNalus ++;
			}
		}
		ReturnValue.accumBitsDU[ pcSlice->getSliceIdx() ] = ( numRBSPBytes << 3 );
		ReturnValue.accumNalsDU[ pcSlice->getSliceIdx() ] = numNalus;   // SEI not counted for bit count; hence shouldn't be counted for # of NALUs - only for consistency
	}

}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Writeout Slice Data Including SLice NALU. Espesically, WPP params here.
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_SAO_Process(TComPic* pcPic, TComSlice* pcSlice, ETRI_SliceInfo& ReturnValue)
{
	// set entropy coder for RD
	em_pcEntropyCoder->setEntropyCoder ( em_pcSbacCoder, pcSlice );
	if ( pcSlice->getSPS()->getUseSAO() )
	{
		em_pcEntropyCoder->resetEntropy();
		em_pcEntropyCoder->setBitstream( em_pcBitCounter );
		Bool sliceEnabled[NUM_SAO_COMPONENTS];
		em_pcSAO->initRDOCabacCoder(em_pcEncTop->getRDGoOnSbacCoder(), pcSlice);
		em_pcSAO->SAOProcess(pcPic
		, sliceEnabled
		, pcPic->getSlice(0)->getLambdas()
#if SAO_ENCODE_ALLOW_USE_PREDEBLOCK
		, em_pcEncTop->getSaoLcuBoundary()
#endif
		);
		em_pcSAO->PCMLFDisableProcess(pcPic);   

		//assign SAO slice header
		for(Int s=0; s< *ReturnValue.uiNumSlices; s++)
		{
			pcPic->getSlice(s)->setSaoEnabledFlag(sliceEnabled[SAO_Y]);
			assert(sliceEnabled[SAO_Cb] == sliceEnabled[SAO_Cr]);
			pcPic->getSlice(s)->setSaoEnabledFlagChroma(sliceEnabled[SAO_Cb]);
		}
	}

}


/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Writeout SEI Data and MD5 Check. Specially, DecodedPictureHash, TemporalLevel0Index
	         : This Function is move to the TencFrame Member Functions 
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_WriteOutSEI(TComPic* pcPic, TComSlice*& pcSlice, AccessUnit& accessUnit)
{

	const Char* digestStr = NULL;
	if (em_pcEncTop->getDecodedPictureHashSEIEnabled())
	{
		/* calculate MD5sum for entire reconstructed picture */
		SEIDecodedPictureHash sei_recon_picture_digest;
		if(em_pcEncTop->getDecodedPictureHashSEIEnabled() == 1)
		{
		sei_recon_picture_digest.method = SEIDecodedPictureHash::MD5;
		calcMD5(*pcPic->getPicYuvRec(), sei_recon_picture_digest.digest);
		digestStr = digestToString(sei_recon_picture_digest.digest, 16);
		}
		else if(em_pcEncTop->getDecodedPictureHashSEIEnabled() == 2)
		{
		sei_recon_picture_digest.method = SEIDecodedPictureHash::CRC;
		calcCRC(*pcPic->getPicYuvRec(), sei_recon_picture_digest.digest);
		digestStr = digestToString(sei_recon_picture_digest.digest, 2);
		}
		else if(em_pcEncTop->getDecodedPictureHashSEIEnabled() == 3)
		{
		sei_recon_picture_digest.method = SEIDecodedPictureHash::CHECKSUM;
		calcChecksum(*pcPic->getPicYuvRec(), sei_recon_picture_digest.digest);
		digestStr = digestToString(sei_recon_picture_digest.digest, 4);
		}
		OutputNALUnit nalu(NAL_UNIT_SUFFIX_SEI, pcSlice->getTLayer());

		/* write the SEI messages */
		em_pcEntropyCoder->setEntropyCoder(em_pcCavlcCoder, pcSlice);
		em_pcseiWriter->writeSEImessage(nalu.m_Bitstream, sei_recon_picture_digest, pcSlice->getSPS());
		writeRBSPTrailingBits(nalu.m_Bitstream);

		accessUnit.insert(accessUnit.end(), new NALUnitEBSP(nalu));
	}

	if (em_pcEncTop->getTemporalLevel0IndexSEIEnabled())
	{
		SEITemporalLevel0Index sei_temporal_level0_index;
		em_pcGOPEncoder->ETRI_SetSEITemporalLevel0Index(pcSlice, sei_temporal_level0_index);

		OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);

		/* write the SEI messages */
		em_pcEntropyCoder->setEntropyCoder(em_pcCavlcCoder, pcSlice);
		em_pcseiWriter->writeSEImessage(nalu.m_Bitstream, sei_temporal_level0_index, pcSlice->getSPS());
		writeRBSPTrailingBits(nalu.m_Bitstream);

		/* insert the SEI message NALUnit before any Slice NALUnits */
		AccessUnit::iterator it = find_if(accessUnit.begin(), accessUnit.end(), mem_fun(&NALUnit::isSlice));
		accessUnit.insert(it, new NALUnitEBSP(nalu));
	}


	if (digestStr)
	{
		if(em_pcEncTop->getDecodedPictureHashSEIEnabled() == 1)	{printf(" [MD5:%s]", digestStr);}
		else if(em_pcEncTop->getDecodedPictureHashSEIEnabled() == 2){printf(" [CRC:%s]", digestStr);}
		else if(em_pcEncTop->getDecodedPictureHashSEIEnabled() == 3){printf(" [Checksum:%s]", digestStr);}
	}

}

/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Rate Control For GOP
			This Function should be move to the TencFrame Member Functions, owing to Frame Parallelism
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_RateControlForGOP(TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue)
{
	Int* actualHeadBits	= ReturnValue.actualHeadBits;
	Int* actualTotalBits	= ReturnValue.actualTotalBits;
	Int* estimatedBits	 	= ReturnValue.estimatedBits;
	Double*	lambda		= ReturnValue.lambda;

	if ( em_pcEncTop->getUseRateCtrl() )
	{
		Double avgQP     = em_pcRateCtrl->getRCPic()->calAverageQP();
		Double avgLambda = em_pcRateCtrl->getRCPic()->calAverageLambda();
		if ( avgLambda < 0.0 ){avgLambda = *lambda;}
		
		em_pcRateCtrl->getRCPic()->updateAfterPicture( *actualHeadBits, *actualTotalBits, avgQP, avgLambda, pcSlice->getSliceType());
		em_pcRateCtrl->getRCPic()->addToPictureLsit( em_pcRateCtrl->getPicList() );

		em_pcRateCtrl->getRCSeq()->updateAfterPic( *actualTotalBits );


		Int iBitforRC = (pcSlice->getSliceType() != I_SLICE )? *actualTotalBits: *estimatedBits ;	///for intra picture, the estimated bits are used to update the current status in the GOP
		em_pcRateCtrl->getRCGOP()->updateAfterPicture(iBitforRC);
	}

}

/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Write HRD Model in VUI and SEI. Inaddition, Reset SEI Flags function if needed.
			This Function should be move to the TencFrame Member Functions, owing to Frame Parallelism
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_WriteOutHRDModel(TComSlice*& pcSlice, SEIPictureTiming& pictureTimingSEI, AccessUnit& accessUnit, ETRI_SliceInfo& ReturnValue)
{
	UInt  j;	
	UInt* accumBitsDU = ReturnValue.accumBitsDU;
	UInt* accumNalsDU = ReturnValue.accumNalsDU;

	/*--------------------------------------------------------------------------------------
		HRD condition corresponding to SEI and VUI Parameters 
	--------------------------------------------------------------------------------------*/
	if( ( em_pcEncTop->getPictureTimingSEIEnabled() || em_pcEncTop->getDecodingUnitInfoSEIEnabled() ) &&
	( pcSlice->getSPS()->getVuiParametersPresentFlag() ) &&
	( ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag() )
	|| ( pcSlice->getSPS()->getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag() ) ) )
	{
		TComVUI *vui = pcSlice->getSPS()->getVuiParameters();
		TComHRD *hrd = vui->getHrdParameters();

		/*--------------------------------------------------------------------------------------
			HRD CPB Parameters to SEI 
		--------------------------------------------------------------------------------------*/
		if( hrd->getSubPicCpbParamsPresentFlag() )
		{
			Int i;
			UInt64 ui64Tmp;
			UInt uiPrev = 0;
			UInt numDU = ( pictureTimingSEI.m_numDecodingUnitsMinus1 + 1 );
			UInt *pCRD = &pictureTimingSEI.m_duCpbRemovalDelayMinus1[0];
			UInt maxDiff = ( hrd->getTickDivisorMinus2() + 2 ) - 1;

			for( i = 0; i < numDU; i ++ )
			pictureTimingSEI.m_numNalusInDuMinus1[ i ]       = ( i == 0 ) ? ( accumNalsDU[ i ] - 1 ) : ( accumNalsDU[ i ] - accumNalsDU[ i - 1] - 1 );

			if( numDU == 1 )
			{
				pCRD[ 0 ] = 0; /* don't care */
			}
			else
			{
				pCRD[ numDU - 1 ] = 0;/* by definition */
				UInt tmp = 0;
				UInt accum = 0;

				for( i = ( numDU - 2 ); i >= 0; i -- )
				{
					ui64Tmp = ( ( ( accumBitsDU[ numDU - 1 ]  - accumBitsDU[ i ] ) * ( vui->getTimingInfo()->getTimeScale() / vui->getTimingInfo()->getNumUnitsInTick() ) * ( hrd->getTickDivisorMinus2() + 2 ) ) / ( em_pcEncTop->getTargetBitrate() ) );
					if( (UInt)ui64Tmp > maxDiff )	{
						tmp ++;
					}
				}
				uiPrev = 0;

				UInt flag = 0;
				for( i = ( numDU - 2 ); i >= 0; i -- )
				{
					flag = 0;
					ui64Tmp = ( ( ( accumBitsDU[ numDU - 1 ]  - accumBitsDU[ i ] ) * ( vui->getTimingInfo()->getTimeScale() / vui->getTimingInfo()->getNumUnitsInTick() ) * ( hrd->getTickDivisorMinus2() + 2 ) ) / ( em_pcEncTop->getTargetBitrate() ) );

					if( (UInt)ui64Tmp > maxDiff )
					{
						if(uiPrev >= maxDiff - tmp)
						{
							ui64Tmp = uiPrev + 1;
							flag = 1;
						}
						else ui64Tmp = maxDiff - tmp + 1;
					}

					pCRD[ i ] = (UInt)ui64Tmp - uiPrev - 1;
					if( (Int)pCRD[ i ] < 0 )
					{
						pCRD[ i ] = 0;
					}
					else if (tmp > 0 && flag == 1)
					{
						tmp --;
					}
					accum += pCRD[ i ] + 1;
					uiPrev = accum;
				}
			}
		}


		/*--------------------------------------------------------------------------------------
			Writing HRD Parameters to SEI 
		--------------------------------------------------------------------------------------*/
		Bool	e_activeParameterSetSEIPresentInAU	= em_pcGOPEncoder->ETRI_getactiveParameterSetSEIPresentInAU();
		Bool e_bufferingPeriodSEIPresentInAU    	= em_pcGOPEncoder->ETRI_getbufferingPeriodSEIPresentInAU();
		if( em_pcEncTop->getPictureTimingSEIEnabled() )
		{
			{
			OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI, pcSlice->getTLayer());
			em_pcEntropyCoder->setEntropyCoder(em_pcCavlcCoder, pcSlice);
			pictureTimingSEI.m_picStruct = (em_bisField && pcSlice->getPic()->isTopField())? 1 : em_bisField? 2 : 0;
			em_pcseiWriter->writeSEImessage(nalu.m_Bitstream, pictureTimingSEI, pcSlice->getSPS());
			writeRBSPTrailingBits(nalu.m_Bitstream);
			UInt seiPositionInAu = em_pcGOPEncoder->xGetFirstSeiLocation(accessUnit);
			UInt offsetPosition = e_activeParameterSetSEIPresentInAU	+ e_bufferingPeriodSEIPresentInAU;    // Insert PT SEI after APS and BP SEI
			AccessUnit::iterator it;
			for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
			{
				it++;
			}
			accessUnit.insert(it, new NALUnitEBSP(nalu));
			em_pcGOPEncoder->ETRI_getpictureTimingSEIPresentInAU() = true;
			}
			if ( em_pcEncTop->getScalableNestingSEIEnabled() ) // put picture timing SEI into scalable nesting SEI
			{
				OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI, pcSlice->getTLayer());
				em_pcEntropyCoder->setEntropyCoder(em_pcCavlcCoder, pcSlice);
				em_scalableNestingSEI.m_nestedSEIs.clear();
				em_scalableNestingSEI.m_nestedSEIs.push_back(&pictureTimingSEI);
				em_pcseiWriter->writeSEImessage(nalu.m_Bitstream, em_scalableNestingSEI, pcSlice->getSPS());
				writeRBSPTrailingBits(nalu.m_Bitstream);
				UInt seiPositionInAu = em_pcGOPEncoder->xGetFirstSeiLocation(accessUnit);
				UInt offsetPosition = e_activeParameterSetSEIPresentInAU	+ e_bufferingPeriodSEIPresentInAU 
					+ em_pcGOPEncoder->ETRI_getpictureTimingSEIPresentInAU() 
					+ em_pcGOPEncoder->ETRI_getnestedBufferingPeriodSEIPresentInAU();    // Insert PT SEI after APS and BP SEI
				AccessUnit::iterator it;
				for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
				{
					it++;
				}
				accessUnit.insert(it, new NALUnitEBSP(nalu));
				em_pcGOPEncoder->ETRI_getnestedBufferingPeriodSEIPresentInAU() = true;
			}
		}


		if( em_pcEncTop->getDecodingUnitInfoSEIEnabled() && hrd->getSubPicCpbParamsPresentFlag() )
		{
			em_pcEntropyCoder->setEntropyCoder(em_pcCavlcCoder, pcSlice);
			for( Int i = 0; i < ( pictureTimingSEI.m_numDecodingUnitsMinus1 + 1 ); i ++ )
			{
				OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI, pcSlice->getTLayer());

				SEIDecodingUnitInfo tempSEI;
				tempSEI.m_decodingUnitIdx = i;
				tempSEI.m_duSptCpbRemovalDelay = pictureTimingSEI.m_duCpbRemovalDelayMinus1[i] + 1;
				tempSEI.m_dpbOutputDuDelayPresentFlag = false;
				tempSEI.m_picSptDpbOutputDuDelay = *ReturnValue.picSptDpbOutputDuDelay;

				AccessUnit::iterator it;
				// Insert the first one in the right location, before the first slice
				if(i == 0)
				{
					// Insert before the first slice.
					em_pcseiWriter->writeSEImessage(nalu.m_Bitstream, tempSEI, pcSlice->getSPS());
					writeRBSPTrailingBits(nalu.m_Bitstream);

					UInt seiPositionInAu = em_pcGOPEncoder->xGetFirstSeiLocation(accessUnit);
					UInt offsetPosition = e_activeParameterSetSEIPresentInAU	+ e_bufferingPeriodSEIPresentInAU 
						+ em_pcGOPEncoder->ETRI_getpictureTimingSEIPresentInAU();  // Insert DU info SEI after APS, BP and PT SEI
					for(j = 0, it = accessUnit.begin(); j < seiPositionInAu + offsetPosition; j++)
					{
						it++;
					}
					accessUnit.insert(it, new NALUnitEBSP(nalu));
				}
				else
				{
					Int ctr;
					// For the second decoding unit onwards we know how many NALUs are present
					for (ctr = 0, it = accessUnit.begin(); it != accessUnit.end(); it++)
					{
						if(ctr == accumNalsDU[ i - 1 ])
						{
							// Insert before the first slice.
							em_pcseiWriter->writeSEImessage(nalu.m_Bitstream, tempSEI, pcSlice->getSPS());
							writeRBSPTrailingBits(nalu.m_Bitstream);

							accessUnit.insert(it, new NALUnitEBSP(nalu));
							break;
						}
						if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
						{
							ctr++;
						}
					}
				}
			}
		}
	}

	em_pcGOPEncoder->xResetNonNestedSEIPresentFlags();
	em_pcGOPEncoder->xResetNestedSEIPresentFlags();

}


// ====================================================================================================================
// Frame Compression Auxlilary Functions : inline, Macro, and Data Set 
// ====================================================================================================================
/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Before ETRI_CompressFrame, using the local and member variable in TEncGOP and TEncGOP::TEncCompressGOP
	@author: Jinwuk Seok : 2015 5 23 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
Void TEncFrame::ETRI_setFrameParameter(Int iGOPid,  Int pocLast, Int iNumPicRcvd, Int IRAPGOPid, int iLastIDR, UInt*	uiaccumBitsDU, UInt* uiaccumNalsDU, TComOutputBitstream*& pcBitstreamRedirect, Bool isField, Bool isTff)
{
	em_iGOPid  		= iGOPid;
	em_bisField		= isField;
	em_bisTff 		= isTff;
	em_iPOCLast		= pocLast;
	em_iNumPicRcvd 	= iNumPicRcvd;
	em_IRAPGOPid  	= IRAPGOPid;
	em_iLastIDR		= iLastIDR;		/// set with m_iLastIDR in TEncGOP : 2015 5 23 by Seok 

	//----------------------------------------------------------------------------------------
	em_iGopSize  		= em_pcGOPEncoder->getGOPSize();		
	em_iLastIDR  		= em_pcGOPEncoder->ETRI_getiLastIDR();

	em_associatedIRAPType	= em_pcGOPEncoder->ETRI_getassociatedIRAPType();	///Is it necessary to a member variable ?? and Value to TEncGOP ??	@ 2015 5 24 by Seok
	em_associatedIRAPPOC 	= em_pcGOPEncoder->ETRI_getassociatedIRAPPOC();		///Is it necessary to a member variable ?? and Value to TEncGOP ??	@ 2015 5 24 by Seok
	em_iLastRecoveryPicPOC	= em_pcGOPEncoder->ETRI_getiLastRecoveryPicPOC();

	em_pocCRA   		= em_pcGOPEncoder->ETRI_getpocCRA();
	em_bRefreshPending  = em_pcGOPEncoder->ETRI_getbRefreshPending();			/// Value to TEncGOP ??	@2015 5 24 by Seok

	/// 2015 5 24 by Seok : it's Test. Not Implemeted as this.
	em_storedStartCUAddrForEncodingSlice = em_pcGOPEncoder->ETRI_getstoredStartCUAddrForEncodingSlice();
	em_storedStartCUAddrForEncodingSliceSegment = em_pcGOPEncoder->ETRI_getstoredStartCUAddrForEncodingSliceSegment();

	//----------------------------------------------------------------------------------------
	em_pcBitstreamRedirect = pcBitstreamRedirect;

	em_scalableNestingSEI.m_bitStreamSubsetFlag 			= 1;	 			///If the nested SEI messages are picture buffereing SEI mesages, picure timing SEI messages or sub-picture timing SEI messages, bitstream_subset_flag shall be equal to 1
	em_scalableNestingSEI.m_nestingOpFlag   				= 0;
	em_scalableNestingSEI.m_nestingNumOpsMinus1  			= 0;	 			///nesting_num_ops_minus1
	em_scalableNestingSEI.m_allLayersFlag					= 0;
	em_scalableNestingSEI.m_nestingNoOpMaxTemporalIdPlus1	= 6 + 1;  			///nesting_no_op_max_temporal_id_plus1
	em_scalableNestingSEI.m_nestingNumLayersMinus1   		= 1 - 1;  			///nesting_num_layers_minus1
	em_scalableNestingSEI.m_nestingLayerId[0] 				= 0;
	em_scalableNestingSEI.m_callerOwnsSEIs   				= true;

	accumBitsDU = uiaccumBitsDU;
	accumNalsDU = uiaccumNalsDU;

	//------------------------------ After V0 Revision --------------------------------------
	// These values are updated in compressFrame. 
	// Thus, always get from GOP before compression Frame and  return the updated value to GOP 
	// @ 2015 5 27 by Seok
	//----------------------------------------------------------------------------------------
	em_bFirst			= em_pcGOPEncoder->ETRI_getbFirst();				///Most Serious variable  2015 5 27 by Seok
	em_cpbRemovalDelay = em_pcGOPEncoder->ETRI_getcpbRemovalDelay();		///So So ... 2015 5 27 by Seok

}

/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Some Parameters have to be copied to the GOP member variable
	@author: Jinwuk Seok : 2015 5 23 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
Void TEncFrame::ETRI_ResetFrametoGOPParameter()
{
	em_pcGOPEncoder->ETRI_getcpbRemovalDelay() = em_cpbRemovalDelay;
}

/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Macro and Service Function for ETRI_compressGOP
			This Function should be move to the TencFrame Member Functions, owing to Frame Parallelism
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
//__inline Void TEncFrame::ETRI_UpdateSliceAfterCompression(TComPic*& pcPic, TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue) 
__inline Void TEncFrame::ETRI_UpdateSliceAfterCompression(TComPic*& pcPic, TComSlice*& pcSlice, ETRI_SliceInfo& ReturnValue) 
{
	em_storedStartCUAddrForEncodingSlice->push_back( pcSlice->getSliceCurEndCUAddr()); 
	(*ReturnValue.startCUAddrSliceIdx)++; 
	em_storedStartCUAddrForEncodingSliceSegment->push_back(pcSlice->getSliceCurEndCUAddr()); 
	(*ReturnValue.startCUAddrSliceSegmentIdx)++;

	pcSlice = pcPic->getSlice(0);
}


#define	ETRI_SET_SISLICEINFO(e_sISliceInfo) \
	e_sISliceInfo.actualHeadBits    			= &actualHeadBits; \
	e_sISliceInfo.actualTotalBits    			= &actualTotalBits; \
	e_sISliceInfo.estimatedBits      			= &estimatedBits; \
	e_sISliceInfo.lambda    					= &lambda; \
	e_sISliceInfo.uiNumSlices   				= &uiNumSlices; \
	e_sISliceInfo.uiRealEndAddress   			= &uiRealEndAddress; \
	e_sISliceInfo.uiInternalAddress   			= &uiInternalAddress; \
	e_sISliceInfo.uiExternalAddress   			= &uiExternalAddress; \
	e_sISliceInfo.uiPosX    					= &uiPosX; \
	e_sISliceInfo.uiPosY    					= &uiPosY; \
	e_sISliceInfo.uiWidth   					= &uiWidth; \
	e_sISliceInfo.uiHeight  					= &uiHeight; \
	e_sISliceInfo.iNumSubstreams    			= &iNumSubstreams; \
	e_sISliceInfo.startCUAddrSlice   			= &startCUAddrSlice;\
	e_sISliceInfo.startCUAddrSliceIdx    		= &startCUAddrSliceIdx; \
	e_sISliceInfo.startCUAddrSliceSegment    	= &startCUAddrSliceSegment; \
	e_sISliceInfo.startCUAddrSliceSegmentIdx 	= &startCUAddrSliceSegmentIdx; \
	e_sISliceInfo.nextCUAddr    				= &nextCUAddr; \
	e_sISliceInfo.picSptDpbOutputDuDelay    	= &picSptDpbOutputDuDelay; \
	e_sISliceInfo.accumBitsDU   				= accumBitsDU; \
	e_sISliceInfo.accumNalsDU   				= accumNalsDU; \
	e_sISliceInfo.tmpBitsBeforeWriting   		= &tmpBitsBeforeWriting; \
	e_sISliceInfo.uiOneBitstreamPerSliceLength 	= &uiOneBitstreamPerSliceLength; \
	e_sISliceInfo.picSptDpbOutputDuDelay 		= &picSptDpbOutputDuDelay;


// ====================================================================================================================
// Frame Compression Functions 
// ====================================================================================================================
/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Main 1 Frame Compression Function 
			This Function consists with the Set accessUnit (bin out), Slice Encoder, Slice Compression, Slice Filtering, Write out the Header of Frame (SPS/VPS/PPS/SEI/VUI) and Slice.
	@param: 	Int iTimeOffset 	: Look atthe GOP Compression 
	@param:  Int pocCurr		
	@param:  TComPic* pcPic	: Input Picture (One)
	@param:  TComPicYuv* pcPicYuvRecOut : Recon Picture (One)
	@param:  TComList<TComPic*>& rcListPic : Input Picture Buffer List 
	@param:  TComList<TComPicYuv*>& rcListPicYuvRecOut : Reconstruction Picture Buffer List (For DPB)
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
Void TEncFrame::ETRI_CompressFrame(Int iTimeOffset, Int pocCurr, TComPic* pcPic, TComPicYuv* pcPicYuvRecOut, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsInGOP)
{
	TComSlice*	pcSlice			= nullptr;
	TEncSbac* 	pcSbacCoders 	= nullptr;
	TComOutputBitstream* pcSubstreamsOut = nullptr;

	Bool  	writeSOP = em_pcEncTop->getSOPDescriptionSEIEnabled();
	Int    	picSptDpbOutputDuDelay = 0;
	SEIPictureTiming pictureTimingSEI;

	/// In ETRI_SetSliceEncoder
	UInt   	uiOneBitstreamPerSliceLength = 0;

	/// Parameter for Rate Control
	Double	lambda		   = 0.0;
	Int    	actualHeadBits, actualTotalBits, estimatedBits, tmpBitsBeforeWriting;

	/// Set CU Address for Slice compression 2015 5 14 by Seok
	UInt   	uiNumSlices, uiInternalAddress, uiExternalAddress, uiRealEndAddress; 
	UInt   	uiPosX, uiPosY, uiWidth, uiHeight;

	/// Set CU Address for Slice Processing ... @2 015 5 14 by Seok 
	UInt   	startCUAddrSliceIdx, startCUAddrSlice, startCUAddrSliceSegmentIdx, startCUAddrSliceSegment, nextCUAddr;

	/// Number of Unit to Parallel Processing such as WPP and ... @2 015 5 14 by Seok : Local Variable Used in ETRI_ComressFrame : Remove 
	Int    	iNumSubstreams;

	TComRefPicListModification* refPicListModification = nullptr;

	/// 2015 5 14 by Seok : Remove
	ETRI_SliceInfo	e_sISliceInfo;
	ETRI_SET_SISLICEINFO(e_sISliceInfo);	///Data Set for CompressionGOP using  ETRI_SliceInfo	e_sISliceInfo @ 2015 5 14 by Seok

	//=================================================================================
	// Initial to start encoding
	//=================================================================================
	UInt uiColDir = 1;
	//-- For time output for each slice
	long iBeforeTime = clock();

	uiColDir=ETRI_Select_UiColDirection(em_iGOPid, uiColDir);					/// select uiColDir : 2015 5 11 by Seok

	// start a new access unit: create an entry in the list of output access units
	accessUnitsInGOP.push_back(AccessUnit());
	AccessUnit& accessUnit = accessUnitsInGOP.back();
	em_pcAU = &accessUnit;														///For Interface @2015 5 25 by Seok
	
	//	Slice data initialization
	pcPic->clearSliceBuffer();
	assert(pcPic->getNumAllocatedSlice() == 1);
	em_pcSliceEncoder->setSliceIdx(0);
	pcPic->setCurrSliceIdx(0);
	em_pcSliceEncoder->initEncSlice ( pcPic, em_iPOCLast, pocCurr, em_iNumPicRcvd, em_iGOPid, pcSlice, em_pcEncTop->getSPS(), em_pcEncTop->getPPS(), em_bisField );

	//Set Frame/Field coding
	pcSlice->getPic()->setField(em_bisField);
	pcSlice->setLastIDR(em_iLastIDR);
	pcSlice->setSliceIdx(0);

	refPicListModification = pcSlice->getRefPicListModification();				///Get RefPicList for each Slice (ot Picture) If you want to Slice Paralleization, This Parameter should be multiple allocated. @2015 5 14 by Seok 

	ETRI_SliceDataInitialization(pcPic, pcSlice);
	ETRI_SetReferencePictureSetforSlice(pcPic, pcSlice, em_iGOPid, pocCurr, em_bisField, rcListPic);
	ETRI_refPicListModification(pcSlice, refPicListModification, rcListPic, em_iGOPid, uiColDir);
	ETRI_NoBackPred_TMVPset(pcSlice, em_iGOPid);

	//=================================================================================
	// Slice compression
	//=================================================================================
	if (em_pcEncTop->getUseASR()){em_pcSliceEncoder->setSearchRange(pcSlice);} 	///When you use a Adaptive Search Ramge, Serach Range is set HERE @ 2015 5 14 by Seok

	ETRI_setMvdL1ZeroFlag(pcPic, pcSlice);   	    							///Set MVDL1Zero Flag according to RefPicList @ 2015 5 14 by Seok
	ETRI_RateControlSlice(pcPic, pcSlice, em_iGOPid, e_sISliceInfo);   			///Rate Control for Slice @ 2015 5 14 by Seok
	ETRI_setCUAddressinFrame(pcPic, pcSlice, e_sISliceInfo);   					///Set Fundamental CU Address for Slice Compression @ 2015 5 14 by Seok

	pcPic->getPicSym()->initTiles(pcSlice->getPPS());   						///Tile Initilization @ 2015 5 14 by Seok
	iNumSubstreams = pcSlice->getPPS()->getNumSubstreams();    			    	///Allocate some coders, now we know how many tiles there are.

	ETRI_EvalCodingOrderMAPandInverseCOMAP(pcPic);    	     					///<Generating Coding Order MAP and Inverse MAP; Check Location of FUnction @ 2015 5 14 by Seok

	// Allocate some coders, now we know how many tiles there are.
	em_pcEncTop->createWPPCoders(iNumSubstreams);
	pcSbacCoders = em_pcEncTop->getSbacCoders();

	pcSubstreamsOut = new TComOutputBitstream[iNumSubstreams];

	ETRI_setStartCUAddr(pcSlice, e_sISliceInfo);								/// Ready to Compress Slice @ 2015 5 14 by Seok

	while(nextCUAddr<uiRealEndAddress)   	    					     		///determine slice boundaries : Multiple Slice Encoding is somewhat difficult @ 2015 5 14 by Seok
	{
		pcSlice->setNextSlice		( false );
		pcSlice->setNextSliceSegment( false );
		assert(pcPic->getNumAllocatedSlice() == startCUAddrSliceIdx);
		em_pcSliceEncoder->precompressSlice( pcPic );   						///If  eem_pcEncTop->getDeltaQpRD() = 0, then it is not working : Default Value is 0 @ 2015 5 14 by Seok
		em_pcSliceEncoder->ETRI_compressSlice   (pcPic, ETRI_MODIFICATION_V00);	///Main Compression Function 
		ETRI_SetNextSlice_with_IF(pcPic, pcSlice, e_sISliceInfo);				///Data Update for Next Slice Compression @ 2015 5 14 by Seok
	}
	ETRI_UpdateSliceAfterCompression(pcPic, pcSlice, e_sISliceInfo); 			///startCUAddrSliceIdx and startCUAddrSliceSegmentrIdx are Update @ 2015 5 14 by Seok	
	ETRI_LoopFilter(pcPic, pcSlice, e_sISliceInfo);   							///Loop Filter and Gather the statistics of SAO @ 2015 5 14 by Seok

	//=================================================================================
	// File Writing 
	//=================================================================================
	ETRI_WriteSeqHeader( pcPic, pcSlice, accessUnit, actualTotalBits);			///Writeout VPS, PPS, SPS when m_bSeqFirst= TRUE @ 2015 5 11 by Seok
	ETRI_WriteSOPDescriptionInSEI(em_iGOPid, pocCurr, pcSlice, accessUnit, writeSOP, em_bisField); 	///write SOP description SEI (if enabled) at the beginning of GOP @2015 5 11 by Seok
	ETRI_setPictureTimingSEI(pcSlice, pictureTimingSEI, em_IRAPGOPid, e_sISliceInfo);	///Write out Picture Timing Information in SEI @ 2015 5 11 by Seok
	ETRI_writeHRDInfo(pcSlice, accessUnit, em_scalableNestingSEI);				///WriteOut HRD Information @ 2015 5 11 by Seok
	ETRI_Ready4WriteSlice(pcPic, pocCurr, pcSlice, accessUnit, e_sISliceInfo);	///Ready for Write Out Slice Information through Encode Slice @ 2015 5 12 by Seok

	Int processingState = (pcSlice->getSPS()->getUseSAO())?(EXECUTE_INLOOPFILTER):(ENCODE_SLICE);
	Bool skippedSlice=false, bStopEncodeSlice = false;
	while (nextCUAddr < uiRealEndAddress) // Iterate over all slices
	{
		switch(processingState)
		{
			case ENCODE_SLICE:
			{
				ETRI_ReInitSliceData(pcPic, pcSlice, e_sISliceInfo); 			///Slice Data Re-Initilization for Encode Slice, Especially, SetRPS is conducted in this function. 
				ETRI_ResetSliceBoundaryData(pcPic, pcSlice, skippedSlice, bStopEncodeSlice, e_sISliceInfo); ///Set Slice Boundary Data 
				if (bStopEncodeSlice) {continue;}								/// If bStopEncodeSlice is true, escape the out of Encode Slice
				OutputNALUnit nalu( pcSlice->getNalUnitType(), pcSlice->getTLayer());		///Get New NALU for Slice Information @ 2015 5 12 by Seok
				ETRI_SetSliceEncoder(pcPic, pcSlice, pcSubstreamsOut, em_pcBitstreamRedirect, pcSbacCoders, nalu, e_sISliceInfo);///Set Slice Data to be written to HEVC ES, including Slice NALU @ 2015 5 12 by Seok
				ETRI_WriteOutSlice(pcPic, pcSlice, pcSubstreamsOut, em_pcBitstreamRedirect, pcSbacCoders, accessUnit, nalu, e_sISliceInfo);///Main Encoding Slice Data set in the ETRI_SetSliceEncoder with CAVLC and CABAC @ 2015 5 12 by Seok
				processingState = ENCODE_SLICE;
			}
			break;

			case EXECUTE_INLOOPFILTER:
			{
				ETRI_SAO_Process(pcPic, pcSlice, e_sISliceInfo); 				///HERE, SAO Process isconducted Not Inloop Filter At First SAO and Encode SLice Finally, InLoopFilter @	2015 5 12 by Seok
				processingState = ENCODE_SLICE;
			}
			break;

			default:{printf("Not a supported encoding state\n"); assert(0); exit(-1);}
		}
	} // end iteration over slices
	pcPic->compressMotion();

	//=================================================================================
	// Final Processing (Rate Control/Services/ Recon Pic Copy/PSNR/ HDR Information and others)
	//=================================================================================
	em_dEncTime = (Double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;   			///For time output for each slice
	
	ETRI_WriteOutSEI(pcPic, pcSlice, accessUnit);     								///Write Out SEI Information and MD5 Hash Check @ 2015 5 13 by Seok
	ETRI_RateControlForGOP(pcSlice, e_sISliceInfo);     							///Rate Cobtrol for GOP !!! @ 2015 5 14 by Seok
	ETRI_WriteOutHRDModel(pcSlice, pictureTimingSEI, accessUnit, e_sISliceInfo);   	///HRD Model in VUI and SEI @ 2015 5 14 by Seok
	ETRI_ResetFrametoGOPParameter();									/// Some Important parameters are updated to GOP @ 2015 5 26 by Seok

	pcPic->getPicYuvRec()->copyToPic(pcPicYuvRecOut);
	pcPic->setReconMark   ( true );

	delete[] pcSubstreamsOut;

	/* logging: insert a newline at end of picture period */
//	ETRI_xCalculateAddPSNR(pcPic, rcListPic, accessUnit, dEncTime, em_bisField, em_bisTff);		///Calculate PSNR as the result of Encoding @ 2015 5 14 by Seok
//	printf("\n");	fflush(stdout);

}


//=================================================================================
// Reserved Code 
//=================================================================================
#if 0
/**
------------------------------------------------------------------------------------------------------------------------------------------------
	@brief: Calculate PSNR. Including PSNR calculation for Interlaced Picture, we make other version of xcalculateAddPSNR
			For Parallelism, the Function through em_pcGOPEncoder may be changed and defined in TEncFrame.
	@author: Jinwuk Seok  2015 5 11 
------------------------------------------------------------------------------------------------------------------------------------------------
*/
void TEncFrame::ETRI_xCalculateAddPSNR(TComPic* pcPic, TComList<TComPic*>& rcListPic, AccessUnit& accessUnit, Double dEncTime, Bool isField, Bool isTff)
{
	em_pcGOPEncoder->xCalculateAddPSNR( pcPic, pcPic->getPicYuvRec(), accessUnit, dEncTime );
	
	//In case of field coding, compute the interlaced PSNR for both fields
	if (isField && ((!pcPic->isTopField() && isTff) || (pcPic->isTopField() && !isTff)) && (pcPic->getPOC()%em_iGopSize != 1))
	{
	  //get complementary top field
	  TComPic* pcPicTop;
	  TComList<TComPic*>::iterator	 iterPic = rcListPic.begin();
	  while ((*iterPic)->getPOC() != pcPic->getPOC()-1)
	  {
		iterPic ++;
	  }
	  pcPicTop = *(iterPic);
	  em_pcGOPEncoder->xCalculateInterlacedAddPSNR(pcPicTop, pcPic, pcPicTop->getPicYuvRec(), pcPic->getPicYuvRec(), accessUnit, dEncTime );
	}
	else if (isField && pcPic->getPOC()!= 0 && (pcPic->getPOC()%em_iGopSize == 0))
	{
	  //get complementary bottom field
	  TComPic* pcPicBottom;
	  TComList<TComPic*>::iterator	 iterPic = rcListPic.begin();
	  while ((*iterPic)->getPOC() != pcPic->getPOC()+1)
	  {
		iterPic ++;
	  }
	  pcPicBottom = *(iterPic);
	  em_pcGOPEncoder->xCalculateInterlacedAddPSNR(pcPic, pcPicBottom, pcPic->getPicYuvRec(), pcPicBottom->getPicYuvRec(), accessUnit, dEncTime );
	}

}
#endif





//! \}
