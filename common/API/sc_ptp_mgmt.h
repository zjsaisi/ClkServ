/*****************************************************************************
*                                                                            *
*   Copyright (C) 2010 Symmetricom, Inc., all rights reserved                *
*                                                                            *
*   This software contains proprietary and confidential information of       *
*   Symmetricom.                                                             *
*   It may not be reproduced, used, or disclosed to others for any purpose   *
*   without the written authorization of Symmetricom.                        *
*                                                                            *
******************************************************************************

FILE NAME    : sc_ptp_mgmt.h

AUTHOR       : Jining Yang

DESCRIPTION  :

The three enumerated types are shared from tp500 ptpmgt.h file. Thess
definitions are common to tp5k grandmaster.

Revision control header:
$Id: API/sc_ptp_mgmt.h 1.1 2010/02/08 22:51:59PST jyang Exp  $

******************************************************************************
*/

#ifndef _SC_PTP_MGMT_H
#define _SC_PTP_MGMT_H

/*
 * This enumerate type defines PTP private status
 */
typedef enum
{
    PS_TP500PERSONALITY		= 0,  /* not provided to end customer, used to identify as symm msg */
    PS_FLLSTATE			= 1,  /* row 1 */
    PS_FLLSTATEDURATION		= 2,
    PS_SELECTEDMASTERIP		= 3,  /* row 30, this needs state defn for none, GM1, GM2 */
    PS_FORWARDWEIGHT		= 4,  /* row 3 */
    PS_FORWARDTRANS900		= 5,
    PS_FORWARDTRANS3600		= 6,
    PS_FORWARDTRANSUSED		= 7,
    PS_FORWARDOPMINTDEV		= 8,
    PS_FORWARDMINCLUWIDTH	= 9,
    PS_FORWARDMODEWIDTH		= 10,  /* row  9 */
    PS_FORWARDGMSYNCPSEC	= 11,  /* row 27 */
    PS_FORWARDACCGMSYNCPSEC	= 12,  /* row 28 */
    PS_REVERSEDWEIGHT		= 13,  /* row 10 */
    PS_REVERSEDTRANS900		= 14,
    PS_REVERSEDTRANS3600	= 15,
    PS_REVERSEDTRANSUSED	= 16,
    PS_REVERSEDOPMINTDEV	= 17,
    PS_REVERSEDMINCLUWIDTH	= 18,
    PS_REVERSEDMODEWIDTH	= 19,  /* row 16 */
    PS_REVERSEDGMSYNCPSEC	= 20,  /* row 29 this is the reverse delay rate for the reference GM */
    PS_REVERSEDACCGMSYNCPSEC	= 21,  /* not used */
    PS_OUTPUTTDEVESTIMATE	= 22,  /* row 19 */
    PS_FREQCORRECT		= 23,  /* row 17 */
    PS_PHASECORRECT		= 24,  /* row 18 */
    PS_RESIDUALPHASEERROR	= 25,  /* row 20 */
    PS_MINTIMEDISPERSION	= 26,  /* row 21 */
    PS_MASTERFLOWSTATE		= 27,  /* row 31 */
    PS_ACCMASTERFLOWSTATE	= 28,
    PS_MASTERCLOCKID		= 29,
    PS_ACCMASTERCLOCKID		= 30,
    PS_LASTUPGRADESTATUS	= 31,  /* row 35 */
    PS_OP_TEMP_MAX		= 32,  /* row 22, all of the temp vars are PS_UINT16_BASE_1E_2 */
    PS_OP_TEMP_MIN		= 33,
    PS_OP_TEMP_AVG		= 34,
    PS_TEMP_STABILITY_5MIN	= 35,
    PS_TEMP_STABILITY_60MIN	= 36,  /* row 26 */
    PS_IPDV_OBS_INTERVAL	= 37,  /* row 36, value in seconds PS_UINT16 */
    PS_IPDV_THRESHOLD		= 38,  /* value enter in ns, present in us, PS_UINT32_BASE_1E_2 takes it to 8 s */
    PS_VAR_PACING_FACTOR	= 39,  /* power-of-2, so uint8 is fine, controls packets to skip for variation comp */
    PS_FWD_IPDV_PCT_BELOW	= 40,  /* value in 10m%, present in %, PS_UINT16_BASE_1E_2 allows 100.xx */
    PS_FWD_IPDV_MAX		= 41,  /* value in ns, present in us, PS_UINT32_BASE_1E_2 takes it to 8 s */
    PS_FWD_INTERPKT_VAR		= 42,  /* value in ns, present in us, PS_UINT32_BASE_1E_2 takes it to 8 s */
    PS_REV_IPDV_PCT_BELOW	= 43,  /* value in 10m%, present in %, PS_UINT16_BASE_1E_2 allows 100.xx */
    PS_REV_IPDV_MAX		= 44,  /* value in ns, present in us, PS_UINT32_BASE_1E_2 takes it to 8 s */
    PS_REV_INTERPKT_VAR		= 45,  /* value in ns, present in us, PS_UINT32_BASE_1E_2 takes it to 8 s */
    PS_CURRENT_TIME		= 46,  /* row 0 , this be PS_ASCII_STRING */
    PS_FORWARDOPMAFE		= 47,  /* forward operation MAFE */
    PS_REVERSEDOPMAFE		= 48,  /* Reversed operation MAFE */
    PS_OUTPUTMDEVESTIMATE	= 49   /* Output MDEV Estimate */
} PS_PS_INDEX;

