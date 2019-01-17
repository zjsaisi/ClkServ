//****************************************************************************
// epl.h
// 
// Copyright (c) 2006 National Semiconductor Corporation.
// All Rights Reserved
// 
// This is the main EPL include (header) file. Include this to pull in 
// all EPL definitions and prototypes.
//****************************************************************************

#ifndef _EPL_INCLUDE
#define _EPL_INCLUDE

// Local include files 
#include "epl_types.h"
#include "epl_regs.h"
#include "oai.h"


// EPL return codes
typedef enum
{
    NS_STATUS_SUCCESS,          // The request operation completed successfully
    NS_STATUS_FAILURE,          // The operation failed 
    NS_STATUS_INVALID_PARM,     // An invalid parameter was detected
    NS_STATUS_RESOURCES,        // Failed to allocate resources required
    NS_STATUS_NOT_SUPPORTED,    // Operation not supported
    NS_STATUS_ABORTED,          // Operation was interrupted before completion
    NS_STATUS_HARDWARE_FAILURE  // Unexpected hardware error encountered
}NS_STATUS;


// EPL Data Structures and Defines

typedef enum {
    EPL_ENUM_MDIO_BIT_BANG,
    EPL_ENUM_DIRECT
}EPL_ENUM_TYPE;

typedef enum {
    EPL_CAPA_NONE = 0x00000000,
    EPL_CAPA_TDR = 0x00000001,
    EPL_CAPA_LINK_QUALITY = 0x00000002,
    EPL_CAPA_MII_PORT_CFG = 0x00000004,
    EPL_CAPA_MII_REG_ACCESS = 0x00000008
}EPL_DEVICE_CAPA_ENUM;

typedef enum {
    MIIPCFG_UNKNOWN,
    MIIPCFG_NORMAL,
    MIIPCFG_PORT_SWAP,
    MIIPCFG_EXT_MEDIA_CONVERTER,
    MIIPCFG_BROADCAST_TX_PORT_A,
    MIIPCFG_BROADCAST_TX_PORT_B,
    MIIPCFG_MIRROR_RX_CHANNEL_A,
    MIIPCFG_MIRROR_RX_CHANNEL_B,
    MIIPCFG_DISABLE_PORT_A,
    MIIPCFG_DISABLE_PORT_B
}EPL_MIICFG_ENUM;

typedef enum {
    DEV_UNKNOWN,
    DEV_DP83848,
    DEV_DP83849,
    DEV_DP83640
}EPL_DEVICE_TYPE_ENUM;

typedef struct EPL_DEV_INFO {
    EPL_DEVICE_TYPE_ENUM deviceType;
    NS_UINT numOfPorts;
    NS_UINT deviceModelNum;
    NS_UINT deviceRevision;
    NS_UINT numExtRegisterPages;
}EPL_DEV_INFO,*PEPL_DEV_INFO;

typedef struct EPL_LINK_STS {
    NS_BOOL linkup;
    NS_BOOL autoNegEnabled;
    NS_BOOL autoNegCompleted;
    NS_UINT speed;
    NS_BOOL duplex;
    NS_BOOL mdixStatus;
    NS_BOOL autoMdixEnabled;
    NS_BOOL polarity;
    NS_BOOL energyDetectPower;
}EPL_LINK_STS,*PEPL_LINK_STS;

typedef enum {
    MDIX_AUTO,
    MDIX_FORCE_NORMAL,
    MDIX_FORCE_SWAP
}EPL_MDIX_ENUM;

typedef struct EPL_LINK_CFG {
    NS_BOOL autoNegEnable;
    NS_UINT speed;
    NS_BOOL duplex;
    NS_BOOL pause;
    EPL_MDIX_ENUM autoMdix;
    NS_BOOL energyDetect;
    NS_UINT energyDetectErrCountThresh;
    NS_UINT energyDetectDataCountThresh;
}EPL_LINK_CFG,*PEPL_LINK_CFG;

typedef enum {
    CABLE_STS_TERMINATED,
    CABLE_STS_OPEN,
    CABLE_STS_SHORT,
    CABLE_STS_CROSS_SHORTED,
    CABLE_STS_UNKNOWN
}EPL_CABLE_STS_ENUM;

#define TDR_CABLE_VELOCITY  4.64

