//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef __FILTERUSER_H__
#define __FILTERUSER_H__

#define MAXPKTSAMPLENUM 4096
#define MAXFLOWSAMPLENUM 4096
#define BASEPKTSAMPLERATE 41
#define BASEFLOWSAMPLERATE 41




#ifndef ETH_LENGTH_OF_ADDRESS
#define ETH_LENGTH_OF_ADDRESS 6
#endif

#ifndef NDIS_MAX_PHYS_ADDRESS_LENGTH
#define NDIS_MAX_PHYS_ADDRESS_LENGTH 32
#endif

#define ETHERNET_ADDR_LENGTH        6

#define CONN_TABLE_SIZE             100000
#define FLOW_TABLE_SIZE             500000


#define FT_MAX_ACTIVE_FLOW_NUM     10000000
#define MAX_FLOW_NOTIFY_TABLE       64
#define FLOW_NOTIFY_NUM             32
#define USER_RECV_PACKET_TABLE_SIZE     10240

#define MAX_TOKEN_SIZE		1000000


#define IP_TCP_PROTOCOL                 6
#define IP_UDP_PROTOCOL					17

#define htons(x)    _byteswap_ushort((USHORT)(x))
#define ntohs(x)    _byteswap_ushort((USHORT)(x))
#define htonl(x)    _byteswap_ulong((ULONG)(x))
#define ntohl(x)    _byteswap_ulong((ULONG)(x))


#pragma warning(disable:4200) //  zero-sized array in struct/union


typedef unsigned long  DWORD;

#ifdef __WIN64
typedef LONGLONG        LONG_PTR;
typedef unsigned LONGLONG ULONG_PTR;
#else
//typedef long            LONG_PTR;
//typedef unsigned long   ULONG_PTR;
#endif
typedef long long LONGLONG;
typedef void* PVOID;
typedef ULONGLONG Time;
//typedef void *HANDLE;

//
// Temp file to test filter
//

#define _NDIS_CONTROL_CODE(request,method) \
            CTL_CODE(FILE_DEVICE_PHYSICAL_NETCARD, request, method, FILE_ANY_ACCESS)

#define IOCTL_FILTER_RESTART_ALL            _NDIS_CONTROL_CODE(0, METHOD_BUFFERED)
#define IOCTL_FILTER_RESTART_ONE_INSTANCE   _NDIS_CONTROL_CODE(1, METHOD_BUFFERED)
#define IOCTL_FILTER_ENUERATE_ALL_INSTANCES _NDIS_CONTROL_CODE(2, METHOD_BUFFERED)
#define IOCTL_FILTER_QUERY_ALL_STAT         _NDIS_CONTROL_CODE(3, METHOD_BUFFERED)
#define IOCTL_FILTER_CLEAR_ALL_STAT         _NDIS_CONTROL_CODE(4, METHOD_BUFFERED)
#define IOCTL_FILTER_SET_OID_VALUE          _NDIS_CONTROL_CODE(5, METHOD_BUFFERED)
#define IOCTL_FILTER_QUERY_OID_VALUE        _NDIS_CONTROL_CODE(6, METHOD_BUFFERED)
#define IOCTL_FILTER_CANCEL_REQUEST         _NDIS_CONTROL_CODE(7, METHOD_BUFFERED)
#define IOCTL_FILTER_READ_DRIVER_CONFIG     _NDIS_CONTROL_CODE(8, METHOD_BUFFERED)
#define IOCTL_FILTER_WRITE_DRIVER_CONFIG    _NDIS_CONTROL_CODE(9, METHOD_BUFFERED)
#define IOCTL_FILTER_READ_ADAPTER_CONFIG    _NDIS_CONTROL_CODE(10, METHOD_BUFFERED)
#define IOCTL_FILTER_WRITE_ADAPTER_CONFIG   _NDIS_CONTROL_CODE(11, METHOD_BUFFERED)
#define IOCTL_FILTER_READ_INSTANCE_CONFIG   _NDIS_CONTROL_CODE(12, METHOD_BUFFERED)
#define IOCTL_FILTER_WRITE_INSTANCE_CONFIG  _NDIS_CONTROL_CODE(13, METHOD_BUFFERED)
#define IOCTL_FT_QUERY_PROTOCOL				_NDIS_CONTROL_CODE(14, METHOD_BUFFERED)
#define IOCTL_FT_CONTROL_PROTOCOL			_NDIS_CONTROL_CODE(15, METHOD_BUFFERED)
#define IOCTL_FT_QUERY_FLOWSET				_NDIS_CONTROL_CODE(16, METHOD_BUFFERED)
#define IOCTL_FT_RECEIVE_PACKET_SET			_NDIS_CONTROL_CODE(17, METHOD_BUFFERED)
#define IOCTL_FT_CONTROL_SAMPLE_RATE		_NDIS_CONTROL_CODE(18, METHOD_BUFFERED)
#define IOCTL_FT_QUERY_PERFORMANCE			_NDIS_CONTROL_CODE(19, METHOD_BUFFERED)

