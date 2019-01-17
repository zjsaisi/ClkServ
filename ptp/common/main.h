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

FILE NAME    : main.h

AUTHOR       : Ken Ho

DESCRIPTION  : 

Revision control header:
$Id: main.h 1.2 2011/09/20 12:25:20PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/
#ifndef H_MAIN_H
#define H_MAIN_H

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/
extern void TimerStop(void);
extern int TimerStart(UINT16 w_multiplier);

extern UINT32 diff_usec(const struct timespec start, const struct timespec end);

#endif /* H_MAIN_H */
