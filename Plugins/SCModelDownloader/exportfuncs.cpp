#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include <capstone.h>
#include "cl_entity.h"
#include "com_model.h"
#include "triangleapi.h"
#include "cvardef.h"
#include "exportfuncs.h"
#include "entity_types.h"
#include "parsemsg.h"
#include "privatehook.h"
#include "plugins.h"
#include "SCModelDatabase.h"
#include "utilhttpclient.h"
#include <string>
#include <functional>
#include <set>
#include <unordered_map>

cvar_t *scmodel_autodownload = NULL;
cvar_t *scmodel_downloadlatest = NULL;
cvar_t *scmodel_baseurl = NULL;

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

const char* SCModel_GetBaseUrl()
{
	return scmodel_baseurl->string;
}

bool SCModel_ShouldDownloadLatest()
{
	return scmodel_downloadlatest->value >= 1 ? true : false;
}

/*
	Purpose: Reload model for players that are using the specified model
*/
void SCModel_ReloadModel(const char *name)
{
	//Reload models for those players
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!strcmp((*DM_PlayerState)[i].name, name))
		{
			(*DM_PlayerState)[i].name[0] = 0;
		}
	}
}

/*
	Purpose: Reload models for all players
*/
void SCModel_ReloadAllModels()
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		(*DM_PlayerState)[i].name[0] = 0;
	}
}

void R_StudioChangePlayerModel(void)
{
	gPrivateFuncs.R_StudioChangePlayerModel();

	int index = IEngineStudio.GetCurrentEntity()->index;

	if (index >= 1 && index <= 32)
	{
		if ((*DM_PlayerState)[index - 1].model == IEngineStudio.GetCurrentEntity()->model)
		{
			if ((*DM_PlayerState)[index - 1].name[0])
			{
				if (scmodel_autodownload->value)
				{
					SCModelDatabase()->OnMissingModel((*DM_PlayerState)[index - 1].name);
				}
			}
		}
	}
}

void SCModel_Reload_f(void)
{
	SCModel_ReloadAllModels();
}

void HUD_Frame(double frame)
{
	gExportfuncs.HUD_Frame(frame);

	SCModelDatabase()->RunFrame();
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();
	
	scmodel_autodownload = gEngfuncs.pfnRegisterVariable("scmodel_autodownload", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	scmodel_downloadlatest = gEngfuncs.pfnRegisterVariable("scmodel_downloadlatest", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	scmodel_baseurl = gEngfuncs.pfnRegisterVariable("scmodel_baseurl", "", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	gEngfuncs.pfnAddCommand("scmodel_reload", SCModel_Reload_f);

	SCModelDatabase()->Init();
}

void HUD_Shutdown(void)
{
	SCModelDatabase()->Shutdown();

	gExportfuncs.HUD_Shutdown();

	UtilHTTPClient_Shutdown();
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	gPrivateFuncs.studioapi_SetupPlayerModel = IEngineStudio.SetupPlayerModel;

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(pstudio->SetupPlayerModel, 0x500, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (address[0] == 0xE8)
			{
				PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;

				if (imm == gPrivateFuncs.R_StudioChangePlayerModel)
				{
					PVOID ptarget = R_StudioChangePlayerModel;
					int rva = (PUCHAR)ptarget - (address + 5);
					g_pMetaHookAPI->WriteMemory(address + 1, &rva, 4);
				}
			}

			if (!DM_PlayerState)
			{
				if (pinst->id == X86_INS_LEA &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
					pinst->detail->x86.operands[1].mem.base != 0 &&
					pinst->detail->x86.operands[1].mem.scale == 1 || pinst->detail->x86.operands[1].mem.scale == 4)
				{
					DM_PlayerState = (decltype(DM_PlayerState))pinst->detail->x86.operands[1].mem.disp;
				}
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);
	}

	int result = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	return result;
}