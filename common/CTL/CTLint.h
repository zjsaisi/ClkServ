/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: CTLint.h 
**    Summary: The control unit CTL controls all node data and all events
**             for the node and his channels to the net. 
**
**             Internal function declarations of the unit CTL
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: CTL_GetActPeerDelay
**             CTL_InitClockDS
**             CTL_ResetCurDs
**             CTL_InitParentDS
**             CTL_InitFrgnMstDS
**             CTL_ClearFrgnMstDs
**             CTL_ResetFrgnMstDs
**             CTL_UpdtFrgnMstDs
**             CTL_SetAccpFlagFrgnMstDs
**             CTL_UpdateM1_M2
**             CTL_UpdateMMDlrqIntv
**             CTL_UpdateS1
**             CTL_AmIGrandmaster
**             CTL_SetPDStoGM
**             CTL_SetPDSstatistics
**             CTL_ResetPDSstatistics
**             CTL_StateDecisionEvent
**             CTL_ResetClockDrift
**
**   Compiler: Ansi-C
**    Remarks:
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef __CTLINT_H__
#define __CTLINT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/
/* error definitions  */
#define CTL_k_ERR_MSGTP    (0u) /* wrong message type */
#define CTL_k_ERR_INIT_1   (1u) /* fault while initialization */
#define CTL_k_ERR_INIT_2   (2u) /* fault while initialization */
#define CTL_k_ERR_INIT_3   (3u) /* fault while initialization */
#define CTL_k_ERR_INIT_4   (4u) /* fault while initialization */
#define CTL_k_ERR_INIT_5   (5u) /* fault while initialization */

#define CTL_k_STS_DEC     (10u) /* wrong state decision code */
#define CTL_k_RD_UCTBL    (11u) /* error reading unicast table */
#define CTL_k_WR_UCTBL    (12u) /* error writing unicast table */

/* BMC errors */
#define CTL_k_BMC_R_EQ_S  (20u) /* Receiver is equal to sender */
#define CTL_k_BMC_A_EQ_B  (21u) /* Announce messages are identical */
#define CTL_k_ERR_SCL     (22u) /* Error with scaling the drift change */

#define CTL_k_ERR_SLSMBOX (30u) /* Error sending msg to Slave sync task */
#define CTL_k_ERR_SLDMBOX (31u) /* Error sending msg to Slave delay task */
#define CTL_k_ERR_UCMBOX  (32u) /* Error sending msg to Unicast tasks */
#define CTL_k_ERR_P2PMBOX (33u) /* Error sending msg to P2Pdel task */
#define CTL_k_ERR_UCDMBOX (34u) /* Error sending msg to UCD task */
#define CTL_k_ERR_MASMBOX (35u) /* Error sending msg to Master sync task */
#define CTL_k_ERR_MAAMBOX (36u) /* Error sending msg to Master announce task */
#define CTL_k_ERR_MADMBOX (37u) /* Error sending msg to Master delay task */

#define CTL_k_MDLRSP_ERR  (40u) /* subsequent missing delay response error */
#define CTL_k_MSYNC_ERR   (41u) /* subsequent missing sync error */
#define CTL_k_MFLWUP_ERR  (42u) /* subsequent missing folow up error */

#define CTL_k_CLOSE_ERR_0 (50u) /* error closing unit */

#define CTL_k_ERR_FILT_INIT (60u) /* error initializing filter */
#define CTL_k_ERR_FILT_DNE  (61u) /* filter does not exist */
#define CTL_k_ERR_FILT_WR   (62u) /* error writing filter file */
#define CTL_k_ERR_FILT_RD   (63u) /* error reading filter file */
#define CTL_k_ERR_FLT_FUNC  (64u) /* wrong filter function initialization */

#define CTL_k_CFGFILE_DEF   (65u) /* error reading config file, 
                                     default values will be written */
#define CTL_k_ERR_CRCVR     (66u) /* Clock recovery file access not OK - 
                                     clock recovery works with default values */


