/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: CTLmain.c 
**    Summary: The control unit CTL controls all node data and all events
**             for the node and his channels to the net. A central task 
**             of the control unit is the state decision event. Within the 
**             announce interval, the unit ctl calculates a recommended 
**             state for each port of the node, and switches the channels 
**             to this state. In the state decision event, the best master
**             clock algorithm is executed. This algorithm determines the 
**             best clock in the PTP domain and configures the node to the
**             required state to work correct in the synchronization 
**             hierarchy.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: CTL_Init
**             CTL_Close
**             CTL_GetActPeerDelay
**             CTL_Task
**             CTL_ResetClockDrift
**             CTL_GetErrStr
**
**             Startup
**             ChangeDomain
**             GetAnncRecptTmo
**             GetMinAnncIntvTmo
**             InitUcDiscovery
**             Synchronize
**             SyncTimeOfDay
**             SyncFreq
**             GoToFaultyState
**             HandleSendError
**             ClearPathDelay
**             HandleMntMsg
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
#include "CIF/CIF.h"
#include "TSU/TSU.h"
#include "DIS/DIS.h"
#include "SLV/SLV.h"
#include "MST/MST.h"
#include "TCF/TCF.h"
#include "P2P/P2P.h"
#include "MNT/MNT.h"
#include "MNT/MNTapi.h"
#include "UCM/UCM.h"
#include "UCD/UCD.h"
#include "FIO/FIO.h"
#include "CTL/CTL.h"
#include "CTL/CTLint.h"
#include "CTL/CTLflt.h"
#include "APH/APH.h"
#include "APH/APHint.h"
#include "TGT/TGT.h"
#include "include/sc_api.h"
#include "CLK/sc_ptp_servo.h"

/*************************************************************************
**    global variables
*************************************************************************/

/* Handles of the tasks to address messageboxes */
UINT16 CTL_aw_Hdl_mas[k_NUM_IF];
UINT16 CTL_aw_Hdl_mad[k_NUM_IF];
UINT16 CTL_aw_Hdl_maa[k_NUM_IF];
#if( k_CLK_DEL_P2P == TRUE )
UINT16 CTL_aw_Hdl_p2p_del[k_NUM_IF];
UINT16 CTL_aw_Hdl_p2p_resp[k_NUM_IF];
#endif
/* Clock data set */
PTP_t_ClkDs CTL_s_ClkDS;
#if( k_UNICAST_CPBL == TRUE )
/* unicast master table */
PTP_t_PortAddrQryTbl s_ucMstTbl;
#endif /* if( k_UNICAST_CPBL == TRUE ) */
/* the connected channel port IDs */
CTL_t_commIf CTL_s_comIf;
/* pointer to external variable, that holds pointer to 
   external time properties data set. This data set is filled 
   with the data of an external primary clock source */
PTP_t_TmPropDs* CTL_ps_extTmPropDs;
/* jyang: extend announce timeout when changing announce interval */
UINT16 CTL_w_anncTmoExt[k_NUM_IF][k_CLK_NMB_FRGN_RCD];

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
#define k_IFIDX_ALL (0xFFFF)
/*************************************************************************
**    static function-prototypes
*************************************************************************/
static BOOLEAN Startup(void);
#if((k_CLK_IS_OC) || (k_CLK_IS_BC))
static void ChangeDomain( UINT8 b_domain );
static UINT32 GetAnncRecptTmo( UINT16 w_ifIdx );
static UINT32 GetMinAnncIntvTmo( void );
#if( k_UNICAST_CPBL == TRUE )
static BOOLEAN InitUcDiscovery( void );
#endif /* #if( k_UNICAST_CPBL == TRUE ) */
static INT64 Synchronize( const PTP_t_TmIntv *ps_offs,
                                UINT32       dw_TaUsec,
                                BOOLEAN      o_reset);
#if( k_SYNC_MODE == k_SYNC_TM_OF_DAY )                      
//static INT64 SyncTimeOfDay( const PTP_t_TmIntv *ps_offs,
//                                  UINT32       dw_TaUsec,
//                                  BOOLEAN      o_reset);
#endif                            
#if( k_SYNC_MODE == k_SYNC_FREQ )                               
static INT64 SyncFreq( const PTP_t_TmIntv *ps_offs,
                             UINT32       dw_TaUsec,
                             BOOLEAN      o_reset);
#endif
static void    GoToFaultyState( UINT16 w_ifIdx);
#endif /* #if((k_CLK_IS_OC) || (k_CLK_IS_BC)) */
static UINT8 HandleMntMsg( PTP_t_MboxMsg **pps_msg,
                           UINT8         b_state,
                           UINT32        *pdw_anncRcptTmo,
                           UINT16        w_actSlvPortIdx);
static void HandleSendError( UINT16 w_ifIdx , BOOLEAN o_reset);
#if( k_CLK_DEL_P2P == TRUE )
static void ClearPathDelay( void );
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**
** Function    : CTL_Init
**
** Description : Initializes the Unit CTL 
**               The task handles of the MST_Task, SLV_SyncTask and
**               SLV_DelayTask get initialized. The handles must be 
**               declared and grouped in the SIS configuration in 
**               increasing order ( MST1_TSK,MST2_TSK ... MSTn_TSK
**                                  ...                            )
**              
** Parameters  : ps_tmPropDs (IN) - pointer to external time properties
**                                  data set provided by an external 
**                                  primary reference clock.
**                                  Must be NULL, if there is none.
**               ps_clkId   (OUT) - clock id of own node
**               
**
** Returnvalue : <> NULL - function succeeded, 
**                         read-only pointer to Clock data set
**               == NULL - function failed
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
const PTP_t_ClkDs* CTL_Init(PTP_t_TmPropDs* ps_tmPropDs,PTP_t_ClkId *ps_clkId)
{
  UINT16  w_ifIdx;
  const PTP_t_ClkDs *ps_clkDs;
  /* Initialize the taskhandles */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
#if((k_CLK_IS_OC) || (k_CLK_IS_BC))
    /* task handles of MST_Tasks */
    CTL_aw_Hdl_mas[w_ifIdx]      = MAS1_TSK + w_ifIdx;
    CTL_aw_Hdl_mad[w_ifIdx]      = MAD1_TSK + w_ifIdx;
    CTL_aw_Hdl_maa[w_ifIdx]      = MAA1_TSK + w_ifIdx; 
#endif
#if( k_CLK_DEL_P2P == TRUE )  
    CTL_aw_Hdl_p2p_del[w_ifIdx]  = P2Pdel_TSK1 + w_ifIdx;
    CTL_aw_Hdl_p2p_resp[w_ifIdx] = P2Prsp_TSK1 + w_ifIdx;
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
  } 
  /* initialize extended announce timeout */
  memset(CTL_w_anncTmoExt, 0, sizeof(CTL_w_anncTmoExt));
  /* initialize utc pointer */
  CTL_ps_extTmPropDs = ps_tmPropDs;
  /* try to startup the stack */
  if( Startup() == TRUE )
  {
    /* copy clock id */
#if( k_CLK_IS_TC == TRUE )
    *ps_clkId = CTL_s_ClkDS.s_TcDefDs.s_clkId;
#else /* #if( k_CLK_IS_TC == TRUE ) */
    *ps_clkId = CTL_s_ClkDS.s_DefDs.s_clkId;
#endif /* #if( k_CLK_IS_TC == TRUE ) */
    /* Set units own task to ready */
    SIS_TaskExeReq(CTL_TSK);  
    ps_clkDs = &CTL_s_ClkDS;
  }
  else
  {
    ps_clkDs = NULL;
  }
  return ps_clkDs;
}

/***********************************************************************
**
** Function    : CTL_Close
**
** Description : Deinitializes the Unit CTL.
**              
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_Close(void)
{
  PTP_t_cfgClkRcvr s_cRcvr;
  UINT16           w_ifIdx;

  /* just for unicast devices - cancel all unicast services */ 
#if((k_UNICAST_CPBL == TRUE)&&((k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC == TRUE)))
  UCD_Close();  
#endif 
  /* begin with net interfaces */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    NIF_CloseMcConn(w_ifIdx);
  }
  /* just for unicast devices */ 
#if((k_UNICAST_CPBL==TRUE) && ((k_CLK_IS_OC==TRUE) || (k_CLK_IS_BC == TRUE)))
  NIF_CloseUcConn();
#endif 
  /* close timestamp units */
  TSU_Close(CTL_s_comIf.dw_amntInitComIf);  
  /* read configuration */
  if( FIO_ReadClkRcvr(&s_cRcvr) == FALSE )
  {
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_CRCVR,e_SEVC_NOTC);
  }  
#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
  /* store mean drift */
  s_cRcvr.ll_drift_ppb = CTL_s_ClkDS.ll_drift_ppb;
#if( k_CLK_DEL_E2E == TRUE )
  s_cRcvr.ll_E2EmeanPDel = CTL_s_ClkDS.s_CurDs.s_tiMeanPathDel.ll_scld_Nsec;
#else /* #if( k_CLK_DEL_E2E == TRUE ) */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    /* write mean path delay in configuration set */
    s_cRcvr.all_P2PmeanPDel[w_ifIdx] = 
                  CTL_s_ClkDS.as_PortDs[w_ifIdx].s_tiP2PMPDel.ll_scld_Nsec;
  }
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
  /* write new data */
  if( FIO_WriteClkRcvr(&s_cRcvr) == FALSE )
  {
    PTP_SetError(k_CTL_ERR_ID,CTL_k_CLOSE_ERR_0,e_SEVC_NOTC);
  }
  /* deinitialize target */
  GOE_Close();
  /* close clock interface */
  CIF_Close();
  TGT_Close();
}


#if( k_CLK_DEL_P2P == TRUE )
/***********************************************************************
**
** Function    : CTL_GetActPeerDelay
**
** Description : Returns the actual peer delay of the requested interface.
**              
** Parameters  : w_ifIdx  (IN) - communication interface index 
**                               to get the peer delay.
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
INT64 CTL_GetActPeerDelay(UINT16 w_ifIdx)
{
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
  return CTL_s_ClkDS.as_PortDs[w_ifIdx].s_tiP2PMPDel.ll_scld_Nsec;
#else
#if (k_CLK_IS_TC == TRUE )
  return CTL_s_ClkDS.as_TcPortDs[w_ifIdx].s_peerMPDel.ll_scld_Nsec;
#endif /* #if (k_CLK_IS_TC == TRUE ) */
#endif /*#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
}
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

/***********************************************************************
**
** Function    : CTL_Task
**
** Description : The controltask is the central control unit of the 
**               protocol. Switching of the port state and handling of 
**               the clock data sets are managed centrally. 
**              
** Parameters  : w_hdl           (IN) - the SIS-task-handle            
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void CTL_Task(UINT16 w_hdl)
{    
  PTP_t_MboxMsg       *ps_msg;
  UINT8               b_mode; 
#if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE )) 
  static INT64        ll_temp;
  PTP_t_PortDs        *ps_portDs;
  PTP_t_CurDs         *ps_curDs;
  PTP_t_PortAddr      *ps_pAddr;
  INT64               ll_nsecCompl;
  static INT64        ll_sec;
  static PTP_t_TmIntv s_tiMtsdUnfltrd;
  static PTP_t_TmIntv s_tiMtsdFltrd;
  const PTP_t_TmStmp  *ps_origTsSyn,*ps_rxTsSyn;
  static UINT32       dw_missDlRsp = 0; 
  static UINT32       dw_missFlwUp = 0; 
  static UINT32       dw_missSync  = 0;  
#if( k_CLK_DEL_E2E == TRUE )
  const PTP_t_TmStmp  *ps_txTsDlrq,*ps_rxTsDlrq;
  static PTP_t_TmIntv s_tiStmdUnfltrd;
  static PTP_t_TmIntv s_tiStmdFltrd;
  static PTP_t_TmIntv s_tiOwdUnfltrd;
  PTP_t_TmIntv        s_owd;
  static INT8         c_dlrqIntv;
  static UINT32       dw_delIntvUsec;
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
  INT32               l_nsec;
  PTP_t_TmIntv        s_tiNsec;    
  BOOLEAN             ao_anncRcptTmoExp[k_NUM_IF] = {FALSE};
  BOOLEAN             ao_rstAnncRcptTmo[k_NUM_IF] = {FALSE};
  NIF_t_PTPV2_AnnMsg  *ps_msgAnnc;
  static INT8         c_newSynIntv = 0;
  static INT8         c_actSynIntv = 0;
  static UINT32       dw_synIntvUsec;
  UINT32              dw_offsIntvUsec;
#endif /* #if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE )) */
#if( k_CLK_DEL_P2P == TRUE )
  PTP_t_TmIntv        *ps_tiP2PdelUnFlt;
  PTP_t_TmIntv        s_tiP2PdelFlt;
  PTP_t_TmStmp        *ps_txTsPdreq;
  PTP_t_TmStmp        *ps_rxTsPdresp;
  UINT32              dw_P2PintvUsec;
  INT8                c_ldP2PDelIntv;
#if( k_CLK_IS_TC == TRUE )
  PTP_t_TcPortDs      *ps_tcPortDs;
#endif /* #if( k_CLK_IS_TC == TRUE ) */
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
  static UINT32       adw_anncRcptTmo[k_NUM_IF];
  static UINT16       w_actSlvPort = 0;
  UINT16              w_ifIdx;
  static UINT8        b_state;  
  PTP_t_fltCfg        s_fltCfg;
  
  /* starting point for the task */
  SIS_Continue(w_hdl); 
    /* force the control task to work at startup */
    SIS_TaskExeReq(CTL_TSK); 
    /* start the loop in state OPERATIONAL */
    b_state = CTL_k_PRE_OPERATIONAL;
