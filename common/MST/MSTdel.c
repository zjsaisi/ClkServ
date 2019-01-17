/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: MSTdel.c 
**    Summary: The unit MST encapsulates all master activities of a PTP node.
**             The task MSTdel_Task is active in the PTP states PTP_PRE_MASTER
**             and PTP_MASTER. It answers the delay requests of all plugged 
**             slaves.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: MSTdel_Task
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
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "DIS/DIS.h"
#include "MST/MST.h"
#include "MST/MSTint.h"

/* not used for TCs without OC */
#if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC))
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
** Function    : MSTdel_Task
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
void MSTdel_Task( UINT16 w_hdl )
{
  UINT32              dw_evnt;
  NIF_t_PTPV2_DlrqMsg *ps_dlrqMsg;
  UINT16              w_tskHdl = w_hdl - MAD1_TSK;
  PTP_t_TmStmp        s_tsRx; 
  PTP_t_TmIntv        s_tiDlrspCorr;
  PTP_t_PortAddr      *ps_pAddr = NULL;
  UINT32              dw_qual_tmo;

  static PTP_t_MboxMsg *aps_msg[k_NUM_IF];
  static UINT8         ab_taskSts[k_NUM_IF];

  /* multitasking restart point - generates a 
     jump to the last BREAK at env */
  SIS_Continue(w_hdl);    
    /* initialize the Task with status = k_MST_STOPPED */
    ab_taskSts[w_tskHdl] = k_MST_STOPPED;

  while( TRUE ) /*lint !e716 */
  {       
    /* got ready with an event?  */
    dw_evnt=SIS_EventGet(k_EVT_ALL);
    if( dw_evnt != 0 )
    {
      if( dw_evnt & (k_EV_SC_PSV|k_EV_SC_FLT|k_EV_SC_DSBL|k_EV_INIT))
      {
        /* change task state to stopped */
        ab_taskSts[w_tskHdl] = k_MST_STOPPED;
        /* stop timeout */
        SIS_TimerStop(w_hdl);
      }
    }
    /* got ready with message arrival? */
    while( ( aps_msg[w_tskHdl] = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    { 
      if( aps_msg[w_tskHdl]->e_mType == e_IMSG_SC_MST )
      {
        /* master task was passive - now first change in PRE_MASTER state */
        ab_taskSts[w_tskHdl] = k_MST_PRE_MASTER;            
        /* get multiplicator for the QUALIFICATION_TIMEOUT */
        dw_qual_tmo = aps_msg[w_tskHdl]->s_etc1.dw_u32;
        /* set qualification timeout */
        if( dw_qual_tmo > 0 )
        {              
          /* .. and start after QUALIFICATION_TIMEOUT */
          SIS_TimerStart(w_hdl,dw_qual_tmo);              
        }
        else
        {
          /* task must change immediately in state k_MST_OPERATIONAL */
          SIS_TimerStart(w_hdl,2);
        }    
      }
      /* a delay request message sent by the DIS_TSK */
      else if( aps_msg[w_tskHdl]->e_mType == e_IMSG_DLRQ )
      {
        if( ab_taskSts[w_tskHdl] == k_MST_OPERATIONAL )
        {
          /* get the message and rx timestamp */         
          ps_dlrqMsg = aps_msg[w_tskHdl]->s_pntData.ps_dlrq;
          /* get port address of sender */
          ps_pAddr   = aps_msg[w_tskHdl]->s_pntExt2.ps_pAddr;
          /* get rx timestamp */
          s_tsRx     = *aps_msg[w_tskHdl]->s_pntExt1.ps_tmStmp;
          /* set correction field to correction field value of delay request */
          s_tiDlrspCorr.ll_scld_Nsec = ps_dlrqMsg->s_ptpHead.ll_corField;       
          /* send delay response */
// jyang: temporarily block master output
//          DIS_SendDelayResp(ps_pAddr,
//                            MST_as_pId[w_tskHdl].w_portNmb-1,
//                            &ps_dlrqMsg->s_ptpHead.s_srcPortId,
//                            ps_dlrqMsg->s_ptpHead.w_seqId,
//                            &s_tiDlrspCorr,
//                            &s_tsRx);          
        }
        else
        {
          /* delay request does not get answered */
        }
        /* free message struct memory */
        SIS_Free(aps_msg[w_tskHdl]->s_pntData.pv_data);
        /* free timestamp memory */
        SIS_Free(aps_msg[w_tskHdl]->s_pntExt1.pv_data);
        /* free port address memory */
        if( aps_msg[w_tskHdl]->s_pntExt2.ps_pAddr != NULL )
        {
          SIS_Free(aps_msg[w_tskHdl]->s_pntExt2.pv_data);
        }
      }      
      else
      {
        /* set error - message misplaces */
        PTP_SetError(k_MST_ERR_ID,MST_k_UNKWN_MSG,e_SEVC_NOTC);
      }
      /* release mbox entry */
      SIS_MboxRelease();
    }
    /* got ready with elapsed timer?  */
    if( SIS_TimerStsRead( w_hdl ) == SIS_k_TIMER_EXPIRED )
    {      
      /* change to operational state */
      ab_taskSts[w_tskHdl] = k_MST_OPERATIONAL;
    }
    /* cooperative multitasking 
       set task to blocked, till it is ready through an event,
       message or timeout  */
    SIS_Break(w_hdl,1); /*lint !e646 !e717*/ 
  }
  SIS_Return(w_hdl,0); /*lint !e744 */
}


/*************************************************************************
**    static functions
*************************************************************************/
#endif /*#if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC))*/
