#include <metahook.h>
#include <capstone.h>
#include <vector>
#include <set>
#include "plugins.h"
#include "privatehook.h"
#include "ResourceReplacer.h"

#define S_LOADSOUND_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\x8B\x8C\x24\x2A\x2A\x00\x00\x56\x8B\xB4\x24\x2A\x2A\x00\x00\x8A\x06\x3C\x2A"
#define S_LOADSOUND_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x34\x05\x00\x00\xA1"
#define S_LOADSOUND_SIG_8308 "\x55\x8B\xEC\x81\xEC\x28\x05\x00\x00\x53\x8B\x5D\x08"
#define S_LOADSOUND_SIG_NEW "\x55\x8B\xEC\x81\xEC\x44\x05\x00\x00\x53\x56\x8B\x75\x08"
#define S_LOADSOUND_SIG_BLOB "\x81\xEC\x2A\x2A\x00\x00\x53\x8B\x9C\x24\x2A\x2A\x00\x00\x55\x56\x8A\x03\x57"

private_funcs_t gPrivateFuncs = { 0 };

static hook_t* g_phook_S_LoadSound_FS_Open = NULL;
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

FileHandle_t S_LoadSound_FS_Open(const char* pFileName, const char* pOptions)
{
	if (!strcmp(pOptions, "rb"))
	{
		std::string ReplacedFileName;
		if (SoundReplacer()->ReplaceFileName(pFileName, ReplacedFileName))
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
		/*
.text:01D98912 68 F0 52 ED 01                                      push    offset aSLoadsoundCoul ; "S_LoadSound: Couldn't load %s\n"
.text:01D98917 E8 24 70 F9 FF                                      call    sub_1D2F940
.text:01D9891C 83 C4 08                                            add     esp, 8
		*/
		const char sigs[] = "S_LoadSound: Couldn't load %s";
		auto S_LoadSound_String = Search_Pattern_Data(sigs);
		if (!S_LoadSound_String)
			S_LoadSound_String = Search_Pattern_Rdata(sigs);
		if (S_LoadSound_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)S_LoadSound_String;
			auto S_LoadSound_PushString = (PUCHAR)Search_Pattern(pattern);
			if (S_LoadSound_PushString)
			{
				gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(S_LoadSound_PushString, 0x500, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
					{
						return TRUE;
					}

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC)
					{
						return TRUE;
					}

					//.text:01D98710 81 EC 48 05 00 00                                   sub     esp, 548h
					if (Candidate[0] == 0x81 &&
						Candidate[1] == 0xEC &&
						Candidate[4] == 0x00 &&
						Candidate[5] == 0x00)
					{
						return TRUE;
					}

					return FALSE;
				});
			}
		}
	}

	if (!gPrivateFuncs.S_LoadSound)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))Search_Pattern(S_LOADSOUND_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))Search_Pattern(S_LOADSOUND_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))Search_Pattern(S_LOADSOUND_SIG_NEW);
			if (!gPrivateFuncs.S_LoadSound)
				gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))Search_Pattern(S_LOADSOUND_SIG_8308);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.S_LoadSound = (decltype(gPrivateFuncs.S_LoadSound))Search_Pattern(S_LOADSOUND_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(S_LoadSound);

	if (1)
	{
		typedef struct
		{
			PVOID base;
			size_t max_insts;
			int max_depth;
			std::set<PVOID> code;
			std::set<PVOID> branches;
			std::vector<walk_context_t> walks;

			PVOID address_rb;
			int instCount_rb;
		}S_LoadSound_SearchContext;

		S_LoadSound_SearchContext ctx = { 0 };

		ctx.base = gPrivateFuncs.S_LoadSound;

		ctx.max_insts = 1000;
		ctx.max_depth = 16;
		ctx.walks.emplace_back(ctx.base, 0x1000, 0);

		while (ctx.walks.size())
		{
			auto walk = ctx.walks[ctx.walks.size() - 1];
			ctx.walks.pop_back();

			g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (S_LoadSound_SearchContext*)context;

				if (g_phook_S_LoadSound_FS_Open)
					return TRUE;

				if (ctx->code.size() > ctx->max_insts)
					return TRUE;

				if (ctx->code.find(address) != ctx->code.end())
					return TRUE;

				ctx->code.emplace(address);

				if (!ctx->address_rb &&
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
						ctx->address_rb = address;
					}
				}

				if (address[0] == 0xE8 && instLen == 5 &&
					ctx->address_rb && address > ctx->address_rb && address <= (PUCHAR)ctx->address_rb + 0x30 &&
					instCount > ctx->instCount_rb && instCount <= ctx->instCount_rb + 5)
				{
					if (!gPrivateFuncs.FS_Open)
					{
						gPrivateFuncs.FS_Open = (decltype(gPrivateFuncs.FS_Open))GetCallAddress(address);
					}
					g_phook_S_LoadSound_FS_Open = g_pMetaHookAPI->InlinePatchRedirectBranch(address, S_LoadSound_FS_Open, NULL);
					return TRUE;
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
							ctx->walks.emplace_back(imm, 0x300, depth + 1);
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

		if (!g_phook_S_LoadSound_FS_Open)
		{
			Sys_Error("S_LoadSound.FS_Open not found");
			return;
		}
	}

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
	}

	if (1)
	{
		typedef struct
		{
			PVOID base;
			size_t max_insts;
			int max_depth;
			std::set<PVOID> code;
			std::set<PVOID> branches;
			std::vector<walk_context_t> walks;

			int instCount_rb;
			PUCHAR address_rb;
		}Mod_LoadModel_SearchContext;

		Mod_LoadModel_SearchContext ctx = { 0 };

		ctx.base = gPrivateFuncs.Mod_LoadModel;

		ctx.max_insts = 1000;
		ctx.max_depth = 16;
		ctx.walks.emplace_back(ctx.base, 0x1000, 0);

		while (ctx.walks.size())
		{
			auto walk = ctx.walks[ctx.walks.size() - 1];
			ctx.walks.pop_back();

			g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (Mod_LoadModel_SearchContext*)context;

				if (g_phook_Mod_LoadModel_FS_Open)
					return TRUE;

				if (ctx->code.size() > ctx->max_insts)
					return TRUE;

				if (ctx->code.find(address) != ctx->code.end())
					return TRUE;

				ctx->code.emplace(address);

				if (!ctx->address_rb &&
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
					if (!memcmp(pString, "rb", sizeof("rb")))
					{
						ctx->instCount_rb = instCount;
						ctx->address_rb = address;
					}
				}

				if (address[0] == 0xE8 && instLen == 5 && ctx->address_rb
					&& instCount > ctx->instCount_rb && instCount <= ctx->instCount_rb + 5
					&& address > ctx->address_rb && address <= ctx->address_rb + 0x50)
				{
					if (!gPrivateFuncs.FS_Open)
					{
						gPrivateFuncs.FS_Open = (decltype(gPrivateFuncs.FS_Open))GetCallAddress(address);
					}

					g_phook_Mod_LoadModel_FS_Open = g_pMetaHookAPI->InlinePatchRedirectBranch(address, Mod_LoadModel_FS_Open, NULL);
					return TRUE;
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
							ctx->walks.emplace_back(imm, 0x300, depth + 1);
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

		if (!g_phook_Mod_LoadModel_FS_Open)
		{
			Sys_Error("Mod_LoadModel.FS_Open not found");
			return;
		}
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
	Uninstall_Hook(S_LoadSound_FS_Open);
	Uninstall_Hook(Mod_LoadModel_FS_Open);
	Uninstall_Hook(CL_PrecacheResources);
}