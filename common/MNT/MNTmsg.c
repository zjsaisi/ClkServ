/*******************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
********************************************************************************
**
**       File: MNTmsg.c 
**    Summary: The MNTmsg.c is responsible to handle all management requests. 
**             GET, SET and COMMAND are separated into three functions. For 
**             each GET request a separate supporting function for TLV payload 
**             generation is realized within MNTget.c. SET and COMMAND requests 
**             are handled within each case, or local static functions are
**             used. If the MNT is not capable of setting the values, the 
**             management message has to be passed to the CTL. On return, the 
**             RESPONSE or ACKNOWLEDGE generation functions are called. There 
**             are several supporting functions to generate general error status 
**             messages as well as to check the incoming requests and their 
**             data values.
**
**    Version: 1.01.01
**
********************************************************************************
********************************************************************************
**
**  Functions: MNTmsg_HandleGet
**             MNTmsg_HandleSet
**             MNTmsg_HandleCom
**             MNTmsg_GenSetResp
**             MNTmsg_GenResponse
**             MNTmsg_GenAck
**             MNTmsg_GenStandMMsgErr
**             MNTmsg_SendMMErr
**             MNTmsg_SendMMsg
**             MNTmsg_InitFltRec
**             MNTmsg_PutFltRec
**
**             ChkLenEquals
**             ChkLenExceeds
**             Get_FaultLog
**             Cmd_SaveNVolStor
**             Cmd_RstNVolStor
**             GetRangeExcErrMsg
**
**   Compiler: Ansi-C
**    Remarks:  
**
********************************************************************************
**    all rights reserved, Template Version 1
*******************************************************************************/

/*******************************************************************************
**    compiler directives
*******************************************************************************/

/*******************************************************************************
**    include-files
*******************************************************************************/
#include "target.h"
#include "PTP/PTPdef.h"
#include "SIS/SIS.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "DIS/DIS.h"
#include "GOE/GOE.h"
#include "FIO/FIO.h"
#include "MNT/MNT.h"
#include "MNT/MNTapi.h"
#include "MNT/MNTint.h"

/*******************************************************************************
**    global variables
*******************************************************************************/

/*******************************************************************************
**    static constants, types, macros, variables
*******************************************************************************/
/* response or acknowledge tlv */
static PTP_t_MntTLV s_RespTlv;
/* fault log queue */
static MNT_t_FltRecItem as_fltLogQue[k_NUM_OF_FLT_REC];
/* put index of the fault log queue */
static UINT8 b_fltLogPutIdx;
/* amount of active fault record items */
static UINT8 b_fltLogCnt;

/*******************************************************************************
**    static function-prototypes
*******************************************************************************/
#define k_MAX_MNT_ERRSTR_LEN (50u) /* max of 50 characters of management error string */
/* static GET functions */
static BOOLEAN Get_FaultLog( UINT16             *pw_dynSize,
                             PTP_t_mntErrIdEnum *pe_mntErr );

/* static COMMAND functions */
static BOOLEAN Cmd_SaveNVolStor( void );
static BOOLEAN Cmd_RstNVolStor( void );

/* other static functions */
static BOOLEAN ChkLenEquals( const NIF_t_PTPV2_MntMsg *ps_mntMsg,
                                   UINT16             w_mntTblIdx );
static BOOLEAN ChkLenExceeds( const NIF_t_PTPV2_MntMsg *ps_mntMsg,
                                    UINT16             w_mntTblIdx,
                                    UINT16             *w_tlvPldLen );
static void GetRangeExcErrMsg(PTP_t_Text *ps_errStr,INT32 l_min,INT32 l_max);

/*******************************************************************************
**    global functions
*******************************************************************************/

