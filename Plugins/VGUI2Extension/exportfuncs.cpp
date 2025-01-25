#include <metahook.h>
#include "exportfuncs.h"
#include "privatefuncs.h"
#include "DpiManagerInternal.h"

//VGUI2
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <VGUI_controls/Controls.h>

#include <functional>

//#include <SDL2/SDL_events.h>
#include <SDL3/SDL_events.h>

#define SDL2_TEXTEDITINGEVENT_TEXT_SIZE (32)
typedef struct SDL2_TextEditingEvent
{
	Uint32 type;
	Uint32 timestamp;
	Uint32 windowID;
	char text[SDL2_TEXTEDITINGEVENT_TEXT_SIZE];
	Sint32 start;
	Sint32 length;
} SDL2_TextEditingEvent;

typedef struct SDL2_TextEditingCandidatesEvent
{
	SDL_EventType type;         /**< SDL_EVENT_TEXT_EDITING_CANDIDATES */
	Uint32 timestamp;
	SDL_WindowID windowID;      /**< The window with keyboard focus, if any */
	const char* const* candidates;    /**< The list of candidates, or NULL if there are no candidates available */
	Sint32 num_candidates;      /**< The number of strings in `candidates` */
	Sint32 selected_candidate;  /**< The index of the selected candidate, or -1 if no candidate is selected */
	bool horizontal;          /**< true if the list is horizontal, false if it's vertical */
	Uint8 padding1;
	Uint8 padding2;
	Uint8 padding3;
} SDL2_TextEditingCandidatesEvent;

#include "VGUI2ExtensionInternal.h"


cl_enginefunc_t gEngfuncs = { 0 };

int m_iIntermission = 0;

//client.dll
void *GameViewport = NULL;
int *g_iVisibleMouse = NULL;
void *gHud = NULL;

HWND g_MainWnd = NULL;
WNDPROC g_MainWndProc = NULL;

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
	NativeClientUI_UninstallHooks();
	Client_UninstallHooks();

	gExportfuncs.HUD_Shutdown();
}

void HUD_Frame(double time)
{
	//Update resolution?

	gExportfuncs.HUD_Frame(time);
}

void IN_MouseEvent(int mstate)
{
	if (g_iVisibleMouse && vgui::surface() && vgui::surface()->IsCursorVisible())
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
	if (g_iVisibleMouse && vgui::surface() && vgui::surface()->IsCursorVisible())
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
	if (g_iVisibleMouse && vgui::surface() && vgui::surface()->IsCursorVisible())
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
	VGUI1_PostInstallHooks();

	gExportfuncs.HUD_Init();
}

IBaseInterface *NewCreateInterface(const char *pName, int *pReturnCode)
{
	auto pfnCreateInterface = (decltype(NewCreateInterface) *)Sys_GetFactoryThis();
	auto pInterface = pfnCreateInterface(pName, pReturnCode);
	if (pInterface)
		return pInterface;

	pfnCreateInterface = (decltype(NewCreateInterface) *)GetProcAddress(g_hClientModule, CREATEINTERFACE_PROCNAME);
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

double engine_GetAbsoluteTime()
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

	return CallWindowProc(g_MainWndProc, hWnd, uMsg, wParam, lParam);
}

HWND Sys_GetMainWindowWin32HWND()
{
#ifdef _WIN32
	if (Sys_GetMainWindow())
	{
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo((SDL_Window*)Sys_GetMainWindow(), &wmInfo);

		return wmInfo.info.win.window;
	}
#else

#endif
	return nullptr;
}

