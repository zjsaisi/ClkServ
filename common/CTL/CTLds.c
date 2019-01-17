/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: CTLds.c 
**    Summary: The control unit CTL controls all node data and all events
**             for the node and his channels to the net. This module 
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: CTL_InitClockDS
**             CTL_ResetCurDs
**             CTL_InitParentDS
**             CTL_InitFrgnMstDS
**             CTL_ClearFrgnMstDs
**             CTL_UpdtFrgnMstDs
**             CTL_SetAccpFlagFrgnMstDs
**             CTL_UpdateM1_M2
**             CTL_UpdateMMDlrqIntv
**             CTL_UpdateS1
**             CTL_AmIGrandmaster
**             CTL_SetPDStoGM
**             CTL_SetPDSstatistics
**             CTL_ResetPDSstatistics
**             CTL_GetActOffs
**
**             InitDefDS
**             InitCurDs
**             InitTmPrptDS
**             InitPortDS
**             SetFrgnMstDsRec
**             ClearFrgnMstDsEntry
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
#include "FIO/FIO.h"
#include "CIF/CIF.h"
#include "CTL/CTL.h"
#include "CTL/CTLint.h"

/*************************************************************************
**    global variables
*************************************************************************/
extern UINT16 CTL_w_anncTmoExt[k_NUM_IF][k_CLK_NMB_FRGN_RCD];

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/

static void InitDefDS( const PTP_t_cfgRom* ps_cfgRom);
#if((k_CLK_IS_OC) || (k_CLK_IS_BC))
static void InitCurDs(const PTP_t_cfgClkRcvr *ps_cRcvr);
static void InitTmPrptDS(void);
static void SetFrgnMstDsRec(UINT16                   w_ifIdx,
                            UINT16                   w_rcrd,
                            const NIF_t_PTPV2_AnnMsg *ps_msg,
                            BOOLEAN                  o_isUc,
                            const PTP_t_PortAddr     *ps_pAddr,
                            UINT32                   dw_ticks);
static void ClearFrgnMstDsEntry(UINT16 w_ifIdx,UINT16 w_rcrd, UINT16 w_ent);

#endif /* #if((k_CLK_IS_OC) || (k_CLK_IS_BC)) */
static void InitPortDS(const PTP_t_cfgRom *ps_cfgRom,
                       const PTP_t_cfgClkRcvr *ps_cRcvr,
                       UINT16 w_ifIdx,
                       PTP_t_stsEnum e_sts);
/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**
** Function    : CTL_InitClockDS
**
** Description : Initializes the Clock Data set. 
**               Member of the clock data set are: 
**                             - Default data set
**                             - Current data set 
**                             - Parent data set 
**                             - Global time properties data set 
**                             - Port configuration data sets 
**                             - Foreign master data sets 
** 
** Parameters  : ps_cfgRom    (IN) - pointer to configuration out of rom
**               ps_cRcvr     (IN) - pointer to clock recovery file
**
** Returnvalue : -
** 
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_InitClockDS( const PTP_t_cfgRom     *ps_cfgRom,
                      const PTP_t_cfgClkRcvr *ps_cRcvr)
{
  UINT16 w_ifIdx;
#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
  UINT16 w_rcrd;

  /* set grandmaster flag to TRUE */
  CTL_s_ClkDS.o_IsGM = TRUE;
  /* CURRENT DATA SET */   
  InitCurDs(ps_cRcvr);
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
  /* DEFAULT DATA SET */ 
  InitDefDS(ps_cfgRom);
#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
  /* PARENT DATA SET */ 
  CTL_InitParentDS();
  /* GLOBAL TIME PROPERTIES DATA SET */
  InitTmPrptDS();  
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
  /* initialize PORT CONFIGURATION DATA SETS to status INITIALIZING */
  for( w_ifIdx = 0 ; w_ifIdx < CTL_s_comIf.dw_amntInitComIf ; w_ifIdx++)
  {
    /* call init function */
    InitPortDS(ps_cfgRom,ps_cRcvr,w_ifIdx,e_STS_INIT);
  } 
  /* initialize PORT CONFIGURATION DATA SETS of the rest 
      of the interfaces to FAULTY */
  for( ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    /* call init function */
    InitPortDS(ps_cfgRom,ps_cRcvr,w_ifIdx,e_STS_FAULTY);
  }
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
  /* FOREIGN MASTER DATA SETS */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    /* initialize all records of the port */
    for( w_rcrd = 0 ; w_rcrd < k_CLK_NMB_FRGN_RCD ; w_rcrd++ )
    {
      /* call init function for port w_ifIdx and entry b_entry */
      CTL_InitFrgnMstDS(w_ifIdx,w_rcrd);
    }
  }
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
}

