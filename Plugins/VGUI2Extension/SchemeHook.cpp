#include <metahook.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface2.h>
#include "Scheme2.h"
#include "plugins.h"
#include <intrin.h>

using namespace vgui;

#define CLIENTUI_USE_640_480_PROPBASE

extern bool g_IsNativeClientUIHDProportional;

extern vgui::ISurface2* g_pVGuiSurface2;
extern vgui::ISchemeManager2 * g_pVGuiSchemeManager2;
extern vgui::ISchemeManager  * g_pSchemeManager;
extern vgui::ISchemeManager_HL25* g_pSchemeManager_HL25;

/*
========================================================
GoldSrc Scheme Manager hook proxy
========================================================
*/

static HScheme(__fastcall* m_pfnLoadSchemeFromFile)(void* pthis, int, const char* fileName, const char* tag) = NULL;
static void(__fastcall* m_pfnReloadSchemes)(void* pthis, int) = NULL;
static HScheme(__fastcall* m_pfnGetDefaultScheme)(void* pthis, int) = NULL;
static HScheme(__fastcall* m_pfnGetScheme)(void* pthis, int, const char* tag) = NULL;
static IImage* (__fastcall* m_pfnGetImage)(void* pthis, int, const char* imageName, bool hardwareFiltered) = NULL;
static HTexture(__fastcall* m_pfnGetImageID)(void* pthis, int, const char* imageName, bool hardwareFiltered) = NULL;
static IScheme* (__fastcall* m_pfnGetIScheme)(void* pthis, int, HScheme scheme) = NULL;
static void(__fastcall* m_pfnShutdown)(void* pthis, int, bool full) = NULL;
static int(__fastcall* m_pfnGetProportionalScaledValue)(void* pthis, int, int normalizedValue) = NULL;
static int(__fastcall* m_pfnGetProportionalNormalizedValue)(void* pthis, int, int scaledValue) = NULL;

class CSchemeManagerProxy : public ISchemeManager
{
public:
	virtual HScheme LoadSchemeFromFile(const char *fileName, const char *tag);
	virtual void ReloadSchemes(void);
	virtual HScheme GetDefaultScheme(void);
	virtual HScheme GetScheme(const char *tag);
	virtual IImage *GetImage(const char *imageName, bool hardwareFiltered);
	virtual HTexture GetImageID(const char *imageName, bool hardwareFiltered);
	virtual IScheme *GetIScheme(HScheme scheme);
	virtual void Shutdown(bool full = true);
	virtual int GetProportionalScaledValue(int normalizedValue);
	virtual int GetProportionalNormalizedValue(int scaledValue);
};

HScheme CSchemeManagerProxy::LoadSchemeFromFile(const char *fileName, const char *tag)
{
	return g_pVGuiSchemeManager2->LoadSchemeFromFile(fileName, tag);
}

void CSchemeManagerProxy::ReloadSchemes(void)
{
	g_pVGuiSchemeManager2->ReloadSchemes();
}

HScheme CSchemeManagerProxy::GetDefaultScheme(void)
{
	return g_pVGuiSchemeManager2->GetDefaultScheme();
}

HScheme CSchemeManagerProxy::GetScheme(const char *tag)
{
	return g_pVGuiSchemeManager2->GetScheme(tag);
}

IImage *CSchemeManagerProxy::GetImage(const char *imageName, bool hardwareFiltered)
{
	return g_pVGuiSchemeManager2->GetImage(imageName, hardwareFiltered);
}

HTexture CSchemeManagerProxy::GetImageID(const char *imageName, bool hardwareFiltered)
{
	return g_pVGuiSchemeManager2->GetImageID(imageName, hardwareFiltered);
}

IScheme *CSchemeManagerProxy::GetIScheme(HScheme scheme)
{
	return g_pVGuiSchemeManager2->GetIScheme(scheme);
}

void CSchemeManagerProxy::Shutdown(bool full)
{
	g_pVGuiSchemeManager2->Shutdown(full);
}

int CSchemeManagerProxy::GetProportionalScaledValue(int normalizedValue)
{
#ifdef CLIENTUI_USE_640_480_PROPBASE
	auto retaddr = (PUCHAR)_ReturnAddress();

	if (g_pVGuiSurface2->IsForcingHDProportional() && !g_IsNativeClientUIHDProportional && retaddr > g_dwClientTextBase && retaddr < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		g_pVGuiSurface2->SetForcingHDProportional(false);

		auto r = g_pVGuiSchemeManager2->GetProportionalScaledValue(normalizedValue);

		g_pVGuiSurface2->SetForcingHDProportional(true);

		return r;
	}
#endif

	return g_pVGuiSchemeManager2->GetProportionalScaledValue(normalizedValue);
}

