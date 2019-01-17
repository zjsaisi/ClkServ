/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: FLTmin.c
**    Summary: This unit implements the minimum filter methods
**             The minimum filter finds the minimum values of a
**             predefined time-window and uses this value to trigger
**             the PI controller. This means oversampling and therefore
**             the control-loop reacts slowlier.
**              
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: FLTmin_Init
**             FLTmin_inpMTSD
**             FLTmin_inpSTMD
**             FLTmin_inpP2Pdel
**             FLTmin_inpNewMst
**             FLTmin_resetFilter
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
#include "FLT/FLTmin.h"

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
#define k_THRES_INIT_CNT (30u) /* initialization measurements */

static UINT32 dw_filtLen = 0;
/* counters */
static UINT32  dw_callCntMtsd = 0;
/* minimum value */
static INT64  ll_tiMinMtsd   = k_MAX_I64;
static INT64  ll_tiMinStmd   = k_MAX_I64;
static INT64  ll_lastMinStmd;
static INT64  ll_lastP2Pdel;/*lint !e551*/
static INT64  ll_Owd = (INT64)0;
static INT64  all_tiMinP2PDel[k_NUM_IF];
/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**  
** Function    : FLTmin_Init
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
BOOLEAN FLTmin_Init( const PTP_t_fltCfg    *ps_fltCfg,
                     PTP_t_FUNC_INP_MTSD   *ppf_inpMtsd,
                     PTP_t_FUNC_INP_STMD   *ppf_inpStmd,
                     PTP_t_FUNC_INP_P2P    *ppf_inpP2P,
                     PTP_t_FUNC_INP_NEWMST *ppf_inpNewMst,
                     PTP_t_FUNC_FLT_RESET  *ppf_fltReset)
{
  BOOLEAN o_ret;
  /* call init function of 'no filtering - it is used for startup */
  o_ret = FLTno_Init(ps_fltCfg,ppf_inpMtsd,ppf_inpStmd,
                     ppf_inpP2P,ppf_inpNewMst,ppf_fltReset);
  
  /* set function pointers */
  *ppf_inpMtsd   = FLTmin_inpMTSD;
  *ppf_inpStmd   = FLTmin_inpSTMD;
  *ppf_inpP2P    = FLTmin_inpP2Pdel;
  *ppf_inpNewMst = FLTmin_inpNewMst;
  *ppf_fltReset  = FLTmin_resetFilter;

  /* init filter length in amount of measurements */
  dw_filtLen = ps_fltCfg->s_filtMin.dw_filtLen;
  
  /* call reset function */
  FLTmin_resetFilter();
  return o_ret;
}