#if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE ))
    /* initialize variables */
    s_tiMtsdUnfltrd.ll_scld_Nsec = 0;
    s_tiMtsdFltrd.ll_scld_Nsec = 0;
#if( k_CLK_DEL_E2E == TRUE )    
    s_tiStmdUnfltrd.ll_scld_Nsec = 0;
    s_tiStmdFltrd.ll_scld_Nsec = 0;
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
#endif /* #if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE )) */
      /* try to open and read filter config file */
      if( FIO_ReadFltCfg(&s_fltCfg) == FALSE )
      {
        PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_FILT_RD,e_SEVC_INFO);
        /* try to write it in the configuration file */
        if( FIO_WriteFltCfg(&s_fltCfg) == FALSE )
        {
          PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_FILT_WR,e_SEVC_INFO);
        }
      }
      /* initialize filter algorithm */
      CTLflt_Init(&s_fltCfg);
  /* the tasks main loop */
  while( TRUE )/*lint !e716 */
  {
    /* transient state for reinitialization */
    if( b_state == CTL_k_REINIT )
    {
      /* first close all units */
      CTL_Close();
      /* call startup routine */
      if( Startup() == TRUE)
      {
        /* change in operation state*/
        b_state = CTL_k_PRE_OPERATIONAL;
#ifdef MNT_API
        /* unlock MNT API */
        MNT_UnlockApi();
#endif /* #ifdef MNT_API */
      }
      else
      {
        /* cooperative multitasking - give cpu to other tasks */
        SIS_Break(w_hdl,1);  /*lint !e646 !e717 */ 
      }      
    }
    /* transient state for pre-operational computations -
       there must always be reinitialization before changing in this state */
    if( b_state == CTL_k_PRE_OPERATIONAL )
    {
      /* reset send-error count */
      HandleSendError(k_IFIDX_ALL,TRUE);
      /* reset filter */
      CTLflt_resetFilter();
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
      /* reset error variables */
      dw_missDlRsp = 0;
      dw_missFlwUp = 0;
      dw_missSync  = 0;
      /* calculate the announce receipt timeouts for all 
       communication interfaces */
      for( w_ifIdx = 0 ; w_ifIdx < CTL_s_comIf.dw_amntInitComIf ; w_ifIdx++ )
      {
        adw_anncRcptTmo[w_ifIdx] = GetAnncRecptTmo( w_ifIdx );
      }
#endif
      /* set timer to smallest announce receipt timeout 
        (should all be the same -> chapter 7.7.3.1) */
      /* change to state operational */
      b_state = CTL_k_OPERATIONAL;
    }
    /* operational state - non-transient */
    while( b_state == CTL_k_OPERATIONAL )
    {
      /* cooperative multitasking - give cpu to other tasks */
      SIS_Break(w_hdl,2);  /*lint !e646 !e717 */ 
      /* got ready through message */
      while( (ps_msg = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
      {
        switch( ps_msg->e_mType )
        {
          case e_IMSG_MM_SET:
          {
            /* call management message handler */
            b_state = HandleMntMsg(&ps_msg,
                                   b_state,
                                   adw_anncRcptTmo,
                                   w_actSlvPort);
            break;
          }
          /* reinitialization */
          case e_IMSG_INIT:
          {
#ifdef MNT_API
            /* lock MNT API */
            MNT_LockApi(); /* REMARK: unlock must be done after re-init */
            /* clear MNT API requests */
            MNT_PreInitClear( NULL, NULL );
#endif /* #ifdef MNT_API */
            /* switch to reinitializing state */
            b_state = CTL_k_REINIT;  
            break;
          }
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
          /* received announce message */
          case e_IMSG_ANNC:
          {
            /* get announce message */
            ps_msgAnnc = ps_msg->s_pntData.ps_annc;
            /* get port address */
            ps_pAddr   = ps_msg->s_pntExt1.ps_pAddr;
            /* get interface index */
            w_ifIdx    = ps_msg->s_etc1.w_u16;
            /* get according port data set */
            ps_portDs = &CTL_s_ClkDS.as_PortDs[w_ifIdx];
            /* get alternate master flag */
            if( GET_FLAG(ps_msgAnnc->s_ptpHead.w_flags,k_FLG_ALT_MASTER) 
                == FALSE )
            {
              /* update the foreign master data set with this annc. message */
              if((CTL_UpdtFrgnMstDs(ps_msgAnnc,w_ifIdx,ps_pAddr) == TRUE) &&
                 (ps_portDs->e_portSts != e_STS_UNCALIBRATED) &&
                 (ps_portDs->e_portSts != e_STS_SLAVE))
              {
                /* restart announce receipt timeout for passive port state,  
                   if announce message is from the master 
                   that made this interface passive */
                if( ps_portDs->e_portSts == e_STS_PASSIVE )
                {
#if( k_CLK_IS_TC == FALSE )
                  if( PTP_ComparePortId( &ps_portDs->s_pIdmadeMePsv, 
                                         &ps_msgAnnc->s_ptpHead.s_srcPortId)
                      == PTP_k_SAME )
                  {
                    /* restart announce receipt timeout */
                    adw_anncRcptTmo[w_ifIdx] = GetAnncRecptTmo( w_ifIdx );
                  }
#endif /* #if( k_CLK_IS_TC == FALSE ) */
                }
                /* restart announce receipt timeout 
                   for all other port states */
                else
                {
                  /* restart announce receipt timeout */
                  adw_anncRcptTmo[w_ifIdx] = GetAnncRecptTmo( w_ifIdx );
                }
              }
            }
            else
            {
              /* no alternate master option included in this stack */
            }
            /* free allocated memory */
            if( ps_pAddr != NULL )
            {
              SIS_Free(ps_pAddr);
            }
            break;
          }
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
#if( k_CLK_DEL_P2P == TRUE )
          /* new value for peer delay */
          case e_IMSG_ACT_PDEL:
          {
            w_ifIdx          = ps_msg->s_etc1.w_u16;    
            /* get rx timestamp of Pdel-resp and tx timestamp of Pdel-req */
            ps_rxTsPdresp    = ps_msg->s_pntExt1.ps_tmStmp;
            ps_txTsPdreq     = ps_msg->s_pntExt2.ps_tmStmp;
            /* get new peer delay */
            ps_tiP2PdelUnFlt = ps_msg->s_pntData.ps_tmIntv;
            /* get peer delay interval in usec */
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
            /* get pointer to according port data set */
            ps_portDs = &CTL_s_ClkDS.as_PortDs[w_ifIdx];
            c_ldP2PDelIntv = ps_portDs->c_PdelReqIntv;
            /* store unfiltered value */
            ps_portDs->s_tiP2PMPDelUf = *ps_tiP2PdelUnFlt;
#endif
#if( k_CLK_IS_TC == TRUE )
            /* get pointer to according transparent port data set */
            ps_tcPortDs = &CTL_s_ClkDS.as_TcPortDs[w_ifIdx];
            c_ldP2PDelIntv = ps_tcPortDs->c_logMPdelIntv;
            /* store unfiltered value */
            ps_tcPortDs->s_peerMPDelUf = *ps_tiP2PdelUnFlt;
#endif
            if( c_ldP2PDelIntv < 0 )
            {
              dw_P2PintvUsec = k_USEC_IN_SEC / 
                               (UINT32)PTP_PowerOf2((UINT8)-c_ldP2PDelIntv);
            }
            else
            {
              dw_P2PintvUsec = k_USEC_IN_SEC * 
                               (UINT32)PTP_PowerOf2((UINT8)c_ldP2PDelIntv);
            }
            /* call filter */
            CTLflt_inpP2Pdel(ps_tiP2PdelUnFlt,ps_txTsPdreq,ps_rxTsPdresp,
                             w_ifIdx,dw_P2PintvUsec,&s_tiP2PdelFlt,&b_mode);
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
            /* set value into unfiltered variable */
            ps_portDs->s_tiP2PMPDel = s_tiP2PdelFlt;
#endif
#if( k_CLK_IS_TC == TRUE )
            /* set value */
            ps_tcPortDs->s_peerMPDel = s_tiP2PdelFlt;
#endif /* #if( k_CLK_IS_TC == TRUE ) */
            /* free allocated memory */
            SIS_Free(ps_rxTsPdresp);
            SIS_Free(ps_txTsPdreq);
            SIS_Free(ps_tiP2PdelUnFlt);
            break;
          }
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
          /* send error */
          case e_IMSG_SEND_ERR:
          {
            w_ifIdx = ps_msg->s_etc1.w_u16;
            HandleSendError( w_ifIdx,FALSE );
            break;
          }
          /* reinitialized network interface */
          case e_IMSG_NETINIT:
          {
            /* get port */
            w_ifIdx = ps_msg->s_etc1.w_u16;
#if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE ))
            /* get pointer to according port data set */
            ps_portDs = &CTL_s_ClkDS.as_PortDs[w_ifIdx];
            /* fault cleared event */
            if( ps_portDs->e_portSts == e_STS_FAULTY )
            {
              /* set port state to listening */
              ps_portDs->e_portSts = e_STS_LISTENING;
              /* change state to initialize */
              SIS_EventSet(CTL_aw_Hdl_mas[w_ifIdx],k_EV_INIT);
              SIS_EventSet(CTL_aw_Hdl_mad[w_ifIdx],k_EV_INIT);
              SIS_EventSet(CTL_aw_Hdl_maa[w_ifIdx],k_EV_INIT);
#if( k_CLK_DEL_E2E == TRUE )
              c_dlrqIntv = ps_portDs->c_mMDlrqIntv;
              SLV_SetDelReqInt(w_ifIdx,c_dlrqIntv);
#else
              SIS_EventSet(CTL_aw_Hdl_p2p_del[w_ifIdx],k_EV_INIT);
              P2P_SetPDelReqInt(w_ifIdx,ps_portDs->c_PdelReqIntv);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
              SLV_SetSyncInt(w_ifIdx,ps_portDs->c_SynIntv);
              MST_SetSyncInt(w_ifIdx,ps_portDs->c_SynIntv);
              MST_SetAnncInt(w_ifIdx,ps_portDs->c_AnncIntv);
            }
#else /* #if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE )) */
#if( k_CLK_DEL_P2P == TRUE )
            SIS_EventSet(CTL_aw_Hdl_p2p_del[w_ifIdx],k_EV_INIT);
            P2P_SetPDelReqInt(w_ifIdx,
                              CTL_s_ClkDS.as_TcPortDs[w_ifIdx].c_logMPdelIntv); 
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
#endif /*#if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE )) */
            break;
          }   
#if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE ))
          /* slave got no sync message in sync interval 
             -> reset drift to mean drift */
          case e_IMSG_ADJP_END:
          {
            /* reset clock drift */
// jyang: not reset clock drift
//            CTL_ResetClockDrift();
            break;
          }
#if( k_CLK_DEL_E2E == TRUE )          
          /* Slave delay task detected slave to master delay */
          case e_IMSG_STMD:
          {
            /* get actual slave port number */
            w_actSlvPort  = ps_msg->s_etc1.w_u16;
            /* get rx and tx timetamps */
            ps_rxTsDlrq   = ps_msg->s_pntExt1.ps_tmStmp;
            ps_txTsDlrq   = ps_msg->s_pntExt2.ps_tmStmp;
            /* did interval change ? */
            /* jyang: in unicast mode, intv value is 0x7F. do not set. */
            if( ps_msg->s_etc2.c_s8 != c_dlrqIntv && ps_msg->s_etc2.c_s8 != 0x7F )
            {
              static INT8 prev_c_dlrqIntv = -6;
              /* check value with profile range */
              c_dlrqIntv = (INT8) PTP_ChkRngSetExtr(ps_msg->s_etc2.c_s8,
                                    CTL_s_ClkDS.as_PortDs[w_actSlvPort].c_mMDlrqIntv,
                                    CTL_s_ClkDS.s_prof.ac_delMIntRng[k_RNG_MAX]);
              if (prev_c_dlrqIntv != c_dlrqIntv)
              {
                /* set delay request interval to new value */
                SLV_SetDelReqInt(w_actSlvPort,c_dlrqIntv); 
                prev_c_dlrqIntv = c_dlrqIntv;
              }
              /* update delay request interval in port data set */
              /* jyang: do not change configuration if GM change rate */
              //CTL_s_ClkDS.as_PortDs[w_actSlvPort].c_mMDlrqIntv = c_dlrqIntv;
            }
            /* get delay request interval in usec */
            if( c_dlrqIntv < 0 )
            {
              dw_delIntvUsec = k_USEC_IN_SEC / 
                               (UINT32)PTP_PowerOf2((UINT8)-c_dlrqIntv);
            }
            else
            {
              dw_delIntvUsec = k_USEC_IN_SEC * 
                               (UINT32)PTP_PowerOf2((UINT8)c_dlrqIntv);
            }
            /* input actual slave to master delay into filter */
            s_tiStmdUnfltrd = *ps_msg->s_pntData.ps_tmIntv; 
            /* call the used filter algorithm */
            CTLflt_inpSTMD(&s_tiStmdUnfltrd,ps_rxTsDlrq,ps_txTsDlrq,
                           dw_delIntvUsec,&s_tiStmdFltrd,&s_owd,
                           &b_mode);
            CTL_s_ClkDS.s_CurDs.s_tiMeanPathDel = s_owd;
            /* calculate unfiltered owd */
            s_tiOwdUnfltrd.ll_scld_Nsec = (s_tiStmdUnfltrd.ll_scld_Nsec + 
                                           s_tiMtsdUnfltrd.ll_scld_Nsec) / (INT64)2;
            /* free allocated memory */
            SIS_Free(ps_msg->s_pntData.pv_data);
            SIS_Free(ps_rxTsDlrq);
            SIS_Free(ps_txTsDlrq);
            /* reset subsequent missing delay response error counter */
            dw_missDlRsp = 0;
            break;
          } 
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
          /* Slave sync task detected master to slave delay */
          case e_IMSG_MTSD:
          { 
            /* get actual slave port */
            w_actSlvPort = ps_msg->s_etc1.w_u16;
            /* get actual sync interval of master */
            c_newSynIntv = ps_msg->s_etc2.c_s8;
            /* get origin timestamp */
            ps_origTsSyn = ps_msg->s_pntExt1.ps_tmStmp;
            ps_rxTsSyn   = ps_msg->s_pntExt2.ps_tmStmp;
            /* check sync interval */
            /* jyang: in unicast mode, intv value is 0x7F. do not set. */
            if( c_newSynIntv > c_actSynIntv && c_newSynIntv != 0x7F )
            {
              /* set the new interval as actual sync interval */
              c_actSynIntv = c_newSynIntv;
              /* set the new interval in the slave sync task */
              SLV_SetSyncInt(w_actSlvPort,c_newSynIntv);
            }            
            /* get sync interval in usec */
            if( c_actSynIntv < 0 )
            {
              dw_synIntvUsec = k_USEC_IN_SEC / 
                               (UINT32)PTP_PowerOf2((UINT8)-c_actSynIntv);
            }
            else
            {
              dw_synIntvUsec = k_USEC_IN_SEC * 
                               (UINT32)PTP_PowerOf2((UINT8)c_actSynIntv);
            }
            /* get master to slave delay */
            s_tiMtsdUnfltrd = *ps_msg->s_pntData.ps_tmIntv;
            /* get pointer to port data set */
            ps_portDs = &CTL_s_ClkDS.as_PortDs[w_actSlvPort];
            /* get pointer to current data set */
            ps_curDs = &CTL_s_ClkDS.s_CurDs;
#if( k_CLK_DEL_E2E == TRUE )
            /* calculate offset from master with unfiltered values */
            ps_curDs->s_tiOffsFrmMst.ll_scld_Nsec 
              = s_tiMtsdUnfltrd.ll_scld_Nsec - s_tiOwdUnfltrd.ll_scld_Nsec;

#else
            ps_curDs->s_tiOffsFrmMst.ll_scld_Nsec
                           = s_tiMtsdUnfltrd.ll_scld_Nsec - 
                                      ps_portDs->s_tiP2PMPDel.ll_scld_Nsec;
#endif
            /* call filter to get filtered offset */
            if( CTLflt_inpMTSD(&s_tiMtsdUnfltrd,ps_origTsSyn,ps_rxTsSyn,
                               w_actSlvPort,dw_synIntvUsec,
                               &ps_curDs->s_tiFilteredOffs,&dw_offsIntvUsec,
                               &s_tiMtsdFltrd,&b_mode) == TRUE )
            {
              /* offset bigger than one second ? */
#ifndef LET_ALL_TS_THROUGH
              if( (ps_curDs->s_tiFilteredOffs.ll_scld_Nsec >  
                    k_MAX_SYNC_OFFS)                           || 
                  (-ps_curDs->s_tiFilteredOffs.ll_scld_Nsec > 
                    k_MAX_SYNC_OFFS))
              {
                /* hard time reset */
                PTP_SetSysTimeOffset(&s_tiMtsdUnfltrd);/*lint !e534 */
                /* reset synchronization */
                Synchronize(NULL,0,TRUE);/*lint !e534*/
                /* reset filter and variables */
                CTLflt_resetFilter();
                /* reset PDS statistics */
                CTL_ResetPDSstatistics();
                /* reset variables */
                s_tiMtsdUnfltrd.ll_scld_Nsec = 0LL;
                s_tiMtsdFltrd.ll_scld_Nsec   = 0LL;
                /* clear all path delay values */
#if( k_CLK_DEL_P2P == TRUE )
                ClearPathDelay();
#else
                s_tiStmdUnfltrd.ll_scld_Nsec = 0LL;
                s_tiStmdFltrd.ll_scld_Nsec   = 0LL;
                /* set delay request interval to same value 
                    as sync interval for startup */
                // jyang: not use sync interval
                //c_dlrqIntv = (INT8)PTP_ChkRngSetExtr(c_actSynIntv,
                c_dlrqIntv = (INT8)PTP_ChkRngSetExtr(
                               CTL_s_ClkDS.as_PortDs[w_actSlvPort].c_mMDlrqIntv,
                               CTL_s_ClkDS.s_prof.ac_delMIntRng[k_RNG_MIN],
                               CTL_s_ClkDS.s_prof.ac_delMIntRng[k_RNG_MAX]);
                SLV_SetDelReqInt(w_actSlvPort,c_dlrqIntv);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
              }    
              else
#endif
              {
                SC_goodPacketRstCounter();  /* clear attempt register when go packet */
                /* synchronize with minimizing offset_from_master */
                ll_temp = Synchronize(&ps_curDs->s_tiFilteredOffs,
                                      dw_offsIntvUsec,
                                      FALSE);
                /* calculate PDS statistics */
                CTL_SetPDSstatistics(&ll_temp,&CTL_s_ClkDS.ll_drift_ppb);
              }
            }              
            else
            {
              /* do not use returned value of filter */
            }
#if( k_CLK_DEL_E2E == TRUE )
            /* debug out */
            TGT_printSyncData(&ps_curDs->s_tiOffsFrmMst,
                              &ps_curDs->s_tiFilteredOffs,
                              &s_tiMtsdUnfltrd,
                              &s_tiMtsdFltrd,
                              &s_tiStmdUnfltrd,
                              &s_tiStmdFltrd,  
                              &s_tiOwdUnfltrd,
                              &ps_curDs->s_tiMeanPathDel);
#else
            TGT_printSyncData(&ps_curDs->s_tiOffsFrmMst,
                              &ps_curDs->s_tiFilteredOffs,
                              &s_tiMtsdUnfltrd,
                              &s_tiMtsdFltrd,
                              NULL,
                              NULL,
                              &ps_portDs->s_tiP2PMPDelUf,
                              &ps_portDs->s_tiP2PMPDel);        
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
            /* set offset from master to the filtered value 
               in the current data set for management issues */
            ps_curDs->s_tiOffsFrmMst = ps_curDs->s_tiFilteredOffs;
            /* print drift, if requested */
            TGT_printDrift(ll_temp);
            /* free allocated memory */
            SIS_Free(ps_msg->s_pntData.pv_data); /* MTSD time interval */
            SIS_Free(ps_origTsSyn); /* origin timestamp */
            SIS_Free(ps_rxTsSyn);   /* rx timestamp */
            /* reset subsequent missing follow up error counter */
            dw_missFlwUp = 0;
            dw_missSync  = 0;
            break;
          }
          /* Slave sync task detected master to slave delay bigger 
             than 140737 seconds (max value for PTP_t_TmIntv) */
          case e_IMSG_MTSDBIG:
          {
            /* get act slave port */
            w_actSlvPort = ps_msg->s_etc1.w_u16;
            /* get unscaled nanoseconds */
            ll_nsecCompl = *ps_msg->s_pntData.pll_s64;
            /* get seconds */
            ll_sec = ll_nsecCompl / (INT32)k_NSEC_IN_SEC;
            l_nsec = (INT32)(ll_nsecCompl % (INT32)k_NSEC_IN_SEC);
            s_tiNsec.ll_scld_Nsec = PTP_NSEC_TO_INTV(l_nsec);
            /* make big timejump */
            PTP_SetSysTimeOffsSec(&ll_sec);/*lint !e534 */
            /* set nanoseconds portion */
            PTP_SetSysTimeOffset(&s_tiNsec);/*lint !e534 */
            /* free allocated memory */
            SIS_Free(ps_msg->s_pntData.pv_data); /* MTSD time interval */
            SIS_Free(ps_msg->s_pntExt1.pv_data); /* origin timestamp */
            SIS_Free(ps_msg->s_pntExt2.pv_data); /* rx timestamp */
            /* reset subsequent missing follow up error counter */
            dw_missFlwUp = 0;
            dw_missSync  = 0;            
            /* reset filters - this is a time jump */
            Synchronize(NULL,0,TRUE);/*lint !e534 */
            CTLflt_resetFilter();
            /* reset PDS statistics */
            CTL_ResetPDSstatistics();
            /* reset variables */
            s_tiMtsdUnfltrd.ll_scld_Nsec = 0LL;
            s_tiMtsdFltrd.ll_scld_Nsec   = 0LL;
#if( k_CLK_DEL_E2E == TRUE )
            s_tiStmdUnfltrd.ll_scld_Nsec = 0LL;
            s_tiStmdFltrd.ll_scld_Nsec   = 0LL;
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
            break; 
          }
          /* an acceptable master entry */
          case e_IMSG_ACTBL_MST:
          {
            /* get interface index */
            w_ifIdx = ps_msg->s_etc3.w_u16;
            /* set the acceptable flag in the foreign master data set */
            CTL_SetAccpFlagFrgnMstDs(ps_msg->s_pntExt1.ps_pId,
                                     w_ifIdx,
                                     ps_msg->s_etc2.o_bool,
                                     ps_msg->s_etc1.o_bool);
            /* free allocated memory */
            SIS_Free(ps_msg->s_pntExt1.ps_pId);
            break;
          }
          /* missing delay response error */
          case e_IMSG_MDLRSP:
          {
            /* get port */
            w_ifIdx = ps_msg->s_etc1.w_u16;
            /* increment error counter for missed delay response */
            dw_missDlRsp++;
            /* threshold reached ?*/
            if( dw_missDlRsp > k_MISS_DLRSP_THRES )
            {
              /* change actual master to not accepted */
              CTL_SetAccpFlagFrgnMstDs(ps_msg->s_pntData.ps_pId,
                                       w_ifIdx,
                                       ps_msg->s_etc2.o_bool,
                                       FALSE);
              /* trigger next BMC immediately */
              SIS_TimerStart(w_hdl,1);
              /* reset error counter */
              dw_missDlRsp = 0;
              /* set error */
              PTP_SetError(k_CTL_ERR_ID,CTL_k_MDLRSP_ERR,e_SEVC_NOTC);
            }
            break;
          }
          /* missing follow up error */
          case e_IMSG_MFLWUP:
          {
            /* get port */
            w_ifIdx = ps_msg->s_etc1.w_u16;
            /* increment error counter for missed follow up */
            dw_missFlwUp++; 
            /* threshold reached ?*/
            if( dw_missFlwUp > k_MISS_FLWUP_THRES )
            {
              /* change actual master to not accepted */
              CTL_SetAccpFlagFrgnMstDs(ps_msg->s_pntData.ps_pId,
                                       w_ifIdx,
                                       ps_msg->s_etc2.o_bool,
                                       FALSE);
              /* set error */
              PTP_SetError(k_CTL_ERR_ID,CTL_k_MFLWUP_ERR,e_SEVC_NOTC);

            }
            break;
          }
          /* missing sync error */
          case e_IMSG_MSYNC:
          {
            /* get port */
            w_ifIdx = ps_msg->s_etc1.w_u16;
            /* increment error counter */
            dw_missSync++;
            /* threshold reached ?*/
            if( dw_missSync > k_MISS_SYNC_THRES )
            {
              /* change actual master to not accepted */
              CTL_SetAccpFlagFrgnMstDs(ps_msg->s_pntData.ps_pId,
                                       w_ifIdx,
                                       ps_msg->s_etc2.o_bool,
                                       FALSE);
              /* set error */
              PTP_SetError(k_CTL_ERR_ID,CTL_k_MSYNC_ERR,e_SEVC_NOTC);
            }
            break;
          }
#endif /* #if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE )) */
          default:
          {
            /* set warning - this messagetype is misdirected */
            PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_MSGTP,e_SEVC_WARN);
            break;
          }          
        }/*lint !e788*/
        /* release last mbox entry */
        if(ps_msg != NULL)
        {
          SIS_MboxRelease();
        }
      }
#if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE ))
      /* got ready with elapsed timer? */
      if( SIS_TimerStsRead( w_hdl ) == SIS_k_TIMER_EXPIRED )
      {
        /* check announce receipt timeout expired event */
        for( w_ifIdx = 0 ; w_ifIdx < CTL_s_comIf.dw_amntInitComIf ; w_ifIdx++ )
        {
          ao_anncRcptTmoExp[w_ifIdx] = SIS_TIME_OVER(adw_anncRcptTmo[w_ifIdx]);
        }
        /* execute state decision event */
        if( CTL_StateDecisionEvent(ao_anncRcptTmoExp,
                                   ao_rstAnncRcptTmo) == TRUE )
        {
          /* reset filters */
          Synchronize(NULL,0,TRUE); /*lint !e534 */ 
          CTLflt_resetFilter();
          /* reset PDS statistics */
          CTL_ResetPDSstatistics();
          /* reset error variables */
          dw_missDlRsp = 0;
          dw_missSync  = 0;
          dw_missFlwUp = 0;
          /* reset variables */
          s_tiMtsdUnfltrd.ll_scld_Nsec = 0LL;
          s_tiMtsdFltrd.ll_scld_Nsec   = 0LL;          
#if( k_CLK_DEL_E2E == TRUE )
          s_tiStmdUnfltrd.ll_scld_Nsec = 0LL;
          s_tiStmdFltrd.ll_scld_Nsec   = 0LL;
          /* set delay request interval to same value like 
             sync interval for startup */
          // jyang: not use sync interval
          //c_dlrqIntv = (INT8)
             //PTP_ChkRngSetExtr(CTL_s_ClkDS.as_PortDs[w_actSlvPort].c_SynIntv,
          c_dlrqIntv = (INT8)
             PTP_ChkRngSetExtr(CTL_s_ClkDS.as_PortDs[w_actSlvPort].c_mMDlrqIntv,
                               CTL_s_ClkDS.s_prof.ac_delMIntRng[k_RNG_MIN],
                               CTL_s_ClkDS.s_prof.ac_delMIntRng[k_RNG_MAX]);
          SLV_SetDelReqInt(w_actSlvPort,c_dlrqIntv);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
        }
        /* restart announce receipt timeout, if required */
        for( w_ifIdx = 0 ; w_ifIdx < CTL_s_comIf.dw_amntInitComIf ; w_ifIdx++ )
        {
          if( ao_rstAnncRcptTmo[w_ifIdx] == TRUE )
          {
            adw_anncRcptTmo[w_ifIdx] = GetAnncRecptTmo(w_ifIdx);
          }
        }
        /* reset timeout */
        SIS_TimerStart(w_hdl,GetMinAnncIntvTmo());
      }
#endif /* #if(( k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE )) */
    }
  }
  /* finish multitasking macros */
  SIS_Return(w_hdl,0); /*lint !e744 */
}

