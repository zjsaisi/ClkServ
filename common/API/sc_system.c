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

FILE NAME    : sc_system.c

AUTHOR       : Ken Ho

DESCRIPTION  : 

The functions in this file are used to set up the SoftClient System.
	SC_Init()
	SC_Run()
	SC_Shutdown()
	SC_Timer()


Revision control header:
$Id: API/sc_system.c 1.46 2011/08/31 10:55:35PDT Kenneth Ho (kho) Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "target.h"
#include "sc_types.h"
#include "PTP/PTPdef.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "THD/THD.h"
#include "DBG/DBG.h"
#include "CLK/clk_private.h"
#include "CLK/ptp.h"
#include "GNS/GN_GPS_Task.h"
#include "GPS/GPS.h"
#include "SE/SE.h"
#include "E1/E1.h"
#include "T1/T1.h"
#include "RED/Red.h"
#include "sc_api.h"
#include "sc_rev.h"
#include "sc_system.h"
#include "sc_chan.h"
#include "sc_chan_local.h"

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
static t_ptpGeneralConfig s_ptpGenCfg =
	{e_CLASS_SLV_ONLY, 255, 255, 18944, 0, TRUE, FALSE,
	 {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
	 "Symmetricom IEEE1588-2008 SCi"};
static t_ptpPortConfig s_ptpPortCfg[k_NUM_IF] =
	{{e_DEL_MECH_E2E, -6, 1, 3, -6, -6}};
static t_ptpProfileConfig s_ptpProfCfg =
	{e_ADDR_MODE_UNICAST, e_BMC_MODE_DEFAULT};
static t_ucMasterType s_lclUcMstTbl = 
	{TRUE, 300, 1, 0,
         {{e_SC_UDP_IPv4, 4, {0, 0, 0, 0}},
	  {e_SC_UDP_IPv4, 4, {0, 0, 0, 0}}}};
static PTP_t_ProfRng s_ptpProfRng =
	{{0, 1, 3}, {2, 3, 10}, {-6, -6, 1}, {-6, -6, 1},
	 {0, 0, 127}, {255, 255, 255}, {255, 255, 255}};
static UINT16 aw_leastDurRng[k_RNG_SZE] = {10, 300, 1000};
static INT8 aw_ucQueryIntvRng[k_RNG_SZE] = {0, 1, 4};

static BOOLEAN o_initDone = FALSE;

UINT16 w_leaseDuration = 300;

#ifdef GPS_BUILD
static SC_t_GPS_GeneralConfig s_gpsGenConfig =
   {FALSE, e_RCVR_ST_ERIC_GNS7560_BGA, 0, FALSE, 0, 0, 0};
#endif

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/
static void SC_Close(void);
static void SC_ErrCallback(UINT32          dw_unitId,
                           UINT32          dw_errNmb,
                           PTP_t_sevCdEnum e_sevCode,
                           PTP_t_TmStmp    *ps_ts,
                           BOOLEAN         o_ptpRunning);

extern int SC_InitMnt(void);
extern int SC_CloseMnt(void);
extern UINT8 GetNumberOfChan(void);

/*
----------------------------------------------------------------------------
                                SC_InitPtpConfig()
Description:
This function set initial PTP configuration parameters. The parameter will
be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
        ps_ptpGenCfg
        Pointer to PTP general configuration data structure

        ps_ptpPortCfg
        Pointer to PTP port configuration data structure

        p_ptpProfCfg
        Pointer to PTP profile configuration data structire

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed
        -2: already running
        -3: invalid value in ps_ptpGenCfg
        -4: invalid value in ps_ptpPortCfg
        -5: invalid value in ps_ptpProfCfg

-----------------------------------------------------------------------------
*/
int SC_InitPtpConfig(t_ptpGeneralConfig* ps_ptpGenCfg,
                     t_ptpPortConfig*    ps_ptpPortCfg,
                     t_ptpProfileConfig* ps_ptpProfCfg)
{
	t_ptpPortConfig* ps_portCfg;
	int i;

	if (o_initDone)
		return -2;

	/* Check if the configuration value are valid */
	if (ps_ptpGenCfg->o_slaveOnly == FALSE)
		return -3;
	for (i=0,ps_portCfg=ps_ptpPortCfg; i<k_NUM_IF; i++,ps_portCfg++)
	{
		if (ps_portCfg->e_delMech != e_DEL_MECH_E2E)
			return -4;
		if (ps_portCfg->c_delReqIntv < s_ptpProfRng.ac_delMIntRng[0] ||
		    ps_portCfg->c_delReqIntv > s_ptpProfRng.ac_delMIntRng[2])
			return -4;
		if (ps_portCfg->c_anncIntv < s_ptpProfRng.ac_annIntRng[0] ||
		    ps_portCfg->c_anncIntv > s_ptpProfRng.ac_annIntRng[2])
			return -4;
		if (ps_portCfg->b_anncRcptTmo < s_ptpProfRng.ab_annRcpTmo[0] ||
		    ps_portCfg->b_anncRcptTmo > s_ptpProfRng.ab_annRcpTmo[2])
			return -4;
		if (ps_portCfg->c_syncIntv < s_ptpProfRng.ac_synIntRng[0] ||
		    ps_portCfg->c_syncIntv > s_ptpProfRng.ac_synIntRng[2])
			return -4;
		if (ps_portCfg->c_pdelReqIntv < s_ptpProfRng.ac_delMIntRng[0] ||
		    ps_portCfg->c_pdelReqIntv > s_ptpProfRng.ac_delMIntRng[2])
			return -4;
	}
	if (ps_ptpProfCfg->e_ptpMode != e_ADDR_MODE_UNICAST &&
	    ps_ptpProfCfg->e_ptpMode != e_ADDR_MODE_MULTICAST)
		return -5;

	s_ptpGenCfg = *ps_ptpGenCfg;
	s_ptpProfCfg = *ps_ptpProfCfg;
	for (i=0,ps_portCfg=ps_ptpPortCfg; i<k_NUM_IF; i++,ps_portCfg++)
	{
		s_ptpPortCfg[i] = *ps_portCfg;
	}

	/* make sure slave only is configured consistantly */
	if (s_ptpGenCfg.o_slaveOnly)
		s_ptpGenCfg.e_clkClass = e_CLASS_SLV_ONLY;

	return 0;
}

#ifdef GPS_BUILD
/*
----------------------------------------------------------------------------
                                SC_InitGpsConfig()
Description:
This function set initial GPS configuration parameters. The parameters will
be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
        ps_gpsGenCfg
        Pointer to GPS general configuration data structure

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed
        -2: already running
        -3: invalid value in ps_gpsGenCfg

-----------------------------------------------------------------------------
*/
int SC_InitGpsConfig(SC_t_GPS_GeneralConfig* ps_gpsGenCfg)
{
	if (o_initDone)
		return -2;

   /* Check whether GPS enabled */
   if (ps_gpsGenCfg->o_gpsEnabled)
   {
      s_gpsGenConfig.o_gpsEnabled = TRUE;
   }
   else
   {
      s_gpsGenConfig.o_gpsEnabled = FALSE;
   }
	/* Check receiver type */
   switch (ps_gpsGenCfg->e_rcvrType)
   {
      /* valid receiver types */
      case e_RCVR_FURUNO_GT8031:
      case e_RCVR_ST_ERIC_GNS7560_BGA:
         s_gpsGenConfig.e_rcvrType = ps_gpsGenCfg->e_rcvrType;
         break;
      /* invalid case falls through */
      default:
         return -3;
   }
   /* Check coldstart value */
   if (ps_gpsGenCfg->ColdStrtTmOut == k_COLDSTRT_INVALID_VAL)
   {
      return -3;
   }
   else
   {
      s_gpsGenConfig.ColdStrtTmOut = ps_gpsGenCfg->ColdStrtTmOut;
   }
   /* Check whether user entered position is enabled */
   if (ps_gpsGenCfg->UserPosEnabled)
   {
      s_gpsGenConfig.UserPosEnabled = TRUE;
   }
   else
   {
      s_gpsGenConfig.UserPosEnabled = FALSE;
   }
   /* copy Lat, Long, Alt */
   s_gpsGenConfig.Latitude = ps_gpsGenCfg->Latitude;
   s_gpsGenConfig.Longitude = ps_gpsGenCfg->Longitude;
   s_gpsGenConfig.Altitude = ps_gpsGenCfg->Altitude;

	return 0;
}
#endif /* GPS_BUILD */

/*
----------------------------------------------------------------------------
                                SC_InitUnicastConfig()

Description:
This function set initial PTP unicast master table configuration parameters.
The parameter will be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
        ps_lclUcMstTbl
        Pointer to PTP local unicast master table data structure

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed
        -2: already running
        -3: invalid value in ps_unMstTbl

-----------------------------------------------------------------------------
*/
int SC_InitUnicastConfig(t_ucMasterType* ps_ucMstTbl)
{
	int i;

	if (o_initDone)
		return -2;

	/* Check if the configuration value are valid */
	if (ps_ucMstTbl->w_leaseDuration < aw_leastDurRng[0] ||
	    ps_ucMstTbl->w_leaseDuration > aw_leastDurRng[2])
		return -3;
	if (ps_ucMstTbl->c_logQryIntv < aw_ucQueryIntvRng[0] ||
	    ps_ucMstTbl->c_logQryIntv > aw_ucQueryIntvRng[2])
		return -3;
	if (ps_ucMstTbl->w_ucMasterTableSize > k_MAX_UC_MASTER_TABLE_SIZE)
		return -3;
	for (i=0; i<ps_ucMstTbl->w_ucMasterTableSize; i++)
	{
		if (ps_ucMstTbl->as_addr[i].e_netwProt != e_SC_UDP_IPv4)
			return -3;
		if (ps_ucMstTbl->as_addr[i].w_addrLen != 4)
			return -3;
	}

	s_lclUcMstTbl = *ps_ucMstTbl;
	w_leaseDuration = ps_ucMstTbl->w_leaseDuration;
	return 0;
}

/*
----------------------------------------------------------------------------
                                SC_InitAmtConfig()

Description:
This function set initial PTP acceptable master table configuration parameters.
The parameter will be used when SC_InitConfigComplete() is called.

Not supported for 2.0 release.

Parameters:

Inputs
        ps_accMstTbl
        Pointer to PTP acceptable master table data structure

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed
        -2: already running
        -3: invalid value in ps_accMstTbl
        -100: not implemented

-----------------------------------------------------------------------------
*/
int SC_InitAmtConfig(t_accMasterType* ps_accMstTbl)
{
	if (o_initDone)
		return -2;

	return -100;
}

/*
----------------------------------------------------------------------------
                                SC_InitServoConfig()

Description:
This function set initial clock servo configuration parameters. The
parameter will be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
        ps_servoCfg
        Pointer to clock configuration data structure

Outputs:
        None

Return value:
         0: function succeeded
        -1: function failed
        -2: already running
        -3: invalid value in ps_servoCfg

-----------------------------------------------------------------------------
*/
int SC_InitServoConfig(t_servoConfigType* ps_servoCfg)
{
	if (o_initDone)
		return -2;

	return SC_ServoConfig(ps_servoCfg, e_PARAM_SET);
}

/*
----------------------------------------------------------------------------
                                SC_InitConfigComplete()

Description:
This function is called after all the initial configuration is complete.
When this function is called, SoftClient will start initialization with
those configuration parameters.

Parameters:

Inputs
        w_multiplier
        Clock rate multiplier. This parameter is the multiplier for
        calculating timer interval:
                timer interval = clock resolution * multiplier
        where clock resolution is retrieved uing system call:
                clock_getres(CLOCK_REALTIME, &res)

        The time interval is the rate that SC_Run() will be called.

Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed
	-2: already running
   -3: channels not configured yet

-----------------------------------------------------------------------------
*/
int SC_InitConfigComplete(UINT16 w_multiplier)
{
   SC_t_ChanConfig as_locChanConfig[SC_TOTAL_CHANNELS];

   /* if already running */      
   if (o_initDone)
      return -2;
   /* if channels not configured yet */
   if (!Is_ChanConfigured())
      return -3;

   /* get channel config structures */
   if (SC_GetChanConfig(&as_locChanConfig[0], NULL, NULL, NULL) < 0)
      return -1;

   /* re-init the servo if needed */ 
   if (SC_RestartServo() < 0)
      return -1;
   if (THD_InitTimeRes(w_multiplier) < 0)
      return -1;
   PTP_Init(SC_ErrCallback, NULL);
   
#ifdef GPS_BUILD
   if (s_gpsGenConfig.o_gpsEnabled)
   {
      if (GPS_Init(&s_gpsGenConfig) == FALSE)
         return -1;
   }
#endif
#if (SC_SYNCE_CHANNELS > 0)
   if (SE_Init(&as_locChanConfig[0], GetNumberOfChan()) == FALSE)
      return -1;
#endif
#if (SC_SPAN_CHANNELS > 0)
   if (E1_Init(&as_locChanConfig[0], GetNumberOfChan()) == FALSE)
      return -1;
   if (T1_Init(&as_locChanConfig[0], GetNumberOfChan()) == FALSE)
      return -1;
#endif
#if (SC_RED_CHANNELS > 0)
   if (Red_Init(&as_locChanConfig[0], GetNumberOfChan()) == FALSE)
      return -1;
#endif
	if (SC_InitMnt() < 0)
		return -1;
	/* thread start must run last */
	if (THD_Start() < 0)
		return -1;
	o_initDone = TRUE;
	return 0;
}

/*
----------------------------------------------------------------------------
                                SC_Run()

Description:
This function is called from the Timer ISR.  This function is used to
run the PTP stack, and must be call at a regular predetermined rate
(i.e every 4 milliseconds.)

Parameters:

	Inputs:
	None
	
	Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed
	-2: not initialized

-----------------------------------------------------------------------------
*/
int SC_Run( void )
{
	if (!o_initDone)
		return -2;

	if (THD_Run() < 0)
		return -1;
	return 0;
}

/*
----------------------------------------------------------------------------
                                SC_Shutdown()

Description:
This function will free all the memory that was allocated using the 
SC_Init() function.

Parameters:

	Inputs:
	None
	
	Outputs:
	None

Return value:
	 0: function succeeded
	-1: function failed

-----------------------------------------------------------------------------
*/
int SC_Shutdown(void)
{
	if (o_initDone)
	{
      SC_CloseMnt();
      THD_Shutdown();
      SC_Close();
      o_initDone = FALSE;
      Set_ChanConfigured(FALSE);
	}
	/* delay 1 second to allow all thread shutdown */
	sleep(1);
	return 0;
}

/*
----------------------------------------------------------------------------
                                SC_Close()

Description:
This will call on specific channels to close their interfaces and call
user API functions.

Parameters:

	Inputs:
	None
	
	Outputs:
	None

Return value:
   None

-----------------------------------------------------------------------------
*/
static void SC_Close(void)
{
   SC_t_ChanConfig as_locChanConfig[SC_TOTAL_CHANNELS];

   /* get channel config structures */
   SC_GetChanConfig(&as_locChanConfig[0], NULL, NULL, NULL);

	if (o_initDone)
	{
#ifdef GPS_BUILD
      /* Close GPS */
		GPS_Close();
#endif
#if (SC_SYNCE_CHANNELS > 0)
      /* Close Sync-E */
		SE_Close(&as_locChanConfig[0], GetNumberOfChan());
#endif
#if (SC_SPAN_CHANNELS > 0)
      /* Close E1 */
		E1_Close(&as_locChanConfig[0], GetNumberOfChan());
      /* Close T1 */
		T1_Close(&as_locChanConfig[0], GetNumberOfChan());
#endif
#if (SC_RED_CHANNELS > 0)
      /* Close Redunancy Channel */
		Red_Close(&as_locChanConfig[0], GetNumberOfChan());
#endif
   }
	return;
}

/*
----------------------------------------------------------------------------
                                SC_GetUcMstTbl()

Description:
This function replaces FIO_ReadUcMstTbl function.

Parameters:

Inputs
        None

Outputs:
        ps_ucMstTbl
        Pointer to structure of the master table to be written to.

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
BOOLEAN SC_GetUcMstTbl(PTP_t_PortAddrQryTbl* ps_ucMstTbl)
{
	UINT16 dw_i, dw_ucMst;

	ps_ucMstTbl->c_logQryIntv = s_lclUcMstTbl.c_logQryIntv;
	for (dw_i=0,dw_ucMst=0; dw_i<s_lclUcMstTbl.w_ucMasterTableSize; dw_i++)
	{
		/* if a correct network protocol is used, try to copy */
		if (ps_ucMstTbl->aps_pAddr[dw_ucMst] != NULL)
		{
			/* copy this entry */
			ps_ucMstTbl->aps_pAddr[dw_ucMst]->e_netwProt =
			             s_lclUcMstTbl.as_addr[dw_i].e_netwProt;
			ps_ucMstTbl->aps_pAddr[dw_ucMst]->w_AddrLen =
			             s_lclUcMstTbl.as_addr[dw_i].w_addrLen;
			memcpy(ps_ucMstTbl->aps_pAddr[dw_ucMst]->ab_Addr,
			       s_lclUcMstTbl.as_addr[dw_i].ab_addr,
			       ps_ucMstTbl->aps_pAddr[dw_ucMst]->w_AddrLen);
			/* next master */
			dw_ucMst++;
		}
	}
	ps_ucMstTbl->w_actTblSze  = dw_ucMst;
	return TRUE;
}