/***********************************************************************
**
** Function    : MNTmsg_HandleGet
**
** Description : All GET requests are handled within this function.
**               Each supported GET is dispatched through the switch/
**               case defines to a corresponding function. Each GET 
**               function will return the TLV payload and if necessary 
**               the dynamic data size as well as an error. If not 
**               supported, a standardized error status TLV is generated.
**               Finally the RESPONSE message will be generated by 
**               calling the transmit function. This function will be 
**               also called after a successful SET request to generate 
**               a corresponding RESPONSE message. 
**
** Parameters  : ps_msg      (IN) - message box entry
**               ps_rxMntMsg (IN) - received mnt message
**               w_mntTblIdx (IN) - MNT table index of MNT ID
**               w_ifIdx     (IN) - portnumber (-1) to read 
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNTmsg_HandleGet(       PTP_t_MboxMsg      *ps_msg,
                       const NIF_t_PTPV2_MntMsg *ps_rxMntMsg,
                             UINT16             w_mntTblIdx,
                             UINT16             w_ifIdx )
{
  PTP_t_mntErrIdEnum e_mntErr  = e_MNTERR_NOTSUPPORT;
  BOOLEAN            o_noErr   = TRUE; /* no failure occured */
  UINT16             w_dynSize = 0;    /* default value zero bytes */

  /* check payload length = 2, if GET and accept SET */
  if( ((ps_rxMntMsg->s_mntTlv.w_len >= k_MNT_TLV_LEN_MIN) &&
       (ps_rxMntMsg->b_actField == k_ACTF_GET))
      || (ps_rxMntMsg->b_actField == k_ACTF_SET) )
  {
    switch (ps_rxMntMsg->s_mntTlv.w_mntId)
    {
      case k_NULL_MANAGEMENT:
      {
        /* there is nothing to do, except for generating a RESPONSE */
        break;
      }
      case k_CLOCK_DESCRIPTION:
      {
        o_noErr = MNTget_ClkDesc(w_ifIdx,&w_dynSize,&e_mntErr);
        break;
      }
      case k_USER_DESCRIPTION:
      {
        o_noErr = MNTget_UsrDesc(&w_dynSize);
        break;
      }
      case k_FAULT_LOG:
      {
        o_noErr = Get_FaultLog(&w_dynSize,&e_mntErr);
        break;
      }
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
      /* applicable to ordinary and boundary clocks */
      case k_DEFAULT_DATA_SET:
      {
        o_noErr = MNTget_DefaultDs();
        break;
      }
      case k_CURRENT_DATA_SET:
      {
        o_noErr = MNTget_CurrentDs();
        break;
      }
      case k_PARENT_DATA_SET:
      {
        o_noErr = MNTget_ParentDs();
        break;
      }
      case k_TIME_PROPERTIES_DATA_SET:
      {
        o_noErr = MNTget_TimePropDs();
        break;
      }
      case k_PORT_DATA_SET:
      {
        o_noErr = MNTget_PortDs(w_ifIdx);
        break;
      }
      case k_PRIORITY1:
      {
        o_noErr = MNTget_Priority1();
        break;
      }
      case k_PRIORITY2:
      {
        o_noErr = MNTget_Priority2();
        break;
      }
      case k_DOMAIN:
      {
        o_noErr = MNTget_Domain();
        break;
      }
      case k_SLAVE_ONLY:
      {
        o_noErr = MNTget_SlaveOnly();
        break;
      }
      case k_LOG_MEAN_ANNOUNCE_INTERVAL:
      {
        o_noErr = MNTget_AnncIntv(w_ifIdx);
        break;
      }
      case k_ANNOUNCE_RECEIPT_TIMEOUT:
      {
        o_noErr = MNTget_AnncRxTimeout(w_ifIdx);
        break;
      }
      case k_LOG_MEAN_SYNC_INTERVAL:
      {
        o_noErr = MNTget_SyncIntv(w_ifIdx);
        break;
      }
      case k_VERSION_NUMBER:
      {
        o_noErr = MNTget_VersionNumber(w_ifIdx);
        break;
      }
      case k_TIME:
      {
        o_noErr = MNTget_Time(&e_mntErr);
        break;
      }
      case k_CLOCK_ACCURACY:
      {
        o_noErr = MNTget_ClkAccuracy();
        break;
      }
      case k_UTC_PROPERTIES:
      {
        o_noErr = MNTget_UtcProp();
        break;
      }
      case k_TRACEABILITY_PROPERTIES:
      {
        o_noErr = MNTget_TraceProp();
        break;
      }
      case k_TIMESCALE_PROPERTIES:
      {
        o_noErr = MNTget_TimeSclProp();
        break;
      }
      /* only unicast master */
      case k_UNICAST_NEGOTIATION_ENABLE:
      {
        o_noErr = MNTget_UcNegoEnable();
        break;
      }
      /* case k_PATH_TRACE_LIST:
              not supported in our stack  */
      /* case k_PATH_TRACE_ENABLE:
              not supported in our stack  */
      /* case k_GRANDMASTER_CLUSTER_TABLE:
              not supported in our stack  */
#if( k_UNICAST_CPBL == TRUE )
      /* only unicast master */
      case k_UNICAST_MASTER_TABLE:
      {
        o_noErr = MNTget_UCMasterTbl(&w_dynSize,&e_mntErr);
        break;
      }
      case k_UNICAST_MASTER_MAX_TABLE_SIZE:
      {
        o_noErr = MNTget_UCMstMaxTblSize();
        break;
      }
#endif /* #if( k_UNICAST_CPBL == TRUE) */
      /* case k_ACCEPTABLE_MASTER_TABLE:
              not supported in this stack */
      /* case k_ACCEPTABLE_MASTER_TABLE_ENABLED:
              not supported in this stack */
      /* case k_ACCEPTABLE_MASTER_MAX_TABLE_SIZE:
              not supported in this stack */
      /* case k_ALTERNATE_MASTER:
              not supported in this stack */
      /* case k_ALTERNATE_TIME_OFFSET_ENABLE:
              not supported in this stack */
      /* case k_ALTERNATE_TIME_OFFSET_NAME:
              not supported in this stack */
      /* case k_ALTERNATE_TIME_OFFSET_MAX_KEY:
              not supported in this stack */
      /* case k_ALTERNATE_TIME_OFFSET_PROPERTIES:
              not supported in this stack */
#endif /* if OC and BC == TRUE */
#if( k_CLK_IS_TC == TRUE )
      /* applicable to transparent clocks */
      case k_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
      {
        o_noErr = MNTget_TCDefaultDs();
        break;
      }
      case k_TRANSPARENT_CLOCK_PORT_DATA_SET:
      {
        o_noErr = MNTget_TCPortDs(w_ifIdx);
        break;
      }
      /* case k_PRIMARY_DOMAIN:
              not supported in our stack */
#endif /* if TC == TRUE ) */
#if((k_CLK_IS_OC == FALSE) || (k_CLK_IS_BC == FALSE) || (k_CLK_IS_TC == FALSE))
      /* applicable to OCs, BCs and TCs */
      case k_DELAY_MECHANISM:
      {
        o_noErr = MNTget_DelayMechanism(w_ifIdx);
        break;
      }
      case k_LOG_MIN_MEAN_PDELAY_REQ_INTERVAL:
      {
        o_noErr = MNTget_MinPDelReqIntv(w_ifIdx);
        break;
      }
#endif /* if OC and BC and TC == FALSE */
      /* Symmetricom private management messages */
      case k_PRIVATE_STATUS:
      {
        o_noErr = MNTget_PrivateStatus(&w_dynSize,&e_mntErr);
        break;
      }
      /* not supported ids */
      default:
      {
        /* GET is not supported using this MNT ID */
        e_mntErr  = e_MNTERR_NOTSUPPORT;
        o_noErr = FALSE;
        break;
      }
    } /* switch (ps_rxMntMsg->s_mntTlv.w_mntId) */
  } /* if(ps_rxMntMsg->s_mntTlv.w_len == k_MNT_TLV_LEN_MIN) */
  else
  {
    /* GET is using TLV payload */
    e_mntErr  = e_MNTERR_WRONGLENGTH;
    o_noErr = FALSE;
  }

  /* generate RESPONSE */
  if( o_noErr == TRUE )
  {
    MNTmsg_GenResponse(ps_msg,w_mntTblIdx,w_dynSize);
  }
  /* GET is not supported using this MNT ID */
  else
  {
    /* generate error message */
    MNTmsg_GenStandMMsgErr(ps_msg,e_mntErr);
  }
  return;
}