/* state definitions for the CTLmain task */
#define CTL_k_OPERATIONAL     (1u)
#define CTL_k_PRE_OPERATIONAL (2u)
#define CTL_k_REINIT          (3u)

/* results of the best master clock comparison algorithm  */
#define k_A_SAME_AS_B                  (0u)
#define k_A_BETTER_THAN_B              (1u)
#define k_A_BETTER_THAN_B_BY_TOPOLOGY  (2u)
#define k_B_BETTER_THAN_A              (3u)
#define k_B_BETTER_THAN_A_BY_TOPOLOGY  (4u)
#define k_MESSAGE_FROM_SELF            (5u)
#define k_A_SIMILAR_TO_B               (6u)

/* state decision codes */
#define k_UDC_M1   (0u)
#define k_UDC_M2   (1u)
#define k_UDC_M3   (2u)
#define k_UDC_S1   (3u)
#define k_UDC_P1   (4u)
#define k_UDC_P2   (5u)
#define k_UDC_DONO (6u)
/*************************************************************************
**    data types
*************************************************************************/

/************************************************************************/
/** CTL_t_ClkCmpSet :
               This struct is required for the state decision event and
               the best master clock. The best master clock algorithm
               compares two time sources against each other. When the
               state decision event searches for the next state 
               constellation, the receiving port is needed to set the
               appropriate state to it.

**/
typedef struct 
{
  NIF_t_PTPV2_AnnMsg *ps_anncMsg; /* a qualified announce message */
  PTP_t_PortId       *ps_rcvPId;  /* port id of receiving interface */
  PTP_t_PortId       *ps_frgnPId; /* port id of foreign master */ 
  PTP_t_PortAddr     *ps_pAddr;   /* port address (for unicast) */
  BOOLEAN            o_isUc;      /* flag determines, if master is unicast */ 
}CTL_t_ClkCmpSet;

/************************************************************************/
/** CTL_t_commIf :
               This struct stores the communication interface informations
**/
typedef struct 
{
  PTP_t_PortId  as_pId[k_NUM_IF]; /* port ids of all initialized 
                                     communication interfaces */
  UINT32        dw_amntInitComIf; /* number of initialized +
                                     communication interfaces */
}CTL_t_commIf;

/*************************************************************************
**    global variables
*************************************************************************/
/* Handles of the tasks to address messageboxes */
extern UINT16 CTL_aw_Hdl_mas[k_NUM_IF];
extern UINT16 CTL_aw_Hdl_mad[k_NUM_IF];
extern UINT16 CTL_aw_Hdl_maa[k_NUM_IF];
#if( k_CLK_DEL_P2P == TRUE )
  extern UINT16 CTL_aw_Hdl_p2p_del[k_NUM_IF];
  extern UINT16 CTL_aw_Hdl_p2p_resp[k_NUM_IF];
#endif

/* Clock data set   */
extern PTP_t_ClkDs CTL_s_ClkDS;
extern CTL_t_commIf CTL_s_comIf;
/* pointer to external variable, that holds pointer to 
   external time properties data set. This data set is filled 
   with the data of an external primary clock source */
extern PTP_t_TmPropDs* CTL_ps_extTmPropDs;

/*************************************************************************
**    function prototypes
*************************************************************************/

