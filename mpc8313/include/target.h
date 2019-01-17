/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: target.h 
**    Summary: Declares all project-specific types and definitions, that 
**             are not defined in the standard, but required global of all
**             or many units. Some of it is implementation specific and has
**             to be changed for new Implementations.
**  
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: -
**
**   Compiler: gcc 3.3.2
**    Remarks: 
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __TARGET_H__
#define __TARGET_H__

#ifdef __cplusplus
extern "C"
{
#endif


/*************************************************************************
**    constants and macros
*************************************************************************/
#define LET_ALL_TS_THROUGH /* lets all timestamps through IXXAT stack */

/* compiler specific includes */
#define ERR_STR   /* optional binding of errornumber 
                     to errorstring conversion */
#define MNT_API   /* optional binding of MNT-API */ 
#define __32BIT_CPU


#include <stdlib.h>
#include <stdio.h>/*lint !e829*/
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>/*lint !e537*/
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdarg.h>/*lint !e537*/
#include <pthread.h>
#include <errno.h>

/* includes for link speed/link status */
#include <asm/types.h>
#include <sys/types.h>/*lint !e537*/
#include <linux/sockios.h>
#include <linux/ethtool.h>

#if __BYTE_ORDER == __BIG_ENDIAN
/* The host byte order is the same as network byte order,
   so these functions are all just identity.  */
  #define _ENDIAN_  (k_BIG)
#else
  #if __BYTE_ORDER == __LITTLE_ENDIAN
  #define _ENDIAN_  (k_LITTLE)
  #endif
#endif


/* callback function calling convention */
#define k_CB_CALL_CONV 
/* define for using hooks (test-SW) */
#define k_TSTHOOKS_USED        (FALSE)
/* define for debug version of SIS memory allocation for debug purpose */
#define SIS_k_ALLOC_DEBUG_VERS (FALSE)

/*********************************************************************************/
/* PTP clock and profile description (page 154/)                                  */
/*********************************************************************************/
/* Set manufacturer OUI owned by the manufacturer of the node */
#define k_CLKDES_MANU_ID    (0x00B0AE)
/* Set product description of the node - maximum number of symbols are 64.
   Following values must be set seperated by a semicolon(;):
   - manufacturer name; 
   - model number; 
   - instance identifier such as MAC address or serial number */
#define k_CLKDES_PRODUCT    "Symmetricom IEEE1588-2008 SCi"
/* Set revision data of the node - maximum number of symbols are 32.
   Following values must be set speerated by a semicolon(;):
   - hardware revision
   - firmware revision
   - software revision */
#define k_CLKDES_REVISION   "V1.01.01"
/* Set user description of the node - maximum number of symbols are 128.
   The user descrition is a configurable member via management und must
   contain following description seperated by a semicolon(;):
   - a user defined name of the device, e.g. Sensor-1
   - a user defined physical location, e.g. Testreck-2 or Shelf-5 */
#define k_CLKDES_USER       "Symmetricom IEEE1588-2008 SCi"
/* Set the profile identity of the node. This identity must be unique for
   each node and must be generated out of the OUI owned by the manufaturer
   and a unique profile index number. First three octecs are the OUI, last
   three octects identifies the profile index. */
#define k_CLKDES_PROFILE_ID {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

/*********************************************************************************/
/* PTP clock attributes                                                          */
/*********************************************************************************/

#define k_CLK_COMM_TECH     (e_UDP_IPv4) /* communication technologie of the clock */ 
#define k_NUM_IF            (1)      /* number of communication interfaces */
#define k_IMPL_SLAVE_ONLY   (FALSE)  /* is this a slave-only implementation ? */
#define k_TWO_STEP          (TRUE)   /* is this a two-step clock ? */
                                     /* (Follow up msg necessary) */
#define k_HW_NEEDS_PADDING  (TRUE)      /* hardware needs padding to V1 message size for
                                            timestamping */                                    
#define k_CLK_NMB_FRGN_RCD  (5)      /* number of foreign master data sets (min. 5) */


/* clock basic type */
#define k_CLK_IS_OC         (TRUE)  /* is node ordinary clock ? */
#define k_CLK_IS_BC         (FALSE)   /* is node boundary clock ? */
#define k_CLK_IS_TC         (FALSE)  /* is node transparent clock ? */
/* delay request mechanism */
#define k_CLK_DEL_P2P       (FALSE)   /* implements node P2P delay mechanism ? */
#define k_CLK_DEL_E2E       (TRUE)   /* implements node E2E delay mechanism ? */
/* synchronization mode */
#define k_SYNC_MODE         (k_SYNC_TM_OF_DAY) /* synchronization mode 
                                        'k_SYNC_TM_OF_DAY' or 'k_SYNC_FREQ' */
#define k_USE_STRD_DRIFT    (FALSE)  /* set clock drift to last stored drift 
                                        when in master mode and at startup */                                        
/* current utc offset at design-time */
//#define k_CUR_UTC_OFFS      (33)     /* UTC offset to TAI 22.Apr.2008 */             
#define k_CUR_UTC_OFFS      (0)     /* UTC offset to TAI 22.Apr.2008 */             
/* if a master fails or is erroneous or refuses a service, this is the time
   to retry a connection to the master [seconds]*/
#define k_MST_RETRY_TIME_SEC (60) 

/*********************************************************************************/
/* PTP device attributes                                                         */
/*********************************************************************************/
#define k_CLK_CLASS         (e_CLKC_DEF)  /* clock class */
#define k_CLK_ACCUR         (e_ACC_UNKWN) /* inherit clock accuracy */
/* the time source of the local node. Gets used for the BMC */
/* out of set: e_ATOMIC_CLK,e_GPS,e_TER_RADIO,e_PTP,e_NTP,e_HAND_SET,e_OTHER,e_INT_OSC, */
#define k_TIME_SOURCE       (e_INT_OSC) /* the time source at startup */
#define k_IS_PTP_TIMESCL    (FALSE)     /* is it PTP timescale ? */

/* define some values determining the variance */
#define k_N_PTPVAR   (20) /* N values to compute the allan deviation */

/*********************************************************************************/
/* Profile specific device attribute setting ranges                              */
/*********************************************************************************/

/* profile specific flags for the message headers */
#define k_PROF_SPEC_1       (FALSE)
#define k_PROF_SPEC_2       (FALSE)

/* log2 of min, default and max announce interval */
#define k_ANNC_INTV_MIN  (0)
#define k_ANNC_INTV_DEF  (1)
#define k_ANNC_INTV_MAX  (4)
/* min, default and max announce receipt timeout  */
#define k_ANNC_RTMO_MIN  (2u)
#define k_ANNC_RTMO_DEF  (3u)
#define k_ANNC_RTMO_MAX (10u)
/* log2 of min, default and max sync interval */
#define k_SYN_INTV_MIN   (-6)
#define k_SYN_INTV_DEF   (-6)
#define k_SYN_INTV_MAX    (1)
/* log2 of min, default and max delay mechanism (E2E or P2P) interval */
#define k_DELM_INTV_MIN  (-6)
#define k_DELM_INTV_DEF  (-6)
#define k_DELM_INTV_MAX   (5)
/* min, default and max domain number */
#define k_DOMN_MIN        (0)
#define k_DOMN_DEF        (0)
#define k_DOMN_MAX      (127)
/* min, default and max priority 1 */
#define k_PRIO1_MIN       (0)
#define k_PRIO1_DEF     (128)
#define k_PRIO1_MAX     (255)
/* min, default and max priority 2 */
#define k_PRIO2_MIN       (0)
#define k_PRIO2_DEF     (128)
#define k_PRIO2_MAX     (255)

/*********************************************************************************/
/* Transparent clock specific defines                                            */
/*********************************************************************************/
#define k_AMNT_SYN_COR  (20) /* array size for sync corrections per interface */
#define k_AMNT_DRQ_COR  (20) /* array size for delay request corrections per interface */
#define k_AMNT_P2P_COR   (2) /* array size for P2P corrections per interface */

/*********************************************************************************/
/* Optional features of the IEEE 1588 protocol                                   */
/*********************************************************************************/
#define k_ALT_MASTER       (FALSE) /* alternate master option */
#define k_ACCPT_MST        (FALSE) /* acceptable master table option */
#define k_UNICAST_CPBL     (TRUE)  /* is implementation unicast capable ? */

/*********************************************************************************/
/* Unicast specific (just relevant with k_UNICAST_CPBL set to TRUE               */
/*********************************************************************************/
#define k_MAX_UC_MST_TBL_SZE  (4) /* maximum port address query table size */
#define k_INTV_UC_MST_QRY     (1) /* log2 of unicast master query interval */
//#define k_UC_TRNSMS_DUR_SEC  (300) /* duration of a unicast service (min 10 max 1000 seconds) */
#define k_UC_TRNSMS_DUR_SEC  (w_leaseDuration) /* duration of a unicast service (min 10 max 1000 seconds) */
#define k_UC_MST_PREF       (TRUE)   /* if a master sends multicast AND unicast, 
                                      which shall be preferred ? */

/*********************************************************************************/
/* Link speed detection                                                          */
/*********************************************************************************/
/* link speed detection - is used to determine the correct inbound and outbound 
   latencies. If the latencies are not dependent of the link speed, or the link 
   speed does not change, set the flag to FALSE. */
#define k_FRQ_LAT_CHK         (TRUE)       /* flag to do link speed detection */
#define k_LAT_CHK_INTV    (k_PTP_TIME_SEC) /* interval to detect link speed */                      
                       
/*********************************************************************************/
/* Physical attributes of the target                                             */
/*********************************************************************************/
#define k_TS_GRANULARITY_NS   (10) /* timestamp granularity in nanoseconds */
#define k_TS_GRANULARITY_PS   (1000*k_TS_GRANULARITY_NS)
#define k_MAX_SYNC_OFFS       (INT64)0x3B9ACA000000LL /* max scaled nanosecond offset to sync */ 

                         
/* 10BaseT latencies */
/* ATTENTION - the latencies of 10 MBit/s connection are unknown */
#define k_CLK_INB_LAT_10BT       0  /* inbound latency in nanoseconds to subtract */
                                    /* from the rx timestamp for 10BT */
#define k_CLK_OUTB_LAT_10BT      0  /* outbound latency in nanoseconds to add */
                                    /* to the tx timestamp for 10BT */

/* 100BaseT latencies */
#define k_CLK_INB_LAT_100BT    955  /* inbound latency in nanoseconds to subtract */
                                    /* from the rx timestamp for 100BT */
#define k_CLK_OUTB_LAT_100BT  -245  /* outbound latency in nanoseconds to add */ 
                                    /* to the tx timestamp for 100BT */
/* 1000BaseT latencies */
#define k_CLK_INB_LAT_1000BT   420  /* inbound latency in nanoseconds to subtract */
                                    /* from the rx timestamp for 1000BT */
#define k_CLK_OUTB_LAT_1000BT  -45  /* outbound latency in nanoseconds to add */ 
                                    /* to the tx timestamp for 1000BT */                                                                        
                                

/* max address array size of used network technology (e.g. IPv4) */
#define k_MAX_NETW_ADDR_SZE   (4)

/*********************************************************************************/
/* Error handling of the stack                                                   */
/*********************************************************************************/
/* error threshold configurations */
//jyang: temperorily increased to 128 so during switchover, it won't drop
//       the desired master too quickly. Need Ixxat to make final fix
//#define k_MISS_DLRSP_THRES (10) /* threshold of subsequent missing delay
#define k_MISS_DLRSP_THRES (128) /* threshold of subsequent missing delay
                                   responses to go to faulty status */    
#define k_MISS_FLWUP_THRES (10) /* threshold of subsequent missing follow
                                   up to go to faulty status */  
#define k_MISS_SYNC_THRES  (10) /* threshold of subsequent missing follow
                                   up to go to faulty status */                                   
#define k_ERR_REINIT_THRESH (512 * k_NUM_IF)
                                 /* threshold for ptp errors. Exceeding this  
                                    threshold reinitializes the PTP engine */
#define k_ERR_EXIT_THRESH   (1024 * k_NUM_IF)
                                 /* threshold for ptp errors. Exceeding this  
                                    threshold shuts the PTP engine down */
#define k_SENDERR_THRES   (5)    /* threshold of number of errors while sending 
                                   after reaching, the appropriate interface 
                                   will be resetted */

/* definition of the fault log */
#define k_NUM_OF_FLT_REC    (10) /* amount of maintained fault records */

/* number of ptp packets to process in single ptp run */
#define k_NUM_MSG_THRESH    (50)

/*********************************************************************************/
/* Time definitions                                                              */
/*********************************************************************************/
/* Time slices for ptp engine call and SIS internal timer tick resolution
in nanoseconds - the timer of the system must be able to trigger this 
time slice. */
//#define k_PTP_TIME_RES_NSEC  ((UINT32)4000000) /* 1 million nsec = 1 msec  */
#define k_PTP_TIME_RES_NSEC    (l_timeResNsec)   /* 1 million nsec = 1 msec  */

/* derived timer resolution definitions */
/* time resolution in nanoseconds */
#define k_PTP_TIME_RES_USEC  (k_PTP_TIME_RES_NSEC/1000)    /* time resolution in microseconds */
#define k_PTP_TIME_RES_MSEC  (k_PTP_TIME_RES_NSEC/1000000) /* time resolution in milliseconds */

/* SIS Timer ticks resolved to one second */
#define k_PTP_TIME_SEC  ((UINT32)( 1000000000 / k_PTP_TIME_RES_NSEC )) /* one second scaled */
                                              /* to the time resolution */
/*********************************************************************************/
/* Functions and macros                                                          */
/*********************************************************************************/
/* macro for wrapping standard functions */
#define PTP_BCOPY(dst,src,len)  (memcpy(((void*)(dst)), \
                                 ((const void*)(src)),((size_t)(len))))
