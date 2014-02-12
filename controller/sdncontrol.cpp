//#include "stdafx.h"

#include <winsock2.h>
//#include <ws2tcpip.h>
#include <stdio.h>
#include "sdncontrol.h"
#include <errno.h>
#include <map>
#include <queue>


#pragma comment(lib, "ws2_32")


#define FILEIO

#ifdef FILEIO
	FILE *ofp;
#endif

UINT GetNumRoutesbySock(SOCKET sock);


typedef enum cmd_result_e {
	CMD_OK = 0,            /* Command completed successfully */
	CMD_FAIL = -1,            /* Command failed */
	CMD_USAGE = -2,            /* Command failed, print usage  */
	CMD_NFND = -3,            /* Command not found */
	CMD_EXIT = -4,            /* Exit current shell level */
	CMD_INTR = -5,            /* Command interrupted */
	CMD_NOTIMPL = -6            /* Command not implemented */
} cmd_result_t;

struct{
	CRITICAL_SECTION lock;
	std::map<std::string, SOCKET> table;
} gIDSSocketTable;

struct{
	CRITICAL_SECTION lock;
	std::map<USHORT, SOCKET> table;
} gTGSocketTable;

struct {
	CRITICAL_SECTION lock;
	std::map<SOCKET, AGENT_CONTROL> table;
} gIDSStatus;

UINT  gCurrentTime; //in millisecond
UINT   gLast_Scale_Back_Time = 0;


#define TYPE_AGENT_IDS 0
#define TYPE_AGENT_TG  1





//#define ROUTING_BLOCK 1

#define MAX_MBUFCOUNT 64
#define COLLECTION_INTERVAL 1000  //collect states in every second
#define MITIGATION_INTERVAL 5000  //mitigation agent in every interval
#define SCALEBACK_INTERVAL 10000
//#define MAX_BUFF_LEN 1024
//#define FT_EPOCH 1000  //millisecond
#define SCALEBACK_INACTIVE_TIMEOUT 30000


std::map<UINT, UINT> g_Heavyhitter;
std::map<UINT, UINT> g_top_groundtruth;




bool hhsortFn(std::pair<UINT, UINT> first, std::pair<UINT, UINT> second)
{
	return (first.second > second.second);
}


unsigned int ip_to_int(const char * ip)
{
	/* The return value. */
	unsigned v = 0;
	/* The count of the number of bytes processed. */
	int i;
	/* A pointer to the next digit to process. */
	const char * start;

	start = ip;
	for (i = 0; i < 4; i++) {
		/* The digit being processed. */
		char c;
		/* The value of this byte. */
		int n = 0;
		while (1) {
			c = *start;
			start++;
			if (c >= '0' && c <= '9') {
				n *= 10;
				n += c - '0';
			}
			/* We insist on stopping at "." if we are still parsing
			the first, second, or third numbers. If we have reached
			the end of the numbers, we will allow any character. */
			else if ((i < 3 && c == '.') || i == 3) {
				break;
			}
			else {
				return 0;
			}
		}
		if (n >= 256) {
			return 0;
		}
		v *= 256;
		v += n;
	}
	return v;
}





typedef struct _opening_vm
{
	INT budget;
	UINT num_of_vm;
	UINT vm_mac;
	char name[128];
} OPENING_VM;

OPENING_VM g_opening_vm = {0,0,0, ""};


#define PKT_BASE_SAMPLE

#ifdef PKT_BASE_SAMPLE
UINT g_pkt_base_sample = 1;
UINT g_flow_base_sample = 0;
#else
UINT g_pkt_base_sample = 0;
UINT g_flow_base_sample = 1;
#endif




typedef struct _connect_para
{
	char* AgentAddr;
	USHORT port;
	UINT ConnectionTO;
	UINT type;
} connect_para;




int ft_strcasecmp(const char *s1, const char *s2)
{
	unsigned char c1, c2;

	do {
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
	} while (c1 == c2 && c1 != 0);

	return c1 - c2;
}



int start_sdn(int server);

HANDLE gTHandle;
HANDLE gTTGHandle;

#define MAX_START_VM_THREAD 32
static UINT gStartVMOverlay_idx = 0;
typedef struct _START_VM_OVERLAY
{
	HANDLE ThHandle;
	DWORD ThreadPID;
	UINT VM_Idx;
}
START_VM_OVERLAY;

START_VM_OVERLAY gStart_VM_Overlay[MAX_START_VM_THREAD];

std::queue<std::string> gVM_Pool;

std::map<UINT, std::set<UINT>> gVM_route_set;  //node_id, routes

//UINT tg_idx = 0;
#define MAX_NUM_TG (5+5)
TG_INFO gTG_list[] = 
{
	{ "204.57.0.30", 5100 },
	{ "204.57.0.30", 5101 },
	{ "204.57.0.30", 5102 },
	{ "204.57.0.30", 5103 },
	{ "204.57.0.30", 5104 },
	{ "204.57.0.30", 5105 },
	{ "204.57.0.30", 5106 },
	{ "204.57.0.30", 5107 },
	{ "204.57.0.30", 5108 },
	{ "204.57.0.30", 5109 },
	{ "204.57.0.30", 5110 },
	{ "204.57.0.30", 5111 },
	{ "204.57.0.30", 5112 },
	{ "204.57.0.30", 5113 },
	{ "204.57.0.30", 5114 },
	{ "204.57.0.30", 5115 },
	{ "204.57.0.30", 5116 },
	{ "204.57.0.30", 5117 },
	{ "204.57.0.30", 5118 },
	{ "204.57.0.30", 5119 },
	{ "204.57.0.30", 5120 },
	{ "204.57.0.30", 5121 },
	{ "204.57.0.30", 5122 },
	{ "204.57.0.30", 5123 }


	//{ "204.57.0.31", 5100 },
	//{ "204.57.0.31", 5101 },
	//{ "204.57.0.31", 5102 },
	//{ "204.57.0.31", 5103 },
	//{ "204.57.0.31", 5104 },
	//{ "204.57.0.31", 5105 },
	//{ "204.57.0.31", 5106 },
	//{ "204.57.0.31", 5107 },
	//{ "204.57.0.31", 5108 },
	//{ "204.57.0.31", 5109 },
	//{ "204.57.0.31", 5110 },
	//{ "204.57.0.31", 5111 },
	//{ "204.57.0.31", 5112 },
	//{ "204.57.0.31", 5113 },
	//{ "204.57.0.31", 5114 }
};



//UINT gVM_idx = 0;
//static UINT gVM_list[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
#define MAX_NUM_VM 15
IDS_AGENT_INFO gVM_list[] = 
{
	{ "204.57.0.33", "[datastore1] vm3/vm3.vmx", 3},
	{ "204.57.0.34", "[datastore1] vm4/vm3.vmx", 4 },
	{ "204.57.0.35", "[datastore1] vm5/vm3.vmx", 5 },
	{ "204.57.0.36", "[datastore1] vm6/vm3.vmx", 6 },
	{ "204.57.0.37", "[datastore1] vm7/vm3.vmx", 7 },
	{ "204.57.0.38", "[datastore1] vm8/vm3.vmx", 8 },
	{ "204.57.0.39", "[datastore1] vm9/vm3.vmx", 9 },
	{ "204.57.0.20", "[datastore1] vm10/vm3.vmx", 10 },
	{ "204.57.0.41", "[datastore1] vm11/vm3.vmx", 11 },
	{ "204.57.0.42", "[datastore1] vm12/vm3.vmx", 12 },
	{ "204.57.0.43", "[datastore1] vm13/vm3.vmx", 13 },
	{ "204.57.0.44", "[datastore1] vm14/vm3.vmx", 14},
	{ "204.57.0.45", "[datastore1] vm15/vm3.vmx", 15 },
	{ "204.57.0.46", "[datastore1] vm16/vm3.vmx", 16 },
	{ "204.57.0.47", "[datastore1] vm17/vm3.vmx", 17 }

};




typedef struct _tg_cmd 
{
	UINT time;
	UINT id;
	UINT start;
}TG_CMD;

UINT g_tg_idx = 0;
//UINT g_tg_replay_idx = 0;
TG_CMD g_tg_cmd[] =
{
	{ 2, 0, 1 },
	//{ 3, 1, 1 },
	////{10000, 2, 1},
	//{ 15, 1, 1 },
	//{ 16, 2, 1 },
	//{ 17, 3, 1 },
	//{ 18, 4, 1 },

	//{ 35, 1, 0 },
	//{ 36, 2, 0 },
	//{ 37, 3, 0 },
	//{ 38, 4, 0 },
	//{ 89, 5, 0 },
	//{ 90, 6, 0 },
	//{ 91, 7, 0 },
	//{ 92, 8, 0 },
	//{ 93, 9, 0 },

	//{ 145, 1, 0 },
	//{ 146, 2, 0 },
	//{ 147, 3, 0 },
	//{ 148, 4, 0 },

	//{ 10000, 1, 0 },




	
	{ 15, 1, 1 },
	{ 16, 2, 1 },
	{ 17, 3, 1 },
	{ 18, 4, 1 },
	{ 19, 5, 1 },
	{ 20, 6, 1 },
	{ 21, 7, 1 },
	{ 22, 8, 1 },
	{ 23, 9, 1 },


	//{ 24, 10, 1 },
	//{ 25, 11, 1 },
	//{ 26, 12, 1 },
	//{ 27, 13, 1 },
	//{ 28, 14, 1 },
	//{ 10000, 1, 0 },

	{ 85, 1, 0 },
	{ 86, 2, 0 },
	{ 87, 3, 0 },
	{ 88, 4, 0 },
	{ 89, 5, 0 },
	{ 80, 6, 0 },
	{ 81, 7, 0 },
	{ 82, 8, 0 },
	{ 83, 9, 0 },

	//{ 145, 1, 0 },
	//{ 146, 2, 0 },
	//{ 147, 3, 0 },
	//{ 148, 4, 0 },
	//{ 149, 5, 0 },
	//{ 150, 6, 0 },
	//{ 151, 7, 0 },
	//{ 152, 8, 0 },
	//{ 153, 9, 0 },

	{ 10000, 1, 0 },



	//10s
	{ 15, 1, 1 },
	{ 16, 2, 1 },
	{ 17, 3, 1 },
	{ 18, 4, 1 },
	{ 19, 5, 1 },
	{ 20, 6, 1 },
	{ 21, 7, 1 },
	{ 22, 8, 1 },
	{ 23, 9, 1 },

	{ 85, 1, 0 },
	{ 86, 2, 0 },
	{ 87, 3, 0 },
	{ 88, 4, 0 },
	{ 89, 5, 0 },
	{ 90, 6, 0 },
	{ 91, 7, 0 },
	{ 92, 8, 0 },
	{ 93, 9, 0 },


	{ 10000, 1, 0 },



	{ 34, 2, 0 },
	{ 35, 3, 0 },
	{ 36, 4, 0 },
	{ 37, 5, 0 },
	/*{ 15, 6, 1 },
	{ 16, 7, 1 },
	{ 17, 8, 1 },
	{ 18, 9, 1 },
	{ 19, 10, 1 },*/

	{ 10000, 1, 0 },


	//{ 38, 6, 1 },

	//{ 34, 6, 1 },
	//{ 35, 7, 1 },
	//{ 36, 8, 1 },
	//{ 37, 9, 1 },
	//{ 38, 10, 1 },

	//{ 119, 11, 1 },
	//{ 120, 12, 1 },
	//{ 131, 13, 1 },
	//{ 22, 14, 1 },
	//{ 23, 15, 1 },

	//{ 24, 16, 1 },
	//{ 25, 17, 1 },
	//{ 26, 18, 1 },
	//{ 27, 19, 1 },
	//{ 28, 20, 1 },

	//{ 19, 9, 1 },
	//{ 20, 10, 1 },
	//{ 21, 11, 1 },
	//{ 22, 12, 1 },
	{ 10000, 12, 1 },
	//{ 10, 1, 1 },
	//{ 11, 2, 1 },
	//{ 12, 3, 1 },
	//{ 13, 4, 1 },

	//{ 35, 1, 0 },
	//{ 36, 2, 0 },
	//{ 37, 3, 0 },
	//{ 38, 4, 0 },

	{ 100, 5, 1 },
	{ 101, 6, 1 },
	{ 102, 7, 1 },
	{ 103, 8, 1 },

	{ 115, 5, 0 },
	{ 116, 6, 0 },
	{ 117, 7, 0 },
	{ 118, 8, 0 },

	{ 180, 9, 1 },
	{ 181, 10, 1 },
	{ 182, 11, 1 },
	{ 183, 12, 1 },

	{ 195, 9, 0 },
	{ 196, 10, 0 },
	{ 197, 11, 0 },
	{ 198, 12, 0 },

	{ 310, 5, 1 }

	/*{ 5, 0, 1 },
	{ 10, 1, 1 },
	{ 30, 2, 1 },
	{ 40, 2, 1 },
	{ 50, 3, 1 },
	{ 60, 3, 1 },
	{ 70, 4, 1 },
	{ 80, 4, 1 },
	{ 90, 5, 1 },
	{ 100, 5, 1 },
	{ 110, 10, 1 },
	{ 190, 11, 1 },
	{ 210, 12, 1 },
	{ 230, 13, 1 },
	{ 240, 14, 1 }*/
};


