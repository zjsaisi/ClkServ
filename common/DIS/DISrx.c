/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: DISrx.c 
**    Summary: This module of the unit DIS defines all functions that are
**             used to get data of lower units and dispatch them to upper 
**             units. Also functions that operate on the lower units are
**             included.
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: DIS_DispEvntMsg
**             DIS_DispGnrlMsg
**             DIS_DispEvtMsgTc
**             DIS_DispGnrlMsgTc
**             DIS_WhoIsReceiver
**
**             DispTlv
**             HandleReqUcTrmTlv
**             HandleCnclUcTrmTlv
**             HandleGrantUcTrmTlv
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
#include "SIS/SIS.h"
#include "GOE/GOE.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "DIS/DIS.h"
#include "DIS/DISint.h"

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
static PTP_t_ClkId s_clkId_FF = {{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}};
/*************************************************************************
**    static function-prototypes
*************************************************************************/
static void DispTlv(const UINT8          *pb_tlv,
                    const PTP_t_PortAddr *ps_pAddr,
                    const PTP_t_PortId   *ps_pId,
                    UINT16               w_rcvIfIdx,
                    UINT16               w_len);
#if((k_UNICAST_CPBL==TRUE) && ((k_CLK_IS_OC==TRUE) || (k_CLK_IS_BC==TRUE)))
static void HandleReqUcTrmTlv(const UINT8          *pb_tlv,
                              const PTP_t_PortAddr *ps_pAddr,
                              UINT16               w_rcvIfIdx);
static void HandleCnclUcTrmTlv(const UINT8          *pb_tlv,
                               const PTP_t_PortAddr *ps_pAddr,
                               const PTP_t_PortId   *ps_pId,
                               UINT16               w_rcvIfIdx);
static void HandleGrantUcTrmTlv(const UINT8 *pb_tlv,
                                const PTP_t_PortAddr *ps_pAddr,
                                UINT16 w_rcvIfIdx);
/*#if((k_UNICAST_CPBL==TRUE)&&((k_CLK_IS_OC==TRUE)||(k_CLK_IS_BC == TRUE)))*/
#endif 
/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**
** Function    : DIS_DispEvntMsg
**
** Description : Filters and dispatches the messages  
**               depending on the message content and the actual port state.
**              
** Parameters  : w_ifIdx   (IN) - actual communication interface
**               pb_rxMsg  (IN) - rx message of this channel
**               ps_rxTs   (IN) - receive timestamp 
**               i_len     (IN) - amount of received data bytes
**               e_msgType (IN) - message type to dispatch
**               ps_pAddr  (IN) - port address of sender
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DIS_DispEvntMsg( UINT16               w_ifIdx,
                      const UINT8          *pb_rxMsg,
                      PTP_t_TmStmp         *ps_rxTs,
                      INT16                i_len,
                      PTP_t_msgTypeEnum    e_msgType,
                      const PTP_t_PortAddr *ps_pAddr)
{
  BOOLEAN        o_disp = TRUE;
  void           *pv_mem = NULL;
  PTP_t_PortAddr *ps_pAddrCpy = NULL;
#if(( k_CLK_IS_OC == TRUE )||(k_CLK_IS_BC == TRUE)||( k_CLK_DEL_P2P == TRUE))
  PTP_t_MboxMsg  s_msg;
#endif
#if(( k_CLK_IS_OC == TRUE )||(k_CLK_IS_BC == TRUE))
  PTP_t_stsEnum  e_portSts;
#endif  
#if( k_ALT_MASTER == FALSE )
  BOOLEAN        o_altMst;
  /* get alternate master flag */
  o_altMst = GET_FLAG(NIF_UNPACKHDR_FLAGS(pb_rxMsg),k_FLG_ALT_MASTER);
#endif /* #if( k_ALT_MASTER == FALSE ) */
#if(( k_CLK_IS_OC == TRUE )||(k_CLK_IS_BC == TRUE))
  /* check domain number */
  if( NIF_UNPACKHDR_DOMNMB(pb_rxMsg) != DIS_ps_ClkDs->s_DefDs.b_domn )
#else
  if( NIF_UNPACKHDR_DOMNMB(pb_rxMsg) != DIS_ps_ClkDs->s_TcDefDs.b_primDom )
#endif  
  {
    /* discard msg and timestamp */
    o_disp = FALSE;    
  }
  else
  {
#if(( k_CLK_IS_OC == TRUE )||(k_CLK_IS_BC == TRUE))
    /* Get the port state of the responsible port */ 
    e_portSts = DIS_ps_ClkDs->as_PortDs[w_ifIdx].e_portSts;
#endif
    /* preset flag to discard message */
    o_disp = FALSE;
    /* dispatch depending on port-state and message-type */ 
    switch( e_msgType )
    { 
#if(( k_CLK_IS_OC == TRUE )||(k_CLK_IS_BC == TRUE))
      case e_MT_SYNC:
      {
        /* if the port is not in slave or uncalibrated state 
           discard the message */
        if((( e_portSts != e_STS_SLAVE ) && 
            ( e_portSts != e_STS_UNCALIBRATED )) 
           || ( i_len  < (k_PTPV2_HDR_SZE + k_PTPV2_SYNC_PLD))
#if( k_ALT_MASTER == FALSE)
           || ( o_altMst == TRUE )
#endif /* #if( k_ALT_MASTER == FALSE) */
           )
        {
          /* discard msg and timestamp */
          o_disp = FALSE;
        }
        else
        {
          /* get memory */
          pv_mem = SIS_Alloc(sizeof(NIF_t_PTPV2_SyncMsg));
          if( pv_mem == NULL )
          {
            /* set mem error */ 
            PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_1,e_SEVC_WARN); 
            /* discard msg and timestamp */
            o_disp = FALSE;
          }
          else
          {
            /* unpack the message */
            NIF_UnPackSyncMsg((NIF_t_PTPV2_SyncMsg*)pv_mem,pb_rxMsg);
            /* set mbox-entry parameters */
            s_msg.s_pntData.pv_data   = pv_mem;  
            s_msg.s_pntExt1.ps_tmStmp = ps_rxTs;        
            s_msg.s_etc1.w_u16        = w_ifIdx;        
            s_msg.e_mType             = e_IMSG_SYNC;
            /* send message to sync task to synchronize */
            o_disp = SIS_MboxPut(SLS_TSK,&s_msg);                 
          }
        }
        break;
      }
/* just work with delay request and delay response messages, if E2E is used */ 
#if( k_CLK_DEL_E2E == TRUE )
      case e_MT_DELREQ:
      {
        if(( e_portSts != e_STS_MASTER) ||
           ( i_len      < (k_PTPV2_HDR_SZE + k_PTPV2_DLRQ_PLD)))
        {
          /* discard msg and timestamp */
          o_disp = FALSE;
        }
        else
        {
          /* get memory */
          pv_mem = SIS_Alloc(sizeof(NIF_t_PTPV2_DlrqMsg));
          if( pv_mem == NULL )
          {
            /* set mem error */ 
            PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_2,e_SEVC_WARN); 
            /* discard msg and timestamp */
            o_disp = FALSE;
          }
          else
          {
            /* unpack message */
            NIF_UnPackDelReqMsg((NIF_t_PTPV2_DlrqMsg*)pv_mem,pb_rxMsg);
#if( k_UNICAST_CPBL == TRUE )            
            /* copy port address of unicast messages 
               if it is a unicast address */
            if( GET_FLAG(((NIF_t_PTPV2_DlrqMsg*)pv_mem)->s_ptpHead.w_flags,
                         k_FLG_UNICAST) == TRUE )
            {              
              /* copy sender port address */
              ps_pAddrCpy = (PTP_t_PortAddr*)SIS_Alloc(sizeof(PTP_t_PortAddr));
              if( ps_pAddrCpy == NULL )
              {
                /* set error */
                PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_3,e_SEVC_NOTC);
              }
              else
              {
                /* copy port address */
                ps_pAddrCpy->w_AddrLen  = ps_pAddr->w_AddrLen;
                ps_pAddrCpy->e_netwProt = ps_pAddr->e_netwProt;
                PTP_BCOPY(ps_pAddrCpy->ab_Addr,
                          ps_pAddr->ab_Addr,
                          ps_pAddr->w_AddrLen);
              }
            }
            else
            {
              ps_pAddrCpy = NULL;
            }
#endif /* #if( k_UNICAST_CPBL == TRUE ) */            
            /* set mbox-entry parameters */
            s_msg.s_pntData.pv_data   = pv_mem;          
            s_msg.s_pntExt1.ps_tmStmp = ps_rxTs;
            s_msg.s_pntExt2.ps_pAddr  = ps_pAddrCpy;
            s_msg.s_etc1.w_u16        = w_ifIdx;            
            s_msg.e_mType             = e_IMSG_DLRQ;
            /* send to Master delay task to respond */
            o_disp = SIS_MboxPut(DIS_aw_MboxHdl_mad[w_ifIdx],&s_msg);
          }
        }     
        break;
      }
