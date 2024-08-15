#pragma once

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <VGUI_controls/Controls.h>
#include <VGUI_controls/Panel.h>
#include <VGUI_controls/Frame.h>
#include <sequence.h>
#include <regex>

class CViewport : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CViewport, vgui::Panel);

public:
	CViewport();
	virtual ~CViewport(void);

public:
	//ClientVGUI Interface
	void Start(void);
	void Init(void);
	void VidInit(void);
	void Think(void);
	void Paint(void);
	void SetParent(vgui::VPANEL vPanel);
	void ConnectToServer(const char* game, int IP, int port);
	void ActivateClientUI(void);
	void HideClientUI(void);

private:
	CPhysicEditorDialog* m_pPhysicEditorDialog{};
};

extern CViewport *g_pViewPort;