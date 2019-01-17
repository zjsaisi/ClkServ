/* ptp.h
*/
#include "proj_inc.h"
#include "sc_ptp_servo.h"
//#include <lwevent.h>
//include <mcfevb5235.h>
//#include <rtcs.h>

#define TICKS_PER_SEC  (BSP_ALARM_FREQUENCY)

//typedef enum CHOICES FOR DEFAULT OSCILLATOR
//{
//  e_RB        = (0),   /* Rubidium Oscillator */
//  e_DOCXO     = (1),   /* Double oven OCXO    */
//  e_OCXO      = (2),   /* OCXO                */
//  e_MINI_OCXO = (3),   /* Mini- OCXO          */
//  e_TCXO      = (4)    /* TCXO                */
//} t_loTypeEnum;


//#define DEFAULT_OCXO e_MINI_OCXO
#if PRODUCT_ID == TP1500_PROJECT
#define DEFAULT_OCXO e_RB
#else
//#define DEFAULT_OCXO e_MINI_OCXO
#define DEFAULT_OCXO e_OCXO
#endif

#define MINIMAL_PRINT 0
#define N0_1SEC_PRINT 0 
#define SC_FASTER_SERVO_MODE 0 //GPZ NOV 2010 Added this flag to allow the general softclient the be more temperature immune 
#define SYNC_E_ENABLE 0 // global flag for sync E application code
#define LOW_PACKET_RATE_ENABLE 1 // global flag for support lower update rates

#define ASYMM_CORRECT_ENABLE 0  // global flag for power industry 1pps assymmetry correction
#define HYDRO_QUEBEC_ENABLE 0 // global flag for power industry 1pps assymmetry correction
//#define CROSSOVER_TS_GLITCH 0 // flag used to add glitch detection on TimeStamp input flow
//#define DSL 0 //testing DSL applications

//#define TDEV_WINDOWS (1)
//#define TDEV_CAT 0 // critical TDEV category
//#define LSF_WINDOW_MAX	 (1024)	 //was 512 increase to 1024 for RB test purpose 
//#define LSF_WINDOW_LARGE (300)   //was 256 dropped to 240 March 2010 
//#define LSF_WINDOW_SMALL (4)

//#define RFE_WINDOW_MAX (32)
//#define RFE_WINDOW_LARGE (300)
//#define IPP_CHAN 2 //Input process channels 2 for now may be quad 

#if(SYNC_E_ENABLE==1)
#define SE_WINDOW_MAX (32) // maximum one second sample history
#define SE_CHANS (1)
#define SE_WINDOW (8) // size of measurement window
#define SE_MAX_PHASE (125000000) //based on 125MHz counter clock for softclient
#define SE_LSB (1.0) // Sync E  measurement resolution ns 1 over SE_MAX_PHASE softclient
#define SE_DIFF_THRES 2000  
#define SE_SECOND_DIFF_THRES 500
#endif

#define VAR_TABLE_SIZE (64) //maximum size of varactor correction table

//#if((ENTERPRISE ==1))
//#define OCXO 2//set to one for OCXO build set to 2 for mini-OCXO build
//#else
//#define OCXO 0//set to one for OCXO build set to 2 for mini-OCXO build 0 for Rb
//#endif
//#if OCXO
//#define CLIENT_IP_ADDR (0x64000066)//100.0.0.102
//#else
//#define CLIENT_IP_ADDR (0x64000067)//100.0.0.103 
//#endif
//#define MASTER_IP_ADDR (0x64000065)//100.0.0.101
#define PTP_MESS_SIGNALING 0x0C
#define PTP_MESS_DEL_RESP  0x09


#define DELAY_TIMESTAMP_BUF_SIZE 128
#define DELAY_TIMESTAMP_SEQ_NUM_FIFO_SIZE 8

 
typedef struct {
	uint_32 seconds;
	uint_32 nanoseconds;
	uint_16 sequence_num;
	uint_8  used_flag;
} DELAY_TIMESTAMP_STRUCT;

typedef volatile struct {
	DELAY_TIMESTAMP_STRUCT	Data[DELAY_TIMESTAMP_BUF_SIZE];/* buffer for writes to MMFR   (page + reg)  */
	uint_16 InIndex;             /* points to next   empty entry position    */ 
	uint_16 OutIndex;            /* points to oldest unused position         */ 
	uint_16 Seq_number_buf[DELAY_TIMESTAMP_SEQ_NUM_FIFO_SIZE];/* Stores a FIFO of outgoing sequence numbers */
	uint_16 SeqNumInIndex;              /* points to next entry position to enter sequence number */
	uint_16 SeqNumOutIndex;             /* points to next entry position to Read sequence number  */
} DELAY_TIMESTAMP_BUF_STRUCT;

