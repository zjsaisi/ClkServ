/* ptp.c
*/

//////////////////// includes
#include "target.h"
#include "PTP/PTPdef.h"
#include <ctype.h>
#include <string.h>
#include "logger.h"
#include "clk_private.h"
#include "ptp.h"

#define ACC_MASTER 1
#define NUM_BAD_SYNC_CNTS 10


///////////////////////////////////////////////////////////////////////
// local variables
///////////////////////////////////////////////////////////////////////
static UINT32 		prev_master_addr;
//static TIME_STRUCT 	time;
static UINT32 		last_sent_seconds = 0;
static UINT16 		prev_lease_dur;
static INT16 		ticks_per_delay_response;
static INT16 		prev_master_delay_rate;
static PTP_Flow_State flow_state;
static UINT32 		delay_req_ip_addr;


static UINT16 prev_master_lease_dur;	
static INT16 prev_master_delay_rate;
static UINT32 ticks_per_connection_timeout;
static UINT32 del_count;
static 	UINT32 del_ticks_announce_timeout;
FLOW_STRUCT_TYPE master_stuct[2];
static GM_select current_GM;

static void Update_Select(void);
static GM_select Who_should_be_GM(void);

DELAY_TIMESTAMP_BUF_STRUCT Delay_Timestamp_Buf;


///////////////////////////////////////////////////////////////////////
// This function is used to create delay request messge. 
///////////////////////////////////////////////////////////////////////

#define ANNOUNCE_TIMEOUT 10

UINT16 keni;

UINT8 Get_Delay_Timestamp(UINT16 seq_num, UINT32 *sec, UINT32 *nsec)
{
	UINT8 status = 0;
//	UINT16 i;

	*sec = 0;
	*nsec = 0;
/* search list from In to Out Index */	

	for(keni=0;keni<DELAY_TIMESTAMP_BUF_SIZE;keni++)
	{
		if((seq_num == Delay_Timestamp_Buf.Data[keni].sequence_num) && 
		   (!Delay_Timestamp_Buf.Data[keni].used_flag))
		{
			*sec = Delay_Timestamp_Buf.Data[keni].seconds;
			*nsec = Delay_Timestamp_Buf.Data[keni].nanoseconds;
			Delay_Timestamp_Buf.OutIndex = keni;
			Delay_Timestamp_Buf.Data[keni].used_flag = TRUE;
			status = 1;
			break;
		}
	
	}
	return status;
}

BOOLEAN Is_GM_Lock_Holdover(UINT8 master)
{
	//UINT32 time = ptp_announce_time[master] + (ANNOUNCE_TIMEOUT * (1 << nvm.nv_master_announce_intv));
	UINT32 time = ptp_announce_time[master] + (ANNOUNCE_TIMEOUT * (1 << k_INTV_ANNC));
	
/* see if announce message is old */
//	if(time < Get_ticks())
//	{
//		return FALSE;
//	}
	
/* check to see if clockCLass is 6 or 7 (locked or holdover) */
//	if((announce_message[master].grandmasterClass == 6) ||
//	(announce_message[master].grandmasterClass == 7))
	if(announce_message[master].grandmasterClass != 248)
	{
		return TRUE;
	}
	
	return FALSE;
}

BOOLEAN Is_Announce_Timeout(UINT8 master)
{
	UINT32 time = ptp_announce_time[master] + (ANNOUNCE_TIMEOUT * (1 << nvm.nv_master_announce_intv));
	
/* see if announce message is old */
	if(time < Get_ticks())
	{
		return TRUE;
	}

	return FALSE;
}

static void msgPackDelayReq(UINT32 *sock, UINT16 seq_num)
{
	UINT8 buf[200];
	UINT8 *buf_ptr;
	UINT16 blen = 200;
	UINT16 dlen = 0x2C;
	UINT16 i;
	UINT32 handle;
	sockaddr_in remote_sin;
	INT32 count;
	char my_buffer[500];
	
	
	for(i=0;i<200;i++)
	{
		buf[i] = 0;
	}
	buf_ptr = buf + 10;

	tx_uni_dreq (0, buf_ptr, nvm.nv_clock_id);

/*  insert sequence number */
	buf[40] = seq_num >> 8;
	buf[41] = seq_num & 0xFF;

	memset((char *) &remote_sin, 0, sizeof(sockaddr_in));
	remote_sin.sin_family = AF_INET;
	remote_sin.sin_port = 319;
	remote_sin.sin_addr.s_addr = delay_req_ip_addr;

	count = sendto(*sock, buf + 10, dlen, 0, &remote_sin, sizeof(sockaddr_in));

	if (count == RTCS_ERROR)
	{
		debug_printf(LAN_DEBUG_PRT, "\nsendto() failed with error %lx", RTCS_geterror(Ptp_sock));
	} 
	//udp_send (Ptp.sochandle, MASTER_IP_ADDR, 319, buf + 10, blen, dlen);  
}


