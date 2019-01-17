/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: PTPmath.c  
**    Summary: This unit provides reentrant mathematical routines bases
**             on integer mathematics.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
** 
**  Functions: PTP_AddTmSTmpTmStmp
**             PTP_SubtrTmStmpTmStmp
**             PTP_SubtrTmIntvTmStmp
**             PTP_AddTmIntvTmStmp
**             PTP_sqrt_U64
**             PTP_LowPass
**             PTP_PIctr
**             PTP_stdDev
**             PTP_getTimeIntv
**             PTP_logDualis
**             PTP_scaleVar
**             PTP_GetAllenVar
**             PTP_PowerOf2
**             PTP_CompareClkId
**             PTP_ComparePortId
**             PTP_CompPortAddr
**             PTP_AddnCheckI64
**             PTP_ChkRngSetExtr
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
#include "PTP/PTPint.h"

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/

/*************************************************************************
**    global functions
*************************************************************************/

/*************************************************************************
**
** Function    : PTP_AddTmSTmpTmStmp
**
** Description : This function generates the sum of two timestamps. 
**               The result is a timestamp.
**
** See Also    : PTP_SubtrTmStmpTmStmp(),PTP_SubtrTmIntvTmStmp(), 
**               PTP_AddTmIntvTmStmp()
**              
** Parameters  : ps_ts_A     (IN)  - pointer to first operand/timestamp
**               ps_ts_B     (IN)  - pointer to second operand/timestamp
**               ps_tsRes   (OUT) - pointer to resulting timestamp
**
** Returnvalue : TRUE  - function succeeded
**               FALSE - function succeeded, but there is a second overflow
**                       in the resulting timestamp.
**
** Remarks     : Function is reentrant / is unit-tested
**
*************************************************************************/
BOOLEAN PTP_AddTmSTmpTmStmp(PTP_t_TmStmp const *ps_ts_A,
                            PTP_t_TmStmp const *ps_ts_B,
                            PTP_t_TmStmp *ps_tsRes)
{
  BOOLEAN o_ret = TRUE;
  ps_tsRes->u48_sec = ps_ts_A->u48_sec + ps_ts_B->u48_sec;
  ps_tsRes->dw_Nsec = ps_ts_A->dw_Nsec + ps_ts_B->dw_Nsec;
  /* nanosecond overflow ? */
  if( ps_tsRes->dw_Nsec > k_MAX_NSEC )
  {
    /* increment seconds */
    ps_tsRes->u48_sec++;
    /* subtract 1 second of the nanoseconds member */
    ps_tsRes->dw_Nsec -= k_NSEC_IN_SEC;
  }
  /* seconds overflow ? */
  if( ps_tsRes->u48_sec & k_U48_OVRFLW_MSK )
  {
    o_ret = FALSE;
  }
  return o_ret;
}

/*************************************************************************
**
** Function    : PTP_SubtrTmStmpTmStmp
**
** Description : This function subtracts two timestamps. The result 
**               is the difference of the time-stamps (A minus B) and
**               is contained in the data type {PTP_t_TmIntv}.
**
** See Also    : PTP_SubtrTmIntvTmStmp(), 
**               PTP_AddTmIntvTmStmp(),PTP_AddTmSTmpTmStmp()
**              
** Parameters  : ps_ts_A     (IN)  - pointer to first operand/timestamp
**               ps_ts_B     (IN)  - pointer to second operand/timestamp
**               ps_ti_Res   (OUT) - pointer to resulting time interval
**
** Returnvalue : TRUE  - function succeeded/time interval is scaled
**               FALSE - function failed/ time interval is unscaled
**
** Remarks     : Function is reentrant / is unit-tested
**
*************************************************************************/
BOOLEAN PTP_SubtrTmStmpTmStmp(PTP_t_TmStmp const *ps_ts_A,
                              PTP_t_TmStmp const *ps_ts_B,
                              PTP_t_TmIntv   *ps_ti_Res)
{
  INT64   ll_nsecRes;
  BOOLEAN o_res;
  /* get nanoseconds of A */
  ll_nsecRes = (ps_ts_A->u48_sec * k_NSEC_IN_SEC) + 
               ps_ts_A->dw_Nsec;/*lint !e713*/
  /* subtract nanoseconds of B */
  ll_nsecRes -= ((ps_ts_B->u48_sec * k_NSEC_IN_SEC) 
                 + ps_ts_B->dw_Nsec);/*lint !e713 !e737*/
  /* check, if result can be scaled */
  if( PTP_ABS(INT64,ll_nsecRes) & k_I48_OVRFLW_MSK ) /*lint !e737*/
  {
    /* return result unscaled */
    ps_ti_Res->ll_scld_Nsec = ll_nsecRes;
    o_res = FALSE;
  }
  else
  {
    /* scale nanoseconds */
    ps_ti_Res->ll_scld_Nsec = PTP_NSEC_TO_INTV(ll_nsecRes);
    o_res = TRUE;
  }
  return o_res;
}

