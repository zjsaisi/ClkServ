/******************************************************************************
$Id: config_if/sc_readconfig.c 1.38 2012/02/01 16:04:45PST German Alvarez (galvarez) Exp  $

Copyright (C) 2009, 2010, 2011 Symmetricom, Inc., all rights reserved
This software contains proprietary and confidential information of Symmetricom.
It may not be reproduced, used, or disclosed to others for any purpose
without the written authorization of Symmetricom.


Original author: German Alvarez

Last revision by: $Author: German Alvarez (galvarez) $

File name: $Source: config_if/sc_readconfig.c $

Functionality:

This file is used to initialize the soft clock reading a configuration file.

The default configuration file that gets installed into the board is
the sc.cfg file in this same directory. Please update the file with any
new configuration options. The default configuration file is the only place
where the commands available and syntax is available for a user.

******************************************************************************
Log:
$Log: config_if/sc_readconfig.c  $
Revision 1.38 2012/02/01 16:04:45PST German Alvarez (galvarez) 
added HIGH_JITTER_ACCESS configuration
Revision 1.37 2011/10/10 14:06:06PDT German Alvarez (galvarez) 
reorganized and simplified code to asign logical channles to chanA and chanB phasors
Revision 1.36 2011/10/07 20:12:27PDT Kenneth Ho (kho) 
Fixed FreqOnly bug
Revision 1.35 2011/10/07 11:06:13PDT Kenneth Ho (kho) 
Minor work to get katana scrdb build working
Revision 1.34 2011/10/07 09:59:56PDT German Alvarez (galvarez) 

Revision 1.33 2011/10/07 09:54:46PDT German Alvarez (galvarez) 

Revision 1.32 2011/10/06 17:24:21PDT German Alvarez (galvarez) 

Revision 1.31 2011/10/04 11:14:04PDT Kenneth Ho (kho) 
Make sure that all channels can be implemented
Revision 1.30 2011/09/20 12:25:20PDT German Alvarez (galvarez) 
Implement user side of the redundancy code.
Revision 1.29 2011/09/12 20:56:48PDT German Alvarez (galvarez) 
added phase control menu. Sarted to work on redundancy menu.
Revision 1.28 2011/09/07 16:41:18PDT Kenneth Ho (kho) 
Fixed span line bug
Revision 1.27 2011/08/24 17:04:33PDT Kenneth Ho (kho) 
Added Ext reference configurations
Revision 1.26 2011/06/09 14:03:21PDT German Alvarez (galvarez) 
minor changes and cleanup
Revision 1.25 2011/03/25 14:45:50PDT Daniel Brown (dbrown)
Additions for user-entered position and cold start timeout values.
Revision 1.24 2011/03/25 09:38:40PDT German Alvarez (galvarez)
Read PhaseMode as a number
Revision 1.23 2011/03/10 17:36:47PST German Alvarez (galvarez)
fix an error reading the delay request value from the file.
Revision 1.22 2011/03/02 13:37:58PST Kenneth Ho (kho)
Fixed PTP valid indication (always true)
Revision 1.21 2011/02/25 18:11:20PST German Alvarez (galvarez)
minor changes. printing error messages when write freq. corrections fails. more relaxed checks on the input file to
allow testing the API for out of bounds parameters.
Revision 1.20 2011/02/22 17:12:20PST German Alvarez (galvarez)
read Local oscillator QL from config file, added SetSelChan UI
Revision 1.19 2011/02/20 11:35:24PST Kenneth Ho (kho)
Added functions to get channel number for specific types of channels
Revision 1.18 2011/02/17 10:07:31PST German Alvarez (galvarez)
clean-up no changes in functionality.
Revision 1.17 2011/02/16 16:52:08PST Kenneth Ho (kho)
Changed name of variable in channel init array (l_MeasRate)
Added code to store values of GPS and PTP channel assignments
Revision 1.16 2011/02/16 15:11:55PST German Alvarez (galvarez)
create configuration file from default configuration in /active/appl/sc.cfg.
The default configuration file in config_if/sc.cfg is copied to /active/appl/sc.cfg as part of the build process.
Revision 1.15 2011/02/15 16:57:47PST German Alvarez (galvarez)
parse E1, T1 channel types.
Revision 1.14 2011/02/14 12:54:48PST German Alvarez (galvarez)
added support for sync rate and delay request rate. Removed SSM from configuration file.
Revision 1.11 2011/02/10 16:53:57PST German Alvarez (galvarez)
Handle the SSM from the input drivers and hand it to the soft clock.
Also allow for injections of SSM values to help testing.
Revision 1.10 2011/02/10 11:25:18PST German Alvarez (galvarez)
add TransportType to the configuration options
Revision 1.9 2011/02/09 17:27:04PST German Alvarez (galvarez)
simulate the SSM implemented int SC_ChanSSM(UINT8 b_chanNum, UINT8 *pb_ssm, BOOLEAN *po_Valid)
with an array of values.
Revision 1.8 2011/02/08 17:53:26PST German Alvarez (galvarez)
to remove measurement resolution from the channel configuration call
Revision 1.7 2011/02/07 18:05:11PST German Alvarez (galvarez)

Revision 1.5 2011/02/03 14:33:04PST German Alvarez (galvarez)
reads the offset file. Corrected a parsing error with blank lines.
Revision 1.4 2011/02/03 12:30:22PST German Alvarez (galvarez)
fixed parsing of commands with only one parameter, added changed the following commands:

#ChanSwitchMode <AR or AS or OFF>
#Default is AR (is this a good default??)
#Example:
#ChanSwitchMode AR

#ChanSelectionMode <PRIO or QL>
#Defualt is PRIO (This needs to be verified)
#Example:
#ChanSelectionMode QL
Revision 1.3 2011/02/02 17:36:21PST Kenneth Ho (kho)
changed t_SC_Chan_Type to SC_t_Chan_Type
Revision 1.2 2011/02/02 17:29:26PST German Alvarez (galvarez)

Revision 1.1 2011/02/01 16:25:22PST German Alvarez (galvarez)
Initial revision
Member added to project e:/MKS_Projects/PTPSoftClientDevKit/SCRDB_MPC8313/ptp/common/project.pj

******************************************************************************/

/*--------------------------------------------------------------------------*/
//              include
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
//#include <pthread.h>
//#include <unistd.h>
//#include <signal.h>
//#include <syslog.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <inttypes.h>

#include "sc_types.h"
#include "sc_api.h"
#include "sc_servo_api.h"
#include "log/sc_log.h"
#include "network_if/sc_network_if.h"
#include "clock_if/sc_clock_if.h"
#include "local_debug.h"
#include "sc_chan.h"
#include "user_if/sc_ui.h"
#include "chan_processing_if/sc_SSM_handling.h"
#include "redundancy_if/sc_redundancy_if.h"

#include "sc_readconfig.h"
/*--------------------------------------------------------------------------*/
//              defines
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              types
/*--------------------------------------------------------------------------*/