UINT8 Sig_Acks_are_good(UINT8 master)
{
	INT32 lease_timeout;
	UINT8 is_good;
	
	lease_timeout = (INT32)Get_ticks() - 10 - (INT32)(nvm.nv_master_lease_dur/2);

	if((master_stuct[master].sync_signaling_ack_time > lease_timeout) &&
		   (master_stuct[master].delay_signaling_ack_time > lease_timeout) &&
		   (master_stuct[master].announce_signaling_ack_time > lease_timeout))
	{
		is_good = TRUE;
	}
	else
	{	
		is_good = FALSE;
	}
	
	return is_good;	
}
/* This function will manage the signaling to the GM and
* Acceptable GM
*/
void Update_flow(FLOW_STRUCT_TYPE *flow_struct)
{
	UINT8 clock_id[8];	
	UINT32 sync_time;
	UINT32 ticks = Get_ticks();
	INT32 lease_timeout;
	UINT8 master = flow_struct->master;
	UINT16 sync_pkt_per_sec, delay_pkt_per_sec;
	UINT16 bad_transmit = FALSE;


	Get_Flow_Status(&sync_pkt_per_sec, &delay_pkt_per_sec, master);

/* only send signaling if ip address is non-zero */
	if(*flow_struct->master_ip_address && nvm.nv_ptp_state_on)
	{
	/* SYNC */
		if((ticks > flow_struct->sync_signaling_scheduled) || 
		   (flow_struct->sync_signaling_ack_time == -10000))
		{
			if(tx_uni_req (*flow_struct->master_ip_address, 0x00, (UINT32)nvm.nv_master_lease_dur, (INT16)nvm.nv_master_sync_rate, &clock_id[0]))
			{
			/*good*/
			
			}
			else
			{
			/*bad*/
				bad_transmit = TRUE;
			}	// sync msg, long time, tx rate (pow 2)
			flow_struct->sync_signaling_scheduled = ticks + 5;
		}

	/* DELAY */	
		if(((ticks > flow_struct->delay_signaling_scheduled) || 
		   (flow_struct->delay_signaling_ack_time == -10000)))
		{
//			if(tx_uni_req (*flow_struct->master_ip_address, 0x09, (UINT32)nvm.nv_master_lease_dur, (INT32)-7, &clock_id[0]))
			if(tx_uni_req (*flow_struct->master_ip_address, 0x09, (UINT32)nvm.nv_master_lease_dur, (INT16)nvm.nv_master_sync_rate, &clock_id[0]))
			{
			/*good*/
			
			}
			else
			{
			/*bad*/
				bad_transmit = TRUE;
			}
			flow_struct->delay_signaling_scheduled = ticks + 5;
		}

	/* ANNOUNCE */
		if(((ticks > flow_struct->announce_signaling_scheduled) || 
		   (flow_struct->announce_signaling_scheduled == -10000)))
		{
			tx_uni_req (*flow_struct->master_ip_address, 0x0B, (UINT32)nvm.nv_master_lease_dur, (INT16)nvm.nv_master_announce_intv, &clock_id[0]);		// announce, long time, 1hz
			flow_struct->announce_signaling_scheduled = ticks + 5;
		}
	}

	
	if(sync_pkt_per_sec <= 1)
	{
		flow_struct->bad_sync_cntr++;
	}
	else
	{
		flow_struct->bad_sync_cntr = 0;
	} 

	lease_timeout = (INT32)ticks - 10 - (INT32)(nvm.nv_master_lease_dur/2);
	switch(flow_struct->flow_state)
	{
	/* This state will init  */
	case PTP_FLOW_INIT:
		debug_printf(KEN_PTP_PRT, "ptp_init\n");
		flow_struct->flow_state = PTP_FLOW_CHECK_ACKS;
		flow_struct->sync_signaling_scheduled = 0;
		flow_struct->delay_signaling_scheduled = 0;
		flow_struct->announce_signaling_scheduled = 0;
		flow_struct->sync_signaling_ack_time = -10000;
		flow_struct->delay_signaling_ack_time = -10000;
		flow_struct->announce_signaling_ack_time = -10000;
		flow_struct->state_count = 0;
		break;
	
	/* This state will check for lastest ACKs for SYNC, DELAY REQ, and ANNOUNCE flows */
	case PTP_FLOW_CHECK_ACKS:
		debug_printf(KEN_PTP_PRT, "ptp_check_acks\n");
		debug_printf(KEN_PTP_PRT, "%d %d %d %d %d\n",
				flow_struct->sync_signaling_ack_time,
				flow_struct->delay_signaling_ack_time,
				flow_struct->announce_signaling_ack_time,
				lease_timeout,
				ticks);

		
		if((flow_struct->sync_signaling_ack_time > lease_timeout) &&
		   (flow_struct->delay_signaling_ack_time > lease_timeout) &&
		   (flow_struct->announce_signaling_ack_time > lease_timeout))
		{
			flow_struct->flow_state = PTP_FLOW_CHECK_SYNC;
			flow_struct->state_count = 0;				
		}

			
		break;
	
	/* This state will check for a sync flow */
	case PTP_FLOW_CHECK_SYNC:
		debug_printf(KEN_PTP_PRT, "ptp_check_sync\n");
		if(!flow_struct->bad_sync_cntr)
		{
			flow_struct->flow_state = PTP_FLOW_CHECK_ANNOUNCE;
			flow_struct->state_count = 0;	
		}

/* requalify flow if there is a problem */		
		if((flow_struct->sync_signaling_ack_time < lease_timeout) ||
		   (flow_struct->delay_signaling_ack_time < lease_timeout) ||
		   (flow_struct->announce_signaling_ack_time < lease_timeout))
		{
			flow_struct->flow_state = PTP_FLOW_INIT;
			flow_struct->state_count = 0;
		}
/* if it is 10 seconds without a SYNC flow then ask again */
		else if(flow_struct->state_count > 10)
		{
			flow_struct->flow_state = PTP_FLOW_INIT;
			flow_struct->state_count = 0;				
		}
		break;
	
	/* This state will read look at the announce message 
	* for clockClass = 6 (Locked) and 7 (Holdover) 
	*/
	case PTP_FLOW_CHECK_ANNOUNCE:
		debug_printf(KEN_PTP_PRT, "ptp_check_announce\n");
		if(Is_GM_Lock_Holdover(master))
		{
			flow_struct->flow_state = PTP_FLOW_NORMAL;
			flow_struct->connection_trys = 0;
			flow_struct->state_count = 0;
		}

/* requalify flow if there is a problem */		
		if((flow_struct->sync_signaling_ack_time < lease_timeout) ||
		   (flow_struct->delay_signaling_ack_time < lease_timeout) ||
		   (flow_struct->announce_signaling_ack_time < lease_timeout))
		{
			flow_struct->flow_state = PTP_FLOW_INIT;
			flow_struct->state_count = 0;				
		}
		else if(flow_struct->bad_sync_cntr > NUM_BAD_SYNC_CNTS)
		{
			flow_struct->flow_state = PTP_FLOW_INIT;
			flow_struct->state_count = 0;
		}
		
		break;
		
	/* send out delay requests */
	case PTP_FLOW_NORMAL:
		debug_printf(KEN_PTP_PRT, "ptp_normal\n");
		
/* requalify flow if there is a problem */		
		if((flow_struct->sync_signaling_ack_time < lease_timeout) ||
		   (flow_struct->delay_signaling_ack_time < lease_timeout) ||
		   (flow_struct->announce_signaling_ack_time < lease_timeout))
		{
			flow_struct->flow_state = PTP_FLOW_INIT;				
			flow_struct->state_count = 0;				
		}
		else if(flow_struct->bad_sync_cntr > NUM_BAD_SYNC_CNTS)
		{
			flow_struct->flow_state = PTP_FLOW_INIT;				
			flow_struct->state_count = 0;
		}
		else if(!Is_GM_Lock_Holdover(master))
		{
			flow_struct->flow_state = PTP_FLOW_CHECK_ANNOUNCE;
			flow_struct->state_count = 0;
		}


		break;
	case PTP_FLOW_SUSPEND:
		flow_struct->flow_state = PTP_FLOW_INIT;
		break;
	default:
		break;
	}
	
	if(*flow_struct->master_ip_address == 0)
	{
		flow_struct->flow_state = PTP_FLOW_SUSPEND;
		flow_struct->state_count = 0;
	}
	
	if(!nvm.nv_ptp_state_on)
	{
		flow_struct->flow_state = PTP_FLOW_SUSPEND;
		flow_struct->state_count = 0;
	}
	
	flow_struct->state_count++;
}

