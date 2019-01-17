/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: FLTno.c
**    Summary: This unit provides the offset calculation 
**             without filtering.
**              
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: FLTno_Init
**             FLTno_inpMTSD
**             FLTno_inpSTMD
**             FLTno_inpP2Pdel
**             FLTno_inpNewMst
**             FLTno_resetFilter
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
#include "target.h"
#include "PTP/PTPdef.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "CTL/CTLflt.h"
#include "FLT/FLTno.h"

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
static PTP_t_TmIntv s_tiLastMtsd;
static PTP_t_TmIntv s_tiOwd;
static PTP_t_TmIntv as_tiPeerDelay[k_NUM_IF];

/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**  
** Function    : FLTno_Init
**  
** Description : Initialization function for this filter implementation.
**               Initializes the function pointers for the input functions
**               of this filter implementation.
**  
** Parameters  : ps_fltCfg      (IN) - filter configuration file
**               ppf_inpMtsd   (OUT) - input MTSD function-pointer
**               ppf_inpStmd   (OUT) - input STMD function-pointer
**               ppf_inpP2P    (OUT) - input P2P delay function-pointer
**               ppf_inpNewMst (OUT) - input function for new master
**               ppf_fltReset  (OUT) - reset function
**               
** Returnvalue : TRUE  - filter initialization succeeded
**               FALSE - filter initialization failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN FLTno_Init( const PTP_t_fltCfg    *ps_fltCfg,
                    PTP_t_FUNC_INP_MTSD   *ppf_inpMtsd,
                    PTP_t_FUNC_INP_STMD   *ppf_inpStmd,
                    PTP_t_FUNC_INP_P2P    *ppf_inpP2P,
                    PTP_t_FUNC_INP_NEWMST *ppf_inpNewMst,
                    PTP_t_FUNC_FLT_RESET  *ppf_fltReset)
{
  /* unused variables */
  ps_fltCfg = ps_fltCfg;
  /* set function pointers */
  *ppf_inpMtsd   = FLTno_inpMTSD;
  *ppf_inpStmd   = FLTno_inpSTMD;
  *ppf_inpP2P    = FLTno_inpP2Pdel;
  *ppf_inpNewMst = FLTno_inpNewMst;
  *ppf_fltReset  = FLTno_resetFilter;
  /* call reset function */
  FLTno_resetFilter();
  /* always return TRUE - this implementation cannot fail */
  return TRUE;
}

/***********************************************************************
**  
** Function    : FLTno_inpMTSD
**  
** Description : Input function for the master-to-slave-delay measurements.
**               The function stores the actual MTSD. Later this 
**               measurement is used to determine the STMD, if E2E-delay
**               measurement is used. The function returns the 
**               offset-from-master value. Therefore, the last computed
**               one-way-delay or P2P-path-delay is subtracted from the
**               actual MTSD.
**  
** Parameters  : ps_tiMtsd         (IN) - MTSD input
**               ps_txTsSyn        (IN) - origin timestamp of sync message
**               ps_rxTsSyn        (IN) - receive timestamp of sync message
**               w_ifIdx           (IN) - comm. interface index of measurement
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
BOOLEAN FLTno_inpMTSD( const PTP_t_TmIntv *ps_tiMtsd,
                       const PTP_t_TmStmp *ps_txTsSyn,
                       const PTP_t_TmStmp *ps_rxTsSyn,
                       UINT16             w_ifIdx,
                       UINT32             dw_TaSynIntvUsec,
                       PTP_t_TmIntv       *ps_tiOffset,
                       UINT32             *pdw_TaOffsUsec,
                       PTP_t_TmIntv       *ps_tiFltMtsd,
                       UINT8              *pb_mode)
{
  /* unused variables */
  ps_txTsSyn = ps_txTsSyn;
  ps_rxTsSyn = ps_rxTsSyn;
  w_ifIdx    = w_ifIdx;

#if( k_CLK_DEL_E2E == TRUE )
  /* get offset from master */
  ps_tiOffset->ll_scld_Nsec = ps_tiMtsd->ll_scld_Nsec - 
                              s_tiOwd.ll_scld_Nsec;
#else
  ps_tiOffset->ll_scld_Nsec = ps_tiMtsd->ll_scld_Nsec - 
                              as_tiPeerDelay[w_ifIdx].ll_scld_Nsec;
#endif
  /* remember last MTSD value */
  s_tiLastMtsd = *ps_tiMtsd;
  /* set Ta for offset to value of sync interval */
  *pdw_TaOffsUsec = dw_TaSynIntvUsec;
  /* return filtered value for MTSD */
  *ps_tiFltMtsd = *ps_tiMtsd;
  /* filter mode is running */
  *pb_mode = k_RUNNING;
  /* use every computed value */
  return TRUE;
}

