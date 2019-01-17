
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

FILE NAME    : sc_ptp_servo.c

AUTHOR       : Ken Ho

DESCRIPTION  : 

The functions in this file are used to configure and retreive PTP servo 
configuration and status.
	SC_ServoConfig()
	SC_VerInfo()
	SC_PtpPerformanceConfig()
	SC_GetPtpPerformance()
	SC_ClearPerformanceCounters()
	SC_GetServoStatus()
	
Revision control header:
$Id: CLK/sc_ptp_servo.c 1.41 2012/04/23 15:18:14PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include "proj_inc.h"
#include <math.h>
#include "target.h"
#include "sc_alarms.h"
#include "sc_servo_api.h"
#include "sc_api.h"
#include "PTP/PTPdef.h"
#include "CLK/clk_private.h"
#include "CLK/ptp.h"
#include "API/sc_rev.h"
#include "DBG/DBG.h"
#include <syslog.h>
#include "sys_defines.h"
#include "fpga_ioctl.h"
#include "fpga_map.h"
int lost_count = 0;
/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/

#define k_TIME_SET_HYST 2  /* Number of seconds before accepting new system time */
#define NUM_OF_PHASORS 5
#define MEAS_TIMEOUT_SEC 2
#define SECOND_IN_NS 1000000000
#define HALF_SECOND_IN_NS (SECOND_IN_NS/2)
#define ROLLOVER_THRES 400000000
#define TIME_CORRECT_THRES 500000000
#define MIN_SYNC_THRESHOLD_NS 1000        /* 1 us */
#define MAX_SYNC_THRESHOLD_NS 1000000000 /* 1 second */
#define DEFAULT_THRESHOLD_NS  80000      /* 80 usecond default */

/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
static UINT32 dw_syncThresholdNs = DEFAULT_THRESHOLD_NS;
static INT64 ll_prevT1Time = 0;
static BOOLEAN sync_was_los = FALSE;
static BOOLEAN sync_to_gps_now = FALSE;
static BOOLEAN sync_to_red_now = FALSE;
static BOOLEAN sync_to_ptp_now = FALSE;
static BOOLEAN time_was_just_adjusted = FALSE;

/* following to varaiables will allow user to see summary of the timestamp mismatch */
INT32 f_mismatch_count=0;
INT32 b_mismatch_count=0;
INT32 f_offset_element=0;
INT32 b_offset_element=0;
INT32 max_timestamp_threshold=2000000; /* set as default */

FIFO_ELEMENT_STRUCT sync_min_sample;
FIFO_ELEMENT_STRUCT delay_min_sample;
static INT32 current_rtd_1sec_min = 0;
static uint_32 ptp_transfer_now_cntr = 0;
#if NO_PHY==0
static PHASOR_FIFO_QUEUE_STRUCT Phasor_FIFO_Queue[NUM_OF_PHASORS];
#endif
static INT64 ll_timestamp_offset = 0;
static INT64 ll_curr_gps_time = 0;  /* current time from GPS reference in TAI time */
static INT64 ll_curr_red_time = 0;  /* current time from GPS reference in TAI time */

static t_servoConfigType   s_servoConfig = {
   k_DEFAULT_SERVO_LO_TYPE,
   k_DEFAULT_SERVO_LO_QL,
   k_DEFAULT_SERVO_TRANSPORT,
   k_DEFAULT_SERVO_PHASE_MODE,
   k_DEFAULT_SERVO_FREQ_CORR,
   k_DEFAULT_SERVO_FREQ_CORR_TIME,
   k_DEFAULT_SERVO_FACT_CORR,
   k_DEFAULT_SERVO_FACT_CORR_TIME,
   k_DEFAULT_SERVO_BRIDGE};  /* Local copy of clock configuration */

static t_ptpIpdvConfigType s_ptpIpdvConfig = {
   k_IPDV_DEFAULT_INTV, 
   k_IPDV_DEFAULT_THRES, 
   k_IPDV_DEFAULT_PACING}; /*Local copy */

static FIFO_QUEUE_STRUCT PTP_data_transfer_queue;
#ifndef LET_ALL_TS_THROUGH
static UINT32  dw_StartOfSetSysTimeAttempts = 0;
#endif
static BOOLEAN o_AttemptsStartedFlag = FALSE;

const float kOscOffsetLimit[] = { [e_RB] = 16, [e_OCXO] = 1000, [e_MINI_OCXO] = 7000 }; //Maximum absolute correction (PPB) per oscillator type

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/
void Check_and_Sync_Using_GPS(void);
void Check_and_Sync_Using_RED(void);
static INT32 Pick_Best_Offset_Mod_Sec(INT32 input, INT32 prev_offset);


/*
----------------------------------------------------------------------------
                               SC_ServoConfig()

Description:
This function sets or gets the servo parameters.

Parameters:

Inputs
	t_servoConfigType *ps_servoConfig
	On a set, this pointer defines the clock configuration.
	
	t_paramOperEnum   b_operation
	Indicates a set or get operation.

Outputs:
	t_servoConfigType *ps_servoConfig
	On a get, the current clock configuration is written to this
	pointer location.

Return value:
    0: function succeeded
   -1: function failed
   -3: invalid value in ps_servoCfg

-----------------------------------------------------------------------------
*/
int SC_ServoConfig(t_servoConfigType *ps_servoConfig,
                   t_paramOperEnum   b_operation)
{
   if (b_operation == e_PARAM_SET)
   {
/* check values */
/* if not Standard mode and Outband, then fail */
      if(ps_servoConfig->dw_ptpPhaseMode != k_PTP_PHASE_MODE_STANDARD)
         return -3;

      switch(ps_servoConfig->e_loType)
      {
      case e_OCXO:
      case e_MINI_OCXO:
      case e_RB: //if new new oscillator types are supported add them to kOscOffsetLimit
         break;
      default:
         return -3;
         break;
      }

      switch(ps_servoConfig->e_ptpTransport)
      {
      case e_PTP_MODE_ETHERNET:
      case e_PTP_MODE_DSL:
      case e_PTP_MODE_SONET:
      case e_PTP_MODE_MICROWAVE:
      case e_PTP_MODE_SLOW_ETHERNET:
      case e_PTP_MODE_HIGH_JITTER_ACCESS:
         break;
      default:
         return -3;
         break;
      }

      //check boundaries for warm start value. Use Zero if it is too big.
      if(fabs(ps_servoConfig->f_freqCorr) > kOscOffsetLimit[ps_servoConfig->e_loType])
      {
        SC_Event(e_EXCESSIVE_FREQUENCY_CORR, e_EVENT_WARN, e_EVENT_TRANSIENT, sc_alarm_lst[e_EXCESSIVE_FREQUENCY_CORR].dsc_str, 0);
        ps_servoConfig->f_freqCorr = 0.0;
      }

//      if(ps_servoConfig->f_freqCorr)
//      if(ps_servoConfig->f_freqCalFactory)

/* store to local copy */
      s_servoConfig = *ps_servoConfig;
   }
   else if (b_operation == e_PARAM_GET)
   {
      *ps_servoConfig = s_servoConfig;
   }
   else
   {
         return -3;
   }

   return 0;
}