void PTP_Flow_Control_Task (UINT32  parameter)
{
	current_GM = GRANDMASTER_NONE;

/* put master index into .master */
	master_stuct[0].master = GRANDMASTER_PRIMARY;

/* put into initial state */
	master_stuct[0].flow_state = PTP_FLOW_INIT;
/* set pointers to master ip addresses */
	master_stuct[0].master_ip_address = &nvm.nv_master_ip;

/* reset all timers */
	master_stuct[0].sync_signaling_scheduled = 0;
	master_stuct[0].delay_signaling_scheduled = 0;
	master_stuct[0].announce_signaling_scheduled = 0;
	master_stuct[0].sync_signaling_ack_time = -10000;
	master_stuct[0].delay_signaling_ack_time = -10000;
	master_stuct[0].announce_signaling_ack_time = -10000;

	_time_delay_ticks(TICKS_PER_SEC * 5);

/* run the state machine */
	while(1)
	{
		Update_flow(&master_stuct[0]);
		_time_delay_ticks(TICKS_PER_SEC);
		Update_Select();
	}
}

void PTP_Flow_Control_Task2 (UINT32  parameter)
{
/* put master index into .master */
	master_stuct[1].master = GRANDMASTER_ACCEPTABLE;

/* put into initial state */
	master_stuct[1].flow_state = PTP_FLOW_INIT;

/* set pointers to master ip addresses */
	master_stuct[1].master_ip_address = &nvm.nv_acc_master_ip;

/* reset all timers */
	master_stuct[1].sync_signaling_scheduled = 0;
	master_stuct[1].delay_signaling_scheduled = 0;
	master_stuct[1].announce_signaling_scheduled = 0;
	master_stuct[1].sync_signaling_ack_time = -10000;
	master_stuct[1].delay_signaling_ack_time = -10000;
	master_stuct[1].announce_signaling_ack_time = -10000;

	_time_delay_ticks(TICKS_PER_SEC * 5);

/* run the state machine */
	while(1)
	{
		Update_flow(&master_stuct[1]); /* send signaling for grandmaster */
		_time_delay_ticks(TICKS_PER_SEC);
	}
}


