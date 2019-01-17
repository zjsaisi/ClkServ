/*******************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
********************************************************************************
**
**       File: MNTapi.h
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
**             is not responded to during certain duration or addressed to
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
**             
**
**   Compiler: Ansi-C
**    Remarks:  
**
**
********************************************************************************
**    all rights reserved, Template Version 1
*******************************************************************************/

#ifndef __MNTAPI_H__
#define __MNTAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**    constants and macros
*******************************************************************************/

/************************************************************************/
/** MNTapi TLV payload data
             The following defines are used to access the TLV payload,
             returned upon a management request. All defines for each
             message adhere to the TLV data field specification of the
             IEEE 1588 standard and can be used as index variables to
             access the TLV data array. For more information, refer to
             the IEEE1588-2008 standard. 
*/
/* offset values of TLV payload data */
/* USER_DESCRIPTION */
#define k_OFFS_UD_SIZE           (0) /* size of user desc. */
#define k_OFFS_UD_TEXT           (1) /* start of user desc. text */
/* INITIALIZE */
#define k_OFFS_INIT_KEY          (0) /* initialization key */
/* FAULT_LOG */
#define k_OFFS_FLTLOG_NUM        (0) /* number of fault records */
#define k_OFFS_FLTLOG_LOGS       (2) /* start of fault log list */
/* DEFAULT_DATA_SET */
#define k_OFFS_DFDS_FLAGS        (0) /* TSC and SO Flags */
#define k_OFFS_DFDS_RES1         (1) /* reserved byte */
#define k_OFFS_DFDS_PNUM         (2) /* number of ports */
#define k_OFFS_DFDS_PRIO1        (4) /* priority 1 */
#define k_OFFS_DFDS_QAL_CC       (5) /* clk class of clk quality */
#define k_OFFS_DFDS_QAL_CA       (6) /* clk accuracy of clk 
                                        quality */
#define k_OFFS_DFDS_QAL_OSLV     (7) /* offset scaled log variance
                                        of clock quality */
#define k_OFFS_DFDS_PRIO2        (9) /* priority 2 */
#define k_OFFS_DFDS_CLKID       (10) /* clock identity */
#define k_OFFS_DFDS_DOMAIN      (18) /* domain number */
#define k_OFFS_DFDS_RES2        (19) /* reserved byte */
/* CURRENT_DATA_SET */
#define k_OFFS_CUDS_STEPS        (0) /* steps removed */
#define k_OFFS_CUDS_OFF2M        (2) /* current offet to master */
#define k_OFFS_CUDS_MPDELAY     (10) /* current mean path delay */
/* PARENT_DATA_SET */
#define k_OFFS_PADS_PID_CID      (0) /* clock id of port id */
#define k_OFFS_PADS_PID_PNUM     (8) /* port number of port id */
#define k_OFFS_PADS_FLAGS       (10) /* PS flag */
#define k_OFFS_PADS_RESERVED    (11) /* reserved byte */
#define k_OFFS_PADS_OPOSLV      (12) /* observed parent offset 
                                        scaled log variance */
#define k_OFFS_PADS_OPCPCR      (14) /* observed parent clock
                                        phase change rate */
#define k_OFFS_PADS_GMPRIO1     (18) /* grandmaster priority 1 */
#define k_OFFS_PADS_GM_QAL_CC   (19) /* clock class of gm's 
                                        clock quality */
#define k_OFFS_PADS_GM_QAL_CA   (20) /* clock accuracy of gm's 
                                        clock quality */
#define k_OFFS_PADS_GM_QAL_OSLV (21) /* offset scaled log variance 
                                        of gm's clock quality */
#define k_OFFS_PADS_GMPRIO2     (23) /* gm priority 2 */
#define k_OFFS_PADS_GMID        (24) /* gm clock identity */
/* TIME_PROPERTIES_DATA_SET */
#define k_OFFS_TIDS_UTCOFFS      (0) /* current UTC offset */
#define k_OFFS_TIDS_FLAGS        (2) /* FTRA, TTRA, PTP, UTCV,
                                        LI-59 LI-61 flags */
#define k_OFFS_TIDS_TISRC        (3) /* time source enumeration */
/* PORT_DATA_SET */
#define k_OFFS_PODS_PID_CID      (0) /* clock id of port id */
#define k_OFFS_PODS_PID_PNUM     (8) /* port number of port 
                                        identity */
