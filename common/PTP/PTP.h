/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: PTP.h  
**    Summary: PTP - PTP main unit
**             The unit PTP is the main interface of the IEEE 1588
**             protocol software towards the application and therefore
**             the super unit for binding the IEEE 1588 engine. The
**             application can access the functions for initializing
**             and de-initializing the PTP engine and to get and set
**             the system time by the application. Furthermore some
**             mathematical functions are provided for simple
**             calculations e.g. with timestamps and time intervals.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: PTP_Init
**             PTP_Main
**             PTP_Close
**             PTP_ReadClkDs
**             PTP_ChgFltCfg
**             PTP_GetSysTime
**             PTP_SetSysTime
**             PTP_SetSysTimeOffset
**             PTP_SetSysTimeOffsSec
**             PTP_AddTmSTmpTmStmp
**             PTP_SubtrTmStmpTmStmp
**             PTP_SubtrTmIntvTmStmp
**             PTP_AddTmIntvTmStmp
**             PTP_ComparePortId
**             PTP_CompPortAddr
**             PTP_CompareClkId
**             PTP_sqrt_U64
**             PTP_LowPass
**             PTP_PIctr
**             PTP_stdDev
**             PTP_getTimeIntv
**             PTP_logDualis
**             PTP_scaleVar
**             PTP_GetAllenVar
**             PTP_PowerOf2
**             PTP_AddnCheckI64
**             PTP_ChkRngSetExtr
**             PTP_SetError
**             PTP_GetErrStr
**             
**   Compiler: Ansi-C
**    Remarks: 
**
**************************************************************************
**    all rights reserved
*************************************************************************/
#ifndef __PTP_H__
#define __PTP_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/

/*************************************************************************
**    data types
*************************************************************************/

/* application error callback function data type */
typedef void (k_CB_CALL_CONV *PTP_t_pfCbErr)(UINT32          dw_unit, 
                                             UINT32          dw_err,
                                             PTP_t_sevCdEnum e_sev,
                                             PTP_t_TmStmp    *ps_ts,
                                             BOOLEAN         o_run);

/************************************************************************/
/** PTP_t_FUNC_INP_MTSD 
             This typedef defines the filter input-mtsd function type.
**/
typedef BOOLEAN (*PTP_t_FUNC_INP_MTSD)( const PTP_t_TmIntv *ps_tiMtsd,
                                        const PTP_t_TmStmp *ps_txTsSyn,
                                        const PTP_t_TmStmp *ps_rxTsSyn,
                                        UINT16             w_ifIdx,
                                        UINT32             dw_TaSynIntvUsec,
                                        PTP_t_TmIntv       *ps_tiOffset,
                                        UINT32             *pdw_TaOffsUsec,
                                        PTP_t_TmIntv       *ps_tiFltMtsd,
                                        UINT8              *pb_mode);

/************************************************************************/
/** PTP_t_FUNC_INP_STMD 
             This typedef defines the filter input-stmd function type.
**/
typedef void (*PTP_t_FUNC_INP_STMD)( const PTP_t_TmIntv *ps_tiStmd,
                                     const PTP_t_TmStmp *ps_rxTsDlrq,
                                     const PTP_t_TmStmp *ps_txTsDlrq,
                                     UINT32             dw_TaDlrqIntv,
                                     PTP_t_TmIntv       *ps_tiFltStmd,
                                     PTP_t_TmIntv       *ps_tiFltOwd,
                                     UINT8              *pb_mode);

/************************************************************************/
/** PTP_t_FUNC_INP_P2P 
             This typedef defines the filter input-p2p-delay function type.
**/
typedef void (*PTP_t_FUNC_INP_P2P)( const PTP_t_TmIntv *ps_tiP2PDel,
                                    const PTP_t_TmStmp *ps_txTsPDlrq,
                                    const PTP_t_TmStmp *ps_rxTsPDlrsp,
                                    UINT16             w_ifIdx,
                                    UINT32             dw_taP2PIntvUsec,
                                    PTP_t_TmIntv       *ps_tiFltP2Pdel,
                                    UINT8              *pb_mode);

