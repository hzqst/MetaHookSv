#include <metahook.h>
#include "CvarToggleCheckButton.h"
#include <vgui/IVGui.h>
#include "tier1/KeyValues.h"
#include "IGameUIFuncs.h"

using namespace vgui;

vgui::Panel *CvarToggleCheckButton_Factory(void)
{
	return new CCvarToggleCheckButton(NULL, NULL, "CvarToggleCheckButton", NULL);
}

DECLARE_BUILD_FACTORY_CUSTOM(CCvarToggleCheckButton, CvarToggleCheckButton_Factory);

CCvarToggleCheckButton::CCvarToggleCheckButton(Panel *parent, const char *panelName, const char *text, char const *cvarname) : CheckButton(parent, panelName, text)
{
	m_bStartValue = false;

	if (cvarname)
	{
		Q_strncpy(m_szCvarName, cvarname, 63);
		m_szCvarName[63] = 0;
	}
	else
	{
		m_szCvarName[0] = 0;
	}

	Reset();

	AddActionSignalTarget(this);
}

CCvarToggleCheckButton::~CCvarToggleCheckButton(void)
{
	m_szCvarName[0] = 0;
}

void CCvarToggleCheckButton::Paint(void)
{
	if (!m_szCvarName[0])
	{
		BaseClass::Paint();
		return;
	}

	bool value = gEngfuncs.pfnGetCvarFloat(m_szCvarName) > 0.0f ? true : false;

	if (value != m_bStartValue)
	{
		SetSelected(value);
		m_bStartValue = value;
	}

	BaseClass::Paint();
}

void CCvarToggleCheckButton::ApplyChanges(void)
{
	if (!m_szCvarName[0])
		return;

	m_bStartValue = IsSelected();
	gEngfuncs.Cvar_SetValue(m_szCvarName, m_bStartValue ? 1.0f : 0.0f);
}

void CCvarToggleCheckButton::Reset(void)
{
	if (!m_szCvarName[0])
		return;

	auto cvar = gEngfuncs.pfnGetCvarPointer(m_szCvarName);

	if (!cvar)
	{
		SetEnabled(false);
		return;
	}

	m_bStartValue = gEngfuncs.pfnGetCvarFloat(m_szCvarName) > 0.0f ? true : false;
	SetSelected(m_bStartValue);
}

bool CCvarToggleCheckButton::HasBeenModified(void)
{
	return IsSelected() != m_bStartValue;
}

const char *CCvarToggleCheckButton::GetCvarName(void)
{
	return m_szCvarName;
}

void CCvarToggleCheckButton::SetSelected(bool state)
{
	BaseClass::SetSelected(state);
}

void CCvarToggleCheckButton::OnButtonChecked(void)
{
	if (HasBeenModified())
		PostActionSignal(new KeyValues("ControlModified"));
}

void CCvarToggleCheckButton::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	const char *cvarName = inResourceData->GetString("cvar_name", "");
	const char *cvarValue = inResourceData->GetString("cvar_value", "");

	if (Q_stricmp(cvarName, "") == 0)
		return;

	Q_strncpy(m_szCvarName, cvarName, 63);
	m_szCvarName[63] = 0;

	if (Q_stricmp(cvarValue, "1") == 0)
		m_bStartValue = true;
	else
		m_bStartValue = false;

	if (gEngfuncs.pfnGetCvarFloat(m_szCvarName) != 0)
		SetSelected(true);
	else
		SetSelected(false);
}