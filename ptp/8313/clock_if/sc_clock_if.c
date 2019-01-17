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

FILE NAME    : sc_clock_if.c

AUTHOR       : Ken Ho

DESCRIPTION  :

The functions in this file should be edited by the customer to allow the
steering of the main synthesizer that is running the timestamper and other
timing systems.

        SC_InitClock()
        SC_PhaseOffset()
        SC_FrequencyCorrection()
        SC_SystemTime()
        SC_SetSynthOn()

Revision control header:
$Id: clock_if/sc_clock_if.c 1.43 2011/11/02 15:41:43PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "sc_types.h"
#include "sc_api.h"
#include "timestamper_if/sc_timestamp.h"
#include "network_if/sc_network_if.h"
#include "clock_if/sc_clock_if.h"
#include "log/sc_log.h"
#include "../../../drivers/fpga_map.h"
#include "../../../drivers/fpga_ioctl.h"
#include "natsemi_util/epl.h"
#include "local_debug.h"
#include "sys_defines.h"
#include "mgm_pub.h"
/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/

#define VWARP_CONFIG_FILE	"/active/config/Vwarp.cfg"

#define CONV_FACTOR		(1.0/2.91038E-11) // 2^-32/8
#if 0
#define EFC_SENSE		-(1.586e-11/16.0) // 1.586e-11/bit scaled to ppb
#endif

#if 1
#define EFC_SENSE       1.70530257e-12  /* sensitivity=delta(f)/fout=150M/(5M*2^44)*/
#endif

#define PTP_SET_CTRL		(SIOCDEVPRIVATE + 15)
#define PTP_TIMER_REG_CTRL	0
#define PTP_TIMER_REG_PRSC	1
#define PTP_TIMER_REG_OFF	2
#define PTP_TIMER_REG_FIPER1	3
#define PTP_TIMER_REG_ALARM1_L	4
#define PTP_TIMER_REG_ALARM1_H	5

#define PPS_CLOCKRATE 125000000

/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/
typedef struct
{
   double df_segFfo;       // fractional freq offset at start of segment ppb
   double df_segIncSlope;  // incremental slope of segment (1 is perfect)
   UINT8  b_doneFlag;      // one indicates segment data is valid
} t_vcalElement;

typedef struct
{
   UINT8 b_numSeg;         // number of segments in table
   UINT8 b_index;          // current index into table
   double df_minFreq;      // lower bounds of table ppb
   double df_maxFreq;      // upper bounds of table ppb
   double df_deltaFreq;    // delta of table ppb
   INT32 l_curFreq;        //current calibration frequency in E-11
   double df_zeroCalStart;
   double df_zeroCalEnd;
   t_vcalElement s_vcalElement[16];
} t_vcalTable;


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
static UINT32 dw_phaseMode = k_PHASE_MODE;
static INT32 ddw_accumOffset = 0;


/* warp calibration table for varactor steered OCXO */
static t_vcalTable s_vcalDat;

/* original frequency compensation at startup */
static UINT32  dw_origFreqComp;

/* net handles to access the netsocket ioctl function */
static t_netHandle as_netHandle[k_NUM_IF];

static int i_fpgaFd = -1;
static UINT8* pb_fpgaMem = (UINT8*)0xFFFFFFFF;

static BOOLEAN o_synthOn = FALSE;
static INT16 gi_utcOffset = 35; /*(TAI - UTC) value. Current as 2013-06 */
static PORT_OBJ s_ptpPort;
static PEPL_PORT_HANDLE ps_portHandle = &s_ptpPort;

extern int SC_WriteFreqCorrToFile(
   FLOAT32             f_freqCorr,
   t_ptpTimeStampType  s_freqCorrTime
);

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/
double SC_WarpFrequency(double df_fin);
static int openAndMapFPGA();


