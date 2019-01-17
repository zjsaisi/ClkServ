/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: TGT.h 
**    Summary: TGT - Target definitions
**             This unit defines target specific functions that are not 
**             related to other units such as TSU, CIF and GOE. The porting 
**             engineer could use this unit to define additional macros,
**             data types and functions for the port to the hardware. 
**             Furthermore, some functions are defined for the use of the 
**             IEEE 1588 protocol software, such as thread-safe print 
**             functions.
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: TGT_Init
**             TGT_Close
**             TGT_printf
**             TGT_printSyncData
**             TGT_printDrift
**
**   Compiler: gcc 3.3.2
**    Remarks:  
**
**************************************************************************
**    all rights reserved
*************************************************************************/

#ifndef _TGT_H_
#define _TGT_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*************************************************************************
**    constants and macros
*************************************************************************/

#define PTP_GET_RX_TIMESTAMP_SYNC        (SIOCDEVPRIVATE)
#define PTP_GET_RX_TIMESTAMP_DEL_REQ     (SIOCDEVPRIVATE + 1)
#define PTP_GET_RX_TIMESTAMP_FOLLOWUP    (SIOCDEVPRIVATE + 2)
#define PTP_GET_RX_TIMESTAMP_DEL_RESP    (SIOCDEVPRIVATE + 3)
#define PTP_GET_TX_TIMESTAMP             (SIOCDEVPRIVATE + 4)
#define PTP_SET_CNT                      (SIOCDEVPRIVATE + 5)
#define PTP_GET_CNT                      (SIOCDEVPRIVATE + 6)
#define PTP_SET_FIPER_ALARM              (SIOCDEVPRIVATE + 7)
#define PTP_ADJ_ADDEND                   (SIOCDEVPRIVATE + 9)
#define PTP_GET_ADDEND                   (SIOCDEVPRIVATE + 10)
#define PTP_GET_RX_TIMESTAMP_PDELAY_REQ  (SIOCDEVPRIVATE + 11)
#define PTP_GET_RX_TIMESTAMP_PDELAY_RESP (SIOCDEVPRIVATE + 12)
#define PTP_CLEANUP_TIMESTAMP_BUFFERS    (SIOCDEVPRIVATE + 13)

#define NSEC                             ((UINT64)1000000000)
#define BOARD_TIME_TO_SEC(board_time)    ((UINT32)(((UINT64)board_time)/NSEC))
#define BOARD_TIME_TO_NSEC(board_time, board_time_sec) ((UINT32)(board_time - (((UINT64)board_time_sec) * NSEC)))


/*************************************************************************
**    data types
*************************************************************************/

/** TGT_t_8313TimeFormat
                 This struct defines the MPC8313 internal timestamp format.
**/
typedef struct
{
  UINT32  dw_high;
  UINT32  dw_low;
}TGT_t_8313TimeFormat;

/*************************************************************************
**    global variables
*************************************************************************/

extern BOOLEAN         TGT_o_silent;
extern BOOLEAN         TGT_o_offs;
extern BOOLEAN         TGT_o_mtsd;
extern BOOLEAN         TGT_o_stmd;
extern BOOLEAN         TGT_o_owd;
extern BOOLEAN         TGT_o_meanDrft;
/*************************************************************************
**    function prototypes
*************************************************************************/

/***********************************************************************
**  
** Function    : TGT_Init
**  
** Description : Initializes target-specific things not covered 
**               in GOE. This is the first initialization routine 
**               inside PTP_Init
** 
** See Also    : PTP_Init()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void TGT_Init( void );
/***********************************************************************
**  
** Function    : TGT_Close
**  
** Description : Deinitializes target-specific things not covered 
**               in GOE. This is the last deinitialization routine 
**               inside PTP_Close
** 
** See Also    : PTP_Close()
**  
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
void TGT_Close( void );

/***********************************************************************
**
** Function    : TGT_printf
**
** Description : This function is a device specific debug print method. 
**               This method must be adapted to the target and implement 
**               a thread-safe printf() functionality. One method for 
**               achieving this requirement might include the use of 
**               semaphores.
**
**               This function should be used instead of standard I/O 
**               function printf() or similar functions. 
**
** See Also    : TGT_printSyncData(), TGT_printDrift()
**              
** Parameters  : ac_Fmt  (IN) - format string
**               ...     (IN) - unspecified parameters
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void TGT_printf(const CHAR* ac_Fmt, ...);

/***********************************************************************
**
** Function    : TGT_printSyncData
**
** Description : This function prints some informative sync timestamp
**               data with the thread-safe {TGT_printf()} function.
**
** See Also    : TGT_printf(), TGT_printDrift()
**              
** Parameters  : ps_offsUf (IN) - actual measured offset from master
**               ps_offsF  (IN) - actual filtered offset from master
**               ps_mtsdUf (IN) - actual measured master to slave delay
**               ps_mtsdF  (IN) - actual filtered master to slave delay
**               ps_stmdUf (IN) - actual measured slave to master delay
**               ps_stmdF  (IN) - actual filtered slave to master delay
**               ps_owdUf  (IN) - actual measured one way delay
**               ps_owdF   (IN) - actual filtered one way delay
**             
** Returnvalue : -
**                      
** Remarks     : This function must not be modified.
**
***********************************************************************/
void TGT_printSyncData(const PTP_t_TmIntv *ps_offsUf,
                       const PTP_t_TmIntv *ps_offsF,
                       const PTP_t_TmIntv *ps_mtsdUf, 
                       const PTP_t_TmIntv *ps_mtsdF,
                       const PTP_t_TmIntv *ps_stmdUf,
                       const PTP_t_TmIntv *ps_stmdF,
                       const PTP_t_TmIntv *ps_owdUf,
                       const PTP_t_TmIntv *ps_owdF);

/***********************************************************************
**
** Function    : TGT_printDrift
**
** Description : This function prints the actual mean drift in
**               ppb (parts per billion) with the thread-safe
**               {TGT_printf()} function.
**
** See Also    : TGT_printf(), TGT_printSyncData()
**
** Parameters  : ll_phsChgRt (IN) - act phase change rate [ps/sec]
**             
** Returnvalue : -
**                      
** Remarks     : This function must not be modified.
**
***********************************************************************/  
void TGT_printDrift(INT64 ll_phsChgRt);

#ifdef __cplusplus
}
#endif
#endif /* _TGT_H_ */
