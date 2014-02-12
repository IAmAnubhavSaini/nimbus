// ftcmd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
//#include <windows.h>
#include "Psapi.h"

#define APPLICATION_INSTANCE_MUTEX_NAME "{BA49C45E-B29A-4359-A07C-51B65B5571AD}"

//Make sure at most one instance of the tool is running
HANDLE hMutexOneInstance(::CreateMutex(NULL, TRUE, (LPCWSTR)APPLICATION_INSTANCE_MUTEX_NAME));





#define FT_EPOCH 1000  //millisecond
#define FT_CLEAN_EPOCH 30000  //millisecond
#define INACTIVE_TIMEOUT 30000




typedef struct _CONN
{
	//LIST_ENTRY      ListEntry;

	//BOOLEAN         InUse;
	UINT            hash;
	FLOW_LABEL      key;
	//UINT            status;             /* FLOW_BEGIN, LONG_FLOW_MARK, REPORTED */
	//UINT            lastpkts;
	//ULONGLONG       lastbytes;
	UINT            packets;
	//ULONGLONG       bytes;
	ULONG			bytes;
	UINT			numofsource;
	std::map<UINT,UINT> uniquesource;

	//ULONGLONG       lastactivetime;     /* KeQueryInterruptTime(), 100ns granularity */
	ULONGLONG       lasttime;           /* KeQueryInterruptTime(), 100ns granularity */
	//ULONG           incpkts;
	//ULONG           incbytes;
	//ULONG           inctime;            /* ms granularity */
	//UCHAR           outExtIf;
	//UINT			pktsamplerate;
	//UINT			flowsamplerate;
} CONN, *PCONN;

typedef struct _CONN_HASH_ENTRY
{
	CRITICAL_SECTION      Lock;
	//LIST_ENTRY          ListHead;
	std::list<CONN>		  flowlist;
} CONN_HASH_ENTRY, *PCONN_HASH_ENTRY;

typedef struct _FLOW
{
	//LIST_ENTRY      ListEntry;

	//BOOLEAN         InUse;
	UINT            hash;
	FLOW_LABEL      key;
	//UINT            status;             /* FLOW_BEGIN, LONG_FLOW_MARK, REPORTED */
	//UINT            lastpkts;
	//ULONGLONG       lastbytes;
	UINT            packets;
	//ULONGLONG       bytes;
	ULONG			bytes;
	//std::map<UINT, UINT> uniquesource;

	//ULONGLONG       lastactivetime;     /* KeQueryInterruptTime(), 100ns granularity */
	ULONGLONG       lasttime;           /* KeQueryInterruptTime(), 100ns granularity */
	//ULONG           incpkts;
	//ULONG           incbytes;
	//ULONG           inctime;            /* ms granularity */
	//UCHAR           outExtIf;
	//UINT			pktsamplerate;
	//UINT			flowsamplerate;
} FLOW, *PFLOW;

typedef struct _FLOW_HASH_ENTRY
{
	CRITICAL_SECTION      Lock;
	//LIST_ENTRY          ListHead;
	std::list<FLOW>		  flowlist;
} FLOW_HASH_ENTRY, *PFLOW_HASH_ENTRY;



typedef struct _FLOWTABLE {
	UINT					initialized;
	UINT                    count_packet_receive;
	//conn
	//UINT					count_allocate_connfail;
	UINT					count_reach_max_active_conn;
	UINT					count_active_conn;
	//UINT					count_total_conn;
	UINT					count_max_active_conn;
	UINT					max_active_conn;

	//flow
	UINT					count_reach_max_active_flow;
	UINT					count_active_flow;
	//UINT					count_total_conn;
	UINT					count_max_active_flow;
	UINT					max_active_flow;

	//UINT                    count_flow_notify;
	//UINT                    count_hwflow_max;
	//UINT                    count_hwflow_dup;
	//UINT                    count_hwflow_new;
	//UINT                    count_hwflow_swap;
	//UINT                    count_hwflow_noswap_nosorted;
	//UINT                    count_hwflow_noswap_small;

	//UINT                    count_hwflow_install_ok;
	//UINT                    count_hwflow_remove_ok;
	//UINT                    count_hwflow_install_fail;
	//UINT                    count_hwflow_remove_fail;
} FLOWTABLE;






CONN_HASH_ENTRY         ConnHashTable[CONN_TABLE_SIZE];
FLOW_HASH_ENTRY			FlowHashTable[FLOW_TABLE_SIZE];
TOP_K_TABLE				TopKTable;



FLOWTABLE				flowtable;

#define WIN_FT_BASE_DEVICE_NAME       L"NDISLWF"
#define WIN_FT_DEVICE_NAME            L"\\\\.\\" WIN_FT_BASE_DEVICE_NAME
#define MAX_QUERY_FLOW_NUM            32

#define MAX_SET_SAMPLE_RATE_NUM            1024

#define FT_THREAD_F_RUN                    0x01
#define FT_THREAD_F_STOP                   0x02


AGENT_STATE ft_agent_state;
SystemUsage systemusage;



#define PKT_BASE_SAMPLE

#ifdef PKT_BASE_SAMPLE
UINT g_pkt_base_sample = 1;
UINT g_flow_base_sample = 0;
#else
UINT g_pkt_base_sample = 0;
UINT g_flow_base_sample = 1;
#endif

//#define FT_SOCKET_F_RUN                    0x01
//#define FT_SOCKET_F_STOP                   0x02


/*
typedef struct _OVERLAPPED
{
ULONG_PTR Internal;
ULONG_PTR InternalHigh;
union
{
struct
{
DWORD Offset;
DWORD OffsetHigh;
};
PVOID Pointer;
};
HANDLE hEvent;
} 	OVERLAPPED, *LPOVERLAPPED;
*/


typedef struct _PKT_EVENT {
	USER_PACKET_SET        *pktset;
	UINT                    BytesReturned;
	OVERLAPPED              stOverlapped;
} PKT_EVENT;


//#define FT_THREAD_ERROR					((sal_thread_t) -1)


typedef DWORD nthread_t;


static HANDLE			ft_thread_handle;
static nthread_t		ft_thread_pid;
static UINT				ft_thread_flags = 0;   /* Flags this unit PU_F_XXX */

static HANDLE			ft_timer_thread_handle;
static nthread_t		ft_timer_thread_pid;
static UINT				ft_timer_thread_flags = 0;   /* Flags this unit PU_F_XXX */

static HANDLE			ft_agent_thread_handle;
static nthread_t		ft_agent_thread_pid;
//static SOCKET           ft_agent_socket;

//static UINT				ft_agent_socket_flags = 0;   /* Flags this unit PU_F_XXX */



#define MAX_QUERY_PKT_NUM                   32
#define QUERY_PKT_SIZE                      2000
static int                  ft_periodic_check = 100000;

//static sal_sem_t            bft_thread_sync = NULL; /* Sync command semaphore */



FLOW_EVENT      FlowEvent;
PKT_EVENT       PktEvent;

HANDLE	        FTHandle = NULL;








INT SystemUsage::GetCPUUsage(INT *user, INT *kernel, INT *total)
{
	BOOL res;

	res = GetSystemTimes(&idleTime, &kernelTime, &userTime);

	if (res == 0)
		return CMD_FAIL;

	double usr = (long)(userTime.dwHighDateTime - last_userTime.dwHighDateTime) * unit + (long)(userTime.dwLowDateTime - last_userTime.dwLowDateTime);
	double ker = (long)(kernelTime.dwHighDateTime - last_kernelTime.dwHighDateTime) * unit + (long)(kernelTime.dwLowDateTime - last_kernelTime.dwLowDateTime);
	double idl = (long)(idleTime.dwHighDateTime - last_idleTime.dwHighDateTime) * unit + (long)(idleTime.dwLowDateTime - last_idleTime.dwLowDateTime);
	//printf("%lu %lu, %lu %lu, %lu %u\n",userTime.dwHighDateTime, userTime.dwLowDateTime, 
	//	kernelTime.dwHighDateTime, kernelTime.dwLowDateTime,
	//	idleTime.dwHighDateTime, idleTime.dwLowDateTime);

	last_userTime.dwHighDateTime = userTime.dwHighDateTime;
	last_userTime.dwLowDateTime = userTime.dwLowDateTime;
	last_kernelTime.dwHighDateTime = kernelTime.dwHighDateTime;
	last_kernelTime.dwLowDateTime = kernelTime.dwLowDateTime;
	last_idleTime.dwHighDateTime = idleTime.dwHighDateTime;
	last_idleTime.dwLowDateTime = idleTime.dwLowDateTime;

	double sys = usr + ker;

	if (sys == 0)
		return CMD_FAIL;

	*user = INT(usr * 100 / sys);
	*kernel = INT((ker - idl) * 100 / sys);
	*total = INT((sys - idl) * 100 / sys);

	if (*total > 100 || *total < 0)
		return CMD_FAIL;

	return CMD_OK;
}

INT SystemUsage::GetMemUsage(UINT *mem)
{
	int rc;
	PROCESS_MEMORY_COUNTERS pmc;

	rc = GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
	if (rc == 0)
		return CMD_FAIL;

	memusage = (pmc.WorkingSetSize) / 1024;

	*mem = (UINT)memusage;
	
	return CMD_OK;
}