#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
/***********************************************************************
**
** Function    : CTL_ResetCurDs
**
** Description : Resets the current data set to zero.
**              
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_ResetCurDs( void )
{
  CTL_s_ClkDS.s_CurDs.w_stepsRmvd                   = 0;
  CTL_s_ClkDS.s_CurDs.s_tiOffsFrmMst.ll_scld_Nsec   = 0;
  CTL_s_ClkDS.s_CurDs.s_tiMeanPathDel.ll_scld_Nsec  = 0;
  CTL_s_ClkDS.s_CurDs.s_tiFilteredOffs.ll_scld_Nsec = 0;
}

/***********************************************************************
**
** Function    : CTL_InitParentDS
**
** Description : Initializes the parent data set. 
**              
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_InitParentDS(void)
{
  CTL_s_ClkDS.s_PrntDs.s_prntPortId.s_clkId = CTL_s_ClkDS.s_DefDs.s_clkId;
  CTL_s_ClkDS.s_PrntDs.s_prntPortId.w_portNmb  = 0;
  CTL_s_ClkDS.s_PrntDs.o_prntStats = FALSE;
  CTL_s_ClkDS.s_PrntDs.w_obsPrntOffsScldLogVar = k_MAX_U16;
  CTL_s_ClkDS.s_PrntDs.l_obsPrntClkPhsChgRate  = k_MAX_I32;
  CTL_s_ClkDS.s_PrntDs.s_gmClkId = CTL_s_ClkDS.s_DefDs.s_clkId;
  CTL_s_ClkDS.s_PrntDs.s_gmClkQual.e_clkAccur 
    = CTL_s_ClkDS.s_DefDs.s_clkQual.e_clkAccur;
  CTL_s_ClkDS.s_PrntDs.s_gmClkQual.e_clkClass
    = CTL_s_ClkDS.s_DefDs.s_clkQual.e_clkClass;
  CTL_s_ClkDS.s_PrntDs.s_gmClkQual.w_scldVar
    = CTL_s_ClkDS.s_DefDs.s_clkQual.w_scldVar;
  CTL_s_ClkDS.s_PrntDs.b_gmPrio1 = CTL_s_ClkDS.s_DefDs.b_prio1;
  CTL_s_ClkDS.s_PrntDs.b_gmPrio2 = CTL_s_ClkDS.s_DefDs.b_prio2;
}

/***********************************************************************
**
** Function    : CTL_InitFrgnMstDS
**
** Description : Initializes a foreign master data set. 
**              
** Parameters  : w_ifIdx (IN) - communication interface index
**               w_rcrd  (IN) - entry of data set ( 0,...,n-1 )
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_InitFrgnMstDS(UINT16 w_ifIdx,UINT16 w_rcrd)
{
  UINT16 w_entr;
  
  /* initialization of standard fields:   */
  PTP_MEMSET(CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].s_frgnMstPId.s_clkId.ab_id,
             '\0',
             k_CLKID_LEN);
  CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].s_frgnMstPId.w_portNmb = 0;
  CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].dw_NmbAnncMsg = 0;   
  /* initialization of implementation specific fields   */
  for( w_entr = 0 ; w_entr <  k_FRGN_MST_TIME_WND ; w_entr ++ )
  {
    CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].adw_rxTc[w_entr] = 0;
    CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].ao_Valid[w_entr] = FALSE;
    CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].o_isUc = FALSE;
  }  
  CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].ps_anncMsg = NULL;
}

/***********************************************************************
**
** Function    : CTL_ClearFrgnMstDs
**
** Description : Clears the foreign master data sets. 
**               All sync messages older than PTP_FOREIGN_MASTER_TIME_WINDOW 
**               shall be cleared.
**
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_ClearFrgnMstDs(void)
{
  UINT16          w_ifIdx;
  UINT16          w_rcrd,w_ent;
    
  /* clear all datasets of syncs older than PTP_FOREIGN_MASTER_TIME_WINDOW 
     for each record of all ports */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    for( w_rcrd = 0 ; w_rcrd < k_CLK_NMB_FRGN_RCD ; w_rcrd++ )
    {
      /* if this record contains data */
      if( CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].dw_NmbAnncMsg != 0 )
      {
        /* for each time entry of the record of this port  */
        for( w_ent = 0 ; w_ent < k_FRGN_MST_TIME_WND ; w_ent++ )
        {        
          ClearFrgnMstDsEntry(w_ifIdx,w_rcrd,w_ent);
        }
      }      
    }
  }
}

