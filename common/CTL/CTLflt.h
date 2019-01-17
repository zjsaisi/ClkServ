/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: CTLflt.h 
**    Summary: This unit implements different filter methods 
**             that can be used to filter the measurement data.
**             It depends on the used system, what filter is ideal.
**             Please refer to the documentation to see more details.
**              
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**             
**  Functions: CTLflt_Init
**             CTLflt_chgFilter
**             CTLflt_regExtFilter
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
**    constants and macros
*************************************************************************/
/* constants for the filter mode */
#define k_HOLDOVER (0) /* holdover mode - data unfiltered */
#define k_RUNNING  (1) /* running mode - data are filtered */

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
void CTLflt_Init( const PTP_t_fltCfg *ps_fltCfg );

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
                         PTP_t_FUNC_FLT_RESET  pf_rstFltExt);

/***********************************************************************
**  
** Function    : CTLflt_regExtFilter
**  
** Description : Registers the input filter functions for the
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
BOOLEAN CTLflt_regExtFilter( PTP_t_FUNC_INP_MTSD   pf_inpMtsdExt,
                             PTP_t_FUNC_INP_STMD   pf_inpStmdExt,
                             PTP_t_FUNC_INP_P2P    pf_inpP2PdExt,
                             PTP_t_FUNC_INP_NEWMST pf_inpNewMstExt,
                             PTP_t_FUNC_FLT_RESET  pf_rstFltExt);

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
                       UINT8              *pb_mode);

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
                    const PTP_t_TmStmp *ps_rxTsDlrq,
                    const PTP_t_TmStmp *ps_txTsDlrq,
                    UINT32             dw_TaDlrqIntv,
                    PTP_t_TmIntv       *ps_tiFltStmd,
                    PTP_t_TmIntv       *ps_tiFltOwd,
                    UINT8              *pb_mode);
                       
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
                       UINT8              *pb_mode);
                                                
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
                      BOOLEAN              o_mstIsUnicast);
 
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
void CTLflt_resetFilter( void );
