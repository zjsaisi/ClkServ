/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: CTLbmc.c 
**    Summary: The control unit CTL controls all node data and all events
**             for the node and his channels to the net. A central task 
**             of the control unit is the state change event. With the sync
**             reiceipt interval, the unit ctl calculates a recommended 
**             state for each port of the node, and switches the channels 
**             to this state.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: CTL_StateDecisionEvent
**
**             StsDecide
**             Bmc_MsgToDefDs
**             Bmc_Msgs
**             Bmc_Msgs_X
**             CheckAndChgStsTC
**             ReqSyncServUc
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
#include "CTL/CTLint.h"
#include "CTL/CTLflt.h"

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
static PTP_t_stsEnum StsDecide(const CTL_t_ClkCmpSet *ps_E_Best,
                               const CTL_t_ClkCmpSet *ps_E_r_Best,
                               UINT8                 *pb_UpdtDec);
static UINT8 Bmc_MsgToDefDs(const CTL_t_ClkCmpSet* );
static UINT8 Bmc_Msgs(const CTL_t_ClkCmpSet*,const CTL_t_ClkCmpSet*);
static UINT8 Bmc_Msgs_X(const CTL_t_ClkCmpSet*,const CTL_t_ClkCmpSet*);
#if( k_CLK_IS_TC == TRUE )
static void CheckAndChgStsTC(PTP_t_stsEnum *pe_recmSts,UINT8 *pb_updCode);
#endif /* #if( k_CLK_IS_TC == TRUE ) */
#if( k_UNICAST_CPBL == TRUE )  
static void ReqSyncServUc(const CTL_t_ClkCmpSet *ps_e_best);
#endif /* #if( k_UNICAST_CPBL == TRUE )   */
/*************************************************************************
**    global functions
*************************************************************************/
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
/***********************************************************************
**
** Function    : CTL_StateDecisionEvent
**
** Description : Executes the state decision event.
**               The state decision event is the mechanism for using the
**               data in received announce messages and his own default data
**               set to determine which is the best master clock, and whether
**               the node needs to change its state. This event occurs 
**               simultaneously on all ports of a clock once per announce
**               interval. 
**               First this function determines the best clock for each port,
**               plugged to this port ( E_rBest ). Then out of this
**               the best clock for the port is determined. Then it
**               decides for each port, which state is recommended and 
**               switches to this state, if it is another.
**              
** Parameters  : pao_anncRcptTmoExp (IN)  - announce receipt timeouts 
**                                          expired flag
**               pao_rstAnncTmo     (OUT) - flag array returns, if announce 
**                                          receipt timeout shall be restarted
**
** Returnvalue : TRUE  - configuration changed
**               FALSE - configuration did not change
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
BOOLEAN CTL_StateDecisionEvent( const BOOLEAN *pao_anncRcptTmoExp,
                                      BOOLEAN *pao_rstAnncTmo)
{
  UINT16                 w_ifIdx,w_rec;
  UINT8                  b_ret;
  UINT8                  ab_updtCode[k_NUM_IF]={0};
  PTP_t_stsEnum          ae_recmSts[k_NUM_IF]; /* recommended state */
  PTP_t_FMstDs           *ps_frgnMstDs;
  NIF_t_PTPV2_AnnMsg     *ps_anncMsg;
  static PTP_t_PortId    s_newMstPid;
  /* best clock plugged to interface n */
  CTL_t_ClkCmpSet        as_er_Best[k_NUM_IF]; 
  /* best clock plugged to node */
  CTL_t_ClkCmpSet        s_e_Best;  
  CTL_t_ClkCmpSet        s_clkTemp;
  BOOLEAN                o_mstChg = FALSE;
  UINT16                 w_N;
  UINT32                 dw_qualTmo;
  INT8                   c_ldIntv;
  PTP_t_MboxMsg          s_msg;

  /* First Clear all old entries in the foreign master data set */
  CTL_ClearFrgnMstDs();

  /* first calculate E_rBest ( best clock, connected to a channel ) 
     for each channel */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    /* preinitialize announce restart flag to FALSE */
    pao_rstAnncTmo[w_ifIdx] = FALSE;
    /* preinitialize the E_rBest of this port to NULL */
    as_er_Best[w_ifIdx].ps_anncMsg = NULL; 
    /* when the port state is INITIALIZING, the announce messages
       are not qualified */
    if( CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts != e_STS_INIT )
    {    
      /* search for the first qualified announce message with 
         acceptable flag set to TRUE */
      for( w_rec = 0 ; w_rec < k_CLK_NMB_FRGN_RCD ; w_rec++ )
      {
        /* if there is an announce message */
        if( (CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ps_anncMsg != NULL) &&
            (CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].o_isAcc == TRUE) )
        {
          ps_frgnMstDs = &CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec];
          ps_anncMsg   = (NIF_t_PTPV2_AnnMsg*)ps_frgnMstDs->ps_anncMsg;
          /* is it qualified ? */
          if( ( ps_frgnMstDs->dw_NmbAnncMsg > k_FRGN_MST_THRESH) &&
              ( ps_anncMsg->w_stepsRemvd < 255 ))
          {
            /* set the first qualified announce message from this port 
               to E_rBest */
            as_er_Best[w_ifIdx].ps_anncMsg = (NIF_t_PTPV2_AnnMsg*)
               CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ps_anncMsg;
            as_er_Best[w_ifIdx].ps_rcvPId  = &CTL_s_comIf.as_pId[w_ifIdx];
            as_er_Best[w_ifIdx].ps_frgnPId 
              = &CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].s_frgnMstPId;
            as_er_Best[w_ifIdx].o_isUc 
              = CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].o_isUc;
            as_er_Best[w_ifIdx].ps_pAddr 
              = &CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].s_pAddr;
            break;
          }
        }
      }
      /* compare this first with the others */
      for(w_rec++ ; w_rec < k_CLK_NMB_FRGN_RCD ; w_rec++ )
      { 
        if( (CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ps_anncMsg != NULL ) &&
            (CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].o_isAcc == TRUE) )
        {
          ps_frgnMstDs = &CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec];
          ps_anncMsg   = (NIF_t_PTPV2_AnnMsg*)ps_frgnMstDs->ps_anncMsg;
          /* is it qualified ? */
          if( ( ps_frgnMstDs->dw_NmbAnncMsg > k_FRGN_MST_THRESH) &&
              ( ps_anncMsg->w_stepsRemvd < 255 ))
          {
            s_clkTemp.ps_anncMsg  = ps_anncMsg;
            s_clkTemp.o_isUc      = ps_frgnMstDs->o_isUc;
            s_clkTemp.ps_frgnPId  = &ps_frgnMstDs->s_frgnMstPId;
            s_clkTemp.ps_rcvPId   = &CTL_s_comIf.as_pId[w_ifIdx];
            /* comparison */
            b_ret = Bmc_Msgs(&as_er_Best[w_ifIdx],&s_clkTemp);
            /* IF this sync is from a better clock, set it as E_rBest */
            if( (b_ret == k_B_BETTER_THAN_A) || 
                (b_ret == k_B_BETTER_THAN_A_BY_TOPOLOGY ))
            { 
              /* this is the new Er_best of this port */
              as_er_Best[w_ifIdx].ps_anncMsg = (NIF_t_PTPV2_AnnMsg*)
                 CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ps_anncMsg;
              as_er_Best[w_ifIdx].ps_rcvPId = &CTL_s_comIf.as_pId[w_ifIdx];
              as_er_Best[w_ifIdx].ps_frgnPId 
              = &CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].s_frgnMstPId;
              as_er_Best[w_ifIdx].o_isUc 
                = CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].o_isUc;
              as_er_Best[w_ifIdx].ps_pAddr 
              = &CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].s_pAddr;
            }            
          }
        }
      } 
    }    
  }
  /* calculate E_Best (E_Best = best external clock of the hole node )
     use E_rBest for this */
  
  /* preinitialize E_Best */
  s_e_Best.ps_anncMsg = NULL;  

  /* search for a first valid E_rBest */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    if( as_er_Best[w_ifIdx].ps_anncMsg != NULL )
    {
      /* set this as first candidate */
      s_e_Best.ps_anncMsg = as_er_Best[w_ifIdx].ps_anncMsg;
      s_e_Best.ps_rcvPId  = &CTL_s_comIf.as_pId[w_ifIdx];
      s_e_Best.ps_frgnPId = as_er_Best[w_ifIdx].ps_frgnPId;
      s_e_Best.o_isUc     = as_er_Best[w_ifIdx].o_isUc;
      s_e_Best.ps_pAddr   = as_er_Best[w_ifIdx].ps_pAddr;
      break;
    }
  }
  /* compare first found E_rBest with the others */
  for( w_ifIdx++ ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    /* check, if port has a valid E_rBest */
    if( as_er_Best[w_ifIdx].ps_anncMsg != NULL )
    {
      /* comparison */
      b_ret = Bmc_Msgs(&s_e_Best,&as_er_Best[w_ifIdx]);
      
      if( (b_ret == k_B_BETTER_THAN_A) || 
          (b_ret == k_B_BETTER_THAN_A_BY_TOPOLOGY ))
      {
        s_e_Best.ps_anncMsg = as_er_Best[w_ifIdx].ps_anncMsg;
        s_e_Best.ps_rcvPId  = &CTL_s_comIf.as_pId[w_ifIdx];
        s_e_Best.ps_frgnPId = as_er_Best[w_ifIdx].ps_frgnPId;
        s_e_Best.o_isUc     = as_er_Best[w_ifIdx].o_isUc;
        s_e_Best.ps_pAddr   = as_er_Best[w_ifIdx].ps_pAddr;
      }
    }
  }
  /* State decision algorithm - for each port calculate
     the recommended state */   
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    /* when the port state is PTP_FAULTY or PTP_DISABLED,
       calculate no state change */
    if((CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts != e_STS_DISABLED) &&
       (CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts != e_STS_FAULTY ))
    {
      /* calculate recommended state for port */
      if( s_e_Best.ps_anncMsg != NULL )
      {
        if( as_er_Best[w_ifIdx].ps_anncMsg != NULL )
        {
          /* decide port status */
          ae_recmSts[w_ifIdx] = StsDecide(&s_e_Best,
                                          &as_er_Best[w_ifIdx],
                                          &ab_updtCode[w_ifIdx]); 
        }
        else
        {
          /* E_Best belongs to another port -> this port must be master */
          ae_recmSts[w_ifIdx]  = e_STS_MASTER; 
          ab_updtCode[w_ifIdx] = k_UDC_M2;
        }
      }
      /* no E_Best -> no E_rBest !! */
      else
      {
        /* so this node must be master */
        ae_recmSts[w_ifIdx]  = e_STS_MASTER;
        ab_updtCode[w_ifIdx] = k_UDC_M1;
      }
    }
    else
    {
      /* set recommended status to the actual */
      ae_recmSts[w_ifIdx] = CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts;
    }
    /* if interface is in states LISTENING,UNCALIBRATED,SLAVE or PASSIVE,
       wait for state change to MASTER until announce receipt timeout expires */
    if( pao_anncRcptTmoExp[w_ifIdx] == FALSE )
    {
      if( ((CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts == e_STS_LISTENING) ||
           (CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts == e_STS_UNCALIBRATED) ||
           (CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts == e_STS_SLAVE) ||
           (CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts == e_STS_PASSIVE)) &&
           ((ae_recmSts[w_ifIdx] == e_STS_MASTER) && 
            (as_er_Best[w_ifIdx].ps_anncMsg == NULL)))
      {
        /* set recommended status to listening */
        ae_recmSts[w_ifIdx] = e_STS_LISTENING;
      }
    }
  }
