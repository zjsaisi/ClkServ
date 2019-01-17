//****************************************************************************
// epl_1588.c
// 
// Copyright (c) 2007 National Semiconductor Corporation.
// All Rights Reserved
// 
// Contains sources for IEEE 1588 related functions.
//
// The following functions are implemented in this module:
//
//      PTPEnable
//      PTPSetTriggerConfig
//      PTPSetEventConfig
//      PTPSetTransmitConfig
//      PTPSetPhyStatusFrameConfig
//      PTPSetReceiveConfig
//      PTPCalcSourceIdHash
//      PTPSetTempRateDurationConfig
//      PTPSetClockConfig
//      PTPSetGpioInterruptConfig
//      PTPSetMiscConfig
//      PTPClockReadCurrent
//      PTPClockStepAdjustment
//      PTPClockSet
//      PTPClockSetRateAdjustment
//      PTPCheckForEvents
//      PTPGetTransmitTimestamp
//      PTPGetReceiveTimestamp
//      PTPGetTimestampFromFrame
//      PTPArmTrigger
//      PTPHasTriggerExpired
//      PTPCancelTrigger
//      PTPGetEvent
//      MonitorGpioSignals
//      IsPhyStatusFrame
//      GetNextPhyMessage
//****************************************************************************
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#ifdef use_linux_libc5
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#endif
#include <linux/sockios.h>

#include "epl.h"

typedef unsigned short int uint_16;
typedef short int int_16;
typedef unsigned long int uint_32;
typedef long int int_32;

#define delay_loop(x)  	for(i=0;i<x;i++)	{j = 0;}

//static uint_16 busy_flag = 0;
static pthread_mutex_t mii_mutex = PTHREAD_MUTEX_INITIALIZER;
static int skfd = -1;
static unsigned int phy_id = 0;
static struct ifreq if_data;

//static LWSEM_STRUCT  MII_lwsem;

void Init_PHY_Resource(void)
{
	unsigned short *data = (unsigned short *)&if_data.ifr_data;
        char* ifname = "eth0";

        if ((skfd = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
//              perror("socket");
                fprintf(stderr, "Socket error\n");
                exit(-1);
        }

        /* Get the vitals from the interface. */
        strncpy(if_data.ifr_name, ifname, IFNAMSIZ);
        if (ioctl(skfd, SIOCGMIIPHY, &if_data) < 0) {
                fprintf(stderr, "SIOCGMIIPHY on %s failed: %s\n", ifname,
                                strerror(errno));
                fprintf(stderr, "skfd: %d, SIOCGMIIPHY: %d, ifr: %p\n", skfd, SIOCGMIIPHY, &if_data);
                (void) close(skfd);
                exit(-1);
        }
        phy_id = data[0];

	// create semaphore for resource protection
	pthread_mutex_init(&mii_mutex, NULL);
}

int Get_MII_Resource(void)
{
	return pthread_mutex_lock(&mii_mutex);
}

int Release_MII_Resource(void)
{
	return pthread_mutex_unlock(&mii_mutex);
}

int mdio_read(int skfd, int phy_id, int location)
{
	unsigned short *data = (unsigned short *)&if_data.ifr_data;

	data[0] = phy_id;
	data[1] = location;
	if (ioctl(skfd, SIOCGMIIREG, &if_data) < 0) {
		fprintf(stderr, "SIOCGMIIREG on %s failed: %s\n",
		        if_data.ifr_name, strerror(errno));
		return -1;
	}
	return data[3];
}

void mdio_write(int skfd, int phy_id, int location, int value)
{
	unsigned short *data = (unsigned short *)&if_data.ifr_data;

	data[0] = phy_id;
	data[1] = location;
	data[2] = value;
	if (ioctl(skfd, SIOCSMIIREG, &if_data) < 0) {
		fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n",
		        if_data.ifr_name, strerror(errno));
	}
}

int write_phy_reg (int skfd, unsigned int phy_id, unsigned int regval)
{
	int reg = (regval >> 16) & 0x1f;
	mdio_write (skfd, phy_id, reg, regval & 0xffff);
//	printf ("writing phy %d register %d with value: %04x\n",phy_id,reg,regval&0xffff);
	return 0;
}

int read_phy_reg (int skfd, unsigned phy_id, int reg)
{
	unsigned short int readval;
	readval = mdio_read (skfd, phy_id, reg & 0x1f);
//	printf ("reading register %d : %04x\n", reg & 0x1f, readval);
	return readval;
}

void send_to_MMFR(uint_16 page, uint_16 reg, uint_16 data)
{
	if (skfd < 0) return;

	write_phy_reg (skfd, phy_id, ((0x13 << 16) | page));
	write_phy_reg (skfd, phy_id, ((reg  << 16) | data));
	return;
}

uint_16 read_from_MMFR(uint_16 page, uint_16 reg)
{
	if (skfd < 0) return 0;

	write_phy_reg (skfd, phy_id, ((0x13 << 16) | page));
	return read_phy_reg (skfd, phy_id, ((page << 16) | reg));
}
uint_16 read_from_MMFR_no_paging(uint_16 reg)
{
#if 0
	Get_MII_Resource();
	
	MCF_FEC_MMFR =  MCF_FEC_MMFR_ST_01 | /* start bits */
					MCF_FEC_MMFR_TA_10 | /* tail bits */
					MCF_FEC_MMFR_OP_READ | /* write OP */
					MCF_FEC_MMFR_PA(1) | /* page number */
					MCF_FEC_MMFR_RA(reg) | /* page number */
					0; /* 16 bits of data */

	_time_delay(10);
	busy_flag = 0;
	Release_MII_Resource();
	
	return (uint_16)MCF_FEC_MMFR;
#else
	return 0;
#endif
}
void OAIBeginMultiCriticalSection( struct OAI_DEV_HANDLE_STRUCT *portHandle)
{
	Get_MII_Resource();	
}
void OAIEndMultiCriticalSection( struct OAI_DEV_HANDLE_STRUCT *portHandle)
{
	Release_MII_Resource();	
}
//****************************************************************************
EXPORT NS_UINT
    EPLReadReg(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT registerIndex)

//  Reads the contents of the specified port register. Refer to the 
//  epl_regs.h file for register definitions.
//
//  portHandle
//      Handle that represents a port. This is obtained using the 
//      EPLEnumPort function.
//  registerIndex
//      Index of the register to read. Bits 7:5 select the 
//      register page (000-pg0, 001-pg1, 010-pg3, 011-pg4, etc.).
//
//  Returns:
//      Value read from the register.
//****************************************************************************
{
    return read_from_MMFR(registerIndex>>5, registerIndex & 0x1F);
#if 0	
	uint_16 data;
	Read_MMFR(registerIndex, &data);

	return data;
#endif
}


//****************************************************************************
EXPORT void
    EPLWriteReg(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT registerIndex,
        IN NS_UINT value)
        
//  Writes a value to the specified port register. Refer to the epl_regs.h 
//  file for register definitions.
//
//  portHandle
//      Handle that represents a port. This is obtained using the 
//      EPLEnumPort function.
//  registerIndex
//      Index of the register to write. Bits 7:5 select the 
//      register page (000-pg0, 001-pg1, 010-pg3, 011-pg4, etc.).
//  value
//      The value to write to the register (0x0000 - 0xFFFF).
//
//  Returns:
//      Nothing
//****************************************************************************
{
    send_to_MMFR(registerIndex>>5, registerIndex & 0x1F, value);
#if 0
	Write_MMFR(registerIndex, value);
#else
#endif
	
}


//****************************************************************************
EXPORT void
    PTPEnable(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_BOOL enableFlag)
        
//  Enables or disables the PHY's PTP 1588 Clock.
//
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  enableFlag
//      Set to TRUE to enable the Phy's 1588 hardware clock. Set to FALSE
//      to disable it.
//
//  Returns:
//      Nothing
//****************************************************************************
{
    EPLWriteReg( portHandle, PHY_PG4_PTP_CTL, (enableFlag) ? P640_PTP_ENABLE : P640_PTP_DISABLE);
    return;
}

 
//****************************************************************************
EXPORT void
    PTPSetTriggerConfig(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT trigger,
        IN NS_UINT triggerBehavior,
        IN NS_UINT gpioConnection)
        
//  Configures the operational behavior of an individual trigger.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  trigger
//      The trigger to configure, 0 - 7.
//  triggerBehavior
//      A bitmap of configuration options. Zero or more of the bits defined 
//      by TRGOPT_??? OR'ed together.
//  gpioConnection
//      The GPIO pin the trigger should be connected to. A value of 0 - 12. 
//      If 0 is specified no GPIO pin connection is made.
//  Returns
//      Nothing
//****************************************************************************
{
NS_UINT reg;

    reg = 0;
    if ( triggerBehavior & TRGOPT_PULSE)        reg |= P640_TRIG_PULSE;
    if ( triggerBehavior & TRGOPT_PERIODIC)     reg |= P640_TRIG_PER;
    if ( triggerBehavior & TRGOPT_TRG_IF_LATE)  reg |= P640_TRIG_IF_LATE;
    if ( triggerBehavior & TRGOPT_NOTIFY_EN)    reg |= P640_TRIG_NOTIFY;
    if ( triggerBehavior & TRGOPT_TOGGLE_EN)    reg |= P640_TRIG_TOGGLE;
    
    reg |= gpioConnection << P640_TRIG_GPIO_SHIFT;
    reg |= trigger << P640_TRIG_CSEL_SHIFT;
    reg |= P640_TRIG_WR;
    
    EPLWriteReg( portHandle, PHY_PG5_PTP_TRIG, reg);
    return;
}
 