/* This function will transmitt Delay requests to port 319 */
void Delay_Request_Transmit_Task(UINT32  parameter)
{
	UINT16 seq;
	UINT32 sec, nsec;
	UINT8 master;
	BOOLEAN	delay_req_go = FALSE;
	UINT32 ticks = 0;
	UINT32 prev_ticks = 0;
	UINT16 pkts_per_sec = 0;
	UINT16 data[4];
	UINT16 ts[5];
	UINT16 count;
	UINT16 index;
/* start with master address */
	delay_req_ip_addr = nvm.nv_master_ip;
	delay_req_go = FALSE;
	ticks_per_delay_response = TICKS_PER_SEC / (1 << (-nvm.nv_master_sync_rate));
	ticks_per_connection_timeout = ticks_per_delay_response * 5; /* 5 seconds timeout */
	del_ticks_announce_timeout = (1 << (-nvm.nv_master_sync_rate)) *
								(nvm.nv_master_lease_dur/2);
	prev_master_delay_rate = nvm.nv_master_sync_rate; 
	prev_master_lease_dur = nvm.nv_master_lease_dur; 

	
	while(1)
	{
/* check to see if master if ready to send delay requests */
		switch(Get_Current_GM(&delay_req_ip_addr))
		{
		case GRANDMASTER_1:
			delay_req_go = TRUE;
			master = 0;
			break;
		case GRANDMASTER_2:
			master = 1;
			delay_req_go = TRUE;
			break;
		case GRANDMASTER_NONE:
			delay_req_go = FALSE;
		default: 
			break;
		}
		
		if((delay_req_go) && (delay_req_ip_addr))
		{
/* delay spacing depends on delay request rate */
			_time_delay_ticks(ticks_per_delay_response);
			
/* wait for delay response, or timeout */
//			_lwevent_wait_ticks(&Ptp_Delay_Req_Event, 1, 1, ticks_per_delay_response * 8);
 
	
/* set ip address for GM */
			Get_Current_GM(&delay_req_ip_addr);

/* increment sequence number and send packet */

			Current_Delay_Req_Seq_Num++;
			
			ticks = Get_ticks();

/* if # of packets is less than or equal to the rate then send delay request */
			if(pkts_per_sec <= (1 << (-nvm.nv_master_sync_rate)))
			{
				msgPackDelayReq(&Ptp_sock, Current_Delay_Req_Seq_Num);
				debug_printf(SEQ_PRT, "sent seq= %hd\n", Current_Delay_Req_Seq_Num);
				
				Delay_Timestamp_Buf.Seq_number_buf[Delay_Timestamp_Buf.SeqNumInIndex] = Current_Delay_Req_Seq_Num;
				Delay_Timestamp_Buf.SeqNumOutIndex = Delay_Timestamp_Buf.SeqNumInIndex;
	/* increment sequence number */
				if(++Delay_Timestamp_Buf.SeqNumInIndex >= DELAY_TIMESTAMP_SEQ_NUM_FIFO_SIZE)
				{
					Delay_Timestamp_Buf.SeqNumInIndex = 0;
				}
				
				pkts_per_sec++;	
			}
			/* check for timestamps */
			count = 2;
			while(count--)
			{
//				Get_MII_Resource();
				Read_MMFR(PHY_PG4_PTP_STS, &data[0]);
				if(data[0] & P640_TXTS_RDY)
				{
					Read_MMFR(PHY_PG4_PTP_TXTS, &ts[0]);
					Read_MMFR(PHY_PG4_PTP_TXTS, &ts[1]);
					Read_MMFR(PHY_PG4_PTP_TXTS, &ts[2]);
					Read_MMFR(PHY_PG4_PTP_TXTS, &ts[3]);
					Release_MII_Resource();
					
					index = Delay_Timestamp_Buf.InIndex;

#if 1
					_int_disable();				
					nsec = Delay_Timestamp_Buf.Data[index].nanoseconds = ((UINT32)ts[1] << 16) | (UINT32)ts[0];
					sec = Delay_Timestamp_Buf.Data[index].seconds = ((UINT32)ts[3] << 16) | (UINT32)ts[2];
					seq = Delay_Timestamp_Buf.Data[index].sequence_num = 
					    Delay_Timestamp_Buf.Seq_number_buf[Delay_Timestamp_Buf.SeqNumOutIndex]; 
//					if(++Delay_Timestamp_Buf.SeqNumOutIndex >= DELAY_TIMESTAMP_SEQ_NUM_FIFO_SIZE)
//					{
//						Delay_Timestamp_Buf.SeqNumOutIndex = 0;
//					
//					}
					if(++Delay_Timestamp_Buf.InIndex >= DELAY_TIMESTAMP_BUF_SIZE)
					{
						Delay_Timestamp_Buf.InIndex = 0;
					
					}
					Delay_Timestamp_Buf.Data[index].used_flag = FALSE;
					_int_enable();
#endif
					debug_printf(SEQ_PRT, "mmfr: seq= %hd sec= %X nsec= %X\n", seq, sec, nsec);
				}
			}
			
/* if a new second, then zero out the packet counter */
			if(prev_ticks != ticks)
			{
				prev_ticks = ticks;	
				pkts_per_sec = 0;
			}
		}
		else
		{
/* wait for a second */
			_time_delay_ticks(TICKS_PER_SEC);	
		}
		
/* update constants if the setting were changed */
		if((prev_master_delay_rate != nvm.nv_master_sync_rate) || 
		   (prev_master_lease_dur != nvm.nv_master_lease_dur))
		{
			ticks_per_delay_response = TICKS_PER_SEC / (1 << (-nvm.nv_master_sync_rate));
			ticks_per_connection_timeout = ticks_per_delay_response * 5; /* 5 seconds timeout */
			del_ticks_announce_timeout = (1 << (-nvm.nv_master_sync_rate)) *
										(nvm.nv_master_lease_dur/2);
			prev_master_delay_rate = nvm.nv_master_sync_rate; 
			prev_master_lease_dur = nvm.nv_master_lease_dur; 
			debug_printf(KEN_PTP_PRT,"changing rate: %hd)\n", prev_master_delay_rate);
		}
	}
}

