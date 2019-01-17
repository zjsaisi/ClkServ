/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: GOE.c 
**    Summary: GOE - Generic Operating System Environment
**             The generic OS environment encapsulates all environmental, 
**             socket and operation system dependent functions. This unit
**             is responsible for the following realizations:
**             - convert data from network to host order and vice versa
**             - general initialization and usage, for example, read and 
**               receive of messages from the Ethernet interfaces via 
**               sockets and a TCP/IP stack
**             - detect inbound and outbound latencies
**             - read and write functions for non-volatile data storage 
**               of some configuration data sets
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: GOE_ntoh16
**             GOE_ntoh32
**             GOE_ntoh48
**             GOE_ntoh64
**             GOE_hton16
**             GOE_hton32
**             GOE_hton48
**             GOE_hton64
**             GOE_NetAddrToStr
**             GOE_NetStrToAddr
**             GOE_LockStack
**             GOE_UnLockStack
**             GOE_Init         -- moved to RDB - jyang
**             GOE_Close        -- moved to RDB - jyang
**             GOE_GetSockHdl   -- moved to RDB - jyang
**             GOE_InitMcConn   -- moved to RDB - jyang
**             GOE_CloseMcConn  -- moved to RDB - jyang
**             GOE_InitUcConn   -- moved to RDB - jyang
**             GOE_CloseUcConn  -- moved to RDB - jyang
**             GOE_getInbLat    -- moved to RDB - jyang
**             GOE_getOutbLat   -- moved to RDB - jyang
**             GOE_GetIfAddrs   -- moved to RDB - jyang
**             GOE_SendMsg      -- moved to RDB - jyang
**             GOE_RecvMsg      -- moved to RDB - jyang
**             GOE_ReadFile
**             GOE_WriteFile
**             GOE_GetErrStr
**             SetError
**             ReadLat          -- moved to RDB - jyang
**             WriteLat         -- moved to RDB - jyang
**             SendMulticast    -- moved to RDB - jyang
**             SendUnicast      -- moved to RDB - jyang
**             WriteAscii
**             ReadAscii
**             WriteBinary
**             ReadBinary
**             GetLinkSpeed     -- moved to RDB - jyang
**             GetLinkStatus    -- moved to RDB - jyang
**             ReadSectInt
**
**   Compiler: gcc 3.3.2
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
#include "sc_types.h"
#include "sc_api.h"
#include "PTP/PTPdef.h"
#include "GOE/GOE.h"
#include "API/sc_system.h"

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
/* file I/O */
#define k_MAX_FILE_NAME         (64u)
#define k_STORE_ASCII           (TRUE)

/* section strings for timestamp latency file */
#define k_SECT_HDR_STR_INBL     ("[INB_LAT]")
#define k_SECT_STR_INBL_10      ("[INB_10BT]")
#define k_SECT_STR_INBL_100     ("[INB_100BT]")
#define k_SECT_STR_INBL_1000    ("[INB_1000BT]")

#define k_SECT_HDR_STR_OUTBL    ("[OUTB_LAT]")
#define k_SECT_STR_OUTBL_10     ("[OUTB_10BT]")
#define k_SECT_STR_OUTBL_100    ("[OUTB_100BT]")
#define k_SECT_STR_OUTBL_1000   ("[OUTB_1000BT]")

/************************************************************************/
/** GOE_t_cfgLat
                This struct defines the structure of the timestamp 
                latency file in ROM. 
**/
typedef struct
{
  /* inbound latency */ 
  INT32    l_inbLat10BT;
  INT32    l_inbLat100BT;
  INT32    l_inbLat1000BT;
  /* outbound latency */
  INT32    l_outbLat10BT;
  INT32    l_outbLat100BT;
  INT32    l_outbLat1000BT;
}GOE_t_cfgLat;

typedef struct
{
  INT16  w_ifNum;
  UINT8  ab_phyAddr[k_PHY_ADDR_LEN];
  t_PortAddr portAddr;
} t_IfInfo;

/*************************************************************************
**    global variables
*************************************************************************/

/* Mutex for thread-safety of SIS */
static pthread_mutex_t s_mtxSIS;
/* Network interface list */
static t_IfInfo as_ifList[k_NUM_IF];
/* error callback function */
t_PTP_ERR_CLBK_FUNC GOE_pf_errClbk = NULL;

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static void SetError(UINT32 dw_err,PTP_t_sevCdEnum e_sevCode );
#if( k_STORE_ASCII == TRUE )
static BOOLEAN WriteAscii(const CHAR  *pc_fName,
                          const CHAR  *pc_fileAscii,
                          UINT32      dw_fileSize);
static BOOLEAN ReadAscii(const CHAR *pc_fName,
                         UINT8 *pb_data,
                         UINT32 *pdw_len);
#else
static BOOLEAN ReadBinary(const CHAR *pc_fName,
                          UINT8 *pb_data,
                          UINT32 *pdw_len);
static BOOLEAN WriteBinary(const CHAR  *pc_fName, 
                           const UINT8 *pb_fileBin,
                           UINT32       dw_fileSizeBin);
#endif
//static BOOLEAN ReadSectInt(CHAR** ppc_inp,const CHAR* pc_sectKey,INT64* pll_val);

/*************************************************************************
**    global functions
*************************************************************************/

/*************************************************************************
**
** Function    : GOE_ntoh16
**
** Description : This function converts 16bit types from network to host 
**               order. For standard use, this function must not be 
**               ported; it is adjusted using the _ENDIAN_ define in 
**               target.h. For speed optimization or memory alignment 
**               problems, this code may be changed. 
**
** See Also    : GOE_ntoh32(), GOE_ntoh48(), GOE_ntoh64(),
**               GOE_hton16(), GOE_hton32(), GOE_hton48(), GOE_hton64()
**
** Parameters  : pb_src   (IN)  - 2byte source array
**
** Returnvalue : UINT16         - converted number
**               
** Remarks     : Function is reentrant and does never block.
**
*************************************************************************/
UINT16 GOE_ntoh16(const UINT8 *pb_src)
{
  UINT16 w_ret;
#if(_ENDIAN_ == k_BIG)
  PTP_BCOPY(&w_ret,pb_src,2);  
#elif(_ENDIAN_ == k_LITTLE )
  /* LITTLE ENDIAN */
  (*(((UINT8*)(&w_ret)) + 1u)) = pb_src[0];    
  (*(((UINT8*)(&w_ret)) + 0u)) = pb_src[1];
#else
  #error "Endian is not defined"
#endif
  return w_ret;
}

/*************************************************************************
**
** Function    : GOE_ntoh32
**
** Description : This function converts 32bit types from network to host 
**               order. For standard use, this function must not be 
**               ported; it is adjusted using the _ENDIAN_ define in 
**               target.h. For speed optimization or memory alignment 
**               problems, this code may be changed. 
**
** See Also    : GOE_ntoh16(), GOE_ntoh48(), GOE_ntoh64(),
**               GOE_hton16(), GOE_hton32(), GOE_hton48(), GOE_hton64()
**
** Parameters  : pb_src   (IN)  - 4byte source array
**
** Returnvalue : UINT32         - converted number
**               
** Remarks     : Function is reentrant and does never block.
**
*************************************************************************/
UINT32 GOE_ntoh32(const UINT8 *pb_src)
{
  UINT32 dw_ret;
#if(_ENDIAN_ == k_BIG)
  PTP_BCOPY(&dw_ret,pb_src,4);  
#elif(_ENDIAN_ == k_LITTLE )
  /* LITTLE ENDIAN */
  (*(((UINT8*)(&dw_ret)) + 3u)) = pb_src[0]; 
  (*(((UINT8*)(&dw_ret)) + 2u)) = pb_src[1];    
  (*(((UINT8*)(&dw_ret)) + 1u)) = pb_src[2];    
  (*(((UINT8*)(&dw_ret)) + 0u)) = pb_src[3];  
#else
  #error "Endian is not defined"
#endif
  return dw_ret;
}

