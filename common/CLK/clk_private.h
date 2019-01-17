
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

FILE NAME    : clk.h

AUTHOR       : George Zampetti

DESCRIPTION  : 

High level description here..,

Revision control header:
$Id: CLK/clk_private.h 1.35 2012/04/23 15:22:22PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/


/* Prevent file from being included multiple times. */
#ifndef H_clk_h
#define H_clk_h


/*****************************************************************************/
/*                       ***INCLUDES Definitions***                          */
/*  Nested includes should not be used as a general practice.  If there is a */
/*  good reason to include an include within an include, do so here. 	     */
/*****************************************************************************/
#include "datatypes.h"
#include "proj_inc.h"
#include "sc_servo_api.h"

/*****************************************************************************/
/*                       ***CONSTANT Definitions***                          */
/*  This section should be used to define constants (#defines) and macros    */
/*  that can be used by any module that includes this file.                  */
/*****************************************************************************/
#define CROSSOVER_TS_GLITCH 0 // flag used to add glitch detection on TimeStamp input flow

//#define Send_Log(x...) 
#define NV_BRIDGE_TIME 120
#define k_STORED_FREQ_TIME_THRES  7200
////////////////// defines
#define AUTONEG_ON
#define START_OFFSET 		(5000)
// #define DRIFT_BUF_SIZE 		(1024)
#define MIN_CLUSTER_1 		(1000) //was 2000
#define MIN_CLUSTER_2 		(10000)//10000 
#define MIN_CLUSTER_3 		(20000)//20000 
#define MIN_CLUSTER_4 		(50000)//50000 
//#if DSL
//#define STEP_THRES 			(2000000)
//#else
//#define STEP_THRES 			(11500)
//#endif
#define OFFSET_THRES 		(100)
#define POP_THRES 			(1500)
#define POP_THRES_RET 		(1400)
//#define SACC_THRES 			(50000)
#define MAX_DELTA_POP		(20000) //was 5000 then 10000 now 20K to address real world congestion
#define MIN_DELTA_POP		(1000)  //was 100 then 5000 back down to 1000
#define STICKY_THRES		(64)
#define CLUSTER_INC			(100) //was 30
#define CONV_FACTOR 		(1.0/2.91038E-11) /* 2^-32/8 */
#define DEFAULT_SYNC_RATE		64
#define DEFAULT_DELAY_REQ_RATE	64
#define SLEW_COUNT				48 //was 32
#define ONE_SEC_IN_MS			1000
#define MAXMIN				(1000000000)
#define HUNDREDF (100.0)
#define THOUSANDF (1000.0)
#define MILLIONF (1000000.0)
#define PER_90 (0.90)
#define PER_10 (0.10)
#define PER_95 (0.95)
#define PER_05 (0.05)
#define PER_99 (0.99)
#define PER_01 (0.01)
#define FORWARD	(0)
#define REVERSE	(1)
#define BOTH	(2)
#define SENSE 1 //change sense of packet measurement USE 1
#define FREEZE 0// force constant frequency
#define FREEZE_VERIFY 0// force constant frequency
#define FREEZE_MOD 0 // force 2ppb square wave with 512s period
// GPZ USE MCOMPRESS TO VALIDATE Tellabs compensation before full integration
// To Integrate need a new TDEV at 63 seconds to compare floor to mode_ceiling and
// only use compression if ceiling is "stable"
#define MCOMPRESS 0//if 1 compensates for mode compression effected on min floor
#define M_COMPRESS_MAX 50000
#define M_COMPRESS_MIN 10000

#define MIN_FIND  1 //use case where operational cluster for min gets very low 
#define G1_OCXO (double)(0.01667*0.0167)		//(0.01667*0.025 )0.1=600sec 60*6 360 sec proportional steady state was (0.01667*0.167)
#define G2_OCXO  (double)(1.85185e-5*0.0167*0.0167)  //900*6 5400 sec integral steady state (1.85185e-5*0.167*0.167)
#define G1_MOCXO (double)(0.01667*0.5)		//60*2 120 sec proportional steady state was 0.25 base
#define G2_MOCXO  (double)(1.85185e-5*0.5*0.5)  //900*2 1800 sec integral steady state was 0.25

