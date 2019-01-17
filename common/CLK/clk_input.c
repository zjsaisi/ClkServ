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

FILE NAME    : clk_input.c

AUTHOR       : George Zampetti

DESCRIPTION  : 


Revision control header:
$Id: CLK/clk_input.c 1.53 2012/02/14 16:55:09PST Kenneth Ho (kho) Exp  $

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
#include "sc_alarms.h"
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
#include "debug.h"
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

#ifndef MODIFY_BY_JIWANG
#define MODIFY_BY_JIWANG 1
#endif


/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/
//#define FLOOR_DENSITY_ALPHA (0.01)
//#define FLOOR_DENSITY_BETA (1.0 -FLOOR_DENSITY_ALPHA)


/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/



static BOOLEAN prev_high_generic_mode = FALSE;

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/
static INT32 CreateLoadParam(Load_State loadstate);


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
// This function update input packet stats
//
//////////////////////////////////////////////////////////////////////
//static INT32 prev_test_offsetA; //test only
//static int complete_phase_flag=0;
//static int turbo_cluster_for=0;
//static int turbo_cluster_rev=0;
static	UINT32 pcount_turbo[2];
static	UINT32 pcount_turbo_small[2];
static	UINT32 pcount_turbo_large[2];
static	UINT32 pcount_turbo_band[2];
//static	UINT32 pcount_turbo_old[2];

//static	UINT32 pcount_turbo_old[2];
//static	INT32  pcount_turbo_delta[2];
static INT8 cluster_state_f = 0; //assume small to begin
static INT8 cluster_state_r= 0;
//static Load_State LoadState = LOAD_CALNEX_10;
//static Load_State LoadState = LOAD_G_8261_5;
//static Load_State LoadState = LOAD_G_8261_10;
static UINT8 Load_State_Cnt = 0;
static INT8 turbo_indx[2];
static	INPROCESS_PHASE_STRUCT *pipc  = &(IPD.PA[0]);
static   INPROCESS_PHASE_STRUCT *pipc2 = &(IPD.PA[0]);	
#if (LOW_PACKET_RATE_ENABLE == 1)	 	
static double avg_samples_update_f;
static double avg_samples_update_r;
#endif
static INT32 cluster_inc_up_f;
static INT32 cluster_inc_up_r;
static INT32 cluster_inc_down_f;
static INT32 cluster_inc_down_r;
static INT32 candidate_phase_floor, delta_phase_floor;
static INT8 traffic_mode_A_flag = 0;
static INT16 traffic_mode_A_gate = 0;
static INT16 traffic_mode_A_cnt = 0;
static INT16 floor_lift_cnt = 0;
static double working_bias_metric_fw;
static double working_bias_metric_rw;

#ifdef MODIFY_BY_JIWANG
UINT8 set_update_phy_flag[IPP_CHAN_PHY];
#endif

