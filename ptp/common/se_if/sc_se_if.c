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
      SC_InitSeIf

Revision control header:
$Id: se_if/sc_se_if.c 1.4 2011/02/15 10:30:33PST Daniel Brown (dbrown) Exp  $

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
#include "network_if/sc_network_if.h"
#include "sc_se_if.h"
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
                                SC_InitSeIf()

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
int SC_InitSeIf(UINT8 b_ChanIndex)
{
   int retval;

#ifdef SC_SE_IF_DEBUG
   printf( "+SC_InitSeIf b_ChanIndex: %d\r\n", b_ChanIndex);
#endif

   /* try to initialize the Sync-E port and pass return value back to SoftClock */
   retval = InitSePort(b_ChanIndex);

   /* pass value back to SoftClock */
   return(retval);
}

/*
----------------------------------------------------------------------------
                                SC_CloseSeIf()

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
void SC_CloseSeIf(
	UINT8 b_ChanIndex
)
{
#ifdef SC_SE_IF_DEBUG
   printf( "+SC_CloseSeIf b_ChanIndex: %d\r\n", b_ChanIndex);
#endif
   /* Close Sync-E Port */
   CloseSePort(b_ChanIndex);
}

