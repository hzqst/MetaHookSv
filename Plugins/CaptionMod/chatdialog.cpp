#include <metahook.h>
#include <vgui/ISystem.h>
#include "chatdialog.h"
#include "engfuncs.h"
#include "ViewPort.h"

#define MAX_PLAYER_NAME_LENGTH 128
#define TEXTCOLOR_NORMAL 1
#define TEXTCOLOR_USEOLDCOLORS 2
#define TEXTCOLOR_PLAYERNAME 3
#define TEXTCOLOR_LOCATION 4
#define TEXTCOLOR_MAX 5

float g_ColorDefault[3] = { 1.0, 1.0, 1.0 };

extern cvar_t *hud_saytext_time;

using namespace vgui;

CChatDialogLine::CChatDialogLine(vgui::Panel *parent, const char *panelName) : vgui::RichText(parent, panelName)
{
	m_hFont = m_hFontMarlett = 0;
	m_flExpireTime = 0.0f;
	m_flStartTime = 0.0f;
	m_iNameLength = 0;
	m_text = NULL;

	//SetScheme("ChatScheme");

	SetPaintBackgroundEnabled(true);
	SetVerticalScrollbar(false);
}

CChatDialogLine::~CChatDialogLine(void)
{
	if (m_text)
		delete [] m_text;
}

void CChatDialogLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("ChatFont");
	m_hFontMarlett = pScheme->GetFont("Marlett");
	m_clrText = pScheme->GetColor("FgColor", GetFgColor());

	Color defaultColor = pScheme->GetColor("TanLight", Color(235, 226, 202, 255));

	g_ColorDefault[0] = defaultColor.r() / 255.0f;
	g_ColorDefault[1] = defaultColor.g() / 255.0f;
	g_ColorDefault[2] = defaultColor.b() / 255.0f;

	SetFont(m_hFont);
	SetBgColor(Color(0, 0, 0, 100));
}

void CChatDialogLine::PerformFadeout(void)
{
	float curtime = gEngfuncs.GetAbsoluteTime();

	int lr = m_clrText[0];
	int lg = m_clrText[1];
	int lb = m_clrText[2];

	if (curtime >= m_flStartTime && curtime < m_flStartTime + CHATLINE_FLASH_TIME)
	{
		float frac1 = (curtime - m_flStartTime) / CHATLINE_FLASH_TIME;
		float frac = frac1;

		frac *= CHATLINE_NUM_FLASHES;
		frac *= 2 * M_PI;
		frac = cos(frac);
		frac = clamp(frac, 0.0f, 1.0f);
		frac *= (1.0f - frac1);

		int r = lr, g = lg, b = lb;

		r = r + (255 - r) * frac;
		g = g + (255 - g) * frac;
		b = b + (255 - b) * frac;

		int alpha = 63 + 192 * (1.0f - frac1);
		alpha = clamp(alpha, 0, 255);

		wchar_t wbuf[4096];
		GetText(0, wbuf, sizeof(wbuf));
		SetText("");

		InsertColorChange(Color(r, g, b, 255));
		InsertString(wbuf);
	}
	else if (curtime <= m_flExpireTime && curtime > m_flExpireTime - CHATLINE_FADE_TIME)
	{
		float frac = (m_flExpireTime - curtime) / CHATLINE_FADE_TIME;

		int alpha = frac * 255;
		alpha = clamp(alpha, 0, 255);

		wchar_t wbuf[4096];
		GetText(0, wbuf, sizeof(wbuf));
		SetText("");

		InsertColorChange(Color(lr * frac, lg * frac, lb * frac, alpha));
		InsertString(wbuf);
	}
	else
	{
		wchar_t wbuf[4096];
		GetText(0, wbuf, sizeof(wbuf));
		SetText("");

		InsertColorChange(Color(lr, lg, lb, 255));
		InsertString(wbuf);
	}

	OnThink();
}

