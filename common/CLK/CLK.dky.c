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

FILE NAME    : CLK.c

AUTHOR       : George Zampetti

DESCRIPTION  : 

Main PTP frequency and time engine

Revision control header:
$Id: CLK/CLK.c 1.180 2012/04/23 15:22:22PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/

//////////////////// includes
//#define TP500_BUILD
//#define TEMPERATURE_ON
//////////////////// includes
#ifndef TP500_BUILD
#include "proj_inc.h"
#include "target.h"
#include "sc_types.h"
#include "sc_servo_api.h"
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <syslog.h>
#include "DBG/DBG.h"
#include "sc_alarms.h"
#include "logger.h"
#include "ptp.h"
#include "CLKflt.h"
#include "CLKpriv.h"
#include "clk_private.h"
#define sqrt(x)  sqrt_local(x)
double sqrt_local(double f64_value);
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
//#include "datatypes.h"
#include "m523xevb.h"
#include "clk_private.h"
#include "ptp.h"
#include "debug.h"
#include "var.h"
//#include "math.h"
#include "logger.h"
#include "pps.h"
#include "sc_ptp_servo.h"
#include "sock_manage.h"
#include "CLKpriv.h"
#include "sc_api.h"
//#include "sc_servo_api.h"
#include "temperature.h"
#include "chan_select.h"
#endif
#include "sys_defines.h"
#include "fpga_ioctl.h"

#define HOLDOFF_DELAY_PTP 300
#define HOLDOFF_DELAY_GPS 300
#define HOLDOFF_DELAY_SE 300

#define START_WITH_FREQ_ONLY

#ifndef ZEROF
#define ZEROF (0.0)
#endif
//#define FAST_VCAL
//#define debug_printf(a,x...) printf(x)
// global variables
/////////////////// external variables
extern int SC_GetSePhaseOffset(UINT8 mux, INT32 *u32_phaseOffset);

#define GPS_PHASOR_MUX    0
#define SYNCE1_PHASOR_MUX 1
#define SYNCE2_PHASOR_MUX 2

extern int SC_GetPhasorPhaseOffset(UINT8 mux, INT32 *i32_phaseOffset);
extern UINT16 ptp_delay_resp_flag;
extern void BesTime_init(int);
extern void Set_Timestamp_Offset(INT64 ll_ts_offset);

extern const float kOscOffsetLimit[];

//static INT64 local_pps_offset = 0;
//jiwang
#if 0
static INT16 time_hold_off= 180;
#endif
static INT16 time_hold_off= 10;
static INT16 time_rate_hold_off= 0;
//static UINT8 zero_load_case = 0;
static INT8 input_repeat_cnt_limit=7; //Controls pacing of core measurement process
#if 1
typedef struct 
{
	double seg_ffo;    //fractional frequnency offset at start of segment ppb
	double seg_inc_slope;  //incremental slope of segment (1 is perfect)
	UINT8  done_flag; // one indicates segment data is valid		
 }
 VCAL_ELEMENT_STRUCT;

typedef struct 
{
	UINT8	Varactor_Cal_Flag; //asserted while cal cycle is active
	UINT8 N; 		 //number of segments in table bounded by VAR_TABLE_SIZE 
	UINT8 index; 		 //current index into VAR_TABLE_SIZE 
	UINT8 pacing;  // control settling time
	double min_freq, max_freq, delta_freq; //upper,lower bounds and delta of table ppb
	long int cur_freq; //current calibration frequency in E-11
	double zero_cal_start,zero_cal_end;
	VCAL_ELEMENT_STRUCT vcal_element[VAR_TABLE_SIZE];
} VCAL_STRUCT;
#endif

static UINT32 jam_threshold_counter = 0;
static int vcal_complete = -1;
static UINT32 tick_counter = 0;
static UINT8 Phase_test_flag = 0;
static UINT8 Freq_test_flag = 0;
static UINT8 gSC_TST_FREQ_ZERO = 0;
//static INT8 var_cal_flag=0;
static VCAL_STRUCT vcal_working; //working copy of varactor table
//static VCAL_STRUCT vcal_test; //test copy of varactor table

/////////////////// local variables
static  UINT32 tstate_counter = 0;

//static NVM nvm;  /* store set-up variables into this structure */
static UINT16 restart_phase_time_flag = FALSE;
//static UINT32 offset_comp = 0;
//static INT16 fll_settle =FLL_SETTLE;  
//static double detilt_cluster = ZEROF;
static INT16 log_pace =14400; //pacing of diag KPI entries
static UINT8 hold_reason_transient, hold_reason_flow;
//static UINT16 nv_ptp_transport = PTP_TRANSPORT_ETH;
#define PPS_MODE_STANDARD 0
#define PPS_MODE_COUPLED  1
//static UINT16 nv_pps_mode = PPS_MODE_STANDARD;
//static UINT8 timestamp_align_flag=0; // TEST SET To ZERO for release GPZ allows control of timestamp alignment

//static double phase_offset = 0;
static double pf,pr;

static UINT16 set_phase_counter = 0;
//static UINT16 number_of_delay_RTD_filtered_1sec;
//static UINT16 number_of_sync_RTD_filtered_1sec;
boolean signaling_sync_ack_flag[2];
boolean signaling_delay_ack_flag[2];
boolean signaling_announce_ack_flag[2];
UINT16 Current_Delay_Req_Seq_Num;

static	UINT16 sync_cntr = 0, delay_cntr = 0;
//static  UINT16 sync_sticky[2], delay_sticky;
//FIFO_STRUCT PTP_Fifo[2]; /* large 1024 element array containing seq number, t1-t4 */
//FIFO_STRUCT PTP_Delay_Fifo;  /*large 1024 element array containing seq number, t1-t4 */ FIFO_STRUCT PTP_Fifo_Local;  /* large 1024 element array containing seq number, t1-t4 */

FIFO_STRUCT PTP_Delay_Fifo_Local;  /* large 1024 element array containing seq number, t1-t4 */
FIFO_STRUCT PTP_Fifo_Local;  /* large 1024 element array containing seq number, t1-t4 */
uint_16 Delay_Out_Indx;
uint_16 Sync_Out_Indx;
#if(NO_PHY==0)
FIFO_STRUCT GPS_A_Fifo_Local,GPS_B_Fifo_Local, GPS_C_Fifo_Local, SYNCE_A_Fifo_Local,SYNCE_B_Fifo_Local, RED_Fifo_Local;
#endif


#ifdef TEMPERATURE_ON
extern TEMPERATURE_STRUCT temp_stats;
#endif
#if(SYNC_E_ENABLE==1)
static SE_MEAS_CHAN SMC[SE_CHANS];
#endif 
#if(ASYMM_CORRECT_ENABLE==1)
static ASYMM_CORR_LIST ACL;
#endif
//static UINT16 	Delay_req_period_ms;
//static double 	holdover_f;
//static double 	holdover_r;
//static double	fdrift_f=ZEROF;
//static double	fdrift_r=ZEROF;
//static double	pll_int_f = ZEROF;
//static double	pll_int_r = ZEROF;

//static double	fdrift=ZEROF;
//static double	fdrift_raw=ZEROF;
//static double 	fdrift_warped=ZEROF;

//static double	fdrift_smooth=ZEROF;
static double	fdrift_f_meas=ZEROF;
static double	fdrift_r_meas=ZEROF;
static double pshape=ZEROF;
static double pshape_raw=ZEROF;
static double tshape_raw=ZEROF;
static double pshape_smooth=ZEROF;
static UINT8 clr_phase_flag =0;
//static double fpairs[256]; 
//static double wpairs[256];	
static INT8 min_count=0;
static double pshape_range       = 20000.0;
//static double pshape_floor_fine  =  2000.0; //was 2000 try 5000 NOV 2010
static double pshape_acq_fine    =  1000.0;
static double pshape_norm_fine_a =  4000.0;
static double pshape_norm_fine_b = 12000.0;
static double pshape_norm_fine_c = 18000.0;
static double pshape_range_use   = 18000.0;
static double pshape_norm_std    = 20000.0;
static double pshape_wide_std    = 86400.0;
static double pshape_tc_old = 1000.0;
static double pshape_tc_cur = 1000.0;
static double fshape=ZEROF;
//static double tshape_acc=ZEROF; //GPZ OCT 2010 added to address long term biases
static double tshape=ZEROF;
static double pshape_base=ZEROF;
static double pshape_base_old=ZEROF;
static double delta_pshape=ZEROF;
static INT32 phase_new_cnt;
static double tgain_comp=1.0;
static double tgain_comm;
static double tgain_diff;
static double ltgain;
static int min_accel=0;
static int jam_needed = FALSE;
static UINT32 pll_int_f_less_than_3ppb_count;
static double cal_freq=ZEROF;
static VCAL_State vstate = VCAL_START;

static int dflag; //engage delay line
// OCT 29 move to top
static double scw_f, scw_r, smw_f, smw_r; 
static double cscw_f, cscw_r, csmw_f, csmw_r;
// cluster stats
static double scw_avg_f,scw_avg_r,scw_e2_f,scw_e2_r;

//static double test_scale=ZEROF;
static int reshape =0;
//static int calnex_f, calnex_r; //DEC 2010 flags indicating operating with calnex playback artificial PDV
static double hgain1=PER_05;
static double hgain2=PER_95;
static double hgain_floor= 0.001;
//static INT32 min_thres_1 =MIN_CLUSTER_1;
//static INT32 min_thres_2 =MIN_CLUSTER_2;
//static INT32 min_thres_3 =MIN_CLUSTER_3;
//static INT32 min_thres_4 =MIN_CLUSTER_4;
static INT32 fll_stabilization = 0;
//static INT32 min_oper_thres_f =1000;
//static INT32 min_oper_thres_r =1000;
//static int ctp=64; //cluster test density point
//static int min_density_thres=4;


//static INT8 Forward_Floor_Change_Flag=0;
//static INT8 Reverse_Floor_Change_Flag=0;
double Get_time_correct(void);


//static INT32 mcompress_comp_f =0;
//static INT32 mcompress_comp_r =0;

//static FLOAT32 last_good_freq;
//static UINT32 gaptime;

//TODO Things remaining


//Add BesTime Holdover

// Data for combining function
static UINT8 	mchan = BOTH; //Need to support forward reverse or both
static UINT8 	psteer = 1; //Use phase shifter
static INT32 min_samp_forward[264], min_samp_reverse[264]; //change from 32 to 64 sept 2009
static UINT16 min_samp_phase = 0;
static UINT16 min_samp_phase_flag=2;//alert first pass through buffer
static INT32  sdrift_min_forward,	sdrift_min_reverse/*, test_modulation*/;
//static INT32 cdline_f[120], sdrift_min_forward_delay; 
//static INT32 cdline_r[120], sdrift_min_reverse_delay; 
#if	(CROSSOVER_TS_GLITCH==1)
static INT32 prev_offset_gd_f;
static INT32 prev_offset_gd_r;
#endif	
#if (FREEZE_MOD ==1)
static double freeze_mod=2.0; //2 ppb modulation
#endif
static INT32  min_samp_forward_prev,	min_samp_reverse_prev;
static INT32  delta_forward,delta_reverse;
static INT32  delta_forward_prev,delta_reverse_prev;
double dynamic_forward=MIN_DELTA_POP*MIN_DELTA_POP;
double dynamic_reverse=MIN_DELTA_POP*MIN_DELTA_POP;
static UINT8  delta_forward_valid, delta_reverse_valid;
//static UINT16  delta_forward_slew, delta_reverse_slew;// dynamic slew control active
//static UINT16  delta_forward_slew_cnt, delta_reverse_slew_cnt;// dynamic slew control active
static UINT8  delta_forward_skip, delta_reverse_skip;// dynamic slew control active
static UINT8  weight_forward_flag, weight_reverse_flag;
//double forward_weight, reverse_weight;
double nfreq_forward,nfreq_reverse;
static UINT8  mode_compression_flag=0;// auto mode by default Force to zero for ALU build 0 
// end min selection
//Data for holdover routine
double holdover_buffer_f[40],hprev_f;
static UINT8 hold_phase_f;
double holdover_buffer_r[40],hprev_r;
static UINT8 hold_phase_r;
//static UINT16 hcount=0; 
//OCXO related parameters for holdover
//static double G1_cur_OCXO=(0.01667);	  //60 sec proportional 
//static double G2_cur_OCXO=(1.85185e-5);   //900 sec integral 
//static double cur_gear=GEAR_OCXO;
//static INT8 gspeed=0;
//end data for holdover routine

//static INT32	stilt,sdrift_prev,sbuf[2],stemp;
static double 	sdrift_cal_forward,sdrift_cal_reverse,sacc_f=ZEROF,sacc_r=ZEROF,dtemp;
//static double   sacc_f_last=ZEROF, sacc_r_last=ZEROF;
//static double   sacc_f_cor=ZEROF, sacc_r_cor=ZEROF;
static int		pop_flag=0;
//static double	tgain=PER_01; //initial gain for tilt filter was PER_05
//static double tgain_min=0.0001;
//static int		hcomDbgOutput;
//static double	pll_int_f = ZEROF;
//static double	pll_int_r = ZEROF;
static long int	sdrift;
static UINT32	drift;
static UINT32	positive;
static UINT32	absdrift;
static UINT32	abspositive;
static int		firstTime = 256,fbi = 0,fbj,sindx=0,idletime,idle;
//static int 		start_in_progress = TRUE;
UINT8 	PTP_Seq_Num_prev;
UINT16 	PTP_Seq_Num_curr;
UINT16 		Num_of_missed_pkts = 0; 
//static int		train_on = 0 ;
//static int		bSync, bFollowUp;
//static UINT16	lastSeqId;
//static UINT32 	prevOrigSec;
//static UINT32 	PrevDelaySeconds;
//static UINT32 	PrevDelayNanoSeconds;
//static UINT32 	DatumNanoSeconds;
//static INT32 	AccumRate;
//static UINT32 		FLL_Elapse_Sec_Count = 0;
//static UINT32 		FLL_Elapse_Sec_Count_Use;
static UINT32  number_of_packets = 0;
UINT16  number_of_sync_packets[2] = {0, 0};
UINT16  number_of_delay_packets = 0;
UINT16  number_of_sync_packets_for_timeout[2] = {0, 0};
UINT16  number_of_delay_packets_for_timeout = 0;
UINT16  number_of_sync_RTD_filtered = 0;
UINT16  number_of_delay_RTD_filtered = 0;
static UINT32 TickCount = 0;
static UINT16  Print_Phase=0;
static UINT16  Minute_Phase=0;
//static UINT16  boost_cnt=0;
//LWEVENT_STRUCT Ptp_Delay_Req_Event;
//static UINT32 client_ip_address ;
//static	UINT32  offset_comp;
//static	UINT32  phase_adj_ns;
static double time_bias, time_bias_base/*,time_bias_boost,boost_offset*/,	time_bias_offset;
//static double freq_var,freq_mean;
static double time_gain = 0.1;  //Was 0.5
static double lcomp_f,lcomp_r,hcomp_f,hcomp_r;
//static double dchan_err;
static unsigned int exception_gate; //paces debug reporting
// operational configuration variables
static 	INT32	sacc_thres_oper = 50000;
static 	double	diff_chan_err_oper= 30000.0;
static double lsf_smooth_g1 = 0.001; //was 0.005 decrease to 0.001 March 2010
static double lsf_smooth_g2 = 0.999;
static double nfreq_smooth_g1 = 0.015; //GPZ March 2010 generalize nfreq smoothing filter
static double nfreq_smooth_g2 = 0.985;

static 	INT32	detilt_time_oper=1024;
static	double phy_correct= 215.0/2.0;
static int lsf_window;

//static UINT8 timestamp_align_flag=0; // TEST SET To ZERO for release GPZ allows control of timestamp alignment

//static UINT16 current_index, prev_index;
//static INT16 timestamp_align_skip;

static PTP_State state = PTP_START;
static UINT16 counter_state = 0;
static UINT16 gm_index = GRANDMASTER_PRIMARY;
//static UINT32 cnt = 0;
//static BOOLEAN time_sync_once = FALSE;


static UINT16 measCnt[NUM_OF_SERVO_CHAN];
static SC_t_servoChanSelectTableType ChanSelectTable;
static BOOLEAN major_ts_change = FALSE;
static BOOLEAN go_to_bridge = FALSE;

static UINT32 reference_holdoff_counter[NUM_OF_SERVO_CHAN];
static BOOLEAN prev_chan_valid[NUM_OF_SERVO_CHAN];
static t_servoChanAcceptType s_servoLocalChanAccept;
static void init_holdoff(void);
static void Update_Reference_Holdoff(void);

//static void Get_and_Clear_Pkt_Counters(UINT16 *sync_cntr1, UINT16 *sync_cntr2, UINT16 *delay_cntr);
//static void msgPackDelayReq(UINT32 *sock, UINT16 seq_num);
void Process_PTP_Offset(UINT32 OffsetNanoSeconds, 
						UINT32 CurrDelayNanoSeconds, 
						UINT32 origNumberOfSeconds,
						UINT32 CurrDelaySeconds);
void Set_Seq_Num(UINT16 seq_num);
//static UINT16 Get_Seq_Num(void);
// GPZ added functions//
static void fll_onesec(void);
//static void snapshot(void);
//static void get_min_offset(FIFO_STRUCT *,MIN_STRUCT *, INT32, int);
//static void update_input_stats(void);
static boolean time_has_been_set_once = FALSE;

static void fll_update(void);
static void fll_print(void);
static void fll_synth(void);
static void ofd_print(void);
static void forward_tdev_print(void);
static void reverse_tdev_print(void);
static void update_phase(void);
static void update_drive(void);
static void combine_drive(void);
static void combiner_print(void);
static void holdover_forward(double);
//static void holdover_reverse(double);

//static void ocxo_gain(void);
static void init_fll(void);
//static void init_pshape_long(void);
static void fll_status(void);
static void update_time(void);
//static int mode_compress_update(void);
void set_mode_compression_flag(int flag);
void clr_residual_phase(void);
void Restart_Phase_Time(void);
void update_se(void);
void validate_se(void);
//static void init_se(void);
//static void start_se(double);
#if (ASYMM_CORRECT_ENABLE==1)
static void init_acl();
static void update_acl();
#endif				


static void cal_for_loop(void);
static void cal_rev_loop(void);
static void set_exc_gate(INT16);
void clr_exc_gate(INT16);
static int read_exc_gate(INT16);
void set_oper_config(void);
static void Local_Minute_Task(void);
//static void Local_16s_Task(void);
//void External_Minute_Task(void);
void sync_xfer_8s(void);
//static void Write_Vcal_to_file(void);
//static void Read_Vcal_from_file(void);
//static double Varactor_Warp_Routine(double fin);
void Varactor_Cal_Routine(void);
//static t_loTypeEnum OCXO_Type(void);
static void Phase_Test_Update(void);
static void Freq_Test_Update(void);
void Restart_Phase_Time_now(void);
static double getfmeas(void);
void Varactor_Cal_Routine(void);
//double Varactor_Warp_Routine(double);
UINT16 Is_Sync_timeout(UINT8);
UINT64 GetTime(void);
static void select_channel(void);

static int SC_FrequencyCorrection_Local(FLOAT64 *pff_freqCorr, t_paramOperEnum b_operation);
static int SC_PhaseOffset_Local(INT64 *pll_phaseOffset, t_paramOperEnum b_operation);
//Define macros for the local versions in case we forget...
#define SC_FrequencyCorrection(x, y) SC_FrequencyCorrection_Local((x), (y))
#define SC_PhaseOffset(x, y) SC_PhaseOffset_Local((x), (y))
static int Jam_Clear(void);


#if(NO_PHY==0)
void Get_GPS_Offset(INT32 *i32_PhaseOffset)
{
//	INT32 i32_dummy;	

//	SC_GetPhasorPhaseOffset(GPS_PHASOR_MUX, i32_PhaseOffset);
//	SC_GetPhasorPhaseOffset(SYNCE1_PHASOR_MUX, &i32_dummy);
//	SC_GetPhasorPhaseOffset(SYNCE2_PHASOR_MUX, &i32_dummy);

	return;
}
#endif
static BOOLEAN is_PTP_mode = TRUE;

BOOLEAN Is_in_PTP_mode(void)
{
   return is_PTP_mode;
}

BOOLEAN Is_in_PHY_mode(void)
{
   return !is_PTP_mode;
}

BOOLEAN Is_Major_Time_Change(void)
{
	return major_ts_change;
}

BOOLEAN Is_PTP_Disabled(UINT16 index)
{
   if(index == GRANDMASTER_PRIMARY)
   {
      return !Is_GPS_OK(RM_GM_1);
   }
   else
   {
      return !Is_GPS_OK(RM_GM_2);
   }
}

BOOLEAN Is_GPS_OK(RM_State id)
{
//#if(GPS_ENABLE==1)
   if(ChanSelectTable.s_servoChanSelect[id].o_valid)
   {
//      debug_printf(GEORGE_PTP_PRT, "Is_GPS1_OK() = TRUE\n");   
      return TRUE;
   }
   else
      return FALSE;
//#else
//	return(FALSE);
//#endif
}

BOOLEAN Is_Active_Time_Reference_OK(void)
{
   BOOLEAN ret_val = FALSE;

  	if(rms.time_source[0]== RM_GPS_1)
   {
      if(Is_GPS_OK(RM_GPS_1))
      {
		   ret_val = TRUE;
      }
   }
//jiwang
  	if(rms.time_source[0]== RM_GPS_2)
   {
      if(Is_GPS_OK(RM_GPS_2))
      {
		   ret_val = TRUE;
      }
   }
  	if(rms.time_source[0]== RM_GPS_3)
   {
      if(Is_GPS_OK(RM_GPS_3))
      {
		   ret_val = TRUE;
      }
   }
  	if(rms.time_source[0]== RM_RED)
   {
      if(Is_GPS_OK(RM_RED))
      {
		   ret_val = TRUE;
      }
   }
   else if( (rms.time_source[0]== RM_GM_1) || (rms.time_source[0]== RM_GM_2))
   {
      if (!Is_Sync_timeout(gm_index) && 
            Is_GM_Lock_Holdover(gm_index) &&
          !Is_Delay_timeout())
      {
         ret_val = TRUE;
      }
	}

   return ret_val;
}

BOOLEAN Is_Active_Freq_Reference_OK(void)
{
   if(rms.freq_source[0]== RM_GPS_1)
   {
      return(Is_GPS_OK(RM_GPS_1)) ;      
   }
//jiwang
   else if(rms.freq_source[0]== RM_GPS_2)
   {
      return(Is_GPS_OK(RM_GPS_2)) ;      
   }
   else if(rms.freq_source[0]== RM_GPS_3)
   {
      return(Is_GPS_OK(RM_GPS_3)) ;      
   }
   else if(rms.freq_source[0]== RM_SE_1)
   {
      return(Is_GPS_OK(RM_SE_1)) ;      
   }
   else if(rms.freq_source[0]== RM_SE_2)
   {
      return(Is_GPS_OK(RM_SE_2)) ;      
   }
   else if(rms.freq_source[0]== RM_RED)
   {
      return(Is_GPS_OK(RM_RED)) ;      
   }
   else if( (rms.freq_source[0]== RM_GM_1) || (rms.freq_source[0]== RM_GM_2))
   {

      if (!Is_Sync_timeout(gm_index) && 
         Is_GM_Lock_Holdover(gm_index) &&
         !Is_Delay_timeout())
      {
         return(TRUE);
      }
      else
      {
         return(FALSE);
	   }
   }
   else
   {
	printf("77777%s: unknown freq source....\n", __FUNCTION__);
   	return(FALSE);
   }
}

BOOLEAN Is_Active_Phy_Freq_Reference_OK(void)
{
   if(rms.freq_source[0]== RM_GPS_1)
   {
      return(Is_GPS_OK(RM_GPS_1)) ;      
   }
//jiwang
   else if(rms.freq_source[0]== RM_GPS_2)
   {
      return(Is_GPS_OK(RM_GPS_2)) ;      
   }
   else if(rms.freq_source[0]== RM_GPS_3)
   {
      return(Is_GPS_OK(RM_GPS_3)) ;      
   }
   else if(rms.freq_source[0]== RM_SE_1)
   {
      return(Is_GPS_OK(RM_SE_1)) ;      
   }
   else if(rms.freq_source[0]== RM_SE_2)
   {
      return(Is_GPS_OK(RM_SE_2)) ;      
   }
   else if(rms.freq_source[0]== RM_RED)
   {
      return(Is_GPS_OK(RM_RED)) ;      
   }
   else
   {
   	return(FALSE);
   }
}

BOOLEAN Is_Any_Freq_Reference_OK(void)
{
//	if( Is_GPS_OK(RM_GPS_1) || Is_GPS_OK(RM_GPS_2) || ( !Is_Sync_timeout(gm_index) && !Is_Delay_timeout() && Is_GM_Lock_Holdover(gm_index)) )
	if( Is_GPS_OK(RM_GPS_1) || 
//      ( !Is_Sync_timeout(gm_index) && !Is_Delay_timeout() && Is_GM_Lock_Holdover(gm_index)) ||
      Is_GPS_OK(RM_GPS_2) ||//jiwang
      Is_GPS_OK(RM_GPS_3) ||//jiwang
	  Is_GPS_OK(RM_GM_1) ||
      Is_GPS_OK(RM_SE_1) ||
      Is_GPS_OK(RM_SE_2) ||
      Is_GPS_OK(RM_RED))
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}

BOOLEAN Is_PTP_FREQ_ACTIVE(void)
{
	if( (rms.freq_source[0]== RM_GM_1) || (rms.freq_source[0]== RM_GM_2) )
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}

BOOLEAN Is_PTP_FREQ_VALID(void)
{
	if(Is_GPS_OK(RM_GM_1) || Is_GPS_OK(RM_GM_2))
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}

BOOLEAN Is_PHY_FREQ_ACTIVE(void)
{
	if((rms.freq_source[0]== RM_GPS_1) || (rms.freq_source[0]== RM_GPS_2) || (rms.freq_source[0]== RM_GPS_3) || (rms.freq_source[0]== RM_SE_1) || (rms.freq_source[0]== RM_SE_2)|| (rms.freq_source[0]== RM_RED))
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}

BOOLEAN Is_PTP_Warm(void)
{
		return (!start_in_progress_PTP);
}

BOOLEAN Is_PTP_TIME_ACTIVE(void)
{
	if( (rms.time_source[0]== RM_GM_1) || (rms.time_source[0]== RM_GM_2) )
	{
// 	   debug_printf(GEORGE_PTP_PRT, "PTP_Time_Active\n");
		return (TRUE);
	}
	else
	{
// 	   debug_printf(GEORGE_PTP_PRT, "PTP_Time_Not_Active: %d\n",rms.time_source[0]);
		return (FALSE);
	}
}

#ifndef TP500_BUILD
//void Get_SE_Offset(INT32 *i32_PhaseOffset)
//{
//   SC_GetSePhaseOffset(i32_PhaseOffset);
//   return;
//}
BOOLEAN Is_GM_Lock_Holdover(UINT8 master)
{
   return TRUE;
}
static BOOLEAN Is_OCXO_Warm(void)
{
   return TRUE;
}
#endif
t_loTypeEnum OCXO_Type(void)
{
   t_loTypeEnum e_loType;

   Get_LO_Type(&e_loType);

   return e_loType;
}
void Start_Phase_Test(void)
{
	Phase_test_flag=1;
#if NO_PHY == 0
   debug_printf(GEORGE_PTP_PRT, "start Phase Test\n");
#endif
	return;
}
void Start_Freq_Test(void)
{
	Freq_test_flag=1;
#if NO_PHY == 0
   debug_printf(GEORGE_PTP_PRT, "start Freq Test\n");
#endif
	return;
}

void Start_FreqZero_Test(int enable)
{
  gSC_TST_FREQ_ZERO = enable?1:0;
#if NO_PHY == 0
  debug_printf(GEORGE_PTP_PRT, "e_SC_TST_FREQ_ZERO: %d\n", gSC_TST_FREQ_ZERO);
#endif
}

static void Phase_Test_Update(void)
{
   static int counter;
   static INT64 dl_data;

   if(counter++ > 30)
   {
      counter = 0;
   }
   if(counter == 10)
   {
      dl_data = 0;
   }
   else if(counter == 20)
   {
      dl_data = 100000;
   }
   else if(counter == 0)
   {
      dl_data = -100000;
   }

   SC_PhaseOffset_Local(&dl_data, e_PARAM_SET);
#if NO_PHY == 0
   debug_printf(GEORGE_PTP_PRT, "%d phase set to %ld ns\n", counter, (INT32)dl_data);
#endif			
}

static void Freq_Test_Update(void)
{
   static FLOAT64 df_freqCorr = -512.0;
   static int counter = 0;
   INT64 dl_phase = 0;

   if(counter++ > 30 * 16)
   {
      counter = 0;
      df_freqCorr = -512.0;
   }
   if((counter % 30) == 29)
   {
      df_freqCorr = df_freqCorr + 64;
   }
   SC_FrequencyCorrection_Local(&df_freqCorr, e_PARAM_SET);
 
   SC_PhaseOffset_Local(&dl_phase, e_PARAM_SET); // make sure phase offset is 0

// Jan 26   debug_printf(GEORGE_PTP_PRT, "%d freq set to %e ns\n", counter, df_freqCorr);
			
}

UINT32 Get_Tick(void)
{
   return tick_counter;
}

#if 0
static void Set_PPS_Offset(INT64 offset, int jam_flag)
{
     INT64 ll_off_temp;
//   if(jam_flag)
//   {
//      pps_jam_offset = offset;
//      ll_off_temp = -local_pps_offset + (INT64)offset;
      ll_off_temp = (INT64)offset;
//      if(ll_off_temp < 0)
//      {
//         ll_off_temp = ll_off_temp % 1000000000;
//      }
      offset = ll_off_temp;

      SC_PhaseOffset_Local(&offset, e_PARAM_SET);
}
#endif

#ifndef TP500_BUILD
char *Get_FLL_State_str(unsigned long int state)
{
	switch(state)
	{
	case FLL_UNKNOWN:
		return "Power on";
		break;
	case FLL_WARMUP:
		return "Warmup";
		break;
	case FLL_FAST:
		return "Fast FLL";
		break;
	case FLL_NORMAL:
		return "Normal FLL";
		break;
	case FLL_BRIDGE:
		return "Bridging";
		break;
	case FLL_HOLD:
		return "Holdover";
		break;
	default:
		return "";		
	}
}

void Send_Log (UINT16 log_type, UINT16 log_entry_num, UINT32 ulparam1, UINT32 ulparam2, UINT16 reserved) {
	char buf[255];
	char *str1_ptr,*str2_ptr;
	float *f_ptr;
	int param1=(int)ulparam1;
	int param2=(int)ulparam2;
	switch( log_entry_num ) {
		case LOG_TYPE_POWER_ON:
			sprintf(buf, "Power On");
			break;
		case LOG_TYPE_STATE_CHANGE:
			str1_ptr = Get_FLL_State_str(param1);
			str2_ptr = Get_FLL_State_str(param2);
			
			sprintf(buf, "FLL state change from %s to %s", str1_ptr, str2_ptr);
			break;
#if 0
		case LOG_TYPE_ALARM:
			if(reserved >= NUM_OF_HM)
			{
				reserved = 0;
			}
			sprintf(buf, "Enter event %d - %s", reserved, health_mon_lst[reserved].test_dsc_str );
			break;
		case LOG_TYPE_ALARM_EXIT:
			if(reserved >= NUM_OF_HM)
			{
				reserved = 0;
			}
			sprintf(buf, "Exit event %d - %s", reserved, health_mon_lst[reserved].test_dsc_str);
			break;
		case LOG_TYPE_LOGIN:
			sprintf(buf, "User logged in (%s)",param1?"Telnet":"CLI");
			break;
		case LOG_TYPE_LOGOUT:
			sprintf(buf, "User logged out (%s)",param1?"Telnet":"CLI");
			break;
#endif
		case LOG_TYPE_REF_CHANGE:
			sprintf(buf, "Reference change to %s",param1?"Acc-GM":"GM");
			break;
		case LOG_TYPE_CHANGE_SETTING:
			switch(reserved)
			{
			case LOG_CH_LOG_CLEAR:
				sprintf(buf, "Logs cleared");			
				break;
#if 0
			case LOG_CH_SET_DHCP:
				sprintf(buf, "IP-mode set to %s", param1?"DHCP":"static");
				break;
			case LOG_CH_OUTPUT_TYPE:
				switch(param1)
				{
				case OUTPUT_TYPE_E1_CCS:
					sprintf(buf, "Output set to E1 CCS");
					break;
				case OUTPUT_TYPE_E1_CCS4:
					sprintf(buf, "Output set to E1 CCS with CRC4");
					break;
				case OUTPUT_TYPE_E1_CAS:
					sprintf(buf, "Output set to E1 CAS");
					break;
				case OUTPUT_TYPE_E1_CAS4:
					sprintf(buf, "Output set to E1 CAS with CRC4");
					break;
				case OUTPUT_TYPE_E1_AIS:
					sprintf(buf, "Output set to E1 AIS");
					break;
				case OUTPUT_TYPE_E1_2048M:
					sprintf(buf, "Output set to E1 2.048 MHz");
					break;
				case OUTPUT_TYPE_T1_SF:
					sprintf(buf, "Output set to T1 SF");
					break;
				case OUTPUT_TYPE_T1_ESF:
					sprintf(buf, "Output set to T1 ESF");
					break;
				case OUTPUT_TYPE_T1_ESF_SSM:
					sprintf(buf, "Output set to T1 ESF with SSM");
					break;
				case OUTPUT_TYPE_T1_AIS:
					sprintf(buf, "Output set to T1 with AIS");
					break;
				case OUTPUT_TYPE_T1_ISO_PULSE:
					sprintf(buf, "Output set to T1 isolated pulse");
					break;
				case OUTPUT_TYPE_1544M:
					sprintf(buf, "Output set to T1 1.544 MHz");		
					break;
				case OUTPUT_TYPE_DISABLE:
					sprintf(buf, "Output is disabled");
					break;
				default:
					sprintf(buf, "Output is unknown");
					break; 
				}
				break;
			case LOG_CH_HDB3:
				sprintf(buf, "HDB3 set to %s", param1?"enable":"disable");
				break;
			case LOG_CH_EIA232_PARITY:
				switch(param1)
				{
				case IO_SERIAL_PARITY_NONE:
					sprintf(buf, "EIA-232 parity set to none");
					break; 
				case IO_SERIAL_PARITY_ODD:
					sprintf(buf, "EIA-232 parity set to odd");
					break; 
				case IO_SERIAL_PARITY_EVEN:
					sprintf(buf, "EIA-232 parity set to even");
					break; 
				default:
					sprintf(buf, "EIA-232 parity set to unknown");
					break; 
				}
				break;
			case LOG_CH_SER_NUM:
				sprintf(buf, "Serial number set");
				break;
			case LOG_CH_LOG_PRESET:
				switch(param1)
				{
				case PRESET_DEFAULTS:
					sprintf(buf, "Configuration set to default");
					break; 
				case FACTORY_DEFAULTS:
					sprintf(buf, "Configuration set to factory");
					break; 
				default:
					sprintf(buf, "Configuration set to unknown");
					break; 
				}
				break;
#endif
			case LOG_CH_SET_FDRIFT:
				f_ptr = (float *)&param1;
				sprintf(buf, "Fdrift set to %f", *f_ptr);
				break;
#if 0
			case LOG_CH_SET_TIME:
				time.SECONDS = param1;
				time.MILLISECONDS = 0;
				_time_to_date(&time, &date);
				sprintf(buf, "Time was %02hd:%02hd:%02hd %02hd/%02hd/%04hd",	
					date.HOUR,
					date.MINUTE,
					date.SECOND,
					date.MONTH,
					date.DAY,
					date.YEAR);
				break;
#endif
			case LOG_CH_SET_ALM_ENABLE:
				if(param1 == LOG_CH_SET_ALM_ALL)
				{
					sprintf(buf, "All alarms %s", param2?"enabled":"disabled");
				}
				else
				{
					sprintf(buf, "Alarm %d %s", param1, param2?"enabled":"disabled");
				}
				break;	
#define ALARM_EVENT 0
#define ALARM_MINOR 1
#define ALARM_MAJOR 2
#define ALARM_CRITICAL 3

			case LOG_CH_SET_ALM_LEVEL:
				if(param1 == LOG_CH_SET_ALM_ALL)
				{
					switch(param2)
					{
					case ALARM_EVENT:
						sprintf(buf, "All alarm levels set to event");
						break;
					case ALARM_MINOR:
						sprintf(buf, "All alarm levels set to minor");
						break;
					case ALARM_MAJOR:
						sprintf(buf, "All alarm levels set to major");
						break;
					case ALARM_CRITICAL:
						sprintf(buf, "All alarm levels set to critical");
						break;
					default:
						sprintf(buf, "All alarm levels set to unknown");
						break;
					}
				}
				else
				{
					switch(param2)
					{
					case ALARM_EVENT:
						sprintf(buf, "Alarm %d level set to event", (int)param1);
						break;
					case ALARM_MINOR:
						sprintf(buf, "Alarm %d level set to minor", (int)param1);
						break;
					case ALARM_MAJOR:
						sprintf(buf, "Alarm %d level set to major", (int)param1);
						break;
					case ALARM_CRITICAL:
						sprintf(buf, "Alarm %d level set to critical", (int)param1);
						break;
					default:
						sprintf(buf, "Alarm %d level set to unknown", (int)param1);
						break;
					}
				}
				break;	
			case LOG_CH_SET_ALM_DELAY:
				if(param1 == LOG_CH_SET_ALM_ALL)
				{
					sprintf(buf, "All alarm delays set to %d", (int)param2);
				}
				else
				{
					sprintf(buf, "Alarm %d delay set to %d", (int)param1, (int)param2);
					break;
				}
				break;					
			case LOG_CH_IP:
				switch(param1)
				{
				case 0:
					sprintf(buf, "IP address set to %d.%d.%d.%d", param2 >> 24,
  						(param2 >> 16) & 0xFF,
  						(param2 >> 8) & 0xFF,
  						param2 & 0xFF);
					break;
				case 1:
					sprintf(buf, "IP mask set to %d.%d.%d.%d", param2 >> 24,
  						(param2 >> 16) & 0xFF,
  						(param2 >> 8) & 0xFF,
  						param2 & 0xFF);
					break;
				case 2:
					sprintf(buf, "IP gateway set to %d.%d.%d.%d", param2 >> 24,
  						(param2 >> 16) & 0xFF,
  						(param2 >> 8) & 0xFF,
  						param2 & 0xFF);
					break;
				default:
					sprintf(buf, "IP set to unknown");
					break; 
				}
				break;		
			case LOG_CH_MASTER:
				sprintf(buf, "Master IP set to %d.%d.%d.%d", param1 >> 24,
					(param1 >> 16) & 0xFF,
					(param1 >> 8) & 0xFF,
					param1 & 0xFF);
				break;
			case LOG_CH_ACC_MASTER:
				sprintf(buf, "Acceptable master IP set to %d.%d.%d.%d", param1 >> 24,
					(param1 >> 16) & 0xFF,
					(param1 >> 8) & 0xFF,
					param1 & 0xFF);
				break;
			case LOG_CH_LEASE_DUR:
				sprintf(buf, "Lease duration set to %d" , param1);
				break;
			case LOG_CH_SYNC_INTV:
				sprintf(buf, "Sync rate set to log2(%d) pkts/s" , param1);
				break;
			case LOG_CH_DEL_INTV:
				sprintf(buf, "Delay request rate set to log2(%d) pkts/s" , param1);
				break;
			case LOG_CH_INIT_SYNC_INTV:
				sprintf(buf, "Initial sync rate set to log2(%d) pkts/s" , param1);
				break;
			case LOG_CH_ANNOUNCE_INTV:
				sprintf(buf, "Announce period set to %d s" , (1 << param1));
				break;
			case LOG_CH_CLOCK_ID:
				sprintf(buf, "Clock id has been set");
				break;
			case LOG_CH_MAC:
				sprintf(buf, "MAC address has been set");
				break;
			case LOG_CH_EIA232_DATA:
				sprintf(buf, "EIA-232 data has been set to %d bits", param1);
				break;	
#if 0
			case LOG_CH_EIA232_BAUD:
				switch(param1)
				{
				case BAUD_9600:
					sprintf(buf, "EIA-232 baud has been set to 9600");
					break;
				case BAUD_19200:
					sprintf(buf, "EIA-232 baud has been set to 19200");
					break;
				case BAUD_38400:
					sprintf(buf, "EIA-232 baud has been set to 38400");
					break;
				case BAUD_57600:
					sprintf(buf, "EIA-232 baud has been set to 57600");
					break;
				default:
					sprintf(buf, "EIA-232 baud has been set to unknown");
					break;
				}
				break;
#endif
			case LOG_CH_VLAN_ID_PRI:
				sprintf(buf, "VLAN ID set to %d and priority to %d", param1, param2);
				break;	
			case LOG_CH_VLAN_ENABLE:
				sprintf(buf, "VLAN set to %s", param1?"enable":"disable");
				break;	
			case LOG_CH_LOGIN_PASSWD:
				sprintf(buf, "Login name and/or password has been changed");
				break;	
			case LOG_CH_FIREWALL_TELNET:
				sprintf(buf, "Telnet set to %s", param1?"block":"allow");
				break;
			case LOG_CH_FIREWALL_FTP:
				sprintf(buf, "TFTP set to %s", param1?"block":"allow");
				break;
			case LOG_CH_FIREWALL_ICMP:
				sprintf(buf, "ICMP set to %s", param1?"block":"allow");
				break;
#if 0
			case LOG_CH_OUTP_FREE_MODE:
				switch(param1)
				{
				case OUTPUT_ON:
					str1_ptr = "on";
					break;
				case OUTPUT_SQUELCH:
					str1_ptr = "squelch";
					break;
				case OUTPUT_AIS:
					str1_ptr = "AIS";
					break;
				default:
					str1_ptr = "unknown";					
					break;
				}
				sprintf(buf, "Freerun output mode has been set to %s", str1_ptr);
				break;
			case LOG_CH_OUTP_HOLD_MODE:
				switch(param1)
				{
				case OUTPUT_ON:
					str1_ptr = "on";
					break;
				case OUTPUT_SQUELCH:
					str1_ptr = "squelch";
					break;
				case OUTPUT_AIS:
					str1_ptr = "AIS";
					break;
				default:
					str1_ptr = "unknown";					
					break;
				}
				sprintf(buf, "Holdover output mode has been set to %s", str1_ptr);
				break;
#endif
			case LOG_CH_BRIDGE_TIME:
				sprintf(buf, "Bridging time set to %d" , param1);
				break;			
			case LOG_CH_PTP_ENABLE_TYPE:
				sprintf(buf, "PTP State set to %s" , param1?"enable":"disable");
				break;			
			case LOG_CH_DSCP:
				sprintf(buf, "DSCP set to %d" , param1);
				break;			
			case LOG_CH_DOMAIN:
				sprintf(buf, "Domain set to %d" , param1);
				break;			
			default:
				break;
			}
			break;
		case LOG_TYPE_EXCEPTION:
			switch(reserved)
			{	
			case LOG_EXC_NO_SYNC :
				sprintf(buf, "Sync Packets Stopped");
				break;
			case LOG_EXC_SYNC_START :
				sprintf(buf, "Sync Packets Started");
				break;
			case LOG_EXC_NO_DELAY :
				sprintf(buf, "Delay Packets Stopped");
				break;
			case LOG_EXC_DELAY_START :
				sprintf(buf, "Delay Packets Started");
				break;
			case LOG_EXC_FLL_START :
				sprintf(buf, "FLL Settle Start");
				break;
			case LOG_EXC_FLL_COMPLETE :
				sprintf(buf, "FLL Settle Stop");
				break;
			case LOG_EXC_EXCESSIVE_DIFF_CHAN_ERR :
				sprintf(buf, "Excessive Differential Channel Error");
				break;
			case LOG_EXC_SYNC_LOOP_PHASE_RANGE: 
				sprintf(buf, "Sync Loop Phase Range Error");
				break;
			case LOG_EXC_DELAY_LOOP_PHASE_RANGE: 
				sprintf(buf, "Delay Loop Phase Range Error");
				break;
			case LOG_EXC_RECENTER_QUIET_TIMEOUT: 
				sprintf(buf, "Recenter Phase Quiet Timeout");
				break;
			case LOG_EXC_RECENTER_BRIDGING: 
				sprintf(buf, "Recenter Phase Bridging");
				break;
			case LOG_EXC_RECENTER_RANGE: 
				sprintf(buf, "Recenter Phase Range");
				break;
			case LOG_EXC_RESHAPE_USE_NEW_BASE: 
				sprintf(buf, "Reshape use new base");
				break;
			case LOG_EXC_RESHAPE_USE_OLD_BASE: 
				sprintf(buf, "Reshape use old base");
				break;
			case LOG_EXC_RECENTER_QUIET_RANGE: 
				sprintf(buf, "Recenter Quiet Range");
				break;
			case LOG_EXC_SYNC_POP_DETECTED: 
				sprintf(buf, "Sync Pop Detected");
				break;
			case LOG_EXC_SYNC_RANGE_DETECTED: 
				sprintf(buf, "Sync Range Detected");
				break;
			case LOG_EXC_SYNC_SLEW_DETECTED: 
				sprintf(buf, "Sync Slew Detected");
				break;
			case LOG_EXC_DELAY_POP_DETECTED: 
				sprintf(buf, "Delay Pop Detected");
				break;
			case LOG_EXC_DELAY_RANGE_DETECTED: 
				sprintf(buf, "Delay Range Detected");
				break;
			case LOG_EXC_DELAY_SLEW_DETECTED: 
				sprintf(buf, "Delay Slew Detected");
				break;
			case LOG_EXC_FIT_FREQ_EST:
				sprintf(buf, "Frequency State Estimates sync ps/s: %8ld, delay ps/s: %8ld",
				(long int)(param1),
				(long int)(param2)
				);
				break;
			case LOG_EXC_FLOOR_BIAS_EST:
				sprintf(buf, "Floor Bias Estimates sync ns: %8ld, delay ns: %8ld",
				(long int)(param1),
				(long int)(param2)
				);
				break;
			case LOG_EXC_FLOOR_TDEV_EST:
				sprintf(buf, "Floor Oper TDEV ns: %8ld, delay ns: %8ld",
				(long int)(param1),
				(long int)(param2)
				);
				break;
				
			default:
				break;
			}
			break;
		case LOG_TYPE_TIME_SET:
#if 0
			time.SECONDS = param1;
			time.MILLISECONDS = 0;
			_time_to_date(&time, &date);
			sprintf(buf, "PTP time set, time was %02hd:%02hd:%02hd %02hd/%02hd/%04hd",	
				date.HOUR,
				date.MINUTE,
				date.SECOND,
				date.MONTH,
				date.DAY,
				date.YEAR);
#endif
			sprintf(buf, "PTP time set, time was %d seconds Unix time\n",param1);
			break;
      case LOG_TYPE_RESTART_PHASE:
         sprintf(buf, "Phase realigned");
         break;
		default:
			sprintf(buf, "Unknown log entry %hd", log_entry_num);
			break;
	}
	

}
#endif



void Get_Flow_Status(UINT16 *sync, UINT16 *delay, UINT8 master)
{
	*sync = sync_cntr;
	*delay = delay_cntr;
}

UINT32 Get_number_packets(void)
{
	UINT32 num = number_of_packets;
	
	number_of_packets = 0;
	return num;
}
UINT16 Is_Sync_timeout(UINT8 master)
{
//#if (LOW_PACKET_RATE_ENABLE == 1)
//   if(sync_cntr < 1)
//      return TRUE;
//#else
   if(Is_Sync_LOS())
     return TRUE;
   else if(sync_cntr < 1)
      return TRUE;
//#endif
   else
      return FALSE;
}
UINT16 Is_Delay_timeout(void) //based on 64 rate
{
//#if (LOW_PACKET_RATE_ENABLE == 1)
//   if(delay_cntr < 1)
//      return TRUE;
//#else
   if(Is_Sync_LOS())
      return TRUE;
   else if(delay_cntr < 1)
      return TRUE;
//#endif

   else
      return FALSE;
}

int Restart_FLL(void);
int Restart_Client(void);
int measmode(int);
//int setgear(int );
int exittilt(void);
int get_fll_config(void);
void set_floor(int);
void set_fll(int);
void set_tdev(int);
void set_combiner(int);
int set_min_cluster(INT32);

static int floor_flag=0;
static int fll_flag=1;
static int tdev_flag=0;
static int combiner_flag=1;
static UINT16 seq_num_from_last_sync_pkt;
void Set_Seq_Num(UINT16 seq_num)
{
	seq_num_from_last_sync_pkt = seq_num;	
}

#if 0
static UINT16 Get_Seq_Num(void)
{
	return seq_num_from_last_sync_pkt;	
}
#endif

boolean Is_Sync_Flow_Ack(UINT8 master, int clear_flag)
{
	boolean flag = signaling_sync_ack_flag[master];
	
	if(clear_flag == CLEAR_SYNC_ACK_FLAG)
	{
		signaling_sync_ack_flag[master] = 0;
	}
	return flag;
}

boolean Is_Delay_Flow_Ack(UINT8 master, int clear_flag)
{
	boolean flag = signaling_delay_ack_flag[master];
	
	if(clear_flag == CLEAR_SYNC_ACK_FLAG)
	{
		signaling_delay_ack_flag[master] = 0;
	}
	return flag;
}

boolean Is_Announce_Flow_Ack(UINT8 master, int clear_flag)
{
	boolean flag = signaling_announce_ack_flag[master];
	
	if(clear_flag == CLEAR_SYNC_ACK_FLAG)
	{
		signaling_announce_ack_flag[master] = 0;
	}
	return flag;
}


///////////////////////////////////////////////////////////////////////
// This function is used to process the SYNC and DELAY_RESP Fifos once
// a second. 
///////////////////////////////////////////////////////////////////////

#define SET_RED_LED_ON 	mcf5282_ptr->GPIO.PPDSDR_CS = 0x20;
#define SET_RED_LED_OFF	mcf5282_ptr->GPIO.PCLRR_CS  = 0xDF;
int SC_RunOneSecTask(void)
{
	INT64 ll_dphase;
	double f_offset;
	UINT16 index_forward;
	UINT16 index_reverse;
	UINT16 fll_start;

   UINT64 start_time, end_time;

   /* if SC_InitChanConfig() has not been successfully run at least once,
      checked for the servo only case in which SC_InitConfigComplete() is not run */
   if (!Is_ChanConfigured())
      return -2;

   start_time = GetTime();
   if(tick_counter == 0)
     CLK_Init_PTP_Tranfer_Queue();

	tick_counter++;

   Update_Reference_Holdoff();
	Update_chanSelect();
   SC_SyncPhaseThreshold(e_PARAM_GET, &jam_threshold);
//   printf("sync threshold %d\n", (int)jam_threshold);
//	restart_phase_time_flag = FALSE;

//	Set_fdrift(nvm.nv_fdrift);
//	CLK_Init_PTP_Tranfer_Queue();

//	_time_delay_ticks(TICKS_PER_SEC * 5);

//	while(1)
	{
//		_time_delay_ticks(TICKS_PER_SEC);
/* wait for PPS event*/
//		_lwevent_wait_ticks(&PTP_clock_tick_event, 1, 1, (TICKS_PER_SEC * 2));
#if 1
	printf("******************************************************************\n");
	printf("restart_phase_time_flag = %d\n", restart_phase_time_flag);
	printf("******************************************************************\n");
#endif
		if(restart_phase_time_flag)
		{
			restart_phase_time_flag = FALSE;
			Restart_Phase_Time_now();
		}

//		Get_and_Clear_Pkt_Counters(&sync_cntr, &sync_cntr, &delay_cntr);
//		Reset_RTD();  /* update RTD measurement */

/* start taking temperature data 1 minute after reset */
#ifdef TEMPERATURE_ON
		if((Get_ticks() > 60)  && !Is_Rev1())
		{
			temperature_calc();
		}
#endif
    Get_servoChanSelect(&ChanSelectTable);

    Transfer_PTP_data_now(&PTP_Fifo_Local, &PTP_Delay_Fifo_Local, &sync_cntr, &delay_cntr);
#if (NO_PHY==0)
	  Transfer_Phasor_data_now(SERVO_CHAN_GPS1, &GPS_A_Fifo_Local, &measCnt[0]);
	  if(GPS_A_Fifo_Local.is_offset_flag)
	  	GPS_A_Fifo_Local.is_offset_flag = 0;
//jiwang
	  Transfer_Phasor_data_now(SERVO_CHAN_GPS2, &GPS_B_Fifo_Local, &measCnt[1]);
	  if(GPS_B_Fifo_Local.is_offset_flag)
	  	GPS_B_Fifo_Local.is_offset_flag = 0;
//jiwang
	  Transfer_Phasor_data_now(SERVO_CHAN_GPS3, &GPS_C_Fifo_Local, &measCnt[2]);
	  if(GPS_C_Fifo_Local.is_offset_flag)
	  	GPS_C_Fifo_Local.is_offset_flag = 0;
	  	
	  Transfer_Phasor_data_now(SERVO_CHAN_SYNCE1, &SYNCE_A_Fifo_Local, &measCnt[3]);
	  if(SYNCE_A_Fifo_Local.is_offset_flag)
	  	SYNCE_A_Fifo_Local.is_offset_flag = 0;

	  Transfer_Phasor_data_now(SERVO_CHAN_SYNCE2, &SYNCE_B_Fifo_Local, &measCnt[4]);
	  if(SYNCE_B_Fifo_Local.is_offset_flag)
	  	SYNCE_B_Fifo_Local.is_offset_flag = 0;

	  Transfer_Phasor_data_now(SERVO_CHAN_RED, &RED_Fifo_Local, &measCnt[5]);
	  if(RED_Fifo_Local.is_offset_flag)
	  	RED_Fifo_Local.is_offset_flag = 0;
#endif

			index_forward = PTP_Fifo_Local.fifo_in_index;
			index_reverse = PTP_Delay_Fifo_Local.fifo_in_index;

//      debug_printf(RATE_PRT, "%d sync: %d del: %d s_rate: %d d_rate: %d\n", 
//            counter_state,
//            PTP_Fifo_Local.offset[index_forward],
//            PTP_Delay_Fifo_Local.offset[index_reverse],
//            sync_cntr,
//            delay_cntr);

/* run phase test pattern */
      if(Phase_test_flag)
      {
         Phase_Test_Update();
         return 0;
      }
      if(Freq_test_flag)
      {
         Freq_Test_Update();
         return 0;
      }

/* Freq_Zero_flag is now handled by SC_FrequencyCorrection_Local
      if(Freq_Zero_flag)
      {
	FreqZero_Test_Update();
	return 0;
      }
*/
		switch(state)
		{
		case PTP_START:
//    		Get_Sync_Rate(&ptpRate);	 	//GPZ DEC 2010 
		ptpRate = ChanSelectTable.s_servoChanSelect[SERVO_CHAN_PTP1].w_measRate;
#if NO_PHY == 0
         	debug_printf(GEORGE_PTP_PRT, "ptpRate: %d\n", (int)ptpRate);
#endif
	      fll_status();  /* updates timer in cli status command */
         select_channel();
         ll_dphase = 0;
         SC_PhaseOffset_Local(&ll_dphase, e_PARAM_SET);
//			Set_PPS_Offset(0, TRUE);
			index_forward = PTP_Fifo_Local.fifo_in_index;
			index_reverse = PTP_Delay_Fifo_Local.fifo_in_index;

			if(counter_state == 0) 
			{

            f_offset = ZEROF;
//            SC_FrequencyCorrection_Local(&f_offset, e_PARAM_SET);

//            Read_Vcal_from_file();
				init_fll();
				init_ipp();
//      	   debug_printf(GEORGE_PTP_PRT, "Why_init PTP Start\n"); //TEST TEST TEST

				init_tdev();
				init_ztie();
//				init_fit(); 
				init_rfe(); 
				#if(SYNC_E_ENABLE==1)
				init_se();
				#endif
				init_rm();	
            #if(NO_PHY==0)
				init_ipp_phy();
				init_rfe_phy(); 
				#endif
#if SMARTCLOCK == 1
            	Init_Holdover();
#endif
				#if (ASYMM_CORRECT_ENABLE==1)
				init_acl();
				#endif
				#ifdef TEMPERATURE_ON				
				init_temperature();
				#endif
				#if(!NO_IPPM)				
				init_ippm();
  		     	SC_ClkIpdvConfig(&s_ptpIpdvConfig, e_PARAM_GET);  
				set_ippm_pacing(s_ptpIpdvConfig.w_pacingFactor);
				set_ippm_thres(s_ptpIpdvConfig.l_ipdvThres);
				set_ippm_window(s_ptpIpdvConfig.w_ipdvObservIntv);
#endif
            init_holdoff();

				//Placeholder to launch BesTime init task
//				BesTime_init(0);
			}
			fll_synth();

#if NO_PHY == 0
			debug_printf(GEORGE_PTP_PRT, "%d sync offset: %ld delay offset %ld s: %d d: %d %d\n", 
					counter_state,
					PTP_Fifo_Local.offset[index_forward],
					PTP_Delay_Fifo_Local.offset[index_reverse],
					sync_cntr, 
					delay_cntr,
               (int)Is_PTP_Mode_Inband());
#endif
			/* make sure that there is a viable active time reference before exiting this state */

// Let through if any reference is valid 
//			if (Is_Active_Time_Reference_OK() == FALSE)
//			if (Is_Any_Freq_Reference_OK() == FALSE)
         if (rms.freq_source[0]== RM_NONE)
         {
#ifdef START_WITH_FREQ_ONLY
            if(Is_Active_Freq_Reference_OK())
            {
/* this is the sync-e case, go on */
               state = PTP_SET_PHASE;
            	counter_state = 0;
            	set_phase_counter=0;	
            }
            else
#endif
            {
            	counter_state = 0;
            }
         }
         else
         {
            if ((counter_state > 4))
            {
//#ifdef GPS_BUILD
#if (NO_PHY==0)
               if(rms.time_source[0]== RM_GPS_1 || rms.time_source[0] == RM_GPS_2 || rms.time_source[0] == RM_GPS_3)//jiwang
               {
                  Sync_Using_GPS();
                  Clear_Time_Adjusted_Flag(); //do not throw into holdover mode
               }
               else
               if(rms.time_source[0]== RM_RED)
               {
                  Sync_Using_RED();
                  Clear_Time_Adjusted_Flag(); //do not throw into holdover mode
               }
               else
#endif
               if((rms.time_source[0]== RM_GM_1) || (rms.time_source[0]== RM_GM_2))
               {
                  Sync_Using_PTP();
               }
               

               state = PTP_SET_PHASE;
            	counter_state = 0;
            	set_phase_counter=0;	
            }

         }
 			break;
		case PTP_SET_PHASE:
		    fll_status();  /* updates timer in cli status command */
            select_channel();
#if (NO_PHY==0)   //JAN 26
			debug_printf(GEORGE_PTP_PRT, "%d sync offset: %ld delay offset %ld s: %d d: %d %d\n", 
					counter_state,
					PTP_Fifo_Local.offset[index_forward],
					PTP_Delay_Fifo_Local.offset[index_reverse],
					sync_cntr, 
					delay_cntr);
#endif
			/* get offset */
			if(set_phase_counter==3) 
			{
			
				// get last good freq //
				Get_Freq_Corr(&last_good_freq,&gaptime);
				debug_printf(GEORGE_PTP_PRT, "phase cal: last_good_freq %e gap %ld\n", 
                              (float)last_good_freq,gaptime);
                if(1) //always use for now              
//				if(gaptime < 3000000)
				{
					fdrift_f=fdrift_r=fdrift=fdrift_raw=fdrift_warped=fdrift_smooth=pll_int_r=pll_int_f= holdover_f=holdover_r= (double)(last_good_freq);
					fll_settle=128; //was 256 reduce to 128
				}
	// end get last good freq
			
			
//				Restart_Phase_Time(); //use same routine for all timescale coarse adjsut
//				Set_PPS_Offset(0, FALSE);
			}
			else
			{
//			   	ll_dphase = 0;
//        		SC_PhaseOffset_Local(&ll_dphase, e_PARAM_SET);
			}
 			
			#if 0
			debug_printf(GEORGE_PTP_PRT, "%d offset: %ld reverseoffset %ld phase_adj %ld offset_comp %ld or %lu sync pkts: %d delay_pkts: %d\n", 
//		printf("%d offset: %ld reverseoffset %ld phase_adj %ld sync pkts: %d delay_pkts: %d\n", 
					set_phase_counter,
					Initial_Offset,
					Initial_Offset_Reverse,
					phase_adj_ns,
					(long int)offset_comp,
					offset_comp,
					sync_cntr[gm_index], 
					delay_cntr);
			#endif

/* only run once */
			if(set_phase_counter == 8) 
			{
				init_tdev();
				init_ztie();
            if(Is_Active_Time_Reference_OK())
            {
				   Restart_Phase_Time(); //use same routine for all timescale coarse adjsut
				}
			}
         if(OCXO_Type() >= e_MINI_OCXO)
			{
				fll_start=FLLSTART_TCXO;
			}	
			else
			{
				fll_start=FLLSTART_OCXO;
			}							
			if ((set_phase_counter > fll_start) || (Is_OCXO_Warm() && (set_phase_counter > FLLSTART_TCXO)))
         {
				state = PTP_LOCKED;
            Clear_Time_Adjusted_Flag();
         }

//			if (Is_Active_Freq_Reference_OK() == TRUE) set_phase_counter++;
			if (Is_Any_Freq_Reference_OK() == TRUE) set_phase_counter++;
			
//			/* restart if GM not locked or holdover according to the announce message */
//       		if( (rms.time_source[0]== RM_GM_1) || (rms.time_source[0]== RM_GM_2) )
//       		{
//				if(!Is_GM_Lock_Holdover(gm_index))
//				{
//					fll_start=PTP_START;
//				}
//      		}
 //        Transfer_PTP_data_now(&PTP_Fifo_Local, &PTP_Delay_Fifo_Local, &sync_cntr, &delay_cntr);
			break;
		case PTP_LOCKED:
			fll_onesec();

/* if time has not been set, then set it if there is a active time source */
         if((!time_has_been_set_once && Is_Active_Time_Reference_OK()) || Was_Time_Adjusted())
         {
//            if(rms.time_source[0]== RM_GPS_1)
//            {
//               Sync_Using_GPS();
//               Set_Time_Adjusted_Flag();
//            }
            counter_state = 0;
		      state = PTP_MAJOR_TS_EVENT_WAIT; //use same routine for all timescale coarse adjsut            
         }
         break; 
#define DELAY_FOR_MAJOR_TS_ADJUSTMENT 20             
      case PTP_MAJOR_TS_EVENT_WAIT:
 			debug_printf(GEORGE_PTP_PRT, "!!Major timing event mask\n"); 
         major_ts_change = TRUE;
         fll_onesec();
         if(counter_state > DELAY_FOR_MAJOR_TS_ADJUSTMENT)
         {
            counter_state = 0;
            state = PTP_MAJOR_TS_EVENT;        
         }
         else if(!Is_Active_Time_Reference_OK())
         {
            major_ts_change = FALSE;
            counter_state = 0;
            state = PTP_LOCKED;        
         }
			break;
      case PTP_MAJOR_TS_EVENT:
         if(Is_Active_Time_Reference_OK())
         {
    			debug_printf(GEORGE_PTP_PRT, "!!Major timing event\n"); 
            Restart_Phase_Time();
            time_transient_flag=256;
            counter_state = 0;
            state = PTP_MAJOR_TS_EVENT_END;        
         }
         else
         {
            major_ts_change = FALSE;
            counter_state = 0;
            state = PTP_LOCKED;        
         }

         fll_onesec();  /* after setting the Restart phase flag */
         break; 
#define DELAY_FOR_MAJOR_TS_ADJUSTMENT_BLANK 10             
      case PTP_MAJOR_TS_EVENT_END:
/* let the time settle before switching on loops */   
 			debug_printf(GEORGE_PTP_PRT, "!!Major timing event end\n"); 
         if(counter_state > DELAY_FOR_MAJOR_TS_ADJUSTMENT_BLANK)
         {
            counter_state = 0;
            state = PTP_LOCKED;        
            major_ts_change = FALSE;
         }
         fll_onesec();  /* after setting the Restart phase flag */
         break;        
		default:
			break;	
		}
		counter_state++;
	} //while forever loop

#ifdef FAST_VCAL
	if(vcal_working.Varactor_Cal_Flag==1)
	{
		Varactor_Cal_Routine();	
	}   
#endif
   end_time = GetTime();

   debug_printf(CPU_BANDWIDTH_PRT, "%ld, %s thread %lld ns, start_time %lld\n", tick_counter, __func__, end_time-start_time, start_time);
   
   return 0;
}


///////////////////////////////////////////////////////////////////////
// This function executes the FLL one second tasks
///////////////////////////////////////////////////////////////////////
static int STOP_PTP = 0; //For testing purposes
static void fll_onesec(void)
{
	LOG_PARAM_UNION param[2];
//	double ftemp;
//	uint_16 sync_indx, delay_indx;
   t_ptpTimeStampType  s_sysTime;
   INT16 d_utcOffset;
   int input_repeat_cnt;
//	static int input_repeat_cnt_limit;
//   INT32 GPS_Local_Offset;
//   UINT16 gps_a_index;
#if 1  //GPZ MARCH 2011 move to first task on one second thread to minimize latency variation
   SC_FrequencyCorrection_Local(&fdrift_warped, e_PARAM_SET);
#endif

	fll_update(); //move to start of task to improve timing
    fll_synth(); //update main fll national phyter synthesizer
	debug_printf(GEORGE_PTP_PRT,"Elapse Time: Elapse_Phy:%ld ::Elapse_PTP:%ld \n",FLL_Elapse_Sec_Count,FLL_Elapse_Sec_PTP_Count);

	if(FLL_Elapse_Sec_Count_Use>75L) // give time for buffers to stabilize
	{
		#ifdef ENABLE_TIME
		update_time(); 
		#endif
	}
#if(NO_PHY==0)
	if(Is_Major_Time_Change() == FALSE)
	{
		update_input_stats_phy(); //update all input physical signal stats for all channels
	}
#endif	
	if(fll_stabilization>0)
	{
		fll_stabilization --;
		clr_phase_flag =1;	
		holdover_f = fdrift_smooth;	
	}
	if(FLL_Elapse_Sec_Count<2000000000L)
	{
			if (Is_Active_Phy_Freq_Reference_OK() == TRUE) FLL_Elapse_Sec_Count++;	
	}
	if(FLL_Elapse_Sec_PTP_Count<2000000000L)
	{
			if (  !Is_Sync_timeout(gm_index) && !Is_Delay_timeout() ) FLL_Elapse_Sec_PTP_Count++;	
	}	
   if(FLL_Elapse_Sec_Count > FLL_Elapse_Sec_PTP_Count)
   {
       FLL_Elapse_Sec_Count_Use = FLL_Elapse_Sec_Count;
//      if(FLL_Elapse_Sec_PTP_Count>3600) FLL_Elapse_Sec_PTP_Count=FLL_Elapse_Sec_Count_Use;  //GPZ JAN 2012 Looks Wrong
   }
   else
   {
       FLL_Elapse_Sec_Count_Use = FLL_Elapse_Sec_PTP_Count;
//      if(FLL_Elapse_Sec_Count>3600) FLL_Elapse_Sec_Count=FLL_Elapse_Sec_Count_Use; //GPZ JAN 2012 Looks Wrong
   }
	//do snapshot to get local data
//	snapshot();
#ifdef TEMPERATURE_ON
	if(FLL_Elapse_Sec_Count_Use> 60L) // start collection temperature data
	{
		temperature_calc();
	}	
#endif
	if(((Is_Delay_timeout()) && (Is_Sync_timeout(gm_index))) ||(STOP_PTP))
	{
		
	}
	else if (Is_Major_Time_Change() == FALSE)
	{
#if 0
		Sync_Out_Indx= PTP_Fifo_Local.fifo_out_index;	
		Delay_Out_Indx= PTP_Delay_Fifo_Local.fifo_out_index;	
		update_input_stats(); //update all input packet stats for all channels
		// Repeat to accelerate cluster estimation convergence
		PTP_Fifo_Local.fifo_out_index= Sync_Out_Indx;
		PTP_Delay_Fifo_Local.fifo_out_index=Delay_Out_Indx;
		update_input_stats(); //update all input packet stats for all channels
		// Repeat to accelerate cluster estimation convergence
		PTP_Fifo_Local.fifo_out_index= Sync_Out_Indx;
		PTP_Delay_Fifo_Local.fifo_out_index=Delay_Out_Indx;
		update_input_stats(); //update all input packet stats for all channels
		// Repeat to accelerate cluster estimation convergence
		PTP_Fifo_Local.fifo_out_index= Sync_Out_Indx;
		PTP_Delay_Fifo_Local.fifo_out_index=Delay_Out_Indx;
		update_input_stats(); //update all input packet stats for all channels
#else
  		Sync_Out_Indx= PTP_Fifo_Local.fifo_out_index;	
  		Delay_Out_Indx= PTP_Delay_Fifo_Local.fifo_out_index;	
		update_input_stats(); //update all input packet stats for all channels
		if(OCXO_Type() == e_MINI_OCXO)
		{
//			input_repeat_cnt_limit=7;
			input_repeat_cnt_limit=3;  //OCT 11 slow down
		}
		else if(OCXO_Type() == e_OCXO)
		{
			input_repeat_cnt_limit=3;
		}
		else
		{
			input_repeat_cnt_limit=3;
		}
      for(input_repeat_cnt=0;input_repeat_cnt<input_repeat_cnt_limit;input_repeat_cnt++) //was 7
      {
   		// Repeat to accelerate cluster estimation convergence
	   	PTP_Fifo_Local.fifo_out_index= Sync_Out_Indx;
	   	PTP_Delay_Fifo_Local.fifo_out_index=Delay_Out_Indx;
      	update_input_stats(); //update all input packet stats for all channels
      }
#endif
	}
#if (SYNC_E_ENABLE == 1)	
	update_se(); // update synchronous ethernet input 	
#endif	
#if(ASYMM_CORRECT_ENABLE==1)
	update_acl();
#endif	
	//update minimum selection
   	if(mchan == FORWARD||mchan==BOTH)
   	{
//	 	get_min_offset(&PTP_Fifo_Local,&min_forward,Initial_Offset,1); //forward channel
//	 	if(min_forward.sdrift_min_1024==MAXMIN && (!start_in_progress))
//	 	{
//  			debug_printf(GEORGE_PTP_PRT, "exception: no forward min found\n");
//	 		return; //Jan 2010 Do not exit routinr
//	 	}
//		min_forward.sdrift_min_1024=1234; //GPZ TEST
		if(var_cal_flag==0 && (rfe_turbo_flag==0))	 	
	 	{
	 		min_valid(1);
	 	}
		#if (N0_1SEC_PRINT==0)		
	 	if(min_forward.sdrift_min_1024==MAXMIN && (!start_in_progress_PTP))
	 	{
  			debug_printf(GEORGE_PTP_PRT, "exception: no forward min found\n");
	 	}
		#endif	 		
   	}
	if(mchan ==REVERSE||mchan==BOTH)
	{
//	 	get_min_offset(&PTP_Delay_Fifo_Local,&min_reverse,Initial_Offset_Reverse,0); //reverse channel
//	 	if(min_reverse.sdrift_min_1024==MAXMIN && (!start_in_progress))
//	 	{
// 			debug_printf(GEORGE_PTP_PRT, "exception: no reverse min found\n");
//	 		return; //Jan 2010 Do not exit routinr
//	 	}
//		min_reverse.sdrift_min_1024=4321; //GPZ TEST
		if(var_cal_flag==0&& (rfe_turbo_flag==0))
	 	{
		 	min_valid(0);
	 	}
		#if (N0_1SEC_PRINT==0)		
	 	if(min_reverse.sdrift_min_1024==MAXMIN && (!start_in_progress_PTP))
	 	{
  			debug_printf(GEORGE_PTP_PRT, "exception: no reverse min found\n");
	 	}
		#endif	 	
	}
	if(start_in_progress && FLL_Elapse_Sec_Count >(300)) 
	{
		/* if PTP hasn't been start either */
		if(start_in_progress_PTP)
		{	
			Get_Freq_Corr(&last_good_freq,&gaptime);
			debug_printf(GEORGE_PTP_PRT, "early NVM warm reboot detilt exit\n");
         fdrift_f=fdrift_r=fdrift=fdrift_warped=fdrift_smooth=pll_int_r=
         pll_int_f=fdrift_raw=holdover_f=holdover_r=last_good_freq;

      // allow to set common variables if PTP is not currently being used 
	      if(!Is_PTP_FREQ_ACTIVE())
	      {
    	      setfreq((long int)(last_good_freq*HUNDREDF),1); // 29.57 GPZ Jan 2010 force tight lock down of frequency
 			}
	      else
	      {
   	      setfreq((long int)(last_good_freq*HUNDREDF),0); // 29.57 GPZ Jan 2010 force tight lock down of frequency
 			}
 		}
#if(NO_PHY==0)
	   else if(!Is_PTP_FREQ_ACTIVE())
	   {
	   		fdrift_f=fdrift_r=fdrift=fdrift_warped=fdrift_smooth=pll_int_r=pll_int_f= fdrift_raw=holdover_r=holdover_f;
     			debug_printf(GEORGE_PTP_PRT, "restart phy rfe exit warmup\n");
            #if(NO_PHY==0)
   	      start_rfe_phy(holdover_f);
            #endif
 		}
#endif
		start_in_progress = FALSE;
		min_accel=1; //always use graceful regression shifting	
			
	}
/* check for PTP startup */

// JAN 26	debug_printf(GEORGE_PTP_PRT, "NVM exit criteria: start_PTP: %ld, Elapse_PTP: %ld, detilt: %ld, density_f: %ld, density_r: %ld\n",
//   (long int)(start_in_progress_PTP),
//   (long int)(FLL_Elapse_Sec_PTP_Count),
//   (long int)(detilt_time_oper),
//   (long int)(floor_density_small_f),
//   (long int)(floor_density_small_f));
//	debug_printf(GEORGE_PTP_PRT, "Elapse_Time: %ld start_in_progress: %ld\n",FLL_Elapse_Sec_PTP_Count,(long int)start_in_progress);

	if((start_in_progress_PTP) && (FLL_Elapse_Sec_PTP_Count < detilt_time_oper))
	{
		if( ((floor_density_small_f> 10000)&&(floor_density_small_r >10000)&& (FLL_Elapse_Sec_PTP_Count > 120))
          ||(FLL_Elapse_Sec_PTP_Count > 300)) // use NVM for fast exit if short gap
		{
			if(start_in_progress_PTP) //GPZ Jan 2011 make PTP based
			{
				Get_Freq_Corr(&last_good_freq,&gaptime);
	 			debug_printf(GEORGE_PTP_PRT, "very early NVM warm reboot exit\n");
             /* allow to set common variables if a Phy channel is not currently being used */
//            if(!Is_PHY_FREQ_ACTIVE()) //GPZ Jan 2011 make PTP base
            {
               fdrift_f=fdrift_r=fdrift=fdrift_warped=fdrift_smooth
               =pll_int_r=pll_int_f= fdrift_raw=holdover_f=holdover_r=last_good_freq;
   				setfreq((long int)(last_good_freq*HUNDREDF),0); // 29.57 GPZ Jan 2010 force tight lock down of frequency
   				time_transient_flag=time_hold_off; //do not update time steering while in holdover FEB 2011
            }
			}
   	   else if(!Is_PHY_FREQ_ACTIVE())
   	   {
   	 			debug_printf(GEORGE_PTP_PRT, "other very early NVM warm reboot exit\n");
   	   		fdrift_f=fdrift_r=fdrift=fdrift_warped=fdrift_smooth=pll_int_r=pll_int_f= fdrift_raw=holdover_r=holdover_f;
      	      start_rfe(holdover_f);
   				time_transient_flag=time_hold_off; //do not update time steering while in holdover FEB 2011
   		}
			start_in_progress_PTP = FALSE;
          exittilt();
       
			min_accel=1; //always use graceful regression shifting	
		}
				
		// Calculate stilt in preparation for calling detilt function GPZ Jan 2010		
		if(SENSE==0)
		{
			if(mchan==BOTH)
			{
				min_detilt.sdrift_min_1024 = (min_reverse.sdrift_min_1024-min_forward.sdrift_min_1024)/2;//positive sign for reverse
			}	
			else if(mchan ==FORWARD)
			{
				min_detilt.sdrift_min_1024= (-min_forward.sdrift_min_1024);//positive sign for reverse
			}
			else
			{
				min_detilt.sdrift_min_1024= (min_reverse.sdrift_min_1024);//positive sign for reverse
			}
		}
		else
		{
			if(mchan==BOTH)
			{
				min_detilt.sdrift_min_1024= -(min_reverse.sdrift_min_1024-min_forward.sdrift_min_1024)/2;
			}	
			else if(mchan ==FORWARD)
			{
				min_detilt.sdrift_min_1024= -(-min_forward.sdrift_min_1024);
			}
			else
			{
				min_detilt.sdrift_min_1024= -(min_reverse.sdrift_min_1024);
			}
		}
		
		detilt_cluster= fllstatus.r_min_cluster_width;

		if((FLL_Elapse_Sec_PTP_Count) > 75) // give time for buffers to stabilize
		{
//			detilt();
			//Update phase calibration while in warmup
			update_phase();
		}
		else //hold off validation to allow for buffer stabilization
		{
			rmfirst_f=1;
			rmfirst_r=1;
		}	
	}
	else // warmup complete 
	{
		update_drive(); //updated drive signals to combiner
		combine_drive();
//		fll_update(); //move to start of task to improve timing
//		if((mchan == FORWARD||mchan==BOTH)&& delta_forward_valid && (delta_forward_slew==0))
		if(Print_Phase%4==1) // update forward TDEV
		{
			if((mchan == FORWARD||mchan==BOTH)&& (delta_forward_valid))
			{
			// NOTE GPZ March 2009 compensate with floor bias
			forward_stats.tdev_element[TDEV_CAT].current=(min_forward.sdrift_min_1024);
			//GPZ test square wave for calculator
//			if((Print_Phase&0xF)==2)
//			{
//				if( forward_stats.tdev_element[TDEV_CAT].current==-1000)
//				{
//					forward_stats.tdev_element[TDEV_CAT].current=1000;
//				}	
//				else
//				{
//					forward_stats.tdev_element[TDEV_CAT].current=-1000;
//				}
//			}	
			// End test square wave for calculator
			tdev_calc(&forward_stats);
//			forward_tdev_print();
			}
		}
		if(Print_Phase%4 ==3) // update reverse TDEV
		{
//		if((mchan == REVERSE||mchan==BOTH)&& delta_reverse_valid && (delta_reverse_slew==0))
			//March 7 2009 GPZ added slew as condition to skip update

			if((mchan == REVERSE||mchan==BOTH)&& delta_reverse_valid )
			{	
			reverse_stats.tdev_element[TDEV_CAT].current=(min_reverse.sdrift_min_1024);
			tdev_calc(&reverse_stats);
//			reverse_tdev_print();
			}
		}
#if 0		
// GPZ OCT 2010 MOVE BLOCK TO 16 TASK			
//		if((Print_Phase&0xF)==2)  fll_print();
//		else if((Print_Phase&0xF)==4)  combiner_print();
//		else if((Print_Phase&0xF)==6) min_print(& min_forward);
//		else if((Print_Phase&0xF)==8) ofd_print();
//		else if((Print_Phase&0xF)==10) min_print(& min_reverse);
//		else if((Print_Phase&0xF)==12) forward_tdev_print();
//		else if((Print_Phase&0xF)==14) reverse_tdev_print();
// END MAJOR PRINT BLOCK
#endif		
		if(FLL_Elapse_Sec_Count_Use< 3600)
		{
			log_pace=300;
		}	
		else if(FLL_Elapse_Sec_Count_Use< 86400)
		{
			log_pace=3600;
		}	
		else
		{
			log_pace=14400;	
		}
//		log_pace=7200; //for metrics test comment out for buid
		if(Print_Phase % log_pace ==0) // Simulate four-hour hour task call make 4 hours for release
//		if(Print_Phase % 300 ==0) // For TEST ONLY
		{
			param[0].sign.signed_32 = (long int)(xds.xfer_fest_f*1000.0);  //Ken_Post_Merge
			param[1].sign.signed_32 = (long int)(xds.xfer_fest_r*1000.0);  //Ken_Post_Merge
			Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, param[0].sign.unsigned_32,param[1].sign.unsigned_32, LOG_EXC_FIT_FREQ_EST );
			param[0].sign.signed_32 = (long int)(fbias);
			param[1].sign.signed_32 = (long int)(rbias);
			Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, param[0].sign.unsigned_32,param[1].sign.unsigned_32, LOG_EXC_FLOOR_BIAS_EST);
			param[0].sign.signed_32 = (long int)(sqrt(forward_stats.tdev_element[TDEV_CAT].var_est));
			param[1].sign.signed_32 = (long int)(sqrt(reverse_stats.tdev_element[TDEV_CAT].var_est));
			Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, param[0].sign.unsigned_32,param[1].sign.unsigned_32, LOG_EXC_FLOOR_TDEV_EST);
			param[0].sign.signed_32 = (long int) (fllstatus.f_mafie*1000.0);
			param[1].sign.signed_32 = (long int) (fllstatus.r_mafie*1000.0);
			Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, param[0].sign.unsigned_32,param[1].sign.unsigned_32, LOG_EXC_FLOOR_MAFE_EST);
			param[0].sign.signed_32 = (long int)(round_trip_delay_alt);
			param[1].sign.signed_32 =(long int) (fllstatus.out_mdev_est*1000.0);
			Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, param[0].sign.unsigned_32,param[1].sign.unsigned_32, LOG_EXC_FLOOR_RTD_EST);
#ifdef TEMPERATURE_ON
			param[0].sign.signed_32 = (long int)(temp_stats.tes[(temp_stats.t_indx+127)%128].t_avg*1000);
			param[1].sign.signed_32 = 0;
			Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, param[0].sign.unsigned_32,param[1].sign.unsigned_32, LOG_EXC_TEMP_AVG_EST);
#endif
		}
		
	}

/* store frequency with current time */
	if((Print_Phase % log_pace ==0) && (FLL_Elapse_Sec_Count_Use> 10000))
	{
      if(SC_SystemTime(&s_sysTime, &d_utcOffset, e_PARAM_GET))
   	{
#if NO_PHY == 0
	
      	debug_printf(UNMASK_PRT, "Error while running SC_SystemTime()");
#endif
     	}
     	debug_printf(GEORGE_PTP_PRT, "SC_StoreFreqCorr(): %le\n", fdrift);
		SC_StoreFreqCorr((float)(fdrift),  s_sysTime); //store current frequency state
	}	
	
	if(min_accel==1)
	{
      if(OCXO_Type()==e_MINI_OCXO)
		{
			if(FLL_Elapse_Sec_Count_Use>4000)
			{
				min_accel=0; //return to steady state regression case
			}
		}
		else
		{
			if(FLL_Elapse_Sec_Count_Use>4000)//was 28800
			{
				min_accel=0; //return to steady state regression case
			}
		}
	}
//	else
//	{
	sync_xfer_8s();
	if(Print_Phase % 64 ==0) // check configuratio
	{
		Local_Minute_Task();
	}
	if(Print_Phase % 32 ==0) // calibration cycle check
	{
		if(vcal_working.Varactor_Cal_Flag==1)
		{
			Varactor_Cal_Routine();	
		}
	}
	
//	if(Print_Phase % 16 ==0) // turbo loop related functions
//	{
//		Local_16s_Task();
//	}
		
	
//	}
	//TODO Comment out and comment external 1 minute task
#ifdef TEMPERATURE_ON
	if(Print_Phase % 8 ==0) // debug run analysis 8x faster
	{
		temperature_analysis();
	}
#endif	
	Print_Phase++;	
	fll_status();	

}


///////////////////////////////////////////////////////////////////////
// This function steers the control loop during normal lock
//
//
///////////////////////////////////////////////////////////////////////
static double dline_f[16], nfreq_forward_delay,nfreq_forward_smooth; //test concept add a delay line in path
static double dline_r[16], nfreq_reverse_delay,nfreq_reverse_smooth; //test concept add a delay line in path
static int test_delay=8;
static int idelay=0;
//static int sel_cnt_f=0,sel_cnt_r=0,sel_thres_f,sel_thres_r;
//static double sel_min_f=MAXMIN;
//static double sel_min_r=MAXMIN;
//static double sacc_gain=1.0;
//double pf,pr;
static INT32 sticky_cluster_band_f, sticky_cluster_width_f; //GPZ NOV 2010 create hysteris based metrics
static INT32 sticky_cluster_band_r, sticky_cluster_width_r; //GPZ NOV 2010 create hysteris based metrics
//static double bias_cross_corr;
//static double rgain;
//static double lcomp_f,lcomp_r,hcomp_f,hcomp_r;
static void fll_update(void)
{
	double oper_sq;
//	double oper_cb;
	INT32 temp;
	test_delay=2; //make minimal
	if(skip_rfe>0)
	{
		skip_rfe--;
	}
	#if(NO_PHY==0)
	if(skip_rfe_phy>0)
	{
		skip_rfe_phy--;
	}
	#endif	
	// update open loop accumulate term and check for range
   if((LoadState == LOAD_GENERIC_HIGH)||(OCXO_Type()==e_RB))  //TODO use same approach with slow Rubidium steering
   {
     hold_res_acc = 0.0; 
   }
   else
   {
	   hold_res_acc += (fdrift - holdover_f);
   }
	if(  ( hold_res_acc>(double)(sacc_thres_oper) ) || ( hold_res_acc< (double)(-sacc_thres_oper) ) )
	{
		Forward_Floor_Change_Flag=1; 
		Reverse_Floor_Change_Flag=1;
		hold_res_acc=0.0; //GPZ Jan 2011 Appeared to be a missing accumulation reset that needs to be here		
	}
	// prepare sticky metrics  GPZ NOV 2010
	sticky_cluster_band_f = min_oper_thres_f/2000; //was 10 make less sticky Feb 2011 (80) make less sticky April 2011 (200)
	sticky_cluster_band_r = min_oper_thres_r/2000;	
	temp= sticky_cluster_width_f -	min_oper_thres_f;
	if(temp<0) temp= -temp;
	if(temp > sticky_cluster_band_f)
	{
		sticky_cluster_width_f=min_oper_thres_f;
	}
	temp= sticky_cluster_width_r -	min_oper_thres_r;
	if(temp<0) temp= -temp;
	if(temp > sticky_cluster_band_r)
	{
		sticky_cluster_width_r=min_oper_thres_r;
	}
	
	
	
	if((dflag==0)&&(FLL_Elapse_Sec_PTP_Count>3600)&&(fll_settle==0)) //set once 
	{
			nfreq_forward_smooth=nfreq_forward;
			nfreq_reverse_smooth=nfreq_reverse;
			scw_f=sticky_cluster_width_f;
			scw_r=sticky_cluster_width_r;
			smw_f=(double)ofw_start_f;
			smw_r=(double)ofw_start_r;
			fdrift_smooth= fdrift; //no smoothing for time build
			scw_avg_f= scw_f;	
			scw_e2_f=  scw_f*scw_f;	
			scw_avg_r= scw_r;	
			scw_e2_r=  scw_r*scw_r;
//			init_rfe();
//			start_rfe(fdrift);//GPZ always restart rfe at this point
//			Forward_Floor_Change_Flag=2; //GPZ NOV 2010 force floor change during holdover recovery
//			Reverse_Floor_Change_Flag=2;
//			skip_rfe=64; // give smoothing average a chance to stabilize	was 384
			dflag=1;
	}
 	if(dflag)
	{
		//May 13 2009 reduce smoothing factor from 0.99 to PER_95
		//June 2009 back up 0.99 to improve cancellation with tshape
		nfreq_forward_delay= dline_f[idelay];
		nfreq_reverse_delay= dline_r[idelay];
		if(var_cal_flag==0)
		{
			nfreq_forward_smooth=0.9*nfreq_forward_smooth+0.1*nfreq_forward;
			nfreq_reverse_smooth=0.9*nfreq_reverse_smooth+0.1*nfreq_reverse;

//			scw_f= (1.0 - 0.033)*scw_f +(0.033)*sticky_cluster_width_f;
//			scw_r= (1.0 - 0.033)*scw_r +(0.033)*sticky_cluster_width_r;
//			smw_f= (1.0 - 0.039)*smw_f +(0.039)*(double)ofw_start_f;
//			smw_r= (1.0 - 0.039)*smw_r +(0.039)*(double)ofw_start_r;
			
			scw_f= sticky_cluster_width_f;
			scw_r= sticky_cluster_width_r;
			smw_f=(double)ofw_start_f;
			smw_r=(double)ofw_start_r;
			
			fdrift_smooth=nfreq_smooth_g2*fdrift_smooth+nfreq_smooth_g1*(fdrift);

		}
		else
		{
			nfreq_forward_smooth=0.5*nfreq_forward_smooth+0.5*nfreq_forward;
			nfreq_reverse_smooth=0.5*nfreq_reverse_smooth+0.5*nfreq_reverse;
			scw_f= 0.5*scw_f +0.5*sticky_cluster_width_f;
			scw_r= 0.5*scw_r +0.5*sticky_cluster_width_r;
			smw_f= 0.5*smw_f +0.5*(double)ofw_start_f;
			smw_r= 0.5*smw_r +0.5*(double)ofw_start_r;
			fdrift_smooth=0.5*fdrift_smooth+0.5*(fdrift);
		}	
	}
	else //no smoothing
	{
		nfreq_forward_smooth=nfreq_forward;
		nfreq_reverse_smooth=nfreq_reverse;
		scw_f=sticky_cluster_width_f;
		scw_r=sticky_cluster_width_r;
		smw_f=(double)ofw_start_f;
		smw_r=(double)ofw_start_r;
		fdrift_smooth= fdrift; //no smoothing for time build
//		fdrift_smooth=0.90*fdrift_smooth+0.1*fdrift;
	}
	// cluster statistics
	scw_avg_f= 0.001*scw_f + 0.999*	scw_avg_f;	
	scw_e2_f= 0.001*scw_f*scw_f + 0.999*scw_e2_f;	
	scw_avg_r= 0.001*scw_r + 0.999*	scw_avg_r;	
	scw_e2_r= 0.001*scw_r*scw_r + 0.999*scw_e2_r;	
	
	
	
#if 0	
	if ((ptpTransport!=e_PTP_MODE_ETHERNET)) // Non_Ethernet Transport Cases
	{
		// clipped metrics
		if(scw_f<(double)(cluster_width_max)) //was 10000
		{
			cscw_f=scw_f;
		}
		else
		{
			cscw_f= (double)cluster_width_max;
		}
		if(scw_r<(double)(cluster_width_max))
		{
			cscw_r=scw_r;
		}
		else	
		{
			cscw_r=(double)cluster_width_max;
		}
		if(smw_f>500000.0)//was 103000
		{
			csmw_f=500000.0;
		}
		else if(smw_f>50000.0) 
		{
			csmw_f=smw_f;
		}
		else
		{
			csmw_f=50000.0;
		}
		if(smw_r>500000.0)
		{
			csmw_r=500000.0;
		}
		else if(smw_r>50000.0)
		{
			csmw_r=smw_r;
		}
		else
		{
			csmw_r=50000.0;
		}
	// end clip metrics	
		pf=(sqrt(csmw_f)); //normalization to sqrt of mode width
		pf=cscw_f/pf;
		oper_sq=(double)(pf)*(double)(pf);
		hcomp_f= 129.65*pf + 2.8433*(oper_sq); //NOV 28 after recal with new bias and open loop system  0.00082
//		debug_printf(GEORGE_PTP_PRT, "forward microwave bias comp: \n");
		pr=(sqrt(csmw_r)); //normalization to sqrt of mode width
		pr=cscw_r/pr;
		oper_sq=(double)(pr)*(double)(pr);
		hcomp_r= -129.65*pr - 2.8433*(oper_sq); 
//		debug_printf(GEORGE_PTP_PRT, "reverse microwave bias comp: \n");
	}
#endif
	// update compensation
	
	
	lcomp_f=hcomp_f; //no difference in low and high regions now
	lcomp_r=hcomp_r;	
	// end update compensation
	dline_f[idelay]= nfreq_forward_smooth;
	dline_r[idelay]= nfreq_reverse_smooth;
	idelay++;
	if(idelay>test_delay) idelay=0; //sets depth of delay line
 	if(dflag)
 	{
		nfreq_forward= nfreq_forward_delay;
		nfreq_reverse= nfreq_reverse_delay;
	}
			
//  	sacc_f_last = sacc_f_cor;
//		sacc_r_last = sacc_r_cor; 
#if 0
	if ((ptpTransport!=e_PTP_MODE_ETHERNET)) // fbias and rbias  controlled in clk input now
	{

   	if(scw_f<10000)
   	{
   		fbias=	-lcomp_f;
   	}
   	else
   	{
   		fbias=	-hcomp_f;
   	}
		if(scw_r<10000)
		{
			rbias=	-lcomp_r;
		}
		else
		{
			rbias=	-hcomp_r;
		}
	}
#endif
	if(fll_settle > 0)
	{
		fll_settle--;
//		if(  (fll_settle>32) && (  (OCXO_Type() == e_OCXO)||(OCXO_Type() == e_MINI_OCXO)||(OCXO_Type() == e_TCXO)  )   )
		if(fll_settle>32)
		{
			fll_settle=32;	
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTP_PRT, "exception: fll_settle started mini:%d\n",fll_settle);
			#endif
			if(read_exc_gate(LOG_EXC_FLL_START))
			{
				Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_FLL_START);
				set_exc_gate(LOG_EXC_FLL_START);
			}
		}
		else if(fll_settle==(FLL_SETTLE-2)) 
		{
			#if (N0_1SEC_PRINT==0)		
 			debug_printf(GEORGE_PTP_PRT, "exception: fll_settle started:%d\n",fll_settle);
 			#endif
			if(read_exc_gate(LOG_EXC_FLL_START))
			{
				Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_FLL_START);
				set_exc_gate(LOG_EXC_FLL_START);
			}	
 			
 		}
 		else if(fll_settle==1)
 		{
			#if (N0_1SEC_PRINT==0)		
 			debug_printf(GEORGE_PTP_PRT, "exception: fll_settle complete:%d\n",fll_settle);
 			#endif
			if(read_exc_gate(LOG_EXC_FLL_COMPLETE))
			{
				Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_FLL_COMPLETE);
				set_exc_gate(LOG_EXC_FLL_COMPLETE);
			}	
 		}	
	}
#if(FREEZE==1)
	fbias=ZEROF;
	rbias=ZEROF;
#endif	
	if(var_cal_flag==1)
	{
		fbias=ZEROF;
		rbias=ZEROF;
	}	
	if((!fll_settle)&&(!skip_rfe))
	{
//		if((reshape == 0)&& (fll_settle==0)) //GPZ APRIL 2010 add fll_settle
		if((fll_settle==0)) //GPZ APRIL 2010 add fll_settle
		{
			// FEB 18 GPZ calculate open loop compensated freq measurements
			if(delta_forward_slew >0)
			{
				fdrift_f_meas= fdrift_smooth;
				nfreq_forward_smooth=ZEROF;
			}
			else
			{	
//				fdrift_f_meas= (double)(nfreq_forward_smooth) + fdrift_smooth;
				fdrift_f_meas= (double)(nfreq_forward_smooth) + holdover_f;//yes use holdover_f it is for all channels
			}
//			#ifdef TP500_BUILD
//			fdrift_f_meas -= 2.91038E-2; //natsemi LSB adjust
//			#endif
			sacc_f+=fdrift_f_meas;
			if(delta_reverse_slew >0)
			{
				fdrift_r_meas= fdrift_smooth;
				nfreq_reverse_smooth=ZEROF;
				
			}
			else
			{
//				fdrift_r_meas= (double)(nfreq_reverse_smooth) + fdrift_smooth;
				fdrift_r_meas= (double)(nfreq_reverse_smooth) + holdover_f; //yes use holdover_f it is for all channels
				
			}
//			#ifdef TP500_BUILD
//			fdrift_r_meas -= 2.91038E-2; //natsemi LSB adjust
//			#endif
			sacc_r+=fdrift_r_meas;
		}
		else
		{
			fdrift_f_meas= fdrift_smooth;
			fdrift_r_meas= fdrift_smooth;
			nfreq_forward_smooth=ZEROF;
			nfreq_reverse_smooth=ZEROF;
		}	
	}
	/* run the loop filter */
	//*sanity check
	if((!start_in_progress) && (sacc_f>sacc_thres_oper || sacc_f< -sacc_thres_oper))
	{
		// loop phase error is too large recal integral
		#if (N0_1SEC_PRINT==0)		
		debug_printf(GEORGE_PTP_PRT, "exception: forward loop phase too large: sacc_f %le\n",sacc_f);
		debug_printf(GEORGE_FAST_PRT, "exception: forward loop phase too large: sacc_f %le\n",sacc_f);
		#endif
		if(read_exc_gate(LOG_EXC_SYNC_LOOP_PHASE_RANGE))
		{
			Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_SYNC_LOOP_PHASE_RANGE );
			set_exc_gate(LOG_EXC_SYNC_LOOP_PHASE_RANGE);
		}	
		cal_for_loop();				
	}
	if((!start_in_progress) && (sacc_r>sacc_thres_oper || sacc_r< -sacc_thres_oper))
	{
		// loop phase error is too large recal integral
		#if (N0_1SEC_PRINT==0)		
		debug_printf(GEORGE_PTP_PRT, "exception: reverse loop phase too large: sacc_r %le\n",sacc_r);
		debug_printf(GEORGE_FAST_PRT, "exception: reverse loop phase too large: sacc_r %le\n",sacc_r);
		#endif		
		
		if(read_exc_gate(LOG_EXC_DELAY_LOOP_PHASE_RANGE))
		{
			Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_DELAY_LOOP_PHASE_RANGE );
			set_exc_gate(LOG_EXC_DELAY_LOOP_PHASE_RANGE);
		}	
		cal_rev_loop();				
	}
#if ((FREEZE==1))		
//		fdrift_f= (double)(nfreq_forward_smooth); //For test purposed only can comment out
// 		fdrift_r= (double)(nfreq_reverse_smooth); //For test purposed only can comment out

#else 
//	   	 fdrift_f= pll_prop_f + pll_int_f;
// 		 holdover_forward(fdrift_f);
//	   	 fdrift_r= pll_prop_r + pll_int_r;
// 		 holdover_reverse(fdrift_r);
		if((!fll_settle)&&!(skip_rfe)) // only update if inputs are settled
		{
   	 		fdrift_f= lsf_smooth_g1*fdrift_f_meas + lsf_smooth_g2*fdrift_f; //was 0.002 April 20 2009
   	 		fdrift_r= lsf_smooth_g1*fdrift_r_meas +lsf_smooth_g2*fdrift_r;  //was 0.002 April 20 2009
//	 		holdover_forward(xds.xfer_fest_f); 
//.	 		holdover_reverse(xds.xfer_fest_r); 
	 		// new approach just use one holdover routine with fdrift_smooth
#if SMARTCLOCK == 0
	 		if((hcount==0)&&(hcount_rfe==0)) //don't update if in holdover
#endif
	 		{
		 		holdover_forward(fdrift_smooth); 
//		 		holdover_forward(fdrift_raw); 
	 		}
 		} 
// 		else
// 		{
// 			fdrift_f=holdover_f; //GPZ add protection Sept 2009
// 			fdrift_r=holdover_r; //GPZ add protection Sept 2009
// 	  	} 
#endif   	
}
///////////////////////////////////////////////////////////////////////
// This function prints FLL control loop stats
///////////////////////////////////////////////////////////////////////
static void fll_print(void)
{
	long int scaledA,scaledB,scaledC,scaledD,scaledE,scaledF;
	

	#if (MINIMAL_PRINT==0)
	if(fll_flag)
	{
		scaledA=(long int)(cscw_f);
		scaledB=(long int)(nfreq_forward_smooth*THOUSANDF);
		scaledC=(long int) (csmw_f);
		scaledD=(long int) fbias;
		scaledE=(long int) hold_res_acc;
		debug_printf(GEORGE_PTP_PRT, "fll_state_f:%d  cscw_f:%ld  csmw_f:%ld fbias: %ld fmeas:%ld res_acc: %ld\n",
	        FLL_Elapse_Sec_Count_Use,
	        (scaledA),
	        (scaledC),
	        (scaledD),
	        (scaledB),
	        (scaledE)
	        );
		scaledA=(long int)(long int)(cscw_r);
		scaledB=(long int)(nfreq_reverse_smooth*THOUSANDF);
		scaledC=(long int) (csmw_r);
		scaledD=(long int) rbias;
		debug_printf(GEORGE_PTP_PRT, "fll_state_r:%d  cscw_r:%ld  csmw_r:%ld rbias: %ld fmeas:%ld\n",
	        FLL_Elapse_Sec_Count_Use,
	        (scaledA),
	        (scaledC),
	        (scaledD),
	        (scaledB)
	        );

	        
	    scaledA=(long int)(fdrift_smooth*THOUSANDF);
	    scaledB=(long int)(fdrift_raw*THOUSANDF);
	    scaledC=(long int)(fdrift_warped*THOUSANDF);
	    scaledE=(long int)(pshape_smooth);
	    scaledF=(long int)(fshape*THOUSANDF);
	    
		debug_printf(GEORGE_PTP_PRT, "fll_general:%d dflag: %d fdrift:%ld,fdrift_raw:%ld,fdrift_warp:%ld pshape:%ld fshape:%ld reshape:%ld hnt:%ld hrfe:%ld\n",
	        FLL_Elapse_Sec_Count_Use,
	        dflag,
	        scaledA,
	        scaledB,
	        scaledC,
	        scaledE,
	        scaledF,
	        (long int) reshape,
	        (long int) hcount,
	        (long int) hcount_rfe
	        );
#if 1	        
		debug_printf(GEORGE_PTP_PRT, "fll_bias_comp:%d clus_inc_f:%ld clus_slew_f:%ld clus_cnt_f:%ld  clus_inc_r:%ld clus_slew_r:%ld clus_cnt_r:%ld  \n",
	        FLL_Elapse_Sec_Count_Use,
	        (cluster_inc_f),
	        (cluster_inc_slew_count_f),
	        (cluster_inc_count_f),
	        (cluster_inc_r),
	        (cluster_inc_slew_count_r),
	        (cluster_inc_count_r)
	        );
#endif
#if 0
		// Calibration Debug channel used for looking at key correlations
		debug_printf(GEORGE_FAST_PRT, "cal:f:%d om:%ld rmf:%ld min_f:%ld smw:%ld scw:%ld :r: om:%ld rmf:%ld min-r:%ld smw:%ld scw:%ld\n",
	        FLL_Elapse_Sec_Count_Use,
	        (long int)(om_f),
	        (long int)(rmf_f),
	        (long int)(min_forward.sdrift_min_1024),
	        (long int)(smw_f),
	        (long int)(scw_f),
	        (long int)(om_r),
	        (long int)(rmf_r),
 	        (long int)(min_reverse.sdrift_min_1024),
	        (long int)(smw_r),
	        (long int)(scw_r)
	        );
#endif	        
	        
	}
	#endif	        
}
///////////////////////////////////////////////////////////////////////
// This function write FLL synthesizer
///////////////////////////////////////////////////////////////////////
static void fll_synth(void)
{
//   double fdrift_warped;
//	int adjDirectionFlag;
//	UINT32 AbsAccumRate;
	int reshape_jam_thres;
	double dtemp;
   if(Is_PTP_FREQ_ACTIVE()== TRUE)
   {
      is_PTP_mode = TRUE;
   }
   else if(Is_PHY_FREQ_ACTIVE() == TRUE)
   {
      is_PTP_mode = FALSE;
   }

	if((start_in_progress) && (start_in_progress_PTP))
	{
		fdrift_raw=fdrift_f;
		fdrift_smooth=fdrift_f;
		#if (N0_1SEC_PRINT==0)		
   		debug_printf(GEORGE_PTPD_PRT, "synth update in progress: fdrift %le, for  %le, rev %le, fw %le, re %ld\n",fdrift_raw,fdrift_f,fdrift_r, forward_weight, reverse_weight);
		#endif   		
		
	}
	else
	{
//	if(Is_PTP_FREQ_VALID())
   if(is_PTP_mode==TRUE)
//	if(1)
      {
// GPZ FEB 9 2010 Disable phase control after reshape
         if(reshape > 0)
         {
            hcount_recovery_rfe = 3600;
  			   debug_printf(GEORGE_PTP_PRT, "Set hcount recovery \n");

         }
         if(hcount_recovery_rfe >0)
         {
            hcount_recovery_rfe--; 
         }
         if((hcount>0)||(hcount_rfe>0)||(reshape > 0))
         {
   			pshape_raw=pshape_base; //no shaping when both directions are not viable	
         }
         else
         {
      		if( (time_transient_flag == 0)&&(!Phase_Control_Enable)) 
	   		{  
//                  if(hcount_recovery_rfe>0)
//                  {
//	   				   pshape_raw =0.5*(sdrift_min_forward+sdrift_min_reverse);//be careful with bias sign
//                   }
                  pshape_raw = 0.0;
                  tshape_raw= - 0.5*(sdrift_min_forward+sdrift_min_reverse);//be careful with bias sign

                  Phase_Control_Enable= 1;
	   		}
	   		else 
	   		{
	   			pshape_raw = pshape_base;	
	   		}
         }
#if 0
			debug_printf(GEORGE_PTP_PRT, "phase debug pshape_raw %le pshape_smooth %le hold_rec %ld enable %ld, min_for %ld min_rev %ld\n",
            pshape_raw,
            pshape_smooth,
            (long int) hcount_recovery_rfe,
            (long int)Phase_Control_Enable,
            (long int)sdrift_min_forward,
            (long int)sdrift_min_reverse);
#endif

      }
		if(is_PTP_mode==TRUE)
		{
//		if((Is_Major_Time_Change() == TRUE) || (((forward_weight==0) && (reverse_weight==0))||((delta_forward_skip==0) && (delta_reverse_skip==0))))
//		{
//		debug_printf(GEORGE_PTP_PRT, "synth update holdover PTP: fdrift %le, fw %le, rw %le\n",fdrift_smooth, xds.xfer_weight_f, xds.xfer_weight_r);
//			if(mchan==2)
//			{
//		  		debug_printf(GEORGE_PTPD_PRT, "exception: synth update transient: fdrift %le, fw %le, rw %le\n",fdrift_raw, xds.xfer_weight_f, xds.xfer_weight_r);
//			}
//			if(!rfe_turbo_flag) //MAY 2011 do not skip while in turbo
//			{
//			#if(  (FREEZE==0) && (FREEZE_VERIFY==0))	
//				if(hcount< 7200) hcount ++;	
//				fdrift_raw = holdover_f - (fshape + tshape);			
//			#endif 
//				skip_rfe=60; //holdoff rfe function while in holdover
//			}
//		}
//		else if(hcount_rfe>0)
		if(hcount_rfe>0)
		{
			debug_printf(GEORGE_PTP_PRT, "synth update rfe holdover PTP: hcount: %ld, hcount_rfe: %ld, hold_rec: %ld\n",
         hcount,hcount_rfe,hcount_recovery_rfe);
			fdrift_raw = holdover_f - (fshape + tshape);			
		}
		else  //both channels are operative
		{
 			fdrift_raw=xds.xfer_fest;
         hcount = 0;
//			#if (N0_1SEC_PRINT==0)		
// JAN 26  			debug_printf(GEORGE_PTPD_PRT, "synth update normal: fdrift %le, fw %le, rw %le\n",fdrift_raw, xds.xfer_weight_f, xds.xfer_weight_r);
 //		#endif
			
//            
//				if(hcount > 360) //restart rfe on long outage
//				{
//#if SMARTCLOCK == 1
//					start_rfe(holdover_f);//GPZ always restart rfe at this point June 2011 was fdrift use holdover
//#else
//					start_rfe(fdrift);//GPZ always restart rfe at this point
//#endif
//				}
//				hcount=0;
//				fll_settle=FLL_SETTLE*2; 
//				skip_rfe=128; //was 384
//				Forward_Floor_Change_Flag=2; //GPZ NOV 2010 force floor change during holdover recovery
//				Reverse_Floor_Change_Flag=2;
//				#if(SYNC_E_ENABLE==1)
//				start_se(0.0); //for now assume no residual error
//				#endif
//				
//			}				
		}
		} // logic when PTP is active
		else  
		{
			int ret1, ret2;

            if(hcount_rfe == 0)
            {
         		if((time_transient_flag == 0)&&(!Phase_Control_Enable))
	      		{  
                     tshape_raw= - 0.5*(sdrift_min_forward+sdrift_min_reverse);//be careful with bias sign
                     Phase_Control_Enable= 1;
	      		}
            }
         #if 0
			debug_printf(GEORGE_PTP_PRT, "phase debug tshape_raw %le hold_rec %ld enable %ld, min_for %ld min_rev %ld\n",
            tshape_raw,
            (long int) hcount_recovery_rfe,
            (long int)Phase_Control_Enable,
            (long int)sdrift_min_forward,
            (long int)sdrift_min_reverse);
         #endif
         #if(NO_PHY==0)
			ret1 = Is_Active_Freq_Reference_OK();
			ret2 = Is_Major_Time_Change();
#if 0 /*saisi*/
			    if((Is_Active_Freq_Reference_OK() ==FALSE) || (Is_Major_Time_Change() == TRUE))
#endif
				if((ret1 == FALSE) || (ret2 == TRUE))
			    {
					if(hcount< 7200) hcount ++;	
			    	printf("555555..%s: hcount = %d..%d...%d..\n", __FUNCTION__, hcount, ret1, ret2);	
					fdrift_raw=fdrift_smooth- (fshape + tshape);			
			    }
			    else
			    {
					fdrift_raw=xds.xfer_fest;
               hcount_rfe = 0; //GPZ DEC 2011 Make sure the we are not using RFE holdover control
//					#if (N0_1SEC_PRINT==0)		
//  					debug_printf(GEORGE_PTPD_PRT, "synth update normal: fdrift %le, fw %le, rw %le\n",fdrift_raw, xds.xfer_weight_f, xds.xfer_weight_r);
//  					#endif
					if(hcount >0)
					{
#if SMARTCLOCK == 0
                  #if(NO_PHY==0)
                  if(hcount >3600)
                  {
						   init_rfe_phy(); //GPZ DEC 2011 Dont restart phy
                     start_rfe_phy(holdover_f);
                  }
                  #endif
#endif
//						if(hcount > 3600) //restart rfe on long outage
//						{
//							start_rfe_phy(fdrift);
//						}
         			debug_printf(GEORGE_PTP_PRT, "restart PHY rfe holdover recover: %ld\n", hcount);
                  hcount = 0;
#if SMARTCLOCK == 0
						PHY_Floor_Change_Flag[0]=2;//must change floor to avoid mixing observation window times
						PHY_Floor_Change_Flag[1]=2;//must change floor to avoid mixing observation window times
						PHY_Floor_Change_Flag[2]=2;//must change floor to avoid mixing observation window times
						PHY_Floor_Change_Flag[3]=2;//must change floor to avoid mixing observation window times
						PHY_Floor_Change_Flag[4]=2;//must change floor to avoid mixing observation window times
						PHY_Floor_Change_Flag[5]=2;//must change floor to avoid mixing observation window times
//                 #if(NO_PHY==0)
//						start_rfe_phy(fdrift); //renormalize RFE data to 16x speed //GPZ DEC 2011 Dont restart phy
//                  #endif
#endif
						skip_rfe_phy=PHY_SKIP;  //TEST TEST TEST was 32 //GPZ DEC 2011 Dont restart phy
						#if(SYNC_E_ENABLE==1)
						start_se(0.0); //for now assume no residual error
						#endif
					}
			    }
			#endif				
			
		} // logic when GPS is active
	} 
	// Optional shaping to keep accumulated errors bounded
	if(phase_new_cnt<86400)
	{
		phase_new_cnt++;
	}
	else
	{
		phase_new_cnt=0;
	}
		//GPZ make quiet adjustment if pshape exceeds 1000ns
//		if((phase_new_cnt>56000) && ((pshape_smooth>pshape_range_use)||(pshape_smooth<-pshape_range_use))) //eight hour since last reshape activity
		if((phase_new_cnt>1000) && ((pshape>pshape_range_use)||(pshape<-pshape_range_use))&&(delta_forward_skip==1)&&(delta_reverse_skip==1)) //check every 20 minutes for excess phase shape
		{
			if((mode_jitter==0)&& (scw_f<5000)&& (scw_r<5000)) //was 2000
			{
				phase_new_cnt=0;
				#if (N0_1SEC_PRINT==0)		
		  		debug_printf(GEORGE_PTP_PRT, "exception: re-center pshape quiet period timeout: pshape %le, pshape_base  %le\n",pshape,pshape_base);
  				#endif
				Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_RECENTER_QUIET_TIMEOUT);
                clr_phase_flag=1;
			}		
	}
	if((pshape_raw-pshape_base) > 4000.0)
	{
		#if (N0_1SEC_PRINT==0)		
	  	debug_printf(GEORGE_PTPD_PRT, "exception: pshape step: pshape %le, pshape_base  %le, forward_weight %le, reverse_weight %le, mf %ld, mr %ld \n",pshape,pshape_base, forward_weight,reverse_weight,sdrift_min_forward,sdrift_min_reverse);
	  	#endif
	}
	else if((pshape_raw-pshape_base) < -4000.0)
	{
		#if (N0_1SEC_PRINT==0)		
	  	debug_printf(GEORGE_PTPD_PRT, "exception: pshape step: pshape %le, pshape_base  %le, forward_weight %le, reverse_weight %le, mf %ld, mr %ld \n",pshape,pshape_base, forward_weight,reverse_weight,sdrift_min_forward,sdrift_min_reverse);
	  	#endif
	}
   if(0)
//	if(hcount >HUNDREDF) // long enough that rearrangement transient won't automatically trip
	{
		if(hcount<200)
		{
			#if (N0_1SEC_PRINT==0)		
  			debug_printf(GEORGE_PTP_PRT, "exception: re-center pshape holdover: pshape %le, pshape_base  %le\n",pshape,pshape_base);
  			#endif
			if(read_exc_gate(LOG_EXC_RECENTER_BRIDGING))
			{
				Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_RECENTER_BRIDGING );
				set_exc_gate(LOG_EXC_RECENTER_BRIDGING);
			}	
		}
  		
		clr_residual_phase();  
//		if(dsl_flag)
//		{
//			init_pshape_long();	
//		}
//		else
//		{
//			init_pshape();	
//		}
		init_pshape();	
	}
	if(clr_phase_flag==1)
	{
		clr_residual_phase();  
//		if(dsl_flag)
//		{
//			init_pshape_long();	
//		}
//		else
//		{
//			init_pshape();	
//		}		
		clr_phase_flag=0;
	}	
//	if((fllstatus.cur_state== FLL_FAST) || (fllstatus.cur_state== FLL_WARMUP))
//	if((fllstatus.cur_state== FLL_WARMUP))
//	{
//		clr_residual_phase();  
// 	}
	if(reshape >0)
	{
// 		cal_for_loop();	
// 		cal_rev_loop();
//		if((rfe_turbo_flag)&&(pshape_base_old!=0.0)) //GPZ MAY 2011 
//		{
//			if(reshape > 16) reshape ==16;
//		}
 		if(dsl_flag)
 		{
			reshape_jam_thres =500; 
		}
		else
		{
			reshape_jam_thres =200; //was 120 increase for better settling Jan 2010
		}	
		if(reshape > reshape_jam_thres)
		{
			pshape_base=pshape_raw;
			#if (N0_1SEC_PRINT==0)		
			if(reshape%16==0)
			{
				debug_printf(GEORGE_PTP_PRT, "reshape jam: pshape %le, pshape_base  %le, reshape %d\n",pshape,pshape_base,reshape);
			}	
			#endif
			
		}
		else	
		{
			pshape_base= PER_95*pshape_base +PER_05*pshape_raw;
		}	
		#if (N0_1SEC_PRINT==0)		
		if(reshape%16==0)
		{
			debug_printf(GEORGE_PTP_PRT, "reshape smooth: pshape_raw %le, pshape_base  %le,pshape_base_old  %le, reshape %d,pshape_new_cnt %ld\n",pshape_raw,pshape_base,pshape_base_old,reshape,phase_new_cnt);
		}	
		#endif
		if(time_transient_flag==0)
		{
			reshape--;
		}
		if(reshape==10) //test if new base is worth switching too
		{
		
			phase_new_cnt=0;
			delta_pshape=(pshape_base_old-pshape_base);
			if(delta_pshape<0.0) delta_pshape=-delta_pshape;
			if(dsl_flag==1)
			{
				if((delta_pshape) > (double)(0.7*step_thres_oper)) //was 4000
				{
					pshape_base_old=pshape_base;
					pshape_smooth=ZEROF;
					pshape=ZEROF;
					pshape_raw= pshape_base;
					#if (N0_1SEC_PRINT==0)		
					debug_printf(GEORGE_PTP_PRT, "exception: reshape use new base  outside 0.5 window\n");
					#endif
					Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_RESHAPE_USE_NEW_BASE);
					Forward_Floor_Change_Flag=1;
					Reverse_Floor_Change_Flag=1;
					skip_rfe=128; //was 384
					reshape=0;
				}
				else if(FLL_Elapse_Sec_Count_Use<7200) //use new base at start
				{
					pshape_base_old=pshape_base;
					pshape_smooth=ZEROF;
					pshape=ZEROF;
					pshape_raw= pshape_base;
					#if (N0_1SEC_PRINT==0)		
					debug_printf(GEORGE_PTP_PRT, "exception: reshape use new base \n");
					#endif
					Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_RESHAPE_USE_NEW_BASE);
					Forward_Floor_Change_Flag=1;
					Reverse_Floor_Change_Flag=1;
					skip_rfe=128;
					reshape=0;
				}
			
				else
				{
					pshape_smooth= ZEROF; //realign smoothing filter
					pshape=ZEROF;
					pshape_raw= pshape_base_old;
					pshape_base=pshape_base_old;//revert to old base
					#if (N0_1SEC_PRINT==0)		
					debug_printf(GEORGE_PTP_PRT, "exception: reshape use old base \n");
					#endif		
					Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_RESHAPE_USE_OLD_BASE);
					reshape=0;
				}
			
			}
//			else if((rfe_turbo_flag)&&(pshape_base_old!=0.0)) //GPZ MAY 2011 
//			{
//				pshape_smooth= ZEROF;//realign smoothing filter
//				pshape=ZEROF;
//				pshape_raw= pshape_base_old;
//				pshape_base=pshape_base_old;//revert to old base
//				#if (N0_1SEC_PRINT==0)		
//				debug_printf(GEORGE_PTP_PRT, "exception: reshape use old base \n");
//				#endif
//				Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_RESHAPE_USE_OLD_BASE);
//				pshape_smooth= pshape-pshape_base; //realign smoothing filter
//				reshape=0;
//			}
//			else if( (OCXO_Type()==e_RB) && (delta_pshape > 600.0) && (scw_f<20000.0)&&(scw_r<20000) ) 
//			{
//				pshape_base_old=pshape_base;
//				pshape_smooth=ZEROF;
//				pshape=ZEROF;
//				pshape_raw= pshape_base;
//				#if (N0_1SEC_PRINT==0)		
//				debug_printf(GEORGE_PTP_PRT, "exception: reshape use new base \n");
//				#endif
//				Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_RESHAPE_USE_NEW_BASE);
//				Forward_Floor_Change_Flag=1;
//				Reverse_Floor_Change_Flag=1;
//				skip_rfe=128;
//				reshape=0;
//				
//			}
			else if(  ((delta_pshape) > (double)(800.0)) &&(scw_f<20000.0)  &&(scw_r<20000)  ) 
			{
				pshape_base_old=pshape_base;
				pshape_smooth=ZEROF;
				pshape=ZEROF;
				pshape_raw= pshape_base;
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "exception: reshape use new base \n");
				#endif
				Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_RESHAPE_USE_NEW_BASE);
				Forward_Floor_Change_Flag=1;
				Reverse_Floor_Change_Flag=1;
				skip_rfe=128;
				reshape=0;
				
			}
			
			else if(FLL_Elapse_Sec_Count_Use<7200) //use new base at start
			{
				pshape_base_old=pshape_base;
				pshape_smooth=ZEROF;
				pshape=ZEROF;
				pshape_raw= pshape_base;
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "exception: reshape use new base \n");
				#endif
				Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_RESHAPE_USE_NEW_BASE);
				Forward_Floor_Change_Flag=1;
				Reverse_Floor_Change_Flag=1;
				skip_rfe=128;
				reshape=0;
			}
			
			else
			{
				pshape_smooth= ZEROF;//realign smoothing filter
				pshape=ZEROF;
				pshape_raw= pshape_base_old;
				pshape_base=pshape_base_old;//revert to old base
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "exception: reshape use old base \n");
				#endif
				Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_RESHAPE_USE_OLD_BASE);
				pshape_smooth= pshape-pshape_base; //realign smoothing filter
				reshape=0;
			}
			
		}
	}
	pshape=pshape_raw-pshape_base;
	pshape_smooth= 0.994*pshape_smooth+0.006*pshape; //GPZ April 2010 slow from .01 to .004
	if(pshape_smooth >pshape_range)
	{
		 pshape_smooth=pshape_range;
	}	 
	else if(pshape_smooth <-pshape_range)
	{
		 pshape_smooth=-pshape_range;
	}	 
	if(scw_f>scw_r) dtemp = scw_f/scw_r;
	else dtemp= scw_r/scw_f;	
	pshape_tc_old=pshape_tc_cur; // save previous time constant selection 

	delta_pshape= pshape;
	if(delta_pshape<0.0) delta_pshape=-delta_pshape;
//   if(0)
   if(delta_pshape > 12000.0)
   {
      init_pshape();
      pshape= 0.0;
      pshape_smooth = 0.0;
		debug_printf(GEORGE_PTP_PRT, "phase control: excessive pshape re-init\n");

   }

	if((ptpTransport==e_PTP_MODE_MICROWAVE)||(ptpTransport==e_PTP_MODE_DSL)) //skip phase control under microwave transport
	{
		pshape_tc_cur=ZEROF;
		fshape=ZEROF;
		#if (N0_1SEC_PRINT==0)		
		if(Print_Phase % 64 ==4)
		{
			debug_printf(GEORGE_PTP_PRT, "phase control: pshape suspended: pshape %le, tc  %le\n",pshape_smooth,pshape_tc_cur);
		}
		#endif	
	}
   else
   {
     if((time_transient_flag == 0)&&(hcount_rfe==0))
     { 
         if( ((-fbias) < 4000.0) && (rbias <4000.0) )  //was 4000
         {
		      pshape_tc_cur= 120.0;  
         }
         else
         {
		      pshape_tc_cur=pshape_norm_fine_c;
         }
			debug_printf(GEORGE_PTP_PRT, "phase control: active: pshape %le, tc  %le\n",pshape_smooth,pshape_tc_cur);
     }
     else
     {
     		pshape_tc_cur=ZEROF;
   		fshape=ZEROF;
     }  
   }	
	
//	GPZ April 2010 apply new phase contour 
	if(pshape_tc_cur>HUNDREDF)
	{
		if(pshape_tc_cur > pshape_tc_old)
		{
//			pshape_tc_cur  =0.97*(pshape_tc_old) + 0.03*(pshape_tc_cur); //was 0.99 speed up transition
			pshape_tc_cur  =0.1*(pshape_tc_old) + 0.9*(pshape_tc_cur); //April 2011 very rapid increase in time constant
		}
		else
		{
//			pshape_tc_cur  =0.97*(pshape_tc_old) + 0.03*(pshape_tc_cur); 
			pshape_tc_cur  =0.98*(pshape_tc_old) + 0.02*(pshape_tc_cur); //April 2011 very slow decrease in time constant
		}

		fshape= (pshape_smooth)/pshape_tc_cur;
//		fshape= (pshape_raw)/128.0; //TEST TEST TEST experimental drive fast response
		#if (N0_1SEC_PRINT==0)		
		if(Print_Phase % 64 ==4)
		{
			debug_printf(GEORGE_PTP_PRT, "new pshape tc: pshape %le, tc  %le\n",pshape_smooth,pshape_tc_cur);
		}	
		#endif
	}	
	else
	{
		pshape_tc_cur=pshape_tc_old;
	}
	if(ptpTransport==e_PTP_MODE_SLOW_ETHERNET) //DEC 2010 extend settling window in slow settling conditions
	{
		fshape=0.0;
	}
	
#if ((ASYMM_CORRECT_ENABLE==1) || (HYDRO_QUEBEC_ENABLE==1))
	fshape=0.0;
#endif	
	if(Is_PTP_FREQ_ACTIVE()== FALSE) fshape=0.0;
//   fshape=fshape*10.0;
//	fshape=0.0; //TEST TEST TEST force no contouring
#if( (FREEZE==0) && (FREEZE_VERIFY==0) )	
//	if((hcount == 0)&&(hcount_rfe == 0))

	if((Is_in_PHY_mode()&&(hcount == 0)) ||
	   (Is_in_PTP_mode()&&(hcount == 0)&&(hcount_rfe == 0)))
	{
		fdrift = fdrift_raw + fshape + tshape;
	}
	else
	{
		fdrift= holdover_f;
	}
	if(var_cal_flag==1)
	{
		fdrift=cal_freq;
	}
//   fdrift=fdrift_smooth=holdover_f= 	100.0 + fshape; // Phase ONLY TESTING 
//   fdrift=fdrift_smooth=holdover_f = 125.0; // Time ONLY TESTING 
#elif (FREEZE_MOD==1)
	if((FLL_Elapse_Sec_Count_Use>3600)&&(FLL_Elapse_Sec_Count_Use%512==0))
	{
		freeze_mod = -1.0*freeze_mod;
		#if (N0_1SEC_PRINT==0)		
		debug_printf(GEORGE_PTP_PRT, "freeze_mod change: freeze_mod: %le, fdrift base:  %le\n",freeze_mod,fdrift);
		#endif
	}
#endif
#if(SYNC_E_ENABLE==1)
	fdrift_warped= fdrift_smooth + SMC[0].freq_se_corr; 
//	fdrift_warped= SMC[0].freq_se_corr; //TEST TEST TEST force all control in Sync E for now
#elif (FREEZE_MOD ==1)
//	fdrift_warped= fdrift_smooth + freeze_mod; 
	fdrift_warped= fdrift_smooth = fdrift= fdrift_raw=freeze_mod; 
	holdover_f=0.0;
#else
   if((LoadState == LOAD_GENERIC_HIGH)||(OCXO_Type()==e_RB))
   {
     fdrift_warped=holdover_f; 
   }
   else
   {
	   fdrift_warped=fdrift_smooth;
   }
#endif
//   fdrift_warped *= 1.9; //TEST TEST TEST force non-linear output
//	debug_printf(GEORGE_PTP_PRT, "fdrift_raw: %le xds.xfer_fest: %le holdover_f: %le fdrift_warped: %le fdrift_smooth: %le\n",fdrift_raw, xds.xfer_fest, holdover_f, fdrift_warped, fdrift_smooth);

	//GPZ dynamic saw tooth test code overide
//	if( ((FLL_Elapse_Sec_Count+10000)%10000)< 2000)
//	{
//		fdrift_warped= -1.5*0.0291038;
//	}
//	else if( ((FLL_Elapse_Sec_Count+10000)%10000)< 4000)
//	{
//		fdrift_warped= -0.5*0.0291038;
//	}
//	else if( ((FLL_Elapse_Sec_Count+10000)%10000)< 6000)
//	{
//		fdrift_warped=  0.5*0.0291038;
//	}
//	else if( ((FLL_Elapse_Sec_Count+10000)%10000)< 8000)
//	{
//		fdrift_warped= 1.5*0.0291038;
//	}
//	else 
//	{
//		fdrift_warped= 2.5*0.0291038;
//	}
	
//	fdrift_warped=(double)((FLL_Elapse_Sec_Count+4096)%4096) -2048.0;	
//	debug_printf(GEORGE_PTP_PRT, "fdrift_warped:%le\n", fdrift_warped);
//	fdrift_warped= 0.0001*fdrift_warped;
//	debug_printf(GEORGE_PTP_PRT, "fdrift_warped:%le\n", fdrift_warped);

	// TEST CODE example of how to use varactor warp (bypass if in cal mode)
//	fdrift_warped=Varactor_Warp_Routine(fdrift); //TEST ONLY comment out of release
//	fdrift_warped=fdrift;
	// End TEST Code	
//	fdrift=19.30; //force for test
//	AccumRate = fdrift_smooth * 1e-9 * CONV_FACTOR; /* convert to National Phy bits */
//	if(AccumRate < 0)
//	{
//		adjDirectionFlag = 0;
//		AbsAccumRate = -AccumRate;	
//	}
//	else
//	{
//		adjDirectionFlag = 1;
//		AbsAccumRate = AccumRate;		
//	}
#if 0  //GPZ MARCH 2011 move to first task on one second thread to minimize latency variation
   if(SC_FrequencyCorrection(&fdrift_warped, e_PARAM_SET))
   {
	  #if (N0_1SEC_PRINT==0)		
      debug_printf(UNMASK_PRT, "Error while running SC_FrequencyCorrection()");
      #endif
   }
//	PTPClockSetRateAdjustment(0, AbsAccumRate, 0, adjDirectionFlag);
#endif
}
///////////////////////////////////////////////////////////////////////
// This function initialize FLL loop
///////////////////////////////////////////////////////////////////////
#if 0
static void fll_init(void)
{
	
		if(Is_Sync_timeout(gm_index))
		{
			fdrift_f = pll_int_f = -(double)START_OFFSET/(1e-9*CONV_FACTOR);
			fdrift_f = pll_int_f = 0;
			pll_prop_f = 0;
			fdrift_r = pll_int_r = -(double)START_OFFSET/(1e-9*CONV_FACTOR);
			fdrift_r = pll_int_r = 0;
			pll_prop_r = 0;
		}
		else
		{
			fdrift_f = pll_int_f = -(double)START_OFFSET/(1e-9*CONV_FACTOR);
			fdrift_f = pll_int_f = 0;
			pll_prop_f = 0;
			fdrift_r = pll_int_r = -(double)START_OFFSET/(1e-9*CONV_FACTOR);
			fdrift_r = pll_int_r = 0;
			pll_prop_r = 0;
		
		}
}
#endif
///////////////////////////////////////////////////////////////////////
// This function prints offset floor data
///////////////////////////////////////////////////////////////////////

static void ofd_print(void)
{
#if(MINIMAL_PRINT==0)
	if(floor_flag)
	{
		debug_printf(GEORGE_PTP_PRT, " forward offset floor: %d  iter: %2d ,total: %6ld, sum: %6ld start: %6ld gain: %6d uslew: %6d  \n",
	        FLL_Elapse_Sec_Count_Use,
		 	ofw_cluster_iter_f,	
 		 	ofw_cluster_total_prev_f,
 		 	ofw_cluster_total_sum_prev_f,
 		 	ofw_start_f,
 		 	ofw_gain_f,
 		 	ofw_uslew_f
	        );
		debug_printf(GEORGE_PTP_PRT, " reverse offset floor: %d  iter: %2d ,total: %6ld, sum: %6ld start: %6ld gain: %6d uslew: %6d  \n",
	        FLL_Elapse_Sec_Count_Use,
		 	ofw_cluster_iter_r,	
 		 	ofw_cluster_total_prev_r,
 		 	ofw_cluster_total_sum_prev_r,
 		 	ofw_start_r,
 		 	ofw_gain_r,
 		 	ofw_uslew_r
	        );
	        
		debug_printf(GEORGE_PTP_PRT, " forward next floor: %d  total: %6ld, sum: %6ld start: %6ld \n",
	        FLL_Elapse_Sec_Count_Use,
 		 	next_ofw_cluster_total_prev_f,
 		 	next_ofw_cluster_total_sum_prev_f,
 		 	next_ofw_start_f
	        );
		debug_printf(GEORGE_PTP_PRT, " reverse next floor: %d  total: %6ld, sum: %6ld start: %6ld \n",
	        FLL_Elapse_Sec_Count_Use,
		 	next_ofw_cluster_total_prev_r,
 		 	next_ofw_cluster_total_sum_prev_r,
 		 	next_ofw_start_r
	        );
	        
	        
  	        
   }
#endif       
}
#if 0
///////////////////////////////////////////////////////////////////////
// This function prints minimum selection data 
///////////////////////////////////////////////////////////////////////
static void min_print(MIN_STRUCT * min)
{
#if(MINIMAL_PRINT==0)
	if(floor_flag)
	{
		debug_printf(GEORGE_PTP_PRT, "floor: %d  min_1024: %2d ,%6ld,%6ld\n",
	        FLL_Elapse_Sec_Count,
	        min->min_count_1024,
	        min->sdrift_min_1024,
	        min->sdrift_floor_1024
	        );
   } 
#endif      
}
#endif






///////////////////////////////////////////////////////////////////////
// This function prints forward tdev stats
///////////////////////////////////////////////////////////////////////
static void forward_tdev_print(void)
{
#if (MINIMAL_PRINT==0)
	if(tdev_flag)
	{
		debug_printf(GEORGE_PTP_PRT, "forward min_tdev ns^2: %d oper: %d, %7ld \n",
	        FLL_Elapse_Sec_PTP_Count,
	        min_oper_thres_f,
	        (long int)(forward_stats.tdev_element[TDEV_CAT].var_est)
	        	);
  	} 
#endif  	    
}
///////////////////////////////////////////////////////////////////////
// This function prints reverse tdev stats
///////////////////////////////////////////////////////////////////////
static void reverse_tdev_print(void)
{
#if (MINIMAL_PRINT==0)
	if(tdev_flag)
	{
		debug_printf(GEORGE_PTP_PRT, "reverse min_tdev ns^2: %d oper: %d, %7d \n",
	        FLL_Elapse_Sec_PTP_Count,
	        min_oper_thres_r,
	        (long int)(reverse_stats.tdev_element[TDEV_CAT].var_est)
	        );
   } 
#endif      
}
// Function gives underlying fault code for the holdover condition
// 0: None
// 1: Flow fault
// 2: Transient fault
int get_holdover_code(void)
{
	if(hold_reason_flow) return 1;
	else if(hold_reason_transient) return 2;
	else return 0;
}	

///////////////////////////////////////////////////////////////////////
// This function updates the fll control loop drive 
///////////////////////////////////////////////////////////////////////
static void update_drive(void)
{
	if(SENSE==0)
	{			
      	if(mchan == FORWARD||mchan==BOTH)
      	{
	 		sdrift_min_forward= -(	min_forward.sdrift_min_1024 - (INT32)(sdrift_cal_forward));
     	}
       	if(mchan == REVERSE||mchan==BOTH)
       	{
 	 		sdrift_min_reverse= (	min_reverse.sdrift_min_1024 - (INT32)(sdrift_cal_reverse));
       	}
	}
	else
	{			
      	if(mchan == FORWARD||mchan==BOTH)
      	{
//		    if(min_forward.min_count_1024==0||(mflag_f==0))
//    	  	{
//      			min_forward.sdrift_min_1024=sdrift_cal_forward; // force zero drive
//     		}
	 		sdrift_min_forward= (	min_forward.sdrift_min_1024 - (INT32)(sdrift_cal_forward));
     	}
       	if(mchan == REVERSE||mchan==BOTH)
       	{
//		    if(min_reverse.min_count_1024==0||(mflag_r==0))
//    	  	{
//      			min_reverse.sdrift_min_1024=sdrift_cal_reverse; // force zero drive
//     		}
 	 		sdrift_min_reverse= -(	min_reverse.sdrift_min_1024 - (INT32)(sdrift_cal_reverse));
       	}
	}
#if 0	
	// force ramp modulation for testing
	if((FLL_Elapse_Sec_Count&0x1000) == 0) //every 4096 seconds 
	{
		if(test_modulation < 2048) test_modulation+= 1;
	}
	else
	{		
		if(test_modulation > -2048) test_modulation -= 1;
	}
		sdrift_min_reverse += test_modulation;
		
#endif
     
}
///////////////////////////////////////////////////////////////////////
// This function updates the phase calibration (during warmup)
///////////////////////////////////////////////////////////////////////
static void update_phase(void)
{
		 if(mchan==FORWARD||mchan==BOTH)
		 {
			if( ((min_forward.sdrift_min_1024) < sdrift_cal_forward) && FLL_Elapse_Sec_PTP_Count<90)  //was 64 increase to 90 to allow jam
		    {
//		    	if(min_forward.sdrift_min_1024 > -1000000)
	 			 sdrift_cal_forward = (double)(min_forward.sdrift_min_1024);
			}
	 	 	else
		 	{
//		 		if(min_forward.sdrift_min_1024 > -1000000)
	  			sdrift_cal_forward += 0.1 * ((double)(min_forward.sdrift_min_1024)- sdrift_cal_forward);
	 	 	}		
		 }
		 if(mchan==REVERSE||mchan==BOTH)
		 {
			if( ((min_reverse.sdrift_min_1024) < sdrift_cal_reverse) && FLL_Elapse_Sec_PTP_Count<90) //was 64 increase to 90 to allow jam
		    {
//		    	if(min_reverse.sdrift_min_1024 > -1000000)
	 			 sdrift_cal_reverse = (double)(min_reverse.sdrift_min_1024);
			}
	 	 	else
		 	{
//		    	if(min_reverse.sdrift_min_1024 > -1000000)
	  			sdrift_cal_reverse += 0.1 * ((double)(min_reverse.sdrift_min_1024)- sdrift_cal_reverse);
	 	 	}		
		 }
		if((FLL_Elapse_Sec_PTP_Count&0xF)== 0)
		{ 
//			#if (N0_1SEC_PRINT==0)		
  			debug_printf(GEORGE_PTP_PRT, "update_phase: looptime: %ld cal_forward:%ld ,cal_reverse:%ld\n",FLL_Elapse_Sec_PTP_Count,min_forward.sdrift_min_1024,min_reverse.sdrift_min_1024);
//			#endif
  		}	
		 
}
///////////////////////////////////////////////////////////////////////
// This function combines the forward and reverse channels
///////////////////////////////////////////////////////////////////////
static int rule_flag =0;
static int othres_now =2300;
static double t1=1.0;
static double t2=4.0;
static double t3=15.0;
static double twf,twr;
static double pop_gain=5.0;
//static int icdelay_f=0;
//static int icdelay_r=0;

static void combine_drive(void)
{
	double thres;
	double test,dtemp,dtemp2;
//	INT32 tsearch,twidth,dup,ddown;
//	int i, isearch, mcomp_gate;
	// TEST TEST TEST force ramp drive in both channels
//	sdrift_min_forward= FLL_Elapse_Sec_Count;
//	sdrift_min_reverse= -FLL_Elapse_Sec_Count;
	// END TEST code	
	
	//Update min samples buffers and calculate min_deltas
	delta_forward_valid =0;
	delta_reverse_valid =0;
	rule_flag=0;
	if(xds.xfer_first)
	{
		pop_gain=15.0;
	}
	else
	{
		pop_gain=5.0;
	}
	
	min_samp_forward_prev= - min_samp_forward[min_samp_phase];//prime with oldest sample
    min_samp_forward[min_samp_phase] = sdrift_min_forward; //reassign newest sample
   	delta_forward =min_samp_forward[min_samp_phase] + min_samp_forward_prev; 
	   	
	min_samp_reverse_prev= - min_samp_reverse[min_samp_phase];//prime with oldest sample
    min_samp_reverse[min_samp_phase] = sdrift_min_reverse; //reassign newest sample
   	delta_reverse =min_samp_reverse[min_samp_phase] + min_samp_reverse_prev; 
	   	
 	if(var_cal_flag==0&&(mode_jitter==0))
	{
		min_samp_phase = (min_samp_phase+1)&0x03F; //loop every  16 seconds	change to 128 Sept 2009	
   	}
	else if(mode_jitter==1)
   	{
		min_samp_phase = (min_samp_phase+1)&0x03F; //loop every  16 seconds	change to 128 Sept 2009	
	}
	else
	{
		min_samp_phase = (min_samp_phase+1)&0x0F; //loop every  16 seconds		
	}	
	
 	if(mchan==FORWARD||mchan==BOTH)
 	{
   			if((!rfe_turbo_flag) && (!min_samp_phase_flag)&&((delta_forward > pop_gain*step_thres_oper) || (delta_forward < -pop_gain*step_thres_oper))) //was 1.75, then 3.0 with new sticky phase back down to 2.0
   			{
					#if (N0_1SEC_PRINT==0)		
		    		debug_printf(GEORGE_PTP_PRT, "exception: delta_forward_pop_detected: %ld,%ld,%ld,%d\n",delta_forward,sdrift_min_forward,min_samp_forward_prev,min_samp_phase);
		    		debug_printf(GEORGE_FAST_PRT, "exception: delta_forward_pop_detected: %ld,%ld,%ld\n,%d",delta_forward,sdrift_min_forward,min_samp_forward_prev,min_samp_phase);
		    		#endif
					if(read_exc_gate(LOG_EXC_SYNC_POP_DETECTED))
					{
						Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_SYNC_POP_DETECTED );
						set_exc_gate(LOG_EXC_SYNC_POP_DETECTED);
					}	
					if(!rfe_turbo_flag)
					{					
		   			delta_forward_slew=SLEW_COUNT*6; //map timeout to general slew transient
		   			delta_forward=ZEROF;
		   			hold_reason_transient=1;
						Forward_Floor_Change_Flag=2; //GPZ OCT 2010 don't activate 
					}
					else
					{
						PTP_LOS=PTP_LOS_TIMEOUT;
					}
					if((mode_jitter==0) && (xds.xfer_first==0))
					{
						if(rfe_turbo_flag)
						{
							short_skip_cnt_f=400;
							
						}
						else
						{
							short_skip_cnt_f=45;
							
						}
					}	
					if(ptpTransport==e_PTP_MODE_SLOW_ETHERNET) //DEC 2010 extend settling window in slow settling conditions
					{
				   			delta_forward_slew=600; //map timeout to general slew transient
					}
		   			#if((ASYMM_CORRECT_ENABLE==1))
		   			delta_forward_slew=600; //map timeout to general slew transient
		   			#elif((HYDRO_QUEBEC_ENABLE==1))
		   			delta_forward_slew=600; //was 1024
					#endif
					
		   			
			}			
	   	// maximum pop validation
	   	if (((delta_forward < sacc_thres_oper)  && (delta_forward >-sacc_thres_oper) ) || (rfe_turbo_flag))
	   	{
	   		delta_forward_valid =1;
	   	}
//	   	else if(!fll_settle&&(!min_samp_phase_flag))
	   	else if((!min_samp_phase_flag))
	   	{
			#if (N0_1SEC_PRINT==0)		
	  		debug_printf(GEORGE_PTP_PRT, "exception delta_forward_range_detected: %ld\n",delta_forward);
    		debug_printf(GEORGE_FAST_PRT, "exception: delta_forward_range_detected: %ld\n",delta_forward);
    		#endif
			if(read_exc_gate(LOG_EXC_SYNC_RANGE_DETECTED))
			{
				Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_SYNC_RANGE_DETECTED );
				set_exc_gate(LOG_EXC_SYNC_RANGE_DETECTED);
			}	
			
    		
	   		delta_forward=0;
	   		delta_forward_valid =0;
   			delta_forward_slew=SLEW_COUNT*6; //GPZ tweak FEB 2009 1.01.08 
			hold_reason_transient=1;   			
	   		
	   	}
	   	//dynamic pop validation
	   	if((delta_forward_valid&&((min_samp_phase%16)==0)) && (rfe_turbo_flag==FALSE))
	   	{
	   		thres=7.0*(long int)(fllstatus.f_oper_mtdev); //use operational TDEV
	   		if( (thres>15000) && (mode_jitter==0)) thres=15000;
	   		if(thres < 11000) thres=11000; //place holder was 4500 after TC13 calibration replace with 7000
				dynamic_forward=thres;	   	
				#if ((HYDRO_QUEBEC_ENABLE==1)) //GPZ NOV 15 use regular slew for asymm
					dynamic_forward=200.0*thres;	   	
				#endif
				if(ptpTransport==e_PTP_MODE_SLOW_ETHERNET) //DEC 2010 extend settling window in slow settling conditions
				{
					#if(ASYMM_CORRECT_ENABLE==0)
					dynamic_forward=200.0*thres;	   	
					#endif
				}
				else if(ptpTransport==e_PTP_MODE_DSL)
				{
					dynamic_forward=20.0*thres;
				}
				if(delta_forward>ZEROF)
				{
					dtemp =delta_forward;
				}
				else
				{
					dtemp=-delta_forward;
				}	
	   		if((dtemp > dynamic_forward)&&(xds.xfer_first==0))
	   		{
					#if (N0_1SEC_PRINT==0)		
		   		if(combiner_flag)
		   		{
			    		debug_printf(GEORGE_PTP_PRT, "dynamic forward:%d thres=:%8ld abs delta=:%8ld delta=:%8ld sdrift_min_forward %ld,min_samp_forward_prev %ld min_samp_phase: %d\n",
		    	    	FLL_Elapse_Sec_PTP_Count,
	    	    		(long int)(dynamic_forward),
	   		    	(long int)(dtemp),
		        		(long int)(delta_forward),
		        		sdrift_min_forward,
		        		min_samp_forward_prev,
		        		min_samp_phase
		        		);
	    	    	}
		    		debug_printf(GEORGE_PTP_PRT, "exception: sync slew event detected\n");
		    		#endif
					if(read_exc_gate(LOG_EXC_SYNC_SLEW_DETECTED))
					{
						Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_SYNC_SLEW_DETECTED );
						set_exc_gate(LOG_EXC_SYNC_SLEW_DETECTED);
					}	
					// GPZ Sept 2009 Simplier approach just zero out Delta Forward	
					delta_forward=ZEROF;
					if( (Forward_Floor_Change_Flag==0) && (delta_forward_slew==0))
					{
						if((mode_jitter==0) && (xds.xfer_first==0))
						{
							if(rfe_turbo_flag)
							{
								short_skip_cnt_f=400;
							}
							else
							{
								short_skip_cnt_f=45;
							}
						}			   			
	   				delta_forward_slew=256; //map timeout to general slew transient
	   				delta_forward=ZEROF;
						Forward_Floor_Change_Flag=2; //GPZ July 2010 don't activate on dynamic
						if(ptpTransport==e_PTP_MODE_SLOW_ETHERNET) //DEC 2010 extend settling window in slow settling conditions
						{
				   			delta_forward_slew=600; //map timeout to general slew transient
						}
	   				#if((ASYMM_CORRECT_ENABLE==1))
	   				delta_forward_slew=600; //map timeout to general slew transient
	   				#elif((HYDRO_QUEBEC_ENABLE==1))
	   				delta_forward_slew=600; //was 1024
						#endif
					}		
	   		}
		   	delta_forward_prev=delta_forward;
	   	}
 	   	if (Is_Sync_timeout(gm_index) || (Is_PTP_Disabled(gm_index) && Is_in_PTP_mode())) //always test
	   	{
					if(!rfe_turbo_flag)
					{
			   		if(delta_forward_slew<SLEW_COUNT*6)
			   		{
	   					delta_forward_slew=SLEW_COUNT*6; //was 4 map timeout to general slew transient
						#if (N0_1SEC_PRINT==0)		
			   	 	debug_printf(GEORGE_PTP_PRT, "exception: sync timeout detected: %d\n",delta_forward_slew);
			    		#endif
	   				}	
	   				// just zero out delta zero
	   				delta_forward=0;
						hold_reason_flow=1;	  
					}
					else
					{
						if(PTP_LOS<6)
						{
							#if (N0_1SEC_PRINT==0)		
			   	 		debug_printf(GEORGE_PTP_PRT, "exception: sync timeout detected: %d\n",delta_forward_slew);
			    			#endif
						}
						PTP_LOS=PTP_LOS_TIMEOUT;
					} 			
   		}
	   	
   		if(delta_forward_slew) 
   		{
	   		 delta_forward_slew--;
	   		 delta_forward_skip=0;
			 delta_forward=0; //GPZ FEB 2009 Force zero drive under transient	   		 
   		}
   		else
   		{
   			 delta_forward_skip=1;
   			 hold_reason_transient=0;
   			 hold_reason_flow=0;
   		}
			if(var_cal_flag==0&&(mode_jitter==0))
			{
 		   	nfreq_forward=(double)(delta_forward)/128.0; //change from 16 to 64 2009 to 128 NOV 2010
	   	}
	   	else if(mode_jitter==1)
   		{
 		   	nfreq_forward=(double)(delta_forward)/128.0; //change from 16 to 64 2009 to 128 NOV 2010
			}
	   	else
	   	{
	   		nfreq_forward=(double)(delta_forward)/16.0; //16 for cal cycle
	   	}	
	   	
   	} // end Forward Both case
   	if(mchan==REVERSE||mchan==BOTH)
   	{
   			if((!rfe_turbo_flag) && (!min_samp_phase_flag)&&((delta_reverse > pop_gain*step_thres_oper) || (delta_reverse < -pop_gain*step_thres_oper))) //allow tighter treshold in reverse
   			{
					#if (N0_1SEC_PRINT==0)		
  		    		debug_printf(GEORGE_PTP_PRT, "exception: delta_reverse_pop_detected: %ld,%ld,%ld,%d\n",delta_reverse,sdrift_min_reverse,min_samp_reverse_prev,min_samp_phase);
		    		#endif
   					if(read_exc_gate(LOG_EXC_DELAY_POP_DETECTED))
					{
						Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_DELAY_POP_DETECTED );
						set_exc_gate(LOG_EXC_DELAY_POP_DETECTED);
					}
					if((mode_jitter==0) && (xds.xfer_first==0))
					{
						if(rfe_turbo_flag)
						{
							short_skip_cnt_r=400;
							
						}
						else
						{
							short_skip_cnt_r=45;
							
						}
					}	
					if(!rfe_turbo_flag)
					{					
		   			delta_reverse_slew=SLEW_COUNT*6; //map timeout to general slew transient
		   			delta_reverse=ZEROF;
		   			hold_reason_transient=1;
						Reverse_Floor_Change_Flag=2; //GPZ OCT 2010 don't activate 
					}
					else
					{
						PTP_LOS=PTP_LOS_TIMEOUT;
					}
					if(ptpTransport==e_PTP_MODE_SLOW_ETHERNET) //DEC 2010 extend settling window in slow settling conditions
					{
				   			delta_reverse_slew=600; //map timeout to general slew transient
					}
	   			#if((ASYMM_CORRECT_ENABLE==1))
	   			delta_reverse_slew=600; //map timeout to general slew transient
	   			#elif((HYDRO_QUEBEC_ENABLE==1))
	   			delta_reverse_slew=600; //was 1024
					#endif
			}			
	   	// maximum pop validation
	   	if(((delta_reverse < sacc_thres_oper)  && (delta_reverse >-sacc_thres_oper) ) || (rfe_turbo_flag))
	   	{
	   		delta_reverse_valid =1;
	   	}
	   	else if((!min_samp_phase_flag))
	   	{
				#if (N0_1SEC_PRINT==0)		
		  		debug_printf(GEORGE_PTP_PRT, "exception delta_reverse_range_detected: %ld\n",delta_reverse);
    			#endif
				if(read_exc_gate(LOG_EXC_DELAY_RANGE_DETECTED))
				{
					Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_DELAY_RANGE_DETECTED );
					set_exc_gate(LOG_EXC_DELAY_RANGE_DETECTED);
				}	
	   		delta_reverse=0;
	   		delta_reverse_valid =0;
  				delta_reverse_slew=SLEW_COUNT*6; //GPZ tweak FEB 2009 1.01.08 
  				hold_reason_transient=1;
			}
	   	//dynamic pop validation
	   	if((delta_reverse_valid&&((min_samp_phase%16)==0)) && (rfe_turbo_flag==FALSE))
	   	{
	   		thres=7.0*(long int)(fllstatus.r_oper_mtdev); //use operational TDEV
	   		if( (thres>15000) && (mode_jitter==0)) thres=15000;
	   		if(thres < 11000) thres=11000;
				dynamic_reverse=thres;
				#if ((HYDRO_QUEBEC_ENABLE==1)) //GPZ NOV 15 use regular slew for asymm
					dynamic_reverse=200.0*thres;	   	
				#endif
				if((ptpTransport==e_PTP_MODE_SLOW_ETHERNET))//DEC 2010 extend settling window in slow settling conditions
				{
					#if(ASYMM_CORRECT_ENABLE==0)
					dynamic_reverse=200.0*thres;
					#endif	   	
				}
				else if(ptpTransport==e_PTP_MODE_DSL)
				{
					dynamic_reverse=20.0*thres;
				}
				if(delta_reverse>ZEROF)
				{
					dtemp =delta_reverse;
				}
				else
				{
					dtemp=-delta_reverse;
				}	
	   		if((dtemp > dynamic_reverse)&&(xds.xfer_first==0))
	   		{
					#if (N0_1SEC_PRINT==0)		
		   		if(combiner_flag)
		   		{
			    		debug_printf(GEORGE_PTP_PRT, "dynamic reverse:%d thres=:%8ld abs delta=:%8ld delta=:%8ld sdrift_min_reverse %ld, min_samp_reverse_prev %ld, min_samp_phase: %d\n",
		    	    	FLL_Elapse_Sec_PTP_Count,
	    	    		(long int)(dynamic_reverse),
	   		    	(long int)(dtemp),
		        		(long int)(delta_reverse),
						sdrift_min_reverse,		
						min_samp_reverse_prev,					        	
						min_samp_phase		        	
		        		);
	    	    	}
		    		debug_printf(GEORGE_PTP_PRT, "exception: delay slew event detected\n");
		    		debug_printf(GEORGE_FAST_PRT, "exception: delay slew event detected\n");
					#endif		    	
					if(read_exc_gate(LOG_EXC_DELAY_SLEW_DETECTED))
					{
						Send_Log(LOG_DEBUG, LOG_TYPE_EXCEPTION, 0 , 0, LOG_EXC_DELAY_SLEW_DETECTED );
						set_exc_gate(LOG_EXC_DELAY_SLEW_DETECTED);
					}	
					// GPZ Sept 2009 Simplier approach just zero out Delta Revese	
					delta_reverse=ZEROF;
					if(	Reverse_Floor_Change_Flag==0 && (delta_reverse_slew==0))
					{
						if((mode_jitter==0) && (xds.xfer_first==0))
						{
							if(short_skip_cnt_f==0)
							{
								if(rfe_turbo_flag)
								{
									short_skip_cnt_r=400;
								}
								else
								{
									short_skip_cnt_r=45;
								}
						}
						
					}			   			
		   		delta_reverse_slew=256; //map timeout to general slew transient
		   		delta_reverse=ZEROF;
					Reverse_Floor_Change_Flag=2;
					if(ptpTransport==e_PTP_MODE_SLOW_ETHERNET) //DEC 2010 extend settling window in slow settling conditions
					{
				   			delta_reverse_slew=600; //map timeout to general slew transient
					}
		   		#if((ASYMM_CORRECT_ENABLE==1))
		   		delta_reverse_slew=600; //map timeout to general slew transient
		   		#elif((HYDRO_QUEBEC_ENABLE==1))
		   		delta_reverse_slew=600;
					#endif
				
				}		
   		}
	  	 	delta_reverse_prev=delta_reverse;
	  	 	
	   	}
	   	if (Is_Delay_timeout() || (Is_PTP_Disabled(gm_index) && Is_in_PTP_mode())) //always test for this case
	   	{
					if(!rfe_turbo_flag)
					{
			   		if(delta_reverse_slew<SLEW_COUNT*6)
			   		{
			   			delta_reverse_slew=SLEW_COUNT*6; // was 4 map timeout to general slew transient
							#if (N0_1SEC_PRINT==0)		
			   	 		debug_printf(GEORGE_PTP_PRT, "exception: delay timeout detected:%d\n",delta_reverse_slew);
			    			#endif
	   				}
	   				delta_reverse=0;
	   				hold_reason_flow=0;
					}
					else
					{
						if(PTP_LOS < 6)
						{
							#if (N0_1SEC_PRINT==0)		
			   	 		debug_printf(GEORGE_PTP_PRT, "exception: delay timeout detected: %d\n",delta_forward_slew);
			    			#endif
						}
						PTP_LOS=PTP_LOS_TIMEOUT;
					}
   		}
   		if(delta_reverse_slew)
   		{
	   		 delta_reverse_slew--;
	   		 delta_reverse_skip=0;
	   		 delta_reverse=0; //GPZ FEB 2009 force zero drive under transient
   		}
   		else 
   		{
   			delta_reverse_skip=1;
   			hold_reason_transient=0;
   			hold_reason_flow=0;
   		}
		if(var_cal_flag==0&&(mode_jitter==0))
		{
//		   	nfreq_reverse=(double)(delta_reverse)/64.0; //change from 16 to 64 2009
 		   	nfreq_reverse=(double)(delta_reverse)/256.0; //change from 16 to 64 2009
		}
	   	else if(mode_jitter==1)
   		{
 		   	nfreq_reverse=(double)(delta_reverse)/256.0; //change from 16 to 64 2009
		}
		else
		{   	
		   	nfreq_reverse=(double)(delta_reverse)/16.0; //change from 16 to 64 2009
		}   	
 	}  //end Reverse Both case
	if(min_samp_phase==0)
	{
		if(min_samp_phase_flag) min_samp_phase_flag --;
	}	
	// combining rules start here
	weight_forward_flag=0;
	weight_reverse_flag=0;
	twf=ZEROF;
	twr=ZEROF;
	rule_flag = 0x1;
	// validity combining rules
	if(!delta_forward_valid)
	{
		twf=ZEROF;
		weight_forward_flag=1;
		if(delta_reverse_valid)
		{
			if(delta_reverse_slew>SLEW_COUNT)
			{
				twr=ZEROF;
			}
			else
			{
			twr=1.0;
			
			}
			weight_reverse_flag=1;
		}
		else
		{
			twr=ZEROF;
			weight_reverse_flag=1;
		}
					
	}
	if(!delta_reverse_valid)
	{
		twr=ZEROF;
		weight_reverse_flag=1;	
		if(delta_forward_valid)
		{
			if(delta_forward_slew>SLEW_COUNT)
			{
				twf=ZEROF;
			}
			else
			{
				twf=1.0;
			
			}	
			weight_forward_flag=1;
		}
		else
		{
			twf=ZEROF;
			weight_forward_flag=1;
		}
	}
	// slew combining rules
	if(	(weight_forward_flag==0) && (weight_reverse_flag ==0)) // weights still unassigned
	{
		rule_flag |= 0x2;
#if 0 //GPZ bypass weight re-aasignment under slew		
		if(delta_forward_slew || delta_reverse_slew )// if any slew activity default assign
		{
			#if (N0_1SEC_PRINT==0)		
	    	debug_printf(GEORGE_PTPD_PRT, "pick default slew weight assignment\n");
	    	#endif
		
			if(delta_reverse_valid)
			{
				twr=1.0;
				weight_forward_flag=1;
				twf=ZEROF;
				weight_reverse_flag=1;
			}
			else
			{
				twf=1.0;
				weight_forward_flag=1;
				twr=ZEROF;
				weight_reverse_flag=1;
			}	
		}
		if(delta_forward_slew && (!delta_reverse_slew&&delta_reverse_valid))
		{
//			#if (N0_1SEC_PRINT==0)		
//	    	debug_printf(GEORGE_PTPD_PRT, "pick reverse better slew weight assignment\n");
//	    	#endif
		
			twf=ZEROF;
			weight_forward_flag=1;
			twr=1.0;
			weight_reverse_flag=1;
					
		}
		else if(delta_reverse_slew &&(!delta_forward_slew&&delta_forward_valid))
		{
//			#if (N0_1SEC_PRINT==0)		
//	    	debug_printf(GEORGE_PTPD_PRT, "pick forward better slew weight assignment\n");
//	    	#endif
		
			twr=ZEROF;
			weight_reverse_flag=1;	
			twf=1.0;
			weight_forward_flag=1;
		}
#endif		
		
	}//slew combining rules complete
	// min count combining rules
	if((weight_forward_flag==0) && (weight_reverse_flag ==0)) // weights still unassigned
	{
		rule_flag |= 0x4;
#if 0 //No longer needed as operation threshold rules cover this case		
		if(min_forward.min_count_1024<3)
		{
			if( min_reverse.min_count_1024>20)
			{
				twf=ZEROF;
				weight_forward_flag=1;
				twr=1.0;
				weight_reverse_flag=1;
				fdrift_f=holdover_r;
				holdover_f= fdrift_f;
	   			sacc_f=ZEROF;
	   			sacc_f_cor=ZEROF;
 				pll_int_f = holdover_f;
				nfreq_forward_smooth=ZEROF;	
				#if (N0_1SEC_PRINT==0)		
	    		debug_printf(GEORGE_PTPD_PRT, "forward weight zero for min count\n");
				#endif		
				
			}
			else
			{
				twf=0.5;
				weight_forward_flag=1;
				twr=0.5;
				weight_reverse_flag=1;
			}	
		}
		else if(min_reverse.min_count_1024<3)
		{
			if( min_forward.min_count_1024>20)
			{
				twr=ZEROF;
				weight_reverse_flag=1;
				twf=1.0;
				weight_forward_flag=1;
				fdrift_r=holdover_f;
				holdover_r= fdrift_r;
	   			sacc_r=ZEROF;
	   			sacc_r_cor=ZEROF;
				nfreq_reverse_smooth=ZEROF;		
 				pll_int_r = holdover_r;
				S_cur_OCXO_r=ZEROF;
				fll_settle=FLL_SETTLE; 
				#if (N0_1SEC_PRINT==0)		
	    		debug_printf(GEORGE_PTPD_PRT, "reverse weight zero for min count\n");
	    		#endif

			}
			else
			{
				rwf=0.5;
				weight_forward_flag=1;
				twr=0.5;
				weight_reverse_flag=1;
			}	
		}
# endif		
	}//min count combining rules complete
	// Operational threshold rules
	if(	(weight_forward_flag==0) && (weight_reverse_flag ==0)) // weights still unassigned
	{
		rule_flag |= 0x8;
		if(	(min_oper_thres_f <othres_now) && (min_oper_thres_r <othres_now)) // weights still unassigned
		{
			if(dsl_flag)
			{
				twf=0.5; 
				twr=0.5; 
			}
			else	
			{
				twf=0.2; 
				twr=0.8; 
			}
			weight_forward_flag=1;

			weight_reverse_flag=1;
			othres_now=2300;
		}
		else if(min_oper_thres_f>= min_oper_thres_r )
		{
			othres_now=1700;
			test= (min_oper_thres_f-min_oper_thres_r)/min_oper_thres_r;
			if(test< t1)
			{
				if(dsl_flag)
				{
					 test=0.6;
				} 
				else
				{
					 test=0.8;
				} 
//				 test=0.8; //was 0.8
				 t1= 1.0;
				 t2= 4.0;
				 t3= 15.0;
			}	 
			else if(test< t2)
			{
				 test=0.85; //was 0.85 prev VF
				 t1= 0.7;
				 t2= 4.0;
				 t3= 15.0;
			} 
			else if(test< t3)
			{
				 test=0.90;
				 t1= 0.7;
				 t2= 3.3;
				 t3= 15.0;
			} 
			else 
			{	
				test =PER_95;
				t1= 0.7;
				t2= 3.3;
				t3= 12.0;
			}
			twf=(1.0-test);
			weight_forward_flag=1;
			twr=test;
			weight_reverse_flag=1;
		}	
		else if(min_oper_thres_f< min_oper_thres_r )
		{
			othres_now=1700;
			test= (min_oper_thres_r-min_oper_thres_f)/min_oper_thres_f;
			if(test< t1)
			{
				if(dsl_flag)
				{
					 test=0.6;
				} 
				else
				{
					 test=0.25;
				} 

				 t1= 1.0;
				 t2= 4.0;
				 t3= 15.0;
			}	 
			else if(test< t2)
			{
				if(dsl_flag)
				{
					 test=0.7;
				} 
				else
				{
					 test=0.35;
				} 

				 t1= 0.7;
				 t2= 4.0;
				 t3= 15.0;
			} 
			else if(test< t3)
			{
				if(dsl_flag)
				{
					 test=0.8;
				} 
				else
				{
					 test=0.4;
				} 

				 t1= 0.7;
				 t2= 3.3;
				 t3= 15.0;
			} 
			else 
			{	
				test =0.5;
				t1= 0.7;
				t2= 3.3;
				t3= 12.0;
			}
			twf=(test);
			weight_forward_flag=1;
			twr=(1.0-test);
			weight_reverse_flag=1;
		}
		// Mode compression overide reverse is better even though forward is smaller FEB 2009/
		// was x > 3.0*y reduced to 2.0 to more agressively deweight June 2009
		
		if(forward_stats.tdev_element[TDEV_CAT].var_est > (2.0*reverse_stats.tdev_element[TDEV_CAT].var_est) )
		{
			if((smw_f<40000.0)&&(smw_f>14000.0)&&(smw_r>(smw_f+500.0)))
			{
					twf=PER_01;
					weight_forward_flag=1;
					twr=PER_99;
					weight_reverse_flag=1;
					#if (N0_1SEC_PRINT==0)		
					if(Print_Phase % 64 ==0)
					{
		    			debug_printf(GEORGE_PTP_PRT, "combiner: forward mode compression detected\n");
		    		}
		    		#endif
			}
#if 0	//GPZ NOV 12 2010 			
			else if (scw_f > 1000) //ignore compression for crossover cases
			{
					twf=PER_01;
					weight_forward_flag=1;
					twr=PER_99;
					weight_reverse_flag=1;
					#if (N0_1SEC_PRINT==0)		
					if(Print_Phase % 64 ==0)
					{
		    			debug_printf(GEORGE_PTP_PRT, "combiner: forward mode excess noise detected\n");
		    		}
					#endif
			}
#endif					
		}	
		else if(reverse_stats.tdev_element[TDEV_CAT].var_est > (2.0*forward_stats.tdev_element[TDEV_CAT].var_est) )
		{
			if((smw_r<40000.0)&&(smw_r>14000.0)&&(smw_f>(smw_r+500.0)) )
			{
					twr=0.20;
					weight_forward_flag=1;
					twf=0.80;
					weight_reverse_flag=1;
					#if (N0_1SEC_PRINT==0)		
					if(Print_Phase % 64 ==4)
					{
		    			debug_printf(GEORGE_PTP_PRT, "combiner: reverse mode compression detected\n");
		    		}
		    		#endif
			}
#if 0	//GPZ NOV 12 2010 			
			else
			{
					twr=0.20;
					weight_forward_flag=1;
					twf=0.80;
					weight_reverse_flag=1;
					#if (N0_1SEC_PRINT==0)		
					if(Print_Phase % 64 ==4)
					{
		    			debug_printf(GEORGE_PTP_PRT, "combiner: reverse mode excess noise detected\n");
		    		}
		    		#endif
			}	
#endif				
		}
		// GPZ APRIL 2010 New Rule use balanced weighting if both forward and reverse noise are high
		if(	(min_oper_thres_f >10000) && (min_oper_thres_r >10000)) 
		{
			test= (min_oper_thres_r-min_oper_thres_f)/min_oper_thres_f;
			if((test<1.0) && (test>-1.0))  //balanced noise condition
			{
				twr=0.5;	
				twf=0.5;
				weight_forward_flag=1;
				weight_reverse_flag=1;
				#if (N0_1SEC_PRINT==0)		
				if(Print_Phase % 64 ==4)
				{
	    			debug_printf(GEORGE_PTP_PRT, "high noise balanced weighting condition\n");
	    		}
	    		#endif
				
			}
		}
		
		
		if(fllstatus.r_avg_tp60min > fllstatus.f_avg_tp60min) 
		{
			if(fllstatus.f_avg_tp60min<3600.0)
			{
					dtemp2= (3600.0-fllstatus.r_avg_tp60min)/(3600.0-fllstatus.f_avg_tp60min);
					if(dtemp2<0.25) dtemp2=0.025;
			}
			else
			{
					dtemp2= 1.0;
			}		
			dtemp2 = (dtemp2)*(dtemp2)*(dtemp2);//March 2009 use cubic 	
			#if (N0_1SEC_PRINT==0)		
			if(Print_Phase % 64 ==8)
			{
	   			debug_printf(GEORGE_PTP_PRT, "reverse transient weight reduction: %9.1le,%5ld,%5ld\n", dtemp2,
    			(long int) fllstatus.f_avg_tp60min,
    			(long int)fllstatus.r_avg_tp60min);
				twr= twr*dtemp2;
				twf= 1.0-twr;
				weight_forward_flag=1;
				weight_reverse_flag=1;
			}
			#endif
		}
		else if(fllstatus.r_avg_tp60min <  fllstatus.f_avg_tp60min) 
		{
			if(fllstatus.r_avg_tp60min<3600.0)
			{
					dtemp2= (3600.0-fllstatus.f_avg_tp60min)/(3600.0-fllstatus.r_avg_tp60min);
					if(dtemp2<0.1) dtemp2=0.1;
			}
			else
			{
					dtemp2= 1.0;
			}		
			dtemp2 = (dtemp2)*(dtemp2)*(dtemp2);//March 2009 use cubic 
			#if (N0_1SEC_PRINT==0)		
			if(Print_Phase % 64 ==8)
			{
 	    		debug_printf(GEORGE_PTP_PRT, "forward transient weight reduction: %9.1le,%5ld,%5ld\n", dtemp2,
    			(long int) fllstatus.f_avg_tp60min,
    			(long int)fllstatus.r_avg_tp60min);
				twf= twf*dtemp2;
				twr= 1.0-twf;
				weight_forward_flag=1;
				weight_reverse_flag=1;
			}
			#endif
		}
			
	}	
#if 0					
	// stability combining rules
	if(	(weight_forward_flag==0) && (weight_reverse_flag ==0)) // weights still unassigned
	{
		rule_flag |= 0xA;
		if((forward_stats.tdev_element[TDEV_CAT].var_est <(MIN_DELTA_POP*MIN_DELTA_POP))&&
			(reverse_stats.tdev_element[TDEV_CAT].var_est <(MIN_DELTA_POP*MIN_DELTA_POP)))
		{
			twf=0.5;
			weight_forward_flag=1;
			twr=0.5;
			weight_reverse_flag=1;
		}
		else if(forward_stats.tdev_element[TDEV_CAT].var_est > reverse_stats.tdev_element[TDEV_CAT].var_est )
		{
			if(reverse_stats.tdev_element[TDEV_CAT].var_est< (MIN_DELTA_POP*MIN_DELTA_POP))
			{
				test=(double)(MIN_DELTA_POP*MIN_DELTA_POP);
			}
			else
			{
				test=(double)(reverse_stats.tdev_element[TDEV_CAT].var_est);
			}	
			test=(forward_stats.tdev_element[TDEV_CAT].var_est - test)/test;
			if(test<1.0) //was 2.0
			{
				twf=0.5;
				weight_forward_flag=1;
				twr=0.5;
				weight_reverse_flag=1;
			}
			else
			{
				twf=ZEROF;
				weight_forward_flag=1;
				twr=1.0;
				weight_reverse_flag=1;
				fdrift_f=holdover_r;
				holdover_f= fdrift_f;
	   			sacc_f=ZEROF;
	   			sacc_f_cor=ZEROF;
				nfreq_forward_smooth=0.0;		
 				pll_int_f = holdover_f;
				#if (N0_1SEC_PRINT==0)		
	    		debug_printf(GEORGE_PTPD_PRT, "forward weight zero for stab thres\n");
	    		#endif
			}
			
		}
		else 
		{
			if(forward_stats.tdev_element[TDEV_CAT].var_est< (MIN_DELTA_POP*MIN_DELTA_POP))
			{
				test=(double)(MIN_DELTA_POP*MIN_DELTA_POP);
			}
			else
			{
				test=(double)(forward_stats.tdev_element[TDEV_CAT].var_est);
			}	
			test=(reverse_stats.tdev_element[TDEV_CAT].var_est - test)/test;
			if(test<1.0) //was 2.0
			{
				twf=0.5;
				weight_forward_flag=1;
				twr=0.5;
				weight_reverse_flag=1;
			}
			else
			{
				twf=1.0;
				weight_forward_flag=1;
				twr=ZEROF;
				weight_reverse_flag=1;
				fdrift_r=holdover_f;
				holdover_r= fdrift_r;
	   			sacc_r=ZEROF;
	   			sacc_r_cor=ZEROF;
				nfreq_reverse_smooth=ZEROF;		
 				pll_int_r = holdover_r;
				S_cur_OCXO_r=ZEROF;
				fll_settle=FLL_SETTLE; 	
				#if (N0_1SEC_PRINT==0)		
	    		debug_printf(GEORGE_PTPD_PRT, "forward weight zero for stab thres\n");
				#endif				

			}
			
		}
	}//end stability combining rules complete
#endif	
	// update operational weights
   	if(mchan == FORWARD)
   	{
 			reverse_weight= ZEROF;
			forward_weight= 1.0;
  	
   	}
   	else if(mchan == REVERSE)
   	{
 			reverse_weight= 1.0;
			forward_weight= ZEROF;
   	
	}
	else if(twf>twr)
	{
		if(twr> reverse_weight)
		{
			//GPZ March 2010 tie weight adjustment rate to lsf_smooth
			// moving up slowly
			reverse_weight= lsf_smooth_g2*reverse_weight +lsf_smooth_g1*twr;
			forward_weight= 1.0-reverse_weight;
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTPD_PRT, "twf %ld, twr %ld, f>r move rev up \n",(long int)(100*twf),(long int)(100*twr));
			#endif
			
		}
		else
		{
			// move down 
			reverse_weight= lsf_smooth_g2*reverse_weight +lsf_smooth_g1*twr;
			forward_weight= 1.0-reverse_weight;
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTPD_PRT, "twf %ld, twr %ld, f>r move rev down \n",(long int)(100*twf),(long int)(100*twr));
			#endif
			
		}
	
	}
	else
	{
		if(twf> forward_weight)
		{
			// moving up slowly
			forward_weight= lsf_smooth_g2*forward_weight +lsf_smooth_g1*twf;//GPZ April 2009 slow down
			reverse_weight= 1.0-forward_weight;
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTPD_PRT, "twf %ld, twr %ld, f<r move for up \n",(long int)(100*twf),(long int)(100*twr));
			#endif

		}
		else
		{
			// move down 
			forward_weight= lsf_smooth_g2*forward_weight +lsf_smooth_g1*twf;//GPZ April 2009 slow down
			reverse_weight= 1.0-forward_weight;
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTPD_PRT, "twf %ld, twr %ld, f<r move for down \n",(long int)(100*twf),(long int)(100*twr));
			#endif
		}	
	}
#if 0	
	if((forward_weight<0.001) && (reverse_weight >0.5))
	{
				fdrift_f=holdover_r;
				holdover_f= fdrift_f;
				cal_for_loop();
				fll_settle=FLL_SETTLE;	
				#if (N0_1SEC_PRINT==0)		
	    		debug_printf(GEORGE_PTPD_PRT, "twf %ld, twr %ld, forward weight zero for oper thres\n",(long int)(100*twf),(long int)(100*twr));
	    		#endif
	
	}
	else if((reverse_weight<0.001) && (forward_weight >0.5))
	{
				fdrift_r=holdover_f;
				holdover_r= fdrift_r;
				cal_rev_loop();
				fll_settle=FLL_SETTLE; 
				#if (N0_1SEC_PRINT==0)		
	    		debug_printf(GEORGE_PTPD_PRT, "twf %ld, twr %ld, reverse weight zero for oper thres\n",(long int)(100*twf),(long int)(100*twr));
	    		#endif
	}
#endif
	
}

///////////////////////////////////////////////////////////////////////
// This function prints combiner data
///////////////////////////////////////////////////////////////////////
static void combiner_print(void)
{
#if(MINIMAL_PRINT==0)
	if(combiner_flag)
	{
		debug_printf(GEORGE_PTP_PRT, "combiner part A:%d rf:%3x fw:%3ld rw:%3ld fd:%ld rd:%ld ff:%ld rf:%ld\n",
	        FLL_Elapse_Sec_PTP_Count,
	        rule_flag,
	        (long int)(forward_weight*HUNDREDF),
	        (long int)(reverse_weight*HUNDREDF),
	        (long int) weight_forward_flag,
	        (long int) weight_reverse_flag,
	        (long int)(nfreq_forward*HUNDREDF),
	        (long int)(nfreq_reverse*HUNDREDF)
	        );
		debug_printf(GEORGE_PTP_PRT, "combiner part B:%d sf: %d fm:%ld rm:%ld fv:%d rv:%d fs:%d rs:%d fc:%d rc:%d\n",
	        FLL_Elapse_Sec_PTP_Count,
	        min_samp_phase_flag,
	        (long int)sdrift_min_forward,
	        (long int)sdrift_min_reverse,
  			delta_forward_valid,
  			delta_reverse_valid,
			delta_forward_slew,
  			delta_reverse_slew,
  			delta_forward_slew_cnt,
  			delta_reverse_slew_cnt
	        );
	        
	}    
#endif	   
}
#if 0
///////////////////////////////////////////////////////////////////////
// This function updates the mode compression state machine
// Conditions to transition out of mode compression
// If #DEFINE MODE_COMPRESS IS DISABLED THEN DON'T USE
// IF Mode widths of forward or reverse are not in expected range then turn off
// If upper falling edge is unstable then don't use
// If min cluster width is too large reject compression (4000 ns seems good)
///////////////////////////////////////////////////////////////////////
static int mcompress_state=0; //mode compression state
static int mcomp_cal=0,mcomp_index=0,mcomp_skip; // control duration of baseline compression variables
static double m_comp_stab=5000.0; 
static double m_comp_cur, m_comp_prev;
static double m_comp_table[34];

// sdrift_min_forward + test_scale*(ofw_start_f)- 28500 for reference
static int mode_compress_update(void)
{
	double temp;
	// update stability metric
	mcomp_index ++;
	mcomp_skip ++;
	if(mcomp_skip%4==0) // every second interation
	{
		if(mcomp_index==32)
		{
			mcomp_index=0;
	   		debug_printf(GEORGE_PTP_PRT, "Current mode stability metric:  %le, state: %d, for_comp %ld, rev_comp %ld\n",m_comp_stab,mcompress_state,mcompress_comp_f,mcompress_comp_r);
		}	
		m_comp_cur= (double)(sdrift_min_forward -(ofw_start_f)); //only use forward channel info for now
		temp= m_comp_cur - m_comp_table[mcomp_index];
		m_comp_table[mcomp_index]=	m_comp_cur;
		if(temp <ZEROF) temp = temp*-1.0;
		m_comp_stab = 0.97*m_comp_stab+0.03* temp;
		m_comp_prev=m_comp_cur;
	}	
	if(mcompress_state) //in compression state
	{
		mcompress_comp_f=  1.2835*(ofw_start_f)- 28500;
		mcompress_comp_r= -1.4245*(ofw_start_r)+ 28500;
		if((ofw_start_f > M_COMPRESS_MAX)|| (ofw_start_f < M_COMPRESS_MIN))	
		{
			// Turn off mode compression out of range
				mcompress_comp_f =0;
				mcompress_comp_r =0;
				mcompress_state=0;
				init_pshape();
	    		debug_printf(GEORGE_PTPD_PRT, "Disable Mode Compress forward range:  %ld\n",ofw_start_f);
				
		
		}
		else if((ofw_start_r > M_COMPRESS_MAX)|| (ofw_start_r < M_COMPRESS_MIN))	
		{
			// Turn off mode compression out of range
				mcompress_comp_f =0;
				mcompress_comp_r =0;
				mcompress_state=0;
				init_pshape();
	    		debug_printf(GEORGE_PTPD_PRT, "Disable Mode Compress reverse range:  %ld\n",ofw_start_r);
		}
		else //range okay now check cluster
		{
			if(((min_oper_thres_f > 6000)|| (min_oper_thres_f < -6000))&&(mode_compression_flag!=1))	
			{
			// Turn off mode compression cluster width 
				mcompress_comp_f =0;
				mcompress_comp_r =0;
				mcompress_state=0;
				init_pshape();
	    		debug_printf(GEORGE_PTPD_PRT, "Disable Mode Compress forward cluster width:  %ld\n",min_oper_thres_f);
			
			}
			else if(((min_oper_thres_r > 6000)|| (min_oper_thres_r < -6000))&&(mode_compression_flag!=1))	
			{
			
			// Turn off mode compression cluster width 
				mcompress_comp_f =0;
				mcompress_comp_r =0;
				mcompress_state=0;
				init_pshape();
	    		debug_printf(GEORGE_PTPD_PRT, "Disable Mode Compress reverse cluster width:  %ld\n",min_oper_thres_r);
			
			}
			else if((m_comp_stab>2500.0||(mode_compression_flag==0))&&(mode_compression_flag!=1)) //now check stability Note use mode_compress_flag to gate
			{
			
				mcompress_comp_f =0;
				mcompress_comp_r =0;
				mcompress_state=0;
				init_pshape();
	    		debug_printf(GEORGE_PTPD_PRT, "Disable Mode Compress mode stability:  %le\n",m_comp_stab);
			}
		
		
		}	
	
	}
	else // in zero compression state
	{
		if((ofw_start_f < (M_COMPRESS_MAX-1000))&& (ofw_start_f > (M_COMPRESS_MIN+1000))&& (ofw_start_r < (M_COMPRESS_MAX-1000))&& (ofw_start_r > (M_COMPRESS_MIN+1000)))	
		{
			// Range criteria meet for mode compression
			
	    		debug_printf(GEORGE_PTPD_PRT, "Range Criteria meet for mode compression:for: %ld,rev: %ld\n",ofw_start_f,ofw_start_r);
				//Next check cluster width
				if((min_oper_thres_f < 5500)&& (min_oper_thres_r < 5500))	
				{
		    		debug_printf(GEORGE_PTPD_PRT, "Cluster Width Criteria meet for mode compression:for: %ld,rev: %ld\n",min_oper_thres_f,min_oper_thres_r);
					if((m_comp_stab<1500.0||(mode_compression_flag==1))&&(mode_compression_flag!=0)) //use mode_compress_flag to gate
					{
		    			debug_printf(GEORGE_PTPD_PRT, "Mode Stability Criteria meet for mode compression:for: %le,rev: %ld\n",m_comp_stab);
						init_pshape();
						mcompress_state=1;
					}
				}
		
		}
	
	
	}	
	return mcompress_state;

}
#endif
///////////////////////////////////////////////////////////////////////
// This function updates the holdover word
///////////////////////////////////////////////////////////////////////
static void holdover_forward(double hin)
{
	hprev_f=  holdover_buffer_f[hold_phase_f];//get oldest sample
   holdover_buffer_f[hold_phase_f] = hin; //reassign newest sample
	hold_phase_f= ((hold_phase_f+1)&0x1F);
   double hgain_limit;
//	holdover_f=hin;
#if 1
	if(xds.xfer_first)
	{
		holdover_f=hin;
		hprev_f=hin;
		hgain1= PER_01;
		hgain2 = (1.0- hgain1);

	}
	else if((!xds.xfer_first) && rfe_turbo_flag)
	{
			hprev_f=hin;
         if(LoadState == LOAD_GENERIC_HIGH)
         {
            if(OCXO_Type()!=e_MINI_OCXO)
            {
               hgain_limit= 0.00015;
            }
            else
            {
//               hgain_limit= 0.0003;
               hgain_limit= 0.00015;

            }
            if(hgain1 != hgain_limit)
            {
//                 if(hgain1 > 0.005) hgain1 = 0.005;
//                 hgain1 *=0.9988;
                 hgain1 = hgain_limit; 
// JAN 26      			  debug_printf(GEORGE_PTP_PRT, "rfe holdover generic adjust hgain: gain1: %le holdover_f %le in %le\n",hgain1,holdover_f,hprev_f);
            }
         }
         else if(OCXO_Type()==e_RB)
         {
            hgain1 = 0.0003;
         }
         else
         {
//			   hgain1= 0.002;
 			   hgain1= 0.004;
// 			   hgain1= 0.01;
         }
         if(Is_PTP_FREQ_ACTIVE()== FALSE)
         {
            hgain1 = PER_01;
         }
			hgain2 = (1.0- hgain1);
	   	holdover_f= hgain2*holdover_f+hgain1*hprev_f; 
//         debug_printf(GEORGE_PTP_PRT, "rfe holdover estimation: gain2: %le gain1: %le holdover_f %le in %le\n",hgain2,hgain1,holdover_f,hprev_f);

// JAN 26			debug_printf(GEORGE_PTP_PRT, "rfe holdover estimation: gain2: %le gain1: %le holdover_f %le in %le\n",hgain2,hgain1,holdover_f,hprev_f);
	}
	else
	{
		if(hgain1 >	hgain_floor)

		{
			hgain1 *= 0.9998;
			hgain2 = (1.0- hgain1);
		}				
	   	holdover_f= hgain2*holdover_f+hgain1*hprev_f; 
	   	
	}
#endif
#if SMARTCLOCK == 1
//  if(((fllstatus.cur_state!=FLL_FAST)||(fllstatus.cur_state!=FLL_WARMUP))&&(FLL_Elapse_Sec_Count_Use>14400)) 
       if((fllstatus.cur_state!=FLL_FAST)||(fllstatus.cur_state!=FLL_WARMUP)) 
      {
     		debug_printf(GEORGE_PTP_PRT, "SMARTCLOCK Holdover before: %ld\n",(long int)(holdover_f));
         holdover_f = Get_LO_Pred();
    		debug_printf(GEORGE_PTP_PRT, "SMARTCLOCK Holdover after: %ld\n",(long int)(holdover_f));
      }
#endif
}
#if 0
static void holdover_reverse(double hin)
{
	hprev_r=  holdover_buffer_r[hold_phase_r];//get oldest sample
    holdover_buffer_r[hold_phase_r] = hin; //reassign newest sample
	hold_phase_r= ((hold_phase_r+1)&0x1F);
	if(xds.xfer_first|| rfe_turbo_flag)
	{
		holdover_r=hin;
		hprev_r=hin;
	}
	else
	{
	   	holdover_r= hgain2*holdover_r+hgain1*hprev_r; 
	   	
	}
//	#if (N0_1SEC_PRINT==0)		
//	if(hold_phase_r==0)
//	{	   
//		debug_printf(GEORGE_PTP_PRT, "hold_phase_r %d, gain2: %le gain1: %le holdover_r %le in %le\n",(int)(hold_phase_r),hgain2,hgain1,holdover_r,hprev_r);
//	}
//	#endif

}
#endif

#if 0
///////////////////////////////////////////////////////////////////////
// This function updates adjustable OCXO loop timeconstants
///////////////////////////////////////////////////////////////////////
static void ocxo_gain(void)
{
//		if(OCXO<2)
	   if(OCXO_Type()<e_MINI_OCXO)
		{
			if((G1_cur_OCXO < 0.00277) && (gspeed==0)) //slow gearing once 360 sec reached
			{
				cur_gear=0.99995;
			}
			if(G1_cur_OCXO>G1_OCXO)
			{
				G1_cur_OCXO= G1_cur_OCXO*cur_gear;
				G2_cur_OCXO= G2_cur_OCXO*cur_gear*cur_gear;
			}
		}
		else
		{
		
			if(G1_cur_OCXO>G1_MOCXO)
			{
				G1_cur_OCXO= G1_cur_OCXO*cur_gear;
				G2_cur_OCXO= G2_cur_OCXO*cur_gear*cur_gear;
			}
		
		}
}
#endif

///////////////////////////////////////////////////////////////////////
// This function initializes fll
///////////////////////////////////////////////////////////////////////
static void init_fll(void)
{
   FLOAT32 f_freqCorr;
   UINT32  s_freqCorrTime;

 	fdrift_f=ZEROF;//   Get_Freq_Corr(&f_freqCorr,  &s_freqCorrTime);
//	Set_fdrift((double)f_freqCorr);
 	fdrift_r=ZEROF;
 	fll_settle=FLL_SETTLE; 
 	mchan = BOTH; //Need to support forward reverse or both
 	dynamic_forward=MIN_DELTA_POP*MIN_DELTA_POP;
 	dynamic_reverse=MIN_DELTA_POP*MIN_DELTA_POP;
 	delta_forward_valid=delta_reverse_valid=0;
 	delta_forward_slew=delta_reverse_slew=0;// dynamic slew control active
 	weight_forward_flag=weight_reverse_flag=0;
	forward_weight=reverse_weight=0;
	nfreq_forward=nfreq_reverse=ZEROF;
	var_cal_flag=0;					
	cal_freq=ZEROF;
//	printf("init freq: %ld\n", (long int)(cal_freq));

	// end min selection
	//Data for holdover routine
 	hold_phase_f=0;
 	hold_phase_r=0;
	//OCXO related parameters
	hgain1 = PER_05;
	hgain2 = PER_95;
	//end data for holdover routine

// 	stilt=sdrift_prev=sbuf[0]=sbuf[1]=stemp=0;
 	sdrift_cal_forward=sdrift_cal_reverse=sacc_f=sacc_r=dtemp=ZEROF;
 	pop_flag=0;
 	pll_int_f = ZEROF;
 	pll_int_r = ZEROF;
 	
 	sdrift=0;
 	drift=0;
 	positive=0;
 	absdrift=0;
 	abspositive=0;
 	firstTime = 256;
 	fbi = 0;
 	fbj=sindx=idletime=idle=0;
 	start_in_progress = TRUE;
	start_in_progress_PTP = TRUE;
 	pll_int_f_less_than_3ppb_count = 0;
	FLL_Elapse_Sec_Count = 0;
   FLL_Elapse_Sec_PTP_Count = 0;
 	number_of_packets = 0;
 	number_of_sync_packets[GRANDMASTER_PRIMARY] = 0;
 	number_of_sync_packets[GRANDMASTER_ACCEPTABLE] = 0;
 	number_of_delay_packets = 0;
 	TickCount = 0;
 	Print_Phase=0;
	fllstatus.f_avg_tp15min= ZEROF;
	fllstatus.r_avg_tp15min= ZEROF;
	fllstatus.f_avg_tp60min= ZEROF;
	fllstatus.r_avg_tp60min= ZEROF;
	fbias=ZEROF;
	rbias=ZEROF;
	#if (N0_1SEC_PRINT==0)		
	debug_printf(GEORGE_PTP_PRT, "Init_FLL Reset min_oper_thres:\n");
	#endif
	
	if (enterprise_flag)
	{
		min_oper_thres_f =1000;
		min_oper_thres_r =1000;
	}	
	else if(dsl_flag)
	{
		min_oper_thres_f =10000;
		min_oper_thres_r =10000;
	}
	else
	{
		min_oper_thres_f =1000;
		min_oper_thres_r =1000;
	}
	othres_now =2300;
	pshape=ZEROF;
	pshape_base=ZEROF;
	ofw_gain_f=32;	
	ofw_gain_r=32;	
	set_oper_config();

	if(mode_jitter==1) //adjust default start of key parameters for high jitter case
	{	
		pshape_tc_cur=10000.0;	
		pshape_tc_old=10000.0;
	}	

   Get_Freq_Corr(&f_freqCorr,  &s_freqCorrTime);
#if 1
   Set_fdrift((double)f_freqCorr);
#endif
   o_fll_init = TRUE;
}



///////////////////////////////////////////////////////////////////////
// This function initializes phase shaping
///////////////////////////////////////////////////////////////////////
void init_pshape(void)
{
		reshape= 360; //change Jan 2010 was 200 to 360 May 2010
}
///////////////////////////////////////////////////////////////////////
// This function initializes phase shaping
///////////////////////////////////////////////////////////////////////
void init_pshape_long(void)
{
		reshape= 600;
}


///////////////////////////////////////////////////////////////////////
// This function initializes phase shaping clears residual phase error
///////////////////////////////////////////////////////////////////////
void clr_residual_phase(void)
{
		pshape_base  = pshape_raw; //GPZ NOV 2010 fix shaping bug 
		pshape_base_old=pshape_base;
//		pshape_raw= ZEROF;
		pshape=ZEROF;	
		pshape_smooth =ZEROF;
}




#if 1
///////////////////////////////////////////////////////////////////////
// This function restarts phase-time alignment
// Wait at least 10 seconds after change to call
///////////////////////////////////////////////////////////////////////
void Restart_Phase_Time(void)
{
	restart_phase_time_flag = TRUE;
}
void Restart_Phase_Time_now(void)
{
#if 1
// t_ptpTimeStampType s_sysTime;
// INT16              i_utcOffset = 0;
   INT64 dl_phaseAdj, dl_oldphaseAdj;
	INT32 testf, testr,tof,tor;
	UINT16 index_forward;
	UINT16 index_reverse;
	UINT16 gps_index;
   INT32 GPS_Local_Offset;
	double dtime,doffset,dbias;
//	UINT32 phase_adj_ns;
	int i;	
	#if (N0_1SEC_PRINT==0)		
	debug_printf(GEORGE_PTP_PRT, "start phase calibration: \n");
	#endif
 	clr_phase_flag=1;
	
	
   time_has_been_set_once = TRUE;

#if !NO_PHY
   	// GPZ Jan 2011 different criteria to set time
   	 if(rms.time_source[0]== RM_GPS_1)
   	 {
//		#if(GPS_ENABLE==1)
			gps_index = (GPS_A_Fifo_Local.fifo_in_index);
			GPS_Local_Offset = GPS_A_Fifo_Local.offset[gps_index];
			dbias = 0.0;
			dtime = doffset = (double)GPS_Local_Offset;
//		#endif
     }
	else if(rms.time_source[0]== RM_GPS_2){			//jiwang
			gps_index = (GPS_B_Fifo_Local.fifo_in_index);
			GPS_Local_Offset = GPS_B_Fifo_Local.offset[gps_index];
			dbias = 0.0;
			dtime = doffset = (double)GPS_Local_Offset;
	}
	else if(rms.time_source[0]== RM_GPS_3){			//jiwang
			gps_index = (GPS_C_Fifo_Local.fifo_in_index);
			GPS_Local_Offset = GPS_C_Fifo_Local.offset[gps_index];
			dbias = 0.0;
			dtime = doffset = (double)GPS_Local_Offset;
	}
     else if(rms.time_source[0]== RM_RED)
   	 {
			gps_index = (RED_Fifo_Local.fifo_in_index);
			GPS_Local_Offset = RED_Fifo_Local.offset[gps_index];
			dbias = 0.0;
			dtime = doffset = (double)GPS_Local_Offset;
     }
//#if 0
//     else if(rms.time_source[0]== RM_GPS_2)
//     {
//		#if(GPS_ENABLE==1)
			//Get_GPS_Offset(&GPS_Local_Offset);
//			gps_index = (GPS_A_Fifo_Local.fifo_in_index);
//			GPS_A_Fifo_Local.offset[gps_index];
//			dbias = 0.0;
//			dtime = doffset = (double)GPS_Local_Offset;
//		#endif
//	 }
//#endif
     else 
#endif
     if( (rms.time_source[0]== RM_GM_1) || (rms.time_source[0]== RM_GM_2) )
     {
     	
	 	index_forward = PTP_Fifo_Local.fifo_in_index;
		index_reverse = PTP_Delay_Fifo_Local.fifo_in_index;
		if(SENSE==0)
		{
			testf = PTP_Fifo_Local.offset[index_forward];
			testr = PTP_Delay_Fifo_Local.offset[index_reverse];
		}
		else
		{
			testf = -PTP_Fifo_Local.offset[index_forward];
			testr = -PTP_Delay_Fifo_Local.offset[index_reverse];
		}
		tof= testf;
		tor= testr;
		// get min estimates
		for(i=0;i<128;i++)
		{
			if(testf< tof)
			{
				tof=testf;
			}
			if(testr< tor)
			{
				tor=testr;
			}
			if(SENSE==0)
			{
				testf = PTP_Fifo_Local.offset[(index_forward+FIFO_BUF_SIZE-i)%FIFO_BUF_SIZE];
				testr = PTP_Delay_Fifo_Local.offset[(index_reverse+FIFO_BUF_SIZE-i)%FIFO_BUF_SIZE];
			}
			else
			{
				testf = -PTP_Fifo_Local.offset[(index_forward+FIFO_BUF_SIZE-i)%FIFO_BUF_SIZE];
				testr = -PTP_Delay_Fifo_Local.offset[(index_reverse+FIFO_BUF_SIZE-i)%FIFO_BUF_SIZE];
			}
		}	
		if(tof>1000000000) tof -= 1000000000;
		if(tor>1000000000) tor -= 1000000000;

		doffset= (double)(tor-tof)/2.0;
		dbias= -(double)(tof+tor)/2.0;
	
		dtime= doffset;	
    
     }
	debug_printf(GEORGE_PTP_PRT, "dtime setting: %le  offset: %le bias: %le tof:%ld tor: %ld \n", dtime,doffset,dbias,tof,tor);

	sdrift_cal_forward=0;	
	sdrift_cal_reverse=0;
	//TODO Clear all time relate registers and put in fast acquire mode
	Initial_Offset_Reverse=0;
	time_bias=(double)(Initial_Offset_Reverse);
	time_bias_est = time_bias;
	time_bias_est_rate = 0.0; 
	debug_printf(GEORGE_PTP_PRT, "reset time_bias_est in restart phase time now\n");

	time_bias_base=time_bias;
	time_bias_mean=ZEROF;
	time_bias_var=ZEROF;
	tshape=ZEROF;
	psteer=1;
	time_gain=PER_01;	
	init_pshape();
   SC_PhaseOffset_Local(&dl_oldphaseAdj, e_PARAM_GET);

	if(Is_PTP_Mode_Inband())
	{
		#if (N0_1SEC_PRINT==0)		
		debug_printf(GEORGE_PTP_PRT, "In-Band Mode:%d\n",(int)Is_PTP_Mode_Inband()); 
		#endif
		Initial_Offset=dtime;  /* this may need to be 0, 0 for timestamp alignment TODO */
		Initial_Offset_Reverse=-dtime;
		
		dl_phaseAdj = -dtime;
      if(state == PTP_SET_PHASE)
      {
        SC_PhaseOffset_Local(&dl_phaseAdj, e_PARAM_SET);
        SC_PhaseOffset_Local(&dl_phaseAdj, e_PARAM_SET);
        SC_PhaseOffset_Local(&dl_phaseAdj, e_PARAM_SET);
      }
	}
	else
	{
		/* get the 1PPS set to the correct position accordding to min delays */  
		#if (N0_1SEC_PRINT==0)		
		debug_printf(GEORGE_PTP_PRT, "Out-Band Mode:%d\n",(int)Is_PTP_Mode_Inband()); 
		#endif
      dl_phaseAdj = -doffset;
      if(state == PTP_SET_PHASE)
      {
        SC_PhaseOffset_Local(&dl_phaseAdj, e_PARAM_SET);
#if 0
        SC_PhaseOffset_Local(&dl_phaseAdj, e_PARAM_SET);
        SC_PhaseOffset_Local(&dl_phaseAdj, e_PARAM_SET);
#endif
      }
		Initial_Offset=0;
		Initial_Offset_Reverse=0;
	}
		time_bias=(double)(Initial_Offset_Reverse);
		time_bias_est = time_bias;
		time_bias_est_rate = 0.0;
// JAN 26	debug_printf(GEORGE_PTP_PRT, "reset time_bias_est in restart phase time now again\n");

		time_bias_base=time_bias;
		#if (N0_1SEC_PRINT==0)		
		debug_printf(GEORGE_PTP_PRT, "RPT: initial offset: %ld reverseoffset: %ld\n", 
			Initial_Offset,
			Initial_Offset_Reverse
			);
		#endif
	Send_Log(LOG_ALARM, LOG_TYPE_RESTART_PHASE, 0, 0, 0);
	event_printf(e_PHASE_ALIGNMENT, e_EVENT_NOTC, e_EVENT_TRANSIENT,
			             sc_alarm_lst[e_PHASE_ALIGNMENT].dsc_str, (INT32)(dl_phaseAdj - dl_oldphaseAdj));
#endif	
}
#endif

///////////////////////////////////////////////////////////////////////
// This function restart FLL at detilt
///////////////////////////////////////////////////////////////////////
int Restart_FLL(void)
{
	if(state >= PTP_SET_PHASE)
	{
		if(counter_state>20)
		{
			init_fll();
			init_ipp();	
//	   debug_printf(GEORGE_PTP_PRT, "Why_init Restart_FLL\n"); //TEST TEST TEST
		
			init_tdev(); 
			init_ztie();
//			init_fit();
			init_rfe();
			#if(SYNC_E_ENABLE==1)
			init_se();
			#endif
         #if(NO_PHY==0)
			init_ipp_phy();
			#endif
			
			state=PTP_LOCKED;
			return 1;
		}
		else
		{
			return 0;
		}	
	}
	else return 0;
}
///////////////////////////////////////////////////////////////////////
// This function restart PTP Interworking Client
///////////////////////////////////////////////////////////////////////
int SC_RestartServo(void)
{
	memset((void *)&fllstatus, 0, sizeof(fllstatus));
	tstate = FLL_UNKNOWN;
	prev_tstate = FLL_UNKNOWN; 
	tstate_counter = 0;

	if(state >= PTP_START)
	{
			state=PTP_START;
			counter_state=0;
//			return 0;
	}
	CLK_Init_PTP_Tranfer_Queue();

   start_in_progress = TRUE;
   start_in_progress_PTP = TRUE;
	return 0;
}
///////////////////////////////////////////////////////////////////////
// This function starts the varactor cal cycle
///////////////////////////////////////////////////////////////////////
int Start_Vcal(void)
{
	vcal_working.Varactor_Cal_Flag=1;
	vstate=VCAL_START;
	return(1);
}
///////////////////////////////////////////////////////////////////////
// This function selects measurement mode
///////////////////////////////////////////////////////////////////////
int measmode(int mode)
{
	if((mode >= 0) && (mode < 3) )
	{
		mchan=mode;
		return 1;
	}
	else return 0;
}


//INT32 getfreq(void)
//{
//	return (INT32)((fdrift-fshape-tshape) * 100);
//}
///////////////////////////////////////////////////////////////////////
// This function average closed loop frequency error meas ppb
///////////////////////////////////////////////////////////////////////
double getfmeas(void)
{
return ((nfreq_forward_smooth+nfreq_reverse_smooth)/(-2.0));
}
int setfreq_switchover(INT32 freq)
{
			setfreq(freq,0);
			return 1;
}
///////////////////////////////////////////////////////////////////////
// This function sets the loop frequency
///////////////////////////////////////////////////////////////////////
#if 1
int setfreq(INT32 freq,INT8 isPHY)
{
			int i;
			double dfreq;
         FLOAT64 ff_freq;
			dfreq= (double)(freq)/HUNDREDF;

         ff_freq = dfreq;
         SC_FrequencyCorrection_Local(&ff_freq, e_PARAM_SET);


			min_accel=1; // force minute accel
			if((freq<1000000) && (freq >-1000000))
			{
	   			sacc_f=ZEROF;
	   			sacc_r=ZEROF;
//	   			sacc_f_cor=ZEROF;
//	   			sacc_r_cor=ZEROF;
				nfreq_forward=ZEROF;
				nfreq_reverse=ZEROF;
				nfreq_forward_smooth=ZEROF;		
				nfreq_reverse_smooth=ZEROF;	
//         #if(GPS_ENABLE==1)
				if(!isPHY)
				{
					init_rfe();
					start_rfe(dfreq);
   				FLL_Elapse_Sec_PTP_Count =3600;
 				}
				else
				{
            #if(NO_PHY==0)
//	   		debug_printf(GEORGE_PTP_PRT, "set freq restart phy \n");
					init_rfe_phy();
					start_rfe_phy(dfreq);
 
            #endif
   				FLL_Elapse_Sec_Count =3600;
   			}
//			#else
//				init_rfe();
//				start_rfe(dfreq);
//				FLL_Elapse_Sec_PTP_Count =3600;
//			#endif
			#if(SYNC_E_ENABLE==1)
				start_se(0.0);
			#endif
	         time_transient_flag=time_hold_off;   				
	   			
	   		debug_printf(GEORGE_PTP_PRT, "new freq: %ld\n", (long int)(freq));
//				holdover_f=pll_int_f;
//				holdover_r=pll_int_r;
//			   	fdrift_f= pll_int_f;
//			   	fdrift_r= pll_int_r;
//			   	fdrift_smooth=fdrift=pll_int_f;
			   	fll_settle=FLL_SETTLE;  
//				stilt=0;
				delta_forward_prev=delta_forward=0;
				delta_reverse_prev=delta_reverse=0;
				delta_forward_valid =0;
				delta_reverse_valid =0;
				min_samp_phase=0;
				forward_stats.update_phase=0;				
				reverse_stats.update_phase=0;
//	   			delta_forward_slew=SLEW_COUNT*2; //map timeout to general slew transient
//	   			delta_reverse_slew=SLEW_COUNT*2; //map timeout to general slew transient
	   			delta_forward_slew=0; //map timeout to general slew transient
	   			delta_reverse_slew=0; //map timeout to general slew transient
//				forward_weight=0;					
//				reverse_weight=0;
				var_cal_flag=0;	
				cal_freq=ZEROF;
				fllstatus.f_avg_tp15min= ZEROF;
				fllstatus.r_avg_tp15min= ZEROF;
				fllstatus.f_avg_tp60min= ZEROF;
				fllstatus.r_avg_tp60min= ZEROF;
//				fllstatus.current_state_dur=3600;				
//				fllstatus.cur_state=FLL_NORMAL;				
//				fbias=ZEROF;
//				rbias=ZEROF;
				hgain1 = PER_05;
				hgain2 = PER_95;
				init_pshape();								
				init_tdev();
				init_ztie();
				for(i=0;i<test_delay;i++)
				{
					dline_f[i]=0;
					dline_r[i]=0;
				}	
							
 				return 1;
			}
			else return 0;	
}
#endif

///////////////////////////////////////////////////////////////////////
// This function sets the loop into stabilization state
///////////////////////////////////////////////////////////////////////
int set_stabilization(INT32 dur)
{
	if(dur<7200)
	{
		fll_stabilization = dur;
		return 1;	
	}
	else
	{
		return 0;
	}
}
//static double var_fmax = 1000.0; //maximum peak freq output from var driven osc
//static double var_fmin = -1000.0; //minimum peak freq output from var driven osc
///////////////////////////////////////////////////////////////////////
// This function puts system in calibration mode (frozen update) at specified frequency
// It also configures clustering measurement for low noise operation
// 
///////////////////////////////////////////////////////////////////////
int calfreq(INT32 freq)
{
			int i;
			double dfreq;
			dfreq= (double)(freq)/HUNDREDF;
			min_accel=1; // force minute accel
			cal_freq=dfreq;
			if((freq<10000000) && (freq >-10000000))//10 ppm adjustment range
			{
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "Calibration Function :\n");
				#endif			
	   			sacc_f=ZEROF;
	   			sacc_r=ZEROF;
//	   			sacc_f_cor=ZEROF;
//	   			sacc_r_cor=ZEROF;
				nfreq_forward=ZEROF;
				nfreq_reverse=ZEROF;
				nfreq_forward_smooth=ZEROF;		
				nfreq_reverse_smooth=ZEROF;		
	   			pll_int_f=dfreq;
	   			pll_int_r=dfreq;
//	   			start_fit(dfreq); //populate LSF 
   				start_rfe(dfreq);
				#if(SYNC_E_ENABLE==1)
				start_se(0.0);
				#endif
   				
//	   			printf("new freq: %ld\n", (long int)(pll_int_f));
//	   			printf("new freq: %ld\n", (long int)(cal_freq));
				holdover_f=pll_int_f;
				holdover_r=pll_int_r;
			   	fdrift_f= pll_int_f;
			   	fdrift_r= pll_int_r;
			   	fdrift_smooth=pll_int_f;
//				stilt=0;
				delta_forward_prev=delta_forward=0;
				delta_reverse_prev=delta_reverse=0;
				delta_forward_valid =0;
				delta_reverse_valid =0;
				min_samp_phase=0;
				forward_stats.update_phase=0;				
				reverse_stats.update_phase=0;
	   			delta_forward_slew=0; //map timeout to general slew transient
	   			delta_reverse_slew=0; //map timeout to general slew transient
				var_cal_flag=1;					
				fbias=ZEROF;
				rbias=ZEROF;
				rm_size_oper = (64 > MAX_RM_SIZE)?MAX_RM_SIZE:64;
				min_oper_thres_oper=20000;
				cluster_width_max = 250000;
				step_thres_oper = 100000; //was 2000000
				sacc_thres_oper = 500000;
				cluster_inc_f = 8*CLUSTER_INC;
				cluster_inc_r = 8*CLUSTER_INC;
				mode_width_max_oper=900000;
				min_density_thres_oper=1;
				othres_now =2300;
				ofw_gain_f=32;	
				ofw_gain_r=32;	
				//setgear(1);
//				init_pshape();								
//				init_tdev();
//				init_ztie();
				for(i=0;i<test_delay;i++)
				{
					dline_f[i]=0;
					dline_r[i]=0;
				}	
							
 				return 1;
			}
			else return 0;	
}

#if 0
// Special just change frequency version
int setfreq(INT32 freq)
{
			int i;
			if((freq<100000) && (freq >-100000))
			{
	   			pll_int_f=(double)(freq/HUNDREDF);
	   			pll_int_r=(double)(freq/HUNDREDF);
//	   			printf("new freq: %ld\n", (long int)(pll_int_f));
				holdover_f=pll_int_f;
				holdover_r=pll_int_r;
			   	fdrift_f= pll_int_f;
			   	fdrift_r= pll_int_r;
			   	fdrift_smooth=pll_int_f;
			   	
			}
			else return 0;	
}
#endif
#if 0
///////////////////////////////////////////////////////////////////////
// This function sets ocxo gear speed
//////////////////////////////////////////////////////////////////////
int setgear(int mode)
{
			if(mode==0 || mode ==1)
			{
				if(mode==0) //standard speed
				{
					cur_gear=GEAR_OCXO;
					gspeed=0;

				}
				else
				{
					 cur_gear=GEAR_OCXO_FAST; // accelerated speed
					 gspeed=1;
				}	
				return 1;
			}
			else return 0;					
}
#endif
///////////////////////////////////////////////////////////////////////
// This function exits deltilt
///////////////////////////////////////////////////////////////////////
int exittilt(void)
{

	if(start_in_progress_PTP && (FLL_Elapse_Sec_PTP_Count < detilt_time_oper))
	{
		start_in_progress_PTP = 0;
		FLL_Elapse_Sec_PTP_Count =detilt_time_oper;
		fll_settle=FLL_SETTLE; 	
		return 1;
	}
	else return 0;
}
///////////////////////////////////////////////////////////////////////
// This function prints FLL configuration
///////////////////////////////////////////////////////////////////////
int get_fll_config(void)
{
		debug_printf(GEORGE_PTP_PRT, "fll config:%d measmode:%2d cfreq(ppb):%5ld mode_comp:%2d f_cdense:%3ld r_cdense:%3ld state:%2d f_15m_t:%5ld r_15m_t:%5ld \n",
	        (int)FLL_Elapse_Sec_PTP_Count,
	        (int)mchan,
	        (long int)(fdrift),
	        (int)mode_compression_flag,
	        (long int)(HUNDREDF*fllstatus.f_cdensity),
	        (long int)(HUNDREDF*fllstatus.r_cdensity),
	        (int)fllstatus.cur_state,
	        (long int)fllstatus.f_avg_tp15min,
	        (long int)fllstatus.r_avg_tp15min
	        	);
		return(1);	
}
///////////////////////////////////////////////////////////////////////
// This function enables min floor stats
///////////////////////////////////////////////////////////////////////
void set_floor(int flag)
{
	floor_flag=flag;	
}
///////////////////////////////////////////////////////////////////////
// This function enables fll stats
///////////////////////////////////////////////////////////////////////
void set_fll(int flag)
{
	fll_flag=flag;	
}
///////////////////////////////////////////////////////////////////////
// This function enables tdev stats
///////////////////////////////////////////////////////////////////////
void set_tdev(int flag)
{
	tdev_flag=flag;	
}
///////////////////////////////////////////////////////////////////////
// This function enables combiner stats
///////////////////////////////////////////////////////////////////////
void set_combiner(int flag)
{
	combiner_flag=flag;	
}
///////////////////////////////////////////////////////////////////////
// This function set min cluster value
///////////////////////////////////////////////////////////////////////
int set_min_cluster(INT32 thres)
{


			if((thres>999) && (thres< 10001))
			{
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "Set Min Cluster  min_oper_thres:\n");
				#endif
				min_oper_thres_f=min_oper_thres_r=thres; //note need to add an auto mode 	
				return 1;
			}
			else return 0;					
}

///////////////////////////////////////////////////////////////////////
// This function sets mode_compression flag 0: off 1:on 2:auto
///////////////////////////////////////////////////////////////////////
void set_mode_compression_flag(int flag)
{
	mode_compression_flag=flag;	
}

extern	int32_t	i_fpgaFd;
///////////////////////////////////////////////////////////////////////
// This function update fll status structure
///////////////////////////////////////////////////////////////////////
void fll_status(void)
{
   UINT32 dw_bridgeTime;
	double f_tin,r_tin,fden,rden;
	PTP_Ref_State gm;
	
	gm = PTP_REF_GM1;
	// Determine current clock state
	prev_tstate = tstate;
	
//   debug_printf(GEORGE_PTP_PRT, "hcount %d hcount_rfe %d is_PTP_mode %d\n", (int)hcount, (int)hcount_rfe, (int)is_PTP_mode);

	switch(tstate)
	{
    case FLL_WARMUP:
		if((!start_in_progress) || (!start_in_progress_PTP))
		{
			tstate = FLL_FAST;
		}
		break;
    case FLL_FAST:
//    	if((hcount >10)||(hcount_rfe>1)) // Really Need to Dig into this MAY 2011 when we enter bridging I lose flow !!!!!
    	if(((hcount>10)&&(Is_in_PHY_mode()))||
         (((hcount >10)||(hcount_rfe>1)) && (Is_in_PTP_mode()))) // Really Need to Dig into this MAY 2011 when we enter bridging I lose flow !!!!!
    	{
			tstate = FLL_BRIDGE;
		}
      else if(go_to_bridge)
		{
			tstate = FLL_BRIDGE;
		}
		else if((gm == PTP_REF_BRIDGE_GM1_TO_GM2) || 
				(gm == PTP_REF_BRIDGE_GM2_TO_GM1))
		{
			tstate = FLL_BRIDGE;
		}
    	else if(fll_stabilization >0)
    	{
			tstate = FLL_STABILIZE;
		}
//		else if(OCXO_Type()==e_RB) //Rb case 
//		{
//			#if (ASYMM_CORRECT_ENABLE==1)
//			if((xds.xfer_Nmax > (5))&&(xds.xfer_first<2)) //exit fast if regression is sufficiently stabilized Jan 2010
//			#else		
//			if((xds.xfer_Nmax > (30))&&(!xds.xfer_first)) //exit fast if regression is sufficiently stabilized Jan 2010
//			#endif
//			{
//				tstate = FLL_NORMAL;
//			}
//		}
		else if((OCXO_Type()==e_RB)||(OCXO_Type()==e_OCXO)||(OCXO_Type()==e_DOCXO)) //OCXO cases
		{
			if((xds.xfer_Nmax > (120))&&(!xds.xfer_first)) //exit fast if regression is sufficiently stabilized Jan 2010
			{
				tstate = FLL_NORMAL;
			}
		}
		else
		{
			if((xds.xfer_Nmax > (100))&&(!xds.xfer_first)) //exit fast if regression is sufficiently stabilized Jan 2010
			{
				tstate = FLL_NORMAL;
			}
		}
		break;
    case FLL_NORMAL:
//    	if((hcount >10)||(hcount_rfe>1)) // Really Need to Dig into this MAY 2011 when we enter bridging I lose flow !!!!!
    	if(((hcount>10)&&(Is_in_PHY_mode()))||
         (((hcount >10)||(hcount_rfe>1)) && (Is_in_PTP_mode()))) // Really Need to Dig into this MAY 2011 when we enter bridging I lose flow !!!!!
		{
			tstate = FLL_BRIDGE;
		}
      else if(go_to_bridge)
		{
			tstate = FLL_BRIDGE;
		}
		else if((gm == PTP_REF_BRIDGE_GM1_TO_GM2) || 
		        (gm == PTP_REF_BRIDGE_GM2_TO_GM1))
		{
			tstate = FLL_BRIDGE;
		}
    	else if(fll_stabilization >0)
    	{
			tstate = FLL_STABILIZE;
		}
//		else if(FLL_Elapse_Sec_Count <= detilt_time_oper)
//		{
//			tstate = FLL_FAST;
//		}  
		break;
    case FLL_BRIDGE:
      go_to_bridge = FALSE;
      Get_Bridge_Time(&dw_bridgeTime);
    	if(tstate_counter > (dw_bridgeTime))//Never goes below 2 minutes
//    	if(tstate_counter > (nvm.nv_bridge_time))//Never goes below 2 minutes
		{
			uint8_t	hold_flag = 1;
			tstate = FLL_HOLD;
			if(i_fpgaFd > 0)
				ioctl(i_fpgaFd, FPGA_SET_SERVO_HOLD, (uint32_t)&hold_flag);
		}
//		else if(((hcount == 0)&& (hcount_rfe == 0)) && 
//					!((gm == PTP_REF_BRIDGE_GM1_TO_GM2) || 
//		        	 (gm == PTP_REF_BRIDGE_GM2_TO_GM1)))
      else if(((hcount==0) && (Is_in_PHY_mode()))||
         (((hcount == 0)&& (hcount_rfe == 0)) && (Is_in_PTP_mode())))  
      {
//         if(OCXO_Type()==e_RB) //Rb case 
//   		{
//   			#if (ASYMM_CORRECT_ENABLE==1)
//   			if((xds.xfer_Nmax > (5))&&(xds.xfer_first<2)) //exit fast if regression is sufficiently stabilized Jan 2010
//   			#else		
//   			if((xds.xfer_Nmax > (30))&&(!xds.xfer_first)) //exit fast if regression is sufficiently stabilized Jan 2010
//   			#endif
//   			{
//   				tstate = FLL_NORMAL;
//   			}
//            else
//            {
//               tstate = FLL_FAST;
//            }
//   		}
   		if((OCXO_Type()==e_RB)||(OCXO_Type()==e_OCXO)||(OCXO_Type()==e_DOCXO)) //OCXO cases
   		{
   			if((xds.xfer_Nmax > (120))&&(!xds.xfer_first)) //exit fast if regression is sufficiently stabilized Jan 2010
   			{
   				tstate = FLL_NORMAL;
   			}
            else
            {
               tstate = FLL_FAST;
            }
   		}
   		else
   		{
   			if((xds.xfer_Nmax > (100))&&(!xds.xfer_first)) //exit fast if regression is sufficiently stabilized Jan 2010
   			{
   				tstate = FLL_NORMAL;
   			}
            else
            {
               tstate = FLL_FAST;
            }
   		}
      }
//		{
//			if(xds.xfer_first)
//			{
//				tstate = FLL_FAST;	
//			}
//			else
//			{
//				tstate = FLL_NORMAL;
//			}
//		}	
		break;
    case FLL_HOLD:
//		if((hcount == 0)&& (hcount_rfe == 0))
      if(((hcount==0) && (Is_in_PHY_mode()))||
         ((hcount_rfe==0) && (Is_in_PTP_mode())))
      {
			uint8_t	hold_flag = 0;
			if(i_fpgaFd > 0)
				ioctl(i_fpgaFd, FPGA_SET_SERVO_HOLD, (uint32_t)&hold_flag);
//         if(OCXO_Type()==e_RB) //Rb case 
//   		{
//   			#if (ASYMM_CORRECT_ENABLE==1)
//   			if((xds.xfer_Nmax > (5))&&(xds.xfer_first<2)) //exit fast if regression is sufficiently stabilized Jan 2010
//   			#else		
//   			if((xds.xfer_Nmax > (30))&&(!xds.xfer_first)) //exit fast if regression is sufficiently stabilized Jan 2010
//   			#endif
//   			{
//   				tstate = FLL_NORMAL;
//   			}
//            else
//            {
//               tstate = FLL_FAST;
//            }
//   		}
   		if((OCXO_Type()==e_RB)||(OCXO_Type()==e_OCXO)||(OCXO_Type()==e_DOCXO)) //OCXO cases
   		{
   			if((xds.xfer_Nmax > (120))&&(!xds.xfer_first)) //exit fast if regression is sufficiently stabilized Jan 2010
   			{
   				tstate = FLL_NORMAL;
   			}
            else
            {
               tstate = FLL_FAST;
            }
   		}
   		else
   		{
   			if((xds.xfer_Nmax > (100))&&(!xds.xfer_first)) //exit fast if regression is sufficiently stabilized Jan 2010
   			{
   				tstate = FLL_NORMAL;
   			}
            else
            {
               tstate = FLL_FAST;
            }
   		}
      }
//		{
//			if(hgain1>0.01)
//			{
//				tstate = FLL_FAST;	
//			}
//			else
//			{
//				tstate = FLL_NORMAL;
//			}
//		}		
		break;
    case FLL_STABILIZE:
		if(fll_stabilization == 0)
		{
			tstate = FLL_FAST;
		}		
		break;
		
    default:
	case FLL_UNKNOWN:
    	tstate = FLL_WARMUP;	
    	break;
    }

	if(prev_tstate != tstate)
	{
		tstate_counter = 0;
#ifndef TP500_BUILD
		if (prev_tstate == FLL_WARMUP)
			event_printf(e_FREERUN, e_EVENT_ERR, e_EVENT_CLEAR,
			             sc_alarm_lst[e_FREERUN].dsc_str, -1);
		else if (prev_tstate == FLL_BRIDGE)
			event_printf(e_BRIDGING, e_EVENT_NOTC, e_EVENT_CLEAR,
			             sc_alarm_lst[e_BRIDGING].dsc_str, -1);
		else if (prev_tstate == FLL_HOLD)
			event_printf(e_HOLDOVER, e_EVENT_ERR, e_EVENT_CLEAR,
			             sc_alarm_lst[e_HOLDOVER].dsc_str, -1);
		if (tstate == FLL_WARMUP)
			event_printf(e_FREERUN, e_EVENT_ERR, e_EVENT_SET,
			             sc_alarm_lst[e_FREERUN].dsc_str, -1);
		else if (tstate == FLL_BRIDGE)
			event_printf(e_BRIDGING, e_EVENT_NOTC, e_EVENT_SET,
			             sc_alarm_lst[e_BRIDGING].dsc_str, -1);
		else if (tstate == FLL_HOLD)
			event_printf(e_HOLDOVER, e_EVENT_ERR, e_EVENT_SET,
			             sc_alarm_lst[e_HOLDOVER].dsc_str, -1);
#endif
	}
	else
	{
		tstate_counter++;
	}
	
	if(tstate != fllstatus.cur_state) //change of state
	{
		debug_printf(GEORGE_PTP_PRT, "Change FLL State: new: %ld old: %ld\n", tstate, fllstatus.cur_state );
		syslog(LOG_BSS|LOG_DEBUG, "Change FLL State: new: %ld old: %ld", tstate, fllstatus.cur_state );
		Send_Log(LOG_ALARM, LOG_TYPE_STATE_CHANGE, (UINT32)fllstatus.cur_state, (UINT32)tstate, 0);
		fllstatus.prev_state=fllstatus.cur_state;
		fllstatus.previous_state_dur=fllstatus.current_state_dur;
		fllstatus.cur_state=tstate;
		fllstatus.current_state_dur=0;	
	}
	else
	{
		if(fllstatus.current_state_dur < 1000000000) fllstatus.current_state_dur ++;	
	}
	//TODO add lock state logic
//	typedef enum
//	{
//    Lock_Off      = 0,
//	Lock_Green_Blink,
//	Lock_Green_On,
//	Lock_Yellow_On,
//	Lock_Yellow_Blink
//	} Lock_State;
	fllstatus.lock= Lock_Off;
	if(tstate==FLL_WARMUP||tstate== FLL_FAST)
	{
		fllstatus.lock=	Lock_Green_Blink;
	}
	else if(tstate== FLL_NORMAL ||	tstate== FLL_BRIDGE)
	{
		fllstatus.lock=	Lock_Green_On;
	}
	else if(tstate==FLL_HOLD)
	{
		fllstatus.lock=	Lock_Yellow_On;
	} 
	//Update transient rate info
	if(delta_forward_slew  ) f_tin=900.0;
	else f_tin=ZEROF;
	if(delta_reverse_slew  ) r_tin=900.0;
	else r_tin=ZEROF;
	
	fllstatus.f_avg_tp15min= ((449.0)*fllstatus.f_avg_tp15min + f_tin)/450.0;
	fllstatus.r_avg_tp15min= ((449.0)*fllstatus.r_avg_tp15min + r_tin)/450.0;
	fllstatus.f_avg_tp60min= ((1799.0)*fllstatus.f_avg_tp60min + 4.0*f_tin)/1800.0;
	fllstatus.r_avg_tp60min= ((1799.0)*fllstatus.r_avg_tp60min + 4.0*r_tin)/1800.0;
	// update average cluster density
	fden=(double)(IPD.floor_cluster_count[FORWARD_GM1])/1024.0;
	rden=(double)(IPD.floor_cluster_count[REVERSE_GM1])/1024.0;
	if(fden>1.0) fden=1.0;
	if(rden>1.0) rden=1.0;
				
	fllstatus.f_cdensity= ((299.0)*fllstatus.f_cdensity +fden)/300.0;
	fllstatus.r_cdensity= ((299.0)*fllstatus.r_cdensity +rden)/300.0;
				
}

double Get_fdrift(void)
{
	return fdrift;
}
#if 0
boolean Is_FLL_Elapse_Sec_Count_gt_20(void)
{
	if(FLL_Elapse_Sec_Count > 20)
	{
		return TRUE;
	}

	return FALSE;
}

boolean Is_FLL_Elapse_Sec_Count_gt_400(void)
{
	if(FLL_Elapse_Sec_Count > detilt_time_oper)
	{
		return TRUE;
	}

	return FALSE;
}
#endif
void Set_fdrift(double freq)
{
			fbias=0;
			fdrift_f=freq;
			fll_settle=FLL_SETTLE; 
			holdover_f= fdrift_f;
   			sacc_f=ZEROF;
// 			sacc_f_cor=ZEROF;
			pll_int_f = holdover_f;
			rbias=0;
			fdrift_r=freq;
			holdover_r= fdrift_r;
   			sacc_r=ZEROF;
// 			sacc_r_cor=ZEROF;
			pll_int_r = holdover_r;
			nfreq_forward_smooth=ZEROF;		
			nfreq_reverse_smooth=ZEROF;
			var_cal_flag=0;					
		
			
/* send to synth */			
			fll_synth();		
}


///////////////////////////////////////////////////////////////////////
// This function updates time bias estimate
///////////////////////////////////////////////////////////////////////
#ifdef ENABLE_TIME
static double time_slew_limit = 1000.0;
static double time_slew_err;
static int time_slew_limit_cnt = 0;

static void update_time(void)
{
   UINT32 temp_jam_threshold;
   INT64  ll_phase_offset;
	INT32 delta_offset = 0;
	double dtime;
	double inter, inter1;
	double rate_correct;
	double tcomp_limit,dtemp;
   INT32 avg_TIME;

   switch(rms.time_source[0])
   {
   case RM_GM_1:
      break;
	case RM_GM_2:
      break;
#if (!NO_PHY)
	case RM_GPS_1:
      avg_TIME=avg_GPS_A;
      break;
	case RM_GPS_2:
      avg_TIME=avg_GPS_B;
      break;
	case RM_GPS_3:
      avg_TIME=avg_GPS_C;
      break;
	case RM_SE_1:
      avg_TIME=avg_GPS_A;
      break;
	case RM_SE_2:
      avg_TIME=avg_GPS_A;
      break;
	case RM_RED:
      avg_TIME=avg_RED;
      break;
#endif
	case RM_NONE:
      break;
   default:
      break;
	}


	if((Is_PTP_TIME_ACTIVE() == TRUE) && (time_has_been_set_once))	
	{
			// update transient flag
			time_hold_off=10;
			if(time_transient_flag==0)
			{
				time_transient_flag = 1; //assume transient condition unless proven wrong
			}
			if((PTP_LOS==0)&&(time_transient_flag==1) && (low_warning_rev == 0) && (low_warning_for == 0)
                && (high_warning_for == 0) && (high_warning_rev == 0)&&(turbo_cluster_for==0)&&(turbo_cluster_rev ==0 ))
			{
				if ((RFE.rfee_r[rfe_cindx].tfs==1 ) && 
					(RFE.rfee_f[rfe_cindx].tfs==1 ) 				
					)
				{
					time_transient_flag = 0; 
				}
			}
//         if((FLL_Elapse_Sec_Count_Use<10800)&&(FLL_Elapse_Sec_PTP_Count<10800)&&(FLL_Elapse_Sec_Count<10800))
         if(FLL_Elapse_Sec_PTP_Count<10800)
         {
            if(time_transient_flag<2)  //GPZ NOV 2011
            {
               time_transient_flag=0;  //FORCE NO TIME TRANSIENTS
 //  			debug_printf(GEORGE_PTP_PRT, "update_time  holdoff transient before 10800:\n");
            }
         }

			if(time_transient_flag != 0)
			{
				#if (N0_1SEC_PRINT==0)		

				debug_printf(GEORGE_PTP_PRT, "update_time time transient: LOSL %ld, ttf: %ld, lwr: %ld, lwf:  %ld, hwr:  %ld, hwf: %ld, tcf: %ld, tcr: %ld\n",
            (long int)PTP_LOS,
				(long int)time_transient_flag,
				(long int)low_warning_rev,
				(long int)low_warning_for,
				(long int)high_warning_rev,
				(long int)high_warning_for,
            (long int)turbo_cluster_for,
            (long int)turbo_cluster_rev);
            #endif				
				debug_printf(GEORGE_PTP_PRT, "update_time time transient: %d, %d\n",
			
				 (RFE.rfee_r[rfe_cindx].tfs ),
				 (RFE.rfee_f[rfe_cindx].tfs )
				 );			
			}
			time_bias =  ((double)(- Initial_Offset) +
	              (double)(Initial_Offset_Reverse))/2.0;
	              
			inter = -((double)(sdrift_cal_forward) -
	    	      (double)(sdrift_cal_reverse))/2;
		//		rate_correct=-3275.0; //was -2800         
		//		rate_correct= -2800.0; //was -2800       
			rate_correct=ZEROF;  
			inter=inter+rate_correct+phy_correct;	          
			//GPZ March 2009 change bias sign
//			inter1 = -((double)(sdrift_min_forward- (INT32)hold_res_acc) +
//	    		      (double)(sdrift_min_reverse - (INT32)hold_res_acc))/2;

//			inter1 = -((double)(sdrift_min_forward) +
//	    		      (double)(sdrift_min_reverse))/2;
			inter1 = tshape_raw;
//				debug_printf(GEORGE_PTP_PRT, "update_time components: cal_f: %le, cal_r: %le tbias: %le rcorr: %le, pcorr: %le\n",
//             (double)(sdrift_cal_forward),
//             (double)(sdrift_cal_reverse),
//             (double)(time_bias),
//             (double)(rate_correct),
//             (double)(phy_correct));
                 

			if(FLL_Elapse_Sec_PTP_Count<300)
			{
				round_trip_delay_alt=ZEROF;
			}
			else 
			{     
//				round_trip_delay_alt= ZEROF - ((double)Initial_Offset         -((double)(sdrift_cal_forward)+(double)(sdrift_min_forward-(INT32)hold_res_acc)))
//		          -   ((double)Initial_Offset_Reverse -((double)(sdrift_cal_reverse)-(double)(sdrift_min_reverse-(INT32)hold_res_acc))); 

				round_trip_delay_alt= ZEROF - ((double)Initial_Offset         -((double)(sdrift_cal_forward)+(double)(sdrift_min_forward)))
		          -   ((double)Initial_Offset_Reverse -((double)(sdrift_cal_reverse)-(double)(sdrift_min_reverse))); 
     

			}
			time_bias = time_bias - inter - inter1;

			#if (ASYMM_CORRECT_ENABLE==1)  //GPZ NOV 2010 NEED to move inside sensibility check to prevent phase step could be china power issue 
				#if (N0_1SEC_PRINT==0)		
				if(Print_Phase%16==11)
				{
					debug_printf(GEORGE_PTP_PRT, "asymm comp: time_bias:%le  asymm:%le \n", time_bias,ACL.asymm_calibration);
				}	
			#endif
			time_bias -= ACL.asymm_calibration;
			#endif
	}
	else //PTP is not active Time Source
	{
//				#if(GPS_ENABLE==1)
				time_hold_off=10;
				// TODO ADD Code for time transient flag is this case
            if(Is_Active_Time_Reference_OK() && (time_transient_flag < 2))
            {
               time_transient_flag=0;
            }
            else
            {  
               if(time_transient_flag <2)
               {
                  time_transient_flag = time_hold_off;
               }
            }
				if(Print_Phase%16==11)
				{
					debug_printf(GEORGE_PTP_PRT, "update_time: GPS Active Time Transient Flag: %d\n", (int)time_transient_flag);
				}	

				time_bias =  ((double)(- Initial_Offset) +
	            (double)(Initial_Offset_Reverse))/2.0;
				inter=0.0; //for GPS ignore calibration
//				inter1 = -(double)(avg_GPS_A); //do not compensate with hold_res_acc
//				inter1 = -(double)(avg_RED); //TEST TEST TEST FORCE TIME to Redundancy
            inter1 = -(double)avg_TIME;
				time_bias = time_bias - inter - inter1;
//            #else
//				time_hold_off=180;
//            inter=0.0;
//            inter1=0.0;
//            time_bias=0.0;
//				#endif
	}
	if(hcount >0)  //Use rfe version of hcount
	{
		if(time_transient_flag==0)
		{
			time_transient_flag=time_hold_off; //do not update time steering while in holdover FEB 2011
		}
	}
	#if (ASYMM_CORRECT_ENABLE==1)
	if(FLL_Elapse_Sec_Count_Use<3800)
	{
		time_transient_flag=2;
	}
	#endif	
	if(Is_Active_Time_Reference_OK() ==FALSE)
	{
		time_transient_flag=time_hold_off; //3 minute settling period TODO need a better way to verify stability of GPS
	}
	else
	{
		if(time_transient_flag>1) time_transient_flag--; //GPZ Mar 2010 don't allow it to go to zero
	}
//	if( (time_transient_flag<2) && ( (time_transient_flag==0)||(FLL_Elapse_Sec_Count_Use<5200) )   )
   if(1) //TEST TEST TEST DEC 2011 Always attempt to steer
	{
		if(enterprise_flag)
		{
			if(time_gain > 0.005) time_gain=0.005;
			if(time_gain > 0.001) time_gain*=0.9988; //was 0.993 was 0.00015 June 26 2009
			else time_gain = 0.001;
			
			tcomp_limit=0.005;
		}
		else if(OCXO_Type()== e_MINI_OCXO)
		{
         // GPZ use same time gain control as OCXO NOV 2011
			if(time_gain > 0.005) time_gain=0.005;
			if(time_gain > 0.0003) time_gain*=0.9992; //was 0.993 was 0.00015 June 26 2009 0.9988
			else time_gain = 0.0003;

			
			tcomp_limit=0.005;
		}
		else if(OCXO_Type()== e_OCXO)
		{
		
			if(time_gain > 0.005) time_gain=0.005;
			if(time_gain > 0.0003) time_gain*=0.9992; //was 0.993 was 0.00015 June 26 2009 0.9988
			else time_gain = 0.0003;
			
			tcomp_limit=0.005;
		}
		else //run slower time control for Rb
		{
			if(time_gain > 0.005) time_gain=0.005;
			if(time_gain > 0.0001) time_gain*=0.9992; //was 0.993
			else time_gain = 0.0001;
			tcomp_limit=0.005;
		}	
		if(FLL_Elapse_Sec_Count_Use<7200)//TODO may need to be 14400
		{
			if(time_gain<0.005) time_gain=0.005; //keep time gain medium speed during first 4 hours
			#if (N0_1SEC_PRINT==0)		
			if(Print_Phase%64==0)
			{
				debug_printf(GEORGE_PTP_PRT, "time_check early tgain medium jam tgain:%le \n",time_gain);
			}
			#endif

		}
		else if(min_accel==1)
			{
			if(time_gain<0.005) time_gain=0.005; //keep time gain medium speed during cal
			#if (N0_1SEC_PRINT==0)		
			if(Print_Phase%64==0)
			{
				debug_printf(GEORGE_PTP_PRT, "time_check accel tgain medium jam tgain:%le \n",time_gain);
			}	
			#endif
			}
				
		tgain_comm=(rbias-fbias)/1.75; //was 2.0 
		tgain_diff=rbias+fbias;
		// make all parameters positive for asymmetry test FIX Nov 15 2009
		if(tgain_comm<ZEROF) tgain_comm=-tgain_comm;
		if(tgain_comm>60000.0) tgain_comm=60000.0;
		if(tgain_diff<ZEROF) tgain_diff=-tgain_diff;
		dtemp=tgain_comm+tgain_diff;
		// end of parameter fix
		if(dtemp>ZEROF)
		{
			if(tgain_comm < 1000.0) //crossover conditions
			{
				tgain_comp=1.0;
			}
			else
			{
				tgain_comp= (60000.0-tgain_comm)/60000.0;
				tgain_comp= tgain_comp*(tgain_comm/(dtemp));
				tgain_comp= tgain_comp*tgain_comp;
//				tgain_comp= tgain_comp*tgain_comp;
				if(tgain_comp<tcomp_limit) tgain_comp=tcomp_limit;
				if(tgain_comp>1.0)tgain_comp=1.0;
			}
		}
		else
		{
			tgain_comp=1.0;	
		}
		ltgain=time_gain*tgain_comp;
			if(Print_Phase%64==0)
			{
				debug_printf(GEORGE_PTP_PRT, "time_check compensated:ltgain:%le  tgain:%le comp: %le\n",ltgain, time_gain, tgain_comp);
			}	

		if(tgain_comm<600.0)
		{
//			if(ltgain< 0.0003) ltgain= 0.0003; //near crossover cable conditions was 0.003
			ltgain= 0.01; //near crossover cable conditions was 0.003
			debug_printf(GEORGE_PTP_PRT, "time_check near crossover:%le \n",ltgain);
         if(rms.time_source[0] < RM_GPS_1)
            time_bias += 44.3; //GPZ crossover bias calibration Dec 2011

		}
		else if(tgain_comm< 300.0)
		{
			ltgain= 0.01; //near crossover cable conditions was 0.003

//			if(ltgain<0.003) ltgain= PER_05 ; //crossover cable conditions was 0.05
			debug_printf(GEORGE_PTP_PRT, "time_check crossover:%le \n",ltgain);
         if(rms.time_source[0] < RM_GPS_1)
            time_bias += 44.3; //GPZ crossover bias calibration Dec 2011

		}
		if(ptpTransport == e_PTP_MODE_SLOW_ETHERNET) //DEC 2010 extend settling window in slow settling conditions
		{
			time_slew_limit=10000.0; //was 100000.0
		}	
		else if(Is_PTP_FREQ_ACTIVE() == FALSE)
		{
//			time_slew_limit=100.0; 
         if((FLL_Elapse_Sec_Count>7200))
         {
		      time_slew_limit=1000.0;//modify by jiwang from 100 to 1000
         }
         else
         {         
		      time_slew_limit=1000.0;
         }


		}
		else
		{
         if((FLL_Elapse_Sec_Count>7200))
         {
		      time_slew_limit=100.0;
         }
         else
         {         
		      time_slew_limit=2000.0;
         }
		}	
//#if (HYDRO_QUEBEC_ENABLE==1)		
//		time_slew_limit=10000.0; //was 100000.0
//#else
//		time_slew_limit=1000.0;
//#endif
#if(ASYMM_CORRECT_ENABLE==1)		
		time_slew_limit=1000.0;
#endif
#if ((ASYMM_CORRECT_ENABLE==0) && (HYDRO_QUEBEC_ENABLE==0))
      if(1)
//		if(!(ptpRate > -4))
		{
//		 	if(OCXO_Type()==e_RB)
//		 	{
//	 			ltgain=ltgain/2.0; //slow down phase loop  Packet PRC was 8.0	
//		 	}
//		 	else if((OCXO_Type()==e_OCXO)&& (SC_FASTER_SERVO_MODE==0))
//	 		{
//		 		ltgain=ltgain/1.0; //slow down phase loop  Packet PRC	
//		 	}
		}
		else
		{
			if(ltgain < 0.003) ltgain=0.003; //was 0.008	 	
//			debug_printf(GEORGE_PTP_PRT, "time_check slow update rate:%le \n",ltgain);

		}
#else //GPZ make time control more agressive 
			if(ltgain < 0.003) ltgain=0.003; //was 0.008	 	
#endif	 
		if(ptpTransport==e_PTP_MODE_SLOW_ETHERNET) ////GPZ make time control more agressive 
		{
			ltgain=ltgain/2.0;	
//			debug_printf(GEORGE_PTP_PRT, "time_check slow ethernet:%le \n",ltgain);
			
		
		}
		if(rfe_turbo_flag && (mode_jitter==0)) //GPZ DEC 2010 make more agressive during turbo mode
		{
//			if(ltgain < 0.0005) ltgain=0.0005; //was 0.003	 
//			debug_printf(GEORGE_PTP_PRT, "time_check turbo mode:%le \n",ltgain);
		   if( ((pf>33.0) || (pr>33.0)) && (pf>10)&& (pr>10) ) //April 25 2011 new test for excess bias 65 when CTP=256
		   {
            ltgain= 0.000001;	
				debug_printf(GEORGE_PTP_PRT, "time_check extreme excess asymm:%le \n",ltgain);

         }
		   else if( ((pf>30.0) || (pr>30.0)) && (pf>10)&& (pr>10) ) //April 25 2011 new test for excess bias 65 when CTP=256
		   {
            ltgain= 0.00001;	
				debug_printf(GEORGE_PTP_PRT, "time_check excess asymm:%le \n",ltgain);
         }
			if(FLL_Elapse_Sec_Count_Use<7200)
         {
   			if(ltgain < 0.005) ltgain=0.005; 
   			if(time_gain < PER_05) time_gain=PER_05;
//				debug_printf(GEORGE_PTP_PRT, "time_check early turbo holdoff:%le \n",ltgain);
         }
		}
//#if (GPS_ENABLE==1)
		if(Is_PTP_TIME_ACTIVE() == FALSE)	
		{
			if(ltgain < 0.21){
				ltgain=0.21; 
				if(abs(time_slew_err) > 100 && abs(time_slew_err) < 1000)
					ltgain = 0.1;
				else if(abs(time_slew_err) <= 100)
				 ltgain=0.01; //GPZ JAN 2011 make more agressive during GPS mode	 was 0.005
//			debug_printf(GEORGE_PTP_PRT, "time_check PHY Time Mode:%le \n",ltgain);
			}
		}
//#endif
//		if(ltgain < 0.005) ltgain=0.005; //TEST TEST TEST FORCE HIGH GAIN ALL THE TIME	
#if 1          // time bias rate calculation  
/* if frequency and time references are different kkk*/
#if !NO_PHY
         if((rms.freq_source[0] != RM_NONE)&&(rms.time_source[0] != RM_NONE))
         {
            if(rms.time_source[0] != rms.freq_source[0])     
	   		{
               time_rate_hold_off = 0;
	   			if(time_transient_flag==0)
	   			{
                  if(rms.time_source[0] == RM_GPS_1)
                  {
      				   time_bias_est_rate  =  RFE_PHY.fest - RFE_PHY.fest_chan[GPS_A]; //GPZ DEC 2011 RFE estimators for optimal tilt estimate
                     debug_printf(GEORGE_PTP_PRT, "update_time all phy: rate_calc: %ld %le %le %le\n",(long int) GPS_A, time_bias_est_rate, RFE_PHY.fest, RFE_PHY.fest_chan[GPS_A]);

                  }
//jiwang
				 else if(rms.time_source[0] == RM_GPS_2)
                  {
      				   time_bias_est_rate  =  RFE_PHY.fest - RFE_PHY.fest_chan[GPS_B]; //GPZ DEC 2011 RFE estimators for optimal tilt estimate
                     debug_printf(GEORGE_PTP_PRT, "update_time all phy: rate_calc: %ld %le %le %le\n",(long int) GPS_B, time_bias_est_rate, RFE_PHY.fest, RFE_PHY.fest_chan[GPS_B]);

                  }
				 else if(rms.time_source[0] == RM_GPS_3)
                  {
      				   time_bias_est_rate  =  RFE_PHY.fest - RFE_PHY.fest_chan[GPS_C]; //GPZ DEC 2011 RFE estimators for optimal tilt estimate
                     debug_printf(GEORGE_PTP_PRT, "update_time all phy: rate_calc: %ld %le %le %le\n",(long int) GPS_C, time_bias_est_rate, RFE_PHY.fest, RFE_PHY.fest_chan[GPS_C]);

                  }
                  else
                  {
      				   time_bias_est_rate  =  RFE_PHY.fest - RFE.fest; //GPZ DEC 2011 RFE estimators for optimal tilt estimate
               debug_printf(GEORGE_PTP_PRT, "update_time: rate calc est diff: %le %le\n",RFE_PHY.fest,RFE.fest);
                  }
//             time_bias_est_rate= 30.0;  //TEST TEST TEST
//             time_bias_est_rate= 0.0;  //TEST TEST TEST
                  debug_printf(GEORGE_PTP_PRT, "update_time: rate calc:%le %le %le %le \n",time_slew_err, time_bias_est_rate, RFE_PHY.fest, RFE.fest);
	   			}
	   			time_bias_est  -=  time_bias_est_rate;
               debug_printf(GEORGE_PTP_PRT, "update_time: rate_calc: %le %le\n",time_bias_est, time_bias_est_rate);
            }
  	   		else 
      		{
                  if(time_rate_hold_off)
                  {
                     time_rate_hold_off--;  
                  }
                  else
                  {
                     if(time_bias_est_rate !=0.0)
                     {
                       #if 0 
                       if(rms.time_source[0] == RM_GPS_1)
                       {
      	      	   	   time_bias_est = time_bias_est - (double)(90.0*time_bias_est_rate);  
                       }
                       else
                       {   
	   	      	        time_bias_est = time_bias_est - (double)(28.0*time_bias_est_rate);  
                       }
                       #endif
                     }
      	   			time_bias_est_rate  = 0.0;
//                     debug_printf(GEORGE_PTP_PRT, "update_time: why am I here?: \n");
   
                  }
      		}
         }
         else
         {
//      	   	time_bias_est_rate  = 0.0;
//               time_bias= 0.0;
               time_transient_flag = 1.5 * time_hold_off;
               debug_printf(GEORGE_PTP_PRT, "update_time: No Time or Freq Ref: \n");
         }
#endif 
#endif	
#if 1
		// update tilt compensation TODO add reset with transient indication	
		if( ((rms.freq_source[0])== RM_GM_1)||((rms.freq_source[0])== RM_GM_2) ) //active freq source case
		{
			detilt_correction_rev = -detilt_correction_for;
		}
		else
		{
				detilt_correction_for += time_bias_est_rate;
				detilt_correction_rev = -detilt_correction_for;
//				detilt_correction_for = 0.0;
//				detilt_correction_rev = 0.0;

		   	hold_res_acc=0.0;
      }
#endif
		
//      if(Is_PTP_Mode_Standard())
      if(1) //Do for both Standard and Coupled Mode FEB 2012
		{
//         time_slew_limit= 100000.0;
//         ltgain = 0.003;  //TEST TEST TEST 
//         if(1)  //TEST TEST TEST
		#if 1
			printf("FLL_Elapse_Sec_Count_Use = %d, time_transient_flag = %d\n",FLL_Elapse_Sec_Count_Use,time_transient_flag);
		#endif
         #if (LOW_PACKET_RATE_ENABLE == 1)
			if((tstate!=FLL_WARMUP)&&(FLL_Elapse_Sec_Count_Use<1200) && (time_transient_flag < 2))  //was 5200
         #else
			if((tstate!=FLL_WARMUP)&&(FLL_Elapse_Sec_Count_Use<7200) && (time_transient_flag < 2))  //was 5200
         #endif
			{
				time_bias_est=time_bias;
				debug_printf(GEORGE_PTP_PRT, "update_time: early loop: %ld, est %ld \n",(long int)time_bias,  (long int)time_bias_est);
				psteer=1;
            jam_needed = FALSE;
			}
			else if( (fll_settle==0) && (time_transient_flag==0) )
			{
				time_slew_err=(time_bias-time_bias_est);
			#if 1
				printf("time_slew_err=  %d\n",time_slew_err);
			#endif
	
//            time_slew_err=0.0; //TEST TEST TEST DEC 2011 disable all slew control
            if(time_slew_err > 1000.0)
            {
   				debug_printf(GEORGE_PTP_PRT, "slew limit jam needed: error %d threshold %d slew_limit: %ld\n",(int)time_slew_err,  (int)jam_threshold, (long int)time_slew_limit);
			      jam_needed = TRUE;
            }
            else if(-time_slew_err > 1000.0)
            {
   				debug_printf(GEORGE_PTP_PRT, "slew limit jam needed: error %d threshold %d slew_limit: %ld\n",(int)time_slew_err,  (int)jam_threshold, (long int)time_slew_limit);
			      jam_needed = TRUE;

            }
            else
            {
			      jam_needed = FALSE;
            }
            if(never_jam_flag==FALSE)
            {
               if((LoadState == LOAD_GENERIC_HIGH) && (jam_threshold < 10000000))
               {
                  temp_jam_threshold = 10000000;
               }
               else
               {
                  temp_jam_threshold = jam_threshold;
               }
               if((time_slew_err > (double)temp_jam_threshold) || (-time_slew_err > (double)temp_jam_threshold))
               {
				      #if (N0_1SEC_PRINT==0)		
      				debug_printf(GEORGE_PTP_PRT, "real slew limit jam: error %d threshold %d cnt %d\n",
                     (int)time_slew_err,  
                     (int)jam_threshold, 
                     (int)jam_threshold_counter);
                  #endif
                  if(jam_threshold_counter++ > 180)  //wa 60
                  {
         				debug_printf(GEORGE_PTP_PRT, "!!!!!!JAMMED\n");
   				      time_bias_est=time_bias;
                     event_printf(e_PHASE_ALIGNMENT, e_EVENT_NOTC, e_EVENT_TRANSIENT,
   		             sc_alarm_lst[e_PHASE_ALIGNMENT].dsc_str, (INT32)(time_slew_err));
                  }
               }
               else
               {
                  jam_threshold_counter = 0;
               }
            }
			 	residual_time_error= time_slew_err;
				if(time_slew_err>time_slew_limit)
				{
					time_slew_err=time_slew_limit; // was 1000ns increase to 20000ns for RC3 revert back to 1000 
               time_slew_limit_cnt++;
				}
				else if(time_slew_err< (-time_slew_limit))
				{
					time_slew_err= (-time_slew_limit);	
               time_slew_limit_cnt++;
				}
            else
            {
               time_slew_limit_cnt=0;
            }
//			 	residual_time_error= time_slew_err;
				if(xds.xfer_first&&(FLL_Elapse_Sec_Count_Use<1320)) //modify from 4400 to 1320
				{
					time_bias_est=time_bias;
 				}
            else if(jam_now_flag==TRUE)
				{
					time_bias_est=time_bias;
               debug_printf(GEORGE_PTP_PRT, "update_time: Jam Clear:\n");
               Jam_Clear();
               event_printf(e_PHASE_ALIGNMENT, e_EVENT_NOTC, e_EVENT_TRANSIENT,
		             sc_alarm_lst[e_PHASE_ALIGNMENT].dsc_str, (INT32)(residual_time_error));
 				}
				else
				{
   				time_bias_est += ltgain*time_slew_err ;  // TEST TEST TEST DEC 2011 try smoothing filter 
//					time_bias_est =  0.9999*time_bias_est + 0.0001*time_bias;
               debug_printf(GEORGE_PTP_PRT, "update_time: new loop:%ld %ld slew %ld, est %ld, rate %le ltgain %le trans:%d\n",FLL_Elapse_Sec_Count_Use, (long int)time_bias, (long int)time_slew_err, (long int)time_bias_est, time_bias_est_rate , ltgain, time_transient_flag);
					
				}
				time_bias_base=time_bias_est; //establish time baseline
				psteer=1;
			}//end of else if in line 6758
			else
			{
            jam_needed = FALSE;
				psteer=2;
				residual_time_error=0;
            debug_printf(GEORGE_PTP_PRT, "update_time: transient loop:\n");
			}	
			tshape=ZEROF;
			time_bias_offset=ZEROF;
		}//end of if in line 6741
      if(!Is_PTP_Mode_Standard())
		{
			debug_printf(GEORGE_PTP_PRT, "Time Coupled Mode Case\n");
		
			if(fll_settle==0)
			{
//			time_bias_offset=time_bias - time_bias_base;
            if((rms.time_source[0] == RM_GM_1)||(rms.time_source[0] == RM_GM_2))
            {
  			      ll_phase_offset = time_bias_est - (double)(28.0*time_bias_est_rate);  //was 28.0
            }
            else
            {   
  			      ll_phase_offset = time_bias_est - (double)(4.0*time_bias_est_rate);  //was 110
             }
 
   			time_bias_offset=ll_phase_offset - time_bias_base;
			}
			if(ltgain>0.01)
			{
				 ltgain=0.01; //was 0.006
		 	}
			if(((time_bias_offset< 3000.0) && (time_bias_offset > -3000.0)))
			{
				tshape= time_bias_offset*ltgain; 
				if(tshape>2.0)tshape=2.0;
				else if(tshape<-2.0)tshape=-2.0;
				psteer=0;	
			}
			else if(mode_jitter==1)
			{
				tshape=ZEROF;
				psteer=0;	
			}
			else // Nov 16 2009 make default case without exception
			{
            //Feb 2012 New Simplified Approach just use time_bias_est
			   time_bias_base = time_bias_est;
            psteer = 1;
//				time_slew_err=(time_bias-time_bias_est);
//				if(time_slew_err>time_slew_limit) time_slew_err=time_slew_limit;
//				else if(time_slew_err<-time_slew_limit) time_slew_err=-time_slew_limit;
//				time_bias_est = time_bias_est + ltgain*time_slew_err;
//				time_bias_base=time_bias_est; //establish time baseline
//				tshape=ZEROF;
//				psteer=1;
			}
		}
	}
	else
	{
   	debug_printf(GEORGE_PTP_PRT, "exception: special time skip steering\n");

      jam_needed = FALSE;
		tshape=ZEROF; //no shaping when not tracking
		time_bias_offset=ZEROF;
		residual_time_error=0;
		psteer=2; //special state skip steering
	}
	if((start_in_progress && start_in_progress_PTP)||var_cal_flag==1)
	{
		time_bias_est = time_bias;
		time_bias_base=time_bias;
		time_bias_est_rate=0.0;	
		time_bias_offset=ZEROF;
		time_bias_mean=ZEROF;
		time_bias_var=ZEROF;
		tshape=ZEROF;
		residual_time_error=0;
		psteer=2; //NOV 22 just don't time steer while de-tilting
	}
	else
	{
		time_bias_mean=PER_99*time_bias_mean+PER_01*inter1;
		dtime=inter1-time_bias_mean;
		time_bias_var=PER_99*time_bias_var+PER_01*(dtime*dtime);
	}	
#if (ASYMM_CORRECT_ENABLE==1)
	if(ACL.astate== ASYMM_CALIBRATE)
	{
		psteer=2;
		residual_time_error=0;
	}
#endif	
#if 0
if(Print_Phase%64==0)
{
	debug_printf(TIME_PRT, "time for: %ld  min_f:%le cal_f:%le iof:%le \n",
        FLL_Elapse_Sec_Count,
        (float)sdrift_min_forward,
        (float)sdrift_cal_forward,
        (float)Initial_Offset
        );

	debug_printf(TIME_PRT, "time rev: %ld  min_r:%l
66666..Transfer_Phasor_data_now: los_cntr 255..rate 1.e cal_r:%le ior:%le \n",
        FLL_Elapse_Sec_Count,
        (float)sdrift_min_reverse,
        (float)sdrift_cal_reverse,
        (float)Initial_Offset_Reverse
        );
	debug_printf(TIME_PRT, "time com: %ld rtd:%le tshape:%le tbo:%le tbase:%le gain:%le psteer:%ld tbias_est:%le tbias_est_rate:%le \n",
        FLL_Elapse_Sec_Count,
        (float) round_trip_delay_alt,
        (float) tshape,
        (float)time_bias_offset,
        (float)time_bias_base,
        (float)ltgain,
        psteer,
        (float)time_bias_est,
        (float)time_bias_est_rate
        );
	debug_printf(TIME_PRT, "time comp: %ld slew_err:%le comm:%le diff:%le tcomp:%le \n",
        FLL_Elapse_Sec_Count,
        (float)time_slew_err,
        (float) tgain_comm,
        (float) tgain_diff,
        (float) tgain_comp
         );
        
}        
#endif     
/* Set_PPS_Offset() will offset the 1PPS phase by (1/60e+6) seconds per bit.  
*  valid values are 0 to 59999999 
*/        

/* convert to bits (signed) */
#if 0
		time_bias_count = time_bias_est * (2.0 * PPS_CLOCKRATE / 1e9);

		if(time_bias_est < 0)
		{
			time_bias_count += (PPS_CLOCKRATE * 2);
		}
		offset = time_bias_count;
#endif

/* convert to bits (unsigned) */
//		if((psteer==1)|| ((tstate!=FLL_WARMUP)&&(FLL_Elapse_Sec_Count_Use<4800) && (time_transient_flag < 2)))
		if(1)
		{
//			if(timestamp_align_flag==0)
/* Out band phase steering (perhaps FPGA) */
			if(!Is_PTP_Mode_Inband())
			{
				#if (N0_1SEC_PRINT==0)		
				debug_printf(PLL_PRT,"++ll_phase_offset1: %e\n", time_bias_est);
				#endif
            if(time_bias_est_rate < 0.0)
            {
               if((rms.time_source[0] == RM_GM_1)||(rms.time_source[0] == RM_GM_2))
               {
    			      ll_phase_offset = time_bias_est - (double)(28.0*time_bias_est_rate);  //was 28.0
               }
               else
               {   
    			      ll_phase_offset = time_bias_est - (double)(4.0*time_bias_est_rate);  //was 110
               }
            }
            else
            {
               if((rms.time_source[0] == RM_GM_1)||(rms.time_source[0] == RM_GM_2))
               {
    			      ll_phase_offset = time_bias_est - (double)(28.0*time_bias_est_rate);  //was 28.0
               }
               else
               {   
    			      ll_phase_offset = time_bias_est - (double)(4.0*time_bias_est_rate);  //was 75 
               }
            }
            if((Is_PTP_Mode_Standard() == 1) || (psteer != 0))
            {
//            ll_phase_offset = time_bias_est; //TEST TEST TEST 
//               debug_printf(GEORGE_PTP_PRT, "update_time: apply correction:%ld %ld bias %ld, est %ld, rate %le  trans:%d\n",
//FLL_Elapse_Sec_Count_Use, (long int)time_bias, (long int)time_bias, (long int)time_bias_est, time_bias_est_rate , time_transient_flag);

  	   	        SC_PhaseOffset_Local(&ll_phase_offset, e_PARAM_SET);
   				  timestamp_align_offset_servo=0;
            }
	    	}
/* In band phase steering */
	    	else
	    	{
//	    		Set_PPS_Offset(0, TRUE);
	    		if(timestamp_align_skip_servo>0) timestamp_align_skip_servo--;
	    		delta_offset= (INT32)(time_bias_est+0.5)-timestamp_align_offset_servo;
	    		if(((!start_in_progress)||(!start_in_progress_PTP))||(FLL_Elapse_Sec_Count_Use<240))
	    		{
   	    		if((timestamp_align_skip_servo==0)&&((delta_offset>100) || (delta_offset<-100)))
   	    		{
   	    			timestamp_align_offset_prev_servo = timestamp_align_offset_servo;
   		    		timestamp_align_offset_servo=(INT32)(time_bias_est+0.5);
   					timestamp_align_skip_servo=360;
                  timestamp_head_f_servo=PTP_Fifo_Local.fifo_in_index;
   				   timestamp_head_r_servo=PTP_Delay_Fifo_Local.fifo_in_index;

   					timestamp_transition_flag_servo=1;					
   					
#if 0					
   					delta_offset=-delta_offset; //use opposite sign		    		
   					if(delta_offset>0)
   					{
   						PTPClockStepAdjustment(0,0,(UINT32)(delta_offset),0);
						#if (N0_1SEC_PRINT==0)		
   						debug_printf(PLL_PRT,"Positive TimeStamp Adjust: %ld\n",delta_offset);
   						#endif
   					}
   					else
   					{
   						PTPClockStepAdjustment(0,0,(UINT32)(-(delta_offset)),1);
						#if (N0_1SEC_PRINT==0)		
   						debug_printf(PLL_PRT,"Negative TimeStamp Adjust: %ld\n",-(delta_offset));
   						#endif
   					}
#endif
 //              	ll_phase_offset = delta_offset;
					ll_phase_offset = time_bias_est; 

   //               Set_Timestamp_Offset(ll_phase_offset);
   					#if (N0_1SEC_PRINT==0)		
                  	debug_printf(PLL_PRT,"++ll_phase_offset2: %lld\n", ll_phase_offset);
                  	#endif

                  //Set_PPS_Offset(ll_phase_offset, TRUE);
					SC_PhaseOffset_Local(&ll_phase_offset, e_PARAM_SET);

   				 }
                else
                {
 //       	    		Set_PPS_Offset(0, FALSE);
                }
				}
			}
	    }
	    else if (psteer==0)
	    {
//	    	timestamp_align_offset=0;
//	    	delta_offset=0;
	    }
#if 1
		#if (N0_1SEC_PRINT==0)		
		debug_printf(PLL_PRT,"time_a:%ld %ld time_bias %.8e time_bias_est %.8e time_gain %e\n", 
			(UINT32)FLL_Elapse_Sec_Count_Use, 
			(UINT32)start_in_progress, 
			time_bias, 
			time_bias_est, 
			time_gain
		);
		debug_printf(PLL_PRT, "time_b:%ld  tflag: %d, toff: %ld,scnt: %4d,ttflag: %d,headf: %d,headr: %d,doff: %ld\n", 
			(UINT32)FLL_Elapse_Sec_Count_Use, 
			(int)Is_PTP_Mode_Inband(),
			timestamp_align_offset_servo,
			timestamp_skip_cnt_servo,
			timestamp_transition_flag_servo,
			timestamp_head_f_servo,					
			timestamp_head_r_servo,					
			delta_offset
		);
		#endif		
		

#else
		#if (N0_1SEC_PRINT==0)		
		debug_printf(PLL_PRT, "%ld %ld time_bias_est %e  offset %d off %ld roff %ld\n", 
			(UINT32)FLL_Elapse_Sec_Count_Use, (UINT32)start_in_progress, time_bias_est, offset,
			PTP_Fifo_Local.fifo_element[PTP_Fifo_Local.fifo_in_index].offset,
			PTP_Delay_Fifo_Local.fifo_element[PTP_Delay_Fifo_Local.fifo_in_index].offset);
		#endif	
#endif
//		debug_printf(PLL_PRT, "inter %e inter1 %e\n", inter, inter1);
	    
}
#endif
double Get_time_correct(void)
{
	return time_bias_est;
}

static  UINT8 s_prevTimeWeight[NUM_OF_SERVO_CHAN];
static  UINT8 s_prevFreqWeight[NUM_OF_SERVO_CHAN];
static BOOLEAN select_channel_initialized = FALSE;

#if 0
static BOOLEAN In_Major_Time_Ajust(void)
{
   if((state == PTP_MAJOR_TS_EVENT_WAIT) ||
   (state == PTP_MAJOR_TS_EVENT) ||
   (state == PTP_MAJOR_TS_EVENT_END))
   {
      return TRUE;
   }
   return FALSE;
}
#endif
static int selected_freq_queue[3] = {RM_NONE, RM_NONE, RM_NONE};

static void select_channel(void)
{
#if(NO_PHY==0)
   double local_time_bias_est_rate;
   INT8 input_chan = 0;
   INT8 chan_change = 0;
   int i;
 
//   int_16 prev_freq = rms.prev_freq_source[0];
   if(!select_channel_initialized)
   {
      for(i=0;i<NUM_OF_SERVO_CHAN;i++)
      {
         s_prevTimeWeight[i] = 0;
         s_prevFreqWeight[i] = 0;
      }	
      select_channel_initialized = TRUE;
   }

/* determine time source */
	if(Was_Time_Adjusted() && ((FLL_Elapse_Sec_PTP_Count > 300) || (FLL_Elapse_Sec_Count > 300)))
   {
//      go_to_bridge = TRUE;
      rms.time_source[0] = RM_NONE;
   }
	else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_PTP1].b_selectedTime)
	{
/* if transitioning from to reference, then force into bridging */
      if(s_prevTimeWeight[SERVO_CHAN_PTP1] == 0)
      {
//         go_to_bridge = TRUE;
         rms.time_source[0] = RM_NONE;         
      }
      else
      {
         rms.time_source[0] = RM_GM_1;
      }
   }
   else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_GPS1].b_selectedTime)
	{
/* if transitioning from to reference, then force into bridging */
      if(s_prevTimeWeight[SERVO_CHAN_GPS1] == 0)
      {
//         go_to_bridge = TRUE;
         rms.time_source[0] = RM_NONE;         
      }
      else
      {
         rms.time_source[0] = RM_GPS_1;
      }
   }
//jiwang
   else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_GPS2].b_selectedTime)
	{
/* if transitioning from to reference, then force into bridging */
      if(s_prevTimeWeight[SERVO_CHAN_GPS2] == 0)
      {
//         go_to_bridge = TRUE;
         rms.time_source[0] = RM_NONE;         
      }
      else
      {
         rms.time_source[0] = RM_GPS_2;
      }
   }
   else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_GPS3].b_selectedTime)
	{
/* if transitioning from to reference, then force into bridging */
      if(s_prevTimeWeight[SERVO_CHAN_GPS3] == 0)
      {
//         go_to_bridge = TRUE;
         rms.time_source[0] = RM_NONE;         
      }
      else
      {
         rms.time_source[0] = RM_GPS_3;
      }
   }
   else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_RED].b_selectedTime)
	{
/* if transitioning from to reference, then force into bridging */
      if(s_prevTimeWeight[SERVO_CHAN_RED] == 0)
      {
//         go_to_bridge = TRUE;
         rms.time_source[0] = RM_NONE;         
      }
      else
      {
         rms.time_source[0] = RM_RED;
      }
   }
   else
   {
         rms.time_source[0] = RM_NONE;
   }

/* determine frequency source */
	if(Was_Time_Adjusted() && ((FLL_Elapse_Sec_PTP_Count > 300) || (FLL_Elapse_Sec_Count > 300)))
   {
      Clear_Time_Adjusted_Flag();
		input_chan=0;
      #if(NO_PHY==0)
		skip_rfe_phy=PHY_SKIP;  //TEST TEST TEST was 32
      #endif
		rms.freq_source[0]= RM_NONE;
		debug_printf(GEORGE_PTP_PRT,"xfer select NONE as freq chan time adjusted\n");
   }
   else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_PTP1].b_selectedFreq)
	{
      if(s_prevFreqWeight[SERVO_CHAN_PTP1] == 0)
      {
//         go_to_bridge = TRUE;
         rms.freq_source[0] = RM_NONE;    
   		debug_printf(GEORGE_PTP_PRT,"change to GM1, temp NONE\n");
      }
      else
      {
   		if(rms.freq_source[0] != RM_GM_1)
   		{
				if(rms.prev_freq_source[0] != RM_GM_1)
  					chan_change=1;  // new chan_change
				
//				if((rms.freq_source[0] != RM_NONE) || first_selected_freq)
//      		{
//               rms.prev_freq_source[0] = rms.freq_source[0];
//               first_selected_freq = FALSE;
//            }

            rms.freq_source[0]= RM_GM_1;

   			debug_printf(GEORGE_PTP_PRT,"xfer select GM1 as freq chan\n");
            if(rms.prev_freq_source[0] != RM_GM_1)
            {
       		   init_pshape();	//KEN This is why TC15 blows up we keep switch from none to GM and back
            }
   		}
 //  		xds.xfer_turbo_phase[0]=rfe_turbo_phase[0];
 //  		xds.xfer_turbo_phase[1]=rfe_turbo_phase[1];
      }
	}
	else
	{
		if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_GPS1].b_selectedFreq)
		{
  			input_chan=0;

         if(s_prevFreqWeight[SERVO_CHAN_GPS1] == 0)
         {
 //           go_to_bridge = TRUE;
            rms.freq_source[0] = RM_NONE;         
//  				chan_change=1;
   		   debug_printf(GEORGE_PTP_PRT,"change to GPS1, temp NONE\n");
         }
         else
         {
   			if(rms.freq_source[0] != RM_GPS_1)
   			{
					if(rms.prev_freq_source[0] != RM_GPS_1)
  						chan_change=1;  // new chan_change
				   
//               if((rms.freq_source[0] != RM_NONE) || first_selected_freq)
//      		   {
//                  rms.prev_freq_source[0] = rms.freq_source[0];
//                  first_selected_freq = FALSE;
//               }

   				rms.freq_source[0]= RM_GPS_1;

   				debug_printf(GEORGE_PTP_PRT,"xfer select GPS1 as freq chan\n");;
   			}
         }
		}
//jiwang
		else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_GPS2].b_selectedFreq)
		{
  			input_chan=0;

         if(s_prevFreqWeight[SERVO_CHAN_GPS2] == 0)
         {
 //           go_to_bridge = TRUE;
            rms.freq_source[0] = RM_NONE;         
//  				chan_change=1;
   		   debug_printf(GEORGE_PTP_PRT,"change to GPS2, temp NONE\n");
         }
         else
         {
   			if(rms.freq_source[0] != RM_GPS_2)
   			{
					if(rms.prev_freq_source[0] != RM_GPS_2)
  						chan_change=1;  // new chan_change
				   
//               if((rms.freq_source[0] != RM_NONE) || first_selected_freq)
//      		   {
//                  rms.prev_freq_source[0] = rms.freq_source[0];
//                  first_selected_freq = FALSE;
//               }

   				rms.freq_source[0]= RM_GPS_2;

   				debug_printf(GEORGE_PTP_PRT,"xfer select GPS2 as freq chan\n");;
   			}
         }
		}
		else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_GPS3].b_selectedFreq)
		{
  			input_chan=0;

         if(s_prevFreqWeight[SERVO_CHAN_GPS3] == 0)
         {
 //           go_to_bridge = TRUE;
            rms.freq_source[0] = RM_NONE;         
//  				chan_change=1;
   		   debug_printf(GEORGE_PTP_PRT,"change to GPS3, temp NONE\n");
         }
         else
         {
   			if(rms.freq_source[0] != RM_GPS_3)
   			{
					if(rms.prev_freq_source[0] != RM_GPS_3)
  						chan_change=1;  // new chan_change
				   
//               if((rms.freq_source[0] != RM_NONE) || first_selected_freq)
//      		   {
//                  rms.prev_freq_source[0] = rms.freq_source[0];
//                  first_selected_freq = FALSE;
//               }

   				rms.freq_source[0]= RM_GPS_3;

   				debug_printf(GEORGE_PTP_PRT,"xfer select GPS3 as freq chan\n");;
   			}
         }
		}
      else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_RED].b_selectedFreq)
		{
  			input_chan=4;
 
         if(s_prevFreqWeight[SERVO_CHAN_RED] == 0)
         {
 //           go_to_bridge = TRUE;
            rms.freq_source[0] = RM_NONE;         
//  				chan_change=1;
   		   debug_printf(GEORGE_PTP_PRT,"change to RED, temp NONE\n");
         }
         else
         {
   			if(rms.freq_source[0] != RM_RED)
   			{
					if(rms.prev_freq_source[0] != RM_RED)
  						chan_change=1;  // new chan_change

//				   if((rms.freq_source[0] != RM_NONE) || first_selected_freq)
//      		   {
//                  rms.prev_freq_source[0] = rms.freq_source[0];
//                  first_selected_freq = FALSE;
//               }

   				rms.freq_source[0]= RM_RED;
 
   				debug_printf(GEORGE_PTP_PRT,"xfer select RED as freq chan\n");;
   			}
         }
		}
		else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_SYNCE1].b_selectedFreq)
		{
			input_chan=2;

         if(s_prevFreqWeight[SERVO_CHAN_SYNCE1] == 0)
         {
 //           go_to_bridge = TRUE;
            rms.freq_source[0] = RM_NONE;         
//  				chan_change=1;
   			debug_printf(GEORGE_PTP_PRT,"xfer select SE_A as freq chan\n");;
         }
         else
         {
   			if(rms.freq_source[0] != RM_SE_1)
   			{
					if(rms.prev_freq_source[0] != RM_SE_1)
  						chan_change=1;  // new chan_change

//				   if((rms.freq_source[0] != RM_NONE) || first_selected_freq)
//      		   {
//                  rms.prev_freq_source[0] = rms.freq_source[0];
//                  first_selected_freq = FALSE;
//               }

   				rms.freq_source[0]= RM_SE_1;

   				debug_printf(GEORGE_PTP_PRT,"xfer select SE_A as freq chan\n");
   			}
         }
		}
		else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_SYNCE2].b_selectedFreq)
		{
			input_chan=3;

         if(s_prevFreqWeight[SERVO_CHAN_SYNCE2] == 0)
         {
//            go_to_bridge = TRUE;
            rms.freq_source[0] = RM_NONE;         
//  				chan_change=1;
  				debug_printf(GEORGE_PTP_PRT,"xfer select SE_B as freq chan\n");;            
         }
         else
         {
   			if(rms.freq_source[0] != RM_SE_2)
   			{
					if(rms.freq_source[0] != RM_SE_2)
  						chan_change=1;  // new chan_change

//				   if((rms.freq_source[0] != RM_NONE) || first_selected_freq)
//      		   {
//                  rms.prev_freq_source[0] = rms.freq_source[0];
//                  first_selected_freq = FALSE;
//               }

   				rms.freq_source[0]= RM_SE_2;

   				debug_printf(GEORGE_PTP_PRT,"xfer select SE_B as freq chan\n");
   			}
         }
		}
		else
		{
			input_chan=0;
         #if(NO_PHY==0)
//			skip_rfe_phy=PHY_SKIP; //TEST TEST TEST was 32
         #endif
//			if(rms.freq_source[0] != RM_NONE)
//      		   rms.prev_freq_source[0] = rms.freq_source[0];
			rms.freq_source[0]= RM_NONE;
			debug_printf(GEORGE_PTP_PRT,"xfer select NONE as freq chan\n");		
		}
   }

/* if there is a change in frequency source, then see if you need to change the prev */
   if(rms.freq_source[0] != selected_freq_queue[0])
   {
      selected_freq_queue[2] = selected_freq_queue[1];
      selected_freq_queue[1] = selected_freq_queue[0];
      selected_freq_queue[0] = rms.freq_source[0];

/* determine prev */
      if(selected_freq_queue[1] == RM_NONE)
      {
         rms.prev_freq_source[0] = selected_freq_queue[2];
      }
      else
      {
         rms.prev_freq_source[0] = selected_freq_queue[1];
      }
   }
//   printf("rms.prev_freq_source[0] = %d rms.freq_source[0] = %d q0= %d q1= %d q2= %d\n",
//         (int)rms.prev_freq_source[0], (int)rms.freq_source[0],selected_freq_queue[0],selected_freq_queue[1],selected_freq_queue[2]);



		if(chan_change==1) //for now alway restart all channels to be safe
		{
#if(NO_PHY==0)
/* if previous frequency channel was PTP then use holdover_f to seed */
//      	       debug_printf(GEORGE_PTP_PRT," Channel_Change: old: %ld new : %ld \n", (long int)rms.prev_freq_source[0],(long int)rms.freq_source[0]); 
               if(rms.prev_freq_source[0] >1)
               {
                  xds.xfer_active_chan_phy_prev = rms.prev_freq_source[0] -2;
               }
               if(rms.freq_source[0] >1)
               {
                  xds.xfer_active_chan_phy = rms.freq_source[0] -2;
               }

            if( ((RFE_PHY.Nmax < 15) || (start_in_progress)) && (rms.freq_source[0] > 1) )
            {
            	debug_printf(GEORGE_PTP_PRT,"Using start_rfe_phy() using holdover_f: %le\n", holdover_f); 
               init_rfe_phy();
				   start_rfe_phy(holdover_f); //renormalize RFE data to 16x speed
//            	debug_printf(GEORGE_PTP_PRT,"Using start_rfe_phy() early  holdover_f: %le\n", holdover_f); 
				   fdrift_f=fdrift_r=fdrift=fdrift_warped=fdrift_smooth=pll_int_r=pll_int_f= fdrift_raw=holdover_r=holdover_f;
            }
            else
            {
               debug_printf(GEORGE_PTP_PRT,"Using start_rfe_phy() using xds.xfer_active_chan_phy: %ld, %le\n",  xds.xfer_active_chan_phy,RFE_PHY.fest_chan[xds.xfer_active_chan_phy]); 
               if(rms.freq_source[0] > 1)        
               {
                  holdover_f = RFE_PHY.fest_chan[xds.xfer_active_chan_phy];
               }
               if(rms.time_source[0] == rms.freq_source[0]) //new rate is zero     
               {
               	   debug_printf(GEORGE_PTP_PRT,"Jam Time_Bias_Est_Rate New Rate Zero: %ld \n", (long int) time_bias_est_rate); 
                     local_time_bias_est_rate=0.0;
               }
               else
               {
/* if PHY to PHY switch */
                  if((rms.prev_freq_source[0] >= RM_GPS_1)&&(rms.freq_source[0] >= RM_GPS_1))
                  {
             	       debug_printf(GEORGE_PTP_PRT," PHY to PHY cur: %ld prev:%ld \n",(long int)xds.xfer_active_chan_phy,(long int)xds.xfer_active_chan_phy_prev); 

                     local_time_bias_est_rate = RFE_PHY.fest_chan[xds.xfer_active_chan_phy] -
                     RFE_PHY.fest_chan[xds.xfer_active_chan_phy_prev];
                  }
/* if PTP to PHY switch */
                  else if((rms.prev_freq_source[0] < RM_GPS_1)&&(rms.freq_source[0] >= RM_GPS_1))
                  {
             	       debug_printf(GEORGE_PTP_PRT," PTP to PHY cur: %ld prev:%ld, new: %ld, old: %ld \n",(long int)RFE_PHY.fest_chan[xds.xfer_active_chan_phy],(long int)RFE.fest,(long)rms.freq_source[0],(long)rms.prev_freq_source[0]); 

                     local_time_bias_est_rate = RFE_PHY.fest_chan[xds.xfer_active_chan_phy] -
                     RFE.fest;
//                     local_time_bias_est_rate = 20.0;
                  }
/* if PHY to PTP switch */
                  else if((rms.prev_freq_source[0] >= RM_GPS_1)&&(rms.freq_source[0] < RM_GPS_1))
                  {
             	       debug_printf(GEORGE_PTP_PRT," PHY to PTP \n"); 

                     local_time_bias_est_rate = RFE.fest - RFE_PHY.fest_chan[xds.xfer_active_chan_phy_prev];
                  }

                  time_bias_est_rate += local_time_bias_est_rate;
             	   debug_printf(GEORGE_PTP_PRT,"Jam Time_Bias_Est_Rate: %ld \n", (long int) time_bias_est_rate); 
               }
               if((rms.freq_source[0] >= RM_GPS_1)&&!(start_in_progress))
               {
                  init_rfe_phy();  
                  start_rfe_phy(RFE_PHY.fest_chan[xds.xfer_active_chan_phy]);

				      fdrift_f=fdrift_r=fdrift=fdrift_warped=fdrift_smooth=pll_int_r=pll_int_f= fdrift_raw=holdover_r=holdover_f;
//                  skip_rfe_phy=70;
                  skip_rfe_phy=16;
               }
            }
            RFE_PHY.first=0;   
#endif 
 //           STOP_PTP=1;
//            detilt_blank[xds.xfer_active_chan_phy] = 16;
         	debug_printf(GEORGE_PTP_PRT,"channel change restart phy\n");         

//				fdrift_f=fdrift_r=fdrift=fdrift_warped=fdrift_smooth=pll_int_r=pll_int_f= fdrift_raw=holdover_r=holdover_f;

//				skip_rfe_phy=70;
//            go_to_bridge = TRUE;

            time_transient_flag = 1.5 * time_hold_off;
//          time_bias_est_rate=0.0; //GPZ TEST TEST TEST need to not change rate unless needed


		}

		// FOR NOW degenerate case is single channel at a time for frequency	
//		xds.xfer_turbo_phase_phy[0]=rfe_turbo_phase_phy[input_chan];
//		xds.xfer_turbo_phase_phy[1]=rfe_turbo_phase_phy[input_chan]; 
//		xds.xfer_turbo_phase_phy[2]=rfe_turbo_phase_phy[input_chan]; 
//		xds.xfer_turbo_phase_phy[3]=rfe_turbo_phase_phy[input_chan]; 


// This function reports offset frequency of requested PHY channel

//	}




	debug_printf(GEORGE_PTP_PRT,"freq_source: %d time_source: %d\n", rms.freq_source[0], rms.time_source[0]);         
   for(i=0;i<NUM_OF_SERVO_CHAN;i++)
   {
      s_prevTimeWeight[i] = ChanSelectTable.s_servoChanSelect[i].b_selectedTime;
      s_prevFreqWeight[i] = ChanSelectTable.s_servoChanSelect[i].b_selectedFreq;
   }	
#endif
}
#if(NO_PHY==0)
///////////////////////////////////////////////////////////////////////
// This function reports offset frequency of requested PHY channel
// 
//
//
/////////////////////////////////////////////////////////////////////////
static double foff_local[8];
double report_freq_offset(int chan)
{
#if 0 //TEST TEST TEST Feb 13 2012 moved to part of normal decision
   if(time_transient_flag)
   {
      return(foff_local[chan]);  //return previous state during a known transient
   }
#endif
//   foff_local[chan] = 0.0;
   if((chan >=RM_GPS_1)&&(chan<RM_NONE)) 
//   if((chan > RM_GPS_2)&&(chan<RM_RED)) // Only test Sync E channels
   {
         // is current time source GPS or PTP
         if((rms.time_source[0] < RM_GPS_1)&&(RFE.Nmax > 30)) //time source is PTP
         {
            if(time_transient_flag)
            {
               return(foff_local[chan]);  //return previous state during a known transient
            }
            else
            {
               foff_local[chan] = RFE_PHY.fest_chan[chan - RM_GPS_1] - RFE.fest;
//            debug_printf(GEORGE_PTP_PRT," report foff  PTP freq chan_freq  %le PTP time:%le ppb \n",RFE_PHY.fest_chan[chan -RM_GPS_1], RFE.fest); 
            }
         }
//         else if((rms.time_source[0] == RM_SE_1)&&(RFE_PHY.Nmax > 80))
         else if(((rms.time_source[0] == RM_GPS_1) || (rms.time_source[0] == RM_GPS_2) ||(rms.time_source[0]) == RM_GPS_3)&&
                     (RFE_PHY.Nmax > 0))
         {
            if(time_transient_flag)
            {
               return(foff_local[chan]);  //return previous state during a known transient
            }
            else
            {
               foff_local[chan] = RFE_PHY.fest_chan[chan -RM_GPS_1] - RFE_PHY.fest_chan[rms.time_source[0] -RM_GPS_1];
//            debug_printf(GEORGE_PTP_PRT," report foff  PHY freq chan_freq  %le PHY time:%le ppb \n",RFE_PHY.fest_chan[chan -RM_GPS_1], RFE_PHY.fest_chan[rms.time_source[0] -RM_GPS_1]); 
            }
         }
         else
         {
            foff_local[chan]=0.0;
         }
         if(foff_local[chan] < 0.0) foff_local[chan] = -foff_local[chan];
   }
   else
   {
         foff_local[chan]=0.0;

   }
//    debug_printf(GEORGE_PTP_PRT," report foff chan: %ld offset:%le ppb \n",(long int)chan, foff_local[chan]); 
//   foff_local[chan]=0.0; //TEST TEST TEST never reject
   return(foff_local[chan]);
}
#endif



///////////////////////////////////////////////////////////////////////
// This function coordinates data tranfer to from low speed tasks tasks
// 
//typedef struct
//{
//	XFER_State xstate;
//	int_8 working_sec_cnt;
//	int_8 duration;
//	int_8 idle_cnt;
//	// Protected RFE In Data
//	double xfer_sacc_f, xfer_sacc_r;
//	double xfer_var_est_f, xfer_var_est_r;
//	// Protected RFE Out Data
//	double xfer_fest, xfer_fest_f, xfer_fest_r;
//	double xfer_weight_f, xfer_weight_r;
// 	int_8	xfer_first;
//	int_16  xfer_Nmax;
//	// protected FLL status Data
//	double xfer_fdrift, xfer_fshape, xfer_tshape;
//	double xfer_pshape_smooth;
//	double xfer_scw_avg_f, xfer_scw_avg_r;
//	double xfer_scw_e2_f,xfer_scw_e2_r;
//}
//XFER_DATA_STRUCT;
///////////////////////////////////////////////////////////////////////
void sync_xfer_8s(void)
{
		INT8 xfer_wait/*,input_chan,chan_change*/;
//		SC_t_servoChanSelectTableType ChanSelectTable;
		switch(xds.xstate)
		{
    	case XFER_IDLE:
    	{
 			if(xds.idle_cnt)
 			{
 				xds.idle_cnt--;
//				debug_printf(GEORGE_PTP_PRT,"xfer idle count down: %d dur: %d\n", xds.idle_cnt,xds.duration);
 				
 			}
 			else  //tranfer protected data now
 			{
 //				debug_printf(GEORGE_PTP_PRT,"xfer idle transfer data: %d\n", xds.idle_cnt);
				// Protected RFE In Data
				xds.xfer_sacc_f= sacc_f;
				xds.xfer_sacc_r= sacc_r;
				xds.xfer_var_est_f= forward_stats.tdev_element[TDEV_CAT].var_est;
				xds.xfer_var_est_r= reverse_stats.tdev_element[TDEV_CAT].var_est;
				xds.xfer_in_weight_f=forward_weight;
				xds.xfer_in_weight_r=reverse_weight;
				//Protected RFE Out Data
//#if(GPS_ENABLE==1) //GPZ TEST TEST FORCE turbo input to be from GPS
/* PTP is active */
				if(Is_PTP_FREQ_ACTIVE()==TRUE)
				{
					xds.xfer_fest=RFE.fest;
					xds.xfer_first=RFE.first;
					xds.xfer_Nmax=RFE.Nmax;
				}
/* PHY is active */
            else if(Is_PHY_FREQ_ACTIVE()==TRUE)
				{
               #if(NO_PHY==0)
					xds.xfer_fest=RFE_PHY.fest;
					xds.xfer_first=RFE_PHY.first;
					xds.xfer_Nmax=RFE_PHY.Nmax;
               #endif
				}
/* Holdover case */
            else
            {
					xds.xfer_fest=holdover_f;
            }
//#else
/* PTP is active */
//				if(Is_PTP_FREQ_ACTIVE()==TRUE)
//				{
//					xds.xfer_fest=RFE.fest;
//					xds.xfer_first=RFE.first;
//					xds.xfer_Nmax=RFE.Nmax;
//				}
/* Holdover case */
//            else
//            {
//					xds.xfer_fest=holdover_f;
//            }
//#endif
				xds.xfer_fest_f=RFE.fest_for;
				xds.xfer_fest_r=RFE.fest_rev;
				xds.xfer_weight_f=RFE.weight_for;
				xds.xfer_weight_r=RFE.weight_rev;
				// protected FLL status Data
				xds.xfer_fdrift =fdrift;
				xds.xfer_fshape = fshape;
				xds.xfer_tshape= tshape;
				xds.xfer_pshape_smooth= pshape_smooth;
				xds.xfer_scw_avg_f= scw_avg_f;
				xds.xfer_scw_avg_r= scw_avg_r;
				xds.xfer_scw_e2_f= scw_e2_f;
				xds.xfer_scw_e2_r= scw_e2_r;
				xds.xfer_turbo_phase[0]=rfe_turbo_phase[0];
				xds.xfer_turbo_phase[1]=rfe_turbo_phase[1];
//				chan_change=0;
//				input_chan=0;
#if 0
            input_chan=0;
		      if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_GPS1].b_selectedFreq)
               input_chan=0;
      		else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_SYNCE1].b_selectedFreq)
               input_chan=2;
      		else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_SYNCE2].b_selectedFreq)
               input_chan=3;
      		else if(ChanSelectTable.s_servoChanSelect[SERVO_CHAN_RED].b_selectedFreq)
               input_chan=4;
#endif
 //				debug_printf(GEORGE_PTP_PRT,"xfer input phy chan: %d\n", input_chan);
           #if(NO_PHY==0)//GPZ  FORCE turbo input to be from GPS
//      		xds.xfer_turbo_phase_phy[0]=rfe_turbo_phase_phy[input_chan];
//      		xds.xfer_turbo_phase_phy[1]=rfe_turbo_phase_phy[input_chan]; 
//      		xds.xfer_turbo_phase_phy[2]=rfe_turbo_phase_phy[input_chan]; 
//      		xds.xfer_turbo_phase_phy[3]=rfe_turbo_phase_phy[input_chan]; 
            // New un ganged mode DEC 2011

        		xds.xfer_turbo_phase_phy[0]=rfe_turbo_phase_phy[0];
      		xds.xfer_turbo_phase_phy[1]=rfe_turbo_phase_phy[1]; 
      		xds.xfer_turbo_phase_phy[2]=rfe_turbo_phase_phy[2]; 
      		xds.xfer_turbo_phase_phy[3]=rfe_turbo_phase_phy[3]; 
      		xds.xfer_turbo_phase_phy[4]=rfe_turbo_phase_phy[4]; 


            #endif
 			}   	
    	}
		break;
		case XFER_START:
		{
			// update working second count
			xds.working_sec_cnt++;
		}
		break;
		case XFER_END:
		{
			// close out count
			xds.duration= xds.working_sec_cnt;
			xds.working_sec_cnt=0;
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTP_PRT,"xfer end: %d\n", xds.duration);
			#endif
			// Idle state started
			xds.xstate= XFER_IDLE;
			xfer_wait= (4- xds.duration); //reduce wait to 8 seconds (make 4 seconds for new turbo loop DEC 2010
			if(xfer_wait > 0)
			{
				xds.idle_cnt= xfer_wait; //count down delay in idle state
			}
			else
			{
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT,"exception xfer timeout: %d\n", xds.duration);
				#endif
			}	
		}
		break;
		default:
		break;
		}

}

#if(SYNC_E_ENABLE==1)
///////////////////////////////////////////////////////////////////////
// This function is called once per second to update synchronous ethernet 
// frequnecy estimates
// 
///////////////////////////////////////////////////////////////////////
void update_se(void)
{
	INT32 SE_Local_Offset,Local_Delta;
	double prev_phase,delta_phase,raw_freq;
	INT8 se_vflag,pre_vflag;
	// start by getting current offset reading
//	Get_SE_Offset(&SE_Local_Offset);
//	debug_printf(GEORGE_PTP_PRT,"se_update:offset:%ld\n",
//	SE_Local_Offset
//   		);
	// construct previous offset and delta_phase
//	debug_printf(GEORGE_PTP_PRT,"se_update:vflag:%d, index:%d\n",
//   SMC[0].SMF[SMC[0].index].Valid,
//   SMC[0].index
//   );
	se_vflag = (SMC[0].SMF[SMC[0].index].Valid)&&(SMC[0].se_valid);
	pre_vflag= (double)(SMC[0].SMF[SMC[0].index].Valid);
	if(se_vflag&&pre_vflag)
	{
		prev_phase= (double)(SMC[0].SMF[SMC[0].index].Offset);
  		Local_Delta= (SE_Local_Offset - prev_phase);
  		if(Local_Delta > (SE_MAX_PHASE/2))
  		{
  			Local_Delta -= SE_MAX_PHASE;
  		}
  		else if(Local_Delta < -(SE_MAX_PHASE/2))
  		{
  			Local_Delta += SE_MAX_PHASE;
  		}	
      if(Local_Delta> 2000) Local_Delta =2000.0;
      else if (Local_Delta<-2000) Local_Delta=-2000.0; 
  		delta_phase=(double) Local_Delta;	
 		raw_freq= (delta_phase*SE_LSB)/(double)(SE_WINDOW);
	}
	else
	{
		raw_freq= SMC[0].freq_se_hold; //use estimate to bridge trouble
	}	
	// insert new phase point and advance index
	SMC[0].SMF[SMC[0].index].Offset= SE_Local_Offset;
	SMC[0].SMF[SMC[0].index].Valid= se_vflag; //assume valid for now
	SMC[0].index=(SMC[0].index+1)%SE_WINDOW;
	// update frequency estimate
	raw_freq += fdrift_smooth; //open loop with respect to PTP corrections
	SMC[0].freq_est     += (1.0-SMC[0].alpha_tracking)*raw_freq;
	SMC[0].freq_se_hold = (1.0-SMC[0].alpha_holdover)*SMC[0].freq_est + SMC[0].alpha_holdover*SMC[0].freq_se_hold;
	// TODO STUFF calculate compensated freq-est against average add pop validation
	SMC[0].freq_se_corr=(SMC[0].freq_est-SMC[0].freq_se_hold);
	validate_se();
   if((Print_Phase%16)==12)
   {
	   debug_printf(GEORGE_PTP_PRT,"se_update:offset:%ld,prev_phase:%le,delta:%le, rfreq:%le,fest:%le,fest_hold:%le\n",
   		SE_Local_Offset,
   		prev_phase,
   		delta_phase,
   		raw_freq,
   		SMC[0].freq_est,
         SMC[0].freq_se_hold
   		);
   }

}
///////////////////////////////////////////////////////////////////////
// This function is called once per second to validate synchronous ethernet 
// frequnecy estimates
// Only support channel zero for first application
///////////////////////////////////////////////////////////////////////
void validate_se(void)
{
	int i;
	INT32 second_diff,peak_second_diff;
	INT32 diff,peak_diff;
	// extract current stats from buffer
	peak_second_diff=peak_diff=0;
	for(i=0;i<(SE_WINDOW-2);i++)
	{
		diff=SMC[0].SMF[i].Offset-SMC[0].SMF[i+1].Offset;	
		second_diff=SMC[0].SMF[i].Offset-2*SMC[0].SMF[i+1].Offset+SMC[0].SMF[i+2].Offset;
		if(diff<0) diff=-diff;	
		if(diff<0) second_diff=-second_diff;
		if(diff>peak_diff) peak_diff=diff;	
		if(second_diff>peak_second_diff) peak_second_diff=second_diff;	
	}
	SMC[0].se_valid=1;
	if(peak_diff>SE_DIFF_THRES)
	{
		SMC[0].se_valid=0;
	}
	else if(peak_second_diff>SE_SECOND_DIFF_THRES)
	{
		SMC[0].se_valid=0;
	}	
   if((Print_Phase%16)==14)
   {
	   debug_printf(GEORGE_PTP_PRT,"se_validate:valid:%ld,peak_diff:%ld,peak_second_diff:%ld,diff:%ld,second_diff:%ld\n",
   		SMC[0].se_valid,
   		peak_diff,
   		peak_second_diff,
   		diff,
   		second_diff
   		);
   }
}
///////////////////////////////////////////////////////////////////////
// This function call initializes Synchronous Ethernet Measurement
	// Data structures to support Synchronous Ethernet mode
//	typedef struct
// 	{
// 		int_16 Offset; //raw phase measurement minus current baseline phase
// 		int_8 Valid;	//Indicates current measurement was performed with a valid measurement channel
// 	} 
//	SE_MEAS_FIFO;
// 	typedef struct
//  	{
//	 		int_8  index;
//  		int_32 total_meas_cur, total_meas_lag; 
//  		int_16 cnt_meas_cur, cnt_meas_lag;
//  		int_32 baseline_delta, baseline_phase;
//  		int_16 phase_pop_cnt;
//  		int_8  baseline_valid;
//  		double freq_open_est; //open loop frequency ppb
//  		double freq_se_corr;  // correction to be applied (open - holdover from PTP)
//  		double freq_se_hold;  // holdover se freq to apply
//  		double alpha_tracking, alpha_holdover;
//  		SE_MEAS_FIFO SMF[SE_WINDOW_MAX]; 
//  	}
//	SE_MEAS_CHAN;
///////////////////////////////////////////////////////////////////////
static void init_se(void)
{
	int i,j;
	#if (N0_1SEC_PRINT==0)		
	debug_printf(GEORGE_PTP_PRT,"se_init\n");
	#endif
	for(i=0;i<SE_CHANS;i++)
	{
		SMC[i].index=0;
		SMC[i].total_meas_cur=0;
		SMC[i].total_meas_lag=0;
		SMC[i].cnt_meas_cur=0;
		SMC[i].cnt_meas_lag=0;
//		SMC[i].baseline_delta=0;
//		SMC[i].baseline_phase=0;
		SMC[i].phase_pop_cnt=0;
		SMC[i].se_valid=0;
//		SMC[i].freq_est=0.0;
//		SMC[i].freq_se_corr=0.0;
//		SMC[i].freq_se_hold=0.0;
		SMC[i].alpha_tracking= 0.95;
		SMC[i].alpha_holdover=0.9999;
		
		for(j=0;j<SE_WINDOW_MAX;j++)
		{
			SMC[i].SMF[j].Offset=0;
			SMC[i].SMF[j].Valid=0;
		}
	} // end per channel code
	
}
///////////////////////////////////////////////////////////////////////
// This function call starts Sync Ethernet Measurement Services
///////////////////////////////////////////////////////////////////////
static void start_se(double in)
{
	int i,j,k;
	INT32 SE_Local_Offset,SE_Local_Delta;
	for(i=0;i<SE_CHANS;i++)
	{
		SMC[i].total_meas_cur=0;
		SMC[i].total_meas_lag=0;
		SMC[i].cnt_meas_cur=0;
		SMC[i].cnt_meas_lag=0;
		SE_Local_Delta = (INT32)(in*(double)(SE_WINDOW/SE_LSB)+0.5);
//		Get_SE_Offset(&SE_Local_Offset);
//		SMC[i].baseline_phase=SE_Local_Offset;
		SMC[i].se_valid=1;
		// END set baseline code 
		SMC[i].phase_pop_cnt=0;
		SMC[i].freq_est=in;
		SMC[i].freq_se_corr= 0.0;
		SMC[i].freq_se_hold=0.0;
		SMC[i].alpha_tracking= 0.95;
		SMC[i].alpha_holdover=0.999;
		for(j=0;j<SE_WINDOW;j++)
		{
			k=(SE_WINDOW-j)%SE_WINDOW;
			SMC[i].SMF[k].Offset=SE_Local_Offset- (j*SE_Local_Delta);
			SMC[i].SMF[k].Valid=1;
		}
		#if (N0_1SEC_PRINT==0)		
		debug_printf(GEORGE_PTP_PRT,"se_start:i:%3d,offset:%ld,delta:%ld\n",
		#endif
	   		i,
    		SE_Local_Offset,
     		SE_Local_Delta
     		);
		
	} // end per channel code
	
}
#endif  //end of Sync Ethernet code modules
#if (ASYMM_CORRECT_ENABLE==1)

static int_8 match_acl;
///////////////////////////////////////////////////////////////////////
// This function is called once per second to update time bias 
// asymmetrical correction list
// 
///////////////////////////////////////////////////////////////////////
void update_acl(void)
{
	int i;
	int_8 match;
	double dtemp,time_err_A,time_err_B;
//	debug_printf(GEORGE_PTP_PRT,"acl_update:ACL.astate: %d  rtd: %le skip:%d\n",ACL.astate,round_trip_delay_alt,ACL.skip);
	switch(ACL.astate)
	{
   	case ASYMM_UNKNOWN :
   	{
   		if((fllstatus.cur_state==FLL_NORMAL))
   		{
   			if(!ACL.skip)
   			{
 	  			// search for roundtrip delay match
   				match=32;
   				i=0;
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT,"acl_update unknown state: rtd:%le\n",(round_trip_delay_alt));
				#endif
	   			while(i<match)
   				{
					#if (N0_1SEC_PRINT==0)		
					debug_printf(GEORGE_PTP_PRT,"acl_update match search: index:%d rtd_min:%le, rtd_max:%le\n"
					,i,ACL.ACE[i].rtd_min,ACL.ACE[i].rtd_max);
					#endif
   					if((round_trip_delay_alt>ACL.ACE[i].rtd_min) &&
   					   (round_trip_delay_alt<ACL.ACE[i].rtd_max))
   					{
	   				  match=i; //we have a match
   					  // prepare output and change state
   					  ACL.asymm_calibration=ACL.ACE[match].calibration;
   					  ACL.astate= ASYMM_OPERATE;
   					  ACL.index=match;
   					  match_acl=31;
   					  ACL.skip=30;
					  #if (N0_1SEC_PRINT==0)		
					  debug_printf(GEORGE_PTP_PRT,"acl_update match found: match:%3d\n",match);
					  #endif
   					}
	   				i++;
   				}
   			}
  	   		else if(ACL.skip) ACL.skip--;
   		}
   		else if (fllstatus.cur_state==FLL_FAST)
   		{
   			// set initial asymm value
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTP_PRT,"acl_update unknown state fast prime: rtd:%le\n",(round_trip_delay_alt));
			#endif
			ACL.ACE[0].cal_category=ASYMM_PRE_CALIBRATED;
			ACL.ACE[0].rtd_mean=round_trip_delay_alt;
			ACL.ACE[0].rtd_min=round_trip_delay_alt-2000.0;
			ACL.ACE[0].rtd_max=round_trip_delay_alt+2000.0;
			ACL.ACE[0].calibration= 0.0; //use 100ns test offset
		}	
   	}
	break;
	case ASYMM_OPERATE :
	{
		if(ACL.skip<10)
		{
			// is current index still relevant ?
 			if((round_trip_delay_alt>ACL.ACE[ACL.index].rtd_min) &&
 		   		(round_trip_delay_alt<ACL.ACE[ACL.index].rtd_max))
 			{
				  ACL.asymm_calibration=ACL.ACE[ACL.index].calibration;
				  if((ACL.skip) && (time_transient_flag == 0)) //GPZ Fevb 2011 protect measurement during time transients
				  {
				  	ACL.ACE[ACL.index].time_bias_meas=time_bias;
				  }
				  else
				  {
				  	dtemp= time_bias - ACL.ACE[ACL.index].time_bias_meas;
				  	if(dtemp> 2000.0) dtemp=2000.0;
				  	else if(dtemp<-2000.0)dtemp=-2000.0;
				  	ACL.ACE[ACL.index].time_bias_meas += 0.005*dtemp;
//				  	ACL.ACE[ACL.index].time_bias_meas = 0.995*ACL.ACE[ACL.index].time_bias_meas  +0.005*time_bias;
				  }	
				  #if (N0_1SEC_PRINT==0)		
				  if(Print_Phase%16==11)
				  {
				  	debug_printf(GEORGE_PTP_PRT,"acl_update operate state index is relevant: index: %d, cal:%le, tbias:%le, tbias_meas%le\n",ACL.index,ACL.asymm_calibration,time_bias,ACL.ACE[ACL.index].time_bias_meas );
				  }	
				  #endif
 			}  
 			else // shift to calibration state
 			{
				#if (N0_1SEC_PRINT==0)		
			  	debug_printf(GEORGE_PTP_PRT,"acl_update operate state index is unsure: index: %d, cal:%le, tbias:%le, tbias_meas%le\n",ACL.index,ACL.asymm_calibration,time_bias,ACL.ACE[ACL.index].time_bias_meas );
			  	#endif
				if((delta_forward_slew==0) && (delta_reverse_slew ==0))
				{			
	  				ACL.astate= ASYMM_CALIBRATE;
  					ACL.time_bias_baseline=ACL.ACE[ACL.index].time_bias_meas; 
  					ACL.skip=1250; //was 480
  				}
 			}
 		}	
   		if(ACL.skip)
   		{
   		 	ACL.skip--;
   		} 
	}
	break;
	case ASYMM_CALIBRATE :
	{
		if((ACL.skip<701)&&(time_transient_flag == 0)) // GPZ FEB 2011 only interate when time is good
		{
			time_bias_est = ACL.time_bias_baseline; //keep priming estimate 	
	debug_printf(GEORGE_PTP_PRT, "reset time_bias_est in ASYMM correct\n");
	
			if(1) // always search for possible match	
	 		{
 				match=32;
  				i=0;
   				while(i<match)
  				{
//					debug_printf(GEORGE_PTP_PRT,"acl_update match search:index:%d rtd_min:%le, rtd_max:%le\n"
//					,i,ACL.ACE[i].rtd_min,ACL.ACE[i].rtd_max);
					if(0) //GPZ Feb 2011 do not attempt perfect match too risky now this is a FUTURE
// 					if((round_trip_delay_alt>ACL.ACE[i].rtd_min) &&
// 				   	(round_trip_delay_alt<ACL.ACE[i].rtd_max))
  					{
   				  		match=i; //we have a possible match
  				  		//if possible match is a pre calibrated or autonomous entry then prepare output and change state
  				  		if(((ACL.ACE[match].cal_category)==ASYMM_PRE_CALIBRATED)||
  				  			((ACL.ACE[match].cal_category)==ASYMM_AUTONOMOUS))
  				  		{
  				  			// verify sign of correction
  				  			dtemp= time_bias - ACL.time_bias_baseline;
  				  			time_err_A = ACL.ACE[match].calibration-dtemp;
  				  			if(time_err_A < 0.0) time_err_A=-time_err_A;
 				  			time_err_B = ACL.ACE[match].calibration+dtemp;
  				  			if(time_err_B < 0.0) time_err_B=-time_err_B;
  				  			if(time_err_A < time_err_B)
  				  			{
  				  				if(time_err_A > 1000.0)
  				  				{
  				  					match=32; //NOV 28 2010 Reject match
  				  				}
  				  				else
  				  				{
	  								#if (N0_1SEC_PRINT==0)		
									debug_printf(GEORGE_PTP_PRT,"acl_update normal match found:index:%d err_A:%le, err_B:%le\n",
									match,
									time_err_A,
									time_err_B);
									#endif
  				  					ACL.asymm_calibration=ACL.ACE[match].calibration;
  				  				}
  				  			}
  				  			else
  				  			{
 				  				if(time_err_B > 1000.0)
  				  				{
  				  					match=32; //NOV 28 2010 Reject match
  				  				}
  				  				else
  				  				{
									#if (N0_1SEC_PRINT==0)		
  									debug_printf(GEORGE_PTP_PRT,"acl_update inverse match found:index:%d err_A:%le, err_B:%le\n",
									match,
									time_err_A,
									time_err_B);
									#endif
  				  					ACL.asymm_calibration=-ACL.ACE[match].calibration;
  				  				}
  				  			}
 				  			if(match<32) //NOV 28 2010 Reject match logic
 				  			{
	  				  			ACL.astate= ASYMM_OPERATE;
  					  			ACL.skip=30;
  			  					ACL.index=match;
								#if (N0_1SEC_PRINT==0)		
  								debug_printf(GEORGE_PTP_PRT,"acl_update match found: match:%3d\n",match);
  								#endif
								ACL.ACE[match_acl].cal_category=ASYMM_UN_CALIBRATED_A; //re-position cluster state machine
 				  			}
						}
						else
						{
							//force exit to recal for now always recal
							i=31;
						}	
						
   					}
					i++;
   				}
				if (i==32) //prime time_bias_meas A no match found
				{
					// find first available slot
					match=32;
					i=1; //never pick zero
   					while(i<match)
  					{
  				  		if(((ACL.ACE[i].cal_category)!=ASYMM_PRE_CALIBRATED)&&((ACL.ACE[i].cal_category)!=ASYMM_AUTONOMOUS))
  				  		{
  				  			match=i;
  				  		}
  				  		i++;
					}
					if(i==32) match=31;
					match_acl=match;
					if(ACL.ACE[match_acl].cal_category==ASYMM_UN_CALIBRATED)
					{
						ACL.ACE[match_acl].cal_category=ASYMM_UN_CALIBRATED_A; //start cluster state machine
						#if (N0_1SEC_PRINT==0)		
						debug_printf(GEORGE_PTP_PRT,"acl_update goto uncal_A category match %d\n",match_acl);
						#endif
						Forward_Floor_Change_Flag=1;
						Reverse_Floor_Change_Flag=1;
						skip_rfe=600;						
					}	
   				}
   		   				
			}
			if(ACL.astate!= ASYMM_OPERATE)
			{
		  			switch(ACL.ACE[match_acl].cal_category)
					{
					   	case ASYMM_UN_CALIBRATED_A :
   						{
   							// wait for 2 minutes
							if(ACL.skip<580) //was 300
							{
								ACL.ACE[match_acl].cal_category=ASYMM_UN_CALIBRATED_B;
							 	ACL.ACE[match_acl].time_bias_meas_A=time_bias;
								#if (N0_1SEC_PRINT==0)		
								debug_printf(GEORGE_PTP_PRT,"acl_update goto uncal_B category match %d\n",match_acl);
								#endif
								
							}
   						}
   						break;
					   	case ASYMM_UN_CALIBRATED_B :
   						{
							if(ACL.skip<420) //was 180
							{
								ACL.ACE[match_acl].cal_category=ASYMM_UN_CALIBRATED_C;
							 	ACL.ACE[match_acl].time_bias_meas_B=time_bias;
		 						#if (N0_1SEC_PRINT==0)		
								debug_printf(GEORGE_PTP_PRT,"acl_update goto uncal_C category match %d\n",match_acl);
								#endif
							 	
							}
   						}
   						break;
					   	case ASYMM_UN_CALIBRATED_C :
   						{
							if(ACL.skip<300) //was 180
							{
								ACL.ACE[match_acl].cal_category=ASYMM_UN_CALIBRATED_D;
							 	ACL.ACE[match_acl].time_bias_meas_C=time_bias;
								#if (N0_1SEC_PRINT==0)		
								debug_printf(GEORGE_PTP_PRT,"acl_update goto uncal_D category match %d\n",match_acl);
								#endif
							 	
							}
   						}
   						break;
					   	case ASYMM_UN_CALIBRATED_D :
   						{
							if(ACL.skip<180) //was 180
							{
								ACL.ACE[match_acl].cal_category=ASYMM_UN_CALIBRATED_E;
							 	ACL.ACE[match_acl].time_bias_meas_D=time_bias;
		 						#if (N0_1SEC_PRINT==0)		
								debug_printf(GEORGE_PTP_PRT,"acl_update goto uncal_E category match %d\n",match_acl);
								#endif
							 	
							}
   						}
   						break;
   						
					   	case ASYMM_UN_CALIBRATED_E :
   						{
							if(ACL.skip<60) //was 60
							{
							 	ACL.ACE[match_acl].time_bias_meas_E=time_bias;
							 	// verify cluster 
							 	dtemp= ACL.ACE[match_acl].time_bias_meas_A -
							 		2.0*ACL.ACE[match_acl].time_bias_meas_C +
							 		ACL.ACE[match_acl].time_bias_meas_E;
							 	if((dtemp>800.0) || (dtemp<-800.0)) //was 500
							 	{
							 		// redo cluster
		 							#if (N0_1SEC_PRINT==0)		
									debug_printf(GEORGE_PTP_PRT,"acl_update uncal redo cluster:,A: %le, B: %le, C: %le, dtemp: %le\n",
									ACL.ACE[match_acl].time_bias_meas_A,
									ACL.ACE[match_acl].time_bias_meas_C,
									ACL.ACE[match_acl].time_bias_meas_E,
									dtemp
									);
									#endif
									ACL.ACE[match_acl].time_bias_meas_A=ACL.ACE[match_acl].time_bias_meas_C;
		   							ACL.ACE[match_acl].cal_category=ASYMM_UN_CALIBRATED_A; //re-start cluster state machine
		   							ACL.skip=700; //
							 	}
							 	else
							 	{
							 		ACL.ACE[match_acl].time_bias_meas= (ACL.ACE[match_acl].time_bias_meas_A +
							 										ACL.ACE[match_acl].time_bias_meas_C +
							 										ACL.ACE[match_acl].time_bias_meas_E)/3.0;
									ACL.ACE[match_acl].cal_category=ASYMM_AUTONOMOUS;
									ACL.ACE[match_acl].calibration=	ACL.asymm_calibration+ACL.ACE[match_acl].time_bias_meas-ACL.time_bias_baseline; //TODO check sign						 										
						  			ACL.asymm_calibration=ACL.ACE[match_acl].calibration;
						  			ACL.astate= ASYMM_OPERATE;
				   					ACL.index=match_acl;
  						  			ACL.skip=30;
  						  			//TODO STORE RTD INFO in the table !!!!!
									ACL.ACE[match_acl].rtd_mean=round_trip_delay_alt;
									ACL.ACE[match_acl].rtd_min=round_trip_delay_alt-2000.0;
									ACL.ACE[match_acl].rtd_max=round_trip_delay_alt+2000.0;
									#if (N0_1SEC_PRINT==0)		
									debug_printf(GEORGE_PTP_PRT,"acl_update uncal autonomous cal complete:,A: %le, B: %le, C: %le, D: %le,E: %le, rtd: %le, dtemp: %le\n",
									ACL.ACE[match_acl].time_bias_meas_A,
									ACL.ACE[match_acl].time_bias_meas_B,
									ACL.ACE[match_acl].time_bias_meas_C,
									ACL.ACE[match_acl].time_bias_meas_D,
									ACL.ACE[match_acl].time_bias_meas_E,
									ACL.ACE[match_acl].rtd_mean,
									dtemp
									);
									#endif
									match_acl=31;
							 	}	
							 	
 							}
   						
   						}
   						break;
   						
   					}	
  		
			}
	
 		}	
   		if(ACL.skip)
   		{
   		 	ACL.skip--;
   		} 
	
	}
	break;
	default:
	break;
	}
}
///////////////////////////////////////////////////////////////////////
// This function is called to initialize the
// asymmetrical correction list
// 	typedef enum
//	{
//    	ASYMM_UNKNOWN      = 0,
//		ASYMM_OPERATE,
//		ASYMM_CALIBRATE,
//	} ASYMM_STATE;
//	typedef enum
//	{
//		ASYMM_UN_CALIBRATED		=0,
//		ASYMM_UN_CALIBRATED_A,
//		ASYMM_UN_CALIBRATED_B,
//		ASYMM_UN_CALIBRATED_C,
//		ASYMM_UN_CALIBRATED_D,
//		ASYMM_UN_CALIBRATED_E,
//	  	ASYMM_PRE_CALIBRATED,
//		ASYMM_AUTONOMOUS,
//	} ASYMM_CATEGORY;
//	typedef struct
//	{
//		ASYMM_CATEGORY 	cal_category;
//		int_16	cal_count; //number of samples included in the estimate
//		double	calibration;
//		double	time_bias_meas;
//		double 	bias_asymm_factor;
//		double	residual_noise;
//		int_32	time_last_used;
//		double	rtd_min;
//		double 	rtd_mean;
//		double 	rtd_max;
//	}ASYMM_CORR_ELEMENT;
//	typedef struct
//	{
//		int_8 	index;
//		int 	skip;
//		ASYMM_STATE	astate;
//		double asymm_calibration;
//		ASYMM_CORR_ELEMENT ACE[35];
//	}
///////////////////////////////////////////////////////////////////////
void init_acl()
{
	int i;
	ACL.index=0;
	ACL.skip=30;
	ACL.astate=ASYMM_UNKNOWN;
	ACL.asymm_calibration=0.0;
	#if (N0_1SEC_PRINT==0)		
	debug_printf(GEORGE_PTP_PRT,"acl_init start\n");
	#endif
	for(i=0;i<32;i++)
	{
		ACL.ACE[i].cal_category=ASYMM_UN_CALIBRATED;
		ACL.ACE[i].cal_count=0;
		ACL.ACE[i].calibration=0.0;
		ACL.ACE[i].time_bias_meas=0.0;
		ACL.ACE[i].time_bias_meas_A=0.0;
		ACL.ACE[i].time_bias_meas_B=0.0;
		ACL.ACE[i].time_bias_meas_C=0.0;
		ACL.ACE[i].bias_asymm_factor=0.0;// note use commom mode and diff mode timebias
		ACL.ACE[i].residual_noise=0.0;
		ACL.ACE[i].time_last_used=0;
		ACL.ACE[i].rtd_min=0.0;
		ACL.ACE[i].rtd_mean=0.0;
		ACL.ACE[i].rtd_max=0.0;
	}
	// TODO Add NVM support to get saved asymm database
	// for test make on record a match
//	ACL.ACE[8].cal_category=ASYMM_PRE_CALIBRATED;
//	ACL.ACE[8].rtd_min= 4889000.0;
//	ACL.ACE[8].rtd_mean=4890000.0;
//	ACL.ACE[8].rtd_max= 4891000.0;
//	ACL.ACE[8].calibration= -200.0; //use 100ns test offset
}

#endif //end of asymmetrical correction modules
///////////////////////////////////////////////////////////////////////
// This function call sets forward loop to optimal estimates
///////////////////////////////////////////////////////////////////////
static void cal_for_loop(void)
{
	sacc_f=0;
	Forward_Floor_Change_Flag=1;
//	skip_rfe=128;
	
}
///////////////////////////////////////////////////////////////////////
// This function call sets reverse loop to optimal estimates
///////////////////////////////////////////////////////////////////////
static void cal_rev_loop(void)
{
	sacc_r=0;
	Reverse_Floor_Change_Flag=1;
//	skip_rfe=128;
	
}
///////////////////////////////////////////////////////////////////////
// This function call sets exception gate
///////////////////////////////////////////////////////////////////////
static void set_exc_gate(INT16 index)
{
	exception_gate = exception_gate | (1<<index);
}
///////////////////////////////////////////////////////////////////////
// This function call reads exception gate
///////////////////////////////////////////////////////////////////////
static int read_exc_gate(INT16 index)
{
	INT32 gate;
	gate = exception_gate &(1<<index);
	if(gate)
	{
		return(0);
	}
	else
	{
		return(1);
	}
}
///////////////////////////////////////////////////////////////////////
// This function call clear exception gate
///////////////////////////////////////////////////////////////////////
void clr_exc_gate(INT16 index)
{
	INT32 mask;
	if(index < 32)
	{
		mask= (1<<index);
		mask= ~mask;
		exception_gate = exception_gate&(mask);
	}
	else
	{
		exception_gate=0;
	}	
}


///////////////////////////////////////////////////////////////////////
// This function set operating configuration 
//
///////////////////////////////////////////////////////////////////////
void set_oper_config(void)
{
			INT8 deactivate_flag;
 //        	t_ptpAccessTransportEnum e_ptpTransport;
#if ENTERPRISE
			enterprise_flag=1;
#else
			enterprise_flag=0;		
#endif			
#if 0			
#else
		// detect high mode jitter condition
		#if 0	
		if((min_oper_thres_f>49000) && (min_oper_thres_r>49000))  //GPZ FEB 2009 Post Tweak on 1.01.08
		{
			if((mode_jitter == 0) && (!start_in_progress))
			{
				init_pshape_long();		
			}
			mode_jitter=1;
		}	
		else if((min_oper_thres_f<47000) && (min_oper_thres_r<47000))  //GPZ FEB 2009 Post Tweak on 1.01.08
		{
			if((mode_jitter== 1) &&(!start_in_progress))
			{
				init_pshape_long();		
			}
			mode_jitter=0;
		}
		#endif
	    if((ptpTransport==e_PTP_MODE_ETHERNET)||(ptpTransport==e_PTP_MODE_HIGH_JITTER_ACCESS))
		{
			mode_jitter=0;
		}
		else if ((ptpTransport==e_PTP_MODE_MICROWAVE))
		{
			mode_jitter=0;
		}
		else if ((ptpTransport==e_PTP_MODE_DSL))
		{
			mode_jitter=0;
		}
		else
		{
			mode_jitter=1;
		}
		if(mode_jitter)
		{
			step_thres_oper = 500000; //was 2000000
			sacc_thres_oper = 100000000;
			RFE.alpha=1.0/20.0; // was 15 oscillator dependent smoothing gain across update default 1/60
			RFE.N=30;
			#if(NO_PHY==0)
 			RFE_PHY.alpha=1.0/50.0; // was 15 oscillator dependent smoothing gain across update default 1/60
			RFE_PHY.N=30;
         #endif
			diff_chan_err_oper= 100000.0; //was 500000
			lsf_window=500; 
			rm_size_oper = (128 > MAX_RM_SIZE)?MAX_RM_SIZE:128;
			min_oper_thres_oper=10000;  
			cluster_width_max = 2500000; //was 250000
			cluster_inc = 32*CLUSTER_INC;
			mode_width_max_oper=10000000;
			min_density_thres_oper=4;
			detilt_time_oper=2048;
			dsl_flag=1;
			lsf_smooth_g1 = 0.0008; //GPZ March 2010 increase smoothing
			lsf_smooth_g2 = 0.9992;
			nfreq_smooth_g1 = 0.01; //GPZ March 2010 generalize nfreq smoothing filter
			nfreq_smooth_g2 = 0.99;
			pshape_range = 250000; 		
			pshape_range_use = 225000; 
			pshape_norm_std    =  500000.0; //was 25000 Changed Jan 11 2010
			pshape_wide_std    =  500000.0; //was 40000 Changed Jan 11 2010
 			pshape_acq_fine    =  500000.0;
 			pshape_norm_fine_a =  500000.0;
 			pshape_norm_fine_b =  500000.0;
 			pshape_norm_fine_c =  500000.0;

 			
		}
		else
		{
			step_thres_oper = 11500;
			sacc_thres_oper = 100000000;
			RFE.alpha=1.0/10.0; // was 20 Ken_Post_Merge oscillator dependent smoothing gain across update default 1/60
			RFE.N=30;
			#if(NO_PHY==0)
			RFE_PHY.alpha=1.0/50.0; // was 15 oscillator dependent smoothing gain across update default 1/60
			RFE_PHY.N=30; 
			#endif			
			
			diff_chan_err_oper= 30000.0;
			rm_size_oper = (128 > MAX_RM_SIZE)?MAX_RM_SIZE:128; //was 64 change june 22 2009
			min_oper_thres_oper=200;
			cluster_width_max = 60000; //increase to 60000 to accomodate 15 node case
			cluster_inc= 4*CLUSTER_INC;//set to 4x NOV 2010 but only in turbo mode
//			cluster_inc_f = 8*CLUSTER_INC;
//			cluster_inc_r = 8*CLUSTER_INC;
			mode_width_max_oper=300000;
			min_density_thres_oper=4;
			detilt_time_oper=1280;
			dsl_flag=0;
			pshape_range = 50000; 		
			pshape_range_use =   30000; //increased from 18000
 			pshape_norm_std    = 10000.0; //decrease to 10000 Nov 2010
 			pshape_wide_std    = 86400.0;
		}
		if ((ptpTransport== e_PTP_MODE_MICROWAVE)) //microwave specific settings
		{
			mode_width_max_oper=500000;
			cluster_width_max = 150000; 
		}	
		else if ((ptpTransport== e_PTP_MODE_DSL	)) //microwave specific settings
		{
			mode_width_max_oper=5000000;
			cluster_width_max = 1500000; 
			step_thres_oper = 	500000; 
			min_oper_thres_oper=10000;  
		}
		//GPZ rfe turbo condition code
		// should we activate 16x turbo mode //now 16x DEC 2010 Test case
		if((rfe_turbo_flag==2) && fll_settle> 8) fll_settle=7; //GPZ DEC 2010 speed up loop settling function
#if(NO_PHY==0)
//		RFE_PHY.first=8; //force constant first mode to test this area
		if(rfe_turbo_flag_phy==0 && ((fllstatus.cur_state==FLL_FAST)||(fllstatus.cur_state==FLL_NORMAL)))
		{
			rfe_turbo_flag_phy=2;
			RFE_PHY.first=8; //GPZ DEC 2010 dwell in first longer than 2 cycles 
			PHY_Floor_Change_Flag[0]=2;//must change floor to avoid mixing observation window times
			PHY_Floor_Change_Flag[1]=2;//must change floor to avoid mixing observation window times
			PHY_Floor_Change_Flag[2]=2;//must change floor to avoid mixing observation window times
			PHY_Floor_Change_Flag[3]=2;//must change floor to avoid mixing observation window times
			PHY_Floor_Change_Flag[4]=2;//must change floor to avoid mixing observation window times
			PHY_Floor_Change_Flag[5]=2;//must change floor to avoid mixing observation window times
			start_rfe_phy(fdrift); //renormalize RFE data to 16x speed
			skip_rfe_phy=16;
		}
		rfe_turbo_flag_phy=2;
		rfe_turbo_flag_forward_phy=TRUE;
		rfe_turbo_flag_reverse_phy=TRUE;			
#endif
		// HQ APRIL 2011
//		if( (fll_stabilization>0) || ( (FLL_Elapse_Sec_PTP_Count> 300) && (OCXO_Type()!=e_RB)  && (mode_jitter==0) )   ) // skip during initialization or Rb build
		if( (fll_stabilization>0) || ( (FLL_Elapse_Sec_PTP_Count> 300) && (mode_jitter==0) )   ) // skip during initialization or Rb build
		{
			if(0) //disable 16x for PTP flows
//			if((OCXO_Type()!=e_OCXO)&&(OCXO_Type()!=e_RB)&&(FLL_Elapse_Sec_PTP_Count<4800)&&(xds.xfer_first)&&(rfe_turbo_flag==0)&&((min_oper_thres_f<2400)||(min_oper_thres_r<2400))) //very good floor in one direction
			{
				rfe_turbo_flag_forward=FALSE;	
				rfe_turbo_flag_reverse=FALSE;	
				if(min_oper_thres_f<2400)
				{
					rfe_turbo_flag_forward=TRUE;
				}
				if(min_oper_thres_r<2400)
				{
					rfe_turbo_flag_reverse=TRUE;
				}
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "test to activate 16x\n");
				#endif

				if((xds.xfer_first)&& ((fllstatus.cur_state==FLL_FAST)||(fllstatus.cur_state==FLL_NORMAL)))
				{
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "activate 16x\n");
				#endif
				rfe_turbo_flag=2; //16x turbo condition
				RFE.first=8; //GPZ DEC 2010 dwell in first longer than 2 cycles 
				Forward_Floor_Change_Flag=2;//must change floor to avoid mixing observation window times
				Reverse_Floor_Change_Flag=2;
				init_pshape();
				init_rfe();
				time_transient_flag=256;				
				start_rfe(fdrift); //renormalize RFE data to 8x speed
				skip_rfe=64;
				}
			}// activate 16x turbo mode
			//deactivate 16x mode
			if(rfe_turbo_flag==2)
			{
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "Test to de-activate 16x: min_f:%ld, min_r:%ld\n",
				min_oper_thres_f, min_oper_thres_r);
				#endif
				if( (((min_oper_thres_f>3000)&&(min_oper_thres_r>3000))&&(RFE.first))
					||(RFE.first==0))
				{
					#if (N0_1SEC_PRINT==0)		
					debug_printf(GEORGE_PTP_PRT, "de-activate 16x\n");
					#endif
					rfe_turbo_flag=FALSE;
						rfe_turbo_flag_forward=FALSE;	
						rfe_turbo_flag_reverse=FALSE;	
					Forward_Floor_Change_Flag=2;
					Reverse_Floor_Change_Flag=2;
					init_pshape();
					init_rfe();
					time_transient_flag=256;				
					start_rfe(fdrift); //renormalize RFE data to 1x speed
					skip_rfe=128;
				}					
			} //deactivate  16x rfe_turbo	
		// should we activate 4x turbo mode 
//			if (((rfe_turbo_flag==0)&&(fll_stabilization>0))  || (  (rfe_turbo_flag==0)  && (OCXO_Type()!=e_RB) &&
//			   ((min_oper_thres_f<2400)||(min_oper_thres_r<2400)) && (FLL_Elapse_Sec_PTP_Count<7000))) //good floor in both directions
			if (((rfe_turbo_flag==0)&&(fll_stabilization>0))  || (  (rfe_turbo_flag==0)  &&
			   ((min_oper_thres_f<2400)||(min_oper_thres_r<2400)) && (FLL_Elapse_Sec_PTP_Count<7000))) //good floor in both directions
			{	
				rfe_turbo_flag_forward=TRUE;	
				rfe_turbo_flag_reverse=TRUE;	
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "test to activate 4x\n");
				#endif
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "activate 4x\n");
				#endif
//				rfe_turbo_flag=1; //4x turbo condition
				Forward_Floor_Change_Flag=2;//must change floor to avoid mixing observation window times
				Reverse_Floor_Change_Flag=2;
				init_pshape();
				init_rfe();
				rfe_turbo_flag=1; //4x turbo condition
				time_transient_flag=256;				
				start_rfe(fdrift); //renormalize RFE data to 4x speed
//				skip_rfe=64;
				skip_rfe=16;
			}//activate 4x turbo mode always operate in 4x turbo for ocxo and below
//			else if((rfe_turbo_flag==0)&& ((OCXO_Type() == e_MINI_OCXO)||(OCXO_Type() == e_TCXO)||(OCXO_Type() == e_OCXO))    )
			else if(rfe_turbo_flag==0)
			{
				rfe_turbo_flag_forward=TRUE;	
				rfe_turbo_flag_reverse=TRUE;	
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "test to activate mini 4x\n");
				#endif
				rfe_turbo_flag=1; //4x turbo condition
				Forward_Floor_Change_Flag=2;//must change floor to avoid mixing observation window times
				Reverse_Floor_Change_Flag=2;
				init_pshape();
				init_rfe();
				time_transient_flag=256;				
				start_rfe(fdrift); //renormalize RFE data to 4x speed
				skip_rfe=64;
			}//activate 4x turbo mode
			//deactivate 4x mode
			if(rfe_turbo_flag==1)
			{
				//GPZ allow quicker exit from turbo 2	
//		        if((OCXO_Type() == e_MINI_OCXO)||(OCXO_Type() == e_TCXO)||(OCXO_Type() == e_OCXO)) 
		        if(1) 
		        {
		        	deactivate_flag=FALSE;
		        }
		        else
		        {
		        	if(FLL_Elapse_Sec_PTP_Count>7200)
		        	{
			        	deactivate_flag=TRUE;
			        }	
			        else
			        {
			        	deactivate_flag=FALSE;
			        }
		        }
				if(deactivate_flag)
				{
					#if (N0_1SEC_PRINT==0)		
					debug_printf(GEORGE_PTP_PRT, "de-activate 4x\n");
					#endif
					rfe_turbo_flag=FALSE;
						rfe_turbo_flag_forward=FALSE;	
						rfe_turbo_flag_reverse=FALSE;	
					Forward_Floor_Change_Flag=2;
					Reverse_Floor_Change_Flag=2;
//					if(FLL_Elapse_Sec_Count>4600)
//					{
//						start_rfe(holdover_f); //GPZ Aug 2010 renormalize RFE data to 1x speed
//					}
//					else
					{
						init_pshape();
						init_rfe();
						time_transient_flag=256;				
						start_rfe(fdrift); //renormalize RFE data to 1x speed
					}	
					skip_rfe=128;
				}			
			} //deactivate  4x rfe_turbo	
		} //initialization skip logic
		if(var_cal_flag==1) // override for calibration cycle
		{
			step_thres_oper = 100000; //was 2000000
			sacc_thres_oper = 500000;
			diff_chan_err_oper= 100000.0; //was 500000
			rm_size_oper = (64 > MAX_RM_SIZE)?MAX_RM_SIZE:64;
			min_oper_thres_oper=20000;
			cluster_width_max = 250000;
			cluster_inc= 8*CLUSTER_INC;
			cluster_inc_f = 8*CLUSTER_INC;
			cluster_inc_r = 8*CLUSTER_INC;
			mode_width_max_oper=900000;
			min_density_thres_oper=1;
			detilt_time_oper=2048;
			dsl_flag=1;
			enterprise_flag=0;
		}
	#endif
if(OCXO_Type()==e_MINI_OCXO)
{
 			if(mode_jitter==0)
			{
				RFE.alpha=1.0/12.0;  // was 1 over 12.0
				RFE.N=12;  //was 15
            N_init_rfe=12;
   			#if(NO_PHY==0)
//				RFE_PHY.alpha=1.0/24.0; // was 15 oscillator dependent smoothing gain across update default 1/40
				RFE_PHY.alpha=1.0/12.0; // was 15 oscillator dependent smoothing gain across update default 1/40
				RFE_PHY.N=12;//was 15
				#endif			
				
				rm_size_oper = (128 > MAX_RM_SIZE)?MAX_RM_SIZE:128;
				min_oper_thres_oper=425; //was 200 increase to 425 to improve performance with sawtooth phy noise
				lsf_smooth_g1 = 0.005;
				lsf_smooth_g2 = 0.995;
				hgain_floor=0.01; //Feb 2011 accelerate for oTCXO				
				if(rfe_turbo_flag)
				{
					nfreq_smooth_g1 = 1.0; //GPZ March 2010 generalize nfreq smoothing filter
					nfreq_smooth_g2 = 0.0;
				}
				else
				{				
//					nfreq_smooth_g1 = 0.002; //GPZ March 2010 generalize nfreq smoothing filter
//					nfreq_smooth_g2 = 0.998;
					nfreq_smooth_g1 = 0.05; //GPZ NOV 2010 speed up to prevent transient leakage
					nfreq_smooth_g2 = 0.95;

				}	
				// Ken_Post_Merge Tweaked MOC case 360, 360, 480, 600 
	 			pshape_acq_fine    =  300.0; //April 2011 steady turbo baseline 240 Explore ultra-fast  120
 				pshape_norm_fine_a =  600.0; //Had good run at 2000 try 180
 				pshape_norm_fine_b =  1000.0; // 1000
 				pshape_norm_fine_c =  2000.0; // 2000
	 			pshape_norm_std    =  7200.0; // 7200
 				pshape_wide_std    =  86400.0;// was 86400 April 2011
				pshape_range_use =    40000; 
			}
			else
			{
			}	
				min_density_thres_oper=2;
//#else //OCXO based parameters
}
else
{
				RFE.alpha=1.0/24.0;  // was 1 over 48
				RFE.N=12;  //was 15
            N_init_rfe=12;
   			#if(NO_PHY==0)
				RFE_PHY.alpha=1.0/48.0; // was 15 oscillator dependent smoothing gain across update default 1/60
				RFE_PHY.N=15;//was 15
				#endif			
				rm_size_oper = (128 > MAX_RM_SIZE)?MAX_RM_SIZE:128;
				min_oper_thres_oper=425; //was 200 increase to 1000 to improve performance with sawtooth phy noise
				// GPZ phase in order last settings 600, 600, 1000, 2000, 7200, 86400 : 600, 1200, 2000,4000,7200,100000
				pshape_acq_fine    = 1200.0; 
 				pshape_norm_fine_a = 1800.0; 
 				pshape_norm_fine_b = 3600.0; 
 				pshape_norm_fine_c = 7200.0; 
 				pshape_norm_std    = 10000.0;  //was 7200  
 				pshape_wide_std    = 100000.0; 
				pshape_range_use =   20000.0; //GPZ March 2011 restrict useable range for OCXO application was 5000
 				
				lsf_smooth_g1 = 0.0008; //GPZ March 2010 increase smoothing
				lsf_smooth_g2 = 0.9992;
				hgain_floor=0.001;				
				if(rfe_turbo_flag)
				{
					nfreq_smooth_g1 = 1.0; //GPZ April 2011
					nfreq_smooth_g2 = 0.0;
				}
				else
				{				
//					nfreq_smooth_g1 = 0.002; //GPZ March 2010 generalize nfreq smoothing filter
//					nfreq_smooth_g2 = 0.998;
					nfreq_smooth_g1 = 0.05; //GPZ NOV 2010 speed up to prevent transient leakage
					nfreq_smooth_g2 = 0.95;
					
				}	
}
//#endif
	if(ptpTransport == e_PTP_MODE_SLOW_ETHERNET) //DEC 2010 extend settling window in slow settling conditions
	{
		step_thres_oper= 1000000; //GPZ Sept 2010 Experiment force large pop count
		rm_size_oper = (1024 > MAX_RM_SIZE)?MAX_RM_SIZE:1024; //GPZ maximize protection from outliers
	}
	#if(ASYMM_CORRECT_ENABLE==1)
		rm_size_oper = (600 > MAX_RM_SIZE)?MAX_RM_SIZE:600; //GPZ maximize protection from outliers
		cluster_width_max = 1000; //GPZ NOV 12 2010 force only cluster at physical floor
	#endif
	#if (N0_1SEC_PRINT==0)		
	debug_printf(GEORGE_PTP_PRT, "config:%5d,type: %d,step:%ld,sacc:%ld,rmsize:%ld,min_clus:%ld,mode_max:%ld,min_den:%ld,detilt:%ld,dslflag:%ld,var_cal_flag %ld transport %d\n",
     FLL_Elapse_Sec_Count_Use,
     (int)(OCXO_Type() ),
     (long int)step_thres_oper,
     (long int)sacc_thres_oper,
	 (long int)rm_size_oper,
	 (long int)min_oper_thres_oper,
	 (long int)mode_width_max_oper,
	 (long int)min_density_thres_oper,
	 (long int)detilt_time_oper,
	 (long int)dsl_flag,
	 (long int)var_cal_flag,
	 ptpTransport	 	 
	 );
	 #endif
}


void Local_Minute_Task(void)
{
		if((Minute_Phase%8)==0) // was 4 
		{
			clr_exc_gate(32); // force clear of exception pacing every N minute
			#if (N0_1SEC_PRINT==0)		
   			debug_printf(GEORGE_PTP_PRT,"clear debug gate:%d\n",Minute_Phase);
   			#endif
   		}	
//		set_oper_config(); //verify configuration
		Minute_Phase++;
}
#if 0
void Local_16s_Task(void)
{
		if(rfe_turbo_flag) // use 8x turbo RFE operation 
		{
			update_rfe();
   		}	
}
#endif
int SC_RunAnalysisTask(void) //runs as lower priority GPZ DEC 2010 Try 8 second backround task
{
		int update_rfe_flag;
      UINT64 start_time;
      UINT64 end_time;

      /* if SC_InitChanConfig() has not been successfully run at least once,
         checked for the servo only case in which SC_InitConfigComplete() is not run */
      if (!Is_ChanConfigured())
         return -2;

      start_time = GetTime();

		update_rfe_flag=0;
		xds.xstate=XFER_START;
//		Get_Sync_Rate(&ptpRate);	 	//GPZ DEC 2010 
		ptpRate = ChanSelectTable.s_servoChanSelect[SERVO_CHAN_PTP1].w_measRate;
		Get_Access_Transport(&ptpTransport); //GPZ DEC 2010	
		set_oper_config(); //verify configuration
      select_channel();		


       if(hcount_recovery_rfe_start)
       {
      		debug_printf(GEORGE_PTP_PRT,"Holdover Recovery Restart of Packet RFE\n");

               warm_start_rfe(holdover_f);
//               warm_start_rfe(125.0);
               min_count = 0;
               skip_rfe=0;
               hcount_recovery_rfe_start=0;
       }


#if 0
		debug_printf(GEORGE_PTP_PRT,"Phy_Channel Frequency Offset:%ld , %ld, %ld, %ld , %ld\n",
      (long int)report_freq_offset(2),
      (long int)report_freq_offset(3),
      (long int)report_freq_offset(4),
      (long int)report_freq_offset(5),
      (long int)report_freq_offset(6));
#endif
      if(rfe_turbo_flag)
      {

//	      debug_printf(GEORGE_PTP_PRT,"ALignment of 8 second task skew %d \n",IPD.phase_index);

			if(OCXO_Type() == e_MINI_OCXO)
			{
//	   	   if((IPD.phase_index > 8)||(IPD.phase_index == 0))
		      if( ( (IPD.phase_index%32) > 8)||( (IPD.phase_index%32) == 0) )
	  	   	{
	      	  debug_printf(GEORGE_PTP_PRT,"Background 8 second task skew %d \n",IPD.phase_index);
   	     	  IPD.phase_index = 4; // was 4
      	 	}
			}
			else if(OCXO_Type() == e_OCXO)
			{
		      if( ( (IPD.phase_index%32) > 8)||( (IPD.phase_index%32) == 0) )
   		   {
      		  debug_printf(GEORGE_PTP_PRT,"Background 8 second task skew %d \n",IPD.phase_index);
        		  IPD.phase_index = 4; // was 4
	       	}
			}
			else
			{
		      if( ( (IPD.phase_index%32) > 8)||( (IPD.phase_index%32) == 0) )
   		   {
      		  debug_printf(GEORGE_PTP_PRT,"Background 8 second task skew %d \n",IPD.phase_index);
	        	  IPD.phase_index = 4; // was 4
   	    	}
			}

       }
//		debug_printf(GEORGE_PTP_PRT,"Access Transport is:%d\n",ptpTransport);
		
		#if(NO_PHY==0)
		debug_printf(GEORGE_PTP_PRT,"xfer start: %d, skip_rfe %d, skip_rfe_phy %d, min_count %d \n", xds.working_sec_cnt, skip_rfe, skip_rfe_phy, (unsigned int)min_count);
		#else
		debug_printf(GEORGE_PTP_PRT,"xfer start: %d, skip_rfe %d, min_count %d \n", xds.working_sec_cnt, skip_rfe,(int)min_count);
		#endif
//		if((fllstatus.cur_state== FLL_NORMAL)||(fllstatus.cur_state== FLL_FAST) || (fllstatus.cur_state== FLL_STABILIZE))
		if(fllstatus.cur_state!= FLL_WARMUP) //Include Holdover and Bridging
		{

			#if(NO_PHY==0)
			if(skip_rfe_phy==0)
			{

				update_rfe_phy();
			}
			#endif				
			if(rfe_turbo_flag==2)
			{

					update_rfe();
					update_rfe_flag=1;
			}
			else if(rfe_turbo_flag==1)
			{

				if(OCXO_Type() == e_MINI_OCXO)
				{

 					if((min_count%2)==0) //2 8 seconds update or 16 seconds 
					{
						update_rfe();
						update_rfe_flag=1;
					}
				}
				else if(OCXO_Type() == e_OCXO)
				{
 					if((min_count%2)==0) //4 8 seconds update or 32 seconds
					{
						update_rfe();
						update_rfe_flag=1;
					}
				}
				else
				{
 					if((min_count%2)==0) // 4 8 seconds update or 32 seconds
					{
						update_rfe();
						update_rfe_flag=1;
					}
				}

			}
			else if(skip_rfe==0)
			{
				// GPZ Jan 2011 just use 2 minute fpair control
//				#if(ASYMM_CORRECT_ENABLE==1)
//					if(OCXO_Type()==e_RB) //GPZ OCT 2010 use 4 min for Asymm Case
//					{
//	 					if((min_count%32)==0) //16 16 seconds update or 256 second 4 minutes GPZ change from 16 to 32
//						{
//							update_rfe();
//							update_rfe_flag=1;
//						}				
//					}
//					else
//					{
//	 					if((min_count%16)==0) //8 16 seconds update or 128 second 2 minutes GPZ change from 8 to 16
//						{
//							update_rfe();
//							update_rfe_flag=1;
//						}
//					}
//				#else
	 					if((min_count%16)==0) //8 16 seconds update or 128 second 2 minutes GPZ change from 8 to 16
						{
							update_rfe();
							update_rfe_flag=1;
						}
//				#endif	
				
			}
#if SMARTCLOCK == 1
         Holdover_Update(); //June 2011 call smart clock holdover routine					
#endif
			min_count++;
		}	
#ifdef TEMPERATURE_ON
		temperature_analysis(); // update temperature metrics		
#endif
		#if(!NO_IPPM)				
		ippm_analysis();
		#endif	
		ztie_calc(); //update ztie 15 minute metrics	
		fll_status_1min(); //update background status info
		
		if(!update_rfe_flag)
		{
			if((min_count%4)==0) // 64 second updates
			{
				fll_print();
			}
			else if((min_count%4)==1) // 64 second updates
			{
				combiner_print();
	 		 	debug_printf(GEORGE_PTP_PRT, "min_val_f:valid:%2d, indx:%2d, floor:%2d, in_min:%6ld,out_min:%6ld,rmc_f: %2d,lvm_f:%6ld \n",
 			 	mflag_f,
 			 	rmi_f,
 		 		rmf_f,
	 		 	min_forward.sdrift_min_1024,
 			 	om_f,
	 		 	rmc_f,
 			 	lvm_f);
		 	debug_printf(GEORGE_PTP_PRT, "min_val_r:valid:%2d, indx:%2d, floor:%2d, in_min:%9ld,out_min:%9ld,rmc_r: %2d,lvm_r:%9ld \n",
 		 	mflag_r,
 		 	rmi_r,
 		 	rmf_r,
 		 	min_reverse.sdrift_min_1024,
 		 	om_r,
 		 	rmc_r,
 		 	lvm_r);
 			 	
				
			}
			else if((min_count%4)==2) // 64 second updates
			{
				ofd_print();
				debug_printf(GEORGE_PTP_PRT, "hold_phase_f %d, gain2: %ld gain1: %ld holdover_f ppt %ld in ppt %ld\n",(int)(hold_phase_f),(long)(hgain2*10000.0),(long)(hgain1*10000.0),(long)(holdover_f*1000.0),(long)(hprev_f*1000.0));
				debug_printf(GEORGE_PTP_PRT, "hold_phase_r %d, gain2: %ld gain1: %ld holdover_r ppt %ld in ppt %ld\n",(int)(hold_phase_r),(long)(hgain2*10000.0),(long)(hgain1*10000.0),(long)(holdover_r*1000.0),(long)(hprev_r*1000.0));
			}
			else if((min_count%4)==3) // 64 second updates
			{
				forward_tdev_print();
				reverse_tdev_print();
			
				debug_printf(GEORGE_PTP_PRT, "time for: %ld  min_f:%ld cal_f:%ld iof:%ld \n",
    		    FLL_Elapse_Sec_Count_Use,
        		(long)sdrift_min_forward,
		        (long)sdrift_cal_forward,
    		    (long)Initial_Offset
        		);

				debug_printf(GEORGE_PTP_PRT, "time rev: %ld  min_r:%ld cal_r:%ld ior:%ld \n",
    		    FLL_Elapse_Sec_Count_Use,
        		(long)sdrift_min_reverse,
		        (long)sdrift_cal_reverse,
    		    (long)Initial_Offset_Reverse
        		);
				debug_printf(GEORGE_PTP_PRT, "time com: %ld rtd ns:%ld tshape ppt:%ld tbo ns:%ld tbase ns:%ld gain:%ld psteer:%ld tbias_est:%ld tbias_rate:%ld\n",
    		    FLL_Elapse_Sec_Count_Use,
        		(long) round_trip_delay_alt,
	        	(long) (tshape*1000.0),
	    	    (long)(time_bias_offset),
    	    	(long)(time_bias_base),
	    	    (long)(ltgain*10000.0),
    	    	psteer,
	        	(long) time_bias_est,
	        	(long) (time_bias_est_rate*1000.0)
		        );
				debug_printf(GEORGE_PTP_PRT, "time comp: %ld slew_err ns:%ld comm:%ld diff:%ld tcomp:%ld \n",
        		FLL_Elapse_Sec_Count_Use,
		        (long)time_slew_err,
    		    (long) tgain_comm,
        		(long) tgain_diff,
	        	(long) (tgain_comp*1000.0));
			}
		}
//		min_print(& min_forward);
//		min_print(& min_reverse);
		
		xds.xstate=XFER_END;
      end_time = GetTime();

      debug_printf(CPU_BANDWIDTH_PRT, "%s thread %lld ns\n", __func__, end_time-start_time);

      return 0;
}

#ifndef TP500_BUILD
static FIFO_STRUCT print_fifo_buf;
void Print_FIFO_Buffer(UINT16 index, UINT16 fast)
{
	UINT16 i;
		
	switch(index)
	{
	case 0:
		#if (N0_1SEC_PRINT==0)		
		debug_printf(UNMASK_PRT, "GM SYNC flow FIFO\n");
		#endif
//		_int_disable();
		memcpy((char*)&print_fifo_buf,(char*)&PTP_Fifo_Local,sizeof(FIFO_STRUCT));
//		_int_enable();
		break;
	case 1:
	default:
		#if (N0_1SEC_PRINT==0)		
		debug_printf(UNMASK_PRT, "Delay flow FIFO\n");
		#endif
//		_int_disable();
		memcpy((char*)&print_fifo_buf,(char*)&PTP_Delay_Fifo_Local,sizeof(FIFO_STRUCT));
//		_int_enable();
		break;
	}
	#if (N0_1SEC_PRINT==0)		
	debug_printf(UNMASK_PRT, "%hd fifo_in_index: %d fifo_out_index: %d\n", 
			index,
			print_fifo_buf.fifo_in_index, 
			print_fifo_buf.fifo_out_index);
	#endif		
	
	for(i=0;i<FIFO_BUF_SIZE;i++)
	{
		#if (N0_1SEC_PRINT==0)		
		debug_printf(UNMASK_PRT, "%d offset: %d\n",
			i,
			(int)print_fifo_buf.offset[i]);
		#endif	
		if(fast)
		{
			i += 63;
		}
	}

}
#endif
/*********************************************************************************
// Routine should be called once per minutes while Varactor_Cal_Flag is set
*********************************************************************************/

void Varactor_Cal_Routine(void)
{
		int i;
		double delta,avg;
		#if (N0_1SEC_PRINT==0)		
		debug_printf(GEORGE_PTP_PRT, "enter vcal: state: %3d\n",vstate);
		#endif
 		switch(vstate)
		{
		case VCAL_START: //start varactor cal initialize working copy of cal data
//			vcal_working.N=32;
			vcal_working.N=16; //Test at 2x speed
			vcal_working.min_freq =-512.0;
			vcal_working.max_freq =+512.0;
			vcal_working.index=0;
			vcal_working.delta_freq = (vcal_working.max_freq-vcal_working.min_freq)/(double)(vcal_working.N);
			for(i=0;i< VAR_TABLE_SIZE; i++)
			{
				vcal_working.vcal_element[i].seg_ffo=ZEROF;
				vcal_working.vcal_element[i].seg_inc_slope=ZEROF;
				vcal_working.vcal_element[i].done_flag=0;
			}	
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTP_PRT, "vcal: %5d,state: %3d,N: %3d,min_ffo: %ld,max_ffo: %ld,delta_ffo: %ld\n",
     		FLL_Elapse_Sec_Count_Use,
     		vstate,
     		vcal_working.N,
     		(long int)(vcal_working.min_freq),
     		(long int)(vcal_working.max_freq),
     		(long int)(vcal_working.delta_freq)
  			);
  			#endif
  			vcal_working.pacing=0;
  			calfreq(0); //start zero baseline measurement;
			vstate= VCAL_START_BASELINE;
			break;
		case VCAL_START_BASELINE: //baseline zero start cal value
			if(vcal_working.pacing<1) //was 2
			{
				vcal_working.pacing++;
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "vcal: %5d,state: %3d,pacing: %d\n",
     			FLL_Elapse_Sec_Count_Use,
     			vstate,
     			vcal_working.pacing
   				);
   				#endif
 			}
 			else
 			{
				vcal_working.zero_cal_start=getfmeas();
				vcal_working.pacing=0;
				vcal_working.cur_freq=(long int)(vcal_working.min_freq*HUNDREDF);
				calfreq(vcal_working.cur_freq); //prep next measurement
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "vcal: %5d,state: %3d,N: %3d,zero_start: %ld\n",
  		     		FLL_Elapse_Sec_Count_Use,
     				vstate,
     				vcal_working.N,
     				(long int)(vcal_working.zero_cal_start*HUNDREDF)
  				);
  				#endif
  				vstate=VCAL_CYCLE;
  			}
			break;
		case VCAL_CYCLE: //main vcal cycle
			if(vcal_working.cur_freq <= (long int)(vcal_working.max_freq*HUNDREDF))
			{
				
				if(vcal_working.pacing<2) //was 3
				{
					vcal_working.pacing++;
					#if (N0_1SEC_PRINT==0)		
					debug_printf(GEORGE_PTP_PRT, "vcal: %5d,state: %3d,pacing: %d\n",
     				FLL_Elapse_Sec_Count_Use,
     				vstate,
     				vcal_working.pacing
   					);
   					#endif
 				}
 				else
 				{
					vcal_working.vcal_element[vcal_working.index].seg_ffo=getfmeas();
					vcal_working.cur_freq+= (long int)(vcal_working.delta_freq*HUNDREDF);
					calfreq(vcal_working.cur_freq); //prep next measurement
					vcal_working.pacing=1; 
					#if (N0_1SEC_PRINT==0)		
					debug_printf(GEORGE_PTP_PRT, "vcal: %5d,state: %3d,index: %3d,seg_ffo: %ld\n",
  		     			FLL_Elapse_Sec_Count_Use,
     					vstate,
     					vcal_working.index,
     					(long int)(vcal_working.vcal_element[vcal_working.index].seg_ffo*HUNDREDF)
  					);
  					#endif
					vcal_working.index++;
  				}
  				break;
			}
			vcal_working.cur_freq=0;
			calfreq(vcal_working.cur_freq); //prep next measurement
			vcal_working.pacing=0;
			vstate=VCAL_STOP_BASELINE;
			break;
		case VCAL_STOP_BASELINE: //baseline zero end cal value
			if(vcal_working.pacing<1) //was 2
			{
				vcal_working.pacing++;
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "vcal: %5d,state: %3d,pacing: %d\n",
     			FLL_Elapse_Sec_Count_Use,
     			vstate,
     			vcal_working.pacing
   				);
   				#endif
 			}
 			else
 			{
				vcal_working.zero_cal_end=getfmeas();
				vcal_working.pacing=0;
				vcal_working.cur_freq=0;
				calfreq(vcal_working.cur_freq); 
				#if (N0_1SEC_PRINT==0)		
				debug_printf(GEORGE_PTP_PRT, "vcal: %5d,state: %3d,N: %3d,zero_end: %ld curr_freq: %e\n",
  		     		FLL_Elapse_Sec_Count_Use,
     				vstate,
     				vcal_working.N,
     				(long int)(vcal_working.zero_cal_end*HUNDREDF),
               vcal_working.cur_freq
  				);
  				#endif
  				vstate=VCAL_COMPLETE;
  			}
			break;
		case VCAL_COMPLETE: //complete vcal cycle
			// calculate delta compensation for drift
			delta= (vcal_working.zero_cal_end -vcal_working.zero_cal_start)/(double)(vcal_working.N);
			avg = (vcal_working.zero_cal_end +vcal_working.zero_cal_start)/2;
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTP_PRT, "vcal complete: delta: %ld, avg %ld\n",
				(long int)(delta*HUNDREDF),
  				(long int)(avg*HUNDREDF)
			);
			#endif
			// apply tilt compensation and normalize to zero
			for(i=0;i<vcal_working.N;i++)
			{
				vcal_working.vcal_element[i].seg_ffo -= (double)(i)*delta;
				vcal_working.vcal_element[i].seg_ffo -= avg;
			}
			// calculate incremental slopes
			for(i=0;i<(vcal_working.N-1);i++)
			{
				vcal_working.vcal_element[i].seg_inc_slope = 
				vcal_working.vcal_element[i+1].seg_ffo-vcal_working.vcal_element[i].seg_ffo;
				vcal_working.vcal_element[i].seg_inc_slope = vcal_working.vcal_element[i].seg_inc_slope/vcal_working.delta_freq;
				vcal_working.vcal_element[i].done_flag = 1;
			}
			vcal_working.vcal_element[(vcal_working.N-1)].done_flag = 1;
			vcal_working.Varactor_Cal_Flag=0;
			var_cal_flag=0;
			// PRINT VAR COMPENSATION TABLE
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTP_PRT, "vcal: VAR COMP TABLE\n");
			for(i=0;i<(vcal_working.N);i++)
			{
				debug_printf(GEORGE_PTP_PRT, "vcal: indx: %d seg_ffo: %ld inc_ffo %ld flag: %d\n",
				i,
				(long int)(vcal_working.vcal_element[i].seg_ffo*HUNDREDF),
  				(long int)(vcal_working.vcal_element[i].seg_inc_slope*HUNDREDF),
				vcal_working.vcal_element[i].done_flag  				
				);
		
			}
			#endif	
//         Write_Vcal_to_file();
         vcal_complete = 0;       
			break;
		default:
			break;	
		}
}
/*********************************************************************************
// Test code to fill test copy of varactor table
*********************************************************************************/
#if NO_PHY == 0
static int vfill =1;
static VCAL_STRUCT *vp; //pointer to active var table

void fill_var_table(void)
{
	int i;
	if(vfill==1)
	{
			vp = &vcal_working;
			vcal_working.N=16; //Test at 2x speed
			vcal_working.min_freq =-512.0;
			vcal_working.max_freq =+512.0;
			vcal_working.index=0;
			vcal_working.delta_freq = (vcal_working.max_freq-vcal_working.min_freq)/(double)(vcal_working.N);
			for(i=0;i< VAR_TABLE_SIZE; i++)
			{
				vcal_working.vcal_element[i].seg_ffo=(double)(i)*vcal_working.delta_freq -512.0;
				vcal_working.vcal_element[i].seg_inc_slope=1.0;
				vcal_working.vcal_element[i].done_flag=1;
			}	
	}
	else
	{
		vfill=0;
	}			
}	
#endif

/*********************************************************************************
// Routine pre-warps fin based on varator table
*********************************************************************************/
#if 0
double Varactor_Warp_Routine(double fin)
{
	int i;
	double fout,ferr;
	fout=fin; //default
	if(var_cal_flag) //skip if in calibration mode
	{
		return(fout);
	}
//	fill_var_table(); // TEST TEST TEST comment out
	// make sure vp is pointing to correct var_table not test table
	// find candidate segment
	i=0;
	while( i< (vp->N-1))
	{
//		debug_printf(GEORGE_PTP_PRT, "i: %d fin: %e ffo: %e ffo+1: %e\n",
//          i,
//          (float)fin, 
//          (float)vp->vcal_element[i].seg_ffo, 
//          (float)vp->vcal_element[i+1].seg_ffo);

		if((vp->vcal_element[i].seg_ffo < fin) && (vp->vcal_element[i+1].seg_ffo > fin))
		{
//			debug_printf(GEORGE_PTP_PRT, "vwarp: candidate index: %d\n",i);
			break;
		}
		i++;
	}
	if(i>=vp->N)
	{
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTP_PRT, "vwarp: no candidate index found: %d\n",i);
			#endif
			return(fout);
	}
	else //construct warped output
	{
		// start with base segment
		fout= vp->min_freq + (double)(i)*vp->delta_freq;
		ferr= fin- vp->vcal_element[i].seg_ffo; //error from base
		if(vp->vcal_element[i].seg_inc_slope>ZEROF)
		{
			ferr=ferr/vp->vcal_element[i].seg_inc_slope;
		}
		fout += ferr;
		if(fin!=fout)
		{
			#if (N0_1SEC_PRINT==0)		
			debug_printf(GEORGE_PTP_PRT, "vwarp: fin: %ld, fout: %ld \n", (long int)(fin*HUNDREDF),(long int)(fout*HUNDREDF));
			#endif
		}
		return(fout);
	}
}
#endif

#if 0
///////////////////////////////////////////////////////////////////////
// This function provides agile update of the servo loop under low PDV
// noise conditions (such as hop-by-hop boundary clocks)
///////////////////////////////////////////////////////////////////////
static int_8 agile_tmp_cnt;  //debug print pacing for FIFO procession
static void agile_servo_update( )
{
	int i,j,base_f,base_r,cur_index_f,cur_index_r,depth;
	int_32 test_offset_f,test_offset_r,timestamp_align_offset_local;
	/* find minimum candidates */
    for(j=0;j<16;j++) 
    {
		min_forward.sdrift_mins_f[j] = MAXMIN; 
		min_reverse.sdrift_mins_r[j]= MAXMIN; 				 	
    }
	agile_tmp_cnt=0;
	depth=16; //force fixed window of 16 samples
    base_f= (PTP_Fifo_Local[0].fifo_in_index+FIFO_BUF_SIZE)%FIFO_BUF_SIZE;
    base_r= (PTP_Delay_Fifo_Local.fifo_in_index+FIFO_BUF_SIZE)%FIFO_BUF_SIZE;
   	for(j=0;j<depth;j++) 
   	{
   		agile_tmp_cnt++; //debug print related 
	    cur_index_f=(base_f+FIFO_BUF_SIZE-j)%FIFO_BUF_SIZE;
	    cur_index_r=(base_r+FIFO_BUF_SIZE-j)%FIFO_BUF_SIZE;
//		for now always use current offset		
		timestamp_align_offset_local=timestamp_align_offset;			
		if(timestamp_skip_cnt)
		{
			timestamp_skip_cnt--;
			test_offset_f=MAXMIN;
			test_offset_r=MAXMIN;
		}
		else
		{					    
	   		if(SENSE==0)
    		{
	    		test_offset_f=PTP_Fifo_Local[0].fifo_element[cur_index_f].offset;
	    		test_offset_r=PTP_Delay_Fifo_Local.fifo_element[cur_index_r].offset;
	    		if(test_offset_f >= 500000000L)
	    		{
	    			 test_offset_f= 1000000000L - test_offset_f;
	    		}
	    		else if (test_offset_f <= -500000000L)
	    		{
	    			 test_offset_f= 1000000000L + test_offset_f;
	    		}
	    		if(test_offset_r >= 500000000L)
	    		{
	    			 test_offset_r= 1000000000L - test_offset_r;
	    		}
	    		else if (test_offset_r <= -500000000L)
	    		{
	    			 test_offset_r= 1000000000L + test_offset_r;
	    		}	 
				test_offset_f -= (Initial_Offset+ timestamp_align_offset_local);
				test_offset_r -= (Initial_Offset_Reverse - timestamp_align_offset_local);
			}	
			else
			{
 	    		test_offset_f=-PTP_Fifo_Local[0].fifo_element[cur_index_f].offset;
	    		test_offset_r=-PTP_Delay_Fifo_Local.fifo_element[cur_index_r].offset;
	    		if(test_offset_f >= 500000000L)
	    		{
	    			 test_offset_f= 1000000000L - test_offset_f;
	    		}
	    		else if (test_offset_f <= -500000000L)
	    		{
	    			 test_offset_f= 1000000000L + test_offset_f;
	    		}
	    		if(test_offset_r >= 500000000L)
	    		{
	    			 test_offset_r= 1000000000L - test_offset_r;
	    		}
	    		else if (test_offset_r <= -500000000L)
	    		{
	    			 test_offset_r= 1000000000L + test_offset_r;
	    		}	 
				test_offset_f += (Initial_Offset+timestamp_align_offset_local);
				test_offset_r += (Initial_Offset_Reverse-timestamp_align_offset_local);
   			}
   		}  //end don't timestamp skip case
		if(test_offset_f<min_forward.sdrift_mins_f[j]) min_forward.sdrift_mins_f[j] =  test_offset_f;
		if(test_offset_r<min_reverse.sdrift_mins_r[j]) min_reverse.sdrift_mins_r[j] =  test_offset_r;
   	} //end first j-depth loop
	/* find minimum */
	/*start with min of mins*/
	min_forward.sdrift_floor_f= MAXMIN;
	min_reverse.sdrift_floor_r= MAXMIN;
    for(j=0;j<depth;j++) 
    {
  		if(min_forward.sdrift_mins_f[j]<min_forward.sdrift_floor_f) min_forward.sdrift_floor_f =  min_forward.sdrift_mins_f[j];
  		if(min_reverse.sdrift_mins_r[j]<min_reverse.sdrift_floor_r) min_reverse.sdrift_floor_r =  min_reverse.sdrift_mins_r[j];
    }
	//Simple clustering: add clustering algorithm to select good min be sure to throw out
	//any min of mins that are too early
	min_forward.sdrift_min_f_sum=0;
	min_reverse.sdrift_min_r_sum=0; 		
	min_forward.min_count_f=0;
	min_reverse.min_count_r=0; 		
    for(j=0;j<depth;j++) 
   	{
    		test_offset_f=min_forward.sdrift_mins_f[j]-min_forward.sdrift_floor_f;
			test_offset_r=min_reverse.sdrift_mins_r[j]-min_reverse.sdrift_floor_r;
  	  		 if((test_offset_f)< min_oper_thres_f)	
   			 {
				 min_forward.sdrift_min_f_sum +=  min_forward.sdrift_mins_f[j];
				 min_forward.min_count_f++;
    		 }
 	 	   	 if((test_offset_r)< min_oper_thres_r)	
   			 {
 				 min_reverse.sdrift_min_r_sum +=  min_reverse.sdrift_mins_r[j];
				 min_reverse.min_count_r++;
    		 }
    }
   	if(min_forward.min_count_f>4)//GPZ only update  if threshold is met
    {
		min_forward.sdrift_min_f= min_forward.sdrift_min_f_sum/min_forward.min_count_f;	
   	}
   	else if(!start_in_progress)
   	{
   		min_forward.sdrift_min_f=MAXMIN;
//			debug_printf(GEORGE_PTP_PRT, "exception: forward low min density:%d\n",min->min_count_1024);
   	}
   	if(min_reverse.min_count_r>4)//GPZ only update  if threshold is met
   	{
		min_reverse.sdrift_min_r= min_reverse.sdrift_min_r_sum/min_reverse.min_count_r;	
   	}
   	else if(!start_in_progress)
   	{
   		min_reverse.sdrift_min_f=MAXMIN;
//			debug_printf(GEORGE_PTP_PRT, "exception: forward low min density:%d\n",min->min_count_1024);
   	}
// TODO complete agile control servo calculate bias corrected time error and PID     	
  
}
#endif
/*
----------------------------------------------------------------------------
                                SC_GetVcalData

This function will return a varactor calibration values.  The calibration 
is done in 16 steps, from +512 ppb to -512 ppb.  The results of the 
calibration is 16 offset values of the actual measured frequency offset.

Parameters:
OUT:
   FLOAT64 *df_segFfo
   This is a pointer to a data array that this function will load.  Currently,
   the array is 16 elements in size.  The data stored in this array is the
   frequency offset measured.

Return value:
	 0 - data is based on a completed varactor calibration test.
   -1 - data is not based on a completed varactor calibration test.

-----------------------------------------------------------------------------
*/ 
#if NO_PHY == 0
int SC_GetVcalData(FLOAT64 *pdf_segFfo)
{
  UINT32 i;

  for(i=0;i<(vcal_working.N);i++)
  {
     *pdf_segFfo = vcal_working.vcal_element[i].seg_ffo;
      pdf_segFfo++;
  }			  

  return vcal_complete;
}
#endif


#ifndef TP500_BUILD //sqrt implementation

#if defined(BRCM_Katana)

/*
 * Katana has a problem with floating point and needs special sqrt function
 *
 */

double sqrt_local(double f64_value)
{
  const double kEpsilon = 1e-2;
  const INT64 kMagicNumber = 0x5fe6eb50c7aa19f9LLU;
  const int kNewtonIterations = 5;

  if(f64_value <= kEpsilon) //skip negatives and very small numbers
  {
    return 0;
  }
  else
  {
    // note: this is assuming the current Katana 'double' representation
    uint32_t *repr = (uint32_t*)&f64_value;
    uint64_t full = ((uint64_t)(repr[0]) << 32) | repr[1];
    uint64_t manip_full = kMagicNumber - (full >> 1);
    double guess;
    uint32_t *guess_repr = (uint32_t*)&guess;
    guess_repr[0] = (manip_full >> 32);
    guess_repr[1] = (uint32_t)manip_full;

    //with the guess iterate with the Newton-Raphson method
    double x = guess;
    double z = 0.5 * f64_value;
    int i;
    for (i=0; i < kNewtonIterations; ++i)
    {
      x = (1.5 * x) - (x*x) * (x*z);
    }
    //debug_printf(UNMASK_PRT, "sqrt_local (katana) i,o=%g, %g\n", f64_value, x * f64_value);

    return x * f64_value;
  }
}

#else //defined(BRCM_Katana)


/*
----------------------------------------------------------------------------
      double InvSqrt(double x)
Quake method for O(1) inverse square root APPROXIMATION

WARNING: Requires that floating point is implementation is IEEE-754

use the magic number 0x5fe6eb50c7aa19f9 for 64 bits and 0x5f3759df for 32 bits.
----------------------------------------------------------------------------
*/
static inline double InvSqrt(double x)
{
  const INT64 kMagicNumber = 0x5fe6eb50c7aa19f9LLU; //use 0x5f3759df for 32 bits
  union {
    double f;
    long long i;
  } tmp;
  double y;
  
  if(sizeof(tmp.f) == sizeof(tmp.i)) //sanity check, should be an assert()
  {
    tmp.f = x;
    tmp.i = kMagicNumber - (tmp.i >> 1);
    y = tmp.f;
    return y * (1.5 - 0.5 * x * y * y);
  }
  else //this should not happen
  {
    debug_printf(UNMASK_PRT, "InvSqrt implementation error at %s line %d\n", __FILE__, __LINE__);
    return 0;
  }
}

/*
----------------------------------------------------------------------------
      double sqrt_local(double x)
Approximation for sqrt using  InvSqrt(double x)
----------------------------------------------------------------------------
*/

double sqrt_local(double f64_value)
{
  const double kEpsilon = 1e-2; //skip negatives and very small numbers
  
  if(f64_value <= kEpsilon)
    return 0;
  else
  {
    double result = InvSqrt(1.0/f64_value);
    
    //debug_printf(UNMASK_PRT, "sqrt_local i,o=%lg, %lg\n", f64_value, (result +(f64_value/result))/2.0);
    
    //improve the approximation by one iteration of Newton's method
    return (result +(f64_value/result))/2.0;
  }
}

#endif //(CPU == BRCM-Katana)

#endif //TP500_BUILD


UINT64 GetTime(void)
{
   t_ptpTimeStampType  s_sysTime;
   INT16 d_utcOffset;

   SC_SystemTime(&s_sysTime, &d_utcOffset, e_PARAM_GET);

   return ((UINT64)s_sysTime.u48_sec * 1000000000LL) + (UINT64)s_sysTime.dw_nsec;   
}

/*
----------------------------------------------------------------------------
                                SC_FrequencyCorrection_Local

This function is a wrapper for SC_FrequencyCorrection.

It checks the debug flags and if necessary prints the correction value and then
calls SC_FrequencyCorrection

Inputs
	FLOAT64 *pff_freqCorr
	On a set, this pointer defines the the current frequency correction to 
	be applied to the local clock.  The value is in units of ppb.

	t_paramOperEnum b_operation  
	This enumeration defines the operation of the function to either set or 
	get.
		e_PARAM_GET - Get fractional frequency offset
		e_PARAM_SET - Set fractional frequency offset
Outputs:
	FLOAT64 *pff_freqCorr
	On a get, the current frequency correction is written to this pointer 
	location.

Return value:
	 0: function succeeded
	-1: function failed
-----------------------------------------------------------------------------
*/
static int SC_FrequencyCorrection_Local(FLOAT64 *pff_freqCorr, t_paramOperEnum b_operation)
{
#undef SC_FrequencyCorrection
  int retValue = 0;
  FLOAT64 correctionValue;
  static BOOLEAN excessiveCorrectionTrip = FALSE;
  
  switch(b_operation)
  {
    case e_PARAM_SET:
      if(gSC_TST_FREQ_ZERO)
    	correctionValue = 0.0;
      else
    	correctionValue = *pff_freqCorr;

      debug_printf(CLK_COR_PRT, "SC_FrequencyCorrection: %g%s\n", *pff_freqCorr, gSC_TST_FREQ_ZERO?" (ignored)":"");

      //generate an alarm if the frequency corrections are too big for the selected oscillator type
      if(fabs(*pff_freqCorr) > kOscOffsetLimit[OCXO_Type()])
      {
        if(excessiveCorrectionTrip == FALSE)
        {
          SC_Event(e_EXCESSIVE_FREQUENCY_CORR, e_EVENT_WARN, e_EVENT_SET, sc_alarm_lst[e_EXCESSIVE_FREQUENCY_CORR].dsc_str, 0);
          excessiveCorrectionTrip = TRUE;
        }
      }
      else
      {
        if(excessiveCorrectionTrip == TRUE)
        {
          SC_Event(e_EXCESSIVE_FREQUENCY_CORR, e_EVENT_WARN, e_EVENT_CLEAR, sc_alarm_lst[e_EXCESSIVE_FREQUENCY_CORR].dsc_str, 0);
          excessiveCorrectionTrip = FALSE;
        }
      }

      retValue = SC_FrequencyCorrection(&correctionValue, e_PARAM_SET);
    break;

    case e_PARAM_GET:
      retValue = SC_FrequencyCorrection(pff_freqCorr, e_PARAM_GET);
    break;
   default:
    break;
  }
  
  if(retValue != 0)
  {
    debug_printf(UNMASK_PRT, "Err %d: SC_FrequencyCorrection()\n", retValue);
  }
  
  return retValue;

#define SC_FrequencyCorrection(x, y) SC_FrequencyCorrection_Local((x), (y))
}

/*
----------------------------------------------------------------------------
                                SC_PhaseOffset_Local()

Description:
This function is a wrapper for SC_PhaseOffset.

It checks the debug flags and if necessary prints the correction value and then
calls SC_PhaseOffset

Parameters:

Inputs
	INT64 *pll_ phaseCorrection
	On a set, this pointer defines the the current phase correction to
	be applied to the local clock.  The value is in units of nanoseconds.

	t_paramOperEnum b_operation  
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get current phase correction
		e_PARAM_SET - Set phase correction

Outputs:
	INT64 *pll_ phaseCorrection
	On a get, the current phase correction is written to this pointer
	location.

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
static int SC_PhaseOffset_Local(INT64 *pll_phaseOffset, t_paramOperEnum b_operation)
{
#undef SC_PhaseOffset
int retValue;
static INT64 lastPhaseOffset = 0;
  
  if(b_operation == e_PARAM_SET)
  {
    debug_printf(CLK_COR_PRT, "SC_PhaseOffset: %lld, phase delta: %lld\n", *pll_phaseOffset, lastPhaseOffset - *pll_phaseOffset);
    lastPhaseOffset = *pll_phaseOffset;
  }
  
  retValue = SC_PhaseOffset(pll_phaseOffset, b_operation);
	if(retValue)
  {
    debug_printf(UNMASK_PRT, "Err %d: SC_PhaseOffset()\n", retValue);
  }
  
  return retValue;
#define SC_PhaseOffset(x, y) SC_PhaseOffset_Local((x), (y))
}


static void init_holdoff(void)
{
   int i;

   for(i=0;i<NUM_OF_SERVO_CHAN;i++)
   {
      reference_holdoff_counter[i] = 0;
      prev_chan_valid[i] = FALSE;
   }
}


static void Update_Reference_Holdoff(void)
{
//   int i;

/** holdoff PTP until PTP is primed */
//   if(Is_PTP_Warm() || 
//      (state == PTP_SET_PHASE))
//   {
     s_servoLocalChanAccept.b_accepted[RM_GM_1] = TRUE;
//   }
//   else
//   {
//     s_servoLocalChanAccept.b_accepted[RM_GM_1] = FALSE;
//   }
   s_servoLocalChanAccept.b_accepted[RM_GPS_1] = TRUE;
//jiwwang
   s_servoLocalChanAccept.b_accepted[RM_GPS_2] = TRUE;
   s_servoLocalChanAccept.b_accepted[RM_GPS_3] = TRUE;
   s_servoLocalChanAccept.b_accepted[RM_SE_1] = TRUE;
   s_servoLocalChanAccept.b_accepted[RM_SE_2] = TRUE;
   s_servoLocalChanAccept.b_accepted[RM_RED] = TRUE;


#if 0
/* update holdoff counters (count down one) */
   for(i=0;i<NUM_OF_SERVO_CHAN;i++)
   {
/* check to see if any references became valid and set holdoff
* counter to 180 if not in start up */
      if(ChanSelectTable.s_servoChanSelect[i].o_valid && !prev_chan_valid[i])
      {
         if((state != PTP_START) && (state != PTP_SET_PHASE))
            reference_holdoff_counter[i] = 180;
      }      
      prev_chan_valid[i] = ChanSelectTable.s_servoChanSelect[i].o_valid;    

      if(reference_holdoff_counter[i])
      {
         reference_holdoff_counter[i]--;
       	debug_printf(GEORGE_PTP_PRT, "chan %d: holdoff_counter= %d\n", i, reference_holdoff_counter[i]);
         s_servoLocalChanAccept.b_accepted[i] = FALSE;
      }
   }
#endif
/* send holdoff to sc_chan */

   return;
}

/*
----------------------------------------------------------------------------
                                Get_servoAccept()

Description:


Parameters:

Inputs

Outputs:

Return value:

-----------------------------------------------------------------------------
*/
#define REPORT_FREQ_OFFSET_THRESHOLD 30.0 /* 30 ppb offset */
#define REPORT_FREQ_HYST     8.0
static int report_freq_state[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int Get_servoAccept(t_servoChanAcceptType *p_servoChanAccept)
{
   int chan;
   double foffset;

#if 1
   for(chan=0;chan<RM_GPS_1;chan++)
   {  
//       	debug_printf(GEORGE_PTP_PRT, "PTP_ACCEPT: chan: %d Nmax: %ld Elapse_PTP: %ld Elapse: %ld\n"
//         ,chan, 
//         (long int)RFE.Nmax,
//         (long int)FLL_Elapse_Sec_PTP_Count,
//         (long int)FLL_Elapse_Sec_Count_Use );

      if(((FLL_Elapse_Sec_PTP_Count < 1800) || (RFE.Nmax < 150)) && (FLL_Elapse_Sec_Count_Use > 7200))
      {
         s_servoLocalChanAccept.b_accepted[chan] = FALSE;
      }
      else
      {
         s_servoLocalChanAccept.b_accepted[chan] = TRUE;
      }   

   }    
#endif
#if(NO_PHY==0)

   for(chan=RM_GPS_1;chan<RM_NONE;chan++)
   {      
      foffset = report_freq_offset(chan);
//      foffset = 0.0;
//      printf("report freq offset chan %d, offset %e\n", chan, foffset);

      if((foffset > REPORT_FREQ_OFFSET_THRESHOLD) && (report_freq_state[chan]==FALSE))
      {
//         s_servoLocalChanAccept.b_accepted[chan] = FALSE;
         report_freq_state[chan]=TRUE;
//       	debug_printf(GEORGE_PTP_PRT, "SERVOPM + freq: chan: %d offset %le Nmax: %ld Elapse_PTP: %ld Elapse: %ld\n",
//         chan,
//         foffset,
//         (long int)RFE.Nmax,
//         (long int)FLL_Elapse_Sec_PTP_Count,
//         (long int)FLL_Elapse_Sec_Count_Use );
		printf("%s: foffset %f...\n", __FUNCTION__, foffset);

      }
      else if((foffset < REPORT_FREQ_OFFSET_THRESHOLD - REPORT_FREQ_HYST)&& (report_freq_state[chan]==TRUE))
      {
//         s_servoLocalChanAccept.b_accepted[chan] = TRUE;
         report_freq_state[chan]=FALSE;
//       	debug_printf(GEORGE_PTP_PRT, "SERVOPM - freq: chan: %d offset %le Nmax: %ld Elapse_PTP: %ld Elapse: %ld\n",
//         chan,
//         foffset, 
//         (long int)RFE.Nmax,
//         (long int)FLL_Elapse_Sec_PTP_Count,
//         (long int)FLL_Elapse_Sec_Count_Use );

      }  
      if( (Phy_Channel_Transient_Free_Cnt[chan-RM_GPS_1 ] < 38) && (FLL_Elapse_Sec_Count > 1800) )  //was 38 increase to 75
      {
         s_servoLocalChanAccept.b_accepted[chan]= FALSE;
//       	debug_printf(GEORGE_PTP_PRT, "SERVOPM - transient free: chan: %d offset %le Nmax: %ld transient %d\n",
//         chan,
//         foffset,
//         (long int)RFE_PHY.Nmax, 
//         (int)Phy_Channel_Transient_Free_Cnt[chan-RM_GPS_1]);
		if(chan == RM_GPS_1)
			printf("%s: #%d Phy_Channel_Transient_Free_Cnt %d, FLL_Elapse_Sec_Count %d..\n", __FUNCTION__, chan, Phy_Channel_Transient_Free_Cnt[chan-RM_GPS_1 ], FLL_Elapse_Sec_Count);
      }
      else
      {
         s_servoLocalChanAccept.b_accepted[chan]=  !report_freq_state[chan]; 
      }

		/* add by jiwang for DianKeYuan testing to accept all valid inputs */
//		s_servoLocalChanAccept.b_accepted[chan] = TRUE;	
		/* end of jiwang adding */
   }
#endif

   *p_servoChanAccept = s_servoLocalChanAccept;
	return 0;
}

/* this function will jam the 1PPS phase threshold if possible
* Return value:
    0 - successful
   -1 - function failed
   -2 - function failed (no time reference)
   -3 - function failed (no jam necessary)
*/
int Jam_Now(void)
{
/* jam only if there is a valid time source */
   if(rms.time_source[0] == RM_NONE)
      return  -2;

/* jam only when needed */
   if(!jam_needed)
      return -3;

   jam_now_flag = TRUE;
   return 0;
}

/* this function will clear the jam the 1PPS phase threshold if possible, will return 0 if sucessful or -1 if failed */
static int Jam_Clear(void)
{
   jam_now_flag = FALSE;
   return 0;
}

/* this function will jam the 1PPS phase threshold if possible, will return 0 if sucessful or -1 if failed */
int Jam_Never(BOOLEAN never_jam_enable)
{
   never_jam_flag = never_jam_enable;
   return 0;
}


/* returns jam threshold in units of ns */
UINT32 Get_Jam_Threshold(void)
{
   SC_SyncPhaseThreshold(e_PARAM_GET, &jam_threshold);

   return jam_threshold;   
}

int Is_Jam_Needed(void)
{
   return jam_needed;
}