/*
----------------------------------------------------------------------------
                                SC_VerInfo()

Description:
This function requests the current version information.  The version
information is copied to the input data buffer as a string.  The string
contains name value pairs separated by commas.  White space should be
ignored.  If the buffer is not large enough to contain the entire version
string, the output will be truncated to fit the available space.  Output
will be NULL terminated.  e.g. ServoRev=1.0.0,SCiRev=1.0.0,PTPVer=2


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
void SC_VerInfo(INT8 *c_revBuffer, UINT16 w_lenBuffer)
{
  if(c_revBuffer != NULL)
  {
    snprintf((char *)c_revBuffer, w_lenBuffer, "%s,%s,%s", SC_PRODUCT_NUM, k_SC_SERVO_REV, k_SC_LIB_REV);
  }
}
/*
----------------------------------------------------------------------------
                                SC_ClkIpdvConfig()

Description:
This function gets or sets the current IPDV performance parameters.   On a get, 
the current parameters are copied into the supplied structure.  On a set, 
the current parameters are set from the supplied structure.  Note that all 
parameters are valid during set operations.  Modifications to parameters 
should be done using a read-modify-write cycle.

Parameters:

Inputs
	t_ptpIpdvConfigType *ps_ptpIpdvConfig 
	On a set, this pointer defines the IPDV performance configuration.
	
	t_paramOperEnum b_operation  
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get parameters
		e_PARAM_SET - Set parameters
Outputs:
	t_ptpIpdvConfigType *ps_ptpIpdvConfig 
	On a get, the current IPDV performance configuration is written
	to this pointer location.

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int SC_ClkIpdvConfig(
	t_ptpIpdvConfigType *ps_ptpIpdvConfig,
	t_paramOperEnum      b_operation
)
{
/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      *ps_ptpIpdvConfig = s_ptpIpdvConfig;
      break;
   case e_PARAM_SET:
/* check interval is 1-255 minutes */
      if((ps_ptpIpdvConfig->w_ipdvObservIntv < 1) || 
         (ps_ptpIpdvConfig->w_ipdvObservIntv > 255))
      {
         return -1;
      }

/* check pacing is 1,2,4...512 samples */
      switch(ps_ptpIpdvConfig->w_pacingFactor)
      {
      case 1:
      case 2:
      case 4:
      case 8:
      case 16:
      case 32:
      case 64:
      case 128:
      case 256:
      case 512:
         break;
      default:
         return -1;
      }

/* check threshold 0 - 1,000,000,000 nsecs */
      if((ps_ptpIpdvConfig->l_ipdvThres < 0) || 
         (ps_ptpIpdvConfig->l_ipdvThres > 1000000000))
      {
         return -1;
      }

      s_ptpIpdvConfig = *ps_ptpIpdvConfig;

      set_ippm_thres((INT32)(s_ptpIpdvConfig.l_ipdvThres));
      set_ippm_window(s_ptpIpdvConfig.w_ipdvObservIntv);
      set_ippm_pacing(s_ptpIpdvConfig.w_pacingFactor);
      break;
   default:
      return -1;
      break; 
   }
   return 0;
}

