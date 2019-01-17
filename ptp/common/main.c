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

FILE NAME    : main.c

AUTHOR       : Ken Ho

DESCRIPTION  :

Main run file for SoftClient example.  Contains simple examples of
most commonly used functions.

Revision control header:
$Id: main.c 1.71 2012/03/09 16:02:22PST German Alvarez (galvarez) Exp  $

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
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
#include "sc_types.h"
#include "sc_api.h"
#include "sc_servo_api.h"
#include "sc_chan.h"
#include "../../common/CLK/clk_private.h"
#include "log/sc_log.h"
#include "network_if/sc_network_if.h"
#include "clock_if/sc_clock_if.h"
#include "redundancy_if/sc_redundancy_if.h"
#include "se_if/sc_se_if.h"
#include "local_debug.h"
#include "config_if/sc_readconfig.h"
#include "user_if/sc_ui.h"
#include "../../../drivers/fpga_map.h"
#include "../../../drivers/fpga_ioctl.h"
#include "chan_processing_if/sc_SSM_handling.h"
#include "../../common/DBG/DBG.h" 
#include configFileName
#include "main.h"
#include "clkserv_api.h"
#include "clkserv_priv.h"
#include "sys_defines.h"
#include "../../../Include/mstate_param_def.h"
#include "../../../Mstate/mstated_pub.h"
#include "../../../Clock/clock_api.h"


/*************************************************************************
**    global variables
*************************************************************************/
int		gps_valid = FALSE;
int		bd_valid = FALSE;
int		irigb1_valid = FALSE;
int		irigb2_valid = FALSE;
static  uint8_t update_intpps = 0;

static BOOLEAN o_terminating = FALSE;
static struct itimerval s_itimerIntvPTP;
static BOOLEAN o_synthOn = FALSE;
static int i_fpgaFd = -1;
static  uint8_t local_quality = 0x0f;

t_LocalDebugType s_localDebCfg;
static SC_t_GPS_TOD s_gpsTOD;
//add by Hooky	20140214
static is_normal_crystal = 0;
//add end

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

#define SW2_REG_OFFSET	0x03

#define SE1_PORT  0
#define SE2_PORT  1


/*************************************************************************
**    static function-prototypes
*************************************************************************/
static void RunSoftClock(int signal);
static void StopSoftClock(int signal);
static int CheckSwOption(BOOLEAN o_synth);
static void UpdateLinkStatus();
static int GetChanConfig(char *fname);
static int GetChanStatus(char *fname);
static int GetActiveRef(uint8_t *chan);
static int GetChanValid(uint8_t chan, uint8_t *valid);

/*************************************************************************
**    External functions
*************************************************************************/
extern void SC_SetSynthOn(BOOLEAN o_on);
extern int SC_StartUi(void);
extern int SC_StopUi(void);
extern void SC_ReadVcalFromFile(void);

/*
----------------------------------------------------------------------------
                                PhasorTask()

Description:
This function is the one second thread that runs one minute CLK task.

Parameters:

Inputs:
	arg: not used

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
#if (SC_GPS_CHANNELS > 0) || (SC_SYNCE_CHANNELS > 0) || (SC_SPAN_CHANNELS > 0) || (SC_FREQ_CHANNELS > 0)
#define SECOND_IN_NS 1000000000
#define SEC_IN_WEEK (86400 * 7)

/* (TAI)1970-Jan-1 and (GPS)1980-Jan-8 - 10 year + 2 leap years + 7 days = 3657 days */
/* offset 19 seconds between GPS and TAI */
#define TAI_GPS_OFFSET ((UINT64)(3657 * 86400) + (UINT64)19)
#define PHASOR_PERIOD_US (10000)

