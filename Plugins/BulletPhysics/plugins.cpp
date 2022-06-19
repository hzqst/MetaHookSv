#include <metahook.h>
#include <com_model.h>
#include <capstone.h>
#include "exportfuncs.h"
#include "privatehook.h"
#include "plugins.h"
#include "command.h"
#include "message.h"
#include "qgl.h"

cl_exportfuncs_t gExportfuncs;
mh_interface_t *g_pInterface;
metahook_api_t *g_pMetaHookAPI;
mh_enginesave_t *g_pMetaSave;
IFileSystem *g_pFileSystem;

HINSTANCE g_hInstance, g_hThisModule, g_hEngineModule;
PVOID g_dwEngineBase;
DWORD g_dwEngineSize;
PVOID g_dwEngineTextBase;
DWORD g_dwEngineTextSize;
PVOID g_dwEngineDataBase;
DWORD g_dwEngineDataSize;
PVOID g_dwEngineRdataBase;
DWORD g_dwEngineRdataSize;
DWORD g_dwEngineBuildnum;
int g_iEngineType;
PVOID g_dwClientBase;
DWORD g_dwClientSize;

void IPluginsV4::Init(metahook_api_t *pAPI, mh_interface_t *pInterface, mh_enginesave_t *pSave)
{
	g_pInterface = pInterface;
	g_pMetaHookAPI = pAPI;
	g_pMetaSave = pSave;
	g_hInstance = GetModuleHandle(NULL);
}

void IPluginsV4::Shutdown(void)
{
}

#define R_NEWMAP_SIG_SVENGINE "\x55\x8B\xEC\x51\xC7\x45\xFC\x00\x00\x00\x00\xEB\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC\x81\x7D\xFC\x00\x01\x00\x00"
#define R_NEWMAP_SIG_NEW "\x55\x8B\xEC\x83\xEC\x08\xC7\x45\xFC\x00\x00\x00\x00\x2A\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC\x81\x7D\xFC\x00\x01\x00\x00\x2A\x2A\x8B\x4D\xFC"

#define R_RECURSIVEWORLDNODE_SIG_SVENGINE "\x83\xEC\x08\x53\x8B\x5C\x24\x10\x83\x3B\xFE"
#define R_RECURSIVEWORLDNODE_SIG_NEW "\x55\x8B\xEC\x83\xEC\x08\x53\x56\x57\x8B\x7D\x08\x83\x3F\xFE\x0F\x2A\x2A\x2A\x2A\x2A\x8B\x47\x04"

void IPluginsV4::LoadEngine(cl_enginefunc_t *pEngfuncs)
{
	g_pFileSystem = g_pInterface->FileSystem;
	g_iEngineType = g_pMetaHookAPI->GetEngineType();
	g_dwEngineBuildnum = g_pMetaHookAPI->GetEngineBuildnum();
	g_hEngineModule = g_pMetaHookAPI->GetEngineModule();
	g_dwEngineBase = g_pMetaHookAPI->GetEngineBase();
	g_dwEngineSize = g_pMetaHookAPI->GetEngineSize();
	g_dwEngineTextBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".text\x0\x0\x0", &g_dwEngineTextSize);
	g_dwEngineDataBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".data\x0\x0\x0", &g_dwEngineDataSize);
	g_dwEngineRdataBase = g_pMetaHookAPI->GetSectionByName(g_dwEngineBase, ".rdata\x0\x0", &g_dwEngineRdataSize);

	memcpy(&gEngfuncs, pEngfuncs, sizeof(gEngfuncs));

	if (g_iEngineType != ENGINE_SVENGINE && g_iEngineType != ENGINE_GOLDSRC)
	{
		g_pMetaHookAPI->SysError("Unsupported engine: %s, buildnum %d", g_pMetaHookAPI->GetEngineTypeName(), g_dwEngineBuildnum);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern(R_RECURSIVEWORLDNODE_SIG_SVENGINE);
		Sig_FuncNotFound(R_RecursiveWorldNode);

		//mov     eax, [edi+4]
		//mov     ecx, r_visframecount
#define R_VISFRAMECOUNT_SIG_SVENGINE "\x8B\x43\x04\x3B\x05"
		{
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_RecursiveWorldNode, 0x100, R_VISFRAMECOUNT_SIG_SVENGINE, sizeof(R_VISFRAMECOUNT_SIG_SVENGINE) - 1);
			Sig_AddrNotFound(r_visframecount);
			r_visframecount = *(int **)(addr + 5);
		}
