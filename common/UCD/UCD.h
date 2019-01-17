/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: UCD.h 
**    Summary: The unicast discovery task is responsible to establish
**             the connection to a unicast master. First announce messages
**             get requested from the master. If the BMC algorithm detected
**             a unicast master to be the best master, the sync and delay
**             messages have to be requested from this master.
**             This services are limited to some duration. After that, they
**             have to be re-requested from the master.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: UCD_Init
**             UCD_Close
**             UCD_Task
**             UCD_SendUcCancel
**             UCD_GetErrStr
**             
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __UCD_H__
#define __UCD_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
#define k_UC_TLV_TYPE_LEN_SZE     (4u)
#define k_REQ_UC_TRM_TLV_SZE     (10u)
#define k_CNCL_UC_TRM_TLV_SZE     (6u)
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
** Function    : UCD_Init
**
** Description : Initializes the Unit UCD with the Port ids and the 
**               used network interfaces. 
**
** Parameters  : ps_portId       (IN) - pointer to the port id array
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void UCD_Init( const PTP_t_PortId *ps_portId);

/*************************************************************************
**
** Function    : UCD_Close
**
** Description : Deinitializes the Unit UCD by canceling every running service.
**
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void UCD_Close( void );

/***********************************************************************
**
** Function    : UCD_Task
**
** Description : The unicast discovery task is responsible for the 
**               unicast master discovery.
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : Function is reentrant.
**
***********************************************************************/
void UCD_Task( UINT16 w_hdl );

/***********************************************************************
**  
** Function    : UCD_SendUcCancel
**  
** Description : Sends a CANCEL UNICAST TRANSMISSION tlv. 
**  
** Parameters  : ps_pAddr   (IN) - port address of addressed node
**               w_ifIdx    (IN) - comm. interface to addressed node
**               w_seqId    (IN) - sequence id of message
**               e_msgType  (IN) - message type to cancel
**               
** Returnvalue : -
** 
** Remarks     : function is reentrant
**  
***********************************************************************/
void UCD_SendUcCancel(const PTP_t_PortAddr *ps_pAddr,
                      UINT16               w_ifIdx,
                      UINT16               w_seqId,
                      PTP_t_msgTypeEnum    e_msgType);

#ifdef ERR_STR
/*************************************************************************
**
** Function    : UCD_GetErrStr
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
const CHAR* UCD_GetErrStr(UINT32 dw_errNmb);
#endif

void UCD_SetAnncInt(INT8 c_logIntv);
void UCD_SetSyncInt(INT8 c_logIntv);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __UCD_H__ */