/***********************************************************************
**  
** Function    : FLTmin_inpMTSD
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
**               w_ifIdx           (IN) - communication interface index
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
BOOLEAN FLTmin_inpMTSD( const PTP_t_TmIntv *ps_tiMtsd,
                        const PTP_t_TmStmp *ps_txTsSyn,
                        const PTP_t_TmStmp *ps_rxTsSyn,
                        UINT16             w_ifIdx,
                        UINT32             dw_TaSynIntvUsec,
                        PTP_t_TmIntv       *ps_tiOffset,
                        UINT32             *pdw_TaOffsUsec,
                        PTP_t_TmIntv       *ps_tiFltMtsd,
                        UINT8              *pb_mode)
{
  BOOLEAN o_ret;
  static UINT32 dw_mtsdMsrCnt = 0;
  static INT64  ll_minOffs;
  static INT64  ll_lastMinMtsd;
  static UINT32 dw_actMsrPeriod;
#if( k_CLK_DEL_P2P == TRUE )
  INT64 ll_actP2Pdel;
#endif
 
  /* unused variables */
  ps_txTsSyn = ps_txTsSyn;
  ps_rxTsSyn = ps_rxTsSyn;

  /* it takes some cycles to have a stable system.
      Filtering should be done after that cycles */
  if( dw_callCntMtsd < k_THRES_INIT_CNT )
  {
    dw_callCntMtsd++;
    /* call 'NoFilter' implementation */
    o_ret = FLTno_inpMTSD(ps_tiMtsd,ps_txTsSyn,ps_rxTsSyn,w_ifIdx,dw_TaSynIntvUsec,
                          ps_tiOffset,pdw_TaOffsUsec,ps_tiFltMtsd,pb_mode);
    /* set measurement count to zero */
    dw_mtsdMsrCnt = 0;
    /* start with measure period = 2 */
    dw_actMsrPeriod = 2;
    /* remember last min MTSD */
    ll_lastMinMtsd = ps_tiFltMtsd->ll_scld_Nsec;
    /* change mode to HOLDOVER */
    *pb_mode = k_HOLDOVER;
    if( dw_callCntMtsd == k_THRES_INIT_CNT )
    {
      /* change trigger interval, if filtering starts now */
      *pdw_TaOffsUsec = dw_TaSynIntvUsec * dw_actMsrPeriod;    
    }
  }  
  else
  {
    /* compare actual value with min value */
    if( ps_tiMtsd->ll_scld_Nsec < ll_tiMinMtsd )
    {
      /* remember min value */
      ll_tiMinMtsd = ps_tiMtsd->ll_scld_Nsec;
    }  
    /* increment measurement count */
    dw_mtsdMsrCnt++;
    
    /* period over ? */
    if( dw_mtsdMsrCnt == dw_actMsrPeriod )
    {
      /* get next measure period */
      if( dw_actMsrPeriod < dw_filtLen )
      {
        dw_actMsrPeriod++;
      }
      /* reset measure count */
      dw_mtsdMsrCnt = 0;
      /* remember this as last period min MTSD */
      ll_lastMinMtsd = ll_tiMinMtsd;
#if( k_CLK_DEL_E2E == TRUE ) 
      if( ll_tiMinStmd != k_MAX_I64 )
      {
        /* compute OWD */
        ll_Owd = (ll_tiMinMtsd + ll_tiMinStmd)/(INT64)2;
        /* remember last period STMD */
        ll_lastMinStmd = ll_tiMinStmd;
      }
      else
      {
        ll_Owd = (ll_tiMinMtsd + ll_lastMinStmd)/(INT64)2;
      }
      /* get actual offset */     
      ll_minOffs = ll_tiMinMtsd - ll_Owd;
#else
      if( all_tiMinP2PDel[w_ifIdx] == k_MAX_I64 )
      {
        ll_actP2Pdel = ll_lastP2Pdel;
      }
      else
      {
        ll_actP2Pdel = all_tiMinP2PDel[w_ifIdx];
        /* remember old value */
        ll_lastP2Pdel = ll_actP2Pdel;
      }
      /* get actual offset */
      ll_minOffs = ll_tiMinMtsd - ll_actP2Pdel;
      /* reset stored min-value */
      all_tiMinP2PDel[w_ifIdx] = k_MAX_I64;
#endif
      /* reset stored min values */
      ll_tiMinMtsd = k_MAX_I64;
      ll_tiMinStmd = k_MAX_I64;
      /* use this value */
      o_ret = TRUE;
    }
    else
    {
      /* do not use this value for control loop triggering */
      o_ret = FALSE;
    }    
    
    /* set offset to actual min offset */
    ps_tiOffset->ll_scld_Nsec = ll_minOffs;
    /* set filtered min value for MTSD */
    ps_tiFltMtsd->ll_scld_Nsec = ll_lastMinMtsd;
    /* set filter mode to running */
    *pb_mode = k_RUNNING;
    /* scale trigger interval */
    *pdw_TaOffsUsec = dw_TaSynIntvUsec * dw_actMsrPeriod;
  }
  return o_ret;
}

/***********************************************************************
**  
** Function    : FLTmin_inpSTMD
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
void FLTmin_inpSTMD(const PTP_t_TmIntv *ps_tiStmd,
                    const PTP_t_TmStmp *ps_rxTsDlrsp,
                    const PTP_t_TmStmp *ps_txTsDlrq,
                    UINT32             dw_TaDlrqIntv,
                    PTP_t_TmIntv       *ps_tiFltStmd,
                    PTP_t_TmIntv       *ps_tiFltOwd,
                    UINT8              *pb_mode)
{
  /* unused variables in this filter */
  ps_rxTsDlrsp  = ps_rxTsDlrsp;
  ps_txTsDlrq   = ps_txTsDlrq;
  dw_TaDlrqIntv = dw_TaDlrqIntv;

  /* it takes some cycles to have a stable system.
      Filtering should be done after that cycles */
  if( dw_callCntMtsd < k_THRES_INIT_CNT )
  {
    /* call 'NoFilter' implementation */
    FLTno_inpSTMD(ps_tiStmd,ps_rxTsDlrsp,ps_txTsDlrq,dw_TaDlrqIntv,
                  ps_tiFltStmd,ps_tiFltOwd,pb_mode);
    /* remember OWD */
    ll_Owd = ps_tiFltOwd->ll_scld_Nsec;
    /* remember last value for STMD */
    ll_lastMinStmd = ps_tiFltStmd->ll_scld_Nsec;
    /* change mode to HOLDOVER */
    *pb_mode = k_HOLDOVER;
  } 
  else
  {
    /* compare this measurement with the last one */
    if( ps_tiStmd->ll_scld_Nsec < ll_tiMinStmd )
    {
      ll_tiMinStmd = ps_tiStmd->ll_scld_Nsec;      
    }
    /* return OWD */
    ps_tiFltOwd->ll_scld_Nsec = ll_Owd;
    /* return filtered Stmd */
    ps_tiFltStmd->ll_scld_Nsec = ll_lastMinStmd;
    /* set mode to running */
    *pb_mode = k_RUNNING;
  }
}

