#include "PhysicObjectConfigPage.h"

#include "PhysicUTIL.h"
#include "exportfuncs.h"

#include <format>

CPhysicObjectConfigPage::CPhysicObjectConfigPage(vgui::Panel* parent, const char* name, uint64 physicObjectId, const std::shared_ptr<CClientPhysicObjectConfig>& pPhysicObjectConfig) :
	BaseClass(parent, name), m_physicObjectId(physicObjectId), m_pPhysicObjectConfig(pPhysicObjectConfig)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pBarnacle = new vgui::CheckButton(this, "Barnacle", "#BulletPhysics_Barnacle");
	m_pGargantua = new vgui::CheckButton(this, "Gargantua", "#BulletPhysics_Gargantua");
	m_pVerifyBoneChunk = new vgui::CheckButton(this, "VerifyBoneChunk", "#BulletPhysics_VerifyBoneChunk");
	m_pVerifyModelFile = new vgui::CheckButton(this, "VerifyModelFile", "#BulletPhysics_VerifyModelFile");
	m_pCrc32BoneChunk = new vgui::TextEntry(this, "Crc32BoneChunk");
	m_pCrc32ModelFile = new vgui::TextEntry(this, "Crc32ModelFile");
	m_pDebugDrawLevel = new vgui::TextEntry(this, "DebugDrawLevel");

	LoadControlSettings("bulletphysics/PhysicObjectConfigPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CPhysicObjectConfigPage::OnApplyChanges()
{
	SaveConfigFromControls();
}

void CPhysicObjectConfigPage::OnResetData()
{
	LoadConfigIntoControls();
}

void CPhysicObjectConfigPage::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ENTER)
	{

	}

	BaseClass::OnKeyCodeTyped(code);
}

void CPhysicObjectConfigPage::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{

	}
	else if (!stricmp(command, "GenerateCrc32BoneChunk"))
	{
		GenerateCrc32BoneChunk();
		return;
	}
	else if (!stricmp(command, "GenerateCrc32ModelFile"))
	{
		GenerateCrc32ModelFile();
		return;
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CPhysicObjectConfigPage::GenerateCrc32BoneChunk()
{
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(m_physicObjectId);
	auto model = EngineGetModelByIndex(modelindex);

	if (!model)
		return;

	std::string Crc32Value;
	if (!UTIL_GetCrc32ForBoneChunk(model, &Crc32Value))
		return;

#define LOAD_VALUE_INTO_TEXT_ENTRY(val, to) { auto str##to = std::format("{0}", val); m_p##to->SetText(str##to.c_str());}

	LOAD_VALUE_INTO_TEXT_ENTRY(Crc32Value, Crc32BoneChunk);

#undef LOAD_VALUE_INTO_TEXT_ENTRY

}

void CPhysicObjectConfigPage::GenerateCrc32ModelFile()
{
	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(m_physicObjectId);
	auto model = EngineGetModelByIndex(modelindex);

	if (!model)
		return;

	std::string Crc32Value;
	if (!UTIL_GetCrc32ForModelFile(model, &Crc32Value))
		return;

#define LOAD_VALUE_INTO_TEXT_ENTRY(val, to) { auto str##to = std::format("{0}", val); m_p##to->SetText(str##to.c_str());}

	LOAD_VALUE_INTO_TEXT_ENTRY(Crc32Value, Crc32ModelFile);

#undef LOAD_VALUE_INTO_TEXT_ENTRY
}

void CPhysicObjectConfigPage::LoadConfigIntoControls()
{
#define LOAD_INTO_TEXT_ENTRY(from, to) { auto str##to = std::format("{0}", m_pPhysicObjectConfig->from); m_p##to->SetText(str##to.c_str());}
	LOAD_INTO_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel);
	LOAD_INTO_TEXT_ENTRY(crc32BoneChunk, Crc32BoneChunk);
	LOAD_INTO_TEXT_ENTRY(crc32ModelFile, Crc32ModelFile);
#undef LOAD_INTO_TEXT_ENTRY

#define LOAD_INTO_CHECK_BUTTON(from, to) m_p##to->SetSelected((m_pPhysicObjectConfig->from & PhysicObjectFlag_##to) ? true : false);
	LOAD_INTO_CHECK_BUTTON(flags, Barnacle);
	LOAD_INTO_CHECK_BUTTON(flags, Gargantua);
#undef LOAD_INTO_CHECK_BUTTON

#define LOAD_INTO_CHECK_BUTTON(from, to) m_p##to->SetSelected((m_pPhysicObjectConfig->from) ? true : false);
	LOAD_INTO_CHECK_BUTTON(verifyBoneChunk, VerifyBoneChunk);
	LOAD_INTO_CHECK_BUTTON(verifyModelFile, VerifyModelFile);
#undef LOAD_INTO_CHECK_BUTTON
}

void CPhysicObjectConfigPage::SaveConfigFromControls()
{
	char szText[256];
#define SAVE_FROM_TEXT_ENTRY(to, from, processor) {m_p##from->GetText(szText, sizeof(szText)); m_pPhysicObjectConfig->to = processor(szText);}

	SAVE_FROM_TEXT_ENTRY(debugDrawLevel, DebugDrawLevel, atoi);
	SAVE_FROM_TEXT_ENTRY(crc32BoneChunk, Crc32BoneChunk, std::string);
	SAVE_FROM_TEXT_ENTRY(crc32ModelFile, Crc32ModelFile, std::string);

#undef SAVE_FROM_TEXT_ENTRY

#define SAVE_FROM_CHECK_BUTTON(to, from) if (m_p##from->IsSelected()) { m_pPhysicObjectConfig->to |= PhysicObjectFlag_##from; } else { m_pPhysicObjectConfig->to &= ~PhysicObjectFlag_##from; }
	SAVE_FROM_CHECK_BUTTON(flags, Barnacle);
	SAVE_FROM_CHECK_BUTTON(flags, Gargantua);
#undef SAVE_FROM_CHECK_BUTTON

#define SAVE_FROM_CHECK_BUTTON(to, from) if (m_p##from->IsSelected()) { m_pPhysicObjectConfig->to = true; } else { m_pPhysicObjectConfig->to = false; }
	SAVE_FROM_CHECK_BUTTON(verifyBoneChunk, VerifyBoneChunk);
	SAVE_FROM_CHECK_BUTTON(verifyModelFile, VerifyModelFile);
#undef SAVE_FROM_CHECK_BUTTON

	m_pPhysicObjectConfig->configModified = true;
}