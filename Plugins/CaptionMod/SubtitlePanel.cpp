#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <FileSystem.h>

//#include <glew.h>
#include <triangleapi.h>

#include "SubtitlePanel.h"
#include "privatefuncs.h"

extern cvar_t* cap_subtitle_prefix;
extern cvar_t* cap_subtitle_waitplay;
extern cvar_t* cap_subtitle_antispam;
extern cvar_t* cap_subtitle_fadein;
extern cvar_t* cap_subtitle_fadeout;
extern cvar_t* cap_subtitle_holdtime;
extern cvar_t* cap_subtitle_stimescale;
extern cvar_t* cap_subtitle_htimescale;
extern cvar_t* cap_subtitle_extraholdtime;
using namespace vgui;

SubtitlePanel::SubtitlePanel(Panel *parent)  : EditablePanel(parent, "Subtitle")
{
	m_bInLevel = false;
	//m_flFadeIn = 0;
	//m_flFadeOut = 0;
	//m_flHoldTime = 0;
	//m_flHoldTimeScale = 0;
	//m_flStartTimeScale = 0;
	//m_iAntiSpam = 0;
	//m_iPrefix = 0;
	//m_iWaitPlay = 0;
	m_hTextFont = NULL;
	m_iCornorSize = 0;
	m_flCurPanelY = 0;
	m_flCurPanelYEnd = 0;
	m_iFontTall = 0;
	m_iLineSpace = 0;
	m_iMaxLines = 0;
	m_iPanelAlpha = 0;
	m_iPanelTop = 0;
	m_iPanelY = 0;
	m_iPanelYEnd = 0;
	m_iRoundCornorMaterial[0] = 0;
	m_iRoundCornorMaterial[1] = 0;
	m_iRoundCornorMaterial[2] = 0;
	m_iRoundCornorMaterial[3] = 0;
	m_iScaledCornorSize = 0;
	m_iScaledLineSpace = 0;
	m_iScaledXSpace = 0;
	m_iScaledYSpace = 0;
	m_iTextAlign = ALIGN_DEFAULT;
	m_iXSpace = 0;
	m_iYSpace = 0;
	m_szTextAlign[0] = 0;
	m_szTextFont[0] = 0;

	SetPaintBackgroundEnabled(true);
	SetPaintBorderEnabled(false);
	SetProportional(true);

	SetScheme2("CaptionScheme");

	LoadControlSettings("captionmod/SubtitlePanel.res");
	
	InvalidateLayout(false, true);

	vgui::ivgui()->AddTickSignal( GetVPanel() );
};

SubtitlePanel::~SubtitlePanel()
{
	ClearSubtitle();

	for (int i = 0; i < 4; ++i)
	{
		if (m_iRoundCornorMaterial[i])
		{
			surface()->DeleteTextureByID(m_iRoundCornorMaterial[i]);
			m_iRoundCornorMaterial[i] = 0;
		}
	}
}

void SubtitlePanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const char *font = inResourceData->GetString("textfont");
	Q_strncpy(m_szTextFont, (font) ? font : "Default", sizeof(m_szTextFont));

	m_iTextAlign = ALIGN_DEFAULT;
	const char *align = inResourceData->GetString("textalign");
	if(align[0] == 'r' || align[0] == 'R')
		m_iTextAlign = ALIGN_RIGHT;
	else if(align[0] == 'c' || align[0] == 'C')
		m_iTextAlign = ALIGN_CENTER;
	else if(align[0] == 'l' || align[0] == 'L')
		m_iTextAlign = ALIGN_LEFT;
}

void SubtitlePanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_iRoundCornorMaterial[0] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iRoundCornorMaterial[0], "captionmod/materials/round_corner_nw", true, false);
	
	m_iRoundCornorMaterial[1] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iRoundCornorMaterial[1], "captionmod/materials/round_corner_ne", true, false);

	m_iRoundCornorMaterial[2] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iRoundCornorMaterial[2], "captionmod/materials/round_corner_se", true, false);

	m_iRoundCornorMaterial[3] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iRoundCornorMaterial[3], "captionmod/materials/round_corner_sw", true, false);

	m_iScaledCornorSize = scheme()->GetProportionalScaledValue(m_iCornorSize);
	m_iScaledXSpace = scheme()->GetProportionalScaledValue(m_iXSpace);
	m_iScaledYSpace = scheme()->GetProportionalScaledValue(m_iYSpace);
	m_iScaledLineSpace = scheme()->GetProportionalScaledValue(m_iLineSpace);

	m_hTextFont = pScheme->GetFont(m_szTextFont, true);
	if(!m_hTextFont) 
		m_hTextFont = pScheme->GetFont("Default", true);

	m_iFontTall = surface()->GetFontTall(m_hTextFont);
	if(!m_iFontTall)
		m_iFontTall = 14;

	m_iMaxLines = 1;
	int iTextTall = GetTall() - (m_iScaledYSpace << 1);
	while( (m_iMaxLines - 1) * m_iScaledLineSpace + m_iMaxLines * m_iFontTall <= iTextTall)
	{
		m_iMaxLines ++;
	}

	m_iMaxLines = max(1, m_iMaxLines-1);
	m_iPanelTop = iTextTall - (m_iMaxLines - 1) * m_iScaledLineSpace - m_iMaxLines * m_iFontTall;

	m_flCurPanelY = 0;
	m_flCurPanelYEnd = GetTall();
	m_iPanelY = 9999;
	m_iPanelYEnd = -9999;
}

bool CAnimMovement::Update(void)
{
	if (IsDonePlay())
	{
		m_pLine->m_YPos = m_EndValue;
		return false;
	}
	else if(IsPlaying())
	{
		if(!m_Started)
		{
			m_Started = true;
			m_StartValue = m_pLine->m_YPos;
		}
		float frac = min((g_pViewPort->GetCurTime() - m_StartTime) / m_AnimTime, 1);
		m_pLine->m_YPos = (int)(frac * m_EndValue + (1 - frac) * m_StartValue);
	}
	return true;
}

bool CAnimAlphaFade::Update(void)
{
	if (IsDonePlay())
	{
		m_pLine->m_Alpha = m_EndValue;
		return false;
	}
	else if(IsPlaying())
	{
		if(!m_Started)
		{
			m_Started = true;
			m_StartValue = m_pLine->m_Alpha;
		}
		float frac = min((g_pViewPort->GetCurTime() - m_StartTime) / m_AnimTime, 1);
		m_pLine->m_Alpha = (int)(frac * m_EndValue + (1 - frac) * m_StartValue);
	}
	return true;
}

float CSubLine::GetYPosOutRate(void)
{
	if(m_YPos < m_Panel->m_iPanelTop + m_Panel->m_iScaledYSpace)
	{
		float frac = (float)(m_Panel->m_iPanelTop + m_Panel->m_iScaledYSpace - m_YPos) / m_Panel->m_iFontTall;
		return min(frac, 1);
	}
	return 0;
}

bool CSubLine::ShouldStart(void)
{
	return (g_pViewPort->GetCurTime() > m_StartTime);
}

bool CSubLine::ShouldRetire(void)
{
	return (g_pViewPort->GetCurTime() > m_EndTime || m_LineIndex > m_Panel->m_iMaxLines);
}

bool CSubLine::Update(void)
{
	for (auto it = m_AnimList.begin(); it != m_AnimList.end(); )
	{
		const auto& Temp = (*it);

		if(false == Temp->Update())
		{
			it = m_AnimList.erase(it);
		}
		else
		{
			++it;
		}
	}

	if (!m_Retired && ShouldRetire())
	{
		m_Retired = true;
		AlphaFade(0, m_FadeOut);
	}
	
	if (m_Retired && 0 == m_Alpha)
	{
		return false;//It dies when it goes 0 alpha 
	}

	return true;
}

