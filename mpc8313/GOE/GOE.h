/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: GOE.h
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
**             GOE_Init
**             GOE_Close
**             GOE_GetSockHdl
**             GOE_getInbLat
**             GOE_getOutbLat
**             GOE_InitMcConn
**             GOE_CloseMcConn
**             GOE_InitUcConn
**             GOE_CloseUcConn
**             GOE_GetIfAddrs
**             GOE_SendMsg
**             GOE_RecvMsg
**             GOE_ReadFile
**             GOE_WriteFile
**             GOE_GetErrStr
**   
**   Compiler: gcc 3.3.2
**    Remarks: 
**
**************************************************************************
**    all rights reserved
*************************************************************************/
#ifndef __GOE_H__
#define __GOE_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/


/* error definitions  */
#define GOE_k_NO_ERR            (0u)  /* no error occured */
#define GOE_k_ERR_SPEED         (1u)  /* error with getting the ethernet 
                                         link speed */
#define GOE_k_ERR_MCAST         (2u)  /* eth interface with multicast 
                                         disabled */
#define GOE_k_ERR_SEND          (3u)  /* message could not be sent */
#define GOE_k_ERR_LAT           (4u)  /* error reading timestamp latency file */
#define GOE_k_ERR_WRONG_PARAM   (5u)  /* Wrong parameter for read/write 
                                         functions */
#define GOE_k_ERR_NETIF_INIT_0  (19u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_1  (20u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_2  (21u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_3  (22u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_4  (23u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_5  (24u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_6  (25u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_7  (26u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_8  (27u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_9  (28u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_10 (29u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_11 (30u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_12 (31u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_13 (32u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_14 (33u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_15 (34u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_16 (35u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_17 (36u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_18 (37u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_19 (38u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_20 (39u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_21 (40u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_22 (41u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_23 (42u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_24 (43u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_25 (44u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_26 (45u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_27 (46u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_28 (47u) /* Communication initialization error */
#define GOE_k_ERR_NETIF_INIT_29 (48u) /* Communication initialization error */

#define GOE_k_ERR_INIT_0        (50u) /* initialization error */
#define GOE_k_ERR_INIT_1        (51u) /* initialization error */
#define GOE_k_ERR_INIT_2        (52u) /* initialization error */
#define GOE_k_ERR_INIT_3        (53u) /* initialization error */
#define GOE_k_ERR_INIT_4        (54u) /* initialization error */
#define GOE_k_ERR_INIT_5        (55u) /* initialization error */
#define GOE_k_ERR_INIT_6        (56u) /* initialization error */
#define GOE_k_ERR_INIT_7        (57u) /* initialization error */
#define GOE_k_ERR_INIT_8        (58u) /* initialization error */
#define GOE_k_ERR_INIT_9        (59u) /* initialization error */
#define GOE_k_ERR_INIT_10       (60u) /* initialization error */
#define GOE_k_ERR_INIT_11       (61u) /* initialization error */

/* Multicast IP-Addresses for PTP */
#define k_MC_IPADDR           (inet_addr("224.0.1.129"))
#define k_MC_IPADDR_PDEL      (inet_addr("224.0.0.107"))
/* Ports */
#define k_EVNT_PORT       (319)
#define k_GNRL_PORT       (320)

/* size of PHY array */
#define k_PHY_ADDR_LEN   (6u)
/* size of IP-addr-len */
#define k_NETW_ADDR_LEN  (4u) 

/*************************************************************************
**    data types
*************************************************************************/
/************************************************************************
** SOCKET
               This type defines a socket handle.
**/
typedef INT32 SOCKET;

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
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
UINT16 GOE_ntoh16(const UINT8 *pb_src);

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
UINT32 GOE_ntoh32(const UINT8 *pb_src);

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
UINT64 GOE_ntoh48(const UINT8 *pb_src);

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
UINT64 GOE_ntoh64(const UINT8 *pb_src);

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
void GOE_hton16(UINT8 *pb_dst,UINT16 w_src);

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
void GOE_hton32(UINT8 *pb_dst,UINT32 dw_src);

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
void GOE_hton48(UINT8 *pb_dst,const UINT64 *pu48_src);

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
void GOE_hton64(UINT8 *pb_dst,const UINT64 *pddw_src);

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
                         const UINT8 *pb_netAddr,UINT32 dw_addrLen);

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
                         const CHAR *pc_netAddrStr);

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
void* GOE_LockStack( void );

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
void GOE_UnLockStack(void* pv_sem);

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
UINT32 GOE_Init(t_PTP_ERR_CLBK_FUNC pf_errClbk, PTP_t_PortId  *ps_portId);

/*************************************************************************
**
** Function    : GOE_Close
**
** Description : The function {GOE_Close()} de-initializes the unit
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
void GOE_Close( void );

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
SOCKET GOE_GetSockHdl( UINT8 b_port,CHAR *pc_ifName );

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
void GOE_getInbLat(UINT16 w_ifIdx, PTP_t_TmIntv *ps_tiInbLat);

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
void GOE_getOutbLat(UINT16 w_ifIdx, PTP_t_TmIntv *ps_tiOutbLat);

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
BOOLEAN GOE_InitMcConn(UINT16 w_ifIdx);

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
void GOE_CloseMcConn(UINT16 w_ifIdx);

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
BOOLEAN GOE_InitUcConn( void );

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
void GOE_CloseUcConn( void );

#endif /* #if( k_UNICAST_CPBL == TRUE ) */

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
                       UINT16 w_ifIdx);

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
                          UINT16   w_len);

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
                     PTP_t_PortAddr *ps_pAdr);
                     
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
                     UINT8      *pb_mode);

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
                      UINT32       dw_fileSizeBin);

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
INT16 GOE_GetNetIfNum(UINT16 w_ifIdx);

/*************************************************************************
**
** Function    : GOE_GetErrStr
**
** Description : This function resolves the error number to an
**               according error string.
**
** Parameters  : dw_errNmb (IN) - error number
**
** Returnvalue : CHAR*          - pointer to corresponding error string
**
** Remarks     : Gets compiled with definition of ERR_STR.
**  
***********************************************************************/
const CHAR* GOE_GetErrStr(UINT32 dw_errNmb);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __GOE_H__ */

