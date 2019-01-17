
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

FILE NAME    : clk_ana.c

AUTHOR       : George Zampetti

DESCRIPTION  : 


Revision control header:
$Id: CLK/clk_ana.c 1.67 2012/04/20 16:40:24PDT Kenneth Ho (kho) Exp  $

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
#include "sc_ptp_servo.h"
#include "datatypes.h"
#include "ptp.h"
#include "CLKflt.h"
#include "CLKpriv.h"
#include "clk_private.h"
#define sprintf diag_sprintf
//Use the private version, not the library one
#define sqrt(x)  sqrt_local(x)
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

double fpairs[8]; 
double wpairs[8];
#if(NO_PHY==0)
double fpairs_phy[128]; //was 512 reduced to 128 Dec 2011
double wpairs_phy[128];
#endif	
//static int short_skip_cnt_f =0; //GPZ NOV 2010 used to skip short RFE window during transients
//static int short_skip_cnt_r =0;



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
// This function update fll status now every 16 second background info
///////////////////////////////////////////////////////////////////////
void fll_status_1min(void)
{
//	float fscale,rscale;
	double dtemp;
//	UINT16 min_flow = get_rate(nvm.nv_master_sync_rate)/2;
//	UINT16 min_flow = 64/2;
	
	fllstatus.f_weight=xds.xfer_weight_f;  //Ken_Post_Merge
	fllstatus.r_weight=xds.xfer_weight_r;	//Ken_Post_Merge
	fllstatus.freq_cor =(xds.xfer_fdrift-xds.xfer_fshape-xds.xfer_tshape);
	fllstatus.time_cor  =(xds.xfer_fshape+xds.xfer_tshape); //
//	if(xds.xfer_pshape_smooth > 100000.0) 	fllstatus.residual_time_cor=50000.0;
//	else if (xds.xfer_pshape_smooth < -100000.0) 	fllstatus.residual_time_cor=-50000.0;
//	else
//	{
//		fllstatus.residual_time_cor=(xds.xfer_pshape_smooth);
//	}	
	fllstatus.residual_time_cor= residual_time_error;
;
	fllstatus.f_min_cluster_width= min_oper_thres_f;
	fllstatus.r_min_cluster_width=min_oper_thres_r;
	fllstatus.f_mode_width= ofw_start_f;
	fllstatus.r_mode_width= ofw_start_r;
	fllstatus.f_cw_avg=xds.xfer_scw_avg_f;
	fllstatus.f_cw_std= sqrt(xds.xfer_scw_e2_f - xds.xfer_scw_avg_f*xds.xfer_scw_avg_f);
	fllstatus.r_cw_avg=xds.xfer_scw_avg_r;
	fllstatus.r_cw_std= sqrt(xds.xfer_scw_e2_r - xds.xfer_scw_avg_r*xds.xfer_scw_avg_r);
		
	fllstatus.f_oper_mode_tdev= ZEROF; //TODO
	fllstatus.r_oper_mode_tdev= ZEROF; //TODO

	fllstatus.f_oper_mtdev= sqrt(xds.xfer_var_est_f);
	fllstatus.r_oper_mtdev= sqrt(xds.xfer_var_est_r);
	
//	if(fllstatus.f_cdensity>0.1)
//	{
//		fscale =(float) (sqrt(fllstatus.f_cdensity*1024.0));
//	}
//	if(fllstatus.f_cdensity>0.1)
//	{
//	
//		rscale =(float)( sqrt(fllstatus.r_cdensity*1024.0));
//	}
//	else
//	{
//		fscale=1.0;
//		rscale=1.0;	
//	}
#if 0 /* changed to 1 second minimun */
	fllstatus.rtd_us = Get_RTD_1sec_min();
	
#else
/* use fast RTD calculation if in warmup, unknown, or the flow is 
*  too low 
*/
	if((tstate==FLL_WARMUP) ||
	   (tstate== FLL_UNKNOWN))
// ||
//	   ((number_of_sync_RTD_filtered_1sec < min_flow/2) &&
//	    (number_of_sync_RTD_filtered_1sec < min_flow/2)))
	{	
		fllstatus.rtd_us = Get_RTD_1sec_min()/1000.0;
	}
	else if(round_trip_delay_alt>ZEROF)
	{
		fllstatus.rtd_us=round_trip_delay_alt/1000.0; // get current round trip delay	
	}
	else
	{
		fllstatus.rtd_us=Get_RTD_1sec_min()/1000.0;
	}	
#endif
	
	
//	if(enterprise_flag)
//	{
//		if(time_bias_mean>ZEROF)
//		{
//			fllstatus.fll_min_time_disp=( 2.0*sqrt(time_bias_var));	
//		}
//		else
//		{
//			fllstatus.fll_min_time_disp= (-time_bias_mean + 2.0*sqrt(time_bias_var));	
//		}
//	}
//	else
//	{
			fllstatus.fll_min_time_disp=ZEROF;
//	}	
	if(RFE.Nmax>0)
	{
		// note divide includes a sqrt 256 (16) factor of reduction from LSF as well as the weights
//		dtemp= (forward_weight*forward_stats.tdev_element[TDEV_CAT].var_est +reverse_weight*reverse_stats.tdev_element[TDEV_CAT].var_est)/sqrt(LSF.N);
		dtemp= (forward_weight*forward_stats.tdev_element[TDEV_CAT].var_est +reverse_weight*reverse_stats.tdev_element[TDEV_CAT].var_est);
		//note scale var by 4 hour window square 7200*7200 to bound temperature effect
		// 16e-22 is 4e-11 per deg C ^2 worse case tempco assumption
#ifdef TEMPERATURE_ON
		dtemp+= temp_stats.t_var_60min*16e-22*51840000.0*(double)(RFE.N)/64.0;
#endif
		if(dtemp<ZEROF) dtemp=ZEROF;
		if(dtemp> 100000000000.0) dtemp= 10000000000.0; //GPZ DEC 2009 extend range
		fllstatus.out_tdev_est=sqrt(dtemp);
		fllstatus.out_mdev_est=sqrt(3.0)*fllstatus.out_tdev_est/((double)(RFE.N)*60.0);
	}

#ifdef TEMPERATURE_ON
	if(Temperature_data_ready())
	{	
       fllstatus.t_max = temp_stats.tes[(temp_stats.t_indx+127)%128].t_max_hold;
       fllstatus.t_min = temp_stats.tes[(temp_stats.t_indx+127)%128].t_min_hold;
       fllstatus.t_avg = temp_stats.tes[(temp_stats.t_indx+127)%128].t_avg;
	   fllstatus.t_std_5min=  sqrt(temp_stats.t_var_5min);
	   fllstatus.t_std_60min= sqrt(temp_stats.t_var_60min);
	}
	else
	{
       fllstatus.t_max = ZEROF;
       fllstatus.t_min = ZEROF;
       fllstatus.t_avg = ZEROF;
	   fllstatus.t_std_5min= ZEROF;
	   fllstatus.t_std_60min=ZEROF;		
	}
#endif

	fllstatus.f_mafie=ztie_stats.mafie_f;
	fllstatus.r_mafie=ztie_stats.mafie_r;

}
///////////////////////////////////////////////////
// This function make a copy of current fll status info
/////////////////////////////////////////////////////
float Get_out_tdev_est(void)
{
	return fllstatus.out_tdev_est;
}

void get_fll_status(FLL_STATUS_STRUCT *fp)
{
//	volatile MCF5282_STRUCT_PTR mcf5282_ptr;
//	mcf5282_ptr = _PSP_GET_IPSBAR();

//	mcf5282_ptr->GPIO.PODR_CS  =  0x7F;
			
//	_int_disable();
   if(o_fll_init)
   {   
	   memcpy((char*)fp,(char*)&fllstatus,sizeof(fllstatus));
   }
   else
   {
/* fill with zeros if not initialized */
      memset((char*)fp,0,sizeof(fllstatus));
   }
//	_int_enable();
//	mcf5282_ptr->GPIO.PODR_CS  =  0xFF;
}

Fll_Clock_State get_fll_state(void)
{
	return fllstatus.cur_state;
}

Fll_Clock_State get_fll_prev_state(void)
{
	return fllstatus.prev_state;
}

UINT16 get_fll_led_state(void)
{
	return fllstatus.lock;
}