/*--------------------------------------------------------------------------
    int openAndMapFPGA()

Description:
Tries to open and map the FPGA.
file descriptor and pointer to FPGA are put in a global for other functions to use
Will not do anything if the FPGA is already opened

Return value:
  0          no error
  Any value different from zero is an error.

Parameters:

  Inputs
    None

  Outputs
    None

Global variables affected and border effects:
  Read
    i_fpgaFd
  Write
    i_fpgaFd, pb_fpgaMem
--------------------------------------------------------------------------*/
static int openAndMapFPGA()
{
  int fd;
  UINT8* devPtr;
  const char deviceName[] = "/dev/fpga";

  if (i_fpgaFd < 0) //first time open and map FPGA
  {
    fd = open(deviceName, O_RDWR);
    if(fd < 0)
    {
      syslog(LOG_SYMM|LOG_DEBUG, "Cannot open device %s\n", deviceName);
      return -1;
    }
#if 0
    devPtr = (UINT8*)mmap(0, FPGA_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (devPtr == MAP_FAILED)
    {
      close(fd);
      syslog(LOG_SYMM|LOG_DEBUG, "Memory mapping of fpga device failed\n");
      return -1;
    }
#endif
    //set the globals and return
    i_fpgaFd = fd;
#if 0
    pb_fpgaMem = devPtr + FPGA_OFFSET;
#endif
    return 0;
  }

  //all is fine and ready just return
  return 0;
}

/*
----------------------------------------------------------------------------
                                SC_InitClock()

Description:
This function is called by the Softclient code.  The host needs to include
code in this function to allocate memory, select clock inputs and dividers,
etc.  The client performs no action on successful return.  Error returns
result in the softclient staying in the INIT state and retrying the
initialization function after SYSTEM_TIMEOUT seconds.

Parameters:

Inputs
        None

Outputs
        None

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int SC_InitClock(void)
{
   int i;
   INT16 d_utcOffset;
   UINT32 dw_newComp;
   UINT32 dw_width = 100000000;  /* width of 1PPS pulse of 100ms */
   UINT32 dw_rxCfgOpts;
   t_ptpTimeStampType s_sysTime;
   RX_CFG_ITEMS s_rxCfgItems;
   char ac_buf[80];
   UINT32 *pwd_reg;

   ddw_accumOffset = 0;

   /* initialize TSU net handles */
#if 0
   for (i = 0; i < k_NUM_IF; i++)
   {
      as_netHandle[i].s_evtSock =
               SC_GetSockHdl(i, as_netHandle[i].s_if_data.ifr_name);
   }
#endif
   /* open fpga device if not already opened */
   if(openAndMapFPGA() != 0)
   {
     return -1;
   }

   /* set 1PPS pulse width to 100ms */
#if 0
   ioctl(i_fpgaFd, SET_PPS_WIDTH, &dw_width);
#endif
   as_netHandle[0].s_if_data.ifr_data = (void*)(ac_buf);
   pwd_reg = (UINT32*)ac_buf;

   if(o_synthOn)
   {
      Init_PHY_Resource();

      EPLWriteReg(ps_portHandle,0x1E, 0x803F);
      EPLWriteReg(ps_portHandle,0x1E, 0x803F);
      EPLWriteReg(ps_portHandle,0x04, 0x0181);
      EPLWriteReg(ps_portHandle, PHY_BMCR, BMCR_AUTO_NEG_ENABLE | BMCR_FORCE_SPEED_100 | BMCR_FORCE_SPEED_MASK | BMCR_FORCE_FULL_DUP);

      /* Enable 1588 clock, set start time, set rate to 0 */
      PTPEnable(ps_portHandle, FALSE);
      PTPClockSetRateAdjustment(ps_portHandle, 0, FALSE, FALSE);
      PTPClockSet(ps_portHandle, 1, 0);

      PTPSetClockConfig(ps_portHandle, CLKOPT_CLK_OUT_EN, 0x0A, 0x00, 8);//25MHz
      PTPEnable(ps_portHandle, TRUE);

      PTPClockSetRateAdjustment(ps_portHandle, 0, FALSE, TRUE);
      PTPSetTempRateDurationConfig (ps_portHandle, 0);

      /* Disable Transmit and Receive Timestamp */
      PTPSetTransmitConfig(ps_portHandle,  0, 0, 0, 0);
      memset( &s_rxCfgItems, 0, sizeof( RX_CFG_ITEMS));
      PTPSetReceiveConfig(ps_portHandle,  0, &s_rxCfgItems);

      /* Enable Transmit Timestamp operation */
      PTPSetTransmitConfig(ps_portHandle, TXOPT_IPV4_EN | TXOPT_TS_EN, 2, 0x00, 0x00);

      /* Enable Receive Timestamp operation */
      s_rxCfgItems.ptpVersion = 0x02;
      s_rxCfgItems.ptpFirstByteMask = 0xFF;
      s_rxCfgItems.ptpFirstByteData = 0x00;
      s_rxCfgItems.ipAddrData = 0;
      s_rxCfgItems.tsMinIFG  = 0x00;
      s_rxCfgItems.srcIdHash = 0;
      s_rxCfgItems.ptpDomain = 0;
      s_rxCfgItems.tsSecLen  = 0;
      s_rxCfgItems.rxTsSecondsOffset = 0;
      s_rxCfgItems.rxTsNanoSecOffset = 0;
      dw_rxCfgOpts = RXOPT_RX_IPV4_EN | RXOPT_RX_TS_EN | RXOPT_RX_SLAVE;

      PTPSetReceiveConfig(ps_portHandle, dw_rxCfgOpts, &s_rxCfgItems);

      /* enable 25MHz from Nat semi to be reference for 125MHz to 8313 */
      *(pb_fpgaMem + 0x05) = 0x80;
      *(pb_fpgaMem + 0x05) = 0x00;
      *(pb_fpgaMem + 0x05) = 0x10;

      /* center EFC */
      *(pb_fpgaMem + 0x1d) = 0x07;
      *(pb_fpgaMem + 0x1e) = 0xFF;
      *(pb_fpgaMem + 0x1f) = 0xFF;
      syslog(LOG_SYMM|LOG_DEBUG, "Set Synth\n");
   }
   else
   {
#if 0
      /* set prescaler */
      pwd_reg[0] = PTP_TIMER_REG_PRSC;
      pwd_reg[1] = 0x3d09;  /* 4KHz output */
      if (ioctl(as_netHandle[0].s_evtSock, PTP_SET_CTRL, &as_netHandle[0].s_if_data))
      {
         syslog(LOG_SYMM|LOG_DEBUG, "Set PTP_TIMER_REG_PRSC fail\n");
      }

      /* set to external FPGA phase control */
      pwd_reg[0] = PTP_TIMER_REG_CTRL;
      pwd_reg[1] = 0x1008000c; /* bypass mode running 125MHz clock to timestamper */
      if (ioctl(as_netHandle[0].s_evtSock, PTP_SET_CTRL, &as_netHandle[0].s_if_data))
      {
         syslog(LOG_SYMM|LOG_DEBUG, "Set PTP_TIMER_REG_CTRL fail\n");
      }

      /* set frequency to half of system clock 125MHz/2 = 62.5MHz */
      dw_newComp = 0x80000000;
      as_netHandle[0].s_if_data.ifr_data = (void*)(&dw_newComp);
      if (ioctl(as_netHandle[0].s_evtSock, PTP_ADJ_ADDEND, &as_netHandle[0].s_if_data))
      {
         syslog(LOG_SYMM|LOG_DEBUG, "Set PTP_ADJ_ADDEND fail\n");
      }

      /* enable 10MHz OCXO to be reference for 125MHz to 8313 */
      *(pb_fpgaMem + 0x05) = 0x90;
      *(pb_fpgaMem + 0x05) = 0x10;
      *(pb_fpgaMem + 0x05) = 0x00;

      syslog(LOG_SYMM|LOG_DEBUG, "Set Varactor Steered\n");
#endif
   }

   /* set systime to get values inserted */
   SC_SystemTime(&s_sysTime,&d_utcOffset,e_PARAM_GET);
   SC_SystemTime(&s_sysTime,&d_utcOffset,e_PARAM_SET);

   /* get initial frequency compensation value from driver */
#if 0
   as_netHandle[0].s_if_data.ifr_data = (void*)(&dw_origFreqComp);
   if (ioctl(as_netHandle[0].s_evtSock, PTP_GET_ADDEND, &as_netHandle[0].s_if_data))
   {
      return -1;
   }
#endif
   return 0;
}

