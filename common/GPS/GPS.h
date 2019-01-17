
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

FILE NAME    : GPS.h

AUTHOR       : Daniel Brown

DESCRIPTION  : 

GPS shim layer between SoftClient and user API

Revision control header:
$Id: GPS/GPS.h 1.5 2012/04/23 15:18:15PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/


/* Prevent file from being included multiple times. */
#ifndef H_GPS_h
#define H_GPS_h

/*****************************************************************************/
/*                       ***INCLUDES Definitions***                          */
/*  Nested includes should not be used as a general practice.  If there is a */
/*  good reason to include an include within an include, do so here. 	     */
/*****************************************************************************/

#include "sc_api.h"

/*****************************************************************************/
/*                       ***CONSTANT Definitions***                          */
/*  This section should be used to define constants (#defines) and macros    */
/*  that can be used by any module that includes this file.                  */
/*****************************************************************************/


/*****************************************************************************/
/*                       ***Data Type Specifications***                      */
/*  This section should be used to specify data types including structures,  */
/*  enumerations, unions, and redefining individual data types.              */
/*****************************************************************************/


/*****************************************************************************/
/*                       ***Public Data Section***                           */
/*  Global variables, if used, should be defined in a ".c" source file.      */
/*  The scope of a global may be extended to other modules by declaring it as*/
/*  as an extern here. 			                                     */
/*****************************************************************************/


/*****************************************************************************/
/*                       ***Function Prototype Section***                    */
/*  This section to be used to prototype public functions.                   */
/*                                                                           */
/*  Descriptions of parameters and return code should be given for each      */
/*  function.                                                                */
/*****************************************************************************/

/***********************************************************************
** 
** Function    : GPS_Init
**
** Description : This function initializes the GPS engine.
**
** input:
**   ps_gpsGenCfg:   Configuration data structure
**
** outputs:
**   None
**
** return: TRUE:  success
**         FALSE: failure
**
***********************************************************************/
BOOLEAN GPS_Init (SC_t_GPS_GeneralConfig *ps_gpsGenCfg);

/***********************************************************************
** 
** Function    : GPS_Close
**
** Description : This function stops the GPS engine.
**
** input:
**
** outputs:
**   None
**
** return:
**   None
**
***********************************************************************/
void GPS_Close(void);

/***********************************************************************
** 
** Function    : GPS_Main
**
** Description : The main function of the GPS engine. Its call interval
**               is set by its calling functions and is determined at 
**               configuration time by the type of GPS engine.
**
** input:
**
** outputs:
**   None
**
** return:  TRUE:    GPS engine is still running
**          FALSE:   GPS engine stopped
**
***********************************************************************/
BOOLEAN GPS_Main(void);

/***********************************************************************
** 
** Function    : GPS_Stat
**
** Description : This function gets status GPS engine.
**
** Parameters  : -
**
** Returnvalue : TRUE   - function succeeded
**               FALSE  - function failed
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN GPS_Stat (SC_t_GPS_Status *ps_gpsStatus);

/***********************************************************************
** 
** Function    : GPS_Valid
**
** Description : This function gets the validity status of the GPS engine (typically
**               whether the engine is tracking or not.
**
** Parameters  : -
**
** Returnvalue : TRUE   - GPS status is valid
**               FALSE  - GPS status is NOT valid
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN GPS_Valid(int chan);

#endif /*  H_GPS_h */








