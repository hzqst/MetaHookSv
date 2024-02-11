#ifndef ICOUNTERSTRIKEVIEWPORT_H
#define ICOUNTERSTRIKEVIEWPORT_H

#ifdef _WIN32
#pragma once
#endif

#include <ITeamFortressViewport.h>

class CounterStrikeViewport : public TeamFortressViewport
{
public:
#ifdef _WIN32
	virtual void SetNumberOfTeams(int numteams) = 0;
	virtual bool IsVGUIMenuActive(int iMenu) = 0;
	virtual bool IsAnyVGUIMenuActive() = 0;
	virtual void DisplayVGUIMenu(int iMenu) = 0;
#else
	virtual void Start() = 0;
	virtual void Shutdown() = 0;
	virtual void SetParent(vgui::VPANEL vpanel) = 0;
	virtual bool UseVGUI1() = 0;
	virtual void ActivateClientUI() = 0;
	virtual void HideClientUI() = 0;
	virtual void SetNumberOfTeams(int numteams) = 0;
	virtual int MsgFunc_ValClass(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_TeamNames(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_Feign(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_Detpack(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_VGUIMenu(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_TutorText(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_TutorLine(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_TutorState(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_TutorClose(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_MOTD(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_BuildSt(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_RandomPC(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_ServerName(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_ScoreInfo(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_TeamScore(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_TeamInfo(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_Spectator(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_AllowSpec(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_SpecFade(const char* pszName, int iSize, void* pbuf) = 0;
	virtual bool IsVGUIMenuActive(int iMenu) = 0;
	virtual bool IsAnyVGUIMenuActive() = 0;
	virtual void DisplayVGUIMenu(int iMenu) = 0;
	//Added in czero
	virtual int GetForceCamera() = 0;
	virtual int MsgFunc_ForceCam(const char* pszName, int iSize, void* pbuf) = 0;
	virtual int MsgFunc_Location(const char* pszName, int iSize, void* pbuf) = 0;
	virtual void UpdateBuyPresets() = 0;
	virtual void UpdateScheme() = 0;
	virtual bool IsProgressBarVisible() = 0;
	virtual void StartProgressBar(const char* name, int a2, int a3, bool a4) = 0;
	virtual void UpdateProgressBar(const char* name, int a2) = 0;
	virtual void StopProgressBar() = 0;
#endif
};

#endif