/***********************************************************************
**
** Function    : CTL_UpdtFrgnMstDs
**
** Description : Updates the foreign master data set.
**               When a announce message is received from a foreign master
**               clock, the foreign master data set of the receiving
**               port shall be updated. If the foreign master is known,
**               the corresponding data set will be updated by 
**               incrementing the field dw_NmbAnncMsg. When it
**               is a unknown foreign master, a new set will be created.
**               Data sets, that were not updated since 
**               PTP_FOREIGN_MASTER_TIME_WINDOW will be deleted.
**
** Parameters  : ps_newMsg (IN) - pointer to received announce msg
**               w_ifIdx   (IN) - receiving communication interface index
**               ps_pAddr  (IN) - port Address (for unicast masters)
**
** Returnvalue : TRUE  - is acceptable master
**               FALSE - is not acceptable master
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
BOOLEAN CTL_UpdtFrgnMstDs(const NIF_t_PTPV2_AnnMsg* ps_newMsg,
                          UINT16 w_ifIdx,
                          const PTP_t_PortAddr *ps_pAddr)
{
  UINT32  dw_ticks;
  UINT32  dw_oldestTick = 0;
  UINT16  w_old;  
  UINT16  w_rec,w_ent;
  BOOLEAN o_isUc;
  BOOLEAN o_ret = FALSE;
  const NIF_t_PTPV2_AnnMsg *ps_oldMsg;
  BOOLEAN o_isAcc = TRUE;
  
  /* get actual time in SIS ticks */
  dw_ticks = SIS_GetTime();
  /* get unicast flag */
  o_isUc = GET_FLAG(ps_newMsg->s_ptpHead.w_flags,k_FLG_UNICAST);
  /* fill new foreign master announce message in appropriate field */

  /* check all records if they are identical with new message */
  for( w_rec = 0 ; w_rec < k_CLK_NMB_FRGN_RCD ; w_rec++ )
  {
    /* check, if the port id´s are the same, and if communication tech. 
      is the same (unicast / multicast) */
    if((o_isUc == CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].o_isUc) && 
       (PTP_ComparePortId(&ps_newMsg->s_ptpHead.s_srcPortId,
                          &CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].s_frgnMstPId)
                          == PTP_k_SAME))
    {
      /* search for a free entry */
      for( w_ent = 0 ; w_ent < k_FRGN_MST_TIME_WND ; w_ent++ )
      {
        /* first clear this entry (remove too OLD records) */
        ClearFrgnMstDsEntry(w_ifIdx,w_rec,w_ent);
        /* get pointer to old message */
        ps_oldMsg = (const NIF_t_PTPV2_AnnMsg*)
                     CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ps_anncMsg;      
        /* entry not free ? */
        if( CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ao_Valid[w_ent] == TRUE )
        {  
          if( ps_oldMsg != NULL )
          {
            /* check, if this is a repeated message */
            if( ps_newMsg->s_ptpHead.w_seqId == ps_oldMsg->s_ptpHead.w_seqId )
            {
              /* stop loop, this is a repeated message */
              break;
            }
          }
        }
        /* this is a free entry */
        else
        {
          /* set the rx timestamp in the entry */
          CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].adw_rxTc[w_ent] = dw_ticks;
          /* set the entry to valid */
          CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ao_Valid[w_ent] = TRUE;
          /* release old message */
          if( ps_oldMsg != NULL )
          {
            SIS_Free((void*)ps_oldMsg);
          }
          /* set new message as qualified announce message
             of this foreign master */
          CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ps_anncMsg = ps_newMsg;
          /* increment member foreign master announce messages */
          CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].dw_NmbAnncMsg++;    
          /* stop loop */
          o_ret = TRUE;
          break;
        }        
      }
      /* store new message in oldest entry and delete oldest one */
      if( o_ret == FALSE )
      {
        /* search for the oldest entry */
        dw_oldestTick = dw_ticks;
        w_old = 0;
        for( w_ent = w_old ; w_ent < k_FRGN_MST_TIME_WND ; w_ent++ )
        {
          if( CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].adw_rxTc[w_ent] < 
              dw_oldestTick )
          {
            dw_oldestTick 
                      = CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].adw_rxTc[w_ent];
            w_old = w_ent;
          }
        }  
        /* set the rx timestamp in the entry */
        CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].adw_rxTc[w_old] = dw_ticks;
        /* set the entry to valid */
        CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ao_Valid[w_old] = TRUE;
        /* release old message */
        if( CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ps_anncMsg != NULL )
        {
          SIS_Free((void*)CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ps_anncMsg);
        }
        /* set new message as qualified announce message of this 
           foreign master */
        CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ps_anncMsg = ps_newMsg;
        o_ret = TRUE;
      }
      /* get acceptable flag */
      o_isAcc = CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].o_isAcc;
    }
    /* if this is not an acceptable master, check, if the timeout 
       for a retry is reached */
    if( CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].o_isAcc == FALSE )
    {
      if( SIS_TIME_OVER(CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].dw_retryTime))
      {
        CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].o_isAcc = TRUE;
      }
    }
  }
  /* if not done, search for a free record */
  if( o_ret == FALSE )
  {
    for( w_rec = 0 ; w_rec < k_CLK_NMB_FRGN_RCD ; w_rec++ )
    {
      /* a free entry has no pointer to a qualified sync */
      if( CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rec].ps_anncMsg == NULL )
      {
        /* set the entry */
        SetFrgnMstDsRec(w_ifIdx,w_rec,ps_newMsg,o_isUc,ps_pAddr,dw_ticks);  
        o_ret = TRUE;
        /* stop loop */
        break;
      }
    }
  }
  /* if the announce message was not inserted, free the memory */
  if( o_ret == FALSE )
  {  
    SIS_Free((void*)ps_newMsg);
  }
  return o_isAcc;
}

/***********************************************************************
**  
** Function    : CTL_SetAccpFlagFrgnMstDs
**  
** Description : Sets the value for the acceptable flag in the foreign
**               master data set and the timeout if the master is set 
**               to unacceptable.
**  
** Parameters  : ps_pId  (IN) - port Id of master
**               w_ifIdx (IN) - communication interface index to master
**               o_isUc  (IN) - is master unicast master ?
**               o_isAcc (IN) - flag to set
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTL_SetAccpFlagFrgnMstDs(const PTP_t_PortId *ps_pId,
                              UINT16 w_ifIdx,
                              BOOLEAN o_isUc,
                              BOOLEAN o_isAcc)
{
  UINT16 i;
  UINT64 ddw_period;
  /* search for the master */
  for( i = 0 ; i < k_CLK_NMB_FRGN_RCD ; i++ )
  {
    /* compare port id of entry */
    if( (PTP_ComparePortId(ps_pId,
                          &CTL_s_ClkDS.aas_FMstDs[w_ifIdx][i].s_frgnMstPId)
                          == PTP_k_SAME) &&
        (CTL_s_ClkDS.aas_FMstDs[w_ifIdx][i].o_isUc == o_isUc))

    {
      CTL_s_ClkDS.aas_FMstDs[w_ifIdx][i].o_isAcc = o_isAcc;
      /* set timeout */
      if( o_isAcc == FALSE )
      {
        /* get period in SIS ticks */
        ddw_period = k_MST_RETRY_TIME_SEC * (k_NSEC_IN_SEC/k_PTP_TIME_RES_NSEC);
        CTL_s_ClkDS.aas_FMstDs[w_ifIdx][i].dw_retryTime 
                                           = SIS_GetTime() + (UINT32)ddw_period;
      }
      /* found - stop loop */
      break;
    }
  }
}

