/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: TGT.c 
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
**             PrintTi
**
**   Compiler: gcc 3.3.2
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
#include "TGT/TGT.h"

/*************************************************************************
**    global variables
*************************************************************************/
pthread_mutex_t TGT_s_mtx;
BOOLEAN         TGT_o_silent;
BOOLEAN         TGT_o_offs;
BOOLEAN         TGT_o_mtsd;
BOOLEAN         TGT_o_stmd;
BOOLEAN         TGT_o_owd;
BOOLEAN         TGT_o_meanDrft;

/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static void PrintTi(const PTP_t_TmIntv *ps_ti);

/*************************************************************************
**    global functions
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
void TGT_Init( void )
{
  /* initialize mutex for debug-print */
  if( pthread_mutex_init( &TGT_s_mtx,NULL) < 0 )
  {
    printf("\nMutex for thread-safe printing could not be initialized");
  }
}

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
void TGT_Close( void )
{
  /* remove mutex */
  pthread_mutex_destroy(&TGT_s_mtx);/*lint !e534*/
}

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
void TGT_printf(const CHAR* ac_Fmt, ...)
{ 
  va_list p_args;
  /* for silent mode just return */
  if( TGT_o_silent == TRUE )
  {
    return;
  }
  
  /* lock mutex */
  if( pthread_mutex_lock(&TGT_s_mtx) == 0 )
  {
    va_start(p_args,ac_Fmt);/*lint !e534 !e530*/
    /* print */
    if( vprintf(ac_Fmt,p_args) > 0 )
    {
      fflush(stdout);/*lint !e534 */    
    }
    va_end(p_args);
    /* unlock mutex   */
    pthread_mutex_unlock(&TGT_s_mtx);/*lint !e534 */    
  }
}

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
                       const PTP_t_TmIntv *ps_owdF)
{
  if( TGT_o_offs || TGT_o_mtsd || TGT_o_stmd || TGT_o_owd || TGT_o_meanDrft)
  {
    TGT_printf("\n");
  }
#if( k_CLK_DEL_E2E == TRUE )  
  if( TGT_o_offs == TRUE )
  {  
    TGT_printf("ofm (msrd/filt);");
    PrintTi(ps_offsUf);
    TGT_printf("; ");
    PrintTi(ps_offsF); 
  }
  if( TGT_o_mtsd == TRUE )
  {
    TGT_printf(";mtsd (msrd/filt);");
    PrintTi(ps_mtsdUf);
    TGT_printf("; ");
    PrintTi(ps_mtsdF);
  }
  if( TGT_o_stmd == TRUE )
  {
    TGT_printf(";stmd (msrd/filt);");
    PrintTi(ps_stmdUf);
    TGT_printf("; ");
    PrintTi(ps_stmdF);    
  }
  if( TGT_o_owd == TRUE )
  {
    TGT_printf(";owd (msrd/filt);");
    PrintTi(ps_owdUf);
    TGT_printf("; ");
    PrintTi(ps_owdF);        
  }
#else /* #if( k_CLK_DEL_E2E == TRUE ) */
  /* avoid compiler warnings */
  ps_stmdF  = ps_stmdF;
  ps_stmdUf = ps_stmdUf;
  if( TGT_o_offs == TRUE )
  {  
    TGT_printf("ofm (msrd/filt);");
    PrintTi(ps_offsUf);
    TGT_printf("; ");
    PrintTi(ps_offsF);  
  }
  if( TGT_o_mtsd == TRUE )
  {
    TGT_printf(";mtsd (msrd/filt);");
    PrintTi(ps_mtsdUf);
    TGT_printf("; ");
    PrintTi(ps_mtsdF);
  }  
  if( TGT_o_owd == TRUE )
  {
    TGT_printf(";pdel (msrd/filt);");
    PrintTi(ps_owdUf);
    TGT_printf("; ");
    PrintTi(ps_owdF); 
  }
#endif /* #if( k_CLK_DEL_E2E == TRUE ) */
}


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
void TGT_printDrift(INT64 ll_phsChgRt)
{
  if( TGT_o_meanDrft )
  {
    TGT_printf(";pcr;");
    TGT_printf("%lli ",ll_phsChgRt);
    TGT_printf("; ");
  }
}

/*************************************************************************
**    static functions
*************************************************************************/

/***********************************************************************
**
** Function    : PrintTi
**
** Description : Prints a time interval in nanoseconds
**              
** Parameters  : ps_ti (IN) - pointer to time interval to print
**             
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/                       
static void PrintTi(const PTP_t_TmIntv *ps_ti)
{
  INT32 l_nsec;
  INT32 l_sec;
  INT64 ll_nsec = PTP_INTV_TO_NSEC(ps_ti->ll_scld_Nsec);
  l_sec = (INT32)(ll_nsec/k_NSEC_IN_SEC);
  l_nsec= (INT32)(ll_nsec%k_NSEC_IN_SEC);
  if( ll_nsec < 0 )
  {
    TGT_printf(" -%u,%09u",-l_sec, -l_nsec);
  }
  else
  {
    TGT_printf("  %u,%09u",l_sec, l_nsec); 
  }
}