#define	GEAR_OCXO (double)(0.999654)
#define	GEAR_OCXO_SLOW (double)(0.99995)
#define	GEAR_OCXO_FAST (double)(0.9986)
#define FLLSTART_TCXO 10
#define FLLSTART_OCXO 120
#define FLL_SETTLE 128 //16 to 64 Sept 2009
#define PTP_LOS_TIMEOUT 12
#define RFE_CHAN_MAX 6 // GPZ Jan 2011 support four physical layer channels increase to 5 for redundancy
//#if ENTERPRISE
//#define SACC_THRES (50000)
//#define BIAS_COMP (double)(1.0) //was 1.0
//#define DIFF_CHAN_ERR (5000.0)
//#define DETILT_TIME 360
//#define RMSIZE 64
//#define MIN_OPER_THRES (1000)
//#define MODE_WIDTH_MAX (300000)
//#define MIN_DENSITY_THRES (2)
//#elif DSL
//#define SACC_THRES (500000)
//#define BIAS_COMP (double)(1.0) //was 1.0
//#define DIFF_CHAN_ERR (500000.0)
//#define DETILT_TIME 2048
//#define RMSIZE 128
//#define MIN_OPER_THRES (20000)
//#define MODE_WIDTH_MAX (900000)
//#define MIN_DENSITY_THRES (4)
//#else
//#define SACC_THRES (50000)
//#define BIAS_COMP (double)(1.0) //was 0.5
//#define DIFF_CHAN_ERR 30000.0 //was 10000
//#define DETILT_TIME 1024
//#define RMSIZE 64
//#define MIN_OPER_THRES (1000)
//#define MODE_WIDTH_MAX (300000)
//#define MIN_DENSITY_THRES (4)
//#endif
// DEFINTIONS Supporting operational offset floor extension
#define F_OFW_SIZE 7500 //was 5000 Forward offset floor window size was 10000 trim to 7500 March 2011
#define F_OFW_STEP 5000 //was 2500 Forward offset floor window coarse search step use CLUSTER_INC for fine
#define F_OFW_ITR  8    //was 8 Iterations at each step
#define RFE_WINDOW_MAX (32)
#define RFE_WINDOW_LARGE (300)
// definitions supporting TDEV calculation
#define TDEV_WINDOWS (1)
#define TDEV_CAT 0 // critical TDEV category
// Input Processing Definition
#define IPP_CHAN 2 //Input process channels 2 for now may be quad 
#define IPP_CHAN_PHY 7 //Input process channels 2 for now may be quad 
#define PHY_WINDOW 32  // Moving Window Depth to process input FIFO data
#define PHY_SKIP 32  // Default skip RFE PHY Count

#if (NO_PHY==0)
#define MAX_RM_SIZE   1024
#else
#define MAX_RM_SIZE    128
#endif

/*****************************************************************************/
/*                       ***Data Type Specifications***                      */
/*  This section should be used to specify data types including structures,  */
/*  enumerations, unions, and redefining individual data types.              */
/*****************************************************************************/
typedef enum
{
    XFER_IDLE      = 0,
	XFER_START,
	XFER_END
} 	XFER_State;

typedef enum
{
    Lock_Off      = 0,
	Lock_Green_Blink,
	Lock_Green_On,
	Lock_Yellow_On,
	Lock_Yellow_Blink
} Lock_State;
typedef enum
{
	FLL_UNKNOWN	=0,		
    FLL_WARMUP,
	FLL_FAST,	// Detilt or Fast Gear
	FLL_NORMAL, //Clock Normal or Bridge
	FLL_BRIDGE, //Clock Normal with slew counts
	FLL_HOLD,
	FLL_STABILIZE
} Fll_Clock_State;

typedef struct
{
	Fll_Clock_State cur_state;
	Fll_Clock_State prev_state;
	uint_32 current_state_dur;
	uint_32 previous_state_dur;
	Lock_State lock;
	double f_avg_tp15min, f_avg_tp60min; //rate of transients per 15 minute and per hour
	double r_avg_tp15min, r_avg_tp60min; 
	double f_oper_mtdev,r_oper_mtdev;
	double f_oper_mode_tdev,r_oper_mode_tdev;
	double f_mode_width, r_mode_width;
	double f_min_cluster_width, r_min_cluster_width;
	double out_tdev_est,out_mdev_est;
	double f_cdensity, r_cdensity;
	double f_weight, r_weight;
	double freq_cor, time_cor,residual_time_cor;
	double fll_min_time_disp;
	double rtd_us; //minimal rtd in microseconds
	double t_max;
	double t_min;
	double t_avg;
	double t_std_5min;
	double t_std_60min; 
	double f_mafie, r_mafie; //15 minute operational mafie ppb 
	double f_cw_avg, r_cw_avg;	
	double f_cw_std, r_cw_std;	
} FLL_STATUS_STRUCT;

