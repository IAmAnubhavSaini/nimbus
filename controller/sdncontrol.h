#ifndef _SDNCONTROL_H
#define _SDNCONTROL_H
#include <set>

#define CONTROL_PORT 5003
#define SIGNAL_PORT 5004
#define TG_PORT 5005
#define COMMLEN 2

#define COMMAND_WAIT    0
#define COMMAND_SEND    1
#define COMMAND_FINISH  2


#define ROUTING_MAXBUFLEN 10240


#define MAXBUFLEN 9000
#define TOP_K_TABLE_SIZE			256
#define BOTTOM_K_TABLE_SIZE			7

#define NOTICE_TABLE_SIZE			5120

//#define REROUTE_TABLE_SIZE			256

#define SERVER_ADDR "204.57.0.8"


#define CPU_HIGH_WATERMARK	50
#define CPU_LOW_WATERMARK	30


#define SAMPLE_RATE 100
#define HIGH_EXTRA SAMPLE_RATE
#define LOW_EXTRA (SAMPLE_RATE/10)    // 1%  *10 ,  10% * 10
#define EXTRAPOLATE_THRESH 2



//#define FLOW_BASE_SAMPLE

#ifdef FLOW_BASE_SAMPLE
#define SAMPLE_IN_HIGH_WATERMARK 5824    //1% sample rate
#define SAMPLE_IN_LOW_WATERMARK 4000
#else
#define SAMPLE_IN_HIGH_WATERMARK 1000 * (100 / SAMPLE_RATE)    //1500  1%    1000 
#define SAMPLE_IN_LOW_WATERMARK 800 * (100 / SAMPLE_RATE)      //1000  1%    800
#endif


#define MAX_NUM_ROUTE 65536

#define NOTICE_TIMEOUT 5000  //millisecond


//#define PKT_BASE_SAMPLE 1


DWORD WINAPI StartServer(LPVOID paramPtr);
DWORD WINAPI StartClient(LPVOID paramPtr);
//DWORD WINAPI StartAgent(LPVOID paramPtr);

typedef struct _TOP_CONN_INFO
{
	UINT				mode;		//0: normal. 1: need to move
	UINT				hash;
	UINT                daddr;
	UCHAR               protocol;
	UINT				numofsource;
	UINT				pkt;

}TOP_CONN_INFO;


typedef struct _TOP_K_TABLE
{
	CRITICAL_SECTION      Lock;
	UINT		lasttime;
	UINT        reported;
	UINT		K;
	UINT		idx;
	TOP_CONN_INFO	TopKTable[TOP_K_TABLE_SIZE];
}TOP_K_TABLE;

typedef struct _IDS_AGENT_INFO
{
	char *name;
	char *path;
	UINT MACidx;
} IDS_AGENT_INFO;


typedef struct _TG_INFO
{
	char *name;
	USHORT port;
} TG_INFO;

#define ROUTE_CANNOT_REDI 0
#define ROUTE_CAN_REDI 1
#define ROUTE_REDIRECTED 2

typedef struct _ROUTE_NOTICE
{
	//UINT		K;
	
	mutable UINT reported;    //0: no redirection for now, 1: have redirection, 2: redirected
	UINT		noticetime;
	UINT		RouteEntry;
	UINT		NewDst;
	char		*NewDstip;

	//ROUTE_NOTICE(){};

	bool operator<(const _ROUTE_NOTICE &entry) const
	{
		return RouteEntry < entry.RouteEntry;
	}
	bool operator==(const _ROUTE_NOTICE &entry) const
	{
		return RouteEntry == entry.RouteEntry;
	}


} ROUTE_NOTICE;





typedef struct _agent_status
{
	//IDS info
	UINT MACidx;  
	//SOCKET sock;

	//stage
	BOOL miti_stage;   //mitigation
	UINT last_miti_time;
	UINT last_active_time;
	//UINT last_scale_back_time;

	//status
	UINT total_cpu_usage;
	UINT user_cpu_usage;
	UINT kernel_cpu_usage;
	UINT smooth_cpu_usage;

	UINT mem_usage;
	UINT PPS;
	UINT MBS;
	UINT samplepps;
	UINT samplembs;

	UINT conn;
	UINT flow;

	UINT totalsamples;
	UINT budget;
	UINT overhead;
	INT overload;
	//UINT routes;
	//UINT own;

	TOP_K_TABLE topktable;

	

	char status[MAXBUFLEN];


} AGENT_STATUS;

typedef struct _AGNET_STATUS_TOTAL
{
	UINT smooth_cpu_usage;

	UINT mem_usage;
	UINT PPS;
	UINT MBS;

	UINT conn;
	UINT flow;
	UINT totalsample;
	UINT totaltopk;
}AGNET_STATUS_TOTAL;

typedef struct _agent_control
{
	AGENT_STATUS agent_status;
	std::set<ROUTE_NOTICE> noticetable;
} AGENT_CONTROL;




typedef unsigned int UINT;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;
typedef int INT;
typedef unsigned long ULONG;
typedef unsigned char	uint8;
typedef unsigned __int64 ULONGLONG;
typedef UCHAR BOOLEAN;

#if BE_HOST==1
#define NIPQUAD_HOSTORDER(addr) \
	((uint8 *)&addr)[0], \
	((uint8 *)&addr)[1], \
	((uint8 *)&addr)[2], \
	((uint8 *)&addr)[3]
#else
#define NIPQUAD_HOSTORDER(addr) \
	((uint8 *)&addr)[3], \
	((uint8 *)&addr)[2], \
	((uint8 *)&addr)[1], \
	((uint8 *)&addr)[0]
#endif







#endif