#define k_OFFS_PODS_POSTATE     (10) /* current port state */
#define k_OFFS_PODS_LMDRI       (11) /* log min delay req interval */
#define k_OFFS_PODS_PMPD        (12) /* peer mean path delay */
#define k_OFFS_PODS_ANNCINV     (20) /* log announce interval */
#define k_OFFS_PODS_ANNCTO      (21) /* annc receipt timeout */
#define k_OFFS_PODS_SYNCINV     (22) /* log sync interval */
#define k_OFFS_PODS_DELMECH     (23) /* delay mechanism */
#define k_OFFS_PODS_LMPRI       (24) /* log min pdelay reqest 
                                        interval */
#define k_OFFS_PODS_VERSION     (25) /* version number */
/* PRIORITY1 */
#define k_OFFS_PRIO1_PRIO        (0) /* priority 1 */
#define k_OFFS_PRIO1_RES         (1) /* reserved byte */
/* PRIORITY2 */
#define k_OFFS_PRIO2_PRIO        (0) /* priority 2 */
#define k_OFFS_PRIO2_RES         (1) /* reserved byte */
/* DOMAIN */
#define k_OFFS_DOM_NUM           (0) /* domain number */
#define k_OFFS_DOM_RES           (1) /* reserved byte */
/* SLAVE_ONLY */
#define k_OFFS_SLVO_FLAGS        (0) /* slave only flag */
#define k_OFFS_SLVO_RES          (1) /* reserved byte */
/* LOG_ANNOUNCE_INTERVAL */
#define k_OFFS_LAI_INTV          (0) /* log announce interval */
#define k_OFFS_LAI_RES           (1) /* reserved byte */
/* ANNOUNCE_RECEIPT_TIMEOUT */
#define k_OFFS_ARTO_TO           (0) /* annc receipt timeout */
#define k_OFFS_ARTO_RES          (1) /* reserved byte */
/* LOG_SYNC_INTERVAL */
#define k_OFFS_LSI_INTV          (0) /* los sync interval */
#define k_OFFS_LSI_RES           (1) /* reserved byte */
/* VERSION_NUMBER */
#define k_OFFS_VN_NUM            (0) /* version number (only four 
                                        least significant bits */
#define k_OFFS_VN_RES            (1) /* reserved byte */
/* TIME */
#define k_OFFS_TIME_SEC          (0) /* current time (seconds) */
#define k_OFFS_TIME_NSEC         (6) /* current time (nano sec) */
/* CLOCK_ACCURACY */
#define k_OFFS_CLKA_ACC          (0) /* clock accuracy */
#define k_OFFS_CLKA_RES          (1) /* reserved byte */
/* UTC_PROPERTIES */
#define k_OFFS_UTC_OFFS          (0) /* current UTC offset */
#define k_OFFS_UTC_FLAGS         (2) /* UTCV, LI-59 & LI-61 flags*/
#define k_OFFS_UTC_RES           (3) /* reserved byte */
/* TRACEABILITY_PROPERTIES */
#define k_OFFS_TRACE_FLAGS       (0) /* FTRA & TTRA traceability 
                                        flags */
#define k_OFFS_TRACE_RES         (1) /* reserved byte */
/* TIMESCALE_PROPERTIES */
#define k_OFFS_SCALE_FLAGS       (0) /* PTP timescale flag */
#define k_OFFS_SCALE_TISRC       (1) /* time source enumeration */
/* UNICAST_NEGOTIATION_ENABLE */
#define k_OFFS_UCN_FLAGS         (0) /* unicast enable flag */
#define k_OFFS_UCN_RES           (1) /* reserved byte */
/* PATH_TRACE_LIST */
#define k_OFFS_PATH_LIST         (0) /* start of path trace list */
/* PATH_TRACE_ENABLE */
#define k_OFFS_PATHENA_FLAG      (0) /* path trace enable flag */
#define k_OFFS_PATHENA_RES       (1) /* reserved byte */
/* GRANDMASTER_CLUSTER_TABLE */
#define k_OFFS_GMTBL_LQUINV      (0) /* log query interval */
#define k_OFFS_GMTBL_SIZE        (1) /* count of table entries */
#define k_OFFS_GMTBL_MEMBERS     (2) /* begin GM cluster table */
/* UNICAST_MASTER_TABLE */
#define k_OFFS_UCMATBL_LQI       (0) /* log query interval */
#define k_OFFS_UCMATBL_SIZE      (1) /* table size */
#define k_OFFS_UCMATBL_TBL       (3) /* begin of UC MST table */
/* UNICAST_MASTER_MAX_TABLE_SIZE */
#define k_OFFS_UCMA_SIZE         (0) /* max UC MST table size */
/* ACCEPTABLE_MASTER_TABLE */
#define k_OFFS_ACMATBL_SIZE      (0) /* size of acceptable 
                                        master table */
