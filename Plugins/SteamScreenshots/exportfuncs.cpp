#include <metahook.h>
#include <glew.h>
#include "cvardef.h"
#include "exportfuncs.h"
#include "entity_types.h"
#include "parsemsg.h"
#include "gl_capture.h"
#include <steam_api.h>

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

char g_szServerName[256] = {0};

namespace
{
	bool g_bLoggedSteamScreenshotsUnavailable = false;
	bool g_bLoggedSteamUserUnavailable = false;
	bool g_bLoggedSteamScreenshotsWriteUnavailable = false;

	void Con_Printf_Once(bool& bAlreadyPrinted, const char* pszMessage)
	{
		if (bAlreadyPrinted)
			return;

		bAlreadyPrinted = true;
		gEngfuncs.Con_Printf(pszMessage);
	}
}
class CSnapshotManager
{
public:
	STEAM_CALLBACK_MANUAL(CSnapshotManager, OnSnapshotCallback, ScreenshotReady_t, m_ScreenshotReadyCallback);
	void TryRegisterCallback();
	void UnregisterCallback();

private:
	bool m_bCallbackRegistered = false;
};

CSnapshotManager g_SnapshotManager;

void CSnapshotManager::TryRegisterCallback()
{
	if (m_bCallbackRegistered)
		return;

	if (!SteamScreenshots() || !SteamUser())
		return;

	m_ScreenshotReadyCallback.Register(this, &CSnapshotManager::OnSnapshotCallback);
	m_bCallbackRegistered = true;
}

void CSnapshotManager::UnregisterCallback()
{
	if (!m_bCallbackRegistered)
		return;

	m_ScreenshotReadyCallback.Unregister();
	m_bCallbackRegistered = false;
}

void CSnapshotManager::OnSnapshotCallback(ScreenshotReady_t* pCallback)
{
	auto pSteamScreenshots = SteamScreenshots();

	if (!pSteamScreenshots)
	{
		Con_Printf_Once(g_bLoggedSteamScreenshotsUnavailable, "[SteamScreenshots] Steam screenshots interface unavailable.\n");
		return;
	}

	if (pCallback->m_eResult == k_EResultOK)
	{
		pSteamScreenshots->SetLocation(pCallback->m_hLocal, g_szServerName);

		auto pSteamUser = SteamUser();
		if (pSteamUser)
		{
			pSteamScreenshots->TagUser(pCallback->m_hLocal, pSteamUser->GetSteamID());
		}
		else
		{
			Con_Printf_Once(g_bLoggedSteamUserUnavailable, "[SteamScreenshots] Cannot tag user because Steam user interface is unavailable.\n");
		}

		gEngfuncs.Con_Printf("[SteamScreenshots] Snapshot saved.\n");
	}
	else if (pCallback->m_eResult == k_EResultIOFailure)
	{
		gEngfuncs.Con_Printf("[SteamScreenshots] Cannot save snapshot. Got an IO error.\n");
	}
	else
	{
		gEngfuncs.Con_Printf("[SteamScreenshots] Cannot save snapshot. Got an unknown error.\n");
	}
}

void HUD_Shutdown(void)
{
	g_SnapshotManager.UnregisterCallback();

	GL_ShutdownCapture();

	gExportfuncs.HUD_Shutdown();
}

void ScreenshotCallback(void* pBuf, size_t cbBufSize, int width, int height)
{
	auto pSteamScreenshots = SteamScreenshots();

	if (!pSteamScreenshots)
	{
		Con_Printf_Once(g_bLoggedSteamScreenshotsWriteUnavailable, "[SteamScreenshots] Cannot write screenshot because Steam screenshots interface is unavailable.\n");
		return;
	}

	pSteamScreenshots->WriteScreenshot(pBuf, cbBufSize, width, height);
}

void VID_Snapshot_f(void)
{
	GL_BeginCapture(ScreenshotCallback);
}

void HUD_Frame(double time)
{
	gExportfuncs.HUD_Frame(time);

	g_SnapshotManager.TryRegisterCallback();

	auto levelname = gEngfuncs.pfnGetLevelName();

	if (!levelname || !levelname[0])
	{
		g_szServerName[0] = 0;
	}

	GL_QueryAsyncCapture(ScreenshotCallback);
}

pfnUserMsgHook m_pfnServerName = NULL;

int __MsgFunc_ServerName(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	char *szServerName = READ_STRING();

	strncpy(g_szServerName, szServerName, sizeof(g_szServerName) - 1);
	g_szServerName[sizeof(g_szServerName) - 1] = 0;

	if (m_pfnServerName)
		return m_pfnServerName(pszName, iSize, pbuf);

	return 0;
}

void IN_ActivateMouse(void)
{
	gExportfuncs.IN_ActivateMouse();

	static bool init = false;

	if (!init)
	{
		GL_InitCapture();

		//cmd "snapshot" is registered after HUD_Init

		g_pMetaHookAPI->HookCmd("snapshot", VID_Snapshot_f);

		m_pfnServerName = HOOK_MESSAGE(ServerName);

		g_SnapshotManager.TryRegisterCallback();

		init = true;
	}
}



