
//*****************************************************************************
// GPS IP Centre, ST-Ericsson (UK) Ltd.
// Copyright (c) 2009 ST-Ericsson (UK) Ltd.
// 15-16 Cottesbrooke Park, Heartlands Business Park, Daventry, NN11 8YL, UK.
// All rights reserved.
//
// Filename  GN_GPS_GNB_Patch_301.c
//
// $Header: GNS/GN_GPS_GNB_Patch_301.c 1.2 2011/04/01 15:05:50PDT Daniel Brown (dbrown) Exp  $
// $Locker:  $
//*****************************************************************************


//*****************************************************************************
//
// Example utility functions relating to Uploading the GN Baseband Patch code
// for GNS4540 ROM3 v301.
//
// This module need only to be included if the chip could be seen on the target.
//
//*****************************************************************************

#include "GN_GPS_Task.h"

#ifdef GN_GPS_GNB_PATCH_301

//*****************************************************************************
// The following are parameters relating to uploading a patch file
// to the baseband chip.

extern U1  gn_Patch_Status;            // Status of GPS baseband patch transmission
extern U1  gn_Patch_Progress;          // % progress of patch upload for each stage
extern U2  gn_Cur_Mess[5];             // Current messages to send, of each 'stage'


#include "Baseband_Patch_301.c"           // The baseband patch code to upload


//*****************************************************************************
//  Get the the patch checksum for GNS4540 ROM3 v301 baseband.

U2 GN_GetPatchCheckSum_301( void )
{
   return( PatchCheckSum );
}


//*****************************************************************************
// Upload the next set of patch code to a GNS4540 ROM3 v301 baseband.
// This function sends up to a maximum of Max_Num_Patch_Mess sentences each time
// it is called.
// The complete set of patch data is divided into six stages.

void GN_Upload_GNB_Patch_301(
   U4 Max_Num_Patch_Mess ) // i  - Maximum Number of Patch Messages to Upload
{
   const U2 *p_Comm;       // Pointer to the array of sentence identifiers to send
   const U2 *p_Addr;       // Pointer to the array of addresses to send
   const U2 *p_Data;       // pointer to the array of data to send
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
         case 1:              // First stage
         {
            index   = 0;
            tot_num = NUM_1;
            p_Comm  = &Comm1[gn_Cur_Mess[index]];
            p_Addr  = &Addr1[gn_Cur_Mess[index]];
            p_Data  = &Data1[gn_Cur_Mess[index]];
            break;
         }
         case 2:              // Second stage
         {
            index   = 1;      // All the second stage are COMD 23
            tot_num = NUM_2;
            p_Comm  = NULL;                              // Remove compiler warning
            p_Addr  = &Addr2[gn_Cur_Mess[index]];
            p_Data  = &Data2[gn_Cur_Mess[index]];
            break;
         }
         case 3:              // Third stage
         {
            index   = 2;
            tot_num = NUM_3;
            p_Comm  = &Comm3[gn_Cur_Mess[index]];
            p_Addr  = &Addr3[gn_Cur_Mess[index]];
            p_Data  = &Data3[gn_Cur_Mess[index]];
            break;
         }
         case 4:             // Fourth stage (i.e. first upload)
         case 5:             // Fifth stage  (i.e. second upload
         {                   // All the 4th/5th 'index' are COMD 23
            index   = 3;     // and are consecutive addresses
            tot_num = NUM_4;
            p_Comm  = NULL;                              // Remove compiler warning
            p_Addr  = NULL;                              // Remove compiler warning
            p_Data  = &Data4[gn_Cur_Mess[index]];
            break;
         }
         case 6:             // Sixth stage
         {
            index   = 4;
            tot_num = NUM_6;
            p_Comm  = &Comm6[gn_Cur_Mess[index]];
            p_Addr  = &Addr6[gn_Cur_Mess[index]];
            p_Data  = &Data6[gn_Cur_Mess[index]];
            break;
         }
         default : return;          // Error
   }

   num_sent = 0;
   while ( (num_sent < Max_Num_Patch_Mess) && (gn_Cur_Mess[index] < tot_num) )
   {
      if ( index == 1 )
      {
         // All the index 1 sentences are of type 23
         size = 26;
         sprintf( (char*)patch_data,"#COMD 23 %05d %05d &  \n\r",
                  p_Addr[num_sent], p_Data[num_sent] );
      }
      else if ( index == 3 )
      {
         // All the index 3 sentences are of type 23, with consecutive addresses
         size = 26;
         Addr = START_ADDR_4 + gn_Cur_Mess[index];
         sprintf( (char*)patch_data,"#COMD 23 %05d %05d &  \n\r",
                  Addr, p_Data[num_sent] );
      }
      else
      {
         if ( p_Comm[num_sent] == 11 )
         {
            size = 16;
            sprintf( (char*)patch_data,"#COMD %2d %1d &  \n\r",
                     p_Comm[num_sent], p_Addr[num_sent]);
         }
         else if ( p_Comm[num_sent] == 25 )
         {
            size = 20;
            sprintf( (char*)patch_data,"#COMD %2d %05d &  \n\r",
                     p_Comm[num_sent], p_Addr[num_sent]);
         }
         else
         {
            size = 26;
            sprintf( (char*)patch_data, "#COMD %2d %05d %05d &  \n\r",
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

      // Send the data
      if ( GN_GPS_Write_GNB_Ctrl( size, patch_data ) != size )
      {
#ifdef GNS_LOG_ENABLE
         GN_Log( "GN_Upload_GNB_Patch: ERROR:  Some Patch data not taken by UART TX driver" );
#endif
      }

      gn_Cur_Mess[index]++;
      num_sent++;
   }

   // Check if all the data has been sent
   if ( gn_Cur_Mess[index] == tot_num )
   {
      gn_Patch_Status++;
      gn_Patch_Progress = 0;
      if ( gn_Patch_Status == 5 )
      {
         gn_Cur_Mess[3] = 0;  // Repeat sending second patch file
      }
   }
   else
   {
      gn_Patch_Progress = (U1)( (U4)gn_Cur_Mess[index] * 100 / tot_num );
   }

   if ( gn_Patch_Status == 7 )
   {
#ifdef GNS_LOG_ENABLE
      GN_Log( "GN_Upload_GNB_Patch: COMPLETED." );
#endif

      // Clear the counter of messages sent ready for a future time.
      gn_Patch_Progress = 100;
      gn_Cur_Mess[0]    = 0;
      gn_Cur_Mess[1]    = 0;
      gn_Cur_Mess[2]    = 0;
      gn_Cur_Mess[3]    = 0;
      gn_Cur_Mess[4]    = 0;
   }

   return;
}

//*****************************************************************************

#endif  // GN_GPS_GNB_PATCH_301
