#include <metahook.h>
#include "VGUI2ExtensionInternal.h"

#include <vector>
#include <algorithm>

const char* GetBaseDirectory();
const char* GetCurrentGameLanguage();

class CVGUI2Extension : public IVGUI2ExtensionInternal
{
private:
	std::vector<IVGUI2Extension_BaseUICallbacks*> m_BaseUICallbacks;
	std::vector<IVGUI2Extension_GameUICallbacks*> m_GameUICallbacks;
	std::vector<IVGUI2Extension_GameUIOptionDialogCallbacks*> m_GameUIOptionDialogCallbacks;
	std::vector<IVGUI2Extension_GameUITaskBarCallbacks*> m_GameUITaskBarCallbacks;
	std::vector<IVGUI2Extension_GameUIBasePanelCallbacks*> m_GameUIBasePanelCallbacks;
	std::vector<IVGUI2Extension_GameConsoleCallbacks*> m_GameConsoleCallbacks;
	std::vector<IVGUI2Extension_ClientVGUICallbacks*> m_ClientVGUICallbacks;
	std::vector<IVGUI2Extension_KeyValuesCallbacks*> m_KeyValuesCallbacks;

public:

	void RegisterBaseUICallbacks(IVGUI2Extension_BaseUICallbacks* pCallbacks) override
	{
		m_BaseUICallbacks.emplace_back(pCallbacks);

		std::sort(m_BaseUICallbacks.begin(), m_BaseUICallbacks.end(),
			[](const IVGUI2Extension_BaseUICallbacks* a, const IVGUI2Extension_BaseUICallbacks* b) -> bool {
				return a->GetAltitude() > b->GetAltitude();
			});
	}

	void RegisterGameUICallbacks(IVGUI2Extension_GameUICallbacks* pCallbacks) override
	{
		m_GameUICallbacks.emplace_back(pCallbacks);

		std::sort(m_GameUICallbacks.begin(), m_GameUICallbacks.end(),
			[](const IVGUI2Extension_GameUICallbacks* a, const IVGUI2Extension_GameUICallbacks* b) -> bool {
				return a->GetAltitude() > b->GetAltitude();
			});
	}

	void RegisterGameUIOptionDialogCallbacks(IVGUI2Extension_GameUIOptionDialogCallbacks* pCallbacks) override
	{
		m_GameUIOptionDialogCallbacks.emplace_back(pCallbacks);

		std::sort(m_GameUIOptionDialogCallbacks.begin(), m_GameUIOptionDialogCallbacks.end(),
			[](const IVGUI2Extension_GameUIOptionDialogCallbacks* a, const IVGUI2Extension_GameUIOptionDialogCallbacks* b) -> bool {
				return a->GetAltitude() > b->GetAltitude();
			});
	}

	void RegisterGameUITaskBarCallbacks(IVGUI2Extension_GameUITaskBarCallbacks* pCallbacks) override
	{
		m_GameUITaskBarCallbacks.emplace_back(pCallbacks);

		std::sort(m_GameUITaskBarCallbacks.begin(), m_GameUITaskBarCallbacks.end(),
			[](const IVGUI2Extension_GameUITaskBarCallbacks* a, const IVGUI2Extension_GameUITaskBarCallbacks* b) -> bool {
				return a->GetAltitude() > b->GetAltitude();
			});
	}

	void RegisterGameUIBasePanelCallbacks(IVGUI2Extension_GameUIBasePanelCallbacks* pCallbacks) override
	{
		m_GameUIBasePanelCallbacks.emplace_back(pCallbacks);

		std::sort(m_GameUIBasePanelCallbacks.begin(), m_GameUIBasePanelCallbacks.end(),
			[](const IVGUI2Extension_GameUIBasePanelCallbacks* a, const IVGUI2Extension_GameUIBasePanelCallbacks* b) -> bool {
				return a->GetAltitude() > b->GetAltitude();
			});
	}