/************************************************************************/
/** PTP_t_FUNC_INP_NEWMST 
             This typedef defines the filter input-new-master function type.
**/
typedef void (*PTP_t_FUNC_INP_NEWMST)(const PTP_t_PortId   *ps_pIdNewMst,
                                      const PTP_t_PortAddr *ps_pAddrNewMst,
                                      BOOLEAN              o_mstIsUnicast);

/************************************************************************/
/** PTP_t_FUNC_FLT_RESET 
             This typedef defines the filter reset function type.
**/
typedef void (*PTP_t_FUNC_FLT_RESET)( void ) ;

/************************************************************************/
/** PTP_t_FUNC_FLT_INIT 
             This typedef defines the filter initialization function type.
**/
typedef BOOLEAN (*PTP_t_FUNC_FLT_INIT)( const PTP_t_fltCfg    *ps_fltCfg,
                                        PTP_t_FUNC_INP_MTSD   *ppf_inpMtsd,
                                        PTP_t_FUNC_INP_STMD   *ppf_inpStmd,
                                        PTP_t_FUNC_INP_P2P    *ppf_inpP2P,
                                        PTP_t_FUNC_INP_NEWMST *ppf_inpNewMst,
                                        PTP_t_FUNC_FLT_RESET  *ppf_fltReset);

/************************************************************************/
/** PTP_t_pntUnion
                 Union for differen pointer types. 
                 This pointer type union is used in the 
                 PTP_t_MboxMsg type.
**/
typedef union
{
  PTP_t_TmStmp          *ps_tmStmp;
  PTP_t_TmIntv          *ps_tmIntv;
  PTP_t_PortId          *ps_pId;
  PTP_t_PortAddr        *ps_pAddr;
  PTP_t_PortAddrQryTbl  *ps_pAQTbl;
  PTP_t_Text            *ps_text;
  UINT8                 *pb_u8;
  UINT16                *pw_u16;
  UINT32                *pdw_u32;
  UINT64                *pddw_u64;
  INT8                  *pc_s8;
  INT16                 *ps_s16;
  INT32                 *pl_s32;
  INT64                 *pll_s64;
  void                  *pv_data;
  NIF_t_PTPV2_AnnMsg    *ps_annc;
  NIF_t_PTPV2_SyncMsg   *ps_syn;
  NIF_t_PTPV2_FlwUpMsg  *ps_flw;
  NIF_t_PTPV2_MntMsg    *ps_mnt;
  NIF_t_PTPV2_DlrqMsg   *ps_dlrq;
  NIF_t_PTPV2_DlRspMsg  *ps_dlrsp;
  NIF_t_PTPV2_PDelResp  *ps_pDrsp;
  NIF_t_PTPV2_PDlRspFlw *ps_pDrFu;
  NIF_t_PTPV2_PDelReq   *ps_pDreq;
}PTP_t_pntUnion;

/************************************************************************/
/** PTP_t_dataUnion
                 Union for differen data types. This data type union
                 is used in the PTP_t_MboxMsg type.
**/
typedef union
{
  BOOLEAN           o_bool;
  INT8              c_s8;
  INT16             i_s16;
  INT32             l_s32;
  UINT8             b_u8;
  UINT16            w_u16;
  UINT32            dw_u32;
  PTP_t_sevCdEnum   e_sevC;
  PTP_t_msgTypeEnum e_extMType;
}PTP_t_dataUnion;

/************************************************************************/
/** PTP_t_MboxMsg
                 Definition of a message type for message passing via SIS 
                 mailboxes.
**/
typedef struct 
{
  PTP_t_dataUnion  s_etc1;     /* additional information 1*/ 
  PTP_t_dataUnion  s_etc2;    /* additional information 2*/ 
  PTP_t_dataUnion  s_etc3;    /* additional information 3 */
  PTP_t_pntUnion   s_pntData; /* pointer union to data */ 
  PTP_t_pntUnion   s_pntExt1; /* pointer union to extra data  1 */
  PTP_t_pntUnion   s_pntExt2; /* pointer union to extra data  2 */
  PTP_t_intMsgEnum e_mType;    /* the message type */
}PTP_t_MboxMsg;

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
*************************************************************************/