void update_input_stats(void)
{
	INPUT_CHANNEL chan_indx;
	FIFO_STRUCT *fifo;
	int *low_warning;
	int *high_warning;
	int *turbo_cluster;
//	int *load_drop;
#if (LOW_PACKET_RATE_ENABLE == 1)	 		 	
	int packet_count;
#endif
	int floor_indx, floor_cnt,delta_cnt;
	double *pdetilt_correction_ptp;
	double dtemp,sq_temp,quad_temp;
#if (LOW_PACKET_RATE_ENABLE == 1)	 	
	double *avg_samples_update;
#endif	
	INT32 chan_offset,test_offsetA,test_offsetB,test_offset_scaled,floor_thres,floor_thres_small,floor_thres_large,ceiling_thres,density_thres,swap;
	UINT16 /*cur_head,cur_tail,*/pcount;
	int i,j;
#if	(CROSSOVER_TS_GLITCH==1)
	INT32 *pog;
	INT32 delta_test_offset;
#endif	
	if(RFE.first)
	{
		density_thres=1;
	}
	else
	{
		density_thres=min_density_thres_oper;
	}
 	// Set up Current Channel
	for(chan_indx=0;chan_indx<2;chan_indx++)
	{
		switch(chan_indx)
		{
    	case FORWARD_GM1:
	    	fifo=&PTP_Fifo_Local;
	    	floor_thres=min_oper_thres_f;
	    	floor_thres_small=min_oper_thres_small_f;
	    	floor_thres_large=min_oper_thres_large_f;
	    	ceiling_thres=ofw_start_f;
	    	low_warning=&low_warning_for;
	    	high_warning=&high_warning_for;
//			load_drop= &load_drop_for;
	    	turbo_cluster= &turbo_cluster_for; 
	    	pdetilt_correction_ptp=&detilt_correction_for;
#if (LOW_PACKET_RATE_ENABLE == 1)	 	
	    	avg_samples_update=&avg_samples_update_f;
#endif	    	
#if	(CROSSOVER_TS_GLITCH==1)
	    	pog=&prev_offset_gd_f;
#endif	    	
	    	// TODO add Timestamp Align Offset case for in-band case !!!
    		if(SENSE==0)
    		{
    			chan_offset= - Initial_Offset-(INT32)hold_res_acc;
    		}
    		else
    		{
    			chan_offset= + Initial_Offset+(INT32)hold_res_acc;
    		}
 //        chan_offset=0;
		break;
		case REVERSE_GM1:
			fifo=&PTP_Delay_Fifo_Local;
	    	floor_thres=min_oper_thres_r;
	    	floor_thres_small=min_oper_thres_small_r;
	    	floor_thres_large=min_oper_thres_large_r;
	    	ceiling_thres=ofw_start_r;
	    	low_warning=&low_warning_rev;
	    	high_warning=&high_warning_rev;
	    	turbo_cluster= &turbo_cluster_rev; 
//			load_drop= &load_drop_rev;
	    	pdetilt_correction_ptp=&detilt_correction_rev;
#if (LOW_PACKET_RATE_ENABLE == 1)	 	
	    	avg_samples_update=&avg_samples_update_r;
#endif	    	
#if	(CROSSOVER_TS_GLITCH==1)
	    	pog=&prev_offset_gd_r;
#endif	    	
    		if(SENSE==0)
    		{
    			chan_offset= - Initial_Offset_Reverse+(INT32)hold_res_acc;
    		}
    		else
    		{
    			chan_offset= + Initial_Offset_Reverse-(INT32)hold_res_acc;
    		}
//        chan_offset=0;
		break;
		default:
		break;
		}
#if 0
		// update tilt compensation TODO add reset with transient indication	
		if( ((rms.freq_source[0])== RM_GM_1)||((rms.freq_source[0])== RM_GM_2) ) //active freq source case
		{
	//		*pdetilt_correction_ptp =0.0; Just Freeze at current position
			detilt_correction_rev = -detilt_correction_for;
         printf ("DETILT GM case chan_indx= %d *pdetilt_correction_ptp= %le\n", chan_indx, *pdetilt_correction_ptp);

		}
		else
		{

			if(chan_indx == 0)
			{
				*pdetilt_correction_ptp += time_bias_est_rate;
//            printf ("DETILT SE case - PTP chan_indx= %d *pdetilt_correction_ptp= %le\n", chan_indx, *pdetilt_correction_ptp);
			}
			else
			{
				*pdetilt_correction_ptp = -detilt_correction_for;
//            printf ("DETILT SE case - not PTP chan_indx= %d *pdetilt_correction_ptp= %le\n", chan_indx, *pdetilt_correction_ptp);

			}
			hold_res_acc=0.0;
//			*pdetilt_correction_ptp = 8000.0; //TEST Force Offset 
//			*pdetilt_correction_ptp = 0.0; //TEST Force Offset 
		}
#endif
 //     printf ("chan_indx= %d *pdetilt_correction_ptp= %le\n", chan_indx, *pdetilt_correction_ptp);
      // TODO tilt bias cancels when crossover but not TC13
      // NEED to MAKE SURE the logic is correct for all cluster cases 
//      detilt_correction_for= 0;  //TEST TEST TEST
//      detilt_correction_rev= -0;
		// Special Case minimum floor threshold while in turbo mode
//		if(rfe_turbo_flag) 
//		{
//			if(floor_thres<1000) floor_thres=1000;
//		}
		// Common Stat Accumulation Logic for all Phases
		pcount=0;
//	 	rfe_turbo_phase_acc[0]=0.0;
//	 	rfe_turbo_phase_acc[1]=0.0;
#if (LOW_PACKET_RATE_ENABLE == 1)	 		 	
//		if(ptpRate > -4) //operating with a low packet rate
//      if(ptpTransport!=e_PTP_MODE_HIGH_JITTER_ACCESS)
		{
			
		
		 	// GPZ DEC 2010 add flow normalization to support 0.5, 1,2, 4, 8 ... rate
		 	packet_count=((FIFO_BUF_SIZE + fifo->fifo_in_index-fifo->fifo_out_index)%FIFO_BUF_SIZE);
		 	if(packet_count>150) packet_count=150;
		 	*avg_samples_update = 0.9*(*avg_samples_update) + 0.1 * (double)packet_count;
//			debug_printf(GEORGE_PTP_PRT, "IPD avg_samples_update: %ld, %ld\n",(long)(*avg_samples_update), (long)(packet_count));
	 	
		 	if((*avg_samples_update) < 56) //Adjust out pointer 
		 	{
	 			if((*avg_samples_update) > 0)
	 			{
	 				fifo->fifo_out_index = (fifo->fifo_out_index + FIFO_BUF_SIZE - (64L-(long)(*avg_samples_update)) )%FIFO_BUF_SIZE;
//					debug_printf(GEORGE_PTP_PRT, "IPD new out indx: %d\n",fifo->fifo_out_index);

	 			}
	 			else
		 		{
		 			fifo->fifo_out_index = (fifo->fifo_out_index + FIFO_BUF_SIZE - (16L-(long)(*avg_samples_update)) )%FIFO_BUF_SIZE;
//					debug_printf(GEORGE_PTP_PRT, "IPD new out indx: %d\n",fifo->fifo_out_index);
	 			
	 			}
	 		}
		}
#endif	 	
 		while(fifo->fifo_out_index != fifo->fifo_in_index)
		{
         if((IPD.phase_index & 0x3) == 0) //March 2012 decimate every fourth time
         {
			if(SENSE==0)
			{
				test_offsetA= fifo->offset[fifo->fifo_out_index];
			}
			else
			{
				test_offsetA= -fifo->offset[fifo->fifo_out_index];
			}
//					debug_printf(GEORGE_PTP_PRT, "IPD snapshot: cind %d, off:%ld,o_ind:%d,i_ind:%d\n"
//					,chan_indx,test_offsetA,fifo->fifo_out_index,fifo->fifo_in_index);
				test_offsetA+= chan_offset;

			for(i=0;i<8;i++) //update all phase stats
			{
				switch(i)
				{
    				case 0:
    					pipc = &(IPD.PA[chan_indx]);
    				break;
    				case 1:
	  					pipc = &(IPD.PB[chan_indx]);
    				break;
    				case 2:
	  					pipc = &(IPD.PC[chan_indx]);
     				break;
    				case 3:
	  					pipc = &(IPD.PD[chan_indx]);
     				break;
    				case 4:
    					pipc = &(IPD.PE[chan_indx]);
    				break;
    				case 5:
	  					pipc = &(IPD.PF[chan_indx]);
    				break;
    				case 6:
	  					pipc = &(IPD.PG[chan_indx]);
     				break;
    				case 7:
	  					pipc = &(IPD.PH[chan_indx]);
     				break;
     				
    				default:
     					pipc = &(IPD.PA[chan_indx]); //dummy assignment
 
    				break;
				}
				// GPZ April 2011 find 8 candidate minimum
				if(test_offsetA< pipc->IPC.floor_set[0])
				{
					pipc->IPC.floor_set[0] = test_offsetA;
				}
				else if((test_offsetA != pipc->IPC.floor_set[0]) && (test_offsetA< pipc->IPC.floor_set[1]))
				{
					pipc->IPC.floor_set[1] = test_offsetA;
				}
				else if((test_offsetA != pipc->IPC.floor_set[1]) && (test_offsetA< pipc->IPC.floor_set[2]))
				{
					pipc->IPC.floor_set[2] = test_offsetA;
				}
				else if((test_offsetA != pipc->IPC.floor_set[2]) && (test_offsetA< pipc->IPC.floor_set[3]))
				{
					pipc->IPC.floor_set[3] = test_offsetA;
				}
				else if((test_offsetA != pipc->IPC.floor_set[3]) && (test_offsetA< pipc->IPC.floor_set[4]))
				{
					pipc->IPC.floor_set[4] = test_offsetA;
				}
				else if((test_offsetA != pipc->IPC.floor_set[4]) && (test_offsetA< pipc->IPC.floor_set[5]))
				{
					pipc->IPC.floor_set[5] = test_offsetA;
				}
				else if((test_offsetA != pipc->IPC.floor_set[5]) && (test_offsetA< pipc->IPC.floor_set[6]))
				{
					pipc->IPC.floor_set[6] = test_offsetA;
				}
				else if((test_offsetA != pipc->IPC.floor_set[6]) && (test_offsetA< pipc->IPC.floor_set[7]))
				{
					pipc->IPC.floor_set[7] = test_offsetA;
				}
				if(test_offsetA > pipc->IPC.ceiling)
				{
					pipc->IPC.ceiling = test_offsetA;
				}
				test_offsetB = test_offsetA - IPD.working_phase_floor[chan_indx];
            test_offset_scaled =4*test_offsetA; //scale for 4 repeats per second operation
            
				if((test_offsetB)< floor_thres)	
				{
    			 	 pipc->IPC.floor_cluster_sum+= test_offset_scaled;
    		 		 pipc->IPC.floor_cluster_count +=4;
    		 		 rfe_turbo_phase_acc[chan_indx]+=(double)(test_offset_scaled);
    		 		 pcount_turbo[chan_indx] +=4;
 		 		}
				if((test_offsetB)< floor_thres_small)	
				{
    		 		 rfe_turbo_phase_acc_small[chan_indx]+=(double)(test_offset_scaled);
    		 		 pcount_turbo_small[chan_indx] +=4;
 		 		}
            // use different metric approach for mean
 				if((test_offsetB)< floor_thres_large)	
				{
    		 		 rfe_turbo_phase_acc_large[chan_indx]+=(double)(test_offset_scaled);
   		 		 pcount_turbo_large[chan_indx] +=4;;
 		 		}
            if( (test_offsetB) < (10000.0))  //was 50000
            {
  		 		    pcount_turbo_band[chan_indx] +=4;
     		 		 rfe_turbo_phase_acc_large_band[chan_indx]+= (double)(test_offset_scaled);
            }  
 		 		
 		 		
				if(((test_offsetB)> ceiling_thres) &&  ((test_offsetB)< (ceiling_thres+F_OFW_SIZE)))	
 				{
    			 	 pipc->IPC.ofw_cluster_sum+= (4*test_offsetB);
    		 		 pipc->IPC.ofw_cluster_count += 4;
   				}
 		 		
 		 		
 			} // end for loop to update all phase stats		 	
 		 	pcount++;
         } //end decimation if logic
 		 	fifo->fifo_out_index=(fifo->fifo_out_index+1)%FIFO_BUF_SIZE;

 		} 
//		debug_printf(GEORGE_PTP_PRT, "IPD after FIFO chan: %ld, pcount: %ld, phase_index: %ld, pcount_turbo: %ld\n",(long int)chan_indx, pcount, (long int)IPD.phase_index, (long int)pcount_turbo[chan_indx] );

		// State Machine for Updating each phase //
		if(IPD.phase_index==7) //complete phase A
		{
			pipc2 = &(IPD.PA[chan_indx]);
			complete_phase_flag=1;
		}
		else if(IPD.phase_index==15) //complete phase B
		{
		
			pipc2 = &(IPD.PB[chan_indx]);
			complete_phase_flag=2;
		}
		else if(IPD.phase_index==23) //complete phase C
		{
			pipc2 = &(IPD.PC[chan_indx]);
			complete_phase_flag=3;
		}
		else if(IPD.phase_index==31) //complete phase D
		{
			pipc2 = &(IPD.PD[chan_indx]);
			complete_phase_flag=4;
		}
     		
		else if(IPD.phase_index==39) //complete phase E
		{
			pipc2 = &(IPD.PE[chan_indx]);
			complete_phase_flag=5;
		}
     		
		else if(IPD.phase_index==47) //complete phase F
		{
			pipc2 = &(IPD.PF[chan_indx]);
			complete_phase_flag=6;
		}

		else if(IPD.phase_index==55) //complete phase G
		{
			pipc2 = &(IPD.PG[chan_indx]);
			complete_phase_flag=7;
		}
		else if(IPD.phase_index==63) //complete phase H
		{
			pipc2 = &(IPD.PH[chan_indx]);
			complete_phase_flag=8;
			turbo_indx[chan_indx]++;
		}
		if(complete_phase_flag)
		{
		
				// Single Pass Sort of floor sets
            for(j=0;j<7;j++)
            {
               if((pipc->IPC.floor_set[j+1])> (pipc->IPC.floor_set[j]))
               {
                  swap = (pipc->IPC.floor_set[j+1]);
                  (pipc->IPC.floor_set[j+1])=(pipc->IPC.floor_set[j]);
                  (pipc->IPC.floor_set[j])=swap;
               }
            }
		
//			if((pcount_turbo[chan_indx]) && (complete_phase_flag==8)&&(turbo_indx[chan_indx] & 0x01) ) //every other phase index loop (32 seconds)
			if(complete_phase_flag==8)  //every phase index loop (16 seconds)
 			{
#if (SENSE==0)
					if(chan_indx==0)
					{
                  if(pcount_turbo[chan_indx])
                  {
 						   rfe_turbo_phase_avg[chan_indx]= -rfe_turbo_phase_acc[chan_indx]/(double)(pcount_turbo[chan_indx]); 
						   rfe_turbo_phase[chan_indx]= rfe_turbo_phase_avg[chan_indx] + (INT32)fbias;
                     rfe_turbo_phase[chan_indx]-= detilt_correction_for;
                  }
                  if(pcount_turbo_small[chan_indx])
                  {
 						   rfe_turbo_phase_avg_small[chan_indx]= -rfe_turbo_phase_acc_small[chan_indx]/(double)(pcount_turbo_small[chan_indx]); 
						   rfe_turbo_phase_small[chan_indx]= rfe_turbo_phase_avg_small[chan_indx] + (INT32)fbias_small;
                     rfe_turbo_phase_small[chan_indx]-= detilt_correction_for; 
                  }	
                  if(pcount_turbo_large[chan_indx])
                  {
 						   rfe_turbo_phase_avg_large[chan_indx]= -rfe_turbo_phase_acc_large[chan_indx]/(double)(pcount_turbo_large[chan_indx]); 
						   rfe_turbo_phase_large[chan_indx]= rfe_turbo_phase_avg_large[chan_indx];
//                     rfe_turbo_phase_large[chan_indx]-= detilt_correction_for; 
 		
                  }	
 	   	         if(pcount_turbo_band[chan_indx])
                  {
   			   	   rfe_turbo_phase_avg_large_band[chan_indx]= -rfe_turbo_phase_acc_large_band[chan_indx]/(double)(pcount_turbo_band[chan_indx]); 
						   rfe_turbo_phase_large_band[chan_indx]= rfe_turbo_phase_avg_large_band[chan_indx] + (INT32)fbias_large;
                     rfe_turbo_phase_large_band[chan_indx]-= detilt_correction_for;
                  }
 			
		  			}	
 					else
 					{
                  if(pcount_turbo[chan_indx])
                  {
    						rfe_turbo_phase_avg[chan_indx]= rfe_turbo_phase_acc[chan_indx]/(double)(pcount_turbo[chan_indx]); 
   						rfe_turbo_phase[chan_indx] = rfe_turbo_phase_avg[chan_indx] - (INT32)rbias;		
                     rfe_turbo_phase[chan_indx]+= detilt_correction_rev; 

                  }
                  if(pcount_turbo_small[chan_indx])
                  {
 						   rfe_turbo_phase_avg_small[chan_indx]= rfe_turbo_phase_acc_small[chan_indx]/(double)(pcount_turbo_small[chan_indx]); 
						   rfe_turbo_phase_small[chan_indx]= rfe_turbo_phase_avg_small[chan_indx] - (INT32)rbias_small;
                     rfe_turbo_phase_small[chan_indx]+= detilt_correction_rev; 
                  }	
                  if(pcount_turbo_large[chan_indx])
                  {
 						   rfe_turbo_phase_avg_large[chan_indx]= rfe_turbo_phase_acc_large[chan_indx]/(double)(pcount_turbo_large[chan_indx]); 
						   rfe_turbo_phase_large[chan_indx]= rfe_turbo_phase_avg_large[chan_indx];
//                     rfe_turbo_phase_large[chan_indx]+= detilt_correction_rev; 
		            }	
	               if(pcount_turbo_band[chan_indx])
                  {
		               rfe_turbo_phase_avg_large_band[chan_indx]= rfe_turbo_phase_acc_large_band[chan_indx]/(double)(pcount_turbo_band[chan_indx]); 
						   rfe_turbo_phase_large_band[chan_indx]= rfe_turbo_phase_avg_large_band[chan_indx] - (INT32)rbias_large;
                     rfe_turbo_phase_large_band[chan_indx]+= detilt_correction_rev; 
                  }   	
   	  			}
#else
   				if(chan_indx==0)
					{
                  if(pcount_turbo[chan_indx])
                  {
 						   rfe_turbo_phase_avg[chan_indx]= rfe_turbo_phase_acc[chan_indx]/(double)(pcount_turbo[chan_indx]); 
						   rfe_turbo_phase[chan_indx]= rfe_turbo_phase_avg[chan_indx] + (INT32)fbias;
                     rfe_turbo_phase[chan_indx]+= detilt_correction_for;
                  }
                  if(pcount_turbo_small[chan_indx])
                  {
 						   rfe_turbo_phase_avg_small[chan_indx]= rfe_turbo_phase_acc_small[chan_indx]/(double)(pcount_turbo_small[chan_indx]); 
						   rfe_turbo_phase_small[chan_indx]= rfe_turbo_phase_avg_small[chan_indx] + (INT32)fbias_small;
                     rfe_turbo_phase_small[chan_indx]+= detilt_correction_for; 
                  }	
                  if(pcount_turbo_large[chan_indx])
                  {
 						   rfe_turbo_phase_avg_large[chan_indx]= rfe_turbo_phase_acc_large[chan_indx]/(double)(pcount_turbo_large[chan_indx]); 
						   rfe_turbo_phase_large[chan_indx]= rfe_turbo_phase_avg_large[chan_indx];
 //                    rfe_turbo_phase_large[chan_indx]+= detilt_correction_for;
 		            }
                  if(pcount_turbo_band[chan_indx])
                  {
		               rfe_turbo_phase_avg_large_band[chan_indx]= rfe_turbo_phase_acc_large_band[chan_indx]/(double)(pcount_turbo_band[chan_indx]); 
						   rfe_turbo_phase_large_band[chan_indx]= rfe_turbo_phase_avg_large_band[chan_indx] + (INT32)fbias_large;
                     rfe_turbo_phase_large_band[chan_indx]+= detilt_correction_for; 
        	         }
	 				}	
 					else
 					{
                  if(pcount_turbo[chan_indx])
                  {
    						rfe_turbo_phase_avg[chan_indx]= -rfe_turbo_phase_acc[chan_indx]/(double)(pcount_turbo[chan_indx]); 
   						rfe_turbo_phase[chan_indx] = rfe_turbo_phase_avg[chan_indx] + (INT32)rbias;		
                     rfe_turbo_phase[chan_indx]-= detilt_correction_rev; 
                  }
                  if(pcount_turbo_small[chan_indx])
                  {
 						   rfe_turbo_phase_avg_small[chan_indx]= -rfe_turbo_phase_acc_small[chan_indx]/(double)(pcount_turbo_small[chan_indx]); 
						   rfe_turbo_phase_small[chan_indx]= rfe_turbo_phase_avg_small[chan_indx] + (INT32)rbias_small;
                     rfe_turbo_phase_small[chan_indx]-= detilt_correction_rev; 
                  }	
                  if(pcount_turbo_large[chan_indx])
                  {
 						   rfe_turbo_phase_avg_large[chan_indx]= -rfe_turbo_phase_acc_large[chan_indx]/(double)(pcount_turbo_large[chan_indx]); 
						   rfe_turbo_phase_large[chan_indx]= rfe_turbo_phase_avg_large[chan_indx];
 //                    rfe_turbo_phase_large[chan_indx]-= detilt_correction_rev; 
                  }
                  if(pcount_turbo_band[chan_indx])
                  {
				         rfe_turbo_phase_avg_large_band[chan_indx]= -rfe_turbo_phase_acc_large_band[chan_indx]/(double)(pcount_turbo_band[chan_indx]); 
						   rfe_turbo_phase_large_band[chan_indx]= rfe_turbo_phase_avg_large_band[chan_indx] + (INT32)rbias_large;
                     rfe_turbo_phase_large_band[chan_indx]-= detilt_correction_rev; 
                  }	
	   			}
#endif 	
            rfe_turbo_phase_acc[chan_indx]=0.0;
            rfe_turbo_phase_acc_small[chan_indx]=0.0;
            rfe_turbo_phase_acc_large[chan_indx]=0.0;
            rfe_turbo_phase_acc_large_band[chan_indx]=0.0;
           if(chan_indx==0)
            {
               floor_density_f=pcount_turbo[chan_indx];
               floor_density_small_f=pcount_turbo_small[chan_indx];
               floor_density_large_f=pcount_turbo_large[chan_indx];
               floor_density_band_f=pcount_turbo_band[chan_indx];
            }
            else
            {
               floor_density_r=pcount_turbo[chan_indx];
               floor_density_small_r=pcount_turbo_small[chan_indx];
               floor_density_large_r=pcount_turbo_large[chan_indx];
               floor_density_band_r=pcount_turbo_band[chan_indx];
            }
            pcount_turbo[chan_indx]=0;
            pcount_turbo_small[chan_indx]=0;
            pcount_turbo_large[chan_indx]=0;
            pcount_turbo_band[chan_indx]=0;
       }
         if((*turbo_cluster)>0) (*turbo_cluster)=(*turbo_cluster)-1;
#if 0  //GPZ FEB 9 disable for all builds
			if((!((ptpTransport==e_PTP_MODE_ETHERNET) && !(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS))||(ptpTransport==e_PTP_MODE_MICROWAVE))  )
			{

			   // GPZ 	APRIL 2011 MAKE turbo cluster decision separate logic
      		if((floor_thres<5000) && (pipc2->IPC.floor_cluster_count<4) // 5000, 4
      		    && (!start_in_progress_PTP)&&(mode_jitter==0)) 
				{
					{
				     	(*turbo_cluster)=20;//was 20
//						debug_printf(GEORGE_PTP_PRT, "IPD turbo cluster cnt low density chan %d\n", chan_indx);
					}
				}		
				// add low density warning state NOV 20 2010
      		if((floor_thres<5000) && (pipc2->IPC.floor_cluster_count<40) // 4 GPZ APRIL 2011 Adjust to detect both forward and reverse TC 13 size density shocks
      		    && (!start_in_progress_PTP)&&(mode_jitter==0)) 
      		{
      			*low_warning= (*low_warning) +1;
					debug_printf(GEORGE_PTP_PRT, "IPD warning:low min density:chan %d, count %ld, wfloor %ld, warn_cnt %d\n",
					chan_indx,
					pipc2->IPC.floor_cluster_count,
					pipc2->working_phase_floor,
					*low_warning
				 	);
      		}  

 				// update floor stats  
				// GPZ 	APRIL 2011 MAKE turbo cluster decision separate logic
      		if((floor_thres>1000)&&(pipc2->IPC.floor_cluster_count>265)   //was 2000
      		    && (!start_in_progress_PTP)&&(mode_jitter==0)) //GPZ NOV 2010 NEW high min density condition
      		
      		{
 			     	(*turbo_cluster)=20; //was 64
// JAN 26		debug_printf(GEORGE_PTP_PRT, "IPD turbo cluster cnt high density chan %d\n", chan_indx);
     			
      		}
			}
#endif
	   	if(pipc2->IPC.floor_cluster_count>density_thres)
    		{
 				pipc2->phase_floor_avg= pipc2->IPC.floor_cluster_sum/pipc2->IPC.floor_cluster_count;
 //				pipc2->phase_floor_avg += *pdetilt_correction_ptp; //GPZ FEB 2011 re-tilt after processing window	
 					
				debug_printf(GEORGE_PTP_PRT, "IPD PTP stats: %ld, indx: %d, chan: %d,tilt: %ld clus_med: %ld, clus_small: %ld, clus_large: %ld, cnt:%ld, wfloor:%ld, thres_med:%ld, thres_small:%ld, thres_large: %ld\n",
					(long int)FLL_Elapse_Sec_Count,
					(int)complete_phase_flag,
					(int)chan_indx,
               (long int) (*pdetilt_correction_ptp),
               (long int)rfe_turbo_phase[chan_indx],
               (long int)rfe_turbo_phase_small[chan_indx],
               (long int)rfe_turbo_phase_large[chan_indx],
					(long int)pipc2->IPC.floor_cluster_count,
					(long int)pipc2->working_phase_floor,				
					(long int)floor_thres,
					(long int)floor_thres_small,
					(long int)floor_thres_large

				);
      		}
      		else if(!start_in_progress_PTP&&(!RFE.first))
      		{
 //    			pipc2->phase_floor_avg=MAXMIN;
// Jan 26	debug_printf(GEORGE_PTP_PRT, "IPD PTP exception:low min density:chan %d, count %ld, wfloor %ld, floor %ld\n",
//				chan_indx,
//				pipc2->IPC.floor_cluster_count,
//				pipc2->working_phase_floor,
//				pipc2->IPC.floor_set[0]
//				 );
      			
    		}
     		else if(LoadState != LOAD_GENERIC_HIGH)
//     		else
     		{
     			// APRIL 2011 turbo attack while no floor is detected
		     	(*turbo_cluster)=64;
// Jan 26	debug_printf(GEORGE_PTP_PRT, "IPD turbo cluster cnt low density chan %d\n", chan_indx);
//   			pipc2->phase_floor_avg=pipc2->IPC.floor_set[0];
//				debug_printf(GEORGE_PTP_PRT, "IPD PTP No floor:chan %d, count %ld, wfloor %ld, floor %ld\n",
//				chan_indx,
//				pipc2->IPC.floor_cluster_count,
//				pipc2->working_phase_floor,
//				pipc2->IPC.floor_set[0]
//				 );
//#if ((ASYMM_CORRECT_ENABLE==1)||(HYDRO_QUEBEC_ENABLE==1))
//     			pipc2->phase_floor_avg=MAXMIN;
//#endif				 
     			
     		}
			// update offset window stats
			
			// update new floor baseline for phase APRIL 2011 add small 4 item cluster average to reduce buzz at floor
			candidate_phase_floor=0;
			floor_cnt=0;						
//			for(floor_indx=1; floor_indx<5; floor_indx++)
			for(floor_indx=0; floor_indx<2; floor_indx++)
			{
				if(pipc2->IPC.floor_set[floor_indx] != MAXMIN)
				{
					candidate_phase_floor += pipc2->IPC.floor_set[floor_indx];	
					floor_cnt++;
				}
			}
			if(floor_cnt > 0)
			{
				candidate_phase_floor= (candidate_phase_floor / floor_cnt);
			}
			else
			{
				candidate_phase_floor= pipc2->working_phase_floor;
			}
         delta_phase_floor= candidate_phase_floor - pipc2->working_phase_floor;
//       if(1)
         if((RFE.first != 0)||((ptpTransport!=e_PTP_MODE_ETHERNET)&&(ptpTransport!=e_PTP_MODE_HIGH_JITTER_ACCESS))||(delta_phase_floor>15000)||(delta_phase_floor<-15000))
         {
            pipc2->working_phase_floor += delta_phase_floor;
         }
         else if(chan_indx==0)
         {
            if(delta_phase_floor >  1000) pipc2->working_phase_floor += 1000;
            else if(delta_phase_floor < -1000) pipc2->working_phase_floor -= 1000;
            else pipc2->working_phase_floor += delta_phase_floor;
         }
         else if(chan_indx==1)
         {
            if(delta_phase_floor >  3000) pipc2->working_phase_floor += 3000;
            else if(delta_phase_floor < -3000) pipc2->working_phase_floor -= 3000;
            else pipc2->working_phase_floor += delta_phase_floor;
         }
      


//			pipc2->working_phase_floor = pipc2->working_phase_floor - (floor_thres/4) -500; //TEST TEST TEST NOTE THIS should be made permanent to ensure clean floor detect

  				debug_printf(GEORGE_PTP_PRT, "IPD PTP floor set :chan %d, floor_0 %ld, floor_1 %ld, floor_2 %ld, floor_3 %ld\n",
				chan_indx,
				pipc2->IPC.floor_set[0],
				pipc2->IPC.floor_set[1],
				pipc2->IPC.floor_set[2],
				pipc2->IPC.floor_set[3]
				 );
         if(pipc2->phase_ceiling_avg < pipc2->IPC.ceiling)
         {
            pipc2->phase_ceiling_avg = pipc2->IPC.ceiling;		
         }	
			// assign current output data
			IPD.working_phase_floor[chan_indx]=	pipc2->working_phase_floor;
			IPD.phase_floor_avg[chan_indx]=	pipc2->phase_floor_avg;
			IPD.ofw_cluster_count[chan_indx]=pipc2->IPC.ofw_cluster_count;
			IPD.floor_cluster_count[chan_indx]=pipc2->IPC.floor_cluster_count;
         if(complete_phase_flag == 8)
         {
            IPD.phase_ceiling_avg[chan_indx]= pipc2->phase_ceiling_avg;
            pipc2->phase_ceiling_avg = -MAXMIN;
         }
			// re-initialize working phase variables
         if(rfe_turbo_flag==0)
         {
   			pipc2->IPC.floor_set[0] =  MAXMIN;
	   		pipc2->IPC.floor_set[1] =  MAXMIN;
	   		pipc2->IPC.floor_set[2] =  MAXMIN;
	   		pipc2->IPC.floor_set[3] =  MAXMIN;
   			pipc2->IPC.floor_set[4] =  MAXMIN;
	   		pipc2->IPC.floor_set[5] =  MAXMIN;
	   		pipc2->IPC.floor_set[6] =  MAXMIN;
	   		pipc2->IPC.floor_set[7] =  MAXMIN;

         }
//        else if(turbo_indx[chan_indx] & 0x01)
        else 
        {
    			pipc2->IPC.floor_set[0] =  MAXMIN;
	   		pipc2->IPC.floor_set[1] =  MAXMIN;
	   		pipc2->IPC.floor_set[2] =  MAXMIN;
	   		pipc2->IPC.floor_set[3] =  MAXMIN;
   			pipc2->IPC.floor_set[4] =  MAXMIN;
	   		pipc2->IPC.floor_set[5] =  MAXMIN;
	   		pipc2->IPC.floor_set[6] =  MAXMIN;
	   		pipc2->IPC.floor_set[7] =  MAXMIN;

         }
	      pipc2->IPC.ceiling=-MAXMIN;
			pipc2->IPC.floor_cluster_count=0;
			pipc2->IPC.floor_cluster_sum=0L;	
			pipc2->IPC.ofw_cluster_count=0;
			pipc2->IPC.ofw_cluster_sum=0L;
		
			complete_phase_flag=0;
		}
		
		// End Phase State Machine
		
	} //end for loop for each input channel
	IPD.phase_index=(IPD.phase_index+1)%64; //advance through 64 second block
	// assign system minimum data elements
	delta_cnt= (  (long int)(floor_density_f)- IPD.floor_cluster_count[FORWARD_GM1]);
	if(!RFE.first || ((delta_cnt< 800) && (delta_cnt> -800)))
	{
//      turbo_cluster_for=20;
	}
#if 0
	// update smooth floor density
	delta_cnt= (  (long int)(floor_density_r)- IPD.floor_cluster_count[REVERSE_GM1]);
	if(0)
//	if(!RFE.first || ((delta_cnt< 800) && (delta_cnt> -800)))
	{
		floor_density_r = (FLOOR_DENSITY_BETA)*floor_density_r + (FLOOR_DENSITY_ALPHA)*(double)(IPD.floor_cluster_count[REVERSE_GM1]);
	}
	else
	{
		floor_density_r =(double)IPD.floor_cluster_count[REVERSE_GM1];
	}
#endif
   if(IPD.phase_index == 0)
   {
		debug_printf(GEORGE_PTP_PRT,"Load Mode: %ld, Mode: %ld flr_r: %ld cnt: %ld\n", 
            (long int) LoadState, (long int) traffic_mode_A_flag, (long int)floor_density_r, (long int)traffic_mode_A_gate);
   }

   if((IPD.phase_index & 0xF)==4) //every 16th cycle decimate for better pacing
	{
//   Use rule base to establish current Load State to apply proper bias compensation and
//   select small or standard clustering  
   	if((ptpTransport==e_PTP_MODE_ETHERNET) || (ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS))
      {	
//         debug_printf(GEORGE_FAST_PRT,"Load_State: reverse density %ld\n", (long int)floor_density_r); 

         if( (floor_density_r>27000) && (floor_density_r<31500) &&
             (floor_density_small_r >25000) && (floor_density_small_r<31500)&&(!RFE.first) ) //traffic mode A region
         {
//            debug_printf(GEORGE_FAST_PRT,"Load_State:Traffic Mode Gate %ld\n", (long int)traffic_mode_A_gate); 

           if(traffic_mode_A_flag == 0)
           { 
              traffic_mode_A_gate++;
              if(traffic_mode_A_gate > 32)
              {
                traffic_mode_A_gate=0;
                traffic_mode_A_flag=1;
                traffic_mode_A_cnt=22000;
                 debug_printf(GEORGE_PTP_PRT,"Load_State: change to Traffic Mode A\n"); 
              }
           }  
         }
         else
         {
            traffic_mode_A_gate=0;
         }
         if(traffic_mode_A_cnt > 1)
         {
            traffic_mode_A_cnt--;
         }
         else if ( (traffic_mode_A_cnt == 1)||((floor_density_r>31500)&&(floor_density_f>31500) ))
         {
               if(traffic_mode_A_flag==1)
               {
                  traffic_mode_A_gate=0;
                  traffic_mode_A_flag=0;
                  traffic_mode_A_cnt=0;
                  debug_printf(GEORGE_PTP_PRT,"Load_State: change to Traffic Mode B\n"); 
               }
         }
//         debug_printf(GEORGE_FAST_PRT,"Enter Load State\n"); 
         if( (ofw_start_f>56000)&&(ofw_start_f<58000)&&(!RFE.first)&&(LoadState != LOAD_G_8261_10))
         {
            if(floor_density_small_f >0.0) sq_temp = floor_density_f/floor_density_small_f;
            else sq_temp=100.0;
//         debug_printf(GEORGE_FAST_PRT,"Ratio: %le Cnt:%ld\n", sq_temp, (long int)Load_State_Cnt); 
            if((sq_temp < 10.0)&&(sq_temp > 4.0))  
            {
               if((LoadState != LOAD_G_8261_5)&&(Load_State_Cnt < 100)) Load_State_Cnt++; 
               else Load_State_Cnt=0; 
               if((LoadState != LOAD_G_8261_5)&&(Load_State_Cnt>64))
               {
                 LoadState=LOAD_G_8261_5;
                 Load_State_Cnt=0; 
                 debug_printf(GEORGE_PTP_PRT,"Load_State: change to G_8261_5\n"); 
//			        event_printf(e_LOAD_MODE_CHANGED, e_EVENT_NOTC, e_EVENT_TRANSIENT,
//			             sc_alarm_lst[e_LOAD_MODE_CHANGED].dsc_str, CreateLoadParam(LoadState));
                 Forward_Floor_Change_Flag=2;  
                 Reverse_Floor_Change_Flag=2;  
                 init_pshape_long();
 

               }
            }
            else 
            {
               if((LoadState != LOAD_CALNEX_10)&&(Load_State_Cnt < 100)) Load_State_Cnt++; 
               else Load_State_Cnt=0; 
               if(LoadState != LOAD_CALNEX_10&&(Load_State_Cnt>64))
               {
                  LoadState=LOAD_CALNEX_10;
                  debug_printf(GEORGE_PTP_PRT,"Load_State: change to CALNEX_10: %ld\n",(long int)Load_State_Cnt); 
//						event_printf(e_LOAD_MODE_CHANGED, e_EVENT_NOTC, e_EVENT_TRANSIENT,
//			             sc_alarm_lst[e_LOAD_MODE_CHANGED].dsc_str, CreateLoadParam(LoadState));
               }
            }
         }//end at 57K level logic 
         else if ((ofw_start_f>71000)&&(!RFE.first)&&(ofw_start_f<100000))
         {
               if((LoadState != LOAD_G_8261_10)&&(Load_State_Cnt < 128)) Load_State_Cnt++; 
               else Load_State_Cnt=0; 
               if((LoadState != LOAD_G_8261_10)&&(Load_State_Cnt>100))
               {
                 LoadState=LOAD_G_8261_10;
                 Load_State_Cnt=0; 
                 debug_printf(GEORGE_PTP_PRT,"Load_State: change to G_8261_10\n");
//					  event_printf(e_LOAD_MODE_CHANGED, e_EVENT_NOTC, e_EVENT_TRANSIENT,
//			             sc_alarm_lst[e_LOAD_MODE_CHANGED].dsc_str, CreateLoadParam(LoadState)); 
                 Forward_Floor_Change_Flag=2;  
                 Reverse_Floor_Change_Flag=2;
                 init_pshape_long();
               }  
         }
//         else if ((floor_density_large_f<1000)&&(RFE.first < 4))
         else if (RFE.first < 4)
         {
            if(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS)
            {
               if((LoadState != LOAD_GENERIC_HIGH)&&(Load_State_Cnt < 100)) Load_State_Cnt++; 
               else Load_State_Cnt=0; 
               if((LoadState != LOAD_GENERIC_HIGH)&&(Load_State_Cnt>32)) //was 64
               {
                 LoadState=LOAD_GENERIC_HIGH;
                 Load_State_Cnt=0; 
                 debug_printf(GEORGE_PTP_PRT,"Load_State: change to LOAD_GENERIC_HIGH\n");
//					  event_printf(e_LOAD_MODE_CHANGED, e_EVENT_NOTC, e_EVENT_TRANSIENT,
//			             sc_alarm_lst[e_LOAD_MODE_CHANGED].dsc_str, CreateLoadParam(LoadState)); 
     	   			Get_Freq_Corr(&last_good_freq,&gaptime);
                  /* allow to set common variables if a Phy channel is not currently being used */
                   fdrift_f=fdrift_r=fdrift=fdrift_warped=fdrift_smooth =pll_int_r=pll_int_f= fdrift_raw=holdover_f=holdover_r=last_good_freq;
            		setfreq((long int)(last_good_freq*HUNDREDF),0); // 29.57 GPZ Jan 2010 force tight lock    down of frequency
                 Forward_Floor_Change_Flag=2;  
                 Reverse_Floor_Change_Flag=2;
                 short_skip_cnt_f=short_skip_cnt_r= 0;
                 init_pshape_long();
                 RFE.first = 0;
//                 rfe_settle_count=15;
                 RFE.Nmax = 40; 

               }  
            }
            else
            {
/* send the Incompatible transport mode event if jitter is high and not in holdover or bridging or los */
               if((prev_high_generic_mode == FALSE) && 
						!((tstate == FLL_BRIDGE) || (tstate == FLL_HOLD)) && 
						(Is_GPS_OK(RM_GM_1)))
               {
						if(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS)
					   {
							event_printf(e_INCOMPATIBLE_TRANSPORT_MODE, e_EVENT_ERR, e_EVENT_SET,
			             	sc_alarm_lst[e_INCOMPATIBLE_TRANSPORT_MODE].dsc_str, 0); 
                  }
						prev_high_generic_mode = TRUE;
               }
            }
         }
         else
         {
            if(prev_high_generic_mode == TRUE)
            {
					if(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS)
					{
					 	event_printf(e_INCOMPATIBLE_TRANSPORT_MODE, e_EVENT_ERR, e_EVENT_CLEAR,
			             sc_alarm_lst[e_INCOMPATIBLE_TRANSPORT_MODE].dsc_str, 0);
					}
               prev_high_generic_mode = FALSE;
            }
         }



//      debug_printf(GEORGE_FAST_PRT,"Bias Calc Load State: %ld\n", (long int) LoadState); 
      if(LoadState == LOAD_CALNEX_10)
      {
        if(!traffic_mode_A_flag) //traffic mode B case
        {
         if(floor_density_small_f > 0.0)  //was 2000
         {
           if(floor_density_small_f < 14304.0)
           { 
               dtemp= 14304-floor_density_small_f;
               sq_temp= dtemp*dtemp;
               fbias_small = 
                  (double)(-1.40663e-6)*sq_temp+
                  (double)(-7.35384e-3)*dtemp -372.6;  
//      	   debug_printf(GEORGE_FAST_PRT,"fbias_calc:small density:%le, bias %le\n", dtemp, fbias_small); 

           }
           else
           {
             fbias_small = -375.0;
           }
         } //end small density case
        {
            dtemp = rfe_turbo_phase_large[0] - rfe_turbo_phase_avg_large_band[0];
            sq_temp= dtemp*dtemp;
            quad_temp=sq_temp*sq_temp;
            fbias_large = 
                  (double)(-7.30206e-14)*quad_temp +
                  (double)(4.252345e-9)*dtemp*sq_temp +
                  (double)(-8.50601e-5)*sq_temp+
                  (double)(5.19927e-1)*dtemp -4686.8;  
//      	   debug_printf(GEORGE_PTP_PRT,"fbias_calc: density:%le, bias %le, large; %le, band %le\n", dtemp,
//             fbias_large,
//           rfe_turbo_phase_large[0],
//           rfe_turbo_phase_avg_large_band[0]); 
         } //end standard density case
        }
        else //Traffic Mode A
        {
         if(floor_density_small_f > 0.0)  //was 2000
         {
           if(floor_density_small_f < 20000.0)
           { 
               dtemp= 20000-floor_density_small_f;
               sq_temp= dtemp*dtemp;
               fbias_small = 
                  (double)(-1.03064e-6)*sq_temp+
                  (double)(-1.43173e-2)*dtemp -618;  
           }
           else
           {
             fbias_small = -618.0;
           }
         } //end small density case
        {
            dtemp = rfe_turbo_phase_large[0] - rfe_turbo_phase_avg_large_band[0];
            sq_temp= dtemp*dtemp;
            quad_temp=sq_temp*sq_temp;
            fbias_large = 
                  (double)(-5.84411e-14)*quad_temp +
                  (double)(3.90485e-9)*dtemp*sq_temp +
                  (double)(-9.32093e-5)*sq_temp+
                  (double)(7.6486e-1)*dtemp -5942.3;  
//      	   debug_printf(GEORGE_FAST_PRT,"fbias_calc: density:%le, bias %le\n", dtemp, fbias_large); 
         } //end standard density case

        }
         if(RFE.first)
         {
            fbias=fbias_large=fbias_small=-300.0;
         }


            dtemp = rfe_turbo_phase_large[1] - rfe_turbo_phase_avg[1];
            sq_temp= dtemp*dtemp;
            quad_temp=sq_temp*sq_temp;
            rbias = 
                  (double)(-7.389824e-15)*quad_temp +
                  (double)(-7.186048e-10)*dtemp*sq_temp +
                  (double)(-2.380882e-5)*sq_temp+
                  (double)(-4.288797e-1 +0.000)*dtemp +750.0;  //was (double)(-4.288797e-1+0.009)*dtemp +750.0
            if(traffic_mode_A_flag) rbias -= 0.018*dtemp;

         if(traffic_mode_A_flag)
         {
            dtemp=floor_density_small_r;
            sq_temp= dtemp*dtemp;
            rbias_small = 
                  (double)(8.716e-7)*sq_temp-
                  (double)(4.164955e-2)*dtemp +500.0;  //was 755.0,255.0,10000
         }
         else
         {
            dtemp = rfe_turbo_phase_avg_small[1] - rfe_turbo_phase_large[1];
            working_bias_metric_rw =dtemp;

            sq_temp= dtemp*dtemp;
            if(floor_lift_cnt == 0)
            {
               rbias_small = 
                     (double)(8.044779e-11)*dtemp*sq_temp +
                     (double)(-3.546237e-6)*sq_temp+
                     (double)(7.377042e-2 +0.0)*dtemp + 500.0;  //was 500.0
            }
            else
            {
               rbias_small = 
                     (double)(8.044779e-11)*dtemp*sq_temp +
                     (double)(-3.546237e-6)*sq_temp+
                     (double)(7.377042e-2 +0.03)*dtemp + 500.0;  //was 500.0 0.030
 //                 	   debug_printf(GEORGE_FAST_PRT,"Floor Lifting Count: %ld\n", (long int)floor_lift_cnt); 

            }
//            rbias_small +=-0.05*(floor_density_f); //TEST TEST TEST

            if(floor_lift_cnt > 0)
            {
               floor_lift_cnt--;
            }
//            floor_lift_cnt = 0; //TEST TEST TEST
            if((floor_density_small_r > 0)&&(floor_density_r < 30000))
            {
               if(floor_density_r > 23000)
               {
                  if((floor_density_r/floor_density_small_r) > 2.0)
                  {
                     if(floor_lift_cnt < 100)
                     {
                  	   debug_printf(GEORGE_PTP_PRT,"Floor Lifting Detected\n"); 
                     }
                     if(floor_lift_cnt < 7200) floor_lift_cnt = 7200;
                   }
               }
            }
         }

         if(RFE.first)
         {
             rbias=rbias_small=0.0;
         }
      }  //end if CALNEX_10 case
      else if(LoadState == LOAD_G_8261_5)
      {
         if(floor_density_small_f >15000.0) floor_density_small_f = 15000.0;
         sq_temp= (floor_density_f)*(floor_density_f);
   	   fbias =  (double)(-7.1854e-6)*sq_temp+(double)(0.47788)*(floor_density_f)- (double)9458.0;
         fbias_small =  (double)(0.0)*(floor_density_small_f) - (double)95;   //was 0.129 
  	      rbias_small =  (double)(-0.01)*(floor_density_small_r) + (double)551; 
//       fbias=fbias_small=0.0;
//         rbias=rbias_small=0.0;
         rbias_small += -0.013*(floor_density_small_f-4000.0); //GPZ Key cross correlation component -0.01
      }
      else if(LoadState == LOAD_G_8261_10)
      {
           if(floor_density_small_f >6000.0) floor_density_small_f =6000.0;
           if(floor_density_r >20000.0) floor_density_r = 20000.0;
#if 1    //old method
         sq_temp= (floor_density_f)*(floor_density_f);
         fbias =  (double)(-2.3051e-4+0.0005)*sq_temp+(double)(2.40405+1.0)*(floor_density_f)- (double)(12617.0+2500.0);
#endif
#if 0 //need to work on
            dtemp = rfe_turbo_phase_large[0] - rfe_turbo_phase_avg_large_band[0] - 18000.0;
            sq_temp= dtemp*dtemp;
            quad_temp=sq_temp*sq_temp;
            fbias_large = 
                  (double)(-5.413641e-14)*quad_temp +
                  (double)(4.187045e-9)*dtemp*sq_temp +
                  (double)(-1.185199e-4)*sq_temp+
                  (double)(1.321889)*dtemp -14946.0;  
//      	   debug_printf(GEORGE_FAST_PRT,"fbias_calc: density:%le, bias %le\n", dtemp, fbias_large); 

#endif
         sq_temp= (floor_density_small_f)*(floor_density_small_f);
         fbias_small =  (double)(-4.63859e-5)*sq_temp+(double)(0.310705)*(floor_density_small_f)- (double)673.0;
         sq_temp= (floor_density_r)*(floor_density_r);
//  	      rbias =  (double)(-0.0)*(floor_density_r); //-0.1
  	      rbias =  sq_temp*(double)(7.95041e-14)*sq_temp-sq_temp*(double)(3.88486e-9)*(floor_density_r)+
         +(double)(6.97289e-5)*sq_temp-(double)(0.591031)*(floor_density_r)+(double)(4193);  

         rbias += -0.06*(floor_density_f); //GPZ Key cross correlation component -0.06
//         fbias+=4000.0;            
//         fbias_small+=4000.0;            
//          fbias=fbias_small=0.0;
//          rbias=rbias_small=0.0;
      }
      else if(LoadState == LOAD_GENERIC_HIGH)  
      {
//            dtemp = rfe_turbo_phase_large[0] - rfe_turbo_phase_avg[0]-2.0e6;
//            fbias = (double)(3.9758)*dtemp -2.0e+5;  
//            dtemp = rfe_turbo_phase_avg[1] - rfe_turbo_phase_avg_large[1] -2.4e6;
//            rbias = (double)(-0.2032)*dtemp;  
          fbias=fbias_large=fbias_small=0.0;
          rbias=rbias_large=rbias_small=rbias;
            rbias=-fbias;
      }
#if 0
      // common shift of rbias and fbias
      fbias -= 0.0;
      fbias_large -= 0.0;
      fbias_small -= 0.0;
      rbias += 0.0;  //was 500
      rbias_large += 0.0;
      rbias_small += 0.0;
#endif
      } // End PTP Transport Case
   	else if(ptpTransport==e_PTP_MODE_MICROWAVE)
      {
          fbias=fbias_large=fbias_small=1000.0;
          rbias=rbias_large=rbias_small=0.0;
      } // End Microwave Transport Case
   }//end pacing every 16th
	hold_res_acc_avg = hold_res_acc;
//   if(0)
   if(ptpTransport==e_PTP_MODE_MICROWAVE)
   {  
      //REVERSE CHANNEL LOGIC
      if(floor_density_small_r < 28000) //was 28250
      {
		   min_reverse.sdrift_min_1024= -rfe_turbo_phase[1]+ (INT32)hold_res_acc_avg; //GPZ June 2011 adjust for holdover residual assume SENSE is 1	
         if(cluster_state_r == 1)
         {
            turbo_cluster_rev = 20;
             cluster_state_r = 0;
         }
      }
      else
      {
		   min_reverse.sdrift_min_1024= -rfe_turbo_phase_small[1]+ (INT32)hold_res_acc_avg; //GPZ June 2011 adjust for holdover residual assume SENSE is 1
         rfe_turbo_phase[1]=rfe_turbo_phase_small[1];
         rbias=rbias_small;
         if(cluster_state_r == 0)
         {
            turbo_cluster_rev = 20;
            cluster_state_r = 1;
         }

      }
      //FORWARD CHANNEL LOGIC    
      if(floor_density_small_f < 7500) 
      {
         if(cluster_state_f > 0)
         {
            // TODO set turbo cluster flag forward
//        	   debug_printf(GEORGE_PTP_PRT,"exit forward small debug:%ld\n", (long int)cluster_state_f); 

            if(cluster_state_f==1)
            {
//          	   debug_printf(GEORGE_PTP_PRT,"exit forward small:%ld\n", (long int)cluster_state_f); 
               turbo_cluster_for = 20;
               cluster_state_f = 0;

            }
            else cluster_state_f--; 
         }
         if(1)  //always use delta cluster approach for now
         {
            min_forward.sdrift_min_1024= rfe_turbo_phase_large_band[0]- (INT32)hold_res_acc_avg; //November use 10K band
            rfe_turbo_phase[0]=rfe_turbo_phase_large_band[0];
            fbias=fbias_large;
         }
         else
         {
      	   min_forward.sdrift_min_1024= rfe_turbo_phase[0]- (INT32)hold_res_acc_avg; //November use 10K band
         }
      }
      else
      {
		   min_forward.sdrift_min_1024= rfe_turbo_phase_small[0]- (INT32)hold_res_acc_avg; //GPZ June 2011 adjust for holdover residual assume SENSE is 1
         rfe_turbo_phase[0]=rfe_turbo_phase_small[0];
         fbias=fbias_small;
         if(cluster_state_f == 0)
         {
            // TODO set turbo cluster flag forward
            turbo_cluster_for = 20;
         }
         cluster_state_f= 1;

      }

	}
	else if(rfe_turbo_flag)
	{
//         debug_printf(GEORGE_PTP_PRT,"Load_State_Print: %ld: %ld\n",(long int)LoadState,(long int)traffic_mode_A_flag); 

//        if(0) //TEST TEST TEST  
      if(((floor_density_small_r < 30000.0)&&(LoadState == LOAD_CALNEX_10)) || (LoadState == LOAD_G_8261_10)||(LoadState == LOAD_GENERIC_HIGH)) //was 28250
      {
//         debug_printf(GEORGE_PTP_PRT,"High_Load_Spot\n"); 

         if((traffic_mode_A_flag)||(LoadState != LOAD_CALNEX_10)) //GPZ Feb 2 target new model only for B
         {
		      min_reverse.sdrift_min_1024= -rfe_turbo_phase[1]+ (INT32)hold_res_acc_avg; //GPZ June 2011 adjust for holdover residual assume SENSE is 1	
//     	      debug_printf(GEORGE_PTP_PRT,"MIN_1024 not 5n reverse norm:%ld\n", (long int)min_reverse.sdrift_min_1024); 
         }
         else  // Feb 2 new small based model for B
         {
   		   min_reverse.sdrift_min_1024= -rfe_turbo_phase_small[1]+ (INT32)hold_res_acc_avg; //GPZ June 2011 adjust for holdover residual assume SENSE is 1
            rfe_turbo_phase[1]=rfe_turbo_phase_small[1];
            rbias = rbias_small;
         }
         if(cluster_state_r == 1)
         {
            turbo_cluster_rev = 20;
//      	   debug_printf(GEORGE_FAST_PRT,"exit crossover reverse:%ld\n", (long int)floor_density_small_r); 

             cluster_state_r = 0;
         }
      }
      else
      {
//         debug_printf(GEORGE_PTP_PRT,"Low_Load_Spot\n"); 

		   min_reverse.sdrift_min_1024= -rfe_turbo_phase_small[1]+ (INT32)hold_res_acc_avg; //GPZ June 2011 adjust for holdover residual assume SENSE is 1
//    	   debug_printf(GEORGE_PTP_PRT,"MIN_1024 small reverse:%ld\n", (long int)min_reverse.sdrift_min_1024); 

         rfe_turbo_phase[1]=rfe_turbo_phase_small[1];
         rbias=rbias_small;
         if(cluster_state_r == 0)
         {
            turbo_cluster_rev = 20;
//      	   debug_printf(GEORGE_FAST_PRT,"enter crossover reverse:%ld\n", (long int)floor_density_small_r); 
            cluster_state_r = 1;
         }

      }
      //GPZ for LOAD_Calnex_10 case try different break point then 750
      if( ((floor_density_small_f < 500.0)&&(LoadState == LOAD_CALNEX_10))||
          ((floor_density_small_f < 7500.0)&&(LoadState == LOAD_G_8261_5 )) ||
          ((floor_density_small_f <  500.0)&&(LoadState == LOAD_G_8261_10)) ||
           (LoadState == LOAD_GENERIC_HIGH)  )
      {
         if(cluster_state_f > 0)
         {
            // TODO set turbo cluster flag forward
 //       	   debug_printf(GEORGE_PTP_PRT,"exit forward small debug:%ld\n", (long int)cluster_state_f); 

            if(cluster_state_f==1)
            {
//          	   debug_printf(GEORGE_PTP_PRT,"exit forward small:%ld\n", (long int)cluster_state_f); 
               turbo_cluster_for = 20;
               cluster_state_f = 0;

            }
            else cluster_state_f--; 
         }
//         if((LoadState == LOAD_CALNEX_10)|| (LoadState == LOAD_G_8261_10))
         if((LoadState == LOAD_CALNEX_10))
         {
            min_forward.sdrift_min_1024= rfe_turbo_phase_large_band[0]- (INT32)hold_res_acc_avg; //November use 10K band
            rfe_turbo_phase[0]=rfe_turbo_phase_large_band[0];
            fbias=fbias_large;
//    	      debug_printf(GEORGE_PTP_PRT,"MIN_1024 large forward Cal 10:%ld %le %le\n", (long int)min_forward.sdrift_min_1024, fbias, hold_res_acc_avg); 

         }
         else
         {
      	   min_forward.sdrift_min_1024= rfe_turbo_phase[0]- (INT32)hold_res_acc_avg; //November use 10K band
//    	      debug_printf(GEORGE_PTP_PRT,"MIN_1024 large forward Not Cal 10:%ld %le %le\n", (long int)min_forward.sdrift_min_1024, fbias, hold_res_acc_avg); 

         }
      }
      else
      {
		   min_forward.sdrift_min_1024= rfe_turbo_phase_small[0]- (INT32)hold_res_acc_avg; //GPZ June 2011 adjust for holdover residual assume SENSE is 1
         rfe_turbo_phase[0]=rfe_turbo_phase_small[0];
         fbias=fbias_small;
//  	      debug_printf(GEORGE_PTP_PRT,"MIN_1024 small forward:%ld\n", (long int)min_forward.sdrift_min_1024); 

         if(cluster_state_f == 0)
         {
            // TODO set turbo cluster flag forward
            turbo_cluster_for = 20;
         }
         cluster_state_f= 1;

      }

	}
	else
	{
		min_forward.sdrift_min_1024=IPD.phase_floor_avg[FORWARD_GM1]+(INT32)fbias; //GPZ Sept 2010 use floor average NOV 2010 add bias compensation here
      min_forward.sdrift_min_1024+=detilt_correction_for;
		min_reverse.sdrift_min_1024=IPD.phase_floor_avg[REVERSE_GM1]-(INT32)rbias; //GPZ Sept 2010 use floor average NOV 2010 add bias compensation here
      min_reverse.sdrift_min_1024+=detilt_correction_rev;
	}

			#if 1			// Measurement  Debug channel 
			if(((IPD.phase_index & 0xF)==1)&&(FLL_Elapse_Sec_Count_Use>3600)&& (FLL_Elapse_Sec_Count_Use<822000))  //use 17700 for up transient
 			{

			debug_printf(GEORGE_FAST_PRT,"skew:r:%d rcwl:%ld rcws:%ld mean:%ld min_r:%ld min_fr:%ld wbm:%ld density:%ld sdensity:%ld ratio:%le mode_w:%ld rbias:%ld rbias_s:%ld working_flr: %ld\n",
					FLL_Elapse_Sec_Count_Use,
					(long int)(min_oper_thres_large_r),
					(long int)(min_oper_thres_small_r),
               (long int)(rfe_turbo_phase_avg_large[1]),
               (long int)(rfe_turbo_phase_avg[1]),
               (long int)(rfe_turbo_phase_avg_small[1]),
               (long int) working_bias_metric_rw, 
					(long int)floor_density_r,
               (long int)floor_density_small_r,
               (double)(floor_density_r/floor_density_small_r),
               (long int) ofw_start_r,
					(long int)rbias,
					(long int)rbias_small,
					(long int)IPD.working_phase_floor[1]
		        );

			}
			#endif
			#if 0
			// Measurement  Debug channel 
			if(((IPD.phase_index & 0xF)==1)&&(FLL_Elapse_Sec_Count_Use>3600)&& (FLL_Elapse_Sec_Count_Use<120000))  //use 17700 for up transient
			{
			debug_printf(GEORGE_FAST_PRT,"skew:f:%d fcwl:%ld fcws:%ld mean:%ld min_f:%ld min_f:%ld  m_density:%ld density:%ld sdensity:%ld ratio:%le mode_w:%ld fbias:%ld fbias_s:%ld working_flr: %ld\n",
					FLL_Elapse_Sec_Count_Use,
					(long int)(min_oper_thres_large_f),
					(long int)(min_oper_thres_small_f),
               (long int)(rfe_turbo_phase_avg_large[0]),
               (long int)(rfe_turbo_phase_avg[0]),
               (long int)(rfe_turbo_phase_avg_small[0]),
 					(long int)floor_density_large_f,
               (long int)floor_density_f,
               (long int)floor_density_small_f,
               (double)(floor_density_f/floor_density_small_f),
               (long int) ofw_start_f,
					(long int)fbias,
					(long int)fbias_small,
					(long int)IPD.working_phase_floor[0]
		        );
			}
			#endif

	if((IPD.phase_index & 0xF)==4) //every 16th cycle decimate try every 16th cycle June 2010
	{	
		// update forward offset window
    	if(ofw_cluster_iter_f>= F_OFW_ITR)  
   	{
	 		ofw_cluster_iter_f=0;	
//	 		debug_printf(GEORGE_PTP_PRT, "reset OFD\n");
		}
		if(  (ofw_cluster_iter_f==0) && (!Is_Sync_timeout(GRANDMASTER_PRIMARY))&&(low_warning_for ==0)&&(high_warning_for ==0))
		{
  			if(ofw_cur_dir_f!=ofw_prev_dir_f)
  			{
				ofw_uslew_f=0;
				if(ofw_gain_f >4) ofw_gain_f /=2; //was min of 2 
   		}
   		else
			{
				ofw_uslew_f++;
				if(ofw_uslew_f>1)
	   			{
   					ofw_uslew_f=0;
					if(ofw_gain_f < 8 )ofw_gain_f*=2;  //GPZ APRIL 2011 reduce maximum gain
				}
 	  		}
			ofw_prev_dir_f=ofw_cur_dir_f;
			if(IPD.ofw_cluster_count[FORWARD_GM1]<(256)) // was 256 TODO INCREASE TO 256 one forward flow is fixed increased from 64 March 21 2011
 			{
//		 	 	debug_printf(GEORGE_PTP_PRT, "decrease for ofw: %6ld iter: %d start %ld\n",ofw_cluster_total_prev_f,ofw_cluster_iter_f,ofw_start_f);
				ofw_start_thres_f =2*min_oper_thres_f;
				if(ofw_start_thres_f<1000) ofw_start_thres_f=1000;
  				if(ptpTransport== e_PTP_MODE_DSL)
   				{
   				if(ofw_start_thres_f<100000) ofw_start_thres_f=100000;
   				}
		   		if(ofw_start_f>ofw_start_thres_f)// was OFFSET_THRES
    			{
	   				if(turbo_cluster_for>0)
	   				{
		   				ofw_start_f-=(30*ofw_gain_f); //was  30
	   				}
    				else
    				{
 		   				ofw_start_f-=(30*ofw_gain_f); //was 30
     				}
					if(ofw_start_f< min_oper_thres_f) ofw_start_f=min_oper_thres_f;
    				ofw_cur_dir_f=1;
    			}
    			else
    			{
    				ofw_start_f=ofw_start_thres_f;
    			}
 			}
 			else
 			{
 //				debug_printf(GEORGE_PTP_PRT, "increase for ofw: %6ld iter: %d start %ld\n",ofw_cluster_total_prev_f,ofw_cluster_iter_f,ofw_start_f);
				if(ofw_start_f< mode_width_max_oper)
				{
				
	   				if(turbo_cluster_for>0)
	   				{
 		   				ofw_start_f+=(30*ofw_gain_f); //was 30
	   				}
    				else
    				{
 		   				ofw_start_f+=(30*ofw_gain_f); //was 30 
     				}
					ofw_cur_dir_f=-1;
				}
			}
 				
		} //complete forward offset floor logic
		// update reverse offset window
    	if(ofw_cluster_iter_r>= F_OFW_ITR)  
   		{
	 		ofw_cluster_iter_r=0;	
//			debug_printf(GEORGE_PTP_PRT, "reset OFD\n");
		}
		if((ofw_cluster_iter_r==0)&&(!Is_Delay_timeout()) && (low_warning_rev==0)&& (high_warning_rev==0) )
		{
  			if(ofw_cur_dir_r!=ofw_prev_dir_r)
  			{
				ofw_uslew_r=0;
				if(ofw_gain_r >4) ofw_gain_r /=2; //was min of 2 
   		}
   		else
			{
				ofw_uslew_r++;
				if(ofw_uslew_r>1)
   			{
   					ofw_uslew_r=0;
					if(ofw_gain_r < 8 )ofw_gain_r*=2;  //GPZ APRIL 2011 reduce maximum gain
				}
   		}
			ofw_prev_dir_r=ofw_cur_dir_r;

			if(IPD.ofw_cluster_count[REVERSE_GM1]<(256)) // was 256 TODO INCREASE TO 256 one forward flow is fixed increased from 64 March 21 2011
 			{
//		 		debug_printf(GEORGE_PTP_PRT, "decrease for ofw: %6ld iter: %d start %ld\n",ofw_cluster_total_prev_r,ofw_cluster_iter_r,ofw_start_r);
				ofw_start_thres_r =2*min_oper_thres_r;
				if(ofw_start_thres_r<1000) ofw_start_thres_r=1000;
 				if(ptpTransport== e_PTP_MODE_DSL)
   				{
   					if(ofw_start_thres_r<100000) ofw_start_thres_r=100000;
   				}
				
	   			if(ofw_start_r>ofw_start_thres_r)// was OFFSET_THRES
    			{
 	   				if(turbo_cluster_rev>0)
	   				{
		   				ofw_start_r-=(30*ofw_gain_r); //was 30
	   				}
    				else
    				{
 		   				ofw_start_r-=(30*ofw_gain_r); //was 30
     				}
					if(ofw_start_r< min_oper_thres_r) ofw_start_r=min_oper_thres_r;
    				ofw_cur_dir_r=1;
    			}
    			else
    			{
    				ofw_start_r=ofw_start_thres_r;
    			}
 			}
 			else
 			{
 //				debug_printf(GEORGE_PTP_PRT, "increase for ofw: %6ld iter: %d start %ld\n",ofw_cluster_total_prev_r,ofw_cluster_iter_r,ofw_start_r);
				if(ofw_start_r< mode_width_max_oper)
				{
				
 	   				if(turbo_cluster_rev>0)
	   				{
 		   				ofw_start_r+=(30*ofw_gain_r); //was 30
	   				}
    				else
    				{
 		   				ofw_start_r+=(30*ofw_gain_r); //was 30
     				}
					ofw_cur_dir_r=-1;
				}
			}
		} //complete reverse offset floor logic
	} //end every 4th cycle decimation
	if((IPD.phase_index & 0x3)==1) //every 4th cycle 
	{	
	// Common cluster test point logic
	if ((ptpTransport==e_PTP_MODE_ETHERNET)||(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS)) //ethernet specific settings
	{
//		ctp=256; //New Larger Cluster April 2011
//		ctp=64; // April 26 2011 USE smaller more floor centric cluster
//		ctp=76; // May 8 TEST TEST TEST
		ctp=32; // Ideal Reverse Density for TC13 Calnex
	}
	else
	{
		ctp=128; //baseline cluster width GPZ Feb 2 increase to 128
	}	
	
	if(enterprise_flag)
	{
		ctp=16; //precise time mode for enterprise
	}
	if(mode_jitter==1)
	{
		ctp=256; // was 64 GPZ experiment with larger cluster widths for DSL cases 128 to 64 Jan 2010		
	}
	if(var_cal_flag)
	{
		ctp=512; // run at maximum during cal
	}
	// Forward floor cluster logic
	if( (IPD.floor_cluster_count[FORWARD_GM1]) > ctp) //was 32 experiment with 64 for VF
	{
		if(min_oper_thres_f > (min_oper_thres_oper+cluster_inc_down_f))
		{
			min_oper_thres_f-=cluster_inc_down_f; //was 15
			cluster_inc_sense_f=0;
		}
	}
	else
	{
		if(min_oper_thres_f<cluster_width_max) //tight bound for TOD apps
		{
//				if(start_in_progress||(RFE.first))
//				{
//					min_oper_thres_f+=(8*cluster_inc_f); //was 25
//					cluster_inc_sense_f=1;
//  			}
//    			else
			{
    			min_oper_thres_f+= cluster_inc_up_f; //was 25
  				cluster_inc_sense_f=1;
    		}	
		}
	}
	if ((ptpTransport==e_PTP_MODE_ETHERNET)||(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS))
	{
      if(LoadState == LOAD_GENERIC_HIGH)  
      {
		   min_oper_thres_f=8000000; //Base  2000000
		   min_oper_thres_small_f=100000; //OCT 12 was 2000 New approach for ethernet use fixed cluster width and use CTP to correct
		   min_oper_thres_large_f=10000000; //OCT 12 was 2000 New approach for ethernet use fixed cluster width and use CTP to correct
      }
      else
      {    
		   min_oper_thres_f=15000; //OCT 12 New approach for ethernet use fixed cluster width and use CTP to correct
		   min_oper_thres_small_f=2000; //OCT 12 was 2000 New approach for ethernet use fixed cluster width and use CTP to correct
		   min_oper_thres_large_f=125000; //OCT 12 was 2000 New approach for ethernet use fixed cluster width and use CTP to correct
      }
	}
   else if (ptpTransport==e_PTP_MODE_MICROWAVE)
   {
		   min_oper_thres_f=120000; //Base  120000
		   min_oper_thres_small_f=40000; //OCT 12 was 2000 New approach for ethernet use fixed cluster width and use CTP to correct
		   min_oper_thres_large_f=8000000; //OCT 12 was 2000 New approach for ethernet use fixed cluster width and use CTP to correct
   }

#if 0	
	// adapt cluster inc new April 2010
	if(cluster_inc_sense_f ==cluster_inc_sense_prev_f)
	{
		cluster_inc_slew_count_f++;
			if(cluster_inc_slew_count_f > 100) // underslew condition was 188
		{
			cluster_inc_slew_count_f=0;
			cluster_inc_count_f=0;
			if(cluster_inc_f< cluster_inc) cluster_inc_f+=32;
			else cluster_inc_f=cluster_inc;
		}
	}
	else
	{
		cluster_inc_count_f++; 
		cluster_inc_slew_count_f=0;
			if(cluster_inc_count_f > 16) //was 32
		{
			cluster_inc_slew_count_f=0;
			cluster_inc_count_f=0;
			if(cluster_inc_f>(2*CLUSTER_INC)) cluster_inc_f =cluster_inc_f/2;
			else if(cluster_inc_f>50) cluster_inc_f -=8; //increase minimum from 16
		}
  	}
   	cluster_inc_sense_prev_f=cluster_inc_sense_f;
#endif   	
   	if(1)  //force static mode always but use 4x call of update phase to compensate
//	if(OCXO_Type() != e_TCXO) //use static cluster width if not mini
	{
 //  			if(!start_in_progress_PTP)
   			{
   				if(ptpTransport== e_PTP_MODE_DSL)
   				{
   					cluster_inc_f=2000;
                  cluster_inc_up_f=2000;  // was 19
                  cluster_inc_down_f=2000;// was 19

   				}
   				else
   				{
                  if((OCXO_Type()) != e_MINI_OCXO) 
                  {
                      cluster_inc_up_f=9;  // was 19
                      cluster_inc_down_f=9;// was 19
                  }
	   				else if(turbo_cluster_for>0)
   					{
//	   					cluster_inc_f=12 + (64*turbo_cluster_for)/64; //GPZ March 2011 2x speed APRIL 25 2011 start at 256 then taper
                     cluster_inc_up_f=19; //was 19
                     cluster_inc_down_f=19;

   					}
   					else
   					{
                     cluster_inc_up_f=19;
                     cluster_inc_down_f=19;
   					}
   				}
   			}
	}
 	// Reverse floor cluster logic
	if( (IPD.floor_cluster_count[REVERSE_GM1]) > ctp) //was 32 experiment with 64 for VF
	{
		if(min_oper_thres_r > (min_oper_thres_oper+cluster_inc_down_r))
		{
			min_oper_thres_r-=cluster_inc_down_r; //was 15
			cluster_inc_sense_r=0;
//			debug_printf(GEORGE_PTP_PRT, "IPD reverse cluster down step: %6d, %6ld, %6ld, %6ld, %6ld\n",IPD.phase_index,cluster_inc,cluster_inc_f,cluster_inc_r,min_oper_thres_r);
		}
	}
	else
	{
		if(min_oper_thres_r<cluster_width_max) //tight bound for TOD apps
		{
//				if(start_in_progress||(RFE.first))
//				{
//					min_oper_thres_r+=(8*cluster_inc_r); //was 25
//					cluster_inc_sense_r=1;
//    			}
//    			else
			{
    			min_oper_thres_r+= cluster_inc_up_r; //was 25
  				cluster_inc_sense_r=1;



//	 			debug_printf(GEORGE_PTP_PRT, "IPD reverse cluster up step: %6d, %6ld, %6ld, %6ld, %6ld\n",IPD.phase_index,cluster_inc,cluster_inc_f,cluster_inc_r,min_oper_thres_r);
  	   		}	
		}
	}
	if ((ptpTransport==e_PTP_MODE_ETHERNET)||(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS))
   {
      if(LoadState == LOAD_GENERIC_HIGH)  
      {
	   	min_oper_thres_r=8000000; //Base  2000000
		   min_oper_thres_small_r=100000; 
	   	min_oper_thres_large_r=10000000; 
      }
	   else
      {
		   min_oper_thres_r=7500; //OCT 12 New approach for ethernet use fixed cluster width and use CTP to correct
	   	min_oper_thres_small_r=2000; //OCT 12 was 2000 New approach for ethernet use fixed cluster width and use CTP to correct
		   min_oper_thres_large_r=125000; //OCT 12 was 2000 New approach for ethernet use fixed cluster width and use CTP to correct
	   }
   }
   else if (ptpTransport==e_PTP_MODE_MICROWAVE)
   {
		   min_oper_thres_r=120000; //Base  2000000
		   min_oper_thres_small_r=40000; //OCT 12 was 2000 New approach for ethernet use fixed cluster width and use CTP to correct
		   min_oper_thres_large_r=8000000; //OCT 12 was 2000 New approach for ethernet use fixed cluster width and use CTP to correct
   }

#if 0	
	// adapt cluster inc new April 2010
	if(cluster_inc_sense_r ==cluster_inc_sense_prev_r)
	{
		cluster_inc_slew_count_r++;
			if(cluster_inc_slew_count_r > 100) // underslew condition was 188
		{
			cluster_inc_slew_count_r=0;
			cluster_inc_count_r=0;
			if(cluster_inc_r< cluster_inc) cluster_inc_r+=32;
			else cluster_inc_r=cluster_inc;
//			debug_printf(GEORGE_PTP_PRT, "IPD underslew write cluster_inc_r: %6d, %6ld, %6ld, %6ld, %6ld\n",IPD.phase_index,cluster_inc,cluster_inc_f,cluster_inc_r,min_oper_thres_r);
			
		}
	}
	else
	{
		cluster_inc_count_r++; 
		cluster_inc_slew_count_r=0;
			if(cluster_inc_count_r > 16) 
		{
			cluster_inc_slew_count_r=0;
			cluster_inc_count_r=0;
			if(cluster_inc_r>(2*CLUSTER_INC)) cluster_inc_r =cluster_inc_r/2;
			else if(cluster_inc_r>50) cluster_inc_r -=8; //increase minimum from 16
//			debug_printf(GEORGE_PTP_PRT, "IPD over slew write cluster_inc_r: %6d, %6ld, %6ld, %6ld, %6ld\n",IPD.phase_index,cluster_inc,cluster_inc_f,cluster_inc_r,min_oper_thres_r);
			
		}
  	}
   	cluster_inc_sense_prev_r=cluster_inc_sense_r;
#endif   	
   	if(1)  //force static mode always but use 4x call of update phase to compensate
//	if(OCXO_Type() != e_TCXO) //use static cluster width if not mini
	{
 //  			if(!start_in_progress_PTP)
   			{
   			
   			
  				if(ptpTransport== e_PTP_MODE_DSL)
   				{
   					cluster_inc_r=2000;
                  cluster_inc_up_r=2000;
                  cluster_inc_down_r=2000;

   				}
   				else
   				{
                  if((OCXO_Type()) != e_MINI_OCXO) 
                  {
                      cluster_inc_up_r=9;
                      cluster_inc_down_r=9;

                  }
	   				else if(turbo_cluster_rev>0)
   					{
                      cluster_inc_up_r=500;  //was 14
                      cluster_inc_down_r=500;//was 14

   					}
   					else
   					{
                      cluster_inc_up_r=3;  //was 14
                      cluster_inc_down_r=3;//was 14 
   					}
   				}
//				debug_printf(GEORGE_PTP_PRT, "IPD static write cluster_inc_r: %6d, %6ld, %6ld, %6ld, %6ld\n",IPD.phase_index,cluster_inc,cluster_inc_f,cluster_inc_r,min_oper_thres_r);
   			}
	}
   	
	} //end every 4th cycle   	
}