#endif /*#if( k_CLK_DEL_E2E == TRUE )*/
#endif /*#if(( k_CLK_IS_OC == TRUE )||(k_CLK_IS_BC == TRUE)) */      
      /* just work with pdelay request and pdelay response messages, 
         if P2P is used */ 
#if( k_CLK_DEL_P2P == TRUE )
      case e_MT_PDELREQ:
      {
#if(( k_CLK_IS_OC == TRUE )||(k_CLK_IS_BC == TRUE))
        /* do not dispatch in disabled state */
        if( e_portSts == e_STS_DISABLED )
        {
          o_disp = FALSE;
        }
        else
#endif
        {
          /* get memory */
          pv_mem = (void*)SIS_Alloc(sizeof(NIF_t_PTPV2_PDelReq));
          if( pv_mem == NULL )
          {
            /* set mem error */ 
            PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_4,e_SEVC_WARN); 
            /* discard msg and timestamp */
            o_disp = FALSE;
          }
          else
          {
            /* unpack the message */
            NIF_UnpackPDlrq((NIF_t_PTPV2_PDelReq*)pv_mem,pb_rxMsg);
            /* copy port address of unicast messages 
              if it is a unicast address */
            if( GET_FLAG(((NIF_t_PTPV2_PDelReq*)pv_mem)->s_ptpHead.w_flags,
                        k_FLG_UNICAST) == TRUE )
            {              
              /* copy sender port address */
              ps_pAddrCpy = (PTP_t_PortAddr*)SIS_Alloc(sizeof(PTP_t_PortAddr));
              if( ps_pAddrCpy == NULL )
              {
                /* set error */
                PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_3,e_SEVC_NOTC);
              }
              else
              {
                /* copy port address */
                ps_pAddrCpy->w_AddrLen  = ps_pAddr->w_AddrLen;
                ps_pAddrCpy->e_netwProt = ps_pAddr->e_netwProt;
                PTP_BCOPY(ps_pAddrCpy->ab_Addr,
                          ps_pAddr->ab_Addr,
                          ps_pAddr->w_AddrLen);
              }
            }
            else
            {
              ps_pAddrCpy = NULL;
            }
            /* set mbox-entry parameters */
            s_msg.s_pntData.pv_data   = pv_mem;
            s_msg.s_pntExt1.ps_tmStmp = ps_rxTs;
            s_msg.s_pntExt2.ps_pAddr  = ps_pAddrCpy;
            s_msg.s_etc1.w_u16        = w_ifIdx;   
            s_msg.e_mType   = e_IMSG_P_DLRQ;
            /* send to P2P response task */
            o_disp = SIS_MboxPut(DIS_aw_MboxHdl_p2p_resp[w_ifIdx],&s_msg);
          }
        }
        break;
      }
      case e_MT_PDELRESP:
      {
        /* get memory */
        pv_mem = (void*)SIS_Alloc(sizeof(NIF_t_PTPV2_PDelResp));
        if( pv_mem == NULL )
        {
          /* set mem error */ 
          PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_5,e_SEVC_WARN); 
          /* discard msg and timestamp */
          o_disp = FALSE;
        }
        else
        {
          /* unpack the message */
          NIF_UnpackPDlrsp((NIF_t_PTPV2_PDelResp*)pv_mem,pb_rxMsg);
          /* set mbox-entry parameters */
          s_msg.s_pntData.pv_data   = pv_mem;
          s_msg.s_pntExt1.ps_tmStmp = ps_rxTs;
          s_msg.s_etc1.w_u16        = w_ifIdx;   
          s_msg.e_mType   = e_IMSG_P_DLRSP;
          /* send to pdelay task - maybe response to pdelay req */
          o_disp = SIS_MboxPut(DIS_aw_MboxHdl_p2p_del[w_ifIdx],&s_msg);
        } 
        break;
      }
  #endif /*   #if( k_CLK_DEL_P2P == TRUE ) */
      default:
      {
        /* check, if this is a general message */
        if(e_msgType > e_MT_PDELRESP)         
        {
          /* message discarded because wrong port */
          PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_WRG_PORT,e_SEVC_NOTC);
        }
        /* discard msg and timestamp */
        o_disp = FALSE;
        break;
      }
    }/*lint !e788 */
  }
  /* free allocated memory */
  if( o_disp == FALSE)
  {
    if( ps_rxTs != NULL )
    {    
      /* free allocated timestamp memory */
      SIS_Free((void*)ps_rxTs);    
    }
    if( pv_mem != NULL )
    {
      /* free allocated message struct memory */
      SIS_Free(pv_mem);          
    }
    if( ps_pAddrCpy != NULL ) /*lint !e774 */
    {
      /* free allocated port address struct memory */
      SIS_Free(ps_pAddrCpy);
    }
  }
}  

/***********************************************************************
**
** Function    : DIS_DispGnrlMsg
**
** Description : Filters and dispatches the general messages  
**               depending on the message content and the actual port state.
**              
** Parameters  : w_ifIdx   (IN) - actual communication interface
**               pb_rxMsg  (IN) - rx message of this channel
**               i_len     (IN) - amount of received data bytes
**               e_msgType (IN) - message type to dispatch
**               ps_pAddr  (IN) - port address of sender
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DIS_DispGnrlMsg( UINT16               w_ifIdx,
                      const UINT8          *pb_rxMsg,
                      INT16                i_len,
                      PTP_t_msgTypeEnum    e_msgType,
                      const PTP_t_PortAddr *ps_pAddr)
{
  PTP_t_MboxMsg  s_msg;
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
  PTP_t_stsEnum  e_portSts;
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
  BOOLEAN        o_disp = TRUE;
  void           *pv_mem = NULL;
  PTP_t_PortId   s_trgtPrtId;
  PTP_t_PortId   s_sendPrtId;
  PTP_t_PortAddr *ps_pAddrCpy = NULL;
  BOOLEAN        o_ownNode;
  BOOLEAN        o_remoteNode;
  BOOLEAN        o_oneMsg;
#if( k_ALT_MASTER == FALSE )
  BOOLEAN        o_altMst;

  /* get alternate master flag */
  o_altMst = GET_FLAG(NIF_UNPACKHDR_FLAGS(pb_rxMsg),k_FLG_ALT_MASTER);