/***********************************************************************
**  
** Function    : FLTno_inpSTMD
**  
** Description : Input function for the slave-to-master-delay measurements.
**               The function calculates the actual one-way-delay. Later this 
**               measurement is used to determine the offset-from-master, 
**               if E2E-delay measurement is used. 
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
void FLTno_inpSTMD(const PTP_t_TmIntv *ps_tiStmd,
                   const PTP_t_TmStmp *ps_rxTsDlrsp,
                   const PTP_t_TmStmp *ps_txTsDlrq,
                   UINT32             dw_TaDlrqIntv,
                   PTP_t_TmIntv       *ps_tiFltStmd,
                   PTP_t_TmIntv       *ps_tiFltOwd,
                   UINT8              *pb_mode)
{
  /* unused variables */
  ps_rxTsDlrsp   = ps_rxTsDlrsp;
  ps_txTsDlrq    = ps_txTsDlrq;
  dw_TaDlrqIntv  = dw_TaDlrqIntv;

  /* get one-way-delay */
  s_tiOwd.ll_scld_Nsec  = ( s_tiLastMtsd.ll_scld_Nsec + 
                            ps_tiStmd->ll_scld_Nsec     )/(INT64)2;
  /* set owd output */
  *ps_tiFltOwd  = s_tiOwd;
  /* return filtered STMD */
  *ps_tiFltStmd = *ps_tiStmd;
  /* filter mode is always running */
  *pb_mode = k_RUNNING;
}

/***********************************************************************
**  
** Function    : FLTno_inpP2Pdel
**  
** Description : Input function for the peer-to-peer-delay measurements.
**               The function stores the actual peer-to-peer-delay. Later this 
**               measurement is used to determine the offset-from-master, 
**               if P2P-delay measurement is used. 
**  
** Parameters  : ps_tiP2PDel       (IN) - actual measured P2P delay
**               ps_txTsPDlrq      (IN) - tx timestamp of Pdel-req
**               ps_rxTsPDlrsp     (IN) - rx timestamp of Pdel-resp (corrected)
**               w_ifIdx           (IN) - comm. interface index of measurement
**               dw_TaP2PIntvUsec  (IN) - mean time interval of P2P in usec
**               ps_tiFltP2Pdel   (OUT) - filtered P2P delay
**               pb_mode          (OUT) - filter mode (HOLDOVER/RUNNING)
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void FLTno_inpP2Pdel( const PTP_t_TmIntv *ps_tiP2PDel,
                      const PTP_t_TmStmp *ps_txTsPDlrq,
                      const PTP_t_TmStmp *ps_rxTsPDlrsp,
                      UINT16             w_ifIdx,
                      UINT32             dw_TaP2PIntvUsec,
                      PTP_t_TmIntv       *ps_tiFltP2Pdel,
                      UINT8              *pb_mode)
{
  /* unused variables in this implementation */
  ps_txTsPDlrq     = ps_txTsPDlrq;
  ps_rxTsPDlrsp    = ps_rxTsPDlrsp;
  dw_TaP2PIntvUsec = dw_TaP2PIntvUsec;
  /* store peer delay */
  as_tiPeerDelay[w_ifIdx] = *ps_tiP2PDel;
  /* return filtered peer delay */
  *ps_tiFltP2Pdel = *ps_tiP2PDel;
  /* mode is always 'RUNNING' */
  *pb_mode = k_RUNNING;  
}

/***********************************************************************
**  
** Function    : FLTno_inpNewMst
**  
** Description : Informs the filter of a change to a new master
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
void FLTno_inpNewMst(const PTP_t_PortId   *ps_pIdNewMst,
                     const PTP_t_PortAddr *ps_pAddrNewMst,
                     BOOLEAN              o_mstIsUnicast)
{
  /* unused variables in this implementation */
  ps_pIdNewMst   = ps_pIdNewMst;
  ps_pAddrNewMst = ps_pAddrNewMst;
  o_mstIsUnicast = o_mstIsUnicast;
  /* just reset filter */
  FLTno_resetFilter();
}

/***********************************************************************
**  
** Function    : FLTno_resetFilter
**  
** Description : Resets the filter
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void FLTno_resetFilter( void )
{
  /* initialize old MTSD and STMD values */
  s_tiLastMtsd.ll_scld_Nsec = (INT64)0;
  s_tiOwd.ll_scld_Nsec      = (INT64)0;
}


/*************************************************************************
**    static functions
*************************************************************************/