/***********************************************************************
**
** Function    : CTL_UpdateM1_M2
**
** Description : Updates for state decision codes M1 and M2.  
**               The port switches to PTP_MASTER. So the parent data set 
**               and the grandmaster data set is set to the data 
**               of the default data set.
**              
** Parameters  : -
**
** Returnvalue : -            
**                      
** Remarks     : definition in chapter 9.3.5 table 13 of the standard
**
***********************************************************************/
void CTL_UpdateM1_M2( void )
{
  /* no updates in current data set. They are done when node is grandmaster */
  
  /* Updates for parent data set  */
  /* is done after determining, if the node is grandmaster now */

  /* time properties data set */  
  /* set read pointer to appropriate time properties data set */
  if( CTL_ps_extTmPropDs != NULL )
  {
    CTL_s_ClkDS.ps_rdTmPrptDs = CTL_ps_extTmPropDs;
  }
  else
  {
    CTL_s_ClkDS.ps_rdTmPrptDs = &CTL_s_ClkDS.s_TmPrptDs;
  }
  /* Update port data set - is done in state decision event */
}

/***********************************************************************
** 
** Function    : CTL_UpdateMMDlrqIntv
**
** Description : Updates the min mean delay request interval value in 
**               the clock data set, if a node changes to master.
**
** Parameters  : w_ifIdx (IN) - communication interface index to update
**
** Returnvalue : -
**
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_UpdateMMDlrqIntv(UINT16 w_ifIdx)
{
  PTP_t_cfgRom s_cfgRom;
  /* read data from rom - ignore errors */
  FIO_ReadConfig(&s_cfgRom);/*lint !e534*/
  /* reset min mean of port delay request interval */
  CTL_s_ClkDS.as_PortDs[w_ifIdx].c_mMDlrqIntv = 
                             s_cfgRom.as_pDs[w_ifIdx].c_dlrqIntv;  
}

/***********************************************************************
** 
** Function    : CTL_UpdateS1
**
** Description : Updates for state decision code S1.
**               After changing the port state to PTP_SLAVE, the parent
**               data set must be updated to the new parent and grand-
**               master values of the announce msg. 
**
** Parameters  : ps_msg (IN) - pointer to received announce msg
**               o_isUc (IN) - flag says, if parent master is unicast
**
** Returnvalue : -
**
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_UpdateS1(const NIF_t_PTPV2_AnnMsg *ps_msg,BOOLEAN o_isUc)
{
  /* update data sets */
  
  /* Update current data set  */
  CTL_s_ClkDS.s_CurDs.w_stepsRmvd = ps_msg->w_stepsRemvd + 1;

  /* Update parent data set  */
  CTL_s_ClkDS.s_PrntDs.s_prntPortId =ps_msg->s_ptpHead.s_srcPortId;
  CTL_s_ClkDS.s_PrntDs.s_gmClkId = ps_msg->s_grdMstId;
  CTL_s_ClkDS.s_PrntDs.s_gmClkQual.e_clkAccur 
    = ps_msg->s_grdMstClkQual.e_clkAccur;
  CTL_s_ClkDS.s_PrntDs.s_gmClkQual.e_clkClass
    = ps_msg->s_grdMstClkQual.e_clkClass;
  CTL_s_ClkDS.s_PrntDs.s_gmClkQual.w_scldVar
    = ps_msg->s_grdMstClkQual.w_scldVar;
  CTL_s_ClkDS.s_PrntDs.b_gmPrio1 = ps_msg->b_grdMstPrio1;
  CTL_s_ClkDS.s_PrntDs.b_gmPrio2 = ps_msg->b_grdMstPrio2;
  /* store if parent master is unicast master */
  CTL_s_ClkDS.s_PrntDs.o_isUc    = o_isUc;

  /* Update time properties data set  */
  CTL_s_ClkDS.s_TmPrptDs.i_curUtcOffs = ps_msg->i_curUTCOffs;
  CIF_SetSysUtcOffset(CTL_s_ClkDS.s_TmPrptDs.i_curUtcOffs);
  CTL_s_ClkDS.s_TmPrptDs.o_curUtcOffsVal 
    = GET_FLAG(ps_msg->s_ptpHead.w_flags,k_FLG_CUR_UTCOFFS_VAL);
  CTL_s_ClkDS.s_TmPrptDs.o_leap_59 
    = GET_FLAG(ps_msg->s_ptpHead.w_flags,k_FLG_LI_59);
  CTL_s_ClkDS.s_TmPrptDs.o_leap_61 
    = GET_FLAG(ps_msg->s_ptpHead.w_flags,k_FLG_LI_61);
  CTL_s_ClkDS.s_TmPrptDs.o_tmTrcable
    = GET_FLAG(ps_msg->s_ptpHead.w_flags,k_FLG_TIME_TRACEABLE);
  CTL_s_ClkDS.s_TmPrptDs.o_frqTrcable
    = GET_FLAG(ps_msg->s_ptpHead.w_flags,k_FLG_FREQ_TRACEABLE);
  CTL_s_ClkDS.s_TmPrptDs.o_ptpTmScale
    = GET_FLAG(ps_msg->s_ptpHead.w_flags,k_FLG_PTP_TIMESCALE);
  CTL_s_ClkDS.s_TmPrptDs.e_tmSrc = ps_msg->e_timeSrc;
  /* set read pointer to internal time properties data set */
  CTL_s_ClkDS.ps_rdTmPrptDs = &CTL_s_ClkDS.s_TmPrptDs;

  /* Update port data set - is done in state decision event */
}