//static char *gVM_name_list[]=
//{
//	"204.57.0.33",
//	"204.57.0.34",
//	"204.57.0.35",
//	"204.57.0.36",
//	"204.57.0.37",
//	"204.57.0.38",
//	"204.57.0.39",
//	"204.57.0.40",
//	"204.57.0.41",
//	"204.57.0.42"
//};
//static char *gVM_path[] =
//{
//	"[datastore1] vm3/vm3.vmx",
//	"[datastore1] vm4/vm3.vmx",
//	"[datastore1] vm5/vm3.vmx",
//	"[datastore1] vm6/vm3.vmx",
//	"[datastore1] vm7/vm3.vmx",
//	"[datastore1] vm8/vm3.vmx",
//	"[datastore1] vm9/vm3.vmx",
//	"[datastore1] vm10/vm3.vmx",
//	"[datastore1] vm11/vm3.vmx",
//	"[datastore1] vm12/vm3.vmx"
//};



#define NUM_STANDBY_VM 3
SOCKET g_standby_sock[NUM_STANDBY_VM];
SOCKET g_delete_sock[MAX_NUM_VM];


BOOL g_force_back = 0;
BOOL g_self_delete = 0;





//std::map<std::string, IDS_AGENT_INFO> gVM_pool = 
//{
//	std::pair<std::string, IDS_AGENT_INFO>("204.57.0.33", { "vm3/vm3.vmx", 3}),
//	std::pair<std::string, IDS_AGENT_INFO>("204.57.0.33", { "vm3/vm3.vmx", 3}),
//};



std::string GetNamebySock(SOCKET sock)
{
	std::string str;
	std::map<std::string, SOCKET>::iterator kt;
	BOOL find = 0;
	for (kt = gIDSSocketTable.table.begin(); kt != gIDSSocketTable.table.end(); ++kt)
	{
		if (kt->second == sock)
		{
			str = kt->first;
			find = 1;
			break;
		}
	}
	if (find == 0)
		return "";
	else
		return str;
}

UINT GetMacIdxbyName(std::string name)
{
	//std::string str;
	//std::map<std::string, SOCKET>::iterator kt;
	UINT i;
	UINT idx;
	for (i = 0; i < MAX_NUM_VM; ++i)
	{
		if (name == std::string(gVM_list[i].name))
		{
			idx = gVM_list[i].MACidx;
			break;
		}

	}
	return idx;
}


UINT GetMacIdxbySock(SOCKET sock)
{
	//std::string str;
	//std::map<std::string, SOCKET>::iterator kt;
	UINT i;
	UINT idx = (UINT)-1;
	std::string str = GetNamebySock(sock);
	if (str == "")
	{
		printf("cannot find Macidx\n");
		return (UINT)-1;
	}

	for (i = 0; i < MAX_NUM_VM; ++i)
	{
		if (str == std::string(gVM_list[i].name))
		{
			idx = gVM_list[i].MACidx;
			break;
		}

	}
	return idx;
}


UINT GetPortbySock(SOCKET sock)
{
	//std::string str;
	std::map<USHORT, SOCKET>::iterator it;
	//UINT i;
	UINT port = (UINT)-1;
	
	for (it = gTGSocketTable.table.begin(); it != gTGSocketTable.table.end(); ++it)
	{
		if (it->second == sock)
		{
			port = it->first;
			break;
		}

	}
	return port;
}
UINT GetNumRoutesbySock(SOCKET sock)
{
	UINT macidx = GetMacIdxbySock(sock);
	std::map<UINT, std::set<UINT>>::iterator it;
	std::set<UINT>::iterator jt;
	UINT count;

	it = gVM_route_set.find(macidx);
	if (it != gVM_route_set.end())
	{
		count = (UINT)it->second.size();
	}

	return count;
}


BOOL HasRoute(SOCKET sock, UINT daddr)
{
	std::map<UINT, std::set<UINT>>::iterator it =
		gVM_route_set.find(GetMacIdxbySock(sock));

	if (it == gVM_route_set.end())
		return false;

	std::set<UINT>::iterator jt = it->second.find(daddr);
	if (jt == it->second.end())
		return false;
	else
		return true;
}


int main(int argc, char **argv) {

	WSADATA wsaData;
	int rc = WSAStartup(0x202, &wsaData);
	if (rc == SOCKET_ERROR)
	{
		printf("socket error\n");
		return -1;
	}
	
	InitializeCriticalSection(&gIDSSocketTable.lock);
	InitializeCriticalSection(&gTGSocketTable.lock);
	InitializeCriticalSection(&gIDSStatus.lock);
	gIDSSocketTable.table.clear();
	gTGSocketTable.table.clear();
	gIDSStatus.table.clear();
	//ghMutex = CreateMutex(NULL, false, NULL);

	//if (ghMutex == NULL)
	//{
	//	printf("createMutex error: %d\n", GetLastError());
	//}

	/*if (strcmp(argv[1],"s") == 0)
		start_sdn(0);
	else
		start_sdn(1);*/

	start_sdn(0);


	return CMD_OK;
}


//
//DWORD WINAPI StartServer(LPVOID paramPtr)
//{
//	int mSock;
//	int rc;
//	int addrLen, connectedSock;
//	sockaddr_in clientAddr;
//	sockaddr_in serveraddr;
//	DWORD ThreadID;
//	HANDLE hThread;
//
//	UNREFERENCED_PARAMETER(paramPtr);
//
//
//	printf("server start\n");
//
//	mSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (mSock == INVALID_SOCKET)
//	{
//		printf("invalid socket\n");
//		return -1;
//	}
//	serveraddr.sin_family = AF_INET;
//	serveraddr.sin_port = htons(CONTROL_PORT);
//	serveraddr.sin_addr.s_addr = INADDR_ANY;
//	rc = bind(mSock, (sockaddr *)&serveraddr, sizeof(sockaddr));
//	if (mSock == INVALID_SOCKET)
//	{
//		printf("bind error\n");
//		return -1;
//	}
//	rc = listen(mSock, 5);
//	if (mSock == INVALID_SOCKET)
//	{
//		printf("listen error\n");
//		return -1;
//	}
//
//	while (true) {
//		// accept a connection
//		addrLen = sizeof(clientAddr);
//		connectedSock = accept(mSock, (struct sockaddr*) &clientAddr, &addrLen);
//
//		// handle accept being interupted
//		if (connectedSock == INVALID_SOCKET  &&  errno == EINTR) {
//			//printf("Socket Accept: INVALID_SOCKET\n");
//			continue;
//		}
//		//printf();
//
//		hThread = CreateThread(NULL, 0, StartAgent, LPVOID(&connectedSock), 0, &ThreadID);
//
//		//return connectedSock;
//	}
//	return 0;
//}
//





void CreateAgentStatus(std::string ids_ip, SOCKET sock)
{
	//std::map<std::string, SOCKET>::iterator jt;

	//std::string ids_ip = it->first;
	//SOCKET sock = it->second;

	AGENT_CONTROL new_agent;

	memset(&new_agent.agent_status, 0, sizeof(new_agent.agent_status));
	new_agent.noticetable.clear();

	int b = (int)ids_ip.find_last_of(".");
	std::string last_tuple = std::string(ids_ip, b + 1, ids_ip.size());
	new_agent.agent_status.MACidx = atoi(const_cast<char *>(last_tuple.c_str())) - 30;
	//new_agent.agent_status.sock = sock;

	if (gIDSStatus.table.empty())
	{
		//first node, add routes
		//printf("first node\n");
		//new_agent.agent_status.routes = (UINT)pow(2, 17);
	}

	EnterCriticalSection(&gIDSStatus.lock);
	gIDSStatus.table.insert(std::pair<SOCKET, AGENT_CONTROL>(sock, new_agent));
	LeaveCriticalSection(&gIDSStatus.lock);

	//pay own budget
	if (g_opening_vm.num_of_vm > 0)
	{
		g_opening_vm.num_of_vm -= 1;
		if (g_opening_vm.num_of_vm == 0)
			g_opening_vm.budget = 0;
	}
}



DWORD DoConnectAgent(connect_para* conn_para)
{
	SOCKET mSock;
	int rc;

	sockaddr_in serveraddr;
	//char *cmd;
	//std::map<std::string, SOCKET>::iterator it;
	//char* next_token = NULL;
	//char* token = " ";
	//char* command = NULL;

	//command = strtok_s(cmd, token, &next_token);

	

	//std::string str(cmd);

	//char * cstr = new char[conn_para->AgentAddr.length() + 1];
	//strcpy_s(cstr, conn_para->AgentAddr.length() + 1,conn_para->AgentAddr.c_str());

	//printf("connecting to %s\n", conn_para->AgentAddr);

	if (conn_para->type == TYPE_AGENT_IDS)
	{
		if (gIDSSocketTable.table.find(conn_para->AgentAddr) != gIDSSocketTable.table.end())
		{
			printf("already connected to that agent\n");
			return CMD_NOTIMPL;
		}
	}
	else
	{
		if (gTGSocketTable.table.find(conn_para->port) != gTGSocketTable.table.end())
		{
			printf("already connected to that agent\n");
			return CMD_NOTIMPL;
		}
	}

	mSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (mSock == INVALID_SOCKET)
	{
		printf("invalid socket: %d\n", GetLastError());
		return CMD_FAIL;
	}
	

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(conn_para->AgentAddr);
	//if (conn_para->type == TYPE_AGENT_IDS)
	//	serveraddr.sin_port = htons(CONTROL_PORT);
	//else
	//	serveraddr.sin_port = htons(TG_PORT);
	serveraddr.sin_port = htons(conn_para->port);


	UINT i = 0;
	do
	{

		rc = connect(mSock, (sockaddr *)&serveraddr, sizeof(serveraddr));
		if (rc == SOCKET_ERROR)
		{		
			//return CMD_FAIL;
			printf("connection fail with error = %d\n", GetLastError());
			i += 1;
			if (i >= conn_para->ConnectionTO)
			{
				printf("connect timeout with error = %d\n", GetLastError());
				closesocket(mSock);
				return CMD_FAIL;
			}
		}
		else
			break;
		
		//Sleep(1000);

	} while (true);

	std::string str(conn_para->AgentAddr);
	USHORT port = conn_para->port;

	if (conn_para->type == TYPE_AGENT_IDS)
	{
		EnterCriticalSection(&gIDSSocketTable.lock);
		gIDSSocketTable.table.insert(std::pair<std::string, SOCKET>(str, mSock));
		LeaveCriticalSection(&gIDSSocketTable.lock);
		CreateAgentStatus(str, mSock);
	}
	else
	{
		EnterCriticalSection(&gTGSocketTable.lock);
		gTGSocketTable.table.insert(std::pair<USHORT, SOCKET>(port, mSock));
		LeaveCriticalSection(&gTGSocketTable.lock);

	}
	
	printf("server connected and log status\n");

	return CMD_OK;
}



//DWORD WINAPI ConnectToAgent(LPVOID paramPtr)
//{
//	connect_para *conn_para = (connect_para*)paramPtr;
//	DWORD rc = DoConnectAgent(conn_para);
//
//	return rc;
//}



//cmd_result_t ConnectAgenet(char* AgenetAddr, int type)
//{
//	//DWORD ThreadID;
//	connect_para conn_para;
//	//std::string str(AgenetAddr);
//	conn_para.AgentAddr = AgenetAddr;
//	conn_para.ConnectionTO = 3;  //try 3 times
//	conn_para.type = type;
//
	//printf("IDS %s\n", conn_para.AgentAddr);

	//gTHandle = CreateThread(NULL, 0, ConnectToAgent, (LPVOID)&conn_para, 0, &ThreadID);

	//if (gTHandle == NULL)
	//{

	//	printf("create thread fail\n");
	//	//return -1;
	//	return CMD_FAIL;
	//}
	//DoConnectAgent();



//	return CMD_OK;
//}