static void *PhasorTask(void *arg)
{
   int ret_val;
   INT32 phase;
   UINT8 losCntr;
   UINT8 currLosCntr;
   static   UINT8 prevExtALosCntr = 0;
   static   UINT8 prevExtBLosCntr = 0;
   static   UINT8 prevGpsLosCntr = 0;
   static   UINT8 prevSe1LosCntr = 0;
   static   UINT8 prevSe2LosCntr = 0;

   INT64 ll_GPS;
   INT64 ll_TAI;
   t_ptpTimeStampType s_Time;
	uint32_t sec_val;
	uint8_t reg_val;


   /* open fpga if not opened yet */
   if (i_fpgaFd < 0)
   {
      i_fpgaFd = open("/dev/fpga", O_RDWR);
      if (i_fpgaFd < 0)
      {
         syslog(LOG_SYMM|LOG_DEBUG, "Cannot open fpga device\n");
         return;
      }
	}

//	ioctl(i_fpgaFd, FPGA_INIT_INTPPS, 0);

   while (1)
   {
      /* sleep for 1 second */
#if 0
      usleep(PHASOR_PERIOD_US);
#endif
#if 1
		ioctl(i_fpgaFd, FPGA_WAIT_PPS, 0);
#endif
/* Get Ext A phasor measurement */
#if 0
      if((SC_RED_CHANNELS == 0) || (GetExtAChan() != GetRedundancyChan()))
      { //redundancy channel is handled in sc_redundancy_if.c
        losCntr = SC_GetExtRefPhasorLosCntr();
        currLosCntr = (losCntr & FPGA_LOS_COUNT_EXT_A_MASK) >> FPGA_LOS_COUNT_EXT_A_SHIFT;
        if(currLosCntr != prevExtALosCntr)
        {
          /* get phasor measurement from FPGA */
          SC_GetPhasorOffset(&phase, FPGA_EXT_REF_A_IN_CNT3);

          /* Send phasor measurement to SCi */
          ret_val = SC_TimeMeasToServo(GetExtAChan(), phase, NULL);

          prevExtALosCntr = currLosCntr;
        }
      }
#endif

/* Get Ext B phasor measurement */
#if 0
      if((SC_RED_CHANNELS == 0) || (GetExtBChan() != GetRedundancyChan()))
      { //redundancy channel is handled in sc_redundancy_if.c
        losCntr = SC_GetExtRefPhasorLosCntr();
        currLosCntr = (losCntr & FPGA_LOS_COUNT_EXT_B_MASK) >> FPGA_LOS_COUNT_EXT_B_SHIFT;

        if(currLosCntr != prevExtBLosCntr)
        {
          /* get phasor measurement from FPGA */
          SC_GetPhasorOffset(&phase, FPGA_EXT_REF_B_IN_CNT3);


          /* Send phasor measurement to SCi */
          ret_val = SC_TimeMeasToServo(GetExtBChan(), phase, NULL);

          prevExtBLosCntr = currLosCntr;
        }
      }
#endif

/* Get GPS phase measurement */
#if 0
      losCntr = SC_GetPhasorLosCntr();
      currLosCntr = (losCntr & FPGA_GPS_1PPS_BIT_MASK) >> FPGA_GPS_1PPS_BIT_SHIFT;

      if(currLosCntr != prevGpsLosCntr)
      {
         /* get phasor measurement from FPGA */
         SC_GetPhasorOffset(&phase, FPGA_GPS_IN_CNT3);

         /* get GPS time from GPS engine */
         ll_GPS = ((UINT64)s_gpsTOD.Gps_WeekNo * (UINT64)SEC_IN_WEEK) + (s_gpsTOD.Gps_TOW);
         ll_TAI = ll_GPS + TAI_GPS_OFFSET;

         /* add one second because we are reading GPS time at GPS 1PPS edge, and GPS changes time at around 200-300ms after 1PPS */
         s_Time.u48_sec = ll_TAI + 1;
         s_Time.dw_nsec = 0;

         /* Send GPS phasor measurement to SCi */
         ret_val = SC_TimeMeasToServo(GetGpsChan(), phase, &s_Time);

         prevGpsLosCntr = currLosCntr;
      }
#endif
#if 0
	losCntr = SC_GetPhasorLosCntr();
	 currLosCntr = losCntr & 0x03;
	if(currLosCntr != prevGpsLosCntr){
#endif

#ifdef MAIN_CLOCK
	ioctl(i_fpgaFd, FPGA_RD_PPS_STAT, (uint32_t)&reg_val);	
	if((reg_val & FPGA_GPS_PPS_LOS) == 0){
		SC_GetPhasorOffset(&phase, 0);
		ioctl(i_fpgaFd, FPGA_GET_LOCAL_SEC, (uint32_t)&sec_val);
		ll_GPS = sec_val;
		ll_TAI = ll_GPS + TAI_GPS_OFFSET;
		/*s_Time.u48_sec = ll_TAI + 1;*/
		s_Time.u48_sec = ll_TAI;
		s_Time.dw_nsec = 0;
		ret_val = SC_TimeMeasToServo(GetGps1Chan(), phase, &s_Time);
	}

	ioctl(i_fpgaFd, FPGA_RD_PPS_STAT, (uint32_t)&reg_val);	
	if((reg_val & FPGA_BD_PPS_LOS) == 0){
		SC_GetPhasorOffset1(&phase, 0);
		ioctl(i_fpgaFd, FPGA_GET_LOCAL_SEC, (uint32_t)&sec_val);
		ll_GPS = sec_val;
		ll_TAI = ll_GPS + TAI_GPS_OFFSET;
		/*s_Time.u48_sec = ll_TAI + 1;*/
		s_Time.u48_sec = ll_TAI;
		s_Time.dw_nsec = 0;
		ret_val = SC_TimeMeasToServo(GetGps2Chan(), phase, &s_Time);
	}

//	ioctl(i_fpgaFd, FPGA_RD_PPS_STAT, (uint32_t)&reg_val);	
//	if((reg_val & FPGA_IRIG_OPTI1_PPS_LOS) == 0){
		SC_GetPhasorOffset2(&phase, 0);
		ioctl(i_fpgaFd, FPGA_GET_LOCAL_SEC, (uint32_t)&sec_val);
		ll_GPS = sec_val;
		ll_TAI = ll_GPS + TAI_GPS_OFFSET;
		/*s_Time.u48_sec = ll_TAI + 1;*/
		s_Time.u48_sec = ll_TAI;
		s_Time.dw_nsec = 0;
		ret_val = SC_TimeMeasToServo(GetGps3Chan(), phase, &s_Time);
//	}

//	ioctl(i_fpgaFd, FPGA_RD_PPS_STAT, (uint32_t)&reg_val);	
//	if((reg_val & FPGA_IRIG_OPTI2_PPS_LOS) == 0){
		SC_GetPhasorOffset3(&phase, 0);
		ioctl(i_fpgaFd, FPGA_GET_LOCAL_SEC, (uint32_t)&sec_val);
		ll_GPS = sec_val;
		ll_TAI = ll_GPS + TAI_GPS_OFFSET;
		/*s_Time.u48_sec = ll_TAI + 1;*/
		s_Time.u48_sec = ll_TAI;
		s_Time.dw_nsec = 0;
		ret_val = SC_TimeMeasToServo(GetGps4Chan(), phase, &s_Time);
//	}

#else
//	ioctl(i_fpgaFd, FPGA_RD_PPS_STAT, (uint32_t)&reg_val);	
//	if((reg_val & FPGA_IRIG_OPTI1_PPS_LOS) == 0){
		SC_GetPhasorOffset2(&phase, 0);
		ioctl(i_fpgaFd, FPGA_GET_LOCAL_SEC, (uint32_t)&sec_val);
		ll_GPS = sec_val;
		ll_TAI = ll_GPS + TAI_GPS_OFFSET;
		/*s_Time.u48_sec = ll_TAI + 1;*/
		s_Time.u48_sec = ll_TAI;
		s_Time.dw_nsec = 0;
		ret_val = SC_TimeMeasToServo(GetGps1Chan(), phase, &s_Time);
//	}

//	ioctl(i_fpgaFd, FPGA_RD_PPS_STAT, (uint32_t)&reg_val);	
//	if((reg_val & FPGA_IRIG_OPTI2_PPS_LOS) == 0){
		SC_GetPhasorOffset3(&phase, 0);
		ioctl(i_fpgaFd, FPGA_GET_LOCAL_SEC, (uint32_t)&sec_val);
		ll_GPS = sec_val;
		ll_TAI = ll_GPS + TAI_GPS_OFFSET;
		/*s_Time.u48_sec = ll_TAI + 1;*/
		s_Time.u48_sec = ll_TAI;
		s_Time.dw_nsec = 0;
		ret_val = SC_TimeMeasToServo(GetGps2Chan(), phase, &s_Time);
//	}
#endif

#if 0
	}
#endif
/* Get Sync-E 1 phasor measurement */
#if 0
      currLosCntr = (losCntr & FPGA_SE1_1PPS_BIT_MASK) >> FPGA_SE1_1PPS_BIT_SHIFT;
      if(currLosCntr != prevSe1LosCntr)
      {
         SC_GetPhasorOffset(&phase, FPGA_SE1_IN_CNT3);


         if(GetEth0Chan() != 0xFF)
            ret_val = SC_TimeMeasToServo(GetEth0Chan(), phase, NULL);

         prevSe1LosCntr = currLosCntr;
//         printf("SE1: %d %d %d\n", phase, (int)GetSe1Chan(), (int)ret_val);
      }
#endif

/* Get Sync-E 2 phasor measurement */
#if 0
      currLosCntr = (losCntr & FPGA_SE2_1PPS_BIT_MASK) >> FPGA_SE2_1PPS_BIT_SHIFT;
      if(currLosCntr != prevSe2LosCntr)
      {
         SC_GetPhasorOffset(&phase, FPGA_SE2_IN_CNT3);

         if(GetEth1Chan() != 0xFF)
            ret_val = SC_TimeMeasToServo(GetEth1Chan(), phase, NULL);

         prevSe2LosCntr = currLosCntr;
//         printf("SE2: %d %d %d\n", phase, (int)GetSe2Chan(), (int)ret_val);
      }
#endif
   }

    return 0;
}

/*
----------------------------------------------------------------------------
                                TODtoPhasorTask()

Description:
Pass GPS time into local data variable.

Parameters:

Inputs:
   None

Outputs:
	None

Return value:
   None

-----------------------------------------------------------------------------
*/

void TODtoPhasorTask (
   SC_t_GPS_TOD *ps_gpsTOD
)
{
   /* copy data to user space */
   s_gpsTOD = *ps_gpsTOD;
}

#endif
#if 0
static void *PhasorTask(void *arg)
{
   UINT32 GPS_phase;
   INT32 SE1_phase;
   INT32 SE2_phase;

   while (1)
   {
      /* sleep for 1 second */
      sleep(1);

      /* check the phasor measurments in FPGA */
      SC_GetGpsPhaseOffset(&GPS_phase);
      SC_GetSe1PhaseOffset(&SE1_phase);
      SC_GetSe2PhaseOffset(&SE2_phase);

      //      printf("%d %d\n", GPS_phase, SE_phase);
      /* send phasor measurement to SCi */
/* TBD - Uncomment below lines and insert the appropriate channel index */
//      SC_TimeMeasToServo(0, GPS_phase);
//      SC_TimeMeasToServo(1, SE1_phase);
//      SC_TimeMeasToServo(2, SE2_phase);
   }

    return 0;
}
#endif

/*
----------------------------------------------------------------------------
                                SeTask()

Description:
This function is Sync-E task that takes care of processing Sync-E packets.

Parameters:

Inputs:
	arg: not used

Outputs:
	None

Return value:
   None

-----------------------------------------------------------------------------
*/
static void *SeTask(void *arg)
{
   while (1)
   {
      /* sleep for 1 second */
      sleep(1);

      /* Update link status in FPGA and SoftClock */
      UpdateLinkStatus();

      /* Process link states by handing valid flags to SSM handling unit */
      ProcessLOS();

      /* check the phasor measurments in FPGA */
      ProcessESMC();
   }
   return NULL;
}

