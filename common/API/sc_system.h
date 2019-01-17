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

FILE NAME    : sc_system.h

AUTHOR       : Jining Yang

DESCRIPTION  : 

This header file contains the API definitions for internal use.

Revision control header:
$Id: API/sc_system.h 1.7 2011/02/02 16:26:51PST Kenneth Ho (kho) Exp  $

******************************************************************************
*/

/* Prevent file from being included multiple times. */
#ifndef H_SC_SYSTEM_h
#define H_SC_SYSTEM_h

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************/
/*                       ***INCLUDES Definitions***                          */
/*  Nested includes should not be used as a general practice.  If there is a */
/*  good reason to include an include within an include, do so here. 	     */
/*****************************************************************************/
#include "PTP/PTPdef.h"
#include "sc_types.h"
#include "sc_api.h"

/*
----------------------------------------------------------------------------
                                SC_GetUcMstTbl()

Description:
This function replaces FIO_ReadUcMstTbl function.

Parameters:

Inputs
        None

Outputs:
        ps_ucMstTbl
        Pointer to structure the master table to be written to.

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
extern BOOLEAN SC_GetUcMstTbl(PTP_t_PortAddrQryTbl* ps_ucMstTbl);

/*
----------------------------------------------------------------------------
                                SC_GetConfig()

Description:
This function replaces FIO_ReadConfig function.

Parameters:

Inputs
        None

Outputs:
        ps_cfgFile
        Pointer to structure the configuration structure to be written to.

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
extern BOOLEAN SC_GetConfig(PTP_t_cfgRom* ps_cfgFile);

/*
----------------------------------------------------------------------------
                                SC_GetClkRcvr()

Description:
This function replaces FIO_ReadClkRcvr function.

Parameters:

Inputs
        None

Outputs:
        ps_clkRcvr
        Pointer to structure the clock recover structure to be written to.

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
extern BOOLEAN SC_GetClkRcvr(PTP_t_cfgClkRcvr* ps_clkRcvr);

/*
----------------------------------------------------------------------------
                                SC_GetClkId()

Description:
This function reads clock Id from configuration data.

Parameters:

Inputs
        None

Outputs:
        ps_cfgFile
        Pointer to structure the configuration structure to be written to.

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
extern BOOLEAN SC_GetClkId(t_clockIdType* ps_clkId);

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
extern int SC_ServoConfig(t_servoConfigType *ps_servoConfig,
                          t_paramOperEnum   b_operation);

/*
----------------------------------------------------------------------------
                                SC_GetPrivateStatus()

Description:
This function writes the client status to the buffer in TLV format.

Parameters:

Inputs
        None

Outputs:
        UINT8* ab_buff
        Pointer to the buffer for writing status in TLV format

        UINT16* pw_dynSize
        Pointer for writing size of status data

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_GetPrivateStatus(UINT8* ab_buff, UINT16* pw_dynSize);
 
/*
----------------------------------------------------------------------------
                                SC_GetPrivateStatus()

Description:
This function converts UTC seconds to date/time format.

Parameters:

Inputs
        UINT64 time_sec
        UTC seconds

Outputs:
        char* ab_dateStr
        Pointer to the buffer for writing date/time string

Return value:
        None

-----------------------------------------------------------------------------
*/
extern void SC_TimeToDate(UINT64 time_sec, char* ab_dateStr);

/*
----------------------------------------------------------------------------
                                Is_InitComplete()

Description:
This function will return if the initializatin is complete.

Parameters:

Inputs
        None

Outputs:
        None

Return value:
        TRUE - Initialization is complete.

-----------------------------------------------------------------------------
*/
extern BOOLEAN Is_InitComplete(void);
#ifdef __cplusplus
}
#endif

#endif /*  H_SC_API_h */