/*
----------------------------------------------------------------------------
                                SC_PhaseOffset()

Description:
This function is called by the Softclient code.  This function sets or gets
the phase correction  value.  The signed offset is in nanoseconds.  The
offset should be added to the current clock value.  This function is called
with a write operation at roughly 1 Hz after softClient has reached the XXXX
state.

Parameters:

Inputs
        INT64 *pll_ phaseCorrection
        On a set, this pointer defines the the current phase correction to
        be applied to the local clock.  The value is in units of nanoseconds.

        t_paramOperEnum b_operation
        This enumeration defines the operation of the function to either
        set or get.
                e_PARAM_GET - Get current phase correction
                e_PARAM_SET - Set phase correction

Outputs
        INT64 *pll_ phaseCorrection
        On a get, the current phase correction is written to this pointer
        location.

Return value:
         0: function succeeded
        -1: function failed
-----------------------------------------------------------------------------
*/
int SC_PhaseOffset(INT64           *pll_phaseOffset,
                   t_paramOperEnum b_operation)
{
   INT64  ll_offset = *pll_phaseOffset;
   UINT32 dw_offset;
   int  delta_offset;
   static INT64  prev_ll_offset=0;
	printf("*******************************************************\n");
	printf("phase offset = %lld\n", prev_ll_offset);
	printf("*******************************************************\n");

   if (b_operation == e_PARAM_GET)
   {
      *pll_phaseOffset = prev_ll_offset;
   }
   /* Phase offset using FPGA */
   else if (b_operation == e_PARAM_SET)
   {
     delta_offset = ll_offset - prev_ll_offset;

	 if(abs(delta_offset > 1000)){
		syslog(LOG_TS|LOG_DEBUG,"PPS on change large than 1us: %dns\n", delta_offset);
	}
     /* open fpga device if not already opened */
     if(openAndMapFPGA() != 0)
     {
       return -1;
     }

      /* make unsigned */
      if (ll_offset < 0)
      {
         dw_offset = (UINT32)(ll_offset + 1000000000);
      }
      else
      {
         dw_offset = (UINT32)ll_offset;
      }

	  if ((s_localDebCfg.deb_mask & LOCAL_DEB_PH_TRACE_MASK)!=0)
	  {
		  printf("PH %lld, delta=%d\n", ll_offset, delta_offset);
	  }

      ioctl(i_fpgaFd, FPGA_SET_PPS_OFFSET, &dw_offset);

      prev_ll_offset = ll_offset;


   }
   else
   {
      return -1;
   }
   return 0;
}

UINT8 SC_GetPhasorLosCntr(void)
{
  /* open fpga device if not already opened */
	unsigned char c_val;

  if(openAndMapFPGA() != 0)
  {
    return -1;
  }

  /* check phase offset */
#if 0
  return (UINT8) *(pb_fpgaMem+FPGA_PPS_EDGE_CNT);
#endif
	ioctl(i_fpgaFd, FPGA_GET_PPS_EDGE_CNT, (UINT32)&c_val);
	return c_val;
}