/* ************************************************************************/
/**
 * @funtion: update local clock quality thread
 *
 * @quality defined: Normal track: 	0
 * 					 bridge:		2
 * 					 fasttrack:		3
 * 					 holdover:		4(0		<= t < 1h)
 * 					 				5(1h	<= t < 10h)
 * 					 				6(10h	<= t < 24h)
 * 					 				f(t		>= 	24h)
 *
 * @Returns 
 */
/* ************************************************************************/
static	void *QualityTask(void *arg)
{
	uint8_t reg_val;
	uint32_t current_state_dur;
	uint8_t fpga_local_quality;
	uint8_t run_flag = 0;


	while(1){
		sleep(2);
		ioctl(i_fpgaFd, FPGA_GET_LOCAL_QUALITY, (uint32_t)&fpga_local_quality);	

		FLL_STATUS_STRUCT fp;
		get_fll_status(&fp);	
		switch(fp.cur_state){
						
				case FLL_BRIDGE:
					{
						run_flag = 0;
						current_state_dur = fp.current_state_dur;
						if(current_state_dur >= 30)
							if(fpga_local_quality < 4){
								fpga_local_quality = 4;
								ioctl(i_fpgaFd, FPGA_SET_LOCAL_QUALITY, (uint32_t)&fpga_local_quality);	
								syslog(LOG_BSS|LOG_DEBUG, "Bridge fpga_local_quality change to %u\n", fpga_local_quality);	
							}
						break;
					}
	
				case FLL_HOLD:
					{
						current_state_dur = fp.current_state_dur/60;
						if(current_state_dur >= 0 && current_state_dur < 60){
						}
						else if(current_state_dur >= 60 && current_state_dur < 600){
							if(run_flag == 0){
								fpga_local_quality += 1;
								ioctl(i_fpgaFd, FPGA_SET_LOCAL_QUALITY, (uint32_t)&fpga_local_quality);
								syslog(LOG_BSS|LOG_DEBUG, "Holdover(1h<=t<10h) fpga_local_quality change to %u\n", fpga_local_quality);	
								run_flag ++;
							}
						}
						else if(current_state_dur >= 600 && current_state_dur < 1440){
							if(run_flag == 1 ){
								fpga_local_quality += 1;
								ioctl(i_fpgaFd, FPGA_SET_LOCAL_QUALITY, (uint32_t)&fpga_local_quality);
								syslog(LOG_BSS|LOG_DEBUG, "Holdover(10h<=t<24h) fpga_local_quality change to %u\n", fpga_local_quality);		
								run_flag ++;
							}
						}
						else{
							if(run_flag == 2){
								fpga_local_quality = 0x0f;
								ioctl(i_fpgaFd, FPGA_SET_LOCAL_QUALITY, (uint32_t)&fpga_local_quality);	
								syslog(LOG_BSS|LOG_DEBUG, "Holdover(t>24h)fpga_local_quality change to %u\n", fpga_local_quality);	
								run_flag ++;

								//add by Hooky	20140214
								is_normal_crystal = 0;
								//add end
							}
						}
					}
					break;
	
				//add by Hooky	20140214
				case FLL_NORMAL:
					current_state_dur = fp.current_state_dur/60;
					if(current_state_dur > 60 ){
						is_normal_crystal = 1;
					}
					break;
				//add end

				default:
					{
						run_flag = 0;
						break;
					}
		}
	}
}

static	void *DelayTask(void *arg)
{
	uint8_t reg_val;
	uint8_t source_gps,source_bd,source_irigb1,source_irigb2;

	while(1){
		sleep(1);
		if(i_fpgaFd < 0){
			gps_valid = FALSE;
			bd_valid = FALSE;
			irigb1_valid = FALSE;
			irigb2_valid = FALSE;
			continue;
		}
#ifdef MAIN_CLOCK
		ioctl(i_fpgaFd, FPGA_RD_SYS_CTRL, (uint32_t)&reg_val);
		if((reg_val & FPGA_GPS_LOCK_FLAG) != 0){
			gps_valid = TRUE;
		}
		else{
			gps_valid = FALSE;
		}

		ioctl(i_fpgaFd, FPGA_RD_BD_CTRL, (uint32_t)&reg_val);
		if((reg_val & FPGA_BD_LOCK_FLAG) != 0){
			bd_valid = TRUE;
		}
		else{
			bd_valid = FALSE;
		}

		ioctl(i_fpgaFd, FPGA_RD_BD_CTRL, (uint32_t)&reg_val);
		if((reg_val & FPGA_OPTI1_LOCK_FLAG) != 0){
			irigb1_valid = TRUE;
		}
		else{
			irigb1_valid = FALSE;
		}

		ioctl(i_fpgaFd, FPGA_RD_BD_CTRL, (uint32_t)&reg_val);
		if((reg_val & FPGA_OPTI2_LOCK_FLAG) != 0){
			irigb2_valid = TRUE;
		}
		else{
			irigb2_valid = FALSE;
		}
#else
		ioctl(i_fpgaFd, FPGA_RD_BD_CTRL, (uint32_t)&reg_val);
		if((reg_val & FPGA_OPTI1_LOCK_FLAG) != 0){
			gps_valid = TRUE;
		}
		else{
			gps_valid = FALSE;
		}

		ioctl(i_fpgaFd, FPGA_RD_BD_CTRL, (uint32_t)&reg_val);
		if((reg_val & FPGA_OPTI2_LOCK_FLAG) != 0){
			bd_valid = TRUE;
		}
		else{
			bd_valid = FALSE;
		}
#endif

		//printf("gps_valid=%d, bd_valid=%d, irigb1_valid=%d, irigb2_valid=%d\n", gps_valid, bd_valid, irigb1_valid, irigb2_valid);
		if(update_intpps != 100){
			clock_get_source_refvalid(0,&source_gps);
			if(source_gps == 1){
				if((update_intpps%5)==0)
					ioctl(i_fpgaFd, FPGA_UPDATE_INTPPS_GPS_ALIGN, 0);
				//update_intpps = 1;
				update_intpps++;
				if(update_intpps >= 65)
					update_intpps = 100;
				syslog(LOG_BSS|LOG_DEBUG, "FPGA_UPDATE_INTPPS_GPS");
			}
			else{
				clock_get_source_refvalid(1,&source_bd);
				if(source_bd == 1){
					if((update_intpps%5)==0)
						ioctl(i_fpgaFd, FPGA_UPDATE_INTPPS_BD_ALIGN, 0);
					//update_intpps = 1;
					update_intpps++;
					if(update_intpps >= 65)
						update_intpps = 100;
					syslog(LOG_BSS|LOG_DEBUG, "FPGA_UPDATE_INTPPS_BD");
				}
				else{
					clock_get_source_refvalid(2,&source_irigb1);
					if(source_irigb1 == 1){;
					//	if((update_intpps%5)==0)
					//		ioctl(i_fpgaFd, FPGA_UPDATE_INTPPS_IRIGB1_ALIGN, 0);
					//	update_intpps++;
					//	if(update_intpps >= 65)
					//		update_intpps = 100;
					//	syslog(LOG_BSS|LOG_DEBUG, "FPGA_UPDATE_INTPPS_IRIGB1");
					}
					else{
						clock_get_source_refvalid(3,&source_irigb2);
						if(source_irigb2 == 1){;
						//	if((update_intpps%5)==0)
						//		ioctl(i_fpgaFd, FPGA_UPDATE_INTPPS_IRIGB2_ALIGN, 0);
						//	update_intpps++;
						//	if(update_intpps >= 65)
						//		update_intpps = 100;
						//	syslog(LOG_BSS|LOG_DEBUG, "FPGA_UPDATE_INTPPS_IRIGB2");
						}
					}
				}
			}
		}

		
#if 0
		if(gps_valid){
			if(update_intpps == 0){
#ifdef MAIN_CLOCK
				ioctl(i_fpgaFd, FPGA_UPDATE_INTPPS_GPS, 0);
#else
				ioctl(i_fpgaFd, FPGA_UPDATE_INTPPS_IRIGB1, 0);
#endif
				update_intpps = 1;
			}
		}
		else if(bd_valid){
			if(update_intpps == 0){
#ifdef MAIN_CLOCK
				ioctl(i_fpgaFd, FPGA_UPDATE_INTPPS_BD, 0);
#else
				ioctl(i_fpgaFd, FPGA_UPDATE_INTPPS_IRIGB2, 0);
#endif
				update_intpps = 1;
			}
		}
		else if(irigb1_valid){
			if(update_intpps == 0){
				ioctl(i_fpgaFd, FPGA_UPDATE_INTPPS_IRIGB1, 0);
				update_intpps = 1;
			}
		}
		else if(irigb2_valid){
			if(update_intpps == 0){
				ioctl(i_fpgaFd, FPGA_UPDATE_INTPPS_IRIGB2, 0);
				update_intpps = 1;
			}
		}
#endif	
	}
}

