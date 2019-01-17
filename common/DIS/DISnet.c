/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: DISnet.c 
**    Summary: The DISnet task handles initialization and reinitialization
**             of the net interfaces and the link speed detection to 
**             get the inbound and outbound latencies.
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: DISnet_Init
**             DISnet_Task
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
#include "DIS/DISint.h"

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
** Function    : DISnet_Init
**
** Description : Initializes the network interfaces.
**              
** Parameters  : dw_amntNetIf (IN) - number of initializable net interfaces
**               pao_ifInit  (OUT) - array of flags indicating if interfaces
**                                   are initialized.
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DISnet_Init(UINT32 dw_amntNetIf, BOOLEAN *pao_ifInit)
{
  UINT16 w_ifIdx;
  /* Initialize net interface struct */
  DIS_s_NetSts.dw_amntIf = dw_amntNetIf;
  DIS_s_NetSts.o_allInit = TRUE;
  for( w_ifIdx = 0 ; w_ifIdx < dw_amntNetIf ; w_ifIdx++ )
  {
    /* try to initialize the net interface */
    DIS_s_NetSts.ao_mcIfInit[w_ifIdx] = NIF_InitMcConn(w_ifIdx);
    pao_ifInit[w_ifIdx] = DIS_s_NetSts.ao_mcIfInit[w_ifIdx];
    if( DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == FALSE)
    {
      /* not all net interfaces are operational now */
      DIS_s_NetSts.o_allInit = FALSE;
      /* Set timer to 1 second to retry the initialization */
      SIS_TimerStart(DISnet_TSK,k_PTP_TIME_SEC);
    }
    /* initialize timestamp latencies */
    else
    {
      GOE_getInbLat(w_ifIdx,&DIS_as_inbLat[w_ifIdx]);
      GOE_getOutbLat(w_ifIdx,&DIS_as_outbLat[w_ifIdx]);
    }
    /* set the fault count to zero */
    DIS_s_NetSts.ab_ifFltCnt[w_ifIdx] = 0;
  }
/* just for unicast devices */
#if( k_UNICAST_CPBL == TRUE )
  /* try to initialize the unicast interface */
  DIS_s_NetSts.o_ucIfInit = NIF_InitUcConn();
  if( DIS_s_NetSts.o_ucIfInit == FALSE )
  {
    /* not all net interfaces are operational now */
    DIS_s_NetSts.o_allInit = FALSE;
  }
#endif /* #if( k_UNICAST_CPBL == TRUE ) */
/* shall frequently latency check work ? */
#if( k_FRQ_LAT_CHK == TRUE )
  /* start timer for frequently latency check */
  SIS_TimerStart(DISlat_TSK,k_LAT_CHK_INTV);  
#endif /* #if( k_FRQ_LAT_CHK == TRUE ) */
}

