/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: MNTint.h 
**    Summary: Declares all internal functions, types and variables of the 
**             unit MNT.
**             The unit MNT handles all management messages 
**             and reacts to them as specified in the
**             standard IEEE 1588.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: MNT_GetTblIdx
**             MNT_GenPortLoopData
**             MNTmsg_HandleGet
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
**             MNTget_ClkDesc
**             MNTget_UsrDesc
**             MNTget_FaultLog
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
**
**   Compiler: Ansi-C
**    Remarks:
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __MNTINT_H__
#define __MNTINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/

#define k_MNT_TLV_LEN_MIN      (2) /* min size, no TLV payload */


/*************************************************************************
**    data types
*************************************************************************/

/************************************************************************/
/** MNT_t_initKeyEnum
             Defines all initialization keys used by the INITIALIZ
             management message.
*/
typedef enum
{
  e_INITIALZE_EVENT = 0x0000  /* causes init in OCs or BCs */
  /* keys (0x0001 - 0x7FFF) are reserved */
  /* keys (0x8000 - 0xFFFF) are implementation specific */
}MNT_t_initKeyEnum;


/************************************************************************/
/** MNT_t_apiMboxData
             This structure defines a temporary storage for all
             information to send a MNT messages from the API to
             an external node. This must be passed to the MNT_Task
             via the SIS Message Box. The MNT TLV contains all TLV 
             information, whereas addressing and configuration of
             the message for the remote node is done by the config
             structure.
*/
typedef struct
{
  /* message configuration and addressing */
  MNT_t_apiMsgConfig s_msgConfig;
  /* callback data  */
  MNT_t_apiClbkData  s_clbkData;
  /* the generated TLV */
  PTP_t_MntTLV       s_mntTlv;
}MNT_t_apiMboxData;


/************************************************************************/
/** MNT_t_apiTblEntry
             This structure is used to remenber all necessary data
             to fulfil the API request. 
*/
typedef struct
{
  /* application defined callback data and configuration */
  MNT_t_apiClbkData  s_clbkData;
  /* sequence id of request */
  UINT16             w_seqId;
  /* only one response expected */
  BOOLEAN            o_oneMsg;
  /* timeout counter */
  UINT16             w_timeout;
  /* current state */
  MNT_t_apiStateEnum e_reqState;
}MNT_t_apiTblEntry;


/************************************************************************/
/** MNT_t_MsgInfoTable
             Defines the structure of an management info table item.
             This item holds values and information to realize 
             certain accesses due to a corresponding management 
             message indicated by the management msg id w_mntId.
*/
typedef struct
{
  /* IEEE defined management msg id */
  const UINT16 w_mntId;
  /* addressable to all ports */
  const BOOLEAN o_allPorts;
  /* size of message payload for GET responses */ 
  const UINT8 w_tlvPld;
  /* pointer to API request structure */
  MNT_t_apiTblEntry *ps_apiData;
}MNT_t_MsgInfoTable;

/* define for last table entry */
#define k_MNT_TABLE_END    (0xFFFF)

/************************************************************************/
/** MNT_t_FltRecItem
             This defines a structure of a fault record item hold
             in a queue of fault records. This item must save all
             needed values to generate a fault log management msg.
             Therefore each available fault data is saved.
*/
typedef struct
{
  PTP_t_TmStmp    s_tsErr;     /* time of the occurance of the failure */
  PTP_t_sevCdEnum e_sevCode;   /* severity code of the failure */
  UINT32          dw_errNmb;   /* error number given by the unit */
  UINT32          dw_unitId;   /* unit, where this failure occured */
}MNT_t_FltRecItem;

/*************************************************************************
**    global variables
*************************************************************************/

/* pointer to clock data set */
extern const PTP_t_ClkDs *MNT_ps_ClkDs;
#if( k_UNICAST_CPBL == TRUE )
/* pointer to unicast master table */
extern const PTP_t_PortAddrQryTbl *MNT_ps_ucMstTbl;
#endif /* if( k_UNICAST_CPBL == TRUE ) */
/* channel port ids */
extern PTP_t_PortId MNT_as_pId[k_NUM_IF];
/* number of interfaces */
extern UINT32 MNT_dw_amntNetIf;
/* mnt msg info table */
extern MNT_t_MsgInfoTable s_MntInfoTbl[];
/* number of table entries */
extern UINT16 w_numTblEntries;
/* data array for response or acknowledge generation */
extern UINT8 ab_RespTlvPld[k_MAX_TLV_PLD_LEN];
#ifdef MNT_API
  /* flag to indicate initializing state during re-init */
  extern BOOLEAN o_initialize;
#endif /* #ifdef MNT_API */

/*************************************************************************
**    function prototypes of MNTmain.c
*************************************************************************/

