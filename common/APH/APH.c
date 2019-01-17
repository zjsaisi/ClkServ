/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: APH.c  
**    Summary: Hook Unit APH
**             The hook unit is an additional code unit, normally not used by
**             the standard stack. The hooks are used for testing. 
**             They provide the possibility to change parts of the network 
**             packets to test the reaction of other nodes and/or the 
**             complete network. 
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**             
**  Functions: APH_Init
**             APH_Hook_Send_All
**             APH_RegSndHook
**             APH_UnregSndHook
**             APH_UnregAllSndHook
**             APH_Hook_Snd
**             APH_Hook_SndTsSeq
**             APH_Hook_SndTsVers
**             APH_RegSendMsgHook
**             APH_UnregSendMsgHook
**             APH_Hook_Send
**             APH_CheckTxTs
**             APH_SetToOneStep
**             APH_ResetToTwoStep
**             APH_OneStepHook
**             APH_RegRxTsHookCb
**             APH_UnregRxTsHook
**             APH_Hook_RxTs
**             APH_RegTxTsHookCb
**             APH_UnregTxTsHook
**             APH_Hook_TxTs
**             APH_RegRecvMsgHook
**             APH_UnregRecvMsgHook
**             APH_Hook_Recv
**
**             ResetSndHookTable
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
#include "GOE/GOE.h"
#include "CTL/CTLint.h" /* access to clock data set */
#include "APH/APH.h"
#include "APH/APHint.h"

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/* send-hook table */
APH_t_SndHookCmd as_sndHookCmds[k_MAX_SND_HOOK_CMDS];

/* rx timestamp hook callback function */
APH_t_pfCbRxTs    APH_pf_cbRxTs = NULL;
/* tx timestamp hook callback function */
APH_t_pfCbTxTs    APH_pf_cbTxTs = NULL;
/* rx message hook callback function */
APH_t_pfCbRxMsg   APH_pf_cbRxMsg  = NULL;
/* tx message hook callback function */
APH_t_pfCbTxMsg   APH_pf_cbTxMsg = NULL;

/* TX-message timestamp destroy flag */
BOOLEAN o_tsDestrFlag;
/* one-step functionality flag */
BOOLEAN o_oneStep;

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static void ResetSndHookTable( void );

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**  
** Function    : APH_Init
**  
** Description : Initializes the unit APH. 
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_Init( void )
{
  /* reset send-hook table */
  ResetSndHookTable();
  /* init Rx timestamp hook function */
  APH_pf_cbRxTs  = NULL;
  /* init Tx timestamp hook function */
  APH_pf_cbTxTs  = NULL;  
  /* init Rx message callback function */
  APH_pf_cbRxMsg = NULL;
  /* init Tx message callback function */
  APH_pf_cbTxMsg = NULL;
  /* init TS-destroy-flag */
  o_tsDestrFlag  = FALSE;
  /* one-step functionality flag */
  o_oneStep      = FALSE;
}

/***********************************************************************
**  
** Function    : APH_Hook_Send_All
**  
** Description : Calls all send-hook functions after each other.
**               First, the one-step-hook is called, then the 
**               hook-function to add or change defined values is called
**               and least the send-callback function is used.
**
**    See Also : APH_OneStepHook(), APH_Hook_Snd(), APH_Hook_Send()
**  
** Parameters  : pb_msg    (IN) - message in network format
**               w_len     (IN) - length of message
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
const UINT8* APH_Hook_Send_All( const UINT8* pb_msg,UINT16 w_len)
{
  const UINT8 *pb_hookedBuf;
  /* call one-step-hook function */
  pb_hookedBuf = APH_OneStepHook(pb_msg,w_len);
  if( pb_hookedBuf != NULL )
  {
    pb_hookedBuf = APH_Hook_Snd(NIF_UNPACKHDR_MSGTYPE(pb_hookedBuf),
                                pb_hookedBuf,w_len);
    /* pb_hookedBuf cannot be NULL after this hook */
    pb_hookedBuf = APH_Hook_Send(pb_hookedBuf,&w_len);
  }
  return pb_hookedBuf;
}  