/*
 * This enumerate type defines FLL state shared between tp5k gm
 */
typedef enum
{
    PTP_FLL_UNKNOWN = 0,	// Unkonw
    PTP_FLL_WARMUP,		// Warming 	
    PTP_FLL_FAST,		// Detilt or Fast Gear
    PTP_FLL_NORMAL,		//Clock Normal or Bridge
    PTP_FLL_BRIDGE,		//Clock Normal with slew counts
    PTP_FLL_HOLD,		// Holdover
    PTP_FLL_NO_SYNC_FLOW,	// No Sync flow
    PTP_FLL_ACQUIRING		// Acquiring
} PTP_Fll_Clock_State;

/*
 * This enumerate type defines the data type shared between tp5k gm
 */
typedef enum  
{
    PS_BINARY_STRING = 0,	// Binary string type
    PS_UINT8,			// Unsigned 8 bits integer
    PS_INT8,			// Signed 8 bits
    PS_UINT16,			// Unsigned 16 bits
    PS_INT16,			// Signed 16 bits	
    PS_UINT16_BASE_1E_1,	// Divided by 10
    PS_UINT16_BASE_1E_2,	// Divided by 100
    PS_INT16_BASE_1E_1,		// Divided by 10
    PS_INT16_BASE_1E_2,		// Divided by 100
    PS_UINT32,			// Unsigned 32 bits
    PS_UINT32_BASE_1E_1,	// Divided by 10
    PS_UINT32_BASE_1E_2,	// Divided by 100
    PS_INT32,			// Unsigned 32 bits
    PS_INT32_BASE_1E_1,		// Divided by 10
    PS_INT32_BASE_1E_2,		// Divided by 100
    PS_ASCII_STRING,		// Testing string
    PS_UINT16_BASE_1E_3,	// Divided by 1000
    PS_INT16_BASE_1E_3,		// Divided by 1000
    PS_UINT32_BASE_1E_3,	// Divided by 1000
    PS_INT32_BASE_1E_3		// Divided by 1000
} PTP_PS_DATA_TYPE;

/*
 * This structure defines the convertied status items for transmitting
 * to tp5k gm
 */
typedef struct ptp_mgmt_status_s {
    UINT32 tp500personality;
    UINT32 fllState;
    UINT32 fllStateDuration;
    UINT32 SelecetedMasterIP;

    UINT32 forwardWeight;
    UINT32 forwardTrans900;
    UINT32 forwardTrans3600;
    UINT32 forwardTransUsed;
    UINT32 forwardOperationalMinTdev;
    UINT32 forwardMinClusterWidth;
    UINT32 forwardModeWidth;
    UINT32 forwardMasterSyncPerSec;
    UINT32 forwardAccMasterSyncPerSec;

    UINT32 reverseWeight;
    UINT32 reverseTrans900;
    UINT32 reverseTrans3600;
    UINT32 reverseTransUsed;
    UINT32 reverseOperationalMinTdev;
    UINT32 reverseMinClusterWidth;
    UINT32 reverseModeWidth;
    UINT32 reverseMasterDelayRespPerSec;
    UINT32 reverseAccMasterDelayRespPerSec;

    UINT32 outputTdevEstimate;
    INT32  freqCorrect;
    INT32  phaseCorrect;
    INT32  residualPhaseError;
    UINT32 masterFlowState;
    UINT32 accMasterFlowState;
    UINT8  masterClockId[8];
    UINT8  accMasterClockId[8];
    UINT32 last_upgrade_status;
    UINT32 min_time_dispersion;
    INT32  op_temp_max;
    INT32  op_temp_min;
    INT32  op_temp_avg;
    INT32  temp_stability_5min;
    INT32  temp_stability_60min;
    UINT32 ipdv_obs_interval;
    UINT32 ipdv_threshold;
    UINT32 var_pacing_factor;
    UINT32 forward_ipdv_pct_below;
    UINT32 forward_ipdv_max;
    UINT32 forward_interpkt_var;
    UINT32 reversed_ipdv_pct_below;
    UINT32 reversed_ipdv_max;
    UINT32 reversed_interpkt_var;
    char   current_time[64];
    UINT32 forwardOpMafe;
    UINT32 reverseOpMafe;
    UINT32 outputMdevEstimate;
} ptp_mgmt_status_type;

#endif // _SC_PTP_MGMT_H
