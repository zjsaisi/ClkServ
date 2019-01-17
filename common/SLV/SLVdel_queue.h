
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

FILE NAME    : SLVdel_queue.h

AUTHOR       : Ken Ho

DESCRIPTION  : 

These functions are used to queue up the timestamps from a delay request, to 
be accessed when the delay response returns.

Revision control header:
$Id: SLV/SLVdel_queue.h 1.1 2011/03/02 13:20:34PST Kenneth Ho (kho) Exp  $

******************************************************************************
*/


/* Prevent file from being included multiple times. */
#ifndef H_slvdel_queue_h
#define H_slvdel_queue_h


/*****************************************************************************/
/*                       ***INCLUDES Definitions***                          */
/*  Nested includes should not be used as a general practice.  If there is a */
/*  good reason to include an include within an include, do so here. 	     */
/*****************************************************************************/


/*****************************************************************************/
/*                       ***CONSTANT Definitions***                          */
/*  This section should be used to define constants (#defines) and macros    */
/*  that can be used by any module that includes this file.                  */
/*****************************************************************************/


/*****************************************************************************/
/*                       ***Data Type Specifications***                      */
/*  This section should be used to specify data types including structures,  */
/*  enumerations, unions, and redefining individual data types.              */
/*****************************************************************************/


/*****************************************************************************/
/*                       ***Public Data Section***                           */
/*  Global variables, if used, should be defined in a ".c" source file.      */
/*  The scope of a global may be extended to other modules by declaring it as*/
/*  as an extern here. 			                                     */
/*****************************************************************************/


/*****************************************************************************/
/*                       ***Function Prototype Section***                    */
/*  This section to be used to prototype public functions.                   */
/*                                                                           */
/*  Descriptions of parameters and return code should be given for each      */
/*  function.                                                                */
/*****************************************************************************/
/*
----------------------------------------------------------------------------
                                Put_Delay_Timestamp()

Description:

This function will place the delay request timestamps in a buffer to be
search when the delay response packets arrive.


Parameters:

Inputs
   SLV_t_LastDlrq *ps_lastDlrq
   Timestamp and sequence number data to be stored.
	
Outputs:
   None

Return value:
	None

-----------------------------------------------------------------------------
*/
extern void Put_Delay_Timestamp(SLV_t_LastDlrq *ps_lastDlrq);

/*
----------------------------------------------------------------------------
                                Get_Delay_Timestamp()

Description:

This function will place the delay request timestamps in a buffer to be
search when the delay response packets arrive.


Parameters:

Inputs
   UINT16 seq_num
   Sequency number to use to search for timestamp data.

   SLV_t_LastDlrq *ps_lastDlrq
   copy the data into this address.
	
Outputs:
   None

Return value:
	1 = success
   0 = fail


-----------------------------------------------------------------------------
*/
extern UINT8 Get_Delay_Timestamp(UINT16 seq_num, SLV_t_LastDlrq *ps_lastDlrq);
#endif /*  H_slvdel_queue_h */








