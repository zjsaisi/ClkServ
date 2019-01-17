
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

FILE NAME    : clk_main.c

AUTHOR       : George Zampetti

DESCRIPTION  : 


Revision control header:
$Id: CLK/clk_main.c 1.30 2012/04/23 15:22:22PDT German Alvarez (galvarez) Exp  $

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
#include "ptp.h"
#include "clk_private.h"
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
BOOLEAN jam_now_flag = FALSE;
BOOLEAN never_jam_flag = FALSE;
UINT32 jam_threshold = 100000; /* 100usecs */
INT32	mode_jitter = 0;
INT32	dsl_flag=0;
INT32	enterprise_flag=0;
FLOAT32 last_good_freq;
UINT32  gaptime;
UINT32 	FLL_Elapse_Sec_Count = 0;
UINT32 	FLL_Elapse_Sec_PTP_Count = 0;
UINT32	FLL_Elapse_Sec_Count_Use=0;

INT16   fll_settle =FLL_SETTLE;  
double  detilt_cluster = ZEROF;
double 	holdover_f;
double 	holdover_r;
double	fdrift_f=ZEROF;
double	fdrift_r=ZEROF;
double	pll_int_f = ZEROF;
double	pll_int_r = ZEROF;
double 	fbias,rbias;
double 	fbias_small,rbias_small;
double 	fbias_large,rbias_large;
INT32 min_oper_thres_f =1000;
INT32 min_oper_thres_r =1000;
INT32 min_oper_thres_small_f =1000;
INT32 min_oper_thres_small_r =1000;
INT32 min_oper_thres_large_f =1000;
INT32 min_oper_thres_large_r =1000;
Load_State LoadState = LOAD_CALNEX_10;
//Load_State LoadState = LOAD_G_8261_5;
//Load_State LoadState = LOAD_G_8261_10;
int  rfe_settle_count;
UINT16 hcount=0; 
#if(NO_PHY==0)
INPUT_PROCESS_DATA_PHY IPD_PHY; //DEC 2010 New GPS input channels
RFE_STRUCT_PHY RFE_PHY;
GPS_PERFORMANCE_STRUCT gps_pa, gps_pb, gps_pc, gps_pd, se_pa, se_pb, red_pa;
INT32 min_oper_thres_phy[IPP_CHAN_PHY] = { 200, 200, 200, 200, 200, 200, 200};
INT32 Phy_Channel_Transient_Free_Cnt[IPP_CHAN_PHY]= {0, 0, 0, 0, 0, 0, 0};
int ctp_phy = 64; //cluster test density point
int low_warning_phy[IPP_CHAN_PHY]= {0, 0, 0, 0, 0, 0, 0};//GPZ NOV 2010 consecutive low density warning
int high_warning_phy[IPP_CHAN_PHY]= {0, 0, 0, 0, 0, 0, 0};//GPZ NOV 2010 consecutive low density warning
// rfe_turbo flag 1=4x 2=8x speed
UINT16 rfe_turbo_flag_phy = FALSE;
UINT16 rfe_turbo_flag_forward_phy = FALSE;
UINT16 rfe_turbo_flag_reverse_phy = FALSE;
double rfe_turbo_acc_phy = 0.0;
INT64 rfe_turbo_phase_acc_phy[IPP_CHAN_PHY];
double rfe_turbo_phase_phy[IPP_CHAN_PHY];
INT8 PHY_Floor_Change_Flag[IPP_CHAN_PHY]={ 0, 0, 0, 0, 0, 0, 0};
INT16 skip_rfe_phy = 0;  

INT32 avg_GPS_A;
INT32 avg_GPS_B;
INT32 avg_GPS_C;
INT32 avg_GPS_D;
INT32 avg_RED;
INT32  cluster_inc_phy[IPP_CHAN_PHY] = {CLUSTER_INC,CLUSTER_INC,CLUSTER_INC,CLUSTER_INC}; 
INT32  cluster_inc_count_phy[IPP_CHAN_PHY] = {0,0,0,0,0,0,0}; 
INT32  cluster_inc_slew_count_phy[IPP_CHAN_PHY] = {0,0,0,0,0,0,0}; 
INT32  cluster_inc_sense_phy[IPP_CHAN_PHY] = {0,0,0,0,0,0,0}; 
INT32  cluster_inc_sense_prev_phy[IPP_CHAN_PHY] = {0,0,0,0,0,0,0}; 
double detilt_rate_phy[IPP_CHAN_PHY]={0.0, 0.0, 0.0, 0.0,0.0,0.0,0.0};