typedef struct 
{
	int_32	sdrift_min_cal,sdrift_min_1024;
}
 MIN_STRUCT;
 // Input Histogram Data Processing templates
 typedef struct
 {
 	int_32 floor_cluster_count; //count over phase interval of samples in floor cluster
 	int_32 ofw_cluster_count;	//count over phase interval of samples in ceiling cluster
 	
 	long long floor_cluster_sum;
 	long long ofw_cluster_sum;
 	
 	int_32 floor_set[8];  //minimum in one  interval use to replace working phase floor
 	int_32 ceiling;//maximum in one  interval
  } 
  INPROCESS_CHANNEL_STRUCT;
  
  typedef struct
  {
  	int_32 phase_floor_avg; // cluster phase floor average
  	int_32 phase_ceiling_avg; //cluster phase ceiling average
  	int_32 working_phase_floor; //baseline phase_floor 
  	INPROCESS_CHANNEL_STRUCT IPC; 
  }
  INPROCESS_PHASE_STRUCT;
  typedef struct
  {
  	int_16 phase_index; 
  	INPROCESS_PHASE_STRUCT PA[IPP_CHAN],PB[IPP_CHAN],PC[IPP_CHAN],PD[IPP_CHAN]; //one structure for each channel
  	INPROCESS_PHASE_STRUCT PE[IPP_CHAN],PF[IPP_CHAN],PG[IPP_CHAN],PH[IPP_CHAN]; //one structure for each channel
  	//outputs from process//
  	int_32 working_phase_floor[IPP_CHAN]; // current working phase floor 
  	int_32 phase_floor_avg[IPP_CHAN]; // current phase floor average
  	int_32 ofw_cluster_count[IPP_CHAN]; // cluster count
  	int_32 floor_cluster_count[IPP_CHAN]; // cluster count
  	int_32 phase_ceiling_avg[IPP_CHAN]; // current phase floor average
  }
  INPUT_PROCESS_DATA;
  
 typedef struct
 {
 	int_32 floor_cluster_count; //count over phase interval of samples in floor cluster
 	int_32 ofw_cluster_count;	//count over phase interval of samples in ceiling cluster
 	
 	long long floor_cluster_sum;
 	long long ofw_cluster_sum;
 	
 	int_32 floor;  //minimum in one  interval use to replace working phase floor
 	int_32 ceiling;//maximum in one  interval
  } 
  INPROCESS_CHANNEL_STRUCT_PHY;

  typedef struct
  {
  	int_32 phase_floor_avg; // cluster phase floor average
  	int_32 phase_ceiling_avg; //cluster phase ceiling average
  	int_32 working_phase_floor; //baseline phase_floor 
  	INPROCESS_CHANNEL_STRUCT_PHY IPC_PHY; 
  }
  INPROCESS_PHASE_STRUCT_PHY;

  typedef struct
  {
  	int_16 phase_index; 
  	INPROCESS_PHASE_STRUCT_PHY PA[IPP_CHAN_PHY],PB[IPP_CHAN_PHY],PC[IPP_CHAN_PHY],PD[IPP_CHAN_PHY]; //one structure for each channel
  	INPROCESS_PHASE_STRUCT_PHY PE[IPP_CHAN_PHY],PF[IPP_CHAN_PHY],PG[IPP_CHAN_PHY],PH[IPP_CHAN_PHY]; //one structure for each channel
  	//outputs from process//
  	int_32 working_phase_floor[IPP_CHAN_PHY]; // current working phase floor 
  	int_32 phase_floor_avg[IPP_CHAN_PHY]; // current phase floor average
  	int_32 ofw_cluster_count[IPP_CHAN_PHY]; // cluster count
  	int_32 floor_cluster_count[IPP_CHAN_PHY]; // cluster count
  }
  INPUT_PROCESS_DATA_PHY;

