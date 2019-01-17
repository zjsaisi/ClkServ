/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: PTPdef.h  
**    Summary: Includes all Protocol-specific definitions and types 
**             that are defined in the IEEE 1588 V2 Standard.
**             Defines global used functions, that associate with no 
**             special unit.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions:
**
**   Compiler: Ansi-C
**    Remarks: IEEE1588 V2
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __PTPDEF_H__
#define __PTPDEF_H__

#ifdef __cplusplus
extern "C"
{
#endif

/** C2DOC_stop */

/*************************************************************************
**    constants and macros
*************************************************************************/

/* defines for ordering  */
#define PTP_k_LESS    ((INT8)-1)
#define PTP_k_SAME    ((INT8) 0)
#define PTP_k_GREATER ((INT8) 1)

/* defines for ranges */
#define k_RNG_MIN (0u)
#define k_RNG_DEF (1u)
#define k_RNG_MAX (2u)
#define k_RNG_SZE (3u)

/* mathematical defines */
#define PTP_NSEC_TO_INTV(dw_nsec) ((INT64)((INT64)(dw_nsec) * (INT64)65536LL))
#define PTP_INTV_TO_NSEC(ll_intv) ((INT64)(ll_intv) / 65536LL)
#define PTP_PSEC_TO_INTV(ddw_psec) ((INT64)((INT64)((dw_psec) * (INT64)65536)/(INT64)1000LL))
#define PTP_INTV_TO_PSEC(ll_intv) ((INT64)((ll_intv)*(INT64)1000LL)/(INT64)65536LL)
#define PTP_ABS(TYPECAST,val)   (TYPECAST)(((val) < 0) ? (-val) : (val)) 
#define PTP_CHK_RNG(TYPE,val,min,max)  (((TYPE)val >= (TYPE)min)&&((TYPE)val <= (TYPE)max))

/* lenght of error string */
#define k_MAX_ERR_STR_LEN  (20u)

/*
**  Boolean constants:
*/
#ifndef TRUE
  #define TRUE    (1==1)
#endif

#ifndef FALSE
  #define FALSE   (1==0)
#endif

/* some numeric range definitions for standard data types */
#define k_MIN_I8        (0x80)
#define k_MAX_I8        (0x7F)
#define k_MIN_U8        (0x00)
#define k_MAX_U8        (0xFF)
#define k_MIN_I16       (0x8000)
#define k_MAX_I16       (0x7FFF)
#define k_MIN_U16       (0x0000)
#define k_MAX_U16       (0xFFFF)
#define k_MIN_I32       (0x80000000)
#define k_MAX_I32       (0x7FFFFFFF)
#define k_MIN_U32       (0x00000000)
#define k_MAX_U32       (0xFFFFFFFF)
#define k_MIN_I64       (INT64)(0x8000000000000000LL)
#define k_MAX_I64       (INT64)(0x7FFFFFFFFFFFFFFFLL)
#define k_MIN_U64       (UINT64)(0x0000000000000000ULL)
#define k_MAX_U64       (UINT64)(0xFFFFFFFFFFFFFFFFULL)

/* Little / Big endian */
#define k_LITTLE (1)
#define k_BIG    (0)

/* file types (binary/ascii) */
#define k_FILE_ASCII (0u)  /* ascii-file */
#define k_FILE_BNRY  (1u)  /* binary file */

/* version 2 */
#define k_PTP_VERSION    (0x02)
/* length of clock ID */
#define k_CLKID_LEN      (0x08)

/* foreign master */
#define k_FRGN_MST_THRESH   (2) /* the minimal amount of announce messages in an
                                   announce interval toqualify the announce 
                                   messages for BMC */
#define k_FRGN_MST_TIME_WND (4) /* foreing master time window to qualify announce
                                   messages for BMC */
/* internal define for event/general socket */
#define k_SOCK_EVT       ((UINT8)1) /* Messages Sync and DelayRequest */
#define k_SOCK_GNRL      ((UINT8)2) /* Messages FollowUp, DelayResponse, announce, 
                                       signaling and Management */
#define k_SOCK_EVT_PDEL  ((UINT8)3) /* PeerDelay request, PeerDelay response */ 
#define k_SOCK_GNRL_PDEL ((UINT8)4) /* PeerDelay response follow up */

/* synchronization modes */
#define k_SYNC_TM_OF_DAY (0u) /* time of day synchronization, called 'synchronization' 
                                 in standard document */
#define k_SYNC_FREQ      (1u) /* frequency synchronization, called 'syntonization' 
                                 in standard document */

/* Position of bits in PTP message 'flag-field' from 0 (LSB) to 15 (MSB) */
#define k_FLG_ALT_MASTER        (8u) /* TRUE if the originator is not in the MASTER state */
#define k_FLG_TWO_STEP          (9u) /* TRUE for two-step clocks */
#define k_FLG_UNICAST          (10u) /* TRUE for unicast messages */
#define k_FLG_PTP_PROF_SPEC1   (13u) /* as defined by an alternate PTP profile, otherwise FALSE */
#define k_FLG_PTP_PROF_SPEC2   (14u) /* as defined by an alternate PTP profile, otherwise FALSE */
#define k_FLG_SECURITY         (15u) /* security flag for security extension */
#define k_FLG_LI_61             (0u) /* Value of leap_59 of time properties data set */
#define k_FLG_LI_59             (1u) /* Value of leap_61 of time properties data set */
#define k_FLG_CUR_UTCOFFS_VAL   (2u) /* Value of current_utc_offset_valid of time properties data set */
#define k_FLG_PTP_TIMESCALE     (3u) /* Value of ptp_timescale of time properties data set */
#define k_FLG_TIME_TRACEABLE    (4u) /* Value of time_traceable of time properties data set */
#define k_FLG_FREQ_TRACEABLE    (5u) /* Value of frequency_traceable of time properties data set */


/* Makros to set and reset Flags */
#define SET_FLAG(dst,pos,val)   (dst = ((UINT16)(dst & ~(1u << (UINT8)pos))) | ((UINT8)val << (UINT8)pos))
#define GET_FLAG(src,pos)       (BOOLEAN)((((UINT16)src) & (1u<<pos)) == (1u<<pos)) 

/* management ID values */
/* Applicable to all node types 0x0000 - 0x1FFF */
#define k_NULL_MANAGEMENT                     (0x0000) /* GET, SET */
#define k_CLOCK_DESCRIPTION                   (0x0001) /* GET      */
#define k_USER_DESCRIPTION                    (0x0002) /* GET, SET */ 
#define k_SAVE_IN_NON_VOLATILE_STORAGE        (0x0003) /* COMMAND  */ 
#define k_RESET_NON_VOLATILE_STORAGE          (0x0004) /* COMMAND  */ 
#define k_INITIALIZE                          (0x0005) /* COMMAND  */ 
#define k_FAULT_LOG                           (0x0006) /* GET      */ 
#define k_FAULT_LOG_RESET                     (0x0007) /* COMMAND  */ 
/* _Reserved 0x0008 - 0x1FFF */
/* Applicable to ordinary and boundary clocks 0x2000 - 0x3FFF */
#define k_DEFAULT_DATA_SET                    (0x2000) /* GET      */ 
#define k_CURRENT_DATA_SET                    (0x2001) /* GET      */ 
#define k_PARENT_DATA_SET                     (0x2002) /* GET      */ 
#define k_TIME_PROPERTIES_DATA_SET            (0x2003) /* GET      */ 
#define k_PORT_DATA_SET                       (0x2004) /* GET      */
#define k_PRIORITY1                           (0x2005) /* GET, SET */ 
#define k_PRIORITY2                           (0x2006) /* GET, SET */ 
#define k_DOMAIN                              (0x2007) /* GET, SET */ 
#define k_SLAVE_ONLY                          (0x2008) /* GET, SET */ 
#define k_LOG_MEAN_ANNOUNCE_INTERVAL          (0x2009) /* GET, SET */
#define k_ANNOUNCE_RECEIPT_TIMEOUT            (0x200A) /* GET, SET */
#define k_LOG_MEAN_SYNC_INTERVAL              (0x200B) /* GET, SET */
#define k_VERSION_NUMBER                      (0x200C) /* GET, SET */
#define k_ENABLE_PORT                         (0x200D) /* COMMAND  */
#define k_DISABLE_PORT                        (0x200E) /* COMMAND  */
#define k_TIME                                (0x200F) /* GET, SET */ 
#define k_CLOCK_ACCURACY                      (0x2010) /* GET, SET */ 
#define k_UTC_PROPERTIES                      (0x2011) /* GET, SET */ 
#define k_TRACEABILITY_PROPERTIES             (0x2012) /* GET, SET */ 
#define k_TIMESCALE_PROPERTIES                (0x2013) /* GET, SET */ 
#define k_UNICAST_NEGOTIATION_ENABLE          (0x2014) /* GET, SET */
#define k_PATH_TRACE_LIST                     (0x2015) /* GET      */ 
#define k_PATH_TRACE_ENABLE                   (0x2016) /* GET, SET */ 
#define k_GRANDMASTER_CLUSTER_TABLE           (0x2017) /* GET, SET */ 
#define k_UNICAST_MASTER_TABLE                (0x2018) /* GET, SET */
#define k_UNICAST_MASTER_MAX_TABLE_SIZE       (0x2019) /* GET      */
#define k_ACCEPTABLE_MASTER_TABLE             (0x201A) /* GET, SET */ 
#define k_ACCEPTABLE_MASTER_TABLE_ENABLED     (0x201B) /* GET, SET */
#define k_ACCEPTABLE_MASTER_MAX_TABLE_SIZE    (0x201C) /* GET      */ 
#define k_ALTERNATE_MASTER                    (0x201D) /* GET, SET */
#define k_ALTERNATE_TIME_OFFSET_ENABLE        (0x201E) /* GET, SET */ 
#define k_ALTERNATE_TIME_OFFSET_NAME          (0x201F) /* GET, SET */ 
#define k_ALTERNATE_TIME_OFFSET_MAX_KEY       (0x2020) /* GET      */ 
#define k_ALTERNATE_TIME_OFFSET_PROPERTIES    (0x2021) /* GET, SET */ 
/* Reserved 0x2022 - 0x3FFF */
/* Applicable to transparent clocks 0x4000 to 0x5FFF */
#define k_TRANSPARENT_CLOCK_DEFAULT_DATA_SET  (0x4000) /* GET      */ 
#define k_TRANSPARENT_CLOCK_PORT_DATA_SET     (0x4001) /* GET      */
#define k_PRIMARY_DOMAIN                      (0x4002) /* GET, SET */ 
/* Reserved 0x4004 - 0x5FFF */
/* Applicable to ordinary, boundary 
   and transparent clocks 0x6000 - 0x7FFF */
#define k_DELAY_MECHANISM                     (0x6000) /* GET, SET */
#define k_LOG_MIN_MEAN_PDELAY_REQ_INTERVAL    (0x6001) /* GET, SET */
/* Reserved 0x6002 - 0xBFFF */
/* This range is to be used for implementation 
   specific identifiers. */
/* 0xC000 - 0xDFFF */
#define k_PRIVATE_STATUS                      (0xc401) /* GET */
/* This range is to be assigned by an alternate 
   PTP profile 0xE000 - 0xFFFE */
/* Reserved 0xFFFF */

/* Error id definitions for all units */
#define k_TSU_ERR_ID ((UINT32)( 0u))
#define k_DIS_ERR_ID ((UINT32)( 1u))
#define k_CIF_ERR_ID ((UINT32)( 2u))
#define k_SLV_ERR_ID ((UINT32)( 3u))
#define k_MST_ERR_ID ((UINT32)( 4u))
#define k_MNT_ERR_ID ((UINT32)( 5u))
#define k_CTL_ERR_ID ((UINT32)( 6u))
#define k_PTP_ERR_ID ((UINT32)( 7u))
#define k_GOE_ERR_ID ((UINT32)( 8u))
#define k_TCF_ERR_ID ((UINT32)( 9u))
#define k_P2P_ERR_ID ((UINT32)(10u))
#define k_UCM_ERR_ID ((UINT32)(11u))
#define k_UCD_ERR_ID ((UINT32)(12u))
#define k_FIO_ERR_ID ((UINT32)(13u))

/* time definitions for nanosecond/second resolution */
#define k_MSEC_IN_SEC                    (1000u)
#define k_USEC_IN_SEC                 (1000000u)
#define k_NSEC_IN_SEC              (1000000000u)
#define k_PSEC_IN_SEC ((UINT64)1000000000000ULL)
#define k_MAX_NSEC                  (999999999u)

/* maximum and minimum for intervals for this implementation */
// jyang: change interval range to +/-7
//#define k_MIN_LOG_MSG_INTV  (-11) /* minimum for a message interval = 488 usec */
//#define k_MAX_LOG_MSG_INTV   (11) /* maximum for a message interval = 2048 sec */
#define k_MIN_LOG_MSG_INTV  (-7) /* minimum for a message interval = 7812 usec */
#define k_MAX_LOG_MSG_INTV   (7) /* maximum for a message interval = 128 sec */


/************************************************************************/
/** PTP_t_intMsgEnum : 
            Internal message type. This message type defines the type
            of the internal used messages that are managed by the
            SIS-Mboxes. See also PTP_t_MboxMsg
*/
typedef enum{
  e_IMSG_NONE,         /* no message !! */
  e_IMSG_ANNC,         /* an announce message */
  e_IMSG_SYNC,         /* a sync msg */
  e_IMSG_DLRQ,         /* a delay-request msg */
  e_IMSG_FLWUP,        /* a follow-up msg */
  e_IMSG_DLRSP,        /* a delay-response msg */
  e_IMSG_MM,           /* a management msg */

  e_IMSG_NETINIT,       /* a reinitialized net interface */
  e_IMSG_NETDEINIT,     /* a net interface to be deinitialized */

  e_IMSG_MTSD,          /* Timestamp difference 'master to slave delay' */
  e_IMSG_MTSDBIG,       /* big master to slave delay */
  e_IMSG_STMD,          /* Timestamp difference 'slave to master delay' */
  e_IMSG_TRGDLRQ,       /* trigger delay request message sending */
                            
  e_IMSG_MSYNC,         /* missing sync message */
                            
  e_IMSG_SC_MST,        /* message to unit mst: state change to master */
  e_IMSG_INIT,         /* reinitialize node (mngmsg) */
  e_IMSG_NEW_MST,       /* the master changed */
  e_IMSG_ADJP_END,      /* slave task did not receive new sync message 
                                 in sync interval -> reset drift */
                            
  e_IMSG_MDLRSP,        /* missing delay response error */
  e_IMSG_MFLWUP,        /* missing follow up error */
                              
  e_IMSG_FWD_ANNC,      /* forward announce message in TCs */
  e_IMSG_FWD_SYN,       /* forward sync message in TCs */
  e_IMSG_FWD_FLW,       /* forward follow up message in TCs */
  e_IMSG_FWD_DLRQ,      /* forward delay request message in TCs */
  e_IMSG_FWD_DLRSP,     /* forward delay response message in TCs */
  e_IMSG_FWD_P_DLRQ,    /* forward Pdelay request mesage in E2E TCs */
  e_IMSG_FWD_P_DLRSP,   /* forward Pdelay request mesage in E2E TCs */
  e_IMSG_FWD_P_DLRFLW,  /* forward Pdelay request mesage in E2E TCs */
  e_IMSG_TSD_TX_TC,     /* Tx timestamp of forwarded message in a TC*/
                            
  e_IMSG_P_DLRQ,        /* peer delay request message */
  e_IMSG_P_DLRSP,       /* peer delay response message */
  e_IMSG_P_DLRSP_FLW,   /* peer delay response follow up */
                            
  e_IMSG_ACT_PDEL,      /* actual path delay */ 
  e_IMSG_SEND_ERR,      /* send error */
  e_IMSG_SGNLG,         /* signaling message */
  e_IMSG_PORTADDR,      /* a unicast port address to be passed */

/* management messages */
  e_IMSG_MM_SET,       /* set a new value */
  e_IMSG_MM_RES,       /* response of a internal set request */
  e_IMSG_MM_ERRRES,    /* response of a internal set request with error */
  e_IMSG_MM_API,       /* management message from API to remote node */

/* unicast */
  e_IMSG_UC_MST_TBL,    /* unicast master table */
  e_IMSG_UC_SRV_REQ,    /* unicast service request */
  e_IMSG_UC_GRNT_ANC,  /* unicast service grant announce message */
  e_IMSG_UC_GRNT_SYN,   /* unicast service grant sync message */
  e_IMSG_UC_GRNT_DEL,   /* unicast service grant delay message */
  e_IMSG_UC_CNCL,       /* unicast service cancel */
  e_IMSG_UC_CNCL_IF,    /* cancel all granted unicast services on one interface */
  e_IMSG_ACTBL_MST,     /* acceptable unicast master */
  e_IMSG_REQ_UC_SRV,    /* request unicast sync and delay service */
  e_IMSG_NEW_FLT        /* fault sent to management task to store in
                                         fault log */
}PTP_t_intMsgEnum;

/* classification of management messages */
#define k_MNT_MSG_INT  (1u) /* indicates MNT msgs from the API */
#define k_MNT_MSG_EXT  (2u) /* indicates MNT msgs from external nodes */

#define k_EVT_ALL ((UINT32)0xFFFFFFFF)

/* announce msg needs 64 bytes PTP, 106 byte complete ethernet frame */
/* sync, delreq and followUp  need 44 bytes, 86 byte complete ethernet frame  */
/* delayResp, pdelayReq, pdelayResp and pdelayFollowUp needs 54 bytes, 96 bytes complete eth frame */
/* signaling needs up to full ethernet packet (1472 max) bytes */
/* management needs up to full ethernet packet (1472 max) bytes */ 
#define k_MAX_PTP_PLD       (1472) 

/* tlv action field definitions */
/*      Action        Value */
#define k_ACTF_GET    (0x00) 
#define k_ACTF_SET    (0x01)
#define k_ACTF_RESP   (0x02)
#define k_ACTF_CMD    (0x03)
#define k_ACTF_ACK    (0x04)
/* Reserved           (0x05) - (0xF) */

/* via standard defined (maximal) size of description strings */
#define k_MANUF_ID_SZE        (3u) /* manufacturer id */
#define k_MAX_PRODDESC_SZE   (64u) /* product description */
#define k_MAX_REVDATA_SZE    (32u) /* revision data */
#define k_MAX_USRDESC_SZE   (128u) /* user-description max size */
#define k_PROF_ID_SZE         (6u) /* profile id */

/************************************************************************/
/** PTP_t_msgTypeEnum : 
            PTP V2 message type enumeration for the 
            messageType field in the V2 header- 13.3.2.2
*/
typedef enum
{
  e_MT_SYNC           = (0x0),
  e_MT_DELREQ         = (0x1),
  e_MT_PDELREQ        = (0x2),
  e_MT_PDELRESP       = (0x3),
/* reserved             (0x4) */
/* reserved             (0x5) */
/* reserved             (0x6) */
/* reserved             (0x7) */
  e_MT_FLWUP          = (0x8),
  e_MT_DELRESP        = (0x9),
  e_MT_PDELRES_FLWUP  = (0xA),
  e_MT_ANNOUNCE       = (0xB),
  e_MT_SGNLNG         = (0xC),
  e_MT_MNGMNT         = (0xD)
/* reserved              0xE  */
/* reserved              0xF */
}PTP_t_msgTypeEnum;

/************************************************************************/
/** PTP_t_tlvTypeEnum
            The tlvType shall identify the TLV. (chapter 14.1.1) 
*/
typedef enum
{
  /* Reserved                    (0x0000) */
  /* Standard TLVs  0001 - 1FFF    value   defined in clause*/ 
  e_TLVT_MNT                 = (0x0001),  /* 15.5.3 */
  e_TLVT_MNT_ERR_STS         = (0x0002),  /* 15.5.4 */
  e_TLVT_ORG_EXT             = (0x0003),  /* 14.3   */
  /* Optional unicast message negotiation TLVs 16.1   */
  e_TLVT_REQ_UC_TRM          = (0x0004),
  e_TLVT_GRNT_UC_TRM         = (0x0005),
  e_TLVT_CNCL_UC_TRM         = (0x0006),
  e_TLVT_ACK_CNCL_UC_TRM     = (0x0007),
  /* Optional path trace mechanism TLV    16.2 */
  e_TLVT_PATH_TRACE          = (0x0008),
  /* Optional alternate timescale TLV     16.3 */
  e_TLVT_ALT_TME_OFFS_IND    = (0x0009),
  /* Reserved                  (0x000A) - (0x1FFF) */
  /* Experimental TLVs         (0x2000) - 3FFF  14.2 */
  /* Security TLVs                      Annex K */
  e_TLVT_AUTH                = (0x2000),
  e_TLVT_AUTH_CHLNG          = (0x2001),
  e_TLVT_SECRTY_ASS_UPD      = (0x2002),
  /* Cumulative frequency scale_factor offset Annex L */
  e_TLVT_CUMFRQ_SCLFCTR_OFFS = (0x2003)
  /* Reserved for experimental TLVs (0x2004) - (0x3FFF) */
  /* Reserved  4000 - FFFF  */
}PTP_t_tlvTypeEnum;

/** C2DOC_start */

/************************************************************************/
/** PTP_t_nwProtEnum
             The identification of the transport protocol in use for a
             communication path shall be indicated by the network  
             protocol enumeration. (IEEE standard chapter 7.4.1) 
*/
typedef enum
{
  e_UDP_IPv4   = (0x0001), /* ethernet / UDP / IPv4 */
  e_UDP_IPv6   = (0x0002), /* ethernet / UDP / IPv6 */
  e_IEEE_802_3 = (0x0003), /* ethernet layer 2 */
  e_DEVICENET  = (0x0004), /* Devicenet */
  e_CONTROLNET = (0x0005), /* Controlnet */
  e_PROFINET   = (0x0006), /* Profinet */
/*                0x7 - 0xEFFF      Reserved for assignment by the
                                    Precise Networked Clock Working
                                    Group of the IM/ST Committee */
/*               0xF000-0xFFFD      Reserved for assignment in a
                                     PTP profile */
  e_UNKNOWN    = (0xFFFE) /* unknown protocol */
/*                      0xFFFF      reserved */
}PTP_t_nwProtEnum;



/************************************************************************/
/** PTP_t_sevCdEnum
             The severity code enumeration reflects the severity of 
             an occurred error condition. (IEEE standard chapter 
             15.5.3.1.7) 
*/
typedef enum
{
  e_SEVC_EMRGC = 0x00, /* Emergency: system is unusable */
  e_SEVC_ALRT  = 0x01, /* Alert: immediate action needed */
  e_SEVC_CRIT  = 0x02, /* Critical: critical conditions */
  e_SEVC_ERR   = 0x03, /* Error: error conditions */
  e_SEVC_WARN  = 0x04, /* Warning: warning conditions */
  e_SEVC_NOTC  = 0x05, /* Notice: normal but significant condition */
  e_SEVC_INFO  = 0x06, /* Informational: informational messages */
  e_SEVC_DBG   = 0x07  /* Debug: debug-level messages */
  /* 0x08-0xFF Reserved */
}PTP_t_sevCdEnum;

/** C2DOC_stop */

/************************************************************************/
/** PTP_t_clkClassEnum
             The clockClass attribute of an ordinary or boundary clock denotes 
             the traceability of the time or frequency distributed by the 
             grandmaster clock. (IEEE standard chapter 7.6.2.4)
*/
typedef enum
{
  /* 0 Reserved to enable compatibility with future versions. */
  /* 1-5 Reserved */
  e_CLKC_PRIM     = 6, /* Shall designate a clock that is synchronized to a
              primary reference time source. The timescale distributed shall
              be PTP. A clockClass 6 clock shall not be a slave to another 
              clock in the domain. */
  e_CLKC_PRIM_HO  = 7, /*Shall designate a clock that has previously been 
              designated as clockClass 6 but that has lost the ability to 
              synchronize to a primary reference time source and is in holdover
              mode and within holdover specifications. The timescale distributed
              shall be PTP. A clockClass 7 clock shall not be a slave to another 
              clock in the domain. */
/* 8 Reserved */
/* 9-10 Reserved to enable compatibility with future versions. */
/* 11-12 Reserved */
  e_CLKC_ARB      = 13, /* Shall designate a clock that is synchronized to an
              application specific source of time. The timescale distributed
              shall be ARB. A clockClass 13 clock shall not be a slave to another
              clock in the domain. */
  e_CLKC_ARB_HO   = 14, /* Shall designate a clock that has previously been
              designated as clockClass 13 but that has lost the ability to 
              synchronize to an application specific source of time and is in
              holdover mode and within holdover specifications. The timescale 
              distributed shall be ARB. A clockClass 14 clock shall not be a 
              slave to another clock in the domain. */
/* 15-51 Reserved */
  e_CLKC_PRIM_A   = 52, /* Degradation alternative A for a clock of clockClass 7
              that is not within holdover specification. A clock of clockClass 52
              shall not be a slave to another clock in the domain. */
/* 53-57 Reserved */
  e_CLKC_ARB_A    = 58, /* Degradation alternative A for a clock of clockClass 14
              that is not within holdover specification. A clock of clockClass 58
              shall not be a slave to another clock in the domain. */
/* 59-67 Reserved */
/* 68-122 For use by alternate PTP profiles */
/* 123-127 Reserved */
/* 128-132 Reserved */
/* 133-170 For use by alternate PTP profiles */
/* 171-186 Reserved */
  e_CLKC_PRIM_B   = 187, /* Degradation alternative B for a clock of clockClass 7
              that is not within holdover specification. A clock of 
              clockClass 187 may be a slave to another clock in the domain. */
/* 188-192 Reserved */
  e_CLKC_ARB_B    = 193, /* Degradation alternative B for a clock of 
              clockClass 14 that is not within holdover specification. A clock
              of clockClass 193 may be a slave to another clock in the domain. */
/* 194-215 reserved */
/* 216-232 For use by alternate PTP profiles */
/* 233-247 Reserved */
  e_CLKC_DEF      = (248), /* Default. This clockClass shall be used if none
                            of the other clockClass definitions apply. */
/* 249-250 Reserved */
/* 251 Reserved for version 1 compatibility, see Clause 18. */
/* 252-254 Reserved */
  e_CLKC_SLV_ONLY = (255) /* Shall be the clockClass of a 
                            slave-only clock, see 9.2.2. */
}PTP_t_clkClassEnum;


/** C2DOC_start */

/************************************************************************/
/** PTP_t_clkAccEnum
             The clock accuracy enumeration indicates the expected
             accuracy of a clock when it is the grandmaster, or in the
             event it becomes the grandmaster. (IEEE standard chapter 
             7.6.2.5) 
*/
typedef enum
{
  /* 0x00-0x1F reserved */
  e_ACC_25NS   = (0x20), /* The time is accurate to within 25 ns */
  e_ACC_100NS  = (0x21), /* The time is accurate to within 100 ns */
  e_ACC_250NS  = (0x22), /* The time is accurate to within 250 ns */
  e_ACC_1US    = (0x23), /* The time is accurate to within 1 us */
  e_ACC_2500NS = (0x24), /* The time is accurate to within 2.5 us */
  e_ACC_10US   = (0x25), /* The time is accurate to within 10 us */
  e_ACC_25US   = (0x26), /* The time is accurate to within 25 us */
  e_ACC_100US  = (0x27), /* The time is accurate to within 100 us */
  e_ACC_250US  = (0x28), /* The time is accurate to within 250 us */
  e_ACC_1MS    = (0x29), /* The time is accurate to within 1 ms */
  e_ACC_2500US = (0x2A), /* The time is accurate to within 2.5 ms */
  e_ACC_10MS   = (0x2B), /* The time is accurate to within 10 ms */
  e_ACC_25MS   = (0x2C), /* The time is accurate to within 25 ms */
  e_ACC_100MS  = (0x2D), /* The time is accurate to within 100 ms */
  e_ACC_250MS  = (0x2E), /* The time is accurate to within 250 ms */
  e_ACC_1S     = (0x2F), /* The time is accurate to within 1 s */
  e_ACC_10S    = (0x30), /* The time is accurate to within 10 s */
  e_ACC_L10S   = (0x31), /* The time is accurate to >10 s */
  /* 0x32 - 0x7F reserved */
  /* 0x80 - 0xFD For use by alternate PTP profiles */
  e_ACC_UNKWN  = (0xFE), /* Unknown */
  e_ACC_RES    = (0xFF)  /* reserved */
}PTP_t_clkAccEnum;



/************************************************************************/
/** PTP_t_tmSrcEnum
             This is an information only attribute indicating the
             source of time used by the grandmaster clock. (IEEE
             standard chapter 7.6.2.6)
*/
typedef enum
{
  /* time source enumeration */
  e_ATOMIC_CLK = (0x10), /* A device, that is based on
                            atomic resonance */
  e_GPS        = (0x20), /* device synchronized to a 
                            satellite system */
  e_TER_RADIO  = (0x30), /* Any device synchronized via any 
                            of the radio  distribution systems */
  e_PTP        = (0x40), /* Any device synchronized to a 
                            PTP based source */
  e_NTP        = (0x50), /* Any device synchronized via 
                            NTP or SNTP */
  e_HAND_SET   = (0x60), /* Time has been set by means of a
                            human interface */
  e_OTHER      = (0x90), /* Other source of time not covered 
                            by other values */
  e_INT_OSC    = (0xA0)  /* time is based on a free-running
                            oscillator */
  /* 0xF0 - 0xFE used by alternate PTP profiles */
  /* 0xFF reserved */
}PTP_t_tmSrcEnum;

/** C2DOC_stop */

/************************************************************************/
/** PTP_t_stsEnum
             Port status enumeration (chapter 8.2.5.3.1)
*/
typedef enum
{
  e_STS_INIT         = (0x1),
  e_STS_FAULTY       = (0x2),
  e_STS_DISABLED     = (0x3),
  e_STS_LISTENING    = (0x4),
  e_STS_PRE_MASTER   = (0x5),
  e_STS_MASTER       = (0x6),
  e_STS_PASSIVE      = (0x7),
  e_STS_UNCALIBRATED = (0x8),
  e_STS_SLAVE        = (0x9)
}PTP_t_stsEnum;

/************************************************************************/
/** PTP_t_delmEnum
             delay mechanism enumeration (chapter 8.2.5.4.4)
*/
typedef enum
{
  e_DELMECH_E2E   = (0x01), /* The port is configured to use the end to end
                               delay request-response mechanism */
  e_DELMECH_P2P   = (0x02), /* The port is configured to use 
                               the peer to peer delay mechanism */
  e_DELMECH_DISBl = (0xFE)  /* The port does not implement a delay 
                               mechanism, see Note chapter 8.2.5.4.4 */
}PTP_t_delmEnum;

/************************************************************************/
/** PTP_t_cfEnum
             control field enumeration for conformance with
             V1 message header (chapter 13.3.2.10)
*/
typedef enum
{
  e_CTLF_SYNC        = (0x0), /* sync message */
  e_CTLF_DELAY_REQ   = (0x1), /* delay request message */
  e_CTLF_FOLLOWUP    = (0x2), /* follow up message */
  e_CTLF_DELAY_RESP  = (0x3), /* delay response message */
  e_CTLF_MANAGEMENT  = (0x4), /* management message */
  e_CTLF_ALL_OTHERS  = (0x5)  /* all other messages */
  /* 0x06 - 0xFF reserved */
}PTP_t_cfEnum;

/** C2DOC_start */

/************************************************************************/
/** PTP_t_mntErrIdEnum
             The management error id enumeration contains the defined 
             management error id for an occurred error condition. 
             (IEEE standard chapter 15.5.4.1.4) 
*/
typedef enum
{
  e_MNTERR_RES2BIG     = (0x0001), /* requested operation could
                                      not fit in a single message */
  e_MNTERR_NOSUCHID    = (0x0002), /* mnt id not recognized */
  e_MNTERR_WRONGLENGTH = (0x0003), /* mnt id ok, but wrong length */
  e_MNTERR_WRONGVALUE  = (0x0004), /* mnt id and length ok, but one
                                      or more values were wrong */
  e_MNTERR_NOTSETABLE  = (0x0005), /* some variables could 
                                      not be set */
  e_MNTERR_NOTSUPPORT  = (0x0006), /* request not supported
                                      by this node */
  /* 0x0007 - 0xBFFF reserved */
  /* 0xC000 - 0xDFFF implementation specific 
                     This range is to be used for implementation
                     specific errors */
  /* 0xE000 - 0xFFFD PTP profile defined
                     This range is to be assigned by an alternate
                     PTP profile. */
  e_MNTERR_GENERALERR  = (0xFFFE)
  /* 0xFFFF reserved */
}PTP_t_mntErrIdEnum;

/** C2DOC_stop */

/*************************************************************************
**    data types
*************************************************************************/

/** C2DOC_start */

/************************************************************************/
/** PTP_t_TmStmp
                 This struct defines a PTP-TimeStamp. It represents the
                 past seconds and nanoseconds since the epoch, defined in
                 the clock data set. Attention should be paid to the 64 
                 bit seconds member, as only 48 bits are used within the 
                 IEEE 1588-2008 standard.
**/
typedef struct
{
  UINT64 u48_sec; /* ATTENTION, 48bit !! */
  UINT32 dw_Nsec;
}PTP_t_TmStmp;

/************************************************************************/
/** PTP_t_TmIntv
                 This struct defines a time interval in scaled Nanoseconds.
                 The Nanoseconds are multiplied by 2 to the power of 16. 
                 For example: 2.5 ns = 0x28000.
**/
typedef struct 
{
  INT64 ll_scld_Nsec; 
}PTP_t_TmIntv;

/************************************************************************/
/** PTP_t_ClkId
                 A Clock Identity identifies a PTP clock. Within Ethernet
                 the clock id corresponds to the EUI-64 network address.
**/
typedef struct
{
  UINT8 ab_id[k_CLKID_LEN];
}PTP_t_ClkId;  

/************************************************************************/
/** PTP_t_PortId
                 This structure identifies a PTP clock communication
                 interface.
**/
typedef struct    
{
  PTP_t_ClkId s_clkId;    /* clock id */
  UINT16      w_portNmb;  /* port number */
}PTP_t_PortId;

/************************************************************************/
/** PTP_t_PortAddr
                 The Port Address represents the protocol address of
                 a PTP port. k_MAX_NETW_ADDR_SZE defines the size of
                 the address name and should be 4 within Ethernet
                 communications.
**/
typedef struct    
{
  PTP_t_nwProtEnum e_netwProt; /* network protocol enumeration */
  UINT16           w_AddrLen;  /* length of the following 
                                  address array */
  UINT8            ab_Addr[k_MAX_NETW_ADDR_SZE]; 
                               /* address array */
}PTP_t_PortAddr;

/************************************************************************/
/** PTP_t_PortAddrQryTbl
                The port address query table gets used for optional
                features, such as the grand-master cluster and 
                unicast discovery. It contains a list of port 
                addresses (e.g. ac-ceptable masters). 
**/
typedef struct
{
  INT8           c_logQryIntv;        /* log2 of query interval */
  UINT16         w_actTblSze;         /* actual table size */
  PTP_t_PortAddr *aps_pAddr[k_MAX_UC_MST_TBL_SZE]; 
                                      /* port address table array */
}PTP_t_PortAddrQryTbl;

/************************************************************************/
/** PTP_t_AcceptMaster
                 The acceptable master type represents the address 
                 information of an acceptable master. 
**/
typedef struct    
{
  PTP_t_PortAddr s_acceptAdr;  /* port address of the master */
  UINT8          b_altPrio1;   /* alternate priority 1 
                                  of the master */
}PTP_t_AcceptMaster;

/** C2DOC_stop */

/************************************************************************/
/** PTP_t_ClkQual
                 The Clock Quality represents the quality of a clock
**/
typedef struct    
{
  PTP_t_clkClassEnum e_clkClass;  /* clock class */
  PTP_t_clkAccEnum   e_clkAccur;  /* accuracy of the clock */
  UINT16             w_scldVar;   /* scaled variance of offset */
}PTP_t_ClkQual;

/************************************************************************/
/** PTP_t_TLV
                 TLV type represents TLV extension fields
                 TLV = type,length,value
**/
typedef struct    
{
  PTP_t_tlvTypeEnum  e_type;   /* type */
  UINT16             w_len;    /* length of referenced array */
  UINT8*             pb_value; /* referenced value array */
}PTP_t_TLV; 

/** C2DOC_start */

/************************************************************************/
/** PTP_t_MntTLV
                 The management TLV type represents the management TLV
                 extension fields: type, length and value. The value 
                 of the management TLV contains the management id and
                 the data array accessed by pd_data. 
**/
typedef struct    
{
  UINT16  w_type;   /* tlv type */
  UINT16  w_len;    /* length of referenced array */
  UINT16  w_mntId;  /* management id */
  UINT8*  pb_data;  /* pointer to data payload */
}PTP_t_MntTLV; 

/************************************************************************/
/** PTP_t_Text
                 The PTP text data type is used to represent the textual
                 material in PTP messages. Length determines the length
                 of the text array accessed by the pb_text pointer. 
**/
typedef struct    
{
  UINT8       b_len;     /* length of referenced text array */ 
  const CHAR* pc_text;   /* referenced text array */
}PTP_t_Text;

/************************************************************************/
/** PTP_t_FltRcrd
                 The Fault Record type is used to construct fault logs. 
                 The length is the length of the complete fault record 
                 excluding the fault record length. 
**/
typedef struct    
{
  UINT16          w_fltRcrdLen;  /* length of fault record */
  PTP_t_TmStmp    s_fltTime;     /* time of fault */
  PTP_t_sevCdEnum e_severity;    /* severity of fault */
  PTP_t_Text      s_fltName;     /* name of fault */
  PTP_t_Text      s_fltVal;      /* fault value */
  PTP_t_Text      s_fltDesc;     /* Description of fault */
}PTP_t_FltRcrd;

/* data sets for OC and BC chapter 8.2 std. doc. 1588-2008 */

/************************************************************************/
/** PTP_t_DefDs
                 This data set defines inherent or assumed properties of the 
                 local clock. These values are used when the local clock 
                 becomes the grandmaster clock in a subdomain. They depend 
                 on the specific source of time for the local clock when it 
                 is the grandmaster.
**/
typedef struct     /* std. doc. 1588-2008 chapter 8.2.1 */
{
  /* static members */
  BOOLEAN       o_twoStep;  /* two step clock ? */
  PTP_t_ClkId   s_clkId;    /* clock id */
  UINT16        w_nmbPorts; /* number of ports */
  /* dynamic members */
  PTP_t_ClkQual s_clkQual;  /* clock quality */
  /* configurable members */
  UINT8         b_prio1;    /* clock priority 1 */
  UINT8         b_prio2;    /* clock priority 2 */
  UINT8         b_domn;     /* used ptp subdomain */
  BOOLEAN       o_slvOnly;  /* slave only implementation ? */ 
}PTP_t_DefDs;

/************************************************************************/
/** PTP_t_CurDs 
                 This data set defines members whose values characterize 
                 the current properties of the local clock that describe 
                 the source and quality of the local time. These values 
                 depend on the specific source of time for the local clock 
                 and whether it is the grandmaster. 
**/
typedef struct      /* std. doc. 1588-2008 chapter 8.2.2 */
{  
  UINT16       w_stepsRmvd;
  PTP_t_TmIntv s_tiOffsFrmMst;
  PTP_t_TmIntv s_tiMeanPathDel;
  /* proprietary element */
  PTP_t_TmIntv s_tiFilteredOffs; /* filtered offset */
}PTP_t_CurDs;

/************************************************************************/
/** PTP_t_PrntDs 
                 This data set defines quantities whose values characterize 
                 the current properties of the PTP port, the parent or 
                 parent port, serving as the source for the time for a PTP
                 slave port via the PTP protocol. 
**/
typedef struct   /* std. doc. 1588-2008 chapter 8.2.3 */
{
  PTP_t_PortId  s_prntPortId;
  BOOLEAN       o_prntStats;
  UINT16        w_obsPrntOffsScldLogVar;
  INT32         l_obsPrntClkPhsChgRate;
  PTP_t_ClkId   s_gmClkId;
  PTP_t_ClkQual s_gmClkQual;
  UINT8         b_gmPrio1;
  UINT8         b_gmPrio2;
  /* proprietary elements */
  BOOLEAN       o_isUc; 
}PTP_t_PrntDs;

/************************************************************************/
/** PTP_t_TmPropDs
                 This data set defines members needed to interpret the time 
                 provided in PTP messages. 
**/
typedef struct   /* std. doc. 1588-2008 chapter 8.2.4 */
{
  INT16           i_curUtcOffs;    /* current UTC offset */
  BOOLEAN         o_curUtcOffsVal; /* current UTC valid? */
  BOOLEAN         o_leap_59;       /* leap 59 sec. valid? */
  BOOLEAN         o_leap_61;       /* leap 61 sec. valid? */
  BOOLEAN         o_tmTrcable;     /* is time traceable? */
  BOOLEAN         o_frqTrcable;    /* is frequency traceable? */
  BOOLEAN         o_ptpTmScale;    /* PTP timescale? */
  PTP_t_tmSrcEnum e_tmSrc;         /* time sorce of GM */
}PTP_t_TmPropDs;

/************************************************************************/
/** PTP_t_PortDs 
                 For the single port of an ordinary clock and for each port
                 of a boundary clock, the following port configuration data 
                 set shall be maintained as the basis for protocol decisions
                 and providing values for message fields. The number of such 
                 records shall be the value of number_ports of the default 
                 data set. 
**/
typedef struct    /* std. doc. 1588-2008 chapter 8.2.5 */
{
  /* static members */
  PTP_t_PortId   s_portId;        /* PortIdentity of the local port */
  /* dynamic members */
  PTP_t_stsEnum  e_portSts;         /* current port status */
  INT8           c_mMDlrqIntv;      /* log2 of the min. delay req interval */
  PTP_t_TmIntv   s_tiP2PMPDel;      /* filtered P2P mean path delay */
  /* configurable members */
  INT8           c_AnncIntv;      /* log2 of mean Announce interval */
  UINT8          b_anncRecptTmo;  /* number of announce receipt 
                                      timouts */
  INT8           c_SynIntv;       /* log2 of mean sync interval */
  PTP_t_delmEnum e_delMech;       /* used delay mechanism */
  INT8           c_PdelReqIntv;   /* log2 of min mean pdelay request */
  UINT8          b_verNumber;     /* version number used on this port */ 
  /* implementation specific members */
  PTP_t_TmIntv   s_tiP2PMPDelUf;  /* unfiltered P2P mean path delay */
  PTP_t_PortId   s_pIdmadeMePsv;  /* port id of master that made 
                                      this port passive */
}PTP_t_PortDs;

/************************************************************************/
/** PTP_t_FMstDs 
                 For the single port of an ordinary clock and for each port
                 of a boundary clock, the following foreign master data 
                 sets shall be maintained as the basis for the BMC and the 
                 state decision event. The number of such 
                 records per port shall be minimal 5 and is configurable.
**/
typedef struct /* std. doc. 1588-2008 chapter 8.2.5 */
{
  /*general */
  PTP_t_PortId   s_frgnMstPId;  /* Port Identity of the 
                                   foreign master */
  UINT32         dw_NmbAnncMsg; /* amount of announce message in 
                                   the FOREIGN_MASTER_TIME_WINDOW */
  /* implementation specific */
  BOOLEAN        ao_Valid[k_FRGN_MST_TIME_WND]; 
                               /* flag to validate entry */
  BOOLEAN        o_isUc;       /* flag determines, if master sends  
                                  via unicast or multicast */
  PTP_t_PortAddr s_pAddr;      /* port address if it is a 
                                  unicast master */
  BOOLEAN        o_isAcc;      /* flag determines, if this node 
                                  is an acceptable master */
  UINT32         dw_retryTime; /* If a master was marked not 
                                  acceptable,  this is the timeout 
                                  for a retry */
  UINT32         adw_rxTc[k_FRGN_MST_TIME_WND]; 
                               /* rx SIS tick count of entry */
  const void     *ps_anncMsg;  /* the most recent announce
                                  message of this master */
}PTP_t_FMstDs;

/* data sets for transparent clocks chapter 8.3 std. doc. 1588-2008 */

/************************************************************************/
/** PTP_t_TcDefDs 
                This struct defines the default data set members for 
                transparent clocks.
**/
typedef struct  /* std. doc. 1588-2008 chapter 8.3.2 */
{
  /* static members */
  PTP_t_ClkId    s_clkId;    /* the clock id */
  UINT16         w_nmbPorts; /* number of ports */
  /* configurable members */
  PTP_t_delmEnum e_delMech;  /* delay mechanism */
  UINT8          b_primDom;  /* primary domain */
}PTP_t_TcDefDs;

/************************************************************************/
/** PTP_t_TcPortDs 
                This struct defines the port data set members for 
                transparent clocks.
**/
typedef struct  /* std. doc. 1588-2008 chapter 8.3.3 */
{
  /* static members */
  PTP_t_PortId  s_pId;          /* port id */
  INT8          c_logMPdelIntv; /* log min mean peer 
                                   delay request interval */
  /* dynamic members */
  BOOLEAN       o_flt;          /* faulty flag of the port */
  PTP_t_TmIntv  s_peerMPDel;    /* mean peer path delay */
  /* implementation specific */
  PTP_t_TmIntv  s_peerMPDelUf;  /* unfiltered actual peer path delay */
}PTP_t_TcPortDs;

/************************************************************************/
/** PTP_t_ProfRng
                Defines member variables of the configuration 
                file to initialize the used profile. It is part of the 
                main configuration file PTP_t_cfgRom
**/
typedef struct
{
  /* log2 of min, default and max announce interval */
  INT8  ac_annIntRng[k_RNG_SZE];
  /* min, default and max announce receipt timeout  */
  UINT8 ab_annRcpTmo[k_RNG_SZE];
  /* log2 of min, default and max sync interval */
  INT8  ac_synIntRng[k_RNG_SZE];
  /* log2 of min, default and max delay mechanism (E2E or P2P) interval */
  INT8  ac_delMIntRng[k_RNG_SZE];  
  /* domain number */
  UINT8 ab_domn[k_RNG_SZE];
  /* priority 1 */
  UINT8 ab_prio1[k_RNG_SZE];
  /* priority 2 */
  UINT8 ab_prio2[k_RNG_SZE];
}PTP_t_ProfRng;


/************************************************************************/
/** PTP_t_ClkDs
                 This data set includes all required data sets of a 
                 PTP node.
**/
typedef struct /* std. doc. 1588-2008 chapters 8.2 and 8.3 */
{
#if((k_CLK_IS_BC == TRUE) || (k_CLK_IS_OC == TRUE))
  PTP_t_DefDs    s_DefDs;
  PTP_t_CurDs    s_CurDs;
  PTP_t_PrntDs   s_PrntDs;
    /* internal time prop data set */
  PTP_t_TmPropDs s_TmPrptDs;  
    /* pointer to used time prop data set */
  PTP_t_TmPropDs *ps_extTmPrptDs;  
    /* read pointer for time prop data set */
  const PTP_t_TmPropDs *ps_rdTmPrptDs;  
  PTP_t_PortDs   as_PortDs[k_NUM_IF]; 
  PTP_t_FMstDs   aas_FMstDs[k_NUM_IF][k_CLK_NMB_FRGN_RCD];
#endif /* #if((k_CLK_IS_BC == TRUE) || (k_CLK_IS_OC == TRUE)) */
#if( k_CLK_IS_TC == TRUE )
  PTP_t_TcDefDs  s_TcDefDs;
  PTP_t_TcPortDs as_TcPortDs[k_NUM_IF];
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
  /* implementation specific */
  /* flag determines, if node is grandmaster */
  BOOLEAN        o_IsGM;
  /* user description */
  CHAR ac_userDesc[k_MAX_USRDESC_SZE+1];
  /* mean drift of module in parts per billion 
     (for clock drift recovery)*/
  INT64          ll_drift_ppb;
  BOOLEAN        o_drift_use;
  PTP_t_ProfRng  s_prof;
}PTP_t_ClkDs;


/************************************************************************/
/** PTP_t_cfgPort
                This struct defines member variables of the port configuration 
                file to initialize the port data set. It is part of the 
                main configuration file PTP_t_cfgRom
**/
typedef struct
{
  /* delay mechanism of the port */
  PTP_t_delmEnum e_delMech;
  /* log2 of  delay request interval */
  INT8           c_dlrqIntv;
  /* log2 of announce interval*/
  INT8           c_AnncIntv;
  /* announce receipt timeout */
  UINT8          b_anncRcptTmo;
  /* log2 of sync interval */
  INT8           c_syncIntv;
  /* log2 of pdelay request interval */
  INT8           c_pdelReqIntv;
}PTP_t_cfgPort;

/************************************************************************/
/** PTP_t_cfgDefDs
                Defines member variables of the configuration 
                file to initialize the default data set. It is part of the 
                main configuration file PTP_t_cfgRom
**/
typedef struct
{
  /* clock class of local clock */
  PTP_t_clkClassEnum e_clkClass;
  /* log2 of scaled variance of local clock */
  UINT16             w_scldVar;
  /* clock priority 1 of local clock */
  UINT8              b_prio1;
  /* clock priority 2 of local clock */
  UINT8              b_prio2;
  /* domain number attribute of local clock */
  UINT8              b_domNmb;
  /* TRUE = slave only */
  BOOLEAN            o_slaveOnly; 
}PTP_t_cfgDefDs;


/************************************************************************/
/** PTP_t_cfgTcDefDs
                Defines member variables of the configuration 
                file to initialize the TC default data set. It is part of the 
                main configuration file PTP_t_cfgRom
**/
typedef struct
{
  /* primary domain of the transparent clock */
  UINT8          b_TcprimDom;
  /* delay mechanism of the TC */
  PTP_t_delmEnum e_TcDelMech;
}PTP_t_cfgTcDefDs;

/************************************************************************/
/** PTP_t_cfgClkRcvr
                Defines member variables of the configuration 
                file to initialize the clock recovery. It is part of the 
                main configuration file PTP_t_cfgRom
**/
typedef struct
{
  /* stored clock drift */
  INT64    ll_drift_ppb;  
  BOOLEAN  o_drift_use;
  /* current mean path delay */
  INT64    ll_E2EmeanPDel; 
  /* current mean path delay */
  INT64    all_P2PmeanPDel[k_NUM_IF];
}PTP_t_cfgClkRcvr;

   
/************************************************************************/
/** PTP_t_cfgRom
                This struct defines member variables of the configuration 
                file to initialize the clock data set. The configuration
                can be changed during runtime and shall be stored in non-
                volatile storage to be restored after device reset. The 
                default and current data set depend on the overall clock.
                A port configuration data set must exist for each port of
                that node. Therefore several arrays of all these data are
                defined. k_NUM_IF determines the amount of ports used
                (see target.h).
**/
typedef struct
{
  /* description */
  CHAR             ac_userDesc[k_MAX_USRDESC_SZE+1];
  /* default data set member variables */
  PTP_t_cfgDefDs   s_defDs;
  /* transparent clock data set member variables */
  PTP_t_cfgTcDefDs s_tcDefDs;
  /* port configuration data set */
  PTP_t_cfgPort    as_pDs[k_NUM_IF];                                         
  /* profile ranges */
  PTP_t_ProfRng    s_prof;
}PTP_t_cfgRom;


/************************************************************************/
/** PTP_t_fltTypeEnum
          This enumeration defines all filter names used within
          the protocol stack.
*/
typedef enum
{
  e_FILT_NO     = 0x0,  /* no filter */
  e_FILT_MIN    = 0x1,  /* Minimum filter */ 
  e_FILT_EST    = 0x2,  /* Estimation filter */ 
  e_FILT_ISPCFC = 0x4   /* implementation specific filter */
}PTP_t_fltTypeEnum;

/************************************************************************/
/** PTP_t_filtMin
                This structure holds data that parameterizes the 
                filter F1
**/
typedef struct
{
  UINT32 dw_filtLen; /* filter window length */  
}PTP_t_filtMin;

/************************************************************************/
/** PTP_t_filtEst
                This structure holds data that parameterizes the
                filter F2.
**/
typedef struct
{
  UINT8  b_histLen;    /* number of history samples for statistics */
  UINT64 ddw_WhtNoise; /* amplitude of white noise in nsec */
  UINT8  b_amntFlt;    /* number of successive corrections */  
}PTP_t_filtEst;

/************************************************************************/
/** PTP_t_fltCfg
                This structure holds data that get read out of non-
                volatile storage that specify the used filter and 
                holds the parameterization of the used filter.
**/
typedef struct
{
  PTP_t_fltTypeEnum e_usedFilter; /* Filter number */
  PTP_t_filtMin     s_filtMin;    /* F1 paremeters */
  PTP_t_filtEst     s_filtEst;    /* F2 parameters */
}PTP_t_fltCfg;

/************************************************************************/
/** t_PTP_ERR_CLBK_FUNC (Error callback function)
                This typedef defines the error callback function.
**/
typedef void (*t_PTP_ERR_CLBK_FUNC)(UINT32          dw_unitId,
                                    UINT32          dw_err,
                                    PTP_t_sevCdEnum e_sevCode);

/** C2DOC_stop */

/*************************************************************************
**    global variables
*************************************************************************/


/*************************************************************************
**    function prototypes
*************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __PTPDEF_H__ */

