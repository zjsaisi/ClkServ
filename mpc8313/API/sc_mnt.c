
/*****************************************************************************
*                                                                            *
*   Copyright (C) 2009 Symmetricom, Inc., all rights reserved                *
*                                                                            *
*   This software contains proprietary and confidential information of       *
*   Symmetricom.                                                             *
*   It may not be reproduced, used, or disclosed to others for any purpose   *
*   without the written authorization of Symmetricom.                        *
*                                                                            *
******************************************************************************

FILE NAME    : sc_mnt.c

AUTHOR       : Ken Ho

DESCRIPTION  : 

The function in this file operate as a shim layer to bring the IXXAT management
commands out to the api interface:
   SC_PTP_ClkDesc()
   SC_PTP_UsrDesc()
   SC_PTP_SaveNVolStor()
   SC_PTP_RstNVolStor()
   SC_PTP_Initialize()
   SC_PTP_FaultLog()
   SC_PTP_FaultLogReset()
   SC_PTP_DefaultDS()
   SC_PTP_CurrentDS()
   SC_PTP_ParentDS()
   SC_PTP_TimePropDS()
   SC_PTP_PortDS()
   SC_PTP_Priority1()
   SC_PTP_Priority2()
   SC_PTP_Domain()
   SC_PTP_SlaveOnly()
   SC_PTP_AnncIntv()
   SC_PTP_AnncRxTimeout()
   SC_PTP_SyncIntv()
   SC_PTP_VersionNumber()
   SC_PTP_EnaPort()
   SC_PTP_DisPort()
   SC_PTP_Time()
   SC_PTP_ClkAccuracy()
   SC_PTP_UTCProp()
   SC_PTP_TraceProp()
   SC_PTP_TimeSclProp()
   SC_PTP_UcNegoEnable()
   SC_PTP_UCMasterTbl()
   SC_PTP_UCMstMaxTblSize()
   SC_PTP_AcceptMstTbl()
   SC_PTP_AcceptMstTblEna()
   SC_PTP_AcceptMstTblSize() 
   SC_PTP_AltMaster()

Revision control header:
$Id: API/sc_mnt.c 1.7 2010/03/15 13:33:39PDT jyang Exp  $

******************************************************************************
*/



/*--------------------------------------------------------------------------*/
/*                         INCLUDE FILES                                    */
/*--------------------------------------------------------------------------*/
#include <semaphore.h>

#include "sc_types.h"
#include "sc_api.h"

#include "target.h"
#include "PTP/PTPdef.h"
#include "NIF/NIF.h"
#include "PTP/PTP.h"
#include "MNT/MNTapi.h"


//#include "PTP/PTPint.h"
//#include "SIS/SIS.h"
//#include "GOE/GOE.h"

//#include "MNT/MNT.h"
//#include "MNT/MNTint.h"
#include "CTL/CTLint.h"

/*--------------------------------------------------------------------------*/
/*                              defines                                     */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              types                                       */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                              data                                        */
/*--------------------------------------------------------------------------*/
static sem_t semMnt;
static pthread_mutex_t mutexMnt;
static BOOLEAN o_initDone = FALSE;
static PTP_t_MntTLV s_tlvData;
static UINT8 b_tlvData[k_MAX_TLV_PLD_LEN];
static MNT_t_apiStateEnum e_localReqState;

/*--------------------------------------------------------------------------*/
/*                              functions                                   */
/*--------------------------------------------------------------------------*/
static void Mnt_Return( UINT32* pdw_cbHandle,
                             MNT_t_apiReturnTLV *ps_rtnTlv,
                             MNT_t_apiStateEnum e_reqState )
{
   e_localReqState = e_reqState;

//   printf("\nReturn code: %d ", e_reqState);

   if(e_reqState == e_MNTAPI_DONE)
   {
/* copy in type, length, management id.  Then data buffer */ 
      s_tlvData = ps_rtnTlv->s_mntTlv;
      s_tlvData.pb_data = b_tlvData;

//  printf("type: %hd length: %hd management id: %hx\n", 
//    ps_rtnTlv->s_mntTlv.w_type,   /* tlv type */
//    ps_rtnTlv->s_mntTlv.w_len,    /* length of referenced array */
//    ps_rtnTlv->s_mntTlv.w_mntId  /* management id */
//  );
/* copy tlv data, because data will be freed after return from this 
* function.  If 2 or less, then there is no data 
*/
      if(ps_rtnTlv->s_mntTlv.w_len > 2)
      {
         memcpy(b_tlvData, ps_rtnTlv->s_mntTlv.pb_data, ps_rtnTlv->s_mntTlv.w_len);
      }
   }

/* post the semaphore */
	sem_post(&semMnt);
  	return;
}