DWORD StartVM(char *path)
{
	INT rc;
	if (!system(NULL))
		return CMD_FAIL;

	char command[1024];
	command[0] = 0;
	char * cmd = "\"C:\\vmware-cmd.pl -U root -P uscnsl224 -H 204.57.0.95 \"";
	char *cmd1 = "\" start soft\"";
	strcat_s(command, cmd);
	if (path == NULL)
	{
		printf("start vm: NULL path\n");
	}
	strcat_s(command, path);
	strcat_s(command, cmd1);


#ifdef FILEIO
	fprintf(ofp, "cmd: %s\n", command);
#endif

	printf("cmd: %s\n", command);
	
	rc = system(command);

	if (rc == 0){
		printf("vm started\n");
		return CMD_OK;
	}
	else {
		printf("start vm fail");
		return CMD_FAIL;
	}
}


DWORD WINAPI StopVM(LPVOID argu)
{
	INT rc;
	if (!system(NULL))
		return CMD_FAIL;

	char *path = (char *)argu;

	char command[1024];
	command[0] = 0;
	char * cmd = "\"C:\\vmware-cmd.pl -U root -P uscnsl224 -H 204.57.0.94 \"";
	char *cmd1 = "\" suspend soft\"";
	strcat_s(command, cmd);
	if (path == NULL)
	{
		printf("start vm: NULL path\n");
	}
	strcat_s(command, path);
	strcat_s(command, cmd1);

	printf("cmd: %s\n", command);

	rc = system(command);

	if (rc == 0){
		printf("vm stopped\n");
		return CMD_OK;
	}
	else {
		printf("stop vm fail");
		return CMD_FAIL;
	}
}

DWORD StopAgent(SOCKET sock)
{
	std::map<std::string, SOCKET>::iterator fit;
	std::string idsname;
	EnterCriticalSection(&gIDSSocketTable.lock);
	for (fit = gIDSSocketTable.table.begin(); fit != gIDSSocketTable.table.end();)
	{
		if (fit->second == sock)
		{
			idsname = fit->first;
			gIDSSocketTable.table.erase(fit++);
			break;
		}
		else
			++fit;
	}
	LeaveCriticalSection(&gIDSSocketTable.lock);



	UINT vmidx = 0;
	for (int j = 0; j < MAX_NUM_VM; ++j)
	{
		if (std::string(gVM_list[j].name) == idsname)
		{
			vmidx = j;
			gVM_Pool.push(idsname);
			break;
		}
	}
	printf("id: %d, idsname = %s\n", vmidx, idsname);

	gStart_VM_Overlay[gStartVMOverlay_idx].VM_Idx = vmidx;
	gStart_VM_Overlay[gStartVMOverlay_idx].ThHandle = CreateThread(NULL, 0, StopVM,
		(LPVOID)gVM_list[vmidx].path, 0, &gStart_VM_Overlay[gStartVMOverlay_idx].ThreadPID);

	if (gStart_VM_Overlay[gStartVMOverlay_idx].ThHandle == NULL)
	{
		printf("create thread fail\n");
		//return -1;
		return CMD_FAIL;
	}
	gStartVMOverlay_idx = (gStartVMOverlay_idx + 1) % MAX_START_VM_THREAD;

	return CMD_OK;
}


DWORD WINAPI ReconnectAgent(LPVOID paramPtr)
{
	connect_para conn_para;
	char *ids_name = (char *)paramPtr;
	//std::string str(AgenetAddr);
	conn_para.AgentAddr = ids_name;
	conn_para.ConnectionTO = 3;  //30 sec
	conn_para.type = TYPE_AGENT_IDS;
	//ConnectAgenet(cmd);
	conn_para.port = CONTROL_PORT;
	if (DoConnectAgent(&conn_para) != CMD_OK)
		printf("reconnect fail\n");
	else
		printf("reconnect again\n");

	return CMD_OK;
}




DWORD WINAPI StartAgent(LPVOID paramPtr)
//DWORD StartAgent(UINT idx)
{
	WSABUF CmdBuf;
	//SOCKET mSock;
	std::map<std::string, SOCKET>::iterator it;

	CmdBuf.buf = new char[MAXBUFLEN];
	//CmdBuf.len = MAXBUFLEN;
	//CmdBuf.buf[0] = 's';

	//UINT vmidx = gVM_idx;

	UINT vmidx = *(UINT *)paramPtr;
	//char *cmd = (char *)paramPtr;
	char *cmd = gVM_list[vmidx].name;

	std::string str(cmd);

	DWORD currLen;
	DWORD Flags = 0;
	//char word[10];

	if (vmidx >= NUM_STANDBY_VM)
	{
		StartVM(gVM_list[vmidx].path);
	}


	if (gIDSSocketTable.table.find(str) == gIDSSocketTable.table.end())
	{
		//printf("havent connected\n");
		connect_para conn_para;
		//std::string str(AgenetAddr);
		conn_para.AgentAddr = cmd;
		conn_para.ConnectionTO = 3;  //30 sec
		conn_para.type = TYPE_AGENT_IDS;
		//ConnectAgenet(cmd);
		conn_para.port = CONTROL_PORT;
		DoConnectAgent(&conn_para);

	}

	it = gIDSSocketTable.table.find(str);
	if (it == gIDSSocketTable.table.end()) {
		printf("Start Agent Fail\n");
		return CMD_FAIL;
	}
	else
	{
		if (vmidx <= NUM_STANDBY_VM)
			g_standby_sock[vmidx] = it->second;
	}



	do
	{
		//gets_s(CmdBuf.buf, MAXBUFLEN);
		CmdBuf.buf = "startvm";
		CmdBuf.len = (ULONG)strlen(CmdBuf.buf);

		//printf("send %s with size %d\n", CmdBuf.buf, CmdBuf.len);

		INT rc = WSASend(it->second, &CmdBuf, 1, &currLen, Flags, NULL, NULL);

		if (rc == SOCKET_ERROR)
		{
			//WARN_errno(true, "recv");
			printf("send error = %d\n", GetLastError());
			//break;
			closesocket(it->second);
			return CMD_FAIL;
		}

	} while (0);

	return CMD_OK;
}


DWORD DoConnectTrafficGenerator(char *cmd, USHORT port)
{
	std::map<USHORT, SOCKET>::iterator it;
	if (gTGSocketTable.table.find(port) == gTGSocketTable.table.end())
	{
		//printf("havent connected\n");
		connect_para conn_para;
		//std::string str(AgenetAddr);
		conn_para.AgentAddr = cmd;
		conn_para.ConnectionTO = 3;  //30 sec
		conn_para.type = TYPE_AGENT_TG;
		conn_para.port = port;
		//ConnectAgenet(cmd);
		printf("Connect TG %s on port %d\n", conn_para.AgentAddr, conn_para.port);
		DoConnectAgent(&conn_para);
	}



	return CMD_OK;
	
}




DWORD DoStartTrafficGenerator(char *cmd, USHORT port)
{

	//UNREFERENCED_PARAMETER(paramPtr);

	WSABUF CmdBuf;
	//SOCKET mSock;
	std::map<USHORT, SOCKET>::iterator it;

	CmdBuf.buf = new char[ROUTING_MAXBUFLEN];
	if (CmdBuf.buf == NULL)
	{
		printf("allocate buffer fail\n");
		return CMD_FAIL;
	}
	memset(CmdBuf.buf, 0, ROUTING_MAXBUFLEN);
	
	//CmdBuf.len = MAXBUFLEN;
	//CmdBuf.buf[0] = 's';

	//char *cmd = (char *)paramPtr;
	std::string str(cmd);

	DWORD currLen;
	DWORD Flags = 0;

	if (gTGSocketTable.table.find(port) == gTGSocketTable.table.end())
	{
		//printf("havent connected\n");
		connect_para conn_para;
		//std::string str(AgenetAddr);
		conn_para.AgentAddr = cmd;
		conn_para.ConnectionTO = 3;  //30 sec
		conn_para.type = TYPE_AGENT_TG;
		conn_para.port = port;
		//ConnectAgenet(cmd);
		printf("TG %s on port %d\n", conn_para.AgentAddr, conn_para.port);

		DoConnectAgent(&conn_para);

	}

	it = gTGSocketTable.table.find(port);

	if (it == gTGSocketTable.table.end())
	{
		printf("cannot connect to TG\n");
		return CMD_FAIL;
	}


	do
	{
		//gets_s(CmdBuf.buf, MAXBUFLEN);
		//CmdBuf.buf = "tgctrl 7|starttg";
		//strcat_s(CmdBuf.buf, ROUTING_MAXBUFLEN,"starttg");
		//CmdBuf.len = (ULONG)strlen(CmdBuf.buf);
		//CmdBuf.len = ROUTING_MAXBUFLEN;
		//printf("send %s with size %d\n", CmdBuf.buf, CmdBuf.len);

		strcat_s(CmdBuf.buf, ROUTING_MAXBUFLEN, "starttg");
		//CmdBuf.len = ROUTING_MAXBUFLEN;

		strcat_s(CmdBuf.buf, ROUTING_MAXBUFLEN, " ");
		INT curr = (INT)strnlen(CmdBuf.buf, ROUTING_MAXBUFLEN);
		memset(&CmdBuf.buf[curr], '0', ROUTING_MAXBUFLEN - curr);
		CmdBuf.buf[ROUTING_MAXBUFLEN - 1] = '\0';
		CmdBuf.len = ROUTING_MAXBUFLEN;


		INT rc = WSASend(it->second, &CmdBuf, 1, &currLen, Flags, NULL, NULL);

		if (rc == SOCKET_ERROR)
		{
			//WARN_errno(true, "recv");
			printf("send error = %d\n", GetLastError());
			//break;		
			closesocket(it->second);
			EnterCriticalSection(&gTGSocketTable.lock);
			gTGSocketTable.table.erase(it);
			LeaveCriticalSection(&gTGSocketTable.lock);

			//it->second = INVALID_SOCKET;
			return CMD_FAIL;
		}

		//printf("cmd sent\n");
		//break;

	} while (0);

	return CMD_OK;

}



DWORD DoStopTrafficGenerator(char *cmd, USHORT port)
{

	//UNREFERENCED_PARAMETER(paramPtr);

	WSABUF CmdBuf;
	//SOCKET mSock;
	std::map<USHORT, SOCKET>::iterator it;

	CmdBuf.buf = new char[ROUTING_MAXBUFLEN];
	memset(CmdBuf.buf, 0, ROUTING_MAXBUFLEN);

	//CmdBuf.len = MAXBUFLEN;
	//CmdBuf.buf[0] = 's';

	//char *cmd = (char *)paramPtr;
	std::string str(cmd);

	DWORD currLen;
	DWORD Flags = 0;

	if (gTGSocketTable.table.find(port) == gTGSocketTable.table.end())
	{
		//printf("havent connected\n");
		connect_para conn_para;
		//std::string str(AgenetAddr);
		conn_para.AgentAddr = cmd;
		conn_para.ConnectionTO = 3;  //30 sec
		conn_para.type = TYPE_AGENT_TG;
		conn_para.port = port;
		//ConnectAgenet(cmd);
		DoConnectAgent(&conn_para);

	}

	it = gTGSocketTable.table.find(port);

	if (it == gTGSocketTable.table.end())
	{
		printf("stop TG cannot find\n");
		return CMD_FAIL;
	}
	


	do
	{
		//gets_s(CmdBuf.buf, MAXBUFLEN);
		//CmdBuf.buf = "tgctrl 6|stoptg";
		//CmdBuf.len = (ULONG)strlen(CmdBuf.buf);
		//CmdBuf.buf[0] = 0;
		strcat_s(CmdBuf.buf, ROUTING_MAXBUFLEN, "stoptg");
		//CmdBuf.len = ROUTING_MAXBUFLEN;

		strcat_s(CmdBuf.buf, ROUTING_MAXBUFLEN, " ");
		INT curr = (INT)strnlen(CmdBuf.buf, ROUTING_MAXBUFLEN);
		memset(&CmdBuf.buf[curr], '0', ROUTING_MAXBUFLEN - curr);
		CmdBuf.buf[ROUTING_MAXBUFLEN - 1] = '\0';
		CmdBuf.len = ROUTING_MAXBUFLEN;


		//printf("send %s with size %d\n", CmdBuf.buf, CmdBuf.len);

		INT rc = WSASend(it->second, &CmdBuf, 1, &currLen, Flags, NULL, NULL);

		if (rc == SOCKET_ERROR)
		{
			//WARN_errno(true, "recv");
			printf("send error = %d\n", GetLastError());
			closesocket(it->second);
			EnterCriticalSection(&gTGSocketTable.lock);
			gTGSocketTable.table.erase(it);
			LeaveCriticalSection(&gTGSocketTable.lock);

			return CMD_FAIL;
		}

		//printf("cmd sent\n");
		//break;

	} while (0);

	return CMD_OK;

}