/*************************************************************************
**
** Function    : GOE_ntoh48
**
** Description : This function converts 48bit types from network to host 
**               order. In host, the 48bit number is contained in a 64bit 
**               type. In network format, it is represented as 6byte array.
**               For standard use, this function must not be ported; it is 
**               adjusted using the _ENDIAN_ define in target.h. For speed 
**               optimization or memory alignment prob-lems, this code may 
**               be changed. 
**
** See Also    : GOE_ntoh16(), GOE_ntoh32(), GOE_ntoh64(),
**               GOE_hton16(), GOE_hton32(), GOE_hton48(), GOE_hton64()
**
** Parameters  : pb_src    (IN) - 6byte source array
**
** Returnvalue : UINT64         - converted number
**               
** Remarks     : Function is reentrant and does never block.
**
*************************************************************************/
UINT64 GOE_ntoh48(const UINT8 *pb_src)
{
  UINT64 ddw_ret = 0ULL;
#if(_ENDIAN_ == k_BIG)
  PTP_BCOPY(&((UINT8*)&ddw_ret)[2],pb_src,6); /*lint !e419 !e740 !e826*/
#elif(_ENDIAN_ == k_LITTLE )
  /* LITTLE ENDIAN */  
  (*(((UINT8*)(&ddw_ret)) + 5u)) = pb_src[0]; 
  (*(((UINT8*)(&ddw_ret)) + 4u)) = pb_src[1]; 
  (*(((UINT8*)(&ddw_ret)) + 3u)) = pb_src[2]; 
  (*(((UINT8*)(&ddw_ret)) + 2u)) = pb_src[3]; 
  (*(((UINT8*)(&ddw_ret)) + 1u)) = pb_src[4]; 
  (*(((UINT8*)(&ddw_ret)) + 0u)) = pb_src[5]; 
#else
  #error "Endian is not defined"
#endif
  return ddw_ret;
}

/*************************************************************************
**
** Function    : GOE_ntoh64
**
** Description : This function converts 64bit types from network to host 
**               order. For standard use, this function must not be 
**               ported; it is adjusted using the _ENDIAN_ define in 
**               target.h. For speed optimization or memory alignment 
**               problems, this code may be changed. 
**
** See Also    : GOE_ntoh16(), GOE_ntoh32(), GOE_ntoh48(), 
**               GOE_hton16(), GOE_hton32(), GOE_hton48(), GOE_hton64()
**
** Parameters  : pb_src    (IN) - 8byte source array
**
** Returnvalue : UINT64         - converted number
**               
** Remarks     : Function is reentrant and does never block.
**
*************************************************************************/
UINT64 GOE_ntoh64(const UINT8 *pb_src)
{
  UINT64 ddw_ret;
#if(_ENDIAN_ == k_BIG)
  PTP_BCOPY(&ddw_ret,pb_src,8); 
#elif(_ENDIAN_ == k_LITTLE )
  /* LITTLE ENDIAN */
  (*(((UINT8*)(&ddw_ret)) + 7u)) = pb_src[0]; 
  (*(((UINT8*)(&ddw_ret)) + 6u)) = pb_src[1]; 
  (*(((UINT8*)(&ddw_ret)) + 5u)) = pb_src[2]; 
  (*(((UINT8*)(&ddw_ret)) + 4u)) = pb_src[3]; 
  (*(((UINT8*)(&ddw_ret)) + 3u)) = pb_src[4]; 
  (*(((UINT8*)(&ddw_ret)) + 2u)) = pb_src[5];    
  (*(((UINT8*)(&ddw_ret)) + 1u)) = pb_src[6];    
  (*(((UINT8*)(&ddw_ret)) + 0u)) = pb_src[7];  
#else
  #error "Endian is not defined"
#endif
  return ddw_ret;
}

/*************************************************************************
**
** Function    : GOE_hton16
**
** Description : This function converts 16bit types from host to network 
**               order. For standard use, this function must not be 
**               ported; it is adjusted using the _ENDIAN_ define in 
**               target.h. For speed optimization or memory alignment 
**               problems, this code may be changed. 
**
** See Also    : GOE_ntoh16(), GOE_ntoh32(), GOE_ntoh48(), GOE_ntoh64(),
**               GOE_hton32(), GOE_hton48(), GOE_hton64()
**
** Parameters  : pb_dst   (OUT) - 2byte array to write in
**               w_src    (IN)  - Number to convert
**
** Returnvalue : - 
**               
** Remarks     : Function is reentrant and does never block.
**
*************************************************************************/
void GOE_hton16(UINT8 *pb_dst,UINT16 w_src)
{
#if(_ENDIAN_ == k_BIG)
  PTP_BCOPY(pb_dst,((UINT8*)&w_src),2); 
#elif(_ENDIAN_ == k_LITTLE )
  /* LITTLE ENDIAN */  
  pb_dst[0] = (*(((UINT8*)(&w_src)) + 1)); 
  pb_dst[1] = (*(((UINT8*)(&w_src)) + 0));
#else
  #error "Endian is not defined"
#endif
}

/*************************************************************************
**
** Function    : GOE_hton32
**
** Description : This function converts 32bit types from host to network 
**               order. For standard use, this function must not be 
**               ported; it is adjusted using the _ENDIAN_ define in 
**               target.h. For speed optimization or memory alignment 
**               problems, this code may be changed. 
**
** See Also    : GOE_ntoh16(), GOE_ntoh32(), GOE_ntoh48(), GOE_ntoh64(),
**               GOE_hton16(), GOE_hton48(), GOE_hton64()
**
** Parameters  : pb_dst   (OUT) - 4byte array to write in
**               dw_src   (IN)  - Number to convert
**
** Returnvalue : - 
**               
** Remarks     : Function is reentrant and does never block.
**
*************************************************************************/
void GOE_hton32(UINT8 *pb_dst,UINT32 dw_src)
{
#if(_ENDIAN_ == k_BIG)
  PTP_BCOPY(pb_dst,((UINT8*)&dw_src),4); 
#elif(_ENDIAN_ == k_LITTLE )
  /* LITTLE ENDIAN */  
  pb_dst[0] = (*(((UINT8*)(&dw_src)) + 3)); 
  pb_dst[1] = (*(((UINT8*)(&dw_src)) + 2)); 
  pb_dst[2] = (*(((UINT8*)(&dw_src)) + 1)); 
  pb_dst[3] = (*(((UINT8*)(&dw_src)) + 0)); 
#else
  #error "Endian is not defined"
#endif
}

