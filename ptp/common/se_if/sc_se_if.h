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

FILE NAME    : sc_se_if.h

AUTHOR       : Daniel Brown

DESCRIPTION  : 

This unit defines event/message logging functions.
        SC_Log

Revision control header:
$Id: se_if/sc_se_if.h 1.1 2011/01/26 18:03:44PST Daniel Brown (dbrown) Exp  $

******************************************************************************
*/
#ifndef H_SC_SE_IF_H
#define H_SC_SE_IF_H

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <syslog.h>

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/

//#define SC_SE_IF_DEBUG 1

#define SE_PORT1 "eth0"
#define SE_PORT2 "eth1"

#endif /* H_SC_SE_IF_H */