#endif /* #if( k_ALT_MASTER == FALSE ) */
  /* check domain number */
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
  if( NIF_UNPACKHDR_DOMNMB(pb_rxMsg) != DIS_ps_ClkDs->s_DefDs.b_domn )
  {
    /* discard msg and timestamp */
    o_disp = FALSE;    
  }
#else /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
  if( NIF_UNPACKHDR_DOMNMB(pb_rxMsg) != DIS_ps_ClkDs->s_TcDefDs.b_primDom )  
  {
    /* discard msg and timestamp */
    o_disp = FALSE;    
  }
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
  else
  {
#if(( k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE))
    /* Get the port state of the responsible port */ 
    e_portSts = DIS_ps_ClkDs->as_PortDs[w_ifIdx].e_portSts;
#endif /* #if(( k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE)) */
    /* dispatch depending on port-state and message-type */ 
    switch( e_msgType )
    { 
      case e_MT_ANNOUNCE:
      {
#if(( k_CLK_IS_OC == FALSE )&&(k_CLK_IS_BC == FALSE))
        /* discard message */
        o_disp = FALSE;
#else /* #if(( k_CLK_IS_OC == FALSE )&&(k_CLK_IS_BC == FALSE)) */
        /* if the port is in initializing or disabled state 
           discard the message */
        if(( e_portSts == e_STS_INIT )     || 
           ( e_portSts == e_STS_DISABLED ) ||
           ( e_portSts == e_STS_FAULTY )   ||
           ( i_len < (k_PTPV2_HDR_SZE + k_PTPV2_ANNC_PLD))
#if( k_ALT_MASTER == FALSE)
           || ( o_altMst == TRUE )
#endif /* #if( k_ALT_MASTER == FALSE) */
           )
        {
          /* discard msg and timestamp */
          o_disp = FALSE;
        }
        else
        {
          /* get memory */
          pv_mem = SIS_Alloc(sizeof(NIF_t_PTPV2_AnnMsg));
          if( pv_mem == NULL )
          {
            /* set mem error */ 
            PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_6,e_SEVC_WARN); 
            /* discard msg and timestamp */
            o_disp = FALSE;
          }
          else
          {
            /* unpack the message */
            NIF_UnPackAnncMsg((NIF_t_PTPV2_AnnMsg*)pv_mem,pb_rxMsg);
            /* steps removed field exceeds 254 ? */
            if( ((NIF_t_PTPV2_AnnMsg*)pv_mem)->w_stepsRemvd >= 255 )
            {
              o_disp = FALSE;
            }
            else
            {
#if( k_UNICAST_CPBL == TRUE )
              /* copy sender port address */
              ps_pAddrCpy = (PTP_t_PortAddr*)SIS_Alloc(sizeof(PTP_t_PortAddr));
              if( ps_pAddrCpy == NULL )
              {
                /* set error */
                PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_7,e_SEVC_NOTC);
                break;
              }
              else
              {
                /* copy port address */
                ps_pAddrCpy->w_AddrLen  = ps_pAddr->w_AddrLen;
                ps_pAddrCpy->e_netwProt = ps_pAddr->e_netwProt;
                PTP_BCOPY(ps_pAddrCpy->ab_Addr,
                          ps_pAddr->ab_Addr,
                          ps_pAddr->w_AddrLen);
              }
#endif /* #if( k_UNICAST_CPBL == TRUE ) */              
              /* set mbox-entry parameters */
              s_msg.s_pntData.pv_data  = pv_mem;  
              s_msg.s_pntExt1.ps_pAddr = ps_pAddrCpy;
              s_msg.s_etc1.w_u16       = w_ifIdx;        
              s_msg.e_mType            = e_IMSG_ANNC;
              /* sent message to control task for BMC */
              o_disp = SIS_MboxPut(CTL_TSK,&s_msg);   
            }
          }
        }
#endif /* #if(( k_CLK_IS_OC == FALSE )&&(k_CLK_IS_BC == FALSE)) */
        break;
      }       
      case e_MT_FLWUP:
      {
#if(( k_CLK_IS_OC == FALSE )&&(k_CLK_IS_BC == FALSE))
        /* discard message */
        o_disp = FALSE;
#else /* #if(( k_CLK_IS_OC == FALSE )&&(k_CLK_IS_BC == FALSE)) */
        /* if the port is not in slave or uncalibrated state 
           discard the message */
        if((( e_portSts != e_STS_SLAVE )&&( e_portSts != e_STS_UNCALIBRATED ))
           ||( i_len < (k_PTPV2_HDR_SZE + k_PTPV2_FLUP_PLD))
#if( k_ALT_MASTER == FALSE)
           || ( o_altMst == TRUE )
#endif /* #if( k_ALT_MASTER == FALSE) */
           )
        {
          /* discard msg and timestamp */
          o_disp = FALSE;
        }
        else
        {
          /* get memory */
          pv_mem = (void*)SIS_Alloc(sizeof(NIF_t_PTPV2_FlwUpMsg));
          if( pv_mem == NULL )
          {
            /* set mem error */ 
            PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_8,e_SEVC_WARN); 
            /* discard msg and timestamp */
            o_disp = FALSE;            
          }
          else
          {
            /* unpack the message */
            NIF_UnPackFlwUpMsg((NIF_t_PTPV2_FlwUpMsg*)pv_mem,pb_rxMsg);
            /* set mbox-entry parameters */
            s_msg.s_pntData.pv_data = pv_mem;  
            s_msg.s_etc1.w_u16      = w_ifIdx;        
            s_msg.e_mType           = e_IMSG_FLWUP;
            o_disp = SIS_MboxPut(SLS_TSK,&s_msg); 
          }
        }
#endif /* #if(( k_CLK_IS_OC == FALSE )&&(k_CLK_IS_BC == FALSE)) */
        break;
      }
