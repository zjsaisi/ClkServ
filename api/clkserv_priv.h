#ifndef __INC_CLKSERV_PRIV_H
#define __INC_CLKSERV_PRIV_H

#define	LOG_BSS				LOG_LOCAL2

/*Command definitions*/
#define	CMD_GET_STATE				0x01
#define	CMD_GET_STATICS				0x02
#define	CMD_GET_CHAN_CONFIG			0x03
#define	CMD_GET_CHAN_STATUS			0x04
#define	CMD_GET_SEL_MODE			0x05
#define	CMD_SET_SEL_MODE			0x06
#define	CMD_GET_SWITCH_MODE			0x07
#define	CMD_SET_SWITCH_MODE			0x08
#define	CMD_SET_CHAN_ENABLE			0x09
//#define	CMD_SET_CHAN_PRIO			0x0A
#define	CMD_SET_CHAN_PQL_ENA		0x0B
#define	CMD_SET_CHAN_PQL			0x0C
#define CMD_GET_ACT_REF				0x0D
#define CMD_SET_ACT_REF				0x0E
#define CMD_GET_CHAN_VALID			0x0F
#define CMD_GET_PHASE_OFFSET		0x10
#define CMD_GET_LOCAL_QUALITY		0x11
#define CMD_SET_CHAN_PRIO			0x12
#define CMD_SET_PHASE_OFFSET		0x13
//add by Hooky 20140214
#define CMD_GET_CRYSTAL				0x14
//add end

/*socket definition*/
#define SOCKET_CONN_TRY		50
#define SOCKET_BUF_SIZE		512
#define	CLKSERV_SOCKET		"/tmp/.clkservd"
#define	CLKSERV_SOCKET_NCONN	5

#endif /*__INC_CLKSERV_PRIV_H*/
