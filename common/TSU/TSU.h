/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: TSU.h
**    Summary: TSU - Time Stamping Unit
**             The TSU provides a generic interface for gathering 
**             timestamps. This unit is responsible for the generation
**             and data management of all timestamps. Simple access
**             functions deliver requested timestamps to the protocol
**             software. All functions depend on the platform and/or
**             communication technology and must be adapted to the
**             target.
**  
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: TSU_Init
**             TSU_Close
**             TSU_GetRxTimeStamp
**             TSU_GetTxTimeStamp
**             TSU_GetErrStr
**             
**   Compiler: gcc 3.3.2
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __TSU_H__
#define __TSU_H__

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
** Function    : TSU_Init
**
** Description : This function initializes the unit TSU and all required
**               system resources. Two parameters could be used to
**               determine the corresponding Ethernet interface. 
**
**               The function parameter pf_errClbk is a pointer to the 
**               error callback function of the software stack. The 
**               initialization function of the unit CIF must copy this 
**               pointer to a local buffer, if unit CIF wants to generate 
**               error messages for example during network-initialization. 
**
**               The parameter ps_portId points to a structure array of 
**               all initialized interfaces. These structures contain 
**               the clock id and the port number of the corresponding
**               Ethernet interface (see also {PTP_t_PortId}). The
**               second parameter dw_AmntIntf shows the amount of active
**               interfaces and could be used to go through all 
**               interfaces of the ps_portId array.
**
** See Also    : TSU_Close()
**
** Parameters  : pf_errClbk  (IN) - error callback function
**               ps_portId   (IN) - pointer to the Port Id of all
**                                  communication-channels
**               dw_AmntIntf (IN) - amount of active interfaces
**
** Returnvalue : TRUE             - function succeeded
**               FALSE            - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN TSU_Init(t_PTP_ERR_CLBK_FUNC pf_errClbk,
                 const PTP_t_PortId  *ps_portId,
                 UINT32              dw_AmntIntf);

/***********************************************************************
** 
** Function    : TSU_Close
**
** Description : This function closes all system resources of
**               the unit TSU.
**
** See Also    : TSU_Init()
**
** Parameters  : dw_AmntIntf (IN) - amount of active interfaces
**
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
void TSU_Close(UINT32 dw_AmntIntf);

/*************************************************************************
**
** Function    : TSU_GetRxTimeStamp
**
** Description : This function retrieves the Rx timestamp qualified by 
**               the function parameters. Each time a PTP event message 
**               is received and dispatched by the protocol stack, this 
**               function is called. The purpose is to retrieve the 
**               appropriate timestamp of the received event message. To 
**               match the correct timestamp, all parameters except ps_ts 
**               must be identical to additionally saved timestamp data. 
**               The timestamp it-self is passed through ps_ts. It is the 
**               responsibility of the porting engineer to pass the 
**               appropriate timestamp to the corresponding event message. 
**
** See Also    : TSU_GetTxTimeStamp()
**
** Parameters  : w_ifIdx    (IN)  - interface index to get the timestamp
**               b_version  (IN)  - version number of PTP stack
**               e_mType    (IN)  - PTP message type of timestamped message
**               w_seqId    (IN)  - sequence id of timestamped message
**               ps_sndPId  (IN)  - sender port id of timestamped message
**               ps_ts      (OUT) - pointer to the tx timestamp
**
** Returnvalue : TRUE          - function succeeded
**               FALSE         - function failed
**
** Remarks     : Function is reentrant.
**
*************************************************************************/
BOOLEAN TSU_GetRxTimeStamp(UINT16             w_ifIdx,
                           UINT8              b_version,
                           PTP_t_msgTypeEnum  e_mType,
                           UINT16             w_seqId,
                           const PTP_t_PortId *ps_sndPId,
                           PTP_t_TmStmp       *ps_ts);
                           
/*************************************************************************
**
** Function    : TSU_GetTxTimeStamp
**
** Description : This function retrieves the Tx timestamp qualified by the 
**               function parameters. Each time a PTP event message is 
**               transmitted by the protocol stack, this function is 
**               called. The purpose is to retrieve the appropriate 
**               timestamp of the transmitted event message. To match the 
**               correct timestamp, all parameters except ps_ts must be 
**               identical to additionally saved timestamp data. The 
**               timestamp itself is passed through ps_ts. It is the 
**               responsibility of the porting engineer to pass the 
**               appropriate timestamp to the corresponding event message. 
**
** See Also    : TSU_GetRxTimeStamp()
**
** Parameters  : w_ifIdx   (IN) - interface index to get the timestamp
**               b_version (IN) - version number of PTP stack
**               b_sock    (IN) - socket of timestamped message
**               w_seqId   (IN) - sequence id of timestamped message
**               ps_ts    (OUT) - pointer to the tx timestamp
**
** Returnvalue : TRUE         - function succeeded
**               FALSE        - function failed
**
** Remarks     : Function is reentrant.
**
*************************************************************************/
BOOLEAN TSU_GetTxTimeStamp(UINT16       w_ifIdx,
                           UINT8        b_version,
                           UINT8        b_sock,
                           UINT16       w_seqId,
                           PTP_t_TmStmp *ps_ts);

/***********************************************************************
** 
** Function    : TSU_GetErrStr
**
** Description : This function resolves the error number to a correspondent 
**               error string. 
**
** Parameters  : dw_errNmb (IN) - error number code
**
** Returnvalue : const CHAR*    - pointer to corresponding error string
**
** Remarks     : Gets compiled with definition of ERR_STR.
**
***********************************************************************/
const CHAR* TSU_GetErrStr(UINT32 dw_errNmb);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __TSU_H__ */