/* just work with delay request and delay response messages, if E2E is used */ 
#if( k_CLK_DEL_E2E == TRUE )
      case e_MT_DELRESP:
      {
#if(( k_CLK_IS_OC == FALSE )&&(k_CLK_IS_BC == FALSE))
        /* discard message */
        o_disp = FALSE;
#else /* #if(( k_CLK_IS_OC == FALSE )&&(k_CLK_IS_BC == FALSE)) */
        /* if the port is not in slave or uncalibrated state 
           discard the message */
        if((( e_portSts != e_STS_SLAVE )&&( e_portSts != e_STS_UNCALIBRATED )) 
           || ( i_len < (k_PTPV2_HDR_SZE + k_PTPV2_DLRSP_PLD))
#if( k_ALT_MASTER == FALSE)
           || ( o_altMst == TRUE )
#endif /* #if( k_ALT_MASTER == FALSE) */
           )
        {
          /* discard msg and timestamp */
          o_disp = FALSE;
        }
        else
        {
          /* get memory */
          pv_mem = (void*)SIS_Alloc(sizeof(NIF_t_PTPV2_DlRspMsg));
          if( pv_mem == NULL )
          {
            /* set mem error */ 
            PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_9,e_SEVC_WARN); 
            /* discard msg and timestamp */
            o_disp = FALSE;
          }
          else
          {
            /* unpack the message */
            NIF_UnPackDelRespMsg((NIF_t_PTPV2_DlRspMsg*)pv_mem,pb_rxMsg);
            /* set mbox-entry parameters */
            s_msg.s_pntData.pv_data = pv_mem;  
            s_msg.s_etc1.w_u16      = w_ifIdx;        
            s_msg.e_mType           = e_IMSG_DLRSP;
            /* send to sync delay task - maybe response to delay req */
            o_disp = SIS_MboxPut(SLD_TSK,&s_msg);            
          }
        }
#endif /* #if(( k_CLK_IS_OC == FALSE )&&(k_CLK_IS_BC == FALSE)) */
        break;
      }
#endif /*#if( k_CLK_DEL_E2E == TRUE )*/
      case e_MT_MNGMNT:
      {
        /* unpack target port identity */
        NIF_UnPackTrgtPortId(&s_trgtPrtId,pb_rxMsg);
        /* check, if node is addressed */
        DIS_WhoIsReceiver(&s_trgtPrtId,&o_ownNode,&o_remoteNode,&o_oneMsg);
        if( o_ownNode == TRUE )
        {
          /* get memory */
          pv_mem = (void*)SIS_Alloc(sizeof(NIF_t_PTPV2_MntMsg));
          if( pv_mem == NULL )
          {
            /* set mem error */ 
            PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_10,e_SEVC_WARN); 
            /* discard msg */
            o_disp = FALSE;
          }
          else
          {
            /* unpack the message */
            NIF_UnPackMntMsg((NIF_t_PTPV2_MntMsg*)pv_mem,pb_rxMsg);
            /* set mbox-entry parameters */
            s_msg.s_pntExt1.pv_data = NULL;
            /* check for unicast transfer */
            if(GET_FLAG((((NIF_t_PTPV2_MntMsg*)pv_mem)->s_ptpHead.w_flags), 
                        k_FLG_UNICAST))
            {
              /* unictast detected, 
                 alloc buffer to save node network address */
              ps_pAddrCpy = (PTP_t_PortAddr*)SIS_Alloc(sizeof(PTP_t_PortAddr));
              if(ps_pAddrCpy != NULL)
              {
                *ps_pAddrCpy = *ps_pAddr;
              }
              /* if alloc failed -> NULL pointer, else pointer to buffer */
              s_msg.s_pntExt2.ps_pAddr = ps_pAddrCpy;
            }
            else
            {
              /* no unicast -> set NULL for multicast */
              s_msg.s_pntExt2.pv_data = NULL;
            }
            s_msg.s_pntData.pv_data = pv_mem; 
            s_msg.s_etc1.w_u16      = w_ifIdx;
            s_msg.s_etc3.dw_u32     = k_MNT_MSG_EXT;
            s_msg.e_mType           = e_IMSG_MM;
            /* send to sync delay task - maybe response to delay req */
            if( SIS_MboxPut(MNT_TSK,&s_msg) == FALSE )
            {
              o_disp = FALSE;
              if( ((NIF_t_PTPV2_MntMsg*)pv_mem)->s_mntTlv.pb_data != NULL )
              {
                SIS_Free(((NIF_t_PTPV2_MntMsg*)pv_mem)->s_mntTlv.pb_data);
              }
            }
          }
        }
        else
        {
          /* discard msg */
          o_disp = FALSE;
        }
        /* shall message be forwarded ? */
        if( o_remoteNode == TRUE )
        {
          /* forward message */
          DIS_FwMntMsg(pb_rxMsg,(UINT16)i_len,w_ifIdx);          
        }
        break;
      }
      case e_MT_SGNLNG:
      {
        /* unpack target port identity */
        NIF_UnPackTrgtPortId(&s_trgtPrtId,pb_rxMsg);
        /* check, if node is addressed */
        DIS_WhoIsReceiver(&s_trgtPrtId,&o_ownNode,&o_remoteNode,&o_oneMsg);
        if( o_ownNode == TRUE )
        {
          /* unpack sender port Id */
          NIF_UnPackHdrPortId(&s_sendPrtId,pb_rxMsg);
          /* unpack TLVs and dispatch it */
          DispTlv(&pb_rxMsg[k_OFFS_TLVS],
                  ps_pAddr,
                  &s_sendPrtId,
                  w_ifIdx,
                  (UINT16)(i_len-k_OFFS_TLVS));  
        }
        else
        {
          o_disp = FALSE;
        }
        /* shall message be forwarded ? */
        if( o_remoteNode == TRUE )
        {
          /* forward message on all interfaces */
          DIS_FwSignMsg(pb_rxMsg,(UINT16)i_len,w_ifIdx);
        }
        /* discard msg */
        o_disp = FALSE;
        break;
      }
/* just work with pdelay request and pdelay response messages, if P2P is used */
#if( k_CLK_DEL_P2P == TRUE )
      case e_MT_PDELRES_FLWUP:
      {
        /* get memory */
        pv_mem = (void*)SIS_Alloc(sizeof(NIF_t_PTPV2_PDlRspFlw));
        if( pv_mem == NULL )
        {
          /* set mem error */ 
          PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_11,e_SEVC_WARN); 
          /* discard msg and timestamp */
          o_disp = FALSE;
        }
        else
        {
          /* unpack the message */
          NIF_UnpackPDlrspFlw((NIF_t_PTPV2_PDlRspFlw*)pv_mem,pb_rxMsg);
          /* set mbox-entry parameters */
          s_msg.s_pntData.pv_data = pv_mem;
          s_msg.s_etc1.w_u16      = (UINT32)w_ifIdx;   
          s_msg.e_mType           = e_IMSG_P_DLRSP_FLW;
          /* send to sync delay task - maybe response to delay req */
          o_disp = SIS_MboxPut(DIS_aw_MboxHdl_p2p_del[w_ifIdx],&s_msg); 
        }
        break;
      }
#endif /*   #if( k_CLK_DEL_P2P == TRUE ) */
      default:
      {
        /* check, if this is an event message */
        if(e_msgType < e_MT_FLWUP)         
        {
          /* message discarded because wrong port */
          PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_WRG_PORT,e_SEVC_NOTC);
        }
        /* discard msg and timestamp */
        o_disp = FALSE;
        break;
      }
    }/*lint !e788 */
  }
  /* free allocated memory */
  if( o_disp == FALSE)
  {
    if( pv_mem != NULL )
    {
      /* free allocated message struct memory */
      SIS_Free(pv_mem);          
    }
    if( ps_pAddrCpy != NULL ) /*lint !e774 */
    {
      /* free allocated port address struct memory */
      SIS_Free(ps_pAddrCpy);
    }
  }
}