///////////////////////////////////////////////////////////////////////
// This function is called once per robust update period (8s to 2minute) to update robust frequency  estimates
// 
///////////////////////////////////////////////////////////////////////
#if SMARTCLOCK == 1
//static int  rfe_settle_count;
static INT32 last_good_turbo_phase[2];
#endif
void update_rfe(void)
{
//   UINT8 interrupt;
	INT16 i,j,jmax,print_index;
	INT16 Fix_Space,Fix_Space_Inc,Fix_Space_List[8];
	double Turbo_Scale;
	double dtemp,delta_phase;
	INT16 inner_range;
	INT16 inner_range_prev;
	INT16 Pair_Valid_Flag;
//	INT16 rfe_cindx; //cur_index
//	INT16 rfe_pindx; //prev_index
//	INT16 rfe_findx; //future_index
	INT16 rfe_start; //start pair index
	INT16 rfe_end; //end pair index
	INT32 ibias_f,ibias_r;
	INT16 total; //total pairs used in calculation
	INT32 scale1,scale2,scale3,scale4,scale5;
	double weight; // total weight of calculation
	double weight_for, weight_rev; //Ken_Post_Merge
	double bias_asymmetry_factor, instability_factor,comm_bias,diff_bias;
	double fest,delta_fest;
	double fest_for, fest_rev; //Ken_Post_Merge
	double fest_first,delta_first_thres;
	double weight_first,weight_first_thres;
	double alpha_local = 1.0;
   double slew_limit_local;
	INT16 total_first,total_first_thres;
	rfe_pindx= RFE.index;
	rfe_ppindx= (RFE.index+RFE.N - 1)%RFE.N;
	rfe_cindx= (RFE.index+1)%RFE.N;
	rfe_findx= (RFE.index+2)%RFE.N;
	RFE.index=rfe_cindx;
	if(rfe_turbo_flag==FALSE)
	{
		Turbo_Scale=1.0;
	}
	else if(rfe_turbo_flag==1)
	{
//		debug_printf(GEORGE_PTP_PRT,"set Turbo_Scale\n");

		if(OCXO_Type() == e_MINI_OCXO)
		{
			Turbo_Scale=8.0;
//			debug_printf(GEORGE_PTP_PRT,"set Turbo_Scale to MINI\n");

		}
		else if(OCXO_Type() == e_OCXO)
		{
			Turbo_Scale=8.0;
//			debug_printf(GEORGE_PTP_PRT,"set Turbo_Scale to OCXO\n");

		}
		else
		{
			Turbo_Scale=8.0;
//			debug_printf(GEORGE_PTP_PRT,"set Turbo_Scale to Rb\n");

		}
	}
	else 
	{
		Turbo_Scale=16.0;  //128 over 16
	}
	
	if(RFE.Nmax<RFE_WINDOW_LARGE)
	{
		RFE.Nmax++;
	}
	total=0;
	weight=ZEROF;
	fest=ZEROF;
	weight_for=ZEROF; 
	weight_rev=ZEROF; 
	fest_for  =ZEROF; 
	fest_rev  =ZEROF; 
	if((RFE.first)&&(RFE.index==(N_init_rfe-5))) // base line phase once after first cycle
	{
		RFE.first--; //allow RFE.first to count multiple loops if needed
#if 0
		delta_fest= (RFE.fest- RFE.fest_prev);
		if(delta_fest<0.0) delta_fest=-delta_fest;
		debug_printf(GEORGE_PTP_PRT,"rfe: Delta Fest Check: Delta:%le, cur:%le, prev:%le\n",delta_fest,RFE.fest,RFE.fest_prev);
		if(delta_fest<25.0 && (total_first>0)) //ensure loop is settled to within 25 ppb 
		{
//			clr_residual_phase(); //Ken_Post_Merge	
//			init_pshape();				
//			RFE.first--; //allow RFE.first to count multiple loops if needed
         RFE.first = 1;
		}
#endif	
	}
	else if((RFE.first)&&(RFE.index==(N_init_rfe-1))) // base line phase once after first cycle
	{
		RFE.fest_prev=RFE.fest;
	}
	if((OCXO_Type()==e_MINI_OCXO) && (mode_jitter==0))
	{
		Fix_Space=3; // start with 6 minute spacing
		Fix_Space_Inc=6; //was 3
		// was 5,6,6
		Fix_Space_List[1]=11;		
		Fix_Space_List[2]=11;		
		Fix_Space_List[3]=11;		
		jmax=4;
	}
	else if((mode_jitter==1)&&(min_oper_thres_f>50000)&&(min_oper_thres_r>50000))
	{
		Fix_Space=5;
		Fix_Space_Inc=5;
		Fix_Space_List[1]=5;		
		Fix_Space_List[2]=9;		
		Fix_Space_List[3]=15;		
		jmax=4;
	}
	else 
	{
		Fix_Space=3; // start with 6 minute spacing was (6)
		Fix_Space_Inc=3; //was 4 try 3 to create sync pattern was (3)
		Fix_Space_List[1]=11;		
		Fix_Space_List[2]=11;		
		Fix_Space_List[3]=11;		
		jmax=4;
	}
	
	debug_printf(GEORGE_PTP_PRT,"rfe: Fix_Space:%5d,jmax:%5d,first:%5d\n",Fix_Space,jmax,RFE.first);
	for(i=0;i<8;i++) //was 512
	{
		fpairs[i]=ZEROF;
		wpairs[i]=ZEROF;
	}
	if(RFE.first)
	{
		fest_first=0.0;
		weight_first=0.0;
		total_first=0;
	}
#if SMARTCLOCK == 1
   if(PTP_LOS ==0)
   {		
      last_good_turbo_phase[0]= xds.xfer_turbo_phase[0];
      last_good_turbo_phase[1]= xds.xfer_turbo_phase[1];
   }
#endif
	// update rfe open loop compensation for turbo case
	if(rfe_turbo_flag==1)
	{	
//		rfe_turbo_acc+= 32.0*fdrift_smooth;  //32s  4x is turbo update window
//		rfe_turbo_acc+= 32.0*holdover_f;  //32s  4x is turbo update window
//		rfe_turbo_acc+= 16.0*holdover_f;  //16s  8x is turbo update window

		if(OCXO_Type() == e_MINI_OCXO)
		{
			rfe_turbo_acc+= 16.0*holdover_f;
		}
		else if(OCXO_Type() == e_OCXO)
		{
			rfe_turbo_acc+= 16.0*holdover_f;
		}
		else
		{
			rfe_turbo_acc+= 16.0*holdover_f;
		}
	}
	else if	(rfe_turbo_flag==2)	
	{
//		rfe_turbo_acc+= 8.0*fdrift_smooth;  //8s  16x is turbo update window
		rfe_turbo_acc+= 8.0*holdover_f;  //8s  16x is turbo update window
	}
	// update current index information
	// forward stats
	if(rfe_turbo_flag)
	{
//		RFE.rfee_f[rfe_cindx].phase = (double)sdrift_min_forward+ rfe_turbo_acc;
		if(xds.xfer_turbo_phase[0]<(double)MAXMIN)
		{
#if SMARTCLOCK == 1
			RFE.rfee_f[rfe_cindx].phase = last_good_turbo_phase[0]+ rfe_turbo_acc;
#else
			RFE.rfee_f[rfe_cindx].phase = xds.xfer_turbo_phase[0]+ rfe_turbo_acc;
#endif
			RFE.rfee_f[rfe_cindx].tfs = 1;
		}
		else
		{
			RFE.rfee_f[rfe_cindx].phase=0.0;
			RFE.rfee_f[rfe_cindx].tfs = 0;
			Forward_Floor_Change_Flag=1;
		}
		debug_printf(GEORGE_PTP_PRT,"xfer turbo phase: Forward: %le, Reverse: %le\n",xds.xfer_turbo_phase[0],xds.xfer_turbo_phase[1]);
		// Process Delay line used to manage transients
		RFE_in_forward[4]=RFE_in_forward[3];
		RFE_in_forward[3]=RFE_in_forward[2];
		RFE_in_forward[2]=RFE_in_forward[1];
		RFE_in_forward[1]=RFE_in_forward[0];
		RFE_in_forward[0]=RFE.rfee_f[rfe_cindx].phase;
		delta_phase= RFE_in_forward[2] - 2.0*RFE_in_forward[1] + RFE_in_forward[0];	
//		delta_phase= RFE_in_forward[1] - RFE_in_forward[0];	
		if(delta_phase<0.0) delta_phase= -delta_phase;
		if(delta_phase>40000.0) //was 80000 reduce to lower using 2nd difference
		{
			if ((LoadState != LOAD_GENERIC_HIGH)&&(ptpTransport==e_PTP_MODE_ETHERNET)) //ethernet specific settings
//			if ((LoadState != LOAD_GENERIC_HIGH)&&((ptpTransport==e_PTP_MODE_ETHERNET)||(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS))&&(!RFE.first)) //ethernet specific settings
			{
				PTP_LOS=PTP_LOS_TIMEOUT;
				debug_printf(GEORGE_PTP_PRT,"RFE delta phase pop forward:%ld\n",(long int)(delta_phase) );
				debug_printf(GEORGE_PTP_PRT,"PTP_LOS delta pop forward: %d\n",(PTP_LOS));

			}
		}

	}
	else
	{
		RFE.rfee_f[rfe_cindx].phase =  xds.xfer_sacc_f;
	}	
	ibias_f=(long)(-1.0*fbias/2500.0); //was -4
	ibias_f= ibias_f*2500; 

//	ibias_f=(long)(-1.0*fbias); //was -4
//	ibias_f= ibias_f; 


	if(ptpTransport==e_PTP_MODE_DSL)
	{
		RFE.rfee_f[rfe_cindx].bias = 0.01*(double)(ibias_f);
	}
	else
	{
		RFE.rfee_f[rfe_cindx].bias = (double)(ibias_f);
	}
	if(RFE.rfee_f[rfe_cindx].bias > RFE.rfee_f[rfe_pindx].bias)
	{
		RFE.rfee_f[rfe_pindx].bias = (double)(ibias_f); //GPZ NOV 2010 back annotate higher bias
	}
	if(ptpTransport==e_PTP_MODE_DSL)
	{
		RFE.rfee_f[rfe_cindx].noise = 0.001*xds.xfer_var_est_f;
	}
	else
	{
		RFE.rfee_f[rfe_cindx].noise =  xds.xfer_var_est_f;
	}
	if(RFE.rfee_f[rfe_cindx].noise <10.0)
	{
		RFE.rfee_f[rfe_cindx].noise=10.0; 
		debug_printf(GEORGE_PTP_PRT,"RFE sqrt noise in low: %ld\n",(xds.xfer_var_est_f*1000));
	}
	if((rfe_turbo_flag==FALSE)&&(delta_forward_slew_cnt||(fll_settle>0)))
	{
		RFE.rfee_f[rfe_cindx].tfs  = 0;
		RFE.rfee_f[rfe_pindx].tfs  = 0; //GPZ NOV 12 2010 add protection one update back
		RFE.rfee_f[rfe_ppindx].tfs = 0; //GPZ NOV 12 2010 add protection one update back
		RFE.rfee_f[rfe_findx].tfs  = 2; //GPZ NOV 12 2010 add protection one update back
		if(RFE.first==0)
		{
			 short_skip_cnt_f=45;
		}	
	}
// GPZ April 2011 make independent of turbo	
//	else if((rfe_turbo_flag==FALSE) && ((low_warning_for>2)||(high_warning_for>0))   ) // GPZ NOV 2010 add density warning protection  
	else if((low_warning_for>3)||(high_warning_for>0)) // GPZ NOV 2010 add density warning protection  
	{
		RFE.rfee_f[rfe_cindx].tfs = 0;
		RFE.rfee_f[rfe_pindx].tfs = 0; //GPZ NOV 12 2010 add protection one update back
		RFE.rfee_f[rfe_ppindx].tfs = 0; //GPZ NOV 12 2010 add protection one update back
		RFE.rfee_f[rfe_findx].tfs = 2; //GPZ NOV 12 2010 add protection one update forward
		Forward_Floor_Change_Flag=2;   //GPZ JAN 2011 added protection
		debug_printf(GEORGE_PTP_PRT,"RFE Density Warning for: low %d high %d first %d\n", low_warning_for, high_warning_for,RFE.first );
		if((RFE.first==0)&&(LoadState != LOAD_GENERIC_HIGH))
		{
			 short_skip_cnt_f=800; //GPZ APRIL 2011 extend to bridge turbo mde
		}
	}
#if SMARTCLOCK == 1
	else if( (turbo_cluster_for >0)) //GPZ June 22 new approach allow update with PTP LOS but use estimate
#else
	else if( (turbo_cluster_for >0)|| (PTP_LOS) )
#endif
	{
		RFE.rfee_f[rfe_cindx].tfs = 0; // assume suspect phase during turbo cluster
		RFE.rfee_f[rfe_pindx].tfs = 0; //MAY 2 2011 add protection one step back
//		RFE.rfee_f[rfe_ppindx].tfs = 0; //GPZ NOV 12 2010 add protection one update back
		RFE.rfee_f[rfe_findx].tfs = 2; //GPZ NOV 12 2010 add protection one update forward
		debug_printf(GEORGE_PTP_PRT,"RFE suspect forward phase during turbo cluster: turbo_cnt:%ld, PTP_LOS:%ld\n",(long int)turbo_cluster_for, (long int)PTP_LOS);
		if((RFE.first==0)&&(LoadState != LOAD_GENERIC_HIGH))
		{
			 short_skip_cnt_f=800; //GPZ APRIL 2011 extend to bridge turbo mde
		}
	}
	else
	{
		if(RFE.rfee_f[rfe_cindx].tfs==2)
		{
			RFE.rfee_f[rfe_cindx].tfs = 0;
		}
		else
		{
		RFE.rfee_f[rfe_cindx].tfs = 1;
		if(short_skip_cnt_f>0) short_skip_cnt_f --;
		}
	}
	low_warning_for=0;
	high_warning_for=0;
	debug_printf(GEORGE_PTP_PRT,"RFE  Forward Floor Change Flag Status:%ld\n",(long int) Forward_Floor_Change_Flag);

	if(Forward_Floor_Change_Flag) 
	{
		if(f_floor_event == 0)
		{
				debug_printf(GEORGE_PTP_PRT,"RFE  Forward Floor Change Executed\n" );

			RFE.rfee_f[rfe_cindx].floor_cat=RFE.rfee_f[rfe_pindx].floor_cat+1;
			//GPZ invalid around floor change
			RFE.rfee_f[rfe_cindx].tfs = 0;
			RFE.rfee_f[rfe_pindx].tfs = 0; // GPZ Add protection one update back NOV 12 2010
			RFE.rfee_f[rfe_findx].tfs = 2; // GPZ Add protection one update future NOV 12 2010
			f_floor_event = 1;
		}
		else
		{
			RFE.rfee_f[rfe_cindx].floor_cat=RFE.rfee_f[rfe_pindx].floor_cat;
		}
		Forward_Floor_Change_Flag--;
		if(Forward_Floor_Change_Flag == 0)
		{
			debug_printf(GEORGE_PTP_PRT,"RFE  Forward Floor Change Retired\n" );
			f_floor_event = 0;
		}
		
	}
	else
	{
		RFE.rfee_f[rfe_cindx].floor_cat=RFE.rfee_f[rfe_pindx].floor_cat;
	}
	// reverse stats
	if(rfe_turbo_flag)
	{
//		RFE.rfee_r[rfe_cindx].phase = (double)sdrift_min_reverse+ rfe_turbo_acc;
		if(xds.xfer_turbo_phase[1]<(double)MAXMIN)
		{
#if SMARTCLOCK == 1
			RFE.rfee_r[rfe_cindx].phase = last_good_turbo_phase[1]+ rfe_turbo_acc;
#else
			RFE.rfee_r[rfe_cindx].phase = xds.xfer_turbo_phase[1]+ rfe_turbo_acc;
#endif
			RFE.rfee_r[rfe_cindx].tfs = 1;
		}
		else
		{
			RFE.rfee_r[rfe_cindx].phase=0.0;
			RFE.rfee_r[rfe_cindx].tfs = 0;
			Reverse_Floor_Change_Flag=1;
		}
		// Process Delay line used to manage transients
		RFE_in_reverse[4]=RFE_in_reverse[3];
		RFE_in_reverse[3]=RFE_in_reverse[2];
		RFE_in_reverse[2]=RFE_in_reverse[1];
		RFE_in_reverse[1]=RFE_in_reverse[0];
		RFE_in_reverse[0]=RFE.rfee_r[rfe_cindx].phase;
		//GPZ June 2011 explore using 2nd difference
		delta_phase= RFE_in_reverse[2] - 2.0*RFE_in_reverse[1] + RFE_in_reverse[0];	
//		delta_phase= RFE_in_reverse[1] - RFE_in_reverse[0];	

		if(delta_phase<0.0) delta_phase= -delta_phase;
		if(delta_phase>40000.0)  //was 80000 reduce to 40000 with new 2nd difference
		{
			if ((LoadState != LOAD_GENERIC_HIGH)&&(ptpTransport==e_PTP_MODE_ETHERNET)) //ethernet specific settings
//			if ((LoadState != LOAD_GENERIC_HIGH)&&((ptpTransport==e_PTP_MODE_ETHERNET)||(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS))&&(!RFE.first)) //ethernet specific settings
			{
				PTP_LOS=PTP_LOS_TIMEOUT;
				debug_printf(GEORGE_PTP_PRT,"RFE delta phase pop reverse:%ld\n",(long int)(delta_phase) );
				debug_printf(GEORGE_PTP_PRT,"PTP_LOS delta pop reverse: %d\n",(PTP_LOS));
			}
		}

	}
	else
	{
		RFE.rfee_r[rfe_cindx].phase =  xds.xfer_sacc_r;
	}
	if(PTP_LOS > 0)
	{
		if(hcount_rfe<7200) hcount_rfe++;
      hcount_recovery_rfe = 3600; //1 hour holdover recovery period
//	   debug_printf(GEORGE_PTP_PRT,"In PTP_LOS Holdover: %d\n",(int)(PTP_LOS));

	}
	else
#if 0
//#if SMARTCLOCK == 1
	{
      if(hcount_rfe > 0)
      {
		   hcount_rfe=0;
         rfe_settle_count=15;
      }
	}
#else
	{
      if(rfe_settle_count) rfe_settle_count--;
      if(hcount_rfe > 0)
      {
        hcount_recovery_rfe_start=1;
      }

   	hcount_rfe=0;
	}
#endif
	ibias_r=(long)(rbias/2500.0);
	ibias_r= ibias_r*2500; 
	if(ptpTransport==e_PTP_MODE_DSL)
	{
		RFE.rfee_r[rfe_cindx].bias = 0.01*(double)ibias_r;
	}
	else
	{
		RFE.rfee_r[rfe_cindx].bias = (double)ibias_r;
	}
	if(RFE.rfee_r[rfe_cindx].bias > RFE.rfee_r[rfe_pindx].bias)
	{
		RFE.rfee_r[rfe_pindx].bias = (double)(ibias_r); //GPZ NOV 2010 back annotate higher bias
	}
	if(ptpTransport==e_PTP_MODE_DSL)
	{
		RFE.rfee_r[rfe_cindx].noise = 0.001*xds.xfer_var_est_r;
	}
	else
	{
		RFE.rfee_r[rfe_cindx].noise =  xds.xfer_var_est_r;
	}
	if(RFE.rfee_r[rfe_cindx].noise <10.0)
	{
		RFE.rfee_r[rfe_cindx].noise=10.0; 
		debug_printf(GEORGE_PTP_PRT,"RFE sqrt noise in low: %ld\n",(xds.xfer_var_est_r*1000));
	}
	
	if( (rfe_turbo_flag==FALSE) && (delta_reverse_slew_cnt||(fll_settle>0)) )
	{
		RFE.rfee_r[rfe_cindx].tfs = 0;
		RFE.rfee_r[rfe_pindx].tfs = 0; // GPZ Add protection one update back NOV 12 2010
		RFE.rfee_r[rfe_ppindx].tfs = 0; //GPZ NOV 12 2010 add protection one update back
		RFE.rfee_r[rfe_findx].tfs = 2; // GPZ Add protection one update future NOV 12 2010
		if(RFE.first==0)
		{
			if(short_skip_cnt_f ==0)
			{
				 short_skip_cnt_r=45;
			}
		}	
		
	}
// GPZ April 2011 make independent of turbo	
//	else if( (rfe_turbo_flag==FALSE) && ((low_warning_rev>3)||(high_warning_rev>0))  ) // GPZ NOV 2010 add density warning protection  
	else if((low_warning_rev>6)||(high_warning_rev>1)) // GPZ NOV 2010 add density warning protection  
	{
		RFE.rfee_r[rfe_cindx].tfs = 0;
		RFE.rfee_r[rfe_pindx].tfs = 0; //GPZ NOV 12 2010 add protection one update back
		RFE.rfee_r[rfe_ppindx].tfs = 0; //GPZ NOV 12 2010 add protection one update back
		RFE.rfee_r[rfe_findx].tfs = 2; //GPZ NOV 12 2010 add protection one update future
		Reverse_Floor_Change_Flag=2;   //GPZ JAN 2011 added protection
		
		debug_printf(GEORGE_PTP_PRT,"RFE Density Warning rev: low %d high %d first %d\n", low_warning_rev, high_warning_rev, RFE.first);
		if((RFE.first==0)&&(LoadState != LOAD_GENERIC_HIGH))
		{
			if(short_skip_cnt_f ==0)
			{
				 short_skip_cnt_r=800;  //GPZ APRIL 2011 extend to bridge turbo mode
			}
		}
	}
//	else if(  ((turbo_cluster_rev >0)&&(RFE.first)) || (PTP_LOS))
#if SMARTCLOCK == 1
	else if( (turbo_cluster_rev >0)&&(RFE.first)  ) //GPZ June 22 new approach allow update with PTP LOS but use estimate
#else
	else if( ((turbo_cluster_rev >0)&&(RFE.first) ) ||  (PTP_LOS)) //Reverse suspect phase condition not density related 
#endif
	{
		RFE.rfee_r[rfe_cindx].tfs = 0; // assume suspect phase during turbo cluster
		RFE.rfee_r[rfe_pindx].tfs = 0; //MAY 2 2011 add protection one step back
//		RFE.rfee_r[rfe_ppindx].tfs = 0; //GPZ NOV 12 2010 add protection one update back
		RFE.rfee_r[rfe_findx].tfs = 2; //GPZ NOV 12 2010 add protection one update future
		debug_printf(GEORGE_PTP_PRT,"RFE suspect reverse phase during turbo cluster turbo_cnt:%ld, PTP_LOS:%ld\n",(long int)turbo_cluster_rev, (long int)PTP_LOS );
		if((RFE.first==0)&&(LoadState != LOAD_GENERIC_HIGH))
		{
			if(short_skip_cnt_f ==0)
			{
				 short_skip_cnt_r=800;  //GPZ APRIL 2011 extend to bridge turbo mode
			}
		}

	}
	else
	{
		if(RFE.rfee_r[rfe_cindx].tfs==2)
		{
			RFE.rfee_r[rfe_cindx].tfs = 0;
		}
		else
		{
			RFE.rfee_r[rfe_cindx].tfs = 1;
			if(short_skip_cnt_r>0) short_skip_cnt_r --;
		}
	}
	low_warning_rev=0;
	high_warning_rev=0;
	if(Reverse_Floor_Change_Flag)
	{
	
		if(r_floor_event == 0)
		{
			RFE.rfee_r[rfe_cindx].floor_cat=RFE.rfee_r[rfe_pindx].floor_cat+1;
			r_floor_event = 1;
			//GPZ invalid around floor change
			RFE.rfee_r[rfe_cindx].tfs = 0;
			RFE.rfee_r[rfe_pindx].tfs = 0; //GPZ NOV 12 2010 add protection one update back
			RFE.rfee_r[rfe_findx].tfs = 2; //GPZ NOV 12 2010 add protection one update back
			
		}
		else
		{
			RFE.rfee_r[rfe_cindx].floor_cat=RFE.rfee_r[rfe_pindx].floor_cat;
		}
		Reverse_Floor_Change_Flag--;
		if(Reverse_Floor_Change_Flag == 0)
		{
			r_floor_event = 0;
		}
	}
	else
	{
		RFE.rfee_r[rfe_cindx].floor_cat=RFE.rfee_r[rfe_pindx].floor_cat;
	}
   if((turbo_cluster_for ==0)&& (turbo_cluster_rev ==0)&&(PTP_LOS==0) )
   {
      if(Phase_Control_Enable==1)
      {
         Phase_Control_Enable=0; //GPZ NOV 2011 Only re-enable phase control under transient free conditions
      }
   }
   else Phase_Control_Enable = 2;  //was 6
   if(Phase_Control_Enable> 1) 
   {
      Phase_Control_Enable--;
   }
	if(PTP_LOS>0) 
	{
		PTP_LOS --; // decrement PTP Loss of Signal Count
		debug_printf(GEORGE_PTP_PRT,"PTP_LOS count down: %d\n",(PTP_LOS));
	}
//	Outer window spacing control 4,8,16,32 minute tested
	inner_range_prev=0;
	for(j=1;j<jmax;j++)
	{
	Fix_Space=Fix_Space_List[j]; //GPZ OCT 2010 new method to space windows
//	if( ((short_skip_cnt_f>0) || (short_skip_cnt_r>0)) && (j==1)) Fix_Space=Fix_Space_List[j+1]; // use medium window  APRIL 25 2011
	
//	inner_range=(RFE.N-(Fix_Space+2*j)); //was 4j reduce to 2j
//	inner_range=(RFE.N-(Fix_Space+1)); //GPZ OCT 2010 use maximum available signal
	inner_range=(RFE.N-(Fix_Space)); //GPZ OCT 2010 use maximum available signal
//	Inner fixed spacing version of algorithm
	for(i=0;i<inner_range;i++)
	{
		rfe_start = (rfe_cindx +RFE.N - i)%RFE.N;  //GPZ NOV 2010 start at current not previous index
		rfe_end =   (rfe_cindx +RFE.N - i- Fix_Space)%RFE.N;//GPZ NOV 2010 start at current not previous index
		// update pair valid flag for forward direction
		if((RFE.rfee_f[rfe_start].tfs==0)||(RFE.rfee_f[rfe_end].tfs==0))
		{
			Pair_Valid_Flag=0;
		}
		else if((RFE.rfee_f[rfe_start].floor_cat) != (RFE.rfee_f[rfe_end].floor_cat))
		{
			Pair_Valid_Flag=0;
		}
		else if((RFE.rfee_f[rfe_start].floor_cat)==0)
		{
			Pair_Valid_Flag=2;
		}
		else
		{
			Pair_Valid_Flag=1;
		}
		// update forward frequency if valid
		if(Pair_Valid_Flag)
		{
			fpairs[2*i+inner_range_prev]=(Turbo_Scale)*(RFE.rfee_f[rfe_start].phase -RFE.rfee_f[rfe_end].phase)/(128.0*(double)(Fix_Space));
//			debug_printf(GEORGE_PTP_PRT,"fpairs inner loop: indx: %d, %le, %le, %le\n",(2*i+inner_range_prev),fpairs[2*i+inner_range_prev],RFE.rfee_f[rfe_start].phase,RFE.rfee_f[rfe_end].phase);

			// calculate bias asymmetry factor and instability factor
			comm_bias=(RFE.rfee_f[rfe_start].bias + RFE.rfee_f[rfe_end].bias)/2.0;
			diff_bias=(RFE.rfee_f[rfe_start].bias - RFE.rfee_f[rfe_end].bias);
			if(comm_bias<0.0) comm_bias= - comm_bias;
			if(diff_bias<0.0) diff_bias= - diff_bias;
			if(comm_bias<200.0) comm_bias=200.0 ;
			if(diff_bias<(double)(200.0)) diff_bias=(double)(200.0);
			
//			bias_asymmetry_factor= ((double)(min_oper_thres_oper)/(comm_bias))*((double)(min_oper_thres_oper)/(diff_bias));
			// GPZ Sept 2010 increase de-emphasis of common bias term
			// GPZ NOV 2010 increase de-emphasis of common bias term even more
//			bias_asymmetry_factor= sqrt( (double)(14.14)/(sqrt(comm_bias)) )  *    ((double)(min_oper_thres_oper)/(diff_bias));
//			bias_asymmetry_factor= ( (double)(8.2424e12)  /  ((20000.0+comm_bias)*(20000.0+comm_bias)*(20000.0+comm_bias))  ) 
//							 	 * ( (double)(1.0612e12)  /  ((10000.0+diff_bias)*(10000.0+diff_bias)*(10000.0+diff_bias))  );
			// Rescale for smaller net bias situations  1e-12 10000.0 + comm_bias  and 1e11   5000 + diff_bias
			bias_asymmetry_factor= ( (double)(1.0e11)  /  ((5000.0+comm_bias)*(5000.0+comm_bias)*(5000.0+comm_bias))  ) 
							 	 * ( (double)(1.0e11)  /  ((5000.0+diff_bias)*(5000.0+diff_bias)*(5000.0+diff_bias))  );
//			if(ptpTransport==e_PTP_MODE_ETHERNET)
//			{
//				bias_asymmetry_factor= bias_asymmetry_factor/3.0; //MAY 2011 de-emphasis when operating in G.8261 environment
//			}
							 	 
//			
//         if(RFE.rfee_f[rfe_start].noise + RFE.rfee_f[rfe_end].noise < 0)
//         {
 //collision           debug_printf(GEORGE_PTP_PRT,"rfe_start=%d rfe_end=%d snoise=%le enoise=%le\n",
 //collision              rfe_start, 
 //collision              rfe_end, 
 //collision              RFE.rfee_f[rfe_start].noise,
 //collision              RFE.rfee_f[rfe_end].noise);
//         }

			instability_factor= sqrt(RFE.rfee_f[rfe_start].noise + RFE.rfee_f[rfe_end].noise);
			if(instability_factor<HUNDREDF) instability_factor=HUNDREDF; //bound min noise
			wpairs[2*i+inner_range_prev]=(double)(Fix_Space*Fix_Space)*bias_asymmetry_factor/instability_factor;
         // TEST TEST TEST force constant weight
//			wpairs[2*i+inner_range_prev]=0.01;

#if 1       // SEPT 2011 Added this targeted slew rate limited for fpair correction
            if(((ptpTransport==e_PTP_MODE_ETHERNET)||(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS)) && 
               ((OCXO_Type()==e_MINI_OCXO) || (OCXO_Type()==e_OCXO) || (OCXO_Type()==e_RB)) && 
               !((RFE.Nmax < 100) || (fllstatus.cur_state==FLL_WARMUP)))
            {
               delta_fest= fpairs[(2*i+inner_range_prev)]-RFE.fest_for;
               if(OCXO_Type()==e_MINI_OCXO)
               {
                  slew_limit_local =  25.0;     // was 25.0
               }
               else
               {
                  slew_limit_local =  25.0;      //25.0
               }
//   				debug_printf(GEORGE_PTP_PRT,"fff: delta: %le slew_limit_local: %le fpairs: %le RFE.fest_rev: %le Fix_Space %d\n", 
//                     (double)(delta_fest), 
//                     slew_limit_local,
//                     fpairs[(2*i+inner_range_prev)],
//                     RFE.fest_for,
//                     (int)Fix_Space);
//               LoadState = LOAD_GENERIC_HIGH;
               if(((delta_fest >= slew_limit_local)||(delta_fest <= -slew_limit_local))&&(LoadState != LOAD_GENERIC_HIGH))
               {
        				debug_printf(GEORGE_PTP_PRT,"Exception: rfe slew limit forward: delta:%10ld indx:%5d\n", (long int)(delta_fest), (2*i+inner_range_prev) );
                  fpairs[2*i+inner_range_prev]=ZEROF;
         			wpairs[2*i+inner_range_prev]=ZEROF;
         			bias_asymmetry_factor=ZEROF;
         			instability_factor=ZEROF;
         			Pair_Valid_Flag=0;
//                  Forward_Floor_Change_Flag = 1;
                  RFE.rfee_f[rfe_start].tfs= 0;
                  RFE.rfee_f[(rfe_start+RFE.N-1)%RFE.N].tfs= 0;
                  RFE.rfee_f[(rfe_start+1)%RFE.N].tfs= 2;
               }
               else
               {
                  total++;
               }
            }
            else
            {
               total++;
            }
#else
         total++;
#endif
			if(RFE.first && Pair_Valid_Flag==1) //non-floor zero calibration
			{
				fest_first+=fpairs[2*i+inner_range_prev];
				weight_first+=wpairs[2*i+inner_range_prev];
				total_first++;
			}
		}
     	else
     	{
     		fpairs[2*i+inner_range_prev]=ZEROF;
   			wpairs[2*i+inner_range_prev]=ZEROF;
   			bias_asymmetry_factor=ZEROF;
   			instability_factor=ZEROF;
     	}
     	
		// forward debug print
		if(1)
//		if( (!rfe_turbo_flag)  || ((((RFE.index)%RFE.N)&0x3)==1) )
//		if(((i&0x7)==0) && (rfe_turbo_flag==FALSE) )
		{
			//Print Header
			if(i==0)
			{
				debug_printf(GEORGE_PTP_PRT," indx strt  end  pvf       pstrt        pend  flr    fpairs      wpairs\n");
 			}
			print_index=(2*i+inner_range_prev);
//			scale1=(long int)(RFE.rfee_f[rfe_start].phase);
//			scale6=(long int)(RFE.rfee_f[rfe_end].phase);
//			scale2=(long int)(fpairs[print_index]*THOUSANDF);
//			scale3=(long int)(wpairs[print_index]*MILLIONF);
//			scale4=(long int)(bias_asymmetry_factor*MILLIONF);
//			scale5=(long int)(instability_factor);
			if(print_index<32)
			{
			debug_printf(GEORGE_PTP_PRT,"%5ld%5ld%5ld%5ld%12.3le%12.3le%5ld%12.3le%12.3le\n",
    		(long int)print_index,
    		(long int)rfe_start,
     		(long int)rfe_end,
     		(long int)Pair_Valid_Flag,
     		(RFE.rfee_f[rfe_start].phase),
     		(RFE.rfee_f[rfe_end].phase),
  			(long int)RFE.rfee_f[rfe_start].floor_cat,     		
     		(fpairs[print_index]),
     		(wpairs[print_index])
 //    		(bias_asymmetry_factor),
 //    		(instability_factor)
    		);
			}
     	}
		// end forward forward direction
		// update pair valid flag for reverse direction
		if((RFE.rfee_r[rfe_start].tfs==0)||(RFE.rfee_r[rfe_end].tfs==0))
		{
			Pair_Valid_Flag=0;
		}
		else if((RFE.rfee_r[rfe_start].floor_cat) != (RFE.rfee_r[rfe_end].floor_cat))
		{
			Pair_Valid_Flag=0;
		}
		else if((RFE.rfee_r[rfe_start].floor_cat)==0)
		{
			Pair_Valid_Flag=2;
		}
		else
		{
			Pair_Valid_Flag=1;
		}
		// update reverse frequency if valid
		if(Pair_Valid_Flag)
		{
			fpairs[2*i+1+inner_range_prev]= (Turbo_Scale)*(RFE.rfee_r[rfe_start].phase -RFE.rfee_r[rfe_end].phase)/(128.0*(double)(Fix_Space));
			// calculate bias asymmetry factor and instability factor
			comm_bias=(RFE.rfee_r[rfe_start].bias + RFE.rfee_r[rfe_end].bias)/2.0;
			diff_bias=(RFE.rfee_r[rfe_start].bias - RFE.rfee_r[rfe_end].bias);
			if(diff_bias<0.0) diff_bias= - diff_bias;
			if(comm_bias<0.0) comm_bias= - comm_bias;
			if(comm_bias<min_oper_thres_oper) comm_bias=min_oper_thres_oper ;
			if(diff_bias<(double)(min_oper_thres_oper)) diff_bias=(double)(min_oper_thres_oper);
//			bias_asymmetry_factor= ((double)(min_oper_thres_oper)/(comm_bias))*((double)(min_oper_thres_oper)/(diff_bias));
			// GPZ Sept 2010 increase de-emphasis of common bias term
			// GPZ NOV 2010 increase de-emphasis of common bias term even more
//			bias_asymmetry_factor= sqrt( (double)(14.14)/(sqrt(comm_bias)) )*    ((double)(min_oper_thres_oper)/(diff_bias));
//			bias_asymmetry_factor= ( (double)(8.2424e12)  /  ((20000.0+comm_bias)*(20000.0+comm_bias)*(20000.0+comm_bias))  ) 
//							 	 * ( (double)(1.0612e12)  /  ((10000.0+diff_bias)*(10000.0+diff_bias)*(10000.0+diff_bias))  );
			// Rescale for smaller net bias situations
			bias_asymmetry_factor= ( (double)(1.0e12)  /  ((10000.0+comm_bias)*(10000.0+comm_bias)*(10000.0+comm_bias))  ) 
							 	 * ( (double)(1.0e11)  /  ((5000.0+diff_bias)*(5000.0+diff_bias)*(5000.0+diff_bias))  );
			
			instability_factor= sqrt(RFE.rfee_r[rfe_start].noise + RFE.rfee_r[rfe_end].noise);
			if(instability_factor<HUNDREDF) instability_factor=HUNDREDF; //bound min noise
			wpairs[2*i+1+inner_range_prev]=(double)(Fix_Space*Fix_Space)*bias_asymmetry_factor/instability_factor;
         // TEST TEST TEST force constant weight
//			wpairs[2*i+1+inner_range_prev]= 0.01;

#if 1       // SEPT 2011 Added this targeted slew rate limited for fpair correction
            if(((ptpTransport==e_PTP_MODE_ETHERNET)||(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS)) && 
               ((OCXO_Type()==e_MINI_OCXO) || (OCXO_Type()==e_OCXO) || (OCXO_Type()==e_RB)) && 
               !((RFE.Nmax < 100) || (fllstatus.cur_state==FLL_WARMUP))) 
            {
               delta_fest= fpairs[(2*i+1+inner_range_prev)]-RFE.fest_rev;
               if(OCXO_Type()==e_MINI_OCXO)
               {
                  slew_limit_local =  25.0;     // was 25
               }
               else
               {
                  slew_limit_local =  25.0;      //25
               }
//       				debug_printf(GEORGE_PTP_PRT,"rrr: delta: %le slew_limit_local: %le fpairs: %le RFE.fest_rev: %le\n", 
//                     (double)(delta_fest), 
//                     slew_limit_local,
//                     fpairs[(2*i+1+inner_range_prev)],
//                     RFE.fest_rev);
//               LoadState = LOAD_GENERIC_HIGH;  //TEST TEST TEST

               if(((delta_fest >= slew_limit_local)||(delta_fest <= -(slew_limit_local)))&&(LoadState != LOAD_GENERIC_HIGH))
               {

        				 debug_printf(GEORGE_PTP_PRT,"Exception: rfe slew limit reverse: delta:%10ld slew_limit_local:%5ld\n", (long int)(delta_fest), (long int)slew_limit_local);

              		fpairs[2*i+1+inner_range_prev]=ZEROF;
   		      	wpairs[2*i+1+inner_range_prev]=ZEROF;
   			      bias_asymmetry_factor=ZEROF;
         			Pair_Valid_Flag=0;
   			      instability_factor=ZEROF;
//                Reverse_Floor_Change_Flag=1;
                  RFE.rfee_r[rfe_start].tfs= 0;
                  RFE.rfee_r[(rfe_start+RFE.N-1)%RFE.N].tfs= 0;
                  RFE.rfee_r[(rfe_start+1)%RFE.N].tfs= 2;
                }
               else
               {
                  total++;
               }
            }
            else
            {
               total++;
            }
#else
         total++;
#endif
			if(RFE.first && Pair_Valid_Flag==1) //non-floor zero calibration
			{
				fest_first+=fpairs[2*i+1+inner_range_prev];
				weight_first+=wpairs[2*i+1+inner_range_prev];
				total_first++;
			}
			
		}
    	else
     	{
       		fpairs[2*i+1+inner_range_prev]=ZEROF;
   			wpairs[2*i+1+inner_range_prev]=ZEROF;
   			bias_asymmetry_factor=ZEROF;
   			instability_factor=ZEROF;
     	}
		// reverse debug print
		if(1)
//		if( (!rfe_turbo_flag)  || ((((RFE.index)%RFE.N)&0x3)==1) )
//		if(((i&0x7)==0) && (rfe_turbo_flag==FALSE) )
		{
			print_index=(2*i+1+inner_range_prev);
//			scale1=(long int)(RFE.rfee_r[rfe_start].phase);
//			scale6=(long int)(RFE.rfee_r[rfe_end].phase);
//			scale2=(long int)(fpairs[print_index]*THOUSANDF);
//			scale3=(long int)(wpairs[print_index]*MILLIONF);
//			scale4=(long int)(bias_asymmetry_factor*MILLIONF);
//			scale5=(long int)(instability_factor);
			if(print_index<32)
			{
			debug_printf(GEORGE_PTP_PRT,"%5ld%5ld%5ld%5ld%12.3le%12.3le%5ld%12.3le%12.3le\n",
    		(long int)print_index,
    		(long int)rfe_start,
     		(long int)rfe_end,
     		(long int)Pair_Valid_Flag,
     		(RFE.rfee_r[rfe_start].phase),
     		(RFE.rfee_r[rfe_end].phase),
  			(long int)RFE.rfee_r[rfe_start].floor_cat,     		
     		(fpairs[print_index]),
     		(wpairs[print_index])
 //    		(bias_asymmetry_factor),
 //    		(instability_factor)
    		);
			}
		}
		// end reverse direction
	} //end for all fixed spaced pairs i-indexed inner loop
		inner_range_prev+=2.0* inner_range;
		Fix_Space=Fix_Space+Fix_Space_Inc; //double window each pass
//collision		debug_printf(GEORGE_PTP_PRT,"rfe: new inner range prev:%5d\n",inner_range_prev);
	} //end j-indexed outer loop
	// update fest 
   weight_for=0.0;
   weight_rev=0.0;	
  	if(total >0) //was zero make minimum 4
	{
		for(i=0;i<8;i++) //Ken_Post_Merge
		{
			if(wpairs[i] != ZEROF)
			{
				// Ken_Post_Merge This Block Replaces below
				if((i%2)==0) //even samples are forward
				{
					fest_for+= wpairs[i]*fpairs[i];
					weight_for+= wpairs[i];
				}
				else
				{
					fest_rev+= wpairs[i]*fpairs[i];
					weight_rev+= wpairs[i];
				}
			}
			// End new Ken_Post_Merge Block
//			fest  += wpairs[i]*fpairs[i];
//			weight+= wpairs[i];
		}
		// Ken_Post_Merge Block
		weight= weight_for+ weight_rev; // Ken_Post_Merge
		if(weight_for >ZEROF)
//		if(weight_for >ZEROF)
		{
			fest_for= fest_for/weight_for;
		}
		else
		{
//			fest_for=RFE.fest;
		}
		if(weight_rev >ZEROF)
//		if(weight_rev >ZEROF)
		{
			fest_rev= fest_rev/weight_rev;
		}
		else
		{
//			fest_rev=RFE.fest;
		}
		if(weight >0.0)
		{
			weight_for= weight_for/weight;
			weight_rev= weight_rev/weight;
		}
		else
		{
			weight_for=xds.xfer_in_weight_f;
			weight_rev=xds.xfer_in_weight_r;
		}	
		// APRIL 25 2011 just use RFE weighting in turbo mode
		if(rfe_turbo_flag==0)
		{
			if((xds.xfer_in_weight_f < weight_for) && (xds.xfer_in_weight_f<0.10))
			{
				weight_for= xds.xfer_in_weight_f;
				weight_rev= 1.0 - xds.xfer_in_weight_f;
			}
		}
   }
   if((weight_rev > 0.0) || ((RFE.first > 0)&& (total > 0)) )  //assymetrical tendency toward reverse channel 
   {
#if 1 // APRIL 25 2011		
		if( (short_skip_cnt_f>0) && (short_skip_cnt_r==0) && (weight_rev > 0.0) )
		{
			weight_for = 0.0;
			weight_rev = 1.0;
		}
		else if( (short_skip_cnt_r>0) && (short_skip_cnt_f==0) && (weight_for > 0.0))
		{
			weight_for = 1.0;
			weight_rev = 0.0;
		}
//      if(ptpTransport==e_PTP_MODE_ETHERNET)
//      {
//         if(weight_rev > 0.0)
//         {
//            if(weight_rev < 0.95)
//            {
//               weight_rev = 0.95;
//               weight_for = 1 - weight _rev;
//            }
//         }
//      }
//			weight_for = 0.0;  //TEST TEST TEST 
//			weight_rev = 1.0;	 //TEST TEST TEST 

#endif	
		RFE.weight_for= weight_for;
		RFE.weight_rev= weight_rev;
      RFE.delta_freq = -RFE.fest_cur;
		RFE.fest_cur= (fest_for)*RFE.weight_for +(fest_rev)*RFE.weight_rev;
      RFE.delta_freq += RFE.fest_cur;
		// GPZ MAY 2011 with new turbo mode try to skip first processing
//		if(rfe_turbo_flag)
//		{
//			RFE.first=0;	
//		}
		if(RFE.first)
		{
#if 0
			// GPZ MAY 2011 Just use standard alpha
			RFE.wavg = (1.0-(1.0*RFE.alpha))*RFE.wavg + weight*(1.0*RFE.alpha);
			RFE.dfactor=1.0;
			// GPZ OCT 2010 always use no alpha shaping during first mode if Rb
			if(RFE.Nmax < 15)
//			if( (RFE.index < (N_init_rfe+4)%RFE.N)|| (OCXO_Type()==e_RB) ) 
			{
				RFE.fest = RFE.fest_for = RFE.fest_rev = RFE.fest_cur; 
			}
			else
			{
				RFE.fest = (1.0-(1.0*RFE.alpha))*RFE.fest + RFE.fest_cur*(1.0*RFE.alpha);
				if(weight_for > ZEROF) 
				{
					RFE.fest_for = (1.0-(1.0*RFE.alpha))*RFE.fest_for + (fest_for)*(1.0*RFE.alpha);
				}
				if(weight_rev > ZEROF) 
				{
					RFE.fest_rev = (1.0-(1.0*RFE.alpha))*RFE.fest_rev + (fest_rev)*(1.0*RFE.alpha);
				}
			}
#endif
			// Test for large offset condition
			if(mode_jitter==0)
			{
				delta_first_thres= 250.0;  //was 250
				weight_first_thres= 1e-6;
				total_first_thres=2;  //was 1000  Cold Start Fix Feb 8 1 of 3
			}
			else
			{
				delta_first_thres= 250.0;  //was 250
				weight_first_thres= 1e-8;
				total_first_thres=2;//was 1000 was 1000  Cold Start Fix Feb 8 2 of 3
			}	
			if(total_first>total_first_thres)
			{
				fest_first = fest_first/(double)(total_first);
				weight_first = weight_first/(double)(total_first);
				delta_fest= RFE.fest- fest_first;
				if(delta_fest<0.0) delta_fest= -delta_fest;
				debug_printf(GEORGE_PTP_PRT,"rfe large offset:  total: %d, weight: %le, fest_first:%le, fest:%le, delta:%le\n",
				total_first,weight_first,fest_first,RFE.fest,delta_fest);
//				if(RFE.first>0) RFE.first--;  //Cold Start Fix Feb 8 3 of 3
				// HQ APRIL 2011
//				if((delta_fest > delta_first_thres)&&(OCXO_Type()!=e_RB)&& (mode_jitter==0)&& ((min_oper_thres_f<2400)||(min_oper_thres_r<2400))    )
				if((delta_fest > delta_first_thres)&&(OCXO_Type()!=e_RB)&& (mode_jitter==0)    )
				{
//					if((weight_first > weight_first_thres) && (Is_PTP_FREQ_ACTIVE()==TRUE))
/* Only do this major frequency change if PTP is active and PHY is still first */
#if(NO_PHY==0)
					if((Is_PTP_FREQ_ACTIVE()==TRUE)&& (RFE_PHY.first))
#else
					if((Is_PTP_FREQ_ACTIVE()==TRUE))

#endif
					{
						rfe_turbo_acc = 0.0;
						clr_residual_phase();
						init_pshape();				
						#if(SYNC_E_ENABLE==1)
//						init_se();
						#endif
						fdrift_f=fdrift_r=fdrift=fdrift_warped=fdrift_smooth=pll_int_r=pll_int_f= fdrift_raw= holdover_f=holdover_r=fest_first;
			         debug_printf(GEORGE_PTP_PRT,"Jam Starting Freq fdrift_warped:  %le, fest_first:%le\n",fdrift_warped, fest_first);

						RFE.fest=RFE.fest_for=RFE.fest_rev=xds.xfer_fest=fest_first;
						init_rfe();
						if(rfe_turbo_flag)
						{
							RFE.first=0;
						}
						else
						{	
							if(RFE.first<3)
							{
								RFE.first=2; //Stay in first for at least two cycles under cold start
							}
						}
						setfreq(HUNDREDF*fest_first, 0);
						RFE.res_freq_err=0.0;
						RFE.delta_freq=0.0;
						RFE.smooth_delta_freq=0.0;
						RFE.fest_cur=fest_first;
						fest_for=fest_first;
						fest_rev=fest_first;
						Forward_Floor_Change_Flag=2; //GPZ NOV 2010 force floor change during holdover recovery
						Reverse_Floor_Change_Flag=2;
						
					}
				}
			}
		}
//		else
//#if SMARTCLOCK == 1
//      if(rfe_settle_count == 0)
//#endif
      if(rfe_settle_count == 0)

		{
			if(RFE.Nmax<18)  //was 18
			{
  				RFE.res_freq_err=0.0;
  				RFE.delta_freq=0.0;
  				RFE.smooth_delta_freq=0.0;
//            if((FLL_Elapse_Sec_Count_Use<7200)&&(FLL_Elapse_Sec_PTP_Count<7200)&&(FLL_Elapse_Sec_Count<7200))
            if(FLL_Elapse_Sec_PTP_Count<7200)
            {
	   			debug_printf(GEORGE_PTP_PRT, "fast PTP alpha before 7200:\n");

               if((OCXO_Type()) == e_MINI_OCXO) 
               {
   		         alpha_local= 1.0;	//was 0.18
               }
               else
               {   
   			      alpha_local= 1.0;	//was 0.18
               }
            }
            else
            {
   			   alpha_local= RFE.alpha;	
            }
 			}
        else if(RFE.Nmax<36)
        {
//            if((FLL_Elapse_Sec_Count_Use<7200)&&(FLL_Elapse_Sec_PTP_Count<7200)&&(FLL_Elapse_Sec_Count<7200))
           if(FLL_Elapse_Sec_PTP_Count<7200)
           {
    				RFE.res_freq_err=0.0;
    				RFE.delta_freq=0.0;
    				RFE.smooth_delta_freq=0.0;
      		   alpha_local= 1.0;
           }	
           else
           {
      		   alpha_local= RFE.alpha;	
    				RFE.res_freq_err=0.0;
    				RFE.delta_freq=0.0;
    				RFE.smooth_delta_freq=0.0;
      		   alpha_local= 1.0;

           } 
        } 
        else
        {
   		   alpha_local= RFE.alpha;	
        }
			RFE.wavg = (1.0-(alpha_local))*RFE.wavg + weight*alpha_local;
			// update dfactor
			if(RFE.wavg>ZEROF)
			{
				RFE.dfactor= weight/(RFE.wavg);
				if(RFE.dfactor> 4.0) RFE.dfactor=4.0;
				else if (RFE.dfactor<0.02) RFE.dfactor=0.02;
			}
			else
			{
				RFE.dfactor=1.0;
			}
         if((OCXO_Type()) == e_MINI_OCXO) //5.8175 was 0.1 Beta
         {
				RFE.dfactor=1.0; //TEST Alway force dfactor to unity gain
  			   RFE.smooth_delta_freq = (RFE.smooth_delta_freq*(1.0-0.004))  + (RFE.delta_freq*(0.004)); // update smooth delta freq was 0.008
            RFE.res_freq_err =  RFE.smooth_delta_freq*(4.88/sqrt(alpha_local));//project error forward to compensate drift alpha = 1 over 12 yields 4.76 1 over 24 yields 5.817
         }
         else if((OCXO_Type()) == e_OCXO) //10.41 with 0.1 Beta
         {
				RFE.dfactor=1.0; //TEST Alway force dfactor to unity gain 0.012
     			RFE.smooth_delta_freq = (RFE.smooth_delta_freq*(1.0-0.008))  + (RFE.delta_freq*(0.008)); // update smooth delta freq
            RFE.res_freq_err =  RFE.smooth_delta_freq*(5.9/sqrt(alpha_local));//project error forward to compensate drift 8.53    1 over 30 yields 6.57
        }
         else
         {
//            RFE.res_freq_err=0.0;
				RFE.dfactor=1.0; //TEST Alway force dfactor to unity gain 0.012
     			RFE.smooth_delta_freq = (RFE.smooth_delta_freq*(1.0-0.008))  + (RFE.delta_freq*(0.008)); // update smooth delta freq
            RFE.res_freq_err =  RFE.smooth_delta_freq*(5.9/sqrt(alpha_local));//project error forward to compensate drift 8.53    1 over 30 yields 6.57

         }
//			RFE.res_freq_err=0.0; //TEST TEST TEST disable 2nd order control
  			dtemp = (1.0-(alpha_local*RFE.dfactor))*(RFE.fest-RFE.res_freq_err) + RFE.fest_cur*alpha_local*RFE.dfactor;
#if 0
			RFE.delta_freq =dtemp - (RFE.fest - RFE.res_freq_err);
			RFE.smooth_delta_freq = (RFE.smooth_delta_freq*(1.0-alpha_local*0.06))  + (RFE.delta_freq*(alpha_local*0.06));
 			RFE.res_freq_err = 1.0*RFE.smooth_delta_freq*(0.977/(alpha_local) + 1.7);
#endif
//			RFE.fest=dtemp +RFE.res_freq_err;
         delta_fest= (dtemp +RFE.res_freq_err)-RFE.fest;
         RFE.fest += delta_fest;
//         if(hcount_recovery_rfe > 1000)
//         {
//            RFE.fest = (RFE.fest + (hcount_recovery_rfe - 1000 )*holdover_f)/(hcount_recovery_rfe - 999);
//         }
   		if(weight_for > ZEROF) 
			{
				RFE.fest_for = (1.0-(alpha_local*RFE.dfactor))*RFE.fest_for + (fest_for)*alpha_local*RFE.dfactor;
			}
			if(weight_rev > ZEROF) 
			{
				RFE.fest_rev = (1.0-(alpha_local*RFE.dfactor))*RFE.fest_rev + (fest_rev)*alpha_local*RFE.dfactor;
			}
		}	
	}  //OCT 11 total meets minimum
	else
	{
		debug_printf(GEORGE_PTP_PRT, "RFE_Flywheel use predictor: %le %le %le\n",RFE.fest,holdover_f, time_bias_est_rate);

		RFE.fest=holdover_f-time_bias_est_rate; //FEB 1 TEST TEST TEST add 
//      RFE.fest_cur = RFE.fest;
//      hcount_recovery_rfe = 3600; //FEB 1 TEST TEST Remove
	}


#if 1
	debug_printf(GEORGE_PTP_PRT,"rfe_a:%5d,fest:%ld,total:%ld,sdelta:%ld,res:%ld,set:%ld,indx:%3d,N:%3d,Nmax:%3d,TW:%ld,AW:%ld,df:%ld\n",
     FLL_Elapse_Sec_PTP_Count,
     (long)(RFE.fest*1000.0),
     total,
     (long)(RFE.smooth_delta_freq*1000.0),
     (long)(RFE.res_freq_err*1000.0),
     (long)rfe_settle_count,
     RFE.index,
     RFE.N,
     RFE.Nmax,
     (long)(weight*1000.0),
     (long)(RFE.wavg*1000.0),
     (long)(RFE.dfactor*1000.0));
	debug_printf(GEORGE_PTP_PRT,"rfe_b:%5d,fest_for:%ld,fest_rev:%ld,fest_c:%ld,fest_f:%ld,fest_r:%ld,weight_f:%ld,weight_r:%ld,alpha:%ld, tscale: %ld, short_f: %d, short_r %d, turbo:%d\n",
     FLL_Elapse_Sec_PTP_Count,
     (long) fest_for,
     (long) fest_rev,
     (long)(RFE.fest_cur*1000.0),
     (long)(RFE.fest_for*1000.0),
     (long)(RFE.fest_rev*1000.0),
     (long)(RFE.weight_for*1000.0),
     (long)(RFE.weight_rev*1000.0),
     (long)(alpha_local*1000.0),
	  (long)Turbo_Scale,
	 short_skip_cnt_f,
	 short_skip_cnt_r,
     rfe_turbo_flag
     );
#endif
  
}
///////////////////////////////////////////////////////////////////////
// This function call initializes fit calculator
///////////////////////////////////////////////////////////////////////
void init_rfe(void)
{
	int i;
	RFE.Nmax=0;
   rfe_turbo_acc = 0.0; //Good Practice added FEB 11 2012
	if((OCXO_Type()==e_MINI_OCXO)&& (mode_jitter==0))
	{
		RFE.N=12; //was 8
//		debug_printf(GEORGE_PTP_PRT,"init_rfe: mini_ocxo low jitter mode \n");
	}
	else if((OCXO_Type()==e_OCXO)&& (mode_jitter==0))
	{
		RFE.N=12;
	}
	else
	{
		RFE.N=12;
//		debug_printf(GEORGE_PTP_PRT,"init_rfe: ocxo or RB mode \n");
	}	
	RFE.index=0;
	if(OCXO_Type()!=e_RB)
	{	
		RFE.first=4; //TEST TEST TEST was 4 
	}	
	else
	{
		RFE.first=4; //Sept 2010 use First Mode with Rb as well
	}
	RFE.alpha=1.0/15.0; //oscillator dependent smoothing gain across update default 1/60
	RFE.dfactor=1.0; // additional de weighting factor based on total weight 0.1 to 1
	RFE.wavg=ZEROF;  //running average weight using alpha factor
	RFE.weight_for= ZEROF;
	RFE.weight_rev= 1.0;
	
	
	for(i=0;i<RFE.N;i++)
	{
		RFE.rfee_f[i].phase=ZEROF;
		RFE.rfee_f[i].bias=ZEROF;
		RFE.rfee_f[i].noise=ZEROF;
		RFE.rfee_f[i].tfs=0; //0 is FALSE do not use
		RFE.rfee_f[i].floor_cat=0; //0 is FALSE do not use
		RFE.rfee_r[i].phase=ZEROF;
		RFE.rfee_r[i].bias=ZEROF;
		RFE.rfee_r[i].noise=ZEROF;
		
		RFE.rfee_r[i].tfs=0; //0 is FALSE do not use
		RFE.rfee_r[i].floor_cat=0; //0 is FALSE do not use
	}
}
///////////////////////////////////////////////////////////////////////
// This function call starts rfe calculator
///////////////////////////////////////////////////////////////////////
void start_rfe(double in)
{
	int i;
	int blank;
	double inc_phase;
	RFE.fest=in;
	RFE.fest_prev=in;
	xds.xfer_fest=xds.xfer_fest_f=xds.xfer_fest_r=RFE.fest;
	if(rfe_turbo_flag==1)
	{
		inc_phase= 16.0*in; //turbo mode 32 seconds 4x change to 16

		if(OCXO_Type() == e_MINI_OCXO)
		{
			inc_phase= 16.0*in; //turbo mode 32 seconds 4x change to 16
		}
		else if(OCXO_Type() == e_OCXO)
		{
			inc_phase= 16.0*in; //turbo mode 32 seconds 4x change to 16
		}
		else
		{
			inc_phase= 16.0*in; //turbo mode 32 seconds 4x change to 16
		}

	}
	else if(rfe_turbo_flag==2)
	{
		inc_phase= 8.0*in; //turbo mode 16 seconds 16x
	}
	else
	{
		inc_phase= 128.0*in; //standard mode 120 seconds 
	}
	Get_Freq_Corr(&last_good_freq,&gaptime);
	if(mode_jitter==1)
	{
		if(gaptime<20000) gaptime=20000;
	}
	else
	{
		if(gaptime<20000) gaptime=20000;
	}
	if(gaptime>7776000) gaptime= 7776000; //3 months	
	//GPZ Aug 2010 correct start init logic	
	if((OCXO_Type()==e_MINI_OCXO)&& (mode_jitter==0))
	{
		N_init_rfe=12; //was 8
		blank=0;
	}
	else if((OCXO_Type()==e_OCXO)&& (mode_jitter==0))
	{
		N_init_rfe=12; //was 15
		blank=0;
	}
	else
	{
		N_init_rfe=12;
		blank=0; //was 10
	}
	blank = 0; //TEST TEST TEST	
	for(i=0;i<N_init_rfe;i++)
	{
		RFE.rfee_f[i].phase=(double)(i) * inc_phase;
		if(mode_jitter==1)
		{
			RFE.rfee_f[i].bias=10.0*(double)min_oper_thres_oper*sqrt((double)(gaptime)/10000.0); //TODO Factor in Gap Time
		}
		else
		{
			RFE.rfee_f[i].bias=10.0*(double)min_oper_thres_oper*sqrt((double)(gaptime)/10000.0); //TODO Factor in Gap Time
		}
		if(OCXO_Type()==e_RB)
		{
			RFE.rfee_f[i].bias= (double)min_oper_thres_oper;
		}
		RFE.rfee_f[i].noise=(double)(min_oper_thres_oper)*(double)(min_oper_thres_oper)*100.0;
		RFE.rfee_f[i].tfs=1; 
		RFE.rfee_r[i].tfs=1; 
		RFE.rfee_f[i].floor_cat=0; 
		RFE.rfee_r[i].floor_cat=0; 
		RFE.rfee_r[i].phase=RFE.rfee_f[i].phase;
		RFE.rfee_r[i].bias=RFE.rfee_f[i].bias;
		RFE.rfee_r[i].noise=RFE.rfee_f[i].noise;
		//GPZ SEPT 2010 invalided partial block to speed up settling for initial conditions
		if(i<blank)
		{
			RFE.rfee_f[i].tfs=0; 
			RFE.rfee_r[i].tfs=0; 
		}
		else
		{
			RFE.rfee_f[i].tfs=1; 
			RFE.rfee_r[i].tfs=1; 
		}
		debug_printf(GEORGE_PTP_PRT,"rfe_start:i:%3d,fphase:%ld,:rphase%ld\n",
	   		i,
     		(long)RFE.rfee_f[i].phase,
     		(long)RFE.rfee_r[i].phase
     		);
	}
		RFE.index=N_init_rfe-1;
		Forward_Floor_Change_Flag=2;
		Reverse_Floor_Change_Flag=2;
		f_floor_event = 0;
		r_floor_event = 0;		
//		skip_rfe=128;
		skip_rfe=64;
}
///////////////////////////////////////////////////////////////////////
// This function does warm restart of RFE
///////////////////////////////////////////////////////////////////////
void warm_start_rfe(double in)
{
	int i;
	double inc_phase;
	RFE.fest=in;
	RFE.fest_prev=in;
	xds.xfer_fest=xds.xfer_fest_f=xds.xfer_fest_r=RFE.fest;
   RFE.fest_for=RFE.fest_rev=RFE.fest;
   rfe_turbo_acc = 0.0; //Restart at know place

	if(rfe_turbo_flag==1)
	{
		inc_phase= 16.0*in; //turbo mode 32 seconds 4x change to 16

	}
	else if(rfe_turbo_flag==2)
	{
		inc_phase= 8.0*in; //turbo mode 16 seconds 16x
	}
	else
	{
		inc_phase= 128.0*in; //standard mode 120 seconds 
	}
   gaptime = 20000;
	N_init_rfe=12; 
		RFE_in_forward[4]=xds.xfer_turbo_phase[0];
		RFE_in_forward[3]=xds.xfer_turbo_phase[0];
		RFE_in_forward[2]=xds.xfer_turbo_phase[0];
		RFE_in_forward[1]=xds.xfer_turbo_phase[0];
		RFE_in_forward[0]=xds.xfer_turbo_phase[0];

		RFE_in_reverse[4]=xds.xfer_turbo_phase[1];
		RFE_in_reverse[3]=xds.xfer_turbo_phase[1];
		RFE_in_reverse[2]=xds.xfer_turbo_phase[1];
		RFE_in_reverse[1]=xds.xfer_turbo_phase[1];
		RFE_in_reverse[0]=xds.xfer_turbo_phase[1];

	for(i=(N_init_rfe-1);i >=0;i--)

	{
		RFE.rfee_f[i].phase=((double)(i-N_init_rfe+1) * inc_phase)+ xds.xfer_turbo_phase[0];
		if(mode_jitter==1)
		{

			RFE.rfee_f[i].bias=10.0*(double)min_oper_thres_oper*sqrt((double)(gaptime)/10000.0); //TODO Factor in Gap Time
		}
		else
		{
			RFE.rfee_f[i].bias=10.0*(double)min_oper_thres_oper*sqrt((double)(gaptime)/10000.0); //TODO Factor in Gap Time
		}
		if(OCXO_Type()==e_RB)
		{
			RFE.rfee_f[i].bias= (double)min_oper_thres_oper;
		}
		RFE.rfee_f[i].noise=(double)(min_oper_thres_oper)*(double)(min_oper_thres_oper)*100.0;
		RFE.rfee_f[i].tfs=1; 
		RFE.rfee_r[i].tfs=1; 
		RFE.rfee_f[i].floor_cat=0; 
		RFE.rfee_r[i].floor_cat=0; 
		RFE.rfee_r[i].phase=((double)(i-N_init_rfe+1) * inc_phase)+ xds.xfer_turbo_phase[1];
		RFE.rfee_r[i].bias=RFE.rfee_f[i].bias;
		RFE.rfee_r[i].noise=RFE.rfee_f[i].noise;
		debug_printf(GEORGE_PTP_PRT,"warm_rfe_start:i:%3d,fphase:%ld,:rphase%ld, xtp_f: %ld ,xtp_r: %ld\n",
	   		i,
     		(long)RFE.rfee_f[i].phase,
     		(long)RFE.rfee_r[i].phase,
         (long)xds.xfer_turbo_phase[0],
         (long)xds.xfer_turbo_phase[1]
     		);
	}
		RFE.index=N_init_rfe-1;
}