/***********************************************************************
**  
** Function    : APH_RegSndHook
**  
** Description : Registers a send hook. The returned value is a handle
**               to the hook which can be used to stop it. It is possible 
**               to register more than one hook at one time. Use the 
**               unregister function to disable the registered hook.  
**               To unregister use the returned handle.
**
**    See Also : APH_UnregSndHook()
**  
** Parameters  : b_cmd     (IN) - command to be done in hook 
**                                (k_SNDHOOK_SET,k_SNDHOOK_ADD)
**               e_msgType (IN) - message type to be changed in hook 
**                                (PTP_t_msgTypeEnum)
**               ddw_val   (IN) - value to set or add in the hook function
**               ps_srcPId (IN) - value of source port Id to set (shall be
**                                NULL if port ID shall not be changed)
**               b_offs    (IN) - offset of value to set or add
**               b_valLen  (IN) - size of value to add or set
**               
** Returnvalue :  < 0 - function failed
**               >= 0 - function succeeded - number of hook
** 
** Remarks     : -
**  
***********************************************************************/
INT32 APH_RegSndHook( UINT8 b_cmd,
                      PTP_t_msgTypeEnum e_msgType,
                      UINT64 ddw_val,
                      const PTP_t_PortId *ps_srcPId,
                      UINT8 b_offs,
                      UINT8 b_valLen) 
{
  INT32 l_res = -1;  
  UINT16 w_hook;

  /* search for free table entry */
  for( w_hook = 0 ; w_hook < k_MAX_SND_HOOK_CMDS ; w_hook++ )
  {
    if( as_sndHookCmds[w_hook].o_hookIsSet == FALSE )
    {
      /* set the values */
      as_sndHookCmds[w_hook].b_cmd     = b_cmd;
      as_sndHookCmds[w_hook].e_msgType = e_msgType;
      as_sndHookCmds[w_hook].ddw_val   = ddw_val;
      if( ps_srcPId == NULL )
      {
        as_sndHookCmds[w_hook].o_srcPIDToSet = FALSE;
      }
      else
      {
        as_sndHookCmds[w_hook].o_srcPIDToSet = TRUE;
        as_sndHookCmds[w_hook].s_srcPId = *ps_srcPId;
      }
      as_sndHookCmds[w_hook].b_offs    = b_offs;
      as_sndHookCmds[w_hook].b_valLen  = b_valLen;
      /* set return handle to this hook */
      l_res = (INT32)w_hook;
      /* get flag that determines, if seqId gets changed */
      if( b_offs == k_OFFS_SEQ_ID )
      {
        as_sndHookCmds[w_hook].o_chgsSeqId = TRUE;
      }
      else
      {
        as_sndHookCmds[w_hook].o_chgsSeqId = FALSE;
      }
      /* get flag that determines, if versionnumber gets changed */
      if( b_offs == k_OFFS_VER_PTP )
      {
        as_sndHookCmds[w_hook].o_chgsVers = TRUE;
      }
      else
      {
        as_sndHookCmds[w_hook].o_chgsVers = FALSE;
      }
      /* mark table entry to be not free */
      as_sndHookCmds[w_hook].o_hookIsSet = TRUE; 
      /* stop the loop */
      break;
    }
  }
  return l_res;
}