/***********************************************************************
**
** Function    : PTP_Init
**
** Description : Initializes and starts the PTP engine. The return
**               value determines the unique clock id of the node.
**
** Parameters  : pf_cbErr       (IN) - pointer to callback function
**                                     that is called with all errors
**               ps_extTmPropDs (IN) - pointer to external time properties
**                                     data set provided by an external 
**                                     primary reference clock.
**                                     Must be NULL, if there is none.
**
** Returnvalue : <>NULL - PTP initialization started / pointer to clock id
**               ==NULL - PTP initialization failed
**                      
** Remarks     : -
**
***********************************************************************/
PTP_t_ClkId* PTP_Init(void (k_CB_CALL_CONV *pf_cbErr)
                                        (UINT32          dw_unit,
                                         UINT32          dw_err,
                                         PTP_t_sevCdEnum e_sev,
                                         PTP_t_TmStmp    *ps_ts,
                                         BOOLEAN         o_run),
                      PTP_t_TmPropDs* ps_extTmPropDs);

/***********************************************************************
**  
** Function    : PTP_Main
**  
** Description : The main function of the PTP engine must be called
**               cyclically within a interval of k_PTP_TIME_RES_NSEC.
**               The function must be called in the same thread-
**               context as PTP_Init and PTP_Close.
**  
** Parameters  : ps_tiActOffs (OUT) - current offset
**               
** Returnvalue : TRUE          - PTP engine is still running
**               FALSE         - PTP engine stopped
** 
** Remarks     : Function is not reentrant.
**  
***********************************************************************/
BOOLEAN PTP_Main(PTP_t_TmIntv *ps_tiActOffs);

/***********************************************************************
** 
** Function    : PTP_Close
**
** Description : This function stops the PTP engine.
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void PTP_Close(void);

/***********************************************************************
**  
** Function    : PTP_ReadClkDs
**  
** Description : Gives read-only-access to the Clock Data set.
**               This function copies the actual clock data set
**               into the structure, passed by the application.
**               Use this function to get fast read access to the 
**               Clock data set. 
**               For write-access, the MNT API must be used.
**  
** Parameters  : ps_clkDs (OUT) - pointer to copy of clock data
**                                set. Function writes into this location.
**               
** Returnvalue : TRUE           - function succeeded
**               FALSE          - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN PTP_ReadClkDs(PTP_t_ClkDs *ps_clkDs);

/***********************************************************************
**  
** Function    : PTP_ChgFltCfg
**  
** Description : Allows the user to register its own implementation
**               specific filter while runtime of the stack.
**               Additionally it is possible to change the filter 
**               to an internal filter implementation and to 
**               configure the filter.
**               The reconfiguration of the filter causes internal
**               reinitialization of the filter. The new settings for
**               the filter file are passed through the filter configuration
**               file. This file can be stored in non-volatile memory
**               to recover the filter settings after reboot of the 
**               device.
**               To use an internal filter, set the filter config file
**               with
**
**               filter type:
**               - e_FLT_NO  (no filtering)
**               - e_FLT_MIN (Minimum filter)
**               - e_FLT_EST (Estimator filter)
**
**               filter parameters: 
**               - Take a look into the manual for the filter configuration
**               description.
**
**               To use an external filter, set the filter configuration
**               file -> e_filtType to e_FILT_ISPCFC and pass the implementation
**               specific filter functions to register it in the
**               filter module.
** 
**               See CTLflt.h for the filter functions interface.
**  
** Parameters  : ps_fltCfg       (IN) - filter configuration file
**               o_strFltCfg     (IN) - flag to store the config file
**               pf_inpMtsdExt   (IN) - external input function for MTSD
**               pf_inpStmdExt   (IN) - external input function for STMD
**               pf_inpP2PdExt   (IN) - external input funciton for P2P del
**               pf_inpNewMstExt (IN) - external input function for 
**                                      new Master information
**               pf_rstFltExt    (IN) - extern filter reset function
**               
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN PTP_ChgFltCfg(const PTP_t_fltCfg    *ps_fltCfg,
                      BOOLEAN               o_strFltCfg,
                      PTP_t_FUNC_INP_MTSD   pf_inpMtsdExt,
                      PTP_t_FUNC_INP_STMD   pf_inpStmdExt,
                      PTP_t_FUNC_INP_P2P    pf_inpP2PdExt,
                      PTP_t_FUNC_INP_NEWMST pf_inpNewMstExt,
                      PTP_t_FUNC_FLT_RESET  pf_rstFltExt);

/***********************************************************************
** 
** Function    : PTP_GetSysTime
**
** Description : This function gets the system time as a timestamp 
**               in PTP-epoch.
**
** Parameters  : ps_ts          (OUT) - pointer to the system-time in
**                                      PTP Timestamp format
**
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
**
** Remarks     : Function is not reentrant.
**
*************************************************************************/
BOOLEAN PTP_GetSysTime(PTP_t_TmStmp *ps_ts);