UINT8 SC_GetExtRefPhasorLosCntr(void)
{
  /* open fpga device if not already opened */
  if(openAndMapFPGA() != 0)
  {
    return -1;
  }

  /* check phase offset */
#if 0
  return (UINT8) *(pb_fpgaMem+FPGA_LOS_COUNT_EXT);
#endif
	return 0;
}

int SC_GetPhasorOffset(INT32 *i32_phaseOffset, UINT32 dw_fpgaOffset)
{
   INT32 phase_offset;
	INT32 se2_phase_offset;
	double se2_nsec;
	double nat_semi_nsec;
	INT32 diff_ns;

  /* open fpga device if not already opened */
  if(openAndMapFPGA() != 0)
  {
   return -1;
  }

	ioctl(i_fpgaFd, FPGA_GET_PPS_PHA_OFFSET, (UINT32)&phase_offset);
	ioctl(i_fpgaFd, FPGA_GET_GPS_PHA_OFFSET, (UINT32)&se2_phase_offset);

	se2_nsec = (double)se2_phase_offset * (1000000000.0/(double)PPS_CLOCKRATE);
	nat_semi_nsec = (double)phase_offset * (1000000000.0/(double)PPS_CLOCKRATE);
	diff_ns= (INT32)(se2_nsec - nat_semi_nsec);
	*i32_phaseOffset=-diff_ns;

#if 1
	printf("*******%s: GPS %x P %x diff %d********\n", __FUNCTION__, se2_phase_offset,
			phase_offset, *i32_phaseOffset);
#endif
   return 0;
}

int SC_GetPhasorOffset1(INT32 *i32_phaseOffset, UINT32 dw_fpgaOffset)
{
   INT32 phase_offset;
	INT32 se2_phase_offset;
	double se2_nsec;
	double nat_semi_nsec;
	INT32 diff_ns;

  /* open fpga device if not already opened */
  if(openAndMapFPGA() != 0)
  {
   return -1;
  }

	ioctl(i_fpgaFd, FPGA_GET_PPS_PHA_OFFSET, (UINT32)&phase_offset);
	ioctl(i_fpgaFd, FPGA_GET_BD_PHA_OFFSET, (UINT32)&se2_phase_offset);

	se2_nsec = (double)se2_phase_offset * (1000000000.0/(double)PPS_CLOCKRATE);
	nat_semi_nsec = (double)phase_offset * (1000000000.0/(double)PPS_CLOCKRATE);
	diff_ns= (INT32)(se2_nsec - nat_semi_nsec);
	*i32_phaseOffset=-diff_ns;

#if 1
	printf("*******%s: Beidou %x P %x diff %d********\n", __FUNCTION__, se2_phase_offset,
			phase_offset, *i32_phaseOffset);
#endif

   return 0;
}
int SC_GetPhasorOffset2(INT32 *i32_phaseOffset, UINT32 dw_fpgaOffset)
{
   INT32 phase_offset;
	INT32 se2_phase_offset;
	double se2_nsec;
	double nat_semi_nsec;
	INT32 diff_ns;

  /* open fpga device if not already opened */
  if(openAndMapFPGA() != 0)
  {
   return -1;
  }

	ioctl(i_fpgaFd, FPGA_GET_PPS_PHA_OFFSET, (UINT32)&phase_offset);
	ioctl(i_fpgaFd, FPGA_GET_OPTI1_PHA_OFFSET, (UINT32)&se2_phase_offset);

	se2_nsec = (double)se2_phase_offset * (1000000000.0/(double)PPS_CLOCKRATE);
	nat_semi_nsec = (double)phase_offset * (1000000000.0/(double)PPS_CLOCKRATE);
	diff_ns= (INT32)(se2_nsec - nat_semi_nsec);
	*i32_phaseOffset=-diff_ns;

#if 1
	printf("*******%s: IRIG-B1 %x P %x diff %d********\n", __FUNCTION__, se2_phase_offset,
			phase_offset, *i32_phaseOffset);
#endif

   return 0;
}

int SC_GetPhasorOffset3(INT32 *i32_phaseOffset, UINT32 dw_fpgaOffset)
{
   INT32 phase_offset;
	INT32 se2_phase_offset;
	double se2_nsec;
	double nat_semi_nsec;
	INT32 diff_ns;

  /* open fpga device if not already opened */
  if(openAndMapFPGA() != 0)
  {
   return -1;
  }

	ioctl(i_fpgaFd, FPGA_GET_PPS_PHA_OFFSET, (UINT32)&phase_offset);
	ioctl(i_fpgaFd, FPGA_GET_OPTI2_PHA_OFFSET, (UINT32)&se2_phase_offset);

	se2_nsec = (double)se2_phase_offset * (1000000000.0/(double)PPS_CLOCKRATE);
	nat_semi_nsec = (double)phase_offset * (1000000000.0/(double)PPS_CLOCKRATE);
	diff_ns= (INT32)(se2_nsec - nat_semi_nsec);
	*i32_phaseOffset=-diff_ns;

#if 1
	printf("*******%s: IRIG-B2 %x P %x diff %d********\n", __FUNCTION__, se2_phase_offset,
			phase_offset, *i32_phaseOffset);
#endif

   return 0;
}