// April 2010 New Robust Frequency Estimation Data Elements
// This is part of a brand new way to regress frequency estimates
// It is based like for like comparison across parallel windows
// TODO this is the way to make DSL Robust 
typedef struct 
{
	double phase;  // uncompenstated phase error estimate ns use sacc_f
	double bias;   // bias estimate ns use min_oper_thres as a full range source of this
	double noise;  // noise estimate ns^2
	int_16 tfs;    // transient free seconds state   use delta_[forward]_slew_cnt
	int_16 floor_cat; // indicated common floor index
}
 RFE_ELEMENT_STRUCT;
typedef struct 
{
	int_16 N,Nmax;
	int_16 index;
	int_8 first;
	double alpha; //oscillator dependent smoothing gain across update default 1/60
	double dfactor; // additional de weighting factor based on total weight 0.1 to 1
	double wavg;  //running average weight using alpha factor
	double weight_for, weight_rev;  //Ken_Post_Merge
	double fest; // frequency estimate in ppb
	double fest_prev; //spaced N back use to check initial stabilization
	double fest_for,fest_rev; // frequency estimate in ppb Ken_Post_Merge
	double fest_cur;
	double delta_freq;
	double smooth_delta_freq;
	double res_freq_err;
	RFE_ELEMENT_STRUCT rfee_f[RFE_WINDOW_MAX];
	RFE_ELEMENT_STRUCT rfee_r[RFE_WINDOW_MAX];
} RFE_STRUCT;

// End Robust Frequency Estimation Data
typedef struct
{
	XFER_State xstate;
	int_8 working_sec_cnt;
	int_8 duration;
	int_8 idle_cnt;
	// Protected RFE In Data
	double xfer_sacc_f, xfer_sacc_r;
	double xfer_var_est_f, xfer_var_est_r;
	double xfer_in_weight_f, xfer_in_weight_r;
	// Protected RFE Out Data
	double xfer_fest, xfer_fest_f, xfer_fest_r;
	double xfer_weight_f, xfer_weight_r;
	int_8	xfer_first;
	int_16  xfer_Nmax;
	// protected FLL status Data
	double xfer_fdrift, xfer_fshape, xfer_tshape;
	double xfer_pshape_smooth;
	double xfer_scw_avg_f, xfer_scw_avg_r;
	double xfer_scw_e2_f,xfer_scw_e2_r;
	double xfer_turbo_phase[2];
	double xfer_turbo_phase_phy[RFE_CHAN_MAX]; //GPZ increase to 4 channels JAN 2011
   int_16 xfer_active_chan_phy; //current selected active phy channel GPZ phy_channelize Dec 2011
   int_16 xfer_active_chan_phy_prev; //current selected active phy channel GPZ phy_channelize Dec 2011
}
XFER_DATA_STRUCT;
#if(!NO_IPPM)				

// IPPM Data Structure
typedef struct 
{
	int_32 ipdv_max;
	int_32 under_thres_cnt;
	int_32 total_thres_cnt;
	int_32 over_thres_99_9_cnt;
	int_32 thres_99_9;
	int_32 jitter_sum;
	int_32 jitter_samp;
	int_16 jitter_cnt;
}
 IPPM_ELEMENT_STRUCT;
typedef struct 
{
	double jitter;
	double ipdv;
	double ipdv_99_9;
	double thres_prob;
}
 IPPM_OUTPUT_STRUCT;
typedef struct 
{
	double cur_jitter_1sec, cur_jitter_1min, out_jitter;
	double cur_ipdv_1sec, cur_ipdv_1min, out_ipdv;
	double cur_ipdv_99_9_1sec, cur_ipdv_99_9_1min,   out_ipdv_99_9;
	double cur_thres_prob_1sec, cur_thres_prob_1min, out_thres_prob;
	int_16 ies_head, ios_head; //index to head of queue
	int_16 pacing; //pacing skip factor for jitter calculation
	int_32 thres;  // threshold for accepting delay sample (ns)
	int_32 slew_gain; //attack rate for 99.9 IPDV 
	int_16 slew_gap;  // consecutive samples with under slew
	int_16 window; // observation window in minutes
	int_16 N_valid; // number of valid minute stats entries
	IPPM_ELEMENT_STRUCT ies[256]; //ippm stat circular queue one second process	
	IPPM_OUTPUT_STRUCT  ios[256]; //256 minute circular minute queue
}
IPPM_STRUCT;
#endif
typedef struct 
{
	double in_f,in_r;
	double cur_avg_f;  //note update current before calling calculator
    double cur_avg_r;
	double prev_avg_f;  //note update current before calling calculator
    double prev_avg_r;
    double ztie_est_f;
	double ztie_est_r;
 }
 ZTIE_ELEMENT_STRUCT;