/***********************************************************************
**  
** Function    : APH_UnregSndHook
**  
** Description : Unregisters a send hook. To unregister, use the 
**               returned handle of the register function.
**
**    See Also : APH_RegSndHook()
**  
** Parameters  : dw_hdl (IN) - handle of send hook to unregister
**               
** Returnvalue : TRUE        - function succeeded
**               FALSE       - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN APH_UnregSndHook(UINT32 dw_hdl)
{
  if( dw_hdl >= k_MAX_SND_HOOK_CMDS )
  {
    return FALSE;
  }
  else
  {
    as_sndHookCmds[dw_hdl].o_hookIsSet = FALSE;
    return TRUE;
  }
}

/***********************************************************************
**  
** Function    : APH_UnregAllSndHook
**  
** Description : Unregisters all send hooks. 
**
**    See Also : APH_RegSndHook(),APH_UnregSndHook()
**
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_UnregAllSndHook( void )
{
  ResetSndHookTable();
}

/***********************************************************************
**  
** Function    : APH_Hook_Snd
**  
** Description : Send Hook that changes the message according to the 
**               registered hooks. 
**  
** Parameters  : e_msgType (IN) - message type of hooked message to send
**               pb_msg    (IN) - message in network format
**               w_len     (IN) - length of message
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
UINT8* APH_Hook_Snd( PTP_t_msgTypeEnum e_msgType,
                     const UINT8* pb_msg,UINT16 w_len)
{
  static UINT8 ab_cpy[1536];
  UINT64 ddw_valToSet = 0;
  UINT16 w_hook;
  
  /* copy message */
  PTP_BCOPY(ab_cpy,pb_msg,w_len);
  /* search hook table */
  for( w_hook = 0 ; w_hook < k_MAX_SND_HOOK_CMDS ; w_hook++ )
  {
    /* is this hook to be used ? */
    if( (as_sndHookCmds[w_hook].o_hookIsSet == TRUE) &&
        (as_sndHookCmds[w_hook].e_msgType   == e_msgType))
    {
      /* port id to change ? */
      if( as_sndHookCmds[w_hook].o_srcPIDToSet == TRUE  )
      {
        /* change source port ID of message */
        PTP_BCOPY(&ab_cpy[k_OFFS_SRCPRTID],
                  as_sndHookCmds[w_hook].s_srcPId.s_clkId.ab_id,
                  k_CLKID_LEN);
        GOE_hton16(&ab_cpy[k_OFFS_SRCPRTID+k_CLKID_LEN],
                  as_sndHookCmds[w_hook].s_srcPId.w_portNmb);
      }
      else
      {
        if( w_len < (as_sndHookCmds[w_hook].b_offs  +
                     as_sndHookCmds[w_hook].b_valLen))
        {
          /* do nothing - out of message range */
          return ab_cpy;
        }
        if( as_sndHookCmds[w_hook].b_cmd == k_SNDHOOK_SET )
        {
          ddw_valToSet = as_sndHookCmds[w_hook].ddw_val;
        }
        /* add command - first get old value in field */
        else
        {
          switch( as_sndHookCmds[w_hook].b_valLen )
          {
            case 1:
            {
              ddw_valToSet = ab_cpy[as_sndHookCmds[w_hook].b_offs];
              break;
            }
            case 2:
            {
              ddw_valToSet = GOE_ntoh16(&ab_cpy[as_sndHookCmds[w_hook].b_offs]);
              break;
            }
            case 4:
            {
              ddw_valToSet = GOE_ntoh32(&ab_cpy[as_sndHookCmds[w_hook].b_offs]);
              break;
            }
            case 6:
            {
              ddw_valToSet = GOE_ntoh48(&ab_cpy[as_sndHookCmds[w_hook].b_offs]);
              break;
            }
            case 8:
            {
              ddw_valToSet = GOE_ntoh64(&ab_cpy[as_sndHookCmds[w_hook].b_offs]);
              break;
            }
            default:
            {
              PTP_BCOPY(&ddw_valToSet,
                        &ab_cpy[as_sndHookCmds[w_hook].b_offs],
                        as_sndHookCmds[w_hook].b_valLen);
              break;
            }
          }
          /* add value */
          ddw_valToSet += as_sndHookCmds[w_hook].ddw_val;
        }        
        /* write value */
        switch( as_sndHookCmds[w_hook].b_valLen )
        {
          case 1:
          {
            ab_cpy[as_sndHookCmds[w_hook].b_offs] = (UINT8) ddw_valToSet;
            break;
          }
          case 2:
          {
            GOE_hton16(&ab_cpy[as_sndHookCmds[w_hook].b_offs],
                       (UINT16)ddw_valToSet);
            break;
          }
          case 4:
          {
            GOE_hton32(&ab_cpy[as_sndHookCmds[w_hook].b_offs],
                       (UINT32)ddw_valToSet);
            break;
          }
          case 6:
          {
            GOE_hton48(&ab_cpy[as_sndHookCmds[w_hook].b_offs],
                       &ddw_valToSet);
            break;
          }
          case 8:
          {
            GOE_hton64(&ab_cpy[as_sndHookCmds[w_hook].b_offs],
                       &ddw_valToSet);
            break;
          }
          default:
          {
            PTP_BCOPY(&ab_cpy[as_sndHookCmds[w_hook].b_offs],
                      (UINT8*)&ddw_valToSet,/*lint !e740*/
                      as_sndHookCmds[w_hook].b_valLen);
            break;
          }
        }
      }
    }
  }
  return ab_cpy;
}