void CSubLine::MoveTo(int ToPos, float Time)
{
	//Remove any movement animation before add a new one
	for(auto it = m_AnimList.begin(); it != m_AnimList.end(); )
	{
		const auto& Temp = (*it);

		if(Temp->GetType() == ANIM_MOVEMENT)
		{
			it = m_AnimList.erase(it);
		}
		else
		{
			++it;
		}
	}
	auto NewAnim = std::make_shared<CAnimMovement>(g_pViewPort->GetCurTime(), Time, ToPos, this);
	m_AnimList.push_back(NewAnim);
}

void CSubLine::AlphaFade(int Alpha, float Time)
{
	auto NewAnim = std::make_shared<CAnimAlphaFade>(g_pViewPort->GetCurTime(), Time, Alpha, this);
	m_AnimList.push_back(NewAnim);
}

int CSubLine::CalcYPos(void)
{
	return m_Panel->GetTall() - m_Panel->m_iScaledYSpace - m_Panel->m_iFontTall * (m_LineIndex + 1) - m_Panel->m_iScaledLineSpace * m_LineIndex;
}

void CSubLine::Draw(int x, int w, int align)
{
	int nAlpha = m_Color.a() * m_Alpha / 255;
	int nTextAlign = ALIGN_DEFAULT;

	float flFraction = GetYPosOutRate();

	if(flFraction != 0)
	{
		nAlpha = (float)nAlpha * (1 - flFraction);
	}

	if(m_TextAlign)
		nTextAlign = m_TextAlign;
	else if(align)
		nTextAlign = align;

	surface()->DrawSetTextColor(m_Color.r(), m_Color.g(), m_Color.b(), nAlpha);
	if(nTextAlign == ALIGN_CENTER)
	{
		surface()->DrawSetTextPos(x + ((w - m_TextWide) >> 1), m_YPos);
	}
	else if(nTextAlign == ALIGN_RIGHT)
	{
		surface()->DrawSetTextPos(x + (w - m_TextWide), m_YPos);
	}
	else
	{
		surface()->DrawSetTextPos(x, m_YPos);
	}
	surface()->DrawPrintText(&m_Sentence[0], m_Length);
	surface()->DrawFlushText();
}

void SubtitlePanel::StartNextSubtitle(const std::shared_ptr<CDictionary>& dict, const CStartSubtitleContext* pStartSubtitleContext)
{
	//Check if there is a next dict to be played
	auto pNextDict = dict->m_pNext.lock();

	if (pNextDict)
	{
		StartSubtitle(pNextDict, dict->m_flDuration, g_pViewPort->GetCurTime() + dict->m_flNextDelay, pStartSubtitleContext);
	}
}

void SubtitlePanel::OnTick( void )
{
	//Start backed lines if they are ready to start
	for(auto it = m_BackLines.begin(); it != m_BackLines.end(); )
	{
		auto& Temp = (*it);

		if(Temp->ShouldStart())
		{
			StartLine(Temp);
			it = m_BackLines.erase(it);
		}
		else
		{
			++it;
		}
	}

	for(auto it = m_Lines.begin(); it != m_Lines.end(); )
	{
		const auto& Temp = (*it);

		if (false == Temp->Update())
		{
			it = m_Lines.erase(it);
		}
		else
		{
			++it;
		}
	}

	if (gEngfuncs.GetMaxClients() > 1)
	{
		const char *pLevel = gEngfuncs.pfnGetLevelName();
		if (m_bInLevel && !pLevel[0])
		{
			m_bInLevel = false;
			ClearSubtitle();
		}
		else if (!m_bInLevel && pLevel[0])
		{
			m_bInLevel = true;
		}
	}
}

