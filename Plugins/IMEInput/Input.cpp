#include <metahook.h>
#include "BaseUI.h"
#include "Input.h"
#include "Encode.h"
#include "plugins.h"

using namespace vgui;

void (__fastcall *g_pfnInternalKeyTyped)(void *pthis, int, wchar_t unichar);

class CInput : public IInputInternal
{
public:
	virtual void SetMouseFocus(VPANEL newMouseFocus);
	virtual void SetMouseCapture(VPANEL panel);
	virtual void GetKeyCodeText(KeyCode code, char *buf, int buflen);
	virtual VPANEL GetFocus(void);
	virtual VPANEL GetMouseOver(void);
	virtual void SetCursorPos(int x, int y);
	virtual void GetCursorPos(int &x, int &y);
	virtual bool WasMousePressed(MouseCode code);
	virtual bool WasMouseDoublePressed(MouseCode code);
	virtual bool IsMouseDown(MouseCode code);
	virtual void SetCursorOveride(HCursor cursor);
	virtual HCursor GetCursorOveride(void);
	virtual bool WasMouseReleased(MouseCode code);
	virtual bool WasKeyPressed(KeyCode code);
	virtual bool IsKeyDown(KeyCode code);
	virtual bool WasKeyTyped(KeyCode code);
	virtual bool WasKeyReleased(KeyCode code);
	virtual VPANEL GetAppModalSurface(void);
	virtual void SetAppModalSurface(VPANEL panel);
	virtual void ReleaseAppModalSurface(void);
	virtual void GetCursorPosition(int &x, int &y);
	virtual void RunFrame(void);
	virtual void UpdateMouseFocus(int x, int y);
	virtual void PanelDeleted(VPANEL panel);
	virtual bool InternalCursorMoved(int x, int y);
	virtual bool InternalMousePressed(MouseCode code);
	virtual bool InternalMouseDoublePressed(MouseCode code);
	virtual bool InternalMouseReleased(MouseCode code);
	virtual bool InternalMouseWheeled(int delta);
	virtual bool InternalKeyCodePressed(KeyCode code);
	virtual void InternalKeyCodeTyped(KeyCode code);
	virtual void InternalKeyTyped(wchar_t unichar);
	virtual bool InternalKeyCodeReleased(KeyCode code);
	virtual HInputContext CreateInputContext(void);
	virtual void DestroyInputContext(HInputContext context);
	virtual void AssociatePanelWithInputContext(HInputContext context, VPANEL pRoot);
	virtual void ActivateInputContext(HInputContext context);
	virtual VPANEL GetMouseCapture(void);
	virtual bool IsChildOfModalPanel(VPANEL panel);
	virtual void ResetInputContext(HInputContext context);
};

CInput g_Input;
IInputInternal *g_pInput;

void CInput::SetMouseFocus(VPANEL newMouseFocus)
{
	g_pInput->SetMouseFocus(newMouseFocus);
}

void CInput::SetMouseCapture(VPANEL panel)
{
	g_pInput->SetMouseCapture(panel);
}

void CInput::GetKeyCodeText(KeyCode code, char *buf, int buflen)
{
	g_pInput->GetKeyCodeText(code, buf, buflen);
}

VPANEL CInput::GetFocus(void)
{
	return g_pInput->GetFocus();
}

VPANEL CInput::GetMouseOver(void)
{
	return g_pInput->GetMouseOver();
}

void CInput::SetCursorPos(int x, int y)
{
	g_pInput->SetCursorPos(x, y);
}

void CInput::GetCursorPos(int &x, int &y)
{
	g_pInput->GetCursorPos(x, y);
}

bool CInput::WasMousePressed(MouseCode code)
{
	return g_pInput->WasMousePressed(code);
}

bool CInput::WasMouseDoublePressed(MouseCode code)
{
	return g_pInput->WasMouseDoublePressed(code);
}

bool CInput::IsMouseDown(MouseCode code)
{
	return g_pInput->IsMouseDown(code);
}

void CInput::SetCursorOveride(HCursor cursor)
{
	return g_pInput->SetCursorOveride(cursor);
}

HCursor CInput::GetCursorOveride(void)
{
	return g_pInput->GetCursorOveride();
}

bool CInput::WasMouseReleased(MouseCode code)
{
	return g_pInput->WasMouseReleased(code);
}

bool CInput::WasKeyPressed(KeyCode code)
{
	return g_pInput->WasKeyPressed(code);
}

bool CInput::IsKeyDown(KeyCode code)
{
	return g_pInput->IsKeyDown(code);
}

