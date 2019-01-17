/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: CTLflt.c 
**    Summary: This unit implements the filter unit for PDV filtering
**             of the delay measurements. There are different filters
**             included in the stack that are chosen and configured
**             by the filter configuration file stored in ROM.
**              
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: CTLflt_Init
**             CTLflt_chgFilter
**             CTLflt_inpMTSD
**             CTLflt_inpSTMD
**             CTLflt_inpP2Pdel
**             CTLflt_inpNewMst
**             CTLflt_resetFilter
**
**   Compiler: Ansi-C
**    Remarks:
**
**************************************************************************
**    all rights reserved
*************************************************************************/


/*************************************************************************
**    compiler directives
*************************************************************************/


/*************************************************************************
**    include-files
*************************************************************************/
#include <stdio.h>
#include "target.h"
#include "PTP/PTPdef.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "CTL/CTLint.h"
#include "CTL/CTLflt.h"
#include "FLT/FLTno.h"
#include "FLT/FLTmin.h"
#include "FLT/FLTest.h"
#include "sc_servo_api.h"
#include "CLK/clk_private.h"
#include "CLK/ptp.h"
#include "CLK/CLKflt.h"

/*************************************************************************
**    global variables
*************************************************************************/
/* function pointer to be initialized to call the appropriate filter */
static PTP_t_FUNC_FLT_INIT   pf_fltInit   = NULL;
static PTP_t_FUNC_INP_MTSD   pf_inpMtsd   = NULL;
static PTP_t_FUNC_INP_STMD   pf_inpStmd   = NULL;
static PTP_t_FUNC_INP_P2P    pf_inpP2Pd   = NULL;
static PTP_t_FUNC_INP_NEWMST pf_inpNewMst = NULL;
static PTP_t_FUNC_FLT_RESET  pf_rstFlt    = NULL;

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**  
** Function    : CTLflt_Init
**  
** Description : Initializes the filter unit. The filter configuration 
**               is read and the used filter is parametrized.
**               If it is impossible to open filter configuration,
**               there is 'no filtering' applied.
**  
** Parameters  : ps_fltCfg (IN) - filter configuration file
**               
** Returnvalue : -
** 
** Remarks     : 
**  
***********************************************************************/
void CTLflt_Init( const PTP_t_fltCfg *ps_fltCfg )
{
#if 0
  /* get the filter initialization function */
  switch( ps_fltCfg->e_usedFilter )
  {
    /* choice is no filter */
    case e_FILT_NO:
    {
      /* initialize function pointer to init function */
      pf_fltInit = FLTno_Init;
      break;
    }
    /* choice is minimum filter */
    case e_FILT_MIN:
    {
      /* initialize function pointer to init function */
      pf_fltInit = FLTmin_Init;
      break;
    }
    /* choice is estimation filter */
    case e_FILT_EST:
    {
      /* initialize function pointer to init function */
      pf_fltInit = FLTest_Init;
      break;
    }
    /* choice is unknown - use no filter */
    case e_FILT_ISPCFC:
    default:
    {
      /* initialize function pointers to 'no-filter' */
      pf_fltInit = FLTno_Init;
      /* set notice */
      PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_FILT_DNE,e_SEVC_INFO);
      break;
    }
  }
#endif
   pf_fltInit = (PTP_t_FUNC_FLT_INIT) SC_ClkFltInit;

  /* call the filter initialization function that initializes 
     the filter handler functions */
  if( pf_fltInit(ps_fltCfg,&pf_inpMtsd,&pf_inpStmd,&pf_inpP2Pd,
                 &pf_inpNewMst,&pf_rstFlt) == FALSE )
  {
    /* if initialization fails, use no-filter implementation */
    FLTno_Init(ps_fltCfg,&pf_inpMtsd,&pf_inpStmd,&pf_inpP2Pd,
               &pf_inpNewMst,&pf_rstFlt); /*lint !e534*/
    /* set error */
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_FILT_INIT,e_SEVC_INFO);
  }
}

