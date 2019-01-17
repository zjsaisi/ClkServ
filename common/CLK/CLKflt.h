
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

FILE NAME    : CLKflt.h

AUTHOR       : Ken Ho

DESCRIPTION  : 

These functions are used to move timestamp data into the PTP servo task.

Revision control header:
$Id: CLK/CLKflt.h 1.7 2011/02/02 15:53:47PST Kenneth Ho (kho) Exp  $

******************************************************************************
*/


/* Prevent file from being included multiple times. */
#ifndef H_clkflt_h
#define H_clkflt_h


/*****************************************************************************/
/*                       ***INCLUDES Definitions***                          */
/*  Nested includes should not be used as a general practice.  If there is a */
/*  good reason to include an include within an include, do so here. 	     */
/*****************************************************************************/



/*****************************************************************************/
/*                       ***CONSTANT Definitions***                          */
/*  This section should be used to define constants (#defines) and macros    */
/*  that can be used by any module that includes this file.                  */
/*****************************************************************************/



/*****************************************************************************/
/*                       ***Data Type Specifications***                      */
/*  This section should be used to specify data types including structures,  */
/*  enumerations, unions, and redefining individual data types.              */
/*****************************************************************************/


/*****************************************************************************/
/*                       ***Public Data Section***                           */
/*  Global variables, if used, should be defined in a ".c" source file.      */
/*  The scope of a global may be extended to other modules by declaring it as*/
/*  as an extern here. 			                                     */
/*****************************************************************************/

/*****************************************************************************/
/*                       ***Function Prototype Section***                    */
/*  This section to be used to prototype public functions.                   */
/*                                                                           */
/*  Descriptions of parameters and return code should be given for each      */
/*  function.                                                                */
/*****************************************************************************/
/************************************************************************/
/** t_fltTypeEnum
          This enumeration defines all filter names used within
          the protocol stack.
*/
typedef enum
{
  SC_e_FILT_NO     = 0x0,  /* no filter */
  SC_e_FILT_MIN    = 0x1,  /* Minimum filter */
  SC_e_FILT_EST    = 0x2,  /* Estimation filter */
  SC_e_FILT_ISPCFC = 0x4   /* implementation specific filter */
} t_fltTypeEnum;

/************************************************************************/
/** t_filtMin
                This structure holds data that parameterizes the 
                filter F1
**/
typedef struct
{
  UINT32 dw_filtLen; /* filter window length */
} t_filtMin;

/************************************************************************/
/** t_filtEst
                This structure holds data that parameterizes the
                filter F2.
**/
typedef struct
{
  UINT8  b_histLen;    /* number of history samples for statistics */
  UINT64 ddw_WhtNoise; /* amplitude of white noise in nsec */
  UINT8  b_amntFlt;    /* number of successive corrections */
} t_filtEst;

/************************************************************************/
/** t_fltCfg
                This structure holds data that get read out of non-
                volatile storage that specify the used filter and 
                holds the parameterization of the used filter.
**/
typedef struct
{
  t_fltTypeEnum e_usedFilter; /* Filter number */
  t_filtMin     s_filtMin;    /* F1 paremeters */
  t_filtEst     s_filtEst;    /* F2 parameters */
} t_fltCfg;

/************************************************************************/
/** PTP_t_FUNC_INP_MTSD 
             This typedef defines the filter input-mtsd function type.
**/
typedef BOOLEAN (*t_FUNC_INP_MTSD)(const t_TmIntv           *ps_tiMtsd,
                                   const t_ptpTimeStampType *ps_txTsSyn,
                                   const t_ptpTimeStampType *ps_rxTsSyn,
                                   UINT16                   w_ifIdx,
                                   UINT32                   dw_TaSynIntvUsec,
                                   t_TmIntv                 *ps_tiOffset,
                                   UINT32                   *pdw_TaOffsUsec,
                                   t_TmIntv                 *ps_tiFltMtsd,
                                   UINT8                    *pb_mode);

/************************************************************************/
/** PTP_t_FUNC_INP_STMD 
             This typedef defines the filter input-stmd function type.
**/
typedef void (*t_FUNC_INP_STMD)(const t_TmIntv           *ps_tiStmd,
                                const t_ptpTimeStampType *ps_rxTsDlrq,
                                const t_ptpTimeStampType *ps_txTsDlrq,
                                UINT32                   dw_TaDlrqIntv,
                                t_TmIntv                 *ps_tiFltStmd,
                                t_TmIntv                 *ps_tiFltOwd,
                                UINT8                    *pb_mode);