/***********************************************************************
** 
** Function    : PTP_SetSysTime
**
** Description : This function sets the system time in PTP-epoch. 
**
** Parameters  : ps_ts          (IN)  - pointer to the system-time in
**                                      PTP Timestamp format
**
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
**
** Remarks     : Function is not reentrant.
**
*************************************************************************/
BOOLEAN PTP_SetSysTime(const PTP_t_TmStmp *ps_ts);

/*************************************************************************
**
** Function    : PTP_SetSysTimeOffset
**
** Description : This function adds or subtracts a step offset to the
**               current system time. It is only used to correct offsets
**               between the master and slave clock, if these offsets
**               exceeds one second during the first synchronization or
**               after switching to a new master.
** 
**               The function parameter is not the absolute time to be set.
**               It represents the offset to the current master given
**               in PTP time interval (see also {PTP_t_TmIntv}). It
**               should be noted that a positive offset value describes
**               a slave time located within the future compared to the
**               master clock. Hence, a //positive offset// must be
**               //subtracted// from the current slave time. Likewise, a
**               //negative offset// value must be //added// to the 
**               current slave time.
**
**               //Remark//: To correct the clock drift during normal
**               operation the function {CIF_AdjClkDrift()} is used.
**
** See Also    : PTP_SetSysTimeOffsSec()
**
** Parameters  : ps_ti          (IN)  - offset to correct
**
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
**
** Remarks     : Function is not reentrant.
**
*************************************************************************/
BOOLEAN PTP_SetSysTimeOffset(const PTP_t_TmIntv *ps_ti);

/*************************************************************************
**
** Function    : PTP_SetSysTimeOffsSec
**
** Description : This function adds or subtracts a step offset to the
**               current system time, if the offset between the master and
**               slave clock exceeds 140737 seconds. The time interval
**               data type in {PTP_SetSysTimeOffset()} is not sufficient
**               enough for that correction value and therefore this
**               function is used. The function parameter represents the
**               offset in seconds to be added or subtracted from the
**               current time.
**
**               It should be noted, that a positive offset value describes
**               a slave time, which is located within the future compared
**               to the master clock. Hence, a //positive offset// must
**               be //subtracted// from the current slave time. The other
**               way round, a //negative offset// value must be //added// 
**               to the current slave time.
**
**               //Remark//: To correct the clock drift during normal
**               operation the function {CIF_AdjClkDrift()} is used.
**
** See Also    : PTP_SetSysTimeOffset()
**
** Parameters  : pll_secOffs    (IN)  - second offset to correct
**
** Returnvalue : TRUE                 - function succeeded
**               FALSE                - function failed
**
** Remarks     : Function is not reentrant.
**
*************************************************************************/
BOOLEAN PTP_SetSysTimeOffsSec(const INT64* pll_secOffs);

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
** Remarks     : Function is reentrant.
**
*************************************************************************/
BOOLEAN PTP_AddTmSTmpTmStmp(PTP_t_TmStmp const *ps_ts_A,
                            PTP_t_TmStmp const *ps_ts_B,
                            PTP_t_TmStmp *ps_tsRes);

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
** Remarks     : Function is reentrant.
**
*************************************************************************/
BOOLEAN PTP_SubtrTmStmpTmStmp(PTP_t_TmStmp const *ps_ts_A,
                              PTP_t_TmStmp const *ps_ts_B,
                              PTP_t_TmIntv   *ps_ti_Res);

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
** Remarks     : Function is reentrant.
**
*************************************************************************/
BOOLEAN PTP_SubtrTmIntvTmStmp(PTP_t_TmStmp const *ps_ts_A,
                              PTP_t_TmIntv   const *ps_ti_B,
                              PTP_t_TmStmp *ps_ts_Res);

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
** Remarks     : Function is reentrant.
**
*************************************************************************/
BOOLEAN PTP_AddTmIntvTmStmp(PTP_t_TmStmp const *ps_ts_A,
                            PTP_t_TmIntv   const *ps_ti_B,
                            PTP_t_TmStmp *ps_ts_Res);

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
** Remarks     : -
**
*************************************************************************/
INT8 PTP_ComparePortId(const PTP_t_PortId *ps_portId_A,
                       const PTP_t_PortId *ps_portId_B);

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
** Remarks     : -
**
*************************************************************************/
INT8 PTP_CompPortAddr(const PTP_t_PortAddr *ps_portAddr_A,
                      const PTP_t_PortAddr *ps_portAddr_B);

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
** Remarks     : -
**
*************************************************************************/
INT8 PTP_CompareClkId(const PTP_t_ClkId *ps_clkId_A,
                      const PTP_t_ClkId *ps_clkId_B);


