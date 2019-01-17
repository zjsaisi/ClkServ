#ifndef __CLK_H__
#define __CLK_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* ptp.h
*/
//#include <lwevent.h>
//#include <mcfevb5235.h>
//#include <rtcs.h>
//#include "Target.h"

#define ENTERPRISE 0
#define ENABLE_TIME 1
#define SYNC_E_ENABLE 0 // global flag for sync E application code
#define CROSSOVER_TS_GLITCH 0

#define TICKS_PER_SEC  (BSP_ALARM_FREQUENCY)

//#define DSL 0 //testing DSL applications
#define FIFO_BUF_SIZE 128//was (4096)
#define TDEV_WINDOWS (1)
#define TDEV_CAT 0 // critical TDEV category

//#define LSF_WINDOW_MAX	 (512)	
//#define LSF_WINDOW_LARGE (256)
//#define LSF_WINDOW_SMALL (4)
#define VAR_TABLE_SIZE (64) //maximum size of varactor correction table

#define RFE_WINDOW_MAX (32)
#define RFE_WINDOW_LARGE (300)
#define IPP_CHAN 2 //Input process channels 2 for now may be quad 

#if(SYNC_E_ENABLE==1)
#define SE_WINDOW_MAX (32) // maximum one second sample history
#define SE_CHANS (1)
#define SE_WINDOW (8) // size of measurement window
#define SE_MAX_PHASE (125000000) //based on 125MHz counter clock for softclient
#define SE_LSB (8.0) // Sync E  measurement resolution ns 1 over SE_MAX_PHASE softclient
#define SE_DIFF_THRES 200
#define SE_SECOND_DIFF_THRES 10
#endif

//#if((ENTERPRISE ==1))
//#define OCXO 2//set to one for OCXO build set to 2 for mini-OCXO build
//#else
//#define OCXO 1//set to one for OCXO build set to 2 for mini-OCXO build 0 for Rb (kjh)
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
	UINT32 seconds;
	UINT32 nanoseconds;
	UINT16 sequence_num;
	UINT8  used_flag;
} DELAY_TIMESTAMP_STRUCT;

typedef volatile struct {
	DELAY_TIMESTAMP_STRUCT	Data[DELAY_TIMESTAMP_BUF_SIZE];/* buffer for writes to MMFR   (page + reg)  */
	UINT16 InIndex;             /* points to next   empty entry position    */ 
	UINT16 OutIndex;            /* points to oldest unused position         */ 
	UINT16 Seq_number_buf[DELAY_TIMESTAMP_SEQ_NUM_FIFO_SIZE];/* Stores a FIFO of outgoing sequence numbers */
	UINT16 SeqNumInIndex;              /* points to next entry position to enter sequence number */
	UINT16 SeqNumOutIndex;             /* points to next entry position to Read sequence number  */
} DELAY_TIMESTAMP_BUF_STRUCT;


typedef struct time_struct
{

   /* The number of seconds in the time.  */
   UINT32     SECONDS;

   /* The number of milliseconds in the time. */
   UINT32     MILLISECONDS;

} TIME_STRUCT;

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
typedef struct 
{
	INT32 ipdv_max;
	INT32 under_thres_cnt;
	INT32 total_thres_cnt;
	INT32 over_thres_99_9_cnt;
	INT32 thres_99_9;
	INT32 jitter_sum;
	INT32 jitter_samp;
	INT16 jitter_cnt;
}
 IPPM_ELEMENT_STRUCT;
typedef struct 
{
	double jitter;
	double ipdv;
	double ipdv_99_9;
	double thres_prob;
}
 IPPM_OUTPUT_STRUCT;
