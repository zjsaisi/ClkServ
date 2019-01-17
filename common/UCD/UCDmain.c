/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: UCDmain.c  
**    Summary: The unicast discovery task is responsible to establish
**             the connection between unicast masters and unicast slaves. 
**             First announce messages get requested from the master.
**             If the BMC algorithm detected
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
**             regServReq
**             SearchNode
**             deleteServReq
**             SendUcReq
**             HandleSrvGrnt
**             SendUcCancel
**             RespUcCnclAck
**             SndCTLMstNotActbl
**
**   Compiler: Ansi-C
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

/*************************************************************************
**    compiler directives
*************************************************************************/

/*************************************************************************
**    include-files
*************************************************************************/
#include "target.h"
#include "PTP/PTPdef.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "SIS/SIS.h"
#include "GOE/GOE.h"
#include "DIS/DIS.h"
#include "UCD/UCD.h"
#include "UCD/UCDint.h"

/* just for unicast enabled */
#if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC)))
/*************************************************************************
**    global variables
*************************************************************************/
static PTP_t_PortId ks_pId_FF = {{{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}},0xFFFF};
/* port Ids of all local ports */
PTP_t_PortId UCD_as_portId[k_NUM_IF];

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
static UCD_t_srvReq s_srvReqHead; /* dynamic list head */
/*************************************************************************
**    static function-prototypes
*************************************************************************/
static void regServReq( const PTP_t_PortAddr *ps_pAddr,
                        const PTP_t_PortId   *ps_pId,
                        UCD_t_srvReq         *ps_listHead,
                        PTP_t_msgTypeEnum    e_msgType,
                        INT8                 c_logIntv);
static BOOLEAN SearchNode(UCD_t_srvReq   *ps_listHead,
                          const PTP_t_PortAddr *ps_pAddr,
                          UCD_t_srvReq   **pps_fndNode);
static void deleteServReq(const UCD_t_srvReq *ps_req);
static void SendUcReq(const UCD_t_srvReq *ps_srvReq,UCD_t_srv *ps_srv);
static void HandleSrvGrnt(const PTP_t_MboxMsg *ps_msg,
                          PTP_t_msgTypeEnum   e_msgType,
                          UCD_t_srvReq        *ps_listHead);
static void SendUcCancel( const UCD_t_srvReq* ps_srvReq);
static void RespUcCnclAck( const PTP_t_PortAddr *ps_pAddr,
                           UINT16               w_ifIdx,
                           UINT16               w_seqId, 
                           PTP_t_msgTypeEnum    e_msgType);
static void SndCTLMstNotActbl( UINT16 w_rcvIfIdx,
                               const PTP_t_PortId   *ps_pId);

