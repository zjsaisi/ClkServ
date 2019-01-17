
//*****************************************************************************
// GPS IP Centre, ST-Ericsson (UK) Ltd.
// Copyright (c) 2009 ST-Ericsson (UK) Ltd.
// 15-16 Cottesbrooke Park, Heartlands Business Park, Daventry, NN11 8YL, UK.
// All rights reserved.
//
// Filename  GN_GPS_GNB_Patch_506.c
//
// $Header: GNS/GN_GPS_GNB_Patch_506.c 1.1 2011/01/07 13:30:52PST Daniel Brown (dbrown) Exp  $
// $Locker:  $
//*****************************************************************************


//*****************************************************************************
//
// Example utility functions relating to Uploading the GN Baseband Patch code
// for GNS7560 ROM5 v506.
//
// This module need only to be included if the chip could be seen on the target.
//
//*****************************************************************************

#include "GN_GPS_Task.h"

#ifdef GN_GPS_GNB_PATCH_506

//*****************************************************************************
// The following are parameters relating to uploading a patch file
// to the baseband chip

extern U1  gn_Patch_Status;         // Status of GPS baseband patch transmission
extern U2  gn_Cur_Mess[5];          // Current messages to send, of each 'stage'

#include "Baseband_Patch_506.c"     // The baseband patch code to upload


//*****************************************************************************
U2  GN_GetPatchCheckSum_506()
{
   return( PatchCheckSum );
}


//*****************************************************************************
// Upload the next set of patch code to the GPS baseband.
// This function sends up to a maximum of Max_Num_Patch_Mess sentences each time
// it is called.
// The complete set of patch data is divided into six stages.

