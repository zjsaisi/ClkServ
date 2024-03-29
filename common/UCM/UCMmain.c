/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: UCMmain.c  
**    Summary: The unit UCM encapsulates all master activities of a 
**             unicastPTP node.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: UCM_Init
**             UCM_initSrvReqList
**             UCM_regServReq
**             UCM_unregServReq
**             UCM_SrchSlv
**             UCM_recalcPresc
**             UCM_checkPeriod
**             UCM_RespUcRequest
**             UCM_RespUcCnclAck
**             UCM_GetErrStr
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
#include "UCM/UCM.h"
#include "UCM/UCMint.h"


/* just for unicast enabled */
#if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC)))
/*************************************************************************
**    global variables
*************************************************************************/

/* the connected channel port ids */
PTP_t_PortId UCM_as_pId[k_NUM_IF];

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**
** Function    : UCM_Init
**
** Description : Initializes the Unit UCM 
**              
** Parameters  : ps_portId       (IN) - pointer to the port id�s
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void UCM_Init( const PTP_t_PortId *ps_portId)
{
  UINT16 w_ifIdx; 
  /* initilize port id of all interfaces */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    UCM_as_pId[w_ifIdx] = ps_portId[w_ifIdx];
  } 
  return;
}

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
void UCM_initSrvReqList(UCM_t_slvSrv *ps_listHead)
{
  ps_listHead->c_intv             = 0;
  ps_listHead->dw_prdEnd          = 0;
  ps_listHead->pv_prev            = NULL;
  ps_listHead->pv_next            = NULL;
  ps_listHead->s_pAddr.w_AddrLen  = 0;
  ps_listHead->s_pAddr.e_netwProt = e_UNKNOWN;
  PTP_MEMSET(ps_listHead->s_pAddr.ab_Addr,'\0',k_MAX_NETW_ADDR_SZE);
  ps_listHead->w_actCnt           = 0;
  ps_listHead->w_presc            = 0;
}

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
                     INT8                c_minIntv)
{
  PTP_t_PortAddr *ps_pAddr;
  UCM_t_slvSrv   *ps_srv = NULL;
  UCM_t_slvSrv   *ps_next;
  UINT64         ddw_ticks;
  UINT16         w_ifIdx;
  
  /* get pointer to port address */
  ps_pAddr = ps_msg->s_pntData.ps_pAddr;
  /* search slave in list */
  if( UCM_SrchSlv(ps_listHead,ps_pAddr,&ps_srv) != TRUE )
  {
    /* allocate memory for new service request */
    ps_srv = (UCM_t_slvSrv*)SIS_Alloc(sizeof(UCM_t_slvSrv));
    if( ps_srv != NULL )
    {
      /* store port address */
      ps_srv->s_pAddr.w_AddrLen  = ps_pAddr->w_AddrLen;
      ps_srv->s_pAddr.e_netwProt = ps_pAddr->e_netwProt;
      PTP_BCOPY( ps_srv->s_pAddr.ab_Addr,
                ps_pAddr->ab_Addr,
                ps_pAddr->w_AddrLen);
      /* set sequence id of the new request to zero */
      ps_srv->w_seqIdSrv  = 0;
      ps_srv->w_seqIdSign = 0;
      /* store request in list */
      ps_srv->pv_next      = ps_listHead->pv_next;
      ps_next              = (UCM_t_slvSrv*)ps_srv->pv_next;
      if( ps_next != NULL )
      {
        ps_next->pv_prev   = ps_srv;
      }
      ps_srv->pv_prev      = (void*)ps_listHead;
      ps_listHead->pv_next = (void*)ps_srv;
    }
  }  
  /* get receivuing interface index */
  w_ifIdx = ps_msg->s_etc3.w_u16;
  /* if not found and no resources available, refuse request */
  if( ps_srv == NULL )
  {  
    /* refuse service request */
    UCM_RespUcRequest(ps_pAddr,w_ifIdx,0,0,c_minIntv,e_msgType);
  }
  else
  { 
    /* calculate ticks out of requested seconds */
    ddw_ticks         = (UINT64)ps_msg->s_etc1.dw_u32 * (k_NSEC_IN_SEC / k_PTP_TIME_RES_NSEC);
    /* store service period end time */
    ps_srv->dw_prdEnd = SIS_GetTime() + (UINT32)ddw_ticks;
    /* recalculate duration in seconds */
    ddw_ticks = ps_srv->dw_prdEnd - SIS_GetTime();
    ps_srv->dw_durSec   = (UINT32)((ddw_ticks * k_PTP_TIME_RES_NSEC) / k_NSEC_IN_SEC);
    /* actualize interval */
    ps_srv->c_intv    = ps_msg->s_etc2.c_s8;
    /* set interface index to use */
    ps_srv->w_ifIdx   = w_ifIdx;
    /* check, if this is the smallest interval now */
    if( ps_srv->c_intv < c_minIntv )
    {
      /* get new minimal interval */
      c_minIntv = ps_srv->c_intv;      
    }
    /* get prescaler value */
    ps_srv->w_presc   = (UINT16)(1u << (ps_srv->c_intv - c_minIntv));
    /* recalculate intervals and prescalers */
    UCM_recalcPresc(ps_listHead,c_minIntv);
    /* set actual count to start the service immediately */
    ps_srv->w_actCnt  = 1;  
    /* grant service request */
    UCM_RespUcRequest(&ps_srv->s_pAddr,
                      ps_srv->w_ifIdx,
                      ps_srv->w_seqIdSign,
                      ps_srv->dw_durSec,
                      ps_srv->c_intv,
                      e_msgType);
  }
  /* free allocated memory */
  SIS_Free(ps_pAddr);
  return c_minIntv;
}

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
void UCM_unregServReq(const UCM_t_slvSrv *ps_srv)
{
  UCM_t_slvSrv *ps_prev;
  UCM_t_slvSrv *ps_next;

  /* get out of list */
  ps_prev = (UCM_t_slvSrv*)ps_srv->pv_prev;
  ps_next = (UCM_t_slvSrv*)ps_srv->pv_next;
  
  ps_prev->pv_next = (void*)ps_next;
  if( ps_next != NULL )
  {
    ps_next->pv_prev = (void*)ps_prev;
  }
  /* free allocated memory */
  SIS_Free((void*)ps_srv);
}

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
                    UCM_t_slvSrv         **pps_fndSlv)
{
  BOOLEAN       o_fnd = FALSE;
  UCM_t_slvSrv  *ps_slv;
  
  /* preset pointer to found slave to zero */
  *pps_fndSlv = NULL;
  /* set iterator to beginning of array */
  ps_slv = ps_slvSrvArr;
  while( ps_slv->pv_next != NULL )
  {
    ps_slv = (UCM_t_slvSrv*)ps_slv->pv_next;
    /* compare port addresses */
    if( PTP_CompPortAddr(&ps_slv->s_pAddr,ps_pAddr) == PTP_k_SAME )
    {
      /* set pointer to found slave */
      *pps_fndSlv = ps_slv;
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
void UCM_recalcPresc(UCM_t_slvSrv *ps_slvSrvArr,INT8 c_minIntv)
{
  UCM_t_slvSrv  *ps_slv;
  UINT16        w_oldPresc;
  
  /* set iterator to beginning of array */
  ps_slv = ps_slvSrvArr;
  while( ps_slv->pv_next != NULL )
  {
    /* set iterator to next slave request */
    ps_slv = (UCM_t_slvSrv*)ps_slv->pv_next;
    /* store old prescaler value */
    w_oldPresc = ps_slv->w_presc;
    /* recalculate prescaler value */
    ps_slv->w_presc = (UINT16)((ps_slv->c_intv - c_minIntv) + 1);
    /* recalculate new counter */
    ps_slv->w_actCnt = (UINT16)((ps_slv->w_presc/w_oldPresc)*ps_slv->w_actCnt);
  }
}

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
void UCM_checkPeriod(const UCM_t_slvSrv *ps_srv,UINT32 dw_actTime)
{
  /* check if period is over */
  if( (dw_actTime - ps_srv->dw_prdEnd) < 0x80000000UL )
  {
    /* get out of list */
    UCM_unregServReq(ps_srv);
  }
}

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
                       PTP_t_msgTypeEnum    e_msgType)
{
  UINT8        ab_data[12];
  PTP_t_PortId s_pId;

  /* set port id of slave to unknown */
  PTP_MEMSET(s_pId.s_clkId.ab_id,0xFF,k_CLKID_LEN);
  s_pId.w_portNmb     = 0xFFFF;
  
  /* pack header into tlv */
  NIF_PackTlv(ab_data,e_TLVT_GRNT_UC_TRM,8);
  /* pack message type */
  ab_data[4] = ((UINT8)e_msgType << 4);
  /* pack granted interval */
  ab_data[5] = (UINT8)c_logIntv;
  /* pack granted duration */
  GOE_hton32(&ab_data[6],dw_durSec);
  /* set reserved field to zero */
  ab_data[10] = 0; 
  /* set renewal invited to TRUE */
  ab_data[11] = 1;
  /* send it to slave */
// jyang: temporarily block master output
//  DIS_Send_Sign(ps_pAddr,
//                &UCM_as_pId[w_ifIdx],
//                &s_pId,
//                w_ifIdx,
//                w_seqId,
//                ab_data,
//                12);
}

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
// jyang: temporarily block master output
//  DIS_Send_Sign(ps_pAddr,
//                &UCM_as_pId[w_ifIdx],
//                &s_pId,
//                w_ifIdx,
//                w_seqId,
//                ab_data,
//                6);
}

#ifdef ERR_STR
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
const CHAR* UCM_GetErrStr(UINT32 dw_errNmb)
{
  const CHAR* pc_ret;
                    
  /* handle errors */
  switch(dw_errNmb)
  { 
    case UCM_k_MEMERR:
    {
      pc_ret = "Allocation error";
      break;
    }
    case UCM_k_MSGERR:
    {
      pc_ret = "Misplaced internal message error";
      break;
    }
    case UCM_k_EVT_NDEF:
    {
      pc_ret = "Event is not defined for this task";
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
#endif /* #if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC))) */
