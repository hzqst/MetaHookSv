#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <FileSystem.h>

#include <glew.h>

#include "SubtitlePanel.h"
#include "privatefuncs.h"

extern client_textmessage_t *g_pCurrentTextMessage;

using namespace vgui;

SubtitlePanel::SubtitlePanel(Panel *parent)  : EditablePanel(parent, "Subtitle")
{
	SetPaintBackgroundEnabled(true);
	SetPaintBorderEnabled(false);
	SetProportional(true);

	SetScheme("CaptionScheme");

	LoadControlSettings("captionmod/SubtitlePanel.res", "GAME");
	InvalidateLayout(false, true);

	vgui::ivgui()->AddTickSignal( GetVPanel() );
};

SubtitlePanel::~SubtitlePanel()
{
	for (int i = 0; i < m_Lines.Count(); ++i)
	{
		delete m_Lines[i];
	}
	m_Lines.RemoveAll();

	for (int i = 0; i < m_BackLines.Count(); ++i)
	{
		delete m_BackLines[i];
	}
	m_BackLines.RemoveAll();
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

	m_iCurPanelY = 0;
	m_iCurPanelYEnd = GetTall();
	m_iPanelY = 9999;
	m_iPanelYEnd = -9999;
}

bool CAnimMovement::Update(void)
{
	if(IsDonePlay())
	{
		m_Line->m_YPos = m_EndValue;
		return false;
	}
	else if(IsPlaying())
	{
		if(!m_Started)
		{
			m_Started = true;
			m_StartValue = m_Line->m_YPos;
		}
		float frac = min((g_pViewPort->GetSystemTime() - m_StartTime) / m_AnimTime, 1);
		m_Line->m_YPos = (int)(frac * m_EndValue + (1 - frac) * m_StartValue);
	}
	return true;
}

