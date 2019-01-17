/*****************************************************************************
*                                                                            *
*   Copyright (C) 2010 Symmetricom, Inc., all rights reserved                *
*                                                                            *
*   This software contains proprietary and confidential information of       *
*   Symmetricom.                                                             *
*   It may not be reproduced, used, or disclosed to others for any purpose   *
*   without the written authorization of Symmetricom.                        *
*                                                                            *
******************************************************************************

FILE NAME    : sc_ptp_mgmt.c

AUTHOR       : Jining Yang

DESCRIPTION  :

This file implements the Symmetricom private management messages.

Revision control header:
$Id: API/sc_ptp_mgmt.c 1.4 2010/03/09 15:32:28PST kho Exp  $

******************************************************************************
*/

/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>
#include "target.h"
#include "sc_api.h"
#include "PTP/PTPdef.h"
#include "DBG/DBG.h"
#include "GOE/GOE.h"
#include "sc_system.h"
#include "sc_ptp_mgmt.h"

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/

static struct ptp_mgmt_status_s ptp_mgmt_status;
static PTP_Fll_Clock_State fll_state_to_tp500_state[] = {
           PTP_FLL_ACQUIRING,  // e_FLL_STATE_ACQUIRING = (0)
           PTP_FLL_ACQUIRING,  // e_FLL_STATE_WARMUP    = (1)
           PTP_FLL_FAST,       // e_FLL_STATE_FAST      = (2)
           PTP_FLL_NORMAL,     // e_FLL_STATE_NORMAL    = (3)
           PTP_FLL_BRIDGE,     // e_FLL_STATE_BRIDGE    = (4)
           PTP_FLL_HOLD        // e_FLL_STATE_HOLDOVER  = (5)
};

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/

/*
 * make a 32 bits TLV
 */
void mgmt_tlv_packet_32(UINT32* src, unsigned char *des_ptr, UINT8 type)
{
    des_ptr[0] = type;
    des_ptr[1] = 4;
    GOE_hton32(des_ptr+2, *src);
}

/*
 * make a 16 bits TLV
 */
void mgmt_tlv_packet_16(UINT16 *src, unsigned char *des_ptr, UINT8 type)
{
    des_ptr[0] = type;
    des_ptr[1] = 2;
    GOE_hton16(des_ptr+2, *src);
}

/*
 * make a 8 bit TLV
 */
void mgmt_tlv_packet_8(UINT8 *src, unsigned char *des_ptr, UINT8 type)
{
    des_ptr[0] = type;
    des_ptr[1] = 1;
    des_ptr[2] = *src;
}

/*
 * make a string TLV
 */
void mgmt_tlv_packet_string(unsigned char *src_str, unsigned char *des_ptr, UINT8 type, int length)
{
    int i =0;

    des_ptr[0] = type;
    des_ptr[1] = (UINT8)length;

    for (i=0; i< length; i++)
    {
        des_ptr[i+2] = src_str[i];
    }
    if (type == PS_ASCII_STRING)
    {
        des_ptr[2+length] = 0;		
    }
}

/*
 * convert each status item to approperiate format for generate private
 * status message
 */