class CVGUI2Extension_BaseUICallbacks : public IVGUI2Extension_BaseUICallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	void Initialize(CreateInterfaceFn* factories, int count) override
	{
		
	}

	void Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion) override
	{

	}

	void Shutdown(void)
	{

	}

	void Key_Event(int& down, int& keynum, const char*& pszCurrentBinding, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void CallEngineSurfaceAppProc(void*& pevent, void*& userData, VGUI2Extension_CallbackContext* CallbackContext) override
	{
#if 0
		const auto pSDLEvent = (const SDL_Event *)pevent;

		switch (pSDLEvent->type)
		{
		case SDL_EVENT_TEXT_EDITING_CANDIDATES:
		{
			const auto pTextEditingCandidateEvent = (const SDL2_TextEditingCandidatesEvent *)pSDLEvent;

			vgui::input()->OnIMECandidateSDL(
				pTextEditingCandidateEvent->candidates,
				pTextEditingCandidateEvent->num_candidates, 
				pTextEditingCandidateEvent->selected_candidate);

			CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
			break;
		}
		case SDL_EVENT_TEXT_INPUT:
		{
			//Already captured by BaseUISurface::AppHandler
			//So we do nothing here.
			break;
		}
		case SDL_EVENT_TEXT_EDITING:
		{
			/*
			Update the composition text.
			Update the cursor position.
			Update the selection length (if any).
			*/
			const auto pTextEditingEvent = (const SDL2_TextEditingEvent *)pSDLEvent;
			
			vgui::input()->OnIMECompositionSDL(pTextEditingEvent->text, pTextEditingEvent->start, pTextEditingEvent->length);

			CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
			break;
		}
		}
#endif

	}

	void CallEngineSurfaceWndProc(void*& hwnd, unsigned int& msg, unsigned int& wparam, long& lparam, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		switch (msg)
		{
		case WM_SYSCHAR:
		case WM_CHAR:
		{
			if (vgui::input()->IsIMEComposing())
			{
				CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
				break;
			}

			break;
		}
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			if (wparam == VK_BACK)
			{
				if (vgui::input()->IsIMEComposing())
				{
					CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
					break;
				}
			}

			break;
		}
		case WM_INPUTLANGCHANGE:
		{
			vgui::input()->SetIMEWindow(hwnd);
			vgui::input()->OnInputLanguageChanged();
			CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
			break;
		}

		case WM_IME_STARTCOMPOSITION:
		{
			vgui::input()->SetIMEWindow(hwnd);
			vgui::input()->OnIMEStartComposition();
			CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
			break;
		}

		case WM_IME_COMPOSITION:
		{
			vgui::input()->SetIMEWindow(hwnd);
			vgui::input()->OnIMECompositionWin32(lparam);
			CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
			break;
		}

		case WM_IME_ENDCOMPOSITION:
		{
			vgui::input()->SetIMEWindow(hwnd);
			vgui::input()->OnIMEEndComposition();
			CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
			break;
		}

		case WM_IME_NOTIFY:
		{
			switch (wparam)
			{
			case IMN_OPENCANDIDATE:
			{
				vgui::input()->SetIMEWindow(hwnd);
				vgui::input()->OnIMEShowCandidates();
				CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
				break;
			}

			case IMN_CHANGECANDIDATE:
			{
				vgui::input()->SetIMEWindow(hwnd);
				vgui::input()->OnIMEChangeCandidates();
				CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
				break;
			}

			case IMN_CLOSECANDIDATE:
			{
				vgui::input()->SetIMEWindow(hwnd);
				vgui::input()->OnIMECloseCandidates();
				CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
				break;
			}

			case IMN_SETCONVERSIONMODE:
			case IMN_SETSENTENCEMODE:
			case IMN_SETOPENSTATUS:
			{
				vgui::input()->SetIMEWindow(hwnd);
				vgui::input()->OnIMERecomputeModes();
				CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
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
			lparam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
			lparam &= ~ISC_SHOWUIGUIDELINE;
			lparam &= ~ISC_SHOWUIALLCANDIDATEWINDOW;
			break;
		}

		case WM_IME_CHAR:
		{
			CallbackContext->Result = VGUI2Extension_Result::SUPERCEDE;
		}
		}
	}

	void Paint(int& x, int& y, int& right, int& bottom, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HideGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ActivateGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HideConsole(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ShowConsole(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}
};

static CVGUI2Extension_BaseUICallbacks s_BaseUICallbacks_IMEHandler;

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

				DpiManagerInternal()->InitFromHwnd(g_MainWnd);
				
				return FALSE;
			}
		}
		return TRUE;
	}, NULL);

	VGUI2ExtensionInternal()->RegisterBaseUICallbacks(&s_BaseUICallbacks_IMEHandler);

	//g_MainWndProc = (WNDPROC)GetWindowLong(g_MainWnd, GWL_WNDPROC);
	//SetWindowLong(g_MainWnd, GWL_WNDPROC, (LONG)VID_MainWndProc);
}