/***********************************************************************
**  
** Function    : APH_Hook_SndTsSeq
**  
** Description : Returns changed sequence id to get the correct timestamp.
**  
** Parameters  : e_msgType (IN) - message type of hooked message to send
**               w_seqId   (IN) - original sequence id
**               
** Returnvalue : sequence id before changing in hook
** 
** Remarks     : -
**  
***********************************************************************/
UINT16 APH_Hook_SndTsSeq( PTP_t_msgTypeEnum e_msgType,UINT16 w_seqId)
{
  UINT16 w_hook,w_retSeqId = w_seqId;
  /* search hook table */
  for( w_hook = 0 ; w_hook < k_MAX_SND_HOOK_CMDS ; w_hook++ )
  {
    /* is this hook to be used ? */
    if( (as_sndHookCmds[w_hook].o_hookIsSet == TRUE) &&
        (as_sndHookCmds[w_hook].e_msgType   == e_msgType) &&
        (as_sndHookCmds[w_hook].o_chgsSeqId == TRUE))
    {
      /* set or add ? */
      if( as_sndHookCmds[w_hook].b_cmd == k_SNDHOOK_SET )
      {
        w_retSeqId = (UINT16)as_sndHookCmds[w_hook].ddw_val;
      }
      else
      {
        w_retSeqId = (UINT16)(w_seqId + (UINT16)as_sndHookCmds[w_hook].ddw_val);
      }
    }
  }
  return w_retSeqId;
}

/***********************************************************************
**  
** Function    : APH_Hook_SndTsVers
**  
** Description : Returns changed version number to get the correct timestamp.
**  
** Parameters  : e_msgType (IN) - message type of hooked message to send
**               
** Returnvalue : version after changing in hook
** 
** Remarks     : -
**  
***********************************************************************/
UINT8 APH_Hook_SndTsVers( PTP_t_msgTypeEnum e_msgType)
{
  UINT16 w_hook;
  UINT8  b_retVers = k_PTP_VERSION;
  /* search hook table */
  for( w_hook = 0 ; w_hook < k_MAX_SND_HOOK_CMDS ; w_hook++ )
  {
    /* is this hook to be used ? */
    if( (as_sndHookCmds[w_hook].o_hookIsSet == TRUE) &&
        (as_sndHookCmds[w_hook].e_msgType   == e_msgType) &&
        (as_sndHookCmds[w_hook].o_chgsVers  == TRUE))
    {
      /* set or add ? */
      if( as_sndHookCmds[w_hook].b_cmd == k_SNDHOOK_SET )
      {
        b_retVers = (UINT8)as_sndHookCmds[w_hook].ddw_val;
      }
      else
      {
        b_retVers = (UINT8)(k_PTP_VERSION + 
                           (UINT16)as_sndHookCmds[w_hook].ddw_val);            
      }
    }
  }
  return b_retVers;  
}

