#include <metahook.h>
#include <glew.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"
#include "com_model.h"
#include "triangleapi.h"
#include "cvardef.h"
#include "exportfuncs.h"
#include "entity_types.h"
#include "privatehook.h"
#include "msghook.h"
#include "parsemsg.h"
#include <steam_api.h>

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

char g_ServerName[256] = {0};

class CSnapshotManager
{
public:
	STEAM_CALLBACK(CSnapshotManager, OnSnapshotCallback, ScreenshotReady_t);
};

CSnapshotManager g_SnapshotManager;

void Sys_ErrorEx(const char *fmt, ...)
{
	char msg[4096] = { 0 };

	va_list argptr;

	va_start(argptr, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	if (gEngfuncs.pfnClientCmd)
		gEngfuncs.pfnClientCmd("escape\n");

	MessageBox(NULL, msg, "Fatal Error", MB_ICONERROR);
	TerminateProcess((HANDLE)(-1), 0);
}

void VID_Snapshot_f(void)
{
	int glwidth, glheight;
	g_pMetaHookAPI->GetVideoMode(&glwidth, &glheight, NULL, NULL);

	byte *pBuf = (byte *)malloc(glwidth * glheight * 3);

	int read_fbo = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadPixels(0, 0, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, pBuf);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo);

	for (int y = 0; y < glheight / 2; ++y) {
		for (int x = 0; x < glwidth; ++x) {
			byte temp[3];
			memcpy(temp, &pBuf[(glheight - y - 1) * glwidth * 3 + x * 3], 3);
			memcpy(&pBuf[(glheight - y - 1) * glwidth * 3 + x * 3], &pBuf[y * glwidth * 3 + x * 3], 3);
			memcpy(&pBuf[y * glwidth * 3 + x * 3], temp, 3);
		}
	}

	SteamScreenshots()->WriteScreenshot(pBuf, glwidth * glheight * 3, glwidth, glheight);

	free(pBuf);
}

void CSnapshotManager::OnSnapshotCallback(ScreenshotReady_t* pCallback)
{
	if (pCallback->m_eResult == k_EResultOK)
	{
		SteamScreenshots()->SetLocation(pCallback->m_hLocal, g_ServerName);

		SteamScreenshots()->TagUser(pCallback->m_hLocal, SteamUser()->GetSteamID());

		gEngfuncs.Con_Printf("Snapshot saved.\n");
	}
	else if (pCallback->m_eResult == k_EResultFail)
	{
		gEngfuncs.Con_Printf("Error: Cannot parse snapshot.\n");
	}
	else if (pCallback->m_eResult == k_EResultIOFailure)
	{
		gEngfuncs.Con_Printf("Error: Cannot save snapshot.\n");
	}
}

void HUD_Frame(double time)
{
	gExportfuncs.HUD_Frame(time);

	auto levelname = gEngfuncs.pfnGetLevelName();
	if (!levelname || !levelname[0])
	{
		g_ServerName[0] = 0;
	}

	SteamAPI_RunCallbacks();
}

pfnUserMsgHook m_pfnServerName;

int __MsgFunc_ServerName(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	char *servername = READ_STRING();

	strncpy(g_ServerName, servername, 255);
	g_ServerName[255] = 0;

	return m_pfnServerName(pszName, iSize, pbuf);
}

void IN_ActivateMouse(void)
{
	gExportfuncs.IN_ActivateMouse();

	static bool init = false;

	if (!init)
	{
		g_pMetaHookAPI->HookCmd("snapshot", VID_Snapshot_f);

		m_pfnServerName = HOOK_MESSAGE(ServerName);
		init = true;
	}
}