/*************************************************************************
**
** Function    : PTP_SubtrTmIntvTmStmp
**
** Description : This function subtracts a time interval from a 
**               timestamp. The result is a timestamp.
**
** See Also    : PTP_SubtrTmStmpTmStmp(), 
**               PTP_AddTmIntvTmStmp(),PTP_AddTmSTmpTmStmp()
**              
** Parameters  : ps_ts_A     (IN)  - pointer to first operand/timestamp
**               ps_ti_B     (IN)  - pointer to second operand/time interval
**               ps_ts_Res   (OUT) - pointer to resulting /timestamp
**
** Returnvalue : TRUE              - function succeeded
**               FALSE             - function failed because of 
**                                   negative resulting timestamp or 
**                                   exceeding timestamp
**
** Remarks     : Function is reentrant / is unit-tested
**
*************************************************************************/
BOOLEAN PTP_SubtrTmIntvTmStmp(PTP_t_TmStmp const *ps_ts_A,
                              PTP_t_TmIntv const *ps_ti_B,
                              PTP_t_TmStmp *ps_ts_Res)
{
  INT64   ll_tmeIntv;
  INT32   l_nsec;
  INT64   ll_sec;
  INT64   ll_resNsec;
  INT64   ll_resSec;
  BOOLEAN o_res = TRUE;

  /* get time interval in nanoseconds */
  ll_tmeIntv = PTP_INTV_TO_NSEC(ps_ti_B->ll_scld_Nsec);
  ll_sec = ll_tmeIntv / (INT64)k_NSEC_IN_SEC;
  l_nsec = (INT32)(ll_tmeIntv % k_NSEC_IN_SEC);
  /* get nanoseconds result */
  ll_resNsec = (INT64)ps_ts_A->dw_Nsec - (INT64)l_nsec;
  /* positive overflow ? */
  if( ll_resNsec >= k_NSEC_IN_SEC )
  {
    ll_resSec          = (ps_ts_A->u48_sec - ll_sec) + 1; /*lint !e737 !e713*/
    ps_ts_Res->dw_Nsec = (UINT32)(ll_resNsec - k_NSEC_IN_SEC);
  }
  /* negative overflow ? */
  else if( ll_resNsec < 0 )
  {
    ll_resSec          = (ps_ts_A->u48_sec - ll_sec) - 1;/*lint !e737 !e713*/
    ps_ts_Res->dw_Nsec = (UINT32)(ll_resNsec + k_NSEC_IN_SEC);
  }
  else
  {
    ll_resSec          = ps_ts_A->u48_sec - ll_sec;/*lint !e737 !e713*/
    ps_ts_Res->dw_Nsec = (UINT32)ll_resNsec;
  }  
  /* check result */
  if( ll_resSec & k_U48_OVRFLW_MSK )/*lint !e737*/
  {
    /* resulting seconds are negative or bigger than 48 bit */
    o_res = FALSE;

    /* set resulting timestamp to minimal value if negative result */
    if( ll_resSec & k_MAX_NEG_I48 )/*lint !e737*/
    {
      ps_ts_Res->u48_sec = 0;
      ps_ts_Res->dw_Nsec = 0;
    }
    else
    {
      /* let resulting timestamp as is and return FALSE */    
      ps_ts_Res->u48_sec = (UINT64)ll_resSec;
    }
  }
  else
  {
    ps_ts_Res->u48_sec = (UINT64)ll_resSec;
  }
  return o_res;
}

