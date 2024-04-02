#include <metahook.h>
#include <capstone.h>
#include "plugins.h"
#include "privatehook.h"
#include "ThreadManager.h"

#include <IEngine.h>

private_funcs_t gPrivateFuncs = { 0 };

hook_t* g_pHook_FreeLibrary_Engine = NULL;
hook_t* g_pHook_FreeLibrary_GameUI = NULL;

IThreadManager* g_ThreadManager_Engine = NULL;
IThreadManager* g_ThreadManager_GameUI = NULL;
IThreadManager* g_ThreadManager_ServerBrowser = NULL;
IThreadManager* g_ThreadManager_ServerDLL = NULL;

IEngine** engine = NULL;

void ServerDLL_WaitForShutdown(HMODULE hModule);
void ServerBrowser_WaitForShutdown(HMODULE hModule);

#define DLL_INACTIVE 0		// no dll
#define DLL_ACTIVE   1		// dll is running
#define DLL_PAUSED   2		// dll is paused
#define DLL_CLOSE    3		// closing down dll
#define DLL_TRANS    4 		// Level Transition
#define DLL_RESTART  5      // engine is shutting down but will restart right away

int GetEngineDLLState()
{
	if (engine)
		return (*engine)->GetState();

	return DLL_INACTIVE;
}

BOOL WINAPI NewFreeLibrary_Engine(HMODULE hModule)
{
	if (g_ThreadManager_ServerDLL && g_ThreadManager_ServerDLL->GetModule() == hModule)
	{
		ServerDLL_WaitForShutdown(hModule);
	}

	return FreeLibrary(hModule);
}

BOOL WINAPI NewFreeLibrary_GameUI(HMODULE hModule)
{
	if (g_ThreadManager_ServerBrowser && g_ThreadManager_ServerBrowser->GetModule() == hModule)
	{
		ServerBrowser_WaitForShutdown(hModule);
	}

	return FreeLibrary(hModule);
}

void Engine_WaitForShutdown(HMODULE hModule, BlobHandle_t hBlobModule)
{
	if (g_ThreadManager_Engine && (GetEngineDLLState() == DLL_CLOSE || GetEngineDLLState() == DLL_RESTART))
	{
		g_ThreadManager_Engine->StartTermination();
		g_ThreadManager_Engine->WaitForAliveThreadsToShutdown();
	}
}

void Engine_InstallHook(HMODULE hModule, BlobHandle_t hBlobModule)
{
	if (hModule)
	{
		g_ThreadManager_Engine = CreateThreadManagerForModule(hModule);
		g_ThreadManager_Engine->InstallHook(hookflag_CreateThread | hookflag_WaitForSingleObject | hookflag_Sleep);

		if (0 == stricmp(g_pMetaHookAPI->GetGameDirectory(), "svencoop"))
		{
			g_pHook_FreeLibrary_Engine = g_pMetaHookAPI->IATHook(hModule, "kernel32.dll", "FreeLibrary", NewFreeLibrary_Engine, NULL);
		}
	}
	else if (hBlobModule)
	{
		g_ThreadManager_Engine = CreateThreadManagerForBlob(hBlobModule);
		g_ThreadManager_Engine->InstallHook(hookflag_CreateThread | hookflag_WaitForSingleObject | hookflag_Sleep);
	}

	if (1)
	{
		const char sigs1[] = "Sys_InitArgv( OrigCmd )";
		auto Sys_InitArgv_String = Search_Pattern_Data(sigs1);
		if (!Sys_InitArgv_String)
			Sys_InitArgv_String = Search_Pattern_Rdata(sigs1);
		if (Sys_InitArgv_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 1) = (DWORD)Sys_InitArgv_String;
			auto Sys_InitArgv_PushString = (PUCHAR)Search_Pattern(pattern);
			if (!Sys_InitArgv_PushString)
			{
				//Build 4554
				/*
.text:01DC57DE 68 84 C6 E8 01                                      push    offset aSysInitargvOri ; "Sys_InitArgv( OrigCmd )"
.text:01DC57E3 83 E1 03                                            and     ecx, 3
.text:01DC57E6 F3 A4                                               rep movsb
				*/
				char pattern2[] = "\x68\x2A\x2A\x2A\x2A";
				*(DWORD*)(pattern2 + 1) = (DWORD)Sys_InitArgv_String;
				Sys_InitArgv_PushString = (PUCHAR)Search_Pattern(pattern2);
			}
			if (Sys_InitArgv_PushString)
			{
				g_pMetaHookAPI->DisasmRanges(Sys_InitArgv_PushString, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
				{
					auto pinst = (cs_insn*)inst;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].mem.base == 0)
					{//A1 40 77 7B 02 mov     eax, gl_backbuffer_fbo
						ULONG_PTR imm = pinst->detail->x86.operands[1].mem.disp;

						if (imm >= (ULONG_PTR)g_dwEngineDataBase && imm < (ULONG_PTR)g_dwEngineDataBase + g_dwEngineDataSize)
						{
							engine = (decltype(engine))imm;
							return TRUE;
						}
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
				}, 0, NULL);
			}
		}
	}

	if (!engine)
	{
		Sys_Error("CEngine not found");
	}
}

void Engine_UninstallHook(HMODULE hModule, BlobHandle_t hBlobModule)
{
	if (g_pHook_FreeLibrary_Engine)
	{
		g_pMetaHookAPI->UnHook(g_pHook_FreeLibrary_Engine);
		g_pHook_FreeLibrary_Engine = NULL;
	}

	if (g_ThreadManager_Engine)
	{
		g_ThreadManager_Engine->UnistallHook();
		DeleteThreadManager(g_ThreadManager_Engine);
		delete g_ThreadManager_Engine;
		g_ThreadManager_Engine = NULL;
	}
}