#if( k_CLK_IS_TC == TRUE )
  CheckAndChgStsTC(ae_recmSts,ab_updtCode);
#endif /* #if( k_CLK_IS_TC == TRUE ) */

  /* elaborate update codes */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    /* when the port state is PTP_FAULTY or PTP_DISABLED,
       execute no state change */
    if( (CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts != e_STS_DISABLED) &&
        (CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts != e_STS_FAULTY ))
    {
      /* if recommended status is LISTENING, FAULTY or DISABLED
         do nothing */
      if( ( ae_recmSts[w_ifIdx] == e_STS_DISABLED) ||
          ( ae_recmSts[w_ifIdx] == e_STS_FAULTY) ||
          ( ae_recmSts[w_ifIdx] == e_STS_LISTENING ))
      {
        /* set status in port data set */
        CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts = ae_recmSts[w_ifIdx];
      }
      /* if state change required */
      else if( ae_recmSts[w_ifIdx] != 
               CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts )
      {
        switch( ab_updtCode[w_ifIdx] )
        {
          case k_UDC_M1:
          case k_UDC_M2:
          {
            /* update data sets */
            CTL_UpdateM1_M2();
            CTL_UpdateMMDlrqIntv(w_ifIdx);
/* if this is a unicast P2P node, reset port address to NULL */
#if((k_UNICAST_CPBL == TRUE)&&(k_CLK_DEL_P2P == TRUE ))
            /* pack message */
            s_msg.e_mType           = e_IMSG_PORTADDR;
            s_msg.s_pntExt1.pv_data = NULL;
            if( SIS_MboxPut(P2Pdel_TSK1 + w_ifIdx,&s_msg ) == FALSE )
            {
              PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_P2PMBOX,e_SEVC_NOTC);
            }
#endif /* #if((k_UNICAST_CPBL == TRUE)&&(k_CLK_DEL_P2P == TRUE )) */
            break;
          }          
          case k_UDC_S1:
          { 
            /* update data sets */
            CTL_UpdateS1(s_e_Best.ps_anncMsg,s_e_Best.o_isUc);
            break;
          }
          case k_UDC_M3:
          case k_UDC_P1:          
          case k_UDC_P2:
          {
            /* update min mean delay request interval */
            CTL_UpdateMMDlrqIntv(w_ifIdx);
/* if this is a unicast P2P node, reset port address to NULL */
#if((k_UNICAST_CPBL == TRUE)&&(k_CLK_DEL_P2P == TRUE ))
            /* pack message */
            s_msg.e_mType           = e_IMSG_PORTADDR;
            s_msg.s_pntExt1.pv_data = NULL;
            if( s_e_Best.o_isUc == TRUE )
            {
              if( SIS_MboxPut(P2Pdel_TSK1 + w_ifIdx,&s_msg ) == FALSE )
              {
                PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_P2PMBOX,e_SEVC_NOTC);
              }
            }             
#endif /* #if((k_UNICAST_CPBL == TRUE)&&(k_CLK_DEL_P2P == TRUE )) */
            break;
          }
          default:
          {
            /* set error - state decision code doesn�t exist */
            PTP_SetError(k_CTL_ERR_ID,CTL_k_STS_DEC,e_SEVC_ERR);
            break;
          }
        }
        /* switch change in appropriate units */
        switch( ae_recmSts[w_ifIdx] )
        {
          case e_STS_SLAVE:
          {
            /* restart announce receipt timeout */
            pao_rstAnncTmo[w_ifIdx] = TRUE;
            /* update state decision = S1 */
            SIS_EventSet(CTL_aw_Hdl_mas[w_ifIdx],k_EV_SC_SLV);
            SIS_EventSet(CTL_aw_Hdl_mad[w_ifIdx],k_EV_SC_SLV);
            SIS_EventSet(CTL_aw_Hdl_maa[w_ifIdx],k_EV_SC_SLV);            
/* just for unicast capable devices */
#if( k_UNICAST_CPBL == TRUE )
            /* check, if the announce message is from a unicast master */
            if( s_e_Best.o_isUc == TRUE )
            {
              /* send unicast request message to request sync and delay 
                 messages from the unicast master */
              ReqSyncServUc(&s_e_Best);
            }
#endif /* #if( k_UNICAST_CPBL == TRUE ) */
            /* get port id of new master to send it to slave task */
            s_newMstPid = s_e_Best.ps_anncMsg->s_ptpHead.s_srcPortId;
            /* pack message */
            s_msg.e_mType            = e_IMSG_NEW_MST;
            s_msg.s_pntData.ps_pId   = &s_newMstPid;
            s_msg.s_pntExt1.ps_pAddr = s_e_Best.ps_pAddr;
            s_msg.s_etc1.w_u16       = w_ifIdx;
            s_msg.s_etc2.o_bool      = s_e_Best.o_isUc;
            /* send it to SLV_SyncTask */
            if( SIS_MboxPut(SLS_TSK,&s_msg) == FALSE )
            {
              PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_SLSMBOX,e_SEVC_NOTC);
            }
            /* input the new master information to the filter unit */
            CTLflt_inpNewMst(&s_newMstPid,s_e_Best.ps_pAddr,s_e_Best.o_isUc);
#if( k_CLK_DEL_E2E == TRUE )
            /* send it to SLV_DelayTask */
            if( SIS_MboxPut(SLD_TSK,&s_msg) == FALSE )
            {
              PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_SLDMBOX,e_SEVC_NOTC);
            }