void CChatDialogLine::SetExpireTime(void)
{
	m_flStartTime = gEngfuncs.GetAbsoluteTime();
	m_flExpireTime = m_flStartTime + hud_saytext_time->value;
	m_nCount = CChatDialog::m_nLineCounter++;
}

int CChatDialogLine::GetCount(void)
{
	return m_nCount;
}

bool CChatDialogLine::IsReadyToExpire(void)
{
	if (!(gEngfuncs.GetMaxClients() > 1))
		return true;

	if (gEngfuncs.GetAbsoluteTime() >= m_flExpireTime)
		return true;

	return false;
}

float CChatDialogLine::GetStartTime(void)
{
	return m_flStartTime;
}

void CChatDialogLine::Expire(void)
{
	SetVisible(false);
}

CChatDialogInputLine::CChatDialogInputLine(CChatDialog *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetMouseInputEnabled(false);

	m_pPrompt = new vgui::TextEntry(this, "ChatInputPrompt");
	m_pInput = new CChatDialogEntry(this, "ChatInput", parent);
	m_pInput->SetMaximumCharCount(127);
}

void CChatDialogInputLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	vgui::HFont hFont = pScheme->GetFont("ChatFont");

	m_pPrompt->SetFont(hFont);
	m_pInput->SetFont(hFont);
	m_pInput->SetFgColor(pScheme->GetColor("Chat.TypingText", pScheme->GetColor("Panel.FgColor", Color(255, 255, 255, 255))));

	SetPaintBackgroundEnabled( true );
	m_pPrompt->SetPaintBackgroundEnabled( true );

	m_pInput->SetMouseInputEnabled(true);

	SetBgColor(Color(0, 0, 0, 0));
}

void CChatDialogInputLine::SetPrompt(const char *prompt)
{
	Assert(m_pPrompt);

	m_pPrompt->SetText(prompt);

	InvalidateLayout();
}

void CChatDialogInputLine::ClearEntry(void)
{
	Assert(m_pInput);

	SetEntry(L"");
}

void CChatDialogInputLine::SetEntry(const wchar_t *entry)
{
	Assert(m_pInput);
	Assert(entry);

	m_pInput->SetText(entry);
}

void CChatDialogInputLine::GetMessageText(wchar_t *buffer, int buffersizebytes)
{
	m_pInput->GetText(buffer, buffersizebytes);
}

void CChatDialogInputLine::PerformLayout(void)
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize( wide, tall );

	int w,h;
	m_pPrompt->GetSize( w, h); 
	m_pPrompt->SetBounds( 0, 0, w, tall );

	m_pInput->SetBounds( w + 2, 0, wide - w - 2 , tall );
}

vgui::Panel *CChatDialogInputLine::GetInputPanel(void)
{
	return m_pInput;
}

CChatDialogHistory::CChatDialogHistory(vgui::Panel *pParent, const char *panelName) : BaseClass(pParent, "ChatHistory")
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( NULL, "captionmod/ChatScheme.res", "ChatScheme");
	SetScheme(scheme);

	InsertFade(-1, -1);
}

void CChatDialogHistory::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	vgui::HFont font = pScheme->GetFont("ChatHistoryFont");

	if (font == vgui::INVALID_FONT)
		font = pScheme->GetFont("ChatFont");

	SetFont(font);
	SetAlpha(255);
}

void CChatDialogHistory::Paint(void)
{
	BaseClass::Paint();
}

int CChatDialog::m_nLineCounter = 1;

CChatDialog::CChatDialog(void) : BaseClass(NULL, PANEL_CHAT)
{
	//MakePopup();
	SetZPos(-30);

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( NULL, "captionmod/ChatScheme.res", "ChatScheme" );
	SetScheme(scheme);

	SetTitleBarVisible(false);

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_nMessageMode = MM_NONE;
	m_pChatHistory = new CChatDialogHistory(this, "ChatHistory");

	CreateChatLines();
	CreateChatInputLine();
}