#define PTP_BLEN(src)           (strlen((const CHAR*)(src)))
#define PTP_BCMP(str1,str2,len) (memcmp((const void*)(str1),(const void*)(str2),(size_t)(len)))
#define PTP_MEMSET(ptr,chr,len) (memset((void*)(ptr),(INT32)(chr),(UINT32)(len)))
#define PTP_SPRINTF(buf_fmt_param) (sprintf buf_fmt_param)
#define PTP_STRNCPY(str1,str2,max) (strncpy(str1,str2,max))
#define PTP_STRCMP(str1,str2)      (strcmp(str1,str2))
#define PTP_SRAND()             (srand((UINT32)time(NULL)))
#define PTP_RAND()              (rand())

/* results for PTP_BMCP */
#define PTP_BCMP_A_SAME_B(l_ret)            (l_ret == 0)
#define PTP_BCMP_A_LESS_B(l_ret)            (l_ret < 0)
#define PTP_BCMP_A_GREATR_B(l_ret)          (l_ret > 0)

/* data type extensions */
#define ULL  ull /* extension for unsigned 64bit variables */
#define LL    ll /* extension for signed 64bit variables */

/*************************************************************************
**    data types
*************************************************************************/

/** NULL
    Standard define for NULL pointer
*/
#ifndef NULL
  #define NULL    ((void *)0)