#else /* #if( k_CLK_DEL_E2E == TRUE ) */
            /* if the best master is unicast AND P2P, then the 
               P2P unit has to be initialized with the port address
               of this master */
            s_msg.e_mType = e_IMSG_PORTADDR;
            if( s_e_Best.o_isUc == TRUE )
            {
              if( SIS_MboxPut(P2Pdel_TSK1 + w_ifIdx,&s_msg ) == FALSE )
              {
                PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_P2PMBOX,e_SEVC_NOTC);
              }
            }
#endif
            
            break;
          }
          case e_STS_PASSIVE:
          {
            /* restart announce receipt timeout */
            pao_rstAnncTmo[w_ifIdx] = TRUE;
            /* update state decision = P1 or P2 */
            SIS_EventSet(CTL_aw_Hdl_mas[w_ifIdx],k_EV_SC_PSV);
            SIS_EventSet(CTL_aw_Hdl_mad[w_ifIdx],k_EV_SC_PSV);
            SIS_EventSet(CTL_aw_Hdl_maa[w_ifIdx],k_EV_SC_PSV);  
            /* switch slave task into passive state, 
               if this is the slave interface */
            if( CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts == e_STS_SLAVE )
            {
              SIS_EventSet(SLS_TSK,k_EV_SC_PSV);
#if( k_CLK_DEL_E2E == TRUE )
              SIS_EventSet(SLD_TSK,k_EV_SC_PSV);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
            }
#if( k_CLK_IS_TC == FALSE ) 
            /* store pId of the master that made this interface passive */
            CTL_s_ClkDS.as_PortDs[w_ifIdx].s_pIdmadeMePsv 
                                        = *as_er_Best[w_ifIdx].ps_frgnPId;
#endif /* #if( k_CLK_IS_TC == FALSE ) */
            break;
          }
          case e_STS_MASTER:
          {
            /* restart announce receipt timeout */
            pao_rstAnncTmo[w_ifIdx] = FALSE;
            /* update state decision = M1 or M2 or M3
               the master need�s to know, how long he should stay in the 
               PTP_PRE_MASTER state, before changing to PTP_MASTER state */
                       
            /* get multiplicator N (page ?) */
            if( ab_updtCode[w_ifIdx] == k_UDC_M3 )
            {
              /* value of steps removed of current data set, incremented by 1 */
              w_N = CTL_s_ClkDS.s_CurDs.w_stepsRmvd + 1;              
            }
            else
            {
              w_N = 0;
            }
            /* get ld announce interval */
            c_ldIntv = CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv;
            /* get qualification timeout for master interface */
            dw_qualTmo = w_N * PTP_getTimeIntv(c_ldIntv);
            s_msg.e_mType = e_IMSG_SC_MST;
            /* calculate qualification timeout */
            s_msg.s_etc1.dw_u32 = dw_qualTmo;
            s_msg.s_etc2.c_s8   = CTL_s_ClkDS.as_PortDs[w_ifIdx].c_SynIntv;
            /* send it to the Master tasks */
            if( SIS_MboxPut(CTL_aw_Hdl_mas[w_ifIdx],&s_msg) == FALSE )
            {
              PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_MASMBOX,e_SEVC_NOTC);
            }
            s_msg.s_etc2.c_s8 = CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv;
            if( SIS_MboxPut(CTL_aw_Hdl_maa[w_ifIdx],&s_msg) == FALSE )
            {
              PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_MAAMBOX,e_SEVC_NOTC);
            } 
            s_msg.s_etc2.c_s8 = CTL_s_ClkDS.as_PortDs[w_ifIdx].c_mMDlrqIntv;
            if( SIS_MboxPut(CTL_aw_Hdl_mad[w_ifIdx],&s_msg) == FALSE )
            {
              PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_MADMBOX,e_SEVC_NOTC);
            }
            break;
          }
          case e_STS_PRE_MASTER:
          case e_STS_FAULTY:
          case e_STS_DISABLED:
          {
            /* do nothing */
            break;
          }
          case e_STS_LISTENING:
          case e_STS_INIT:
          case e_STS_UNCALIBRATED:
          {
            /* set error */
            PTP_SetError(k_CTL_ERR_ID,CTL_k_STS_DEC,e_SEVC_ERR);
            break;
          }
          default:
          {
            /* set error */
            PTP_SetError(k_CTL_ERR_ID,CTL_k_STS_DEC,e_SEVC_ERR);
            break;
          }
        }
        /* change port state in port data set */
        CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts = ae_recmSts[w_ifIdx];
      }
      else
      {
       /* no state change required, but inform Unit SLV from changing  
          best master and apply updates of the master */
        if( ae_recmSts[w_ifIdx] == e_STS_SLAVE )
        {
          /* if the best master changed or master changed in communication
             mode (unicast - multicast), send message to unit SLV  */
          if( (PTP_ComparePortId(&CTL_s_ClkDS.s_PrntDs.s_prntPortId,
                                 &s_e_Best.ps_anncMsg->s_ptpHead.s_srcPortId)
                                   != PTP_k_SAME)||
              (CTL_s_ClkDS.s_PrntDs.o_isUc != s_e_Best.o_isUc ))
          {            
/* just for unicast enabled */
#if( k_UNICAST_CPBL == TRUE )  
            /* set master changed flag */
            if(CTL_s_ClkDS.s_PrntDs.o_isUc != s_e_Best.o_isUc )
            {              
              o_mstChg = FALSE;
            }
            else
            {
              o_mstChg = TRUE;
            }
            /* request sync services if it is unicast master */
            if( s_e_Best.o_isUc == TRUE )
            {
              /* request sync service from unicast master */
              ReqSyncServUc(&s_e_Best);
            }
#endif /* #if( k_UNICAST_CPBL == TRUE ) */
            /* get port id of new master to send it to slave task */
            s_newMstPid = s_e_Best.ps_anncMsg->s_ptpHead.s_srcPortId;
            /* pack message */
            s_msg.e_mType            = e_IMSG_NEW_MST;
            s_msg.s_pntData.ps_pId   = &s_newMstPid;
            s_msg.s_pntExt1.ps_pAddr = s_e_Best.ps_pAddr;
            s_msg.s_etc1.w_u16       = w_ifIdx;
            s_msg.s_etc2.o_bool      = s_e_Best.o_isUc;
            /* send it to SLV_SyncTask */
            if(SIS_MboxPut(SLS_TSK,&s_msg) == FALSE )
            {
              PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_SLSMBOX,e_SEVC_NOTC);
            }  
            /* input the new master information to the filter unit */
            CTLflt_inpNewMst(&s_newMstPid,s_e_Best.ps_pAddr,s_e_Best.o_isUc);
#if( k_CLK_DEL_E2E == TRUE )
            /* send it to SLV_DelayTask */
            if(SIS_MboxPut(SLD_TSK,&s_msg) == FALSE )
            {
              PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_SLDMBOX,e_SEVC_NOTC);
            }
#else
            /* if the best master is unicast AND P2P, then the 
               P2P unit has to be initialized with the port address
               of this master */
            s_msg.e_mType = e_IMSG_PORTADDR;
            if( s_e_Best.o_isUc == TRUE )
            {
              if( SIS_MboxPut(P2Pdel_TSK1 + w_ifIdx,&s_msg ) == FALSE )
              {
                PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_P2PMBOX,e_SEVC_NOTC);
              }
            }
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
          } 
          else
          {
            /* do nothing */
          }
          /* update data sets */
          CTL_UpdateS1(s_e_Best.ps_anncMsg,s_e_Best.o_isUc);
        }
        else
        {
          /* do nothing */
        }
      }
    }
  }
  /* check, if the node is now grandmaster */
  return (CTL_AmIGrandmaster() | o_mstChg);
}
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */

/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
**
** Function    : StsDecide
**
** Description : Function decide�s recommended port status
**               on basis of E_Best,E_rBest and the own clock data set .
**               With a state change, the data sets must be updated,
**               depending on the recommended state. This update 
**               decision code is given by a pointer.
**               
**              
** Parameters  : ps_E_Best    (IN)  - Best plugged node clock
**               ps_E_r_Best  (IN)  - Best plugged node on port
**               pb_UpdtDec   (OUT) - pointer to update decision code   
**
** Returnvalue : PTP_t_stsEnum      - the recommended port state
**                      
** Remarks     : definition at page 67 of standard
**
***********************************************************************/
static PTP_t_stsEnum StsDecide(const CTL_t_ClkCmpSet *ps_E_Best,
                               const CTL_t_ClkCmpSet *ps_E_r_Best,
                               UINT8                 *pb_UpdtDec)
{
  UINT8 b_ret=0;
  
  if( CTL_s_ClkDS.s_DefDs.s_clkQual.e_clkClass < (PTP_t_clkClassEnum)128 )
  {
    /* compare def data set to E_rBest. A = DefDataSet ; B = Er_best */
    b_ret = Bmc_MsgToDefDs(ps_E_r_Best);
    if((b_ret == k_A_BETTER_THAN_B)||(b_ret == k_A_BETTER_THAN_B_BY_TOPOLOGY ))
    {
      /* update state decision code = M1 */
      *pb_UpdtDec = k_UDC_M1;
      return e_STS_MASTER;
    }
    else
    {
      /* update state decision code = P1 */
      *pb_UpdtDec = k_UDC_P1;
      return e_STS_PASSIVE;
    }
  }
  else
  {
    /* compare E_Best to def data set. A = DefDataSet ; B = E_Best */
    b_ret = Bmc_MsgToDefDs(ps_E_Best);
    if((b_ret == k_A_BETTER_THAN_B)||(b_ret == k_A_BETTER_THAN_B_BY_TOPOLOGY))
    {
      /* update state decision code M2 */
      *pb_UpdtDec = k_UDC_M2;
      return e_STS_MASTER;
    }
    else
    { 
      /* check if E_Best was received on this port */
      if( ps_E_Best->ps_rcvPId->w_portNmb == ps_E_r_Best->ps_rcvPId->w_portNmb )
      {
        /* update state decision code S1 */
        *pb_UpdtDec = k_UDC_S1;
        return e_STS_SLAVE;
      }
      else
      {
        b_ret = Bmc_Msgs(ps_E_Best,ps_E_r_Best);
        if( b_ret == k_A_BETTER_THAN_B_BY_TOPOLOGY )
        {
          /* update state decision code P2 */
          *pb_UpdtDec = k_UDC_P2;
          return e_STS_PASSIVE;
        }
        else
        {
          /* update state decision code M3 */
          *pb_UpdtDec = k_UDC_M3;
          return e_STS_MASTER;
        }
      }
    }
  }
}

