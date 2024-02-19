#include <metahook.h>
#include <capstone.h>
#include "plugins.h"
#include "privatehook.h"
#include "ResourceReplacer.h"

private_funcs_t gPrivateFuncs = { 0 };

//static hook_t* g_phook_FileSystem_Open = NULL;
static hook_t* g_phook_Mod_LoadModel_FS_Open = NULL;
static hook_t* g_phook_CL_PrecacheResources = NULL;

void RemoveFileExtension(std::string& filePath);

qboolean CL_PrecacheResources()
{
	if (1)
	{
		std::string name = gEngfuncs.pfnGetLevelName();

		RemoveFileExtension(name);

		name += ".gmr";

		ModelReplacer()->LoadMapReplaceList(name.c_str());
	}

	if (1)
	{
		std::string name = gEngfuncs.pfnGetLevelName();

		RemoveFileExtension(name);

		name += ".gsr";

		SoundReplacer()->LoadMapReplaceList(name.c_str());
	}

	return gPrivateFuncs.CL_PrecacheResources();
}

FileHandle_t Mod_LoadModel_FS_Open(const char* pFileName, const char* pOptions)
{
	if (!strcmp(pOptions, "rb"))
	{
		std::string ReplacedFileName;
		if (ModelReplacer()->ReplaceFileName(pFileName, ReplacedFileName))
		{
			return gPrivateFuncs.FS_Open(ReplacedFileName.c_str(), pOptions);
		}
	}
	return gPrivateFuncs.FS_Open(pFileName, pOptions);
}

void Engine_InstallHooks()
{
	if (1)
	{
		const char sigs1[] = "Mod_NumForName: %s not found";
		auto Mod_NumForName_String = Search_Pattern_Data(sigs1);
		if (!Mod_NumForName_String)
			Mod_NumForName_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Mod_NumForName_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
		*(DWORD*)(pattern + 1) = (DWORD)Mod_NumForName_String;
		auto Mod_NumForName_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(Mod_NumForName_PushString);

		gPrivateFuncs.Mod_LoadModel = (decltype(gPrivateFuncs.Mod_LoadModel))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Mod_NumForName_PushString, 0x500, [](PUCHAR Candidate) {

			//.text:01D40B30 81 EC 0C 01 00 00                                   sub     esp, 10Ch
			if (Candidate[0] == 0x81 &&
				Candidate[1] == 0xEC &&
				Candidate[4] == 0x00 &&
				Candidate[5] == 0x00)
				return TRUE;

			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC)
				return TRUE;

			return FALSE;
		});

		Sig_FuncNotFound(Mod_LoadModel);

		typedef struct
		{
			int instCount_rb;
		}Mod_LoadModel_SearchContext;

		Mod_LoadModel_SearchContext ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.Mod_LoadModel, 0x1000, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (Mod_LoadModel_SearchContext*)context;

				if (!ctx->instCount_rb &&
					pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					(
						((PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)g_dwEngineDataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize) ||
						((PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)g_dwEngineRdataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineRdataBase + g_dwEngineRdataSize)
						))
				{
					auto pString = (PCHAR)pinst->detail->x86.operands[0].imm;
					if (!memcmp(pString, "rb", sizeof("rb") - 1))
					{
						ctx->instCount_rb = instCount;
					}
				}

				if (address[0] == 0xE8 && instLen == 5 && 
					ctx->instCount_rb && instCount > ctx->instCount_rb && instCount <= ctx->instCount_rb + 5)
				{
					gPrivateFuncs.FS_Open = (decltype(gPrivateFuncs.FS_Open))GetCallAddress(address);

					g_phook_Mod_LoadModel_FS_Open = g_pMetaHookAPI->InlinePatchRedirectBranch(address, Mod_LoadModel_FS_Open, NULL);
					return TRUE;
				}

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
		}, 0, &ctx);
	}

	if (1)
	{
		const char sigs1[] = "#GameUI_PrecachingResources";
		auto CL_PrecacheResources_String = Search_Pattern_Data(sigs1);
		if (!CL_PrecacheResources_String)
			CL_PrecacheResources_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(CL_PrecacheResources_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)CL_PrecacheResources_String;

		auto CL_PrecacheResources_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(CL_PrecacheResources_PushString);

		gPrivateFuncs.CL_PrecacheResources = (decltype(gPrivateFuncs.CL_PrecacheResources))g_pMetaHookAPI->ReverseSearchFunctionBegin(CL_PrecacheResources_PushString, 0x50);
		Sig_FuncNotFound(CL_PrecacheResources);
	}

	Install_InlineHook(CL_PrecacheResources);
}

void Engine_UninstallHooks()
{
	Uninstall_Hook(Mod_LoadModel_FS_Open);
	Uninstall_Hook(CL_PrecacheResources);
}

#if 0
FileHandle_t __fastcall FileSystem_Open(const char* pFileName, const char* pOptions, const char* pathID)
{


	return gPrivateFuncs.FileSystem_Open(pFileName, pOptions, pathID);
}

void FileSystem_InstallHooks()
{
	g_phook_FileSystem_Open = g_pMetaHookAPI->VFTHook(g_pFileSystem, 0, 10, FileSystem_Open, (void **) & gPrivateFuncs.FileSystem_Open);
}

void FileSystem_UninstallHooks()
{
	Uninstall_Hook(FileSystem_Open);
}
#endif