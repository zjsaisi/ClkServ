/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: FLTest.c
**    Summary: This unit implements the estimation filter methods
**             The estimation filter enables itself after reaching
**             a predefined performance, measured by standard deviation
**             of the measurement values. 
**             After enabling, the estimation filter cuts the measurement
**             values that deviate more than the predefined cutoff value.
**              
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: FLTest_Init
**             FLTest_inpMTSD
**             FLTest_inpSTMD
**             FLTest_inpP2Pdel
**             FLTest_inpNewMst
**             FLTest_resetFilter
**             Estimator
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
#include "SIS/SIS.h"
#include "CTL/CTLflt.h"
#include "FLT/FLTest.h"

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/
#define k_MTSD (0u)
#define k_STMD (1u)
#define k_P2P  (1u)
#if( k_CLK_DEL_E2E == TRUE )
#define k_AMNT_EST_VAR (2u)
#else
#define k_AMNT_EST_VAR (1+k_NUM_IF) /* amount of used test arrays */
#endif
static INT32 *apl_HistNsec[k_AMNT_EST_VAR] = {NULL};

/* estimator variables */
static PTP_t_TmIntv as_oldDelVal[k_AMNT_EST_VAR] = {{0}}; 
static UINT8        ab_succCuts[k_AMNT_EST_VAR] = {0};
static UINT32       adw_samples[k_AMNT_EST_VAR] = {0};    
/* variables for standard deviation function */
static UINT16 aw_amnt[k_AMNT_EST_VAR]   = {0};
static UINT16 aw_pos[k_AMNT_EST_VAR]    = {0};
static UINT16 aw_oldPos[k_AMNT_EST_VAR] = {0};
static UINT64 addw_sum_powedxi[k_AMNT_EST_VAR] = {0};
static INT64  all_sum[k_AMNT_EST_VAR]   = {0};

/* variables used in filter */
static UINT32 dw_histLen;
static UINT32 dw_amntFlt;
static UINT64 ddw_whtNoise;
static INT64  ll_owd = 0;
static INT64  all_tiP2Pdel[k_NUM_IF] = {(INT64)0};/*lint !e551*/
static INT64  ll_lastMtsd = 0;

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static UINT8 Estimator( INT64 ll_inp,UINT8 b_which,INT64* pl_outp );
/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
**  
** Function    : FLTest_Init
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
BOOLEAN FLTest_Init( const PTP_t_fltCfg    *ps_fltCfg,
                     PTP_t_FUNC_INP_MTSD   *ppf_inpMtsd,
                     PTP_t_FUNC_INP_STMD   *ppf_inpStmd,
                     PTP_t_FUNC_INP_P2P    *ppf_inpP2P,
                     PTP_t_FUNC_INP_NEWMST *ppf_inpNewMst,
                     PTP_t_FUNC_FLT_RESET  *ppf_fltReset)
{
  BOOLEAN o_ret = FALSE;
  UINT16 w_i;
  UINT16 w_arr;

  /* set function pointers */
  *ppf_inpMtsd   = FLTest_inpMTSD;
  *ppf_inpStmd   = FLTest_inpSTMD;
  *ppf_inpP2P    = FLTest_inpP2Pdel;
  *ppf_inpNewMst = FLTest_inpNewMst;
  *ppf_fltReset  = FLTest_resetFilter;

  /* get history length */
  dw_histLen  = ps_fltCfg->s_filtEst.b_histLen;
  dw_amntFlt  = ps_fltCfg->s_filtEst.b_amntFlt;
  ddw_whtNoise = ps_fltCfg->s_filtEst.ddw_WhtNoise;

  /* free allocated memory for case of reinitialization */
  for( w_arr = 0 ; w_arr < k_AMNT_EST_VAR ; w_arr++ )
  {
    /* free allocated memory */
    if( apl_HistNsec[w_arr] != NULL )
    {
      SIS_Free(apl_HistNsec[w_arr]);
    }
    if( apl_HistNsec[w_arr] != NULL )
    {
      SIS_Free(apl_HistNsec[w_arr]);
    } 
  }

  o_ret = TRUE;
  /* allocate memory */
  for( w_arr = 0 ; w_arr < k_AMNT_EST_VAR ; w_arr++ )
  {
    apl_HistNsec[w_arr] = 
        (INT32*)SIS_Alloc((ps_fltCfg->s_filtEst.b_histLen + 1) * sizeof(INT32));
    if( apl_HistNsec[w_arr] == NULL )
    {
      o_ret = FALSE;
    }
  }
  /* if allocation was OK */
  if( o_ret == TRUE )
  {
    /* init variables */
    for( w_i = 0 ; w_i < ps_fltCfg->s_filtEst.b_histLen ; w_i++ )
    {
      for( w_arr = 0 ; w_arr < k_AMNT_EST_VAR ; w_arr++ )
      {
        apl_HistNsec[w_arr][w_i] = 0;
        apl_HistNsec[w_arr][w_i] = 0;
      }
    }
    /* call reset function of this filter */
    FLTest_resetFilter();
    /* initialization succeeded*/
    o_ret = TRUE;
  }
  else
  {    
    /* free memory */
    for( w_arr = 0 ; w_arr < k_AMNT_EST_VAR ; w_arr++ )
    {
      /* free allocated memory */
      if( apl_HistNsec[w_arr] != NULL )
      {
        SIS_Free(apl_HistNsec[w_arr]);
      }
      if( apl_HistNsec[w_arr] != NULL )
      {
        SIS_Free(apl_HistNsec[w_arr]);
      } 
    }
    /* initialization failed */
    o_ret = FALSE;
  }  
  return o_ret;
}

