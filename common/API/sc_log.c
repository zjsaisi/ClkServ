
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

AUTHOR       : Ken Ho

DESCRIPTION  : 

The functions in this file are used to configure and retreive the log from
the SoftClient System.
	SC_LogConfig()
	SC_LogGet()

Revision control header:
$Id: API/sc_log.c 1.4 2010/01/22 14:27:01PST kho Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include "sc_types.h"
#include "sc_api.h"
#include "DBG/DBG.h"


/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/




/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

/*
----------------------------------------------------------------------------
                                SC_LogConfig()

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
#if 0
extern void Print_FIFO_Buffer(UINT16 index, UINT16 fast);
extern int Start_Vcal();
extern int setfreq_switchover(INT32 freq);
int SC_LogConfig(
	t_logConfigType *ps_logConfig,
	t_paramOperEnum b_operation 
)
{
   INT32 *p_temp;
   if(b_operation == e_PARAM_GET)
   {
      /* bitmapped log mask */
      ps_logConfig->l_logMask = 0;
      /* bitmapped debug message mask */
      ps_logConfig->l_debugMask = get_debug_print();
   }
   else if(b_operation == e_PARAM_SET_SPECIAL)
   {
       switch(ps_logConfig->l_debugMask)
       {
       case 0:
           Print_FIFO_Buffer(0, 0);
           break;
       case 1:
           Print_FIFO_Buffer(1, 0);
           break;
       case 2:
           Print_FIFO_Buffer(0, 1);
           break;
       case 3:
           Print_FIFO_Buffer(1, 1);
           break;
       case 4:  // vwarp
           Start_Vcal();
           break;
       case 5:  // detilt
           exittilt();
           break;
       case 6:  // set_fdrift
          p_temp = (INT32 *)&ps_logConfig->l_logMask;
          printf("sending %ld\n", *p_temp);
          setfreq(*p_temp);
           break;
      }
   }
   else
   {
	set_debug_print(ps_logConfig->l_debugMask);
   }
   return 0;
}
#endif