int
FTOpenDevice(LPCWSTR devicename)
{
	FTHandle = CreateFileW(devicename,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,   // security attributes
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,      // flags & attributes
		NULL);
	if (FTHandle == INVALID_HANDLE_VALUE)
	{
		printf("[FTOpenDevice] Could not access FT: %d\n", GetLastError());
		return FT_FAIL;
	}
	return FT_SUCCESS;
}


int
ft_strcasecmp(const char *s1, const char *s2)
{
	unsigned char c1, c2;

	do {
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
	} while (c1 == c2 && c1 != 0);

	return c1 - c2;
}

int
ft_query_protocol(
FT_INFO_PROTOCOL  *PT)
{
	ULONG                BytesReturned;

	if (!DeviceIoControl(FTHandle,
		IOCTL_FT_QUERY_PROTOCOL,
		NULL, 0,
		PT, sizeof *PT, &BytesReturned,
		NULL))
	{
		printf("[ft_query_protocol] error 0x%x\n", GetLastError());
		return CMD_FAIL;
	}

	if (BytesReturned != sizeof *PT)
	{
		printf("[ft_query_protocol] Protocol info length returned %d != %d\n", BytesReturned, sizeof *PT);
		return CMD_FAIL;
	}

	return CMD_OK;
}

int
ft_query_performance(
FT_INFO_PERFORMANCE  *PT)
{
	ULONG                BytesReturned;

	if (!DeviceIoControl(FTHandle,
		IOCTL_FT_QUERY_PERFORMANCE,
		NULL, 0,
		PT, sizeof *PT, &BytesReturned,
		NULL))
	{
		printf("[ft_query_performance] error 0x%x\n", GetLastError());
		return CMD_FAIL;
	}

	if (BytesReturned != sizeof *PT)
	{
		printf("[ft_query_performance] performance info length returned %d != %d\n", BytesReturned, sizeof *PT);
		return CMD_FAIL;
	}

	return CMD_OK;
}


cmd_result_t
ft_query()
{
	FT_INFO_PROTOCOL   ftinfo;
	UINT i;
	std::list<CONN>::iterator it;
	
	if (ft_query_protocol(&ftinfo) == CMD_FAIL)
	{
		return CMD_FAIL;
	}
	//printf("FT address:");
	//for (i = 0; i < 32; i++)
	//{

	//	printf("%x ",
	//		ftinfo.DeviceAddress[i]);
	//}
	//printf("\n");

	ft_agent_state.totalsamples = 0;

	for (i = 0; i < CONN_TABLE_SIZE; ++i)
	{
		EnterCriticalSection(&ConnHashTable[i].Lock);

		for (it = ConnHashTable[i].flowlist.begin(); it != ConnHashTable[i].flowlist.end(); ++it)
		{
			ft_agent_state.totalsamples += it->numofsource;
		}
		LeaveCriticalSection(&ConnHashTable[i].Lock);
	}

	printf(" init %u, psamp: %u, fsamp: %u, prate: %u, frate: %u, maxconn %u, token %u\n",
		ftinfo.initialized,
		ftinfo.enable_pkt_samp,
		ftinfo.enable_flow_samp,
		ftinfo.pkt_default_samp,
		ftinfo.flow_default_samp,
		//bftinfo.mode,
		//bftinfo.MinNotifyPacket,
		//bftinfo.FlowTimeout,
		ftinfo.MaxActiveConn,
		ftinfo.MaxTokenSize);

	printf("FT stat (kernel)\n");
	//active %u,
	printf("  Conn: total %u, max active %u, allocate fail %u, reach max %u\n",
		ftinfo.CountTotalConn,
		//ftinfo.CountActiveConn,
		ftinfo.CountMaxActiveConn,
		ftinfo.CountAllocateConnFail,
		ftinfo.CountReachMaxActiveConn);


	printf("  Discard: BMcast %u, Small %u, full %u\n",
		ftinfo.CountDiscardBMcastPacket,
		ftinfo.CountDiscardSmallPacket,
		ftinfo.CountRecvDiscardUserPacket);

	printf("user pkt %u, samples %u\n", ftinfo.CountDeliverUserPacket, ftinfo.CountSamplePacket);
	printf("cpu %u, mem %u, pps %u, mbs %u, conn %u, flow %u, top %u, samp %u, in %u\n",
		ft_agent_state.totalusage,
		ft_agent_state.memusage,
		ft_agent_state.fwdpps,
		ft_agent_state.fwdmbs,
		ft_agent_state.countconn,
		ft_agent_state.countflow,
		TopKTable.K,
		ft_agent_state.totalsamples,
		ft_agent_state.inboundpps);

	//printf("Set Sample Rate: Fail %u\n", ftinfo.CountSetSampleRateFail);

	return CMD_OK;
}

static void
init_ft_control_protocol(
FT_CONTROL_PROTOCOL   *Control)
{
	memset(Control, (UINT)-1, sizeof(*Control));

	Control->initialized = (UINT)-1;
	//Control->mode = (UINT)-1;
	Control->MaxActiveConn = (UINT)-1;
	//Control->FlowTimeout = (UINT)-1;
	//Control->MinNotifyPacket = (UINT)-1;
	//Control->FlowCheckPeriod = (UINT)-1;
	Control->enable_flow_samp = (UINT)-1;
	Control->enable_pkt_samp = (UINT)-1;
	Control->ResetCounter = (UINT)-1;
	Control->MaxTokenSize = (UINT)-1;
}


int
ft_control_protocol(
	FT_CONTROL_PROTOCOL        Control
	)
{
	ULONG  BytesReturned;

	if (!DeviceIoControl(FTHandle,
		IOCTL_FT_CONTROL_PROTOCOL,
		  &Control, sizeof Control,
		  NULL, 0, &BytesReturned, NULL)) {
		printf("control error: %x\n", GetLastError());
		return CMD_FAIL;
	}

	return CMD_OK;
}


int
read_kernel_flowset(FLOW_EVENT *pflowEvent)
{
	BOOL                status;
	INT                 err;
	FT_QUERY_FLOWSET   query;
	ULONG BytesReturned;

	query.qtype = 1;
	query.nflow = 32;

	status = DeviceIoControl(FTHandle,
		IOCTL_FT_QUERY_FLOWSET,
		&query, sizeof(FT_QUERY_FLOWSET),
		(PVOID)pflowEvent->flowset,
		sizeof(FT_INFO_FLOWSET)+MAX_QUERY_FLOW_NUM * sizeof(FT_INFO_FLOW),
		&BytesReturned,
		NULL);
		//&pflowEvent->stOverlapped);
	if (!status && (ERROR_IO_PENDING != (err = GetLastError())))
	{
		fprintf(stderr, "ft receive packet err 0x%x\n", err);
		return CMD_FAIL;
	}

	return CMD_OK;
}



VOID
FTGettop(std::vector<CONN> &Connlist)
{
	
	INT                     hash;
	//CONN_HASH_ENTRY        *pConnHash;
	//PLIST_ENTRY             pEntry;
	//PLIST_ENTRY             pDelEntry;
	//CONN                   *pConn;
	int                     i;
	//CONN                   *ConnTable[FLOW_NOTIFY_NUM];
	int                     nConn = 0;
	std::list<CONN>::iterator	it;


	//for (i = 0; i < FLOW_NOTIFY_NUM; ++i)
	//{
	//	ConnTable[i] = NULL;
	//}

	Connlist.clear();

	for (hash = 0; hash < CONN_TABLE_SIZE; ++hash)
	{
		EnterCriticalSection(&ConnHashTable[hash].Lock);
		//pEntry = pConnHash->ListHead.Flink;
		for (it = ConnHashTable[hash].flowlist.begin(); it != ConnHashTable[hash].flowlist.end(); ++it)
		{
			if (nConn < FLOW_NOTIFY_NUM)
				Connlist.push_back(*it);
			/* find the largest 256 flows, insert flows from the tail */
			for (i = nConn - 1; i >= 0; --i)
			{
				if (it->packets > Connlist[i].packets)
				{
					if (i + 1 < FLOW_NOTIFY_NUM)
					{
						/* move entry one-step to the tail */
						Connlist[i + 1] = Connlist[i];
					}
					else
					{
						/* remove the last entry */
						nConn--;
					}
				}
				else
				{
					break;
				}
			}
			if (i + 1 < FLOW_NOTIFY_NUM)
			{

				Connlist[i + 1] = *it;
				nConn++;
			}
		}
		LeaveCriticalSection(&ConnHashTable[hash].Lock);

		//NdisDprReleaseSpinLock(&ConnHashTable[hash].Lock);
	}
}

INT
ClearUserTable()
{
	INT                     hash;
	INT						flownum;
	//CONN_HASH_ENTRY        *pConnHash;
	std::list<CONN>::iterator it;

	for (hash = 0; hash < CONN_TABLE_SIZE; ++hash)
	{
		//pConnHash = &ConnHashTable[hash];
		EnterCriticalSection(&ConnHashTable[hash].Lock);
		flownum = (INT)(0 - ConnHashTable[hash].flowlist.size());
		InterlockedAdd((PLONG)&flowtable.count_active_conn, flownum);
		ConnHashTable[hash].flowlist.clear();
		LeaveCriticalSection(&ConnHashTable[hash].Lock);
	}

	return CMD_OK;
}