/*
----------------------------------------------------------------------------
                                SC_ClkIpdvGet()

Description:
This function gets or sets the current IPDV performance parameters.   On a get, 
the current parameters are copied into the supplied structure.  On a set, 
the current parameters are set from the supplied structure.  Note that all 
parameters are valid during set operations.  Modifications to parameters 
should be done using a read-modify-write cycle.

Parameters:

Inputs
	None

Outputs:
	t_ptpIpdvType *ps_ptpIpdvConfig 
	On a get, the current IPDV performance status is written
	to this pointer location.

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int SC_ClkIpdvGet(
	t_ptpIpdvType *ps_ptpIpdv
)
{
   ps_ptpIpdv->f_ipdvFwdPct = get_ippm_f_thres_prob() * 100.0; 
   ps_ptpIpdv->f_ipdvFwdMax = get_ippm_f_ipdv_99_9() / 1000.0; 
   ps_ptpIpdv->f_ipdvFwdJitter = get_ippm_f_jitter() / 1000.0; 
   ps_ptpIpdv->f_ipdvRevPct = get_ippm_r_thres_prob() * 100.0; 
   ps_ptpIpdv->f_ipdvRevMax = get_ippm_r_ipdv_99_9() / 1000.0; 
   ps_ptpIpdv->f_ipdvRevJitter = get_ippm_r_jitter() / 1000.0;

   return 0;
}

/*
----------------------------------------------------------------------------
                                SC_GetPtpPerformance()

Description:
This function requests the current PTP performance data.  

Parameters:

Inputs
	None

Outputs:
	t_ptpPerformanceStatusType *ps_ptpPerformanceConfig 
	The current PTP performance data is written to this pointer location.

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int SC_GetPtpPerformance(
	t_ptpPerformanceStatusType *ps_ptpPerformanceConfig
)
{
	FLL_STATUS_STRUCT fp;
	UINT16 sync;
	UINT16 delay;

	get_fll_status(&fp);
	Get_Flow_Status(&sync, &delay, 0);

   ps_ptpPerformanceConfig->e_fllState = fp.cur_state; 
   ps_ptpPerformanceConfig->dw_fllStateDur = fp.current_state_dur; 
   ps_ptpPerformanceConfig->f_fwdWeight = 100.0*fp.f_weight; 
   ps_ptpPerformanceConfig->dw_fwdTransFree900 = 900 - fp.f_avg_tp15min; 
   ps_ptpPerformanceConfig->dw_fwdTransFree3600 = 3600 - fp.f_avg_tp60min; 
   ps_ptpPerformanceConfig->f_fwdPct = fp.f_cdensity * 200.0; 
   ps_ptpPerformanceConfig->f_fwdMinTdev = fp.f_oper_mtdev ; 
   ps_ptpPerformanceConfig->f_fwdMafie = fp.f_mafie; 
   ps_ptpPerformanceConfig->f_fwdMinClstrWidth = fp.f_min_cluster_width; 
   ps_ptpPerformanceConfig->f_fwdModeWidth = fp.f_mode_width; 
   ps_ptpPerformanceConfig->f_revWeight = 100.0*(1.0- fp.f_weight); 
   ps_ptpPerformanceConfig->dw_revTransFree900 = 900 - fp.r_avg_tp15min; 
   ps_ptpPerformanceConfig->dw_revTransFree3600 = 3600 - fp.r_avg_tp60min; 
   ps_ptpPerformanceConfig->f_revPct = fp.r_cdensity * 200.0; 
   ps_ptpPerformanceConfig->f_revMinTdev = fp.r_oper_mtdev; 
   ps_ptpPerformanceConfig->f_revMafie = fp.r_mafie;
   ps_ptpPerformanceConfig->f_revMinClstrWidth = fp.r_min_cluster_width; 
   ps_ptpPerformanceConfig->f_revModeWidth = fp.r_mode_width; 
   ps_ptpPerformanceConfig->f_freqCorrection = fp.freq_cor; 
   ps_ptpPerformanceConfig->f_phaseCorrection = fp.time_cor; 
   ps_ptpPerformanceConfig->f_tdevEstimate = fp.out_tdev_est;
   ps_ptpPerformanceConfig->f_mdevEstimate = fp.out_mdev_est;
   ps_ptpPerformanceConfig->f_residualPhaseErr = fp.residual_time_cor; 
   ps_ptpPerformanceConfig->f_minRoundTripDly = fp.rtd_us; 
   ps_ptpPerformanceConfig->w_fwdPktRate = sync; 
   ps_ptpPerformanceConfig->w_revPktRate = delay; 

	return 0;
}
/*
----------------------------------------------------------------------------
                                SC_ClkIpdvClrCtrs()

Description:
This function clears the current PTP performance counters. 

Parameters:

Inputs
	None

Outputs:
	None


Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int SC_ClkIpdvClrCtrs( void )
{
   reset_ippm();
   return 0;
}
/*
----------------------------------------------------------------------------
                                SC_GetServoConfig()

Description:
This function get clock servo configuration parameters.

Parameters:

Inputs
        None

Outputs:
        ps_servoCfg
        Pointer to clock configuration data structure

Return value:

         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_GetServoConfig(t_servoConfigType* ps_servoCfg);

/*
----------------------------------------------------------------------------
                                SC_GetServoStatus()

Description:
This function reads the current PTP servo status. 
Parameters:

Inputs
	None

Outputs:
	t_ptpServoStatusType *ps_ptpServoStatus
	The current PTP performance status is written to this pointer location.

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int SC_GetServoStatus(
	t_ptpServoStatusType *ps_ptpServoStatus
)
{
	FLL_STATUS_STRUCT fp;

	get_fll_status(&fp);

   ps_ptpServoStatus->e_fllState = fp.cur_state;
   ps_ptpServoStatus->dw_fllStateDur = fp.current_state_dur;

	return 0;
}

/*
----------------------------------------------------------------------------
                                Get_Freq_Corr()

Description:
This function reads the frequency correction that is stored every hour in
non-volatile memory.

Inputs
	None

Outputs:
	FLOAT32  *pf_freqCorr
	The current frequency correction in ppb.
	
   UINT32   *pdw_freqCorrTimeDelta
   The how old the correction is in seconds

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int Get_Freq_Corr(
   FLOAT32   *pf_freqCorr,          /* OCXO frequency offset sampled hourly    */ 
   UINT32    *pdw_freqCorrTimeDelta /* timestamp for hourly frequency offset delta (s) */
)
{
   t_ptpTimeStampType  s_sysTime;
   INT16 w_utcOffset;

/* get current time */
   if(SC_SystemTime(&s_sysTime,&w_utcOffset,e_PARAM_GET))
   {
      debug_printf(UNMASK_PRT, "Error while running SC_SystemTime()");
   }

   *pf_freqCorr = s_servoConfig.f_freqCorr;
   if(s_servoConfig.s_freqCorrTime.u48_sec > s_sysTime.u48_sec)
   {
      *pdw_freqCorrTimeDelta = 0x7FFFFFFF;   /* return large number */   
   }
   else
   {
      *pdw_freqCorrTimeDelta = s_sysTime.u48_sec - s_servoConfig.s_freqCorrTime.u48_sec;
   }

   return 0;
}
/*
----------------------------------------------------------------------------
                                Get_Freq_Corr_Factory()

Description:
This function reads the factory frequency correction.  This value should
be a known good calibration.

Inputs
	None

Outputs:
	FLOAT32            *pf_freqCalFactory
	The factory frequency correction in ppb.
	
   UINT64 *ps_freqCorrTime
   The timestamp when the correction was recorded (in seconds TAI)

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int Get_Freq_Corr_Factory(
   FLOAT32  *pf_freqCalFactory,     /* OCXO frequency offset at factory        */
   UINT64   *ps_freqCalFactoryTime /* timestamp for factory frequency offset  */
)
{
   *pf_freqCalFactory = s_servoConfig.f_freqCalFactory;
   *ps_freqCalFactoryTime = s_servoConfig.s_freqCalFactoryTime.u48_sec;

   return 0;
}
/*
----------------------------------------------------------------------------
                                Get_Bridge_Time()

Description:
This function reads the current bridging time set by the initialization
function.

Inputs
	None

Outputs:
	UINT32 *pdd_bridgeTime
	The bridge time in nanoseconds.
	
Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int Get_Bridge_Time(UINT32 *pdd_bridgeTime)
{
   *pdd_bridgeTime = s_servoConfig.dw_bridgeTime;

   return 0;
}
/*
----------------------------------------------------------------------------
                                Get_Access_Transport()

Description:
This function reads the frequency correction that is stored every hour in
non-volatile memory.

Inputs
	None

Outputs:
	t_ptpAccessTransportEnum *pe_ptpTransport
   The enumerated transport type

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int Get_Access_Transport(t_ptpAccessTransportEnum *pe_ptpTransport)
{
   *pe_ptpTransport = s_servoConfig.e_ptpTransport;

   return 0;
}
/*
----------------------------------------------------------------------------
                                Get_LO_Type()

Description:
This function reads the frequency correction that is stored every hour in
non-volatile memory.

Inputs
	None

Outputs:
	t_loTypeEnum *pe_loType
   The enumerated local oscillator type.

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int Get_LO_Type(t_loTypeEnum *pe_loType)
{
   *pe_loType = s_servoConfig.e_loType; 

   return 0;
}

/*
----------------------------------------------------------------------------
                                Try_SC_SystemTimeSet()

Description:
This function is called by the Softclient code.  This function will decide
if the SystemTime command should be sent to the Timestamper.  If no good
packets are received for 10 seconds, then the time will be set.


Parameters:

Inputs
	t_ptpTimeStampType *ps_sysTime
	On a set, this pointer defines the the current system time.

	INT16 i_utcOffset
	On a set, this is the UTC offset.

Outputs:
   None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int Try_SC_SystemTimeSet(
	PTP_t_TmStmp  *ps_sysTimeTry,
	INT16          i_utcOffset 
)
{
#ifdef LET_ALL_TS_THROUGH
   debug_printf(GEORGE_PTP_PRT, "!!!!!!!!!!!!sending time, but time allowed to set\n");
   return -1;
#else
   int dw_retVal = -1;
   UINT32 dw_currTick = Get_Tick();
   INT16 i_localUtcOffset = i_utcOffset;
   t_ptpTimeStampType  s_oldsysTime;
   INT16 w_oldutcOffset;

/* if already started attempts, then see if hysteresis time is passed */
   if(o_AttemptsStartedFlag)
   {
 //     debug_printf(GEORGE_PTP_PRT, "attempt %ld to send time\n", dw_currTick);
     if(dw_currTick > (dw_StartOfSetSysTimeAttempts + k_TIME_SET_HYST))
      {
         debug_printf(GEORGE_PTP_PRT, "!!!!!!!!!!!!sending time\n");
         time_was_just_adjusted = TRUE;



/* get current time */
         if(SC_SystemTime(&s_oldsysTime,&w_oldutcOffset,e_PARAM_GET))
         {
            debug_printf(UNMASK_PRT, "Error while running SC_SystemTime()");
         }

         if((dw_retVal = SC_SystemTime((t_ptpTimeStampType *)ps_sysTimeTry, &i_localUtcOffset, e_PARAM_SET)))
         {
            debug_printf(UNMASK_PRT, "Error while running SC_SystemTime()");
         }
         else
         {

      	      event_printf(e_TIMELINE_CHANGED, e_EVENT_NOTC, e_EVENT_TRANSIENT,
			             sc_alarm_lst[e_TIMELINE_CHANGED].dsc_str, (UINT32)(s_oldsysTime.u48_sec));
               debug_printf(GEORGE_PTP_PRT,  "!!!Try_SC_SystemTimeSet() - Transient");

//            Major_TS_Change();
         }
         o_AttemptsStartedFlag = FALSE;
      }  
   }
/* get ticks at the first attempt */
   else
   {
     dw_StartOfSetSysTimeAttempts = Get_Tick();      
      o_AttemptsStartedFlag = TRUE;
   }

   return dw_retVal;
#endif
}

/*
----------------------------------------------------------------------------
                                SC_goodPacketRstCounter()

Description:
This function Resets the system time set flag.  Therefore restarting the
hysteresis period


Parameters:

Inputs
   None

Outputs:
   None

Return value:
	None
-----------------------------------------------------------------------------
*/
void SC_goodPacketRstCounter(void)
{
   o_AttemptsStartedFlag = FALSE;
}
/*
----------------------------------------------------------------------------
                                Is_PTP_Phase_Mode_Standard()

Description:
This function returns TRUE if PTP phase is in standard mode. 


Parameters:

Inputs
   None

Outputs:
   None

Return value:
	TRUE  - PTP Phase Mode is Standard
	FALSE - PTP Phase Mode is Coupled
-----------------------------------------------------------------------------
*/
BOOLEAN Is_PTP_Mode_Standard(void)
{
   if(s_servoConfig.dw_ptpPhaseMode & k_PTP_PHASE_MODE_STANDARD)
   {
      return TRUE;
   }
   
   return FALSE;
}