///////////////////////////////////////////////////////////////////////
// This function initializes new input packet processing
//typedef struct
// {
// 	int_32 floor_cluster_count; //count over phase interval of samples in floor cluster
// 	int_32 ofw_cluster_count;	//count over phase interval of samples in ceiling cluster
// 	
// 	long long floor_cluster_sum;
// 	long long ofw_cluster_sum;
// 	
// 	int_32 floor;  //minimum in one  interval use to replace working phase floor
// 	int_32 ceiling;//maximum in one  interval
// } 
//  INPROCESS_CHANNEL_STRUCT;
//  
//  typedef struct
//  {
//  	int_32 phase_floor_avg; // cluster phase floor average
//  	int_32 phase_ceiling_avg; //cluster phase ceiling average
//  	int_32 working_phase_floor; //baseline phase_floor 
//  	INPROCESS_CHANNEL_STRUCT IPC; 
//  }
//  INPROCESS_PHASE_STRUCT;
//  typedef struct
//  {
//  	int_16 phase_index; 
//  	INPROCESS_PHASE_STRUCT PA,PB,PC,PD;
//  }
//  INPUT_PROCESS_DATA;
//
///////////////////////////////////////////////////////////////////////
void init_ipp(void)
{
	int i;	
	IPD.phase_index=0;
//   debug_printf(GEORGE_PTP_PRT, "Why_init Inside init_ipp\n"); //TEST TEST TEST

	for(i=0;i<2;i++)
	{
		IPD.PA[i].phase_floor_avg= 0;
		IPD.PA[i].phase_ceiling_avg=0;
		IPD.PA[i].working_phase_floor=MAXMIN;
		IPD.PA[i].IPC.floor_set[0]=MAXMIN;
		IPD.PA[i].IPC.floor_set[1]=MAXMIN;
		IPD.PA[i].IPC.floor_set[2]=MAXMIN;
		IPD.PA[i].IPC.floor_set[3]=MAXMIN;
		IPD.PA[i].IPC.floor_set[4]=MAXMIN;
 		IPD.PA[i].IPC.floor_set[5]=MAXMIN;
 		IPD.PA[i].IPC.floor_set[6]=MAXMIN;
  		IPD.PA[i].IPC.floor_set[7]=MAXMIN;

		IPD.PA[i].IPC.ceiling=-MAXMIN;
		IPD.PB[i].phase_floor_avg= 0;
		IPD.PB[i].phase_ceiling_avg=0;
		IPD.PB[i].working_phase_floor=MAXMIN;
		IPD.PB[i].IPC.floor_set[0]=MAXMIN;
		IPD.PB[i].IPC.floor_set[1]=MAXMIN;
		IPD.PB[i].IPC.floor_set[2]=MAXMIN;
		IPD.PB[i].IPC.floor_set[3]=MAXMIN;
		IPD.PB[i].IPC.floor_set[4]=MAXMIN;
 		IPD.PB[i].IPC.floor_set[5]=MAXMIN;
 		IPD.PB[i].IPC.floor_set[6]=MAXMIN;
  		IPD.PB[i].IPC.floor_set[7]=MAXMIN;

		IPD.PB[i].IPC.ceiling=-MAXMIN;
		IPD.PC[i].phase_floor_avg= 0;
		IPD.PC[i].phase_ceiling_avg=0;
		IPD.PC[i].working_phase_floor=MAXMIN;
		IPD.PC[i].IPC.floor_set[0]=MAXMIN;
		IPD.PC[i].IPC.floor_set[1]=MAXMIN;
		IPD.PC[i].IPC.floor_set[2]=MAXMIN;
		IPD.PC[i].IPC.floor_set[3]=MAXMIN;
		IPD.PC[i].IPC.floor_set[4]=MAXMIN;
 		IPD.PC[i].IPC.floor_set[5]=MAXMIN;
 		IPD.PC[i].IPC.floor_set[6]=MAXMIN;
  		IPD.PC[i].IPC.floor_set[7]=MAXMIN;

		IPD.PC[i].IPC.ceiling=-MAXMIN;
		IPD.PD[i].phase_floor_avg= 0;
		IPD.PD[i].phase_ceiling_avg=0;
		IPD.PD[i].working_phase_floor=MAXMIN;
		IPD.PD[i].IPC.floor_set[0]=MAXMIN;
		IPD.PD[i].IPC.floor_set[1]=MAXMIN;
		IPD.PD[i].IPC.floor_set[2]=MAXMIN;
		IPD.PD[i].IPC.floor_set[3]=MAXMIN;
		IPD.PD[i].IPC.floor_set[4]=MAXMIN;
 		IPD.PD[i].IPC.floor_set[5]=MAXMIN;
 		IPD.PD[i].IPC.floor_set[6]=MAXMIN;
  		IPD.PD[i].IPC.floor_set[7]=MAXMIN;

		IPD.PD[i].IPC.ceiling=-MAXMIN;


		IPD.PE[i].phase_floor_avg= 0;
		IPD.PE[i].phase_ceiling_avg=0;
		IPD.PE[i].working_phase_floor=MAXMIN;
		IPD.PE[i].IPC.floor_set[0]=MAXMIN;
		IPD.PE[i].IPC.floor_set[1]=MAXMIN;
		IPD.PE[i].IPC.floor_set[2]=MAXMIN;
		IPD.PE[i].IPC.floor_set[3]=MAXMIN;
		IPD.PE[i].IPC.floor_set[4]=MAXMIN;
 		IPD.PE[i].IPC.floor_set[5]=MAXMIN;
 		IPD.PE[i].IPC.floor_set[6]=MAXMIN;
  		IPD.PE[i].IPC.floor_set[7]=MAXMIN;

		IPD.PE[i].IPC.ceiling=-MAXMIN;
		IPD.PF[i].phase_floor_avg= 0;
		IPD.PF[i].phase_ceiling_avg=0;
		IPD.PF[i].working_phase_floor=MAXMIN;
		IPD.PF[i].IPC.floor_set[0]=MAXMIN;
		IPD.PF[i].IPC.floor_set[1]=MAXMIN;
		IPD.PF[i].IPC.floor_set[2]=MAXMIN;
		IPD.PF[i].IPC.floor_set[3]=MAXMIN;
		IPD.PF[i].IPC.floor_set[4]=MAXMIN;
 		IPD.PF[i].IPC.floor_set[5]=MAXMIN;
 		IPD.PF[i].IPC.floor_set[6]=MAXMIN;
  		IPD.PF[i].IPC.floor_set[7]=MAXMIN;
		IPD.PF[i].IPC.ceiling=-MAXMIN;
		IPD.PG[i].phase_floor_avg= 0;
		IPD.PG[i].phase_ceiling_avg=0;
		IPD.PG[i].working_phase_floor=MAXMIN;
		IPD.PG[i].IPC.floor_set[0]=MAXMIN;
		IPD.PG[i].IPC.floor_set[1]=MAXMIN;
		IPD.PG[i].IPC.floor_set[2]=MAXMIN;
		IPD.PG[i].IPC.floor_set[3]=MAXMIN;
		IPD.PG[i].IPC.floor_set[4]=MAXMIN;
 		IPD.PG[i].IPC.floor_set[5]=MAXMIN;
 		IPD.PG[i].IPC.floor_set[6]=MAXMIN;
  		IPD.PG[i].IPC.floor_set[7]=MAXMIN;

		IPD.PG[i].IPC.ceiling=-MAXMIN;
		IPD.PH[i].phase_floor_avg= 0;
		IPD.PH[i].phase_ceiling_avg=0;
		IPD.PH[i].working_phase_floor=MAXMIN;
		IPD.PH[i].IPC.floor_set[0]=MAXMIN;
		IPD.PH[i].IPC.floor_set[1]=MAXMIN;
		IPD.PH[i].IPC.floor_set[2]=MAXMIN;
		IPD.PH[i].IPC.floor_set[3]=MAXMIN;
		IPD.PH[i].IPC.floor_set[4]=MAXMIN;
 		IPD.PH[i].IPC.floor_set[5]=MAXMIN;
 		IPD.PH[i].IPC.floor_set[6]=MAXMIN;
  		IPD.PH[i].IPC.floor_set[7]=MAXMIN;
		IPD.PH[i].IPC.ceiling=-MAXMIN;
		
	}
}





