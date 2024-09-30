#include "PhysicEditorDialog.h"
#include "PhysicObjectConfigPage.h"
#include "PhysicRigidBodyPage.h"
#include "PhysicConstraintPage.h"
#include "PhysicBehaviorPage.h"
#include "AnimControlPage.h"

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

CPhysicEditorDialog::CPhysicEditorDialog(vgui::Panel* parent, const char *name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pPhysicObjectConfig(pPhysicObjectConfig)
{
	SetDeleteSelfOnClose(true);

	wchar_t wszShortName[64] = { 0 };
	vgui::localize()->ConvertANSIToUnicode(pPhysicObjectConfig->shortName.c_str(), wszShortName, sizeof(wszShortName));

	auto title = std::format(L"{0} ({1} - {2})", vgui::localize()->Find("#BulletPhysics_PhysicEditor"), vgui::localize()->Find(UTIL_GetPhysicObjectTypeLocalizationToken(pPhysicObjectConfig->type)), wszShortName);

	SetTitle(title.c_str(), false);

	m_pPhysicObjectConfigPage = new CPhysicObjectConfigPage(this, "PhysicObjectConfigPage", m_physicObjectId, pPhysicObjectConfig);
	m_pPhysicObjectConfigPage->MakeReadyForUse();

	m_pPhysicRigidBodyPage = new CPhysicRigidBodyPage(this, "PhysicRigidBodyPage", m_physicObjectId, pPhysicObjectConfig);
	m_pPhysicRigidBodyPage->MakeReadyForUse();

	m_pPhysicConstraintPage = new CPhysicConstraintPage(this, "PhysicConstraintPage", m_physicObjectId, pPhysicObjectConfig);
	m_pPhysicConstraintPage->MakeReadyForUse();

	m_pPhysicBehaviorPage = new CPhysicBehaviorPage(this, "PhysicBehaviorPage", m_physicObjectId, pPhysicObjectConfig);
	m_pPhysicBehaviorPage->MakeReadyForUse();

	SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(640), vgui::scheme()->GetProportionalScaledValue(384));
	SetSize(vgui::scheme()->GetProportionalScaledValue(640), vgui::scheme()->GetProportionalScaledValue(384));

	m_pTabPanel = new vgui::PropertySheet(this, "PhysicTabs");
	m_pTabPanel->SetTabWidth(vgui::scheme()->GetProportionalScaledValue(72));
	m_pTabPanel->AddPage(m_pPhysicObjectConfigPage, "#BulletPhysics_Base");
	m_pTabPanel->AddPage(m_pPhysicRigidBodyPage, "#BulletPhysics_RigidBody");
	m_pTabPanel->AddPage(m_pPhysicConstraintPage, "#BulletPhysics_Constraint");
	m_pTabPanel->AddPage(m_pPhysicBehaviorPage, "#BulletPhysics_PhysicBehavior");

	if (pPhysicObjectConfig->type == PhysicObjectType_RagdollObject)
	{
		auto pRagdollObjectConfig = UTIL_ConvertPhysicObjectConfigToRagdollObjectConfig(pPhysicObjectConfig);

		m_pAnimControlPage = new CAnimControlPage(this, "AnimControlPage", m_physicObjectId, pRagdollObjectConfig);
		m_pAnimControlPage->MakeReadyForUse();

		m_pTabPanel->AddPage(m_pAnimControlPage, "#BulletPhysics_AnimControl");
	}

	m_pTabPanel->AddActionSignalTarget(this);

	LoadControlSettings("bulletphysics/PhysicEditorDialog.res", "GAME");

	m_pTabPanel->SetActivePage(m_pPhysicObjectConfigPage);

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

CPhysicEditorDialog::~CPhysicEditorDialog()
{

}

void CPhysicEditorDialog::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		m_pTabPanel->ApplyChanges();
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
		Close();
		return;
	}
	else if (!stricmp(command, "Apply"))
	{
		m_pTabPanel->ApplyChanges();
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
		return;
	}

	BaseClass::OnCommand(command);
}