/*
----------------------------------------------------------------------------
                                SC_GetConfig()

Description:
This function replaces FIO_ReadConfig function.

Parameters:

Inputs
        None

Outputs:
        ps_cfgFile
        Pointer to structure the configuration structure to be written to.

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
BOOLEAN SC_GetConfig(PTP_t_cfgRom* ps_cfgFile)
{
	strcpy(ps_cfgFile->ac_userDesc, (char*)s_ptpGenCfg.c_userDescription);
	ps_cfgFile->s_defDs.e_clkClass = s_ptpGenCfg.e_clkClass;
	ps_cfgFile->s_defDs.w_scldVar = s_ptpGenCfg.w_scldVar;
	ps_cfgFile->s_defDs.b_prio1 = s_ptpGenCfg.b_prio1;
	ps_cfgFile->s_defDs.b_prio2 = s_ptpGenCfg.b_prio2;
	ps_cfgFile->s_defDs.b_domNmb = s_ptpGenCfg.b_domNmb;
	ps_cfgFile->s_defDs.o_slaveOnly = s_ptpGenCfg.o_slaveOnly;
	ps_cfgFile->s_tcDefDs.b_TcprimDom = 0;
	ps_cfgFile->s_tcDefDs.e_TcDelMech = e_DELMECH_E2E;
	ps_cfgFile->as_pDs[0].e_delMech = s_ptpPortCfg[0].e_delMech;
	ps_cfgFile->as_pDs[0].c_dlrqIntv = s_ptpPortCfg[0].c_delReqIntv;
	ps_cfgFile->as_pDs[0].c_AnncIntv = s_ptpPortCfg[0].c_anncIntv;
	ps_cfgFile->as_pDs[0].b_anncRcptTmo = s_ptpPortCfg[0].b_anncRcptTmo;
	ps_cfgFile->as_pDs[0].c_syncIntv = s_ptpPortCfg[0].c_syncIntv;
	ps_cfgFile->as_pDs[0].c_pdelReqIntv = s_ptpPortCfg[0].c_pdelReqIntv;
	ps_cfgFile->s_prof = s_ptpProfRng;
	return TRUE;
}

/*
----------------------------------------------------------------------------
                                SC_GetClkRcvr()

Description:
This function replaces FIO_ReadClkRcvr function.

Parameters:

Inputs
        None

Outputs:
        ps_clkRcvr
        Pointer to structure the clock recover structure to be written to.

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
BOOLEAN SC_GetClkRcvr(PTP_t_cfgClkRcvr* ps_clkRcvr)
{
	ps_clkRcvr->ll_drift_ppb = 254174733528839072LL;
	ps_clkRcvr->o_drift_use = 0;
	ps_clkRcvr->ll_E2EmeanPDel = 268704812LL;
	ps_clkRcvr->all_P2PmeanPDel[0] = 4294967295LL;
	return TRUE;
}

/*
----------------------------------------------------------------------------
                                SC_GetClkId()

Description:
This function reads clock Id from configuration data.

Parameters:

Inputs
        None

Outputs:
        ps_cfgFile
        Pointer to structure the configuration structure to be written to.

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
BOOLEAN SC_GetClkId(t_clockIdType* ps_clkId)
{
	*ps_clkId = s_ptpGenCfg.s_ptpClkId;
	return TRUE;
}

/*
----------------------------------------------------------------------------
                                SC_ErrCallback()

Description:
This is error callback function to registrate within the funtion PTP_Init.
Every asynchronous error raise a call of this function. The format of the
function follows Ixxat stack.  The error is sent to debug log.

Parameters:

Inputs
	dw_unitId
	Module where error occured

	dw_errNmb
	Error number 

	e_sevCode
	Severity code of error

	ps_ts
	Time of error

	o_ptpRunning
	Engine is running flag

Outputs:
        None

Return value:
        None

-----------------------------------------------------------------------------
*/
static void SC_ErrCallback(UINT32          dw_unitId,
                           UINT32          dw_errNmb,
                           PTP_t_sevCdEnum e_sevCode,
                           PTP_t_TmStmp    *ps_ts,
                           BOOLEAN         o_ptpRunning)
{
	debug_printf(PTP_STACK_PRT, "%s", PTP_GetErrStr(dw_unitId, dw_errNmb));
}

BOOLEAN Is_InitComplete(void)
{
  return o_initDone;
}