/***********************************************************************
**  
** Function    : FLTest_inpMTSD
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
BOOLEAN FLTest_inpMTSD( const PTP_t_TmIntv *ps_tiMtsd,
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
  
  /* call estimator */
  *pb_mode = Estimator(ps_tiMtsd->ll_scld_Nsec,
                       k_MTSD,
                       &ps_tiFltMtsd->ll_scld_Nsec);
#if( k_CLK_DEL_E2E == TRUE )
  /* calculate actual offset */  
  ps_tiOffset->ll_scld_Nsec = ps_tiFltMtsd->ll_scld_Nsec - ll_owd;
  /* remember MTSD value for OWD measurement */
  ll_lastMtsd = ps_tiFltMtsd->ll_scld_Nsec;
#else
  ps_tiOffset->ll_scld_Nsec = ps_tiFltMtsd->ll_scld_Nsec - all_tiP2Pdel[w_ifIdx];
#endif
  /* set trigger interval to same value like sync interval */
  *pdw_TaOffsUsec = dw_TaSynIntvUsec;
  return TRUE;
}

/***********************************************************************
**  
** Function    : FLTest_inpSTMD
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
void FLTest_inpSTMD(const PTP_t_TmIntv *ps_tiStmd,
                    const PTP_t_TmStmp *ps_rxTsDlrsp,
                    const PTP_t_TmStmp *ps_txTsDlrq,
                    UINT32             dw_TaDlrqIntv,
                    PTP_t_TmIntv       *ps_tiFltStmd,
                    PTP_t_TmIntv       *ps_tiFltOwd,
                    UINT8              *pb_mode)
{
  /* unused variables */
  ps_rxTsDlrsp  = ps_rxTsDlrsp;
  ps_txTsDlrq   = ps_txTsDlrq;
  dw_TaDlrqIntv = dw_TaDlrqIntv;
  
  /* call estimator */
  *pb_mode = Estimator(ps_tiStmd->ll_scld_Nsec,
                       k_STMD,
                       &ps_tiFltStmd->ll_scld_Nsec);
  /* calculate actual owd */  
  ll_owd = (ll_lastMtsd + ps_tiFltStmd->ll_scld_Nsec)/(INT64)2;
  /* give filtered owd back */
  ps_tiFltOwd->ll_scld_Nsec = ll_owd;
}

/***********************************************************************
**  
** Function    : FLTest_inpP2Pdel
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
void FLTest_inpP2Pdel( const PTP_t_TmIntv *ps_tiP2PDel,
                       const PTP_t_TmStmp *ps_txTsPDlrq,
                       const PTP_t_TmStmp *ps_rxTsPDlrsp,
                       UINT16             w_ifIdx,
                       UINT32             dw_TaP2PIntvUsec,
                       PTP_t_TmIntv       *ps_tiFltP2Pdel,
                       UINT8              *pb_mode)
{
  /* unused variables */
  ps_rxTsPDlrsp    = ps_rxTsPDlrsp;
  ps_txTsPDlrq     = ps_txTsPDlrq;
  dw_TaP2PIntvUsec = dw_TaP2PIntvUsec;
  
  /* call estimator */
  *pb_mode = Estimator(ps_tiP2PDel->ll_scld_Nsec,
                       k_P2P+w_ifIdx,
                       &ps_tiFltP2Pdel->ll_scld_Nsec);
  /* remember stored peer delay */
  all_tiP2Pdel[w_ifIdx] = ps_tiFltP2Pdel->ll_scld_Nsec;
}

/***********************************************************************
**  
** Function    : FLTest_inpNewMst
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
void FLTest_inpNewMst(const PTP_t_PortId   *ps_pIdNewMst,
                      const PTP_t_PortAddr *ps_pAddrNewMst,
                      BOOLEAN              o_mstIsUnicast)
{
  /* unused variables in this implementation */
  ps_pIdNewMst   = ps_pIdNewMst;
  ps_pAddrNewMst = ps_pAddrNewMst;
  o_mstIsUnicast = o_mstIsUnicast;
  /* reset filter */
  FLTest_resetFilter();
}