/***********************************************************************
**  
** Function    : CTLflt_chgFilter
**  
** Description : External function to exchange the used filter
**               Registers the input filter functions for the
**               external custom filter implementation
**  
** Parameters  : pf_inpMtsdExt   (IN) - filter function to input MTSD
**               pf_inpStmdExt   (IN) - filter function to input STMD
**               pf_inpP2PdExt   (IN) - filter function to input P2P delay
**               pf_inpNewMstExt (IN) - filter function to input new master id
**               pf_rstFltExt    (IN) - filter function to reset the filter
**               
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN CTLflt_chgFilter(PTP_t_FUNC_INP_MTSD   pf_inpMtsdExt,
                         PTP_t_FUNC_INP_STMD   pf_inpStmdExt,
                         PTP_t_FUNC_INP_P2P    pf_inpP2PdExt,
                         PTP_t_FUNC_INP_NEWMST pf_inpNewMstExt,
                         PTP_t_FUNC_FLT_RESET  pf_rstFltExt)
{
  BOOLEAN o_ret;

  /* check, if all functions are OK */
  o_ret = ((pf_inpMtsdExt   != NULL) &&
#if( k_CLK_DEL_E2E == TRUE )
           (pf_inpStmdExt   != NULL) &&
#else
           (pf_inpP2PdExt   != NULL) &&
#endif
           (pf_inpNewMstExt != NULL) &&
           (pf_rstFltExt    != NULL));
  /* set function pointer to external functions if all are OK */
  if( o_ret == TRUE )
  {
    pf_inpMtsd   = pf_inpMtsdExt;
    pf_inpStmd   = pf_inpStmdExt;
    pf_inpP2Pd   = pf_inpP2PdExt;
    pf_inpNewMst = pf_inpNewMstExt;
    pf_rstFlt    = pf_rstFltExt;
  }
  return o_ret;
}

/***********************************************************************
**  
** Function    : CTLflt_inpMTSD
**  
** Description : Calls the configured MTSD input function.
**  
** Parameters  : ps_tiMtsd         (IN) - MTSD input
**               ps_txTsSyn        (IN) - origin timestamp of sync message
**               ps_rxTsSyn        (IN) - receive timestamp of sync message
**               w_ifIdx           (IN) - actual comm. interface index
**               dw_TaSynIntvUsec  (IN) - sync interval in usec
**               ps_tiOffset      (OUT) - actual offset from master
**               pdw_TaOffsUsec   (OUT) - trigger interval for PI contr. in usec
**               ps_tiFltMtsd     (OUT) - filtered MTSD value
**               pb_mode          (OUT) - filter mode (HOLDOVER/RUNNING)
**               
** Returnvalue : TRUE  - trigger actual value into PI controller
**               FALSE - do not trigger actual value into PI controller
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN CTLflt_inpMTSD(const PTP_t_TmIntv *ps_tiMtsd,
                       const PTP_t_TmStmp *ps_txTsSyn,
                       const PTP_t_TmStmp *ps_rxTsSyn,
                       UINT16             w_ifIdx,
                       UINT32             dw_TaSynIntvUsec,
                       PTP_t_TmIntv       *ps_tiOffset,
                       UINT32             *pdw_TaOffsUsec,
                       PTP_t_TmIntv       *ps_tiFltMtsd,
                       UINT8              *pb_mode)
{
  /* call implementation specific routine */
  if( pf_inpMtsd != NULL )
  {
    return pf_inpMtsd(ps_tiMtsd,ps_txTsSyn,ps_rxTsSyn,w_ifIdx,dw_TaSynIntvUsec,
                      ps_tiOffset,pdw_TaOffsUsec,ps_tiFltMtsd,pb_mode);
  }
  else
  {
    /* set error */
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_FLT_FUNC,e_SEVC_EMRGC);
    return FALSE;
  }
}

