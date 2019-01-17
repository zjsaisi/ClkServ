
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

FILE NAME    : sc_chan.h

AUTHOR       : Daniel Brown

DESCRIPTION  :

Contains constant definitions, type definitions and function prototypes for
channelization associated items for SoftClock.


SC_InitChanConfig()
SC_GetChanConfig()

SC_ChanSSM()
SC_ChanValid()
SC_ChanSwtMode()
SC_ChanSelMode()
SC_SetSelChan()
SC_ChanEnable()
SC_ChanPriority()
SC_ChanAssumeQL()
SC_ChanEnableAssumeQL()


Revision control header:
$Id: sc_chan.h 1.33 2012/02/02 14:55:18PST Jining Yang (jyang) Exp  $

******************************************************************************
*/


/* Prevent file from being included multiple times. */
#ifndef H_SC_CHAN_H
#define H_SC_CHAN_H


/*****************************************************************************/
/*                       ***INCLUDES Definitions***                          */
/*  Nested includes should not be used as a general practice.  If there is a */
/*  good reason to include an include within an include, do so here. 	     */
/*****************************************************************************/

/* file included becomes one of renamed partition build specific header files
   at build time - established by build environment setup scripts */
#ifdef configFileName
#include configFileName
#endif
#include "sc_servo_api.h"

/*****************************************************************************/
/*                       ***CONSTANT Definitions***                          */
/*  This section should be used to define constants (#defines) and macros    */
/*  that can be used by any module that includes this file.                  */
/*****************************************************************************/

#define k_CHAN_MIN_PRIO    0     /* minimum priority number */
#define k_CHAN_MAX_PRIO    10    /* maximum priority number */
#define k_CHAN_MIN_QL      1     /* minimum quality level number */
#define k_CHAN_MAX_QL      16    /* maximum quality level number */

/*****************************************************************************/
/*                       ***Data Type Specifications***                      */
/*  This section should be used to specify data types including structures,  */
/*  enumerations, unions, and redefining individual data types.              */
/*****************************************************************************/

typedef enum
{
   e_SC_CHAN_TYPE_PTP       = 0, /* PTP Channel type */
   e_SC_CHAN_TYPE_GPS       = 1, /* GPS Channel type */
   e_SC_CHAN_TYPE_SE        = 2, /* Sync-E Channel type */
   e_SC_CHAN_TYPE_E1        = 3, /* E1 Channel type */
   e_SC_CHAN_TYPE_T1        = 4, /* T1 Channel type */   
   e_SC_CHAN_TYPE_REDUNDANT = 5, /* Redundant Channel type */
   e_SC_CHAN_TYPE_FREQ_ONLY = 6, /* Generic Frequency only Channel type */
   e_SC_CHAN_TYPE_RESERVED /* Reserved value. TODO Check how is used. */
} SC_t_Chan_Type;

typedef enum
{
   e_SC_ACTIVE                = 0, /* this SC is active */
   e_SC_STANDBY               = 1, /* this SC is standby */
   e_SC_TRANSITION_TO_ACTIVE  = 2, /* this SC is becoming active */
   e_SC_TRANSITION_TO_STANDBY = 3 /* this SC is becoming standby */
} SC_t_activeStandbyEnum;


typedef enum
{
   e_SC_CHAN_SEL_PRIO   = 0, /* Channel selection performed first by priority level */
   e_SC_CHAN_SEL_QL     = 1, /* Channel selection performed first by global QL */
} SC_t_selModeEnum;

typedef enum
{
	e_SC_SWTMODE_AR      = 0,
	e_SC_SWTMODE_AS,
	e_SC_SWTMODE_OFF
} SC_t_swtModeEnum;

typedef enum
{
	e_SC_CHAN_STATUS_OK      = 0, /* Channel status is qualified */
	e_SC_CHAN_STATUS_FLT,         /* Channel status is fault     */
	e_SC_CHAN_STATUS_DIS          /* Channel status is disabled  */
} SC_t_chanStatusEnum;