///////////////////////////////////////////////////////////////////////
// This function determines if current min is valid and if necessary 
// generated a replacement min
///////////////////////////////////////////////////////////////////////
static int base_block = 32;
void min_valid(int forward)
{
	int i;
	INT32 test_offset,in_min, local_thres;
	int Cluster_Count_Thres;
	int LVM_Count_Thres;
/* find candidate relacement minimum */
	/*start with min of mins*/
	if(forward)
	{
		rm_f=0;
		rmf_f=MAXMIN;
		rmc_f=0;
		if(rmfirst_f)
		{
			rmfirst_f=0;
			rmi_f=0;
			for(i=0;i<rm_size_oper;i++)
			{
				rmw_f[i]=MAXMIN;
			}
		}
	}
	else
	{
		rm_r=0;
		rmf_r=MAXMIN;
		rmc_r=0;
		if(rmfirst_r)
		{
			rmfirst_r=0;
			rmi_r=0;
			for(i=0;i<rm_size_oper;i++)
			{
				rmw_r[i]=MAXMIN;
			}
		}
				
	}
	// add latest min to search window
	if(forward)
	{
			rmw_f[rmi_f]=min_forward.sdrift_min_1024;
			rmi_f=((rmi_f+1)%rm_size_oper);
			in_min=min_forward.sdrift_min_1024;
	}
	else
	{
			rmw_r[rmi_r]=min_reverse.sdrift_min_1024;
			rmi_r=((rmi_r+1)%rm_size_oper);
			in_min=min_reverse.sdrift_min_1024;
	}
	#if ((ASYMM_CORRECT_ENABLE==1)||(HYDRO_QUEBEC_ENABLE==1))
	base_block=128;
	Cluster_Count_Thres=128;
	LVM_Count_Thres = 128;	
	#else
	base_block=32;
	Cluster_Count_Thres=50;
	LVM_Count_Thres = 32; //was 4	
	#endif
	if(ptpTransport==e_PTP_MODE_SLOW_ETHERNET) //DEC 2010 extend settling window in slow settling conditions
	{
		base_block=128;
	}	
	if((start_in_progress_PTP||(RFE.first))) //limit search to latest 32
	{
	    for(i=0;i<base_block;i++)
    	{
			if(forward)
			{
				if(rmw_f[(rmi_f+rm_size_oper-i-4)%rm_size_oper]<rmf_f) rmf_f =  rmw_f[(rmi_f+rm_size_oper-i-4)%rm_size_oper];
			}
			else
			{
				if(rmw_r[(rmi_r+rm_size_oper-i-4)%rm_size_oper]<rmf_r) rmf_r =  rmw_r[(rmi_r+rm_size_oper-i-4)%rm_size_oper];
			
			}
    	}
    }
    else
    {
 	    for(i=0;i<rm_size_oper;i++)
    	{
			if(forward)
			{
				if(rmw_f[i]<rmf_f) rmf_f =  rmw_f[i];
			}
			else
			{
				if(rmw_r[i]<rmf_r) rmf_r =  rmw_r[i];
			
			}
    	}
   
    }
	//Simple clustering: add clustering algorithm to select good min be sure to throw out
	//any min of mins that are too early
	    for(i=0;i<rm_size_oper;i++)
    	{
 			 if(forward)
 			 {
  		 		test_offset=rmw_f[i]-rmf_f;
  		 		if(min_oper_thres_f < 2000) local_thres=2000;
  		 		else local_thres=min_oper_thres_f;
	  	  		 if((test_offset)< 	local_thres)	
    			 {
 					 rm_f +=  rmw_f[i];
  					 rmc_f++;
 	    		 }
 	    		 
 			 }
 			 else
 			 {
		   		test_offset=rmw_r[i]-rmf_r;
  		 		if(min_oper_thres_r < 2000) local_thres=2000;
  		 		else local_thres=min_oper_thres_r;
		   		
	 	   		 if((test_offset)< 	local_thres)	
    			 {
  					 rm_r +=  rmw_r[i];
  					 rmc_r++;
	    		 }
 			 }	   		
     	}
  		if(forward)
  		{
			if(start_in_progress_PTP||(RFE.first))
			{
				//do nothing just use min of mins
				rm_f=rmf_f;
			}
     		else if(rmc_f)
    		{
 				rm_f= rm_f/rmc_f;	
    	  	}
  		}
  		else
  		{
 			if(start_in_progress_PTP||(RFE.first))
			{
				//do nothing just use min of mins
				rm_r=rmf_r;
			}
     		else if(rmc_r)
    		{
 				rm_r= rm_r/rmc_r;	
    	  	}
 			
  		}
		// TODO determine validity and proper outputs
		if(forward)
		{
			mflag_f=0;
			// first see if current min agrees with last valid use extended accept window NOV 2010
			test_offset=min_forward.sdrift_min_1024-lvm_f;
			if(0) //GPZ Feb 18 2011 always use replacement min
//			if((test_offset<(4.0*min_oper_thres_f))&&(test_offset>(-4.0*min_oper_thres_f))&&(rmc_f>Cluster_Count_Thres))
			{
				om_f=min_forward.sdrift_min_1024;
				mflag_f=1;
			}
			else  //see if replacement min agrees with last valid
			{
				test_offset=rm_f-lvm_f;
				if((test_offset<min_oper_thres_f)&&(test_offset>-min_oper_thres_f))
				{
					om_f=rm_f;
					mflag_f=1;
				}
			}
			if(start_in_progress_PTP||(RFE.first)) //overide just use floor at start
			{
					om_f=rm_f;
					mflag_f=1;
			}
			if(mflag_f)
			{
				min_forward.sdrift_min_1024=om_f;
				//TODO MAKE SURE the mflag is used to in update_drive and to force holdover
			}
			else //GPZ OCT 2010 in doubt use last valid min (protect time solution)
			{
				min_forward.sdrift_min_1024=lvm_f;
			}
			// now update last valid
			if(lvm_f<MAXMIN)
			{
				if(rmc_f>LVM_Count_Thres) // at least four in the cluster over 1 minute to replace
				{
					lvm_f=rm_f;
				}
			}
			else
			{
				if(rmc_f>(LVM_Count_Thres/2)) // at least 2 in the cluster over 1 minute to replace
				{
					lvm_f=rm_f;
				}
				
			}
			#if 0
 		 	debug_printf(GEORGE_PTP_PRT, "min_val_f:valid:%2d, indx:%2d, floor:%2d, in_min:%6ld,out_min:%6ld,rmc_f: %2d,lvm_f:%6ld \n",
 		 	mflag_f,
 		 	rmi_f,
 		 	rmf_f,
 		 	min_forward.sdrift_min_1024,
 		 	om_f,
// 		 	rm_f,
 		 	rmc_f,
 		 	lvm_f);
			#endif
			
		}
		else
		{
			mflag_r=0;
			// first see if current min agrees with last valid
			test_offset=min_reverse.sdrift_min_1024-lvm_r;
			if(0) //GPZ Feb 18 2011 always use replacement min
//			if((test_offset<(4.0*min_oper_thres_r))&&(test_offset>(-4.0*min_oper_thres_r))&&(rmc_r>Cluster_Count_Thres))
			{
				om_r=min_reverse.sdrift_min_1024;
				mflag_r=1;
			}
			else  //see if replacement min agrees with last valid
			{
				test_offset=rm_r-lvm_r;
				if((test_offset<min_oper_thres_r)&&(test_offset>-min_oper_thres_r))
				{
					om_r=rm_r;
					mflag_r=1;
				}
			}
			if(start_in_progress_PTP||(RFE.first)) //overide just use floor at start
			{
					om_r=rm_r;
					mflag_r=1;
			}
 			if(mflag_r)
			{
				min_reverse.sdrift_min_1024=om_r;
				//TODO MAKE SURE the mflag is used to in update_drive and to force holdover
			}
			else //GPZ OCT 2010 in doubt use last valid min (protect time solution)
			{
				min_reverse.sdrift_min_1024=lvm_r;
			}
			// now update last valid
			if(lvm_r<MAXMIN)
			{
				if(rmc_r>LVM_Count_Thres) // at least four in the cluster over 1 minute to replace
				{
					lvm_r=rm_r;
				}
			}
			else
			{
				if(rmc_r>(2)) // at least 2 in the cluster over 1 minute to replace
				{
					lvm_r=rm_r;
				}
			}
			#if 0
		 	debug_printf(GEORGE_PTP_PRT, "min_val_r:valid:%2d, indx:%2d, floor:%2d, in_min:%9ld,out_min:%9ld,rmc_r: %2d,lvm_r:%9ld \n",
 		 	mflag_r,
 		 	rmi_r,
 		 	rmf_r,
 		 	min_reverse.sdrift_min_1024,
 		 	om_r,
// 		 	rm_r,
 		 	rmc_r,
 		 	lvm_r);
			#endif		
 			
		}
  
}


