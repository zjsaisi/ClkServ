/*************************************************************************
**    (C) 2008 - 2009 IXXAT Automation GmbH, all rights reserved
**************************************************************************
**
**       File: MNTmain.c 
**    Summary: This unit represents the main unit of the management
**             system and offers all task functionality for all
**             management messages and management API requests. Each
**             management message, Ethernet messages as well as
**             internal API requests, are passed to the management
**             task. All messages are dispatched to the corresponding
**             functionality. API requests are handled internally as
**             normal messages coming from the Ethernet. Otherwise
**             the API request is for another node. Thus the request
**             is dispatched to the transmitting task. Main data 
**             storage is a global management information table. There
**             the corresponding function, static management message
**             length and an API data structure is assigned to each
**             management id. Therefore the unit has only to determine
**             the table index to a processing management id and could
**             use all information hold within this management
**             information table. 
**
**    Version: 1.01.01
**
**************************************************************************
**************************************************************************
**
**  Functions: MNT_Init
**             MNT_Task
**             MNT_GetPTPText
**             MNT_GetTblIdx
**             MNT_GenPortLoopData
**             MNT_PreInitClear
**             MNT_LockApi
**             MNT_UnlockApi
**             MNT_GetErrStr
**             FreeMMsg
**             HandleApiReq
**             ExTransMMsg
**             IntTransMMsg
**             GenApiClbk
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
#include "SIS/SIS.h"
#include "DIS/DIS.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "MNT/MNT.h"
#include "MNT/MNTapi.h"
#include "MNT/MNTint.h"
#include "GOE/GOE.h"
/*************************************************************************
**    global variables
*************************************************************************/
/* pointer to clock data set */
const PTP_t_ClkDs *MNT_ps_ClkDs;
#if( k_UNICAST_CPBL == TRUE )
/* pointer to unicast master table */
const PTP_t_PortAddrQryTbl *MNT_ps_ucMstTbl;
#endif /* if( k_UNICAST_CPBL == TRUE ) */
/* channel port ids */
PTP_t_PortId MNT_as_pId[k_NUM_IF];
/* number of interfaces */
UINT32 MNT_dw_amntNetIf;

/* management message information table */
MNT_t_MsgInfoTable s_MntInfoTbl[] = {
/*  mnt-id; port/node addressable; tlv-paylod-static-size; API data */
  /* apllicable to all nodes */
  {k_NULL_MANAGEMENT,                    TRUE,   0, NULL},
  {k_CLOCK_DESCRIPTION,                  TRUE,  14, NULL},   
  {k_USER_DESCRIPTION,                   FALSE,  0, NULL},
  {k_SAVE_IN_NON_VOLATILE_STORAGE,       FALSE,  0, NULL},
  {k_RESET_NON_VOLATILE_STORAGE,         FALSE,  0, NULL},
  {k_INITIALIZE,                         FALSE,  2, NULL},
  {k_FAULT_LOG,                          FALSE,  2, NULL},
  {k_FAULT_LOG_RESET,                    FALSE,  0, NULL},
  /* applicable to ordinary and boundary clocks */
  {k_DEFAULT_DATA_SET,                   FALSE, 20, NULL},
  {k_CURRENT_DATA_SET,                   FALSE, 18, NULL},
  {k_PARENT_DATA_SET,                    FALSE, 32, NULL},
  {k_TIME_PROPERTIES_DATA_SET,           FALSE,  4, NULL},
  {k_PORT_DATA_SET,                      TRUE,  26, NULL},
  {k_PRIORITY1,                          FALSE,  2, NULL},
  {k_PRIORITY2,                          FALSE,  2, NULL},
  {k_DOMAIN,                             FALSE,  2, NULL},
  {k_SLAVE_ONLY,                         FALSE,  2, NULL},
  {k_LOG_MEAN_ANNOUNCE_INTERVAL,         TRUE,   2, NULL},
  {k_ANNOUNCE_RECEIPT_TIMEOUT,           TRUE,   2, NULL},
  {k_LOG_MEAN_SYNC_INTERVAL,             TRUE,   2, NULL},
  {k_VERSION_NUMBER,                     TRUE,   2, NULL},
  {k_ENABLE_PORT,                        TRUE,   0, NULL},
  {k_DISABLE_PORT,                       TRUE,   0, NULL},
  {k_TIME,                               FALSE, 10, NULL},
  {k_CLOCK_ACCURACY,                     FALSE,  2, NULL},
  {k_UTC_PROPERTIES,                     FALSE,  4, NULL},
  {k_TRACEABILITY_PROPERTIES,            FALSE,  2, NULL},
  {k_TIMESCALE_PROPERTIES,               FALSE,  2, NULL},
  {k_UNICAST_NEGOTIATION_ENABLE,         TRUE,   2, NULL},
  {k_PATH_TRACE_LIST,                    FALSE,  0, NULL},
  {k_PATH_TRACE_ENABLE,                  FALSE,  2, NULL},
  {k_GRANDMASTER_CLUSTER_TABLE,          FALSE,  2, NULL},
  {k_UNICAST_MASTER_TABLE,               TRUE,   3, NULL},
  {k_UNICAST_MASTER_MAX_TABLE_SIZE,      TRUE,   2, NULL},
  {k_ACCEPTABLE_MASTER_TABLE,            FALSE,  2, NULL},
  {k_ACCEPTABLE_MASTER_TABLE_ENABLED,    TRUE,   2, NULL},
  {k_ACCEPTABLE_MASTER_MAX_TABLE_SIZE,   FALSE,  2, NULL},
  {k_ALTERNATE_MASTER,                   TRUE,   4, NULL},
  {k_ALTERNATE_TIME_OFFSET_ENABLE,       FALSE,  2, NULL},
  {k_ALTERNATE_TIME_OFFSET_NAME,         FALSE,  1, NULL},
  {k_ALTERNATE_TIME_OFFSET_MAX_KEY,      FALSE,  2, NULL},
  {k_ALTERNATE_TIME_OFFSET_PROPERTIES,   FALSE, 16, NULL},
  /* applicable only to transparent clocks */
  {k_TRANSPARENT_CLOCK_DEFAULT_DATA_SET, FALSE, 12, NULL},
  {k_TRANSPARENT_CLOCK_PORT_DATA_SET,    TRUE,  20, NULL},
  {k_PRIMARY_DOMAIN,                     FALSE,  2, NULL},
  /* applicable to ordinary, boundary and transparent clocks */
  {k_DELAY_MECHANISM,                    TRUE,   2, NULL},
  {k_LOG_MIN_MEAN_PDELAY_REQ_INTERVAL,   TRUE,   2, NULL},
  /* Symmetricom private management messages */
  {k_PRIVATE_STATUS,                     TRUE,   1, NULL},
  /* table end dummy item */ 
  {k_MNT_TABLE_END,                      FALSE,  0, NULL}
};