//****************************************************************************
EXPORT void
    PTPSetEventConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT event,
        IN NS_BOOL eventRiseFlag,
        IN NS_BOOL eventFallFlag,
        IN NS_UINT gpioConnection)

//  Configures the operational behavior of an individual event.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  event
//      The event to configure, 0 - 7.
//  eventRiseFlag
//      If set to TRUE, enables detection of rising edge on Event input.
//  eventFallFlag
//      If set to TRUE, enables detection of falling edge on Event input.
//  gpioConnection
//      The GPIO pin the event should be connected to. A value of 0 - 12. 
//      If 0 is specified no GPIO pin connection is made.
//  Returns
//      Nothing
//****************************************************************************
{
NS_UINT reg;

    reg = 0;
    
    reg |= gpioConnection << P640_EVNT_GPIO_SHIFT;
    reg |= event << P640_EVNT_SEL_SHIFT;
    reg |= P640_EVNT_WR;
    EPLWriteReg( portHandle, PHY_PG5_PTP_EVNT, reg);
    
    if ( eventRiseFlag) reg |= P640_EVNT_RISE;
    if ( eventFallFlag) reg |= P640_EVNT_FALL;
    EPLWriteReg( portHandle, PHY_PG5_PTP_EVNT, reg);
    return;
}
 

//****************************************************************************
EXPORT void
    PTPSetTransmitConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT txConfigOptions,
        IN NS_UINT ptpVersion,
        IN NS_UINT ptpFirstByteMask,
        IN NS_UINT ptpFirstByteData)

//  Configures the device's 1588 transmit operation.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  txConfigOptions
//      A bitmap of configuration options. Zero or more of the bits defined 
//      by TXOPT_??? OR'ed together.
//  ptpVersion
//      Enable Timestamp capture for a specific version of the IEEE 1588 
//      specification. This value may be set to any value between 1 and 15 
//      and allows support for future versions of the IEEE 1588 specification. 
//      A value of 0 will disable version checking (not recommended).
//  ptpFirstByteMask
//      Bit mask to be used for matching Byte0 of the PTP Message. A one in 
//      any bit enables matching for the associated data bit. If no matching 
//      is required, all bits of the mask should be set to 0.
//  ptpFirstByteData
//      Data to be used for matching Byte0 of the PTP Message. This parameter 
//      is ignored if ptpFirstByteMask is 0x00.
//  Returns
//      Nothing
//****************************************************************************
{
NS_UINT reg;

    reg = 0;
    if ( txConfigOptions & TXOPT_SYNC_1STEP)    reg |= P640_SYNC_1STEP;
    if ( txConfigOptions & TXOPT_DR_INSERT)     reg |= P640_DR_INSERT;
    if ( txConfigOptions & TXOPT_IGNORE_2STEP)  reg |= P640_IGNORE_2STEP;
    if ( txConfigOptions & TXOPT_CRC_1STEP)     reg |= P640_CRC_1STEP;
    if ( txConfigOptions & TXOPT_CHK_1STEP)     reg |= P640_CHK_1STEP;
    if ( txConfigOptions & TXOPT_IP1588_EN)     reg |= P640_IP1588_EN;
    if ( txConfigOptions & TXOPT_L2_EN)         reg |= P640_TX_L2_EN;
    if ( txConfigOptions & TXOPT_IPV6_EN)       reg |= P640_TX_IPV6_EN;
    if ( txConfigOptions & TXOPT_IPV4_EN)       reg |= P640_TX_IPV4_EN;
    if ( txConfigOptions & TXOPT_TS_EN)         reg |= P640_TX_TS_EN;

    reg |= ptpVersion << P640_TX_PTP_VER_SHIFT;
    EPLWriteReg( portHandle, PHY_PG5_PTP_TXCFG0, reg);

    reg = (ptpFirstByteMask << P640_BYTE0_MASK_SHIFT) | (ptpFirstByteData << P640_BYTE0_DATA_SHIFT);
    EPLWriteReg( portHandle, PHY_PG5_PTP_TXCFG1, reg);
    return;
}
 

//****************************************************************************
EXPORT void
    PTPSetPhyStatusFrameConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT statusConfigOptions,
        IN MAC_SRC_ADDRESS_ENUM srcAddrToUse,
        IN NS_UINT minPreamble,
        IN NS_UINT ptpReserved,
        IN NS_UINT ptpVersion,
        IN NS_UINT transportSpecific,
        IN NS_UINT messageType,
        IN NS_UINT32 sourceIpAddress,
        IN NS_UINT ipChecksum)

//  Configures the device's Phy Status Frame (PSF) operational configuration.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  statusConfigOptions
//      A bitmap of configuration options. Zero or more of the bits defined
//      the STSOPT_??? bit definitions - OR'ed together.
//  srcAddrToUse
//      Specifies the MAC source address to use in Phy Status Frames (PSF). 
//      Must be one of the following values.
//          STS_SRC_ADDR_1 - [00 00 00 00 00 00]
//          STS_SRC_ADDR_2 - [08 00 17 0B 6B 0F]
//          STS_SRC_ADDR_3 - [08 00 17 00 00 00]
//          STS_SRC_ADDR_USE_MC - Use MAC multicast destination address
//  minPreamble
//      Determines the minimum number of preamble bytes required for sending 
//      Phy Status Frames (PSF) on the MII interface. It is recommended that 
//      this be set to the smallest value the MAC will tolerate.
//  ptpReserved
//      PTP v2 reserved field: This field contains the reserved 4-bit field 
//      (at offset 1) to be sent in status packets from the Phy to the local 
//      MAC using the MII receive data interface.
//  ptpVersion
//      The PTP version number to set in the PTP version field of the status 
//      frame. Typically this is set to a value of 0x02.
//  transportSpecific
//      The value to use for the transportSpecific field for status frames 
//      from the Phy to the local MAC using the MII receive data interface. 
//      A value of 0x0F ensures the frame will not be interpreted as a valid 
//      PTP message.
//  messageType
//      The value to use for the messageType field for status frames from 
//      the Phy to the local MAC using the MII receive data interface. 
//      The default value of 0x0F ensures the frame will not be interpreted 
//      as a valid PTP message.
//  sourceIpAddress
//      4-byte source IP address to use in frames created by the Phy.
//  ipChecksum
//      This 16-bit value assists in computation of the IP checksum for an 
//      IPv4 status-based frame. This is a precomputed value ones-complement 
//      addition of all fixed values in the IP Header. The device will add 
//      the Total Length and Identification values to generate the final 
//      checksum.
//
//  Returns
//      Nothing
//****************************************************************************
{
PPORT_OBJ portHdl = (PPORT_OBJ)portHandle;
NS_UINT reg;
NS_UINT8 *ptr;
static NS_UINT8 srcAddrs[4][6] = \
                        { {0x08,0x00,0x17,0x0B,0x6B,0x0F},
                          {0x08,0x00,0x17,0x00,0x00,0x00},
                          {0x01,0x00,0x00,0x00,0x00,0x00},
                          {0x00,0x00,0x00,0x00,0x00,0x00} };
                          
    portHdl->psfConfigOptions = statusConfigOptions;

    ptr = srcAddrs[ srcAddrToUse ];
    *(NS_UINT32*)&portHdl->psfSrcMacAddr[0] = *(NS_UINT32*)&ptr[0];
    *(NS_UINT16*)&portHdl->psfSrcMacAddr[4] = *(NS_UINT16*)&ptr[4];
    
    reg = 0;
    if ( statusConfigOptions & STSOPT_LITTLE_ENDIAN) reg |= P640_PKT_ENDIAN;
    if ( statusConfigOptions & STSOPT_IPV4)          reg |= P640_PKT_IPV4;
    if ( statusConfigOptions & STSOPT_TXTS_EN)       reg |= P640_PKT_TXTS_EN;
    if ( statusConfigOptions & STSOPT_RXTS_EN)       reg |= P640_PKT_RXTS_EN;
    if ( statusConfigOptions & STSOPT_TRIG_EN)       reg |= P640_PKT_TRIG_EN;
    if ( statusConfigOptions & STSOPT_EVENT_EN)      reg |= P640_PKT_EVNT_EN;

    reg |= srcAddrToUse << P640_MAC_SRC_ADD_SHIFT;
    reg |= minPreamble << P640_MIN_PRE_SHIFT;
    EPLWriteReg( portHandle, PHY_PG5_PTP_PKTSTS0, reg);
    
    reg = ptpReserved << P640_PTP_RESERVED_SHIFT;
    reg |= ptpVersion << P640_VERSION_PTP_SHIFT;
    reg |= transportSpecific << P640_TRANSP_SPEC_SHIFT;
    reg |= messageType << P640_MESSAGE_TYPE_SHIFT;
    EPLWriteReg( portHandle, PHY_PG6_PTP_PKTSTS1, reg);

    reg = (sourceIpAddress & 0x000000FF) << P640_IP_SA_BYTE0_SHIFT;
    reg |= ((sourceIpAddress & 0x0000FF00) >> 8) << P640_IP_SA_BYTE1_SHIFT;
    EPLWriteReg( portHandle, PHY_PG6_PTP_PKTSTS2, reg);
    
    reg = ((sourceIpAddress & 0x00FF0000) >> 16) << P640_IP_SA_BYTE2_SHIFT;
    reg |= ((sourceIpAddress & 0xFF000000) >> 24) << P640_IP_SA_BYTE3_SHIFT;
    EPLWriteReg( portHandle, PHY_PG6_PTP_PKTSTS3, reg);
    
    EPLWriteReg( portHandle, PHY_PG6_PTP_PKTSTS4, ipChecksum);
    return;
}
 

