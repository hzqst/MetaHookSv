#include <metahook.h>
#include "exportfuncs.h"
#include "privatefuncs.h"
#include "DpiManager.h"

//Steam API
#include <steam_api.h>

//VGUI2
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <VGUI_controls/Controls.h>

#include <functional>

cl_enginefunc_t gEngfuncs = { 0 };

int m_iIntermission = 0;

//client.dll
void *GameViewport = NULL;
int *g_iVisibleMouse = NULL;
void *gHud = NULL;

HWND g_MainWnd = NULL;
WNDPROC g_MainWndProc = NULL;

#if 0
void SDL_GetWindowSize(void* window, int* w, int* h)
{
	gPrivateFuncs.SDL_GetWindowSize(window, w, h);

	if (dpimanager()->IsScaling())
	{
		float flScalingFactor = (1.0f / dpimanager()->GetDpiScaling());

		(*w) *= flScalingFactor;
		(*h) *= flScalingFactor;
	}
}

void VGuiWrap2_Paint(void)
{
	dpimanager()->BeginScaling();

	gPrivateFuncs.VGuiWrap2_Paint();

	dpimanager()->EndScaling();
}

#endif

void COM_FixSlashes(char *pname)
{
#ifdef _WIN32
	while (*pname) {
		if (*pname == '/')
			*pname = '\\';
		pname++;
	}
#else
	while (*pname) {
		if (*pname == '\\')
			*pname = '/';
		pname++;
	}
#endif
}

#if 0

int FileSystem_SetGameDirectory(const char *pDefaultDir, const char *pGameDir)
{
	int result = gPrivateFuncs.FileSystem_SetGameDirectory(pDefaultDir, pGameDir);

	if (dpimanager()->IsHighDpiSupportEnabled())
	{
		char temp[1024];
		snprintf(temp, sizeof(temp), "%s\\%s_dpi%.0f", GetBaseDirectory(), gEngfuncs.pfnGetGameDirectory(), dpimanager()->GetDpiScaling() * 100.0f);
		COM_FixSlashes(temp);

		g_pFileSystem->AddSearchPathNoWrite(temp, "SKIN");
	}

	return result;
}

#endif

const char *GetBaseDirectory()
{
	return (const char *)(*hostparam_basedir);
}

IBaseInterface* CreateInterfaceProxy(const char* pName, int* pReturnCode)
{
	auto ret = CreateInterface(pName, pReturnCode);
	if (ret)
		return ret;

	auto CreateInterfaceClientDll = (decltype(CreateInterfaceProxy)*)gExportfuncs.ClientFactory();
	return CreateInterfaceClientDll(pName, pReturnCode);
}

void *NewClientFactory(void)
{
	return CreateInterfaceProxy;
}

int HUD_VidInit(void)
{
	int result = gExportfuncs.HUD_VidInit();

	return result;
}

int HUD_Redraw(float time, int intermission)
{
	m_iIntermission = intermission;

	return gExportfuncs.HUD_Redraw(time, intermission);
}

void HUD_Shutdown(void)
{
	Client_UninstallHooks();

	gExportfuncs.HUD_Shutdown();
}

void HUD_Frame(double time)
{
	//Update resolution?
	g_pMetaHookAPI->GetVideoMode(&g_iVideoWidth, &g_iVideoHeight, NULL, NULL);

	gExportfuncs.HUD_Frame(time);
}

void IN_MouseEvent(int mstate)
{
	if (g_iVisibleMouse && vgui::surface()->IsCursorVisible())
	{
		int iVisibleMouse = *g_iVisibleMouse;
		*g_iVisibleMouse = 1;

		gExportfuncs.IN_MouseEvent(mstate);

		*g_iVisibleMouse = iVisibleMouse;
	}
	else
	{
		gExportfuncs.IN_MouseEvent(mstate);
	}
}

void IN_Accumulate(void)
{
	if (g_iVisibleMouse && vgui::surface()->IsCursorVisible())
	{
		int iVisibleMouse = *g_iVisibleMouse;
		*g_iVisibleMouse = 1;

		gExportfuncs.IN_Accumulate();

		*g_iVisibleMouse = iVisibleMouse;
	}
	else
	{
		gExportfuncs.IN_Accumulate();
	}
}

void CL_CreateMove(float frametime, struct usercmd_s *cmd, int active)
{
	if (g_iVisibleMouse && vgui::surface()->IsCursorVisible())
	{
		int iVisibleMouse = *g_iVisibleMouse;
		*g_iVisibleMouse = 1;

		gExportfuncs.CL_CreateMove(frametime, cmd, active);

		*g_iVisibleMouse = iVisibleMouse;
	}
	else
	{
		gExportfuncs.CL_CreateMove(frametime, cmd, active);
	}
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();
}

