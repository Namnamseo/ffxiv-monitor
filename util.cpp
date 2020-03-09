#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

void print_error_and_die()
{
	printf("error code %d\n", GetLastError());
	exit(1);
}

UINT64 readNet64(BYTE *pt)
{
	UINT64 ret = 0;
	for(int i=0; i<8; ++i) {
		ret = (ret << 8) | (*pt);
		++pt;
	}
	return ret;
}

UINT32 readNet32(BYTE *pt)
{
	UINT32 ret = 0;
	for(int i=0; i<4; ++i) {
		ret = (ret << 8) | (*pt);
		++pt;
	}
	return ret;
}

UINT16 readNet16(BYTE *pt)
{
	UINT16 ret = 0;
	for(int i=0; i<2; ++i) {
		ret = (ret << 8) | (*pt);
		++pt;
	}
	return ret;
}

UINT64 readHost64(BYTE *pt)
{
	return *(UINT64 *)pt;
}

UINT32 readHost32(BYTE *pt)
{
	return *(UINT32 *)pt;
}

UINT16 readHost16(BYTE *pt)
{
	return *(UINT16 *)pt;
}
