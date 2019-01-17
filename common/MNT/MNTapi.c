/*******************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
********************************************************************************
**
**       File: MNTapi.c
**    Summary: MNTapi - Management application interface
**             This API is included and compiled with the corresponding
**             define, MNT_API, in target.h. The management API offers a
**             user interface for accessing and configuring the local
**             node as well as all remote nodes within a PTP domain via
**             the application. A corresponding management message is 
**             generated and transmitted to the local node or via the
**             underlying network topology to other nodes by calling the
**             associated function of the management API. 
**
**             Each function requires a set of general information for
**             message generation. The application must pass the structure
**             ps_clbkData (of type MNT_t_apiClbkData) containing a 
**             callback function (for RESPONSE, ACKNOWLEDGE or management
**             status error messages) and a user defined handle. The
**             second parameter, ps_msgConfig (MNT_t_apiMsgConfig), is also
**             a structure and contains the necessary information to
**             generate the message. For this, target addresses as well as
**             general configurable elements of the PTP management message
**             must be defined. A timeout parameter is also required. The
**             timeout parameter is used to generate a timeout, if a request
**             is not responded to during a certain duration or addressed to
**             all clocks / all ports by setting the target port id to all
**             ones. Depending on the message function, additional
**             parameters must be passed to configure the management 
**             message. These parameters are only used with a SET request. 
** 
**             After receiving a response either from the local node or from
**             a remote node, the callback function is called. A structure 
**             of the type MNT_t_apiReturnTLV as well as the state is returned 
**             to the application. The return TLV contains the source port id 
**             (s_srcPortId), the TLV structure s_mntTlv and the buffer size. 
**             The application must copy all needed data; upon return to the
**             management API all data are freed. The callback is also used
**             to indicate a state change due to an error, abort or timeout.
**
**    Version: 1.01.01
**
********************************************************************************
********************************************************************************
**
**  Functions: MNTapi_NullManagement
**             MNTapi_ClkDesc
**             MNTapi_UsrDesc
**             MNTapi_SaveNVolStor
**             MNTapi_RstNVolStor
**             MNTapi_Initialize
**             MNTapi_FaultLog
**             MNTapi_FaultLogReset
**             MNTapi_DefaultDs
**             MNTapi_CurrentDs
**             MNTapi_ParentDs
**             MNTapi_TimePropDs
**             MNTapi_PortDs
**             MNTapi_Priority1
**             MNTapi_Priority2
**             MNTapi_Domain
**             MNTapi_SlaveOnly
**             MNTapi_AnncIntv
**             MNTapi_AnncRxTimeout
**             MNTapi_SyncIntv
**             MNTapi_VersionNumber
**             MNTapi_EnaPort
**             MNTapi_DisPort
**             MNTapi_Time
**             MNTapi_ClkAccuracy
**             MNTapi_UtcProp
**             MNTapi_TraceProp
**             MNTapi_TimeSclProp
**             MNTapi_UcNegoEnable
**             MNTapi_PathTraceList
**             MNTapi_PathTraceEna
**             MNTapi_GMClusterTbl
**             MNTapi_UCMasterTbl
**             MNTapi_UCMstMaxTblSize
**             MNTapi_AcceptMstTbl
**             MNTapi_AcceptMstTblEna
**             MNTapi_AcceptMstMaxTblSize
**             MNTapi_AltMaster
**             MNTapi_AltTimeOffsEna
**             MNTapi_AltTimeOffsName
**             MNTapi_AltTimeOffsKey
**             MNTapi_AltTimeOffsProp
**             MNTapi_TCDefaultDs
**             MNTapi_TCPortDs
**             MNTapi_PrimaryDomain
**             MNTapi_DelayMechanism
**             MNTapi_MinPDelReqIntv
**             MNTapi_GatherAnswers
**             HandleRequest
**             SendApiReq
**             
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


/*************************************************************************
**    include-files
*************************************************************************/
#include "target.h"
#ifdef MNT_API
#include "PTP/PTPdef.h"
#include "SIS/SIS.h"
#include "NIF/NIF.h"
#include "GOE/GOE.h"
#include "PTP/PTP.h"
#include "MNT/MNTapi.h"
#include "MNT/MNTint.h"

/*******************************************************************************
**    global variables
*******************************************************************************/


/*******************************************************************************
**    static constants, types, macros, variables
*******************************************************************************/


/*******************************************************************************
**    static function-prototypes
*******************************************************************************/
static BOOLEAN HandleRequest( const MNT_t_apiMsgConfig *ps_msgConfig,
                              const MNT_t_apiClbkData  *ps_clbkData,
                                    PTP_t_MntTLV       *ps_mntTlv,
                                    UINT16             w_dynSize );
static BOOLEAN SendApiReq( const MNT_t_apiMsgConfig *ps_msgConfig,
                           const MNT_t_apiClbkData  *ps_clbkData,
                           const PTP_t_MntTLV       *ps_mntTlv);

/*******************************************************************************
**    global functions
*******************************************************************************/

