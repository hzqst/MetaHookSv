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

class CPhysicEditorDialog;
class CPhysicDebugViewGUI;
typedef model_s model_t;

class CViewport : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CViewport, vgui::Panel);

public:
	CViewport();
	virtual ~CViewport(void);

public:
	void Think(void) override;
	void Paint(void) override;
	void SetParent(vgui::VPANEL vPanel) override;

public:
	void Start(void);
	void Init(void);
	void NewMap(void);
	void VidInit(void);
	void ActivateClientUI(void);
	void HideClientUI(void);

	void UpdateInspectEntity(int entindex);
	void UpdateInspectPhysicComponent(int physicComponentId);

private:
	CPhysicEditorDialog* m_pPhysicEditorDialog{};
	CPhysicDebugViewGUI* m_pPhysicDebugViewGUI{};

	int m_iCachedInspectEntity{};
	model_t * m_iCachedInspectModel{};

	int m_iCachedInspectPhysicComponentId{};
};

extern CViewport *g_pViewPort;