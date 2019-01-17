/******************************************************************************/
/* GPS IP Centre, ST-Ericsson (UK) Ltd.                                       */
/* Copyright (c) 2009 ST-Ericsson (UK) Ltd.                                   */
/* 15-16 Cottesbrooke Park, Heartlands Business Park, Daventry, NN11 8YL, UK. */
/* All rights reserved.                                                       */
/*                                                                            */
/* Filename  GN_GPS_Task.c                                                    */
/*                                                                            */
/* $Id: GNS/GN_GPS_Task.c 1.14 2011/05/04 11:06:55PDT Daniel Brown (dbrown) Exp  $                                                                       */
/******************************************************************************/

#include "GN_GPS_Task.h"

#if 0
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#endif

#include "sc_api.h"

//*****************************************************************************
// GN GPS Task related Global variables

#ifdef GN_NMEA_ENABLE
static CH gn_Pipe_NMEA[64]  = GN_PIPE_NMEA;  // GN NMEA Output Pipe name

static pthread_t gn_id_NMEA_Output_Task;     // GN NMEA Output Task Thread id.
static BL gn_NMEA_Output_Task_Exit;          // Set to TRUE to trigger the NMEA Output Task to exit
static BL gn_NMEA_Output_Task_Exited;        // Set to TRUE when the NMEA Output Task has exited

int gn_iPipe_NMEA;                           // Index to the GN NMEA Output Pipe

U1 gn_NMEA_buf[GN_NMEA_BUF_SIZE];            // GPS NMEA Data Buffer (ie where the
                                             //    Application SW can read it from)

U2 gn_NMEA_buf_bytes;                        // GPS NMEA Data Buffer bytes written

pthread_mutex_t gn_NMEA_CritSec;             // GPS NMEA Data Buffer critical section
#endif

#ifdef GNS_DEBUG
U4 gn_CLK_TCK;                               // Process Clock Tick units from CLK_TCK
#endif

#if 0
static CH gn_Clear_NV_Store_Items[16] = "";  // GN Non-Volatile Store Items to Clear at Startup
#endif

static s_GN_GPS_Config gn_GPS_Config;        // GN GPS Configuration data

static s_GN_GPS_Nav_Data gn_Nav_Data;        // The latest GN GPS Nav solution data
static s_GN_GPS_Time_Data gn_Time_Data;      // The latest GN GPS Time solution data

static U2 gn_GPS_Task_Time;                  // GN GPS Call Interval time in milliseconds
static U2 gn_GPS_Task_One_Sec_Cnts;          // GN GPS Number of Cnts in 1 second

static U4 gn_Started_ms;                     // GPS Session Started At Time [ms]
static U4 gn_TTFF;                           // GPS NMEA Time To First Fix [sec]

#if ((GN_NMEA_ENABLE && GN_NMEA_FILE_ENABLE) || GNS_TIME_DEBUG)
static CH gn_Log_Dir[128] = "";              // buffer to store Log Directory pathname
#endif
 
#ifdef GN_NMEA_FILE_ENABLE
FILE *gn_fp_NMEA;                            // GN NMEA file pointer
#endif

#ifdef GNS_TIME_DEBUG
FILE *gn_fp_TIME;                            // GN TIME file pointer
#endif

#ifdef GNS_TIME_DEBUG
static U4 timelaststart = 0;
#endif

#define PI 3.14592654

typedef struct LocDatum    // Local geodetic Datum description data structure
{
   U2 datum_no;               // datum number, 0 = default (WGS84)
   BL rotate;                 // TRUE if a rotation is required
   R8 majA;                   // datum semi-major axis (m)
   R8 majA2;                  // datum (semi-major axis)^2
   R8 ecc2;                   // datum (eccentricity)^2
   R8 ecc4;                   // datum (eccentricity)^4
   R8 ome2;                   // datum ( 1.0 - (eccentricity)^2 )
   R8 dX[3];                  // datum X,Y,Z axis shift at the origin from
                              //   WGS84 to the local datum
   R8 RotM[3][3];             // Rotation about X,Y,Z axis from WGS84 to the
                              //    local datum (any scale change is included
                              //    in the diagonal terms)

}  s_LocDatum;             // Local geodetic Datum description data structure

extern const s_LocDatum WGS84_Datum;
#ifdef GN_NMEA_ENABLE /* only needed if NMEA is enabled and GPINS message is output */
static R8 ECEF[3];
#endif

static BL FixedPosition = FALSE;
static s_GN_AGPS_Ref_Pos AGPS_Pos;

static const CH gn_GpsCmdStr[] = GN_1PPS_CMD_STR;
static s_UserPosition gn_UserPos;

//*****************************************************************************
// Local Prototypes functions 

#ifdef GN_NMEA_ENABLE
static void GN_NMEA_Output_Task( void );
static int OutputGPINS( U1 *NMEA_buffer );
static void Add_NMEA_Checksum(char *index);
static void deg2ints(R8 *input,unsigned int *deg,unsigned int *min,unsigned int *frac);
#endif

static void Do_Self_Survey (void);
static void ClearMem(void *, CH, int);

#if 0
static void GN_Clear_NV_Store(CH Items[16]);
#endif

static BL GN_GPS_Tod (void);


//*****************************************************************************
//  GN GPS Library routine 
//  Geo2ECEF : Convert Geodetic (lat,long,height) to ECEF Cartesian
//  coordinates.  After performing this conversion, the ECEF coordinates
//  are transformed from the local datum to WGS84.
//  This routine assumes that all the local datum tranformation data is
//  already correct and has been set up in the input 'LocDatum' structure.

void Geo2ECEF(
   const R8 GeoLLH[],      // i - Local Datum Geodetic Latitude, Longitude
                           // [radians] and Ellipsoidal Height [m]
   const s_LocDatum *d,    // i - Pointer to Local Datum description data
   R8 ECEF[]);             // o - WGS84 ECEF Cartesian coordinates [m]

//*****************************************************************************
// GN_GPS_Task_Start : The main function used when the GN GPS Task is Running.