void CChatDialog::CreateChatInputLine(void)
{
	m_pChatInput = new CChatDialogInputLine(this, "ChatInputLine");
	m_pChatInput->SetVisible(false);

	if (GetChatHistory())
	{
		GetChatHistory()->SetMaximumCharCount(127 * 100);
		GetChatHistory()->SetVisible(true);
	}
}

void CChatDialog::CreateChatLines(void)
{
	m_ChatLine = new CChatDialogLine(this, "ChatLine");
	m_ChatLine->SetVisible(false);
}

void CChatDialog::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	LoadControlSettings("captionmod/ChatDialog.res", "GAME");

	BaseClass::ApplySchemeSettings(pScheme);

	SetPaintBackgroundType(2);
	SetPaintBorderEnabled(true);
	SetPaintBackgroundEnabled(true);

	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	m_iHistoryAlpha = GetChatHistory()->GetBgColor().a();
	m_iAlpha = CHAT_HISTORY_ALPHA;

	Color cColor = pScheme->GetColor("DullWhite", GetBgColor());

	if (m_iAlpha == 0)
		m_iAlpha = cColor.a();

	SetBgColor(Color(cColor.r(), cColor.g(), cColor.b(), m_iAlpha));

	GetChatHistory()->SetVerticalScrollbar(false);
}

void CChatDialog::Reset(void)
{
	Clear();
}

void CChatDialog::Paint(void)
{
}

CChatDialogHistory *CChatDialog::GetChatHistory(void)
{
	return m_pChatHistory;
}

void CChatDialog::Init(void)
{
}

void CChatDialog::VidInit(void)
{
	Clear();
	SetVisible(true);
	MakeReadyForUse();
	SetPaintBackgroundEnabled(false);

	if (GetChatHistory())
	{
		GetChatHistory()->SetPaintBorderEnabled(false);
		GetChatHistory()->SetBgColor(Color(GetChatHistory()->GetBgColor().r(), GetChatHistory()->GetBgColor().g(), GetChatHistory()->GetBgColor().b(), 0));
		GetChatHistory()->SetMouseInputEnabled(false);
		GetChatHistory()->SetVerticalScrollbar(false);
	}

	m_pChatInput->SetVisible(false);
}

void CChatDialog::Update(void)
{
}

void CChatDialog::ShowPanel(bool bShow)
{
	if (BaseClass::IsVisible() == bShow)
		return;

	if (bShow)
	{
		Activate();
		SetMouseInputEnabled(true);
	}
	else
	{
		SetVisible(false);
		SetMouseInputEnabled(false);
	}
}

int CChatDialog::GetChatInputOffset(void)
{
	return m_iFontHeight;
}

void CChatDialog::OnThink(void)
{
	if (m_ChatLine)
	{
		vgui::HFont font = m_ChatLine->GetFont();
		m_iFontHeight = vgui::surface()->GetFontTall( font ) + 2;

		// Put input area at bottom

		int iChatX, iChatY, iChatW, iChatH;
		int iInputX, iInputY, iInputW, iInputH;
		
		m_pChatInput->GetBounds( iInputX, iInputY, iInputW, iInputH );
		GetBounds( iChatX, iChatY, iChatW, iChatH );

		m_pChatInput->SetBounds( iInputX, iChatH - (m_iFontHeight * 1.75), iInputW, m_iFontHeight );

		//Resize the History Panel so it fits more lines depending on the screen resolution.
		int iChatHistoryX, iChatHistoryY, iChatHistoryW, iChatHistoryH;

		GetChatHistory()->GetBounds( iChatHistoryX, iChatHistoryY, iChatHistoryW, iChatHistoryH );

		iChatHistoryH = (iChatH - (m_iFontHeight * 2.25)) - iChatHistoryY;

		GetChatHistory()->SetBounds( iChatHistoryX, iChatHistoryY, iChatHistoryW, iChatHistoryH );
	}

	SetAlpha(255);

	if (CHAT_HISTORY_FADE_TIME > 0.0)
		FadeChatHistory();
}