#define k_OFFS_ACMATBL_TBL       (2) /* begin of acceptable 
                                        master table */
/* ACCEPTABLE_MASTER_TABLE_ENABLED */
#define k_OFFS_ACMAENA_FLAGS     (0) /* enable flag */
#define k_OFFS_ACMAENA_RES       (1) /* reserved byte */
/* ACCEPTABLE_MASTER_MAX_TABLE_SIZE */
#define k_OFFS_ACMA_SIZE         (0) /* max acceptable  
                                        master table size */
/* ALTERNATE_MASTER */
#define k_OFFS_ALTMA_FLAGS       (0) /* transmit sync flag */
#define k_OFFS_ALTMA_LAMSI       (1) /* log alternate multicast 
                                        sync interval */
#define k_OFFS_ALTMA_NUM         (2) /* number of alternate 
                                        masters */
#define k_OFFS_ALTMA_RES         (3) /* reserved byte */
/* ALTERNATE_TIME_OFFSET_ENABLE */
#define k_OFFS_ALTENA_KEY        (0) /* key field */
#define k_OFFS_ALTENA_FLAGS      (1) /* enable flag */
/* ALTERNATE_TIME_OFFSET_NAME */
#define k_OFFS_ALTNME_KEY        (0) /* key field */
#define k_OFFS_ALTNME_NAME_SIZE  (1) /* displey text size */
#define k_OFFS_ALTNME_NAME_TXT   (2) /* display text */
/* ALTERNATE_TIME_OFFSET_MAX_KEY */
#define k_OFFS_ALTNME_MAXKEY     (0) /* maximal keys */
#define k_OFFS_ALTNME_RES        (1) /* reserved byte */
/* ALTERNATE_TIME_OFFSET_PRPOERTIES */
#define k_OFFS_ALTPROP_KEY       (0) /* key field */
#define k_OFFS_ALTPROP_OFFS      (1) /* current offset */
#define k_OFFS_ALTPROP_JMP       (5) /* next jump-seconds */
#define k_OFFS_ALTPROP_TNEXT     (9) /* time of next jump */
#define k_OFFS_ALTPROP_RES      (15) /* reserved byte */
/* TRANSPARENT_CLOCK_DEFAULT_DATA_SET */
#define k_OFFS_TCDFDS_CID        (0) /* clock identity */
#define k_OFFS_TCDFDS_PNUM       (8) /* number of ports */
#define k_OFFS_TCDFDS_DELM      (10) /* delay mechanism */
#define k_OFFS_TCDFDS_PRIM      (11) /* primary domain */
/* TRANSPARENT_CLOCK_PORT_DATA_SET */
#define k_OFFS_TCPODS_PID_CID    (0) /* clock id of port id */
#define k_OFFS_TCPODS_PID_PNUM   (8) /* port number of port id */
#define k_OFFS_TCPODS_FLAGS     (10) /* faulty flag */
#define k_OFFS_TCPODS_LMPDRI    (11) /* log min pdelay request 
                                        interval */
#define k_OFFS_TCPODS_PMPD      (12) /* peer mean path delay */
/* PRIMARY_DOMAIN */
#define k_OFFS_PRIM_DOM          (0) /* primary domain */
#define k_OFFS_PRIM_RES          (1) /* reserved byte */
/* DELAY_MECHANISM */
#define k_OFFS_DELM_MECH         (0) /* deleay mechanism */
#define k_OFFS_DELM_RES          (1) /* reserved byte */
/* LOG_MIN_PDELAY_REQ_INTERVAL */
#define k_OFFS_LMPDRI_INTV       (0) /* log min pdelay request 
                                        interval */
#define k_OFFS_LMPDRI_RES        (1) /* reserved byte */