/*************************************************************************
**
** Function    : PTP_AddTmIntvTmStmp
**
** Description : This function adds a time interval to a 
**               timestamp. The result is a timestamp.
**
** See Also    : PTP_SubtrTmStmpTmStmp(),PTP_SubtrTmIntvTmStmp(), 
**               PTP_AddTmSTmpTmStmp()
**              
** Parameters  : ps_ts_A     (IN)  - pointer to first operand/timestamp
**               ps_ti_B     (IN)  - pointer to second operand/time interval
**               ps_ts_Res   (OUT) - pointer to resulting /timestamp
**
** Returnvalue : TRUE              - function succeeded
**               FALSE             - function failed because of 
**                                   negative resulting timestamp or 
**                                   exceeding timestamp
**
** Remarks     : Function is reentrant / is unit-tested
**
*************************************************************************/
BOOLEAN PTP_AddTmIntvTmStmp(PTP_t_TmStmp const *ps_ts_A,
                            PTP_t_TmIntv const *ps_ti_B,
                            PTP_t_TmStmp *ps_ts_Res)
{
  PTP_t_TmIntv s_ti_B;
  /* initialize time interval as negative to invert operation */
  s_ti_B.ll_scld_Nsec = -ps_ti_B->ll_scld_Nsec;
  /* care for negative maximum */
  if( ps_ti_B->ll_scld_Nsec == k_MAX_NEG_I48 )/*lint !e737 !e650*/
  {
    s_ti_B.ll_scld_Nsec = -ps_ti_B->ll_scld_Nsec-1;
  }
  /* call subtracting function */
  return PTP_SubtrTmIntvTmStmp(ps_ts_A,&s_ti_B,ps_ts_Res);
}

/***********************************************************************
**
** Function    : PTP_sqrt_U64
**
** Description : Calculates the square-root of an 64 bit unsigned integer 
**               and returns floor(result)
**              
** Parameters  : ddw_inp (IN) - variable to calculate the square root of it
**
** Returnvalue : UINT16       - result of sqrt_int(dw_val)
**                      
** Remarks     : function is reentrant / unit-tested
**
***********************************************************************/
UINT32 PTP_sqrt_U64(UINT64 ddw_inp)
{
  UINT32 dw_Root = 0;
  UINT64 ddw_RemHi = 0;
  UINT64 ddw_RemLo = ddw_inp;
  UINT64 ddw_TestDiv;
  UINT8  b_Bits;

  for( b_Bits = 0 ; b_Bits < 32; b_Bits++)
  {
    ddw_RemHi   = (ddw_RemHi << 2) | (ddw_RemLo >> 62);
    ddw_RemLo   <<= 2;
    dw_Root     <<= 1;
    ddw_TestDiv = ((UINT64)dw_Root << 1) + 1;

    if(ddw_RemHi >= ddw_TestDiv)
    {
      ddw_RemHi -= ddw_TestDiv;
      dw_Root++;
    }
  }
  return dw_Root;
}

/*************************************************************************
**
** Function    : PTP_LowPass
**
** Description : Filters the input with a low-pass with time-constant T
**              
** Parameters  : ll_actVal (IN)     - actual drift correction
**               pll_sum   (IN(OUT) - internal sum value. Must be static 
**                                    defined in the calling function.
**               i_T       (IN)     - time constant T that characterizes 
**                                    the lowpass
**
** Returnvalue : INT64              - filtered value
**                      
** Remarks     : function is reentrant / unit-tested
**
***********************************************************************/
INT64 PTP_LowPass(const INT64 ll_actVal,INT64 *pll_sum,INT16 i_T)
{
  INT64 ll_ret;
  
  *pll_sum = *pll_sum + ll_actVal;
  ll_ret = *pll_sum / (INT64)i_T;
  *pll_sum = *pll_sum - ll_ret;
  return ll_ret;
}