/***********************************************************************
**
** Function    : MNTmsg_HandleSet
**
** Description : All SET requests are handled within this function. 
**               Each supported SET is dispatched through the switch/
**               case defines. If not supported, a standardized error 
**               status TLV is generated. All SET requests must be 
**               checked due to their TLV payload. Normally set data 
**               is transferred and a corrupted payload will lead to 
**               an error. All work is done within this function, or 
**               the management message has to be passed to the CTL 
**               task. Thus, the message is passed to the CTL message
**               box and will not be freed until the work is done 
**               within CTL. Return value does not indicate the 
**               success!
**
** Parameters  : ps_msg      (IN) - message box entry
**               ps_rxMntMsg (IN) - received mnt message
**               w_mntTblIdx (IN) - MNT table index of MNT ID
**
** Returnvalue : TRUE  - if allocated buffer has to be freed
**               FALSE - if allocated buffer has not to be freed
**
** Remarks     : Return value does not indicate the success!
**
***********************************************************************/
BOOLEAN MNTmsg_HandleSet(       PTP_t_MboxMsg      *ps_msg,
                          const NIF_t_PTPV2_MntMsg *ps_rxMntMsg,
                                UINT16              w_mntTblIdx)
{
  PTP_t_mntErrIdEnum  e_mntErr    = e_MNTERR_WRONGLENGTH;
  BOOLEAN             o_noErr     = TRUE;
  PTP_t_Text          s_errTxt    = {0,NULL};
  BOOLEAN             o_completed = FALSE;
  UINT16              w_tlvPldLen;
  const PTP_t_ProfRng *ps_prof = &MNT_ps_ClkDs->s_prof;
  /* temporary data buffers */
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
  PTP_t_tmSrcEnum s_tmSrcEnum;
#if( k_UNICAST_CPBL == TRUE)
  UINT16          w_numMasters;
  UINT16          w_dataSize;
  UINT16          w_tmpAddrLen;
#endif /* #if( k_UNICAST_CPBL == TRUE) */
#endif /* #if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) ) */

  switch (ps_rxMntMsg->s_mntTlv.w_mntId)
  {
    /* applicable to all node types */
    case k_NULL_MANAGEMENT:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      if(o_noErr == TRUE)
      {
        /* there is nothing to do */
        o_completed = TRUE;
      }
      break;
    }
    case k_USER_DESCRIPTION:
    {
      /* legnth check within function */
      o_noErr = ChkLenExceeds(ps_rxMntMsg,w_mntTblIdx,&w_tlvPldLen);
      if( o_noErr != FALSE )
      {
        /* check max length of string in TLV */
        if( ps_rxMntMsg->s_mntTlv.pb_data[k_OFFS_UD_SIZE] > k_MAX_USRDESC_SZE )
        {
          o_noErr = FALSE;
        }
        else
        {
          o_noErr = TRUE;
        }
      }
      break;
    }
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
    /* applicable to ordinary and boundary clocks */
    case k_PRIORITY1:
    {
      if((o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx )) == TRUE )
      {
        /* check priority with range */
        if( PTP_CHK_RNG(UINT8,
                        ps_rxMntMsg->s_mntTlv.pb_data[0],
                        ps_prof->ab_prio1[k_RNG_MIN],
                        ps_prof->ab_prio1[k_RNG_MAX]) 
                        == FALSE )/*lint !e731 */
        {
          /* value exceeds range */
          GetRangeExcErrMsg(&s_errTxt,
                            ps_prof->ab_prio1[k_RNG_MIN],/*lint !e713*/
                            ps_prof->ab_prio1[k_RNG_MAX]);/*lint !e713*/
          o_noErr = FALSE;
        }
      }
      break;
    }
    case k_PRIORITY2:
    {
      if((o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx )) == TRUE )
      {
        /* check priority with range */
        if( PTP_CHK_RNG(UINT8,
                        ps_rxMntMsg->s_mntTlv.pb_data[0],
                        ps_prof->ab_prio2[k_RNG_MIN],
                        ps_prof->ab_prio2[k_RNG_MAX]) 
                        == FALSE )/*lint !e731*/
        {
          /* value exceeds range */
          GetRangeExcErrMsg(&s_errTxt,
                            ps_prof->ab_prio2[k_RNG_MIN],/*lint !e713*/
                            ps_prof->ab_prio2[k_RNG_MAX]);/*lint !e713*/
          o_noErr = FALSE;
        }
      }
      break;
    }
    case k_DOMAIN:
    {
      /* if no length error */
      if((o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx )) == TRUE )
      {
        /* check priority with range */
        if( PTP_CHK_RNG(UINT8,
                        ps_rxMntMsg->s_mntTlv.pb_data[0],
                        ps_prof->ab_domn[k_RNG_MIN],
                        ps_prof->ab_domn[k_RNG_MAX]) == FALSE )/*lint !e731*/
        {
          /* value exceeds range */
          GetRangeExcErrMsg(&s_errTxt,
                            ps_prof->ab_domn[k_RNG_MIN],/*lint !e713*/
                            ps_prof->ab_domn[k_RNG_MAX]);/*lint !e713*/
          o_noErr = FALSE;
        }
      }
      break;
    }
    /* case k_SLAVE_ONLY: n
            not supported in our stack  */
    case k_LOG_MEAN_ANNOUNCE_INTERVAL:
    {
      if((o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx )) == TRUE )
      {
        /* check priority with range */
        if( PTP_CHK_RNG(INT8,
                        ps_rxMntMsg->s_mntTlv.pb_data[0],
                        ps_prof->ac_annIntRng[k_RNG_MIN],
                        ps_prof->ac_annIntRng[k_RNG_MAX]) 
                        == FALSE )/*lint !e731*/
        {
          /* value exceeds range */
          GetRangeExcErrMsg(&s_errTxt,
                            ps_prof->ac_annIntRng[k_RNG_MIN],
                            ps_prof->ac_annIntRng[k_RNG_MAX]);
          o_noErr = FALSE;
        }
      }
      break;
    }
    case k_ANNOUNCE_RECEIPT_TIMEOUT:
    {
      if((o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx )) == TRUE )
      {
        /* check priority with range */
        if( PTP_CHK_RNG(UINT8,
                        ps_rxMntMsg->s_mntTlv.pb_data[0],
                        ps_prof->ab_annRcpTmo[k_RNG_MIN],
                        ps_prof->ab_annRcpTmo[k_RNG_MAX]) 
                        == FALSE )/*lint !e731*/
        {
          /* value exceeds range */
          GetRangeExcErrMsg(&s_errTxt,
                            ps_prof->ab_annRcpTmo[k_RNG_MIN],/*lint !e713*/
                            ps_prof->ab_annRcpTmo[k_RNG_MAX]);/*lint !e713*/
          o_noErr = FALSE;
        }
      }
      break;
    }
    case k_LOG_MEAN_SYNC_INTERVAL:
    {
      if((o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx )) == TRUE )
      {
        /* check priority with range */
        if( PTP_CHK_RNG(INT8,
                        ps_rxMntMsg->s_mntTlv.pb_data[0],
                        ps_prof->ac_synIntRng[k_RNG_MIN],
                        ps_prof->ac_synIntRng[k_RNG_MAX]) 
                        == FALSE )/*lint !e731*/
        {
          /* value exceeds range */
          GetRangeExcErrMsg(&s_errTxt,
                            ps_prof->ac_synIntRng[k_RNG_MIN],
                            ps_prof->ac_synIntRng[k_RNG_MAX]);
          o_noErr = FALSE;
        }
      }
      break;
    }
    /* case k_VERSION_NUMBER:
            not supported in our stack  */
    case k_TIME:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      /* if no length error */
      if( o_noErr == TRUE )
      {  
        /* only the GM must be updated */
        if( MNT_ps_ClkDs->o_IsGM == FALSE )
        {
          /* no grandmaster -> not settable */
          o_noErr = FALSE;
          e_mntErr = e_MNTERR_NOTSETABLE;
        }
        /* else is handled by the CTL */
      }
      break;
    }
    case k_CLOCK_ACCURACY:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      /* if no length error */
      if( o_noErr == TRUE )
      {  
        /* only the GM must be updated */
        if( MNT_ps_ClkDs->o_IsGM == FALSE )
        {
          /* no grandmaster -> not settable */
          o_noErr = FALSE;
          e_mntErr = e_MNTERR_NOTSETABLE;
        }
        else
        {
          /* else is handled by the CTL, but values must be checked */
          if( ((ps_rxMntMsg->s_mntTlv.pb_data[0] >= (UINT8)e_ACC_25NS) &&
               (ps_rxMntMsg->s_mntTlv.pb_data[0] <= (UINT8)e_ACC_L10S)) ||
              (ps_rxMntMsg->s_mntTlv.pb_data[0] == (UINT8)e_ACC_UNKWN) )
          {
            /* no error, send it to CTL */
          }
          else
          {
            /* no grandmaster -> not settable */
            o_noErr = FALSE;
            e_mntErr = e_MNTERR_WRONGVALUE;
          }
        }       
      }
      break;
    }
    case k_UTC_PROPERTIES:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      break;
    }
    case k_TRACEABILITY_PROPERTIES:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      break;
    }
    case k_TIMESCALE_PROPERTIES:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      /* if no length error */
      if( o_noErr == TRUE )
      { 
        /* get timeSource */
        s_tmSrcEnum = (PTP_t_tmSrcEnum)ps_rxMntMsg->s_mntTlv.pb_data[1];
        /* if value compares not the following defines */
        if(!((s_tmSrcEnum == e_ATOMIC_CLK) ||
             (s_tmSrcEnum == e_GPS) ||
             (s_tmSrcEnum == e_TER_RADIO) ||
             (s_tmSrcEnum == e_PTP) ||
             (s_tmSrcEnum == e_NTP) ||
             (s_tmSrcEnum == e_HAND_SET) ||
             (s_tmSrcEnum == e_OTHER) ||
             (s_tmSrcEnum == e_INT_OSC)) )
        {
          /* wrong settable value for timesource */
          o_noErr = FALSE;
          e_mntErr = e_MNTERR_WRONGVALUE;
        }
      }
      break;
    }
#if( k_UNICAST_CPBL == TRUE)
    /* case k_UNICAST_NEGOTIATION_ENABLE:
            not supported in our stack */
#endif /* #if( k_UNICAST_CPBL == TRUE) */
    /* case k_PATH_TRACE_ENABLE:
            not supported in our stack */
    /* case k_GRANDMASTER_CLUSTER_TABLE:
            not supported in our stack */