/***********************************************************************
**  
** Function    : APH_RegSendMsgHook
**  
** Description : Registers a callback function that hooks the 
**               Tx Messages to the application. 
**               Only one callback function can be registered.
**               It is possible to 
**                 - read the message 
**                 - change the message content
**                 - change the message length
**                 - destroy the tx message (by returning FALSE)
**               in the callback function.
**               ATTENTION: The sequence id of event messages shall not 
**               be changed because it is not possible to get the correct 
**               tx timestamps afterwards. 
**               Use function APH_RegSndHook to do that.
**
**    See Also : APH_UnregSendMsgHook()
**  
** Parameters  : pf_cbTxMsg (IN) - Tx message callback function to register
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_RegSendMsgHook( APH_t_pfCbTxMsg pf_cbTxMsg)
{
  APH_pf_cbTxMsg = pf_cbTxMsg;
  return;
}

/***********************************************************************
**  
** Function    : APH_UnregSendMsgHook
**  
** Description : Unregisters the callback function that hooks the 
**               Tx Messages to the application. 
**
**    See Also : APH_RegSendMsgHook()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_UnregSendMsgHook( void )
{
  APH_pf_cbTxMsg = NULL;
  return;
}

/***********************************************************************
**  
** Function    : APH_Hook_Send
**  
** Description : Tx message hook function. Hooks the message to send
**               to the registered callback function. If the callback
**               function changes the length of the message to zero,
**               the message does not get sent
**
**    See Also : APH_RegSendMsgHook(),APH_UnregSendMsgHook()
**  
** Parameters  : pb_txMsg (IN/OUT) - tx message
**               pw_len   (IN/OUT) - lenght of tx message
**               
** Returnvalue : <> NULL - message shall get sent
**               == NULL - message shall get discarded 
** 
** Remarks     : -
**  
***********************************************************************/
const UINT8* APH_Hook_Send(const UINT8* pb_txMsg,UINT16 *pw_len)
{
  static UINT8 ab_msg[k_MAX_PTP_PLD];
  BOOLEAN o_ret;
  PTP_t_msgTypeEnum e_mType;
  const UINT8 *pb_ret;
  /* get message type */
  e_mType = NIF_UNPACKHDR_MSGTYPE(pb_txMsg);
  /* preset TS-destroy-flag to FALSE */
  o_tsDestrFlag = FALSE;
  /* check, if callback is registered */
  if( APH_pf_cbTxMsg != NULL )
  {
    /* copy message */
    PTP_BCOPY(ab_msg,pb_txMsg,*pw_len);
    o_ret = APH_pf_cbTxMsg(ab_msg,pw_len);
    /* shall message be destroyed ? */
    if((o_ret == FALSE) ||(*pw_len < k_PTPV2_HDR_SZE) )
    {
      /* is it timestamped ?*/
      if((e_mType == e_MT_SYNC)   ||
         (e_mType == e_MT_DELREQ)  ||
         (e_mType == e_MT_PDELREQ) ||
         (e_mType == e_MT_PDELRESP))
      {
        /* set TS-destroy-flag to FALSE */
        o_tsDestrFlag = TRUE;
      }       
    }
    if( o_ret == TRUE )
    {
      /* set return pointer to copied buffer */
      pb_ret = ab_msg;
    }
    else
    {
      pb_ret = NULL;
    }
  }
  else
  {
    /* return same address */
    pb_ret = pb_txMsg;
  }
  return pb_ret;
}

/***********************************************************************
**  
** Function    : APH_CheckTxTs
**  
** Description : Returns value of destroy-flag. This flag determines, if
**               the last sent event message was destroyed by hook.
**  
** Parameters  : -
**               
** Returnvalue : value of flag
**
** Remarks     : -
**  
***********************************************************************/
BOOLEAN APH_CheckTxTs( void )
{
  return o_tsDestrFlag;
}

/***********************************************************************
**  
** Function    : APH_SetToOneStep
**  
** Description : Changes the clock to a one-step clock. All other send
**               hooks are executed after changing the messages to 
**               one-step.
**
**    See Also : APH_ResetToTwoStep()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_SetToOneStep( void ) 
{
#if((k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC == TRUE ))
  /* change flag in clock data set */
  CTL_s_ClkDS.s_DefDs.o_twoStep = FALSE;