/***********************************************************************
**
** Function    : Bmc_MsgToDefDs
**
** Description : Function compares an announce message of a 
**               foreign clock to the own clock, using a sync message
**               and the default data set.
**              
** Parameters  : ps_clk       (IN) - pointer to sync message of clock A
**
** Returnvalue : UINT8             - decision, which clock is better 
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static UINT8 Bmc_MsgToDefDs(const CTL_t_ClkCmpSet *ps_clk)
{
  NIF_t_PTPV2_AnnMsg s_msg;
  CTL_t_ClkCmpSet    s_clkComp;
  PTP_t_PortId       s_ownClkId;

  /* pack relevant data of clock data set in an announce message */
  s_msg.s_grdMstId                 = CTL_s_ClkDS.s_DefDs.s_clkId;
  s_msg.b_grdMstPrio1              = CTL_s_ClkDS.s_DefDs.b_prio1;
  s_msg.s_grdMstClkQual.e_clkClass = CTL_s_ClkDS.s_DefDs.s_clkQual.e_clkClass;
  s_msg.s_grdMstClkQual.e_clkAccur = CTL_s_ClkDS.s_DefDs.s_clkQual.e_clkAccur;
  s_msg.s_grdMstClkQual.w_scldVar  = CTL_s_ClkDS.s_DefDs.s_clkQual.w_scldVar;
  s_msg.b_grdMstPrio2              = CTL_s_ClkDS.s_DefDs.b_prio2;
  s_msg.w_stepsRemvd               = 0;
  s_msg.s_ptpHead.s_srcPortId.s_clkId = CTL_s_ClkDS.s_DefDs.s_clkId;
  
  /* set the rx port in the clk_data_type to the clock identity */
  s_ownClkId.s_clkId                  = CTL_s_ClkDS.s_DefDs.s_clkId;
  s_ownClkId.w_portNmb = 0;
  /* set the message in the clk_data_type */
  s_clkComp.ps_anncMsg = &s_msg;
  s_clkComp.ps_rcvPId  = &s_ownClkId;
  
  /* call function to compare two messages */
  return Bmc_Msgs(&s_clkComp,ps_clk);  
}

