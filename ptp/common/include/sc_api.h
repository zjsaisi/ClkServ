/*****************************************************************************
*                                                                            *
*   Copyright (C) 2009 Symmetricom, Inc., all rights reserved                *
*                                                                            *
*   This software contains proprietary and confidential information of       *
*   Symmetricom.                                                             *
*   It may not be reproduced, used, or disclosed to others for any purpose   *
*   without the written authorization of Symmetricom.                        *
*                                                                            *
******************************************************************************

FILE NAME    : sc_api.h

AUTHOR       : Ken Ho

DESCRIPTION  :

This header file contains the API definitions for SoftClient.

Revision control header:
$Id: sc_api.h 1.68 2012/02/02 13:48:38PST Jining Yang (jyang) Exp  $

******************************************************************************
*/

/* Prevent file from being included multiple times. */
#ifndef H_SC_API_h
#define H_SC_API_h

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************/
/*                       ***INCLUDES Definitions***                          */
/*  Nested includes should not be used as a general practice.  If there is a */
/*  good reason to include an include within an include, do so here. 	     */
/*****************************************************************************/
#include "sc_types.h"
#include "sc_servo_api.h"

/*****************************************************************************/
/*                       ***CONSTANT Definitions***                          */
/*  This section should be used to define constants (#defines) and macros    */
/*  that can be used by any module that includes this file.                  */
/*****************************************************************************/
#define k_CLOCK_ID_SIZE             8    /* number of bytes in a clock ID */
#define k_PHY_ADDRESS_SIZE          6
#define k_MAX_NETWORK_ADDRESS_SIZE  16
#define k_MAX_UC_MASTER_TABLE_SIZE  4
#define k_MAX_ACC_MASTER_TABLE_SIZE 4
#define k_MAX_CLK_DES               128
#define k_MAX_USR_DES               128
#define k_RANGE_SIZE                3
#define k_SC_MAX_TLV_PLD_LEN        1418
#define k_NMEA_SV                   16
#define k_COLDSTRT_INVALID_VAL      255

/* Default configuration values */
#define k_DEFAULT_CLK_CLASS         e_CLASS_SLV_ONLY
#define k_DEFAULT_PRIORITY          255
#define k_DEFAULT_VARIANCE          18944
#define k_DEFAULT_DOMAIN            0
#define k_DEFAULT_CLK_ID_BYTE       0xFF
#define k_DEFAULT_DEL_MECH          e_DEL_MECH_E2E
#define k_DEFAULT_DEL_INTV          -6
#define k_DEFAULT_ANNC_INTV         1
#define k_DEFAULT_ANNC_RCPT_TMO     3
#define k_DEFAULT_SYNC_INTV         -6
#define k_DEFAULT_PDEL_INTV         -6
#define k_DEFAULT_TWO_STEP          TRUE
#define k_DEFAULT_ADDR_MODE         e_ADDR_MODE_UNICAST
#define k_DEFAULT_BMC_MODE          e_BMC_MODE_DEFAULT
#define k_DEFAULT_UC_NEGO           TRUE
#define k_DEFAULT_UC_LEASE_DUR      300
#define k_DEFAULT_UC_QUERY_INTV     1
#define k_DEFAULT_NETW_PROTOCOL     e_SC_UDP_IPv4
#define k_DEFAULT_NETW_ADDR_LEN     4

/* The Event ID in SC_Event */
#define e_HOLDOVER	0
#define e_FREERUN	1
#define e_BRIDGING	2
#define e_CHAN_NO_DATA			3
#define e_CHAN_PERFORMANCE		4
#define e_FREQ_CHAN_QUALIFIED	5
#define e_FREQ_CHAN_SELECTED	6
#define e_TIME_CHAN_QUALIFIED	7
#define e_TIME_CHAN_SELECTED	8
#define e_CHAN_NOT_VALID		9
#define e_TIMELINE_CHANGED		10
#define e_PHASE_ALIGNMENT		11
#define e_EXCESSIVE_FREQUENCY_CORR    12
#define e_INCOMPATIBLE_TRANSPORT_MODE 13

/*****************************************************************************/
/*                       ***Data Type Specifications***                      */
/*  This section should be used to specify data types including structures,  */
/*  enumerations, unions, and redefining individual data types.              */
/*****************************************************************************/

/* Enumerations */
typedef enum
{
  e_SLAVE_MODE = (0)     /* slave mode only */
} t_ordinaryClockEnum;

typedef enum
{
  e_END2END = (0)        /* end to end mode */
} t_delayMechEnum;

typedef enum
{
  e_BMC_MODE_DEFAULT = (0)       /* default BMC mode */
} t_bmcModeEnum;

typedef enum
{
  e_ADDR_MODE_UNICAST   = (0),   /* PTP unicast mode   */
  e_ADDR_MODE_MULTICAST = (1),   /* PTP multicast mode */
  e_ADDR_MODE_HYBRID    = (2)    /* PTP hybrid mode -- for future */
} t_addrModeEnum;

typedef enum
{
  e_PTP_TIMESOURCE_GPS  = (6),   /* Timesource is GPS */
} t_timesourceEnum;

/*
------------------------------------------------------------------------
The identification of the transport protocol in use for a
communication path shall be indicated by the network
protocol enumeration. (IEEE standard chapter 7.4.1)
------------------------------------------------------------------------
*/
typedef enum
{
  e_SC_UDP_IPv4   = (0x0001), /* ethernet / UDP / IPv4 */
  e_SC_UDP_IPv6   = (0x0002), /* ethernet / UDP / IPv6 */
  e_SC_IEEE_802_3 = (0x0003), /* ethernet layer 2 */
  e_SC_DEVICENET  = (0x0004), /* Devicenet */
  e_SC_CONTROLNET = (0x0005), /* Controlnet */
  e_SC_PROFINET   = (0x0006), /* Profinet */
/*                0x7 - 0xEFFF      Reserved for assignment by the
                                    Precise Networked Clock Working
                                    Group of the IM/ST Committee */
/*               0xF000-0xFFFD      Reserved for assignment in a
                                     PTP profile */
  e_SC_UNKNOWN    = (0xFFFE) /* unknown protocol */
/*                      0xFFFF      reserved */
} t_nwProtEnum;

/*
------------------------------------------------------------------------
PTP V2 message type enumeration for the messageType field in the V2
header- 13.3.2.2
------------------------------------------------------------------------
*/
typedef enum
{
  e_MTYPE_SYNC           = (0x0),
  e_MTYPE_DELREQ         = (0x1),
  e_MTYPE_PDELREQ        = (0x2),
  e_MTYPE_PDELRESP       = (0x3),
/* reserved                (0x4) */
/* reserved                (0x5) */
/* reserved                (0x6) */
/* reserved                (0x7) */
  e_MTYPE_FLWUP          = (0x8),
  e_MTYPE_DELRESP        = (0x9),
  e_MTYPE_PDELRES_FLWUP  = (0xA),
  e_MTYPE_ANNOUNCE       = (0xB),
  e_MTYPE_SGNLNG         = (0xC),
  e_MTYPE_MNGMNT         = (0xD),
/* reserved                 0xE  */
/* reserved                 0xF  */
} t_ptpMsgTypeEnum;

