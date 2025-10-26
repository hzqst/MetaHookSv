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

class CSnapshotManager
{
public:
	STEAM_CALLBACK(CSnapshotManager, OnSnapshotCallback, ScreenshotReady_t);
};

CSnapshotManager g_SnapshotManager;

void CSnapshotManager::OnSnapshotCallback(ScreenshotReady_t* pCallback)
{
	if (pCallback->m_eResult == k_EResultOK)
	{
		SteamScreenshots()->SetLocation(pCallback->m_hLocal, g_szServerName);

		SteamScreenshots()->TagUser(pCallback->m_hLocal, SteamUser()->GetSteamID());

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
	GL_ShutdownCapture();

	gExportfuncs.HUD_Shutdown();
}

void ScreenshotCallback(void* pBuf, size_t cbBufSize, int width, int height)
{
	SteamScreenshots()->WriteScreenshot(pBuf, cbBufSize, width, height);
}

void VID_Snapshot_f(void)
{
	GL_BeginCapture(ScreenshotCallback);
}

void HUD_Frame(double time)
{
	gExportfuncs.HUD_Frame(time);

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

	return m_pfnServerName(pszName, iSize, pbuf);
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

		init = true;
	}
}