#if(k_CLK_IS_TC)
/***********************************************************************
**
** Function    : DIS_DispEvtMsgTc
**
** Description : Dispatches the event messages to the regarding 
**               transparent clock unit to forward it.
**              
** Parameters  : w_ifIdx    (IN) - actual interface 
**               pb_rxMsg     (IN) - rx message of this channel
**               ps_rxTs      (IN) - receive timestamp (just event messages)
**               i_len        (IN) - amount of received data bytes4
**               e_msgType    (IN) - message type to dispatch
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void DIS_DispEvtMsgTc( UINT16             w_ifIdx,
                       const UINT8        *pb_rxMsg,
                       const PTP_t_TmStmp *ps_rxTs,
                       INT16              i_len,
                       PTP_t_msgTypeEnum  e_msgType)
{  
  BOOLEAN       o_disp = TRUE;
  UINT8         *pb_msgTc = NULL;
  PTP_t_TmStmp  *ps_rxTsTc = NULL;
  UINT16        w_tskHdl = 0;
  PTP_t_MboxMsg s_msg;  
  UINT16        w_flags;
#if(((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) && (k_CLK_DEL_E2E == TRUE))
  PTP_t_stsEnum e_portSts;
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */  
  /* get flags */
  w_flags = NIF_UNPACKHDR_FLAGS(pb_rxMsg);
  if( GET_FLAG(w_flags,k_FLG_UNICAST) == TRUE )
  {
    /* do not forward unicast messages */
    return;
  }
  /* send it to TC forward task */
  switch( e_msgType )
  {
    case e_MT_SYNC:
    {
      /* check message length */
      if( i_len >= (k_PTPV2_HDR_SZE + k_PTPV2_SYNC_PLD))
      {
        s_msg.e_mType = e_IMSG_FWD_SYN;
        /* task handle of task to send to */
        w_tskHdl = TCS_TSK;
        /* get memory to copy the rx timestamp */
        ps_rxTsTc = (PTP_t_TmStmp*)SIS_Alloc(sizeof(PTP_t_TmStmp));
        if( ps_rxTsTc == NULL )
        {
          /* set error */
          PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_12,e_SEVC_NOTC);
          /* set return value to FALSE - message was not dispatched */
          o_disp = FALSE;
        }
        else
        {
          /* copy rx timestamp */
          *ps_rxTsTc = *ps_rxTs;
          /* is dispatched */
          o_disp = TRUE;
          i_len  = k_PTPV2_HDR_SZE + k_PTPV2_SYNC_PLD;
        }
      }
      else
      {
        /* does not get dispatched */
        o_disp = FALSE;
      }
      break;
    }    
    /* just works with delay request and delay response messages, 
       if E2E is used */ 
#if( k_CLK_DEL_E2E == TRUE )
    case e_MT_DELREQ:
    {
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
      /* Get the port state of the responsible port */ 
      e_portSts = DIS_ps_ClkDs->as_PortDs[w_ifIdx].e_portSts;
      /* just forward it, if this node is not master */
      if( e_portSts == e_STS_MASTER )
      {
        o_disp = FALSE;
      }
      else
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
      {
        /* check message length */
        if( i_len >= (k_PTPV2_HDR_SZE + k_PTPV2_DLRQ_PLD))
        {
          s_msg.e_mType      = e_IMSG_FWD_DLRQ;
          /* task handle of task to send to */
          w_tskHdl = TCD_TSK;
          /* get memory to copy the rx timestamp */
          ps_rxTsTc = (PTP_t_TmStmp*)SIS_Alloc(sizeof(PTP_t_TmStmp));
          if( ps_rxTsTc == NULL )
          {
            /* set error */
            PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_13,e_SEVC_NOTC);
            /* set return value to FALSE - message was not dispatched */
            o_disp = FALSE;
          }
          else
          {
            /* copy rx timestamp */
            *ps_rxTsTc = *ps_rxTs;
            /* is dispatched */
            o_disp = TRUE;
            i_len  = k_PTPV2_HDR_SZE + k_PTPV2_DLRQ_PLD;
          }
        }
        else
        {
          /* does not get dispatched */
          o_disp = FALSE;
        }
      }
      break;
    }
    /* forwarding of P2P messages by E2E TCs */
    case e_MT_PDELREQ:
    {
      /* check message length */
      if( i_len >= (k_PTPV2_HDR_SZE + k_PTPV2_P_DLRQ_PLD))
      {
        s_msg.e_mType = e_IMSG_FWD_P_DLRQ;
        /* task handle of task to send to */
        w_tskHdl = TCP_TSK;
        /* get memory to copy the rx timestamp */
        ps_rxTsTc = (PTP_t_TmStmp*)SIS_Alloc(sizeof(PTP_t_TmStmp));
        if( ps_rxTsTc == NULL )
        {
          /* set error */
          PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_14,e_SEVC_NOTC);
          /* set return value to FALSE - message was not dispatched */
          o_disp = FALSE;
        }
        else
        {
          /* copy rx timestamp */
          *ps_rxTsTc = *ps_rxTs;
          /* is dispatched */
          o_disp = TRUE;
          i_len  = k_PTPV2_HDR_SZE + k_PTPV2_P_DLRQ_PLD;
        }
      }
      else
      {
        /* does not get dispatched */
        o_disp = FALSE;
      }
      break;
    }
    case e_MT_PDELRESP:
    {
      /* check message length */
      if( i_len >= (k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_PLD))
      {
        s_msg.e_mType = e_IMSG_FWD_P_DLRSP;
        /* task handle of task to send to */
        w_tskHdl = TCP_TSK;
        /* get memory to copy the rx timestamp */
        ps_rxTsTc = (PTP_t_TmStmp*)SIS_Alloc(sizeof(PTP_t_TmStmp));
        if( ps_rxTsTc == NULL )
        {
          /* set error */
          PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_15,e_SEVC_NOTC);
          /* set return value to FALSE - message was not dispatched */
          o_disp = FALSE;
        }
        else
        {
          /* copy rx timestamp */
          *ps_rxTsTc = *ps_rxTs;
          /* is dispatched */
          o_disp = TRUE;
          i_len  = k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_PLD;
        }
      }
      else
      {
        /* does not get dispatched */
        o_disp = FALSE;
      }
      break;
      
    }
#endif /*#if( k_CLK_DEL_E2E == TRUE )*/  
    default:
    {
      /* does not get dispatched */
      o_disp = FALSE;
    }
  }/*lint !e788*/
  if( o_disp == TRUE )
  {
    /* get memory to copy the message */
    pb_msgTc = (UINT8*)SIS_Alloc((UINT16)i_len);
    if( pb_msgTc == NULL )
    {
      /* set error */
      PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_16,e_SEVC_NOTC);
      /* set return value to FALSE - message was not dispatched */
      o_disp = FALSE;
    }
    else
    {
      /* copy message */
      PTP_BCOPY(pb_msgTc,pb_rxMsg,(UINT16)i_len); 
      s_msg.s_pntData.pb_u8     = pb_msgTc;
      s_msg.s_pntExt1.ps_tmStmp = ps_rxTsTc;
      s_msg.s_etc1.w_u16        = w_ifIdx;
      s_msg.s_etc2.w_u16        = (UINT16)i_len;
      /* try to send */
      if( SIS_MboxPut(w_tskHdl,&s_msg) == FALSE )
      {
        /* free memory */
        SIS_Free(pb_msgTc);
        if( ps_rxTsTc != NULL )
        {
          SIS_Free(ps_rxTsTc);
        }
        /* set error */
        PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MSG_MBOX,e_SEVC_NOTC);
        /* set return value to FALSE - message was not dispatched */
        o_disp = FALSE;
      }
      else
      {
        /* set return value to TRUE - message was dispatched */
        o_disp = TRUE;
      }
    }
  }
}

