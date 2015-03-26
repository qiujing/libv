#include <Windows.h>
#include "Lock.h"

CLock::CLock(CRITICAL_SECTION& lock):cs(lock)
{  
    EnterCriticalSection(&cs);  
}  


CLock::~CLock(void)
{
	LeaveCriticalSection(&cs);
}
