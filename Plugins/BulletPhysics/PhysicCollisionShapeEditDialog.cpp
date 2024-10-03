#include "PhysicCollisionShapeEditDialog.h"

#include "exportfuncs.h"

#include "ClientPhysicManager.h"
#include "PhysicUTIL.h"

#include <format>

CPhysicCollisionShapeEditDialog::CPhysicCollisionShapeEditDialog(vgui::Panel* parent, const char* name,
	uint64 physicObjectId,
	const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig,
	const std::shared_ptr<CClientRigidBodyConfig>& pRigidBodyConfig,
	const std::shared_ptr<CClientCollisionShapeConfig>& pCollisionShapeConfig) :
	BaseClass(parent, name),
	m_physicObjectId(physicObjectId),
	m_pPhysicObjectConfig(pPhysicObjectConfig),
	m_pRigidBodyConfig(pRigidBodyConfig),
	m_pCollisionShapeConfig(pCollisionShapeConfig)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#BulletPhysics_CollisionShapeEditor", false);

	SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(350), vgui::scheme()->GetProportionalScaledValue(350));
	SetSize(vgui::scheme()->GetProportionalScaledValue(350), vgui::scheme()->GetProportionalScaledValue(350));

	m_pShape = new vgui::ComboBox(this, "Shape", 0, false);
	m_pShape->AddActionSignalTarget(this);

	m_pDirectionLabel = new vgui::Label(this, "DirectionLabel", "#BulletPhysics_Direction");
	m_pDirection = new vgui::ComboBox(this, "Direction", 0, false);

	m_pSizeLabel = new vgui::Label(this, "SizeLabel", "#BulletPhysics_Size");
	m_pSizeX = new vgui::TextEntry(this, "SizeX");
	m_pSizeY = new vgui::TextEntry(this, "SizeY");
	m_pSizeZ = new vgui::TextEntry(this, "SizeZ");

	m_pOriginLabel = new vgui::Label(this, "OriginLabel", "#BulletPhysics_Origin");
	m_pOriginX = new vgui::TextEntry(this, "OriginX");
	m_pOriginY = new vgui::TextEntry(this, "OriginY");
	m_pOriginZ = new vgui::TextEntry(this, "OriginZ");

	m_pAnglesLabel = new vgui::Label(this, "AnglesLabel", "#BulletPhysics_Angles");
	m_pAnglesX = new vgui::TextEntry(this, "AnglesX");
	m_pAnglesY = new vgui::TextEntry(this, "AnglesY");
	m_pAnglesZ = new vgui::TextEntry(this, "AnglesZ");

	m_pResourcePathLabel = new vgui::Label(this, "ResourcePathLabel", "#BulletPhysics_ResourcePath");
	m_pResourcePath = new vgui::TextEntry(this, "ResourcePath");

	LoadAvailableShapesIntoControl(m_pShape);
	LoadAvailableShapeDirectionsIntoControl(m_pDirection);

	LoadControlSettings("bulletphysics/PhysicCollisionShapeEditDialog.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

CPhysicCollisionShapeEditDialog::~CPhysicCollisionShapeEditDialog()
{

}

void CPhysicCollisionShapeEditDialog::Activate(void)
{
	BaseClass::Activate();

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CPhysicCollisionShapeEditDialog::OnResetData()
{
	LoadConfigIntoControls();

	UpdateControlStates();
}

void CPhysicCollisionShapeEditDialog::OnTextChanged(vgui::Panel* panel)
{
	if (panel == m_pShape) {
		UpdateControlStates();
	}
}

void CPhysicCollisionShapeEditDialog::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		SaveConfigFromControls();
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
		PostActionSignal(new KeyValues("RefreshCollisionShape", "configId", m_pCollisionShapeConfig->configId));
		Close();
		return;
	}
	else if (!stricmp(command, "Apply"))
	{
		SaveConfigFromControls();
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pPhysicObjectConfig.get());
		PostActionSignal(new KeyValues("RefreshCollisionShape", "configId", m_pCollisionShapeConfig->configId));
		return;
	}
	else if (!stricmp(command, "CloseModalDialogs"))
	{
		for (int i = 0; i < GetChildCount(); i++)
		{
			auto pChild = GetChild(i);
			PostMessage1(pChild, new KeyValues("Command", "command", "CloseModalDialogs"), NULL);
		}
		Close();
		return;
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CPhysicCollisionShapeEditDialog::LoadAvailableShapesIntoControl(vgui::ComboBox* pComboBox)
{
	for (int i = 0; i < PhysicShape_Maximum; ++i)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("type", i);

		pComboBox->AddItem(UTIL_GetCollisionShapeTypeLocalizationToken(i), kv);

		kv->deleteThis();
	}
}

