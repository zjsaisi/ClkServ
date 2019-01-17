
//*****************************************************************************
// GPS IP Centre, ST-Ericsson (UK) Ltd.
// Copyright (c) 2009 ST-Ericsson (UK) Ltd.
// 15-16 Cottesbrooke Park, Heartlands Business Park, Daventry, NN11 8YL, UK.
// All rights reserved.
//
// Filename  GN_GPS_Example.h
//
// $Id: GNS/GN_GPS_Task.h 1.9 2011/04/01 15:09:27PDT Daniel Brown (dbrown) Exp  $
//
//*****************************************************************************

//*****************************************************************************
//
//  Definitions related to the GN GPS Task source code which is used to
//  activate the GN GPS Library.
//
//*****************************************************************************

#ifndef GN_GPS_TASK_H
#define GN_GPS_TASK_H

// Include all the header files required for the GN_GPS_Task modules here.

#include <stdio.h>
#if 0
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#endif

#include "gps_ptypes.h"
#include "GN_GPS_api.h"
#include "GN_AGPS_api.h"
#include "sc_api.h"

//*****************************************************************************
//
//  GN GPS Linux Implementation defines
//
//*****************************************************************************

// Define the default GN NMEA Output named pipe
#define GN_PIPE_NMEA             "/var/run/nmeapipe"

// GPS NMEA buffer size (1Kb) ((ie where the Application SW can read it from)
#define GN_NMEA_BUF_SIZE         (1*1024)          // 1Kb NMEA Buffer

// Define which GNS???? chip patchs should be included in the build.
// Including patches not required is not harmful but does increase the code size.
//#define GN_GPS_GNB_PATCH_205     // Include patch for GNS4540 ROM2 v205
#define GN_GPS_GNB_PATCH_301     // Include patch for GNS4540 ROM3 v301
//#define GN_GPS_GNB_PATCH_502     // Include patch for GNS7560 ROM5 v502
#define GN_GPS_GNB_PATCH_506     // Include patch for GNS7560 ROM5 v506
#define GN_GPS_GNB_PATCH_510     // Include patch for GNS7560 ROM5 v510

// Define the GN GPS Diagnostic data file names.
// Commenting any of these out will remove the corresponding debug log file.
#define GN_LOG_DIR               "/var/logs/"
// Define the GN GPS Diagnostic data file names.
// Commenting any of these out will remove the corresponding debug log file.
#define GN_FILENAME_NMEA         "GN_NMEA.txt"
// Define the GN GPS Diagnostic data file names.
// Commenting any of these out will remove the corresponding debug log file.
#define GN_FILENAME_TIME         "GN_TIME.txt"
// Commenting this out will remove the NMEA output tasks and data space
//#define GN_NMEA_ENABLE           1
// Commenting this out will remove the NMEA output file creation
//#define GN_NMEA_FILE_ENABLE    1

// Baseband port command string to set GPIO<0> to mode 3 (1PPS output)
#define GN_1PPS_CMD_STR          "\r\n#COMD 24 18947 20319 &15\r\n#COMD 24 16422 31449 &0D\r\n"

// Number of ms in one second
#define NUM_MS_PER_SEC           1000

// Define that Extra status data should be output to stdout every update.
// TODO:  This should be commented out for a final system
//#define GN_EXTRA_STDOUT

// Define that Extra Real-Time-Clock Debug Data be sent to the Event Log file.
// TODO:  This should be commented out for a final system
//#define GN_EXTRA_RTC_DEBUG

/* DEBUG messages */
//#define GNS_DEBUG 1

/* Task time debug messages */
//#define GNS_TIME_DEBUG 1

/* GN_Log messages */
//#define GNS_LOG_ENABLE 1

//*****************************************************************************
//
//  GN GPS Linux Implementation external data definitions
//
//*****************************************************************************

extern const s_GN_GPS_Version gn_Version; // Host GPS SW Version Details

#ifdef GNS_DEBUG
extern U4 gn_CLK_TCK;                     // Process Clock Tick units from CLK_TCK
#endif

#ifdef GN_NMEA_ENABLE
extern int gn_iPipe_NMEA;                 // Index to the GN NMEA Output Pipe

extern U1 gn_NMEA_buf[GN_NMEA_BUF_SIZE];  // GPS NMEA Data Buffer (ie where the
                                          //    Application SW can read it from)
extern U2 gn_NMEA_buf_bytes;              // GPS NMEA Data Buffer bytes written

extern pthread_mutex_t gn_NMEA_CritSec;   // GPS NMEA Data Buffer critical section
#endif

#if (GN_NMEA_ENABLE && GN_NMEA_FILE_ENABLE)
extern FILE *gn_fp_NMEA;                  // GN NMEA file pointer
#endif

extern U1  gn_Patch_Status;               // Status of GPS baseband patch transmission
extern U1  gn_Patch_Progress;             // % progress of patch upload for each stage

typedef struct RTC_Calib
{
   U4 CTime_Set;           // 'C' Time when the RTC was last Set / Calibrated
   I4 Offset_s;            // (UTC - RTC) calibration offset [s]
   I4 Offset_ms;           // (UTC - RTC) calibration offset [ms]
   U4 Acc_Est_Set;         // Time Accuracy Estimate when RTC was Set / Calibrated [ms]
   U4 checksum;            // RTC Calibration File 32-bit checksum
} s_RTC_Calib;