#if (NO_IPPM == 0)
///////////////////////////////////////////////////////////////////////
// This function analysis ippm data stability metrics 
// call in one minute task to conserve  CPU
///////////////////////////////////////////////////////////////////////
void ippm_analysis(void)
{
//	int i;
	int ippm_indx,scan_indx;
	int jcnt_f,jcnt_r;
	int tcnt_f,tcnt_r;
	double out_jitter_f, out_jitter_r, out_ipdv_f, out_ipdv_r;
	double out_ipdv_99_9_f,out_ipdv_99_9_r,out_thres_prob_f,out_thres_prob_r;
	ippm_indx= (ippm_forward.ies_head+255)%256;
	//set current 1 minutes stats
	scan_indx=ippm_indx;
	ippm_forward.cur_ipdv_1min=ZEROF;	
	ippm_reverse.cur_ipdv_1min=ZEROF;
	ippm_forward.cur_ipdv_99_9_1min	=ZEROF;	
	ippm_reverse.cur_ipdv_99_9_1min	=ZEROF;
	ippm_forward.cur_thres_prob_1min=ZEROF;		
	ippm_reverse.cur_thres_prob_1min=ZEROF;		
	jcnt_f=0;
	jcnt_r=0;
	tcnt_f=0;
	tcnt_r=0;
	ippm_forward.cur_jitter_1min=ZEROF;	
	ippm_reverse.cur_jitter_1min=ZEROF;	
	// do forward and reverse separate
	//forward analysis section	
	while(scan_indx != (ippm_forward.ies_head+196)%256)
	{
		// update ipdv 
		// Temporary code need to search window for max
		ippm_forward.cur_ipdv_1sec= (double)ippm_forward.ies[scan_indx].ipdv_max;	
		if((ippm_forward.cur_ipdv_1sec)>(ippm_forward.cur_ipdv_1min))
		{
			ippm_forward.cur_ipdv_1min=ippm_forward.cur_ipdv_1sec;
		}
		// Temporary need to average over window
		ippm_forward.cur_ipdv_99_9_1sec= (double)ippm_forward.ies[scan_indx].thres_99_9;	
		ippm_forward.cur_ipdv_99_9_1min	+= 	ippm_forward.cur_ipdv_99_9_1sec;
		if(ippm_forward.ies[scan_indx].total_thres_cnt>0)
		{
			ippm_forward.cur_thres_prob_1sec= (double)ippm_forward.ies[scan_indx].under_thres_cnt/(double)ippm_forward.ies[scan_indx].total_thres_cnt;
			ippm_forward.cur_thres_prob_1min +=	ippm_forward.cur_thres_prob_1sec;
			tcnt_f++;
		}
		// Temporary need to rms average over window
		if(ippm_forward.ies[scan_indx].jitter_cnt>0)
		{
			ippm_forward.cur_jitter_1sec= (double)(ippm_forward.ies[scan_indx].jitter_sum)/(double)(ippm_forward.ies[scan_indx].jitter_cnt);	
			jcnt_f++;
			ippm_forward.cur_jitter_1min += ippm_forward.cur_jitter_1sec*ippm_forward.cur_jitter_1sec;
		}	
		scan_indx= (scan_indx+255)%256;	
	} // end while construct current 1 minute stats
	// normalize one minute stats
	ippm_forward.cur_ipdv_99_9_1min =	ippm_forward.cur_ipdv_99_9_1min/60.0;
	if(tcnt_f)
	{
		ippm_forward.cur_thres_prob_1min = ippm_forward.cur_thres_prob_1min/tcnt_f;
	}
	if(jcnt_f)
	{
		ippm_forward.cur_jitter_1min=ippm_forward.cur_jitter_1min/jcnt_f;
		ippm_forward.cur_jitter_1min=sqrt(ippm_forward.cur_jitter_1min);	
	}
	// End forward analysis section	
	
	ippm_indx= (ippm_reverse.ies_head+255)%256;
	scan_indx=ippm_indx;
	//reverse analysis section	
	while(scan_indx != (ippm_reverse.ies_head+196)%256)
	{
		// update ipdv 
		// Temporary code need to search window for max
		ippm_reverse.cur_ipdv_1sec= (double)ippm_reverse.ies[scan_indx].ipdv_max;
		if((ippm_reverse.cur_ipdv_1sec)>(ippm_reverse.cur_ipdv_1min))
		{
			ippm_reverse.cur_ipdv_1min=ippm_reverse.cur_ipdv_1sec;
		}
		// Temporary need to average over window
		ippm_reverse.cur_ipdv_99_9_1sec= (double)ippm_reverse.ies[scan_indx].thres_99_9;
		ippm_reverse.cur_ipdv_99_9_1min	+= 	ippm_reverse.cur_ipdv_99_9_1sec;
		if(ippm_reverse.ies[scan_indx].total_thres_cnt>0)
		{
			ippm_reverse.cur_thres_prob_1sec= (double)ippm_reverse.ies[scan_indx].under_thres_cnt/(double)ippm_reverse.ies[scan_indx].total_thres_cnt;
			ippm_reverse.cur_thres_prob_1min +=	ippm_reverse.cur_thres_prob_1sec;
			tcnt_r++;
		}		
		// Temporary need to rms average over window
		if(ippm_reverse.ies[scan_indx].jitter_cnt>0)
		{
			ippm_reverse.cur_jitter_1sec= (double)(ippm_reverse.ies[scan_indx].jitter_sum)/(double)(ippm_reverse.ies[scan_indx].jitter_cnt);	
			jcnt_r++;
			ippm_reverse.cur_jitter_1min += ippm_reverse.cur_jitter_1sec*ippm_reverse.cur_jitter_1sec;
		}	
		scan_indx= (scan_indx+255)%256;	
	} // end while construct current 1 minute stats
	// normalize one minute stats
	ippm_reverse.cur_ipdv_99_9_1min =	ippm_reverse.cur_ipdv_99_9_1min/60.0;
	if(tcnt_r)
	{
		ippm_reverse.cur_thres_prob_1min = ippm_reverse.cur_thres_prob_1min/tcnt_r;
	}
	if(jcnt_r)
	{
		ippm_reverse.cur_jitter_1min=ippm_reverse.cur_jitter_1min/jcnt_r;
		ippm_reverse.cur_jitter_1min=sqrt(ippm_reverse.cur_jitter_1min);	
	}
	// End reverse analysis section		
	//update one minute index
	ippm_forward.ios[ippm_forward.ios_head].jitter=ippm_forward.cur_jitter_1min;
	ippm_reverse.ios[ippm_reverse.ios_head].jitter=ippm_reverse.cur_jitter_1min;
	ippm_forward.ios[ippm_forward.ios_head].ipdv=ippm_forward.cur_ipdv_1min;
	ippm_reverse.ios[ippm_reverse.ios_head].ipdv=ippm_reverse.cur_ipdv_1min;
	ippm_forward.ios[ippm_forward.ios_head].ipdv_99_9=ippm_forward.cur_ipdv_99_9_1min;
	ippm_reverse.ios[ippm_reverse.ios_head].ipdv_99_9=ippm_reverse.cur_ipdv_99_9_1min;
	ippm_forward.ios[ippm_forward.ios_head].thres_prob=ippm_forward.cur_thres_prob_1min;
	ippm_reverse.ios[ippm_reverse.ios_head].thres_prob=ippm_reverse.cur_thres_prob_1min;
	#if 0		
	debug_printf(GEORGE_PTP_PRT,"ippm_indx: %d\n", ippm_indx);
	debug_printf(GEORGE_PTP_PRT, "IPDV forward: %le, reverse %le\n",ippm_forward.cur_ipdv_1min,ippm_reverse.cur_ipdv_1min);
	debug_printf(GEORGE_PTP_PRT, "IPDV_99.9 forward: %le, reverse %le\n",ippm_forward.cur_ipdv_99_9_1min,ippm_reverse.cur_ipdv_99_9_1min);
	debug_printf(GEORGE_PTP_PRT, "jitter forward: %le, reverse %le\n",ippm_forward.cur_jitter_1min,ippm_reverse.cur_jitter_1min);
	debug_printf(GEORGE_PTP_PRT, "thres prob forward: %le, reverse %le\n",ippm_forward.cur_thres_prob_1min,ippm_reverse.cur_thres_prob_1min);
	debug_printf(GEORGE_PTP_PRT, "index validity ies head forward: %d, reverse %d ios head forward: %d, reverse %d\n",ippm_forward.ies_head,
	ippm_reverse.ies_head,
	ippm_forward.ios_head,
	ippm_reverse.ios_head);
	debug_printf(GEORGE_PTP_PRT, "tcount validity  forward: %d, reverse %d \n",tcnt_f,tcnt_r);
	#endif
	if(ippm_forward.N_valid< ippm_forward.window)
	{
		ippm_forward.N_valid++;	
	}
	if(ippm_reverse.N_valid< ippm_reverse.window)
	{
		ippm_reverse.N_valid++;	
	}
	// generate output stats
	ippm_indx= (ippm_forward.ios_head);
	scan_indx=ippm_indx;
	out_ipdv_f=ZEROF;	
	out_ipdv_r=ZEROF;
	out_ipdv_99_9_f	=ZEROF;	
	out_ipdv_99_9_r	=ZEROF;
	out_thres_prob_f=ZEROF;		
	out_thres_prob_r=ZEROF;		
	jcnt_f=0;
	jcnt_r=0;
	out_jitter_f=ZEROF;	
	out_jitter_r=ZEROF;	
	while(scan_indx != (ippm_forward.ios_head+(256-ippm_forward.N_valid))%256)
	{
		if((ippm_forward.ios[scan_indx].ipdv)> out_ipdv_f)
		{
			out_ipdv_f=ippm_forward.ios[scan_indx].ipdv;
		}
		if((ippm_reverse.ios[scan_indx].ipdv)> out_ipdv_r)
		{
			out_ipdv_r=ippm_reverse.ios[scan_indx].ipdv;
		}
		out_ipdv_99_9_f	+= ippm_forward.ios[scan_indx].ipdv_99_9;
		out_ipdv_99_9_r	+= ippm_reverse.ios[scan_indx].ipdv_99_9;
		out_thres_prob_f +=	ippm_forward.ios[scan_indx].thres_prob;
		out_thres_prob_r +=	ippm_reverse.ios[scan_indx].thres_prob;
		// Temporary need to rms average over window
		jcnt_f++;
		out_jitter_f += (ippm_forward.ios[scan_indx].jitter)*(ippm_forward.ios[scan_indx].jitter);
		jcnt_r++;
		out_jitter_r += (ippm_reverse.ios[scan_indx].jitter)*(ippm_reverse.ios[scan_indx].jitter);
		scan_indx= (scan_indx+255)%256;	
	}
	// normalize output stats
//	_int_disable();
	ippm_forward.out_ipdv =out_ipdv_f;
	ippm_reverse.out_ipdv =out_ipdv_r;
//	_int_enable();
	out_ipdv_99_9_f=out_ipdv_99_9_f/ippm_forward.N_valid;	
	out_ipdv_99_9_r=out_ipdv_99_9_r/ippm_forward.N_valid;	
//	_int_disable();
	ippm_forward.out_ipdv_99_9 =out_ipdv_99_9_f;
	ippm_reverse.out_ipdv_99_9 =out_ipdv_99_9_r;
//	_int_enable();
	out_thres_prob_f=out_thres_prob_f/ippm_forward.N_valid;	
	out_thres_prob_r=out_thres_prob_r/ippm_forward.N_valid;	
//	_int_disable();
	ippm_forward.out_thres_prob =out_thres_prob_f;
	ippm_reverse.out_thres_prob =out_thres_prob_r;
//	_int_enable();
	if(jcnt_f)
	{
		out_jitter_f=out_jitter_f/jcnt_f;
		out_jitter_f=sqrt(out_jitter_f);
	}	
	if(jcnt_r)
	{
		out_jitter_r=out_jitter_r/jcnt_r;
		out_jitter_r=sqrt(out_jitter_r);
	}
//	_int_disable();
	ippm_forward.out_jitter=out_jitter_f;
	ippm_reverse.out_jitter=out_jitter_r;
//	_int_enable();
	#if 0
	debug_printf(GEORGE_PTP_PRT,"N_valid: %d\n", ippm_forward.N_valid);
	debug_printf(GEORGE_PTP_PRT,"N-IPDV forward: %le, reverse %le\n",ippm_forward.out_ipdv,ippm_reverse.out_ipdv);
	debug_printf(GEORGE_PTP_PRT,"N_IPDV_99.9 forward: %le, reverse %le\n",ippm_forward.out_ipdv_99_9,ippm_reverse.out_ipdv_99_9);
	debug_printf(GEORGE_PTP_PRT,"N-jitter forward: %le, reverse %le\n",ippm_forward.out_jitter,ippm_reverse.out_jitter);
	debug_printf(GEORGE_PTP_PRT,"N-thres prob forward: %le, reverse %le\n",ippm_forward.out_thres_prob,ippm_reverse.out_thres_prob);
	#endif
	
	ippm_forward.ios_head= (ippm_forward.ios_head + 1)&0x0FF;
	ippm_reverse.ios_head= (ippm_reverse.ios_head + 1)&0x0FF;
}