/***********************************************************************
**  
** Function    : PTP_PIctr
**  
** Description : This function implements the PI controller. With resetting
**               the controller, it is parametrized.
**               Formula is :            __ n  
**                                       \
**               u(k) = Kp * e(k) + Ki * / e(i)
**                                       -- i = 0
**
**               with:
**               Kp = P / Ta [sec] ; P = 0.7
**               Ki = I / Ta [sec] ; I = 0.3
**
**               This PI controller can be preloaded. This makes sense 
**               in all device configurations, which will end up in the
**               same stationary status as in the last run. For that, the
**               PI-controller shall be preloaded with the mean value of
**               the last run.
**               Remark: This PI controller uses a forward-approximation.
**               Thus, the i-value is added to the first iteration of the
**               function.
**  
** Parameters  : ll_ek        (IN) - input value
**               o_rst         (IN) - flag to reset PI controller
**               dw_TaUsec     (IN) - Ta in microseconds 
**               ll_preLoadSum (IN) - preload value for the internal sum
**               
** Returnvalue : INT64 - output of controller
**
** Remarks     : Function is not reentrant / unit-tested
**  
***********************************************************************/
INT64 PTP_PIctr(INT64 ll_ek,BOOLEAN o_rst,UINT32 dw_TaUsec,INT64 ll_preLoadSum)
{
  static INT64 ll_sum = 0;
  /* P = 0.7 I = 0.3 */
  const  INT64 ll_P = 7; /* divided by 10 in function to get 0.7 */
  const  INT64 ll_I = 3; /* divided by 10 in function to get 0.3 */
  
  INT64 ll_uk;
  INT64 ll_Ta; 

  
  if( o_rst == TRUE )
  {
    /* reset sum of the I-controller */
    ll_sum = ll_preLoadSum;
    /* return e(k) */
    ll_uk = ll_ek;    
  }
  else
  {
    ll_Ta = dw_TaUsec / (INT64)100;
    /* term 1: u(k) = Kp * e(k) 
                     
                    = (P / Ta [sec]) * e(k)
                    = (((10P/10)*1e6)/Ta[usec]) * e(k)
                    with P = 0.7
                    = ((7 * 1e5)/Ta[usec]) * e(k)       
                    = ((7 * 1e3)/(Ta[usec]/100)) * e(k) */
    ll_uk  = (ll_ek * ll_P * (INT64)1000) / ll_Ta;
    /* sum up for integrator */
    /* term 2: u(k) = Ki * sum(e(i)) 
                      
                    = (I / Ta [sec]) * sum(e(i))
                    = (((10I/10)*1e6)/Ta[usec]) * sum(e(i))
                    with I = 0.3
                    = ((3 * 1e5)/Ta[usec] * sum(e(i))
                    = ((3 * 1e3)/(Ta[usec]/100)) * sum(e(i))  */
    ll_sum = PTP_AddnCheckI64(ll_sum,((ll_ek * ll_I * (INT64)1000) / ll_Ta));
    /* add I-value to term */
    ll_uk  = PTP_AddnCheckI64(ll_uk,ll_sum);   
  } 
  /* return result */
  return ll_uk;
}