#pragma optimize("", off)

int CChatDialog::ComputeBreakChar(int width, const char *text, int textlen)
{
	CChatDialogLine *line = m_ChatLine;
	vgui::HFont font = line->GetFont();

	int currentlen = 0;
	int lastbreak = textlen;

	for (int i = 0; i < textlen; i++)
	{
		char ch = text[i];

		if (ch <= 32)
			lastbreak = i;

		wchar_t wch[2];
		g_pVGuiLocalize->ConvertANSIToUnicode(&ch, wch, sizeof(wch));

		int a, b, c;
		vgui::surface()->GetCharABCwide(font, wch[0], a, b, c);
		currentlen += a + b + c;

		if (currentlen >= width)
		{
			if (lastbreak == textlen)
				lastbreak = max(0, i - 1);

			break;
		}
	}

	if (currentlen >= width)
		return lastbreak;

	return textlen;
}

#pragma warning(push)
#pragma warning(disable: 4748)

void CChatDialog::Printf(const char *fmt, ...)
{
	va_list marker;
	char msg[4096];

	va_start(marker, fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, marker);
	va_end(marker);

	ChatPrintf(0, "%s", msg);
}

void CChatDialog::Printf(const wchar_t *fmt, ...)
{
	va_list marker;
	wchar_t msg[4096];

	va_start(marker, fmt);
	_vsnwprintf(msg, sizeof(msg), fmt, marker);
	va_end(marker);

	ChatPrintf(0, "%s", msg);
}

#pragma warning(pop)
#pragma optimize("", on)

void CChatDialog::StartMessageMode(int iMessageModeType)
{
	if (!IsVisible())
		SetVisible(true);

	m_nMessageMode = iMessageModeType;
	m_pChatInput->ClearEntry();
	m_pChatInput->SetVisible(true);

	if (m_nMessageMode == MM_SAY)
		m_pChatInput->SetPrompt("Say");
	else if (m_nMessageMode == MM_SAY_TEAM)
		m_pChatInput->SetPrompt("Team");
	else
		m_pChatInput->SetPrompt("");

	if (GetChatHistory())
	{
		GetChatHistory()->SetMouseInputEnabled(true);
		GetChatHistory()->SetKeyBoardInputEnabled(false);
		GetChatHistory()->SetVerticalScrollbar(true);
		GetChatHistory()->ResetAllFades(true);
		GetChatHistory()->SetPaintBorderEnabled(true);
		GetChatHistory()->SetVisible(true);
		GetChatHistory()->SetCursor(vgui::dc_arrow);
	}

	vgui::SETUP_PANEL(this);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	vgui::surface()->CalculateMouseVisible();

	m_pChatInput->RequestFocus();
	m_pChatInput->SetPaintBorderEnabled(true);
	m_pChatInput->SetMouseInputEnabled(true);

	/*if (CHAT_HISTORY_FADE_TIME <= 0)
	{
		SetPaintBackgroundEnabled(true);

		if (GetChatHistory())
			GetChatHistory()->SetBgColor(Color(GetChatHistory()->GetBgColor().r(), GetChatHistory()->GetBgColor().g(), GetChatHistory()->GetBgColor().b(), m_iHistoryAlpha));
	}
	else*/
	{
		//Place the mouse cursor near the text so people notice it.
		//int x, y, w, h;
		//GetChatHistory()->GetBounds( x, y, w, h );
		//vgui::input()->SetCursorPos( x + ( w/2), y + (h/2) );
		
		m_flHistoryFadeTime = gEngfuncs.GetAbsoluteTime() + CHAT_HISTORY_FADE_TIME;
	}
}