/***********************************************************************
**
** Function    : MNTapi_NullManagement
**
** Description : This function handles the management message
**               NULL_MANAGEMENT. It returns a specifically defined
**               response to the requester without affecting data
**               sets or states.
**               Behavior based on the action command:
**               - GET returns a RESPONSE message
**               - SET returns a RESPONSE message
**               - COMMAND returns a ACKNOWLEDGE message
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.1 
**
***********************************************************************/
BOOLEAN MNTapi_NullManagement( const MNT_t_apiMsgConfig *ps_msgConfig,
                               const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* for GET, SET and COMMAND */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) ||
        (ps_msgConfig->b_actionFld == k_ACTF_SET) ||
        (ps_msgConfig->b_actionFld == k_ACTF_CMD) )
    {

      /* set TLV values */
      s_mntTlv.w_mntId = k_NULL_MANAGEMENT;   
      s_mntTlv.pb_data = NULL;    

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_ClkDesc
**
** Description : This function handles the management message 
**               CLOCK_DESCRIPTION. It handles the physical and 
**               implementation specific descriptions of the node 
**               and the requested port (interface). A GET message 
**               prompts a response including all information data. 
**               The only supported action command is GET. 
**
** See Also    : MNTapi_UsrDesc()
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.2 
**
***********************************************************************/
BOOLEAN MNTapi_ClkDesc( const MNT_t_apiMsgConfig *ps_msgConfig,
                        const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_CLOCK_DESCRIPTION;   
      s_mntTlv.pb_data = NULL;    

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_UsrDesc
**
** Description : This function handles the management message
**               USER_DESCRIPTION. The user description defines the
**               name and physical location of the device, described
**               in a PTP text profile. GET and SET commands are both
**               supported; however, the additional data parameter is
**               only used with a SET command. 
**
** Parameters  : ps_msgConfig (IN) - msg addresses & config
**               ps_clbkData  (IN) - status and callback info
**               ps_userDesc  (IN) - user description
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.3 
**
***********************************************************************/
BOOLEAN MNTapi_UsrDesc( const MNT_t_apiMsgConfig *ps_msgConfig,
                        const MNT_t_apiClbkData  *ps_clbkData,
                        const PTP_t_Text         *ps_userDesc )
{
  UINT8        ab_tlvPld[130];
  PTP_t_MntTLV s_mntTlv;
  UINT16       w_dynSize = 0;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_USER_DESCRIPTION;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      w_dynSize = 0;
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* check parameter */
      if( ps_userDesc != NULL )
      {
        /* check PTP text parameter */
        if( (ps_userDesc->pc_text != NULL) && (ps_userDesc->b_len != 0) )
        {
          /* check set values */
          if( ps_userDesc->b_len <= k_MAX_USRDESC_SZE )
          {
            /* generate TLV payload */
            ab_tlvPld[k_OFFS_UD_SIZE] = ps_userDesc->b_len;
            PTP_BCOPY(&ab_tlvPld[k_OFFS_UD_TEXT],    
                      ps_userDesc->pc_text,    
                      ps_userDesc->b_len);  
            /* calculate size of dynamic members */
            w_dynSize = ps_userDesc->b_len + 1;
            /* get padding data */
            if((w_dynSize & 0x01) == 0x01)
            {
              /* add padding byte */
              ab_tlvPld[w_dynSize] = 0;
              w_dynSize++;
            }

            /* set payload pointer in TLV structure */
            s_mntTlv.pb_data = ab_tlvPld;  
            o_ret = TRUE;
          }
          else
          {
            /* wrong value, to many symbols in PTP text */
            PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC);
          }
        }
        else
        {
          /* wrong value, no PTP text available */
          PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC);
        }
      }
      else
      {
        /* wrong value, no PTP text available */
        PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC);
      }
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC);
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             w_dynSize );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_SaveNVolStor
**
** Description : This function handles the management message
**               SAVE_IN_NON_VOLATILE_STORAGE. This command will
**               store all current values of the applicable dynamic
**               and configurable data set members into non volatile
**               storage. The only supported action command is 
**               COMMAND. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.4
**
***********************************************************************/
BOOLEAN MNTapi_SaveNVolStor( const MNT_t_apiMsgConfig *ps_msgConfig,
                             const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only command is allowed */
    if( (ps_msgConfig->b_actionFld == k_ACTF_CMD) )
    {
      /* set TLV values */  
      s_mntTlv.w_mntId = k_SAVE_IN_NON_VOLATILE_STORAGE;   
      s_mntTlv.pb_data = NULL;    

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC);
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_RstNVolStor
**
** Description : This function handles the management message 
**               RESET_NON_VOLATILE_STORAGE. This command resets
**               the dynamic and configurable data set members of
**               non volatile storage to initialization default 
**               values. The only supported action command is
**               COMMAND. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.5
**
***********************************************************************/
BOOLEAN MNTapi_RstNVolStor( const MNT_t_apiMsgConfig *ps_msgConfig,
                            const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    if( (ps_msgConfig->b_actionFld == k_ACTF_CMD) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_RESET_NON_VOLATILE_STORAGE;   
      s_mntTlv.pb_data = NULL;    

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_Initialize
**
** Description : This function handles the management message
**               INITIALIZE. This command starts special
**               initialization depending on the INITIALIZATION_KEY.
**               The only supported action command is COMMAND.
**               Defined initialization keys are: 
**
**                0000 - INITIALIZE_EVENT
**                       Causes INITIALZE event within an OC or BC.
**                0001-7FFF - Reserved
**                8000-FFFF - Implementation-specific
**
** Parameters  : ps_msgConfig (IN) - msg addresses & config
**               ps_clbkData  (IN) - status and callback info
**               w_initKey    (IN) - initialization key
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occured
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.6
**
***********************************************************************/
BOOLEAN MNTapi_Initialize( const MNT_t_apiMsgConfig *ps_msgConfig,
                           const MNT_t_apiClbkData  *ps_clbkData,
                                 UINT16             w_initKey )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only COMMAND allowed */
    if( (ps_msgConfig->b_actionFld == k_ACTF_CMD) )
    {
      /* set TLV values - set initialize key */
      s_mntTlv.w_mntId = k_INITIALIZE;   
      s_mntTlv.pb_data = ab_tlvPld;   

      /* add data to TLV paylaod */
      GOE_hton16(&ab_tlvPld[k_OFFS_INIT_KEY], w_initKey);

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_FaultLog
**
** Description : This function handles the management message
**               FAULT_LOG. This management messages returns all
**               recorded faults of the node. The only supported
**               action command is GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.7
**
***********************************************************************/
BOOLEAN MNTapi_FaultLog( const MNT_t_apiMsgConfig *ps_msgConfig,
                         const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only GET allowed */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_FAULT_LOG;   
      s_mntTlv.pb_data = NULL;    

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_FaultLogReset
**
** Description : This function handles the management message
**               FAULT_LOG_RESET. This management messages resets
**               the complete fault record of the node. The only
**               supported action command is COMMAND. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.8
**
***********************************************************************/
BOOLEAN MNTapi_FaultLogReset( const MNT_t_apiMsgConfig *ps_msgConfig,
                              const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    if( (ps_msgConfig->b_actionFld == k_ACTF_CMD) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_FAULT_LOG_RESET;   
      s_mntTlv.pb_data = NULL;   

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_DefaultDs
**
** Description : This function handles the management message
**               DEFAULT_DATA_SET. This management message prompts
**               a response including the default data set members.
**               The only supported action command is GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.3.1
**
***********************************************************************/
BOOLEAN MNTapi_DefaultDs( const MNT_t_apiMsgConfig *ps_msgConfig,
                          const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only GET allowed */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_DEFAULT_DATA_SET;   
      s_mntTlv.pb_data = NULL;   

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_CurrentDs
**
** Description : This function handles the management message
**               CURRENT_DATA_SET. This management message prompts
**               a response including the current data set members.
**               The only supported action command is GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.4.1
**
***********************************************************************/
BOOLEAN MNTapi_CurrentDs( const MNT_t_apiMsgConfig *ps_msgConfig,
                          const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only GET allowed */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_CURRENT_DATA_SET;   
      s_mntTlv.pb_data = NULL;   

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_ParentDs
**
** Description : This function handles the management message
**               PARENT_DATA_SET. This management message prompts
**               a response including the parent data set members.
**               The only supported action command is GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.5.1
**
***********************************************************************/
BOOLEAN MNTapi_ParentDs( const MNT_t_apiMsgConfig *ps_msgConfig,
                         const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only GET allowed */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_PARENT_DATA_SET;   
      s_mntTlv.pb_data = NULL;   

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_TimePropDs
**
** Description : This function handles the management message
**               TIME_PROPERTIES_DATA_SET. This management message
**               prompts a response including the time properties 
**               data set members. The only supported action command
**               is GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.6.1
**
***********************************************************************/
BOOLEAN MNTapi_TimePropDs( const MNT_t_apiMsgConfig *ps_msgConfig,
                           const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only GET allowed */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_TIME_PROPERTIES_DATA_SET;   
      s_mntTlv.pb_data = NULL;   

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_PortDs
**
** Description : This function handles the management message
**               PORT_DATA_SET. This management message prompts a
**               response including the port data set members. If
**               addressed to all ports, a response for each port
**               must be issued. The only supported action command
**               is GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.7.1
**
***********************************************************************/
BOOLEAN MNTapi_PortDs( const MNT_t_apiMsgConfig *ps_msgConfig,
                       const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only GET allowed */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_PORT_DATA_SET;   
      s_mntTlv.pb_data = NULL;   

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_Priority1
**
** Description : This function handles the management message
**               PRIORITY1. This message gets or updates the
**               priority1 member of the default data set. The GET
**               and SET commands are both supported; however, the
**               additional data parameter (b_priority1) is only 
**               used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_priority1    (IN) - value of priority 1
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.3.2
**
***********************************************************************/
BOOLEAN MNTapi_Priority1( const MNT_t_apiMsgConfig *ps_msgConfig,
                          const MNT_t_apiClbkData  *ps_clbkData,
                                UINT8              b_priority1 )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_PRIORITY1;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_PRIO1_PRIO] = b_priority1;
      ab_tlvPld[k_OFFS_PRIO1_RES]  = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_Priority2
**
** Description : This function handles the management message
**               PRIORITY2. This message gets or updates the
**               priority2 member of the default data set. The GET 
**               and SET commands are both supported; however, the 
**               additional data parameter (b_priority2) is only 
**               used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_priority2    (IN) - value of priority 2
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occured
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.3.3
**
***********************************************************************/
BOOLEAN MNTapi_Priority2( const MNT_t_apiMsgConfig *ps_msgConfig,
                          const MNT_t_apiClbkData  *ps_clbkData,
                                UINT8              b_priority2 )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_PRIORITY2;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_PRIO2_PRIO] = b_priority2;
      ab_tlvPld[k_OFFS_PRIO2_RES]  = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_Domain
**
** Description : This function handles the management message
**               DOMAIN. This message gets or updates the domain
**               member of the default data set. The GET and SET 
**               commands are both supported; however, the additional
**               data parameter (b_domain) is only used with a SET
**               command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_domain       (IN) - value of domain number
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.3.4
**
***********************************************************************/
BOOLEAN MNTapi_Domain( const MNT_t_apiMsgConfig *ps_msgConfig,
                       const MNT_t_apiClbkData  *ps_clbkData,
                             UINT8              b_domain )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_DOMAIN;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_DOM_NUM] = b_domain;
      ab_tlvPld[k_OFFS_DOM_RES] = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_SlaveOnly
**
** Description : This function handles the management message
**               SLAVE_ONLY. This message gets or updates the slave
**               only member of the default data set. The GET and 
**               SET commands are both supported; however, the 
**               additional data parameter (o_slaveOnly) is only 
**               used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               o_slaveOnly    (IN) - TRUE  -> slave only
**                                     FALSE -> master is possible
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.3.5
**
***********************************************************************/
BOOLEAN MNTapi_SlaveOnly( const MNT_t_apiMsgConfig *ps_msgConfig,
                          const MNT_t_apiClbkData  *ps_clbkData,
                                BOOLEAN            o_slaveOnly )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_SLAVE_ONLY;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL; 
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_SLVO_FLAGS] = 0; /* reset flags */
      SET_FLAG(ab_tlvPld[k_OFFS_SLVO_FLAGS],0,o_slaveOnly); 
      ab_tlvPld[k_OFFS_SLVO_RES]   = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_AnncIntv
**
** Description : This function handles the management message
**               LOG_ANNOUNCE_INTERVAL. This message gets or updates
**               the announce interval member of the port data set.
**               If a request is addressed to all ports, a response 
**               message for each port must be issued. The GET and
**               SET commands are both supported; however, the 
**               additional data parameter (c_anncIntv) is only used
**               with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               c_anncIntv     (IN) - value of the announce
**                                     interval
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.7.2
**
***********************************************************************/
BOOLEAN MNTapi_AnncIntv( const MNT_t_apiMsgConfig *ps_msgConfig,
                         const MNT_t_apiClbkData  *ps_clbkData,
                               INT8               c_anncIntv )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_LOG_MEAN_ANNOUNCE_INTERVAL;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ((INT8*)ab_tlvPld)[k_OFFS_LAI_INTV] = c_anncIntv;
      ab_tlvPld[k_OFFS_LAI_RES]  = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_AnncRxTimeout
**
** Description : This function handles the management message 
**               ANNOUNCE_RECEIPT_TIMEOUT. This message gets or 
**               updates the announce receipt timeout member of 
**               the port data set. If a request is addressed to 
**               all ports, a response message for each port must 
**               be issued. The GET and SET commands are both 
**               supported; however, the additional data parameter
**               (b_anncTOut) is only used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_anncTOut     (IN) - value of the announce
**                                     receipt timeout
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.7.3
**
***********************************************************************/
BOOLEAN MNTapi_AnncRxTimeout( const MNT_t_apiMsgConfig *ps_msgConfig,
                              const MNT_t_apiClbkData  *ps_clbkData,
                                    UINT8              b_anncTOut )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_ANNOUNCE_RECEIPT_TIMEOUT;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_ARTO_TO]  = b_anncTOut;
      ab_tlvPld[k_OFFS_ARTO_RES] = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_SyncIntv
**
** Description : This function handles the management message 
**               LOG_SYNC_INTERVAL. This message gets or updates 
**               the sync interval member of the port data set. 
**               If a request is addressed to all ports, a 
**               response message for each port must be issued. 
**               The GET and SET commands are both supported; 
**               however, the additional data parameter 
**               (c_syncIntv) is only used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               c_syncIntv     (IN) - value of sync interval
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.7.4
**
***********************************************************************/
BOOLEAN MNTapi_SyncIntv( const MNT_t_apiMsgConfig *ps_msgConfig,
                         const MNT_t_apiClbkData  *ps_clbkData,
                               INT8               c_syncIntv )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_LOG_MEAN_SYNC_INTERVAL;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_LSI_INTV] = (UINT8)c_syncIntv;
      ab_tlvPld[k_OFFS_LSI_RES]  = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_VersionNumber
**
** Description : This function handles the management message 
**               VERSION_NUMBER. This message gets or updates the
**               version number member of the port data set. If a 
**               request is addressed to all ports, a response 
**               message for each port must be issued. The GET and
**               SET commands are both supported; however, the 
**               additional data parameter (b_version) is only used
**               with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_version      (IN) - value of version number
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.7.7
**
***********************************************************************/
BOOLEAN MNTapi_VersionNumber( const MNT_t_apiMsgConfig *ps_msgConfig,
                              const MNT_t_apiClbkData  *ps_clbkData,
                                    UINT8              b_version )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_VERSION_NUMBER;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL; 
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_VN_NUM] = b_version;
      ab_tlvPld[k_OFFS_VN_RES] = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_EnaPort
**
** Description : This function handles the management message 
**               ENABLE_PORT. This command enables a port on an OC
**               or BC using the DESIGNATED_ENABLED event. The only
**               supported action command is COMMAND. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.2.3
**
***********************************************************************/
BOOLEAN MNTapi_EnaPort( const MNT_t_apiMsgConfig *ps_msgConfig,
                        const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    if( (ps_msgConfig->b_actionFld == k_ACTF_CMD) )
    {
      /* set TLV values */  
      s_mntTlv.w_mntId = k_ENABLE_PORT;   
      s_mntTlv.pb_data = NULL;    

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_DisPort
**
** Description : This function handles the management message 
**               DISABLE_PORT. This command disables a port on an OC
**               or BC using the DESIGNATED_DISABLED event. The only 
**               supported action command is COMMAND. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.2.4
**
***********************************************************************/
BOOLEAN MNTapi_DisPort( const MNT_t_apiMsgConfig *ps_msgConfig,
                        const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only COMMAND allowed */
    if( (ps_msgConfig->b_actionFld == k_ACTF_CMD) )
    {
      /* set TLV values */  
      s_mntTlv.w_mntId = k_DISABLE_PORT;   
      s_mntTlv.pb_data = NULL;    

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_Time
**
** Description : This function handles the management message 
**               TIME. This message gets or updates the local time
**               of a node. A SET request is only relevant on a
**               grandmaster node, because all other nodes are 
**               synchronized to the GM, and therefore, an updated 
**               time would be overwritten by the next sync interval.
**               Thus, a SET request to nodes other than the GM is
**               prohibited and a management error status is 
**               returned. The GET and SET commands are both 
**               supported; however, the additional data parameter
**               (s_tmStmp) is only used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               ps_tmStmp     (IN) - value of time
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occured
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.2.1
**
***********************************************************************/
BOOLEAN MNTapi_Time( const MNT_t_apiMsgConfig *ps_msgConfig,
                     const MNT_t_apiClbkData  *ps_clbkData,
                     const PTP_t_TmStmp       *ps_tmStmp )
{
  UINT8        ab_tlvPld[sizeof(PTP_t_TmStmp)];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_TIME;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* check parameter */
      if( ps_tmStmp != NULL )
      {
        /* generate TLV payload */
        GOE_hton48((UINT8*)&ab_tlvPld[k_OFFS_TIME_SEC],&ps_tmStmp->u48_sec);
        GOE_hton32((UINT8*)&ab_tlvPld[k_OFFS_TIME_NSEC],ps_tmStmp->dw_Nsec);
        /* set payload pointer in TLV structure */
        s_mntTlv.pb_data = ab_tlvPld;  
        o_ret = TRUE;
      }
      else
      {
        /* no time available */
        PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
      }
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_ClkAccuracy
**
** Description : This function handles the management message 
**               CLOCK_ACCURACY. This mes-sage gets or updates the 
**               local clock accuracy member of the clock quality 
**               member of the default data set. A SET request is
**               only relevant on a grandmaster node, because all 
**               other nodes are synchronized to the GM, and 
**               therefore, an updated accuracy would be overwritten
**               by the next sync interval. Thus, a SET request to
**               nodes other than the GM is prohibited and a 
**               management error status is re-turned. The GET and
**               SET commands are both supported; however, the 
**               additional data parameter (e_clkAccuracy) is only
**               used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               e_clkAccuracy  (IN) - value of clock accuracy
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.2.2
**
***********************************************************************/
BOOLEAN MNTapi_ClkAccuracy( const MNT_t_apiMsgConfig *ps_msgConfig,
                            const MNT_t_apiClbkData  *ps_clbkData,
                                  PTP_t_clkAccEnum   e_clkAccuracy )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_CLOCK_ACCURACY;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_CLKA_ACC] = (UINT8)e_clkAccuracy;
      ab_tlvPld[k_OFFS_CLKA_RES] = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_UtcProp
**
** Description : This function handles the management message 
**               UTC_PROPERTIES. This message gets or updates UTC 
**               based members of the time properties data set. The
**               GET and SET commands are both supported; however 
**               the additional data parameters (i_utcOffset, 
**               o_leap61, o_leap59, o_utcValid) are only used with
**               a SET command.              
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               i_utcOffset    (IN) - value of current UTC offset
**               o_leap61       (IN) - TRUE  -> plus one second
**                                     FALSE -> no leap second
**               o_leap59       (IN) - TRUE  -> minus one second
**                                     FALSE -> no leap second
**               o_utcValid     (IN) - TREU  -> offset to UTC true
**                                     FALSE -> no sync. to UTC
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.6.2
**
***********************************************************************/
BOOLEAN MNTapi_UtcProp( const MNT_t_apiMsgConfig *ps_msgConfig,
                        const MNT_t_apiClbkData  *ps_clbkData,
                              INT16              i_utcOffset,
                              BOOLEAN            o_leap61,
                              BOOLEAN            o_leap59,
                              BOOLEAN            o_utcValid )
{
  UINT8        ab_tlvPld[4];
  PTP_t_MntTLV s_mntTlv;
  UINT16       w_tmpFlags;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_UTC_PROPERTIES;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      w_tmpFlags = 0;
      /* generate TLV payload */
      GOE_hton16(&ab_tlvPld[k_OFFS_UTC_OFFS],(UINT16)i_utcOffset);
      SET_FLAG(w_tmpFlags,
               k_OFFS_TIDS_FLAGS_LI61,
               o_leap61); 
      SET_FLAG(w_tmpFlags,
               k_OFFS_TIDS_FLAGS_LI59,
               o_leap59); 
      SET_FLAG(w_tmpFlags,
               k_OFFS_TIDS_FLAGS_UTCV,
               o_utcValid); 
      ab_tlvPld[k_OFFS_UTC_FLAGS] = (UINT8)w_tmpFlags; 
      ab_tlvPld[k_OFFS_UTC_RES] = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld; 
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_TraceProp
**
** Description : This function handles the management message 
**               TRACEABILITY_PROPERTIES. This message gets or
**               updates traceability members of the time 
**               properties data set. The GET and SET commands
**               are both supported; however, the additional 
**               data parameters (o_timeTrace, o_freqTrace) are 
**               only used with a SET command.               
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               o_timeTrace    (IN) - TRUE  -> time tracable
**                                     FALSE -> time is not tracable
**               o_freqTrace    (IN) - TRUE  -> frequency tracable
**                                     FALSE -> frequency not tracable
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.6.3
**
***********************************************************************/
BOOLEAN MNTapi_TraceProp( const MNT_t_apiMsgConfig *ps_msgConfig,
                          const MNT_t_apiClbkData  *ps_clbkData,
                                BOOLEAN            o_timeTrace,
                                BOOLEAN            o_freqTrace )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  UINT16       w_tmpFlags;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_TRACEABILITY_PROPERTIES;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      w_tmpFlags = 0;
      /* generate TLV payload */
      SET_FLAG(w_tmpFlags,
               k_OFFS_TIDS_FLAGS_TTRA,
               o_timeTrace); 
      SET_FLAG(w_tmpFlags,
               k_OFFS_TIDS_FLAGS_FTRA,
               o_freqTrace); 
      ab_tlvPld[k_OFFS_TRACE_FLAGS] = (UINT8)w_tmpFlags; 
      ab_tlvPld[k_OFFS_TRACE_RES] = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_TimeSclProp
**
** Description : This function handles the management message 
**               TIMESCALE_PROPERTIES. This message gets or updates
**               timescale members of the time properties data set. 
**               The GET and SET commands are both supported; however,
**               the additional data parameters (o_timeTrace, 
**               e_timeSource) are only used with a SET command.   
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               e_timeSource   (IN) - value of time source
**               o_ptpTmScale   (IN) - TRUE  -> PTP timescale
**                                     FALSE -> no PTP timescale
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.6.4
**
***********************************************************************/
BOOLEAN MNTapi_TimeSclProp( const MNT_t_apiMsgConfig *ps_msgConfig,
                            const MNT_t_apiClbkData  *ps_clbkData,
                                  PTP_t_tmSrcEnum    e_timeSource,
                                  BOOLEAN            o_ptpTmScale )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  UINT16       w_tmpFlags;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_TIMESCALE_PROPERTIES;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      w_tmpFlags = 0;
      /* generate TLV payload */
      SET_FLAG(w_tmpFlags,
               k_OFFS_TIDS_FLAGS_PTP,
               o_ptpTmScale); 
      ab_tlvPld[k_OFFS_SCALE_FLAGS] = (UINT8)w_tmpFlags; 
      ab_tlvPld[k_OFFS_SCALE_TISRC] = (UINT8)e_timeSource;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_UcNegoEnable
**
** Description : This function handles the management message 
**               UNICAST_NEGOTIATION_ENABLE. This message enables or 
**               disables unicast negotiation. The GET and SET 
**               commands are both supported; however, the additional
**               data parameter (o_unNegoEna) is only used with a SET
**               command.   
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               o_unNegoEna    (IN) - TRUE  -> enabled UC negotiation
**                                     FALSE -> disabled UC negotiation
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 16.1.4.5
**
***********************************************************************/
BOOLEAN MNTapi_UcNegoEnable( const MNT_t_apiMsgConfig *ps_msgConfig,
                             const MNT_t_apiClbkData  *ps_clbkData,
                                   BOOLEAN            o_unNegoEna )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_UNICAST_NEGOTIATION_ENABLE;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL; 
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_UCN_FLAGS] = 0; /* reset flags */
      SET_FLAG(ab_tlvPld[k_OFFS_UCN_FLAGS],
               k_OFFS_UCN_FLAGS_EN,
               o_unNegoEna); 
      ab_tlvPld[k_OFFS_UCN_RES] = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld; 
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}


/***********************************************************************
**
** Function    : MNTapi_PathTraceList
**
** Description : This function handles the management message 
**               PATH_TRACE_LIST. This message forces a node to 
**               respond the current and optional implemented path 
**               trace list. The only supported action command is
**               GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 16.2.8
**
***********************************************************************/
BOOLEAN MNTapi_PathTraceList( const MNT_t_apiMsgConfig *ps_msgConfig,
                              const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only GET available */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_PATH_TRACE_LIST;   
      s_mntTlv.pb_data = NULL;   


      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_PathTraceEna
**
** Description : This function handles the management message 
**               PATH_TRACE_ENABLE. This message enables or 
**               disables the path trace functionality. If 
**               enabled, a list could be requested with the
**               management message PATH_TRACE_LIST. The GET 
**               and SET commands are both supported; however,
**               the additional data parameter (o_pathTraceEna)
**               is only used within a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               o_pathTraceEna (IN) - TRUE  -> enabled path trace
**                                     FALSE -> disabled path trace
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 16.2.9
**
***********************************************************************/
BOOLEAN MNTapi_PathTraceEna( const MNT_t_apiMsgConfig *ps_msgConfig,
                             const MNT_t_apiClbkData  *ps_clbkData,
                                   BOOLEAN            o_pathTraceEna )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_PATH_TRACE_ENABLE;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_PATHENA_FLAG] = 0; /* reset flags */
      SET_FLAG(ab_tlvPld[k_OFFS_PATHENA_FLAG], 
               k_OFFS_PATHENA_FLAG_EN,
               o_pathTraceEna); 
      ab_tlvPld[k_OFFS_PATHENA_RES] = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_GMClusterTbl
**
** Description : This function handles the management message 
**               GRANDMASTER_CLUSTER_TABLE. A GET request responds 
**               the current grandmaster cluster table including 
**               each entry. A SET command forces the node to 
**               reconfigure the grandmaster cluster table. 
**               Configuring an empty table (b_tblSize is zero 
**               and ps_gmClustTbl points to NULL) generates a 
**               message, which causes the node to reset the 
**               grandmaster cluster table. Otherwise, a list of 
**               port addresses (ps_gmClustTbl) will be transmitted 
**               and stored by the receiving node. The GET and SET
**               commands are both supported; however, the 
**               additional data parameters are only used within 
**               the SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               c_queryIntv    (IN) - mean interval between UC
**                                     announce msg of the masters
**               b_tblSize      (IN) - amount of masters / table
**                                     entries
**               ps_gmClustTbl (IN) - pointer to first table
**                                     entry (port address)
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 17.3.4
**
***********************************************************************/
BOOLEAN MNTapi_GMClusterTbl( const MNT_t_apiMsgConfig *ps_msgConfig,
                             const MNT_t_apiClbkData  *ps_clbkData,
                                   INT8               c_queryIntv,
                                   UINT8              b_tblSize,
                             const PTP_t_PortAddr     *ps_gmClustTbl )
{
  UINT8        ab_tlvPld[2+(8*sizeof(PTP_t_PortAddr))];
  PTP_t_MntTLV s_mntTlv;
  UINT16       w_dynSize = 0;
  UINT8        b_tblIdx;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_GRANDMASTER_CLUSTER_TABLE;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL; 
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* if grandmaster cluster table has to be cleared */
      if((b_tblSize == 0) && (ps_gmClustTbl == NULL))
      {
        /* generate TLV payload */
        ab_tlvPld[k_OFFS_GMTBL_LQUINV] = (UINT8)c_queryIntv;
        ab_tlvPld[k_OFFS_GMTBL_SIZE+0] = 0;
        ab_tlvPld[k_OFFS_GMTBL_SIZE+1] = 0;
        /* values ok */
        o_ret = TRUE;
      }
      /* if grandmaster cluster table has to be configured */
      else if( ((b_tblSize * (4+k_MAX_NETW_ADDR_SZE)) < (k_MAX_TLV_PLD_LEN-3))
                    && (ps_gmClustTbl != NULL) )
      {
        /* generate TLV payload */
        ab_tlvPld[k_OFFS_GMTBL_LQUINV] = (UINT8)c_queryIntv;
        ab_tlvPld[k_OFFS_GMTBL_SIZE]   = b_tblSize;

        /* copy port address table */
        for(b_tblIdx = 0; b_tblIdx < b_tblSize; b_tblIdx++)
        {
          GOE_hton16(&ab_tlvPld[k_OFFS_GMTBL_MEMBERS+w_dynSize],
                     (UINT16)ps_gmClustTbl[b_tblIdx].e_netwProt);
          GOE_hton16(&ab_tlvPld[k_OFFS_GMTBL_MEMBERS+w_dynSize+2],
                     (UINT16)ps_gmClustTbl[b_tblIdx].w_AddrLen);
          PTP_BCOPY(&ab_tlvPld[k_OFFS_GMTBL_MEMBERS+w_dynSize+4],
                    &ps_gmClustTbl[b_tblIdx].ab_Addr[0],
                    ps_gmClustTbl[b_tblIdx].w_AddrLen);
          /* set new dynamic size and position within the array */
          w_dynSize += (4+k_NETW_ADDR_LEN);
        }

        /* dynamic size must be even to get an even payload size,
           otherwise correct with an padding byte*/
        if((w_dynSize & 0x01) == 0x01)
        {
          /* add padding byte */
          ab_tlvPld[k_OFFS_GMTBL_MEMBERS+w_dynSize] = 0;
          w_dynSize++;
        }
        /* values ok */
        o_ret = TRUE;
      }
      else
      {
        /* wrong table size */
        PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
      }
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             w_dynSize );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_UCMasterTbl
**
** Description : This function handles the management message 
**               UNICAST_MASTER_TABLE. A GET request responds the 
**               current unicast master table including each entry. 
**               A SET command forces the node to reconfigure the 
**               unicast master table. Configuring an empty table 
**               (w_tblSize is zero and ps_ucMasTbl points to NULL) 
**               prompts the generation of a message, which causes 
**               the node to reset the unicast master table. Otherwise,
**               a list of port addresses (ps_ucMasTbl) will be 
**               transmitted and stored by the receiving node. If 
**               addressed to all ports, a response for each port must 
**               be issued. The GET and SET commands are both supported; 
**               however, the additional data parameters are only used 
**               within the SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               c_queryIntv    (IN) - mean interval for re-transmit
**                                     a not granted  request
**               w_tblSize      (IN) - amount of masters / table
**                                     entries
**               ps_ucMasTbl   (IN) - pointer to first table
**                                     entry (port address)
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 17.5.3
**
***********************************************************************/
BOOLEAN MNTapi_UCMasterTbl( const MNT_t_apiMsgConfig *ps_msgConfig,
                            const MNT_t_apiClbkData  *ps_clbkData,
                                  INT8               c_queryIntv,
                                  UINT16             w_tblSize,
                            const PTP_t_PortAddr     *ps_ucMasTbl )
{
  UINT8        ab_tlvPld[k_MAX_TLV_PLD_LEN];
  PTP_t_MntTLV s_mntTlv;
  UINT16       w_dynSize = 0;
  UINT8        b_tblIdx;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_UNICAST_MASTER_TABLE;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL; 
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* if unicast master table has to be cleared */
      if((w_tblSize == 0) && (ps_ucMasTbl == NULL))
      {
        /* generate TLV payload */
        ab_tlvPld[k_OFFS_UCMATBL_LQI]    = (UINT8)c_queryIntv;
        ab_tlvPld[k_OFFS_UCMATBL_SIZE+0] = 0;
        ab_tlvPld[k_OFFS_UCMATBL_SIZE+1] = 0;
        /* values ok */
        o_ret = TRUE;
      }
      /* if unicast master table has to be configured */
      else if( ((w_tblSize * ( 4+k_MAX_NETW_ADDR_SZE )) < (k_MAX_TLV_PLD_LEN-3))
                 && (ps_ucMasTbl != NULL) )
      {
        /* generate TLV payload */
        ab_tlvPld[k_OFFS_UCMATBL_LQI]  = (UINT8)c_queryIntv;
        GOE_hton16(&ab_tlvPld[k_OFFS_UCMATBL_SIZE], w_tblSize);

        /* copy port address table */
        for(b_tblIdx = 0; b_tblIdx < w_tblSize; b_tblIdx++)
        {
          GOE_hton16(&ab_tlvPld[k_OFFS_UCMATBL_TBL+w_dynSize],
                     (UINT16)ps_ucMasTbl[b_tblIdx].e_netwProt);
          GOE_hton16(&ab_tlvPld[k_OFFS_UCMATBL_TBL+w_dynSize+2],
                     (UINT16)ps_ucMasTbl[b_tblIdx].w_AddrLen);
          PTP_BCOPY(&ab_tlvPld[k_OFFS_UCMATBL_TBL+w_dynSize+4],
                    &ps_ucMasTbl[b_tblIdx].ab_Addr[0],
                    ps_ucMasTbl[b_tblIdx].w_AddrLen);
          /* set new dynamic size and position within the array 
              add 2 for network protocol
              add 2 for address length
              add address length */
          w_dynSize += (4+ps_ucMasTbl[b_tblIdx].w_AddrLen);
        }

        /* dynamic size must be odd to get an even payload size,
           otherwise correct with an padding byte*/
        if((w_dynSize & 0x01) == 0x00)
        {
          /* add padding byte */
          ab_tlvPld[k_OFFS_UCMATBL_TBL+w_dynSize] = 0;
          w_dynSize++;
        }
        /* values ok */
        o_ret = TRUE;
      }
      else
      {
        /* wrong table size */
        PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
      }
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             w_dynSize );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_UCMstMaxTblSize
**
** Description : This function handles the management message 
**               UNICAST_MASTER_MAX_TABLE_SIZE. This message prompts 
**               a response message including the maximum number of 
**               unicast master table entries of the requested port. 
**               If addressed to all ports, a response for each port 
**               must be issued. The only supported action command is 
**               GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 17.5.4
**
***********************************************************************/
BOOLEAN MNTapi_UCMstMaxTblSize( const MNT_t_apiMsgConfig *ps_msgConfig,
                                const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only GET available */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_UNICAST_MASTER_MAX_TABLE_SIZE;   
      s_mntTlv.pb_data = NULL;   

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_AcceptMstTbl
**
** Description : This function handles the management message 
**               ACCEPTABLE_MASTER_TABLE. A GET request prompts the
**               node to respond the current acceptable master table. 
**               With a SET request, new entries are configured. If 
**               addressed to all ports, a response for each port must
**               be issued. The GET and SET commands are both supported;
**               however, the additional data parameters (w_tblSize and
**               ps_accMstTbl) are only used within the SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               w_tblSize      (IN) - amount of table entries
**               ps_accMstTbl  (IN) - pointer to first table
**                                     entry of accept masters
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 17.6.3
**
***********************************************************************/
BOOLEAN MNTapi_AcceptMstTbl( const MNT_t_apiMsgConfig *ps_msgConfig,
                             const MNT_t_apiClbkData  *ps_clbkData,
                                   UINT16             w_tblSize,
                             const PTP_t_AcceptMaster *ps_accMstTbl )
{
  UINT8        ab_tlvPld[k_MAX_TLV_PLD_LEN];
  PTP_t_MntTLV s_mntTlv;
  UINT16       w_dynSize = 0;
  UINT8        b_tblIdx;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_ACCEPTABLE_MASTER_TABLE;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL; 
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* check amount of entries and buffer size */
      if( (w_tblSize * (6+k_MAX_NETW_ADDR_SZE)) < (k_MAX_TLV_PLD_LEN-2) )
      {
        if( ps_accMstTbl != NULL )
        {
          /* generate TLV payload */
          GOE_hton16(&ab_tlvPld[k_OFFS_ACMATBL_SIZE],w_tblSize);

          /* copy port address table */
          for(b_tblIdx = 0; b_tblIdx < w_tblSize; b_tblIdx++)
          {
            GOE_hton16(&ab_tlvPld[k_OFFS_ACMATBL_TBL+w_dynSize],
                       (UINT16)ps_accMstTbl[b_tblIdx].s_acceptAdr.e_netwProt);
            GOE_hton16(&ab_tlvPld[k_OFFS_ACMATBL_TBL+w_dynSize+2],
                       (UINT16)ps_accMstTbl[b_tblIdx].s_acceptAdr.w_AddrLen);
            PTP_BCOPY(&ab_tlvPld[k_OFFS_ACMATBL_TBL+w_dynSize+4],
                      &ps_accMstTbl[b_tblIdx].s_acceptAdr.ab_Addr[0],
                      ps_accMstTbl[b_tblIdx].s_acceptAdr.w_AddrLen);
            ab_tlvPld[k_OFFS_ACMATBL_TBL+4+
                      ps_accMstTbl[b_tblIdx].s_acceptAdr.w_AddrLen+w_dynSize] = 
                      ps_accMstTbl[b_tblIdx].b_altPrio1;
            /* set new dynamic size and position within the array 
                add 2 for network protocol
                add 2 for address length
                add 1 for priority
                add address length */
            w_dynSize += (5 + ps_accMstTbl[b_tblIdx].s_acceptAdr.w_AddrLen);
          }

          /* dynamic size must be even to get an even payload size,
             otherwise correct with an padding byte*/
          if((w_dynSize & 0x01) == 0x01)
          {
            /* add padding byte */
            ab_tlvPld[k_OFFS_ACMATBL_TBL+w_dynSize] = 0;
            w_dynSize++;
          }
          o_ret = TRUE;
        }
        else
        {
          /* wrong table size - too many entries for TLV payload size */
          PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
        }
      }
      else
      {
        /* wrong table size - too many entries for TLV payload size */
        PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
      }
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             w_dynSize );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_AcceptMstTblEna
**
** Description : This function handles the management message 
**               ACCEPTABLE_MASTER_TABLE_ENABLED. This message 
**               enables or disables the acceptable master 
**               functionality of a node interface. A GET request
**               gets the configuration of the enable flag; a SET 
**               request configures the node. If addressed to all 
**               ports, a response for each port must be issued. 
**               The GET and SET commands are both supported; 
**               however, the additional data parameter (o_enabled) 
**               is only used within the SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               o_enabled      (IN) - TRUE  -> acceptable MST enabled
**                                     FALSE -> not enabled
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 17.6.5
**
***********************************************************************/
BOOLEAN MNTapi_AcceptMstTblEna( const MNT_t_apiMsgConfig *ps_msgConfig,
                                const MNT_t_apiClbkData  *ps_clbkData,
                                      BOOLEAN            o_enabled)
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_ACCEPTABLE_MASTER_TABLE_ENABLED;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_ACMAENA_FLAGS] = 0; /* reset flags */
      SET_FLAG(ab_tlvPld[k_OFFS_ACMAENA_FLAGS],
               k_OFFS_ACMAENA_FLAGS_EN,
               o_enabled); 
      ab_tlvPld[k_OFFS_ACMAENA_RES] = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_AcceptMstMaxTblSize
**
** Description : This function handles the management message 
**               ACCEPTABLE_MASTER_MAX_TABLE_SIZE. This message 
**               prompts a response message including the maximum 
**               number of acceptable master table entries of the 
**               requested port. If addressed to all ports, a 
**               response for each port must be issued. The only
**               supported action command is GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 17.6.4
**
***********************************************************************/
BOOLEAN MNTapi_AcceptMstMaxTblSize( const MNT_t_apiMsgConfig *ps_msgConfig,
                                    const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_ACCEPTABLE_MASTER_MAX_TABLE_SIZE;   
      s_mntTlv.pb_data = NULL;   

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_AltMaster
**
** Description : This function handles the management message 
**               ALTERNATE_MASTER. This message gets or sets the 
**               attributes of the optional alternate master 
**               mechanism. A GET request responds the current 
**               values; a SET request configures the target. If
**               addressed to all ports, a response for each port
**               must be issued. The GET and SET commands are both
**               supported; however, the additional data are only
**               used within the SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               o_txSync       (IN) - TRUE  -> alternate sync msg
**                                     FALSE -> no alt. sync msg
**               c_syncIntv     (IN) - log alternate multicast 
**                                     sync interval
**               b_numOfAltMas  (IN) - amount of alternate masters
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 17.4.3
**
***********************************************************************/
BOOLEAN MNTapi_AltMaster( const MNT_t_apiMsgConfig *ps_msgConfig,
                          const MNT_t_apiClbkData  *ps_clbkData,
                                BOOLEAN            o_txSync,
                                INT8               c_syncIntv,
                                UINT8              b_numOfAltMas )
{
  UINT8        ab_tlvPld[4];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_ALTERNATE_MASTER;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      ab_tlvPld[k_OFFS_ALTMA_FLAGS] = 0; /* reset flags */
      /* generate TLV payload */
      SET_FLAG(ab_tlvPld[k_OFFS_ALTMA_FLAGS],
               k_OFFS_ALTMA_FLAGS_S,
               o_txSync); 
      ab_tlvPld[k_OFFS_ALTMA_LAMSI] = (UINT8)c_syncIntv;
      ab_tlvPld[k_OFFS_ALTMA_NUM]   = b_numOfAltMas;
      ab_tlvPld[k_OFFS_ALTMA_RES]   = 0;

      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}



/***********************************************************************
**
** Function    : MNTapi_AltTimeOffsEna
**
** Description : This function handles the management message 
**               ALTERNATE_TIME_OFFSET_ENABLE. This message enables
**               or disables the optional alternate time offset 
**               functionality of a node. The GET and SET commands
**               are both supported; however, the additional data 
**               parameters (o_altTOEna) are only used within a 
**               SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_Key          (IN) - the number of the timescale
**               o_altTOEna     (IN) - TRUE  -> enables functionality
**                                     FALSE -> disables functionality
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 16.3.4
**
***********************************************************************/
BOOLEAN MNTapi_AltTimeOffsEna( const MNT_t_apiMsgConfig *ps_msgConfig,
                               const MNT_t_apiClbkData  *ps_clbkData,
                                     UINT8              b_Key,
                                     BOOLEAN            o_altTOEna )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_ALTERNATE_TIME_OFFSET_ENABLE;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_ALTENA_KEY] = b_Key;
      ab_tlvPld[k_OFFS_ALTENA_FLAGS] = 0; /* reset flags */
      SET_FLAG(ab_tlvPld[k_OFFS_ALTENA_FLAGS],
               k_OFFS_ALTENA_FLAGS_EN,
               o_altTOEna); 
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_AltTimeOffsName
**
** Description : This function handles the management message 
**               ALTERNATE_TIME_OFFSET_NAME. This message configures 
**               the timescale offset description attributes for an 
**               alternate timescale. The GET and SET commands are 
**               both supported; however, the additional data 
**               parameters (b_maxKey, ps_dispTxt) are only used 
**               within a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_Key          (IN) - the number of the timescale
**               ps_dispTxt    (IN) - display text of timescale
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 16.3.5
**
***********************************************************************/
BOOLEAN MNTapi_AltTimeOffsName( const MNT_t_apiMsgConfig *ps_msgConfig,
                                const MNT_t_apiClbkData  *ps_clbkData,
                                      UINT8              b_Key,
                                const PTP_t_Text         *ps_dispTxt )
{
  UINT8        ab_tlvPld[12];
  PTP_t_MntTLV s_mntTlv;
  UINT16       w_dynSize = 0;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_ALTERNATE_TIME_OFFSET_NAME;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      w_dynSize = 0;
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* check paramter */
      if( ps_dispTxt != NULL )
      {
        if( (ps_dispTxt->pc_text != NULL) && (ps_dispTxt->b_len != 0) )
        {
          if(ps_dispTxt->b_len <= 10)
          {
            /* generate TLV payload */
            ab_tlvPld[k_OFFS_ALTNME_KEY] = b_Key;
            ab_tlvPld[k_OFFS_ALTNME_NAME_SIZE] = ps_dispTxt->b_len;
            PTP_BCOPY(&ab_tlvPld[k_OFFS_ALTNME_NAME_TXT],    
                      ps_dispTxt->pc_text,    
                      ps_dispTxt->b_len);  
            /* calculate size of dynamic members */
            w_dynSize = ps_dispTxt->b_len + 1;
            /* get padding data */
            if((w_dynSize & 0x01) == 0x00)
            {
              /* add padding byte */
              w_dynSize++;
              ab_tlvPld[w_dynSize] = 0;
            }
            /* set payload pointer in TLV structure */
            s_mntTlv.pb_data = ab_tlvPld;  
            o_ret = TRUE;
          }
          else
          {
            /* wrong text length */
            PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
          }
        }
        else
        {
          /* wrong text length */
          PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
        }
      }
      else
      {
        /* no display text available */
        PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
      }
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             w_dynSize );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_AltTimeOffsKey
**
** Description : This function handles the management message 
**               ALTERNATE_TIME_OFFSET_MAX_KEY. This message gets 
**               the number of maintained alternate timescales 
**               within a node. The only supported action command 
**               is GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 16.3.6
**
***********************************************************************/
BOOLEAN MNTapi_AltTimeOffsKey( const MNT_t_apiMsgConfig *ps_msgConfig,
                               const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only GET available */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_ALTERNATE_TIME_OFFSET_MAX_KEY;   
      s_mntTlv.pb_data = NULL;   

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_AltTimeOffsProp
**
** Description : This function handles the management message 
**               ALTERNATE_TIME_OFFSET_PROPERTIES. This message gets
**               or configures timescale offset attributes for an 
**               alternate timescale. The GET and SET commands are
**               both supported; however, the additional data 
**               parameters are only used within a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_Key          (IN) - the number of the timescale
**               l_curOffset    (IN) - current offset of the alternate
**                                     time
**               l_jmpSeconds   (IN) - size of the next discontinuity
**               ddw_timeNxJump (IN) - time of next occurrence of a 
**                                     discontinuity
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Only the first 48 bit of ddw_timeNxJump are used!
**               Cross-reference to the IEEE 1588 standard; 
**               see also chapter 16.3.7
**
***********************************************************************/
BOOLEAN MNTapi_AltTimeOffsProp( const MNT_t_apiMsgConfig *ps_msgConfig,
                                const MNT_t_apiClbkData  *ps_clbkData,
                                      UINT8              b_Key,
                                      INT32              l_curOffset,
                                      INT32              l_jmpSeconds,
                                      UINT64             ddw_timeNxJump)
{
  UINT8        ab_tlvPld[16];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_ALTERNATE_TIME_OFFSET_PROPERTIES;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_ALTPROP_KEY] = b_Key;
      GOE_hton32(&ab_tlvPld[k_OFFS_ALTPROP_OFFS],(UINT32)l_curOffset);
      GOE_hton32(&ab_tlvPld[k_OFFS_ALTPROP_JMP],(UINT32)l_jmpSeconds);
      GOE_hton48(&ab_tlvPld[k_OFFS_ALTPROP_TNEXT],&ddw_timeNxJump);
      ab_tlvPld[k_OFFS_ALTPROP_RES] = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_TCDefaultDs
**
** Description : This function handles the management message 
**               TRANSPARENT_CLOCK_DEFAULT_DATA_SET. This management
**               message prompts a response including the default 
**               data set members of the transparent clock. The only 
**               supported action command is GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.8.1
**
***********************************************************************/
BOOLEAN MNTapi_TCDefaultDs( const MNT_t_apiMsgConfig *ps_msgConfig,
                            const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only GET available */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_TRANSPARENT_CLOCK_DEFAULT_DATA_SET;   
      s_mntTlv.pb_data = NULL;   

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_TCPortDs
**
** Description : This function handles the management message 
**               TRANSPARENT_CLOCK_PORT_DATA_SET. This management
**               message prompts a response including the port data 
**               set members of the transparent clock. The only 
**               supported action command is GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.10.1
**
***********************************************************************/
BOOLEAN MNTapi_TCPortDs( const MNT_t_apiMsgConfig *ps_msgConfig,
                         const MNT_t_apiClbkData  *ps_clbkData )
{
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* only GET available */
    if( (ps_msgConfig->b_actionFld == k_ACTF_GET) )
    {
      /* set TLV values */
      s_mntTlv.w_mntId = k_TRANSPARENT_CLOCK_PORT_DATA_SET;   
      s_mntTlv.pb_data = NULL;   

      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_PrimaryDomain
**
** Description : This function handles the management message 
**               PRIMARY DOMAIN. This management message prompts a
**               response including the port data set members of the
**               transparent clock. The GET and SET commands are both
**               supported; however, the additional data parameter 
**               (b_primaryDom) is only used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_primaryDom   (IN) - value of primary domain
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.9.1
**
***********************************************************************/
BOOLEAN MNTapi_PrimaryDomain( const MNT_t_apiMsgConfig *ps_msgConfig,
                              const MNT_t_apiClbkData  *ps_clbkData,
                                    UINT8              b_primaryDom )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_PRIMARY_DOMAIN;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;  
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_PRIM_DOM] = b_primaryDom; 
      ab_tlvPld[k_OFFS_PRIM_RES] = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_DelayMechanism
**
** Description : This function handles the management message
**               DELAY_MECHANISM. This message gets the configured
**               delay mechanism of a port. It may be used to 
**               configure the delay mechanism, but this behavior 
**               is out of the scope of the standard. If a request 
**               is addressed to all ports, a response message for 
**               each port must be issued. The GET and SET commands
**               are both supported; however, the additional data 
**               parameter (b_delMech) is only used with a SET 
**               command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_delMech     (IN) - value of delay mechanism
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.7.5 / 15.5.3.9
**
***********************************************************************/
BOOLEAN MNTapi_DelayMechanism( const MNT_t_apiMsgConfig *ps_msgConfig,
                               const MNT_t_apiClbkData  *ps_clbkData,
                                     UINT8              b_delMech )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_DELAY_MECHANISM;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL; 
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_DELM_MECH] = b_delMech; 
      ab_tlvPld[k_OFFS_DELM_RES]  = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_MinPDelReqIntv
**
** Description : This function handles the management message 
**               LOG_MIN_PDELAY_REQ_INTERVAL. This message gets or 
**               updates the minimum peer delay request interval 
**               member of the port data set. If a request is 
**               addressed to all ports, a response message for 
**               each port must be issued. The GET and SET 
**               commands are both supported; however, the 
**               additional data parameter (c_pDelReqIntv) is only
**               used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               c_pDelReqIntv  (IN) - value of log min pdelay
**                                     request interval
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - an error occurred
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.10.2
**
***********************************************************************/
BOOLEAN MNTapi_MinPDelReqIntv( const MNT_t_apiMsgConfig *ps_msgConfig,
                               const MNT_t_apiClbkData  *ps_clbkData,
                                     UINT8              c_pDelReqIntv )
{
  UINT8        ab_tlvPld[2];
  PTP_t_MntTLV s_mntTlv;
  BOOLEAN      o_ret = FALSE;

  /* ckeck paramters */
  if( (ps_msgConfig != NULL) && (ps_clbkData != NULL) )
  {
    /* set TLV values */
    s_mntTlv.w_mntId = k_LOG_MIN_MEAN_PDELAY_REQ_INTERVAL;  

    if( ps_msgConfig->b_actionFld == k_ACTF_GET )
    {
      s_mntTlv.pb_data = NULL;
      o_ret = TRUE;
    }
    else if( ps_msgConfig->b_actionFld == k_ACTF_SET )
    {
      /* generate TLV payload */
      ab_tlvPld[k_OFFS_LMPDRI_INTV] = (UINT8)c_pDelReqIntv; 
      ab_tlvPld[k_OFFS_LMPDRI_RES]  = 0;
      /* set payload pointer in TLV structure */
      s_mntTlv.pb_data = ab_tlvPld;  
      o_ret = TRUE;
    }
    else
    {
      /* unknown action command */
      PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_UNKN_ACOM,e_SEVC_NOTC); 
    }

    if( o_ret == TRUE )
    {
      o_ret = HandleRequest( ps_msgConfig, 
                             ps_clbkData,
                             &s_mntTlv,
                             0 );
    }
  }
  else
  {
    /* no callback and/or message data available */
    PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_WRONG_VALUE,e_SEVC_NOTC); 
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : MNTapi_GatherAnswers
**
** Description : All RESPONSE and ACKNOWLEDGE messages addressed
**               to this clock are issued upon an API request and
**               therefore passed to this function. This function
**               determines the corresponding caller and copies all
**               received information into the appreciate buffer.
**
** Parameters  : ps_msg    (IN) - Received message out of the 
**                                Message Box
**               ps_mntMsg (IN) - direct pointer mbox entry
**               ps_mntTlv (IN) - direct pointer mnt TLV
**               w_tblIdx  (IN) - mnt info table index 
**                                corresonding to the mnt msg
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNTapi_GatherAnswers( const PTP_t_MboxMsg      *ps_msg,
                           const NIF_t_PTPV2_MntMsg *ps_mntMsg,
                           const PTP_t_MntTLV       *ps_mntTlv,
                                 UINT16             w_tblIdx )
{
  MNT_t_apiTblEntry  *ps_apiTblData;
  MNT_t_apiClbkData  *ps_apiClbkData;
  MNT_t_apiReturnTLV s_apiReturnTlv;
  MNT_t_apiStateEnum e_curState;

  /* external responses are passed with the table index, 
     for internal reponses the index must be determined */
  if(ps_msg->s_etc3.dw_u32 == k_MNT_MSG_INT)
  {
    /* if an error status TLV arrived */
    if( ps_mntTlv->w_type == (UINT16)e_TLVT_MNT_ERR_STS )
    {
      /* return value must not be checked, this is done before */
      MNT_GetTblIdx( GOE_ntoh16(&ps_mntTlv->pb_data[0]),
                     &w_tblIdx );   /*lint !e534 */
    }
    /* a normal management message arrived */
    else
    {
      /* return value must not be checked, this is done before */
      MNT_GetTblIdx(ps_mntTlv->w_mntId, &w_tblIdx);   /*lint !e534 */
    }
  }
  
  /* get table information */
  ps_apiTblData = s_MntInfoTbl[w_tblIdx].ps_apiData;

  if(ps_apiTblData != NULL)
  {
    /* if message sequence id matches the sequence id of the request */
    if(ps_mntMsg->s_ptpHead.w_seqId == ps_apiTblData->w_seqId)
    {
      /* copy message information */
      s_apiReturnTlv.s_srcPortId    = ps_mntMsg->s_ptpHead.s_srcPortId;
      s_apiReturnTlv.s_mntTlv       = *ps_mntTlv;
      s_apiReturnTlv.w_sizeOfTlvPld = ps_mntTlv->w_len - 2;

      /* get callback information */
      ps_apiClbkData = (MNT_t_apiClbkData*)&(ps_apiTblData->s_clbkData); 

      /* check, if only one answer is expected */
      if(ps_apiTblData->o_oneMsg == TRUE)
      {
        /* delete management info table entry and free buffer */
        SIS_Free(ps_apiTblData);
        s_MntInfoTbl[w_tblIdx].ps_apiData = NULL;
        e_curState = e_MNTAPI_DONE;
      }
      else
      {
        /* more than one answer expected */
        /* table entry must be freed within MNT timer */
        e_curState = e_MNTAPI_PENDING;
      }

      /* call application */
      if(ps_apiClbkData->pf_clbkFunc != NULL)
      {
        ps_apiClbkData->pf_clbkFunc(ps_apiClbkData->pdw_cbHandle,
                                    &s_apiReturnTlv,
                                    e_curState );
      }
      else
      {
        /* no callback function available */
        PTP_SetError(k_MNT_ERR_ID,MNTAPI_k_ERR_NO_CLBK,e_SEVC_NOTC);
      }
    }
    else
    {
      /* wrong sequence id - discard message */
    }
  }
  else
  {
    /* no request for this management id available (any more) */
  }

  return;
}

/*******************************************************************************
**    static functions
*******************************************************************************/

/***********************************************************************
**
** Function    : HandleRequest
**
** Description : This function is responsible to generate all common 
**               information for an API request. This, the port is 
**               checked permitting a request on port zero. TLV length
**               is also calculated out of the given data. All 
**               information is passed to the next function, which is 
**               responsible for putting the request into the message 
**               box of the MNT task.
**
** See Also    : SendApiReq()
**
** Parameters  : ps_msgConfig (IN) - message configurations
**               ps_clbkData  (IN) - application callback infos
**               ps_mntTlv    (IN) - mnt TLV / request
**               w_dynSize    (IN) - dynamic data size of TLV payload
**
** Returnvalue : TRUE  - success 
**               FALSE - failure
**
** Remarks     : -
**
***********************************************************************/
static BOOLEAN HandleRequest( const MNT_t_apiMsgConfig *ps_msgConfig,
                              const MNT_t_apiClbkData  *ps_clbkData,
                                    PTP_t_MntTLV       *ps_mntTlv,
                                    UINT16             w_dynSize )
{
  UINT16  w_tblIdx;
  BOOLEAN o_ret = TRUE;
  void* pv_ctx;

  /* check port adress limitation - if zero, return failure */
  if( ps_msgConfig->s_desPortId.w_portNmb == 0 )
  {
    /* port zero is not adressable */
    PTP_SetError(k_MNT_ERR_ID,MNTAPT_k_ERR_ZERO_PORT,e_SEVC_NOTC); 
    o_ret = FALSE;
  }
  else
  {
    /* determine table index */
    if( MNT_GetTblIdx( ps_mntTlv->w_mntId, &w_tblIdx ) == TRUE )
    {
      /* set TLV type to management */
      ps_mntTlv->w_type  = (UINT16)e_TLVT_MNT;
      /* calculate MNT TLV length */
      if( ps_msgConfig->b_actionFld == k_ACTF_GET )
      {

        /* set TLV size of TLV value */
        ps_mntTlv->w_len = k_MNT_TLV_LEN_MIN;
      }
      else
      {
        /* set TLV size of TLV value */
        ps_mntTlv->w_len = k_MNT_TLV_LEN_MIN 
                           + s_MntInfoTbl[w_tblIdx].w_tlvPld
                           + w_dynSize;
      }
      /* lock stack */
      pv_ctx = GOE_LockStack();
      /* set API request in stack */
      o_ret = SendApiReq(ps_msgConfig,ps_clbkData,ps_mntTlv);
      /* unlock stack */
      GOE_UnLockStack(pv_ctx);
    }
    else
    {
      /* no table entry for such managemente id - should not occur */
      o_ret = FALSE;
    }
  }
  return o_ret;
}

/*******************************************************************************
**
** Function    : SendApiReq
**
** Description : This function passes the request to the MNT task using the
**               message box. First all buffers are allocated. On success a 
**               message box entry is generated including a whole management 
**               TLV.
**
** See Also    : HandleRequest()
**
** Parameters  : ps_msgConfig (IN) - message configurations
**               ps_clbkData  (IN) - application callback infos
**               ps_mntTlv    (IN) - mnt TLV / request
**
** Returnvalue : TRUE  - success 
**               FALSE - failure
**
*******************************************************************************/
static BOOLEAN SendApiReq( const MNT_t_apiMsgConfig *ps_msgConfig,
                           const MNT_t_apiClbkData  *ps_clbkData,
                           const PTP_t_MntTLV       *ps_mntTlv)
{
  MNT_t_apiMboxData *ps_mboxData;
  PTP_t_MboxMsg     s_msg;
  UINT8             *pb_tlvPld = NULL;
  BOOLEAN           o_ret = FALSE;

  /* recheck, if the application-space pointers are OK */
  if((ps_msgConfig != NULL) && (ps_clbkData != NULL))
  {
    /* check, if stack is not in re-init state */
    if( o_initialize == FALSE )
    {
      /* allocate buffer for mbox entry */
      ps_mboxData = (MNT_t_apiMboxData*)SIS_Alloc(sizeof(MNT_t_apiMboxData));
      if(ps_mboxData != NULL)
      {
        /* only if TLV payload available */
        if( ps_mntTlv->pb_data != NULL )
        {
          /* allocate buffer for TLV payload */
          pb_tlvPld = (UINT8*)(SIS_Alloc(ps_mntTlv->w_len-2));
          if( pb_tlvPld != NULL )
          {
            /* copy tlv payload into buffer */
            PTP_BCOPY((UINT8*)pb_tlvPld,         
                      (UINT8*)ps_mntTlv->pb_data, 
                      ps_mntTlv->w_len-2);    
            o_ret = TRUE;
          }
          else
          {
            /* error: could not allocate buffer for tlv payload */
            o_ret = FALSE;
          }
        }
        else
        {
          /* no TLV payload (thus a GET request) */
          o_ret = TRUE;
        }
      }
      else
      {
        /* error: could not allocate buffer for mbox entry */
        o_ret = FALSE;
      }

      /* no error occured -> send to MNT task */
      if( (o_ret == TRUE) && ( ps_mboxData != NULL) )
      {
        /* copy configuration data */
        ps_mboxData->s_msgConfig = *ps_msgConfig;
        /* copy callback data */
        ps_mboxData->s_clbkData  = *ps_clbkData;

        /* copy generated TLV data */
        ps_mboxData->s_mntTlv.w_len     = ps_mntTlv->w_len;
        ps_mboxData->s_mntTlv.w_mntId   = ps_mntTlv->w_mntId;
        ps_mboxData->s_mntTlv.w_type    = ps_mntTlv->w_type;
        ps_mboxData->s_mntTlv.pb_data   = pb_tlvPld;

        /* set mbox-entry parameters */
        s_msg.s_etc1.dw_u32     = 0;                   /* not used */
        s_msg.s_etc2.dw_u32     = 0;                   /* not used */
        s_msg.s_etc3.dw_u32     = 0;                   /* not used */
        s_msg.s_pntData.pv_data = (void*)ps_mboxData;  /* mnt tlv data */
        s_msg.s_pntExt1.pv_data = NULL;                /* not used */
        s_msg.s_pntExt2.pv_data = NULL;                /* not used */
        s_msg.e_mType  = e_IMSG_MM_API;   /* API request to be handled */

        /* send to management task */
        if( SIS_MboxPut(MNT_TSK,&s_msg) == FALSE )
        {
          /* error: request could not send to task */
          o_ret = FALSE;
        }
      }
      else
      {
        o_ret = FALSE;
      }
      /* free allocated buffer, if an error occurs */
      if(o_ret == FALSE)
      {
        if(ps_mboxData != NULL)
        {
          /* free generated MNT message */
          SIS_Free(ps_mboxData);
        }
        if( pb_tlvPld != NULL )
        {
          /* free tlv payload memory */
          SIS_Free(pb_tlvPld);
        }
      }
    }
    else
    {
      /* request is not possible at the moment, while re-init */
      o_ret = FALSE;
    }
  }
  else
  {
    /* invalid pointers */
    o_ret = FALSE;
  }
  return o_ret;
}

#endif /* ifdef MNT_API */