#ifdef GPS_BUILD
#if 0
int SC_GetPpsPhaseOffset(INT32 *i32_phaseOffset)
{
   UINT8  u8_data;

   /* open fpga device if not already opened */
   if(openAndMapFPGA() != 0)
   {
     return -1;
   }

//   printf("GPS Phase Offset Address: 0x%X\n", (unsigned int)(pb_gps_fpgaMem + FPGA_GPS_IN_CNT3));
   u8_data = 0;
   *i32_phaseOffset = 0;
   u8_data = *(pb_fpgaMem + FPGA_1PPS_ON_CNT3);
   *i32_phaseOffset = u8_data;
   u8_data = *(pb_fpgaMem + FPGA_1PPS_ON_CNT2);
   *i32_phaseOffset = (*i32_phaseOffset << 8) | u8_data;
   u8_data = *(pb_fpgaMem + FPGA_1PPS_ON_CNT1);
   *i32_phaseOffset = (*i32_phaseOffset << 8) | u8_data;
   u8_data = *(pb_fpgaMem + FPGA_1PPS_ON_CNT0);
   *i32_phaseOffset = (*i32_phaseOffset << 8) | u8_data;

   return 0;
}
#endif

#if 0
int SC_GetRefPhaseOffset(INT32 *i32_phaseOffset)
{
   UINT8  u8_data;

   /* open fpga device if not already opened */
   if(openAndMapFPGA() != 0)
   {
     return -1;
   }

//   printf("Ref Phase Offset Address: 0x%X\n", (unsigned int)(pb_ref_fpgaMem + FPGA_1PPS_IN_CNT3));
   u8_data = 0;
   *i32_phaseOffset = 0;
   u8_data = *(pb_fpgaMem + FPGA_1PPS_IN_CNT3);
   *i32_phaseOffset = u8_data;
   u8_data = *(pb_fpgaMem + FPGA_1PPS_IN_CNT2);
   *i32_phaseOffset = (*i32_phaseOffset << 8) | u8_data;
   u8_data = *(pb_fpgaMem + FPGA_1PPS_IN_CNT1);
   *i32_phaseOffset = (*i32_phaseOffset << 8) | u8_data;
   u8_data = *(pb_fpgaMem + FPGA_1PPS_IN_CNT0);
   *i32_phaseOffset = (*i32_phaseOffset << 8) | u8_data;

   return 0;
}
#endif
#endif /* GPS_BUILD */

/*
----------------------------------------------------------------------------
                                SC_FrequencyCorrection()

Description:
This function is called by the Softclient code.  This function gets or sets
the current fractional frequency error.  The signed error is in ppb.  A
positive value indicates a greater frequency.  This function is called
with a write operation at roughly 1 Hz after softClient has reached the XXXX
state.

Parameters:

Inputs
        FLOAT64 *pff_freqCorr
        On a set, this pointer defines the the current frequency correction to
        be applied to the local clock.  The value is in units of ppb.

        t_paramOperEnum b_operation
        This enumeration defines the operation of the function to either set or
        get.
                e_PARAM_GET - Get fractional frequency offset
                e_PARAM_SET - Set fractional frequency offset

Outputs
        FLOAT64 *pff_freqCorr
        On a get, the current frequency correction is written to this pointer
        location.

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int SC_FrequencyCorrection(FLOAT64         *pff_freqCorr,
                           t_paramOperEnum b_operation)
{
   int ret = 0;
   int i_adjDirectionFlag;
   INT32 l_accumRate;
   INT32 l_temp1;
   double	freqCorr;

   if (b_operation == e_PARAM_GET)
   {
      *pff_freqCorr = 0;
      ret = 0;
   }
   else
   {

      freqCorr = *pff_freqCorr;

      /* if we force freq correct is zero, just force it */
      if (s_localDebCfg.force_fc_zero)
      {
		  freqCorr = 0.0;
	  }

	  if ((s_localDebCfg.deb_mask & LOCAL_DEB_FC_TRACE_MASK) != 0)
	  {
		  printf("FC %lf\n", freqCorr);
	  }

      if (o_synthOn)
      {
         /* Nation phy synthesizer */
         l_accumRate = freqCorr * 1e-9 * CONV_FACTOR; /* convert to National Phy bits */
         if(l_accumRate < 0)
         {
            i_adjDirectionFlag = 0;
            l_accumRate = -l_accumRate;
         }
         else
         {
            i_adjDirectionFlag = 1;
         }
         PTPClockSetRateAdjustment(ps_portHandle, l_accumRate, FALSE, i_adjDirectionFlag);
      }
      else
      {
         /* correct (warp) the desired frequency offset to get proper */
         /* frequency change                                          */
         *pff_freqCorr = SC_WarpFrequency(freqCorr);

         /* open fpga device if not already opened */
         if(openAndMapFPGA() != 0)
         {
           return -1;
         }
#if 0
         l_temp1 = 0x7FFFF + (INT32)(freqCorr * 1e-9 / EFC_SENSE); // convert from 1ppb to DAC bits
#endif
#if 1
		l_temp1 = (INT32)(freqCorr * 1e-9 / EFC_SENSE);
#endif

#if 0
         *(pb_fpgaMem + 0x1d) = (UINT8)((l_temp1 >> 16) & 0xFF);
         *(pb_fpgaMem + 0x1e) = (UINT8)((l_temp1 >> 8) & 0xFF);
         *(pb_fpgaMem + 0x1f) = (UINT8)(l_temp1 & 0xFF);
#endif
		ioctl(i_fpgaFd, FPGA_SET_FREQ_CORR, (uint32_t)&l_temp1);
      }
   }

   return ret;
}

