
//****************************************************************************
// GPS IP Centre, ST-Ericsson (UK) Ltd.
// Copyright (c) 2009 ST-Ericsson (UK) Ltd.
// 15-16 Cottesbrooke Park, Heartlands Business Park, Daventry, NN11 8YL, UK.
// All rights reserved.
//
// Filename  GN_GPS_api_calls.c
//
// $Header: GNS/GN_GPS_api_calls.c 1.3 2011/05/04 11:08:16PDT Daniel Brown (dbrown) Exp  $
// $Locker:  $
//****************************************************************************


//****************************************************************************
//
//  This is example code to illustrate how the GN GPS High-Level software
//  can be integrated into the host platform. Note that, although this
//  constitutes a fully-working GPS receiver, it is simplified relative to
//  what would be expected in a real product. The emphasis is in trying to
//  provide clarity of understanding of what is going on in the software, and
//  how the various API function calls relate to each other, rather than in
//  providing a full optimised software implementation.
//
//  The functions in this file are those called by the GN_GPS_Library on the
//  host software to provide platform specific functionality that cannot be
//  achieved in a cross-platform library.
//
//  All these functions MUST be implemented in the host software to suit the
//  target platform and OS, even if only as a "{ return( 0 ); }" stub.
//
//****************************************************************************

#include "GN_GPS_Task.h"
#include "GPS/GPS.h"
#include "sc_api.h"
#include "DBG/DBG.h"

// Static Data definitions local to this module

/* These buffers are limited by the size of the buffer in debug_printf() */ 
#define NAV_DBG_BUFSIZE    DEBUG_BUFLEN

static char * GN_GPS_Memcpy (char *to, const char *from, int size);

//static s_RTC_Calib gn_RTC_Calib;          // RTC Calibration Data

//****************************************************************************
// GN GPS Callback Function to request that the host start uploading a Patch
// File to the GN GPS Baseband for the given ROM code version. The host must
// ensure that if the data in this file is split amongst several writes then
// this is done at a <CR><LF> boundary so as to avoid message corruption by the
// Ctrl Data also being sent to the Baseband.
// Returns the Patch Checksum, or zero if there is no Patch.

U2 GN_GPS_Write_GNB_Patch(
   U2 ROM_version,                  // i - Current GN Baseband ROM version
   U2 Patch_CkSum )                 // i - Current GN Baseband Reported Patch
{
   U2 ret_val;                      // return value

#ifdef GNS_DEBUG
   // Record some details in the event log - for diagnostics purposes only
   GN_Log( "GN_GPS_Write_GNB_Patch: ROM v%3d 0x%04X", ROM_version, Patch_CkSum );
#endif
   // Setup the patch code upload process
   ret_val = GN_Setup_GNB_Patch( ROM_version, Patch_CkSum );

   return( ret_val );
}


//*****************************************************************************
// GN GPS Callback Function to Read back the GPS Non-Volatile Store Data from
// the Host's chosen Non-Volatile Store media.
// Returns the number of bytes actually read back.  If this is not equal to
// 'NV_size' then it is assumed that the Non-Volatile Store Data read back is
// invalid and Time-To_First_fix will be significantly degraded.

U2 GN_GPS_Read_NV_Store(
   U2 NV_size,                      // i - Size of the NV Store Data [bytes]
   U1 *p_NV_Store )                 // i - Pointer to the NV Store Data
{
   return (0);
#if 0
   // In this example implementation the Non-Volatile Store data is stored in a
   // file on the local disk.
   // Note that, in a real product, it would probably not be desirable to
   // read data from a file at this point, because it would be relatively slow.
   // A better approach might be to read the file before the GPS was started,
   // and to store the data in memory.

   size_t num_read_rtc;                       // Number of bytes read
   size_t num_read_nv_store;                  // Number of bytes read

   // Firstly, this is a good time to recover the Real-Time Clock Calibration
   // Data from file to the global structure where it will be read from in
   // function GN_GPS_Read_UTC_Data().
   // Open the Real-Time Clock data file for reading, in binary mode.
   // Note that, if the file does not exist, it must not be created.

   // Execute SC API callback fuction to read data from GN Baseband port
   num_read_rtc = SC_GpsRead_RTC(sizeof(gn_RTC_Calib), (UINT8 *)&gn_RTC_Calib);

   // Check that the correct number of bytes were read
   if ( num_read_rtc != sizeof(s_RTC_Calib) )
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Read_NV_Store RTC Calib Data: WARNING: Only read %d bytes, expected %d bytes.",
               num_read_rtc, sizeof(s_RTC_Calib));
#endif
      memset( &gn_RTC_Calib, 0, sizeof(s_RTC_Calib) );   // Clear the RTC Calib data
      num_read_rtc = 0;
   }
   else
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Read_NV_Store: RTC Calibration data read OK." ) ;
#endif
   }

   // Open the Non-Volatile Store data file for reading, in binary mode.
   // Note that, if the file does not exist, it must not be created.

   // Execute SC API callback fuction to read data from GN Baseband port
   num_read_nv_store = SC_GpsRead_NV(NV_size, (UINT8 *) p_NV_Store);

   // Check that the correct number of bytes were read
   if ( num_read_nv_store != NV_size )
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Read_NV_Store: WARNING: Only read %d bytes, expected %d bytes.",
               num_read_nv_store, NV_size);