#if( k_UNICAST_CPBL == TRUE)
    case k_UNICAST_MASTER_TABLE:
    {
      /* check data and legnth, get tlv payload length */
      o_noErr = ChkLenExceeds( ps_rxMntMsg, w_mntTblIdx, &w_tlvPldLen );
      /* unicast master table achieved, check size */
      if( (o_noErr == TRUE) && (w_tlvPldLen >= 3) )
      { 
        /* get table size (amount of new masters) */
        w_numMasters = 
          GOE_ntoh16(&ps_rxMntMsg->s_mntTlv.pb_data[k_OFFS_UCMATBL_SIZE]);
        /* check number of table entries */
        if( (w_numMasters <= k_MAX_UC_MST_TBL_SZE) )
        {
          /* calculate actual payload length */
          /* get static length */
          w_dataSize = s_MntInfoTbl[w_mntTblIdx].w_tlvPld;
          while( ((w_numMasters != 0) && (o_noErr == TRUE)) )
          {
            /* add network protocol size of 2 (Enum 16)*/
            w_dataSize += 2;
            /* get address length */
            w_tmpAddrLen = 
              GOE_ntoh16(&ps_rxMntMsg->s_mntTlv.pb_data[w_dataSize]);
            if( w_tmpAddrLen > k_MAX_NETW_ADDR_SZE )
            {
              o_noErr = FALSE;
            }
            /* add address length and 2 byte for the addr. length value */
            w_dataSize += ( 2 + w_tmpAddrLen );
            /* next entry */
            w_numMasters--;
          }
          /* add padding byte */
          if( (w_dataSize & 0x1) == 0x01 ) 
          {
            w_dataSize++;
          }
          /* corrupted data length */
          if( w_dataSize != w_tlvPldLen )
          {
            o_noErr = FALSE;
          }
        }
        /* set table exceeds our table size */
        else
        {
          o_noErr = FALSE;
        }
      }
      /* length is too short, no table entries or corrupted? */
      else
      {
        /* check data and legnth */
        o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
        /* SET with 0 entries means reset of the table */
        if( o_noErr == TRUE )
        {
          /* get table size (amount of new masters) */
          w_dataSize = 
            GOE_ntoh16(&ps_rxMntMsg->s_mntTlv.pb_data[k_OFFS_UCMATBL_SIZE]);
          /* wrong values */
          if( w_dataSize != 0 )
          {
            o_noErr = FALSE;
          }
        }
      }
      break;
    }
#endif /* #if( k_UNICAST_CPBL == TRUE) */
    /* case k_ACCEPTABLE_MASTER_TABLE:
            not supported in our stack */
    /* case k_ACCEPTABLE_MASTER_TABLE_ENABLED:
            not supported in our stack */
    /* case k_ALTERNATE_MASTER:
            not supported in our stack */
    /* case k_ALTERNATE_TIME_OFFSET_ENABLE:
            not supported in our stack */
    /* case k_ALTERNATE_TIME_OFFSET_NAME:
            not supported in our stack */
    /* case k_ALTERNATE_TIME_OFFSET_PROPERTIES:
            not supported in our stack */
#endif /* if OC and BC == TRUE */
#if( k_CLK_IS_TC == TRUE )
    /* applicable only to transparent clocks */
    /* case k_PRIMARY_DOMAIN:
            not supported in our stack */
#endif /* of TC == TRUE */
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) || (k_CLK_IS_TC == TRUE))
    /* applicable to ordinary, boundary and transparent clocks */
    /* case k_DELAY_MECHANISM:
            not supported in our stack */
    case k_LOG_MIN_MEAN_PDELAY_REQ_INTERVAL:
    {
      if((o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx )) == TRUE )
      {
        /* check priority with range */
        if( PTP_CHK_RNG(INT8,
                        ps_rxMntMsg->s_mntTlv.pb_data[0],
                        ps_prof->ac_delMIntRng[k_RNG_MIN],
                        ps_prof->ac_delMIntRng[k_RNG_MAX]) 
                        == FALSE )/*lint !e731*/
        {
          /* value exceeds range */
          GetRangeExcErrMsg(&s_errTxt,
                            ps_prof->ac_delMIntRng[k_RNG_MIN],
                            ps_prof->ac_delMIntRng[k_RNG_MAX]);
          o_noErr = FALSE;
        }
      }
      break;
    }
#endif /* if OC or BC, or TC == TRUE */

    /* all other IDs are not settable due to the 1588 standard */
    default:
    {
      /* SET is not supported using this MNT ID */
      o_noErr  = FALSE;
      e_mntErr = e_MNTERR_NOTSUPPORT;
      break;
    }
  }

  /* was MNT ID available? */
  if( o_noErr == TRUE )
  {
    /* work is done? */
    if ( o_completed == TRUE )
    {
      /* get new data and send RESPONSE */
      MNTmsg_GenSetResp( ps_msg, ps_rxMntMsg, w_mntTblIdx );
    }
    /* work is not done yet, must be handled by CTL */
    else
    {
      /* set mbox data */
      ps_msg->e_mType      = e_IMSG_MM_SET;
      ps_msg->s_etc2.w_u16 = w_mntTblIdx;
      /* pass message to CTL */
      if(SIS_MboxPut(CTL_TSK,ps_msg) == FALSE)
      {
        /* internal stack error, send error message */
        MNTmsg_GenStandMMsgErr(ps_msg,
                               e_MNTERR_GENERALERR);
      }
      /* Allocated buffer is transfered to another task and is
         released after reciving the response or upon a failure */
      return FALSE;
    }
  }
  /* an error occured */
  else
  {
    /* was an error string defined ? */
    if( s_errTxt.pc_text != NULL )
    {
      MNTmsg_SendMMErr(ps_msg,&s_errTxt,e_MNTERR_GENERALERR);
    }
    else
    {
      /* generate error message */
      MNTmsg_GenStandMMsgErr(ps_msg,e_mntErr);
    }
  }
  /* return TRUE to free allocated MNT message */
  return TRUE;
}

