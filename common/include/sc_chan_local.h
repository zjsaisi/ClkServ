
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

FILE NAME    : sc_chan_loc.h

AUTHOR       : Daniel Brown

DESCRIPTION  :

Contains some local function prototypes for
channelization associated items for SoftClock.

Revision control header:
$Id: sc_chan_local.h 1.1 2011/04/01 16:10:36PDT Daniel Brown (dbrown) Exp  $

******************************************************************************
*/


/* Prevent file from being included multiple times. */
#ifndef H_SC_CHAN_LOC_H
#define H_SC_CHAN_LOC_H

/*****************************************************************************/
/*                       ***INCLUDES Definitions***                          */
/*  Nested includes should not be used as a general practice.  If there is a */
/*  good reason to include an include within an include, do so here. 	     */
/*****************************************************************************/

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

/*
----------------------------------------------------------------------------
                                Is_ChanConfigured()

Description:
This function will return TRUE if the channel configuration is complete.

Parameters:

Inputs
      None

Outputs:
      None

Return value:
      TRUE - Channel configuration is complete.
      FALSE - Channel configuration is not complete.

-----------------------------------------------------------------------------
*/

extern BOOLEAN Is_ChanConfigured(void);

/*
----------------------------------------------------------------------------
                                Set_ChanConfigured()
Description:
This function sets the channel configured flag.

Parameters:

Inputs
      o_ConfiguredSetVal - value with which to set channel configured flag

Outputs:
      None

Return value:
      None

-----------------------------------------------------------------------------
*/
extern void Set_ChanConfigured(BOOLEAN o_ConfiguredSetVal);

#endif /*  H_SC_CHAN_LOC_H */