/*************************************************************************
**    global functions
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
void UCD_Init( const PTP_t_PortId *ps_portId)
{
  UINT16 w_ifIdx;
  /* initialize port id array */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    UCD_as_portId[w_ifIdx] = ps_portId[w_ifIdx];
  }
  /* initialize service request list head */ 
  s_srvReqHead.pv_next = NULL;
  s_srvReqHead.pv_prev = NULL;
}

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
void UCD_Close( void )
{
  UCD_t_srvReq *ps_srvReq;
  /* set a pointer to the head of the request list */
  ps_srvReq = (UCD_t_srvReq*)s_srvReqHead.pv_next;
  /* send all needed requests to all unicast masters */
  while( ps_srvReq != NULL )
  {
    /* send unicast cancel */
    SendUcCancel(ps_srvReq);
    /* next request */
    ps_srvReq = (UCD_t_srvReq*)ps_srvReq->pv_next;
  }
}

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
void UCD_Task( UINT16 w_hdl )
{
  UINT16            w_i;
  UCD_t_srvReq      *ps_srvReq,*ps_srvDel;
  PTP_t_PortAddr    *ps_pAddr;
  PTP_t_PortId      *ps_pId;
  PTP_t_msgTypeEnum e_msgType;
  UINT16            w_rcvIfIdx;
  static INT8                 c_logIntv;
  static PTP_t_MboxMsg        *ps_msg;
  static UINT32               dw_ticks;
  static PTP_t_PortAddrQryTbl *ps_ucMstTbl = NULL;   
    
  /* multitasking restart point - generates a 
     jump to the last BREAK at env */
  SIS_Continue(w_hdl);   
    /* initialize service request list head */ 
    s_srvReqHead.pv_next = NULL;
    s_srvReqHead.pv_prev = NULL;
  
  while( TRUE ) /*lint !e716 */
  {       
    /* got ready with message arrival? */
    while( ( ps_msg = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    {
      /* complete unicast master table */
      if( ps_msg->e_mType == e_IMSG_UC_MST_TBL )
      {
        /* first delete all registered masters out of table */
        ps_srvReq = (UCD_t_srvReq*)s_srvReqHead.pv_next;
        while( ps_srvReq != NULL )
        {
          /* remember unicast service */
          ps_srvDel = ps_srvReq;
          /* get pointer to next */
          ps_srvReq = (UCD_t_srvReq*)ps_srvReq->pv_next;
          /* send unicast cancel */
          SendUcCancel(ps_srvDel);
          /* delete service request */
          deleteServReq(ps_srvDel);
        }
        /* initialize unicast master table pointer */
        ps_ucMstTbl = ps_msg->s_pntData.ps_pAQTbl;
        /* get requested interval for announce messages */
        c_logIntv = ps_msg->s_etc1.c_s8;
        /* request announce messages of all potential 
           masters in the unicast master table */
        for( w_i = 0 ; w_i < ps_ucMstTbl->w_actTblSze ; w_i++ )
        {
          /* call request function */
          if( ps_ucMstTbl->aps_pAddr[w_i] != NULL )
          {
            /* register announce messages from unicast master */
            regServReq(ps_ucMstTbl->aps_pAddr[w_i],NULL,
                       &s_srvReqHead,e_MT_ANNOUNCE,c_logIntv);
          }
        }
        /* get query interval */
        dw_ticks = PTP_getTimeIntv(ps_ucMstTbl->c_logQryIntv);
        /* start query timer */
        SIS_TimerStart(w_hdl,2);
      }
      /* New request for sync and delay service 
         to be sent to an external unicast master */
      else if( ps_msg->e_mType == e_IMSG_REQ_UC_SRV )
      {
        /* cancel current sync and delay lease */
        ps_srvReq = (UCD_t_srvReq*)s_srvReqHead.pv_next;
        while( ps_srvReq != NULL )
        {
          /* sync request */
          if( ps_srvReq->s_srvSync.o_grnt == TRUE )
          {
            /* cancel sync sercice */
            UCD_SendUcCancel(&ps_srvReq->s_pAddr,
                              ps_srvReq->w_ifIdx,
                              ps_srvReq->s_srvSync.w_seqId,
                              e_MT_SYNC);
            ps_srvReq->s_srvSync.o_grnt = FALSE;
          }
          /* delay response */
          if( ps_srvReq->s_srvDel.o_grnt == TRUE )
          {
            /* cancel sercice */
            UCD_SendUcCancel(&ps_srvReq->s_pAddr,
                              ps_srvReq->w_ifIdx,
                              ps_srvReq->s_srvDel.w_seqId,
                              ps_srvReq->s_srvDel.e_msgType);
            ps_srvReq->s_srvDel.o_grnt = FALSE;
          }
          /* get pointer to next */
          ps_srvReq = (UCD_t_srvReq*)ps_srvReq->pv_next;
        }

        /* get port address of unicast master */
        ps_pAddr = ps_msg->s_pntData.ps_pAddr;
        /* get port id of unicast master */
        ps_pId   = ps_msg->s_pntExt1.ps_pId;
        /* register sync request from unicast master */
        regServReq(ps_pAddr,ps_pId,&s_srvReqHead,e_MT_SYNC,ps_msg->s_etc1.c_s8);
        /* register delay request from unicast master */
#if( k_CLK_DEL_E2E == TRUE )
        regServReq(ps_pAddr,ps_pId,&s_srvReqHead,e_MT_DELRESP,ps_msg->s_etc2.c_s8);
#else
        regServReq(ps_pAddr,ps_pId,&s_srvReqHead,e_MT_PDELRESP,ps_msg->s_etc2.c_s8); 
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
      }
      /* received grant for announce messages */
      else if( ps_msg->e_mType == e_IMSG_UC_GRNT_ANC )
      {
        /* handle the grant */
        HandleSrvGrnt(ps_msg,e_MT_ANNOUNCE,&s_srvReqHead);        
      }
      /* received grant for sync messages */
      else if( ps_msg->e_mType == e_IMSG_UC_GRNT_SYN )
      {
        /* handle the grant */
        HandleSrvGrnt(ps_msg,e_MT_SYNC,&s_srvReqHead);
      }
      /* received grant for delay messages */
      else if( ps_msg->e_mType == e_IMSG_UC_GRNT_DEL )
      {
        /* handle the grant */
#if( k_CLK_DEL_E2E == TRUE )
        HandleSrvGrnt(ps_msg,e_MT_DELRESP,&s_srvReqHead);
#else
        HandleSrvGrnt(ps_msg,e_MT_PDELRESP,&s_srvReqHead);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
      }
      /* unicast cancel of the master */
      else if( ps_msg->e_mType == e_IMSG_UC_CNCL )
      {
        /* get message type to cancel */
        e_msgType = ps_msg->s_etc1.e_extMType;
        /* get interface */
        w_rcvIfIdx = ps_msg->s_etc3.w_u16;
        /* get port address */
        ps_pAddr = ps_msg->s_pntData.ps_pAddr;
        /* get port id */
        ps_pId   = ps_msg->s_pntExt1.ps_pId;
        /* search node */
        if( SearchNode(&s_srvReqHead,ps_pAddr,&ps_srvReq) == TRUE )
        {
          /* send a message to the unit CTL - this master is not
             acceptable */
          SndCTLMstNotActbl(w_rcvIfIdx,ps_pId);
          /* delete granted service out of list */
          deleteServReq(ps_srvReq);
          /* send ack */
          RespUcCnclAck(ps_pAddr,w_rcvIfIdx,0,e_msgType);          
        }
        /* free allocated memory */
        SIS_Free(ps_pId);
        SIS_Free(ps_pAddr);
      }
      else
      {
        /* set error */
        PTP_SetError(k_UCD_ERR_ID,UCD_k_MSG_ERR,e_SEVC_NOTC);
      }      
      /* release mbox entry  */
      SIS_MboxRelease();
    } 
    /* got ready with elapsed timer?  */
    if( SIS_TimerStsRead( w_hdl ) == SIS_k_TIMER_EXPIRED )
    {
      /* set a pointer to the head of the request list */
      ps_srvReq = (UCD_t_srvReq*)s_srvReqHead.pv_next;
      /* send all needed requests to all unicast masters */
      while( ps_srvReq != NULL )
      {
        /* announce request */
        SendUcReq(ps_srvReq,&ps_srvReq->s_srvAnnc);
        /* sync request */
        SendUcReq(ps_srvReq,&ps_srvReq->s_srvSync);
        /* delay request */
        SendUcReq(ps_srvReq,&ps_srvReq->s_srvDel);
        /* next request */
        ps_srvReq = (UCD_t_srvReq*)ps_srvReq->pv_next;
      }
      /* restart timer */
      SIS_TimerStart(w_hdl,dw_ticks);
    }
    /* cooperative multitasking 
       set task to blocked, till it gets ready through an event,
       message or timeout  */
    SIS_Break(w_hdl,1); /*lint !e646 !e717*/ 
  }
  SIS_Return(w_hdl,0);/*lint !e744*/
}

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
                      PTP_t_msgTypeEnum    e_msgType)
{
  UINT8 ab_tlvArr[k_CNCL_UC_TRM_TLV_SZE];

  /* pack header into tlv */
  NIF_PackTlv(ab_tlvArr,e_TLVT_CNCL_UC_TRM,
              k_CNCL_UC_TRM_TLV_SZE - k_UC_TLV_TYPE_LEN_SZE);
  /* pack message type */
  ab_tlvArr[k_UC_TLV_TYPE_LEN_SZE] = ((UINT8)e_msgType << 4); /*lint !e734 */
  ab_tlvArr[k_UC_TLV_TYPE_LEN_SZE+1] = 0;
  /* send signaling message */
  DIS_Send_Sign(ps_pAddr,
                &UCD_as_portId[w_ifIdx],
                &ks_pId_FF,
                w_ifIdx,
                w_seqId,
                ab_tlvArr,
                k_CNCL_UC_TRM_TLV_SZE);
}

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
const CHAR* UCD_GetErrStr(UINT32 dw_errNmb)
{
  const CHAR* pc_ret;
  
  /* resolve error number to error string */
  switch(dw_errNmb)
  {
    case UCD_k_MSG_ERR:   
    {
      pc_ret = "Misplaced task message";
      break;
    }
    case UCD_k_MEM_ERR:
    {
      pc_ret = "SIS memory allocation error";
      break;
    }
    case UCD_k_MBOX_ERR:
    {
      pc_ret = "Message box error";
      break;
    }
    default:
    {
      pc_ret = "unknown error";
      break;
    }
  }
  return pc_ret;
}
#endif

/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
**
** Function    : regServReq
**
** Description : Registers a service request in the appropriate 
**               service request list.
**              
** Parameters  : ps_pAddr    (IN) - pointer to unicast master port address
**               ps_pId      (IN) - pointer to port id of master
**               ps_listHead (IN) - pointer to head of service request list
**               e_msgType   (IN) - message type of request
**               c_logIntv   (IN) - log2 of message interval to request
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
static void regServReq( const PTP_t_PortAddr *ps_pAddr,
                        const PTP_t_PortId   *ps_pId,
                        UCD_t_srvReq         *ps_listHead,
                        PTP_t_msgTypeEnum    e_msgType,
                        INT8                 c_logIntv)
{
  UCD_t_srvReq *ps_req = NULL;
  UCD_t_srvReq *ps_next;
  UCD_t_srv    *ps_srv;
  
  /* search unicast master in list */
  if( SearchNode(ps_listHead,ps_pAddr,&ps_req) != TRUE )
  {
    /* allocate memory for new service request */
    ps_req = (UCD_t_srvReq*)SIS_Alloc(sizeof(UCD_t_srvReq));
    if( ps_req != NULL )
    {
      /* store port address */
      ps_req->s_pAddr.w_AddrLen   = ps_pAddr->w_AddrLen;
      ps_req->s_pAddr.e_netwProt  = ps_pAddr->e_netwProt;
      PTP_BCOPY( ps_req->s_pAddr.ab_Addr,
                ps_pAddr->ab_Addr,
                ps_pAddr->w_AddrLen);
      /* set target clock id to all 1s - not known */
      PTP_MEMSET(ps_req->s_pId.s_clkId.ab_id,0xFF,k_CLKID_LEN);
      ps_req->s_pId.w_portNmb     = 0xFFFF;
      /* initialize services */
      /* announce service */
      ps_req->s_srvAnnc.e_msgType = e_MT_ANNOUNCE;
      ps_req->s_srvAnnc.o_req     = FALSE;
      ps_req->s_srvAnnc.o_grnt    = FALSE;
      ps_req->s_srvAnnc.dw_prdEnd = 0;
      ps_req->s_srvAnnc.w_seqId   = 0;
      /* sync service */
      ps_req->s_srvSync.e_msgType = e_MT_SYNC;
      ps_req->s_srvSync.o_req     = FALSE;
      ps_req->s_srvSync.o_grnt    = FALSE;
      ps_req->s_srvSync.dw_prdEnd = 0;
      ps_req->s_srvSync.w_seqId   = 0;
      /* delay request service */
#if( k_CLK_DEL_E2E == TRUE )  
      ps_req->s_srvDel.e_msgType  = e_MT_DELRESP;
#else
      ps_req->s_srvDel.e_msgType  = e_MT_PDELRESP;
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
      ps_req->s_srvDel.o_req      = FALSE;
      ps_req->s_srvDel.o_grnt     = FALSE;
      ps_req->s_srvDel.dw_prdEnd  = 0;
      ps_req->s_srvDel.w_seqId    = 0;
      /* as interface is unknown take interface 0 
         the ip table sends it to the correct interface */
      ps_req->w_ifIdx             = 0;
       /* store request in list */
      ps_req->pv_next      = ps_listHead->pv_next;
      ps_next              = (UCD_t_srvReq*)ps_req->pv_next;
      if( ps_next != NULL )
      {
        ps_next->pv_prev   = ps_req;
      }
      ps_req->pv_prev      = (void*)ps_listHead;
      ps_listHead->pv_next = (void*)ps_req;
    }
    else
    {
      /* set mem error */
      PTP_SetError(k_UCD_ERR_ID,UCD_k_MEM_ERR,e_SEVC_NOTC);
    }
  }
  /* if not found and no resources available, refuse request */
  if( ps_req != NULL )
  { 
    /* insert new port id */
    if( ps_pId != NULL ) 
    {
      ps_req->s_pId = *ps_pId;
    }
    /* register service */
    switch( e_msgType )
    {
      case e_MT_ANNOUNCE:
      {
        ps_srv = &ps_req->s_srvAnnc;
        break;
      }
      case e_MT_SYNC:
      {
        ps_srv = &ps_req->s_srvSync;
        break;
      }
      case e_MT_DELRESP:
      case e_MT_PDELRESP:
      {
        ps_srv = &ps_req->s_srvDel;
        break;
      }
      default:
      {
        ps_srv = NULL;
      }
    }/*lint !e788 */
    if( ps_srv != NULL )
    {
      ps_srv->o_req     = TRUE;
      ps_srv->c_logIntv = c_logIntv;
    } 
  }
}

/***********************************************************************
**
** Function    : SearchNode
**
** Description : Searches for an unicast master in the service request
**               list.
**              
** Parameters  : ps_listHead  (IN)  - pointer to service request array head
**               ps_pAddr     (IN)  - pointer to unicast master to search
**               pps_fndNode  (OUT) - pointer to found service request
**
** Returnvalue : TRUE              - slave found
**               FALSE             - master found
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static BOOLEAN SearchNode(UCD_t_srvReq   *ps_listHead,
                          const PTP_t_PortAddr *ps_pAddr,
                          UCD_t_srvReq   **pps_fndNode)
{
  BOOLEAN       o_fnd = FALSE;
  UCD_t_srvReq  *ps_req;
  
  /* preset pointer of found request to zero */
  *pps_fndNode = NULL;
  /* set iterator to beginning of list */
  ps_req = ps_listHead;
  while( ps_req->pv_next != NULL )
  {
    ps_req = (UCD_t_srvReq*)ps_req->pv_next;
    /* compare port addresses */
    if( PTP_CompPortAddr(&ps_req->s_pAddr,ps_pAddr) == PTP_k_SAME )
    {
      /* set pointer to found slave */
      *pps_fndNode = ps_req;
      /* set return value to success */
      o_fnd = TRUE;
      /* break loop */
      break;
    }
  }
  return o_fnd;
}