/***********************************************************************
**
** Function    : DIS_DispGnrlMsgTc
**
** Description : Dispatches the general messages to the regarding 
**               transparent clock unit to forward it.
**              
** Parameters  : w_ifIdx    (IN) - actual interface 
**               pb_rxMsg     (IN) - rx message of this channel
**               i_len        (IN) - amount of received data bytes4
**               e_msgType    (IN) - message type to dispatch
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void DIS_DispGnrlMsgTc( UINT16             w_ifIdx,
                        const UINT8        *pb_rxMsg,
                        INT16              i_len,
                        PTP_t_msgTypeEnum  e_msgType)
{  
  BOOLEAN       o_disp = TRUE;
  UINT8         *pb_msgTc = NULL;
  UINT16        w_tskHdl = 0;
  PTP_t_MboxMsg s_msg;  
  UINT16        w_flags;

  /* get flags */
  w_flags = NIF_UNPACKHDR_FLAGS(pb_rxMsg);
  if( GET_FLAG(w_flags,k_FLG_UNICAST) == TRUE )
  {
    /* do not forward unicast messages */
    return;
  }
  /* send it to TC forward task */
  switch( e_msgType )
  {
    case e_MT_ANNOUNCE:
    {
      /* check message length */
      if( i_len >= (k_PTPV2_HDR_SZE + k_PTPV2_ANNC_PLD))
      {
        s_msg.e_mType = e_IMSG_FWD_ANNC;
        /* task handle of task to send to */
        w_tskHdl = TCS_TSK;
        /* is dispatched */
        o_disp = TRUE;  
        i_len  = k_PTPV2_HDR_SZE + k_PTPV2_ANNC_PLD;
      }
      else
      {
        /* does not get dispatched */
        o_disp = FALSE;
      }
      break;
    }
    case e_MT_FLWUP:
    {
      /* check message length */
      if( i_len >= (k_PTPV2_HDR_SZE + k_PTPV2_FLUP_PLD))
      {
        /* set internal message type */
        s_msg.e_mType = e_IMSG_FWD_FLW;
        /* task handle of task to send to */
        w_tskHdl = TCS_TSK;
        /* is dispatched */
        o_disp = TRUE;
        i_len  = k_PTPV2_HDR_SZE + k_PTPV2_FLUP_PLD;
      }
      else
      {
        /* does not get dispatched */
        o_disp = FALSE;
      }
      break;
    }
    /* just works with delay request and delay response messages, 
       if E2E is used */ 
#if( k_CLK_DEL_E2E == TRUE )
    case e_MT_DELRESP:
    {
      /* check message length */
      if( i_len >= (k_PTPV2_HDR_SZE + k_PTPV2_DLRSP_PLD))
      {
        /* set internal message type */
        s_msg.e_mType = e_IMSG_FWD_DLRSP;
        /* task handle of task to send to */
        w_tskHdl = TCD_TSK;
        /* is dispatched */
        o_disp = TRUE;
        i_len  = k_PTPV2_HDR_SZE + k_PTPV2_DLRSP_PLD;
      }
      else
      {
        /* does not get dispatched */
        o_disp = FALSE;
      }
      break;
    }
    case e_MT_PDELRES_FLWUP:
    {
      /* check message length */
      if( i_len >= (k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_FLW_PLD))
      {
        /* set internal message type */
        s_msg.e_mType = e_IMSG_FWD_P_DLRFLW;
        /* task handle of task to send to */
        w_tskHdl = TCP_TSK;
        /* is dispatched */
        o_disp = TRUE;
        i_len  = k_PTPV2_HDR_SZE + k_PTPV2_P_DLRSP_FLW_PLD;
      }
      else
      {
        /* does not get dispatched */
        o_disp = FALSE;
      }
      break;      
    }
#endif /*#if( k_CLK_DEL_E2E == TRUE )*/  
    default:
    {
      /* does not get dispatched */
      o_disp = FALSE;
    }
  }/*lint !e788*/
  if( o_disp == TRUE )
  {
    /* get memory to copy the message */
    pb_msgTc = (UINT8*)SIS_Alloc((UINT16)i_len);
    if( pb_msgTc == NULL )
    {
      /* set error */
      PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_16,e_SEVC_NOTC);
      /* set return value to FALSE - message was not dispatched */
      o_disp = FALSE;
    }
    else
    {
      /* copy message */
      PTP_BCOPY(pb_msgTc,pb_rxMsg,(UINT16)i_len); 
      s_msg.s_pntData.pb_u8 = pb_msgTc;
      s_msg.s_etc1.w_u16    = w_ifIdx;
      /* try to send */
      if( SIS_MboxPut(w_tskHdl,&s_msg) == FALSE )
      {
        /* free memory */
        SIS_Free(pb_msgTc);
        /* set error */
        PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MSG_MBOX,e_SEVC_NOTC);
        /* set return value to FALSE - message was not dispatched */
        o_disp = FALSE;
      }
      else
      {
        /* set return value to TRUE - message was dispatched */
        o_disp = TRUE;
      }
    }
  }
}
#endif /* #if(k_CLK_IS_TC) */

/***********************************************************************
**
** Function    : DIS_WhoIsReceiver
**
** Description : This function determines, if the own node is
**               addressed by the portId. If all 1s are set the own
**               node as well as all other nodes are addressed.
**              
** Parameters  : ps_portId      (IN) - portId to compare
**               po_ownNode    (OUT) - TRUE own node addressed
**                                     FALSE own node not addessed
**               po_remoteNode (OUT) - TRUE remote node addressed
**                                     FALSE remote node not addressed
**               po_oneMsg     (OUT) - TRUE more answers expected
**                                     FALSE only one answer expected
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DIS_WhoIsReceiver( const PTP_t_PortId *ps_portId,
                        BOOLEAN *po_ownNode,
                        BOOLEAN *po_remoteNode,
                        BOOLEAN *po_oneMsg )
{
  /* reset by default return values */
  *po_ownNode    = FALSE;
  *po_remoteNode = FALSE;
  *po_oneMsg     = FALSE;

  /* clock id all 1s ? */
  if( PTP_CompareClkId(&s_clkId_FF,&ps_portId->s_clkId) == PTP_k_SAME )
  {
    /* all clocks requested */
    *po_remoteNode = TRUE;

    /* port number all 1s ? */
    if( ps_portId->w_portNmb == 0xFFFF )
    {
      *po_ownNode = TRUE;
    }
    else
    {
      /* does port number exist ? */
      if( ( ps_portId->w_portNmb > 0 )&& (ps_portId->w_portNmb <= k_NUM_IF ))
      {
        *po_ownNode = TRUE;
      }
    }
  }
  /* compare clock id */