BL GN_GPS_Task_Start (SC_t_GPS_GeneralConfig *ps_gpsGenCfg, U2 w_gpsTaskIntTime)
{
#if 0
#ifdef GN_NMEA_ENABLE
   int error;                       // error return code
#endif

#ifdef GNS_DEBUG
   printf("+GN_GPS_Task_Start:\n");
   GN_Log("GN_GPS_Task_Start: START");
#endif
   
#ifdef GNS_DEBUG
   // Determine the units that the platform's Process Clock Ticks at.
   // This is required by GN_GPS_Get_OS_Time_ms() so must not be called before now.
   gn_CLK_TCK = (U4)sysconf(_SC_CLK_TCK);

   printf("gn_CLK_TCK: %u (CLOCKS_PER_SEC: %u)\n", gn_CLK_TCK, (U4)CLOCKS_PER_SEC);
#endif

#if (GN_NMEA_ENABLE && GN_NMEA_FILE_ENABLE)
   // Open the GN GPS Diagnostic Files as required, in appended mode.
   gn_fp_NMEA = NULL;

   strncat( gn_Log_Dir, GN_LOG_DIR, sizeof(gn_Log_Dir));
   strncat( gn_Log_Dir, GN_FILENAME_NMEA, sizeof(gn_Log_Dir) );
   printf(" GN NMEA LOG: %s\n", gn_Log_Dir);
   gn_fp_NMEA = fopen( gn_Log_Dir, "ab" );
   gn_Log_Dir[0] = '\0';                  // clear the strncat
#endif

#ifdef GNS_TIME_DEBUG
   // Open the GN GPS Diagnostic Files as required, in appended mode.
   gn_fp_TIME = NULL;

   strncat( gn_Log_Dir, GN_LOG_DIR, sizeof(gn_Log_Dir));
   strncat( gn_Log_Dir, GN_FILENAME_TIME, sizeof(gn_Log_Dir) );
   printf(" GN TIME LOG: %s\n", gn_Log_Dir);
   gn_fp_TIME = fopen( gn_Log_Dir, "ab" );
   gn_Log_Dir[0] = '\0';                  // clear the strncat
#endif

#ifdef GNS_DEBUG
   // Print a Start-up message to the Event Log
   GN_Log("%s:  %s  %s.", gn_Version.Name, __DATE__, __TIME__ );
#endif

   // If requested, before starting the GN GPS, clear specific data items
   // from the GN GPS Non-volatile store for certain test modes.
/*   if ( gn_Clear_NV_Store_Items[0] != '\0' )
   {
      GN_Clear_NV_Store( gn_Clear_NV_Store_Items );
   }
*/

   // Initialise the GNB patch code upload status to "not started yet" = 0.
   gn_Patch_Status   = 0;
   gn_Patch_Progress = 0;

   // Clear any previous Navigation Data
   ClearMem(&gn_Nav_Data, 0, sizeof(gn_Nav_Data));
   // Clear any previous Time Data
   ClearMem(&gn_Time_Data, 0, sizeof(gn_Time_Data));

#ifdef GN_NMEA_ENABLE
   memset(gn_NMEA_buf,  '\0', sizeof(gn_NMEA_buf));
   gn_NMEA_buf_bytes  = 0;
#endif
   gn_TTFF            = 0;
   gn_Started_ms      = GN_GPS_Get_OS_Time_ms();

#ifdef GN_NMEA_ENABLE
   // Flag the GN NMEA_Task as running.
   gn_NMEA_Output_Task_Exit   = FALSE;
   gn_NMEA_Output_Task_Exited = FALSE;

   // Create the GN_NMEA_Task thread
   error = pthread_create( &gn_id_NMEA_Output_Task,
                           NULL, (void*)&GN_NMEA_Output_Task, NULL );
   if (error != 0)
   {
      printf( " GN_GPS_Task_Start: pthread_create( GN_NMEA_Output_Task ) Error %d, errno %d\n",
                error, errno );
      exit(-1);
   }

   // Create the GN NMEA Critical Section used to protect data exchanges between
   // the main GN GPS processing task and the NMEA Output task.
   pthread_mutex_init(&gn_NMEA_CritSec, NULL);
#endif
   
   // Initialise the GPS software. This API function MUST be called.
   GN_GPS_Initialise();

   // Apply the Host Wrapper Software version details on to the GN_GPS_Library.
   // This is an optional GN GPS API function call.
   GN_GPS_Set_Version((s_GN_GPS_Version*)&gn_Version);

   // Take a copy of the default GN GPS Configuration Settings.
   // Make the required changes and apply it back.
   if (GN_GPS_Get_Config(&gn_GPS_Config))
   {
      // Only over rule the GN GPS Library default regarding BGA vs CSP chip
      // packages if there was an external input to say its definately a CSP.
      gn_GPS_Config.BGA_Chip = (ps_gpsGenCfg->e_rcvrType == e_RCVR_ST_ERIC_GNS7560_BGA);
      // Set cold start timeout value
      gn_GPS_Config.ForceCold_Timeout = ps_gpsGenCfg->ColdStrtTmOut;
      gn_GPS_Config.PosFiltMode = 0;		    // Turn off position filtering

#ifdef GN_NMEA_ENABLE
      gn_GPS_Config.GPGLL_Rate = 0;           // Turn off NMEA $GPGLL
      gn_GPS_Config.GPGGA_Rate = 0;           // Turn off NMEA $GPGGA
      gn_GPS_Config.GPGSA_Rate = 0;           // Turn off NMEA $GPGSA

//      gn_GPS_Config.GPGST_Rate = 0;           // Turn off NMEA $GPGST
//      gn_GPS_Config.GPGSV_Rate = 0;           // Turn off NMEA $GPGSV
//      gn_GPS_Config.GPRMC_Rate = 0;           // Turn off NMEA $GPRMC
//
//      gn_GPS_Config.GPVTG_Rate = 0;           // Turn off NMEA $GPVTG
//      gn_GPS_Config.GPZCD_Rate = 0;           // Turn off NMEA $GPZCD
//      gn_GPS_Config.GPZDA_Rate = 0;           // Turn off NMEA $GPZDA
//      gn_GPS_Config.PGNVD_Rate = 0;           // Turn off NMEA $PGNVD
#else
      gn_GPS_Config.GPGLL_Rate = 0;           // Turn off NMEA $GPGLL
      gn_GPS_Config.GPGGA_Rate = 0;           // Turn off NMEA $GPGGA
      gn_GPS_Config.GPGSA_Rate = 0;           // Turn off NMEA $GPGSA

      gn_GPS_Config.GPGST_Rate = 0;           // Turn off NMEA $GPGST
      gn_GPS_Config.GPGSV_Rate = 0;           // Turn off NMEA $GPGSV
      gn_GPS_Config.GPRMC_Rate = 0;           // Turn off NMEA $GPRMC

      gn_GPS_Config.GPVTG_Rate = 0;           // Turn off NMEA $GPVTG
      gn_GPS_Config.GPZCD_Rate = 0;           // Turn off NMEA $GPZCD
      gn_GPS_Config.GPZDA_Rate = 0;           // Turn off NMEA $GPZDA
      gn_GPS_Config.PGNVD_Rate = 0;           // Turn off NMEA $PGNVD
#endif
      gn_GPS_Config.ColdTTFF = GN_GPS_COLD_TTFF_SENSITIVE;
      gn_GPS_Config.PowerPerf = GN_GPS_POW_PERF_HI_PERF;
      gn_GPS_Config.SensMode = GN_GPS_SENS_MODE_HIGH;

      gn_GPS_Config.NV_Write_Interval = 0;    // Turn off Non-Volatile writes
      gn_GPS_Config.Enable_Nav_Debug = TRUE;  // Turn on Navigation debug
      gn_GPS_Config.Enable_GNB_Debug = 0;     // Turn off GNB debug
      gn_GPS_Config.Enable_Event_Log = 0;     // Turn off Event logging
      
      if (!GN_GPS_Set_Config( &gn_GPS_Config ))
      {
#ifdef GNS_DEBUG
		   printf("Error setting GPS Configuration!\r\n");
#endif
         return(FALSE);
      }
      GN_GPS_Set_Power_Mode(GN_GPS_POWER_ALLOWED_HIGH);
   }

   // Assign local task time variable
   gn_GPS_Task_Time = w_gpsTaskIntTime;

   // Calculate Number of Task Calls in 1 second
   if ( ! (gn_GPS_Task_Time > 0) )
   {
      /* return error if value is no good */
      return(FALSE);
   }
   else
   {
      /* Calculate One Second Counts */
      gn_GPS_Task_One_Sec_Cnts = NUM_MS_PER_SEC / (gn_GPS_Task_Time);
   }

   /* Store User Position info */
   gn_UserPos.UserPosEnabled = ps_gpsGenCfg->UserPosEnabled;
   gn_UserPos.Latitude = ps_gpsGenCfg->Latitude;
   gn_UserPos.Longitude = ps_gpsGenCfg->Longitude;
   gn_UserPos.Altitude = ps_gpsGenCfg->Altitude;
   
#ifdef GNS_DEBUG
   printf("gn_GPS_Task_Time : %d gn_GPS_Task_One_Sec_Cnts %d\n", gn_GPS_Task_Time, gn_GPS_Task_One_Sec_Cnts);
#endif

#ifdef GNS_DEBUG
   printf("-GN_GPS_Task_Start:\n");
#endif

#endif
   return(TRUE);
}