/******************************************************************************* 
 * function: clkserv_create_socket
 *
 * description: Create a socket
 *
 ******************************************************************************/
int32_t clkserv_create_socket(const char *sockname, int32_t nconn)
{
    int32_t socket_handle;
    int32_t	len;
    struct sockaddr_un local;

	if((sockname == NULL) || (sockname[0] == '\0')){
		syslog(LOG_BSS|LOG_DEBUG, "%s: invalid socket name", __FUNCTION__);
		return -1;
	}

	if((socket_handle = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        syslog(LOG_BSS|LOG_DEBUG, "%s: failed to create socket", __FUNCTION__);
        return -1;
	}

	local.sun_family = AF_UNIX;
	strncpy(local.sun_path, sockname, 100);
	local.sun_path[99] = '\0';
	unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if(bind(socket_handle, (struct sockaddr*)&local, len) == -1){
        syslog(LOG_BSS|LOG_DEBUG, "%s: failed to bind socket", __FUNCTION__);
        close(socket_handle);
        return -1;
    }

    if(listen(socket_handle, nconn) == -1){
        syslog(LOG_BSS|LOG_DEBUG, "%s: failed to listen socket", __FUNCTION__);
        close(socket_handle);
        return -1;
    }

    return socket_handle;
}


/******************************************************************************* 
 * function: clkserv_proc_packet
 *
 * description: Process a packet received from the socket
 *
 * input: socket descriptor
 *        pointer to the packet buffer
 *
 * output:
 *
 * return: none.
 ******************************************************************************/
static void clkserv_proc_packet(int32_t sock, uint8_t* buff)
{
    char data[SOCKET_BUF_SIZE+1];
	FLL_STATUS_STRUCT fp;
	servo_statics_t cur_servo_statics;
	size_t t_size;
  	uint8_t numChannels = 0;
//	static int64_t prev_offset = 0;
	int retValue;
	uint32_t current_state_dur;

    if(buff == NULL){
		syslog(LOG_BSS|LOG_DEBUG, "Buffer is NULL");
        return;
    }

    switch (buff[0])
    {
        case CMD_GET_STATE:
			get_fll_status(&fp);
			data[0] = CMD_GET_STATE;
			data[1] = 3;
			switch(fp.cur_state){
				case FLL_FAST:
					data[2] = 3;
					break;
				
				case FLL_NORMAL:
					data[2] = 4;
					break;

				case FLL_HOLD:
					data[2] = 5;
					break;

				case FLL_BRIDGE:
					data[2] = 6;
					break;
	
				default:
					data[2] = 1;
					break;
			}
			if(send(sock, data, 3, MSG_NOSIGNAL) == -1){
				syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send state packet to client", __FUNCTION__);
			}
			break;

		case CMD_GET_LOCAL_QUALITY:
			data[0] = CMD_GET_LOCAL_QUALITY;
			data[1] = 3;
			data[2] = (char)local_quality;

			if(send(sock, data, 3, MSG_NOSIGNAL) == -1){
				syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send state packet to client", __FUNCTION__);
			}
			break;
		 
		case CMD_GET_STATICS:
			t_size = sizeof(servo_statics_t);
			get_fll_status(&fp);
			cur_servo_statics.state	= fp.cur_state;
			cur_servo_statics.cur_state_dur = fp.current_state_dur/60;
			cur_servo_statics.freq_corr = fp.freq_cor;
			data[0] = CMD_GET_STATICS;
			data[1] = 2 + t_size;
			memcpy(&data[2], (char*)&cur_servo_statics, t_size);
			if(send(sock, data, (2+t_size), MSG_NOSIGNAL) == -1){
				syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send state packet to client", __FUNCTION__);
			} 
			break;

		case CMD_GET_CHAN_CONFIG:
			{	
				char fname[32];
				data[0] = CMD_GET_CHAN_CONFIG;
				data[1] = 34;
				GetChanConfig(fname);
				memcpy(&data[2],fname, sizeof(fname));
				if(send(sock, data, sizeof(data), MSG_NOSIGNAL) == -1){
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send state packet to client", __FUNCTION__);
				} 
			}
			break;

		case CMD_GET_CHAN_STATUS:
			{
				char fname[32];
				data[0] = CMD_GET_CHAN_STATUS;
				data[1] = 34;
				GetChanStatus(fname);
				memcpy(&data[2],fname, sizeof(fname));
				if(send(sock, data, sizeof(data), MSG_NOSIGNAL) == -1){
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send state packet to client", __FUNCTION__);
				} 
			}
			break;

		case CMD_GET_PHASE_OFFSET:
			{
				int64_t offset=0;
//				int64_t delta_offset = 0;
				data[0] = CMD_GET_PHASE_OFFSET;
				data[1] = 10;
				if(SC_PhaseOffset(&offset, e_PARAM_GET)!= 0)
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to get servo phase offset", __FUNCTION__);
//				delta_offset = offset - prev_offset;
//				prev_offset = offset;
				INT64_TO_CHARS(offset, (data + 2));
				if(send(sock, data, sizeof(data), MSG_NOSIGNAL) == -1){
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send state packet to client", __FUNCTION__);
				} 
			}
			break;

		case CMD_SET_PHASE_OFFSET:
			{
				int64_t offset=0;
				CHARS_TO_64BIT((buff + 2), &offset);
				if(SC_PhaseOffset(&offset, e_PARAM_SET)!= 0)
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to set servo phase offset", __FUNCTION__);
			}
			break;

		case CMD_GET_CHAN_VALID:
			{
				BOOLEAN valid;
				UINT8 chan = buff[2];
				data[0] = CMD_GET_CHAN_VALID;
				data[1] = 3;
				
				if(GetChanValid(chan, &valid) != 0)
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to get chan %d valid status", __FUNCTION__, buff[2]);
				
				data[2] = valid;
				if(send(sock, data, sizeof(data), MSG_NOSIGNAL) == -1){
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send state packet to client", __FUNCTION__);
				} 

			}
			break;

		case CMD_GET_ACT_REF:
			{
				data[0] = CMD_GET_ACT_REF;
				data[1] = 3;
				if(GetActiveRef(&data[2]) == -1){
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to get active reference", __FUNCTION__);
				}

				if(send(sock, data, sizeof(data), MSG_NOSIGNAL) == -1){
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send state packet to client", __FUNCTION__);
				} 
			}
			break;

		case CMD_SET_ACT_REF:
			{
				if(SC_SetSelChan(0, buff[2]) != 0)
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to set time reference channel %d", __FUNCTION__, buff[2]);

				if(SC_SetSelChan(1, buff[2]) != 0)
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to set frequence reference channel %d", __FUNCTION__, buff[2]);
			}
			break;

		case CMD_SET_SWITCH_MODE:
			{			 
				SC_t_swtModeEnum swtMode;
				if(buff[2] == 0)
					swtMode = e_SC_SWTMODE_AR;
				else if(buff[2] == 1)
					swtMode =  e_SC_SWTMODE_OFF;
				else{
					syslog(LOG_BSS|LOG_DEBUG, "%s: invalid mode value %d", __FUNCTION__, buff[2]);
					break;
				}	
				if(SC_ChanSwtMode(e_PARAM_SET, &swtMode) !=0 )
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to set switch mode to %d", __FUNCTION__, buff[2]);
			}
			break;

		case CMD_GET_SWITCH_MODE:
			{ 
				data[0] = CMD_GET_SWITCH_MODE;
 				data[1] = 3;

				SC_t_swtModeEnum swtMode;
				if(SC_ChanSwtMode(e_PARAM_GET, &swtMode) !=0 )
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to set switch mode", __FUNCTION__);
			
				if(swtMode == e_SC_SWTMODE_AR)
					data[2] = 0;
				else if(swtMode == e_SC_SWTMODE_OFF)
					data[2] = 1;

				if(send(sock, data, sizeof(data), MSG_NOSIGNAL) == -1){
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send state packet to client", __FUNCTION__);
				} 

			}
			break;

		case CMD_SET_CHAN_PRIO:
			{			 
				UINT8 chanPriority=(UINT8)buff[4];
				BOOLEAN	isFreq = (BOOLEAN)buff[3]; 
              	retValue=SC_ChanPriority(e_PARAM_SET, buff[2], isFreq, &chanPriority);
              	if(retValue != 0)
					syslog(LOG_BSS|LOG_DEBUG, "%s: failed to set chan%d %s priority with error code %d", __FUNCTION__, buff[2], isFreq?"frequency":"time", retValue);
             }
			break;

		//add by Hooky	20140214
		case CMD_GET_CRYSTAL:
			//get_fll_status(&fp);
			data[0] = CMD_GET_CRYSTAL;
			data[1] = 3;
			data[2] = is_normal_crystal;
			if(send(sock, data, 3, MSG_NOSIGNAL) == -1){
				syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send state packet to client", __FUNCTION__);
			}
			break;
		//add end

		default:
			syslog(LOG_BSS|LOG_DEBUG, "%s: unknown service %d", __FUNCTION__, buff[0]);
			break;
	}
	close(sock);
}

/*******************************************************************************
 * function: daemon_init
 *
 * description: Make the process to run as a daemon
 *
 ******************************************************************************/
static int32_t daemon_init(void)
{
    pid_t pid;
                                                                                
    if ((pid = fork()) < 0)
        return -1;
    else if (pid != 0)
        exit(0);
                                                                                
    setsid();
    chdir("/");
    umask(0);
    return 0;
}
                                                                                
/*
----------------------------------------------------------------------------
                                main()

Description:
This function is the main program.

Parameters:

Inputs
	argc, argv
	Not used

Outputs
	None

Return value:
	None

-----------------------------------------------------------------------------
*/
int main(int argc, char *argv[])
{
  pthread_t s_tenHzThread;
  pthread_t s_phasorThread;
  pthread_t s_SeThread;
  pthread_t s_delayThread;
  pthread_t s_local_qualityThread;
  t_logConfigType s_logCfg;
  UINT16 w_clockRateMultiplier;
  INT8 error;
	int32_t sock;
    int32_t sd;
    int32_t sockaddr_size;
    int32_t recv_size;
    struct sockaddr_un remote;
    uint8_t buff[SOCKET_BUF_SIZE+1];
	int32_t option, debug, foreground;
	struct sched_param sched;
	int32_t ret_val;
	char  val[10];
	uint8_t status;
	int32_t mode;

	SC_t_swtModeEnum swtMode;

    while((option = getopt(argc, argv, "df")) != -1){
        if(option == 'd'){
            debug = 1;
            printf("Running in debug mode ... \n");
        }
        else if(option == 'f'){
            foreground = 1;
            printf("Running in foreground ... \n");
        }
    }

    if(!debug && !foreground){
		if(daemon_init() < 0){
            syslog(LOG_BSS|LOG_DEBUG, "Unable to start clkserv daemon");
            exit(0);
        }
    }

    /* Set process priority level */
    sched.sched_priority = CLKSERVD_PRIO_NUM;
    sched_setscheduler(0, SCHED_FIFO, &sched);

  /* set External Ref dividers to 8k */
  setExtRefDivider(EXTREF_A, 8000);
  setExtRefDivider(EXTREF_B, 8000);

  /* check for warp table for varactor pulled OCXO */
  SC_ReadVcalFromFile();

  /* check option dipswitch setting */
  if (CheckSwOption(o_synthOn) < 0)
    exit(0);

  error = readConfigFromFile();
  if(error != 0)
  {
    printf("SC stopped\n");
    exit(error);
  }

  SC_DbgConfig(&s_logCfg, e_PARAM_GET);

  /* initialize the Sync-E port interface data structure */
  InitSePortStructures();

  /* complete initialization */
  w_clockRateMultiplier = 1; /* timer at tick rate, multiplier = 1 */
	
  ret_val =SC_InitConfigComplete(w_clockRateMultiplier);
  if(ret_val < 0)
  {
	printf("ret_val = %d\n", ret_val);
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

  /* Low priority thread to do some debug status printing/processing (in sc_ui.c) */
  pthread_create(&s_tenHzThread, NULL, tenHzThread, (void *)NULL);

#if (SC_GPS_CHANNELS > 0) || (SC_SYNCE_CHANNELS > 0) || (SC_SPAN_CHANNELS > 0) || (SC_FREQ_CHANNELS > 0)
   /* create thread to process phasor measurements */
  pthread_create(&s_phasorThread, NULL, PhasorTask, (void *)NULL);
#endif

   /* create thread to process Sync-E ESMC protocol packets */
  pthread_create(&s_SeThread, NULL, SeTask, (void *)NULL);

  pthread_create(&s_delayThread, NULL, DelayTask, (void*)NULL);

  pthread_create(&s_local_qualityThread, NULL, QualityTask, (void*)NULL);

  /* start user interface */
#if 0
  SC_StartUi();
#endif
	memset(val,0,sizeof(val));
    ret_val = mstated_get_param(MSTATE_SERVO_SWITCH_MODE, 0, 0, 0, val, &status);

	if((ret_val == 0) && (status == 0)){
    	mode =atoi(val);
		if(mode == 0)
			swtMode = e_SC_SWTMODE_AR;
		else
			swtMode = e_SC_SWTMODE_OFF;
    }
	if(SC_ChanSwtMode(e_PARAM_SET, &swtMode) !=0 ){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to set switch mode to %d", __FUNCTION__, mode);
	}

#if 1
	if((sock = clkserv_create_socket(CLKSERV_SOCKET, CLKSERV_SOCKET_NCONN)) > 0){ 	
		/*Accept external request*/
		sockaddr_size = sizeof(remote);
		while(1){
			sd = accept(sock, (struct sockaddr*)&remote, (socklen_t*)&sockaddr_size);
			if(sd == -1){
				syslog(LOG_BSS|LOG_DEBUG, "Unable to accept incoming connection");
				continue;
			}
			recv_size = recv(sd, buff, SOCKET_BUF_SIZE, 0);
			if(recv_size <= 0){
				close(sd);
				continue;
			}
			clkserv_proc_packet(sd, buff);
		}
	}
	else{
		syslog(LOG_BSS|LOG_DEBUG, "Failed to create server socket");
	}
#endif

  /* stop timer */
  TimerStop();

  // Stop all threads
  if (!o_terminating)
    raise(SIGTERM);

  return 0;
}

/*
----------------------------------------------------------------------------
                                UpdateLinkStatus()

Description:
Get link status from ethernet drivers and communicate link speed to fpga. If no
link a given port update a variable or inform SoftClock of LOS on Sync-E channel.

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
static void UpdateLinkStatus(
)
{
#if 0
   INT32 link_eth0, link_speed_eth0;
   INT32 link_eth1, link_speed_eth1;
   static UINT8* pb_fpgaMem = (UINT8*)0xFFFFFFFF;
   static int i_fpgaFd = -1;

   /* open fpga if not opened yet */
   if (i_fpgaFd < 0)
   {
      i_fpgaFd = open("/dev/fpga", O_RDWR);
      if (i_fpgaFd < 0)
      {
         syslog(LOG_SYMM|LOG_DEBUG, "Cannot open fpga device\n");
         return;
      }
      pb_fpgaMem = (UINT8*)mmap(0, FPGA_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, i_fpgaFd, 0);
      if (pb_fpgaMem == (UINT8*)0xFFFFFFFF)
      {
         close(i_fpgaFd);
         syslog(LOG_SYMM|LOG_DEBUG, "Memory mapping fpga device failed\n");
         return ;
      }
      pb_fpgaMem += FPGA_OFFSET;
   }

   /* check link status and speed on eth0 */
   if (SC_GetLinkStat(SE1_PORT, &link_eth0) == 0)
   {
      /* check returned link status */
      if (link_eth0 == LINK)
      {
         /* check link speed */
         if (SC_GetLinkSpeed(SE1_PORT, &link_speed_eth0) == 0)
         {
            switch (link_speed_eth0)
            {
               case LINK_SPEED_100:
#ifdef SC_NETWORK_IF_DEBUG
                  printf("Link speed 100Mbit, eth%d\n", SE1_PORT);
#endif
                  /* set link speed in FPGA, clear the bit */
	               *(pb_fpgaMem + FPGA_LINK_SPEED) &= ~FPGA_LINK_SPEED_ETH0_MASK;
                  break;
               case LINK_SPEED_10:
#ifdef SC_NETWORK_IF_DEBUG
                  printf("Link speed 10Mbit, eth%d\n", SE1_PORT);
#endif
                  /* set link speed in FPGA, set the bit */
	               *(pb_fpgaMem + FPGA_LINK_SPEED) |= FPGA_LINK_SPEED_ETH0_MASK;
                  break;
               default:
#ifdef SC_NETWORK_IF_DEBUG
                  printf("Link speed none, eth%d\n", SE1_PORT);
#endif
                  /* set link speed in FPGA, clear the bit */
	               *(pb_fpgaMem + FPGA_LINK_SPEED) &= ~FPGA_LINK_SPEED_ETH0_MASK;
                  break;
            }
         }

         /* update LOS status variable */
         setSE1los(FALSE);
      }
      else
      {
#ifdef SC_NETWORK_IF_DEBUG
         printf("No link, eth%d\n", SE1_PORT);
#endif
         /* set link speed in FPGA, no link default rate is 100Mbit, clear the bit */
	      *(pb_fpgaMem + FPGA_LINK_SPEED) &= ~FPGA_LINK_SPEED_ETH0_MASK;

         /* update LOS status variable */
         setSE1los(TRUE);
      }
   }

   /* check link status and speed on eth1 */
   if (SC_GetLinkStat(SE2_PORT, &link_eth1) == 0)
   {
      /* check returned link status */
      if (link_eth1 == LINK)
      {
         /* check link speed */
         if (SC_GetLinkSpeed(SE2_PORT, &link_speed_eth1) == 0)
         {
            switch (link_speed_eth1)
            {
               case LINK_SPEED_1000:
#ifdef SC_NETWORK_IF_DEBUG
                  printf("Link speed 1000Mbit, eth%d\n", SE2_PORT);
#endif
                  /* set link speed in FPGA */
                  /* clear the bits */
	               *(pb_fpgaMem + FPGA_LINK_SPEED) &= ~FPGA_LINK_SPEED_ETH1_MASK;
                  break;
               case LINK_SPEED_100:
#ifdef SC_NETWORK_IF_DEBUG
                  printf("Link speed 100Mbit, eth%d\n", SE2_PORT);
#endif
                  /* set link speed in FPGA */
                  /* clear the bits */
	               *(pb_fpgaMem + FPGA_LINK_SPEED) &= ~FPGA_LINK_SPEED_ETH1_MASK;
                  /* set the bits */
	               *(pb_fpgaMem + FPGA_LINK_SPEED) |= (1 << 1);
                  break;
               case LINK_SPEED_10:
#ifdef SC_NETWORK_IF_DEBUG
                  printf("Link speed 10Mbit, eth%d\n", SE2_PORT);
#endif
                  /* set link speed in FPGA */
                  /* clear the bits */
	               *(pb_fpgaMem + FPGA_LINK_SPEED) &= ~FPGA_LINK_SPEED_ETH1_MASK;
                  /* set the bits */
	               *(pb_fpgaMem + FPGA_LINK_SPEED) |= (2 << 1);
                  break;
               default:
#ifdef SC_NETWORK_IF_DEBUG
                  printf("Link speed none, eth%d\n", SE2_PORT);
#endif
                  /* set link speed in FPGA */
                  /* clear the bits */
	               *(pb_fpgaMem + FPGA_LINK_SPEED) &= ~FPGA_LINK_SPEED_ETH1_MASK;
                  /* set the bits */
	               *(pb_fpgaMem + FPGA_LINK_SPEED) |= (1 << 1);
                  break;
            }
         }

         /* update LOS status variable */
         setSE2los(FALSE);
      }
      else
      {
#ifdef SC_NETWORK_IF_DEBUG
         printf("No link, eth%d\n", SE2_PORT);
#endif
         /* set link speed in FPGA, no link default rate is 100Mbit */
         /* clear the bits */
	      *(pb_fpgaMem + FPGA_LINK_SPEED) &= ~FPGA_LINK_SPEED_ETH1_MASK;
         /* set the bits */
	      *(pb_fpgaMem + FPGA_LINK_SPEED) |= (1 << 1);

         /* update LOS status variable */
         setSE2los(TRUE);
      }
   }
#ifdef SC_NETWORK_IF_DEBUG
   printf("Link speed reg value is 0x%X\n", *(pb_fpgaMem + FPGA_LINK_SPEED));
   printf("\n");
#endif

#endif
}

/*************************************************************************
**    static functions
*************************************************************************/

/*
----------------------------------------------------------------------------
                                RunSoftClock()

Description:
Timer service routine for the SoftClock

Parameters:

Inputs
	int signal
	Signal to handle

Outputs
	None

Return value:
	None

-----------------------------------------------------------------------------
*/
static void RunSoftClock(int signal)
{
   if(signal == SIGALRM)
   {
      SC_Run();
   }
}

/*
----------------------------------------------------------------------------
                                StopSoftClock()

Description:
Signal handler that stops the SoftClock on termination and other stop signals

Parameters:

Inputs
	int signal
	Signal to handle

Outputs
	None

Return value:
	None

-----------------------------------------------------------------------------
*/
static void StopSoftClock(int signal)
{
   /* stop SoftClock task */
   o_terminating = TRUE;
   SC_Shutdown();
   SC_StopUi();
}

/*
----------------------------------------------------------------------------
                                TimerStart()

Description:
This function

Parameters:

Inputs
	UINT16 w_multiplier

Outputs
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int TimerStart(UINT16 w_multiplier)
{
   struct timespec s_res;
   UINT32 dw_timeResUsec;

   if (clock_getres(CLOCK_REALTIME, &s_res) < 0)
      return -1;
   dw_timeResUsec = s_res.tv_nsec * w_multiplier / 1000;

   /* register callback function for calling SoftClock */
   signal(SIGALRM, RunSoftClock);

   /* register callback function to terminate the SoftClock */
   signal(SIGINT,  StopSoftClock);
   /*signal(SIGTERM, StopSoftClock);*/
   signal(SIGHUP,  StopSoftClock);

   /* get a timer for calling the SoftClock */
   getitimer(ITIMER_REAL,&s_itimerIntvPTP);
   /* set dt of calling SoftClock in microseconds */
   s_itimerIntvPTP.it_interval.tv_sec = 0;
	dw_timeResUsec = 4000;
   s_itimerIntvPTP.it_interval.tv_usec = dw_timeResUsec - 1;
   s_itimerIntvPTP.it_value.tv_sec = 0;
   s_itimerIntvPTP.it_value.tv_usec = 1;

   /* start timer for the SoftClock */
   if (setitimer(ITIMER_REAL,&s_itimerIntvPTP,NULL))
   {
      /* print error on screen and stop the stack */
      printf("Timer cannot be initialized\n");
      return -1;
   }
   return 0;
}