/***********************************************************************
**  
** Function    : FLTest_resetFilter
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
void FLTest_resetFilter( void )
{
  UINT16 w_i;
  /* reset variables */
  for( w_i = 0 ; w_i < k_AMNT_EST_VAR ; w_i++ )
  {
    /* stdDev variables */
    aw_amnt[w_i]          = 0;
    aw_pos[w_i]           = 0;
    aw_oldPos[w_i]        = 0;
    addw_sum_powedxi[w_i] = (UINT64)0;
    all_sum[w_i]          = (INT64)0;
    /* reset remembered delay values */
    as_oldDelVal[w_i].ll_scld_Nsec = (INT64)0;
    /* reset successive cut counters */
    ab_succCuts[w_i] = 0;
    /* reset number of samples */
    adw_samples[w_i] = 0;    
  }
  /* reset value of last MTSD to zero */
  ll_lastMtsd = 0;
  ll_owd      = 0;
}

/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
**  
** Function    : Estimator
**  
** Description : The estimator filters the raw input values.
**  
** Parameters  : ll_inp  (IN)  - unfiltered input value
**               b_which (IN)  - MTSD or STMD
**               pl_outp (OUT) - estimated output value
**                
** Returnvalue : k_RUNNING     - filter is operational   
**               k_HOLDOVER    - filter is in HOLDOVER mode
** 
** Remarks     : -
**  
***********************************************************************/
static UINT8 Estimator( INT64 ll_inp,UINT8 b_which,INT64* pl_outp )
{
  INT64   ll_stdDevInp;
  BOOLEAN o_ret;
  INT32   al_mean[k_AMNT_EST_VAR];
  UINT32  adw_stdDev[k_AMNT_EST_VAR];
  UINT8   b_mode = k_HOLDOVER;

  /* get last measurement value */
  ll_stdDevInp = PTP_INTV_TO_NSEC(as_oldDelVal[b_which].ll_scld_Nsec);
  /* cut value to +/- 1 sec */
  if( ll_stdDevInp > (INT64)k_NSEC_IN_SEC )
  {
    ll_stdDevInp = (INT64)k_NSEC_IN_SEC;
  }
  if( ll_stdDevInp < -(INT64)k_NSEC_IN_SEC )
  {
    ll_stdDevInp = -(INT64)k_NSEC_IN_SEC;
  }

  /* get standard deviation on nsec format */
  o_ret = PTP_stdDev((INT32)ll_stdDevInp,&al_mean[b_which],&adw_stdDev[b_which],
                     FALSE,dw_histLen,&aw_amnt[b_which],&aw_pos[b_which],
                     &aw_oldPos[b_which],apl_HistNsec[b_which],
                     &addw_sum_powedxi[b_which],&all_sum[b_which]);
  /* increment number of samples */
  if( (adw_samples[b_which] <= dw_histLen ) || ( o_ret == FALSE ))
  {
    adw_samples[b_which]++;
  }
  /* criterion for enabling the estimation filter */
  if( ( adw_samples[b_which] >= dw_histLen) &&
    ( adw_stdDev[b_which] < ddw_whtNoise))
  {
    /* deviates value too much ? */
    if( (UINT64)PTP_ABS(INT64,(PTP_INTV_TO_NSEC(ll_inp) - al_mean[b_which])) < 
         ddw_whtNoise )
    {
      /* reset successive cuts count */
      ab_succCuts[b_which] = 0; 
      /* filter mode is RUNNING */
      b_mode = k_RUNNING;
    }
    else
    {
      /* not too much succeeding cutoff values ? */
      if( ab_succCuts[b_which] < dw_amntFlt )
      {
        /* take old value */
        ll_inp = PTP_NSEC_TO_INTV(al_mean[b_which]);
        /* increment successive cuts */
        ab_succCuts[b_which]++; 
        /* filter mode is RUNNING */
        b_mode = k_RUNNING;
      }
      else
      {
        /* reset filter to mode HOLDOVER */
        b_mode = k_HOLDOVER;       
      }
    }
  }
  else
  {
    /* filter mode is HOLDOVER */
    b_mode = k_HOLDOVER;
    /* reset successive cuts count */
    ab_succCuts[b_which] = 0;
    /* use measured value */ 
  }
  /* remember old value */
  as_oldDelVal[b_which].ll_scld_Nsec = ll_inp;
  /* give filtered delay value back */
  *pl_outp = as_oldDelVal[b_which].ll_scld_Nsec;
  return b_mode;
}