#endif
  /* change flag to change the stack into one-step */
  o_oneStep = TRUE;
  return;
}

/***********************************************************************
**  
** Function    : APH_ResetToTwoStep
**  
** Description : Resets the Clock to the normal two-step functionality
**
**    See Also : APH_SetToOneStep()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_ResetToTwoStep( void )
{
#if((k_CLK_IS_OC == TRUE)||(k_CLK_IS_BC == TRUE ))
  /* change flag in clock data set */
  CTL_s_ClkDS.s_DefDs.o_twoStep = TRUE;
#endif
  /* change flag to change the stack into one-step */
  o_oneStep = FALSE;
  return; 
}


/***********************************************************************
**  
** Function    : APH_OneStepHook
**  
** Description : Changes the stack to behave like one-step.
**  
** Parameters  : pb_msg    (IN) - message in network format
**               w_len     (IN) - length of message
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
const UINT8* APH_OneStepHook( const UINT8* pb_msg,UINT16 w_len)
{
  static UINT8      ab_msg[k_MAX_PTP_PLD];
  PTP_t_msgTypeEnum e_mType;
  const UINT8       *pb_ret;
  PTP_t_TmStmp      s_tsSend;
  PTP_t_TmStmp      s_tsRecv;
  PTP_t_TmIntv      s_tiOld;
  PTP_t_TmIntv      s_tiTrnArnd;

  /* get message type */
  e_mType = NIF_UNPACKHDR_MSGTYPE(pb_msg);
  /* check, if callback is registered */
  if( o_oneStep == TRUE )
  {
    /* copy message */
    PTP_BCOPY(ab_msg,pb_msg,w_len);
    /* change dependent one message type */
    switch( e_mType )
    {
      /* follow up and/or peer delay response 
         follow up messages are disturbed */
      case e_MT_FLWUP:
      case e_MT_PDELRES_FLWUP:
      {
        /* return NULL pointer - message will not be sent */
        pb_ret = NULL;
        break;
      }
      /* sync message and peer delay response message */
      case e_MT_SYNC:
      {
        /* change one-step flag to FALSE */
        NIF_PackHdrFlag(ab_msg,k_FLG_TWO_STEP,FALSE);
        if( PTP_GetSysTime(&s_tsSend) == FALSE )
        {
          /* destroy message - no timestamp */
          pb_ret = NULL;
        }
        else
        {
          /* pack send timestamp into message */
          GOE_hton48(&ab_msg[k_OFFS_TS_SYN_SEC],&s_tsSend.u48_sec);
          /* pack receive timestamp nanoseconds */
          GOE_hton32(&ab_msg[k_OFFS_TS_SYN_NSEC],s_tsSend.dw_Nsec);
          /* return new message */
          pb_ret = ab_msg;
        }
        break;
      }
      case e_MT_PDELRESP:
      {
        /* change one-step flag to FALSE */
        NIF_PackHdrFlag(ab_msg,k_FLG_TWO_STEP,FALSE);
        /* extract pdel request receipt timestamp */
        s_tsRecv.u48_sec = GOE_ntoh48(&ab_msg[k_OFFS_P2P_REQTS_SEC]);
        s_tsRecv.dw_Nsec = GOE_ntoh32(&ab_msg[k_OFFS_P2P_REQTS_NSEC]);
        /* extract old correction field */
        s_tiOld.ll_scld_Nsec = (INT64)GOE_ntoh64(&ab_msg[k_OFFS_CORFIELD]);
        /* set receive timestamp to zero */
        s_tsSend.u48_sec = (UINT64)0;
        s_tsSend.dw_Nsec = 0;
        GOE_hton48(&ab_msg[k_OFFS_P2P_REQTS_SEC],&s_tsSend.u48_sec);
        GOE_hton32(&ab_msg[k_OFFS_P2P_REQTS_NSEC],s_tsSend.dw_Nsec);
        /* get send time estimation */
        if( PTP_GetSysTime(&s_tsSend) == FALSE )
        {
          /* destroy message - no timestamp */
          pb_ret = NULL;
        }
        else
        {
          /* get turnaround time t3-t2 */
          if( PTP_SubtrTmStmpTmStmp(&s_tsSend,&s_tsRecv,&s_tiTrnArnd) == FALSE )
          {
            /* destroy message - no timestamp */
            pb_ret = NULL;
          }
          else
          {
            /* add old correction field value to turnaround time */
            s_tiTrnArnd.ll_scld_Nsec += s_tiOld.ll_scld_Nsec;
            /* pack turnaround time into message */
            GOE_hton64(&ab_msg[k_OFFS_CORFIELD],
                       (const UINT64*)&s_tiTrnArnd.ll_scld_Nsec);
            /* return new message */
            pb_ret = ab_msg;
          }
        }
        break;        
      }
      case e_MT_DELREQ:        
      case e_MT_PDELREQ:
      case e_MT_DELRESP:    
      case e_MT_ANNOUNCE:     
      case e_MT_SGNLNG:      
      case e_MT_MNGMNT:
      {
        /* do nothing and return copied original */
        pb_ret = ab_msg;
        break;
      }
      default:
      {
        /* do nothing and return copied original */
        pb_ret = ab_msg;
        break;
      }
    }
  }
  else
  {
    /* return unchanged buffer */
    pb_ret = pb_msg;
  }
  return pb_ret;
}