/*************************************************************************
**
** Function    : MNT_GetTblIdx
**
** Description : This function is responsible for the table index
**               of the management information table. The return
**               value is the table index of a given management id.
**
** Parameters  : w_mntId    (IN) - management id of searched table
**                                 position
**               pw_tblIdx (OUT) - tabel index
**               
** Returnvalue : TRUE  - successful found table entry
**               FALSE - no table entry of such id found
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN MNT_GetTblIdx( UINT16 w_mntId, UINT16 *pw_tblIdx );

/***********************************************************************
**
** Function    : MNT_GenPortLoopData
**
** Description : This function determines the start and end index
**               of the port loop, if all ports are addressed.
**
** Parameters  : ps_msg      (IN)  - message box entry
**               ps_mntMsg   (IN)  - received mnt message
**               w_tblIdx    (IN)  - mnt table index
**               pw_startIdx (OUT) - start index of loop
**               pw_endIdx   (OUT) - end index of loop
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNT_GenPortLoopData( const PTP_t_MboxMsg       *ps_msg,
                          const NIF_t_PTPV2_MntMsg  *ps_mntMsg,
                          UINT16                    w_tblIdx,
                          UINT16                    *pw_startIdx,
                          UINT16                    *pw_endIdx );
                          
/*************************************************************************
**    function prototypes of MNTmsg.c
*************************************************************************/

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
                             UINT16             w_ifIdx );

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
                                UINT16              w_mntTblIdx);

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
                                UINT16              w_mntTblIdx);

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
                              UINT16              w_mntTblIdx );

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
                         UINT16 w_dynDataSize);

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
void MNTmsg_GenAck( PTP_t_MboxMsg *ps_msg );

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
                            PTP_t_mntErrIdEnum e_errId);

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
                       PTP_t_mntErrIdEnum e_errType);

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
                      UINT8         b_actField);

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
void MNTmsg_InitFltRec( void );

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
void MNTmsg_PutFltRec(const MNT_t_FltRecItem *ps_fltRecItem);

/*************************************************************************
**    function prototypes of MNTget.c
*************************************************************************/

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
                        PTP_t_mntErrIdEnum *pe_mntErr );

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
BOOLEAN MNTget_UsrDesc( UINT16 *pw_dynSize );

/***********************************************************************
**
** Function    : MNTget_FaultLog
**
** Description : This function handles the management message
**               FAULT_LOG.
**
** Parameters  : w_ifIdx      (IN) - port number as array index
**               *pw_dynSize (OUT) - dynamic size of payload
**               *pe_mntErr  (OUT) - mnt error id
**
** Returnvalue : TRUE  - success
**               FALSE - failed
**
** Remarks     : -
**
***********************************************************************/
BOOLEAN MNTget_FaultLog( UINT16 w_ifIdx, UINT16 *pw_dynSize,
                         PTP_t_mntErrIdEnum *pe_mntErr );

#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE) )
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
BOOLEAN MNTget_DefaultDs( void );

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
BOOLEAN MNTget_CurrentDs( void );

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
BOOLEAN MNTget_ParentDs( void );

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
BOOLEAN MNTget_TimePropDs( void );

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
BOOLEAN MNTget_PortDs( UINT16 w_ifIdx );

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
BOOLEAN MNTget_Priority1( void );

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
BOOLEAN MNTget_Priority2( void );

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
BOOLEAN MNTget_Domain( void );

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
BOOLEAN MNTget_SlaveOnly( void );

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
BOOLEAN MNTget_AnncIntv( UINT16 w_ifIdx );

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
BOOLEAN MNTget_AnncRxTimeout( UINT16 w_ifIdx );

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
BOOLEAN MNTget_SyncIntv( UINT16 w_ifIdx );

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
BOOLEAN MNTget_VersionNumber( UINT16 w_ifIdx );

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
BOOLEAN MNTget_Time( PTP_t_mntErrIdEnum *pe_mntErr );

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
BOOLEAN MNTget_ClkAccuracy( void );

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
BOOLEAN MNTget_UtcProp( void );

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
BOOLEAN MNTget_TraceProp( void );

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
BOOLEAN MNTget_TimeSclProp( void );


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
BOOLEAN MNTget_UcNegoEnable( void );

#if( k_UNICAST_CPBL == TRUE)
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
BOOLEAN MNTget_UCMasterTbl( UINT16             *pw_dynSize,
                            PTP_t_mntErrIdEnum *pe_mntErr );

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
BOOLEAN MNTget_UCMstMaxTblSize( void );

#endif /* #if( k_UNICAST_CPBL == TRUE) */
#endif /* if OC or BC == TRUE */

#if( k_CLK_IS_TC == TRUE )
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
BOOLEAN MNTget_TCDefaultDs( void );

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
BOOLEAN MNTget_TCPortDs( UINT16 w_ifIdx );
#endif /* if TC == TRUE ) */

#if((k_CLK_IS_OC == FALSE) || (k_CLK_IS_BC == FALSE) || (k_CLK_IS_TC == FALSE))
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
BOOLEAN MNTget_DelayMechanism( UINT16 w_ifIdx );

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
BOOLEAN MNTget_MinPDelReqIntv( UINT16 w_ifIdx );
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
                              PTP_t_mntErrIdEnum *pe_mntErr );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __MNTINT_H__ */