#if 0
int
ft_getflowset()
{
	FT_INFO_FLOWSET       *pflowset;
	FT_INFO_FLOW          *pflow;
	std::vector<CONN>	Connlist;
	std::vector<CONN>::iterator it;

	UINT             j;


	FlowEvent.flowset = (FT_INFO_FLOWSET *)malloc(sizeof(FT_INFO_FLOWSET)+MAX_QUERY_FLOW_NUM*sizeof(FT_INFO_FLOW));
	//memset(&FlowEvent.stOverlapped, 0, sizeof(OVERLAPPED));

	if (read_kernel_flowset(&FlowEvent) != CMD_OK)
	{
		fprintf(stderr, "read_kernel_flowset err 0x%x\n", GetLastError());
		return CMD_FAIL;
	}

	//gettimeofday(&tv);
	
	pflowset = FlowEvent.flowset;

	printf("kernel:  total %u flows\n", pflowset->nflow);


	for (j = 0; j < pflowset->nflow; ++j)
	{
		//ft.count_flow_notify++;

		pflow = &pflowset->flow[j];

		printf("flow type %d, %u.%u.%u.%u(%u), pktsamp: %u, flowsamp: %u, total %u,%u\n",
			pflow->type,
			NIPQUAD_HOSTORDER(pflow->dstip), pflow->protocol,
			//pflow->portnum,
			pflow->pktsamplerate, pflow->flowsamplerate,
			pflow->totalbytes, pflow->totalpkts);
	}


	FTGettop(Connlist);

	printf("user:  total %u flows\n", Connlist.size());

	for (it = Connlist.begin(); it != Connlist.end(); ++it)
	{
		//ft.count_flow_notify++;

		//pflow = &pflowset->flow[j];

		printf("flow   %u.%u.%u.%u(%u), pktsamp: %u, flowsamp: %u, total %u,%u, flow %u\n",
			//it->status,
			NIPQUAD_HOSTORDER(it->key.daddr), it->key.protocol,
			//pflow->portnum,
			it->pktsamplerate, it->flowsamplerate,
			it->bytes, it->packets,
			it->uniquesource.size());
	}
	return CMD_OK;
}
#endif

int
ft_init()
{
	
	FT_CONTROL_PROTOCOL            Control;
	SOCKET		sock;

	init_ft_control_protocol(&Control);
	Control.initialized = 1;
	Control.ResetCounter = 1;
	if (g_flow_base_sample)
	{
		Control.enable_flow_samp = 1;  //flow sampling by default
		Control.enable_pkt_samp = 0;
	}
	else if (g_pkt_base_sample)
	{
		Control.enable_pkt_samp = 1;
		Control.enable_flow_samp = 0;
	}


	if (ft_control_protocol(Control) != CMD_OK)
	{
		printf("[ft_init] control_protocol error\n");
		return CMD_FAIL;
	}
	

	ClearUserTable();

	
	sock = ft_agent_state.agent_sock;

	memset(&ft_agent_state, 0, sizeof(ft_agent_state));

	ft_agent_state.agent_sock = sock;


	
	return CMD_OK;
}


int
ft_deinit()
{

	FT_CONTROL_PROTOCOL   Control;

	init_ft_control_protocol(&Control);
	Control.initialized = 0;
	Control.enable_flow_samp = 0;
	Control.enable_pkt_samp = 0;
	Control.ResetCounter = 1;
	if (ft_control_protocol(Control) != CMD_OK)
	{
		printf("[ft_init] control_protocol error\n");
		return CMD_FAIL;
	}


	ClearUserTable();
	return CMD_OK;
}

int 
ft_control(char * cmd)
{
	FT_CONTROL_PROTOCOL        Control;
	init_ft_control_protocol(&Control);

	//if (ft_control_protocol(Control) != CMD_OK)
	//{
	//	return CMD_FAIL;
	//}

	char *next_token = NULL;
	char *token = " ";
	char *currentcmd = "";
	currentcmd = strtok_s(cmd, token, &next_token);
	if (!ft_strcasecmp(currentcmd, "fsamp"))
	{
		UINT stat = atoi(next_token);
		Control.enable_flow_samp = stat;

	}
	else if(!ft_strcasecmp(currentcmd, "psamp"))
	{
		UINT stat = atoi(next_token);
		Control.enable_pkt_samp = stat;
	}
	else if (!ft_strcasecmp(currentcmd, "token"))
	{
		
		UINT stat = atoi(next_token);
		//printf("set token to %d\n", stat);
		Control.MaxTokenSize = stat;
	}
	else
	{
		printf("set: unknown command\n");
	}


	if (ft_control_protocol(Control) != CMD_OK)
	{
		return CMD_FAIL;
	}

	return CMD_OK;
}




int
ft_control_sample_rate(
FT_CONTROL_SAMPLE_RATE        SampleRate
)
{
	ULONG  BytesReturned;

	if (!DeviceIoControl(FTHandle,
		IOCTL_FT_CONTROL_SAMPLE_RATE,
		&SampleRate, sizeof SampleRate,
		NULL, 0, &BytesReturned, NULL)) {
		printf("control error: 0x%x return %d, input %d\n", GetLastError(), BytesReturned, sizeof SampleRate);
		return CMD_FAIL;
	}
	return CMD_OK;
}


int
ft_set_sample_rate(char* cmd)
{
	std::list<CONN>::iterator it;
	FT_CONTROL_SAMPLE_RATE SampleRate;
	UINT  i;
	//UINT pktsamplerate;
	//UINT hash;

	//std::vector<CONN> Connlist;
	UINT bChangeDefault = 0;
	char *next_token = NULL;
	char *token = " ";
	char *currentcmd = "";
	currentcmd = strtok_s(cmd, token, &next_token);
	UINT rate_val = atoi(next_token);



	if (!ft_strcasecmp(currentcmd, "default"))
		bChangeDefault = 1;
	else if (!ft_strcasecmp(currentcmd, "all"))
		bChangeDefault = 0;
	else{
		printf("unknown command\n");
		return CMD_FAIL;
	}


	

	//printf("set sample rate to %u\n", rate_val);

	//FTGettop(Connlist);


	//init
	SampleRate.sample_rate = (FT_SAMPLE_RATE *)malloc(sizeof(FT_CONTROL_SAMPLE_RATE)+ MAX_SET_SAMPLE_RATE_NUM * sizeof(FT_SAMPLE_RATE));
	SampleRate.pkt_samp_default = (UINT)-1;
	SampleRate.flow_samp_default = (UINT)-1;

	i = 0;

	if (bChangeDefault == 0)
	{
#if 0
		SampleRate.reason = 1;  //change all flows
		for (hash = 0; hash < CONN_TABLE_SIZE; ++hash)
		{
			EnterCriticalSection(&ConnHashTable[hash].Lock);
			//pEntry = pConnHash->ListHead.Flink;
			for (it = ConnHashTable[hash].flowlist.begin(); it != ConnHashTable[hash].flowlist.end(); ++it)
			{
				SampleRate.sample_rate[i].hash = it->hash;
				SampleRate.sample_rate[i].key.daddr = it->key.daddr;
				SampleRate.sample_rate[i].key.protocol = it->key.protocol;
				//pktsamplerate = min((it->pktsamplerate) * 8, MAXPKTSAMPLENUM);
				SampleRate.sample_rate[i].psrate = (UINT)-1;
				SampleRate.sample_rate[i].fsrate = rate_val;
				it->flowsamplerate = rate_val;

				printf("set sample rate: %u.%u.%u.%u(%u) pkt %u --> rate: %u\n",
					NIPQUAD_HOSTORDER(it->key.daddr), it->key.protocol,
					it->packets,
					SampleRate.sample_rate[i].fsrate);
				++i;
				if (i == MAX_SET_SAMPLE_RATE_NUM)
					break;
			}

			LeaveCriticalSection(&ConnHashTable[hash].Lock);
		}
#endif
	}
	else
	{
		SampleRate.reason = 0;  //change default
		SampleRate.pkt_samp_default = rate_val;
		SampleRate.flow_samp_default = rate_val;
	}

	SampleRate.nflow = i;

	printf("set sample rate to %d ...\n", rate_val);
	if (ft_control_sample_rate(SampleRate) != CMD_OK)
	{
		printf("set sample rate fail %d\n", GetLastError());
		return CMD_FAIL;
	}
	
	return CMD_OK;
}

int
read_kernel_pktset(PKT_EVENT *ppktEvent)
{
	BOOL                status;
	INT                 err;
	QUERY_USER_PACKET_SET   query;

	query.npktmax = MAX_QUERY_PKT_NUM;
	query.buflen = QUERY_PKT_SIZE;

	status = DeviceIoControl(FTHandle,
		IOCTL_FT_RECEIVE_PACKET_SET,
		&query, sizeof(QUERY_USER_PACKET_SET),
		(PVOID)ppktEvent->pktset,
		sizeof(USER_PACKET_SET)+MAX_QUERY_PKT_NUM * (sizeof(USER_PACKET)+QUERY_PKT_SIZE),
		(LPDWORD) &ppktEvent->BytesReturned,
		&ppktEvent->stOverlapped);
	if (!status && (ERROR_IO_PENDING != (err = GetLastError())))
	{
		fprintf(stderr, "snet receive packet err 0x%x\n", err);
		return CMD_FAIL;
	}

	return CMD_OK;
}