//*****************************************************************************
// GN_GPS_Task_Stop : Called to trigger the main GN GPS Task to Stop running.

BL GN_GPS_Task_Stop (void)
{
#if 0
#ifdef GNS_DEBUG
   printf( "+GN_GPS_Task_Stop:\n" );
   GN_Log( "GN_GPS_Task_Stop:  EXIT" );
#endif

#ifdef GN_NMEA_ENABLE
   // Trigger the NMEA Output Task to exit.
   gn_NMEA_Output_Task_Exit = TRUE;

   // Close down the NMEA Output thread. Note that pthread_join() waits
   // for a thread to exit so we rely on the threads to terminate themselves.
   // If we force a threadto terminate it won't free its resources.
   if (gn_id_NMEA_Output_Task != 0)
   {
      pthread_join(gn_id_NMEA_Output_Task, NULL );
   }
#endif

#if (GN_NMEA_ENABLE && GN_NMEA_FILE_ENABLE)
   if (gn_fp_NMEA != NULL)
   {
      fflush( gn_fp_NMEA);
      fclose( gn_fp_NMEA);
   }
#endif

   // Shutdown the GPS. It is recommended that this API function is called
   // before the GPS software is termionated, to make sure that the latest
   // set of non-volatile data is made available to the host for storing.
   GN_GPS_Shutdown();

#ifdef GNS_DEBUG
   GN_Log( "GN_GPS_Task_Stop:  EXITED" );
   printf( "-GN_GPS_Task_Stop:\n" );
   fflush( stdout );
#endif

#endif

   return(TRUE);
}


//*****************************************************************************
// GN_GPS_Task_Run : The main function used when the GN GPS Task is Running.

