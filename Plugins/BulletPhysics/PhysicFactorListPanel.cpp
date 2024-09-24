#include "PhysicFactorListPanel.h"

CPhysicFactorListPanel::CPhysicFactorListPanel(vgui::Panel* parent, const char* pName) : BaseClass(parent, pName)
{
    m_pInlineTextEntryPanel = new CInlineTextEntryPanel();
}

CPhysicFactorListPanel::~CPhysicFactorListPanel()
{
    m_pInlineTextEntryPanel->MarkForDeletion();
}

void CPhysicFactorListPanel::StartCaptureMode()
{
    if (GetSelectedItemsCount() == 0)
        return;

    auto selectItemId = GetSelectedItem(0);

    m_bCaptureMode = true;
    m_iCaptureItemId = selectItemId;

    EnterEditMode(selectItemId, 2, m_pInlineTextEntryPanel);
    vgui::input()->SetMouseFocus(m_pInlineTextEntryPanel->GetVPanel());

    auto kv = GetItemData(selectItemId)->kv;

    if (kv)
    {
        m_pInlineTextEntryPanel->SetText(kv->GetString("value"));
        m_iCaptureItemIndex = kv->GetInt("index");
    }
}

void CPhysicFactorListPanel::EndCaptureMode()
{
    m_bCaptureMode = false;
    m_iCaptureItemId = -1;
    LeaveEditMode();
    RequestFocus();
    vgui::input()->SetMouseFocus(GetVPanel());

    auto kv = new KeyValues("ModifyFactor");

    kv->SetInt("index", m_iCaptureItemIndex);

    char szText[256];
    m_pInlineTextEntryPanel->GetText(szText, sizeof(szText));

    kv->SetString("newValue", szText);

    PostActionSignal(kv);
}

bool CPhysicFactorListPanel::IsCapturing(void) const
{
    return m_bCaptureMode;
}

int CPhysicFactorListPanel::GetCapturingItemId(void) const
{
    return m_iCaptureItemId;
}

int CPhysicFactorListPanel::GetCapturingItemIndex(void) const
{
    return m_iCaptureItemIndex;
}

void CPhysicFactorListPanel::OnMousePressed(vgui::MouseCode code)
{
    BaseClass::OnMousePressed(code);

    if (IsCapturing())
    {
        if (m_VisibleItems.Count() > 0)
        {
            // determine where we were pressed
            int x, y, row, column;
            vgui::input()->GetCursorPos(x, y);
            GetCellAtPos(x, y, row, column);

            if (row < 0 || row >= m_VisibleItems.Count())
            {
                EndCaptureMode();
                return;
            }

            int itemID = m_VisibleItems[row];

            if (itemID != GetCapturingItemId())
            {
                EndCaptureMode();
                return;
            }
        }
    }
}