/************************************************************************/
/** MNTapi TLV payload data flags
             The following defines are used to access single bit-flags
             within a byte of the TLV pay-load, returned upon a 
             management request. All defines for each message adhere
             to the TLV data field specification of the IEEE 1588
             standard and can be used as index variables to access the
             TLV data array. For more information, refer to the IEEE
             1588 standard. 
*/
/* flag position description */
/* CLOCK_DESCRIPTION */
#define k_OFFS_CLKDES_FLAGS_OC    (7) /* implements an OC */
#define k_OFFS_CLKDES_FLAGS_BC    (6) /* implements a BC */
#define k_OFFS_CLKDES_FLAGS_P2PTC (5) /* peer-to-peer TC */
#define k_OFFS_CLKDES_FLAGS_E2ETC (4) /* end-to-end TC */
#define k_OFFS_CLKDES_FLAGS_MN    (3) /* implement mnt node */
/* DEFAULT_DATA_SET */
#define k_OFFS_DFDS_FLAGS_TSC     (0) /* two-step-flag */
#define k_OFFS_DFDS_FLAGS_SO      (1) /* slave only flag */
/* PARENT_DATA_SET */
#define k_OFFS_PADS_FLAGS_PS      (0) /* parent state flag */
/* TIME_PROPERTIES_DATA_SET */
#define k_OFFS_TIDS_FLAGS_LI61    (0) /* leap 61 flag */
#define k_OFFS_TIDS_FLAGS_LI59    (1) /* leap 59 flag */
#define k_OFFS_TIDS_FLAGS_UTCV    (2) /* offset based on UTC? */
#define k_OFFS_TIDS_FLAGS_PTP     (3) /* time = PTP time? */
#define k_OFFS_TIDS_FLAGS_TTRA    (4) /* time tracable? */
#define k_OFFS_TIDS_FLAGS_FTRA    (5) /* frequency traceable */
/* SLAVE_ONLY */
#define k_OFFS_SLVO_FLAGS_SO      (0) /* slave only flag */
/* UTC_PROPERRTIES */
#define k_OFFS_UTC_FLAGS_LI61     k_OFFS_TIDS_FLAGS_LI61
#define k_OFFS_UTC_FLAGS_LI59     k_OFFS_TIDS_FLAGS_LI59
#define k_OFFS_UTC_FLAGS_UTCV     k_OFFS_TIDS_FLAGS_UTCV
/* TRACEABILITY_PROPERTIES */
#define k_OFFS_TRACE_FLAGS_TTRA   k_OFFS_TIDS_FLAGS_TTRA
#define k_OFFS_TRACE_FLAGS_FTRA   k_OFFS_TIDS_FLAGS_FTRA
/* TIMESCALE_PROPERTIES */
#define k_OFFS_SCALE_FLAGS_PTP    k_OFFS_TIDS_FLAGS_PTP
/* UNICAST_NEGOTIATION_ENABLE */
#define k_OFFS_UCN_FLAGS_EN       (0) /* enable/disable UC 
                                         negotiation */
/* PATH_TRACE_ENABLE */
#define k_OFFS_PATHENA_FLAG_EN    (0) /* enable/disable path trace 
                                         functionality */
/* ACCEPTABLE_MASTER_TABLE_ENABLED */
#define k_OFFS_ACMAENA_FLAGS_EN   (0) /* enables/disables acceptable
                                         master functionality */
/* ALTERNATE_MASTER */
#define k_OFFS_ALTMA_FLAGS_S      (0) /* transmit alternate 
                                         multicast sync message */
/* ALTERNATE_TIME_OFFSET_ENABLE */
#define k_OFFS_ALTENA_FLAGS_EN    (0) /* enable/disable alternate 
                                         time offset functionality */
/* TRANSPARENT_CLOCK_PORT_DATA_SET */
#define k_OFFS_TCPODS_FLAGS_FLT   (0) /* port in faulty state? */

/************************************************************************/
/** MNTapi error TLV payload data
             The following defines are used to access the error TLV 
             payload, returned upon a management request. All defines 
             adhere to the TLV data field specification of the
             IEEE 1588 standard and can be used as index variables to
             access the error TLV data array. For more information, 
             refer to the IEEE1588-2008 standard. 
*/
#define k_OFFS_ERRTLV_MNTID       (0) /* management id, which causes
                                         the error */
