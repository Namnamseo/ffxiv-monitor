#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <winuser.h>

#include <stdio.h>
#include <stdlib.h>

#include "detect.h"
#include "util.h"

#pragma comment(lib, "Ws2_32.lib")

int nConn;
CONN connList[MAX_CONN];

static PMIB_TCPTABLE_OWNER_PID _get_tcp_table()
{
	void *table = NULL;
	DWORD size = 0;
	DWORD res = NO_ERROR;
	for (int i = 0; i < 5; ++i) {
		res = GetExtendedTcpTable(
			table,
			&size,
			FALSE,
			AF_INET,
			TCP_TABLE_OWNER_PID_ALL,
			0UL
		);

		if (res == 0)
			break;

		if (res == ERROR_INVALID_PARAMETER) {
			printf("GetExtendedTcpTable failed: ERROR_INVALID_PARAMETER\n");
			exit(1);
		}
		if (table != NULL) {
			delete[] table;
			table = NULL;
		}
		table = new BYTE[size];
	}

	if (res != NO_ERROR || table == NULL)
	{
		printf("GetExtendedTcpTable failed after 5 tries: last call result %d\n", res);
		exit(1);
	}

	return (PMIB_TCPTABLE_OWNER_PID) table;
}

static DWORD find_ffxiv_pid_by_window_name()
{
	HWND hWnd = FindWindowW(NULL, WINDOW_NAME);

	if (hWnd == NULL) {
		printf("FindWindowW failed: ");
		print_error_and_die();
	}

	DWORD pid;
	GetWindowThreadProcessId(hWnd, &pid);

	printf("FFXIV window found with pid=%u.\n", pid);

	return pid;
}

static bool is_same_conn(CONN *conn, PMIB_TCPROW_OWNER_PID row)
{
	return
		(conn->localAddr == row->dwLocalAddr) &&
		(conn->localPort == row->dwLocalPort) &&
		(conn->remoteAddr == row->dwRemoteAddr) &&
		(conn->remotePort == row->dwRemotePort);
}

void find_ffxiv_connection()
{
	DWORD pid = find_ffxiv_pid_by_window_name();

	PMIB_TCPTABLE_OWNER_PID table = _get_tcp_table();
	nConn = 0;

	DWORD nEntries = table->dwNumEntries;
	for (int i = 0; i < nEntries; ++i) {
		PMIB_TCPROW_OWNER_PID row = &table->table[i];

		if (row->dwOwningPid != pid) continue;

		bool exist = false;
		for (int j = 0; j < nConn; ++j) {
			if (is_same_conn(connList, row)) {
				exist = true;
				break;
			}
		}

		if (exist) continue;

		connList[nConn].localAddr = row->dwLocalAddr;
		connList[nConn].localPort = row->dwLocalPort;
		connList[nConn].remoteAddr = row->dwRemoteAddr;
		connList[nConn].remotePort = row->dwRemotePort;
		++nConn;

		if (nConn == MAX_CONN) break;
	}

	delete[] table;

	// The game makes 2 connections and both yield the same packets,
	// so safely ignore them.

	if (nConn > 1)
		nConn = 1;
}