BL GN_GPS_Task_Run (void)
{
#if 0
#ifdef GNS_DEBUG
   U4 Nav_Update_Counter = 0;             // Count of Nav Updates completed
#endif
   BL pps_config = FALSE;
   CH cmd_str_buf[60];
   U2 size;
   static U2 runCount = 0;
#ifdef GNS_TIME_DEBUG
   CH timebuf[100];
   U4 timestart;
#endif

#if 0
#ifdef GNS_DEBUG
   printf("+GN_GPS_Task_Run:\n");
#endif
#endif

#ifdef GNS_TIME_DEBUG
   timestart = GN_GPS_Get_OS_Time_ms();
   sprintf(timebuf, "GPS Task Start: %d [ms]\n", timestart);
   fwrite( timebuf, strlen(timebuf), 1, gn_fp_TIME );
   sprintf(timebuf, "GPS Task of Last Start: %d [ms]\n", timelaststart);
   fwrite( timebuf, strlen(timebuf), 1, gn_fp_TIME );
   sprintf(timebuf, "GPS Task Since Last Start: %d [ms]\n", timestart - timelaststart);
   fwrite( timebuf, strlen(timebuf), 1, gn_fp_TIME );
   timelaststart = timestart;
#endif

   // Aim to go round the main processing loop at an average rate of once
   // every 50 milliseconds

   // Check for new data from the GN GPS baseband.
   // This API function MUST be called at regular intervals.

   GN_GPS_Update();

   // Update Run Count
   runCount++;

   /* Check for 1 second boundary */
   if (runCount >= gn_GPS_Task_One_Sec_Cnts)
   {
      /* Do Self Survey, includes setting fixed position */
      Do_Self_Survey();
      /* reset Count */
      runCount = 0;
   }

   // See if a new fix has been generated.
   // If a new fix was not generated (i.e. even though we received
   // data from the baseband) then this probably means that there
   // is more data available (i.e. we only read part of the complete
   // data), so immediately go back and call UART_Input().

   /* if new update is available */
   if (GN_GPS_Get_Nav_Data(&gn_Nav_Data) == TRUE)
   {
      /* A new fix has just been generated get the time data */
      if (GN_GPS_Get_Time_Data(&gn_Time_Data) == TRUE)
      {
         /* call function to calculate TOD and pass to user through API function */
         GN_GPS_Tod();
      }

      // See if the TTFF flag needs setting.
      if (gn_TTFF == 0  &&  gn_Nav_Data.Valid_2D_Fix)
      {
         gn_TTFF = gn_Started_ms - GN_GPS_Get_OS_Time_ms();
      }

#ifdef GNS_DEBUG
      // Output information to the PC Screen, just for visibility.
      // TODO:  Remove this from the final embedded system.
      Nav_Update_Counter++;
#endif

#ifdef GNS_DEBUG
#ifdef GN_EXTRA_STDOUT
      {
         printf("\n%1d ",  gn_Patch_Status);
         //!!!506P008 printf( "%3d ",  gn_Patch_Progress );
         printf("%4d ",  Nav_Update_Counter);
         printf("%10u ", GN_GPS_Get_OS_Time_ms());
         printf("%10d ", gn_Nav_Data.Local_TTag);
         printf("%02d%02d%02d.%03d ",   gn_Nav_Data.Hours,
                                        gn_Nav_Data.Minutes,
                                        gn_Nav_Data.Seconds,
                                        gn_Nav_Data.Milliseconds);
         printf("%10.6f %11.6f %6.1f ", gn_Nav_Data.Latitude,
                                        gn_Nav_Data.Longitude,
                                        gn_Nav_Data.Altitude_MSL);
         {
            U4 SatsTracked = 0;           // Number of Satellite Tracked
            U4 i = 0;
            while (i < NMEA_SV)
            {
               if (gn_Nav_Data.SatsInViewSNR[i++] > 0)  
               {   
                  SatsTracked++;
               }
            }
            printf("%2d/", SatsTracked );
         }
         printf("%2d/%2d  ", gn_Nav_Data.SatsUsed, gn_Nav_Data.SatsInView);
         if (gn_Nav_Data.Valid_3D_Fix)
         {
            printf("3D fix\n");
         }
         else if (gn_Nav_Data.Valid_2D_Fix)
         {
            printf("2D fix\n");
         }
         else
         {
            printf("No fix\n");
         }
      }
#endif
#endif

#ifdef GNS_DEBUG
      // Every 16 updates fixes flush stdout and the Event Log
      if((Nav_Update_Counter%16) == 0)
      {
         fflush(stdout);
      }
#endif

   }
   else
   {
      // No New Nav data was received from the baseband so GN_GPS_Update()
      // did not call GN_GPS_Write_GNB_Ctrl() to send data to the baseband.
      // If there is still some patch code data that needs to be sent to the
      // baseband, call the patch uploader.
      // This main loop repeats every 50ms which means at 115200 baud TX you
      // can send up to 11520*0.050 = 576 bytes, or 22 x 26 byte messages.
      // So target 20 messages, but this number may need to be reduced based
      // on the efficiency of the UART TX Driver.
      if ((gn_Patch_Status > 0)  &&  (gn_Patch_Status < 7))   // Patching in progress
      {
         GN_Upload_GNB_Patch(20);
         pps_config = FALSE;
      }
      else
      {
			// wait for patch download to finish
		   // and if it hasn't already be done, setup the 1pps output port
			if (pps_config == FALSE)
			{ 
				// set GPIO<0> to mode 3 (1pps output)
				sprintf(cmd_str_buf, gn_GpsCmdStr); 
				size = sizeof(gn_GpsCmdStr)-1;
				if (GN_GPS_Write_GNB_Ctrl(size, cmd_str_buf) != size)
				{
#ifdef GNS_DEBUG
					GN_Log( "GN_GPS_Task_Run: ERROR:  1pps output not configured" );
#endif
				}
				pps_config = TRUE;
			}
      }
   }

   /* TODO: Decide whether necessary - Parse input PGNV messages if enabled by user */
   // GN_GPS_Parse_PGNV();

#ifdef GNS_TIME_DEBUG
   sprintf(timebuf, "GPS Task End, Time Elapsed: %d [ms]\n", GN_GPS_Get_OS_Time_ms() - timestart);
   fwrite( timebuf, strlen(timebuf), 1, gn_fp_TIME );
#endif

#if 0
#ifdef GNS_DEBUG
   printf( "-GN_GPS_Task_Run:\n" );
#endif
#endif

#endif
   return(TRUE);
}


//*****************************************************************************
// GN_GPS_Valid : Get GNS GPS Status 3D fix only 
extern int gps_valid;
extern int bd_valid;
extern int irigb1_valid;
extern int irigb2_valid;

BL GN_GPS_Valid (int chan)
{
#if 0
   return gn_Nav_Data.Valid_3D_Fix;
#endif

	if(chan == 0){
		if(gps_valid == TRUE)
			return TRUE;
		else
			return FALSE;
	}

	if(chan == 1){
		if(bd_valid == TRUE)
			return TRUE;
		else
			return FALSE;
	}

	if(chan == 2){
		if(irigb1_valid == TRUE)
			return TRUE;
		else
			return FALSE;
	}
	if(chan == 3){
		if(irigb2_valid == TRUE)
			return TRUE;
		else
			return FALSE;
	}
}

//*****************************************************************************
// GN_GPS_Stat : Get GNS GPS Status.