///////////////////////////////////////////////////////////////////////
// This function initializes ippm data
///////////////////////////////////////////////////////////////////////
void init_ippm(void)
{
	int i;
	ippm_forward.cur_jitter_1sec=ZEROF;
	ippm_reverse.cur_jitter_1sec=ZEROF;
	ippm_forward.cur_ipdv_1sec=ZEROF;
	ippm_reverse.cur_ipdv_1sec=ZEROF;
	ippm_forward.cur_ipdv_99_9_1sec=ZEROF;
	ippm_reverse.cur_ipdv_99_9_1sec=ZEROF;
	ippm_forward.cur_thres_prob_1sec=ZEROF;
	ippm_reverse.cur_thres_prob_1sec=ZEROF;
	ippm_forward.cur_jitter_1min=ZEROF;
	ippm_reverse.cur_jitter_1min=ZEROF;
	ippm_forward.cur_ipdv_1min=ZEROF;
	ippm_reverse.cur_ipdv_1min=ZEROF;
	ippm_forward.cur_ipdv_99_9_1min=ZEROF;
	ippm_reverse.cur_ipdv_99_9_1min=ZEROF;
	ippm_forward.cur_thres_prob_1min=ZEROF;
	ippm_reverse.cur_thres_prob_1min=ZEROF;
	ippm_forward.out_jitter=ZEROF;
	ippm_reverse.out_jitter=ZEROF;
	ippm_forward.out_ipdv=ZEROF;
	ippm_reverse.out_ipdv=ZEROF;
	ippm_forward.out_ipdv_99_9=ZEROF;
	ippm_reverse.out_ipdv_99_9=ZEROF;
	ippm_forward.out_thres_prob=ZEROF;
	ippm_reverse.out_thres_prob=ZEROF;
	
	ippm_forward.ies_head=0;
	ippm_reverse.ies_head=0;
	ippm_forward.pacing=k_IPDV_DEFAULT_PACING;
	ippm_reverse.pacing=k_IPDV_DEFAULT_PACING;
//	ippm_forward.thres=100000L;
//	ippm_reverse.thres=100000L;
	ippm_forward.thres=k_IPDV_DEFAULT_THRES;
	ippm_reverse.thres=k_IPDV_DEFAULT_THRES;
	ippm_forward.slew_gain=1000L;
	ippm_reverse.slew_gain=1000L;
	ippm_forward.slew_gap=10L;
	ippm_reverse.slew_gap=10L;
	
	ippm_forward.window=k_IPDV_DEFAULT_INTV;
	ippm_reverse.window=k_IPDV_DEFAULT_INTV;
	ippm_forward.N_valid=0;
	ippm_reverse.N_valid=0;
	
	// init onesecond queue
	for(i=0;i<256;i++)
	{
		ippm_forward.ies[i].ipdv_max= 0;
		ippm_forward.ies[i].under_thres_cnt= 0;
		ippm_forward.ies[i].total_thres_cnt= 0;
		ippm_forward.ies[i].over_thres_99_9_cnt= 0;
		ippm_forward.ies[i].thres_99_9= 50000L;
		
		ippm_forward.ies[i].jitter_sum= 0;
		ippm_forward.ies[i].jitter_samp= 0;
		ippm_forward.ies[i].jitter_cnt= 0;
		
		ippm_reverse.ies[i].ipdv_max= 0;
		ippm_reverse.ies[i].under_thres_cnt= 0;
		ippm_reverse.ies[i].total_thres_cnt= 0;
		ippm_reverse.ies[i].over_thres_99_9_cnt= 0;
		ippm_reverse.ies[i].thres_99_9= 50000L;
		ippm_reverse.ies[i].jitter_sum= 0;
		ippm_reverse.ies[i].jitter_samp= 0;
		ippm_reverse.ies[i].jitter_cnt= 0;
	}
	for(i=0;i<256;i++)
	{
		ippm_forward.ios[i].jitter= ZEROF;
		ippm_forward.ios[i].ipdv= ZEROF;
		ippm_forward.ios[i].ipdv_99_9= ZEROF;
		ippm_forward.ios[i].thres_prob= ZEROF;
		ippm_reverse.ios[i].jitter= ZEROF;
		ippm_reverse.ios[i].ipdv= ZEROF;
		ippm_reverse.ios[i].ipdv_99_9= ZEROF;
		ippm_reverse.ios[i].thres_prob= ZEROF;
		
	}

}
void set_ippm_pacing(INT16 lpacing)
{
	INT16 good,i;
	INT16 pacing, window;
	INT32 thres,thres_99_9f,thres_99_9r;
	pacing= ippm_forward.pacing;
	window= ippm_forward.window;
	thres = ippm_forward.thres;

	thres_99_9f=ippm_forward.ies[0].thres_99_9;
	thres_99_9r=ippm_reverse.ies[0].thres_99_9;

	good=0;
	if(lpacing==1) good=1;
	else if(lpacing==2) good=1;
	else if(lpacing==4) good=1;
	else if(lpacing==8) good=1;
	else if(lpacing==16) good=1;
	else if(lpacing==32) good=1;
	else if(lpacing==64) good=1;
	else if(lpacing==128) good=1;
	else if(lpacing==256) good=1;
	else if(lpacing==512) good=1;
	else return;		
	init_ippm();
	ippm_forward.pacing= lpacing;
	ippm_forward.window= window;
	ippm_forward.thres=  thres;
	ippm_reverse.pacing= lpacing;
	ippm_reverse.window= window;
	ippm_reverse.thres=  thres;
	for(i=0;i<256;i++)
	{
	ippm_forward.ies[i].thres_99_9=thres_99_9f;
	ippm_reverse.ies[i].thres_99_9=thres_99_9r;
	}

}
void set_ippm_window(INT16 lwindow)
{
	INT16 good,i;
	INT16 pacing, window;
	INT32 thres,thres_99_9f,thres_99_9r;
	pacing= ippm_forward.pacing;
	window= ippm_forward.window;
	thres = ippm_forward.thres;

	thres_99_9f=ippm_forward.ies[0].thres_99_9;
	thres_99_9r=ippm_reverse.ies[0].thres_99_9;
	
	good=0;
	if((lwindow<256) && (lwindow>0)) good=1;
	else return;
	init_ippm();
	ippm_forward.pacing= pacing;
	ippm_forward.window= lwindow;
	ippm_forward.thres=  thres;
	ippm_reverse.pacing= pacing;
	ippm_reverse.window= lwindow;
	ippm_reverse.thres=  thres;
	for(i=0;i<256;i++)
	{
		ippm_forward.ies[i].thres_99_9=thres_99_9f;
		ippm_reverse.ies[i].thres_99_9=thres_99_9r;
	}	
}
void set_ippm_thres(INT32 lthres)// units of ns
{
	INT16 good,i;
	INT16 pacing, window;
	INT32 thres,thres_99_9f,thres_99_9r;
	pacing= ippm_forward.pacing;
	window= ippm_forward.window;
	thres = ippm_forward.thres;
	thres_99_9f=ippm_forward.ies[0].thres_99_9;
	thres_99_9r=ippm_reverse.ies[0].thres_99_9;

	good=0;
	if((lthres<1000000000L) && (lthres>0)) good=1;
	else return;
	init_ippm();
	ippm_forward.pacing= pacing;
	ippm_forward.window= window;
	ippm_forward.thres=  lthres;
	ippm_reverse.pacing= pacing;
	ippm_reverse.window= window;
	ippm_reverse.thres=  lthres;
	for(i=0;i<256;i++)
	{
		ippm_forward.ies[i].thres_99_9=thres_99_9f;
		ippm_reverse.ies[i].thres_99_9=thres_99_9r;
	}	
}

void reset_ippm(void)
{
   t_ptpIpdvConfigType s_ptpIpdvConfig;

//	INT16 pacing, window;
//	INT32 thres;
   SC_ClkIpdvConfig(&s_ptpIpdvConfig, e_PARAM_GET);

	init_ippm();
	ippm_forward.pacing = ippm_reverse.pacing = s_ptpIpdvConfig.w_pacingFactor;
	ippm_forward.window = ippm_reverse.window = s_ptpIpdvConfig.w_ipdvObservIntv;
	ippm_forward.thres =  ippm_reverse.thres  = s_ptpIpdvConfig.l_ipdvThres;
}

double get_ippm_f_jitter(void) 
{
	if(FLL_Elapse_Sec_Count < 600)
	{
		return ZEROF;
	}
	else
	{
		return ippm_forward.out_jitter;
	}
}

double get_ippm_r_jitter(void)
{
	if(FLL_Elapse_Sec_Count < 600)
	{
		return ZEROF;
	}
	else
	{
		return ippm_reverse.out_jitter;
	}
}
double get_ippm_f_ipdv(void)
{
	if(FLL_Elapse_Sec_Count < 600)
	{
		return ZEROF;
	}
	else
	{
		return ippm_forward.out_ipdv;
	}
}
double get_ippm_r_ipdv(void)
{
	if(FLL_Elapse_Sec_Count < 600)
	{
		return ZEROF;
	}
	else
	{
		return ippm_reverse.out_ipdv;
	}
}
double get_ippm_f_ipdv_99_9(void)
{
	if(FLL_Elapse_Sec_Count < 600)
	{
		return ZEROF;
	}
	else
	{
		return ippm_forward.out_ipdv_99_9;
	}
}
double get_ippm_r_ipdv_99_9(void) 
{
	if(FLL_Elapse_Sec_Count < 600)
	{
		return ZEROF;
	}
	else
	{
		return ippm_reverse.out_ipdv_99_9;
	}
}
double get_ippm_f_thres_prob(void)
{
	if(FLL_Elapse_Sec_Count < 600)
	{
		return ZEROF;
	}
	else
	{
		return ippm_forward.out_thres_prob;
	}
}
double get_ippm_r_thres_prob(void)
{
	if(FLL_Elapse_Sec_Count < 600)
	{
		return ZEROF;
	}
	else
	{
		return ippm_reverse.out_thres_prob;
	}
}






#else // stubs for IPPM functions
void set_ippm_pacing(INT16 lpacing)
{

}
void set_ippm_window(INT16 lwindow)
{
	
}
void set_ippm_thres(INT32 lthres)// units of ns
{
	
}
void reset_ippm(void)
{

}
double get_ippm_f_jitter(void) 
{
	return 0.0;
}

double get_ippm_r_jitter(void)
{
	return 0.0;

}
double get_ippm_f_ipdv(void)
{
	return 0.0;
}
double get_ippm_r_ipdv(void)
{
	return 0.0;
}
double get_ippm_f_ipdv_99_9(void)
{
	return 0.0;
}
double get_ippm_r_ipdv_99_9(void) 
{
	return 0.0;
}
double get_ippm_f_thres_prob(void)
{
	return 0.0;
}
double get_ippm_r_thres_prob(void)
{
	return 0.0;
}




#endif

///////////////////////////////////////////////////////////////////////
// This function calculates ztie 15 minutes stats (call once per minute 
///////////////////////////////////////////////////////////////////////
void ztie_calc(void)
{
	INT8 i;
	double tmp_peak_for, tmp_peak_rev;
#if 0
typedef struct 
{
	double in_f,in_r;
	double cur_avg_f;  //note update current before calling calculator
    double cur_avg_r;
	double prev_avg_f;  //note update current before calling calculator
    double prev_avg_r;
    double ztie_est;
 }
 ZTIE_ELEMENT_STRUCT;

typedef struct 
{
	int_8 head;
	double ztie_f, ztie_r;
	double mafie_f, mafie_r;
	ZTIE_ELEMENT_STRUCT ztie_element[15];
} ZTIE_STRUCT;
#endif
	// update current 1 minute floor estimates
	ztie_stats.ztie_element[ztie_stats.head].in_f=(double)(forward_stats.tdev_element[TDEV_CAT].current);
	ztie_stats.ztie_element[ztie_stats.head].in_r=(double)(reverse_stats.tdev_element[TDEV_CAT].current);
	// update 15 minute averages
	ztie_stats.ztie_element[ztie_stats.head].prev_avg_f=ztie_stats.ztie_element[ztie_stats.head].cur_avg_f;
	ztie_stats.ztie_element[ztie_stats.head].prev_avg_r=ztie_stats.ztie_element[ztie_stats.head].cur_avg_r;
	ztie_stats.ztie_element[ztie_stats.head].cur_avg_f=0;
	ztie_stats.ztie_element[ztie_stats.head].cur_avg_r=0;
	for(i=0;i<15;i++)
	{
		ztie_stats.ztie_element[ztie_stats.head].cur_avg_f+=ztie_stats.ztie_element[(ztie_stats.head+15-i)%15].in_f;
		ztie_stats.ztie_element[ztie_stats.head].cur_avg_r+=ztie_stats.ztie_element[(ztie_stats.head+15-i)%15].in_r;
	}
	ztie_stats.ztie_element[ztie_stats.head].cur_avg_f=ztie_stats.ztie_element[ztie_stats.head].cur_avg_f/15.0;
	ztie_stats.ztie_element[ztie_stats.head].cur_avg_r=ztie_stats.ztie_element[ztie_stats.head].cur_avg_r/15.0;
	// update ztie_est
	ztie_stats.ztie_element[ztie_stats.head].ztie_est_f=
	ztie_stats.ztie_element[ztie_stats.head].cur_avg_f-ztie_stats.ztie_element[ztie_stats.head].prev_avg_f;
	if(ztie_stats.ztie_element[ztie_stats.head].ztie_est_f<ZEROF) 
	{
		ztie_stats.ztie_element[ztie_stats.head].ztie_est_f=-ztie_stats.ztie_element[ztie_stats.head].ztie_est_f;
	}
	ztie_stats.ztie_element[ztie_stats.head].ztie_est_r=
	ztie_stats.ztie_element[ztie_stats.head].cur_avg_r-ztie_stats.ztie_element[ztie_stats.head].prev_avg_r;
	if(ztie_stats.ztie_element[ztie_stats.head].ztie_est_r<ZEROF) 
	{
		ztie_stats.ztie_element[ztie_stats.head].ztie_est_r=-ztie_stats.ztie_element[ztie_stats.head].ztie_est_r;
	}
	
	// update summary stats
	tmp_peak_for=ZEROF;
	tmp_peak_rev=ZEROF;
	for(i=0;i<15;i++)
	{
		if(ztie_stats.ztie_element[i].ztie_est_f > tmp_peak_for)
		{
			tmp_peak_for= ztie_stats.ztie_element[i].ztie_est_f ;		
		}
		if(ztie_stats.ztie_element[i].ztie_est_r > tmp_peak_rev)
		{
			tmp_peak_rev= ztie_stats.ztie_element[i].ztie_est_r ;		
		}
		
	}	
	ztie_stats.ztie_f=tmp_peak_for;
	ztie_stats.ztie_r=tmp_peak_rev;
	ztie_stats.mafie_f=ztie_stats.ztie_f/900.0;
	ztie_stats.mafie_r=ztie_stats.ztie_r/900.0;
#if 0	
	// print debug stats
		debug_printf(GEORGE_PTP_PRT, "ztie_calc_f: %d, in %le, cavg: %le, pavg: %le, zest: %le  \n",
	        FLL_Elapse_Sec_Count,
			ztie_stats.ztie_element[ztie_stats.head].in_f,
			ztie_stats.ztie_element[ztie_stats.head].cur_avg_f,
			ztie_stats.ztie_element[ztie_stats.head].prev_avg_f,
			ztie_stats.ztie_element[ztie_stats.head].ztie_est_f
	        	);
		debug_printf(GEORGE_PTP_PRT, "ztie_sum_f: %d, head %d, ztie %le, mafe %le\n",
	        FLL_Elapse_Sec_Count,
			ztie_stats.head,
			ztie_stats.ztie_f,
			ztie_stats.mafie_f
	        	);
#endif	        	
	// update head
	ztie_stats.head++;
	if(ztie_stats.head>14) ztie_stats.head=0;
}