cmd_result_t UpdateFlowTable(FLOW_LABEL flow, UINT PacketSize, UINT reason)
{
	UINT            hash;
	//PCONN			pConn;
	
	std::list<CONN>::iterator it;
	std::list<FLOW>::iterator jt;
	CONN			newconn;
	FLOW			newflow;


	//update conn table  (application-specific)

	hash = (((flow.daddr & 0xff) << 24) +
		(((flow.daddr >> 8) & 0xff) << 18) +
		(((flow.daddr >> 16) & 0xff) << 10) +
		(((flow.daddr >> 24) & 0xff) << 2) +
		flow.protocol * 256) % CONN_TABLE_SIZE;
	//hash = (flowkey.daddr + flowkey.protocol) % CONN_TABLE_SIZE;

	EnterCriticalSection(&ConnHashTable[hash].Lock);

	//pConn = FindConn(&ConnHashTable[hash], &flowkey);
	//pConn = NULL;
	//printf("find connection\n");

	for (it = ConnHashTable[hash].flowlist.begin(); it != ConnHashTable[hash].flowlist.end(); ++it)
	{
		if (it->key.daddr == flow.daddr &&
			it->key.protocol == flow.protocol)
		{
			break;
		}
	}
	if (it == ConnHashTable[hash].flowlist.end())
	{
		if (flowtable.count_active_conn < flowtable.max_active_conn)
		{
			//printf("new flow\n");
			//pConn = (PCONN)malloc(sizeof(CONN));
			//if (pConn)
			//{
			//memset(&newconn, 0, sizeof(CONN));  //dont use here cuz we have map element
			newconn.key.saddr = 0;
			newconn.key.daddr = flow.daddr;
			newconn.key.sport = 0;
			newconn.key.dport = 0;
			newconn.key.protocol = flow.protocol;
			newconn.hash = hash;
			//newconn.pktsamplerate = BASEPKTSAMPLERATE;
			//newconn.flowsamplerate = BASEFLOWSAMPLERATE;
			newconn.uniquesource.clear();
			newconn.packets = 0;
			newconn.bytes = 0;
			newconn.numofsource = 0;
			newconn.lasttime = ft_agent_state.lastupdatetime;
			//printf("new flow created3\n");
			//pConn->key.portnum = flowkey.portnum;
			//pConn->lasttime = KeQueryInterruptTime();
			//pConn->lastactivetime = pConn->lasttime;
			//if (bft.kmacfwd)
			//{
			//	pConn->outExtIf = BftMacLookup(ethdr->Dest);
			//}
			//InsertHeadList(&ConnHashTable[hash].ListHead, &pConn->ListEntry);
			//ConnHashTable[hash].flowlist.insert(it, newconn);
			ConnHashTable[hash].flowlist.push_front(newconn);
			it = ConnHashTable[hash].flowlist.begin();
			//it->packets = 2;
			//printf("new: b: %u p: %u, p %u\n", it->bytes, it->packets, newconn.packets);

			InterlockedIncrement((PLONG)&flowtable.count_active_conn);
			//InterlockedIncrement((PLONG)&flowtable.count_total_conn);
			//flowtable.CountTotalConn++;
			if (flowtable.count_active_conn > flowtable.count_max_active_conn)
			{
				flowtable.count_max_active_conn = flowtable.count_active_conn;
			}
			//newflow = 1;
			//printf("add one flow...\n");

			//else
			//{
			//	flowtable.count_allocate_connfail++;
			//}
		}
		else
		{
			flowtable.count_reach_max_active_conn++;
		}
	}
	if (it != ConnHashTable[hash].flowlist.end())
	{

		//printf("before: b: %llu p: %u\n", it->bytes, it->packets);
		if ((reason & FT_PACKET_SAMPLE) == FT_PACKET_SAMPLE)
		{
			//printf("update pkt\n");
			it->bytes += PacketSize;
			it->packets++;
			it->lasttime = ft_agent_state.lastupdatetime;
		}
		if ((reason & FT_FLOW_SAMPLE) == FT_FLOW_SAMPLE)
		{
			//printf("update flow\n");
			//printf("src: %u.%u.%u.%u\n", NIPQUAD_HOSTORDER(flow.saddr));

			if (it->uniquesource.find(flow.saddr) == it->uniquesource.end())
			{

				it->uniquesource.insert(std::pair<UINT, UINT>(flow.saddr, 1));
				it->numofsource += 1;
			}
			else
			{
				it->uniquesource[flow.saddr]++;
			}
			it->lasttime = ft_agent_state.lastupdatetime;

			//it->uniquesource.insert(std::pair<UINT,UINT>(flow.saddr,1));


		}


	}
	//printf("after: b: %llu p: %u\n", it->bytes, it->packets);
	LeaveCriticalSection(&ConnHashTable[hash].Lock);





	// update flow table (generic flow table )
	hash = (flow.saddr + flow.daddr, flow.dport 
		+ flow.sport + flow.protocol) % FLOW_TABLE_SIZE;
	//hash = (flowkey.daddr + flowkey.protocol) % CONN_TABLE_SIZE;

	EnterCriticalSection(&FlowHashTable[hash].Lock);

	//pConn = FindConn(&ConnHashTable[hash], &flowkey);
	//pConn = NULL;
	//printf("find connection\n");

	for (jt = FlowHashTable[hash].flowlist.begin(); jt != FlowHashTable[hash].flowlist.end(); ++jt)
	{
		if (jt->key.saddr == flow.saddr &&
			jt->key.daddr == flow.daddr &&
			jt->key.sport == flow.sport &&
			jt->key.dport == flow.dport &&
			jt->key.protocol == flow.protocol)
		{
			break;
		}
	}
	if (jt == FlowHashTable[hash].flowlist.end())
	{
		if (flowtable.count_active_flow < flowtable.max_active_flow)
		{
			//printf("new flow\n");
			//pConn = (PCONN)malloc(sizeof(CONN));
			//if (pConn)
			//{
			//memset(&newconn, 0, sizeof(CONN));  //dont use here cuz we have map element
			newflow.key.saddr = flow.saddr;
			newflow.key.daddr = flow.daddr;
			newflow.key.sport = flow.sport;
			newflow.key.dport = flow.dport;
			newflow.key.protocol = flow.protocol;
			//newflow.hash = hash;
			//newconn.pktsamplerate = BASEPKTSAMPLERATE;
			//newconn.flowsamplerate = BASEFLOWSAMPLERATE;
			//newflow.uniquesource.clear();
			newflow.packets = 0;
			newflow.bytes = 0;
			newflow.lasttime = ft_agent_state.lastupdatetime;
			//printf("new flow created3\n");
			//pConn->key.portnum = flowkey.portnum;
			//pConn->lasttime = KeQueryInterruptTime();
			//pConn->lastactivetime = pConn->lasttime;
			//if (bft.kmacfwd)
			//{
			//	pConn->outExtIf = BftMacLookup(ethdr->Dest);
			//}
			//InsertHeadList(&ConnHashTable[hash].ListHead, &pConn->ListEntry);
			//ConnHashTable[hash].flowlist.insert(it, newconn);
			FlowHashTable[hash].flowlist.push_front(newflow);
			jt = FlowHashTable[hash].flowlist.begin();
			//jt->packets = 2;
			//printf("new: b: %u p: %u, p %u\n", jt->bytes, jt->packets, newconn.packets);

			InterlockedIncrement((PLONG)&flowtable.count_active_flow);
			//InterlockedIncrement((PLONG)&flowtable.count_total_conn);
			//flowtable.CountTotalConn++;
			if (flowtable.count_active_flow > flowtable.count_max_active_flow)
			{
				flowtable.count_max_active_flow = flowtable.count_active_flow;
			}
			//newflow = 1;
			//printf("add one flow...\n");

			//else
			//{
			//	flowtable.count_allocate_connfail++;
			//}
		}
		else
		{
			flowtable.count_reach_max_active_flow++;
		}
	}

	//printf("before: b: %llu p: %u\n", jt->bytes, jt->packets);
	//printf("update pkt\n");
	if (jt != FlowHashTable[hash].flowlist.end())
	{

		jt->bytes += PacketSize;
		jt->packets++;
		jt->lasttime = ft_agent_state.lastupdatetime;
	}

	LeaveCriticalSection(&FlowHashTable[hash].Lock);


	return CMD_OK;
}



