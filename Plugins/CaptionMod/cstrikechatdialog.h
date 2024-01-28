#ifndef CSTRIKECHATDIALOG_H
#define CSTRIKECHATDIALOG_H

#ifdef _WIN32
#pragma once
#endif

#include "chatdialog.h"

class CCSChatDialog : public CChatDialog
{
	DECLARE_CLASS_SIMPLE(CCSChatDialog, CChatDialog);

public:
	CCSChatDialog(Panel *parent, const char* panelName);

public:
	virtual void CreateChatInputLine(void);
	virtual void CreateChatLines(void);
	virtual void Init(void);
	virtual void VidInit(void);
	virtual void Reset(void);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

public:
	virtual void OnThink(void);

public:
	int GetChatInputOffset(void);
	void SetVisible(bool state);

public:
	int m_iSaveX, m_iSaveY;
};

#endif