#pragma once

#include <Windows.h>
#include <imm.h>

// Return true only when the native window procedure must not see the message.
// Keep this before CGame::WindowProc: its DefWindowProcA runs before BaseUI.
template <typename Input>
bool HandleIMEWindowMessage(Input* input, HWND hwnd, UINT msg, WPARAM wparam, LPARAM& lparam)
{
	if (!input)
		return false;

	switch (msg)
	{
	case WM_CHAR:
	case WM_SYSCHAR:
		return input->IsIMEComposing();
	case WM_KEYDOWN:
	case WM_KEYUP:
		return wparam == VK_BACK && input->IsIMEComposing();
	case WM_INPUTLANGCHANGE:
		input->SetIMEWindow(hwnd);
		input->OnInputLanguageChanged();
		return false; // Preserve default propagation to child windows.
	case WM_IME_STARTCOMPOSITION:
		input->SetIMEWindow(hwnd);
		input->OnIMEStartComposition();
		return true;
	case WM_IME_COMPOSITION:
		input->SetIMEWindow(hwnd);
		input->OnIMECompositionWin32(lparam);
		return true;
	case WM_IME_ENDCOMPOSITION:
		input->SetIMEWindow(hwnd);
		input->OnIMEEndComposition();
		return true;
	case WM_IME_CHAR:
		// GCS_RESULTSTR is the sole committed-text source. Default processing
		// here would generate another stream of WM_CHAR messages.
		return true;
	case WM_IME_SETCONTEXT:
		lparam &= ~(ISC_SHOWUICOMPOSITIONWINDOW | ISC_SHOWUIGUIDELINE | ISC_SHOWUIALLCANDIDATEWINDOW);
		return false; // The engine passes the adjusted flags to DefWindowProcA.
	case WM_IME_NOTIFY:
		switch (wparam)
		{
		case IMN_OPENCANDIDATE:
			input->SetIMEWindow(hwnd);
			input->OnIMEShowCandidates();
			return true;
		case IMN_CHANGECANDIDATE:
			input->SetIMEWindow(hwnd);
			input->OnIMEChangeCandidates();
			return true;
		case IMN_CLOSECANDIDATE:
			input->SetIMEWindow(hwnd);
			input->OnIMECloseCandidates();
			return true;
		case IMN_SETCONVERSIONMODE:
		case IMN_SETSENTENCEMODE:
		case IMN_SETOPENSTATUS:
			input->SetIMEWindow(hwnd);
			input->OnIMERecomputeModes();
			return true;
		}
		break;
	}
	return false;
}