#endif
double detilt_correction_phy[IPP_CHAN_PHY]={0.0, 0.0, 0.0, 0.0, 0.0, 0.0,0.0};
double time_bias_est_rate;
int detilt_blank[IPP_CHAN_PHY] = {32, 32, 32, 32, 32, 32,32};
double detilt_correction_for= 0.0;
double detilt_correction_rev= 0.0;
double time_bias_est;
double residual_time_error;
RESOURCE_MANAGEMENT_STRUCT rms;
double fdrift_smooth=ZEROF;
double	fdrift=ZEROF;
double	fdrift_raw=ZEROF;
double 	fdrift_warped=ZEROF;
double	hold_res_acc=ZEROF; //created NOV 2010 new term to improve application of open loop correction
double	hold_res_acc_avg=ZEROF; //created NOV 2010 new term to improve application of open loop correction

UINT16  delta_forward_slew, delta_reverse_slew;// dynamic slew control active
UINT16  delta_forward_slew_cnt, delta_reverse_slew_cnt;// dynamic slew control active
INT8		PTP_LOS;
INT8    Phase_Control_Enable;

double forward_weight, reverse_weight;
INT32	min_oper_thres_oper=100;
INT32	step_thres_oper = 11500;
INT8 var_cal_flag=0;
INT16 ptpRate;
t_ptpAccessTransportEnum ptpTransport;
// Input Histogram Data Processing Section
INPUT_PROCESS_DATA IPD; //may 2010 new general input processing data structure

UINT8 timestamp_transition_flag_servo=0; 
INT32 timestamp_align_offset_servo=0; //GPZ accumulated timestamp error
INT32 timestamp_align_offset_prev_servo=0; //GPZ accumulated timestamp error
UINT16 timestamp_skip_cnt_servo =0;
UINT16 timestamp_head_f_servo, timestamp_head_r_servo;
INT16 timestamp_align_skip_servo;

// Data Supporting operational offset floor extension

INT32 ofw_start_f = OFFSET_THRES; //ensure large enough to avoid lock to rising edge
INT32 ofw_start_thres_f; //ensure large enough to avoid lock to rising edge
INT32 ofw_cluster_total_f;
INT32 ofw_cluster_total_prev_f;
INT32 ofw_cluster_total_sum_f;
INT32 ofw_cluster_total_sum_prev_f;

INT32 next_ofw_start_f = OFFSET_THRES; //ensure large enough to avoid lock to rising edge
INT32 next_ofw_cluster_total_f;
INT32 next_ofw_cluster_total_prev_f;
INT32 next_ofw_cluster_total_sum_f;
INT32 next_ofw_cluster_total_sum_prev_f;

INT32 ofw_gain_f=32;
int ofw_cluster_iter_f; //cluster iteration index
int ofw_cur_dir_f=1;
int ofw_prev_dir_f=1;
int ofw_uslew_f;

INT32 ofw_start_r = OFFSET_THRES; //ensure large enough to avoid lock to rising edge
INT32 ofw_start_thres_r; //ensure large enough to avoid lock to rising edge
INT32 ofw_cluster_total_r;
INT32 ofw_cluster_total_prev_r;
INT32 ofw_cluster_total_sum_r;
INT32 ofw_cluster_total_sum_prev_r;

INT32 next_ofw_start_r = OFFSET_THRES; //ensure large enough to avoid lock to rising edge
INT32 next_ofw_cluster_total_r;
INT32 next_ofw_cluster_total_prev_r;
INT32 next_ofw_cluster_total_sum_r;
INT32 next_ofw_cluster_total_sum_prev_r;

INT32 ofw_gain_r=32;
int ofw_cluster_iter_r; //cluster iteration index
int ofw_cur_dir_r=1;
int ofw_prev_dir_r=1;
int ofw_uslew_r;

INT32 	Initial_Offset = 0;
INT32 	Initial_Offset_Reverse = 0;
int 	start_in_progress = TRUE;
int 	start_in_progress_PTP = TRUE;
int ctp=64; //cluster test density point
double floor_density_f;
double floor_density_r;
double floor_density_small_f;
double floor_density_small_r;
double floor_density_large_f;
double floor_density_large_r;
double floor_density_band_f;
double floor_density_band_r;