/***********************************************************************
**
** Function    : deleteServReq
**
** Description : Deletes unicast master service request out of 
**               doulbe linked list.
**              
** Parameters  : ps_req     (IN) - pointer to unicast master request
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static void deleteServReq(const UCD_t_srvReq *ps_req)
{
  UCD_t_srvReq *ps_prev;
  UCD_t_srvReq *ps_next;

  /* get out of list */
  ps_prev = (UCD_t_srvReq*)ps_req->pv_prev;
  ps_next = (UCD_t_srvReq*)ps_req->pv_next;
  ps_prev->pv_next = (void*)ps_next;
  if( ps_next != NULL )
  {
    ps_next->pv_prev = (void*)ps_prev;
  }
  /* free allocated memory */
  SIS_Free((void*)ps_req);
}

/***********************************************************************
**  
** Function    : SendUcReq
**  
** Description : Sends all needed requests to the unicast master.
**  
** Parameters  : ps_srvReq (IN) - pointer to unicast master requests
**               ps_srv    (IN) - pointer to requested service
**               
** Returnvalue : -
** 
** Remarks     : function is reentrant
**  
***********************************************************************/
static void SendUcReq(const UCD_t_srvReq *ps_srvReq,UCD_t_srv *ps_srv)
{
  UINT8 ab_tlvArr[k_REQ_UC_TRM_TLV_SZE];

  /* check, if service has to be re-requested */
// jyang: if announce is not granted, should request again if time expired
  if( ps_srv->o_grnt == TRUE || ps_srv->e_msgType == e_MT_ANNOUNCE )
  {
    if( SIS_TIME_OVER(ps_srv->dw_prdEnd) )
    {
      /* set flags */
      ps_srv->o_grnt = FALSE;
      ps_srv->o_req = TRUE;
    }
  }
  /* send request, if needed */
  if( ps_srv->o_req == TRUE )
  {
    /* pack header into tlv */
    NIF_PackTlv(ab_tlvArr,e_TLVT_REQ_UC_TRM,
                k_REQ_UC_TRM_TLV_SZE-k_UC_TLV_TYPE_LEN_SZE);
    /* pack message type */
    ab_tlvArr[k_UC_TLV_TYPE_LEN_SZE] = ((UINT8)ps_srv->e_msgType << 4); /*lint !e734 */
    ab_tlvArr[5] = (UINT8)ps_srv->c_logIntv;
    GOE_hton32(&ab_tlvArr[6],k_UC_TRNSMS_DUR_SEC);
    /* send signaling message */
    DIS_Send_Sign(&ps_srvReq->s_pAddr,
                  &UCD_as_portId[ps_srvReq->w_ifIdx],
                  &ks_pId_FF,
                  ps_srvReq->w_ifIdx,
                  ps_srv->w_seqId,
                  ab_tlvArr,
                  k_REQ_UC_TRM_TLV_SZE);
    /* increment sequence id */
    ps_srv->w_seqId++;
  }   
}