BL GN_GPS_Stat (SC_t_GPS_Status *ps_gpsStatus)
{
#if 0
   int i;
   
   /* copy local status to user data structure */
   ps_gpsStatus->Valid_2D_Fix = gn_Nav_Data.Valid_2D_Fix;
   ps_gpsStatus->Valid_3D_Fix = gn_Nav_Data.Valid_3D_Fix;
   ps_gpsStatus->SatsInView   = gn_Nav_Data.SatsInView;
   ps_gpsStatus->SatsUsed     = gn_Nav_Data.SatsUsed;
   for (i = 0; i < k_NMEA_SV; i++)
   {
      ps_gpsStatus->SatsInViewSVid[i]  = gn_Nav_Data.SatsInViewSVid[i];
      ps_gpsStatus->SatsInViewSNR[i]   = gn_Nav_Data.SatsInViewSNR[i];
      ps_gpsStatus->SatsInViewAzim[i]  = gn_Nav_Data.SatsInViewAzim[i];
      ps_gpsStatus->SatsInViewElev[i]  = gn_Nav_Data.SatsInViewElev[i];
      ps_gpsStatus->SatsInViewUsed[i]  = gn_Nav_Data.SatsInViewUsed[i];
   }
   ps_gpsStatus->Latitude = gn_Nav_Data.Latitude;
   ps_gpsStatus->Longitude = gn_Nav_Data.Longitude;
   ps_gpsStatus->Altitude_Ell   = gn_Nav_Data.Altitude_Ell;
   ps_gpsStatus->Altitude_MSL     = gn_Nav_Data.Altitude_MSL;

   ps_gpsStatus->H_DOP = gn_Nav_Data.H_DOP;
   ps_gpsStatus->V_DOP = gn_Nav_Data.V_DOP;
   ps_gpsStatus->P_DOP = gn_Nav_Data.P_DOP;

   ps_gpsStatus->T_DOP = gn_Time_Data.T_DOP;

#endif   
   return(TRUE);
}

//*****************************************************************************
// GN_GPS_Stat : Get GNS GPS Status.

static BL GN_GPS_Tod (void)
{
#if 0
   s_GN_AGPS_UTC p_UTC;
   SC_t_GPS_TOD s_gpsTOD;
   U4 TOW;
   R8 TOW_increment;
   
   /* copy week number to data structure */
   s_gpsTOD.Gps_WeekNo = gn_Time_Data.Gps_WeekNo;

   /* Get integer value of GPS TOW */
   TOW = (U4)gn_Time_Data.Gps_TOW;

   /* Get sub-second value of GPS TOW */
   TOW_increment = gn_Time_Data.Gps_TOW - TOW;

   /* if incremental seconds is above the half-second mark */
   if (TOW_increment > 0.5)
   {
      /* round up to nearest integer (second) */
      TOW = TOW + 1;
   }
   s_gpsTOD.Gps_TOW = (UINT64)TOW;

   /* Copy milliseconds */   
   s_gpsTOD.Milliseconds = gn_Time_Data.Milliseconds;

   /* get leap seconds data */
   if (GN_AGPS_Get_UTC(&p_UTC ) == TRUE)
   {
      /* copy leap seconds data */
      s_gpsTOD.dtLS = p_UTC.dtLS;
      s_gpsTOD.WeekNo_LS = p_UTC.WNLSF;
      s_gpsTOD.DayNo_LS = p_UTC.DN;
      s_gpsTOD.dtLSF = p_UTC.dtLSF;
   }
   else
   {
      /* clear leap seconds data */
      s_gpsTOD.dtLS = 0;
      s_gpsTOD.WeekNo_LS = 0;
      s_gpsTOD.DayNo_LS = 0;
      s_gpsTOD.dtLSF = 0;
   }

   /* copy UTC offset */
   s_gpsTOD.UTC_Offset = gn_Time_Data.UTC_Correction;

   /* call user API function */
   SC_GpsTod(&s_gpsTOD);
#endif
  
   return(TRUE);
}

//****************************************************************************
//
// Function:      static void Do_Self_Survey( void )
// 
// Inputs:        None
//
// Returns:       None
//
// Description:   Perform self survey
//
//****************************************************************************

