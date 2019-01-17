
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

FILE NAME    : sc_chan.c

AUTHOR       : Ken Ho
DESCRIPTION  :


Revision control header:
$Id: CLK/sc_chan.c 1.73 2012/03/17 13:38:45PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
//#define TEMPERATURE_ON
#ifndef TP500_BUILD
#include "proj_inc.h"
#include "target.h"
#include "sc_ptp_servo.h"
#include "sc_servo_api.h"
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "DBG/DBG.h"
#include "sc_alarms.h"
#include "logger.h"
#include "datatypes.h"
#include "ptp.h"
#include "CLKflt.h"
#include "CLKpriv.h"
#include "clk_private.h"
#include "API/sc_system.h"
#include "GPS/GPS.h"
#include configFileName

#else

#include "proj_inc.h"
#include <mqx.h>
#include <bsp.h>
#include <rtcs.h>
#include <enet.h>
#include "rtcsdemo.h"
#include <ctype.h>
#include <string.h>
#include "epl.h"
#include "m523xevb.h"
#include "ptp.h"
#include "clk_private.h"
#include "var.h"
#include "math.h"
#include "logger.h"
#include "pps.h"
#include "sc_ptp_servo.h"
#include "sock_manage.h"
#include "CLKpriv.h"
#include "sc_api.h"
#include "temperature.h"
#include "sc_system.h"
#endif

#include "sc_chan.h"
#include "sc_chan_local.h"

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/

#if (NO_PHY == 1) && (SC_TOTAL_CHANNELS > SC_PTP_CHANNELS)
#error If NO_PHY = 1, only PTP channels are supported
#endif

/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
static SC_t_activeStandbyEnum e_redStatusLocal;
static BOOLEAN o_redEnabledLocal;
static int i_redundantChan;
static int i_redundantPql;
static int i_redChan;
static int i_redPql;
static int b_localChanCount = 0;
static BOOLEAN o_ChanConfigured = FALSE;
static SC_t_ChanConfig s_ChanConfig[SC_TOTAL_CHANNELS];

static UINT8 as_userChan2servoIndexTbl[SC_TOTAL_CHANNELS];

static t_servoChanAcceptType s_servoChanAccept;
static SC_t_servoChanSelectTableType s_servoChanSelectTable;
static UINT32 dw_servoChanCntr = 0;
static SC_t_swtModeEnum e_swtMode = e_SC_SWTMODE_AR;
static SC_t_selModeEnum e_selMode = e_SC_CHAN_SEL_PRIO;

/* JJW */
/* we put here, it may alter merged into main structure, but put here just for development */
typedef struct
{
  int   ssm_ret; 		/* 0=success, -1=failed */
  UINT8   ssm_value; 	/* 8-bit value */
  BOOLEAN ssm_ok;  		/* TRUE=ok, FALSE=not */
  int	valid_ret; 		/* 0=success, -1=failed */
  BOOLEAN valid_ok; 	/* TRUE=ok, FALSE=not */
  BOOLEAN signal_ok;  	/* TRUE=ok, FALSE=not */
} SC_chanStat2Type;

static SC_chanStat2Type s_chanStat2Tbl[SC_TOTAL_CHANNELS];

#define kREF_PRI_STABLE_CNT 2
#define kREF_STABLE_CNT 2

#if 0
static int prevChan_t=-1;
static int prevChan_f=-1;
#endif
static int currChan_t = -1;
static int currChan_f = -1;
static int usrChanFlag_f;
static int usrChanFlag_t;
static int usrChan_f;
static int usrChan_t;

static int currPql_f=-1;
static int currPql_t=-1;


static int Get_AS_OFF_Ref(BOOLEAN is_freq, int *selectedChan);
static int Get_AS_ON_Ref(BOOLEAN is_freq, int *selectedChan);
static int Get_AR_ON_Ref(BOOLEAN is_freq, int *selectedChan);

static void update_chan_valid_status(int chan);
static void update_chan_ssm_status(int chan);
static void update_chan_signal_status(int chan);
static BOOLEAN GetInpChanValid(int chan);
static BOOLEAN GetInpValid(int chan);
static BOOLEAN IsPhaseGood(int chan);
static BOOLEAN GetPerfAcceptable(int chan);

static int GetInpPql(int chan, BOOLEAN is_freq, BOOLEAN *readExternally);
static int GetMyModulePql(void);
static int GetChanType(int chan);
static BOOLEAN GetInpEnab(int chan, BOOLEAN is_freq);
static int GetInpPrio(int chan, BOOLEAN is_freq);
#if 0
static int GetPrevChan(BOOLEAN is_freq);
static void SetPrevChan(int chan, BOOLEAN is_freq);
#endif
static void SetCurrChan(int chan, BOOLEAN is_freq);
static int GetUsrChan(BOOLEAN is_freq);
static void SetUsrChan(int chan, BOOLEAN is_freq);
static int GetUsrChanFlag(BOOLEAN is_freq);
static void ClrUsrChanFlag(BOOLEAN is_freq);
static int FindSsm2Pql(UINT8 chanType, UINT8 ssm);
static void UpdateValidReportToServo(void);
//static void GetRedChan(int *pi_redundantChan, int *pi_redundantPql);
int GetEventBitmap(int chan, UINT32 *bitmap);
void printChanStatus(void);



/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

/*
----------------------------------------------------------------------------
                                GetNumberOfChan()
Description:
This function will return number of channels configured

Parameters:

Inputs
        None

Outputs:
        None

Return value:
         Number of channels configured.

-----------------------------------------------------------------------------
*/
UINT8 GetNumberOfChan(void)
{
  return b_localChanCount;
}
/*
----------------------------------------------------------------------------
                                UserChan2ServoIndex()
Description:
This function will translate a user channel number into a servo index
number

Parameters:

Inputs
        UINT8 b_userChan
            User index number to translate

Outputs:
        None

Return value:
         0xFF - user index number not valid
         0x00-0xFE - servo index number

-----------------------------------------------------------------------------
*/

UINT8 UserChan2ServoIndex(UINT8 b_userChan)
{
  if(b_userChan >= b_localChanCount)
  {
	printf("%s: userChan %d localChanCount %d\n", __FUNCTION__, b_userChan, b_localChanCount);
    return 0xFF;
  }

  return as_userChan2servoIndexTbl[b_userChan];
}

/*
----------------------------------------------------------------------------
                                ServoIndex2UserChan()
Description:
This function will translate a servo index number into a user channel
number

Parameters:

Inputs
        UINT8 b_servoIndex
            Servo index number to translate

Outputs:
        None

Return value:
         0xFF - user index number not valid
         0x00-0xFE - user channel number

-----------------------------------------------------------------------------
*/
UINT8 ServoIndex2UserChan(UINT8 b_servoIndex)
{
  int i;

  for(i=0;i<b_localChanCount;i++)
  {
    if(as_userChan2servoIndexTbl[i] == b_servoIndex)
    {
      return i;
    }
  }
  return 0xFF;
}

static int Init_chanSelect(void)
{
   int i;

   for(i=0;i<NUM_OF_SERVO_CHAN;i++)
   {
   	s_servoChanSelectTable.s_servoChanSelect[i].o_enableChan = TRUE;
   	s_servoChanSelectTable.s_servoChanSelect[i].o_valid = 0;
   	s_servoChanSelectTable.s_servoChanSelect[i].w_measRate = 1;
   	s_servoChanSelectTable.s_servoChanSelect[i].b_selectedFreq = 0;
   	s_servoChanSelectTable.s_servoChanSelect[i].b_selectedTime = 0;
      s_servoChanAccept.b_accepted[i] = TRUE; /* start with all performance acceptable */
   }
   s_servoChanSelectTable.s_servoChanSelect[0].w_measRate = 64;

//   s_servoChanSelectTable.s_servoChanSelect[0].b_selectedFreq = 1;
//   s_servoChanSelectTable.s_servoChanSelect[0].b_selectedTime = 1;
//   s_servoChanSelectTable.s_servoChanSelect[0].o_valid = 1;

   /* JJW, initial the internal data structure */
   /* the runtime routine will put the right value */
   /* TBD */
   for (i = 0; i < (SC_TOTAL_CHANNELS); i++)
   {
	   s_chanStat2Tbl[i].ssm_ret=-1;
	   s_chanStat2Tbl[i].ssm_value=0;
	   s_chanStat2Tbl[i].ssm_ok=0;
	   s_chanStat2Tbl[i].valid_ret=-1;
	   s_chanStat2Tbl[i].valid_ok=0;
	   s_chanStat2Tbl[i].signal_ok=0;
   }

   return 0;
}

/* JJW, please use the interface routine to get get/put into local data structure */
/* So it will be easier to change tlater on */
/* called the SC_ChanValid() and place the result into data structure */
static void update_chan_valid_status(int chan)
{
	int ret_code;
	BOOLEAN valid;
   BOOLEAN o_gps_valid;

	if (!GetInpChanValid(chan))
	{
		return;
	}

	ret_code = SC_ChanValid(chan, &valid);

//   printf("SC_ChanValid() chan: %d valid: %d\n", (int)chan, (int)valid);

   /* reply depends of reference type */
   switch(GetChanType(chan))
   {
   case e_SC_CHAN_TYPE_GPS:
#ifdef GPS_BUILD
      /* GPS valid from 3D fix */
      o_gps_valid = GPS_Valid(chan);

    printf("SC_ChanValid() chan: %d o_gps_valid: %d\n", (int)chan, (int)o_gps_valid);

#else
      /* set to TRUE, real valid flag will be acquired through SC_ChanValid() */
      o_gps_valid = TRUE;
#endif
      valid = valid && o_gps_valid;
      break;
   case e_SC_CHAN_TYPE_SE:
   case e_SC_CHAN_TYPE_PTP:
   default:
      /* do nothing */
      break;
   }
	s_chanStat2Tbl[chan].valid_ret=ret_code;
	s_chanStat2Tbl[chan].valid_ok=valid;
    printf("SC_ChanValid() chan: %d valid: %d\n", (int)chan, (int)valid);

   return;
}

/* called the SC_ChanSSM() and place the result into data structure */
static void update_chan_ssm_status(int chan)
{
	int ret_code;
	BOOLEAN valid;
	UINT8 ssm;

	if (!GetInpChanValid(chan))
	{
		return;
	}

	ret_code = SC_ChanSSM(chan, &ssm, &valid);
	s_chanStat2Tbl[chan].ssm_ret = ret_code;
	s_chanStat2Tbl[chan].ssm_value =ssm;
	s_chanStat2Tbl[chan].ssm_ok = valid;
}

/* you shall get this info from xx, such as LOS */
/* *** */
static void update_chan_signal_status(int chan)
{
   UINT8 b_servo_index;

	if (!GetInpChanValid(chan))
	{
		return;
	}

   /* convert to servo index */
   b_servo_index = UserChan2ServoIndex(chan);

   if(b_servo_index == 0xFF)
   {
      return;
   }

   /* check servo for los */
//   s_chanStat2Tbl[chan].signal_ok = s_servoChanSelectTable.s_servoChanSelect[b_servo_index].o_valid;
// KJH don't use o_valid, get directly from los report

//   printf("chan= %d signal_ok= %d\n", (int)chan, (int)s_chanStat2Tbl[chan].signal_ok);
	/* JJW: TBD */
//	s_chanStat2Tbl[chan].signal_ok=TRUE; /* put ok for now, shall change later */

   return;
}


/* this function checks the range of the channel
index supplied against the user defined channel count */
/* returns true if this channel is valid */
static BOOLEAN GetInpChanValid(int chan)
{
	if (!((chan>=0)&&(chan<b_localChanCount)))
	{
		printf("%s: invalid input %d\n", __FUNCTION__, chan);
		return FALSE;
	}
	return TRUE;
}

/* this function returns true if this input is valid */
/* *** check the field of valid_xxx which update_chan_valid_status() put in */
static BOOLEAN GetInpValid(int chan)
{
	BOOLEAN valid=FALSE;

	if (!GetInpChanValid(chan))
	{
		return FALSE;
	}

	if (s_chanStat2Tbl[chan].valid_ret==0)
	{
		valid = (s_chanStat2Tbl[chan].valid_ok) ? TRUE : FALSE;
	}
	return valid;
}

/* this function checks internal data structure.  It reports if there is
* an los reported by the servo
* return true, if signal is ok
* *** check the field of signal_ok which update_chan_signal_status() put in */
static BOOLEAN IsPhaseGood(int chan)
{
	BOOLEAN good;

	if (!GetInpChanValid(chan))
	{
		return FALSE;
	}

	good = s_chanStat2Tbl[chan].signal_ok;

	return good;
}