#if( k_CLK_IS_TC == TRUE )
  else if( PTP_CompareClkId(&ps_portId->s_clkId,
                            &DIS_ps_ClkDs->s_TcDefDs.s_clkId) == PTP_k_SAME )
  {
    /* port number all 1s ? */
    if( ps_portId->w_portNmb == 0xFFFF )
    {
      *po_ownNode = TRUE;
    }
    else
    {
      *po_oneMsg = TRUE;

      /* does port number exist ? */
      if(( ps_portId->w_portNmb > 0 ) && (ps_portId->w_portNmb <= k_NUM_IF ))
      {
        *po_ownNode = TRUE;
      }
    }
  }
#else /* #if( k_CLK_IS_TC == TRUE ) */
  else if( PTP_CompareClkId(&ps_portId->s_clkId,
                            &DIS_ps_ClkDs->s_DefDs.s_clkId) == PTP_k_SAME )
  {
    /* port number all 1s ? */
    if( ps_portId->w_portNmb == 0xFFFF )
    {
      *po_ownNode = TRUE;
    }
    else
    {
      *po_oneMsg = TRUE;

      /* does port number exist ? */
      if(( ps_portId->w_portNmb > 0 ) && (ps_portId->w_portNmb <= k_NUM_IF ))
      {
        *po_ownNode = TRUE;
      }
    }
  }
#endif /* #if( k_CLK_IS_TC == TRUE ) */  
  /* only remote node */
  else
  {
    *po_remoteNode = TRUE;

    /* port number all 1s ? */
    if( ps_portId->w_portNmb != 0xFFFF )
    {
      *po_oneMsg = TRUE;
    }
  }
  return;
}

/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
**  
** Function    : DispTlv
**  
** Description : Dispatches the TLVs from a signaling message.
**  
** Parameters  : pb_tlv     (IN) - tlv array
**               ps_pAddr   (IN) - port address of originator
**               ps_pId     (IN) - port id of originator
**               w_rcvIfIdx (IN) - receiving interface index
**               w_len      (IN) - length of the tlv array
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void DispTlv(const UINT8          *pb_tlv,
                    const PTP_t_PortAddr *ps_pAddr,
                    const PTP_t_PortId   *ps_pId,
                    UINT16               w_rcvIfIdx,
                    UINT16               w_len)
{
  PTP_t_tlvTypeEnum e_tlvType;
  UINT16            w_tlvLen;
  UINT16            w_restLen;
  UINT16            w_actOffs = 0;
  
  /* avoid compiler warning */
  ps_pAddr   = ps_pAddr;
  w_rcvIfIdx = w_rcvIfIdx;

  /* unpack and dispatch tlv as long as the tlv array contains data */
  while( w_actOffs < w_len )
  {
    w_restLen = w_len - w_actOffs;
    /* the rest of the tlv must be at least enough for 2 UINT16 variables */
    if( w_restLen >= (sizeof(UINT16)+sizeof(UINT16)) )
    {
      e_tlvType  = (PTP_t_tlvTypeEnum)GOE_ntoh16(&pb_tlv[w_actOffs]);
      w_actOffs += sizeof(UINT16);
      w_tlvLen   = GOE_ntoh16(&pb_tlv[w_actOffs]);
      w_actOffs += sizeof(UINT16);
      switch( e_tlvType )
      {
/* just for unicast enabled devices */
#if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC)))
        case e_TLVT_REQ_UC_TRM:
        {
          if( w_restLen >= 10 )
          {
            /* handles request unicast transmission tlv type */
            HandleReqUcTrmTlv(&pb_tlv[w_actOffs],ps_pAddr,w_rcvIfIdx);
          }
          break;
        }
        case e_TLVT_GRNT_UC_TRM:
        {
          if( w_restLen >= 10 )
          {
            /* handles request unicast transmission tlv type */
            HandleGrantUcTrmTlv(&pb_tlv[w_actOffs],
                                ps_pAddr,w_rcvIfIdx);
          }
          break;
        }
        case e_TLVT_CNCL_UC_TRM:
        {
          if( w_restLen >= 6 )
          {
            /* handle a cancel unicast transmission tlv type */
            HandleCnclUcTrmTlv(&pb_tlv[w_actOffs],
                               ps_pAddr,ps_pId,w_rcvIfIdx);
          }
          break;
        }
        case e_TLVT_ACK_CNCL_UC_TRM:
        {
          /* discard message */
          break;
        }
#endif /* #if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC))) */
        default:
        {
          /* unknown TLV - delete */
          break;
        }
      }/*lint !e788 */
      /* next tlv */
      w_actOffs = (UINT16)(w_actOffs + w_tlvLen);
    }
    else
    {
      /* stop loop, last tlv does not contain tlv type and tlv length */
      w_actOffs = w_len;
    }        
  }
}
/* just for unicast enabled */
#if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC)))
/***********************************************************************
**  
** Function    : HandleReqUcTrmTlv
**  
** Description : Handles a received REQUEST UNICAST TRANSMISSION TLV
**  
** Parameters  : pb_tlv     (IN) - buffer to tlv data field
**               ps_pAddr   (IN) - port address of sender
**               w_rcvIfIdx (IN) - receiving interface index
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void HandleReqUcTrmTlv(const UINT8          *pb_tlv,
                              const PTP_t_PortAddr *ps_pAddr,
                              UINT16               w_rcvIfIdx)
{
  PTP_t_msgTypeEnum e_msgType;
  INT8              c_logIntv;
  UINT32            dw_dur;
  PTP_t_MboxMsg     s_msg;
  UINT16            w_hdl;
  BOOLEAN           o_disc = FALSE;
  PTP_t_PortAddr    *ps_cpyPAddr;

  /* get requested message type */
  e_msgType = (PTP_t_msgTypeEnum)(pb_tlv[0] >> 4);
  /* get requested message interval */
  c_logIntv = (INT8)pb_tlv[1];
  /* get requested duration */ 
  dw_dur    = GOE_ntoh32(&pb_tlv[2]);
  
  /* get the task handle in dependance from port state and message type */
  switch( e_msgType )
  {
    case e_MT_ANNOUNCE:
    { 
      w_hdl = UCMann_TSK;
      break;
    }
    case e_MT_SYNC:
    {
      w_hdl = UCMsyn_TSK;
      break;
    }
    case e_MT_DELRESP:
    case e_MT_PDELRESP:
    {
      w_hdl = UCMdel_TSK;
      break;
    }
    default:
    {
      /* discard message */
      w_hdl = 0;
      o_disc = TRUE;
      break;
    }
  }/*lint !e788 */
  if( o_disc == FALSE )
  {
    /* copy sender port address */
    ps_cpyPAddr = (PTP_t_PortAddr*)SIS_Alloc(sizeof(PTP_t_PortAddr));
    if( ps_cpyPAddr == NULL )
    {
      /* set error */
      PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_17,e_SEVC_NOTC);
    }
    else
    {
      /* copy port address */
      ps_cpyPAddr->w_AddrLen  = ps_pAddr->w_AddrLen;
      ps_cpyPAddr->e_netwProt = ps_pAddr->e_netwProt;
      PTP_BCOPY(ps_cpyPAddr->ab_Addr,
                ps_pAddr->ab_Addr,
                ps_pAddr->w_AddrLen);
      /* assemble message */
      s_msg.e_mType            = e_IMSG_UC_SRV_REQ;
      s_msg.s_pntData.ps_pAddr = ps_cpyPAddr;
      s_msg.s_etc1.dw_u32      = dw_dur;
      s_msg.s_etc2.c_s8        = c_logIntv;
      s_msg.s_etc3.w_u16       = w_rcvIfIdx;
      /* send message to unicast master task */
      if( SIS_MboxPut(w_hdl,&s_msg) == FALSE )
      {
        /* free allocated memory */
        SIS_Free(ps_cpyPAddr);
      }
    }
  }
}