typedef struct UserPosition
{
   BL UserPosEnabled;      // User position enabled 
   R8 Latitude;            // WGS84 Latitude  [degrees, positive North] 
   R8 Longitude;           // WGS84 Longitude [degrees, positive East] 
   R4 Altitude;            // Altitude above WGS84 Ellipsoid [m] 
} s_UserPosition;

//*****************************************************************************
//
//  Prototypes for External GN GPS Linux Example Task Wrapper functions
//
//*****************************************************************************

// Setup a Linux / POSIX comm Port.
int GN_Port_Setup(
   CH *port,                  // i  - Port Name
   U4  baud,                  // i  - Port Baudrate
   CH *useage );              // i  - Port Usage description


// Set a Linux / POSIX comm Port RTS Line "High".
BL GN_Port_Set_RTS_High(
   int hPort );               // i  - Handle to the open Comm port


// Set a Linux / POSIX comm Port RTS Line "Low".
BL GN_Port_Set_RTS_Low(
   int hPort );               // i  - Handle to the open Comm port


// GNS???? Chip/Module Hardware Abstraction Layer Functions.
BL GN_GNS_HAL_Reset( void );


// Initialise the GN Baseband Patch code upload handling parameters.
void GN_Initialise_GNB_Patch( void );


// This function is called from the GN GPS Library 'callback' function
// GN_GPS_Write_GNB_Patch() to start / set-up the GN Baseband Patch upload.
U2 GN_Setup_GNB_Patch(
   U2 ROM_version,            // i  - GN Baseband Chips ROM SW Version
   U2 Patch_CkSum );          // i  - GN Baseband Chips ROM SW Checksum


// Upload the next set of patch code to the GN GPS baseband.
// This function sends up to a maximum of Max_Num_Patch_Mess sentences each time
// it is called.
// The complete set of patch data is divided into six stages.
void GN_Upload_GNB_Patch(
   U4 Max_Num_Patch_Mess );   // i  - Maximum Number of Patch Messages to Upload


// Upload the next set of patch code to the GPS baseband.
// This function sends up to a maximum of Max_Num_Patch_Mess sentences each time
// it is called.
// The complete set of patch data is divided into six stages.
void GN_Upload_GNB_Patch_506(
   U4 Max_Num_Patch_Mess ); // i  - Maximum Number of Patch Messages to Upload


// Convert Time in YY-MM-DD & HH:MM:SS format to 'C' Time units [seconds].
// ('C' Time starts on the 1st Jan 1970.)
U4 GN_YMDHMS_To_CTime(
   U2 Year,                   // i  - Year         [eg 2006]
   U2 Month,                  // i  - Month        [range 1..12]
   U2 Day,                    // i  - Days         [range 1..31]
   U2 Hours,                  // i  - Hours        [range 0..23]
   U2 Minutes,                // i  - Minutes      [range 0..59]
   U2 Seconds );              // i  - Seconds      [range 0..59]


// Convert Time in 'C' Time units [seconds] to a YY-MM-DD & HH:MM:SS format.
// ('C' Time starts on the 1st Jan 1970.)
void GN_CTime_To_YMDHMS(
   U4 C_Time,                 // i  - Time in 'C' Time units [seconds]
   U2 *Year,                  //  o - Year         [eg 2006]
   U2 *Month,                 //  o - Month        [range 1..12]
   U2 *Day,                   //  o - Days         [range 1..31]
   U2 *Hours,                 //  o - Hours        [range 0..23]
   U2 *Minutes,               //  o - Minutes      [range 0..59]
   U2 *Seconds );             //  o - Seconds      [range 0..59]


// GN_Log:  Sends <stdio> formatted debug Log messages to the
// GN_GPS_Event_Log channel and appends a <CR><LF>.
// The total message length must be less than 250 characters long.
void GN_Log( const char *format, ... );

//*****************************************************************************
// GN_GPS_Task_Start : The main function used when the GN GPS Task is Running.
//
// Arguments:
//
// ps_gpsGenCfg - configuration structure from SC API
// w_gpsTaskIntTime - task call interval time in milliseconds
//
BL GN_GPS_Task_Start (SC_t_GPS_GeneralConfig *ps_gpsGenCfg, U2 w_gpsTaskIntTime);

//*****************************************************************************
// GN_GPS_Task_Stop : Called to trigger the main GN GPS Task to Stop running.
//
BL GN_GPS_Task_Stop (void);

//*****************************************************************************
// GN_GPS_Task_Run : The main function used when the GN GPS Task is Running.
//
BL GN_GPS_Task_Run (void);

//*****************************************************************************
// GN_GPS_Task_Stat : The main function used when the GN GPS Task is Running.
//
BL GN_GPS_Stat(SC_t_GPS_Status *ps_gpsStatus);

//*****************************************************************************
// GN_GPS_Valid : Get GNS GPS Status 3D fix only 

BL GN_GPS_Valid (int chan);

//*****************************************************************************

#endif   // GN_GPS_TASK_H