/***********************************************************************
**
** Function    : CTL_AmIGrandmaster
**
** Description : Checks, if the node is the grandmaster. 
**               When it changes grandmaster the first time, the parent 
**               data set gets set to the values of the default data set. 
**               The clock drift adjustment gets set to zero.
**              
** Parameters  : -
**
** Returnvalue : TRUE  - configuration changed
**               FALSE - configuration did not change
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
BOOLEAN CTL_AmIGrandmaster(void)
{  
  BOOLEAN       o_amGm;
  BOOLEAN       o_cfgChg = FALSE; 
  UINT16        w_ifIdx;
  /* if all channels are in state PTP_MASTER or PTP_PASSIVE, this is the
     grandmaster node */
  
  /* preinitialize flag */
  o_amGm = TRUE;
  for( w_ifIdx = 0 ; w_ifIdx < CTL_s_comIf.dw_amntInitComIf ; w_ifIdx++ )
  {
    if(( CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts == e_STS_SLAVE ) ||
       ( CTL_s_ClkDS.as_PortDs[w_ifIdx].e_portSts == e_STS_LISTENING ))
    {
      /* set flag to false and break the loop */
      o_amGm = FALSE;
      break;
    }
  }  
  /* if the node changes to grandmaster, the parent data set must
     get some other values and the clock drift adjustment must be set to 0 */
  if( ( o_amGm == TRUE ) && ( CTL_s_ClkDS.o_IsGM == FALSE ) )
  {
    /* set the node to grandmaster */
    CTL_s_ClkDS.o_IsGM = TRUE;
    o_cfgChg   = TRUE;
    /* set the parent and grandmaster data set members to the 
       data members of the default data set */
    CTL_SetPDStoGM();
    /* reset the curent data set members to zero */
    CTL_ResetCurDs();
    /* reset the parent data set statistics */
    CTL_s_ClkDS.s_PrntDs.o_prntStats = FALSE;
    CTL_s_ClkDS.s_PrntDs.w_obsPrntOffsScldLogVar = k_MAX_U16;
    CTL_s_ClkDS.s_PrntDs.l_obsPrntClkPhsChgRate  = k_MAX_I32;
    /* reset clock drift */
    CTL_ResetClockDrift();
    /* stop the slave tasks */
    SIS_EventSet(SLS_TSK,k_EV_SC_MST);
#if( k_CLK_DEL_E2E == TRUE )
    SIS_EventSet(SLD_TSK,k_EV_SC_MST);
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
  }
  /* the other way round */
  if( ( o_amGm == FALSE ) && ( CTL_s_ClkDS.o_IsGM == TRUE ) )
  {
    /* reset the grandmaster flag */
    CTL_s_ClkDS.o_IsGM = FALSE;
    o_cfgChg   = TRUE;
  }
  return o_cfgChg;
}


/***********************************************************************
**
** Function    : CTL_SetPDStoGM
**
** Description : When the node gets grandmaster of the subdomain, the 
**               members of the parent data set have to be set to the 
**               values of the default data set. Some values must then 
**               be changed in the sync - and delay_req message template,too.
**              
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_SetPDStoGM(void)
{
  CTL_s_ClkDS.s_PrntDs.s_prntPortId.s_clkId = CTL_s_ClkDS.s_DefDs.s_clkId;
  CTL_s_ClkDS.s_PrntDs.s_prntPortId.w_portNmb = 0;
  CTL_s_ClkDS.s_PrntDs.s_gmClkId = CTL_s_ClkDS.s_DefDs.s_clkId;
  CTL_s_ClkDS.s_PrntDs.s_gmClkQual.e_clkAccur 
    = CTL_s_ClkDS.s_DefDs.s_clkQual.e_clkAccur;
  CTL_s_ClkDS.s_PrntDs.s_gmClkQual.e_clkClass
    = CTL_s_ClkDS.s_DefDs.s_clkQual.e_clkClass;
  CTL_s_ClkDS.s_PrntDs.s_gmClkQual.w_scldVar
    = CTL_s_ClkDS.s_DefDs.s_clkQual.w_scldVar;
  CTL_s_ClkDS.s_PrntDs.b_gmPrio1 = CTL_s_ClkDS.s_DefDs.b_prio1;
  CTL_s_ClkDS.s_PrntDs.b_gmPrio2 = CTL_s_ClkDS.s_DefDs.b_prio2;
}

/***********************************************************************
**  
** Function    : CTL_SetPDSstatistics
**  
** Description : Generates the synchronization statistics 
**               in the parent data set.
**  
** Parameters  : pll_psec_per_sec  (IN)  - act psec/sec adjustment
**               pll_meanDriftPsec (OUT) - mean drift in psec/sec
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTL_SetPDSstatistics(const INT64 *pll_psec_per_sec,
                          INT64       *pll_meanDriftPsec )
{
  static INT64 ll_driftSum = 0LL;
  INT64  ll_temp;
  UINT64 ddw_temp;
  
  /* get mean drift change rate in psec/sec */
  *pll_meanDriftPsec = PTP_LowPass(*pll_psec_per_sec,&ll_driftSum,20);
  /* transform the phase change rate into required format */
  ll_temp = ((*pll_meanDriftPsec)*(INT64)1000)/(INT64)909;
  /* check, if value exceeds 32-bit */
  if( ll_temp > (INT64)k_MAX_I32 )/*lint !e685*/
  {
    CTL_s_ClkDS.s_PrntDs.l_obsPrntClkPhsChgRate = k_MAX_I32;
  }
  else if( ll_temp < -((INT64)k_MAX_I32))
  {
    CTL_s_ClkDS.s_PrntDs.l_obsPrntClkPhsChgRate = (INT32)k_MIN_I32;
  }
  else
  {
    CTL_s_ClkDS.s_PrntDs.l_obsPrntClkPhsChgRate = (INT32)ll_temp;
  }
  /* calculate observed parent variance */
  /* get unscaled allen variance */
  ddw_temp = 
    PTP_GetAllenVar((INT32)
             PTP_INTV_TO_NSEC(CTL_s_ClkDS.s_CurDs.s_tiFilteredOffs.ll_scld_Nsec),
             FALSE);
  /* scale it */
  CTL_s_ClkDS.s_PrntDs.w_obsPrntOffsScldLogVar = PTP_scaleVar(ddw_temp);
  /* set parent statistics flag to TRUE */
