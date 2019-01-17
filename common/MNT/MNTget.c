/*******************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
********************************************************************************
**
**       File: MNTget.c
**    Summary: To support the generation of the GET RESPONSE (also used by 
**             SET commands) each supported management message gets a 
**             corresponding get-function. Each function reads the clock 
**             data sets and clock information directly out of the data 
**             pools and generates the needed TLV payload as defined within 
**             the standard. This data is returned to the calling function. 
**    Version: 1.01.01
**
********************************************************************************
********************************************************************************
**
**  Functions: MNTget_ClkDesc
**             MNTget_UsrDesc
**             MNTget_DefaultDs
**             MNTget_CurrentDs
**             MNTget_ParentDs
**             MNTget_TimePropDs
**             MNTget_PortDs
**             MNTget_Priority1
**             MNTget_Priority2
**             MNTget_Domain
**             MNTget_SlaveOnly
**             MNTget_AnncIntv
**             MNTget_AnncRxTimeout
**             MNTget_SyncIntv
**             MNTget_VersionNumber
**             MNTget_Time
**             MNTget_ClkAccuracy
**             MNTget_UtcProp
**             MNTget_TraceProp
**             MNTget_TimeSclProp
**             MNTget_UcNegoEnable
**             MNTget_UCMasterTbl
**             MNTget_UCMstMaxTblSize
**             MNTget_TCDefaultDs
**             MNTget_TCPortDs
**             MNTget_DelayMechanism
**             MNTget_MinPDelReqIntv
**             MNTget_PrivateStatus
**
**   Compiler: ANSI-C
**    Remarks:
**
********************************************************************************
**    all rights reserved
*******************************************************************************/


/*******************************************************************************
**    compiler directives
*******************************************************************************/


/*******************************************************************************
**    include-files
*******************************************************************************/
#include "target.h"
#include "PTP/PTPdef.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "MNT/MNT.h"
#include "MNT/MNTapi.h"
#include "MNT/MNTint.h"
#include "GOE/GOE.h"
#include "CIF/CIF.h"
#include "API/sc_rev.h"
#include "API/sc_system.h"

/*******************************************************************************
**    global variables
*******************************************************************************/

/* data array for response or acknowledge generation */
UINT8 ab_RespTlvPld[k_MAX_TLV_PLD_LEN];

/*******************************************************************************
**    static constants, types, macros, variables
*******************************************************************************/


/*******************************************************************************
**    static function-prototypes
*******************************************************************************/


/*******************************************************************************
**    global functions
*******************************************************************************/

/***********************************************************************
**
** Function    : MNTget_ClkDesc
**
** Description : This function handles the management message
**               CLOCK_DESCRIPTION.
**
** Parameters  : w_ifIdx      (IN) - port number as array index
**               pw_dynSize (OUT) - dynamic size of payload
**               pe_mntErr  (OUT) - mnt error id
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN MNTget_ClkDesc( UINT16 w_ifIdx, UINT16 *pw_dynSize,
                        PTP_t_mntErrIdEnum *pe_mntErr )
{
  /* temporary PTP Text variable to generate text */
  PTP_t_Text s_text;
  /* pointer to the text */
  const CHAR *pc_text;
  /* temporary storage for dynamic lengths */
  UINT16 w_lenDyn = 0;
  /* temporary pointer to address buffer */
  UINT8 *pb_phyAddrBuf;
  UINT8 *pb_portAddrBuf;
  /* temporary size buffers */
  UINT16 w_phyAddrSize, w_portAddrSize;
  /* temporary port id */
  UINT8 ab_profileId[] = k_CLKDES_PROFILE_ID;
  /* user description string length */
  UINT8 b_userDescStrLen;

  /* reset flags */
  ab_RespTlvPld[0] = 0;
  ab_RespTlvPld[1] = 0;
  /* get clock type description */
  SET_FLAG(ab_RespTlvPld[0],
           k_OFFS_CLKDES_FLAGS_OC,
           k_CLK_IS_OC); 
  SET_FLAG(ab_RespTlvPld[0],
           k_OFFS_CLKDES_FLAGS_BC,
           k_CLK_IS_BC); 
  SET_FLAG(ab_RespTlvPld[0],
           k_OFFS_CLKDES_FLAGS_P2PTC,
          (k_CLK_DEL_P2P && k_CLK_IS_TC)); /*lint !e506 */ 
  SET_FLAG(ab_RespTlvPld[0],
           k_OFFS_CLKDES_FLAGS_E2ETC,
           (k_CLK_DEL_E2E && k_CLK_IS_TC)); /*lint !e506 */ 
#ifdef MNT_API
    SET_FLAG(ab_RespTlvPld[0],
             k_OFFS_CLKDES_FLAGS_MN,
             FALSE); 
#else /* #ifdef MNT_API */
    SET_FLAG(ab_RespTlvPld[0],
             k_OFFS_CLKDES_FLAGS_MN,
             FALSE); 