typedef struct 
{
	uint_8 head;
	double ztie_f, ztie_r;
	double mafie_f, mafie_r;
	ZTIE_ELEMENT_STRUCT ztie_element[15];
} ZTIE_STRUCT;

typedef struct 
{
	int_32 current;  //note update current before calling calculator
    int_32 lag;
    int_32 dlag;
    double  filt;
    double var_est;
}
 TDEV_ELEMENT_STRUCT;

typedef struct 
{
	int_8 update_phase;
	TDEV_ELEMENT_STRUCT tdev_element[TDEV_WINDOWS];
//	TDEV_ELEMENT_STRUCT tdev_1024[3];//variance for size 2,3 and 4 cluster sizes
	
} TDEV_STRUCT;
typedef enum
{
   LOAD_CALNEX_10 = 0,
   LOAD_G_8261_5,
   LOAD_G_8261_8,
   LOAD_G_8261_10,
   LOAD_GENERIC_HIGH
} Load_State;
typedef enum
{
    RM_GM_1		= 0,
	RM_GM_2,
	RM_GPS_1,
	RM_GPS_2,
	RM_GPS_3,
	RM_GPS_4,
	RM_SE_1,
	RM_SE_2,
	RM_RED,
	RM_NONE
} 	RM_State;
typedef enum
{
    FM_SINGLE	=0,
	FM_MULTI
} 	Fmode_State;
typedef enum
{
    TM_SINGLE	=0,
	TM_MULTI
} 	Tmode_State;

typedef struct 
{
	int_16 time_mode; //current time_mode single or multi
	int_16 time_source[8];  //current single source for longterm time traceability RM_NONE is end of list
 	int_16 freq_mode;  //current frequency mode (single or multi)
	int_16 freq_source[8];  //array of allowed frequency source RM_NONE is end of list
   int_16 prev_freq_source[8];
	int_16 assist_mode; //current assist_mode active or disable
	int_16 assist_source[4];  //current assist source
} 
RESOURCE_MANAGEMENT_STRUCT;


//***************************************************************************************
//		 Softclient 2.0 extensions to support new inputs
//
//*****************************************************************************************
#if(NO_PHY==0)
// New Channelized RFE structure for physical channels
typedef struct 
{
	int_16 N,Nmax;
	int_16 index;
	int_8 first;
	double alpha; //oscillator dependent smoothing gain across update default 1/60
	double dfactor; // additional de weighting factor based on total weight 0.1 to 1
	double wavg;  //running average weight using alpha factor
	double fest; // frequency estimate in ppb
	double fest_prev; //spaced N back use to check initial stabilization
	double fest_cur;
	double delta_freq;
	double smooth_delta_freq;
	double res_freq_err;
	// channelized data section
	double weight_chan[RFE_CHAN_MAX]; 
	double fest_chan[RFE_CHAN_MAX]; 
	// end channelized data section
	RFE_ELEMENT_STRUCT rfee[RFE_CHAN_MAX][RFE_WINDOW_MAX];
} RFE_STRUCT_PHY;


typedef struct 
{
	int_32 cur_phase,lag_phase,dlag_phase;
	double TDEV_sample;
	double TVAR_smooth;
	int_16 transient_bucket,start_tdev_flag;	
 }
 GPS_PERFORMANCE_STRUCT;

#endif
/*****************************************************************************/
/*                       ***Public Data Section***                           */
/*  Global variables, if used, should be defined in a ".c" source file.      */
/*  The scope of a global may be extended to other modules by declaring it as*/
/*  as an extern here. 			                                     */
/*****************************************************************************/
extern BOOLEAN jam_now_flag;
extern BOOLEAN never_jam_flag;
extern UINT32 jam_threshold;
extern INT32	mode_jitter;
extern INT32	dsl_flag;
extern INT32	enterprise_flag;
extern FLOAT32 last_good_freq;
extern UINT32 gaptime;
extern UINT32 		FLL_Elapse_Sec_Count;
extern UINT32 		FLL_Elapse_Sec_PTP_Count;
extern UINT32 		FLL_Elapse_Sec_Count_Use;

