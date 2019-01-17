
/*****************************************************************************
*                                                                            *
*   Copyright (C) 2009 Symmetricom, Inc., all rights reserved                *
*                                                                            *
*   This software contains proprietary and confidential information of       *
*   Symmetricom.                                                             *
*   It may not be reproduced, used, or disclosed to others for any purpose   *
*   without the written authorization of Symmetricom.                        *
*                                                                            *
******************************************************************************

FILE NAME    : clk_acq.c

AUTHOR       : George Zampetti

DESCRIPTION  : 


Revision control header:
$Id: CLK/clk_acq.c 1.8 2011/06/29 10:24:52PDT Kenneth Ho (kho) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
//#define TP500_BUILD
//#define TEMPERATURE_ON
#ifndef TP500_BUILD
#include "proj_inc.h"
#include "target.h"
#include "sc_types.h"
#include "sc_servo_api.h"
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "DBG/DBG.h"
#include "logger.h"
#include "ptp.h"
#include "CLKflt.h"
#include "CLKpriv.h"
#include "clk_private.h"
#define sprintf diag_sprintf

#else
#include "proj_inc.h"
#include <mqx.h>
#include <bsp.h>
#include <rtcs.h>
#include <enet.h>
#include "rtcsdemo.h"
#include <ctype.h>
#include <string.h>
#include "epl.h"
#include "m523xevb.h"
#include "clk_private.h"
#include "ptp.h"
#include "var.h"
#include "math.h"
#include "logger.h"
#include "pps.h"
#include "sc_ptp_servo.h"
#include "sock_manage.h"
#include "CLKpriv.h"
#include "sc_api.h"
#include "temperature.h"
#endif


/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
INT32	stilt,sdrift_prev,sbuf[2],stemp;
static double	tgain=PER_01; //initial gain for tilt filter was PER_05
static double tgain_min=0.0001;
MIN_STRUCT  min_detilt;




/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/


/*
----------------------------------------------------------------------------
                                SC_GetRevInfo()

Description:
This function requests the current revision information.  The revision 
information is copied to the input data buffer as a string.  The string 
contains name value pairs separated by commas.  White space should be 
ignored.  If the buffer is not large enough to contain the entire revision 
string, the output will be truncated to fit the available space.  Output 
will be NULL terminated.  e.g. softclient=1.0a,servo=1.0a,phys=1.2


Parameters:

Inputs
	UINT16 w_lenBuffer
	This parameter passes the buffer length to the function.
	
Outputs:
	INT8 *c_revBuffer, 
	The function will copy the revision string into this pointer location.

Return value:
	None

-----------------------------------------------------------------------------
*/




///////////////////////////////////////////////////////////////////////
// This function detilts the control loop during warmup
///////////////////////////////////////////////////////////////////////
void detilt(void)
{
//	double old_pll_int_f = pll_int_f; /* used to determine delta for warm-up (kjh)*/ 
	int detilt_skip, detilt_fast;
	INT32 loop_mask; 
	/* detilt using first 32768 values */
//	if(enterprise_flag)
//	{
//		loop_mask= 0x1F;
//		detilt_skip=48;
//		detilt_fast=64;
//	}	
	if(dsl_flag)
	{
		loop_mask= 0x7F;
		detilt_skip=256;
		detilt_fast=512;
	}	
	else
	{
		loop_mask= 0x3F;
		detilt_skip=140; //was 256
		detilt_fast=512;
	}	
	
	// get last good freq //
	Get_Freq_Corr(&last_good_freq,&gaptime);
	if(1) //always use for now
//	if(gaptime < 3000000)
	{
		if(	FLL_Elapse_Sec_PTP_Count > detilt_skip&&FLL_Elapse_Sec_PTP_Count < detilt_fast)
		{
//			pll_int_r=pll_int_f= (double)(last_good_freq);	
			fdrift_f=fdrift_r=fdrift=fdrift_raw=fdrift_warped=fdrift_smooth=pll_int_r=pll_int_f= holdover_f=holdover_r= (double)(last_good_freq);
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTP_PRT, "detilt last_good_freq %e gap %ld\n",last_good_freq,gaptime);
			#endif
			FLL_Elapse_Sec_PTP_Count=detilt_fast;
			tgain=tgain_min*2.0; //March 23 reduced to near minimum
		}
		else if (FLL_Elapse_Sec_PTP_Count < detilt_fast)
		{
			fdrift_f=fdrift_r=fdrift=fdrift_raw=fdrift_warped=fdrift_smooth=pll_int_r=pll_int_f= holdover_f=holdover_r= (double)(last_good_freq);
		}
	}
	// end get last good freq
	
	else if(!(FLL_Elapse_Sec_PTP_Count&loop_mask))//once every loop mask
	{

		stilt= (min_detilt.sdrift_min_1024-sdrift_prev);
		sdrift_prev=min_detilt.sdrift_min_1024;
		//sanity check
		if(dsl_flag)
		{
			if(stilt>800000)	stilt=0;
			else if (stilt<-800000) stilt=0;
			if(tgain>0.001) tgain=0.001;
			if( FLL_Elapse_Sec_PTP_Count>detilt_fast)
			{
	   			if(tgain > 	tgain_min) tgain*=.88;//was 0.9
	   			pll_int_f+= tgain*(double)stilt;
		   		pll_int_r=pll_int_f;
			}
			else if(FLL_Elapse_Sec_PTP_Count>detilt_skip)//skip first window
			{
		 		pll_int_f+= 0.001*(double)stilt;
		   		pll_int_r=pll_int_f;
			}
		}
		else
		{
			if(stilt>64000)	stilt=0;
			else if (stilt<-64000) stilt=0;
			if( FLL_Elapse_Sec_PTP_Count>detilt_fast)
			{
		   		if(tgain > 	tgain_min) tgain*=.9;//was 0.9
	   			pll_int_f+= tgain*(double)stilt;
	   			pll_int_r=pll_int_f;
			}
			else if(FLL_Elapse_Sec_PTP_Count>detilt_skip)//skip first window
			{
	 			pll_int_f+= tgain*(double)stilt;
	   			pll_int_r=pll_int_f;
			}
		}
#if 1
//		#if (N0_1SEC_PRINT==0)		
		debug_printf(GEORGE_PTP_PRT, " sdrift_min: %ld  count: %ld tgain: %ld stilt: %ld freq_cor: %ld\n",
			min_detilt.sdrift_min_1024, 
			FLL_Elapse_Sec_PTP_Count,
			(long int)(tgain*10000),
			(long int)stilt,
	        (long int)(pll_int_f)
			);
//		#endif	
			Num_of_missed_pkts = 0;		
#endif
	}
	
	 holdover_f = pll_int_f;
 	 holdover_r = pll_int_r;
   	 fdrift_f=     pll_int_f;
   	 fdrift_r=     pll_int_r;
   	 fll_settle=FLL_SETTLE; 
//   	 fdrift=-1350.0;// force constant offset
//	 if( (min_detilt.sdrift_min_1024) < sdrift_cal && FLL_Elapse_Sec_Count<64)
//	 {
//	 	 sdrift_cal = (double)(min_detilt.sdrift_min_1024);
//	 }
//	 else
//	 {
//	  	sdrift_cal += 0.1 * ((double)(min_detilt.sdrift_min_1024)- sdrift_cal);
//	 }		
}

