/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: MSTsyn.c  
**    Summary: The unit MST encapsulates all master activities of a PTP node.
**             The task MSTsyn_Task is active in the PTP states PTP_PRE_MASTER
**             and PTP_MASTER. It send�s periodic sync messages and follow up 
**             messages and answers the delay requests of all plugged 
**             slaves.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: MST_SetSyncInt
**             MSTsyn_Task
**             
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
#include "DIS/DIS.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "MST/MST.h"
#include "MST/MSTint.h"

/* not used for TCs without OC */
#if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC))
/*************************************************************************
**    global variables
*************************************************************************/
/* the actual sync interval timeout */
static UINT32 MST_adw_SynIntvTmo[k_NUM_IF];
static INT8   MST_ac_synIntv[k_NUM_IF] = {0};

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
** Function    : MST_SetSyncInt
**
** Description : Set the log of sync interval
**
** Parameters  : w_ifIdx    (IN) - communication interface index to change 
**               c_syncIntv (IN) - exponent for calculating the sync 
**                                 interval
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void MST_SetSyncInt(UINT16 w_ifIdx,INT8 c_syncIntv)
{
  /* set the sync interval timeout */
  MST_adw_SynIntvTmo[w_ifIdx] = PTP_getTimeIntv(c_syncIntv);
  MST_ac_synIntv[w_ifIdx]     = c_syncIntv;
}


/***********************************************************************
**
** Function    : MSTsyn_Task
**
** Description : The master task is responsible for sending sync messages
**               to the connected net segment. 
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : Function is reentrant.
**
***********************************************************************/
void MSTsyn_Task( UINT16 w_hdl )
{
  UINT32               dw_evnt;
  UINT32               dw_qual_tmo;
//  PTP_t_TmStmp         s_tsTx; 
//  PTP_t_TmIntv         s_tiFlwUpCorr;
  static UINT16        aw_seqId[k_NUM_IF] = {0};
  static PTP_t_MboxMsg *aps_msg[k_NUM_IF];
  static UINT8         ab_taskSts[k_NUM_IF];  

  /* multitasking restart point - generates a 
     jump to the last BREAK at env */
  SIS_Continue(w_hdl);    
    /* initialize the Task with status = k_MST_STOPPED  */
    ab_taskSts[w_hdl - MAS1_TSK] = k_MST_STOPPED;

  while( TRUE ) /*lint !e716 */
  {       
    /* got ready with a event? */
    dw_evnt=SIS_EventGet(k_EVT_ALL);

    if( dw_evnt != 0 )
    {
       /* stop master sync task on the events: */
      if( dw_evnt & (k_EV_SC_PSV  | /* event state change passive */
                     k_EV_SC_FLT  | /* event state change faulty */
                     k_EV_SC_DSBL | /* event state change disabled */
                     k_EV_INIT    | /* event init */
                     k_EV_SC_SLV))  /* event state change slave */
      {
        /* change task state to stopped */
        ab_taskSts[w_hdl - MAS1_TSK] = k_MST_STOPPED;
        /* stop timeout */
        SIS_TimerStop(w_hdl);
        /* reset all other events */
        dw_evnt = 0;
      }
    }
    /* got ready with message arrival? */
    while(( aps_msg[w_hdl - MAS1_TSK] = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL)
    {  
      if( aps_msg[w_hdl - MAS1_TSK]->e_mType == e_IMSG_SC_MST )
      {
        /* change to PRE_MASTER state */
        ab_taskSts[w_hdl - MAS1_TSK] = k_MST_PRE_MASTER;            
        /* get multiplicator for the QUALIFICATION_TIMEOUT */
        dw_qual_tmo = aps_msg[w_hdl - MAS1_TSK]->s_etc1.dw_u32;
        /* get sync interval for that interface */
        MST_ac_synIntv[w_hdl - MAS1_TSK] = aps_msg[w_hdl - MAS1_TSK]->s_etc2.c_s8;
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
      else
      {
        /* set error */
        PTP_SetError(k_MST_ERR_ID,MST_k_MSG_ERR,e_SEVC_NOTC);
      }      
      /* release mbox entry  */
      SIS_MboxRelease();
    } 
    /* got ready with elapsed timer?  */
    if( SIS_TimerStsRead( w_hdl ) == SIS_k_TIMER_EXPIRED )
    {
      if( ab_taskSts[w_hdl - MAS1_TSK] == k_MST_PRE_MASTER )
      {
        /* change to operational state */
        ab_taskSts[w_hdl - MAS1_TSK] = k_MST_OPERATIONAL;
      }
      if( ab_taskSts[w_hdl - MAS1_TSK] == k_MST_OPERATIONAL )
      {
        /* increment sync sequence number */
        aw_seqId[w_hdl - MAS1_TSK]++; 
        /* issue a sync message */
// jyang: temporarily block master output
//        if( DIS_SendSync(NULL,
//                         MST_as_pId[w_hdl - MAS1_TSK].w_portNmb-1,
//                         MST_ac_synIntv[w_hdl - MAS1_TSK],
//                         aw_seqId[w_hdl - MAS1_TSK],
//                         &s_tsTx) == TRUE)
//        {
//#if(k_TWO_STEP==TRUE)
//          /* DIS_SendSync returns real send time */
//          s_tiFlwUpCorr.ll_scld_Nsec = 0;
//          /* send follow up msg  */
//          DIS_SendFollowUp(NULL,
//                           MST_as_pId[w_hdl - MAS1_TSK].w_portNmb-1,
//                           aw_seqId[w_hdl - MAS1_TSK],
//                           &s_tiFlwUpCorr,
//                           MST_ac_synIntv[w_hdl - MAS1_TSK],
//                           &s_tsTx);
//#endif /*/#if(k_TWO_STEP==TRUE)*/
//        }
        /* reset timer for next sync message */
        SIS_TimerStart(w_hdl,MST_adw_SynIntvTmo[w_hdl-MAS1_TSK]); 
      }
      else
      {
        /* set error - there shouldn�t be a timeout */
        PTP_SetError(k_MST_ERR_ID,MST_k_TMO_ERR_MS,e_SEVC_ERR);
      }
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
#endif /*#if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC))*/
