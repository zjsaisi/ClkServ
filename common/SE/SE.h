
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

FILE NAME    : SE.h

AUTHOR       : Daniel Brown

DESCRIPTION  : 

Generic Syncronous Ethernet layer header file

Revision control header:
$Id: SE/SE.h 1.2 2011/02/02 15:57:27PST Kenneth Ho (kho) Exp  $

******************************************************************************
*/


/* Prevent file from being included multiple times. */
#ifndef H_SE_H
#define H_SE_H

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
BOOLEAN SE_Init (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount);

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
void SE_Close (SC_t_ChanConfig as_ChanConfig[], UINT8 b_ChanCount);

#endif /*  H_SE_H */