#endif
      memset( p_NV_Store, 0, NV_size );            // Clear NV Store data
      num_read_nv_store = 0;
   }
   else
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Read_NV_Store: NV Store data read OK, %d.", num_read_nv_store ) ;
#endif
   }

   // Return the size of NV Store data read back.
   return( (U2)num_read_nv_store );
#endif
}


//*****************************************************************************
// GN GPS Callback Function to Write the GPS Non-Volatile Store Data to the
// Host's chosen Non-Volatile Store media.
// Returns the number of bytes actually written.

void GN_GPS_Write_NV_Store(
   U2 NV_size,                      // i - Size of the NV Store Data [bytes]
   U1 *p_NV_Store )                 // i - Pointer to the NV Store Data
{
#if 0
   // In this example implementation, the non-volatile data is stored in a
   // file on the local disk.
   // Note that, in a real product, it would probably not be desirable to
   // write data to a file at this point, because it would be relatively slow.
   // A better approach might be to write the data to memory, and to output
   // this to the file after the GPS has been stopped.

   size_t num_write_rtc;                       // Number of bytes written
   size_t num_write_nv_store;                  // Number of bytes written

   // Open the non-volatile data file for writing, in binary mode.
   // If the file already exists it will be over-written, otherwise
   // it will be created.

   // Execute SC API callback fuction to read data from GN Baseband port
   num_write_nv_store = SC_GpsWrite_NV(NV_size, (UINT8 *)p_NV_Store);

   // Check that the correct number of bytes were written
   if ( num_write_nv_store != (size_t)NV_size )
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Write_NV_Store: WARNING: Only wrote %d of %d bytes!",
               num_write_nv_store, NV_size );
#endif
   }
   else
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Write_NV_Store: Data written OK, %d.", num_write_nv_store );
#endif
   }

   // Finally, save this is a good time to save the RTC Calibration Data from
   // the structure it was written to in GN_GPS_Write_UTC_Data() to file.
   // Open the non-volatile data file for writing, in binary mode.
   // If the file already exists it will be over-written, otherwise
   // it will be created.

   // Execute SC API callback fuction to read data from GN Baseband port
   num_write_rtc = SC_GpsWrite_RTC(sizeof(gn_RTC_Calib), (UINT8 *)&gn_RTC_Calib);

   // Check that the correct number of bytes were written
   if ( num_write_rtc != sizeof(s_RTC_Calib) )
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Write_NV_Store RTC Calib Data: WARNING: Only wrote %d of %d bytes.",
               num_write_rtc, sizeof(s_RTC_Calib) );
#endif
   }
   else
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Write_NV_Store: RTC Calibration Data written OK." );
#endif
   }

   return;
#endif
}


//*****************************************************************************

// GN GPS Callback Function to Get the current OS Time tick in integer
// millisecond units.  The returned 'OS_Time_ms' must be capable of going all
// the way up to and correctly wrapping a 32-bit unsigned integer boundary.

U4 GN_GPS_Get_OS_Time_ms( void )
{
   return(0);
#if 0
   struct tms  process_times;
   clock_t     pr_time;

   // Get the current process time.
   // Note that times() is used because clock() does not appear to work properly.
   pr_time = times( &process_times );

   // If the process time counts in 1ms units then return the value directly
   // as it correctly wraps every 2^32 milliseconds.
   if ( gn_CLK_TCK == 1000 )
   {
      return( (U4)pr_time );
   }

   // Otherwise, if the process time count 100 times per second, ie 10ms for 1 tick.
   else if ( gn_CLK_TCK == 100 )
   {
      static BL tick_count_set = FALSE;
      static U4 last_clk       = 0;
      static U4 OS_Time_ms     = 0;

      if ( ! tick_count_set )
      {
         last_clk       = (U4)pr_time;
         OS_Time_ms     = last_clk * 10;
         tick_count_set = TRUE;
      }
      else
      {
         U4 this_clk = (U4)pr_time;
         if ( this_clk >= last_clk )
         {
            OS_Time_ms = OS_Time_ms  +  ( this_clk - last_clk ) * 10;
         }
         else
         {
            OS_Time_ms = OS_Time_ms  + ( (0xFFFFFFFF - last_clk) + this_clk + 1 ) * 10;
         }
         last_clk = this_clk;
      }
      return( OS_Time_ms );
   }

   // Otherwise, if the process time count 10000 times per second, ie 10 in 1ms.
   else if ( gn_CLK_TCK == 10000 )
   {
      static BL tick_count_set = FALSE;
      static U4 last_clk       = 0;
      static U4 OS_Time_ms     = 0;

      if ( ! tick_count_set )
      {
         last_clk       = (U4)pr_time;
         OS_Time_ms     = last_clk / 10;
         tick_count_set = TRUE;
      }
      else
      {
         U4 this_clk = (U4)pr_time;
         if ( this_clk >= last_clk )
         {
            OS_Time_ms = OS_Time_ms  +  ( this_clk - last_clk ) / 10;
         }
         else
         {
            OS_Time_ms = OS_Time_ms  + ( (0xFFFFFFFF - last_clk) + this_clk + 1 ) / 10;
         }
         last_clk = this_clk;
      }
      return( OS_Time_ms );
   }

   // Otherwise, the process time counts in as yet unsupported units, so return 0.
   else
   {
      return( 0 );
   }
#endif
}