int CSchemeManagerProxy::GetProportionalNormalizedValue(int scaledValue)
{
#ifdef CLIENTUI_USE_640_480_PROPBASE
	auto retaddr = (PUCHAR)_ReturnAddress();

	if (g_pVGuiSurface2->IsForcingHDProportional() && !g_IsNativeClientUIHDProportional && retaddr > g_dwClientTextBase && retaddr < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		g_pVGuiSurface2->SetForcingHDProportional(false);

		auto r = g_pVGuiSchemeManager2->GetProportionalNormalizedValue(scaledValue);

		g_pVGuiSurface2->SetForcingHDProportional(true);

		return r;
	}
#endif

	return g_pVGuiSchemeManager2->GetProportionalNormalizedValue(scaledValue);
}

static CSchemeManagerProxy g_SchemeProxy;

/*
========================================================
GoldSrc_HL25 Scheme Manager hook proxy
========================================================
*/

static float(__fastcall* m_pfnGetProportionalScale)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnGetHDProportionalScaledValue)(void* pthis, int, int normalizedValue) = NULL;
static int(__fastcall* m_pfnGetHDProportionalNormalizedValue)(void* pthis, int, int scaledValue) = NULL;

class CSchemeManagerProxy_HL25 : public ISchemeManager_HL25
{
public:
	virtual HScheme LoadSchemeFromFile(const char *fileName, const char *tag);
	virtual void ReloadSchemes(void);
	virtual HScheme GetDefaultScheme(void);
	virtual HScheme GetScheme(const char *tag);
	virtual IImage *GetImage(const char *imageName, bool hardwareFiltered);
	virtual HTexture GetImageID(const char *imageName, bool hardwareFiltered);
	virtual IScheme *GetIScheme(HScheme scheme);
	virtual void Shutdown(bool full = true);
	virtual int GetProportionalScaledValue(int normalizedValue);
	virtual int GetProportionalNormalizedValue(int scaledValue);
	virtual float GetProportionalScale(void);
	virtual int GetHDProportionalScaledValue(int normalizedValue);
	virtual int GetHDProportionalNormalizedValue(int scaledValue);
};

HScheme CSchemeManagerProxy_HL25::LoadSchemeFromFile(const char *fileName, const char *tag)
{
	return g_pVGuiSchemeManager2->LoadSchemeFromFile(fileName, tag);
}

void CSchemeManagerProxy_HL25::ReloadSchemes(void)
{
	g_pVGuiSchemeManager2->ReloadSchemes();
}

HScheme CSchemeManagerProxy_HL25::GetDefaultScheme(void)
{
	return g_pVGuiSchemeManager2->GetDefaultScheme();
}

HScheme CSchemeManagerProxy_HL25::GetScheme(const char *tag)
{
	return g_pVGuiSchemeManager2->GetScheme(tag);
}

IImage *CSchemeManagerProxy_HL25::GetImage(const char *imageName, bool hardwareFiltered)
{
	return g_pVGuiSchemeManager2->GetImage(imageName, hardwareFiltered);
}

HTexture CSchemeManagerProxy_HL25::GetImageID(const char *imageName, bool hardwareFiltered)
{
	return g_pVGuiSchemeManager2->GetImageID(imageName, hardwareFiltered);
}

IScheme *CSchemeManagerProxy_HL25::GetIScheme(HScheme scheme)
{
	return g_pVGuiSchemeManager2->GetIScheme(scheme);
}

void CSchemeManagerProxy_HL25::Shutdown(bool full)
{
	g_pVGuiSchemeManager2->Shutdown(full);
}

int CSchemeManagerProxy_HL25::GetProportionalScaledValue(int normalizedValue)
{
#ifdef CLIENTUI_USE_640_480_PROPBASE
	auto retaddr = (PUCHAR)_ReturnAddress();

	if (g_pVGuiSurface2->IsForcingHDProportional() && !g_IsNativeClientUIHDProportional && retaddr > g_dwClientTextBase && retaddr < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		g_pVGuiSurface2->SetForcingHDProportional(false);

		auto r = g_pVGuiSchemeManager2->GetProportionalScaledValue(normalizedValue);

		g_pVGuiSurface2->SetForcingHDProportional(true);

		return r;
	}
#endif

	return g_pVGuiSchemeManager2->GetProportionalScaledValue(normalizedValue);
}

int CSchemeManagerProxy_HL25::GetProportionalNormalizedValue(int scaledValue)
{
#ifdef CLIENTUI_USE_640_480_PROPBASE
	auto retaddr = (PUCHAR)_ReturnAddress();

	if (g_pVGuiSurface2->IsForcingHDProportional() && !g_IsNativeClientUIHDProportional && retaddr > g_dwClientTextBase && retaddr < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		g_pVGuiSurface2->SetForcingHDProportional(false);

		auto r = g_pVGuiSchemeManager2->GetProportionalNormalizedValue(scaledValue);

		g_pVGuiSurface2->SetForcingHDProportional(true);

		return r;
	}
#endif

	return g_pVGuiSchemeManager2->GetProportionalNormalizedValue(scaledValue);
}