	void RegisterKeyValuesCallbacks(IVGUI2Extension_KeyValuesCallbacks* pCallbacks) override
	{
		m_KeyValuesCallbacks.emplace_back(pCallbacks);

		std::sort(m_KeyValuesCallbacks.begin(), m_KeyValuesCallbacks.end(),
			[](const IVGUI2Extension_KeyValuesCallbacks* a, const IVGUI2Extension_KeyValuesCallbacks* b) -> bool {
				return a->GetAltitude() > b->GetAltitude();
			});
	}

	void RegisterGameConsoleCallbacks(IVGUI2Extension_GameConsoleCallbacks* pCallbacks) override
	{
		m_GameConsoleCallbacks.emplace_back(pCallbacks);

		std::sort(m_GameConsoleCallbacks.begin(), m_GameConsoleCallbacks.end(),
			[](const IVGUI2Extension_GameConsoleCallbacks* a, const IVGUI2Extension_GameConsoleCallbacks* b) -> bool {
				return a->GetAltitude() > b->GetAltitude();
			});
	}

	void RegisterClientVGUICallbacks(IVGUI2Extension_ClientVGUICallbacks* pCallbacks) override
	{
		m_ClientVGUICallbacks.emplace_back(pCallbacks);

		std::sort(m_ClientVGUICallbacks.begin(), m_ClientVGUICallbacks.end(),
			[](const IVGUI2Extension_ClientVGUICallbacks* a, const IVGUI2Extension_ClientVGUICallbacks* b) -> bool {
				return a->GetAltitude() > b->GetAltitude();
			});
	}

	void UnregisterBaseUICallbacks(IVGUI2Extension_BaseUICallbacks* pCallbacks) override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			if (*it == pCallbacks)
			{
				m_BaseUICallbacks.erase(it);
				return;
			}
		}
	}

	void UnregisterGameUICallbacks(IVGUI2Extension_GameUICallbacks* pCallbacks) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			if (*it == pCallbacks)
			{
				m_GameUICallbacks.erase(it);
				return;
			}
		}
	}

	void UnregisterGameUIOptionDialogCallbacks(IVGUI2Extension_GameUIOptionDialogCallbacks* pCallbacks) override
	{
		for (auto it = m_GameUIOptionDialogCallbacks.begin(); it != m_GameUIOptionDialogCallbacks.end(); ++it)
		{
			if (*it == pCallbacks)
			{
				m_GameUIOptionDialogCallbacks.erase(it);
				return;
			}
		}
	}

	void UnregisterGameUITaskBarCallbacks(IVGUI2Extension_GameUITaskBarCallbacks* pCallbacks) override
	{
		for (auto it = m_GameUITaskBarCallbacks.begin(); it != m_GameUITaskBarCallbacks.end(); ++it)
		{
			if (*it == pCallbacks)
			{
				m_GameUITaskBarCallbacks.erase(it);
				return;
			}
		}
	}

	void UnregisterGameUIBasePanelCallbacks(IVGUI2Extension_GameUIBasePanelCallbacks* pCallbacks) override
	{
		for (auto it = m_GameUIBasePanelCallbacks.begin(); it != m_GameUIBasePanelCallbacks.end(); ++it)
		{
			if (*it == pCallbacks)
			{
				m_GameUIBasePanelCallbacks.erase(it);
				return;
			}
		}
	}

	void UnregisterKeyValuesCallbacks(IVGUI2Extension_KeyValuesCallbacks* pCallbacks) override
	{
		for (auto it = m_KeyValuesCallbacks.begin(); it != m_KeyValuesCallbacks.end(); ++it)
		{
			if (*it == pCallbacks)
			{
				m_KeyValuesCallbacks.erase(it);
				return;
			}
		}
	}

	void UnregisterGameConsoleCallbacks(IVGUI2Extension_GameConsoleCallbacks* pCallbacks) override
	{
		for (auto it = m_GameConsoleCallbacks.begin(); it != m_GameConsoleCallbacks.end(); ++it)
		{
			if (*it == pCallbacks)
			{
				m_GameConsoleCallbacks.erase(it);
				return;
			}
		}
	}

	void UnregisterClientVGUICallbacks(IVGUI2Extension_ClientVGUICallbacks* pCallbacks) override
	{
		for (auto it = m_ClientVGUICallbacks.begin(); it != m_ClientVGUICallbacks.end(); ++it)
		{
			if (*it == pCallbacks)
			{
				m_ClientVGUICallbacks.erase(it);
				return;
			}
		}
	}

	const char* GetBaseDirectory() const override
	{
		return GetBaseDirectory();
	}

	const char* GetCurrentLanguage() const override
	{
		return GetCurrentGameLanguage();
	}

