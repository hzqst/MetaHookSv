#include <windows.h>
#include "Detours\detours.h"

typedef int (CALLBACK *fnWinMain)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

void HookOEP(void)
{
	HMODULE hGameModule = GetModuleHandle(NULL);
	PBYTE PEHead = (PBYTE)hGameModule;
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hGameModule;
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)&PEHead[pDosHeader->e_lfanew];
	fnWinMain pWinMain = (fnWinMain)&PEHead[pNTHeader->OptionalHeader.AddressOfEntryPoint];

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pWinMain, WinMain);
	DetourTransactionCommit();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			HookOEP();
		}
	}

	return TRUE;
}