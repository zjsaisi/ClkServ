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

FILE NAME    : sc_log.c

AUTHOR       : Jining Yang

DESCRIPTION  :

This unit defines event/message logging functions.
        SC_Log

Revision control header:
$Id: log/sc_log.c 1.13 2011/06/09 14:03:21PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/

/*--------------------------------------------------------------------------*/
//              include
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include "sc_api.h"
#include "sc_log.h"
#include "local_debug.h"

/*--------------------------------------------------------------------------*/
//              defines
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              types
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              data
/*--------------------------------------------------------------------------*/
char * gLocalLog = NULL;

/*--------------------------------------------------------------------------*/
//              externals
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              function prototypes
/*--------------------------------------------------------------------------*/
static int initLocalEventLog();
static int updateLocalEventLog(t_eventSevEnum e_sev,
    t_eventStateEnum e_state, const char *message, INT32 aux_info);
/*--------------------------------------------------------------------------*/
//              functions
/*--------------------------------------------------------------------------*/

/*
----------------------------------------------------------------------------
                                SC_Event()

Description:
Called by SoftClock, this function was formerly named SC_EventLog(). The name has been
changed to remove the implication that the sole purpose of the function is for logging of events.
On the SCRDB, the user-code handling of this function writes the events to syslog, but there
could be other actions taken, such as control of LEDs in a target implementation.
Parameters:

Inputs
	UINT16 w_eventId
	Event Id

	t_eventSevEnum e_sev
	Event severity

	t_eventStateEnum e_state
	Indicate if event is set or clear

	const char* fmt_ptr
	Pointer to printf format string of event message

	INT32 aux_info
	additional context-specific information associated with the specific event.
	If there is no context-specific usage for the event -d, -1 will be assigned.

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int SC_Event(UINT16 w_eventId, t_eventSevEnum e_sev,
                t_eventStateEnum e_state, const char *fmt_ptr, INT32 aux_info
)
{
  updateLocalEventLog(e_sev, e_state, fmt_ptr, aux_info);

  switch(e_state)
  {
  case e_EVENT_SET:
    syslog(LOG_SYMM | e_sev, "(%d) LOG SET: %s, %d", e_sev, fmt_ptr, aux_info);
    break;
  case e_EVENT_CLEAR:
    syslog(LOG_SYMM | e_sev, "(%d) LOG CLR: %s, %d", e_sev, fmt_ptr, aux_info);
    break;
  case e_EVENT_TRANSIENT:
    syslog(LOG_SYMM | e_sev, "(%d) LOG TNT: %s, %d", e_sev, fmt_ptr, aux_info);
    break;
  default:
    syslog(LOG_SYMM | e_sev, "(%d) LOG %d: %s, %d", e_state, e_sev, fmt_ptr, aux_info);
    break;
  }

  return 0;
}

/*
----------------------------------------------------------------------------
                                SC_DbgLog()

Description:
This function sends debug log message to logging system.

Parameters:

Inputs
        const char* fmt_ptr
        Pointer to printf format string

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int SC_DbgLog(const char *msg)
{

  syslog(LOG_SYMM | LOG_DEBUG, "%s", msg);

  if(s_localDebCfg.log_disp_on_console)
  {
    printf("%s", msg);
  }

  return 0;
}

/*--------------------------------------------------------------------------
    int updateLocalEventLog(char * message, t_eventStateEnum e_state)

Description:
The local log is just a text representation of all the active (set and not cleared)
messages in the log. This function will add or remove a message from the log.

Return value:
  0     no error
  -1    error

Parameters:

Inputs
        t_eventSevEnum e_sev
        Event severity

        t_eventStateEnum e_state
        Indicate if event is set or clear

        const char* fmt_ptr
        Pointer to printf format string of event message

        INT32 aux_info
        additional context-specific information associated with the specific event.
        If there is no context-specific usage for the event -d, -1 will be assigned.

Outputs
        None

Global variables affected and border effects:
  Read
    gLocalLog

  Write
    gLocalLog
--------------------------------------------------------------------------*/

