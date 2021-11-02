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

int __MsgFunc_MetaHookSv(const char *pszName, int iSize, void *pbuf)
{
	auto cmd2 = READ_BYTE();
	if (cmd2 == MHSV_CMD_QUERY_PLUGIN)
	{
		mh_plugininfo_t info;
		for (int index = -1; g_pMetaHookAPI->QueryPluginInfo(index, &info); ++index)
		{
			char cmd[128] = { 0 };
			snprintf(cmd, sizeof(cmd) - 1, "mh_plugin %d %d \"%s\" \"%s\" ", info.Index, info.InterfaceVersion, info.PluginName, info.PluginVersion);
			gEngfuncs.pfnServerCmd(cmd);
		}
	}
	return 0;
}

void HUD_DirectorMessage(int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	auto cmd = READ_BYTE();

	if (cmd == 100)
	{
		auto cmd2 = READ_BYTE();

		if (cmd2 == MHSV_CMD_QUERY_PLUGIN)
		{
			mh_plugininfo_t info;
			for (int index = -1; g_pMetaHookAPI->QueryPluginInfo(index, &info); ++index)
			{
				char cmd[128] = { 0 };
				snprintf(cmd, sizeof(cmd) - 1, "mh_plugin %d %d \"%s\" \"%s\" ", info.Index, info.InterfaceVersion, info.PluginName, info.PluginVersion);
				gEngfuncs.pfnServerCmd(cmd);
			}
		}

		return;
	}	

	gExportfuncs.HUD_DirectorMessage(iSize, pbuf);
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	gEngfuncs.pfnHookUserMsg("MetaHookSv", __MsgFunc_MetaHookSv);
}