void Print_flow_stuff(void)
{
	printf("ticks_per_delay_response: %ld\n",(UINT32)ticks_per_delay_response);
	printf("ticks_per_connection_timeout: %ld\n",(UINT32)ticks_per_connection_timeout);
	printf("del_ticks_announce_timeout: %ld\n",(UINT32)del_ticks_announce_timeout);
	printf("prev_master_delay_rate: %ld\n",(UINT32)prev_master_delay_rate);
	printf("ticks_per_delay_response: %ld\n",(UINT32)ticks_per_delay_response);
	printf("prev_master_lease_dur: %ld\n",(UINT32)prev_master_lease_dur);
	printf("ticks_per_delay_response: %ld\n",(UINT32)ticks_per_delay_response);
}
PTP_Flow_State Get_Flow_State(UINT8 master)
{
	return master_stuct[master].flow_state;
}

void Restart_Flow(UINT8 master)
{
	UINT32 next_scheduled_sig = Get_ticks() + 5;
	master_stuct[master].flow_state = PTP_FLOW_CHECK_ACKS;
	master_stuct[master].sync_signaling_scheduled = next_scheduled_sig;
	master_stuct[master].delay_signaling_scheduled = next_scheduled_sig;
	master_stuct[master].announce_signaling_scheduled = next_scheduled_sig;
	master_stuct[master].sync_signaling_ack_time = -10000;
	master_stuct[master].delay_signaling_ack_time = -10000;
	master_stuct[master].announce_signaling_ack_time = -10000;
	master_stuct[master].state_count = 0;	

	return;	
}