/*************************************************************************
**
** Function    : PTP_stdDev
**
** Description : Calculates the standard deviation and the mean value
**               for the last inserted n (=AMNT_HIST) values.
**               It uses the following formula:
**                          ------------------------
**                          |         2            2 
**               StdDev =   | n* SUM(x ) - (SUM(x))
**                        \ | ----------------------
**                         \|        n * (n-1)
**
**
**                         SUM(x)
**               Mean   =  ------
**                           n
**               !!!ATTENTION!!!
**               The function is unit-tested for inputs up to 
**               +/-2.000.000.000 ,array length of 30 and a variable that is
**               normally distributed with a mean-value of ~0
**              
** Parameters  : l_x              (IN)     - input value
**               pl_mean          (OUT)    - result mean value 
**               pdw_stdDev       (OUT)    - result standard deviation 
**               o_reset          (IN)     - reset flag to reset the function
**               w_arrSze         (IN)     - array size ( history size )
**               pw_amnt          (IN/OUT) - Internal used variable.
**                                           Variable must be defined
**                                           static in the calling function 
**                                           and shall not be changed outside.
**               pw_pos           (IN/OUT) - Internal used variable.
**                                           Variable must be defined
**                                           static in the calling function 
**                                           and shall not be changed outside.
**               pw_oldPos        (IN/OUT) - Internal used variable.
**                                           Variable must be defined
**                                           static in the calling function 
**                                           and shall not be changed outside.
**               pl_xi            (IN/OUT) - Internal used variable.
**                                           Variable must be defined
**                                           static in the calling function 
**                                           and shall not be changed outside.
**               pddw_sum_powedxi (IN/OUT) - Internal used variable.
**                                           Variable must be defined
**                                           static in the calling function 
**                                           and shall not be changed outside.
**               pll_sum_xi       (IN/OUT) - Internal used variable.
**                                           Variable must be defined
**                                           static in the calling function 
**                                           and shall not be changed outside.
**
** Returnvalue : TRUE  - returned values are correct
**               FALSE - returned values cannot be used, function is in 
**                       initializing state - more values are needed
**                      
** Remarks     : function is reentrant / unit-tested
**
***********************************************************************/
BOOLEAN PTP_stdDev( const INT32  l_x,
                    INT32        *pl_mean,
                    UINT32       *pdw_stdDev,
                    BOOLEAN      o_reset,
                    const UINT16 w_arrSze,
                    UINT16       *pw_amnt,
                    UINT16       *pw_pos,
                    UINT16       *pw_oldPos,
                    INT32        *pl_xi,
                    UINT64       *pddw_sum_powedxi,
                    INT64        *pll_sum_xi)
{
  UINT64 ddw_powedsum_xi = 0;
  UINT64 ddw_sum_under_root;
  INT64  ll_xi;
  BOOLEAN o_ret = FALSE;

  /* calculate or reset ? */
  if( o_reset == TRUE )
  {
    /* reset values  */
    (*pw_amnt)   = 0;  
    (*pw_pos)    = 0;  
    (*pw_oldPos) = 0;
    (*pddw_sum_powedxi) = 0;
    (*pll_sum_xi)      = 0;
    /* history array must not be cleared - it will be overwritten */

    /* return FALSE for not calculated values */
    o_ret    = FALSE;     
  }
  else
  {   
    /* insert new value in history ring buffers */
    pl_xi[(*pw_pos)]     = l_x;
    ll_xi                = pl_xi[(*pw_pos)];
    /* add new values to variables */
    (*pddw_sum_powedxi) += (UINT64)((ll_xi * ll_xi)/(w_arrSze-1));
    (*pll_sum_xi)       += l_x;    
    
    /* is array full ? */
    if( (*pw_amnt) < w_arrSze)
    {
      /* increment */
      (*pw_amnt)++;
      /* return FALSE for not calculated values */
      o_ret = FALSE;
    }
    else
    {
      /* subtract old values of variables */
      ll_xi           = pl_xi[(*pw_oldPos)];
      /* add new values to variables */
      (*pddw_sum_powedxi) -= (UINT64)((ll_xi * ll_xi)/(INT32)(w_arrSze-1));
      (*pll_sum_xi)       -= ll_xi;
      /* calculate sum of xi to the power of two */
      ddw_powedsum_xi  = (UINT64)(((*pll_sum_xi)/(INT32)w_arrSze) * 
                                  ((*pll_sum_xi)/(INT32)(w_arrSze-1)));

      /* get mean value */
      (*pl_mean) = (INT32)((*pll_sum_xi) / (INT32)w_arrSze);
     
      /* get sum under root */
      ddw_sum_under_root = (*pddw_sum_powedxi) - ddw_powedsum_xi;
      /* get standard deviation */
      *pdw_stdDev = PTP_sqrt_U64(ddw_sum_under_root);       
      /* get next old position in array */
      (*pw_oldPos)++;   
      (*pw_oldPos) %= (w_arrSze+1);
      
      /* return TRUE - values are calculated */
      o_ret = TRUE;
    }
    /* get next position in array */
    (*pw_pos)++;
    (*pw_pos) %= (w_arrSze+1); 
  }
  /* return filtered value */
  return o_ret;
}

/***********************************************************************
**
** Function    : PTP_getTimeIntv
**
** Description : Calculates the power of 2 to the given integer exponent.
**               This function is just used for calculating time intervals
**               for internal timeouts.
**               Therefore, the result is scaled to SIS timer counts.
**              
** Parameters  : c_expon (IN) - exponent
**
** Returnvalue : UINT32       - time interval in SIS timer ticks
**                      
** Remarks     : function is reentrant / unit-tested
**
***********************************************************************/
UINT32 PTP_getTimeIntv(INT8 c_expon)
{
  UINT32 dw_usec = k_USEC_IN_SEC;
  UINT32 dw_ret = 0;

  /* maximum interval = 2048 sek (2^11) */
  if( c_expon > k_MAX_LOG_MSG_INTV )
  {
    c_expon = k_MAX_LOG_MSG_INTV;
    PTP_SetError(k_PTP_ERR_ID,PTP_k_ERR_INTV,e_SEVC_ERR);
  }
  /* minimum interval = 488 usec (2^-11) */
  if( c_expon < k_MIN_LOG_MSG_INTV )
  {
    c_expon = k_MIN_LOG_MSG_INTV;
    PTP_SetError(k_PTP_ERR_ID,PTP_k_ERR_INTV,e_SEVC_ERR);
  }
  /* shift according to exponent of interval */
  if( c_expon > 0 )
  {    
    dw_usec = dw_usec << c_expon;
  }
  else
  {
    dw_usec = dw_usec >> (-c_expon);/*lint !e504*/
  }  
  /* get return value in SIS ticks */
  dw_ret = (dw_usec+(k_PTP_TIME_RES_USEC/2))/k_PTP_TIME_RES_USEC;
  if( dw_ret == 0 )
  {
    dw_ret++;
    PTP_SetError(k_PTP_ERR_ID,PTP_k_ERR_INTV,e_SEVC_ERR);
  }
  return dw_ret;
}