extern INT16 fll_settle;  
extern double detilt_cluster;
extern double 	holdover_f;
extern double 	holdover_r;
extern double	fdrift_f;
extern double	fdrift_r;
extern double	pll_int_f;
extern double	pll_int_r;
extern MIN_STRUCT  min_detilt;
extern INT32 min_oper_thres_f;
extern INT32 min_oper_thres_r;
extern INT32 min_oper_thres_small_f;
extern INT32 min_oper_thres_small_r;
extern INT32 min_oper_thres_large_f;
extern INT32 min_oper_thres_large_r;
extern Load_State LoadState;
extern int  rfe_settle_count;

extern UINT16 hcount;

#if(NO_PHY==0)
extern INPUT_PROCESS_DATA_PHY IPD_PHY; //DEC 2010 New GPS input channels
extern RFE_STRUCT_PHY RFE_PHY;
extern GPS_PERFORMANCE_STRUCT gps_pa, gps_pb, gps_pc, gps_pd, se_pa,se_pb,red_pa;
extern void init_ipp(void);
extern void init_rfe_phy(void);
extern void Init_Holdover(void);
extern double Get_LO_Pred(void);
extern void start_rfe(double);
extern void update_rfe_phy(void);
extern double report_freq_offset(int);

extern INT32 min_oper_thres_phy[];
extern INT32 Phy_Channel_Transient_Free_Cnt[];

extern int low_warning_phy[]; 
extern int high_warning_phy[]; 
// rfe_turbo flag 1=4x 2=8x speed
extern UINT16 rfe_turbo_flag_phy;
extern UINT16 rfe_turbo_flag_forward_phy;
extern UINT16 rfe_turbo_flag_reverse_phy;
extern double rfe_turbo_acc_phy;
extern INT8 PHY_Floor_Change_Flag[];
extern INT16 skip_rfe_phy;   

extern INT64 rfe_turbo_phase_acc_phy[];
extern double rfe_turbo_phase_phy[];
extern int ctp_phy; //cluster test density point
extern INT32 avg_GPS_A;
extern INT32 avg_GPS_B;
extern INT32 avg_GPS_C;
extern INT32 avg_GPS_D;
extern INT32 avg_RED;
extern INT32  cluster_inc_phy[]; 
extern INT32  cluster_inc_count_phy[]; 
extern INT32  cluster_inc_slew_count_phy[]; 
extern INT32  cluster_inc_sense_phy[]; 
extern INT32  cluster_inc_sense_prev_phy[]; 
extern INT32	min_density_thres_oper_phy;
extern double detilt_rate_phy[];
#endif
extern double detilt_correction_phy[];

extern double time_bias_est_rate;
extern int detilt_blank[]; 

extern double detilt_correction_for;
extern double detilt_correction_rev;
extern double time_bias_est;
extern double residual_time_error;

extern RESOURCE_MANAGEMENT_STRUCT rms;

extern double fdrift_smooth;
extern double	fdrift;
extern double	fdrift_raw;
extern double 	fdrift_warped;
extern double	hold_res_acc;
extern double	hold_res_acc_avg; 

extern UINT16  delta_forward_slew, delta_reverse_slew;
extern UINT16 delta_forward_slew_cnt, delta_reverse_slew_cnt;
extern INT8		PTP_LOS;
extern INT8    Phase_Control_Enable;
extern double forward_weight, reverse_weight;
extern INT32	min_oper_thres_oper;
extern INT32	step_thres_oper;
extern INT8 var_cal_flag;
extern INT16 ptpRate;
extern t_ptpAccessTransportEnum ptpTransport;
// Input Histogram Data Processing Section
extern INPUT_PROCESS_DATA IPD; //may 2010 new general input processing data structure
extern UINT8 timestamp_transition_flag_servo; 
extern INT32 timestamp_align_offset_servo; //GPZ accumulated timestamp error
extern INT32 timestamp_align_offset_prev_servo; //GPZ accumulated timestamp error
extern UINT16 timestamp_skip_cnt_servo;
extern UINT16 timestamp_head_f_servo, timestamp_head_r_servo;
extern INT16 timestamp_align_skip_servo;

// Data Supporting operational offset floor extension

extern INT32 ofw_start_f;
extern INT32 ofw_start_thres_f; 
extern INT32 ofw_cluster_total_f;
extern INT32 ofw_cluster_total_prev_f;
extern INT32 ofw_cluster_total_sum_f;
extern INT32 ofw_cluster_total_sum_prev_f;

