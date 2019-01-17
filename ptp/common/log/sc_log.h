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

FILE NAME    : sc_log.h

AUTHOR       : Jining Yang

DESCRIPTION  :

This unit defines event/message logging functions.
        SC_Log

Revision control header:
$Id: log/sc_log.h 1.6 2011/03/02 15:01:35PST German Alvarez (galvarez) Exp  $

******************************************************************************
*/
#ifndef H_SC_LOG_H
#define H_SC_LOG_H


/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <syslog.h>

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/
#define LOG_SYMM	LOG_LOCAL3      // Symm product use facility local4

/*--------------------------------------------------------------------------*/
//              function prototypes
/*--------------------------------------------------------------------------*/
char *getLocalEventLog();


#endif /* H_SC_LOG_H */