#endif /* #ifdef MNT_API */
  /* all other flags resvered */

  /* get physical layer description */
  switch(k_CLK_COMM_TECH)
  {
    case e_UDP_IPv4:
    {
      pc_text = k_COMM_TECH_UDP_IPv4;
      break;
    }
    case e_UDP_IPv6:
    {
      pc_text = k_COMM_TECH_UDP_IPv6;
      break;
    }
    case e_IEEE_802_3:
    {
      pc_text = k_COMM_TECH_IEEE_802_3;
      break;
    }
    case e_DEVICENET:
    {
      pc_text = k_COMM_TECH_DEVICENET;
      break;
    }
    case e_CONTROLNET:
    {
      pc_text = k_COMM_TECH_CONTROLNET;
      break;
    }
    case e_PROFINET:
    {
      pc_text = k_COMM_TECH_PROFINET;
      break;
    }
    case e_UNKNOWN:
    {
      pc_text = k_COMM_TECH_UNKNOWN;
      break;
    }
    default:
    {
      pc_text = k_COMM_TECH_UNKNOWN;
      break;
    }
  }
  MNT_GetPTPText(pc_text, &s_text);

  /* save physicalLayerProtocol length */
  w_lenDyn = s_text.b_len+1;  /* add part L */
  ab_RespTlvPld[2] = s_text.b_len;
  /* copy text data */
  PTP_BCOPY(&ab_RespTlvPld[3], 
            s_text.pc_text,          
            s_text.b_len);         
  /* read addresses */
  if( NIF_GetIfAddrs(&w_phyAddrSize, &pb_phyAddrBuf,
                     &w_portAddrSize, &pb_portAddrBuf,
                     w_ifIdx) == FALSE)
  {
    /* error: wrong values are read */
    *pe_mntErr = e_MNTERR_WRONGVALUE;
    return FALSE;
  }
  else
  {

  }
  /* get physical address length */
  GOE_hton16(&ab_RespTlvPld[2+w_lenDyn],
             (UINT16)w_phyAddrSize);

  /* get physical address */
  PTP_BCOPY(&ab_RespTlvPld[4+w_lenDyn], 
            pb_phyAddrBuf,          
            w_phyAddrSize);    

  /* save physicalLayerProtocol length */
  w_lenDyn += w_phyAddrSize; /* add part S */

  /* get protocol address */
  GOE_hton16(&ab_RespTlvPld[4+w_lenDyn],
             (UINT16)k_CLK_COMM_TECH);
  GOE_hton16(&ab_RespTlvPld[4+w_lenDyn+2],
             (UINT16)w_portAddrSize);
  PTP_BCOPY(&ab_RespTlvPld[4+w_lenDyn+4], 
            pb_portAddrBuf,          
            w_portAddrSize);    

  /* save physicalLayerProtocol length */
  w_lenDyn += (w_portAddrSize+4); /* add part N */

  /* get manufacturer id */
  ab_RespTlvPld[4+w_lenDyn] = (((UINT32)k_CLKDES_MANU_ID)>>16)&0xFF;
  ab_RespTlvPld[5+w_lenDyn] = (((UINT32)k_CLKDES_MANU_ID)>>8)&0xFF;
  ab_RespTlvPld[6+w_lenDyn] = (((UINT32)k_CLKDES_MANU_ID))&0xFF;

  /* reserved */
  ab_RespTlvPld[7+w_lenDyn] = 0;

  /* get product description */
  MNT_GetPTPText(k_CLKDES_PRODUCT,&s_text);
  ab_RespTlvPld[8+w_lenDyn] = s_text.b_len;
  /* copy text data */
  PTP_BCOPY(&ab_RespTlvPld[8+w_lenDyn+1], 
            s_text.pc_text,          
            s_text.b_len); 

  /* save product description length */
  w_lenDyn += (s_text.b_len+1);  /* add part P */
  
  /* get revision data */
  MNT_GetPTPText(k_SC_LIB_REV,&s_text);
  ab_RespTlvPld[8+w_lenDyn] = s_text.b_len + 2;
  /* copy text data */
  ab_RespTlvPld[8+w_lenDyn+1] = ';';
  PTP_BCOPY(&ab_RespTlvPld[8+w_lenDyn+2], 
            s_text.pc_text,          
            s_text.b_len); 
  ab_RespTlvPld[8+w_lenDyn+1+s_text.b_len] = ';';
  ab_RespTlvPld[8+w_lenDyn+2+s_text.b_len] = '\0';

  /* save revision data length */
  w_lenDyn += (s_text.b_len+3);  /* add part P */

  /* get user description */
  b_userDescStrLen = (UINT8)PTP_BLEN(MNT_ps_ClkDs->ac_userDesc);
  ab_RespTlvPld[8+w_lenDyn] = b_userDescStrLen;
  /* copy text data */
  PTP_BCOPY(&ab_RespTlvPld[8+w_lenDyn+1], 
            MNT_ps_ClkDs->ac_userDesc,          
            b_userDescStrLen); 

  /* save revision data length */
  w_lenDyn += (b_userDescStrLen+1);  /* add part P */

  /* get profileIdentity */
  PTP_BCOPY(&ab_RespTlvPld[8+w_lenDyn], 
            (UINT8*)ab_profileId,          
            6); 

  /* padding byte */
  if((w_lenDyn & 0x1) == 0x1)
  {
    /* add padding byte */
    ab_RespTlvPld[14+w_lenDyn] = 0;
    w_lenDyn++;
  }
  /* write dynamic size */
  *pw_dynSize = w_lenDyn;
  /* no failure occured */
  return TRUE;
}