/***********************************************************************
**  
** Function    : FLTmin_inpP2Pdel
**  
** Description : Input function for the peer-to-peer-delay measurements.
**               The function stores the actual peer-to-peer-delay. Later this 
**               measurement is used to determine the offset-from-master, 
**               if P2P-delay measurement is used. 
**  
** Parameters  : ps_tiP2PDel       (IN) - actual measured P2P delay
**               ps_txTsPDlrq      (IN) - tx timestamp of Pdel-req
**               ps_rxTsPDlrsp     (IN) - rx timestamp of Pdel-resp (corrected)
**               w_ifIdx           (IN) - communication interface index
**               dw_TaP2PIntvUsec  (IN) - mean time interval of P2P in usec
**               ps_tiFltP2Pdel   (OUT) - filtered P2P delay
**               pb_mode          (OUT) - filter mode (HOLDOVER/RUNNING)
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void FLTmin_inpP2Pdel( const PTP_t_TmIntv *ps_tiP2PDel,
                       const PTP_t_TmStmp *ps_txTsPDlrq,
                       const PTP_t_TmStmp *ps_rxTsPDlrsp,
                       UINT16             w_ifIdx,
                       UINT32             dw_TaP2PIntvUsec,
                       PTP_t_TmIntv       *ps_tiFltP2Pdel,
                       UINT8              *pb_mode)
{
  /* unused variables in this filter */
  ps_txTsPDlrq     = ps_txTsPDlrq;
  ps_rxTsPDlrsp    = ps_rxTsPDlrsp;
  dw_TaP2PIntvUsec = dw_TaP2PIntvUsec;

  /* it takes some cycles to have a stable system.
      Filtering should be done after that cycles */
  if( dw_callCntMtsd < k_THRES_INIT_CNT )
  {
    /* call 'NoFilter' implementation */
    FLTno_inpP2Pdel(ps_tiP2PDel,ps_txTsPDlrq,ps_rxTsPDlrsp,
                    w_ifIdx,dw_TaP2PIntvUsec,ps_tiFltP2Pdel,pb_mode);
    /* remember P2P del */
    ll_lastP2Pdel = ps_tiFltP2Pdel->ll_scld_Nsec;
    /* change mode to HOLDOVER */
    *pb_mode = k_HOLDOVER;
  } 
  else
  {
    /* compare this measurement with the last one */
    if( ps_tiP2PDel->ll_scld_Nsec < all_tiMinP2PDel[w_ifIdx] )
    {
      all_tiMinP2PDel[w_ifIdx] = ps_tiP2PDel->ll_scld_Nsec;      
    }
    /* return filtered P2P delay */
    ps_tiFltP2Pdel->ll_scld_Nsec = all_tiMinP2PDel[w_ifIdx];
    /* set mode to running */
    *pb_mode = k_RUNNING;
  }
}

/***********************************************************************
**  
** Function    : FLTmin_inpNewMst
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
void FLTmin_inpNewMst(const PTP_t_PortId   *ps_pIdNewMst,
                      const PTP_t_PortAddr *ps_pAddrNewMst,
                      BOOLEAN              o_mstIsUnicast)
{
  /* unused variables in this implementation */
  ps_pIdNewMst   = ps_pIdNewMst;
  ps_pAddrNewMst = ps_pAddrNewMst;
  o_mstIsUnicast = o_mstIsUnicast;
  /* reset filter */
  FLTmin_resetFilter();
}

/***********************************************************************
**  
** Function    : FLTmin_resetFilter
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
void FLTmin_resetFilter( void )
{
  UINT16 w_i;
  /* reset variables */
  dw_callCntMtsd = 0;
  ll_tiMinMtsd  = k_MAX_I64;
  ll_tiMinStmd  = k_MAX_I64;
  ll_lastP2Pdel = 0;
  ll_Owd = (INT64)0;
  for( w_i = 0 ; w_i < k_NUM_IF ; w_i++ )
  {
    all_tiMinP2PDel[w_i] = k_MAX_I64;
  }
  /* call reset function of 'no filter implementation' */
  FLTno_resetFilter();
}
/*************************************************************************
**    static functions
*************************************************************************/