DWORD WINAPI
ft_thread(LPVOID lpParam)
{
	BOOLEAN bTure = TRUE;
	UINT             i;
	UINT             j;
	int             rc = 0;
	HANDLE          hConnectEvent[1];
	DWORD           BytesReturned;
	DWORD           err;
	USER_PACKET_SET *ppktset;
	USER_PACKET     *upkt;
	//FT_INFO_FLOWSET *pflowset;
	FLOW_LABEL		flow;
	//FT_INFO_FLOW    flow;
	ETH_HEADER      *ethdr;
	IP_HEADER       *iphdr;
	L4_HEADER       *l4hdr;
	UINT			PacketSize;


	
	for (i = 0; i < 1; ++i)
	{
		hConnectEvent[i] = CreateEvent(NULL,    /* default security attribute */
			TRUE,    /* manual reset event */
			FALSE,   /* initial state = unsignaled */
			NULL);   /* unnamed event object */
		if (hConnectEvent[i] == NULL)
		{
			DWORD       dwError = GetLastError();
			printf("Could not CreateEvent: %d\n", dwError);
			goto thread_exit;
		}
	}

	PktEvent.pktset = (USER_PACKET_SET *) malloc(sizeof(USER_PACKET_SET)+MAX_QUERY_PKT_NUM*(sizeof(USER_PACKET)+QUERY_PKT_SIZE));
	memset(&PktEvent.stOverlapped, 0, sizeof(OVERLAPPED));
	PktEvent.stOverlapped.hEvent = hConnectEvent[0];
	read_kernel_pktset(&PktEvent);

	//printf("thread start......\n");
	printf("thread started...\n");
	do
	{

		if (ft_thread_flags & FT_THREAD_F_STOP)
		{
			break;
		}

		rc = WaitForMultipleObjectsEx(1, hConnectEvent, 0, ft_periodic_check / 1000, TRUE);

		if (rc == WAIT_OBJECT_0)
		{
			/* receive new flow packet */
			rc = GetOverlappedResult(FTHandle, &PktEvent.stOverlapped, &BytesReturned, FALSE);
			if (!rc)
			{
				err = GetLastError();
				fprintf(stderr, "GetOverlappedResult 0x%x\n", err);
				exit(1);
			}

			
			//gettimeofday(&tv);
			ppktset = PktEvent.pktset;
			
			//pflow = &flow;
			for (j = 0; j < ppktset->npkt; ++j)
			{
				flowtable.count_packet_receive++;

				upkt = (PUSER_PACKET)(ppktset->upkt + j * (sizeof(USER_PACKET)+QUERY_PKT_SIZE));
				ethdr = (ETH_HEADER *)upkt->buf;
				iphdr = (IP_HEADER *)(ethdr + 1);
				l4hdr = (L4_HEADER *)(iphdr + 1);

				//printf("psize %u, %d, %x\n", ntohs(iphdr->length), ntohs(iphdr->length), ntohs(iphdr->length));
				PacketSize = ntohs(iphdr->length) + sizeof(ETH_HEADER);

				flow.saddr = ntohl(iphdr->src);
				flow.daddr = ntohl(iphdr->dest);
				flow.sport = ntohs(l4hdr->sport);
				flow.dport = ntohs(l4hdr->dport);
				flow.protocol = iphdr->protocol;
				
				//pflow->incpkts = 1;

				//snet_debug(SNET_INFO, "rcvpkt %u.%u.%u.%u.%u -> %u.%u.%u.%u.%u(%u), len %u\n",
				//NIPQUAD_HOSTORDER(pflow->srcip), pflow->sport,
				//NIPQUAD_HOSTORDER(pflow->dstip), pflow->dport, pflow->protocol,
				//upkt->len);

				//if (find_hwflow(pflow))
				//{

				//bft.count_hwflow_dup++;
				//snet_debug(SNET_INFO, "flow already in hw\n");
				//continue;
				//}

				//swap_hwflow(unit, pflow, &tv);

				//upkt->outIntIf = upkt->inIntIf;
				//upkt->outExtIf = UNKNOWN_EXTERNAL_PORT;
				//upkt->stype = UPSTYPE_UNICAST_POSTTUNNEL;
				//upkt->priority = 0;
				//upkt->vlanid = 0;
				UpdateFlowTable(flow, PacketSize, upkt->reason);

				ft_agent_state.processpackets++;
				ft_agent_state.processbytes += PacketSize;

			}

			//rc = SNetSendPacketSet(ppktset);
			//if (rc != SNET_SUCCESS)
			//	{
			//	snet_printf("[bft_thread] failed to send packet set\n");
			//}
			
			//ft_agent_state.samplepps += ppktset->npkt;
			//ft_agent_state.samplembs += ppktset->npkt;

			ResetEvent(PktEvent.stOverlapped.hEvent);
			read_kernel_pktset(&PktEvent);
		}
		else if (rc == WAIT_TIMEOUT)
		{
			// do nothing
			//gettimeofday(&tv);
		}
		else
		{
			// may be we receive ctrl-c here?
			fprintf(stderr, "receive error, maybe snet ctrl-c 0x%x\n", rc);
			break;
		}
		//printf("test\n");
		//Sleep(3000);
	} while (bTure);


thread_exit:

	ft_thread_pid = 0;

	//ft_thread_flags = 0;
	return CMD_OK;
}


INT
ft_start_thread()
{
	FT_CONTROL_PROTOCOL    Control;

	//if (ft_thread_sync == NULL)
	//{
	//	return CMD_FAIL;
	//}

	init_ft_control_protocol(&Control);
	//Control.mode = 1;
	if (ft_control_protocol(Control) != CMD_OK)
	{
		printf("Unable to init\n");
		return CMD_FAIL;
	}

	if ((ft_thread_flags & FT_THREAD_F_RUN) == FT_THREAD_F_RUN)
	{

		printf("thread has already been started\n");
		return CMD_FAIL;
	}

	ft_thread_flags |= FT_THREAD_F_RUN;

	ft_thread_handle = CreateThread(
		NULL,
		0,
		ft_thread,
		NULL,
		0,
		&ft_thread_pid);

	if (ft_thread_handle == NULL)
	{
		printf("Unable to start task\n");
		ft_thread_flags &= ~FT_THREAD_F_RUN;
		return CMD_FAIL;
	}

	//if (sal_sem_take(bft_thread_sync, sal_sem_FOREVER))
	//{
	//	snet_printf("%s: Failed to wait for start\n", bft_name);
	//	return -1;
	//}
	
	return CMD_OK;
}

INT
ft_stop_thread()
{
	//FT_CONTROL_PROTOCOL    Control;
	//soc_timeout_t           to;
	UINT timeout =			10000;
	UINT waittime = 0;
	
	if (!(ft_thread_flags & FT_THREAD_F_RUN))
	{
		printf("ft_thread is not running\n");
		return CMD_FAIL;
	}

	//init_ft_control_protocol(&Control);
	//Control.mode = 0;
	//if (ft_control_protocol(Control) != CMD_OK)
	//{
	//	printf("Unable to init\n");
	//}

	//ql_timeout = 10000000;

	ft_thread_flags |= FT_THREAD_F_STOP;

	//soc_timeout_init(&to, ql_timeout, 0);

	/* Check for ql_thread exit */
	//Sleep(500);
	while (ft_thread_pid != 0 && waittime < timeout)
	{
		Sleep(1000);
		waittime += 1000;
	}

	if (ft_thread_pid == 0)
	{
		CloseHandle(ft_thread_handle);
		printf("thread stopped\n");
	}
	else
	{
		printf("thread cannot stop\n");
	}

	//sal_sem_take(bft_thread_sync, sal_sem_FOREVER);

	//ft_thread_flags &= ~FT_THREAD_F_RUN;
	ft_thread_flags = 0;

	return 0;
}



cmd_result_t CleanTopKTable()  //reason = 0: running check; reason = 1: force clean
{
	UINT i;
	UINT hash;
	std::list<CONN>::iterator it;


	EnterCriticalSection(&TopKTable.Lock);


	for (i = 0; i < TopKTable.K; ++i)
	{
		hash = TopKTable.TopKTable[i].hash;
		EnterCriticalSection(&ConnHashTable[hash].Lock);
		for (it = ConnHashTable[hash].flowlist.begin(); it != ConnHashTable[hash].flowlist.end();)
		{
			if (it->key.daddr == TopKTable.TopKTable[i].daddr
				&& it->key.protocol == TopKTable.TopKTable[i].protocol)
			{
				ConnHashTable[hash].flowlist.erase(it++);
				InterlockedDecrement((PLONG)&flowtable.count_active_conn);
				break;
			}
			else
				++it;
		}

		LeaveCriticalSection(&ConnHashTable[hash].Lock);
	}

	TopKTable.reported = 0;
	TopKTable.K = 0;

	LeaveCriticalSection(&TopKTable.Lock);

	return CMD_OK;

}


