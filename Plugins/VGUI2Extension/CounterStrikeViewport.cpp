#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui.h>
#include <VGUI_controls/Controls.h>
#include <VGUI_controls/Panel.h>
#include <ICounterStrikeViewport.h>

static VGuiLibraryInterface_t* (__fastcall* m_pfnCounterStrikeViewpot_GetClientDllInterface)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_SetClientDllInterface)(void* pthis, int, VGuiLibraryInterface_t* clientInterface) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_UpdateScoreBoard)(void* pthis, int) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_AllowedToPrintText)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_GetAllPlayersInfo)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_DeathMsg)(void* pthis, int, int killer, int victim) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_ShowScoreBoard)(void* pthis, int) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_CanShowScoreBoard)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_HideAllVGUIMenu)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_UpdateSpectatorPanel)(void* pthis, int) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_IsScoreBoardVisible)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_HideScoreBoard)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_KeyInput)(void* pthis, int, int down, int keynum, const char* pszCurrentBinding) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_ShowVGUIMenu)(void* pthis, int, int iMenu) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_HideVGUIMenu)(void* pthis, int, int iMenu) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_ShowTutorTextWindow)(void* pthis, int, const wchar_t* szString, int id, int msgClass, int isSpectator) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_ShowTutorLine)(void* pthis, int, int entindex, int id) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_ShowTutorState)(void* pthis, int, const wchar_t* szString) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_CloseTutorTextWindow)(void* pthis, int) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_IsTutorTextWindowOpen)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_ShowSpectatorGUI)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_ShowSpectatorGUIBar)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_HideSpectatorGUI)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_DeactivateSpectatorGUI)(void* pthis, int) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_IsSpectatorGUIVisible)(void* pthis, int) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_IsSpectatorBarVisible)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_ResetFade)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_SetSpectatorBanner)(void* pthis, int, const char* image) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_SpectatorGUIEnableInsetView)(void* pthis, int, int value) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_ShowCommandMenu)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_UpdateCommandMenu)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_HideCommandMenu)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_IsCommandMenuVisible)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_GetValidClasses)(void* pthis, int, int iTeam) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_GetNumberOfTeams)(void* pthis, int, int iTeam) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_GetIsFeigning)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_GetIsSettingDetpack)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_GetBuildState)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_IsRandom)(void* pthis, int) = NULL;
static char* (__fastcall* m_pfnCounterStrikeViewpot_GetTeamName)(void* pthis, int, int iTeam) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_GetCurrentMenu)(void* pthis, int) = NULL;
static const char* (__fastcall* m_pfnCounterStrikeViewpot_GetMapName)(void* pthis, int) = NULL;
static const char* (__fastcall* m_pfnCounterStrikeViewpot_GetServerName)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_InputPlayerSpecial)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_OnTick)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_GetViewPortScheme)(void* pthis, int) = NULL;
static vgui::VPANEL(__fastcall* m_pfnCounterStrikeViewpot_GetViewPortPanel)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_GetAllowSpectators)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_OnLevelChange)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_HideBackGround)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_ChatInputPosition)(void* pthis, int, int* x, int* y) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_GetSpectatorBottomBarHeight)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_GetSpectatorTopBarHeight)(void* pthis, int) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_SlotInput)(void* pthis, int, int iSlot) = NULL;
static VGuiLibraryTeamInfo_t(__fastcall* m_pfnCounterStrikeViewpot_GetPlayerTeamInfo)(void* pthis, int, int playerIndex) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_MakeSafeName)(void* pthis, int, const char* oldName, char* newName, int newNameBufSize) = NULL;

static void(__fastcall* m_pfnCounterStrikeViewpot_Initialize)(void* pthis, int, CreateInterfaceFn* factories, int count) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_Start)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_Shutdown)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_SetParent)(void* pthis, int, vgui::VPANEL parent) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_UseVGUI1)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_ActivateClientUI)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_HideClientUI)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_SetNumberOfTeams)(void* pthis, int, int num) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_ValClass)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_TeamNames)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_Feign)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_Detpack)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_VGUIMenu)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_TutorText)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_TutorLine)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_TutorState)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_TutorClose)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_MOTD)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_BuildSt)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_RandomPC)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_ServerName)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_ScoreInfo)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_TeamScore)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_TeamInfo)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_Spectator)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_AllowSpec)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_SpecFade)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_IsVGUIMenuActive)(void* pthis, int, int iMenu) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_IsAnyVGUIMenuActive)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_DisplayVGUIMenu)(void* pthis, int, int iMenu) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_GetForceCamera)(void* pthis, int) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_ForceCam)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static int(__fastcall* m_pfnCounterStrikeViewpot_MsgFunc_Location)(void* pthis, int, const char* pszName, int iSize, void* pbuf) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_UpdateBuyPresets)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_UpdateScheme)(void* pthis, int) = NULL;
static bool(__fastcall* m_pfnCounterStrikeViewpot_IsProgressBarVisible)(void* pthis, int) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_StartProgressBar)(void* pthis, int, const char* title, int numTicks, int startTicks, bool isTimeBased) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_UpdateProgressBar)(void* pthis, int, const char* statusText, int tick) = NULL;
static void(__fastcall* m_pfnCounterStrikeViewpot_StopProgressBar)(void* pthis, int) = NULL;