void Set_Sync_Timer (UINT8 master, UINT32 ticks)
{
    debug_printf(KEN_PTP_PRT,"%d sync timer ACK: %d\n", Get_ticks(), ticks);
	master_stuct[master].sync_signaling_scheduled = ticks;
	master_stuct[master].sync_signaling_ack_time = Get_ticks();
	
	return;
}
void Set_Delay_Timer (UINT8 master, UINT32 ticks)
{
    debug_printf(KEN_PTP_PRT,"%d delay timer ACK: %d\n", Get_ticks(), ticks);
	master_stuct[master].delay_signaling_scheduled = ticks;
	master_stuct[master].delay_signaling_ack_time = Get_ticks();

	return;
}
void Set_Announce_Timer (UINT8 master, UINT32 ticks)
{
    debug_printf(KEN_PTP_PRT,"%d announce timer ACK: %d\n", Get_ticks(), ticks);
	master_stuct[master].announce_signaling_scheduled = ticks;
	master_stuct[master].announce_signaling_ack_time = Get_ticks();

	return;
}

static PTP_Ref_State curr_ptp_ref_state;
static UINT32 ptp_ref_count;
#define REF_BRIDGE_TIME 30

static GM_select Who_should_be_GM(void)
{
	PTP_Ref_State new_ptp_ref_state;
	GM_select select = GRANDMASTER_NONE;
	BOOLEAN GM_good, ACC_GM_good;
	
	typedef union {
		struct {
		long d1;
		long d2;
		} long1;
		uint_64 longlong;
	} ident;
	
	ident GM_ident, ACC_GM_ident;
	
	GM_ident.longlong = 0, ACC_GM_ident.longlong = 0;

	GM_good = master_stuct[GRANDMASTER_PRIMARY].flow_state == PTP_FLOW_NORMAL;
	ACC_GM_good = master_stuct[GRANDMASTER_ACCEPTABLE].flow_state == PTP_FLOW_NORMAL;
	
	if(GM_good && !ACC_GM_good)
	{
		select =  GRANDMASTER_1;
	}
	else if(!GM_good && ACC_GM_good)
	{
		select =  GRANDMASTER_2;
	}
	else if(!GM_good && !ACC_GM_good)
	{
		select =  GRANDMASTER_NONE;
	}
	else
	{
/* decide using announce compares */
		memcpy(&GM_ident.longlong, 
		       &announce_message[GRANDMASTER_PRIMARY].grandmasterIdentity[0],
		       8);
		memcpy(&ACC_GM_ident.longlong, 
		       &announce_message[GRANDMASTER_ACCEPTABLE].grandmasterIdentity[0],
		       8);
//		printf("GM1 %lx:%lx GM2 %lx:%lx\n",GM_ident.long1.d1, 
//											GM_ident.long1.d2, 
//											ACC_GM_ident.long1.d1, 
//											ACC_GM_ident.long1.d2); 

/* if GM idenity the same use GM1 */
		if(ACC_GM_ident.longlong == ACC_GM_ident.longlong)
		{
			select =  GRANDMASTER_1;
		}

/* use smallest value of priority1 */
		if(announce_message[GRANDMASTER_PRIMARY].grandmasterPriority1 < 
		   announce_message[GRANDMASTER_ACCEPTABLE].grandmasterPriority1)
		{
			select =  GRANDMASTER_1;
		}
		else if(announce_message[GRANDMASTER_PRIMARY].grandmasterPriority1 > 
		        announce_message[GRANDMASTER_ACCEPTABLE].grandmasterPriority1) 
		{
			select =  GRANDMASTER_2;
		}
		else
		{
/* use smallest value of class */
			if(announce_message[GRANDMASTER_PRIMARY].grandmasterClass < 
			   announce_message[GRANDMASTER_ACCEPTABLE].grandmasterClass)
			{
				select =  GRANDMASTER_1;
			}
			else if(announce_message[GRANDMASTER_PRIMARY].grandmasterClass > 
			        announce_message[GRANDMASTER_ACCEPTABLE].grandmasterClass) 
			{
				select =  GRANDMASTER_2;
			}
			else
			{
/* use smallest value of accuracy */
				if(announce_message[GRANDMASTER_PRIMARY].grandmasterAccuracy < 
				   announce_message[GRANDMASTER_ACCEPTABLE].grandmasterAccuracy)
				{
					select =  GRANDMASTER_1;
				}
				else if(announce_message[GRANDMASTER_PRIMARY].grandmasterAccuracy > 
				        announce_message[GRANDMASTER_ACCEPTABLE].grandmasterAccuracy) 
				{
					select =  GRANDMASTER_2;
				}
				else
				{
/* use smallest value of offsetScaledLogVariance */
					if(announce_message[GRANDMASTER_PRIMARY].grandmasterClockVariance < 
					   announce_message[GRANDMASTER_ACCEPTABLE].grandmasterClockVariance)
					{
						select =  GRANDMASTER_1;
					}
					else if(announce_message[GRANDMASTER_PRIMARY].grandmasterClockVariance > 
					        announce_message[GRANDMASTER_ACCEPTABLE].grandmasterClockVariance) 
					{
						select =  GRANDMASTER_2;
					}
					else
					{
/* use greatest priority2 */
						if(announce_message[GRANDMASTER_PRIMARY].grandmasterPriority2 < 
						   announce_message[GRANDMASTER_ACCEPTABLE].grandmasterPriority2)
						{
							select =  GRANDMASTER_1;
						}
						else if(announce_message[GRANDMASTER_PRIMARY].grandmasterPriority2 > 
						        announce_message[GRANDMASTER_ACCEPTABLE].grandmasterPriority2) 
						{
							select =  GRANDMASTER_2;
						}
						else
						{
#if 0
/* use greatest identity */
							if(GM_ident.longlong > ACC_GM_ident.longlong)
							{
								select =  GRANDMASTER_1;
							}
							if(GM_ident.longlong < ACC_GM_ident.longlong)
							{
								select =  GRANDMASTER_2;
							}
							else
							{
								select =  GRANDMASTER_1;
							}
#else
								select =  GRANDMASTER_1;
#endif
						}
					}
				}
			}
		}
	}
		
	if(!nvm.nv_ptp_state_on)
	{
		select =  GRANDMASTER_NONE;
	}

/* update reference state machine */
	new_ptp_ref_state = curr_ptp_ref_state;
	switch(curr_ptp_ref_state)
	{
	case PTP_REF_GM1:
		if(select == GRANDMASTER_2)
		{
			setfreq_switchover(getfreq());
			new_ptp_ref_state = PTP_REF_BRIDGE_GM1_TO_GM2;
		}
		else if(select == GRANDMASTER_NONE)
		{
			new_ptp_ref_state = PTP_REF_NONE;
		}
		break;
	case PTP_REF_GM2:
		if(select == GRANDMASTER_1)
		{
			setfreq_switchover(getfreq());
			new_ptp_ref_state = PTP_REF_BRIDGE_GM2_TO_GM1;
		}
		else if(select == GRANDMASTER_NONE)
		{
			new_ptp_ref_state = PTP_REF_NONE;
		}
		break;
	case 	PTP_REF_BRIDGE_GM1_TO_GM2:
		if(select == GRANDMASTER_1)
		{
			setfreq_switchover(getfreq());
			new_ptp_ref_state = PTP_REF_BRIDGE_GM2_TO_GM1;			
		}
		else if(ptp_ref_count > REF_BRIDGE_TIME)
		{
			new_ptp_ref_state = PTP_REF_GM2;
		}
		else if(select == GRANDMASTER_NONE)
		{
			new_ptp_ref_state = PTP_REF_NONE;
		}
		break;
	case PTP_REF_BRIDGE_GM2_TO_GM1:
		if(select == GRANDMASTER_2)
		{
			setfreq_switchover(getfreq());
			new_ptp_ref_state = PTP_REF_BRIDGE_GM1_TO_GM2;			
		}
		else if(ptp_ref_count > REF_BRIDGE_TIME)
		{
			new_ptp_ref_state = PTP_REF_GM1;
		}
		else if(select == GRANDMASTER_NONE)
		{
			new_ptp_ref_state = PTP_REF_NONE;
		}
		break;
	case PTP_REF_NONE:
	default:
		if(select == GRANDMASTER_1)
		{
			setfreq_switchover(getfreq());
			new_ptp_ref_state = PTP_REF_BRIDGE_GM2_TO_GM1;
		}
		else if(select == GRANDMASTER_2)
		{
			setfreq_switchover(getfreq());
			new_ptp_ref_state = PTP_REF_BRIDGE_GM1_TO_GM2;
		}
		break;
	}

	ptp_ref_count++;
	
	if(new_ptp_ref_state != curr_ptp_ref_state)
	{
		switch(new_ptp_ref_state)
		{
			break;
		case PTP_REF_GM1:
			Send_Log(LOG_ALARM, LOG_TYPE_REF_CHANGE, GRANDMASTER_PRIMARY, 0, 0);
			break;
		case PTP_REF_GM2:
			Send_Log(LOG_ALARM, LOG_TYPE_REF_CHANGE, GRANDMASTER_ACCEPTABLE, 0, 0);
			break;
		case PTP_REF_BRIDGE_GM1_TO_GM2:
		case PTP_REF_BRIDGE_GM2_TO_GM1:
		case PTP_REF_NONE:
		default:
			break;
		}
		ptp_ref_count = 0;
		curr_ptp_ref_state = new_ptp_ref_state;
	}
	current_GM = select;
	
	debug_printf(SELECT_DEBUG_PRT, "select GM: %d ref state: %d ref count: %d\n", 
				(int)select, (int)curr_ptp_ref_state, (int)ptp_ref_count);

	return select;
}

static void Update_Select(void)
{
	GM_select select = Who_should_be_GM();
}

PTP_Ref_State Get_Ref_State(UINT32 *count)
{
	*count = ptp_ref_count;
	
	return curr_ptp_ref_state;
}

GM_select Get_Current_GM(UINT32 *ip)
{
	switch(current_GM)
	{
	case GRANDMASTER_1:
		*ip = nvm.nv_master_ip;
		break;		
	case GRANDMASTER_2:
		*ip = nvm.nv_acc_master_ip;
		break;		
	default:
	case GRANDMASTER_NONE:
		*ip = 0;
		break;		
	}

	return current_GM;
}