typedef enum
{
	GRANDMASTER_NONE	=0,		
	GRANDMASTER_1,		
	GRANDMASTER_2,		
} GM_select;


#pragma pack(1)
struct ptp_header {
	unsigned char TsMt;
	unsigned char RsvVer;
	unsigned short int MsgLen;
	unsigned char Domain;
	unsigned char Rsv0;
	unsigned short int Flags;
	unsigned char Correction[8];
	unsigned char Rsv1[4];
	unsigned char SrcPortIdentity[10];
	unsigned short int SeqNo;
	unsigned char Control;
	char LogMeanMsgInterval;
};
#pragma pack()

#pragma pack(1)
struct ptp_announce {
	struct ptp_header	hdr;
	unsigned char OriginTimestamp[10];
	unsigned short int originCurrentUTCOffset;
	unsigned char reserved;
	unsigned char grandmasterPriority1;

/* ClockQuality */	
	unsigned char grandmasterClass;
	unsigned char grandmasterAccuracy;
	unsigned short int grandmasterClockVariance;
	
	unsigned char grandmasterPriority2;
	unsigned char grandmasterIdentity[8];
	unsigned short int stepsRemoved;
	unsigned char timeSource;
};
#pragma pack()


#define PTP_LI_61               0x0001
#define PTP_LI_59               0x0002
#define PTP_UTC_REASONABLE      0x0004
#define PTP_UTC_TIMESCALE       0x0008
#define PTP_TIME_TRACEABLE      0x0010
#define PTP_FREQUENCY_TRACEABLE 0x0020
#define PTP_ALT_MASTER          0x0100
#define PTP_TWO_STEP            0x0200
#define PTP_UNICAST             0x0400

#pragma pack(1)
struct ptp_tlv {
	struct ptp_header	hdr;
	unsigned char TargetPortID[10];
	unsigned short int tlvType;
	unsigned short int length;
	unsigned char value[1024];
};
#pragma pack()

struct ptp_sync_delay {
	struct ptp_header	hdr;
	unsigned char OriginTimestamp[10];
};

struct ptp_delay_resp {
	struct ptp_header	hdr;
	unsigned char ReceiveTimestamp[10];
	unsigned char RequestingPortIdentity[10];
};

struct ptp_sig_grant {
	struct ptp_header	hdr;
	unsigned char TargetPortIdentity[10];
	unsigned short TlvType;
	unsigned short LengthField;
	unsigned char  MtRv;
	unsigned char  LogInerMessagePeriod;
	unsigned char  DurationField[4];
	unsigned char  Reserved;
};
typedef enum
{
	VCAL_START	=0, //start varactor cal initialize working copy of cal data
	VCAL_START_BASELINE, //baseline zero start cal value
	VCAL_CYCLE, //main vcal cycle
	VCAL_STOP_BASELINE, //baseline zero end cal value
	VCAL_COMPLETE //complete vcal cycle
} VCAL_State;
typedef enum
{
	FORWARD_GM1 =0,
	REVERSE_GM1,
	FORWARD_GM2,
	REVERSE_GM2
} INPUT_CHANNEL;

#if(NO_PHY == 0)
typedef enum
{
	GPS_A =0,
	GPS_B,
	GPS_C,
	GPS_D,
	SE_A,
	SE_B,
    RED
} INPUT_CHANNEL_PHY;
#endif

typedef enum
{
    PTP_REF_NONE      = 0,
	PTP_REF_GM1,
	PTP_REF_GM2,
	PTP_REF_BRIDGE_GM1_TO_GM2,
	PTP_REF_BRIDGE_GM2_TO_GM1		
} PTP_Ref_State;
#if 0
typedef enum
{
    XFER_IDLE      = 0,
	XFER_START,
	XFER_END
} 	XFER_State;
#endif

typedef enum
{
    PTP_FLOW_INIT      = 0,
	PTP_FLOW_CHECK_ACKS,
	PTP_FLOW_CHECK_SYNC,
	PTP_FLOW_CHECK_ANNOUNCE,
	PTP_FLOW_NORMAL,
	PTP_FLOW_SUSPEND,	
	PTP_FLOW_CHECK_DELAY
} PTP_Flow_State;

