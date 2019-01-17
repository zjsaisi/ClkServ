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

FILE NAME    : sc_rev.h

AUTHOR       : Jining Yang

DESCRIPTION  : 

This header file contains the Soft Client Library revision info.

Revision control header:
$Id: API/sc_rev.h 1.54 2012/04/20 16:53:03PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/

/* Prevent file from being included multiple times. */
#ifndef H_SC_REV_h
#define H_SC_REV_h

/* Servo revision can be modified here directly. SC Library revision should
   be modified in file ./scripts/buildNumber in the sandbox directory */
#define k_SC_SERVO_REV  "ServoRev=2.0.32Alpha"
#define k_SC_LIB_REV    ("SCiRev=" SC_REV)

#endif /*  H_SC_REV_h */

