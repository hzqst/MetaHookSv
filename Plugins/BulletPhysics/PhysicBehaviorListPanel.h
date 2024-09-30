#pragma once

#include <vgui_controls/ListPanel.h>

class CPhysicBehaviorListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicBehaviorListPanel, vgui::ListPanel);

	CPhysicBehaviorListPanel(vgui::Panel* parent, const char* pName);

private:

	typedef vgui::ListPanel BaseClass;
};