public:

	//BaseUI

	void BaseUI_Initialize(CreateInterfaceFn* factories, int count) override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			(*it)->Initialize(factories, count);
		}
	}

	void BaseUI_Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion) override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			(*it)->Start(engineFuncs, interfaceVersion);
		}
	}

	void BaseUI_Shutdown() override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			(*it)->Shutdown();
		}
	}

	void BaseUI_Key_Event(int& down, int& keynum, const char*& pszCurrentBinding, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			(*it)->Key_Event(down, keynum, pszCurrentBinding, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void BaseUI_CallEngineSurfaceAppProc(void*& pevent, void*& userData, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			(*it)->CallEngineSurfaceAppProc(pevent, userData, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void BaseUI_CallEngineSurfaceWndProc(void* &hwnd, unsigned int &msg, unsigned int &wparam, long &lparam, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			(*it)->CallEngineSurfaceWndProc(hwnd, msg, wparam, lparam, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void BaseUI_Paint(int& x, int& y, int& right, int& bottom, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			(*it)->Paint(x, y, right, bottom, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void BaseUI_HideGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			(*it)->HideGameUI(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void BaseUI_ActivateGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			(*it)->ActivateGameUI(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void BaseUI_HideConsole(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			(*it)->HideConsole(CallbackContext);
			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void BaseUI_ShowConsole(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_BaseUICallbacks.begin(); it != m_BaseUICallbacks.end(); ++it)
		{
			(*it)->ShowConsole(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	//GameUI

	void GameUI_Initialize(CreateInterfaceFn* factories, int count) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->Initialize(factories, count);
		}
	}

	void GameUI_PreStart(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->PreStart(engineFuncs, interfaceVersion, system);
		}
	}

	void GameUI_Start(struct cl_enginefuncs_s* engineFuncs, int interfaceVersion, void* system) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->Start(engineFuncs, interfaceVersion, system);
		}
	}

	void GameUI_Shutdown() override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->Shutdown();
		}
	}

	void GameUI_PostShutdown() override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->PostShutdown();
		}
	}

	void GameUI_ActivateGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->ActivateGameUI(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_ActivateDemoUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->ActivateDemoUI(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_HasExclusiveInput(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->HasExclusiveInput(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_RunFrame(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->RunFrame(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_ConnectToServer(const char*& game, int& IP, int& port, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->ConnectToServer(game, IP, port, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_DisconnectFromServer(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->DisconnectFromServer(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}
	void GameUI_HideGameUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->HideGameUI(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_IsGameUIActive(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->IsGameUIActive(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_LoadingStarted(const char*& resourceType, const char*& resourceName, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->LoadingStarted(resourceType, resourceName, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_LoadingFinished(const char*& resourceType, const char*& resourceName, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->LoadingFinished(resourceType, resourceName, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_StartProgressBar(const char*& progressType, int& progressSteps, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->StartProgressBar(progressType, progressSteps, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_ContinueProgressBar(int& progressPoint, float& progressFraction, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->ContinueProgressBar(progressPoint, progressFraction, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_StopProgressBar(bool& bError, const char*& failureReason, const char*& extendedReason, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->StopProgressBar(bError, failureReason, extendedReason, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_SetProgressBarStatusText(const char*& statusText, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->SetProgressBarStatusText(statusText, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_SetSecondaryProgressBar(float& progress, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->SetSecondaryProgressBar(progress, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_SetSecondaryProgressBarText(const char*& statusText, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUICallbacks.begin(); it != m_GameUICallbacks.end(); ++it)
		{
			(*it)->SetSecondaryProgressBarText(statusText, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	const char* GameUI_GetControlModuleName(int i) const override
	{
		return m_GameUICallbacks[i]->GetControlModuleName();
	}

	int GameUI_GetCallbackCount() const override
	{
		return (int)m_GameUICallbacks.size();
	}

	void GameUI_COptionsDialog_ctor(IGameUIOptionsDialogCtorCallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUIOptionDialogCallbacks.begin(); it != m_GameUIOptionDialogCallbacks.end(); ++it)
		{
			(*it)->COptionsDialog_ctor(CallbackContext);
		}
	}

	void GameUI_COptionsSubVideo_ApplyVidSettings(void* &pPanel, bool &bForceRestart, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUIOptionDialogCallbacks.begin(); it != m_GameUIOptionDialogCallbacks.end(); ++it)
		{
			(*it)->COptionsSubVideo_ApplyVidSettings(pPanel, bForceRestart, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_CTaskBar_ctor(IGameUITaskBarCtorCallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUITaskBarCallbacks.begin(); it != m_GameUITaskBarCallbacks.end(); ++it)
		{
			(*it)->CTaskBar_ctor(CallbackContext);
		}
	}

	void GameUI_CTaskBar_OnCommand(void*& pPanel, const char*& command, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUITaskBarCallbacks.begin(); it != m_GameUITaskBarCallbacks.end(); ++it)
		{
			(*it)->CTaskBar_OnCommand(pPanel, command, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameUI_CBasePanel_ctor(IGameUIBasePanelCtorCallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUIBasePanelCallbacks.begin(); it != m_GameUIBasePanelCallbacks.end(); ++it)
		{
			(*it)->CBasePanel_ctor(CallbackContext);
		}
	}

	void GameUI_CBasePanel_ApplySchemeSettings(void*& pPanel, void*& pScheme, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_GameUIBasePanelCallbacks.begin(); it != m_GameUIBasePanelCallbacks.end(); ++it)
		{
			(*it)->CBasePanel_ApplySchemeSettings(pPanel, pScheme, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void KeyValues_LoadFromFile(void*& pthis, IFileSystem*& pFileSystem, const char*& resourceName, const char*& pathId, const char *sourceModule, VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_KeyValuesCallbacks.begin(); it != m_KeyValuesCallbacks.end(); ++it)
		{
			(*it)->KeyValues_LoadFromFile(pthis, pFileSystem, resourceName, pathId, sourceModule, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	//ClientVGUI

	void ClientVGUI_Initialize(CreateInterfaceFn* factories, int count) override
	{
		for (auto it = m_ClientVGUICallbacks.begin(); it != m_ClientVGUICallbacks.end(); ++it)
		{
			(*it)->Initialize(factories, count);
		}
	}

	void ClientVGUI_Shutdown() override
	{
		for (auto it = m_ClientVGUICallbacks.begin(); it != m_ClientVGUICallbacks.end(); ++it)
		{
			(*it)->Shutdown();
		}
	}

	void ClientVGUI_Start() override
	{
		for (auto it = m_ClientVGUICallbacks.begin(); it != m_ClientVGUICallbacks.end(); ++it)
		{
			(*it)->Start();
		}
	}

	void ClientVGUI_SetParent(vgui::VPANEL parent) override
	{
		for (auto it = m_ClientVGUICallbacks.begin(); it != m_ClientVGUICallbacks.end(); ++it)
		{
			(*it)->SetParent(parent);
		}
	}

	void ClientVGUI_UseVGUI1(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_ClientVGUICallbacks.begin(); it != m_ClientVGUICallbacks.end(); ++it)
		{
			(*it)->UseVGUI1(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void ClientVGUI_HideScoreBoard(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_ClientVGUICallbacks.begin(); it != m_ClientVGUICallbacks.end(); ++it)
		{
			(*it)->HideScoreBoard(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void ClientVGUI_HideAllVGUIMenu(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_ClientVGUICallbacks.begin(); it != m_ClientVGUICallbacks.end(); ++it)
		{
			(*it)->HideAllVGUIMenu(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void ClientVGUI_ActivateClientUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_ClientVGUICallbacks.begin(); it != m_ClientVGUICallbacks.end(); ++it)
		{
			(*it)->ActivateClientUI(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void ClientVGUI_HideClientUI(VGUI2Extension_CallbackContext* CallbackContext) override
	{
		for (auto it = m_ClientVGUICallbacks.begin(); it != m_ClientVGUICallbacks.end(); ++it)
		{
			(*it)->HideClientUI(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	//GameConsole

	void GameConsole_Activate(VGUI2Extension_CallbackContext* CallbackContext)override
	{
		for (auto it = m_GameConsoleCallbacks.begin(); it != m_GameConsoleCallbacks.end(); ++it)
		{
			(*it)->Activate(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameConsole_Initialize(VGUI2Extension_CallbackContext* CallbackContext)override
	{
		for (auto it = m_GameConsoleCallbacks.begin(); it != m_GameConsoleCallbacks.end(); ++it)
		{
			(*it)->Initialize(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameConsole_Hide(VGUI2Extension_CallbackContext* CallbackContext)override
	{
		for (auto it = m_GameConsoleCallbacks.begin(); it != m_GameConsoleCallbacks.end(); ++it)
		{
			(*it)->Hide(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameConsole_Clear(VGUI2Extension_CallbackContext* CallbackContext)override
	{
		for (auto it = m_GameConsoleCallbacks.begin(); it != m_GameConsoleCallbacks.end(); ++it)
		{
			(*it)->Clear(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameConsole_IsConsoleVisible(VGUI2Extension_CallbackContext* CallbackContext)override
	{
		for (auto it = m_GameConsoleCallbacks.begin(); it != m_GameConsoleCallbacks.end(); ++it)
		{
			(*it)->IsConsoleVisible(CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameConsole_Printf(IVGUI2Extension_String* str, VGUI2Extension_CallbackContext* CallbackContext)override
	{
		for (auto it = m_GameConsoleCallbacks.begin(); it != m_GameConsoleCallbacks.end(); ++it)
		{
			(*it)->Printf(str, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameConsole_DPrintf(IVGUI2Extension_String* str, VGUI2Extension_CallbackContext* CallbackContext)override
	{
		for (auto it = m_GameConsoleCallbacks.begin(); it != m_GameConsoleCallbacks.end(); ++it)
		{
			(*it)->DPrintf(str, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}

	void GameConsole_SetParent(vgui::VPANEL parent, VGUI2Extension_CallbackContext* CallbackContext)override
	{
		for (auto it = m_GameConsoleCallbacks.begin(); it != m_GameConsoleCallbacks.end(); ++it)
		{
			(*it)->SetParent(parent, CallbackContext);

			if (CallbackContext->Result >= VGUI2Extension_Result::HANDLED)
			{
				return;
			}
		}
	}
};

static CVGUI2Extension s_VGUI2Extension;

IVGUI2ExtensionInternal* VGUI2ExtensionInternal()
{
	return &s_VGUI2Extension;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CVGUI2Extension, IVGUI2Extension, VGUI2_EXTENSION_INTERFACE_VERSION, s_VGUI2Extension);