//****************************************************************************
EXPORT void
    PTPSetReceiveConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT rxConfigOptions,
        IN RX_CFG_ITEMS *rxConfigItems)

//  Configures the device's receive operation.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  rxConfigOptions
//      A bitmap of configuration options. Zero or more of the bits defined 
//      by the RXOPT_??? bit definitions OR'ed together.
//  rxConfigItems
//      This structure of configuration values must be filled out prior to 
//      making this call. See the RX_CFG_ITEMS structure definition.
//  Returns
//      Nothing
//****************************************************************************
{
PPORT_OBJ portHdl = (PPORT_OBJ)portHandle;
NS_UINT reg;

    reg = 0;
    if ( rxConfigOptions & RXOPT_DOMAIN_EN)      reg |= P640_DOMAIN_EN;
    if ( rxConfigOptions & RXOPT_ALT_MAST_DIS)   reg |= P640_ALT_MAST_DIS;
    if ( rxConfigOptions & RXOPT_USER_IP_EN)     reg |= P640_USER_IP_EN;
    if ( rxConfigOptions & RXOPT_RX_SLAVE)       reg |= P640_RX_SLAVE;
    if ( rxConfigOptions & RXOPT_IP1588_EN0)     reg |= P640_IP1588_EN0;
    if ( rxConfigOptions & RXOPT_IP1588_EN1)     reg |= P640_IP1588_EN1;
    if ( rxConfigOptions & RXOPT_IP1588_EN2)     reg |= P640_IP1588_EN2;
    if ( rxConfigOptions & RXOPT_RX_L2_EN)       reg |= P640_RX_L2_EN;
    if ( rxConfigOptions & RXOPT_RX_IPV6_EN)     reg |= P640_RX_IPV6_EN;
    if ( rxConfigOptions & RXOPT_RX_IPV4_EN)     reg |= P640_RX_IPV4_EN;
    if ( rxConfigOptions & RXOPT_RX_TS_EN)       reg |= P640_RX_TS_EN;
   
    reg |= rxConfigItems->ptpVersion << P640_RX_PTP_VER_SHIFT;
    EPLWriteReg( portHandle, PHY_PG5_PTP_RXCFG0, reg);
    EPLWriteReg( portHandle, PHY_PG5_PTP_RXCFG2, rxConfigItems->ipAddrData >> 16);
    EPLWriteReg( portHandle, PHY_PG5_PTP_RXCFG0, reg | P640_USER_IP_SEL);
    EPLWriteReg( portHandle, PHY_PG5_PTP_RXCFG2, rxConfigItems->ipAddrData & 0x0000FFFF);
    EPLWriteReg( portHandle, PHY_PG5_PTP_RXCFG0, reg);

    reg = rxConfigItems->ptpFirstByteMask << P640_BYTE0_MASK_SHIFT;
    reg |= rxConfigItems->ptpFirstByteData << P640_BYTE0_DATA_SHIFT;
    EPLWriteReg( portHandle, PHY_PG5_PTP_RXCFG1, reg);
    portHdl->rxConfigOptions = rxConfigOptions;
    reg = 0;    
    if ( rxConfigOptions & RXOPT_ACC_UDP)        reg |= P640_ACC_UDP;
    if ( rxConfigOptions & RXOPT_ACC_CRC)        reg |= P640_ACC_CRC;
    if ( rxConfigOptions & RXOPT_TS_APPEND)      reg |= P640_TS_APPEND;
    if ( rxConfigOptions & RXOPT_TS_INSERT)      reg |= P640_TS_INSERT;
    
    reg |= rxConfigItems->tsMinIFG << P640_TS_MIN_IFG_SHIFT;
    reg |= rxConfigItems->ptpDomain << P640_PTP_DOMAIN_SHIFT;
    EPLWriteReg( portHandle, PHY_PG5_PTP_RXCFG3, reg);
    
    reg = 0;
    if ( rxConfigOptions & RXOPT_IPV4_UDP_MOD) reg |= P640_IPV4_UDP_MOD;
    if ( rxConfigOptions & RXOPT_TS_SEC_EN)    reg |= P640_TS_SEC_EN;

    portHdl->tsSecondsLen = rxConfigItems->tsSecLen;
    portHdl->rxTsNanoSecOffset = rxConfigItems->rxTsNanoSecOffset;
    portHdl->rxTsSecondsOffset = rxConfigItems->rxTsSecondsOffset;
    
    reg |= rxConfigItems->tsSecLen << P640_TS_SEC_LEN_SHIFT;
    reg |= rxConfigItems->rxTsNanoSecOffset << P640_RXTS_NS_OFF_SHIFT;
    reg |= rxConfigItems->rxTsSecondsOffset << P640_RXTS_SEC_OFF_SHIFT;
    EPLWriteReg( portHandle, PHY_PG5_PTP_RXCFG4, reg);

    reg = rxConfigItems->srcIdHash << P640_PTP_RX_HASH_SHIFT;
    if ( rxConfigOptions & RXOPT_SRC_ID_HASH_EN) reg |= P640_RX_HASH_EN;
    EPLWriteReg( portHandle, PHY_PG6_PTP_RXHASH, reg);
    return;
}
 

//****************************************************************************
EXPORT NS_UINT
    PTPCalcSourceIdHash (
        IN NS_UINT8 *tenBytesData)
        
//  Utility function that will calculate a 12-bit CRC-32 (IEEE 802.3, no 
//  complement) value used to program the RX_CFG_ITEMS.srcIdHash field.
//  
//  tenBytesData
//      This is a pointer to the desired fixed values found in bytes 20 - 29 
//      of the PTP event message that receive source identification filtering 
//      should occur.
//  Returns
//      Value representing the CRC-32 (IEEE 802.3, no complement) generated 
//      from the specified 10-byte buffer.
//
//  This function would only be used if RXOPT_SRC_ID_HASH_EN was specified 
//  in a call to PTPSetReceiveConfig.
//****************************************************************************
{
NS_UINT crc, x, i, val;
NS_UINT8 data;

    crc = 0xFFFFFFFF;
    for ( x = 0; x < 10; x++)
    {
        data = tenBytesData[x];
        
        for ( i = 0; i < 8; i++)
        {
            val = crc << 1;
            if ((data & 0x01) ^ ((crc >> 31) & 0x01))
                crc = val ^ 0x04C11DB7;
            else
                crc = val ^ 0;
            data = data >> 1;
        }
    }

    return (crc & 0xFFFFFFFF) >> 20;
}


//****************************************************************************
EXPORT void
    PTPSetTempRateDurationConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT32 duration)

//  Configures the PTP Temporary Duration.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  duration
//      PTP Temporary Rate Duration: This sets the duration for the Temporary 
//      Rate in number of clock cycles. The actual Time duration is dependent 
//      on the value of the configured Temporary Rate. This value has a range 
//      of up to 2^26 (26-bits may be defined). 
//  Returns
//      Nothing
//
//  This value is remembered by the hardware, therefore the setting can be 
//  used for multiple temporary clock adjustments, if the desired duration 
//  remains constant.
//****************************************************************************
{
    EPLWriteReg( portHandle, PHY_PG5_PTP_TRDH, duration >> P640_PTP_RATE_HI_SHIFT);
    EPLWriteReg( portHandle, PHY_PG5_PTP_TRDL, duration & 0xFFFF);
    return;
}
 

//****************************************************************************
EXPORT void
    PTPSetClockConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT clockConfigOptions,
        IN NS_UINT ptpClockDivideByValue,
        IN NS_UINT ptpClockSource,
        IN NS_UINT ptpClockSourcePeriod)