static void Do_Self_Survey (void)
{
#if 0
   static R8 AverageLat = 0.0;
   static R8 AverageLon = 0.0;
   static R8 AverageAlt = 0.0;
   static R8 Average_H_AccMaj = 0.0;               // Horizontal error ellipse semi-major axis [m]
   static R8 Average_H_AccMin = 0.0;               // Horizontal error ellipse semi-minor axis [m]
   static R8 Average_H_AccMajBrg = 0.0;            // Bearing of the semi-major axis [degrees]
   static R8 TotalLat = 0.0;
   static R8 TotalLon = 0.0;
   static R8 TotalAlt = 0.0;
   static R8 Total_H_AccMaj=0.0;                   // Horizontal error ellipse semi-major axis [m]
   static R8 Total_H_AccMin=0.0;                   // Horizontal error ellipse semi-minor axis [m]
   static R8 Total_H_AccMajBrg=0.0;                // Bearing of the semi-major axis [degrees]
   static I4 Count = 0;

	if (gn_Nav_Data.Valid_3D_Fix)
   {
		if ((gn_Nav_Data.H_AccMaj < 200) && (gn_Nav_Data.H_AccMaj > 0))
      {
			Count++;
			TotalLat += gn_Nav_Data.Latitude;
			AverageLat = TotalLat/Count;
			TotalLon += gn_Nav_Data.Longitude;
			AverageLon = TotalLon/Count;
			TotalAlt += gn_Nav_Data.Altitude_Ell;
			AverageAlt = TotalAlt/Count;
			Total_H_AccMaj += gn_Nav_Data.H_AccMaj;
			Average_H_AccMaj = Total_H_AccMaj/Count;
			Total_H_AccMin += gn_Nav_Data.H_AccMin;
			Average_H_AccMin = Total_H_AccMin/Count;
			Total_H_AccMajBrg += gn_Nav_Data.H_AccMajBrg;
			Average_H_AccMajBrg = Total_H_AccMajBrg/Count;
      }
   }

   /* if user desires to enter position themselves */
   if (gn_UserPos.UserPosEnabled)
   {
      /* enter user defined parameters */
      AGPS_Pos.Latitude = gn_UserPos.Latitude;
      AGPS_Pos.Longitude = gn_UserPos.Longitude;
      AGPS_Pos.Height = gn_UserPos.Altitude;
      /* now enter the constants */
      AGPS_Pos.Height_OK = TRUE;
      AGPS_Pos.RMS_SMaj = 30;
      AGPS_Pos.RMS_SMin = 30;
      AGPS_Pos.RMS_SMajBrg = 90;
      AGPS_Pos.RMS_Height = 50;

      /* Call function to set reference position */
      GN_AGPS_Set_Ref_Pos(&AGPS_Pos);

      /* set fixed position flag */
      FixedPosition = TRUE;
   }
   else /* survey and naturally get to fixed position mode */
   {   
	   if (((Count > 0) && (Count > (Average_H_AccMaj * 120.0)))
		   || ((Count > 120) && (Average_H_AccMaj < 100)))
      {
            // allow two minutes per meter of estimated accuracy for survey in
            // e.g. estimated accuracy = 10 meters average positon for 20 minutes
            // or allow for 2 minutes total if estimate accuracy within 100 meters
       

         if ((Count > Average_H_AccMaj * 120.0)) 
         {                    // && ( fabs(gn_Time_Data.Clock_Drift) < 1e-9)) {
	         FixedPosition = TRUE;

			   AGPS_Pos.Latitude =     AverageLat;                // WGS84 Geodetic Latitude  [degrees]
			   AGPS_Pos.Longitude =    AverageLon;                // WGS84 Geodetic Longitude [degrees]
			   AGPS_Pos.RMS_SMaj =     Average_H_AccMaj * 10.0;   // Horizontal RMS Accuracy, Semi-Major axis [metres]
			   AGPS_Pos.RMS_SMin =     Average_H_AccMin * 10.0;   // Horizontal RMS Accuracy, Semi-Minor axis [metres]
			   AGPS_Pos.RMS_SMajBrg =  Average_H_AccMajBrg;       // Horizontal RMS Accuracy, Semi-Major axis Bearing [deg]
			   AGPS_Pos.Height_OK =    TRUE;                      // Is the Height component available & OK to use ?
			   AGPS_Pos.Height =       AverageAlt;                // WGS84 Geodetic Ellipsoidal Height [metres]
			   AGPS_Pos.RMS_Height =   Average_H_AccMaj * 5.0;    // Height RMS Accuracy [metres]

            /* Call function to set reference position */
            GN_AGPS_Set_Ref_Pos(&AGPS_Pos);
         }
      }
   }
#endif
}
//****************************************************************************
//
// Function:      static void ClearMem(void *addr, CH c, int size)
//
// Inputs:        addr - address of memory to clear
//                c - character to write to memory
//                size - size of address buffer to clear
//
// Returns:       none

static void ClearMem(void *addr, CH c, int size)
{
   CH* memptr;
   int i;

   memptr = addr;

   for (i = 0; i < size; i++)
   {
      memptr[i] = c;
   }
}

#ifdef GN_NMEA_ENABLE
//*****************************************************************************
// GN_NMEA_Output_Task : The main function used when the GN NMEA Output Task
// is Running.  This controls the NMEA output to the NMEA named pipe in such a
// way that it does not block the main GPS Task if there is nothing reading from
// the NMEA named pipe.  This also needs to make sure that any data in the named
// pipe FIFO is fresh,  ie NMEA is not written there unless there is an active
// consumer.

void GN_NMEA_Output_Task( void )
{
   U1  nmea_buf[GN_NMEA_BUF_SIZE];        // Local copy of the GPS NMEA Buffer
   int nmea_buf_bytes;                    // Local copy of the GPS NMEA Buffer size
   int bytes_written;                     // Number of bytes written to the NMEA pipe

#ifdef GNS_DEBUG
   printf( "+GN_NMEA_Output_Task:\n" );
#endif

   gn_iPipe_NMEA = -1;

   // Aim to go round the main processing loop at an average rate of once
   // every 50 milliseconds, using variable Sleep intervals.

   while ( gn_NMEA_Output_Task_Exit == FALSE )
   {
      if ( gn_NMEA_buf_bytes > 0 )
      {
         errno = -99;
         // If the NMEA Output pipe is not open, then open it.
         if ( gn_iPipe_NMEA < 0 )
         {
            // On proper Linux the open() should block here until there is a
            // consumer connected to read() the output.
#ifdef GNS_DEBUG
            printf("NMEA PIPE = %s\r\n",gn_Pipe_NMEA);
#endif
            gn_iPipe_NMEA = open( gn_Pipe_NMEA, (O_WRONLY)); // | O_NONBLOCK | O_NOCTTY) );
#ifdef GNS_DEBUG
            printf("GN_NMEA_Output_Task: open() = %d, errno %d\n", gn_iPipe_NMEA, errno);
#endif

            // But on Cygwin the "open" does not block, but the "write" does so
            // write a dummy <CR><LF> block on, rather than NMEA data which will be old
            // by the time the user does connect and read it.
            if (gn_iPipe_NMEA > 0)
            {
               bytes_written = write( gn_iPipe_NMEA, "\x0D\x0A", (sizeof("\x0D\x0A")-1) );
            }
         }

         if ( gn_iPipe_NMEA > 0 )
         {
            // Take a local copy of the latest NMEA buffer published by the main GPS thread.
            // Clear the published buffer size to mark that it has been consumed.
            pthread_mutex_lock( &gn_NMEA_CritSec );
            {
               nmea_buf_bytes    = gn_NMEA_buf_bytes;
               memcpy( nmea_buf, gn_NMEA_buf, gn_NMEA_buf_bytes );
               nmea_buf[nmea_buf_bytes] = '\0';       // NULL terminate the buffer
               gn_NMEA_buf_bytes = 0;                 // Mark the source as consumed
            }
            pthread_mutex_unlock( &gn_NMEA_CritSec );
            // Prepare the GPINS message
			   nmea_buf_bytes += OutputGPINS(&nmea_buf[nmea_buf_bytes]);
            // Write the local NMEA buffer to the destination named pipe.
            bytes_written = 0;
#ifdef GN_EXTRA_STDOUT
            printf( " GN_NMEA_Output_Task:  +write( %d )  %d\n", gn_iPipe_NMEA, nmea_buf_bytes );
#endif
            bytes_written = write( gn_iPipe_NMEA, nmea_buf, nmea_buf_bytes );
            if ( bytes_written > 0 )
            {
#ifdef GN_EXTRA_STDOUT
               printf( " GN_NMEA_Output_Task:  -write( %d )  %d\n", gn_iPipe_NMEA, bytes_written );
#endif
               gn_NMEA_buf_bytes = 0;
            }
            else
            {
#ifdef GNS_DEBUG
               // Write failed, so assume that the user has disconnected.
               printf( " GN_NMEA_Output_Task:  write( %d ) failed,  %d  %d\n", gn_iPipe_NMEA, bytes_written, errno );
#endif

               // Close the NMEA output pipe, so that it will block on the next
               // open() until the user re-connects.
               close( gn_iPipe_NMEA );
#ifdef GNS_DEBUG
               printf( " GN_NMEA_Output_Task:  close( %d ) \n", gn_iPipe_NMEA );
#endif
               gn_iPipe_NMEA = -1;
            }
         }
      }

      // Sleep 50ms before going round this loop to check for more data.
      // Although we know it should not come for another second, this keeps the
      // latency low and allows the task to exit quickly.
      usleep( 50 * 1000 );                         // Sleep 50 ms

   };       // while ( gn_NMEA_Output_Task_Exit == FALSE )

#ifdef GNS_DEBUG
   printf( "-GN_NMEA_Output_Task:\n" );
#endif
   fflush( stdout );

   // Flag that the Thread is about to exit.
   gn_NMEA_Output_Task_Exited = TRUE;

   return;
}
#endif // #ifdef GN_NMEA_ENABLE