/* Channel status structure */
typedef struct
{
   SC_t_chanStatusEnum  e_chanFreqStatus;  /* status of frequency channel */
   SC_t_chanStatusEnum  e_chanTimeStatus;  /* status of time channel      */
   UINT8                b_freqWeight;      /* percentage contribution to frequency correction */
   UINT8                b_timeWeight;      /* percentage contribution to time correction      */
   UINT8                b_freqQL;          /* frequency quality level currently associated with this channel */
   UINT8                b_timeQL;          /* time quality level currently associated with this channel */
   UINT8                b_qlReadExternally;/* TRUE = value came from externally supplied input     */
   UINT32               l_faultMap;        /* 0x1 performance fault exists        */
                                           /* 0x2 externally declared fault       */
                                           /* 0x4 no measurement data (LOS) fault */
} SC_t_chanStatus;

/* Channel config structure */
typedef struct
{
   /*********** Channel Setup Information ***********/

   SC_t_Chan_Type       e_ChanType;             /* Channel Type */
   UINT8                b_ChanFreqPrio;         /* Range 0..10, 0=monitor only */
   BOOLEAN              o_ChanFreqEnabled;      /* Freq mode enabled */
   UINT8                b_ChanTimePrio;         /* Range 0..10, 0=monitor only */
   BOOLEAN              o_ChanTimeEnabled;      /* Time mode enabled */
   BOOLEAN              o_ChanAssumedQLenabled; /* Assumed QL TRUE | FALSE */
   UINT8                b_ChanFreqAssumedQL;    /* Range 1..16 */
   UINT8                b_ChanTimeAssumedQL;    /* Range 1..16 */
   UINT8                reserved[1];            /* Used for 32-bit word alignment */

   /*********** Measurement Information ***********/

   INT32                l_MeasRate;              /* When positive in frequency (Hz),
                                                 * negative in period (seconds)   */

} SC_t_ChanConfig;

/*****************************************************************************/
/*                       ***Public Data Section***                           */
/*  Global variables, if used, should be defined in a ".c" source file.      */
/*  The scope of a global may be extended to other modules by declaring it as*/
/*  as an extern here. 			                                     */
/*****************************************************************************/

/*****************************************************************************/
/*                       ***Function Prototype Section***                    */
/*  This section to be used to prototype public functions.                   */
/*                                                                           */
/*  Descriptions of parameters and return code should be given for each      */
/*  function.                                                                */
/*****************************************************************************/
/*
----------------------------------------------------------------------------
                                SC_InitChanConfig()
Description:
This function sets the channel configuration parameters. The parameters will
be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
        as_ChanConfig[]
            Array of channel configuration data structures
        e_ChanSelMode
            Channel selection mode
        e_ChanSwtMode
            Channel switch mode
        b_ChanCount
            Number of channels to be configured

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed
        -2: already running
        -3: invalid value in as_ChanConfig[]
        -4: channel type not supported in this product
        -5: channel type does not support time
        -6: too many channels configured
        -7: too many channels of specific type configured
        -8: invalid channel selection mode
        -9: invalid channel switch mode

-----------------------------------------------------------------------------
*/
int SC_InitChanConfig(
  SC_t_ChanConfig as_ChanConfig[],
  SC_t_selModeEnum e_ChanSelMode,
  SC_t_swtModeEnum e_ChanSwtMode,
  UINT8 b_ChanCount
);


/*
----------------------------------------------------------------------------
                                SC_GetChanConfig()
Description:
This function gets the channel configuration parameters. The parameters will
be used when SC_InitConfigComplete() is called.
If a parameter is NULL no value is returned for that parameter.

Parameters:

Inputs
        as_ChanConfig[]
            Array of channel configuration data structures
        b_ChanCount
            Number of channels to be configured

Outputs:
        as_ChanConfig
        e_ChanSelMode
	e_ChanSwtMode
        b_ChanCount

Return value:
         0: function succeeded
        -1: function failed
        -2: SC_InitChanConfig() has not been run yet

-----------------------------------------------------------------------------
*/
int SC_GetChanConfig(
  SC_t_ChanConfig as_ChanConfig[],
  SC_t_selModeEnum* e_ChanSelMode,
  SC_t_swtModeEnum* e_ChanSwtMode,
  UINT8* b_ChanCount
);