//  Configures the general PTP clock configuration options.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  clockConfigOptions
//      A bitmap of configuration options. Zero or more of the bits defined in 
//      the CLKOPT_??? bit definitions OR'ed together.
//  ptpClockDivideByValue
//      PTP Clock Divide-by Value: This parameter defines the divide-by value 
//      for the output clock. The output clock is divided from an internal 
//      250MHz clock. Valid values range from 2 to 255 (0x02 to 0xFF), giving 
//      a nominal output frequency range of 125MHz down to 980.4kHz. Divide-by 
//      values of 0 and 1 are not valid and will stop the output clock.
//  ptpClockSource
//      PTP Clock Source Select: Selects among three possible sources for the 
//      PTP reference clock, one of the following values may be specified:
//          0x00 - 125MHz from internal PGM (default)
//          0x01 - Divide-by-N from 125MHz internal PGM
//          0x02 - External reference clock
//  ptpClockSourcePeriod
//      PTP Clock Source Period: Configures the PTP clock source period in 
//      nanoseconds. Values less than 8 are invalid. This parameter is used as 
//      follows by the different clock source modes:
//          0x00 - 125MHz - Ignored
//          0x01 - Divide-by-N - Bits 6:3 are used to divide the 125MHz PGM 
//                 clock by a value between 1 and 15. Bits 2:0 are ignored. 
//          0x02 - External - Bits 6:0 indicate the nominal period in 
//                 nanoseconds of the external reference clock.
//
//  Returns
//      Nothing
//****************************************************************************
{
NS_UINT reg;

    reg = 0;
    if ( clockConfigOptions & CLKOPT_CLK_OUT_EN) reg |= P640_PTP_CLKOUT_EN;
    if ( clockConfigOptions & CLKOPT_CLK_OUT_SEL) reg |= P640_PTP_CLKOUT_SEL;
    if ( clockConfigOptions & CLKOPT_CLK_OUT_SPEED_SEL) reg |= P640_PTP_CLKOUT_SPSEL;
    
    reg |= ptpClockDivideByValue << P640_PTP_CLKDIV_SHIFT;
    EPLWriteReg( portHandle, PHY_PG6_PTP_COC, reg);

    reg = (ptpClockSource << P640_CLK_SRC_SHIFT) | (ptpClockSourcePeriod << P640_CLK_SRC_PER_SHIFT);
    EPLWriteReg( portHandle, PHY_PG6_PTP_CLKSRC, reg);
    
    // Enable the output clock if requested
    reg = EPLReadReg( portHandle, PHY_PG0_PHYCR2);
    if ( clockConfigOptions & CLKOPT_CLK_OUT_EN)
        reg &= ~PHYCR2_CLK_OUT_DIS;
    else
        reg |= PHYCR2_CLK_OUT_DIS;
    EPLWriteReg( portHandle, PHY_PG0_PHYCR2, reg);
    
    return;
}


//****************************************************************************
EXPORT void
    PTPSetGpioInterruptConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT gpioInt)

//  Configures the GPIO pin to be used for the PTP Interrupt function.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  gpioInt
//      Specifies the number of the GPIO to assign to the PTP Interrupt. 
//      This is a value from 1 - 12. Specifying 0 disables interrupt / GPIO 
//      assignment.
//
//  Returns
//      Nothing
//****************************************************************************
{
    EPLWriteReg( portHandle, PHY_PG6_PTP_INTCTL, gpioInt);
    return;
}
 

//****************************************************************************
EXPORT void
    PTPSetMiscConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT ptpEtherType,
        IN NS_UINT ptpOffset,
        IN NS_UINT txSfdGpio,
        IN NS_UINT rxSfdGpio)

//  Sets various miscellaneous PTP configuration options.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  ptpEtherType
//      This parameter defines the Ethernet Type field used to detect PTP 
//      messages transported over Ethernet layer 2. Normally this would be set 
//      to 0xF788.
//  ptpOffset
//      This parameter defines the offset in bytes to the PTP Message from 
//      the preceding header. For Layer2, this is the offset from the Ethernet 
//      Type Field. For IP/UDP, it is the offset from the end of the UDP 
//      Header. Values are from 0x00 - 0xFF.
//  txSfdGpio
//      Tx Start of Frame GPIO Select: This parameter specifies the GPIO 
//      output to which the Tx SFD signal is assigned. Valid values are 0 
//      (disabled) or 1-12.
//  rxSfdGpio
//      Rx Start of Frame GPIO Select: This parameter specifies the GPIO 
//      output to which the Rx SFD signal is assigned. Valid values are 0 
//      (disabled) or 1-12.
//
//  Returns
//      Nothing
//****************************************************************************
{
    EPLWriteReg( portHandle, PHY_PG6_PTP_ETR, ptpEtherType);
    EPLWriteReg( portHandle, PHY_PG6_PTP_OFF, ptpOffset);
    EPLWriteReg( portHandle, PHY_PG6_PTP_SFDCFG, 
                 (txSfdGpio << P640_TX_SFD_GPIO_SHIFT) | 
                 (rxSfdGpio << P640_RX_SFD_GPIO_SHIFT));
    return;
}
 

//****************************************************************************
EXPORT void
    PTPClockReadCurrent (
        IN PEPL_PORT_HANDLE portHandle,
        IN OUT NS_UINT32 *retNumberOfSeconds,
        IN OUT NS_UINT32 *retNumberOfNanoSeconds)

//  Returns a snapshot of the current IEEE 1588 clock value.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  retNumberOfSeconds
//      Will be set on return to the number of seconds comprising the IEEE 
//      1588 hardware clock.
//  retNumberOfNanoSeconds
//      Will be set on return to the number of nanoseconds comprising the 
//      IEEE 1588 hardware clock. This value cannot be larger then 
//      1e9 (1 second).
//
//  Returns
//      Nothing
//****************************************************************************
{
    OAIBeginMultiCriticalSection( portHandle->oaiDevHandle);
    EPLWriteReg( portHandle, PHY_PG4_PTP_CTL, P640_PTP_RD_CLK);
    
    *retNumberOfNanoSeconds  = EPLReadReg( portHandle, PHY_PG4_PTP_TDR);
    *retNumberOfNanoSeconds |= EPLReadReg( portHandle, PHY_PG4_PTP_TDR) << 16;
    *retNumberOfSeconds  = EPLReadReg( portHandle, PHY_PG4_PTP_TDR);
    *retNumberOfSeconds |= EPLReadReg( portHandle, PHY_PG4_PTP_TDR) << 16;
    OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
    return;
}
 

//****************************************************************************
EXPORT void
    PTPClockStepAdjustment (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT32 numberOfSeconds,
        IN NS_UINT32 numberOfNanoSeconds,
        IN NS_BOOL negativeAdj)

//  A step adjustment value is added to the current IEEE 1588 hardware clock 
//  value. Note that the adjustment value can be positive or negative.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  numberOfSeconds
//      The number of seconds to add to the current clock value. This should 
//      always be a positive value, use the negativeAdj flag to indicate a 
//      negative overall value.
//  numberOfNanoSeconds
//      The number of nanoseconds to add to the current clock value. This 
//      should always be a positive value, use the negativeAdj flag to 
//      indicate a negative overall value. This value must NOT be larger then 
//      10^9 (1 second).
//  negativeAdj
//      If TRUE, the numberOfSeconds and numberOfNanoSeconds values will be 
//      subtracted from the current clock value, otherwise the values will be 
//      an added to the clock. 
//
//  Returns
//      Nothing
//
//  This function accounts for the underlying time required within the 
//  hardware to make the adjustment (2 clock periods - 16ns).
//****************************************************************************
{
    if ( negativeAdj)
    {
        // Convert to 2's complement
        numberOfSeconds = (0xFFFFFFFF - numberOfSeconds + 1) & 0xFFFFFFFF;
        numberOfNanoSeconds = (0xFFFFFFFF - numberOfNanoSeconds + 1) & 0xFFFFFFFF;
    }
        
    OAIBeginMultiCriticalSection( portHandle->oaiDevHandle);
    
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, numberOfNanoSeconds & 0xFFFF);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, numberOfNanoSeconds >> 16);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, numberOfSeconds & 0xFFFF);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, numberOfSeconds >> 16);
    EPLWriteReg( portHandle, PHY_PG4_PTP_CTL, P640_PTP_STEP_CLK);
    OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
    return;

}
 

//****************************************************************************
EXPORT void
    PTPClockSet (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT32 numberOfSeconds,
        IN NS_UINT32 numberOfNanoSeconds)

//  Sets the IEEE 1588 hardware clock equal to the specified time value.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  numberOfSeconds
//      The number of seconds to set the current clock value too.
//  numberOfNanoSeconds
//      The number of nanoseconds to set the current clock value too. This 
//      value must NOT be larger then 10^9 (1 second).
//
//  Returns
//      Nothing
//****************************************************************************
{
    OAIBeginMultiCriticalSection( portHandle->oaiDevHandle);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, numberOfNanoSeconds & 0xFFFF);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, numberOfNanoSeconds >> 16);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, numberOfSeconds & 0xFFFF);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, numberOfSeconds >> 16);
    EPLWriteReg( portHandle, PHY_PG4_PTP_CTL, P640_PTP_LOAD_CLK);
    OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
    return;
}
 

//****************************************************************************
EXPORT void
    PTPClockSetRateAdjustment (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT32 rateAdjValue,
        IN NS_BOOL tempAdjFlag,
        IN NS_BOOL adjDirectionFlag)