/*
----------------------------------------------------------------------------
                                SC_SystemTime()

Description:
This function is called by the Softclient code.  This function either sets
or gets the system clock time.  The time is in seconds and nanoseconds.

Parameters:

Inputs
        t_ptpTimeStampType *ps_sysTime
        On a set, this pointer defines the the current system time.

        INT16 *pi_utcOffset
        On a set, this pointer defines the the current UTC offset.

        t_paramOperEnum b_operation
        This enumeration defines the operation of the function to either
        set or get.
                e_PARAM_GET - Get current system time
                e_PARAM_SET - Set system time

Outputs
        t_ptpTimeStampType *ps_sysTime
        On a get, the current system time is written to this pointer location.

        INT16 *pi_utcOffset
        On a get, the current UTC offset is written to this pointer location.

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int SC_SystemTime(t_ptpTimeStampType *ps_sysTime,
    INT16              *pi_utcOffset,
    t_paramOperEnum    b_operation)
{
  int ret = 0;
  t_8313TimeFormat s_ts;
  UINT64 ddw_boardTime;
  char strBuf[256];
	struct timespec cur_time;

  switch(b_operation)
  {
    case e_PARAM_GET:
#if 0
      /* get the time from the 8313 timestamper */
      /* prepare structure */
      as_netHandle[0].s_if_data.ifr_data = (void*)(&s_ts);
      if (ioctl(as_netHandle[0].s_evtSock, PTP_GET_CNT, &as_netHandle[0].s_if_data))
      {
        ret = -1;
      }
      else
      {
        ddw_boardTime = s_ts.dw_high;
        ddw_boardTime = (ddw_boardTime << 32) | s_ts.dw_low;
        ps_sysTime->u48_sec = ddw_boardTime / k_NSEC_IN_SEC;
        ps_sysTime->dw_nsec = ddw_boardTime - (ps_sysTime->u48_sec * k_NSEC_IN_SEC);
        if (pi_utcOffset != NULL)
          *pi_utcOffset = gi_utcOffset;
      }
#endif
        clock_gettime(CLOCK_REALTIME, &cur_time);
        ps_sysTime->u48_sec = cur_time.tv_sec + gi_utcOffset;
        ps_sysTime->dw_nsec = cur_time.tv_nsec;
        if(pi_utcOffset != NULL)
            *pi_utcOffset = gi_utcOffset;
    break;

    case e_PARAM_SET:
      /* prepare structure for 8313 timestamper */
#if 0
      ddw_boardTime = ps_sysTime->u48_sec * k_NSEC_IN_SEC + ps_sysTime->dw_nsec;
      s_ts.dw_high = (ddw_boardTime >> 32) & 0xffffffff;
      s_ts.dw_low = ddw_boardTime & 0xffffffff;

      /* set the time in the 8313 timestamper */
      as_netHandle[0].s_if_data.ifr_data = (void*)(&s_ts);
      if (ioctl(as_netHandle[0].s_evtSock, PTP_SET_CNT, &as_netHandle[0].s_if_data))
      {
        ret = -1;
      }
      else
      {
        ret = 0;
      }
#endif
#if 0
		ioctl(i_fpgaFd, FPGA_UPDATE_INTPPS, 0);
#endif
		
      /* now, be nice and try to set unix time */
	{
      struct timeval tv;
      if(pi_utcOffset != NULL) // if a value is provided, save it.
        gi_utcOffset = *pi_utcOffset;
#if 0
      tv.tv_sec = ps_sysTime->u48_sec - gi_utcOffset;
      tv.tv_usec = ps_sysTime->dw_nsec / 1000;
      settimeofday(&tv, NULL);
#endif
      snprintf(strBuf, 255, "SC_SystemTime set: high= %x low= %x\n", s_ts.dw_high, s_ts.dw_low);
      SC_DbgLog(strBuf);
	}
    break;

    default:
      ret = -1;
    break;
  }
  return ret;
}