/* this function returns true if this input performance is qualified by the servo */
static BOOLEAN GetPerfAcceptable(int chan)
{
	BOOLEAN valid=FALSE;
   UINT8 servo_index;

	if (!GetInpChanValid(chan))
	{
		printf("%s: invalid chan %d\n", __FUNCTION__, chan);
		return FALSE;
	}

/* convert to servo index */
   servo_index = UserChan2ServoIndex((UINT8)chan);

/* get accept value */
	valid = s_servoChanAccept.b_accepted[servo_index];

	return valid;
}
/* This function returns a pql value, it could be assumed or from external reported pql */
/* *** check the field of ssm_xxx which update_chan_ssm_status() put in */
static int GetInpPql(int chan, BOOLEAN is_freq, BOOLEAN *readExternally)
{
	int ret_code;
	int pql=16;
	UINT8  ssm;
	int	chan_type;

	if (!GetInpChanValid(chan))
	{
		return 0;
	}

	*readExternally = FALSE;

	/* make sure it has assumed value */
	if(is_freq)
	{
		pql = s_ChanConfig[chan].b_ChanFreqAssumedQL;
	}
	else
	{
		pql = s_ChanConfig[chan].b_ChanTimeAssumedQL;
	}

	if(s_ChanConfig[chan].o_ChanAssumedQLenabled==FALSE)
	{
		ret_code = s_chanStat2Tbl[chan].ssm_ret;
		if (ret_code==0)
		{
			if (s_chanStat2Tbl[chan].ssm_ok==TRUE)
			{
				chan_type = GetChanType(chan);
				ssm = s_chanStat2Tbl[chan].ssm_value;
				/* Have a SSM lookup table to get pql value */
				pql = FindSsm2Pql(chan_type, ssm);
				if (pql>0)
				{
					*readExternally = TRUE;
				}
				if (pql<0)
				{
					/* if not found use the user provision */
					if(is_freq)
					{
						pql = s_ChanConfig[chan].b_ChanFreqAssumedQL;
					}
					else
					{
						pql = s_ChanConfig[chan].b_ChanTimeAssumedQL;
					}
				}
			}
		}
	}

	return pql;
}


/* This function returns an internal clock pql value */
static int GetMyModulePql(void)
{
  t_servoConfigType sc;

  if(SC_GetServoConfig(&sc) == 0)
  {
    return sc.b_loQl;
  }
  else
    return 16;
}

static int GetChanType(int chan)
{
	if (!GetInpChanValid(chan))
	{
		return e_SC_CHAN_TYPE_RESERVED;
	}
	return s_ChanConfig[chan].e_ChanType;
}

static BOOLEAN GetInpEnab(int chan, BOOLEAN is_freq)
{
//   return 1; /* kjh temp to test */

	if (!GetInpChanValid(chan))
	{
		return FALSE;
	}

	if(is_freq)
	{
		return s_ChanConfig[chan].o_ChanFreqEnabled;
	}
	else
	{
		return s_ChanConfig[chan].o_ChanTimeEnabled;
	}
}



static int GetInpPrio(int chan, BOOLEAN is_freq)
{
	int prio=0;

	if (!GetInpChanValid(chan))
	{
		return 0;
	}

	if(is_freq)
	{
		prio = s_ChanConfig[chan].b_ChanFreqPrio;
	}
	else
	{
		prio = s_ChanConfig[chan].b_ChanTimePrio;
	}

	return prio;
}

#if 0
static int GetPrevChan(BOOLEAN is_freq)
{
	int chan = (is_freq) ? prevChan_f : prevChan_t;

	return chan;
}

static void SetPrevChan(int chan, BOOLEAN is_freq)
{
	if(is_freq)
	{
		prevChan_f = chan;
	}
	else
	{
		prevChan_t = chan;
	}
	return;
}
#endif

int GetCurrChan(BOOLEAN is_freq)
{
	int chan = (is_freq) ? currChan_f : currChan_t;

	return chan;
}

static void SetCurrChan(int chan, BOOLEAN is_freq)
{
	if(is_freq)
	{
		currChan_f = chan;
	}
	else
	{
		currChan_t = chan;
	}
}

/* You need to put user selected channel here  with flag */
/* *** */
static int GetUsrChan(BOOLEAN is_freq)
{
	int chan = (is_freq) ? usrChan_f : usrChan_t;

	return chan;
}

static void SetUsrChan(int chan, BOOLEAN is_freq)
{
	if(is_freq)
	{
		usrChan_f = chan;
		usrChanFlag_f = TRUE;
	}
	else
	{
		usrChan_t = chan;
		usrChanFlag_t = TRUE;
	}
}

static int GetUsrChanFlag(BOOLEAN is_freq)
{
	int chan = (is_freq) ? usrChanFlag_f : usrChanFlag_t;

	return chan;
}

static void ClrUsrChanFlag(BOOLEAN is_freq)
{
	if(is_freq)
	{
		usrChanFlag_f = FALSE;
	}
	else
	{
		usrChanFlag_t = FALSE;
	}
}


/* if the value currPql_x is -1, the it holdover, return reference clock pql */
static int GetCurrPql(BOOLEAN is_freq)
{
	FLL_STATUS_STRUCT fp;

	get_fll_status(&fp);


	int pql;


   if(fp.cur_state == FLL_NORMAL)
   {
   	if (is_freq)
   	{
   		if (currPql_f<0)
   		{
   			pql = GetMyModulePql();
   		}
   		else
   		{
   			pql = currPql_f;
   		}
   	}
   	else
   	{
   		if (currPql_t<0)
   		{
   			pql = GetMyModulePql();
   		}
   		else
   		{
   			pql = currPql_t;
   		}
   	}
   }
   else
   {
   	pql = GetMyModulePql();
   }
	return pql;
}

static void SetCurrPql(int pql, BOOLEAN is_freq)
{
	if(is_freq)
	{
		currPql_f = pql;
	}
	else
	{
		currPql_t = pql;
	}
}

static UINT8 prevF_QuaTbl[SC_TOTAL_CHANNELS];
static UINT8 prevT_QuaTbl[SC_TOTAL_CHANNELS];
static void HandleInputQualifiedEvent(BOOLEAN is_freq)
{
	static BOOLEAN flag=0;
	UINT8 currFTbl[SC_TOTAL_CHANNELS];
	UINT8 currTTbl[SC_TOTAL_CHANNELS];
	int s;
	UINT8 *ctblp, *ptblp;
	int evt_id=0;
	int pql, prio;
	BOOLEAN readExternally;

	/* initialize the internal data structure */
	if (flag==0)
	{
		for (s=0;s<SC_TOTAL_CHANNELS;s++)
		{
			prevF_QuaTbl[s]=0;
			prevT_QuaTbl[s]=0;
		}
		flag = 1;
	}

	/* set table pointer */
	if (is_freq)
	{
		ptblp = &prevF_QuaTbl[0];
		ctblp = &currFTbl[0];
		evt_id = e_FREQ_CHAN_QUALIFIED;
	}
	else
	{
		ptblp = &prevT_QuaTbl[0];
		ctblp = &currTTbl[0];
		evt_id = e_TIME_CHAN_QUALIFIED;
	}


	for (s=0; s< b_localChanCount; s++)
	{
		if(!GetInpEnab(s, is_freq))
		{
			ctblp[s]=0;
			continue;
		}
		if(!GetInpValid(s))
		{
			ctblp[s]=0;
			continue;
		}

		if (!IsPhaseGood(s))
		{
			ctblp[s]=0;
			continue;
		}

		if (!GetPerfAcceptable(s))
		{
			ctblp[s]=0;
			continue;
		}


		prio = GetInpPrio(s, is_freq);
		pql = GetInpPql(s, is_freq, &readExternally);

		if ((prio<=0) || (prio>10))
		{
			ctblp[s]=0;
			continue;
		}

		if ((pql<=0) || (pql>16))
		{
			ctblp[s]=0;
			continue;
		}
		/* qualified */
		ctblp[s]=1;
	}


	for (s=0; s< b_localChanCount; s++)
	{
		if (ctblp[s] != ptblp[s])
		{
			if (ctblp[s])
				SC_Event(evt_id, e_EVENT_INFO, e_EVENT_SET,sc_alarm_lst[evt_id].dsc_str, s);
			else
				SC_Event(evt_id, e_EVENT_INFO, e_EVENT_CLEAR,sc_alarm_lst[evt_id].dsc_str, s);
		}
		ptblp[s]=ctblp[s];
	}
}

static UINT8 prev_HaveDataTbl[SC_TOTAL_CHANNELS];
static void HandleInputNoDataEvent(void)
{
	static BOOLEAN flag=0;
	UINT8 currTbl[SC_TOTAL_CHANNELS];
	int s;
	UINT8 *ctblp, *ptblp;
	int evt_id=0;

	/* initialize the internal data structure */
	if (flag==0)
	{
		for (s=0;s<SC_TOTAL_CHANNELS;s++)
		{
			prev_HaveDataTbl[s]=1;
		}
		flag = 1;
	}

	/* set table pointer */
	ptblp = &prev_HaveDataTbl[0];
	ctblp = &currTbl[0];
	evt_id = e_CHAN_NO_DATA;

	for (s=0; s< b_localChanCount; s++)
	{
		if (!IsPhaseGood(s))
		{
			ctblp[s]=0;
			continue;
		}

		/* has data */
		ctblp[s]=1;
	}


	for (s=0; s< b_localChanCount; s++)
	{
		if (ctblp[s] != ptblp[s])
		{
			if (ctblp[s]==0)
				SC_Event(evt_id, e_EVENT_INFO, e_EVENT_SET,sc_alarm_lst[evt_id].dsc_str, s);
			else
				SC_Event(evt_id, e_EVENT_INFO, e_EVENT_CLEAR,sc_alarm_lst[evt_id].dsc_str, s);
		}
		ptblp[s]=ctblp[s];
	}
}


/* this routine handle each handle being selected or not being selected */
static UINT8 prevF_SelTbl[SC_TOTAL_CHANNELS];
static UINT8 prevT_SelTbl[SC_TOTAL_CHANNELS];
static void HandleInputSelectEvent(BOOLEAN is_freq, int rc, int selectedChan)
{
	static BOOLEAN flag=0;
	int s;
	UINT8 *tblp;
	int evt_id=0;

	/* initialize the internal data structure */
	if (flag==0)
	{
		for (s=0;s<SC_TOTAL_CHANNELS;s++)
		{
			prevF_SelTbl[s]=0;
			prevT_SelTbl[s]=0;
		}
		flag = 1;
	}

	/* set table pointer */
	if (is_freq)
	{
		tblp = &prevF_SelTbl[0];
		evt_id = e_FREQ_CHAN_SELECTED;
	}
	else
	{
		tblp = &prevT_SelTbl[0];
		evt_id = e_TIME_CHAN_SELECTED;
	}

	if (rc==1)
	{	/* if channel found */
		for (s=0;s<b_localChanCount;s++)
		{
			if (s==selectedChan)
			{
				if (tblp[s]==0)
				{
					/* event set */
					tblp[s] = 1;
					SC_Event(evt_id, e_EVENT_INFO, e_EVENT_SET,sc_alarm_lst[evt_id].dsc_str, s);
				}
			}
			else
			{
				if (tblp[s]!=0)
				{
					/* event clr */
					tblp[s] = 0;
					SC_Event(evt_id, e_EVENT_INFO, e_EVENT_CLEAR,sc_alarm_lst[evt_id].dsc_str, s);
				}
			}
		}
	}
	else if (rc==0)
	{	/* if no channel found */
		for (s=0;s<b_localChanCount;s++)
		{
			if (tblp[s]!=0)
			{
				/* event clr */
				tblp[s] = 0;
				SC_Event(evt_id, e_EVENT_INFO, e_EVENT_CLEAR,sc_alarm_lst[evt_id].dsc_str, s);
			}
		}
	}
}

static UINT8 prev_PerformanceFltTbl[SC_TOTAL_CHANNELS];
static void HandlePerformanceFltEvent(void)
{
	static BOOLEAN flag=0;
	UINT8 currTbl[SC_TOTAL_CHANNELS];
	int s;
	UINT8 *ctblp, *ptblp;
	int evt_id=0;

	/* initialize the internal data structure */
	if (flag==0)
	{
		for (s=0;s<SC_TOTAL_CHANNELS;s++)
		{
			prev_PerformanceFltTbl[s]=0;
		}
		flag = 1;
	}

	/* set table pointer */
	ptblp = &prev_PerformanceFltTbl[0];
	ctblp = &currTbl[0];
	evt_id = e_CHAN_PERFORMANCE;

	for (s=0; s< b_localChanCount; s++)
	{
		/* detect performance fault */
		if(GetPerfAcceptable(s))
		{
			ctblp[s]=0;
			continue;
		}
		/* valid */
		ctblp[s]=1;
	}

	for (s=0; s< b_localChanCount; s++)
	{
		if (ctblp[s] != ptblp[s])
		{
			if (ctblp[s]!=0)
				SC_Event(evt_id, e_EVENT_INFO, e_EVENT_SET,sc_alarm_lst[evt_id].dsc_str, s);
			else
				SC_Event(evt_id, e_EVENT_INFO, e_EVENT_CLEAR,sc_alarm_lst[evt_id].dsc_str, s);
		}
		ptblp[s]=ctblp[s];
	}
}


static UINT8 prev_ValidTbl[SC_TOTAL_CHANNELS];
static void HandleInputNotValidEvent(void)
{
	static BOOLEAN flag=0;
	UINT8 currTbl[SC_TOTAL_CHANNELS];
	int s;
	UINT8 *ctblp, *ptblp;
	int evt_id=0;

	/* initialize the internal data structure */
	if (flag==0)
	{
		for (s=0;s<SC_TOTAL_CHANNELS;s++)
		{
			prev_ValidTbl[s]=1;
		}
		flag = 1;
	}

	/* set table pointer */
	ptblp = &prev_ValidTbl[0];
	ctblp = &currTbl[0];
	evt_id = e_CHAN_NOT_VALID;


	for (s=0; s< b_localChanCount; s++)
	{
		if(!GetInpValid(s))
		{
			ctblp[s]=0;
			continue;
		}
		/* valid */
		ctblp[s]=1;
	}


	for (s=0; s< b_localChanCount; s++)
	{
		if (ctblp[s] != ptblp[s])
		{
			if (ctblp[s]==0)
				SC_Event(evt_id, e_EVENT_INFO, e_EVENT_SET,sc_alarm_lst[evt_id].dsc_str, s);
			else
				SC_Event(evt_id, e_EVENT_INFO, e_EVENT_CLEAR,sc_alarm_lst[evt_id].dsc_str, s);
		}
		ptblp[s]=ctblp[s];
	}
}