/***********************************************************************
**  
** Function    : CTL_ResetClockDrift
**  
** Description : Sets the clock drift to the stored drift value if 
**               specified in the configuration file, otherwise the
**               clock runs with its own speed.
**               This function gets called if the node gets master or if
**               sync messages are timed out.
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTL_ResetClockDrift( void )
{
  if( CTL_s_ClkDS.o_drift_use == TRUE )
  {
    CIF_AdjClkDrift( -CTL_s_ClkDS.ll_drift_ppb );/*lint !e534*/
  }
  else
  {
    CIF_AdjClkDrift( (INT64)0 );/*lint !e534*/
  }
}

#ifdef ERR_STR
/*************************************************************************
**
** Function    : CTL_GetErrStr
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
const CHAR* CTL_GetErrStr(UINT32 dw_errNmb)
{
  static CHAR ac_str[50];
  const CHAR *pc_ret;
  /* return according error string */
  switch(dw_errNmb)
  {
    case CTL_k_ERR_MSGTP:
    {
      pc_ret = "Wrong message type";
      break;
    }
    case CTL_k_ERR_INIT_1:
    case CTL_k_ERR_INIT_2:
    case CTL_k_ERR_INIT_3:
    case CTL_k_ERR_INIT_4:
    case CTL_k_ERR_INIT_5:
    {
      PTP_SPRINTF((ac_str,"Initialization error %d",
                   dw_errNmb-CTL_k_ERR_INIT_1));
      pc_ret = ac_str;
      break;
    }
    case CTL_k_STS_DEC:
    {
      pc_ret = "State decision code error";
      break;
    }
    case CTL_k_BMC_R_EQ_S:
    {
      pc_ret = "Receiver is equal to sender";
      break;
    }
    case CTL_k_BMC_A_EQ_B:
    {
      pc_ret = "Announce messages are identical";
      break;
    }
    case CTL_k_ERR_SCL:
    {
      pc_ret = "Error with scaling the drift change";
      break;
    }
    case CTL_k_ERR_SLSMBOX:
    {
      pc_ret = "Error sending msg to Slave sync task";
      break;
    }
    case CTL_k_ERR_SLDMBOX:
    {
      pc_ret = "Error sending msg to Slave delay task";
      break;
    }
    case CTL_k_ERR_UCMBOX:
    {
      pc_ret = "Error sending msg to Unicast master tasks";
      break;
    }
    case CTL_k_ERR_P2PMBOX:
    {
      pc_ret = "Error sending msg to P2Pdel task";
      break;
    }
    case CTL_k_ERR_UCDMBOX:
    {
      pc_ret = "Error sending msg to unicast discover task";
      break;
    }
    case CTL_k_ERR_MASMBOX:
    {
      pc_ret = "Error sending msg to Master sync task";
      break;
    }
    case CTL_k_ERR_MAAMBOX:
    {
      pc_ret = "Error sending msg to Master announce task";
      break;
    }
    case CTL_k_ERR_MADMBOX:
    {
      pc_ret = "Error sending msg to Master delay task";
      break;
    }
    case CTL_k_RD_UCTBL:
    {
      pc_ret = "Error reading unicast table";
      break;
    }
    case CTL_k_WR_UCTBL:
    {
      pc_ret = "Error writing unicast table";
      break;
    }
    case CTL_k_MDLRSP_ERR:
    {
      pc_ret = "Subsequent missing delay resp. error - master not acceptable";
      break;
    }
    case CTL_k_MSYNC_ERR:
    {
      pc_ret = "Subsequent missing sync error - master not acceptable"; 
      break;
    }
    case CTL_k_MFLWUP_ERR:
    {
      pc_ret = "Subsequent missing follow up error - master not acceptable"; 
      break;
    }
    case CTL_k_CLOSE_ERR_0:
    {
      pc_ret = "Error closing unit";
      break;
    }
    case CTL_k_ERR_FILT_INIT:
    {
      pc_ret = "Could not initialize filter - use no filter";
      break;
    }
    case CTL_k_ERR_FILT_DNE:
    {
      pc_ret = "Filter does not exist";
      break;
    }
    case CTL_k_ERR_FILT_WR:
    {
      pc_ret = "Error writing filter file";
      break;
    }
    case CTL_k_ERR_FILT_RD:
    {
      pc_ret = "Error reading filter file";
      break;
    }
    case CTL_k_ERR_FLT_FUNC:
    {
      pc_ret = "Wrong filter function initialization";
      break;
    }
    case CTL_k_CFGFILE_DEF:
    {
      pc_ret = "Error reading config file, default values will be written";
      break;
    }
    case CTL_k_ERR_CRCVR:
    {
      pc_ret = "Clock recovery file access not OK - default values used";
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
** Function    : Startup
**  
** Description : Initializes all units and starts them by event EV_INIT
**  
** Parameters  : - 
**               
** Returnvalue : -
** 
** Remarks     : function is not reentrant
**  
***********************************************************************/
static BOOLEAN Startup( void )
{
  UINT16           w_ifIdx;
  BOOLEAN          ao_ifInit[k_NUM_IF];
  PTP_t_cfgRom     s_cfgRom;
  PTP_t_cfgClkRcvr s_cRcvr;

  /* initialize target debug functions */
  TGT_Init();
  /* Init SIS */
  SIS_Init(); 
  /* preset port-number in all interfaces */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    CTL_s_comIf.as_pId[w_ifIdx].w_portNmb = w_ifIdx + 1;
  }  
  /* Initialize GOE */
  CTL_s_comIf.dw_amntInitComIf = GOE_Init(PTP_SetError,CTL_s_comIf.as_pId);
  /* Initialize clock interface */  
  if( CIF_Init(PTP_SetError) == FALSE )
  {
    /* set error */ 
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_INIT_2,e_SEVC_EMRGC);
    return FALSE;
  }  
  if( CTL_s_comIf.dw_amntInitComIf == 0 )
  {    
    /* set error */  
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_INIT_1,e_SEVC_EMRGC);
    /* close open resources */
    CIF_Close();/*lint !e534 */   
    return FALSE;
  }
  /* initialize hook API */
  APH_INIT();
  /* read clock recovery file out of non-volatile memory */
  if( FIO_ReadClkRcvr(&s_cRcvr) == FALSE )
  {
    /* try to write the file */
    if( FIO_WriteClkRcvr(&s_cRcvr ) == FALSE )
    {
      /* set notification */
      PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_CRCVR,e_SEVC_NOTC);
    }
  }
  /* initialize clock drift for clock drift recovery */  
  CTL_s_ClkDS.ll_drift_ppb = s_cRcvr.ll_drift_ppb;
  CTL_s_ClkDS.o_drift_use  = s_cRcvr.o_drift_use;
  /* Init Clock drift */