/*************************************************************************
**
** Function    : GOE_hton48
**
** Description : This function converts 48bit types from host to network 
**               order. In host, the 48bit number is contained in a 64bit 
**               type. In network format, it is represented as 6byte array. 
**               For standard use, this function must not be ported; it is 
**               adjusted using the _ENDIAN_ define in target.h. For speed 
**               optimization or memory alignment prob-lems, this code may 
**               be changed. 
**
** See Also    : GOE_ntoh16(), GOE_ntoh32(), GOE_ntoh48(), GOE_ntoh64(),
**               GOE_hton16(), GOE_hton32(), GOE_hton64()
**
** Parameters  : pb_dst   (OUT) - 6byte array to write in
**               pu48_src (IN)  - pointer to number to convert
**
** Returnvalue : - 
**               
** Remarks     : Function is reentrant and does never block.
**
*************************************************************************/
void GOE_hton48(UINT8 *pb_dst,const UINT64 *pu48_src)
{
#if(_ENDIAN_ == k_BIG)
  PTP_BCOPY(pb_dst,&(((UINT8*)pu48_src)[2]),6);/*lint !e740 */ 
#elif(_ENDIAN_ == k_LITTLE )
  /* LITTLE ENDIAN */  
  pb_dst[0] = (*(((UINT8*)(pu48_src)) + 5u)); 
  pb_dst[1] = (*(((UINT8*)(pu48_src)) + 4u)); 
  pb_dst[2] = (*(((UINT8*)(pu48_src)) + 3u)); 
  pb_dst[3] = (*(((UINT8*)(pu48_src)) + 2u)); 
  pb_dst[4] = (*(((UINT8*)(pu48_src)) + 1u)); 
  pb_dst[5] = (*(((UINT8*)(pu48_src)) + 0u));  
#else
  #error "Endian is not defined"
#endif
}

/*************************************************************************
**
** Function    : GOE_hton64
**
** Description : This function converts 64bit types from host to network 
**               order. For standard use, this function must not be 
**               ported; it is adjusted using the _ENDIAN_ define in 
**               target.h. For speed optimization or memory alignment 
**               problems, this code may be changed. 
**
** See Also    : GOE_ntoh16(), GOE_ntoh32(), GOE_ntoh48(), GOE_ntoh64(),
**               GOE_hton16(), GOE_hton32(), GOE_hton48()
**
** Parameters  : pb_dst   (OUT) - 8byte array to write in
**               pddw_src (IN)  - pointer to Number to convert
**
** Returnvalue : UINT32                  - converted number
**               
** Remarks     : Function is reentrant and does never block.
**
*************************************************************************/
void GOE_hton64(UINT8 *pb_dst,const UINT64 *pddw_src)
{
#if(_ENDIAN_ == k_BIG)
  PTP_BCOPY(pb_dst,pddw_src,8);  
#elif(_ENDIAN_ == k_LITTLE )
  /* LITTLE ENDIAN */
  pb_dst[0] = (*(((UINT8*)(pddw_src)) + 7u)); 
  pb_dst[1] = (*(((UINT8*)(pddw_src)) + 6u)); 
  pb_dst[2] = (*(((UINT8*)(pddw_src)) + 5u)); 
  pb_dst[3] = (*(((UINT8*)(pddw_src)) + 4u)); 
  pb_dst[4] = (*(((UINT8*)(pddw_src)) + 3u)); 
  pb_dst[5] = (*(((UINT8*)(pddw_src)) + 2u));    
  pb_dst[6] = (*(((UINT8*)(pddw_src)) + 1u));    
  pb_dst[7] = (*(((UINT8*)(pddw_src)) + 0u)); 
#else
  #error "Endian is not defined"
#endif
}

/***********************************************************************
**  
** Function    : GOE_NetAddrToStr
**  
** Description : Converts a network address from byte array format to 
**               a readable string
**
** See Also    : GOE_NetStrToAddr()
**  
** Parameters  : e_nwProt       (IN) - used network protocol
**               pc_netAddrStr (OUT) - netw. addr string
**               dw_strLen      (IN) - length of netw. addr string
**               pb_netAddr     (IN) - byte array to be converted
**               dw_addrLen     (IN) - lenght of byte array
**               
** Returnvalue : TRUE                - function succeeded
**               FALSE               - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN GOE_NetAddrToStr(PTP_t_nwProtEnum e_nwProt,
                         CHAR *pc_netAddrStr,UINT32 dw_strLen,
                         const UINT8 *pb_netAddr,UINT32 dw_addrLen)
{
  struct in_addr s_netwAddr;
  BOOLEAN o_ret = FALSE;

  /* implementation for IPv4 */
  if( e_nwProt == e_UDP_IPv4 )
  {
    if( dw_addrLen < 4 )
    {
      o_ret = FALSE;
    }
    else
    {
      /* copy byte-array in netaddr structure */
      PTP_BCOPY(&s_netwAddr.s_addr,pb_netAddr,sizeof(s_netwAddr.s_addr));
      /* change to network address string and copy in returned string */
      PTP_STRNCPY(pc_netAddrStr,inet_ntoa(s_netwAddr),dw_strLen);
      o_ret = TRUE;
    }
  }
  return o_ret;
}


/***********************************************************************
**  
** Function    : GOE_NetStrToAddr
**  
** Description : Converts a network address from string format to 
**               a byte array as used by IEEE1588
**
** See Also    : GOE_NetAddrToStr()
**  
** Parameters  : e_nwProt      (IN) - used network protocol
**               pb_netAddr   (OUT) - byte array to be written
**               dw_addrLen    (IN) - length of byte array
**               pc_netAddrStr (IN) - network address in readable format
**               
** Returnvalue : TRUE               - function succeeded
**               FALSE              - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN GOE_NetStrToAddr(PTP_t_nwProtEnum e_nwProt,
                         UINT8 *pb_netAddr,UINT32 dw_addrLen,
                         const CHAR *pc_netAddrStr)
{
  BOOLEAN o_ret = FALSE;
  UINT32  dw_addr;

  /* implementation for IPv4 */
  if( e_nwProt == e_UDP_IPv4 )
  {
    if( dw_addrLen < 4 )
    {
      o_ret = FALSE;
    }
    else
    {
      /* get internet address */
      dw_addr = inet_addr(pc_netAddrStr);
      PTP_BCOPY(pb_netAddr,&dw_addr,4);
      o_ret = TRUE;
    }
  }
  return o_ret;
}

/***********************************************************************
**  
** Function    : GOE_LockStack
**  
** Description : Locks the stack for critical sections.
**               This function is used for locking the stack
**               to the Management API and for other accesses.
**               The lock ensures data consistency when using the 
**               functions of the MNT API from other thread or task
**               contexes.
**
** See also    : GOE_UnLockStack
**  
** Parameters  : -
**               
** Returnvalue : Semaphore context if needed
** 
** Remarks     : -
**  
***********************************************************************/
void* GOE_LockStack( void )
{
  pthread_mutex_lock(&s_mtxSIS);/*lint !e534*/
  return NULL;
}

/***********************************************************************
**  
** Function    : GOE_UnLockStack
**  
** Description : Unlocks the stack for critical sections.
**
** See also    : GOE_LockStack
**  
** Parameters  : pv_sem (IN) - pointer to semaphore context if needed
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void GOE_UnLockStack(void* pv_sem)
{
  /* avoid compiler and PClint warning */
  pv_sem = pv_sem;
  pthread_mutex_unlock(&s_mtxSIS);/*lint !e534*/
}