typedef struct {
PTP_Flow_State  flow_state;
uint_32 		state_count;
int_32			sync_signaling_scheduled;
int_32			delay_signaling_scheduled;
int_32			announce_signaling_scheduled;
int_32			sync_signaling_ack_time;
int_32			delay_signaling_ack_time;
int_32			announce_signaling_ack_time;

uint_32			*master_ip_address;
uint_32			connection_trys;
uint_8			master;
uint_32			bad_sync_cntr;
uint_32			bad_delay_cntr;
uint_32			sock;
} FLOW_STRUCT_TYPE;


#define CLEAR_SYNC_ACK_FLAG 0
#define DO_NOT_CLEAR_SYNC_ACK_FLAG 1


extern boolean Is_Sync_Flow_Ack(uint_8 master, int clear_flag);
extern boolean Is_Delay_Flow_Ack(uint_8 master, int clear_flag);
extern boolean Is_Announce_Flow_Ack(uint_8 master, int clear_flag);


typedef enum
{
   PTP_START      = 0,
	PTP_SET_PHASE,
	PTP_LOCKED,
   PTP_MAJOR_TS_EVENT_WAIT,
   PTP_MAJOR_TS_EVENT,
   PTP_MAJOR_TS_EVENT_END
} PTP_State;

extern int_32 test_correction1;
extern int_32 test_correction2;
extern struct ptp_announce announce_message[];
extern uint_32 ptp_announce_time[];
extern uint_32 current_lease_duration[2];
extern uint_8 gm_clock_id[2][8];
extern uint_8 gm_source_port_id[2][2];
//extern LWEVENT_STRUCT Ptp_Delay_Req_Event;
extern uint_16 Current_Delay_Req_Seq_Num;
extern boolean signaling_sync_ack_flag[];
extern boolean signaling_delay_ack_flag[];
extern boolean signaling_announce_ack_flag[];
extern uint_16  number_of_delay_packets;
extern uint_16  number_of_sync_packets_for_timeout[];
extern uint_16  number_of_delay_packets_for_timeout;
extern uint_16 	PTP_Seq_Num_curr;
extern FIFO_STRUCT PTP_Delay_Fifo_Local;
extern uint_8 	PTP_Seq_Num_prev;
extern uint_16 		Num_of_missed_pkts;
extern uint_16  number_of_sync_packets[];
extern FIFO_STRUCT PTP_Fifo[],PTP_Fifo_Local;
#if(NO_PHY == 0)
extern FIFO_STRUCT GPS_A_Fifo_Local,GPS_B_Fifo_Local, GPS_C_Fifo_Local,GPS_D_Fifo_Local, SYNCE_A_Fifo_Local,SYNCE_B_Fifo_Local,RED_Fifo_Local;
#endif
extern FLOW_STRUCT_TYPE master_stuct[];
extern uint_8 init_phy_state;		
extern uint_16  number_of_sync_RTD_filtered;
extern uint_16  number_of_delay_RTD_filtered;

extern void Minute_Task (uint_32  parameter);
#if 0
extern void PTP_Port320_listen_Task (uint_32  parameter);
#endif
#if 1
extern void PTP_Port320_listen0_Task (uint_32  parameter);
extern void PTP_Port320_listen1_Task (uint_32  parameter);
extern void PTP_Port320_listen2_Task (uint_32  parameter);
extern void PTP_Port320_listen3_Task (uint_32  parameter);
extern void PTP_Port320_listen4_Task (uint_32  parameter);
#endif
extern void Delay_Request_Transmit_Task (uint_32  parameter);
extern void PTP_Flow_Control_Task (uint_32  parameter);
extern void PTP_Flow_Control_Task2 (uint_32  parameter);
#if 0
extern void PTP_Port319_listen_Task (uint_32  parameter);
#endif
#if 1
extern void PTP_Port319_listen0_Task (uint_32  parameter);
extern void PTP_Port319_listen1_Task (uint_32  parameter);
extern void PTP_Port319_listen2_Task (uint_32  parameter);
extern void PTP_Port319_listen3_Task (uint_32  parameter);
extern void PTP_Port319_listen4_Task (uint_32  parameter);
#endif
extern void PTP_PPS_Task (uint_32  parameter);
extern void PTP_Timeout_Task (uint_32  parameter);
extern void Set_Master_IP(uint_32 master_addr);
extern uint_32 Get_Master_IP(void);
#if 1
extern void Set_Client_IP(uint_32 client_addr, uint_32 mask, uint_32 gateway);
#endif
#if 0
extern void Set_Client_IP(uint_16 eth, uint_32 client_addr, uint_32 mask, uint_32 gateway, uint_8 redo_dhcp);
#endif
extern uint_32 Get_Client_IP(void);
//extern _rtcs_if_handle ihandle0;