//*****************************************************************************
// GN GPS Callback Function to Get the OS Time tick, in integer millisecond
// units, corresponding to when a burst of Measurement data received from
// GPS Baseband started.  This helps in determining the system latency.
// If it is not possible to determine this value then, return( 0 );
// The returned 'OS_Time_ms' must be capable of going all the way up to and
// correctly wrapping a 32-bit unsigned integer boundary.

U4 GN_GPS_Get_Meas_OS_Time_ms( void )
{
   // This is not implemented.
   return( 0 );
}


//*****************************************************************************
// GN GPS Callback Function to Read back the current UTC Date & Time data
// (eg from the Host's Real-Time Clock).
// Returns TRUE if successful.

BL GN_GPS_Read_UTC(
   s_GN_GPS_UTC_Data *p_UTC )          // i - Pointer to the UTC Date & Time
{
   return (FALSE);
#if 0
   // In this PC Example implementation, RTC is calibrated by the GPS
   // That is, the offset between the GPS time and the PC's RTC time is
   // computed and stored in a structure (checksumed) which is then in-turn
   // stored in a file on the local disk when we save the NV_Store data.
   // When the GPS Library makes this function call (i.e. calls GN_GPS_Read_UTC),
   // we check the content of the calibration data against its expected checksum
   // and if it passes we apply the calibration.
   // If we find anything wrong with the calibration data we assume that
   // the RTC Time cannot be trusted and we return( FALSE ) to say that there
   // is no starting UTC Time available.
   //
   // Note that, if the error in the time returned by this function is
   // significantly greater than is indicated by the associated
   // accuracy estimate (i.e. 'Acc_Est' in the s_GN_GPS_UTC_Data structure)
   // then the performance of the GPS might be degraded..

   struct timeval tv_RTC;              // Local OS Real-Time-Clock Time in timeval format
   struct tm      tm_RTC;              // Local OS Real-Time-Clock Time in tm format
   U4             Curr_CTime;          // Current Time in 'C' Time units [seconds]
   I4             dT_s;                // A Time difference [seconds]
   I4             milliseconds;        // The milliseconds part as a signed value
   I4             acc_est_deg;         // Time Accuracy Estimate degredation [ms]
   I4             acc_est_now;         // Time Accuracy Estimate now [ms]
   I4             age;                 // Age since RTC Last Set/Calibrated
   U4             checksum;            // RTC Calibration File 32-bit checksum

   // Clear the return UTC time to a NULL state with the Estimated Accuracy
   // set very high (ie a NOT SET state).
   memset( p_UTC, 0, sizeof(s_GN_GPS_UTC_Data) );
   p_UTC->Acc_Est  =  0x7FFFFF00;

   // Get the current OS time [ms]
   p_UTC->OS_Time_ms = GN_GPS_Get_OS_Time_ms();

   // Get the current RTC Time via the OS System Time.
   // (This RTC time must always be UTC (ie no time-zone adjustments etc).
   // Read the actual host platform current SystemTime.
   if ( gettimeofday( &tv_RTC, NULL ) < 0 )
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Read_UTC: gettimeofday() failed" );
#endif
      return( FALSE );
   }
   if ( gmtime_r( &tv_RTC.tv_sec, &tm_RTC ) == NULL )
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Read_UTC: gmtime_r() failed." );
#endif
      return( FALSE );
   }

   p_UTC->Year         = (U2)tm_RTC.tm_year + 1900;
   p_UTC->Month        = (U2)tm_RTC.tm_mon + 1;
   p_UTC->Day          = (U2)tm_RTC.tm_mday;
   p_UTC->Hours        = (U2)tm_RTC.tm_hour;
   p_UTC->Minutes      = (U2)tm_RTC.tm_min;
   p_UTC->Seconds      = (U2)tm_RTC.tm_sec;
   p_UTC->Milliseconds = (U2)tv_RTC.tv_usec / 1000;

   // Most embedded RTC's do not have a millisecond resolution, therefore,
   // Set the Milliseconds to 500 so that the error is -499 to +499 ms.
   // p_UTC->Milliseconds = 500;

#ifdef GN_EXTRA_RTC_DEBUG
   GN_Log( "GN_GPS_Read_UTC: GetSystemTime: %d  %d-%d-%d  %d:%02d:%02d.%03d",
            p_UTC->OS_Time_ms, p_UTC->Day, p_UTC->Month, p_UTC->Year,
            p_UTC->Hours, p_UTC->Minutes, p_UTC->Seconds, p_UTC->Milliseconds );
#endif

   // Sensibility check the RTC Time, and QUIT now if it contains a year older
   // than 2001 (we must allow a few years margin because of old GPS Simulations).
   if ( p_UTC->Year < 2001 )
   {
#ifdef GNS_DEBUG
      // Real-Time Clock Time invlaid,  quit now without a UTC time.
      GN_Log( "GN_GPS_Read_UTC: Date before 2001 (%d-%d-%d), Assuming its invalid.",
              p_UTC->Day, p_UTC->Month, p_UTC->Year );
#endif
      return( FALSE );
   }

   // If there is no calibration data available, quit now without a valid starting UTC Time.
   if ( gn_RTC_Calib.CTime_Set == 0 )
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Read_UTC: No calibration data available" );
#endif
      return( FALSE );
   }

   // Convert to 'C' Time units [seconds]
   Curr_CTime = GN_YMDHMS_To_CTime( p_UTC->Year,
                                    p_UTC->Month,
                                    p_UTC->Day,
                                    p_UTC->Hours,
                                    p_UTC->Minutes,
                                    p_UTC->Seconds );