/***********************************************************************
**
** Function    : PTP_logDualis
**
** Description : Calculates the log2 
**              
** Parameters  : ddw_val   (IN) - variable to calculate the log2 of it
**
** Returnvalue : INT16        - result of log2(l_val)
**                      
** Remarks     : function is reentrant / unit-tested
**
***********************************************************************/
UINT16 PTP_logDualis(UINT64 ddw_val)
{
  UINT16 i;
  
  ddw_val /= (INT64)2LL;
  for(i = 0; ddw_val > 0; ++i)
  {
    ddw_val /= (INT64)2LL;
  }
  return i;
}

/***********************************************************************
**  
** Function    : PTP_scaleVar
**  
** Description : Computes the offsetScaledLogVariance and ParentOffset
**               ScaledLogVariance parameters.
**               The algorithm to scale the variance is described by 
**               the standard in chapter 7.6.3.3
**  
** Parameters  : ddw_var (IN) - variance input in nanoseconds to the power of 2
**               
** Returnvalue : scaled value
** 
** Remarks     : -
**  
***********************************************************************/
UINT16 PTP_scaleVar( UINT64 ddw_var )
{
  INT16  i_tmp;
  UINT16 w_res;
 
  /* get logarithm on basis of seconds */
  /* => ld(var) = ld(x nsec^2) */ 
  /* => ld(var) = ld(x sec^2 * 1e-18) */
  /* => ld(var) = ld(x) + ld(1e-18) */
  /* with ld(1e-18)  = -60 */
  /* => ld(interval) = ld(x) - 60 */
  i_tmp = (INT16)(PTP_logDualis(ddw_var) - 60);

  /* scaling is done by multiplying result with 0x100 and adding 0x8000 */
  w_res = (UINT16)((i_tmp * 0x100) + 0x8000);
  /* return result */
  return w_res;
}

/***********************************************************************
**
** Function    : PTP_GetAllenVar
**
** Description : Calculates the PTP variance. The computation is derived
**               from the allan deviation formula. 
**               The method is described in chapter 7.6.3.2 of the 
**               standard.
**               The returned PTP variance is unscaled.
**              
** Parameters  : l_actOffs   (IN) - Actual offset in nsec
**               o_reset     (IN) - flag determines, if statistics are 
**                                  reseted or updated
**
** Returnvalue : unscaled allen variance
**                      
** Remarks     : function is reentrant / unit-tested
**
***********************************************************************/
UINT64 PTP_GetAllenVar(INT32 l_actOffs,BOOLEAN o_reset)
{
  static UINT64 ddw_allanVar = 0;
  static UINT16 w_amnt = 0;
  static INT32  l_pos = 0;
  /* variables for allen variance */
  static INT64  all_sqrs[k_N_PTPVAR] = {(INT64)0}; 
  static INT32  l_off2 = 0,l_off1 = 0;
  static const  INT64 ll_divisor = 6 * (k_N_PTPVAR-2);   

  /* calculate or reset ? */
  if( o_reset == TRUE )
  {
    /* reset values for allan vaiance */
    ddw_allanVar = 0;
    w_amnt       = 0;
    l_pos        = 0;
    l_off1       = 0;
    l_off2       = 0; 
    PTP_MEMSET(all_sqrs,(INT32)0,sizeof(INT64)*k_N_PTPVAR);  
  }
  else
  {
    /* fill history for allan variance */
    all_sqrs[l_pos]  = (INT64)l_off2;
    /* compute new value */
    all_sqrs[l_pos] -= ((INT64)2 * (INT64)l_off1);
    all_sqrs[l_pos] += (INT64)l_actOffs;
    all_sqrs[l_pos]  = all_sqrs[l_pos] * all_sqrs[l_pos];
    all_sqrs[l_pos]  = all_sqrs[l_pos] / ll_divisor; 
    if( w_amnt > 1 )
    {      
      /* add new value */
      ddw_allanVar += (UINT64)all_sqrs[l_pos];            
    }
    /* is array full ? */
    if( w_amnt < k_N_PTPVAR)
    {
      /* increment */
      w_amnt++;
    }
    else
    {
      /* subtract old value */
      ddw_allanVar -= (UINT64)all_sqrs[(l_pos+2)%k_N_PTPVAR];
    }
    /* decrement position in array */
    l_pos = (l_pos+1) % k_N_PTPVAR;
    /* remember the last two passed values */
    l_off2 = l_off1;
    l_off1 = l_actOffs;      
  }
  /* scale the result */
  return ddw_allanVar;
}
 