void GN_Upload_GNB_Patch_506(
   U4 Max_Num_Patch_Mess ) // i  - Maximum Number of Patch Messages to Upload
{
   const U1 *p_Comm;       // Pointer to the array of sentence identifiers to send
   const U2 *p_Addr;       // Pointer to the array of addresses to send
   const U2 *p_Data;       // pointer to the array of data to send
   U2       StartAdr;      // First address of type 23 consecutive addresses
   U1       index;         // Current 'index' of data to be sent
   U1       ckSum;         // Checksum
   U1       CS1;           // First character of checksum in Hex
   U1       CS2;           // Second character of checksum in Hex
   U2       Addr;          // Address to be sent
   U4       i;             // Index into array for checksum calculation
   U4       tot_num;       // Total number of sentences in current stage available
   U2       size;          // Size of patch data to send
   U4       num_sent;      // Number of sentences sent in the current session
   CH       patch_data[30];// Single sentence to be sent

   // When a patch file is available for upload we must write it out here.
   // We must write full sentences only.
   // We reach here approx once every UART_OUT_TASK_SLEEP ms.

   switch( gn_Patch_Status )
   {
         case 1:               // First stage
         {
            index    = 0;
            tot_num  = N_1;
            StartAdr = 0;                                // Remove compiler warning
            p_Comm   = &Comm1[gn_Cur_Mess[index]];
            p_Addr   = &Addr1[gn_Cur_Mess[index]];
            p_Data   = &Data1[gn_Cur_Mess[index]];
            break;
         }
         case 2:               // Second stage
         {                     // All the second stage are COMD 23
            index    = 1;      // and are consecutive addresses
            tot_num  = N_2;
            StartAdr = START_ADDR_2;
            p_Comm   = NULL;                             // Remove compiler warning
            p_Addr   = NULL;                             // Remove compiler warning
            p_Data   = &Data2[gn_Cur_Mess[index]];
            break;
         }
         case 3:               // Third stage
         {
            index    = 2;
            tot_num  = N_3;
            StartAdr = 0;                                // Remove compiler warning
            p_Comm   = &Comm3[gn_Cur_Mess[index]];
            p_Addr   = &Addr3[gn_Cur_Mess[index]];
            p_Data   = &Data3[gn_Cur_Mess[index]];
            break;
         }
         case 4:
         {                    // All the 4th stage are COMD 23
            index    = 3;     // and are consecutive addresses
            tot_num  = N_4;
            StartAdr = START_ADDR_4;
            p_Comm   = NULL;                             // Remove compiler warning
            p_Addr   = NULL;                             // Remove compiler warning
            p_Data   = &Data4[gn_Cur_Mess[index]];
            break;
         }
         case 5:             // Fifth stage
         {
            index    = 4;
            tot_num  = N_5;
            StartAdr = 0;                                // Remove compiler warning
            p_Comm   = &Comm5[gn_Cur_Mess[index]];
            p_Addr   = &Addr5[gn_Cur_Mess[index]];
            p_Data   = &Data5[gn_Cur_Mess[index]];
            break;
         }
         default : return;          // Error
   }

   num_sent = 0;
   while ( (num_sent < Max_Num_Patch_Mess) && (gn_Cur_Mess[index] < tot_num) )
   {
      if ( (index == 1) || (index == 3) )
      {
         // All the index 1 and 3 sentences are of type 23, with consecutive addresses
         size = 25;
         Addr = StartAdr + gn_Cur_Mess[index];
         sprintf( (char*)patch_data,"#COMD 23 %05d %05d &   ",
                  Addr, p_Data[num_sent] );
      }
      else
      {
         if ( p_Comm[num_sent] == 11 )
         {
            size = 15;
            sprintf( (char*)patch_data,"#COMD %2d %1d &   ",
                     p_Comm[num_sent], p_Addr[num_sent]);
         }
         else if ( p_Comm[num_sent] == 25 )
         {
            size = 19;
            sprintf( (char*)patch_data,"#COMD %2d %05d &   ",
                     p_Comm[num_sent], p_Addr[num_sent]);
         }
         else
         {
            size = 25;
            sprintf( (char*)patch_data, "#COMD %2d %05d %05d &   ",
                     p_Comm[num_sent], p_Addr[num_sent], p_Data[num_sent]);
         }
      }

      // Add the checksum
      ckSum = 0;   // Checksum counter.
      i     = 1;   // Don't include the '#' in the checksum validation.
      do
      {
         ckSum = (U1)(ckSum + (U1)patch_data[i++]);
      } while ( patch_data[i] != '&' );
      i++;
      CS1 = ckSum/16;
      if ( CS1 <= 9 )  patch_data[i] = (CH)(CS1+48);
      else             patch_data[i] = (CH)(CS1+55);
      i++;
      CS2 = ckSum % 16;
      if ( CS2 <= 9 )  patch_data[i] = (CH)(CS2+48);
      else             patch_data[i] = (CH)(CS2+55);
      i++;

      // Add the LF
      patch_data[i] = (CH)(0x0A);

      // Send the data
      if ( GN_GPS_Write_GNB_Ctrl( size, patch_data ) != size )
      {
         U2 num;
         CH temp_buffer[128];
         num = sprintf( temp_buffer, "GN_Upload_GNB_Patch: ERROR:  Some Patch data not taken by UART TX driver.\n\r" );
         GN_GPS_Write_Event_Log( num, (CH*)temp_buffer );
      }

      gn_Cur_Mess[index]++;
      num_sent++;
   }

   // Check if all the data has been sent
   if ( gn_Cur_Mess[index] == tot_num )
   {
      gn_Patch_Status++;
   }

   if ( gn_Patch_Status == 6 )
   {
      U2 num;
      CH temp_buffer[128];
      num = sprintf( temp_buffer, "GN_Upload_GNB_Patch: COMPLETED.\n\r" );
      GN_GPS_Write_Event_Log( num, (CH*)temp_buffer );

      // Clear the counter of messages sent ready for a future time.
      gn_Cur_Mess[0]    = 0;
      gn_Cur_Mess[1]    = 0;
      gn_Cur_Mess[2]    = 0;
      gn_Cur_Mess[3]    = 0;
      gn_Cur_Mess[4]    = 0;

      gn_Patch_Status   = 7;   // Flag the end of the patch data
   }

   return;
}

//*****************************************************************************

#endif   // GN_GPS_GNB_PATCH_506