/************************************************************************/
/** SC_t_apiStateEnum :
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
  e_SC_MNTAPI_PENDING  = (0x1),    /* request is still pending */
  e_SC_MNTAPI_TIMEOUT  = (0x2),    /* request is done due timeout */
  e_SC_MNTAPI_DONE     = (0x3),    /* request is done, received all
                                   expected answers */
  e_SC_MNTAPI_ABORT    = (0x4),    /* stack re-init, abort request */
  /* reserved         (0x5) */
  /* reserved         (0x6) */
  /* reserved         (0x7) */
  /* reserved         (0x8) */
  /* reserved         (0x9) */
  /* reserved         (0xA) */
  /* reserved         (0xB) */
  /* reserved         (0xC) */
  /* reserved         (0xD) */
  e_SC_MNTAPI_ERR_OR   = (0xE),    /* other request is pending */
  e_SC_MNTAPI_ERR_AL   = (0xF)     /* general error occured */
} SC_t_apiStateEnum;
/*
------------------------------------------------------------------------
The clockClass attribute of an ordinary or boundary clock denotes
the traceability of the time or frequency distributed by the
grandmaster clock. (IEEE standard chapter 7.6.2.4)
------------------------------------------------------------------------
*/
typedef enum
{
  /* 0 Reserved to enable compatibility with future versions. */
  /* 1-5 Reserved */
  e_CLASS_PRIM     = 6, /* Shall designate a clock that is synchronized to a
              primary reference time source. The timescale distributed shall
              be PTP. A clockClass 6 clock shall not be a slave to another
              clock in the domain. */
  e_CLASS_PRIM_HO  = 7, /*Shall designate a clock that has previously been
              designated as clockClass 6 but that has lost the ability to
              synchronize to a primary reference time source and is in holdover
              mode and within holdover specifications. The timescale distributed
              shall be PTP. A clockClass 7 clock shall not be a slave to another
              clock in the domain. */
/* 8 Reserved */
/* 9-10 Reserved to enable compatibility with future versions. */
/* 11-12 Reserved */
  e_CLASS_ARB      = 13, /* Shall designate a clock that is synchronized to an
              application specific source of time. The timescale distributed
              shall be ARB. A clockClass 13 clock shall not be a slave to
              another clock in the domain. */
  e_CLASS_ARB_HO   = 14, /* Shall designate a clock that has previously been
              designated as clockClass 13 but that has lost the ability to
              synchronize to an application specific source of time and is in
              holdover mode and within holdover specifications. The timescale
              distributed shall be ARB. A clockClass 14 clock shall not be a
              slave to another clock in the domain. */
/* 15-51 Reserved */
  e_CLASS_PRIM_A   = 52, /* Degradation alternative A for a clock of clockClass
              7 that is not within holdover specification. A clock of clockClass
              52 shall not be a slave to another clock in the domain. */
/* 53-57 Reserved */
  e_CLASS_ARB_A    = 58, /* Degradation alternative A for a clock of clockClass
              14 that is not within holdover specification. A clock of
              clockClass 58 shall not be a slave to another clock in the
              domain. */
/* 59-67 Reserved */
/* 68-122 For use by alternate PTP profiles */
/* 123-127 Reserved */
/* 128-132 Reserved */
/* 133-170 For use by alternate PTP profiles */
/* 171-186 Reserved */
  e_CLASS_PRIM_B   = 187, /* Degradation alternative B for a clock of
              clockClass 7 that is not within holdover specification. A clock
              of clockClass 187 may be a slave to another clock in the
              domain. */
/* 188-192 Reserved */
  e_CLASS_ARB_B    = 193, /* Degradation alternative B for a clock of
              clockClass 14 that is not within holdover specification. A clock
              of clockClass 193 may be a slave to another clock in the
              domain. */
/* 194-215 reserved */
/* 216-232 For use by alternate PTP profiles */
/* 233-247 Reserved */
  e_CLASS_DEF      = (248), /* Default. This clockClass shall be used if none
              of the other clockClass definitions apply. */
/* 249-250 Reserved */
/* 251 Reserved for version 1 compatibility, see Clause 18. */
/* 252-254 Reserved */
  e_CLASS_SLV_ONLY = (255) /* Shall be the clockClass of a slave-only clock,
              see 9.2.2. */
} t_clkClassEnum;

/************************************************************************/
/** SC_t_clkAccEnum
             The clock accuracy enumeration indicates the expected
             accuracy of a clock when it is the grandmaster, or in the
             event it becomes the grandmaster. (IEEE standard chapter
             7.6.2.5)
*/
typedef enum
{
  /* 0x00-0x1F reserved */
  e_SC_ACC_25NS   = (0x20), /* The time is accurate to within 25 ns */
  e_SC_ACC_100NS  = (0x21), /* The time is accurate to within 100 ns */
  e_SC_ACC_250NS  = (0x22), /* The time is accurate to within 250 ns */
  e_SC_ACC_1US    = (0x23), /* The time is accurate to within 1 us */
  e_SC_ACC_2500NS = (0x24), /* The time is accurate to within 2.5 us */
  e_SC_ACC_10US   = (0x25), /* The time is accurate to within 10 us */
  e_SC_ACC_25US   = (0x26), /* The time is accurate to within 25 us */
  e_SC_ACC_100US  = (0x27), /* The time is accurate to within 100 us */
  e_SC_ACC_250US  = (0x28), /* The time is accurate to within 250 us */
  e_SC_ACC_1MS    = (0x29), /* The time is accurate to within 1 ms */
  e_SC_ACC_2500US = (0x2A), /* The time is accurate to within 2.5 ms */
  e_SC_ACC_10MS   = (0x2B), /* The time is accurate to within 10 ms */
  e_SC_ACC_25MS   = (0x2C), /* The time is accurate to within 25 ms */
  e_SC_ACC_100MS  = (0x2D), /* The time is accurate to within 100 ms */
  e_SC_ACC_250MS  = (0x2E), /* The time is accurate to within 250 ms */
  e_SC_ACC_1S     = (0x2F), /* The time is accurate to within 1 s */
  e_SC_ACC_10S    = (0x30), /* The time is accurate to within 10 s */
  e_SC_ACC_L10S   = (0x31), /* The time is accurate to >10 s */
  /* 0x32 - 0x7F reserved */
  /* 0x80 - 0xFD For use by alternate PTP profiles */
  e_SC_ACC_UNKWN  = (0xFE), /* Unknown */
  e_SC_ACC_RES    = (0xFF)  /* reserved */
} SC_t_clkAccEnum;

/************************************************************************/
/** SC_t_tmSrcEnum
             This is an information only attribute indicating the
             source of time used by the grandmaster clock. (IEEE
             standard chapter 7.6.2.6)
*/
typedef enum
{
  /* time source enumeration */
  e_SC_ATOMIC_CLK = (0x10), /* A device, that is based on
                            atomic resonance */
  e_SC_GPS        = (0x20), /* device synchronized to a
                            satellite system */
  e_SC_TER_RADIO  = (0x30), /* Any device synchronized via any
                            of the radio  distribution systems */
  e_SC_PTP        = (0x40), /* Any device synchronized to a
                            PTP based source */
  e_SC_NTP        = (0x50), /* Any device synchronized via
                            NTP or SNTP */
  e_SC_HAND_SET   = (0x60), /* Time has been set by means of a
                            human interface */
  e_SC_OTHER      = (0x90), /* Other source of time not covered
                            by other values */
  e_SC_INT_OSC    = (0xA0)  /* time is based on a free-running
                            oscillator */
  /* 0xF0 - 0xFE used by alternate PTP profiles */
  /* 0xFF reserved */
} SC_t_tmSrcEnum;

