/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: TCF.h  
**    Summary: Global header for the TCF unit. 
**             Defines all export functions of the unit TCF.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: TCF_Init
**             TCFsyn_Task
**             TCFdel_Task
**             TCFp2p_Task
**             TCF_GetErrStr
**
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __TCF_H__
#define __TCF_H__

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

/* pointer to callback function to get the actual peer delay of the regarding
   interface */
typedef  INT64 (*TCF_t_pfCbGetActPDel)(UINT16);

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
*************************************************************************/

/*************************************************************************
**
** Function    : TCF_Init
**
** Description : Initializes the Unit TCF with the Port ids and the 
**               used network interfaces 
**
** Parameters  : ps_portId       (IN) - pointer to the port id´s
**               dw_amntNetIf    (IN) - number of initializable net interfaces
**               pf_cbGetActPDel (IN) - pointer to callback function to get
**                                      the actual peer delay of the regarding
**                                      interface.
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCF_Init( const PTP_t_PortId *ps_portId,
               UINT32 dw_amntNetIf,
               TCF_t_pfCbGetActPDel pf_cbGetActPDel);

/***********************************************************************
**
** Function    : TCFsyn_Task
**
** Description : The task forwards sync- and follow up messages.
**              
** Parameters  : w_hdl        (IN) - the task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCFsyn_Task(UINT16 w_hdl );

/***********************************************************************
**
** Function    : TCFdel_Task
**
** Description : This task forwards delay request- and delay response
**               messages.
**              
** Parameters  : w_hdl        (IN) - the task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCFdel_Task(UINT16 w_hdl );

/***********************************************************************
**
** Function    : TCFp2p_Task
**
** Description : This task forwards pdelay request- and pdelay response
**               messages and corrects the residence time in pdelay 
**               response follow up messages.
**              
** Parameters  : w_hdl        (IN) - the task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void TCFp2p_Task(UINT16 w_hdl );

/*************************************************************************
**
** Function    : TCF_GetErrStr
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
const CHAR* TCF_GetErrStr(UINT32 dw_errNmb);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */
#endif /* __TCF_H__ */