// jyang: temperorily set to FALSE, need to evaluate in next release
//  CTL_s_ClkDS.s_PrntDs.o_prntStats = TRUE;
  CTL_s_ClkDS.s_PrntDs.o_prntStats = FALSE;
}

/***********************************************************************
**  
** Function    : CTL_ResetPDSstatistics
**  
** Description : Resets the statistics in the parent data set.
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTL_ResetPDSstatistics( void )
{
  /* set mean drift change rate to zero */
  CTL_s_ClkDS.s_PrntDs.l_obsPrntClkPhsChgRate = 0;
  /* set observed parent variance to zero */
  CTL_s_ClkDS.s_PrntDs.w_obsPrntOffsScldLogVar = 0;
  /* set parent statistics flag to FALSE */
  CTL_s_ClkDS.s_PrntDs.o_prntStats = FALSE;
}

/*************************************************************************
**
** Function    : CTL_GetActOffs
**
** Description : Returns the actual offset from the actual master, or 
**               zero, if the node ist grandmaster.
**
** Parameters  : ps_tiActOffs (OUT) - pointer to actual offset interval
**               
** Returnvalue : - 
** 
** Remarks     : ATTENTION ! Do not use external.
**  
***********************************************************************/
void CTL_GetActOffs(PTP_t_TmIntv *ps_tiActOffs)
{
  ps_tiActOffs->ll_scld_Nsec = CTL_s_ClkDS.s_CurDs.s_tiOffsFrmMst.ll_scld_Nsec;
}
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */

/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
**
** Function    : InitDefDS
**
** Description : Initializes the default data set. 
**              
** Parameters  : ps_cfg    (IN) - pointer to config file in ROM
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static void InitDefDS( const PTP_t_cfgRom* ps_cfg)
{
  /* ordinary / boundary default clock data set */
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
  CTL_s_ClkDS.s_DefDs.o_twoStep            = k_TWO_STEP;
  CTL_s_ClkDS.s_DefDs.s_clkId              = CTL_s_comIf.as_pId[0].s_clkId;
  CTL_s_ClkDS.s_DefDs.w_nmbPorts           = k_NUM_IF;
  CTL_s_ClkDS.s_DefDs.s_clkQual.e_clkAccur = k_CLK_ACCUR;
  CTL_s_ClkDS.s_DefDs.o_slvOnly            = ps_cfg->s_defDs.o_slaveOnly;
  if( CTL_s_ClkDS.s_DefDs.o_slvOnly == TRUE )
  {
    CTL_s_ClkDS.s_DefDs.s_clkQual.e_clkClass = e_CLKC_SLV_ONLY;
  }
  else
  {
    CTL_s_ClkDS.s_DefDs.s_clkQual.e_clkClass = ps_cfg->s_defDs.e_clkClass;
  }
  CTL_s_ClkDS.s_DefDs.s_clkQual.w_scldVar  = ps_cfg->s_defDs.w_scldVar;
  CTL_s_ClkDS.s_DefDs.b_prio1              = ps_cfg->s_defDs.b_prio1;
  CTL_s_ClkDS.s_DefDs.b_prio2              = ps_cfg->s_defDs.b_prio2;
  CTL_s_ClkDS.s_DefDs.b_domn               = ps_cfg->s_defDs.b_domNmb;
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
  /* transparent clock default data set */
#if( k_CLK_IS_TC == TRUE )
  CTL_s_ClkDS.s_TcDefDs.s_clkId    = CTL_s_comIf.as_pId[0].s_clkId;
  CTL_s_ClkDS.s_TcDefDs.w_nmbPorts = k_NUM_IF;
  CTL_s_ClkDS.s_TcDefDs.e_delMech  =
#if( k_CLK_DEL_E2E == TRUE )
      e_DELMECH_E2E;
#else /* #if( k_CLK_DEL_E2E == TRUE ) */
      e_DELMECH_P2P;
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
  CTL_s_ClkDS.s_TcDefDs.b_primDom = ps_cfg->s_defDs.b_domNmb;
#endif /* #if( k_CLK_IS_TC == TRUE ) */
}

#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
/***********************************************************************
**
** Function    : InitCurDs
**
** Description : Initializes the current data set. 
**              
** Parameters  : ps_cRcvr (IN) - clock recovery file
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static void InitCurDs(const PTP_t_cfgClkRcvr *ps_cRcvr)
{
  /* get address of current data set */
  PTP_t_CurDs *ps_curDs = &CTL_s_ClkDS.s_CurDs;
  /* initialize with the configuration data */
  ps_curDs->w_stepsRmvd = 0;
  ps_curDs->s_tiMeanPathDel.ll_scld_Nsec = ps_cRcvr->ll_E2EmeanPDel;
}