/** C2DOC_stop */
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
** Remarks     : function is reentrant
**
***********************************************************************/
UINT32 PTP_sqrt_U64(UINT64 ddw_inp);

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
** Remarks     : function is reentrant
**
***********************************************************************/
INT64 PTP_LowPass(const INT64 ll_actVal,INT64 *pll_sum,INT16 i_T);

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
** Remarks     : Function is not reentrant
**  
***********************************************************************/
INT64 PTP_PIctr(INT64 ll_ek,BOOLEAN o_rst,UINT32 dw_TaUsec,INT64 ll_preLoadSum);

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
** Remarks     : function is reentrant
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
                    INT64        *pll_sum_xi);

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
** Remarks     : function is reentrant
**
***********************************************************************/
UINT32 PTP_getTimeIntv(INT8 c_expon);

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
** Remarks     : function is reentrant
**
***********************************************************************/
UINT16 PTP_logDualis(UINT64 ddw_val);

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
UINT16 PTP_scaleVar( UINT64 ddw_var );

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
** Remarks     : function is reentrant
**
***********************************************************************/
UINT64 PTP_GetAllenVar(INT32 l_actOffs,BOOLEAN o_reset);

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
** Remarks     : -
**  
***********************************************************************/
INT64 PTP_PowerOf2(UINT8 b_exp);

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
** Remarks     : 
**  
***********************************************************************/
INT64 PTP_AddnCheckI64(INT64 ll_var, INT64 ll_add);

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
INT32 PTP_ChkRngSetExtr(INT32 l_val,INT32 l_min,INT32 l_max);

/** C2DOC_start */
/***********************************************************************
** 
** Function    : PTP_SetError
**
** Description : This function calls the application callback 
**               function and sends an error to the unit MNT to 
**               log the errors. 
**
** Parameters  : dw_unitId   (IN) - unit specific error identifier
**               dw_err      (IN) - error number
**               e_sevCode   (IN) - severity code of error
**
** Returnvalue : -
**
** Remarks     : -
**
*************************************************************************/
void PTP_SetError(UINT32          dw_unitId,
                  UINT32          dw_err,
                  PTP_t_sevCdEnum e_sevCode);

/*************************************************************************
**
** Function    : PTP_GetErrStr
**
** Description : This function resolves the error number to the 
**               corrospondent error string
**
** Parameters  : dw_unitId     (IN) - unit where error occured
**               dw_errNmb     (IN) - error number 
**               
** Returnvalue : const CHAR*        - pointer to error string
** 
** Remarks     : Gets compiled with definition of ERR_STR.
**  
***********************************************************************/
const CHAR* PTP_GetErrStr(UINT32 dw_unitId,UINT32 dw_errNmb);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __PTP_H__ */