/***********************************************************************
**
** Function    : MNTget_UsrDesc
**
** Description : This function handles the management message
**               USER_DESCRIPTION.
**
** Parameters  : pw_dynSize (OUT) - dynamic size of payload
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN MNTget_UsrDesc( UINT16 *pw_dynSize )
{
  /* temporary storage for dynamic lengths */
  UINT8 b_lenDyn = 0;
  UINT8 b_lenUsrDesc; 

  /* get user description size */
  b_lenUsrDesc = (UINT8)PTP_BLEN(MNT_ps_ClkDs->ac_userDesc);
  /* set text size */
  ab_RespTlvPld[k_OFFS_UD_SIZE] = b_lenUsrDesc;
  /* set text */
  PTP_BCOPY(&ab_RespTlvPld[k_OFFS_UD_TEXT], 
            MNT_ps_ClkDs->ac_userDesc,
            b_lenUsrDesc); 
  /* set dynamic length */
  b_lenDyn = b_lenUsrDesc + 1;
  /* get padding data */
  if((b_lenUsrDesc & 0x01) == 0x00)
  {
    /* add padding byte */
    ab_RespTlvPld[b_lenDyn] = 0;
    b_lenDyn++;
  }
  /* write dynamic size */
  *pw_dynSize = b_lenDyn;
  /* no failure occured */
  return TRUE;
}