/***********************************************************************
**
** Function    : InitTmPrptDS
**
** Description : Initializes the global time properties data set 
**              
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static void InitTmPrptDS(void)
{ 
  /* just init time properties data set, if there is no external one */
  if( CTL_ps_extTmPropDs == NULL )
  {
    CTL_s_ClkDS.s_TmPrptDs.o_ptpTmScale    = k_IS_PTP_TIMESCL;
    /* is it ptp timescale ? */
    if((k_IS_PTP_TIMESCL == TRUE ) && /*lint !e506 */
       ((k_CLK_CLASS == e_CLKC_PRIM) || /*lint !e506 */ 
       (k_CLK_CLASS == e_CLKC_PRIM_HO) || /*lint !e506 */
       (k_CLK_CLASS == e_CLKC_PRIM_A))) /*lint !e506 !e774*/  
    {
      CTL_s_ClkDS.s_TmPrptDs.o_leap_59       = FALSE;
      CTL_s_ClkDS.s_TmPrptDs.o_leap_61       = FALSE;
      CTL_s_ClkDS.s_TmPrptDs.i_curUtcOffs    = k_CUR_UTC_OFFS;
      CTL_s_ClkDS.s_TmPrptDs.o_curUtcOffsVal = TRUE;
      CTL_s_ClkDS.s_TmPrptDs.e_tmSrc         = k_TIME_SOURCE;
      CTL_s_ClkDS.s_TmPrptDs.o_tmTrcable     = TRUE;
      CTL_s_ClkDS.s_TmPrptDs.o_frqTrcable    = TRUE;
    }
    /* timescale is ARB */
    else
    {
      CTL_s_ClkDS.s_TmPrptDs.o_leap_59    = FALSE;
      CTL_s_ClkDS.s_TmPrptDs.o_leap_61    = FALSE;
      CTL_s_ClkDS.s_TmPrptDs.i_curUtcOffs = k_CUR_UTC_OFFS;
      /* is there an external clock source ? */
      CTL_s_ClkDS.s_TmPrptDs.o_curUtcOffsVal = FALSE;
      CTL_s_ClkDS.s_TmPrptDs.o_tmTrcable  = FALSE;
      CTL_s_ClkDS.s_TmPrptDs.o_frqTrcable = FALSE;
      CTL_s_ClkDS.s_TmPrptDs.e_tmSrc      = k_TIME_SOURCE;
    }
    CTL_s_ClkDS.ps_rdTmPrptDs = &CTL_s_ClkDS.s_TmPrptDs;
  }
  else
  {
    CTL_s_ClkDS.ps_rdTmPrptDs = CTL_ps_extTmPropDs;    
  }

}
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
/***********************************************************************
**
** Function    : InitPortDS
**
** Description : Initializes a port configuration data set. 
**              
** Parameters  : ps_cfgRom (IN) - pointer to config file 
**               ps_cRcvr  (IN) - clock recovery file 
**               w_ifIdx   (IN) - communication interface index
**               e_sts     (IN) - port status to initialize
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static void InitPortDS(const PTP_t_cfgRom *ps_cfgRom,
                       const PTP_t_cfgClkRcvr *ps_cRcvr,
                       UINT16 w_ifIdx,
                       PTP_t_stsEnum e_sts)
{
#if( k_CLK_IS_TC == TRUE )
  PTP_t_TcPortDs *ps_tcPortDs;
#endif
  /* ordinary / boundary port data set */
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
  PTP_t_PortDs *ps_portDs;
  /* get pointer to port data set */
  ps_portDs = & CTL_s_ClkDS.as_PortDs[w_ifIdx];
  /* initialize it */
  ps_portDs->s_portId = CTL_s_comIf.as_pId[w_ifIdx];
  ps_portDs->e_portSts = e_sts;
  ps_portDs->c_mMDlrqIntv = ps_cfgRom->as_pDs[w_ifIdx].c_dlrqIntv;
  ps_portDs->s_tiP2PMPDel.ll_scld_Nsec =
#if( k_CLK_DEL_E2E == TRUE) 
      0LL;
      /* prevent compiler warnign */
      ps_cRcvr = ps_cRcvr;
#else /* #if( k_CLK_DEL_E2E == TRUE) */
      ps_cRcvr->all_P2PmeanPDel[w_ifIdx];
#endif /* #if( k_CLK_DEL_E2E == TRUE) */
  /* get and check announce interval */
  ps_portDs->c_AnncIntv = ps_cfgRom->as_pDs[w_ifIdx].c_AnncIntv;
  /* get and check announce receipt timeout */
  ps_portDs->b_anncRecptTmo = ps_cfgRom->as_pDs[w_ifIdx].b_anncRcptTmo;
  /* get and check sync interval */
  ps_portDs->c_SynIntv      = ps_cfgRom->as_pDs[w_ifIdx].c_syncIntv;
  ps_portDs->e_delMech      = 
#if( k_CLK_DEL_E2E == TRUE )
                  e_DELMECH_E2E;
#else /* #if( k_CLK_DEL_E2E == TRUE ) */
                  e_DELMECH_P2P;
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
  /* get and check peer delay request interval */
  ps_portDs->c_PdelReqIntv = ps_cfgRom->as_pDs[w_ifIdx].c_pdelReqIntv;

  ps_portDs->b_verNumber   = k_PTP_VERSION;
#endif /* #if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE)) */
  /* transparent port data sets */
#if( k_CLK_IS_TC == TRUE )
  /* get pointer to transparent port data set */
  ps_tcPortDs = &CTL_s_ClkDS.as_TcPortDs[w_ifIdx];
  /* initialize it */
  ps_tcPortDs->s_pId = CTL_s_comIf.as_pId[w_ifIdx];
  ps_tcPortDs->c_logMPdelIntv = ps_cfgRom->as_pDs[w_ifIdx].c_pdelReqIntv;
  
  ps_tcPortDs->s_peerMPDel.ll_scld_Nsec =
#if( k_CLK_DEL_E2E == TRUE) 
      0LL;
#else /* #if( k_CLK_DEL_E2E == TRUE) */
      ps_cRcvr->all_P2PmeanPDel[w_ifIdx];
#endif /* #if( k_CLK_DEL_E2E == TRUE) */
  if( e_sts == e_STS_FAULTY )
  {
    ps_tcPortDs->o_flt = TRUE;
  }
  else
  {
    ps_tcPortDs->o_flt = FALSE;
  }
#endif /* #if( k_CLK_IS_TC == TRUE ) */  
}