#ifdef GNS_LOG_ENABLE
//*****************************************************************************
// GN_Log:  Sends <stdio> formatted debug Log messages to the
// GN_GPS_Event_Log channel and appends a <CR><LF>.
// The total message length must be less than 250 characters long.

void GN_Log( const char *format, ... )
{
   char     buff[256];
   va_list  arg_list;
   int      i;

   // Add a header of "~ OS_Time_ms " to distinguish these Host Wrapper Event
   // Logs from the GN_GPS_Lib internal Event Logs.
   i = sprintf( buff, "~ %6u ", GN_GPS_Get_OS_Time_ms() );

   // Print the user string.
   va_start( arg_list, format );
      i = i + vsnprintf( (buff+i), (sizeof(buff)-4-i), format, arg_list );
   va_end( arg_list );

   // NULL terminate and print to the console.
   buff[i]   = '\0';
   printf( "%s\n", buff );

   // Append a <CR><LF> & NULL Terminate
   buff[i]   = 0x0D;
   buff[i+1] = 0x0A;
   buff[i+2] = '\0';

   // Send the String to the GN GPS Event Log channel.
   GN_GPS_Write_Event_Log( (U2)(i+1), (CH*)buff );

   return;
}
#endif

#if 0
//******************************************************************************
// GN_Clear_NV_Store:  Clear Specific items from the GN GPS NV_Store file at
// start up.  The items are identified via a character string as in the following
// examples:
//    "COLD"   :  Revert to Cold Start initial conditions
//    "WARM"   :  Revert to Warm Start initial conditions
//    "E"      :  Clear 'E'phemeris data for all satellites
//    "A"      :  Clear 'A'manac data for all satellites
//    "P"      :  Clear 'P'osition information
//    "T"      :  Clear 'T'ime information (this also clears the RTC calibration)
//    "U"      :  Clear 'U'TC Model parameters
//    "I"      :  Clear 'I'onospheric Model parameters
//    "F"      :  Clear GPS Reference TCXO 'F'requency calibration
//    "TPE"    :  Clear 'T'ime, 'P'osition & 'E'phemeris
//    "EPF"    :  Clear 'E'phemeris, 'P'osition & TCXO 'F'requency calibration
//    etc

static void GN_Clear_NV_Store(                // Clear Specified NV Store Items
   CH Items[16] )                   // i  - List of Items to Delete
{
   U1 nv_store[5*1024];             // Local temporary NV Store buffer
   U1 *nv_ptr;                      // Pointer to the library NV Store data
   U2 nv_size_true;                 // True size of the NV Store [bytes]
   U2 nv_size_read;                 // Size of the NV Store read back [bytes]

   if (Items[0] != '\0')          // Are there any items listed to delete ?
   {
      // Get the true size of the NV Store.
      // Note that before GN_GPS_Initialise() is called nv_ptr is not a valid ptr.
      nv_size_true = GN_GPS_Get_NV_Store(&nv_ptr);
      // Read the NV Store file
      nv_size_read = GN_GPS_Read_NV_Store(nv_size_true, nv_store);
      // If the read NV Store file is the expected size, then delete the
      // specified items and write it back to file again.
      if (nv_size_read == nv_size_true)
      {
         GN_GPS_Clear_NV_Data(nv_store, Items);
         GN_GPS_Write_NV_Store(nv_size_read, nv_store);
      }
#ifdef GNS_DEBUG
      GN_Log("Clear_NV_Store:  %d  %d  %p", nv_size_true, nv_size_read, nv_ptr);
#endif
   }

   return;
}
#endif

#ifdef GN_NMEA_ENABLE
//****************************************************************************
//
// Function:      static void OutputGPINS( void )
// 
// Inputs:        Pointer to NMEA buffer
//
// Returns:       Number of bytes in message
//
// Description:   Outputs the $GPINS message
//
//****************************************************************************

