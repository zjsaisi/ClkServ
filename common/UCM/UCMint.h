/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: UCMint.h  
**    Summary: This unit defines the unit-internal variables, 
**             types and functions.
**             Error definitions of unit UCM.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: UCM_initSrvReqList
**             UCM_regServReq
**             UCM_unregServReq
**             UCM_SrchSlv
**             UCM_recalcPresc
**             UCM_checkPeriod
**             UCM_RespUcRequest
**             UCM_RespUcCnclAck
**             UCM_IsSyncSrvGranted
**
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __UCMINT_H__
#define __UCMINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
/* errors */
#define UCM_k_MEMERR    (1u) /* allocation error */ 
#define UCM_k_MSGERR    (2u) /* misplaced internal message error */
#define UCM_k_EVT_NDEF  (3u) /* Event is not defined for this task */

/* task states */
#define k_UCM_STOPPED   (0u) /* stopped state */

/*************************************************************************
**    data types
*************************************************************************/


/************************************************************************/
/** UCM_t_slvSrv 
          This struct defines all data correlating to a unicast slave to
          provide with a master message service.
**/
typedef struct
{
  PTP_t_PortAddr s_pAddr;     /* slaves port address */
  INT8           c_intv;      /* log2 of interval */
  UINT16         w_presc;     /* prescale value for reloading decrement 
                                 counter */
  UINT16         w_actCnt;    /* actual value of decrement counter */
  UINT32         dw_prdEnd;   /* end period for providing service in SIS 
                                 tick count */
  UINT32         dw_durSec;   /* duration of service in seconds */
  UINT16         w_seqIdSrv;  /* sequence id for the requested messages */
  UINT16         w_seqIdSign; /* sequence id for signaling messages */
  UINT16         w_ifIdx;     /* interface index to send */
  void*          pv_next;     /* pointer to next struct */
  void*          pv_prev;     /* pointer to last struct */
}UCM_t_slvSrv;

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
*************************************************************************/

/***********************************************************************
**
** Function    : UCM_initSrvReqList
**
** Description : Initializes a service request list by setting the head
**               of the list to default values.
**              
** Parameters  : ps_listHead (IN) - pointer to head of service request list
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void UCM_initSrvReqList(UCM_t_slvSrv *ps_listHead);

/***********************************************************************
**
** Function    : UCM_regServReq
**
** Description : Registers a service request in the appropriate 
**               service request list.
**              
** Parameters  : ps_msg      (IN) - pointer to service request message
**               ps_listHead (IN) - pointer to head of service request list
**               e_msgType   (IN) - message type of request
**               c_minIntv   (IN) - actual minimum interval of srv req task
**
** Returnvalue : INT8 - new minimal interval
**                      
** Remarks     : -
**
***********************************************************************/
INT8 UCM_regServReq( const PTP_t_MboxMsg *ps_msg,
                     UCM_t_slvSrv        *ps_listHead,
                     PTP_t_msgTypeEnum   e_msgType,
                     INT8                c_minIntv);
/***********************************************************************
**
** Function    : UCM_unregServReq
**
** Description : Unregisters a service request and frees the allocated memory
**              
** Parameters  : ps_srv     (IN) - pointer to unicast slave request
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void UCM_unregServReq(const UCM_t_slvSrv *ps_srv);

/***********************************************************************
**
** Function    : UCM_SrchSlv
**
** Description : Searches for slave in given slave array.
**              
** Parameters  : ps_slvSrvArr (IN) - pointer to unicast slave array.
**               ps_pAddr     (IN) - pointer to slave to search
**               pps_fndSlv  (OUT) - pointer to pointer to found slave in array.
**
** Returnvalue : TRUE              - slave found
**               FALSE             - slave not found
**                      
** Remarks     : -
**
***********************************************************************/
BOOLEAN UCM_SrchSlv(UCM_t_slvSrv         *ps_slvSrvArr,
                    const PTP_t_PortAddr *ps_pAddr,
                    UCM_t_slvSrv         **pps_fndSlv);

/***********************************************************************
**
** Function    : UCM_recalcPresc
**
** Description : Recalculate new prescaler value for slave service 
**               request list and adapt actual counter to it. 
**              
** Parameters  : ps_slvSrvArr (IN) - pointer to unicast slave array.
**               c_minIntv    (IN) - minimum interval of complete array
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void UCM_recalcPresc(UCM_t_slvSrv *ps_slvSrvArr,INT8 c_minIntv);

/***********************************************************************
**
** Function    : UCM_checkPeriod
**
** Description : Checks the service period of the actual slave request
**               and destroys it if the period is over
**              
** Parameters  : ps_srv     (IN) - pointer to unicast slave request
**               dw_actTime (IN) - actual SIS time tick
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void UCM_checkPeriod(const UCM_t_slvSrv *ps_srv,UINT32 dw_actTime);

/***********************************************************************
**
** Function    : UCM_RespUcRequest
**
** Description : Responses an unicast transmission request
**              
** Parameters  : ps_pAddr  (IN) - port address of unicast slave
**               w_ifIdx   (IN) - interface index to unicast slave
**               w_seqId   (IN) - sequence id for response
**               dw_durSec (IN) - granted period in seconds
**               c_logIntv (IN) - granted interval
**               e_msgType (IN) - requested message type
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void UCM_RespUcRequest(const PTP_t_PortAddr *ps_pAddr,
                       UINT16               w_ifIdx,
                       UINT16               w_seqId, 
                       UINT32               dw_durSec,
                       INT8                 c_logIntv,
                       PTP_t_msgTypeEnum    e_msgType);
                       
/***********************************************************************
**
** Function    : UCM_RespUcCnclAck
**
** Description : Responses an cancel unicast transmission message
**              
** Parameters  : ps_pAddr  (IN) - port address of unicast slave
**               w_ifIdx   (IN) - interface index to unicast slave
**               w_seqId   (IN) - sequence id for response
**               e_msgType (IN) - requested message type
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void UCM_RespUcCnclAck( const PTP_t_PortAddr *ps_pAddr,
                        UINT16               w_ifIdx,
                        UINT16               w_seqId, 
                        PTP_t_msgTypeEnum    e_msgType);

/***********************************************************************
**  
** Function    : UCM_IsSyncSrvGranted
**  
** Description : Returns, if a sync service is granted for the requested
**               service.
**  
** Parameters  : ps_pAddr (IN) - port address of requestor
**               
** Returnvalue : TRUE               - sync service is granted
**               FALSE              - sync servic is not granted
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN UCM_IsSyncSrvGranted(const PTP_t_PortAddr *ps_pAddr);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __UCMINT_H__ */

