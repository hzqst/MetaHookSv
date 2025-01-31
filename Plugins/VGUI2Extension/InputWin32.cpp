#include <metahook.h>
#include <capstone.h>
#include <vgui/IInput.h>
#include <vgui/IVGui.h>
#include <vgui/IInputInternal.h>
#include <vgui/IInput2.h>
#include <KeyValues.h>
#include "VPanel.h"
#include "UtlVector.h"
#include "UtlLinkedList.h"

#include <windows.h>
#include <imm.h>
#include <string>
#include <vector>

#pragma comment(lib, "Imm32.lib")

#include "plugins.h"

extern vgui::IVGui *g_pVGui;
extern vgui::IInput* g_pVGuiInput;

static bool g_bIMEComposing = false;
static double g_flImeComposingTime = 0;

static void* _imeWnd{};
static CANDIDATELIST* _imeCandidatesWin32{};
//static int _imeCandidateSelectedItem{};
//static std::vector<std::wstring> _imeCandidateList;
//static std::wstring _imeCompositionStr;

static bool(__fastcall* g_pfnCWin32Input_PostKeyMessage)(void* pthis, int, KeyValues* message);

void InputWin32_FillAddress(void)
{
	HMODULE hVGUI2 = GetModuleHandleA("vgui2.dll");

	if (1)
	{
		const char sigs1[] = "KeyCodeReleased";
		auto KeyCodeRelease_String = g_pMetaHookAPI->SearchPattern(hVGUI2, g_pMetaHookAPI->GetModuleSize(hVGUI2), sigs1, sizeof(sigs1) - 1);
		Sig_VarNotFound(KeyCodeRelease_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x8B\xC8";
		*(DWORD*)(pattern + 6) = (DWORD)KeyCodeRelease_String;
		auto KeyCodeRelease_PushString = g_pMetaHookAPI->SearchPattern(hVGUI2, g_pMetaHookAPI->GetModuleSize(hVGUI2), pattern, sizeof(pattern) - 1);
		Sig_VarNotFound(KeyCodeRelease_PushString);

		typedef struct
		{
			int iFoundPushEax;
		}KeyCodeRelease_ctx;

		KeyCodeRelease_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(KeyCodeRelease_PushString, 0x250, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				KeyCodeRelease_ctx* ctx = (KeyCodeRelease_ctx*)context;
				if (!ctx->iFoundPushEax && pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_EAX)
				{//.text:01D87E55 C7 01 B8 94 37 02                                   mov     dword ptr [ecx], offset pbodypart
					ctx->iFoundPushEax = 1;
					return FALSE;
				}

				if (ctx->iFoundPushEax)
				{
					if (address[0] == 0xE8 && instLen == 5)
					{
						g_pfnCWin32Input_PostKeyMessage = (decltype(g_pfnCWin32Input_PostKeyMessage))pinst->detail->x86.operands[0].imm;
					}
				}

				if (g_pfnCWin32Input_PostKeyMessage)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);
	}
}

namespace vgui
{
	typedef struct InputContext_s
	{
		VPANEL _rootPanel;

		bool _mousePressed[MOUSE_LAST];
		bool _mouseDoublePressed[MOUSE_LAST];
		bool _mouseDown[MOUSE_LAST];
		bool _mouseReleased[MOUSE_LAST];
		bool _keyPressed[KEY_LAST];
		bool _keyTyped[KEY_LAST];
		bool _keyDown[KEY_LAST];
		bool _keyReleased[KEY_LAST];

		VPanel *_keyFocus;
		VPanel *_oldMouseFocus;
		VPanel *_mouseFocus;
		VPanel *_mouseOver;

		VPanel *_mouseCapture;
		MouseCode m_MouseCaptureStartCode;
		VPanel *_appModalPanel;

		int m_nCursorX;
		int m_nCursorY;

		int m_nLastPostedCursorX;
		int m_nLastPostedCursorY;

		int m_nExternallySetCursorX;
		int m_nExternallySetCursorY;
		bool m_bSetCursorExplicitly;

		CUtlVector<VPanel *> m_KeyCodeUnhandledListeners;

		VPanel *m_pModalSubTree;
		VPanel *m_pUnhandledMouseClickListener;
		bool m_bRestrictMessagesToModalSubTree;
	}InputContext_t;
};

using namespace vgui;

enum LANGFLAG
{
	ENGLISH,
	TRADITIONAL_CHINESE,
	JAPANESE,
	KOREAN,
	SIMPLIFIED_CHINESE,
	UNKNOWN,
	NUM_IMES_SUPPORTED
}LangFlag;

struct LanguageIds
{
	unsigned short id;
	int languageflag;
	wchar_t	const *shortcode;
	wchar_t const *displayname;
	bool invertcomposition;
};

LanguageIds g_LanguageIds[] =
{
	{ 0x0000, UNKNOWN, L"", L"Neutral" },
	{ 0x007f, UNKNOWN, L"", L"Invariant" },
	{ 0x0400, UNKNOWN, L"", L"User Default Language" },
	{ 0x0800, UNKNOWN, L"", L"System Default Language" },
	{ 0x0436, UNKNOWN, L"AF", L"Afrikaans" },
	{ 0x041c, UNKNOWN, L"SQ", L"Albanian" },
	{ 0x0401, UNKNOWN, L"AR", L"Arabic (Saudi Arabia)" },
	{ 0x0801, UNKNOWN, L"AR", L"Arabic (Iraq)" },
	{ 0x0c01, UNKNOWN, L"AR", L"Arabic (Egypt)" },
	{ 0x1001, UNKNOWN, L"AR", L"Arabic (Libya)" },
	{ 0x1401, UNKNOWN, L"AR", L"Arabic (Algeria)" },
	{ 0x1801, UNKNOWN, L"AR", L"Arabic (Morocco)" },
	{ 0x1c01, UNKNOWN, L"AR", L"Arabic (Tunisia)" },
	{ 0x2001, UNKNOWN, L"AR", L"Arabic (Oman)" },
	{ 0x2401, UNKNOWN, L"AR", L"Arabic (Yemen)" },
	{ 0x2801, UNKNOWN, L"AR", L"Arabic (Syria)" },
	{ 0x2c01, UNKNOWN, L"AR", L"Arabic (Jordan)" },
	{ 0x3001, UNKNOWN, L"AR", L"Arabic (Lebanon)" },
	{ 0x3401, UNKNOWN, L"AR", L"Arabic (Kuwait)" },
	{ 0x3801, UNKNOWN, L"AR", L"Arabic (U.A.E.)" },
	{ 0x3c01, UNKNOWN, L"AR", L"Arabic (Bahrain)" },
	{ 0x4001, UNKNOWN, L"AR", L"Arabic (Qatar)" },
	{ 0x042b, UNKNOWN, L"HY", L"Armenian" },
	{ 0x042c, UNKNOWN, L"AZ", L"Azeri (Latin)" },
	{ 0x082c, UNKNOWN, L"AZ", L"Azeri (Cyrillic)" },
	{ 0x042d, UNKNOWN, L"ES", L"Basque" },
	{ 0x0423, UNKNOWN, L"BE", L"Belarusian" },
	{ 0x0445, UNKNOWN, L"", L"Bengali (India)" },
	{ 0x141a, UNKNOWN, L"", L"Bosnian (Bosnia and Herzegovina)" },
	{ 0x0402, UNKNOWN, L"BG", L"Bulgarian" },
	{ 0x0455, UNKNOWN, L"", L"Burmese" },
	{ 0x0403, UNKNOWN, L"CA", L"Catalan" },
	{ 0x0404, TRADITIONAL_CHINESE, L"CHT", L"#IME_0404", true },
	{ 0x0804, SIMPLIFIED_CHINESE, L"CHS", L"#IME_0804", true },
	{ 0x0c04, UNKNOWN, L"CH", L"Chinese (Hong Kong SAR, PRC)" },
	{ 0x1004, UNKNOWN, L"CH", L"Chinese (Singapore)" },
	{ 0x1404, UNKNOWN, L"CH", L"Chinese (Macao SAR)" },
	{ 0x041a, UNKNOWN, L"HR", L"Croatian" },
	{ 0x101a, UNKNOWN, L"HR", L"Croatian (Bosnia and Herzegovina)" },
	{ 0x0405, UNKNOWN, L"CZ", L"Czech" },
	{ 0x0406, UNKNOWN, L"DK", L"Danish" },
	{ 0x0465, UNKNOWN, L"MV", L"Divehi" },
	{ 0x0413, UNKNOWN, L"NL", L"Dutch (Netherlands)" },
	{ 0x0813, UNKNOWN, L"BE", L"Dutch (Belgium)" },
	{ 0x0409, ENGLISH, L"EN", L"#IME_0409" },
	{ 0x0809, ENGLISH, L"EN", L"English (United Kingdom)" },
	{ 0x0c09, ENGLISH, L"EN", L"English (Australian)" },
	{ 0x1009, ENGLISH, L"EN", L"English (Canadian)" },
	{ 0x1409, ENGLISH, L"EN", L"English (New Zealand)" },
	{ 0x1809, ENGLISH, L"EN", L"English (Ireland)" },
	{ 0x1c09, ENGLISH, L"EN", L"English (South Africa)" },
	{ 0x2009, ENGLISH, L"EN", L"English (Jamaica)" },
	{ 0x2409, ENGLISH, L"EN", L"English (Caribbean)" },
	{ 0x2809, ENGLISH, L"EN", L"English (Belize)" },
	{ 0x2c09, ENGLISH, L"EN", L"English (Trinidad)" },
	{ 0x3009, ENGLISH, L"EN", L"English (Zimbabwe)" },
	{ 0x3409, ENGLISH, L"EN", L"English (Philippines)" },
	{ 0x0425, UNKNOWN, L"ET", L"Estonian" },
	{ 0x0438, UNKNOWN, L"FO", L"Faeroese" },
	{ 0x0429, UNKNOWN, L"FA", L"Farsi" },
	{ 0x040b, UNKNOWN, L"FI", L"Finnish" },
	{ 0x040c, UNKNOWN, L"FR", L"#IME_040c" },
	{ 0x080c, UNKNOWN, L"FR", L"French (Belgian)" },
	{ 0x0c0c, UNKNOWN, L"FR", L"French (Canadian)" },
	{ 0x100c, UNKNOWN, L"FR", L"French (Switzerland)" },
	{ 0x140c, UNKNOWN, L"FR", L"French (Luxembourg)" },
	{ 0x180c, UNKNOWN, L"FR", L"French (Monaco)" },
	{ 0x0456, UNKNOWN, L"GL", L"Galician" },
	{ 0x0437, UNKNOWN, L"KA", L"Georgian" },
	{ 0x0407, UNKNOWN, L"DE", L"#IME_0407" },
	{ 0x0807, UNKNOWN, L"DE", L"German (Switzerland)" },
	{ 0x0c07, UNKNOWN, L"DE", L"German (Austria)" },
	{ 0x1007, UNKNOWN, L"DE", L"German (Luxembourg)" },
	{ 0x1407, UNKNOWN, L"DE", L"German (Liechtenstein)" },
	{ 0x0408, UNKNOWN, L"GR", L"Greek" },
	{ 0x0447, UNKNOWN, L"IN", L"Gujarati" },
	{ 0x040d, UNKNOWN, L"HE", L"Hebrew" },
	{ 0x0439, UNKNOWN, L"HI", L"Hindi" },
	{ 0x040e, UNKNOWN, L"HU", L"Hungarian" },
	{ 0x040f, UNKNOWN, L"IS", L"Icelandic" },
	{ 0x0421, UNKNOWN, L"ID", L"Indonesian" },
	{ 0x0434, UNKNOWN, L"", L"isiXhosa/Xhosa (South Africa)" },
	{ 0x0435, UNKNOWN, L"", L"isiZulu/Zulu (South Africa)" },
	{ 0x0410, UNKNOWN, L"IT", L"#IME_0410" },
	{ 0x0810, UNKNOWN, L"IT", L"Italian (Switzerland)" },
	{ 0x0411, JAPANESE, L"JP", L"#IME_0411" },
	{ 0x044b, UNKNOWN, L"IN", L"Kannada" },
	{ 0x0457, UNKNOWN, L"IN", L"Konkani" },
	{ 0x0412, KOREAN, L"KR", L"#IME_0412" },
	{ 0x0812, UNKNOWN, L"KR", L"Korean (Johab)" },
	{ 0x0440, UNKNOWN, L"KZ", L"Kyrgyz." },
	{ 0x0426, UNKNOWN, L"LV", L"Latvian" },
	{ 0x0427, UNKNOWN, L"LT", L"Lithuanian" },
	{ 0x0827, UNKNOWN, L"LT", L"Lithuanian (Classic)" },
	{ 0x042f, UNKNOWN, L"MK", L"FYRO Macedonian" },
	{ 0x043e, UNKNOWN, L"MY", L"Malay (Malaysian)" },
	{ 0x083e, UNKNOWN, L"MY", L"Malay (Brunei Darussalam)" },
	{ 0x044c, UNKNOWN, L"IN", L"Malayalam (India)" },
	{ 0x0481, UNKNOWN, L"", L"Maori (New Zealand)" },
	{ 0x043a, UNKNOWN, L"", L"Maltese (Malta)" },
	{ 0x044e, UNKNOWN, L"IN", L"Marathi" },
	{ 0x0450, UNKNOWN, L"MN", L"Mongolian" },
	{ 0x0414, UNKNOWN, L"NO", L"Norwegian (Bokmal)" },
	{ 0x0814, UNKNOWN, L"NO", L"Norwegian (Nynorsk)" },
	{ 0x0415, UNKNOWN, L"PL", L"Polish" },
	{ 0x0416, UNKNOWN, L"PT", L"Portuguese (Brazil)" },
	{ 0x0816, UNKNOWN, L"PT", L"Portuguese (Portugal)" },
	{ 0x0446, UNKNOWN, L"IN", L"Punjabi" },
	{ 0x046b, UNKNOWN, L"", L"Quechua (Bolivia)" },
	{ 0x086b, UNKNOWN, L"", L"Quechua (Ecuador)" },
	{ 0x0c6b, UNKNOWN, L"", L"Quechua (Peru)" },
	{ 0x0418, UNKNOWN, L"RO", L"Romanian" },
	{ 0x0419, UNKNOWN, L"RU", L"#IME_0419" },
	{ 0x044f, UNKNOWN, L"IN", L"Sanskrit" },
	{ 0x043b, UNKNOWN, L"", L"Sami, Northern (Norway)" },
	{ 0x083b, UNKNOWN, L"", L"Sami, Northern (Sweden)" },
	{ 0x0c3b, UNKNOWN, L"", L"Sami, Northern (Finland)" },
	{ 0x103b, UNKNOWN, L"", L"Sami, Lule (Norway)" },
	{ 0x143b, UNKNOWN, L"", L"Sami, Lule (Sweden)" },
	{ 0x183b, UNKNOWN, L"", L"Sami, Southern (Norway)" },
	{ 0x1c3b, UNKNOWN, L"", L"Sami, Southern (Sweden)" },
	{ 0x203b, UNKNOWN, L"", L"Sami, Skolt (Finland)" },
	{ 0x243b, UNKNOWN, L"", L"Sami, Inari (Finland)" },
	{ 0x0c1a, UNKNOWN, L"SR", L"Serbian (Cyrillic)" },
	{ 0x1c1a, UNKNOWN, L"SR", L"Serbian (Cyrillic, Bosnia, and Herzegovina)" },
	{ 0x081a, UNKNOWN, L"SR", L"Serbian (Latin)" },
	{ 0x181a, UNKNOWN, L"SR", L"Serbian (Latin, Bosnia, and Herzegovina)" },
	{ 0x046c, UNKNOWN, L"", L"Sesotho sa Leboa/Northern Sotho (South Africa)" },
	{ 0x0432, UNKNOWN, L"", L"Setswana/Tswana (South Africa)" },
	{ 0x041b, UNKNOWN, L"SK", L"Slovak" },
	{ 0x0424, UNKNOWN, L"SI", L"Slovenian" },
	{ 0x040a, UNKNOWN, L"ES", L"#IME_040a" },
	{ 0x080a, UNKNOWN, L"ES", L"Spanish (Mexican)" },
	{ 0x0c0a, UNKNOWN, L"ES", L"Spanish (Spain, Modern Sort)" },
	{ 0x100a, UNKNOWN, L"ES", L"Spanish (Guatemala)" },
	{ 0x140a, UNKNOWN, L"ES", L"Spanish (Costa Rica)" },
	{ 0x180a, UNKNOWN, L"ES", L"Spanish (Panama)" },
	{ 0x1c0a, UNKNOWN, L"ES", L"Spanish (Dominican Republic)" },
	{ 0x200a, UNKNOWN, L"ES", L"Spanish (Venezuela)" },
	{ 0x240a, UNKNOWN, L"ES", L"Spanish (Colombia)" },
	{ 0x280a, UNKNOWN, L"ES", L"Spanish (Peru)" },
	{ 0x2c0a, UNKNOWN, L"ES", L"Spanish (Argentina)" },
	{ 0x300a, UNKNOWN, L"ES", L"Spanish (Ecuador)" },
	{ 0x340a, UNKNOWN, L"ES", L"Spanish (Chile)" },
	{ 0x380a, UNKNOWN, L"ES", L"Spanish (Uruguay)" },
	{ 0x3c0a, UNKNOWN, L"ES", L"Spanish (Paraguay)" },
	{ 0x400a, UNKNOWN, L"ES", L"Spanish (Bolivia)" },
	{ 0x440a, UNKNOWN, L"ES", L"Spanish (El Salvador)" },
	{ 0x480a, UNKNOWN, L"ES", L"Spanish (Honduras)" },
	{ 0x4c0a, UNKNOWN, L"ES", L"Spanish (Nicaragua)" },
	{ 0x500a, UNKNOWN, L"ES", L"Spanish (Puerto Rico)" },
	{ 0x0430, UNKNOWN, L"", L"Sutu" },
	{ 0x0441, UNKNOWN, L"KE", L"Swahili (Kenya)" },
	{ 0x041d, UNKNOWN, L"SV", L"Swedish" },
	{ 0x081d, UNKNOWN, L"SV", L"Swedish (Finland)" },
	{ 0x045a, UNKNOWN, L"SY", L"Syriac" },
	{ 0x0449, UNKNOWN, L"IN", L"Tamil" },
	{ 0x0444, UNKNOWN, L"RU", L"Tatar (Tatarstan)" },
	{ 0x044a, UNKNOWN, L"IN", L"Telugu" },
	{ 0x041e, UNKNOWN, L"TH", L"#IME_041e" },
	{ 0x041f, UNKNOWN, L"TR", L"Turkish" },
	{ 0x0422, UNKNOWN, L"UA", L"Ukrainian" },
	{ 0x0420, UNKNOWN, L"PK", L"Urdu (Pakistan)" },
	{ 0x0820, UNKNOWN, L"IN", L"Urdu (India)" },
	{ 0x0443, UNKNOWN, L"UZ", L"Uzbek (Latin)" },
	{ 0x0843, UNKNOWN, L"UZ", L"Uzbek (Cyrillic)" },
	{ 0x042a, UNKNOWN, L"VN", L"Vietnamese" },
	{ 0x0452, UNKNOWN, L"", L"Welsh (United Kingdom)" },
};

struct IMESettingsTransform
{
	IMESettingsTransform(unsigned int cmr, unsigned int cma, unsigned int smr, unsigned int sma) : cmode_remove(cmr), cmode_add(cma), smode_remove(smr), smode_add(sma)
	{
	}

	void Apply(HWND hwnd)
	{
		HIMC hImc = ImmGetContext(hwnd);

		if (hImc)
		{
			DWORD dwConvMode, dwSentMode;
			ImmGetConversionStatus(hImc, &dwConvMode, &dwSentMode);

			dwConvMode &= ~cmode_remove;
			dwSentMode &= ~smode_remove;

			dwConvMode |= cmode_add;
			dwSentMode |= smode_add;

			ImmSetConversionStatus(hImc, dwConvMode, dwSentMode);
			ImmReleaseContext(hwnd, hImc);
		}
	}

	bool ConvMatches(DWORD convFlags)
	{
		convFlags &= (cmode_remove | cmode_add);

		if (convFlags & cmode_remove)
			return false;

		if ((convFlags == cmode_add) || (convFlags & cmode_add))
			return true;

		return false;
	}

	bool SentMatches(DWORD sentFlags)
	{
		sentFlags &= (smode_remove | smode_add);

		if (sentFlags & smode_remove)
			return false;

		if ((sentFlags & smode_add) == smode_add)
			return true;

		return false;
	}

	unsigned int cmode_remove;
	unsigned int cmode_add;
	unsigned int smode_remove;
	unsigned int smode_add;
};

static IMESettingsTransform g_ConversionMode_CHT_ToChinese(IME_CMODE_ALPHANUMERIC, IME_CMODE_NATIVE | IME_CMODE_LANGUAGE, 0, 0);
static IMESettingsTransform g_ConversionMode_CHT_ToEnglish(IME_CMODE_NATIVE | IME_CMODE_LANGUAGE, IME_CMODE_ALPHANUMERIC, 0, 0);
static IMESettingsTransform g_ConversionMode_CHS_ToChinese(IME_CMODE_ALPHANUMERIC, IME_CMODE_NATIVE | IME_CMODE_LANGUAGE, 0, 0);
static IMESettingsTransform g_ConversionMode_CHS_ToEnglish(IME_CMODE_NATIVE | IME_CMODE_LANGUAGE, IME_CMODE_ALPHANUMERIC, 0, 0);
static IMESettingsTransform g_ConversionMode_KO_ToKorean(IME_CMODE_ALPHANUMERIC, IME_CMODE_NATIVE | IME_CMODE_LANGUAGE, 0, 0);
static IMESettingsTransform g_ConversionMode_KO_ToEnglish(IME_CMODE_NATIVE | IME_CMODE_LANGUAGE, IME_CMODE_ALPHANUMERIC, 0, 0);
static IMESettingsTransform g_ConversionMode_JP_Hiragana(IME_CMODE_ALPHANUMERIC | IME_CMODE_KATAKANA, IME_CMODE_NATIVE | IME_CMODE_FULLSHAPE, 0, 0);
static IMESettingsTransform g_ConversionMode_JP_DirectInput(IME_CMODE_NATIVE | (IME_CMODE_KATAKANA | IME_CMODE_LANGUAGE) | IME_CMODE_FULLSHAPE | IME_CMODE_ROMAN, IME_CMODE_ALPHANUMERIC, 0, 0);
static IMESettingsTransform g_ConversionMode_JP_FullwidthKatakana(IME_CMODE_ALPHANUMERIC, IME_CMODE_NATIVE | IME_CMODE_FULLSHAPE | IME_CMODE_ROMAN | IME_CMODE_KATAKANA | IME_CMODE_LANGUAGE, 0, 0);
static IMESettingsTransform g_ConversionMode_JP_HalfwidthKatakana(IME_CMODE_ALPHANUMERIC | IME_CMODE_FULLSHAPE, IME_CMODE_NATIVE | IME_CMODE_ROMAN | (IME_CMODE_KATAKANA | IME_CMODE_LANGUAGE), 0, 0);
static IMESettingsTransform g_ConversionMode_JP_FullwidthAlphanumeric(IME_CMODE_NATIVE | (IME_CMODE_KATAKANA | IME_CMODE_LANGUAGE), IME_CMODE_ALPHANUMERIC | IME_CMODE_FULLSHAPE | IME_CMODE_ROMAN, 0, 0);
static IMESettingsTransform g_ConversionMode_JP_HalfwidthAlphanumeric(IME_CMODE_NATIVE | (IME_CMODE_KATAKANA | IME_CMODE_LANGUAGE) | IME_CMODE_FULLSHAPE, IME_CMODE_ALPHANUMERIC | IME_CMODE_ROMAN, 0, 0);

static IMESettingsTransform g_SentenceMode_JP_None(0, 0, IME_SMODE_PLAURALCLAUSE | IME_SMODE_SINGLECONVERT | IME_SMODE_AUTOMATIC | IME_SMODE_PHRASEPREDICT | IME_SMODE_CONVERSATION, IME_SMODE_NONE);
static IMESettingsTransform g_SentenceMode_JP_General(0, 0, IME_SMODE_NONE | IME_SMODE_PLAURALCLAUSE | IME_SMODE_SINGLECONVERT | IME_SMODE_AUTOMATIC | IME_SMODE_CONVERSATION, IME_SMODE_PHRASEPREDICT);
static IMESettingsTransform g_SentenceMode_JP_BiasNames(0, 0, IME_SMODE_NONE | IME_SMODE_PHRASEPREDICT | IME_SMODE_SINGLECONVERT | IME_SMODE_AUTOMATIC | IME_SMODE_CONVERSATION, IME_SMODE_PLAURALCLAUSE);
static IMESettingsTransform g_SentenceMode_JP_BiasSpeech(0, 0, IME_SMODE_NONE | IME_SMODE_PHRASEPREDICT | IME_SMODE_SINGLECONVERT | IME_SMODE_AUTOMATIC | IME_SMODE_PLAURALCLAUSE, IME_SMODE_CONVERSATION);

static bool IsIDInList(unsigned short id, int count, HKL *list)
{
	for (int i = 0; i < count; ++i)
	{
		if (LOWORD(list[i]) == id)
		{
			return true;
		}
	}

	return false;
}

static LanguageIds *GetLanguageInfo(unsigned short id)
{
	for (int j = 0; j < sizeof(g_LanguageIds) / sizeof(g_LanguageIds[0]); ++j)
	{
		if (g_LanguageIds[j].id == id)
		{
			return &g_LanguageIds[j];
			break;
		}
	}

	return NULL;
}

static const wchar_t *GetLanguageName(unsigned short id)
{
	wchar_t const *name = L"???";

	for (int j = 0; j < sizeof(g_LanguageIds) / sizeof(g_LanguageIds[0]); ++j)
	{
		if (g_LanguageIds[j].id == id)
		{
			name = g_LanguageIds[j].displayname;
			break;
		}
	}

	return name;
}

static void SpewIMEInfo(int langid)
{
#ifdef _DEBUG
	LanguageIds *info = GetLanguageInfo(langid);

	if (info)
	{
		wchar_t const *name = info->shortcode ? info->shortcode : L"???";
		wchar_t outstr[512];
		_snwprintf(outstr, sizeof(outstr) / sizeof(wchar_t), L"IME language changed to:  %s", name);
		OutputDebugStringW(outstr);
		OutputDebugStringW(L"\n");
	}
#endif
}

class CInput2 : public IInput2
{
public:
	void SetMouseFocus(VPANEL newMouseFocus) override
	{
		return g_pVGuiInput->SetMouseFocus(newMouseFocus);
	}

	void SetMouseCapture(VPANEL panel) override
	{
		return g_pVGuiInput->SetMouseCapture(panel);
	}

	void GetKeyCodeText(KeyCode code, char* buf, int buflen) override
	{
		return g_pVGuiInput->GetKeyCodeText(code, buf, buflen);
	}

	VPANEL GetFocus(void) override
	{
		return g_pVGuiInput->GetFocus();
	}

	VPANEL GetMouseOver(void) override
	{
		return g_pVGuiInput->GetMouseOver();
	}

	void SetCursorPos(int x, int y) override
	{
		return g_pVGuiInput->SetCursorPos(x, y);
	}

	void GetCursorPos(int& x, int& y) override
	{
		return g_pVGuiInput->GetCursorPos(x, y);
	}

	bool WasMousePressed(MouseCode code) override
	{
		return g_pVGuiInput->WasMousePressed(code);
	}

	bool WasMouseDoublePressed(MouseCode code) override
	{
		return g_pVGuiInput->WasMouseDoublePressed(code);
	}

	bool IsMouseDown(MouseCode code) override
	{
		return g_pVGuiInput->IsMouseDown(code);
	}

	void SetCursorOveride(HCursor cursor) override
	{
		g_pVGuiInput->SetCursorOveride(cursor);
	}

	HCursor GetCursorOveride(void) override
	{
		return g_pVGuiInput->GetCursorOveride();
	}

	bool WasMouseReleased(MouseCode code) override
	{
		return g_pVGuiInput->WasMouseReleased(code);
	}

	bool WasKeyPressed(KeyCode code) override
	{
		return g_pVGuiInput->WasKeyPressed(code);
	}

	bool IsKeyDown(KeyCode code) override
	{
		return g_pVGuiInput->IsKeyDown(code);
	}

	bool WasKeyTyped(KeyCode code) override
	{
		return g_pVGuiInput->WasKeyTyped(code);
	}

	bool WasKeyReleased(KeyCode code) override
	{
		return g_pVGuiInput->WasKeyReleased(code);
	}

	VPANEL GetAppModalSurface(void) override
	{
		return g_pVGuiInput->GetAppModalSurface();
	}

	void SetAppModalSurface(VPANEL panel) override
	{
		g_pVGuiInput->SetAppModalSurface(panel);
	}

	void ReleaseAppModalSurface(void) override
	{
		g_pVGuiInput->ReleaseAppModalSurface();
	}

	void GetCursorPosition(int& x, int& y) override
	{
		g_pVGuiInput->GetCursorPosition(x, y);
	}

	void RunFrame(void) override
	{
		return ((vgui::IInputInternal *)g_pVGuiInput)->RunFrame();
	}
	void UpdateMouseFocus(int x, int y) override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->UpdateMouseFocus(x, y);
	}

	void PanelDeleted(VPANEL panel)  override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->PanelDeleted(panel);
	}

	bool InternalCursorMoved(int x, int y) override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->InternalCursorMoved(x, y);
	}

	bool InternalMousePressed(MouseCode code) override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->InternalMousePressed(code);
	}

	bool InternalMouseDoublePressed(MouseCode code) override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->InternalMouseDoublePressed(code);
	}

	bool InternalMouseReleased(MouseCode code) override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->InternalMouseReleased(code);
	}

	bool InternalMouseWheeled(int delta) override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->InternalMouseWheeled(delta);
	}

	bool InternalKeyCodePressed(KeyCode code) override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->InternalKeyCodePressed(code);
	}

	void InternalKeyCodeTyped(KeyCode code) override
	{
		((vgui::IInputInternal*)g_pVGuiInput)->InternalKeyCodeTyped(code);
	}

	void InternalKeyTyped(wchar_t unichar) override
	{
		((vgui::IInputInternal*)g_pVGuiInput)->InternalKeyTyped(unichar);
	}

	bool InternalKeyCodeReleased(KeyCode code) override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->InternalKeyCodeReleased(code);
	}

	HInputContext CreateInputContext(void) override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->CreateInputContext();
	}

	void DestroyInputContext(HInputContext context) override
	{
		((vgui::IInputInternal*)g_pVGuiInput)->DestroyInputContext(context);
	}

	void AssociatePanelWithInputContext(HInputContext context, VPANEL pRoot) override
	{
		((vgui::IInputInternal*)g_pVGuiInput)->AssociatePanelWithInputContext(context, pRoot);
	}

	void ActivateInputContext(HInputContext context) override
	{
		((vgui::IInputInternal*)g_pVGuiInput)->ActivateInputContext(context);
	}

	VPANEL GetMouseCapture(void) override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->GetMouseCapture();
	}

	bool IsChildOfModalPanel(VPANEL panel) override
	{
		return ((vgui::IInputInternal*)g_pVGuiInput)->IsChildOfModalPanel(panel);
	}

	void ResetInputContext(HInputContext context) override
	{
		((vgui::IInputInternal*)g_pVGuiInput)->ResetInputContext(context);
	}