typedef struct DSP_LINK_QUALITY_GET {
    NS_BOOL linkQualityEnabled;
    // The following NS_BOOL fields MUST be in this order.
    NS_BOOL freqCtrlHighWarn;
    NS_BOOL freqCtrlLowWarn;
    NS_BOOL freqOffHighWarn;
    NS_BOOL freqOffLowWarn;
    NS_BOOL dblwHighWarn;
    NS_BOOL dblwLowWarn;
    NS_BOOL dagcHighWarn;
    NS_BOOL dagcLowWarn;
    NS_BOOL c1HighWarn;
    NS_BOOL c1LowWarn;
    // The following NS_UINT fields MUST be in this order.
    NS_SINT freqCtrlHighThresh;
    NS_SINT freqCtrlLowThresh;
    NS_SINT freqOffHighThresh;
    NS_SINT freqOffLowThresh;
    NS_SINT dblwHighThresh;
    NS_SINT dblwLowThresh;
    NS_UINT dagcHighThresh;
    NS_UINT dagcLowThresh;
    NS_SINT c1HighThresh;
    NS_SINT c1LowThresh;
    NS_SINT freqCtrlSample;
    NS_SINT freqOffSample;
    NS_SINT dblwCtrlSample;
    NS_UINT dagcCtrlSample;
    NS_SINT c1CtrlSample;
}DSP_LINK_QUALITY_GET,*PDSP_LINK_QUALITY_GET;

typedef struct DSP_LINK_QUALITY_SET {
    NS_BOOL linkQualityEnabled;
    // The following NS_UINT fields MUST be in this order.
    NS_SINT c1LowThresh;
    NS_SINT c1HighThresh;
    NS_UINT dagcLowThresh;
    NS_UINT dagcHighThresh;
    NS_SINT dblwLowThresh;
    NS_SINT dblwHighThresh;
    NS_SINT freqOffLowThresh;
    NS_SINT freqOffHighThresh;
    NS_SINT freqCtrlLowThresh;
    NS_SINT freqCtrlHighThresh;
}DSP_LINK_QUALITY_SET,*PDSP_LINK_QUALITY_SET;

typedef struct TDR_RUN_REQUEST {
    NS_BOOL sendPairTx;
    NS_BOOL reflectPairTx;
    NS_BOOL use100MbTx;
    NS_UINT txPulseTime;
    NS_BOOL detectPosThreshold;
    NS_UINT rxDiscrimStartTime;
    NS_UINT rxDiscrimStopTime;
    NS_UINT rxThreshold;
}TDR_RUN_REQUEST,*PTDR_RUN_REQUEST;

typedef struct TDR_RUN_RESULTS {
    NS_BOOL thresholdMet;
    NS_UINT thresholdTime;
    NS_UINT thresholdLengthRaw;
    NS_UINT peakValue;
    NS_UINT peakTime;
    NS_UINT peakLengthRaw;
    NS_UINT adjustedPeakLengthRaw;
}TDR_RUN_RESULTS,*PTDR_RUN_RESULTS;

// 1588 Related Definitions
#define TRGOPT_PULSE        0x00000001
#define TRGOPT_PERIODIC     0x00000002
#define TRGOPT_TRG_IF_LATE  0x00000004
#define TRGOPT_NOTIFY_EN    0x00000008
#define TRGOPT_TOGGLE_EN    0x00000010

#define TXOPT_DR_INSERT     0x00000001
#define TXOPT_IGNORE_2STEP  0x00000002
#define TXOPT_CRC_1STEP     0x00000004
#define TXOPT_CHK_1STEP     0x00000008
#define TXOPT_IP1588_EN     0x00000010
#define TXOPT_L2_EN         0x00000020
#define TXOPT_IPV6_EN       0x00000040
#define TXOPT_IPV4_EN       0x00000080
#define TXOPT_TS_EN         0x00000100
#define TXOPT_SYNC_1STEP    0x00000200

typedef enum
{
    STS_SRC_ADDR_1 = 0x00000003,
    STS_SRC_ADDR_2 = 0x00000000,
    STS_SRC_ADDR_3 = 0x00000001,
    STS_SRC_ADDR_USE_MC = 0x00000002
}MAC_SRC_ADDRESS_ENUM;

#define STSOPT_LITTLE_ENDIAN    0x00000001
#define STSOPT_IPV4             0x00000002
#define STSOPT_TXTS_EN          0x00000004
#define STSOPT_RXTS_EN          0x00000008
#define STSOPT_TRIG_EN          0x00000010
#define STSOPT_EVENT_EN         0x00000020