/***********************************************************************
**
** Function    : MNTget_DefaultDs
**
** Description : This function handles the management message
**               DEFAULT_DATA_SET.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_DefaultDs( void )
{
  UINT16 w_tmpFlags = 0;

  /* get twoStepFlag and slaveOnly Flag */
  ab_RespTlvPld[k_OFFS_DFDS_FLAGS] = 0;  /* reset all flags */
  SET_FLAG(w_tmpFlags,
           k_OFFS_DFDS_FLAGS_TSC,
           MNT_ps_ClkDs->s_DefDs.o_twoStep); 
  SET_FLAG(w_tmpFlags,
           k_OFFS_DFDS_FLAGS_SO,
           MNT_ps_ClkDs->s_DefDs.o_slvOnly);
  ab_RespTlvPld[k_OFFS_DFDS_FLAGS] = (UINT8)w_tmpFlags;
  /* reserved */
  ab_RespTlvPld[k_OFFS_DFDS_RES1] = 0;
  /* get number of ports */
  GOE_hton16(&ab_RespTlvPld[k_OFFS_DFDS_PNUM],
             MNT_ps_ClkDs->s_DefDs.w_nmbPorts);
  /* get priority 1 */
  ab_RespTlvPld[k_OFFS_DFDS_PRIO1] = MNT_ps_ClkDs->s_DefDs.b_prio1;
  /* get clock quality */
  ab_RespTlvPld[k_OFFS_DFDS_QAL_CC] = 
             (UINT8)MNT_ps_ClkDs->s_DefDs.s_clkQual.e_clkClass;
  ab_RespTlvPld[k_OFFS_DFDS_QAL_CA] = 
             (UINT8)MNT_ps_ClkDs->s_DefDs.s_clkQual.e_clkAccur;
  GOE_hton16(&ab_RespTlvPld[k_OFFS_DFDS_QAL_OSLV],
             MNT_ps_ClkDs->s_DefDs.s_clkQual.w_scldVar);
  /* get priority 2 */
  ab_RespTlvPld[k_OFFS_DFDS_PRIO2] = MNT_ps_ClkDs->s_DefDs.b_prio2;
  /* get clock identity */
  PTP_BCOPY(&ab_RespTlvPld[k_OFFS_DFDS_CLKID],    /* buffer position */
            MNT_ps_ClkDs->s_DefDs.s_clkId.ab_id,               /* id */
            k_CLKID_LEN);                               /* id length */
  /* get domain number */    
  ab_RespTlvPld[k_OFFS_DFDS_DOMAIN] = MNT_ps_ClkDs->s_DefDs.b_domn;
  /* reserved */
  ab_RespTlvPld[k_OFFS_DFDS_RES2] = 0;
  /* no failure occured */
  return TRUE;
} 
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_CurrentDs
**
** Description : This function handles the management message
**               CURRENT_DATA_SET.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_CurrentDs( void )
{
  /* get steps removed */
  GOE_hton16(&ab_RespTlvPld[k_OFFS_CUDS_STEPS],
             MNT_ps_ClkDs->s_CurDs.w_stepsRmvd);
  /* get offset from master */
  GOE_hton64(&ab_RespTlvPld[k_OFFS_CUDS_OFF2M],         
             (const UINT64*)
             &MNT_ps_ClkDs->s_CurDs.s_tiOffsFrmMst.ll_scld_Nsec); 
  /* get mean path delay */
  GOE_hton64(&ab_RespTlvPld[k_OFFS_CUDS_MPDELAY],         
             (const UINT64*)
             &MNT_ps_ClkDs->s_CurDs.s_tiMeanPathDel.ll_scld_Nsec);
  /* no failure occured */
  return TRUE;
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_ParentDs
**
** Description : This function handles the management message
**               PARENT_DATA_SET.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_ParentDs( void )
{
  /* get parent port id */
  PTP_BCOPY(&ab_RespTlvPld[k_OFFS_PADS_PID_CID],        /* buffer position */
            MNT_ps_ClkDs->s_PrntDs.s_prntPortId.s_clkId.ab_id,       /* id */
            k_CLKID_LEN);                                     /* id length */
  GOE_hton16(&ab_RespTlvPld[k_OFFS_PADS_PID_PNUM],
             MNT_ps_ClkDs->s_PrntDs.s_prntPortId.w_portNmb);
  /* get parent flags */
  ab_RespTlvPld[k_OFFS_PADS_FLAGS] = 0;  /* reset all flags */
  SET_FLAG(ab_RespTlvPld[k_OFFS_PADS_FLAGS],
           k_OFFS_PADS_FLAGS_PS,
           MNT_ps_ClkDs->s_PrntDs.o_prntStats);         
  /* reserved */
  ab_RespTlvPld[k_OFFS_PADS_RESERVED] = 0;
  /* get observed parent offset scaled log variance */ 
  GOE_hton16(&ab_RespTlvPld[k_OFFS_PADS_OPOSLV],
             MNT_ps_ClkDs->s_PrntDs.w_obsPrntOffsScldLogVar);
  /* get observed parent clock phase change rate */
  GOE_hton32(&ab_RespTlvPld[k_OFFS_PADS_OPCPCR],
             (UINT32)MNT_ps_ClkDs->s_PrntDs.l_obsPrntClkPhsChgRate);
  /* get grandmaster priority 1 */
  ab_RespTlvPld[k_OFFS_PADS_GMPRIO1] = MNT_ps_ClkDs->s_PrntDs.b_gmPrio1;
  /* get grandmaster clock quality */
  ab_RespTlvPld[k_OFFS_PADS_GM_QAL_CC] = 
             (UINT8)MNT_ps_ClkDs->s_PrntDs.s_gmClkQual.e_clkClass;
  ab_RespTlvPld[k_OFFS_PADS_GM_QAL_CA] = 
             (UINT8)MNT_ps_ClkDs->s_PrntDs.s_gmClkQual.e_clkAccur;
  GOE_hton16(&ab_RespTlvPld[k_OFFS_PADS_GM_QAL_OSLV],
             MNT_ps_ClkDs->s_PrntDs.s_gmClkQual.w_scldVar);
  /* get grandmaster priority 2 */
  ab_RespTlvPld[k_OFFS_PADS_GMPRIO2] = MNT_ps_ClkDs->s_PrntDs.b_gmPrio2;
  /* get grandmaster identity */
  PTP_BCOPY(&ab_RespTlvPld[k_OFFS_PADS_GMID],        /* buffer position */
            MNT_ps_ClkDs->s_PrntDs.s_gmClkId.ab_id,  /* id */
            k_CLKID_LEN);                            /* id length */
  /* no failure occured */
  return TRUE;
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_TimePropDs
**
** Description : This function handles the management message
**               TIME_PROPERTIES_DATA_SET.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_TimePropDs( void )
{
  UINT16 w_tmpFlags = 0;
  
  /* get current utc offset */
  GOE_hton16(&ab_RespTlvPld[k_OFFS_TIDS_UTCOFFS],
             (UINT16)MNT_ps_ClkDs->ps_rdTmPrptDs->i_curUtcOffs);
  /* get LI-61 flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_TIDS_FLAGS_LI61,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_leap_61); 
  /* get LI-59 flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_TIDS_FLAGS_LI59,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_leap_59); 
  /* get UTCV flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_TIDS_FLAGS_UTCV,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_curUtcOffsVal); 
  /* get PTP flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_TIDS_FLAGS_PTP,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_ptpTmScale); 
  /* get TTRA flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_TIDS_FLAGS_TTRA,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_tmTrcable); 
  /* get FTRA flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_TIDS_FLAGS_FTRA,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_frqTrcable); 
  /* write flags into buffer */
  ab_RespTlvPld[k_OFFS_TIDS_FLAGS]=(UINT8)w_tmpFlags;
  /* get time source */
  ab_RespTlvPld[k_OFFS_TIDS_TISRC]=(UINT8)MNT_ps_ClkDs->ps_rdTmPrptDs->e_tmSrc;
  /* no failure occured */
  return TRUE;
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_PortDs
**
** Description : This function handles the management message
**               PORT_DATA_SET.
**
** Parameters  : w_ifIdx      (IN) - port number as array index
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_PortDs( UINT16 w_ifIdx )
{
  /* get port identity */
  PTP_BCOPY(&ab_RespTlvPld[k_OFFS_PODS_PID_CID],               
            MNT_ps_ClkDs->as_PortDs[w_ifIdx].s_portId.s_clkId.ab_id,   
            k_CLKID_LEN);                                
  GOE_hton16(&ab_RespTlvPld[k_OFFS_PODS_PID_PNUM],
             MNT_ps_ClkDs->as_PortDs[w_ifIdx].s_portId.w_portNmb);
  /* get port state */
  ab_RespTlvPld[k_OFFS_PODS_POSTATE] = 
            (UINT8)MNT_ps_ClkDs->as_PortDs[w_ifIdx].e_portSts;
  /* get log min delay request interval */
  ab_RespTlvPld[k_OFFS_PODS_LMDRI] = 
            (UINT8)MNT_ps_ClkDs->as_PortDs[w_ifIdx].c_mMDlrqIntv;     
  /* get peer mean path delay */
  GOE_hton64(&ab_RespTlvPld[k_OFFS_PODS_PMPD],         
             (const UINT64*)
             &MNT_ps_ClkDs->as_PortDs[w_ifIdx].s_tiP2PMPDel.ll_scld_Nsec);
  /* get log announce interval */
  ab_RespTlvPld[k_OFFS_PODS_ANNCINV] = 
            (UINT8)MNT_ps_ClkDs->as_PortDs[w_ifIdx].c_AnncIntv;
  /* get accounce receipt timeout */
  ab_RespTlvPld[k_OFFS_PODS_ANNCTO] = 
            MNT_ps_ClkDs->as_PortDs[w_ifIdx].b_anncRecptTmo;
  /* get log sync interval */
  ab_RespTlvPld[k_OFFS_PODS_SYNCINV] = 
            (UINT8)MNT_ps_ClkDs->as_PortDs[w_ifIdx].c_SynIntv;
  /* get delay mechanism */
  ab_RespTlvPld[k_OFFS_PODS_DELMECH] = 
            (UINT8)MNT_ps_ClkDs->as_PortDs[w_ifIdx].e_delMech;
  /* get log min pdelay request interval */
  ab_RespTlvPld[k_OFFS_PODS_LMPRI] = 
            (UINT8)MNT_ps_ClkDs->as_PortDs[w_ifIdx].c_PdelReqIntv;
  /* get (reserved) and version number */
  ab_RespTlvPld[k_OFFS_PODS_VERSION] = 
            (MNT_ps_ClkDs->as_PortDs[w_ifIdx].b_verNumber & 0x0F);
  /* no failure occured */
  return TRUE;
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_Priority1
**
** Description : This function handles the management message
**               PRIORITY1.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_Priority1( void )
{
  /* get priority 1 */
  ab_RespTlvPld[k_OFFS_PRIO1_PRIO] = MNT_ps_ClkDs->s_DefDs.b_prio1;
  /* reserved */
  ab_RespTlvPld[k_OFFS_PRIO1_RES] = 0;
  /* no failure occured */
  return TRUE;
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_Priority2
**
** Description : This function handles the management message
**               PRIORITY2.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_Priority2( void )
{
  /* get priority 2 */
  ab_RespTlvPld[k_OFFS_PRIO2_PRIO] = MNT_ps_ClkDs->s_DefDs.b_prio2;
  /* reserved */
  ab_RespTlvPld[k_OFFS_PRIO2_RES] = 0;
  /* no failure occured */
  return TRUE; 
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_Domain
**
** Description : This function handles the management message
**               DOMAIN.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_Domain( void )
{
  /* get domain number */
  ab_RespTlvPld[k_OFFS_DOM_NUM] = MNT_ps_ClkDs->s_DefDs.b_domn;
  /* reserved */
  ab_RespTlvPld[k_OFFS_DOM_RES] = 0;
  /* no failure occured */
  return TRUE; 
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_SlaveOnly
**
** Description : This function handles the management message
**               SLAVE_ONLY.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_SlaveOnly( void )
{
  /* reset flags */
  ab_RespTlvPld[k_OFFS_SLVO_FLAGS] = 0;
  /* get slave only flag */
  SET_FLAG(ab_RespTlvPld[k_OFFS_SLVO_FLAGS],
           k_OFFS_SLVO_FLAGS_SO,
           MNT_ps_ClkDs->s_DefDs.o_slvOnly); 
  /* reserved */
  ab_RespTlvPld[k_OFFS_SLVO_RES] = 0;
  /* no failure occured */
  return TRUE;
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_AnncIntv
**
** Description : This function handles the management message
**               LOG_ANNOUNCE_INTERVAL.
**
** Parameters  : w_ifIdx      (IN) - port number as array index
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_AnncIntv( UINT16 w_ifIdx )
{
  /* get log announce interval */
  ab_RespTlvPld[k_OFFS_LAI_INTV] = 
         (UINT8)MNT_ps_ClkDs->as_PortDs[w_ifIdx].c_AnncIntv;
  /* reserved */
  ab_RespTlvPld[k_OFFS_LAI_RES] = 0;
  /* no failure occured */
  return TRUE; 
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_AnncRxTimeout
**
** Description : This function handles the management message
**               ANNOUNCE_RECEIPT_TIMEOUT.
**
** Parameters  : w_ifIdx      (IN) - port number as array index
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_AnncRxTimeout( UINT16 w_ifIdx )
{
  /* get log announce interval */
  ab_RespTlvPld[k_OFFS_ARTO_TO] = 
        MNT_ps_ClkDs->as_PortDs[w_ifIdx].b_anncRecptTmo;
  /* reserved */
  ab_RespTlvPld[k_OFFS_ARTO_RES] = 0;
  /* no failure occured */
  return TRUE; 
} 
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_SyncIntv
**
** Description : This function handles the management message
**               LOG_SYNC_INTERVAL.
**
** Parameters  : w_ifIdx      (IN) - port number as array index
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_SyncIntv( UINT16 w_ifIdx )
{
  /* get log announce interval */
  ab_RespTlvPld[k_OFFS_LSI_INTV] = 
         (UINT8)MNT_ps_ClkDs->as_PortDs[w_ifIdx].c_SynIntv;
  /* reserved */
  ab_RespTlvPld[k_OFFS_LSI_RES] = 0;
  /* no failure occured */
  return TRUE; 
} 
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_VersionNumber
**
** Description : This function handles the management message
**               VERSION_NUMBER.
**
** Parameters  : w_ifIdx      (IN) - port number as array index
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_VersionNumber( UINT16 w_ifIdx )
{
  /* get version number */
  ab_RespTlvPld[k_OFFS_VN_NUM] = 
       (MNT_ps_ClkDs->as_PortDs[w_ifIdx].b_verNumber & 0x0F);
  /* reserved */
  ab_RespTlvPld[k_OFFS_VN_RES] = 0;
  /* no failure occured */
  return TRUE; 
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_Time
**
** Description : This function handles the management message
**               TIME.
**
** Parameters  : pe_mntErr  (OUT) - mnt error id
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_Time( PTP_t_mntErrIdEnum *pe_mntErr )
{
  /* temporary timestamp */
  PTP_t_TmStmp s_ts;
  /* temporary return value */
  BOOLEAN      o_ret;

  /* request a current timestamp */
  if(CIF_GetTimeStamp(&s_ts) == TRUE)
  {
    /* get seconds */
    GOE_hton48(&ab_RespTlvPld[k_OFFS_TIME_SEC],&s_ts.u48_sec);
    /* get nanoseconds */
    GOE_hton32(&ab_RespTlvPld[k_OFFS_TIME_NSEC],s_ts.dw_Nsec);

    /* no failure occured */
    o_ret = TRUE;
  }
  else
  {
    /* could not read the clock - error */
    *pe_mntErr = e_MNTERR_GENERALERR;
    /* return failure */
    o_ret = FALSE;
  }
  return o_ret;
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_ClkAccuracy
**
** Description : This function handles the management message
**               CLOCK_ACCURACY. 
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_ClkAccuracy( void )
{
  /* get clock accuracy */
  ab_RespTlvPld[k_OFFS_CLKA_ACC] = 
       (UINT8)MNT_ps_ClkDs->s_DefDs.s_clkQual.e_clkAccur;
  /* reserved */
  ab_RespTlvPld[k_OFFS_CLKA_RES] = 0;
  /* no failure occured */
  return TRUE;
}
#endif /* if OC or BC == TRUE */  

/***********************************************************************
**
** Function    : MNTget_UtcProp
**
** Description : This function handles the management message
**               UTC_PROPERTIES.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_UtcProp( void )
{
  UINT16 w_tmpFlags = 0;

  /* get current utc offset */
  GOE_hton16(&ab_RespTlvPld[k_OFFS_UTC_OFFS],
             (UINT16)MNT_ps_ClkDs->ps_rdTmPrptDs->i_curUtcOffs);
  /* get LI-61 flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_UTC_FLAGS_LI61,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_leap_61); 
  /* get LI-59 flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_UTC_FLAGS_LI59,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_leap_59); 
  /* get UTCV flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_UTC_FLAGS_UTCV,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_curUtcOffsVal); 
  /* write flags into buffer */
  ab_RespTlvPld[k_OFFS_UTC_FLAGS] = (UINT8)w_tmpFlags;
  /* reserved */
  ab_RespTlvPld[k_OFFS_UTC_RES] = 0;
  /* no failure occured */
  return TRUE;
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_TraceProp
**
** Description : This function handles the management message
**               TRACEABILITY_PROPERTIES.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_TraceProp( void )
{
  UINT16 w_tmpFlags = 0;

  /* get TTRA flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_TRACE_FLAGS_TTRA,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_tmTrcable); 
  /* get FTRA flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_TRACE_FLAGS_FTRA,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_frqTrcable); 
  /* write flags into buffer */
  ab_RespTlvPld[k_OFFS_TRACE_FLAGS] = (UINT8)w_tmpFlags;
  /* reserved */
  ab_RespTlvPld[k_OFFS_TRACE_RES] = 0;
  /* no failure occured */
  return TRUE;
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_TimeSclProp
**
** Description : This function handles the management message
**               TIMESCALE_PROPERTIES.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
BOOLEAN MNTget_TimeSclProp( void )
{
  UINT16          w_tmpFlags = 0;

  /* get PTP flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_SCALE_FLAGS_PTP,
           MNT_ps_ClkDs->ps_rdTmPrptDs->o_ptpTmScale); 
  /* write flags into buffer */
  ab_RespTlvPld[k_OFFS_SCALE_FLAGS] = (UINT8)w_tmpFlags;
  /* get time source */
  ab_RespTlvPld[k_OFFS_SCALE_TISRC] = 
           (UINT8)MNT_ps_ClkDs->ps_rdTmPrptDs->e_tmSrc;
  /* no failure occured */
  return TRUE;
}
#endif /* if OC or BC == TRUE */

/***********************************************************************
**
** Function    : MNTget_UcNegoEnable
**
** Description : This function handles the management message 
**               UNICAST_NEGOTIATION_ENABLE.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN MNTget_UcNegoEnable( void )
{
  UINT16 w_tmpFlags = 0;

  /* get PTP flag */
  SET_FLAG(w_tmpFlags,
           k_OFFS_UCN_FLAGS_EN,
           k_UNICAST_CPBL); 
  /* write flags into buffer */
  ab_RespTlvPld[k_OFFS_UCN_FLAGS] = (UINT8)w_tmpFlags;
  /* reserved byte */
  ab_RespTlvPld[k_OFFS_UCN_RES] = 0;
  /* no failure occured */
  return TRUE;
}

/***********************************************************************
**
** Function    : MNTget_UCMasterTbl
**
** Description : This function handles the management message
**               UNICAST_MASTER_TABLE.
**
** Parameters  : pw_dynSize (OUT) - dynamic size of payload
**               pe_mntErr  (OUT) - mnt error id
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( k_UNICAST_CPBL == TRUE)
BOOLEAN MNTget_UCMasterTbl( UINT16             *pw_dynSize,
                            PTP_t_mntErrIdEnum *pe_mntErr )
{
  PTP_t_PortAddr *ps_ucPortAddr;
  UINT16         w_tblIdx;
  UINT16         w_dynSize = 0;
  UINT16         w_portSize = 0;

  /* get log query interval */
  ab_RespTlvPld[k_OFFS_UCMATBL_LQI] = 
       (UINT8)MNT_ps_ucMstTbl->c_logQryIntv;
  /* get table size */
  GOE_hton16(&ab_RespTlvPld[k_OFFS_UCMATBL_SIZE],
       MNT_ps_ucMstTbl->w_actTblSze);

  for(w_tblIdx = 0; w_tblIdx < MNT_ps_ucMstTbl->w_actTblSze; w_tblIdx++)
  {
    /* get next table entry */
    ps_ucPortAddr = MNT_ps_ucMstTbl->aps_pAddr[w_tblIdx];
    if(ps_ucPortAddr != NULL)
    {
      /* calculate port address size */
      w_portSize = 4 + ps_ucPortAddr->w_AddrLen;
      /* check for free buffer */
      if( (w_dynSize+w_portSize+k_OFFS_UCMATBL_TBL) < k_MAX_TLV_PLD_LEN )
      {
        /* get network protocol */
        GOE_hton16(&ab_RespTlvPld[k_OFFS_UCMATBL_TBL+w_dynSize],
                   (UINT16)ps_ucPortAddr->e_netwProt);
        /* get address length */
        GOE_hton16(&ab_RespTlvPld[k_OFFS_UCMATBL_TBL+w_dynSize+2],
                   ps_ucPortAddr->w_AddrLen);
        /* get address */
        PTP_BCOPY(&ab_RespTlvPld[k_OFFS_UCMATBL_TBL+w_dynSize+4],
                  ps_ucPortAddr->ab_Addr,   
                  ps_ucPortAddr->w_AddrLen);  
        /* update dynamic size */
        w_dynSize += w_portSize;
      }
      else
      {
        /* unicast master table too big for the message */
        *pe_mntErr = e_MNTERR_RES2BIG;
        /* could not generate RESPONSE */
        return FALSE;
      }

      /* check for padding bytes */
      if( (w_dynSize & 0x01) == 0x01 )
      {
         ab_RespTlvPld[k_OFFS_UCMATBL_TBL+w_dynSize] = 0;
         w_dynSize++;
      }
    }
  }
  /* return dynamic size */
  *pw_dynSize = w_dynSize;
  /* no failure occured */
  return TRUE;
}
#endif /* #if( k_UNICAST_CPBL == TRUE) */

/***********************************************************************
**
** Function    : MNTget_UCMstMaxTblSize
**
** Description : This function handles the management message
**               UNICAST_MASTER_MAX_TABLE_SIZE.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( k_UNICAST_CPBL == TRUE)
BOOLEAN MNTget_UCMstMaxTblSize( void )
{
  /* get max table size */
  GOE_hton16(&ab_RespTlvPld[k_OFFS_UCMA_SIZE],
             (UINT16)k_MAX_UC_MST_TBL_SZE);
  /* no failure occured */
  return TRUE;
}
#endif /* #if( k_UNICAST_CPBL == TRUE) */

/***********************************************************************
**
** Function    : MNTget_TCDefaultDs
**
** Description : This function handles the management message
**               TRANSPARENT_CLOCK_DEFAULT_DATA_SET.
**
** Parameters  : -
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( k_CLK_IS_TC == TRUE )
BOOLEAN MNTget_TCDefaultDs( void )
{
  /* get clock id */
  PTP_BCOPY(&ab_RespTlvPld[k_OFFS_TCDFDS_CID],      /* buffer position */
            MNT_ps_ClkDs->s_TcDefDs.s_clkId.ab_id,  /* id */
            k_CLKID_LEN);                           /* id length */
  /* get number of ports */
  GOE_hton16(&ab_RespTlvPld[k_OFFS_TCDFDS_PNUM],
             MNT_ps_ClkDs->s_TcDefDs.w_nmbPorts);
  /* get delay mechanism */
  ab_RespTlvPld[k_OFFS_TCDFDS_DELM] = 
            (UINT8)MNT_ps_ClkDs->s_TcDefDs.e_delMech;
  /* get primary domain */
  ab_RespTlvPld[k_OFFS_TCDFDS_PRIM] = MNT_ps_ClkDs->s_TcDefDs.b_primDom;
  /* no failure occured */
  return TRUE;
}
#endif /* if TC == TRUE ) */

/***********************************************************************
**
** Function    : MNTget_TCPortDs
**
** Description : This function handles the management message
**               TRANSPARENT_CLOCK_PORT_DATA_SET
**
** Parameters  : w_ifIdx      (IN) - port number as array index
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if( k_CLK_IS_TC == TRUE )
BOOLEAN MNTget_TCPortDs( UINT16 w_ifIdx )
{
  /* get port identity */
  PTP_BCOPY(&ab_RespTlvPld[k_OFFS_TCPODS_PID_CID],               
            MNT_ps_ClkDs->as_TcPortDs[w_ifIdx].s_pId.s_clkId.ab_id,   
            k_CLKID_LEN);                                
  GOE_hton16(&ab_RespTlvPld[k_OFFS_TCPODS_PID_PNUM],
             MNT_ps_ClkDs->as_TcPortDs[w_ifIdx].s_pId.w_portNmb);
  /* reset flag byte */
  ab_RespTlvPld[k_OFFS_TCPODS_FLAGS] = 0;
  /* get faulty flag */
  SET_FLAG(ab_RespTlvPld[k_OFFS_TCPODS_FLAGS],
           k_OFFS_TCPODS_FLAGS_FLT,
           MNT_ps_ClkDs->as_TcPortDs[w_ifIdx].o_flt); 
  /* get log min pdelay request interval */
  ab_RespTlvPld[k_OFFS_TCPODS_LMPDRI] = 
           (UINT8)MNT_ps_ClkDs->as_TcPortDs[w_ifIdx].c_logMPdelIntv;     
  /* get peer mean path delay */
  GOE_hton64(&ab_RespTlvPld[k_OFFS_TCPODS_PMPD],         
            (const UINT64*)
            &MNT_ps_ClkDs->as_TcPortDs[w_ifIdx].s_peerMPDel.ll_scld_Nsec); 
  /* no failure occured */
  return TRUE;
}
#endif /* if TC == TRUE ) */

/***********************************************************************
**
** Function    : MNTget_DelayMechanism
**
** Description : This function handles the management message
**               DELAY_MECHANISM.
**
** Parameters  : w_ifIdx      (IN) - port number as array index
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if((k_CLK_IS_OC == FALSE) || (k_CLK_IS_BC == FALSE) || (k_CLK_IS_TC == FALSE))
BOOLEAN MNTget_DelayMechanism( UINT16 w_ifIdx )
{
  /* avoid compiler warning */
  /* may be used in further versions */
  w_ifIdx = w_ifIdx;
#if( k_CLK_IS_TC == TRUE )
  /* get delay mechanism */
  ab_RespTlvPld[k_OFFS_DELM_MECH] = (
       UINT8)MNT_ps_ClkDs->s_TcDefDs.e_delMech;
#endif /* if TC == TRUE ) */
#if( k_CLK_IS_TC == FALSE )
  /* get delay mechanism */
  ab_RespTlvPld[k_OFFS_DELM_MECH] = 
       (UINT8)MNT_ps_ClkDs->as_PortDs[w_ifIdx].e_delMech;
#endif /* if TC == FALSE*/
  /* reserved */
  ab_RespTlvPld[k_OFFS_DELM_RES] = 0;
  /* no failure occured */
  return TRUE;
}
#endif /* if OC and BC and TC == FALSE */

/***********************************************************************
**
** Function    : MNTget_MinPDelReqIntv
**
** Description : This function handles the management message
**               LOG_MIN_PDELAY_REQ_INTERVAL.
**
** Parameters  : w_ifIdx      (IN) - port number as array index
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
#if((k_CLK_IS_OC == FALSE) || (k_CLK_IS_BC == FALSE) || (k_CLK_IS_TC == FALSE))
BOOLEAN MNTget_MinPDelReqIntv( UINT16 w_ifIdx )
{
#if( k_CLK_IS_TC == TRUE )
  /* get log announce interval */
  ab_RespTlvPld[k_OFFS_LMPDRI_INTV] = 
       (UINT8)MNT_ps_ClkDs->as_TcPortDs[w_ifIdx].c_logMPdelIntv;
#endif /* if TC == TRUE */
#if( k_CLK_IS_TC == FALSE )
  /* get log announce interval */
  ab_RespTlvPld[k_OFFS_LMPDRI_INTV] = 
       (UINT8)MNT_ps_ClkDs->as_PortDs[w_ifIdx].c_PdelReqIntv;
#endif
  /* reserved */
  ab_RespTlvPld[k_OFFS_LMPDRI_RES] = 0;
  /* no failure occured */
  return TRUE;
}
#endif /* if OC and BC and TC == FALSE */

/***********************************************************************
**
** Function    : MNTget_PrivateStatus
**
** Description : This function handles the Symmetricom private 
**               management message for status.
**
** Parameters  : pw_dynSize (OUT) - dynamic size of payload
**               pe_mntErr  (OUT) - mnt error id
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN MNTget_PrivateStatus( UINT16 *pw_dynSize,
                              PTP_t_mntErrIdEnum *pe_mntErr )
{
  if( SC_GetPrivateStatus( ab_RespTlvPld, pw_dynSize ) < 0 )
  {
    *pe_mntErr = e_MNTERR_WRONGVALUE;
    return FALSE;
  }
  return TRUE;
}


/*******************************************************************************
**    static functions
*******************************************************************************/