void SubtitlePanel::StartLine(const std::shared_ptr<CSubLine> &Line)
{
	//Notice that the current line must disappear after any other existing lines
	//So we need the latest endtime of the current playing lines.

	float latestEndTime = 0;
	for(auto it = m_Lines.begin(); it != m_Lines.end(); it++)
	{
		const auto& Temp = (*it);

		//Move all existing lines up
		Temp->m_LineIndex ++;
		Temp->MoveTo(Temp->CalcYPos(), cap_subtitle_fadein->value);

		if(Temp->m_EndTime > latestEndTime)
			latestEndTime = Temp->m_EndTime;
	}

	//Add to the current playing line
	m_Lines.push_back(Line);

	if (Line->m_StartTime == 0)
	{
		Line->m_StartTime = g_pViewPort->GetCurTime();
	}

	//Give it the latest endtime
	Line->m_EndTime = max(Line->m_StartTime + Line->m_Duration, latestEndTime);

	//Fade it now
	Line->AlphaFade(255, cap_subtitle_fadein->value);

	//Do we really need sender name here?
	CStartSubtitleContext StartSubtitleContext;

	if (Line->m_bHasSenderName)
	{
		StartSubtitleContext.m_pszSenderName = Line->m_SenderName.c_str();
	}

	StartNextSubtitle(Line->m_Dict, &StartSubtitleContext);
}

std::shared_ptr<CSubLine> SubtitlePanel::AddLine(const std::shared_ptr<CDictionary>& dict, const CStartSubtitleContext* pStartSubtitleContext, const wchar_t * wszSentence, int nLength, float flStartTime, float flDuration, int nTextWide)
{
	std::shared_ptr<CSubLine> Line = std::make_shared<CSubLine>(this, dict, wszSentence, nLength);

	Line->m_TextWide = nTextWide;
	Line->m_StartTime = flStartTime;
	Line->m_Duration = flDuration;
	Line->m_Color = dict->m_Color;

	if (dict->m_bDefaultColor && pStartSubtitleContext->m_pCurrentTextMessage)
	{
		Line->m_Color = Color(
			pStartSubtitleContext->m_pCurrentTextMessage->r1, 
			pStartSubtitleContext->m_pCurrentTextMessage->g1, 
			pStartSubtitleContext->m_pCurrentTextMessage->b1, 
			pStartSubtitleContext->m_pCurrentTextMessage->a1);
	}

	Line->m_Alpha = 0;
	Line->m_LineIndex = 0;
	Line->m_YPos = Line->CalcYPos();
	Line->m_FadeOut = cap_subtitle_fadeout->value;
	Line->m_TextAlign = dict->m_iTextAlign ? dict->m_iTextAlign : m_iTextAlign;

	if (pStartSubtitleContext->m_pszSenderName)
	{
		Line->m_bHasSenderName = true;
		Line->m_SenderName = pStartSubtitleContext->m_pszSenderName;
	}

	m_BackLines.push_back(Line);

	return Line;
}

