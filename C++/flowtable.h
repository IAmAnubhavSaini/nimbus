#ifndef _FLOWTABLE_H
#define _FLOWTABLE_H
#include "precomp.h"
#include "wdm.h"

#pragma warning(disable:28930) // Unused assignment of pointer, by design in samples
#pragma warning(disable:28931) // Unused assignment of variable, by design in samples

#define MICROSECOND (10)
#define MILLISECOND (1000 * MICROSECOND)
#define SECOND      (1000 * MILLISECOND)
#define MINUTE      (60 * SECOND)

typedef unsigned __int64 ULONGLONG;


typedef struct _VELAN
{
	PIRP                        UserRecvPacketSetIrp;
	NDIS_SPIN_LOCK              UserRecvPacketSetSpinLock;      /* Global Spin Lock to process UserRecvPacketSet */

	PUSER_PACKET                UserRecvPacketTable[USER_RECV_PACKET_TABLE_SIZE];
	PUCHAR                      UserRecvPacketTableBuffer;
	NDIS_SPIN_LOCK              UserRecvPacketTableSpinLock;
	UINT                        UserRecvPacketTableHead;
	UINT                        UserRecvPacketTableTail;

	KDPC                        TimeoutDpc;
	Time                        Timeout;
	KTIMER                      Timer;
	Time						LastTimerUpdateTime;
} VELAN;





typedef struct _CONN
{
	LIST_ENTRY      ListEntry;
	BOOLEAN         InUse;
	FLOW_LABEL      key;
	UINT            status;             /* FLOW_BEGIN, LONG_FLOW_MARK, REPORTED */
	//UINT            lastpkts;
	//ULONGLONG       lastbytes;
	UINT            pkts;
	ULONGLONG       bytes;
	//ULONGLONG       lastactivetime;     /* KeQueryInterruptTime(), 100ns granularity */
	//ULONGLONG       lasttime;           /* KeQueryInterruptTime(), 100ns granularity */
	//ULONG           incpkts;
	//ULONG           incbytes;
	//ULONG           inctime;            /* ms granularity */
	UINT			PktSampleRate;		//sample rate times MAXSAMPLENUM
	UINT			FlowSampleRate;		//sample rate times MAXSAMPLENUM
	//UCHAR         outExtIf;
} CONN, *PCONN;

typedef struct _CONN_HASH_ENTRY
{
	NDIS_SPIN_LOCK      Lock;
	LIST_ENTRY          ListHead;
} CONN_HASH_ENTRY, *PCONN_HASH_ENTRY;

typedef struct _FLOWTABLE
{
	INT             initialized;
	UINT			enable_pkt_samp;
	UINT			enable_flow_samp;
	UINT			pkt_samp_default;
	UINT			flow_samp_default;
	//INT             mode;
	//INT             kmacfwd;
	//UINT            MinNotifyPacket;            /* minimum incr packets for user notification */
	UINT            MaxActiveConn;
	//ULONGLONG       FlowTimeout;                /* timeout for a flow. 100ns granularity */
	//ULONGLONG       FlowReportPeriod;           /* 100ns granularity */

	//ULONGLONG       LastFlowReportTime;

	UINT            CountActiveConn;
	UINT            CountMaxActiveConn;
	UINT            CountTotalConn;             /* all conn num */

	//UINT            CountTotalNotifyFlowNum[BFT_FLOW_TYPE_NUM];
	UINT            CountSamplePacket;    //traffic samples 
	UINT            CountDeliverUserPacket;


	//UINT            CountOtherFwdPackets;
	//ULONGLONG       CountOtherFwdBytes;
	UINT            CountFwdPackets;
	ULONGLONG       CountFwdBytes;

	UINT			CountTokenPackets;
	UINT			MaxTokenSize;
	UINT			CurrentInboundPPS;


	UINT			CurrentPPS;
	UINT			CurrentMBS;

	UINT            CountFlowNotifyTableFull;
	UINT            CountReachMaxActiveConn;
	UINT            CountAllocateConnFail;
	UINT            CountDiscardSmallPacket;
	UINT            CountDiscardBMcastPacket;


	UINT            CountRecvDeliverUserPacket;
	UINT            CountRecvDiscardUserPacket;

	UINT			CountSetSampleRateFail;

	
} FLOWTABLE;



typedef struct _FT_FLOW_NOTIFY_TABLE
{
	FT_INFO_FLOW       flows[MAX_FLOW_NOTIFY_TABLE];
	UINT                head;
	UINT                tail;
	NDIS_SPIN_LOCK      SpinLock;
} FT_FLOW_NOTIFY_TABLE;


int FlowTableInit();
VOID FlowTableDeInit();


PCONN
FindConn(
IN CONN_HASH_ENTRY *pConnHash,
IN FLOW_LABEL      *flowkey);

VOID
FTProcessNetPacket(
IN PNET_BUFFER CurrentNB);
//IN PNET_BUFFER_LIST CurrentNBL);


INT
FTDeliverUserPacket(
IN  PNET_BUFFER     NB,
IN  INT             reason);

VOID FTGettop();
VOID FTFlush();
INT AddDeliverFlow(CONN *pConn);

ULONG
NTAPI
RtlRandom(
_Inout_ PULONG Seed
);


VOID
FTTimeout(
PKDPC Dpc,
void *Context,
void *Unused1,
void *Unused2);

//extern ULONG RtlRandomEx(IN OUT PULONG Seed);
#ifdef KERNEL_TABLE
extern CONN_HASH_ENTRY         ConnHashTable[CONN_TABLE_SIZE];
extern FT_FLOW_NOTIFY_TABLE   FTFlowNotifyTable;
#endif

extern FLOWTABLE               ft;
extern VELAN                 GlobalVElan;

#endif