/*
----------------------------------------------------------------------------
                                Is_PTP_Mode_Inband()

Description:
This function returns TRUE if PTP phase correction are in band. 


Parameters:

Inputs
   None

Outputs:
   None

Return value:
	TRUE  - PTP Phase Mode is Standard
	FALSE - PTP Phase Mode is Coupled
-----------------------------------------------------------------------------
*/
BOOLEAN Is_PTP_Mode_Inband(void)
{
   if(s_servoConfig.dw_ptpPhaseMode & k_PTP_PHASE_MODE_INBAND)
   {
      return TRUE;
   }
   
   return FALSE;
}

/*
----------------------------------------------------------------------------
                                SC_GetServoConfig()

Description:
This function get clock servo configuration parameters.

Parameters:

Inputs
        None

Outputs:
        ps_servoCfg
        Pointer to clock configuration data structure

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int SC_GetServoConfig(t_servoConfigType* ps_servoCfg)
{
   if(SC_ServoConfig(ps_servoCfg, e_PARAM_GET))
      return -1;
   else
           return 0;
}

/*
----------------------------------------------------------------------------
                                Get_Sync_Rate()

Description:
This function set initial clock servo configuration parameters. The
parameter will be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
        None

Outputs:
        INT16 *rate

Return value:
         0: function succeeded
        -1: function failed
        -3: invalid value in ps_servoCfg

-----------------------------------------------------------------------------
*/
#if 0
// jyang: This function is obsolete.
int Get_Sync_Rate(INT16 *rate)
{
  int status = 0;
  SC_t_MntTLV s_Tlv;

  status = SC_PTP_SyncIntv(e_PARAM_GET, &s_Tlv, 0);
  if (status == -1)
  {
//      printf("Error: Unsuccessful\n");
  }
  else if (status == -2)
  {
//      printf("Error: PTP not initialized\n");
     status = -1;
  }
  else
  {
    *rate = s_Tlv.pb_data[0];
//    printf("rate %d\n", (int)*rate);
      status = 0;
  }  

   return status;    
}
#endif
/*
----------------------------------------------------------------------------
                                Transfer_PTP_data_now

This function empties the Tranfer FIFO into the servo FIFO.  It is called
once a second by the servo code.

Parameters:
OUT:
	FIFO_STRUCT *PTP_Fifo_Sync   - pointer to servo Sync FIFO structure
   FIFO_STRUCT *PTP_Fifo_Delay  - pointer to servo Delay FIFO structure
   UINT16 *num_sync_pkts        - number of new sync values popped off FIFO
   UINT16 *num_delay_pkts       - number of new delay vales popped off FIFO

Return value:
	UINT8 = 0 if no overrun
	      = 1 if overrun

-----------------------------------------------------------------------------
*/ 
void Transfer_PTP_data_now(
   FIFO_STRUCT *PTP_Fifo_Sync, 
   FIFO_STRUCT *PTP_Fifo_Delay, 
   UINT16 *num_sync_pkts,
   UINT16 *num_delay_pkts
)
{
   INT64 ll_t1_ns;
   INT64 ll_t2_ns;
   INT64 ll_local_ns;
   INT64 ll_offset_ns;
   INT64 ll_delay_ns;
   INT64 ll_new_ns;
   INT64 ll_corr_time;
   INT64 ll_nsecRes;
   BOOLEAN temp_sync_was_los = FALSE;
	INT16 rate;
  	UINT16 index_min_delay = 0;
  	UINT16 index_min_sync = 0;
	t_ptpTimeStampType s_actTime_local;
	t_ptpTimeStampType s_actTime;
	INT16 d_utcOffset = 0;
	INT32  dw_minSyncOffset = -1000000000;
        INT32  dw_minDelayOffset = -1000000000;
	UINT16 inq_index  = PTP_data_transfer_queue.inq_index;
	UINT16 out_index = PTP_data_transfer_queue.out_index;
	UINT8  queue_index,last_index;
	UINT16 new_FIFO_element_index;
   t_ptpTimeStampType  s_oldsysTime;
   INT16 w_oldutcOffset;
   UINT8 servo_chan;
   int curr_chan = GetCurrChan(FALSE);
   INT32 prev_offset;
	INT32 new_offset;
	INT32 best_offset;

   servo_chan = UserChan2ServoIndex(curr_chan);
	Get_servoChanSelectRate(SERVO_CHAN_PTP1, &rate);
	
   *num_sync_pkts = 0;
   *num_delay_pkts = 0;

/* get sync and delay rates */
   while(inq_index != out_index)
	{
		queue_index = PTP_data_transfer_queue.queue_index[out_index];

/* if delay data then send to delay local fifo */
		if(queue_index == PTP_DELAY_TRANSFER_QUEUE)
		{
         *num_delay_pkts = *num_delay_pkts + 1;

         {
   			new_FIFO_element_index = (PTP_Fifo_Delay->fifo_in_index + 1) % FIFO_BUF_SIZE;
   			
   /* data is transfered to local Delay FIFO */			
   			PTP_Fifo_Delay->offset[new_FIFO_element_index] = 
   					PTP_data_transfer_queue.fifo_element[out_index].offset + (long int)detilt_correction_rev;
   			PTP_Fifo_Delay->fifo_in_index = new_FIFO_element_index;

            debug_printf(PDV_PROBE_PRT, "B,00000,%010ld,%09ld,%010ld,%09ld,+0000000000\n", 
               PTP_data_transfer_queue.fifo_element[out_index].master_sec, 
               PTP_data_transfer_queue.fifo_element[out_index].master_nsec, 
               PTP_data_transfer_queue.fifo_element[out_index].client_sec, 
               PTP_data_transfer_queue.fifo_element[out_index].client_nsec, 
               PTP_data_transfer_queue.fifo_element[out_index].offset);
            if(dw_minSyncOffset < PTP_data_transfer_queue.fifo_element[out_index].offset)
            {
      		    index_min_delay = out_index;
		          delay_min_sample = PTP_data_transfer_queue.fifo_element[index_min_delay];
              dw_minSyncOffset = PTP_data_transfer_queue.fifo_element[out_index].offset;
            }
		   }
  			PTP_data_transfer_queue.out_index = out_index = (out_index + 1 ) % FIFO_QUEUE_BUF_SIZE;
      }
/* if sync data then send to sync local fifo */
		else if(queue_index == PTP_SYNC_TRANSFER_QUEUE)
		{
         *num_sync_pkts = *num_sync_pkts + 1;

         {
   			new_FIFO_element_index = (PTP_Fifo_Sync->fifo_in_index + 1) % FIFO_BUF_SIZE;
   			
   /* data is transfered to local Delay FIFO */			
   			PTP_Fifo_Sync->offset[new_FIFO_element_index] =
   					PTP_data_transfer_queue.fifo_element[out_index].offset + (long int)detilt_correction_for;		
   			PTP_Fifo_Sync->fifo_in_index = new_FIFO_element_index;

            debug_printf(PDV_PROBE_PRT, "F,00000,%010ld,%09ld,%010ld,%09ld,+0000000000\n", 
               PTP_data_transfer_queue.fifo_element[out_index].master_sec, 
               PTP_data_transfer_queue.fifo_element[out_index].master_nsec, 
               PTP_data_transfer_queue.fifo_element[out_index].client_sec, 
               PTP_data_transfer_queue.fifo_element[out_index].client_nsec, 
               PTP_data_transfer_queue.fifo_element[out_index].offset);
/* calculate time in NS of t1 */
            ll_nsecRes = (INT64)PTP_data_transfer_queue.fifo_element[out_index].master_sec * (INT64)1000000000 +
                         (INT64)PTP_data_transfer_queue.fifo_element[out_index].master_nsec;

            if(ll_nsecRes > ll_prevT1Time + (INT64)250000000)
            {
               temp_sync_was_los = TRUE;
            }
//            printf("ll_nsecRes: %lld ll_prevT1Time %lld sync_was_los %d\n",ll_nsecRes,ll_prevT1Time, (int)sync_was_los);
            ll_prevT1Time = ll_nsecRes;

            if(dw_minDelayOffset < PTP_data_transfer_queue.fifo_element[out_index].offset)
            {
               dw_minDelayOffset = PTP_data_transfer_queue.fifo_element[out_index].offset;

/* use the  minimum sync offset element for the calculation of the timestamp */               
       		   index_min_sync = out_index;
            }

		   }
  		   sync_min_sample  = PTP_data_transfer_queue.fifo_element[out_index];

        last_index = out_index;
  			PTP_data_transfer_queue.out_index = out_index = (out_index + 1 ) % FIFO_QUEUE_BUF_SIZE;	
		}
	}
/* queue is now empty */

/* update 1 second min RTD if there are values to calcuate */
   if((*num_sync_pkts > 0) && (*num_delay_pkts > 0))
   {
      current_rtd_1sec_min = -(dw_minSyncOffset + dw_minDelayOffset);
      
	/* perform setting of timestamper once */ 
//		if(ptp_transfer_now_cntr++ == 3)
		{
//         ptp_transfer_now_cntr = 0;
         sync_to_ptp_now = FALSE;
#if 1
	/* get the current timestamp */
			SC_SystemTime(&s_actTime_local,&d_utcOffset,e_PARAM_GET);	
 //        Restart_Phase_Time();

         ll_t1_ns = ((INT64)sync_min_sample.master_sec * (INT64)SECOND_IN_NS) + (INT64)sync_min_sample.master_nsec;
         ll_t2_ns = ((INT64)sync_min_sample.client_sec * (INT64)SECOND_IN_NS) + (INT64)sync_min_sample.client_nsec;
         ll_local_ns = ((INT64)s_actTime_local.u48_sec * (INT64)SECOND_IN_NS) + (INT64)s_actTime_local.dw_nsec;
         ll_offset_ns = ll_t1_ns - ll_t2_ns;
         ll_delay_ns = ll_local_ns - ll_t2_ns;
         ll_new_ns = ll_t1_ns + ll_delay_ns;

         ll_corr_time = ll_new_ns - ll_local_ns;
         if(ll_corr_time < 0)
            ll_corr_time = -ll_corr_time;
         if((ll_corr_time > TIME_CORRECT_THRES) && (servo_chan == RM_GM_1))
         {
            ptp_transfer_now_cntr++;

   			debug_printf(GEORGE_PTP_PRT, "current: %ld.%09ld t2: %ld.%09ld t1: %ld.%09ld\n", 
   			(UINT32)s_actTime_local.u48_sec,
   			s_actTime_local.dw_nsec,
   			sync_min_sample.client_sec,
   			sync_min_sample.client_nsec,
   			sync_min_sample.master_sec,
   			sync_min_sample.master_nsec);

   			debug_printf(GEORGE_PTP_PRT, "t1: %lld t2: %lld local %lld offset: %lld delay: %lld\n", 
                  ll_t1_ns, ll_t2_ns, ll_local_ns, ll_offset_ns, ll_delay_ns);

   			s_actTime.u48_sec = ll_new_ns / SECOND_IN_NS;
   			s_actTime.dw_nsec = ll_new_ns % SECOND_IN_NS;	


            if(ptp_transfer_now_cntr > 3)
            {
               SC_SystemTime(&s_oldsysTime,&w_oldutcOffset,e_PARAM_GET);
               time_was_just_adjusted = TRUE;
      			SC_SystemTime(&s_actTime,&d_utcOffset,e_PARAM_SET);	

            	event_printf(e_TIMELINE_CHANGED, e_EVENT_NOTC, e_EVENT_TRANSIENT,
   			             sc_alarm_lst[e_TIMELINE_CHANGED].dsc_str, (UINT32)(s_oldsysTime.u48_sec));

      			debug_printf(GEORGE_PTP_PRT, "new time setting: %ld.%09ld error was %lld\n", 
      			(UINT32)s_actTime.u48_sec,
      			(UINT32)s_actTime.dw_nsec, 
               ll_corr_time);
               ptp_transfer_now_cntr = 0;
            }
         }
         else
         {
            ptp_transfer_now_cntr = 0;
//         	debug_printf(GEORGE_PTP_PRT, "do not reset timestamper, only: %lld ns off\n", ll_corr_time);
   		}
#endif
		}      
   }
   else
   {
      current_rtd_1sec_min = 0;
   }
   debug_printf(RTD_1SEC_PRT, "RTD: %ld\n", current_rtd_1sec_min);


	if((*num_sync_pkts > 0) && (*num_delay_pkts > 0))
	{
		Set_servoChanSelectValid(SERVO_CHAN_PTP1, TRUE);
	}
	else
	{
		Set_servoChanSelectValid(SERVO_CHAN_PTP1, FALSE);		
	}
         
   if(temp_sync_was_los)
   {
      sync_was_los = TRUE;
   }
   else
   {
      sync_was_los = FALSE;
   }

	return;
}
#if NO_PHY == 0
void Transfer_Phasor_data_now(
   SC_servoChanEnum servoChan,
   FIFO_STRUCT *Phasor_Fifo,
   UINT16 *measCnt
)
{
	INT16 rate;
	UINT16 inq_index;
	UINT16 out_index;
	UINT16 old_FIFO_element_index;
	UINT16 new_FIFO_element_index;
	INT32 offset, delta;
   UINT8 phasor_chan; 
   SC_t_servoChanSelectTableType s_ChanSelectTable;
   UINT8 o_valid;

	Get_servoChanSelectRate(servoChan, &rate);

/* offset one to match array index */
	phasor_chan = servoChan - SERVO_CHAN_GPS1;
	inq_index  = Phasor_FIFO_Queue[phasor_chan].inq_index;
	out_index = Phasor_FIFO_Queue[phasor_chan].out_index;
//	*measCnt = 0;
//   debug_printf(PHASOR_PROBE_PRT, "inq_index %d out_index %d phasor_chan %d\n", 
//		(int)inq_index,
//      (int)out_index,
//      (int)phasor_chan);
/* if channel is invalid, then skip */
/* get o_valid for the channel */
    Get_servoChanSelect(&s_ChanSelectTable);
    o_valid = s_ChanSelectTable.s_servoChanSelect[servoChan].o_valid;

	if((inq_index == out_index) && (Phasor_FIFO_Queue[phasor_chan].los_cntr < MAX_LOS_CNTR))
	{
		Phasor_FIFO_Queue[phasor_chan].los_cntr++;
	}

	while(inq_index != out_index)
	{
//		*measCnt++;
		{

			old_FIFO_element_index = Phasor_Fifo->fifo_in_index;
			new_FIFO_element_index = (Phasor_Fifo->fifo_in_index + 1) % FIFO_BUF_SIZE;

			/* data is transfered to local Delay FIFO */			
//			offset = Phasor_FIFO_Queue[phasor_chan].offset[out_index] + (long int)detilt_correction_phy[phasor_chan];
			offset = Phasor_FIFO_Queue[phasor_chan].offset[out_index];
			delta = offset - Phasor_Fifo->offset[old_FIFO_element_index];
//         ll_curr_gps_time = Phasor_FIFO_Queue[phasor_chan].time[out_index];

/* scale offset to be with +/- 0.5 secs */
#if 0
			if(offset > HALF_SECOND_IN_NS)
			{
				offset -= SECOND_IN_NS;	
			}
			else if (offset < -HALF_SECOND_IN_NS)
			{
				offset += SECOND_IN_NS;	
			}
#endif

         if(o_valid)
         {
   			Phasor_Fifo->offset[new_FIFO_element_index] = offset;
   			Phasor_Fifo->fifo_in_index = new_FIFO_element_index;
			}

			delta = offset - Phasor_Fifo->offset[old_FIFO_element_index];
			Phasor_FIFO_Queue[phasor_chan].los_cntr = 0;

//          debug_printf(PHASOR_PROBE_PRT, "chan: %d offset: %ld delta %ld rate %d los_cntr %d\n", 
//  					(int)phasor_chan, 
//  					offset,
//  					(int)delta,
//               (int)rate,
//               (int)Phasor_FIFO_Queue[phasor_chan].los_cntr);
		}
		Phasor_FIFO_Queue[phasor_chan].out_index = out_index = (out_index + 1 ) % FIFO_PHASOR_QUEUE_BUF_SIZE;
	}

	if(Phasor_FIFO_Queue[phasor_chan].los_cntr > (3 * rate))
	{
		if(servoChan == SERVO_CHAN_GPS1)
			printf("66666..%s: los_cntr %d..rate %d.\n", __FUNCTION__, Phasor_FIFO_Queue[phasor_chan].los_cntr, rate);
		Set_servoChanSelectValid(servoChan, FALSE);
	}
	else
	{
		Set_servoChanSelectValid(servoChan, TRUE);		
	}
	
	return;
}
#endif
/*
----------------------------------------------------------------------------
                                Transfer_PTP_data_to_servo

This function will place a new set of timestamp into a transfer FIFO, which 
is emptied by the servo task once a second.


Parameters:
IN:
	UINT8 queue_index                - Which FIFO (SYNC=0, Delay=1) 
   FIFO_ELEMENT_STRUCT *PTP_element - The actual timestamp data element

Return value:
	UINT8 = 0 if no overrun
	      = 1 if overrun

-----------------------------------------------------------------------------
*/ 
UINT8 Transfer_PTP_data_to_servo(UINT8 queue_index, FIFO_ELEMENT_STRUCT *PTP_element)
{
	UINT16 next_in_index;		
	UINT8  error = FALSE;

//	_int_disable();
	next_in_index = (PTP_data_transfer_queue.inq_index + 1) % FIFO_QUEUE_BUF_SIZE;
	
/* check to see if queue is full */
	if(next_in_index != PTP_data_transfer_queue.out_index)
	{
/* enter next element in to queue */
		PTP_data_transfer_queue.fifo_element[PTP_data_transfer_queue.inq_index] = *PTP_element;
		PTP_data_transfer_queue.queue_index[PTP_data_transfer_queue.inq_index] = queue_index;
		
		PTP_data_transfer_queue.inq_index = next_in_index;
	}
	else
	{
		error = TRUE;
	}
//	_int_enable();
	
	return error;
}
/*
----------------------------------------------------------------------------
                                CLK_Init_PTP_Tranfer_Queue

This function reset the indexes for the Tranfer Fifo

Parameters:
	None

Return value:
	None

-----------------------------------------------------------------------------
*/ 
void CLK_Init_PTP_Tranfer_Queue(void)
{
  int i;

  PTP_data_transfer_queue.inq_index = 0;
  PTP_data_transfer_queue.out_index = 0;	
#if NO_PHY == 0
  memset(Phasor_FIFO_Queue  ,0 ,sizeof(PHASOR_FIFO_QUEUE_STRUCT) * NUM_OF_PHASORS);

  for(i=0;i<NUM_OF_PHASORS;i++)
  {
    Phasor_FIFO_Queue[i].los_cntr = MAX_LOS_CNTR;
  }
#endif
}