void CChatDialog::StopMessageMode(void)
{
	m_nMessageMode = MM_NONE;

	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	if (GetChatHistory())
	{
		GetChatHistory()->SetPaintBorderEnabled(false);
		GetChatHistory()->GotoTextEnd();
		GetChatHistory()->SetMouseInputEnabled(false);
		GetChatHistory()->SetVerticalScrollbar(false);
		GetChatHistory()->ResetAllFades(false, true, CHAT_HISTORY_FADE_TIME);
		GetChatHistory()->SelectNoText();
		GetChatHistory()->SetCursor(vgui::dc_none);
	}

	m_pChatInput->ClearEntry();
	m_pChatInput->SetVisible(false);

	/*if (CHAT_HISTORY_FADE_TIME <= 0)
	{
		SetPaintBackgroundEnabled(false);

		if (GetChatHistory())
			GetChatHistory()->SetBgColor(Color(GetChatHistory()->GetBgColor().r(), GetChatHistory()->GetBgColor().g(), GetChatHistory()->GetBgColor().b(), 0));
	}
	else*/
	{
		m_flHistoryFadeTime = gEngfuncs.GetAbsoluteTime() + CHAT_HISTORY_FADE_TIME;
	}
}

void CChatDialog::FadeChatHistory(void)
{
	float frac = (m_flHistoryFadeTime - gEngfuncs.GetAbsoluteTime()) / CHAT_HISTORY_FADE_TIME;

	int alpha = frac * m_iAlpha;
	alpha = clamp(alpha, 0, m_iAlpha);

	if (alpha >= 0)
	{
		if (GetChatHistory())
		{
			if (IsMouseInputEnabled())
			{
				SetPaintBackgroundEnabled(true);
				GetChatHistory()->SetBgColor(Color(GetChatHistory()->GetBgColor().r(), GetChatHistory()->GetBgColor().g(), GetChatHistory()->GetBgColor().b(), m_iAlpha - alpha));

				m_pChatInput->GetPrompt()->SetBgColor(Color(m_pChatInput->GetPrompt()->GetBgColor().r(), m_pChatInput->GetPrompt()->GetBgColor().g(), m_pChatInput->GetPrompt()->GetBgColor().b(), m_iAlpha - alpha));
				m_pChatInput->GetInputPanel()->SetBgColor(Color(m_pChatInput->GetInputPanel()->GetBgColor().r(), m_pChatInput->GetInputPanel()->GetBgColor().g(), m_pChatInput->GetInputPanel()->GetBgColor().b(), m_iAlpha - alpha));
				m_pChatInput->GetPrompt()->SetAlpha((m_iAlpha * 2) - alpha);
				m_pChatInput->GetInputPanel()->SetAlpha((m_iAlpha * 2) - alpha);

				SetBgColor(Color(GetBgColor().r(), GetBgColor().g(), GetBgColor().b(), m_iAlpha - alpha));
			}
			else
			{
				SetPaintBackgroundEnabled(false);
				GetChatHistory()->SetBgColor(Color(GetChatHistory()->GetBgColor().r(), GetChatHistory()->GetBgColor().g(), GetChatHistory()->GetBgColor().b(), alpha));
				SetBgColor(Color(GetBgColor().r(), GetBgColor().g(), GetBgColor().b(), alpha));

				m_pChatInput->GetPrompt()->SetBgColor(Color(m_pChatInput->GetPrompt()->GetBgColor().r(), m_pChatInput->GetPrompt()->GetBgColor().g(), m_pChatInput->GetPrompt()->GetBgColor().b(), alpha));
				m_pChatInput->GetInputPanel()->SetBgColor(Color(m_pChatInput->GetInputPanel()->GetBgColor().r(), m_pChatInput->GetInputPanel()->GetBgColor().g(), m_pChatInput->GetInputPanel()->GetBgColor().b(), alpha));
				m_pChatInput->GetPrompt()->SetAlpha(alpha);
				m_pChatInput->GetInputPanel()->SetAlpha(alpha);
			}
		}
	}
}

