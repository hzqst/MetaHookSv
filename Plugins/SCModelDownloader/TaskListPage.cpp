#include "TaskListPage.h"
#include "TaskListPanel.h"

#include "exportfuncs.h"

#include <format>

CTaskListPage::CTaskListPage(vgui::Panel* parent, const char* name) :
	BaseClass(parent, name)
{
	SetSize(vgui::scheme()->GetProportionalScaledValue(624), vgui::scheme()->GetProportionalScaledValue(300));

	m_pTaskListPanel = new CTaskListPanel(this, "TaskListPanel");

	LoadControlSettings("scmodeldownloader/TaskListPage.res", "GAME");

	vgui::ivgui()->AddTickSignal(GetVPanel());
	vgui::ipanel()->SendMessage(GetVPanel(), new KeyValues("ResetData"), GetVPanel());

	SCModelDatabase()->RegisterQueryStateChangeCallback(this);
}

CTaskListPage::~CTaskListPage()
{
	SCModelDatabase()->UnregisterQueryStateChangeCallback(this);
}

void CTaskListPage::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == vgui::KEY_ENTER)
	{
		if (m_pTaskListPanel->HasFocus() && m_pTaskListPanel->GetSelectedItemsCount() > 0)
		{
			return;
		}
	}

	BaseClass::OnKeyCodeTyped(code);
}

const char * UTIL_GetQueryStateName(SCModelQueryState state)
{
	/*
	SCModelQueryState_Unknown = 0,
	SCModelQueryState_Querying,
	SCModelQueryState_Receiving,
	SCModelQueryState_Failed,
	SCModelQueryState_Finished,
	*/
	const char* s_QueryStateName[] = {
		"Unknown",
		"Querying",
		"Receiving",
		"Failed",
		"Finished"
	};

	if (state >= 0 && state < _ARRAYSIZE(s_QueryStateName))
	{
		return s_QueryStateName[state];
	}

	return "Unknown";
}

void CTaskListPage::AddQueryItem(ISCModelQuery* pQuery)
{
	auto kv = new KeyValues("TaskItem");

	kv->SetInt("taskId", pQuery->GetTaskId());
	kv->SetString("name", pQuery->GetName());
	kv->SetString("identifier", pQuery->GetIdentifier());
	kv->SetString("url", pQuery->GetUrl());

	if (pQuery->GetState() == SCModelQueryState_Receiving && pQuery->GetProgress() >= 0)
	{
		auto progress = std::format("{0:.2f}%", pQuery->GetProgress() * 100.0f);
		kv->SetString("state", progress.c_str());
	}
	else
	{
		kv->SetString("state", UTIL_GetQueryStateName(pQuery->GetState()));
	}

	m_pTaskListPanel->AddItem(kv, pQuery->GetTaskId(), false, true);

	kv->deleteThis();
}

void CTaskListPage::OnEnumQuery(ISCModelQuery* pQuery)
{
	AddQueryItem(pQuery);
}

void CTaskListPage::OnQueryStateChanged(ISCModelQuery* pQuery, SCModelQueryState newState)
{
	for (int i = 0; i < m_pTaskListPanel->GetItemCount(); ++i)
	{
		auto userData = m_pTaskListPanel->GetItemUserData(i);

		if (userData == pQuery->GetTaskId())
		{
			m_pTaskListPanel->RemoveItem(i);

			break;
		}
	}

	AddQueryItem(pQuery);
}

void CTaskListPage::OnResetData()
{
	OnRefreshTaskList();
}

void CTaskListPage::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("ListSmall", IsProportional());

	if (!m_hFont)
		m_hFont = pScheme->GetFont("DefaultSmall", IsProportional());

	m_pTaskListPanel->SetFont(m_hFont);
}

void CTaskListPage::OnRefreshTaskList()
{
	m_pTaskListPanel->RemoveAll();

	SCModelDatabase()->EnumQueries(this);
}