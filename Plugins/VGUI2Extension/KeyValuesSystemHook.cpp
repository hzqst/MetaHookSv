#include <metahook.h>
#include <IKeyValuesSystem.h>

extern IKeyValuesSystem *g_pKeyValuesSystem;

static void (__fastcall *g_pfnRegisterSizeofKeyValues)(void *pthis, int edx, int size) = NULL;
static void *(__fastcall *g_pfnAllocKeyValuesMemory)(void *pthis, int edx, int size) = NULL;
static void (__fastcall *g_pfnFreeKeyValuesMemory)(void *pthis, int edx, void *pMem) = NULL;
static HKeySymbol (__fastcall *g_pfnGetSymbolForString)(void *pthis, int edx, const char *name) = NULL;
static const char *(__fastcall *g_pfnGetStringForSymbol)(void *pthis, int edx, HKeySymbol symbol) = NULL;
static void (__fastcall *g_pfnGetLocalizedFromANSI)(void *pthis, int edx, const char *ansi, wchar_t *outBuf, int unicodeBufferSizeInBytes) = NULL;
static void (__fastcall *g_pfnGetANSIFromLocalized)(void *pthis, int edx, const wchar_t *wchar, char *outBuf, int ansiBufferSizeInBytes) = NULL;
static void (__fastcall *g_pfnAddKeyValuesToMemoryLeakList)(void *pthis, int edx, void *pMem, HKeySymbol name) = NULL;
static void (_fastcall *g_pfnRemoveKeyValuesFromMemoryLeakList)(void *pthis, int edx, void *pMem) = NULL;

//One VFTHook per IKeyValuesSystem slot installed by KeyValuesSystem_InstallHooks;
//kept so KeyValuesSystem_UninstallHooks can UnHook them. Only the alloc/free slots
//are proxied today; the array is sized for the full interface so the remaining
//slots can be enabled without resizing it.
static hook_t* g_phook_CKeyValuesSystem[10] = { NULL };

class CKeyValuesSystemProxy : public IKeyValuesSystem
{
public:
	void RegisterSizeofKeyValues(int size) override;
	void *AllocKeyValuesMemory(int size) override;
	void FreeKeyValuesMemory(void* pMem) override;
	HKeySymbol GetSymbolForString(const char *name) override;
	const char *GetStringForSymbol(HKeySymbol symbol) override;
	void GetLocalizedFromANSI(const char *ansi, wchar_t *outBuf, int unicodeBufferSizeInBytes) override;
	void GetANSIFromLocalized(const wchar_t *wchar, char *outBuf, int ansiBufferSizeInBytes) override;
	void AddKeyValuesToMemoryLeakList(void *pMem, HKeySymbol name) override;
	void RemoveKeyValuesFromMemoryLeakList(void *pMem) override;
};

void CKeyValuesSystemProxy::RegisterSizeofKeyValues(int size)
{
	return g_pfnRegisterSizeofKeyValues(this, 0, size);
}

void * CKeyValuesSystemProxy::AllocKeyValuesMemory(int size)
{
	return malloc(size);
}

void CKeyValuesSystemProxy::FreeKeyValuesMemory(void *pMem)
{
	return free(pMem);
}

HKeySymbol CKeyValuesSystemProxy::GetSymbolForString(const char *name)
{
	return g_pfnGetSymbolForString(this, 0, name);
}

const char * CKeyValuesSystemProxy::GetStringForSymbol(HKeySymbol symbol)
{
	return g_pfnGetStringForSymbol(this, 0, symbol);
}

void CKeyValuesSystemProxy::GetLocalizedFromANSI(const char *ansi, wchar_t *outBuf, int unicodeBufferSizeInBytes)
{
	return g_pfnGetLocalizedFromANSI(this, 0, ansi, outBuf, unicodeBufferSizeInBytes);
}

void CKeyValuesSystemProxy::GetANSIFromLocalized(const wchar_t *wchar, char *outBuf, int ansiBufferSizeInBytes)
{
	return g_pfnGetANSIFromLocalized(this, 0, wchar, outBuf, ansiBufferSizeInBytes);
}

void CKeyValuesSystemProxy::AddKeyValuesToMemoryLeakList(void *pMem, HKeySymbol name)
{
	return g_pfnAddKeyValuesToMemoryLeakList(this, 0, pMem, name);
}

void CKeyValuesSystemProxy::RemoveKeyValuesFromMemoryLeakList(void *pMem)
{
	return g_pfnRemoveKeyValuesFromMemoryLeakList(this, 0, pMem);
}

static CKeyValuesSystemProxy s_KeyValuesSystemProxy;

void KeyValuesSystem_InstallHooks(void)
{
	PVOID *pVFTable = *(PVOID **)&s_KeyValuesSystemProxy;

	//Only the alloc/free slots are proxied today; the other entries stay commented
	//out and keep the engine's originals. Each installed hook is saved so
	//KeyValuesSystem_UninstallHooks can restore the vftable.
	//g_phook_CKeyValuesSystem[1] = g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 1, (void *)pVFTable[1], (void **)&g_pfnRegisterSizeofKeyValues);
	g_phook_CKeyValuesSystem[2] = g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 2, (void *)pVFTable[2], (void **)&g_pfnAllocKeyValuesMemory);
	g_phook_CKeyValuesSystem[3] = g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 3, (void *)pVFTable[3], (void **)&g_pfnFreeKeyValuesMemory);
	//g_phook_CKeyValuesSystem[4] = g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 4, (void *)pVFTable[4], (void **)&g_pfnGetSymbolForString);
	//g_phook_CKeyValuesSystem[5] = g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 5, (void *)pVFTable[5], (void **)&g_pfnGetStringForSymbol);
	//g_phook_CKeyValuesSystem[6] = g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 6, (void *)pVFTable[6], (void **)&g_pfnGetLocalizedFromANSI);
	//g_phook_CKeyValuesSystem[7] = g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 7, (void *)pVFTable[7], (void **)&g_pfnGetANSIFromLocalized);
	//g_phook_CKeyValuesSystem[8] = g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 8, (void *)pVFTable[8], (void **)&g_pfnAddKeyValuesToMemoryLeakList);
	//g_phook_CKeyValuesSystem[9] = g_pMetaHookAPI->VFTHook(g_pKeyValuesSystem, 0, 9, (void *)pVFTable[9], (void **)&g_pfnRemoveKeyValuesFromMemoryLeakList);
}

void KeyValuesSystem_UninstallHooks(void)
{
	//Restores the IKeyValuesSystem vftable entries the proxy replaced.
	//g_pKeyValuesSystem stays cached; only the hooks are dropped.
	for (int i = 1; i < _ARRAYSIZE(g_phook_CKeyValuesSystem); ++i)
	{
		if (g_phook_CKeyValuesSystem[i])
		{
			g_pMetaHookAPI->UnHook(g_phook_CKeyValuesSystem[i]);
			g_phook_CKeyValuesSystem[i] = NULL;
		}
	}
}