#ifdef GN_EXTRA_RTC_DEBUG
   GN_Log( "GN_GPS_Read_UTC: %d = GN_YMDHMS_To_CTime(%d-%d-%d  %d:%02d:%02d)",
            Curr_CTime, p_UTC->Day, p_UTC->Month, p_UTC->Year,
            p_UTC->Hours, p_UTC->Minutes, p_UTC->Seconds );
#endif

   // Check the RTC Calibrtation Data checksum to see if it the data is OK.
   checksum = (U4)( (U4)0x55555555              +
                    (U4)gn_RTC_Calib.CTime_Set  +
                    (U4)gn_RTC_Calib.Offset_s   +
                    (U4)gn_RTC_Calib.Offset_ms  +
                    (U4)gn_RTC_Calib.Acc_Est_Set );

   // If the checksum is NOT correct then Time cannot be trusted, so QUIT now
   // without a valid starting UTC Time.
   if ( checksum != gn_RTC_Calib.checksum  )
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Read_UTC: WARNING: RTC Calibration Checksum fail, 0x%08X != 0x%08X.",
              checksum, gn_RTC_Calib.checksum );
#endif
      return( FALSE );
   }

   // Apply the integer second part of the (UTC - RTC) calibration offset here.
   Curr_CTime = Curr_CTime  -  gn_RTC_Calib.Offset_s;                // {sec]

   // Apply the millisecond part of the calibration offset into the Milliseconds.
   // Use the signed local variable "milliseconds" because it can go negative.
   // If it does go negative, make it postive by moveing in 1 second from the integer part.
   milliseconds = (I4)p_UTC->Milliseconds  -  gn_RTC_Calib.Offset_ms;
   while ( milliseconds < 0 )
   {
      milliseconds = milliseconds + 1000;       // add 1 second to make positive
      Curr_CTime   = Curr_CTime   - 1;          // subtract 1 second to balance
   }
   p_UTC->Milliseconds  = (U2)milliseconds;

   // If the Milliseconds have gone over a second boundary, then move the integer
   // seconds part of it into the Current 'C' Time.
   if ( p_UTC->Milliseconds >= 1000 )
   {
      dT_s                 = p_UTC->Milliseconds / 1000;             // [sec]
      p_UTC->Milliseconds  = p_UTC->Milliseconds  - dT_s*1000;       // [ms]
      Curr_CTime           = Curr_CTime + dT_s;                      // {sec]
   }

   // Compute the age since the RTC was last calibrated.
   age = (I4)( Curr_CTime - gn_RTC_Calib.CTime_Set );             // seconds

   // If the age is slightly negative (eg -10 sec) then set it to zero in case
   // it was caused by rounding etc.
   if ( age < 0  &&  age > -10 )   age = 0;

   // If the age is still negative,  ie Time has gone backwards!
   // This probably means that the RTC has been reset, and therefore can't be
   // trusted, so QUIT now without a valid starting UTC Time.
   // This can also occur when working with GPS Simulators!
   if ( age < 0 )
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Read_UTC: ERROR: RTC time has gone backwards!, age = %d = (%d -%d)",
              age, Curr_CTime, gn_RTC_Calib.CTime_Set );
#endif
      return( FALSE );
   }

   // Compute the expected degradation in the accuracy of this RTC time,
   // allowing for degredations due to.
   // a) 170ms RMS for the +/- 0.5 sec error in reading the RTC.
   // b) 10 ppm RMS for the RTC crystal stability over the age DT.
   // Note that 'age' is in seconds, but RTC_acc and acc are ms.
   acc_est_deg =  150  +  age / (1000000/(10*1000));

   // Compute the Estimated time Accuracy now [RMS ms] , using the Gaussian
   // Propogation of Error Law to combine the estimated accuracy when set
   // and the estimated degredation since then.
   {
      R4 tempR4 =  (R4)gn_RTC_Calib.Acc_Est_Set * (R4)gn_RTC_Calib.Acc_Est_Set
                   +  (R4)acc_est_deg * (R4)acc_est_deg;
      acc_est_now  =  (U4)sqrt( (R8)tempR4 );
   }
   p_UTC->Acc_Est  =  acc_est_now;

   // Limit the accuracy to 300 ms RMS (i.e. a worst-case error of about 1 sec).
   // We do this because the resolution of a Typical OS appears to be 1 seconds.
   if ( p_UTC->Acc_Est < 300 )  p_UTC->Acc_Est = 300;                // [ms]

   // Convert the Current 'C' Time back to a YY-MM-DD hh:mm:ss date & time format.
   GN_CTime_To_YMDHMS( Curr_CTime,
                       &p_UTC->Year,  &p_UTC->Month,   &p_UTC->Day,
                       &p_UTC->Hours, &p_UTC->Minutes, &p_UTC->Seconds );