//  The clock can be programmed to operate at an adjusted frequency value by 
//  programming a rate adjustment value. The rate adjustment allows for 
//  correction on the order of 2-32ns per reference clock cycle. The frequency 
//  adjustment will allow the clock to correct the offset over time, avoiding 
//  any potential side-effects caused by a step adjustment in the time value. 
//  The rate adjustment can be the normal adjustment rate that will always be 
//  used, and a temporary rate adjustment can be specified that will only occur 
//  for a preprogrammed amount of time.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  rateAdjValue
//      This 26-bit magnitude is the number of 2^-32 ns units that should be 
//      added to or subtracted from the IEEE 1588 hardware clock per reference 
//      cycle (8ns).
//  tempAdjFlag
//      If set to TRUE the set rate will override any previously set normal 
//      adjustment value for the amount of time specified using the 
//      PTPSetTempRateDurationConfig() function.  If this is FALSE, this 
//      function sets the normal clock adjustment time.
//  adjDirectionFlag
//      If set to TRUE, this will cause the PTP Clock to operate at a lower
//      frequency than the reference. The rateAdjValue value will be 
//      subtracted from the clock on every cycle. If set to FALSE, the 
//      rateAdjValue will be added to the clock on every cycle thus causing 
//      the IEEE 1588 hardware clock to operate at a higher frequency.
//
//  Returns
//      Nothing
//  
//  This function allows setting two different rate adjustment values, a 
//  Normal Rate and a Temporary Rate. The Temporary Rate allows the 1588 
//  Clock to operate at a modified rate for a programmed amount of time, 
//  as defined with PTPSetTempRateDurationConfig(). The Normal Rate will 
//  be selected and used if a Temporary Rate is not currently active.
//  
//  When setting a rate, the tempAdjFlag parameter indicates the rate is to
//  be temporary. Following completion of the time duration the rate will 
//  revert back to the Normal Rate. Note that the Normal Rate may be changed 
//  while a Temporary Rate is active. This will have not effect on the 
//  Temporary Rate, but the new Normal Rate will be used when the Temporary 
//  Rate Duration completes.
//  
//  To adjust the time value, software uses the 1588 protocol to determine 
//  the time correction required. The time correction may be spread over 
//  multiple clock cycles by programming a Temporary Rate value. To determine 
//  the rate setting, software should compute the rate difference as the time 
//  correction divided by the time duration in number of 8ns clock cycles. 
//  This value should be multiplied by 2^32 to convert to the correct units. 
//  This rate difference should then be added to the current PTP Rate setting 
//  to provide the Temporary Rate.
//  
//  The Temporary Rate value is a 26-bit value plus a sign bit (adjDirectionFlag), 
//  providing a range of -(2^26-1) to +(2^26-1) in units of 2^-32ns/cycle. 
//  Since each reference clock cycle is 8ns, this allows for a rate adjustment 
//  maximum of approximately +/-1950ppm.
//  
//  The Temporary Rate duration is a 26-bit value providing a duration of up 
//  to 536ms.
//  
//  Example:
//      Conditions:
//          Current Rate = +10 ppm, which gives PTP_Rate = 343597 (2^-32ns/cycle)
//          Time_Error = 20ns, gives Time_Corr = -20ns
//          Assume the correction will be done over 1ms, gives
//          Temp_Rate_duration = 1ms/8ns = 125,000
//  
//      Calculation:
//          Temp_Rate_delta = (Time_Corr/Temp_Rate_duration) * 2^32
//                  = (-20/125000) * 2^32 = -687194
//          Temp_Rate = Current_Rate + Temp_Rate_delta = 343597 + -687194 = -343597
//****************************************************************************
{
NS_UINT reg;

    reg = (rateAdjValue >> P640_PTP_RATE_HI_SHIFT) & P640_PTP_RATE_HI_MASK;
    if ( tempAdjFlag) reg |= P640_PTP_TMP_RATE;
    if ( adjDirectionFlag) reg |= P640_PTP_RATE_DIR;
    
    OAIBeginMultiCriticalSection( portHandle->oaiDevHandle);

//	Get_MII_Resource();
    EPLWriteReg( portHandle, PHY_PG4_PTP_RATEH, reg);
    EPLWriteReg( portHandle, PHY_PG4_PTP_RATEL, rateAdjValue & 0xFFFF);
//	Release_MII_Resource();
    OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
    return;
}
 

//****************************************************************************
EXPORT NS_UINT
    PTPCheckForEvents (
        IN PEPL_PORT_HANDLE portHandle)

//  Checks to determine if any of the following hardware events are 
//  outstanding: Transmit timestamp, receive timestamp, trigger done and event 
//  timestamp.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//
//  Returns
//      A bit map of zero or more bits set indicating the types of events that 
//      are available from the hardware. The defined bits are:
//          PTPEVT_TRANSMIT_TIMESTAMP_BIT
//          PTPEVT_RECEIVE_TIMESTAMP_BIT
//          PTPEVT_EVENT_TIMESTAMP_BIT
//          PTPEVT_TRIGGER_DONE_BIT
//
//  This must be called prior to retrieving individual events from the 
//  hardware. The bit map must be used to determine which "Get" functions 
//  need to be called. The applicable "Get" functions are:
//
//      PTPGetTransmitTimestamp
//      PTPGetReceiveTimestamp
//      PTPGetEvent
//      PTPHasTriggerExpired
//
//  It is NOT necessary to call this function prior to calling 
//  PTPHasTriggerExpired, although this function can be useful in a main 
//  polling loop to quickly determine if there is an expired trigger as well 
//  as the other types of events with a single call.
//****************************************************************************
{
NS_UINT reg, eventFlags;

    reg = EPLReadReg( portHandle, PHY_PG4_PTP_STS);
    eventFlags = 0;
    if ( reg & P640_TXTS_RDY) eventFlags |= PTPEVT_TRANSMIT_TIMESTAMP_BIT;
    if ( reg & P640_RXTS_RDY) eventFlags |= PTPEVT_RECEIVE_TIMESTAMP_BIT;
    if ( reg & P640_TRIG_DONE) eventFlags |= PTPEVT_TRIGGER_DONE_BIT;
    if ( reg & P640_EVENT_RDY) eventFlags |= PTPEVT_EVENT_TIMESTAMP_BIT;
    return eventFlags;
}
 

//****************************************************************************
EXPORT void
    PTPGetTransmitTimestamp (
        IN PEPL_PORT_HANDLE portHandle,
        IN OUT NS_UINT32 *retNumberOfSeconds,
        IN OUT NS_UINT32 *retNumberOfNanoSeconds,
        IN OUT NS_UINT *overflowCount)

//  Returns the next available transmit timestamp.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function. 
//  retNumberOfSeconds
//      Will be set on return to the next transmit's seconds timestamp value.
//  retNumberOfNanoSeconds
//      Will be set on return to the next transmit's nanoseconds timestamp 
//      value. This value will never be larger then 10^9 (1 second).
//  overflowCount
//      Will be set on return and indicates if timestamps were dropped due to 
//      an overflow of the Transmit Timestamp queue. The overflow counter 
//      will stick at a value of three if more then three timestamps were 
//      missed. Normally this value should be 0.
//
//  Returns
//      Nothing
//  
//  The caller must have previously called PTPCheckForEvents() and determined 
//  that the PTPEVT_TRANSMIT_TIMESTAMP_BIT bit was set prior to invoking this 
//  function. This function does NOT check to determine if an outstanding 
//  transmit event is available.
//  
//  The hardware can queue up to four transmit timestamps. If more then four 
//  transmits occur without first reading the transmit timeout, then the 
//  overflowCount will indicate (up to 3) the number of timestamps that were 
//  dropped due to overflow.
//  
//  This function will make the necessary adjustment to the timestamp to 
//  account for delay from the timestamp point to the wire (approximately 
//  6 bit times).
//****************************************************************************
{
NS_UINT reg;

    OAIBeginMultiCriticalSection( portHandle->oaiDevHandle);
    *retNumberOfNanoSeconds = EPLReadReg( portHandle, PHY_PG4_PTP_TXTS);
    reg = EPLReadReg( portHandle, PHY_PG4_PTP_TXTS);
    *overflowCount = (reg & 0xC000) >> 14;
    *retNumberOfNanoSeconds |= (reg & 0x3FFF) << 16;
    *retNumberOfSeconds = EPLReadReg( portHandle, PHY_PG4_PTP_TXTS);
    *retNumberOfSeconds |= EPLReadReg( portHandle, PHY_PG4_PTP_TXTS) << 16;
    OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
    return;
}
 

//****************************************************************************
EXPORT void
    PTPGetReceiveTimestamp (
        IN PEPL_PORT_HANDLE portHandle,
        IN OUT NS_UINT32 *retNumberOfSeconds,
        IN OUT NS_UINT32 *retNumberOfNanoSeconds,
        IN OUT NS_UINT *overflowCount,
        IN OUT NS_UINT *sequenceId,
        IN OUT NS_UINT *messageType,
        IN OUT NS_UINT *hashValue)
        