/*
----------------------------------------------------------------------------
                                SC_ChanSSM()


Description: This function is called by the Channel Manager to determine
a channel's SSM.  This function will take the SSM number and the reference
type, and translate to a PQL number that will be used in the channel
selection code.  This function will be written by the customer.

Parameters:

Inputs
	UINT8 b_ChanIndex
	Channel number

	UINT8 *pb_ssm
	Pointer to return SSM data into

  BOOLEAN *po_Valid
	Pointer to return SSM data valid into

Outputs:
	UINT8 *pb_ssm
	SSM level read from channel indexed by b_ChanIndex

  BOOLEAN *po_Valid
  TRUE - SSM value provided in *po_Valid is valid
  FALSE - SSM value provided in *po_Valid is invalid

Return value:
	0 - successful
	-1 - SSM number not available


-----------------------------------------------------------------------------
*/

extern int SC_ChanSSM(UINT8 b_ChanIndex, UINT8 *pb_ssm, BOOLEAN *po_Valid);

/*
----------------------------------------------------------------------------
                                SC_ChanValid()

Description: This function is called by the Channel Manager to determine
if the channel is valid.  This function will be written by the customer.

Parameters:

Inputs
	UINT8 b_ChanIndex
	Channel number to validate

	BOOLEAN *po_valid
	Address to return validation data

Outputs:
	UINT8 *po_valid
	0 = not valid, 1 = valid


Return value:
	0 - successful
	1 - unsuccessful

-----------------------------------------------------------------------------
*/

extern int SC_ChanValid(UINT8 b_ChanIndex, BOOLEAN *po_valid);

/*
----------------------------------------------------------------------------
                                SC_ChanSwtMode()


Description: This function is called by the user to set or get the channel
switch mode.  The values can be AR | AS | OFF.


Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the switch mode
		e_PARAM_SET - Set the switch mode

	SC_t_swtModeEnum *pe_swtMode
		Address to set or get switch mode

Outputs:
	SC_t_swtModeEnum *pe_swtMode
		Returned switch mode enumeration


Return value:
    0 - successful
   -1 - function failed
   -2 - invalid channel switch mode
   -3 - channels not configured yet

-----------------------------------------------------------------------------
*/
extern int SC_ChanSwtMode(
  t_paramOperEnum b_operation,
  SC_t_swtModeEnum *pe_swtMode
);

/*
----------------------------------------------------------------------------
                                SC_ChanSelMode()


Description: This function is called by the user to set or get the channel
select mode.  The values can be PRIORITY or PQL.


Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the channel select mode
		e_PARAM_SET - Set the channel select mode

	SC_t_selModeEnum *pe_selMode
		Address to set or get channel select mode

Outputs:
	SC_t_selModeEnum *pe_selMode
		Returned channel select mode enumeration


Return value:
    0 - successful
   -1 - function failed
   -2 - invalid channel select mode
   -3 - channels not configured yet

-----------------------------------------------------------------------------
*/
extern int SC_ChanSelMode(
  t_paramOperEnum b_operation,
  SC_t_selModeEnum *pe_selMode
);

/*
----------------------------------------------------------------------------
                                SC_SetSelChan()


Description: This function is called by the user to set the selected channel.


Parameters:

Inputs
	BOOLEAN o_isFreq
		TRUE  - entering frequency channel to select
		FALSE - entering time channel to select

	UINT8 b_ChanIndex
		Input channel to select

Outputs:
	None


Return value:
    0 - successful
   -1 - function failed
   -2 - failed - in AR mode
   -3 - channels not configured yet
   -4 - channel type does not support time

-----------------------------------------------------------------------------
*/
extern int SC_SetSelChan(BOOLEAN o_isFreq, UINT8 b_ChanIndex);