// jyang: not reset clock drift
//  CTL_ResetClockDrift();
  /* try to read configuration */
  if( FIO_ReadConfig(&s_cfgRom) == FALSE )
  {
    /* set error */
    PTP_SetError(k_CTL_ERR_ID,CTL_k_CFGFILE_DEF,e_SEVC_NOTC);
    /* try to write default config file */
    if( FIO_WriteConfig(&s_cfgRom) == FALSE )
    {
      /* set error */
      PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_INIT_3,e_SEVC_EMRGC);    
      /* close open resources */
      CIF_Close();
      return FALSE;
    }
  } 
  /* check profile ranges and set to default value, 
     if something is not correct */
  FIO_CheckCfg(&s_cfgRom);
  /* store the profile ranges */
  CTL_s_ClkDS.s_prof = s_cfgRom.s_prof;
  /* initialize user desc in clock data set */
  PTP_BCOPY(CTL_s_ClkDS.ac_userDesc,
            s_cfgRom.ac_userDesc,
            k_MAX_USRDESC_SZE);
  /* Initialize Clock data set */ 
  CTL_InitClockDS(&s_cfgRom,&s_cRcvr);
#if( k_UNICAST_CPBL == TRUE )
  /* initialize unicast units */
  UCD_Init(CTL_s_comIf.as_pId);
  UCM_Init(CTL_s_comIf.as_pId);
  SIS_EventSet(UCMann_TSK,k_EV_INIT);
  SIS_EventSet(UCMsyn_TSK,k_EV_INIT);
  /* initialization of unicast master table */
  if( InitUcDiscovery() == FALSE )
  {         
    /* set error */
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_INIT_4,e_SEVC_ERR);
    /* close open resources */
    CIF_Close();/*lint !e534 */
    return FALSE;
  }
#endif /* #if( k_UNICAST_CPBL == TRUE ) */
  
  /* Initialize the Unit DIS */ 
  DIS_Init(
#if((k_CLK_IS_OC) || (k_CLK_IS_BC)) 
            CTL_aw_Hdl_mas,
            CTL_aw_Hdl_mad,
            CTL_aw_Hdl_maa,
#endif /* #if((k_CLK_IS_OC) || (k_CLK_IS_BC))  */
#if(k_CLK_DEL_P2P == TRUE )
            CTL_aw_Hdl_p2p_del,
            CTL_aw_Hdl_p2p_resp,
#endif /* #if(k_CLK_DEL_P2P == TRUE ) */
            &CTL_s_ClkDS);
  /* starts dis task */
  SIS_EventSet(DIS_TSK,k_EV_INIT);
  /* Initialize the network interface */
  NIF_Init(&CTL_s_ClkDS);
#if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE ))
  /* set the PTP subdomain */  
  NIF_Set_PTP_subdomain(CTL_s_ClkDS.s_DefDs.b_domn);
#else
  NIF_Set_PTP_subdomain(CTL_s_ClkDS.s_TcDefDs.b_primDom);
#endif /* #if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE )) */    

#if( k_CLK_IS_TC == TRUE )
#if( k_CLK_DEL_P2P == TRUE )
  TCF_Init(CTL_s_comIf.as_pId,CTL_s_comIf.dw_amntInitComIf,CTL_GetActPeerDelay);
#else /* #if( k_CLK_DEL_P2P == TRUE ) */
  TCF_Init(CTL_s_comIf.as_pId,CTL_s_comIf.dw_amntInitComIf,NULL);
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
#endif /* #if( k_CLK_IS_TC == TRUE ) */
#if( k_CLK_DEL_P2P == TRUE )
  P2P_Init(CTL_s_comIf.as_pId,CTL_s_comIf.dw_amntInitComIf);
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
#if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE ))
  /* Initialize the unit SLV */ 
  SLV_Init(CTL_s_comIf.as_pId);
  /* Initialize unit MST */ 
  MST_Init(CTL_s_comIf.as_pId);
#endif /* #if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE )) */
  /* Initialize unit MNT */
  MNT_Init(CTL_s_comIf.as_pId,
            CTL_s_comIf.dw_amntInitComIf,
#if( k_UNICAST_CPBL == TRUE )
            &s_ucMstTbl,
#else /* if( k_UNICAST_CPBL == TRUE ) */
            NULL,
#endif /* if( k_UNICAST_CPBL == TRUE ) */
            &CTL_s_ClkDS);
  /* Initialize time stamp unit */
  if( TSU_Init(PTP_SetError,
               CTL_s_comIf.as_pId,
               CTL_s_comIf.dw_amntInitComIf)==FALSE )
  {
    /* set error */
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_INIT_5,e_SEVC_EMRGC);   
    /* close all open resources */
    CIF_Close();/*lint !e534 */
    return FALSE;
  }
  /* Initialize network interfaces */
  DISnet_Init(CTL_s_comIf.dw_amntInitComIf,ao_ifInit);
  /* start slave and master tasks */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {         
    if( ao_ifInit[w_ifIdx] == TRUE )
    {
#if((k_CLK_IS_OC) || (k_CLK_IS_BC))
      SIS_EventSet(SLS_TSK,k_EV_INIT);
#if( k_CLK_DEL_E2E == TRUE )
      SIS_EventSet(SLD_TSK,k_EV_INIT);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
      SIS_EventSet(CTL_aw_Hdl_mas[w_ifIdx],k_EV_INIT);
      SIS_EventSet(CTL_aw_Hdl_mad[w_ifIdx],k_EV_INIT);
      SIS_EventSet(CTL_aw_Hdl_maa[w_ifIdx],k_EV_INIT);
#endif /* #if((k_CLK_IS_OC) || (k_CLK_IS_BC)) */
#if(k_CLK_DEL_P2P == TRUE )
      SIS_EventSet(CTL_aw_Hdl_p2p_del[w_ifIdx],k_EV_INIT);
#endif
    }
#if( k_CLK_DEL_P2P == TRUE )
    if( ao_ifInit[w_ifIdx] == TRUE )
    {
#if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE ))          
      P2P_SetPDelReqInt(w_ifIdx,CTL_s_ClkDS.as_PortDs[w_ifIdx].c_PdelReqIntv);
#else
      P2P_SetPDelReqInt(w_ifIdx,
                        CTL_s_ClkDS.as_TcPortDs[w_ifIdx].c_logMPdelIntv);
#endif /* #if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE )) */
    }
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
#if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE ))
    if( ao_ifInit[w_ifIdx] == TRUE )
    {
      /* set port state to listening   */
      CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts = e_STS_LISTENING;
    }
    else
    {
      /* set port state to faulty   */
      CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts = e_STS_FAULTY;
    }
    /* set the new message intervals in all units   */
    SLV_SetSyncInt(w_ifIdx,CTL_s_ClkDS.as_PortDs[w_ifIdx].c_SynIntv);
#if( k_CLK_DEL_E2E == TRUE )
    SLV_SetDelReqInt(w_ifIdx,CTL_s_ClkDS.as_PortDs[w_ifIdx].c_mMDlrqIntv);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
    MST_SetSyncInt(w_ifIdx,CTL_s_ClkDS.as_PortDs[w_ifIdx].c_SynIntv);
    MST_SetAnncInt(w_ifIdx,CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv);
#endif /* #if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE )) */
  }
