#include <metahook.h>
#include "Controls.h"
#include <locale.h>
#include <VGUI/IEngineVGui.h>
#include <IEngineSurface.h>
#include "plugins.h"

#include <capstone.h>

vgui::IInput *g_pVGuiInput = NULL;
vgui::ISystem *g_pVGuiSystem = NULL;
vgui::IVGui *g_pVGui = NULL;
vgui::IPanel *g_pVGuiPanel = NULL;
vgui::ILocalize *g_pVGuiLocalize = NULL;

vgui::ISurface *g_pSurface = NULL;
vgui::ISchemeManager *g_pScheme = NULL;
vgui::ISchemeManager_HL25 *g_pScheme_HL25 = NULL;

IFileSystem *g_pFullFileSystem = NULL;
IFileSystem_HL25 *g_pFullFileSystem_HL25 = NULL;
IKeyValuesSystem *g_pKeyValuesSystem = NULL;
vgui::IEngineVGui *g_pEngineVGui = NULL;

IEngineSurface *staticSurface = NULL;
IEngineSurface_HL25 *staticSurface_HL25 = NULL;

namespace vgui
{
static char g_szControlsModuleName[256];

bool (__fastcall *g_pfnCWin32Input_PostKeyMessage)(void *pthis, int, KeyValues *message);

bool VGui_InitInterfacesList(const char *moduleName, CreateInterfaceFn *factoryList, int numFactories)
{
	strncpy(g_szControlsModuleName, moduleName, sizeof(g_szControlsModuleName));
	g_szControlsModuleName[sizeof(g_szControlsModuleName) - 1] = 0;

	setlocale(LC_CTYPE, "");
	setlocale(LC_TIME, "");
	setlocale(LC_COLLATE, "");
	setlocale(LC_MONETARY, "");

	g_pFullFileSystem = (IFileSystem *)factoryList[2](FILESYSTEM_INTERFACE_VERSION, NULL);

	g_pVGuiInput = (IInput *)factoryList[1](VGUI_INPUT_INTERFACE_VERSION, NULL);
	g_pVGuiSystem = (ISystem *)factoryList[1](VGUI_SYSTEM_INTERFACE_VERSION, NULL);
	g_pVGui = (IVGui *)factoryList[1](VGUI_IVGUI_INTERFACE_VERSION, NULL);
	g_pVGuiPanel = (IPanel *)factoryList[1](VGUI_PANEL_INTERFACE_VERSION, NULL);
	g_pVGuiLocalize = (ILocalize *)factoryList[1](VGUI_LOCALIZE_INTERFACE_VERSION, NULL);

	g_pEngineVGui = (IEngineVGui *)factoryList[0](VENGINE_VGUI_VERSION, NULL);

	if (!g_pFullFileSystem || !g_pKeyValuesSystem || !g_pVGuiInput || !g_pVGuiSystem || !g_pVGui || !g_pVGuiPanel || !g_pVGuiLocalize)
	{
		g_pMetaHookAPI->SysError("vgui_controls is missing a required interface!\n");
		return false;
	}

	HMODULE hVGUI2 = GetModuleHandleA("vgui2.dll");

	if (1)
	{
		const char sigs1[] = "KeyCodeReleased";
		auto KeyCodeRelease_String = g_pMetaHookAPI->SearchPattern(hVGUI2, g_pMetaHookAPI->GetModuleSize(hVGUI2), sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(KeyCodeRelease_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x8B\xC8";
		*(DWORD *)(pattern + 6) = (DWORD)KeyCodeRelease_String;
		auto KeyCodeRelease_StringPush = g_pMetaHookAPI->SearchPattern(hVGUI2, g_pMetaHookAPI->GetModuleSize(hVGUI2), pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(KeyCodeRelease_StringPush);

		typedef struct
		{
			int iFoundPushEax;
		}KeyCodeRelease_ctx;

		KeyCodeRelease_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(KeyCodeRelease_StringPush, 0x250, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			KeyCodeRelease_ctx *ctx = (KeyCodeRelease_ctx *)context;
			if (!ctx->iFoundPushEax && pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX)
			{//.text:01D87E55 C7 01 B8 94 37 02                                   mov     dword ptr [ecx], offset pbodypart
				ctx->iFoundPushEax = 1;
				return FALSE;
			}

			if (ctx->iFoundPushEax)
			{
				if (address[0] == 0xE8 && instLen == 5)
				{
					g_pfnCWin32Input_PostKeyMessage = (decltype(g_pfnCWin32Input_PostKeyMessage))pinst->detail->x86.operands[0].imm;
				}
			}

			if (g_pfnCWin32Input_PostKeyMessage)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);
	}

	return true;
}

const char *GetControlsModuleName(void)
{
	return g_szControlsModuleName;
}

}