/*
------------------------------------------------------------------------
PTP delay mechanism enumeration (chapter 8.2.5.4.4)
------------------------------------------------------------------------
*/
typedef enum
{
  e_DEL_MECH_E2E   = (0x01), /* The port is configured to use the end to end
                                delay request-response mechanism */
  e_DEL_MECH_P2P   = (0x02), /* The port is configured to use
                                the peer to peer delay mechanism */
  e_DEL_MECH_DISBl = (0xFE)  /* The port does not implement a delay
                                mechanism, see Note chapter 8.2.5.4.4 */
} t_delMechEnum;

typedef enum
{
  e_MSG_GRP_EVT       = 1, /* Sync, DelayRequest */
  e_MSG_GRP_GNRL      = 2, /* FollowUp, DelayResp, Announce, Signaling, Mgmt */
  e_MSG_GRP_EVT_PDEL  = 3, /* PeerDelay request, PeerDelay response */
  e_MSG_GRP_GNRL_PDEL = 4  /* PeerDelay response follow up */
} t_msgGroupEnum;

/*
------------------------------------------------------------------------
GPS parameter enumeration
------------------------------------------------------------------------
*/

typedef enum
{
  e_RCVR_FURUNO_GT8031       = 0, /* Furuno GT8031 */
  e_RCVR_ST_ERIC_GNS7560_BGA = 1  /* ST Ericsson GNS 7560, BGA Package */
} t_gpsRcvrType;

/* Structures */

typedef struct
{
  UINT8     ab_ptpClkId[k_CLOCK_ID_SIZE]; /* Local clock ID */
} t_clockIdType;


/*
------------------------------------------------------------------------
Time Code enumerations and types
------------------------------------------------------------------------
*/
typedef enum
{
  e_TIME_NEVER_SET,
  e_TIME_PTP,
  e_TIME_GPS
} t_timeOriginEnum;

typedef struct
{
  t_timeOriginEnum  e_origin;
  UINT32            l_timeSeconds;     /* TAI time in seconds since 1970-01-01 */
  UINT32            l_TimeNanoSec;
  INT16             i_utcOffset;       /* s_time + i_utcOffset = UTC time */
  BOOLEAN           o_utcOffsetValid;
  BOOLEAN           o_leap59;          /* The current UTC day will have 59 seconds in the last minute */
  BOOLEAN           o_leap61;          /* The current UTC day will have 61 seconds in the last minute */
  BOOLEAN           o_ptpTimescale;    /* if false time is ARB */
  BOOLEAN           o_activelySourced; /* actively updating values */
} t_timeCodeType;
/*
------------------------------------------------------------------------
For future use
------------------------------------------------------------------------
*/
typedef struct
{
  INT16   i_utcOffset;  /* UTC offset            */
  BOOLEAN o_leap61;     /* pending +1 leapsecond */
  BOOLEAN o_leap59;     /* pending -1 leapsecond */
  BOOLEAN o_utcValid;   /* UTC is valid          */
} t_utcType;            /* IEEE1588 15.5.3.6.2   */

/*
------------------------------------------------------------------------
For future use
------------------------------------------------------------------------
*/
typedef struct
{
  BOOLEAN o_timeTrace;   /* time is traceable      */
  BOOLEAN o_freqTrace;   /* frequency is traceable */
} t_traceType;           /* IEEE1588 15.5.3.6.3    */

/*
------------------------------------------------------------------------
For future use
------------------------------------------------------------------------
*/
typedef struct
{
  t_timesourceEnum     e_timeSource;   /* IEEE 1588 15.5.3.6.4 */
  BOOLEAN              o_ptpTimescale;
} t_timescaleType;

typedef struct
{
  t_nwProtEnum e_netwProt; /* network protocol enumeration */
  UINT16       w_addrLen;  /* length of the following address array */
  UINT8        ab_addr[k_MAX_NETWORK_ADDRESS_SIZE];
} t_PortAddr;

typedef struct
{
  BOOLEAN      o_ucNegoEnable;         /* enable unicast negotiation */
  UINT16       w_leaseDuration;        /* Lease duration (10s to 1000s) */
  INT8         c_logQryIntv;           /* log2 unicast query interval */
  UINT16       w_ucMasterTableSize;    /* size of unicast master table */
  t_PortAddr   as_addr[k_MAX_UC_MASTER_TABLE_SIZE];
} t_ucMasterType;

typedef struct
{
  UINT16       w_accMasterTableSize;    /* size of acceptable master table */
  t_PortAddr   as_addr[k_MAX_ACC_MASTER_TABLE_SIZE];
  UINT8        b_altPrio1[k_MAX_ACC_MASTER_TABLE_SIZE];
} t_accMasterType;

typedef struct
{
  t_clkClassEnum e_clkClass;  /* clock class of local clock */
  UINT8          b_prio1;     /* clock priority 1 of local clock */
  UINT8          b_prio2;     /* clock priority 2 of local clock */
  UINT16         w_scldVar;   /* scaled variance of local clock */
  UINT8          b_domNmb;    /* domain number attribute of local clock */
  BOOLEAN        o_slaveOnly; /* slave only clock */
  BOOLEAN        o_mgmtEnable;/* management interface enable */
  t_clockIdType  s_ptpClkId;  /* Local clock ID */
  INT8           c_userDescription[k_MAX_USR_DES+1]; /* up to 128 characters */
} t_ptpGeneralConfig;

typedef struct
{
   BOOLEAN Valid_2D_Fix;               /* Is the published 2D position fix "valid" */
                                       /*    relative to the required Horizontal */
                                       /*    accuracy masks ? */
   BOOLEAN Valid_3D_Fix;               /* Is the published 3D position fix "valid" */
                                       /*    relative to both the required Horizontal */
                                       /*    and Vertical accuracy masks ? */

   UINT8 SatsInView;                   /* Satellites in View count */
   UINT8 SatsUsed;                     /* Satellites in Used for Navigation count */

   UINT8 SatsInViewSVid[k_NMEA_SV];    /* Satellites in View SV id number [PRN] */
   UINT8 SatsInViewSNR[k_NMEA_SV];     /* Satellites in Signal To Noise Ratio [dBHz] */
   UINT16 SatsInViewAzim[k_NMEA_SV];   /* Satellites in View Azimuth [degrees] */
   INT8 SatsInViewElev[k_NMEA_SV];     /* Satellites in View Elevation [degrees] */
                                       /*    if = -99 then Azimuth & Elevation angles */
                                       /*         are currently unknown */
   BOOLEAN SatsInViewUsed[k_NMEA_SV];  /* Satellites in View Used for Navigation ? */

   FLOAT64 Latitude;                   /* WGS84 Latitude  [degrees, positive North] */
   FLOAT64 Longitude;                  /* WGS84 Longitude [degrees, positive East] */
   FLOAT32 Altitude_Ell;               /* Altitude above WGS84 Ellipsoid [m] */
   FLOAT32 Altitude_MSL;               /* Altitude above Mean Sea Level [m] */

   FLOAT32 H_DOP;                      /* Horizontal Dilution of Precision */
   FLOAT32 V_DOP;                      /* Vertical Dilution of Precision */
   FLOAT32 P_DOP;                      /* 3-D Position Dilution of Precision */

   FLOAT32 T_DOP;                      /* Time Dilution of Precision */

} SC_t_GPS_Status;