extern int_16 tx_uni_req (uint_32 sock, uint_32 where, int_16 type, uint_32 duration, int_16 rate,  unsigned char *clock_id);
extern int_16 tx_cancel_uni_req(uint_32 sock, uint_32 where, int_16 type, uint_32 duration, int_16 rate,  unsigned char *clock_id);
extern void Turn_all_flows_off(void);
extern void Get_Flow_Status(uint_16 *sync, uint_16 *delay, uint_8 master);
extern void Get_Flow_Status_RTD(uint_16 *sync, uint_16 *delay);
extern uint_8 Is_SYNC_Flowing(uint_8 master);
extern uint_8 Is_DELAY_RESP_Flowing(void);
extern int get_rate(int_8 data);
extern PTP_Flow_State Get_Flow_State(uint_8 master);
extern uint_32 	Ptp_sock_delay;
extern uint_32 	Ptp_sock;
extern int tx_uni_dreq (unsigned long where, unsigned char *buf, unsigned char *clock_id);
extern uint_16 get_fll_led_state(void);
extern uint_16 Is_Sync_timeout(uint_8 master);
extern uint_16 read_from_MMFR_no_paging(uint_16 reg);

#if 1
extern void Shutdown_PTP_sockets(void);
extern void Restart_PTP_sockets(uint_32 client_addr);
#endif
#if 0
extern void Shutdown_PTP_sockets_for_port(uint_16 eth);
extern void Restart_PTP_sockets_for_port(uint_32 client_addr);
#endif
#define GRANDMASTER_PRIMARY    0
#define GRANDMASTER_ACCEPTABLE 1
#define GRANDMASTER_MANAGEMENT 2
extern void Restart_Flow(uint_8 master);
extern void Set_Sync_Timer (uint_8 master, uint_32 ticks);
extern void Set_Delay_Timer (uint_8 master, uint_32 ticks);
extern void Set_Announce_Timer (uint_8 master, uint_32 ticks);

extern double Get_fdrift(void);
extern void Set_fdrift(double value);
extern boolean Is_FLL_Elapse_Sec_Count_gt_20(void);
extern boolean Is_FLL_Elapse_Sec_Count_gt_400(void);
extern void Init_PHY_Resource(void);
extern void Set_Seq_Num(uint_16 seq_num);
extern void set_mode_compression_flag(int flag);
extern GM_select Get_Current_GM(uint_32 *ip);
extern PTP_Ref_State Get_Ref_State(uint_32 *count);
extern uint_8 Signaling_Ack_Packet_Good(struct ptp_sig_grant *pkt, uint_16 *error, int master);
extern uint_8 Delay_Resp_Packet_Good(struct ptp_sync_delay *pkt, uint_16 *error);
extern uint_8 Sync_Packet_Good(struct ptp_sync_delay *pkt, uint_16 *error);
extern uint_8 Announce_Packet_Good(struct ptp_announce *pkt, uint_16 *error);
extern uint_8 Sig_Acks_are_good(uint_8 master);
extern uint_8 Announce_Sig_Acks_are_good(uint_8 master);
extern uint_8 Delay_Sig_Acks_are_good(uint_8 master);
extern boolean Is_GM_Lock_Holdover(uint_8 master);
extern boolean Is_Announce_Timeout(uint_8 master);