#define CL_PARSECOUNT_SIG_SVENGINE "\x23\x05\x2A\x2A\x2A\x2A\x69\xC8\xD8\x84\x00\x00"
		
		{
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)g_dwEngineTextBase, g_dwEngineTextSize, CL_PARSECOUNT_SIG_SVENGINE, sizeof(CL_PARSECOUNT_SIG_SVENGINE) - 1);
			Sig_AddrNotFound(cl_parsecount);
			cl_parsecount = *(int **)(addr + 2);
		}

		gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))Search_Pattern(R_NEWMAP_SIG_SVENGINE);
		Sig_FuncNotFound(R_NewMap);
		
	}
	else
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern(R_RECURSIVEWORLDNODE_SIG_NEW);
		Sig_FuncNotFound(R_RecursiveWorldNode);

		//mov     eax, [edi+4]
		//mov     ecx, r_visframecount
#define R_VISFRAMECOUNT_SIG_NEW "\x8B\x47\x04\x8B\x0D"
		{
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_RecursiveWorldNode, 0x100, R_VISFRAMECOUNT_SIG_NEW, sizeof(R_VISFRAMECOUNT_SIG_NEW) - 1);
			Sig_AddrNotFound(r_visframecount);
			r_visframecount = *(int **)(addr + 5);
		}
#define CL_PARSECOUNT_SIG_NEW "\x8B\x0D\x2A\x2A\x2A\x2A\x23\xC1\x8B\x12"
		{
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)g_dwEngineTextBase, g_dwEngineTextSize, CL_PARSECOUNT_SIG_NEW, sizeof(CL_PARSECOUNT_SIG_NEW) - 1);
			Sig_AddrNotFound(cl_parsecount);
			cl_parsecount = *(int **)(addr + 2);
		}

		gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))Search_Pattern(R_NEWMAP_SIG_NEW);
		Sig_FuncNotFound(R_NewMap);
	}

#define MOD_KNOWN_SIG "\xB8\x9D\x82\x97\x53\x81\xE9"

	{
		DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)g_dwEngineTextBase, g_dwEngineTextSize, MOD_KNOWN_SIG, sizeof(MOD_KNOWN_SIG) - 1);
		Sig_AddrNotFound(mod_known);
		mod_known = *(void **)(addr + 7);
	}

	{
		const char sigs1[] = "Cached models:\n";
		auto Mod_Print_String = Search_Pattern_Data(sigs1);
		if (!Mod_Print_String)
			Mod_Print_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Mod_Print_String);
		char pattern[] = "\x57\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD *)(pattern + 2) = (DWORD)Mod_Print_String;
		auto Mod_Print_Call = Search_Pattern(pattern);
		Sig_VarNotFound(Mod_Print_Call);

		g_pMetaHookAPI->DisasmRanges(Mod_Print_Call, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{//A1 84 5C 32 02 mov     eax, mod_numknown
				DWORD imm = pinst->detail->x86.operands[1].mem.disp;

				mod_numknown = (decltype(mod_numknown))imm;
			}
			else if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//39 3D 44 32 90 03 cmp     mod_numknown, edi
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				mod_numknown = (decltype(mod_numknown))imm;
			}

			if (mod_numknown)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);
	}
}

void IPluginsV4::LoadClient(cl_exportfuncs_t *pExportFunc)
{
	memcpy(&gExportfuncs, pExportFunc, sizeof(gExportfuncs));

	g_dwClientBase = g_pMetaHookAPI->GetClientBase();
	g_dwClientSize = g_pMetaHookAPI->GetClientSize();

	pExportFunc->HUD_Init = HUD_Init;
	pExportFunc->HUD_GetStudioModelInterface = HUD_GetStudioModelInterface;
	pExportFunc->HUD_TempEntUpdate = HUD_TempEntUpdate;
	pExportFunc->HUD_AddEntity = HUD_AddEntity;
	pExportFunc->HUD_DrawNormalTriangles = HUD_DrawNormalTriangles;
	pExportFunc->V_CalcRefdef = V_CalcRefdef;

	auto err = glewInit();
	if (GLEW_OK != err)
	{
		g_pMetaHookAPI->SysError("glewInit failed, %s", glewGetErrorString(err));
		return;
	}


	Install_InlineHook(R_NewMap);

	gPrivateFuncs.efxapi_R_TempModel = gEngfuncs.pEfxAPI->R_TempModel;
	Install_InlineHook(efxapi_R_TempModel);
}

void IPluginsV4::ExitGame(int iResult)
{
}

const char completeVersion[] =
{
	BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3,
	'-',
	BUILD_MONTH_CH0, BUILD_MONTH_CH1,
	'-',
	BUILD_DAY_CH0, BUILD_DAY_CH1,
	'T',
	BUILD_HOUR_CH0, BUILD_HOUR_CH1,
	':',
	BUILD_MIN_CH0, BUILD_MIN_CH1,
	':',
	BUILD_SEC_CH0, BUILD_SEC_CH1,
	'\0'
};

const char *IPluginsV4::GetVersion(void)
{
	return completeVersion;
}

EXPOSE_SINGLE_INTERFACE(IPluginsV4, IPluginsV4, METAHOOK_PLUGIN_API_VERSION_V4);