typedef enum
{
  e_InitChanConfig,
  e_LOQualityLevel,
  e_FreqSynthOn,
  e_PhaseMode,
  e_Oscillator,
  e_TransportType,
  e_ChanSwitchMode,
  e_ChanSelectionMode,
  e_DebugMask,
  e_UnicastMasterAddress,
  e_SyncIntv,
  e_DelReqIntv,
  e_GpsColdStrtTmOut,
  e_GpsUserPosEnabled,
  e_GpsLatitude,
  e_GpsLongitude,
  e_GpsAltitude,
  e_ExtRefDividerA,
  e_ExtRefDividerB,
  e_SyncPhaseThreshold,
  e_RedundantPeer,
  e_RedundantEnable
} t_ConfigCommand;

typedef struct
{
  t_ConfigCommand cmd;
  char *name;
} t_ConfigCommandName;

/*--------------------------------------------------------------------------*/
//              data
/*--------------------------------------------------------------------------*/
static const char *kConfigFileName = "/active/config/sc.cfg";

//globals for the frequency correction file
static const char *kOffsetFileName = "/active/config/offset.cfg";
static const char *FreqCorrToken = "FreqCorr";
static const char *FreqCorrTimeStampToken = "FreqCorrTime";


static  UINT8 gSe1ChannelNumber = 0xFF;
static  UINT8 gSe2ChannelNumber = 0xFF;
static  UINT8 gGPS1ChannelNumber = 0xFF;
static  UINT8 gGPS2ChannelNumber = 0xFF;
static  UINT8 gGPS3ChannelNumber = 0xFF;
static  UINT8 gGPS4ChannelNumber = 0xFF;
static  UINT8 gPTPChannelNumber = 0xFF;
static  UINT8 gE1ChannelNumber = 0xFF;
static  UINT8 gT1ChannelNumber = 0xFF;
static  UINT8 gEth0ChannelNumber = 0xFF;
static  UINT8 gEth1ChannelNumber = 0xFF;
static  UINT8 gRedChannelNumber = 0xFF;
static  UINT8 gFreqOnlyChannelNumber = 0xFF;
static  UINT8 gExtAChannelNumber = 0xFF;
static  UINT8 gExtBChannelNumber = 0xFF;
static  UINT8 o_ChanATaken = FALSE;
static  UINT8 o_ChanBTaken = FALSE;

static t_ConfigCommandName kConfigCommandName[] = {
  {e_InitChanConfig, "InitChanConfig"},
  {e_LOQualityLevel, "LOQualityLevel"},
  {e_FreqSynthOn, "FreqSynthOn"},
  {e_PhaseMode, "PhaseMode"},
  {e_Oscillator, "Oscillator"},
  {e_TransportType, "TransportType"},
  {e_ChanSwitchMode, "ChanSwitchMode"},
  {e_ChanSelectionMode, "ChanSelectionMode"},
  {e_DebugMask, "DebugMask"},
  {e_UnicastMasterAddress, "UnicastMasterAddress"},
  {e_SyncIntv, "SyncIntv"},
  {e_DelReqIntv, "DelReqIntv"},
  {e_GpsColdStrtTmOut, "GpsColdStrtTmOut"},
  {e_GpsUserPosEnabled, "GpsUserPosEnabled"},
  {e_GpsLatitude, "GpsLatitude"},
  {e_GpsLongitude, "GpsLongitude"},
  {e_GpsAltitude, "GpsAltitude"},
  {e_ExtRefDividerA, "ExtRefDividerA"},
  {e_ExtRefDividerB, "ExtRefDividerB"},
  {e_SyncPhaseThreshold, "SyncPhaseThreshold"},
  {e_RedundantPeer, "RedundantPeerIP"},
  {e_RedundantEnable, "RedundantEnable"}
};

static t_ChanTypeName kChanName[] = {
  {e_SC_CHAN_TYPE_PTP, "PTP"},
  {e_SC_CHAN_TYPE_GPS, "GPS"},
  {e_SC_CHAN_TYPE_SE, "SyncE"},
  {e_SC_CHAN_TYPE_E1, "E1"},
  {e_SC_CHAN_TYPE_T1, "T1"},
  {e_SC_CHAN_TYPE_REDUNDANT, "Redund"},
  {e_SC_CHAN_TYPE_FREQ_ONLY, "FreqOnly"},
};

t_AccessTransportName kAccessTransportName[] = {
  {e_PTP_MODE_ETHERNET, "ETHERNET"},
  {e_PTP_MODE_DSL, "DSL"},
  {e_PTP_MODE_MICROWAVE, "MICROWAVE"},
  {e_PTP_MODE_SONET, "SONET"},
  {e_PTP_MODE_SLOW_ETHERNET, "SLOW_ETHERNET"},
  {e_PTP_MODE_HIGH_JITTER_ACCESS, "HIGH_JITTER_ACCESS"},
};
/*--------------------------------------------------------------------------*/
//              externals
/*--------------------------------------------------------------------------*/
extern void SC_SetSynthOn(BOOLEAN o_on);

/*--------------------------------------------------------------------------*/
//              function prototypes
/*--------------------------------------------------------------------------*/
int parseInitChanConfig(char *line, SC_t_ChanConfig *channel);
void setPTPProtocolConfig(t_ptpGeneralConfig *genConfig, t_ptpPortConfig *ptpPortCfg,
    t_ptpProfileConfig *profileConfig);
int parseIPAddress(char *line, t_PortAddr *address);
char inline *strncat_local(char *dest, const char *src, size_t n);
int stringToChanType(const char *s, SC_t_Chan_Type *chan);
int stringToAccessTransportType(const char *s, t_ptpAccessTransportEnum *transport);
int accessTransportTypeToString(const t_ptpAccessTransportEnum transport, const int bufSize, char *s);
int commandToString(char *s, int bufSize, t_ConfigCommand cmd);
void readOffsetFromFile(t_servoConfigType *config);
int stringToCommand(const char *s, t_ConfigCommand *cmd);
void chanConfigToString(char *s, int bufSize, SC_t_ChanConfig *theChan);
static void verifyConfigFile(void);
int readConfigFromFile();
int chanTypeToString(char *s, const int bufSize, SC_t_Chan_Type chan);
UINT8 GetGps1Chan(void);
UINT8 GetGps2Chan(void);
UINT8 GetGps3Chan(void);
UINT8 GetGps4Chan(void);
UINT8 GetSe1Chan(void);
UINT8 GetSe2Chan(void);
UINT8 GetT1Chan(void);
UINT8 GetE1Chan(void);
UINT8 GetExtAChan(void);
UINT8 GetExtBChan(void);
UINT8 GetEth0Chan(void);
UINT8 GetEth1Chan(void);
UINT8 GetRedundancyChan(void);
UINT8 GetFreqOnlyChan(void);


/*--------------------------------------------------------------------------*/
//              functions
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
    int readConfigFromFile()

