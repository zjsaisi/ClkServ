#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

#include "clkserv_api.h"
#include "clkserv_priv.h"
#include "sys_defines.h"

/*******************************************************************************
 * FUNCTION: clkserv_get_socket
 *
 * DESCRIPTION: This function get one socket connection to clkserv daemon
 *
 * INPUTS: none.
 *
 * OUTPUTS: none.
 *
 * RETURN:
 * 		socket descriptor -- if succeed
 * 		-1 - if failed
*******************************************************************************/
static int32_t	clkserv_get_socket(void)
{
	int32_t sock_hdl;
	int32_t	len;
	struct sockaddr_un ser_addr;
	int32_t con_count = 0;

	if((sock_hdl = socket(PF_UNIX, SOCK_STREAM, 0)) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "Fails to create socket\n");
		return -1;
	}
	
	memset((char*)&ser_addr, 0, sizeof(struct sockaddr_un));
	ser_addr.sun_family = AF_UNIX;
	strncpy(ser_addr.sun_path, CLKSERV_SOCKET, 107);
	len = sizeof(ser_addr.sun_family) + strlen(ser_addr.sun_path);

	while(connect(sock_hdl, (struct sockaddr*)&ser_addr, len) == -1){
		if(++con_count >= SOCKET_CONN_TRY)
			break;
	}
    if (con_count >= SOCKET_CONN_TRY)
    {
        syslog(LOG_BSS|LOG_DEBUG, "Fails to connect to clkserv socket\n");
        close(sock_hdl);
        return -1;
    }

    return sock_hdl;
}

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
int32_t	clkserv_get_servo_state(uint8_t	*state)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;

	if(state == NULL){
		syslog(LOG_BSS|LOG_DEBUG, "%s: NULL parameter", __FUNCTION__);
		return -1;
	}

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}

	/*Prepare packet*/
	buf[0] = CMD_GET_STATE;
	buf[1] = 2;

	if(send(socket_handle, buf, 2, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	recv_size = recv(socket_handle, buf, 3, MSG_NOSIGNAL);
	if(recv_size < 3){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to receive packet from clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	*state = buf[2];
	close(socket_handle);

	return 0;
}
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
int32_t	clkserv_get_servo_statics(servo_statics_t *cur_status)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;
	uint32_t t_size = sizeof(servo_statics_t);

	if(cur_status == NULL){
		syslog(LOG_BSS|LOG_DEBUG, "%s: NULL parameter", __FUNCTION__);
		return -1;
	}

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}

	/*Prepare packet*/
	buf[0] = CMD_GET_STATICS;
	buf[1] = 2;

	if(send(socket_handle, buf, 2, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	recv_size = recv(socket_handle, buf, (2+t_size), MSG_NOSIGNAL);
	if(recv_size < (2+t_size)){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to receive packet from clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	memcpy((char*)cur_status, &buf[2], t_size);
	close(socket_handle);

	return 0;

}


/******************************************************************************
 * FUNCTION: clkserv_get_servo_local_quality
 *
 * DESCRIPTION: get current servo local quality
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
int32_t	clkserv_get_servo_local_quality(uint8_t *local_quality)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;

	if(local_quality == NULL){
		syslog(LOG_BSS|LOG_DEBUG, "%s: NULL parameter", __FUNCTION__);
		return -1;
	}

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}

	/*Prepare packet*/
	buf[0] = CMD_GET_LOCAL_QUALITY;
	buf[1] = 2;

	if(send(socket_handle, buf, 2, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	recv_size = recv(socket_handle, buf,3, MSG_NOSIGNAL);
	if(recv_size < 3){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to receive packet from clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	*local_quality =(uint8_t) buf[2];
	close(socket_handle);

	return 0;

}

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
int32_t	clkserv_get_servo_chan_config(char *chan_config)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;
	uint32_t t_size = sizeof(servo_statics_t);

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}

	/*Prepare packet*/
	buf[0] = CMD_GET_CHAN_CONFIG;
	buf[1] = 2;

	if(send(socket_handle, buf, 2, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	recv_size = recv(socket_handle, buf, sizeof(buf), MSG_NOSIGNAL);
	if(recv_size > 0){
		strncpy(chan_config, &buf[2], 32); 
	}
	else{
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to receive packet from clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}

	close(socket_handle);

	return 0;
}

/******************************************************************************
 * FUNCTION: clkserv_get_servo_chan_status
 *
 * DESCRIPTION: get current servo channel status
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
int32_t	clkserv_get_servo_chan_status(char *chan_status)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;
	uint32_t t_size = sizeof(servo_statics_t);

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}

	/*Prepare packet*/
	buf[0] = CMD_GET_CHAN_STATUS;
	buf[1] = 2;

	if(send(socket_handle, buf, 2, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	recv_size = recv(socket_handle, buf, sizeof(buf), MSG_NOSIGNAL);
	if(recv_size > 0){
		strncpy(chan_status, &buf[2], 32); 
	}
	else{
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to receive packet from clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}

	close(socket_handle);

	return 0;
}

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
int32_t	clkserv_get_servo_active_reference(uint8_t *chan)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;
	uint32_t t_size = sizeof(servo_statics_t);

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}

	/*Prepare packet*/
	buf[0] = CMD_GET_ACT_REF;
	buf[1] = 2;

	if(send(socket_handle, buf, 2, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	recv_size = recv(socket_handle, buf, sizeof(buf), MSG_NOSIGNAL);
	if(recv_size > 0){
		*chan = buf[2];
	}
	else{
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to receive packet from clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}

	close(socket_handle);

	return 0;
}

/******************************************************************************
 * FUNCTION: clkserv_set_servo_active_reference
 *
 * DESCRIPTION: set current servo selection channel
 *
 * INPUTS:  
 * 			chan - channel index
 * 			isFreq  -	1: frequency channel should be set
 *					 	0: time channel should be set
 *
 * OUTPUTS: 
 * 			none.
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
int32_t	clkserv_set_servo_active_reference(uint8_t chan)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;
	uint32_t t_size = sizeof(servo_statics_t);

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}

	/*Prepare packet*/
	buf[0] = CMD_SET_ACT_REF;
	buf[1] = 3;
	buf[2] = chan;

	if(send(socket_handle, buf, 3, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}

	close(socket_handle);

	return 0;

}

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
int32_t	clkserv_set_servo_chan_switch_mode(uint8_t mode)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}
	
	/*Prepare packet*/
	buf[0] = CMD_SET_SWITCH_MODE;
	buf[1] = 3;
	buf[2] = mode;

	if(send(socket_handle, buf, 3, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}

	close(socket_handle);

	return 0;
}

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
int32_t	clkserv_get_servo_chan_switch_mode(uint8_t *mode)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;
	uint32_t t_size = sizeof(servo_statics_t);

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}
	
	/*Prepare packet*/
	buf[0] = CMD_GET_SWITCH_MODE;
	buf[1] = 2;

	if(send(socket_handle, buf, 2, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}

	recv_size = recv(socket_handle, buf, sizeof(buf), MSG_NOSIGNAL);
	if(recv_size > 0){
		*mode = buf[2];
	}
	else{
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to receive packet from clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	close(socket_handle);

	return 0;
}

/******************************************************************************
 * FUNCTION: clkserv_get_servo_chan_valid
 *
 * DESCRIPTION: get servo active reference
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
int32_t	clkserv_get_servo_chan_valid(uint8_t chan, uint8_t *valid)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;
	uint32_t t_size = sizeof(servo_statics_t);

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}

	/*Prepare packet*/
	buf[0] = CMD_GET_CHAN_VALID;
	buf[1] = 3;
	buf[2] = chan;

	if(send(socket_handle, buf, 3, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	recv_size = recv(socket_handle, buf, sizeof(buf), MSG_NOSIGNAL);
	if(recv_size > 0){
		*valid = buf[2];
	}
	else{
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to receive packet from clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}

	close(socket_handle);

	return 0;
}

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
int32_t	clkserv_get_servo_phase_offset(int64_t *offset)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;
	uint32_t t_size = sizeof(servo_statics_t);
	int64_t offset_t;

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}

	/*Prepare packet*/
	buf[0] = CMD_GET_PHASE_OFFSET;
	buf[1] = 2;

	if(send(socket_handle, buf, 2, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	recv_size = recv(socket_handle, buf, sizeof(buf), MSG_NOSIGNAL);
	if(recv_size > 0){
		CHARS_TO_64BIT((buf + 2), &offset_t);
		*offset = offset_t;
	}
	else{
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to receive packet from clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}

	close(socket_handle);

	return 0;
}
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
int32_t	clkserv_set_servo_phase_offset(int64_t offset)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	int64_t offset_t;

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}

	/*Prepare packet*/
	buf[0] = CMD_SET_PHASE_OFFSET;
	buf[1] = 10;
	INT64_TO_CHARS(offset, (buf + 2));
	if(send(socket_handle, buf, 10, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}

	close(socket_handle);

	return 0;
}
/******************************************************************************
 * FUNCTION: clkserv_set_servo_chan_prio
 *
 * DESCRIPTION: set current servo channel switch mode
 *
 * INPUTS:  
 * 			
 * 			chan	: channel index
 * 			isFreq	: TRUE(frequency priority will be set) FALSE(time priority will be set)
 * 			prio	: 1 ~ 10
 *
 * OUTPUTS: 
 * 			none.
 *
 * RETURN: 
 * 			0 -- send request successfully
 * 		   -1 -- send request in failure
 *
 *****************************************************************************/
int32_t	clkserv_set_servo_chan_prio(uint8_t chan, uint8_t isFreq, uint8_t prio)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}
	
	/*Prepare packet*/
	buf[0] = CMD_SET_CHAN_PRIO;
	buf[1] = 3;
	buf[2] = chan;
	buf[3] = isFreq;
	buf[4] = prio;

	if(send(socket_handle, buf, 5, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}

	close(socket_handle);

	return 0;
}

//add by Hooky	20140214
/*************************************************************************************************
  *@@
  *@NAME:		int32_t	clkserv_get_crystal_state(uint8_t	*state)	
  *@GUNCTION:	get crystal state
  *@AUTHOR:		Hooky	20140214
  *@OUTPUT:		state : 0-crytal not tarck ; 1-crytal normal track
  *@
  ************************************************************************************************/
int32_t	clkserv_get_crystal_state(uint8_t	*state)
{
	int32_t	socket_handle;
	char	buf[SOCKET_BUF_SIZE];
	size_t	recv_size;

	if(state == NULL){
		syslog(LOG_BSS|LOG_DEBUG, "%s: NULL parameter", __FUNCTION__);
		return -1;
	}

	socket_handle = clkserv_get_socket();
	if(socket_handle == -1){
		return -1;
	}

	/*Prepare packet*/
	buf[0] = CMD_GET_CRYSTAL;
	buf[1] = 2;

	if(send(socket_handle, buf, 2, MSG_NOSIGNAL) == -1){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to send packet to clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	recv_size = recv(socket_handle, buf, 3, MSG_NOSIGNAL);
	if(recv_size < 3){
		syslog(LOG_BSS|LOG_DEBUG, "%s: failed to receive packet from clkserv daemon", __FUNCTION__);
		close(socket_handle);
		return -1;
	}
	*state = buf[2];
	close(socket_handle);

	return 0;
}
//end add