float *GetTextColor(int colorNum, int clientIndex)
{
	switch (colorNum)
	{
	case TEXTCOLOR_USEOLDCOLORS:
	{
		if (gCapFuncs.GetClientColor)
		{
			return gCapFuncs.GetClientColor(-1);
		}
		break;
	}
	case TEXTCOLOR_PLAYERNAME:
	{
		if (gCapFuncs.GetClientColor)
		{
			return gCapFuncs.GetClientColor(clientIndex);
		}
		break;
	}

	}

	return NULL;
}

Color CChatDialog::GetTextColorForClient(int colorNum, int clientIndex)
{
	float *col = ::GetTextColor(colorNum, clientIndex);

	if (!col)
		col = g_ColorDefault;

	return Color(col[0] * 255, col[1] * 255, col[2] * 255, 255);
}

Color CChatDialog::GetClientColor(int clientIndex)
{
	float *col = g_ColorDefault;

	if (gCapFuncs.GetClientColor)
	{
		col = gCapFuncs.GetClientColor(clientIndex);
	}

	return Color(col[0] * 255, col[1] * 255, col[2] * 255, 255);
}

inline wchar_t *CloneWString(const wchar_t *str)
{
	wchar_t *cloneStr = new wchar_t[wcslen(str) + 1];
	wcscpy(cloneStr, str);
	return cloneStr;
}

void CChatDialogLine::InsertAndColorizeText(wchar_t *buf, int clientIndex)
{
	if (m_text)
	{
		delete [] m_text;
		m_text = NULL;
	}

	m_textRanges.RemoveAll();
	m_text = CloneWString(buf);

	CChatDialog *pChat = dynamic_cast<CChatDialog *>(GetParent());

	if (pChat == NULL)
		return;

	wchar_t *txt = m_text;
	int lineLen = wcslen(m_text);

	if (m_text[0] == TEXTCOLOR_USEOLDCOLORS || m_text[0] == TEXTCOLOR_PLAYERNAME || m_text[0] == TEXTCOLOR_LOCATION || m_text[0] == TEXTCOLOR_NORMAL)
	{
		while (txt && *txt)
		{
			TextRange_VGUI range;

			switch (*txt)
			{
				case TEXTCOLOR_USEOLDCOLORS:
				case TEXTCOLOR_PLAYERNAME:
				case TEXTCOLOR_LOCATION:
				case TEXTCOLOR_NORMAL:
				{
					range.start = (txt - m_text) + 1;
					range.color = pChat->GetTextColorForClient((int)(*txt), clientIndex);
					range.end = lineLen;

					int count = m_textRanges.Count();

					if (count)
						m_textRanges[count - 1].end = range.start - 1;

					m_textRanges.AddToTail(range);

					++txt;
					break;
				}

				default: ++txt;
			}
		}
	}

	if (!m_textRanges.Count() && m_iNameLength > 0 && m_text[0] == TEXTCOLOR_USEOLDCOLORS)
	{
		TextRange_VGUI range;
		range.start = 0;
		range.end = m_iNameStart;
		range.color = pChat->GetTextColorForClient(TEXTCOLOR_NORMAL, clientIndex);
		m_textRanges.AddToTail(range);

		range.start = m_iNameStart;
		range.end = m_iNameStart + m_iNameLength;
		range.color = pChat->GetTextColorForClient(TEXTCOLOR_PLAYERNAME, clientIndex);
		m_textRanges.AddToTail(range);

		range.start = range.end;
		range.end = wcslen(m_text);
		range.color = pChat->GetTextColorForClient(TEXTCOLOR_NORMAL, clientIndex);
		m_textRanges.AddToTail(range);
	}

	if (!m_textRanges.Count())
	{
		TextRange_VGUI range;
		range.start = 0;
		range.end = wcslen(m_text);
		range.color = pChat->GetTextColorForClient(TEXTCOLOR_NORMAL, clientIndex);
		m_textRanges.AddToTail(range);
	}

	for (int i = 0; i < m_textRanges.Count(); ++i)
	{
		wchar_t *start = m_text + m_textRanges[i].start;
		Color color = pChat->GetTextColorForClient((int)(*start), clientIndex);

		if (color == Color(0, 0, 0, 0))
			m_textRanges[i].start += 1;
	}

	Colorize();
}