void CPhysicCollisionShapeEditDialog::LoadShapeIntoControl(vgui::ComboBox* pComboBox)
{
	for (int i = 0; i < pComboBox->GetItemCount(); ++i)
	{
		KeyValues* kv = pComboBox->GetItemUserData(i);

		if (kv && m_pCollisionShapeConfig->type == kv->GetInt("type", PhysicShape_None))
		{
			pComboBox->ActivateItemByRow(i);
			return;
		}
	}

	pComboBox->ActivateItemByRow(0);
}

void CPhysicCollisionShapeEditDialog::LoadAvailableShapeDirectionsIntoControl(vgui::ComboBox* pComboBox)
{
	for (int i = 0; i < PhysicShapeDirection_Maximum; ++i)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("direction", i);

		const char* XYZ[] = { "X", "Y", "Z" };

		pComboBox->AddItem(XYZ[i], kv);

		kv->deleteThis();
	}
}

void CPhysicCollisionShapeEditDialog::LoadShapeDirectionIntoControl(vgui::ComboBox* pComboBox)
{
	for (int i = 0; i < pComboBox->GetItemCount(); ++i)
	{
		KeyValues* kv = pComboBox->GetItemUserData(i);

		if (kv && m_pCollisionShapeConfig->direction == kv->GetInt("direction", PhysicShapeDirection_X))
		{
			pComboBox->ActivateItemByRow(i);
			return;
		}
	}

	pComboBox->ActivateItemByRow(0);
}

void CPhysicCollisionShapeEditDialog::LoadConfigIntoControls()
{
	LoadShapeIntoControl(m_pShape);

	LoadShapeDirectionIntoControl(m_pDirection);

#define LOAD_INTO_TEXT_ENTRY(from, to) { auto str##to = std::format("{0}", m_pCollisionShapeConfig->from); m_p##to->SetText(str##to.c_str());}

	LOAD_INTO_TEXT_ENTRY(size[0], SizeX);
	LOAD_INTO_TEXT_ENTRY(size[1], SizeY);
	LOAD_INTO_TEXT_ENTRY(size[2], SizeZ);

	LOAD_INTO_TEXT_ENTRY(origin[0], OriginX);
	LOAD_INTO_TEXT_ENTRY(origin[1], OriginY);
	LOAD_INTO_TEXT_ENTRY(origin[2], OriginZ);

	LOAD_INTO_TEXT_ENTRY(angles[0], AnglesX);
	LOAD_INTO_TEXT_ENTRY(angles[1], AnglesY);
	LOAD_INTO_TEXT_ENTRY(angles[2], AnglesZ);

	LOAD_INTO_TEXT_ENTRY(resourcePath, ResourcePath);

#undef LOAD_INTO_TEXT_ENTRY
}

void CPhysicCollisionShapeEditDialog::SaveConfigFromControls()
{
	m_pCollisionShapeConfig->type = GetCurrentSelectedShapeType();
	m_pCollisionShapeConfig->direction = GetCurrentSelectedShapeDirection();

	char szText[256] = { 0 };

#define SAVE_FLOAT_FROM_TEXT_ENTRY(from, to, processor) {m_p##from->GetText(szText, sizeof(szText)); m_pCollisionShapeConfig->to = processor(szText); }

	SAVE_FLOAT_FROM_TEXT_ENTRY(SizeX, size[0], atof);
	SAVE_FLOAT_FROM_TEXT_ENTRY(SizeY, size[1], atof);
	SAVE_FLOAT_FROM_TEXT_ENTRY(SizeZ, size[2], atof);

	SAVE_FLOAT_FROM_TEXT_ENTRY(OriginX, origin[0], atof);
	SAVE_FLOAT_FROM_TEXT_ENTRY(OriginY, origin[1], atof);
	SAVE_FLOAT_FROM_TEXT_ENTRY(OriginZ, origin[2], atof);

	SAVE_FLOAT_FROM_TEXT_ENTRY(AnglesX, angles[0], atof);
	SAVE_FLOAT_FROM_TEXT_ENTRY(AnglesY, angles[1], atof);
	SAVE_FLOAT_FROM_TEXT_ENTRY(AnglesZ, angles[2], atof);

	SAVE_FLOAT_FROM_TEXT_ENTRY(AnglesX, angles[0], atof);
	SAVE_FLOAT_FROM_TEXT_ENTRY(AnglesY, angles[1], atof);
	SAVE_FLOAT_FROM_TEXT_ENTRY(AnglesZ, angles[2], atof);

	SAVE_FLOAT_FROM_TEXT_ENTRY(ResourcePath, resourcePath, std::string);

#undef SAVE_FLOAT_FROM_TEXT_ENTRY

	m_pCollisionShapeConfig->configModified = true;
}

