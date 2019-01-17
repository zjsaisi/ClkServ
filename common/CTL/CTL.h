/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: CTL.h 
**    Summary: The control unit CTL controls all node data and all events
**             for the node and his channels to the net. A central task 
**             of the control unit is the state decision event. Within the 
**             announce interval, the unit ctl calculates a recommended 
**             state for each port of the node, and switches the channels 
**             to this state. 
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: CTL_Init
**             CTL_Close
**             CTL_Task
**             CTL_GetActOffs
**             CTL_GetErrStr
**
**   Compiler: Ansi-C
**    Remarks: 
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __CTL_H__
#define __CTL_H__

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
** Function    : CTL_Init
**
** Description : Initializes the Unit CTL 
**               The task handles of the MST_Task, SLV_SyncTask and
**               SLV_DelayTask get initialized. The handles must be 
**               declared and grouped in the SIS configuration in 
**               increasing order ( MST1_TSK,MST2_TSK ... MSTn_TSK
**                                  ...                            )
**              
** Parameters  : ps_tmPropDs (IN) - pointer to external time properties
**                                  data set provided by an external 
**                                  primary reference clock.
**                                  Must be NULL, if there is none.
**               ps_clkId   (OUT) - clock id of own node
**               
**
** Returnvalue : <> NULL - function succeeded, 
**                         read-only pointer to Clock data set
**               == NULL - function failed
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
const PTP_t_ClkDs* CTL_Init(PTP_t_TmPropDs* ps_tmPropDs,PTP_t_ClkId *ps_clkId);

/***********************************************************************
**
** Function    : CTL_Close
**
** Description : Deinitializes the Unit CTL.
**              
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_Close(void);

/***********************************************************************
**
** Function    : CTL_Task
**
** Description : The controltask is the central control unit of the 
**               protocol. Switching of the port state and handling of 
**               the clock data sets are managed centrally. 
**              
** Parameters  : w_hdl           (IN) - the SIS-task-handle            
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void CTL_Task(UINT16 w_hdl);

/*************************************************************************
**
** Function    : CTL_GetActOffs
**
** Description : Returns the actual offset from the actual master, or 
**               zero, if the node ist grandmaster.
**
** Parameters  : ps_tiActOffs (OUT) - pointer to actual offset interval
**               
** Returnvalue : - 
** 
** Remarks     : ATTENTION ! Do not use external.
**  
***********************************************************************/
void CTL_GetActOffs(PTP_t_TmIntv *ps_tiActOffs);

/*************************************************************************
**
** Function    : CTL_GetErrStr
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
const CHAR* CTL_GetErrStr(UINT32 dw_errNmb);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __CTL_H__ */
