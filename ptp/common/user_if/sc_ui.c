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

FILE NAME    : sc_ui.c

AUTHOR       : Ken Ho

DESCRIPTION  :

This file implements simple user interface for SCRDB.

Revision control header:
$Id: user_if/sc_ui.c 1.65 2012/02/09 12:02:44PST German Alvarez (galvarez) Exp  $

******************************************************************************
*/

/*************************************************************************
**    include-files
*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/time.h>
#include "sc_types.h"
#include "sc_api.h"
#include "sc_chan.h"
#include "sc_ui.h"
#include "clock_if/sc_clock_if.h"
#include "local_debug.h"
#include "log/sc_log.h"
#include "config_if/sc_readconfig.h"
#include "chan_processing_if/sc_SSM_handling.h"
#include "redundancy_if/sc_redundancy_if.h"
#include "../main.h"

/*************************************************************************
**    global variables
*************************************************************************/
static BOOLEAN go_printTimeCode = FALSE;
static BOOLEAN o_terminating = FALSE;

#ifdef GPS_BUILD
static SC_t_GPS_Status s_gpsInfo;
#endif

static t_stateDescriptions as_ptpPrivateStatusField[] =
{
        {e_PS_TP500PERSONALITY,         " "},
        {e_PS_FLLSTATE,                 "Current FLL State"},
        {e_PS_FLLSTATEDURATION,         "FLL State Duration (minutes)"},
        {e_PS_SELECTEDMASTERIP,         "Reference"},
        {e_PS_FORWARDWEIGHT,            "Forward Flow Weight (%)"},
        {e_PS_FORWARDTRANS900,          "Forward Flow Transient-free (out of 900 s)"},
        {e_PS_FORWARDTRANS3600,         "Forward Flow Transient-free (out of 3600 s)"},
        {e_PS_FORWARDTRANSUSED,         "Forward Flow Percentile Clustering (%)"},
        {e_PS_FORWARDOPMINTDEV,         "Forward Flow Operational Min TDEV (ns)"},
        {e_PS_FORWARDMINCLUWIDTH,       "Forward Flow Min Cluster Width (ns)"},
        {e_PS_FORWARDMODEWIDTH,         "Forward Flow Mode Width (ns)"},
        {e_PS_FORWARDGMSYNCPSEC,        "Timing Packet Rate GM1 (pkts/s)"},
        {e_PS_FORWARDACCGMSYNCPSEC,     "Timing Packet Rate GM2 (pkts/s)"},
        {e_PS_REVERSEDWEIGHT,           "Reverse Flow Weight (%)"},
        {e_PS_REVERSEDTRANS900,         "Reverse Flow Transient-free (out of 900 s)"},
        {e_PS_REVERSEDTRANS3600,        "Reverse Flow Transient-free (out of 3600 s)"},
        {e_PS_REVERSEDTRANSUSED,        "Reverse Flow Percentile Clustering (%)"},
        {e_PS_REVERSEDOPMINTDEV,        "Reverse Flow Operational Min TDEV (ns)"},
        {e_PS_REVERSEDMINCLUWIDTH,      "Reverse Flow Min Cluster Width (ns)"},
        {e_PS_REVERSEDMODEWIDTH,        "Reverse Flow Mode Width (ns)"},
        {e_PS_REVERSEDGMSYNCPSEC,       "Timing Packet Rate Delay (pkts/s)"},
        {e_PS_REVERSEDACCGMSYNCPSEC,    "GM2 Timing Packet Rate Delay (pkts/s)"},  //not used
        {e_PS_OUTPUTTDEVESTIMATE,       "Output TDEV Estimate (ns)"},
        {e_PS_FREQCORRECT,              "Correction Frequency (ppb)"},
        {e_PS_PHASECORRECT,             "Phase correction (ppb)"},
        {e_PS_RESIDUALPHASEERROR,       "Residual phase error (ns)"},
        {e_PS_MINTIMEDISPERSION,        "Minimal RTD (us)"},
        {e_PS_MASTERFLOWSTATE,          "GM1 Flow State"},
        {e_PS_ACCMASTERFLOWSTATE,       "GM2 Flow State"},
        {e_PS_MASTERCLOCKID,            "GM1 Clock id"},
        {e_PS_ACCMASTERCLOCKID,         "GM2 Clock id"},
        {e_PS_LASTUPGRADESTATUS,        "Last Firmware Upgrade status"},
        {e_PS_OP_TEMP_MAX,              "Operational Temperature Max (deg C)"},
        {e_PS_OP_TEMP_MIN,              "Operational Temperature Min (deg C)"},
        {e_PS_OP_TEMP_AVG,              "Operational Temperature Avg (deg C)"},
        {e_PS_TEMP_STABILITY_5MIN,      "5  Minute Temperature Stability (mdeg C)"},
        {e_PS_TEMP_STABILITY_60MIN,     "60 Minute Temperature Stability (mdeg C)"},
        {e_PS_IPDV_OBS_INTERVAL,        "Observation Interval (min)"},
        {e_PS_IPDV_THRESHOLD,           "IPDV Threshold (nsec)"},
        {e_PS_VAR_PACING_FACTOR,        "Pacing Factor for Jitter Computation"},
        {e_PS_FWD_IPDV_PCT_BELOW,       "Forward IPDV % Below Threshold"},
        {e_PS_FWD_IPDV_MAX,             "Forward Maximum IPDV (usec)"},
        {e_PS_FWD_INTERPKT_VAR,         "Forward InterPkt Jitter (usec)"},
        {e_PS_REV_IPDV_PCT_BELOW,       "Reverse IPDV % Below Threshold"},
        {e_PS_REV_IPDV_MAX,             "Reverse Maximum IPDV (usec)"},
        {e_PS_REV_INTERPKT_VAR,         "Reverse InterPkt Jitter (usec)"},
        {e_PS_CURRENT_TIME,             "Current time is"},
        {e_PS_FORWARDOPMAFE,            "Forward Flow Operational MAFE (ppb)"},
        {e_PS_REVERSEDOPMAFE,           "Reverse Flow Operational MAFE (ppb)"},
        {e_PS_OUTPUTMDEVESTIMATE,       "Output MDEV Estimate (ppb)"}
};

static t_stateDescriptions as_ptpFllStateDesc[] =
{
        {e_FLL_STATE_ACQUIRING,         "Acquiring"},
        {e_FLL_STATE_WARMUP,            "Warmup"},
        {e_FLL_STATE_FAST,              "Fast FLL"},
        {e_FLL_STATE_NORMAL,            "Normal FLL"},
        {e_FLL_STATE_BRIDGE,            "Bridging"},
        {e_FLL_STATE_HOLDOVER,          "Holdover"}
};

static t_stateDescriptions as_RedundancyRoleName[] =
{
        {e_SC_ACTIVE,                   "Active"},
        {e_SC_STANDBY,                  "Standby"},
        {e_SC_TRANSITION_TO_ACTIVE,     "Going Active"},
        {e_SC_TRANSITION_TO_STANDBY,    "Going Standby"},
};

t_timeCodeType gs_timeCode;
UINT32 gd_timeCodeCounter = 0;

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
#define SIZE_OF_REV_STR         80

#define LF '\n'
#define CR '\r'
#define Is_EOL(token) ((LF==(token))||(CR==(token)))

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static void print_tlv(SC_t_MntTLV *ps_Tlv);
static void print_menu(void);
static void print_2_menu(void);
static int  process_2_menu(void);
static void print_3_menu(void);
static int  process_3_menu(void);
static void print_status_list(void);
static char *get_display_title_desc(UINT16 w_titleId);
static char *get_fll_state_desc(int fll_state);
static char *get_RedundancyRoleNames(int redState);
static void print_a_menu(void);
static int  process_a_menu(void);
static void print_b_menu(void);
static int  process_b_menu(void);
static void chanConfigurationMenu(void);
static void chanAttributesMenu(void);
static void redundancyMenu(void);
static void phaseMenu(void);
static void frequencyMenu(void);
static int getChannelNumber(const int numChan);
static BOOLEAN askWantToChange(const char defaultAnswer);
static BOOLEAN askConfirmation(const char defaultAnswer);
static void timeCodeToString(const t_timeCodeType *tc, char * strBuf, size_t bufLen);
const static char *eventMapToString(const UINT32 eventMap);

/*************************************************************************
**    External functions
*************************************************************************/
extern int SC_GetVcalData(FLOAT64 *pdf_segFfo);
extern double SC_WarpFrequency(double df_fin);