/***********************************************************************
**
** Function    : CTL_GetActPeerDelay
**
** Description : Returns the actual peer delay of the requested interface.
**              
** Parameters  : w_ifIdx  (IN) - communication interface index 
**                               to get the peer delay.
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
INT64 CTL_GetActPeerDelay(UINT16 w_ifIdx);

/***********************************************************************
**
** Function    : CTL_InitClockDS
**
** Description : Initializes the Clock Data set. 
**               Member of the clock data set are: 
**                             - Default data set
**                             - Current data set 
**                             - Parent data set 
**                             - Global time properties data set 
**                             - Port configuration data sets 
**                             - Foreign master data sets 
** 
** Parameters  : ps_cfgRom    (IN) - pointer to configuration out of rom
**               ps_cRcvr     (IN) - pointer to clock recovery file
**
** Returnvalue : -
** 
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_InitClockDS( const PTP_t_cfgRom     *ps_cfgRom,
                      const PTP_t_cfgClkRcvr *ps_cRcvr);

/***********************************************************************
**
** Function    : CTL_ResetCurDs
**
** Description : Resets the current data set to zero.
**              
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_ResetCurDs( void );

/***********************************************************************
**
** Function    : CTL_InitParentDS
**
** Description : Initializes the parent data set. 
**              
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_InitParentDS(void);

/***********************************************************************
**
** Function    : CTL_InitFrgnMstDS
**
** Description : Initializes a foreign master data set. 
**              
** Parameters  : w_ifIdx (IN) - communication interface index
**               w_rcrd  (IN) - entry of data set ( 0,...,n-1 )
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_InitFrgnMstDS(UINT16 w_ifIdx,UINT16 w_rcrd);

/***********************************************************************
**
** Function    : CTL_ClearFrgnMstDs
**
** Description : Clears the foreign master data sets. 
**               All sync messages older than PTP_FOREIGN_MASTER_TIME_WINDOW 
**               shall be cleared.
**
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_ClearFrgnMstDs(void);


/***********************************************************************
** 
** Function    : CTL_ResetFrgnMstDs
**
** Description : Resets the foreign master data sets
**               Independent of the PTP_FOREIGN_MASTER_TIME_WINDOW
**
** Parameters  : -
**
** Returnvalue : -
**
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_ResetFrgnMstDs(void);

/***********************************************************************
**
** Function    : CTL_UpdtFrgnMstDs
**
** Description : Updates the foreign master data set.
**               When a announce message is received from a foreign master
**               clock, the foreign master data set of the receiving
**               port shall be updated. If the foreign master is known,
**               the corresponding data set will be updated by 
**               incrementing the field dw_NmbAnncMsg. When it
**               is a unknown foreign master, a new set will be created.
**               Data sets, that were not updated since 
**               PTP_FOREIGN_MASTER_TIME_WINDOW will be deleted.
**
** Parameters  : ps_newMsg (IN) - pointer to received announce msg
**               w_ifIdx   (IN) - receiving communication interface index
**               ps_pAddr  (IN) - port Address (for unicast masters)
**
** Returnvalue : TRUE  - is acceptable master
**               FALSE - is not acceptable master
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
BOOLEAN CTL_UpdtFrgnMstDs(const NIF_t_PTPV2_AnnMsg* ps_newMsg,
                          UINT16 w_ifIdx,
                          const PTP_t_PortAddr *ps_pAddr);

/***********************************************************************
**  
** Function    : CTL_SetAccpFlagFrgnMstDs
**  
** Description : Sets the value for the acceptable flag in the foreign
**               master data set and the timeout if the master is set 
**               to unacceptable.
**  
** Parameters  : ps_pId  (IN) - port Id of master
**               w_ifIdx (IN) - communication interface index to master
**               o_isUc  (IN) - is master unicast master ?
**               o_isAcc (IN) - flag to set
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTL_SetAccpFlagFrgnMstDs(const PTP_t_PortId *ps_pId,
                              UINT16 w_ifIdx,
                              BOOLEAN o_isUc,
                              BOOLEAN o_isAcc);

/***********************************************************************
**
** Function    : CTL_UpdateM1_M2
**
** Description : Updates for state decision codes M1 and M2.  
**               The port switches to PTP_MASTER. So the parent data set 
**               and the grandmaster data set is set to the data 
**               of the default data set.
**              
** Parameters  : -
**
** Returnvalue : -            
**                      
** Remarks     : definition in chapter 9.3.5 table 13 of the standard
**
***********************************************************************/
void CTL_UpdateM1_M2(void);