/*
----------------------------------------------------------------------------
                                processRtn()

Description : 
This function will take the local tlv and store it into the tlv pointer.

Parameters  : ps_Tlv  (IN) - address of tlv to store the data

Returnvalue : -1  - UnSuccessful
               0 - Successful
-----------------------------------------------------------------------------
*/
static int processRtn(SC_t_MntTLV *ps_Tlv)
{
   int o_rtnFail = -1;

//   printf("e_localReqState: %d\n", (int)e_localReqState);

/* pass the data from local to user's data space */
   if(e_localReqState == e_MNTAPI_DONE)
   {
/* copy in type, length, management id.  Then data buffer */ 
      memcpy((void *)ps_Tlv, (void *)&s_tlvData, sizeof(s_tlvData));

/* take off 2 bytes from the payload because of 2 extra bytes in 
* IXXAT tlv for management id. Doug adding the 2 back.
*/
//      ps_Tlv->w_len = s_tlvData.w_len;

/* copy tlv data */
      if(s_tlvData.w_len <= k_MAX_TLV_PLD_LEN)
      {
         memcpy(ps_Tlv->pb_data, s_tlvData.pb_data, s_tlvData.w_len);
         o_rtnFail = 0; 
      }
   }

/* check to see if tlv is management error status message (type 2) */
   if(ps_Tlv->w_type == e_TLVT_MNT_ERR_STS)
   {
      o_rtnFail = -3;
   }

   return o_rtnFail;
}

/*
----------------------------------------------------------------------------
                                BuildCall()

Description : 
This function will stuff the message configuration structure and the Data
structure with information that is in all slave only implementations.

Parameters  : ps_msgConfig  (IN) - msg addresses & config
              ps_clbkData   (IN) - status and callback info

Returnvalue : N/A
-----------------------------------------------------------------------------
*/
static void Build_Call(   
   MNT_t_apiMsgConfig *ps_msgConfig,
   MNT_t_apiClbkData *ps_clbkData
)
{
/* setup Config structure */
   ps_msgConfig->ps_pAddr = NULL;
   ps_msgConfig->b_startBoundHops = 1;
   ps_msgConfig->b_timeout        = 2;

/* set port ID (Clock ID, port num) */
   ps_msgConfig->s_desPortId = CTL_s_comIf.as_pId[0];

/* setup Data structure */
   ps_clbkData->pdw_cbHandle = NULL; 

}

/*
----------------------------------------------------------------------------
                                SC_InitMnt()

 Description : This function initializes the semaphores used in managing
the management messages. 

Parameters  : ps_msgConfig  (IN) - msg addresses & config
               ps_clbkData   (IN) - status and callback info

Returnvalue : 0  - function succeeded
              -1 - an error occurred
-----------------------------------------------------------------------------
*/
int SC_InitMnt(void)
{
   if(pthread_mutex_init(&mutexMnt, NULL))
   {
      return -1;
   }

   if (sem_init(&semMnt, 0, 0) < 0)
   {
      pthread_mutex_destroy(&mutexMnt);
      return -1;
   }

   o_initDone = TRUE;
   return 0;
}


/*
----------------------------------------------------------------------------
                                SC_CloseMnt()

 Description : This function destroys the semaphores used in managing
the management messages. 

Parameters  : ps_msgConfig  (IN) - msg addresses & config
               ps_clbkData   (IN) - status and callback info

Returnvalue : 0  - function succeeded
              -1 - an error occurred
-----------------------------------------------------------------------------
*/
int SC_CloseMnt(void)
{
   o_initDone = FALSE;

   if(pthread_mutex_destroy(&mutexMnt))
      return -1;
   if(sem_destroy(&semMnt))
      return -1;

   return 0;
}


/*
----------------------------------------------------------------------------
                                SC_PTP_ClkDesc()

Description: This function handles the management message 
             CLOCK_DESCRIPTION. It handles the physical and 
             implementation specific descriptions of the node 
             and the requested port (interface). A GET message 
             prompts a response including all information data. 
             The only supported action command is GET. 

Parameters: 

Inputs
        b_operation  
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.1.2 
-----------------------------------------------------------------------------
*/
int SC_PTP_ClkDesc(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV *ps_Tlv
)
{

   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }

   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_ClkDesc(&s_msgConfig, &s_clbkData);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
}  