typedef struct
{
   INT16 Gps_WeekNo;                   /* GPS Week Number */
   UINT64 Gps_TOW;                     /* GPS Time of Week [seconds] */
   UINT16 Milliseconds;                /* UTC Milliseconds into the second  [range 0..999] */

   INT8 dtLS;                          /* UTC time difference due to leap seconds */
                                       /*     before event (seconds) */
   UINT8 WeekNo_LS;                    /* UTC week number when next leap second event */
                                       /*     occurs (weeks) */
   UINT8 DayNo_LS;                     /* UTC day of week when next leap second event */
                                       /*     occurs (days) */
   INT8 dtLSF;                         /* UTC time difference due to leap seconds after */
                                       /*     event (seconds) */
   UINT64 UTC_Offset;                  /* Current (GPS-UTC) time difference [seconds] */

} SC_t_GPS_TOD;

typedef struct
{
  BOOLEAN       o_gpsEnabled;          /* GPS engine enabled */
  t_gpsRcvrType e_rcvrType;            /* GPS receiver type (ie, Furuno, GNS 7560) */
  UINT8         ColdStrtTmOut;         /* Force a cold start if a valid fix has not */
                                       /* been obtained after the defined period [minutes] */
                                       /* 0 : Use default value (15 min) */
                                       /* 1..254: Force a cold-start after defined */ 
                                       /* period (minutes) without a fix after power-on */
                                       /* 255: Invalid */
  BOOLEAN       UserPosEnabled;        /* User position enabled */
  FLOAT64       Latitude;              /* WGS84 Latitude  [degrees, positive North] */
  FLOAT64       Longitude;             /* WGS84 Longitude [degrees, positive East] */
  FLOAT32       Altitude;              /* Altitude above WGS84 Ellipsoid [m] */
  UINT8         reserved[1];           /* Used for 32-bit word alignment */
  
} SC_t_GPS_GeneralConfig;

typedef struct
{
  t_delMechEnum  e_delMech;     /* delay mechanism of the port */
  INT8           c_delReqIntv;  /* log2 of delay request interval */
  INT8           c_anncIntv;    /* log2 of announce interval*/
  UINT8          b_anncRcptTmo; /* announce receipt timeout */
  INT8           c_syncIntv;    /* log2 of sync interval */
  INT8           c_pdelReqIntv; /* log2 of pdelay request interval */
  BOOLEAN        o_twoStep;     /* twp step clock  */
} t_ptpPortConfig;

/*
------------------------------------------------------------------------
For future use
------------------------------------------------------------------------
*/
typedef struct
{
  INT8            c_anncLimit;       /* log2 of announce interval limit */
  INT8            c_syncLimit;       /* log2 of sync interval limit */
  INT8            c_delmLimit;       /* log2 of delay interval limit */
  UINT16          w_leaseDurLimit;   /* lease duration limit */
  UINT8           b_clockAccuracy;   /* clock accuracy enumeration */
  t_timescaleType s_timescaleProp;   /* timescale properties */
  t_utcType       s_utcProp;         /* UTC properties */
  t_traceType     s_traceProp;       /* trace properties */
} t_ptpGrandMasterConfig;

typedef struct
{
  t_addrModeEnum e_ptpMode;          /* PTP mode: unicast, multicast, hybrid */
  t_bmcModeEnum  e_bmcMode;          /* BMC mode: default */
} t_ptpProfileConfig;

typedef struct
{
  t_clockIdType s_clockId;    /* clock id */
  UINT16        w_portNumber; /* port number */
} t_PortIdType;


/************************************************************************/
/** SC_t_MntTLV
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
} SC_t_MntTLV;


/************************************************************************/
/** SC_t_apiReturnTLV
             This structure will be returned upon a call of the
             callback function. Either the received management
             message or a new state due to an error, abort or
             timeout will be passed.
*/
typedef struct
{
  /* source port of received message */
  t_PortIdType s_srcPortId;
  /* the received TLV */
  SC_t_MntTLV s_mntTlv;
  /* size of TLV payload date (of the UINT8 array) */
  UINT16       w_sizeOfTlvPld;
} SC_t_apiReturnTLV;

/************************************************************************/
/** SC_t_apiClbkData
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
  void (*pf_clbkFunc)
                (UINT32*,
                 SC_t_apiReturnTLV*,
                 SC_t_apiStateEnum);
  /* handle for callback function,
     optionally for use of the application */
  UINT32* pdw_cbHandle;
} SC_t_apiClbkData;

/************************************************************************/
/** SC_t_apiMsgConfig
             This structure defines a configuration structure to pass
             all necessary information for the management message
             generation. Additionally a timeout is defined, to limit a
             request to a defined duration.
*/
typedef struct
{
  /* unicast network address */
  t_PortAddr *ps_pAddr;
  /* destination port id */
  t_PortIdType   s_desPortId;
  /* starting boundary hops */
  UINT8          b_startBoundHops;
  /* action field */
  UINT8          b_actionFld;
  /* timeout of request in seconds */
  UINT8          b_timeout;
} SC_t_apiMsgConfig;

/************************************************************************/
/** SC_t_AcceptMaster
                 The acceptable master type represents the address
                 information of an acceptable master.
**/
typedef struct
{
  t_PortAddr s_acceptAdr;  /* port address of the master */
  UINT8      b_altPrio1;   /* alternate priority 1 of the master */
} SC_t_AcceptMaster;

/************************************************************************/
/** SC_t_Text
                 The PTP text data type is used to represent the textual
                 material in PTP messages. Length determines the length
                 of the text array accessed by the pb_text pointer.
**/
typedef struct
{
  UINT8       b_len;     /* length of referenced text array */
  const char* pc_text;   /* referenced text array */
} SC_t_Text;

/*****************************************************************************/
/*                       ***Public Data Section***                           */
/*  Global variables, if used, should be defined in a ".c" source file.      */
/*  The scope of a global may be extended to other modules by declaring it as*/
/*  as an extern here. 			                                     */
/*****************************************************************************/

/*****************************************************************************/
/*                       ***Function Prototype Section***                    */
/*  This section to be used to prototype public functions.                   */
/*                                                                           */
/*  Descriptions of parameters and return code should be given for each      */
/*  function.                                                                */
/*****************************************************************************/