#if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE ))
  /* start state decision event timer of control task */
  SIS_TimerStart(CTL_TSK,GetMinAnncIntvTmo());   
#endif /* #if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE )) */
  /* initialization succeeded */
  return TRUE;
}


#if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE ))
/***********************************************************************
**  
** Function    : ChangeDomain
**  
** Description : Reinitializes the stack engine for a domain change.
**               The HW does not get re-initialized.
**
** Parameters  : b_domain (IN) - new domain
**               
** Returnvalue : -
** 
** Remarks     : function is not reentrant
**  
***********************************************************************/
static void ChangeDomain( UINT8 b_domain )
{
  UINT16        w_ifIdx;
  UINT8         b_entry;
  /* Init SIS */
  SIS_Init();      
#if( k_CLK_IS_TC == TRUE )
#if( k_CLK_DEL_P2P == TRUE )
  TCF_Init(CTL_s_comIf.as_pId,CTL_s_comIf.dw_amntInitComIf,CTL_GetActPeerDelay);
#else /* #if( k_CLK_DEL_P2P == TRUE ) */
  TCF_Init(CTL_s_comIf.as_pId,CTL_s_comIf.dw_amntInitComIf,NULL);
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
#endif /* #if( k_CLK_IS_TC == TRUE ) */
#if( k_CLK_DEL_P2P == TRUE )
  P2P_Init(CTL_s_comIf.as_pId,CTL_s_comIf.dw_amntInitComIf);
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
  /* Initialize the unit SLV */ 
  SLV_Init(CTL_s_comIf.as_pId);
  /* Initialize unit MST */ 
  MST_Init(CTL_s_comIf.as_pId);
  /* Initialize unit MNT */
  MNT_Init(CTL_s_comIf.as_pId,
            CTL_s_comIf.dw_amntInitComIf,
#if( k_UNICAST_CPBL == TRUE )
            &s_ucMstTbl,
#else /* if( k_UNICAST_CPBL == TRUE ) */
            NULL,
#endif /* if( k_UNICAST_CPBL == TRUE ) */
            &CTL_s_ClkDS);
  /* change domain in default data set */
  CTL_s_ClkDS.s_DefDs.b_domn = b_domain;
  /* CURRENT DATA SET */   
  CTL_ResetCurDs();
  /* PARENT DATA SET */ 
  CTL_InitParentDS();

  /* FOREIGN MASTER DATA SETS */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    /* initialize all records of the port */
    for( b_entry = 0 ; b_entry < k_CLK_NMB_FRGN_RCD ; b_entry++ )
    {
      /* call init function for port w_ifIdx and entry b_entry */
      CTL_InitFrgnMstDS(w_ifIdx,b_entry);
    }
  }
  /* Initialize the network interface */
  NIF_Init(&CTL_s_ClkDS);
  /* start slave and master tasks */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    if( CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts != e_STS_FAULTY )
    {
      SIS_EventSet(SLS_TSK,k_EV_INIT);
#if( k_CLK_DEL_E2E == TRUE )
      SIS_EventSet(SLD_TSK,k_EV_INIT);
#else
      SIS_EventSet(CTL_aw_Hdl_p2p_del[w_ifIdx],k_EV_INIT);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
      SIS_EventSet(CTL_aw_Hdl_mas[w_ifIdx],k_EV_INIT);
      SIS_EventSet(CTL_aw_Hdl_mad[w_ifIdx],k_EV_INIT);
      SIS_EventSet(CTL_aw_Hdl_maa[w_ifIdx],k_EV_INIT);

      /* set port state to listening   */
      CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts = e_STS_LISTENING;
    }
  }
#if( k_UNICAST_CPBL == TRUE )
  /* initialize unicast units */
  UCD_Init(CTL_s_comIf.as_pId);
  UCM_Init(CTL_s_comIf.as_pId);
  SIS_EventSet(UCMann_TSK,k_EV_INIT);
  SIS_EventSet(UCMsyn_TSK,k_EV_INIT);
  if( InitUcDiscovery() == FALSE )
  {
    /* set error */
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_INIT_4,e_SEVC_ERR);
  }
#endif /* #if( k_UNICAST_CPBL == TRUE ) */
  /* set the PTP subdomain */  
  NIF_Set_PTP_subdomain(CTL_s_ClkDS.s_DefDs.b_domn);
  /* starts dis task */
  SIS_EventSet(DIS_TSK,k_EV_INIT);
  /* start state decision event timer of control task */
  SIS_TimerStart(CTL_TSK,GetMinAnncIntvTmo());          
  return;
}

/***********************************************************************
**  
** Function    : GetAnncRecptTmo
**  
** Description : Calculates the announce receipt timeout in SIS ticks
**               (absolute SIS time).
**  
** Parameters  : w_ifIdx (IN) - communication interface index
**               
** Returnvalue : SIS time of announce receipt timeout for that interface
** 
** Remarks     : function is reentrant
**  
***********************************************************************/
static UINT32 GetAnncRecptTmo( UINT16 w_ifIdx )
{
  UINT32 dw_anncRcptTmo;
  
  dw_anncRcptTmo = SIS_GetTime() + 
            ( PTP_getTimeIntv(CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv) *
               CTL_s_ClkDS.as_PortDs[w_ifIdx].b_anncRecptTmo );
  return dw_anncRcptTmo;
}

/***********************************************************************
**  
** Function    : GetMinAnncIntvTmo
**  
** Description : Gets the minimal announce interval of all communication
**               interfaces.
**               The returned time interval is scaled in SIS ticks.
**  
** Parameters  : -
**               
** Returnvalue : min. time interval 
** 
** Remarks     : function is reentrant
**  
***********************************************************************/
static UINT32 GetMinAnncIntvTmo( void )
{
  UINT16 w_ifIdx;
  /* preinitialize to max possible value */
  INT8   c_minAnncIntv = k_MAX_LOG_MSG_INTV; 

  /* get the minimal announce interval of all ports */
  for( w_ifIdx = 0 ; w_ifIdx < CTL_s_comIf.dw_amntInitComIf ; w_ifIdx++ )
  {
    if( CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv < c_minAnncIntv )
    {
      c_minAnncIntv = CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv;                       
    }
  }
  /* calculate SIS ticks out of it */
  return PTP_getTimeIntv(c_minAnncIntv);
}

/* just for unicast enabled devices */
#if( k_UNICAST_CPBL == TRUE )
/***********************************************************************
**
** Function    : InitUcDiscovery
**
** Description : Tries to read unicast master table and initialize 
**               unicast discovery.
**              
** Parameters  : -
**
** Returnvalue : TRUE  - initialization succeeded
**               FALSE 
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static BOOLEAN InitUcDiscovery( void )
{
  PTP_t_MboxMsg s_msg;
  BOOLEAN       o_ret = FALSE;
  INT8          c_minIntv = k_MAX_I8;
  UINT16        w_ifIdx;
  UINT32        dw_ent;
  /* allocate SIS memory for the complete table */
  for( dw_ent = 0 ; dw_ent < k_MAX_UC_MST_TBL_SZE ; dw_ent++ )
  {
    s_ucMstTbl.aps_pAddr[dw_ent] 
                        = (PTP_t_PortAddr*) SIS_Alloc(sizeof(PTP_t_PortAddr));
  }
  /* try to read unicast master table */
  if( FIO_ReadUcMstTbl(&s_ucMstTbl) == FALSE )
  {
    /* try to write new table */
    if( FIO_WriteUcMstTbl(&s_ucMstTbl) == FALSE )
    {
      /* error */
      PTP_SetError(k_CTL_ERR_ID,CTL_k_WR_UCTBL,e_SEVC_NOTC);      
    }
  }
  /* free unused memory */
  for(dw_ent = s_ucMstTbl.w_actTblSze; dw_ent<k_MAX_UC_MST_TBL_SZE ; dw_ent++ )
  {
    if( s_ucMstTbl.aps_pAddr[dw_ent] != NULL )
    {
      SIS_Free( s_ucMstTbl.aps_pAddr[dw_ent] );
      s_ucMstTbl.aps_pAddr[dw_ent] = NULL;
    }
  }
  if( s_ucMstTbl.w_actTblSze > 0 )
  {
    /* get minimal announce interval of all interfaces */
    for( w_ifIdx = 0 ; w_ifIdx < CTL_s_comIf.dw_amntInitComIf ; w_ifIdx++ )
    {
      if( CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv < c_minIntv )
      {
        c_minIntv = CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv;
      }
    }
    /* send access of unicast master table to unicast discovery task */
    s_msg.e_mType             = e_IMSG_UC_MST_TBL;
    s_msg.s_pntData.ps_pAQTbl = &s_ucMstTbl;
    s_msg.s_etc1.c_s8         = c_minIntv;
    o_ret = SIS_MboxPut(UCD_TSK,&s_msg);
  }
  else
  {
    /* return TRUE */
    o_ret = TRUE;
  }
  return o_ret;
}
#endif /* #if( k_UNICAST_CPBL == TRUE ) */

/***********************************************************************
**  
** Function    : Synchronize
**  
** Description : Does the synchronization of the offset. This is 
**               a syncronization of the 'time of day'.
**  
** Parameters  : ps_offs     (IN) - pointer to offset to compensate
**               dw_TaUsec   (IN) - actual sync interval in microseconds
**               o_reset     (IN) - resets the filters, if needed.           
**               
** Returnvalue : Picosecond per second to adjust
** 
** Remarks     : function is not reentrant
**  
***********************************************************************/
static INT64 Synchronize( const PTP_t_TmIntv *ps_offs,
                                UINT32       dw_TaUsec,
                                BOOLEAN      o_reset)
{
  INT64 ll_ret;
#if 1
  //do nothing here KJH  
  ll_ret = 0;
#elif( k_SYNC_MODE == k_SYNC_TM_OF_DAY )
  ll_ret = SyncTimeOfDay(ps_offs,dw_TaUsec,o_reset);
#elif( k_SYNC_MODE == k_SYNC_FREQ )
  ll_ret = SyncFreq(ps_offs,dw_TaUsec,o_reset);
#else
  #error "No frequency mode specified"
#endif
  return ll_ret;
}

#if( k_SYNC_MODE == k_SYNC_TM_OF_DAY )
#if 0
/***********************************************************************
**  
** Function    : SyncTimeOfDay
**  
** Description : Does the synchronization of the offset. This is 
**               a syncronization of the 'time of day'.
**      
** Parameters  : ps_offs     (IN) - pointer to offset to compensate
**               dw_TaUsec   (IN) - actual sync interval in microseconds
**               o_reset     (IN) - resets the filters, if needed.             
**               
** Returnvalue : Picosecond per second to adjust
**      
** Remarks     : function is not reentrant
**     
***********************************************************************/
static INT64 SyncTimeOfDay( const PTP_t_TmIntv *ps_offs,
                                  UINT32       dw_TaUsec,
                                  BOOLEAN      o_reset )
{      
  INT64 ll_corr;
  INT64 ll_psecPerSec;
         
  /* reset filters ? */
  if( o_reset == TRUE )
  {    
    /* reset PI controller */
    if( CTL_s_ClkDS.o_drift_use == TRUE )
    {
      /* reset PI controller with stored drift value */
      PTP_PIctr((INT64)0,TRUE,0,CTL_s_ClkDS.ll_drift_ppb);/*lint !e534*/
    }
    else
    {
      /* reset PI controller with 0 */
      PTP_PIctr((INT64)0,TRUE,0,(INT64)0);/*lint !e534*/
    }
    CTL_ResetClockDrift();
    /* return drift = 0 */
    return 0;
  }    
  /* get picoseconds offset */
  ll_corr = PTP_INTV_TO_PSEC(ps_offs->ll_scld_Nsec);
  /* call PI controller */
  ll_psecPerSec = PTP_PIctr(ll_corr,FALSE,dw_TaUsec,(INT64)0);
  /* call clock drift adjustment function */ 
  CIF_AdjClkDrift(-ll_psecPerSec); /*lint !e534 */
  return ll_psecPerSec;
}
#endif // #if 0
#endif