//***************************************************************************************
//		 Softclient 2.0 extensions to support new inputs
//
//*****************************************************************************************
#if(NO_PHY==0)
///////////////////////////////////////////////////////////////////////
// This function update input physical signal stats (GPS, GNSS, Sync E ...)
//
//////////////////////////////////////////////////////////////////////
//static INT32 prev_test_offsetA_phy; //test only
static int complete_phase_flag_phy[IPP_CHAN_PHY]={0,0,0,0,0,0};
static int turbo_cluster_phy[IPP_CHAN_PHY]={0,0,0,0,0,0};
static int avg_samples_update_phy[IPP_CHAN_PHY]={0,0,0,0,0,0};
static int phy_pop[IPP_CHAN_PHY]={0,0,0,0,0,0,};
static long int test_offsetA_phy_cur[IPP_CHAN_PHY]={0.0,0.0,0.0,0.0,0.0,0.0};
static long int test_offsetA_phy_lag[IPP_CHAN_PHY]={0.0,0.0,0.0,0.0,0.0,0.0};
static long int test_offsetA_phy_dlag[IPP_CHAN_PHY]={0.0,0.0,0.0,0.0,0.0,0.0};
//static int detilt_blank = 16; 
//static double detilt_correction_phy[IPP_CHAN_PHY]={0.0, 0.0, 0.0, 0.0};