/***********************************************************************
**  
** Function    : CTLflt_inpSTMD
**  
** Description : Calls the configured STMD input function.
**  
** Parameters  : ps_tiStmd      (IN) - input STMD measurement
**               ps_rxTsDlrsp   (IN) - rx timestamp of delay response
**               ps_txTsDlrq    (IN) - tx timestamp of delay request
**               dw_TaDlrqIntv  (IN) - mean time interval of Dreq in usec
**               ps_tiFltStmd  (OUT) - filtered STMD
**               ps_tiFltOwd   (OUT) - filtered OWD
**               pb_mode       (OUT) - filter mode (HOLDOVER/RUNNING)
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTLflt_inpSTMD(const PTP_t_TmIntv *ps_tiStmd,
                    const PTP_t_TmStmp *ps_rxTsDlrsp,
                    const PTP_t_TmStmp *ps_txTsDlrq,
                    UINT32             dw_TaDlrqIntv,
                    PTP_t_TmIntv       *ps_tiFltStmd,
                    PTP_t_TmIntv       *ps_tiFltOwd,
                    UINT8              *pb_mode)
{
  /* call implementation specific routine */
  if( pf_inpStmd != NULL )
  {
    pf_inpStmd(ps_tiStmd,ps_rxTsDlrsp,ps_txTsDlrq,dw_TaDlrqIntv,
               ps_tiFltStmd,ps_tiFltOwd,pb_mode);
  }
  else
  {
    /* set error */
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_FLT_FUNC,e_SEVC_EMRGC);
  }
}
                       
/***********************************************************************
**  
** Function    : CTLflt_inpP2Pdel
**  
** Description : Calls the configured P2P delay input function
**  
** Parameters  : ps_tiP2PDel       (IN) - actual measured P2P delay
**               ps_txTsPDlrq      (IN) - tx timestamp of Pdel-req
**               ps_rxTsPDlrsp     (IN) - rx timestamp of Pdel-resp (corrected)
**               w_ifIdx           (IN) - comm. interface index of measurement
**               dw_taP2PIntvUsec  (IN) - mean time interval of P2P in usec
**               ps_tiFltP2Pdel   (OUT) - filtered P2P delay
**               pb_mode          (OUT) - filter mode (HOLDOVER/RUNNING)
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTLflt_inpP2Pdel( const PTP_t_TmIntv *ps_tiP2PDel,
                       const PTP_t_TmStmp *ps_txTsPDlrq,
                       const PTP_t_TmStmp *ps_rxTsPDlrsp,
                       UINT16             w_ifIdx,
                       UINT32             dw_taP2PIntvUsec,
                       PTP_t_TmIntv       *ps_tiFltP2Pdel,
                       UINT8              *pb_mode)
{
  /* call implementation specific routine */
  if( pf_inpP2Pd != NULL )
  {
    pf_inpP2Pd(ps_tiP2PDel,ps_txTsPDlrq,ps_rxTsPDlrsp,w_ifIdx,dw_taP2PIntvUsec,
               ps_tiFltP2Pdel,pb_mode);
  }
  else
  {
    /* set error */
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_FLT_FUNC,e_SEVC_EMRGC);
  }
}
                                                
/***********************************************************************
**  
** Function    : CTLflt_inpNewMst
**  
** Description : Calls the configured input function for new masters.
**  
** Parameters  : ps_pIdNewMst   (IN/OUT) - Port ID of new master
**               ps_pAddrNewMst (IN/OUT) - Port Address of new master
**               o_mstIsUnicast (IN/OUT) - flag determines, if master is unicast
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTLflt_inpNewMst(const PTP_t_PortId   *ps_pIdNewMst,
                      const PTP_t_PortAddr *ps_pAddrNewMst,
                      BOOLEAN              o_mstIsUnicast)
{
  /* call implementation specific routine */
  if( pf_inpNewMst != NULL )
  {
    pf_inpNewMst(ps_pIdNewMst,ps_pAddrNewMst,o_mstIsUnicast);
  }
  else
  {
    /* set error */
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_FLT_FUNC,e_SEVC_EMRGC);
  }
}
 
/***********************************************************************
**  
** Function    : CTLflt_resetFilter
**  
** Description : Calls the configured reset function.
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTLflt_resetFilter( void )
{
  /* call implementation specific routine */
  if( pf_rstFlt != NULL )
  {
    pf_rstFlt();
  }
  else
  {
    /* set error */
    PTP_SetError(k_CTL_ERR_ID,CTL_k_ERR_FLT_FUNC,e_SEVC_EMRGC);
  }
}

/*************************************************************************
**    static functions
*************************************************************************/