DWORD WINAPI DoStateRequest(LPVOID paramPtr)
{
	std::map<std::string, SOCKET>::iterator it;
	std::map<SOCKET, AGENT_CONTROL>::iterator it_control;
	WSAOVERLAPPED olp[MAX_MBUFCOUNT];
	WSAEVENT eArray[MAX_MBUFCOUNT];
	WSABUF mBuf[MAX_MBUFCOUNT];
	char *CmdBuf = "getstate";
	UINT Cmdlen = (UINT)strnlen(CmdBuf, MAX_MBUFCOUNT);
	INT i;
	INT err;
	INT rc;
	INT total_agent;
	DWORD currLen;
	DWORD Flags;
	SOCKET socklist[MAX_MBUFCOUNT];

	LARGE_INTEGER frequency;
	LARGE_INTEGER start;
	LARGE_INTEGER next;
	LARGE_INTEGER stop;
	LARGE_INTEGER lap;

	double  elap;

	INT control_loop;

	HANDLE hreconn;
	DWORD pidreconn;

	char *ids_name_reconn;
	std::string ids_name;
	//init

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);

	next.QuadPart = start.QuadPart + COLLECTION_INTERVAL * frequency.QuadPart / 1000;
	control_loop = 1;

	for (i = 0; i < MAX_MBUFCOUNT; ++i)
	{
		eArray[i] = WSACreateEvent();
		if (eArray[i] == NULL)
			printf("WSACreateEvent fail\n");

		ZeroMemory(&olp[i], sizeof(WSAOVERLAPPED));

		olp[i].hEvent = eArray[i];
	}


	do {
		
		QueryPerformanceCounter(&stop);
		while (stop.QuadPart < next.QuadPart)
		{
			lap.QuadPart = next.QuadPart - stop.QuadPart;
			elap = (double)lap.QuadPart / (double)frequency.QuadPart;

			Sleep((DWORD)(elap * 1000));

			QueryPerformanceCounter(&stop);
		}
		next.QuadPart = start.QuadPart + (control_loop + 1) * COLLECTION_INTERVAL * frequency.QuadPart / 1000;
		++control_loop;

		//printf("timer set\n");

		
		EnterCriticalSection(&gIDSSocketTable.lock);

		i = 0;
		for (it = gIDSSocketTable.table.begin(); it != gIDSSocketTable.table.end(); ++it)
		{
			BOOL deleting = FALSE;
			for (int j = 0; j < MAX_NUM_VM; ++j)
			{
				if (it->second == g_delete_sock[j])
				{
					deleting = TRUE;
					break;
				}
			}
			if (deleting == TRUE)
				continue;

			socklist[i] = it->second;
			++i;
		}
		total_agent = i;
		LeaveCriticalSection(&gIDSSocketTable.lock);
		
		if (total_agent == 0)
			continue;

		for (i = 0; i < total_agent; ++i)
		{
			//printf("send one command\n");
			mBuf[i].buf = CmdBuf;
			mBuf[i].len = Cmdlen;
			rc = WSASend(socklist[i], &mBuf[i], 1, NULL, 0, &olp[i], NULL);
			if ((rc == SOCKET_ERROR) &&
				(WSA_IO_PENDING != (err = WSAGetLastError()))) 
			{
				//WARN_errno(true, "WSASend");
				printf("Request State: WSASend fail = %d\n", WSAGetLastError());

				EnterCriticalSection(&gIDSSocketTable.lock);
				ids_name = GetNamebySock(socklist[i]);
				gIDSSocketTable.table.erase(GetNamebySock(socklist[i]));
				LeaveCriticalSection(&gIDSSocketTable.lock);
				
				EnterCriticalSection(&gIDSStatus.lock);
				gIDSStatus.table.erase(socklist[i]);
				LeaveCriticalSection(&gIDSStatus.lock);

				//reconnect again
				ids_name_reconn = const_cast<char *>(ids_name.c_str());
				hreconn = CreateThread(NULL, 0, ReconnectAgent, (LPVOID)ids_name_reconn, 0, &pidreconn);
				if (hreconn == NULL)
				{
					printf("create reconnect thread fail\n");
					//return -1;
					//return CMD_FAIL;
				}
				continue;
			}			
		}

		
		



		rc = WSAWaitForMultipleEvents(total_agent, eArray, TRUE, COLLECTION_INTERVAL, FALSE);
		if (rc == WSA_WAIT_FAILED) 
		{
			printf("WSAWaitForMultipleEvents fail = %d\n", WSAGetLastError());
			continue;
		}

		//printf("WSAWaitForMultipleEvents\n");
		for (i = 0; i < total_agent; ++i)
		{
			rc = WSAGetOverlappedResult(socklist[i], &olp[i], &currLen, FALSE, &Flags);
			if (rc == FALSE && (WSA_IO_INCOMPLETE != (err = WSAGetLastError())))
			{
				printf("WSAGetOverlappedResult = %d\n", WSAGetLastError());
				continue;
			}

			//printf("Send command: %s, %d Bytes\n", CmdBuf, currLen);

			rc = WSAResetEvent(olp[i].hEvent);
			if (rc == FALSE)
			{
				printf("WSAResetEvent = %d\n", WSAGetLastError());
			}
		}

	} while (true);

	
}


