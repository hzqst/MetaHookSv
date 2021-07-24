#include <metahook.h>
#include <vstdlib/IKeyValuesSystem.h>

extern IKeyValuesSystem *g_pKeyValuesSystem;

void (__fastcall *g_pfnRegisterSizeofKeyValues)(void *pthis, int edx, int size) = NULL;
void *(__fastcall *g_pfnAllocKeyValuesMemory)(void *pthis, int edx, int size) = NULL;
void (__fastcall *g_pfnFreeKeyValuesMemory)(void *pthis, int edx, void *pMem) = NULL;
HKeySymbol (__fastcall *g_pfnGetSymbolForString)(void *pthis, int edx, const char *name) = NULL;
const char *(__fastcall *g_pfnGetStringForSymbol)(void *pthis, int edx, HKeySymbol symbol) = NULL;
void (__fastcall *g_pfnGetLocalizedFromANSI)(void *pthis, int edx, const char *ansi, wchar_t *outBuf, int unicodeBufferSizeInBytes) = NULL;
void (__fastcall *g_pfnGetANSIFromLocalized)(void *pthis, int edx, const wchar_t *wchar, char *outBuf, int ansiBufferSizeInBytes) = NULL;
void (__fastcall *g_pfnAddKeyValuesToMemoryLeakList)(void *pthis, int edx, void *pMem, HKeySymbol name) = NULL;
void (_fastcall *g_pfnRemoveKeyValuesFromMemoryLeakList)(void *pthis, int edx, void *pMem) = NULL;

class CKeyValuesSystem : public IKeyValuesSystem
{
public:
	virtual void RegisterSizeofKeyValues(int size);
	virtual void *AllocKeyValuesMemory(int size);
	virtual void FreeKeyValuesMemory(void *pMem);
	virtual HKeySymbol GetSymbolForString(const char *name);
	virtual const char *GetStringForSymbol(HKeySymbol symbol);
	virtual void GetLocalizedFromANSI(const char *ansi, wchar_t *outBuf, int unicodeBufferSizeInBytes);
	virtual void GetANSIFromLocalized(const wchar_t *wchar, char *outBuf, int ansiBufferSizeInBytes);
	virtual void AddKeyValuesToMemoryLeakList(void *pMem, HKeySymbol name);
	virtual void RemoveKeyValuesFromMemoryLeakList(void *pMem);
};

void CKeyValuesSystem::RegisterSizeofKeyValues(int size)
{
	return g_pfnRegisterSizeofKeyValues(this, 0, size);
}

void *CKeyValuesSystem::AllocKeyValuesMemory(int size)
{
	return malloc(size);
}

void CKeyValuesSystem::FreeKeyValuesMemory(void *pMem)
{
	return free(pMem);
}

HKeySymbol CKeyValuesSystem::GetSymbolForString(const char *name)
{
	return g_pfnGetSymbolForString(this, 0, name);
}

const char *CKeyValuesSystem::GetStringForSymbol(HKeySymbol symbol)
{
	return g_pfnGetStringForSymbol(this, 0, symbol);
}

void CKeyValuesSystem::GetLocalizedFromANSI(const char *ansi, wchar_t *outBuf, int unicodeBufferSizeInBytes)
{
	return g_pfnGetLocalizedFromANSI(this, 0, ansi, outBuf, unicodeBufferSizeInBytes);
}

void CKeyValuesSystem::GetANSIFromLocalized(const wchar_t *wchar, char *outBuf, int ansiBufferSizeInBytes)
{
	return g_pfnGetANSIFromLocalized(this, 0, wchar, outBuf, ansiBufferSizeInBytes);
}

void CKeyValuesSystem::AddKeyValuesToMemoryLeakList(void *pMem, HKeySymbol name)
{
	return g_pfnAddKeyValuesToMemoryLeakList(this, 0, pMem, name);
}

void CKeyValuesSystem::RemoveKeyValuesFromMemoryLeakList(void *pMem)
{
	return g_pfnRemoveKeyValuesFromMemoryLeakList(this, 0, pMem);
}

void KeyValuesSystem_InstallHook(void)
{
	CKeyValuesSystem KeyValuesSystem;
	DWORD *pVFTable = *(DWORD **)&KeyValuesSystem;

	//g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 1, (void *)pVFTable[1], (void **)&g_pfnRegisterSizeofKeyValues);
	g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 2, (void *)pVFTable[2], (void **)&g_pfnAllocKeyValuesMemory);
	g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 3, (void *)pVFTable[3], (void **)&g_pfnFreeKeyValuesMemory);
	//g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 4, (void *)pVFTable[4], (void **)&g_pfnGetSymbolForString);
	//g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 5, (void *)pVFTable[5], (void **)&g_pfnGetStringForSymbol);
	//g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 6, (void *)pVFTable[6], (void **)&g_pfnGetLocalizedFromANSI);
	//g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 7, (void *)pVFTable[7], (void **)&g_pfnGetANSIFromLocalized);
	//g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 8, (void *)pVFTable[8], (void **)&g_pfnAddKeyValuesToMemoryLeakList);
	//g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 9, (void *)pVFTable[9], (void **)&g_pfnRemoveKeyValuesFromMemoryLeakList);
}