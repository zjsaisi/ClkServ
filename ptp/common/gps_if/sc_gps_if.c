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

FILE NAME    : sc_gps_if.c

AUTHOR       : Daniel brown

DESCRIPTION  : 

This unit contains the definitions for the following user defined SC GPS API functions.
      SC_InitGpsIf
      SC_GpsRxData
      SC_GpsTxData
      SC_CloseGpsIf
      SC_GpsRead_RTC
      SC_GpsWrite_RTC
      SC_GpsRead_NV
      SC_GpsWrite_NV
      SC_GpsPwrOff
      SC_GpsPwrOn

Revision control header:
$Id: gps_if/sc_gps_if.c 1.6 2011/04/01 15:21:55PDT Daniel Brown (dbrown) Exp  $

******************************************************************************
*/

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>

#include "sc_api.h"
#include "sc_gps_if.h"
#include "local_debug.h"

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/

/* handle for serial com port */
static int hPort;

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/
static void GpsPwrOff(void);
static void GpsPwrOn(void);

extern void TODtoPhasorTask (SC_t_GPS_TOD *ps_gpsTOD);

/*
----------------------------------------------------------------------------
                                SC_InitGpsIf()

Description:
   Opens GPS interface port. Called by SCi2000 upon invoking SC_InitConfigComplete()

Parameters:

Inputs
        None

Outputs:
        None

Return value:
        0: function succeeded
	    -1: function failed

-----------------------------------------------------------------------------
*/
int SC_InitGpsIf(void)
{
//   // Local Data Definitions
//   int            err;           // Error code
//   struct stat    status;        // Returns the status of the baseband serial port.
//   struct termios curr_term;     // The current serial port configuration descriptor.
//   speed_t        baud;
//
//#ifdef SC_GPS_IF_DEBUG
//   printf( "+SC_InitGpsIf: start\r\n" );
//#endif
//
//   hPort = -1;
//
//   // First check the port exists.
//   // This avoids thread cancellation if the port doesn't exist.
//   hPort = -1;
//   if ( ( err = stat( PORT_NAME, &status ) ) == -1 )
//   {
//#ifdef SC_GPS_IF_DEBUG
//      printf( " GN_Port_Setup: stat() = %d,  errno %d\r\n", err, errno );
//#endif
//      return( -1 );
//   }
//
//   // Open the serial port.
//   hPort = open( PORT_NAME, (O_RDWR | O_NOCTTY | O_NONBLOCK) );
//   if ( hPort <= 0 )
//   {
//      printf( " GN_Port_Setup: open() = %d, errno %d\r\n", hPort, errno );
//      return( -1 );
//   }
//
//   // Get the current serial port terminal state.
//   if ( ( err = tcgetattr( hPort, &curr_term ) ) != 0 )
//   {
//      printf( " GN_Port_Setup: tcgetattr(%d) = %d,  errno %d\r\n", hPort, err, errno );
//      close( hPort );
//      return( -1 );
//   }
//
//   // Set the terminal state to local (ie a local serial port, not a modem port),
//   // raw (ie binary mode).
//#ifdef __USE_BSD
//   curr_term.c_cflag  =  curr_term.c_cflag | CLOCAL;
//   cfmakeraw( &curr_term );
//#else
//   // Apparently the following does the same as cfmakeraw
//   curr_term.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
//   curr_term.c_oflag &= ~OPOST;
//   curr_term.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
//   curr_term.c_cflag &= ~(CSIZE|PARENB);
//   curr_term.c_cflag |= CS8;
//#endif
//
//   // Disable modem hang-up on port close.
//   curr_term.c_cflag      =  curr_term.c_cflag & (~HUPCL);
//
//   // Set way that the read() driver behaves.
//// curr_term.c_cc[VMIN]   =  1;                 // read() returns when 1 byte available
//   curr_term.c_cc[VMIN]   =  0;                 // read() returns immediately
//   curr_term.c_cc[VTIME]  =  0;                 // read() Time-Out in 1/10th sec units
//
//   // Set the requested baud rate in the terminal state descriptor.
//   baud = B115200;
//
//   // Set the input baud rates in the termios.
//   if ( cfsetispeed( &curr_term, baud ) )
//   {
//      close( hPort );
//      return( -1 );
//   }
//
//   // Set the output baud rates in the termios.
//   if ( cfsetospeed( &curr_term, baud ) )
//   {
//      close( hPort );
//      return( -1 );
//   }
//
//   // Set the Parity state to NO PARITY.
//   curr_term.c_cflag  =  curr_term.c_cflag  &  (~( PARENB | PARODD ));
//
//   // Set the Flow control state to NONE.
//   curr_term.c_cflag  =  curr_term.c_cflag  &  (~CRTSCTS);
//   curr_term.c_iflag  =  curr_term.c_iflag  &  (~( IXON | IXOFF | IXANY ));
//
//   // Set the number of data bits to 8.
//   curr_term.c_cflag  =  curr_term.c_cflag  &  (~CSIZE);
//   curr_term.c_cflag  =  curr_term.c_cflag  |  CS8;
//
//   // Set 2 stop bits.
//   curr_term.c_cflag  =  curr_term.c_cflag  |  ( CSTOPB | CREAD );
//
//#ifdef SC_GPS_IF_DEBUG
//   printf( " curr_term   i = %08X   o = %08X   l = %08X   c = %08X   cc = %08X \r\n",
//           curr_term.c_iflag, curr_term.c_oflag, curr_term.c_lflag, curr_term.c_cflag, (unsigned int)curr_term.c_cc );
//#endif
//   // Now set the serial port configuration and flush the port.
//   if ( tcsetattr( hPort, TCSAFLUSH, &curr_term ) != 0 )
//   {
//      close( hPort );
//      return( -1 );;
//   }
//
//   /* Turn GPS receiver ON */
//   GpsPwrOn();
//
//   /* return success */
   return(0);
}