#if( k_SYNC_MODE == k_SYNC_FREQ )
/***********************************************************************
**  
** Function    : SyncFreq
**  
** Description : Does the frequency synchronization. This is done by 
**               maintaining the offset at startup.
**  
** Parameters  : ps_offs     (IN) - pointer to offset to compensate
**               dw_TaUsec   (IN) - actual sync interval in microseconds
**               o_reset     (IN) - resets the filters, if needed.            
**               
** Returnvalue : Picosecond per second to adjust
** 
** Remarks     : function is not reentrant
**  
***********************************************************************/
static INT64 SyncFreq( const PTP_t_TmIntv *ps_offs,
                             UINT32       dw_TaUsec,
                             BOOLEAN      o_reset)
{
  INT64 ll_corr;  
  INT64 ll_psecPerSec;
  static UINT8 b_strtCnt = 0;
  static PTP_t_TmIntv s_tiOffs = {0};
     
  /* reset filters ? */
  if( o_reset == TRUE )
  {
    /* reset PI controller */
    if( CTL_s_ClkDS.o_drift_use == TRUE )
    {
      /* reset PI controller with stored drift value */
      PTP_PIctr(0LL,TRUE,0,CTL_s_ClkDS.ll_drift_ppb);
    }
    else
    {
      /* reset PI controller with 0 */
      PTP_PIctr(0LL,TRUE,0,0);
    }
    CTL_ResetClockDrift();
    b_strtCnt = 0;
    s_tiOffs.ll_scld_Nsec = 0LL;
    /* return drift = 0 */
    return 0;
  }  
  if( b_strtCnt < 3 )
  {
    s_tiOffs.ll_scld_Nsec = ps_offs->ll_scld_Nsec;
    b_strtCnt++;
    return 0LL;
  }
  else
  {
    /* get picoseconds offset */
    ll_corr = PTP_INTV_TO_PSEC((ps_offs->ll_scld_Nsec - s_tiOffs.ll_scld_Nsec));
    /* call PI controller */
    ll_psecPerSec = PTP_PIctr(ll_corr,FALSE,dw_TaUsec,0);
    /* call clock drift adjustment function */ 
    CIF_AdjClkDrift(-ll_psecPerSec); /*lint !e534 */
    return ll_psecPerSec;
  }
}
#endif
/*************************************************************************
**
** Function    : GoToFaultyState
**
** Description : Changes the state of the interface with index w_ifIdx to
**               FAULTY
**              
** Parameters  : w_ifIdx (IN) - communication interface index 
**                              to be set in FAULTY state
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
************************************************************************/
static void GoToFaultyState( UINT16 w_ifIdx)
{
  /* just switch to faulty state, if this channel is not in 
     disabled state */
  if( CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts != e_STS_DISABLED )
  {
    /* if this is the slave interface - set slave task to faulty */
    if( CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts == e_STS_SLAVE )
    {
      /* set event state change faulty */
      SIS_EventSet(SLS_TSK,k_EV_SC_FLT);
#if( k_CLK_DEL_E2E == TRUE )
      SIS_EventSet(SLD_TSK,k_EV_SC_FLT);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
    }
    SIS_EventSet(CTL_aw_Hdl_maa[w_ifIdx],k_EV_SC_FLT);
    SIS_EventSet(CTL_aw_Hdl_mad[w_ifIdx],k_EV_SC_FLT);
    SIS_EventSet(CTL_aw_Hdl_mas[w_ifIdx],k_EV_SC_FLT);
#if( k_CLK_DEL_P2P == TRUE )
    SIS_EventSet(CTL_aw_Hdl_p2p_del[w_ifIdx],k_EV_SC_FLT);
#endif
    /* set new port state in port configuration set */
    CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts = e_STS_FAULTY;
  }
}
#endif /* #if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE )) */

/***********************************************************************
**  
** Function    : HandleSendError
**  
** Description : Stores and increments send errors. If t
**  
** Parameters  : w_ifIdx (IN) - interface index 
**               o_reset (IN) - reset flag
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void HandleSendError( UINT16 w_ifIdx , BOOLEAN o_reset)
{
  static UINT32 adw_sendErrCnt[k_NUM_IF] = {0}; 
  PTP_t_MboxMsg s_msg;
  if( o_reset == TRUE )
  {
    if( w_ifIdx == k_IFIDX_ALL )
    {
      /* reset all interfaces */
      for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
      {
        adw_sendErrCnt[w_ifIdx] = 0;
      }
    }
    else
    {
      /* reset just one interface */
      adw_sendErrCnt[w_ifIdx] = 0;
    }
  }
  else
  {
    adw_sendErrCnt[w_ifIdx]++;
    /* check error threshold */
    if( adw_sendErrCnt[w_ifIdx] > k_SENDERR_THRES )
    {
#if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE ))
      /* change to faulty state */
      GoToFaultyState(w_ifIdx);      
#endif /* #if( (k_CLK_IS_OC == TRUE ) || (k_CLK_IS_BC == TRUE )) */
      /* deinitialize network interface 
          (reinitializes automatically) */
      s_msg.e_mType      = e_IMSG_NETDEINIT;
      s_msg.s_etc1.w_u16 = w_ifIdx;
      if( SIS_MboxPut(DISnet_TSK,&s_msg) == TRUE )
      {
        /* reset errors */
        adw_sendErrCnt[w_ifIdx] = 0;
      }
    }
  }
}

#if( k_CLK_DEL_P2P == TRUE )
/***********************************************************************
**  
** Function    : ClearPathDelay
**  
** Description : Clears the values of the mean path delay variables
**               in the port data set and the TC port data set
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void ClearPathDelay( void )
{
  UINT16 w_ifIdx;
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
    CTL_s_ClkDS.as_PortDs[w_ifIdx].s_tiP2PMPDel.ll_scld_Nsec = (INT64)0;
    CTL_s_ClkDS.as_PortDs[w_ifIdx].s_tiP2PMPDelUf.ll_scld_Nsec = (INT64)0;
#endif /* #if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
#if( k_CLK_IS_TC == TRUE)
    CTL_s_ClkDS.as_TcPortDs[w_ifIdx].s_peerMPDel.ll_scld_Nsec   = (INT64)0;
    CTL_s_ClkDS.as_TcPortDs[w_ifIdx].s_peerMPDelUf.ll_scld_Nsec   = (INT64)0;
#endif /* #if( k_CLK_IS_TC == TRUE) */
  }
}
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