/***********************************************************************
**  
** Function    : HandleSrvGrnt
**  
** Description : Handles a unicast service grant.
**  
** Parameters  : ps_msg      (IN) - pointer to received internal message
**               e_msgType   (IN) - message type that was granted
**               ps_listHead (IN) - head of service request linked list 
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void HandleSrvGrnt(const PTP_t_MboxMsg *ps_msg,
                          PTP_t_msgTypeEnum   e_msgType,
                          UCD_t_srvReq        *ps_listHead)
{
  UCD_t_srvReq   *ps_req = NULL;
  UCD_t_srv      *ps_srv;
  PTP_t_PortAddr *ps_pAddr;
  UINT64         ddw_prdTicks;
  UINT32         dw_grntdSec;

  /* get pointer to port address of sender */
  ps_pAddr = ps_msg->s_pntData.ps_pAddr;

  /* search unicast master in list */
  if( SearchNode(ps_listHead,ps_pAddr,&ps_req) == TRUE )
  {
    /* register service */
    switch( e_msgType )
    {
      case e_MT_ANNOUNCE:
      {
        ps_srv = &ps_req->s_srvAnnc;
        break;
      }
      case e_MT_SYNC:
      {
        ps_srv = &ps_req->s_srvSync;
        break;
      }
      case e_MT_DELRESP:
      case e_MT_PDELRESP:
      {
        ps_srv = &ps_req->s_srvDel;
        break;
      }
      default:
      {
        ps_srv = NULL;
      }
    }/*lint !e788 */
    if( ps_srv != NULL )
    {
      /* check, if service is granted or denied */
      dw_grntdSec = ps_msg->s_etc1.dw_u32;
      /* if service is granted */
      if( dw_grntdSec > 0 )
      {
        /* get end of granted interval in SIS ticks */
        ddw_prdTicks = ((UINT64)dw_grntdSec * k_NSEC_IN_SEC) / k_PTP_TIME_RES_NSEC;
        /* scale the timespan to 90 % */
        // jyang: changed to 50%
        //ddw_prdTicks = (ddw_prdTicks * 9) / 10;
        ddw_prdTicks = ddw_prdTicks >> 1;
        ps_srv->dw_prdEnd = SIS_GetTime() + (UINT32)ddw_prdTicks; 
        /* get granted interval */
        ps_srv->c_logIntv = ps_msg->s_etc2.c_s8;
        /* set flag that this request is granted */
        ps_srv->o_grnt    = TRUE;    
        /* reset flag to request the service */
        ps_srv->o_req     = FALSE;
      }
      /* if service is not granted */
      else
      {
        ps_srv->o_grnt = FALSE;
        /* if it is a sync service, CTL unit must know that.
           It will not be an acceptable master */
        if( e_msgType != e_MT_ANNOUNCE )
        {
          /* stop requesting it */
          ps_srv->o_req = FALSE;
          /* send message to CTL - this master is not acceptable */
          SndCTLMstNotActbl(ps_msg->s_etc3.w_u16,&ps_req->s_pId);          
        }
        else
        {
          /* stop requesting it and try it later */
          ps_srv->o_req     = FALSE;
          /* get master service request retry time */
          ddw_prdTicks = k_MST_RETRY_TIME_SEC * (k_NSEC_IN_SEC / k_PTP_TIME_RES_NSEC);
          ps_srv->dw_prdEnd = SIS_GetTime() + (UINT32)ddw_prdTicks; 
        }
      }
    }
  }
  /* free allocated memory */
  SIS_Free(ps_pAddr);
}

