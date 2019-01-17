
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

FILE NAME    : sc_gps.c

AUTHOR       : Daniel Brown

DESCRIPTION  : 

API level functions for GPS function in SoftClock

Revision control header:
$Id: API/sc_gps.c 1.4 2011/03/08 15:05:07PST Daniel Brown (dbrown) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/

#include "GNS/GN_GPS_Task.h"
#include "sc_api.h"
#include "GPS/GPS.h"
#include "sc_gps.h"

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
** Function    : SC_GpsStat
**
** Description : This function gets status from the GPS engine.
**
** Parameters  :
**               ps_gpsStatus - status structure
**
** Returnvalue :  0  - function succeeded
**               -1  - function failed
**
** Remarks     : -
**
***********************************************************************/
int SC_GpsStat (SC_t_GPS_Status *ps_gpsStatus)
{
   /* check current status */  
   if(GPS_Stat(ps_gpsStatus) != TRUE)
      return -1;
   else
      return 0;

} /* end function GPS_Stat */