static int updateLocalEventLog(t_eventSevEnum e_sev,
    t_eventStateEnum e_state, const char *message, INT32 aux_info)
{
  const int kLogSize = 4096; //Maximum size for the active log
  char *where;
  char  messageString[256];
  char  severity[256];
  int messageLength;
  int logLength;
  size_t bytesToMove;

  initLocalEventLog();

  if(gLocalLog == NULL) //Log memory allocation failed. Do nothing here
    return 0;

  switch(e_sev)
  {
    case e_EVENT_EMRG:
      strncpy(severity, "Emergency:", 255);
      break;
    case e_EVENT_ALRT:
      strncpy(severity, "Alert:", 255);
      break;
    case e_EVENT_CRIT:
      strncpy(severity, "Critical:", 255);
      break;
    case e_EVENT_ERR:
      strncpy(severity, "Error:", 255);
      break;
    case e_EVENT_WARN:
      strncpy(severity, "Warning:", 255);
      break;
    case e_EVENT_NOTC:
      strncpy(severity, "Notice:", 255);
      break;
    case e_EVENT_INFO:
      strncpy(severity, "Informational:", 255);
      break;
    case e_EVENT_DBG:
      strncpy(severity, "Debug:", 255);
      break;
    default:
      snprintf(severity, 255, "%d:", e_sev);
      break;
  }

  if(aux_info == -1)
    snprintf(messageString, 254, "%s %s", severity, message); //leave room for the line feed if needed
  else
    snprintf(messageString, 254, "%s %s, %d", severity, message, aux_info); //leave room for the line feed if needed
  //check and add a line feed at the end, if needed
  messageLength = strlen(messageString);
  if(messageString[messageLength - 1] != '\n')
  {
    messageString[messageLength] = '\n';
    messageString[messageLength+1] = 0;
    messageLength++;
  }

  logLength = strlen(gLocalLog);
  where = strstr(gLocalLog, messageString);

  // only handle e_EVENT_SET and e_EVENT_CLEAR messages
  switch(e_state)
  {
    case e_EVENT_SET:
      if(where == NULL) //good, is not a duplicated message
        strncat(gLocalLog, messageString, (kLogSize-messageLength)-1);
      break;

    case e_EVENT_CLEAR:
      if(where != NULL) //good, message is there, go and delete it.
      {
        bytesToMove = 1 + (logLength - messageLength) - (where - gLocalLog);
        memmove(where, where+messageLength, bytesToMove);
      }
      else
        return -1; //message not in the log
      break;

    case e_EVENT_TRANSIENT:
    default:
      //do nothing
      break;
  }

  return 0;
}

/*--------------------------------------------------------------------------
    static int initLocalEventLog()

Description:
Allocates memory for the local active (set and not cleared) messages in the log.

Return value:
  0     no error
  -1    error

Parameters:

Inputs
  None

Outputs
  None

Global variables affected and border effects:
  Read
    gLocalLog

  Write
    gLocalLog
--------------------------------------------------------------------------*/

static int initLocalEventLog()
{
  const int kLogSize = 2048; //Maximum size for the active log

  if(gLocalLog == NULL)
  {
    gLocalLog = malloc(kLogSize);
    if(gLocalLog != NULL)
    {
      memset(gLocalLog, 0, kLogSize);
      return 0;
    }
    else
      return -1;
  }
  return 0;
}

/*--------------------------------------------------------------------------
    static char *getLocalEventLog()

Description:
Returns a pointer to the local debug log. The local debug log is a 0x00 terminated
text with all the active entries in the log. (Set and not Cleared)

Return value:
  0     no error
  -1    error

Parameters:

Inputs
  None

Outputs
  None

Global variables affected and border effects:
  Read
    gLocalLog

  Write
--------------------------------------------------------------------------*/
char *getLocalEventLog()
{
  initLocalEventLog();

  return gLocalLog;
}
