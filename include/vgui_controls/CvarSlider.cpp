#include <metahook.h>
#include "CvarSlider.h"
#include <stdio.h>
#include "tier1/KeyValues.h"
#include <vgui/IVGui.h>
#include <vgui_controls/propertypage.h>

#define CVARSLIDER_SCALE_FACTOR 100.0f

using namespace vgui;

DECLARE_BUILD_FACTORY(CCvarSlider);

CCvarSlider::CCvarSlider(Panel *parent, const char *name) : Slider(parent, name)
{
	SetupSlider(0, 1, "", false);
	m_bCreatedInCode = false;

	AddActionSignalTarget(this);
}

CCvarSlider::CCvarSlider(Panel *parent, const char *panelName, char const *caption, float minValue, float maxValue, char const *cvarname, bool bAllowOutOfRange) : Slider(parent, panelName)
{
	AddActionSignalTarget(this);
	SetupSlider(minValue, maxValue, cvarname, bAllowOutOfRange);

	m_bCreatedInCode = true;
}

void CCvarSlider::SetupSlider(float minValue, float maxValue, const char *cvarname, bool bAllowOutOfRange)
{
	m_flMinValue = minValue;
	m_flMaxValue = maxValue;

	SetRange((int)(CVARSLIDER_SCALE_FACTOR * minValue), (int)(CVARSLIDER_SCALE_FACTOR * maxValue));

	char szMin[32];
	char szMax[32];

	Q_snprintf(szMin, sizeof(szMin), "%.2f", minValue);
	Q_snprintf(szMax, sizeof(szMax), "%.2f", maxValue);

	SetTickCaptions(szMin, szMax);

	Q_strncpy(m_szCvarName, cvarname, sizeof(m_szCvarName));

	m_bModifiedOnce = false;
	m_bAllowOutOfRange = bAllowOutOfRange;

	Reset();
}

CCvarSlider::~CCvarSlider(void)
{
}

void CCvarSlider::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	if (!m_bCreatedInCode)
	{
		float minValue = inResourceData->GetFloat("minvalue", 0);
		float maxValue = inResourceData->GetFloat("maxvalue", 1);
		const char *cvarname = inResourceData->GetString("cvar_name", "");
		bool bAllowOutOfRange = inResourceData->GetInt("allowoutofrange", 0) != 0;
		SetupSlider(minValue, maxValue, cvarname, bAllowOutOfRange);

		if (GetParent())
		{
			if (dynamic_cast<vgui::PropertyPage*>(GetParent()) && GetParent()->GetParent())
				GetParent()->GetParent()->AddActionSignalTarget(this);
			else
				GetParent()->AddActionSignalTarget(this);
		}
	}
}

void CCvarSlider::GetSettings(KeyValues *outResourceData)
{
	BaseClass::GetSettings(outResourceData);

	if (!m_bCreatedInCode)
	{
		outResourceData->SetFloat("minvalue", m_flMinValue);
		outResourceData->SetFloat("maxvalue", m_flMaxValue);
		outResourceData->SetString("cvar_name", m_szCvarName);
		outResourceData->SetInt("allowoutofrange", m_bAllowOutOfRange);
	}
}

void CCvarSlider::SetCVarName(char const *cvarname)
{
	Q_strncpy(m_szCvarName, cvarname, sizeof(m_szCvarName));

	m_bModifiedOnce = false;

	Reset();
}

void CCvarSlider::SetMinMaxValues(float minValue, float maxValue, bool bSetTickDisplay)
{
	SetRange((int)(CVARSLIDER_SCALE_FACTOR * minValue), (int)(CVARSLIDER_SCALE_FACTOR * maxValue));

	if (bSetTickDisplay)
	{
		char szMin[32];
		char szMax[32];

		Q_snprintf(szMin, sizeof(szMin), "%.2f", minValue);
		Q_snprintf(szMax, sizeof(szMax), "%.2f", maxValue);

		SetTickCaptions(szMin, szMax);
	}

	Reset();
}

void CCvarSlider::SetTickColor(Color color)
{
	m_TickColor = color;
}

void CCvarSlider::Paint(void)
{
	float curvalue = gEngfuncs.pfnGetCvarFloat(m_szCvarName);

	if (curvalue != m_fStartValue)
	{
		int val = (int)(CVARSLIDER_SCALE_FACTOR * curvalue);
		m_fStartValue = curvalue;
		m_fCurrentValue = curvalue;

		SetValue(val);
		m_iStartValue = GetValue();
	}

	BaseClass::Paint();
}

void CCvarSlider::ApplyChanges(void)
{
	if (m_bModifiedOnce)
	{
		m_iStartValue = GetValue();

		if (m_bAllowOutOfRange)
			m_fStartValue = m_fCurrentValue;
		else
			m_fStartValue = (float)m_iStartValue / CVARSLIDER_SCALE_FACTOR;

		char value[128];
		Q_snprintf(value, sizeof(value), "%.2f", m_fStartValue);
		gEngfuncs.Cvar_Set(m_szCvarName, value);
	}
}

float CCvarSlider::GetSliderValue(void)
{
	if (m_bAllowOutOfRange)
		return m_fCurrentValue;
	else
		return ((float)GetValue()) / CVARSLIDER_SCALE_FACTOR;
}

void CCvarSlider::SetSliderValue(float fValue)
{
	int nVal = (int)(CVARSLIDER_SCALE_FACTOR * fValue);
	SetValue(nVal, false);

	m_iLastSliderValue = GetValue();

	if (m_fCurrentValue != fValue)
	{
		m_fCurrentValue = fValue;
		m_bModifiedOnce = true;
	}
}

void CCvarSlider::Reset(void)
{
	auto cvar = gEngfuncs.pfnGetCvarPointer(m_szCvarName);
	if (!cvar)
	{
		SetEnabled(false);
		return;
	}

	m_fStartValue = gEngfuncs.pfnGetCvarFloat(m_szCvarName);
	m_fCurrentValue = m_fStartValue;

	int value = (int)(CVARSLIDER_SCALE_FACTOR * m_fStartValue);
	SetValue(value);

	m_iStartValue = GetValue();
	m_iLastSliderValue = m_iStartValue;
}

bool CCvarSlider::HasBeenModified(void)
{
	if (GetValue() != m_iStartValue)
		m_bModifiedOnce = true;

	return m_bModifiedOnce;
}

void CCvarSlider::OnSliderMoved(void)
{
	if (HasBeenModified())
	{
		if (m_iLastSliderValue != GetValue())
		{
			m_iLastSliderValue = GetValue();
			m_fCurrentValue = ((float) m_iLastSliderValue) / CVARSLIDER_SCALE_FACTOR;
		}

		PostActionSignal(new KeyValues("ControlModified"));
	}
}

void CCvarSlider::OnApplyChanges(void)
{
	if (!m_bCreatedInCode)
		ApplyChanges();
}