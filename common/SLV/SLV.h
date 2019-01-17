/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: SLV.h 
**    Summary: The synchronization to the master is done by the task 
**             SLV_SyncTask and the slave to master delay calculation is 
**             done by the SLV_DelayTask independently. A PTP node only 
**             configures one channel maximal to PTP_SLAVE.
**             Declares the public slave functions and defines public 
**             constants and macros of the unit slv.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: SLV_Init
**             SLV_SetSyncInt
**             SLV_SetDelReqInt
**             SLV_SyncTask
**             SLV_DelayTask
**             SLV_GetErrStr
**             
**
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __SLV_H__
#define __SLV_H__

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

/*************************************************************************
**
** Function    : SLV_Init
**
** Description : Initializes the Unit SLV with the Port ids and the 
**               used network interfaces.
**
** Parameters  : ps_portId       (IN) - pointer to the port id array
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void SLV_Init(const PTP_t_PortId *ps_portId);

/***********************************************************************
**
** Function    : SLV_SetSyncInt
**
** Description : Set´s the exponent for calculating the Sync interval
**               Sync interval = 2 ^ x  
**
** Parameters  : w_ifIdx        (IN) - communication interface index to set
**                                     the new sync interval 
**               c_syncInterval (IN) - exponent for calculating the sync 
**                                     interval
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void SLV_SetSyncInt(UINT16 w_ifIdx,INT8 c_syncInterval);

/***********************************************************************
**
** Function    : SLV_SetDelReqInt
**
** Description : Set the exponent for calculating the delay request
**               interval. Interval = 2 ^ x  
**
** Parameters  : w_ifIdx      (IN) - communication interface index to set the 
**                                   new delay request interval (0,...,n-1)
**               c_delReqIntv (IN) - exponent for calculating the delay 
**                                   request interval
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void SLV_SetDelReqInt(UINT16 w_ifIdx,INT8 c_delReqIntv);

/***********************************************************************
**
** Function    : SLV_SyncTask
**
** Description : Does the Synchronization to the actual master in the segment. 
**               The task-handles must be named: 
**               SLS1_TSK,SLS2_TSK, .... , SLSn_TSK. They must
**               be declared in ascending order in the SIS-ConfigFile !!!
**               For each channel of the node implementation there must
**               be a seperate slave sync task.
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void SLV_SyncTask(UINT16 w_hdl );

/***********************************************************************
**
** Function    : SLV_DelayTask
**
** Description : The slave delay task is responsible for calculating
**               the 'slave to master delay'. Therefore it sends delay
**               request messages to the master node. The master sends
**               a delay response message with his Rx Timestamp of the 
**               delay request. 
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void SLV_DelayTask(UINT16 w_hdl);

/*************************************************************************
**
** Function    : SLV_GetErrStr
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
const CHAR* SLV_GetErrStr(UINT32 dw_errNmb);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __SLV_H__ */