int CPhysicCollisionShapeEditDialog::GetCurrentSelectedShapeType()
{
	int type = PhysicShape_None;

	auto kv = m_pShape->GetActiveItemUserData();

	if (kv)
	{
		type = kv->GetInt("type", PhysicShape_None);
	}

	return type;
}

int CPhysicCollisionShapeEditDialog::GetCurrentSelectedShapeDirection()
{
	int direction = PhysicShapeDirection_X;

	auto kv = m_pDirection->GetActiveItemUserData();

	if (kv)
	{
		direction = kv->GetInt("direction", PhysicShapeDirection_X);
	}

	return direction;
}

void CPhysicCollisionShapeEditDialog::UpdateControlStates()
{
	int type = GetCurrentSelectedShapeType();

	switch (type)
	{
	case PhysicShape_Box: {
		m_pDirectionLabel->SetVisible(false);
		m_pDirection->SetVisible(false);

		m_pSizeLabel->SetVisible(true);
		m_pSizeX->SetVisible(true);
		m_pSizeY->SetVisible(true);
		m_pSizeZ->SetVisible(true);

		m_pResourcePathLabel->SetVisible(false);
		m_pResourcePath->SetVisible(false);
		break;
	}
	case PhysicShape_Sphere: {

		m_pDirectionLabel->SetVisible(false);
		m_pDirection->SetVisible(false);

		m_pSizeLabel->SetVisible(true);
		m_pSizeX->SetVisible(true);
		m_pSizeY->SetVisible(false);
		m_pSizeZ->SetVisible(false);

		m_pResourcePathLabel->SetVisible(false);
		m_pResourcePath->SetVisible(false);
		break;
	}
	case PhysicShape_Capsule:
	case PhysicShape_Cylinder: {

		m_pDirectionLabel->SetVisible(true);
		m_pDirection->SetVisible(true);

		m_pSizeLabel->SetVisible(true);
		m_pSizeX->SetVisible(true);
		m_pSizeY->SetVisible(true);
		m_pSizeZ->SetVisible(false);

		m_pResourcePathLabel->SetVisible(false);
		m_pResourcePath->SetVisible(false);
		break;
	}
	case PhysicShape_TriangleMesh: {

		m_pDirectionLabel->SetVisible(false);
		m_pDirection->SetVisible(false);

		m_pSizeLabel->SetVisible(false);
		m_pSizeX->SetVisible(false);
		m_pSizeY->SetVisible(false);
		m_pSizeZ->SetVisible(false);

		m_pResourcePathLabel->SetVisible(true);
		m_pResourcePath->SetVisible(true);
		break;
	}
	default: {

		m_pDirectionLabel->SetVisible(false);
		m_pDirection->SetVisible(false);

		m_pSizeLabel->SetVisible(false);
		m_pSizeX->SetVisible(false);
		m_pSizeY->SetVisible(false);
		m_pSizeZ->SetVisible(false);

		m_pResourcePathLabel->SetVisible(false);
		m_pResourcePath->SetVisible(false);
		break;
	}
	}

	if (m_pCollisionShapeConfig->is_child)
	{
		m_pOriginLabel->SetVisible(true);
		m_pOriginX->SetVisible(true);
		m_pOriginY->SetVisible(true);
		m_pOriginZ->SetVisible(true);

		m_pAnglesLabel->SetVisible(true);
		m_pAnglesX->SetVisible(true);
		m_pAnglesY->SetVisible(true);
		m_pAnglesZ->SetVisible(true);
	}
	else
	{
		m_pOriginLabel->SetVisible(false);
		m_pOriginX->SetVisible(false);
		m_pOriginY->SetVisible(false);
		m_pOriginZ->SetVisible(false);

		m_pAnglesLabel->SetVisible(false);
		m_pAnglesX->SetVisible(false);
		m_pAnglesY->SetVisible(false);
		m_pAnglesZ->SetVisible(false);
	}
}