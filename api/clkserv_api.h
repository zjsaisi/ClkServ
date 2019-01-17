#ifndef __INC_CLKSERV_API
#define __INC_CLKSERV_API

typedef struct servo_statics_type {
	uint8_t		state;
	uint32_t	cur_state_dur;
	double		freq_corr;
} servo_statics_t;

/******************************************************************************
 * FUNCTION: clkserv_get_servo_state
 *
 * DESCRIPTION: get current servo state
 *
 * INPUTS:  
 * 			none.
 *
 * OUTPUTS: 
 * 			state --- to store servo state
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_get_servo_state(uint8_t	*state);

/******************************************************************************
 * FUNCTION: clkserv_get_servo_statics
 *
 * DESCRIPTION: get current servo running statics
 *
 * INPUTS:  
 * 			none.
 *
 * OUTPUTS: 
 * 			
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_get_servo_state(uint8_t	*state);

/******************************************************************************
 * FUNCTION: clkserv_get_servo_statics
 *
 * DESCRIPTION: get current servo running statics
 *
 * INPUTS:  
 * 			none.
 *
 * OUTPUTS: 
 * 			
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *****************************************************************************/
extern	int32_t	clkserv_get_servo_local_quality(uint8_t *local_quality);

/******************************************************************************
 * FUNCTION: clkserv_get_servo_chan_config
 *
 * DESCRIPTION: get current servo channel config
 *
 * INPUTS:  
 * 			none.
 *
 * OUTPUTS: 
 * 			the path of chan config
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_get_servo_chan_config(char *chan_config);

/******************************************************************************
 * FUNCTION: clkserv_get_servo_chan_config
 *
 * DESCRIPTION: get current servo channel status
 *
 * INPUTS:  
 * 			none.
 *
 * OUTPUTS: 
 * 			the path of chan status
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_get_servo_chan_status(char *chan_status);

/******************************************************************************
 * FUNCTION: clkserv_get_servo_chan_select_mode
 *
 * DESCRIPTION: get current servo channel selection mode
 *
 * INPUTS:  
 * 			none.
 *
 * OUTPUTS: 
 * 			channel select mode
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_get_servo_chan_select_mode(uint8_t *mode);

/******************************************************************************
 * FUNCTION: clkserv_set_servo_chan_seletc_mode
 *
 * DESCRIPTION: set current servo channel select mode
 *
 * INPUTS:  
 * 			channel select mode
 *
 * OUTPUTS: 
 * 			none.
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_set_servo_chan_select_mode(uint8_t mode);

/******************************************************************************
 * FUNCTION: clkserv_get_servo_chan_switch_mode
 *
 * DESCRIPTION: get current servo channel switch mode
 *
 * INPUTS:  
 * 			none.
 *
 * OUTPUTS: 
 * 			channel switch mode
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_get_servo_chan_switch_mode(uint8_t *mode);

/******************************************************************************
 * FUNCTION: clkserv_set_servo_chan_switch_mode
 *
 * DESCRIPTION: set current servo channel switch mode
 *
 * INPUTS:  
 * 			channel switch mode
 * 			0: e_SC_SWTMODE_AR
 * 			1: e_SC_SWTMODE_AS
 * 			2: e_SC_SWTMODE_OFF
 *
 * OUTPUTS: 
 * 			none.
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_set_servo_chan_switch_mode(uint8_t mode);

/******************************************************************************
 * FUNCTION: clkserv_set_servo_chan_enable
 *
 * DESCRIPTION: set current servo channel enable or disable
 *
 * INPUTS:  
 * 			chan - channel index
 * 			isFreq -	1: frequency enable should be set
 *						0: time enable should be set
 *			enable -	1: enable
 *						0: disable
 *
 * OUTPUTS: 
 * 			none.
 *
 * RETURN: 
 * 			0 -- send request successfully	
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_set_servo_chan_enable(uint8_t chan, uint8_t isFreq, uint8_t enable);

/******************************************************************************
 * FUNCTION: clkserv_set_servo_chan_prio
 *
 * DESCRIPTION: set current servo channel priority
 *
 * INPUTS:  
 * 			chan - channel index
 * 			isFreq  -	1: frequency priority should be set
 *					 	0: time priority should be set
 *			prio	-	1 ~ 10
 *
 * OUTPUTS: 
 * 			none.
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_set_servo_chan_prio(uint8_t chan, uint8_t isFreq, uint8_t prio);

/******************************************************************************
 * FUNCTION: clkserv_set_servo_chan_pql_enable
 *
 * DESCRIPTION: set current servo channel pql enable or disable
 *
 * INPUTS:  
 * 			chan - channel index
			
 *			enable -	1: enable
 *						0: disable
 *
 * OUTPUTS: 
 * 			none.
 *
 * RETURN: 
 * 			0 -- send request successfully	
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_set_servo_chan_pql_enable(uint8_t chan, uint8_t enable);

/******************************************************************************
 * FUNCTION: clkserv_set_servo_chan_pql
 *
 * DESCRIPTION: set current servo channel pql
 *
 * INPUTS:  
 * 			chan - channel index
 * 			isFreq  -	1: frequency pql should be set
 *					 	0: time pql should be set
 *			prio	-	1 ~ 16
 *
 * OUTPUTS: 
 * 			none.
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_get_servo_chan_pql(uint8_t chan, uint8_t isFreq, uint8_t pql);

/******************************************************************************
 * FUNCTION: clkserv_get_servo_active_reference
 *
 * DESCRIPTION: get servo active reference
 *
 * INPUTS:  
 * 			none.
 * 
 * OUTPUTS: 
 * 			chan	0: GPS
 * 					1: Beidou
 * 					2: IRIG-B
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_get_servo_active_reference(uint8_t *chan);

/******************************************************************************
 * FUNCTION: clkserv_set_servo_active_reference
 *
 * DESCRIPTION: set current servo selection channel
 *
 * INPUTS:  
 * 			chan - channel index
 *
 * OUTPUTS: 
 * 			none.
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_set_servo_active_reference(uint8_t chan);

/******************************************************************************
 * FUNCTION: clkserv_get_servo_chan_valid
 *
 * DESCRIPTION: get servo channel valid
 *
 * INPUTS:  
 * 			chan	0: GPS
 * 					1: Beidou
 * 					2: IRIG-B
 * OUTPUTS: 
 * 			valid	1: valid
 * 					0: no valid
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_get_servo_chan_valid(uint8_t chan, uint8_t *valid);

/******************************************************************************
 * FUNCTION: clkserv_get_servo_phase_offset
 *
 * DESCRIPTION: get servo phase offset
 *
 * INPUTS: none
 *  
 * OUTPUTS: 
 * 			valid	1: valid
 * 					0: no valid
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_get_servo_phase_offset(int64_t *offset);

/******************************************************************************
 * FUNCTION: clkserv_set_servo_phase_offset
 *
 * DESCRIPTION: set servo phase offset
 *
 * INPUTS: none
 *  
 * OUTPUTS: 
 * 			valid	1: valid
 * 					0: no valid
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
extern	int32_t	clkserv_set_servo_phase_offset(int64_t offset);

//add by Hooky	20140214
/*************************************************************************************************
  *@@
  *@NAME:		int32_t	clkserv_get_crystal_state(uint8_t	*state)	
  *@GUNCTION:	get crystal state
  *@AUTHOR:		Hooky	20140214
  *@OUTPUT:		state : 0-crytal not tarck ; 1-crytal normal track
  *@
  ************************************************************************************************/
extern int32_t	clkserv_get_crystal_state(uint8_t	*state);
//end add
#endif
