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

FILE NAME    : sc_network_if.c

AUTHOR       : Ken Ho

DESCRIPTION  :

The functions in this file should be edited by the customer to allow packet
flow from PTP stack to the interface port.

        SC_InitNetIf()
        SC_CloseNetIf()
        SC_GetSockHdl()
        SC_GetIfAddrs()
        SC_getInbLat()
        SC_getOutbLat()
        SC_TxPacket()
        SC_RxPacket()

Revision control header:
$Id: network_if/sc_network_if.c 1.28 2011/10/04 11:13:23PDT Kenneth Ho (kho) Exp  $

******************************************************************************
*/

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <linux/types.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <errno.h>
#include "sc_types.h"
#include "sc_api.h"
#include "sc_chan.h"
#include "sc_network_if.h"
#include "chan_processing_if/sc_SSM_handling.h"
#include "local_debug.h"
#include "config_if/sc_readconfig.h"


#define ETH_PERIOD  "eth1."
#define ETH         "eth1"
/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/

#if( k_CLK_DEL_P2P == TRUE)
#undef k_CLK_DEL_P2P
#endif

#define k_PHY_ADDR_LEN    (6u)

/* Multicast IP-Addresses for PTP */
#define k_MC_IPADDR       (inet_addr("224.0.1.129"))
#define k_MC_IPADDR_PDEL  (inet_addr("224.0.0.107"))
/* Ports */
#define k_EVNT_PORT       (319)
#define k_GNRL_PORT       (320)

#define k_MAX_IFNAME_LEN  (10)

/* link speed defines */
#define SC_k_SPD_10BT     (0) /*   10BaseT */
#define SC_k_SPD_100BT    (1) /*  100BaseT */
#define SC_k_SPD_1000BT   (2) /* 1000BaseT */
#define SC_k_SPD_UNKWN    (3) /* Speed unknown */

#define INVALID_SOCKET    ((int)-1)

/* inbound latency in nanoseconds to subtract from the rx timestamp for 10BT  */
#define k_CLK_INB_LAT_10BT    450
/* outbound latency in nanoseconds to add to the tx timestamp for 10BT   */
#define k_CLK_OUTB_LAT_10BT   390
/* inbound latency in nanoseconds to subtract from the rx timestamp for 100BT */
#define k_CLK_INB_LAT_100BT   370
/* outbound latency in nanoseconds to add to the tx timestamp for 100BT  */
#define k_CLK_OUTB_LAT_100BT  320
/* inbound latency in nanoseconds to subtract from the rx timestamp for 1000BT*/
#define k_CLK_INB_LAT_1000BT  270
/* outbound latency in nanoseconds to add to the tx timestamp for 1000BT */
#define k_CLK_OUTB_LAT_1000BT  70

/* length of the frame length on each packet, expected frame length of Sync-E ESMC 
   message is much smaller */
#define FRM_LEN_REQUEST       1500

/* Number of Ethernet ports */
#define MAX_ETH_CHANNELS      2

/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/
typedef INT32 SOCKET;

/*
----------------------------------------------------------------------------
Description:
This struct holds the socket address structures for multicast connections
----------------------------------------------------------------------------
*/
typedef struct
{
  /* multicast registration */
  struct ip_mreq     s_ipMr;
  /* addresses to send to */
  struct sockaddr_in s_addrEvnt;
  struct sockaddr_in s_addrGnrl;      
} t_NetConnMc;

/*
----------------------------------------------------------------------------
Description:
This struct holds the socket address structures for unicast connections
----------------------------------------------------------------------------
*/
typedef struct
{
  /* addresses to send to */
  struct sockaddr_in s_addrEvnt;
  struct sockaddr_in s_addrGnrl;
} t_NetConnUc;

/*
----------------------------------------------------------------------------
Description:
This struct holds the socket interfaces for the Sync-E ports
----------------------------------------------------------------------------
*/
typedef struct 
{
   int sd_max;                                  /* maximum socket descriptor value in array */
	int sd_enabled[MAX_ETH_CHANNELS];           /* enabled flag */
	int sd_iface[MAX_ETH_CHANNELS];	            /* socket descriptors */
   UINT8 b_ChanIndex[MAX_ETH_CHANNELS];        /* channel index associated with Sync-E port */
} t_SePortIf;

/*
----------------------------------------------------------------------------
Description:
This struct defines a handle an ethernet communication interface for
multicast operation and unicast operation.  Every network interface needs
one handle.
----------------------------------------------------------------------------
*/
typedef struct
{
  UINT16          aw_ptpPortNum[k_NUM_IF]; 
  UINT32          adw_OwnIpAddr[k_NUM_IF]; 
  UINT8           aab_macAddr[k_NUM_IF][k_PHY_ADDR_LEN];
  struct ifreq    as_ifReq[k_NUM_IF];
  /* connection sockets for event and general messages */
  SOCKET          as_sktEvt[k_NUM_IF];
  SOCKET          as_sktGnrl[k_NUM_IF];
  /* multicast addresses for sync, follow up, delay request, 
     delay response and management messages */ 
  t_NetConnMc     as_connMc[k_NUM_IF];
#if( k_CLK_DEL_P2P == TRUE )
  /* multicast adresses for peer delay request, 
     peer delay response peer delay response follow up messages */ 
  t_NetConnMc     as_connMcPdel[k_NUM_IF];
#endif
/* send connection for unicast messages */
  SOCKET          s_sktUcSendEvt;
  SOCKET          s_sktUcSendGnrl;
  t_NetConnUc     s_connUc;  
} t_NetifHdl;

/*
----------------------------------------------------------------------------
Description:
This struct defines the structure of the timestamp latency file in ROM.
----------------------------------------------------------------------------
*/
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
} t_cfgLat;


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
static INT64 all_tiInbLat[4]  = { k_CLK_INB_LAT_10BT,    k_CLK_INB_LAT_100BT,
                                  k_CLK_INB_LAT_1000BT,  k_CLK_INB_LAT_100BT };
static INT64 all_tiOutbLat[4] = { k_CLK_OUTB_LAT_10BT,   k_CLK_OUTB_LAT_100BT,
                                  k_CLK_OUTB_LAT_1000BT, k_CLK_OUTB_LAT_100BT };

/* Handle to the net interfaces */
static t_NetifHdl s_NetHdl;
static BOOLEAN o_netHdlInitDone = FALSE;
static UINT16 dw_fndIntf = 0;
static BOOLEAN o_initUcConnDone = FALSE;
static BOOLEAN o_vlanMode = FALSE;

int gRcvEvtCount=0;
int gRcvGnlCount=0;
int gSndEvtCount=0;
int gSndGnlCount=0;

static UINT8 multi_mac_addrs[6]	= { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x02 };
static UINT32 crc_table[]        = { 0x4DBDF21C, 0x500AE278, 0x76D3D2D4, 0x6B64C2B0,
									          0x3B61B38C, 0x26D6A3E8, 0x000F9344, 0x1DB88320,
									          0xA005713C, 0xBDB26158, 0x9B6B51F4, 0x86DC4190,
									          0xD6D930AC, 0xCB6E20C8, 0xEDB71064, 0xF0000000 };

static t_SePortIf port_itf;

static BOOLEAN se1_los = TRUE;
static BOOLEAN se2_los = TRUE;

/*--------------------------------------------------------------------------*/
/*                              function declarations                       */
/*--------------------------------------------------------------------------*/

static int InitMcConn(UINT16 w_ifIndex);
static void CloseMcConn(UINT16 w_ifIndex);
static int SendMulticast(UINT16 w_ifIndex,
                         t_msgGroupEnum e_msgGroup,
                         const UINT8 *pb_txBuf,
                         UINT16 w_len);
static int InitUcConn(void);
static void CloseUcConn(void);
static int SendUnicast(const t_PortAddr *ps_pAddr,
                       t_msgGroupEnum e_msgGroup,
                       const UINT8 *pb_txBuf,
                       UINT16 w_len);
static UINT8 GetLinkSpeed(UINT16 w_ifIndex);