/* number of table entries */
UINT16 w_numTblEntries;

#ifdef MNT_API
/* flag to indicate initializing state during re-init */
BOOLEAN o_initialize = FALSE;
#endif /* ifdef MNT_API */



/*************************************************************************
**    static constants, types, macros, variables
*************************************************************************/

/*************************************************************************
**    static function-prototypes
*************************************************************************/
static void FreeMMsg(PTP_t_MboxMsg *ps_msg);
#ifdef MNT_API
static void HandleApiReq( const MNT_t_apiMboxData *ps_apiMboxData );
static void ExTransMMsg( const MNT_t_apiMboxData *ps_apiMboxData,
                               UINT16            w_seqId);
static BOOLEAN IntTransMMsg( const MNT_t_apiMboxData *ps_apiMboxData,
                                   UINT16            w_seqId );
static void GenApiClbk( const MNT_t_apiClbkData  *ps_clbkData,
                              MNT_t_apiStateEnum e_retState,
                              UINT16             w_mntId );
#endif
/*************************************************************************
**    global functions
*************************************************************************/

/***********************************************************************
** 
** Function    : MNT_Init
**
** Description : This function initializes the unit MNT. Therefore
**               each port id is saved within an array to get simple
**               access to the addresses. The size of the management
**               information table is determined to get the current
**               amount of defined management messages and the end 
**               of the table. Furthermore the user description is
**               generated, witch is configurable through the mnt. 
**
** Parameters  : ps_portId    (IN) - pointer to the channel port id array
**               dw_amntNetIf (IN) - number of initializable net interfaces
**               ps_ucMstTbl  (IN) - pointer to unicast master table
**               ps_clkDs     (IN) - pointer to the clock data set
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNT_Init(const PTP_t_PortId         *ps_portId,
              UINT32                     dw_amntNetIf,
              const PTP_t_PortAddrQryTbl *ps_ucMstTbl,
              const PTP_t_ClkDs          *ps_clkDs)
{
  UINT16 i;
  /* initialize port ids of all channels  */
  for( i = 0 ; i < k_NUM_IF ; i++ )
  {
    MNT_as_pId[i] = ps_portId[i];
  } 
  /* store amount of network interfaces */
  MNT_dw_amntNetIf = dw_amntNetIf;
  /* initialize read/write pointer to clock data set */
  MNT_ps_ClkDs = ps_clkDs;
#if( k_UNICAST_CPBL == TRUE )
  /* initialize unicast master table */
  MNT_ps_ucMstTbl = ps_ucMstTbl;
#else
  /* avoid compiler warning */
  ps_ucMstTbl = ps_ucMstTbl;
#endif /* if( k_UNICAST_CPBL == TRUE ) */
  
  /* initialze number of info table entries */
  i = 0;
  while(s_MntInfoTbl[i].w_mntId != k_MNT_TABLE_END)
  {
    /* reset pointer to MNTapi request */
    s_MntInfoTbl[i].ps_apiData = NULL;
    /* switch to next table item */
    i++;
  }
  w_numTblEntries = i;
  /* restart task timer immediately */
  SIS_TimerStart(MNT_TSK,1); 
  /* initialize fault record queue */
  MNTmsg_InitFltRec();
}

