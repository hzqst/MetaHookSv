#include "PhysicEditorDialog.h"

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

//Physic Editor

CPhysicEditorDialog::CPhysicEditorDialog(vgui::Panel* parent, const char *name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pPhysicObjectConfig(pPhysicObjectConfig)
{
	SetDeleteSelfOnClose(true);

	wchar_t wszShortName[64] = { 0 };
	vgui::localize()->ConvertANSIToUnicode(pPhysicObjectConfig->shortName.c_str(), wszShortName, sizeof(wszShortName));

	auto title = std::format(L"{0} ({1} - {2})", vgui::localize()->Find("#BulletPhysics_PhysicEditor"), vgui::localize()->Find(UTIL_GetPhysicObjectTypeLocalizationToken(pPhysicObjectConfig->type)), wszShortName);

	SetTitle(title.c_str(), false);

	m_pBaseObjectConfigPage = new CBaseObjectConfigPage(this, "BaseObjectConfigPage", m_physicObjectId, pPhysicObjectConfig);
	m_pBaseObjectConfigPage->MakeReadyForUse();

	m_pRigidBodyPage = new CRigidBodyPage(this, "RigidBodyPage", m_physicObjectId, pPhysicObjectConfig);
	m_pRigidBodyPage->MakeReadyForUse();

	m_pConstraintPage = new CConstraintPage(this, "ConstraintPage", m_physicObjectId, pPhysicObjectConfig);
	m_pConstraintPage->MakeReadyForUse();

	SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(640), vgui::scheme()->GetProportionalScaledValue(384));
	SetSize(vgui::scheme()->GetProportionalScaledValue(640), vgui::scheme()->GetProportionalScaledValue(384));

	m_pTabPanel = new vgui::PropertySheet(this, "PhysicTabs");
	m_pTabPanel->SetTabWidth(vgui::scheme()->GetProportionalScaledValue(72));
	m_pTabPanel->AddPage(m_pBaseObjectConfigPage, "#BulletPhysics_Base");
	m_pTabPanel->AddPage(m_pRigidBodyPage, "#BulletPhysics_RigidBody");
	m_pTabPanel->AddPage(m_pConstraintPage, "#BulletPhysics_Constraint");
	m_pTabPanel->AddActionSignalTarget(this);

	LoadControlSettings("bulletphysics/PhysicEditorDialog.res", "GAME");

	m_pTabPanel->SetActivePage(m_pBaseObjectConfigPage);

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