/*************************************************************************
**
** Function    : GOE_Init
**
** Description : This function defines the general initialization
**               of the unit GOE. Some platform dependent start-up
**               initialization could be done to e.g. implement
**               additional functions or buffers. Furthermore the
**               initialization of the inter-process-communication
**               synchronization should be done if needed. This means
**               all needed initializations for using the
**               {GOE_LockStack()} and {GOE_UnLockStack()} functions.
**
**               The function parameter pf_errClbk is a pointer to the 
**               error callback function of the software stack. The 
**               initialization function of the unit GOE must copy this 
**               pointer to a local buffer, if unit GOE wants to generate 
**               error messages for example during network-initialization.
**
**               The function parameter ps_portId points to an array with 
**               an amount of k_NUM_IF (as configured in target.h)
**               structure elements of {PTP_t_PortId}. This structure
**               contains the Ethernet interface information during
**               runtime. {GOE_Init()} has to initialize these
**               parameters of each interface element of this array.
**               The port number normally corresponds to the interface
**               index of k_NUM_IF with an offset of one. The PTP clock
**               id used within the IEEE 1588 protocol software
**               corresponds to the IEEE defined 64-bit extended unique
**               identifier EUI-64 of an Ethernet network device. For
**               devices using EUI-48 or MAC-48 network addresses these
**               unique id must be mapped to EUI-64 as defined within
**               the standard. A clock uses only one identity. Thus, 
**               only the first MAC address must be ported to the clock
**               id and is used for all other port ids.
**
**               The return value delivers the amount of actual
**               initialized Ethernet interfaces.
**
** See Also    : GOE_Close()
**
** Parameters  : pf_errClbk (IN)  - error callback function
**               ps_portId  (OUT) - pointer to port identity array of
**                                  the node
**
** Returnvalue : UINT32           - number of initialized interfaces
**
** Remarks     : Implementation is CPU/compiler specific.
**
*************************************************************************/
UINT32 GOE_Init(t_PTP_ERR_CLBK_FUNC pf_errClbk, PTP_t_PortId  *ps_portId)
{
  UINT16 w_ifIdx;
  t_IfInfo *ps_if;
  t_clockIdType s_clkId;

  /* initialize error callback function */
  GOE_pf_errClbk = pf_errClbk;
  /* initialize mutex */
  if( pthread_mutex_init( &s_mtxSIS,NULL) < 0 )
  {
    /* set error */
    SetError(GOE_k_ERR_INIT_0,e_SEVC_EMRGC);    
    return 0;
  }
  for( w_ifIdx = 0; w_ifIdx < k_NUM_IF; w_ifIdx++ )
  {
    as_ifList[w_ifIdx].w_ifNum = -1;
  }
  for( w_ifIdx = 0, ps_if = as_ifList; w_ifIdx < k_NUM_IF; w_ifIdx++, ps_if++ )
  {
    UINT8 ab_rxBuf[512];
    INT16 i_bufSize;
    t_PortAddr portAddr;
    if( (ps_if->w_ifNum = SC_InitNetIf(w_ifIdx+1)) < 0 )
    {
      SetError(GOE_k_ERR_INIT_1,e_SEVC_EMRGC);    
      break;
    }

    if( SC_GetIfAddrs(ps_if->w_ifNum, ps_if->ab_phyAddr, &ps_if->portAddr) < 0 )
    {
      SetError(GOE_k_ERR_INIT_2,e_SEVC_EMRGC);    
      break;
    }

    ps_portId[w_ifIdx].w_portNmb = w_ifIdx + 1;
    /* If clock Id not configured, used MAC address to generate one */
    SC_GetClkId(&s_clkId);
    if( s_clkId.ab_ptpClkId[0] != 0xFF || s_clkId.ab_ptpClkId[1] != 0xFF ||
        s_clkId.ab_ptpClkId[2] != 0xFF || s_clkId.ab_ptpClkId[3] != 0xFF ||
        s_clkId.ab_ptpClkId[4] != 0xFF || s_clkId.ab_ptpClkId[5] != 0xFF ||
        s_clkId.ab_ptpClkId[6] != 0xFF || s_clkId.ab_ptpClkId[7] != 0xFF )
    {
      int i;
      for( i=0; i<k_CLOCK_ID_SIZE; i++ )
      {
        ps_portId[w_ifIdx].s_clkId.ab_id[i] = s_clkId.ab_ptpClkId[i];
      }
    }
    else
    {
      if( w_ifIdx == 0 )
      {
        /* copy company id of the mac address in first 3 bytes */
        memcpy(ps_portId[w_ifIdx].s_clkId.ab_id,
               ps_if->ab_phyAddr, (k_PHY_ADDR_LEN/2));
        /* set the next two bytes to sign it to MAC48 PortIdentity */
        ps_portId[w_ifIdx].s_clkId.ab_id[3] = 0xFF;
        ps_portId[w_ifIdx].s_clkId.ab_id[4] = 0xFE;
        /* copy the device number portion of the mac address in least 3 bytes */
        memcpy(&ps_portId[w_ifIdx].s_clkId.ab_id[5],
               &ps_if->ab_phyAddr[k_PHY_ADDR_LEN/2], (k_PHY_ADDR_LEN/2));
      }
      else
      {
        ps_portId[w_ifIdx].s_clkId = ps_portId[0].s_clkId;
      }
    }
    /* drain the Rx packets */
    while (SC_RxPacket(ps_if->w_ifNum, e_MSG_GRP_EVT,
                       ab_rxBuf, &i_bufSize, &portAddr) > 0);
    while (SC_RxPacket(ps_if->w_ifNum, e_MSG_GRP_GNRL,
                       ab_rxBuf, &i_bufSize, &portAddr) > 0);
  }
  return w_ifIdx;

}
 
/*************************************************************************
**
** Function    : GOE_Close
**
** Description : The function {GOE_Close()} de-initializes the Unit
**               GOE and all system resources, initialized by the
**               function {GOE_Init()}.
**
** See Also    : GOE_Init()
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : Implementation is CPU/compiler specific.
**
*************************************************************************/
void GOE_Close( void )
{
  UINT16 w_ifIdx;

  /* reset error callback function to NULL */
  GOE_pf_errClbk = NULL;
  /* remove mutex */
  pthread_mutex_destroy(&s_mtxSIS);/*lint !e534*/
  for( w_ifIdx = 0; w_ifIdx < k_NUM_IF; w_ifIdx++ )
  {
    SC_CloseNetIf(as_ifList[w_ifIdx].w_ifNum);
  }
}

/*************************************************************************
**
** Function    : GOE_GetSockHdl
**
** Description : Returns ptp event socket handle (MPC8313 specific)
**
** Parameters  : b_port    (IN)  - port number (0 or 1 )
**               pc_ifName (OUT) - pointer to array with size IF_NAMESIZE
**                                 function writes the interface name in
**                                 it.
**
** Returnvalue : socket handle
**               
** Remarks     : -
**
*************************************************************************/
SOCKET GOE_GetSockHdl( UINT8 b_port,CHAR *pc_ifName )
{
  /* This function should not be called */
  return 0;
}