/*
----------------------------------------------------------------------------
                                SC_StoreFreqCorr()

Description:
This function stores the frequency correction data into non-volatile
memory.   This function is called by the Softclient no more than every
hour.

Parameters:

Inputs
        FLOAT32  f_freqCorr
        The current frequency correction in ppb.

        t_ptpTimeStampType  s_freqCorrTime
        The timestamp for hourly frequency offset

Outputs
        None

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int SC_StoreFreqCorr(
   FLOAT32             f_freqCorr,
   t_ptpTimeStampType  s_freqCorrTime
)
{
   return SC_WriteFreqCorrToFile(f_freqCorr, s_freqCorrTime);
}

/*
----------------------------------------------------------------------------
                                SC_SetSynthOn()

Description:
This function is used to turn on and off the Synth mode.  It must be set
before any of the other clock functions are accessed.

Parameters:

Inputs
        BOOLEAN o_on
        If o_on is TRUE, then using the synthersizer mode, otherwise using
        the varactor (EFC) mode.

Outputs
       None

Return value:
       None
-----------------------------------------------------------------------------
*/
void SC_SetSynthOn(BOOLEAN o_on)
{
   o_synthOn = o_on;
}


/*
----------------------------------------------------------------------------
                                SC_WarpFrequency()

Description:
This function is used translate the desired frequency offset into a number
that will attain the desired frequency.

Parameters:

Inputs
        double df_fin
        Desired frequency input in ppb.

Outputs
       None

Return value:
       Frequency that should be sent to the varactor that will change the
       Ouput by the df_fin value.
-----------------------------------------------------------------------------
*/
double SC_WarpFrequency(double df_fin)
{
   int i;
   double df_fout, df_ferr;

   df_fout = df_fin; //default

   s_vcalDat.b_numSeg = 16;

   /* If less than minimum, then send with unity */
   if (df_fin < s_vcalDat.s_vcalElement[0].df_segFfo)
   {
      return df_fin;
   }

   /* If greater than maximum, then send with unity */
   if (df_fin > s_vcalDat.s_vcalElement[15].df_segFfo)
   {
      return df_fin;
   }

   /* find the segment that contains df_fin */
   i = 0;
   while (i < (s_vcalDat.b_numSeg-1))
   {
      if ((s_vcalDat.s_vcalElement[i].df_segFfo < df_fin) &&
          (s_vcalDat.s_vcalElement[i+1].df_segFfo > df_fin))
      {
         break;
      }
      i++;
   }

   /* If did not find canidate, just send back the same data */
   if (i >= s_vcalDat.b_numSeg)
   {
   }
   /* If a segment is found then interpolate */
   else
   {
      /* start with base segment */
      df_fout = s_vcalDat.df_minFreq + (double)(i) * s_vcalDat.df_deltaFreq;
      df_ferr = df_fin - s_vcalDat.s_vcalElement[i].df_segFfo; //error from base
      if (s_vcalDat.s_vcalElement[i].df_segIncSlope > 0.0)
      {
         df_ferr = df_ferr / s_vcalDat.s_vcalElement[i].df_segIncSlope;
      }
      df_fout += df_ferr;
   }
   return(df_fout);
}

/*
----------------------------------------------------------------------------
                                SC_ReadVcalFromFile()

Description:
This function is will load the varactor warp table into memory from
a file.  This table will be used when trying to steer the frequency.

Parameters:
       None
Inputs
       None
Outputs
       None
Return value:
       None
-----------------------------------------------------------------------------
*/
void SC_ReadVcalFromFile(void)
{
   FILE* fp_cfg;
   FLOAT64 df_prevInc;
   UINT32 i;
   char line[256];

   /* fill the frequency warp table */
   s_vcalDat.b_numSeg = 16;         // 16 points
   s_vcalDat.df_minFreq = -512.0;   // -512 ppb
   s_vcalDat.df_maxFreq = +512.0;   // +512 ppb
   s_vcalDat.b_index = 0;
   s_vcalDat.df_deltaFreq = (s_vcalDat.df_maxFreq-s_vcalDat.df_minFreq)/(double)(s_vcalDat.b_numSeg);

   for (i=0; i<16; i++)
   {
      s_vcalDat.s_vcalElement[i].df_segFfo = (double)(i) * s_vcalDat.df_deltaFreq - 512.0;
      s_vcalDat.s_vcalElement[i].df_segIncSlope = -1.0;  // give it unity as a default
      s_vcalDat.s_vcalElement[i].b_doneFlag = 1;
   }

   /* read in the file */
   fp_cfg = fopen(VWARP_CONFIG_FILE, "r");

   if (fp_cfg)
   {
      i = 0;

      while (fgets(line, 255, fp_cfg) != NULL)
      {
         sscanf(line, "%le", &s_vcalDat.s_vcalElement[i].df_segFfo);

         /* calculate the incremental differences */
         if (i > 0)
         {
            s_vcalDat.s_vcalElement[i-1].df_segIncSlope = (df_prevInc - s_vcalDat.s_vcalElement[i].df_segFfo) / 64.0;
//            printf("inc: %d, %le\n", i-1, s_vcalDat.s_vcalElement[i-1].df_segIncSlope);

         }
         df_prevInc = s_vcalDat.s_vcalElement[i].df_segFfo;
//         printf("ffo: %d, %le ", i, s_vcalDat.s_vcalElement[i].df_segFfo);
         i++;
      }
   }
   return;
}

