/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: FLTest.h
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
**
**   Compiler: Ansi-C
**    Remarks:
**
**************************************************************************
**    all rights reserved
*************************************************************************/

/*************************************************************************
**    constants and macros
*************************************************************************/

/*************************************************************************
**    data types
*************************************************************************/

/*************************************************************************
**    global variables
*************************************************************************/

/*************************************************************************
**    function prototypes
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
                     PTP_t_FUNC_FLT_RESET  *ppf_fltReset);

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
                        UINT8              *pb_mode);

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
                    UINT8              *pb_mode);

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
                      UINT8              *pb_mode);

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
                      BOOLEAN              o_mstIsUnicast);

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
void FLTest_resetFilter( void );