/***********************************************************************
**  
** Function    : APH_RegRxTsHookCb
**  
** Description : Registers a callback function that hooks the 
**               Rx timestamps and the corresponding data of the timestamped
**               packet to the application. 
**               Only one callback function can be registered at one time.
**
**    See Also : APH_UnregRxTsHook()
**  
** Parameters  : pf_cbRxTs (IN) - Rx timestamp callback function to register
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_RegRxTsHookCb( APH_t_pfCbRxTs pf_cbRxTs)
{
  APH_pf_cbRxTs = pf_cbRxTs;
  return;
}

/***********************************************************************
**  
** Function    : APH_UnregRxTsHook
**  
** Description : Unregisters the Rx timestamp hook function.
**    
**    See Also : APH_RegRxTsHookCb()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_UnregRxTsHook( void )
{
  APH_pf_cbRxTs = NULL;
  return;
}

/***********************************************************************
**  
** Function    : APH_Hook_RxTs
**  
** Description : Rx timestamp Hook that calls the registered rx timestamp
**               callback function and passes all values.
**
**    See Also : APH_RegRxTsHookCb(),APH_UnregRxTsHook()
**  
** Parameters  : ps_rxTs   (IN) - rx timestamp
**               w_seqId   (IN) - sequence id of timestamped message 
**               ps_pIdSnd (IN) - sender port id of of timestamped message
**               w_rxIfIdx (IN) - interface index where timestamped 
**                                message was received
**               e_mType   (IN) - message type of timestamped message
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_Hook_RxTs(PTP_t_TmStmp      *ps_rxTs,
                   UINT16            w_seqId,
                   PTP_t_PortId      *ps_pIdSnd,
                   UINT16            w_rxIfIdx,
                   PTP_t_msgTypeEnum e_mType)
{
  /* check, if callback is registered */
  if( APH_pf_cbRxTs != NULL )
  {
    /* call rx timestamp callback function */
    APH_pf_cbRxTs(ps_rxTs,w_seqId,ps_pIdSnd,w_rxIfIdx,e_mType);
  }
  return;
}

