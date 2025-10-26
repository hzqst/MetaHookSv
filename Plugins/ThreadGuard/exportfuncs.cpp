#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"
#include "com_model.h"
#include "triangleapi.h"
#include "cvardef.h"
#include "exportfuncs.h"
#include "entity_types.h"
#include "plugins.h"
#include <string>
#include <sstream>

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

static xcommand_t g_pfn_Host_KillServer_f;
static xcommand_t g_pfn_Host_Quit_Restart_f;

/*
	Purpose: This fixed a bug from Valve that command "_restart" didn't shutdown server correctly, causing resources like CSteam3Server and such things to leak.
*/

void Host_Quit_Restart_f(void)
{
	g_pfn_Host_KillServer_f();

	return g_pfn_Host_Quit_Restart_f();
}

void EngineCommand_InstallHook(void)
{
	auto entry = g_pMetaHookAPI->FindCmd("shutdownserver");
	if (entry)
	{
		g_pfn_Host_KillServer_f = entry->function;
		g_pfn_Host_Quit_Restart_f = g_pMetaHookAPI->HookCmd("_restart", Host_Quit_Restart_f);
	}
	else
	{
		Sys_Error("Command \"shutdownserver\" not found!");
	}
}