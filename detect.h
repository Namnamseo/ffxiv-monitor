#include <windows.h>

const LPCWSTR WINDOW_NAME = L"FINAL FANTASY XIV";
const int MAX_CONN = 10;

void find_ffxiv_connection();

typedef struct _CONN {
	DWORD localAddr;
	DWORD localPort;
	DWORD remoteAddr;
	DWORD remotePort;
} CONN;

extern int nConn;
extern CONN connList[];