/*
----------------------------------------------------------------------------
                                TimerStop()

Description:
This function stops the timer.

Parameters:

Inputs
	None

Outputs
	None

Return value:
	None

-----------------------------------------------------------------------------
*/
void TimerStop(void)
{
   /* stop the timer */
   s_itimerIntvPTP.it_value.tv_sec = 0;
   s_itimerIntvPTP.it_value.tv_usec = 0;
   if (setitimer(ITIMER_REAL,&s_itimerIntvPTP,NULL))
   {
      printf("SoftClock timer could not be stopped\n");
   }
}

/*
----------------------------------------------------------------------------
                                CheckSwOption()

Description:
This function checks the hardware dipswitch SW2 by reading FPGA register.
If the dipswitch is not set properly, return error.

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
static int CheckSwOption(BOOLEAN o_synth)
{
   int i_fpgaFd;
   UINT8* pb_fpgaMem;
   UINT8 option;
#if 0
   i_fpgaFd = open("/dev/fpga", O_RDWR);
   if (i_fpgaFd < 0)
   {
      printf("Cannot open fpga device\n");
      return -1;
   }
   pb_fpgaMem = (UINT8*)mmap(0, FPGA_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, i_fpgaFd, 0);
   if (pb_fpgaMem == (UINT8*)0xFFFFFFFF)
   {
      printf("Memory mapping fpga device failed\n");
      close(i_fpgaFd);
      return -1;
   }
   option = *(pb_fpgaMem+FPGA_OFFSET+SW2_REG_OFFSET) & 0x07;
   munmap(0, FPGA_SIZE);
   close(i_fpgaFd);
#endif

#if 0
   if ((o_synth && option != 0x04) ||
       (!o_synth && option != 0x04 && option != 0x05))
   {
      printf("Dipswitch SW2 not set properly\n");
      return -1;
   }
#endif
   return 0;
}

/*
----------------------------------------------------------------------------
  UINT32 diff_usec(const struct timespec start, const struct timespec end)

Description:
Calculate the difference between start and end.
End must be greater than start, otherwise 0 is returned.
NOTE: returned difference is in microseconds,
      input timespec is in seconds and nanoseconds.

Parameters:

Inputs
  start, end

Outputs
  None

Return value:
  difference in microseconds

Global variables affected and border effects:
  Read
    none
  Write
    none

-----------------------------------------------------------------------------
*/
UINT32 diff_usec(const struct timespec start, const struct timespec end)
{
  struct  timespec  result ;

  /* Subtract the second time from the first. */
  if ((end.tv_sec < start.tv_sec) || ((end.tv_sec == start.tv_sec) &&
     (end.tv_nsec <= start.tv_nsec)))
  { /* end <= start? */
    result.tv_sec = result.tv_nsec = 0;
  }
  else
  { /* end > start */
    result.tv_sec = end.tv_sec - start.tv_sec ;
    if (end.tv_nsec < start.tv_nsec)
    { /* Borrow a second. */
      result.tv_nsec = end.tv_nsec + 1000000000L - start.tv_nsec;
      result.tv_sec--;
    }
    else
    {
      result.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
  }

  //return (result);
  return (result.tv_sec * 1000000) + (result.tv_nsec / 1000);
}
#ifndef MAX_FILE_INDEX
#define MAX_FILE_INDEX		10
#endif
#define CHAN_CONFIG_FILE	"/tmp/.chan_config"
static chan_config_file_ind = 0;
static int GetChanConfig(char *fname)
{
	FILE* fp;
	int ret;
	int i;
	int j;
	char strBuffer[256];
 	SC_t_ChanConfig chanConfig[SC_TOTAL_CHANNELS];
  	memset(&chanConfig, 0, sizeof(chanConfig));
 	SC_t_selModeEnum chanSelectionMode;
 	SC_t_swtModeEnum chanSwitchMode;
 	UINT8 numChannels = 0;

    memset(&chanConfig, 0, sizeof(chanConfig));
    ret = SC_GetChanConfig(chanConfig, &chanSelectionMode, &chanSwitchMode, &numChannels);
	if(ret != 0){
		syslog(LOG_BSS|LOG_DEBUG, "%s: SC_GetchanConfig failed", __FUNCTION__);
		return -1;
	}

#ifndef MAIN_CLOCK
	numChannels = numChannels - 2;
#endif

	sprintf(fname, "%s%d", CHAN_CONFIG_FILE, chan_config_file_ind);
	fp = fopen(fname, "w");
	if (fp == NULL)
		return -1;

     fprintf(fp, "Number of reference channels: %d\n", numChannels);
     fprintf(fp, "Channel selection mode: ");
	  switch(chanSelectionMode)
	  {
		case e_SC_CHAN_SEL_PRIO:
		  fprintf(fp, "CHAN_SEL_PRIO\n");
		break;

		case e_SC_CHAN_SEL_QL:
		   fprintf(fp, "e_SC_CHAN_SEL_QL\n");
		break;

		default:
		  fprintf(fp, "Unexpected value (%d)\n", chanSelectionMode);
		break;
	  }
	  fprintf(fp, "Channel Switch mode: ");
	  switch(chanSwitchMode)
	  {
		case e_SC_SWTMODE_AR:
		  fprintf(fp, "Auto\n");
		break;

		case e_SC_SWTMODE_AS:
		   fprintf(fp, "Auto\n");
		break;

		case e_SC_SWTMODE_OFF:
		   fprintf(fp, "Manual\n");
		break;

		default:
		  fprintf(fp,"Unexpected value (%d)\n", chanSwitchMode);
		break;
	  }
	  fprintf(fp, "+-----------------------------------------------------+\n");
	  fprintf(fp, "|# |Type        | FP | F | TP | T | AQL | FAQL | TAQL |\n");
	  fprintf(fp, "+-----------------------------------------------------+\n");
	  for(i=0; i<numChannels; i++)
	  {
//		ret=chanTypeToString(strBuffer, sizeof(strBuffer), chanConfig[i].e_ChanType);
#ifdef MAIN_CLOCK
		if(i ==0)
			strcpy(strBuffer, "GPS");
		else if(i ==1)
			strcpy(strBuffer, "Beidou");
		else if(i == 2)
			strcpy(strBuffer, "IRIG-B1");
		else if(i == 3)
			strcpy(strBuffer, "IRIG-B2");
#else
		if(i == 0)
			strcpy(strBuffer, "IRIG-B1");
		else if(i == 1)
			strcpy(strBuffer, "IRIG-B2");
#endif
//		if(ret != 0)
//		  snprintf(strBuffer, sizeof(strBuffer), "%d", chanConfig[i].e_ChanType);

		fprintf(fp, "|%2d|%-12s", i, strBuffer);

		fprintf(fp, "| %2d | %c | %2d | %c ", chanConfig[i].b_ChanFreqPrio, trueFalseIntToChar(chanConfig[i].o_ChanFreqEnabled),
			chanConfig[i].b_ChanTimePrio, trueFalseIntToChar(chanConfig[i].o_ChanTimeEnabled));

		fprintf(fp, "|  %c  |  %2d  |  %2d  ", trueFalseIntToChar(chanConfig[i].o_ChanAssumedQLenabled), chanConfig[i].b_ChanFreqAssumedQL,
			chanConfig[i].b_ChanTimeAssumedQL);

	//	fprintf(fp, "| %6d", chanConfig[i].l_MeasRate);

		fprintf(fp, "|\n");

	  }

	  fprintf(fp, "+-----------------------------------------------------+\n");

	fclose(fp);

	chan_config_file_ind++;
	if (chan_config_file_ind >= MAX_FILE_INDEX)
		chan_config_file_ind = 0;
	
	return 0;
}

#define CHAN_STATUS_FILE	"/tmp/.chan_status"
static chan_status_file_ind = 0;
static int GetChanStatus(char *fname)
{
	FILE* fp;
	int ret;
	int i;
	int8_t j;
	char strBuffer[256];
	const char *kOK_string = "OK";
	const char *kFault_string = "FLT";
	const char *kDisabled_string = "DIS";
	uint8_t irigin_status;
 	SC_t_ChanConfig chanConfig[SC_TOTAL_CHANNELS];
  	memset(&chanConfig, 0, sizeof(chanConfig));
 	UINT8 numChannels = 0;
  	SC_t_chanStatus chanStatus[SC_TOTAL_CHANNELS];

    //get the number of channels
    ret = SC_GetChanConfig(NULL, NULL, NULL, &numChannels);
    if(ret != 0)
    {
		syslog(LOG_BSS|LOG_DEBUG, "%s: SC_GetchanConfig failed", __FUNCTION__);
    	return -1;
    }
	memset(&chanStatus, 0, sizeof(chanStatus));
	ret = SC_GetChanStatus(numChannels, chanStatus);
	if(ret != 0){
		syslog(LOG_BSS|LOG_DEBUG, "%s: SC_GetchanStatus failed", __FUNCTION__);
		return -1;
	}

#ifndef MAIN_CLOCK
	numChannels = numChannels - 2;
#endif

	sprintf(fname, "%s%d", CHAN_CONFIG_FILE, chan_config_file_ind);
	fp = fopen(fname, "w");
	if (fp == NULL)
		return -1;

	fprintf(fp, "Number of reference channels: %d\n", numChannels);
	fprintf(fp, "+------------------------------------------------------------+\n");
	fprintf(fp, "|# |  Signal    |  Frequency  |    Time     |  QL   | Signal |\n");
	fprintf(fp, "|  |  Type      | ST | Weight | ST | Weight | F | T | Valid  |\n");
	fprintf(fp, "+------------------------------------------------------------+\n");
	for(i=0; i<numChannels; i++)
	{
//		if( i==0 || i == 1 /*  || i == 2*/){
			#ifdef MAIN_CLOCK
			if(i == 0)
				strcpy(strBuffer, "GPS");
			else if(i == 1)
				strcpy(strBuffer, "Beidou");
			else if(i == 2)
				strcpy(strBuffer, "IRIG-B1");
			else if(i == 3)
				strcpy(strBuffer, "IRIG-B2");
			#else
			if(i ==0)
				strcpy(strBuffer, "IRIG-B1");
			else if(i == 1)
				strcpy(strBuffer, "IRIG-B2");
			#endif
			fprintf(fp, "|%2d|%-12s", i, strBuffer);

			switch(chanStatus[i].e_chanFreqStatus)
			{
			  case e_SC_CHAN_STATUS_OK: strcpy(strBuffer, kOK_string); break;
			  case e_SC_CHAN_STATUS_FLT: strcpy(strBuffer, kFault_string); break;
			  case e_SC_CHAN_STATUS_DIS: strcpy(strBuffer, kDisabled_string); break;
			  default: sprintf(strBuffer, "%d", chanStatus[i].e_chanFreqStatus); break;
			}
			fprintf(fp, "|%-3s |  %3d   ", strBuffer, chanStatus[i].b_freqWeight);

			switch(chanStatus[i].e_chanTimeStatus)
			{
			  case e_SC_CHAN_STATUS_OK: strcpy(strBuffer, kOK_string); break;
			  case e_SC_CHAN_STATUS_FLT: strcpy(strBuffer, kFault_string); break;
			  case e_SC_CHAN_STATUS_DIS: strcpy(strBuffer, kDisabled_string); break;
			  default: sprintf(strBuffer, "%d", chanStatus[i].e_chanTimeStatus); break;
			}
			fprintf(fp, "|%-3s |  %3d   ", strBuffer, chanStatus[i].b_timeWeight);

			fprintf(fp, "|%2d%1c", chanStatus[i].b_freqQL, (chanStatus[i].b_qlReadExternally)?'*':' ');
			fprintf(fp, "|%2d%1c", chanStatus[i].b_timeQL, (chanStatus[i].b_qlReadExternally)?'*':' ');
			fprintf(fp, "|  %-3s   ",(chanStatus[i].l_faultMap & 0x02)?kFault_string:kOK_string),
			fprintf(fp, "|\n");
//		}

	}
	fprintf(fp, "+------------------------------------------------------------+\n");
	fclose(fp);

	chan_status_file_ind++;
	if (chan_status_file_ind >= MAX_FILE_INDEX)
		chan_status_file_ind = 0;
	
	return 0;
}

static int GetActiveRef(uint8_t *chan)
{
	int ret;
	uint8_t i = 0;
	uint8_t j = 0;

 	SC_t_ChanConfig chanConfig[SC_TOTAL_CHANNELS];
  	memset(&chanConfig, 0, sizeof(chanConfig));
 	UINT8 numChannels = 0;
  	SC_t_chanStatus chanStatus[SC_TOTAL_CHANNELS];

    //get the number of channels
    ret = SC_GetChanConfig(NULL, NULL, NULL, &numChannels);
    if(ret != 0)
    {
		syslog(LOG_BSS|LOG_DEBUG, "%s: SC_GetchanConfig failed", __FUNCTION__);
    	return -1;
    }
	memset(&chanStatus, 0, sizeof(chanStatus));
	ret = SC_GetChanStatus(numChannels, chanStatus);
	if(ret != 0){
		syslog(LOG_BSS|LOG_DEBUG, "%s: SC_GetchanStatus failed", __FUNCTION__);
		return -1;
	}

#ifndef MAIN_CLOCK
	numChannels = numChannels - 2;
#endif

	for(i=0; i<numChannels; i++)
	{
		if(chanStatus[i].b_freqWeight == 100 && chanStatus[i].b_timeWeight == 100){
			*chan = i;
			break;
		}
	}
	if(i == numChannels){
		for(j=0; j < numChannels; j++){
			if((chanStatus[j].l_faultMap & 0x02) == 0){
				*chan = j;
				break;
			}
		}
	}
	if(j == numChannels){
		*chan = 4; //no active reference, set to sys
	}

	return 0;
}

static int GetChanValid(uint8_t chan, uint8_t *valid)
{
	int ret;
	uint8_t i;

 	SC_t_ChanConfig chanConfig[SC_TOTAL_CHANNELS];
  	memset(&chanConfig, 0, sizeof(chanConfig));
 	UINT8 numChannels = 0;
  	SC_t_chanStatus chanStatus[SC_TOTAL_CHANNELS];

    //get the number of channels
    ret = SC_GetChanConfig(NULL, NULL, NULL, &numChannels);
    if(ret != 0)
    {
		syslog(LOG_BSS|LOG_DEBUG, "%s: SC_GetchanConfig failed", __FUNCTION__);
    	return -1;
    }
	memset(&chanStatus, 0, sizeof(chanStatus));
	ret = SC_GetChanStatus(numChannels, chanStatus);
	if(ret != 0){
		syslog(LOG_BSS|LOG_DEBUG, "%s: SC_GetchanStatus failed", __FUNCTION__);
		return -1;
	}

#ifndef MAIN_CLOCK
	numChannels = numChannels - 2;
#endif

	if((chanStatus[chan].l_faultMap & 0x02) != 0)
		*valid = 0;
	else
		if((chanStatus[chan].e_chanFreqStatus == e_SC_CHAN_STATUS_OK) && (chanStatus[chan].e_chanTimeStatus == e_SC_CHAN_STATUS_OK))
			*valid = 1;
		else 
			*valid = 2;
	
	return 0;
}
