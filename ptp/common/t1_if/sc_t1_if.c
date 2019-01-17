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

FILE NAME    : sc_t1_if.c

AUTHOR       : Daniel brown

DESCRIPTION  : 

This unit contains the definitions for the following user defined SC GPS API functions.
      SC_InitT1If

Revision control header:
$Id: t1_if/sc_t1_if.c 1.1 2011/02/16 15:55:39PST Daniel Brown (dbrown) Exp  $

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
#include "sc_t1_if.h"
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
                                SC_InitT1If()

Description:
   Opens GPS interface port. Called by SCi2000 upon invoking SC_InitConfigComplete()

Parameters:

Inputs
        None

Outputs:
        None

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
int SC_InitT1If(UINT8 b_ChanIndex)
{
#ifdef SC_T1_IF_DEBUG
   printf( "+SC_InitT1If b_ChanIndex: %d\r\n", b_ChanIndex);
#endif

   /* pass value back to SoftClock */
   return(0);
}

/*
----------------------------------------------------------------------------
                                SC_CloseT1If()

Description:
This function is called by the SCi2000 to close the Sync-E interface. It 
should be used by the host to free up any resources to be reclaimed 
including but not limited to the Sync-E ports

Parameters:

Inputs
        b_ChanIndex - Channel Index

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
void SC_CloseT1If(
	UINT8 b_ChanIndex
)
{
#ifdef SC_T1_IF_DEBUG
   printf( "+SC_CloseT1If b_ChanIndex: %d\r\n", b_ChanIndex);
#endif
}