/*
----------------------------------------------------------------------------
                                SC_InitTimeStamper()

Description:
This function initializes the timestamper and all required
system resources. The parameter of the function could be used to
determine the corresponding Ethernet interface.

Parameters:

Inputs:
        UINT16 w_ifIndex
        Network interface index

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_InitTimeStamper(UINT16 w_ifIndex);

/*
----------------------------------------------------------------------------
                                SC_GetRxTimeStamp()

Description:
This function is called by the Softclient code.  This function requests a
timestamp from the receive timestamp unit.  The function supplies the packet
data required to identify the timestamp being requested.  The timestamp unit
should remove the timestamp from the queue when replying.  In the event the
timestamp is not found, an error is returned.

Parameters:

Inputs:
	UINT16 w_ifIndex
        Network interface index

	t_ptpMsgTypeEnum e_messageType
	type of message

	UINT16             w_sequenceId
	sequency number of message

	t_PortIdType       *ps_sendPortId
	Port ID of sender

Outputs:
	t_ptpTimeStampType *ps_timestamp
	pointer to the Tx timestamp

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_GetRxTimeStamp(
	UINT16             w_ifIndex,
	t_ptpMsgTypeEnum   e_messageType,
	UINT16             w_sequenceId,
	t_PortIdType       *ps_sendPortId,
	t_ptpTimeStampType *ps_timestamp
);

/*
----------------------------------------------------------------------------
                                SC_GetTxTimeStamp()

Description:
This function is called by the Softclient code.  This function requests a
timestamp from the transmit timestamp unit.  The function supplies the packet
data required to identify the timestamp being requested.  The timestamp unit
should remove the timestamp from the queue when replying.  In the event the
timestamp is not found, an error is returned.

Parameters:

Inputs:
	UINT16 w_ifIndex
        Network interface index

	UINT16 w_sequenceId
	Sequence ID of timestamped message

Outputs:
	t_ptpTimeStampType *ps_timestamp
	pointer to the Tx timestamp

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_GetTxTimeStamp(
	UINT16 w_ifIndex,
	UINT16 w_sequenceId,
	t_ptpTimeStampType *ps_timestamp
);

/*
----------------------------------------------------------------------------
                                SC_InitPtpConfig()

Description:
This function set initial PTP configuration parameters. The parameter will
be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
	ps_ptpGenCfg
	Pointer to PTP general configuration data structure

	ps_ptpPortCfg
	Pointer to PTP port configuration data structure

	p_ptpProfCfg
	Pointer to PTP profile configuration data structire

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed
	-2: already running
	-3: invalid value in ps_ptpGenCfg
	-4: invalid value in ps_ptpPortCfg
	-5: invalid value in ps_ptpProfCfg

-----------------------------------------------------------------------------
*/
extern int SC_InitPtpConfig(
	t_ptpGeneralConfig* ps_ptpGenCfg,
	t_ptpPortConfig*    ps_ptpPortCfg,
	t_ptpProfileConfig* ps_ptpProfCfg);

/*
----------------------------------------------------------------------------
                                SC_InitGpsConfig()
Description:
This function set initial GPS configuration parameters. The parameters will
be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
    ps_gpsGenCfg
    Pointer to GPS general configuration data structure

Outputs:
    None

Return value:
    0: function succeeded
   -1: function failed
   -2: already running
   -3: invalid value in ps_gpsGenCfg

-----------------------------------------------------------------------------
*/
extern int SC_InitGpsConfig(
   SC_t_GPS_GeneralConfig* ps_gpsGenCfg);

/*
----------------------------------------------------------------------------
                                SC_InitUnicastConfig()

Description:
This function set initial PTP unicast master table configuration parameters.
The parameter will be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
	ps_ucMstTbl
	Pointer to PTP unicast master table data structure

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed
	-2: already running
	-3: invalid value in ps_ucMstTbl

-----------------------------------------------------------------------------
*/
extern int SC_InitUnicastConfig(
	t_ucMasterType* ps_ucMstTbl);

/*
----------------------------------------------------------------------------
                                SC_InitAmtConfig()

Description:
This function set initial PTP acceptable master table configuration parameters.
The parameter will be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
	ps_accMstTbl
	Pointer to PTP acceptable master table data structure

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed
	-2: already running
	-3: invalid value in ps_accMstTbl
	-100: not implemented

-----------------------------------------------------------------------------
*/
extern int SC_InitAmtConfig(
	t_accMasterType* ps_accMstTbl);

/*
----------------------------------------------------------------------------
                                SC_InitServoConfig()

Description:
This function set initial clock servo configuration parameters. The
parameter will be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
	ps_servoCfg
	Pointer to clock configuration data structure

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed
	-2: already running
	-3: invalid value in ps_servoCfg

-----------------------------------------------------------------------------
*/
#ifndef SERVO_ONLY //in SERVO_ONLY this call is replaced by a macro in sc_servo_api.h
extern int SC_InitServoConfig(
	t_servoConfigType* ps_servoCfg);
#endif

/*
----------------------------------------------------------------------------
                                SC_InitConfigComplete()

Description:
This function is called after all the initial configuration is complete.
When this function is called, SoftClient will start initialization with
those configuration parameters.

Parameters:

Inputs
        w_multiplier
        Clock rate multiplier. This parameter is the multiplier for
        calculating timer interval:
                timer interval = clock resolution * multiplier
        where clock resolution is retrieved uing system call:
                clock_getres(CLOCK_REALTIME, &res)

        The time interval is the rate that SC_Run() will be called.

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed
	-2: already running
   -3: channels not configured yet

-----------------------------------------------------------------------------
*/
extern int SC_InitConfigComplete(UINT16 w_multiplier);

/*
----------------------------------------------------------------------------
                                SC_Run()

Description:
This function is called from the Timer ISR.  This function is used to
run the PTP stack, and must be call at a regular predetermined rate
(i.e every 4 milliseconds.)

Parameters:

	Inputs:
	None

	Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed
	-2: not initialized

-----------------------------------------------------------------------------
*/
extern int SC_Run( void );