void CChatDialogLine::Colorize(int alpha)
{
	CChatDialog *pChat = dynamic_cast<CChatDialog *>(GetParent());

	if (pChat && pChat->GetChatHistory())
		pChat->GetChatHistory()->InsertString("\n");

	wchar_t wText[4096];
	Color color;

	for (int i = 0; i < m_textRanges.Count(); ++i)
	{
		wchar_t * start = m_text + m_textRanges[i].start;
		int len = m_textRanges[i].end - m_textRanges[i].start + 1;

		if (len > 1)
		{
			wcsncpy(wText, start, len);
			wText[len - 1] = 0;
			color = m_textRanges[i].color;
			color[3] = alpha;
			InsertColorChange(color);
			InsertString(wText);

			CChatDialog *pChat = dynamic_cast<CChatDialog *>(GetParent());

			if (pChat && pChat->GetChatHistory())
			{
				pChat->GetChatHistory()->InsertColorChange(color);
				pChat->GetChatHistory()->InsertString(wText);
				pChat->GetChatHistory()->InsertFade(hud_saytext_time->value, CHAT_HISTORY_IDLE_FADE_TIME);

				if (i == m_textRanges.Count() - 1)
					pChat->GetChatHistory()->InsertFade(-1, -1);
			}
		}
	}

	InvalidateLayout(true);
}

CChatDialogLine *CChatDialog::FindUnusedChatLine(void)
{
	return m_ChatLine;
}

void CChatDialog::Send(void)
{
	wchar_t szTextbuf[128];
	m_pChatInput->GetMessageText(szTextbuf, sizeof(szTextbuf));

	char ansi[128];
	g_pVGuiLocalize->ConvertUnicodeToANSI(szTextbuf, ansi, sizeof(ansi));

	int len = Q_strlen(ansi);

	if (len > 0 && ansi[len - 1] == '\n')
		ansi[len - 1] = '\0';

	if (len > 0)
	{
		char szbuf[144];

		switch (m_nMessageMode)
		{
			case MM_SAY:
			{
				Q_snprintf(szbuf, sizeof(szbuf), "say \"%s\"", ansi);
				gEngfuncs.pfnClientCmd(szbuf);
				break;
			}

			case MM_SAY_TEAM:
			{
				Q_snprintf(szbuf, sizeof(szbuf), "say_team \"%s\"", ansi);
				gEngfuncs.pfnClientCmd(szbuf);
				break;
			}
		}
	}

	m_pChatInput->ClearEntry();
}

vgui::Panel *CChatDialog::GetInputPanel(void)
{
	return m_pChatInput->GetInputPanel();
}

void CChatDialog::Clear(void)
{
	m_pChatInput->ClearEntry();
	//m_pChatHistory->Clear(); // WHAT
}