typedef struct 
{
	double cur_jitter_1sec, cur_jitter_1min, out_jitter;
	double cur_ipdv_1sec, cur_ipdv_1min, out_ipdv;
	double cur_ipdv_99_9_1sec, cur_ipdv_99_9_1min,   out_ipdv_99_9;
	double cur_thres_prob_1sec, cur_thres_prob_1min, out_thres_prob;
	INT16 ies_head, ios_head; //index to head of queue
	INT16 pacing; //pacing skip factor for jitter calculation
	INT32 thres;  // threshold for accepting delay sample (ns)
	INT32 slew_gain; //attack rate for 99.9 IPDV 
	INT16 slew_gap;  // consecutive samples with under slew
	INT16 window; // observation window in minutes
	INT16 N_valid; // number of valid minute stats entries
	IPPM_ELEMENT_STRUCT ies[256]; //ippm stat circular queue one second process	
	IPPM_OUTPUT_STRUCT  ios[256]; //256 minute circular minute queue
}
IPPM_STRUCT;

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
typedef enum
{
    PTP_REF_NONE      = 0,
	PTP_REF_GM1,
	PTP_REF_GM2,
	PTP_REF_BRIDGE_GM1_TO_GM2,
	PTP_REF_BRIDGE_GM2_TO_GM1		
} PTP_Ref_State;
typedef enum
{
    XFER_IDLE      = 0,
	XFER_START,
	XFER_END
} 	XFER_State;

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
UINT32 		state_count;
UINT32			sync_signaling_scheduled;
UINT32			delay_signaling_scheduled;
UINT32			announce_signaling_scheduled;
INT32			sync_signaling_ack_time;
INT32			delay_signaling_ack_time;
INT32			announce_signaling_ack_time;

UINT32			*master_ip_address;
UINT32			connection_trys;
UINT8			master;
UINT32			bad_sync_cntr;
UINT32			bad_delay_cntr;
UINT32			sock;
} FLOW_STRUCT_TYPE;


#define CLEAR_SYNC_ACK_FLAG 0
#define DO_NOT_CLEAR_SYNC_ACK_FLAG 1


extern BOOLEAN Is_Sync_Flow_Ack(UINT8 master, int clear_flag);
extern BOOLEAN Is_Delay_Flow_Ack(UINT8 master, int clear_flag);
extern BOOLEAN Is_Announce_Flow_Ack(UINT8 master, int clear_flag);


typedef enum
{
    PTP_START      = 0,
	PTP_SET_PHASE,
	PTP_ACQUIRE_FREQ,
	PTP_LOCKED
} PTP_State;
typedef enum
{
    Lock_Off      = 0,
	Lock_Green_Blink,
	Lock_Green_On,
	Lock_Yellow_On,
	Lock_Yellow_Blink
} Lock_State;
typedef enum
{
	FLL_UNKNOWN	=0,		
    FLL_WARMUP,
	FLL_FAST,	// Detilt or Fast Gear
	FLL_NORMAL, //Clock Normal or Bridge
	FLL_BRIDGE, //Clock Normal with slew counts
	FLL_HOLD
} Fll_Clock_State;
typedef enum
{
	GRANDMASTER_NONE	=0,		
	GRANDMASTER_1,		
	GRANDMASTER_2,		
} GM_select;

typedef struct
{
	Fll_Clock_State cur_state;
	Fll_Clock_State prev_state;
	UINT32 current_state_dur;
	UINT32 previous_state_dur;
	Lock_State lock;
	double f_avg_tp15min, f_avg_tp60min; //rate of transients per 15 minute and per hour
	double r_avg_tp15min, r_avg_tp60min; 
	double f_oper_mtdev,r_oper_mtdev;
	double f_oper_mode_tdev,r_oper_mode_tdev;
	double f_mode_width, r_mode_width;
	double f_min_cluster_width, r_min_cluster_width;
	double out_tdev_est, out_mdev_est;
	double f_cdensity, r_cdensity;
	double f_weight, r_weight;
	double freq_cor, time_cor,residual_time_cor;
	double fll_min_time_disp;
	double rtd_us; //minimal rtd in microseconds
	double t_max;
	double t_min;
	double t_avg;
	double t_std_5min;
	double t_std_60min; 
   double f_mafie, r_mafie;  //15 minute operational mafie ppb
	double f_cw_avg, r_cw_avg;	
	double f_cw_std, r_cw_std;
} FLL_STATUS_STRUCT;
typedef struct 
{
    UINT32 client_sec;
	UINT32 master_sec;
    UINT32 client_nsec;
    UINT32 master_nsec;
    INT32  offset;
    UINT16 seq_num;
	UINT16 flag;
} FIFO_ELEMENT_STRUCT;