/***********************************************************************
**  
** Function    : HandleCnclUcTrmTlv
**  
** Description : Handles a received CANCEL UNICAST TRANSMISSION TLV
**  
** Parameters  : pb_tlv     (IN) - buffer to tlv data field
**               ps_pAddr   (IN) - port address of sender
**               ps_pId     (IN) - port id of sender
**               w_rcvIfIdx (IN) - receiving interface index
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void HandleCnclUcTrmTlv(const UINT8          *pb_tlv,
                               const PTP_t_PortAddr *ps_pAddr,
                               const PTP_t_PortId   *ps_pId,
                               UINT16               w_rcvIfIdx)
{
  PTP_t_msgTypeEnum e_msgType;
  PTP_t_MboxMsg     s_msg;
  UINT16            w_hdl;
  PTP_t_PortId      *ps_cpyPId;
  PTP_t_PortAddr    *ps_cpyPAddr;
  BOOLEAN           o_disc = FALSE;

  /* get requested message type */
  e_msgType = (PTP_t_msgTypeEnum)(pb_tlv[0] >> 4);

  /* is it a master, that cancels ? */
  if( DIS_ps_ClkDs->as_PortDs[w_rcvIfIdx].e_portSts == e_STS_SLAVE )
  {
    w_hdl  = UCD_TSK;
    o_disc = FALSE;
  }
  else
  {
    /* get the task handle in dependance from port state and message type */
    switch( e_msgType )
    {
      case e_MT_ANNOUNCE:
      { 
        w_hdl = UCMann_TSK;
        break;
      }
      case e_MT_SYNC:
      {
        w_hdl = UCMsyn_TSK;
        break;
      }
      case e_MT_DELRESP:
      case e_MT_PDELRESP:
      {
        w_hdl = UCMdel_TSK;
        break;
      }
      default:
      {
        /* discard message */
        w_hdl = 0;
        o_disc = TRUE;
        break;
      }
    }/*lint !e788 */
  }
  if( o_disc == FALSE )
  {  
    /* alloc memory for port id copy */
    ps_cpyPId = (PTP_t_PortId*)SIS_Alloc(sizeof(PTP_t_PortId));
    if( ps_cpyPId != NULL )
    {
      /* copy port id */
      *ps_cpyPId = *ps_pId;
      /* alloc memory for port address copy */
      ps_cpyPAddr = (PTP_t_PortAddr*)SIS_Alloc(sizeof(PTP_t_PortAddr*));
      if( ps_cpyPAddr != NULL )
      {
        /* copy port address */
        ps_cpyPAddr->w_AddrLen  = ps_pAddr->w_AddrLen;
        ps_cpyPAddr->e_netwProt = ps_pAddr->e_netwProt;
        PTP_BCOPY(ps_cpyPAddr->ab_Addr,
                  ps_pAddr->ab_Addr,
                  ps_pAddr->w_AddrLen);
        s_msg.e_mType            = e_IMSG_UC_CNCL;
        s_msg.s_pntData.ps_pAddr = ps_cpyPAddr;
        s_msg.s_pntExt1.ps_pId   = ps_cpyPId;
        s_msg.s_etc1.e_extMType  = e_msgType;
        s_msg.s_etc3.w_u16       = w_rcvIfIdx;

        /* send message to selected task */
        if( SIS_MboxPut(w_hdl,&s_msg) == FALSE )
        {
          /* free allocated memory */
          SIS_Free(ps_cpyPAddr);
          SIS_Free(ps_cpyPId);
        }
      }
      else
      {
        /* free allocated memory */
        SIS_Free(ps_cpyPId);
      }
    }
  }
}

/***********************************************************************
**  
** Function    : HandleGrantUcTrmTlv
**  
** Description : Handles a received GRANT UNICAST TRANSMISSION TLV
**  
** Parameters  : pb_tlv     (IN) - buffer to tlv data field
**               ps_pAddr   (IN) - port address of sender
**               w_rcvIfIdx (IN) - receiving interface index
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void HandleGrantUcTrmTlv(const UINT8 *pb_tlv,
                                const PTP_t_PortAddr *ps_pAddr,
                                UINT16 w_rcvIfIdx)
{
  PTP_t_msgTypeEnum e_msgType;
  INT8              c_logIntv;
  UINT32            dw_dur;
  PTP_t_MboxMsg     s_msg;
  PTP_t_intMsgEnum  e_intMType;
  BOOLEAN           o_disc = FALSE;
  PTP_t_PortAddr    *ps_cpyPAddr;


  /* get requested message type */
  e_msgType = (PTP_t_msgTypeEnum)(pb_tlv[0] >> 4);
  /* get requested message interval */
  c_logIntv = (INT8)pb_tlv[1];
  /* get requested duration */ 
  dw_dur    = GOE_ntoh32(&pb_tlv[2]);
  
  switch( e_msgType )
  {
    case e_MT_ANNOUNCE:
    { 
      e_intMType = e_IMSG_UC_GRNT_ANC;
      break;
    }
    case e_MT_SYNC:
    {
      e_intMType = e_IMSG_UC_GRNT_SYN;
      break;
    }
    case e_MT_DELRESP:
    case e_MT_PDELRESP:
    {
      e_intMType = e_IMSG_UC_GRNT_DEL;
      break;
    }
    default:
    {
      /* discard message */
      e_intMType = e_IMSG_NONE;
      o_disc = TRUE;
      break;
    }
  }/*lint !e788 */
  if( o_disc == FALSE )
  {
    /* copy sender port address */
    ps_cpyPAddr = (PTP_t_PortAddr*)SIS_Alloc(sizeof(PTP_t_PortAddr));
    if( ps_cpyPAddr == NULL )
    {
      /* set error */
      PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MPOOL_18,e_SEVC_NOTC);
    }
    else
    {
      /* copy port address */
      ps_cpyPAddr->w_AddrLen  = ps_pAddr->w_AddrLen;
      ps_cpyPAddr->e_netwProt = ps_pAddr->e_netwProt;
      PTP_BCOPY(ps_cpyPAddr->ab_Addr,
                ps_pAddr->ab_Addr,
                ps_pAddr->w_AddrLen);
      /* assemble message if message type is OK */
      s_msg.e_mType = e_intMType;
      s_msg.s_pntData.ps_pAddr = ps_cpyPAddr;
      s_msg.s_etc1.dw_u32      = dw_dur;
      s_msg.s_etc2.c_s8        = c_logIntv;
      s_msg.s_etc3.w_u16       = w_rcvIfIdx;
      /* send message to unicast task */
      if( SIS_MboxPut(UCD_TSK,&s_msg) == FALSE )
      {
        /* free allocated memory */
        SIS_Free(ps_cpyPAddr);
      }
    }
  }
}
#endif /* #if((k_UNICAST_CPBL == TRUE) && ((k_CLK_IS_OC) || (k_CLK_IS_BC))) */