extern INT32 next_ofw_start_f; //ensure large enough to avoid lock to rising edge
extern INT32 next_ofw_cluster_total_f;
extern INT32 next_ofw_cluster_total_prev_f;
extern INT32 next_ofw_cluster_total_sum_f;
extern INT32 next_ofw_cluster_total_sum_prev_f;

extern INT32 ofw_gain_f;
extern int ofw_cluster_iter_f; //cluster iteration index
extern int ofw_cur_dir_f;
extern int ofw_prev_dir_f;
extern int ofw_uslew_f;

extern INT32 ofw_start_r; //ensure large enough to avoid lock to rising edge
extern INT32 ofw_start_thres_r; //ensure large enough to avoid lock to rising edge
extern INT32 ofw_cluster_total_r;
extern INT32 ofw_cluster_total_prev_r;
extern INT32 ofw_cluster_total_sum_r;
extern INT32 ofw_cluster_total_sum_prev_r;

extern INT32 next_ofw_start_r; //ensure large enough to avoid lock to rising edge
extern INT32 next_ofw_cluster_total_r;
extern INT32 next_ofw_cluster_total_prev_r;
extern INT32 next_ofw_cluster_total_sum_r;
extern INT32 next_ofw_cluster_total_sum_prev_r;

extern INT32 ofw_gain_r;
extern int ofw_cluster_iter_r; //cluster iteration index
extern int ofw_cur_dir_r;
extern int ofw_prev_dir_r;
extern int ofw_uslew_r;

extern INT32	min_density_thres_oper;
extern INT32 	Initial_Offset;
extern INT32 	Initial_Offset_Reverse;
extern int 	start_in_progress;
extern int 	start_in_progress_PTP;
extern int ctp; //cluster test density point
extern double floor_density_f;
extern double floor_density_r;
extern double floor_density_small_f;
extern double floor_density_small_r;
extern double floor_density_large_f;
extern double floor_density_large_r;
extern double floor_density_band_f;
extern double floor_density_band_r;

extern MIN_STRUCT min_forward, min_reverse;
extern INT32  cluster_width_max;
extern INT32  cluster_inc_f; //was 32
extern INT32  cluster_inc_r;
extern INT32  cluster_inc;
extern INT32  cluster_inc_count_f;
extern INT32  cluster_inc_count_r;
extern INT32  cluster_inc_slew_count_f;
extern INT32  cluster_inc_slew_count_r;
extern INT32  cluster_inc_sense_f;
extern INT32  cluster_inc_sense_r;
extern INT32  cluster_inc_sense_prev_f;
extern INT32  cluster_inc_sense_prev_r;
extern INT32	mode_width_max_oper;
extern INT32	min_density_thres_oper;
extern double 	fbias,rbias;
extern double 	fbias_small,rbias_small;
extern double 	fbias_large,rbias_large;

// min validation section
// Don't forget the other place. (TODO)
extern INT32 rmw_f[],rmw_r[]; //replacement min search windows  //JAN19_MERGE
extern INT32 lvm_f; //last valid min
extern INT32 lvm_r; //last valid minimum
extern long long rm_f, rm_r;   //candidate replacement min
extern INT32 rmf_f, rmf_r;   //candidate replacement min floor
extern INT32 om_f,om_r; 	//output validated min
extern int	rmc_f, rmc_r;	//replacement min cluster count
extern int	mflag_f, mflag_r; //min validity flags
extern int rmfirst_f;
extern int	rmfirst_r;
extern int rmi_f, rmi_r;	
extern INT32	rm_size_oper;

// FLL Status section
extern FLL_STATUS_STRUCT fllstatus;
extern Fll_Clock_State tstate; 
extern Fll_Clock_State prev_tstate;
extern double round_trip_delay_alt; //minimal round trip delay estimate
extern double time_bias_mean, time_bias_var;
extern BOOLEAN o_fll_init;

// RFE Data Section
extern RFE_STRUCT RFE;
extern XFER_DATA_STRUCT xds;
extern INT16 rfe_cindx; //cur_index
extern INT16 rfe_pindx; //prev_index
extern INT16 rfe_ppindx; //prev_index
extern INT16 rfe_pppindx; //prev_index
extern INT16 rfe_findx; //future_index
extern INT16 time_transient_flag;
extern double RFE_in_forward[5];
extern double RFE_in_reverse[5];
extern INT16	hcount_rfe;
extern INT16	hcount_recovery_rfe;
extern INT16 hcount_recovery_rfe_start;

