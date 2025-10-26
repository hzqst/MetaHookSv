#pragma once

typedef struct walk_context_s
{
	walk_context_s(void* a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	void* address;
	size_t len;
	int depth;
}walk_context_t;

typedef struct
{
	xcommand_t Connect_f;

	xcommand_t Retry_f;

	void(__fastcall* CClient_SoundEngine_FlushCache)(int pthis, int dummy, qboolean including_local);

	int (*pfnClientCmd)(const char* szCmdString);

}private_funcs_t;

extern private_funcs_t gPrivateFuncs;

void __fastcall CClient_SoundEngine_FlushCache(int pthis, int dummy, qboolean including_local);

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Client_InstallHooks(void);
void Client_UninstallHooks(void);

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo);