void GameUI_InstallHook(HMODULE hModule)
{
	//It use callback instead of creating stupid thread.
	if (g_pMetaHookAPI->ModuleHasImport(hModule, "steam_api.dll") && !g_pMetaHookAPI->ModuleHasImportEx(hModule, "kernel32.dll", "CreateThread"))
		return;

	g_ThreadManager_GameUI = CreateThreadManagerForModule(hModule);
	g_ThreadManager_GameUI->InstallHook(hookflag_CreateThread | hookflag_WaitForSingleObject);

	g_pHook_FreeLibrary_GameUI = g_pMetaHookAPI->IATHook(hModule, "kernel32.dll", "FreeLibrary", NewFreeLibrary_GameUI, NULL);
}

void GameUI_UnistallHook(HMODULE hModule)
{
	if (g_pHook_FreeLibrary_GameUI)
	{
		g_pMetaHookAPI->UnHook(g_pHook_FreeLibrary_GameUI);
		g_pHook_FreeLibrary_GameUI = NULL;
	}

	if (g_ThreadManager_GameUI)
	{
		g_ThreadManager_GameUI->UnistallHook();
		DeleteThreadManager(g_ThreadManager_GameUI);
		delete g_ThreadManager_GameUI;
		g_ThreadManager_GameUI = NULL;
	}
}

void ServerDLL_WaitForShutdown(HMODULE hModule)
{
	if (g_ThreadManager_ServerDLL && (GetEngineDLLState() == DLL_CLOSE || GetEngineDLLState() == DLL_RESTART))
	{
		g_ThreadManager_ServerDLL->StartTermination();
		g_ThreadManager_ServerDLL->WaitForAliveThreadsToShutdown();
	}
}

void ServerDLL_InstallHook(HMODULE hModule)
{
	//It use callback instead of creating stupid thread.
	if (0 != stricmp(g_pMetaHookAPI->GetGameDirectory(), "svencoop"))
		return;

	//Fuck off the CPlayerDatabase_RunThread
	g_ThreadManager_ServerDLL = CreateThreadManagerForModule(hModule);
	g_ThreadManager_ServerDLL->InstallHook(hookflag_CreateThread);
}

void ServerDLL_UninstallHook(HMODULE hModule)
{
	if (g_ThreadManager_ServerDLL)
	{
		g_ThreadManager_ServerDLL->UnistallHook();
		DeleteThreadManager(g_ThreadManager_ServerDLL);
		delete g_ThreadManager_ServerDLL;
		g_ThreadManager_ServerDLL = NULL;
	}
}

void ServerBrowser_WaitForShutdown(HMODULE hModule)
{
	if (g_ThreadManager_ServerBrowser && (GetEngineDLLState() == DLL_CLOSE || GetEngineDLLState() == DLL_RESTART))
	{
		g_ThreadManager_ServerBrowser->StartTermination();
		g_ThreadManager_ServerBrowser->WaitForAliveThreadsToShutdown();
	}
}

void ServerBrowser_InstallHook(HMODULE hModule)
{
	//It use callback instead of creating stupid thread.
	if (g_pMetaHookAPI->ModuleHasImport(hModule, "steam_api.dll") && !g_pMetaHookAPI->ModuleHasImportEx(hModule, "kernel32.dll", "CreateThread"))
		return;

	g_ThreadManager_ServerBrowser = CreateThreadManagerForModule(hModule);
	g_ThreadManager_ServerBrowser->InstallHook(hookflag_CreateThread | hookflag_WaitForSingleObject);
}

void ServerBrowser_UninstallHook(HMODULE hModule)
{
	if (g_ThreadManager_ServerBrowser)
	{
		g_ThreadManager_ServerBrowser->UnistallHook();
		DeleteThreadManager(g_ThreadManager_ServerBrowser);
		delete g_ThreadManager_ServerBrowser;
		g_ThreadManager_ServerBrowser = NULL;
	}
}

void DllLoadNotification(mh_load_dll_notification_context_t* ctx)
{
	if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_LOAD)
	{
		if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_ENGINE)
		{
			Engine_InstallHook(ctx->hModule, ctx->hBlob);
		}
		else if (ctx->BaseDllName && ctx->hModule && !_wcsicmp(ctx->BaseDllName, L"GameUI.dll"))
		{
			GameUI_InstallHook(ctx->hModule);
		}
		else if (ctx->BaseDllName && ctx->hModule && !_wcsicmp(ctx->BaseDllName, L"ServerBrowser.dll"))
		{
			ServerBrowser_InstallHook(ctx->hModule);
		}
		else if (ctx->BaseDllName && ctx->hModule && !_wcsicmp(ctx->BaseDllName, L"server.dll"))
		{
			ServerDLL_InstallHook(ctx->hModule);
		}
	}
	else if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_UNLOAD)
	{
		if (ctx->flags & LOAD_DLL_NOTIFICATION_IS_ENGINE)
		{
			Engine_UninstallHook(ctx->hModule, ctx->hBlob);
		}
		else if (g_ThreadManager_GameUI && ctx->hModule == g_ThreadManager_GameUI->GetModule())
		{
			GameUI_UnistallHook(ctx->hModule);
		}
		else if (g_ThreadManager_ServerBrowser && ctx->hModule == g_ThreadManager_ServerBrowser->GetModule())
		{
			ServerBrowser_UninstallHook(ctx->hModule);
		}
		else if (g_ThreadManager_ServerDLL && ctx->hModule == g_ThreadManager_ServerDLL->GetModule())
		{
			ServerDLL_UninstallHook(ctx->hModule);
		}
	}
}