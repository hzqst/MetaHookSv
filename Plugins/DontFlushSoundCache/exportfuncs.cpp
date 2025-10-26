#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"
#include "com_model.h"
#include "triangleapi.h"
#include "cvardef.h"
#include "exportfuncs.h"
#include "entity_types.h"
#include "parsemsg.h"

#include "privatehook.h"

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

int g_iRetryState = 0;

int NewClientCmd(const char *szCmdString)
{
	//It's not necessary when flush-on-retry is blocked!
	if (!strncmp(szCmdString, "dlfile ", sizeof("dlfile ") - 1))
	{
		auto levelname = gEngfuncs.pfnGetLevelName();
		if (!strncmp(szCmdString + sizeof("dlfile ") - 1, "maps/soundcache ", sizeof("maps/soundcache") - 1))
		{
			return 0;
		}
	}

	return gPrivateFuncs.pfnClientCmd(szCmdString);
}

void NewConnect_f(void)
{
	if (g_iRetryState == 1)
	{
		g_iRetryState = 2;
	}

	gPrivateFuncs.Connect_f();

	if (g_iRetryState)
	{
		g_iRetryState = 0;
	}
}

void NewRetry_f()
{
	g_iRetryState = 1;

	gPrivateFuncs.Retry_f();
}

void IN_ActivateMouse(void)
{
	gExportfuncs.IN_ActivateMouse();

	static bool init = false;

	if (!init)
	{
		gPrivateFuncs.Connect_f = g_pMetaHookAPI->HookCmd("connect", NewConnect_f);
		gPrivateFuncs.Retry_f = g_pMetaHookAPI->HookCmd("retry", NewRetry_f);

		init = true;
	}
}