/***********************************************************************
**
** Function    : Bmc_Msgs
**
** Description : Function compares two clocks with the best master 
**               clock algorithm to each other, using two announce messages.
**              
** Parameters  : ps_clkA      (IN) - pointer to clock compare set of clock A
**               ps_clkB      (IN) - pointer to clock compare set of clock B
**
** Returnvalue : UINT8             - decision, which clock is better 
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static UINT8 Bmc_Msgs(const CTL_t_ClkCmpSet *ps_clkA,
                      const CTL_t_ClkCmpSet *ps_clkB)
{  
  INT8               c_ret;   
  NIF_t_PTPV2_AnnMsg *ps_msgA,*ps_msgB;

  ps_msgA = ps_clkA->ps_anncMsg;
  ps_msgB = ps_clkB->ps_anncMsg;
 
  /* compare grandmaster identity */
  c_ret = PTP_CompareClkId(&ps_msgA->s_grdMstId,&ps_msgB->s_grdMstId);
  /* do they have the same grandmaster ? */
  if( c_ret == PTP_k_SAME )
  {
    /* (X) */
    return Bmc_Msgs_X(ps_clkA,ps_clkB); 
  }
  /* they have other grandmasters */
  else
  {
    /* compare GM prio 1 */
    if( ps_msgA->b_grdMstPrio1 > ps_msgB->b_grdMstPrio1 )
    {
      return k_B_BETTER_THAN_A;
    }
    else if( ps_msgA->b_grdMstPrio1 < ps_msgB->b_grdMstPrio1 )
    { 
      return k_A_BETTER_THAN_B;
    }
    else /* ps_msgA->b_grdMstPrio1 == ps_msgB->b_grdMstPrio1 */
    {
      /* compare GM class values */
      if( ps_msgA->s_grdMstClkQual.e_clkClass > 
          ps_msgB->s_grdMstClkQual.e_clkClass )
      {
        return k_B_BETTER_THAN_A;
      }
      else if( ps_msgA->s_grdMstClkQual.e_clkClass < 
               ps_msgB->s_grdMstClkQual.e_clkClass )
      { 
        return k_A_BETTER_THAN_B;
      }
      else /* ps_msgA->s_grdMstClkQual.e_clkClass == 
              ps_msgB->s_grdMstClkQual.e_clkClass */
      {
        /* compare GM accuracy values */
        if( ps_msgA->s_grdMstClkQual.e_clkAccur > 
            ps_msgB->s_grdMstClkQual.e_clkAccur )
        {
          return k_B_BETTER_THAN_A;
        }
        else if( ps_msgA->s_grdMstClkQual.e_clkAccur < 
                 ps_msgB->s_grdMstClkQual.e_clkAccur )
        { 
          return k_A_BETTER_THAN_B;
        }
        else /* ps_msgA->s_grdMstClkQual.b_clkAccur == 
                ps_msgB->s_grdMstClkQual.b_clkAccur */
        {
          /* compare GM offset scaled log variance */
          if( ps_msgA->s_grdMstClkQual.w_scldVar > 
              ps_msgB->s_grdMstClkQual.w_scldVar )
          {
            return k_B_BETTER_THAN_A;
          }
          else if( ps_msgA->s_grdMstClkQual.w_scldVar < 
                   ps_msgB->s_grdMstClkQual.w_scldVar )
          { 
            return k_A_BETTER_THAN_B;
          }
          else /* ps_msgA->s_grdMstClkQual.w_scldVar == 
                  ps_msgB->s_grdMstClkQual.w_scldVar */
          {
            /* GM A prio 2 = GM B prio 2 ? */
            if( ps_msgA->b_grdMstPrio2 > ps_msgB->b_grdMstPrio2 )
            {
              return k_B_BETTER_THAN_A;
            }
            else if( ps_msgA->b_grdMstPrio2 < ps_msgB->b_grdMstPrio2 )
            { 
              return k_A_BETTER_THAN_B;
            }
            else /* ps_msgA->b_grdMstPrio2 == ps_msgB->b_grdMstPrio2 */
            {
              /* compare GM identity */
              if( c_ret == PTP_k_LESS )
              {
                return k_A_BETTER_THAN_B;
              }
              else
              {
                return k_B_BETTER_THAN_A;
              }
            }
          }
        }
      }
    }
  }              
}

