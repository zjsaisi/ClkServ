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

FILE NAME    : THD.h

AUTHOR       : Jining Yang

DESCRIPTION  : 

This header file contains the definitions for thread module.

Revision control header:
$Id: THD/THD.h 1.4 2011/01/07 13:13:15PST Daniel Brown (dbrown) Exp  $

******************************************************************************
*/

/* Prevent file from being included multiple times. */
#ifndef H_THD_h
#define H_THD_h

#ifdef __cplusplus
extern "C"
{
#endif

/*
----------------------------------------------------------------------------
                                THD_InitTimeRes()

Description:
This function creates the PTP thread, one second CLK thread and one minuite
CLK thread. The also creates the semophores.


Parameters:

Inputs
        w_rateMultiplier
        Clock rate multiplier


Outputs:
        None


Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int THD_InitTimeRes(UINT16 w_rateMultiplier);

/*
----------------------------------------------------------------------------
                                THD_Start()

Description:
This function initializes the threads for soft client.

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
int THD_Start(void);

/*
----------------------------------------------------------------------------
                                THD_Run()

Description:
This function runs the threads for soft client.

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
int THD_Run(void);

/*
----------------------------------------------------------------------------
                                THD_Shutdown()

Description:
This function shutdown the threads for soft client.

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
int THD_Shutdown(void);


/*
----------------------------------------------------------------------------
                                THD_InitGpsInterval()

Description:
This function sets the call GPS Task call interval. Meant to be used to specify the
call interval differently for different GPS engines.

Parameters:

Inputs
        dw_gpsInterval - GPS Task call interval (msecs)

Outputs:
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
extern void THD_InitGpsInterval(UINT32 dw_gpsInterval);

#ifdef __cplusplus
}
#endif

#endif /*  H_SC_API_h */