/*
----------------------------------------------------------------------------
                                SC_StartUi()

Description:
This function starts the main user interface.

Inputs:
        None

Outputs:
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
int SC_StartUi(void)
{
  //   int i;
  int ret_val;
  UINT32 dw_eventMap;
  UINT8 b_data;
  INT8  ac_inputBuf[80];
  INT8  ac_revStr[SIZE_OF_REV_STR+1];
  t_logConfigType s_logConfig;
  BOOLEAN o_exit = FALSE;
  t_servoConfigType s_servoC;
  t_ptpServoStatusType s_ptpServoStatus;
  time_t time;
  char ac_buf[80];
  UINT16 w_clockRateMultiplier;

  //   FLOAT64 df_segFfo[16];
  //   FLOAT64 df_currentFreqOffset;

  /* print start message on screen */
  print_menu();
  printf("> ");

  while (1)
  {
    char c_cmd;
    fgets((void *)ac_inputBuf, sizeof(ac_inputBuf), stdin);
    sscanf((void *)ac_inputBuf, "%c", &c_cmd);
    if (!o_terminating)
    {
      switch (c_cmd)
      {
        case '1': /* get revision string */
          SC_VerInfo(ac_revStr, SIZE_OF_REV_STR);
          printf("%s\n\n", ac_revStr);
          break;
        case '2': /* PTP management menu */
          print_2_menu();
          process_2_menu();
          break;
        case '3': /* set ipdv menu */
          print_3_menu();
          process_3_menu();
          break;
        case '4': /* get debug config */
          SC_DbgConfig(&s_logConfig, e_PARAM_GET);
          printf("Debug Mode:  0x%hhd\n"  , s_logConfig.b_dbgMode);
          printf("Debug Mask:  0x%04X\n", s_logConfig.l_debugMask);
          printf("Debug TsLmt: %d\n\n", s_logConfig.l_debugTsLimit);
          break;
        case '5': /* enter debug mask */
          printf("Enter debug mask (hex): ");
          fgets((void *)ac_inputBuf, sizeof(ac_inputBuf), stdin);

          SC_DbgConfig(&s_logConfig, e_PARAM_GET);
          if (sscanf((void *)ac_inputBuf, "%x", &s_logConfig.l_debugMask) == 1)
          {
            SC_DbgConfig(&s_logConfig, e_PARAM_SET);
            printf("debug mask set to 0x%04X\n", s_logConfig.l_debugMask);
          }
          break;
        case '6': /* enter test mode */
          printf("0  = normal\n");
          printf("1  = +/-512ppb frequency step test\n");
          printf("2  = 100us phase step test\n");
          printf("3  = varactor cal mode\n");
          printf("4  = print FIFO forward 4k\n");
          printf("5  = print FIFO reverse 4k\n");
          printf("6  = print FIFO forward every 64th entry\n");
          printf("7  = print FIFO reverse every 64th entry\n");
          printf("8  = Set timestamp detection threshold\n");
          printf("9  = Print Channel Servo Info\n");
          printf("10 = Set Frequency correction to Zero\n");

          printf("\nEnter mode: ");
          SC_DbgConfig(&s_logConfig, e_PARAM_GET);

          fgets((void *)ac_inputBuf, sizeof(ac_inputBuf), stdin);
          if (sscanf((void *)ac_inputBuf, "%hhd", &b_data) == 1)
          {
            s_logConfig.b_dbgMode = b_data;

            if (b_data==8)
            {
              printf("Enter debug timestamp threshold (ns): ");
              fgets((void *)ac_inputBuf, sizeof(ac_inputBuf), stdin);
              if (sscanf((void *)ac_inputBuf, "%d", &s_logConfig.l_debugTsLimit) == 1)
              {
                SC_DbgConfig(&s_logConfig, e_PARAM_SET);
                printf("debug timestamp threshold set to %d\n", s_logConfig.l_debugTsLimit);
                break;
              }
            }
            else
            {
              printf("setting mode to %hhd\n", (UINT8)s_logConfig.b_dbgMode);

              SC_DbgConfig(&s_logConfig, e_PARAM_SET);
            }
          }
          break;
        case '7':
          SC_GetServoStatus(&s_ptpServoStatus);
          printf("SC_GetServoStatus(): state= %d duration= %d\n\n",
              (int)s_ptpServoStatus.e_fllState,
              (int)s_ptpServoStatus.dw_fllStateDur);
          break;
        case '8':
          SC_GetServoConfig(&s_servoC);

          printf("Local Oscillator   : ");
          switch (s_servoC.e_loType)
          {
            case e_OCXO:
              printf("OCXO");
              break;
            case e_MINI_OCXO:
              printf("Mini OCXO");
              break;
            case e_RB:
              printf("Rb");
              break;
            default:
              printf("Unknown");
              break;
          }

          printf("\nLocal Oscillator QL: %"PRIu8, s_servoC.b_loQl);

          accessTransportTypeToString(s_servoC.e_ptpTransport, sizeof(ac_buf), ac_buf);
          printf("\nPTP Transport      : %s", ac_buf);

          if (s_servoC.dw_ptpPhaseMode & k_PTP_PHASE_MODE_STANDARD)
          {
            printf("\nPhase Mode         : STANDARD");
          }
          else
          {
            printf("\nPhase Mode         : COUPLED");
          }

          if (s_servoC.dw_ptpPhaseMode & k_PTP_PHASE_MODE_INBAND)
          {
            printf(", INBAND\n\n");
          }
          else
          {
            printf(", OUTBAND\n\n");
          }
          printf("\nFcorr              : %e", s_servoC.f_freqCorr);
          time = s_servoC.s_freqCorrTime.u48_sec;
          printf("\nFcorr Time         : %lld : %s",
              (UINT64)s_servoC.s_freqCorrTime.u48_sec,
              ctime_r(&time, ac_buf));

          printf("Factory Fcorr      : %e", s_servoC.f_freqCalFactory);
          time = s_servoC.s_freqCalFactoryTime.u48_sec;
          printf("\nFactory Fcorr Time : %lld : %s\n",
              (UINT64)s_servoC.s_freqCalFactoryTime.u48_sec,
              ctime_r(&time, ac_buf));
          break;
#if 0
            case '9':
              ret_val = SC_GetVcalData(df_segFfo);
              if(ret_val)
              {
                printf("Calibration has not been completed\n");
              }
              else
              {
                printf("Calibration has been completed\n");
              }

              df_currentFreqOffset = -4512.0;
              for(i=0;i<16;i++)
              {
                printf("%d Programmed input: %e ppb, measured output %e\n", i, df_currentFreqOffset, df_segFfo[i]);
                df_currentFreqOffset += 64.0;
              }
              break;
#endif
            case 'a': case 'A':
              print_a_menu();
              process_a_menu();
              break;

            case 'b': case 'B':
              print_b_menu();
              process_b_menu();
              break;

            case 'c': case 'C':
              chanConfigurationMenu();
              break;

            case 'd': case 'D':
              SC_GetEventMap(&dw_eventMap);
              printf("The event bitmap: 0x%x\n\n", (int)dw_eventMap);

              if(dw_eventMap & (1 << e_HOLDOVER))
                printf("(0x%03x) Holdover Event\n", (int)(1 << e_HOLDOVER));
              if(dw_eventMap & (1 << e_FREERUN))
                printf("(0x%03x) Freerun Event\n", (int)(1 << e_FREERUN));
              if(dw_eventMap & (1 << e_BRIDGING))
                printf("(0x%03x) Bridging Event\n", (int)(1 << e_BRIDGING));
              if(dw_eventMap & (1 << e_CHAN_NO_DATA))
                printf("(0x%03x) No Channel Data Event\n", (int)(1 << e_CHAN_NO_DATA));
              if(dw_eventMap & (1 << e_CHAN_PERFORMANCE))
                printf("(0x%03x) Channel Performance Event\n", (int)(1 << e_CHAN_PERFORMANCE));
              if(dw_eventMap & (1 << e_FREQ_CHAN_QUALIFIED))
                printf("(0x%03x) Frequency Channel Qualified Event\n", (int)(1 << e_FREQ_CHAN_QUALIFIED));
              if(dw_eventMap & (1 << e_FREQ_CHAN_SELECTED))
                printf("(0x%03x) Frequency Channel Selected Event\n", (int)(1 << e_FREQ_CHAN_SELECTED));
              if(dw_eventMap & (1 << e_TIME_CHAN_QUALIFIED))
                printf("(0x%03x) Time Channel Qualified Event\n", (int)(1 << e_TIME_CHAN_QUALIFIED));
              if(dw_eventMap & (1 << e_TIME_CHAN_SELECTED))
                printf("(0x%03x) Time Channel Selected Event\n", (int)(1 << e_TIME_CHAN_SELECTED));
              if(dw_eventMap & (1 << e_CHAN_NOT_VALID))
                printf("(0x%03x) Channel Not Valid Event\n", (int)(1 << e_CHAN_NOT_VALID));
              if(dw_eventMap & (1 <<  e_TIMELINE_CHANGED))
                printf("(0x%03x) Timeline Changed Event\n", (int)(1 << e_TIMELINE_CHANGED));
              if(dw_eventMap & (1 << e_PHASE_ALIGNMENT))
                printf("(0x%03x) Phase Alignment Event\n", (int)(1 << e_PHASE_ALIGNMENT));
              break;

              /* SC_Shutdown() */
            case 'e': case 'E':
              printf("Stop and Restart all SoftClock process.\n");
              if(askConfirmation('Y'))
              {
                ret_val = SC_Shutdown();

                if(ret_val)
                {
                  printf("Failed SC_Shutdown(), return value: %d\n", ret_val);
                }
                /* reinitialize configuration */
                else
                {
                  printf("SC_Shutdown() complete, running configuration\n");
                  /* stop timer */
                  TimerStop();

                  /* set-up configuration from sc.cfg */
                  readConfigFromFile();

                  /* init complete */
                  /* complete initialization */#include "sc_chan.h"

                  w_clockRateMultiplier = 1; /* timer at tick rate, multiplier = 1 */
                  if (SC_InitConfigComplete(w_clockRateMultiplier) < 0)
                  {
                    printf("SoftClock cannot be initialized\n");
                    printf("SoftClock stopped\n");
                    exit(0);
                  }

                  /* start timer */
                  if (TimerStart(w_clockRateMultiplier) < 0)
                  {
                    SC_Shutdown();
                    printf("Start timer failed\n");
                    printf("SoftClock stopped\n");
                    exit(0);
                  }
                }
              }

              break;

            case 'f': case 'F':
              frequencyMenu();
              break;

#ifdef GPS_BUILD
            case 'g': case 'G':
              if (SC_GpsStat(&s_gpsInfo) == 0)
              {
                int i;

                printf("Valid 2-D Fix:        %s\n", (s_gpsInfo.Valid_2D_Fix ? "TRUE" : "FALSE"));
                printf("Valid 3-D Fix:        %s\n", (s_gpsInfo.Valid_3D_Fix ? "TRUE" : "FALSE"));
                printf("Satellites in View:   %d\n", s_gpsInfo.SatsInView);
                printf("Satellites Used:      %d\n\n", s_gpsInfo.SatsUsed);

                printf("SVID\t\tSNR [dBHz]\tAzim [deg]\tElev [deg]\tUsed\n");
                for (i = 0; i < k_NMEA_SV; i++)
                {
                  printf("%d\t\t%d\t\t%d\t\t%d\t\t%s\n",
                      s_gpsInfo.SatsInViewSVid[i],
                      s_gpsInfo.SatsInViewSNR[i],
                      s_gpsInfo.SatsInViewAzim[i],
                      s_gpsInfo.SatsInViewElev[i],
                      (s_gpsInfo.SatsInViewUsed[i] ? "TRUE" : "FALSE"));
                }

                printf("\n");
                printf("Latitude:             %f [deg]\n", s_gpsInfo.Latitude);
                printf("Longitude             %f [deg]\n", s_gpsInfo.Longitude);
                printf("Altitude (Ell):       %f [m]\n", s_gpsInfo.Altitude_Ell);
                printf("Altitude (MSL):       %f [m]\n", s_gpsInfo.Altitude_MSL);

                printf("HDOP:                 %f\n", s_gpsInfo.H_DOP);
                printf("VDOP:                 %f\n", s_gpsInfo.V_DOP);
                printf("PDOP:                 %f\n", s_gpsInfo.P_DOP);
                printf("TDOP:                 %f\n", s_gpsInfo.T_DOP);
              }
              break;
#endif /* GPS_BUILD */

            case 'l': case 'L':
              if(getLocalEventLog() == NULL)
                printf("Active events log not initialized\n");
              else
                printf("Active events log:\n%s\n", getLocalEventLog());
              break;

#if 0
              case 'm': case 'M':
              printf("Channel index for Load Mode query: ");
              fgets((void *)ac_inputBuf, sizeof(ac_inputBuf), stdin);
              if (sscanf((void *)ac_inputBuf, "%d", &l_loadmodechan) == 1)
              {
                 if(SC_LoadMode((UINT8)l_loadmodechan, &w_loadmode))
                 {
                    printf("Error using SC_LoadMode()\n");
                 }
                 else
                 {
                    printf("Channel index %d is Load Mode %hd\n", l_loadmodechan, w_loadmode);
                 }              
              }
              else
              {
                 printf("Error in reading channel index\n");
              }
              break;
#endif
            case 'p': case 'P':
              phaseMenu();
              break;

#if (SC_RED_CHANNELS > 0)
            case 'r': case 'R':
              redundancyMenu();
              break;
#endif

            case 's': case 'S':
              print_status_list();
              break;

            case 't': case 'T':
              go_printTimeCode = !go_printTimeCode;
              break;

#if 0
              /* SC_RestartServo() */
            case 'u': case 'U':
              printf("Restart Servo.\n");
              if(askConfirmation('Y'))
              {
                ret_val = SC_RestartServo();

                if(ret_val)
                {
                  printf("Failed SC_RestartServo(), return value: %d\n", ret_val);
                }
                /* reinitialize configuration */
                else
                {
                  printf("SC_RestartServo() complete\n");
                }
              }
              break;
#endif

            case 'x': case 'X':
              printf("Exit program and terminate all SoftClock process.\n");
              if(askConfirmation('Y'))
                o_exit = TRUE;
              break;

            case 'h': case 'H': case '?':
              print_menu();
              break;

            default:
              break;
      } /*switch*/
    }
    else
    {
      o_exit = TRUE;
    }
    if (o_exit || o_terminating)
      break;
    printf("> ");
  }

  return 0;
}

/*
----------------------------------------------------------------------------
                                SC_StopUi()

Description:
This function terminates the user interface.

Inputs:
        None

Outputs:
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
int SC_StopUi(void)
{
   o_terminating = TRUE;
   return 0;
}

/*************************************************************************
**    static functions
*************************************************************************/

/*
----------------------------------------------------------------------------
                                print_menu()

Description:
This function prints the main menu.

Inputs:
        None

Outputs:
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
static void print_menu(void)
{
    printf("Select from following:\n");
    printf("\t1  - Get Revision Info\n");
    printf("\t2  - PTP Management Messages\n");
    printf("\t3  - Set IPDV\n");
    printf("\t4  - Get Debug Configuration\n");
    printf("\t5  - Set Debug Mask\n");
    printf("\t6  - Set Debug Test Mode/Limit\n");
    printf("\t7  - SC_GetServoStatus\n");
    printf("\t8  - SC_GetServoConfig\n");
//    printf("\t9  - Read V cal table\n");
    printf("\ta  - Set Local Debug Mask\n");
    printf("\tb  - Set Local Debug Action\n");
    printf("\tc  - Channel Configuration Options\n");
    printf("\td  - Event Bitmap\n");
    printf("\te  - SC_Shutdown() and Reinitialize\n");
    printf("\tf  - Programmable Frequency Output Menu\n");
#ifdef GPS_BUILD
    printf("\tg  - Print GPS Information\n");
#endif
    printf("\tl  - Print Active Events Log\n");
#if 0
    printf("\tm  - Print Load Mode\n");
#endif
    printf("\tp  - Phase Options\n");
#if (SC_RED_CHANNELS > 0)
    printf("\tr  - Redundancy Options\n");
#endif
    printf("\ts  - Get Status\n");
    printf("\tt  - Toggle Timecode Print\n");
    //printf("\tu  - SC_RestartServo()\n");
    printf("\tx  - Exit\n");
    printf("\th  - Print this List\n");
}

/*
----------------------------------------------------------------------------
                                print_2_menu()

Description:
This function prints menu 2.

Inputs:
        None

Outputs:
   None

Return value:
        None

-----------------------------------------------------------------------------
*/
static void print_2_menu(void)
{
    printf("Select from following:\n");
    printf("\t1  - Get Clock Description\n");
    printf("\t2  - Get User Description\n");
    printf("\t3  - Set User Description\n");
    printf("\t4  - Get Domain\n");
    printf("\t5  - Set Domain\n");
    printf("\t6  - Enable port\n");
    printf("\t7  - Disable port\n");
    printf("\t8  - Get Unicast Master Table\n");
    printf("\t9  - Set Unicast Master Table\n");
    printf("\ta  - Get Sync Interval\n");
    printf("\tb  - Set Sync Interval\n");
    printf("\tc  - Get Announce Interval\n");
    printf("\td  - Set Announce Interval\n");
    printf("\te  - Get default data set\n");
    printf("\tf  - Get current data set\n");
    printf("\tg  - Get parent data set\n");
    printf("\ti  - Get time property data set\n");
    printf("\tj  - Get port data set\n");
    printf("\tk  - Get UTC properties\n");
    printf("\tl  - Get traceability properties\n");
    printf("\tm  - Get timescale properties\n");
    printf("\tu  - up to main menu\n");
    printf("\th  - print this list\n");
}