/***********************************************************************
**
** Function    : Bmc_Msgs_X
**
** Description : Subfunction of Bmc_Msgs. 
**               This is a branch of the dataset compare function tree,
**               that is used when the announce messages have the same 
**               grandmaster clock id
**              
** Parameters  : ps_clkA      (IN) - pointer to clock compare set of clock A
**               ps_clkB      (IN) - pointer to clock compare set of clock B
**
** Returnvalue : UINT8             - decision, which clock is better 
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static UINT8 Bmc_Msgs_X(const CTL_t_ClkCmpSet *ps_clkA,
                        const CTL_t_ClkCmpSet *ps_clkB)
{
  INT32               l_diff;
  INT8               c_compIdA,c_compIdB,c_compAtoB;
  NIF_t_PTPV2_AnnMsg *ps_msgA,*ps_msgB;  

  ps_msgA = ps_clkA->ps_anncMsg;
  ps_msgB = ps_clkB->ps_anncMsg;
  
  /* compare steps removed of A and B */
  l_diff = (INT32)ps_msgA->w_stepsRemvd - (INT32)ps_msgB->w_stepsRemvd;
  /* compare identities of Receiver A and Sender A */
  c_compIdA = PTP_CompareClkId(&ps_clkA->ps_rcvPId->s_clkId,
                               &ps_msgA->s_ptpHead.s_srcPortId.s_clkId);
  /* compare identities of Receiver B and Sender B */
  c_compIdB = PTP_CompareClkId(&ps_clkB->ps_rcvPId->s_clkId,
                               &ps_msgB->s_ptpHead.s_srcPortId.s_clkId);
  /* A > B + 1 ? */
  if( l_diff > 1 )
  {
    return k_B_BETTER_THAN_A;
  }
  /* A < B - 1 ? */
  else if( l_diff < -1 )
  {
    return k_A_BETTER_THAN_B;
  }
  /* A within 1 of B */
  else
  {
    /* A > B ? */
    if( l_diff > 0 )
    {
      /* Receiver A < Sender A ? */
      if( c_compIdA == PTP_k_LESS )
      { 
        return k_B_BETTER_THAN_A;
      }
      /* Receiver A > Sender A */
      else if( c_compIdA == PTP_k_GREATER )
      {
        return k_B_BETTER_THAN_A_BY_TOPOLOGY;
      }
      /* Receiver == Sender */
      else
      {
        /* error - Reiceiver is equal to sender */
        PTP_SetError(k_CTL_ERR_ID,CTL_k_BMC_R_EQ_S,e_SEVC_NOTC);
        return k_MESSAGE_FROM_SELF;
      }
    }
    /* A < B ? */
    else if( l_diff < 0 )
    {
      /*  Receiver B < Sender B ? */
      if( c_compIdB == PTP_k_LESS )
      { 
        return k_A_BETTER_THAN_B;
      }
      /* Receiver B > Sender B */
      else if( c_compIdB == PTP_k_GREATER )
      {
        return k_A_BETTER_THAN_B_BY_TOPOLOGY;
      }
      /* Receiver == Sender */
      else
      {
        /* error - Reiceiver is equal to sender */
        PTP_SetError(k_CTL_ERR_ID,CTL_k_BMC_R_EQ_S,e_SEVC_NOTC);
        return k_MESSAGE_FROM_SELF;
      }
    }
    /* A == B */
    else
    { 
      /* compare clock identities of senders A and B */
      c_compAtoB = PTP_CompareClkId(&ps_msgA->s_ptpHead.s_srcPortId.s_clkId,
                                    &ps_msgB->s_ptpHead.s_srcPortId.s_clkId);
      /* A > B ? */
      if( c_compAtoB == PTP_k_GREATER )
      {
        return k_B_BETTER_THAN_A_BY_TOPOLOGY;
      }
      /* A < B ? */
      else if( c_compAtoB == PTP_k_LESS )
      {
        return k_A_BETTER_THAN_B_BY_TOPOLOGY;
      }
      /* A == B ? */
      else
      {
        /* compare port numbers */
        l_diff = (INT32)ps_clkA->ps_rcvPId->w_portNmb - 
                 (INT32)ps_clkB->ps_rcvPId->w_portNmb;
        /* A > B ? */
        if( l_diff > 0 )
        {
          return k_B_BETTER_THAN_A_BY_TOPOLOGY;
        }
        /* A < B ? */
        else if( l_diff < 0 )
        {
          return k_A_BETTER_THAN_B_BY_TOPOLOGY;
        }
        /* A == B ? */
        else
        {
          /* IXXAT specific - a master can be stored as multicast AND unicast */
          /* here the correct master gets selected */
          if( ps_clkA->o_isUc != ps_clkB->o_isUc )
          {
            /* decide in accordance to defined preference */
            if( ps_clkA->o_isUc == k_UC_MST_PREF )
            {
              return k_A_BETTER_THAN_B;
            }
            else
            {
              return k_B_BETTER_THAN_A;
            }
          }
          else
          {
            /* error - messages are identical */
            PTP_SetError(k_CTL_ERR_ID,CTL_k_BMC_A_EQ_B,e_SEVC_NOTC);
            return k_A_SIMILAR_TO_B;
          }
        }
      }
    }
  }       
}
#if( k_CLK_IS_TC == TRUE )
/***********************************************************************
**
** Function    : CheckAndChgStsTC
**
** Description : Checks the recommended status of all ports after 
**               the status decision algorithm. If one port is supposed 
**               to be in slave state, all ports in recommended state 
**               PTP_MASTER change to state PTP_LISTENING.
**              
** Parameters  : pe_recmSts  (IN) - array of recommended status after
**                                  status decision algorithm
**               pb_updCode  (IN) - array of update codes 
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static void CheckAndChgStsTC(PTP_t_stsEnum *pe_recmSts,UINT8 *pb_updCode)
{
  UINT16  w_i;
  BOOLEAN o_grndMst = TRUE;

  /* check, if the node will get grandmaster */
  for( w_i = 0 ; w_i < k_NUM_IF ; w_i++ )
  {
    if( pe_recmSts[w_i] == e_STS_SLAVE )
    {
      o_grndMst = FALSE;
      break;
    }
  }
  /* if the node will not get grandmaster, change all ports 
     in recommended status = PTP_MASTER to PTP_PASSIVE */
  if( o_grndMst == FALSE )
  {
    /* check, if the node will get grandmaster */
    for( w_i = 0 ; w_i < CTL_s_comIf.dw_amntInitComIf ; w_i++ )
    {
      if( pe_recmSts[w_i] == e_STS_MASTER )
      {
        pe_recmSts[w_i] = e_STS_LISTENING;
        pb_updCode[w_i] = k_UDC_P2;
      }
    }
  }
}