void FindTopK(UINT budget = 0)
{
	std::list<CONN>::iterator it;
	UINT i;
	INT j;
	INT nConn = 0;
	INT nMove = 0;
	TOP_CONN_INFO	topk[TOP_K_TABLE_SIZE];
	std::list<TOP_CONN_INFO>  bottomk[BOTTOM_K_TABLE_SIZE];
	//INT items;
	//BOOL bottom_flag = 0;
	//TOP_CONN_INFO new_conn_info;
	UINT sample_value;

	
#if 0
	if (ft_agent_state.lastupdatetime - TopKTable.lasttime < NOTICE_TIMEOUT)
		return;

	if (TopKTable.reported == 1)
	{
		CleanTopKTable();	
	}

	if (budget == 0 && ft_agent_state.smoothusage < CPU_HIGH_WATERMARK)  //waiting ack or under utilized
	{	
		return;
	}
#endif

	//TopKTable.K = 0;

	//init
	memset(topk, 0, sizeof(TOP_CONN_INFO)* TOP_K_TABLE_SIZE);

	ft_agent_state.totalsamples = 0;

	for (i = 0; i < CONN_TABLE_SIZE; ++i)
	{
		EnterCriticalSection(&ConnHashTable[i].Lock);

		for (it = ConnHashTable[i].flowlist.begin(); it != ConnHashTable[i].flowlist.end(); ++it)
		{
			sample_value = (g_flow_base_sample ? it->numofsource : it->packets);
			ft_agent_state.totalsamples += sample_value;

			//if (sample_value < BOTTOM_K_TABLE_SIZE)
			//{
			//	new_conn_info.hash = it->hash;
			//	new_conn_info.daddr = it->key.daddr;
			//	new_conn_info.protocol = it->key.protocol;
			//	new_conn_info.numofsource = it->numofsource;
			//	new_conn_info.pkt = it->packets;
			//
			//	bottomk[sample_value].push_back(new_conn_info);

			//}		
			//else
			//{
				for (j = nConn - 1; j >= 0; --j)
				{
					if ((g_flow_base_sample && it->numofsource > topk[j].numofsource) 
						|| (g_pkt_base_sample && it->packets > topk[j].pkt))
					{
						if (j + 1 < TOP_K_TABLE_SIZE)
						{
							/* move entry one-step to the tail */
							memcpy_s(&topk[j + 1], sizeof(TOP_CONN_INFO), &topk[j], sizeof(TOP_CONN_INFO));
							//TopKTable.TopKTable[j + 1].numofsource = TopKTable.TopKTable[j].numofsource;
							//TopKTable.TopKTable[j + 1].daddr = TopKTable.TopKTable[j].daddr;
							//TopKTable.TopKTable[j + 1].protocol = TopKTable.TopKTable[j].protocol;
						}
						else
						{
							/* remove the last entry */
							nConn--;
						}
					}
					else
					{
						break;
					}
				}
				if (j + 1 < TOP_K_TABLE_SIZE)
				{
					topk[j + 1].hash = it->hash;
					topk[j + 1].daddr = it->key.daddr;
					topk[j + 1].protocol = it->key.protocol;
					topk[j + 1].numofsource = it->numofsource;
					topk[j + 1].pkt = it->packets;
					nConn++;
				}

				//reset counters
				it->numofsource = 0;
				it->bytes = 0;
				it->packets = 0;
				//it->uniquesource.clear();
				

			}
		//}
		LeaveCriticalSection(&ConnHashTable[i].Lock);

	}

#if 0
	//want to move?
	items = 0;
	if (ft_agent_state.smoothusage >= CPU_HIGH_WATERMARK)
	{
		
		items = (INT)((ft_agent_state.smoothusage - CPU_HIGH_WATERMARK)
			* ft_agent_state.totalsamples / ft_agent_state.smoothusage);
		//printf("remove %d\n", items);
	}
	else if (budget != 0)
	{
		items = budget;
	}

	UINT temp = items;

	UINT num_in_top = min(nConn, TOP_K_TABLE_SIZE);

	for (i = 0; i < num_in_top; ++i)
	{
		if (temp > 0)
		{
			topk[i].mode = 1;
			temp -= (g_flow_base_sample ? topk[i].numofsource : topk[i].pkt);
			nMove++;
		}
		else
			break;
	}

	printf("conn: %u, items: %u, cpu %u, sample: %u\n", 
		nMove,
		items, 
		ft_agent_state.smoothusage, 
		ft_agent_state.totalsamples);
#else
	nMove = nConn;
#endif

	EnterCriticalSection(&TopKTable.Lock);
#if 0
	if (ft_agent_state.lastupdatetime - TopKTable.lasttime < NOTICE_TIMEOUT)
	{
		LeaveCriticalSection(&TopKTable.Lock);
		return;
	}
	TopKTable.reported = 1;
#endif
	TopKTable.K = nMove;
	memcpy_s(TopKTable.TopKTable, sizeof(TOP_CONN_INFO)* TOP_K_TABLE_SIZE, topk, sizeof(TOP_CONN_INFO)* nMove);
	TopKTable.lasttime = ft_agent_state.lastupdatetime;
	LeaveCriticalSection(&TopKTable.Lock);

}


void CleanTable()
{
	std::list<CONN>::iterator it;
	std::list<FLOW>::iterator jt;
	UINT i;

	for (i = 0; i < CONN_TABLE_SIZE; ++i)
	{
		EnterCriticalSection(&ConnHashTable[i].Lock);

		for (it = ConnHashTable[i].flowlist.begin(); it != ConnHashTable[i].flowlist.end();)
		{
			if ((INT)(ft_agent_state.lastupdatetime - it->lasttime) >= INACTIVE_TIMEOUT)
			{
				ConnHashTable[i].flowlist.erase(it++);
				InterlockedDecrement((PLONG)&flowtable.count_active_conn);
			}
			else
				++it;
		}

		LeaveCriticalSection(&ConnHashTable[i].Lock);
	}


	for (i = 0; i < FLOW_TABLE_SIZE; ++i)
	{
		EnterCriticalSection(&FlowHashTable[i].Lock);

		for (jt = FlowHashTable[i].flowlist.begin(); jt != FlowHashTable[i].flowlist.end();)
		{
			if ((INT)(ft_agent_state.lastupdatetime - jt->lasttime) >= INACTIVE_TIMEOUT)
			{
				FlowHashTable[i].flowlist.erase(jt++);
				InterlockedDecrement((PLONG)&flowtable.count_active_flow);
			}
			else
				++jt;
		}

		LeaveCriticalSection(&FlowHashTable[i].Lock);
	}
}

DWORD WINAPI
ft_timer_thread(LPVOID lpParam)
{
	LARGE_INTEGER frequency;
	LARGE_INTEGER start;
	LARGE_INTEGER next;
	LARGE_INTEGER stop;

	//LARGE_INTEGER begin;
	//LARGE_INTEGER end;
	LARGE_INTEGER lap;

	UINT      i;
	BOOLEAN bTrue = TRUE;
	double  elap;
	//DWORD  sleeptime;
	FT_INFO_PERFORMANCE Info;

	INT kernel_usage;
	INT user_usage;
	INT total_usage;

	UINT memusage;

	INT rc;
	

	

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);

	next.QuadPart = start.QuadPart + FT_EPOCH * frequency.QuadPart / 1000;
	i = 1;

	printf("thread started\n");
	do
	{
		if (ft_timer_thread_flags & FT_THREAD_F_STOP)
		{
			break;
		}
		
		//ft_set_sample_rate();
		QueryPerformanceCounter(&stop);

		
		//lap.QuadPart = end.QuadPart - begin.QuadPart;
		//elap = (double)lap.QuadPart / (double)frequency.QuadPart;


		//sleeptime = max(0, (DWORD)(FT_EPOCH - elap * 1000));
		//Sleep(sleeptime);

		//QueryPerformanceCounter(&stop);

		while (stop.QuadPart < next.QuadPart)
		{
			lap.QuadPart = next.QuadPart - stop.QuadPart;
			elap = (double)lap.QuadPart / (double)frequency.QuadPart;
			
			Sleep((DWORD)(elap * 1000));

			QueryPerformanceCounter(&stop);
		}

		next.QuadPart = start.QuadPart + (i + 1) * FT_EPOCH * frequency.QuadPart / 1000;
		++i;
			
		//timer is set here
		ft_query_performance(&Info);

		ft_agent_state.fwdpps = Info.CurrentPPS;
		ft_agent_state.fwdmbs = Info.CurrentMBS;
		ft_agent_state.lastupdatetime = (UINT)Info.LastTimerUpdateTime;
		ft_agent_state.inboundpps = Info.CurrentInboundPPS;

		rc = systemusage.GetCPUUsage(&user_usage, &kernel_usage, &total_usage);
		if (rc == CMD_OK)
		{

			ft_agent_state.userusage = user_usage;
			ft_agent_state.kernelusage = kernel_usage;
			ft_agent_state.totalusage = total_usage;

			ft_agent_state.smoothusage = (UINT)(ft_agent_state.smoothusage * 0.8 + total_usage * 0.2);

		}

		rc = systemusage.GetMemUsage(&memusage);
		if (rc == CMD_OK)
		{
			ft_agent_state.memusage = memusage;
		}

		ft_agent_state.countconn = flowtable.count_active_conn;
		ft_agent_state.countflow = flowtable.count_active_flow;
			

		ft_agent_state.samplepps = ft_agent_state.processpackets;
		ft_agent_state.samplembs = UINT(ft_agent_state.processbytes * 8 / 1000000);

		FindTopK();
		
		//UINT top = min(8, TopKTable.K);
		//for (i = 0; i < top; ++i)
		//{
		//	printf("%u.%u.%u.%u(%u) num = %u\n", NIPQUAD_HOSTORDER(TopKTable.TopKTable[i].daddr),
		//		TopKTable.TopKTable[i].protocol, TopKTable.TopKTable[i].numofsource);
		//}

		if ((INT)(ft_agent_state.lastupdatetime - ft_agent_state.lastcleantime) > FT_CLEAN_EPOCH)
		{
			//printf("table clean\n");
			CleanTable();
			ft_agent_state.lastcleantime = ft_agent_state.lastupdatetime;
		}

		
		//lap.QuadPart = stop.QuadPart - start.QuadPart;
		//elap = (double)lap.QuadPart / (double)frequency.QuadPart;
		//printf("%d: %f sec, last: %u, pps: %u, mbs: %u\n", i, elap, 
			//(UINT)(Info.LastTimerUpdateTime / 1000), Info.CurrentPPS, Info.CurrentMBS);
		//printf("user: %d, kernel %d, total: %d, mem: %d KB\n", ft_agent_state.userusage, 
			//ft_agent_state.kernelusage, ft_agent_state.totalusage, ft_agent_state.memusage);

	} while (bTrue);
	
	ft_timer_thread_pid = 0;

	//ft_thread_flags = 0;
	return CMD_OK;
}