#define MAX_FILTER_INSTANCE_NAME_LENGTH     256
#define MAX_FILTER_CONFIG_KEYWORD_LENGTH    256

/*
typedef struct _FILTER_DRIVER_ALL_STAT
{
    ULONG          AttachCount;
    ULONG          DetachCount;
    ULONG          ExternalRequestFailedCount;
    ULONG          ExternalRequestSuccessCount;
    ULONG          InternalRequestFailedCount;
} FILTER_DRIVER_ALL_STAT, * PFILTER_DRIVER_ALL_STAT;


typedef struct _FILTER_SET_OID
{
    WCHAR           InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
    ULONG           InstanceNameLength;
    NDIS_OID        Oid;
    NDIS_STATUS     Status;
    UCHAR           Data[sizeof(ULONG)];

}FILTER_SET_OID, *PFILTER_SET_OID;

typedef struct _FILTER_QUERY_OID
{
    WCHAR           InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
    ULONG           InstanceNameLength;
    NDIS_OID        Oid;
    NDIS_STATUS     Status;
    UCHAR           Data[sizeof(ULONG)];

}FILTER_QUERY_OID, *PFILTER_QUERY_OID;

typedef struct _FILTER_READ_CONFIG
{
    _Field_size_bytes_part_(MAX_FILTER_INSTANCE_NAME_LENGTH,InstanceNameLength) 
    WCHAR                   InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
    ULONG                   InstanceNameLength;
    _Field_size_bytes_part_(MAX_FILTER_CONFIG_KEYWORD_LENGTH,KeywordLength) 
    WCHAR                   Keyword[MAX_FILTER_CONFIG_KEYWORD_LENGTH];
    ULONG                   KeywordLength;
    NDIS_PARAMETER_TYPE     ParameterType;
    NDIS_STATUS             Status;
    UCHAR                   Data[sizeof(ULONG)];
}FILTER_READ_CONFIG, *PFILTER_READ_CONFIG;

typedef struct _FILTER_WRITE_CONFIG
{
    _Field_size_bytes_part_(MAX_FILTER_INSTANCE_NAME_LENGTH,InstanceNameLength) 
    WCHAR                   InstanceName[MAX_FILTER_INSTANCE_NAME_LENGTH];
    ULONG                   InstanceNameLength;
    _Field_size_bytes_part_(MAX_FILTER_CONFIG_KEYWORD_LENGTH,KeywordLength) 
    WCHAR                   Keyword[MAX_FILTER_CONFIG_KEYWORD_LENGTH];
    ULONG                   KeywordLength;
    NDIS_PARAMETER_TYPE     ParameterType;
    NDIS_STATUS             Status;
    UCHAR                   Data[sizeof(ULONG)];
}FILTER_WRITE_CONFIG, *PFILTER_WRITE_CONFIG;

*/

typedef struct ft_info_protocol {
	//UCHAR          DeviceAddress[NDIS_MAX_PHYS_ADDRESS_LENGTH];

	INT             initialized;
	UINT			enable_pkt_samp;
	UINT			enable_flow_samp;
	UINT			pkt_default_samp;
	UINT			flow_default_samp;
	INT             mode;
	//INT             MinNotifyPacket;            /* minimum incr packets for user notification */
	//INT             FlowTimeout;                /* timeout for a flow (second) */
	INT             MaxActiveConn;
	//INT             FlowCheckPeriod;            /* millisecond */

	UINT            CountActiveConn;
	UINT            CountMaxActiveConn;
	UINT            CountTotalConn;

	//UINT            CountTotalFlowNotifyNum[BFT_FLOW_TYPE_NUM];
	//UINT            CountDeliverUserPacket;

	//UINT            CountOtherFwdPackets;
	//ULONGLONG       CountOtherFwdBytes;
	//UINT            CountFwdPackets;
	//ULONGLONG       CountFwdBytes;

	UINT			MaxTokenSize;


	UINT            CountAllocateConnFail;
	UINT            CountReachMaxActiveConn;
	UINT            CountDiscardSmallPacket;
	UINT            CountDiscardBMcastPacket;
	//UINT            CountFlowNotifyTableFull;
	UINT			CountDeliverUserPacket;
	UINT			CountRecvDiscardUserPacket;
	UINT			CountSamplePacket;
	UINT			CountSetSampleRateFail;
} FT_INFO_PROTOCOL;

typedef struct ft_control_protocol {
	UINT            initialized;
	UINT			enable_pkt_samp;
	UINT			enable_flow_samp;
	//UINT            mode;                       /* 0: pure sw, 1: hybrid */
	//UINT            MinNotifyPacket;
	//UINT            FlowTimeout;                /* second */
	UINT            MaxActiveConn;
	UINT			MaxTokenSize;
	//UINT            FlowCheckPeriod;            /* millisecond */
	UINT            ResetCounter;
} FT_CONTROL_PROTOCOL;