/*
----------------------------------------------------------------------------
                                SC_CloseGpsIf()

Description:
This function is called by the SCi2000 to close the GPS interface. It 
should be used by the host to free up any resources to be reclaimed 
including but not limited to the interface communication port to the 
GPS receiver.

Parameters:

Inputs
        None

Outputs
        None
        
Return value:
        None

-----------------------------------------------------------------------------
*/
void SC_CloseGpsIf(
	void
)
{
#ifdef SC_GPS_IF_DEBUG
   printf( "+SC_CloseGpsIf:\r\n" );
#endif
   close(hPort);   

   /* Turn GPS receiver OFF */
   GpsPwrOff();
}

/*
----------------------------------------------------------------------------
                                SC_GpsTxData()

Description:
This transmits data to the GPS baseband port. Called by SCi2000.

Parameters:

Inputs
        w_dataLength - length of data to be transmitted
        pb_txBuffer - pointer to data buffer

Outputs:
        None

Return value:
         bytes_written: Number of bytes transmitted
         0: Zero bytes written

-----------------------------------------------------------------------------
*/
int SC_GpsTxData(
   UINT16 w_dataLength, 
   UINT8 *pb_txBuffer
)
{
   int bytes_written;               // Number of bytes written

   // Interface directly with the UART Output Driver.

   // write() returns -1 on an error (eg no data written).
   bytes_written = write( hPort, pb_txBuffer, w_dataLength );
   
   if (bytes_written < 0)
   {
      bytes_written = 0;
   }

   return(bytes_written);

}

/*
----------------------------------------------------------------------------
                                SC_GpsRxData()

Description:
This transmits data to the GPS baseband port. Called by SCi2000.

Parameters:

Inputs
        w_dataLength - length of data to be transmitted
        pb_rxBuffer - pointer to data buffer

Outputs:
        None

Return value:
         bytes_read: Number of bytes transmitted
         0: Zero bytes written

-----------------------------------------------------------------------------
*/
int SC_GpsRxData(
   UINT16 w_dataLength, 
   UINT8 *pb_rxBuffer
)
{
   int bytes_read;               // Number of bytes read

   // Interface directly with the UART Output Driver.

   // read() returns -1 on an error (eg no data read).
   bytes_read = read(hPort, pb_rxBuffer, w_dataLength);
   
   if (bytes_read < 0)
   {
      bytes_read = 0;
   }

   return(bytes_read);

}

/***********************************************************************
** 
** Function    : SC_GpsTod
**
** Description : This function delivers TOD from the GPS engine to the 
**               host. TOD info contains GPS week number, GPS seconds,
**               UTC offset, and leap seconds information. Called by SCi. 
**
** Parameters  :
**               ps_gpsTOD - TOD structure
**
** Returnvalue :  0  - function succeeded
**               -1  - function failed
**
** Remarks     : -
**
***********************************************************************/
int SC_GpsTod (
   SC_t_GPS_TOD *ps_gpsTOD
)
{
   /* pass data to phasor task */
   TODtoPhasorTask(ps_gpsTOD);
   
   /* return success */
   return 0;
}