/***********************************************************************
**
** Function    : GOE_InitMcConn
**
** Description : This function initializes a multicast network 
**               communication interface for PTP communication and is 
**               called for each Ethernet interface. Two sockets per 
**               interface must be initialized to receive and transmit 
**               general and event PTP messages. General messages are 
**               used in combination with the UDP port 320, and event 
**               messages are used in combination with the UDP port 319. 
**               For systems with more than one physical interface (NIC) 
**               to be able to send and receive multicast messages, the
**               sockets must be fixed to one appropriate NIC. The IP 
**               routing table should not be used, otherwise correct 
**               behavior would not guaranteed. 
**
**               There are some socket options, which should be used: 
**               - set sockets to non blocking mode (SO_NBIO)
**               - set sockets for reuse to be able to bind more sockets
**                 to the port (SO_REUSEADDR)
**               - disable multicast loopback to avoid additional 
**                 processor load (IP_MULTICAST_LOOP)
**               - add multicast membership to be able to receive and 
**                 transmit multicast messages via the IP address 
**                 224.0.1.129 (IP_ADD_MEMBERSHIP)
**               - add multicast membership to be able to receive and 
**                 transmit multicast messages via the IP address 
**                 224.0.0.107 (IP_ADD_MEMBERSHIP) - if path delay 
**                 mechanism is used
**               - set TTL for multicasts to 1 (IP_MULTICAST_TTL)
**               - set TOS to the highest priority (IP_TOS) (only 
**                 for event messages)
**
**               //Remark//: This function must be adapted to the utilized 
**               communication media and TCP/IP stack. Function and option
**               names may differ depending on the utilized TCP/IP stack.
**
** See Also    : GOE_CloseMcConn()
**
** Parameters  : w_ifIdx (IN) - communication interface index
**
** Returnvalue : TRUE         - initialization succeeded
**               FALSE        - initialization failed
** 
** Remarks     : -
**
*************************************************************************/
BOOLEAN GOE_InitMcConn(UINT16 w_ifIdx)
{
  /* avoid compiler warning */
  w_ifIdx = w_ifIdx;
  return TRUE;  
}

/*************************************************************************
**
** Function    : GOE_CloseMcConn
**
** Description : This function closes a multicast network communication 
**               interface and all related resources. 
**
** See Also    : GOE_InitMcConn()
**
** Parameters  : w_ifIdx (IN) - index of ommunication interface to close
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void GOE_CloseMcConn( UINT16 w_ifIdx)
{
  /* avoid compiler warning */
  w_ifIdx = w_ifIdx;
  /* do nothing in this portation */
} 

#if( k_UNICAST_CPBL == TRUE )
/***********************************************************************
**
** Function    : GOE_InitUcConn
**
** Description : This function initializes a unicast network communication 
**               interface for PTP communication. Two sockets must be 
**               initialized to receive and transmit general and event PTP 
**               messages. General messages are used in combination with 
**               the UDP port 320, and event messages are used in 
**               combination with the UDP port 319. 
**
**               There are some socket options, which should be used:
**               - set sockets to non blocking mode (SO_NBIO)
**               - set sockets for reuse to be able to bind more sockets 
**                 to the port (SO_REUSEADDR)
**               - set TOS to the highest priority (IP_TOS) (only for 
**                 event messages)
**
**               //Remark//: This function must be adapted to the utilized 
**               communication media and TCP/IP stack. Function and option 
**               names may differ depending on the utilized TCP/IP stack. 
**
** See Also    : GOE_CloseUcConn()
**
** Parameters  : -
**
** Returnvalue : TRUE  - initialization succeeded
**               FALSE - initialization failed
** 
** Remarks     : -
**
*************************************************************************/
BOOLEAN GOE_InitUcConn( void )
{
  return TRUE;
}

/*************************************************************************
**
** Function    : GOE_CloseUcConn
**
** Description : This function closes the unicast network communication 
**               interface and all related resources. 
**
** See Also    : GOE_InitUcConn()
**
** Parameters  : -
**
** Returnvalue : -
**               
** Remarks     : -
**
*************************************************************************/
void GOE_CloseUcConn( void )
{
}
#endif /* #if( k_UNICAST_CPBL == TRUE ) */

/*************************************************************************
**
** Function    : GOE_getInbLat
**
** Description : This function determines and returns the inbound latency 
**               of measured time-stamps depending on the current link-speed 
**               of a given interface. A change within the network and 
**               change within the link-speed normally requires modified 
**               latencies. Therefore, this function is called cyclically 
**               to monitor the inbound latency, if k_FRQ_LAT_CHK in target.h 
**               is configured to TRUE. Otherwise, this function is only 
**               called once during start-up. 
**
** See Also    : GOE_getOutbLat()
**
** Parameters  : w_ifIdx     (IN)  - requested interface
**               ps_tiInbLat (OUT) - inbound latency in PTP time interval
**                                   of the requested interface
**
** Returnvalue : -
**
** Remarks     : Function is reentrant and does never block.
**
*************************************************************************/
void GOE_getInbLat( UINT16 w_ifIdx ,PTP_t_TmIntv *ps_tiInbLat)
{
  INT64 outbLat;
  INT64 inbLat;
  
  if(SC_GetNetLatency(as_ifList[w_ifIdx].w_ifNum, &inbLat, &outbLat))
  {
      inbLat = 0;
  }
  ps_tiInbLat->ll_scld_Nsec = PTP_NSEC_TO_INTV(inbLat);
}

/*************************************************************************
**
** Function    : GOE_getOutbLat
**
** Description : This function determines and returns the outbound latency 
**               of measured time-stamps depending on the current link-speed 
**               of a given interface. A change within the network and 
**               change within the link-speed normally requires modified 
**               latencies. Therefore, this function is called cyclically to 
**               monitor the outbound latency, if k_FRQ_LAT_CHK in target.h 
**               is configured to TRUE. Otherwise, this function is only 
**               called once during start-up.
**
** See Also    : GOE_getInbLat()
**
** Parameters  : w_ifIdx      (IN)  - requested interface
**               ps_tiOutbLat (OUT) - outbound latency in PTP time 
**                                    interval of the requested interface
**
** Returnvalue : -
**               
** Remarks     : Function is reentrant and does never block.
**
*************************************************************************/
void GOE_getOutbLat( UINT16 w_ifIdx ,PTP_t_TmIntv *ps_tiOutbLat)
{
  INT64 outbLat;
  INT64 inbLat;
  
  if(SC_GetNetLatency(as_ifList[w_ifIdx].w_ifNum, &inbLat, &outbLat))
  {
      outbLat = 0;
  }
  ps_tiOutbLat->ll_scld_Nsec = PTP_NSEC_TO_INTV(outbLat);
}

/*************************************************************************
**
** Function    : GOE_GetIfAddrs
**
** Description : This function retrieves the current addresses defined 
**               by the interface index. Addresses are returned by
**               setting the corresponding pointer of the parameter list
**               to the buffer containing the address octets. The size 
**               determines the amount of octets to read and if  
**               necessary must be updated.
**
** See Also    : NIF_GetIfAddrs()
**
** Parameters  : pw_phyAddrLen  (OUT) - number of phyAddr octets
**               ppb_phyAddr    (OUT) - pointer to phyAddr
**               pw_portAddrLen (OUT) - number of portAdrr octets
**               ppb_portAddr   (OUT) - pointer to portAddr
**               w_ifIdx         (IN) - interface index
**               
** Returnvalue : TRUE           - function succeeded
**               FALSE          - function failed
**
** Remarks     : -
**
*************************************************************************/
BOOLEAN GOE_GetIfAddrs(UINT16 *pw_phyAddrLen,
                       UINT8  **ppb_phyAddr,
                       UINT16 *pw_portAddrLen,
                       UINT8  **ppb_portAddr,
                       UINT16 w_ifIdx)
{
  /* all interfaces work in IPv4 -> all lengths equal */
  /* set length of physical and port address */
  *pw_phyAddrLen  = k_PHY_ADDR_LEN;
  *ppb_phyAddr = as_ifList[w_ifIdx].ab_phyAddr;
  *pw_portAddrLen = as_ifList[w_ifIdx].portAddr.w_addrLen;
  *ppb_portAddr = as_ifList[w_ifIdx].portAddr.ab_addr;
  return TRUE;
}