/*
----------------------------------------------------------------------------
                                SC_TimestampPairToServo()

Description:
This function will transfer either a t1, t2 or t3, t4 timestamp pair to 
the PTP servo Task.  This function should be called every time a new
pair is available for the PTP servo.

Parameters:

Inputs
	ps_timestampPair
	Pointer to timestamp pair (either t1, t2 or t3, t4 pair)

  UINT8 b_ChanIndex
  Channel index for PTP measurment

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed (buffer full)

-----------------------------------------------------------------------------
*/
int SC_TimestampPairToServo(
  UINT8 b_ChanIndex,
	t_timestampPairType* ps_timestampPair)
{
  INT64   ll_nsecRes;
  FIFO_ELEMENT_STRUCT s_ptpElement;
  UINT8 b_servoIndex;

	/* check channel number */
	if(b_ChanIndex >= GetNumberOfChan())
	{
		return -1;
	}

  /* convert to servo index */
  b_servoIndex = UserChan2ServoIndex(b_ChanIndex);

  if(b_servoIndex == 0xFF)
    return -1;

   s_ptpElement.master_sec = ps_timestampPair->s_masterTS.u48_sec; 
   s_ptpElement.master_nsec = ps_timestampPair->s_masterTS.dw_nsec; 

   s_ptpElement.client_sec = ps_timestampPair->s_slaveTS.u48_sec; 
   s_ptpElement.client_nsec = ps_timestampPair->s_slaveTS.dw_nsec; 

   s_ptpElement.seq_num = ps_timestampPair->w_seqNum;
/* calculate offset t(client) - time(master) */
   ll_nsecRes = (s_ptpElement.client_sec - s_ptpElement.master_sec) * 1000000000 +
                (s_ptpElement.client_nsec - s_ptpElement.master_nsec);

   s_ptpElement.offset = ll_nsecRes;

   switch(ps_timestampPair->e_pairDir)
   {
   case e_SYNC_PAIR:
     s_ptpElement.offset = -s_ptpElement.offset;
      Transfer_PTP_data_to_servo(0, &s_ptpElement);
      break;
   case e_DELAY_PAIR:
//   s_ptpElement.offset = -s_ptpElement.offset;
     Transfer_PTP_data_to_servo(1, &s_ptpElement);
      break;
   default:
      break;
   }
   
   return 0;
}



