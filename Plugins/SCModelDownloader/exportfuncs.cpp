#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include <capstone.h>
#include <cl_entity.h>
#include <com_model.h>
#include <cvardef.h>
#include <entity_types.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "plugins.h"
#include "SCModelDatabase.h"
#include "UtilHTTPClient.h"
#include "UtilAssetsIntegrity.h"

#include <set>
#include <vector>

cvar_t *scmodel_autodownload = NULL;
cvar_t *scmodel_downloadlatest = NULL;
cvar_t* scmodel_cdn = NULL;

cl_enginefunc_t gEngfuncs = {0};
engine_studio_api_t IEngineStudio = { 0 };
r_studio_interface_t **gpStudioInterface = NULL;

bool SCModel_AutoDownload()
{
	return scmodel_autodownload->value >= 1 ? true : false;
}

bool SCModel_ShouldDownloadLatest()
{
	return scmodel_downloadlatest->value >= 1 ? true : false;
}

int SCModel_CDN()
{
	return (int)scmodel_cdn->value;
}

/*
	Purpose: Reload model for players that are using the specified model
*/

void SCModel_ReloadModel(const char *name)
{
	//Reload models for those players
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!stricmp((*DM_PlayerState)[i].name, name))
		{
			(*DM_PlayerState)[i].name[0] = 0;
			(*DM_PlayerState)[i].model = nullptr;
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
		(*DM_PlayerState)[i].model = nullptr;
	}
}

void R_StudioChangePlayerModel(void)
{
	gPrivateFuncs.R_StudioChangePlayerModel();

	int index = IEngineStudio.GetCurrentEntity()->index;

	if (index >= 1 && index <= 32)
	{
		if ((*DM_PlayerState)[index - 1].model == IEngineStudio.GetCurrentEntity()->model || !(*DM_PlayerState)[index - 1].model)
		{
			if ((*DM_PlayerState)[index - 1].name[0])
			{
				if (SCModel_AutoDownload())
				{
					SCModelDatabase()->QueryModel((*DM_PlayerState)[index - 1].name);
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
	UtilHTTPClient()->RunFrame();
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();
	
	scmodel_autodownload = gEngfuncs.pfnRegisterVariable("scmodel_autodownload", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	scmodel_downloadlatest = gEngfuncs.pfnRegisterVariable("scmodel_downloadlatest", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	scmodel_cdn = gEngfuncs.pfnRegisterVariable("scmodel_cdn", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	gEngfuncs.pfnAddCommand("scmodel_reload", SCModel_Reload_f);

	SCModelDatabase()->Init();
}

void HUD_Shutdown(void)
{
	SCModelDatabase()->Shutdown();

	gExportfuncs.HUD_Shutdown();

	UtilAssetsIntegrity_Shutdown();
	UtilHTTPClient_Shutdown();
}

void EngineStudio_FillAddress_SetupPlayerModel(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID SetupPlayerModel = ConvertDllInfoSpace(pstudio->SetupPlayerModel, RealDllInfo, DllInfo);

	if (!SetupPlayerModel)
	{
		Sig_NotFound(SetupPlayerModel);
	}

	typedef struct SetupPlayerModel_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		PVOID base{};
		size_t max_insts{};
		int max_depth{};
		std::set<PVOID> code;
		std::set<PVOID> branches;
		std::vector<walk_context_t> walks;
		int StudioSetRemapColors_instcount{};
		std::set<PVOID> addr_call_R_StudioChangePlayerModel;
	}SetupPlayerModel_SearchContext;

	SetupPlayerModel_SearchContext ctx = { DllInfo, RealDllInfo };

	ctx.base = SetupPlayerModel;

	ctx.max_insts = 1000;
	ctx.max_depth = 16;
	ctx.walks.emplace_back(ctx.base, 0x1000, 0);

	while (ctx.walks.size())
	{
		auto walk = ctx.walks[ctx.walks.size() - 1];
		ctx.walks.pop_back();

		g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (SetupPlayerModel_SearchContext*)context;

			if (ctx->code.size() > ctx->max_insts)
				return TRUE;

			if (ctx->code.find(address) != ctx->code.end())
				return TRUE;

			ctx->code.emplace(address);

			if (pinst->id == X86_INS_CALL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				pinst->detail->x86.operands[0].imm >= (ULONG_PTR)ctx->DllInfo.ImageBase &&
				pinst->detail->x86.operands[0].imm < (ULONG_PTR)ctx->DllInfo.ImageBase + ctx->DllInfo.ImageSize)
			{
				PVOID calltarget_pfn = (PVOID)pinst->detail->x86.operands[0].imm;
				PVOID calltarget_RealDllBased = ConvertDllInfoSpace(calltarget_pfn, ctx->DllInfo, ctx->RealDllInfo);

				if (calltarget_RealDllBased == gPrivateFuncs.R_StudioChangePlayerModel)
				{
					auto address_RealDllBased = ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);

					ctx->addr_call_R_StudioChangePlayerModel.emplace(address_RealDllBased);
				}
			}

			if (!DM_PlayerState)
			{
				if (pinst->id == X86_INS_LEA &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
					pinst->detail->x86.operands[1].mem.base != 0 &&
					pinst->detail->x86.operands[1].mem.scale == 1 || pinst->detail->x86.operands[1].mem.scale == 4)
				{
					DM_PlayerState = (decltype(DM_PlayerState))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}
			}

			if ((pinst->id == X86_INS_JMP || (pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS)) &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM)
			{
				PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;
				auto foundbranch = ctx->branches.find(imm);
				if (foundbranch == ctx->branches.end())
				{
					ctx->branches.emplace(imm);
					if (depth + 1 < ctx->max_depth)
						ctx->walks.emplace_back(imm, 0x1000, depth + 1);
				}

				if (pinst->id == X86_INS_JMP)
					return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, walk.depth, &ctx);
	}

	Sig_VarNotFound(DM_PlayerState);

	if (ctx.addr_call_R_StudioChangePlayerModel.empty()) {
		Sig_NotFound(call_R_StudioChangePlayerModel);
	}

	for (auto addr : ctx.addr_call_R_StudioChangePlayerModel)
	{
		g_pMetaHookAPI->InlinePatchRedirectBranch(addr, R_StudioChangePlayerModel, NULL);
	}
}

void EngineStudio_FillAddress(struct engine_studio_api_s* pstudio, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	EngineStudio_FillAddress_SetupPlayerModel(pstudio, DllInfo, RealDllInfo);
}

void EngineStudio_InstalHooks()
{

}

void ClientStudio_FillAddress(struct r_studio_interface_s** ppinterface)
{
	
}

void ClientStudio_InstallHooks()
{
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	EngineStudio_FillAddress(pstudio, g_MirrorEngineDLLInfo.ImageBase ? g_MirrorEngineDLLInfo : g_EngineDLLInfo, g_EngineDLLInfo);
	EngineStudio_InstalHooks();

	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	ClientStudio_FillAddress(ppinterface);
	ClientStudio_InstallHooks();

	int result = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	return result;
}