#endif

/** BOOLEAN
    Standard data type.
*/
#ifdef BOOLEAN
  #undef BOOLEAN 
#endif
typedef u_int8_t BOOLEAN;

/** CHAR
    Standard data type.
*/
#ifdef CHAR
  #undef CHAR  
#endif
typedef char CHAR;

/** UINT8
    Standard data type.
*/
#ifdef UINT8
  #undef UINT8 
#endif
typedef u_int8_t UINT8;

/** UINT16
    Standard data type.
*/
#ifdef UINT16
  #undef UINT16 
#endif
typedef u_int16_t UINT16;

/** UINT32
    Standard data type.
*/
#ifdef UINT32
  #undef UINT32 
#endif
typedef u_int32_t UINT32;

/** UINT64
    Standard data type.
*/
#ifdef UINT64
  #undef UINT64 
#endif
typedef unsigned long long UINT64;

/** INT8
    Standard data type.
*/
#ifdef INT8
  #undef INT8  
#endif
typedef int8_t INT8;

/** INT16
    Standard data type.
*/
#ifdef INT16
  #undef INT16  
#endif
typedef int16_t INT16;

/** INT32
    Standard data type.
*/
#ifdef INT32
  #undef INT32  
#endif 
typedef int32_t INT32;

/** INT64
    Standard data type.
*/
#ifdef INT64
  #undef INT64  
#endif
typedef long long INT64;

/** DOUBLE
    Standard data type.
*/
#ifdef DOUBLE
  #undef DOUBLE  
#endif
typedef double DOUBLE;


/** TRUE:
    Standard define for true

*/
#ifndef TRUE
  #define TRUE    (1u)
#endif

/** FALSE
    Standard define for false
*/      
#ifndef FALSE
  #define FALSE   (0u)
#endif

extern UINT32 l_timeResNsec;
extern UINT16 w_leaseDuration;

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
*************************************************************************/
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __TARGET_H__ */



 




