#include "AnimControlEditDialog.h"

#include "ClientPhysicManager.h"
#include "exportfuncs.h"

#include "PhysicUTIL.h"

#include <format>

CAnimControlEditDialog::CAnimControlEditDialog(vgui::Panel* parent, const char* name, uint64 physicObjectId,
	const std::shared_ptr<CClientRagdollObjectConfig>& pRagdollObjectConfig,
	const std::shared_ptr<CClientAnimControlConfig>& pAnimControlConfig) :
	BaseClass(parent, name),
	m_physicObjectId(physicObjectId),
	m_pRagdollObjectConfig(pRagdollObjectConfig),
	m_pAnimControlConfig(pAnimControlConfig)
{
	SetDeleteSelfOnClose(true);

	SetTitle("#BulletPhysics_AnimControlEditor", false);

	SetMinimumSize(vgui::scheme()->GetProportionalScaledValue(350), vgui::scheme()->GetProportionalScaledValue(350));
	SetSize(vgui::scheme()->GetProportionalScaledValue(350), vgui::scheme()->GetProportionalScaledValue(350));

	m_pSequence = new vgui::ComboBox(this, "Sequence", 0, false);
	m_pGaitSequence = new vgui::ComboBox(this, "GaitSequence", 0, false);
	m_pActivityType = new vgui::ComboBox(this, "ActivityType", 0, false);

	m_pAnimFrame = new vgui::TextEntry(this, "AnimFrame");

	m_pController_0 = new vgui::TextEntry(this, "Controller_0");
	m_pController_1 = new vgui::TextEntry(this, "Controller_1");
	m_pController_2 = new vgui::TextEntry(this, "Controller_2");
	m_pController_3 = new vgui::TextEntry(this, "Controller_3");

	m_pBlending_0 = new vgui::TextEntry(this, "Blending_0");
	m_pBlending_1 = new vgui::TextEntry(this, "Blending_1");
	m_pBlending_2 = new vgui::TextEntry(this, "Blending_2");
	m_pBlending_3 = new vgui::TextEntry(this, "Blending_3");

	vgui::HFont hFallbackFont = vgui::scheme()->GetIScheme(GetScheme())->GetFont("DefaultVerySmallFallBack", false);

	if (vgui::INVALID_FONT != hFallbackFont)
	{
		m_pSequence->SetUseFallbackFont(true, hFallbackFont);
		m_pGaitSequence->SetUseFallbackFont(true, hFallbackFont);
		m_pActivityType->SetUseFallbackFont(true, hFallbackFont);
	}

	LoadAvailableSequencesIntoControl(m_pSequence);
	LoadAvailableSequencesIntoControl(m_pGaitSequence);
	LoadAvailableActivityTypesIntoControl(m_pActivityType);

	LoadControlSettings("bulletphysics/AnimControlEditDialog.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

CAnimControlEditDialog::~CAnimControlEditDialog()
{

}

void CAnimControlEditDialog::Activate(void)
{
	BaseClass::Activate();

	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());
}

void CAnimControlEditDialog::OnResetData()
{
	LoadConfigIntoControls();
}

void CAnimControlEditDialog::OnCommand(const char* command)
{
	if (!stricmp(command, "OK"))
	{
		SaveConfigFromControls();
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pRagdollObjectConfig.get());
		PostActionSignal(new KeyValues("RefreshAnimControl", "configId", m_pAnimControlConfig->configId));
		Close();
		return;
	}
	else if (!stricmp(command, "Apply"))
	{
		SaveConfigFromControls();
		ClientPhysicManager()->RebuildPhysicObjectEx(m_physicObjectId, m_pRagdollObjectConfig.get());
		PostActionSignal(new KeyValues("RefreshAnimControl", "configId", m_pAnimControlConfig->configId));
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

	BaseClass::OnCommand(command);
}

void CAnimControlEditDialog::LoadAvailableSequencesIntoControl(vgui::ComboBox* pComboBox)
{
	//-1 means invalid sequence
	if (1)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("sequence", -1);

		pComboBox->AddItem("--", kv);

		kv->deleteThis();
	}

	auto modelindex = UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(m_physicObjectId);

	auto model = EngineGetModelByIndex(modelindex);

	if (model && model->type == mod_studio)
	{
		auto studiohdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(model);

		if (studiohdr)
		{
			for (int i = 0; i < studiohdr->numbones; ++i)
			{
				auto kv = new KeyValues("UserData");

				kv->SetString("name", UTIL_GetSequenceRawName(studiohdr, i));

				kv->SetInt("sequence", i);

				auto formattedName = UTIL_GetFormattedSequenceNameEx(studiohdr, i);

				pComboBox->AddItem(formattedName.c_str(), kv);

				kv->deleteThis();
			}
		}
	}
}

void CAnimControlEditDialog::LoadAvailableActivityTypesIntoControl(vgui::ComboBox* pComboBox)
{
	for (int i = 0; i < (int)StudioAnimActivityType_Maximum; ++i)
	{
		auto kv = new KeyValues("UserData");

		kv->SetInt("activityType", i);

		auto ty = (StudioAnimActivityType)i;

		pComboBox->AddItem(UTIL_GetActivityTypeLocalizationToken(ty), kv);

		kv->deleteThis();
	}
}