/* this routine returns event bitmap of an given channel */
/* return 0 if chan within range of b_localChanCount */
/* return -1, if channel is out of range of b_localChanCount */
/* Output Parameter */
/* 	bitmap: an 32-bit bitmap (allow 32 event max */
/* 		bit0: e_CHAN_NO_DATA 			*/
/* 		bit1: e_CHAN_PERFORMANCE 		*/
/* 		bit2: e_FREQ_CHAN_QUALIFIED 	*/
/* 		bit3: e_FREQ_CHAN_SELECTED 		*/
/* 		bit4: e_TIME_CHAN_QUALIFIED 	*/
/* 		bit5: e_TIME_CHAN_SELECTED 		*/
/* 		bit6: e_CHAN_NOT_VALID 			*/
int GetEventBitmap(int chan, UINT32 *bitmap)
{
	UINT32 evtbitmap=0;

	if (!GetInpChanValid(chan))
	{
		return -1;
	}

	if(chan >= b_localChanCount)
	{
		return -1;
	}

	if (prev_HaveDataTbl[chan]==0)
		evtbitmap |= 0x1;
	if (prev_PerformanceFltTbl[chan]!=0)
		evtbitmap |= 0x2;
	if (prevF_QuaTbl[chan]!=0)
		evtbitmap |= 0x4;
	if (prevF_SelTbl[chan]!=0)
		evtbitmap |= 0x8;
	if (prevT_QuaTbl[chan]!=0)
		evtbitmap |= 0x10;
	if (prevT_SelTbl[chan]!=0)
		evtbitmap |= 0x20;
	if (prev_ValidTbl[chan]==0)
		evtbitmap |= 0x40;

	*bitmap = evtbitmap;

	return 0;
}

/*
----------------------------------------------------------------------------
                                Update_chanSelect()
Description:
This function will update the selection table given to the servo.

Parameters:

Inputs
        None

Outputs:
        None

Return value:
         0 - successful
         1 - failure
-----------------------------------------------------------------------------
*/
int Update_chanSelect(void)
{
	int fchan;
	int tchan;
	int fchan_ret=-1;
	int tchan_ret=-1;
	int s;
	static int b_localChanCount_prev=0;
	UINT8 servo_chan_t, servo_chan_f;

	/* First run through initialize channels */
	if(dw_servoChanCntr++ == 0)
	{
		Init_chanSelect();
	}

	//printf("Update_chanSelect\n");

	if (b_localChanCount_prev != b_localChanCount)
	{
		//printf("b_localChanCount: %d\n",b_localChanCount);
		b_localChanCount_prev = b_localChanCount;
	}

	for (s = b_localChanCount-1; s>=0; s--)
	{
		update_chan_valid_status(s);
		update_chan_ssm_status(s);
		update_chan_signal_status(s);
	}

	/* Get status from Servo */
	Get_servoAccept(&s_servoChanAccept);

	/* display the event of no data */
	HandleInputNoDataEvent();

	/* display the event of not valid channel */
	HandleInputNotValidEvent();

	/* display the event of qualified channel */
	HandleInputQualifiedEvent(1);
	HandleInputQualifiedEvent(0);


	/* display the event of performance fault */
	HandlePerformanceFltEvent();


	switch(e_swtMode)
	{
	case e_SC_SWTMODE_AR:
		tchan_ret = Get_AR_ON_Ref(0, &tchan);
		fchan_ret = Get_AR_ON_Ref(1, &fchan);
		break;
	case e_SC_SWTMODE_AS:
		tchan_ret = Get_AS_ON_Ref(0, &tchan);
		fchan_ret = Get_AS_ON_Ref(1, &fchan);
		break;
	case e_SC_SWTMODE_OFF:
		tchan_ret = Get_AS_OFF_Ref(0, &tchan);
		fchan_ret = Get_AS_OFF_Ref(1, &fchan);
		break;
	default:
		break;
	}


	/* display the input selection event */

	HandleInputSelectEvent(1, fchan_ret, fchan);
	HandleInputSelectEvent(0, tchan_ret, tchan);


	/* we need to convert to servo index */
   /* clear selections */
	for (s=0;s<NUM_OF_SERVO_CHAN;s++)
	{
      s_servoChanSelectTable.s_servoChanSelect[s].b_selectedTime = 0;
      s_servoChanSelectTable.s_servoChanSelect[s].b_selectedFreq = 0;
   }

/* if there is a valid selection */
	if(tchan_ret==1)
	{
		servo_chan_t = UserChan2ServoIndex(tchan);
		if (servo_chan_t != 0xFF)
		{
			s_servoChanSelectTable.s_servoChanSelect[servo_chan_t].b_selectedTime = 1;
		}
	}

	if(fchan_ret==1)
	{
		servo_chan_f = UserChan2ServoIndex(fchan);

		if (servo_chan_f != 0xFF)
		{
			s_servoChanSelectTable.s_servoChanSelect[servo_chan_f].b_selectedFreq = 1;
		}
		else{
			printf("%s: servo_chan_f 0xFF.\n", __FUNCTION__);
		}
	}
	else{
		printf("%s: fchan_ret %d..\n", __FUNCTION__, fchan_ret);
	}

/* update o_valid values in table */
   UpdateValidReportToServo();

  if(get_debug_print() & CHAN_STAT_PRT)
  {
    printChanStatus();
  }

	return 0;
}

/*
----------------------------------------------------------------------------
                                FindHighestPrioAvailInp

Description
	The lowest priority number is the hightest. Therefore,
		1 is the highest. 10 is the lowest.
		0 is the monitor mode.


Parameters:
	chan:	channel will be picked
			0, if time

	is_freq:1, if freq
			0, if time

Return value:
 	TRUE: 	if found
 	FALSE:	not found

-----------------------------------------------------------------------------
*/
static int FindHighestPrioAvailInp(int *chan, int *ql, BOOLEAN is_freq)
{
	int s;
	int prio,inp_prio=99;
	int pql,inp_pql=16;
	int my_chan;
	int found=0;
	static int my_prevchan[2]={-1,-1};
	BOOLEAN readExternally;

	// the lowest chan number has higher priority

	//printf("FindHighestPrioAvailInp: b_localChanCount=%d\n",b_localChanCount);

	if(b_localChanCount==0)
	{
		*chan = 0;
		printf("%s: localChanCount 0...\n", __FUNCTION__);		
		return 0;
	}

	for (s = b_localChanCount-1; s>=0; s--)
	{
/* if channel is redundant input, then disqualify channel from the selection process */
      if(s_ChanConfig[s].e_ChanType == e_SC_CHAN_TYPE_REDUNDANT)
      {
		printf("Redund chan...\n");
         continue;
      }

		if(!GetInpEnab(s, is_freq))
		{
			printf("GetInpEnab: NO, %d \n", s);
			continue;
		}
		if(!GetInpValid(s))
		{
			printf("GetInpValid: NO, %d \n", s);
			continue;
		}

		if (!IsPhaseGood(s))
		{
			printf("IsPhaseGood: NO, %d \n", s);
			continue;
		}

		if (!GetPerfAcceptable(s))
		{
			printf("Unacceptable source: %d\n", s);
			continue;
		}

		pql = GetInpPql(s, is_freq, &readExternally);
		prio = GetInpPrio(s, is_freq);


		if (prio<=0)
		{	// monitor mode
			printf ("prio is zero (%d) pql=%d\n", (int)s, (int)prio);
			continue;
		}

		if ((pql<=0) || (pql>16))
		{	// no such value
			printf ("pql is out of range (%d) pql=%d\n", (int)s, (int)pql);
			continue;
		}

		if (pql>GetMyModulePql())
		{	// if this pql is worse than my clock, we should not choose
			printf ("pql>clkpql (%d) %d, %d\n", (int)s, (int)pql,(int)GetMyModulePql() );
			continue;
		}

		if (prio<inp_prio)
		{
			my_chan=s;
			inp_pql=pql;
			inp_prio=prio;
			found=1;
			//printf ("FOUND (prio): FindHighestPrioAvailInp (%d), prio=%d, pql=%d\n", (int)s, (int)prio, (int)pql);
		}
		else if (prio==inp_prio)
		{	// if equal, we need to use the highest
			if ((pql<=inp_pql) && (pql>0))
			{
				my_chan=s;
				inp_pql=pql;
				inp_prio=prio;
				found=1;
				//printf ("FOUND (pri equ): FindHighestPrioAvailInp (%d), prio=%d, pql=%d, inp_pql=%d\n", (int)s, (int)prio, (int)pql, (int)inp_pql);
			}
			else{
				printf("%s: pql %d prio %d ...\n", __FUNCTION__, pql, prio);
			}
		}
		else{
			printf("%s: pql %d......prio %d..\n", __FUNCTION__, pql, prio);
		}
		if(found == 0)
			printf("s: %d, prio: %d, pql: %d, is_freq %d\n", s, (int)prio, (int)pql, is_freq);
	}

	if (found)
	{
		*ql = inp_pql;
		*chan=my_chan;
		if (my_chan != my_prevchan[is_freq])
		{
			printf ("FindHighestPrioAvailInp, FOUND, (%d), prio=%d, pql=%d\n", (int)my_chan, (int)prio, (int)pql);
		}

		my_prevchan[is_freq] = my_chan;
	}

	return found;
}


/*
----------------------------------------------------------------------------
                                FindHighestPqlAvailInp

Description
 	The lowest priority number is the hightest. Therefore,
 		1 is the highest. 10 is the lowest.
 		0 is the monitor mode.


Parameters:
	chan:	channel will be picked
			0, if time

	is_freq:1, if freq
			0, if time

Return value:
 	TRUE: 	if found
 	FALSE:	not found

-----------------------------------------------------------------------------
*/
static int FindHighestPqlAvailInp(int *chan, int *ql, BOOLEAN is_freq)
{
	int s;
	int prio,inp_prio=99;
	int pql,inp_pql=16;
	int my_chan;
	int found=0;
	static int my_prevchan[2]={-1,-1};
	BOOLEAN readExternally;

	// the rightest has higher priority
	// the smallest port number has higher priority
	if(b_localChanCount==0)
	{
		*chan = 0;
		return 0;
	}

	for (s = b_localChanCount-1; s>=0; s--)
	{
/* if channel is redundant input, then disqualify channel from the selection process */
      if(s_ChanConfig[s].e_ChanType == e_SC_CHAN_TYPE_REDUNDANT)
      {
         continue;
      }
		if(!GetInpEnab(s, is_freq))
		{
			continue;
		}
		if(!GetInpValid(s))
		{
			continue;
		}

		if (!IsPhaseGood(s))
		{
			continue;
		}

		if (!GetPerfAcceptable(s))
		{
			continue;
		}

		pql = GetInpPql(s, is_freq, &readExternally);
		prio = GetInpPrio(s, is_freq);

		if (prio==0)
		{	// monitor mode
			continue;
		}

		if ((pql<=0) || (pql>16))
		{	// no such value
			continue;
		}

		if (pql>GetMyModulePql())
		{	// if this pql is worse than my clock, we should not choose
			// printf ("pql>clkpql %d, %d, %d\n", s, pql,GetMyModulePql() );
			continue;
		}

		if (pql<inp_pql)
		{	// if this pql is better, we found it
			my_chan=s;
			inp_pql=pql;
			inp_prio=prio;
			found=1;
			//printf ("FOUND (pql): FindHighestPqlAvailInp %d, pql=%d, prio=%d\n", s,pql, prio);
		}
		else if (pql==inp_pql)
		{	// if equal, we need to use the highest prio
			if (prio<=inp_prio)
			{
				my_chan=s;
				inp_pql=pql;
				inp_prio=prio;
				found=1;
				//printf ("FOUND (pql equ): FindHighestPqlAvailInp %d, pql=%d, prio=%d, inp_prio=%d\n", s, pql,prio, inp_prio);
			}
		}
	}

	if (found)
	{
		*ql = inp_pql;
		*chan=my_chan;
		if (my_chan != my_prevchan[is_freq])
		{
			//printf ("FindHighestPqlAvailInp, FOUND, (%d), prio=%d, pql=%d\n", (int)my_chan, (int)prio, (int)pql);
		}

		my_prevchan[is_freq] = my_chan;
	}

	return found;
}