/***********************************************************************
**  
** Function    : PTP_PowerOf2
**  
** Description : Calculates 2 to the power of exponent
**  
** Parameters  : b_exp (IN) - exponent
**               
** Returnvalue : result
** 
** Remarks     : unit-tested
**  
***********************************************************************/
INT64 PTP_PowerOf2(UINT8 b_exp)
{
  UINT16 i;
  INT64  ll_ret;
  
  /*  get unscaled value */
  ll_ret = (INT64)1LL;
       
  /* pow */
  for( i = 0 ; i < b_exp ; i++ )
  {
    ll_ret *= (INT64)2LL;
  }
  return ll_ret;
}

/*************************************************************************
**
** Function    : PTP_CompareClkId
**
** Description : Compares two clock IDs
**
** Parameters  : ps_clkId_A (IN)  - first clock Id
**               ps_clkId_B (IN)  - second clock ID
**
** Returnvalue : INT8:
**               PTP_k_LESS          - PORTID_A < PORTID_B
**               PTP_k_SAME          - PORTID_A = PORTID_B
**               PTP_k_GREATER       - PORTID_A > PORTID_B
**               
** Remarks     : unit-tested
**
*************************************************************************/
INT8 PTP_CompareClkId(const PTP_t_ClkId *ps_clkId_A,
                      const PTP_t_ClkId *ps_clkId_B)
{  
  INT32 l_ret;

  /* compare clock id */
  l_ret = PTP_BCMP(ps_clkId_A->ab_id,ps_clkId_B->ab_id,k_CLKID_LEN);
  if(  !PTP_BCMP_A_SAME_B(l_ret) )
  {
    if( PTP_BCMP_A_GREATR_B(l_ret) )
    {
      return PTP_k_GREATER;
    }
    else
    {
      return PTP_k_LESS;
    }
  }  
  /* they are identical */
  return PTP_k_SAME;
}

/*************************************************************************
**
** Function    : PTP_ComparePortId
**
** Description : Compares two port IDs
**
** Parameters  : ps_portId_A (IN)  - pointer to the first port Id
**               ps_portId_B (IN)  - pointer to the second port ID
**
** Returnvalue : INT8:
**               PTP_k_LESS          - PORTID_A < PORTID_B
**               PTP_k_SAME          - PORTID_A = PORTID_B
**               PTP_k_GREATER       - PORTID_A > PORTID_B
**               
** Remarks     : unit-tested
**
*************************************************************************/
INT8 PTP_ComparePortId(const PTP_t_PortId *ps_portId_A,
                       const PTP_t_PortId *ps_portId_B)
{ 
  INT8 c_cmp;
  
  c_cmp = PTP_CompareClkId(&ps_portId_A->s_clkId,&ps_portId_B->s_clkId);
  if(c_cmp == PTP_k_SAME )
  {
    /* compare source port id */
    if(ps_portId_A->w_portNmb != ps_portId_B->w_portNmb )
    {
      if(ps_portId_A->w_portNmb > ps_portId_B->w_portNmb )
      {
        c_cmp = PTP_k_GREATER;
      }
      else
      {
        c_cmp = PTP_k_LESS;
      }
    }
    else
    {
      c_cmp = PTP_k_SAME;
    }
  }
  return c_cmp; 
}