IBaseInterface *NewCreateInterface(const char *pName, int *pReturnCode)
{
	auto pfnCreateInterface = (decltype(NewCreateInterface) *)Sys_GetFactoryThis();
	auto pInterface = pfnCreateInterface(pName, pReturnCode);
	if (pInterface)
		return pInterface;

	pfnCreateInterface = (decltype(NewCreateInterface) *)GetProcAddress(g_hClientDll, CREATEINTERFACE_PROCNAME);
	if (pfnCreateInterface)
	{
		pInterface = pfnCreateInterface(pName, pReturnCode);
		if (pInterface)
			return pInterface;
	}

	return NULL;
}

#if defined(_WIN32)
void Sys_GetRegKeyValueUnderRoot(HKEY rootKey, const char *pszSubKey, const char *pszElement, char *pszReturnString, int nReturnLength, const char *pszDefaultValue)
{
	LONG lResult;           // Registry function result code
	HKEY hKey;              // Handle of opened/created key
	char szBuff[128];       // Temp. buffer
	DWORD dwDisposition;    // Type of key opening event
	DWORD dwType;           // Type of key
	DWORD dwSize;           // Size of element data

	// Assume the worst
	Q_snprintf(pszReturnString, nReturnLength, pszDefaultValue);

	// Create it if it doesn't exist.  (Create opens the key otherwise)
	lResult = RegCreateKeyEx(
		rootKey,	// handle of open key 
		pszSubKey,			// address of name of subkey to open 
		0,					// DWORD ulOptions,	  // reserved 
		"String",			// Type of value
		REG_OPTION_NON_VOLATILE, // Store permanently in reg.
		KEY_ALL_ACCESS,		// REGSAM samDesired, // security access mask 
		NULL,
		&hKey,				// Key we are creating
		&dwDisposition);    // Type of creation

	if (lResult != ERROR_SUCCESS)  // Failure
		return;

	// First time, just set to Valve default
	if (dwDisposition == REG_CREATED_NEW_KEY)
	{
		// Just Set the Values according to the defaults
		lResult = RegSetValueEx(hKey, pszElement, 0, REG_SZ, (CONST BYTE *)pszDefaultValue, Q_strlen(pszDefaultValue) + 1);
	}
	else
	{
		// We opened the existing key. Now go ahead and find out how big the key is.
		dwSize = nReturnLength;
		lResult = RegQueryValueEx(hKey, pszElement, 0, &dwType, (unsigned char *)szBuff, &dwSize);

		// Success?
		if (lResult == ERROR_SUCCESS)
		{
			// Only copy strings, and only copy as much data as requested.
			if (dwType == REG_SZ)
			{
				Q_strncpy(pszReturnString, szBuff, nReturnLength);
				pszReturnString[nReturnLength - 1] = '\0';
			}
		}
		else
			// Didn't find it, so write out new value
		{
			// Just Set the Values according to the defaults
			lResult = RegSetValueEx(hKey, pszElement, 0, REG_SZ, (CONST BYTE *)pszDefaultValue, Q_strlen(pszDefaultValue) + 1);
		}
	};

	// Always close this key before exiting.
	RegCloseKey(hKey);
}

void Sys_GetRegKeyValue(char *pszSubKey, char *pszElement, char *pszReturnString, int nReturnLength, char *pszDefaultValue)
{
	Sys_GetRegKeyValueUnderRoot(HKEY_CURRENT_USER, pszSubKey, pszElement, pszReturnString, nReturnLength, pszDefaultValue);
}

#endif

char * NewV_strncpy(char *a1, const char *a2, size_t a3)
{
	char language[128] = { 0 };
	const char *pszLanguage = NULL;
	auto szGameDir = gEngfuncs.pfnGetGameDirectory();

	if (CommandLine()->CheckParm("-forcelang", &pszLanguage) && pszLanguage && pszLanguage[0])
	{
		a2 = pszLanguage;
	}

	else if ((szGameDir && !strcmp(szGameDir, "svencoop")) || CommandLine()->CheckParm("-steamlang"))
	{
		Sys_GetRegKeyValue("Software\\Valve\\Steam", "Language", language, sizeof(language), "");

		if ((Q_strlen(language) > 0) && (0 != Q_stricmp(language, "english")))
		{
			a2 = language;
		}
	}

	gPrivateFuncs.V_strncpy(m_szCurrentGameLanguage, a2, sizeof(m_szCurrentGameLanguage) - 1);
	m_szCurrentGameLanguage[sizeof(m_szCurrentGameLanguage) - 1] = 0;

	return gPrivateFuncs.V_strncpy(a1, a2, a3);
}