typedef struct 
{
	FIFO_ELEMENT_STRUCT fifo_element[FIFO_BUF_SIZE];
    UINT16 fifo_in_index;   /* index to the last entry that was entered */
    UINT16 fifo_out_index;  /* index to the oldest unconsumed array element */
} FIFO_STRUCT;
#define PTP_SYNC_TRANSFER_QUEUE 	 0
#define PTP_DELAY_TRANSFER_QUEUE  1


#define FIFO_QUEUE_BUF_SIZE (128 * 3 * 5) /* (128 pkts/s * 3 buffers * 5 seconds) */
typedef struct 
{
	FIFO_ELEMENT_STRUCT  fifo_element[FIFO_QUEUE_BUF_SIZE];
	UINT8 queue_index [FIFO_QUEUE_BUF_SIZE];
	UINT16 out_index;  /* oldest unread data element in queue          */
	UINT16 in_index;   /* where to insert newest data element in queue */
} FIFO_QUEUE_STRUCT;

 typedef struct
 {
 	INT32 floor_cluster_count; //count over phase interval of samples in floor cluster
 	INT32 ofw_cluster_count;	//count over phase interval of samples in ceiling cluster
 	
 	long long floor_cluster_sum;
 	long long ofw_cluster_sum;
 	
 	INT32 floor;  //minimum in one  interval use to replace working phase floor
 	INT32 ceiling;//maximum in one  interval
  } 
  INPROCESS_CHANNEL_STRUCT;
  
  typedef struct
  {
  	INT32 phase_floor_avg; // cluster phase floor average
  	INT32 phase_ceiling_avg; //cluster phase ceiling average
  	INT32 working_phase_floor; //baseline phase_floor 
  	INPROCESS_CHANNEL_STRUCT IPC; 
  }
  INPROCESS_PHASE_STRUCT;
  typedef struct
  {
  	INT16 phase_index; 
  	INPROCESS_PHASE_STRUCT PA[IPP_CHAN],PB[IPP_CHAN],PC[IPP_CHAN],PD[IPP_CHAN]; //one structure for each channel
  	//outputs from process//
  	INT32 working_phase_floor[IPP_CHAN]; // current working phase floor 
  	INT32 phase_floor_avg[IPP_CHAN]; // current phase floor average
  	INT32 ofw_cluster_count[IPP_CHAN]; // cluster count
  	INT32 floor_cluster_count[IPP_CHAN]; // cluster count
  }
  INPUT_PROCESS_DATA;

#if(SYNC_E_ENABLE==1)

	// Data structures to support Synchronous Ethernet mode
	typedef struct
 	{
 		INT32 Offset; //raw phase measurement 
 		INT8 Valid;	//Indicates current measurement was performed with a valid measurement channel
 	} 
	SE_MEAS_FIFO;
 	typedef struct
  	{
  		INT8  index;
  		INT32 total_meas_cur, total_meas_lag; 
  		INT16 cnt_meas_cur, cnt_meas_lag;
// 		int_32 baseline_delta, baseline_phase;
  		INT16 phase_pop_cnt;
 		INT8  se_valid;
  		double freq_est; // loop frequency estimate ppb
  		double freq_se_corr;  // correction to be applied (open - holdover from PTP)
  		double freq_se_hold;  // holdover se freq to apply
  		double alpha_tracking, alpha_holdover;
  		SE_MEAS_FIFO SMF[SE_WINDOW_MAX]; 
  	}
	SE_MEAS_CHAN;
