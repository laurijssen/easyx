#define WIN32_LEAN_AND_MEAN
#include <windows.h>

HINSTANCE dllInstance;
BOOL ActiveApp;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	if( dwReason == DLL_PROCESS_ATTACH )
		dllInstance = (HINSTANCE)hModule;

    return TRUE;
}