int update_ptp_mgmt_status(void)
{
    t_ptpTimeStampType s_sysTime;
    INT16 i_utcOffset;
    t_ptpPerformanceStatusType s_ptpPerformance;
    t_ptpIpdvType s_ptpIpdv;

    ptp_mgmt_status.tp500personality = (UINT32)0x00B0AE00;

    // Get time
    if (SC_SystemTime(&s_sysTime, &i_utcOffset, e_PARAM_GET) < 0)
    {
        debug_printf(UNMASK_PRT, "Error while running SC_SystemTime()");
        return -1;
    }

    SC_TimeToDate(s_sysTime.u48_sec-i_utcOffset, ptp_mgmt_status.current_time);

    // Get status
    SC_GetPtpPerformance(&s_ptpPerformance);
    SC_ClkIpdvGet(&s_ptpIpdv);

    ptp_mgmt_status.fllState =
             (UINT32)fll_state_to_tp500_state[s_ptpPerformance.e_fllState];
    ptp_mgmt_status.fllStateDuration =
             (UINT32)(s_ptpPerformance.dw_fllStateDur / 60);
    ptp_mgmt_status.forwardWeight =
             (UINT32)(s_ptpPerformance.f_fwdWeight * 100.0);
    ptp_mgmt_status.forwardTrans900 =
             (UINT32)s_ptpPerformance.dw_fwdTransFree900;
    ptp_mgmt_status.forwardTrans3600 =
             (UINT32)s_ptpPerformance.dw_fwdTransFree3600;
    ptp_mgmt_status.forwardTransUsed =
             (UINT32)(s_ptpPerformance.f_fwdPct * 100.0);
    ptp_mgmt_status.forwardOperationalMinTdev =
             (UINT32)(s_ptpPerformance.f_fwdMinTdev * 100.0);
    ptp_mgmt_status.forwardOpMafe =
             (UINT32)(s_ptpPerformance.f_fwdMafie * 100.0);
    ptp_mgmt_status.forwardMinClusterWidth =
             (UINT32)(s_ptpPerformance.f_fwdMinClstrWidth * 100.0);
    ptp_mgmt_status.forwardModeWidth =
             (UINT32)(s_ptpPerformance.f_fwdModeWidth * 100.0);
    ptp_mgmt_status.reverseWeight =
             (UINT32)(s_ptpPerformance.f_revWeight * 100.0);
    ptp_mgmt_status.reverseTrans900 =
             (UINT32)s_ptpPerformance.dw_revTransFree900;
    ptp_mgmt_status.reverseTrans3600 =
             (UINT32)s_ptpPerformance.dw_revTransFree3600;
    ptp_mgmt_status.reverseTransUsed =
             (UINT32)(s_ptpPerformance.f_revPct * 100.0);
    ptp_mgmt_status.reverseOperationalMinTdev =
             (UINT32)(s_ptpPerformance.f_revMinTdev * 100.0);
    ptp_mgmt_status.reverseOpMafe =
             (UINT32)(s_ptpPerformance.f_revMafie * 100.0);
    ptp_mgmt_status.reverseMinClusterWidth =
             (UINT32)(s_ptpPerformance.f_revMinClstrWidth * 100.0);
    ptp_mgmt_status.reverseModeWidth =
             (UINT32)(s_ptpPerformance.f_revMinClstrWidth * 100.0);
    ptp_mgmt_status.freqCorrect =
             (INT32)(s_ptpPerformance.f_freqCorrection * 100.0);
    ptp_mgmt_status.phaseCorrect =
             (INT32)(s_ptpPerformance.f_phaseCorrection * 100.0);
    ptp_mgmt_status.outputTdevEstimate =
             (UINT32)(s_ptpPerformance.f_tdevEstimate * 100.0);
    ptp_mgmt_status.outputMdevEstimate =
             (UINT32)(s_ptpPerformance.f_mdevEstimate * 100.0);
    ptp_mgmt_status.residualPhaseError =
             (INT32)(s_ptpPerformance.f_residualPhaseErr * 100.0);
    ptp_mgmt_status.min_time_dispersion =
             (UINT32)(s_ptpPerformance.f_minRoundTripDly * 100.0);
    ptp_mgmt_status.forwardMasterSyncPerSec =
             (UINT32)s_ptpPerformance.w_fwdPktRate;
    ptp_mgmt_status.reverseMasterDelayRespPerSec =
             (UINT32)s_ptpPerformance.w_revPktRate;
    ptp_mgmt_status.forward_ipdv_pct_below =
             (UINT32)(s_ptpIpdv.f_ipdvFwdPct * 10.0);
    ptp_mgmt_status.forward_ipdv_max =
             (UINT32)(s_ptpIpdv.f_ipdvFwdMax * 1000.0);
    ptp_mgmt_status.forward_interpkt_var =
             (UINT32)(s_ptpIpdv.f_ipdvFwdJitter * 1000.0);
    ptp_mgmt_status.reversed_ipdv_pct_below =
             (UINT32)(s_ptpIpdv.f_ipdvRevPct * 10.0);
    ptp_mgmt_status.reversed_ipdv_max =
             (UINT32)(s_ptpIpdv.f_ipdvRevMax * 1000.0);
    ptp_mgmt_status.reversed_interpkt_var =
             (UINT32)(s_ptpIpdv.f_ipdvRevJitter * 1000.0);
    return 0;
}

/*
 * genertae private status get response ptp management message
 */