/*
----------------------------------------------------------------------------
                                Get_AR_ON_Ref

Description
This routine select an input reference on AR method


Parameters:
	is_freq:1, if freq
			0, if time

	selectedChan:
		selected channel selected

Return value:
 	TRUE: 	if found
 	FALSE:	not found
 	-1:		Not Stable

-----------------------------------------------------------------------------
*/
static int Get_AR_ON_Ref(BOOLEAN is_freq, int *selectedChan)
{
	int my_chan = -1, my_pql = -1;
	int redundant_pql;
	BOOLEAN found = FALSE;
   BOOLEAN o_redEnabled; 
   SC_t_activeStandbyEnum e_redStatus;
	static int good_cnt[2]={0,0};
	static int not_found_cnt[2]={0,0};
	static int prevChan[2]={-1,-1};

	ClrUsrChanFlag(is_freq);

	//Execute this block
	{
		if (e_selMode == e_SC_CHAN_SEL_PRIO)
			found = FindHighestPrioAvailInp(&my_chan, &my_pql, is_freq);
		else
			found = FindHighestPqlAvailInp(&my_chan, &my_pql, is_freq);

		//printf("+++ my_chan= %d is_freq= %d, found %d\n", (int)my_chan, (int)is_freq, found);

		// add debounce to avoid unstable problems
		if (found)
		{
			if (my_chan==prevChan[is_freq])
				good_cnt[is_freq]++;
			else
				good_cnt[is_freq]=0;

			prevChan[is_freq]=my_chan;
			not_found_cnt[is_freq]=0;

			if (good_cnt[is_freq]<kREF_STABLE_CNT){
				printf("%s: found, my_chan %d prevChan[%d] %d good_cnt[%d] %d..\n",
						__FUNCTION__, my_chan, is_freq, prevChan[is_freq], is_freq, good_cnt[is_freq]);
				return -1; // not stable
			}
		}
		else
		{
			good_cnt[is_freq]=0;
			not_found_cnt[is_freq]++;
			if (not_found_cnt[is_freq]<kREF_STABLE_CNT){
				printf("%s: not found, my_chan %d prevChan[%d] %d not_good_cnt[%d] %d..\n",
						__FUNCTION__, my_chan, is_freq, prevChan[is_freq], is_freq, not_found_cnt[is_freq]);
				return -1; // not stable
			}
		}
	}


/* if redundancy is standby mode then force into holdover or redundancy lock */
   SC_GetRedundantStatus(&o_redEnabled, &e_redStatus);

   if(((e_redStatus == e_SC_STANDBY) || (e_redStatus == e_SC_TRANSITION_TO_STANDBY)) 
        && (o_redEnabled == TRUE))
   {
      if(i_redChan != -1)
      {
   		if(GetInpEnab(i_redChan, is_freq) &&
            GetInpValid(i_redChan) &&
            IsPhaseGood(i_redChan) &&
            prevF_QuaTbl[i_redChan])
   		{
/* find assumed to report to current setting */
            if(is_freq)
            {
               redundant_pql = s_ChanConfig[i_redChan].b_ChanFreqAssumedQL;
            }
            else
            {
               redundant_pql = s_ChanConfig[i_redChan].b_ChanTimeAssumedQL;
            }

/* make redundant reference selected reference */
      		SetCurrPql(redundant_pql, is_freq);
      		SetCurrChan(i_redChan, is_freq);
      		*selectedChan = i_redChan;
            found = TRUE;  
   		}
   		else
         {
/* Holdover mode */
      		SetCurrPql(-1, is_freq);
      		SetCurrChan(-1, is_freq);
      		*selectedChan = -1;
         }
      }
      else
      {
/* no redundant channel configured */
     		SetCurrPql(-1, is_freq);
   		SetCurrChan(-1, is_freq);
   		*selectedChan = -1;
      }
   }
   else
   {
   	if (found)
   	{
   		SetCurrPql(my_pql, is_freq);
   		SetCurrChan(my_chan, is_freq);
   		*selectedChan = my_chan;
   	}
   	else
   	{
   		SetCurrPql(-1, is_freq);
   		SetCurrChan(-1, is_freq);
   		*selectedChan = -1;
   	}
   }

	return found;
}

/*
----------------------------------------------------------------------------
                                Get_AS_ON_Ref

Description
This routine select an input reference on AS method


Parameters:
	is_freq:1, if freq
			0, if time

	selectedChan:
		selected channel selected

Return value:
 	TRUE: 	if found
 	FALSE:	not found
 	-1:		Not Stable

-----------------------------------------------------------------------------
*/
static int Get_AS_ON_Ref(BOOLEAN is_freq, int *selectedChan)
{
	int pql,pri;
	static int good_cnt[2]={0,0};
	static int not_found_cnt[2]={0,0};
	static int bad_cnt[2]={0,0};
	static int prevChan[2]={-1,-1};
	int found = 0;
	int redundant_pql;
   BOOLEAN o_redEnabled; 
   SC_t_activeStandbyEnum e_redStatus;
	int my_chan, my_pql;
	int qualified=TRUE;
	BOOLEAN readExternally;

   SC_GetRedundantStatus(&o_redEnabled, &e_redStatus);

	// If the user selected, choose this one
	if (GetUsrChanFlag(is_freq))
	{
		my_chan = GetUsrChan(is_freq);
		SetCurrChan(my_chan,is_freq);
		ClrUsrChanFlag(is_freq);
	}

	my_chan = GetCurrChan(is_freq);

	if (GetInpValid(my_chan)==FALSE)
		qualified = FALSE;
	else if (IsPhaseGood(my_chan)==FALSE)
		qualified = FALSE;
   else if (GetPerfAcceptable(my_chan) == FALSE)
		qualified = FALSE;
   else if((my_chan == i_redChan) && ((e_redStatus == e_SC_ACTIVE) || (e_redStatus == e_SC_TRANSITION_TO_ACTIVE)))
		qualified = FALSE;

	if (qualified==TRUE)
	{
		if (e_selMode == e_SC_CHAN_SEL_QL)
		{	// if pql input selection
			// priority must greater than zero
			pri=GetInpPrio(my_chan, is_freq);
			if (pri>0)
			{
				pql = GetInpPql(my_chan, is_freq, &readExternally);
				if (pql>GetMyModulePql())
				{	// if this pql is worse than my clock, we should not choose
					bad_cnt[is_freq]++;
					if (bad_cnt[is_freq]<kREF_STABLE_CNT)
					{
						return -1;	// try at least xx times
					}
				}
				else
				{
					pri=GetInpPrio(my_chan, is_freq);
					if (pri==0)
					{
						bad_cnt[is_freq]++;
						if (bad_cnt[is_freq]<kREF_PRI_STABLE_CNT)
						{
							return -1;	// try at least xx times
						}
					}
					else
					{
	 					found = 1;
						my_pql = pql;
						bad_cnt[is_freq]=0;
					}
				}
			}
			else
			{
				bad_cnt[is_freq]++;
				if (bad_cnt[is_freq]<kREF_PRI_STABLE_CNT)
				{
					return -1;	// try at least xx times
				}
			}
		}
		else
		{	// if priority, we found it
			pri=GetInpPrio(my_chan, is_freq);
			if (pri==0)
			{
				bad_cnt[is_freq]++;
				if (bad_cnt[is_freq]<kREF_PRI_STABLE_CNT)
				{
					return -1;	// try at least xx times
				}
			}
			else
			{
				pql = GetInpPql(my_chan, is_freq, &readExternally);
				if (pql>GetMyModulePql())
				{	// if this pql is worse than my clock, we should not choose
					bad_cnt[is_freq]++;
					if (bad_cnt[is_freq]<kREF_STABLE_CNT)
					{
						return -1;	// try at least xx times
					}
				}
				else
				{
					found = 1;
					my_pql = pql;
					bad_cnt[is_freq]=0;
				}
			}
		}
	}
	else
	{
		// don't switch input so fast
		// at least check two consecutive times
		bad_cnt[is_freq]++;
		if (bad_cnt[is_freq]<kREF_STABLE_CNT)
		{
			return -1;	// try at least xx times
		}
	}

	// we need to check inp priority
	if (found==0)
	{
		if (e_selMode == e_SC_CHAN_SEL_PRIO)
			found = FindHighestPrioAvailInp(&my_chan, &my_pql, is_freq);
		else
			found = FindHighestPqlAvailInp(&my_chan, &my_pql, is_freq);

		//	printf("+++ my_chan= %d is_freq= %d", (int)my_chan, (int)is_freq);

		// add debounce to avoid unstable problems
		if (found)
		{
			if (my_chan==prevChan[is_freq])
				good_cnt[is_freq]++;
			else
				good_cnt[is_freq]=0;

			prevChan[is_freq]=my_chan;
			not_found_cnt[is_freq]=0;

			if (good_cnt[is_freq]<kREF_STABLE_CNT)
				return -1; // not stable
		}
		else
		{
			good_cnt[is_freq]=0;
			not_found_cnt[is_freq]++;
			if (not_found_cnt[is_freq]<kREF_STABLE_CNT)
				return -1; // not stable
		}
	}

/* if redundancy is standby mode then force into holdover or redundancy lock */
   if(((e_redStatus == e_SC_STANDBY) || (e_redStatus == e_SC_TRANSITION_TO_STANDBY)) 
        && (o_redEnabled == TRUE))
   {
      if(i_redChan != -1)
      {
   		if(GetInpEnab(i_redChan, is_freq) &&
            GetInpValid(i_redChan) &&
            IsPhaseGood(i_redChan))
   		{
/* find assumed to report to current setting */
            if(is_freq)
            {
               redundant_pql = s_ChanConfig[i_redChan].b_ChanFreqAssumedQL;
            }
            else
            {
               redundant_pql = s_ChanConfig[i_redChan].b_ChanTimeAssumedQL;
            }

/* make redundant reference selected reference */
      		SetCurrPql(redundant_pql, is_freq);
      		SetCurrChan(i_redChan, is_freq);
      		*selectedChan = i_redChan;
            found = TRUE;  
   		}
   		else
         {
/* Holdover mode */
      		SetCurrPql(-1, is_freq);
      		SetCurrChan(-1, is_freq);
      		*selectedChan = -1;
         }
      }
      else
      {
/* no redundant channel configured */
     		SetCurrPql(-1, is_freq);
   		SetCurrChan(-1, is_freq);
   		*selectedChan = -1;
      }
   }
   else
   {
     	if (found)
   	{
   		SetCurrPql(my_pql, is_freq);
   		SetCurrChan(my_chan, is_freq);
   		*selectedChan = my_chan;
   	}
   	else

   	{
   		SetCurrPql(-1, is_freq);
   		SetCurrChan(-1, is_freq);
   		*selectedChan = -1;
   	}
   }
	return found;
}

/*
----------------------------------------------------------------------------
                                Get_AS_OFF_Ref

Description
This routine select an input reference on AS OFF method


Parameters:
	is_freq:1, if freq
			0, if time

	selectedChan:
		selected channel selected

Return value:
 	TRUE: 	if found
 	FALSE:	not found
 	-1:		Not Stable

-----------------------------------------------------------------------------
*/
static int Get_AS_OFF_Ref(BOOLEAN is_freq, int *selectedChan)
{
	static int bad_cnt[2]={0,0};
	int pql,pri;
	int found = 0;
	int redundant_pql;
   BOOLEAN o_redEnabled; 
   SC_t_activeStandbyEnum e_redStatus;
	int my_chan, my_pql;
	static int prev_chan = -1; /* first initialize with invalid channel */
	int qualified=1;
	BOOLEAN readExternally;

   SC_GetRedundantStatus(&o_redEnabled, &e_redStatus);
	my_chan = GetCurrChan(is_freq);
	/* if not a valid channel index (because there is not a currently selected channel),
	   ie, we are in holdover */
	if (!GetInpChanValid(my_chan))
	{
		/* set equal to last qualified and selected channel */
		my_chan = prev_chan;
	}

	// If the user selected, choose this one
	if (GetUsrChanFlag(is_freq))
	{
		my_chan = GetUsrChan(is_freq);
		SetCurrChan(my_chan,is_freq);
		ClrUsrChanFlag(is_freq);
	}

	if (GetInpValid(my_chan)==FALSE)
		qualified = FALSE;
	else if (IsPhaseGood(my_chan)==FALSE)
		qualified = FALSE;
	else if (GetInpEnab(my_chan, is_freq)==FALSE)
		qualified = FALSE;
   else if (GetPerfAcceptable(my_chan) == FALSE)
		qualified = FALSE;
   else if((my_chan == i_redChan) && ((e_redStatus == e_SC_ACTIVE) || (e_redStatus == e_SC_TRANSITION_TO_ACTIVE)))
		qualified = FALSE;

	if (qualified==TRUE)
	{
		if (e_selMode == e_SC_CHAN_SEL_QL)
		{	// if pql input selection
			// priority must greater than zero
			pri=GetInpPrio(my_chan, is_freq);
			if (pri>0)
			{
				pql = GetInpPql(my_chan, is_freq, &readExternally);
				if (pql>GetMyModulePql())
				{	// if this pql is worse than my clock, we should not choose
					bad_cnt[is_freq]++;
					if (bad_cnt[is_freq]<kREF_STABLE_CNT)
					{
						return -1;	// try at least xx times
					}
				}
				else
				{
					pri=GetInpPrio(my_chan, is_freq);
					if (pri==0)
					{
						bad_cnt[is_freq]++;
						if (bad_cnt[is_freq]<kREF_PRI_STABLE_CNT)
						{
							return -1;	// try at least xx times
						}
					}
					else
					{
	 					found = 1;
						my_pql = pql;
						bad_cnt[is_freq]=0;
					}
				}
			}
			else
			{
				bad_cnt[is_freq]++;
				if (bad_cnt[is_freq]<kREF_PRI_STABLE_CNT)
				{
					return -1;	// try at least xx times
				}
			}
		}
		else
		{	// if priority, we found it
			pri=GetInpPrio(my_chan, is_freq);
			if (pri==0)
			{
				bad_cnt[is_freq]++;
				if (bad_cnt[is_freq]<kREF_PRI_STABLE_CNT)
				{
					return -1;	// try at least xx times
				}
			}
			else
			{
				pql = GetInpPql(my_chan, is_freq, &readExternally);
				if (pql>GetMyModulePql())
				{	// if this pql is worse than my clock, we should not choose
					bad_cnt[is_freq]++;
					if (bad_cnt[is_freq]<kREF_STABLE_CNT)
					{
						return -1;	// try at least xx times
					}
				}
				else
				{
					found = 1;
					my_pql = pql;
					bad_cnt[is_freq]=0;
				}
			}
		}
	}

/* if redundancy is standby mode then force into holdover or redundancy lock */

   if(((e_redStatus == e_SC_STANDBY) || (e_redStatus == e_SC_TRANSITION_TO_STANDBY)) 
        && (o_redEnabled == TRUE))
   {
      if(i_redChan != -1)
      {
   		if(GetInpEnab(i_redChan, is_freq) &&
            GetInpValid(i_redChan) &&
            IsPhaseGood(i_redChan))
   		{
/* find assumed to report to current setting */
            if(is_freq)
            {
               redundant_pql = s_ChanConfig[i_redChan].b_ChanFreqAssumedQL;
            }
            else
            {
               redundant_pql = s_ChanConfig[i_redChan].b_ChanTimeAssumedQL;
            }

/* make redundant reference selected reference */
      		SetCurrPql(redundant_pql, is_freq);
      		SetCurrChan(i_redChan, is_freq);
      		*selectedChan = i_redChan;
            found = TRUE;  
   		}
   		else
         {
/* Holdover mode */
      		SetCurrPql(-1, is_freq);
      		SetCurrChan(-1, is_freq);
      		*selectedChan = -1;
         }
      }
      else
      {
/* no redundant channel configured */
     		SetCurrPql(-1, is_freq);
   		SetCurrChan(-1, is_freq);
   		*selectedChan = -1;
      }
   }
   else
   {
   	if (found)
   	{
   		SetCurrPql(my_pql, is_freq);
   		SetCurrChan(my_chan, is_freq);
   		/* save selected channel so we can recover it if we go into holdover because
   		it is disqualified and then come out of holdover if is requalified */
   		prev_chan = my_chan;
   		*selectedChan = my_chan;
   	}
   	else
   	{
   		SetCurrPql(-1, is_freq);
   		SetCurrChan(-1, is_freq);
   		*selectedChan = -1;
   	}
   }

	return found;
}