/***********************************************************************
**
** Function    : MNTmsg_HandleCom
**
** Description : All COMMAND requests are handled within this 
**               function. Each supported COMMAND is dispatched 
**               through the switch/case defines. If not supported,
**               a standardized error status TLV is generated. All 
**               work is done within this function, or the management
**               message has to be passed to the CTL task. Thus, the 
**               message is passed to the CTL message box and will 
**               not be freed until the work is done within CTL.
**
** Parameters  : ps_msg      (IN) - message box entry
**               ps_rxMntMsg (IN) - received mnt message
**               w_mntTblIdx (IN) - MNT table index of MNT ID
**
** Returnvalue : TRUE  - if allocated buffer has to be freed
**               FALSE - if allocated buffer has not to be freed
**
** Remarks     : Return value does not indicate the success!
**
***********************************************************************/
BOOLEAN MNTmsg_HandleCom(       PTP_t_MboxMsg      *ps_msg,
                          const NIF_t_PTPV2_MntMsg *ps_rxMntMsg,
                                UINT16              w_mntTblIdx)
{
  PTP_t_mntErrIdEnum e_mntErr    = e_MNTERR_WRONGLENGTH;
  BOOLEAN            o_noErr     = TRUE;
  BOOLEAN            o_completed = FALSE;

  /* temporary data buffers */
  MNT_t_initKeyEnum e_initKey;
  PTP_t_MboxMsg     s_mboxMsg;

  switch (ps_rxMntMsg->s_mntTlv.w_mntId)
  {
    case k_NULL_MANAGEMENT:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      if(o_noErr == TRUE)
      {
        /* there is nothing todo -> generate ACKNOWLEDGE */
        o_completed = TRUE;
      }
      break;
    }
    case k_SAVE_IN_NON_VOLATILE_STORAGE:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      if(o_noErr == TRUE)
      {
        o_completed = Cmd_SaveNVolStor();
      }
      break;
    }
    case k_RESET_NON_VOLATILE_STORAGE:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      if(o_noErr == TRUE)
      {
        o_completed = Cmd_RstNVolStor();
      }
      break;
    }
    case k_INITIALIZE:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      /* only, if payload ok */
      if( o_noErr == TRUE )
      {
        /* get initialization key */
        e_initKey = (MNT_t_initKeyEnum)
                        GOE_ntoh16((UINT8*)&ps_rxMntMsg->s_mntTlv.pb_data[0]);
        /* initialize OC or BC */
        if(e_initKey == e_INITIALZE_EVENT)
        {
          if( (k_CLK_IS_OC == TRUE) || 
              (k_CLK_IS_BC == TRUE)) /*lint !e506 !e774 */ 
          {
            /* set state to re-init */
            s_mboxMsg.e_mType = e_IMSG_INIT;

            /* send initizalization key to CTL to trigger initialization */
            if(SIS_MboxPut(CTL_TSK,&s_mboxMsg) == FALSE)
            {
              /* internal stack error, send error message */
              o_noErr  = FALSE;
              e_mntErr = e_MNTERR_GENERALERR;
            }
            else
            {
              /* work for MNT is done */
              o_completed = TRUE;
            }
          }
          /* no initialization in TCs */
          else
          {
            /* ony BCs and OCs could be initialized */
            o_noErr  = FALSE;
            e_mntErr = e_MNTERR_NOTSETABLE;
          }
        }
        else
        {
          /* wrong value */
          o_noErr  = FALSE;
          e_mntErr = e_MNTERR_WRONGVALUE;
        }
      }
      break;
    }
    case k_FAULT_LOG_RESET:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      if(o_noErr == TRUE)
      {
        /* to reset buffer we need only to reset index pointer */
        b_fltLogPutIdx = 0;
        b_fltLogCnt    = 0;
        /* work is done */
        o_completed = TRUE;
      }
      break;
    }
    case k_ENABLE_PORT:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      break;
    }
    case k_DISABLE_PORT:
    {
      o_noErr = ChkLenEquals( ps_rxMntMsg, w_mntTblIdx );
      break;
    }
    default:
    {
      /* COMMAND is not supported using this MNT ID */
      o_noErr  = FALSE;
      e_mntErr = e_MNTERR_NOTSUPPORT;
      break;
    }
  }

  /* generate RESPONSE */
  if( o_noErr == TRUE )
  {
    /* work is done? */
    if ( o_completed == TRUE )
    {
      MNTmsg_GenAck( ps_msg );
    }
    else
    {
      /* set mbox data */
      ps_msg->e_mType       = e_IMSG_MM_SET;
      ps_msg->s_etc2.w_u16 = w_mntTblIdx;
      /* pass message to CTL */
      if(SIS_MboxPut(CTL_TSK,ps_msg) == FALSE)
      {
        /* internal stack error, send error message */
        MNTmsg_GenStandMMsgErr(ps_msg,
                               e_MNTERR_GENERALERR);
      }
      /* Allocated buffer is transfered to another task and is
         released after reciving the response or upon a failure */
      return FALSE;
    }
  }
  /* CMD id not found or other error occured */
  else
  {
    /* generate error message */
    MNTmsg_GenStandMMsgErr(ps_msg,e_mntErr);
  }
  /* return TRUE to free allocated MNT message */
  return TRUE;
}

/***********************************************************************
**
** Function    : MNTmsg_GenSetResp
**
** Description : This function generates a management RESPONSE
**               message due to a received SET mnt message. This
**               is done by calling the MNTmsg_HandleGet() function
**               for each updated port. MNTmsg_GenSetResp() should
**               only be called, if no failure occurs!
**
** Parameters  : ps_msg      (IN) - mbox item
**               ps_mntMsg   (IN) - pointer to MNT msg of mbox
**               w_mntTblIdx (IN) - MNT table index of MNT ID
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNTmsg_GenSetResp(       PTP_t_MboxMsg      *ps_msg,
                        const NIF_t_PTPV2_MntMsg *ps_mntMsg,
                              UINT16              w_mntTblIdx )
{
  /* port definition and loop variables */
  UINT16 w_startIdx;
  UINT16 w_endIdx;
  UINT16 w_portIdx;

  /* get loop boundaries */
  MNT_GenPortLoopData(ps_msg, ps_mntMsg, w_mntTblIdx, &w_startIdx, &w_endIdx);
  /* send RESPONSE via MNTmsg_HandleGet() for each requestet port */
  for(w_portIdx = w_startIdx; w_portIdx < w_endIdx; w_portIdx++)
  {
    MNTmsg_HandleGet( ps_msg, ps_mntMsg, w_mntTblIdx, w_portIdx );
  }

  return;
}

/***********************************************************************
**
** Function    : MNTmsg_GenResponse
**
** Description : This function generates a management RESPONSE
**               message due to a received GET or SET mnt message.
**
** Parameters  : ps_msg        (IN) - Received message out of the
**                                    Message Box
**               w_mntTblIdx   (IN) - mnt msg info table index
**               w_dynDataSize (IN) - additional data size, if
**                                    dynamic members are used
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNTmsg_GenResponse( PTP_t_MboxMsg *ps_msg,
                         UINT16 w_mntTblIdx,
                         UINT16 w_dynDataSize )
{
  /* set response values of the TLV */
  s_RespTlv.w_type  = (UINT16)e_TLVT_MNT;
  /* calculate sum of length */
  s_RespTlv.w_len   = k_MNT_TLV_LEN_MIN 
                      + s_MntInfoTbl[w_mntTblIdx].w_tlvPld
                      + w_dynDataSize;
  s_RespTlv.w_mntId = s_MntInfoTbl[w_mntTblIdx].w_mntId;
  /* additional TLV payload available? */
  if(s_RespTlv.w_len <= 2)
  {
    s_RespTlv.pb_data = NULL; 
  }
  else
  {
    s_RespTlv.pb_data = ab_RespTlvPld;  
  }

  /* send RESPONSE message */
  MNTmsg_SendMMsg(ps_msg,
                  &s_RespTlv,
                  k_ACTF_RESP);
  return;
}

/***********************************************************************
**
** Function    : MNTmsg_GenAck
**
** Description : This function generates a management ACKNOWLEDGE 
**               message due to a received COMMAND message.
**
** Parameters  : ps_msg   (IN) - Received message out of the
**                               Message Box
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNTmsg_GenAck( PTP_t_MboxMsg *ps_msg )
{
  /* pointer to MNT message */
  NIF_t_PTPV2_MntMsg *ps_rxMntMsg; 
  /* port definition and loop variables */
  UINT16 w_startIdx;
  UINT16 w_endIdx;
  UINT16 w_portIdx;

  /* get received message */
  ps_rxMntMsg = ps_msg->s_pntData.ps_mnt;

  /* set response to requestor values */
  s_RespTlv.w_type  = (UINT16)e_TLVT_MNT;
  s_RespTlv.w_len   = k_MNT_TLV_LEN_MIN;
  s_RespTlv.w_mntId = ps_rxMntMsg->s_mntTlv.w_mntId;
  s_RespTlv.pb_data = NULL;    

  /* get loop boundaries */
  MNT_GenPortLoopData(ps_msg, 
                      ps_rxMntMsg, 
                      ps_msg->s_etc2.w_u16, 
                      &w_startIdx, 
                      &w_endIdx);

  /* send ACKNOWLEDGE for each requestet port */
  for(w_portIdx = w_startIdx; w_portIdx < w_endIdx; w_portIdx++)
  {
    /* send RESPONSE message */
    MNTmsg_SendMMsg(ps_msg,
                    &s_RespTlv,
                    k_ACTF_ACK);
  }
  return; /* end of message generation */
}