Read the file kConfigFileName and call the following functions with values
from the file:

    SC_InitChanConfig
    SC_InitServoConfig
    SC_SetSynthOn
    SC_DbgConfig
    SC_InitPtpConfig (if a PTP channel is declared)
    SC_InitUnicastConfig (if a PTP channel is declared)
    SC_InitGpsConfig (if a GPS channel is declared)

Return values:
     0          no error

  Any value different from zero is an error.
  It can return any return code from SC_InitChanConfig, SC_InitServoConfig,
  SC_InitPtpConfig, SC_InitUnicastConfig, SC_InitGpsConfig and fopen
  plus the following codes:

    -101          syntax error on config file
    -102          more than kMaxNumberOfChannels channels in file
    -103          no channels configured
    -104          more than k_MAX_UC_MASTER_TABLE_SIZE addresses in file

  Note: Error codes from fopen are positive.
--------------------------------------------------------------------------*/

int readConfigFromFile()
{
  const int retCodeOffset = -100; //to offset the return code
  const int kMaxLineLength = 255;
  const int kMaxNumberOfChannels = SC_TOTAL_CHANNELS + 10; // this is bigger than SC_TOTAL_CHANNELS to be able to test the API error handling
  int retCode = 0;
  FILE *fp_cfg = NULL;
  int lineCounter = 0;
  BOOLEAN o_isE1T1OutEnable = TRUE;

  //variables for parsing the input
  t_ConfigCommand theCommand;
  char line[kMaxLineLength + 1];
  char commandName[kMaxLineLength + 1];

  //short lived variables used to parse individual lines
  int int_data;
  char char_data;
  UINT32 UINT32_data;
  double double_data;
  float float_data;
  char str_data[kMaxLineLength + 1];

  int numPTPProtocolChannels = 0;

  SC_t_ChanConfig chanConfig[kMaxNumberOfChannels];
  memset(&chanConfig, 0, sizeof(chanConfig));
  UINT8 numChannels = 0;

  //variables for e_FreqSynthOn
#if 0
  BOOLEAN o_synthOn = TRUE;
#endif
  BOOLEAN o_synthOn = FALSE;

  //variables for t_servoConfigType
  t_servoConfigType servoConfig;
  memset(&servoConfig, 0, sizeof(servoConfig));
  servoConfig.e_loType = e_OCXO;//e_MINI_OCXO;   /* local oscillator type */
  servoConfig.e_ptpTransport = e_PTP_MODE_ETHERNET;     /* PTP transport modes   */
  servoConfig.dw_ptpPhaseMode = k_PTP_PHASE_MODE_STANDARD;      /* standard, and outband phase */
  servoConfig.f_freqCorr = 0.0;
  servoConfig.f_freqCalFactory= 0.0;
#if 0
  servoConfig.dw_bridgeTime = 420;      /* Bridge time in seconds */
#endif
  servoConfig.dw_bridgeTime = 420;      /* Bridge time in seconds */
  servoConfig.b_loQl = 16;
  readOffsetFromFile(&servoConfig);

  //variables for e_ChanSwitchMode
  SC_t_swtModeEnum chanSwitchMode = e_SC_SWTMODE_AR;

  //variables for e_ChanSelectionMode
  SC_t_selModeEnum chanSelectionMode = e_SC_CHAN_SEL_PRIO;

  //variables for t_logConfigType
  t_logConfigType s_logCfg;
  SC_DbgConfig(&s_logCfg, e_PARAM_GET); //get the defaults

  //variables for PTP protocol
  t_ptpGeneralConfig genConfigPTP;
  t_ptpPortConfig portConfigPTP;
  t_ptpProfileConfig profileConfigPTP;
  //initialize with default values
  setPTPProtocolConfig(&genConfigPTP,  &portConfigPTP, &profileConfigPTP);

  //variables for t_ucMasterType
  t_ucMasterType uCTable;
  memset(&uCTable, 0, sizeof(t_ucMasterType));
  uCTable.o_ucNegoEnable = k_DEFAULT_UC_NEGO;
  uCTable.w_leaseDuration = k_DEFAULT_UC_LEASE_DUR;
  uCTable.c_logQryIntv = k_DEFAULT_UC_QUERY_INTV;
  uCTable.w_ucMasterTableSize = 0;

  //variables for SC_t_GPS_GeneralConfig
  SC_t_GPS_GeneralConfig gpsConfig;
  gpsConfig.o_gpsEnabled = TRUE;                      /* GPS enabled flag */
  gpsConfig.e_rcvrType = e_RCVR_ST_ERIC_GNS7560_BGA;  /* GPS receiver type */
  gpsConfig.ColdStrtTmOut = 0;                        /* GPS cold start timeout value,
                                                         0 means default of 15 [minutes] */
  gpsConfig.UserPosEnabled = FALSE;                   /* GPS cold start timeout value */
  gpsConfig.Latitude = 0;                             /* GPS WGS84 Latitude, degrees */
  gpsConfig.Longitude = 0;                            /* GPS WGS84 Longitude, degrees */
  gpsConfig.Altitude = 0;                             /* GPS WGS84 Altitude, meters */

  //variables for SC_SyncPhaseThreshold
  UINT32 syncPhaseThreshold = 80000;  /* default 80 us */
  int syncPhaseThresholdUsed = 0;

  //variables for redundancy
#if (SC_RED_CHANNELS > 0)
  BOOLEAN isRedundancyEnabled = FALSE;
#endif

  verifyConfigFile();

  fp_cfg = fopen(kConfigFileName, "r");
  if(fp_cfg)
  {
    while(fgets(line, kMaxLineLength, fp_cfg))
    {
      commandName[0] = 0;
      lineCounter++;
      if(line[0] != 0 && line[0] != '#' && isprint(line[0]) && isalnum(line[0]))
      {
        sscanf(line, "%255s", commandName);
        if(stringToCommand(commandName, &theCommand) == 0)
        {
          //printf("Parsing command |%s| line |%s|\n", commandName, line);
          switch (theCommand)
          {
            case e_InitChanConfig:
              if(numChannels >= kMaxNumberOfChannels)
              {
                printf
                  ("Error in line %d: Cannot configure more than %d channels.\n",
                   lineCounter, numChannels);
                retCode = retCodeOffset - 2;
                goto terminateParse;
              }
              retCode = parseInitChanConfig(line, &chanConfig[numChannels]);

              if(retCode == 0)
              {
                switch (chanConfig[numChannels].e_ChanType)
                {
                case e_SC_CHAN_TYPE_PTP:
                  numPTPProtocolChannels++;
                  gPTPChannelNumber = numChannels;
                  break;

                case e_SC_CHAN_TYPE_GPS:
					if(gGPS1ChannelNumber == 0xFF){
                 		gGPS1ChannelNumber = numChannels;
					}	
					else if(gGPS2ChannelNumber == 0xFF){
						gGPS2ChannelNumber = numChannels;
					}
					else if(gGPS3ChannelNumber == 0xFF){
						gGPS3ChannelNumber = numChannels;
					}
					else if(gGPS4ChannelNumber == 0xFF){
						gGPS4ChannelNumber = numChannels;
					}
                  break;

                case e_SC_CHAN_TYPE_SE:
                  if(gEth0ChannelNumber == 0xFF)
                  {
                    gEth0ChannelNumber = numChannels;
                  }
                  else if(gEth1ChannelNumber == 0xFF)
                  {
                    gEth1ChannelNumber = numChannels;
                  }
                  if(gSe1ChannelNumber == 0xFF)
                    gSe1ChannelNumber = numChannels;
                  else
                    gSe2ChannelNumber = numChannels;
                  break;

                case e_SC_CHAN_TYPE_E1:
                case e_SC_CHAN_TYPE_T1:
                case e_SC_CHAN_TYPE_REDUNDANT:
                case e_SC_CHAN_TYPE_FREQ_ONLY:
                  if(o_ChanATaken && o_ChanBTaken)
                  {
                    printf("Error in line %d: No more physical channels available %s.\n", lineCounter, commandName);
                    retCode = retCodeOffset - 1;
                    goto terminateParse;
                    break;
                  }
                  else
                  { //Assign A and B in in order

                    switch (chanConfig[numChannels].e_ChanType) //ugly switch inside a switch on the same condition but saves over 70 lines of code
                    {
                      case e_SC_CHAN_TYPE_E1: gE1ChannelNumber = numChannels; break;
                      case e_SC_CHAN_TYPE_T1: gT1ChannelNumber = numChannels; break;
                      case e_SC_CHAN_TYPE_REDUNDANT: gRedChannelNumber = numChannels; break;
                      case e_SC_CHAN_TYPE_FREQ_ONLY: gFreqOnlyChannelNumber = numChannels; break;
                      default: ; /*should not happen*/ break;
                    }

                    if(!o_ChanATaken)
                    {  //chan A is free, use it
                      o_ChanATaken = TRUE;
                      gExtAChannelNumber = numChannels;
                      o_isE1T1OutEnable = FALSE;
                    }
                    else
                    {  //chan B has to be free, use it
                      o_ChanBTaken = TRUE;
                      gExtBChannelNumber = numChannels;
                    }
                  }
                  break;

                default:
                  printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                  retCode = retCodeOffset - 1;
                  goto terminateParse;
                  break;
                }

                numChannels++;
              }
              break;

            case e_LOQualityLevel:
              if((sscanf(line, "%*s %d", &int_data) == 1) && (int_data >= 0))
              {
                servoConfig.b_loQl = (UINT8)int_data;
              }
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_FreqSynthOn:
              if(sscanf(line, "%*s %c", &char_data) == 1 && isalnum(char_data))
                o_synthOn = trueFalseCharToInt(char_data);
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_PhaseMode:
              if(sscanf(line, "%*s %i", &int_data) == 1) //Number decimal or hex
                servoConfig.dw_ptpPhaseMode = int_data;
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_Oscillator:
              if(sscanf(line, "%*s %d", &int_data) == 1)
                servoConfig.e_loType = int_data;
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_TransportType:
              if(sscanf(line, "%*s %s", str_data) == 1)
              {
                if(stringToAccessTransportType(str_data, &servoConfig.e_ptpTransport) != 0)
                {
                  printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                  retCode = retCodeOffset -1;
                  goto terminateParse;
                }
              }
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
              break;

            case e_ChanSwitchMode:
              //get the second letter. Options are AR AS or OFF
              if(sscanf(line, "%*s %*c%c", &char_data) == 1)
                switch (char_data)
                {
                  case 'R': case 'r': chanSwitchMode = e_SC_SWTMODE_AR; break;
                  case 'S': case 's': chanSwitchMode = e_SC_SWTMODE_AS; break;
                  case 'F': case 'f': chanSwitchMode = e_SC_SWTMODE_OFF; break;

                  default:
                    printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                    retCode = retCodeOffset -1;
                    goto terminateParse;
                  break;
                }
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_ChanSelectionMode:
              if(sscanf(line, "%*s %c", &char_data) == 1)
                switch (char_data)
                {
                  case 'P': case 'p': chanSelectionMode = e_SC_CHAN_SEL_PRIO; break;
                  case 'Q': case 'q': chanSelectionMode = e_SC_CHAN_SEL_QL; break;

                  default:
                    printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                    retCode = retCodeOffset -1;
                    goto terminateParse;
                  break;
                }
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_DebugMask:
              if(sscanf(line, "%*s %x", &UINT32_data) == 1)
                s_logCfg.l_debugMask = UINT32_data;
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_UnicastMasterAddress:
              if(numPTPProtocolChannels == 0)
              {
                printf("Error in line %d: Need to define a PTP protocol channel before using %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
              if(uCTable.w_ucMasterTableSize >= k_MAX_UC_MASTER_TABLE_SIZE)
              {
                printf("Error in line %d: Cannot configure more than %d Unicast Master Addresses.\n", lineCounter, k_MAX_UC_MASTER_TABLE_SIZE);
                retCode = retCodeOffset -4;
                goto terminateParse;
              }
              retCode = parseIPAddress(line, &uCTable.as_addr[uCTable.w_ucMasterTableSize]);
              if(retCode == 0)
                uCTable.w_ucMasterTableSize++;
              else
              {
                printf("In %s for address %d.\n", commandName, uCTable.w_ucMasterTableSize);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_SyncIntv:
              if((sscanf(line, "%*s %d", &int_data) == 1) && (int_data >= -7) && (int_data <= 7))
                portConfigPTP.c_syncIntv = (INT8)int_data;
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_DelReqIntv:
              if((sscanf(line, "%*s %d", &int_data) == 1) && (int_data >= -7) && (int_data <= 7))
                portConfigPTP.c_delReqIntv = (INT8)int_data;
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_GpsColdStrtTmOut:
              if((sscanf(line, "%*s %d", &int_data) == 1))
                gpsConfig.ColdStrtTmOut = (UINT8)int_data;
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_GpsUserPosEnabled:
              if(sscanf(line, "%*s %c", &char_data) == 1 && isalnum(char_data))
                gpsConfig.UserPosEnabled = trueFalseCharToInt(char_data);
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_GpsLatitude:
              if((sscanf(line, "%*s %lf", &double_data) == 1))
                gpsConfig.Latitude = (FLOAT64)double_data;
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_GpsLongitude:
              if((sscanf(line, "%*s %lf", &double_data) == 1))
                gpsConfig.Longitude = (FLOAT64)double_data;
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_GpsAltitude:
              if((sscanf(line, "%*s %f", &float_data) == 1))
                gpsConfig.Altitude = (FLOAT64)float_data;
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;            

            case e_ExtRefDividerA:
              if((sscanf(line, "%*s %d", &int_data) == 1) && (int_data > 0))
              {
                setExtRefDivider(EXTREF_A, (UINT32)int_data);
              }
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }            
            break;

            case e_ExtRefDividerB:
              if((sscanf(line, "%*s %d", &int_data) == 1) && (int_data > 0))
              {
                setExtRefDivider(EXTREF_B, (UINT32)int_data);
              }
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;

            case e_SyncPhaseThreshold:
              if(sscanf(line, "%*s %u", &syncPhaseThreshold) == 1)
              {
                syncPhaseThresholdUsed = TRUE;
              }
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
              break;

#if (SC_RED_CHANNELS > 0)
              case e_RedundantPeer:
              str_data[0] = 0; //clear input buffer
              if((sscanf(line, "%*s %s", str_data) == 1))
              {
                retCode = setRedundantPeerAddress(str_data);
                if(retCode != 0)
                {
                  printf("Error in line %d: setRedundantPeerAddress(%s) returned %d.\n", lineCounter, str_data, retCode);
                  retCode = retCodeOffset -1;
                  goto terminateParse;
                }
              }
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
              break;

            case e_RedundantEnable:
              if(sscanf(line, "%*s %c", &char_data) == 1 && isalnum(char_data))
                isRedundancyEnabled = trueFalseCharToInt(char_data);
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
              break;
#endif

/*            case e_SetSSM:
              if(sscanf(line, "%*s %d", &int_data) == 1)
              {
                if(int_data >= 0 && int_data < kMaxNumberOfChannels)
                {
                  sscanf(line, "%*s %*d %d %c", &int_data2, &char_data);
                  //gSSMTable[int_data].value = (UINT8)int_data2;
                  //gSSMTable[int_data].isValid = trueFalseCharToInt(char_data);
                }
                else
                {
                   printf("Error in line %d: Invalid channel number for %s.\n", lineCounter, commandName);
                   retCode = retCodeOffset -1;
                   goto terminateParse;
                 }
              }
              else
              {
                printf("Error in line %d: Invalid value for %s.\n", lineCounter, commandName);
                retCode = retCodeOffset -1;
                goto terminateParse;
              }
            break;
*/
            default:
              printf("Warning in line %d: Unhandled case for line starting with: %s\n", lineCounter, commandName);
            break;
          }
        }
        else
          printf("Warning in line %d: Ignoring line starting with: %s\n", lineCounter, commandName);
      }
    }
  }
  else
  {
    retCode = (errno == 0)?-1:errno;
    printf("Error: Cannot open file %s.\n", kConfigFileName);
    goto terminateParse;
  }

  if(retCode == 0)
  {
    if(numChannels == 0)
    {
      printf("Error: Need to configure at least one channel.\n");
      retCode = retCodeOffset -3;
      goto terminateParse;
    }

    //if we are here parse completed OK
    //Call all the initialization routines
    retCode = SC_InitChanConfig(chanConfig, chanSelectionMode, chanSwitchMode, numChannels);
    if(retCode != 0)
    {
      printf("Error: SC_InitChanConfig failed with code: %d\n", retCode);
      goto terminateParse;
    }

/* Make PTP and GPS channel valid all the time */
    if(gPTPChannelNumber!=0xFF)
    {
        SC_SetChanValid(gPTPChannelNumber, TRUE);
    }

    if(gGPS1ChannelNumber!=0xFF)
    {
        SC_SetChanValid(gGPS1ChannelNumber, TRUE);
    }

    if(gGPS2ChannelNumber!=0xFF)
    {
        SC_SetChanValid(gGPS2ChannelNumber, TRUE);
    }

    if(gGPS3ChannelNumber!=0xFF)
    {
        SC_SetChanValid(gGPS3ChannelNumber, TRUE);
    }

    if(gGPS4ChannelNumber!=0xFF)
    {
        SC_SetChanValid(gGPS4ChannelNumber, TRUE);
    }

    if(gSe1ChannelNumber!=0xFF)
    {
        SC_SetChanValid(gSe1ChannelNumber, TRUE);
    }

    if(gSe2ChannelNumber!=0xFF)
    {
        SC_SetChanValid(gSe2ChannelNumber, TRUE);
    }

    if(gE1ChannelNumber!=0xFF)
    {
        SC_SetChanValid(gE1ChannelNumber, TRUE);
    }

    if(gT1ChannelNumber!=0xFF)
    {
        SC_SetChanValid(gT1ChannelNumber, TRUE);
    }

    if(gRedChannelNumber!=0xFF)
    {
        SC_SetChanValid(gRedChannelNumber, TRUE);
    }

    if(gFreqOnlyChannelNumber!=0xFF)
    {
        SC_SetChanValid(gFreqOnlyChannelNumber, TRUE);
    }

    retCode = SC_InitServoConfig(&servoConfig);
    if(retCode != 0)
    {
      printf("Error: SC_InitServoConfig failed with code: %d\n", retCode);
      goto terminateParse;
    }

    SC_SetSynthOn(o_synthOn);

    SC_DbgConfig(&s_logCfg, e_PARAM_SET);

    if(numPTPProtocolChannels > 0)
      retCode = SC_InitPtpConfig(&genConfigPTP, &portConfigPTP, &profileConfigPTP);
    if(retCode != 0)
    {
      printf("Error: SC_InitPtpConfig failed with code: %d\n", retCode);
      goto terminateParse;
    }
    if(numPTPProtocolChannels > 0)
      retCode = SC_InitUnicastConfig(&uCTable);
    if(retCode != 0)
    {
      printf("Error: SC_InitUnicastConfig failed with code: %d\n", retCode);
      goto terminateParse;
    }

    retCode = SC_SyncPhaseThreshold(e_PARAM_SET, &syncPhaseThreshold);
    if(retCode != 0)
    {
      printf("Error SC_SyncPhaseThreshold(e_PARAM_SET, %u) failed with code %d.\n", syncPhaseThreshold, retCode);
      retCode = retCodeOffset -1;
      goto terminateParse;
    }

#if (SC_RED_CHANNELS > 0)
    SC_RedundantEnable(isRedundancyEnabled);
#endif

    if(GetGps1Chan() != 0xff)
    {
#ifdef GPS_BUILD
       retCode = SC_InitGpsConfig(&gpsConfig);
#else
       printf("Error: Not GPS build\n");
       retCode = -1;
#endif
    }
    if(retCode != 0)
    {
       printf("Error: SC_InitGpsConfig failed with code: %d\n", retCode);
       goto terminateParse;
     }
   }

   setE1T1OutEnable(o_isE1T1OutEnable);

terminateParse:
  if(fp_cfg)
    fclose(fp_cfg);

  return retCode;
}

int parseInitChanConfig(char *line, SC_t_ChanConfig *channel)
{
  char *token;
  char *savePtr;

  const char *kDelimiters = " .,\t";

  //clean the structure
  memset(channel, 0, sizeof(SC_t_ChanConfig));

  token = strtok_r(line, kDelimiters, &savePtr); //ignore this, we already know the first item

  //get e_ChanType
  token = strtok_r(NULL, kDelimiters, &savePtr);
  if((token == NULL) || (stringToChanType(token, &(channel->e_ChanType)) != 0))
  {
    printf("Error: Invalid value for e_ChanType\n");
    printf("Channel: |%s|\n", token);
    return -1;
  }

  //get b_ChanFreqPrio
  token = strtok_r(NULL, kDelimiters, &savePtr);
  if(token != NULL && isdigit(*token))
    channel->b_ChanFreqPrio = atoi(token);
  else
  {
    printf("Error: Invalid value for b_ChanFreqPrio\n");
    return -1;
  }

  //get o_ChanFreqEnabled
  token = strtok_r(NULL, kDelimiters, &savePtr);
  if(token != NULL && isalnum(*token))
    channel->o_ChanFreqEnabled = trueFalseCharToInt(*token);
  else
  {
    printf("Error: Invalid value for o_ChanFreqEnabled\n");
    return -1;
  }

  //get b_ChanTimePrio
  token = strtok_r(NULL, kDelimiters, &savePtr);
  if(token != NULL  && isdigit(*token))
    channel->b_ChanTimePrio = atoi(token);
  else
  {
    printf("Error: Invalid value for b_ChanTimePrio\n");
    return -1;
  }

  //get o_ChanTimeEnabled
  token = strtok_r(NULL, kDelimiters, &savePtr);
  if(token != NULL && isalnum(*token))
    channel->o_ChanTimeEnabled = trueFalseCharToInt(*token);
  else
  {
    printf("Error: Invalid value for o_ChanTimeEnabled\n");
    return -1;
  }

  //get o_ChanAssumedQLenabled
  token = strtok_r(NULL, kDelimiters, &savePtr);
  if(token != NULL && isalnum(*token))
    channel->o_ChanAssumedQLenabled = trueFalseCharToInt(*token);
  else
  {
    printf("Error: Invalid value for o_ChanAssumedQLenabled\n");
    return -1;
  }

  //get b_ChanFreqAssumedQL
  token = strtok_r(NULL, kDelimiters, &savePtr);
  if(token != NULL && isdigit(*token))
    channel->b_ChanFreqAssumedQL = atoi(token);
  else
  {
    printf("Error: Invalid value for b_ChanFreqAssumedQL\n");
    return -1;
  }

  //get b_ChanTimeAssumedQL
  token = strtok_r(NULL, kDelimiters, &savePtr);
  if(token != NULL && isdigit(*token))
    channel->b_ChanTimeAssumedQL = atoi(token);
  else
  {
    printf("Error: Invalid value for b_ChanTimeAssumedQL\n");
    return -1;
  }

  //get l_MeasRate
  token = strtok_r(NULL, kDelimiters, &savePtr);
  if(token != NULL && isdigit(*token))
    channel->l_MeasRate = atoi(token);
  else
  {
    printf("Error: Invalid value for b_ChanMeasTimeItv\n");
    return -1;
  }

  return 0;
}

void setPTPProtocolConfig(t_ptpGeneralConfig *genConfig, t_ptpPortConfig *ptpPortCfg,
    t_ptpProfileConfig *profileConfig)
{
  const char userDescription[] = "Symmetricom IEEE1588-2008 SCi";
  int i;

  //clean the structure and fill defaults
  memset(genConfig, 0, sizeof(t_ptpGeneralConfig));
  genConfig->e_clkClass = k_DEFAULT_CLK_CLASS;
  genConfig->b_prio1 = k_DEFAULT_PRIORITY;
  genConfig->b_prio2 = k_DEFAULT_PRIORITY;
  genConfig->w_scldVar = k_DEFAULT_VARIANCE;
  genConfig->b_domNmb = k_DEFAULT_DOMAIN;
  genConfig->o_slaveOnly = TRUE;
  genConfig->o_mgmtEnable = TRUE;
  for (i=0; i<k_CLOCK_ID_SIZE; i++)
    genConfig->s_ptpClkId.ab_ptpClkId[i] = k_DEFAULT_CLK_ID_BYTE;
  genConfig->c_userDescription[0] = 0;
  strncat((char*)genConfig->c_userDescription,userDescription, k_MAX_USR_DES);

  //clean the structure and fill defaults
  memset(ptpPortCfg, 0, sizeof(t_ptpPortConfig));
  ptpPortCfg->e_delMech = k_DEFAULT_DEL_MECH;
  ptpPortCfg->c_delReqIntv = k_DEFAULT_DEL_INTV;
  ptpPortCfg->c_anncIntv = k_DEFAULT_ANNC_INTV;
  ptpPortCfg->b_anncRcptTmo = k_DEFAULT_ANNC_RCPT_TMO;
  ptpPortCfg->c_syncIntv = k_DEFAULT_SYNC_INTV;
  ptpPortCfg->c_pdelReqIntv = k_DEFAULT_PDEL_INTV;
  ptpPortCfg->o_twoStep = k_DEFAULT_TWO_STEP;

  //clean the structure and fill defaults
  profileConfig->e_ptpMode = e_ADDR_MODE_UNICAST;
  profileConfig->e_bmcMode = e_BMC_MODE_DEFAULT;
}

int parseIPAddress(char *line, t_PortAddr *address)
{
  char *token;
  char *savePtr;
  int int_data;
  const char *kDelimiters = " .,\t";

  int i;

  //clean the structure and fill defaults
  if(address != NULL)
  {
    memset(address, 0, sizeof(t_PortAddr));
  }

  token = strtok_r(line, kDelimiters, &savePtr); //ignore this, we already know the first item

  //get IP address
  for(i = 0; i < k_DEFAULT_NETW_ADDR_LEN; i++)
  {
    token = strtok_r(NULL, kDelimiters, &savePtr);
    if(token != NULL && isdigit(*token))
    {
      int_data = atoi(token);
      if(int_data >= 0 && int_data < 256)
      {
        if(address != NULL)
        {
          address->ab_addr[i] = (UINT8)int_data;
        }
      }
      else
      {
        printf("Error: Out of range number for IP address.\n");
        return -1;
      }
    }
    else
    {
      printf("Error: Invalid value for IP address.\n");
      return -1;
    }
  }

  //IP address parsed OK set the other values
  if(address != NULL)
  {
    address->e_netwProt = k_DEFAULT_NETW_PROTOCOL;
    address->w_addrLen = k_DEFAULT_NETW_ADDR_LEN;
  }

  return 0;
}

void chanConfigToString(char *s, int bufSize, SC_t_ChanConfig *theChan)
{
  char tempStr[256];

  *s=0;

  tempStr[0]=0;
  commandToString(tempStr, 256, e_InitChanConfig);
  strcat(tempStr, " ");
  strncat_local(s, tempStr, 256);

  tempStr[0]=0;
  if(chanTypeToString(tempStr, 256, theChan->e_ChanType) != 0)
  {
    //unknown item, use the number
    sprintf(tempStr, "%d", theChan->e_ChanType);
  }
  strncat_local(tempStr, " ", 256);
  strncat_local(s, tempStr, 256);

  tempStr[0]=0;
  sprintf(tempStr, "%d %c %d %c ", theChan->b_ChanFreqPrio, trueFalseIntToChar(theChan->o_ChanFreqEnabled),
      theChan->b_ChanTimePrio, trueFalseIntToChar(theChan->o_ChanTimeEnabled));
  strncat_local(s, tempStr, 256);

  tempStr[0]=0;
  sprintf(tempStr, "%c %d %d ", trueFalseIntToChar(theChan->o_ChanAssumedQLenabled), theChan->b_ChanFreqAssumedQL,
      theChan->b_ChanTimeAssumedQL);
  strncat_local(s, tempStr, 256);

  tempStr[0]=0;
  sprintf(tempStr, "%d\n", theChan->l_MeasRate);
  strncat_local(s, tempStr, 256);
}

void pTPUMAddressToString(char *s, int bufSize, t_PortAddr *address)
{
  char tempStr[256];
  int i;

  *s=0;

  tempStr[0]=0;
  commandToString(tempStr, 256, e_UnicastMasterAddress);
  strcat(tempStr, " ");
  strncat_local(s, tempStr, 256);

  for(i = 0; i < k_DEFAULT_NETW_ADDR_LEN; i++)
  {
    tempStr[0]=0;
    if(i < (k_DEFAULT_NETW_ADDR_LEN-1))
      sprintf(tempStr, "%d.", (int)address->ab_addr[i]);
    else
      sprintf(tempStr, "%d\n", (int)address->ab_addr[i]);
    strncat_local(s, tempStr, 256);
  }
}

//similar to strncat, but the RESULT string will NOT be longer than n
char inline *strncat_local(char *dest, const char *src, size_t n)
{
  return strncat(dest, src, n-strlen(dest)-1);
}

int stringToChanType(const char *s, SC_t_Chan_Type *chan)
{
  int index = 0;

  if(s == NULL)
    return -1;

  for(index = 0; index < sizeof(kChanName)/sizeof(t_ChanTypeName); index++)
    if(strcmp(s, kChanName[index].name) == 0)
    {
      if(chan != NULL)
        *chan = kChanName[index].chan;
      return 0;
    }
  return -1;
}

int stringToAccessTransportType(const char *s, t_ptpAccessTransportEnum *transport)
{
  int index = 0;

  if(s == NULL)
    return -1;

  for(index = 0; index < sizeof(kAccessTransportName)/sizeof(t_AccessTransportName); index++)
    if(strcmp(s, kAccessTransportName[index].name) == 0)
    {
      if(transport != NULL)
        *transport = kChanName[index].chan;
      return 0;
    }
  return -1;
}

int accessTransportTypeToString(const t_ptpAccessTransportEnum transport, const int bufSize, char *s)
{
  int index = 0;

  for(index = 0; index < sizeof(kAccessTransportName)/sizeof(t_AccessTransportName); index++)
    if(kAccessTransportName[index].transport == transport)
    {
      *s=0;
      strncat(s, kAccessTransportName[index].name, bufSize-1);
      return 0;
    }
  sprintf(s, "Unrecognized value: %d", transport);
  return -1;
}

int chanTypeToString(char *s, const int bufSize, SC_t_Chan_Type chan)
{
  int index = 0;

  if(s == NULL)
    return -1;

  for(index = 0; index < sizeof(kChanName)/sizeof(t_ChanTypeName); index++)
    if(chan == kChanName[index].chan)
    {
      *s=0;
      strncat(s, kChanName[index].name, bufSize-1);
      return 0;
    }
  sprintf(s, "Unrecognized value: %d", chan);
  return -1;
}

int stringToCommand(const char *s, t_ConfigCommand *cmd)
{
  int index = 0;

  if(s == NULL)
    return -1;


  for(index = 0; index < sizeof(kConfigCommandName)/sizeof(t_ConfigCommandName); index++)
    if(strcmp(s, kConfigCommandName[index].name) == 0)
    {
      if(cmd != NULL)
        *cmd = kConfigCommandName[index].cmd;
      return 0;
    }
  return -1;
}

int commandToString(char *s, int bufSize, t_ConfigCommand cmd)
{
  int index = 0;

  if(s == NULL)
    return -1;

  for(index = 0; index < sizeof(kConfigCommandName)/sizeof(t_ConfigCommandName); index++)
    if(cmd == kConfigCommandName[index].cmd)
    {
      *s=0;
      strncat(s, kConfigCommandName[index].name, bufSize-1);
      return 0;
    }
  sprintf(s, "Unrecognized value: %d", cmd);
  return -1;
}

void readOffsetFromFile(t_servoConfigType *config)
{
  char line[256];
  char desc[256];
  FILE* fp_cfg;
  FLOAT32 df_data;
  UINT32  dw_data;

  fp_cfg = fopen(kOffsetFileName, "r");
  if(fp_cfg)
  {
    while (fgets(line, 255, fp_cfg) != NULL)
    {
      sscanf(line, "%s %f", desc, &df_data);
      if (strcmp(desc, FreqCorrToken) == 0)
      {
        config->f_freqCorr = df_data;
        continue;
      }
      sscanf(line, "%s %d", desc, &dw_data);
      if (strcmp(desc, FreqCorrTimeStampToken) == 0)
      {
        config->s_freqCorrTime.u48_sec = dw_data;
        config->s_freqCorrTime.dw_nsec = 0;
        continue;
      }
    }
    fclose(fp_cfg);
  }
  else
    printf("Warning: file \"%s\" does not exist. Will be created later.\n", kOffsetFileName);
}

/*
----------------------------------------------------------------------------
                                SC_WriteFreqCorrToFile()

Description:
This function stores the frequency correction data into configuration
file.

Parameters:

Inputs
        FLOAT32  f_freqCorr
        The current frequency correction in ppb.

        t_ptpTimeStampType  s_freqCorrTime
        The timestamp for hourly frequency offset

Outputs
        None

Return value:
    0:          function succeeded
    -1:         error

-----------------------------------------------------------------------------
*/
int SC_WriteFreqCorrToFile(
    FLOAT32             f_freqCorr,
    t_ptpTimeStampType  s_freqCorrTime
)
{
  FILE* fp_cfg;

  fp_cfg = fopen(kOffsetFileName, "w");
  if(fp_cfg)
  {
    errno = 0;
    fprintf(fp_cfg, "%s\t\t%e\n", FreqCorrToken, (FLOAT64)f_freqCorr);
    fprintf(fp_cfg, "%s\t\t%d\n", FreqCorrTimeStampToken, (INT32)s_freqCorrTime.u48_sec);

    fclose(fp_cfg);
  }
  else
  {
    printf("Error: SC_WriteFreqCorrToFile failed.\n");
    printf("fopen(\"%s\", \"w\") failed with error %d, %s\n", kOffsetFileName, errno, strerror(errno));
    return -1;
  }
  return 0;
}
/*
----------------------------------------------------------------------------
                                GetGps1Chan()

Description:
Returns the channel number for GPS1.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of GPS (returns 0xFF if false);

Global variables affected and border effects:
  Read
    gGPSChannelNumber

  Write
    none

-----------------------------------------------------------------------------
*/
UINT8 GetGps1Chan(void)
{
   return gGPS1ChannelNumber;
}

/*
----------------------------------------------------------------------------
                                GetGps2Chan()

Description:
Returns the channel number for GPS2.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of GPS (returns 0xFF if false);

Global variables affected and border effects:
  Read
    gGPSChannelNumber

  Write
    none

-----------------------------------------------------------------------------
*/
UINT8 GetGps2Chan(void)
{
   return gGPS2ChannelNumber;
}

/*
----------------------------------------------------------------------------
                                GetGps3Chan()

Description:
Returns the channel number for GPS3.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of GPS (returns 0xFF if false);

Global variables affected and border effects:
  Read
    gGPSChannelNumber

  Write
    none

-----------------------------------------------------------------------------
*/
UINT8 GetGps3Chan(void)
{
   return gGPS3ChannelNumber;
}

/*
----------------------------------------------------------------------------
                                GetGps4Chan()

Description:
Returns the channel number for GPS4.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of GPS (returns 0xFF if false);

Global variables affected and border effects:
  Read
    gGPSChannelNumber

  Write
    none

-----------------------------------------------------------------------------
*/
UINT8 GetGps4Chan(void)
{
   return gGPS4ChannelNumber;
}
/*
----------------------------------------------------------------------------
                                GetSe1Chan()

Description:
Returns the channel number for SYNC-E 1.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of SYNC-E 1 (returns 0xFF if false);

Global variables affected and border effects:
  Read
    gSe1ChannelNumber

  Write
    none

-----------------------------------------------------------------------------
*/
UINT8 GetSe1Chan(void)
{
   return gSe1ChannelNumber;
}
/*
----------------------------------------------------------------------------
                                GetSe2Chan()

Description:
Returns the channel number for Sync-E 2.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of SYNC-E 2 (returns 0xFF if false);

Global variables affected and border effects:
  Read
    gSe2ChannelNumber

  Write
    none

-----------------------------------------------------------------------------
*/
UINT8 GetSe2Chan(void)
{
   return gSe2ChannelNumber;
}

/*
----------------------------------------------------------------------------
                                GetE1Chan()

Description:
Returns the channel number for E1 channel.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of E1 (returns 0xFF if false);

Global variables affected and border effects:
  Read
    gSe2ChannelNumber

  Write
    none

-----------------------------------------------------------------------------
*/
UINT8 GetE1Chan(void)
{
   return gE1ChannelNumber;
}
/*
----------------------------------------------------------------------------
                                GetT1Chan()

Description:
Returns the channel number for T1 channel.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of T1 (returns 0xFF if false);

Global variables affected and border effects:
  Read
    gSe2ChannelNumber

  Write
    none

-----------------------------------------------------------------------------
*/
UINT8 GetT1Chan(void)
{
   return gT1ChannelNumber;
}
/*
----------------------------------------------------------------------------
                                GetRedChan()

Description:
Returns the channel number for Redundancy channel.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of Redundancy channel (returns 0xFF if false);

Global variables affected and border effects:
  Read
    gRedChannelNumber

  Write
    none

-----------------------------------------------------------------------------
*/
UINT8 GetRedChan(void)
{
   return gRedChannelNumber;
}

/*
----------------------------------------------------------------------------
                                GetExtAChan()

Description:
Returns the channel number for Ext Ref A channel.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of Ext Ref channel (returns 0xFF if false);

Global variables affected and border effects:
  Read
    gExtAChannelNumber

  Write
    none

-----------------------------------------------------------------------------
*/
UINT8 GetExtAChan(void)
{
   return gExtAChannelNumber;
}

/*
----------------------------------------------------------------------------
                                GetExtBChan()

Description:
Returns the channel number for Ext Ref B channel.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of Ext Ref channel (returns 0xFF if false);

Global variables affected and border effects:
  Read
    gExtBChannelNumber

  Write
    none

-----------------------------------------------------------------------------
*/
UINT8 GetExtBChan(void)
{
   return gExtBChannelNumber;
}

/*
----------------------------------------------------------------------------
                                GetEth0Chan()

Description:
Returns the channel number oc the channel that is using the Eth0 as the 
frequency reference.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of Eth0 (returns 0xFF if false);

-----------------------------------------------------------------------------
*/
UINT8 GetEth0Chan(void)
{
   return gEth0ChannelNumber;
}
/*
----------------------------------------------------------------------------
                                GetEth1Chan()

Description:
Returns the channel number oc the channel that is using the Eth1 as the 
frequency reference.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    Channel number of Eth1 (returns 0xFF if false);

-----------------------------------------------------------------------------
*/
UINT8 GetEth1Chan(void)
{
   return gEth1ChannelNumber;
}

/*
----------------------------------------------------------------------------
                                GetRedundancyChan()

Description:
Returns the channel number of the channel that is used for redundancy.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    gRedChannelNumber (returns 0xFF if false);

-----------------------------------------------------------------------------
*/
UINT8 GetRedundancyChan(void)
{
   return gRedChannelNumber;
}

/*
----------------------------------------------------------------------------
                                GetFreqOnlyChan()

Description:
Returns the channel number of the channel that is used for frequency only.

Parameters:

Inputs
        None

Outputs
        None

Return value:
    GetFreqOnlyChan (returns 0xFF if false);

-----------------------------------------------------------------------------
*/
UINT8 GetFreqOnlyChan(void)
{
  return gFreqOnlyChannelNumber;
}

/*--------------------------------------------------------------------------
    void verifyConfigFile(void)

Description:
Is there is not a configuration copies a default file from factoryConfigFileName
to kConfigFileName

Return value:
  void

Global variables affected and border effects:
  Read
    kConfigFileName

  Write
--------------------------------------------------------------------------*/
static void verifyConfigFile(void)
{
  const char *factoryConfigFileName = "/active/appl/sc.cfg";
  FILE *inFile = NULL;
  FILE *outFile = NULL;
  int aByte;

  //check if config exists
  outFile = fopen(kConfigFileName, "r");
  if(outFile != NULL)
  { //File exists, close and return
    fclose(outFile);
    return;
  }

  //need to copy default file
  inFile = fopen(factoryConfigFileName, "rb");
  if(inFile != NULL)
  {
    outFile = fopen(kConfigFileName, "wb");
    if(outFile != NULL)
    {
      printf("Creating default configuration file from %s\n", factoryConfigFileName);
      while ((aByte = fgetc(inFile)) != EOF)
          fputc(aByte, outFile);
      fclose(outFile);
      printf("Configuration file %s created.\n", kConfigFileName);
    }
    else
      printf("Error: cannot create configuration file: %s\n", kConfigFileName);

    fclose(inFile);
  }
  else
    printf("Error: no default configuration file: %s\n", factoryConfigFileName);

  return;
}