//  Returns the next available receive timestamp from the device's receive 
//  timestamp queue.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  retNumberOfSeconds
//      Will be set on return to the next receive's seconds timestamp value.
//  retNumberOfNanoSeconds
//      Will be set on return to the next receive's nanoseconds timestamp 
//      value. This value will never be larger then 1e9 (1 second).
//  overflowCount
//      Will be set on return and indicates if timestamps were dropped due to 
//      an overflow of the Receive Timestamp queue. The overflow counter 
//      will stick at a value of three if more then three timestamps were 
//      missed. Normally this value should be 0.
//  sequenceId
//      Will be set on return to the 16-bit SequenceId field from the incoming 
//      PTP frame.
//  messageType
//      Will be set on return to the messageType field from the incoming PTP 
//      frame. For version 1 of the IEEE 1588 specification the Timestamp unit 
//      will return the lower four bits of the control field (octet 32 in the 
//      message). Otherwise, the Timestamp unit will record the version 2 
//      messageType field which is the least significant bits of the first 
//      octet in the PTP message.
//  hashValue
//      Will be set on return to a 12-bit hash value on octets 20-29 of the 
//      PTP event message. For version 1 of the IEEE 1588 specification, 
//      this corresponds to the messageType, sourceCommunicationTechnology, 
//      sourceUuid, and sourcePortId fields. For version 2 of the IEEE 1588 
//      specification, this corresponds to the 10-octet sourcePortIdentity 
//      field. The combination of hash value and sequenceId, allows software 
//      to correctly match a Timestamp with the correct receive event message.
//
//      The hash algorithm used is the CRC function as defined in section 
//      3.2.8 the IEEE 802.3 specification. The Timestamp unit returns the 
//      12 most significant bits as the CRC computation (the resultant bits 
//      are not complemented as done in the 802.3 CRC generation).
//
//      To minimize unnecessary timestamp capture, the device may 
//      be configured to filter based on the source identification hash value.
//      This value may be programmed using the PTPSetMiscConfig() call with 
//      relevance to the ptpRxHash parameter. If the source hash value for 
//      the incoming PTP event message does not match the programmed hash 
//      value, then the message will not be timestamped.
//
//  Returns
//      Nothing
//  
//  The caller must have previously called PTPCheckForEvents() and determined 
//  that the PTPEVT_RECEIVE_TIMESTAMP_BIT bit was set prior to invoking this 
//  function. This function does NOT check to determine if an outstanding 
//  receive event is available.
//  
//  The hardware can queue up to four receive timestamps.
//  
//  This function will make the necessary adjustment to the timestamp to 
//  account for delay from the wire to the timestamp point (approximately 
//  26 bit times).
//****************************************************************************
{
NS_UINT reg;

    OAIBeginMultiCriticalSection( portHandle->oaiDevHandle);
    *retNumberOfNanoSeconds = EPLReadReg( portHandle, PHY_PG4_PTP_RXTS);
    reg = EPLReadReg( portHandle, PHY_PG4_PTP_RXTS);
    *overflowCount = (reg & 0xC000) >> 14;
    *retNumberOfNanoSeconds |= (reg & 0x3FFF) << 16;
    *retNumberOfSeconds = EPLReadReg( portHandle, PHY_PG4_PTP_RXTS);
    *retNumberOfSeconds |= EPLReadReg( portHandle, PHY_PG4_PTP_RXTS) << 16;
    *sequenceId = EPLReadReg( portHandle, PHY_PG4_PTP_RXTS);
    reg = EPLReadReg( portHandle, PHY_PG4_PTP_RXTS);
    OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
    
    *messageType = reg >> 12;
    *hashValue = reg & 0x0FFF;

	// TBD: Adjust by 26 bit times.
    return;
}
 

//****************************************************************************
EXPORT void
    PTPGetTimestampFromFrame (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT8 *receiveFrameData,
        IN OUT NS_UINT32 *retNumberOfSeconds,
        IN OUT NS_UINT32 *retNumberOfNanoSeconds)

//  Extracts the embedded receive timestamp from a received PTP frame.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  receiveFrameData
//      Points to the start of the PTP header.
//  retNumberOfSeconds
//      Will be set on return to the next receive's seconds timestamp value. 
//      The number of significant bits of magnitude returned is determined by 
//      the RXOPT_TS_SEC_EN option bit used during the call to 
//      PSPSetReceiveConfig().
//  retNumberOfNanoSeconds
//      Will be set on return to the next receive's nanoseconds timestamp 
//      value. This value will never be larger then 10^9 (1 second).
//
//  Returns
//      Nothing
//  
//  This function can be used when receive timestamp insertion has been 
//  enabled using the PSPSetReceiveConfig() function with the RXOPT_TS_INSERT 
//  option set. It extracts the embedded timestamp from the frame, either from 
//  the end of the frame if RXOPT_TS_APPEND is enabled, or from the configured 
//  locations within the PTP frame itself. 
//  
//  If RXOPT_TS_APPEND is disabled, the relevant reserved fields are set to a 
//  value of 0.
//  
//  This function will make the necessary adjustment to the timestamp to 
//  account for delay from the wire to the timestamp point (approximately 
//  26 bit times).
//****************************************************************************
{
PPORT_OBJ portHdl = (PPORT_OBJ)portHandle;
NS_UINT8 *nanoField, *secField;
NS_UINT opts;

    opts = portHdl->rxConfigOptions;
    if ( !(opts & RXOPT_TS_INSERT))
        return;

    if ( opts & RXOPT_TS_APPEND)
    {
        // Timestamp is located at the end of the PTP packet at the configured
        // offsets from the end of the PTP message.
        nanoField = &receiveFrameData[ PTP_EVENT_PACKET_LENGTH + portHdl->rxTsNanoSecOffset ];
        secField = &nanoField[ portHdl->rxTsSecondsOffset ];
    }
    else
    {
        // Timestamp is located at the configured offsets from the start of the
        // PTP message.
        nanoField = &receiveFrameData[ portHdl->rxTsNanoSecOffset ];
        secField = &receiveFrameData[ portHdl->rxTsSecondsOffset ];
    }

            
    *retNumberOfNanoSeconds = *(NS_UINT32*)nanoField;
    *(NS_UINT32*)nanoField = 0L;
    
    if ( portHdl->tsSecondsLen == 0)
    {
        *retNumberOfSeconds = (NS_UINT32)*secField;
        *secField = 0;
    }
    else if ( portHdl->tsSecondsLen == 1)
    {
        *retNumberOfSeconds = (NS_UINT32)*(NS_UINT16*)secField;
        *(NS_UINT16*)secField = 0;
    }
    else if ( portHdl->tsSecondsLen == 2)
    {
        *retNumberOfSeconds = 0x00FFFFFF & *(NS_UINT32*)secField;
        *(NS_UINT16*)secField = 0;
        secField[2] = 0;
    }
    else if ( portHdl->tsSecondsLen == 4)
    {
        *retNumberOfSeconds = *(NS_UINT32*)secField;
        *(NS_UINT32*)secField = 0;
    }
   
    // Adj by 26 bit times
    *retNumberOfNanoSeconds += 26 * 10;
#if 0
    if ( *retNumberOfNanoSeconds >= 1e9)
    {

        *retNumberOfSeconds += 1;
        *retNumberOfNanoSeconds -= (NS_UINT)1e9;
    }
#endif
    
    return;
}
 

//****************************************************************************
EXPORT void
    PTPArmTrigger (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT trigger,
        IN NS_UINT32 expireTimeSeconds,
        IN NS_UINT32 expireTimeNanoSeconds,
        IN NS_BOOL initialStateFlag,
        IN NS_BOOL waitForRolloverFlag,
        IN NS_UINT32 pulseWidth,
        IN NS_UINT32 pulseWidth2)

//  Establishes a trigger's expiration time and trigger pulse width behavior.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  trigger
//      The trigger to arm, 0 - 7.
//  expireTimeSeconds
//      The seconds portion of the desired expiration time relative to the 
//      IEEE 1588 hardware clock. 
//  expireTimeNanoSeconds
//      The nanoseconds portion of the desired expiration time relative to 
//      the IEEE 1588 hardware clock. This value may be not be larger then 
//      1e9 (1 second).
//  initialStateFlag
//      Indicates initial state of signal to be set when trigger is armed 
//      (FALSE will cause a signal rise at trigger time, TRUE will cause a 
//      signal fall at trigger time). This parameter is ignored in toggle 
//      mode.
//  waitForRolloverFlag
//      If set to TRUE the device should not arm the trigger until after 
//      the seconds field of the clock time has rolled over from 
//      0xFFFF_FFFF to 0.
//  pulseWidth
//      Sets the 50% duty cycle time for triggers 2 - 7. Its format is 
//      bits [31:30] = seconds, [29:0] = nanoseconds. For triggers 0 and 1 
//      sets the width of the first part of the pulse, the width of the 
//      second part of the pulse is set by the pulseWidth2 parameter.
//  pulseWidth2
//      Ignored for triggers 2 - 7. For Triggers 0 and 1, in a single or 
//      periodic pulse type signal, a second pulse width value controls the 
//      2nd pulse width (period is pulseWidth + pulseWidth2). 
//      Its format is bits [31:30] = seconds, [29:0] = nanoseconds.
//
//      For Edge type signals, pulseWidth2 is interpreted as a 16-bit seconds 
//      field and pulseWidth is a 30-bit nanoseconds field. 
//
//  Returns
//      Nothing
//
//  Once this function has been called, the trigger will be armed. 
//****************************************************************************
{
NS_UINT reg;

  OAIBeginMultiCriticalSection( portHandle->oaiDevHandle);
    reg = (trigger << P640_TRIG_SEL_SHIFT) | P640_TRIG_LOAD;
//	Get_MII_Resource();
    EPLWriteReg( portHandle, PHY_PG4_PTP_CTL, reg);
    
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, expireTimeNanoSeconds & 0xFFFF);
    
    reg = (expireTimeNanoSeconds >> 16) | (initialStateFlag ? 0x80000000 : 0) |
          (waitForRolloverFlag ? 0x40000000 : 0);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, reg);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, expireTimeSeconds & 0xFFFF);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, expireTimeSeconds >> 16);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, pulseWidth & 0xFFFF);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, pulseWidth >> 16);
    
    if ( trigger <= 1)
    {
        EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, pulseWidth2 & 0xFFFF);
        EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, pulseWidth2 >> 16);
    }
    
    reg = (trigger << P640_TRIG_SEL_SHIFT) | P640_TRIG_EN;
    EPLWriteReg( portHandle, PHY_PG4_PTP_CTL, reg);