/*************************************************************************
**
** Function    : MNTmsg_GenStandMMsgErr
**
** Description : Within this function, a standardized management
**               error status TLV is generated and transmitted due
**               to the given error id. To generate individual error 
**               messages use the functions {MNT_GetPTPText()}
**               and {MNTmsg_SendMMErr()}.
**               
** See Also    : MNT_GetPTPText(), MNTmsg_SendMMErr()
**
** Parameters  : ps_msg   (IN) - mbox item with received mnt msg    
**               e_errId  (IN) - error msg id to define the error
**               
** Returnvalue : -        
** 
** Remarks     : -
**  
***********************************************************************/
void MNTmsg_GenStandMMsgErr(PTP_t_MboxMsg      *ps_msg, 
                            PTP_t_mntErrIdEnum e_errId)
{
  PTP_t_Text s_errText;

  switch(e_errId)
  {
    case e_MNTERR_RES2BIG:
    {
      /* generate displayData */
      MNT_GetPTPText("Could not fit into a single message", 
                         &s_errText);
      break;
    }
    case e_MNTERR_NOSUCHID:
    {
      /* generate displayData */
      MNT_GetPTPText("The managementId is not recognized", 
                         &s_errText);
      break;
    }
    case e_MNTERR_WRONGLENGTH:
    {
      /* generate displayData */
      MNT_GetPTPText("Length of the data was wrong", 
                         &s_errText);
      break;
    }
    case e_MNTERR_WRONGVALUE:
    {
      /* generate displayData */
      MNT_GetPTPText("One or more values were wrong", 
                         &s_errText);
      break;
    }
    case e_MNTERR_NOTSETABLE:
    {
      /* generate displayData */
      MNT_GetPTPText("Some variables are not configurable", 
                         &s_errText);
      break;
    }
    case e_MNTERR_NOTSUPPORT:
    {
      /* generate displayData */
      MNT_GetPTPText("Requested operation is not supported", 
                         &s_errText);
      break;
    }
    case e_MNTERR_GENERALERR:
    {
      /* generate displayData */
      MNT_GetPTPText("Undefined error occured. No request possible",
                         &s_errText);
      break;
    }
    default:
    {
      /* generate displayData */
      MNT_GetPTPText("Unknown error occured", 
                         &s_errText);
      e_errId = e_MNTERR_GENERALERR;
      break;
    }
  } /* end switch */

  /* send management error status */
  MNTmsg_SendMMErr(ps_msg, 
                   &s_errText, 
                   e_errId);
  return;
}

/***********************************************************************
**
** Function    : MNTmsg_SendMMErr
**
** Description : This function generates a management error tlv
**               and response this tlv to the requestor given in
**               ps_msg.
**
** Parameters  : ps_msg    (IN) - Received message out of the 
**                                Message Box
**               ps_text   (IN) - NULL, if no text available otherwise
**                                pointer to display error text
**               e_errType (IN) - management error id
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNTmsg_SendMMErr( PTP_t_MboxMsg      *ps_msg, 
                       PTP_t_Text         *ps_text,
                       PTP_t_mntErrIdEnum e_errType)
{
  /* temporary storage for message access */
  NIF_t_PTPV2_MntMsg *ps_rxMntMsg;  /* pointer to rx message */
  /* temporary storage for dynamic length values of tx message */
  UINT16             w_lenN = 0;    /* length of displayData */
  UINT16             w_lenM = 0;    /* length of padding data */
  /* temporary display text, if no other text available */
  PTP_t_Text         s_text;
  UINT8              b_cpLen;

  /* get rx management message and tlv */
  ps_rxMntMsg = ps_msg->s_pntData.ps_mnt;

  /* generate tx management status error message */

  /* convert management id to network order */
  GOE_hton16(&ab_RespTlvPld[0],ps_rxMntMsg->s_mntTlv.w_mntId);
  ab_RespTlvPld[2] = 0x00;
  ab_RespTlvPld[3] = 0x00;
  ab_RespTlvPld[4] = 0x00;
  ab_RespTlvPld[5] = 0x00;

  /* copy display data, if available */
  if(ps_text != NULL)
  {
    if( ps_text->b_len < k_MAX_MNT_ERRSTR_LEN )
    {
      b_cpLen = ps_text->b_len;
    }
    else
    {
      b_cpLen = k_MAX_MNT_ERRSTR_LEN;
    }
    /* copy displayData.lenght */
    ab_RespTlvPld[6] = b_cpLen;
    /* copy displayData */
    PTP_BCOPY(&ab_RespTlvPld[7],  /* displayData buffer position */
              ps_text->pc_text,   /* text */
              b_cpLen);    /* text length + lenght byte */
    /* update lenght N */
    w_lenN = b_cpLen + 1 ;

    /* a odd size of text displeyData must insert a padding byte */
    if((ps_text->b_len&1)==0)
    {
      ab_RespTlvPld[6+w_lenN] = 0;
      w_lenM = 1;
    }
  }
  else
  {
    /* create new display text */
    MNT_GetPTPText("Unknown error arised! Request was not executed!", 
                       &s_text);
    /* copy displayData.lenght */
    ab_RespTlvPld[6] = s_text.b_len;
    /* copy displayData */
    PTP_BCOPY(&ab_RespTlvPld[7], /* displayData buffer position */
              s_text.pc_text,    /* text */
              s_text.b_len);     /* text length + lenght byte */
    /* update lenght N */
    w_lenN = s_text.b_len + 1 ;

    /* a odd size of text displeyData must insert a padding byte */
    if((s_text.b_len&1)==0)
    {
      ab_RespTlvPld[6+w_lenN] = 0;
      w_lenM = 1;
    }
  }

  /* generate response to requestor */
  s_RespTlv.w_type  = (UINT16)e_TLVT_MNT_ERR_STS;
  s_RespTlv.w_len   = 8 + w_lenN + w_lenM;   
  s_RespTlv.w_mntId = (UINT16)e_errType;    /* due to error typ, error id */
  s_RespTlv.pb_data = ab_RespTlvPld;    

  /* if requestor sends a COMMAND message */
  if( ((ps_rxMntMsg->b_actField & 0x0F) == k_ACTF_CMD) )
  {
    MNTmsg_SendMMsg(ps_msg,
                    &s_RespTlv,
                    k_ACTF_ACK);
  }
  /* if requestor sends a GET or SET message */
  else
  {
    MNTmsg_SendMMsg(ps_msg,
                    &s_RespTlv,
                    k_ACTF_RESP);
  }
  return;
} /*lint !e818 */

/***********************************************************************
**
** Function    : MNTmsg_SendMMsg
**
** Description : This function is responsible for the transmission of
**               generated management messages and has to determine the
**               corresponding interface. Either a message must be
**               transmitted via the communication media like Ethernet
**               to another node or, if included with the define MNT_API,
**               must be passed to the management API. 
**
** Parameters  : ps_msg     (IN) - Received message out of the 
**                                 Message Box
**               ps_mntTLV  (IN) - direct pointer to received MNT TLV
**               b_actField (IN) - management action command
**                                 (GET, SET, COMMAND, RESPONSE or
**                                 ACKNOWLEDGE)
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNTmsg_SendMMsg( PTP_t_MboxMsg *ps_msg,
                      PTP_t_MntTLV  *ps_mntTLV,
                      UINT8         b_actField)
{
  static PTP_t_PortId s_txDstPid;
  NIF_t_PTPV2_MntMsg  *ps_rxMntMsg; 

  /* get original mnt message */
  ps_rxMntMsg = ps_msg->s_pntData.ps_mnt;
  /* set destination port identity */
  s_txDstPid = ps_rxMntMsg->s_ptpHead.s_srcPortId;

  /* if management request issued by another node */
  if(ps_msg->s_etc3.dw_u32 == k_MNT_MSG_EXT)
  {
    DIS_Send_MntMsg( ps_msg->s_pntExt2.ps_pAddr, 
                     &s_txDstPid,
                     ps_rxMntMsg->s_ptpHead.w_seqId,
                     ps_mntTLV,
                     ps_msg->s_etc1.w_u16,
                     ps_rxMntMsg->b_strtBndrHops - ps_rxMntMsg->b_bndrHops,
                     ps_rxMntMsg->b_strtBndrHops - ps_rxMntMsg->b_bndrHops,
                     b_actField );  /*lint !e826*/
  }
  /* if requested from the own node */
  else if(ps_msg->s_etc3.dw_u32 == k_MNT_MSG_INT)
  {
#ifdef MNT_API
    MNTapi_GatherAnswers( ps_msg, 
                          ps_msg->s_pntData.ps_mnt,
                          ps_mntTLV,       
                          0 );
    
#endif /* ifdef MNT_API */
#ifndef MNT_API
    /* Without MNT_API defined this code section should never be reached! */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_INTREQ,e_SEVC_WARN); 