/***********************************************************************
**
** Function    : MNT_Task
**
** Description : The management task is responsible for handling all 
**               received management messages and dispatch it to the
**               corresponding functionality. 
**              
** Parameters  : w_hdl        (IN) - the SIS-task-handle
**
** Returnvalue : -
**                      
** Remarks     : -
**
***********************************************************************/
void MNT_Task(UINT16 w_hdl)
{
  static PTP_t_MboxMsg *ps_msg;
  static UINT32        dw_evnt;
  NIF_t_PTPV2_MntMsg   *ps_mntMsg;
  MNT_t_apiMboxData    *ps_apiMboxData;
#ifdef MNT_API
  MNT_t_apiTblEntry    *ps_apiData;
  MNT_t_apiClbkData    s_apiClbkData;
  UINT16               w_mntId;
  static UINT32        dw_tmo;
#endif /* ifdef MNT_API */
  BOOLEAN              o_searchRes;
  BOOLEAN              o_freeMsg = FALSE;
  UINT16               w_tblIdx;
  /* used for fault log */
  MNT_t_FltRecItem     s_fltRecItem;
  PTP_t_TmStmp         *ps_tsErr;
  /* port definition and loop variables */
  UINT16 w_startIdx;
  UINT16 w_endIdx;
  UINT16 w_portIdx;


  /* multitasking restart point - generates a 
     jump to the last BREAK at env */
  SIS_Continue(w_hdl);
#ifdef MNT_API
    /* calculate timeout */
    dw_tmo = (k_NSEC_IN_SEC/(k_PTP_TIME_RES_NSEC*MNT_k_TIMER_SCALER));
    /* ensure, that the SIS timer runs with at least 1 tick */
    if( dw_tmo < 1 )/*lint !e774*/
    {
      dw_tmo = 1;
    }
    /* start MNT_API timer */
    SIS_TimerStart(w_hdl,dw_tmo);
#endif
  
  /* the tasks main loop   */
  while( TRUE ) /*lint !e716 */
  { 
    /* got ready through event ? */
    dw_evnt = SIS_EventGet(k_EVT_ALL);
    if( dw_evnt != 0 )
    {
      /* no events defined */
    }
    /* received Messages */
    while(( ps_msg = (PTP_t_MboxMsg*)SIS_MboxGet()) != NULL )
    {
      /* get the management message */
      ps_mntMsg = ps_msg->s_pntData.ps_mnt;

      /* error message of the own stack - store in fault log */
      if( ps_msg->e_mType == e_IMSG_NEW_FLT )
      {
        /* get data */
        ps_tsErr  = ps_msg->s_pntData.ps_tmStmp;

        /* check timestamp data */
        if(ps_tsErr != NULL)
        {
          s_fltRecItem.s_tsErr   = *ps_tsErr;
        }
        else
        {
          /* even timestamp is corrupted, generate default ts */
          s_fltRecItem.s_tsErr.dw_Nsec = k_MAX_U32;
          s_fltRecItem.s_tsErr.u48_sec = k_MAX_U64;
        }

        s_fltRecItem.dw_errNmb = ps_msg->s_etc1.dw_u32;
        s_fltRecItem.dw_unitId = ps_msg->s_etc2.dw_u32;
        s_fltRecItem.e_sevCode = ps_msg->s_etc3.e_sevC;

        /* put error into error log */
        MNTmsg_PutFltRec(&s_fltRecItem);

        /* free timestamp */
        if( ps_tsErr != NULL )
        {
          SIS_Free(ps_tsErr);
        }       
      }
      /* received a manegement message or error tlv */
      else if( ps_msg->e_mType == e_IMSG_MM )
      {
        /* discard message with no/wrong data length */
        if( ps_mntMsg->s_mntTlv.w_len >= 2 )
        {
          /* a error status TLV was received */
          if( ps_mntMsg->s_mntTlv.w_type == (UINT16)e_TLVT_MNT_ERR_STS )
          {
            /* search for corresponding table index */
            o_searchRes = MNT_GetTblIdx( 
                                  GOE_ntoh16(&ps_mntMsg->s_mntTlv.pb_data[0]),
                                  &w_tblIdx );
          }
          /* a normal management message was received */
          else
          {
            /* search for corresponding table index */
            o_searchRes = MNT_GetTblIdx( ps_mntMsg->s_mntTlv.w_mntId,
                                         &w_tblIdx );
          }
          /* store table index in etc2 of the message 
             for further usage in the unit MNT */
          ps_msg->s_etc2.w_u16 = w_tblIdx;

          /* ckeck if management id was found */
          if( o_searchRes == TRUE )
          {
            /* check incoming port request:
                 if > 0 and <= amount of interfaces, or all ports */
            if( ((ps_mntMsg->s_trgtPid.w_portNmb > 0) &&
                 (ps_mntMsg->s_trgtPid.w_portNmb <= k_NUM_IF) ) ||
                 (ps_mntMsg->s_trgtPid.w_portNmb == 0xFFFF) )
            {
              /* handle GET requests */
              if( ps_mntMsg->b_actField == k_ACTF_GET )
              {
                /* get loop boundaries */
                MNT_GenPortLoopData(ps_msg, ps_mntMsg, w_tblIdx, 
                                    &w_startIdx, &w_endIdx);
                /* request each addressed port (one or all) */
                for(w_portIdx = w_startIdx; w_portIdx < w_endIdx; w_portIdx++)
                {
                  MNTmsg_HandleGet( ps_msg, ps_mntMsg, w_tblIdx, w_portIdx );
                }
                /* allways free allocated buffer */
                o_freeMsg = TRUE;
              }
              /* handle SET requests */
              else if( ps_mntMsg->b_actField == k_ACTF_SET )
              {
                o_freeMsg = MNTmsg_HandleSet( ps_msg, ps_mntMsg, w_tblIdx);
              }
              /* handle COMMAND requests */
              else if( ps_mntMsg->b_actField == k_ACTF_CMD )
              {
                /* free allocated buffer */
                o_freeMsg = MNTmsg_HandleCom( ps_msg, ps_mntMsg, w_tblIdx );
              }
              /* RESPONSE and ACKNOWLEDGE messages are passed to the API */
              else if( ((ps_mntMsg->b_actField & 0x0F) == k_ACTF_RESP) ||
                       ((ps_mntMsg->b_actField & 0x0F) == k_ACTF_ACK) )
              {
  #ifdef MNT_API
                MNTapi_GatherAnswers( ps_msg, 
                                      ps_mntMsg, 
                                      &ps_mntMsg->s_mntTlv,
                                      w_tblIdx );
  #endif /* ifdef MNT_API */
                /* allways free allocated buffer */
                o_freeMsg = TRUE;
              }
              /* unknown action command */
              else
              {
                /* generate standard error */ 
                MNTmsg_GenStandMMsgErr( ps_msg, 
                                        e_MNTERR_NOTSUPPORT); 
                /* alwayes free message buffer */
                o_freeMsg = TRUE;
              }
            }
            /* wrong port number -> does not exist */
            else
            {
              /* in general discard message - there is nothing to do */
              /* allways free allocated buffer */
              o_freeMsg = TRUE;
            }
          }
          /* no such management id */
          else
          {
            /* generate error message */
            MNTmsg_GenStandMMsgErr(ps_msg,
                                   e_MNTERR_NOSUCHID);
            /* allways free allocated buffer */
            o_freeMsg = TRUE;
          }
        }
        /* corrupted data */
        else
        {
          /* free message - no RESPONSE/ACKNOWLEDGE 
             because of possible missing MNT id */
          o_freeMsg = TRUE;
        }

        /* only, if work is done */
        if( o_freeMsg == TRUE )
        {
          /* free complete message */
          FreeMMsg(ps_msg);
        }
        else
        {
          /* nothing to do - message is used within CTL */
        }
      } /* end if mbox type e_IMSG_MM */

      /* positive response from CTL task */
      else if(ps_msg->e_mType == e_IMSG_MM_RES)
      {
        if( ps_mntMsg->b_actField == k_ACTF_SET )
        {
          /* generate RESPONSE */
          MNTmsg_GenSetResp(ps_msg, ps_mntMsg, ps_msg->s_etc2.w_u16);
        }
        else if( ps_mntMsg->b_actField == k_ACTF_CMD )
        {
          /* send ACKNOWLEDGE */
          MNTmsg_GenAck( ps_msg );
        }
        else
        {
          /* error - should not occur */
        }
        /* free allocated buffers */
        FreeMMsg(ps_msg);
      }
      /* negative response from CTL task */
      else if(ps_msg->e_mType == e_IMSG_MM_ERRRES )
      {
          /* generate error message */
          MNTmsg_SendMMErr(ps_msg, 
                           ps_msg->s_pntExt1.ps_text,
                           e_MNTERR_GENERALERR);  

        /* free allocated buffers */
        FreeMMsg(ps_msg);
      }
      /* API message to external nodes */
      else if( ps_msg->e_mType == e_IMSG_MM_API )
      {
        /* get mbox data */
        ps_apiMboxData = (MNT_t_apiMboxData*)ps_msg->s_pntData.pv_data;
#ifdef MNT_API
        HandleApiReq(ps_apiMboxData);
#endif /* ifdef MNT_API */
        /* free allocated buffers */
        if(ps_apiMboxData->s_mntTlv.pb_data != NULL)
        {
          SIS_Free(ps_apiMboxData->s_mntTlv.pb_data);
        }
        SIS_Free(ps_apiMboxData);
      }
      else
      {
        /* should not occur */
      }

      /* Allocated buffers are release within each switch/case
         statement. Here only the messagebox entry must be
         released */
      SIS_MboxRelease();
    }/* end while message available */

    /* got ready with elapsed timer? */
    if( SIS_TimerStsRead( w_hdl ) == SIS_k_TIMER_EXPIRED )
    {
#ifdef MNT_API
      for(w_tblIdx=0; w_tblIdx<w_numTblEntries; w_tblIdx++)
      {
        /* get table entry */
        ps_apiData = s_MntInfoTbl[w_tblIdx].ps_apiData;
        /* if available */
        if(ps_apiData != NULL)
        {
          /* decrement timeout counter */
          ps_apiData->w_timeout = ps_apiData->w_timeout - 1;
          /* if timeout is reached */
          if(ps_apiData->w_timeout == 0)
          {
            /* get application data */
            s_apiClbkData = ps_apiData->s_clbkData;
            /* get management id */
            w_mntId = s_MntInfoTbl[w_tblIdx].w_mntId;
            /* release management info table entry */
            SIS_Free(ps_apiData);
            s_MntInfoTbl[w_tblIdx].ps_apiData = NULL;
            /* call the callback function and indicate timeout */
            GenApiClbk( &s_apiClbkData, e_MNTAPI_TIMEOUT, w_mntId );
          }
          else
          {
            /* timeout not reached */
          }
        }
        else
        {
          /* no API request available */
        }
      } /* end for loop */

      /* reset timer */
      SIS_TimerStart(w_hdl,dw_tmo);
#else /* ifdef MNT_API */
      /* nothing to do */
#endif /* ifndef MNT_API */
    } /* end got ready through timer */

    /* cooperative multitasking 
       set task to blocked, till it gets ready through an event,
       message or timeout */
    SIS_Break(w_hdl,1); /*lint !e717 !e646*/ 
  }
  SIS_Return(w_hdl,0); /*lint !e744 */
}