/*
----------------------------------------------------------------------------
                                SC_ChanEnable()


Description: This function is called by the user to set or get the channel's
frequency or time enable status.

Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the channel enable
		e_PARAM_SET - Set the channel enable

	UINT8 b_ChanIndex
		index for channel to enable/disable

	BOOLEAN o_isFreq
		TRUE  - enable/disable the frequency channel
		FALSE - enable/disable the time channel

	BOOLEAN *po_enable
		Address to set or get enable/disable status


Outputs:

	BOOLEAN *po_enable
		Returned enable/disable status of channel


Return value:
    0 - successful
   -1 - failed
   -2 - channel type does not support time
   -3 - channels not configured yet

-----------------------------------------------------------------------------
*/
extern int SC_ChanEnable(
  t_paramOperEnum b_operation,
  UINT8 b_ChanIndex,
  BOOLEAN o_isFreq,
  BOOLEAN *po_enable
);

/*
----------------------------------------------------------------------------
                                SC_ChanPriority()


Description: This function is called by the user to set or get the channel's
frequency or time priority value.

Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the channel's time or frequency priority value
		e_PARAM_SET - Set the channel's time or frequency priority value

	UINT8 b_ChanIndex
		index for channel to enable/disable

	BOOLEAN o_isFreq
		TRUE  - enable/disable the frequency channel
		FALSE - enable/disable the time channel

	UINT8 *pb_priority
		Address to set or get priority value


Outputs:

	UINT8 *pb_priority
		Returned priority value


Return value:
    0 - successful
   -1 - failed
   -2 - channels not configured yet
   -3 - priority value out of range for set operation

-----------------------------------------------------------------------------
*/
extern int SC_ChanPriority(
  t_paramOperEnum b_operation,
  UINT8 b_ChanIndex,
  BOOLEAN o_isFreq,
  UINT8 *pb_priority
);
/*
----------------------------------------------------------------------------
                                SC_SetMeasRate()

Description:
This function will set the measurement rate.

Parameters:

	UINT8 b_ChanIndex
		index for channel to set measurement rate

	INT32 l_MeasRate
		When positive in frequency (Hz)
      When negative in period (S)
      Example
		   l_MeasRate = 32 (32 Hz)
		   l_MeasRate = 16 (16 Hz)
		   l_MeasRate = -2 (0.5 Hz)
		   l_MeasRate = -1 (1 Hz)
		   l_MeasRate = 1 (1 Hz)

Return value:
	0  - successful
  -1  - failed

-----------------------------------------------------------------------------
*/

extern int SC_SetMeasRate(UINT8 b_ChanIndex, INT32 l_MeasRate);

/*
----------------------------------------------------------------------------
                                SC_ChanAssumeQL()


Description: This function is called by the user to set or get the channel's
frequency or time assumed quality level value.

Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the channel's time or frequency assumed quality level value
		e_PARAM_SET - Set the channel's time or frequency assumed quality level value

	UINT8 b_ChanIndex
		index for channel to enable/disable

	BOOLEAN o_isFreq
		TRUE  - enable/disable the frequency channel
		FALSE - enable/disable the time channel

	UINT8 *pb_ql
		Address to set or get assumed quality level value


Outputs:

	UINT8 *pb_ql
		Returned assumed quality level value


Return value:
    0 - successful
	-1 - failed
   -2 - channels not configured yet
   -3 - ql value out of range for a set operation

-----------------------------------------------------------------------------
*/
extern int SC_ChanAssumeQL(
  t_paramOperEnum b_operation,
  UINT8 b_ChanIndex,
  BOOLEAN o_isFreq,
  UINT8 *pb_ql
);

/*
----------------------------------------------------------------------------
                                SC_ChanEnableAssumeQL()


Description: This function is called by the user to set or get the channel's
assumed QL enable/disable.

Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the channel's assumed QL enable/disable
		e_PARAM_SET - Set the channel's assumed QL enable/disable

	UINT8 b_ChanIndex
		index for channel to enable/disable

	BOOLEAN *po_enable
		Address to set or get channel's assumed QL enable/disable

Outputs:

	BOOLEAN *po_enable
		Returned channel's assumed QL enable/disable


Return value:
     0 - successful
    -1 - failed
    -2 - channels not configured yet

-----------------------------------------------------------------------------
*/
extern int SC_ChanEnableAssumeQL(
  t_paramOperEnum b_operation,
  UINT8 b_ChanIndex,
  BOOLEAN *po_enable
);

