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

FILE NAME    : sc_redundancy_if.h

AUTHOR       : Ken Ho

DESCRIPTION  : 

Revision control header:
$Id: redundancy_if/sc_redundancy_if.h 1.4 2011/09/22 14:37:08PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/
#ifndef H_SC_RED_IF_H
#define H_SC_RED_IF_H

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <syslog.h>

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/

//#define SC_RED_IF_DEBUG 1
extern int redundancyTask(const struct timespec * currentTime);
extern int setRedundantPeerAddress(const char * peerAddress);
extern void setOutboundTimestamp(const UINT64 TAIseconds, const UINT32 TAInanoseconds);
extern int getPeerInfo(t_ptpTimeStampType *remoteTime, UINT32 *remoteEventMap, SC_t_activeStandbyEnum *remoteStatus, struct timespec *localReceptionTime);
extern int getLastPeerTimestampToSoftClock(INT32 *phase, t_ptpTimeStampType *lastTimeStampToServo, struct timespec *lastDataToServoTime);
extern int sendCommandToPeer(const char command);
extern int setRedPckTrace(const int command);

#endif /* H_SC_RED_IF_H */
