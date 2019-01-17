/*****************************************************************************
*                                                                            *
*   Copyright (C) 2011 Symmetricom, Inc., all rights reserved                *
*                                                                            *
*   This software contains proprietary and confidential information of       *
*   Symmetricom.                                                             *
*   It may not be reproduced, used, or disclosed to others for any purpose   *
*   without the written authorization of Symmetricom.                        *
*                                                                            *
******************************************************************************

FILE NAME    : $Source: sc_config_bcom_ks.h $

AUTHOR       : Daniel Brown

DESCRIPTION  : 

This file is for the Broadcom Keystone project. Broadcom Keystone
Uses only PTP. This build has to be as small as possible. The make file defines
BRCM_Keystone=1 for this build.

SoftClock ptp build partition header file. This file is not intended to be used
directly. During build time the build startup script (scripts/setupenv) will
rename the file to "sc_config.h".  Source files or other header files should be 
written to include "sc_config.h" so that they will be aware of the appropriate 
build partition during the build process.

Revision control header:
$Id: sc_config_bcom_ks.h 1.4 2011/12/12 11:30:03PST German Alvarez (galvarez) Exp  $

******************************************************************************
*/


/* Prevent file from being included multiple times. */
#ifndef H_SC_CONFIG_H
#define H_SC_CONFIG_H


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

#define SC_TOTAL_CHANNELS  1  /* maximum number of channels - sum of channels below */
#define SC_PTP_CHANNELS    1  /* maximum number of PTP channels */
#define SC_OTHER_CHANNELS  0  /* maximum number of GPS, Sync-E, E1, T1 channels */
#define SC_GPS_CHANNELS    0  /* maximum number of GPS channels */
#define SC_SYNCE_CHANNELS  0  /* maximum number of Synchronous Ethernet channels */
#define SC_SPAN_CHANNELS   0  /* maximum number of Traditional Sync channels */
#define SC_RED_CHANNELS    0  /* maximum number of Redundancy channels */
#define SC_FREQ_CHANNELS   0  /* maximum number of Frequency only channels */

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

#endif /*  H_SC_CONFIG_H */