#endif

extern INT32 test_correction1;
extern INT32 test_correction2;
extern struct ptp_announce announce_message[];
extern UINT32 ptp_announce_time[];
extern UINT8 gm_clock_id[2][8];
//extern LWEVENT_STRUCT Ptp_Delay_Req_Event;
extern UINT16 Current_Delay_Req_Seq_Num;;
extern BOOLEAN signaling_sync_ack_flag[];
extern BOOLEAN signaling_delay_ack_flag[];
extern BOOLEAN signaling_announce_ack_flag[];
extern UINT16  number_of_delay_packets;
extern UINT16  number_of_sync_packets_for_timeout[];
extern UINT16  number_of_delay_packets_for_timeout;
extern UINT16 	PTP_Seq_Num_curr;
extern FIFO_STRUCT PTP_Delay_Fifo,PTP_Delay_Fifo_Local;
extern UINT8 	PTP_Seq_Num_prev;
extern UINT16 		Num_of_missed_pkts;
extern UINT32 	PtpPacketCount;
extern UINT16  number_of_sync_packets[];
extern FIFO_STRUCT PTP_Fifo[],PTP_Fifo_Local;
extern FLOW_STRUCT_TYPE master_stuct[];

extern void Minute_Task (UINT32  parameter);
extern void PTP_Port320_listen_Task (UINT32  parameter);
extern void Delay_Request_Transmit_Task (UINT32  parameter);
extern void PTP_Flow_Control_Task (UINT32  parameter);
extern void PTP_Flow_Control_Task2 (UINT32  parameter);
extern void PTP_Port319_listen_Task (UINT32  parameter);
extern void CLK_Task (UINT16 w_hdl); // XUAN
extern void PTP_Timeout_Task (UINT32  parameter);
extern void Set_Master_IP(UINT32 master_addr);
extern UINT32 Get_Master_IP(void);
extern UINT32 Get_Client_IP(void);
// extern _rtcs_if_handle ihandle0;
extern INT16 tx_uni_req (UINT32 where, INT16 type, UINT32 duration, INT16 rate,  unsigned char *clock_id);
extern void Get_Flow_Status(UINT16 *sync, UINT16 *delay, UINT8 master);
extern UINT8 Is_SYNC_Flowing(UINT8 master);
extern UINT8 Is_DELAY_RESP_Flowing(void);
//extern int get_rate(int_8 data);
extern PTP_Flow_State Get_Flow_State(UINT8 master);
extern UINT32 	Ptp_sock_delay;
extern UINT32 	Ptp_sock;
extern int tx_uni_dreq (unsigned long where, unsigned char *buf, unsigned char *clock_id);
extern void get_fll_status(FLL_STATUS_STRUCT *fp);
extern Fll_Clock_State get_fll_state(void);
extern Fll_Clock_State get_fll_prev_state(void);
extern UINT16 get_fll_led_state(void);
extern UINT16 Is_Sync_timeout(UINT8 master);
extern UINT16 read_from_MMFR_no_paging(UINT16 reg);

#define GRANDMASTER_PRIMARY    0
#define GRANDMASTER_ACCEPTABLE 1
#define GRANDMASTER_MANAGEMENT 2
extern void Restart_Flow(UINT8 master);
extern void Set_Sync_Timer (UINT8 master, UINT32 ticks);
extern void Set_Delay_Timer (UINT8 master, UINT32 ticks);
extern void Set_Announce_Timer (UINT8 master, UINT32 ticks);

extern double Get_fdrift(void);
extern void Set_fdrift(double value);
extern BOOLEAN Is_FLL_Elapse_Sec_Count_gt_20(void);
extern BOOLEAN Is_FLL_Elapse_Sec_Count_gt_400(void);
extern void Set_Seq_Num(UINT16 seq_num);