/*
----------------------------------------------------------------------------
                                ServoIndextoPhasorArrayIndex()

Description:
This function will return the array index for any given channel based inthe
channel assignments at start-up. 

Parameters:

Inputs
	chan_index
	channel index assigned during configuration

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed (buffer full)

-----------------------------------------------------------------------------
*/

int ServoIndextoPhasorArrayIndex(UINT8 chan_index, UINT8 *array_index)
{
/* Just subtract off the PTP array to get the new index number */
	*array_index = chan_index-SERVO_CHAN_GPS1;
	return 0;
}
/*
----------------------------------------------------------------------------
                                SC_TimeMeasToServo()

Description:
This function will transfer either a phasor measurement to the servo input 
queue.

Parameters:

Inputs
	b_ChanIndex
	Channel index assigned during configuration
	
	l_timeMeasNs
	Measurement in nanoseconds

   t_ptpTimeStampType s_currentTime
   This is the time reported from the phasor source when the phasor 
   measurement was made.  Time is in TAI time. Current implementation
   does not use the nanoseconds field.  If pointer is null, then no
   time is available.
   
Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed (buffer full)

-----------------------------------------------------------------------------
*/
#if NO_PHY == 0
#define PHY_ROLLOVER_THRES 800000000
extern int SC_TimeMeasToServo(
	UINT8 b_ChanIndex, 
	INT32 l_timeMeasNs,
   t_ptpTimeStampType *s_currentTime
)
{
	INT32 best_offset;
	UINT8 array_index;
	int  error = 0;
	UINT8 next_in_index, inq_index;	
   UINT8 b_servoIndex;
   INT32 delta;
//   t_ptpTimeStampType  s_sysTime;
//   INT16 w_utcOffset;

	/* check channel number */
	if(b_ChanIndex >= GetNumberOfChan())
	{
		return -2;
	}

  /* convert to servo index */
  b_servoIndex = UserChan2ServoIndex(b_ChanIndex);

  if(b_servoIndex == 0xFF)
    return -3;

	ServoIndextoPhasorArrayIndex(b_servoIndex, &array_index);
//	printf("iinput: %d detilt %le\n", l_timeMeasNs, detilt_correction_phy[array_index]);

//	_int_disable();
	inq_index = Phasor_FIFO_Queue[array_index].inq_index;
   next_in_index = (inq_index + 1) % FIFO_PHASOR_QUEUE_BUF_SIZE;
	
/* check to see if queue is full */
	if(next_in_index != Phasor_FIFO_Queue[array_index].out_index)
	{
/* enter next element in to queue with detilt*/
      if(s_currentTime)
   	{
      	Phasor_FIFO_Queue[array_index].time[inq_index] = (UINT32)s_currentTime->u48_sec;
      }

		Phasor_FIFO_Queue[array_index].offset[inq_index] = l_timeMeasNs + (long int)detilt_correction_phy[array_index];		
		Phasor_FIFO_Queue[array_index].inq_index = next_in_index;

			delta = l_timeMeasNs - Phasor_FIFO_Queue[array_index].prev_offset;

         Phasor_FIFO_Queue[array_index].prev_offset = l_timeMeasNs;

/* save latest GPS phasor to be used in TS calculation */
//#ifdef GPS_BUILD
         if((b_servoIndex == RM_GPS_1) && (rms.time_source[0] == RM_GPS_1) && s_currentTime)
         {
            ll_curr_gps_time = s_currentTime->u48_sec;
            Check_and_Sync_Using_GPS();
         }
         else if((b_servoIndex == RM_GPS_2) && (rms.time_source[0] == RM_GPS_2) && s_currentTime)//jiwang
         {
            ll_curr_gps_time = s_currentTime->u48_sec;
            Check_and_Sync_Using_GPS();
         }
         else if((b_servoIndex == RM_GPS_3) && (rms.time_source[0] == RM_GPS_3) && s_currentTime)//jiwang
         {
            ll_curr_gps_time = s_currentTime->u48_sec;
            Check_and_Sync_Using_GPS();
         }
         else if((b_servoIndex == RM_GPS_4) && (rms.time_source[0] == RM_GPS_4) && s_currentTime)//jiwang
         {
            ll_curr_gps_time = s_currentTime->u48_sec;
            Check_and_Sync_Using_GPS();
         }
         else if((b_servoIndex == RM_RED) && (rms.time_source[0] == RM_RED) && s_currentTime)
         {
            ll_curr_red_time = s_currentTime->u48_sec;
            Check_and_Sync_Using_RED();
         }
//#endif
         debug_printf(PHASOR_PROBE_PRT, "chan: %d offset: %d delta %d\n", 
					(int)b_ChanIndex, 
					l_timeMeasNs,
					(int)delta);

	}
	else
	{
		error = -1;
	}
//	_int_enable();
	
	return error;
}	