static void CleanupSePorts (void);
static unsigned int CalcSeFCS(char * data, int length);
static void processPacket(UINT8 b_ChanIndex, char * pbuf);

#if 0 /* commented out for now */
static int GetLinkStatus(UINT16 w_ifIndex);
#endif

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

/*
----------------------------------------------------------------------------
                                SC_GetLinkSpeed()

Description:
This function the link speed of the specified ethernet port

Parameters:

Inputs
        portnum - number of ethernet port 

Outputs
        speed - current link speed
        
Return value:
        0 success
       -1 failure

-----------------------------------------------------------------------------
*/
int SC_GetLinkSpeed(int portnum, int* speed)
{
    struct ifreq ifr;
    int skfd = -1;
    struct ethtool_cmd etool;

    // Open a socket
    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return -1;

    // Get interface settings
    sprintf(ifr.ifr_name, "eth%d", portnum);
    etool.cmd = ETHTOOL_GSET;
    ifr.ifr_data = (char*)&etool;
    if (ioctl(skfd, SIOCETHTOOL, &ifr) < 0)
    {
        close(skfd);
        return -1;
    }

    switch (etool.speed)
    {
        case SPEED_1000:
            *speed = LINK_SPEED_1000;
            break;
        case SPEED_100:
            *speed = LINK_SPEED_100;
            break;
        case SPEED_10:
            *speed = LINK_SPEED_10;
            break;
        default:
            *speed = LINK_SPEED_NONE;
            break;
    }

    /* close socket */
    close(skfd);
    
    return 0;
}

/*
----------------------------------------------------------------------------
                                SC_GetLinkStat()

Description:
This function gets the link status of the specified ethernet port

Parameters:

Inputs
        portnum - number of ethernet port 

Outputs
        stat - current link status
        
Return value:
        0 success
       -1 failure

-----------------------------------------------------------------------------
*/
int SC_GetLinkStat(int portnum, int* stat)
{
    struct ifreq ifr;
    int skfd = -1;
    struct ethtool_value edata;

    // Open a socket
    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return -1;

    // Get interface link status
    sprintf(ifr.ifr_name, "eth%d", portnum);
    edata.cmd = ETHTOOL_GLINK;
    ifr.ifr_data = (char*)&edata;
    if (ioctl(skfd, SIOCETHTOOL, &ifr) < 0)
    {
        close(skfd);
        return -1;
    }

    /* close socket */
    close(skfd);

    *stat = (edata.data ? LINK : NOLINK);

    return 0;
}