#endif /* ifndef MNT_API */
  }
  return;
} /*lint !e818 */

/*************************************************************************
**
** Function    : MNTmsg_InitFltRec
**
** Description : This function initializes the fault log queue to
**               default values.
**
** Parameters  : -
**               
** Returnvalue : -        
** 
** Remarks     : -
**  
***********************************************************************/
void MNTmsg_InitFltRec( void )
{
  /* reset index pointer of the queue to the first item */
  b_fltLogPutIdx = 0;
  b_fltLogCnt    = 0;
  return;
}

/*************************************************************************
**
** Function    : MNTmsg_PutFltRec
**
** Description : This function is responsible to put a new fault record
**               into the fault record queue. This queue is realized as
**               a ring buffer and therefore old values are overwritten
**               by this function upon a buffer overflow. 
**
** Parameters  : ps_fltRecItem (IN) - input data to be saved into the
**                                    fault log queue
**               
** Returnvalue : -        
** 
** Remarks     : -
**  
***********************************************************************/
void MNTmsg_PutFltRec(const MNT_t_FltRecItem *ps_fltRecItem)
{
  /* copy data into queue */
  as_fltLogQue[b_fltLogPutIdx] = *ps_fltRecItem;
  /* move put index to the next position */
  b_fltLogPutIdx++;
  if(b_fltLogPutIdx == k_NUM_OF_FLT_REC)
  {
    b_fltLogPutIdx = 0;
  }

  if(b_fltLogCnt < k_NUM_OF_FLT_REC)
  {
    b_fltLogCnt++;
  }
  return;
} 

/*******************************************************************************
**    static functions
*******************************************************************************/

/*************************************************************************
**
** Function    : ChkLenEquals
**
** Description : This function determines, if TLV payload is available
**               and equals the expected length.
**
** Parameters  : ps_mntMsg     (IN) - pointer to received mnt message
**               w_mntTblIdx   (IN) - table index of MNT request table
**               
** Returnvalue : -        
** 
** Remarks     : -
**  
***********************************************************************/
static BOOLEAN ChkLenEquals( const NIF_t_PTPV2_MntMsg *ps_mntMsg,
                                   UINT16             w_mntTblIdx )
{
  /* preinit return value to FALSE */
  BOOLEAN o_ret = FALSE;

  /* if payload is expected */
  if( s_MntInfoTbl[w_mntTblIdx].w_tlvPld > 0 )
  {
    /* than TLV playload must not be NULL */
    if( ps_mntMsg->s_mntTlv.pb_data != NULL )
    {
      /* and playload length must equal expected length */
      if( (ps_mntMsg->s_mntTlv.w_len - 2) ==       
                      s_MntInfoTbl[w_mntTblIdx].w_tlvPld )
      {
        /* message is ok */
        o_ret = TRUE;
      }
    }
  }
  /* if no payload expected */
  else if( s_MntInfoTbl[w_mntTblIdx].w_tlvPld == 0 )
  {
    /*  playload length must equal expected length (=0) */
    if( (ps_mntMsg->s_mntTlv.w_len - 2) ==       
                    s_MntInfoTbl[w_mntTblIdx].w_tlvPld )
    {
      /* message is ok */
      o_ret = TRUE;
    }
  }
  return o_ret;
}

/*************************************************************************
**
** Function    : ChkLenExceeds
**
** Description : This function determines, if TLV payload is available
**               and equals the expected length.
**
** Parameters  : ps_mntMsg     (IN) - pointer to received mnt message
**               w_mntTblIdx   (IN) - table index of MNT request table
**               pw_tlvPldLen (OUT) - tlv payload length of received message  
**               
** Returnvalue : TRUE  - length is OK
**               FALSE - something corrupted
** 
** Remarks     : -
**  
***********************************************************************/
static BOOLEAN ChkLenExceeds( const NIF_t_PTPV2_MntMsg *ps_mntMsg,
                                    UINT16             w_mntTblIdx,
                                    UINT16             *pw_tlvPldLen )
{
  /* preinit return value to FALSE */
  BOOLEAN o_ret = FALSE;
  /* if payload is expected */
  if( (s_MntInfoTbl[w_mntTblIdx].w_tlvPld > 0) ||
      (s_MntInfoTbl[w_mntTblIdx].w_mntId == k_USER_DESCRIPTION) )
  {
    /* than TLV playload must not be NULL */
    if( ps_mntMsg->s_mntTlv.pb_data != NULL )
    {
      /* and playload length must exceed expected length */
      if( (ps_mntMsg->s_mntTlv.w_len - 2) >       
                      s_MntInfoTbl[w_mntTblIdx].w_tlvPld )
      {
        /* get tlv payload length */
        *pw_tlvPldLen = ps_mntMsg->s_mntTlv.w_len - 2;
        /* message is ok */
        o_ret = TRUE;
      }
    }
  }
  /* no payload expected, thus not used */
  return o_ret;
}

/***********************************************************************
**
** Function    : Get_FaultLog
**
** Description : This function handles the management message
**               FAULT_LOG. This management messages shall return
**               all recorded faults of the node. Allowed action
**               command is only GET.
**
** Parameters  : pw_dynSize (OUT) - dynamic size of fault log msg
**               pe_mntErr  (OUT) - fault enumeration defined by standard
**
** Returnvalue : TRUE  - release allocated buffer
**               FALSE - allocated buffer must not be released
**
** Remarks     : -
**
***********************************************************************/
static BOOLEAN Get_FaultLog( UINT16             *pw_dynSize,
                             PTP_t_mntErrIdEnum *pe_mntErr )
{
  PTP_t_Text s_text;
  UINT16     w_dynSize = 0;
  UINT16     w_faultSize = 0;
  UINT16     w_numOfRec = 0;
  UINT8      b_fltItemIdx = 0;

  for(b_fltItemIdx = 0; 
      ( (b_fltItemIdx < b_fltLogCnt) && (b_fltItemIdx < k_NUM_OF_FLT_REC) ); 
      b_fltItemIdx++)
  {
    /* get PTP text string of fault record */
    MNT_GetPTPText(PTP_GetErrStr(as_fltLogQue[b_fltItemIdx].dw_unitId,
                                  as_fltLogQue[b_fltItemIdx].dw_errNmb),
                    &s_text);

    /* size of fault log item including faultRecordLength */
    w_faultSize = 16 + s_text.b_len;

    if( (w_dynSize+w_faultSize+2) > k_MAX_TLV_PLD_LEN )
    {
      /* fault record is too big for a management TLV */
      *pe_mntErr = e_MNTERR_RES2BIG;
      /* return TRUE to free allocated MNT message */
      return FALSE;
    }
    else
    {
      /* set fault record size */
      /* we have to subtract 2 because lenght is not included */
      GOE_hton16(&ab_RespTlvPld[k_OFFS_FLTLOG_LOGS+w_dynSize],
                 w_faultSize-2);
      /* copy seconds */
      GOE_hton48(&ab_RespTlvPld[k_OFFS_FLTLOG_LOGS+w_dynSize+2],
        &as_fltLogQue[b_fltItemIdx].s_tsErr.u48_sec);
      /* copy nanoseconds */
      GOE_hton32(&ab_RespTlvPld[k_OFFS_FLTLOG_LOGS+w_dynSize+8],
        as_fltLogQue[b_fltItemIdx].s_tsErr.dw_Nsec);
      /* copy severity code */
      ab_RespTlvPld[k_OFFS_FLTLOG_LOGS+w_dynSize+12] = 
        (UINT8)as_fltLogQue[b_fltItemIdx].e_sevCode;
      /* copy fault name -> not used */
      ab_RespTlvPld[k_OFFS_FLTLOG_LOGS+w_dynSize+13] = 0;
      /* copy fault value -> not used */
      ab_RespTlvPld[k_OFFS_FLTLOG_LOGS+w_dynSize+14] = 0;
      /* copy faul description */
      ab_RespTlvPld[k_OFFS_FLTLOG_LOGS+w_dynSize+15] = 
        s_text.b_len;
      PTP_BCOPY(&ab_RespTlvPld[k_OFFS_FLTLOG_LOGS+w_dynSize+16], 
                s_text.pc_text,          
                s_text.b_len); 

      /* update dynamic size value */
      w_dynSize  = (UINT16)(w_dynSize + w_faultSize);

      /* update number of records within TLV */
      w_numOfRec++;
    }
  }

  /* add number of included records */
  GOE_hton16(&ab_RespTlvPld[k_OFFS_FLTLOG_NUM], w_numOfRec); 

  /* check for padding bytes */
  if( (w_dynSize & 0x01) == 0x01 )
  {
     ab_RespTlvPld[k_OFFS_FLTLOG_LOGS+w_dynSize] = 0;
     w_dynSize++;
  }
  /* return dynamic size */
  *pw_dynSize = w_dynSize;
  return TRUE;
}