bool g_bIMEComposing = false;
double g_flImeComposingTime = 0;

double GetAbsoluteTime()
{
	return gEngfuncs.GetAbsoluteTime();
}

LRESULT WINAPI VID_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND s_hLastHWnd;
	if (hWnd != s_hLastHWnd)
	{
		s_hLastHWnd = hWnd;
		vgui::input()->SetIMEWindow(hWnd);
	}

	switch (uMsg)
	{
	case WM_SYSCHAR:
	case WM_CHAR:
	{
		if (g_bIMEComposing)
			return 1;

		break;
	}
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		if (wParam == VK_BACK)
		{
			if (g_bIMEComposing)
				return 1;
		}

		break;
	}
	case WM_INPUTLANGCHANGE:
	{
		vgui::input()->OnInputLanguageChanged();
		return 1;
	}

	case WM_IME_STARTCOMPOSITION:
	{
		g_bIMEComposing = true;
		g_flImeComposingTime = GetAbsoluteTime();
		vgui::input()->OnIMEStartComposition();
		return 1;
	}

	case WM_IME_COMPOSITION:
	{
		int flags = (int)lParam;
		vgui::input()->OnIMEComposition(flags);
		return 1;
	}

	case WM_IME_ENDCOMPOSITION:
	{
		g_bIMEComposing = false;
		g_flImeComposingTime = GetAbsoluteTime();
		vgui::input()->OnIMEEndComposition();
		return 1;
	}

	case WM_IME_NOTIFY:
	{
		switch (wParam)
		{
		case IMN_OPENCANDIDATE:
		{
			vgui::input()->OnIMEShowCandidates();
			return 1;
		}

		case IMN_CHANGECANDIDATE:
		{
			vgui::input()->OnIMEChangeCandidates();
			return 1;
		}

		case IMN_CLOSECANDIDATE:
		{
			vgui::input()->OnIMECloseCandidates();
			//break;
			return 1;
		}

		case IMN_SETCONVERSIONMODE:
		case IMN_SETSENTENCEMODE:
		case IMN_SETOPENSTATUS:
		{
			vgui::input()->OnIMERecomputeModes();
			break;
		}

		case IMN_CLOSESTATUSWINDOW:
		case IMN_GUIDELINE:
		case IMN_OPENSTATUSWINDOW:
		case IMN_SETCANDIDATEPOS:
		case IMN_SETCOMPOSITIONFONT:
		case IMN_SETCOMPOSITIONWINDOW:
		case IMN_SETSTATUSWINDOWPOS:
		{
			break;
		}
		}

		break;
	}

	case WM_IME_SETCONTEXT:
	{
		lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
		lParam &= ~ISC_SHOWUIGUIDELINE;
		lParam &= ~ISC_SHOWUIALLCANDIDATEWINDOW;
		break;
	}

	case WM_IME_CHAR:
	{
		return 0;
	}
	}

	return CallWindowProc(g_MainWndProc, hWnd, uMsg, wParam, lParam);
}

void InitWin32Stuffs(void)
{
	EnumWindows([](HWND hwnd, LPARAM lParam){
		DWORD pid = 0;
		if (GetWindowThreadProcessId(hwnd, &pid) && pid == GetCurrentProcessId())
		{
			char windowClass[256] = { 0 };
			RealGetWindowClassA(hwnd, windowClass, sizeof(windowClass));
			if (!strcmp(windowClass, "Valve001") || !strcmp(windowClass, "SDL_app"))
			{
				g_MainWnd = hwnd;

				dpimanager()->InitFromMainHwnd();
				
				return FALSE;
			}
		}
		return TRUE;
	}, NULL);

	g_MainWndProc = (WNDPROC)GetWindowLong(g_MainWnd, GWL_WNDPROC);
	SetWindowLong(g_MainWnd, GWL_WNDPROC, (LONG)VID_MainWndProc);
}

void RemoveFileExtension(std::string& filePath)
{
	// Find the last occurrence of '.'
	size_t lastDotPosition = filePath.find_last_of(".");

	// Check if the dot is part of a directory component rather than an extension
	size_t lastPathSeparator = filePath.find_last_of("/\\");

	if (lastDotPosition != std::string::npos) {
		// Ensure the dot is after the last path separator
		if (lastPathSeparator != std::string::npos && lastDotPosition < lastPathSeparator) {
			return; // Dot is part of a directory name, not an extension
		}
		// Return the substring from the beginning to the dot
		filePath = filePath.substr(0, lastDotPosition);
	}

	// No extension found, return the original path
}