typedef struct ft_query_flowset {
	UINT            qtype;          /* 1: sorted */
	UINT            nflow;          /* max conn to return */
} FT_QUERY_FLOWSET;

typedef struct ft_info_flow {
	UINT            type;           /* 0: periodical check, 1: new flow */
	//UINT            srcip;
	UINT            dstip;
	//USHORT          sport;
	//USHORT          dport;
	UCHAR           protocol;
	UINT            totalbytes;
	UINT            totalpkts;
	UINT			pktsamplerate;
	UINT			flowsamplerate;
	//UINT            incbytes;
	//UINT            incpkts;
	//UINT            inctime;
	//ULONG			portnum;
} FT_INFO_FLOW;

typedef struct ft_info_flowset {
	UINT            nflowmax;       /* max conn to return */
	UINT            nflow;          /* number of conns */
	FT_INFO_FLOW	flow[0];
} FT_INFO_FLOWSET;

typedef struct _FLOW_EVENT {
	FT_INFO_FLOWSET			*flowset;
	//ULONG                   BytesReturned;
	//OVERLAPPED              stOverlapped;
} FLOW_EVENT;





typedef struct ft_info_performance {
	UINT LastTimerUpdateTime;  //in millisecond
	UINT CurrentPPS;
	UINT CurrentMBS;
	UINT CurrentInboundPPS;
} FT_INFO_PERFORMANCE;




typedef struct _FLOW_LABEL
{
	UINT                saddr;
	UINT                daddr;
	USHORT              sport;
	USHORT              dport;
	UCHAR               protocol;
	//NDIS_PORT_NUMBER	portnum;
} FLOW_LABEL, *PFLOW_LABEL;


typedef struct _SAMPLE_RATE {
	UINT hash;
	FLOW_LABEL key;
	UINT psrate;
	UINT fsrate;
} FT_SAMPLE_RATE;

//typedef struct _SAMPLE_RATE_SET {
//	FT_SAMPLE_RATE sample_rate[0];
//} FT_SAMPLE_RATE_SET;

typedef struct _FT_SAMPLE_DEFAULT{
	UINT pkt_samp_default;
	UINT flow_samp_default;
} FT_SAMPLE_DEFAULT;


typedef struct _ft_control_sample_rate {
	UINT reason;   //0: change default; 1: change single flow
	//FT_SAMPLE_DEFAULT samp_default;
	UINT pkt_samp_default;
	UINT flow_samp_default;
	UINT nflow;
	FT_SAMPLE_RATE *sample_rate;
} FT_CONTROL_SAMPLE_RATE;


typedef struct _USER_PACKET
{
	UINT                len;
	//UINT                stype;      /* send hook point, outgoing only (UPSTYPE_XXX) */
	UINT                reason;     /* pkt or flow samples */
	//UINT				sample_type;

	//UCHAR               priority : 3;
	//UCHAR               unused : 5;
	//UCHAR               pad;
	//USHORT              vlanid;     /* vlanid */

	UCHAR               buf[0];
} USER_PACKET, *PUSER_PACKET;

typedef struct _QUERY_USER_PACKET_SET
{
	UINT                npktmax;    /* max number of pkts to return */
	UINT                buflen;     /* buf len */
} QUERY_USER_PACKET_SET, *PQUERY_USER_PACKET_SET;

typedef struct _USER_PACKET_SET
{
	UINT                npktmax;       /* max number of pkts to return */
	UINT                buflen;        /* buf len */
	UINT                npkt;          /* number of pkts */
	UCHAR               upkt[0];
} USER_PACKET_SET, *PUSER_PACKET_SET;


typedef UCHAR EthernetAddress[ETHERNET_ADDR_LENGTH];

typedef struct _ETH_HEADER {
	EthernetAddress Dest;
	EthernetAddress Source;
	USHORT          Type;
} ETH_HEADER;



typedef struct _IP_HEADER {
	UCHAR     verlen;     // Version and length
	UCHAR     tos;        // Type of service
	USHORT    length;     // Total datagram length
	USHORT    id;         // Identification
	USHORT    offset;     // Flags, fragment offset
	UCHAR     ttl;        // Time to live
	UCHAR     protocol;   // Protocol
	USHORT    xsum;       // Header checksum
	UINT      src;        // Source address
	UINT      dest;       // Destination address  
} IP_HEADER;

typedef struct _L4_HEADER {
	USHORT sport;
	USHORT dport;
} L4_HEADER;







#define FT_PACKET_SAMPLE	0x01
#define FT_FLOW_SAMPLE		0x02
#define FT_WITHOUT_SAMPLE	0x00

#define FT_SUCCESS                0
#define FT_FAIL                   1

#endif //__FILTERUSER_H__

