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

#ifndef	ETRI_HEVC_DEFINE_H
#define ETRI_HEVC_DEFINE_H

#if 0
#ifdef __cplusplus
extern "C" {
#endif
#endif

#include <emmintrin.h>	// SSE 2.0
#include <tmmintrin.h>	// SSE 3.0
#include <smmintrin.h>	// SSE 4.1
#include <nmmintrin.h>	// SSE 4.2	


#include <tchar.h>					//For Debug
#include <direct.h>					//For Debug Path when you use VC IDE

#define	E265_VERSION				"0.1(Based on HM15.1)"	
#define	E265_DEV_VERSION			"[DEV 013]"
//==========================================================================
//	ETRI Debug Tools 
//==========================================================================

#define	ERROR_DLL_BUF_SZ			1024
#define	DEBUGFILEPRPX 				"HEVC_debugTH.txt"
#define	_dbgMsgSIZE   				ERROR_DLL_BUF_SZ	

#if defined(_DEBUG) 
#define	_FILE_DEBUG					1
#define	_GLOBAL_DEBUG  				1
#else 
#define	_FILE_DEBUG					0
#define	_GLOBAL_DEBUG  				0				/// 2013 9 15 by Seok : to stop the Threa Processing
#define	_REALTIME_DEBUG				1
#endif
#if defined(_WIN32) || defined(_WIN64)
#define	_ETRI_WINDOWS_APPLICATION	1
#else
#define	_ETRI_WINDOWS_APPLICATION	0
#endif

#define WHERESTR  "[LINE:%d %s] "
#define WHEREARG  __LINE__, __FUNCTION__ 
#define EDPRINTF(Type,_fmt,...) 	fprintf(Type, WHERESTR _fmt, WHEREARG, __VA_ARGS__);
#define ESPRINTF(Key,Type,_fmt,...) if (Key) {fprintf(Type, WHERESTR _fmt, WHEREARG, __VA_ARGS__);}
#define ETPRINTF(Key,_fmt,...) 		if (Key) {_tprintf(_T(WHERESTR _fmt), WHEREARG, __VA_ARGS__);}

#include <conio.h>
#define	E_PAUSE	{EDPRINTF(stdout, "Push Any Key \n"); _gettch();}

//==========================================================================
//	ETRI Service Tools 
//==========================================================================
#define	CounterPrint(idx) 			if((idx & 0x07) == 0) {fprintf(stderr, ".");}
#define	PrintOnOFF(x)   			((x)? "Active" : "OFF")
#define	ETRI_sABS(x)    			(((x) < 0)? -(x) : (x))
#define	ETRI_sMIN(x, y) 			(((x) < (y))? (x) : (y))
#define	GetBoolVal(x)				((x)? "TRUE":"FALSE")	

#define 	MIN_INT32				-2147483647 	///< max. value of signed 32-bit integer
#define 	ETRI_NULLPOINTER		0xcccccccc
#define	DeleteParam(x)		if(x) {delete x; x = nullptr;}
#define	DeleteDimParam(x)	if(x) {delete[] x; x = nullptr;}

// ====================================================================================================================
// ETRI_Configuration for Development
// ====================================================================================================================
#define	ETRI_TRACE_ENABLE  	  		0				//If 1, Trace is enable: 0, Trace is disable 
#define	ETRI_VERBOSE_RATE  	  		0				//Verbose Rate
// ================================================================================================================
//
// ETRI DLL Interface
//
// ====================================================================================================================
// ETRI_DLL_INTERFACE: 2013.03.26 by Seok
// ETRI_DLL_HeaderWrite: 2013.04.19, by yhee
// ====================================================================================================================
#define	ETRI_DLL_INTERFACE 			0				//Now it is 0,  Hoever, for making E265V01:02 it should be active when DLL Application is decveloped @ 2013 3 26 by Seok
#if ETRI_DLL_INTERFACE
#define	ETRI_DLL_DEBUG 				0
#define	ETRI_EXIT(x)				throw(x)
#define	APP_TYPE  					"DLL_APPLICATION"
#else
#define	ETRI_EXIT(x)				exit(x)
#define	APP_TYPE  					"EXE_APPLICATION"
#endif

//Insert Headers to every IDR period
#define ETRI_WriteIDRHeader			0
#define ETRI_WriteVUIHeader			1

/// Full Release Mode :  1  : Prohibit Any printing of Information 2014 4 11 by Seok 
/// If you want to erase the information of each frame, it shoud be set to 1
#define ETRI_FULL_RELEASEMODE		(1 & ETRI_DLL_INTERFACE)	
#if ETRI_FULL_RELEASEMODE
#define	RLS_TYPE					"[FULL_RELEASE]"
#else
#define	RLS_TYPE					"[INFORMATION]"
#endif

// ================================================================================================================
//
// ETRI Fast Algorithm
//
// ====================================================================================================================
#define ETRI_MODIFICATION_V00 					1
#if  ETRI_MODIFICATION_V00
#define ETRI_RETURN_ORIGINAL					1	///< First Phase of Encoder Work 2015 5 11 by Seok
#endif

#define ETRI_E265_PH01							1	///< For Version Management of Developing E265 @ 2015 5 26 by Seok

// ====================================================================================================================
// ETRI Fast RDOQ
// ====================================================================================================================
#define ETRI_RDOQ_OPTIMIZATION  				0
#if ETRI_RDOQ_OPTIMIZATION
#define ETRI_SCALING_LIST_OPTIMIZATION   		0	///< Now it is 0, But for making E265V01:02 it should be active.
#define ETRI_Remove_Redundant_EntoropyLoader	0	///< Now it is inactive. Hoever, for making E265V01:02 it should be active as Lossless Code Optimization @ 2015 5 12 by Seok
#endif

// ================================================================================================================
// ETRI SW Optimization 
// ================================================================================================================
#define ETRI_SW_Optimization 					ETRI_MODIFICATION_V00
#if ETRI_SW_Optimization
#define ETRI_Remove_Multi_Slice_Coding_Loop    0		///< Now it is 0 and No Implementation. However, for making E265V01:02 it should be active.
#endif

// ================================================================================================================
// ETRI Multi Thread Type
// ================================================================================================================
#define ETRI_MULTITHREAD						1	///< Now No Operation. @ 2015 5 25 by Seok
#define ETRI_MAX_TILES							128 	///< 32 * 4 need to be the same to MAX_NUM_TILES
#if ETRI_MULTITHREAD
#define ETRI_SERIAL_TEST						0


#endif

// ====================================================================================================================
// ETRI_Configuration for Development
// ====================================================================================================================
#define	ETRI_RateControl_Yhee  	  	  			0 	///<  Now it is 0 and No Implementation. porting HM 13.0 RC by yhee 2014.03.14
#if ETRI_RateControl_Yhee
#define	ETRI_RateControl_Frame_Yhee	  			1 	///<& ETRI_RateControl_Yhee // Frame level RC, by yhee 2014.03.28
#define	ETRI_RateControl_LCU_Yhee  				1 	///<& ETRI_RateControl_Yhee // LCU level RC, now removed LCU-level RC, by yhee 2014.03.28
#endif


//========================================================================================================
//	ETRI Service Functions on Orther Memory Area seperated to Encoder Core
//========================================================================================================
 void ETRI_Service_Init(int iApplicationNameIdx);




#endif	//#ifndef	ETRI_HEVC_DEFINE_H