/***********************************************************************
**
** Function    : DISnet_Task
**
** Description : The DISnet_Task handles the net interfaces. Initialization,
**               deinitialization and link speed/latency detection is
**               done.
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void DISnet_Task(UINT16 w_hdl)
{
  static UINT16            w_ifIdx;
  static UINT32            dw_evnt;
  static PTP_t_MboxMsg     *ps_msg,s_msg; 
 
  /* starting point of multitasking */
  SIS_Continue(w_hdl); 

  while( TRUE ) /*lint !e716 */
  {
    /* Events */
    dw_evnt=SIS_EventGet(k_EVT_ALL);
    if( dw_evnt != 0 )
    {
      /* no events defined */
      PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_EVT,e_SEVC_NOTC);
    }   
    /* got ready through message?  */
    while( (ps_msg = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    {
      /* deinitialization of one network communication interface */
      if( ps_msg->e_mType == e_IMSG_NETDEINIT )
      {
        w_ifIdx = ps_msg->s_etc1.w_u16;
        /* deinitialize communication interface */
        NIF_CloseMcConn(w_ifIdx);        
        /* try to reinitialize communication interface */
        DIS_s_NetSts.ao_mcIfInit[w_ifIdx] = NIF_InitMcConn(w_ifIdx);
        /* start timer to reinitialize connection immediately */
        SIS_TimerStart(DISnet_TSK,1);
      }
      else
      {
        /* no messages defined */
        PTP_SetError(k_DIS_ERR_ID,DIS_k_ERR_MSGTP,e_SEVC_NOTC);
      }
      /* Release Mbox-entry */ 
      SIS_MboxRelease();
    }    
    /* net reinit timeout */
    if( SIS_TimerStsRead( DISnet_TSK ) == SIS_k_TIMER_EXPIRED )
    {
      /* preset flag */
      DIS_s_NetSts.o_allInit = TRUE;
      for( w_ifIdx = 0 ; w_ifIdx < DIS_s_NetSts.dw_amntIf ; w_ifIdx++ )
      {
        /* check, if this needs to be initialized */
        if( DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == FALSE )
        {
          /* try to initialize the net interface */
          DIS_s_NetSts.ao_mcIfInit[w_ifIdx] = NIF_InitMcConn(w_ifIdx);
          if( DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == FALSE)
          {
            /* not all net interfaces are operational now */
            DIS_s_NetSts.o_allInit = FALSE;
            /* Set timer to 1 sec to retry to init */
            SIS_TimerStart(DISnet_TSK,k_PTP_TIME_SEC);
          }
          /* initialize timestamp latencies for new initialized ports */
          else
          {
            GOE_getInbLat(w_ifIdx,&DIS_as_inbLat[w_ifIdx]);
            GOE_getOutbLat(w_ifIdx,&DIS_as_outbLat[w_ifIdx]);
            /* report CTL unit of functional device */
            s_msg.e_mType           = e_IMSG_NETINIT;
            s_msg.s_pntData.pv_data = NULL;
            s_msg.s_etc1.w_u16      = w_ifIdx;
            /* try to send it to CTL task */
            while( SIS_MboxPut(CTL_TSK,&s_msg) == FALSE )
            {
              /* set warning */
              PTP_SetError(k_DIS_ERR_ID,DIS_k_MBOX_WAIT,e_SEVC_WARN);
              SIS_MboxWListAdd(w_hdl,CTL_TSK);
              SIS_Break(w_hdl,1); /*lint !e646 !e717*/ 
            }
          }
        }
      }
#if( k_UNICAST_CPBL == TRUE )
      /* check, if unicast has to be reinitialized */
      if( DIS_s_NetSts.o_ucIfInit == FALSE )
      {
        /* try to initialize the unicast interface */
        DIS_s_NetSts.o_ucIfInit = NIF_InitUcConn();
        if( DIS_s_NetSts.o_ucIfInit == FALSE )
        {
          /* not all net interfaces are operational now */
          DIS_s_NetSts.o_allInit = FALSE;
        }
        /* restart timer */
        SIS_TimerStart(DISnet_TSK,k_PTP_TIME_SEC);
      }
#endif
    }
    /* net latency timeout */
    if( SIS_TimerStsRead( DISlat_TSK ) == SIS_k_TIMER_EXPIRED )
    {
/* shall frequent latency detection work ? */
#if( k_FRQ_LAT_CHK == TRUE )
      for( w_ifIdx = 0 ; w_ifIdx < DIS_s_NetSts.dw_amntIf ; w_ifIdx++ )
      {
        /* just get latencies for initialized interfaces */
        if( DIS_s_NetSts.ao_mcIfInit[w_ifIdx] == TRUE )
        {
          /* get inbound latency */
          GOE_getInbLat(w_ifIdx,&DIS_as_inbLat[w_ifIdx]);
          /* get outbound latency */
          GOE_getOutbLat(w_ifIdx,&DIS_as_outbLat[w_ifIdx]);
        }
      }
      /* restart this timer */
      SIS_TimerStart(DISlat_TSK,k_LAT_CHK_INTV);      
#endif /* #if( k_FRQ_LAT_CHK == TRUE ) */
    }
    /* cooperative multitasking - give cpu to other tasks */ 
    SIS_Break(w_hdl,2); /*lint !e646 !e717*/ 
  }
  /* finish multitasking */
  SIS_Return(w_hdl,0); /*lint !e744 */
}

/*************************************************************************
**    static functions
*************************************************************************/
