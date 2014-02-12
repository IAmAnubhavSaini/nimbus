#ifndef _FTCMD_H_
#define _FTCMD_H_

#include <windows.h>
#include <iostream>
#include <list>
#include <vector>
#include <map>

#include "../controller/sdncontrol.h"
#pragma comment(lib, "ws2_32")

#define FT_SUCCESS                0
#define FT_FAIL                   1

//#define CONN_TABLE_SIZE             100000



    


typedef enum cmd_result_e {
	CMD_OK = 0,            /* Command completed successfully */
	CMD_FAIL = -1,            /* Command failed */
	CMD_USAGE = -2,            /* Command failed, print usage  */
	CMD_NFND = -3,            /* Command not found */
	CMD_EXIT = -4,            /* Exit current shell level */
	CMD_INTR = -5,            /* Command interrupted */
	CMD_NOTIMPL = -6            /* Command not implemented */
} cmd_result_t;

typedef char *parse_key_t;
#define ARGS_CNT        1024            /* Max argv entries */
#define ARGS_BUFFER     4096            /* # bytes total for arguments */

typedef struct args_s {
	parse_key_t a_cmd;                  /* Initial string */
	char        *a_argv[ARGS_CNT];      /* argv pointers */
	char        a_buffer[ARGS_BUFFER];  /* Split up buffer */
	int         a_argc;                 /* Parsed arg counter */
	int         a_arg;                  /* Pointer to NEXT arg */
} args_t;

typedef struct _agent_state{
	UINT lastupdatetime;   // in millisecond
	UINT lastcleantime;    // in millisecond
	SOCKET agent_sock;

	UINT fwdpps;
	UINT fwdmbs;
	
	UINT processpackets;
	UINT processbytes;

	UINT samplepps;
	UINT samplembs;
	UINT inboundpps;

	UINT userusage;
	UINT kernelusage;
	UINT totalusage;
	UINT smoothusage;

	UINT memusage;

	UINT countconn;
	UINT countflow;

	UINT totalsamples;
	
} AGENT_STATE;

class SystemUsage{
public:
	SystemUsage()
	{
		idleTime = { 0, 0 };
		kernelTime = { 0, 0 };
		userTime = { 0, 0 };
		last_idleTime = { 0, 0 };
		last_kernelTime = { 0, 0 };
		last_userTime = { 0, 0 };
		unit = pow(2, sizeof(DWORD)* 8);

		memusage = 0; 
	};

	~SystemUsage(){};
	INT GetCPUUsage(INT *user, INT *kernel, INT *total);
	INT GetMemUsage(UINT *proc);

private:
	FILETIME idleTime;
	FILETIME kernelTime;
	FILETIME userTime;

	FILETIME last_idleTime;
	FILETIME last_kernelTime;
	FILETIME last_userTime;

	size_t memusage;   //KB

	double unit;


};



#define _ARG_GET(_a)     \
	((_a)->a_argv[(_a)->a_arg++])

#define ARG_GET(_a)     \
	(((_a)->a_arg >= (_a)->a_argc) ? NULL : _ARG_GET(_a))







#endif