/***********************************************************************
**  
** Function    : SendUcCancel
**  
** Description : Sends a unicast cancel of all granted services to
**               the unicast master.
**  
** Parameters  : ps_srvReq (IN) - holds the master port id and all services
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void SendUcCancel( const UCD_t_srvReq* ps_srvReq)
{
  /* announce request */
  if(ps_srvReq->s_srvAnnc.o_grnt == TRUE )
  {
    /* cancel announce sercice */
    UCD_SendUcCancel(&ps_srvReq->s_pAddr,
                      ps_srvReq->w_ifIdx,
                      ps_srvReq->s_srvAnnc.w_seqId,
                      e_MT_ANNOUNCE);
  }
  /* sync request */
  if(ps_srvReq->s_srvSync.o_grnt == TRUE )
  {
    /* cancel sync sercice */
    UCD_SendUcCancel(&ps_srvReq->s_pAddr,
                      ps_srvReq->w_ifIdx,
                      ps_srvReq->s_srvSync.w_seqId,
                      e_MT_SYNC);
  }
  /* delay response */
  if(ps_srvReq->s_srvDel.o_grnt == TRUE )
  {
    /* cancel sercice */
    UCD_SendUcCancel(&ps_srvReq->s_pAddr,
                      ps_srvReq->w_ifIdx,
                      ps_srvReq->s_srvDel.w_seqId,
                      ps_srvReq->s_srvDel.e_msgType);
  }
}