/***********************************************************************
**
** Function    : Cmd_SaveNVolStor
**
** Description : This function handles the management message
**               SAVE_IN_NON_VOLATILE_STORAGE. This command shall
**               cause storage of all current values of the
**               applicable dynamic and configurable data set members
**               into non volatile storage. Allowed action command is
**               only COMMAND.
**
** Parameters  : -
**
** Returnvalue : TRUE  - release allocated buffer
**               FALSE - allocated buffer must not be released
**
** Remarks     : -
**
***********************************************************************/
static BOOLEAN Cmd_SaveNVolStor( void )
{
  PTP_t_cfgRom s_cfgRom;
  UINT16       w_ifIdx;  
  BOOLEAN      o_noErr = TRUE;

  /* initialize it with default values as TC */
#if( k_CLK_IS_TC == TRUE )
  /* values for TC default data set */
  s_cfgRom.s_tcDefDs.b_TcprimDom     = MNT_ps_ClkDs->s_TcDefDs.b_primDom;
  s_cfgRom.s_tcDefDs.e_TcDelMech     = MNT_ps_ClkDs->s_TcDefDs.e_delMech;
  /* values for transparent port data sets */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    s_cfgRom.as_pDs[w_ifIdx].c_pdelReqIntv  = 
      MNT_ps_ClkDs->as_TcPortDs[w_ifIdx].c_logMPdelIntv;
  }
#endif
  /* initialize it with default values as OC or BC */
#if((k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE))
  /* user description */
  PTP_BCOPY(s_cfgRom.ac_userDesc,
            MNT_ps_ClkDs->ac_userDesc,
            k_MAX_USRDESC_SZE);
  /* values for default data set */
  s_cfgRom.s_defDs.e_clkClass  = MNT_ps_ClkDs->s_DefDs.s_clkQual.e_clkClass;
    /* estimate scaled log variance on basis of timestamp granularity */
  s_cfgRom.s_defDs.w_scldVar   = MNT_ps_ClkDs->s_DefDs.s_clkQual.w_scldVar;
  s_cfgRom.s_defDs.b_prio1     = MNT_ps_ClkDs->s_DefDs.b_prio1;
  s_cfgRom.s_defDs.b_prio2     = MNT_ps_ClkDs->s_DefDs.b_prio2;
  s_cfgRom.s_defDs.b_domNmb    = MNT_ps_ClkDs->s_DefDs.b_domn;
  s_cfgRom.s_defDs.o_slaveOnly = MNT_ps_ClkDs->s_DefDs.o_slvOnly;
 
  /* values for port data sets */
  for( w_ifIdx = 0 ; w_ifIdx < k_NUM_IF ; w_ifIdx++ )
  {
    s_cfgRom.as_pDs[w_ifIdx].e_delMech =
      MNT_ps_ClkDs->as_PortDs[w_ifIdx].e_delMech;
    s_cfgRom.as_pDs[w_ifIdx].c_dlrqIntv     = 
      MNT_ps_ClkDs->as_PortDs[w_ifIdx].c_mMDlrqIntv;
    s_cfgRom.as_pDs[w_ifIdx].c_AnncIntv     = 
      MNT_ps_ClkDs->as_PortDs[w_ifIdx].c_AnncIntv;
    s_cfgRom.as_pDs[w_ifIdx].b_anncRcptTmo  = 
      MNT_ps_ClkDs->as_PortDs[w_ifIdx].b_anncRecptTmo;
    s_cfgRom.as_pDs[w_ifIdx].c_syncIntv     = 
      MNT_ps_ClkDs->as_PortDs[w_ifIdx].c_SynIntv;
    s_cfgRom.as_pDs[w_ifIdx].c_pdelReqIntv  = 
      MNT_ps_ClkDs->as_PortDs[w_ifIdx].c_PdelReqIntv;
  }
  /* values of profile range */
  s_cfgRom.s_prof = MNT_ps_ClkDs->s_prof;
#endif /* if((k_CLK_IS_OC == TRUE) || ( (k_CLK_IS_BC == TRUE)) */
  /* write it in the configuration file */
  if(FIO_WriteConfig(&s_cfgRom) != TRUE)
  {
    o_noErr = FALSE;
  }
#if( k_UNICAST_CPBL == TRUE )
  /* write unicast master table */
  if(FIO_WriteUcMstTbl(MNT_ps_ucMstTbl) != TRUE)
  {
    o_noErr = FALSE;
  }        
#endif
  return o_noErr;
}

/***********************************************************************
**
** Function    : Cmd_RstNVolStor
**
** Description : This function handles the management message
**               RESET_NON_VOLATILE_STORAGE. This command shall
**               cause reset of dynamic and configurable data set
**               members of non volatile storage to initialization
**               default values. Allowed action command is only
**               COMMAND.
**
** Parameters  : -
**
** Returnvalue : TRUE  - release allocated buffer
**               FALSE - allocated buffer must not be released
**
** Remarks     : -
**
***********************************************************************/
static BOOLEAN Cmd_RstNVolStor( void )
{
  PTP_t_cfgRom s_cfgRom;

  /* initialize with default values */
  FIO_InitCfgDflt(&s_cfgRom);
  /* write it in the configuration file and return */
  return (FIO_WriteConfig(&s_cfgRom));
}

/***********************************************************************
**  
** Function    : GetRangeExcErrMsg
**  
** Description : Generates an error text structure for range
**               exceed error with the valid range.
**  
** Parameters  : ps_errStr (OUT) - assemble error message
**               l_min      (IN) - range minimum value
**               l_max      (IN) - range maximum value
**               
** Returnvalue : -
** 
** Remarks     : not reentrant !
**  
***********************************************************************/
static void GetRangeExcErrMsg(PTP_t_Text *ps_errStr,INT32 l_min,INT32 l_max)
{
  static CHAR ac_errStr[100];
  /* assemble range exceed error string */
  PTP_SPRINTF((ac_errStr,"Value exceeds range (min= %i - max=%i)",l_min,l_max));
  /* assemble text structure */
  MNT_GetPTPText(ac_errStr,ps_errStr);
}