/*************************************************************************
**
** Function    : GOE_SendMsg
**
** Description : This function sends a message over the utilized 
**               communication media. The TCP/IP stack must not use the 
**               routing table to select the NIC. Instead, the function 
**               itself must select the desired NIC (option IP_MULTICAST_IF).
**               For this purpose, the function is required to identify
**               the message as multicast or unicast and choose the correct 
**               interface using the interface index / port number and 
**               initialized multicast and unicast sockets (see also 
**               {GOE_InitMcConn()} and {GOE_InitUcConn()}). This function 
**               must return immediately and may not block after 
**               transmission attempts. 
**
**               //Remark//: This function must be adapted to the utilized 
**               communication media and TCP/IP stack. Function and option
**               names may differ depending on the utilized TCP/IP stack.
**
** See Also    : GOE_RecvMsg()
**
** Parameters  : ps_pAddr  (IN) - pointer to destination port address
**                                (=NULL for multicast messages)
**               pb_txBuf  (IN) - pointer to message buffer
**               b_socket  (IN) - socket type (evt/general + peer delay)
**               w_ifIdx   (IN) - port number to send (interface index)
**               w_Len     (IN) - size of send buffer
**               
** Returnvalue : TRUE           - function succeeded
**               FALSE          - function failed
**
** Remarks     : Function is not reentrant and does never block.
**
*************************************************************************/
BOOLEAN GOE_SendMsg(const PTP_t_PortAddr *ps_pAddr,
                    const UINT8    *pb_txBuf,
                          UINT8    b_socket,
                          UINT16   w_ifIdx,
                          UINT16   w_len)
{
  int ret;

  if (ps_pAddr == NULL)
  {
    ret = SC_TxPacket(as_ifList[w_ifIdx].w_ifNum, NULL,
                      (t_msgGroupEnum)b_socket, pb_txBuf, w_len);
  }
  else
  {
    t_PortAddr portAddr;
    memcpy(portAddr.ab_addr, ps_pAddr->ab_Addr, sizeof(portAddr.ab_addr));
    ret = SC_TxPacket(as_ifList[w_ifIdx].w_ifNum, &portAddr,
                      (t_msgGroupEnum)b_socket, pb_txBuf, w_len);
  }
  if (ret > 0)
    return TRUE;
  else
    return FALSE;
}

/*************************************************************************
**
** Function    : GOE_RecvMsg
**
** Description : This function reads a message from a specified 
**               communication interface (w_ifIdx) and is called 
**               periodically for each NIC. Messages received due to a 
**               possibly avail-able loopback interface must be discarded.
**               This should not happen, if option IP_MULTICAST_LOOP is 
**               set (see also {GOE_InitMcConn()}). Unicast messages are 
**               addressed to specific IP addresses and therefore are also
**               received through the initialized multicast sockets for 
**               each NIC. If a unicast message has arrived, the port 
**               address of the sender must be copied to ps_pAdr. This 
**               function must return immediately and may not block.
**
**               //Remark//: This function must be adapted to the utilized 
**               communication media and TCP/IP stack. Function and option 
**               names may differ depending on the utilized TCP/IP stack. 
**
** See Also    : GOE_SendMsg()
**
** Parameters  : w_ifIdx         (IN) - specified interface to read 
**               b_socket        (IN) - socket type (evt/general + peer delay)
**               pb_rxBuf       (OUT) - RX buffer to read in
**               pi_bufSize  (IN/OUT) - pointer to size of RX buffer
**               ps_pAdr        (OUT) - port address of sender
**                                      (used to determine unicast sender)
**
** Returnvalue : TRUE                 - message was received
**               FALSE                - no message available
**               
** Remarks     : Function is not reentrant and does never block.
**
*************************************************************************/
BOOLEAN GOE_RecvMsg( UINT16         w_ifIdx,
                     UINT8          b_socket,
                     UINT8          *pb_rxBuf,
                     INT16          *pi_bufSize,
                     PTP_t_PortAddr *ps_pAdr)
{
  t_PortAddr portAddr;
  int ret;

  ret = SC_RxPacket(as_ifList[w_ifIdx].w_ifNum, (t_msgGroupEnum)b_socket,
                    pb_rxBuf, pi_bufSize, &portAddr);
  if (ret > 0)
  {
    ps_pAdr->w_AddrLen  = 4;
    ps_pAdr->e_netwProt = e_UDP_IPv4;
    memcpy(ps_pAdr->ab_Addr, portAddr.ab_addr, 4);
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

/***********************************************************************
**  
** Function    : GOE_ReadFile
**  
** Description : This function reads a file out of ROM that preciously 
**               was written by the function {GOE_WriteFile()}. If the 
**               file to be read does not exist, the function returns 
**               FALSE. The data can be returned either as binary or as 
**               ASCII file. The appropriate file type must be returned 
**               as well.
**
**               The porting engineer is responsible for the 
**               implementation of a corresponding file system to save 
**               all data into non-volatile storage. There is a maximum
**               amount of four different files which are used.
**
** Parameters  : dw_fIdx      (IN) - unique file index
**               pc_fName     (IN) - unique file name
**               pb_data  (IN/OUT) - buffer to be written
**               pdw_len  (IN/OUT) - in: length of buffer / out: read data size
**               pb_mode  (OUT)    - file mode (k_FILE_ASCII / k_FILE_BNRY) 
**               
** Returnvalue : TRUE              - function succeeded
**               FALSE             - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN GOE_ReadFile(UINT32     dw_fIdx,
                     const CHAR *pc_fName,
                     UINT8      *pb_data,
                     UINT32     *pdw_len,
                     UINT8      *pb_mode)
{
  BOOLEAN o_ret;
  /* unused variable in this portation */
  dw_fIdx = dw_fIdx;
#if( k_STORE_ASCII == TRUE )
  /* call ascii routine */
  o_ret = ReadAscii(pc_fName,pb_data,pdw_len);
  *pb_mode = k_FILE_ASCII;
#else
  o_ret = ReadBinary(pc_fName,pb_data,pdw_len);
  *pb_mode = k_FILE_BNRY;
#endif
  return o_ret;
}


/***********************************************************************
**  
** Function    : GOE_WriteFile
**  
** Description : This function writes a file to ROM. The data is 
**               represented in two buffers, a ascii buffer and a 
**               binary buffer. The application can choose to store 
**               one of both. The reading function has to take care, 
**               that it returns the stored buffer and declares,
**               which of both it is. There is more than one file 
**               to read. The application differ the files by using
**               either the file index or the file name. 
**
**               The porting engineer is responsible for the 
**               implementation of a corresponding file system to save 
**               all data into non-volatile storage. There is a maximum
**               amount of four different files which are used.
**
** See Also    : GOE_ReadFile()
**  
** Parameters  : dw_fIdx          (IN) - unique file index 
**               pc_fName         (IN) - unique file name
**               pc_fileAscii     (IN) - pointer to ascii file
**               dw_fileSizeAscii (IN) - size of ascii file
**               pb_fileBin       (IN) - binary file
**               dw_fileSizeBin   (IN) - size of binary file
**               
** Returnvalue : TRUE                  - function succeeded
**               FALSE                 - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN GOE_WriteFile(UINT32      dw_fIdx,
                      const CHAR  *pc_fName,
                      const CHAR  *pc_fileAscii,
                      UINT32       dw_fileSizeAscii,    
                      const UINT8 *pb_fileBin,
                      UINT32       dw_fileSizeBin)
{
  /* unused variable in this portation */
  dw_fIdx          = dw_fIdx;
  
#if( k_STORE_ASCII == TRUE )
  /* avoid compiler warning for unused variables */
  pb_fileBin       = pb_fileBin;
  dw_fileSizeBin   = dw_fileSizeBin;  
  return WriteAscii(pc_fName,pc_fileAscii,dw_fileSizeAscii);
#else
  /* avoid compiler warning for unused variables */
  pc_fileAscii     = pc_fileAscii;
  dw_fileSizeAscii = dw_fileSizeAscii;
  return WriteBinary(pc_fName,pb_fileBin,dw_fileSizeBin);
#endif
}

