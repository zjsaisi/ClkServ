/*****************************************************************************
*                                                                            *
*   Copyright (C) 2010 Symmetricom, Inc., all rights reserved                *
*                                                                            *
*   This software contains proprietary and confidential information of       *
*   Symmetricom.                                                             *
*   It may not be reproduced, used, or disclosed to others for any purpose   *
*   without the written authorization of Symmetricom.                        *
*                                                                            *
******************************************************************************

FILE NAME    : sc_util.c

AUTHOR       : Jining Yang

DESCRIPTION  :

This file implements the unitility functions that are OS or processor
dependent.

Revision control header:
$Id: API/sc_util.c 1.1 2010/02/08 22:57:40PST jyang Exp  $

******************************************************************************
*/

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <time.h>
#include "target.h"
#include "sc_api.h"

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

/*
----------------------------------------------------------------------------
                                SC_GetPrivateStatus()

Description:
This function converts UTC seconds to date/time format.

Parameters:

Inputs
        UINT64 time_sec
        UTC seconds

Outputs:
        char* ab_dateStr
        Pointer to the buffer for writing date/time string

Return value:
        None

-----------------------------------------------------------------------------
*/
void SC_TimeToDate(UINT64 time_sec, char* ab_dateStr)
{
    time_t in_time = (time_t)time_sec;
    struct tm* p_tm = gmtime(&in_time);
    sprintf(ab_dateStr, "%02hd:%02hd:%02hd %02hd/%02hd/%04hd",
            p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec,
            p_tm->tm_mon+1, p_tm->tm_mday, p_tm->tm_year+1900);
}
