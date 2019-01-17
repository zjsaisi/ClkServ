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

FILE NAME    : sc_se_if.c

AUTHOR       : Daniel brown

DESCRIPTION  : 

This unit contains the definitions for the following user defined SC GPS API functions.
      SC_InitE1If

Revision control header:
$Id: e1_if/sc_e1_if.c 1.1 2011/02/16 15:55:07PST Daniel Brown (dbrown) Exp  $

******************************************************************************
*/

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>

#include "sc_api.h"
#include "sc_e1_if.h"
#include "local_debug.h"

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

/*
----------------------------------------------------------------------------
                                SC_InitE1If()

Description:
This function is called by the SCi2000 to initialize the E1 interface.
It should be used to set up any communication and tasks required
to receive SSM information. Also, should be used to perform any hardware 
setup of the E1 receivers and clock measurement.

Parameters:

Inputs
   b_ChanIndex - Channel Index

Outputs
	None

Return value:
    0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int SC_InitE1If(UINT8 b_ChanIndex)
{
#ifdef SC_E1_IF_DEBUG
   printf( "+SC_InitE1If b_ChanIndex: %d\r\n", b_ChanIndex);
#endif

   /* pass value back to SoftClock */
   return(0);
}

/*
----------------------------------------------------------------------------
                                SC_CloseE1If()

Description:
This function is called by the SCi2000 to close the E1 interface. It 
should be used by the host to free up any resources to be reclaimed 
including but not limited to the E1 ports

Parameters:

Inputs
        b_ChanIndex - Channel Index

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
void SC_CloseE1If(
	UINT8 b_ChanIndex
)
{
#ifdef SC_E1_IF_DEBUG
   printf( "+SC_CloseE1If b_ChanIndex: %d\r\n", b_ChanIndex);
#endif
}

