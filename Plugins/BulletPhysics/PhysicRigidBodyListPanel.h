#pragma once

#include <vgui_controls/ListPanel.h>

class CPhysicRigidBodyListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicRigidBodyListPanel, vgui::ListPanel);

	CPhysicRigidBodyListPanel(vgui::Panel* parent, const char* pName);

private:

	typedef vgui::ListPanel BaseClass;
};