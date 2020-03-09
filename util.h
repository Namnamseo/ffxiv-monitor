#include <windows.h>

void print_error_and_die();

UINT64 readNet64(BYTE *pt);
UINT32 readNet32(BYTE *pt);
UINT16 readNet16(BYTE *pt);

UINT64 readHost64(BYTE *pt);
UINT32 readHost32(BYTE *pt);
UINT16 readHost16(BYTE *pt);