float CSchemeManagerProxy_HL25::GetProportionalScale(void)
{
#ifdef CLIENTUI_USE_640_480_PROPBASE
	auto retaddr = (PUCHAR)_ReturnAddress();

	if (g_pVGuiSurface2->IsForcingHDProportional() && !g_IsNativeClientUIHDProportional && retaddr > g_dwClientTextBase && retaddr < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		g_pVGuiSurface2->SetForcingHDProportional(false);

		auto r = g_pVGuiSchemeManager2->GetProportionalScale();

		g_pVGuiSurface2->SetForcingHDProportional(true);

		return r;
	}
#endif

	return g_pVGuiSchemeManager2->GetProportionalScale();
}

int CSchemeManagerProxy_HL25::GetHDProportionalScaledValue(int normalizedValue)
{
#ifdef CLIENTUI_USE_640_480_PROPBASE
	auto retaddr = (PUCHAR)_ReturnAddress();

	if (g_pVGuiSurface2->IsForcingHDProportional() && !g_IsNativeClientUIHDProportional && retaddr > g_dwClientTextBase && retaddr < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		g_pVGuiSurface2->SetForcingHDProportional(false);

		auto r = g_pVGuiSchemeManager2->GetHDProportionalScaledValue(normalizedValue);

		g_pVGuiSurface2->SetForcingHDProportional(true);

		return r;
	}
#endif

	return g_pVGuiSchemeManager2->GetHDProportionalScaledValue(normalizedValue);
}

int CSchemeManagerProxy_HL25::GetHDProportionalNormalizedValue(int normalizedValue)
{
#ifdef CLIENTUI_USE_640_480_PROPBASE
	auto retaddr = (PUCHAR)_ReturnAddress();

	if (g_pVGuiSurface2->IsForcingHDProportional() && !g_IsNativeClientUIHDProportional && retaddr > g_dwClientTextBase && retaddr < (PUCHAR)g_dwClientTextBase + g_dwClientTextSize)
	{
		g_pVGuiSurface2->SetForcingHDProportional(false);

		auto r = g_pVGuiSchemeManager2->GetHDProportionalNormalizedValue(normalizedValue);

		g_pVGuiSurface2->SetForcingHDProportional(true);

		return r;
	}
#endif

	return g_pVGuiSchemeManager2->GetHDProportionalNormalizedValue(normalizedValue);
}

static CSchemeManagerProxy_HL25 g_SchemeProxy_HL25;

void Scheme_InstallHooks(void)
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		DWORD* pVFTable = *(DWORD**)&g_SchemeProxy_HL25;

		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 1, (void *)pVFTable[1], (void **)&m_pfnLoadSchemeFromFile); //Assert (IsValidIndex(i))
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 2, (void*)pVFTable[2], (void**)&m_pfnReloadSchemes);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 3, (void*)pVFTable[3], (void**)&m_pfnGetDefaultScheme);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 4, (void*)pVFTable[4], (void**)&m_pfnGetScheme);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 5, (void *)pVFTable[5], (void **)&m_pfnGetImage);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 6, (void*)pVFTable[6], (void**)&m_pfnGetImageID);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 7, (void *)pVFTable[7], (void **)&m_pfnGetIScheme);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 8, (void*)pVFTable[8], (void**)&m_pfnShutdown);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 9, (void*)pVFTable[9], (void**)&m_pfnGetProportionalScaledValue);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 10, (void*)pVFTable[10], (void**)&m_pfnGetProportionalNormalizedValue);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 11, (void*)pVFTable[11], (void**)&m_pfnGetProportionalScale);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 12, (void*)pVFTable[12], (void**)&m_pfnGetHDProportionalScaledValue);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager_HL25, 0, 13, (void*)pVFTable[13], (void**)&m_pfnGetHDProportionalNormalizedValue);

	}
	else
	{
		DWORD* pVFTable = *(DWORD**)&g_SchemeProxy;

		g_pMetaHookAPI->VFTHook(g_pSchemeManager, 0, 1, (void*)pVFTable[1], (void**)&m_pfnLoadSchemeFromFile);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager, 0, 2, (void*)pVFTable[2], (void**)&m_pfnReloadSchemes);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager, 0, 3, (void*)pVFTable[3], (void**)&m_pfnGetDefaultScheme);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager, 0, 4, (void*)pVFTable[4], (void**)&m_pfnGetScheme);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager, 0, 5, (void*)pVFTable[5], (void**)&m_pfnGetImage);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager, 0, 6, (void*)pVFTable[6], (void**)&m_pfnGetImageID);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager, 0, 7, (void*)pVFTable[7], (void**)&m_pfnGetIScheme);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager, 0, 8, (void*)pVFTable[8], (void**)&m_pfnShutdown);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager, 0, 9, (void*)pVFTable[9], (void**)&m_pfnGetProportionalScaledValue);
		g_pMetaHookAPI->VFTHook(g_pSchemeManager, 0, 10, (void*)pVFTable[10], (void**)&m_pfnGetProportionalNormalizedValue);
	}
}

void Scheme_UninstallHooks(void)
{

}