#ifdef GN_EXTRA_RTC_DEBUG
   GN_Log( "GN_GPS_Read_UTC: %d  %d-%d-%d  %d:%02d:%02d.%03d  RMS=%d  (%d  %d  %d  %d)  %d  %d  %d",
            p_UTC->OS_Time_ms, p_UTC->Day, p_UTC->Month, p_UTC->Year,
            p_UTC->Hours, p_UTC->Minutes, p_UTC->Seconds, p_UTC->Milliseconds,
            p_UTC->Acc_Est,  gn_RTC_Calib.CTime_Set, gn_RTC_Calib.Offset_s,
            gn_RTC_Calib.Offset_ms, gn_RTC_Calib.Acc_Est_Set,
            age, acc_est_deg, acc_est_now );
#endif

   return( TRUE );
#endif
}


//*****************************************************************************
// GN GPS Callback Function to Write UTC Date & Time data to the Host platform
// software area,  which can be used by the Host to update its Real-Time Clock.

void GN_GPS_Write_UTC(
   s_GN_GPS_UTC_Data *p_UTC )       // i - Pointer to the UTC Date & Time
{
#if 0
   static U4 counter = 0;

   // Compute the difference between the time from the PC's RTC and that
   // provided by the GPS.  This difference will be saved as a Calibration
   // Offset to a structure (checksumed), which is then written to a data file
   // on the local disk when we next save the NV Store data.
   // On the next start-up this calibration data will be read back and applied
   // to the Time read back from the RTC.
   //
   // When the GPS is generating position fixes, this function will be called
   // once per update.
   //
   // It may also be necessary to actually set the RTC if the Calibration Offset
   // is too big in order to prevent other applications on the Host system
   // from doing so and hence causing our calibration offset to be wrong.
   // We do not want to do this in PC Example because an incorrect time generated
   // when using a GPS Simulator will clash with the local time calibrated by a
   // Network Time Server, cause source file time-tags to be set in-correctly,
   // thus upsetting make systems, etc, etc

   struct timeval tv_RTC;              // Local OS Real-Time-Clock Time in timeval format
   struct tm      tm_RTC;              // Local OS Real-Time-Clock Time in tm format
   U4             RTC_CTime;           // Real-Time Clock Time, in 'C' Time units [seconds]
   U4             RTC_OS_Time_ms;      // OS Time [ms] corresponding to the RTC Time
   I4             dT_s;                // A Time difference [seconds]
   U4             New_CTime;           // New UTC Time, in 'C' Time units [seconds]
   BL             RTC_Just_Set;        // Was the RTC Just Set with a New time ?

#ifdef GN_EXTRA_RTC_DEBUG
   GN_Log( "GN_GPS_Write_UTC: Given  %d  %d-%d-%d  %d:%02d:%02d.%03d  RMS=%d Curr Cal=%d count=%d",
            p_UTC->OS_Time_ms, p_UTC->Day, p_UTC->Month, p_UTC->Year,
            p_UTC->Hours, p_UTC->Minutes, p_UTC->Seconds, p_UTC->Milliseconds,
            p_UTC->Acc_Est, gn_RTC_Calib.Acc_Est_Set, counter );
#endif

   // Check for the case of being given time of all zero's with a very large
   // Accuracy Estimate and treat this as a request to Clear Time, which is
   // achieved simply by deleting the UTC data file.
   if ( p_UTC->Acc_Est > 0x7FFF0000  &&
        p_UTC->Year  == 0  &&  p_UTC->Month   == 0  &&  p_UTC->Day     == 0  &&
        p_UTC->Hours == 0  &&  p_UTC->Minutes == 0  &&  p_UTC->Seconds == 0    )
   {
#ifdef GNS_DEBUG
      GN_Log( "GN_GPS_Write_UTC: Request to delete UTC Time" );
#endif
      memset( &gn_RTC_Calib, 0, sizeof(gn_RTC_Calib) );
      return;
   }

   // If the new time is worse than the currently saved then consider ignoring it.
   // The worse it is the longer it is ignored in the hope something better comes along.

   if ( gn_RTC_Calib.checksum > 0  &&  p_UTC->Acc_Est >= gn_RTC_Calib.Acc_Est_Set )
   {
      counter++;
      if ( p_UTC->Acc_Est > 1000  &&  counter < ( 20 * 60 ) )  return;
      if ( p_UTC->Acc_Est > 300   &&  counter < ( 10 * 60 ) )  return;
      if ( p_UTC->Acc_Est > 100   &&  counter < (  5 * 60 ) )  return;
      if ( p_UTC->Acc_Est > 30    &&  counter < (  3 * 60 ) )  return;
      if ( p_UTC->Acc_Est > 10    &&  counter < (  2 * 60 ) )  return;
      if (                            counter < (  1 * 60 ) )  return;
   }
   counter = 0;

   // Only Calibrate the RTC if we have been given time to better than
   // 300ms RMS, ie < 1 sec MAX,  ie it is good enough to be reliable.
   if ( p_UTC->Acc_Est < 300 )
   {
      // Convert the New UTC Time to 'C' Time units [seconds]
      New_CTime = GN_YMDHMS_To_CTime( p_UTC->Year,
                                      p_UTC->Month,
                                      p_UTC->Day,
                                      p_UTC->Hours,
                                      p_UTC->Minutes,
                                      p_UTC->Seconds );

      do {
         RTC_Just_Set = FALSE;

         // Get the current RTC Time via the Win OS SYSTEMTIME time,
         // (This RTC time must always be UTC (ie no time-zone adjustments etc).
         // Also get the equivalent OS Time.
         // GetSystemTime( &RTC );
         RTC_OS_Time_ms = GN_GPS_Get_OS_Time_ms();
         gettimeofday( &tv_RTC, NULL );
         if ( gettimeofday( &tv_RTC, NULL ) < 0 )
         {
#ifdef GNS_DEBUG
            GN_Log( "GN_GPS_Write_UTC:  gettimeofday() failed." );
#endif
            return;
         }
         if ( gmtime_r( &tv_RTC.tv_sec, &tm_RTC ) == NULL )
         {
#ifdef GNS_DEBUG
            GN_Log( "GN_GPS_Write_UTC:  gmtime_r() failed." );
#endif
            return;
         }

         // Convert the current RTC Time to 'C' Time units.
         RTC_CTime = GN_YMDHMS_To_CTime( (U2)tm_RTC.tm_year + 1900,
                                         (U2)tm_RTC.tm_mon + 1,
                                         (U2)tm_RTC.tm_mday,
                                         (U2)tm_RTC.tm_hour,
                                         (U2)tm_RTC.tm_min,
                                         (U2)tm_RTC.tm_sec );

#ifdef GN_EXTRA_RTC_DEBUG
         GN_Log( "GN_GPS_Read_UTC:  RTC Now  %d  %d-%d-%d  %d:%02d:%02d",
                  RTC_CTime, tm_RTC.tm_mday, tm_RTC.tm_mon+1, tm_RTC.tm_year+1900,
                  tm_RTC.tm_hour, tm_RTC.tm_min, tm_RTC.tm_sec );
#endif

         // Compute the difference between the Current RTC Time the New UTC Time
         dT_s = (I4)( RTC_CTime - New_CTime );        // [seconds]

         // If the difference is too much for the target system, then update the RTC.
         // TODO:  Adjust this test limit to suit the target platform & environment.
//       if ( abs(dT_s) > 10 )                        // > 10 seconds
         if ( abs(dT_s) > 60 )                        // >  1 minute
//       if ( abs(dT_s) > (10*365*24*60*60) )         // > 10 years
//       if ( FALSE )                                 // Never
         {
            // Big offset, so set the RTC to integer second resolution
            tm_RTC.tm_mday        =  (int)p_UTC->Day;
            tm_RTC.tm_mon         =  (int)p_UTC->Month - 1;
            tm_RTC.tm_year        =  (int)p_UTC->Year - 1900;
            tm_RTC.tm_hour        =  (int)p_UTC->Hours;
            tm_RTC.tm_min         =  (int)p_UTC->Minutes ;
            tm_RTC.tm_sec         =  (int)p_UTC->Seconds;
            tv_RTC.tv_sec  = timegm( &tm_RTC );
            tv_RTC.tv_usec = p_UTC->Milliseconds * 1000;
            if ( settimeofday( &tv_RTC, NULL ) == 0 )
            {
               RTC_Just_Set = TRUE;
#ifdef GNS_DEBUG
               GN_Log( "GN_GPS_Read_UTC:  RTC Set: %d-%d-%d  %d:%02d:%02d",
                       p_UTC->Day, p_UTC->Month, p_UTC->Year,
                       p_UTC->Hours, p_UTC->Minutes, p_UTC->Seconds );
#endif
            }
         }

         // Repeat the above if we decided to Set the RTC, so that the Calibration
         // difference will be re-computed.
      }  while( RTC_Just_Set == TRUE );

      // Save the RTC Calibration data to the global structure

      // a) When this Calibrtation was done
      gn_RTC_Calib.CTime_Set = New_CTime;

      // b) The bigger integer second part of the Calibration Offset
      gn_RTC_Calib.Offset_s  = dT_s;

      // c) The smaller millisecond part of the Calibration Offset based on:
      //    1) The +/- 0.5sec rounding in reading the RTC
      //    2) Milliseconds bit of the input UTC Time which was not used
      //    3) The latency between when the UTC Time was computed for and when
      //       we read-back the RTC to compute the calibration.
      gn_RTC_Calib.Offset_ms = ( 500 - p_UTC->Milliseconds )  +
                               ( RTC_OS_Time_ms - p_UTC->OS_Time_ms );

      // d) The Estimated accuracy of the RTC Calibration.  Assume that we can:
      //    1) Keep the GN_GPS_Update loop latency to within 100ms, say 30ms RMS,
      //    2) Calibration is good to +/- 0.5 second,  ie 170ms RMS
      gn_RTC_Calib.Acc_Est_Set =  p_UTC->Acc_Est + 30 + 150;

      // e) Compute the new checksum for the Calibration data structure
      gn_RTC_Calib.checksum =  (U4)( (U4)0x55555555              +
                                     (U4)gn_RTC_Calib.CTime_Set  +
                                     (U4)gn_RTC_Calib.Offset_s   +
                                     (U4)gn_RTC_Calib.Offset_ms  +
                                     (U4)gn_RTC_Calib.Acc_Est_Set  );

#ifdef GN_EXTRA_RTC_DEBUG
      GN_Log( "GN_GPS_Write_UTC: RTC Calibration   %d  %d s  %d ms  RMS=%d  0x%08X",
      gn_RTC_Calib.CTime_Set, gn_RTC_Calib.Offset_s, gn_RTC_Calib.Offset_ms,
      gn_RTC_Calib.Acc_Est_Set, gn_RTC_Calib.checksum );
#endif
   }

   return;
#endif
}