#define RXOPT_DOMAIN_EN         0x00000001
#define RXOPT_ALT_MAST_DIS      0x00000002
#define RXOPT_USER_IP_EN        0x00000008
#define RXOPT_RX_SLAVE          0x00000010
#define RXOPT_IP1588_EN0        0x00000020
#define RXOPT_IP1588_EN1        0x00000040
#define RXOPT_IP1588_EN2        0x00000080
#define RXOPT_RX_L2_EN          0x00000100
#define RXOPT_RX_IPV6_EN        0x00000200
#define RXOPT_RX_IPV4_EN        0x00000400
#define RXOPT_SRC_ID_HASH_EN    0x00000800
#define RXOPT_RX_TS_EN          0x00001000
#define RXOPT_ACC_UDP           0x00002000
#define RXOPT_ACC_CRC           0x00004000
#define RXOPT_TS_APPEND         0x00008000
#define RXOPT_TS_INSERT         0x00010000
#define RXOPT_IPV4_UDP_MOD      0x00020000
#define RXOPT_TS_SEC_EN         0x00040000

typedef struct RX_CFG_ITEMS
{
    NS_UINT ptpVersion;
    NS_UINT ptpFirstByteMask;
    NS_UINT ptpFirstByteData;
    NS_UINT32 ipAddrData;
    NS_UINT tsMinIFG;
    NS_UINT srcIdHash;
    NS_UINT ptpDomain;
    NS_UINT tsSecLen;
    NS_UINT rxTsNanoSecOffset;
    NS_UINT rxTsSecondsOffset;
} RX_CFG_ITEMS;

#define CLKOPT_CLK_OUT_EN           0x00000001
#define CLKOPT_CLK_OUT_SEL          0x00000002
#define CLKOPT_CLK_OUT_SPEED_SEL    0x00000004

typedef enum PHYMSG_MESSAGE_TYPE_ENUM {
    PHYMSG_STATUS_TX,
    PHYMSG_STATUS_RX,
    PHYMSG_STATUS_TRIGGER,
    PHYMSG_STATUS_EVENT,
    PHYMSG_STATUS_ERROR,
    PHYMSG_STATUS_REG_READ
} PHYMSG_MESSAGE_TYPE_ENUM;


typedef union PHYMSG_MESSAGE {
    struct {
        NS_UINT32 txTimestampSecs;
        NS_UINT32 txTimestampNanoSecs;
        NS_UINT txOverflowCount;
    } TxStatus;

    struct {
        NS_UINT32 rxTimestampSecs;
        NS_UINT32 rxTimestampNanoSecs;
        NS_UINT rxOverflowCount;
        NS_UINT16 sequenceId;
        NS_UINT messageType;
        NS_UINT sourceHash;        
    } RxStatus;

    struct {
        NS_UINT triggerStatus;        // Bits [11:0] indicating what triggers occurred (12 - 1 respectively).
    } TriggerStatus;

    struct {
        NS_UINT ptpEstsRegBits;             // See PTP_ESTS register for bit definitions
        NS_BOOL extendedEventStatusFlag;    // Set to TRUE if ext. info available
        NS_UINT extendedEventInfo;          // See register definition for more info
        NS_UINT32 evtTimestampSecs;
        NS_UINT32 evtTimestampNanoSecs;
    } EventStatus;

    struct {
        NS_BOOL frameBufOverflowFlag;
        NS_BOOL frameCounterOverflowFlag;
    } ErrorStatus;

    struct {
        NS_UINT regIndex;
        NS_UINT regPage;
        NS_UINT readRegisterValue;
    } RegReadStatus;
} PHYMSG_MESSAGE;


#define PTPEVT_TRANSMIT_TIMESTAMP_BIT   0x00000001
#define PTPEVT_RECEIVE_TIMESTAMP_BIT    0x00000002
#define PTPEVT_EVENT_TIMESTAMP_BIT      0x00000004
#define PTPEVT_TRIGGER_DONE_BIT         0x00000008

#define PTP_EVENT_PACKET_LENGTH         93


//****************************************************************************
// The two structures defined below are internal to EPL and should not be referenced
// directly by higher layers. They are placed here for EPL convenience only.
//****************************************************************************
// Internal device tracking structure
typedef struct DEVICE_OBJ {
    struct DEVICE_OBJ *link;
    struct PORT_OBJ *portObjs;
    OAI_DEV_HANDLE oaiDevHandle;
    NS_UINT baseMdioAddress;
    EPL_DEV_INFO devInfo;
    EPL_DEVICE_CAPA_ENUM capa;
}DEVICE_OBJ,*PDEVICE_OBJ;

