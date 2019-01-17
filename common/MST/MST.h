/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: MST.h 
**    Summary: This unit defines all global functions and types of the 
**             unit MST.
**             Functions to initialize the task, and the master-task 
**             itself.
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: MST_Init
**             MST_SetSyncInt
**             MST_SetAnncInt
**             MSTannc_Task
**             MSTsyn_Task
**             MSTdel_Task
**             MST_GetErrStr
**
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __MST_H__
#define __MST_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/

/*************************************************************************
**    data types
*************************************************************************/

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
*************************************************************************/

/***********************************************************************
**
** Function    : MST_Init
**
** Description : Initializes the Unit MST 
**              
** Parameters  : ps_portId       (IN) - pointer to the port id
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void MST_Init(const PTP_t_PortId *ps_portId);

/***********************************************************************
**
** Function    : MST_SetSyncInt
**
** Description : Set the log of sync interval
**
** Parameters  : w_ifIdx    (IN) - communication interface index to change 
**               c_syncIntv (IN) - exponent for calculating the sync 
**                                 interval
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void MST_SetSyncInt(UINT16 w_ifIdx,INT8 c_syncIntv);

/***********************************************************************
**
** Function    : MST_SetAnncInt
**
** Description : Set the log of sync interval
**
** Parameters  : w_ifIdx    (IN) - communication interface index to change 
**               c_anncIntv (IN) - exponent for calculating the sync 
**                                 interval
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void MST_SetAnncInt(UINT16 w_ifIdx,INT8 c_anncIntv);

/***********************************************************************
**
** Function    : MSTannc_Task
**
** Description : The MSTannc task is responsible for sending announce
**               messages on all ports in MASTER state
**              
** Parameters  : w_hdl  (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : Function is reentrant.
**
***********************************************************************/
void MSTannc_Task( UINT16 w_hdl );

/***********************************************************************
**
** Function    : MSTsyn_Task
**
** Description : The master task is responsible for sending sync messages
**               to the connected net segment. 
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : Function is reentrant.
**
***********************************************************************/
void MSTsyn_Task( UINT16 w_hdl );

/***********************************************************************
**
** Function    : MSTdel_Task
**
** Description : The master task is responsible for sending sync messages
**               to the connected net segment. Also it has to answer 
**               received delay request messages.
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : Function is reentrant.
**
***********************************************************************/
void MSTdel_Task( UINT16 w_hdl );

/*************************************************************************
**
** Function    : MST_GetErrStr
**
** Description : Resolves the error number to the according error string
**
** Parameters  : dw_errNmb (IN) - error number
**               
** Returnvalue : const CHAR*   - pointer to error string
** 
** Remarks     : Gets compiled with definition of ERR_STR.
**  
***********************************************************************/
const CHAR* MST_GetErrStr(UINT32 dw_errNmb);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __MST_H__ */
