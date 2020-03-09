#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <stdio.h>
#include <stdlib.h>

#include "detect.h"
#include "monitor.h"

#pragma comment(lib, "Ws2_32.lib")

static void _init_winsock()
{
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		exit(1);
	}
}

static void _print_ip(DWORD addr, DWORD port)
{
	printf("%d.%d.%d.%d:%u",
		(addr      ) & 0xFF,
		(addr >>  8) & 0xFF,
		(addr >> 16) & 0xFF,
		(addr >> 24),
		port
	);
}

DWORD dwThreadIds[MAX_CONN];
HANDLE hThreads[MAX_CONN];

static void _report_conns()
{
	printf("Found %d connections:\n", nConn);
	for (int i = 0; i < nConn; ++i) {
		printf("... %2d: ", i);

		_print_ip(connList[i].localAddr, connList[i].localPort);

		printf(" -> ");

		_print_ip(connList[i].remoteAddr, connList[i].remotePort);
		
		printf("\n");
	}
}

static void _spawn_threads()
{
	for (int i = 0; i < nConn; ++i) {
		hThreads[i] = CreateThread(
			NULL, // default security
			0,    // default stack size
			(LPTHREAD_START_ROUTINE) start_monitor, // func
			(LPVOID) (LONGLONG) i,    // arg
			0,             // (flags)
			&dwThreadIds[i]
		);

		if (hThreads[i] == NULL) {
			printf("CreateThread() for conn. id %d has failed;\n\terror code 0x%08x",
				i, GetLastError());
			exit(1);
		}
	}
}

int main()
{
	_init_winsock();

	find_ffxiv_connection();

	_report_conns();

	_spawn_threads();

	DWORD res;

	res = WaitForMultipleObjects(nConn, hThreads, TRUE, INFINITE);
	if (res == WAIT_FAILED) {
		printf("WaitForMultipleObjects() failed with error code 0x%08x\n",
			GetLastError());
	}

	printf("Wait done.(res %08x=%u.) Now reaping...\n", res, res);

	for(int i=0; i<nConn; i++)
	{
		CloseHandle(hThreads[i]);		
	}

	printf("Reaping done.\n");

	WSACleanup();

	return 0;
}