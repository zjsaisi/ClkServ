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

FILE NAME    : E1.c

AUTHOR       : Daniel Brown

DESCRIPTION  : 

Generic Syncronous Ethernet layer interface functions

Revision control header:
$Id: E1/E1.c 1.1 2011/02/16 15:27:37PST Daniel Brown (dbrown) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "E1.h"
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
** Function    : E1_Init
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
BOOLEAN E1_Init (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount)
{
   UINT8 i;
   int rtn;

   /* cycle through the channel structures to find the Sync-E channels */
   for (i = 0; (i < b_ChanCount) && (i < SC_TOTAL_CHANNELS); i++)
   {
      /* if right channel type */
      if (as_ChanConfig[i].e_ChanType == e_SC_CHAN_TYPE_E1)
      {
         /* call user API function */
         rtn = SC_InitE1If(i);
         /* check return value */
         if (rtn < 0)
            return (FALSE);
      }
   }

   return (TRUE);
} /* end function E1_Init */

/***********************************************************************
** 
** Function    : E1_Close
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
void E1_Close (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount)
{
   UINT8 i;

   /* cycle through the channel structures to find the Sync-E channels */
   for (i = 0; (i < b_ChanCount) && (i < SC_TOTAL_CHANNELS); i++)
   {
      /* if right channel type */
      if (as_ChanConfig[i].e_ChanType == e_SC_CHAN_TYPE_E1)
      {
         /* call user API function */
         SC_CloseE1If(i);
      }
   }
} /* end function E1_Close */