extern UINT16 rfe_turbo_flag;
extern UINT16 rfe_turbo_flag_forward;
extern UINT16 rfe_turbo_flag_reverse;
extern double rfe_turbo_acc;
extern double rfe_turbo_phase_acc[ ];
extern double rfe_turbo_phase[ ];
extern double rfe_turbo_phase_avg[];
extern double rfe_turbo_phase_acc_small[];
extern double rfe_turbo_phase_small[];
extern double rfe_turbo_phase_avg_small[];
extern double rfe_turbo_phase_acc_large[2];
extern double rfe_turbo_phase_large[2];
extern double rfe_turbo_phase_avg_large[2];
extern double rfe_turbo_phase_acc_large_band[2];
extern double rfe_turbo_phase_large_band[2];
extern double rfe_turbo_phase_avg_large_band[2];

extern int turbo_cluster_for;
extern int turbo_cluster_rev;


extern int N_init_rfe;
extern int N_init_rfe_phy;
extern INT8 Forward_Floor_Change_Flag;
extern INT8 Reverse_Floor_Change_Flag;
extern INT8 f_floor_event;
extern INT8 r_floor_event;
extern INT16 skip_rfe;  
extern int short_skip_cnt_f;
extern int short_skip_cnt_r;
extern int low_warning_for; //GPZ NOV 2010 consecutive low density warning
extern int low_warning_rev;
extern int high_warning_for; //GPZ NOV 2010 consecutive low density warning
extern int high_warning_rev;
//extern int load_drop_for; 
//extern int load_drop_rev;

extern int complete_phase_flag;

//IPPM Data Section
#if(!NO_IPPM)				
extern IPPM_STRUCT ippm_forward, ippm_reverse, *pippm;
#endif
extern ZTIE_STRUCT ztie_stats;
// TDEV Data Section
extern TDEV_STRUCT forward_stats,reverse_stats;

/*****************************************************************************/
/*                       ***Function Prototype Section***                    */
/*  This section to be used to prototype public functions.                   */
/*                                                                           */
/*  Descriptions of parameters and return code should be given for each      */
/*  function.                                                                */
/*****************************************************************************/
extern void detilt(void);
extern t_loTypeEnum OCXO_Type(void);
extern void init_pshape(void);
extern void init_pshape_long(void);
//extern void get_fll_status_1min(void);
extern float Get_out_tdev_est(void);
extern void get_fll_status(FLL_STATUS_STRUCT *);
extern Fll_Clock_State get_fll_state(void);
extern Fll_Clock_State get_fll_prev_state(void);
extern UINT16 get_fll_led_state(void);


extern void update_rfe(void);
extern void init_rfe(void);
extern void start_rfe(double);
extern void warm_start_rfe(double);
extern void ippm_analysis(void);
extern void init_ippm(void);
extern void set_ippm_pacing(INT16);
extern void set_ippm_window(INT16);
extern void set_ippm_thres(INT32);// units of ns
extern void reset_ippm(void);
extern double get_ippm_f_jitter(void); 
extern double get_ippm_r_jitter(void); 
extern double get_ippm_f_ipdv(void);
extern double get_ippm_r_ipdv(void);
extern double get_ippm_f_ipdv_99_9(void);
extern double get_ippm_r_ipdv_99_9(void);
extern double get_ippm_f_thres_prob(void);
extern double get_ippm_r_thres_prob(void);

extern void ztie_calc(void);
extern void init_ztie(void);

extern void tdev_calc(TDEV_STRUCT * );
extern void init_tdev(void);

extern void update_input_stats(void);
extern void update_input_stats_phy(void);
extern void min_valid(int);
extern void init_ipp(void);
extern BOOLEAN Is_Major_Time_Change(void);
extern BOOLEAN Is_GPS_OK(RM_State);
extern BOOLEAN Is_Active_Time_Reference_OK(void);
extern BOOLEAN Is_Active_Freq_Reference_OK(void);
extern BOOLEAN Is_Any_Freq_Reference_OK(void);
extern BOOLEAN Is_PTP_FREQ_ACTIVE(void);
extern BOOLEAN Is_PTP_Warm(void);
extern BOOLEAN Is_PTP_TIME_ACTIVE(void);
extern int Jam_Never(BOOLEAN never_jam_enable);
extern int Is_Jam_Needed(void);
extern double sqrt_local(double f64_value);
extern double report_freq_offset(int chan);

#endif /*  H_clk_h */