/*
----------------------------------------------------------------------------
                                process_2_menu()

Description:
This function will process menu 2.

Inputs:
        None

Outputs:
   None

Return value:
        None

-----------------------------------------------------------------------------
*/
static int process_2_menu(void)
{
   INT8 ac_inputBuf[80];
   UINT8 b_data = 0;
   INT8  c_data;
   SC_t_MntTLV s_Tlv;
   UINT8 b_tlvData[k_SC_MAX_TLV_PLD_LEN];
   SC_t_Text s_userDesc;
   char c_usrBuf[k_MAX_USR_DES];
   t_PortAddr s_portAddr;

   s_userDesc.pc_text = c_usrBuf;

   /* connect data buffer to TLV structure */
   s_Tlv.pb_data = b_tlvData;

   printf("> ");

   while (1)
   {
      char c_cmd;
      fgets((void *)ac_inputBuf, sizeof(ac_inputBuf), stdin);
      sscanf((void *)ac_inputBuf, "%c", &c_cmd);
      if (!o_terminating)
      {
         int status;
         switch (c_cmd)
         {
         case '1':
            status = SC_PTP_ClkDesc(e_PARAM_GET, &s_Tlv);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
               print_tlv(&s_Tlv);
            }
            break;
        case '2':
            status = SC_PTP_UsrDesc(e_PARAM_GET, &s_Tlv, &s_userDesc);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
               print_tlv(&s_Tlv);
            }
            break;
        case '3':
            printf("Enter user description: ");
            fgets((void *)c_usrBuf, sizeof(ac_inputBuf), stdin);

            s_userDesc.b_len = strlen(c_usrBuf) - 1;
            if (s_userDesc.b_len == 0)
               break;

            printf("Sending %hhd bytes: %s\n", s_userDesc.b_len, c_usrBuf);

            status = SC_PTP_UsrDesc(e_PARAM_SET, &s_Tlv, &s_userDesc);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
               print_tlv(&s_Tlv);
            }
            break;
        case '4':
            status = SC_PTP_Domain(e_PARAM_GET, &s_Tlv, b_data);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
               print_tlv(&s_Tlv);
            }
            break;
        case '5':
            printf("Enter domain: ");
            fgets((void *)c_usrBuf, sizeof(c_usrBuf), stdin);
            sscanf((void *)c_usrBuf, "%hhd", &b_data);

            printf("Sending domain %hhd\n", b_data);

            status = SC_PTP_Domain(e_PARAM_SET, &s_Tlv, b_data);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
               print_tlv(&s_Tlv);
            }
            break;
        case '6':
            printf("Enable port\n");

            status = SC_PTP_EnaPort(e_PARAM_CMD, &s_Tlv);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
               print_tlv(&s_Tlv);
            }
            break;
         case '7':
            printf("Disable port\n");

            status = SC_PTP_DisPort(e_PARAM_CMD, &s_Tlv);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
               printf("Successful\n");
               print_tlv(&s_Tlv);
            }
            break;
         case '8':
            status = SC_PTP_UCMasterTbl(e_PARAM_GET, &s_Tlv, 0, 0, NULL);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
               print_tlv(&s_Tlv);
            }
            break;
         case '9':
            printf("Enter new unicast master address (XXX.XXX.XXX.XXX): ");
            fgets((void *)ac_inputBuf, sizeof(ac_inputBuf), stdin);
            if (sscanf((void *)ac_inputBuf, "%hhu.%hhu.%hhu.%hhu",
                     &s_portAddr.ab_addr[0],
                     &s_portAddr.ab_addr[1],
                     &s_portAddr.ab_addr[2],
                     &s_portAddr.ab_addr[3]) == 4)
            {
               printf("setting %hhu.%hhu.%hhu.%hhu\n"  ,
                   s_portAddr.ab_addr[0],
                   s_portAddr.ab_addr[1],
                   s_portAddr.ab_addr[2],
                   s_portAddr.ab_addr[3]);

               s_portAddr.e_netwProt = 1;  // e_UDP_IPv4
               s_portAddr.w_addrLen = 4;

               status = SC_PTP_UCMasterTbl(e_PARAM_SET, &s_Tlv, 1, 1, &s_portAddr);
               if (status == -1)
               {
                  printf("Error: Unsuccessful\n");
               }
               else if (status == -2)
               {
                  printf("Error: PTP not initialized\n");
               }
               else
               {
                  printf("Successful\n");
                  print_tlv(&s_Tlv);
               }
            }
            break;
        case 'a':
            status = SC_PTP_SyncIntv(e_PARAM_GET, &s_Tlv, 0);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
              printf("Successful\n");
              print_tlv(&s_Tlv);
            }
            break;
        case 'b':
            printf("Enter sync interval: ");
            fgets((void *)c_usrBuf, sizeof(c_usrBuf), stdin);
            sscanf((void *)c_usrBuf, "%hhd", &c_data);

            printf("Sending sync interval %hhd\n", c_data);
            status = SC_PTP_SyncIntv(e_PARAM_SET, &s_Tlv, c_data);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
               print_tlv(&s_Tlv);
            }
            break;
        case 'c':
            status = SC_PTP_AnncIntv(e_PARAM_GET, &s_Tlv, 0);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
              printf("Successful\n");
              print_tlv(&s_Tlv);
            }
            break;
        case 'd':
            printf("Enter announce interval: ");
            fgets((void *)c_usrBuf, sizeof(c_usrBuf), stdin);
            sscanf((void *)c_usrBuf, "%hhd", &c_data);

            printf("Sending announce interval %hhd\n", c_data);
            status = SC_PTP_AnncIntv(e_PARAM_SET, &s_Tlv, c_data);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
               print_tlv(&s_Tlv);
            }
            break;
        case 'e':
            status = SC_PTP_DefaultDs(e_PARAM_GET, &s_Tlv);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
              printf("Successful\n");
              print_tlv(&s_Tlv);
            }
            break;
        case 'f':
            status = SC_PTP_CurrentDs(e_PARAM_GET, &s_Tlv);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
              printf("Successful\n");
              print_tlv(&s_Tlv);
            }
            break;
        case 'g':
            status = SC_PTP_ParentDs(e_PARAM_GET, &s_Tlv);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
              printf("Successful\n");
              print_tlv(&s_Tlv);
            }
            break;
        case 'i':
            status = SC_PTP_TimePropDs(e_PARAM_GET, &s_Tlv);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
              printf("Successful\n");
              print_tlv(&s_Tlv);
            }
            break;
        case 'j':
            status = SC_PTP_PortDs(e_PARAM_GET, &s_Tlv);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
              printf("Successful\n");
              print_tlv(&s_Tlv);
            }
            break;
        case 'k':
            status = SC_PTP_UtcProp(e_PARAM_GET, &s_Tlv);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
              printf("Successful\n");
              print_tlv(&s_Tlv);
            }
            break;
        case 'l':
            status = SC_PTP_TraceProp(e_PARAM_GET, &s_Tlv);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
              printf("Successful\n");
              print_tlv(&s_Tlv);
            }
            break;
        case 'm':
            status = SC_PTP_TimeSclProp(e_PARAM_GET, &s_Tlv);
            if (status == -1)
            {
               printf("Error: Unsuccessful\n");
            }
            else if (status == -2)
            {
               printf("Error: PTP not initialized\n");
            }
            else
            {
              printf("Successful\n");
              print_tlv(&s_Tlv);
            }
            break;
         case 'u':
         case 'U':
            return 0;
            break;
         case 'h':
         case 'H':
         case '?':
            print_2_menu();
            break;
        default:
            break;
         } /*switch*/
      }
      else
      {
         break;
      }
      printf("> ");
   }

   return 0;
}

/*
----------------------------------------------------------------------------
                                print_3_menu()

Description:
This function will print menu 3.

Inputs:
        None

Outputs:
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
static void print_3_menu(void)
{
    printf("Select from following:\n");
    printf("\t1  - Set observation window\n");
    printf("\t2  - Set threshold\n");
    printf("\t3  - Set pacing factor\n");
    printf("\t4  - Clear IPDV values\n");
    printf("\ts  - Show IPDV configuration\n");
    printf("\tu  - up to main menu\n");
    printf("\th  - print this list\n");
}

/*
----------------------------------------------------------------------------
                                process_3_menu()

Description:
This function will process menu 3.


Inputs:
        None

Outputs:
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
static int process_3_menu(void)
{
   INT8 ac_inputBuf[80];
   t_ptpIpdvConfigType s_ptpIpdvConfig;
   char c_singleChar[] = ":";
   INT16 w_data;
   SC_t_MntTLV s_Tlv;
   UINT8 b_tlvData[k_SC_MAX_TLV_PLD_LEN];
   SC_t_Text s_userDesc;
   char c_usrBuf[k_MAX_USR_DES];

   s_userDesc.pc_text = c_usrBuf;

/* connect data buffer to TLV structure */
   s_Tlv.pb_data = b_tlvData;

   printf("> ");

   while (1)
   {
      char c_cmd;
      fgets((void *)ac_inputBuf, sizeof(ac_inputBuf), stdin);
      sscanf((void *)ac_inputBuf, "%c", &c_cmd);
      if (!o_terminating)
      {
         switch (c_cmd)
         {
         case '1':
            SC_ClkIpdvConfig(&s_ptpIpdvConfig, e_PARAM_GET);
            printf("Enter observation window (%hd): ", s_ptpIpdvConfig.w_ipdvObservIntv);
            fgets((void *)c_usrBuf, sizeof(c_usrBuf), stdin);
            sscanf((void *)c_usrBuf, "%hd", &w_data);

            printf("Sending observation window %hd\n", w_data);

            s_ptpIpdvConfig.w_ipdvObservIntv = w_data;
            if (SC_ClkIpdvConfig(&s_ptpIpdvConfig, e_PARAM_SET))
            {
               printf("Error: Unsuccessful\n");
            }
            else
            {
                printf("Successful\n");
            }
            break;
         case '2':
            SC_ClkIpdvConfig(&s_ptpIpdvConfig, e_PARAM_GET);
            printf("Enter threshold (%d ns): ", s_ptpIpdvConfig.l_ipdvThres);
            fgets((void *)c_usrBuf, sizeof(c_usrBuf), stdin);
            sscanf((void *)c_usrBuf, "%d", &s_ptpIpdvConfig.l_ipdvThres);

            printf("Sending threshold %d ns\n", s_ptpIpdvConfig.l_ipdvThres);

            /* convert to ns from us */
            if (SC_ClkIpdvConfig(&s_ptpIpdvConfig, e_PARAM_SET))
            {
               printf("Error: Unsuccessful\n");
            }
            else
            {
                printf("Successful\n");
            }
            break;
         case '3':
            SC_ClkIpdvConfig(&s_ptpIpdvConfig, e_PARAM_GET);
            printf("Enter pacing factor (%hd): ", s_ptpIpdvConfig.w_pacingFactor);
            fgets((void *)c_usrBuf, sizeof(c_usrBuf), stdin);
            sscanf((void *)c_usrBuf, "%hd", &w_data);

            printf("Sending pacing factor %hd\n", w_data);

            s_ptpIpdvConfig.w_pacingFactor = w_data;
            if (SC_ClkIpdvConfig(&s_ptpIpdvConfig, e_PARAM_SET))
            {
               printf("Error: Unsuccessful\n");
            }
            else
            {
                printf("Successful\n");
            }
            break;
         case '4':
            printf("Clearing IPDV\n");

            if (SC_ClkIpdvClrCtrs())
            {
               printf("Error: Unsuccessful\n");
            }
            else
            {
                printf("Successful\n");
            }
            break;
         case 's':
         case 'S':
            SC_ClkIpdvConfig(&s_ptpIpdvConfig, e_PARAM_GET);
            printf("%-44s %-2s %hd\n",
                   get_display_title_desc(e_PS_IPDV_OBS_INTERVAL),
                   c_singleChar, s_ptpIpdvConfig.w_ipdvObservIntv);
            printf("%-44s %-2s %ld\n",
                   get_display_title_desc(e_PS_IPDV_THRESHOLD),
                   c_singleChar, (long int)s_ptpIpdvConfig.l_ipdvThres);
            printf("%-44s %-2s %hd\n",
                   get_display_title_desc(e_PS_VAR_PACING_FACTOR),
                   c_singleChar, s_ptpIpdvConfig.w_pacingFactor);
            break;
         case 'u':
         case 'U':
            return 0;
            break;
         case 'h':
         case 'H':
         case '?':
            print_3_menu();
            break;
         default:
            break;
         } /*switch*/
      }
      else
      {
         break;
      }
      printf("> ");
   }

   return 0;
}

/*
----------------------------------------------------------------------------
                                get_display_title_desc()

Description:
This function will print out the text associated text for the given
performance id.

Inputs:
        UINT16 w_titleId - title id for desired text title.

Outputs:
        None

Return value:
        pointer to title text string.

-----------------------------------------------------------------------------
*/
static char *get_display_title_desc(UINT16 w_titleId)
{
   int i;

   for (i=0; i<sizeof(as_ptpPrivateStatusField)/sizeof(as_ptpPrivateStatusField[0]); i++)
   {
      if (w_titleId == as_ptpPrivateStatusField[i].b_state)
      {
         return (as_ptpPrivateStatusField[i].pc_textDesc);
      }
   }
   return (as_ptpPrivateStatusField[0].pc_textDesc);
}

/*
----------------------------------------------------------------------------
                                get_fll_state_desc()

Description:
This function will print out the text associated text for the fll state
enumeration.

Inputs:
        int fll_state - state enumeration for desired fll state.

Outputs:
        None

Return value:
        pointer to text string.

-----------------------------------------------------------------------------
*/
static char *get_fll_state_desc(int fll_state)
{
   int i;

   for (i=0; i<sizeof(as_ptpFllStateDesc)/sizeof(as_ptpFllStateDesc[0]); i++)
   {
      if (fll_state == as_ptpFllStateDesc[i].b_state)
      {
         return (as_ptpFllStateDesc[i].pc_textDesc);
      }
   }
   return (as_ptpFllStateDesc[0].pc_textDesc);
}

/*
----------------------------------------------------------------------------
    static char *get_RedundancyRoleNames()

Description:
This function will print out the text associated redundancy states enumeration
enumeration.

Inputs:
        int fll_state - state enumeration for desired fll state.

Outputs:
        None

Return value:
        pointer to text string.

-----------------------------------------------------------------------------
*/
static char *get_RedundancyRoleNames(int redState)
{
  int i;

  for (i=0; i<sizeof(as_RedundancyRoleName)/sizeof(as_RedundancyRoleName[0]); i++)
  {
    if (redState == as_RedundancyRoleName[i].b_state)
    {
      return (as_RedundancyRoleName[i].pc_textDesc);
    }
  }
  return (as_RedundancyRoleName[0].pc_textDesc);
}

