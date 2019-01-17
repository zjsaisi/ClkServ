
//*****************************************************************************
// GPS IP Centre, ST-Ericsson (UK) Ltd.
// Copyright (c) 2009 ST-Ericsson (UK) Ltd.
// 15-16 Cottesbrooke Park, Heartlands Business Park, Daventry, NN11 8YL, UK.
// All rights reserved.
//
// Filename  GN_GPS_Time.c
//
// $Header: GNS/GN_GPS_Time.c 1.1 2011/01/07 13:31:03PST Daniel Brown (dbrown) Exp  $
// $Locker:  $
//*****************************************************************************

#include "GN_GPS_Task.h"

//*****************************************************************************
//
//  GN_GPS_Time :  Time Conversion Utilities
//
//*****************************************************************************


//*****************************************************************************
// Static constant tables local to this file

// Difference [seconds] between the GPS epoch (06 Jan 1980) and the 'C' epoch
// (01 Jan 1970) is:
//    ( 10 years + 2 leap year days + 5 days in Jan ) * seconds in a day
static const U4 Diff_GPS_C_Time = ( (365*10 + 2 + 5) * (24*60*60) );

// An array containing the Number of day in the year up to the of last day of
// the previous month in a non-leap year.

static const U2 Days_to_Month[12] =
      { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };


// Number of Days in a Month look up table.
static const U1 Days_in_Month[12] =
{
   31,   // Jan
   28,   // Feb
   31,   // Mar
   30,   // Apr
   31,   // May
   30,   // Jun
   31,   // Jul
   31,   // Aug
   30,   // Sep
   31,   // Oct
   30,   // Nov
   31    // Dec
};


//*****************************************************************************
// Convert Time in YY-MM-DD & HH:MM:SS format to 'C' Time units [seconds].
// ('C' Time starts on the 1st Jan 1970.)

U4 GN_YMDHMS_To_CTime(
   U2 Year,                   // i  - Year         [eg 2006]
   U2 Month,                  // i  - Month        [range 1..12]
   U2 Day,                    // i  - Days         [range 1..31]
   U2 Hours,                  // i  - Hours        [range 0..23]
   U2 Minutes,                // i  - Minutes      [range 0..59]
   U2 Seconds )               // i  - Seconds      [range 0..59]
 {
   // Return Value Definition
   U4 CTime;                  // Time in 'C' Time units [seconds]
   // Local Definitions
   I4 GPS_secs;               // GPS Seconds since 5/6 Jan 1980.
   I4 dayOfYear;              // Completed days into current year
   I4 noYears;                // Completed years since 1900
   I4 noLeapYears;            // Number of leap years between 1900 and present
   I4 noDays;                 // Number of days since midnight, Dec 31/Jan 1, 1900.
   I4 noGPSdays;              // Number of days since start of GPS time

   // Compute the day number into the year
   dayOfYear = (I4)( Days_to_Month[Month-1] + Day );

   // Leap year check.  (Note that this algorithm fails at 2100!)
   if ( Month > 2  &&  (Year%4) == 0 )  dayOfYear++;

   // The number of days between midnight, Dec31/Jan 1, 1900 and
   // midnight, Jan5/6, 1980 (ie start of GPS) is 28860.
   noYears     = (I4)Year - 1901;
   noLeapYears = noYears / 4;
   noDays      = (noYears*365) + noLeapYears + dayOfYear;
   noGPSdays   = noDays - 28860;                    // Number of GPS days
   GPS_secs    = noGPSdays*86400 + Hours*3600 + Minutes*60  + Seconds;

   // Convert from GPS Time to C Time
   CTime = GPS_secs + Diff_GPS_C_Time;

   return( CTime );
}


//*****************************************************************************
// Convert Time in 'C' Time units [seconds] to a YY-MM-DD & HH:MM:SS format.
// ('C' Time starts on the 1st Jan 1970.)

