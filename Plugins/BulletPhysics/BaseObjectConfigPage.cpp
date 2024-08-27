#include "PhysicEditorDialog.h"

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

//BaseObjectConfig Page

CBaseObjectConfigPage::CBaseObjectConfigPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pPhysicObjectConfig(pPhysicObjectConfig)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pStaticObject = new vgui::CheckButton(this, "StaticObject", "#BulletPhysics_StaticObject");
	m_pDynamicObject = new vgui::CheckButton(this, "DynamicObject", "#BulletPhysics_DynamicObject");
	m_pRagdollObject = new vgui::CheckButton(this, "RagdollObject", "#BulletPhysics_RagdollObject");
	m_pFromBSP = new vgui::CheckButton(this, "FromBSP", "#BulletPhysics_FromBSP");
	m_pFromConfig = new vgui::CheckButton(this, "FromConfig", "#BulletPhysics_FromConfig");
	m_pBarnacle = new vgui::CheckButton(this, "Barnacle", "#BulletPhysics_Barnacle");
	m_pGargantua = new vgui::CheckButton(this, "Gargantua", "#BulletPhysics_Gargantua");

	m_pDebugDrawLevel = new vgui::TextEntry(this, "DebugDrawLevel");

	LoadControlSettings("bulletphysics/BaseObjectConfigPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CBaseObjectConfigPage::OnApplyChanges()
{
	SaveConfigFromControls();
}

void CBaseObjectConfigPage::OnResetData()
{
	LoadConfigIntoControls();
}

void CBaseObjectConfigPage::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ENTER)
	{

	}

	BaseClass::OnKeyCodeTyped(code);
}

void CBaseObjectConfigPage::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{

	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CBaseObjectConfigPage::LoadConfigIntoControls()
{
#define LOAD_INTO_TEXT_ENTRY(from, to) { auto str##to = std::format("{0}", m_pPhysicObjectConfig->from); m_p##to->SetText(str##to.c_str());}
	LOAD_INTO_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel);
#undef LOAD_INTO_TEXT_ENTRY

#define LOAD_INTO_CHECK_BUTTON(from, to) m_p##to->SetSelected((m_pPhysicObjectConfig->from & PhysicObjectFlag_##to) ? true : false);
	LOAD_INTO_CHECK_BUTTON(flags, StaticObject);
	LOAD_INTO_CHECK_BUTTON(flags, DynamicObject);
	LOAD_INTO_CHECK_BUTTON(flags, RagdollObject);
	LOAD_INTO_CHECK_BUTTON(flags, FromBSP);
	LOAD_INTO_CHECK_BUTTON(flags, FromConfig);
	LOAD_INTO_CHECK_BUTTON(flags, Barnacle);
	LOAD_INTO_CHECK_BUTTON(flags, Gargantua);
#undef LOAD_INTO_CHECK_BUTTON
}

void CBaseObjectConfigPage::SaveConfigFromControls()
{
	char szText[256];
#define SAVE_FROM_TEXT_ENTRY(to, from, processor) {m_p##from->GetText(szText, sizeof(szText)); m_pPhysicObjectConfig->to = processor(szText);}

	SAVE_FROM_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel, atoi);

#undef SAVE_FROM_TEXT_ENTRY

#define SAVE_FROM_CHECK_BUTTON(to, from) if (m_p##from->IsSelected()) { m_pPhysicObjectConfig->to |= PhysicObjectFlag_##from; } else { m_pPhysicObjectConfig->to &= ~PhysicObjectFlag_##from; }
	SAVE_FROM_CHECK_BUTTON(flags, Barnacle);
	SAVE_FROM_CHECK_BUTTON(flags, Gargantua);
#undef SAVE_FROM_CHECK_BUTTON

	m_pPhysicObjectConfig->configModified = true;
}