/*************************************************************************
**
** Function    : MNT_GetPTPText
**
** Description : Creates a PTP text according an error string for
**               the generation of management status error messages.
**
** Parameters  : pc_string  (IN)  - pointer to error string
**               ps_retText (OUT) - corrsponding error PTP text
**               
** Returnvalue : -       
** 
** Remarks     : -
**  
***********************************************************************/
void MNT_GetPTPText(const CHAR *pc_string,
                    PTP_t_Text *ps_retText)
{
  // jyang: increased length by 1 to include NULL terminator
  ps_retText->b_len   = (UINT8)PTP_BLEN(pc_string) + 1;
  ps_retText->pc_text = pc_string;
  return;
}

/*************************************************************************
**
** Function    : MNT_GetTblIdx
**
** Description : This function is responsible for the table index
**               of the management information table. The return
**               value is the table index of a given management id.
**
** Parameters  : w_mntId    (IN) - management id of searched table
**                                 position
**               pw_tblIdx (OUT) - tabel index
**               
** Returnvalue : TRUE  - successful found table entry
**               FALSE - no table entry of such id found
** 
** Remarks     : -
**  
***********************************************************************/
BOOLEAN MNT_GetTblIdx( UINT16 w_mntId, UINT16 *pw_tblIdx )
{
  BOOLEAN o_searchRes = FALSE;
  UINT16  w_lowIdx;
  UINT16  w_highIdx;
  UINT16  w_midIdx;

  w_highIdx = w_numTblEntries;
  w_lowIdx  = 0;

  while( w_highIdx >= w_lowIdx )
  {
    /* calculate currend mid value */
    w_midIdx = (w_highIdx + w_lowIdx)/2;
    /* searched id is within the first half */
    if(w_mntId < s_MntInfoTbl[w_midIdx].w_mntId)
    {
      w_highIdx = w_midIdx - 1;
    }
    /* searched id is within the second half */
    else if(w_mntId > s_MntInfoTbl[w_midIdx].w_mntId)
    {
      w_lowIdx = w_midIdx + 1;
    }
    /* found management id */
    else
    {
      o_searchRes = TRUE;
      *pw_tblIdx = w_midIdx;
      break;
    }
  }
  return o_searchRes;
}