/*************************************************************************
**
** Function    : PTP_CompPortAddr
**
** Description : Compares two port addresses.
**
** Parameters  : ps_portAddr_A (IN)  - pointer to the first port address
**               ps_portAddr_B (IN)  - pointer to the second port address
**
** Returnvalue : INT8:
**               PTP_k_LESS          - PORTADDR_A < PORTADDR_B
**               PTP_k_SAME          - PORTADDR_A = PORTADDR_B
**               PTP_k_GREATER       - PORTADDR_A > PORTADDR_B
**               
** Remarks     : unit-tested
**
*************************************************************************/
INT8 PTP_CompPortAddr(const PTP_t_PortAddr *ps_portAddr_A,
                      const PTP_t_PortAddr *ps_portAddr_B)
{
  INT32 l_ret;
  INT8 c_ret;
  /* compare network protocol */
  if( ps_portAddr_A->e_netwProt != ps_portAddr_B->e_netwProt )
  {
    if( ps_portAddr_A->e_netwProt < ps_portAddr_B->e_netwProt )
    {
      c_ret = PTP_k_LESS;
    }
    else
    {
      c_ret = PTP_k_GREATER;
    }
  }
  else
  {
    if( ps_portAddr_A->w_AddrLen != ps_portAddr_B->w_AddrLen )
    {
      if( ps_portAddr_A->w_AddrLen < ps_portAddr_B->w_AddrLen )
      {
        c_ret = PTP_k_LESS;
      }
      else
      {
        c_ret = PTP_k_GREATER;
      }
    }
    else
    {
      /* compare clock id */
      l_ret = PTP_BCMP(ps_portAddr_A->ab_Addr,
                       ps_portAddr_B->ab_Addr,
                       ps_portAddr_A->w_AddrLen);
      if(  !PTP_BCMP_A_SAME_B(l_ret) )
      {
        if( PTP_BCMP_A_GREATR_B(l_ret) )
        {
          c_ret = PTP_k_GREATER;
        }
        else
        {
          c_ret = PTP_k_LESS;
        }
      }
      else
      {
        c_ret = PTP_k_SAME;
      }
    }
  }
  return c_ret;
}

/***********************************************************************
**  
** Function    : PTP_AddnCheckI64
**  
** Description : Adds two INT64 variables and checks for overflow.
**               If overflow occurs, function gives biggest possible 
**               value (negative or positive) back and throws a notification
**               error
**  
** Parameters  : ll_var (IN) - Variable 1
**               ll_add (IN) - Variable 2 to add to variable 1
**               
** Returnvalue : INT64 - result
** 
** Remarks     : unit-tested
**  
***********************************************************************/
INT64 PTP_AddnCheckI64(INT64 ll_var, INT64 ll_add)
{
  INT64 ll_res = ll_var + ll_add;
  
  /* start with negative overflow - two negatives added are negative as well */
  if((ll_add < (INT64)0LL) && (ll_var < (INT64)0LL)) 
  {
    /* check if also negative */
    if( ll_res > (INT64)0LL )
    {
      ll_res = (INT64)k_MIN_I64;
      PTP_SetError(k_PTP_ERR_ID,PTP_k_ERR_CLIP_I64,e_SEVC_NOTC);
    }
  }
  /* positive overflow - two positives added are positive as well */
  else if((ll_add > (INT64)0LL) && (ll_var > (INT64)0LL)) 
  {
    /* check if also positive */
    if( ll_res < (INT64)0LL )
    {
      ll_res = k_MAX_I64;
      PTP_SetError(k_PTP_ERR_ID,PTP_k_ERR_CLIP_I64,e_SEVC_NOTC);
    }
  }
  else
  {
    /* a positive variable added to a negative or vice versa 
      do never generate a overflow */
  }
  return ll_res;
}

/***********************************************************************
**  
** Function    : PTP_ChkRngSetExtr
**  
** Description : Checks a value agains the range. If a value extends 
**               the range, its value is set to the appropriate
**               extreme value.
**  
** Parameters  : l_val (IN) - input value to check
**               l_min (IN) - minimum value of range
**               l_max (IN) - maximum value of range
**               
** Returnvalue : INT32 - returned value
**
** Remarks     : -
**  
***********************************************************************/
INT32 PTP_ChkRngSetExtr(INT32 l_val,INT32 l_min,INT32 l_max)
{
  INT32 l_ret;
  /* value to set smaller than minimum ? */
  if( l_val < l_min )
  {
    /* set value to minimum value */
    l_ret = l_min;
  }
  /* value to set bigger than maximum ? */
  else if( l_val > l_max )
  {
    /* set value to maximum value */
    l_ret = l_max;
  }
  else
  {
    /* return input value if it does not extend the range */
    l_ret = l_val;
  }
  return l_ret;
}


/*************************************************************************
**    static functions
*************************************************************************/