// Internal Port tracking structure
typedef struct PORT_OBJ {
    struct PORT_OBJ *link;              // Must be first field in struct
    OAI_DEV_HANDLE oaiDevHandle;
    PDEVICE_OBJ deviceObj;
    NS_UINT portMdioAddress;
    NS_UINT portMdioAddressShifted;     // portMdioAddress shifted left 23
    NS_BOOL useDirectInterface;
    NS_BOOL useMIIForRegsFlag;
    NS_UINT rxConfigOptions;
    NS_UINT tsSecondsLen;
    NS_UINT rxTsNanoSecOffset;
    NS_UINT rxTsSecondsOffset;
    NS_UINT psfConfigOptions;
    NS_UINT8 psfSrcMacAddr[6];
}PORT_OBJ,*PPORT_OBJ;

#define PEPL_DEV_HANDLE     PDEVICE_OBJ
#define PEPL_PORT_HANDLE    PPORT_OBJ


// EPL Function Prototypes

#ifdef __cplusplus
extern "C" {
#endif

EXPORT void Init_PHY_Resource(void);
EXPORT NS_STATUS EPLInitialize( void);
EXPORT void EPLDeinitialize( void);
EXPORT PEPL_DEV_HANDLE	
    EPLEnumDevice(
        IN OAI_DEV_HANDLE oaiDevHandle,
        IN NS_UINT deviceMdioAddress,
        IN EPL_ENUM_TYPE enumType);
EXPORT void
    EPLSetMgmtInterfaceConfig(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_BOOL usePhyControlFrames,
        IN NS_BOOL useDirectInterface);
EXPORT PEPL_DEV_INFO 
    EPLGetDeviceInfo(
        IN PEPL_DEV_HANDLE deviceHandle);
EXPORT NS_BOOL
    EPLIsDeviceCapable(
        IN PEPL_DEV_HANDLE deviceHandle,
        IN EPL_DEVICE_CAPA_ENUM capability);
EXPORT void
    EPLResetDevice(
        IN PEPL_DEV_HANDLE deviceHandle);
EXPORT EPL_MIICFG_ENUM
    EPLGetMiiConfig(
        IN PEPL_DEV_HANDLE deviceHandle);
EXPORT void
    EPLSetMiiConfig(
        IN PEPL_DEV_HANDLE deviceHandle,
        IN EPL_MIICFG_ENUM miiPortConfig);
EXPORT PEPL_PORT_HANDLE
    EPLEnumPort(
        IN PEPL_DEV_HANDLE deviceHandle,
        IN NS_UINT portIndex);
EXPORT PEPL_DEV_HANDLE
    EPLGetDeviceHandle(
        IN PEPL_PORT_HANDLE portHandle);
EXPORT NS_UINT
    EPLReadReg(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT registerIndex);
EXPORT void
    EPLWriteReg(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT registerIndex,
        IN NS_UINT value);
EXPORT NS_UINT
    EPLGetPortMdioAddress(
        IN PEPL_PORT_HANDLE portHandle);
EXPORT void
    EPLSetPortPowerMode(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_BOOL powerOn);
EXPORT NS_BOOL
    EPLIsLinkUp (
        IN PEPL_PORT_HANDLE portHandle);
EXPORT void
    EPLSetLinkConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN PEPL_LINK_CFG linkConfigStruct);
EXPORT void
    EPLSetLoopbackMode (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_BOOL enableLoopback);
EXPORT void
    EPLBistStartTxTest (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_BOOL psr15Flag);
EXPORT void
    EPLBistStopTxTest (
        IN PEPL_PORT_HANDLE portHandle);
EXPORT void
    EPLRestartAutoNeg (
        IN PEPL_PORT_HANDLE portHandle);
EXPORT void
    EPLDspSetLinkQualityThresholds (
        IN PEPL_PORT_HANDLE portHandle,
        IN PDSP_LINK_QUALITY_SET linkQualityStruct);
EXPORT NS_UINT
    EPLInitTDR(
        IN PEPL_PORT_HANDLE portHandle);
EXPORT void 
    EPLDeinitTDR(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT savedLinkStatus);