INT
ft_start_timer_thread()
{
	ft_timer_thread_flags |= FT_THREAD_F_RUN;

	ft_timer_thread_handle = CreateThread(
		NULL,
		0,
		ft_timer_thread,
		NULL,
		0,
		&ft_timer_thread_pid);

	if (ft_timer_thread_handle == NULL)
	{
		printf("Unable to start task\n");
		ft_timer_thread_flags &= ~FT_THREAD_F_RUN;
		return CMD_FAIL;
	}

	//if (sal_sem_take(bft_thread_sync, sal_sem_FOREVER))
	//{
	//	snet_printf("%s: Failed to wait for start\n", bft_name);
	//	return -1;
	//}
	printf("timer start......\n");
	return CMD_OK;
}

INT
ft_stop_timer_thread()
{
	//soc_timeout_t           to;
	UINT timeout = 10000;
	UINT waittime = 0;

	if (!(ft_timer_thread_flags & FT_THREAD_F_RUN))
	{
		printf("ft_timer_thread is not running\n");
		return CMD_FAIL;
	}

	ft_timer_thread_flags |= FT_THREAD_F_STOP;

	while (ft_timer_thread_pid != 0 && waittime < timeout)
	{
		Sleep(1000);
		waittime += 1000;
	}

	if (ft_timer_thread_pid == 0)
	{
		CloseHandle(ft_timer_thread_handle);
		printf("timer stopped\n");
	}
	else
	{
		printf("timer cannot stop\n");
	}

	//sal_sem_take(bft_thread_sync, sal_sem_FOREVER);

	//ft_timer_thread_flags &= ~FT_THREAD_F_RUN;
	ft_timer_thread_flags = 0;

	return 0;
}


int
user_flowtable_init()
{
	int i;
	for (i = 0; i < CONN_TABLE_SIZE; ++i)
	{
		InitializeCriticalSection(&ConnHashTable[i].Lock);
		//InitializeListHead(&ConnHashTable[i].ListHead);
		ConnHashTable[i].flowlist.clear();
	}

	for (i = 0; i < FLOW_TABLE_SIZE; ++i)
	{
		InitializeCriticalSection(&FlowHashTable[i].Lock);
		//InitializeListHead(&ConnHashTable[i].ListHead);
		FlowHashTable[i].flowlist.clear();
	}


	memset(&flowtable, 0, sizeof(flowtable));

	flowtable.initialized = 0;
	//bft.mode = 0;
	//bft.kmacfwd = 0;
	flowtable.max_active_conn = FT_MAX_ACTIVE_FLOW_NUM;
	flowtable.max_active_flow = FT_MAX_ACTIVE_FLOW_NUM;


	//lock for topktable
	InitializeCriticalSection(&TopKTable.Lock);
	TopKTable.reported = 0;

	return CMD_OK;
}