/***********************************************************************
**
** Function    : MNT_GenPortLoopData
**
** Description : This function determines the start and end index
**               of the port loop, if all ports are addressed.
**
** Parameters  : ps_msg      (IN)  - message box entry
**               ps_mntMsg   (IN)  - received mnt message
**               w_tblIdx    (IN)  - mnt table index
**               pw_startIdx (OUT) - start index of loop
**               pw_endIdx   (OUT) - end index of loop
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNT_GenPortLoopData( const PTP_t_MboxMsg       *ps_msg,
                          const NIF_t_PTPV2_MntMsg  *ps_mntMsg,
                          UINT16                    w_tblIdx,
                          UINT16                    *pw_startIdx,
                          UINT16                    *pw_endIdx )
{
  /* reset port determination values */
  *pw_startIdx = 0;
  *pw_endIdx   = k_NUM_IF;

  /* determine the only one port, if not all ones */
  if(ps_mntMsg->s_trgtPid.w_portNmb != 0xFFFF)              
  {
    *pw_startIdx = ps_mntMsg->s_trgtPid.w_portNmb - 1;
    *pw_endIdx = ps_mntMsg->s_trgtPid.w_portNmb;
  }
  /* if all ports but only clock addressable */
  else if( (s_MntInfoTbl[w_tblIdx].o_allPorts == FALSE) &&
           (ps_mntMsg->s_trgtPid.w_portNmb == 0xFFFF) )
  {
    *pw_startIdx = ps_msg->s_etc1.w_u16;   /* received interface */
    *pw_endIdx   = ps_msg->s_etc1.w_u16+1; /* ending point */
  }
  return;
}

/***********************************************************************
**
** Function    : MNT_PreInitClear
**
** Description : This is a special function only called out of the
**               CTL unit to generate a RESPONSE or ACKNOWLEDGE 
**               shortly before re-initialization of the stack is 
**               done. Thus a response to the requestor is made. 
**               Furthermore all API requests will be cancelled 
**               because some could wait for a timeout.
**
** Parameters  : ps_msg    (IN)  - the mbox item
**               ps_mntMsg (IN)  - mbox data, in this case MNT msg
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
void MNT_PreInitClear(       PTP_t_MboxMsg      *ps_msg,
                       const NIF_t_PTPV2_MntMsg *ps_mntMsg )
{
#ifdef MNT_API
  UINT16 w_tblIdx = 0;
#endif /* #ifdef MNT_API */

  /* check pointers, if message is passed */
  if( (ps_msg != NULL) && (ps_mntMsg != NULL) )
  {
    /* pre-init behaviour of domain change */
    if( (ps_mntMsg->b_actField == k_ACTF_SET) &&
        (ps_mntMsg->s_mntTlv.w_mntId == k_DOMAIN) )
    {
      /* if API request of the same MNT ID available */
      if(s_MntInfoTbl[ps_msg->s_etc2.w_u16].ps_apiData != NULL)
      {
        /* if the own API issued exact this request */
        if(s_MntInfoTbl[ps_msg->s_etc2.w_u16].ps_apiData->w_seqId 
                                         == ps_mntMsg->s_ptpHead.w_seqId)
        {
          /* set one message to TRUE to generate a DONE callback */
          s_MntInfoTbl[ps_msg->s_etc2.w_u16].ps_apiData->o_oneMsg = TRUE;
        }
      }    

      /* generate RESPONSE */
      MNTmsg_GenSetResp(ps_msg, ps_mntMsg, ps_msg->s_etc2.w_u16);
    }
    else
    {
      /* error - should not occur */
    }
  }

#ifdef MNT_API
  /* cancel all API requests by calling the callback
     using the state e_e_MNTAPI_ABORT */
  for( w_tblIdx = 0; 
      (s_MntInfoTbl[w_tblIdx].w_mntId != k_MNT_TABLE_END); w_tblIdx++ )
  {
    /* if table entry for API available */
    if( s_MntInfoTbl[w_tblIdx].ps_apiData != NULL )
    {
      /* cancel request */
      GenApiClbk( &s_MntInfoTbl[w_tblIdx].ps_apiData->s_clbkData, 
                  e_MNTAPI_ABORT, 
                  s_MntInfoTbl[w_tblIdx].w_mntId  );
    }
  }
#endif /* #ifdef MNT_API */
  /* return to CTL to do the re-init */
  return;
}

/*************************************************************************
**
** Function    : MNT_LockApi
**
** Description : This function sets the BOOLEAN flag o_initialize to 
**               TRUE to indicate, that the stack is in re-
**               initialization. There could be a problem using the API
**               and thus we prohibit the API to request a management 
**               message during this time. As soon as the re-
**               initialization is done, the flag must be reset to 
**               FALSE using MNT_UnlockApi().
**
** See Also    : MNT_UnlockApi()
**
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
#ifdef MNT_API
void MNT_LockApi( void )
{
  o_initialize = TRUE;
}
#endif /* #ifdef MNT_API */

