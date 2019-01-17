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

FILE NAME    : T1.c

AUTHOR       : Daniel Brown

DESCRIPTION  : 

Generic Syncronous Ethernet layer interface functions

Revision control header:
$Id: T1/T1.c 1.1 2011/02/16 15:28:43PST Daniel Brown (dbrown) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "T1.h"
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

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/


/***********************************************************************
** 
** Function    : T1_Init
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
BOOLEAN T1_Init (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount)
{
   UINT8 i;
   int rtn;

   /* cycle through the channel structures to find the Sync-E channels */
   for (i = 0; (i < b_ChanCount) && (i < SC_TOTAL_CHANNELS); i++)
   {
      /* if right channel type */
      if (as_ChanConfig[i].e_ChanType == e_SC_CHAN_TYPE_T1)
      {
         /* call user API function */
         rtn = SC_InitT1If(i);
         /* check return value */
         if (rtn < 0)
            return (FALSE);
      }
   }

   return (TRUE);
} /* end function T1_Init */

/***********************************************************************
** 
** Function    : T1_Close
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
void T1_Close (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount)
{
   UINT8 i;

   /* cycle through the channel structures to find the Sync-E channels */
   for (i = 0; (i < b_ChanCount) && (i < SC_TOTAL_CHANNELS); i++)
   {
      /* if right channel type */
      if (as_ChanConfig[i].e_ChanType == e_SC_CHAN_TYPE_T1)
      {
         /* call user API function */
         SC_CloseT1If(i);
      }
   }
} /* end function T1_Close */