bool CAnimAlphaFade::Update(void)
{
	if(IsDonePlay())
	{
		m_Line->m_Alpha = m_EndValue;
		return false;
	}
	else if(IsPlaying())
	{
		if(!m_Started)
		{
			m_Started = true;
			m_StartValue = m_Line->m_Alpha;
		}
		float frac = min((g_pViewPort->GetSystemTime() - m_StartTime) / m_AnimTime, 1);
		m_Line->m_Alpha = (int)(frac * m_EndValue + (1 - frac) * m_StartValue);
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
	return (g_pViewPort->GetSystemTime() > m_StartTime);
}

bool CSubLine::ShouldRetire(void)
{
	return (g_pViewPort->GetSystemTime() > m_EndTime || m_LineIndex > m_Panel->m_iMaxLines);
}

bool CSubLine::Update(void)
{
	for(int i = 0; i < m_AnimList.Count(); ++i)
	{
		if(!m_AnimList[i]->Update())
		{
			delete m_AnimList[i];
			m_AnimList.Remove(i);
			i --;
		}
	}

	if(!m_Retired && ShouldRetire())
	{//Retire route
		m_Retired = true;
		AlphaFade(0, m_FadeOut);
	}
	else if(m_Retired && !m_Alpha)
	{//It dies when it goes 0 alpha 
		return false;
	}
	return true;
}

void CSubLine::MoveTo(int ToPos, float Time)
{
	//remove any movement before add a new one
	for(int i = 0;i < m_AnimList.Count(); ++i)
	{
		if(m_AnimList[i]->GetType() == ANIM_MOVEMENT)
		{
			delete m_AnimList[i];
			m_AnimList.Remove(i);
			i --;
		}
	}
	CSubLineAnim *Anim = (CSubLineAnim *)new CAnimMovement(g_pViewPort->GetSystemTime(), Time, ToPos, this);
	m_AnimList[m_AnimList.AddToTail()] = Anim;
}

void CSubLine::AlphaFade(int Alpha, float Time)
{
	CSubLineAnim *Anim = (CSubLineAnim *)new CAnimAlphaFade(g_pViewPort->GetSystemTime(), Time, Alpha, this);
	m_AnimList[m_AnimList.AddToTail()] = Anim;
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

void SubtitlePanel::StartNextSubtitle(CDictionary *Dict)
{
	//Check if there is a next dict to be played
	CDictionary *pNextDict = Dict->m_pNext;
	if(pNextDict)
	{
		StartSubtitle(pNextDict, g_pViewPort->GetSystemTime() + Dict->m_flNextDelay);
	}
}

void SubtitlePanel::OnTick( void )
{
	for(int i = 0; i < m_BackLines.Count(); ++i)
	{
		if(m_BackLines[i]->ShouldStart())
		{
			StartLine(m_BackLines[i]);
			m_BackLines.Remove(i);
			i --;
		}
	}
	for(int i = 0; i < m_Lines.Count(); ++i)
	{
		if(!m_Lines[i]->Update())
		{
			delete m_Lines[i];
			m_Lines.Remove(i);
			i --;
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

void SubtitlePanel::StartLine(CSubLine *Line)
{
	//Notice that the current line must disappear after any other existing lines
	//So we need the latest endtime of the current playing lines.

	float latestEndTime = 0;
	for(int i = 0;i < m_Lines.Count(); ++i)
	{
		CSubLine *Temp = m_Lines[i];
		//Move all existing lines up
		Temp->m_LineIndex ++;
		Temp->MoveTo(Temp->CalcYPos(), m_flFadeIn);

		if(m_Lines[i]->m_EndTime > latestEndTime)
			latestEndTime = m_Lines[i]->m_EndTime;
	}

	//Add to the current playing line
	m_Lines[m_Lines.AddToTail()] = Line;

	if (Line->m_StartTime == 0)
	{
		Line->m_StartTime = g_pViewPort->GetSystemTime();
	}

	//Give it the lastest endtime
	Line->m_EndTime = max(Line->m_StartTime + Line->m_Duration, latestEndTime);

	//Fade it now
	Line->AlphaFade(255, m_flFadeIn);

	StartNextSubtitle(Line->m_Dict);
}

void SubtitlePanel::AddLine(CDictionary *Dict, wchar_t *wszSentence, int nLength, float flStartTime, float flDuration, int nTextWide)
{
	CSubLine *Line = new CSubLine(this, Dict);
	m_BackLines[m_BackLines.AddToTail()] = Line;

	Line->m_Sentence.SetSize(nLength + 1);
	Q_wcsncpy(&Line->m_Sentence[0], wszSentence, (nLength + 1) * sizeof(wchar_t));
	Line->m_Length = nLength;
	Line->m_TextWide = nTextWide;
	Line->m_StartTime = flStartTime;
	Line->m_Duration = flDuration;
	Line->m_Color = Dict->m_Color;

	if (Dict->m_bDefaultColor && g_pCurrentTextMessage)
	{
		Line->m_Color = Color(g_pCurrentTextMessage->r1, g_pCurrentTextMessage->g1, g_pCurrentTextMessage->b1, g_pCurrentTextMessage->a1);
	}

	Line->m_Alpha = 0;
	Line->m_LineIndex = 0;
	Line->m_YPos = Line->CalcYPos();
	Line->m_FadeOut = m_flFadeOut;
	Line->m_TextAlign = Dict->m_iTextAlign ? Dict->m_iTextAlign : m_iTextAlign;
}

//2015-11-26 added htimescale for SubtitlePanel
void SubtitlePanel::StartSubtitle(CDictionary *Dict, float flStartTime)
{
	//Delay the current line till the last backline plays
	float flLatestStart = 0;
	for(int i = 0; i < m_BackLines.Count(); ++i)
	{
		if (m_iWaitPlay)
		{
			m_BackLines[i]->m_StartTime = 0;
		}
		else
		{
			if (m_BackLines[i]->m_StartTime > flLatestStart)
				flLatestStart = m_BackLines[i]->m_StartTime;
		}
		//Already in list, do not start one subtitle for twice at the same time.
		if(m_BackLines[i]->m_Dict == Dict)
			return;
	}

	if (m_iAntiSpam)
	{
		for (int i = 0; i < m_Lines.Count(); ++i)
		{
			//Already in display, ignore
			if (m_Lines[i]->m_Dict == Dict)
				return;
		}
	}

	std::wstring sentence;

	Dict->FinalizeString(sentence, m_iPrefix);

	int iPanelWidth = GetWide();
	int iMaxTextWidth = iPanelWidth - (m_iScaledXSpace << 1);

	wchar_t szBuf[4096];
	wchar_t *pStart = &sentence[0];
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

	float flDuration = Dict->m_flDuration;

	//use m_flHoldTime as default
	if(flDuration <= 0)
		flDuration = m_flHoldTime;

	if(flDuration <= 0)
		flDuration = 4.0f;

	if (!Dict->m_bOverrideDuration && g_pCurrentTextMessage)
	{
		if (g_pCurrentTextMessage->effect == 2 && g_pCurrentTextMessage->pMessage)
		{
			flDuration = (g_pCurrentTextMessage->fadein * sentence.length() ) + g_pCurrentTextMessage->fadeout + g_pCurrentTextMessage->holdtime;
		}
		else
		{
			flDuration = g_pCurrentTextMessage->holdtime + g_pCurrentTextMessage->fadein + g_pCurrentTextMessage->fadeout;
		}
	}

	if(m_flHoldTimeScale > 0)
		flDuration *= m_flHoldTimeScale;

	p = pStart;

	while(*pStart)
	{
		//fetch one line from this sentence
		nCharNum = 0;
		nWide = 0;

		int nLastWide;
		wchar_t *LastP;
		int nLastCharNum;
		while(1)
		{
			nLastWide = nWide;
			LastP = p;
			nLastCharNum = nCharNum;

			//English word don't break at half...
			if((*p >= L'A' && *p <= L'Z') || (*p >= L'a' && *p <= L'z'))
			{
				while((*p >= L'A' && *p <= L'Z') || (*p >= L'a' && *p <= L'z') || *p == L'.' || *p == L',' || *p == L'\'' || *p == L'"' || *p == L'-')
					szBuf[nCharNum++] = *p++;
				szBuf[nCharNum] = L'\0';
			}
			//number don't break at half...
			else if((*p >= L'0' && *p <= L'9') || *p == L':' || *p == L'-' || *p == L'\'' || *p == L'"'|| *p == L',' || *p == L'.')
			{
				while((*p >= L'0' && *p <= L'9') || *p == L':' || *p == L'-' || *p == L'\'' || *p == L'"' || *p == L',' || *p == L'.')
					szBuf[nCharNum++] = *p++;
				szBuf[nCharNum] = L'\0';
			}
			else
			{
				szBuf[nCharNum++] = *p++;
				szBuf[nCharNum] = L'\0';
			}
			surface()->GetTextSize(m_hTextFont, szBuf, nWide, nTall);

			//It's out of range? go back to last position and make it a new line
			if(nWide > iMaxTextWidth)
			{
				nWide = nLastWide;
				nCharNum = nLastCharNum;
				p = LastP;
				nAddedCharNum += nCharNum;
				break;
			}

			//Force to be a new line
			if (*p == L'\0' || *p == L'\n' || *p == L'\r')
			{
				nAddedCharNum += nCharNum;
				break;
			}
		}

		//Calculate the duration and start time
		float flPercentDuration = (float)nCharNum / nTotalCharNum;
		float flPercentStart = (float)(nAddedCharNum - nCharNum) / nTotalCharNum;

		//Calculate the StartTime
		float flCalcStartTime = flPercentStart * flDuration;
		float flRealStartTime;
		if(m_flStartTimeScale <= 0)
			flRealStartTime = flStartTime;
		else
			flRealStartTime = flStartTime + flCalcStartTime * m_flStartTimeScale;
		//Shall we wait for the latest backlines played?
		if(m_iWaitPlay)
			flRealStartTime = max(flRealStartTime, flLatestStart);

		//Calculate the Duration
		float flCalcDuration = flDuration * flPercentDuration;
		float flRealDuration;
		
		//longer start time won't change the duration
		if(m_flStartTimeScale >= 1)
			flRealDuration = flCalcDuration;
		else//real duration = original starttime - real starttime + original duration
			flRealDuration = max(flStartTime + flCalcStartTime - flRealStartTime, 0) + flCalcDuration;

		AddLine(Dict, pStart, nCharNum, flRealStartTime, flRealDuration, nWide);

		//skip CRLF
		while (*p == L'\r' || *p == L'\n')
			p++;

		pStart = p;
	}
}

void SubtitlePanel::ClearSubtitle(void)
{
	for(int i = 0;i < m_Lines.Count(); ++i)
		delete m_Lines[i];
	m_Lines.RemoveAll();
	for(int i = 0;i < m_BackLines.Count(); ++i)
		delete m_BackLines[i];
	m_BackLines.RemoveAll();
}

void SubtitlePanel::QuerySubtitlePanelVars(SubtitlePanelVars_t *vars)
{
	GetSize(vars->m_iWidth, vars->m_iHeight);

	int x, y;
	GetPos(x, y);
	vars->m_iYPos = y;

	vars->m_flFadeIn = m_flFadeIn;
	vars->m_flFadeOut = m_flFadeOut;
	vars->m_flHoldTime = m_flHoldTime;
	vars->m_flHoldTimeScale = m_flHoldTimeScale;
	vars->m_flStartTimeScale = m_flStartTimeScale;
	vars->m_iAntiSpam = m_iAntiSpam;
	vars->m_iPrefix = m_iPrefix;
	vars->m_iWaitPlay = m_iWaitPlay;
}

void SubtitlePanel::UpdateSubtitlePanelVars(SubtitlePanelVars_t *vars)
{
	SetSize(vars->m_iWidth, vars->m_iHeight);

	int x, y;
	GetPos(x, y);
	SetPos(x, vars->m_iYPos);

	m_flFadeIn = vars->m_flFadeIn;
	m_flFadeOut = vars->m_flFadeOut;
	m_flHoldTime = vars->m_flHoldTime;
	m_flHoldTimeScale = vars->m_flHoldTimeScale;
	m_flStartTimeScale = vars->m_flStartTimeScale;
	m_iAntiSpam = vars->m_iAntiSpam;
	m_iPrefix = vars->m_iPrefix;
	m_iWaitPlay = vars->m_iWaitPlay;
}

void SubtitlePanel::VidInit(void)
{
	if(gEngfuncs.GetMaxClients() > 1)
		ClearSubtitle();
}

void SubtitlePanel::Paint(void)
{
	//if (SCR_IsLoadingVisible())
	//	return;

	int x;

	int iPanelWidth, iPanelHeight;
	GetSize(iPanelWidth, iPanelHeight);

	x = m_iScaledXSpace;
	m_iPanelY = 9999;
	m_iPanelYEnd = -9999;
	m_iPanelAlpha = 0;

	iPanelWidth -= x << 1;

	//Draw lines and get how large the panel suppose to be.

	surface()->DrawSetTextFont(m_hTextFont);

	for(int i = 0; i < m_Lines.Count(); ++i)
	{
		CSubLine *Line = m_Lines[i];

		Line->Draw(x, iPanelWidth, m_iTextAlign);

		if(Line->m_YPos - m_iScaledYSpace < m_iPanelY)
			m_iPanelY = Line->m_YPos - m_iScaledYSpace;

		if(Line->m_YPos + m_iFontTall + m_iScaledYSpace > m_iPanelYEnd)
			m_iPanelYEnd = Line->m_YPos + m_iFontTall + m_iScaledYSpace;

		if(Line->m_Alpha > m_iPanelAlpha)
			m_iPanelAlpha = Line->m_Alpha;
	}

	//If the panel try to fade in when it's fully hidden
	if(m_iCurPanelY > m_iCurPanelYEnd && m_iPanelY < m_iPanelYEnd)
	{
		m_iCurPanelY = m_iPanelY;
		m_iCurPanelYEnd = m_iPanelYEnd;
	}

	//the panel's top won't be higher than m_iPanelTop
	if(m_iPanelY < m_iPanelTop)
		m_iPanelY = m_iPanelTop;

	//do not go away too far
	if(m_iPanelY > iPanelHeight)
		m_iPanelY = iPanelHeight;

	if(m_iPanelYEnd < 0)
		m_iPanelYEnd = 0;

	//Panel scaling animation
	if(m_iCurPanelY != m_iPanelY)
	{
		int sign = (m_iPanelY - m_iCurPanelY) ? 1 : -1;
		m_iCurPanelY += sign * 200.0f * (g_pViewPort->GetFrameTime());
		if(sign == 1 && m_iCurPanelY > m_iPanelY)
			m_iCurPanelY = m_iPanelY;
		else if(sign == -1 && m_iCurPanelY < m_iPanelY)
			m_iCurPanelY = m_iPanelY;
	}
	if(m_iCurPanelYEnd != m_iPanelYEnd)
	{
		int sign = (m_iPanelYEnd - m_iCurPanelYEnd) ? 1 : -1;
		m_iCurPanelYEnd += sign * 200.0f * (g_pViewPort->GetFrameTime());
		if(sign == 1 && m_iCurPanelYEnd > m_iPanelYEnd)
			m_iCurPanelYEnd = m_iPanelYEnd;
		else if(sign == -1 && m_iCurPanelYEnd < m_iPanelYEnd)
			m_iCurPanelYEnd = m_iPanelYEnd;
	}
}

void SubtitlePanel::PaintBackground(void)
{
	int x, y, w, h, r;

	int iPanelWidth, iPanelHeight;
	GetSize(iPanelWidth, iPanelHeight);

	//Use the Current value since we want the animation
	x = 0;
	y = m_iCurPanelY;
	w = iPanelWidth;
	h = m_iCurPanelYEnd - m_iCurPanelY;

	if(h <= 0)
		return;

	//if (SCR_IsLoadingVisible())
	//	return;

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//Cornor is maximum at 1/4 wide or tall
	r = min(min(m_iScaledCornorSize, h / 4), w / 4);

	//Do the AlphaMultiplier
	Color clr = m_PanelColor;
	clr[3] = clr[3] * m_iPanelAlpha / 255;

	surface()->DrawSetColor(clr);

	//Draw 4 side rectangle
	surface()->DrawFilledRect(x+r, y, x+w-r, y+r);
	surface()->DrawFilledRect(x+w-r, y+r, x+w, y+h-r);
	surface()->DrawFilledRect(x+r, y+h-r, x+w-r, y+h);
	surface()->DrawFilledRect(x, y+r, x+r, y+h-r);

	//Draw the central rectangle
	surface()->DrawFilledRect(x+r, y+r, x+w-r, y+h-r);

	//draw 4 round cornor
	surface()->DrawSetTexture(m_iRoundCornorMaterial[0]);
	surface()->DrawTexturedRect(x, y, x+r, y+r);

	surface()->DrawSetTexture(m_iRoundCornorMaterial[1]);
	surface()->DrawTexturedRect(x+w-r, y, x+w, y+r);

	surface()->DrawSetTexture(m_iRoundCornorMaterial[2]);
	surface()->DrawTexturedRect(x+w-r, y+h-r, x+w, y+h);

	surface()->DrawSetTexture(m_iRoundCornorMaterial[3]);
	surface()->DrawTexturedRect(x, y+h-r, x+r, y+h);
}