int SC_GetPrivateStatus(UINT8* ab_buff, UINT16* pw_dynSize)
{
    UINT16 pkt_size = 0;
    UINT8* mgmt_buff = ab_buff;
    UINT8 local_uint8;
    UINT16 local_uint16;

    if (update_ptp_mgmt_status() < 0)
    {
        return -1;
    }

    // copy the identifier to the buffer
    *(UINT32*)(mgmt_buff+pkt_size) = ptp_mgmt_status.tp500personality;
    pkt_size += 4;

    // copy the personality to buffer
    mgmt_buff[pkt_size] = PS_TP500PERSONALITY;
    mgmt_tlv_packet_string((UINT8*)&ptp_mgmt_status.tp500personality, 
                           (mgmt_buff+pkt_size+1), PS_BINARY_STRING, 4);
    pkt_size += 7;

    // copy the date and time to buffer
    mgmt_buff[pkt_size] = PS_CURRENT_TIME;
    mgmt_tlv_packet_string((UINT8*)ptp_mgmt_status.current_time,
                           (mgmt_buff+pkt_size+1), PS_ASCII_STRING, 20);
    pkt_size += 23;

    // copy the fll state to buffer
    mgmt_buff[pkt_size] = PS_FLLSTATE;
    local_uint8 = (UINT8)ptp_mgmt_status.fllState;
    mgmt_tlv_packet_8((UINT8*)&local_uint8,
                      mgmt_buff+pkt_size+1, PS_UINT8);
    pkt_size += 4;

    // copy the fll state duration to buffer
    mgmt_buff[pkt_size] = PS_FLLSTATEDURATION;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.fllStateDuration, 
                       mgmt_buff+pkt_size+1, PS_UINT32);
    pkt_size += 7;

    // copy the forward weight to buffer
    mgmt_buff[pkt_size] = PS_FORWARDWEIGHT;
    local_uint16 = (UINT16)ptp_mgmt_status.forwardWeight;
    mgmt_tlv_packet_16((UINT16*)&local_uint16, 
                       mgmt_buff+pkt_size+1, PS_UINT16_BASE_1E_2);
    pkt_size += 5;

    // copy the forward trans900 to buffer
    mgmt_buff[pkt_size] = PS_FORWARDTRANS900;
    local_uint16 = (UINT16)ptp_mgmt_status.forwardTrans900;
    mgmt_tlv_packet_16((UINT16*)&local_uint16, 
                       mgmt_buff+pkt_size+1, PS_UINT16);
    pkt_size += 5;

	// copy the forward trans3600 to buffer
    mgmt_buff[pkt_size] = PS_FORWARDTRANS3600;
    local_uint16 = (UINT16)ptp_mgmt_status.forwardTrans3600;
    mgmt_tlv_packet_16((UINT16*)&local_uint16, 
                       mgmt_buff+pkt_size+1, PS_UINT16);
    pkt_size += 5;

    // copy the forward transused to buffer
    mgmt_buff[pkt_size] = PS_FORWARDTRANSUSED;
    local_uint16 = (UINT16)ptp_mgmt_status.forwardTransUsed;
    mgmt_tlv_packet_16((UINT16*)&local_uint16, 
                       mgmt_buff+pkt_size+1, PS_UINT16_BASE_1E_2);
    pkt_size += 5;

    // copy the forward operation min TDEV to buffer
    mgmt_buff[pkt_size] = PS_FORWARDOPMINTDEV;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.forwardOperationalMinTdev,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_2);
    pkt_size += 7;

    // copy the forward operation MAFE to buffer
    mgmt_buff[pkt_size] = PS_FORWARDOPMAFE;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.forwardOpMafe,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_2);
    pkt_size += 7;

    // copy the forward min cluster width to buffer
    mgmt_buff[pkt_size] = PS_FORWARDMINCLUWIDTH;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.forwardMinClusterWidth,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_2);
    pkt_size += 7;

    // copy the forward mode width to buffer
    mgmt_buff[pkt_size] = PS_FORWARDMODEWIDTH;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.forwardModeWidth,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_2);
    pkt_size += 7;

    // copy the reversed weight to buffer
    mgmt_buff[pkt_size] = PS_REVERSEDWEIGHT;
    local_uint16 = (UINT16)ptp_mgmt_status.reverseWeight;
    mgmt_tlv_packet_16((UINT16*)&local_uint16, 
                       mgmt_buff+pkt_size+1, PS_UINT16_BASE_1E_2);
    pkt_size += 5;

    // copy the reversed trans900 to buffer
    mgmt_buff[pkt_size] = PS_REVERSEDTRANS900;
    local_uint16 = (UINT16)ptp_mgmt_status.reverseTrans900;
    mgmt_tlv_packet_16((UINT16*)&local_uint16, 
                       mgmt_buff+pkt_size+1, PS_UINT16);
    pkt_size += 5;

    // copy the reversed trans3600 to buffer
    mgmt_buff[pkt_size] = PS_REVERSEDTRANS3600;
    local_uint16 = (UINT16)ptp_mgmt_status.reverseTrans3600;
    mgmt_tlv_packet_16((UINT16*)&local_uint16, 
                       mgmt_buff+pkt_size+1, PS_UINT16);
    pkt_size += 5;

    // copy the reversed transused to buffer
    mgmt_buff[pkt_size] = PS_REVERSEDTRANSUSED;
    local_uint16 = (UINT16)ptp_mgmt_status.reverseTransUsed;
    mgmt_tlv_packet_16((UINT16*)&local_uint16, 
                       mgmt_buff+pkt_size+1, PS_UINT16_BASE_1E_2);
    pkt_size += 5;

    // copy the reversed operation min TDEV to buffer
    mgmt_buff[pkt_size] = PS_REVERSEDOPMINTDEV;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.reverseOperationalMinTdev,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_2);
    pkt_size += 7;

    // copy the reversed operation MAFE to buffer
    mgmt_buff[pkt_size] = PS_REVERSEDOPMAFE;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.reverseOpMafe,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_2);
    pkt_size += 7;

    // copy the reversed min cluster width to buffer
    mgmt_buff[pkt_size] = PS_REVERSEDMINCLUWIDTH;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.reverseMinClusterWidth,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_2);
    pkt_size += 7;

    // copy the reversed mode width to buffer
    mgmt_buff[pkt_size] = PS_REVERSEDMODEWIDTH;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.reverseModeWidth,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_2);
    pkt_size += 7;

    // copy the frequency correct to buffer
    mgmt_buff[pkt_size] = PS_FREQCORRECT;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.freqCorrect,
                       mgmt_buff+pkt_size+1, PS_INT32_BASE_1E_2);
    pkt_size += 7;

    // copy the phase correct to buffer
    mgmt_buff[pkt_size] = PS_PHASECORRECT;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.phaseCorrect,
                       mgmt_buff+pkt_size+1, PS_INT32_BASE_1E_2);
    pkt_size += 7;

    // copy the output TDEV estimate to buffer
    mgmt_buff[pkt_size] = PS_OUTPUTTDEVESTIMATE;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.outputTdevEstimate,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_2);
    pkt_size += 7;

    // copy the output MDEV estimate to buffer
    mgmt_buff[pkt_size] = PS_OUTPUTMDEVESTIMATE;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.outputMdevEstimate,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_2);
    pkt_size += 7;

    // copy the residual phase error to buffer
    mgmt_buff[pkt_size] = PS_RESIDUALPHASEERROR;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.residualPhaseError,
                       mgmt_buff+pkt_size+1, PS_INT32_BASE_1E_2);
    pkt_size += 7;

    // copy the minimal RTD to buffer
    mgmt_buff[pkt_size] = PS_MINTIMEDISPERSION;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.min_time_dispersion,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_2);
    pkt_size += 7;

    // copy the forward GM sync packet/sec to buffer
    mgmt_buff[pkt_size] = PS_FORWARDGMSYNCPSEC;
    local_uint8 = (UINT8)ptp_mgmt_status.forwardMasterSyncPerSec;
    mgmt_tlv_packet_8((UINT8*)&local_uint8, 
                      mgmt_buff+pkt_size+1, PS_UINT8);
    pkt_size += 4;

    // copy the reversed GM sync packet/sec to buffer
    mgmt_buff[pkt_size] = PS_REVERSEDGMSYNCPSEC;
    local_uint8 = (UINT8)ptp_mgmt_status.reverseMasterDelayRespPerSec;
    mgmt_tlv_packet_8((UINT8*)&local_uint8, 
                      mgmt_buff+pkt_size+1, PS_UINT8);
    pkt_size += 4;

    // copy the forward ipdv pct below to buffer
    mgmt_buff[pkt_size] = PS_FWD_IPDV_PCT_BELOW;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.forward_ipdv_pct_below,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_1);
    pkt_size += 7;

    // copy the ipdv pct below to buffer
    mgmt_buff[pkt_size] = PS_FWD_IPDV_MAX;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.forward_ipdv_max,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_3);
    pkt_size += 7;

    // copy the ipdv forward inter-packet to buffer
    mgmt_buff[pkt_size] = PS_FWD_INTERPKT_VAR;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.forward_interpkt_var,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_3);
    pkt_size += 7;

    // copy the forward ipdv pct below to buffer
    mgmt_buff[pkt_size] = PS_REV_IPDV_PCT_BELOW;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.reversed_ipdv_pct_below,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_1);
    pkt_size += 7;

    // copy the ipdv pct below to buffer
    mgmt_buff[pkt_size] = PS_REV_IPDV_MAX;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.reversed_ipdv_max,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_3);
    pkt_size += 7;

    // copy the ipdv forward inter-packet to buffer
    mgmt_buff[pkt_size] = PS_REV_INTERPKT_VAR;
    mgmt_tlv_packet_32((UINT32*)&ptp_mgmt_status.reversed_interpkt_var,
                       mgmt_buff+pkt_size+1, PS_UINT32_BASE_1E_3);
    pkt_size += 7;

    *pw_dynSize = pkt_size;
    return 0;
}