static INT32 Pick_Best_Offset_Mod_Sec(INT32 input, INT32 prev_offset)
{
	INT32	offset[3];
	INT32 best_offset;
	INT32 min, i;

/* We take a look to see if we should rollover the offset number */
/* first we take a look at the 3 possible answers -1sec, 0sec, or +1sec we
* should pick the one closest to the previous offset number */
	offset[0] = input;
	offset[1] = input - SECOND_IN_NS;
	offset[2] = input + SECOND_IN_NS;

/* out of the 3 possible values, pick the on closest to previous offset */
	min = 0;
	for(i=1;i<3;i++)
	{
		if(abs(offset[i]-prev_offset) < abs(offset[min]-prev_offset))
		{
			min = i;
		}
	}


/* now rollover if greater than threshold */
	if(offset[min] > PHY_ROLLOVER_THRES)
	{
		best_offset = offset[min] - SECOND_IN_NS;
	}
	else if(offset[min] < -PHY_ROLLOVER_THRES)
	{
		best_offset = offset[min] + SECOND_IN_NS;
	}
	else
	{
		best_offset = offset[min];
	}
  //printf("offset %d offset1 %d offset2 %d prev_offset %d best %d bindex %d\n", offset[0],  offset[1], offset[2], prev_offset, best_offset, min);

	return best_offset;
}

#endif
void SetTimestampThreshold(int threshold)
{
	max_timestamp_threshold = threshold;
}


void Set_Timestamp_Offset(INT64 ll_ts_offset)
{
   ll_timestamp_offset = ll_ts_offset;
}

#define NS_IN_WEEK (((UINT64)86400 * (UINT64)7) * (UINT64)SECOND_IN_NS)
/* (TAI)1970-Jan-1 and (GPS)1980-Jan-8 - 10 year + 2 leap years + 7 days = 3657 days */
/* offset 19 seconds between GPS and TAI */
#define TAI_GPS_OFFSET_NS (((UINT64)(3657 * 86400) + (UINT64)19) * (UINT64)SECOND_IN_NS)


void Sync_Using_PTP(void)
{
   sync_to_ptp_now = TRUE;
}