/***********************************************************************
**  
** Function    : HandleMntMsg
**  
** Description : Handles all received internal messages received from
**               the MNT Task. These messages contain set-commands to the
**               Clock data set.
**  
** Parameters  : pps_msg      (IN/OUT) - internal message from CTL_Task
**                                       message box.
**               b_state          (IN) - actual state of CTL Task
**               pdw_anncRcptTmo (OUT) - announce receipt timeout array for
**                                       all communication interfaces
**               w_actSlvPortIdx  (IN) - actual slave port
**               
** Returnvalue : new state of the control task
**
** Remarks     : -
**  
***********************************************************************/
static UINT8 HandleMntMsg( PTP_t_MboxMsg **pps_msg,
                           UINT8         b_state,
                           UINT32        *pdw_anncRcptTmo,
                           UINT16        w_actSlvPortIdx)
{
  NIF_t_PTPV2_MntMsg  *ps_mntMsg;
  UINT16              w_ifIdx;
  UINT16              w_tmp;
  INT8 c_pDelIntv;
/* variables not used for TC-only implementations */
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
  PTP_t_TmPropDs      *ps_tmPropDs;
  PTP_t_TmStmp        s_tsNewTime;
#endif /* #if((k_CLK_IS_OC == FALSE) && (k_CLK_IS_BC == FALSE)) */
#if( k_UNICAST_CPBL == TRUE )
  UINT16              w_tlvIdx; 
  PTP_t_MboxMsg       s_msg;
#endif /* #if( k_UNICAST_CPBL == TRUE ) */

  /* get MNT message */
  ps_mntMsg = (*pps_msg)->s_pntData.ps_mnt;
  /* set by default to a successful response */
  (*pps_msg)->e_mType = e_IMSG_MM_RES;  
  /* process message */
  switch( ps_mntMsg->s_mntTlv.w_mntId )
  {
    /* applicable to all node types */
    /* case k_NULL_MANAGEMENT: 
        -> handling is done in MNT */
    /* case k_CLOCK_DESCRIPTION
        -> handling is done in MNT */
    case k_USER_DESCRIPTION:
    {
      /* delete old description */
      PTP_MEMSET(CTL_s_ClkDS.ac_userDesc,'\0',k_MAX_USRDESC_SZE);
      /* get new size of text */
      w_tmp = ps_mntMsg->s_mntTlv.pb_data[k_OFFS_UD_SIZE];
      /* copy new text */
      PTP_BCOPY(CTL_s_ClkDS.ac_userDesc, 
                &ps_mntMsg->s_mntTlv.pb_data[k_OFFS_UD_TEXT],          
                w_tmp); 
      break;
    }
    /* case k_SAVE_IN_NON_VOLATILE_STORAGE:
        -> handling is done in MNT */
    /* case k_RESET_NON_VOLATILE_STORAGE:
        -> handling is done in MNT */
    /* case k_INITIALIZE:
        -> handling is done in MNT and other CTL functions */
    /* case k_FAULT_LOG:
        -> not yet implemented -> is done in MNT */
    /* case k_FAULT_LOG_RESET:
        -> not yet implemented -> is done in MNT */

    /* applicable to ordinary and boundary clocks */
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
    /* case k_DEFAULT_DATA_SET:
        -> GET only, handling is done in MNT */
    /* case k_CURRENT_DATA_SET:
        -> GET only, handling is done in MNT */
    /* case k_PARENT_DATA_SET:
        -> GET only, handling is done in MNT */
    /* case k_TIME_PROPERTIES_DATA_SET:
        -> GET only, handling is done in MNT */
    /* case k_PORT_DATA_SET:
        -> GET only, handling is done in MNT */
    case k_PRIORITY1:
    {
      /* It is used within BMC. Each update is reflected
          after the next call of the BMC */
      CTL_s_ClkDS.s_DefDs.b_prio1 = ps_mntMsg->s_mntTlv.pb_data[0];
      if( CTL_s_ClkDS.o_IsGM == TRUE )
      {
        /* change PDS to new value if this node is GM */
        CTL_s_ClkDS.s_PrntDs.b_gmPrio1 = CTL_s_ClkDS.s_DefDs.b_prio1;
                   
      }
      break; 
    }
    case k_PRIORITY2:
    {
      /* It is used within BMC. Each update is reflected
          after the next call of the BMC */
      CTL_s_ClkDS.s_DefDs.b_prio2 = ps_mntMsg->s_mntTlv.pb_data[0];
      if( CTL_s_ClkDS.o_IsGM == TRUE )
      {
        /* change PDS to new value if this node is GM */
        CTL_s_ClkDS.s_PrntDs.b_gmPrio2 = CTL_s_ClkDS.s_DefDs.b_prio2; 
      }
      break;
    }
    case k_DOMAIN:
    {
#ifdef MNT_API
      /* lock MNT API */
      MNT_LockApi();
#endif /* #ifdef MNT_API */
      /* change domain in default data set */
      CTL_s_ClkDS.s_DefDs.b_domn = ps_mntMsg->s_mntTlv.pb_data[0];
      /* send RESPONSE and clear API */
      MNT_PreInitClear( (*pps_msg), ps_mntMsg );
      /* change domain and reinit all regarding units */
      ChangeDomain(CTL_s_ClkDS.s_DefDs.b_domn);
      /* change to preoperational status */
      b_state = CTL_k_PRE_OPERATIONAL;
      /* do not send a MNT message back to MNT */
      (*pps_msg) = NULL;
#ifdef MNT_API
      /* unlock MNT API */
      MNT_UnlockApi();
#endif /* #ifdef MNT_API */
      break;
    }
    /* case k_SLAVE_ONLY:
        -> not yet implemented -> handling is done in MNT */
    case k_LOG_MEAN_ANNOUNCE_INTERVAL:
    {
      /* if addressed to all ports */
      if(ps_mntMsg->s_trgtPid.w_portNmb == 0xFFFF)
      {
        /* get interface */
        for(w_ifIdx = 0; w_ifIdx < k_NUM_IF; w_ifIdx++)
        {
          if( (INT8)ps_mntMsg->s_mntTlv.pb_data[0] <
              CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv )
          {
            UINT16 w_rcrd;
            for( w_rcrd = 0 ; w_rcrd < k_CLK_NMB_FRGN_RCD ; w_rcrd++ )
              CTL_w_anncTmoExt[w_ifIdx][w_rcrd] += 32;
          }
          /* set new announce interval */
          CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv = 
                                (INT8)ps_mntMsg->s_mntTlv.pb_data[0];
          MST_SetAnncInt(w_ifIdx,CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv);
          UCD_SetAnncInt(CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv);
          /* change announce receipt timeout */
          pdw_anncRcptTmo[w_ifIdx] = GetAnncRecptTmo( w_ifIdx );
        }
      }
      /* if addressed to only one port */
      else
      {
        /* get interface */
        w_ifIdx = ps_mntMsg->s_trgtPid.w_portNmb - 1;
        /* set new announce interval */
        if( (INT8)ps_mntMsg->s_mntTlv.pb_data[0] <
            CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv )
        {
          UINT16 w_rcrd;
          for( w_rcrd = 0 ; w_rcrd < k_CLK_NMB_FRGN_RCD ; w_rcrd++ )
            CTL_w_anncTmoExt[w_ifIdx][w_rcrd] += 32;
        }
        CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv =  
                                   (INT8)ps_mntMsg->s_mntTlv.pb_data[0];
        MST_SetAnncInt(w_ifIdx,CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv);
        UCD_SetAnncInt(CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv);
        /* change announce receipt timeout */
        pdw_anncRcptTmo[w_ifIdx] = GetAnncRecptTmo( w_ifIdx );
      }
      break;
    }
    case k_ANNOUNCE_RECEIPT_TIMEOUT:
    {
      /* if addressed to all ports */
      if(ps_mntMsg->s_trgtPid.w_portNmb == 0xFFFF)
      {
        /* get interface */
        for(w_ifIdx = 0; w_ifIdx < k_NUM_IF; w_ifIdx++)
        {
          /* set new announce interval */
          CTL_s_ClkDS.as_PortDs[w_ifIdx].b_anncRecptTmo =  
                                              ps_mntMsg->s_mntTlv.pb_data[0];
          /* change announce receipt timeout */
          pdw_anncRcptTmo[w_ifIdx] = GetAnncRecptTmo( w_ifIdx );
        }
      }
      /* if addressed to only one port */
      else
      {
        /* get interface */
        w_ifIdx = ps_mntMsg->s_trgtPid.w_portNmb - 1;
        /* set new announce interval */
        CTL_s_ClkDS.as_PortDs[w_ifIdx].b_anncRecptTmo = 
                                             ps_mntMsg->s_mntTlv.pb_data[0];
        /* change announce receipt timeout */
        pdw_anncRcptTmo[w_ifIdx] = GetAnncRecptTmo( w_ifIdx );
      }
      break;
    }
    case k_LOG_MEAN_SYNC_INTERVAL:
    {
      /* if addressed to all ports */
      if(ps_mntMsg->s_trgtPid.w_portNmb == 0xFFFF)
      {
        /* get interface */
        for(w_ifIdx = 0; w_ifIdx < k_NUM_IF; w_ifIdx++)
        {
          /* set new announce interval */
          CTL_s_ClkDS.as_PortDs[w_ifIdx].c_SynIntv = 
                                   (INT8)ps_mntMsg->s_mntTlv.pb_data[0];
          SLV_SetSyncInt(w_ifIdx,CTL_s_ClkDS.as_PortDs[w_ifIdx].c_SynIntv);
          MST_SetSyncInt(w_ifIdx,CTL_s_ClkDS.as_PortDs[w_ifIdx].c_SynIntv);
          UCD_SetSyncInt(CTL_s_ClkDS.as_PortDs[w_ifIdx].c_SynIntv);
        }
      }
      /* if addressed to only one port */
      else
      {
        /* get interface */
        w_ifIdx = ps_mntMsg->s_trgtPid.w_portNmb - 1;
        /* set new announce interval */
        CTL_s_ClkDS.as_PortDs[w_ifIdx].c_SynIntv = 
                                   (INT8)ps_mntMsg->s_mntTlv.pb_data[0];
        SLV_SetSyncInt(w_ifIdx,CTL_s_ClkDS.as_PortDs[w_ifIdx].c_SynIntv);
        MST_SetSyncInt(w_ifIdx,CTL_s_ClkDS.as_PortDs[w_ifIdx].c_SynIntv);
        UCD_SetSyncInt(CTL_s_ClkDS.as_PortDs[w_ifIdx].c_SynIntv);
      }
      break;
    }
    /* case k_VERSION_NUMBER:
        -> not yet implemented -> handling is done in MNT */
    case k_ENABLE_PORT:
    {
      /* if all ports must be enabled */
      if(ps_mntMsg->s_trgtPid.w_portNmb == 0xFFFF)
      {
        /* set event to re-init slave tasks */
        SIS_EventSet(SLS_TSK,k_EV_INIT);
#if( k_CLK_DEL_E2E == TRUE )
        SIS_EventSet(SLD_TSK,k_EV_INIT);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
        /* go through all interfaces */
        for(w_ifIdx = 0; w_ifIdx < k_NUM_IF; w_ifIdx++)
        {
          /* only, if port is DISABLED */
          if(CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts == e_STS_DISABLED)
          {
            /* only inizialized interfaces should be activated */
            if(w_ifIdx < CTL_s_comIf.dw_amntInitComIf)
            {
              /* recalculate announce receipt timeout */
              pdw_anncRcptTmo[w_ifIdx] = GetAnncRecptTmo( w_ifIdx );
              /* change state in port data set */
              CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts = e_STS_LISTENING;
              /* set event to re-init master tasks */
              SIS_EventSet(CTL_aw_Hdl_maa[w_ifIdx],k_EV_INIT);
              SIS_EventSet(CTL_aw_Hdl_mas[w_ifIdx],k_EV_INIT);
              SIS_EventSet(CTL_aw_Hdl_mad[w_ifIdx],k_EV_INIT);  
#if( k_CLK_DEL_P2P == TRUE )
              SIS_EventSet(CTL_aw_Hdl_p2p_del[w_ifIdx],k_EV_INIT);
#endif
            }
            /* other interfaces are faulty */
            else
            {
              /* set internal mbox type to management error response */ 
              (*pps_msg)->e_mType   = e_IMSG_MM_ERRRES; 
              /* allocate buffer for display an error text */
              (*pps_msg)->s_pntExt1.pv_data = SIS_Alloc(sizeof(PTP_t_Text));
              if((*pps_msg)->s_pntExt1.pv_data != NULL)
              {
                MNT_GetPTPText("Port is not initialized",
                               (*pps_msg)->s_pntExt1.ps_text);
              }
            }
          }
          /* post is not disabled */
          else
          {
            /* set internal mbox type to management error response */ 
            (*pps_msg)->e_mType   = e_IMSG_MM_ERRRES; 
            /* allocate buffer for display an error text */
            (*pps_msg)->s_pntExt1.pv_data = SIS_Alloc(sizeof(PTP_t_Text));
            if((*pps_msg)->s_pntExt1.pv_data != NULL)
            {
              MNT_GetPTPText("Port is not disabled",
                             (*pps_msg)->s_pntExt1.ps_text);
            }
          }
        }  
      }
      /* if only one port must be enabled */
      else
      {
        /* get interface index */
        w_ifIdx = ps_mntMsg->s_trgtPid.w_portNmb - 1;
        /* only, if port is DISABLED */
        if(CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts == e_STS_DISABLED)
        {
          /* only inizialized interfaces should be activated */
          if(w_ifIdx < CTL_s_comIf.dw_amntInitComIf)
          {
            /* recalculate announce receipt timeout */
            pdw_anncRcptTmo[w_ifIdx] = GetAnncRecptTmo( w_ifIdx );
            /* change state in port data set */
            CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts = e_STS_LISTENING;
            /* set event to re-init master tasks */
            SIS_EventSet(CTL_aw_Hdl_maa[w_ifIdx],k_EV_INIT);
            SIS_EventSet(CTL_aw_Hdl_mas[w_ifIdx],k_EV_INIT);
            SIS_EventSet(CTL_aw_Hdl_mad[w_ifIdx],k_EV_INIT);   
#if( k_CLK_DEL_P2P == TRUE )
            SIS_EventSet(CTL_aw_Hdl_p2p_del[w_ifIdx],k_EV_INIT);
#endif
          }
          /* other interfaces are faulty */
          else
          {
            /* set internal mbox type to management error response */ 
            (*pps_msg)->e_mType   = e_IMSG_MM_ERRRES; 
            /* allocate buffer for display an error text */
            (*pps_msg)->s_pntExt1.pv_data = SIS_Alloc(sizeof(PTP_t_Text));
            if((*pps_msg)->s_pntExt1.pv_data != NULL)
            {
              MNT_GetPTPText("Port is not initialized",
                             (*pps_msg)->s_pntExt1.ps_text);
            }
          }
        }
        /* post is not disabled */
        else
        {
          /* set internal mbox type to management error response */ 
          (*pps_msg)->e_mType   = e_IMSG_MM_ERRRES; 
          /* allocate buffer for display an error text */
          (*pps_msg)->s_pntExt1.pv_data = SIS_Alloc(sizeof(PTP_t_Text));
          if((*pps_msg)->s_pntExt1.pv_data != NULL)
          {
            MNT_GetPTPText("Port is not disabled",
                           (*pps_msg)->s_pntExt1.ps_text);
          }
        }
      }
      break;
    }
    case k_DISABLE_PORT:
    {
      /* if all ports must be disabled */
      if(ps_mntMsg->s_trgtPid.w_portNmb == 0xFFFF)
      {
        /* set event to stop slave tasks */
        SIS_EventSet(SLS_TSK,k_EV_SC_DSBL);
#if( k_CLK_DEL_E2E == TRUE )
        SIS_EventSet(SLD_TSK,k_EV_SC_DSBL);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */                  
        for(w_ifIdx = 0; w_ifIdx < k_NUM_IF; w_ifIdx++)
        {
          /* this implementation does not allow the state-change
             to disabled out of the faulty state */
          if( CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts != e_STS_FAULTY )
          {
            /* change state in port data set */
            CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts = e_STS_DISABLED;
            /* set event to stop master tasks */
            SIS_EventSet(CTL_aw_Hdl_maa[w_ifIdx],k_EV_SC_DSBL);
            SIS_EventSet(CTL_aw_Hdl_mas[w_ifIdx],k_EV_SC_DSBL);
            SIS_EventSet(CTL_aw_Hdl_mad[w_ifIdx],k_EV_SC_DSBL); 
  #if( k_CLK_DEL_P2P == TRUE )
            SIS_EventSet(CTL_aw_Hdl_p2p_del[w_ifIdx],k_EV_SC_DSBL);
  #endif
  #if( k_UNICAST_CPBL == TRUE )
            /* cancel unicast connections */
            s_msg.s_etc1.w_u16 = w_ifIdx;
            s_msg.e_mType      = e_IMSG_UC_CNCL_IF;
            if( SIS_MboxPut(UCMann_TSK,&s_msg) == FALSE )
            {
              PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_UCMBOX,e_SEVC_NOTC);
            }
            if( SIS_MboxPut(UCMsyn_TSK,&s_msg) == FALSE )
            {
              PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_UCMBOX,e_SEVC_NOTC);
            }          
#endif /* #if( k_UNICAST_CPBL == TRUE ) */
          }
          else
          {
            /* set internal mbox type to management error response */ 
            (*pps_msg)->e_mType   = e_IMSG_MM_ERRRES; 
            /* allocate buffer for display an error text */
            (*pps_msg)->s_pntExt1.pv_data = SIS_Alloc(sizeof(PTP_t_Text));
            if((*pps_msg)->s_pntExt1.pv_data != NULL)
            {
              MNT_GetPTPText("Error disabling one or more ports",
                             (*pps_msg)->s_pntExt1.ps_text);
            }
          }
        }        
      }
      /* if only one port must be disabled */
      else
      {
        /* get interface index */
        w_ifIdx = ps_mntMsg->s_trgtPid.w_portNmb - 1;
        /* check, if requested port is in slave state */
        if( w_actSlvPortIdx == w_ifIdx )
        {
          /* set event to stop slave tasks */
          SIS_EventSet(SLS_TSK,k_EV_SC_DSBL);
#if( k_CLK_DEL_E2E == TRUE )
          SIS_EventSet(SLD_TSK,k_EV_SC_DSBL);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
        }
        /* this implementation does not allow the state-change
           to disabled out of the faulty state */
        if( CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts != e_STS_FAULTY )
        {
          /* change state in port data set */
          CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts = e_STS_DISABLED;
          /* set event to stop master tasks */
          SIS_EventSet(CTL_aw_Hdl_maa[w_ifIdx],k_EV_SC_DSBL);
          SIS_EventSet(CTL_aw_Hdl_mas[w_ifIdx],k_EV_SC_DSBL);
          SIS_EventSet(CTL_aw_Hdl_mad[w_ifIdx],k_EV_SC_DSBL); 
#if( k_CLK_DEL_P2P == TRUE )
          SIS_EventSet(CTL_aw_Hdl_p2p_del[w_ifIdx],k_EV_SC_DSBL);
#endif
#if( k_UNICAST_CPBL == TRUE )
          /* cancel unicast connections */
          s_msg.s_etc1.w_u16 = w_ifIdx;
          s_msg.e_mType = e_IMSG_UC_CNCL_IF;
          if( SIS_MboxPut(UCMann_TSK,&s_msg) == FALSE )
          {
            PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_UCMBOX,e_SEVC_NOTC);
          }
          if( SIS_MboxPut(UCMsyn_TSK,&s_msg) == FALSE )
          {
            PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_UCMBOX,e_SEVC_NOTC);
          }
#endif /* #if( k_UNICAST_CPBL == TRUE ) */
        }
        else
        {
          /* set internal mbox type to management error response */ 
          (*pps_msg)->e_mType   = e_IMSG_MM_ERRRES; 
          /* allocate buffer for display an error text */
          (*pps_msg)->s_pntExt1.pv_data = SIS_Alloc(sizeof(PTP_t_Text));
          if((*pps_msg)->s_pntExt1.pv_data != NULL)
          {
            MNT_GetPTPText("Error enabling one or more ports",
                           (*pps_msg)->s_pntExt1.ps_text);
          }
        }
      }
      break;
    }
    case k_TIME:
    {
      /* get value of seconds */
      s_tsNewTime.u48_sec = GOE_ntoh48((UINT8*)&ps_mntMsg->s_mntTlv.pb_data[0]);
      /* get value of nanoseconds */
      s_tsNewTime.dw_Nsec = GOE_ntoh32((UINT8*)&ps_mntMsg->s_mntTlv.pb_data[6]);
      /* set clock - warning: step offset to the current time possible */
      if( CIF_SetSysTime(&s_tsNewTime) == FALSE )
      {
        /* set internal mbox type to management error response */ 
        (*pps_msg)->e_mType   = e_IMSG_MM_ERRRES; 
        /* allocate buffer for display an error text */
        (*pps_msg)->s_pntExt1.pv_data = SIS_Alloc(sizeof(PTP_t_Text));
        if((*pps_msg)->s_pntExt1.pv_data != NULL)
        {
          MNT_GetPTPText("Error setting system time",
                         (*pps_msg)->s_pntExt1.ps_text);
        }
      }
      break;
    }
    case k_CLOCK_ACCURACY:
    {
      /* copy accuracy */
      CTL_s_ClkDS.s_DefDs.s_clkQual.e_clkAccur =  
                              (PTP_t_clkAccEnum)ps_mntMsg->s_mntTlv.pb_data[0];
      if( CTL_s_ClkDS.o_IsGM == TRUE )
      {
        /* change PDS to new value if this node is GM */
        CTL_s_ClkDS.s_PrntDs.s_gmClkQual.e_clkAccur = 
                              (PTP_t_clkAccEnum)ps_mntMsg->s_mntTlv.pb_data[0]; 
      }
      break;
    }
    case k_UTC_PROPERTIES:
    {
      /* set pointer to appropriate data set to write */
      if( CTL_ps_extTmPropDs != NULL )
      {
        ps_tmPropDs = CTL_ps_extTmPropDs;
      }
      else
      {
        ps_tmPropDs = &CTL_s_ClkDS.s_TmPrptDs;
      }
      /* information data only -> copy direct into Ds */
      /* copy current UTC offset */
      ps_tmPropDs->i_curUtcOffs = 
        (INT8)GOE_ntoh16(&ps_mntMsg->s_mntTlv.pb_data[0]);
      /* copy LI-61 flag */
      ps_tmPropDs->o_leap_61 =
        GET_FLAG((UINT16)ps_mntMsg->s_mntTlv.pb_data[2], k_FLG_LI_61);
      /* copy LI-59 flag */
      ps_tmPropDs->o_leap_59 =
        GET_FLAG((UINT16)ps_mntMsg->s_mntTlv.pb_data[2], k_FLG_LI_59);
      /* copy UTC valid flag */
      ps_tmPropDs->o_curUtcOffsVal =
        GET_FLAG((UINT16)ps_mntMsg->s_mntTlv.pb_data[2], k_FLG_CUR_UTCOFFS_VAL);
      break;
    }
    case k_TRACEABILITY_PROPERTIES:
    {
      /* set pointer to appropriate data set to write */
      if( CTL_ps_extTmPropDs != NULL )
      {
        ps_tmPropDs = CTL_ps_extTmPropDs;
      }
      else
      {
        ps_tmPropDs = &CTL_s_ClkDS.s_TmPrptDs;
      }
      /* information data only -> copy direct into Ds */
      /* copy time traceable (TTRA) flag */
      ps_tmPropDs->o_tmTrcable =
        GET_FLAG((UINT16)ps_mntMsg->s_mntTlv.pb_data[0], k_FLG_TIME_TRACEABLE);
      /* copy frequency traceable (FTRA) flag */
      ps_tmPropDs->o_frqTrcable =
        GET_FLAG((UINT16)ps_mntMsg->s_mntTlv.pb_data[0], k_FLG_FREQ_TRACEABLE);
      break;
    }
    case k_TIMESCALE_PROPERTIES:
    {
      /* set pointer to appropriate data set to write */
      if( CTL_ps_extTmPropDs != NULL )
      {
        ps_tmPropDs = CTL_ps_extTmPropDs;
      }
      else
      {
        ps_tmPropDs = &CTL_s_ClkDS.s_TmPrptDs;
      }
      /* information data only -> copy direct into Ds */
      /* copy PTP timescale flag */
      ps_tmPropDs->o_ptpTmScale =
        GET_FLAG((UINT16)ps_mntMsg->s_mntTlv.pb_data[0], k_FLG_PTP_TIMESCALE);
      /* copy time source */
      ps_tmPropDs->e_tmSrc = (PTP_t_tmSrcEnum)ps_mntMsg->s_mntTlv.pb_data[1];
      break;
    }
    /* case k_UNICAST_NEGOTIATION_ENABLE:
        -> not yet implemented -> handling is done in MNT */
    /* case k_PATH_TRACE_LIST:
        -> GET only, handling is done in MNT */
    /* case k_PATH_TRACE_ENABLE:
        -> not yet implemented -> handling is done in MNT */
    /* case k_GRANDMASTER_CLUSTER_TABLE:
        -> not yet implemented -> handling is done in MNT */