#if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE ))
/***********************************************************************
**
** Function    : SetFrgnMstDsRec
**
** Description : Sets a new record in the foreign master data set. 
**               The data source is an announce message.
**              
** Parameters  : w_ifIdx  (IN)  - communication interface index
**               w_rcrd   (IN)  - entry of data set ( 0,...,n-1 )
**               ps_msg   (IN)  - pointer to an annouce message, the
**                                source of the data to fill in
**               o_isUc   (IN)  - is message unicast ?
**               ps_pAddr (IN)  - port address (for unicast masters=
**               dw_ticks  (IN) - rx time in SIS timer ticks
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static void SetFrgnMstDsRec(UINT16                   w_ifIdx,
                            UINT16                   w_rcrd,
                            const NIF_t_PTPV2_AnnMsg *ps_msg,
                            BOOLEAN                  o_isUc,
                            const PTP_t_PortAddr     *ps_pAddr,
                            UINT32                   dw_ticks)
{
  UINT8 w_entr = 0;
  /* get pointer to foreign master data set record */
  PTP_t_FMstDs *ps_fMstRcrd = &CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd];
  
  /* initialization of standard fields:   */
  /* port id field */
  ps_fMstRcrd->s_frgnMstPId  = ps_msg->s_ptpHead.s_srcPortId;
  /* set foreign master syncs to 1 (this message) */
  ps_fMstRcrd->dw_NmbAnncMsg = 1;   
  /* initialization of implementation specific fields */
  ps_fMstRcrd->adw_rxTc[w_entr] = dw_ticks;
  ps_fMstRcrd->ao_Valid[w_entr] = TRUE;
  ps_fMstRcrd->o_isAcc          = TRUE;
  ps_fMstRcrd->dw_retryTime     = 0;
  ps_fMstRcrd->o_isUc           = o_isUc;
  /* copy port address for unicast masters */
  if( ps_fMstRcrd->o_isUc == TRUE )
  {
    ps_fMstRcrd->s_pAddr.w_AddrLen  = ps_pAddr->w_AddrLen;
    ps_fMstRcrd->s_pAddr.e_netwProt = ps_pAddr->e_netwProt;
    PTP_BCOPY(ps_fMstRcrd->s_pAddr.ab_Addr,
              ps_pAddr->ab_Addr,
              ps_pAddr->w_AddrLen);
  }
  /* set rest of array to initialization value */
  w_entr++;
  while( w_entr < k_FRGN_MST_TIME_WND )
  {
    ps_fMstRcrd->ao_Valid[w_entr] = FALSE;
    ps_fMstRcrd->adw_rxTc[w_entr] = 0;    
    w_entr++;
  }
  /* set qualified message pointer */
  ps_fMstRcrd->ps_anncMsg = ps_msg;
}

/***********************************************************************
**
** Function    : ClearFrgnMstDsEntry
**
** Description : Clears an entry of the foreign master data sets. 
**               All sync messages older than PTP_FOREIGN_MASTER_TIME_WINDOW 
**               shall be cleared.
**
** Parameters  : w_ifIdx (IN) - communication interface index
**               w_rcrd  (IN) - record index
**               w_ent (IN) - record entry index
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
static void ClearFrgnMstDsEntry(UINT16 w_ifIdx,UINT16 w_rcrd, UINT16 w_ent)
{
  UINT32 dw_age;
  UINT32 CTL_dw_tiFrgnMstTmWnd;
  INT8   c_anncIntv;
  UINT32 dw_tck;
    
  if( CTL_w_anncTmoExt[w_ifIdx][w_rcrd] > 0 )
  {
    if( --CTL_w_anncTmoExt[w_ifIdx][w_rcrd] > 0 )
    {
      return;
    }
  }

  /* get act tick count */
  dw_tck = SIS_GetTime();
  /* get the age of this entry */
  dw_age = dw_tck - CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].adw_rxTc[w_ent];
  /* get regarding announce interval */
  c_anncIntv = CTL_s_ClkDS.as_PortDs[w_ifIdx].c_AnncIntv;
  /* get foreign master time window for that interface */
  CTL_dw_tiFrgnMstTmWnd = PTP_getTimeIntv(c_anncIntv) * k_FRGN_MST_TIME_WND;
  /* check the age, if it is outside of the PTP_FOREIGN_MASTER_TIME_WINDOW */
  if(dw_age > CTL_dw_tiFrgnMstTmWnd )
  {
    /* check if it is valid */
    if(CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].ao_Valid[w_ent] == TRUE )
    {
      /* set it invalid */
      CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].ao_Valid[w_ent] = FALSE;
      /* decrement member 'foreign master syncs' */
      CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].dw_NmbAnncMsg--;
    }          
  } 
  /* if the record is now empty, clear qualified message pointer */
  if( CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].dw_NmbAnncMsg == 0)
  {
    /* free allocated memory */
    if( CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].ps_anncMsg
        != NULL )
    {
      SIS_Free((void*)CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].ps_anncMsg); 
      CTL_s_ClkDS.aas_FMstDs[w_ifIdx][w_rcrd].ps_anncMsg = NULL;
    }
    /* reinitialize record  */
    CTL_InitFrgnMstDS(w_ifIdx,w_rcrd);
  }
}

void CTL_ClearAccpFlagFrgnMstDs(UINT16 w_ifIdx)
{
  UINT16 i;
  UINT64 ddw_period;

  for( i = 0 ; i < k_CLK_NMB_FRGN_RCD ; i++ )
  {
    CTL_s_ClkDS.aas_FMstDs[w_ifIdx][i].o_isAcc = FALSE;
    /* get period in SIS ticks */
    ddw_period = k_MST_RETRY_TIME_SEC * (k_NSEC_IN_SEC/k_PTP_TIME_RES_NSEC);
    CTL_s_ClkDS.aas_FMstDs[w_ifIdx][i].dw_retryTime 
                                       = SIS_GetTime() + (UINT32)ddw_period;
  }
}
#endif /* #if((k_CLK_IS_OC == TRUE)|| (k_CLK_IS_BC == TRUE )) */