/***********************************************************************
**  
** Function    : APH_RegTxTsHookCb
**  
** Description : Registers a callback function that hooks the 
**               Tx timestamps and the corresponding data of the timestamped
**               packet to the application. 
**               Only one callback function can be registered at one time.
**
**    See Also : APH_UnregTxTsHook()
**  
** Parameters  : pf_cbTxTs (IN) - Tx timestamp callback function to register
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_RegTxTsHookCb( APH_t_pfCbTxTs pf_cbTxTs )
{
  APH_pf_cbTxTs = pf_cbTxTs;
  return;
}

/***********************************************************************
**  
** Function    : APH_UnregTxTsHook
**  
** Description : Unregisters the Tx timestamp hook function.
**    
**    See Also : APH_RegTxTsHookCb()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_UnregTxTsHook( void )
{
  APH_pf_cbTxTs = NULL;
  return;
}

/***********************************************************************
**  
** Function    : APH_Hook_TxTs
**  
** Description : Tx timestamp Hook that calls the registered tx timestamp
**               callback function and passes all values.
**
**    See Also : APH_RegTxTsHookCb(),APH_UnregTxTsHook()
**  
** Parameters  : ps_txTs   (IN) - tx timestamp
**               w_seqId   (IN) - sequence id of timestamped message 
**               w_txIfIdx (IN) - interface index where timestamped 
**                                message was sent
**               e_mType   (IN) - message type of timestamped message
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_Hook_TxTs(PTP_t_TmStmp      *ps_txTs,
                   UINT16            w_seqId,
                   UINT16            w_txIfIdx,
                   PTP_t_msgTypeEnum e_mType)
{
  /* check, if callback is registered */
  if( APH_pf_cbTxTs != NULL )
  {
    /* call tx timestamp callback function */
    APH_pf_cbTxTs(ps_txTs,w_seqId,w_txIfIdx,e_mType);
  }
  return;
}

/***********************************************************************
**  
** Function    : APH_RegRecvMsgHook
**  
** Description : Registers a callback function that hooks the 
**               Rx Messages to the application. 
**               Only one callback function can be registered.
**               It is possible to 
**                 - read the received message 
**                 - change the received message content
**                 - change the received message length
**                 - destroy the reiceived message (by returning FALSE)
**               in the callback function.
**
**    See Also : APH_UnregRecvMsgHook()
**  
** Parameters  : pf_cbRxMsg (IN) - Rx message callback function to register
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_RegRecvMsgHook( APH_t_pfCbRxMsg pf_cbRxMsg)
{
  APH_pf_cbRxMsg = pf_cbRxMsg;
  return;
}

/***********************************************************************
**  
** Function    : APH_UnregRecvMsgHook
**  
** Description : Unregisters the callback function that hooks the 
**               Rx Messages to the application. 
**
**    See Also : APH_RegRecvMsgHook()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void APH_UnregRecvMsgHook( void )
{
  APH_pf_cbRxMsg = NULL;
  return;
}

/***********************************************************************
**  
** Function    : APH_Hook_Recv
**  
** Description : Rx message hook function. Hooks the received message
**               to the registered callback function. If the callback
**               function returns FALSE, the received message gets destroyed.
**
**    See Also : APH_RegRecvMsgHook(),APH_UnregRecvMsgHook()
**  
** Parameters  : pb_rxMsg (IN/OUT) - Rx message to hook
**               pw_len   (IN/OUT) - length of hooked message
**               dw_ipAddr (IN)    - ip address of sender 
**               
** Returnvalue : TRUE  - message shall be forwarded to stack
**               FALSE - message shall be discarded 
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN APH_Hook_Recv(UINT8* pb_rxMsg,UINT16 *pw_len,UINT32 dw_ipAddr)
{
  /* check, if callback is registered */
  if( APH_pf_cbRxMsg != NULL )
  {
    return APH_pf_cbRxMsg(pb_rxMsg,pw_len,dw_ipAddr);
  }
  else
  {
    return TRUE;
  }
}

/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
**  
** Function    : ResetSndHookTable
**  
** Description : Resets the complete Hook Table.
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
static void ResetSndHookTable( void )
{
  UINT16 w_hook;

  /* clean hook table */
  for( w_hook = 0 ; w_hook < k_MAX_SND_HOOK_CMDS ; w_hook++ )
  {
    /* hook command set to empty */
    as_sndHookCmds[w_hook].o_hookIsSet = FALSE;
  }
}