#endif /* #if( k_CLK_IS_TC == TRUE ) */
/* just for unicast enabled */
#if( k_UNICAST_CPBL == TRUE )  
/***********************************************************************
**  
** Function    : ReqSyncServUc
**  
** Description : Calls the unit UCD (unicast discovery) to request sync
**               services from a unicast master.
**  
** Parameters  : ps_e_best (IN/OUT) - clock data of unicast master
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void ReqSyncServUc(const CTL_t_ClkCmpSet *ps_e_best)
{
  PTP_t_MboxMsg s_msg;

  /* request master for synchronization and delay messages */
  s_msg.e_mType            = e_IMSG_REQ_UC_SRV;
  s_msg.s_pntData.ps_pAddr = ps_e_best->ps_pAddr;
  s_msg.s_pntExt1.ps_pId   = ps_e_best->ps_frgnPId;
  s_msg.s_etc1.c_s8    
       = CTL_s_ClkDS.as_PortDs[ps_e_best->ps_rcvPId->w_portNmb-1].c_SynIntv;
/* E2E delay mechanism */ 
#if( k_CLK_DEL_E2E == TRUE )
  s_msg.s_etc2.c_s8 
       = CTL_s_ClkDS.as_PortDs[ps_e_best->ps_rcvPId->w_portNmb-1].c_mMDlrqIntv;
/* P2P delay mechanism */
#else /* ( k_CLK_DEL_P2P == TRUE ) */
  s_msg.s_etc2.c_s8 
       = CTL_s_ClkDS.as_PortDs[ps_e_best->ps_rcvPId->w_portNmb-1].c_PdelReqIntv;
#endif
  /* send message to unicast discovery task */
  if( SIS_MboxPut(UCD_TSK,&s_msg) == FALSE )
  {
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_UCDMBOX,e_SEVC_NOTC);
  }
}
#endif /* #if( k_UNICAST_CPBL == TRUE )  */
#endif/* #if(( k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC)) */