/************************************************************************/
/** PTP_t_FUNC_INP_P2P 
             This typedef defines the filter input-p2p-delay function type.
**/
typedef void (*t_FUNC_INP_P2P)(const t_TmIntv           *ps_tiP2PDel,
                               const t_ptpTimeStampType *ps_txTsPDlrq,
                               const t_ptpTimeStampType *ps_rxTsPDlrsp,
                               UINT16                   w_ifIdx,
                               UINT32                   dw_taP2PIntvUsec,
                               t_TmIntv                 *ps_tiFltP2Pdel,
                               UINT8                    *pb_mode);

/************************************************************************/
/** PTP_t_FUNC_INP_NEWMST 
             This typedef defines the filter input-new-master function type.
**/
//typedef void (*t_FUNC_INP_NEWMST)(const PTP_t_PortId   *ps_pIdNewMst,
//                                  const PTP_t_PortAddr *ps_pAddrNewMst,
//                                  BOOLEAN              o_mstIsUnicast);
typedef void (*t_FUNC_INP_NEWMST)(const void           *ps_pIdNewMst,
                                  const void           *ps_pAddrNewMst,
                                  BOOLEAN              o_mstIsUnicast);

/************************************************************************/
/** PTP_t_FUNC_FLT_RESET 
             This typedef defines the filter reset function type.
**/
typedef void (*t_FUNC_FLT_RESET)(void);


extern BOOLEAN CLKflt_inpMTSD(const t_TmIntv           *ps_tiMtsd,
                              const t_ptpTimeStampType *ps_txTsSyn,
                              const t_ptpTimeStampType *ps_rxTsSyn,
                              UINT16                   w_ifIdx,
                              UINT32                   dw_TaSynIntvUsec,
                              t_TmIntv                 *ps_tiOffset,
                              UINT32                   *pdw_TaOffsUsec,
                              t_TmIntv                 *ps_tiFltMtsd,
                              UINT8                    *pb_mode);

extern void CLKflt_inpSTMD(const t_TmIntv           *ps_tiStmd,
                           const t_ptpTimeStampType *ps_rxTsDlrsp,
                           const t_ptpTimeStampType *ps_txTsDlrq,
                           UINT32                   dw_TaDlrqIntv,
                           t_TmIntv                 *ps_tiFltStmd,
                           t_TmIntv                 *ps_tiFltOwd,
                           UINT8                    *pb_mode);

extern void CLKflt_inpP2Pdel(const t_TmIntv           *ps_tiP2PDel,
                             const t_ptpTimeStampType *ps_txTsPDlrq,
                             const t_ptpTimeStampType *ps_rxTsPDlrsp,
                             UINT16                   w_ifIdx,
                             UINT32                   dw_TaP2PIntvUsec,
                             t_TmIntv                 *ps_tiFltP2Pdel,
                             UINT8                    *pb_mode);

extern void CLKflt_inpNewMst(const void *ps_pIdNewMst,
                             const void *ps_pAddrNewMst,
                             BOOLEAN    o_mstIsUnicast);

extern void CLKflt_resetFilter( void );

extern void Get_Clear_Counters(int *m2s, int *s2m);

/*
----------------------------------------------------------------------------
                                SC_ClkFltInit()

Description:
This function set initial clock servo configuration parameters. The
parameter will be used when SC_InitConfigComplete() is called.

Parameters:

Inputs
	ps_fltCfg
	Not used. Can be set to NULL.

	ppf_inpMtsd
	ppf_inpStmd
	ppf_inpP2P
	ppf_inpNewMst
	ppf_fltReset

Outputs:
	None

Return value:
	 TRUE: function succeeded
	FALSE: function failed

-----------------------------------------------------------------------------
*/
extern BOOLEAN SC_ClkFltInit(
	const t_fltCfg    *ps_fltCfg,
	t_FUNC_INP_MTSD   *ppf_inpMtsd,
	t_FUNC_INP_STMD   *ppf_inpStmd,
	t_FUNC_INP_P2P    *ppf_inpP2P,
	t_FUNC_INP_NEWMST *ppf_inpNewMst,
	t_FUNC_FLT_RESET  *ppf_fltReset);

/*
----------------------------------------------------------------------------
                                SC_GetFltCfg()

Description:
This function replaces FIO_ReadFltCfg function.

Parameters:

Inputs
        None

Outputs:
        ps_cfgFlt
        Pointer to structure the filter structure to be written to.

Return value:
        TRUE:  function succeeded
        FALSE: function failed

-----------------------------------------------------------------------------
*/
extern BOOLEAN SC_GetFltCfg(t_fltCfg* ps_cfgFlt);
#endif /*  H_clkflt_h */








