
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

FILE NAME    : Red.h

AUTHOR       : Daniel Brown

DESCRIPTION  : 

Generic Redundancy header file

Revision control header:
$Id: RED/Red.h 1.1 2011/08/24 17:30:39PDT Kenneth Ho (kho) Exp  $

******************************************************************************
*/


/* Prevent file from being included multiple times. */
#ifndef H_RED_H
#define H_RED_H

/*****************************************************************************/
/*                       ***INCLUDES Definitions***                          */
/*  Nested includes should not be used as a general practice.  If there is a */
/*  good reason to include an include within an include, do so here. 	     */
/*****************************************************************************/
#include "sc_chan.h"
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
/*  as an extern here. 			                                               */
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
BOOLEAN Red_Init (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount);

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
void Red_Close (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount);

#endif /*  H_RED_H */