///////////////////////////////////////////////////////////////////////
// This function initializes ZTIE data
///////////////////////////////////////////////////////////////////////
void init_ztie(void)
{
	int i;
	ztie_stats.head=0;
	ztie_stats.ztie_f=ZEROF;
	ztie_stats.mafie_f=ZEROF;
	ztie_stats.ztie_r=ZEROF;
	ztie_stats.mafie_r=ZEROF;
	for(i=0;i<15;i++)
	{
			ztie_stats.ztie_element[i].in_f=ZEROF;
			ztie_stats.ztie_element[i].cur_avg_f=ZEROF;
			ztie_stats.ztie_element[i].prev_avg_f=ZEROF;
			ztie_stats.ztie_element[i].ztie_est_f=ZEROF;
			ztie_stats.ztie_element[i].in_r=ZEROF;
			ztie_stats.ztie_element[i].cur_avg_r=ZEROF;
			ztie_stats.ztie_element[i].prev_avg_r=ZEROF;
			ztie_stats.ztie_element[i].ztie_est_r=ZEROF;
	}

}


///////////////////////////////////////////////////////////////////////
// This function calculates percentile tdev equivalent stability 
///////////////////////////////////////////////////////////////////////
void tdev_calc(TDEV_STRUCT * tdevp)
{
	int i,tau;
	INT32 lfilt, lfilt_thres;
	tau=16;
	lfilt_thres= 2 * step_thres_oper;
	for(i=0;i<TDEV_WINDOWS;i++) // for each TAU 2,4,8,16 seconds 
	{
		//update window
		if(tdevp->update_phase%(tau)==0)
		{
			// prime buffer
			if(FLL_Elapse_Sec_PTP_Count<500)
			{
				tdevp->tdev_element[i].dlag = tdevp->tdev_element[i].lag;
				tdevp->tdev_element[i].lag = tdevp->tdev_element[i].current;
//				debug_printf(GEORGE_PTP_PRT,"tdev calc prime buffer\n");
			}			
			else  // ensure buffer fill
			{
				lfilt=(tdevp->tdev_element[i].current -2.0*tdevp->tdev_element[i].lag +	
											 tdevp->tdev_element[i].dlag);
				if(lfilt>(lfilt_thres))
				{
					 lfilt=(lfilt_thres);
				}
				else if(lfilt<-(lfilt_thres))
				{
					 lfilt=-(lfilt_thres);
				}	 
				tdevp->tdev_element[i].filt=((double)(lfilt)*(double)(lfilt))/6.0;
				// GPZ NOV 2010 Prevent zero noise case
				if(tdevp->tdev_element[i].filt < 10.0) tdevp->tdev_element[i].filt=10.0;
				if(i==TDEV_CAT)
				{
				#if (N0_1SEC_PRINT==0)		
				 debug_printf(GEORGE_PTP_PRT,"MinTDEV = %8ld power: %8ld cur: %8ld lag: %8ld dlag: %8ld\n",
				 lfilt,
				 (long int)(tdevp->tdev_element[TDEV_CAT].filt),
				 (tdevp->tdev_element[TDEV_CAT].current),
				 (tdevp->tdev_element[TDEV_CAT].lag),
				 (tdevp->tdev_element[TDEV_CAT].dlag));
				 #endif
						 
				} 	


 //           debug_printf(GEORGE_PTP_PRT,"+++before filter-- var_est= %le filt= %le i= %d\n",
 //                 tdevp->tdev_element[i].var_est, tdevp->tdev_element[i].filt, (int)i);

				tdevp->tdev_element[i].var_est= PER_05*tdevp->tdev_element[i].filt + PER_95*(tdevp->tdev_element[i].var_est);

 //           if(tdevp->tdev_element[i].var_est < -1e30)
 //           {
 //              debug_printf(GEORGE_PTP_PRT,"+++glitch-- var_est= %le filt= %le i= %d\n",
 //                 tdevp->tdev_element[i].var_est, tdevp->tdev_element[i].filt, (int)i);
 //           }
 //           else
 //           {
 //              debug_printf(GEORGE_PTP_PRT,"+++normal-- var_est= %le filt= %le i= %d\n",
 //                tdevp->tdev_element[i].var_est, tdevp->tdev_element[i].filt, (int)i);
 //           }

  				tdevp->tdev_element[i].dlag = tdevp->tdev_element[i].lag;
				tdevp->tdev_element[i].lag = tdevp->tdev_element[i].current;
#if 0				
				if(i==3)//extended cluster stats
				{
					for(j=0;j<3;j++)
					{
						lfilt=(tdevp->tdev_1024[j].current -2.0*tdevp->tdev_1024[j].lag +	
												 tdevp->tdev_1024[j].dlag);
						if(lfilt>lfilt_thres) lfilt=0;
						else if(lfilt<-lfilt_thres) lfilt=0;
						tdevp->tdev_1024[j].filt=((double)(lfilt)*(double)(lfilt))/6.0;
						tdevp->tdev_1024[j].var_est= PER_05*tdevp->tdev_1024[j].filt + PER_95*(tdevp->tdev_1024[j].var_est);
						tdevp->tdev_1024[j].dlag = tdevp->tdev_1024[j].lag;
						tdevp->tdev_1024[j].lag = tdevp->tdev_1024[j].current;
					}
				}
#endif				
			}		
		}
		tau/=2;	
	}
	tdevp->update_phase++;
//	tdev_print();	
}
///////////////////////////////////////////////////////////////////////
// This function initializes TDEV data
///////////////////////////////////////////////////////////////////////
void init_tdev(void)
{

	forward_stats.tdev_element[TDEV_CAT].var_est= 0.25*(double)(MIN_DELTA_POP)*(double)(MIN_DELTA_POP);
//	forward_stats.tdev_element[3].var_est= 0.25*(double)(MIN_DELTA_POP)*(double)(MIN_DELTA_POP);
//	forward_stats.tdev_element[2].var_est= 0.25*(double)(MIN_DELTA_POP)*(double)(MIN_DELTA_POP);
//	forward_stats.tdev_element[1].var_est= 0.25*(double)(MIN_DELTA_POP)*(double)(MIN_DELTA_POP);
	
	reverse_stats.tdev_element[TDEV_CAT].var_est= 0.25*(double)(MIN_DELTA_POP)*(double)(MIN_DELTA_POP);
//	reverse_stats.tdev_element[3].var_est= 0.25*(double)(MIN_DELTA_POP)*(double)(MIN_DELTA_POP);
//	reverse_stats.tdev_element[2].var_est= 0.25*(double)(MIN_DELTA_POP)*(double)(MIN_DELTA_POP);
//	reverse_stats.tdev_element[1].var_est= 0.25*(double)(MIN_DELTA_POP)*(double)(MIN_DELTA_POP);
	
}

// **************************************LEGACY CODE ONLY BELOW THIS POINT ****************
// 
//
// ****************************************************************************************
#if 0
///////////////////////////////////////////////////////////////////////
// This function sets fit calculator to fixed fstart point
///////////////////////////////////////////////////////////////////////
static void start_fit(double in)
{
	INT8 i;
	double xinit,ytemp;
	LSF.N=15;
	LSF.Nmax=15;
	ytemp=in*in;
	LSF.bf=in;
	LSF.mf=ZEROF;
	LSF.fitf=in;
	LSF.sumxf=120.0;
	LSF.sumxsqf=1240.0;
	LSF.sumysqf=15.0*ytemp;
	LSF.sumxyf=120.0*in;
	LSF.sumyf=15.0*in;
	LSF.br=in;
	LSF.mr=ZEROF;
	LSF.fitr=in;
	LSF.sumxr=120.0;
	LSF.sumxsqr=1240.0;
	LSF.sumysqr=15.0*ytemp;
	LSF.sumxyr=120.0*in;
	LSF.sumyr=15.0*in;
	LSF.index=15;
	debug_printf(GEORGE_PTP_PRT, "start_fit:\n");
	//TEST OPEN UP SLEW LIMIT
//	LSF.slew_limit= PER_01; //.01ppb per minute placeholder way too fast try PER_05
//	LSF.slew_limit= HUNDREDF; //.01ppb per minute placeholder way too fast try PER_05
	LSF.slew_limit_f= HUNDREDF; //.1ppb per minute placeholder way too fast try PER_05
	LSF.slew_limit_r= HUNDREDF; //.1ppb per minute placeholder way too fast try PER_05
	LSF.slew_cnt_f=0;
	LSF.slew_cnt_r=0;
	
	
	for(i=1;i<16;i++)
	{	
		xinit=(double)i;
		LSF.index=i;	
		LSF.lsfe_f[LSF.index].x  = xinit;
		LSF.lsfe_f[LSF.index].xsq= xinit*xinit;
		LSF.lsfe_f[LSF.index].ysq= ytemp;
		LSF.lsfe_f[LSF.index].xy = xinit*in;
		LSF.lsfe_f[LSF.index].y  = in;
		LSF.lsfe_r[LSF.index].x  = xinit;
		LSF.lsfe_r[LSF.index].xsq= xinit*xinit;
		LSF.lsfe_r[LSF.index].ysq= ytemp;
		LSF.lsfe_r[LSF.index].xy = xinit*in;
		LSF.lsfe_r[LSF.index].y  = in;
		
		LSF.lsfe_f[LSF.index].b  = in;
		LSF.lsfe_f[LSF.index].m  = ZEROF;
		LSF.lsfe_r[LSF.index].b  = in;
		LSF.lsfe_r[LSF.index].m  = ZEROF;
		
	}	
}
#endif
#if 0
///////////////////////////////////////////////////////////////////////
// This function sets fit calculator to fixed fstart point
///////////////////////////////////////////////////////////////////////
static void start_fit_med(double in)
{
	INT16 i;
	double xinit,ytemp;
	LSF.N=60;
	LSF.Nmax=60;
	ytemp=in*in;
	LSF.bf=in;
	LSF.mf=ZEROF;
	LSF.fitf=in;
	LSF.sumxf=1830.0;
	LSF.sumxsqf=73810.0;
	LSF.sumysqf=60.0*ytemp;
	LSF.sumxyf=1830.0*in;
	LSF.sumyf=60.0*in;
	LSF.br=in;
	LSF.mr=ZEROF;
	LSF.fitr=in;
	LSF.sumxr=1830.0;
	LSF.sumxsqr=73810.0;
	LSF.sumysqr=60.0*ytemp;
	LSF.sumxyr=1830.0*in;
	LSF.sumyr=60.0*in;
	LSF.index=60;
	debug_printf(GEORGE_PTP_PRT, "start_fit_med:\n");
	
	//TEST OPEN UP SLEW LIMIT
//	LSF.slew_limit= PER_01; //.01ppb per minute placeholder way too fast try PER_05
//	LSF.slew_limit= HUNDREDF; //.01ppb per minute placeholder way too fast try PER_05
	LSF.slew_limit_f= 200.0; //.1ppb per minute placeholder way too fast try PER_05
	LSF.slew_limit_r= 200.0; //.1ppb per minute placeholder way too fast try PER_05
	LSF.slew_cnt_f=0;
	LSF.slew_cnt_r=0;
	
	
	for(i=1;i<61;i++)
	{	
		xinit=(double)i;
		LSF.index=i;	
		LSF.lsfe_f[LSF.index].x  = xinit;
		LSF.lsfe_f[LSF.index].xsq= xinit*xinit;
		LSF.lsfe_f[LSF.index].ysq= ytemp;
		LSF.lsfe_f[LSF.index].xy = xinit*in;
		LSF.lsfe_f[LSF.index].y  = in;
		LSF.lsfe_r[LSF.index].x  = xinit;
		LSF.lsfe_r[LSF.index].xsq= xinit*xinit;
		LSF.lsfe_r[LSF.index].ysq= ytemp;
		LSF.lsfe_r[LSF.index].xy = xinit*in;
		LSF.lsfe_r[LSF.index].y  = in;
		
		LSF.lsfe_f[LSF.index].b  = in;
		LSF.lsfe_f[LSF.index].m  = ZEROF;
		LSF.lsfe_r[LSF.index].b  = in;
		LSF.lsfe_r[LSF.index].m  = ZEROF;
		
	}	
}
#endif
#if 0
///////////////////////////////////////////////////////////////////////
// This function is called once per minute to update fit estimates
// for improved stability for stable OCXO application
///////////////////////////////////////////////////////////////////////
void update_fit(void)
{
	INT16 old_index,i,j,fitcount;
	double slewf, slewr;
	double xtempf,ytempf;
	double xtempr,ytempr;
	double denom,dtemp_lsf;
	// increment modulo index
	old_index = LSF.index;
	LSF.index = (LSF.index+1)%lsf_window;
	if(LSF.N<lsf_window)
	{
		LSF.N++;
	}
	if(LSF.Nmax<lsf_window)
	{
	
		LSF.Nmax++;
	}
	
	//if filled remove oldest point 
	if(LSF.N==lsf_window)
	{
		LSF.sumxf   -= LSF.lsfe_f[LSF.index].x;
		LSF.sumxsqf -= LSF.lsfe_f[LSF.index].xsq;
		LSF.sumxyf  -= LSF.lsfe_f[LSF.index].xy;
		LSF.sumyf   -= LSF.lsfe_f[LSF.index].y;
		LSF.sumysqf -= LSF.lsfe_f[LSF.index].ysq;
		
		LSF.sumxr   -= LSF.lsfe_r[LSF.index].x;
		LSF.sumxsqr -= LSF.lsfe_r[LSF.index].xsq;
		LSF.sumxyr  -= LSF.lsfe_r[LSF.index].xy;
		LSF.sumyr   -= LSF.lsfe_r[LSF.index].y;
		LSF.sumysqr -= LSF.lsfe_r[LSF.index].ysq;
	}	
	// add newest point
	if(LSF.N < 8) //skip slew control early on
	{
		ytempf=(fdrift_f);//GPZ May 2009 added shaping in control
	}
	else
	{
		slewf = (fdrift_f) - LSF.lsfe_f[old_index].y;
		if(slewf<-LSF.slew_limit_f)
		{
			ytempf=LSF.lsfe_f[old_index].y-LSF.slew_limit_f;
			debug_printf(GEORGE_PTP_PRT,"exception lsf_f slew: ytempf:%9.1le,old:%9.1le,limit:%9.1le\n",ytempf,LSF.lsfe_f[old_index].y,LSF.slew_limit_f);
			LSF.slew_cnt_f++;
			
		}
		else if(slewf>LSF.slew_limit_f)
		{
			ytempf=LSF.lsfe_f[old_index].y+LSF.slew_limit_f;
			debug_printf(GEORGE_PTP_PRT,"exception lsf_f slew: ytempf:%9.1le,old:%9.1le,limit:%9.1le\n",ytempf,LSF.lsfe_f[old_index].y,LSF.slew_limit_f);
			LSF.slew_cnt_f++;
		}
		else
		{
			ytempf=(fdrift_f);
		}
	}

	if(OCXO_Type()==e_RB)	{
		if(ytempf > 15.0) ytempf=15.0;
		else if (ytempf<-15.0) ytempf=-15.0;
	}

	xtempf= LSF.lsfe_f[old_index].x +1.0;
	LSF.lsfe_f[LSF.index].x  = xtempf;
	LSF.lsfe_f[LSF.index].xsq= xtempf*xtempf;
	LSF.lsfe_f[LSF.index].ysq= ytempf*ytempf;
	LSF.lsfe_f[LSF.index].xy = xtempf*ytempf;
	LSF.lsfe_f[LSF.index].y  = ytempf;
	if(LSF.N < 8) //skip slew control early on
	{
		ytempr=(fdrift_r);
	}
	else
	{
		slewr = (fdrift_r) - LSF.lsfe_r[old_index].y;
		if(slewr<-LSF.slew_limit_r)
		{
			ytempr=LSF.lsfe_r[old_index].y-LSF.slew_limit_r;
			debug_printf(GEORGE_PTP_PRT,"exception lsf_r slew: ytempr:%9.1le,old:%9.1le,limit:%9.1le\n",ytempr,LSF.lsfe_r[old_index].y,LSF.slew_limit_r);
			LSF.slew_cnt_r++;
			
		}
		else if(slewr>LSF.slew_limit_r)
		{
			ytempr=LSF.lsfe_r[old_index].y+LSF.slew_limit_r;
			debug_printf(GEORGE_PTP_PRT,"exception lsf_r slew: ytempr:%9.1le,old:%9.1le,limit:%9.1le\n",ytempr,LSF.lsfe_r[old_index].y,LSF.slew_limit_r);
			LSF.slew_cnt_r++;
		}
		else
		{
			ytempr=(fdrift_r);
		}
	}
	xtempr= LSF.lsfe_r[old_index].x +1.0;
#if 0	
	// update y moving average	
	LSF.lsfe_r[LSF.index].y_ma=ytempr;
	for(i=1;i<LSF.Nmax;i++)
	{
		LSF.lsfe_r[LSF.index].y_ma +=LSF.lsfe_r[(LSF.index+256-i)%256].y;
	}	
	if(LSF.Nmax>0)
	{
		LSF.lsfe_r[LSF.index].y_ma= LSF.lsfe_r[LSF.index].y_ma/LSF.Nmax;
	}
#endif	
	LSF.lsfe_r[LSF.index].x  = xtempr;
	LSF.lsfe_r[LSF.index].xsq= xtempr*xtempr;
	LSF.lsfe_r[LSF.index].ysq= ytempr*ytempr;
	LSF.lsfe_r[LSF.index].xy = xtempr*ytempr;
	LSF.lsfe_r[LSF.index].y  = ytempr;
	//update sums	
	LSF.sumxf   += LSF.lsfe_f[LSF.index].x;
	LSF.sumxsqf += LSF.lsfe_f[LSF.index].xsq;
	LSF.sumysqf += LSF.lsfe_f[LSF.index].ysq;
	LSF.sumxyf  += LSF.lsfe_f[LSF.index].xy;
	LSF.sumyf   += LSF.lsfe_f[LSF.index].y;
	LSF.sumyr   += LSF.lsfe_r[LSF.index].y;
	LSF.sumxr   += LSF.lsfe_r[LSF.index].x;
	LSF.sumxsqr += LSF.lsfe_r[LSF.index].xsq;
	LSF.sumysqr += LSF.lsfe_r[LSF.index].ysq;
	LSF.sumxyr  += LSF.lsfe_r[LSF.index].xy;
	// calculate slope intercept estimates
	denom=(double)(LSF.N)*(LSF.sumxsqf) - LSF.sumxf*LSF.sumxf;
	if(denom != ZEROF)
	{
		LSF.bf = (LSF.sumxsqf * LSF.sumyf) - (LSF.sumxf * LSF.sumxyf);
		LSF.bf = LSF.bf/denom;
		LSF.lsfe_f[LSF.index].b=LSF.bf;
	
		LSF.mf = ((double)(LSF.N) * LSF.sumxyf) - (LSF.sumxf * LSF.sumyf);
		LSF.mf = LSF.mf/denom;
		LSF.lsfe_f[LSF.index].m=LSF.mf;
		
//		LSF.fitf= LSF.mf * xtempf + LSF.bf;
		dtemp_lsf=ZEROF;
		fitcount=0;
		for(j=2;j<LSF.N;j++) //skip a few to stay in window
		{
			dtemp_lsf += LSF.lsfe_f[LSF.index].m * (xtempf-LSF.N+1) + LSF.lsfe_f[LSF.index].b; //GPZ APRIL 2010 use effective intercept
			fitcount++;
		}
		LSF.fitf= dtemp_lsf/fitcount;
		if(LSF.N>4)
		{
			LSF.varf= (LSF.sumysqf - LSF.sumyf*LSF.sumyf/LSF.N)-LSF.mf*LSF.mf*denom/LSF.N;
			LSF.varf=LSF.varf/(LSF.N-2);
		}	
		
	}	
	denom=(double)(LSF.N)*(LSF.sumxsqr) - LSF.sumxr*LSF.sumxr;
	if(denom != ZEROF)
	{
		LSF.br = (LSF.sumxsqr * LSF.sumyr) - (LSF.sumxr * LSF.sumxyr);
		LSF.br = LSF.br/denom;
		LSF.lsfe_r[LSF.index].b=LSF.br;
		
		
		LSF.mr = ((double)(LSF.N) * LSF.sumxyr) - (LSF.sumxr * LSF.sumyr);
		LSF.mr = LSF.mr/denom;
		LSF.lsfe_r[LSF.index].m=LSF.mr;
		
//		LSF.fitr= LSF.mr * xtempr + LSF.br;
		dtemp_lsf=ZEROF;
		fitcount=0;
		for(j=2;j<LSF.N;j++)
		{
			dtemp_lsf += LSF.lsfe_r[LSF.index].m* (xtempr-LSF.N+1) + LSF.lsfe_r[LSF.index].b;//GPZ APRIL 2010 use effective intercept
			fitcount++;
		}	
		LSF.fitr= dtemp_lsf/fitcount;
		if(LSF.N>4)
		{
			LSF.varr= (LSF.sumysqr - LSF.sumyr*LSF.sumyr/LSF.N)-LSF.mr*LSF.mr*denom/LSF.N;
			LSF.varr=LSF.varr/(LSF.N-2);
		}
		
	}
	debug_printf(GEORGE_PTP_PRT,"lsf_f:%5d,xtmp:%9.1le,ytmp:%9.1le,indx:%3d,N:%3d,sx:%9.1le,sy:%12.3le,m:%9.1le,b:%12.4le,gf:%9.1le,scnt:%ld\n",
     FLL_Elapse_Sec_Count,
//     fdrift_f,
     xtempf,
     ytempf,
     LSF.index,
     LSF.N,
     LSF.sumxf,
     LSF.sumyf,
     LSF.mf,
     LSF.bf,
     LSF.varr,
 	 LSF.slew_cnt_f

     );
		
	debug_printf(GEORGE_PTP_PRT, "lsf_r:%5d,xtmp:%9.1le,ytmp:%9.1le,indx:%3d,N:%3d,sx:%9.1le,sy:%12.3le,m:%9.1le,b:%12.4le,gf:%9.1le,scnt:%ld\n",
     FLL_Elapse_Sec_Count,
//     fdrift_r,
     xtempr,
     ytempr,
     LSF.index,
     LSF.N,
     LSF.sumxr,
	 LSF.sumyr,
     LSF.mr,
     LSF.br,
     LSF.varr,
 	 LSF.slew_cnt_r
     );
}
#endif
#if 0
///////////////////////////////////////////////////////////////////////
// This function call initializes fit calculator
///////////////////////////////////////////////////////////////////////
static void init_fit(void)
{
	LSF.N=0;
	LSF.Nmax=0;
	LSF.bf=ZEROF;
	LSF.mf=ZEROF;
	LSF.fitf=ZEROF;
	LSF.sumxf=ZEROF;
	LSF.sumxsqf=ZEROF;
	LSF.sumysqf=ZEROF;
	LSF.sumxyf=ZEROF;
	LSF.sumyf=ZEROF;
	LSF.br=ZEROF;
	LSF.mr=ZEROF;
	LSF.fitr=ZEROF;
	LSF.sumxr=ZEROF;
	LSF.sumxsqr=ZEROF;
	LSF.sumysqr=ZEROF;
	LSF.sumxyr=ZEROF;
	LSF.sumyr=ZEROF;
	LSF.index=0;
	//TEST OPEN UP SLEW LIMIT
//	LSF.slew_limit= PER_01; //.1ppb per minute placeholder way too fast try PER_05
	LSF.slew_limit_f= HUNDREDF; //.1ppb per minute placeholder way too fast try PER_05
	LSF.slew_limit_r= HUNDREDF; //.1ppb per minute placeholder way too fast try PER_05
	LSF.slew_cnt_f=0;
	LSF.slew_cnt_r=0;
}
#endif

#if 0
///////////////////////////////////////////////////////////////////////
// This function sets fit calculator to fixed fstart point
///////////////////////////////////////////////////////////////////////
static void start_fit_long(double in)
{
	INT16 i;
	double xinit,ytemp;
	LSF.N=120;
	LSF.Nmax=120;
	ytemp=in*in;
	LSF.bf=in;
	LSF.mf=ZEROF;
	LSF.fitf=in;
	LSF.sumxf=7260.0;
	LSF.sumxsqf=583220.0;
	LSF.sumysqf=120.0*ytemp;
	LSF.sumxyf=7260.0*in;
	LSF.sumyf=120.0*in;
	LSF.br=in;
	LSF.mr=ZEROF;
	LSF.fitr=in;
	LSF.sumxr=7260.0;
	LSF.sumxsqr=583220.0;
	LSF.sumysqr=120.0*ytemp;
	LSF.sumxyr=7260.0*in;
	LSF.sumyr=120.0*in;
	LSF.index=120;
	debug_printf(GEORGE_PTP_PRT, "start_fit_long:\n");
	
	//TEST OPEN UP SLEW LIMIT
//	LSF.slew_limit= PER_01; //.01ppb per minute placeholder way too fast try PER_05
//	LSF.slew_limit= HUNDREDF; //.01ppb per minute placeholder way too fast try PER_05
	LSF.slew_limit_f= HUNDREDF; //.1ppb per minute placeholder way too fast try PER_05
	LSF.slew_limit_r= HUNDREDF; //.1ppb per minute placeholder way too fast try PER_05
	LSF.slew_cnt_f=0;
	LSF.slew_cnt_r=0;
	
	
	for(i=1;i<121;i++)
	{	
		xinit=(double)i;
		LSF.index=i;	
		LSF.lsfe_f[LSF.index].x  = xinit;
		LSF.lsfe_f[LSF.index].xsq= xinit*xinit;
		LSF.lsfe_f[LSF.index].ysq= ytemp;
		LSF.lsfe_f[LSF.index].xy = xinit*in;
		LSF.lsfe_f[LSF.index].y  = in;
		LSF.lsfe_r[LSF.index].x  = xinit;
		LSF.lsfe_r[LSF.index].xsq= xinit*xinit;
		LSF.lsfe_r[LSF.index].ysq= ytemp;
		LSF.lsfe_r[LSF.index].xy = xinit*in;
		LSF.lsfe_r[LSF.index].y  = in;
		
		LSF.lsfe_f[LSF.index].b  = in;
		LSF.lsfe_f[LSF.index].m  = ZEROF;
		LSF.lsfe_r[LSF.index].b  = in;
		LSF.lsfe_r[LSF.index].m  = ZEROF;
		
	}	
}
#endif