static int OutputGPINS(U1 *NMEA_buffer)
{
   int length;
   char GPINS_buffer[180];

   char NS, EW, Fix;
   unsigned int deg,min,frac;
   static R8 GeoLLH[3];
   static I2 LossOfLockCount = 0;
   static R8 TotalAlt = 0.0;
   static R8 AverageAlt = 0.0;
   U2 Days;
   U2 Hours;
   U2 Minutes;
   U2 Seconds;
   U4 TOW;
   static I4 Count = 0;

	if(gn_Nav_Data.Valid_3D_Fix) {
		if((gn_Nav_Data.H_AccMaj < 200) && (gn_Nav_Data.H_AccMaj > 0)) {
			Count++;
			TotalAlt += gn_Nav_Data.Altitude_Ell ;
			AverageAlt = TotalAlt/Count;
			GeoLLH[0] = gn_Nav_Data.Latitude * PI / 180.0;
			GeoLLH[1] = gn_Nav_Data.Longitude * PI / 180.0;
			GeoLLH[2] = AverageAlt;
			Geo2ECEF(GeoLLH, &WGS84_Datum,ECEF);
		}
		LossOfLockCount = 0;
	}
	sprintf(GPINS_buffer,"$GPINS,");

	TOW = (long)rint(gn_Nav_Data.Gps_TOW);

	sprintf(&GPINS_buffer[7],"%06lu.00,", (long unsigned) TOW);
	length = strlen(GPINS_buffer);

	if(gn_Nav_Data.Latitude < 0.0) NS = 'S';
	else NS = 'N';

	Days = TOW/(24*3600);
	Hours =   (TOW - (Days * (24*3600)))/3600;
	Minutes = (TOW - ((Days * (24*3600)) + (Hours * 3600))) / 60;
	Seconds = (TOW - ((Days * (24*3600)) + (Hours * 3600) + (Minutes * 60)));

	sprintf(&GPINS_buffer[length],"%02d%02d%02d.000,",
		(short)Hours,(short)Minutes,(short)Seconds);
	length = strlen(GPINS_buffer);

	deg2ints(&gn_Nav_Data.Latitude,&deg,&min,&frac);

	sprintf(&GPINS_buffer[length],"%02d%02d.%04d,",deg,min,frac);
	length = strlen(GPINS_buffer);


	sprintf(&GPINS_buffer[length],"%c,",NS);
	length +=2;

	if(gn_Nav_Data.Longitude < 0.0) EW = 'W';
	else EW = 'E';

	deg2ints(&gn_Nav_Data.Longitude,&deg,&min,&frac);

	sprintf(&GPINS_buffer[length],"%03d%02d.%04d,",deg,min,frac);
	length = strlen(GPINS_buffer);

	sprintf(&GPINS_buffer[length],"%c,",EW);
	length +=2;

	sprintf(&GPINS_buffer[length],"%d,",(int)gn_Nav_Data.Altitude_MSL);
	length = strlen(GPINS_buffer);

	if(gn_Nav_Data.FixType > 0){
		Fix = 'A';
	}
	else {
		Fix = 'V';
	}

	sprintf(&GPINS_buffer[length],"%c,",Fix);
	length +=2;

	sprintf(&GPINS_buffer[length],"%d,",(short)gn_Nav_Data.Gps_WeekNo);
	length = strlen(GPINS_buffer);
	sprintf(&GPINS_buffer[length],"%d,",(short)(gn_Time_Data.Clock_Drift*(1e9)));
	length = strlen(GPINS_buffer);

	sprintf(&GPINS_buffer[length],"%d,",(short)(gn_Time_Data.T_AccEst*(1e9)));
	length = strlen(GPINS_buffer);

	sprintf(&GPINS_buffer[length],"%d,",(short)(gn_Time_Data.F_AccEst*(1e10)));
	length = strlen(GPINS_buffer);

	sprintf(&GPINS_buffer[length],"%d,",(short)(gn_Nav_Data.UTC_Correction));
	length = strlen(GPINS_buffer);

	if(gn_Nav_Data.H_AccMaj > 9999.0)
		gn_Nav_Data.H_AccMaj = 9999.0;
	sprintf(&GPINS_buffer[length],"%d",(short)gn_Nav_Data.H_AccMaj);
	length = strlen(GPINS_buffer);


   if (FixedPosition == TRUE) 
   {
      sprintf(&GPINS_buffer[length],",F");
   }
   else
   {
      sprintf(&GPINS_buffer[length],",S");
   }

	length = strlen(GPINS_buffer);

	Add_NMEA_Checksum(GPINS_buffer);
	length = strlen(GPINS_buffer);
   // Add to NMEA buffer
   memcpy( NMEA_buffer, GPINS_buffer, length );

	return (length);
}

//****************************************************************************
//
// Function:      static void Add_NMEA_Checksum (char *)
//
// Inputs:        pointer to NMEA string
//
// Returns:       None
//
//  Add_NMEA_checksum : Adds the NMEA checksum to the end of the input string,
//  and replaces all spaces with leading zeros because they are not allowed in NMEA.
//  The checksum is the 8-bit exclusive OR of all the characters
//  in the sentense, including ',' but not including the '$' and '*'
//  delimiters.  This function assumes that the input index points to the
//  $, and that the terminating '*' has not yet been added.
//  Adds a <CR><LF> to the end of the string ready for output.
//****************************************************************************

static void Add_NMEA_Checksum(char *index)
{
   unsigned char checksum;               		// computed value of checksum

   index++;								// ignore the $
   checksum = 0;
   while ( *index != 0 )
   {
      if ( *index == ' ' )
      {
         *index = '_';                 	// replace all spaces with zeros
      }
      checksum = checksum ^ *index; 	// exclusive OR
      index++;
   }
   sprintf( index,"*%02X\r\n", checksum & 0xff);
   return;

}

//****************************************************************************
//
// Function:      static void deg2ints(R8 *decimal,int *deg,int *min,int *frac)
//
// Inputs:        pointer to decimal (R8) lat or long
//
// Returns:       degrees minutes and fractions of a minute to 4 dec places


static void deg2ints(R8 *input,unsigned int *deg,unsigned int *min,unsigned int *frac)
{
	R8 decimal;

	decimal = *input;

   decimal = fabs(decimal);
   *deg =  (unsigned int)rint((double)decimal);
   decimal = (decimal - (R8)*deg) * 60.0;
   *min =  (unsigned int)rint((double)decimal);
   decimal = (decimal - (R8)*min) * 10000.0;
   *frac = (unsigned int)decimal;
}
#endif // #ifdef GN_NMEA_ENABLE

//*****************************************************************************
