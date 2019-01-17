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

FILE NAME    : sc_types.h

AUTHOR       : Ken Ho

DESCRIPTION  : 

This header file contains the type definitions for SoftClient.

Revision control header:
$Id: sc_types.h 1.3 2010/02/22 11:09:03PST jyang Exp  $

******************************************************************************
*/

/* Prevent file from being included multiple times. */
#ifndef H_SC_TYPES_h
#define H_SC_TYPES_h

/*****************************************************************************/
/*                       ***INCLUDES Definitions***                          */
/*  Nested includes should not be used as a general practice.  If there is a */
/*  good reason to include an include within an include, do so here. 	     */
/*****************************************************************************/
#include <stdint.h>

/*****************************************************************************/
/*                       ***CONSTANT Definitions***                          */
/*  This section should be used to define constants (#defines) and macros    */
/*  that can be used by any module that includes this file.                  */
/*****************************************************************************/
#ifndef BOOLEAN
#define BOOLEAN uint8_t
#endif

#ifndef UINT8
#define UINT8 uint8_t
#endif

#ifndef INT8
#define INT8 int8_t
#endif

#ifndef UINT16
#define UINT16 uint16_t
#endif

#ifndef INT16
#define INT16 int16_t
#endif

#ifndef UINT32
#define UINT32 uint32_t
#endif

#ifndef INT32
#define INT32 int32_t
#endif

#ifndef UINT64
#define UINT64 unsigned long long
#endif

#ifndef INT64
#define INT64 signed long long
#endif

#ifndef FLOAT32
#define FLOAT32 float
#endif

#ifndef FLOAT64
#define FLOAT64 double
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif /* H_SC_TYPES_h */