extern void set_mode_compression_flag(int flag);
extern GM_select Get_Current_GM(UINT32 *ip);
extern PTP_Ref_State Get_Ref_State(UINT32 *count);
extern UINT8 Signaling_Ack_Packet_Good(struct ptp_sig_grant *pkt, UINT16 *error);
extern UINT8 Delay_Resp_Packet_Good(struct ptp_sync_delay *pkt, UINT16 *error);
extern UINT8 Sync_Packet_Good(struct ptp_sync_delay *pkt, UINT16 *error);
extern UINT8 Announce_Packet_Good(struct ptp_announce *pkt, UINT16 *error);
extern UINT8 Sig_Acks_are_good(UINT8 master);
extern BOOLEAN Is_GM_Lock_Holdover(UINT8 master);
extern BOOLEAN Is_Announce_Timeout(UINT8 master);

extern INT32 getfreq(void);
extern int setfreq_switchover(INT32 freq);
extern int setfreq(INT32 freq);

typedef struct 
{
	INT32 current;  //note update current before calling calculator
    INT32 lag;
    INT32 dlag;
    double  filt;
    double var_est;
}
 TDEV_ELEMENT_STRUCT;

typedef struct 
{
	INT8 update_phase;
	TDEV_ELEMENT_STRUCT tdev_element[TDEV_WINDOWS + 1];
//	TDEV_ELEMENT_STRUCT tdev_1024[3];//variance for size 2,3 and 4 cluster sizes
	
} TDEV_STRUCT;
typedef struct 
{
	double in_f,in_r;
	double cur_avg_f;  //note update current before calling calculator
    double cur_avg_r;
	double prev_avg_f;  //note update current before calling calculator
    double prev_avg_r;
    double ztie_est_f;
	double ztie_est_r;
 }
 ZTIE_ELEMENT_STRUCT;

typedef struct 
{
	INT8 head;
	double ztie_f, ztie_r;
	double mafie_f, mafie_r;
	ZTIE_ELEMENT_STRUCT ztie_element[15];
} ZTIE_STRUCT;
typedef struct 
{
	INT32	sdrift_mins_1024[512],sdrift_mins_512[512],sdrift_mins_256[512],sdrift_mins_128[512];
	UINT16 min_count_1024,min_count_512,min_count_256,min_count_128;
	UINT16 min_count_1024_2,min_count_1024_3,min_count_1024_4;
	INT32	sdrift_min_cal,sdrift_min_1024,sdrift_min_512,sdrift_min_256,sdrift_min_128;
	INT32	sdrift_min_1024_2,	sdrift_min_1024_3,sdrift_min_1024_4;
	long long sdrift_min_1024_sum,sdrift_min_512_sum,sdrift_min_256_sum,sdrift_min_128_sum;
	long long sdrift_min_1024_2_sum,sdrift_min_1024_3_sum,sdrift_min_1024_4_sum;
	INT32	 sdrift_floor_1024,sdrift_floor_512,sdrift_floor_256,sdrift_floor_128;
}
 MIN_STRUCT;
#if 0
typedef struct 
{
	double x;  
	double xsq;
	double ysq;
	double xy;
	double y;
	double b;
	double m;
//	double y_ma; //added moving average pre-processing Sept 2009
}
 LSF_ELEMENT_STRUCT;

typedef struct 
{
	INT16 N,Nmax;
	double bf;
	double mf;
	double fitf;
	double varf;
	double sumxf;
	double sumxsqf;
	double sumysqf;
	double sumxyf;
	double sumyf;
	double br;
	double mr;
	double fitr;
	double varr;
	double sumxr;
	double sumxsqr;
	double sumysqr;
	double sumxyr;
	double sumyr;
	double slew_limit_f,slew_limit_r;
	INT16 slew_cnt_f, slew_cnt_r; 
	INT16 index;
	LSF_ELEMENT_STRUCT lsfe_f[LSF_WINDOW_MAX];
	LSF_ELEMENT_STRUCT lsfe_r[LSF_WINDOW_MAX];
} LSF_STRUCT;
#endif