#define k_OFFS_ERRTLV_RESERVED    (2) /* reserved 4 bytes */
#define k_OFFS_ERRTLV_TXTLEN      (6) /* text length of error text */
#define k_OFFS_ERRTLV_DISPTXT     (7) /* text of the error */



#ifdef MNT_API

/* timeout definitions */
#define MNT_k_TIMER_SCALER     10    /* Defines a scaled count to 1 second.
                                        How often a timer event is generated,
                                        to reach a timeout of one second. */

/* API error definitions  */
#define MNTAPI_k_ERR_UNKN_ACOM   (0u) /* unknown action command */
#define MNTAPI_k_ERR_UNKN_MNTID  (1u) /* unkown management id */
#define MNTAPT_k_ERR_ZERO_PORT   (2u) /* port zero is not addressable */
#define MNTAPI_k_ERR_WRONG_VALUE (3u) /* one or more values are out of bound 
                                         or used in-acceptable values */
#define MNTAPI_k_ERR_STILL_PEND  (4u) /* another request is still pending */
#define MNTAPI_k_ERR_ALLOC       (5u) /* could not allocate buffer 
                                         to handel request */
#define MNTAPI_k_ERR_INT_TRANS   (6u) /* failure on transmit to the own node */
#define MNTAPI_k_ERR_EXT_TRANS   (7u) /* failure on transmit to remote node */
#define MNTAPI_k_ERR_NO_CLBK     (8u) /* no callback function registered */
#define MNTAPI_k_ERR_NO_TBL_DATA (9u) /* could not found table entry 
                                         fatal error */
#endif /* ifdef MNT_API */
/* API errors, which must allways be defined */
#define MNTAPI_k_ERR_INTREQ     (10u) /* internel Request, although MNT_API
                                         not defined */

/************************************************************************/
/** MNT API communication technology description
             These defines are used to describe the communication
             technology.
*/
#define k_COMM_TECH_UDP_IPv4   "Ethernet using UDP/IPv4"
#define k_COMM_TECH_UDP_IPv6   "Ethernet using UDP/IPv6"
#define k_COMM_TECH_IEEE_802_3 "Ethernet using IEEE 802.3"
#define k_COMM_TECH_DEVICENET  "DeviceNet"
#define k_COMM_TECH_CONTROLNET "ControlNet"
#define k_COMM_TECH_PROFINET   "PROFINET"
#define k_COMM_TECH_UNKNOWN    "Unknown network technology used"

/*******************************************************************************
**    data types
*******************************************************************************/

/************************************************************************/
/** MNT_t_apiStateEnum :
             The following defines are used to indicate the current
             state of an API request. The process is completed as
             soon as the state switches to the TIMEOUT or DONE.
             A timeout is used if no answer is received or if the
             request was addressed to all ones. Furthermore errors
             are defined.
*/
typedef enum
{
  /* reserved         (0x0) */
  e_MNTAPI_PENDING  = (0x1),    /* request is still pending */
  e_MNTAPI_TIMEOUT  = (0x2),    /* request is done due timeout */
  e_MNTAPI_DONE     = (0x3),    /* request is done, received all
                                   expected answers */
  e_MNTAPI_ABORT    = (0x4),    /* stack re-init, abort request */
  /* reserved         (0x5) */
  /* reserved         (0x6) */
  /* reserved         (0x7) */
  /* reserved         (0x8) */
  /* reserved         (0x9) */
  /* reserved         (0xA) */
  /* reserved         (0xB) */
  /* reserved         (0xC) */
  /* reserved         (0xD) */
  e_MNTAPI_ERR_OR   = (0xE),    /* other request is pending */
  e_MNTAPI_ERR_AL   = (0xF)     /* general error occured */
}MNT_t_apiStateEnum;


/************************************************************************/
/** MNT_t_apiReturnTLV
             This structure will be returned upon a call of the 
             callback function. Either the received management 
             message or a new state due to an error, abort or 
             timeout will be passed.
*/
typedef struct
{
  /* source port of received message */
  PTP_t_PortId s_srcPortId;
  /* the received TLV */
  PTP_t_MntTLV s_mntTlv;
  /* size of TLV payload date (of the UINT8 array) */
  UINT16       w_sizeOfTlvPld;
}MNT_t_apiReturnTLV;