#if( k_UNICAST_CPBL == TRUE )
    case k_UNICAST_MASTER_TABLE:
    {
      /* first delete all registered masters */
      for( w_tmp=0 ; w_tmp < k_MAX_UC_MST_TBL_SZE ; w_tmp++)
      {
        if(s_ucMstTbl.aps_pAddr[w_tmp] != NULL)
        {
          SIS_Free(s_ucMstTbl.aps_pAddr[w_tmp]);
          s_ucMstTbl.aps_pAddr[w_tmp] = NULL;
        }
      }
      /* set new table size */
      s_ucMstTbl.w_actTblSze = 
        GOE_ntoh16(&ps_mntMsg->s_mntTlv.pb_data[k_OFFS_UCMATBL_SIZE]);
      /* set new query interval */
      s_ucMstTbl.c_logQryIntv = 
        (INT8)ps_mntMsg->s_mntTlv.pb_data[k_OFFS_UCMATBL_LQI];

      /* for new table register all new masters */
      if(s_ucMstTbl.w_actTblSze != 0)
      {
        /* start of unicast master table - first element */
        w_tlvIdx = k_OFFS_UCMATBL_TBL; 
        /* get new table entries */
        for( w_tmp=0 ; w_tmp < s_ucMstTbl.w_actTblSze ; w_tmp++)
        {
          /* allocate new buffer */
          s_ucMstTbl.aps_pAddr[w_tmp] = 
            (PTP_t_PortAddr*)SIS_Alloc(sizeof(PTP_t_PortAddr));
          if( s_ucMstTbl.aps_pAddr[w_tmp] == NULL )
          {
            /* error occured -> break operation and send error */

            /* allocate buffer for display an error text */
            (*pps_msg)->s_pntExt1.pv_data = SIS_Alloc(sizeof(PTP_t_Text));
            if((*pps_msg)->s_pntExt1.pv_data != NULL)
            {
              MNT_GetPTPText("One or more table entries not saved",
                             (*pps_msg)->s_pntExt1.ps_text);
            }
            (*pps_msg)->e_mType = e_IMSG_MM_ERRRES;
            /* correct act table size */
            s_ucMstTbl.w_actTblSze = w_tmp;
            break;
          }
          else
          {
            /* copy network protocol */
            s_ucMstTbl.aps_pAddr[w_tmp]->e_netwProt = (PTP_t_nwProtEnum)
                            GOE_ntoh16(&ps_mntMsg->s_mntTlv.pb_data[w_tlvIdx]);
            /* copy address size */
            s_ucMstTbl.aps_pAddr[w_tmp]->w_AddrLen = 
              GOE_ntoh16(&ps_mntMsg->s_mntTlv.pb_data[w_tlvIdx+2]);
            /* copy port address into allocated buffer */
            PTP_BCOPY((UINT8*)s_ucMstTbl.aps_pAddr[w_tmp]->ab_Addr,
                      &ps_mntMsg->s_mntTlv.pb_data[w_tlvIdx+4],   
                      s_ucMstTbl.aps_pAddr[w_tmp]->w_AddrLen); 
            /* switch to next table entry in TLV */
            w_tlvIdx += (4+s_ucMstTbl.aps_pAddr[w_tmp]->w_AddrLen);
          }                    
        }
      }
      /* Zero new masters means deletion of the master table.
          This is already done! */

      /* get interface index */
      w_ifIdx = ps_mntMsg->s_trgtPid.w_portNmb - 1;
      /* clear accept flag */
      CTL_ClearAccpFlagFrgnMstDs(w_ifIdx);

      /* send new table to unicast discovery */
      s_msg.e_mType             = e_IMSG_UC_MST_TBL;
      s_msg.s_pntData.ps_pAQTbl = &s_ucMstTbl;
      s_msg.s_etc1.c_s8         = s_ucMstTbl.c_logQryIntv;
      /* send new master table to unicast discovery task */
      if(SIS_MboxPut(UCD_TSK,&s_msg) == FALSE)
      {
        /* send error response */
        (*pps_msg)->e_mType           = e_IMSG_MM_ERRRES;
        /* free allocated memory */
        if( (*pps_msg)->s_pntExt1.pv_data != NULL )
        {
          SIS_Free((*pps_msg)->s_pntExt1.pv_data);
        }
        (*pps_msg)->s_pntExt1.pv_data = NULL;  /* no display data */
      }
      break;
    }
#endif /* if( k_UNICAST_CPBL == TRUE ) */
    /* case k_UNICAST_MASTER_MAX_TABLE_SIZE:
        -> GET only, handling is done in MNT */
    /* case k_ACCEPTABLE_MASTER_TABLE:
        -> not yet implemented -> handling is done in MNT */
    /* case k_ACCEPTABLE_MASTER_TABLE_ENABLED:
        -> not yet implemented -> handling is done in MNT */
    /* case k_ACCEPTABLE_MASTER_MAX_TABLE_SIZE:
        -> GET only, handling is done in MNT */
    /* case k_ALTERNATE_MASTER:
        -> not yet implemented -> handling is done in MNT */
    /* case k_ALTERNATE_TIME_OFFSET_ENABLE:
        -> not yet implemented -> handling is done in MNT */
    /* case k_ALTERNATE_TIME_OFFSET_NAME:
        -> not yet implemented -> handling is done in MNT */
    /* case k_ALTERNATE_TIME_OFFSET_MAX_KEY:
        -> GET only, handling is done in MNT */
    /* case k_ALTERNATE_TIME_OFFSET_PROPERTIES:
        -> not yet implemented -> handling is done in MNT */
#endif /* #if OC or BC = TRUE */

    /* applicable to transparent clocks */
#if( k_CLK_IS_TC == TRUE )
    /* case k_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
        -> GET only, handling is done in MNT */
    /* case k_TRANSPARENT_CLOCK_PORT_DATA_SET:
        -> GET only, handling is done in MNT */
    /* case k_PRIMARY_DOMAIN:
        -> not yet implemented -> handling is done in MNT */
#endif /* if TC == TRUE ) */

    /* applicable to ordinary, boundary and transparent clocks */
    /* case k_DELAY_MECHANISM:
        -> not yet implemented -> handling is done in MNT */
    case k_LOG_MIN_MEAN_PDELAY_REQ_INTERVAL:
    {
      /* if addressed to all ports */
      if(ps_mntMsg->s_trgtPid.w_portNmb == 0xFFFF)
      {
        /* get and check range of desired pdelay interval */
        c_pDelIntv = (INT8)ps_mntMsg->s_mntTlv.pb_data[0];
        /* get interface */
        for(w_ifIdx = 0; w_ifIdx < k_NUM_IF; w_ifIdx++)
        {                    
#if( k_CLK_IS_TC == TRUE )
          /* set new min pdelay request interval */
          CTL_s_ClkDS.as_TcPortDs[w_ifIdx].c_logMPdelIntv = c_pDelIntv;            
#endif /* if TC == TRUE */
#if( (k_CLK_IS_OC == TRUE) || ( k_CLK_IS_BC == TRUE))
          /* set new min pdelay request interval */
          CTL_s_ClkDS.as_PortDs[w_ifIdx].c_PdelReqIntv = c_pDelIntv;
#endif /* #if( (k_CLK_IS_OC == TRUE) || ( k_CLK_IS_BC == TRUE)) */
#if( k_CLK_DEL_P2P == TRUE )
          /* if peer-to-peer mechanism is implemented */
          P2P_SetPDelReqInt(w_ifIdx,c_pDelIntv);
#endif /* if P2P == TRUE */
        }
      }
      /* if addressed to only one port */
      else
      {
        /* get interface */
        w_ifIdx = ps_mntMsg->s_trgtPid.w_portNmb - 1;
#if( k_CLK_IS_TC == TRUE )
        /* set new min pdelay request interval */
        CTL_s_ClkDS.as_TcPortDs[w_ifIdx].c_logMPdelIntv = 
          (INT8)ps_mntMsg->s_mntTlv.pb_data[0];
#endif /* if TC == TRUE */
#if( k_CLK_IS_TC == FALSE )
        /* set new min pdelay request interval */
        CTL_s_ClkDS.as_PortDs[w_ifIdx].c_PdelReqIntv = 
          (INT8)ps_mntMsg->s_mntTlv.pb_data[0];
#endif /* if TC == FALSE */
#if( k_CLK_DEL_P2P == TRUE )
        /* if peer-to-peer mechanism is implemented */
        P2P_SetPDelReqInt(w_ifIdx,(INT8)ps_mntMsg->s_mntTlv.pb_data[0]);
#endif /* if P2P == TRUE */
      }
      break;
    }
    /* unknown management id */
    default:
    {
      /* set internal mbox type to management error response */ 
      (*pps_msg)->e_mType   = e_IMSG_MM_ERRRES; 
      /* allocate buffer for display an error text */
      (*pps_msg)->s_pntExt1.pv_data = SIS_Alloc(sizeof(PTP_t_Text));
      if((*pps_msg)->s_pntExt1.pv_data != NULL)
      {
        MNT_GetPTPText("The managementId is not recognized",
                       (*pps_msg)->s_pntExt1.ps_text);
      }
      break;
    }
  }
  if((*pps_msg) != NULL)
  {
    /* send response to MNT task */
    if(SIS_MboxPut(MNT_TSK,(*pps_msg)) == FALSE)
    {
      /* free all buffers on failure */
      if(ps_mntMsg->s_mntTlv.pb_data != NULL)
      {
        SIS_Free(ps_mntMsg->s_mntTlv.pb_data);
      }
      if((*pps_msg)->s_pntExt1.pv_data != NULL)
      {
        SIS_Free((*pps_msg)->s_pntExt1.pv_data);
      }
      SIS_Free(ps_mntMsg);
    }
  }
  return b_state;
}