/*
----------------------------------------------------------------------------
                                print_status_list()

Description:
This function will print the performance status.

Inputs:
        None

Outputs:
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
static void print_status_list(void)
{
   t_ptpPerformanceStatusType s_ptpPerformance;
   t_ptpIpdvType s_ptpIpdv;
   char *pc_str;
   char c_singleChar[] = ":";

   SC_GetPtpPerformance(&s_ptpPerformance);
   SC_ClkIpdvGet(&s_ptpIpdv);

   pc_str = get_fll_state_desc(s_ptpPerformance.e_fllState);

   printf("\n");
   printf("%-44s %-2s %s\n",get_display_title_desc(e_PS_FLLSTATE), c_singleChar, pc_str);
   printf("%-44s %-2s %d\n",get_display_title_desc(e_PS_FLLSTATEDURATION), c_singleChar,
   s_ptpPerformance.dw_fllStateDur/60);
   printf("%-44s %-2s %.2f\n",get_display_title_desc(e_PS_FORWARDWEIGHT), c_singleChar, s_ptpPerformance.f_fwdWeight);
   printf("%-44s %-2s %d\n",get_display_title_desc(e_PS_FORWARDTRANS900), c_singleChar, s_ptpPerformance.dw_fwdTransFree900);
   printf("%-44s %-2s %d\n",get_display_title_desc(e_PS_FORWARDTRANS3600), c_singleChar, s_ptpPerformance.dw_fwdTransFree3600);
   printf("%-44s %-2s %.1f\n",get_display_title_desc(e_PS_FORWARDTRANSUSED), c_singleChar, s_ptpPerformance.f_fwdPct);
   printf("%-44s %-2s %.1f\n",get_display_title_desc(e_PS_FORWARDOPMINTDEV), c_singleChar, s_ptpPerformance.f_fwdMinTdev);
   printf("%-44s %-2s %.2f\n",get_display_title_desc(e_PS_FORWARDOPMAFE), c_singleChar, s_ptpPerformance.f_fwdMafie);
   printf("%-44s %-2s %.1f\n",get_display_title_desc(e_PS_FORWARDMINCLUWIDTH), c_singleChar, s_ptpPerformance.f_fwdMinClstrWidth);
   printf("%-44s %-2s %.1f\n",get_display_title_desc(e_PS_FORWARDMODEWIDTH), c_singleChar, s_ptpPerformance.f_fwdModeWidth);

   printf("\n");
   printf("%-44s %-2s %.2f\n",get_display_title_desc(e_PS_REVERSEDWEIGHT), c_singleChar, s_ptpPerformance.f_revWeight);
   printf("%-44s %-2s %d\n",get_display_title_desc(e_PS_REVERSEDTRANS900), c_singleChar, s_ptpPerformance.dw_revTransFree900);
   printf("%-44s %-2s %d\n",get_display_title_desc(e_PS_REVERSEDTRANS3600), c_singleChar, s_ptpPerformance.dw_revTransFree3600);
   printf("%-44s %-2s %.1f\n",get_display_title_desc(e_PS_REVERSEDTRANSUSED), c_singleChar, s_ptpPerformance.f_revPct);
   printf("%-44s %-2s %.1f\n",get_display_title_desc(e_PS_REVERSEDOPMINTDEV), c_singleChar, s_ptpPerformance.f_revMinTdev);
   printf("%-44s %-2s %.2f\n",get_display_title_desc(e_PS_REVERSEDOPMAFE), c_singleChar, s_ptpPerformance.f_revMafie);
   printf("%-44s %-2s %.1f\n",get_display_title_desc(e_PS_REVERSEDMINCLUWIDTH), c_singleChar, s_ptpPerformance.f_revMinClstrWidth);
   printf("%-44s %-2s %.1f\n",get_display_title_desc(e_PS_REVERSEDMODEWIDTH), c_singleChar, s_ptpPerformance.f_revModeWidth);

   printf("\n");
   printf("%-44s %-2s %.2f\n",get_display_title_desc(e_PS_FREQCORRECT), c_singleChar, s_ptpPerformance.f_freqCorrection);
   printf("%-44s %-2s %.2f\n",get_display_title_desc(e_PS_PHASECORRECT), c_singleChar, s_ptpPerformance.f_phaseCorrection);
   printf("%-44s %-2s %.2f\n",get_display_title_desc(e_PS_OUTPUTTDEVESTIMATE), c_singleChar, s_ptpPerformance.f_tdevEstimate);
   printf("%-44s %-2s %.2f\n",get_display_title_desc(e_PS_OUTPUTMDEVESTIMATE), c_singleChar, s_ptpPerformance.f_mdevEstimate);
   printf("%-44s %-2s %.2f\n",get_display_title_desc(e_PS_RESIDUALPHASEERROR), c_singleChar, s_ptpPerformance.f_residualPhaseErr);
   printf("%-44s %-2s %.2f\n",get_display_title_desc(e_PS_MINTIMEDISPERSION), c_singleChar, s_ptpPerformance.f_minRoundTripDly);
   printf("%-44s %-2s %hd\n",get_display_title_desc(e_PS_FORWARDGMSYNCPSEC), c_singleChar, s_ptpPerformance.w_fwdPktRate);
   printf("%-44s %-2s %hd\n",get_display_title_desc(e_PS_REVERSEDGMSYNCPSEC), c_singleChar, s_ptpPerformance.w_revPktRate);

   printf("\n");
   printf("%-44s %-2s %.1f\n",get_display_title_desc(e_PS_FWD_IPDV_PCT_BELOW), c_singleChar, (double)s_ptpIpdv.f_ipdvFwdPct);
   printf("%-44s %-2s %.3f\n",get_display_title_desc(e_PS_FWD_IPDV_MAX), c_singleChar, (double)s_ptpIpdv.f_ipdvFwdMax);
   printf("%-44s %-2s %.3f\n",get_display_title_desc(e_PS_FWD_INTERPKT_VAR), c_singleChar, (double)s_ptpIpdv.f_ipdvFwdJitter);
   printf("%-44s %-2s %.1f\n",get_display_title_desc(e_PS_REV_IPDV_PCT_BELOW), c_singleChar, (double)s_ptpIpdv.f_ipdvRevPct);
   printf("%-44s %-2s %.3f\n",get_display_title_desc(e_PS_REV_IPDV_MAX), c_singleChar, (double)s_ptpIpdv.f_ipdvRevMax);
   printf("%-44s %-2s %.3f\n",get_display_title_desc(e_PS_REV_INTERPKT_VAR), c_singleChar, (double)s_ptpIpdv.f_ipdvRevJitter);
   printf("\n");

   return;
}

/*
----------------------------------------------------------------------------
                                print_tlv()

Description:
This function will print the tlv data in hex and ASCII.

Inputs:
        SC_t_MntTLV *ps_Tlv
        Pointer to TLV structure to print.

Outputs:
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
static void print_tlv(SC_t_MntTLV *ps_Tlv)
{
   int i, j;
   char ac_buf[80];
   char ac_null_buf[1];

   printf("Type: %hd Length: %hd MntID: 0x%04hX\n", ps_Tlv->w_type, ps_Tlv->w_len, ps_Tlv->w_mntId);


   if (ps_Tlv->w_len)
   {
      j=0;
      for (i=0; i<ps_Tlv->w_len - 2; i++)
      {
         printf("%02hhx " , ps_Tlv->pb_data[i]);
         if (isprint(ps_Tlv->pb_data[i]))
         {
            sprintf(ac_buf + j++, "%c", ps_Tlv->pb_data[i]);
         }
         else
         {
            sprintf(ac_buf + j++, ".");
         }
         if (i%16 == 15)
         {
            j = 0;
            printf("  %s\n", ac_buf);
            ac_buf[0] = 0;
         }
      }

      ac_null_buf[0] = 0;
      printf("%*s  %s\n",(16-j) * 3, ac_null_buf, ac_buf);
   }

   printf("\n");
}

/*
----------------------------------------------------------------------------
                                print_a_menu()

Description:
This function prints menu a.

Inputs:
        None

Outputs:
   None

Return value:
        None

-----------------------------------------------------------------------------
*/
static void print_a_menu(void)
{
    printf("Select from following:\n");
    printf("\t0  - clear mask\n");
        printf("\t1  - set local debug mask\n");
    printf("\t2  - get local debug mask\n");

    printf("\tu  - up to main menu\n");
    printf("\th  - print this list\n");
}

