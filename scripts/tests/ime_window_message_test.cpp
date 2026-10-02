#include "../../Plugins/VGUI2Extension/IMEWindowMessage.h"
#include <cassert>
#include <string>

struct FakeInput
{
	bool composing = false;
	HWND window = nullptr;
	int commits = 0;
	int notifications = 0;
	std::wstring text;
	bool IsIMEComposing() { return composing; }
	void SetIMEWindow(void* hwnd) { window = static_cast<HWND>(hwnd); }
	void OnInputLanguageChanged() { ++notifications; }
	void OnIMEStartComposition() { composing = true; }
	void OnIMEEndComposition() { composing = false; }
	void OnIMECompositionWin32(long flags)
	{
		if (flags & GCS_RESULTSTR)
		{
			++commits;
			text += L"\x554a\x554a";
		}
	}
	void OnIMEShowCandidates() { ++notifications; }
	void OnIMEChangeCandidates() { ++notifications; }
	void OnIMECloseCandidates() { ++notifications; }
	void OnIMERecomputeModes() { ++notifications; }
};

int main()
{
	FakeInput input;
	const auto hwnd = reinterpret_cast<HWND>(1);
	int forwarded = 0;
	auto dispatch = [&](UINT msg, WPARAM wp = 0, LPARAM lp = 0)
	{
		const bool handled = HandleIMEWindowMessage(&input, hwnd, msg, wp, lp);
		if (!handled)
			++forwarded;
		return handled;
	};
	assert(dispatch(WM_IME_STARTCOMPOSITION));
	assert(dispatch(WM_IME_COMPOSITION, 0, GCS_COMPSTR));
	assert(dispatch(WM_IME_COMPOSITION, 0, GCS_RESULTSTR));
	assert(dispatch(WM_IME_ENDCOMPOSITION));
	assert(dispatch(WM_IME_CHAR, 0x554a));
	assert(0 == forwarded);
	assert(1 == input.commits);
	assert(std::wstring(L"\x554a\x554a") == input.text);
	assert(hwnd == input.window);
	assert(!input.composing);
	assert(!dispatch(WM_CHAR, 'a'));
	assert(!dispatch(WM_KEYDOWN, VK_BACK));
	assert(!dispatch(WM_MOUSEMOVE));
	assert(dispatch(WM_IME_STARTCOMPOSITION));
	assert(dispatch(WM_CHAR, 'a'));
	assert(dispatch(WM_SYSCHAR, 'a'));
	assert(dispatch(WM_KEYDOWN, VK_BACK));
	assert(dispatch(WM_KEYUP, VK_BACK));
	assert(!dispatch(WM_KEYUP, 'A'));
	for (WPARAM notification : { IMN_OPENCANDIDATE, IMN_CHANGECANDIDATE, IMN_CLOSECANDIDATE,
		IMN_SETCONVERSIONMODE, IMN_SETSENTENCEMODE, IMN_SETOPENSTATUS })
		assert(dispatch(WM_IME_NOTIFY, notification));
	assert(6 == input.notifications);
	assert(!dispatch(WM_IME_NOTIFY, IMN_GUIDELINE));
	assert(!dispatch(WM_INPUTLANGCHANGE));
	assert(7 == input.notifications);
	LPARAM flags = ISC_SHOWUIALL | 0x10;
	assert(!HandleIMEWindowMessage(&input, hwnd, WM_IME_SETCONTEXT, TRUE, flags));
	assert(0 == (flags & (ISC_SHOWUICOMPOSITIONWINDOW | ISC_SHOWUIGUIDELINE | ISC_SHOWUIALLCANDIDATEWINDOW)));
	assert(0x10 == (flags & 0x10));
	assert(!HandleIMEWindowMessage<FakeInput>(nullptr, hwnd, WM_IME_COMPOSITION, 0, flags));
}