static UINT16 pcount_turbo_phy[IPP_CHAN_PHY];
static INT32 test_offsetA_base[IPP_CHAN_PHY];
#ifdef MODIFY_BY_JIWANG 
UINT8 set_update_phy_flag[IPP_CHAN_PHY] = {0,0,0,0,0,0};
#endif
void update_input_stats_phy(void)
{
	INPUT_CHANNEL_PHY chan_indx_phy;
	FIFO_STRUCT *fifo_phy;
	GPS_PERFORMANCE_STRUCT	*gps_perf;
	int *low_warning_input_phy;
	int *high_warning_input_phy;
	int *pturbo_cluster_phy;
	int packet_count_phy;
	int *pavg_samples_update_phy;
	double delta_offset;
   INT32 *ptest_offsetA_base;
	double *pdetilt_correction_phy,*pdetilt_rate_phy;
   double dtemp;
	INPROCESS_PHASE_STRUCT_PHY *pipc_phy, *pipc2_phy;	
	INT32 chan_offset_phy,test_offsetA_phy,test_offsetB_phy,floor_thres_phy,density_thres_phy;
	UINT16 /*cur_head_phy,cur_tail_phy,*/pcount_phy;
	int i;

#ifdef MODIFY_BY_JIWANG 
	INT64 cur_phase_offset; 
	static UINT8 set_phy_flag_cnt[IPP_CHAN_PHY] = {0,0,0,0,0,0,0};
	static UINT8 clr_phy_flag_cnt[IPP_CHAN_PHY] = {0,0,0,0,0,0,0};
#endif

//	debug_printf(GEORGE_PTP_PRT, "IPD_PHY Entering at index: %ld\n", IPD_PHY.phase_index);

	if(RFE_PHY.first)
	{
		density_thres_phy=1;
	}
	else
	{
		density_thres_phy=min_density_thres_oper;
	}
 	// Set up Current Channel
	for(chan_indx_phy=0;chan_indx_phy<IPP_CHAN_PHY;chan_indx_phy++)
	{
	
	
    	// TODO add Timestamp Align Offset case for in-band case !!!
   		if(SENSE==0)
   		{
 	  			chan_offset_phy= - Initial_Offset;
   		}
   		else
   		{
	  			chan_offset_phy= + Initial_Offset;
   		}
 //           chan_offset_phy=0;


		switch(chan_indx_phy)
		{
    	case GPS_A:
	    	fifo_phy=&GPS_A_Fifo_Local;
	    	gps_perf=&gps_pa;
	    	floor_thres_phy=min_oper_thres_phy[GPS_A];
	    	low_warning_input_phy=&low_warning_phy[GPS_A];
	    	high_warning_input_phy=&high_warning_phy[GPS_A];
	    	pturbo_cluster_phy= &turbo_cluster_phy[GPS_A]; 
	    	pavg_samples_update_phy=&avg_samples_update_phy[GPS_A];
			pdetilt_correction_phy=	&detilt_correction_phy[GPS_A];    	
			pdetilt_rate_phy=	&detilt_rate_phy[GPS_A]; 
         ptest_offsetA_base= &test_offsetA_base[GPS_A];   	
		break;
 	  	case GPS_B:
	    	fifo_phy=&GPS_B_Fifo_Local; //Re-use GPS_A FIFO for now//jiwang
	    	gps_perf=&gps_pb;
	    	floor_thres_phy=min_oper_thres_phy[GPS_B];
	    	low_warning_input_phy=&low_warning_phy[GPS_B];
	    	high_warning_input_phy=&high_warning_phy[GPS_B];
	    	pturbo_cluster_phy= &turbo_cluster_phy[GPS_B]; 
	    	pavg_samples_update_phy=&avg_samples_update_phy[GPS_B];
			pdetilt_correction_phy=	&detilt_correction_phy[GPS_B];    	
			pdetilt_rate_phy=	&detilt_rate_phy[GPS_B];    	
         ptest_offsetA_base= &test_offsetA_base[GPS_B];   	
	    	
		break;

 	  	case GPS_C:
	    	fifo_phy=&GPS_C_Fifo_Local; //Re-use GPS_A FIFO for now//jiwang
	    	gps_perf=&gps_pc;
	    	floor_thres_phy=min_oper_thres_phy[GPS_C];
	    	low_warning_input_phy=&low_warning_phy[GPS_C];
	    	high_warning_input_phy=&high_warning_phy[GPS_C];
	    	pturbo_cluster_phy= &turbo_cluster_phy[GPS_C]; 
	    	pavg_samples_update_phy=&avg_samples_update_phy[GPS_C];
			pdetilt_correction_phy=	&detilt_correction_phy[GPS_C];    	
			pdetilt_rate_phy=	&detilt_rate_phy[GPS_C];    	
         ptest_offsetA_base= &test_offsetA_base[GPS_C];   	
	    	
		break;

 	  	case GPS_D:
	    	fifo_phy=&GPS_D_Fifo_Local; //Re-use GPS_A FIFO for now//jiwang
	    	gps_perf=&gps_pd;
	    	floor_thres_phy=min_oper_thres_phy[GPS_D];
	    	low_warning_input_phy=&low_warning_phy[GPS_D];
	    	high_warning_input_phy=&high_warning_phy[GPS_D];
	    	pturbo_cluster_phy= &turbo_cluster_phy[GPS_D]; 
	    	pavg_samples_update_phy=&avg_samples_update_phy[GPS_D];
			pdetilt_correction_phy=	&detilt_correction_phy[GPS_D];    	
			pdetilt_rate_phy=	&detilt_rate_phy[GPS_D];    	
         ptest_offsetA_base= &test_offsetA_base[GPS_D];   	
	    	
		break;

    	case SE_A:
	    	fifo_phy=&SYNCE_A_Fifo_Local;
	    	gps_perf=&se_pa;
	    	floor_thres_phy=min_oper_thres_phy[SE_A];
	    	low_warning_input_phy=&low_warning_phy[SE_A];
	    	high_warning_input_phy=&high_warning_phy[SE_A];
	    	pturbo_cluster_phy= &turbo_cluster_phy[SE_A]; 
	    	pavg_samples_update_phy=&avg_samples_update_phy[SE_A];
			pdetilt_correction_phy=	&detilt_correction_phy[SE_A];    	
			pdetilt_rate_phy=	&detilt_rate_phy[SE_A]; 
         ptest_offsetA_base= &test_offsetA_base[SE_A];   	

 		break;
    	case SE_B:
	    	fifo_phy=&SYNCE_B_Fifo_Local;
	    	gps_perf=&se_pb;
	    	floor_thres_phy=min_oper_thres_phy[SE_B];
	    	low_warning_input_phy=&low_warning_phy[SE_B];
	    	high_warning_input_phy=&high_warning_phy[SE_B];
	    	pturbo_cluster_phy= &turbo_cluster_phy[SE_B]; 
	    	pavg_samples_update_phy=&avg_samples_update_phy[SE_B];
			pdetilt_correction_phy=	&detilt_correction_phy[SE_B];    	
			pdetilt_rate_phy=	&detilt_rate_phy[SE_B];  
         ptest_offsetA_base= &test_offsetA_base[SE_B];   	
		break;
    	case RED:
	    	fifo_phy=&RED_Fifo_Local;
	    	gps_perf=&red_pa;
	    	floor_thres_phy=min_oper_thres_phy[RED];
	    	low_warning_input_phy=&low_warning_phy[RED];
	    	high_warning_input_phy=&high_warning_phy[RED];
	    	pturbo_cluster_phy= &turbo_cluster_phy[RED]; 
	    	pavg_samples_update_phy=&avg_samples_update_phy[RED];
			pdetilt_correction_phy=	&detilt_correction_phy[RED];    	
			pdetilt_rate_phy=	&detilt_rate_phy[RED];  
         ptest_offsetA_base= &test_offsetA_base[RED];   	
		break;
		default:
		break;
		}
//      if(floor_thres_phy < 500) floor_thres_phy=500; //GPZ Jan 2012 TEST TEST TEST
		delta_offset = (double)(fifo_phy->offset[fifo_phy->fifo_in_index] - fifo_phy->offset[fifo_phy->fifo_out_index]);
      
      if(Is_GPS_OK(RM_GPS_1 + chan_indx_phy))
      {
   		if(detilt_blank[chan_indx_phy] == 0)
   		{
               if(RFE_PHY.Nmax > 100)  
               {
                  if(delta_offset >100) delta_offset = 100;
                  else if(delta_offset < -100) delta_offset= -100;
               }
   #if 1 //KEN THIS IS A GUESS AT THE FEB 7 FIX 
               else  
               {
                  if(delta_offset >5000) delta_offset = 0;
                  else if(delta_offset < -5000) delta_offset= 0;
               }
   #endif   
   				*pdetilt_rate_phy +=  0.05*((double)(delta_offset));
   		}
   		else
   		{
   				detilt_blank[chan_indx_phy]--;	

   		}
#if 1
      }
      else
      {
         
         detilt_blank[chan_indx_phy] = 16;
         *pdetilt_rate_phy = 0;
         *pdetilt_correction_phy = 0;
//         if(chan_indx_phy == 2)
//         {
//            debug_printf(GEORGE_PTP_PRT, "SE_A detilt restart:\n"); 
//         }

      }
#endif
//      if(chan_indx_phy == 2)
//     {
//      debug_printf(GEORGE_PTP_PRT, "SE_A detilt: %d blank: %ld rate: %ld OK: %d acc: %ld\n", 
// (int)chan_indx_phy, (long int)detilt_blank[chan_indx_phy], (long int)*pdetilt_rate_phy, (int)Is_GPS_OK(RM_GPS_1 + chan_indx_phy),(long int)*pdetilt_correction_phy);
//      }
		if(((rms.freq_source[0] >= RM_GPS_1) && (rms.freq_source[0]-2)== chan_indx_phy) )//active freq source case
      {
         //restart PHY RFE if large tilt exists
         if((RFE_PHY.Nmax == 10) && ((*pdetilt_rate_phy > 80.0) ||(*pdetilt_rate_phy < -80.0)))
         {
        	   debug_printf(GEORGE_PTP_PRT,"Large tilt RFE_PHY Restart:\n"); 
            init_rfe_phy();
   	      start_rfe_phy(holdover_f - (*pdetilt_rate_phy)); //renormalize RFE data to 16x speed
		      fdrift_f=fdrift_r=fdrift=fdrift_warped=fdrift_smooth=pll_int_r=pll_int_f= fdrift_raw=holdover_r=holdover_f - (*pdetilt_rate_phy);
            (*pdetilt_rate_phy)=0.0;

         }
#if 0
         debug_printf(GEORGE_PTP_PRT, "IPD_PHY detilt rate: chan: %d %ld, %ld, %ld, in %d, out %d, off_in %ld, off_out %ld\n",
         (int)(chan_indx_phy),
         (long int)(*pdetilt_rate_phy),
         (long int) (delta_offset),
         (long int)(detilt_blank[chan_indx_phy]),
         (int)(fifo_phy->fifo_in_index),
         (int)(fifo_phy->fifo_out_index),
         (long int)(fifo_phy->offset[fifo_phy->fifo_in_index]),
         (long int)(fifo_phy->offset[fifo_phy->fifo_out_index]) );
#endif

      }
			*pdetilt_correction_phy -= *pdetilt_rate_phy;
//     		*pdetilt_correction_phy = 0.0; //TEST TEST TEST  Force Offset Leave in For Now

		// Special Case minimum floor threshold while in turbo mode
		if(rfe_turbo_flag) 
		{
			if(floor_thres_phy<150) floor_thres_phy=150; //was 100 increase 150
		}
		
		// Common Stat Accumulation Logic for all Phases
		pcount_phy=0;
	 	// GPZ DEC 2010 add flow normalization to support 0.5, 1,2, 4, 8 ... rate
	 	packet_count_phy=((FIFO_BUF_SIZE + fifo_phy->fifo_in_index-fifo_phy->fifo_out_index)%FIFO_BUF_SIZE);
//	 	if(packet_count_phy>150) packet_count_phy=150;

//	 	*pavg_samples_update_phy = (int)(0.9*(double)(*pavg_samples_update_phy) + 0.1 * (double)(packet_count_phy));
//		debug_printf(GEORGE_PTP_PRT, "IPD_PHY phy avg_samples_update: %ld\n",(long)(*avg_samples_update_phy));
	 	
//	 	if((*pavg_samples_update_phy) < 56) //Adjust out pointer 
//	 	{
// 			if((*pavg_samples_update_phy) > 4)
// 			{
// 				fifo_phy->fifo_out_index = (fifo_phy->fifo_out_index + FIFO_BUF_SIZE - (PHY_WINDOW- (*pavg_samples_update_phy)) )%FIFO_BUF_SIZE;
//				debug_printf(GEORGE_PTP_PRT, "IPD phy new out indx: %d\n",fifo_phy->fifo_out_index);
// 			}
// 			else
//	 		{
//		fifo_phy->fifo_out_index = (fifo_phy->fifo_out_index + FIFO_BUF_SIZE - PHY_WINDOW )%FIFO_BUF_SIZE;
//				debug_printf(GEORGE_PTP_PRT, "IPD phy new out indx: %d\n",fifo_phy->fifo_out_index);
//			}
		
// 		}
 		while(fifo_phy->fifo_out_index != fifo_phy->fifo_in_index)
		{
//      debug_printf(GEORGE_PTP_PRT, "IPD phy out indx: %d in index: %d \n",(int)fifo_phy->fifo_out_index,(int)fifo_phy->fifo_in_index);

			if(SENSE==0)
			{
				test_offsetA_phy= fifo_phy->offset[fifo_phy->fifo_out_index];
			}
			else
			{
				test_offsetA_phy= -fifo_phy->offset[fifo_phy->fifo_out_index];
			}

         if(chan_indx_phy== SE_A)
         {
            if( (Is_GPS_OK(RM_SE_1)) == 0 )
            {
                *ptest_offsetA_base = test_offsetA_phy;
//      			debug_printf(GEORGE_PTP_PRT, "IPD_PHY Jam SE_A Base\n");

            }
         } 
         else if(chan_indx_phy== SE_B)
         { 
            if( (Is_GPS_OK(RM_SE_2)) == 0 )
            {
                *ptest_offsetA_base = test_offsetA_phy;
            }
         }
         else
         {
           *ptest_offsetA_base = 0;
//     			debug_printf(GEORGE_PTP_PRT, "Test Offset A : %ld, chan: %ld count: %ld\n", (long int)test_offsetA_phy, (long int)chan_indx_phy, pcount_phy );

         }    
         test_offsetA_phy= test_offsetA_phy - *ptest_offsetA_base;     

#ifdef MODIFY_BY_JIWANG
		/* get current phase offset */
		if(FLL_Elapse_Sec_Count > 600){
			SC_PhaseOffset(&cur_phase_offset,e_PARAM_GET);

			if(abs(test_offsetA_phy + *pdetilt_correction_phy - cur_phase_offset) > 1000){
				clr_phy_flag_cnt[chan_indx_phy] = 60;
				set_phy_flag_cnt[chan_indx_phy] ++;
				printf("set_phy_flag_cnt[%d] = %d\n", chan_indx_phy, set_phy_flag_cnt[chan_indx_phy]);
			}
			else{
				set_phy_flag_cnt[chan_indx_phy] = 0;
				if(set_update_phy_flag[chan_indx_phy])	
					clr_phy_flag_cnt[chan_indx_phy] --;
			}

			if(set_phy_flag_cnt[chan_indx_phy] > 2){
				set_update_phy_flag[chan_indx_phy] = 1;	
				IPD_PHY.phase_floor_avg[chan_indx_phy]= test_offsetA_phy + *pdetilt_correction_phy;
			}
			else if(clr_phy_flag_cnt[chan_indx_phy] == 0)
				set_update_phy_flag[chan_indx_phy] = 0;

			printf("============>>cur_phase_offset = %lld, test_offsetA_phy = %d, set_phy_flag_cnt[%d] = %d, clr_phy_flag_cnt[%d] = %d, set_update_phy_flag[%d]= %hhu\n",
				cur_phase_offset, test_offsetA_phy, chan_indx_phy, set_phy_flag_cnt[chan_indx_phy], chan_indx_phy, clr_phy_flag_cnt[chan_indx_phy], chan_indx_phy, set_update_phy_flag[chan_indx_phy]);	
}
#endif  

//			test_offsetA_phy += chan_offset_phy; //TEST TEST TEST

			for(i=0;i<8;i++) //update all phase stats
			{
				switch(i)
				{
    				case 0:
    					pipc_phy = &(IPD_PHY.PA[chan_indx_phy]);
    				break;
    				case 1:
	  					pipc_phy = &(IPD_PHY.PB[chan_indx_phy]);
    				break;
    				case 2:
	  					pipc_phy = &(IPD_PHY.PC[chan_indx_phy]);
     				break;
    				case 3:
	  					pipc_phy = &(IPD_PHY.PD[chan_indx_phy]);
     				break;
    				case 4:
    					pipc_phy = &(IPD_PHY.PE[chan_indx_phy]);
    				break;
    				case 5:
	  					pipc_phy = &(IPD_PHY.PF[chan_indx_phy]);
    				break;
    				case 6:
	  					pipc_phy = &(IPD_PHY.PG[chan_indx_phy]);
     				break;
    				case 7:
	  					pipc_phy = &(IPD_PHY.PH[chan_indx_phy]);
     				break;
     				
    				default:
    				break;
				}
				if((test_offsetA_phy< pipc_phy->IPC_PHY.floor) || (test_offsetA_phy> (pipc_phy->IPC_PHY.floor+2*floor_thres_phy) )  )
				{
					pipc_phy->IPC_PHY.floor = test_offsetA_phy;
				}
				else if(test_offsetA_phy> pipc_phy->IPC_PHY.ceiling)
				{
					pipc_phy->IPC_PHY.ceiling = test_offsetA_phy;
				}
				test_offsetB_phy = test_offsetA_phy - IPD_PHY.working_phase_floor[chan_indx_phy];
				if(chan_indx_phy == GPS_D){
					printf("====>>%s GPS_D: test_offsetA_phy %d, test_offsetB_phy %d, floor_thres_phy %d #%d floor_cluster_count %d\n", __FUNCTION__, test_offsetA_phy, test_offsetB_phy, floor_thres_phy, i, pipc_phy->IPC_PHY.floor_cluster_count);
				}
				if((test_offsetB_phy)< floor_thres_phy)	
				{
    			 	 pipc_phy->IPC_PHY.floor_cluster_sum+= test_offsetA_phy;
    		 		 pipc_phy->IPC_PHY.floor_cluster_count++;
    		 		 rfe_turbo_phase_acc_phy[chan_indx_phy]+=test_offsetA_phy;
    		 		 pcount_turbo_phy[chan_indx_phy]++;
 		 		}
//				if(((test_offsetB_phy)> ceiling_thres_phy) &&  ((test_offsetB_phy)< (ceiling_thres_phy+F_OFW_SIZE)))	
// 				{
//    			 	 pipc_phy->IPC_PHY.ofw_cluster_sum+= test_offsetB_phy;
//    		 		 pipc_phy->IPC_PHY.ofw_cluster_count++;
// 				}
 		 		
 		 		
 			} // end for loop to update all phase stats		 
#if 1
            test_offsetA_phy_dlag[chan_indx_phy]= test_offsetA_phy_lag[chan_indx_phy];
            test_offsetA_phy_lag[chan_indx_phy]= test_offsetA_phy_cur[chan_indx_phy];
            test_offsetA_phy_cur[chan_indx_phy]=test_offsetA_phy;
            dtemp= (double)test_offsetA_phy_cur[chan_indx_phy] - 2.0*(double)test_offsetA_phy_lag[chan_indx_phy] +(double)test_offsetA_phy_dlag[chan_indx_phy];
            if((skip_rfe_phy==0)&&(dtemp > 3000.0 || dtemp < -3000.0))
            {
         		debug_printf(GEORGE_PTP_PRT, "IPD pop: chan: %ld  %ld %ld %ld\n",(long int)chan_indx_phy,test_offsetA_phy_cur[chan_indx_phy],test_offsetA_phy_lag[chan_indx_phy],test_offsetA_phy_dlag[chan_indx_phy]);
				if(chan_indx_phy == GPS_A)
					printf("%s GPS_A: test_offsetA_phy_lag %ld phy_dlag %ld\n", __FUNCTION__, test_offsetA_phy_lag[chan_indx_phy],test_offsetA_phy_dlag[chan_indx_phy]);

               phy_pop[chan_indx_phy]=1;
            }
#endif
	
 		 	fifo_phy->fifo_out_index=(fifo_phy->fifo_out_index+1)%FIFO_BUF_SIZE;
 		 	pcount_phy++;
 		} 
 		// Update turbo RFE phase
 		if(pcount_turbo_phy[chan_indx_phy]>16)  //was 256
 		{
#if (SENSE==0)
 				rfe_turbo_phase_phy[chan_indx_phy]= (double)(-rfe_turbo_phase_acc_phy[chan_indx_phy])/(double)(pcount_turbo_phy[chan_indx_phy]); 
				rfe_turbo_phase_phy[chan_indx_phy]-= detilt_correction_phy[chan_indx_phy]; //added correction for tilt compensation
#else
				rfe_turbo_phase_phy[chan_indx_phy]= (double)(rfe_turbo_phase_acc_phy[chan_indx_phy])/(double)(pcount_turbo_phy[chan_indx_phy]); 
				rfe_turbo_phase_phy[chan_indx_phy]+= detilt_correction_phy[chan_indx_phy]; //added correction for tilt compensation
#endif 
			pcount_turbo_phy[chan_indx_phy]=0;	
		 	rfe_turbo_phase_acc_phy[chan_indx_phy]=0;  
 		}
// 		else
// 		{
// 			rfe_turbo_phase_phy[chan_indx_phy]=(double)MAXMIN;
// 		}
//		debug_printf(GEORGE_PTP_PRT, "IPD pcount: %ld\n",pcount);

		// State Machine for Updating each phase //
//		debug_printf(GEORGE_PTP_PRT, "IPD_PHY Complete testing at index: %ld\n", IPD_PHY.phase_index);

		if(IPD_PHY.phase_index==7) //complete phase A
		{
			pipc2_phy = &(IPD_PHY.PA[chan_indx_phy]);
			complete_phase_flag_phy[chan_indx_phy]=1;
		}
		else if(IPD_PHY.phase_index==15) //complete phase B
		{
		
			pipc2_phy = &(IPD_PHY.PB[chan_indx_phy]);
			complete_phase_flag_phy[chan_indx_phy]=2;
		}
		else if(IPD_PHY.phase_index==23) //complete phase C
		{
			pipc2_phy = &(IPD_PHY.PC[chan_indx_phy]);
			complete_phase_flag_phy[chan_indx_phy]=3;
		}
		else if(IPD_PHY.phase_index==31) //complete phase D
		{
			pipc2_phy = &(IPD_PHY.PD[chan_indx_phy]);
			complete_phase_flag_phy[chan_indx_phy]=4;
		}
     		
		else if(IPD_PHY.phase_index==39) //complete phase E
		{
			pipc2_phy = &(IPD_PHY.PE[chan_indx_phy]);
			complete_phase_flag_phy[chan_indx_phy]=5;
		}
     		
		else if(IPD_PHY.phase_index==47) //complete phase F
		{
			pipc2_phy = &(IPD_PHY.PF[chan_indx_phy]);
			complete_phase_flag_phy[chan_indx_phy]=6;
		}

		else if(IPD_PHY.phase_index==55) //complete phase G
		{
			pipc2_phy = &(IPD_PHY.PG[chan_indx_phy]);
			complete_phase_flag_phy[chan_indx_phy]=7;
		}
     		
		else if(IPD_PHY.phase_index==63) //complete phase H
		{
			pipc2_phy = &(IPD_PHY.PH[chan_indx_phy]);
			complete_phase_flag_phy[chan_indx_phy]=8;
		}

		if(chan_indx_phy == GPS_A)
			printf("%s GPS_A: complete_phase_flag_phy %d, pturbo_cluster_phy %d start_in_progress %d mode_jitter %d low_warning_input %d\n", __FUNCTION__, complete_phase_flag_phy[chan_indx_phy], *pturbo_cluster_phy, start_in_progress, mode_jitter, *low_warning_input_phy);
		if(complete_phase_flag_phy[chan_indx_phy])
		{
//			debug_printf(GEORGE_PTP_PRT, "IPD_PHY phase complete for channel: %d\n",chan_indx_phy);
	    	if((*pturbo_cluster_phy)>0) (*pturbo_cluster_phy)=(*pturbo_cluster_phy)-1;
			// add low density warning state NOV 20 2010
      		if((pipc2_phy->IPC_PHY.floor_cluster_count<13) //GPZ NOV 20 Adjusted to capture suspect low density events
      		    && (!start_in_progress)&&(mode_jitter==0)&&(*pturbo_cluster_phy==0) ) //GPZ NOV 2010 NEW low min density condition
      		{
      			*low_warning_input_phy= (*low_warning_input_phy) +1;
      			if((*low_warning_input_phy)>3)
      			{
			     	(*pturbo_cluster_phy)=64;
      			}
				debug_printf(GEORGE_PTP_PRT, "IPD_PHY warning:low min density:chan %d, count %ld, wfloor %ld, warn_cnt %d\n",
				chan_indx_phy,
				pipc2_phy->IPC_PHY.floor_cluster_count,
				pipc2_phy->working_phase_floor,
				*low_warning_input_phy
				 );
      		}  
      		
 			// update floor stats
      		if((pipc2_phy->IPC_PHY.floor_cluster_count>2045)&& (floor_thres_phy>1000) //GPZ NOV 20 Adjusted to capture all high density events
      		    && (!start_in_progress)&&(mode_jitter==0) &&(*pturbo_cluster_phy==0)) //GPZ NOV 2010 NEW high min density condition
      		{
		 		(*pturbo_cluster_phy)=64; //always use turbo mode	
      			*high_warning_input_phy= (*high_warning_input_phy) +1;
				if(floor_thres_phy>4500)   
				{
// 	     			pipc2_phy->phase_floor_avg=MAXMIN; GPZ JAN 2011 new strategy freeze floor during exception period
				}
				debug_printf(GEORGE_PTP_PRT, "IPD_PHY exception:high min density:chan %d, count %ld, wfloor %ld, floor %ld\n",
				chan_indx_phy,
				pipc2_phy->IPC_PHY.floor_cluster_count,
				pipc2_phy->working_phase_floor,
				pipc2_phy->IPC_PHY.floor
				 );
      			
    		}
	   		else if(pipc2_phy->IPC_PHY.floor_cluster_count>density_thres_phy)
    		{
 				pipc2_phy->phase_floor_avg= pipc2_phy->IPC_PHY.floor_cluster_sum/pipc2_phy->IPC_PHY.floor_cluster_count;
 				pipc2_phy->phase_floor_avg += *pdetilt_correction_phy; //GPZ JAN 2011 re-tilt after processing window	
				debug_printf(GEORGE_PTP_PRT, "IPD_PHY stats: %ld, indx: %d, chan: %d, rfe_turbo: %ld, floor avg: %ld, cnt:%ld, wfloor:%ld, floor :%ld, ctp:%d, thres:%ld, tilt:%ld, base %ld\n",
					FLL_Elapse_Sec_Count,
					(long int)complete_phase_flag_phy[chan_indx_phy],
					chan_indx_phy,
					(long int)rfe_turbo_phase_phy[chan_indx_phy],
					(long int)pipc2_phy->phase_floor_avg,
					(long int)pipc2_phy->IPC_PHY.floor_cluster_count,
					(long int)pipc2_phy->working_phase_floor,				
					(long int)pipc2_phy->IPC_PHY.floor,
					ctp_phy,
					(long int)floor_thres_phy,
					(long int)(*pdetilt_correction_phy),
               (long int)(*ptest_offsetA_base)
					);
      		}
      		else if(!start_in_progress&&(!RFE_PHY.first))
      		{
 //     		pipc2_phy->phase_floor_avg=MAXMIN; GPZ JAN 2011 new strategy freeze floor during exception period
				debug_printf(GEORGE_PTP_PRT, "IPD PHY exception:low min density:chan %d, count %ld, wfloor %ld, floor %ld\n",
				chan_indx_phy,
				pipc2_phy->IPC_PHY.floor_cluster_count,
				pipc2_phy->working_phase_floor,
				pipc2_phy->IPC_PHY.floor
				 );
      			
    		}
     		else
     		{
     			pipc2_phy->phase_floor_avg=pipc2_phy->IPC_PHY.floor;
  				debug_printf(GEORGE_PTP_PRT, "IPD PHY  No floor:chan %d, count %ld, wfloor %ld, floor %ld\n",
				chan_indx_phy,
				pipc2_phy->IPC_PHY.floor_cluster_count,
				pipc2_phy->working_phase_floor,
				pipc2_phy->IPC_PHY.floor
				 );
     			
     		}
#if 0
         // KEN OCT 18 2011 here is the main fix notice the commented out if statement and we
         // will invalidate 
     		// update GPS TDEV noise metrics start index invalidated if active source is not okay
   		if( (rms.freq_source[0]-2)== chan_indx_phy) //active freq source case
	   	{
            if( (Is_GPS_OK(chan_indx_phy+2)) != 0 )  //active source is not okay
            {
               low_warning_phy[GPS_A]++;
               low_warning_phy[GPS_B]++;
               low_warning_phy[SE_A]++;
               low_warning_phy[SE_B]++;
               skip_rfe_phy=32; //was 32
            }
         }
#endif

//			if((IPD_PHY.phase_index == 7)||(IPD_PHY.phase_index == 23)||(IPD_PHY.phase_index == 39)||(IPD_PHY.phase_index == 55) );

//			if((IPD_PHY.phase_index == 3 )||(IPD_PHY.phase_index ==  7)||(IPD_PHY.phase_index == 11)||(IPD_PHY.phase_index == 15) 
//          ||(IPD_PHY.phase_index == 19)||(IPD_PHY.phase_index == 23)||(IPD_PHY.phase_index == 27)||(IPD_PHY.phase_index == 31) 
//          ||(IPD_PHY.phase_index == 35)||(IPD_PHY.phase_index == 39)||(IPD_PHY.phase_index == 43)||(IPD_PHY.phase_index == 47)
//          ||(IPD_PHY.phase_index == 51)||(IPD_PHY.phase_index == 55)||(IPD_PHY.phase_index == 59)||(IPD_PHY.phase_index == 63) 
//         ) // decimate every 32 seconds
         if(1)
//         if((IPD_PHY.phase_index & 0x1)==0)
			{
			if(chan_indx_phy == GPS_A){
				printf("%s GPS_A: start_tdev_flag %d transient_bucket %d\n", __FUNCTION__,gps_perf->start_tdev_flag,
									gps_perf->transient_bucket); 
			}
 	     		if(gps_perf->start_tdev_flag < 32) 
            {
               gps_perf->start_tdev_flag ++;
               gps_perf->transient_bucket=1000;
            }
    	 		gps_perf->dlag_phase = gps_perf->lag_phase;
     			gps_perf->lag_phase = gps_perf->cur_phase;
     			gps_perf->cur_phase = pipc2_phy->phase_floor_avg;
      		if(gps_perf->start_tdev_flag > 24) //was 6
    	  		{
		   	  	gps_perf->TDEV_sample = (double)(gps_perf->cur_phase - 2*gps_perf->lag_phase + gps_perf->dlag_phase);
    		 		if(gps_perf->TDEV_sample < 0.0) gps_perf->TDEV_sample = -1.0 *gps_perf->TDEV_sample;
     				if(gps_perf->TDEV_sample > THOUSANDF ) gps_perf->TDEV_sample = THOUSANDF;
	     			gps_perf->TVAR_smooth =  (PER_90 * gps_perf->TVAR_smooth) + (PER_10 *gps_perf->TDEV_sample*gps_perf->TDEV_sample/6.0);

					if(chan_indx_phy == GPS_A){
						printf("%s GPS_A: cur %ld, lag %ld, dlag %ld, tdev_sample %le, tvar %le\n", __FUNCTION__,
							gps_perf->cur_phase, gps_perf->lag_phase, gps_perf->dlag_phase, gps_perf->TDEV_sample,
							gps_perf->TVAR_smooth);
					}
#if 0
	     			if(gps_perf->TDEV_sample > 100.0) // large transient event in input was 500, 50
#endif
					if(gps_perf->TDEV_sample > 800.0)
		     		{
		     			if(gps_perf->transient_bucket >100)	gps_perf->transient_bucket = 100;	
	   				debug_printf(GEORGE_PTP_PRT, "IPD_PHY LARGE: cur: %ld, lag: %ld,dlag: %ld, tdev_samp: %le, tvar %le, start: %ld, bucket: %ld, p_indx: %ld \n",
	   				gps_perf->cur_phase,
	   				gps_perf->lag_phase,
	   				gps_perf->dlag_phase,
	   				gps_perf->TDEV_sample,
	   				gps_perf->TVAR_smooth,
	   				(long int)gps_perf->start_tdev_flag,
	   				(long int)gps_perf->transient_bucket,
	   				(long int)IPD_PHY.phase_index					
	   				);
               }
#if 0
	     			else if(gps_perf->TDEV_sample > 44.0) // excess noise condition was 200, 22
#endif
					else if(gps_perf->TDEV_sample > 352.0)
		     		{
		     			if(gps_perf->transient_bucket >500) gps_perf->transient_bucket = gps_perf->transient_bucket - 500;	
	   				debug_printf(GEORGE_PTP_PRT, "IPD_PHY EXCESS: cur: %ld, lag: %ld,dlag: %ld, tdev_samp: %le, tvar %le, start: %ld, bucket: %ld, p_indx: %ld \n",
	   				gps_perf->cur_phase,
	   				gps_perf->lag_phase,
	   				gps_perf->dlag_phase,
	   				gps_perf->TDEV_sample,
	   				gps_perf->TVAR_smooth,
	   				(long int)gps_perf->start_tdev_flag,
	   				(long int)gps_perf->transient_bucket,
	   				(long int)IPD_PHY.phase_index					
	   				);
	     			}
               else
               {
                 	if(gps_perf->transient_bucket < 1000) gps_perf->transient_bucket += 100;
               }
	     			if(gps_perf->transient_bucket < 600) // activate transient event
	     			{
/* only do this step if this is the active channel */
                  debug_printf(GEORGE_PTP_PRT, "IPD_PHY activate transient: chan_indx_phy: %d %d\n", (int)chan_indx_phy, (int)rms.freq_source[0]);
//                  if(chan_indx_phy == (rms.freq_source[0] - 2)) TEST TEST TEST
                  {            
 //  	     				*low_warning_input_phy= (*low_warning_input_phy) +1;
                     //For current release RFE use a ganged common input so we must also gang the warning indication
//                     low_warning_phy[GPS_A]++;
//                     low_warning_phy[GPS_B]++;
//                     low_warning_phy[SE_A]++;
//                     low_warning_phy[SE_B]++;
                     low_warning_phy[chan_indx_phy]++; //GPZ DEC 2011 new unganged mode
//               		skip_rfe_phy=PHY_SKIP; //GPZ Feb 2012 Now that channels are unganged can't skip PHY
                  }	     			
               }
	     			
					debug_printf(GEORGE_PTP_PRT, "IPD_PHY TDEV PERF: cur: %ld, lag: %ld,dlag: %ld, tdev_samp: %le, tvar %le, start: %ld, bucket: %ld, p_indx: %ld \n",
					gps_perf->cur_phase,
					gps_perf->lag_phase,
					gps_perf->dlag_phase,
					gps_perf->TDEV_sample,
					gps_perf->TVAR_smooth,
					(long int)gps_perf->start_tdev_flag,
					(long int)gps_perf->transient_bucket,
					(long int)IPD_PHY.phase_index					
					);
      		}
			}
			// update offset window stats
			
			// update new floor baseline for phase 
			pipc2_phy->working_phase_floor= pipc2_phy->IPC_PHY.floor;
			// assign current output data
			IPD_PHY.working_phase_floor[chan_indx_phy]=	pipc2_phy->working_phase_floor;
#ifdef MODIFY_BY_JIWANG
			if(set_update_phy_flag[chan_indx_phy])
				IPD_PHY.phase_floor_avg[chan_indx_phy]= test_offsetA_phy + *pdetilt_correction_phy;
			else
				IPD_PHY.phase_floor_avg[chan_indx_phy]=	pipc2_phy->phase_floor_avg;	
#else
			IPD_PHY.phase_floor_avg[chan_indx_phy]=	pipc2_phy->phase_floor_avg;
#endif
			IPD_PHY.ofw_cluster_count[chan_indx_phy]=pipc2_phy->IPC_PHY.ofw_cluster_count;
			IPD_PHY.floor_cluster_count[chan_indx_phy]=pipc2_phy->IPC_PHY.floor_cluster_count;
			// re-initialize working phase variables
			pipc2_phy->IPC_PHY.floor=   MAXMIN;
			pipc2_phy->IPC_PHY.ceiling=-MAXMIN;
			pipc2_phy->IPC_PHY.floor_cluster_count=0;
			pipc2_phy->IPC_PHY.floor_cluster_sum=0L;	
			pipc2_phy->IPC_PHY.ofw_cluster_count=0;
			pipc2_phy->IPC_PHY.ofw_cluster_sum=0L;
		
			complete_phase_flag_phy[chan_indx_phy]=0;
		}
		
		// End Phase State Machine
       // KEN OCT 18 2011 here is the main fix notice the commented out if statement and we
       // will invalidate 
 		// update GPS TDEV noise metrics start index invalidated if active source is not okay
          if( !Is_GPS_OK(chan_indx_phy+2) || (phy_pop[chan_indx_phy]==1)) //active source is not okay
          {
//              debug_printf(GEORGE_PTP_PRT, "IPD_PHY pop transient: chan_indx_phy: %d  cnt:%ld\n", (int)chan_indx_phy,(long int)low_warning_phy[chan_indx_phy]);
              low_warning_phy[chan_indx_phy]++;
              phy_pop[chan_indx_phy]=0;
          }

		if(chan_indx_phy == GPS_A)
			printf("%s GPS_A:  low_warning_input_phy %d\n", __FUNCTION__, *low_warning_input_phy);


#if 0 // Replace with above
 		if( (rms.freq_source[0]-2)== chan_indx_phy) //active freq source case
   	{
           if( !Is_GPS_OK(chan_indx_phy+2) || (phy_pop[chan_indx_phy]==1)) //active source is not okay
           {
              debug_printf(GEORGE_PTP_PRT, "IPD_PHY pop transient: chan_indx_phy: %d \n", (int)chan_indx_phy);
             low_warning_phy[GPS_A]++;
              low_warning_phy[GPS_B]++;
              low_warning_phy[SE_A]++;
              low_warning_phy[SE_B]++;
              skip_rfe_phy=PHY_SKIP; //TEST TEST TEST was 32
              phy_pop[chan_indx_phy]=0;
           }
       }	
#endif	
	} //end for loop for each input channel

	IPD_PHY.phase_index=(IPD_PHY.phase_index+1)%64; //advance through 64 second block
 	// assign system minimum data elements
	//	min_forward.sdrift_min_1024=IPD_PHY.phase_floor_avg[0]+(INT32)fbias; //GPZ Sept 2010 use floor average NOV 2010 add bias compensation here
	// IGNORE floor bias compensation for now
	avg_GPS_A=IPD_PHY.phase_floor_avg[0]; //GPZ Sept 2010 use floor average NOV 2010 add bias compensation here
	avg_GPS_B=IPD_PHY.phase_floor_avg[1]; //GPZ Sept 2010 use floor average NOV 2010 add bias compensation here
	avg_GPS_C=IPD_PHY.phase_floor_avg[2]; //GPZ Sept 2010 use floor average NOV 2010 add bias compensation here//jiwang
	avg_GPS_D=IPD_PHY.phase_floor_avg[3]; //GPZ Sept 2010 use floor average NOV 2010 add bias compensation here//jiwang
	avg_RED=IPD_PHY.phase_floor_avg[4]; //GPZ Sept 2010 use floor average NOV 2010 add bias compensation here
	if((IPD_PHY.phase_index & 0x3)==1) //every 4th cycle 
	{	
	// Common cluster test point logic
	ctp_phy=128; //baseline cluster width Jan 2011 increase to 1024
		for(chan_indx_phy=0;chan_indx_phy<5;chan_indx_phy++)//jiwang modify
		{
	
	
			// GPS_A floor cluster logic
			if( (IPD_PHY.floor_cluster_count[chan_indx_phy]) > ctp_phy) 
			{
				if(min_oper_thres_phy[chan_indx_phy] > (2.0 * cluster_inc_phy[chan_indx_phy]))
				{
					min_oper_thres_phy[chan_indx_phy] -=cluster_inc_phy[chan_indx_phy]; 
					cluster_inc_sense_phy[chan_indx_phy]=0;
				}
			}
			else
			{
				if(min_oper_thres_phy[chan_indx_phy]<cluster_width_max) //tight bound for TOD apps
				{
					{
    					min_oper_thres_phy[chan_indx_phy]+= cluster_inc_phy[chan_indx_phy]; //was 25
  						cluster_inc_sense_phy[chan_indx_phy]=1;
	    			}	
				}
			}
			// adapt cluster inc new April 2010
			if(cluster_inc_sense_phy[chan_indx_phy] ==cluster_inc_sense_prev_phy[chan_indx_phy])
			{
				cluster_inc_slew_count_phy[chan_indx_phy]++;
				if(cluster_inc_slew_count_phy[chan_indx_phy] > 100) // underslew condition was 188
				{
					cluster_inc_slew_count_phy[chan_indx_phy]=0;
					cluster_inc_count_phy[chan_indx_phy]=0;
					if(cluster_inc_phy[chan_indx_phy]< cluster_inc) cluster_inc_phy[chan_indx_phy]+=32;
					else cluster_inc_phy[chan_indx_phy]=cluster_inc;
				}
			}
			else
			{
				cluster_inc_count_phy[chan_indx_phy]++; 
				cluster_inc_slew_count_phy[chan_indx_phy]=0;
				if(cluster_inc_count_phy[chan_indx_phy]> 16) //was 32
				{
					cluster_inc_slew_count_phy[chan_indx_phy]=0;
					cluster_inc_count_phy[chan_indx_phy]=0;
					if(cluster_inc_phy[chan_indx_phy]>(2*CLUSTER_INC)) cluster_inc_phy[chan_indx_phy] =cluster_inc_phy[chan_indx_phy]/2;
					else if(cluster_inc_phy[chan_indx_phy]>50) cluster_inc_phy[chan_indx_phy] -=8; //increase minimum from 16
				}
  			}
	   		cluster_inc_sense_prev_phy[chan_indx_phy]=cluster_inc_sense_phy[chan_indx_phy];
			if(OCXO_Type() != e_TCXO) //use static cluster width if not mini
			{
	   			if(!start_in_progress)
   				{
   					if(turbo_cluster_phy[chan_indx_phy]>0)
   					{
	   					cluster_inc_phy[chan_indx_phy]=cluster_inc; //GPZ OCT 2010 use defined cluster inc
	   				}
   					else
   					{
	   					cluster_inc_phy[chan_indx_phy]=25; //GPZ NOV 2010 use steady state small adjustment
   					}
   				}
			}
		} //end for each channel loop
	} //end every 4th cycle   	
}