//*****************************************************************************
// GN GPS Callback Function to Read GPS Measurement Data from the Host's
// chosen GPS Baseband communications interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, 'max_bytes' is dynamically set to prevent a single Read
// operation from straddling the internal circular buffer's end wrap point, or
// from over writing data that has not been processed yet.
// Returns the number of bytes actually read.  If this is equal to 'max_bytes'
// then this callback function may be called again if 'max_bytes' was limited
// due to the circular buffer's end wrap point.

U2 GN_GPS_Read_GNB_Meas(
   U2  max_bytes,                   // i - Maximum number of bytes to read
   CH *p_GNB_Meas )                 // i - Pointer to the Measurement data.
{
   int bytes_read;                  // Number of bytes actually read

   // Execute SC API callback fuction to read data from GN Baseband port
   bytes_read = SC_GpsRxData( max_bytes, (UINT8 *)p_GNB_Meas);
   if ( bytes_read < 0 )
   {
      bytes_read = 0;
   }

   return((U2)bytes_read);
}


//*****************************************************************************
// GN GPS Callback Function to Write GPS Control Data to the Host's chosen
// GPS Baseband communications interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, this callback function may be called twice if the data to
// be written straddles the internal circular buffer's end wrap point.
// Returns the number of bytes actually written.  If this is less than the
// number of bytes requested to be written, then it is assumed that this is
// because the host side cannot physically handle any more data at this time.