/*************************************************************************
**
** Function    : MNT_UnlockApi
**
** Description : This function resets the BOOLEAN flag o_initialize 
**               to FALSE to indicate, that the stack is not in re-
**               initialization any more. There could be a problem 
**               using the API and thus we prohibit the API to request 
**               a management message during this time.
**
** See Also    : MNT_LockApi()
**
** Parameters  : -
**               
** Returnvalue : -
** 
** Remarks     : -
**  
***********************************************************************/
#ifdef MNT_API
void MNT_UnlockApi( void )
{
  o_initialize = FALSE;
}
#endif /* #ifdef MNT_API */

#ifdef ERR_STR
/*************************************************************************
**
** Function    : MNT_GetErrStr
**
** Description : Resolves the error number to the according error string
**
** Parameters  : dw_errNmb (IN) - error number
**               
** Returnvalue : const CHAR*   - pointer to error string
** 
** Remarks     : Gets compiled with definition of ERR_STR.
**  
***********************************************************************/
const CHAR* MNT_GetErrStr(UINT32 dw_errNmb)
{
  const CHAR *pc_ret;
  /* return according error string */
  switch(dw_errNmb)
  {
#ifdef MNT_API
    case MNTAPI_k_ERR_UNKN_ACOM:
    {
      pc_ret = "Unknown action command";
      break;
    }
    case MNTAPI_k_ERR_UNKN_MNTID:
    {
      pc_ret = "Unkown management id";
      break;
    }
    case MNTAPT_k_ERR_ZERO_PORT:
    {
      pc_ret = "Port zero is not addressable";
      break;
    }
    case MNTAPI_k_ERR_WRONG_VALUE:
    {
      pc_ret = "One or more parameters got wrong value";
      break;
    }
    case MNTAPI_k_ERR_STILL_PEND:
    {
      pc_ret = "Another request is still pending";
      break;
    }
    case MNTAPI_k_ERR_ALLOC:
    {
      pc_ret = "Couldn't allocate buffer to handle request";
      break;
    }
    case MNTAPI_k_ERR_INT_TRANS:
    {
      pc_ret ="Failure on transmission to the own node";
      break;
    }
    case MNTAPI_k_ERR_EXT_TRANS:
    {
      pc_ret = "Failure on transmission to remote node";
      break;
    }
    case MNTAPI_k_ERR_NO_CLBK:
    {
      pc_ret = "No callback function registered";
      break;
    }
    case MNTAPI_k_ERR_NO_TBL_DATA:
    {
      pc_ret = "Could not find any table entry - should not occur";
      break;
    }
#endif /* ifdef MNT_API */
    case MNTAPI_k_ERR_INTREQ:
    {
      pc_ret = "API request, although MNT_API not defined";
      break;
    }
    default:
    {
      pc_ret = "Unknown error";
      break;
    }
  }
  return pc_ret;
}
#endif

/*************************************************************************
**    static functions
*************************************************************************/

/*************************************************************************
**
** Function    : FreeMMsg
**
** Description : This function gets a pointer to a message box item
**               including a complete management message. This could
**               be either a message from the API, a message received
**               via the Ethernet or received from another task as a
**               response to a SET or COMMAND request. This function
**               determines allocated buffer and releases all resources.
**
** Parameters  : ps_msg (IN) - pointer to the MNT mbox item to free
**               
** Returnvalue : -        
** 
** Remarks     : -
**  
***********************************************************************/
static void FreeMMsg(PTP_t_MboxMsg *ps_msg)
{
  NIF_t_PTPV2_MntMsg* ps_mntMsg;

  /* get the management message */
  ps_mntMsg = ps_msg->s_pntData.ps_mnt;

  /* free additional buffer normaly used for display data*/
  if(ps_msg->s_pntExt1.pv_data != NULL)
  {
    SIS_Free(ps_msg->s_pntExt1.pv_data);
  }

  /* free additinal buffer normaly used for unicast addesse */
  if(ps_msg->s_pntExt2.pv_data != NULL)
  {
    SIS_Free(ps_msg->s_pntExt2.pv_data);
  }

  /* free MNT TLV payload */
  if(ps_mntMsg->s_mntTlv.pb_data != NULL)
  {
    SIS_Free(ps_mntMsg->s_mntTlv.pb_data);
  }

  /* than free MNT message */
  SIS_Free(ps_mntMsg);
  return;
} /*lint !e818 */