extern int_32 getfreq(void);
extern int setfreq(INT32 freq, INT8 isPHY);
extern BOOLEAN Is_PTP_FREQ_ACTIVE(void);
extern BOOLEAN Is_PHY_FREQ_ACTIVE(void);
extern void clr_residual_phase(void);




 
#if(SYNC_E_ENABLE==1)

	// Data structures to support Synchronous Ethernet mode
	typedef struct
 	{
 		int_32 Offset; //raw phase measurement 
 		int_8 Valid;	//Indicates current measurement was performed with a valid measurement channel
 	} 
	SE_MEAS_FIFO;
 	typedef struct
  	{
  		int_8  index;
  		int_32 total_meas_cur, total_meas_lag; 
  		int_16 cnt_meas_cur, cnt_meas_lag;
// 		int_32 baseline_delta, baseline_phase;
  		int_16 phase_pop_cnt;
 		int_8  se_valid; //indicates the synchronous ethernet is valid for use
  		double freq_est; // loop frequency estimate ppb
  		double freq_se_corr;  // correction to be applied (open - holdover from PTP)
  		double freq_se_hold;  // holdover se freq to apply
  		double alpha_tracking, alpha_holdover;
  		SE_MEAS_FIFO SMF[SE_WINDOW_MAX]; 
  	}
	SE_MEAS_CHAN;
#endif
 
#if (ASYMM_CORRECT_ENABLE==1)
	typedef enum
	{
    	ASYMM_UNKNOWN      = 0,
		ASYMM_OPERATE,
		ASYMM_CALIBRATE,
	} ASYMM_STATE;
	typedef enum
	{
		ASYMM_UN_CALIBRATED		=0,
		ASYMM_UN_CALIBRATED_A,
		ASYMM_UN_CALIBRATED_B,
		ASYMM_UN_CALIBRATED_C,
		ASYMM_UN_CALIBRATED_D,
		ASYMM_UN_CALIBRATED_E,
    	ASYMM_PRE_CALIBRATED,
		ASYMM_AUTONOMOUS,
	} ASYMM_CATEGORY;
	typedef struct
	{
		ASYMM_CATEGORY 	cal_category;
		int_16	cal_count; //number of samples included in the estimate
		double	calibration;
		double 	time_bias_meas;
		double 	time_bias_meas_A;
		double 	time_bias_meas_B;
		double 	time_bias_meas_C;
		double 	time_bias_meas_D;
		double 	time_bias_meas_E;
		double 	bias_asymm_factor;
		double	residual_noise;
		int_32	time_last_used;
		double	rtd_min;
		double 	rtd_mean;
		double 	rtd_max;
	}ASYMM_CORR_ELEMENT;
	typedef struct
	{
		int_8 	index;
		int		skip;
		ASYMM_STATE	astate;
		double asymm_calibration;
		double time_bias_baseline;
		ASYMM_CORR_ELEMENT ACE[32];
	}
	ASYMM_CORR_LIST;
#endif
extern uint_8 Get_Delay_Timestamp(uint_16 seq_num, uint_32 *sec, uint_32 *nsec);
extern uint_16 Get_MII_Resource(void);
extern uint_16 Release_MII_Resource(void);          
extern void Get_Sync_Time(uint_8 master, uint_32 *time);
extern void Get_Sync_Flags(uint_8 master, uint_16 *flags);
extern void Start_Multicast_Listen(int interface);
extern void Stop_Multicast_Listen(void);

extern int_32 Get_Sync_Correction(uint_8 master);
extern void Avg_and_clr_Delay_Sync_Correction(void);
extern int_32 Get_Delay_Correction(void);
extern void Avg_and_clr_Delay_Correction(void);

extern uint_16 Get_Sample_Flag(uint_8 master);
extern void test_leap(uint_16 flag);
extern uint_8 Pick_Best_Announce(struct ptp_announce *announce0, struct ptp_announce *announce1);
extern uint_32 Get_Multicast_IP(void);
extern void Clear_Multicast_IP(void);

extern float Get_out_tdev_est(void);

extern INT32 Get_RTD_1sec_min(void);
extern void Reset_RTD(void);
extern uint_32 Calc_Instant_RTD(uint_32 t1_sec, uint_32 t1_nsec,
						uint_32 t2_sec, uint_32 t2_nsec,
						uint_32 t3_sec, uint_32 t3_nsec,
						uint_32 t4_sec, uint_32 t4_nsec);
extern uint_8 Temperature_data_ready(void);
extern void Print_FIFO_Buffer(uint_16 index, uint_16 fast);
extern int get_holdover_code(void);
extern void Restart_Phase_Time(void);
extern void init_phy(void);
extern void init_ipp_phy(void);
extern void init_rm(void);
extern void start_rfe_phy(double in);
extern void fll_status_1min(void);
extern double sqrt_aprox(double f64_value);
extern UINT16 Is_Delay_timeout(void); //based on 64 rate
extern UINT32 Get_Tick(void);
extern void Restart_Phase_Time(void);