EXPORT NS_UINT
    EPLMeasureTDRBaseline(
        IN PEPL_PORT_HANDLE portHandle,
        NS_BOOL useTxChannel);
EXPORT void
    PTPEnable(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_BOOL enableFlag);
EXPORT void
    PTPSetTriggerConfig(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT trigger,
        IN NS_UINT triggerBehavior,
        IN NS_UINT gpioConnection);
EXPORT void
    PTPSetTriggerConfig(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT trigger,
        IN NS_UINT triggerBehavior,
        IN NS_UINT gpioConnection);
EXPORT void
    PTPSetEventConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT event,
        IN NS_BOOL eventRiseFlag,
        IN NS_BOOL eventFallFlag,
        IN NS_UINT gpioConnection);
EXPORT void
    PTPSetTransmitConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT txConfigOptions,
        IN NS_UINT ptpVersion,
        IN NS_UINT ptpFirstByteMask,
        IN NS_UINT ptpFirstByteData);
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
        IN NS_UINT ipChecksum);
EXPORT void
    PTPSetReceiveConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT rxConfigOptions,
        IN RX_CFG_ITEMS *rxConfigItems);
EXPORT NS_UINT
    PTPCalcSourceIdHash (
        IN NS_UINT8 *tenBytesData);
EXPORT void
    PTPSetTempRateDurationConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT32 duration);
EXPORT void
    PTPSetClockConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT clockConfigOptions,
        IN NS_UINT ptpClockDivideByValue,
        IN NS_UINT ptpClockSource,
        IN NS_UINT ptpClockSourcePeriod);
EXPORT void
    PTPSetGpioInterruptConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT gpioInt);
EXPORT void
    PTPSetMiscConfig (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT ptpEtherType,
        IN NS_UINT ptpOffset,
        IN NS_UINT txSfdGpio,
        IN NS_UINT rxSfdGpio);
EXPORT void
    PTPClockStepAdjustment (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT32 numberOfSeconds,
        IN NS_UINT32 numberOfNanoSeconds,
        IN NS_BOOL negativeAdj);
EXPORT void
    PTPClockSet (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT32 numberOfSeconds,
        IN NS_UINT32 numberOfNanoSeconds);
EXPORT void
    PTPClockSetRateAdjustment (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT32 rateAdjValue,
        IN NS_BOOL tempAdjFlag,
        IN NS_BOOL adjDirectionFlag);
EXPORT NS_UINT
    PTPCheckForEvents (
        IN PEPL_PORT_HANDLE portHandle);
EXPORT void
    PTPArmTrigger (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT trigger,
        IN NS_UINT32 expireTimeSeconds,
        IN NS_UINT32 expireTimeNanoSeconds,
        IN NS_BOOL initialStateFlag,
        IN NS_BOOL waitForRolloverFlag,
        IN NS_UINT32 pulseWidth,
        IN NS_UINT32 pulseWidth2);
        
EXPORT void
    PTPArmTrigger_ns (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT trigger,
        IN NS_UINT32 expireTimeNanoSeconds);
        
        
EXPORT NS_STATUS
    PTPHasTriggerExpired (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT trigger);
EXPORT void
    PTPCancelTrigger (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT trigger);
EXPORT NS_UINT
    MonitorGpioSignals (
        IN PEPL_PORT_HANDLE portHandle);

EXPORT void
    EPLGetLinkStatus (
        IN PEPL_PORT_HANDLE portHandle,
        IN OUT PEPL_LINK_STS linkStatusStruct);
#ifndef SWIG
EXPORT void
    EPLBistGetStatus (
        IN PEPL_PORT_HANDLE portHandle,
        IN OUT NS_BOOL *bistActiveFlag,
        IN OUT NS_UINT *errDataNibbleCount);
EXPORT NS_STATUS
    EPLGetCableStatus (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT sampleTime,
        IN OUT NS_UINT *cableLength,
        IN OUT NS_SINT *freqOffsetValue,
        IN OUT NS_SINT *freqControlValue,
        IN OUT NS_UINT *varianceValue);
EXPORT void
    EPLDspGetLinkQuality(
        IN PEPL_PORT_HANDLE portHandle,
        IN OUT PDSP_LINK_QUALITY_GET linkQualityStruct);