/*
----------------------------------------------------------------------------
                                Get_servoAccept()

Description:


Parameters:

Inputs

Outputs:

Return value:

-----------------------------------------------------------------------------
*/
#if 0
int Get_servoAccept(t_servoChanAcceptType *p_servoChanAccept)
{
  *p_servoChanAccept = s_servoChanAccept;
	return 0;
}
#endif

/*
----------------------------------------------------------------------------
                                Get_servoChanSelect()

Description: This function will copy over the entire servo channel table.
This function will be called by the servo task to determine which reference
to use as a reference.


Parameters:

Inputs

Outputs:

Return value:

-----------------------------------------------------------------------------
*/
int Get_servoChanSelect(SC_t_servoChanSelectTableType *p_servoChanSelectTable)
{
	 *p_servoChanSelectTable = s_servoChanSelectTable;
	return 0;
}
/*
----------------------------------------------------------------------------
                                Set_servoChanSelect()

Description:  This function will set the servo channel table.  It is only
used in debug to bypass the selection process.


Parameters:

Inputs

Outputs:

Return value:

-----------------------------------------------------------------------------
*/

int Set_servoChanSelect(SC_t_servoChanSelectTableType *p_servoChanSelectTable)
{
	s_servoChanSelectTable = *p_servoChanSelectTable;
//	printf("setting %d\n", s_servoChanSelectTable.s_servoChanSelect[1].b_enableFreq);
	return 0;
}


void Set_servoChanSelectValid(UINT16 chan_index, INT8 los)
{
   UINT8 user_chan;

	if(chan_index < NUM_OF_SERVO_CHAN)
	{
//		s_servoChanSelectTable.s_servoChanSelect[chan_index].o_valid = los;
// change to make signal_ok the los, o_valid should be telling servo if
// measurement is ok.
      user_chan = ServoIndex2UserChan(chan_index);
      s_chanStat2Tbl[user_chan].signal_ok = los;
	}

	return;
}

/*
----------------------------------------------------------------------------
                                SC_ChanSwtMode()


Description: This function is called by the user to set or get the channel
switch mode.  The values can be AR | AS | OFF.


Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the switch mode
		e_PARAM_SET - Set the switch mode

	SC_t_swtModeEnum *pe_swtMode
		Address to set or get switch mode

Outputs:
	SC_t_swtModeEnum *pe_swtMode
		Returned switch mode enumeration


Return value:
    0 - successful
   -1 - function failed
   -2 - invalid channel switch mode
   -3 - channels not configured yet

-----------------------------------------------------------------------------
*/
int SC_ChanSwtMode(t_paramOperEnum b_operation, SC_t_swtModeEnum *pe_swtMode)
{
   /* if channels not configured yet */
   if (!o_ChanConfigured)
      return -3;

   /* check for null pointer */
   if (pe_swtMode == NULL)
      return -1;

	switch(b_operation)
	{
      case e_PARAM_GET:
         *pe_swtMode = e_swtMode;
         break;
      case e_PARAM_SET:
         /* check channel switch mode input */
         switch (*pe_swtMode)
         {
            /* valid switch mode */
            case e_SC_SWTMODE_AR:
            case e_SC_SWTMODE_AS:
            case e_SC_SWTMODE_OFF:
               e_swtMode = *pe_swtMode;
               break;
            /* invalid switch mode falls through */
            default:
               return -2;
         }
         break;
      default:
         return -1;
   }

	return 0;
}
/*
----------------------------------------------------------------------------
                                SC_ChanSelMode()


Description: This function is called by the user to set or get the channel
select mode.  The values can be PRIORITY or PQL.


Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the channel select mode
		e_PARAM_SET - Set the channel select mode

	SC_t_selModeEnum *pe_selMode
		Address to set or get channel select mode

Outputs:
	SC_t_selModeEnum *pe_selMode
		Returned channel select mode enumeration


Return value:
    0 - successful
   -1 - function failed
   -2 - invalid channel select mode
   -3 - channels not configured yet

-----------------------------------------------------------------------------
*/
int SC_ChanSelMode(t_paramOperEnum b_operation, SC_t_selModeEnum *pe_selMode)
{
   /* if channels not configured yet */
   if (!o_ChanConfigured)
      return -3;

   /* check for null pointer */
   if (pe_selMode == NULL)
      return -1;

   switch(b_operation)
   {
      case e_PARAM_GET:
         *pe_selMode = e_selMode;
         break;
      case e_PARAM_SET:
         /* check channel select mode input */
         switch (*pe_selMode)
         {
            /* valid channel select mode */
            case e_SC_CHAN_SEL_PRIO:
            case e_SC_CHAN_SEL_QL:
               e_selMode = *pe_selMode;
               break;
            /* invalid channel select mode falls through */
            default:
               return -2;
         }
         break;
      default:
         return -1;
   }

   return 0;
}

/*
----------------------------------------------------------------------------
                                SC_SetSelChan()


Description: This function is called by the user to set the selected channel.


Parameters:

Inputs
	BOOLEAN o_isFreq
		TRUE  - entering frequency channel to select
		FALSE - entering time channel to select

	UINT8 b_ChanIndex
		Input channel to select

Outputs:
	None


Return value:
    0 - successful
   -1 - function failed
   -2 - failed - in AR mode
   -3 - channels not configured yet
   -4 - channel type does not support time

-----------------------------------------------------------------------------
*/
int SC_SetSelChan(BOOLEAN o_isFreq, UINT8 b_ChanIndex)
{
   /* if channels not configured yet */
   if (!o_ChanConfigured)
      return -3;

   /* check range of channel index */
   if(!GetInpChanValid(b_ChanIndex))
      return -1;

/* fail if trying to switch to redundant channel */
   if((s_ChanConfig[b_ChanIndex].e_ChanType) == e_SC_CHAN_TYPE_REDUNDANT)
      return -1;

   /* if trying to select channel for time mode */
   if (!o_isFreq)
   {
      /* Check for valid time mode per channel type */
      if (((s_ChanConfig[b_ChanIndex].e_ChanType) != e_SC_CHAN_TYPE_PTP) &&
         ((s_ChanConfig[b_ChanIndex].e_ChanType) != e_SC_CHAN_TYPE_GPS))
      {
         /* time mode not supported for this channel type */
         return -4;
      }
   }

   switch(e_swtMode)
   {
      case e_SC_SWTMODE_AR:
         return -2;
      case e_SC_SWTMODE_AS:
         SetUsrChan(b_ChanIndex,o_isFreq);
         break;
      case e_SC_SWTMODE_OFF:
         SetUsrChan(b_ChanIndex,o_isFreq);
         break;
      default:
         return -1;
   }

   return 0;
}
/*
----------------------------------------------------------------------------
                                SC_ChanEnable()


Description: This function is called by the user to set or get the channel's
frequency or time enable status.

Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the channel enable
		e_PARAM_SET - Set the channel enable

	UINT8 b_ChanIndex
		index for channel to enable/disable

	BOOLEAN o_isFreq
		TRUE  - enable/disable the frequency channel
		FALSE - enable/disable the time channel

	BOOLEAN *po_enable
		Address to set or get enable/disable status


Outputs:

	BOOLEAN *po_enable
		Returned enable/disable status of channel


Return value:
    0 - successful
   -1 - failed
   -2 - channel type does not support time
   -3 - channels not configured yet

-----------------------------------------------------------------------------
*/
int SC_ChanEnable (t_paramOperEnum b_operation, UINT8 b_ChanIndex, BOOLEAN o_isFreq, BOOLEAN *po_enable)
{
   /* if channels not configured yet */
   if (!o_ChanConfigured)
      return -3;

   /* check range of channel index */
  	if(!GetInpChanValid(b_ChanIndex))
    	return -1;

   /* check for null pointer */
   if (po_enable == NULL)
      return -1;

	switch(b_operation)
	{
      case e_PARAM_GET:
         /* if enable setting is for frequency */
         if(o_isFreq)
         {
            *po_enable = s_ChanConfig[b_ChanIndex].o_ChanFreqEnabled;
         }
         else
         {
            *po_enable = s_ChanConfig[b_ChanIndex].o_ChanTimeEnabled;
         }
         break;
      case e_PARAM_SET:
         /* if enable setting is for frequency */
         if(o_isFreq)
         {
            s_ChanConfig[b_ChanIndex].o_ChanFreqEnabled = (*po_enable ? TRUE : FALSE);
         }
         else
         {
            /* if setting time mode to enabled (TRUE) */
            if (*po_enable)
            {
               /* Check for valid time mode per channel type */
               if (((s_ChanConfig[b_ChanIndex].e_ChanType) == e_SC_CHAN_TYPE_PTP) ||
                  ((s_ChanConfig[b_ChanIndex].e_ChanType) == e_SC_CHAN_TYPE_GPS) || 
                  ((s_ChanConfig[b_ChanIndex].e_ChanType) == e_SC_CHAN_TYPE_REDUNDANT))
               {
                  /* set equal to true */
                  s_ChanConfig[b_ChanIndex].o_ChanTimeEnabled = TRUE;
               }
               else
               {
                  /* time mode not supported for this channel type */
                  return -2;
               }
            }
            else
            {
               /* set equal to false */
               s_ChanConfig[b_ChanIndex].o_ChanTimeEnabled = FALSE;
            }
         }
         break;
      default:
         return -1;
   }

   return 0;
}

/*
----------------------------------------------------------------------------
                                SC_ChanPriority()


Description: This function is called by the user to set or get the channel's
frequency or time priority value.

Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the channel's time or frequency priority value
		e_PARAM_SET - Set the channel's time or frequency priority value

	UINT8 b_ChanIndex
		index for channel to enable/disable

	BOOLEAN o_isFreq
		TRUE  - enable/disable the frequency channel
		FALSE - enable/disable the time channel

	UINT8 *pb_priority
		Address to set or get priority value


Outputs:

	UINT8 *pb_priority
		Returned priority value


Return value:
    0 - successful
   -1 - failed
   -2 - channels not configured yet
   -3 - priority value out of range for set operation

-----------------------------------------------------------------------------
*/
int SC_ChanPriority(t_paramOperEnum b_operation, UINT8 b_ChanIndex, BOOLEAN o_isFreq, UINT8 *pb_priority)
{
   /* if channels not configured yet */
   if (!o_ChanConfigured)
      return -2;

   /* check range of channel index */
  	if(!GetInpChanValid(b_ChanIndex))
    	return -1;

   /* check for null pointer */
   if (pb_priority == NULL)
      return -1;

	switch(b_operation)
	{
	   case e_PARAM_GET:
         /* check whether for frequency or time */
		   if(o_isFreq)
		   {
			   *pb_priority = s_ChanConfig[b_ChanIndex].b_ChanFreqPrio;
		   }
		   else
		   {
			   *pb_priority = s_ChanConfig[b_ChanIndex].b_ChanTimePrio;
		   }
		   break;
	   case e_PARAM_SET:
         /* check range of priority input */
         if (*pb_priority > k_CHAN_MAX_PRIO)
         {
            return -3;
         }
         /* check whether for frequency or time */
		   if(o_isFreq)
		   {
			   s_ChanConfig[b_ChanIndex].b_ChanFreqPrio = *pb_priority;
		   }
		   else
		   {
			   s_ChanConfig[b_ChanIndex].b_ChanTimePrio = *pb_priority;
		   }
		   break;
	   default:
		   return -1;
	}

	return 0;
}