static bool IsNonBreakableCharacter(wchar_t ch)
{
	return ((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z') || ch == L'Ç' || ch == L'ç' || ch == L'Ğ' || ch == L'ğ' || ch == L'İ' || ch == L'ı' || ch == L'Ö' || ch == L'ö' || ch == L'Ş' || ch == L'ş' || ch == L'Ü' || ch == L'ü' || ch == L'â' || ch == L'Â' || ch == L':' || ch == L'-' || ch == L'\'' || ch == L'"' || ch == L',' || ch == L'.' || ch == L'!' || ch == L'?' || ch == L';' || ch == '%' || (ch >= L'0' && ch <= L'9')) ? true : false;
}

//2015-11-26 added htimescale for SubtitlePanel
void SubtitlePanel::StartSubtitle(const std::shared_ptr<CDictionary>& dict, float flDurationTime, float flStartTime, const CStartSubtitleContext* pStartSubtitleContext)
{
	//Delay the current line till the last backline plays
	float flLatestStart = 0;
	for(auto it = m_BackLines.begin(); it != m_BackLines.end(); it++)
	{
		const auto& Temp = (*it);

		if (cap_subtitle_waitplay->value >= 1.0f)
		{
			Temp->m_StartTime = 0;
		}
		else
		{
			if (Temp->m_StartTime > flLatestStart)
				flLatestStart = Temp->m_StartTime;
		}

		//Already in list, do not start one subtitle for twice at the same time.
		if(Temp->m_Dict == dict)
			return;
	}

	if (cap_subtitle_antispam->value >= 1.0f)
	{
		for (auto it = m_Lines.begin(); it != m_Lines.end(); it++)
		{
			const auto& Temp = (*it);

			//Already in display, ignore
			if (Temp->m_Dict == dict)
				return;
		}
	}

	std::wstring speakerName;
	std::wstring fullSentence;

	dict->ProcessString(dict->m_szSentence, pStartSubtitleContext, fullSentence);

	if (cap_subtitle_prefix->value >= 1.0f)
	{
		dict->ProcessString(dict->m_szSpeaker, pStartSubtitleContext, speakerName);

		fullSentence = speakerName + fullSentence;
	}

	int iPanelWidth = GetWide();
	int iMaxTextWidth = iPanelWidth - (m_iScaledXSpace << 1);

	wchar_t szBuf[4096];
	wchar_t *pStart = &fullSentence[0];
	wchar_t *p = pStart;

	if(!pStart[0])
		return;

	int nCharNum, nWide, nTall;
	int nTotalCharNum;
	int nAddedCharNum;

	nCharNum = 0;
	nAddedCharNum = 0;
	nTotalCharNum = 0;

	//Count the number of chars, skip CRLF
	while(*p++)
	{
		if(*p != '\r' && *p != '\n')
			nTotalCharNum ++;
	}

	//wtf no chars in this sentence?
	if(!nTotalCharNum)
		return;

	float flDuration = flDurationTime;

	//Fallback to m_flHoldTime
	if(flDuration <= 0)
		flDuration = cap_subtitle_holdtime->value;

	if(flDuration <= 0)
		flDuration = 4.0f;

	if (!dict->m_bOverrideDuration && pStartSubtitleContext->m_pCurrentTextMessage)
	{
		if (pStartSubtitleContext->m_pCurrentTextMessage->effect == 2 &&
			pStartSubtitleContext->m_pCurrentTextMessage->pMessage)
		{
			flDuration = (pStartSubtitleContext->m_pCurrentTextMessage->fadein * fullSentence.length() ) +
				pStartSubtitleContext->m_pCurrentTextMessage->fadeout + pStartSubtitleContext->m_pCurrentTextMessage->holdtime;
		}
		else
		{
			flDuration = pStartSubtitleContext->m_pCurrentTextMessage->holdtime +
				pStartSubtitleContext->m_pCurrentTextMessage->fadein +
				pStartSubtitleContext->m_pCurrentTextMessage->fadeout;
		}
	}

	if(cap_subtitle_htimescale->value > 0)
		flDuration *= cap_subtitle_htimescale->value;

	p = pStart;

	std::shared_ptr<CSubLine> pAddedLine;

	while(*pStart)
	{
		//fetch one line from this sentence
		nCharNum = 0;
		nWide = 0;

		int nLastWide = 0;
		wchar_t *LastP = nullptr;
		int nLastCharNum = 0;

		while(1)
		{
			nLastWide = nWide;
			LastP = p;
			nLastCharNum = nCharNum;

			//Leave at least 2 chars, one for (*p) and another for '\0'
			if (nCharNum + 2 >= _ARRAYSIZE(szBuf) - 1)
				break;

			//Make sure English words, numbers, punctuations, and certain additional characters don't break in half...
			if(IsNonBreakableCharacter(*p))
			{
				while (IsNonBreakableCharacter(*p))
				{
					//Leave at least 2 chars, one for (*p) and another for '\0'
					if (nCharNum + 2 >= _ARRAYSIZE(szBuf) - 1)
						break;

					szBuf[nCharNum] = *p++;
					nCharNum++;
				}
				szBuf[nCharNum] = L'\0';
			}
			else
			{
				szBuf[nCharNum++] = *p++;
				szBuf[nCharNum] = L'\0';
			}

			surface()->GetTextSize(m_hTextFont, szBuf, nWide, nTall);

			//It's out of range? Go back to last position and make it a new line
			if(nWide > iMaxTextWidth)
			{
				nWide = nLastWide;
				nCharNum = nLastCharNum;
				p = LastP;
				nAddedCharNum += nCharNum;
				break;
			}

			//Force to be a new line
			if ((*p) == L'\0' || (*p) == L'\n' || (*p) == L'\r')
			{
				nAddedCharNum += nCharNum;
				break;
			}
		}

		if (nCharNum == 0)
			break;

		//Calculate the duration and start time
		float flPercentDuration = (float)nCharNum / nTotalCharNum;
		float flPercentStart = (float)(nAddedCharNum - nCharNum) / nTotalCharNum;

		//Calculate the StartTime
		float flCalcStartTime = flPercentStart * flDuration;
		float flRealStartTime;

		if(cap_subtitle_stimescale->value <= 0)
			flRealStartTime = flStartTime;
		else
			flRealStartTime = flStartTime + flCalcStartTime * cap_subtitle_stimescale->value;

		//Shall we wait for the latest backlines played?
		if(cap_subtitle_waitplay->value >= 1.0f)
			flRealStartTime = max(flRealStartTime, flLatestStart);

		//Calculate the Duration
		float flCalcDuration = flDuration * flPercentDuration;
		float flRealDuration;
		
		//Longer start time won't change the duration
		if(cap_subtitle_stimescale->value >= 1)
			flRealDuration = flCalcDuration;
		else//real duration = original starttime - real starttime + original duration
			flRealDuration = max(flStartTime + flCalcStartTime - flRealStartTime, 0) + flCalcDuration;

		pAddedLine = AddLine(dict, pStartSubtitleContext, pStart, nCharNum, flRealStartTime, flRealDuration, nWide);

		//Skip CRLF
		while (*p == L'\r' || *p == L'\n')
			p++;

		pStart = p;
	}

	if (pAddedLine && cap_subtitle_extraholdtime->value > 0)
	{
		//Add extra holdtime for the last line added
		pAddedLine->m_Duration += cap_subtitle_extraholdtime->value;
	}
}

void SubtitlePanel::ClearSubtitle(void)
{
	m_Lines.clear();
	m_BackLines.clear();
}

void SubtitlePanel::VidInit(void)
{

}

void SubtitlePanel::ConnectToServer(const char* game, int IP, int port)
{
	if (gEngfuncs.GetMaxClients() > 1)
	{
		ClearSubtitle();
	}
}

void SubtitlePanel::AdjustClock(double flAdjustment)
{
	for (auto it = m_Lines.begin(); it != m_Lines.end(); it++)
	{
		const auto& Temp = (*it);

		Temp->m_EndTime += flAdjustment;
		Temp->m_StartTime += flAdjustment;
	}

	for (auto it = m_BackLines.begin(); it != m_BackLines.end(); it++)
	{
		const auto& Temp = (*it);

		Temp->m_EndTime += flAdjustment;
		Temp->m_StartTime += flAdjustment;

	}
}

void SubtitlePanel::Paint(void)
{
	int iPanelWidth, iPanelHeight;
	GetSize(iPanelWidth, iPanelHeight);

	int x = m_iScaledXSpace;
	m_iPanelY = 9999;
	m_iPanelYEnd = -9999;
	m_iPanelAlpha = 0;

	iPanelWidth -= x << 1;

	//Draw lines and calculate the size of panel.

	surface()->DrawSetTextFont(m_hTextFont);

	for (auto it = m_Lines.begin(); it != m_Lines.end(); it++)
	{
		const auto& Line = (*it);

		Line->Draw(x, iPanelWidth, m_iTextAlign);

		if(Line->m_YPos - m_iScaledYSpace < m_iPanelY)
			m_iPanelY = Line->m_YPos - m_iScaledYSpace;

		if(Line->m_YPos + m_iFontTall + m_iScaledYSpace > m_iPanelYEnd)
			m_iPanelYEnd = Line->m_YPos + m_iFontTall + m_iScaledYSpace;

		if(Line->m_Alpha > m_iPanelAlpha)
			m_iPanelAlpha = Line->m_Alpha;
	}

	//Check if the panel is trying to fade in when it's fully hidden
	if(m_flCurPanelY > m_flCurPanelYEnd && m_iPanelY < m_iPanelYEnd)
	{
		m_flCurPanelY = m_iPanelY;
		m_flCurPanelYEnd = m_iPanelYEnd;
	}

	//The panel's top won't be higher than m_iPanelTop
	if(m_iPanelY < m_iPanelTop)
		m_iPanelY = m_iPanelTop;

	//Do not go away too far
	if(m_iPanelY > iPanelHeight)
		m_iPanelY = iPanelHeight;

	if(m_iPanelYEnd < 0)
		m_iPanelYEnd = 0;

	//Panel scaling animation
	if(m_flCurPanelY != m_iPanelY)
	{
		int sign = (m_iPanelY - m_flCurPanelY) ? 1 : -1;
		m_flCurPanelY += sign * 200.0f * (g_pViewPort->GetFrameTime());
		if(sign == 1 && m_flCurPanelY > m_iPanelY)
			m_flCurPanelY = m_iPanelY;
		else if(sign == -1 && m_flCurPanelY < m_iPanelY)
			m_flCurPanelY = m_iPanelY;
	}

	if(m_flCurPanelYEnd != m_iPanelYEnd)
	{
		int sign = (m_iPanelYEnd - m_flCurPanelYEnd) ? 1 : -1;
		m_flCurPanelYEnd += sign * 200.0f * (g_pViewPort->GetFrameTime());
		if(sign == 1 && m_flCurPanelYEnd > m_iPanelYEnd)
			m_flCurPanelYEnd = m_iPanelYEnd;
		else if(sign == -1 && m_flCurPanelYEnd < m_iPanelYEnd)
			m_flCurPanelYEnd = m_iPanelYEnd;
	}
}

void SubtitlePanel::PaintBackground(void)
{
	int x, y, w, h, r;

	int iPanelWidth, iPanelHeight;
	GetSize(iPanelWidth, iPanelHeight);

	//Use the Current value since we want the animation
	x = 0;
	y = m_flCurPanelY;
	w = iPanelWidth;
	h = m_flCurPanelYEnd - m_flCurPanelY;

	if(h <= 0)
		return;

	//if (SCR_IsLoadingVisible())
	//	return;

	//Remove any raw OpenGL call
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//Force GL_MODULATE to be applied
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);

	//Corner is maximum at 1/4 wide or tall
	r = min(min(m_iScaledCornorSize, h / 4), w / 4);

	//Do the AlphaMultiplier
	Color clr = m_PanelColor;
	clr[3] = clr[3] * m_iPanelAlpha / 255;

	surface()->DrawSetColor(clr);

	//Draw 4 side rectangles
	surface()->DrawFilledRect(x+r, y, x+w-r, y+r);
	surface()->DrawFilledRect(x+w-r, y+r, x+w, y+h-r);
	surface()->DrawFilledRect(x+r, y+h-r, x+w-r, y+h);
	surface()->DrawFilledRect(x, y+r, x+r, y+h-r);

	//Draw the central rectangle
	surface()->DrawFilledRect(x+r, y+r, x+w-r, y+h-r);

	//Draw 4 round cornors
	surface()->DrawSetTexture(m_iRoundCornorMaterial[0]);
	surface()->DrawTexturedRect(x, y, x+r, y+r);

	surface()->DrawSetTexture(m_iRoundCornorMaterial[1]);
	surface()->DrawTexturedRect(x+w-r, y, x+w, y+r);

	surface()->DrawSetTexture(m_iRoundCornorMaterial[2]);
	surface()->DrawTexturedRect(x+w-r, y+h-r, x+w, y+h);

	surface()->DrawSetTexture(m_iRoundCornorMaterial[3]);
	surface()->DrawTexturedRect(x, y+h-r, x+r, y+h);

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