EXPORT void
    EPLGetTDRPulseShape(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_BOOL useTxChannel,
        IN NS_BOOL use50nsPulse,
        IN OUT NS_SINT8 *positivePulseResults,
        IN OUT NS_SINT8 *negativePulseResults);
EXPORT NS_STATUS
    EPLGetTDRCableInfo(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_BOOL useTxChannel,
        IN OUT EPL_CABLE_STS_ENUM *cableStatus,
        IN OUT NS_UINT *rawCableLength);
EXPORT NS_BOOL
    EPLGatherTDRInfo(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_BOOL useTxChannel,
        IN NS_BOOL useTxReflectChannel,
        IN NS_UINT thresholdAdjustConstant,
        IN NS_BOOL stopAfterSuccess,
        IN OUT NS_UINT *baseline,
        IN OUT PTDR_RUN_RESULTS posResultsArrayNoInvert,
        IN OUT PTDR_RUN_RESULTS negResultsArrayNoInvert,
        IN OUT PTDR_RUN_RESULTS posResultsArrayInvert,
        IN OUT PTDR_RUN_RESULTS negResultsArrayInvert);
EXPORT NS_BOOL
    EPLShortTDRPulseRun(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_BOOL useTxChannel,
        IN NS_BOOL useTxReflectChannel,
        IN NS_UINT posThreshold,
        IN NS_UINT negThreshold,
        IN NS_BOOL noninvertedThreshold,
        IN NS_BOOL stopAfterSuccess,
        IN OUT PTDR_RUN_RESULTS posResultsArray,
        IN OUT PTDR_RUN_RESULTS negResultsArray);
EXPORT NS_BOOL
    EPLLongTDRPulseRun(
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_BOOL useTxChannel,
        IN NS_BOOL useTxReflectChannel,
        IN NS_UINT threshold,
        IN NS_BOOL stopAfterSuccess,
        IN OUT PTDR_RUN_RESULTS resultsArray);
EXPORT void
    EPLRunTDR(
        IN PEPL_PORT_HANDLE portHandle,
        IN PTDR_RUN_REQUEST tdrParms,
        IN OUT PTDR_RUN_RESULTS tdrResults);
EXPORT void
    PTPClockReadCurrent (
        IN PEPL_PORT_HANDLE portHandle,
        IN OUT NS_UINT32 *retNumberOfSeconds,
        IN OUT NS_UINT32 *retNumberOfNanoSeconds);
EXPORT void
    PTPGetTransmitTimestamp (
        IN PEPL_PORT_HANDLE portHandle,
        IN OUT NS_UINT32 *retNumberOfSeconds,
        IN OUT NS_UINT32 *retNumberOfNanoSeconds,
        IN OUT NS_UINT *overflowCount);
EXPORT void
    PTPGetReceiveTimestamp (
        IN PEPL_PORT_HANDLE portHandle,
        IN OUT NS_UINT32 *retNumberOfSeconds,
        IN OUT NS_UINT32 *retNumberOfNanoSeconds,
        IN OUT NS_UINT *overflowCount,
        IN OUT NS_UINT *sequenceId,
        IN OUT NS_UINT *messageType,
        IN OUT NS_UINT *hashValue);
EXPORT void
    PTPGetTimestampFromFrame (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT8 *receiveFrameData,
        IN OUT NS_UINT32 *retNumberOfSeconds,
        IN OUT NS_UINT32 *retNumberOfNanoSeconds);
EXPORT NS_BOOL
    PTPGetEvent (
        IN PEPL_PORT_HANDLE portHandle,
        IN OUT NS_UINT *eventBits,
        IN OUT NS_UINT *riseFlags,
        IN OUT NS_UINT32 *eventTimeSeconds,
        IN OUT NS_UINT32 *eventTimeNanoSeconds,
        IN OUT NS_UINT *eventsMissed);
EXPORT NS_BOOL
    IsPhyStatusFrame (
        IN PEPL_PORT_HANDLE portHandle,
        IN NS_UINT8 *frameBuffer,
        IN NS_UINT frameLength,
        IN OUT NS_UINT8 **nextMsgLocation);
EXPORT NS_BOOL
    GetNextPhyMessage (
        IN OUT NS_UINT8 **nextMsgLocation,
        IN OUT PHYMSG_MESSAGE_TYPE_ENUM *messageType,
        IN OUT PHYMSG_MESSAGE *message);

#endif // not SWIG        

#ifdef __cplusplus
}
#endif

#endif // _EPL_INCLUDE