//***************************************************************************************
//		 Softclient 2.0 extensions to support new inputs
//
//*****************************************************************************************
#if(NO_PHY==0)

///////////////////////////////////////////////////////////////////////
// This function is called once per robust update period (8s to 2minute) to update robust frequency  estimates
// for GPS or GNSS inputs signals
// 
///////////////////////////////////////////////////////////////////////
static	double gain_large_phy = 0.5; 
static	double gain_small_phy = 0.5;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 	Channelized Form of update_rfe_phy
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if SMARTCLOCK == 1
static int  rfe_phy_settle_count;
static INT32 last_good_turbo_phase_phy[RFE_CHAN_MAX];
#endif
void update_rfe_phy(void)
{
	INT16 chan_indx_rfe,i_phy,j_phy,jmax_phy,print_index_phy;  
	INT16 Fix_Space_phy,Fix_Space_Inc_phy,Fix_Space_List_phy[8];
	double Turbo_Scale_phy;
	double dtemp_phy;
	INT16 inner_range_phy;
	INT16 inner_range_prev_phy;
	INT16 Pair_Valid_Flag_phy;
	INT16 rfe_cindx_phy; 
	INT16 rfe_pindx_phy; 
	INT16 rfe_findx_phy; 
	INT16 rfe_start_phy; 
	INT16 rfe_end_phy; 
	INT16 total_phy; //total pairs used in calculation
	INT32 scale1_phy,scale2_phy,scale3_phy,scale4_phy,scale5_phy;
	double weight_phy; // total weight of calculation
	double bias_asymmetry_factor_phy, instability_factor_phy,comm_bias_phy,diff_bias_phy;
	double fest_phy,delta_fest_phy;//GPZ verify correct data types Jan 2011
	double fest_first_phy,delta_first_thres_phy;
	double weight_first_phy,weight_first_thres_phy;
	INT16 total_first_phy,total_first_thres_phy;
	
	// TODO DATA that needs to be channelized
	INT32   ibias_chan[RFE_CHAN_MAX];
	double weight_chan[RFE_CHAN_MAX]; 
	double   fest_chan[RFE_CHAN_MAX]; 
	rfe_pindx_phy= RFE_PHY.index;
	rfe_cindx_phy= (RFE_PHY.index+1)%RFE_PHY.N;
	rfe_findx_phy= (RFE_PHY.index+2)%RFE_PHY.N;
	RFE_PHY.index=rfe_cindx_phy;
//	debug_printf(GEORGE_PTP_PRT,"rfe_phy: STARTING POINT\n");

	if(rfe_turbo_flag_phy==FALSE)
	{
		Turbo_Scale_phy=1.0;
	}
//	else if(rfe_turbo_flag_phy==1)
//	{
//		Turbo_Scale_phy=4.0;  
//	}
	else 
	{
		Turbo_Scale_phy=16.0;  
	}
	
	if(RFE_PHY.Nmax<RFE_WINDOW_LARGE)
	{
		RFE_PHY.Nmax++;
	}
	total_phy=0;
	weight_phy=ZEROF;
	fest_phy=ZEROF;
	for(i_phy=0;i_phy<RFE_CHAN_MAX;i_phy++)
	{
		weight_chan[i_phy]=ZEROF; 
		fest_chan[i_phy]=ZEROF; 
	}
	if((RFE_PHY.first)&&(RFE_PHY.index==(N_init_rfe_phy-5))) // base line phase once after first cycle
	{
		delta_fest_phy= (RFE_PHY.fest- RFE_PHY.fest_prev);
		if(delta_fest_phy<0.0) delta_fest_phy=-delta_fest_phy;
		debug_printf(GEORGE_PTP_PRT,"rfe_phy: Delta Fest Check: Delta:%le, cur:%le, prev:%le\n",delta_fest_phy,RFE_PHY.fest,RFE_PHY.fest_prev);
		if(delta_fest_phy<25.0) //ensure loop is settled to within 25 ppb
		{
//			clr_residual_phase(); 
//			init_pshape();			
			RFE_PHY.first--; //allow RFE.first to count multiple loops if needed
		}	
	}
	else if((RFE_PHY.first)&&(RFE_PHY.index==(N_init_rfe_phy-1))) // base line phase once after first cycle
	{
		RFE_PHY.fest_prev=RFE_PHY.fest;
	}
	if((OCXO_Type()==e_MINI_OCXO) && (mode_jitter==0))
	{
		Fix_Space_phy=3; // start with 6 minute spacing
		Fix_Space_Inc_phy=6; //was 3
		Fix_Space_List_phy[1]=11;		
		Fix_Space_List_phy[2]=11;		
		Fix_Space_List_phy[3]=11;		
		jmax_phy=4;
	}
	else if((mode_jitter==1))
	{
		Fix_Space_phy=5;
		Fix_Space_Inc_phy=5;
		Fix_Space_List_phy[1]=5;		
		Fix_Space_List_phy[2]=10;		
		Fix_Space_List_phy[3]=15;		
		jmax_phy=4;
	}
	else
	{
		#if (SC_FASTER_SERVO_MODE==1)
		Fix_Space_phy=5; // start with 6 minute spacing was (6)
		Fix_Space_Inc_phy=5; //was 4 try 3 to create sync pattern was (3)
		Fix_Space_List_phy[1]=14;		
		Fix_Space_List_phy[2]=14;		
		Fix_Space_List_phy[3]=14;		
		jmax_phy=4;
		#else
		Fix_Space_phy=5; // start with 6 minute spacing was (6)
		Fix_Space_Inc_phy=5; //was 4 try 3 to create sync pattern was (3)
		Fix_Space_List_phy[1]=14;	//was 26
		Fix_Space_List_phy[2]=14;	//was 27	
		Fix_Space_List_phy[3]=14;	//was 28	
		jmax_phy=4;
		#endif

	}
	debug_printf(GEORGE_PTP_PRT,"rfe_phy: Fix_Space:%5d,%5d,%5d,jmax:%5d,first:%5d\n",
    Fix_Space_List_phy[1],Fix_Space_List_phy[2],Fix_Space_List_phy[3],jmax_phy,RFE_PHY.first);
	for(i_phy=0;i_phy<128;i_phy++) //was 512 
	{
		fpairs_phy[i_phy]=ZEROF;
		wpairs_phy[i_phy]=ZEROF;
	}
	if(RFE_PHY.first)
	{
		fest_first_phy=ZEROF;
		weight_first_phy=ZEROF;
		total_first_phy=ZEROF;
	}		
#if SMARTCLOCK == 1
   if(hcount==0)
   {	
     last_good_turbo_phase_phy[0]=xds.xfer_turbo_phase_phy[0];
     last_good_turbo_phase_phy[1]=xds.xfer_turbo_phase_phy[1];
     last_good_turbo_phase_phy[2]=xds.xfer_turbo_phase_phy[2];
     last_good_turbo_phase_phy[3]=xds.xfer_turbo_phase_phy[3];
     last_good_turbo_phase_phy[4]=xds.xfer_turbo_phase_phy[4];
     last_good_turbo_phase_phy[5]=xds.xfer_turbo_phase_phy[5];
   }
   	debug_printf(GEORGE_PTP_PRT,"rfe_phy: last good phase: %ld, hcount: %ld\n",(long int)last_good_turbo_phase_phy[0],hcount);

#endif
	// update rfe open loop compensation for turbo case
//	if(rfe_turbo_flag_phy==1)
//	{	
//		rfe_turbo_acc_phy+= 32.0;  //32s  4x is turbo update window
//   	debug_printf(GEORGE_PTP_PRT,"rfe_phy: Wrong Turbo Phase\n");
//	}
//	else if	(rfe_turbo_flag_phy==2)	
	{
		rfe_turbo_acc_phy+= 8.0*fdrift_smooth;  //8s  16x is turbo update window
	}
	// update current index information
	for(chan_indx_rfe=0;chan_indx_rfe<RFE_CHAN_MAX;chan_indx_rfe++)
	{
		if(rfe_turbo_flag_phy)
		{
			if( (xds.xfer_turbo_phase_phy[chan_indx_rfe]<(double)MAXMIN) && (low_warning_phy[chan_indx_rfe]==0) )
			{
#if SMARTCLOCK == 1
				RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].phase = last_good_turbo_phase_phy[chan_indx_rfe]+ rfe_turbo_acc_phy;
#else
				RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].phase = xds.xfer_turbo_phase_phy[chan_indx_rfe]+ rfe_turbo_acc_phy;
#endif
				RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].tfs = 1;
			}
			else
			{
				RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].phase=0.0;
				RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].tfs = 0;
				PHY_Floor_Change_Flag[chan_indx_rfe]=1;
			}
			debug_printf(GEORGE_PTP_PRT,"rfe_phy: xfer turbo phase: chan: %d, phase: %le, flag:%ld\n",chan_indx_rfe, xds.xfer_turbo_phase_phy[chan_indx_rfe],RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].tfs);
		
		}
		else
		{
			RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].phase =  xds.xfer_turbo_phase_phy[chan_indx_rfe]+ rfe_turbo_acc_phy; //GPZ place holder always plan to use turbo with GNSS and Sync E
		}	
		ibias_chan[chan_indx_rfe]=(long)(min_oper_thres_phy[chan_indx_rfe]/2500.0);
		ibias_chan[chan_indx_rfe]= ibias_chan[chan_indx_rfe]*2500; 
		RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].bias = (double)(ibias_chan[chan_indx_rfe]);
		if(RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].bias > RFE_PHY.rfee[chan_indx_rfe][rfe_pindx_phy].bias)
		{
			RFE_PHY.rfee[chan_indx_rfe][rfe_pindx_phy].bias = (double)(ibias_chan[chan_indx_rfe]); //GPZ NOV 2010 back annotate higher bias
		}
		RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].noise =  10.0; //GPZ place holder may use Carrier to noise metric for GNSS
		if(RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].noise <10.0)
		{
			RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].noise=10.0; 
		}

      if(low_warning_phy[chan_indx_rfe] == 0)
      {
         if(Phy_Channel_Transient_Free_Cnt[chan_indx_rfe] < 10000)
         Phy_Channel_Transient_Free_Cnt[chan_indx_rfe] ++;
      }
      else
      {
         Phy_Channel_Transient_Free_Cnt[chan_indx_rfe] = 0;
      }

		low_warning_phy[chan_indx_rfe]=0;
		high_warning_phy[chan_indx_rfe]=0;
		if((PHY_Floor_Change_Flag[chan_indx_rfe]) > 0) 
		{
			RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].floor_cat=RFE_PHY.rfee[chan_indx_rfe][rfe_pindx_phy].floor_cat+1;
			PHY_Floor_Change_Flag[chan_indx_rfe]--;
			//GPZ invalid around floor change
			RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].tfs = 0;
			RFE_PHY.rfee[chan_indx_rfe][rfe_pindx_phy].tfs = 0; // GPZ Add protection one update back NOV 12 2010
			RFE_PHY.rfee[chan_indx_rfe][rfe_findx_phy].tfs = 2; // GPZ Add protection one update future NOV 12 2010
		}
		else
		{
			RFE_PHY.rfee[chan_indx_rfe][rfe_cindx_phy].floor_cat=RFE_PHY.rfee[chan_indx_rfe][rfe_pindx_phy].floor_cat;
		}
		
	   debug_printf(GEORGE_PTP_PRT,"rfe_phy: transient free count: chan: %d, count:%ld\n",chan_indx_rfe,(long int) Phy_Channel_Transient_Free_Cnt   [chan_indx_rfe] );
}

//	Outer window spacing control 4,8,16,32 minute tested
	inner_range_prev_phy=0;
	for(j_phy=1;j_phy<jmax_phy;j_phy++)
	{
	Fix_Space_phy=Fix_Space_List_phy[j_phy]; //GPZ OCT 2010 new method to space windows
	inner_range_phy=(RFE_PHY.N-(Fix_Space_phy)); //GPZ OCT 2010 use maximum available signal
//	Inner fixed spacing version of algorithm
	for(i_phy=0;i_phy<inner_range_phy;i_phy++)
	{
		rfe_start_phy = (rfe_cindx_phy +RFE_PHY.N - i_phy)%RFE_PHY.N;  //GPZ NOV 2010 start at current not previous index
		rfe_end_phy =   (rfe_cindx_phy +RFE_PHY.N - i_phy- Fix_Space_phy)%RFE_PHY.N;//GPZ NOV 2010 start at current not previous index
		
		for(chan_indx_rfe=0;chan_indx_rfe<RFE_CHAN_MAX;chan_indx_rfe++)
		{
		
			// update pair valid flag for channel  
			if((RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].tfs==0)||(RFE_PHY.rfee[chan_indx_rfe][rfe_end_phy].tfs==0))
			{
				Pair_Valid_Flag_phy=0;
			}
			else if((RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].floor_cat) != (RFE_PHY.rfee[chan_indx_rfe][rfe_end_phy].floor_cat)) 
			{
				Pair_Valid_Flag_phy=0;
			}
			else if((RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].floor_cat)==0)
			{
				Pair_Valid_Flag_phy=2;
			}
			else
			{
				Pair_Valid_Flag_phy=1;
			}
			// update channel  frequency if valid
			if(Pair_Valid_Flag_phy)
			{
				fpairs_phy[5*i_phy+inner_range_prev_phy+chan_indx_rfe]=(Turbo_Scale_phy)*(RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].phase -RFE_PHY.rfee[chan_indx_rfe][rfe_end_phy].phase)/(128.0*(double)(Fix_Space_phy));
				// calculate bias asymmetry factor and instability factor
				comm_bias_phy=(RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].bias + RFE_PHY.rfee[chan_indx_rfe][rfe_end_phy].bias)/2.0;
				diff_bias_phy=(RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].bias - RFE_PHY.rfee[chan_indx_rfe][rfe_end_phy].bias);
				if(comm_bias_phy<0.0) comm_bias_phy= - comm_bias_phy;
				if(diff_bias_phy<0.0) diff_bias_phy= - diff_bias_phy;
				if(comm_bias_phy<HUNDREDF) comm_bias_phy=HUNDREDF ;
				if(diff_bias_phy<(double)(min_oper_thres_oper)) diff_bias_phy=(double)(min_oper_thres_oper);
			
				bias_asymmetry_factor_phy= ((double)(1000000.0)/(comm_bias_phy*comm_bias_phy*comm_bias_phy))*((double)(min_oper_thres_oper)/(diff_bias_phy));
	
				instability_factor_phy= sqrt(RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].noise + RFE_PHY.rfee[chan_indx_rfe][rfe_end_phy].noise);
				if(instability_factor_phy<HUNDREDF) instability_factor_phy=HUNDREDF; //bound min noise
				wpairs_phy[5*i_phy+inner_range_prev_phy+chan_indx_rfe]=(double)(Fix_Space_phy*Fix_Space_phy)*bias_asymmetry_factor_phy/instability_factor_phy;
            if(chan_indx_rfe == xds.xfer_active_chan_phy)
            {
				   total_phy++;
            }
				if(RFE_PHY.first && Pair_Valid_Flag_phy==1) //non-floor zero calibration
				{
					fest_first_phy+=fpairs_phy[2*i_phy+inner_range_prev_phy];
					weight_first_phy+=wpairs_phy[2*i_phy+inner_range_prev_phy];
               if(chan_indx_rfe == xds.xfer_active_chan_phy)
               {
	   			   total_first_phy++;
               }
				}
//				if(fpairs_phy[5*i_phy+inner_range_prev_phy+chan_indx_rfe]<2.1 || fpairs_phy[5*i_phy+inner_range_prev_phy+chan_indx_rfe]>2.7)
//				{
//						debug_printf(GEORGE_PTP_PRT,"weird fpair generated : chan:%d , start:%ld, tflg:%d, floor:%d, phase:%le\n",
//						chan_indx_rfe,
//						rfe_start_phy,	 
//						RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].tfs,
//						RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].floor_cat,
//						RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].phase
//						);
//						debug_printf(GEORGE_PTP_PRT,"weird fpair generated : chan:%d , end:%ld, tflg:%d, floor:%d, phase:%le\n",
//						chan_indx_rfe,
//						rfe_end_phy,	 
//						RFE_PHY.rfee[chan_indx_rfe][rfe_end_phy].tfs,
//						RFE_PHY.rfee[chan_indx_rfe][rfe_end_phy].floor_cat,
//						RFE_PHY.rfee[chan_indx_rfe][rfe_end_phy].phase
//						);
//						
//					
//				}
				
			}
    	 	else
     		{
	     		fpairs_phy[5*i_phy+inner_range_prev_phy+chan_indx_rfe]=ZEROF;
   				wpairs_phy[5*i_phy+inner_range_prev_phy+chan_indx_rfe]=ZEROF;
   				bias_asymmetry_factor_phy=ZEROF;
   				instability_factor_phy=ZEROF;
     		}
			// channel  debug print
#if 1			
//			if ( (((RFE_PHY.index)%5)==1) && (chan_indx_rfe==0) )//GPZ verify correct data types Jan 2011
			if(1)//GPZ verify correct data types Jan 2011
			{
				print_index_phy=(5*i_phy+inner_range_prev_phy+chan_indx_rfe);
				//Print Header
				if(print_index_phy==0)
				{
					debug_printf(GEORGE_PTP_PRT," indx_phy  strt      end       pvf       sphase         ephase        flr       fpairs         wpairs\n");
 				}

//				if(print_index_phy < 64) //truncate to conserve print resources
				{
//					scale1_phy=(long int)(RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].phase);
//					scale2_phy=(long int)(fpairs_phy[print_index_phy]*THOUSANDF);
//					scale3_phy=(long int)(wpairs_phy[print_index_phy]*MILLIONF);
//					scale4_phy=(long int)(bias_asymmetry_factor_phy*MILLIONF);
//					scale5_phy=(long int)(instability_factor_phy);
					debug_printf(GEORGE_PTP_PRT,"%10ld%10ld%10ld%10ld%15le%15le%10ld%15le%15le\n",
	    			(long int)print_index_phy,
    				(long int)rfe_start_phy,
     				(long int)rfe_end_phy,
	     			(long int)Pair_Valid_Flag_phy,
		     		(RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].phase),
		     		(RFE_PHY.rfee[chan_indx_rfe][rfe_end_phy].phase),
  					(long int)RFE_PHY.rfee[chan_indx_rfe][rfe_start_phy].floor_cat,     		
     				(fpairs_phy[print_index_phy]),
	     			(wpairs_phy[print_index_phy])
    				);
				}
     		}
#endif     		
		} // end channelized pairwise calculations
		
	} //end for all fixed spaced pairs i-indexed inner loop
//		inner_range_prev_phy+=2.0* inner_range_phy;
		inner_range_prev_phy+=5.0* inner_range_phy; //GPZ Change to 4.0 TEST Dec 2011
		Fix_Space_phy=Fix_Space_phy+Fix_Space_Inc_phy; 
	} //end j-indexed outer loop
	// update fest
#if 1 // Process all raw estimates for all channels
	for(i_phy=0;i_phy<128;i_phy++) 
	{
		if(wpairs_phy[i_phy] > ZEROF)
		{
			if((i_phy%RFE_CHAN_MAX)==0) 
			{
				fest_chan[0]+= wpairs_phy[i_phy]*fpairs_phy[i_phy];
				weight_chan[0]+= wpairs_phy[i_phy];
			}
			else if((i_phy%RFE_CHAN_MAX)==1) 
			{
				fest_chan[1]+= wpairs_phy[i_phy]*fpairs_phy[i_phy];
				weight_chan[1]+= wpairs_phy[i_phy];
			}
			else if((i_phy%RFE_CHAN_MAX)==2) 
			{
				fest_chan[2]+= wpairs_phy[i_phy]*fpairs_phy[i_phy];
				weight_chan[2]+= wpairs_phy[i_phy];
			}
			else if((i_phy%RFE_CHAN_MAX)==3) 
			{
				fest_chan[3]+= wpairs_phy[i_phy]*fpairs_phy[i_phy];
				weight_chan[3]+= wpairs_phy[i_phy];
			}
			else if((i_phy%RFE_CHAN_MAX)==4) 
			{
				fest_chan[4]+= wpairs_phy[i_phy]*fpairs_phy[i_phy];
				weight_chan[4]+= wpairs_phy[i_phy];
			}
			else if((i_phy%RFE_CHAN_MAX)==5) 
			{
				fest_chan[5]+= wpairs_phy[i_phy]*fpairs_phy[i_phy];
				weight_chan[5]+= wpairs_phy[i_phy];
			}
		}
	}
