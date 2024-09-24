#pragma once

#include <vgui_controls/ListPanel.h>

class CPhysicConstraintListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicConstraintListPanel, vgui::ListPanel);

	CPhysicConstraintListPanel(vgui::Panel* parent, const char* pName);

private:

	typedef vgui::ListPanel BaseClass;
};