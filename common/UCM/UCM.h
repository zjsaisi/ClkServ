/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: UCM.h  
**    Summary: This unit defines all global functions and types of the 
**             unit unicast master UCM.
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: UCM_Init
**             UCMann_Task
**             UCMsyn_Task
**             UCMdel_Task
**             UCM_GetErrStr
**             
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __UCM_H__
#define __UCM_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/

#define k_GRNT_UC_TRM_TLV_SZE    (12u)
#define k_ACK_CNCL_UC_TRM_TLV_SZE (6u)
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
** Function    : UCM_Init
**
** Description : Initializes the Unit UCM 
**              
** Parameters  : ps_portId       (IN) - pointer to the port id´s
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void UCM_Init( const PTP_t_PortId *ps_portId);

/***********************************************************************
**
** Function    : UCMann_Task
**
** Description : The UCMannc task is responsible for sending announce
**               messages on all ports in MASTER state
**              
** Parameters  : w_hdl  (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : Function is reentrant.
**
***********************************************************************/
void UCMann_Task( UINT16 w_hdl );

/***********************************************************************
**
** Function    : UCMsyn_Task
**
** Description : The unicast master sync task is responsible for 
**               sending sync messages to all registered unicast slaves. 
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : Function is reentrant.
**
***********************************************************************/
void UCMsyn_Task( UINT16 w_hdl );

/***********************************************************************
**
** Function    : UCMdel_Task
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
void UCMdel_Task( UINT16 w_hdl );

/*************************************************************************
**
** Function    : UCM_GetErrStr
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
const CHAR* UCM_GetErrStr(UINT32 dw_errNmb);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __MST_H__ */