/***********************************************************************
** 
** Function    : CTL_UpdateMMDlrqIntv
**
** Description : Updates the min mean delay request interval value in 
**               the clock data set, if a node changes to master.
**
** Parameters  : w_ifIdx (IN) - communication interface index to update
**
** Returnvalue : -
**
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_UpdateMMDlrqIntv(UINT16 w_ifIdx);

/***********************************************************************
** 
** Function    : CTL_UpdateS1
**
** Description : Updates for state decision code S1.
**               After changing the port state to PTP_SLAVE, the parent
**               data set must be updated to the new parent and grand-
**               master values of the announce msg. 
**
** Parameters  : ps_msg (IN) - pointer to received announce msg
**               o_isUc (IN) - flag says, if parent master is unicast
**
** Returnvalue : -
**
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_UpdateS1(const NIF_t_PTPV2_AnnMsg *ps_msg,BOOLEAN o_isUc);


/***********************************************************************
**
** Function    : CTL_AmIGrandmaster
**
** Description : Checks, if the node is the grandmaster. 
**               When it changes grandmaster the first time, the parent 
**               data set gets set to the values of the default data set. 
**               The clock drift adjustment gets set to zero.
**              
** Parameters  : -
**
** Returnvalue : TRUE  - configuration changed
**               FALSE - configuration did not change
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
BOOLEAN CTL_AmIGrandmaster(void);

/***********************************************************************
**
** Function    : CTL_SetPDStoGM
**
** Description : When the node gets grandmaster of the subdomain, the 
**               members of the parent data set have to be set to the 
**               values of the default data set. Some values must then 
**               be changed in the sync - and delay_req message template,too.
**              
** Parameters  : -
**
** Returnvalue : -
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
void CTL_SetPDStoGM(void);

/***********************************************************************
**  
** Function    : CTL_SetPDSstatistics
**  
** Description : Generates the synchronization statistics 
**               in the parent data set.
**  
** Parameters  : pll_psec_per_sec  (IN)  - act psec/sec adjustment
**               pll_meanDriftPsec (OUT) - mean drift in psec/sec
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTL_SetPDSstatistics(const INT64 *pll_psec_per_sec,
                          INT64       *pll_meanDriftPsec );

/***********************************************************************
**  
** Function    : CTL_ResetPDSstatistics
**  
** Description : Resets the statistics in the parent data set.
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTL_ResetPDSstatistics( void );

/***********************************************************************
**
** Function    : CTL_StateDecisionEvent
**
** Description : Executes the state decision event.
**               The state decision event is the mechanism for using the
**               data in received announce messages and his own default data
**               set to determine which is the best master clock, and whether
**               the node needs to change its state. This event occurs 
**               simultaneously on all ports of a clock once per announce
**               interval. 
**               First this function determines the best clock for each port,
**               plugged to this port ( E_rBest ). Then out of this
**               the best clock for the port is determined. Then it
**               decides for each port, which state is recommended and 
**               switches to this state, if it is another.
**              
** Parameters  : pao_anncRcptTmoExp (IN)  - announce receipt timeouts 
**                                          expired flag
**               pao_rstAnncTmo     (OUT) - flag array returns, if announce 
**                                          receipt timeout shall be restarted
**
** Returnvalue : TRUE  - configuration changed
**               FALSE - configuration did not change
**                      
** Remarks     : function is reentrant
**
***********************************************************************/
BOOLEAN CTL_StateDecisionEvent( const BOOLEAN *pao_anncRcptTmoExp,
                                      BOOLEAN *pao_rstAnncTmo);

/***********************************************************************
**  
** Function    : CTL_ResetClockDrift
**  
** Description : Sets the clock drift to the stored drift value if 
**               specified in the configuration file, otherwise the
**               clock runs with its own speed.
**               This function gets called if the node gets master or if
**               sync messages are timed out.
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void CTL_ResetClockDrift( void );

void CTL_ClearAccpFlagFrgnMstDs( UINT16 w_ifIdx );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __CTLINT_H__ */