#ifdef MNT_API
/*******************************************************************************
**
** Function    : HandleApiReq
**
** Description : Upon call of an API function, a management message is
**               placed within the MNT task. This function handles all 
**               incoming API requests. For each request a table entry is
**               generated, but only, if no other request is still pending.
**               A positive request, therefore no pending request, generates
**               some general data like the sequence id and transmits the 
**               request to the own node as well as remote nodes, depending
**               on the target.
**
** Parameters  : ps_apiMboxData  (IN) - direct pointer mbox entry
**
** Returnvalue : -
**
** Remark      : -
**
*******************************************************************************/
static void HandleApiReq( const MNT_t_apiMboxData *ps_apiMboxData )
{
  MNT_t_apiTblEntry  *ps_apiTblData;
  UINT16             w_tblIdx;
  BOOLEAN            o_ownNode     = FALSE;
  BOOLEAN            o_remoteNodes = FALSE;
  /* sequence ids for all interfaces */
  static UINT16      w_mntSeqId = 0;

  /* determine table index */
  if( MNT_GetTblIdx( ps_apiMboxData->s_mntTlv.w_mntId, &w_tblIdx ) == TRUE )
  {
    /* check, if no request is pending */
    if( s_MntInfoTbl[w_tblIdx].ps_apiData == NULL )
    {
      /* allocate new MNT api table entry */
      ps_apiTblData = (MNT_t_apiTblEntry*)SIS_Alloc(sizeof(MNT_t_apiTblEntry));
      if( ps_apiTblData != NULL )
      {
        /* save new table entry */
        s_MntInfoTbl[w_tblIdx].ps_apiData = ps_apiTblData;
        /* get new sequence id */
        ps_apiTblData->w_seqId     = w_mntSeqId++;
        /* copy callback data */
        ps_apiTblData->s_clbkData  = ps_apiMboxData->s_clbkData;
        /* set state to pending */
        ps_apiTblData->e_reqState  = e_MNTAPI_PENDING;

        /* determine timeout */
        if( ps_apiMboxData->s_msgConfig.b_timeout < 1 )
        {
          /* set default timeout counter */
          ps_apiTblData->w_timeout = (UINT16)(MNT_k_TIMER_SCALER);
        }
        else
        {
          /* set timeout counter */
          ps_apiTblData->w_timeout = (UINT16)(MNT_k_TIMER_SCALER 
                                    * ps_apiMboxData->s_msgConfig.b_timeout);
        }

        /* determine target and set one message flag */
        DIS_WhoIsReceiver( &ps_apiMboxData->s_msgConfig.s_desPortId, 
                           &o_ownNode, 
                           &o_remoteNodes,
                           &ps_apiTblData->o_oneMsg );

        /* request must be transmitted to the own node */
        if( o_ownNode == TRUE )
        {
          /* generate a management request to the own node */
          if( IntTransMMsg( ps_apiMboxData, 
                            ps_apiTblData->w_seqId ) == FALSE )
          {
            /* error: allocation error of mnt tably entry */
            GenApiClbk( &ps_apiMboxData->s_clbkData,
                        e_MNTAPI_ERR_AL,
                        ps_apiMboxData->s_mntTlv.w_mntId );
          }
        }

        /* request must be transmitted to other nodes */
        if( (o_remoteNodes == TRUE) )
        {
          /* generate a management request on all ports */
          ExTransMMsg( ps_apiMboxData, ps_apiTblData->w_seqId );

        }
      }
      else
      {
        /* error: allocation error of mnt tably entry */
        GenApiClbk( &ps_apiMboxData->s_clbkData,
                    e_MNTAPI_ERR_AL,
                    ps_apiMboxData->s_mntTlv.w_mntId );
      }
    }
    else
    {
      /* error: another request of that MNT ID is pending */
      GenApiClbk( &ps_apiMboxData->s_clbkData,
                  e_MNTAPI_ERR_OR,
                  ps_apiMboxData->s_mntTlv.w_mntId );
    }
  }
  return;
}

/***********************************************************************
**
** Function    : ExTransMMsg
**
** Description : This function is called to transmit a management
**               API request to an external node. Minor values are
**               added to the general call.
**
** Parameters  : ps_apiMboxData  (IN) - direct pointer mbox entry
**               w_seqId         (IN) - sequence id of the message
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
static void ExTransMMsg( const MNT_t_apiMboxData *ps_apiMboxData,
                               UINT16            w_seqId)
{
  UINT16 w_ifIdx;

  /* send management message through all ports */
  for(w_ifIdx=0; w_ifIdx<MNT_dw_amntNetIf; w_ifIdx++)
  { 
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE ))
   /* ckeck, if port active */
    if( !((MNT_ps_ClkDs->as_PortDs[w_ifIdx].e_portSts == e_STS_DISABLED) || 
          (MNT_ps_ClkDs->as_PortDs[w_ifIdx].e_portSts == e_STS_FAULTY)) )
    {
#endif
      /* send message */
      DIS_Send_MntMsg( ps_apiMboxData->s_msgConfig.ps_pAddr,
                       &ps_apiMboxData->s_msgConfig.s_desPortId,
                       w_seqId,
                       &ps_apiMboxData->s_mntTlv,
                       w_ifIdx,
                       ps_apiMboxData->s_msgConfig.b_startBoundHops,
                       ps_apiMboxData->s_msgConfig.b_startBoundHops,
                       ps_apiMboxData->s_msgConfig.b_actionFld );
#if( (k_CLK_IS_OC == TRUE) || (k_CLK_IS_BC == TRUE ))
    }
#endif
  }

  return;
}

