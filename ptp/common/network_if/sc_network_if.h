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

FILE NAME    : sc_network_if.h

AUTHOR       : Jining Yang

DESCRIPTION  : 

This header file contains the definitions related to network interface.

Revision control header:
$Id: network_if/sc_network_if.h 1.8 2011/02/16 17:53:39PST Daniel Brown (dbrown) Exp  $

******************************************************************************
*/

/* Prevent file from being included multiple times. */
#ifndef H_SC_NETWORK_IF_h
#define H_SC_NETWORK_IF_h

#ifdef __cplusplus
extern "C"
{
#endif

#include <linux/sockios.h>
#include <net/if.h>

#define k_NUM_IF      1

#define LINK_SPEED_1000 1000
#define LINK_SPEED_100  100
#define LINK_SPEED_10   10
#define LINK_SPEED_NONE 0

#define LINK   1
#define NOLINK 0

/*** Sync-E ESMC defines */
#define ETH_DEST_OFFSET                0
#define ETH_SRC_OFFSET                 6
#define ETH_ETHER_TYPE_OFFSET          12
#define ETH_SLOPRO_TYPE_OFFSET         14
#define ETH_ITU_OUI_OFFSET             15
#define ETH_ITU_SUBTYPE_OFFSET         18
#define ETH_EVENT_OFFSET               20
#define ETH_SSM_OFFSET                 27
#define ETH_FCS_OFFSET                 60

#define ETH_FCS_LEN                    4
#define ETH_VLAN_LEN                   4
#define OSSP_FRAME_LEN                 64

#define ESMC_VERSION                   0x10
#define ESMC_EVENT                     0x08

//#define SC_NETWORK_IF_DEBUG 1

/************************************************************/
/***                ESMC Standard Frame	                ***/
/************************************************************/

/***********************************************/
/***             Ethernet Header             ***/
/***********************************************/

/*****************************************************************************/
/*    0x01, 0x80, 0xC2, 0x00, 0x00, 0x02,             --> DST MAC address    */
/*    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             --> SRC MAC address    */
/*    0x88, 0x09,                                     --> Ether Type         */
/*****************************************************************************/

/***********************************************/
/***           Slow Protocol Header          ***/
/***********************************************/

/*****************************************************************************/
/*    0x0A,                                           --> Slow Proto Subtype */
/*    0x00, 0x19, 0xA7,                               --> ITU-OUI				  */
/*    0x00, 0x01,                                     --> ITU Subtype		  */	
/*    0x00,                                           --> Version & Event	  */
/*    0x00, 0x00, 0x00,                               --> Reserved			  */	
/*    0x01,                                           --> TLV Type			  */	
/*    0x00, 0x04,                                     --> TLV Length			  */
/*    0x00,                                           --> QL					  */
/*    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, --> Padding				  */
/*    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, --> Padding				  */
/*    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, --> Padding				  */
/*    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, --> Padding				  */
/*    0x00, 0x00, 0x00, 0x00                          --> FCS					  */
/*****************************************************************************/

/*
----------------------------------------------------------------------------
Description:
This struct holds the data for the driver device handle of a network interface.
-----------------------------------------------------------------------------
*/
typedef struct
{
  INT32        s_evtSock;
  struct ifreq s_if_data;
} t_netHandle;

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
INT32 SC_GetSockHdl(UINT8 b_port, char* pb_ifName);

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
int SC_GetLinkSpeed(int portnum, int* speed);
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
int SC_GetLinkStat(int portnum, int* stat);

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
void InitSePortStructures (void);

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
int InitSePort (UINT8 b_ChanIndex);

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
void ProcessESMC (void);

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
void CloseSePort (UINT8 b_ChanIndex);

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
void setSE1los(BOOLEAN b_se1_los);

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
void setSE2los(BOOLEAN b_se2_los) ;

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
void ProcessLOS(void);

#ifdef __cplusplus
}
#endif

#endif /* H_SC_NETWORK_IF_h */