#if NO_PHY == 0
void Sync_Using_GPS(void)
{
   sync_to_gps_now = TRUE;
}
void Sync_Using_RED(void)
{
   sync_to_red_now = TRUE;
}
int32_t i_fpgaFd = -1;
void Check_and_Sync_Using_GPS(void)
{
   int failed;
   t_ptpTimeStampType s_sysTime;
   INT16 i_localUtcOffset = 0;
   t_ptpTimeStampType  s_oldsysTime;
   INT16 w_oldutcOffset;
   static int counter = 0;
   UINT8 curr_chan;
   UINT8 servo_chan;
	static int gps_ready = 0;
	static int gps_state_change = 0;
	uint8_t reg_val;

   curr_chan = GetCurrChan(FALSE);
   servo_chan = UserChan2ServoIndex(curr_chan);
//   printf("curr_chan %d servo_chan %d\n", (int)curr_chan, (int)servo_chan);

/* if current time channel isn't redundant then skip */ 
   if(servo_chan != RM_GPS_1 || servo_chan != RM_GPS_2 || servo_chan != RM_GPS_3 || servo_chan != RM_GPS_4)//jiwang
      return;

   s_sysTime.u48_sec = ll_curr_gps_time;
   s_sysTime.dw_nsec = 0;

   SC_SystemTime(&s_oldsysTime,&w_oldutcOffset,e_PARAM_GET);
#define GPS_TIMELINE_CHANGE_DELAY 5
	i_localUtcOffset = w_oldutcOffset;

   if(s_oldsysTime.dw_nsec > 500000000)
   {
      s_oldsysTime.u48_sec++;
   }

   if (i_fpgaFd <= 0){
      i_fpgaFd = open("/dev/fpga", O_RDWR);
      if (i_fpgaFd < 0)
      {
         return;
      }
	}
	else {
		ioctl(i_fpgaFd, FPGA_RD_SYS_CTRL, (uint32_t)&reg_val);
		if((reg_val & FPGA_GPS_LOCK_FLAG) && (gps_ready == 0)){
			if(counter++ > GPS_TIMELINE_CHANGE_DELAY){
				gps_ready = 1;
				gps_state_change = 1;
			}
		}
		else if(((reg_val & FPGA_GPS_LOCK_FLAG) == 0) && (gps_ready == 1)){
			gps_ready = 0;
			gps_state_change = 1;
		}
		else{
			gps_state_change = 0;
		}
	}
#if 0
   if((s_oldsysTime.u48_sec != ll_curr_gps_time) || sync_to_gps_now)
#endif
	if(((gps_ready == 1) && (gps_state_change == 1)) || sync_to_gps_now) 
   {
 
      syslog(LOG_BSS|LOG_DEBUG, "!!GPS TS: %ld local TS: %ld %ld", (UINT32)ll_curr_gps_time, (UINT32)s_oldsysTime.u48_sec, (UINT32)s_oldsysTime.dw_nsec);
      if((counter++ > GPS_TIMELINE_CHANGE_DELAY) || (sync_to_gps_now))
      {
         time_was_just_adjusted = TRUE;
         debug_printf(GEORGE_PTP_PRT, "!!!!! reset TS with GPS time: %lld\n", ll_curr_gps_time);
         counter = 0;
         if((failed = SC_SystemTime(&s_sysTime, &i_localUtcOffset, e_PARAM_SET)))
         {
            debug_printf(UNMASK_PRT, "Error while running SC_SystemTime()\n");
         }
         else
         {
            event_printf(e_TIMELINE_CHANGED, e_EVENT_NOTC, e_EVENT_TRANSIENT,
                  sc_alarm_lst[e_TIMELINE_CHANGED].dsc_str, (UINT32)(s_oldsysTime.u48_sec));
            debug_printf(GEORGE_PTP_PRT,  "!!!Sync_Using_GPS_now() - GPS set phase - Transient\n");
         }             
      }
     sync_to_gps_now = FALSE; 
  }
   else
   {
//      debug_printf(GEORGE_PTP_PRT, "!!GPS TS: %ld local TS: %ld %dl\n", (UINT32)ll_curr_gps_time, (UINT32)s_oldsysTime.u48_sec, (UINT32)s_oldsysTime.dw_nsec);
      counter = 0;
   }

 
   return;
}
void Check_and_Sync_Using_RED(void)
{
   int failed;
   t_ptpTimeStampType s_sysTime;
   INT16 i_localUtcOffset;
   t_ptpTimeStampType  s_oldsysTime;
   INT16 w_oldutcOffset;
   static int counter = 0;
   UINT8 curr_chan;
   UINT8 servo_chan;

   curr_chan = GetCurrChan(FALSE);
   servo_chan = UserChan2ServoIndex(curr_chan);
//   printf("curr_chan %d servo_chan %d\n", (int)curr_chan, (int)servo_chan);

/* if current time channel isn't redundant then skip */
   if(servo_chan != RM_RED)
      return;

#if 1
   s_sysTime.u48_sec = ll_curr_red_time;
   s_sysTime.dw_nsec = 0;

   SC_SystemTime(&s_oldsysTime,&w_oldutcOffset,e_PARAM_GET);
#define GPS_TIMELINE_CHANGE_DELAY 5


   if(s_oldsysTime.dw_nsec > 500000000)
   {
      s_oldsysTime.u48_sec++;
   }

   if((s_oldsysTime.u48_sec != ll_curr_red_time) || sync_to_red_now)
   {
 
      debug_printf(GEORGE_PTP_PRT, "!!Redundant TS: %ld local TS: %ld %dl\n", (UINT32)ll_curr_red_time, (UINT32)s_oldsysTime.u48_sec, (UINT32)s_oldsysTime.dw_nsec);
      if((counter++ > GPS_TIMELINE_CHANGE_DELAY) || (sync_to_red_now))
      {
         time_was_just_adjusted = TRUE;
         debug_printf(GEORGE_PTP_PRT, "!!!!! reset TS with Redundant time: %lld\n", ll_curr_red_time);
         counter = 0;
         if((failed = SC_SystemTime(&s_sysTime, &i_localUtcOffset, e_PARAM_SET)))
         {
            debug_printf(UNMASK_PRT, "Error while running SC_SystemTime()\n");
         }
         else
         {
            event_printf(e_TIMELINE_CHANGED, e_EVENT_NOTC, e_EVENT_TRANSIENT,
                  sc_alarm_lst[e_TIMELINE_CHANGED].dsc_str, (UINT32)(s_oldsysTime.u48_sec));
            debug_printf(GEORGE_PTP_PRT,  "!!!Sync_Using_Red_now() - Redundant set phase - Transient\n");
         }             
      }
     sync_to_red_now = FALSE; 
  }
   else
   {
      counter = 0;
   }

#endif 
   return;
}
#endif
BOOLEAN Was_Time_Adjusted(void)
{
   return time_was_just_adjusted;
}

void Set_Time_Adjusted_Flag(void)
{
   time_was_just_adjusted = TRUE;
}

void Clear_Time_Adjusted_Flag(void)
{
   time_was_just_adjusted = FALSE;
}

INT32 Get_RTD_1sec_min(void)
{
   if(current_rtd_1sec_min < 0)
      return 0;
   return current_rtd_1sec_min;
}

BOOLEAN Is_Sync_LOS(void)
{
   return sync_was_los;
}

/*
----------------------------------------------------------------------------
                                SC_SyncPhaseThreshold()


Description: This function is called by the user to set or get the threshold
at which the phase will automatically jam to the current reference.


Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the sync threshold
		e_PARAM_SET - Set the sync threshold

	UINT32 *pdw_syncThresholdNs
		Address to set or get sync threshold in nanoseconds

Outputs:
	UINT32 *pdw_syncThresholdNs
		sync threshold in nanoseconds


Return value:
    0 - successful
   -1 - function failed

-----------------------------------------------------------------------------
*/
int SC_SyncPhaseThreshold(
  t_paramOperEnum b_operation,
  UINT32 *pdw_syncThresholdNs
)
{
/* if redundany channel not configured then return -1 */
//   if(Get_RedChan() == -1)
//      return -1;

   if (b_operation == e_PARAM_SET)
   {
/* check values */
      if((*pdw_syncThresholdNs < MAX_SYNC_THRESHOLD_NS)  && (*pdw_syncThresholdNs > MIN_SYNC_THRESHOLD_NS))
      {
         dw_syncThresholdNs = *pdw_syncThresholdNs;
         Jam_Never(FALSE);
      }
      else if(*pdw_syncThresholdNs == 1000000000)
      {
         Jam_Never(TRUE);
      }
      else
      {
         return -1;
      }  
   }
   else if (b_operation == e_PARAM_GET)
   {
      *pdw_syncThresholdNs = dw_syncThresholdNs;
   }
   else
   {
         return -1;
   }

   return 0;
}

/*
----------------------------------------------------------------------------
                                SC_SyncPhaseNow()


Description: This function is called by the user to immediately jam (sync) to 
the currently selected timing reference phase.


Parameters:

Inputs
	None

Outputs:
	None


Return value:
    0 - successful
   -1 - function failed
   -2 - function failed (no time reference)
   -3 - function failed (no jam necessary)

-----------------------------------------------------------------------------
*/
int SC_SyncPhaseNow(void)
{
   int ret = 0;

   ret = Jam_Now();
   return ret;
}

int Get_Load_Mode(UINT8 b_servoIndex, UINT16 *w_LoadMode)
{
	if(b_servoIndex != RM_GM_1)
		return -1;

	*w_LoadMode = LoadState;

	return 0;
}