INT64 getInbandOffset(void)
{
   return ddw_accumOffset;
}


void putPhaseMode(UINT32 dw_putPhaseMode)
{
   dw_phaseMode = dw_putPhaseMode;
   return;
}


int setExtRefDivider(BOOLEAN o_isRefA, UINT32 dw_divider)
{
  /* open fpga device if not already opened */
  if(openAndMapFPGA() != 0)
  {
    return -1;
  }

#if 0
   if(o_isRefA)
   {
      *(pb_fpgaMem + FPGA_EXT_REF_A_DIVIDER2) = (dw_divider >> 16) & 0xFF;
      *(pb_fpgaMem + FPGA_EXT_REF_A_DIVIDER1) = (dw_divider >> 8) & 0xFF;
      *(pb_fpgaMem + FPGA_EXT_REF_A_DIVIDER0) = dw_divider & 0xFF;
   }
   else
   {
      *(pb_fpgaMem + FPGA_EXT_REF_B_DIVIDER2) = (dw_divider >> 16) & 0xFF;
      *(pb_fpgaMem + FPGA_EXT_REF_B_DIVIDER1) = (dw_divider >> 8) & 0xFF;
      *(pb_fpgaMem + FPGA_EXT_REF_B_DIVIDER0) = dw_divider & 0xFF;
   }
#endif
   return 0;
}

/* this function will turn on E1/T1 output, default is Ext Ref A in */
int setE1T1OutEnable(UINT8 enable)
{
   UINT8 b_data;

   /* open fpga device if not already opened */
   if(openAndMapFPGA() != 0)
   {
     return -1;
   }
#if 0
   if(enable)
   {
      b_data = *(pb_fpgaMem + FPGA_LOS_COUNT_EXT);
      b_data = b_data | FPGA_E1_T1_OUT_ENABLE;
      *(pb_fpgaMem + FPGA_LOS_COUNT_EXT) = b_data;
   }
   else
   {
      b_data = *(pb_fpgaMem + FPGA_LOS_COUNT_EXT);
      b_data = b_data & ~FPGA_E1_T1_OUT_ENABLE;
      *(pb_fpgaMem + FPGA_LOS_COUNT_EXT) = b_data;      
   }
#endif
   return 0;
}

/*--------------------------------------------------------------------------
    UINT32 setExtOutFreq(UINT32 freq)

Description:
Attempts to program the external frequency output to the requested value in Hz.
Returns the frequency actually set.
Frequency is 8KHz * M, where M is a 12 bit number
25 MHz is special does not use the multiplier.
0 will just return the current set frequency.

Return value:
  Frequency actually set.
  0 indicates an error.

Parameters:

  Inputs
    UINT32 freq output frequency, 0 just to query

  Outputs
    None

Global variables affected and border effects:
  Read
    pb_fpgaMem
  Write
    pb_fpgaMem
--------------------------------------------------------------------------*/

UINT32 setExtOutFreq(UINT32 freq)
{
  const UINT32 k8KHz = 8000;
  const UINT32 k25MHz = 25000000;
  UINT32 multiplier;
  UINT8 temp;


  /* open fpga device if not already opened */
  if(openAndMapFPGA() != 0)
  {
    return 0;
  }
#if 0
  //do we want to set or only read
  if(freq != 0)
  {
    //25MHz is special
    if(freq == k25MHz)
    {
      //set 25MHz
      *(pb_fpgaMem + FPGA_FRQ_OUT_MODE_AND_MSN) = FPGA_FRQ_OUT_MODE_25_125M;
      *(pb_fpgaMem + FPGA_FRQ_OUT_MULT_LSB) = 0x01;
    }
    else
    {
      multiplier = freq / k8KHz;
      //only 11 bits are available, clamp the multiplier
      multiplier &= 0x07ff;

      *(pb_fpgaMem + FPGA_FRQ_OUT_MODE_AND_MSN) = (FPGA_FRQ_OUT_MULT_MSN & (multiplier >> 8)) + FPGA_FRQ_OUT_MODE_8K_MULT;
      *(pb_fpgaMem + FPGA_FRQ_OUT_MULT_LSB) = 0xff & multiplier;
    }
  }

  //now read and return the actual values
  temp = *(pb_fpgaMem + FPGA_FRQ_OUT_MODE_AND_MSN);

  //check if 25MHz
  if((temp & 0xf0) == FPGA_FRQ_OUT_MODE_25_125M)
  {
    return k25MHz;
  }
  else if((temp & 0xf0) == FPGA_FRQ_OUT_MODE_8K_MULT)
  {
    multiplier = ((*(pb_fpgaMem + FPGA_FRQ_OUT_MODE_AND_MSN) & 0x0f)) << 8;
    multiplier |= *(pb_fpgaMem + FPGA_FRQ_OUT_MULT_LSB);

    return multiplier * k8KHz;
  }
  else
    return 0;
#endif
	return 0;
}

