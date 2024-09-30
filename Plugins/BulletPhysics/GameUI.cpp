#include <metahook.h>
#include <cvardef.h>
#include <IGameUI.h>
#include <vgui/VGUI.h>
#include <vgui/IPanel.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/QueryBox.h>
#include <vgui_controls/MessageMap.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/CvarSlider.h>
#include <vgui_controls/CvarToggleCheckButton.h>
#include <vgui_controls/CvarTextEntry.h>

#include <IVGUI2Extension.h>

#include "plugins.h"

class COptionsSubBulletPhysicsDlg : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(COptionsSubBulletPhysicsDlg, vgui::PropertyPage);

public:
	COptionsSubBulletPhysicsDlg(vgui::Panel* parent) : BaseClass(parent, "OptionsSubBulletPhysicsDlg")
	{
		m_pDebugDraw = new CCvarToggleCheckButton(this, "DebugDraw", "#GameUI_BulletPhysics_DebugDraw", "bv_debug_draw");
		m_pDebugDrawWallHack = new CCvarToggleCheckButton(this, "DebugDrawWallHack", "#GameUI_BulletPhysics_DebugDrawWallHack", "bv_debug_draw_wallhack");
		m_pDebugDrawLevelStaticObject = new CCvarTextEntry(this, "DebugDrawLevelStaticObject", "bv_debug_draw_level_static");
		m_pDebugDrawLevelDynamicObject = new CCvarTextEntry(this, "DebugDrawLevelDynamicObject", "bv_debug_draw_level_dynamic");
		m_pDebugDrawLevelRagdollObject = new CCvarTextEntry(this, "DebugDrawLevelRagdollObject", "bv_debug_draw_level_ragdoll");
		m_pDebugDrawLevelRigidBody = new CCvarTextEntry(this, "DebugDrawLevelRigidBody", "bv_debug_draw_level_rigidbody");
		m_pDebugDrawLevelConstraint = new CCvarTextEntry(this, "DebugDrawLevelConstraint", "bv_debug_draw_level_constraint");
		m_pDebugDrawLevelPhysicBehavior = new CCvarTextEntry(this, "DebugDrawLevelPhysicBehavior", "bv_debug_draw_level_behavior");

		LoadControlSettings("bulletphysics/OptionsSubBulletPhysicsDlg.res");
	}

	void ApplyChangesToConVar(const char* pConVarName, int value)
	{
		char szCmd[256] = { 0 };
		Q_snprintf(szCmd, sizeof(256) - 1, "%s %d\n", pConVarName, value);
		gEngfuncs.pfnClientCmd(szCmd);
	}

	void ApplyChanges(void)
	{
		m_pDebugDraw->ApplyChanges();
		m_pDebugDrawWallHack->ApplyChanges();
		m_pDebugDrawLevelStaticObject->ApplyChanges();
		m_pDebugDrawLevelDynamicObject->ApplyChanges();
		m_pDebugDrawLevelRagdollObject->ApplyChanges();
		m_pDebugDrawLevelRigidBody->ApplyChanges();
		m_pDebugDrawLevelConstraint->ApplyChanges();
		m_pDebugDrawLevelPhysicBehavior->ApplyChanges();
	}

	void OnApplyChanges() override
	{
		ApplyChanges();
	}

	void OnResetData(void) override
	{
		m_pDebugDraw->Reset();
		m_pDebugDrawWallHack->Reset();
		m_pDebugDrawLevelStaticObject->Reset();
		m_pDebugDrawLevelDynamicObject->Reset();
		m_pDebugDrawLevelRagdollObject->Reset();
		m_pDebugDrawLevelRigidBody->Reset();
		m_pDebugDrawLevelConstraint->Reset();
		m_pDebugDrawLevelPhysicBehavior->Reset();
	}

	void OnCommand(const char* command) override
	{
		if (!stricmp(command, "OK"))
		{
			ApplyChanges();
		}
		else if (!stricmp(command, "Apply"))
		{
			ApplyChanges();
		}
		else
		{
			BaseClass::OnCommand(command);
		}
	}

	MESSAGE_FUNC(OnDataChanged, "ControlModified");

private:

	CCvarToggleCheckButton* m_pDebugDraw;
	CCvarToggleCheckButton* m_pDebugDrawWallHack;
	CCvarTextEntry* m_pDebugDrawLevelStaticObject;
	CCvarTextEntry* m_pDebugDrawLevelDynamicObject;
	CCvarTextEntry* m_pDebugDrawLevelRagdollObject;
	CCvarTextEntry* m_pDebugDrawLevelRigidBody;
	CCvarTextEntry* m_pDebugDrawLevelConstraint;
	CCvarTextEntry* m_pDebugDrawLevelPhysicBehavior;
};