U2 GN_GPS_Write_GNB_Ctrl(
   U2  num_bytes,                   // i - Available number of bytes to Write
   CH *p_GNB_Ctrl )                 // i - Pointer to the Ctrl data.
{
   int bytes_written;               // Number of bytes written

   // Execute SC API callback fuction to write data to GN Baseband port
   bytes_written = SC_GpsTxData( num_bytes, (UINT8 *)p_GNB_Ctrl);
   if ( bytes_written < 0 )
   {
      bytes_written = 0;
   }

   return((U2)bytes_written);
}

//*****************************************************************************
// GN GPS Callback Function to request that if possible the host should
// perform a Hard Power-Down Reset of the GN Baseband Chips.
// Returns TRUE is this is possible.

BL GN_GPS_Hard_Reset_GNB( void )
{
   // Call the GN GNS???? Chip/Module Hardware Abstraction Layer Function,
   // which will return TRUE if "Reset" Control is implemented.

   return (GN_GNS_HAL_Reset());
}


//*****************************************************************************
// GN GPS Callback Function to Write GPS NMEA 183 Output Sentences to the
// the Host's chosen NMEA interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, this callback function may be called twice if the data to
// be written straddles the internal circular buffer's end wrap point.
// Returns the number of bytes actually written.  If this is less than the
// number of bytes requested to be written, then it is assumed that this is
// because the host side cannot physically handle any more data at this time.

U2 GN_GPS_Write_NMEA(
   U2  num_bytes,                   // i - Available number of bytes to Write
   CH *p_NMEA )                     // i - Pointer to the NMEA data.
{
   // Write the data to the NMEA log file
   U2 bytes_written;                // Number of bytes written

#if (GN_NMEA_ENABLE && GN_NMEA_FILE_ENABLE)
   // If the NMEA debug log file is open, write the NMEA data there.
   if ( gn_fp_NMEA != NULL )
   {
      bytes_written = (U2)fwrite( p_NMEA, (int)num_bytes, 1, gn_fp_NMEA );
   }
#else
   bytes_written = num_bytes;
#endif

#ifdef GN_NMEA_ENABLE
   // Copy the NMEA data to the NMEA Output Buffer where GN_NMEA_Output_Task()
   // will pick it up and output it to the user application.
   // This buffer is ASCII NULL terminated.
   // TODO:  This may need changing to suit how the target GPS application
   //        expects to get its GPS data.
   pthread_mutex_lock( &gn_NMEA_CritSec );
   {
      gn_NMEA_buf_bytes      = 0;            // Flag no data available during the update
      memcpy( gn_NMEA_buf, p_NMEA, num_bytes );
      gn_NMEA_buf[num_bytes] = '\0';         // NULL terminate the buffer
      gn_NMEA_buf_bytes      = num_bytes;    // Flag number of bytes NMEA now available
   }
   pthread_mutex_unlock( &gn_NMEA_CritSec );

   // State that this function has consumed all the NMEA data to prevent the
   // GN_GPS_Lib presenting it again later.
   bytes_written = num_bytes;
#else
   bytes_written = 0;
#endif

   return( bytes_written );
}


//*****************************************************************************
// GN GPS Callback Function to Read $PGNV GN GPS Propriatary NMEA Input
// Messages from the Host's chosen $PGNV communications interface.
// Internally the GN GPS Core library a circular buffer to store this data.
// Therefore, 'max_bytes' is dynamically set to prevent a single Read operation
// from straddling the internal circular buffer's end wrap point, or from
// over-writing data that has not yet been processed.
// Returns the number of bytes actually read.  If this is equal to 'max_bytes'
// then this callback function may be called again if 'max_bytes' was limited
// due to the circular buffer's end wrap point.

