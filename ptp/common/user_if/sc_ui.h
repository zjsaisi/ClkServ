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

FILE NAME    : sc_ui.h

AUTHOR       : Ken Ho

DESCRIPTION  :

This file defines the data types used in user interface.

Revision control header:
$Id: user_if/sc_ui.h 1.6 2011/03/24 18:51:03PDT German Alvarez (galvarez) Exp  $

******************************************************************************
*/

/* Prevent file from being included multiple times. */
#ifndef H_SC_UI_h
#define H_SC_UI_h

#ifdef __cplusplus
extern "C"
{
#endif


/*--------------------------------------------------------------------------*/
//              include
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              defines
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              types
/*--------------------------------------------------------------------------*/
typedef struct
{
        UINT8   b_state;
        char    *pc_textDesc;
} t_stateDescriptions;

typedef enum
{
        //The enumeration of PTP private status
        e_PS_TP500PERSONALITY           = 0,  //not provided to end customer, used to identify as symm msg
        e_PS_FLLSTATE                   = 1,  //row 1
        e_PS_FLLSTATEDURATION           = 2,
        e_PS_SELECTEDMASTERIP           = 3,  //row 30, this needs state defn for none, GM1, GM2
        e_PS_FORWARDWEIGHT              = 4,  //row 3
        e_PS_FORWARDTRANS900            = 5,
        e_PS_FORWARDTRANS3600           = 6,
        e_PS_FORWARDTRANSUSED           = 7,
        e_PS_FORWARDOPMINTDEV           = 8,
        e_PS_FORWARDMINCLUWIDTH         = 9,
        e_PS_FORWARDMODEWIDTH           = 10,  //row  9
        e_PS_FORWARDGMSYNCPSEC          = 11,  //row 27
        e_PS_FORWARDACCGMSYNCPSEC       = 12,  //row 28
        e_PS_REVERSEDWEIGHT             = 13,  //row 10
        e_PS_REVERSEDTRANS900           = 14,
        e_PS_REVERSEDTRANS3600          = 15,
        e_PS_REVERSEDTRANSUSED          = 16,
        e_PS_REVERSEDOPMINTDEV          = 17,
        e_PS_REVERSEDMINCLUWIDTH        = 18,
        e_PS_REVERSEDMODEWIDTH          = 19,  //row 16
        e_PS_REVERSEDGMSYNCPSEC         = 20,  //row 29 this is the reverse delay rate for the reference GM
        e_PS_REVERSEDACCGMSYNCPSEC      = 21,  //not used
        e_PS_OUTPUTTDEVESTIMATE         = 22,  //row 19
        e_PS_FREQCORRECT                = 23,  //row 17
        e_PS_PHASECORRECT               = 24,  //row 18
        e_PS_RESIDUALPHASEERROR         = 25,  //row 20
        e_PS_MINTIMEDISPERSION          = 26,  //row 21
        e_PS_MASTERFLOWSTATE            = 27,  //row 31
        e_PS_ACCMASTERFLOWSTATE         = 28,
        e_PS_MASTERCLOCKID              = 29,
        e_PS_ACCMASTERCLOCKID           = 30,
        e_PS_LASTUPGRADESTATUS          = 31,  //row 35
        e_PS_OP_TEMP_MAX                = 32,  //row 22   all of the temp vars are PS_UINT16_BASE_1E_2
        e_PS_OP_TEMP_MIN                = 33,
        e_PS_OP_TEMP_AVG                = 34,
        e_PS_TEMP_STABILITY_5MIN        = 35,
        e_PS_TEMP_STABILITY_60MIN       = 36,  //row 26
        e_PS_IPDV_OBS_INTERVAL          = 37,  //row 36    value in seconds PS_UINT16
        e_PS_IPDV_THRESHOLD             = 38,  //value enter in ns, present in us, PS_UINT32_BASE_1E_2 takes it to 8 s
        e_PS_VAR_PACING_FACTOR          = 39,  //power-of-2, so uint8 is fine, controls packets to skip for variation comp
        e_PS_FWD_IPDV_PCT_BELOW         = 40,  //value in 10m%, present in %, PS_UINT16_BASE_1E_2 allows 100.xx
        e_PS_FWD_IPDV_MAX               = 41,  //value in ns, present in us, PS_UINT32_BASE_1E_2 takes it to 8 s
        e_PS_FWD_INTERPKT_VAR           = 42,  //value in ns, present in us, PS_UINT32_BASE_1E_2 takes it to 8 s
        e_PS_REV_IPDV_PCT_BELOW         = 43,  //value in 10m%, present in %, PS_UINT16_BASE_1E_2 allows 100.xx
        e_PS_REV_IPDV_MAX               = 44,  //value in ns, present in us, PS_UINT32_BASE_1E_2 takes it to 8 s
        e_PS_REV_INTERPKT_VAR           = 45,  //value in ns, present in us, PS_UINT32_BASE_1E_2 takes it to 8 s
        e_PS_CURRENT_TIME               = 46,  //row 0 , can this be PS_ASCII_STRING?
        e_PS_FORWARDOPMAFE              = 47,  //forward operation MAFE
        e_PS_REVERSEDOPMAFE             = 48,  //Reversed operation MAFE
        e_PS_OUTPUTMDEVESTIMATE         = 49   //Output MDEV Estimate
} t_ptpStatusIndex;

/*--------------------------------------------------------------------------*/
//              data
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
//              function prototypes
/*--------------------------------------------------------------------------*/

int SC_StartUi(void);
int SC_StopUi(void);
char trueFalseIntToChar(int n);
int trueFalseCharToInt(char c);
void *tenHzThread(void *arg);



#ifdef __cplusplus
}
#endif

#endif /*  H_SC_API_h */