/*
----------------------------------------------------------------------------
                                SC_ChanAssumeQL()


Description: This function is called by the user to set or get the channel's
frequency or time assumed quality level value.

Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the channel's time or frequency assumed quality level value
		e_PARAM_SET - Set the channel's time or frequency assumed quality level value

	UINT8 b_ChanIndex
		index for channel to enable/disable

	BOOLEAN o_isFreq
		TRUE  - enable/disable the frequency channel
		FALSE - enable/disable the time channel

	UINT8 *pb_ql
		Address to set or get assumed quality level value


Outputs:

	UINT8 *pb_ql
		Returned assumed quality level value


Return value:
    0 - successful
	-1 - failed
   -2 - channels not configured yet
   -3 - ql value out of range for a set operation

-----------------------------------------------------------------------------
*/
int SC_ChanAssumeQL(t_paramOperEnum b_operation, UINT8 b_ChanIndex, BOOLEAN o_isFreq, UINT8 *pb_ql)
{
   /* if channels not configured yet */
   if (!o_ChanConfigured)
      return -2;

   /* check range of channel index */
  	if(!GetInpChanValid(b_ChanIndex))
    	return -1;

   /* check for null pointer */
   if (pb_ql == NULL)
      return -1;

	switch(b_operation)
	{
	   case e_PARAM_GET:
         /* check whether for frequency or time */
		   if(o_isFreq)
		   {
			   *pb_ql = s_ChanConfig[b_ChanIndex].b_ChanFreqAssumedQL;
		   }
		   else
		   {
			   *pb_ql = s_ChanConfig[b_ChanIndex].b_ChanTimeAssumedQL;
		   }
		   break;
	   case e_PARAM_SET:
         /* check range of assumed QL input */
         if ((*pb_ql < k_CHAN_MIN_QL) || (*pb_ql > k_CHAN_MAX_QL))
         {
            return -3;
         }
         /* check whether for frequency or time */
		   if(o_isFreq)
		   {
			   s_ChanConfig[b_ChanIndex].b_ChanFreqAssumedQL = *pb_ql;
		   }
		   else
		   {
			   s_ChanConfig[b_ChanIndex].b_ChanTimeAssumedQL = *pb_ql;
		   }
		   break;
	   default:
		   return -1;
	}

	return 0;
}
/*
----------------------------------------------------------------------------
                                SC_ChanEnableAssumeQL()


Description: This function is called by the user to set or get the channel's
assumed QL enable/disable.

Parameters:

Inputs
	b_operation
	This enumeration defines the operation of the function to either
	set or get.
		e_PARAM_GET - Get the channel's assumed QL enable/disable
		e_PARAM_SET - Set the channel's assumed QL enable/disable

	UINT8 b_ChanIndex
		index for channel to enable/disable

	BOOLEAN *po_enable
		Address to set or get channel's assumed QL enable/disable

Outputs:

	BOOLEAN *po_enable
		Returned channel's assumed QL enable/disable


Return value:
    0 - successful
	-1 - failed
   -2 - channels not configured yet

-----------------------------------------------------------------------------
*/
int SC_ChanEnableAssumeQL(t_paramOperEnum b_operation, UINT8 b_ChanIndex, BOOLEAN *po_enable)
{
   /* if channels not configured yet */
   if (!o_ChanConfigured)
      return -2;

   /* check range of channel index */
  	if(!GetInpChanValid(b_ChanIndex))
    	return -1;

   /* check for null pointer */
   if (po_enable == NULL)
      return -1;

	switch(b_operation)
	{
	   case e_PARAM_GET:
		   *po_enable = s_ChanConfig[b_ChanIndex].o_ChanAssumedQLenabled;
		   break;
	   case e_PARAM_SET:
		   s_ChanConfig[b_ChanIndex].o_ChanAssumedQLenabled = (*po_enable ? TRUE : FALSE);
		   break;
	   default:
		   return -1;
	}

	return 0;
}

/*
----------------------------------------------------------------------------
                                SC_GetChanConfig()
Description:
This function gets the channel configuration parameters. The parameters will
be used when SC_InitConfigComplete() is called.
If a parameter is NULL no value is returned for that parameter.

Parameters:

Inputs
        as_ChanConfig[]
            Array of channel configuration data structures
        b_ChanCount
            Number of channels to be configured

Outputs:
        as_ChanConfig
        e_ChanSelMode
        e_ChanSwtMode
        b_ChanCount

Return value:
         0: function succeeded
        -1: function failed
        -2: SC_InitChanConfig() has not been run yet

-----------------------------------------------------------------------------
*/
int SC_GetChanConfig(
  SC_t_ChanConfig as_ChanConfig[],
  SC_t_selModeEnum* e_ChanSelMode,
  SC_t_swtModeEnum* e_ChanSwtMode,
  UINT8* b_ChanCount
)
{
   /*** local variables ***/
   UINT8 i;

   /* if channels not configured yet */
   if (!o_ChanConfigured)
      return -2;

   /* Copy local Channel Count to user space */
   if(b_ChanCount != NULL)
      *b_ChanCount = b_localChanCount;

   /* Copy local Channel Selection mode to user space */
   if(e_ChanSelMode != NULL)
      *e_ChanSelMode = e_selMode;

   /* Copy local Channel Switch mode to user space */
   if(e_ChanSwtMode != NULL)
      *e_ChanSwtMode = e_swtMode;

   /* Copy the config structure to user space */
   if(as_ChanConfig != NULL)
   {
      for (i = 0; (i < b_localChanCount) && (i < SC_TOTAL_CHANNELS); i++)
      {
         /* copy the structures to user space */
         as_ChanConfig[i] = s_ChanConfig[i];
      }
   }
   return 0;
}

/*
----------------------------------------------------------------------------
                                SC_InitChanConfig()
Description:
This function sets the channel configuration parameters. The parameters will
be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
        as_ChanConfig[]
            Array of channel configuration data structures
        e_ChanSelMode
            Channel selection mode
        e_ChanSwtMode
            Channel switch mode
        b_ChanCount
            Number of channels to be configured

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed
        -2: already running
        -3: invalid value in as_ChanConfig[]
        -4: channel type not supported in this product
        -5: channel type does not support time
        -6: too many channels configured
        -7: too many channels of specific type configured
        -8: invalid channel selection mode
        -9: invalid channel switch mode

-----------------------------------------------------------------------------
*/
int SC_InitChanConfig(
  SC_t_ChanConfig as_ChanConfig[],
  SC_t_selModeEnum e_ChanSelMode,
  SC_t_swtModeEnum e_ChanSwtMode,
  UINT8 b_ChanCount
)
{
   UINT8 i;

#if (SC_PTP_CHANNELS > 0)
   UINT8 b_PtpChanCnt = 0; /* Count to keep track of number of user configured PTP channels */
#endif

#if (SC_GPS_CHANNELS > 0)
   UINT8 b_GpsChanCnt = 0; /* Count to keep track of number of user configured GPS channels */
#endif

#if (SC_RED_CHANNELS > 0)
   UINT8 b_RedChanCnt = 0;  /* Count to keep track of number of user configured redundancy channels */
#endif

   UINT8 b_SeChanCnt = 0;  /* Count to keep track of number of user configured Sync-E channels */
   UINT8 b_SpanChanCnt = 0;  /* Count to keep track of number of user configured Span channels */
   UINT8 b_FreqChanCnt = 0;  /* Count to keep track of number of user configured frequency only channels */

/* init some global variables */
   e_redStatusLocal = e_SC_ACTIVE;
   o_redEnabledLocal = FALSE;
   i_redundantChan = -1;
   i_redundantPql = -1;
   i_redChan = -1;
   i_redPql = -1;
   b_localChanCount = 0;
   o_ChanConfigured = FALSE;
   dw_servoChanCntr = 0;
   e_swtMode = e_SC_SWTMODE_AR;
   e_selMode = e_SC_CHAN_SEL_PRIO;
   currChan_t = -1;
   currChan_f = -1;
   currPql_f=-1;
   currPql_t=-1;

#ifndef SERVO_ONLY
   /* check for init config complete */
	if (Is_InitComplete())
		return -2;
#endif

   /* check for null pointer */
   if (as_ChanConfig == NULL)
      return -1;

   /* Check for valid channel count */
   if (b_ChanCount > SC_TOTAL_CHANNELS)
   {
      return -6;
   }
   /* cycle through channel configuration array */
   for (i = 0; (i < b_ChanCount) && (i < (SC_TOTAL_CHANNELS)); i++)
   {
      /* Check for valid channel type and valid quantity of each type */
      switch (as_ChanConfig[i].e_ChanType)
      {
         /* Valid channel types */
#if (SC_PTP_CHANNELS > 0)
         case e_SC_CHAN_TYPE_PTP:
            /* if incremented channel count exceeds max number of this channel type */
            if ((++b_PtpChanCnt) > SC_PTP_CHANNELS)
            {
               return -7;
            }
            break;
#endif
#if (SC_GPS_CHANNELS > 0)
         case e_SC_CHAN_TYPE_GPS:
            /* if incremented channel count exceeds max number of this channel type */
            if ((++b_GpsChanCnt) > SC_GPS_CHANNELS)
            {
               return -7;
            }
            break;
#endif
#if (SC_SYNCE_CHANNELS > 0)
         case e_SC_CHAN_TYPE_SE:
            /* if incremented channel count exceeds max number of this channel type */
            if ((++b_SeChanCnt) > SC_SYNCE_CHANNELS)
            {
               return -7;
            }
            break;
#endif
#if (SC_SPAN_CHANNELS > 0)
         case e_SC_CHAN_TYPE_E1:
         case e_SC_CHAN_TYPE_T1:
            /* if incremented channel count exceeds max number of this channel type */
            if ((++b_SpanChanCnt) > SC_SPAN_CHANNELS)
            {
               return -7;
            }
            break;
#endif
#if (SC_RED_CHANNELS > 0)
         case e_SC_CHAN_TYPE_REDUNDANT:
            /* if incremented channel count exceeds max number of this channel type */
            if ((++b_RedChanCnt) > SC_RED_CHANNELS)
            {
               return -7;
            }
            /* set the redChan */
            i_redChan = i;
            break;
#endif
#if (SC_FREQ_CHANNELS > 0)
         case e_SC_CHAN_TYPE_FREQ_ONLY:
            /* if incremented channel count exceeds max number of this channel type */
            if ((++b_FreqChanCnt) > SC_FREQ_CHANNELS)
            {
               return -7;
            }
            break;
#endif
         /* Invalid channel types */
         default:
            /* invalid Channel type falls through */
           return -4;
           break;
      }

      /* Check for valid time mode per channel type */
      if (((as_ChanConfig[i].e_ChanType) != e_SC_CHAN_TYPE_PTP) &&
            ((as_ChanConfig[i].e_ChanType) != e_SC_CHAN_TYPE_GPS) &&
            ((as_ChanConfig[i].e_ChanType) != e_SC_CHAN_TYPE_REDUNDANT))
      {
         /* if time mode is enabled for a channel type that does not support it */
         if (as_ChanConfig[i].o_ChanTimeEnabled)
         {
            return -5;
         }
      }

      /* Check for valid frequency priority */
      if (as_ChanConfig[i].b_ChanFreqPrio > k_CHAN_MAX_PRIO)
      {
         return -3;
      }
      /* Check for valid time priority */
      if (as_ChanConfig[i].b_ChanTimePrio > k_CHAN_MAX_PRIO)
      {
         return -3;
      }
      /* Check for frequency assumed QL */
      if ((as_ChanConfig[i].b_ChanFreqAssumedQL < k_CHAN_MIN_QL) ||
            (as_ChanConfig[i].b_ChanFreqAssumedQL > k_CHAN_MAX_QL))
      {
         return -3;
      }
      /* Check for valid time assumed QL */
      if ((as_ChanConfig[i].b_ChanTimeAssumedQL < k_CHAN_MIN_QL) ||
            (as_ChanConfig[i].b_ChanTimeAssumedQL > k_CHAN_MAX_QL))
      {
         return -3;
      }

      /* Copy user channel config structure to SC global */
      s_ChanConfig[i] = as_ChanConfig[i];

      /* enter user channel number into translation table */
      /* order is PTP, GPS, SE */
      switch(as_ChanConfig[i].e_ChanType)
      {
#if (SC_PTP_CHANNELS > 0)
      case e_SC_CHAN_TYPE_PTP:
        as_userChan2servoIndexTbl[i] = b_PtpChanCnt - 1;
        break;
#endif
#if (SC_GPS_CHANNELS > 0) || (SC_OTHER_CHANNELS > 0)
      case e_SC_CHAN_TYPE_GPS:
        as_userChan2servoIndexTbl[i] = SERVO_CHAN_GPS1 + b_GpsChanCnt - 1;
        break;
#endif
#if (SC_SYNCE_CHANNELS > 0) || (SC_OTHER_CHANNELS > 0)
      case e_SC_CHAN_TYPE_SE:
        as_userChan2servoIndexTbl[i] = SERVO_CHAN_SYNCE1 + b_SeChanCnt + b_SpanChanCnt + b_FreqChanCnt - 1;
        break;
#endif
#if (SC_SPAN_CHANNELS > 0) || (SC_OTHER_CHANNELS > 0)
      case e_SC_CHAN_TYPE_E1:
      case e_SC_CHAN_TYPE_T1:
        as_userChan2servoIndexTbl[i] = SERVO_CHAN_SYNCE1 + b_SeChanCnt + b_SpanChanCnt + b_FreqChanCnt - 1;
        break;
#endif
#if (SC_RED_CHANNELS > 0) || (SC_OTHER_CHANNELS > 0)
      case e_SC_CHAN_TYPE_REDUNDANT:
        as_userChan2servoIndexTbl[i] = SERVO_CHAN_RED + b_RedChanCnt - 1;
        break;
#endif
#if (SC_FREQ_CHANNELS > 0) || (SC_OTHER_CHANNELS > 0)
      case e_SC_CHAN_TYPE_FREQ_ONLY:
        as_userChan2servoIndexTbl[i] = SERVO_CHAN_SYNCE1 + b_SeChanCnt + b_SpanChanCnt + b_FreqChanCnt - 1;
        break;
#endif
      default:    
        //printf("hi\n");
        return -4;
        break;
      }
		printf("%s: userChan2servoIndexTbl[%d] %d..\n", __FUNCTION__, i, as_userChan2servoIndexTbl[i]);
   } /* for (i=0, i < b_ChanCount && i < SC_TOTAL_CHANNELS) */

   /* Check out channel selection mode */
   switch (e_ChanSelMode)
   {
      /* Valid Selection Modes */
      case e_SC_CHAN_SEL_PRIO:
      case e_SC_CHAN_SEL_QL:
         /* break out of switch */
         break;
      /* invalid selection mode */
      default:
         return -8;
         break;
   }

   /* Check out channel switch mode */
   switch (e_ChanSwtMode)
   {
      /* Valid switch Modes */
      case e_SC_SWTMODE_AR:
      case e_SC_SWTMODE_AS:
      case e_SC_SWTMODE_OFF:
         /* break out of switch */
         break;
      /* invalid switch mode */
      default:
         return -9;
         break;
   }

   /* Copy user Channel Switch mode to global variable */
   e_swtMode = e_ChanSwtMode;

   /* Copy user Channel Selection mode to global variable */
   e_selMode = e_ChanSelMode;

   /* Copy user total channel count to global variable */
   b_localChanCount = b_ChanCount;

   /* flag that SC_InitChanConfig() completed successfully,
      now there should be a valid config structure */
   o_ChanConfigured = TRUE;

	return 0;
}