/***********************************************************************
**
** Function    : RespUcCnclAck
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
static void RespUcCnclAck( const PTP_t_PortAddr *ps_pAddr,
                           UINT16               w_ifIdx,
                           UINT16               w_seqId, 
                           PTP_t_msgTypeEnum    e_msgType)
{
  UINT8        ab_data[6];
  PTP_t_PortId s_pId;

  /* set port id of slave to unknown */
  PTP_MEMSET(s_pId.s_clkId.ab_id,0xFF,k_CLKID_LEN);
  s_pId.w_portNmb     = 0xFFFF;
  
  /* pack header into tlv */
  NIF_PackTlv(ab_data,e_TLVT_ACK_CNCL_UC_TRM,2);
  /* pack message type */
  ab_data[4] = ((UINT8)e_msgType << 4);
  /* reserved field */
  ab_data[5] = 0;
  /* send it to slave */
  DIS_Send_Sign(ps_pAddr,
                &UCD_as_portId[w_ifIdx],
                &s_pId,
                w_ifIdx,
                w_seqId,
                ab_data,
                6);
}

/***********************************************************************
**  
** Function    : SndCTLMstNotActbl
**  
** Description : 
**  
** Parameters  : w_rcvIfIdx (IN) - receiveing interface index
**               ps_pId     (IN) - port id of master
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void SndCTLMstNotActbl( UINT16 w_rcvIfIdx,
                               const PTP_t_PortId   *ps_pId)
{
  PTP_t_MboxMsg s_msg;
  PTP_t_PortId  *ps_pIdCopy;

  /* allocate memory */
  ps_pIdCopy = (PTP_t_PortId*)SIS_Alloc(sizeof(PTP_t_PortId));
  if( ps_pId != NULL )
  {
    /* copy port id */
    *ps_pIdCopy = *ps_pId;
    /* prepare message */
    s_msg.e_mType            = e_IMSG_ACTBL_MST;
    s_msg.s_etc1.o_bool      = FALSE; /* is not acceptable */
    s_msg.s_etc2.o_bool      = TRUE; /* is unicast */
    s_msg.s_etc3.w_u16       = w_rcvIfIdx; 
    s_msg.s_pntExt1.ps_pId   = ps_pIdCopy;
    /* send it to CTL task */
    if( SIS_MboxPut(CTL_TSK,&s_msg) == FALSE )
    {
      /* free allocated memory */
      SIS_Free(ps_pId);
      /* set error */
      PTP_SetError(k_UCD_ERR_ID,UCD_k_MBOX_ERR,e_SEVC_ERR);
    }
  }
}

void UCD_SetAnncInt(INT8 c_logIntv)
{
  UCD_t_srvReq *ps_srvReq;

  ps_srvReq = (UCD_t_srvReq*)&s_srvReqHead;
  while( ps_srvReq != NULL )
  {
    ps_srvReq->s_srvAnnc.c_logIntv = c_logIntv;
    ps_srvReq->s_srvAnnc.o_req = TRUE;
    ps_srvReq = (UCD_t_srvReq*)ps_srvReq->pv_next;
  }
}

void UCD_SetSyncInt(INT8 c_logIntv)
{
  UCD_t_srvReq *ps_srvReq;

  ps_srvReq = (UCD_t_srvReq*)&s_srvReqHead;
  while( ps_srvReq != NULL )
  {
    ps_srvReq->s_srvSync.c_logIntv = c_logIntv;
    ps_srvReq->s_srvSync.o_req = TRUE;
    ps_srvReq = (UCD_t_srvReq*)ps_srvReq->pv_next;
  }
}

#endif /* #if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC))) */