/************************************************************************/
/** MNT_t_apiClbkData
             The application must provide a callback function to get
             management responses as well as the status information 
             like an error, abort or timeout. Furthermore a handle 
             could be defined and passed upon request, which is passed
             to the callback function. The definition of the handle is 
             application specific.
*/
typedef struct
{
  /* pointer to a callback function */
  void (k_CB_CALL_CONV *pf_clbkFunc)
                (UINT32*,MNT_t_apiReturnTLV*,MNT_t_apiStateEnum);
  /* handle for callback function, 
     optionally for use of the application */
  UINT32* pdw_cbHandle;
}MNT_t_apiClbkData;

/************************************************************************/
/** MNT_t_apiMsgConfig
             This structure defines a configuration structure to pass
             all necessary information for the management message 
             generation. Additionally a timeout is defined, to limit a 
             request to a defined duration. 
*/
typedef struct
{
  /* unicast network address */
  PTP_t_PortAddr *ps_pAddr;
  /* destination port id */
  PTP_t_PortId   s_desPortId;
  /* starting boundary hops */
  UINT8          b_startBoundHops;
  /* action field */
  UINT8          b_actionFld;
  /* timeout of request in seconds */
  UINT8          b_timeout;
}MNT_t_apiMsgConfig;

/*******************************************************************************
**    global variables
*******************************************************************************/


/*******************************************************************************
**    function prototypes
*******************************************************************************/

#ifdef MNT_API
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
                               const MNT_t_apiClbkData  *ps_clbkData );

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
                        const MNT_t_apiClbkData  *ps_clbkData );

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
                        const PTP_t_Text         *ps_userDesc );

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
                             const MNT_t_apiClbkData  *ps_clbkData );

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
                            const MNT_t_apiClbkData  *ps_clbkData );

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
                                 UINT16             w_initKey );

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
                         const MNT_t_apiClbkData  *ps_clbkData );

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
                              const MNT_t_apiClbkData  *ps_clbkData );

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
                          const MNT_t_apiClbkData  *ps_clbkData );

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
                          const MNT_t_apiClbkData  *ps_clbkData );

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
                         const MNT_t_apiClbkData  *ps_clbkData );

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
                           const MNT_t_apiClbkData  *ps_clbkData );

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
                       const MNT_t_apiClbkData  *ps_clbkData );

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
                                UINT8              b_priority1 );

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
                                UINT8              b_priority2 );

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
                             UINT8              b_domain );

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
                                BOOLEAN            o_slaveOnly );

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
                               INT8               c_anncIntv );

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
                                    UINT8              b_anncTOut );

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
                               INT8               c_syncIntv );

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
                                    UINT8              b_version );

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
                        const MNT_t_apiClbkData  *ps_clbkData );

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
                        const MNT_t_apiClbkData  *ps_clbkData );

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
                     const PTP_t_TmStmp       *ps_tmStmp );

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
                                  PTP_t_clkAccEnum   e_clkAccuracy );

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
                              BOOLEAN            o_utcValid );

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
                                BOOLEAN            o_freqTrace );

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
                                  BOOLEAN            o_ptpTmScale );

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
                                   BOOLEAN            o_unNegoEna );

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
                              const MNT_t_apiClbkData  *ps_clbkData );

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
                                   BOOLEAN            o_pathTraceEna );

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
                             const PTP_t_PortAddr     *ps_gmClustTbl );

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
                            const PTP_t_PortAddr     *ps_ucMasTbl );

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
                                const MNT_t_apiClbkData  *ps_clbkData );

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
                             const PTP_t_AcceptMaster *ps_accMstTbl );

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
                                      BOOLEAN            o_enabled);

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
                                    const MNT_t_apiClbkData  *ps_clbkData );

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
                                UINT8              b_numOfAltMas );

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
                                     BOOLEAN            o_altTOEna );

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
                                const PTP_t_Text         *ps_dispTxt );

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
                               const MNT_t_apiClbkData  *ps_clbkData );

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
                                      UINT64             ddw_timeNxJump);

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
                            const MNT_t_apiClbkData  *ps_clbkData );

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
                         const MNT_t_apiClbkData  *ps_clbkData );

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
                                    UINT8              b_primaryDom );

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
                                     UINT8              b_delMech );

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
                                     UINT8              c_pDelReqIntv );

/** C2DOC_stop */

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
                                 UINT16             w_tblIdx );

#endif /* ifdef MNT_API */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __MNTAPI_H__ */