/*************************************************************************
**
** Function    : GOE_GetNetIfNum
**
** Description : This function returns network interface number received
**               during init.
**
** See Also    : GOE_Init()
**
** Parameters  : w_ifIdx   (IN) - interface index
**               
** Returnvalue : interface number - function succeeded
**               -1               - function failed
**
*************************************************************************/
INT16 GOE_GetNetIfNum(UINT16 w_ifIdx)
{
  if( w_ifIdx < k_NUM_IF )
    return as_ifList[w_ifIdx].w_ifNum;
  else
    return -1;
}

#ifdef ERR_STR
/*************************************************************************
**
** Function    : GOE_GetErrStr
**
** Description : This function resolves the error number to an
**               according error string.
**
** Parameters  : dw_errNmb (IN) - error number
**
** Returnvalue : CHAR*         - pointer to corresponding error string
**
** Remarks     : Gets compiled with definition of ERR_STR.
**  
***********************************************************************/
const CHAR* GOE_GetErrStr(UINT32 dw_errNmb)
{
  const CHAR* pc_ret = NULL;
  static CHAR  ac_str[50];
  /* return according error string */
  switch(dw_errNmb)
  {
    case GOE_k_ERR_SEND:
    {
      pc_ret = "Message send error";
      break;
    }
    case GOE_k_ERR_WRONG_PARAM:
    {
      pc_ret = "Read/Write parameter error";
      break;
    }
    case GOE_k_ERR_SPEED:
    {
      pc_ret = "Unable to get link speed";
      break;
    }
    case GOE_k_ERR_MCAST:
    {
      pc_ret ="Eth. interface is disabled for multicast";
      break;
    }
    case GOE_k_ERR_LAT:
    {
      pc_ret = "Error reading timestamp latency file";
      break;
    }
    case GOE_k_ERR_INIT_0:
    case GOE_k_ERR_INIT_1:
    case GOE_k_ERR_INIT_2:
    case GOE_k_ERR_INIT_3:
    case GOE_k_ERR_INIT_4:
    case GOE_k_ERR_INIT_5:
    case GOE_k_ERR_INIT_6:
    case GOE_k_ERR_INIT_7:
    case GOE_k_ERR_INIT_8:
    case GOE_k_ERR_INIT_9:
    case GOE_k_ERR_INIT_10:
    {
      PTP_SPRINTF((ac_str,"Initialization error %li",(long int)(dw_errNmb-GOE_k_ERR_INIT_0)));
      pc_ret = ac_str;
      break;
    }
    case GOE_k_ERR_NETIF_INIT_0:
    case GOE_k_ERR_NETIF_INIT_1:
    case GOE_k_ERR_NETIF_INIT_2:
    case GOE_k_ERR_NETIF_INIT_3:
    case GOE_k_ERR_NETIF_INIT_4:
    case GOE_k_ERR_NETIF_INIT_5:
    case GOE_k_ERR_NETIF_INIT_6:
    case GOE_k_ERR_NETIF_INIT_7:
    case GOE_k_ERR_NETIF_INIT_8:
    case GOE_k_ERR_NETIF_INIT_9:
    case GOE_k_ERR_NETIF_INIT_10:
    case GOE_k_ERR_NETIF_INIT_11:
    case GOE_k_ERR_NETIF_INIT_12:
    case GOE_k_ERR_NETIF_INIT_13:
    case GOE_k_ERR_NETIF_INIT_14:
    case GOE_k_ERR_NETIF_INIT_15:
    case GOE_k_ERR_NETIF_INIT_16:
    case GOE_k_ERR_NETIF_INIT_17:
    case GOE_k_ERR_NETIF_INIT_18:
    case GOE_k_ERR_NETIF_INIT_19:
    case GOE_k_ERR_NETIF_INIT_20: 
    case GOE_k_ERR_NETIF_INIT_21:   
    case GOE_k_ERR_NETIF_INIT_22:   
    case GOE_k_ERR_NETIF_INIT_23:   
    case GOE_k_ERR_NETIF_INIT_24:  
    case GOE_k_ERR_NETIF_INIT_25:  
    case GOE_k_ERR_NETIF_INIT_26: 
    case GOE_k_ERR_NETIF_INIT_27: 
    case GOE_k_ERR_NETIF_INIT_28: 
    case GOE_k_ERR_NETIF_INIT_29:
    {
      PTP_SPRINTF((ac_str,
                   "Channel initialization error %li",
                   (long int)(dw_errNmb-GOE_k_ERR_NETIF_INIT_0)));
      pc_ret = ac_str;
      break;
    }
    default:
    {
      pc_ret = "unknown error";
      break;
    }
  }
  return pc_ret;
}
#endif

/*************************************************************************
**    static functions
*************************************************************************/

/*************************************************************************
**
** Function    : SetError
**
** Description : This is the error function for the unit GOE.
**               It calls the error function of the stack.
**               This functions is registered in the function 
**               GOE_Init
**
** See Also    : GOE_Init()
**
** Parameters  : dw_err    (IN) - internal GOE error number
**               e_sevCode (IN) - severity code
**
** Returnvalue : -
**
** Remarks     : Implementation is CPU/compiler specific.
**
*************************************************************************/
static void SetError(UINT32 dw_err,PTP_t_sevCdEnum e_sevCode )
{
  /* if error callback function initialized, call */
  if( GOE_pf_errClbk != NULL )
  {
    GOE_pf_errClbk(k_GOE_ERR_ID,dw_err,e_sevCode);
  }  
}

#if( k_STORE_ASCII == TRUE )
/***********************************************************************
**  
** Function    : WriteAscii
**  
** Description : Writes an ascii file (human-readable) into the 
**               non-volatile storage to recover settings and configurations
**               after restart/reboot.
**
**               //Remark//: It is up to the porting engineer to realize
**               the non-volatile storage.
**  
** Parameters  : pc_fName    (IN) - file name
**               pc_file     (IN) - ascii file to write
**               dw_fileSize (IN) - file size
**               
** Returnvalue : TRUE             - function succeeded
**               FALSE            - function failed
** 
** Remarks     : -
**  
***********************************************************************/
static BOOLEAN WriteAscii(const CHAR  *pc_fName,
                          const CHAR  *pc_fileAscii,
                          UINT32      dw_fileSize)
{
  FILE    *pf_romFile;
  BOOLEAN o_ret = FALSE;
  CHAR    ac_fileName[k_MAX_FILE_NAME];
  INT32   l_written;

  /* copy string name */
  strncpy(ac_fileName,pc_fName,k_MAX_FILE_NAME);
  strncat(ac_fileName,".txt",k_MAX_FILE_NAME);
   
  /* open file, delete content or create it */
  pf_romFile = fopen(ac_fileName,"wt");
  
  /* does file exist now ? */
  if( pf_romFile == NULL )
  {
    o_ret = FALSE;
  }
  else
  {
    /* print in file */
    l_written = fprintf( pf_romFile,"%s\n",pc_fileAscii);
    /* check, if written size is correct */
    if( l_written < (INT32)dw_fileSize )
    {
      o_ret = FALSE;
    }
    else
    {
      o_ret = TRUE;
    }
    /* flush the file stream */
    if( fflush( pf_romFile ) != 0 )
    {
      o_ret &= FALSE;
    }
    else
    {
      o_ret &= TRUE;
    }

    /* close file */
    fclose(pf_romFile);
  }
  return o_ret;  
}