#endif
   if(total_phy >0)
	{
#if 0 // Process all raw estimates for all channels
	for(i_phy=0;i_phy<128;i_phy++) 
	{
		if(wpairs_phy[i_phy] > ZEROF)
		{
			if((i_phy%RFE_CHAN_MAX)==0) 
			{
				fest_chan[0]+= wpairs_phy[i_phy]*fpairs_phy[i_phy];
				weight_chan[0]+= wpairs_phy[i_phy];
			}
			else if((i_phy%RFE_CHAN_MAX)==1) 
			{
				fest_chan[1]+= wpairs_phy[i_phy]*fpairs_phy[i_phy];
				weight_chan[1]+= wpairs_phy[i_phy];
			}
			else if((i_phy%RFE_CHAN_MAX)==2) 
			{
				fest_chan[2]+= wpairs_phy[i_phy]*fpairs_phy[i_phy];
				weight_chan[2]+= wpairs_phy[i_phy];
			}
			else if((i_phy%RFE_CHAN_MAX)==3) 
			{
				fest_chan[3]+= wpairs_phy[i_phy]*fpairs_phy[i_phy];
				weight_chan[3]+= wpairs_phy[i_phy];
			}
			else if((i_phy%RFE_CHAN_MAX)==4) 
			{
				fest_chan[4]+= wpairs_phy[i_phy]*fpairs_phy[i_phy];
				weight_chan[4]+= wpairs_phy[i_phy];
			}
		}
	}
#endif

//		weight_phy= weight_chan[0] + weight_chan[1]+ weight_chan[2]+ weight_chan[3]; 
		weight_phy= weight_chan[xds.xfer_active_chan_phy]; //GPZ DEC 2011 only use active PHY channel
		for(chan_indx_rfe=0;chan_indx_rfe<RFE_CHAN_MAX;chan_indx_rfe++)
		{
			if(weight_chan[chan_indx_rfe] >ZEROF)
			{
				fest_chan[chan_indx_rfe]= fest_chan[chan_indx_rfe]/weight_chan[chan_indx_rfe];
			}
			else
			{
            if(chan_indx_rfe == xds.xfer_active_chan_phy)
            {
				   fest_chan[chan_indx_rfe]=RFE_PHY.fest;
            }
            else
            {
				   fest_chan[chan_indx_rfe]=RFE_PHY.fest_chan[chan_indx_rfe];
            }
			}
#if 0
			if(weight_phy >0.0) //normalize weights
			{
				weight_chan[chan_indx_rfe]=weight_chan[chan_indx_rfe]/weight_phy;
			}
			else
			{
				weight_chan[chan_indx_rfe]=0.25;	
			}
			RFE_PHY.weight_chan[chan_indx_rfe]= weight_chan[chan_indx_rfe];
#endif
         // Force all weight on selected active channel GPZ DEC 2011
         if(chan_indx_rfe == xds.xfer_active_chan_phy)
         {
   			if(weight_chan[chan_indx_rfe] >ZEROF)
            {
               RFE_PHY.weight_chan[chan_indx_rfe]= 1.0;
            }
            else
            {
               RFE_PHY.weight_chan[chan_indx_rfe]= 0.0;
            }
         }
         else
         {
            RFE_PHY.weight_chan[chan_indx_rfe]= 0.0;
         }

		}
      RFE_PHY.delta_freq=-RFE_PHY.fest_cur;
		RFE_PHY.fest_cur= fest_chan[0]*RFE_PHY.weight_chan[0]+
                        fest_chan[1]*RFE_PHY.weight_chan[1]+
                        fest_chan[2]*RFE_PHY.weight_chan[2]+
                        fest_chan[3]*RFE_PHY.weight_chan[3]+
                        fest_chan[4]*RFE_PHY.weight_chan[4]+
                        fest_chan[5]*RFE_PHY.weight_chan[5];
      RFE_PHY.delta_freq += RFE_PHY.fest_cur;
		if(RFE_PHY.first)
		{
			gain_small_phy=(2.0*RFE_PHY.alpha); //was 6.0
			gain_large_phy= 1.0- gain_small_phy;
			RFE_PHY.wavg = (gain_large_phy)*RFE_PHY.wavg + weight_phy*(gain_small_phy);
			RFE_PHY.dfactor=1.0;
			RFE_PHY.fest = (gain_large_phy)*RFE_PHY.fest + RFE_PHY.fest_cur*(gain_small_phy);
			for(chan_indx_rfe=0;chan_indx_rfe<RFE_CHAN_MAX;chan_indx_rfe++)
			{
            if(chan_indx_rfe == xds.xfer_active_chan_phy)
            {
   				if(weight_chan[chan_indx_rfe] > ZEROF) 
   				{
     					RFE_PHY.fest_chan[chan_indx_rfe] = (gain_large_phy)*RFE_PHY.fest_chan[chan_indx_rfe] + (fest_chan[chan_indx_rfe])*(gain_small_phy);
               }
            }
            else
            {
               RFE_PHY.fest_chan[chan_indx_rfe]= fest_chan[chan_indx_rfe];
            }
			}
			// Test for large offset condition
			if(mode_jitter==0)
			{
				delta_first_thres_phy= 20.0;
				weight_first_thres_phy= 1e-6;
				total_first_thres_phy=3;
			}
			else
			{
				delta_first_thres_phy= 80.0;  //was 200.0 Feb 13 2012
				weight_first_thres_phy= 1e-8;
				total_first_thres_phy=10;
			}	
			if(total_first_phy>total_first_thres_phy)
			{
				fest_first_phy = fest_first_phy/(double)(total_first_phy);
				weight_first_phy = weight_first_phy/(double)(total_first_phy);
				delta_fest_phy= RFE_PHY.fest- fest_first_phy;
				if(delta_fest_phy<0.0) delta_fest_phy= -delta_fest_phy;
				debug_printf(GEORGE_PTP_PRT,"rfe_phy large offset:  total: %d, weight: %le, fest_first:%le, fest:%le, delta:%le\n",
				total_first_phy,weight_first_phy,fest_first_phy,RFE_PHY.fest,delta_fest_phy);
				if((delta_fest_phy > delta_first_thres_phy)&&(OCXO_Type()!=e_RB))
				{
//					if((weight_first_phy > weight_first_thres_phy) && (Is_PTP_FREQ_ACTIVE()==FALSE))
/* Only do this major frequency change if PHY is active and PTP is still first */
					if((Is_PHY_FREQ_ACTIVE()==TRUE)&& (RFE.first))
					{
						if(RFE_PHY.first<3)
						{
							RFE_PHY.first=2; //Stay in first for at least two cycles under cold start
						}
						rfe_turbo_acc_phy = 0.0;
						clr_residual_phase();
						init_pshape();
						#if(SYNC_E_ENABLE==1)
//						init_se();
						#endif
						detilt_rate_phy[0]=0.0;
						detilt_rate_phy[1]=0.0;
						detilt_rate_phy[2]=0.0;
						detilt_rate_phy[3]=0.0;
						detilt_rate_phy[4]=0.0;
						detilt_rate_phy[5]=0.0;
						fdrift_f=fdrift_r=fdrift=fdrift_warped=fdrift_smooth=pll_int_r=pll_int_f= fdrift_raw= holdover_f=holdover_r=fest_first_phy;
			         debug_printf(GEORGE_PTP_PRT,"fdrift_warped:  %le, fest_first_phy:%le\n",fdrift_warped, fest_first_phy);


						RFE_PHY.fest=RFE_PHY.fest_chan[0]= RFE_PHY.fest_chan[1]=RFE_PHY.fest_chan[2]=RFE_PHY.fest_chan[3]=RFE_PHY.fest_chan[4]=xds.xfer_fest=fest_first_phy;
						init_rfe_phy(); //GPZ Feb 2011 added to ensure clean startup
						setfreq(HUNDREDF*fest_first_phy,1);
						PHY_Floor_Change_Flag[0]=2;//must change floor to avoid mixing observation window times
						PHY_Floor_Change_Flag[1]=2;//must change floor to avoid mixing observation window times
						PHY_Floor_Change_Flag[2]=2;//must change floor to avoid mixing observation window times
						PHY_Floor_Change_Flag[3]=2;//must change floor to avoid mixing observation window times
						PHY_Floor_Change_Flag[4]=2;//must change floor to avoid mixing observation window times
						PHY_Floor_Change_Flag[5]=2;//must change floor to avoid mixing observation window times
						
					}
				}
			}
		}
		else
		{
			if(gain_small_phy > RFE_PHY.alpha )
			{
				gain_small_phy = 0.993 * gain_small_phy;
				gain_large_phy = 1.0 - gain_small_phy; 
			}
		
			RFE_PHY.wavg = (gain_large_phy)*RFE_PHY.wavg + weight_phy*gain_small_phy;
			// update dfactor
			if(RFE_PHY.wavg>ZEROF)
			{
				RFE_PHY.dfactor= weight_phy/(RFE_PHY.wavg);
				if(RFE_PHY.dfactor> 1.0) RFE_PHY.dfactor=1.0; //GPZ prevent excess dfactor DEC 2010
 				else if (RFE_PHY.dfactor<0.02) RFE_PHY.dfactor=0.02;
			}
			else
			{
				RFE_PHY.dfactor=1.0;
			}


			if((RFE_PHY.Nmax<18)&&((OCXO_Type()) == e_MINI_OCXO)) //was 18
			{
  				RFE_PHY.res_freq_err=0.0;
  				RFE_PHY.delta_freq=0.0;
  				RFE_PHY.smooth_delta_freq=0.0;
            RFE_PHY.fest_cur = RFE_PHY.fest;
 			}
			else if((RFE_PHY.Nmax<22)&&((OCXO_Type()) == e_OCXO)) //was 20
			{
  				RFE_PHY.res_freq_err=0.0;
  				RFE_PHY.delta_freq=0.0;
  				RFE_PHY.smooth_delta_freq=0.0;
            RFE_PHY.fest_cur = RFE_PHY.fest;
 			}

         if((OCXO_Type()) == e_MINI_OCXO) //5.8175 was 0.1 Beta 5.924
         {
				RFE_PHY.dfactor=1.0; //TEST Alway force dfactor to unity gain
    			RFE_PHY.smooth_delta_freq = (RFE_PHY.smooth_delta_freq*(1.0-0.01))  + (RFE_PHY.delta_freq*(0.01)); 
            RFE_PHY.res_freq_err =  RFE_PHY.smooth_delta_freq*(4.891/sqrt(gain_small_phy));
         }
         else if((OCXO_Type()) == e_OCXO)
         {
				RFE_PHY.dfactor=1.0; //TEST Alway force dfactor to unity gain 8.774
    			RFE_PHY.smooth_delta_freq = (RFE_PHY.smooth_delta_freq*(1.0-0.008))  + (RFE_PHY.delta_freq*(0.008)); 
            RFE_PHY.res_freq_err =  RFE_PHY.smooth_delta_freq*(7.9/sqrt(gain_small_phy));
         }
         else
         {
//            RFE_PHY.res_freq_err= 0.0;
				RFE_PHY.dfactor=1.0; //TEST Alway force dfactor to unity gain 8.774
    			RFE_PHY.smooth_delta_freq = (RFE_PHY.smooth_delta_freq*(1.0-0.008))  + (RFE_PHY.delta_freq*(0.008)); 
            RFE_PHY.res_freq_err =  RFE_PHY.smooth_delta_freq*(7.9/sqrt(gain_small_phy));
         }
//          RFE_PHY.res_freq_err= 0.0; //TEST TEST TEST
 
			dtemp_phy = (1.0-(gain_small_phy*RFE_PHY.dfactor))*(RFE_PHY.fest-RFE_PHY.res_freq_err)
                     + RFE_PHY.fest_cur*gain_small_phy*RFE_PHY.dfactor;                    
			RFE_PHY.fest=dtemp_phy +RFE_PHY.res_freq_err;
			
			for(chan_indx_rfe=0;chan_indx_rfe<RFE_CHAN_MAX;chan_indx_rfe++)
			{

            if(chan_indx_rfe == xds.xfer_active_chan_phy)
            {
   				if(weight_chan[chan_indx_rfe] > ZEROF) 
   				{
    					RFE_PHY.fest_chan[chan_indx_rfe] = (1.0-(gain_small_phy*RFE_PHY.dfactor))*RFE_PHY.fest_chan[chan_indx_rfe] + (fest_chan[chan_indx_rfe])*gain_small_phy*RFE_PHY.dfactor;
               }
            }
            else
            {
               RFE_PHY.fest_chan[chan_indx_rfe]= fest_chan[chan_indx_rfe];
            }
			}
		}	
	}
   else
   {
		for(chan_indx_rfe=0;chan_indx_rfe<RFE_CHAN_MAX;chan_indx_rfe++)
		{
  				if(weight_chan[chan_indx_rfe] > ZEROF) 
  				{
					  fest_chan[chan_indx_rfe]= fest_chan[chan_indx_rfe]/weight_chan[chan_indx_rfe];
                 RFE_PHY.fest_chan[chan_indx_rfe]= fest_chan[chan_indx_rfe];
            }
            else
            {
               RFE_PHY.fest_chan[chan_indx_rfe]= holdover_f;
            }
//			   debug_printf(GEORGE_PTP_PRT,"Assign RFE_PHY.fest_chan: chan: %d, value %ld holdover_f: %le\n",chan_indx_rfe, (long int)fest_chan[chan_indx_rfe],holdover_f );

		}
   }
#if 1
	debug_printf(GEORGE_PTP_PRT,"rfe_phy_a:%5d,fest:%ld,total:%ld,sdelta:%ld,res:%ld,indx:%3d,N:%3d,Nmax:%3d,TW:%ld,AW:%ld,df:%ld\n",
     FLL_Elapse_Sec_Count,
     (long)(RFE_PHY.fest*1000.0),
     total_phy,
     (long)(RFE_PHY.smooth_delta_freq*1000.0),
     (long)(RFE_PHY.res_freq_err*1000.0),
     RFE_PHY.index,
     RFE_PHY.N,
     RFE_PHY.Nmax,
     (long)(weight_phy*1000.0),
     (long)(RFE_PHY.wavg*1000.0),
     (long)(RFE_PHY.dfactor*1000.0));
	for(chan_indx_rfe=0;chan_indx_rfe<RFE_CHAN_MAX;chan_indx_rfe++)
	{
		debug_printf(GEORGE_PTP_PRT,"rfe_b:%5d,fest_chan:%ld,weight_chan:%ld, alpha:%ld, turbo:%ld\n",
    	 chan_indx_rfe,
		(long)(RFE_PHY.fest_chan[chan_indx_rfe]*1000.0),
	    (long)(RFE_PHY.weight_chan[chan_indx_rfe]*1000.0),
	    (long)(gain_small_phy*1000.0),
    	(long)rfe_turbo_flag_phy
     );
	}
#endif
     
}


///////////////////////////////////////////////////////////////////////
// This function call initializes rfe physical calculator
///////////////////////////////////////////////////////////////////////
void init_rfe_phy(void)
{
	int i,j;
	RFE_PHY.Nmax=0;
	if((OCXO_Type()==e_MINI_OCXO)&& (mode_jitter==0))
	{
		RFE_PHY.N=12;
   	RFE_PHY.alpha=1.0/24.0; //oscillator dependent smoothing gain across update default 1/60

//		debug_printf(GEORGE_PTP_PRT,"init_rfe: mini_ocxo low jitter mode \n");
	}
	else
	{
		RFE_PHY.N=15;  //was 15
   	RFE_PHY.alpha=1.0/48.0; //oscillator dependent smoothing gain across update default 1/60

//		debug_printf(GEORGE_PTP_PRT,"init_rfe: ocxo or RB mode \n");
	}	
	RFE_PHY.index=0;
	if(OCXO_Type()!=e_RB)
	{	
		RFE_PHY.first=8;
	}	
	else
	{
		RFE_PHY.first=8; //Sept 2010 use First Mode with Rb as well
	}
	RFE_PHY.dfactor=1.0; // additional de weighting factor based on total weight 0.1 to 1
	RFE_PHY.wavg=ZEROF;  //running average weight using alpha factor
	for(j=0;j<RFE_CHAN_MAX;j++)
	{
		RFE_PHY.weight_chan[j]= ZEROF;
	}
	for(i=0;i<RFE_PHY.N;i++)
	{
		for(j=0;j<RFE_CHAN_MAX;j++)
		{
			RFE_PHY.rfee[j][i].phase=ZEROF;
			RFE_PHY.rfee[j][i].bias=ZEROF;
			RFE_PHY.rfee[j][i].noise=ZEROF;
			RFE_PHY.rfee[j][i].tfs=0; //0 is FALSE do not use
			RFE_PHY.rfee[j][i].floor_cat=0; //0 is FALSE do not use
		}
	}
}
///////////////////////////////////////////////////////////////////////
// This function call starts rfe calculator
///////////////////////////////////////////////////////////////////////
void start_rfe_phy(double in)
{
	int i,j;
	int blank;
	double inc_phase;
	RFE_PHY.fest=in;
	RFE_PHY.fest_prev=in;
	xds.xfer_fest=xds.xfer_fest_f=xds.xfer_fest_r=RFE_PHY.fest;
	RFE_PHY.fest_chan[0]=RFE_PHY.fest;	
	RFE_PHY.fest_chan[1]=RFE_PHY.fest;	
	RFE_PHY.fest_chan[2]=RFE_PHY.fest;	
	RFE_PHY.fest_chan[3]=RFE_PHY.fest;	
	RFE_PHY.fest_chan[4]=RFE_PHY.fest;	
	RFE_PHY.fest_chan[5]=RFE_PHY.fest;	
	if(rfe_turbo_flag_phy==1)
	{
		inc_phase= 32.0*in; //turbo mode 32 seconds 4x
	}
	else if(rfe_turbo_flag_phy==2)
	{
		inc_phase= 8.0*in; //turbo mode 16 seconds 16x
	}
	else
	{
		inc_phase= 128.0*in; //standard mode 120 seconds 
	}
	Get_Freq_Corr(&last_good_freq,&gaptime);
	if(mode_jitter==1)
	{
		if(gaptime<20000) gaptime=20000;
	}
	else
	{
		if(gaptime<20000) gaptime=20000;
	}
	if(gaptime>7776000) gaptime= 7776000; //3 months	
	//GPZ Aug 2010 correct start init logic	
	if((OCXO_Type()==e_MINI_OCXO)&& (mode_jitter==0))
	{
		N_init_rfe_phy=12;
	}
	else
	{
		N_init_rfe_phy=15;  //was 15
	}
	blank = 0; //TEST TEST TEST	
	for(i=0;i<N_init_rfe_phy;i++)
	{
		for(j=0;j<RFE_CHAN_MAX;j++)
		{
	
			RFE_PHY.rfee[j][i].phase=(double)(i) * inc_phase;
			if(mode_jitter==1)
			{
				RFE_PHY.rfee[j][i].bias=(double)min_oper_thres_oper*sqrt((double)(gaptime)/10000.0); //TODO Factor in Gap Time
			}
			else
			{
				RFE_PHY.rfee[j][i].bias=(double)min_oper_thres_oper*sqrt((double)(gaptime)/10000.0); //TODO Factor in Gap Time
			}			
			RFE_PHY.rfee[j][i].noise=10.0;
			RFE_PHY.rfee[j][i].tfs=1; 
			RFE_PHY.rfee[j][i].tfs=1; 
			RFE_PHY.rfee[j][i].floor_cat=0; 
			//GPZ SEPT 2010 invalided partial block to speed up settling for initial conditions
			if(i<blank)
			{
				RFE_PHY.rfee[j][i].tfs=0; 
				RFE_PHY.rfee[j][i].floor_cat=1; 
			}
			else
			{
				RFE_PHY.rfee[j][i].tfs=1; 
			}
			debug_printf(GEORGE_PTP_PRT,"rfe_start_phy:i:%3d,phase:%ld\n",
	   			i,
	     		(long)RFE_PHY.rfee[j][i].phase
     			);
		}
	}
		RFE_PHY.index=N_init_rfe_phy-1;
		PHY_Floor_Change_Flag[0]=2;
		PHY_Floor_Change_Flag[1]=2;
		PHY_Floor_Change_Flag[2]=2;
		PHY_Floor_Change_Flag[3]=2;
		PHY_Floor_Change_Flag[4]=2;
		PHY_Floor_Change_Flag[5]=2;
		skip_rfe_phy=16;
		rfe_turbo_acc_phy=0.0;  //restart phy tilt estimate
		RFE_PHY.smooth_delta_freq = 0.0; //restart 2nd order correct 
      RFE_PHY.res_freq_err =  0.0; //restart 2nd order correction
		start_in_progress = FALSE;

}


#endif  //SOFTCLOCK EXTENSIONS endif

///////////////////////////////////////////////////////////////////////
// This function call initializes the Resource Manager
///////////////////////////////////////////////////////////////////////
void init_rm(void)
{
	int i;
	debug_printf(GEORGE_PTP_PRT,"INITIALIZE RM:\n");
	
	for(i=0;i<8;i++)
	{
		rms.time_source[i]= RM_NONE;
		rms.freq_source[i]= RM_NONE;
		if(i<4)
		{
			rms.assist_source[i]= RM_NONE;
		}
	}
	rms.time_mode = TM_SINGLE;
	rms.freq_mode = FM_SINGLE;
	rms.assist_mode=0;
//	rms.time_source[0]= RM_GPS_1; //force to GPS operation for testing
//	rms.freq_source[0]= RM_GPS_1; //force to GPS operation for testing
	rms.time_source[0]= RM_GM_1; //force to GPS operation for testing
	rms.freq_source[0]= RM_GM_1; //force to GPS operation for testing
	rms.prev_freq_source[0]= RM_GM_1; //force to GPS operation for testing

}



#if SMARTCLOCK == 1
#define HOLD_DAY_PTS 48 //number of sample points over a 24 hour period

struct Holdover_Management
{
    double Daily_Freq_Log[HOLD_DAY_PTS];    /*24 Hour Frequency Log of points*/
    double Holdover_Base[HOLD_DAY_PTS];     /*  starting point for holdover period */
    double Holdover_Drift[HOLD_DAY_PTS];    /*  incremental offset per TAU between points */
    double Prediction_Var24[HOLD_DAY_PTS];  /* log of prediction variance sec^2*/
    double Monthly_Freq_Log[30]; /*Monthly Frequency Log of daily points*/
    float Monthly_Pred_Log[30]; /*monthly prediction error log*/
    double FreqSum;           /*hourly frequency sum*/
    double Predict_Err24Hr;  /* TUNC Prediction Error over 24 hours */
    double Freq_Sum_Daily;    /*current sum of daily freq log data */
    double Drift_Sum_Weekly;  /*  sum of delta freq per day from monthly log*/
    double Osc_Freq_State;    /*Estimate of oscillator free-running freq state*/
    double Long_Term_Drift;   /*  incremental offset per TAU between points */
    double Var_Sum_Daily;     /*current sum of prediction variance prediction errors*/
    unsigned short FreqSumCnt;/*number of valid 1 second updates in sum*/
    unsigned short Cur_Point; /* Range 0-HOLD_DAY_PTS index to current point in 24 hour freq log*/
    unsigned short Cur_Day;   /* Range 0-30 index to current day in 30 day freq log */
    unsigned short Nsum_Point;  /*total number of data points in Daily Log*/
    unsigned short Nsum_Day;   /*total number of data points in Monthly log*/
   unsigned short Restart;    /*Restart flag to manage start up */
} HMS;
struct Holdover_LSF
{
  double mfit,bfit;  /*estimated slope and intercept*/
  double sx,sy,sxy,sx2; /*lsf parameters*/
  double finp;      /*previous frequency input*/
  double dsmooth;   /*smoothed drift sample*/
  double wa,wb;     /*smoothing filter weight functions*/
  unsigned short N; /*points in fit*/
} HLSF;
struct Tracking_Stats
{
     unsigned short Good_Track_Min[HOLD_DAY_PTS]; /*number of successful tracking minutes
                                                      per hour*/
     unsigned short Total_Good_Min;
    unsigned short Cur_Point;
     float Daily_Track_Success; /* Probability of successful tracking over 24 hours*/
     unsigned short Hold_Stats;   /* cumulative per minute holdover stats
                                             No. of events upper byte
                                             Total duration lower byte
                                             Check and Reset every four hours            */
     unsigned short Hold_Stats_Latch; /*Set to holdstats every 4 hours BT3 shell
                                                    clears after reporting*/
     unsigned short Last_Mode;
     unsigned short Min_Cnt;    
} TrkStats;


/*************************************************************************

Module to update holdover state

*************************************************************************/  
#define MIN_PER_HOUR 224
#define HOUR_FREQ_CNT_MIN 150
//#define MIN_PER_HOUR 449 //TEST TEST TEST
//#define HOUR_FREQ_CNT_MIN 300//TEST TEST TEST

#define MAX_HOLD_RANGE 2000
#define MAX_HOLD_SLEW 2
#define MAX_DRIFT_RATE 30
#define SMART_NARROW_FM 2500/(double)(3600*4)
#define SMART_WIDE_FM   4000/(double)(3600*4)
static double pvar_wa=1;
static double pvar_wb=0;
static double pred_var=0;
static short hold_count[4]={0,0,0,0};
static long int Holdover_Update_min_cnt = 0;
static int     gpz_test_seq[4]={0,0,0,0};
static int     gpz_test_cnt = 0;
static double  gpz_test_freq;
static double Lloop_Freq_Err_Sum_rec;