/*
----------------------------------------------------------------------------
                                SC_PTP_UsrDesc()

Description: (GET,SET) This function handles the management message
             USER_DESCRIPTION. The user description defines the
             name and physical location of the device, described
             in a PTP text profile. GET and SET commands are both
             supported; however, the additional data parameter is
             only used with a SET command. 

Parameters: 

Input
        b_operation  
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration
                e_PARAM_SET - Set the current log configuration

        ps_userDesc
        On set, pointer to user descriiption

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.1.2 
-----------------------------------------------------------------------------
*/
int SC_PTP_UsrDesc(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV *ps_Tlv, 
   SC_t_Text *ps_userDesc
)
{
   int o_rtnFail = -1;;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
      s_msgConfig.b_actionFld = k_ACTF_SET;
      break;   
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_UsrDesc(&s_msgConfig, &s_clbkData, (PTP_t_Text *)ps_userDesc);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
}  


/*
----------------------------------------------------------------------------
                                SC_PTP_DefaultDS()

Description: This function handles the management message
             DEFAULT_DATA_SET. This management message prompts
             a response including the default data set members.
             The only supported action command is GET. 

Parameters: 

Input
        b_operation  
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Outout
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.3.1 
-----------------------------------------------------------------------------
*/
int SC_PTP_DefaultDs(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV *ps_Tlv
)
{
   int o_rtnFail = -1;;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_DefaultDs(&s_msgConfig, &s_clbkData);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
}  


/*
----------------------------------------------------------------------------
                                SC_PTP_CurrentDs()

Description: This function handles the management message
             CURRENT_DATA_SET. This management message prompts
             a response including the current data set members.
             The only supported action command is GET. 

Parameters: 

Input
        b_operation  
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.4.1
-----------------------------------------------------------------------------
*/
int SC_PTP_CurrentDs(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV *ps_Tlv
)
{
   int o_rtnFail = -1;;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_CurrentDs(&s_msgConfig, &s_clbkData);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
}


/*
----------------------------------------------------------------------------
                                SC_PTP_ParentDs()

Description: This function handles the management message
             PARENT_DATA_SET. This management message prompts
             a response including the parent data set members.
             The only supported action command is GET. 

Parameters: 

Input
        b_operation  
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.5.1
-----------------------------------------------------------------------------
*/
int SC_PTP_ParentDs(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV *ps_Tlv
)
{
   int o_rtnFail = -1;;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_ParentDs(&s_msgConfig, &s_clbkData);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
}


/*
----------------------------------------------------------------------------
                                SC_PTP_TimePropDs()

Description: This function handles the management message
             TIME_PROPERTIES_DATA_SET. This management message
             prompts a response including the time properties 
             data set members. The only supported action command
             is GET. 

Parameters: 

Input
        b_operation  
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.6.1
-----------------------------------------------------------------------------
*/
int SC_PTP_TimePropDs(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV *ps_Tlv
)
{
   int o_rtnFail = -1;;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_TimePropDs(&s_msgConfig, &s_clbkData);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
}


/*
----------------------------------------------------------------------------
                                SC_PTP_PortDs()

Description: This function handles the management message
             PORT_DATA_SET. This management message prompts a
             response including the port data set members. If
             addressed to all ports, a response for each port
             must be issued. The only supported action command
             is GET. 

Parameters: 

Input
        b_operation  
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.7.1
-----------------------------------------------------------------------------
*/
int SC_PTP_PortDs(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV *ps_Tlv
)
{
   int o_rtnFail = -1;;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_PortDs(&s_msgConfig, &s_clbkData);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
}


/*
----------------------------------------------------------------------------
                                SC_PTP_Domain()

Description: This function handles the management message
              DOMAIN. This message gets or updates the domain
              member of the default data set. The GET and SET 
              commands are both supported; however, the additional
              data parameter (b_domain) is only used with a SET
              command. 

Parameters: 
        b_operation  
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration
                e_PARAM_SET - Set the current log configuration

        b_domain
        On set, contains domain value
Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.3.4
-----------------------------------------------------------------------------
*/
int SC_PTP_Domain(t_paramOperEnum b_operation, SC_t_MntTLV *ps_Tlv, UINT8  b_domain)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
      s_msgConfig.b_actionFld = k_ACTF_SET;
      break;   
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_Domain(&s_msgConfig, &s_clbkData, b_domain);
/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
}  


/*
----------------------------------------------------------------------------
                                SC_PTP_AnncIntv()

Description: This function handles the management message
             LOG_ANNOUNCE_INTERVAL. This message gets or updates
             the announce interval member of the port data set.
             If a request is addressed to all ports, a response 
             message for each port must be issued. The GET and
             SET commands are both supported; however, the 
             additional data parameter (c_anncIntv) is only used
             with a SET command. 

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration
                e_PARAM_SET - Set the current log configuration

        c_anncIntv
        On set, contains value of the announce interval
Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.7.2
-----------------------------------------------------------------------------
*/
int SC_PTP_AnncIntv(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV *ps_Tlv, 
   INT8 c_anncIntv
)
{
   int o_rtnFail = -1;;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
      s_msgConfig.b_actionFld = k_ACTF_SET;
      break;   
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_AnncIntv(&s_msgConfig, &s_clbkData, c_anncIntv);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
}  