/***********************************************************************
**  
** Function    : ReadAscii
**  
** Description : Reads an ascii file (human-readable) out of the 
**               non-volatile storage to recover settings and configurations
**               after restart/reboot.
**
**               //Remark//: It is up to the porting engineer to realize
**               the non-volatile storage.
**  
** Parameters  : pc_fName     (IN) - file name of file to read
**               pb_data     (OUT) - buffer, contains read data
**               pdw_len  (IN/OUT) - IN: buffer size / OUT: data size of file
**               
** Returnvalue : TRUE              - function succeeded
**               FALSE             - function failed
** 
** Remarks     : -
**  
***********************************************************************/
static BOOLEAN ReadAscii(const CHAR *pc_fName,
                         UINT8 *pb_data,
                         UINT32 *pdw_len)
{
  BOOLEAN o_ret;
  FILE    *pf_romFile;
  size_t  s_fileSize;
  CHAR    ac_fileName[k_MAX_FILE_NAME];

  /* copy string name */
  strncpy(ac_fileName,pc_fName,k_MAX_FILE_NAME);
  strncat(ac_fileName,".txt",k_MAX_FILE_NAME);
  /* try to open file in ROM */
  pf_romFile = fopen(ac_fileName,"rt");
 
  /* does file exist ? */
  if( pf_romFile == NULL )
  {
    /* no data available */
    o_ret = FALSE;
  }
  else
  {
    /* copy file into buffer */
    s_fileSize = fread(pb_data,1,*pdw_len,pf_romFile);
    /* copy read bytes count to return value */
    *pdw_len = (UINT32)s_fileSize;
    /* close file */
    fclose(pf_romFile);
    /* return success */
    o_ret = TRUE;
  }
  return o_ret;
}

#else /* #if( k_STORE_ASCII == TRUE ) */
/***********************************************************************
**  
** Function    : WriteBinary
**  
** Description : Writes ab binary file into the non-volatile storage
**               to recover settings and configurations
**               after restart/reboot.
**
**               //Remark//: It is up to the porting engineer to realize
**               the non-volatile storage.
**  
** Parameters  : pc_fName       (IN) - file name
**               pb_fileBin     (IN) - the binary file to write
**               dw_fileSizeBin (IN) - file size in bytes
**               
** Returnvalue : TRUE                - function succeeded
**               FALSE               - function failed
** 
** Remarks     : -
**  
***********************************************************************/
static BOOLEAN WriteBinary(const CHAR  *pc_fName, 
                           const UINT8 *pb_fileBin,
                           UINT32       dw_fileSizeBin)
{
  FILE    *pf_romFile;
  BOOLEAN o_ret = FALSE;
  CHAR    ac_fileName[k_MAX_FILE_NAME];
  UINT32  dw_written;

  /* copy string name */
  strncpy(ac_fileName,pc_fName,k_MAX_FILE_NAME);
  strncat(ac_fileName,".bin",k_MAX_FILE_NAME);
   
  /* open file, delete content or create it */
  pf_romFile = fopen(ac_fileName,"wb");
  
  /* does file exist now ? */
  if( pf_romFile == NULL )
  {
    o_ret = FALSE;
  }
  else
  {    
    /* print in file */
    dw_written = (UINT32)fwrite(pb_fileBin,1,dw_fileSizeBin,pf_romFile);
    /* check, if written size of file is correct */
    if( dw_written < dw_fileSizeBin )
    {
      o_ret = FALSE;
    }
    else
    {
      o_ret = TRUE;
    }
    /* flush the file stream */
    if( fflush( pf_romFile ) != 0 )
    {
      o_ret &= FALSE;
    }
    else
    {
      o_ret &= TRUE;
    }

    /* close file */
    fclose(pf_romFile);
  }
  return o_ret;  
}

/***********************************************************************
**  
** Function    : ReadBinary
**  
** Description : Reads an binary file out of the 
**               non-volatile storage to recover settings and configurations
**               after restart/reboot.
**
**               //Remark//: It is up to the porting engineer to realize
**               the non-volatile storage.
**  
** Parameters  : pc_fName     (IN) - file name of file to read
**               pb_data     (OUT) - buffer, contains read data
**               pdw_len  (IN/OUT) - IN: buffer size / OUT: data size of file
**               
** Returnvalue : TRUE              - function succeeded
**               FALSE             - function failed
** 
** Remarks     : -
**  
***********************************************************************/
static BOOLEAN ReadBinary(const CHAR *pc_fName,
                          UINT8 *pb_data,
                          UINT32 *pdw_len)
{
  BOOLEAN o_ret;
  FILE    *pf_romFile;
  size_t  s_fileSize;
  CHAR    ac_fileName[k_MAX_FILE_NAME];

  /* copy string name */
  strncpy(ac_fileName,pc_fName,k_MAX_FILE_NAME);
  strncat(ac_fileName,".bin",k_MAX_FILE_NAME);
  
  /* try to open file in ROM */
  pf_romFile = fopen(ac_fileName,"rb");
 
  /* does file exist ? */
  if( pf_romFile == NULL )
  {
    /* no data available */
    o_ret = FALSE;
  }
  else
  {
    /* copy file into buffer */
    s_fileSize = fread(pb_data,1,*pdw_len,pf_romFile);
    /* copy read bytes count to return value */
    *pdw_len = (UINT32)s_fileSize;
    /* close file */
    fclose(pf_romFile);
    /* return success */
    o_ret = TRUE;
  }
  return o_ret;
}
#endif /* #if( k_STORE_ASCII == TRUE ) */

#if 0
/*************************************************************************
**
** Function    : ReadSectInt
**
** Description : Reads an integer value behind a section key in the config data
**               file.
**
** Parameters  : ppc_inp    (IN/OUT) - pointer to actual string, is moved  
**               pc_sectKey (IN)     - key to search for
**               pll_val     (OUT)   - value in key
**
** Returnvalue : TRUE                - function succeeded
**               FALSE               - function failed
**               
** Remarks     : -
**
*************************************************************************/
static BOOLEAN ReadSectInt(CHAR** ppc_inp,const CHAR* pc_sectKey,INT64* pll_val)
{
  CHAR    *pc_loc;
  CHAR    *pc_eol;
  size_t  s_len;
  BOOLEAN o_ret = FALSE;

  /* search for location of section key in input string*/
  pc_loc = strstr(*ppc_inp,pc_sectKey); 
  if( pc_loc != NULL )
  {
    /* get length of section key string to read behind */
    s_len = strlen(pc_sectKey);
    /* read behind section key string */
    if( sscanf(&pc_loc[s_len],"%lli",pll_val) == 1 )
    {
      /* search for end of line */
      pc_eol = strstr(&pc_loc[s_len],"\n");
      /* move string pointer behind read string */
      if( pc_eol != NULL )
      {
        *ppc_inp = pc_eol;
        o_ret = TRUE;
      }
      else
      {
        o_ret = FALSE;
      }
    }
    else
    {
      o_ret = FALSE;
    }
  }
  else
  {
    o_ret = FALSE;
  }
  return o_ret;
}
#endif /* #if 0 */