U2 GN_GPS_Read_PGNV(
   U2  max_bytes,                   // i - Maximum number of bytes to read
   CH *p_PGNV )                     // i - Pointer to the $PGNV data.
{
   // This function will be called by the GPS Core only if the host has
   // called the GN_GPS_Parse_PGNV() function.
   // This is only necessary on host platforms that require the ability to
   // trigger some GN GPS API functionality via an input NMEA message rather
   // than by directly calling the API function.  Usually, this would only be
   // necessary or desirable to test purposes.

   /* Not using input PGNV messaging, return 0 */
   return( (U2)0 );
}


//*****************************************************************************
// Debug Callback Functions called by the GN GPS High Level Software library
// that need to be implemented by the Host platform software to capture debug
// data to an appropriate interface (eg UART, File, both etc).
// GN GPS Callback Function to Write GPS Baseband I/O communications Debug data
// to the the Host's chosen debug interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, this callback function may be called twice if the data to
// be written straddles the internal circular buffer's end wrap point.
// Returns the number of bytes actually written.  If this is less than the
// number of bytes requested to be written, then it is assumed that this is
// because the host side cannot physically handle any more data at this time.

U2 GN_GPS_Write_GNB_Debug(
   U2  num_bytes,                   // i - Available number of bytes to Write
   CH *p_GNB_Debug )                // i - Pointer to the GNB Debug data.
{
   /* Don't write any data for now, return 0 bytes to the GNS core library */
   return(0);
}


//*****************************************************************************
// GN GPS Callback Function to Write GPS Navigation Solution Debug data to the
// Host's chosen debug interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, this callback function may be called twice if the data to
// be written straddles the internal circular buffer's end wrap point.
// Returns the number of bytes actually written.  If this is less than the
// number of bytes requested to be written, then it is assumed that this is
// because the host side cannot physically handle any more data at this time.

U2 GN_GPS_Write_Nav_Debug(
   U2  num_bytes,                   // i - Available number of bytes to Write
   CH *p_Nav_Debug )                // i - Pointer to the Nav Debug data.
{
   CH Nav_buf[NAV_DBG_BUFSIZE];
   U2 num_written;
   U2 remainder;

   /* if number of bytes is less than buffer size (leave extra byte for null) */
   if (num_bytes < (NAV_DBG_BUFSIZE-1))
   {
      /* copy bytes */
      GN_GPS_Memcpy(Nav_buf, p_Nav_Debug, num_bytes);
      /* add null termination */
      Nav_buf[num_bytes] = (CH) NULL;
      /* save number of bytes written */
      num_written = num_bytes;
   
      /* write to SoftClock GPS debug port */
      debug_printf(GPS_NAV_DEBUG_PRT,"%s",Nav_buf);
   }
   else
   {
      /* initialize num_written */
      num_written = 0;
      /* iterate until all bytes have been written */
      while (num_written < num_bytes)
      {
         /* calculate remaining bytes to be written */
         remainder = num_bytes - num_written;
         /* if remaining bytes is less than buffer size (leave extra byte for null) */
         if (remainder < (NAV_DBG_BUFSIZE-1))
         {
            /* copy bytes */
            GN_GPS_Memcpy(Nav_buf, &p_Nav_Debug[num_written], remainder);
            /* add null termination */
            Nav_buf[remainder] = (CH) NULL;
            /* save number of bytes written */
            num_written += remainder;
         }
         else
         {
            /* copy bytes */
            GN_GPS_Memcpy(Nav_buf, &p_Nav_Debug[num_written], NAV_DBG_BUFSIZE);
            /* add null termination */
            Nav_buf[NAV_DBG_BUFSIZE] = (CH) NULL;
            /* save number of bytes written */
            num_written += NAV_DBG_BUFSIZE;
         }
   
         /* write to SoftClock GPS debug port */
         debug_printf(GPS_NAV_DEBUG_PRT,"%s",Nav_buf);
      }
   }

   return(num_written);
}


//*****************************************************************************
// GN GPS Callback Function to Write GPS Navigation Library Event Log data
// to the Host's chosen debug interface.
// Internally the GN GPS Core library uses a circular buffer to store this
// data.  Therefore, this callback function may be called twice if the data to
// be written straddles the internal circular buffer's end wrap point.
// Returns the number of bytes actually written.  If this is less than the
// number of bytes requested to be written, then it is assumed that this is
// because the host side cannot physically handle any more data at this time.

U2 GN_GPS_Write_Event_Log(
   U2  num_bytes,                   // i - Available number of bytes to Write
   CH *p_Event_Log )                // i - Pointer to the Event Log data.
{
   /* Don't write any data for now, return 0 bytes to the GNS core library */
   return(0);
}

/***********************************************************************
** 
** Function    : GN_GPS_Memcpy
**
** Description : Copies size bytes from the object beginning at from <from> into
**               the object beginning at to <to>.
**
** Parameters  : - to - buffer to copy string to
**                 from - buffer to copy string from
**                 size - number of bytes to copy 
**
** Returnvalue : - <to>
**
** Remarks     : -
**
***********************************************************************/
static char * GN_GPS_Memcpy (char *to, const char *from, int size)
{
   int i = 0;

   /* iterate over from string */
   while(i < size)
   {
      /* copy character */
      to[i] = from[i]; 
      /* increment iterator */
      i++;
   }

   return(to);
}

//*****************************************************************************


