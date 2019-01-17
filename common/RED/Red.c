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

FILE NAME    : Red.c

AUTHOR       : Ken Ho

DESCRIPTION  : 

Generic Redundancy interface functions

Revision control header:
$Id: RED/Red.c 1.2 2011/09/22 14:10:59PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "Red.h"
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
** Function    : Red_Init
**
** Description : This function initializes the Redundancy channels.
**
** Parameters  : -
**
** Returnvalue : TRUE   - function succeeded
**               FALSE  - function failed
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN Red_Init (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount)
{
   UINT8 i;
   int rtn;

   /* cycle through the channel structures to find the Sync-E channels */
   for (i = 0; (i < b_ChanCount) && (i < SC_TOTAL_CHANNELS); i++)
   {
      /* if right channel type */
      if (as_ChanConfig[i].e_ChanType == e_SC_CHAN_TYPE_REDUNDANT)
      {
         /* call user API function */
         rtn = SC_InitRedIf(i);
         /* check return value */
         if (rtn < 0)
            return (FALSE);
      }
   }

   return (TRUE);
} /* end function E1_Init */

/***********************************************************************
** 
** Function    : Red_Close
**
** Description : This function closes the Redundancy channels.
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void Red_Close (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount)
{
   UINT8 i;

   /* cycle through the channel structures to find the e_SC_CHAN_TYPE_REDUNDANT channels */
   for (i = 0; (i < b_ChanCount) && (i < SC_TOTAL_CHANNELS); i++)
   {
      /* if right channel type */
      if (as_ChanConfig[i].e_ChanType == e_SC_CHAN_TYPE_REDUNDANT)
      {
         /* call user API function */
         SC_CloseRedIf(i);
      }
   }
} /* end function Red_Close */