/*
----------------------------------------------------------------------------
                                process_a_menu()

Description:
This function will process menu a.

Inputs:
        None

Outputs:
   None

Return value:
        None

-----------------------------------------------------------------------------
*/
static int process_a_menu(void)
{
   INT8 ac_inputBuf[80];

   printf("> ");

   while (1)
   {
      char c_cmd;
      fgets((void *)ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
      sscanf((void *)ac_inputBuf, "%c", &c_cmd);
      if (!o_terminating)
      {
         switch (c_cmd)
         {
         case '0':
                s_localDebCfg.deb_mask = 0;
                printf("clear local debug mask\n");
            break;

         case '1':
            printf("Freq Correction Mask: 0x%04X\n", LOCAL_DEB_FC_TRACE_MASK);
            printf("RX Timestamp Mask:    0x%04X\n", LOCAL_DEB_RXTS_TRACE_MASK);
            printf("TX Timestamp Mask:    0x%04X\n", LOCAL_DEB_TXTS_TRACE_MASK);
            printf("Phase Offset Mask:    0x%04X\n", LOCAL_DEB_PH_TRACE_MASK);
            printf("Enter local debug mask (hex): ");
            fgets((void *)ac_inputBuf, sizeof(ac_inputBuf), stdin);

            if (sscanf((void *)ac_inputBuf, "%x", &s_localDebCfg.deb_mask) == 1)
            {
               printf("local debug mask set to 0x%04X\n", s_localDebCfg.deb_mask);
            }
            break;

         case '2':
                        printf("local debug mask: 0x%04X\n", s_localDebCfg.deb_mask);
            break;

         case 'u':
         case 'U':
            return 0;
            break;
         case 'h':
         case 'H':
         case '?':
            print_a_menu();
            break;
        default:
            break;
         } /*switch*/
      }
      else
      {
         break;
      }
      printf("> ");
   }

   return 0;
}



/*
----------------------------------------------------------------------------
                                print_b_menu()

Description:
This function prints menu b.

Inputs:
        None

Outputs:
   None

Return value:
        None

-----------------------------------------------------------------------------
*/
static void print_b_menu(void)
{
    printf("Select from following:\n");
    printf("\t0  - normal\n");
        printf("\t1  - enable log print on console\n");
        printf("\t2  - disable log print on console\n");

    printf("\t3  - enable one second trace\n");
    printf("\t4  - disable one second trace\n");

    printf("\t5  - enable force zero frequency correction\n");
    printf("\t6  - disable force zero frequency correction\n");

    printf("\tu  - up to main menu\n");
    printf("\th  - print this list\n");
}

/*
----------------------------------------------------------------------------
                                process_b_menu()

Description:
This function will process menu a.

Inputs:
        None

Outputs:
   None

Return value:
        None

-----------------------------------------------------------------------------
*/
static int process_b_menu(void)
{
   INT8 ac_inputBuf[80];

   printf("> ");

   while (1)
   {
      char c_cmd;
      fgets((void *)ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
      sscanf((void *)ac_inputBuf, "%c", &c_cmd);
      if (!o_terminating)
      {
         switch (c_cmd)
         {
         case '0':
                s_localDebCfg.log_disp_on_console = 0;
                s_localDebCfg.one_sec_trace = 0;
                s_localDebCfg.force_fc_zero = 0;
                printf("normal\n");
                break;

         case '1':
                s_localDebCfg.log_disp_on_console = 1;
                printf("enable log display on console\n");
            break;
         case '2':
                s_localDebCfg.log_disp_on_console = 0;
                printf("disable log display on console\n");
            break;

         case '3':
                s_localDebCfg.one_sec_trace = 1;
                printf("enable local one second rx/tm trace\n");
            break;
         case '4':
                s_localDebCfg.one_sec_trace = 0;
                printf("disable local one second rx/tm trace\n");
            break;

         case '5':
                s_localDebCfg.force_fc_zero = 1;
                printf("enable force zero frequency correction\n");
            break;
         case '6':
                s_localDebCfg.force_fc_zero = 0;
                printf("disable force zero frequency correction\n");
            break;

         case 'u':
         case 'U':
            return 0;
            break;
         case 'h':
         case 'H':
         case '?':
            print_b_menu();
            break;
        default:
            break;
         } /*switch*/
      }
      else
      {
         break;
      }
      printf("> ");
   }

   return 0;
}

/*--------------------------------------------------------------------------
    void chanConfigurationMenu(void)

Description:
Allows to read and change the input channels configuration
Inputs:
        None

Outputs:
        None

Return value:
        None

--------------------------------------------------------------------------*/

void chanConfigurationMenu(void)
{
  const char *kOK_string = "OK";
  const char *kFault_string = "FLT";
  const char *kDisabled_string = "DIS";
  char menuOption = 0;
  BOOLEAN printMenu = TRUE; //print menu the first time
  char ac_inputBuf[80];
  char strBuffer[256];
  char choice;
  int retValue;
  int readInt;
  int i;
  int currentChannel = -1;

  //variables to hold the channel configuration
  SC_t_ChanConfig chanConfig[SC_TOTAL_CHANNELS];
  memset(&chanConfig, 0, sizeof(chanConfig));
  SC_t_selModeEnum chanSelectionMode;
  SC_t_swtModeEnum chanSwitchMode;
  UINT8 numChannels = 0;

  //variables to hold the channel status
  SC_t_chanStatus chanStatus[SC_TOTAL_CHANNELS];
  memset(&chanStatus, 0, sizeof(chanStatus));

  //Variables to hold the SSM values and conversions
  UINT8 currentQL[2]; //for frequency and time
  UINT8 currentSSM;
  BOOLEAN isSSMStandard;

  //Variables to hold the servo configuration
  t_servoConfigType s_servoC;

#if (SC_RED_CHANNELS > 0)
  //Variables for redundancy status display
  SC_t_activeStandbyEnum localRedStatus;
  BOOLEAN localRedEnabled;
#endif

  //get the number of channels
  retValue = SC_GetChanConfig(NULL, NULL, NULL, &numChannels);
  if(retValue != 0)
  {
    printf("SC_GetchanConfig failed with error code: %d\n", retValue);
    return;
  }

  while((menuOption != 'u') && (!o_terminating)) //go UP, return to main
  {
    if(printMenu)
    {
      printMenu = FALSE;
      printf("Channel Configuration Menu.\n");
      printf("Select from the following options:\n");
      printf("\t0  - Get Channel Configuration (SC_GetchanConfig)\n");
      printf("\n");
      printf("\t1  - Get and Optionally set SC_ChanEnable Value\n");
      printf("\t2  - Get and Optionally set SC_ChanPriority Value\n");
      printf("\t3  - Get and Optionally set SC_ChanEnableAssumeQL value\n");
      printf("\t4  - Get and Optionally set SC_ChanAssumeQL Value\n");
      printf("\t5  - Get and Optionally set SC_ChanSwtMode Value\n");
      printf("\t6  - Get and Optionally set SC_ChanSelMode Value\n");
      printf("\t7  - Select a Channel (SC_SetSelChan)\n");
      printf("\n");
      printf("\te  - ESMC/SSM Channel Attributes, Diagnostics, and Test Menu\n");
      printf("\n");
      printf("\ts  - Get Channel Status and Configuration. (SC_GetChanStatus and SC_GetchanConfig)\n");
      printf("\n");
      printf("\tt  - Print ESMC/SSM QL Translation Table\n");
      printf("\n");
      printf("\tu  - Up to Previous Menu\n");
      printf("\th  - Print This List\n");
    }

    //read
    printf("? for options ->\n");
    fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
    sscanf(ac_inputBuf, "%c", &menuOption);

    //dispatch
    switch(menuOption)
    {
      case 'S': case 's'://SC_GetChanStatus
        memset(&chanStatus, 0, sizeof(chanStatus));
        retValue = SC_GetChanStatus(numChannels, chanStatus);
        if(retValue != 0)
          printf("SC_GetChanStatus failed with error code: %d\n", retValue);
        else
        {
          printf("SC_GetChanStatus:\n");
          printf("Number of configured channels: %d\n", numChannels);
          printf("+--------------------------------------------------------------+\n");
          printf("|# |  Frequency  |    Time     |  QL   |         Fault         |\n");
          printf("|  | ST | Weight | ST | Weight | F | T | Perf. | Valid |  LOS  |\n");
          printf("+--------------------------------------------------------------+\n");
          for(i=0; i<numChannels; i++)
          {
            printf("|%2d", i);
            switch(chanStatus[i].e_chanFreqStatus)
            {
              case e_SC_CHAN_STATUS_OK: strcpy(strBuffer, kOK_string); break;
              case e_SC_CHAN_STATUS_FLT: strcpy(strBuffer, kFault_string); break;
              case e_SC_CHAN_STATUS_DIS: strcpy(strBuffer, kDisabled_string); break;
              default: sprintf(strBuffer, "%d", chanStatus[i].e_chanFreqStatus); break;
            }
            printf("|%-3s |  %3d   ", strBuffer, chanStatus[i].b_freqWeight);

            switch(chanStatus[i].e_chanTimeStatus)
            {
              case e_SC_CHAN_STATUS_OK: strcpy(strBuffer, kOK_string); break;
              case e_SC_CHAN_STATUS_FLT: strcpy(strBuffer, kFault_string); break;
              case e_SC_CHAN_STATUS_DIS: strcpy(strBuffer, kDisabled_string); break;
              default: sprintf(strBuffer, "%d", chanStatus[i].e_chanTimeStatus); break;
            }
            printf("|%-3s |  %3d   ", strBuffer, chanStatus[i].b_timeWeight);

            printf("|%2d%1c", chanStatus[i].b_freqQL, (chanStatus[i].b_qlReadExternally)?'*':' ');
            printf("|%2d%1c", chanStatus[i].b_timeQL, (chanStatus[i].b_qlReadExternally)?'*':' ');

            printf("|  %-3s  |  %-3s  |  %-3s  ",
              (chanStatus[i].l_faultMap & 0x01)?kFault_string:kOK_string,
              (chanStatus[i].l_faultMap & 0x02)?kFault_string:kOK_string,
              (chanStatus[i].l_faultMap & 0x04)?kFault_string:kOK_string);

            printf("|\n");
          }
          printf("+--------------------------------------------------------------+\n");

          printf("\n");

          retValue = SC_GetServoConfig(&s_servoC);
          if(retValue == 0)
            printf("Local Oscillator QL: %"PRIu8"\n", s_servoC.b_loQl);
          else
            printf("SC_GetServoConfig failed with error code: %d\n", retValue);

#if (SC_RED_CHANNELS > 0)
          if(SC_GetRedundantStatus(&localRedEnabled, &localRedStatus) == 0)
          {
            printf("Redundancy role: %s\n", get_RedundancyRoleNames(localRedStatus));
          }
          else
          {
            printf("Error: Call to SC_GetRedundantStatus() failed.\n");
          }
#endif
          printf("Output quality level: (x) marks a non standard value.\n");
          printf("+---------------------------------------------------+\n");
          printf("|           |    QL |   PTP | SyncE |    E1 |    T1 |\n");
          printf("+---------------------------------------------------+\n");

          currentQL[0] = SC_OutQualityLevel(TRUE); //frequency
          currentQL[1] = SC_OutQualityLevel(FALSE); //time
          for(i = 0; i < 2; i++)
          {
            printf("| %-10s", (i == 0)?"Frequency":"Time");
            printf("| %6d", currentQL[i]);

            retValue = SC_QlToSSM(currentQL[i], e_SC_CHAN_TYPE_PTP, &isSSMStandard);
            if(retValue >= 0)
            {
              currentSSM = (UINT8)retValue;
              if(isSSMStandard)
                snprintf(strBuffer, sizeof(strBuffer), "%d", currentSSM);
              else
                snprintf(strBuffer, sizeof(strBuffer), "(%d)", currentSSM);
            }
            else
              strBuffer[0] = 0;
            printf("| %6s", strBuffer);

            retValue = SC_QlToSSM(currentQL[i], e_SC_CHAN_TYPE_SE, &isSSMStandard);
            if(retValue >= 0)
            {
              currentSSM = (UINT8)retValue;
              if(isSSMStandard)
                snprintf(strBuffer, sizeof(strBuffer), "%d", currentSSM);
              else
                snprintf(strBuffer, sizeof(strBuffer), "(%d)", currentSSM);
            }
            else
              strBuffer[0] = 0;
            printf("| %6s", strBuffer);

            retValue = SC_QlToSSM(currentQL[i], e_SC_CHAN_TYPE_E1, &isSSMStandard);
            if(retValue >= 0)
            {
              currentSSM = (UINT8)retValue;
              if(isSSMStandard)
                snprintf(strBuffer, sizeof(strBuffer), "%d", currentSSM);
              else
                snprintf(strBuffer, sizeof(strBuffer), "(%d)", currentSSM);
            }
            else
              strBuffer[0] = 0;
            printf("| %6s", strBuffer);

            retValue = SC_QlToSSM(currentQL[i], e_SC_CHAN_TYPE_T1, &isSSMStandard);
            if(retValue >= 0)
            {
              currentSSM = (UINT8)retValue;
              if(isSSMStandard)
                snprintf(strBuffer, sizeof(strBuffer), "%d", currentSSM);
              else
                snprintf(strBuffer, sizeof(strBuffer), "(%d)", currentSSM);
            }
            else
              strBuffer[0] = 0;
            printf("| %6s", strBuffer);

            printf("|\n");
          }
          printf("+---------------------------------------------------+\n");
        }
        printf("\n");
      //break; //channel status also includes channel configuration

      case '0': //SC_GetChanConfig
        memset(&chanConfig, 0, sizeof(chanConfig));
        retValue = SC_GetChanConfig(chanConfig, &chanSelectionMode, &chanSwitchMode, &numChannels);
        if(retValue != 0)
          printf("SC_GetchanConfig failed with error code: %d\n", retValue);
        else
        {
          printf("SC_GetChanConfig:\n");
          printf("Number of configured channels: %d\n", numChannels);
          printf("Channel selection mode: ");
          switch(chanSelectionMode)
          {
            case e_SC_CHAN_SEL_PRIO:
              printf("e_SC_CHAN_SEL_PRIO\n");
            break;

            case e_SC_CHAN_SEL_QL:
               printf("e_SC_CHAN_SEL_QL\n");
            break;

            default:
              printf("Unexpected value (%d)\n", chanSelectionMode);
            break;
          }
          printf("Channel Switch mode: ");
          switch(chanSwitchMode)
          {
            case e_SC_SWTMODE_AR:
              printf("e_SC_SWTMODE_AR\n");
            break;

            case e_SC_SWTMODE_AS:
               printf("e_SC_SWTMODE_AS\n");
            break;

            case e_SC_SWTMODE_OFF:
               printf("e_SC_SWTMODE_OFF\n");
            break;

            default:
              printf("Unexpected value (%d)\n", chanSwitchMode);
            break;
          }
          printf("+---------------------------------------------------------+\n");
          printf("|# |Type    | FP | F | TP | T | AQL | FAQL | TAQL | RATE  |\n");
          printf("+---------------------------------------------------------+\n");
          for(i=0; i<numChannels; i++)
          {
            retValue=chanTypeToString(strBuffer, sizeof(strBuffer), chanConfig[i].e_ChanType);
            if(retValue != 0)
              snprintf(strBuffer, sizeof(strBuffer), "%d", chanConfig[i].e_ChanType);
            printf("|%2d|%-8s", i, strBuffer);

            printf("| %2d | %c | %2d | %c ", chanConfig[i].b_ChanFreqPrio, trueFalseIntToChar(chanConfig[i].o_ChanFreqEnabled),
                chanConfig[i].b_ChanTimePrio, trueFalseIntToChar(chanConfig[i].o_ChanTimeEnabled));

            printf("|  %c  |  %2d  |  %2d  ", trueFalseIntToChar(chanConfig[i].o_ChanAssumedQLenabled), chanConfig[i].b_ChanFreqAssumedQL,
                chanConfig[i].b_ChanTimeAssumedQL);

            printf("| %6d", chanConfig[i].l_MeasRate);

            printf("|\n");
          }
          printf("+---------------------------------------------------------+\n");
        }
      break;

      case 't': case 'T':
        printf("QL to ESMC/SSM translation table: (x) marks a non standard value.\n");
        printf("+---------------------------------------+\n");
        printf("|    QL |   PTP | SyncE |    E1 |    T1 |\n");
        printf("+---------------------------------------+\n");
        for(i = 1; i < 17; i++)
        {
          printf("| %6d", i);

          retValue = SC_QlToSSM(i, e_SC_CHAN_TYPE_PTP, &isSSMStandard);
          if(retValue >= 0)
          {
            currentSSM = (UINT8)retValue;
            if(isSSMStandard)
              snprintf(strBuffer, sizeof(strBuffer), "%d", currentSSM);
            else
              snprintf(strBuffer, sizeof(strBuffer), "(%d)", currentSSM);
          }
          else
            strBuffer[0] = 0;
          printf("| %6s", strBuffer);

          retValue = SC_QlToSSM(i, e_SC_CHAN_TYPE_SE, &isSSMStandard);
          if(retValue >= 0)
          {
            currentSSM = (UINT8)retValue;
            if(isSSMStandard)
              snprintf(strBuffer, sizeof(strBuffer), "%d", currentSSM);
            else
              snprintf(strBuffer, sizeof(strBuffer), "(%d)", currentSSM);
          }
          else
            strBuffer[0] = 0;
          printf("| %6s", strBuffer);

          retValue = SC_QlToSSM(i, e_SC_CHAN_TYPE_E1, &isSSMStandard);
          if(retValue >= 0)
          {
            currentSSM = (UINT8)retValue;
            if(isSSMStandard)
              snprintf(strBuffer, sizeof(strBuffer), "%d", currentSSM);
            else
              snprintf(strBuffer, sizeof(strBuffer), "(%d)", currentSSM);
          }
          else
            strBuffer[0] = 0;
          printf("| %6s", strBuffer);

          retValue = SC_QlToSSM(i, e_SC_CHAN_TYPE_T1, &isSSMStandard);
          if(retValue >= 0)
          {
            currentSSM = (UINT8)retValue;
            if(isSSMStandard)
              snprintf(strBuffer, sizeof(strBuffer), "%d", currentSSM);
            else
              snprintf(strBuffer, sizeof(strBuffer), "(%d)", currentSSM);
          }
          else
            strBuffer[0] = 0;
          printf("| %6s", strBuffer);

          printf("|\n");
        }
        printf("+---------------------------------------+\n");
      break;

      case '1': //SC_ChanEnable
        currentChannel = getChannelNumber(numChannels);
        if(currentChannel != -1)
        {
          BOOLEAN chanIsEnabled;
          retValue=SC_ChanEnable(e_PARAM_GET, currentChannel, TRUE, &chanIsEnabled);
          if(retValue == 0)
            printf("Channel %d is enabled for frequency: %c\n", currentChannel, trueFalseIntToChar(chanIsEnabled));
          else
            printf("SC_ChanEnable e_PARAM_GET failed with error code: %d\n", retValue);

          retValue=SC_ChanEnable(e_PARAM_GET, currentChannel, FALSE, &chanIsEnabled);
          if(retValue == 0)
            printf("Channel %d is enabled for time: %c\n", currentChannel, trueFalseIntToChar(chanIsEnabled));
          else
            printf("SC_ChanEnable e_PARAM_GET failed with error code: %d\n", retValue);

          if(askWantToChange('N'))
          {
            printf("Enable channel %d for frequency? (Y/N, Enter=NC)\n", currentChannel);
            fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
            if(sscanf(ac_inputBuf, "%c", &choice) == 1)
            {
              if (!Is_EOL(choice))
              {
                chanIsEnabled=trueFalseCharToInt(choice);
                retValue=SC_ChanEnable(e_PARAM_SET, currentChannel, TRUE, &chanIsEnabled);
                if(retValue != 0)
                  printf("SC_ChanEnable e_PARAM_SET failed with error code: %d\n", retValue);
              }
              else
              {
                printf("No change\n");
              }
            }

            printf("Enable channel %d for time? (Y/N, Enter=NC)\n", currentChannel);
            fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
            if(sscanf(ac_inputBuf, "%c", &choice) == 1)
            {
              if (!Is_EOL(choice))
              {
                chanIsEnabled=trueFalseCharToInt(choice);
                retValue=SC_ChanEnable(e_PARAM_SET, currentChannel, FALSE, &chanIsEnabled);
                if(retValue != 0)
                  printf("SC_ChanEnable e_PARAM_SET failed with error code: %d\n", retValue);
              }
              else
              {
                printf("No change\n");
              }
            }
          }
        }
      break;

      case '2': //SC_ChanPriority
        currentChannel = getChannelNumber(numChannels);
        if(currentChannel != -1)
        {
          UINT8 chanPriority;
          retValue=SC_ChanPriority(e_PARAM_GET, currentChannel, TRUE, &chanPriority);
          if(retValue == 0)
            printf("Channel %d priority for frequency: %d\n", currentChannel, chanPriority);
          else
            printf("SC_ChanPriority e_PARAM_GET failed with error code: %d\n", retValue);

          retValue=SC_ChanPriority(e_PARAM_GET, currentChannel, FALSE, &chanPriority);
           if(retValue == 0)
             printf("Channel %d priority for time: %d\n", currentChannel, chanPriority);
           else
             printf("SC_ChanPriority e_PARAM_GET failed with error code: %d\n", retValue);

          if(askWantToChange('N'))
          {
            printf("Channel %d priority for frequency? (Enter=NC)\n", currentChannel);
            fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
            if(sscanf(ac_inputBuf, "%d", &readInt) == 1)
            {
              chanPriority=(UINT8)readInt;
              retValue=SC_ChanPriority(e_PARAM_SET, currentChannel, TRUE, &chanPriority);
              if(retValue != 0)
                printf("SC_ChanPriority e_PARAM_SET failed with error code: %d\n", retValue);
            }
            else
            {
              printf("No change\n");
            }
            printf("Channel %d priority for time? (Enter=NC)\n", currentChannel);
            fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
            if(sscanf(ac_inputBuf, "%d", &readInt) == 1)
            {
              chanPriority=(UINT8)readInt;
              retValue=SC_ChanPriority(e_PARAM_SET, currentChannel, FALSE, &chanPriority);
              if(retValue != 0)
                printf("SC_ChanPriority e_PARAM_SET failed with error code: %d\n", retValue);
            }
            else
            {
              printf("No change\n");
            }
          }
        }
      break;

      case '3': //SC_ChanEnableAssumeQL
        currentChannel = getChannelNumber(numChannels);
        if(currentChannel != -1)
        {
          BOOLEAN chanEnaAssumedQL;
          retValue=SC_ChanEnableAssumeQL(e_PARAM_GET, currentChannel, &chanEnaAssumedQL);
          if(retValue == 0)
            printf("Channel %d Enable Assumed Quality Level: %c\n", currentChannel, trueFalseIntToChar(chanEnaAssumedQL));
          else
            printf("SC_ChanEnableAssumeQL e_PARAM_GET failed with error code: %d\n", retValue);

          if(askWantToChange('N'))
          {
            printf("Enable channel %d Assumed Quality Level? (Y/N, Enter=NC)\n", currentChannel);
            fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
            if(sscanf(ac_inputBuf, "%c", &choice) == 1)
            {
              if (!Is_EOL(choice))
              {
                chanEnaAssumedQL=trueFalseCharToInt(choice);
                retValue=SC_ChanEnableAssumeQL(e_PARAM_SET, currentChannel, &chanEnaAssumedQL);
                if(retValue != 0)
                  printf("SC_ChanPriority e_PARAM_SET failed with error code: %d\n", retValue);
              }
              else
              {
                printf("No change\n");
              }
            }
          }
        }
      break;

      case '4': //SC_ChanAssumeQL
        currentChannel = getChannelNumber(numChannels);
        if(currentChannel != -1)
        {
          UINT8 chanAssumeQL;
          retValue=SC_ChanAssumeQL(e_PARAM_GET, currentChannel, TRUE, &chanAssumeQL);
          if(retValue == 0)
            printf("Channel %d assumed quality level for frequency: %d\n", currentChannel, chanAssumeQL);
          else
            printf("SC_ChanAssumeQL e_PARAM_GET failed with error code: %d\n", retValue);

          retValue=SC_ChanAssumeQL(e_PARAM_GET, currentChannel, FALSE, &chanAssumeQL);
          if(retValue == 0)
            printf("Channel %d assumed quality level for time: %d\n", currentChannel, chanAssumeQL);
          else
            printf("SC_ChanAssumeQL e_PARAM_GET failed with error code: %d\n", retValue);

          if(askWantToChange('N'))
          {
            printf("Channel %d assumed quality level for frequency? (Enter=NC)\n", currentChannel);
            fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
            if(sscanf(ac_inputBuf, "%d", &readInt) == 1)
            {
              chanAssumeQL=(UINT8)readInt;
              retValue=SC_ChanAssumeQL(e_PARAM_SET, currentChannel, TRUE, &chanAssumeQL);
              if(retValue != 0)
                printf("SC_ChanAssumeQL e_PARAM_SET failed with error code: %d\n", retValue);
            }
            else
            {
              printf("No change\n");
            }
            printf("Channel %d assumed quality level for time? (Enter=NC)\n", currentChannel);
            fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
            if(sscanf(ac_inputBuf, "%d", &readInt) == 1)
            {
              chanAssumeQL=(UINT8)readInt;
              retValue=SC_ChanAssumeQL(e_PARAM_SET, currentChannel, FALSE, &chanAssumeQL);
              if(retValue != 0)
                printf("SC_ChanAssumeQL e_PARAM_SET failed with error code: %d\n", retValue);
            }
            else
            {
              printf("No change\n");
            }
          }
        }
      break;

      case '5': //SC_ChanSwtMode
        retValue=SC_ChanSwtMode(e_PARAM_GET, &chanSwitchMode);
        if(retValue == 0)
        {
          printf("Channel Switch Mode: ");
          switch(chanSwitchMode)
          {
            case e_SC_SWTMODE_AR:
              printf("e_SC_SWTMODE_AR\n");
            break;

            case e_SC_SWTMODE_AS:
              printf("e_SC_SWTMODE_AS\n");
            break;

            case e_SC_SWTMODE_OFF:
              printf("e_SC_SWTMODE_OFF\n");
            break;

            default:
              printf("Unexpected value (%d)\n", chanSwitchMode);
            break;
          }
        }
        else
          printf("SC_ChanSwtMode e_PARAM_GET failed with error code: %d\n", retValue);

        if(askWantToChange('N'))
        {
          printf("Channel Switch Mode? (AR/AS/OFF, Enter=NC)\n");
          fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
          if(sscanf(ac_inputBuf, "%*c%c", &choice) == 1)
          {
            switch(choice)
            {
              case 'R': case 'r':
                chanSwitchMode = e_SC_SWTMODE_AR;
              break;

              case 'S': case 's':
                chanSwitchMode = e_SC_SWTMODE_AS;
              break;

              case 'F': case 'f':
                chanSwitchMode = e_SC_SWTMODE_OFF;
              break;

              default:
                printf("Invalid option\n");
              break;
            }
            retValue=SC_ChanSwtMode(e_PARAM_SET, &chanSwitchMode);
            if(retValue != 0)
              printf("SC_ChanSwtMode e_PARAM_SET failed with error code: %d\n", retValue);
          }
          else
          {
            printf("No change\n");
          }
        }
      break;

      case '6': //SC_ChanSelMode
        retValue=SC_ChanSelMode(e_PARAM_GET, &chanSelectionMode);
        if(retValue == 0)
        {
          printf("Channel Selection Mode:");
          switch(chanSelectionMode)
          {
            case e_SC_CHAN_SEL_PRIO:
              printf("e_SC_CHAN_SEL_PRIO\n");
            break;

            case e_SC_CHAN_SEL_QL:
              printf("e_SC_CHAN_SEL_QL\n");
            break;

            default:
              printf("Unexpected value (%d)\n", chanSelectionMode);
            break;
          }
        }
        else
          printf("SC_ChanSelMode e_PARAM_GET failed with error code: %d\n", retValue);

        if(askWantToChange('N'))
        {
          printf("Channel Selection Mode? (P/Q, Enter=NC)\n");
          fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
          if(sscanf(ac_inputBuf, "%c", &choice) == 1)
          {
            if (!Is_EOL(choice))
            {
              switch(choice)
              {
                case 'P': case 'p':
                  chanSelectionMode = e_SC_CHAN_SEL_PRIO;
                break;

                case 'Q': case 'q':
                  chanSelectionMode = e_SC_CHAN_SEL_QL;
                break;

                default:
                  printf("Invalid option\n");
                break;
              }
              retValue=SC_ChanSelMode(e_PARAM_SET, &chanSelectionMode);
              if(retValue != 0)
                printf("SC_ChanSelMode e_PARAM_SET failed with error code: %d\n", retValue);
            }
            else
            {
              printf("No change\n");
            }
          }
        }
      break;

      case '7': //SC_SetSelChan
        printf("Selected channel for frequency? (number, Enter=NC)\n");
        fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
        if(sscanf(ac_inputBuf, "%d", &readInt) == 1)
        {
          retValue=SC_SetSelChan(TRUE, (UINT8)readInt);
          if(retValue != 0)
            printf("SC_SetSelChan failed with error code: %d\n", retValue);
        }
        else
        {
          printf("No change\n");
        }
        printf("Selected channel for time? (number, Enter=NC)\n");
        fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
        if(sscanf(ac_inputBuf, "%d", &readInt) == 1)
        {
          retValue=SC_SetSelChan(FALSE, (UINT8)readInt);
          if(retValue != 0)
            printf("SC_SetSelChan failed with error code: %d\n", retValue);
        }
        else
        {
          printf("No change\n");
        }
      break;

      case 'E': case 'e': //ESMCOptionsMenu
        chanAttributesMenu();
        printMenu = TRUE;
      break;

      case 'u': case 'U': case 'x': case 'X':
        menuOption = 'u';
        printf("Up to previous menu...\n");
      break;

      default:
        printf("Invalid option.\n");
      case 'h': case 'H': case '?':
        printMenu = TRUE;
      break;
    }
  }
}

/*--------------------------------------------------------------------------
    void chanAttributesMenu(void)

Description:
The ESMC/SSM options menu

Return value:

Parameters:
  Inputs


  Outputs

Global variables affected and border effects:
  Read

  Write

--------------------------------------------------------------------------*/

static void chanAttributesMenu(void)
{
  char menuOption = 0;
  BOOLEAN printMenu = TRUE; //print menu the first time
  char ac_inputBuf[80];
  char choice;
  int retValue;
  int readInt;
  int i;
  int currentChannel = -1;
  struct timespec currentTime; //in CLOCK_MONOTONIC timescale

  UINT8 numChannels = 0;
  SC_t_ChanConfig chanConfig[SC_TOTAL_CHANNELS];

  t_chanAttributes chanAttributes;

  //get the number of channels
  retValue = SC_GetChanConfig(chanConfig, NULL, NULL, &numChannels);
  if(retValue != 0)
  {
    printf("SC_GetchanConfig failed with error code: %d\n", retValue);
    return;
  }

  while((menuOption != 'u') && (!o_terminating)) //go UP
  {
    if(printMenu)
    {
      printMenu = FALSE;
      printf("ESMC/SSM and channel attributes diagnostics and test menu.\n");
      printf("Select from the following options:\n");
      printf("\ts  - Show Current Status\n");
      printf("\n");
      printf("\t1  - Set ESMC/SSM Mode and Parameters\n");
      printf("\t2  - Set Channel Validity Mode and Value\n");
      printf("\n");
      printf("\tu  - Up to Previous Menu\n");
      printf("\th  - Print This List\n");
    }

    //read
    printf("? for options ->\n");
    fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
    sscanf(ac_inputBuf, "%c", &menuOption);

    //dispatch
    switch(menuOption)
    {
      case 'S': case 's': case '0': //Show current status
        printf("Number of configured channels: %d\n", numChannels);
        printf("+-----------------------------------------+\n");
        printf("|# |        ESMC/SSM         |   Valid    |\n");
        printf("|  | SSM | OK | TTL  | Mode  | OK | Mode  |\n");
        printf("+-----------------------------------------+\n");
        for(i=0; i<numChannels; i++)
        {
          printf("|%-2d", i); //channel number

          //get values as the SoftClock sees them
          retValue = SC_ChanSSM(i, &chanAttributes.ESMCValue, &chanAttributes.isESMCValid);
          if(retValue != 0)
            printf("SC_ChanSSM failed with error code: %d\n", retValue);
          else
          {
            // QL & OK
            printf("|%-4d |%-3c ", chanAttributes.ESMCValue, trueFalseIntToChar(chanAttributes.isESMCValid));

            //for the TTL we need the RAW info
            retValue = chanAttributes_RAW(e_PARAM_GET, i, &chanAttributes);
            if(retValue != 0)
              printf("chanAttributes_RAW failed with error code: %d\n", retValue);
            else
            {
              // TTL
              if(chanAttributes.ESMCGoodUntil == 0)
                printf("|%-5s ", "Inf.");
              else
              {
                clock_gettime(CLOCK_MONOTONIC, &currentTime);
                if(currentTime.tv_sec > chanAttributes.ESMCGoodUntil)
                  printf("|%-5s ", "Exp.");
                else
                  printf("|%-5ld ", chanAttributes.ESMCGoodUntil - currentTime.tv_sec);
              }

              //mode
              if(chanAttributes.ESMCOverride)
                printf("|%-6s ", "Manual");
              else
                printf("|%-6s ", "Normal");

              //For the valid part we can use the RAW that we already have
              // OK
              printf("|%-3c ", trueFalseIntToChar(chanAttributes.isChanValid));

              // Mode
              if(chanAttributes.chanValidOverride)
                printf("|%-6s ", "Manual");
              else
                printf("|%-6s ", "Normal");
            }
          }
          printf("|\n");
          printf("+-----------------------------------------+\n");
        }
      break;

      case '1': // Set ESMC/SSM mode and parameters
        currentChannel = getChannelNumber(numChannels);
        if(currentChannel != -1)
        {
          retValue = chanAttributes_RAW(e_PARAM_GET, currentChannel, &chanAttributes);
          if(retValue != 0)
            printf("chanAttributes_RAW failed with error code: %d\n", retValue);
          else
          {
            printf("Set channel %d in manual override mode for ESMC/SSM? (Y/N)\n", currentChannel);
            fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
            if(sscanf(ac_inputBuf, "%c", &choice) == 1)
            {
              if(trueFalseCharToInt(choice))
              { // manual override
                chanAttributes.ESMCOverride = TRUE;
                printf("ESMC/SSM value? (Enter=NC)\n");
                fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
                readInt = chanAttributes.ESMCValue;
                if (sscanf(ac_inputBuf, "%d", &readInt) == 1)
                {
                  chanAttributes.ESMCValue = (UINT8)readInt;
                }
                else
                {
                  printf("No change\n");
                }
                chanAttributes.isESMCValid = TRUE;
              }
              else
              { // set to auto
                chanAttributes.ESMCOverride = FALSE;
                chanAttributes.isESMCValid = FALSE;
              }

              chanAttributes_RAW(e_PARAM_SET, currentChannel, &chanAttributes);
            }
          }
        }
      break;

      case '2': // Set channel validity mode and value
        currentChannel = getChannelNumber(numChannels);
        if(currentChannel != -1)
        {
          retValue = chanAttributes_RAW(e_PARAM_GET, currentChannel, &chanAttributes);
          if(retValue != 0)
            printf("chanAttributes_RAW failed with error code: %d\n", retValue);
          else
          {
            printf("Set channel %d in manual override mode for validity? (Y/N)\n", currentChannel);
            fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
            if(sscanf(ac_inputBuf, "%c", &choice) == 1)
            {
              if(trueFalseCharToInt(choice))
              { // manual override
                chanAttributes.chanValidOverride = TRUE;
                printf("Declare channel as valid? (Y/N, Enter=NC)\n");
                fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
                //choice = trueFalseCharToInt(chanAttributes.isChanValid);
                sscanf(ac_inputBuf, "%c", &choice);
                if (!Is_EOL(choice))
                {
                  chanAttributes.isChanValid = trueFalseCharToInt(choice);
                }
                else
                {
                  printf("No change\n");
                }
              }
              else
              { // set to auto
                chanAttributes.chanValidOverride = FALSE;

                //for GPS & PTP make them always valid when going to auto
                switch(chanConfig[currentChannel].e_ChanType)
                {
                  case e_SC_CHAN_TYPE_PTP:
                  case e_SC_CHAN_TYPE_GPS:
                    chanAttributes.isChanValid = TRUE;
                  break;

                  case e_SC_CHAN_TYPE_SE:
                  case e_SC_CHAN_TYPE_E1:
                  case e_SC_CHAN_TYPE_T1:
                  case e_SC_CHAN_TYPE_RESERVED:
                  default:
                    //no need to change, the driver will do it eventualy...
                  break;
                }
              }

              chanAttributes_RAW(e_PARAM_SET, currentChannel, &chanAttributes);
            }
          }
        }
      break;

      case 'u': case 'U': case 'x': case 'X':
        menuOption = 'u';
        printf("Up to previous menu...\n");
      break;

      default:
        printf("Invalid option.\n");
      case 'h': case 'H': case '?':
        printMenu = TRUE;
      break;
    }
  }
}


/*--------------------------------------------------------------------------
    static void redundancyMenu(void)

Description:
Allows manually test the redundancy options
Inputs:
        None

Outputs:
        None

Return value:
        None

--------------------------------------------------------------------------*/
#if (SC_RED_CHANNELS > 0)
static void redundancyMenu(void)
{
  char menuOption = 0;
  BOOLEAN printMenu = TRUE; //print menu the first time
  char ac_inputBuf[80];
  const char *kNAStr = "N/A";

  t_ptpTimeStampType remoteTime, localTime;
  UINT32 remoteEventMap, localEventMap;
  SC_t_activeStandbyEnum remoteStatus, localStatus;
  struct timespec localReceptionTime, currentMonotonicTime;
  int remoteDataAvailable;
  BOOLEAN localRedEnabled;
  INT32 phase;

  while((menuOption != 'u') && (!o_terminating)) //go UP, return to main
  {
    if(printMenu)
    {
      printMenu = FALSE;
      printf("Redundancy Test Menu.\n");
      printf("Select from the following options:\n");
      printf("\ts  - Redundancy Status\n");
      printf("\n");
      printf("\t1  - Attempt to Reverse Active/Standby Roles\n");
      printf("\n");
      printf("\t2  - Set This Unit as Active\n");
      printf("\t3  - Set This Unit as Standby\n");
      printf("\n");
      printf("\t4  - Set Remote Unit as Active\n");
      printf("\t5  - Set Remote Unit as Standby\n");
      printf("\n");
      printf("\tt  - Toggle Printing of Communication Messages\n");
      printf("\n");
      printf("\tu  - Up to Previous Menu\n");
      printf("\th  - Print This List\n");
    }

    //read
    printf("? for options ->\n");
    fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
    sscanf(ac_inputBuf, "%c", &menuOption);

    //dispatch
    switch(menuOption)
    {
      case 'S': case 's': case '0':
        clock_gettime(CLOCK_MONOTONIC, &currentMonotonicTime);
        //get the data from the remote site
        if(getPeerInfo(&remoteTime, &remoteEventMap, &remoteStatus, &localReceptionTime) == 0)
        {
          remoteDataAvailable = TRUE;
        }
        else
        {
          remoteDataAvailable = FALSE;
        }

        //get the local data
        localTime.u48_sec = gs_timeCode.l_timeSeconds;
        localTime.dw_nsec = gs_timeCode.l_TimeNanoSec;
        SC_GetEventMap(&localEventMap);
        if(SC_GetRedundantStatus(&localRedEnabled, &localStatus) != 0)
        {
          localStatus = e_SC_ACTIVE; //redundancy not enabled, we have to be active!
          printf("Warning: SC_GetRedundantStatus returned error. Redundancy may not be enabled on this unit.\n");
        }

        printf("Redundancy Status\n");
        printf("+--------------------------------------------------+\n");
        printf("|State           |     Local      |     Remote     |\n");
        printf("+--------------------------------------------------+\n");

        printf("|%-15s ", "Time (TAI)");
        printf("|%15llu ", localTime.u48_sec);
        if(remoteDataAvailable)
          printf("|%15llu ", remoteTime.u48_sec);
        else
          printf("|%15s ", kNAStr);
        printf("|\n");

        printf("|%-15s ", "State");
        printf("|%-15s ", eventMapToString(localEventMap));
        if(remoteDataAvailable)
          printf("|%-15s ", eventMapToString(remoteEventMap));
        else
          printf("|%-15s ", kNAStr);
        printf("|\n");

        printf("|%-15s ", "Role");
        printf("|%-15s ", get_RedundancyRoleNames(localStatus));
        if(remoteDataAvailable)
          printf("|%-15s ", get_RedundancyRoleNames(remoteStatus));
        else
          printf("|%-15s ", kNAStr);
        printf("|\n");

        printf("|%-15s ", "Age (ms)");
        printf("|%15s ", kNAStr);
        if(remoteDataAvailable)
          printf("|%15u ", diff_usec(localReceptionTime, currentMonotonicTime)/1000);
        else
          printf("|%15s ", kNAStr);
        printf("|\n");

        printf("+--------------------------------------------------+\n");
        printf("\n");

        if(getLastPeerTimestampToSoftClock(&phase, &remoteTime, &localReceptionTime) == 0)
        {
            printf("The last call to SC_TimeMeasToServo() for the redundancy channel was made %u ms ago:\n",
                   diff_usec(localReceptionTime, currentMonotonicTime)/1000);
            printf("Phase: %u ns, Time: (TAI) ", phase);
            if(remoteTime.u48_sec != 0)
              printf("%llu\n", remoteTime.u48_sec);
            else
              printf("%s\n", kNAStr);
        }
        else
        {
          printf("No call to SC_TimeMeasToServo() for the redundancy channel so far\n");
        }
        break;

      case '1':
        //get the current state of the remote peer
        clock_gettime(CLOCK_MONOTONIC, &currentMonotonicTime);
        //get the data from the remote site
        if(getPeerInfo(NULL, NULL, &remoteStatus, &localReceptionTime) == 0)
        {
          remoteDataAvailable = TRUE;
        }
        else
        {
          remoteDataAvailable = FALSE;
        }

        //get the local data
        if(SC_GetRedundantStatus(&localRedEnabled, &localStatus) != 0)
        {
          localStatus = e_SC_ACTIVE; //redundancy not enabled, we have to be active!
          printf("Warning: SC_GetRedundantStatus returned error. Redundancy may not be enabled on this unit.\n");
        }

        //check if we have fresh data, newer than 1.2 seconds
        if((remoteDataAvailable != FALSE) && (diff_usec(localReceptionTime, currentMonotonicTime) < 1200000))
        {
          //based on the local unit decide what to do
          switch(localStatus)
          {
            case e_SC_ACTIVE:
              if(remoteStatus == e_SC_STANDBY)
              {
                sendCommandToPeer('a');
                SC_RedundantEnable(TRUE);
                printf("Setting this unit as %s and remote unit as %s.\n",
                    get_RedundancyRoleNames(e_SC_STANDBY), get_RedundancyRoleNames(e_SC_ACTIVE));
              }
              else
              {
                printf("Cannot switch roles. This unit is %s and the remote one is %s.\n",
                    get_RedundancyRoleNames(localStatus), get_RedundancyRoleNames(remoteStatus));
              }
              break;

            case e_SC_STANDBY:
              if(remoteStatus == e_SC_ACTIVE)
              {
                SC_RedundantEnable(FALSE);
                sendCommandToPeer('s');
                printf("Setting this unit as %s and remote unit as %s.\n",
                    get_RedundancyRoleNames(e_SC_ACTIVE), get_RedundancyRoleNames(e_SC_STANDBY));
              }
              else
              {
                printf("Cannot switch roles. This unit is %s and the remote one is %s.\n",
                    get_RedundancyRoleNames(localStatus), get_RedundancyRoleNames(remoteStatus));
              }
              break;

            default:
              printf("Cannot switch roles. This unit is %s and the remote one is %s.\n",
                  get_RedundancyRoleNames(localStatus), get_RedundancyRoleNames(remoteStatus));
              break;
          }
        }
        else
          printf("Error: Not receiving data from peer.\n");
        break;

      case '2':
        printf("SC_RedundantEnable(FALSE) ");
        if(SC_RedundantEnable(FALSE) == 0) //go to Active
          printf("completed OK.\n");
        else
          printf("failed.\n");
        break;

      case '3':
        printf("SC_RedundantEnable(TRUE) ");
        if(SC_RedundantEnable(TRUE) == 0) //go to Standby
          printf("completed OK.\n");
        else
          printf("failed.\n");
        break;

      case '4':
        printf("sendCommandToPeer(a) ");
        if(sendCommandToPeer('a') == 0) //peer please go to Active
          printf("completed OK.\n");
        else
          printf("failed.\n");
        break;

      case '5':
        printf("sendCommandToPeer(s) ");
        if(sendCommandToPeer('s') == 0) //peer please go to Standby
          printf("completed OK.\n");
        else
          printf("failed.\n");
        break;

      case 'T': case 't':
        printf("Printing of communication messages ");
        if(setRedPckTrace(2) == 1)
          printf("enabled.\n");
        else
          printf("disabled.\n");
        break;

      case 'u': case 'U': case 'x': case 'X':
        menuOption = 'u';
        printf("Up to previous menu...\n");
        break;

      default:
        printf("Invalid option.\n");
      case 'h': case 'H': case '?':
        printMenu = TRUE;
        break;
    }
  }
}
#endif
/*--------------------------------------------------------------------------
    static void phaseMenu(void)

Description:
Allows manually test the phase options
Inputs:
        None

Outputs:
        None

Return value:
        None

--------------------------------------------------------------------------*/

static void phaseMenu(void)
{
  char menuOption = 0;
  BOOLEAN printMenu = TRUE; //print menu the first time
  char ac_inputBuf[80];
  //char strBuffer[256];
  //char choice;
  int retValue;
  //int readInt;
  UINT32 phaseThreshold;
  //int i;


  while((menuOption != 'u') && (!o_terminating)) //go UP, return to main
  {
    if(printMenu)
    {
      printMenu = FALSE;
      printf("Phase Options Menu.\n");
      printf("Select from the following options:\n");
      printf("\ts  - Show SC_SyncPhaseThreshold\n");
      printf("\n");
      printf("\t1  - Set SC_SyncPhaseThreshold\n");
      printf("\t2  - SC_SyncPhaseNow\n");
      printf("\n");
      printf("\tu  - Up to Previous Menu\n");
      printf("\th  - Print This List\n");
    }

    //read
    printf("? for options ->\n");
    fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
    sscanf(ac_inputBuf, "%c", &menuOption);

    //dispatch
    switch(menuOption)
    {
      case 'S': case 's': case '0':
        retValue = SC_SyncPhaseThreshold(e_PARAM_GET, &phaseThreshold);
        if(retValue != 0)
        {
          printf("Error: SC_SyncPhaseThreshold(e_PARAM_GET) returned %d.\n", retValue);
        }
        else
        {
          printf("SC_SyncPhaseThreshold: %u.\n", phaseThreshold);
        }
        break;

      case '1': //set
        printf("SC_SyncPhaseThreshold value: (use 1000000000 to disable)\n");
        fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
        if(sscanf(ac_inputBuf, "%u", &phaseThreshold) == 1)
        {
          retValue = SC_SyncPhaseThreshold(e_PARAM_SET, &phaseThreshold);
          if(retValue != 0)
          {
            printf("Error: SC_SyncPhaseThreshold(e_PARAM_SET, %u) returned %d.\n", phaseThreshold, retValue);
          }
        }
        else
        {
          printf("Error: Invalid value.\n");
        }
        break;

      case '2': //sync phase now!
        retValue = SC_SyncPhaseNow();
        switch(retValue)
        {
          case 0:
            printf("SC_SyncPhaseNow completed OK\n");
            break;

          case -2:
            printf("Error: SC_SyncPhaseNow failed. No time reference.\n");
            break;

          case -3:
            printf("Error: SC_SyncPhaseNow failed. No jam necessary.\n");
            break;

          case -1:
          default:
            printf("Error: SC_SyncPhaseNow failed with error %d\n", retValue);
            break;
        }
        break;


      case 'u': case 'U': case 'x': case 'X':
        menuOption = 'u';
        printf("Up to previous menu...\n");
        break;

      default:
        printf("Invalid option.\n");
      case 'h': case 'H': case '?':
        printMenu = TRUE;
        break;
    }
  }
}


/*--------------------------------------------------------------------------
    static void frequencyMenu(void)

Description:
Allows manually test the redundancy options
Inputs:
        None

Outputs:
        None

Return value:
        None

--------------------------------------------------------------------------*/

static void frequencyMenu(void)
{
  char menuOption = 0;
  BOOLEAN printMenu = TRUE; //print menu the first time
  char ac_inputBuf[80];
  //char strBuffer[256];
  //char choice;
  //int readInt;
  UINT32 outputFrequency;
  //int i;


  while((menuOption != 'u') && (!o_terminating)) //go UP, return to main
  {
    if(printMenu)
    {
      printMenu = FALSE;
      printf("Output Frequency Options Menu.\n");
      printf("Select from the following options:\n");
      printf("\ts  - Show the current output frequency\n");
      printf("\n");
      printf("\t1  - 8 KHz\n");
      printf("\t2  - T1 (1.544 MHz)\n");
      printf("\t3  - E1 (2.048 MHz)\n");
      printf("\t4  - 10 MHz\n");
      printf("\t5  - 25 MHz\n");
      printf("\n");
      //printf("\tn  - Set the Frequency to a User Specified Value\n");
      printf("\tu  - Up to Previous Menu\n");
      printf("\th  - Print This List\n");
    }

    //read
    printf("? for options ->\n");
    fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
    sscanf(ac_inputBuf, "%c", &menuOption);

    //dispatch
    switch(menuOption)
    {
      case 'S': case 's': case '0':
        outputFrequency = setExtOutFreq(0);
        if(outputFrequency != 0)
        {
          printf("Output Frequency is %u Hz\n", outputFrequency);
        }
        else
        {
          printf("setExtOutFreq(0) returned an error.\n");
        }
        break;

      case '1': case '2': case '3': case '4': case '5':
        switch(menuOption)
        {
          case '1':  outputFrequency =     8000; break;
          case '2':  outputFrequency =  1544000; break;
          case '3':  outputFrequency =  2048000; break;
          case '4':  outputFrequency = 10000000; break;
          case '5':  outputFrequency = 25000000; break;
          default:   outputFrequency =        0; break;
        }
        outputFrequency = setExtOutFreq(outputFrequency);
        if(outputFrequency != 0)
        {
          printf("Output Frequency is %u Hz\n", outputFrequency);
        }
        else
        {
          printf("setExtOutFreq(%s) returned an error.\n", ac_inputBuf);
        }
        break;

      case 'N': case 'n':
        printf("Output Frequency in Hz? (Enter=NC)\n");
        fgets(ac_inputBuf, sizeof(ac_inputBuf)-1, stdin);
        if(sscanf(ac_inputBuf, "%u", &outputFrequency) == 1)
        {
          outputFrequency = setExtOutFreq(outputFrequency);
          if(outputFrequency != 0)
          {
            printf("Output Frequency is %u Hz\n", outputFrequency);
          }
          else
          {
            printf("setExtOutFreq(%s) returned an error.\n", ac_inputBuf);
          }
        }
        break;

      case 'u': case 'U': case 'x': case 'X':
        menuOption = 'u';
        printf("Up to previous menu...\n");
        break;

      default:
        printf("Invalid option.\n");
      case 'h': case 'H': case '?':
        printMenu = TRUE;
        break;
    }
  }
}

/*--------------------------------------------------------------------------
    static int getChannelNumber(int numChan)

Description:
Ask for a channel number. Last channel number used is the default

Inputs:
        Number of channels.
        number typed has to be 0<= N < numChan

Outputs:
        None

Return value:
        Number typed/selected
--------------------------------------------------------------------------*/
static int getChannelNumber(const int numChan)
{
 static int lastChan = -1;
 char inLine[10];
 int theChan;

 if((lastChan >= 0) && (lastChan < numChan))
    printf("Channel number (enter for %d)?", lastChan);
  else
    printf("Channel number?");

  fgets(inLine, sizeof(inLine)-1, stdin);
  if(sscanf(inLine, "%d", &theChan) != 1)
  { //hit enter
    if((lastChan >= 0) && (lastChan < numChan))
      return lastChan;
  }
  else
  { //typed a number
    if((theChan >= 0) && (theChan < numChan))
       return lastChan = theChan;
  }
  printf("Invalid channel number. Channel number must be between 0 and %d\n", numChan-1);
  return -1;
}

/*--------------------------------------------------------------------------
    static BOOLEAN askConfirmation(char defaultAnswer)
--------------------------------------------------------------------------*/
static BOOLEAN askConfirmation(const char defaultAnswer)
{
  char inLine[10];
  char choice = defaultAnswer;

  printf("Are you sure you want to continue? (Y/N, Enter=%c)\n", defaultAnswer);

  fgets(inLine, sizeof(inLine)-1, stdin);
  sscanf(inLine, "%c", &choice);
  if(Is_EOL(choice))
    choice = defaultAnswer;

  return (choice == 'Y' || choice == 'y')?TRUE:FALSE;
}



/*--------------------------------------------------------------------------
    static BOOLEAN askWantToChange(char defaultAnswer)
--------------------------------------------------------------------------*/
static BOOLEAN askWantToChange(const char defaultAnswer)
{
  char inLine[10];
  char choice = defaultAnswer;

  printf("Do you want to change the value? (Y/N, Enter=%c)\n", defaultAnswer);

  fgets(inLine, sizeof(inLine)-1, stdin);
  sscanf(inLine, "%c", &choice);
  if(Is_EOL(choice))
    choice = defaultAnswer;

  return (choice == 'Y' || choice == 'y')?TRUE:FALSE;
}

/*--------------------------------------------------------------------------
    char trueFalseIntToChar(int n)
--------------------------------------------------------------------------*/
char trueFalseIntToChar(const int n)
{
  return (n==0)?'N':'Y';
}

/*--------------------------------------------------------------------------
    int trueFalseCharToInt(char c)
--------------------------------------------------------------------------*/
int trueFalseCharToInt(const char c)
{
  return (c=='1' || c=='T' || c=='t'|| c=='Y'|| c=='y')?1:0;
}

/*
----------------------------------------------------------------------------
                                ten_Hz_thread()

Description:
Low priority thread to do some debug status printing/processing

Parameters:

Inputs
        None

Outputs
        None

Return value:

Global variables affected and border effects:
  Read
    gRcvEvtCount, gRcvGnlCount, gSndEvtCount
    gSndGnlCount, gRcvTsCount, gTmxTsCount;

  Write
    none

-----------------------------------------------------------------------------
*/

void *tenHzThread(void *arg)
{
  const UINT32 kTaskPeriod_us = 1000000/10;
  const UINT32 kGuardTime_us = kTaskPeriod_us/10; //will never sleep less than this
  UINT32 elapsedTime;
  UINT32 tenHzCounter = 0;
  struct timespec startTime, endTime;
  char strBuf[256];

  //for the one second trace
  extern int gRcvEvtCount;
  extern int gRcvGnlCount;
  extern int gSndEvtCount;
  extern int gSndGnlCount;
  extern int gRcvTsCount;
  extern int gTmxTsCount;

  //for the timecode print
  UINT32 lastTimeCode = 0;


  while (!o_terminating)
  {
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    tenHzCounter++;

#if (SC_RED_CHANNELS > 0)
    redundancyTask(&startTime);
#endif

    //handle the one second trace
    if(s_localDebCfg.one_sec_trace &&((tenHzCounter % 10) == 1))
    {
      printf("%04d, Rcv ev= %02d, gn= %02d, Snd ev= %02d, gn= %02d, RcvTs= %02d, TmxTs= %02d\n", tenHzCounter/10,
          gRcvEvtCount, gRcvGnlCount, gSndEvtCount, gSndGnlCount, gRcvTsCount, gTmxTsCount);
      gRcvEvtCount = 0;
      gRcvGnlCount = 0;
      gSndEvtCount = 0;
      gSndGnlCount = 0;
      gRcvTsCount = 0;
      gTmxTsCount = 0;
    }

    //check if we have a new TimeCode
    if(lastTimeCode != gd_timeCodeCounter)
    {
      if(go_printTimeCode)
      {
        timeCodeToString(&gs_timeCode, strBuf, 255);
        printf("%s\n", strBuf);
      }
      lastTimeCode = gd_timeCodeCounter;
    }

    //sleep for the rest of the period. ZZZ... ZZZ...
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    elapsedTime = diff_usec(startTime, endTime);
    if(elapsedTime < kTaskPeriod_us)
    {
      if((kTaskPeriod_us - elapsedTime) >= kGuardTime_us)
      {
        usleep(kTaskPeriod_us - elapsedTime);
      }
      else
      {
        usleep(kGuardTime_us);
      }
    }
    else
      usleep(kGuardTime_us);
  }

  pthread_exit(NULL);
}

/*
----------------------------------------------------------------------------
                                SC_TimeCode()

Description: This function needs to be written by the user.
It is called by the SCi every second to deliver time code information.
This function should not block or do any I/O
Copy the data to a local structure an return.
The time is TAI time for the next PPS second.

Parameters:

Input
 None

Output
 Pointer to a t_timeCodeType structure.

Return value:
   None

Remarks:
-----------------------------------------------------------------------------
*/
void SC_TimeCode(const t_timeCodeType *tc)
{ //this
  memcpy(&gs_timeCode, tc, sizeof(gs_timeCode)); //a copy used for printing TOD

#if (SC_RED_CHANNELS > 0)
  setOutboundTimestamp(tc->l_timeSeconds, 0);
#endif

  gd_timeCodeCounter++;
}

static void timeCodeToString(const t_timeCodeType *tc, char * strBuf, size_t bufLen)
{
  char wrkStr[256];
  time_t timeSeconds;
  struct tm convertedTime;

  strBuf[0] = 0;

  switch(tc->e_origin)
  {
    case e_TIME_NEVER_SET:
      sprintf(wrkStr, "Time never set, ");
    break;

    case e_TIME_PTP:
      sprintf(wrkStr, "Time PTP, ");
    break;

    case e_TIME_GPS:
      sprintf(wrkStr, "Time GPS, ");
    break;

    default:
      sprintf(wrkStr, "Time origin %d, ", tc->e_origin);
    break;
  }
  strncat(strBuf, wrkStr, bufLen-strlen(strBuf)-1);

  timeSeconds = tc->l_timeSeconds;
  gmtime_r(&timeSeconds, &convertedTime);
  sprintf(wrkStr, "%04d-%02d-%02d %02d:%02d:%02d.%09d, ",
      (UINT32)convertedTime.tm_year+1900,
      (UINT32)convertedTime.tm_mon+1,
      (UINT32)convertedTime.tm_mday,
      (UINT32)convertedTime.tm_hour,
      (UINT32)convertedTime.tm_min,
      (UINT32)convertedTime.tm_sec,
      tc->l_TimeNanoSec);
  strncat(strBuf, wrkStr, bufLen-strlen(strBuf)-1);

  sprintf(wrkStr, "UTC offset=%d, UTCV=%c, LI-61=%c, LI-59=%c, PTP TS=%c, AS=%c",
      (UINT32)tc->i_utcOffset,
      trueFalseIntToChar(tc->o_utcOffsetValid),
      trueFalseIntToChar(tc->o_leap61),
      trueFalseIntToChar(tc->o_leap59),
      trueFalseIntToChar(tc->o_ptpTimescale),
      trueFalseIntToChar(tc->o_activelySourced));
  strncat(strBuf, wrkStr, bufLen-strlen(strBuf)-1);
}


/*
----------------------------------------------------------------------------
    static char *eventMapToString(const UINT32 eventMap)

Description: Takes a event map as returned by SC_GetEventMap and returns
a human readable format. It converts the eventMap into a enumeration and then
uses get_fll_state_desc(). Since the bit map can represent more than one state at the same time
then it tries to return the "worst" possible state represented in the bitmap

Parameters:

  Input
    UINT32 eventMap a returned by SC_GetEventMap

  Output
    None

Return value:
   Pointer to a string with the ASCII name of the state

Remarks:
-----------------------------------------------------------------------------
*/
const static char *eventMapToString(const UINT32 eventMap)
{
  static const char *kFreeRunStr = "Free run"; //not handled by get_fll_state_desc
  static const char *kLockedStr = "Locked"; //This covers Normal FLL and Fast FLL

  if((eventMap & (1 << e_FREERUN)) != 0)
    return kFreeRunStr;

  if((eventMap & (1 << e_HOLDOVER)) != 0)
    return get_fll_state_desc(e_FLL_STATE_HOLDOVER);

  if((eventMap & (1 << e_BRIDGING)) != 0)
    return get_fll_state_desc(e_FLL_STATE_BRIDGE);

  //It must be OK then...
  return kLockedStr;
}
