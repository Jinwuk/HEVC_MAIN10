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
	\file   	ETRI_HEVC_service.cpp
   	\brief    	Various Service Function such as confirm HW (CPU..) types, messages, high precision timer, and others...
*/

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <emmintrin.h>
#include <tchar.h>

#ifdef _USRDLL
#include "..\..\App\TAppEncoder\TDllEncoder.h"
#endif
#include "ETRI_HEVC_Define.h"


//===========================================================================================
//	ETRI Service Functions on Orther Memory Area seperated to Encoder Core
//===========================================================================================
/**
	@brief: Main Service function. This function is called at Definition of TAppEncTop. i.e. foremost of main Encoder function 
	@Author: JInwuk Seok                    
*/
void ETRI_Service_Init(int iApplicationNameIdx)
{
	char			ETRI_HeaderMsg [ERROR_DLL_BUF_SZ];
	unsigned int	VersionIdx = 0;
	unsigned int	uiLen = 0;

	uiLen = sprintf(ETRI_HeaderMsg    , "\n\n\n");
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t###################################################\n");
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t#                                                 #\n");
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t#      ETRI HEVC %s VER %s #\n", ((iApplicationNameIdx == 0)? "Encoder" : "Decoder"), E265_VERSION);
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t#                                                 #\n");
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t# Contributors :                                  #\n");
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t#   Jinwuk seok     (jnwseok@etri.re.kr)          #\n");
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t#   Younhee Kim     (kimyounhee@etri.re.kr)       #\n");
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t#   Myeongsuk Ki    (serdong@etri.re.kr)          #\n");
#if defined(_DEBUG)
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t#                                Debug Mode       #\n");
#else
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t#                              Release Mode       #\n");
#endif
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t#                           Copyright by ETRI     #\n");
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t#    Compiled @    %s  Time : %s   #\n",  __DATE__, __TIME__);
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t#    Developing VERSION : %s               #\n", E265_DEV_VERSION);	
	uiLen += sprintf(ETRI_HeaderMsg + uiLen, "\t###################################################\n\n");

	fprintf(stderr, "%s", ETRI_HeaderMsg);	
//	_tprintf(_T("%s Len: %d \n"), ETRI_HeaderMsg, uiLen);	///< Check Total number of usage to ETRI_HeaderMsg @ 2015 5 22 by Seok

	memset(ETRI_HeaderMsg, 0, ERROR_DLL_BUF_SZ); uiLen = 0;
	VersionIdx = ETRI_MODIFICATION_V00; 

	switch(VersionIdx)
	{
		case 1 : uiLen = sprintf(ETRI_HeaderMsg, "VERSION : E265 V0  with %s \n",  E265_DEV_VERSION);		break;
		default: uiLen = sprintf(ETRI_HeaderMsg, "VERSION : HM Original  with %s \n",  E265_DEV_VERSION);	break;
	}
	fprintf(stderr, "%s", ETRI_HeaderMsg);	
	memset(ETRI_HeaderMsg, 0, ERROR_DLL_BUF_SZ); uiLen = 0;
	//------------------------------------------------------------------------------------------------------------
	//	Here, HW Check, High Precision Counter and others ....
	//------------------------------------------------------------------------------------------------------------

}