/*
----------------------------------------------------------------------------
                                SC_InitNetIf()

Description:
This function initializes network interfaces. The intialization includes
creating socket, identifying clock ID based on MAC address, setting up
interface latency, etc.

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
int SC_InitNetIf(UINT16 w_ptpPortNum)
{
  UINT16 i;
  INT32  j=-1, if_idx;
  struct ifconf s_ifconf;
  struct ifreq as_devList[255];
  BOOLEAN o_found;
  SOCKET  s_skt;

  /* if all the interface has been used, simply return */
  if( dw_fndIntf >= k_NUM_IF )
  {
    return -1;
  }

  /* initialize net handle structure */
  if( !o_netHdlInitDone )
  {
    memset(&s_NetHdl, 0, sizeof(s_NetHdl));
    o_netHdlInitDone = TRUE;
  }

  /* if the port has been initialized, simply return */
  for( i = 0; i < dw_fndIntf; i++ )
  {
    if( s_NetHdl.aw_ptpPortNum[i] == w_ptpPortNum )
    {
      return i;
    }
  }

  /* set igmp rev to v2 */
  system("echo 2 > /proc/sys/net/ipv4/conf/all/force_igmp_version");

  /* initialize interfaces config list */
  s_ifconf.ifc_len = sizeof(as_devList);
  s_ifconf.ifc_req = as_devList;  
  memset(s_ifconf.ifc_buf,0,s_ifconf.ifc_len);
  /* try to open a socket handle to get access to the network interfaces */
  if( (s_skt = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == INVALID_SOCKET )
  {
    return -1; 
  }

  /* get list of network interfaces */
  if( ioctl(s_skt, SIOCGIFCONF, &s_ifconf) < 0 )
  {
    close(s_skt); 
    s_skt = INVALID_SOCKET;
    return -1; 
  }

  /**********************************************************************/
  /* Linux specific routines                                            */
  /**********************************************************************/
  /* init flag */
  o_found = FALSE;
  o_vlanMode = FALSE;
  /* search through interface list */
  while( j < s_ifconf.ifc_len )
  {
    BOOLEAN o_match = FALSE;
    UINT16 i;

    /* increment interface number for next search */
    j++;

    /* check if the interface has been used */
    for( i = 0; i < dw_fndIntf; i++ )
    {
      if( strcmp(as_devList[j].ifr_name,
                 s_NetHdl.as_ifReq[i].ifr_name) == 0)
      {
        o_match = TRUE;
        break;
      }
    }
    if( o_match )
      continue;

    /* get interface flags */
    if( ioctl(s_skt, SIOCGIFFLAGS, &as_devList[j]) < 0 )
    {
      continue;
    }
    /* check if this is an aliased interface - do not use */
    else if( strchr(as_devList[j].ifr_name,':') != NULL )
    {
      /* continue - aliased */
      continue;
    }
    /* ethernet interfaces must have default names eth1 */
    else if( strncmp(ETH,as_devList[j].ifr_name,4) == 0 )
    {
      if( (as_devList[j].ifr_flags & (IFF_MULTICAST|IFF_UP)) == 
                                     (IFF_MULTICAST|IFF_UP))
      {
        /* this is base interface */
        o_found = TRUE; 
        if_idx = j;
        /* check if this is a vlan */
        if( strncmp(ETH_PERIOD,as_devList[j].ifr_name,5) == 0 )
        {
          /* workaround: restart vlan interface for multicast mode */
          char cmd[128];
          sprintf(cmd, "netifcfg -d %s", as_devList[j].ifr_name);
          system(cmd);
          sprintf(cmd, "netifcfg -u %s", as_devList[j].ifr_name);
          system(cmd);
          o_vlanMode = TRUE; 
          break;
        }
      }
      continue;
    }
  }
  /* exit loop, if no interface found */
  if( !o_found && !o_vlanMode )
  {
    close(s_skt); 
    s_skt = INVALID_SOCKET;
    return -1; 
  }

  s_NetHdl.aw_ptpPortNum[dw_fndIntf] = w_ptpPortNum;
  /* store the interface name */
  strncpy(s_NetHdl.as_ifReq[dw_fndIntf].ifr_name,
          as_devList[if_idx].ifr_name,
          sizeof(as_devList[if_idx].ifr_name));        

  /* store the ip address of the interface in the network handle */
  s_NetHdl.adw_OwnIpAddr[dw_fndIntf] 
        = ((struct sockaddr_in *)&as_devList[if_idx].ifr_addr)->sin_addr.s_addr;
  /* get the mac address for use as the uuid */
  if( ioctl(s_skt, SIOCGIFHWADDR, &as_devList[if_idx]) < 0 )
  {
    /* close initial socket */
    close(s_skt); 
    s_skt = INVALID_SOCKET;
    return -1; 
  }
  close(s_skt); 
  s_skt = INVALID_SOCKET;
  /* store mac address for internal use */
  memcpy(s_NetHdl.aab_macAddr[dw_fndIntf],
         as_devList[if_idx].ifr_hwaddr.sa_data,
         k_PHY_ADDR_LEN);
  /* Open the event socket  */
  if( (s_NetHdl.as_sktEvt[dw_fndIntf]
       = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == INVALID_SOCKET )
  {
    return -1; 
  }
  /* set non-blocking mode for event port */
  if( fcntl(s_NetHdl.as_sktEvt[dw_fndIntf],
            F_SETFL,
            fcntl(s_NetHdl.as_sktEvt[dw_fndIntf],F_GETFL)|O_NONBLOCK) < 0 )
  {
    close(s_NetHdl.as_sktEvt[dw_fndIntf]);
    s_NetHdl.as_sktEvt[dw_fndIntf] = INVALID_SOCKET;
    return -1; 
  }
  /* Open the general socket  */  
  if( (s_NetHdl.as_sktGnrl[dw_fndIntf]
       = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == INVALID_SOCKET )
  {
    close(s_NetHdl.as_sktEvt[dw_fndIntf]);
    s_NetHdl.as_sktEvt[dw_fndIntf] = INVALID_SOCKET;
    return -1; 
  }
  /* set non-blocking mode for general port */
  if( fcntl(s_NetHdl.as_sktGnrl[dw_fndIntf],
            F_SETFL,
            fcntl(s_NetHdl.as_sktGnrl[dw_fndIntf],F_GETFL)|O_NONBLOCK) < 0 )
  {
    close(s_NetHdl.as_sktEvt[dw_fndIntf]);
    s_NetHdl.as_sktEvt[dw_fndIntf] = INVALID_SOCKET;
    close(s_NetHdl.as_sktGnrl[dw_fndIntf]);
    s_NetHdl.as_sktGnrl[dw_fndIntf] = INVALID_SOCKET;
    return -1; 
  }
  if( InitMcConn(dw_fndIntf) < 0 )
  {
    return -1;
  }
  if( !o_initUcConnDone )
  { 
    if( InitUcConn() < 0 )
    {
      return -1;
    }
    o_initUcConnDone = TRUE;
  }
  /* increment number of found interfaces */
  dw_fndIntf++;
  return (dw_fndIntf - 1);
}
 
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
void SC_CloseNetIf(UINT16 w_ifIndex)
{
  CloseUcConn();
  if( w_ifIndex < k_NUM_IF )
  {
    CloseMcConn(w_ifIndex);
  }
  dw_fndIntf = 0;
  o_netHdlInitDone = FALSE;
  o_initUcConnDone = FALSE;
}

/*
----------------------------------------------------------------------------
                                SC_GetSockHdl()

Description:
This function Returns ptp event socket handle (MPC8313 specific)

Parameters:

Inputs
        UINT8 b_port
        port number (0 or 1)

Outputs
        char* pb_ifName
        interface name

Return value:
        socket handle

-----------------------------------------------------------------------------
*/
SOCKET SC_GetSockHdl(UINT8 b_port, char *pc_ifName)
{
  strncpy(pc_ifName,s_NetHdl.as_ifReq[b_port].ifr_name, 4);
  return s_NetHdl.as_sktEvt[b_port];
}

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
int SC_GetIfAddrs(UINT16     w_ifIndex,
                  UINT8      *pb_phyAddr,
                  t_PortAddr *ps_portAddr)
{
  /* only if index within definition */
  if( w_ifIndex < k_NUM_IF )
  {
    memcpy(pb_phyAddr, s_NetHdl.aab_macAddr[w_ifIndex], k_PHY_ADDRESS_SIZE);
    ps_portAddr->e_netwProt = e_SC_UDP_IPv4;
    ps_portAddr->w_addrLen = 4;
    *((UINT32*)ps_portAddr->ab_addr) = s_NetHdl.adw_OwnIpAddr[w_ifIndex];
    return 0;
  }
  else
  {
    return -1;
  }
}


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
int SC_GetNetLatency(
	UINT16 w_ifIndex,
	INT64 *ps_tiInbLat,
	INT64 *ps_tiOutbLat
)
{
  UINT8 b_speed;
  /* correct parameter ? */
  if( w_ifIndex < k_NUM_IF )
  {
    /* get link speed */
    b_speed = GetLinkSpeed(w_ifIndex);
    /* get inbound latency out of the latency array */
    *ps_tiInbLat  = all_tiInbLat[b_speed];    
    *ps_tiOutbLat = all_tiOutbLat[b_speed];
    return 0;
  }
  else
  {
    return -1;
  }
}
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
int SC_TxPacket(UINT16           w_ifIndex,
                const t_PortAddr *ps_pAddr,
                t_msgGroupEnum   e_msgGroup,
                const UINT8      *pb_txBuf,
                UINT16           w_len)
{
  /* multicast or unicast message ? */
  if( ps_pAddr == NULL )
  {
    /* multicast */
    return SendMulticast(w_ifIndex, e_msgGroup, pb_txBuf, w_len);
  }
  else
  {
    /* unicast */
    return SendUnicast(ps_pAddr, e_msgGroup, pb_txBuf, w_len);
  }
}

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
int SC_RxPacket(UINT16         w_ifIndex,
                t_msgGroupEnum e_msgGroup,
                UINT8          *pb_rxBuf,
                INT16          *pi_bufSize,
                t_PortAddr     *ps_pAddr)
{
  struct sockaddr_in s_sockadr_snd;
  socklen_t          s_sockAdrSze=0;
  UINT16             w_bufSize;
  SOCKET             s_readSock;

  /* Search for selected socket */  
  if( w_ifIndex < k_NUM_IF)
  {
    /* get socket */
    switch(e_msgGroup)
    {
      case e_MSG_GRP_EVT:
      {
        s_readSock = s_NetHdl.as_sktEvt[w_ifIndex];
        break;
      }
      case e_MSG_GRP_GNRL:
      {
        s_readSock = s_NetHdl.as_sktGnrl[w_ifIndex];
        break;
      }
      default:
      {
        s_readSock = INVALID_SOCKET;
      }
    }
    /* get size of struct SOCKADDR_IN */ 
    s_sockAdrSze = (socklen_t)sizeof(struct sockaddr_in);
    /* store buffer size temporarly */
    w_bufSize = (UINT16)*pi_bufSize;
  
    /* IF read request on the eventport */ 
    if( s_readSock != INVALID_SOCKET )
    {
      /* read till a foreignmessage was received or socket queue is empty */
      while( 1 )
      {
        /* IF No error with receiving the Msg  */
        if((*pi_bufSize = recvfrom(s_readSock,
                                   (char*)pb_rxBuf,                   
                                   w_bufSize,
                                   0,
                                   (struct sockaddr*)&s_sockadr_snd,
                                   &s_sockAdrSze)) > 0) 
        {
          /* check, if it is from ourself */
          if( s_sockadr_snd.sin_addr.s_addr == 
                 s_NetHdl.adw_OwnIpAddr[w_ifIndex])
          {
            /* is from myself - try again */         
          }
          else
          {
            /* store port address */
            ps_pAddr->w_addrLen  = 4;
            ps_pAddr->e_netwProt = e_SC_UDP_IPv4;
            memcpy(ps_pAddr->ab_addr,&s_sockadr_snd.sin_addr.s_addr,4);
            
			if (e_msgGroup==e_MSG_GRP_EVT)
				gRcvEvtCount++;
			else if(e_msgGroup==e_MSG_GRP_GNRL)
				gRcvGnlCount++;;

            /* event message from another node or channel received */
            return *pi_bufSize;
          }
        }
        else
        {
          return -1;
        }
      }
    }
    else /* Wrong socket Type */
    {
      return -1;
    }
  }
  else /* Wrong interface number */
  {
    return -1;
  }
}

/*--------------------------------------------------------------------------*/
/*                         static functions                                 */
/*--------------------------------------------------------------------------*/

/*
----------------------------------------------------------------------------
                                InitMcConn()

Description:
This function initializes a multicast network communication interface for
PTP communication and is called for each Ethernet interface. Two sockets
per interface must be initializes to receive and transmit general and
event PTP messages. General messages are used combined with the UDP port
320 and event messages are used combined with the UDP port 319. For systems
with more than one physical interface (NIC) to be able to send and receive
multicast messages the sockets must be fixed to one appropriate NIC. The
IP routing table shall not be used, otherwise correct behaviour is not
guaranteed.

There are some socket options, which should be used:
    - set sockets to non blocking mode (SO_NBIO)
    - set sockets for reuse to be able to bind more sockets
      to the port (SO_REUSEADDR)
    - disable multicast loopback to avoid additional
      processor load (IP_MULTICAST_LOOP)
    - add multicast membership to be able to receive and
      transmit multicast messages via the IP address
      224.0.1.129 (IP_ADD_MEMBERSHIP)
    - add multicast membership to be able to receive and
      transmit multicast messages via the IP address
      224.0.0.107 (IP_ADD_MEMBERSHIP) - if P2P delay
      mechanism is used
    - set TTL for multicasts to 1 (IP_MULTICAST_TTL)
    - set TOS to the highest priority (IP_TOS) (only for event messages)

Remark:
This function must be adapted to the used communication media and TCP/IP
stack. Function and option names may differ depending on the used TCP/IP
stack!

Parameters:

Inputs
        UINT16 w_ifIndex
        This value defines the interface port.

Outputs:
        None

Return value:
         0: initialization succeeded
        -1: initialization failed

-----------------------------------------------------------------------------
*/
static int InitMcConn(UINT16 w_ifIndex)
{
  UINT32  dw_opt = 1,
          dw_ownIpAddr = 0;
  UINT8   b_val;
  struct sockaddr_in s_addrBindEvnt;
  struct sockaddr_in s_addrBindGnrl;

  dw_ownIpAddr = s_NetHdl.adw_OwnIpAddr[w_ifIndex];

  /* multicast connection for sync, follow up, 
     delay request, delay response and management messages */
  s_NetHdl.as_connMc[w_ifIndex].s_ipMr.imr_interface.s_addr = dw_ownIpAddr;
  s_NetHdl.as_connMc[w_ifIndex].s_ipMr.imr_multiaddr.s_addr = k_MC_IPADDR;
  /* socket address multicast event socket */
  s_NetHdl.as_connMc[w_ifIndex].s_addrEvnt.sin_addr.s_addr  = k_MC_IPADDR;
  s_NetHdl.as_connMc[w_ifIndex].s_addrEvnt.sin_port         = htons(k_EVNT_PORT);
  s_NetHdl.as_connMc[w_ifIndex].s_addrEvnt.sin_family       = AF_INET;

  /* socket address multicast general socket */
  s_NetHdl.as_connMc[w_ifIndex].s_addrGnrl.sin_addr.s_addr  = k_MC_IPADDR;
  s_NetHdl.as_connMc[w_ifIndex].s_addrGnrl.sin_port         = htons(k_GNRL_PORT);
  s_NetHdl.as_connMc[w_ifIndex].s_addrGnrl.sin_family       = AF_INET;

/* just for P2P devices */
#if( k_CLK_DEL_P2P == TRUE )
  /* multicast connection for peer delay request, 
     peer delay response and peer delay response follow up */
  s_NetHdl.as_connMcPdel[w_ifIndex].s_ipMr.imr_interface.s_addr = dw_ownIpAddr;
  s_NetHdl.as_connMcPdel[w_ifIndex].s_ipMr.imr_multiaddr.s_addr = k_MC_IPADDR_PDEL;
  /* socket address multicast event socket */
  s_NetHdl.as_connMcPdel[w_ifIndex].s_addrEvnt.sin_addr.s_addr  = k_MC_IPADDR_PDEL;
  s_NetHdl.as_connMcPdel[w_ifIndex].s_addrEvnt.sin_port         = htons(k_EVNT_PORT);
  s_NetHdl.as_connMcPdel[w_ifIndex].s_addrEvnt.sin_family       = AF_INET;

  /* socket address multicast general socket */
  s_NetHdl.as_connMcPdel[w_ifIndex].s_addrGnrl.sin_addr.s_addr  = k_MC_IPADDR_PDEL;
  s_NetHdl.as_connMcPdel[w_ifIndex].s_addrGnrl.sin_port         = htons(k_GNRL_PORT);
  s_NetHdl.as_connMcPdel[w_ifIndex].s_addrGnrl.sin_family       = AF_INET;
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */ 
 
  /* Set options to reuse the address, so we can bind multiple sockets to it  */
  /* The event port  */
  dw_opt = 1;
  if( setsockopt(s_NetHdl.as_sktEvt[w_ifIndex],
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 (char*)&dw_opt,
                 sizeof(dw_opt)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }
  /* general port */ 
  dw_opt = 1;
  if( setsockopt(s_NetHdl.as_sktGnrl[w_ifIndex],
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 (char*)&dw_opt,
                 sizeof(dw_opt)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }
  /* fix this event socket to one interface */
  if( setsockopt(s_NetHdl.as_sktEvt[w_ifIndex],
                 SOL_SOCKET,
                 SO_BINDTODEVICE,
                 (char*)&s_NetHdl.as_ifReq[w_ifIndex],
                 sizeof(struct ifreq)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;    
  }
  /* fix this general socket to one interface */
  if( setsockopt(s_NetHdl.as_sktGnrl[w_ifIndex],
                 SOL_SOCKET,
                 SO_BINDTODEVICE,
                 (char*)&s_NetHdl.as_ifReq[w_ifIndex],
                 sizeof(struct ifreq)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }

  /* bind event socket to any IP address to receive every 
     event message on the same socket */
  s_addrBindEvnt.sin_addr.s_addr  = INADDR_ANY;
  s_addrBindEvnt.sin_port         = htons(k_EVNT_PORT);
  s_addrBindEvnt.sin_family       = AF_INET;

  if( bind(s_NetHdl.as_sktEvt[w_ifIndex],
           (struct sockaddr*)&(s_addrBindEvnt),
           sizeof(struct sockaddr_in)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1 ; 
  }

  /* bind general socket to any IP address to receive 
     every general message on the same socket */
  s_addrBindGnrl.sin_addr.s_addr  = INADDR_ANY;
  s_addrBindGnrl.sin_port         = htons(k_GNRL_PORT);
  s_addrBindGnrl.sin_family       = AF_INET;

  if( bind(s_NetHdl.as_sktGnrl[w_ifIndex],
           (struct sockaddr*)&(s_addrBindGnrl),
           sizeof(struct sockaddr_in)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1; 
  }

  /* set option to be able to receive multicasts 
     -> for event port */ 
  if( setsockopt(s_NetHdl.as_sktEvt[w_ifIndex],
                 IPPROTO_IP, 
                 IP_ADD_MEMBERSHIP,
                 &s_NetHdl.as_connMc[w_ifIndex].s_ipMr, 
                 sizeof(struct ip_mreq)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }
  /* -> for general port */ 
  if( setsockopt(s_NetHdl.as_sktGnrl[w_ifIndex],
                 IPPROTO_IP, 
                 IP_ADD_MEMBERSHIP,
                 &s_NetHdl.as_connMc[w_ifIndex].s_ipMr, 
                 sizeof(struct ip_mreq)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }
/* just for P2P devices */
#if( k_CLK_DEL_P2P == TRUE )

   /* set option to be able to receive Pdelay multicasts 
     -> for event port */
  if( setsockopt(s_NetHdl.as_sktEvt[w_ifIndex],
                 IPPROTO_IP, 
                 IP_ADD_MEMBERSHIP,
                 &s_NetHdl.as_connMcPdel[w_ifIndex].s_ipMr, 
                 sizeof(struct ip_mreq)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }
  /* -> for general port */
  if( setsockopt(s_NetHdl.as_sktGnrl[w_ifIndex],
                 IPPROTO_IP, 
                 IP_ADD_MEMBERSHIP,
                 &s_NetHdl.as_connMcPdel[w_ifIndex].s_ipMr, 
                 sizeof(struct ip_mreq)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
  
  /* disable multicast loopback for event port */
  b_val = 0;
  if( setsockopt(s_NetHdl.as_sktEvt[w_ifIndex], 
                 IPPROTO_IP, 
                 IP_MULTICAST_LOOP, 
                 (char *)&b_val, 
                 sizeof(b_val)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }

  /* for general port */
  if( setsockopt(s_NetHdl.as_sktGnrl[w_ifIndex], 
                 IPPROTO_IP, 
                 IP_MULTICAST_LOOP, 
                 (char *)&b_val, 
                 sizeof(b_val)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }

  /* set TTL for multicasts to 8 */
  dw_opt = 8;
  if( setsockopt(s_NetHdl.as_sktEvt[w_ifIndex],
                 IPPROTO_IP,
                 IP_MULTICAST_TTL,
                 &dw_opt, 
                 sizeof(UINT32)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }
  if( setsockopt(s_NetHdl.as_sktGnrl[w_ifIndex],
                 IPPROTO_IP,
                 IP_MULTICAST_TTL,
                 &dw_opt,
                 sizeof(UINT32)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }
  /* set socket priority to highest to get the highest TOS value 
     (Differentiated Service class selector codepoint)*/
  b_val = 0xE0;
  if( setsockopt(s_NetHdl.as_sktEvt[w_ifIndex],
                 IPPROTO_IP,
                 IP_TOS,
                 &b_val,
                 sizeof(b_val)) < 0 )
  {
    CloseMcConn(w_ifIndex);
    return -1;
  }
  /* for vlan mode, set to promiscuous mode, otherwise TCPIP stack */
  /* cound not receive igmp query packet.                          */
  if( strncmp(ETH_PERIOD,s_NetHdl.as_ifReq[w_ifIndex].ifr_name,5) == 0 )
  {
    struct ifreq s_ifr;
    strncpy(s_ifr.ifr_name, s_NetHdl.as_ifReq[w_ifIndex].ifr_name,
            sizeof(s_ifr.ifr_name));
    if( ioctl(s_NetHdl.as_sktEvt[w_ifIndex], SIOCGIFFLAGS, &s_ifr) < 0 )
    {
      return -1;
    }
    s_ifr.ifr_flags |= IFF_PROMISC;
    if( ioctl(s_NetHdl.as_sktEvt[w_ifIndex], SIOCSIFFLAGS, &s_ifr) < 0 )
    {
      return -1;
    }
  }
  return 0;  
}

/*
----------------------------------------------------------------------------
                                CloseMcConn()

Description:
Closes a multicast network communication interface and all related resources.

Parameters:

Inputs
        UINT16 w_ifIndex
        This value defines the interface port.

Outputs:
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
static void CloseMcConn(UINT16 w_ifIndex)
{
  if( s_NetHdl.as_sktEvt[w_ifIndex] != INVALID_SOCKET )
  {
    /* drop from multicast membership - we don't
       need to receive messages from the address anymore */
    setsockopt(s_NetHdl.as_sktEvt[w_ifIndex],
               IPPROTO_IP,
               IP_DROP_MEMBERSHIP,
               (char*)&s_NetHdl.as_connMc[w_ifIndex].s_ipMr,
               sizeof(s_NetHdl.as_connMc[w_ifIndex].s_ipMr));

/* just for P2P devices */
#if( k_CLK_DEL_P2P == TRUE )
    setsockopt(s_NetHdl.as_sktEvt[w_ifIndex],
               IPPROTO_IP,
               IP_DROP_MEMBERSHIP,
               (char*)&s_NetHdl.as_connMcPdel[w_ifIndex].s_ipMr,
               sizeof(s_NetHdl.as_connMcPdel[w_ifIndex].s_ipMr));
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

    /* close the socket */ 
    close(s_NetHdl.as_sktEvt[w_ifIndex]);
    s_NetHdl.as_sktEvt[w_ifIndex] = INVALID_SOCKET;
  }
  if( s_NetHdl.as_sktGnrl[w_ifIndex] != INVALID_SOCKET )
  {
    /* drop from multicast membership - we don't
       need to receive messages from the address anymore */
    setsockopt(s_NetHdl.as_sktGnrl[w_ifIndex],
               IPPROTO_IP,
               IP_DROP_MEMBERSHIP,
               (char*)&s_NetHdl.as_connMc[w_ifIndex].s_ipMr,
               sizeof(s_NetHdl.as_connMc[w_ifIndex].s_ipMr));

/* just for P2P devices */
#if( k_CLK_DEL_P2P == TRUE )
    setsockopt(s_NetHdl.as_sktGnrl[w_ifIndex],
               IPPROTO_IP,
               IP_DROP_MEMBERSHIP,
               (char*)&s_NetHdl.as_connMcPdel[w_ifIndex].s_ipMr,
               sizeof(s_NetHdl.as_connMcPdel[w_ifIndex].s_ipMr));
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */

    /* close the socket */ 
    close(s_NetHdl.as_sktGnrl[w_ifIndex]);
    s_NetHdl.as_sktGnrl[w_ifIndex] = INVALID_SOCKET;
  }
  /* for vlan mode, restore to non-promiscuous mode */
  if( strncmp(ETH_PERIOD,s_NetHdl.as_ifReq[w_ifIndex].ifr_name,5) == 0 )
  {
    struct ifreq s_ifr;
    strncpy(s_ifr.ifr_name, s_NetHdl.as_ifReq[w_ifIndex].ifr_name,
            sizeof(s_ifr.ifr_name));
    if( ioctl(s_NetHdl.as_sktEvt[w_ifIndex], SIOCGIFFLAGS, &s_ifr) < 0 )
    {
      return;
    }
    s_ifr.ifr_flags &= ~IFF_PROMISC;
    ioctl(s_NetHdl.as_sktEvt[w_ifIndex], SIOCSIFFLAGS, &s_ifr);
  }
}

/*
----------------------------------------------------------------------------
                                InitUcConn()

Description:
This function initializes a unicast network communication interface for
PTP communication. Two sockets must be initialized to receive and transmit
general and event PTP messages. General messages are used combined with
the UDP port 320 and event messages are used combined with the UDP port 319.

There are some socket options, which should be used:
    - set sockets to non blocking mode (SO_NBIO)
    - set sockets for reuse to be able to bind more sockets
      to the port (SO_REUSEADDR)
    - set TOS to the highest priority (IP_TOS) (only for
      event messages)

Remark:
This function must be adapted to the used communication media and TCP/IP
stack. Function and option names may differ depending on the used TCP/IP
stack!

Parameters:

Inputs
        None

Outputs:
        None

Return value:
         0: initialization succeeded
        -1: initialization failed

-----------------------------------------------------------------------------
*/
static int InitUcConn(void)
{
  UINT32  dw_opt; 
  UINT8   b_val;

  /* socket address unicast event socket */
  s_NetHdl.s_connUc.s_addrEvnt.sin_addr.s_addr = INADDR_ANY;
  s_NetHdl.s_connUc.s_addrEvnt.sin_port        = htons(k_EVNT_PORT);
  s_NetHdl.s_connUc.s_addrEvnt.sin_family      = AF_INET;

  /* socket address unicast general socket */
  s_NetHdl.s_connUc.s_addrGnrl.sin_addr.s_addr = INADDR_ANY;
  s_NetHdl.s_connUc.s_addrGnrl.sin_port        = htons(k_GNRL_PORT);
  s_NetHdl.s_connUc.s_addrGnrl.sin_family      = AF_INET;

  /* Open a event socket for sending unicast event messages */
  if( (s_NetHdl.s_sktUcSendEvt
       = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == INVALID_SOCKET )
  {
    return -1; 
  }
  /* set non-blocking mode for unicast event socket */
  if( fcntl(s_NetHdl.s_sktUcSendEvt,
            F_SETFL,
            fcntl(s_NetHdl.s_sktUcSendEvt,F_GETFL)|O_NONBLOCK) < 0 )
  {
    close(s_NetHdl.s_sktUcSendEvt);
    s_NetHdl.s_sktUcSendEvt = INVALID_SOCKET;
    return -1;
  }
  /* Open a socket for sending unicast general messages */
  if( (s_NetHdl.s_sktUcSendGnrl
       = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == INVALID_SOCKET )
  {
    close(s_NetHdl.s_sktUcSendEvt);
    s_NetHdl.s_sktUcSendEvt = INVALID_SOCKET;
    return -1; 
  }
  /* set non-blocking mode for unicast general socket */
  if( fcntl(s_NetHdl.s_sktUcSendGnrl,
            F_SETFL,
            fcntl(s_NetHdl.s_sktUcSendGnrl,F_GETFL)|O_NONBLOCK) < 0 )
  {
    CloseUcConn();
    return -1;
  }

  /* Set options to reuse the address, so we can bind 
      two and more sockets to it
      event port  */
  dw_opt = 1;
  if( setsockopt(s_NetHdl.s_sktUcSendEvt,
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 (char*)&dw_opt,
                 sizeof(dw_opt)) < 0 )
  {
    CloseUcConn();
    return -1;    
  }
  /* general port */ 
  if( setsockopt(s_NetHdl.s_sktUcSendGnrl,
                 SOL_SOCKET,
                 SO_REUSEADDR,
                 (char*)&dw_opt,
                 sizeof(dw_opt)) < 0 )
  {
    CloseUcConn();
    return -1;  
  }
  /* bind event socket to any IP address to send every 
      event message on the same socket */
  if( bind(s_NetHdl.s_sktUcSendEvt,
           (struct sockaddr*)&(s_NetHdl.s_connUc.s_addrEvnt),
           sizeof(s_NetHdl.s_connUc.s_addrEvnt)) < 0 )
  {
    CloseUcConn();
    return -1;
  }

  /* bind general socket to any IP address to send 
    every general message on the same socket */
  if( bind(s_NetHdl.s_sktUcSendGnrl,
           (struct sockaddr*)&(s_NetHdl.s_connUc.s_addrGnrl),
           sizeof(s_NetHdl.s_connUc.s_addrGnrl)) < 0 )
  {
    CloseUcConn();
    return -1; 
  }
  /* set socket priority to highest to get the highest TOS value 
     (Differentiated Service class selector codepoint)*/
  b_val = 0xE0;
  if( setsockopt(s_NetHdl.s_sktUcSendEvt,
                 IPPROTO_IP,
                 IP_TOS,
                 &b_val,
                 sizeof(b_val)) < 0 )
  {
    CloseUcConn();
    return -1;
  }
  return 0;
}

/*
----------------------------------------------------------------------------
                                CloseUcConn()

Description:
Closes the unicast network communication interface and all related resources.

Parameters:

Inputs
        None

Outputs:
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
static void CloseUcConn(void)
{
  if( s_NetHdl.s_sktUcSendEvt != INVALID_SOCKET )
  {
    close(s_NetHdl.s_sktUcSendEvt); 
    s_NetHdl.s_sktUcSendEvt = INVALID_SOCKET;
  }
  if( s_NetHdl.s_sktUcSendGnrl != INVALID_SOCKET )
  {
    close(s_NetHdl.s_sktUcSendGnrl);   
    s_NetHdl.s_sktUcSendGnrl = INVALID_SOCKET;
  }
}

/*
----------------------------------------------------------------------------
                                SendMulticast()

Description:
Sends a multicast message over TCP/IP. The function returns immediately
and does not block. This function does NOT use the IP routing table to
select the appropriate network interface. The function itself must select
the NIC to be used (option IP_MULTICAST_IF).

Parameters:

Inputs
        UINT16 w_ifIndex
        This is the destination interface port.

        t_msgGroupEnum e_msgGroup
        This is the message type (event/general.)

        const UINT8 *pb_txBuf
        This is a pointer to the message buffer.

        UINT16 w_len
        This is the buffer length.

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
static int SendMulticast(UINT16         w_ifIndex,
                         t_msgGroupEnum e_msgGroup,
                         const UINT8    *pb_txBuf,
                         UINT16         w_len)
{
  SOCKET             s_sendSock = 0;
  struct sockaddr_in *ps_sendAddr = NULL;

  /* check selected socket */ 
  if( w_ifIndex < k_NUM_IF )
  {
    /* get socket */
    switch(e_msgGroup)
    {
      case e_MSG_GRP_EVT:
      {
        s_sendSock  = s_NetHdl.as_sktEvt[w_ifIndex];
        ps_sendAddr = &s_NetHdl.as_connMc[w_ifIndex].s_addrEvnt;
        gSndEvtCount++;
        break;
      }
      case e_MSG_GRP_GNRL:
      {
        s_sendSock  = s_NetHdl.as_sktGnrl[w_ifIndex];
        ps_sendAddr = &s_NetHdl.as_connMc[w_ifIndex].s_addrGnrl;
        gSndGnlCount++;
        break;
      }
/* just for P2P devices */
#if( k_CLK_DEL_P2P == TRUE )
      case e_MSG_GRP_EVT_PDEL:
      {
        s_sendSock  = s_NetHdl.as_sktEvt[w_ifIndex];
        ps_sendAddr = &s_NetHdl.as_connMcPdel[w_ifIndex].s_addrEvnt;
        gSndEvtCount++;
        break;
      }
      case e_MSG_GRP_GNRL_PDEL:
      {
        s_sendSock  = s_NetHdl.as_sktGnrl[w_ifIndex];
        ps_sendAddr = &s_NetHdl.as_connMcPdel[w_ifIndex].s_addrGnrl;
        gSndGnlCount++;
        break;
      }
#endif /* #if( k_CLK_DEL_P2P == TRUE ) */
      default:
      {
        s_sendSock = INVALID_SOCKET;
      }
    }
    /* select port  */
    if( s_sendSock != INVALID_SOCKET )
    {
      /* send packet */ 
      if(sendto(s_sendSock, 
                (char*)pb_txBuf,
                w_len,
                0,
                (struct sockaddr*)ps_sendAddr,
                (socklen_t)sizeof(*ps_sendAddr)) == w_len)
      {
        return w_len;
      }
      else
      {
        return -1;
      }
    }
    else /* unknown socket */ 
    {
      return -1;
    }
  }
  else /* w_ifIndex < k_NUM_IF */
  {
    return -1;
  }
}

/*
----------------------------------------------------------------------------
                                SendUnicast()

Description:
Sends a unicast message over TCP/IP. The function returns immediately and
does not block. This function uses the IP routing table to select the
appropriate network interface.

Parameters:

Inputs
        t_PortAddr *ps_pAddr
        This is a pointer to the destination port address.

        t_msgGroupEnum e_msgGroup
        This is the message type (event/general.)

        const UINT8 *pb_txBuf
        This is a pointer to the message buffer.

        UINT16 w_len
        This is the buffer length.

Outputs
        None

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
extern unsigned int print_signaling;
static int SendUnicast(const t_PortAddr *ps_pAddr,
                       t_msgGroupEnum   e_msgGroup,
                       const UINT8      *pb_txBuf,
                       UINT16           w_len)
{
  SOCKET             s_sendSock = 0;
  struct sockaddr_in *ps_sendAddr;

  /* get socket */
  switch(e_msgGroup)
  {
    case e_MSG_GRP_EVT:
    case e_MSG_GRP_EVT_PDEL:
    {
      s_sendSock  = s_NetHdl.s_sktUcSendEvt;
      ps_sendAddr = &s_NetHdl.s_connUc.s_addrEvnt;
      gSndEvtCount++;
      break;
    }
    case e_MSG_GRP_GNRL:
    case e_MSG_GRP_GNRL_PDEL:
    {
      s_sendSock  = s_NetHdl.s_sktUcSendGnrl;
      ps_sendAddr = &s_NetHdl.s_connUc.s_addrGnrl;
      gSndGnlCount++;
      break;
    }
    default:
    {
      s_sendSock  = INVALID_SOCKET;
      /* useless initialization - PC-lint likes that */
      ps_sendAddr = &s_NetHdl.s_connUc.s_addrGnrl;
    }
  }
  /* select port */
  if( s_sendSock != INVALID_SOCKET )
  {
    /* copy address to socket address struct */
    memcpy(&ps_sendAddr->sin_addr.s_addr,ps_pAddr->ab_addr,4);
    /* send packet */
    if(sendto(s_sendSock,
              (char*)pb_txBuf,
              w_len,
              0,
              (const struct sockaddr*)ps_sendAddr,
              sizeof(*ps_sendAddr))
              == (INT32)w_len)
    {
      return w_len;
    }
    else
    {
      return -1;
    }
  }
  else /* unknown socket */ 
  {
    return -1;
  }
}

/*
----------------------------------------------------------------------------
                                GetLinkSpeed()

Description:
Returns the link speed for a network interface.

Parameters:

Inputs
        UINT16 w_ifIndex
        This is the destination interface port.

Outputs
        None

Return value:
        SC_k_SPD_10BT   - link speed is   10 MBit
        SC_k_SPD_100BT  - link speed is  100 MBit
        SC_k_SPD_1000BT - link speed is 1000 MBit
        SC_k_SPD_UNKWN  - speed is unknown

-----------------------------------------------------------------------------
*/
static UINT8 GetLinkSpeed(UINT16 w_ifIndex)
{
  struct ethtool_cmd s_ecmd;
  UINT8  b_speed;

  /* get device settings */
  s_ecmd.cmd = ETHTOOL_GSET;
  s_NetHdl.as_ifReq[w_ifIndex].ifr_data = (caddr_t)&s_ecmd;
  /* get settings from the event send socket (equal settings as receive socket) */
  if(ioctl(s_NetHdl.as_sktEvt[w_ifIndex], SIOCETHTOOL,&s_NetHdl.as_ifReq[w_ifIndex]) < 0)
  {
    /* set error and return 100BaseT (default) */
    b_speed = SC_k_SPD_100BT;
  }
  else
  {          
    /* translate returned speed to internal defines */
    switch (s_ecmd.speed)
    {
      case SPEED_10:
      {
        b_speed = SC_k_SPD_10BT;
        break;
      }
      case SPEED_100:
      {
        b_speed = SC_k_SPD_100BT;
        break;
      }
      case SPEED_1000:
      {
        b_speed = SC_k_SPD_1000BT;
        break;
      }
      default:
      {
        /* set error and return 100BaseT (default) */
        b_speed = SC_k_SPD_100BT;
        break;
      }
    }
  }
  /* return link speed */
  return b_speed;
}

/*
----------------------------------------------------------------------------
                                GetLinkStatus()

Description:
Returns the link status for a network interface.

Parameters:

Inputs
        UINT16 w_ifIndex
        This is the destination interface port.

Outputs
        None

Return value:
         0: interface is connected
        -1: interface is not connected

-----------------------------------------------------------------------------
*/
#if 0 /* commented out for now */
static int GetLinkStatus(UINT16 w_ifIndex)
{
   struct ethtool_value s_edata;
   int o_ret;
 
  /* get link status information */
  s_edata.cmd = ETHTOOL_GLINK;
  s_NetHdl.as_ifReq[w_ifIndex].ifr_data = (caddr_t)&s_edata;
  /* call driver */
  if(ioctl(s_NetHdl.as_sktEvt[w_ifIndex], 
           SIOCETHTOOL, 
           &s_NetHdl.as_ifReq[w_ifIndex]) == 0)
  {
    if(s_edata.data)
    {
      /* link detected  */
      o_ret = 0;
    }
    else
    {
      /* no link */
      o_ret = -1;
    }
  }
  else
  {
    o_ret = -1;
  }
  return o_ret;
}
#endif

/*
----------------------------------------------------------------------------
                                InitSePortStructures()

Description:
Initialize the Sync-E port socket interfaces data structures.

Parameters:

Inputs
        None

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
void InitSePortStructures (void)
{
   /* Initialize Port Interface structure */
   memset(&port_itf, 0, sizeof(port_itf));
}

/*
----------------------------------------------------------------------------
                                InitSePort()

Description:
Initialize the Sync-E port socket interface for a requested channel index
to prepare for ESMC protocol.

Parameters:

Inputs
        b_ChanIndex - index of Sync-E channel

Outputs
        None
        
Return value:
        0 - success
       -1 - failure

-----------------------------------------------------------------------------
*/
int InitSePort (UINT8 b_ChanIndex)
{
	UINT32 i;
	struct ifreq ifr;
	struct packet_mreq mr;
	struct sockaddr_ll sll;
   int flags;

	/* look through the arrays to find an available port interface */
	for (i = 0; i < MAX_ETH_CHANNELS; i++) 
	{
      if((GetSe1Chan() == b_ChanIndex) && (i == 0))
      {
         /* carry on */
      }
      else if((GetSe2Chan() == b_ChanIndex) && (i == 1))
      {
         /* carry on */
      }
      else
      {
         continue;
      }
      /* if port is already being used */
      if (port_itf.sd_enabled[i])
      {
         /* iterate to next port interface to see if it unused */
         continue;
      }

      if ( (port_itf.sd_iface[i] = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_SLOW))) < 0 ) { 
         fprintf(stderr,"PORTITF: Can't create raw socket\n");
         /* Close any open ports */
         CleanupSePorts();
         /* return failed */
         return -1;
      }

      /* if current socket descriptor maximum handle value is less than 
         newly generated socket descriptor handle */
      if (port_itf.sd_max < port_itf.sd_iface[i])
      {
         /* store larger value */
         port_itf.sd_max = port_itf.sd_iface[i];
      }

      flags = fcntl(port_itf.sd_iface[i], F_GETFL);
      flags |= O_NONBLOCK;
      fcntl(port_itf.sd_iface[i], F_SETFL, flags);

      memset(&ifr, 0, sizeof(struct ifreq));
      sprintf(ifr.ifr_name, "eth%d", i);

      if (ioctl(port_itf.sd_iface[i], SIOCGIFINDEX, &ifr ) < 0)
      {
         fprintf(stderr,"PORTITF: Can't get interface index\n");
         /* Close any open ports */
         CleanupSePorts();
         /* return failed */
         return -1;
      }

      memset(&sll,'\0',sizeof(struct sockaddr_ll));
      sll.sll_family   = AF_PACKET;
      sll.sll_ifindex  = ifr.ifr_ifindex;
      sll.sll_protocol = htons(ETH_P_SLOW); 
      sll.sll_pkttype  = PACKET_MULTICAST; 

      if (bind(port_itf.sd_iface[i], (struct sockaddr*)&(sll), sizeof(sll)) < 0) {
         fprintf(stderr,"PORTITF: Can't bind socket\n");
         /* Close any open ports */
         CleanupSePorts();
         /* return failed */
         return -1;
      }

      /** Join multicast membership */
      memset(&mr, 0, sizeof(mr));
      mr.mr_ifindex = ifr.ifr_ifindex;
      mr.mr_type =  PACKET_MR_MULTICAST;
      mr.mr_alen = 6;
      bzero((char *)&mr.mr_address, 6); 
      memcpy(mr.mr_address, &multi_mac_addrs[0], 6);

      if (setsockopt( port_itf.sd_iface[i], SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
         fprintf( stderr, "Multicast: Can't add multicast membership. Errno: %s\n", strerror(errno) );
         /* Close any open ports */
         CleanupSePorts();
         /* return failed */
         return -1;
      }

      /* set flag to true only after successful completion of all socket related system calls */
      port_itf.sd_enabled[i] = TRUE;

      /* store channel index */
      port_itf.b_ChanIndex[i] = b_ChanIndex;

      /* return successfully */
      return 0;
   }
   
   /* return failure, reached the end of the array without an available port interface */
   return -1;
}

/*
----------------------------------------------------------------------------
                                ProcessESMC()

Description:
Receive Sync-E ESMC messages and process.

Parameters:

Inputs
        None

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
void ProcessESMC (void)
{
   UINT32 i;
   char ossp_eth_std_raw_frame[MAX_ETH_CHANNELS][FRM_LEN_REQUEST];
   int frame_size;
   socklen_t from_size[MAX_ETH_CHANNELS];
   struct sockaddr_in from[MAX_ETH_CHANNELS];
   struct timeval timeout;
   fd_set fds;
   UINT32 fcs_sent, fcs_calc;

   /* zero out timeout structure */
   memset(&timeout, 0, sizeof(struct timeval));

   /* zero out file descriptor set */
   FD_ZERO(&fds);

   /* for all channels */
   for (i = 0; i < MAX_ETH_CHANNELS; i++)
   {
      /* if port is enabled */
      if (port_itf.sd_enabled[i])
      {
         /* prepare file descriptor set, add socket to set */
         FD_SET(port_itf.sd_iface[i], &fds);
      }
   }

   /* zero out packet buffer */
   for (i = 0; i < MAX_ETH_CHANNELS; i++)
   {
      memset(&(ossp_eth_std_raw_frame[i]), 0, FRM_LEN_REQUEST);
   } 

   /* check the availability of data */
   if (select(port_itf.sd_max+1, &fds, NULL, NULL, &timeout) > 0)
   {
      /* traverse port list */
      for (i = 0; i < MAX_ETH_CHANNELS; i++)
      {
         /* if socket is member of file descriptor set */
         if (FD_ISSET(port_itf.sd_iface[i], &fds) > 0)
         {   
            /* set SOCKADDR_IN size */
            from_size[i] = sizeof(struct sockaddr_in);

            /* if no error receiving the packet  */
            if((frame_size = recvfrom(port_itf.sd_iface[i], (char*)ossp_eth_std_raw_frame[i], FRM_LEN_REQUEST, 0, 
                                       (struct sockaddr*)&from[i], &from_size[i])) > 0) 
            {
               /* retrieve fcs from frame */
               fcs_sent = ntohl(*(UINT32 *)(&ossp_eth_std_raw_frame[i][ ETH_FCS_OFFSET ]));
         
               /* if there is a valid checksum to checkagainst */
               if (fcs_sent > 0) 
               {
                  /* calculcate checksum and check whether frame is uncorrupted */
                  if ((fcs_calc = (CalcSeFCS(ossp_eth_std_raw_frame[i], OSSP_FRAME_LEN - ETH_FCS_LEN))) == fcs_sent)
                  {
                     /* process packet */
                     processPacket(port_itf.b_ChanIndex[i], &ossp_eth_std_raw_frame[i][0]);
                  }
               }
               /* no checksum to check, process packet */
               else
               {
                     /* process packet */
                     processPacket(port_itf.b_ChanIndex[i], &ossp_eth_std_raw_frame[i][0]);
               }
            }
         }
      }
   }
}

/*
----------------------------------------------------------------------------
                                CleanupSePorts()

Description:
Close Sync-E ports and clear enable flags.

Parameters:

Inputs
        None

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
static void CleanupSePorts (void)
{
   UINT32 i;

   /* search through array of port interfaces */
   for (i = 0; i < MAX_ETH_CHANNELS; i++) 
   {
      /* clear enabled flag */
      port_itf.sd_enabled[i] = FALSE;

      /* if socket is open */
      if ( (port_itf.sd_iface[i] > 0) )
      {
         /* close socket */
         close(port_itf.sd_iface[i]);
      }
   }
}

/*
----------------------------------------------------------------------------
                                CloseSePort()

Description:
Close Sync-E ports and clear enable flags.

Parameters:

Inputs
        b_ChanIndex - index of Sync-E channel

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
void CloseSePort (UINT8 b_ChanIndex)
{
   UINT32 i;

   /* search through array of port interfaces */
   for (i = 0; i < MAX_ETH_CHANNELS; i++) 
   {
      /* if this is the requested channel index */
      if (port_itf.b_ChanIndex[i] == b_ChanIndex)
      {
         /* clear enabled flag */
         port_itf.sd_enabled[i] = FALSE;

         /* if socket is open */
         if ( (port_itf.sd_iface[i] > 0) )
         {
            /* close socket */
            close(port_itf.sd_iface[i]);
         }
      }
   }
}

/*
----------------------------------------------------------------------------
                                CalcSeFCS()

Description:
Clean up open ports.

Parameters:

Inputs
        None

Outputs
        None
        
Return value:
        fcs value

-----------------------------------------------------------------------------
*/
static UINT32 CalcSeFCS(char * data, int length)
{
   UINT32 n, fcs, crc;
	
   fcs = crc = 0;
   for (n=0; n<length; n++)
   {
      crc = (crc >> 4) ^ crc_table[(crc ^ (data[n] >> 0)) & 0x0F];  
      crc = (crc >> 4) ^ crc_table[(crc ^ (data[n] >> 4)) & 0x0F];  
   }
	
   fcs  = (crc & 0x000000FFUL) << 24;
   fcs |= (crc & 0x0000FF00UL) << 8;
   fcs |= (crc & 0x00FF0000UL) >> 8;
   fcs |= (crc & 0xFF000000UL) >> 24;

   return fcs;
}

/*
----------------------------------------------------------------------------
                                processPacket()

Description:
Deconstruct the Sync-E packet and store the appropriate components (version, SSM).

Parameters:

Inputs
        None

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
static void processPacket(UINT8 b_ChanIndex, char * pbuf)
{
   /* extract the SSM and version and deliver to channel SSM handling unit */
   SC_SetSSMValue(b_ChanIndex, pbuf[ETH_SSM_OFFSET] & 0x0F, TRUE,
      ((pbuf[ETH_EVENT_OFFSET] & 0xF0) >> 4), 5);

#ifdef SC_NETWORK_IF_DEBUG
   printf("Dest MAC address is 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n", 
         pbuf[ETH_DEST_OFFSET+0], pbuf[ETH_DEST_OFFSET+1],
         pbuf[ETH_DEST_OFFSET+2], pbuf[ETH_DEST_OFFSET+3],
         pbuf[ETH_DEST_OFFSET+4], pbuf[ETH_DEST_OFFSET+5]);
   printf("Ether type is 0x%X 0x%X\n", pbuf[ETH_ETHER_TYPE_OFFSET], 
         pbuf[ETH_ETHER_TYPE_OFFSET+1]);
   printf("Slow Protocol type is 0x%X\n", pbuf[ETH_SLOPRO_TYPE_OFFSET]);
   printf("ITU-OUI is 0x%X 0x%X 0x%X\n", pbuf[ETH_ITU_OUI_OFFSET],
         pbuf[ETH_ITU_OUI_OFFSET+1], pbuf[ETH_ITU_OUI_OFFSET+2]);
   printf("ITU subtype is 0x%X 0x%X\n", pbuf[ETH_ITU_SUBTYPE_OFFSET],
         pbuf[ETH_ITU_SUBTYPE_OFFSET+1]);
   printf("Version and Event are 0x%X\n", pbuf[ETH_EVENT_OFFSET]);
   printf("SSM value is 0x%X\n", pbuf[ETH_SSM_OFFSET]);
   printf("FCS value is 0x%X 0x%X 0x%X 0x%X\n", pbuf[ETH_FCS_OFFSET],
         pbuf[ETH_FCS_OFFSET+1], pbuf[ETH_FCS_OFFSET+2],
         pbuf[ETH_FCS_OFFSET+3]);
   printf("\n");
#endif
}

/*
----------------------------------------------------------------------------
                                setSE1los()

Description:
Set LOS state for Sync-E 1 (eth0).

Parameters:

Inputs
        b_se1_los - TRUE - no link status on eth0
                  - FALSE - link on eth0

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
void setSE1los(BOOLEAN b_se1_los)
{
   se1_los = b_se1_los;
}

/*
----------------------------------------------------------------------------
                                setSE2los()

Description:
Set LOS state for Sync-E 2 (eth1).

Parameters:

Inputs
        b_se2_los - TRUE - no link status on eth1
                  - FALSE - link on eth1

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
void setSE2los(BOOLEAN b_se2_los)
{
   se2_los = b_se2_los;
}

/*
----------------------------------------------------------------------------
                                setSE2los()

Description:
Send status of LOS (link status) to SSM handling code.

Parameters:

Inputs
        None

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
void ProcessLOS(void)
{
   UINT32 i;

   /* look through the arrays to find whether port interface is being used */
   for (i = 0; i < MAX_ETH_CHANNELS; i++) 
   {
      /* if port is already being used, then Sync-E has been enabled 
         and channel index is correct */
      if (port_itf.sd_enabled[i])
      {
         /* branch on port interface number to get appropriate LOS flag */
         switch (i)
         {
            case 0:
               /* SE1 is in first element of port interface structure, set valid flag 
                  based on LOS flag */
               SC_SetChanValid(port_itf.b_ChanIndex[i], (se1_los ? FALSE : TRUE));
               break;
            case 1:
               /* SE2 is in second element of port interface structure, set valid flag 
                  based on LOS flag */
               SC_SetChanValid(port_itf.b_ChanIndex[i], (se2_los ? FALSE : TRUE));
               break;
            default:
               /* Invalid case */
               break;
         }
      }
   }
}