void COptionsSubBulletPhysicsDlg::OnDataChanged()
{
	GetParentWithModuleName("GameUI")->PostActionSignal(new KeyValues("ApplyButtonEnable"));
}

/*
=================================================================================================================
GameUI Callbacks
=================================================================================================================
*/

class CVGUI2Extension_GameUICallbacks : public IVGUI2Extension_GameUICallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	const char* GetControlModuleName() const
	{
		return "BulletPhysics";
	}

	void Initialize(CreateInterfaceFn* factories, int count) override
	{
		if (!vgui::VGui_InitInterfacesList("BulletPhysics", factories, count))
		{
			Sys_Error("Failed to VGui_InitInterfacesList");
			return;
		}
	}

	void Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system) override
	{
		if (g_pFileSystem)
		{
			if (!vgui::localize()->AddFile(g_pFileSystem, "bulletphysics/gameui_%language%.txt"))
			{
				if (!vgui::localize()->AddFile(g_pFileSystem, "bulletphysics/gameui_english.txt"))
				{
					Sys_Error("Failed to load \"bulletphysics/gameui_english.txt\"");
				}
			}
		}
		else if (g_pFileSystem_HL25)
		{
			if (!vgui::localize()->AddFile((IFileSystem*)g_pFileSystem_HL25, "bulletphysics/gameui_%language%.txt"))
			{
				if (!vgui::localize()->AddFile((IFileSystem*)g_pFileSystem_HL25, "bulletphysics/gameui_english.txt"))
				{
					Sys_Error("Failed to load \"bulletphysics/gameui_english.txt\"");
				}
			}
		}
	}

	void Shutdown(void) override
	{

	}

	void ActivateGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ActivateDemoUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HasExclusiveInput(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void RunFrame(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ConnectToServer(const char*& game, int& IP, int& port, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		
	}

	void DisconnectFromServer(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void HideGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void IsGameUIActive(VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void LoadingStarted(const char*& resourceType, const char*& resourceName, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void LoadingFinished(const char*& resourceType, const char*& resourceName, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void StartProgressBar(const char*& progressType, int& progressSteps, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void ContinueProgressBar(int& progressPoint, float& progressFraction, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void StopProgressBar(bool& bError, const char*& failureReason, const char*& extendedReason, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void SetProgressBarStatusText(const char*& statusText, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void SetSecondaryProgressBar(float& progress, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

	void SetSecondaryProgressBarText(const char*& statusText, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}

};

static CVGUI2Extension_GameUICallbacks s_GameUICallbacks;

/*
=================================================================================================================
GameUI OptionDialog Callbacks
=================================================================================================================
*/

class CVGUI2Extension_GameUIOptionDialogCallbacks : public IVGUI2Extension_GameUIOptionDialogCallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	void COptionsDialog_ctor(IGameUIOptionsDialogCtorCallbackContext* CallbackContext) override
	{
		CallbackContext->AddPage(new COptionsSubBulletPhysicsDlg((vgui::Panel*)CallbackContext->GetDialog()), "#GameUI_BulletPhysics_Tab");
	}

	void COptionsSubVideo_ApplyVidSettings(void*& pPanel, bool& bForceRestart, VGUI2Extension_CallbackContext* CallbackContext) override
	{

	}
};

static CVGUI2Extension_GameUIOptionDialogCallbacks s_GameUIOptionDialogCallbacks;

/*
=================================================================================================================
KeyValues Callbacks
=================================================================================================================
*/

class CVGUI2Extension_KeyValuesCallbacks : public IVGUI2Extension_KeyValuesCallbacks
{
public:
	int GetAltitude() const override
	{
		return 0;
	}

	void KeyValues_LoadFromFile(void*& pthis, IFileSystem*& pFileSystem, const char*& resourceName, const char*& pathId, const char* sourceModule, VGUI2Extension_CallbackContext* CallbackContext)
	{
		
	}
};

static CVGUI2Extension_KeyValuesCallbacks s_KeyValuesCallbacks;

/*
=================================================================================================================
GameUI init & shutdown
=================================================================================================================
*/

void GameUI_InstallHooks(void)
{
	if (!VGUI2Extension())
		return;

	VGUI2Extension()->RegisterGameUICallbacks(&s_GameUICallbacks);
	VGUI2Extension()->RegisterGameUIOptionDialogCallbacks(&s_GameUIOptionDialogCallbacks);
	//VGUI2Extension()->RegisterKeyValuesCallbacks(&s_KeyValuesCallbacks);
}

void GameUI_UninstallHooks(void)
{
	if (!VGUI2Extension())
		return;

	VGUI2Extension()->UnregisterGameUICallbacks(&s_GameUICallbacks);
	VGUI2Extension()->UnregisterGameUIOptionDialogCallbacks(&s_GameUIOptionDialogCallbacks);
	//VGUI2Extension()->UnregisterKeyValuesCallbacks(&s_KeyValuesCallbacks);
}