//	Release_MII_Resource();
    OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
    return;
}

EXPORT void
    PTPArmTrigger_ns (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT trigger,
        IN NS_UINT32 expireTimeNanoSeconds)

//  Establishes a trigger's expiration time and trigger pulse width behavior.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  trigger
//      The trigger to arm, 0 - 7.
//  expireTimeNanoSeconds
//      The nanoseconds portion of the desired expiration time relative to 
//      the IEEE 1588 hardware clock. This value may be not be larger then 
//      1e9 (1 second).
//
//  Returns
//      Nothing
//
//  Once this function has been called, the trigger will be armed. 
//****************************************************************************
{
NS_UINT reg;

  OAIBeginMultiCriticalSection( portHandle->oaiDevHandle);
    reg = (trigger << P640_TRIG_SEL_SHIFT) | P640_TRIG_LOAD;
//	Get_MII_Resource();
    EPLWriteReg( portHandle, PHY_PG4_PTP_CTL, reg);
    
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, expireTimeNanoSeconds & 0xFFFF);
    
    reg = (expireTimeNanoSeconds >> 16);
          
#if 0
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, reg);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, expireTimeSeconds & 0xFFFF);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, expireTimeSeconds >> 16);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, pulseWidth & 0xFFFF);
    EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, pulseWidth >> 16);
    
    if ( trigger <= 1)
    {
        EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, pulseWidth2 & 0xFFFF);
        EPLWriteReg( portHandle, PHY_PG4_PTP_TDR, pulseWidth2 >> 16);
    }
#endif
    
    reg = (trigger << P640_TRIG_SEL_SHIFT) | P640_TRIG_EN;
    EPLWriteReg( portHandle, PHY_PG4_PTP_CTL, reg);
//	Release_MII_Resource();
    OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
    return;
}

//****************************************************************************
EXPORT NS_STATUS
    PTPHasTriggerExpired (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT trigger)

//  Determines if a particular trigger has occurred or if an error has 
//  occurred. An interrupt mechanism may also be used to detect trigger 
//  expiration.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  trigger
//      The trigger to check, 0 - 7.
//
//  Returns
//      NS_STATUS_SUCCESS
//          The specified trigger has expired successfully.
//      NS_STATUS_FAILURE
//          The specified trigger has not yet expired.
//      NS_STATUS_INVALID_PARM
//          The specified trigger was armed late and expired. For a periodic 
//          signal, if the Trigger-if-late control is set, this function will 
//          return NS_STATUS_SUCCESS. If the Trigger-if-late is not set, this 
//          status return code will be returned.
//
//  It is NOT necessary to call PTPCheckForEvents() prior to invocation.
//****************************************************************************
{
NS_UINT reg, trgBit;

    OAIBeginMultiCriticalSection( portHandle->oaiDevHandle);
    reg = EPLReadReg( portHandle, PHY_PG4_PTP_TSTS);
    OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
    
    trgBit = 1 << (trigger * 2);
    if ( reg & trgBit)
        return NS_STATUS_FAILURE;
    if ( reg & (trgBit << 1))
        return NS_STATUS_INVALID_PARM;
    return NS_STATUS_SUCCESS;
}
 

//****************************************************************************
EXPORT void
    PTPCancelTrigger (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT trigger)

//  Cancels a previously schedule trigger event (if active).
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  trigger
//      The trigger to cancel, 0 - 7.
//
//  Returns
//      Nothing
//  
//  It is possible that a trigger will expire before this function can cancel 
//  it, depending on how close the expiration time is to the current IEEE 
//  1588 time.
//****************************************************************************
{
NS_UINT reg;

    reg = (trigger << P640_TRIG_SEL_SHIFT) | P640_TRIG_DIS;
    OAIBeginMultiCriticalSection( portHandle->oaiDevHandle);
    EPLWriteReg( portHandle, PHY_PG4_PTP_CTL, reg);
    OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
    
    return;
}
 

//****************************************************************************
EXPORT NS_BOOL
    PTPGetEvent (
        IN PEPL_PORT_HANDLE portHandle,
        IN OUT NS_UINT *eventBits,
        IN OUT NS_UINT *riseFlags,
        IN OUT NS_UINT32 *eventTimeSeconds,
        IN OUT NS_UINT32 *eventTimeNanoSeconds,
        IN OUT NS_UINT *eventsMissed)
        
//  Returns the available asynchronous event timestamps.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  eventBits
//      Set on return to a bit map of events that have occurred. Bit 0 is 
//      event 0, etc. 0 or more bits may be set.
//  riseFlags
//      Set on return to a bit map indicating if an event occurred on the 
//      rising edge (set to 1), or on the falling edge (set to 0). Bit 0 is
//      is the rising edge flag for event 0, etc.
//  eventTimeSeconds
//      The seconds portion of the IEEE 1588 clock timestamp when this event 
//      occurred. 
//  eventTimeNanoSeconds
//      The nanosecond portion of the IEEE 1588 clock timestamp when this 
//      event occurred. This value will not be larger then 1e9 (1 second).
//  eventsMissed
//      Set on return to indicate the number of events that have been missed 
//      prior to this event due to internal event queue overflow. The maximum 
//      value is 7.
//
//  Returns
//      TRUE if an event was returned, FALSE otherwise.
//  
//  The caller must have previously called PTPCheckForEvents() and determined 
//  that the PTPEVT_EVET_TIMESTAMP_BIT bit was set prior to invoking this 
//  function. This function does NOT check to determine if an outstanding 
//  event is available.
//  
//  This function properly tracks and handles events that occur at the same 
//  exact time. It also adjusts the timestamp values to compensate for input 
//  path and synchronization delays.
//****************************************************************************
{
NS_UINT reg, exSts, x;

    *eventBits = 0;
    *riseFlags = 0;

    OAIBeginMultiCriticalSection( portHandle->oaiDevHandle);
    reg = EPLReadReg( portHandle, PHY_PG4_PTP_ESTS);
    
    *eventsMissed = (reg & P640_EVNTS_MISSED_MASK) >> P640_EVNTS_MISSED_SHIFT;
    
    if ( !(reg & P640_EVENT_DET))
    {
        OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
        return FALSE;
    }
        
    if ( reg & P640_MULT_EVENT)
    {
        exSts = EPLReadReg( portHandle, PHY_PG4_PTP_EDATA);
        
        for ( x = 8; x; x--)
        {
            if ( exSts & 0x40)
                *eventBits |= 1 << (x-1);
                if ( exSts & 0x80)
                    *riseFlags |= 1 << (x-1);    
            exSts <<= 2;
        }
    }
    else    
    {
        *eventBits |= 1 << ((reg & P640_EVNT_NUM_MASK) >> P640_EVNT_NUM_SHIFT);
        *riseFlags |= ((reg & P640_EVNT_RF) ? 1 : 0) << ((reg & P640_EVNT_NUM_MASK) >> P640_EVNT_NUM_SHIFT);
    }
    
    *eventTimeNanoSeconds = EPLReadReg( portHandle, PHY_PG4_PTP_EDATA);
    *eventTimeNanoSeconds |= EPLReadReg( portHandle, PHY_PG4_PTP_EDATA) << 16;
    *eventTimeSeconds = EPLReadReg( portHandle, PHY_PG4_PTP_EDATA);
    *eventTimeSeconds |= EPLReadReg( portHandle, PHY_PG4_PTP_EDATA) << 16;
    OAIEndMultiCriticalSection( portHandle->oaiDevHandle);
    
    // Adj for pin input delay and edge detection time (8ns * 3)
    *eventTimeNanoSeconds -= 8 * 3;
    if ( *eventTimeNanoSeconds < 0)
    {
        *eventTimeSeconds -= 1;
        *eventTimeNanoSeconds += (NS_UINT)1e9;
        
        if ( *eventTimeSeconds < 0)
            *eventTimeSeconds = *eventTimeNanoSeconds = 0;
    }
        
    return TRUE;
}
 

//****************************************************************************
EXPORT NS_UINT
    MonitorGpioSignals (
        IN PEPL_PORT_HANDLE portHandle)

//  Provides a function to read the current status of the device's GPIO 
//  signals.
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//
//  Returns
//      Returns bits [11:0] reflecting the current values on the 12 GPIOs. 
//      GPIO 12 is bit 11, etc.
//****************************************************************************
{
    return EPLReadReg( portHandle, PHY_PG6_PTP_GPIOMON);
}
 

//****************************************************************************
EXPORT NS_BOOL
    IsPhyStatusFrame (
		IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT8 *frameBuffer,
        IN NS_UINT frameLength,
        IN OUT NS_UINT8 **nextMsgLocation)

