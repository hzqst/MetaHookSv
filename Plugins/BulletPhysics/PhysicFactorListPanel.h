#pragma once

#include <vgui_controls/ListPanel.h>
#include <vgui_controls/TextEntry.h>

class CInlineTextEntryPanel : public vgui::TextEntry
{
public:
    CInlineTextEntryPanel() : vgui::TextEntry(NULL, "InlineTextEntryPanel")
    {
        SetEditable(true);
        SetEnabled(true);
    }

    void OnKeyCodeTyped(vgui::KeyCode code) override
    {
        if (code == vgui::KEY_ENTER)
        {
            if (GetParent())
            {
                GetParent()->OnKeyCodeTyped(code);
            }
            return;
        }

        BaseClass::OnKeyCodeTyped(code);
    }

    void ApplySchemeSettings(vgui::IScheme* pScheme) override
    {
        BaseClass::ApplySchemeSettings(pScheme);
        SetBorder(pScheme->GetBorder("DepressedButtonBorder"));

        SetPaintBackgroundEnabled(true);
        auto bgColor = GetBgColor();
        bgColor[3] = 255;
        SetBgColor(bgColor);
        m_hFont = pScheme->GetFont("Default", IsProportional());

        SetFont(m_hFont);
    }

private:
    typedef vgui::TextEntry BaseClass;
    vgui::HFont		m_hFont{};
};

class CPhysicFactorListPanel : public vgui::ListPanel
{
public:
	DECLARE_CLASS_SIMPLE(CPhysicFactorListPanel, vgui::ListPanel);

    CPhysicFactorListPanel(vgui::Panel* parent, const char* pName);
	~CPhysicFactorListPanel();

	void StartCaptureMode();
	void EndCaptureMode();
	bool IsCapturing(void) const;
	int GetCapturingItemId(void) const;
	int GetCapturingItemIndex(void) const;
	void OnMousePressed(vgui::MouseCode code) override;

private:

	typedef vgui::ListPanel BaseClass;
	bool m_bCaptureMode{};
	int m_iCaptureItemId{};
	int m_iCaptureItemIndex{};

	CInlineTextEntryPanel* m_pInlineTextEntryPanel{};
};