/***********************************************************************
**
** Function    : IntTransMMsg
**
** Description : This function handles GET, SET and COMMAND
**               managements requests to the own node. A full
**               management massage is generated including the PTP
**               header. All needed information is copied to the
**               message, which than will be re-transmitted to the
**               MNT_Task via the Mbox as an internal management
**               massage.
**
** Parameters  : ps_apiMboxData  (IN) - direct pointer mbox entry
**               w_seqId         (IN) - sequence id of the message
**
** Returnvalue : TRUE  - no failure occured
**               FALSE - a failure occured
**
** Remarks     : -
**
***********************************************************************/
static BOOLEAN IntTransMMsg( const MNT_t_apiMboxData *ps_apiMboxData,
                                   UINT16            w_seqId )
{ 
  NIF_t_PTPV2_MntMsg *ps_mntMsg;
  PTP_t_MboxMsg      s_msg;
  UINT8              *pb_tlvPld = NULL;
  BOOLEAN            o_ret = TRUE;

  ps_mntMsg = (NIF_t_PTPV2_MntMsg*)SIS_Alloc(sizeof(NIF_t_PTPV2_MntMsg));
  if( ps_mntMsg != NULL )
  {
    /* if TLV payload available */
    if( ps_apiMboxData->s_mntTlv.pb_data != NULL )
    {
      pb_tlvPld = (UINT8*)(SIS_Alloc(ps_apiMboxData->s_mntTlv.w_len-2));
      if( pb_tlvPld != NULL )
      {
        /* copy tlv payload into buffer */
        PTP_BCOPY( pb_tlvPld,         
                   ps_apiMboxData->s_mntTlv.pb_data, 
                   ps_apiMboxData->s_mntTlv.w_len-2 );     
      }
      else
      {
        /* error: could not allocate buffer for tlv payload */
        o_ret = FALSE;
      }
    }
 
    /* only, if no TLV payload or copied */
    if( o_ret  == TRUE )
    {
      /* copy all needed management information */
      ps_mntMsg->b_actField      = ps_apiMboxData->s_msgConfig.b_actionFld;
      ps_mntMsg->b_strtBndrHops  = ps_apiMboxData->s_msgConfig.b_startBoundHops;
      ps_mntMsg->b_bndrHops      = ps_apiMboxData->s_msgConfig.b_startBoundHops;
      ps_mntMsg->s_trgtPid       = ps_apiMboxData->s_msgConfig.s_desPortId;

      /* copy all needed PTP head information */

      /* Following parameters are not used */
      /* ps_mntMsg->s_ptpHead.b_dmnNmb         = ?; */
      /* ps_mntMsg->s_ptpHead.b_versPTP        = ?; */
      /* ps_mntMsg->s_ptpHead.c_logMeanMsgIntv = ?; */
      /* ps_mntMsg->s_ptpHead.e_ctrl           = ?; */
      /* ps_mntMsg->s_ptpHead.e_msgType        = ?; */
      /* ps_mntMsg->s_ptpHead.ll_corField      = ?; */
      /* ps_mntMsg->s_ptpHead.w_flags          = ?; */
      /* ps_mntMsg->s_ptpHead.w_msgLen         = ?; */
      ps_mntMsg->s_ptpHead.w_seqId = w_seqId;

        /* set source port identity */
#if( k_CLK_IS_TC == TRUE )
        ps_mntMsg->s_ptpHead.s_srcPortId.s_clkId = 
                                  MNT_ps_ClkDs->s_TcDefDs.s_clkId;
#else /* #if( k_CLK_IS_TC == TRUE ) */
        ps_mntMsg->s_ptpHead.s_srcPortId.s_clkId = 
                                  MNT_ps_ClkDs->s_DefDs.s_clkId;
#endif /* #if( k_CLK_IS_TC == TRUE ) */
      ps_mntMsg->s_ptpHead.s_srcPortId.w_portNmb = 1;

      /* copy TLV data */
      ps_mntMsg->s_mntTlv.pb_data = pb_tlvPld;
      ps_mntMsg->s_mntTlv.w_len   = ps_apiMboxData->s_mntTlv.w_len;
      ps_mntMsg->s_mntTlv.w_mntId = ps_apiMboxData->s_mntTlv.w_mntId;
      ps_mntMsg->s_mntTlv.w_type  = ps_apiMboxData->s_mntTlv.w_type;

      /* set mbox-entry parameters */
      s_msg.s_etc1.dw_u32     = 0;             /* interface index */
      s_msg.s_etc2.dw_u32     = 0;             /* not used */
      s_msg.s_etc3.dw_u32     = k_MNT_MSG_INT; /* intern or extern request? */
      s_msg.s_pntData.ps_mnt  = ps_mntMsg;     /* PTP MNT msg buffer */
      s_msg.s_pntExt1.pv_data = NULL;          /* not used */
      s_msg.s_pntExt2.pv_data = NULL;          /* unicast addr. - not used */
      s_msg.e_mType           = e_IMSG_MM;     /* MNT mbox message */

      /* send to management task */
      if( SIS_MboxPut(MNT_TSK,&s_msg) == FALSE )
      {
        if( ((NIF_t_PTPV2_MntMsg*)ps_mntMsg)->s_mntTlv.pb_data != NULL )
        {
          /* free TLV data */
          SIS_Free(((NIF_t_PTPV2_MntMsg*)ps_mntMsg)->s_mntTlv.pb_data);
        }
        /* free generated MNT message */
        SIS_Free(ps_mntMsg);
        o_ret = FALSE;
      }
    }
  }
  else
  {
    /* error: could not create a management message */
    o_ret = FALSE;
  }
  return o_ret;
}

/***********************************************************************
**
** Function    : GenApiClbk
**
** Description : The callback function defined within the parameter
**               list is called due to a state change without any
**               management message received. A general TLV is passed 
**               to the callback function to indicate the management 
**               id and clock id. Furthermore the state is passed to 
**               indicate the state change.
**
** Parameters  : ps_clbkData   (IN) - pointer to callback infos
**               e_retState    (IN) - the new state
**               w_mntId       (IN) - mnt-id issuing this function
**
** Returnvalue : -
**
** Remarks     : -
**
***********************************************************************/
static void GenApiClbk( const MNT_t_apiClbkData  *ps_clbkData,
                              MNT_t_apiStateEnum e_retState,
                              UINT16             w_mntId )
{
  MNT_t_apiReturnTLV s_retTlv;

  /* generate a return TLV with for error */
#if( k_CLK_IS_TC == TRUE )
    s_retTlv.s_srcPortId.s_clkId = MNT_ps_ClkDs->s_TcDefDs.s_clkId;
#else /* #if( k_CLK_IS_TC == TRUE ) */
    s_retTlv.s_srcPortId.s_clkId = MNT_ps_ClkDs->s_DefDs.s_clkId;
#endif /* #if( k_CLK_IS_TC == TRUE ) */
  s_retTlv.s_srcPortId.w_portNmb = 1;
  s_retTlv.w_sizeOfTlvPld = 0;
  s_retTlv.s_mntTlv.pb_data = NULL;
  s_retTlv.s_mntTlv.w_len   = 2;
  s_retTlv.s_mntTlv.w_mntId = w_mntId;
  s_retTlv.s_mntTlv.w_type  = (UINT16)e_TLVT_MNT;

  /* callback call only if no NULL pointer */
  if( (ps_clbkData != NULL) && ( ps_clbkData->pf_clbkFunc != NULL ) )
  {
    ps_clbkData->pf_clbkFunc( ps_clbkData->pdw_cbHandle,
                              &s_retTlv,
                              e_retState );
  }
  return;
}
#endif /* ifdef MNT_API */