public:

	void SetIMEWindow(void* hwnd) override
	{
		_imeWnd = hwnd;
	}

	void* GetIMEWindow() override
	{
		return _imeWnd;
	}

	void OnChangeIME(bool forward) override
	{
		HKL currentKb = GetKeyboardLayout(0);
		UINT numKBs = GetKeyboardLayoutList(0, NULL);

		if (numKBs > 0)
		{
			HKL* list = new HKL[numKBs];
			GetKeyboardLayoutList(numKBs, list);

			int oldKb = 0;
			CUtlVector<HKL> selections;

			for (unsigned int i = 0; i < numKBs; ++i)
			{
				BOOL first = !IsIDInList(LOWORD(list[i]), i, list);

				if (!first)
					continue;

				selections.AddToTail(list[i]);

				if (list[i] == currentKb)
					oldKb = selections.Count() - 1;
			}

			oldKb += forward ? 1 : -1;

			if (oldKb < 0)
			{
				oldKb = max(0, selections.Count() - 1);
			}
			else if (oldKb >= selections.Count())
			{
				oldKb = 0;
			}

			ActivateKeyboardLayout(selections[oldKb], 0);

			int langid = LOWORD(selections[oldKb]);
			SpewIMEInfo(langid);

			delete[] list;
		}
	}

	int GetCurrentIMEHandle() override
	{
		HKL hkl = (HKL)GetKeyboardLayout(0);
		return (int)hkl;
	}

	int GetEnglishIMEHandle() override
	{
		HKL hkl = (HKL)0x04090409;
		return (int)hkl;
	}

	// Returns the Language Bar label (Chinese, Korean, Japanese, Russion, Thai, etc.)
	void GetIMELanguageName(wchar_t* buf, int unicodeBufferSizeInBytes) override
	{
		buf[0] = 0;

		wchar_t const* name = GetLanguageName(LOWORD(GetKeyboardLayout(0)));
		wcsncpy(buf, name, unicodeBufferSizeInBytes / sizeof(wchar_t) - 1);
		buf[unicodeBufferSizeInBytes / sizeof(wchar_t) - 1] = L'\0';
	}

	// Returns the short code for the language (EN, CH, KO, JP, RU, TH, etc. ).
	void GetIMELanguageShortCode(wchar_t* buf, int unicodeBufferSizeInBytes) override
	{
		buf[0] = 0;

		LanguageIds* info = GetLanguageInfo(LOWORD(GetKeyboardLayout(0)));

		if (!info)
		{
			buf[0] = L'\0';
		}
		else
		{
			wcsncpy(buf, info->shortcode, unicodeBufferSizeInBytes / sizeof(wchar_t) - 1);
			buf[unicodeBufferSizeInBytes / sizeof(wchar_t) - 1] = L'\0';
		}
	}

	// Call with NULL dest to get item count
	int GetIMELanguageList(LanguageItem* dest, int destcount) override
	{
		//return 0;
		int iret = 0;
		UINT numKBs = GetKeyboardLayoutList(0, NULL);

		if (numKBs > 0)
		{
			HKL* list = new HKL[numKBs];
			GetKeyboardLayoutList(numKBs, list);

			CUtlVector<HKL> selections;

			for (unsigned int i = 0; i < numKBs; ++i)
			{
				BOOL first = !IsIDInList(LOWORD(list[i]), i, list);

				if (!first)
					continue;

				selections.AddToTail(list[i]);
			}

			iret = selections.Count();

			if (dest)
			{
				int langid = LOWORD(GetKeyboardLayout(0));

				for (int i = 0; i < min(iret, destcount); ++i)
				{
					HKL hkl = selections[i];
					LanguageItem* p = &dest[i];

					LanguageIds* info = GetLanguageInfo(LOWORD(hkl));
					memset(p, 0, sizeof(LanguageItem));

					wcsncpy(p->shortname, info->shortcode, sizeof(p->shortname) / sizeof(wchar_t));
					p->shortname[sizeof(p->shortname) / sizeof(wchar_t) - 1] = L'\0';

					wcsncpy(p->menuname, info->displayname, sizeof(p->menuname) / sizeof(wchar_t));
					p->menuname[sizeof(p->menuname) / sizeof(wchar_t) - 1] = L'\0';

					p->handleValue = (int)hkl;
					p->active = (info->id == langid) ? true : false;
				}
			}

			delete[] list;
		}

		return iret;
	}

	int GetIMEConversionModes(ConversionModeItem* dest, int destcount) override
	{
		//return 0;
		if (dest)
			memset(dest, 0, destcount * sizeof(ConversionModeItem));

		DWORD dwConvMode = 0, dwSentMode = 0;
		HIMC hImc = ImmGetContext((HWND)GetIMEWindow());

		if (hImc)
		{
			ImmGetConversionStatus(hImc, &dwConvMode, &dwSentMode);
			ImmReleaseContext((HWND)GetIMEWindow(), hImc);
		}

		LanguageIds* info = GetLanguageInfo(LOWORD(GetKeyboardLayout(0)));

		switch (info->languageflag)
		{
		default:
		{
			return 0;
		}

		case TRADITIONAL_CHINESE:
		{
			if (dest)
			{
				ConversionModeItem* item;
				int i = 0;
				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_Chinese", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_CHT_ToChinese;
				item->active = g_ConversionMode_CHT_ToChinese.ConvMatches(dwConvMode);

				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_English", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_CHT_ToEnglish;
				item->active = g_ConversionMode_CHT_ToEnglish.ConvMatches(dwConvMode);
			}

			return 2;
		}

		case JAPANESE:
		{
			if (dest)
			{
				ConversionModeItem* item;

				int i = 0;
				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_Hiragana", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_JP_Hiragana;
				item->active = g_ConversionMode_JP_Hiragana.ConvMatches(dwConvMode);

				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_FullWidthKatakana", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_JP_FullwidthKatakana;
				item->active = g_ConversionMode_JP_FullwidthKatakana.ConvMatches(dwConvMode);

				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_FullWidthAlphanumeric", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_JP_FullwidthAlphanumeric;
				item->active = g_ConversionMode_JP_FullwidthAlphanumeric.ConvMatches(dwConvMode);

				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_HalfWidthKatakana", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_JP_HalfwidthKatakana;
				item->active = g_ConversionMode_JP_HalfwidthKatakana.ConvMatches(dwConvMode);

				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_HalfWidthAlphanumeric", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_JP_HalfwidthAlphanumeric;
				item->active = g_ConversionMode_JP_HalfwidthAlphanumeric.ConvMatches(dwConvMode);

				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_English", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_JP_DirectInput;
				item->active = g_ConversionMode_JP_DirectInput.ConvMatches(dwConvMode);
			}

			return 6;
		}

		case KOREAN:
		{
			if (dest)
			{
				ConversionModeItem* item;
				int i = 0;
				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_Korean", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_KO_ToKorean;
				item->active = g_ConversionMode_KO_ToKorean.ConvMatches(dwConvMode);

				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_English", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_KO_ToEnglish;
				item->active = g_ConversionMode_KO_ToEnglish.ConvMatches(dwConvMode);
			}

			return 2;
		}

		case SIMPLIFIED_CHINESE:
		{
			if (dest)
			{
				ConversionModeItem* item;
				int i = 0;
				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_Chinese", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_CHS_ToChinese;
				item->active = g_ConversionMode_CHS_ToChinese.ConvMatches(dwConvMode);

				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_English", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_ConversionMode_CHS_ToEnglish;
				item->active = g_ConversionMode_CHS_ToEnglish.ConvMatches(dwConvMode);
			}

			return 2;
		}
		}
		return 0;
	}

	int GetIMESentenceModes(SentenceModeItem* dest, int destcount) override
	{
		if (dest)
			memset(dest, 0, destcount * sizeof(SentenceModeItem));

		DWORD dwConvMode = 0, dwSentMode = 0;
		HIMC hImc = ImmGetContext((HWND)GetIMEWindow());

		if (hImc)
		{
			ImmGetConversionStatus(hImc, &dwConvMode, &dwSentMode);
			ImmReleaseContext((HWND)GetIMEWindow(), hImc);
		}

		LanguageIds* info = GetLanguageInfo(LOWORD(GetKeyboardLayout(0)));

		switch (info->languageflag)
		{
		default:
		{
			return 0;
		}

		case JAPANESE:
		{
			if (dest)
			{
				SentenceModeItem* item;

				int i = 0;
				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_General", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_SentenceMode_JP_General;
				item->active = g_SentenceMode_JP_General.SentMatches(dwSentMode);

				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_BiasNames", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_SentenceMode_JP_BiasNames;
				item->active = g_SentenceMode_JP_BiasNames.SentMatches(dwSentMode);

				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_BiasSpeech", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_SentenceMode_JP_BiasSpeech;
				item->active = g_SentenceMode_JP_BiasSpeech.SentMatches(dwSentMode);

				item = &dest[i++];
				wcsncpy(item->menuname, L"#IME_NoConversion", sizeof(item->menuname) / sizeof(wchar_t));
				item->handleValue = (int)&g_SentenceMode_JP_None;
				item->active = g_SentenceMode_JP_None.SentMatches(dwSentMode);
			}

			return 4;
		}
		}

		return 0;
	}

	void OnChangeIMEByHandle(int handleValue) override
	{
		HKL hkl = (HKL)handleValue;
		ActivateKeyboardLayout(hkl, 0);

		int langid = LOWORD(hkl);
		SpewIMEInfo(langid);
	}

	void OnChangeIMEConversionModeByHandle(int handleValue) override
	{
		if (handleValue == 0)
			return;

		IMESettingsTransform* txform = (IMESettingsTransform*)handleValue;
		txform->Apply((HWND)GetIMEWindow());
	}

	void OnChangeIMESentenceModeByHandle(int handleValue) override
	{

	}

	void OnInputLanguageChanged() override
	{

	}

	void OnIMEStartComposition() override
	{
		g_bIMEComposing = true;
		g_flImeComposingTime = gEngfuncs.GetAbsoluteTime();

		gEngfuncs.Con_DPrintf("IME Start Composition\n");
	}

	void OnIMECompositionWin32(long flags) override
	{
		HIMC hIMC = ImmGetContext((HWND)GetIMEWindow());

		if (hIMC)
		{
			if (flags & VGUI_GCS_RESULTSTR)
			{
				wchar_t tempstr[64] = { 0 };
				int len = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, (LPVOID)tempstr, sizeof(tempstr));

				if (len > 0)
				{
					if ((len % 2) != 0)
						len++;

					int numchars = len / sizeof(wchar_t);

					for (int i = 0; i < numchars; ++i)
					{
						PostKeyMessage(new KeyValues("KeyTyped", "unichar", tempstr[i]));
					}
				}

				len = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, (LPVOID)tempstr, sizeof(tempstr));

				if (len == 0)
				{
					InternalSetCompositionString(L"");
					InternalHideCandidateWindow();
					DestroyCandidateList();
				}
			}

			if (flags & VGUI_GCS_COMPSTR)
			{
				wchar_t tempstr[512];
				int len = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, (LPVOID)tempstr, sizeof(tempstr));

				if (len > 0)
				{
					if ((len % 2) != 0)
						len++;

					int numchars = len / sizeof(wchar_t);
					tempstr[numchars] = L'\0';

					InternalSetCompositionString(tempstr);

					DestroyCandidateList();
					CreateNewCandidateListWin32();

					InternalShowCandidateWindow();
				}
			}

			ImmReleaseContext((HWND)GetIMEWindow(), hIMC);
		}
	}

	void OnIMECompositionSDL(const char *text, int start, int length) override
	{
		//gEngfuncs.Con_Printf("OnIMECompositionSDL %s, %d, %d\n", text, start, length);
		//
		//wchar_t wcomposition[256]{};
		//int cb = Q_UTF8ToUnicode(text, wcomposition, sizeof(wcomposition));
		//
		//_imeCompositionStr = wcomposition;
		//
		//InternalSetCompositionString(wcomposition);
		//
		//DestroyCandidateList();
		//CreateNewCandidateListWin32();
		//
		//InternalShowCandidateWindow();
	}

	void OnIMECandidateSDL(const char* const* candidates, int num_candidates, int selected_candidate) override
	{
		//gEngfuncs.Con_Printf("OnIMECandidateSDL %d, %d\n", num_candidates, selected_candidate);

		//_imeCandidateList.resize(num_candidates);

		//for (int i = 0; i < num_candidates; ++i)
		//{
		//	wchar_t wcandidate[256]{};
		//	int cb = Q_UTF8ToUnicode(candidates[i], wcandidate, sizeof(wcandidate));
		//
		//	_imeCandidateList[i] = wcandidate;
		//}
	}

	void DestroyCandidateList(void) override
	{
		if (_imeCandidatesWin32)
		{
			delete[](char*)_imeCandidatesWin32;
			_imeCandidatesWin32 = nullptr;
		}

		//_imeCandidateList.clear();
	}

	void CreateNewCandidateListWin32(void) override
	{
		HIMC hImc = ImmGetContext((HWND)GetIMEWindow());

		if (hImc)
		{
			DWORD buflen = ImmGetCandidateListW(hImc, 0, NULL, 0);

			if (buflen > 0)
			{
				char* buf = new char[buflen];
				Q_memset(buf, 0, buflen);

				CANDIDATELIST* list = (CANDIDATELIST*)buf;
				DWORD copyBytes = ImmGetCandidateListW(hImc, 0, list, buflen);

				if (copyBytes > 0)
				{
					_imeCandidatesWin32 = list;

				}
				else
				{
					delete[] buf;
				}
			}

			ImmReleaseContext((HWND)GetIMEWindow(), hImc);
		}
	}

	void InternalSetCompositionString(const wchar_t* compstr) override
	{
		PostKeyMessage(new KeyValues("DoCompositionString", "string", compstr));
	}

	void InternalShowCandidateWindow(void) override
	{
		PostKeyMessage(new KeyValues("DoShowIMECandidates"));
	}

	void InternalHideCandidateWindow(void) override
	{
		PostKeyMessage(new KeyValues("DoHideIMECandidates"));
	}

	void InternalUpdateCandidateWindow(void) override
	{
		PostKeyMessage(new KeyValues("DoUpdateIMECandidates"));
	}

	void OnIMEEndComposition() override
	{
		if (!g_bIMEComposing)
			return;

		g_bIMEComposing = false;
		g_flImeComposingTime = gEngfuncs.GetAbsoluteTime();

		PostKeyMessage(new KeyValues("DoCompositionString", "string", L""));

		gEngfuncs.Con_DPrintf("IME End Composition\n");
	}

	void OnKeyCodeUnhandled(int keyCode) override
	{


	}

	VPANEL GetModalSubTree(void) override
	{
		return NULL;
	}

	bool ShouldModalSubTreeReceiveMessages() const override
	{
		return false;
	}

	void OnIMEShowCandidates() override
	{
		DestroyCandidateList();
		CreateNewCandidateListWin32();

		InternalShowCandidateWindow();
	}

	void OnIMEChangeCandidates() override
	{
		DestroyCandidateList();
		CreateNewCandidateListWin32();

		InternalUpdateCandidateWindow();
	}

	void OnIMECloseCandidates() override
	{
		InternalHideCandidateWindow();
		DestroyCandidateList();
	}

	void OnIMERecomputeModes() override
	{

	}

	int GetCandidateListCount() override
	{
		//return (int)_imeCandidateList.size();

		if (!_imeCandidatesWin32)
			return 0;

		return (int)_imeCandidatesWin32->dwCount;
	}

	void GetCandidate(int num, wchar_t* dest, int destSizeBytes) override
	{
		dest[0] = 0;
		

		//if (num < 0 || num >= (int)_imeCandidateList.size())
		//{
		//	return;
		//}

		//wcsncpy(dest, _imeCandidateList[num].c_str(), destSizeBytes / sizeof(wchar_t) - 1);
		//dest[destSizeBytes / sizeof(wchar_t) - 1] = L'\0';

		//return;

		if (num < 0 || num >= (int)_imeCandidatesWin32->dwCount)
		{
			return;
		}

		DWORD offset = *(DWORD*)((char*)(_imeCandidatesWin32->dwOffset + num));
		wchar_t* s = (wchar_t*)((char*)_imeCandidatesWin32 + offset);

		wcsncpy(dest, s, destSizeBytes / sizeof(wchar_t) - 1);
		dest[destSizeBytes / sizeof(wchar_t) - 1] = L'\0';
	}

	int GetCandidateListSelectedItem() override
	{
		//return _imeCandidateSelectedItem;

		if (!_imeCandidatesWin32)
			return 0;

		return (int)_imeCandidatesWin32->dwSelection;
	}

	int GetCandidateListPageSize() override
	{
		if (!_imeCandidatesWin32)
			return 0;

		return (int)_imeCandidatesWin32->dwPageSize;
	}

	int GetCandidateListPageStart() override
	{
		if (!_imeCandidatesWin32)
			return 0;

		return (int)_imeCandidatesWin32->dwPageStart;
	}

	void SetCandidateWindowPos(int x, int y) override
	{
		auto hWnd = (HWND)GetIMEWindow();
		if (!hWnd)
			return;

		POINT point;
		CANDIDATEFORM Candidate;

		point.x = x;
		point.y = y;

		HIMC hIMC = ImmGetContext(hWnd);

		if (hIMC)
		{
			Candidate.dwIndex = 0;
			Candidate.dwStyle = CFS_FORCE_POSITION;
			Candidate.ptCurrentPos.x = point.x;
			Candidate.ptCurrentPos.y = point.y;
			ImmSetCandidateWindow(hIMC, &Candidate);

			ImmReleaseContext((HWND)GetIMEWindow(), hIMC);
		}
	}

	bool GetShouldInvertCompositionString() override
	{
		LanguageIds* info = GetLanguageInfo(LOWORD(GetKeyboardLayout(0)));

		if (!info)
			return false;

		return info->invertcomposition;
	}

	bool CandidateListStartsAtOne() override
	{
		DWORD prop = ImmGetProperty(GetKeyboardLayout(0), IGP_PROPERTY);

		if (prop & IME_PROP_CANDLIST_START_FROM_1)
		{
			return true;
		}

		return false;
	}

	void SetCandidateListPageStart(int start) override
	{
		HIMC hImc = ImmGetContext((HWND)GetIMEWindow());

		if (hImc)
		{
			ImmNotifyIME(hImc, NI_COMPOSITIONSTR, 0, start);
			ImmReleaseContext((HWND)GetIMEWindow(), hImc);
		}
	}

	void GetCompositionString(wchar_t* dest, int destSizeBytes) override
	{
		//auto numchars = min(_imeCompositionStr.length(), (destSizeBytes / sizeof(wchar_t)) - 1);
		//wcsncpy(dest, _imeCompositionStr.c_str(), numchars);
		//dest[numchars] = 0;
		//return;

		dest[0] = L'\0';
		HIMC hImc = ImmGetContext((HWND)GetIMEWindow());

		if (hImc)
		{
			int len = ImmGetCompositionStringW(hImc, GCS_COMPSTR, (LPVOID)dest, destSizeBytes);

			if (len > 0)
			{
				if ((len % 2) != 0)
					len++;

				int numchars = len / sizeof(wchar_t);
				dest[numchars] = L'\0';
			}

			ImmReleaseContext((HWND)GetIMEWindow(), hImc);
		}
	}

	void OnIMESelectCandidate(int num) override
	{
		if (num < 1 || num > 9)
			return;

		BYTE nVirtKey = '0' + num;
		
		keybd_event(nVirtKey, 0, 0, 0);
		keybd_event(nVirtKey, 0, KEYEVENTF_KEYUP, 0);
	}

	bool PostKeyMessage(KeyValues* message) override
	{
		return g_pfnCWin32Input_PostKeyMessage(g_pVGuiInput, 0, message);
	}

	bool IsIMEComposing() const override
	{
		return g_bIMEComposing;
	}

	double GetImeComposingTime() const override
	{
		return g_flImeComposingTime;
	}
};

static CInput2 g_Input2;

vgui::IInput2* g_pVGuiInput2 = &g_Input2;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CInput2, IInput2, VGUI_INPUT2_INTERFACE_VERSION, g_Input2);
