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

FILE NAME    : DBG.c

AUTHOR       : Ken Ho

DESCRIPTION  :

This unit defines debug printf functions.
The functions in this file are used to print debug information.
        debug_printf_function
        set_debug_print
        get_debug_print


Revision control header:
$Id: DBG/DBG.c 1.22 2012/02/16 15:09:25PST German Alvarez (galvarez) Exp  $

******************************************************************************
*/

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include "target.h"
#include "sc_types.h"
#include "sc_api.h"
#include "PTP/PTPdef.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "CIF/CIF.h"
#include "CLK/clk_private.h"
#include "CLK/ptp.h"
#include "DBG.h"

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
static t_logConfigType s_logConfig =
  { e_SC_TST_NORMAL, k_DEFAULT_DEBUG_MASK, k_DEFAULT_DEBUG_TS_LIMIT };

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

extern void Start_Phase_Test(void);
extern void Start_Freq_Test(void);
extern int Start_Vcal(void);
extern void Start_FreqZero_Test(int enable);

/*
----------------------------------------------------------------------------
                                SC_DbgConfig()

Description:
This function gets or sets the current long configuration parameters.
On a get, the current parameters are copied into the supplied structure.
On a set, the current parameters are set from the supplied structure.


Parameters:

Inputs
	t_logConfigType *ps_logConfig
	On a set, this pointer defines the current log configuration .

	t_paramOperEnum b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

Outputs
	t_logConfigType *s_logConfig
	On a get, the current log configuration is written to this
	pointer location.

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int SC_DbgConfig(t_logConfigType * ps_logConfig, t_paramOperEnum b_operation)
{
  extern void SetTimestampThreshold(int threshold);

  if(b_operation == e_PARAM_GET)
  {
    /* bitmapped log mask */
    *ps_logConfig = s_logConfig;
  }
  else if(b_operation == e_PARAM_SET)
  {

/* set timestamp threshold */
    s_logConfig.l_debugTsLimit = ps_logConfig->l_debugTsLimit;

/* set debug prints */
    s_logConfig.l_debugMask = ps_logConfig->l_debugMask;

/* set mode */
    switch (ps_logConfig->b_dbgMode)
    {
    case e_SC_TST_NORMAL:
      s_logConfig.b_dbgMode = ps_logConfig->b_dbgMode;
      Start_FreqZero_Test(FALSE); //e_SC_TST_FREQ_ZERO is the only test that can be stopped. All other need a restart
      break;
    case e_SC_TST_FREQ_64PPB_STEP:
      s_logConfig.b_dbgMode = ps_logConfig->b_dbgMode;
      Start_Freq_Test();
      break;
    case e_SC_TST_PHASE_100US_STEP:
      s_logConfig.b_dbgMode = ps_logConfig->b_dbgMode;
      Start_Phase_Test();
      break;
    case e_SC_TST_VAR_CAL:
      s_logConfig.b_dbgMode = ps_logConfig->b_dbgMode;
      Start_Vcal();
      break;
    case e_SC_TST_FIFO_BUF_FWD:
      Print_FIFO_Buffer(0, 0);
      break;
    case e_SC_TST_FIFO_BUF_REV:
      Print_FIFO_Buffer(1, 0);
      break;
    case e_SC_TST_FIFO_BUF_FWD_SHORT:
      Print_FIFO_Buffer(0, 1);
      break;
    case e_SC_TST_FIFO_BUF_REV_SHORT:
      Print_FIFO_Buffer(1, 1);
      break;
    case e_SC_TST_TIMESTAMP_THRESHOLD:
      SetTimestampThreshold(s_logConfig.l_debugTsLimit);
      break;
    case e_SC_TST_SERVO_CHAN_READ:
      /* the option is deprecated, this should be part of the user code.
	   * or use the debug flag CHAN_STAT_PRT
       * Print_Servo_Chan_Info();
       */
      break;
    case e_SC_TST_FREQ_ZERO:
      Start_FreqZero_Test(TRUE);
      break;
    default:
      return -1;
      break;
    }
  }
  else
  {
    return -1;
  }
  return 0;
}

/***********************************************************************
**
** Function    : event_printf
**
** Description : This function prints the a event message with severity
**               level.
**
** See Also    : debug_printf_function()
**
** Parameter   : event_id (IN) - event Id
**               pri (IN) - severity level
**               state (IN) - set (1) or clear (0)
**               fmtptr (IN) - pointer to a string
**               aux_info (IN) - additional information
**
** Returnvalue : SC_Log return
**
** Remarks     : This function coded by Symmetricom
**
***********************************************************************/
int event_printf(UINT16 event_id, t_eventSevEnum pri, t_eventStateEnum state,
                 const char *fmt_ptr, INT32 aux_info)
{
  return SC_Event(event_id, pri, state, fmt_ptr, aux_info);
}

/***********************************************************************
**
** Function    : debug_printf_function
**
** Description : This function prints the a debug message with a
**               qualifying debug mask.
**
** See Also    : set_debug_print()
**
** Parameter   : message_num (IN) - debug mask
**               fmtptr (IN) - pointer to printf format string
**
** Returnvalue : SC_Log return
**
** Remarks     : This function coded by Symmetricom
**
***********************************************************************/
int debug_printf_function(UINT32 message_num, const char *fmt_ptr, ...)
{
  char str[DEBUG_BUFLEN];
  va_list ap;

  if((s_logConfig.l_debugMask & message_num) || message_num == UNMASK_PRT)
  {
    va_start(ap, fmt_ptr);
    vsprintf(str, fmt_ptr, ap);
    va_end(ap);

    return SC_DbgLog(str);
  }
  return 0;
}

/***********************************************************************
**
** Function    : set_debug_print
**
** Description : This function sets the debug mask for debug_pintf()
**
** See Also    : debug_printf_function()
**
** Parameter   : mask - 32 bit debug mask
**
** Returnvalue : -
**
** Remarks     : This function coded by Symmetricom
**
***********************************************************************/
void set_debug_print(UINT32 mask)
{
  s_logConfig.l_debugMask = mask;
}

/***********************************************************************
**
** Function    : get_debug_print
**
** Description : This function gets the debug mask for debug_pintf()
**
** See Also    : debug_printf_function()
**
** Parameter   : -
**
** Returnvalue : 32 bit debug mask
**
** Remarks     : This function coded by Symmetricom
**
***********************************************************************/
UINT32 get_debug_print(void)
{
  return s_logConfig.l_debugMask;
}