/*
----------------------------------------------------------------------------
                                SC_Shutdown()

Description:
This function will free all the memory that was allocated using the
SC_Init() function.

Parameters:

	Inputs:
	None

	Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_Shutdown( void );

/*
----------------------------------------------------------------------------
                                SC_InitNetIf()

Description:
This function initializes network interfaces. The intialization includes
creating socket, setting up multicast membership, setting up interface
latency, etc.

Parameters:

Inputs
	UINT16 w_ptpPortNum
	PTP port number (1-65534)

Outputs
	None

Return value:
        Network interface index (0-65535): function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int SC_InitNetIf(
	UINT16 w_ptpPortNum
);

/*
----------------------------------------------------------------------------
                                SC_InitGpsIf()

Description:
This function is called by the SCi2000 to initialize the GPS interface.
It should be used to set up any communication required for the interface
the GPS receiver and also to start up any GPS related resources (ie,
GPS receiver).

Parameters:

Inputs
   None

Outputs
	None

Return value:
    0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_InitGpsIf(
	void
);

/*
----------------------------------------------------------------------------
                                SC_InitSeIf()

Description:
This function is called by the SCi2000 to initialize the Sync-E interface.
It should be used to set up any ethernet communication and tasks required
to receive ESMC information including the SSM. Also, should be used to
perform any hardware setup of the Sync-E clock measurement.

Parameters:

Inputs
   b_ChanIndex - Channel Index

Outputs
	None

Return value:
    0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_InitSeIf(
   UINT8 b_ChanIndex
);

/*
----------------------------------------------------------------------------
                                SC_InitE1If()

Description:
This function is called by the SCi2000 to initialize the E1 interface.
It should be used to set up any communication and tasks required
to receive SSM information. Also, should be used to perform any hardware
setup of the E1 receivers and clock measurement.

Parameters:

Inputs
   b_ChanIndex - Channel Index

Outputs
	None

Return value:
    0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_InitE1If(
   UINT8 b_ChanIndex
);

/*
----------------------------------------------------------------------------
                                SC_InitT1If()

Description:
This function is called by the SCi2000 to initialize the E1 interface.
It should be used to set up any communication and tasks required
to receive SSM information. Also, should be used to perform any hardware
setup of the T1 receivers and clock measurement.

Parameters:

Inputs
   b_ChanIndex - Channel Index

Outputs
	None

Return value:
    0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_InitT1If(
   UINT8 b_ChanIndex
);

/*
----------------------------------------------------------------------------
                                SC_InitRedIf()

Description:
This function is called by the SCi2000 to initialize the redundancy interface.
It should be used to perform any hardware setup required to make the redundancy
clock measurement.  It can also be used to initialize the redundancy arbitrator.

Parameters:

Inputs
   b_ChanIndex - Channel Index

Outputs
	None

Return value:
    0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_InitRedIf(
   UINT8 b_ChanIndex
);

/*
----------------------------------------------------------------------------
                                SC_CloseNetIf()

Description:
This function stops multicast memberships and close sockets.

Parameters:

Inputs
	UINT16 w_ifIndex
	Network interface index return by SC_InitNetIf()

Outputs
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
void SC_CloseNetIf(
	UINT16 w_ifIndex
);

/*
----------------------------------------------------------------------------
                                SC_CloseGpsIf()

Description:
This function is called by the SCi2000 to close the GPS interface. It
should be used by the host to free up any resources to be reclaimed
including but not limited to the interface communication port to the
GPS receiver as well as shutting down the GPS receiver itself.

Parameters:

Inputs
        None

Outputs
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
extern void SC_CloseGpsIf(
	void
);

/*
----------------------------------------------------------------------------
                                SC_CloseSeIf()

Description:
This function is called by the SCi2000 to close the Sync-E interface. It
should be used by the host to free up any resources to be reclaimed
including but not limited to the Sync-E ports

Parameters:

Inputs
        b_ChanIndex - Channel Index

Outputs
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
void SC_CloseSeIf(
	UINT8 b_ChanIndex
);

/*
----------------------------------------------------------------------------
                                SC_CloseE1If()

Description:
This function is called by the SCi2000 to close the E1 interface. It
should be used by the host to free up any resources to be reclaimed
including but not limited to the E1 ports

Parameters:

Inputs
        b_ChanIndex - Channel Index

Outputs
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
void SC_CloseE1If(
	UINT8 b_ChanIndex
);

/*
----------------------------------------------------------------------------
                                SC_CloseT1If()

Description:
This function is called by the SCi2000 to close the T1 interface. It
should be used by the host to free up any resources to be reclaimed
including but not limited to the T1 ports

Parameters:

Inputs
        b_ChanIndex - Channel Index

Outputs
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
void SC_CloseT1If(
	UINT8 b_ChanIndex
);

/*
----------------------------------------------------------------------------
                                SC_CloseRedIf()

Description:
This function is called by the SCi2000 to close the redundancy interface. It
should be used by the host to free up any resources to be reclaimed
including but not limited to the redundancy ports

Parameters:

Inputs
        b_ChanIndex - Channel Index

Outputs
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
void SC_CloseRedIf(
	UINT8 b_ChanIndex
);

/*
----------------------------------------------------------------------------
                                SC_GetIfAddrs()

Description:
This function returns the current addresses defined by the interface index.
Addresses are returned by setting the corresponding pointer of the parameter
list to the buffer containing the address octets. The size determines the
amount of octets to read and if necessary must be updated.

Parameters:

Inputs
        UINT16 w_ifIndex
        Network interface index

Outputs
        UINT8 *pb_phyAddr
        pointer to phy address

        t_PortAddr *pb_portAddr
        pointer to port address structure

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int SC_GetIfAddrs(
	UINT16     w_ifIndex,
	UINT8      *pb_phyAddr,
	t_PortAddr *ps_portAddr
);

/*
----------------------------------------------------------------------------
                                SC_GetNetLatency()

Description:
This function returns the inbound and outbound latency of measured
timestamps depending on the current link-speed of a given interface.
A change within the network and change within the link-speed normally
means modified latencies. Therefore this function is called cyclically
to monitor the outbound latency.

Parameters:

Inputs
        UINT16 w_ifIndex
        interface index

Outputs
        INT64 **ps_tiInbLat
        inbound latency in nanoseconds of the requested interface

        INT64 **ps_tiOutbLat
        outbound latency in nanoseconds of the requested interface

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_GetNetLatency(
	UINT16 w_ifIndex,
	INT64 *ps_tiInbLat,
	INT64 *ps_tiOutbLat
);

/*
----------------------------------------------------------------------------
                                SC_TxPacket()

Description:
This function passes a data packet to the host system for transmission on
the requested port.  The contents of the buffer must be copied if needed
after returning.  The call must not block.  No guarantee of delivery is
necessary.  Error returns are indicative of local transmission failures.

Parameters:

Inputs
	UINT16 w_ifIndex
	This is the destination interface port.

	t_PortAddr *ps_pAddr
	This is a pointer to the destination port address. A null pointer
	indicates multicast packet.

	t_msgGroupEnum e_msgGroup
	This is the message type (event/general.)

	const UINT8 *pb_txBuffer
	This is a pointer to the message buffer.

	UINT16 w_bufferLength
	This is the buffer length.

Outputs
	None

Return value:
	Number of bytes sent: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_TxPacket(
	UINT16 w_ifIndex,
	const t_PortAddr *ps_pAddr,
	t_msgGroupEnum e_msgGroup,
	const UINT8 *pb_txBuffer,
	UINT16 w_bufferLength
);

/*
----------------------------------------------------------------------------
                                SC_GpsTxData()

Description:
This function passes GPS data to the host system for transmission to the
GPS receiver.  The call must not block.  No guarantee of delivery is
necessary.  Error returns are indicative of local transmission failures.

Parameters:

Inputs
	UINT16 w_dataLength
	This is the length of the data stream in the buffer to be transmitted.

	const UINT8 *pb_txBuffer
	This is a pointer to the message buffer.

Outputs
	None

Return value:
	Number of bytes sent: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_GpsTxData(
   UINT16 w_dataLength,
   UINT8 *pb_txBuffer
);

/*
----------------------------------------------------------------------------
                                SC_RxPacket()

Description:
This function returns received packets from the requested port.  The
softclient will call this function on each port sequentially.   If the
packet will not fit in the buffer, no data should be copied and an error
should be returned

Parameters:

Inputs
	UINT16 w_ifIndex
	This value defines the interface port.

	t_msgGroupEnum e_msgGroup
	This is the message type (event/general.)

	INT16 *pi_bufferSize
	This is a pointer to the size of the receive buffer.

Outputs:
	UINT8 *pb_rxBuffer
	This is a pointer to the receive buffer.

	INT16 *pi_bufferSize
	This is a pointer to the size of the data received.

	t_PortAddr *ps_pAddr
	This is a pointer to the address of the sender.

Return value:
	Number of bytes received: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_RxPacket(
	UINT16 w_ifIndex,
	t_msgGroupEnum e_msgGroup,
	UINT8 *pb_rxBuffer,
	INT16 *pi_bufferSize,
	t_PortAddr *ps_pAddr
);

/*
----------------------------------------------------------------------------
                                SC_GpsRxData()

Description:
This function calls on the the host system to pass received data
GPS receiver port. The call must not block.  No guarantee of receipt is
necessary.  Error returns are indicative of local receive failures.

Parameters:

Inputs
	UINT16 w_dataLength
	This is the length of the data requested.

	const UINT8 *pb_txBuffer
	This is a pointer to the message buffer to be filled with incoming data.

Outputs
	None

Return value:
	Number of bytes sent: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_GpsRxData(
   UINT16 w_dataLength,
   UINT8 *pb_rxBuffer
);

/*
----------------------------------------------------------------------------
                                SC_InitClock()

Description:
This function is called by the Softclient code.  The host needs to include
code in this function to allocate memory, select clock inputs and dividers,
etc.  The client performs no action on successful return.  Error returns
result in the softclient staying in the INIT state and retrying the
initialization function after SYSTEM_TIMEOUT seconds.

Parameters:

Inputs
	None

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
extern int SC_InitClock( void );

/*
----------------------------------------------------------------------------
                                SC_PTP_ClkDesc()

Description: This function handles the management message
             CLOCK_DESCRIPTION. It handles the physical and
             implementation specific descriptions of the node
             and the requested port (interface). A GET message
             prompts a response including all information data.
             The only supported action command is GET.

Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.1.2
-----------------------------------------------------------------------------
*/
extern int SC_PTP_ClkDesc(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv);

