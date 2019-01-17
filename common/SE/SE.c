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

FILE NAME    : SE.c

AUTHOR       : Daniel Brown

DESCRIPTION  : 

Generic Syncronous Ethernet layer interface functions

Revision control header:
$Id: SE/SE.c 1.1 2011/01/26 15:04:15PST Daniel Brown (dbrown) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "SE.h"
#include "sc_chan.h"

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/

/* gps engine status variable */
//static UINT8  SE_b_engSts = k_SE_STS_STOP;

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/


/***********************************************************************
** 
** Function    : SE_Init
**
** Description : This function initializes the Synchronous Ethernet channels.
**
** Parameters  : -
**
** Returnvalue : TRUE   - function succeeded
**               FALSE  - function failed
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN SE_Init (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount)
{
   UINT8 i;
   INT8 rtn;

   /* cycle through the channel structures to find the Sync-E channels */
   for (i = 0; (i < b_ChanCount) && (i < SC_TOTAL_CHANNELS); i++)
   {
      /* if right channel type */
      if (as_ChanConfig[i].e_ChanType == e_SC_CHAN_TYPE_SE)
      {
         /* call user API function */
         rtn = SC_InitSeIf(i);
         /* check return value */
         if (rtn < 0)
            return (FALSE);
      }
   }

   return (TRUE);
} /* end function SE_Init */

/***********************************************************************
** 
** Function    : SE_Close
**
** Description : This function closes the Synchronous Ethernet channels.
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void SE_Close (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount)
{
   UINT8 i;

   /* cycle through the channel structures to find the Sync-E channels */
   for (i = 0; (i < b_ChanCount) && (i < SC_TOTAL_CHANNELS); i++)
   {
      /* if right channel type */
      if (as_ChanConfig[i].e_ChanType == e_SC_CHAN_TYPE_SE)
      {
         /* call user API function */
         SC_CloseSeIf(i);
      }
   }

   return;
} /* end function SE_Close */