/*
----------------------------------------------------------------------------
                                SC_PTP_AnncRxTimeout()

Description: This function handles the management message 
             ANNOUNCE_RECEIPT_TIMEOUT. This message gets or 
             updates the announce receipt timeout member of 
             the port data set. If a request is addressed to 
             all ports, a response message for each port must 
             be issued. The GET and SET commands are both 
             supported; however, the additional data parameter
             (b_anncTOut) is only used with a SET command. 

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration
                e_PARAM_SET - Set the current log configuration

        b_anncTOut
        On set, contains value of the announce receipt timeout

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.7.3
-----------------------------------------------------------------------------
*/
int SC_PTP_AnncRxTimeout(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV    *ps_Tlv, 
   UINT8           b_anncTOut
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
      s_msgConfig.b_actionFld = k_ACTF_SET;
      break;   
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_AnncRxTimeout(&s_msgConfig, &s_clbkData, b_anncTOut);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
}  


/*
----------------------------------------------------------------------------
                                SC_PTP_SyncIntv()

Description: This function handles the management message 
             LOG_SYNC_INTERVAL. This message gets or updates 
             the sync interval member of the port data set. 
             If a request is addressed to all ports, a 
             response message for each port must be issued. 
             The GET and SET commands are both supported; 
             however, the additional data parameter 
             (c_syncIntv) is only used with a SET command. 

Parameters: 
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration
                e_PARAM_SET - Set the current log configuration

        c_syncIntv
        On set, contails value of sync interval
Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.7.4
-----------------------------------------------------------------------------
*/
int SC_PTP_SyncIntv(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV    *ps_Tlv, 
   INT8           c_syncIntv
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
      s_msgConfig.b_actionFld = k_ACTF_SET;
      break;   
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_SyncIntv(&s_msgConfig, &s_clbkData, c_syncIntv);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_VersionNumber()

Description: This function handles the management message 
             VERSION_NUMBER. This message gets or updates the
             version number member of the port data set. If a 
             request is addressed to all ports, a response 
             message for each port must be issued. The GET and
             SET commands are both supported; however, the 
             additional data parameter (b_version) is only used
             with a SET command. 

Parameters  : 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration
                e_PARAM_SET - Set the current log configuration

        b_version
        On set, contails value of version number

Output
        ps_Tlv
        On get, pointer to location to place TLV information


Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.7.7
-----------------------------------------------------------------------------
*/
int SC_PTP_VersionNumber(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV    *ps_Tlv, 
   UINT8           b_version
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get or Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
      s_msgConfig.b_actionFld = k_ACTF_SET;
      break;   
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_VersionNumber(&s_msgConfig, &s_clbkData, b_version);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_EnaPort()

Description: This function handles the management message 
             ENABLE_PORT. This command enables a port on an OC
             or BC using the DESIGNATED_ENABLED event. The only
             supported action command is COMMAND. 

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_CMD - Send Enable Port command to PTP stack

Output
        ps_Tlv
        On command, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.2.3
-----------------------------------------------------------------------------
*/
int SC_PTP_EnaPort(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV    *ps_Tlv
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Cmd */
   switch(b_operation)
   {
   case e_PARAM_CMD:
      s_msgConfig.b_actionFld = k_ACTF_CMD;
      break;   
   case e_PARAM_GET:
   case e_PARAM_SET:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_EnaPort(&s_msgConfig, &s_clbkData);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_DisPort()

Description: This function handles the management message 
             DISABLE_PORT. This command disables a port on an OC
             or BC using the DESIGNATED_DISABLED event. The only 
             supported action command is COMMAND. 

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_CMD - Send Enable Port command to PTP stack

Output
        ps_Tlv
        On command, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.2.4
-----------------------------------------------------------------------------
*/
int SC_PTP_DisPort(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV    *ps_Tlv
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Cmd */
   switch(b_operation)
   {
   case e_PARAM_CMD:
      s_msgConfig.b_actionFld = k_ACTF_CMD;
      break;   
   case e_PARAM_GET:
   case e_PARAM_SET:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_DisPort(&s_msgConfig, &s_clbkData);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_Time()

Description: This function handles the management message 
             TIME. This message gets or updates the local time
             of a node. Only GET command is supported.

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.2.1
-----------------------------------------------------------------------------
*/
int SC_PTP_Time(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV    *ps_Tlv
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_Time(&s_msgConfig, &s_clbkData, NULL);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_ClkAccuracy()

Description: This function handles the management message 
             CLOCK_ACCURACY. This message gets the 
             local clock accuracy member of the clock quality 
             member of the default data set. Only the GET is supported.

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.2.2
-----------------------------------------------------------------------------
*/
int SC_PTP_ClkAccuracy(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV    *ps_Tlv
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;
   SC_t_clkAccEnum   e_clkAccuracy;

   if (!o_initDone)
      return -2;

/* Get */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_ClkAccuracy(&s_msgConfig, &s_clbkData, e_clkAccuracy);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_UtcProp()

Description: This function handles the management message 
             UTC_PROPERTIES. This message gets or updates UTC 
             based members of the time properties data set. Only the
             GET command is supported.

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.6.2
-----------------------------------------------------------------------------
*/
int SC_PTP_UtcProp(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV    *ps_Tlv
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;
   INT16           i_utcOffset;
   BOOLEAN         o_leap61;
   BOOLEAN         o_leap59;
   BOOLEAN         o_utcValid;

   if (!o_initDone)
      return -2;

/* Get */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_UtcProp(&s_msgConfig, &s_clbkData, i_utcOffset, o_leap61, o_leap59, o_utcValid);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_TraceProp()

Description: This function handles the management message 
             TRACEABILITY_PROPERTIES. This message gets or
             updates traceability members of the time 
             properties data set. Only the GET command is 
             supported.

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.6.3
-----------------------------------------------------------------------------
*/
int SC_PTP_TraceProp(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV    *ps_Tlv
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;
   BOOLEAN           o_timeTrace;
   BOOLEAN           o_freqTrace;

   if (!o_initDone)
      return -2;

/* Get */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_TraceProp(&s_msgConfig, &s_clbkData, o_timeTrace, o_freqTrace);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                 SC_PTP_TimeSclProp()

Description: This function handles the management message 
             TIMESCALE_PROPERTIES. This message gets or updates
             timescale members of the time properties data set. 
             Only the GET command is supported.

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 15.5.3.6.4
-----------------------------------------------------------------------------
*/
int SC_PTP_TimeSclProp(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV    *ps_Tlv
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;
   SC_t_tmSrcEnum    e_timeSource;
   BOOLEAN           o_ptpTmScale;

   if (!o_initDone)
      return -2;

/* Get */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_TimeSclProp(&s_msgConfig, &s_clbkData, e_timeSource, o_ptpTmScale);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_UcNegoEnable()

Description: This function handles the management message 
             UNICAST_NEGOTIATION_ENABLE. This message enables or 
             disables unicast negotiation. The GET and SET 
             commands are both supported; however, the additional
             data parameter (o_ucNegoEna) is only used with a SET
             command.  

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration
                e_PARAM_SET - Set the current log configuration

        o_unNegoEna
        TRUE  -> enabled UC negotiation
        FALSE -> disabled UC negotiation

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received


Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 16.1.4.5
-----------------------------------------------------------------------------
*/
int SC_PTP_UcNegoEnable(
   t_paramOperEnum b_operation, 
   SC_t_MntTLV    *ps_Tlv,
   BOOLEAN         o_unNegoEna
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
      s_msgConfig.b_actionFld = k_ACTF_SET;
      break;
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_UcNegoEnable(&s_msgConfig, &s_clbkData, o_unNegoEna);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_UCMasterTbl()

Description: This function handles the management message 
             UNICAST_MASTER_TABLE. A GET request responds the 
             current unicast master table including each entry. 
             A SET command forces the node to reconfigure the 
             unicast master table. Configuring an empty table 
             (w_tblSize is zero and ps_ucMasTbl points to NULL) 
             prompts the generation of a message, which causes 
             the node to reset the unicast master table. Otherwise,
             a list of port addresses (ps_ucMasTbl) will be 
             transmitted and stored by the receiving node. If 
             addressed to all ports, a response for each port must 
             be issued. The GET and SET commands are both supported; 
             however, the additional data parameters are only used 
             within the SET command. 

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration
                e_PARAM_SET - Set the current log configuration

        c_queryIntv
        On set, mean interval for re-transmit a not granted  request

        w_tblSize
        On set, amount of masters / table entries

        ps_ucMasTbl
        On set, pointer to first table entry (port address)

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 17.5.3
-----------------------------------------------------------------------------
*/
int SC_PTP_UCMasterTbl(
   t_paramOperEnum   b_operation, 
   SC_t_MntTLV      *ps_Tlv,
   INT8              c_queryIntv,
   UINT16            w_tblSize,
   const t_PortAddr *ps_ucMasTbl
)
{
   int o_rtnFail = -1;;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
      s_msgConfig.b_actionFld = k_ACTF_SET;
      break;
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_UCMasterTbl(&s_msgConfig, 
                      &s_clbkData, 
                      c_queryIntv, 
                      w_tblSize, 
                      (PTP_t_PortAddr *)ps_ucMasTbl);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_UCMstMaxTblSize()

Description: This function handles the management message 
             UNICAST_MASTER_MAX_TABLE_SIZE. This message prompts 
             a response message including the maximum number of 
             unicast master table entries of the requested port. 
             If addressed to all ports, a response for each port 
             must be issued. The only supported action command is 
             GET. 

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 17.5.4
-----------------------------------------------------------------------------
*/
int SC_PTP_UCMstMaxTblSize(
   t_paramOperEnum   b_operation, 
   SC_t_MntTLV      *ps_Tlv
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_UCMstMaxTblSize(&s_msgConfig, 
                      &s_clbkData);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_AcceptMstTbl()

Description: This function handles the management message 
             ACCEPTABLE_MASTER_TABLE. A GET request prompts the
             node to respond the current acceptable master table. 
             With a SET request, new entries are configured. If 
             addressed to all ports, a response for each port must
             be issued. The GET and SET commands are both supported;
             however, the additional data parameters (w_tblSize and
             ps_accMstTbl) are only used within the SET command. 

Parameters  : 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration
                e_PARAM_SET - Set the current log configuration

        w_tblSize
        On set, amount of table entries

        ps_accMstTbl
        On set, pointer to first table entry of accept masters

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 17.5.4
-----------------------------------------------------------------------------
*/
int SC_PTP_AcceptMstTbl(
   t_paramOperEnum          b_operation, 
   SC_t_MntTLV             *ps_Tlv,
   UINT16                   w_tblSize,
   SC_t_AcceptMaster *ps_accMstTbl
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get, Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
      s_msgConfig.b_actionFld = k_ACTF_SET;
      break;
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_AcceptMstTbl(&s_msgConfig, 
                      &s_clbkData, 
                      w_tblSize, 
                      (PTP_t_AcceptMaster *)ps_accMstTbl);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_AcceptMstTblEna()

Description: This function handles the management message 
             ACCEPTABLE_MASTER_TABLE_ENABLED. This message 
             enables or disables the acceptable master 
             functionality of a node interface. A GET request
             gets the configuration of the enable flag; a SET 
             request configures the node. If addressed to all 
             ports, a response for each port must be issued. 
             The GET and SET commands are both supported; 
             however, the additional data parameter (o_enabled) 
             is only used within the SET command. 


Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration
                e_PARAM_SET - Set the current log configuration

        o_enabled
        On set, TRUE  -> acceptable MST enabled
        On set, FALSE -> not enabled

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 17.6.5
-----------------------------------------------------------------------------
*/
int SC_PTP_AcceptMstTblEna(
   t_paramOperEnum          b_operation, 
   SC_t_MntTLV             *ps_Tlv,
   BOOLEAN                  o_enabled
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get, Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
      s_msgConfig.b_actionFld = k_ACTF_SET;
		break;
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_AcceptMstTblEna(&s_msgConfig, 
                      &s_clbkData, 
                      o_enabled);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_AcceptMstTblSize()

Description: This function handles the management message 
             ACCEPTABLE_MASTER_MAX_TABLE_SIZE. This message 
             prompts a response message including the maximum 
             number of acceptable master table entries of the 
             requested port. If addressed to all ports, a 
             response for each port must be issued. The only
             supported action command is GET. 

Parameters: 

Input
        b_operation
        This enumeration defines the operation of the function to either 
        set or get.
                e_PARAM_GET - Get the current log configuration

Output
        ps_Tlv
        On get, pointer to location to place TLV information

Return value:
         0: function succeeded
        -1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 17.6.4
-----------------------------------------------------------------------------
*/
int SC_PTP_AcceptMstMaxTblSize(
   t_paramOperEnum          b_operation, 
   SC_t_MntTLV             *ps_Tlv
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_AcceptMstMaxTblSize(&s_msgConfig, 
                      &s_clbkData);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 

#if 0
/*
----------------------------------------------------------------------------
                                SC_PTP_AltMaster()

Description: This function handles the management message 
             ALTERNATE_MASTER. This message gets or sets the 
             attributes of the optional alternate master 
             mechanism. A GET request responds the current 
             values; a SET request configures the target. If
             addressed to all ports, a response for each port
             must be issued. The GET and SET commands are both
             supported; however, the additional data are only
             used within the SET command. 

Parameters: 

Input
	b_operation (IN) 
	This enumeration defines the operation of the function to either 
	set or get.
		e_PARAM_GET - Get the current log configuration
		e_PARAM_SET - Set the current log configuration

	o_txSync
	On set, TRUE  -> alternate sync msg
	On set, FALSE -> no alt. sync msg

	c_syncIntv
	On set, log alternate multicast sync interval

	b_numOfAltMas
	On set, amount of alternate masters

Output
	ps_Tlv
	On get, pointer to location to place TLV information

Return value:
	 0: function succeeded
	-1: function failed
        -2: not initialized
        -3: management error status packet received

Remarks: Cross-reference to the IEEE 1588 standard; 
         see also chapter 17.4.3
-----------------------------------------------------------------------------
*/
int SC_PTP_AltMaster(
   t_paramOperEnum          b_operation, 
   SC_t_MntTLV             *ps_Tlv,
   BOOLEAN                  o_txSync,
   INT8                     c_syncIntv,
   UINT8                    b_numOfAltMas
)
{
   int o_rtnFail = -1;
   MNT_t_apiMsgConfig s_msgConfig;
   MNT_t_apiClbkData s_clbkData;

   if (!o_initDone)
      return -2;

/* Get, Set */
   switch(b_operation)
   {
   case e_PARAM_GET:
      s_msgConfig.b_actionFld = k_ACTF_GET;
      break;
   case e_PARAM_SET:
      s_msgConfig.b_actionFld = k_ACTF_SET;
   case e_PARAM_CMD:
   default:
      return -1;
      break; 
   }
   
   Build_Call(&s_msgConfig, &s_clbkData);

/* set return function in data block */
   s_clbkData.pf_clbkFunc = Mnt_Return;      

/* wait if there is someone else accessing the semaphore */
   pthread_mutex_lock(&mutexMnt);

/* send command to IXXAT */
   MNTapi_AltMaster(&s_msgConfig, 
                      &s_clbkData, 
                      o_txSync, 
                      c_syncIntv, 
                      b_numOfAltMas);

/* wait for return semaphore */
   sem_wait(&semMnt);

/* copy over the tlv */
   o_rtnFail = processRtn(ps_Tlv);

/* unlock semaphore resource */
   pthread_mutex_unlock(&mutexMnt);

   return o_rtnFail;
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_SaveNVolStor()

** Description : This function handles the management message
**               SAVE_IN_NON_VOLATILE_STORAGE. This command will
**               store all current values of the applicable dynamic
**               and configurable data set members into non volatile
**               storage. The only supported action command is 
**               COMMAND. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function failed
**               FALSE - function successful
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.4
-----------------------------------------------------------------------------
*/
int SC_PTP_SaveNVolStor( 
   const SC_t_apiMsgConfig *ps_msgConfig,
   const SC_t_apiClbkData  *ps_clbkData
)
{
   return MNTapi_SaveNVolStor((MNT_t_apiMsgConfig *)ps_msgConfig, (MNT_t_apiClbkData *)ps_clbkData);
}  


/*
----------------------------------------------------------------------------
                                SC_PTP_RstNVolStor()

** Description : This function handles the management message 
**               RESET_NON_VOLATILE_STORAGE. This command resets
**               the dynamic and configurable data set members of
**               non volatile storage to initialization default 
**               values. The only supported action command is
**               COMMAND. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function failed
**               FALSE - function successful
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.5
-----------------------------------------------------------------------------
*/
int SC_PTP_RstNVolStor( 
   const SC_t_apiMsgConfig *ps_msgConfig,
   const SC_t_apiClbkData  *ps_clbkData
)
{
   return MNTapi_RstNVolStor((MNT_t_apiMsgConfig *)ps_msgConfig, (MNT_t_apiClbkData *)ps_clbkData);
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_Initialize()

** Description : This function handles the management message
**               INITIALIZE. This command starts special
**               initialization depending on the INITIALIZATION_KEY.
**               The only supported action command is COMMAND.
**               Defined initialization keys are: 
**
**                0000 - INITIALIZE_EVENT
**                       Causes INITIALZE event within an OC or BC.
**                0001-7FFF - Reserved
**                8000-FFFF - Implementation-specific
**
** Parameters  : ps_msgConfig (IN) - msg addresses & config
**               ps_clbkData  (IN) - status and callback info
**               w_initKey    (IN) - initialization key
**
** Returnvalue : TRUE  - function failed
**               FALSE - an error occured
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.6
-----------------------------------------------------------------------------
*/
int SC_PTP_Initialize( 
   const SC_t_apiMsgConfig *ps_msgConfig,
   const SC_t_apiClbkData  *ps_clbkData,
   UINT16                    w_initKey
)
{
   return MNTapi_Initialize((MNT_t_apiMsgConfig *)ps_msgConfig, (MNT_t_apiClbkData *)ps_clbkData, w_initKey);
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_FaultLog()

** Description : This function handles the management message
**               FAULT_LOG. This management messages returns all
**               recorded faults of the node. The only supported
**               action command is GET. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function failed
**               FALSE - function successful
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.7
-----------------------------------------------------------------------------
*/
int SC_PTP_FaultLog( 
   const SC_t_apiMsgConfig *ps_msgConfig,
   const SC_t_apiClbkData  *ps_clbkData
)
{
   return MNTapi_FaultLog((MNT_t_apiMsgConfig *)ps_msgConfig, (MNT_t_apiClbkData *)ps_clbkData);
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_FaultLogReset()

** Description : This function handles the management message
**               FAULT_LOG_RESET. This management messages resets
**               the complete fault record of the node. The only
**               supported action command is COMMAND. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**
** Returnvalue : TRUE  - function failed
**               FALSE - function successful
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.1.8
-----------------------------------------------------------------------------
*/
int SC_PTP_FaultLogReset( 
   const SC_t_apiMsgConfig *ps_msgConfig,
   const SC_t_apiClbkData  *ps_clbkData
)
{
   return MNTapi_FaultLogReset((MNT_t_apiMsgConfig *)ps_msgConfig, (MNT_t_apiClbkData *)ps_clbkData);
} 


/*
----------------------------------------------------------------------------
                                SC_PTP_Priority1()

** Description : This function handles the management message
**               PRIORITY1. This message gets or updates the
**               priority1 member of the default data set. The GET
**               and SET commands are both supported; however, the
**               additional data parameter (b_priority1) is only 
**               used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_priority1    (IN) - value of priority 1
**
** Returnvalue : TRUE  - function failed
**               FALSE - function successful
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.3.2
-----------------------------------------------------------------------------
*/
int SC_PTP_Priority1( 
   const SC_t_apiMsgConfig *ps_msgConfig,
   const SC_t_apiClbkData  *ps_clbkData,
   UINT8                    b_priority1
)
{
   return MNTapi_Priority1((MNT_t_apiMsgConfig *)ps_msgConfig, (MNT_t_apiClbkData *)ps_clbkData, b_priority1);
}


/*
----------------------------------------------------------------------------
                                SC_PTP_Priority2()

** Description : This function handles the management message
**               PRIORITY2. This message gets or updates the
**               priority2 member of the default data set. The GET 
**               and SET commands are both supported; however, the 
**               additional data parameter (b_priority2) is only 
**               used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               b_priority2    (IN) - value of priority 2
**
** Returnvalue : TRUE  - function failed
**               FALSE - an error occured
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.3.3
-----------------------------------------------------------------------------
*/
int SC_PTP_Priority2( 
   const SC_t_apiMsgConfig *ps_msgConfig,
   const SC_t_apiClbkData  *ps_clbkData,
   UINT8                    b_priority2
)
{
   return MNTapi_Priority2((MNT_t_apiMsgConfig *)ps_msgConfig, (MNT_t_apiClbkData *)ps_clbkData, b_priority2);
}


/*
----------------------------------------------------------------------------
                                SC_PTP_SlaveOnly()

** Description : This function handles the management message
**               SLAVE_ONLY. This message gets or updates the slave
**               only member of the default data set. The GET and 
**               SET commands are both supported; however, the 
**               additional data parameter (o_slaveOnly) is only 
**               used with a SET command. 
**
** Parameters  : ps_msgConfig  (IN) - msg addresses & config
**               ps_clbkData   (IN) - status and callback info
**               o_slaveOnly    (IN) - TRUE  -> slave only
**                                     FALSE -> master is possible
**
** Returnvalue : TRUE  - function failed
**               FALSE - function successful
**
** Remarks     : Cross-reference to the IEEE 1588 standard; 
**               see also chapter 15.5.3.3.5
-----------------------------------------------------------------------------
*/
int SC_PTP_SlaveOnly( 
   const SC_t_apiMsgConfig *ps_msgConfig,
   const SC_t_apiClbkData  *ps_clbkData,
   BOOLEAN                   o_slaveOnly
)
{
   return MNTapi_SlaveOnly((MNT_t_apiMsgConfig *)ps_msgConfig, (MNT_t_apiClbkData *)ps_clbkData, o_slaveOnly);
}

#endif