cmd_result_t ReportStatus()
{
	DWORD Flags = 0;
	WSABUF CmdBuf;
	CmdBuf.buf = new char[MAXBUFLEN];
	//CmdBuf.buf = buf;
	//CmdBuf.len = MAXBUFLEN;
	DWORD currLen;
	INT curr;
	int rc;
	char buf[10];
	UINT i;
	//printf("start to report status\n");

	memset(CmdBuf.buf, '0', MAXBUFLEN); 
	CmdBuf.buf[0] = '\0';

	strcat_s(CmdBuf.buf, MAXBUFLEN, "cpuuser ");
	_itoa_s((INT)ft_agent_state.userusage, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	strcat_s(CmdBuf.buf, MAXBUFLEN, " cpuker ");
	_itoa_s((INT)ft_agent_state.kernelusage, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	strcat_s(CmdBuf.buf, MAXBUFLEN, " cputotal ");
	_itoa_s((INT)ft_agent_state.totalusage, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	strcat_s(CmdBuf.buf, MAXBUFLEN, " cpusmooth ");
	_itoa_s((INT)ft_agent_state.smoothusage, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	strcat_s(CmdBuf.buf, MAXBUFLEN, " mem ");
	_itoa_s((INT)ft_agent_state.memusage, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	strcat_s(CmdBuf.buf, MAXBUFLEN, " PPS ");
	
	_itoa_s((INT)ft_agent_state.fwdpps, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	strcat_s(CmdBuf.buf, MAXBUFLEN, " MBS ");
	//printf("mbs = %u\n", ft_agent_state.fwdmbs);
	_itoa_s((INT)ft_agent_state.fwdmbs, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	strcat_s(CmdBuf.buf, MAXBUFLEN, " spps ");
	_itoa_s((INT)ft_agent_state.samplepps, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	strcat_s(CmdBuf.buf, MAXBUFLEN, " smbs ");
	//printf("mbs = %u\n", ft_agent_state.fwdmbs);
	_itoa_s((INT)ft_agent_state.samplembs, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);


	strcat_s(CmdBuf.buf, MAXBUFLEN, " conn ");
	//printf("mbs = %u\n", ft_agent_state.fwdmbs);
	_itoa_s((INT)ft_agent_state.countconn, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	strcat_s(CmdBuf.buf, MAXBUFLEN, " flow ");
	//printf("mbs = %u\n", ft_agent_state.fwdmbs);
	_itoa_s((INT)ft_agent_state.countflow, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	strcat_s(CmdBuf.buf, MAXBUFLEN, " sum ");
	//printf("mbs = %u\n", ft_agent_state.fwdmbs);
	_itoa_s((INT)ft_agent_state.totalsamples, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	EnterCriticalSection(&TopKTable.Lock);
	strcat_s(CmdBuf.buf, MAXBUFLEN, " ktop ");
	//printf("mbs = %u\n", ft_agent_state.fwdmbs);
	_itoa_s((INT)TopKTable.K, buf, 10);
	strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

	for (i = 0; i < TopKTable.K; ++i)
	{
		//if (TopKTable.TopKTable[i].mode == 0)
		//	continue;

		strcat_s(CmdBuf.buf, MAXBUFLEN, " top ");
		//printf("mbs = %u\n", ft_agent_state.fwdmbs);
		_ui64toa_s(TopKTable.TopKTable[i].daddr, buf, 20, 10);
		strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

		strcat_s(CmdBuf.buf, MAXBUFLEN, ";");
		_itoa_s(TopKTable.TopKTable[i].protocol, buf, 10);
		strcat_s(CmdBuf.buf, MAXBUFLEN, buf);

		strcat_s(CmdBuf.buf, MAXBUFLEN, ";");
		if (g_flow_base_sample)
			_itoa_s(TopKTable.TopKTable[i].numofsource, buf, 10);
		else if(g_pkt_base_sample)
			_itoa_s(TopKTable.TopKTable[i].pkt, buf, 10);
		strcat_s(CmdBuf.buf, MAXBUFLEN, buf);
	}

	LeaveCriticalSection(&TopKTable.Lock);
	//report once
	//TopKTable.K = 0;
	
	//strcat_s(CmdBuf.buf)
	//printf();
	strcat_s(CmdBuf.buf, MAXBUFLEN, " ");
	curr = (INT)strnlen(CmdBuf.buf, MAXBUFLEN);
	memset(&CmdBuf.buf[curr], '0', MAXBUFLEN - curr);
	CmdBuf.buf[MAXBUFLEN - 1] = '\0';
	CmdBuf.len = MAXBUFLEN;
	//CmdBuf.len = (ULONG)strnlen(CmdBuf.buf, MAXBUFLEN);
	//printf("%d\n", CmdBuf.len);


	rc = WSASend(ft_agent_state.agent_sock, &CmdBuf, 1, &currLen, Flags, NULL, NULL);

	if (rc == SOCKET_ERROR)
	{
		printf("send error: 0x%x\n", GetLastError());
		closesocket(ft_agent_state.agent_sock);
		ft_agent_state.agent_sock = INVALID_SOCKET;
		free(CmdBuf.buf);
		return CMD_FAIL;
	}

	//printf("status reported\n");

	free(CmdBuf.buf);
	return CMD_OK;
}

DWORD WINAPI StartAgent(LPVOID paramPtr)
{

	WSABUF CmdBuf;
	CmdBuf.buf = new char[MAXBUFLEN];
	//CmdBuf.buf = buf;
	CmdBuf.len = MAXBUFLEN;
	DWORD currLen;

	//int sock = *(int *)paramPtr;
	int rc;

	printf("VM %d connected wait for command\n", ft_agent_state.agent_sock);

	do {

		DWORD Flags = 0;
		rc = WSARecv(ft_agent_state.agent_sock, &CmdBuf, 1, &currLen, &Flags, NULL, NULL);

		if (rc == SOCKET_ERROR)
		{
			printf("rec error: 0x%x\n", GetLastError());
			closesocket(ft_agent_state.agent_sock);
			ft_agent_state.agent_sock = INVALID_SOCKET;
			break;
		}

		if (currLen == 0) {
			printf("connect closed\n");
			closesocket(ft_agent_state.agent_sock);
			ft_agent_state.agent_sock = INVALID_SOCKET;
			break;
		}
		//printf("received\n");
		char *cmd;
		char *token = " ";
		char *next_token;
		CmdBuf.buf[currLen] = '\0';
		//printf("cmd: %s\n", CmdBuf.buf);
		cmd = strtok_s(CmdBuf.buf, token, &next_token);

		if (!ft_strcasecmp(cmd, "startvm"))
		{
			printf("connect with controller and init\n");
			ft_init();
		}
		else if (!ft_strcasecmp(cmd, "stopvm"))
			printf("get stop");
		else if (!ft_strcasecmp(cmd, "getstate"))
			ReportStatus();
		else if (!ft_strcasecmp(cmd, "withdraw"))
		{
			//UINT budget = atoi(next_token);
			//FindTopK(budget);
		}
		//if (!ft_strcasecmp(CmdBuf.buf, "redirected"))
			//;
			//CleanTopKTable(1);

	} while (true);

	return 0;

}


DWORD WINAPI StartServer(LPVOID paramPtr)
{
	SOCKET mSock;
	int rc;
	int addrLen;
	SOCKET connectedSock;
	sockaddr_in clientAddr;
	sockaddr_in serveraddr;
	//DWORD ThreadID;
	//HANDLE hThread;

	UNREFERENCED_PARAMETER(paramPtr);


	printf("server start\n");

	mSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (mSock == INVALID_SOCKET)
	{
		printf("invalid socket\n");
		return -1;
	}

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(CONTROL_PORT);
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	rc = bind(mSock, (sockaddr *)&serveraddr, sizeof(sockaddr));
	if (mSock == INVALID_SOCKET)
	{
		printf("bind error\n");
		return -1;
	}

	rc = listen(mSock, 5);
	if (mSock == INVALID_SOCKET)
	{
		printf("listen error\n");
		return -1;
	}

	ft_agent_state.agent_sock = INVALID_SOCKET;

	while (true) {
		// accept a connection
		addrLen = sizeof(clientAddr);
		connectedSock = accept(mSock, (struct sockaddr*) &clientAddr, &addrLen);

		// handle accept being interupted
		if (connectedSock == INVALID_SOCKET  &&  errno == EINTR) {
			//printf("Socket Accept: INVALID_SOCKET\n");
			continue;
		}
		//printf();

		if (ft_agent_state.agent_sock != INVALID_SOCKET) //we have already communicated with controller
		{
			closesocket(connectedSock);
			continue;
		}
		ft_agent_state.agent_sock = connectedSock;

		ft_agent_thread_handle = CreateThread(NULL, 0, StartAgent, NULL, 0, &ft_agent_thread_pid);
		rc = SetThreadPriority(ft_agent_thread_handle, THREAD_PRIORITY_TIME_CRITICAL);
		if (rc == 0)
		{
			printf("Set priority fail %d\n", GetLastError());
		}

		if (ft_agent_thread_handle == NULL)
		{
			printf("create agent thread fail\n");
			return CMD_FAIL;
		}

		//return connectedSock;
	}
	return CMD_OK;
}


//
//DWORD WINAPI StartClient(LPVOID paramPtr)
//{
//	SOCKET mSock;
//	int rc;
//
//	WSABUF CmdBuf;
//	sockaddr_in serveraddr;
//
//
//	printf("client start\n");
//
//	mSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (mSock == INVALID_SOCKET)
//	{
//		printf("invalid socket: %d\n", GetLastError());
//		return -1;
//	}
//
//	serveraddr.sin_family = AF_INET;
//	serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
//	serveraddr.sin_port = htons(CONTROL_PORT);
//
//	rc = connect(mSock, (sockaddr *)&serveraddr, sizeof(serveraddr));
//	if (rc == SOCKET_ERROR)
//	{
//		printf("connect error = %d\n", GetLastError());
//		return -1;
//	}
//
//
//
//
//	CmdBuf.buf = new char[MAXBUFLEN];
//	CmdBuf.len = 2;
//	CmdBuf.buf[0] = 's';
//
//	DWORD currLen;
//	DWORD Flags = 0;
//	//char word[10];
//
//
//	printf("server connected\n");
//
//
//	do
//	{
//		//while (strcmp(gets_s(word, 10), "send") != 0);
//
//
//		rc = WSASend(mSock, &CmdBuf, 1, &currLen, Flags, NULL, NULL);
//
//		if (rc == SOCKET_ERROR)
//		{
//			//WARN_errno(true, "recv");
//			printf("send error = %d\n", GetLastError());
//			//break;
//			return -1;
//		}
//
//		printf("client started\n");
//		break;
//
//	} while (1);
//
//	return 0;
//}



int _tmain(int argc, _TCHAR* argv[])
{
	//_TCHAR *c;
	char cmd[256];
	BOOLEAN bTrue = TRUE;
	HANDLE controlthread;
	DWORD controlthreadid;
	char* next_token = NULL;
	char* token = " ";
	char* command = NULL;

	bool bAlreadyRunning((::GetLastError() == ERROR_ALREADY_EXISTS));
	if (hMutexOneInstance == NULL || bAlreadyRunning)
	{
		if (hMutexOneInstance)
		{
			::ReleaseMutex(hMutexOneInstance);
			::CloseHandle(hMutexOneInstance);
		}
		printf("already running\n");
		throw std::exception("The application is already running");
	}
	//printf("running\n");
	//exit(1);


	printf("set real time process\n");
	if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
	{
		printf("set process priority fail %d", GetLastError());
		return CMD_FAIL;
	}


	if (FTHandle == NULL)
	{
		if (FTOpenDevice(WIN_FT_DEVICE_NAME) != FT_SUCCESS)
		{
			return CMD_FAIL;
		}
	}

	if (user_flowtable_init() != CMD_OK)
	{
		printf("init fail\n");
		return CMD_FAIL;
	}

	WSADATA wsaData;
	int rc = WSAStartup(0x202, &wsaData);
	if (rc == SOCKET_ERROR)
	{
		printf("socket error\n");
		return -1;
	}

	controlthread = CreateThread(NULL, 0, StartServer, NULL, 0, &controlthreadid);
	if (controlthread == NULL)
	{
		printf("create thread fail\n");
		return CMD_FAIL;
	}
	//if (argc < 2)
	//{
	//	printf("more arguments\n");
	//	return 0;
	//}

	ft_init();
	printf("init\n");

	char sampcmd[] = "default 41";
	ft_set_sample_rate(sampcmd);

	ft_start_thread();
	printf("start\n");
	ft_start_timer_thread();
	printf("check\n");



	printf("input command\n");
	

	//memset(cmd, 0, 256);
	gets_s(cmd);
	//while ((c = argv[1]) != NULL)
	do 
	{
		command = strtok_s(cmd, token, &next_token);

		if (!ft_strcasecmp(command, "init"))
		{
			ft_init();
		}
		else if (!ft_strcasecmp(command, "deinit"))
		{
			ft_deinit();
		}
		else if (!ft_strcasecmp(command, "query"))
		{
			(ft_query());
		}
		else if (!ft_strcasecmp(command, "set"))
		{
			
			(ft_control(next_token));
		}
#if 0
		else if (!ft_strcasecmp(command, "flow"))
		{
			(ft_getflowset());
		}
#endif
		else if (!ft_strcasecmp(command, "stop"))
		{
			(ft_stop_thread());
		}
		else if (!ft_strcasecmp(command, "start"))
		{
			(ft_start_thread());
		}
		else if (!ft_strcasecmp(command, "uncheck"))
		{
			(ft_stop_timer_thread());
		}
		else if (!ft_strcasecmp(command, "check"))
		{
			(ft_start_timer_thread());
		}
		else if (!ft_strcasecmp(command, "exit"))
		{
			printf("program exit\n");
			break;
		}
		else if (!ft_strcasecmp(command, "rate"))
		{
			//printf("dummy test\n");
			//UINT rate = atoi(next_token);
			ft_set_sample_rate(next_token);  //test
		}
		else if (!ft_strcasecmp(command, "test"))
		{
			printf("dummy test\n");
		}
		else
		{
			printf("unknown command\n");
		}

		memset(cmd, 0, 256);
		gets_s(cmd);
	} while (bTrue);
	return 1;
}

