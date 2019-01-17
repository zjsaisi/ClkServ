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

FILE NAME    : sc_ptp_servo.h

AUTHOR       : Ken Ho

DESCRIPTION  : 

This header file contains the API definitions for SoftClient.

Revision control header:
$Id: CLK/sc_ptp_servo.h 1.20 2012/02/02 13:51:23PST Jining Yang (jyang) Exp  $

******************************************************************************
*/

/* Prevent file from being included multiple times. */
#ifndef H_SC_PTP_SERVO_h
#define H_SC_PTP_SERVO_h

#ifdef __cplusplus
extern "C"
{
#endif
#include "datatypes.h"
#include "sc_chan.h"


#ifndef ZEROF
#define ZEROF (0.0)
#endif

#define PTP_SYNC_TRANSFER_QUEUE  0
#define PTP_DELAY_TRANSFER_QUEUE 1

#define FIFO_BUF_SIZE 128//was (4096) use 128 for all new builds

//#define FIFO_QUEUE_BUF_SIZE (128 * 1 * 2) /* (128 pkts/s * 3 buffers * 5 seconds) */
#define FIFO_QUEUE_BUF_SIZE (128 + 64) /* (128 pkts/s * 3 buffers * 5 seconds) */
#define FIFO_PHASOR_QUEUE_BUF_SIZE (6) /* */

#define MAX_LOS_CNTR 255

/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
typedef struct
{
  BOOLEAN   o_enableChan;   /* 1 = channel enabled, 0 = not enabled   */
  UINT8     b_selectedFreq; /* 1 = channel selected, 0 = not selected */
  UINT8     b_selectedTime; /* 1 = channel selected, 0 = not selected */
  BOOLEAN   o_valid;        /* 1 = channel measurement valid, 0 = not valid */
  UINT8		  b_userChanIdx;  /* user channel index number                    */
  INT16     w_measRate;     /* positive value in Hz, negative in seconds    */
} SC_t_servoChanSelectType;  

typedef struct 
{
  uint_16 fifo_in_index;   /* index to the last entry that was entered */
  uint_16 fifo_out_index;  /* index to the oldest unconsumed array element */
	uint_8  is_offset_by_180deg;
	uint_8  is_offset_flag;
	int_32  offset[FIFO_BUF_SIZE+8]; /* offset in ns */
} FIFO_STRUCT;

typedef struct 
{
    uint_32 client_sec;
	  uint_32 master_sec;
    uint_32 client_nsec;
    uint_32 master_nsec;
    int_32  offset;
    uint_16 seq_num;
    uint_16 flag;
} FIFO_ELEMENT_STRUCT;

typedef struct 
{
	uint_16 out_index;  /* oldest unread data element in queue          */
	uint_16 inq_index;   /* where to insert newest data element in queue */
	FIFO_ELEMENT_STRUCT  fifo_element[FIFO_QUEUE_BUF_SIZE+2];
	uint_8 queue_index [FIFO_QUEUE_BUF_SIZE+2];
} FIFO_QUEUE_STRUCT;

typedef struct 
{
	uint_16 out_index;  /* oldest unread data element in queue          */
	uint_16 inq_index;   /* where to insert newest data element in queue */
	uint_16 los_cntr;    /* will be set to 0, if measurement available, will count up if not */
   INT32  prev_offset;
	INT32  offset[FIFO_PHASOR_QUEUE_BUF_SIZE+1];
	UINT32 time[FIFO_PHASOR_QUEUE_BUF_SIZE+1];
} PHASOR_FIFO_QUEUE_STRUCT;

typedef enum
{
  SERVO_CHAN_PTP1      = 0,
  SERVO_CHAN_PTP2,
	SERVO_CHAN_GPS1,
	SERVO_CHAN_GPS2,
	SERVO_CHAN_GPS3,
	SERVO_CHAN_GPS4,
	SERVO_CHAN_SYNCE1,
	SERVO_CHAN_SYNCE2,
	SERVO_CHAN_RED,
	NUM_OF_SERVO_CHAN
} 	SC_servoChanEnum;

typedef struct
{
  SC_t_servoChanSelectType s_servoChanSelect[NUM_OF_SERVO_CHAN];
} SC_t_servoChanSelectTableType;  


typedef struct
{
  UINT8 b_accepted[NUM_OF_SERVO_CHAN]; /* 1 = servo will accept, 0 = servo will reject */
} t_servoChanAcceptType; 

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

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
extern int Get_Freq_Corr(
   FLOAT32   *pf_freqCorr,          /* OCXO frequency offset sampled hourly    */ 
   UINT32    *pdw_freqCorrTimeDelta /* timestamp for hourly frequency offset delta (s) */
);
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
extern int Get_Freq_Corr_Factory(
   FLOAT32  *pf_freqCalFactory,     /* OCXO frequency offset at factory        */
   UINT64   *ps_freqCalFactoryTime /* timestamp for factory frequency offset  */
);
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
	
   t_ptpTimeStampType *ps_freqCorrTime
   The timestamp when the correction was recorded

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int Get_Bridge_Time(UINT32 *pdd_bridgeTime);
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
extern int Get_Access_Transport(t_ptpAccessTransportEnum *pe_ptpTransport);
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
extern int Get_LO_Type(t_loTypeEnum *pe_loType);
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
//extern int Try_SC_SystemTimeSet(
//	PTP_t_TmStmp  *ps_sysTimeTry,
//	INT16          i_utcOffset 
//);
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
extern void SC_goodPacketRstCounter(void);

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
extern BOOLEAN Is_PTP_Mode_Standard(void);

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
extern BOOLEAN Is_PTP_Mode_Inband(void);

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
extern void Transfer_PTP_data_now(
   FIFO_STRUCT *PTP_Fifo_Sync, 
   FIFO_STRUCT *PTP_Fifo_Delay, 
   UINT16 *num_sync_pkts,
   UINT16 *num_delay_pkts
);

extern void Transfer_Phasor_data_now(
   SC_servoChanEnum servoChan,
   FIFO_STRUCT *Phasor_Fifo,
   UINT16 *measCnt
);


/*
----------------------------------------------------------------------------
                                Get_RTD_1SEC_MIN

This function returns the RTD (Round Trip Delay) based on the last 1 second
samples.

Parameters:
IN:
   N/A
OUT:
	N/A


Return value:
	INT32 rtd value in nanoseconds, if 0 then no rtd for this second

-----------------------------------------------------------------------------
*/ 
extern INT32 Get_RTD_1sec_min(void);

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

extern void CLK_Init_PTP_Tranfer_Queue(void);

/*
----------------------------------------------------------------------------
                                Is_ChanConfigured()

Description:
This function will return if the channel comfiguration is complete.

Parameters:

Inputs
        None

Outputs:
        None

Return value:
        TRUE - Channel comfiguration is complete.

-----------------------------------------------------------------------------
*/
extern BOOLEAN Is_ChanConfigured(void);

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
extern UINT8 Transfer_PTP_data_to_servo(UINT8 queue_index, FIFO_ELEMENT_STRUCT *PTP_element);

extern UINT8 UserChan2ServoIndex(UINT8 b_userChan);
extern UINT8 ServoIndex2UserChan(UINT8 b_servoIndex);
extern UINT8 GetNumberOfChan(void);



// extern int Get_Sync_Rate(INT16 *rate);
/* function to servo */
extern int Update_chanSelect(void);
extern int Get_servoAccept(t_servoChanAcceptType *p_servoChanAccept);
extern int Get_servoChanSelectRate(UINT16 chan_index, INT16 *rate);

/* temporary control over George's stuff */
extern int Get_servoChanSelect(SC_t_servoChanSelectTableType *p_servoChanSelectTable);
extern int Set_servoChanSelect(SC_t_servoChanSelectTableType *p_servoChanSelectTable);
extern int Get_servoChanSelectRate(UINT16 chan_index, INT16 *rate);
extern void Set_servoChanSelectValid(UINT16 chan_index, INT8 los);
extern void Print_Servo_Chan_Info(void);
extern void Sync_Using_GPS(void);
extern void Sync_Using_RED(void);
extern void Sync_Using_PTP(void);
extern BOOLEAN Was_Time_Adjusted(void);
extern void Clear_Time_Adjusted_Flag(void);
extern void Set_Time_Adjusted_Flag(void);
extern INT32 Get_Current_RTD(void);
extern BOOLEAN Is_Sync_LOS(void);
extern int GetCurrChan(BOOLEAN is_freq);
extern int Jam_Now(void);
extern int Get_RedChan(void);
extern int Get_Load_Mode(UINT8 b_servoIndex, UINT16 *w_LoadMode);

#ifdef __cplusplus
}
#endif

#endif /*  H_SC_PTP_SERVO_h */
