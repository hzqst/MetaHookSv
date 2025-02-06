#pragma once

#include <vgui_controls/ListPanel.h>

class CTaskListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CTaskListPanel, vgui::ListPanel);

	CTaskListPanel(vgui::Panel* parent, const char* pName);

private:

	typedef vgui::ListPanel BaseClass;
};
