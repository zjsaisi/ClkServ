/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: MSTannc.c 
**    Summary: The unit MST encapsulates all master activities of a PTP node.
**             The MSTannc task issues announce messages for all ports
**             that are in state MASTER
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: MST_SetAnncInt
**             MSTannc_Task
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
#include "DIS/DIS.h"
#include "MST/MST.h"
#include "MST/MSTint.h"

/* not used for TCs without OC */
#if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC))
/*************************************************************************
**    global variables
*************************************************************************/
static UINT32 MST_adw_AncIntvTmo[k_NUM_IF];
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
** Function    : MST_SetAnncInt
**
** Description : Set the log of sync interval
**
** Parameters  : w_ifIdx    (IN) - communication interface index to change 
**               c_anncIntv (IN) - exponent for calculating the sync 
**                                 interval
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void MST_SetAnncInt(UINT16 w_ifIdx,INT8 c_anncIntv)
{
  /* set the sync interval timeout */
  MST_adw_AncIntvTmo[w_ifIdx] = PTP_getTimeIntv(c_anncIntv);
}

/***********************************************************************
**
** Function    : MSTannc_Task
**
** Description : The MSTannc task is responsible for sending announce
**               messages on all ports in MASTER state
**              
** Parameters  : w_hdl  (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : Function is reentrant.
**
***********************************************************************/
void MSTannc_Task( UINT16 w_hdl )
{
  UINT32              dw_evnt;
  UINT32              dw_qual_tmo;
  UINT16              w_tskIdx = w_hdl - MAA1_TSK;
  static UINT16       aw_seqId[k_NUM_IF] = {0};  

  static PTP_t_MboxMsg *aps_msg[k_NUM_IF];
  static UINT8         ab_taskSts[k_NUM_IF];  

  /* multitasking restart point - generates a 
     jump to the last BREAK at env */
  SIS_Continue(w_hdl);    
    /* initialize the Task with status = k_MST_STOPPED  */
    ab_taskSts[w_tskIdx] = k_MST_STOPPED;
  while( TRUE ) /*lint !e716 */
  {       
    /* got ready with a event?  */
    dw_evnt=SIS_EventGet(k_EVT_ALL);    
    if( dw_evnt != 0 )
    {
      /* stop master announce task on the events: */
      if( dw_evnt & (k_EV_SC_PSV  | /* event state change passive */
                     k_EV_SC_FLT  | /* event state change faulty */
                     k_EV_SC_DSBL | /* event state change disabled */
                     k_EV_INIT    | /* event init */
                     k_EV_SC_SLV))  /* event state change slave */
      {
        /* change task state to stopped */
        ab_taskSts[w_tskIdx] = k_MST_STOPPED;
        /* stop timeouts */
        SIS_TimerStop(w_hdl);
      }
    }
    /* got ready with message arrival?  */
    while( ( aps_msg[w_tskIdx] = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    { 
      if( aps_msg[w_tskIdx]->e_mType == e_IMSG_SC_MST )
      {
        /* master task was passive - now first change in PRE_MASTER state */
        ab_taskSts[w_tskIdx] = k_MST_PRE_MASTER;            
        /* get multiplicator for the QUALIFICATION_TIMEOUT */
        dw_qual_tmo = aps_msg[w_tskIdx]->s_etc1.dw_u32;
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
        /* unexpected message */
        PTP_SetError(k_MST_ERR_ID,MST_k_UNKWN_MSG,e_SEVC_ERR);
      }
      /* release mbox entry */
      SIS_MboxRelease();
    }
    /* got ready with elapsed timer?  */
    if( SIS_TimerStsRead( w_hdl ) == SIS_k_TIMER_EXPIRED )
    {
      if( ab_taskSts[w_tskIdx] == k_MST_PRE_MASTER )
      {
        /* change to operational state */
        ab_taskSts[w_tskIdx] = k_MST_OPERATIONAL;
      }
      if( ab_taskSts[w_tskIdx] == k_MST_OPERATIONAL )
      {
        /* increment sequence id */
        aw_seqId[w_hdl-MAA1_TSK]++;
        /* issue an announce message */
// jyang: temporarily block master output
//        DIS_SendAnnc(NULL,
//                     MST_as_pId[w_hdl-MAA1_TSK].w_portNmb-1,
//                     aw_seqId[w_hdl-MAA1_TSK]);
        /* restart timer */
        SIS_TimerStart(w_hdl,MST_adw_AncIntvTmo[w_hdl-MAA1_TSK]);
      }
      else
      {
        /* set error - there shouldn�t be a timeout */
        PTP_SetError(k_MST_ERR_ID,MST_k_TMO_ERR_MA,e_SEVC_ERR);
      }
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