///////////////////////////////////////////////////////////////////////
// This function initializes new input physical signal processing
//typedef struct
// {
// 	int_32 floor_cluster_count; //count over phase interval of samples in floor cluster
// 	int_32 ofw_cluster_count;	//count over phase interval of samples in ceiling cluster
// 	
// 	long long floor_cluster_sum;
// 	long long ofw_cluster_sum;
// 	
// 	int_32 floor;  //minimum in one  interval use to replace working phase floor
// 	int_32 ceiling;//maximum in one  interval
// } 
//  INPROCESS_CHANNEL_STRUCT;
//  
//  typedef struct
//  {
//  	int_32 phase_floor_avg; // cluster phase floor average
//  	int_32 phase_ceiling_avg; //cluster phase ceiling average
//  	int_32 working_phase_floor; //baseline phase_floor 
//  	INPROCESS_CHANNEL_STRUCT IPC; 
//  }
//  INPROCESS_PHASE_STRUCT;
//  typedef struct
//  {
//  	int_16 phase_index; 
//  	INPROCESS_PHASE_STRUCT PA,PB,PC,PD;
//  }
//  INPUT_PROCESS_DATA;
//
///////////////////////////////////////////////////////////////////////
void init_ipp_phy(void)
{
	int i;	
	IPD_PHY.phase_index=0;
	for(i=0;i<IPP_CHAN_PHY;i++)
	{
		pcount_turbo_phy[i]=0;	

		IPD_PHY.PA[i].phase_floor_avg= 0;
		IPD_PHY.PA[i].phase_ceiling_avg=0;
		IPD_PHY.PA[i].working_phase_floor=MAXMIN;
		IPD_PHY.PA[i].IPC_PHY.floor=MAXMIN;
		IPD_PHY.PA[i].IPC_PHY.ceiling=-MAXMIN;
		IPD_PHY.PB[i].phase_floor_avg= 0;
		IPD_PHY.PB[i].phase_ceiling_avg=0;
		IPD_PHY.PB[i].working_phase_floor=MAXMIN;
		IPD_PHY.PB[i].IPC_PHY.floor=MAXMIN;
		IPD_PHY.PB[i].IPC_PHY.ceiling=-MAXMIN;
		IPD_PHY.PC[i].phase_floor_avg= 0;
		IPD_PHY.PC[i].phase_ceiling_avg=0;
		IPD_PHY.PC[i].working_phase_floor=MAXMIN;
		IPD_PHY.PC[i].IPC_PHY.floor=MAXMIN;
		IPD_PHY.PC[i].IPC_PHY.ceiling=-MAXMIN;
		IPD_PHY.PD[i].phase_floor_avg= 0;
		IPD_PHY.PD[i].phase_ceiling_avg=0;
		IPD_PHY.PD[i].working_phase_floor=MAXMIN;
		IPD_PHY.PD[i].IPC_PHY.floor=MAXMIN;
		IPD_PHY.PD[i].IPC_PHY.ceiling=-MAXMIN;


		IPD_PHY.PE[i].phase_floor_avg= 0;
		IPD_PHY.PE[i].phase_ceiling_avg=0;
		IPD_PHY.PE[i].working_phase_floor=MAXMIN;
		IPD_PHY.PE[i].IPC_PHY.floor=MAXMIN;
		IPD_PHY.PE[i].IPC_PHY.ceiling=-MAXMIN;
		IPD_PHY.PF[i].phase_floor_avg= 0;
		IPD_PHY.PF[i].phase_ceiling_avg=0;
		IPD_PHY.PF[i].working_phase_floor=MAXMIN;
		IPD_PHY.PF[i].IPC_PHY.floor=MAXMIN;
		IPD_PHY.PF[i].IPC_PHY.ceiling=-MAXMIN;
		IPD_PHY.PG[i].phase_floor_avg= 0;
		IPD_PHY.PG[i].phase_ceiling_avg=0;
		IPD_PHY.PG[i].working_phase_floor=MAXMIN;
		IPD_PHY.PG[i].IPC_PHY.floor=MAXMIN;
		IPD_PHY.PG[i].IPC_PHY.ceiling=-MAXMIN;
		IPD_PHY.PH[i].phase_floor_avg= 0;
		IPD_PHY.PH[i].phase_ceiling_avg=0;
		IPD_PHY.PH[i].working_phase_floor=MAXMIN;
		IPD_PHY.PH[i].IPC_PHY.floor=MAXMIN;
		IPD_PHY.PH[i].IPC_PHY.ceiling=-MAXMIN;
		
	}
}

#endif
// This function is used to create the Load Mode Event Aux Parameter
static INT32 CreateLoadParam(Load_State loadstate)
{
	INT32 aux = loadstate;

// add the PTP channel number to the upper 16 bits of aux
	aux |=  (ServoIndex2UserChan(RM_GM_1) << 16);
	
	return aux;
}
//***************************************************************************************
//			LEGACY CODE BELOW THIS LINE
//
//*****************************************************************************************
///////////////////////////////////////////////////////////////////////
// This function get a best estimate of true minimum offset
///////////////////////////////////////////////////////////////////////
