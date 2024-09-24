#pragma once

#include <vgui_controls/ListPanel.h>

class CPhysicActionListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicActionListPanel, vgui::ListPanel);

	CPhysicActionListPanel(vgui::Panel* parent, const char* pName);

private:

	typedef vgui::ListPanel BaseClass;
};