bool CInput::WasKeyTyped(KeyCode code)
{
	return g_pInput->WasKeyTyped(code);
}

bool CInput::WasKeyReleased(KeyCode code)
{
	return g_pInput->WasKeyReleased(code);
}

VPANEL CInput::GetAppModalSurface(void)
{
	return g_pInput->GetAppModalSurface();
}

void CInput::SetAppModalSurface(VPANEL panel)
{
	g_pInput->SetAppModalSurface(panel);
}

void CInput::ReleaseAppModalSurface(void)
{
	g_pInput->ReleaseAppModalSurface();
}

void CInput::GetCursorPosition(int &x, int &y)
{
	g_pInput->GetCursorPosition(x, y);
}

void CInput::RunFrame(void)
{
	g_pInput->RunFrame();
}

void CInput::UpdateMouseFocus(int x, int y)
{
	g_pInput->UpdateMouseFocus(x, y);
}

void CInput::PanelDeleted(VPANEL panel)
{
	g_pInput->PanelDeleted(panel);
}

bool CInput::InternalCursorMoved(int x, int y)
{
	return g_pInput->InternalCursorMoved(x, y);
}

bool CInput::InternalMousePressed(MouseCode code)
{
	return g_pInput->InternalMousePressed(code);
}

bool CInput::InternalMouseDoublePressed(MouseCode code)
{
	return g_pInput->InternalMouseDoublePressed(code);
}

bool CInput::InternalMouseReleased(MouseCode code)
{
	return g_pInput->InternalMouseReleased(code);
}

bool CInput::InternalMouseWheeled(int delta)
{
	return g_pInput->InternalMouseWheeled(delta);
}

bool CInput::InternalKeyCodePressed(KeyCode code)
{
	return g_pInput->InternalKeyCodePressed(code);
}

void CInput::InternalKeyCodeTyped(KeyCode code)
{
	g_pInput->InternalKeyCodeTyped(code);
}

void CInput::InternalKeyTyped(wchar_t unichar)
{
	char cChar = (char)unichar;
	static bool bDoubleChar = false;
	static char sInpub[3] = { 0 };
	wchar_t *pComplete;

	if (bDoubleChar)
	{
		vgui::VPANEL focus = g_pInput->GetFocus();

		if (focus)
		{
			const char *module = g_pPanel->GetModuleName(focus);
			const char *name = g_pPanel->GetName(focus);

			if (!strcmp(name, "ConsoleEntry") || !strcmp(name, "NameEntry"))
			{
				void *panel = g_pPanel->GetPanel(focus, module);

				if (panel)
				{
					void **pVtable = *(void ***)panel;

					sInpub[1] = cChar;
					bDoubleChar = false;
					pComplete = ANSIToUnicode(sInpub);

					*(bool *)((char *)panel + 303) = true;
					reinterpret_cast<void (__fastcall *)(void *, int, wchar_t)>(pVtable[128])(panel, 0, pComplete[0]);
					return;
				}
			}
		}

		bDoubleChar = false;
	}
	else if (IsDBCSLeadByte(cChar))
	{
		bDoubleChar = true;
		sInpub[0] = cChar;
		return;
	}

	g_pfnInternalKeyTyped(this, 0, unichar);
}

bool CInput::InternalKeyCodeReleased(KeyCode code)
{
	return g_pInput->InternalKeyCodeReleased(code);
}

HInputContext CInput::CreateInputContext(void)
{
	return g_pInput->CreateInputContext();
}

void CInput::DestroyInputContext(HInputContext context)
{
	g_pInput->DestroyInputContext(context);
}

void CInput::AssociatePanelWithInputContext(HInputContext context, VPANEL pRoot)
{
	g_pInput->AssociatePanelWithInputContext(context, pRoot);
}

void CInput::ActivateInputContext(HInputContext context)
{
	g_pInput->ActivateInputContext(context);
}

VPANEL CInput::GetMouseCapture(void)
{
	return g_pInput->GetMouseCapture();
}

bool CInput::IsChildOfModalPanel(VPANEL panel)
{
	return g_pInput->IsChildOfModalPanel(panel);
}

void CInput::ResetInputContext(HInputContext context)
{
	return g_pInput->ResetInputContext(context);
}

void Input_InstallHook(vgui::IInputInternal *pInput)
{
	DWORD *pVFTable = *(DWORD **)&g_Input;

	g_pInput = pInput;

	if (g_dwEngineBuildnum < 5953)
	{
		g_pMetaHookAPI->VFTHook(pInput, 0, 32, (void *)pVFTable[32], (void *&)g_pfnInternalKeyTyped);
	}
}