void ParseState(char *recv, SOCKET sock)
{
	char *token = " ";
	char *token2 = ";";
	char *next_token;
	char *command;
	char *cmd2;
	char *rest;
	int i = 0;
	std::map<std::string, SOCKET>::iterator jt;
	std::map<SOCKET, AGENT_CONTROL>::iterator it;

	

	it = gIDSStatus.table.find(sock);
	
	if (it == gIDSStatus.table.end())
	{
		printf("no status\n");
		return;
	}

	if (strnlen_s(recv, 100000) > MAXBUFLEN)
		//too long
		return;
	it->second.agent_status.status[0] = '\0';

	strcat_s(it->second.agent_status.status, MAXBUFLEN, recv);

	if (it->second.agent_status.status[0] == '\0')
		return;
	else if (strnlen_s(it->second.agent_status.status, MAXBUFLEN) != MAXBUFLEN - 1)
		return;

	command = strtok_s(it->second.agent_status.status, token, &next_token);

	//printf("parse the result\n");
	if (command == NULL)
		return;

	do
	{
		if (!ft_strcasecmp(command, "cpuuser"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			it->second.agent_status.user_cpu_usage = val;
		}
		else if(!ft_strcasecmp(command, "cpuker"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			it->second.agent_status.kernel_cpu_usage = val;
		}
		else if (!ft_strcasecmp(command, "cputotal"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			it->second.agent_status.total_cpu_usage = val;
			//it->second.agent_status.smooth_cpu_usage = (UINT)(it->second.agent_status.smooth_cpu_usage * 0.8 + val * 0.2);

		}
		else if (!ft_strcasecmp(command, "cpusmooth"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			it->second.agent_status.smooth_cpu_usage = val;

			//if (val >= CPU_HIGH_WATERMARK)
			//{
			//	//number of flow needed to migrate (extrapolcated)
			//	it->second.agent_status.overload = (val - CPU_HIGH_WATERMARK)
			//		* SAMPLE_IN_HIGH_WATERMARK * SAMPLE_RATE / val;
			//}
			//else
			//	it->second.agent_status.overload = 0;
			////printf("overload = %d\n", it->second.agent_status.overload);

			//if (val < CPU_LOW_WATERMARK)
			//{
			//	it->second.agent_status.budget = (CPU_LOW_WATERMARK - val)
			//		* SAMPLE_IN_LOW_WATERMARK * SAMPLE_RATE / CPU_LOW_WATERMARK;
			//}
			//else 
			//	it->second.agent_status.budget = 0;

			//it->second.agent_status.overhead = val * SAMPLE_IN_LOW_WATERMARK
			//	* SAMPLE_RATE / CPU_LOW_WATERMARK;
			

												
		}
		else if (!ft_strcasecmp(command, "mem"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			it->second.agent_status.mem_usage = val;
		}
		else if (!ft_strcasecmp(command, "pps"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			it->second.agent_status.PPS = val;
		}
		else if (!ft_strcasecmp(command, "mbs"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			//printf("receive mbs %d\n", val);
			it->second.agent_status.MBS = val;
		}
		else if (!ft_strcasecmp(command, "spps"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			it->second.agent_status.samplepps = val;
		}
		else if (!ft_strcasecmp(command, "smbs"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			//printf("receive mbs %d\n", val);
			it->second.agent_status.samplembs = val;
		}
		else if (!ft_strcasecmp(command, "conn"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			//printf("receive mbs %d\n", val);
			it->second.agent_status.conn = val;
		}
		else if (!ft_strcasecmp(command, "flow"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			//printf("receive mbs %d\n", val);
			it->second.agent_status.flow = val;
		}
		else if (!ft_strcasecmp(command, "sum"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			UINT val = atoi(command);
			it->second.agent_status.totalsamples = val;

			UINT smoothcpu = it->second.agent_status.smooth_cpu_usage;

			if (smoothcpu >= CPU_HIGH_WATERMARK)
			{
				//number of flow needed to migrate (extrapolcated)
				it->second.agent_status.overload = (smoothcpu - CPU_HIGH_WATERMARK)
					* val * SAMPLE_RATE / smoothcpu;
			}
			else
				it->second.agent_status.overload = 0;
			//printf("overload = %d\n", it->second.agent_status.overload);

			if (smoothcpu == 0)
			{
				it->second.agent_status.budget = SAMPLE_IN_LOW_WATERMARK * SAMPLE_RATE;
			}
			else if (smoothcpu < CPU_LOW_WATERMARK)
			{
				it->second.agent_status.budget = max( ((CPU_LOW_WATERMARK - smoothcpu)* val * SAMPLE_RATE / smoothcpu), 
					((CPU_LOW_WATERMARK - smoothcpu) * SAMPLE_IN_LOW_WATERMARK * SAMPLE_RATE / CPU_LOW_WATERMARK));
			}
			else 
				it->second.agent_status.budget = 0;

			it->second.agent_status.overhead = max( (val * SAMPLE_RATE),
				(smoothcpu * SAMPLE_IN_LOW_WATERMARK * SAMPLE_RATE / CPU_LOW_WATERMARK) );

		}
		else if (!ft_strcasecmp(command, "ktop"))
		{
			rest = next_token;
			command = strtok_s(rest, token, &next_token);
			INT val = atoi(command);
			//printf("receive mbs %d\n", val);
			it->second.agent_status.topktable.K = val;
			it->second.agent_status.topktable.idx = 0;
		}
		else if (!ft_strcasecmp(command, "top"))
		{
			INT val;
			rest = next_token;
			command = strtok_s(rest, token, &next_token);   //rest = xx;xx;xxx  next_token = "other"

			command = strtok_s(command, token2, &cmd2);      
			UINT daddr = (UINT)_atoi64(command);
			//printf("receive mbs %d\n", val);
			
			it->second.agent_status.topktable.TopKTable[it->second.agent_status.topktable.idx].daddr = daddr;
			command = strtok_s(cmd2, token2, &cmd2);
			val = atoi(command);
			it->second.agent_status.topktable.TopKTable[it->second.agent_status.topktable.idx].protocol = val;

			command = strtok_s(cmd2, token2, &cmd2);
			val = atoi(command);
			if (g_pkt_base_sample)
			{
				it->second.agent_status.topktable.TopKTable[it->second.agent_status.topktable.idx].pkt = val;
				std::map<UINT, UINT>::iterator hh_it = g_Heavyhitter.find(daddr);
				if (hh_it != g_Heavyhitter.end())
				{
					hh_it->second += val;
				}
				else
					g_Heavyhitter.insert(std::pair<UINT, UINT>(daddr, val));

			}
			else if (g_flow_base_sample)
				it->second.agent_status.topktable.TopKTable[it->second.agent_status.topktable.idx].numofsource = val;

			it->second.agent_status.topktable.idx++;
			if (it->second.agent_status.topktable.idx >= it->second.agent_status.topktable.K)
				it->second.agent_status.topktable.idx = 0;
		}
		else
		{
			break;
		}


		rest = next_token;
		command = strtok_s(rest, token, &next_token);
		if (command == NULL)
		{
			break;
		}

	} while (true);

	it->second.agent_status.status[0] = '\0';

}


void DisplayAgentStatus()
{
	static UINT measure_interval = 0;
	std::map<SOCKET, AGENT_CONTROL>::iterator it;
	//INT i;
	//INT displaynum;
	//printf("num of agent %d\n", gIDSSocketTable.size());
	AGNET_STATUS_TOTAL the_sum;
	memset(&the_sum, 0, sizeof(the_sum));
	printf("%u: ", (gCurrentTime/1000));

	for (it = gIDSStatus.table.begin(); it != gIDSStatus.table.end(); ++it)
	{

		BOOL deleting = FALSE;
		for (int j = 0; j < MAX_NUM_VM; ++j)
		{
			if (it->first == g_delete_sock[j])
			{
				deleting = TRUE;
				break;
			}
		}
		if (deleting == TRUE)
			continue;


		the_sum.conn += it->second.agent_status.conn;
		the_sum.flow += it->second.agent_status.flow;
		the_sum.smooth_cpu_usage += it->second.agent_status.smooth_cpu_usage;
		the_sum.mem_usage += it->second.agent_status.mem_usage;
		the_sum.MBS += it->second.agent_status.MBS;
		the_sum.PPS += it->second.agent_status.PPS;
		the_sum.totalsample += it->second.agent_status.totalsamples;
		the_sum.totaltopk += it->second.agent_status.topktable.K;
		printf("%u(%u) ", it->second.agent_status.smooth_cpu_usage, 
			GetNumRoutesbySock(it->first));

#ifdef FILEIO
		fprintf(ofp, "%u: ", (gCurrentTime / 1000));
		fprintf(ofp, "%u(%u) ", it->second.agent_status.smooth_cpu_usage,
			GetNumRoutesbySock(it->first));
#endif

		//printf("%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n", 
		//	it->second.agent_status.smooth_cpu_usage,
		//	it->second.agent_status.total_cpu_usage,
		//	it->second.agent_status.mem_usage,
		//	it->second.agent_status.PPS,
		//	it->second.agent_status.MBS,
		//	it->second.agent_status.conn,
		//	it->second.agent_status.flow,
		//	it->second.noticetable.size(),
		//	it->second.agent_status.totalsamples
		//	);

		//displaynum = min(1, it->second.agent_status.topktable.K);
		//for (i = 0; i < displaynum; ++i)
			//printf("%u.%u.%u.%u:%u %u\n", NIPQUAD_HOSTORDER(it->second.agent_status.topktable.TopKTable[i].daddr),
			//it->second.agent_status.topktable.TopKTable[i].protocol,
			//it->second.agent_status.topktable.TopKTable[i].numofsource);
	}

	

	printf("\t%u %u %u %u %u %u %u\n", the_sum.MBS, the_sum.PPS, the_sum.conn,
		the_sum.flow, the_sum.mem_usage, the_sum.totalsample, the_sum.totaltopk);

#ifdef FILEIO
	fprintf(ofp, "\t%u %u %u %u %u %u %u\n", the_sum.MBS, the_sum.PPS, the_sum.conn,
		the_sum.flow, the_sum.mem_usage, the_sum.totalsample, the_sum.totaltopk);
	
#endif


	measure_interval += 1;
	if (measure_interval == 5)
	{
		measure_interval = 0;

		std::vector<std::pair<UINT, UINT> > hhvector(g_Heavyhitter.begin(), g_Heavyhitter.end());
		std::vector<std::pair<UINT, UINT>>::iterator v_it;
		std::sort(hhvector.begin(), hhvector.end(), &hhsortFn);
		UINT accu = 0;
		UINT count = 0;
		for (v_it = hhvector.begin(); v_it != hhvector.end(); ++v_it)
		{
			if (g_top_groundtruth.find(v_it->first) != g_top_groundtruth.end())
				accu += 1;
			count += 1;
			if (count == 100)
				break;
		}

		printf("accuracy = %u\n", accu);
#ifdef FILEIO
		fprintf(ofp, "accuracy = %u\n", accu);
		fflush(ofp);
#endif
		g_Heavyhitter.clear();

	}
}

UINT IPtoRoutingIdx(UINT ip)
{
	UINT idx = 0;
	idx = ((uint8 *)&ip)[2] & 0x01;
	idx = idx << 8 | ((uint8 *)&ip)[1];
#ifndef ROUTING_BLOCK
	idx = idx << 8 | ((uint8 *)&ip)[0];
#endif
	//idx = ip & 0x0001ffff;

	return idx;
}



cmd_result_t CommandTG(char *command, std::string tg)
{
	std::map<USHORT, SOCKET>::iterator it;
	//std::map<SOCKET, AGENT_CONTROL>::iterator it_control;
	WSAOVERLAPPED olp[MAX_MBUFCOUNT];
	WSAEVENT eArray[MAX_MBUFCOUNT];
	WSABUF mBuf[MAX_MBUFCOUNT];
	//char *CmdBuf = "getstate";
	//UINT Cmdlen = (UINT)strnlen(CmdBuf, MAX_MBUFCOUNT);
	INT i;
	INT err;
	INT rc;
	INT total_agent;
	DWORD currLen;
	DWORD Flags;
	SOCKET socklist[MAX_MBUFCOUNT];


	if (command == NULL || command[0] == '\0')
		return CMD_FAIL;


	char *cmdbuf = new char[ROUTING_MAXBUFLEN];

	if (cmdbuf == NULL)
	{
		printf("allocate fail\n");
		return CMD_FAIL;
	}

	cmdbuf[0] = 0;
	strcat_s(cmdbuf, ROUTING_MAXBUFLEN, command);
	strcat_s(cmdbuf, ROUTING_MAXBUFLEN, " ");
	INT curr = (INT)strnlen(cmdbuf, ROUTING_MAXBUFLEN);
	memset(&cmdbuf[curr], '0', ROUTING_MAXBUFLEN - curr);
	cmdbuf[ROUTING_MAXBUFLEN - 1] = '\0';
	

	//printf("reroute %s", command);


	//char *CmdBuf = "getstate";
	//UINT Cmdlen = (UINT)strnlen(CmdBuf, MAX_MBUFCOUNT);

	/*size_t cmdlen = (size_t)strnlen_s(command, ROUTING_MAXBUFLEN);
	cmdbuf[0] = '\0';
	char lenbuf[30];
	strcat_s(cmdbuf, ROUTING_MAXBUFLEN, "tgctrl ");
	_ui64toa_s(cmdlen, lenbuf, 30, 10);
	strcat_s(cmdbuf, ROUTING_MAXBUFLEN, lenbuf);
	strcat_s(cmdbuf, ROUTING_MAXBUFLEN, "|");
	strcat_s(cmdbuf, ROUTING_MAXBUFLEN, command);
	strcat_s(cmdbuf, ROUTING_MAXBUFLEN, " ");*/

	//char CmdBuf[ROUTING_MAXBUFLEN];
	//CmdBuf[0] = 0;
	//if (strcat_s(CmdBuf, ROUTING_MAXBUFLEN, command) != 0)
	//	return CMD_FAIL;


	for (i = 0; i < MAX_MBUFCOUNT; ++i)
	{
		eArray[i] = WSACreateEvent();
		if (eArray[i] == NULL)
			printf("WSACreateEvent fail\n");

		ZeroMemory(&olp[i], sizeof(WSAOVERLAPPED));

		olp[i].hEvent = eArray[i];
	}



	if (tg == "all")
	{
		
		EnterCriticalSection(&gTGSocketTable.lock);
		i = 0;
		for (it = gTGSocketTable.table.begin(); it != gTGSocketTable.table.end(); ++it)
		{
			socklist[i] = it->second;
			++i;
		}	
		LeaveCriticalSection(&gTGSocketTable.lock);
		total_agent = i;

		if (total_agent == 0)
			return CMD_FAIL;


		for (i = 0; i < total_agent; ++i)
		{
			//printf("send one command\n");
			mBuf[i].buf = cmdbuf;
			//mBuf[i].len = (ULONG)strnlen(cmdbuf, ROUTING_MAXBUFLEN);
			mBuf[i].len = ROUTING_MAXBUFLEN;

			rc = WSASend(socklist[i], &mBuf[i], 1, NULL, 0, &olp[i], NULL);
			if ((rc == SOCKET_ERROR) &&
				(WSA_IO_PENDING != (err = WSAGetLastError())))
			{
				//WARN_errno(true, "WSASend");
				printf("CommandTG: WSASend fail = %d\n", WSAGetLastError());
				closesocket(socklist[i]);
				EnterCriticalSection(&gTGSocketTable.lock);
				gTGSocketTable.table.erase(GetPortbySock(socklist[i]));
				LeaveCriticalSection(&gTGSocketTable.lock);
				//break;
				continue;
			}
		}

		


		rc = WSAWaitForMultipleEvents(total_agent, eArray, TRUE, INFINITE, FALSE);
		if (rc == WSA_WAIT_FAILED)
		{
			printf("WSAWaitForMultipleEvents fail = %d\n", WSAGetLastError());
			return CMD_FAIL;
		}

		//printf("WSAWaitForMultipleEvents\n");
		for (i = 0; i < total_agent; ++i)
		{
			rc = WSAGetOverlappedResult(socklist[i], &olp[i], &currLen, FALSE, &Flags);
			if (rc == FALSE && (WSA_IO_INCOMPLETE != (err = WSAGetLastError())))
			{
				printf("WSAGetOverlappedResult = %d\n", WSAGetLastError());
				continue;
			}

			//printf("Send command: %s, %d Bytes\n", CmdBuf, currLen);

			rc = WSAResetEvent(olp[i].hEvent);
			if (rc == FALSE)
			{
				printf("WSAResetEvent = %d\n", WSAGetLastError());
			}
		}

	}

	else{

		//change specific tg, no use case for now
	}

	free(cmdbuf);
	return CMD_OK;

}




cmd_result_t CommandIDS(char *command, SOCKET ids)
{
#if 0
	std::map<std::string, SOCKET>::iterator it;
	//std::map<SOCKET, AGENT_CONTROL>::iterator it_control;
	WSAOVERLAPPED olp[MAX_MBUFCOUNT];
	WSAEVENT eArray[MAX_MBUFCOUNT];
	WSABUF mBuf[MAX_MBUFCOUNT];
	//char *CmdBuf = "getstate";
	//UINT Cmdlen = (UINT)strnlen(CmdBuf, MAX_MBUFCOUNT);
	INT i;
	INT err;
	INT rc;
	INT total_agent;
	DWORD currLen;
	DWORD Flags;
	SOCKET socklist[MAX_MBUFCOUNT];

	//char *CmdBuf = "getstate";
	//UINT Cmdlen = (UINT)strnlen(CmdBuf, MAX_MBUFCOUNT);

	char CmdBuf[MAXBUFLEN];
	CmdBuf[0] = 0;
	if (strcat_s(CmdBuf, MAXBUFLEN, command) != 0)
		return CMD_FAIL;


	for (i = 0; i < MAX_MBUFCOUNT; ++i)
	{
		eArray[i] = WSACreateEvent();
		if (eArray[i] == NULL)
			printf("WSACreateEvent fail\n");

		ZeroMemory(&olp[i], sizeof(WSAOVERLAPPED));

		olp[i].hEvent = eArray[i];
	}



	if (ids == (UINT)-1) //to all ids agents, no use case for now
	{
		i = 0;
		for (it = gTGSocketTable.begin(); it != gTGSocketTable.end();)
		{
			//printf("send one command\n");
			mBuf[i].buf = CmdBuf;
			mBuf[i].len = (ULONG)strnlen(CmdBuf, MAXBUFLEN);

			rc = WSASend(it->second, &mBuf[i], 1, NULL, 0, &olp[i], NULL);
			if ((rc == SOCKET_ERROR) &&
				(WSA_IO_PENDING != (err = WSAGetLastError())))
			{
				//WARN_errno(true, "WSASend");
				printf("WSASend fail = %d\n", WSAGetLastError());
				closesocket(it->second);
				gTGSocketTable.erase(it++);
				//break;
				continue;
			}

			socklist[i] = it->second;
			++i;
			++it;
		}

		total_agent = i;

		if (total_agent == 0)
			return CMD_FAIL;


		rc = WSAWaitForMultipleEvents(total_agent, eArray, TRUE, INFINITE, FALSE);
		if (rc == WSA_WAIT_FAILED)
		{
			printf("WSAWaitForMultipleEvents fail = %d\n", WSAGetLastError());
			return CMD_FAIL;
		}

		//printf("WSAWaitForMultipleEvents\n");
		for (i = 0; i < total_agent; ++i)
		{
			rc = WSAGetOverlappedResult(socklist[i], &olp[i], &currLen, FALSE, &Flags);
			if (rc == FALSE && (WSA_IO_INCOMPLETE != (err = WSAGetLastError())))
			{
				printf("WSAGetOverlappedResult = %d\n", WSAGetLastError());
				continue;
			}

			//printf("Send command: %s, %d Bytes\n", CmdBuf, currLen);

			rc = WSAResetEvent(olp[i].hEvent);
			if (rc == FALSE)
			{
				printf("WSAResetEvent = %d\n", WSAGetLastError());
			}


		}

	}

	else{
#endif

		INT bytesend = send(ids, command, (INT)strnlen(command, MAXBUFLEN), 0);
		if (bytesend < 0)
			printf("notification feedback error\n");


#if 0
	}
#endif 


	return CMD_OK;

}


cmd_result_t ChangeRoute()
{
	std::map<SOCKET, AGENT_CONTROL>::iterator it;
	std::set<ROUTE_NOTICE>::iterator jt;
	char buf[20];
	char *cmd  = new char [ROUTING_MAXBUFLEN];

	UINT rt_count = 0;
	UINT ANNOUNCE_SIZE = ROUTING_MAXBUFLEN / 20;
	UINT annouce_idx = 0;


	if (cmd == NULL)
	{
		printf("allocate buffer fail\n");
		return CMD_FAIL;
	}
	//cmd[0] = 0;

	memset(cmd, 0, ROUTING_MAXBUFLEN);

	for (it = gIDSStatus.table.begin(); it != gIDSStatus.table.end(); ++it)
	{
		
		cmd[0] = 0;

		for (jt = it->second.noticetable.begin(); jt != it->second.noticetable.end(); ++jt)
		{
			if (jt->reported == ROUTE_CAN_REDI)
			{
				//std::string str(jt->NewDstip);
				//std::map<std::string, SOCKET>::iterator kt;
				//std::map<SOCKET, AGENT_CONTROL>::iterator mt;
				//kt = gIDSSocketTable.find(str);
				//if (kt != gIDSSocketTable.end())
				//{
				//	mt = gIDSStatus.find(kt->second);
				//	if (mt != gIDSStatus.end())
				//	{
				//		mt->second.agent_status.routes++;
				//	}
				//}

				
				
				strcat_s(cmd, ROUTING_MAXBUFLEN, " rt ");
				_itoa_s((INT)jt->RouteEntry, buf, 10);
				strcat_s(cmd, ROUTING_MAXBUFLEN, buf);
				strcat_s(cmd, ROUTING_MAXBUFLEN, " ");
				_itoa_s((INT)jt->NewDst, buf, 10);
				strcat_s(cmd, ROUTING_MAXBUFLEN, buf);

				jt->reported = ROUTE_REDIRECTED;

				//it->second.agent_status.routes--;
				//rt_count++;
				//if (rt_count >= REROUTE_TABLE_SIZE)
				//	break;
				annouce_idx++;
			}
			else if (jt->reported == ROUTE_CANNOT_REDI)
			{
				std::string str(jt->NewDstip);
				std::map<std::string, SOCKET>::iterator kt;
				//std::map<SOCKET, AGENT_CONTROL>::iterator mt;
				kt = gIDSSocketTable.table.find(str);
				if (kt != gIDSSocketTable.table.end())
				{
					//mt = gIDSStatus.find(kt->second);
					//if (mt != gIDSStatus.end())
					//{
					//	mt->second.agent_status.routes++;
					//}

					//printf("redirect --> new VM\n");
					strcat_s(cmd, ROUTING_MAXBUFLEN, " rt ");
					_itoa_s((INT)jt->RouteEntry, buf, 10);
					strcat_s(cmd, ROUTING_MAXBUFLEN, buf);
					strcat_s(cmd, ROUTING_MAXBUFLEN, " ");
					_itoa_s((INT)jt->NewDst, buf, 10);
					strcat_s(cmd, ROUTING_MAXBUFLEN, buf);

					jt->reported = ROUTE_REDIRECTED;
					//it->second.agent_status.routes--;
					//rt_count++;
					//if (rt_count >= REROUTE_TABLE_SIZE)
					//	break;

					//printf("%s\n", cmd);

					annouce_idx++;
				}
			}

			

			if (annouce_idx >= ANNOUNCE_SIZE)
			{
				size_t cmdlen = (size_t)strnlen_s(cmd, ROUTING_MAXBUFLEN);
				if (cmdlen > 0)
					CommandTG(cmd, "all");
				cmd[0] = 0;
				annouce_idx = 0;
				memset(cmd, 0, ROUTING_MAXBUFLEN);
				//CommandIDS("redirected", it->first);
			}
		}	

		size_t cmdlen = (size_t)strnlen_s(cmd, ROUTING_MAXBUFLEN);
		if (cmdlen > 0) {
			CommandTG(cmd, "all");
			memset(cmd, 0, ROUTING_MAXBUFLEN);
		}
	}



	//free(cmdbuf);
	free(cmd);

	return CMD_OK;
}


//dst mac, src mac, daddr
cmd_result_t MoveRoute(UINT dst, UINT src, UINT entry)
{
	std::map<UINT, std::set<UINT>>::iterator it;
	std::map<UINT, std::set<UINT>>::iterator kt;
	std::set<UINT>::iterator jt;
	it = gVM_route_set.find(src);
	if (it == gVM_route_set.end())
		return CMD_FAIL;
	
	jt = it->second.find(entry);
	if (jt == it->second.end())
		return CMD_FAIL;

	kt = gVM_route_set.find(dst);
	if (kt == gVM_route_set.end())
		return CMD_FAIL;

	kt->second.insert(*jt);

	it->second.erase(jt);

	return CMD_OK;

}

cmd_result_t QueryRoutes(UINT count, UINT macidx, std::vector<UINT> &routes)
{
	std::map<UINT, std::set<UINT>>::iterator jt;
	std::set<UINT>::iterator it;
	UINT total_count;

	jt = gVM_route_set.find(macidx);
	if (jt != gVM_route_set.end())
	{
		total_count = (UINT)min(count, jt->second.size());
		for (it = jt->second.begin(); it != jt->second.end(); ++it)
		{
			routes.push_back(*it);

		}
	}
	return CMD_OK;
}

cmd_result_t RePackFlow(SOCKET sock, UINT sample_value, UINT daddr, OUT std::set<ROUTE_NOTICE> &route_notice)
{
	//std::map<SOCKET, AGENT_CONTROL>::iterator it;
	std::map<SOCKET, AGENT_CONTROL>::iterator jt;
	//UINT i;
	UINT idx;
	ROUTE_NOTICE route_entry;
	BOOL newreport = false;
	//UINT budget;
	UINT count = 0;
	BOOL allocated;
	std::string str;
	//UINT traffic_own;
	//UINT own_budget = 0;
	static UINT notice = 0;
	//UINT sample_value;


	//sample_value = (g_flow_base_sample ? conn_info.numofsource:conn_info.pkt);

	memset(&route_entry, 0, sizeof(route_entry));

	//idx = IPtoRoutingIdx(conn_info.daddr);

	idx = IPtoRoutingIdx(daddr);
	route_entry.RouteEntry = idx;

	allocated = false;

	if (route_notice.size() < NOTICE_TABLE_SIZE
		&& route_notice.find(route_entry) == route_notice.end())
	{
		for (jt = gIDSStatus.table.begin(); jt != gIDSStatus.table.end(); ++jt)
		{
			if (jt->first == sock)
				continue;

			//allocation
			//if (i == it->second.agent_status.topktable.K - 1) //last one
			//	printf("budge: %d\n", jt->second.agent_status.budget);

			if (jt->second.agent_status.budget >= sample_value)
			{

				str = GetNamebySock(jt->first);

				route_entry.NewDst = jt->second.agent_status.MACidx;
				route_entry.noticetime = gCurrentTime;
				route_entry.reported = ROUTE_CAN_REDI;  //redirect
				route_entry.NewDstip = const_cast<char *>(str.c_str());
				route_notice.insert(route_entry);

				//migrate route
				UINT src_idx = GetMacIdxbySock(sock);
				UINT dst_idx = GetMacIdxbySock(jt->first);
				MoveRoute(dst_idx, src_idx, idx);

				//jt->second.agent_status.routes++;
				if (sample_value > EXTRAPOLATE_THRESH)
					jt->second.agent_status.budget -= sample_value * HIGH_EXTRA;
				else
					jt->second.agent_status.budget -= (sample_value * LOW_EXTRA);


				allocated = true;

			}
		}

		if (allocated == false)
		{
			//UINT vol = it->second.agent_status.topktable.TopKTable[i].numofsource;
			//if (i == it->second.agent_status.topktable.K - 1) //last one
			//printf("own budge = %d\n", g_starting_vm_budget);

			if (g_force_back){
				//printf("scale out diabled\n");
				return CMD_OK;
			}

			if (g_opening_vm.budget >= (INT)sample_value)
			{
				if (sample_value > EXTRAPOLATE_THRESH)
					g_opening_vm.budget -= (sample_value * HIGH_EXTRA);
				
				else
					g_opening_vm.budget -= (sample_value * LOW_EXTRA);

				route_entry.NewDst = g_opening_vm.vm_mac;
				route_entry.noticetime = gCurrentTime;
				//route_entry.reported = ROUTE_CAN_REDI; //ROUTE_CANNOT_REDI;  //not redirect
				route_entry.reported = ROUTE_CANNOT_REDI;  //not redirect
				route_entry.NewDstip = g_opening_vm.name;
				route_notice.insert(route_entry);

				MoveRoute(g_opening_vm.vm_mac, GetMacIdxbySock(sock), idx);
			}
			else
			{
				//start new
				if (!gVM_Pool.empty())
				{
					UINT vmidx = 0;
					for (int j = 0; j < MAX_NUM_VM; ++j)
					{
						if (std::string(gVM_list[j].name) == gVM_Pool.front())
						{
							vmidx = j;
							gVM_Pool.pop();
							break;
						}
					}

					printf("start new VM id: %d\n", vmidx);
					//DWORD ThreadID;
					//UINT vmidx = *gVM_idx;
					
					gStart_VM_Overlay[gStartVMOverlay_idx].VM_Idx = vmidx;
					gStart_VM_Overlay[gStartVMOverlay_idx].ThHandle = CreateThread(NULL, 0, StartAgent,
						(LPVOID)&gStart_VM_Overlay[gStartVMOverlay_idx].VM_Idx, 0, &gStart_VM_Overlay[gStartVMOverlay_idx].ThreadPID);

					if (gStart_VM_Overlay[gStartVMOverlay_idx].ThHandle == NULL)
					{
						printf("create thread fail\n");
						//return -1;
						return CMD_FAIL;
					}

					//gVM_idx++;
					gStartVMOverlay_idx = (gStartVMOverlay_idx + 1) % MAX_START_VM_THREAD;
					//start new vm

					route_entry.NewDst = gVM_list[vmidx].MACidx;
					route_entry.noticetime = gCurrentTime;
					//route_entry.reported = ROUTE_CAN_REDI; //ROUTE_CANNOT_REDI;  //not redirect
					route_entry.reported = ROUTE_CANNOT_REDI;  //not redirect
					route_entry.NewDstip = gVM_list[vmidx].name;
					route_notice.insert(route_entry);

					//add routes
					std::set<UINT> routeset;
					gVM_route_set.insert(std::pair<UINT, std::set<UINT>>(gVM_list[vmidx].MACidx, routeset));

					UINT src_idx = GetMacIdxbySock(sock);
					UINT dst_idx = GetMacIdxbyName(std::string(gVM_list[vmidx].name));
					MoveRoute(dst_idx, src_idx, idx);

					g_opening_vm.budget = SAMPLE_IN_LOW_WATERMARK * SAMPLE_RATE;  // !! need change
					if (sample_value > EXTRAPOLATE_THRESH)
						g_opening_vm.budget -= (sample_value * HIGH_EXTRA);
					else
						g_opening_vm.budget -= (sample_value * LOW_EXTRA);

					//std::string str = GetNamebySock(sock);
					g_opening_vm.num_of_vm += 1;
					//memcpy(g_opening_vm.name, const_cast<char *>(str.c_str()), str.length());
					//g_opening_vm.name[str.length() + 1] = 0;
					
					g_opening_vm.name[0] = 0;
					strcat_s(g_opening_vm.name, gVM_list[vmidx].name);
					
					//memcpy(g_opening_vm.name, const_cast<char *>(str.c_str()), str.length());
					g_opening_vm.vm_mac = gVM_list[vmidx].MACidx;

				}
				else
				{
					if (notice == 0)
					{
						notice = 1;
						printf("no new VM\n");
						return CMD_FAIL;
					}
				}
			}
		}

		
	}
	//duplicate notice

	return CMD_OK;
}


cmd_result_t ForceScaleback()
{
	UINT idx = 0;
	memset(g_delete_sock, 0, MAX_NUM_VM);

	std::map<std::string, SOCKET>::iterator it;
	for (it = gIDSSocketTable.table.begin(); it != gIDSSocketTable.table.end(); ++it)
	{
		BOOL standby = 0;
		for (int i = 0; i < NUM_STANDBY_VM; ++i)
		{
			if (it->second == g_standby_sock[i])
			{
				standby = 1;
				break;
			}
		}
		if (standby == 1)
			continue;

		g_delete_sock[idx++] = it->second;

		UINT macidx = GetMacIdxbySock(it->second);
		std::vector<UINT> routes;
		QueryRoutes(MAX_NUM_ROUTE, macidx, routes);
		//printf("withdraw %d flows from %d vm\n", routes.size(), GetMacIdxbySock(move_it->first));

		while (!routes.empty())
		{
			MoveRoute(3, macidx, routes.back());
			routes.pop_back();
		}
	}

	g_self_delete = 1;

	return CMD_OK;

}

cmd_result_t Scaleback()
{
	std::map<SOCKET, AGENT_CONTROL>::iterator it;
	std::map<SOCKET, AGENT_STATUS>::iterator kt;
	std::map<SOCKET, AGENT_CONTROL>::iterator move_it;
	UINT min_overhead = SAMPLE_IN_HIGH_WATERMARK;
	UINT count = 0;
	UINT total_budget = 0;
	//SOCKET sock;

	for (it = gIDSStatus.table.begin(); it != gIDSStatus.table.end();)
	{
		//printf("routes = %d", it->second.agent_status.routes);
		//gVM_route_set.find(GetMacIdxbySock(it->first))

		if (GetNumRoutesbySock(it->first) == 0)
		{
			if (gCurrentTime - it->second.agent_status.last_active_time > SCALEBACK_INACTIVE_TIMEOUT)
			{
				printf("scale back\n");
				if (StopAgent(it->first) == CMD_OK)
				{
					EnterCriticalSection(&gIDSStatus.lock);
					gIDSStatus.table.erase(it++);
					LeaveCriticalSection(&gIDSStatus.lock);
				}
			}
		}
		else
		{
			if (it->second.agent_status.smooth_cpu_usage < CPU_LOW_WATERMARK)
			{
				count++;
				if (it->second.agent_status.overhead < min_overhead)
				{
					min_overhead = it->second.agent_status.overhead;
					//sock = it->first;
					move_it = it;
				}
				total_budget += it->second.agent_status.budget;
			}


			it->second.agent_status.last_active_time = gCurrentTime;
			++it;
		}

	}
	//migrate the volume to other lightly weighted VM
	if (count > NUM_STANDBY_VM && total_budget > min_overhead)
	{
		//char withdrawcmd[128];
		//char *cmd = "withdraw ";
		//withdrawcmd[0] = '\0';
		//strcat_s(withdrawcmd, cmd);
		//char buf[10];
		//_itoa_s(total_budget, buf, 10);
		//strcat_s(withdrawcmd, buf);
		//
		//INT cmdlen = (INT)strnlen(withdrawcmd, 128);

		//INT currLen = send(sock, withdrawcmd, cmdlen, 0);
		//if (currLen < 0)
		//{
		//	printf("send withdrow error = %");
		//}
		std::vector<UINT> routes;
		QueryRoutes(MAX_NUM_ROUTE, GetMacIdxbySock(move_it->first), routes);
		printf("withdraw %d flows from %d vm\n", routes.size(), GetMacIdxbySock(move_it->first));

		while (!routes.empty())
		{
			RePackFlow(it->first, 1, routes.back(), move_it->second.noticetable);
			routes.pop_back();
		}
	}

	return CMD_OK;

}

cmd_result_t TrafficRedirection()
{
	std::map<SOCKET, AGENT_CONTROL>::iterator it;
	std::map<SOCKET, AGENT_CONTROL>::iterator jt;
	UINT i;
	//UINT idx;
	//ROUTE_NOTICE route_entry;
	BOOL newreport = false;
	//UINT budget;
	UINT count = 0;
	//BOOL allocated;
	std::string str;
	//UINT traffic_own;
	//UINT own_budget = 0;
	static UINT notice = 0;
	UINT sample_value;
	UINT volume_shift = 0;
	UINT total_volume_shift = 0;


	//scale-out

	for (it = gIDSStatus.table.begin(); it != gIDSStatus.table.end(); ++it)
	{
#if 0
		if ((it->second.agent_status.miti_stage == 0
			&& it->second.agent_status.smooth_cpu_usage >= CPU_HIGH_WATERMARK)
			|| (it->second.agent_status.miti_stage == 1
			&& it->second.agent_status.smooth_cpu_usage >= CPU_LOW_WATERMARK
			&& gCurrentTime - it->second.agent_status.last_miti_time >= MITIGATION_INTERVAL))
		{
			if (it->second.agent_status.miti_stage == 0)
				it->second.agent_status.miti_stage = 1;

			it->second.agent_status.last_miti_time = gCurrentTime;

			for (i = 0; i < it->second.agent_status.topktable.K; ++i)
			{
				idx = IPtoRoutingIdx(it->second.agent_status.topktable.TopKTable[i].daddr);
				route_entry.RouteEntry = idx;

				if (it->second.noticetable.size() < NOTICE_TABLE_SIZE
					&& it->second.noticetable.find(route_entry) == it->second.noticetable.end())
				{
					route_entry.noticetime = gCurrentTime;
					route_entry.reported = false;
					it->second.noticetable.insert(route_entry);

					if (newreport == false)
						newreport = true;
				}
			}

		}
		else if (it->second.agent_status.miti_stage == 1
			&& it->second.agent_status.smooth_cpu_usage < CPU_LOW_WATERMARK)
		{
			printf("back to normal\n");
			it->second.agent_status.miti_stage = 0;
		}
#endif
		//printf("move %d conn from sock %d\n", it->second.agent_status.topktable.K, it->first);
		//if (it->second.agent_status.smooth_cpu_usage < CPU_HIGH_WATERMARK 
		if (it->second.agent_status.overload <= 0
			|| gCurrentTime - it->second.agent_status.last_miti_time < MITIGATION_INTERVAL)
			continue;

		printf("repack: topk %d, overload %d, notice: %d\n", 
			it->second.agent_status.topktable.K, it->second.agent_status.overload,
			it->second.noticetable.size());

		total_volume_shift = it->second.agent_status.overload;

		for (i = 0; i < it->second.agent_status.topktable.K; ++i)
		{	

			if (volume_shift >= total_volume_shift)
				break;

			TOP_CONN_INFO conn_info(it->second.agent_status.topktable.TopKTable[i]);
			
			if (it->second.agent_status.overload > 0 && HasRoute(it->first, conn_info.daddr))
				//&& it->second.noticetable.size() < NOTICE_TABLE_SIZE)
			{
				sample_value = (g_flow_base_sample ? conn_info.numofsource : conn_info.pkt);
				if (sample_value > EXTRAPOLATE_THRESH)
				{
					it->second.agent_status.overload -= (sample_value * HIGH_EXTRA);
					volume_shift = sample_value * HIGH_EXTRA;
				}
				else
				{
					it->second.agent_status.overload -= (sample_value * LOW_EXTRA);
					volume_shift = sample_value * LOW_EXTRA;
				}

				//re-pack this flow
				RePackFlow(it->first, sample_value, conn_info.daddr,
					it->second.noticetable);
			}


		}

		//printf("Top: left: overload %d, route %d, notice %d, own %d\n", it->second.agent_status.overload,
		//	gVM_route_set[GetMacIdxbySock(it->first)].size(),
		//	it->second.noticetable.size(),
		//	g_opening_vm.budget);

		if (it->second.agent_status.overload > 0 && it->second.noticetable.size() < NOTICE_TABLE_SIZE)
		{
			if (volume_shift >= total_volume_shift)
				break;

			UINT needtomove = min((it->second.agent_status.overload / LOW_EXTRA),
				(INT)(total_volume_shift - volume_shift) / LOW_EXTRA);

			needtomove = (UINT)min(needtomove,
				NOTICE_TABLE_SIZE - it->second.noticetable.size());

			std::vector<UINT> total_daddr;

			QueryRoutes(needtomove, GetMacIdxbySock(it->first), total_daddr);

			while (!total_daddr.empty() && it->second.noticetable.size() < NOTICE_TABLE_SIZE) {
				RePackFlow(it->first, 1, total_daddr.back(),
					it->second.noticetable);
				total_daddr.pop_back();
			}
		}

		//printf("Bottom: left: overload %d, route %d, notice %d, own %d\n", it->second.agent_status.overload,
		//	gVM_route_set[GetMacIdxbySock(it->first)].size(),
		//	it->second.noticetable.size(),
		//	g_opening_vm.budget);

		//update mitigation time
		it->second.agent_status.last_miti_time = gCurrentTime;

	}

	//scale-back
	//if (gCurrentTime - gLast_Scale_Back_Time >= SCALEBACK_INTERVAL)
	//{
	//	Scaleback();
	//	gLast_Scale_Back_Time = gCurrentTime;
	//}



	
	if(	ChangeRoute() == CMD_FAIL)
		return CMD_FAIL;
	
	return CMD_OK;
}


void UpdateIDSStatus(void)
{
	std::map<SOCKET, AGENT_CONTROL>::iterator it;
	std::set<ROUTE_NOTICE>::iterator jt;

	for (it = gIDSStatus.table.begin(); it != gIDSStatus.table.end(); ++it)
	{

		for (jt = it->second.noticetable.begin(); jt != it->second.noticetable.end();)
		{
			if (jt->reported == ROUTE_REDIRECTED && gCurrentTime - jt->noticetime >= MITIGATION_INTERVAL)
			{
				it->second.noticetable.erase(jt++);
				//printf("delete 1: %u\n", it->second.noticetable.size());
			}
			else
				++jt;
		}
		
	}

}



void UpdateTGStatus()
{
	//static UINT num_interval = 0;
	//update traffic generator
	//char cmd[] = "tgctrl 10|rt 12125 5";
	static UINT deleting_time = 0;
	static UINT stop = 0;
	static BOOL deleted = 1;
	if ( stop == 0 && (gCurrentTime / 1000) >= g_tg_cmd[g_tg_idx].time)
	{
		if (g_tg_cmd[g_tg_idx].start)
		{
#ifdef FILEIO
			fprintf(ofp, "start tg %d\n", g_tg_cmd[g_tg_idx].id);
#endif
			printf("start tg %d\n", g_tg_cmd[g_tg_idx].id);
			g_force_back = 0;
			DoStartTrafficGenerator(gTG_list[g_tg_cmd[g_tg_idx].id].name, 
				gTG_list[g_tg_cmd[g_tg_idx].id].port);
		}
		else
		{

#ifdef FILEIO
			fprintf(ofp, "stop tg %d\n", g_tg_cmd[g_tg_idx].id);
#endif
			printf("stop tg %d\n", g_tg_cmd[g_tg_idx].id);
			DoStopTrafficGenerator(gTG_list[g_tg_cmd[g_tg_idx].id].name,
				gTG_list[g_tg_cmd[g_tg_idx].id].port);

			/*g_force_back = 1;
			ForceScaleback();
			deleting_time = gCurrentTime;
			deleted = 0;*/


		}

		if (deleted == 0 && gCurrentTime - deleting_time > 30000)
		{
			std::map<std::string, SOCKET>::iterator it;
			EnterCriticalSection(&gIDSSocketTable.lock);
			for (it = gIDSSocketTable.table.begin(); it != gIDSSocketTable.table.end();)
			{
				BOOL deleting = 0;
				for (int j = 0; j < MAX_NUM_VM; ++j)
				{
					if (g_delete_sock[j] == it->second)
					{
						deleting = 1;
						break;
					}
				}
				if (deleting)
				{
					gIDSSocketTable.table.erase(it++);
				}
				else
					++it;
			}
			LeaveCriticalSection(&gIDSSocketTable.lock);

			std::map<SOCKET, AGENT_CONTROL>::iterator jt;
			EnterCriticalSection(&gIDSStatus.lock);
			for (jt = gIDSStatus.table.begin(); jt != gIDSStatus.table.end();)
			{
				BOOL deleting = 0;
				for (int j = 0; j < MAX_NUM_VM; ++j)
				{
					if (g_delete_sock[j] == jt->first)
					{
						deleting = 1;
						break;
					}
				}
				if (deleting)
				{
					gIDSStatus.table.erase(jt++);
				}
				else
					++jt;
			}
			LeaveCriticalSection(&gIDSStatus.lock);


			memset(g_delete_sock, 0, MAX_NUM_VM);
			deleted = 1;
		}

		g_tg_idx++;
		
		//if (g_tg_idx >= MAX_NUM_TG)
		//{
		//	if (stop == 0){

			//	printf("experiment stop\n");
			//	stop = 1;
			//}
		//}
		
		
	}	
	if (gCurrentTime > 1000 * 1000)
	{
		printf("experiment stop\n");
		exit(1);
	}
}




DWORD WINAPI DoStateCollection(LPVOID paramPtr)
{
	std::map<std::string, SOCKET>::iterator it;
	std::map<SOCKET, AGENT_CONTROL>::iterator it_status;
	WSAOVERLAPPED olp[MAX_MBUFCOUNT];
	WSAEVENT eArray[MAX_MBUFCOUNT];
	WSABUF mBuf[MAX_MBUFCOUNT];
	//char *CmdBuf = "getstate";
	//UINT Cmdlen = strlen(CmdBuf);
	INT i;
	INT err;
	INT rc;
	INT total_agent;
	DWORD currLen;
	DWORD Flags;
	SOCKET socklist[MAX_MBUFCOUNT];

	LARGE_INTEGER frequency;
	LARGE_INTEGER start;
	LARGE_INTEGER next;
	LARGE_INTEGER stop;
	LARGE_INTEGER lap;

	double  elap;

	INT control_loop;
	char *ids_reconn;
	std::string ids_name;
	//DWORD Flags;
	HANDLE hreconn;
	DWORD pidreconn;
	//init

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);

	next.QuadPart = start.QuadPart + COLLECTION_INTERVAL * frequency.QuadPart / 1000;
	control_loop = 1;

	for (i = 0; i < MAX_MBUFCOUNT; ++i)
	{
		eArray[i] = WSACreateEvent();
		if (eArray[i] == NULL)
			printf("WSACreateEvent fail\n");

		ZeroMemory(&olp[i], sizeof(WSAOVERLAPPED));

		olp[i].hEvent = eArray[i];


		mBuf[i].buf = new char[MAXBUFLEN];
		if (mBuf[i].buf == NULL)
			printf("init buffer fail\n");
		mBuf[i].len = MAXBUFLEN;

	}


	do {

		QueryPerformanceCounter(&stop);
		while (stop.QuadPart < next.QuadPart)
		{
			lap.QuadPart = next.QuadPart - stop.QuadPart;
			elap = (double)lap.QuadPart / (double)frequency.QuadPart;

			Sleep((DWORD)(elap * 1000));

			QueryPerformanceCounter(&stop);
		}
		next.QuadPart = start.QuadPart + (control_loop + 1) * COLLECTION_INTERVAL * frequency.QuadPart / 1000;
		++control_loop;

		gCurrentTime = control_loop * 1000;


		//basic timer

		//clear timeout notifications
		UpdateIDSStatus();
		
		UpdateTGStatus();
		
		EnterCriticalSection(&gIDSSocketTable.lock);
		i = 0;
		for (it = gIDSSocketTable.table.begin(); it != gIDSSocketTable.table.end(); ++it)
		{
			BOOL deleting = FALSE;
			for (int j = 0; j < MAX_NUM_VM; ++j)
			{
				if (it->second == g_delete_sock[j])
				{
					deleting = TRUE;
					break;
				}
			}
			if (deleting == TRUE)
				continue;

			socklist[i] = it->second;
			++i;
		}
		LeaveCriticalSection(&gIDSSocketTable.lock);
		
		total_agent = i;
		if (total_agent == 0)
			continue;

		for (i = 0; i < total_agent; ++i)
		{

			//mBuf[i].buf = CmdBuf;
			//mBuf[i].len = Cmdlen;
			Flags = 0;
			rc = WSARecv(socklist[i], &mBuf[i], 1, NULL, &Flags, &olp[i], NULL);
			if ((rc == SOCKET_ERROR) &&
				(WSA_IO_PENDING != (err = WSAGetLastError())))
			{
				//WARN_errno(true, "WSASend");
				printf("WSARecv fail = %d\n", WSAGetLastError());
				closesocket(socklist[i]);

				EnterCriticalSection(&gIDSSocketTable.lock);
				ids_name = GetNamebySock(socklist[i]);
				gIDSSocketTable.table.erase(ids_name);
				LeaveCriticalSection(&gIDSSocketTable.lock);

				EnterCriticalSection(&gIDSStatus.lock);
				gIDSStatus.table.erase(socklist[i]);
				LeaveCriticalSection(&gIDSStatus.lock);
				
				//reconnect
				ids_reconn = const_cast<char *>(ids_name.c_str());
				hreconn = CreateThread(NULL, 0, ReconnectAgent, (LPVOID)ids_reconn, 0, &pidreconn);
				if (hreconn == NULL)
				{
					printf("create reconnect thread fail\n");
					//return -1;
					//return CMD_FAIL;
				}

				//break;
				continue;
			}



		}



		rc = WSAWaitForMultipleEvents(total_agent, eArray, TRUE, COLLECTION_INTERVAL, FALSE);
		if (rc == WSA_WAIT_FAILED)
		{
			printf("Recv WSAWaitForMultipleEvents fail = %d\n", WSAGetLastError());
			continue;
		}

		//printf("WSAWaitForMultipleEvents\n");
		for (i = 0; i < total_agent; ++i)
		{
			rc = WSAGetOverlappedResult(socklist[i], &olp[i], &currLen, FALSE, &Flags);
			if (rc == FALSE)
			{
				if (WSA_IO_INCOMPLETE == (err = WSAGetLastError()))
					continue;

				printf("Recv WSAGetOverlappedResult = %d\n", WSAGetLastError());
				continue;
			}

			//printf("Send command: %d Bytes\n", currLen);
			rc = WSAResetEvent(olp[i].hEvent);
			if (rc == FALSE)
			{
				printf("Recv WSAResetEvent = %d\n", WSAGetLastError());
			}

			mBuf[i].buf[currLen] = '\0';
			//printf("recv: %u\n", strnlen(mBuf[i].buf, MAXBUFLEN));
			ParseState(mBuf[i].buf, socklist[i]);

		}


		DisplayAgentStatus();
		TrafficRedirection();

	} while (true);

	free(mBuf[i].buf);

	return CMD_OK;
}


//
//DWORD WINAPI StartTrafficGenerator(LPVOID paramPtr)
//{
//	UINT idx = 0;
//	UINT interval = 60000;   //60 sec 
//	UINT i;
//	for (i = 0; i < MAX_NUM_TG; i++)
//	{
//		while (gCurrentTime <= i * interval)
//			Sleep(1000);	
//		
//		
//	}
//
//	return CMD_OK;
//}



int start_sdn(int server) {
	//HANDLE mHandle;
	
	char cmd[MAXBUFLEN];
	char* next_token = NULL;
	char* token = " ";
	char* command = NULL;

	DWORD ThreadIDRequest;
	DWORD ThreadIDCollect;
	HANDLE THCollect;
	HANDLE THRequest;
	//connect_para conn_para;

#ifdef FILEIO
	fopen_s(&ofp, "log.txt", "w");

	if (ofp == NULL) {
		fprintf(stderr, "Can't open output file!\n");
		exit(1);
	}
#endif

	g_Heavyhitter.clear();

	FILE *hhfile;
	fopen_s(&hhfile, "E:\\heavyhitters.txt", "r");
	if (hhfile == NULL){
		printf("open file fail\n");
		return 0;
	}
	char line[128];
	char *tkn = "	";
	char *next;
	char *word;
	UINT ipvalue;
	UINT count;
	for (int i = 0; i < 100; ++i)
	{
		if (fgets(line, sizeof line, hhfile) != NULL)
		{
			line[127] = '\0';
			word = strtok_s(line, tkn, &next);
			ipvalue = ip_to_int(word);
			count = atoi(next);
			g_top_groundtruth.insert(std::pair<UINT, UINT>(ipvalue, count));
		}
		
	}

	fclose(hhfile);



	UINT i;
	for (i = 0; i < MAX_NUM_VM; ++i)
		gVM_Pool.push(gVM_list[i].name);     //create vm pool

	THRequest = CreateThread(NULL, 0, DoStateRequest, NULL, 0, &ThreadIDRequest);
	if (THRequest == NULL)
	{

		printf("create request thread fail\n");
		//return -1;
		return CMD_FAIL;
	}
	//THRequest

	THCollect = CreateThread(NULL, 0, DoStateCollection, NULL, 0, &ThreadIDCollect);
	if (THCollect == NULL)
	{

		printf("create collection thread fail\n");
		//return -1;
		return CMD_FAIL;
	}

	for (i = 0; i < MAX_NUM_TG; ++i)
	{
		DoConnectTrafficGenerator(gTG_list[i].name, gTG_list[i].port);
	}


	
	UINT vm_id = 0;
	for (int j = 0; j < MAX_NUM_VM; ++j)
	{
		if (std::string(gVM_list[j].name) == gVM_Pool.front())
		{
			vm_id = j;
			printf("start one vm %d\n", vm_id);
			gVM_Pool.pop();
			break;
		}
	}
	
	std::set<UINT> routeset;
	for (UINT k = 0; k < MAX_NUM_ROUTE/2; ++k)
		routeset.insert(k);
	printf("route size %d\n", routeset.size());

	gVM_route_set.insert(std::pair<UINT,std::set<UINT>>(gVM_list[vm_id].MACidx, routeset));

	//start one
	StartAgent(&vm_id);

	
	vm_id = 0;
	for (int j = 0; j < MAX_NUM_VM; ++j)
	{
		if (std::string(gVM_list[j].name) == gVM_Pool.front())
		{
			vm_id = j;
			printf("start one vm %d\n", vm_id);
			gVM_Pool.pop();
			break;
		}
	}

	std::set<UINT> routeset2;
	for (UINT k = MAX_NUM_ROUTE / 2; k < MAX_NUM_ROUTE; ++k)
		routeset2.insert(k);
	printf("route size %d\n", routeset2.size());
	gVM_route_set.insert(std::pair<UINT, std::set<UINT>>(gVM_list[vm_id].MACidx, routeset2));
	//standby one
	StartAgent(&vm_id);



	//third one
	vm_id = 0;
	for (int j = 0; j < MAX_NUM_VM; ++j)
	{
		if (std::string(gVM_list[j].name) == gVM_Pool.front())
		{
			vm_id = j;
			printf("start one vm %d\n", vm_id);
			gVM_Pool.pop();
			break;
		}
	}
	std::set<UINT> routeset3;
	gVM_route_set.insert(std::pair<UINT, std::set<UINT>>(gVM_list[vm_id].MACidx, routeset3));
	//standby one
	StartAgent(&vm_id);

	
	//printf("vm id %d\n", gVM_idx);

	//Sleep(5000);
	//printf("start traffic\n");
	//DWORD TG_pid;
	//gTTGHandle = CreateThread(NULL, 0, StartTrafficGenerator, NULL, 0, &TG_pid);
	//DoStartTrafficGenerator("204.57.0.30");

	while (1);

#ifdef FILEIO
	fclose(ofp);
#endif

	do
	{

		gets_s(cmd, MAXBUFLEN);
		command = strtok_s(cmd, token, &next_token);

		if (!ft_strcasecmp(command, "connect"))
		{
			//printf("havent connected\n");
			connect_para conn_para;
			//std::string str(AgenetAddr);
			conn_para.AgentAddr = next_token;
			conn_para.ConnectionTO = 3;  //30 sec
			conn_para.type = TYPE_AGENT_IDS;
			conn_para.port = CONTROL_PORT;
			//ConnectAgenet(cmd);
			DoConnectAgent(&conn_para);
			//ConnectAgenet(next_token, TYPE_AGENT_IDS);
		}
		else if (!ft_strcasecmp(command, "start"))
		{
			StartAgent(next_token);
		}
		else if (!ft_strcasecmp(command, "starttg"))
		{
			//DoStartTrafficGenerator(next_token);
		}
		else if (!ft_strcasecmp(command, "stoptg"))
		{
			//DoStopTrafficGenerator(next_token);
		}
		else
		{
			printf("unknown command\n");
			continue;
		}


		//}
		//else if (server == 1) {
		//	gTHandle = CreateThread(NULL, 0, StartClient, NULL, 0, &ThreadID);
		//	if (gTHandle == NULL)
		//		printf("create thread fail\n");
		//}
		//  
		
	} while (true);

	//WaitForSingleObject(gTHandle, INFINITE);

    return 0;
} // end main



//
//DWORD WINAPI StartAgent(LPVOID paramPtr)
//{
//
//	WSABUF CmdBuf;
//	CmdBuf.buf = new char[MAXBUFLEN];
//	//CmdBuf.buf = buf;
//	CmdBuf.len = MAXBUFLEN;
//	DWORD currLen;
//
//	int sock = *(int *)paramPtr;
//	int rc;
//
//	printf("VM %d connected\n", sock);
//
//	do {
//
//		DWORD Flags = 0;
//		rc = WSARecv(sock, &CmdBuf, 1, &currLen, &Flags, NULL, NULL);
//
//		if (rc == SOCKET_ERROR)
//		{
//			printf("rec error: %d\n", GetLastError());
//			break;
//		}
//
//		if (currLen == 0) {
//			printf("connect closed\n");
//			break;
//		}
//
//		if (CmdBuf.buf[0] == 's')
//			printf("get stat\n");
//	} while (true);
//
//	return 0;
//
//}