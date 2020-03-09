#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <iphlpapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "detect.h"
#include "monitor.h"
#include "parse.h"
#include "const.h"

int sock[MAX_CONN];

static struct sockaddr_in _get_sockaddr(DWORD addr, DWORD port)
{
	sockaddr_in ret;
	memset(&ret, 0, sizeof(ret));
	ret.sin_family = AF_INET;
	ret.sin_port = port;
	ret.sin_addr.s_addr = addr;
	return ret;
}

static void _enable_listen_all(int s)
{
	RCVALL_VALUE lpvInBuffer = RCVALL_IPLEVEL;
	DWORD recv;

	int res = WSAIoctl(
		s, // socket
		SIO_RCVALL,           // dwIoControlCode
		&lpvInBuffer,          // lpvInBuffer
		sizeof(lpvInBuffer),   // cbInBuffer
		NULL,                  // lpvOutBuffer
		(DWORD) 0,             // cbOutBuffer
		&recv,                 // lpcbBytesReturned
		NULL,                  // lpOverlapped
		NULL                   // lpCompletionRoutine
	);

	if (res == SOCKET_ERROR) {
		printf("WSAIoctl() failed: error code %8x\n", res);
		exit(1);
	}
}

static int _setup_socket(int i)
{
	int res;
	int s;

	printf("hi! setting up socket %d\n", i);

	s = socket(AF_INET, SOCK_RAW, IPPROTO_IP);

	if (s == INVALID_SOCKET) {
		printf("conn %d: socket() failed:\n\terror code %d\n",
			i, WSAGetLastError());
		exit(1);
	}

	struct sockaddr_in addr_local;
	addr_local = _get_sockaddr(connList[i].localAddr, connList[i].localPort);
	res = bind(s, (const sockaddr *) &addr_local, sizeof(addr_local));
	if (res == SOCKET_ERROR) {
		printf("conn %d: bind() failed:\n\terror code %d\n",
			i, WSAGetLastError());
		exit(1);
	}

	_enable_listen_all(s);

	struct sockaddr_in addr_remote;
	addr_remote = _get_sockaddr(connList[i].remoteAddr, connList[i].remotePort);
	res = connect(s, (const sockaddr *) &addr_remote, sizeof(addr_remote));
	if (res == SOCKET_ERROR) {
		printf("conn %d: connect() failed:\n\terror code %d\n",
			i, WSAGetLastError());
		exit(1);
	}

	return s;
}

static void _destroy_socket(int i, int s)
{
	int res;

	res = closesocket(s);
	if (res == SOCKET_ERROR) {
		printf("conn %d: closesocket() failed: error code %d\n",
			i, WSAGetLastError());
		// exit(1);
	}
}

DWORD start_monitor(LPVOID idx_pt)
{
	printf("hi! setting up.\n");

	int idx = (LONGLONG) idx_pt;

	int s = _setup_socket(idx);

	printf("setup socket for idx=%d done.\n", idx);

	void *buf = new char[BUFSIZE];

	prepare_parse();

	while(true)
	{
		int res = recv(s, (char *) buf, BUFSIZE, 0);

		if (res == SOCKET_ERROR) {
			int err = WSAGetLastError();
			printf("conn %d: recv() failed: error code 0x%08x (%u)\n",
				idx, err, err);
			break;
		}

		parse_ip_packet(idx, (BYTE *) buf, res);
	}

	delete[] buf;

	_destroy_socket(idx, s);

	return 0;
}