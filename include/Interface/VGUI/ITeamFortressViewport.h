#ifndef ITEAMFORTRESSVIEWPORT_H
#define ITEAMFORTRESSVIEWPORT_H

#ifdef _WIN32
#pragma once
#endif

#include <VGUI/VGUI.h>
#include <VGUI/VGuiLibrary.h>

class TeamFortressViewport
{
public:
	virtual VGuiLibraryInterface_t* GetClientDllInterface(void) = 0;
	virtual void SetClientDllInterface(VGuiLibraryInterface_t* clientInterface) = 0;
	virtual void UpdateScoreBoard(void) = 0;
	virtual bool AllowedToPrintText(void) = 0;
	virtual void GetAllPlayersInfo(void) = 0;
	virtual void DeathMsg(int killer, int victim) = 0;
	virtual void ShowScoreBoard(void) = 0;
	virtual bool CanShowScoreBoard(void) = 0;
	virtual void HideAllVGUIMenu(void) = 0;
	virtual void UpdateSpectatorPanel(void) = 0;
	virtual bool IsScoreBoardVisible(void) = 0;
	virtual void HideScoreBoard(void) = 0;
	virtual int KeyInput(int down, int keynum, const char* pszCurrentBinding) = 0;
	virtual void ShowVGUIMenu(int iMenu) = 0;
	virtual void HideVGUIMenu(int iMenu) = 0;
	virtual void ShowTutorTextWindow(const wchar_t* szString, int id, int msgClass, int isSpectator) = 0;
	virtual void ShowTutorLine(int entindex, int id) = 0;
	virtual void ShowTutorState(const wchar_t* szString) = 0;
	virtual void CloseTutorTextWindow(void) = 0;
	virtual bool IsTutorTextWindowOpen(void) = 0;
	virtual void ShowSpectatorGUI(void) = 0;
	virtual void ShowSpectatorGUIBar(void) = 0;
	virtual void HideSpectatorGUI(void) = 0;
	virtual void DeactivateSpectatorGUI(void) = 0;
	virtual bool IsSpectatorGUIVisible(void) = 0;
	virtual bool IsSpectatorBarVisible(void) = 0;
	virtual int MsgFunc_ResetFade(const char* pszName, int iSize, void* pbuf) = 0;
	virtual void SetSpectatorBanner(const char* image) = 0;
	virtual void SpectatorGUIEnableInsetView(int value) = 0;
	virtual void ShowCommandMenu(void) = 0;
	virtual void UpdateCommandMenu(void) = 0;
	virtual void HideCommandMenu(void) = 0;
	virtual int IsCommandMenuVisible(void) = 0;
	virtual int GetValidClasses(int iTeam) = 0;
	virtual int GetNumberOfTeams(int iTeam) = 0;
	virtual bool GetIsFeigning(void) = 0;
	virtual int GetIsSettingDetpack(void) = 0;
	virtual int GetBuildState(void) = 0;
	virtual int IsRandomPC(void) = 0;
	virtual const char* GetTeamName(int iTeam) = 0;
	virtual int GetCurrentMenu(void) = 0;
	virtual const char* GetMapName(void) = 0;
	virtual const char* GetServerName(void) = 0;
	virtual void InputPlayerSpecial(void) = 0;
	virtual void OnTick(void) = 0;
	virtual int GetViewPortScheme(void) = 0;
	virtual vgui::VPANEL GetViewPortPanel(void) = 0;
	virtual int GetAllowSpectators(void) = 0;
	virtual void OnLevelChange(void) = 0;
	virtual void HideBackGround(void) = 0;
	virtual void ChatInputPosition(int* x, int* y) = 0;
	virtual int GetSpectatorBottomBarHeight(void) = 0;
	virtual int GetSpectatorTopBarHeight(void) = 0;
	virtual bool SlotInput(int iSlot) = 0;
	virtual VGuiLibraryTeamInfo_t *GetPlayerTeamInfo(int playerIndex) = 0;
	virtual void MakeSafeName(const char* oldName, char* newName, int newNameBufSize) = 0;
};

#endif