void GN_CTime_To_YMDHMS(
   U4 C_Time,                 // i  - Time in 'C' Time units [seconds]
   U2 *Year,                  //  o - Year         [eg 2006]
   U2 *Month,                 //  o - Month        [range 1..12]
   U2 *Day,                   //  o - Days         [range 1..31]
   U2 *Hours,                 //  o - Hours        [range 0..23]
   U2 *Minutes,               //  o - Minutes      [range 0..59]
   U2 *Seconds )              //  o - Seconds      [range 0..59]
 {
   I4 GPS_secs;               // GPS Seconds since 5/6 Jan 1980.
   I2 gpsWeekNo;              // GPS Week Number
   I4 gpsTOW;                 // GPS Time of Week [seconds]
   I4 loSecOfD;               // Local Second of Day    (range 0-86399)
   I4 loYear;                 // Local Year             (range 1980-2100)
   I4 loDayOfW;               // Local Day of Week      (range 1-7)
   I4 loDayOfY;               // Local Day of Year      (range 1-366)
   I4 loSecOfH;               // Local Second of Hour   (range 0-3599)
   I4 i;                      // Loop index
   I4 tempI4;                 // Temporary I4 value

   // Convert from 'C' Time to GPS Time.
   GPS_secs = C_Time - Diff_GPS_C_Time;

   // Convert UTC Time of Week to Day of Week, Hours, Minutes and Seconds.
   gpsWeekNo = (I2)( GPS_secs /604800 );
   gpsTOW    = GPS_secs - 604800*gpsWeekNo;

   loDayOfW  = gpsTOW / 86400;            // Calculate completed Days into Week
   loSecOfD  = gpsTOW - 86400*loDayOfW;   // Calculate current Second of Day
   tempI4    = loSecOfD / 3600;           // Calculate current Hour of Day
   *Hours    = (U2)tempI4;                // Store current Hour of Day
   loSecOfH  = loSecOfD - 3600*tempI4;    // Calculate current Second of Hour
   tempI4    = loSecOfH / 60;             // Calculate current Minute of Hour
   *Minutes  = (U2)tempI4;                // Store current Minute of Hours
   *Seconds  = (U2)(loSecOfH - 60*tempI4);// Calc & Store current Minute of Second

   // Convert day of week and week number to day of year (tempI4) and year
   tempI4  = loDayOfW + (I4)gpsWeekNo*7;  // Calculate GPS day number
   tempI4  = tempI4 + 6;                  // Offset for start of GPS time 6/1/1980
   loYear  = 1980;

   // Advance completed 4 years periods,  which includes one leap year.
   // (Note that this algorithm fails at 2100, which is not a leap year.)
   while ( tempI4 > ((365*4) + 1) )
   {
      tempI4 = tempI4 - ((365*4) + 1);
      loYear = loYear + 4;
   };
   // Advance remaining completed years, which don't include any leap years.
   // (Note that this algorithm fails at 2100, which is not a leap year.)
   while ( tempI4 > 366 )
   {
      tempI4 = tempI4 - 365;
      if ( (loYear & 0x3) == 0 ) tempI4--;   // TRUE for leap years (fails at 2100)
      loYear++;
   };
   // Check for one too many days in a non-leap year.
   // (Note that this algorithm fails at 2100, which is not a leap year.)
   if ( tempI4 == 366  &&  (loYear & 0x3) != 0 )
   {
      loYear++;
      tempI4 = 1;
   }

   loDayOfY = tempI4;
   *Year    = (U2)loYear;

   // Convert Day of Year to Day of Month and Month of Year
   for ( i=0 ; i<12 ; i++ )
   {
      if ( loDayOfY <= Days_in_Month[i] )
      {
         *Day   = (U2)loDayOfY;
         *Month = (U2)i+1;
         break;
      }
       else
      {
         loDayOfY = loDayOfY - Days_in_Month[i];

         // Check for the extra day in February in Leap years
         if ( i == 1  &&  (loYear & 0x3) == 0 )   // Only Works up to 2100
         {
            if ( loDayOfY > (29-28) )  // After Feb 29th in a Leap year
            {
               loDayOfY--;                // Take off the 29th Feb
            }
            else                       // Must be the 29th of Feb on a Leap year
            {
               *Day     = (U2)29;
               *Month   = (U2)2;
               break;
            }
         }
      }
   }

   return;
}

//*****************************************************************************