/*
----------------------------------------------------------------------------
                                SC_GetChanStatus()
Description:
This function will retrieve the channel status of all the configured channels

Parameters:

Inputs
        b_ChanCount
            Number of channels (must match already configured number)

       pas_chanStatus[]
            Pointer to address to copy channel status information into.

Outputs:
       pas_chanStatus[]
            A copy of the channel status information.

Return value:
         0: function succeeded
        -1: function failed
        -2: channel count does not match configured channel count
        -3: channels not configured yet

-----------------------------------------------------------------------------
*/
int SC_GetChanStatus(UINT8 b_ChanCount, SC_t_chanStatus pas_chanStatus[])
{
   int i, v;
   UINT8 b_servoChan;
   BOOLEAN readExternally;

   /* if channels not configured yet */
   if (!o_ChanConfigured)
      return -3;

   /* check for channel count the same */
   if(b_ChanCount != b_localChanCount)
      return -2;

   /* check for null pointer */
   if (pas_chanStatus == NULL)
      return -1;

   for(i=0;i<b_ChanCount;i++)
   {
      /* tranlate user channel to servo index */
      b_servoChan = UserChan2ServoIndex(i);

      /* load Frequency weight */
      if(s_servoChanSelectTable.s_servoChanSelect[b_servoChan].b_selectedFreq)
      {
         pas_chanStatus[i].b_freqWeight = 100;
      }
      else
      {
         pas_chanStatus[i].b_freqWeight = 0;
      }

      /* load Time weight */
      if(s_servoChanSelectTable.s_servoChanSelect[b_servoChan].b_selectedTime)
      {
         pas_chanStatus[i].b_timeWeight = 100;
      }
      else
      {
         pas_chanStatus[i].b_timeWeight = 0;
      }


      /* load Frequency status */
      if (GetInpEnab(i, 1)==FALSE)
         v = e_SC_CHAN_STATUS_DIS;
      else if (prevF_QuaTbl[i]==0)
         v = e_SC_CHAN_STATUS_FLT;
      else
         v = e_SC_CHAN_STATUS_OK;
      pas_chanStatus[i].e_chanFreqStatus = v;

      /* load Time status */
      if (GetInpEnab(i, 0)==FALSE)
         v = e_SC_CHAN_STATUS_DIS;
      else if (prevT_QuaTbl[i]==0)
         v = e_SC_CHAN_STATUS_FLT;
      else
         v = e_SC_CHAN_STATUS_OK;
      pas_chanStatus[i].e_chanTimeStatus = v;

      /* load quality level */
      readExternally = FALSE;
      pas_chanStatus[i].b_freqQL = GetInpPql(i, TRUE, &readExternally);
      pas_chanStatus[i].b_qlReadExternally = readExternally;
      readExternally = FALSE;
      pas_chanStatus[i].b_timeQL = GetInpPql(i, FALSE, &readExternally);
      // b_qlReadExternally is the OR of the frequency and time values
      pas_chanStatus[i].b_qlReadExternally = pas_chanStatus[i].b_qlReadExternally || readExternally;

      /* load fault map */
      v = 0;
      if (IsPhaseGood(i)==FALSE)
         v |= 0x4;
      if (prev_ValidTbl[i]==0)
         v |= 0x2;
      if (prev_PerformanceFltTbl[i])
         v |= 0x1;

      pas_chanStatus[i].l_faultMap = v;
   }

   return 0;
}

/*
----------------------------------------------------------------------------
                                SC_OutQualityLevel()


Description: Called by Host. This function provides the current output quality level.
If there is an active input, the value will be the quality level associated with that
input. If there is no active input, the value will be the quality level associated
with the internal reference oscillator.

Parameters:

Inputs
	BOOLEAN o_isFreq
		TRUE  - frequency channel
		FALSE - the time channel

Outputs:

	None

Return value:
	function succeeded.
	The returned value is the output quality level, a value in the range
	1..16

	Function failed
	-1: any failure not covered by other failure codes
-----------------------------------------------------------------------------
*/
int SC_OutQualityLevel(BOOLEAN o_isFreq)
{
	int pql;

	pql = GetCurrPql(o_isFreq);

	return pql;
}

/*
----------------------------------------------------------------------------
                                Is_ChanConfigured()

Description:
This function will return TRUE if the channel configuration is complete.

Parameters:

Inputs
      None

Outputs:
      None

Return value:
      TRUE - Channel configuration is complete.
      FALSE - Channel configuration is not complete.

-----------------------------------------------------------------------------
*/
BOOLEAN Is_ChanConfigured(void)
{
   return o_ChanConfigured;
}

/*
----------------------------------------------------------------------------
                                Set_ChanConfigured()
Description:
This function sets the channel configured flag.

Parameters:

Inputs
      o_ConfiguredSetVal - value with which to set channel configured flag

Outputs:
      None

Return value:
      None

-----------------------------------------------------------------------------
*/
void Set_ChanConfigured(BOOLEAN o_ConfiguredSetVal)
{
   o_ChanConfigured = (o_ConfiguredSetVal ? TRUE : FALSE);
}

/*
----------------------------------------------------------------------------
                                Get_servoChanSelectRate()
Description:
This function reads the rate of the channel given the servo index number

Parameters:

Inputs
        None

Outputs:
       None

Return value:
         TRUE: Channel config has been performed.
         FALSE:Channel config has not been performed.

-----------------------------------------------------------------------------
*/
int Get_servoChanSelectRate(UINT16 chan_index, INT16 *rate)
{
	if(chan_index < NUM_OF_SERVO_CHAN)
	{
		*rate = s_servoChanSelectTable.s_servoChanSelect[chan_index].w_measRate;
	}
	else
	{
		*rate = 1;
		return 1;
	}

	return 0;
}

/*--------------------------------------------------------------------------
    void printChanStatus(void)

Description:
Prints the channel status when the debug flag CHAN_STAT_PRT is enabled
Tries to use a similar, but lightweight format to the used in the
channel status SCRDB menu option.
This function duplicates some of the functionality of SC_GetChanStatus to avoid
calling it because SC_GetChanStatus needs the caller to allocate memory and
the servo should not use any dynamic memory.


Return value:
  None

Parameters:
  None

Global variables affected and border effects:
  Read
    b_localChanCount
    s_ChanConfig
    s_servoChanSelectTable

  Write

--------------------------------------------------------------------------*/

void printChanStatus(void)
{
  static UINT8 passCounter = 0;
  int i;
  UINT8 servoChanIdx;
  char chanST;
  char chanFltTbl[3];
  BOOLEAN readExternally;
  UINT8 qualityLevel;
  t_servoConfigType servoConfig;
  SC_t_swtModeEnum swtMode = -1;
  SC_t_selModeEnum selMode = -1;
  char strBuf[200];
  char strBuf2[100];

  strBuf[0] = strBuf2[0] = 0;
  //General servo config info that will not change
  //only print every 64 pass
  if((passCounter % 64) == 0)
  {
    strncat(strBuf, "ServoConfig: ", sizeof(strBuf)-strlen(strBuf)-1);
    if(SC_GetServoConfig(&servoConfig) != 0)
    {
      strncat(strBuf, "NA\n", sizeof(strBuf)-strlen(strBuf)-1);
    }
    else
    {
      snprintf(strBuf2, sizeof(strBuf2), "lo=%d, loQl=%d, transport=%d, phm=%lu, fc=%f, fct=%lld, ffc=%f, ffct=%lld, brg=%lu\n",
          servoConfig.e_loType, servoConfig.b_loQl, servoConfig.e_ptpTransport, (long unsigned int)servoConfig.dw_ptpPhaseMode,
          servoConfig.f_freqCorr, servoConfig.s_freqCorrTime.u48_sec, servoConfig.f_freqCalFactory,
          servoConfig.s_freqCalFactoryTime.u48_sec, (long unsigned int)servoConfig.dw_bridgeTime);
      strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);
    }
    debug_printf(CHAN_STAT_PRT, strBuf);
    strBuf[0] = strBuf2[0] = 0;
  }

  //Values that can change via API calls
  //only print every 32 pass
  strBuf[0] = strBuf2[0] = 0;
  if(o_ChanConfigured && ((passCounter % 32) == 0))
  {
    //Values that can change via API calls
    SC_ChanSwtMode(e_PARAM_GET, &swtMode);
    SC_ChanSelMode(e_PARAM_GET, &selMode);
    snprintf(strBuf2, sizeof(strBuf2), "SwtMode=%d, SelMode=%d\n", swtMode, selMode);
    strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);

    for(i=0; i < b_localChanCount; i++)
    {
      snprintf(strBuf2, sizeof(strBuf2), "Chan %d config: ", i);
      strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);
      snprintf(strBuf2, sizeof(strBuf2), "ctyp=%d, fp=%d, fe=%d, tp=%d, te=%d, aqle=%d, faql=%d, taql=%d, mint=%ld\n",
          s_ChanConfig[i].e_ChanType, s_ChanConfig[i].b_ChanFreqPrio, s_ChanConfig[i].o_ChanFreqEnabled,
          s_ChanConfig[i].b_ChanTimePrio, s_ChanConfig[i].o_ChanTimeEnabled, s_ChanConfig[i].o_ChanAssumedQLenabled,
          s_ChanConfig[i].b_ChanFreqAssumedQL, s_ChanConfig[i].b_ChanTimeAssumedQL, (long int)s_ChanConfig[i].l_MeasRate);
      strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);
    }
    debug_printf(CHAN_STAT_PRT, strBuf);
    strBuf[0] = strBuf2[0] = 0;
  }

  //Values that change during normal operation
  if(o_ChanConfigured)
  {
    //debug_printf(CHAN_STAT_PRT, "SC_GetChanStatus:\n");
    for(i=0; i < b_localChanCount; i++)
    {
      snprintf(strBuf2, sizeof(strBuf2), "Chan %d status:", i);
      strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);
      servoChanIdx = UserChan2ServoIndex(i);
      if(servoChanIdx == 0xff)
      {
        snprintf(strBuf2, sizeof(strBuf2), " NA\n");
        strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);
      }
      else
      {
        //frequency status
        if (GetInpEnab(i, 1) == FALSE)
          chanST = 'D';     // e_SC_CHAN_STATUS_DIS
        else if (prevF_QuaTbl[i] == 0)
          chanST = 'F';     //e_SC_CHAN_STATUS_FLT
        else
          chanST = 'K';     //e_SC_CHAN_STATUS_OK
        snprintf(strBuf2, sizeof(strBuf2), " %c", chanST); //status
        strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);

        //frequency weight
        snprintf(strBuf2, sizeof(strBuf2), ", %d", s_servoChanSelectTable.s_servoChanSelect[servoChanIdx].b_selectedFreq); //weight
        strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);

        //time status
        if (GetInpEnab(i, 0) == FALSE)
          chanST = 'D';     // e_SC_CHAN_STATUS_DIS
        else if (prevT_QuaTbl[i] == 0)
          chanST = 'F';     //e_SC_CHAN_STATUS_FLT
        else
          chanST = 'K';     //e_SC_CHAN_STATUS_OK
        snprintf(strBuf2, sizeof(strBuf2), ", %c", chanST); //status
        strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);

        //time weight
        snprintf(strBuf2, sizeof(strBuf2), ", %d", s_servoChanSelectTable.s_servoChanSelect[servoChanIdx].b_selectedFreq); //weight
        strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);

        //frequency quality level
        qualityLevel = GetInpPql(i, TRUE, &readExternally);
        snprintf(strBuf2, sizeof(strBuf2), ", %d%c", qualityLevel, (readExternally)?'*':'-');
        strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);

        //time quality level
        qualityLevel = GetInpPql(i, FALSE, &readExternally);
        snprintf(strBuf2, sizeof(strBuf2), ", %d%c", qualityLevel, (readExternally)?'*':'-');
        strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);

        //fault table
        chanFltTbl[0] = (IsPhaseGood(i) == FALSE)?'F':'K';
        chanFltTbl[1] = (prev_ValidTbl[i] == 0)?'F':'K';
        chanFltTbl[2] = (prev_PerformanceFltTbl[i])?'F':'K';
        snprintf(strBuf2, sizeof(strBuf2), ", %c, %c, %c", chanFltTbl[0], chanFltTbl[1], chanFltTbl[2]);
        strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);

        snprintf(strBuf2, sizeof(strBuf2), ", %d\n", s_servoChanSelectTable.s_servoChanSelect[servoChanIdx].w_measRate);
        strncat(strBuf, strBuf2, sizeof(strBuf)-strlen(strBuf)-1);
      }

      debug_printf(CHAN_STAT_PRT, strBuf);
      strBuf[0] = strBuf2[0] = 0;
    }
  }

  passCounter++;
}