INT32  cluster_width_max=60000;
INT32  cluster_inc_f = CLUSTER_INC*8; //was 32
INT32  cluster_inc_r = CLUSTER_INC*8;
INT32  cluster_inc	  = CLUSTER_INC*8;
INT32  cluster_inc_count_f = 0;
INT32  cluster_inc_count_r = 0;
INT32  cluster_inc_slew_count_f = 0;
INT32  cluster_inc_slew_count_r = 0;
INT32  cluster_inc_sense_f = 0;
INT32  cluster_inc_sense_r = 0;
INT32  cluster_inc_sense_prev_f = 0;
INT32  cluster_inc_sense_prev_r = 0;
INT32	mode_width_max_oper=300000;
INT32	min_density_thres_oper=4;

// min validation section
// Congrats !!!!!!! After 1 day of debuging you have found the problem (TODO)
INT32 rmw_f[MAX_RM_SIZE + 1],rmw_r[MAX_RM_SIZE + 1]; //replacement min search windows  //JAN19_MERGE
INT32 lvm_f=MAXMIN; //last valid min
INT32 lvm_r=MAXMIN; //last valid minimum
long long rm_f, rm_r;   //candidate replacement min
INT32 rmf_f, rmf_r;   //candidate replacement min floor
INT32 om_f,om_r; 	//output validated min
int	rmc_f, rmc_r;	//replacement min cluster count
int	mflag_f, mflag_r; //min validity flags
int rmfirst_f=1;
int	rmfirst_r=1;
int rmi_f, rmi_r;	
INT32	rm_size_oper= (64 > MAX_RM_SIZE)?MAX_RM_SIZE:64;


MIN_STRUCT min_forward, min_reverse;

// RFE Data Section
RFE_STRUCT RFE;
XFER_DATA_STRUCT xds;
INT16 rfe_cindx; //cur_index
INT16 rfe_pindx; //prev_index
INT16 rfe_ppindx; //prev_index
INT16 rfe_pppindx; //prev_index
INT16 rfe_findx; //future_index
INT16 time_transient_flag;
double RFE_in_forward[5];
double RFE_in_reverse[5];
INT16	hcount_rfe;
INT16	hcount_recovery_rfe;
INT16 hcount_recovery_rfe_start;
// rfe_turbo flag 1=4x 2=8x speed
UINT16 rfe_turbo_flag = 1;  //GPZ Jan 2011 start with turbo on
UINT16 rfe_turbo_flag_forward = FALSE;
UINT16 rfe_turbo_flag_reverse = FALSE;
double rfe_turbo_acc = 0.0;
double rfe_turbo_phase_acc[2];
double rfe_turbo_phase[2];
double rfe_turbo_phase_avg[2];
double rfe_turbo_phase_acc_small[2];
double rfe_turbo_phase_small[2];
double rfe_turbo_phase_avg_small[2];
double rfe_turbo_phase_acc_large[2];
double rfe_turbo_phase_large[2];
double rfe_turbo_phase_avg_large[2];
double rfe_turbo_phase_acc_large_band[2];
double rfe_turbo_phase_large_band[2];
double rfe_turbo_phase_avg_large_band[2];

int turbo_cluster_for=120; //GPZ APRIL 2011
int turbo_cluster_rev=120; //GPZ APRIL 2011

int N_init_rfe=60;
int N_init_rfe_phy=60;
INT8 Forward_Floor_Change_Flag=0;
INT8 Reverse_Floor_Change_Flag=0;
INT8 f_floor_event = 0;
INT8 r_floor_event = 0;
INT16 skip_rfe = 0;  
int short_skip_cnt_f =0; //GPZ NOV 2010 used to skip short RFE window during transients
int short_skip_cnt_r =0;
int low_warning_for=0; //GPZ NOV 2010 consecutive low density warning
int low_warning_rev=0;
int high_warning_for=0; //GPZ NOV 2010 consecutive low density warning
int high_warning_rev=0;
//int load_drop_for=0; 
//int load_drop_rev=0;

int complete_phase_flag=0;

// FLL Status section
FLL_STATUS_STRUCT fllstatus;
Fll_Clock_State tstate = FLL_UNKNOWN; 
Fll_Clock_State prev_tstate = FLL_UNKNOWN;
double round_trip_delay_alt; //minimal round trip delay estimate
double time_bias_mean, time_bias_var;
BOOLEAN o_fll_init = FALSE;

//IPPM Data Section
#if(!NO_IPPM)				
IPPM_STRUCT ippm_forward, ippm_reverse, *pippm;
#endif
// ZTIE Calculator
ZTIE_STRUCT ztie_stats;
//TDEV Section
TDEV_STRUCT forward_stats,reverse_stats;


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