//  Determines if the specified receive frame is a specially formatted IEEE 
//  1588 Phy Status Frame (PSF).
//  
//  portHandle
//      Handle that represents a port. This is obtained using the EPLEnumPort 
//      function.
//  frameBuffer
//      Pointer to a byte array containing a pointer to the incoming IEEE 
//      1588 frame. Where in the frame this points is defined by the 
//      offsetLocation parameter. This should point to the start of the
//      Ethernet frame.
//  frameLength
//      The number of bytes pointed to by frameBuffer.
//  nextMsgLocation
//      Set on return to the offset of the Phy message that should be 
//      processed. This is only set if TRUE is returned by this function.
//
//  Returns
//      Returns TRUE if the frame is a Phy Status Frame message, FALSE 
//      otherwise.
//  
//  This function used the previously configured values for the IEEE 1588 
//  header transportSpecific, messageType and versionPTP to determine if the 
//  frame is a Phy Status Frame. Refer to PTPSetPhyStatusFrameConfig().
//  
//  If the frame is a PSF message, the caller should use the 
//  GetNextPhyMessage() to obtain the message's information.
//****************************************************************************
{
PPORT_OBJ portHdl = (PPORT_OBJ)portHandle;
static NS_UINT8 ipDestAddr[6] = {0x01,0x00,0x5E,0x00,0x01,0x81};
static NS_UINT8 macDestAddr[6] = {0x01,0x1B,0x19,0x00,0x00,0x00};

    if ( portHdl->psfConfigOptions & STSOPT_IPV4)
    {
        // IPv4/UDP Frame
        if ( frameLength < 50)
            return FALSE;
    
        if ( *(NS_UINT32*)&frameBuffer[0] != *(NS_UINT32*)&ipDestAddr[0] || 
             *(NS_UINT16*)&frameBuffer[4] != *(NS_UINT16*)&ipDestAddr[4])
            return FALSE;
            
        if ( *(NS_UINT32*)&frameBuffer[6] != *(NS_UINT32*)&portHdl->psfSrcMacAddr[0] || 
             *(NS_UINT16*)&frameBuffer[10] != *(NS_UINT16*)&portHdl->psfSrcMacAddr[4])
            return FALSE;
            
        if ( frameBuffer[12] != 0x08 || frameBuffer[13] != 0x00)
            return FALSE;    
            
        *nextMsgLocation = &frameBuffer[44];
    }
    else
    {
        // Layer 2 Ethernet Frame
        if ( frameLength < 22)
            return FALSE;
    
        if ( *(NS_UINT32*)&frameBuffer[0] != *(NS_UINT32*)&macDestAddr[0] || 
             *(NS_UINT16*)&frameBuffer[4] != *(NS_UINT16*)&macDestAddr[4])
            return FALSE;
            
        if ( *(NS_UINT32*)&frameBuffer[6] != *(NS_UINT32*)&portHdl->psfSrcMacAddr[0] || 
             *(NS_UINT16*)&frameBuffer[10] != *(NS_UINT16*)&portHdl->psfSrcMacAddr[4])
            return FALSE;
            
        if ( frameBuffer[12] != 0x88 || frameBuffer[13] != 0xF7)
            return FALSE;
            
        *nextMsgLocation = &frameBuffer[16];
    }

    return TRUE;    
}
 

//****************************************************************************
EXPORT NS_BOOL
    GetNextPhyMessage (
        IN OUT NS_UINT8 **nextMsgLocation,
        IN OUT PHYMSG_MESSAGE_TYPE_ENUM *messageType,
        IN OUT PHYMSG_MESSAGE *message)

//  Returns information about the next Phy message contained in a list of one 
//  or more Phy message structures.
//  
//  nextMsgLocation
//      Set by the caller to previously determine next message location offset. 
//      This is initially determined by calling IsPhyStatusFrame(). It is 
//      updated on return to point to the next message location (if any). The 
//      caller should treat this field opaquely.
//  messageType
//      Set on return to one of the PHYMSG_MESSAGE_TYPE_ENUM values indicating 
//      the type of message structure returned in the message union buffer.
//  message
//      Caller allocated union/structure that will be filled out on return to 
//      contain the relevant message information fields.
//
//  Returns
//      Returns TRUE if a message was successfully returned in the message 
//      union/structure, otherwise FALSE indicates no more messages exist.
//****************************************************************************
{
NS_UINT16 val, stsType;
NS_UINT16 *ptr;
NS_UINT8 *msg;

    msg = *nextMsgLocation;
    
    if ( *(NS_UINT32*)msg == 0x00000000)
        return FALSE;
        
    stsType = *(NS_UINT16*)msg;
        
    switch ( stsType & 0xF800)
    {
        case 0x1000:
            // Tx timestamp message
            *messageType = PHYMSG_STATUS_TX;
            message->TxStatus.txTimestampSecs = *(NS_UINT16*)&msg[6];
            message->TxStatus.txTimestampSecs |= *(NS_UINT16*)&msg[8] << 16;
            message->TxStatus.txTimestampNanoSecs = *(NS_UINT16*)&msg[2];
            val = *(NS_UINT16*)&msg[4];
            message->TxStatus.txTimestampNanoSecs |= (val & ~0xC000) << 16;
            message->TxStatus.txOverflowCount = val >> 14;
            *nextMsgLocation += 5 * sizeof( NS_UINT16);
            break;
        
        case 0x2000:
            // Rx timestamp message
            *messageType = PHYMSG_STATUS_RX;
           
            message->RxStatus.rxTimestampSecs = *(NS_UINT16*)&msg[6];
            message->RxStatus.rxTimestampSecs |= *(NS_UINT16*)&msg[8] << 16;
            message->RxStatus.rxTimestampNanoSecs = *(NS_UINT16*)&msg[2];
            val = *(NS_UINT16*)&msg[4];
            message->RxStatus.rxTimestampNanoSecs |= (val & ~0xC000) << 16;
            message->RxStatus.rxOverflowCount = val >> 14;
         
            message->RxStatus.sequenceId = *(NS_UINT16*)&msg[10];
            message->RxStatus.messageType = *(NS_UINT16*)&msg[12] >> 12;
            message->RxStatus.sourceHash = *(NS_UINT16*)&msg[12] & 0x0FFF;
            *nextMsgLocation += 7 * sizeof( NS_UINT16);
            break;
        
        case 0x3000:
            // Trigger message
            *messageType = PHYMSG_STATUS_TRIGGER;
            message->TriggerStatus.triggerStatus = *(NS_UINT16*)&msg[2];
            *nextMsgLocation += 2 * sizeof( NS_UINT16);
            break;
            
        case 0x4000:
            // Event timestamp message
            *messageType = PHYMSG_STATUS_EVENT;
            message->EventStatus.ptpEstsRegBits = stsType & 0x0FFF;
            message->EventStatus.extendedEventStatusFlag = FALSE;
            
            val = (message->EventStatus.ptpEstsRegBits & P640_EVNTS_TS_LEN_MASK) >> P640_EVNTS_TS_LEN_SHIFT;
            
            if ( message->EventStatus.ptpEstsRegBits & P640_MULT_EVENT)
            {
                ptr = (NS_UINT16*)&msg[4];
                message->EventStatus.extendedEventStatusFlag = TRUE;
                message->EventStatus.extendedEventInfo = *(NS_UINT16*)&msg[2];
            }
            else
                ptr = (NS_UINT16*)&msg[2];
            
            message->EventStatus.evtTimestampSecs = 0;
            message->EventStatus.evtTimestampNanoSecs = 0;
           
            message->EventStatus.evtTimestampNanoSecs = *ptr++;
            if ( val == 1)
                message->EventStatus.evtTimestampNanoSecs |= *ptr++ << 16;
            else if ( val == 2)
            {
                message->EventStatus.evtTimestampNanoSecs |= *ptr++ << 16;
                message->EventStatus.evtTimestampSecs = *ptr++;
            }
            else if ( val == 3)
            {
                message->EventStatus.evtTimestampNanoSecs |= *ptr++ << 16;
                message->EventStatus.evtTimestampSecs = *ptr++;
                message->EventStatus.evtTimestampSecs |= *ptr++ << 16;
            }
                
            *nextMsgLocation = (NS_UINT8*)ptr;
            break;
        
        case 0x5000:
            // Status frame error status
            *messageType = PHYMSG_STATUS_ERROR;
            
            message->ErrorStatus.frameBufOverflowFlag = (*(NS_UINT16*)&msg[0] & 0x0FFF) == 0x00 ? TRUE : FALSE;
            message->ErrorStatus.frameCounterOverflowFlag = (*(NS_UINT16*)&msg[0] & 0x0FFF) == 0x01 ? TRUE : FALSE;
            *nextMsgLocation += 1 * sizeof( NS_UINT16);
            break;
        
        case 0x6000:
            // Register read response
            *messageType = PHYMSG_STATUS_REG_READ;
            
            message->RegReadStatus.regIndex = *(NS_UINT16*)&msg[0] & 0x001F;
            message->RegReadStatus.regPage  = (*(NS_UINT16*)&msg[0] & 0x00E0) >> 5;
            message->RegReadStatus.readRegisterValue = *(NS_UINT16*)&msg[1];
            *nextMsgLocation += 2 * sizeof( NS_UINT16);
            break;

        default:
            return FALSE;    
    }

    return TRUE;
}
 