#define VTYPE
typedef struct {
VTYPE UINT16    nv_ippm_pacing;  /* units of transactions skipped 1,2...256*/
VTYPE UINT32    nv_ippm_thres;   /* units of nsecs 0 to 1 second*/
VTYPE UINT16    nv_ippm_window;  /* units of minutes 1 - 255*/
VTYPE UINT8     nv_pps_mode;     /* */
VTYPE UINT8     nv_ptp_transport;/* */
} NVM;

// April 2010 New Robust Frequency Estimation Data Elements
// This is part of a brand new way to regress frequency estimates
// It is based like for like comparison across parallel windows
// TODO this is the way to make DSL Robust 
typedef struct 
{
	double phase;  // uncompenstated phase error estimate ns use sacc_f
	double bias;   // bias estimate ns use min_oper_thres as a full range source of this
	double noise;  // noise estimate ns^2
	INT16 tfs;    // transient free seconds state   use delta_[forward]_slew_cnt
	INT16 floor_cat; // indicated common floor index
}
 RFE_ELEMENT_STRUCT;
typedef struct 
{
	INT16 N,Nmax;
	INT16 index;
	INT8 first;
	double alpha; //oscillator dependent smoothing gain across update default 1/60
	double dfactor; // additional de weighting factor based on total weight 0.1 to 1
	double wavg;  //running average weight using alpha factor
	double weight_for, weight_rev;  //Ken_Post_Merge
	double fest; // frequency estimate in ppb
	double fest_prev; //spaced N back use to check initial stabilization
	double fest_for,fest_rev; // frequency estimate in ppb Ken_Post_Merge
	double fest_cur;
  double delta_freq;
  double smooth_delta_freq;
  double res_freq_err;
	RFE_ELEMENT_STRUCT rfee_f[RFE_WINDOW_MAX];
	RFE_ELEMENT_STRUCT rfee_r[RFE_WINDOW_MAX];
} RFE_STRUCT;

typedef struct
{
	XFER_State xstate;
	INT8 working_sec_cnt;
	INT8 duration;
	INT8 idle_cnt;
	// Protected RFE In Data
	double xfer_sacc_f, xfer_sacc_r;
	double xfer_var_est_f, xfer_var_est_r;
	double xfer_in_weight_f, xfer_in_weight_r;
	// Protected RFE Out Data
	double xfer_fest, xfer_fest_f, xfer_fest_r;
	double xfer_weight_f, xfer_weight_r;
	// protected FLL status Data
	double xfer_fdrift, xfer_fshape, xfer_tshape;
	double xfer_pshape_smooth;
	double xfer_scw_avg_f, xfer_scw_avg_r;
	double xfer_scw_e2_f,xfer_scw_e2_r;
  double xfer_turbo_phase[2];
}
XFER_DATA_STRUCT;

extern void set_ippm_pacing(INT16);
extern void set_ippm_thres(INT32);
extern void set_ippm_window(INT16);
extern void reset_ippm(void);
extern double get_ippm_f_jitter(void);
extern double get_ippm_r_jitter(void);
extern double get_ippm_f_ipdv(void);
extern double get_ippm_r_ipdv(void);
extern double get_ippm_f_ipdv_99_9(void);
extern double get_ippm_r_ipdv_99_9(void);
extern double get_ippm_f_thres_prob(void);
extern double get_ippm_r_thres_prob(void);
extern void Print_FIFO_Buffer(UINT16 index, UINT16 fast);
extern void Start_Phase_Test(void);
extern void Start_Freq_Test(void);
extern int Start_Vcal(void);
extern UINT32 Get_Tick(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