/*
----------------------------------------------------------------------------
                                SC_GpsReset()

Description: Function to reset the GPS receiver.

Parameters: 

Input
   none

Output
	none

Return value:
   none

Remarks:
-----------------------------------------------------------------------------
*/
extern void SC_GpsReset(
   void
)
{
   int modem_stat;               // Modem line state

#ifdef SC_GPS_IF_DEBUG
      printf( "+SC_GpsReset: OFF\n");
#endif

   if ( hPort <= -1 )
      return;

   // Read port's Modem Line state.
   if ( ioctl( hPort, TIOCMGET, &modem_stat ) == -1 )
   {
#ifdef SC_GPS_IF_DEBUG
      printf( "SC_GpsReset:  ioctl( TIOCMGET ) error %d\n", errno );
#endif
      return;
   }

   // Setting / Enabling the RTS bit sets the RTS line "Low".
   modem_stat = modem_stat | (TIOCM_RTS);

   // Write back the Modem line state
   if ( ioctl( hPort, TIOCMSET, &modem_stat ) == -1 )
   {
#ifdef SC_GPS_IF_DEBUG
      printf( "SC_GpsReset:  ioctl( TIOCMSET ) error %d\n", errno );
#endif
      return;
   }

   /* delay for a half second */
   usleep(500 * 1000);
   

#ifdef SC_GPS_IF_DEBUG
      printf( "+SC_GpsReset: ON\n");
#endif

   if (hPort <= -1)
      return;

   // Read port's Modem Line state.
   if (ioctl (hPort, TIOCMGET, &modem_stat ) == -1)
   {
#ifdef SC_GPS_IF_DEBUG
      printf( "SC_GpsReset:  ioctl( TIOCMGET ) error %d\n", errno );
#endif
      return;
   }

   // Clearing / Disabling the RTS bit, sets the RTS line "High".
   modem_stat = modem_stat & (~TIOCM_RTS);

   // Write back the Modem line state
   if (ioctl (hPort, TIOCMSET, &modem_stat) == -1)
   {
#ifdef SC_GPS_IF_DEBUG
      printf( "SC_GpsReset:  ioctl( TIOCMSET ) error %d\n", errno );
#endif
      return;
   }
}

/*
----------------------------------------------------------------------------
                                GpsPwrOff()

Description: Function to turn the GPS receiver off (put the receiver in reset).

Parameters: 

Input
   none

Output
	none

Return value:
   none

Remarks:
-----------------------------------------------------------------------------
*/
static void GpsPwrOff(
   void
)
{
   int modem_stat;               // Modem line state

#ifdef SC_GPS_IF_DEBUG
      printf( "+GpsPwrOff: OFF\n");
#endif

   if ( hPort <= -1 )
      return;

   // Read port's Modem Line state.
   if ( ioctl( hPort, TIOCMGET, &modem_stat ) == -1 )
   {
#ifdef SC_GPS_IF_DEBUG
      printf( "GpsPwrOff:  ioctl( TIOCMGET ) error %d\n", errno );
#endif
      return;
   }

   // Setting / Enabling the RTS bit sets the RTS line "Low".
   modem_stat = modem_stat | (TIOCM_RTS);

   // Write back the Modem line state
   if ( ioctl( hPort, TIOCMSET, &modem_stat ) == -1 )
   {
#ifdef SC_GPS_IF_DEBUG
      printf( "GpsPwrOff:  ioctl( TIOCMSET ) error %d\n", errno );
#endif
      return;
   }
}


/*
----------------------------------------------------------------------------
                                GpsPwrOn()

Description: Function to turn the GPS receiver on (take the receiver out of reset).

Parameters: 

Input
   none

Output
	none

Return value:
   none

Remarks:
-----------------------------------------------------------------------------
*/
static void GpsPwrOn(
   void
)
{
   int modem_stat;               // Modem line state
   

#ifdef SC_GPS_IF_DEBUG
      printf( "+GpsPwrOn: ON\n");
#endif

   if (hPort <= -1)
      return;

   // Read port's Modem Line state.
   if (ioctl (hPort, TIOCMGET, &modem_stat ) == -1)
   {
#ifdef SC_GPS_IF_DEBUG
      printf( "GpsPwrOn:  ioctl( TIOCMGET ) error %d\n", errno );
#endif
      return;
   }

   // Clearing / Disabling the RTS bit, sets the RTS line "High".
   modem_stat = modem_stat & (~TIOCM_RTS);

   // Write back the Modem line state
   if (ioctl (hPort, TIOCMSET, &modem_stat) == -1)
   {
#ifdef SC_GPS_IF_DEBUG
      printf( "GpsPwrOn:  ioctl( TIOCMSET ) error %d\n", errno );
#endif
      return;
   }
}
