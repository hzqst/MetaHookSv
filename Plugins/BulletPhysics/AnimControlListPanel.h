#pragma once

#include <vgui_controls/ListPanel.h>

class CAnimControlListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CAnimControlListPanel, vgui::ListPanel);

	CAnimControlListPanel(vgui::Panel* parent, const char* pName);

private:

	typedef vgui::ListPanel BaseClass;
};