void CChatDialog::ChatPrintf(int iPlayerIndex, const char *fmt, ...)
{
	va_list marker;
	char msg[4096];

	va_start(marker, fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, marker);
	va_end(marker);

	if (strlen(msg) > 0 && msg[strlen(msg) - 1] == '\n')
		msg[strlen(msg) - 1] = 0;

	char *pmsg = msg;
	Color color = GetTextColorForClient((int)(*pmsg), iPlayerIndex);

	while (*pmsg && (*pmsg == '\n' || ( *pmsg > 0 && *pmsg < TEXTCOLOR_MAX ) ))
		pmsg++;

	if (!*pmsg)
		return;

	pmsg = msg;

	while (*pmsg && (*pmsg == '\n'))
		pmsg++;

	if (!*pmsg)
		return;

	CChatDialogLine *line = (CChatDialogLine *)FindUnusedChatLine();

	if (!line)
		line = (CChatDialogLine *)FindUnusedChatLine();

	if (!line)
		return;

	line->SetText("");

	int iNameStart = 0;
	int iNameLength = 0;

	hud_player_info_t sPlayerInfo = {0};

	if (iPlayerIndex == 0)
	{
		Q_memset(&sPlayerInfo, 0, sizeof(hud_player_info_t));
		sPlayerInfo.name = "Console";
	}
	else
	{
		gEngfuncs.pfnGetPlayerInfo(iPlayerIndex, &sPlayerInfo);
	}

	int bufSize = (strlen(pmsg) + 1) * sizeof(wchar_t);
	wchar_t *wbuf = static_cast<wchar_t *>(_alloca(bufSize));

	if (wbuf)
	{
		line->SetExpireTime();
		g_pVGuiLocalize->ConvertANSIToUnicode(pmsg, wbuf, bufSize);

		if (sPlayerInfo.name)
		{
			wchar_t wideName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode(sPlayerInfo.name, wideName, sizeof(wideName));

			const wchar_t *nameInString = wcsstr(wbuf, wideName);

			if (nameInString)
			{
				iNameStart = (nameInString - wbuf);
				iNameLength = wcslen(wideName);
			}
		}

		Color clrNameColor = GetClientColor(iPlayerIndex);

		line->SetVisible(false);
		line->SetNameStart(iNameStart);
		line->SetNameLength(iNameLength);
		line->SetNameColor(clrNameColor);
		line->InsertAndColorizeText(wbuf, iPlayerIndex);
	}

	SetVisible(true);
}

void CChatDialog::ChatPrintf(int iPlayerIndex, const wchar_t *fmt, ...)
{
	va_list marker;
	wchar_t msg[4096];

	va_start(marker, fmt);
	_vsnwprintf(msg, sizeof(msg), fmt, marker);
	va_end(marker);

	if (wcslen(msg) > 0 && msg[wcslen(msg) - 1] == '\n')
		msg[wcslen(msg) - 1] = 0;

	wchar_t *pmsg = msg;
	Color color = GetTextColorForClient((int)(*pmsg), iPlayerIndex);

	while ( *pmsg && ( *pmsg == '\n' || ( *pmsg > 0 && *pmsg < TEXTCOLOR_MAX ) ) )
	{
		pmsg++;
	}
	
	if (!*pmsg)
		return;

	pmsg = msg;

	while (*pmsg && (*pmsg == L'\n'))
		pmsg++;

	if (!*pmsg)
		return;

	CChatDialogLine *line = (CChatDialogLine *)FindUnusedChatLine();

	if (!line)
		line = (CChatDialogLine *)FindUnusedChatLine();

	if (!line)
		return;

	line->SetText("");

	int iNameStart = 0;
	int iNameLength = 0;

	hud_player_info_t sPlayerInfo = {0};

	if (iPlayerIndex == 0)
	{
		Q_memset(&sPlayerInfo, 0, sizeof(hud_player_info_t));
		sPlayerInfo.name = "Console";
	}
	else
	{
		gEngfuncs.pfnGetPlayerInfo(iPlayerIndex, &sPlayerInfo);
	}

	line->SetExpireTime();

	if (sPlayerInfo.name)
	{
		wchar_t wideName[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode(sPlayerInfo.name, wideName, sizeof(wideName));

		const wchar_t *nameInString = wcsstr(msg, wideName);

		if (nameInString)
		{
			iNameStart = (nameInString - msg);
			iNameLength = wcslen(wideName);
		}
	}

	Color clrNameColor = GetClientColor(iPlayerIndex);

	line->SetVisible(false);
	line->SetNameStart(iNameStart);
	line->SetNameLength(iNameLength);
	line->SetNameColor(clrNameColor);
	line->InsertAndColorizeText(msg, iPlayerIndex);

	SetVisible(true);
}