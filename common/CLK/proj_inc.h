
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

FILE NAME    : proj_inc.h

AUTHOR       : Ken Ho

DESCRIPTION  : 

Project level defines

Revision control header:
$Id: CLK/proj_inc.h 1.9 2012/03/16 14:08:48PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/


/* Prevent file from being included multiple times. */
#ifndef H_proj_inc_h
#define H_proj_inc_h


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
/*   Projects name:     Project enum                                         */
/*   ELEKTRA            1                                                    */
/*   PEGASUS            2                                                    */
/*                                                                           */
/*****************************************************************************/
#define ELEKTRA        1
#define PEGASUS        2
#define GPS_PROJECT    3
#define TP1500_PROJECT 4

#define PRODUCT_ID ELEKTRA
/*****************************************************************************/
/*   Projects name:     ELEKTRA                                        */
/*****************************************************************************/
#if PRODUCT_ID == ELEKTRA
#define SMARTCLOCK 0

#ifndef NO_PHY
#define NO_PHY 0
#endif //#ifndef NO_PHY

#define VERSION_STR "2.1.2cq_rb_asym"
#define PROMPT_STR  "TP-1500_BETA"
#define ENTERPRISE 0 //testing enterprise TOD optimization
#define ENABLE_TIME //enable time always
#define PPS_CLOCKRATE 125000000 // 130MHz 1PPS resolution freq
#define NO_IPPM 1

#elif PRODUCT_ID == GPS_PROJECT
#define VERSION_STR "2.1.2gpsr"
#define PROMPT_STR  "TP-500_GPS"
#define ENTERPRISE 0 //testing enterprise TOD optimization
#define ENABLE_TIME //enable time always
#define PPS_CLOCKRATE 130000000 // 130MHz 1PPS resolution freq

#elif PRODUCT_ID == PEGASUS
#define VERSION_STR "2.1.0"
#define PROMPT_STR  "PTM"
#define ENTERPRISE 1 //testing enterprise TOD optimization
#define ENABLE_TIME //enable time always
#define PPS_CLOCKRATE 125000000 // 130MHz 1PPS resolution freq

#elif PRODUCT_ID == TP1500_PROJECT
#define VERSION_STR "0.0.11"
#define PROMPT_STR  "TP-1500_BETA"
#define ENTERPRISE 0 //testing enterprise TOD optimization
#define ENABLE_TIME //enable time always
#define PPS_CLOCKRATE 130000000 // 130MHz 1PPS resolution freq

#else
#define VERSION_STR "2.1.0"
#define PROMPT_STR  "TP-500"
#define ENTERPRISE 0 //testing enterprise TOD optimization
#define ENABLE_TIME //enable time always
#define PPS_CLOCKRATE 130000000 // 130MHz 1PPS resolution freq

#endif /* PRODUCT */



#endif /*  H_proj_inc_h */








