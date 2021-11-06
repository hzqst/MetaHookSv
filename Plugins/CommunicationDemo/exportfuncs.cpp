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

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

#define MHSV_CMD_QUERY_PLUGIN 1
#define MHSV_CMD_QUERY_CVAR 2

int __MsgFunc_MetaHook(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	auto cmd2 = READ_BYTE();
	if (cmd2 == MHSV_CMD_QUERY_PLUGIN)
	{
		mh_plugininfo_t info;
		for (int index = -1; g_pMetaHookAPI->QueryPluginInfo(index, &info); ++index)
		{
			char cmd[128] = { 0 };
			snprintf(cmd, sizeof(cmd) - 1, "mh_reportplugin %d %d \"%s\" \"%s\" ", info.Index, info.InterfaceVersion, info.PluginName, info.PluginVersion);
			gEngfuncs.pfnServerCmd(cmd);
		}
	}
	else if (cmd2 == MHSV_CMD_QUERY_CVAR)
	{
		auto request_id = READ_LONG();
		auto cvar_name = READ_STRING();
		auto cvar = gEngfuncs.pfnGetCvarPointer(cvar_name);
		if (cvar)
		{
			char cmd[128] = { 0 };
			snprintf(cmd, sizeof(cmd) - 1, "mh_reportcvar %d \"%s\" \"%s\" ", request_id, cvar->name, cvar->string);
			gEngfuncs.pfnServerCmd(cmd);
		}
		else
		{
			char cmd[128] = { 0 };
			snprintf(cmd, sizeof(cmd) - 1, "mh_reportcvar %d \"%s\"", request_id, cvar_name);
			gEngfuncs.pfnServerCmd(cmd);
		}
	}
	return 0;
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	gEngfuncs.pfnHookUserMsg("MetaHook", __MsgFunc_MetaHook);
}