void CAnimControlEditDialog::LoadSequenceIntoControl(vgui::ComboBox* pComboBox, int sequence)
{
	for (int i = 0; i < pComboBox->GetItemCount(); ++i)
	{
		KeyValues* kv = pComboBox->GetItemUserData(i);

		if (kv && sequence == kv->GetInt("sequence", -1))
		{
			pComboBox->ActivateItemByRow(i);
			return;
		}
	}

	pComboBox->ActivateItemByRow(0);
}

void CAnimControlEditDialog::LoadActivityTypeIntoControl(vgui::ComboBox* pComboBox)
{
	for (int i = 0; i < pComboBox->GetItemCount(); ++i)
	{
		KeyValues* kv = pComboBox->GetItemUserData(i);

		if (kv && m_pAnimControlConfig->activity == kv->GetInt("activityType", -1))
		{
			pComboBox->ActivateItemByRow(i);
			return;
		}
	}

	pComboBox->ActivateItemByRow(0);
}

void CAnimControlEditDialog::LoadConfigIntoControls()
{
	LoadSequenceIntoControl(m_pSequence, m_pAnimControlConfig->sequence);
	LoadSequenceIntoControl(m_pGaitSequence, m_pAnimControlConfig->gaitsequence);

	LoadActivityTypeIntoControl(m_pActivityType);

#define LOAD_INTO_TEXT_ENTRY(from, to) { auto str##to = std::format("{0}", m_pAnimControlConfig->from); m_p##to->SetText(str##to.c_str());}

	LOAD_INTO_TEXT_ENTRY(animframe, AnimFrame);

	LOAD_INTO_TEXT_ENTRY(controller[0], Controller_0);
	LOAD_INTO_TEXT_ENTRY(controller[1], Controller_1);
	LOAD_INTO_TEXT_ENTRY(controller[2], Controller_2);
	LOAD_INTO_TEXT_ENTRY(controller[3], Controller_3);

	LOAD_INTO_TEXT_ENTRY(blending[0], Blending_0);
	LOAD_INTO_TEXT_ENTRY(blending[1], Blending_1);
	LOAD_INTO_TEXT_ENTRY(blending[2], Blending_2);
	LOAD_INTO_TEXT_ENTRY(blending[3], Blending_3);

#undef LOAD_INTO_TEXT_ENTRY
}

static int clamp_0_255_atoi(const char* str)
{
	auto val = atoi(str);
	return max(min(val, 255), 0);
}

static float clamp_0_255_atof(const char* str)
{
	auto val = atof(str);
	return max(min(val, 255), 0);
}

void CAnimControlEditDialog::SaveConfigFromControls()
{
	char szText[256];

	m_pAnimControlConfig->sequence = GetCurrentSelectedSequence(m_pSequence);
	m_pAnimControlConfig->gaitsequence = GetCurrentSelectedSequence(m_pGaitSequence);

	m_pAnimControlConfig->activity = GetCurrentSelectedActivityType(m_pActivityType);

#define SAVE_FROM_TEXT_ENTRY(to, from, processor) { \
        m_p##from->GetText(szText, sizeof(szText)); \
        m_pAnimControlConfig->to = processor(szText); \
    }

	SAVE_FROM_TEXT_ENTRY(animframe, AnimFrame, clamp_0_255_atof);

	SAVE_FROM_TEXT_ENTRY(controller[0], Controller_0, clamp_0_255_atoi);
	SAVE_FROM_TEXT_ENTRY(controller[1], Controller_1, clamp_0_255_atoi);
	SAVE_FROM_TEXT_ENTRY(controller[2], Controller_2, clamp_0_255_atoi);
	SAVE_FROM_TEXT_ENTRY(controller[3], Controller_3, clamp_0_255_atoi);

	SAVE_FROM_TEXT_ENTRY(blending[0], Blending_0, clamp_0_255_atoi);
	SAVE_FROM_TEXT_ENTRY(blending[1], Blending_1, clamp_0_255_atoi);
	SAVE_FROM_TEXT_ENTRY(blending[2], Blending_2, clamp_0_255_atoi);
	SAVE_FROM_TEXT_ENTRY(blending[3], Blending_3, clamp_0_255_atoi);

#undef SAVE_FROM_TEXT_ENTRY

	m_pAnimControlConfig->configModified = true;
}

int CAnimControlEditDialog::GetCurrentSelectedSequence(vgui::ComboBox* pComboBox)
{
	int sequence = -1;

	auto kv = pComboBox->GetActiveItemUserData();

	if (kv)
	{
		sequence = kv->GetInt("sequence", -1);
	}

	return sequence;
}

StudioAnimActivityType CAnimControlEditDialog::GetCurrentSelectedActivityType(vgui::ComboBox* pComboBox)
{
	StudioAnimActivityType activityType = StudioAnimActivityType_Idle;

	auto kv = pComboBox->GetActiveItemUserData();

	if (kv)
	{
		activityType = (StudioAnimActivityType)kv->GetInt("activityType", StudioAnimActivityType_Idle);
	}

	return activityType;
}