/*
----------------------------------------------------------------------------
                                SC_PTP_UsrDesc()

Description: (GET,SET) This function handles the management message
             USER_DESCRIPTION. The user description defines the
             name and physical location of the device, described
             in a PTP text profile. GET and SET commands are both
             supported; however, the additional data parameter is
             only used with a SET command.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

        ps_userDesc
	On set, pointer to user descriiption

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.1.2
-----------------------------------------------------------------------------
*/
extern int SC_PTP_UsrDesc(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv,
   SC_t_Text      *ps_userDesc
);

/*
----------------------------------------------------------------------------
                                SC_PTP_DefaultDs()

Description: This function handles the management message
             DEFAULT_DATA_SET. This management message prompts
             a response including the default data set members.
             The only supported action command is GET.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Outout
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.3.1
-----------------------------------------------------------------------------
*/
int SC_PTP_DefaultDs(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_CurrentDs()

Description: This function handles the management message
             CURRENT_DATA_SET. This management message prompts
             a response including the current data set members.
             The only supported action command is GET.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.4.1
-----------------------------------------------------------------------------
*/
extern int SC_PTP_CurrentDs(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_ParentDs()

Description: This function handles the management message
             PARENT_DATA_SET. This management message prompts
             a response including the parent data set members.
             The only supported action command is GET.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.5.1
-----------------------------------------------------------------------------
*/
extern int SC_PTP_ParentDs(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_TimePropDs()

Description: This function handles the management message
             TIME_PROPERTIES_DATA_SET. This management message
             prompts a response including the time properties
             data set members. The only supported action command
             is GET.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.6.1
-----------------------------------------------------------------------------
*/
extern int SC_PTP_TimePropDs(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_PortDs()

Description: This function handles the management message
             PORT_DATA_SET. This management message prompts a
             response including the port data set members. If
             addressed to all ports, a response for each port
             must be issued. The only supported action command
             is GET.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.7.1
-----------------------------------------------------------------------------
*/
extern int SC_PTP_PortDs(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_Domain()

Description: This function handles the management message
              DOMAIN. This message gets or updates the domain
              member of the default data set. The GET and SET
              commands are both supported; however, the additional
              data parameter (b_domain) is only used with a SET
              command.

Parameters:
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

	b_domain
	On set, contains domain value
Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.3.4
-----------------------------------------------------------------------------
*/
extern int SC_PTP_Domain(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv,
   UINT8           b_domain
);

/*
----------------------------------------------------------------------------
                                SC_PTP_AnncIntv()

Description: This function handles the management message
             LOG_ANNOUNCE_INTERVAL. This message gets or updates
             the announce interval member of the port data set.
             If a request is addressed to all ports, a response
             message for each port must be issued. The GET and
             SET commands are both supported; however, the
             additional data parameter (c_anncIntv) is only used
             with a SET command.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

	c_anncIntv
	On set, contains value of the announce interval
Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.7.2
-----------------------------------------------------------------------------
*/
extern int SC_PTP_AnncIntv(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv,
   INT8            c_anncIntv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_AnncRxTimeout()

Description: This function handles the management message
             ANNOUNCE_RECEIPT_TIMEOUT. This message gets or
             updates the announce receipt timeout member of
             the port data set. If a request is addressed to
             all ports, a response message for each port must
             be issued. The GET and SET commands are both
             supported; however, the additional data parameter
             (b_anncTOut) is only used with a SET command.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

	b_anncTOut
	On set, contains value of the announce receipt timeout

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.7.3
-----------------------------------------------------------------------------
*/
extern int SC_PTP_AnncRxTimeout(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv,
   UINT8           b_anncTOut
);

/*
----------------------------------------------------------------------------
                                SC_PTP_SyncIntv()

Description: This function handles the management message
             LOG_SYNC_INTERVAL. This message gets or updates
             the sync interval member of the port data set.
             If a request is addressed to all ports, a
             response message for each port must be issued.
             The GET and SET commands are both supported;
             however, the additional data parameter
             (c_syncIntv) is only used with a SET command.

Parameters:
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

	c_syncIntv
	On set, contails value of sync interval
Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.7.4
-----------------------------------------------------------------------------
*/
extern int SC_PTP_SyncIntv(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv,
   INT8            c_syncIntv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_VersionNumber()

Description: This function handles the management message
             VERSION_NUMBER. This message gets or updates the
             version number member of the port data set. If a
             request is addressed to all ports, a response
             message for each port must be issued. The GET and
             SET commands are both supported; however, the
             additional data parameter (b_version) is only used
             with a SET command.

Parameters  :

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

	b_version
	On set, contails value of version number

Output
	ps_Tlv
	On get, pointer to location to place TLV information


Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.7.7
-----------------------------------------------------------------------------
*/
extern int SC_PTP_VersionNumber(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv,
   UINT8           b_version
);

/*
----------------------------------------------------------------------------
                                SC_PTP_EnaPort()

Description: This function handles the management message
             ENABLE_PORT. This command enables a port on an OC
             or BC using the DESIGNATED_ENABLED event. The only
             supported action command is COMMAND.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_CMD - Send Enable Port command to PTP stack

Output
	ps_Tlv
	On command, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.2.3
-----------------------------------------------------------------------------
*/
extern int SC_PTP_EnaPort(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_DisPort()

Description: This function handles the management message
             DISABLE_PORT. This command disables a port on an OC
             or BC using the DESIGNATED_DISABLED event. The only
             supported action command is COMMAND.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_CMD - Send Enable Port command to PTP stack

Output
	ps_Tlv
	On command, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.2.4
-----------------------------------------------------------------------------
*/
extern int SC_PTP_DisPort(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_Time()

Description: This function handles the management message
             TIME. This message gets or updates the local time
             of a node. Only GET command is supported.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.2.1
-----------------------------------------------------------------------------
*/
extern int SC_PTP_Time(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_ClkAccuracy()

Description: This function handles the management message
             CLOCK_ACCURACY. This message gets the
             local clock accuracy member of the clock quality
             member of the default data set. Only the GET is supported.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.2.2
-----------------------------------------------------------------------------
*/
extern int SC_PTP_ClkAccuracy(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_UtcProp()

Description: This function handles the management message
             UTC_PROPERTIES. This message gets or updates UTC
             based members of the time properties data set. Only the
             GET command is supported.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.6.2
-----------------------------------------------------------------------------
*/
extern int SC_PTP_UtcProp(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_TraceProp()

Description: This function handles the management message
             TRACEABILITY_PROPERTIES. This message gets or
             updates traceability members of the time
             properties data set. Only the GET command is
             supported.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.6.3
-----------------------------------------------------------------------------
*/
extern int SC_PTP_TraceProp(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                 SC_PTP_TimeSclProp()

Description: This function handles the management message
             TIMESCALE_PROPERTIES. This message gets or updates
             timescale members of the time properties data set.
             Only the GET command is supported.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 15.5.3.6.4
-----------------------------------------------------------------------------
*/
extern int SC_PTP_TimeSclProp(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_UcNegoEnable()

Description: This function handles the management message
             UNICAST_NEGOTIATION_ENABLE. This message enables or
             disables unicast negotiation. The GET and SET
             commands are both supported; however, the additional
             data parameter (o_ucNegoEna) is only used with a SET
             command.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

	o_unNegoEna
	TRUE  -> enabled UC negotiation
	FALSE -> disabled UC negotiation

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received


Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 16.1.4.5
-----------------------------------------------------------------------------
*/
extern int SC_PTP_UcNegoEnable(
   t_paramOperEnum b_operation,
   SC_t_MntTLV    *ps_Tlv,
   BOOLEAN         o_ucNegoEna
);

/*
----------------------------------------------------------------------------
                                SC_PTP_UCMasterTbl()

Description: This function handles the management message
             UNICAST_MASTER_TABLE. A GET request responds the
             current unicast master table including each entry.
             A SET command forces the node to reconfigure the
             unicast master table. Configuring an empty table
             (w_tblSize is zero and ps_ucMasTbl points to NULL)
             prompts the generation of a message, which causes
             the node to reset the unicast master table. Otherwise,
             a list of port addresses (ps_ucMasTbl) will be
             transmitted and stored by the receiving node. If
             addressed to all ports, a response for each port must
             be issued. The GET and SET commands are both supported;
             however, the additional data parameters are only used
             within the SET command.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

	c_queryIntv
	On set, mean interval for re-transmit a not granted  request

	w_tblSize
	On set, amount of masters / table entries

	ps_ucMasTbl
	On set, pointer to first table entry (port address)

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 17.5.3
-----------------------------------------------------------------------------
*/
extern int SC_PTP_UCMasterTbl(
   t_paramOperEnum   b_operation,
   SC_t_MntTLV      *ps_Tlv,
   INT8              c_queryIntv,
   UINT16            w_tblSize,
   const t_PortAddr *ps_ucMasTbl
);

/*
----------------------------------------------------------------------------
                                SC_PTP_UCMstMaxTblSize()

Description: This function handles the management message
             UNICAST_MASTER_MAX_TABLE_SIZE. This message prompts
             a response message including the maximum number of
             unicast master table entries of the requested port.
             If addressed to all ports, a response for each port
             must be issued. The only supported action command is
             GET.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 17.5.4
-----------------------------------------------------------------------------
*/
extern int SC_PTP_UCMstMaxTblSize(
   t_paramOperEnum   b_operation,
   SC_t_MntTLV      *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_PTP_AcceptMstTbl()

Description: This function handles the management message
             ACCEPTABLE_MASTER_TABLE. A GET request prompts the
             node to respond the current acceptable master table.
             With a SET request, new entries are configured. If
             addressed to all ports, a response for each port must
             be issued. The GET and SET commands are both supported;
             however, the additional data parameters (w_tblSize and
             ps_accMstTbl) are only used within the SET command.

Parameters  :

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

	w_tblSize
	On set, amount of table entries

	ps_accMstTbl
	On set, pointer to first table entry of accept masters

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
	 0: function succeeded
	-1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 17.5.4
-----------------------------------------------------------------------------
*/
extern int SC_PTP_AcceptMstTbl(
   t_paramOperEnum          b_operation,
   SC_t_MntTLV             *ps_Tlv,
   UINT16                   w_tblSize,
   SC_t_AcceptMaster       *ps_accMstTbl
);

/*
----------------------------------------------------------------------------
                                SC_PTP_AcceptMstTblEna()

Description: This function handles the management message
             ACCEPTABLE_MASTER_TABLE_ENABLED. This message
             enables or disables the acceptable master
             functionality of a node interface. A GET request
             gets the configuration of the enable flag; a SET
             request configures the node. If addressed to all
             ports, a response for each port must be issued.
             The GET and SET commands are both supported;
             however, the additional data parameter (o_enabled)
             is only used within the SET command.


Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

	o_enabled
	On set, TRUE  -> acceptable MST enabled
        On set, FALSE -> not enabled

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
	 0: function succeeded
	-1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 17.6.5
-----------------------------------------------------------------------------
*/
extern int SC_PTP_AcceptMstTblEna(
   t_paramOperEnum          b_operation,
   SC_t_MntTLV             *ps_Tlv,
   BOOLEAN                  o_enabled
);

/*
----------------------------------------------------------------------------
                                SC_PTP_AcceptMstTblSize()

Description: This function handles the management message
             ACCEPTABLE_MASTER_MAX_TABLE_SIZE. This message
             prompts a response message including the maximum
             number of acceptable master table entries of the
             requested port. If addressed to all ports, a
             response for each port must be issued. The only
             supported action command is GET.

Parameters:

Input
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the current log configuration

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
	 0: function succeeded
	-1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard;
         see also chapter 17.6.4
-----------------------------------------------------------------------------
*/
extern int SC_PTP_AcceptMstMaxTblSize(
   t_paramOperEnum          b_operation,
   SC_t_MntTLV             *ps_Tlv
);

/*
----------------------------------------------------------------------------
                                SC_GpsReset()

Description: Function to reset the GPS receiver. Can be used to switch GPS power off and
             on, toggle a GPIO line, write an FPGA register, whatever is required to reset
             the GPS receiver.

Parameters:

Input
   none

Output
	none

Return value:
   none

Remarks:
-----------------------------------------------------------------------------
*/
extern void SC_GpsReset(
   void
);

/*
----------------------------------------------------------------------------
                                SC_GpsStat()

Description: This function gets status from the GPS engine.

Parameters:

Input
   s_GN_GPS_Nav_Data *ps_gpsStatus
   pointer to user status structure

Output
   s_GN_GPS_Nav_Data *ps_gpsStatus
   pointer to user status structure

Return value:
   0 - success
  -1 - failure

Remarks:
-----------------------------------------------------------------------------
*/
extern int SC_GpsStat(
   SC_t_GPS_Status *ps_gpsStatus
);

/***********************************************************************
**
** Function    : SC_GpsTod
**
** Description : This function delivers TOD from the GPS engine to the 
**               host. TOD info contains GPS week number, GPS seconds,
**               UTC offset, and leap seconds information. Called by SCi. 
**
** Parameters  :
**               ps_gpsTOD - TOD structure
**
** Returnvalue :  0  - function succeeded
**               -1  - function failed
**
** Remarks     : -
**
***********************************************************************/
extern int SC_GpsTod (
   SC_t_GPS_TOD *ps_gpsTOD
);

/*
----------------------------------------------------------------------------
                                SC_TimeCode()

Description: This function needs to be written by the user.
It is called by the SCi every second to deliver time code information.
This function should not block or do any I/O
Copy the data to a local structure an return.

Parameters:

Input
 Pointer to a t_timeCodeType structure.

Output
 Timecode data.

Return value:
   None

Remarks:
-----------------------------------------------------------------------------
*/
extern void SC_TimeCode(const t_timeCodeType *ps_timeCode);


#ifdef __cplusplus
}
#endif

#endif /*  H_SC_API_h */