/* SSM to PQL Lookup Tables */
// DS1 and E1 table will be updated by spi command 0x2B
static UINT8 gT1_SSM_TABLE[16] =
  {0x88,0x04,0x84,0x08,0x0c,0x8c,0x78,0x7c,0x90,0x10,0xa2,0x22,0x28,0x40,0x30,0xc0};
static UINT8 gE1_SSM_TABLE[16] =
  {0x80,0x82,0x02,0x00,0x84,0x84,0x04,0x88,0x08,0x8b,0x0b,0x8f,0x8f,0x8f,0x0f,0x8f};
static UINT8 gSE_SSM_TABLE[16] =
  {0x81,0x01,0x82,0x02,0x03,0x84,0x84,0x84,0x84,0x04,0x85,0x05,0x06,0x87,0x07,0x87};
static UINT8 gPTP_SSM_TABLE[16] =
  {80,82,84,86,88,90,92,94,96,98,100,102,104,106,108,110};

/* This routine return a pql value if the given ssm found in SE look-up table */
/* return 1 to 16 if found */
/* return -1, if not found */
static int FindSsm2Pql(UINT8 chanType, UINT8 ssm)
{
  int i;
  int pql = -1;
  UINT8 *lut;

  switch(chanType)
  {
    case e_SC_CHAN_TYPE_PTP:
      lut = &gPTP_SSM_TABLE[0];
      break;

    case e_SC_CHAN_TYPE_SE:
      lut = &gSE_SSM_TABLE[0];
      break;

    case e_SC_CHAN_TYPE_E1:
      lut = &gE1_SSM_TABLE[0];
      break;

    case e_SC_CHAN_TYPE_T1:
      lut = &gT1_SSM_TABLE[0];
      break;

    case e_SC_CHAN_TYPE_FREQ_ONLY:
    case e_SC_CHAN_TYPE_REDUNDANT:
      //no translation for this channels
      return ssm;
      break;

    default:
      return -1;
      break;
  }

  for (i=0;i<16; i++)
  {
    if (lut[i] == ssm)
    {
      pql = i + 1;
      break;
    }
  }

  return pql;
}

/*
----------------------------------------------------------------------------
                                SC_QlToSSM()


Description: Called by Host. This function provides conversion from specified
quality level to an SSM value associated with the specified channel-type.
The primary purpose of the function is to provide a method for user to learn
the channel-specific SSM value that should be used if an output of that
channel-type is being generated from SoftClock

Parameters:

Inputs
	UINT8	b_quality_level
	SC_t_Chan_Type
			e_chan_type
	BOOLEAN *b_standard
		TRUE  - standard
		FALSE - non-standard

Outputs:

	None

Return value:
	>=0 function succeeded.
	The returned value is the SSM (or appropriate) value associated
	with the specified quality level and channel type

	Function failed
	-1: any failure not covered by other failure codes
	-2: if all input parameters are valid, but channel does not support SSM,
	    such as GPS, Redundancy or frequency only channel
-----------------------------------------------------------------------------
*/
int SC_QlToSSM(UINT8 b_quality_level, SC_t_Chan_Type e_chan_type, BOOLEAN *b_standard)
{
  int ssm=-1, v;
  int pql= b_quality_level;

  if (!((pql>=1) && (pql<=16)))
  {
    return -1;
  }

  switch(e_chan_type)
  {
    case e_SC_CHAN_TYPE_PTP:
      v = gPTP_SSM_TABLE[pql-1];
      ssm = v & 0x7f;
      *b_standard = TRUE;
      break;

    case e_SC_CHAN_TYPE_GPS:
    case e_SC_CHAN_TYPE_REDUNDANT:
    case e_SC_CHAN_TYPE_FREQ_ONLY:
      return -2;
      break;

    case e_SC_CHAN_TYPE_SE:
      v = gSE_SSM_TABLE[pql-1];
      ssm = v & 0x7f;
      *b_standard = ((v & 0x80)==0) ? TRUE : FALSE;
      break;

    case e_SC_CHAN_TYPE_E1:
      v = gE1_SSM_TABLE[pql-1];
      ssm = v & 0x7f;
      *b_standard = ((v & 0x80)==0) ? TRUE : FALSE;
      break;

    case e_SC_CHAN_TYPE_T1:
      v = gT1_SSM_TABLE[pql-1];
      ssm = v & 0x7f;
      *b_standard = ((v & 0x80)==0) ? TRUE : FALSE;
      break;

    default:
      return -1;
      break;
  }


  return ssm;
}

static void UpdateValidReportToServo(void)
{
   int i;
   UINT8 servo_chan;
   BOOLEAN o_valid = FALSE;


/* go through all user channels and find out if user has disqualified the input */
   for(i=0;i<b_localChanCount;i++)
   {
      servo_chan = UserChan2ServoIndex(i);

/* is valid if either frequeny or time is enabled */
      if(GetInpEnab(i, TRUE) || GetInpEnab(i, FALSE))
      {
      	o_valid = TRUE;
      }
      else
      {
		printf("%s: inp disable..\n", __FUNCTION__);
         o_valid  = FALSE;
      }

      if(!GetInpValid(i))
      {
		printf("%s: inp invalid..\n", __FUNCTION__);
         o_valid = FALSE;
      }

/* if LOS then not valid */
      if(!s_chanStat2Tbl[i].signal_ok)
      {
		printf("%s: inp signal not ok\n", __FUNCTION__);
         o_valid = FALSE;
      }
      s_servoChanSelectTable.s_servoChanSelect[servo_chan].o_valid = o_valid;
   }
}
/*
----------------------------------------------------------------------------
                                SC_GetEventMap()

Description:
Called by user, this function was return the current state of all the events in
the form of a bit map.


Parameters:

Inputs
	UINT32 *pdw_eventMap
	Address of UINT32 memory location where the results will be stored.

Outputs:
	UINT32 *pdw_eventMap
	The bit mapped events.  The position of the event is based on the event's
	ID number.  For example, if there is a '1' in the bit 0 position, then the
	SoftClock is currently in Holdover mode (see sc_api.h for event ID numbers.)


Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int SC_GetEventMap(UINT32 *pdw_eventMap)
{
   UINT32 dw_bitMap = 0;
   Fll_Clock_State fll_state = get_fll_state();
   int i;
   int num_of_chans = b_localChanCount;
   int event_true_flag;

#ifndef SERVO_ONLY
/* if initialization not completed, then fail */
   if(!Is_InitComplete())
   {
      return -1;
   }
#endif

#if 1
/* check e_HOLDOVER 0 */
   if(fll_state == FLL_HOLD)
      dw_bitMap |= 1 << e_HOLDOVER;

/* check e_FREERUN 1 */
   if(fll_state == FLL_WARMUP)
      dw_bitMap |= 1 << e_FREERUN;

/* check e_BRIDGING 2 */
   if(fll_state == FLL_BRIDGE)
      dw_bitMap |= 1 << e_BRIDGING;

/* check e_CHAN_NO_DATA 3 */
   event_true_flag = FALSE;
   for(i=0;i<num_of_chans;i++)
   {
      if(!prev_HaveDataTbl[i])
         event_true_flag = TRUE;
   }
   if(event_true_flag)
      dw_bitMap |= 1 << e_CHAN_NO_DATA;

/* check e_CHAN_PERFORMANCE 4 */
   event_true_flag = FALSE;
   for(i=0;i<num_of_chans;i++)
   {
      if(prev_PerformanceFltTbl[i])
         event_true_flag = TRUE;
   }
   if(event_true_flag)
      dw_bitMap |= 1 << e_CHAN_PERFORMANCE;

/* check e_FREQ_CHAN_QUALIFIED 5 */
   event_true_flag = FALSE;
   for(i=0;i<num_of_chans;i++)
   {
      if(prevF_QuaTbl[i])
         event_true_flag = TRUE;
   }
   if(event_true_flag)
      dw_bitMap |= 1 << e_FREQ_CHAN_QUALIFIED;

/* check e_FREQ_CHAN_SELECTED 6 */
   event_true_flag = FALSE;
   for(i=0;i<num_of_chans;i++)
   {
      if(prevF_SelTbl[i])
         event_true_flag = TRUE;
   }
   if(event_true_flag)
      dw_bitMap |= 1 << e_FREQ_CHAN_SELECTED;

/* check e_TIME_CHAN_QUALIFIED 7 */
   event_true_flag = FALSE;
   for(i=0;i<num_of_chans;i++)
   {
      if(prevT_QuaTbl[i])
         event_true_flag = TRUE;
   }
   if(event_true_flag)
      dw_bitMap |= 1 << e_TIME_CHAN_QUALIFIED;

/* check e_TIME_CHAN_SELECTED 8 */
   event_true_flag = FALSE;
   for(i=0;i<num_of_chans;i++)
   {
      if(prevT_SelTbl[i])
         event_true_flag = TRUE;
   }
   if(event_true_flag)
      dw_bitMap |= 1 << e_TIME_CHAN_SELECTED;

/* check e_CHAN_NOT_VALID 9 */
   event_true_flag = FALSE;
   for(i=0;i<num_of_chans;i++)
   {
      if(!prev_ValidTbl[i])
         event_true_flag = TRUE;
   }
   if(event_true_flag)
      dw_bitMap |= 1 << e_CHAN_NOT_VALID;

/* check e_TIMELINE_CHANGED 10 not reported, transient */
/* check e_PHASE_ALIGNMENT 11 not reported, transient */
#endif
   *pdw_eventMap = dw_bitMap;
   
   return 0;
}



/*
static void GetRedChan(int *pi_redundantChan, int *pi_redundantPql)
{
   *pi_redundantChan = i_redundantChan;
   *pi_redundantPql = i_redundantPql;
}
*/
/*
----------------------------------------------------------------------------
                                SC_GetRedundantStatus()

Description:
This function is called by the user to read the redundancy status information
from the SoftClock.

Parameters:

Inputs
        BOOLEAN *po_redEnabled
        Address of BOOLEAN memory location where the results will be stored. 
        
        SC_t_activeStandbyEnum *pe_redStatus;
        Address of BOOLEAN memory location where the results will be stored.

Outputs:
        BOOLEAN *po_redEnabled
        1 - The redundancy channel is enabled
        0 - The redundancy channel is disabled
        
        SC_t_activeStandbyEnum *pe_redStatus;
        The current redundancy status state as expressed by SC_t_activeStandbyEnum
        enumeration.


Return value:
	0: successful
       -1: function failed

-----------------------------------------------------------------------------
*/
int SC_GetRedundantStatus(BOOLEAN *po_redEnabled, SC_t_activeStandbyEnum *pe_redStatus)
{
/* fail if there is no redundant channel */
   if(i_redChan == -1)
   {
      return -1;
   }

  *po_redEnabled = o_redEnabledLocal;
  *pe_redStatus = e_redStatusLocal;

  return 0;
}
/*
----------------------------------------------------------------------------
                                SC_RedundantEnable()
Description:
This function is called by user to enable or disable the redundancy.

Parameters:

Inputs
        BOOLEAN o_redEnabled
        0 - Disable Redundancy (go to Active mode)
        1 - Enable Redundancy (go to Standby mode)

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed

-----------------------------------------------------------------------------
*/
int SC_RedundantEnable(BOOLEAN o_redEnabled)
{
/* fail if there is no redundant channel */
   if(i_redChan == -1)
   {
      return -1;
   }

   if(o_redEnabledLocal != o_redEnabled)
   {
      if(o_redEnabled)
      {
         e_redStatusLocal = e_SC_STANDBY; 
      }
      else
      {
         e_redStatusLocal = e_SC_ACTIVE; 
      }
   }
  o_redEnabledLocal = o_redEnabled;

  return 0;
}

int Get_RedChan(void)
{
   return i_redChan;
}

/*
----------------------------------------------------------------------------
                                SC_LoadMode()


Description: This function is called by the user to query the Load Mode
of the PTP channel.  


Parameters:

Inputs
	UINT8 b_ChanIndex
	The PTP channel index number of the queried Load Mode  

	UINT16 *w_LoadMode
	The address that the Load Mode should be stored.

Outputs:
	UINT16 *w_LoadMode
	The Load Mode is returned in this pointer.

Return value:
    0 - successful
   -1 - function failed (Not a PTP channel, or not valid channel index)

-----------------------------------------------------------------------------
*/
int SC_LoadMode(UINT8 b_ChanIndex, UINT16 *w_LoadMode)
{
   int ret = 0;
  	UINT8 b_servoIndex;

	/* check channel number */
	if(b_ChanIndex >= GetNumberOfChan())
	{
		return -1;
	}

  	/* convert to servo index */
  	b_servoIndex = UserChan2ServoIndex(b_ChanIndex);

  	if(b_servoIndex == 0xFF)
    	return -1;
	
	if(s_ChanConfig[b_ChanIndex].e_ChanType != e_SC_CHAN_TYPE_PTP)
		return -1;

	if(Get_Load_Mode(b_servoIndex, w_LoadMode))
		return -1;

   return ret;
}
