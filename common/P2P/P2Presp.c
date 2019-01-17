/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: P2Presp.c  
**    Summary: Peer Delay response task.
**             Responses to received peer delay requests by the next node 
**             and answers them with peer delay response and peer delay
**             response follow up.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: P2P_PrspTask
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
#if( k_CLK_DEL_P2P == TRUE )
#include "SIS/SIS.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "DIS/DIS.h"
#include "P2P/P2Pint.h"
#include "P2P/P2P.h"

/*************************************************************************
**    global variables
*************************************************************************/

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
** Function    : P2P_PrspTask
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
void P2P_PrspTask( UINT16 w_hdl )
{
  UINT16               w_tskIdx = (w_hdl - P2Prsp_TSK1);
  static PTP_t_MboxMsg *aps_msg[k_NUM_IF];
  NIF_t_PTPV2_PDelReq  *ps_p_Dlrq;
  PTP_t_TmStmp         *ps_rxTs;
  PTP_t_TmStmp         s_txTs;
  PTP_t_TmIntv         s_corr;
  PTP_t_PortAddr       *ps_portAddr;
  
  /* multitasking restart point - generates a 
     jump to the last BREAK at env */
  SIS_Continue(w_hdl); 

  while( TRUE )/*lint !e716 */
  {       
    /* got ready with message arrival? */
    while( ( aps_msg[w_tskIdx] = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    { 
      if( aps_msg[w_tskIdx]->e_mType == e_IMSG_P_DLRQ )
      {
        /* get message */
        ps_p_Dlrq   = aps_msg[w_tskIdx]->s_pntData.ps_pDreq;
        /* get port address */
        ps_portAddr = aps_msg[w_tskIdx]->s_pntExt2.ps_pAddr;
        /* get rx timestamp */
        ps_rxTs     = aps_msg[w_tskIdx]->s_pntExt1.ps_tmStmp;
        /* send response */
        if( DIS_SendPdlRsp( ps_portAddr,
                            w_tskIdx,
                            ps_p_Dlrq->s_ptpHead.w_seqId,
                            &ps_p_Dlrq->s_ptpHead.s_srcPortId,
                            ps_rxTs,
                            &s_txTs) == TRUE )
        {
/* two-step */
#if( k_TWO_STEP == TRUE )
          /* calculate correction field for the peer delay response follow up */
          if( PTP_SubtrTmStmpTmStmp(&s_txTs,ps_rxTs,&s_corr) == TRUE )
          {
            s_corr.ll_scld_Nsec += ps_p_Dlrq->s_ptpHead.ll_corField;
            /* send peer delay response follow up message */
            DIS_SendPdlRspFlw(ps_portAddr,
                              w_tskIdx,
                              ps_p_Dlrq->s_ptpHead.w_seqId,
                              &ps_p_Dlrq->s_ptpHead.s_srcPortId,
                              &s_corr.ll_scld_Nsec);
          }
          else
          {
            /* do nothing */
          }
#endif /* #if( k_TWO_STEP == TRUE ) */
        }
        /* free allocated memory */
        SIS_Free(ps_p_Dlrq);
        SIS_Free(ps_rxTs);
        if( ps_portAddr != NULL )
        {
          SIS_Free(ps_portAddr);
        }
      } 
      else
      {
        /* set error - message misplaces */
        PTP_SetError(k_P2P_ERR_ID,P2P_k_UNKWN_MSG,e_SEVC_NOTC);
      }      
      /* release mbox entry */
      SIS_MboxRelease();
    }    
    /* cooperative multitasking 
       set task to blocked, till it is ready through an event,
       message or timeout  */
    SIS_Break(w_hdl,1); /*lint !e646 !e717*/ 
  }
  SIS_Return(w_hdl,0);/*lint !e744*/
}


/*************************************************************************
**    static functions
*************************************************************************/

#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