void Holdover_Update()
{
      unsigned short Hfreq_Valid,Hskip,Hgps_Valid;
      double Delta_Hold,Old_Freq,Old_Base,Old_Var;
      double freq_est,ftemp,pred_err,max_pred;
      double temp,fm_mod,freq_no_mod;
      double SlewWindow;
      short i,j,lag_indx,lag0,lag1,lag2,lag3;
//      unsigned short Status;
      struct Holdover_Management *phm;
      /******* local input message data *****/
      unsigned short Lloop_Freq_Sum_Cnt_rec;
      phm=&HMS;
//      PRR_Holdover_Print();
//      PRR_Monthly_Holdover_Print();  
//      PRR_Daily_Holdover_Print();

      /* receive eight second update FLL input data*/
      ftemp = fdrift_smooth - Lloop_Freq_Err_Sum_rec;
      if(ftemp < 0.0) ftemp = - ftemp;
      if(ftemp > 0.5)
      {
                  debug_printf(GEORGE_PTP_PRT,"%5ld: Holdover_Pop: %le, fdrift:%le, prev_fdrift: %le\n",
                  FLL_Elapse_Sec_Count_Use, ftemp, fdrift_smooth, Lloop_Freq_Err_Sum_rec );
      }
      Lloop_Freq_Err_Sum_rec= fdrift_smooth;
      Lloop_Freq_Sum_Cnt_rec=1;
      /******* complete receive of local input data *****/

      /**** accumulate one minute data */
      if(fllstatus.cur_state!=FLL_HOLD) /*restart hourly collection process*/
      {
         HMS.FreqSum+=Lloop_Freq_Err_Sum_rec;
         HMS.FreqSumCnt+=Lloop_Freq_Sum_Cnt_rec;
      } 
      /* calculate cumulative holdover stats over 4 hour window*/
      if(fllstatus.cur_state==FLL_HOLD)
      {
            if((TrkStats.Hold_Stats&0xFF)<241) TrkStats.Hold_Stats++;
            if(TrkStats.Last_Mode==FLL_NORMAL)
            {
                  if((TrkStats.Hold_Stats&0xFF00)<0xFF00) TrkStats.Hold_Stats+=0x100;
            }
      }
      TrkStats.Last_Mode=fllstatus.cur_state;
      TrkStats.Min_Cnt++;
      if(TrkStats.Min_Cnt>239)
      {
            TrkStats.Min_Cnt=0;
            TrkStats.Hold_Stats_Latch=TrkStats.Hold_Stats;
            TrkStats.Hold_Stats=0;
      }
      /******** end holdover stats calculation *********************/
      /******* end one minute accumulate*/
      Holdover_Update_min_cnt++;
      /******* generate 1 minute message from Holdover to FLL ********/
//      MSG_HOLDOVER_FLL_A.Lloop_Osc_Freq_Pred=HMS.Osc_Freq_State;
//      MSG_HOLDOVER_FLL_A.Tracking_Success=TrkStats.Daily_Track_Success;
      /*************end message generation *********************/

      /******* generate 1 minute message from Holdover to NCOUP ********/
//      NCOVAR(MSG_HOLDOVER_NCOUP_A.Lloop_Pred24_Var,HMS.Predict_Err24Hr,LO_TDEV_24);
     /*************end message generation *********************/
      /****** Update BT3 Shell  Holdover Report Mailboxes****/
//      Holdover_Report();
      if(FLL_Elapse_Sec_Count_Use < 7200)
//    if((fllstatus.cur_state==FLL_FAST)||(FLL_Elapse_Sec_Count_Use < 10000)) /*restart hourly collection process*/
      {
        /* generate initial estimates for Osc Freq State*/
            if(HMS.FreqSumCnt)
            {
                  temp=HMS.FreqSum/HMS.FreqSumCnt;
                  if(FLL_Elapse_Sec_Count_Use < 3800) HMS.Osc_Freq_State=temp;
                  else HMS.Osc_Freq_State=0.2*HMS.Osc_Freq_State+0.8*temp; 
                  HMS.Osc_Freq_State=temp;
                  HLSF.dsmooth = 0.0;    
                  debug_printf(GEORGE_PTP_PRT,"%5ld:Early Hour Estimate: %ld, count: %d\n",
                  FLL_Elapse_Sec_Count_Use,(long int) HMS.Osc_Freq_State, HMS.FreqSumCnt);

                  gpz_test_seq[0]==1;  
            }
            Holdover_Update_min_cnt=0;
            HMS.FreqSum=0.0;
            HMS.FreqSumCnt=0;
      }
      else if(Holdover_Update_min_cnt>MIN_PER_HOUR) /*perform hourly update*/
      {
      
         Holdover_Update_min_cnt=0;

        /*front end to assign pointers for multiple channels*/
           
          for(i=0;i<1;i++) /*single channel SC smart clock application */  
          {   
            gpz_test_seq[i]=0; 
            Hfreq_Valid=TRUE;
            Hgps_Valid=TRUE;            
            Hskip=FALSE;
            if(i==0)
            {
               if(HMS.FreqSumCnt > HOUR_FREQ_CNT_MIN)
               {
                  freq_est=( HMS.FreqSum/HMS.FreqSumCnt);
                  gpz_test_freq=freq_est;
                  gpz_test_cnt=HMS.FreqSumCnt;
                  Hgps_Valid=TRUE;
                  Hfreq_Valid=TRUE;
                  gpz_test_seq[i]|=2;
                  debug_printf(GEORGE_PTP_PRT,"%5ld:Holdover_Update:Hour Estimate: %ld, count: %d\n",
                  FLL_Elapse_Sec_Count_Use,(long int) freq_est, HMS.FreqSumCnt);
               }
               else
               {
                  Hfreq_Valid=FALSE;
                  Hgps_Valid=FALSE;
                  gpz_test_seq[i]|=4;
//                gpz_test_cnt=0;
               }
               /***** Only update tracking success when in normal hour update mode******/                         
               TrkStats.Total_Good_Min-= TrkStats.Good_Track_Min[HMS.Cur_Point];
               TrkStats.Good_Track_Min[HMS.Cur_Point]=(HMS.FreqSumCnt)/60;
               TrkStats.Total_Good_Min+= TrkStats.Good_Track_Min[HMS.Cur_Point];
               TrkStats.Cur_Point=HMS.Cur_Point;
               TrkStats.Daily_Track_Success=(float)(TrkStats.Total_Good_Min)/14.4;
               HMS.FreqSum=0.0;
               HMS.FreqSumCnt=0;
               phm = &HMS;
            }
            if(Hskip)
            {
               gpz_test_seq[i]|=8;
               debug_printf(GEORGE_PTP_PRT,"%5ld:Holdover_Update:Holdover update skip channel: %d\n",
               FLL_Elapse_Sec_Count_Use,i);
            }
            else
            {
               lag0=phm->Cur_Point;
               lag1=(phm->Cur_Point+HOLD_DAY_PTS-1)%HOLD_DAY_PTS;
               lag2=(phm->Cur_Point+HOLD_DAY_PTS-2)%HOLD_DAY_PTS;
               lag3=(phm->Cur_Point+HOLD_DAY_PTS-3)%HOLD_DAY_PTS;
               Old_Freq = phm->Daily_Freq_Log[lag0];
               Old_Base = phm->Holdover_Base[lag0];
               Old_Var  = phm->Prediction_Var24[lag0];
               phm->Nsum_Point++;   /*max out at 24 see code */ 
               gpz_test_seq[i]|=8192;
               if(Hfreq_Valid&&(hold_count[i]&0x01)==0) /*good  measurement data */
               {
                  hold_count[i]=0;       
                  /*** GPZ add renormalize holdover if large step****/
                  Delta_Hold = freq_est - phm->Daily_Freq_Log[lag1];
                  if(Delta_Hold<0) Delta_Hold *=-1.0; 
                  /*open up slew window for 1st day */
                  SlewWindow = MAX_HOLD_SLEW*24.0/(phm->Nsum_Point);
                  if(phm->Restart|(Delta_Hold>SlewWindow))
                  {   
                     gpz_test_seq[i]|=16;     
                     if(phm->Restart&&i==0)
                     {
                        debug_printf(GEORGE_PTP_PRT,"Restart init HLSF\n");
                        HLSF.finp=freq_est; 
                        HLSF.wa=1.0;
                        HLSF.wb=0.0;       
                        HLSF.dsmooth=phm->Long_Term_Drift;
                     }    
                     phm->Restart=FALSE;
                     phm->Nsum_Point=1;
                     phm->Cur_Point=0;
                     lag0=0;
                     lag1=HOLD_DAY_PTS-1;
                     lag2=HOLD_DAY_PTS-2;
                     lag3=HOLD_DAY_PTS-3;
                     temp=freq_est; 
                     for (j=HOLD_DAY_PTS;j>0;j--)
                     {
                        lag_indx=(lag0+j)%HOLD_DAY_PTS;
                        phm->Daily_Freq_Log[lag_indx]= temp;
                        phm->Holdover_Base[lag_indx] = temp;
                        phm->Holdover_Drift[lag_indx]=phm->Long_Term_Drift;
                        phm->Prediction_Var24[lag_indx]=0.0;
                        temp -=phm->Long_Term_Drift;
                     }
                     Old_Freq = temp;
                     Old_Base = temp;
                     Old_Var  = 0.0;
                     phm->Freq_Sum_Daily=0.0;
                     phm->Var_Sum_Daily =0.0;
                     // Special Case if large freq step double pump to clear
                     if(Delta_Hold>SlewWindow)
                     {
                        phm->Restart=TRUE;
                        return;
                     }    
                  }
                  Delta_Hold = freq_est - phm->Daily_Freq_Log[lag1];
                  if(Delta_Hold > SlewWindow)
                  {
                     Delta_Hold = SlewWindow;
                     debug_printf(GEORGE_PTP_PRT,"%5ld:Holdover_Update:Hourly update slew limit for channel: %d\n",
                     FLL_Elapse_Sec_Count_Use,i);
                  }
                  else if(Delta_Hold< -SlewWindow)
                  {
                     Delta_Hold = -SlewWindow;
                     debug_printf(GEORGE_PTP_PRT,"%5ld:Holdover_Update:Hourly update slew limit for channel: %d\n",
                     FLL_Elapse_Sec_Count_Use,i);
                  }
                  freq_no_mod=  phm->Daily_Freq_Log[lag1]+ Delta_Hold;
                  phm->Daily_Freq_Log[lag0]= freq_no_mod;
                  gpz_test_seq[i]|=32;
                  fm_mod=0.0;
               } //end if logic for good measurement data
               else
               {
                  /* forcast next freq word*/
                  /***** GPZ 12-13-00 prevent forcast until first good data*/
                  if(phm->Restart) return; 
                  gpz_test_seq[i]|=4096; 
                  hold_count[i]++;       
                  /* always add FM mod in local oscillator loop*/
                  freq_no_mod= phm->Holdover_Base[lag1]+ phm->Holdover_Drift[lag1];
                  phm->Daily_Freq_Log[lag0]= freq_no_mod;
                  gpz_test_seq[i]|=64;
                  /*Inserted FM modulation to keep prediction error at
                  upper limit for current smart clock condition*/
                  if(i==0)
                  {
 //                  if(get_prr_smartclk()==SMART_ON)
                     if(1)
                     { 
                        temp=SMART_NARROW_FM;  
                        if(HMS.Predict_Err24Hr<temp)temp=HMS.Predict_Err24Hr;
                        if(OCXO_Type()==e_RB)   temp=temp*0.75;
                     }
                     else
                     {
                        temp=SMART_WIDE_FM;                             
                     } 
                     if((lag0%2)==0) /* no need for complex modulation scheme*/
                     { 
                        gpz_test_seq[i]|=1024;
                        phm->Daily_Freq_Log[lag0]+=temp;
                        fm_mod=temp;
                     }
                     else
                     {
                        gpz_test_seq[i]|=2048;
                        phm->Daily_Freq_Log[lag0]-=temp;
                        fm_mod=-temp;
                     }
                  } /*end add FM modulation*/
                     debug_printf(GEORGE_PTP_PRT,"%5ld:Holdover_Update:Hourly Freq Invalid for channel: %d\n",
                     FLL_Elapse_Sec_Count_Use,i);
               } //else use modulated estimate logic completed 
               /******** update holdover state calculations *******/
               if(OCXO_Type()!= e_RB)
               {
                  if(i==0)
                  {
                     if(phm->Nsum_Day <2) /*use early aging curve during first 3 days */
                     {
                        Holdover_Inverse_Drift_Fit(freq_no_mod);
                        //ftemp=HLSF.bfit+HLSF.N*HLSF.mfit; /*current inverse fit*/
                        ftemp=HLSF.dsmooth ; // strait line fit
                        gpz_test_seq[i]|=128;
                        for(j=0;j<HOLD_DAY_PTS;j++)
                        {
                           if(j<phm->Nsum_Point)
                           {
                              lag_indx=(lag0-j+HOLD_DAY_PTS)%HOLD_DAY_PTS;
//                            if(ftemp!=0.0)
//                            {
                                 phm->Holdover_Drift[lag_indx]= ftemp;
//                               phm->Holdover_Drift[lag_indx]= 1.0/ftemp;
                                 /****** GPZ add sanity constraints 12-10-00*****/
                                 if(phm->Holdover_Drift[lag_indx] > MAX_DRIFT_RATE)
                                 {
                                    phm->Holdover_Drift[lag_indx]= MAX_DRIFT_RATE;
                                 }
                                 else if(phm->Holdover_Drift[lag_indx] < -MAX_DRIFT_RATE)
                                 {
                                    phm->Holdover_Drift[lag_indx]= -MAX_DRIFT_RATE;
                                 }
//                            }
//                             ftemp-= HLSF.mfit; SKIP FOR STRAIT LINE FIT  
                           }
                        }
                     } //end early aging model
                     else
                     {  
                        gpz_test_seq[i]|=256;        
                        phm->Holdover_Drift[lag0]= phm->Long_Term_Drift;
                     }
                  }  //end single local oscillator i=0 channel
               } //end non-Rb oscillator case
               ftemp=  (phm->Daily_Freq_Log[lag0]+phm->Daily_Freq_Log[lag1]);
               ftemp+= (phm->Daily_Freq_Log[lag2]+phm->Daily_Freq_Log[lag3]);
               if(OCXO_Type()!= e_RB)
               {
                  gpz_test_seq[i]|=512;   
                  phm->Holdover_Base[lag0]=freq_no_mod;
//                phm->Holdover_Base[lag0] =ftemp/4.0 +1.5*phm->Holdover_Drift[lag1]; /*use 4 hour average as base*/
               }
               else
               {
                   phm->Holdover_Drift[lag0]= 0.0;    
//                  phm->Holdover_Base[lag0] =freq_no_mod; //simple average approach A 
                   phm->Holdover_Base[lag0] =ftemp/4.0; /*use 4 hour average as base*/

#if 0
                if(phm->Nsum_Day==0)
                {
                   phm->Holdover_Base[lag0] =ftemp/4.0; /*use 4 hour average as base*/
                }
                else
                {
                    phm->Holdover_Base[lag0]=phm->Monthly_Freq_Log[((phm->Cur_Day+29)%30)];
                }
#endif
               }

               phm->Freq_Sum_Daily +=freq_no_mod;
               /******** update 24 hr prediction error******/
               max_pred=0.0;
//             ftemp= Old_Base;
//             pred_err = Old_Freq-Old_Base;
//               lag_indx=(17+lag0)%24;
               lag_indx=(HOLD_DAY_PTS+lag0 - 15)%HOLD_DAY_PTS;
               ftemp=phm->Holdover_Base[lag_indx];
               pred_err= 0;
               for(j=1;j<16;j++) //Reduced from 24 hrs to 8 for RTHC
               {
                  lag_indx=(HOLD_DAY_PTS - 15+ lag0+j)%HOLD_DAY_PTS;
                  ftemp+=phm->Holdover_Drift[lag_indx];
                  pred_err+=phm->Daily_Freq_Log[lag_indx]-ftemp;
//                if(pred_err>max_pred) max_pred=pred_err;
//                else if(pred_err<-max_pred) max_pred= -pred_err;
               }
               max_pred = pred_err*3600.0;/*convert to time error estimate*/
               /* factor in percent of holdover time which biases the prediction error to zero*/
//             max_pred = ((100.0-TrkStats.Daily_Track_Success)*phm->Predict_Err24Hr +
//             TrkStats.Daily_Track_Success*max_pred)/100.0;
               max_pred *= max_pred;
               phm->Prediction_Var24[lag0]= max_pred;
               phm->Var_Sum_Daily+= max_pred;
               /******* end prediction error update **********/
               if(phm->Nsum_Point>HOLD_DAY_PTS)
               {
                  phm->Nsum_Point=HOLD_DAY_PTS;
                  phm->Freq_Sum_Daily -= Old_Freq;
                  phm->Var_Sum_Daily-=   Old_Var;
               }
               if(i==0)
               { 
                  if(phm->Nsum_Point >14)
                  {   
                     pred_var=pvar_wa*max_pred+pvar_wb*pred_var;
                     temp=pred_var;
                     /*reduce filter bandwidth to 24hr moving average equivalant*/
                     if(pvar_wa>.0833)
                     {
                        pvar_wa*=.96;
                        pvar_wb=1.0-pvar_wa;
                     }
                  }
                  else temp=max_pred; 
                  phm->Predict_Err24Hr=sqrt(temp);
                  if(phm->Nsum_Point >30)
                  {  
//                   if(get_prr_smartclk()==SMART_OFF )
                     if(0)
                     {
                        pvar_wa=1.0; 
                        pvar_wb=1.0-pvar_wa;
                     }
                  }
               } // end i= zero logic
            /*   phm->Osc_Freq_State= (double)(phm->Holdover_Base[lag0]+ 1.5*phm->Holdover_Drift[lag0]);*/
            /*This is adjusted to make transition to holdover data seamless*/
            phm->Osc_Freq_State= (double)(phm->Holdover_Base[lag0]+0.5*phm->Holdover_Drift[lag0]);
            if(phm->Osc_Freq_State > MAX_HOLD_RANGE)
            {
               phm->Osc_Freq_State= MAX_HOLD_RANGE;
               debug_printf(GEORGE_PTP_PRT,"%5ld:Holdover_Update:Osc_Freq_State Maximum Pos for channel: %d\n",
               FLL_Elapse_Sec_Count_Use,i);
            }
            else if(phm->Osc_Freq_State < -MAX_HOLD_RANGE)
            {
               phm->Osc_Freq_State=  -MAX_HOLD_RANGE;
               debug_printf(GEORGE_PTP_PRT,"%5ld:Holdover_Update:Osc_Freq_State Maximum Neg for channel: %d\n",
               FLL_Elapse_Sec_Count_Use,i);
            }
            phm->Cur_Point++;
            if(phm->Cur_Point>(HOLD_DAY_PTS-1))  /* end of day logic  long term drift evaluation*/
            {
               phm->Cur_Point=0;
               /* Update monthly data and drift estimate */
               phm->Nsum_Day++;
               phm->Monthly_Freq_Log[phm->Cur_Day]= (phm->Freq_Sum_Daily/(double)(phm->Nsum_Point));
               phm->Monthly_Pred_Log[phm->Cur_Day]=(float)( phm->Predict_Err24Hr);
               if(phm->Nsum_Day >1)
               {
                  temp =  ((double)(phm->Monthly_Freq_Log[phm->Cur_Day])
                  -(double)(phm->Monthly_Freq_Log[(phm->Cur_Day+29)%30]))/(double)(HOLD_DAY_PTS);
                  if(phm->Nsum_Day ==3) phm->Drift_Sum_Weekly=2.0*temp; /*special case discard first drift*/
                  else  phm->Drift_Sum_Weekly+=temp;
                  if(phm->Nsum_Day>4)  /* for RTHC use 3 day average*/
                  {
                     phm->Nsum_Day=4;
                     phm->Drift_Sum_Weekly -=
                     ((double)(phm->Monthly_Freq_Log[(phm->Cur_Day+27)%30])-
                     (double)(phm->Monthly_Freq_Log[(phm->Cur_Day+26)%30]))/(double)(HOLD_DAY_PTS);
                  }
                  /*  incremental offset per hour */
                  phm->Long_Term_Drift = phm->Drift_Sum_Weekly/(phm->Nsum_Day-1);
                  /* Update NCO Holdover Drift with Range Limits */
                  if(phm->Long_Term_Drift > MAX_DRIFT_RATE)
                  {
                     debug_printf(GEORGE_PTP_PRT,"%5ld:Holdover_Update:Daily update POS slew limit for channel: %d\n",
                     FLL_Elapse_Sec_Count_Use,i);
                     phm->Long_Term_Drift= MAX_DRIFT_RATE;
                  }
                  else if(phm->Long_Term_Drift < -MAX_DRIFT_RATE)
                  {
                     debug_printf(GEORGE_PTP_PRT,"%5ld:Holdover_Update:Daily update NEG slew limit for channel: %d\n",
                     FLL_Elapse_Sec_Count_Use,i);
                     phm->Long_Term_Drift = -MAX_DRIFT_RATE;
                  } 
               }  /* end if Nsum_Daily >1 */
               if(OCXO_Type()!= e_RB)
               {
                  if(phm->Nsum_Day ==2) /*special case */
                  {
                     for(j=0;j<HOLD_DAY_PTS;j++)phm->Holdover_Drift[j]= phm->Long_Term_Drift;
                  }
               }
               phm->Cur_Day++;
               if(phm->Cur_Day>29)phm->Cur_Day=0;
               PRR_Monthly_Holdover_Print();  
             } /* end of end of day logic */
            } /*end else no holdover skip*/
        } /*end for each channel*/
         PRR_Holdover_Print();  
         PRR_Daily_Holdover_Print();

      } /*end hourly update*/
}  
/************************************************************************
Get Oscillator Prediction
*************************************************************************/
double Get_LO_Pred(void)
{
      double LO_est;
      //Interpolate during sampling period
      LO_est=  HMS.Osc_Freq_State + 
      ((double)(Holdover_Update_min_cnt)/(double)(MIN_PER_HOUR))*HLSF.dsmooth;

      return(LO_est);
}
/*********************************************************************
This function is used during the first week of oscillator aging to determine
the early aging curve an OCXO.
This function calculates a least squares fit to the "inverse drift rate"
of the LO hourly frequency data. This is equivalant to fitting the standard
log fit y=A+Blog(1+ct) aging curve. By taking the derivative we obtain the
drift curve fit d=B/(c(1+ct)). Thus the inverse drift rate can be modelled
as a linear process id = mt+b (m=c/B and b=c*c/B).


*************************************************************************/
void Holdover_Inverse_Drift_Fit(double fin)
{
  double id,denom; /*derived inverse drift sample*/
//  if(HLSF.wa==0.0)  /* initialize algorithm if start */
//  {
//      HLSF.wa=1.0;
//      HLSF.wb=0.0;
//      HLSF.dsmooth=0.0;
//      HLSF.mfit=0.0;
//      HLSF.bfit=0.0;
//      HLSF.sx=0.0;
//      HLSF.sy=0.0;
//      HLSF.sxy=0.0;
//      HLSF.sx2=0.0;
//      HLSF.finp=fin;
//      return;
//  }  
  id= fin-HLSF.finp;
  HLSF.finp=fin;
    if(id > MAX_DRIFT_RATE)
    {
        id= MAX_DRIFT_RATE;
    }
    else if(id < -MAX_DRIFT_RATE)
    {
        id= -MAX_DRIFT_RATE;
    }
 
 
  /**** progressively increase filtering to counteract high noise gain as
          drift approaches zero. *******/
  HLSF.dsmooth=HLSF.wa*id+HLSF.wb*HLSF.dsmooth;
 
  /*reduce filter bandwidth to 24hr moving average equivalant*/
  if(HLSF.wa>.0833)
  {   
//      DebugPrint("update wa,wb\r\n", 3); 

     HLSF.wa*=.96;
     HLSF.wb=1.0-HLSF.wa;
  }
  /* clip hyperbolic infinite gain for drift less than 1e-12 per hr */
  if(HLSF.dsmooth< 1e-3)
  {
//    DebugPrint("clip id level\r\n", 3); 
 
     if (HLSF.dsmooth> 0.0) id = 1e-3;
     else if(HLSF.dsmooth > -1e-3) id=-1e-3;
     else id = HLSF.dsmooth;
  }
  else id = HLSF.dsmooth;
  id=1.0/id;
  /* update LSF stats */
  HLSF.N++;
  HLSF.sx+=HLSF.N;
  HLSF.sy+=id;
  HLSF.sxy+= HLSF.N*id;
  HLSF.sx2+= HLSF.N*HLSF.N;
  /* update fit coefficients*/
  denom=HLSF.N*HLSF.sx2-HLSF.sx*HLSF.sx;
  if(denom!=0.0)
  {
     HLSF.mfit= ((HLSF.N*HLSF.sxy)- HLSF.sx*HLSF.sy)/denom;
     HLSF.bfit= ((HLSF.sy*HLSF.sx2)-(HLSF.sx*HLSF.sxy))/denom;
  }
}
void Init_Holdover(void)
{
      int i;
      HMS.Restart=TRUE;
     for(i=0;i<HOLD_DAY_PTS;i++)
     {
        TrkStats.Good_Track_Min[i]=30;
     }
     TrkStats.Total_Good_Min=1440;
     TrkStats.Cur_Point=0;
     TrkStats.Daily_Track_Success=100.0;
     TrkStats.Hold_Stats=0;
     TrkStats.Last_Mode=FLL_WARMUP;
     TrkStats.Min_Cnt=0;
     TrkStats.Hold_Stats_Latch=0;
}
/*****************************************************************
Time Provider Holdover Debug Print Function
******************************************************************/
void PRR_Holdover_Print()
{   
     int i1,i2;
    i1= (HMS.Cur_Point+HOLD_DAY_PTS-1)%HOLD_DAY_PTS; 
    i2= (HMS.Cur_Day +29)%30;
    debug_printf(GEORGE_PTP_PRT,"HOLD: cnt ,seq ,freq ,base,drift, perr, osc ,ltd  ,lsf-d,wa   ,wb   ,chour,cday,nhour,nday,start\n");
    debug_printf(GEORGE_PTP_PRT,"HOLD:%4d,%4x,%5ld,%5ld,%5ld,%5ld,%5ld,%5ld,%5ld,%5ld,%5ld,%5d,%4d,%5d,%4d,%3d\n", 
    gpz_test_cnt,
    gpz_test_seq[0],  
    (long int)(HMS.Daily_Freq_Log[i1]*(double)(1e3)),
    (long int)(HMS.Holdover_Base[i1]*(double)(1e3)),
    (long int)(HMS.Holdover_Drift[i1]*(double)(1e3)),
    (long int)(HMS.Predict_Err24Hr*(double)(1)),
    (long int)(HMS.Osc_Freq_State*(double)(1e3)),
    (long int)(HMS.Long_Term_Drift*(double)(1e3)),
   (long int)(  HLSF.dsmooth*(double)(1e3)),  
    (long int)(  HLSF.wa*(double)(1e2)),  
    (long int)(  HLSF.wb*(double)(1e2)),  
    HMS.Cur_Point,
    HMS.Cur_Day,
    HMS.Nsum_Point,
    HMS.Nsum_Day,
    HMS.Restart
    );  
}   
/*****************************************************************
Time Provider Daily Holdover Debug Print Function
******************************************************************/
void PRR_Daily_Holdover_Print()
{                   
    int i2,i1;
    i2= (HMS.Cur_Point +HOLD_DAY_PTS-1)%HOLD_DAY_PTS;    
    debug_printf(GEORGE_PTP_PRT,"DAILY_GPS_HOLDOVER:index %4d\n",i2); 
    for(i1=0;i1<HOLD_DAY_PTS;i1=i1+6)
    {
         debug_printf(GEORGE_PTP_PRT,"%15ld,%15ld,%15ld,%15ld,%15ld,%15ld,",
        (long int)(HMS.Daily_Freq_Log[(i2+HOLD_DAY_PTS-i1)%HOLD_DAY_PTS]*(double)(1e3)),
        (long int)(HMS.Daily_Freq_Log[(i2+HOLD_DAY_PTS-1-i1)%HOLD_DAY_PTS]*(double)(1e3)),
        (long int)(HMS.Daily_Freq_Log[(i2+HOLD_DAY_PTS-2-i1)%HOLD_DAY_PTS]*(double)(1e3)),
        (long int)(HMS.Daily_Freq_Log[(i2+HOLD_DAY_PTS-3-i1)%HOLD_DAY_PTS]*(double)(1e3)),
        (long int)(HMS.Daily_Freq_Log[(i2+HOLD_DAY_PTS-4-i1)%HOLD_DAY_PTS]*(double)(1e3)),
        (long int)(HMS.Daily_Freq_Log[(i2+HOLD_DAY_PTS-5-i1)%HOLD_DAY_PTS]*(double)(1e3))
        );     
       debug_printf(GEORGE_PTP_PRT,"\n");
     }   
}
 
/*****************************************************************
Time Provider Monthly Holdover Debug Print Function
******************************************************************/
void PRR_Monthly_Holdover_Print()
{                   
     int i2,i1;
    i2= (HMS.Cur_Day +29)%30;    
    debug_printf(GEORGE_PTP_PRT,"MONTHLY_GPS_HOLDOVER:index %4d\r\n",i2); 
    for(i1=0;i1<30;i1=i1+6)
    {
        debug_printf(GEORGE_PTP_PRT,"%15ld,%15ld,%15ld,%15ld,%15ld,%15ld,",
        (long int)(HMS.Monthly_Freq_Log[(i2+30-i1)%30]*(double)(1e3)),
        (long int)(HMS.Monthly_Freq_Log[(i2+29-i1)%30]*(double)(1e3)),
        (long int)(HMS.Monthly_Freq_Log[(i2+28-i1)%30]*(double)(1e3)),
        (long int)(HMS.Monthly_Freq_Log[(i2+27-i1)%30]*(double)(1e3)),
        (long int)(HMS.Monthly_Freq_Log[(i2+26-i1)%30]*(double)(1e3)),
        (long int)(HMS.Monthly_Freq_Log[(i2+25-i1)%30]*(double)(1e3))
        );     
       debug_printf(GEORGE_PTP_PRT,"\n");
     }   
}
#if 0
/***************************************************************************
Module for testing holdover algorithm only.
*****************************************************************************/
void Holdover_Test()
{
    int i;
    /* jam to normal state with long duration */
    smi.BT3_mode=GPS_Normal;
    smi.BT3_mode_dur = (smi.sloop_int_tc *3)/60; /* force 3 time constants of
                                                                    stable operation */
        DloopSA.Prim_Int_State = 1e-7; /*fixed offset test*/
    for(i=0;i<900;i++)
    {
        if(i%10==5&&i>50) smi.BT3_mode=GPS_Warmup;
        else smi.BT3_mode=GPS_Normal;
        DloopSA.Prim_Int_State += 1e-11; /*linear ramp test*/
        Holdover_Update();
    }
} 
#endif



#endif