/*
----------------------------------------------------------------------------
                                SC_GetChanStatus()
Description:
This function will retrieve the channel status of all the configured channels

Parameters:

Inputs
        b_ChanCount
            Number of channels (must match already configured number)

       pas_chanStatus[]
            Pointer to address to copy channel status information into.

Outputs:
       pas_chanStatus[]
            A copy of the channel status information.

Return value:
         0: function succeeded
        -1: function failed
        -2: channel count does not match configured channel count
        -3: channels not configured yet

-----------------------------------------------------------------------------
*/
extern int SC_GetChanStatus(UINT8 b_ChanCount, SC_t_chanStatus pas_chanStatus[]);

/*
----------------------------------------------------------------------------
                                SC_OutQualityLevel()


Description: Called by Host. This function provides the current output quality level.
If there is an active input, the value will be the quality level associated with that
input. If there is no active input, the value will be the quality level associated
with the internal reference oscillator.

Parameters:

Inputs
	BOOLEAN o_isFreq
		TRUE  - frequency channel
		FALSE - time channel

Outputs:

	None

Return value:
	function succeeded.
	The returned value is the output quality level, a value in the range
	1..16

	Function failed
	-1: any failure not covered by other failure codes
-----------------------------------------------------------------------------
*/
extern int SC_OutQualityLevel(BOOLEAN o_isFreq);


/*
----------------------------------------------------------------------------
                                SC_QlToSSM()


Description: Called by Host. This function provides conversion from specified
quality level to an SSM value associated with the specified channel-type.
The primary purpose of the function is to provide a method for user to learn
the channel-specific SSM value that should be used if an output of that
channel-type is being generated from SoftClock

Parameters:

Inputs
	UINT8	b_quality_level
	SC_t_Chan_Type
			e_chan_type
	BOOLEAN *b_standard
		TRUE  - standard
		FALSE - non-standard

Outputs:

	None

Return value:
	>=0 function succeeded.
	The returned value is the SSM (or appropriate) value associated
	with the specified quality level and channel type

	Function failed
        Function failed
        -1: any failure not covered by other failure codes
        -2: if all input parameters are valid, but channel does not support SSM,
            such as GPS, Redundancy or frequency only channel
-----------------------------------------------------------------------------
*/
extern int SC_QlToSSM(UINT8 b_quality_level, SC_t_Chan_Type e_chan_type, BOOLEAN *b_standard);

/*
----------------------------------------------------------------------------
                                SC_RedundantEnable()
Description:
This function is called by user to enable or disable the redundancy.

Parameters:

Inputs
        BOOLEAN o_redEnabled
        0 - Disable Redundancy (go to Active mode)
        1 - Enable Redundancy (go to Standby mode)

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_RedundantEnable(BOOLEAN o_redEnabled);


/*
----------------------------------------------------------------------------
                                SC_GetRedundantStatus()

Description:
This function is called by the user to read the redundancy status information
from the SoftClock.

Parameters:

Inputs
        BOOLEAN *po_redEnabled
        Address of BOOLEAN memory location where the results will be stored. 
        
        SC_t_activeStandbyEnum *pe_redStatus;
        Address of BOOLEAN memory location where the results will be stored.

Outputs:
        BOOLEAN *po_redEnabled
        1 - The redundancy channel is enabled
        0 - The redundancy channel is disabled
        
        SC_t_activeStandbyEnum *pe_redStatus;
        The current redundancy status state as expressed by SC_t_activeStandbyEnum
        enumeration.


Return value:
	0: successful
       -1: function failed

-----------------------------------------------------------------------------
*/

extern int SC_GetRedundantStatus(BOOLEAN *po_redEnabled, SC_t_activeStandbyEnum *pe_redStatus);

#endif /*  H_SC_CHAN_H */
