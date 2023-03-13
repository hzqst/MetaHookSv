#ifndef SUBTITLEPANEL_H
#define SUBTITLEPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <UtlVector.h>
#include "Viewport.h"
#include "privatefuncs.h"

enum LineAnim_t
{
	ANIM_ALPHAFADE,
	ANIM_MOVEMENT
};

class CSubLine;

class CSubLineAnim
{
public:
	CSubLineAnim(float StartTime, float AnimTime, CSubLine *Line)
	{
		m_StartTime = StartTime;
		m_AnimTime = AnimTime;
		m_Line = Line;
		m_Started = false;
	}
	virtual bool IsPlaying(void)
	{
		return (g_pViewPort->GetSystemTime() >= m_StartTime && g_pViewPort->GetSystemTime() < m_StartTime + m_AnimTime);
	}
	virtual bool IsDonePlay(void)
	{
		return (g_pViewPort->GetSystemTime() >= m_StartTime + m_AnimTime);
	}
	virtual LineAnim_t GetType(void) = 0;
	virtual bool Update(void) = 0;
protected:
	float	m_StartTime;
	float	m_AnimTime;
	CSubLine *m_Line;
	BOOL	m_Started;
};

class CAnimMovement : public CSubLineAnim
{
public:
	CAnimMovement(float StartTime, float AnimTime, int EndValue, CSubLine *Line) : CSubLineAnim(StartTime, AnimTime, Line)
	{
		m_EndValue = EndValue;
	}
	virtual LineAnim_t GetType(void){return ANIM_MOVEMENT;}
	virtual bool Update(void);
private:
	int m_StartValue;
	int m_EndValue;
};

class CAnimAlphaFade : public CSubLineAnim
{
public:
	CAnimAlphaFade(float StartTime, float AnimTime, int EndValue, CSubLine *Line) : CSubLineAnim(StartTime, AnimTime, Line)
	{
		m_EndValue = EndValue;
	}
	virtual LineAnim_t GetType(void){return ANIM_ALPHAFADE;}
	virtual bool Update(void);
private:
	int m_StartValue;
	int m_EndValue;
};

class CSubLine
{
public:
	CSubLine(SubtitlePanel *Panel, CDictionary *Dict)
	{
		m_Length = 0;
		m_Duration = 0;
		m_StartTime = 0;
		m_Alpha = 0;
		m_YPos = 0;
		m_LineIndex = 0;
		m_Retired = false;
		m_FadeOut = 0;
		m_Panel = Panel;
		m_Dict = Dict;
		m_TextWide = 0;
		m_TextAlign = ALIGN_DEFAULT;
	}

	void Draw(int x, int w, int align);
	bool Update(void);
	void MoveTo(int Dist, float Time);
	void AlphaFade(int Alpha, float Time);
	bool ShouldRetire(void);
	bool ShouldStart(void);
	int CalcYPos(void);
	float GetYPosOutRate(void);

	CUtlVector<wchar_t>	m_Sentence;
	int				m_Length;
	float			m_EndTime;
	float			m_StartTime;
	float			m_Duration;
	Color			m_Color; //Render color
	int				m_Alpha;//Alpha multiplier
	int				m_YPos;//Y position
	int				m_LineIndex;//Line number, 0 means the bottom
	bool			m_Retired;//Should this line be retired?
	float			m_FadeOut;//Fadeout duration
	CUtlVector<CSubLineAnim *>	m_AnimList;//Animation list
	SubtitlePanel	*m_Panel;//Subtitle panel
	CDictionary		*m_Dict;//Linked dictionary
	int				m_TextWide;
	textalign_t		m_TextAlign;
};

class SubtitlePanel : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE(SubtitlePanel, vgui::EditablePanel);

public:
	SubtitlePanel(Panel *parent);
	virtual ~SubtitlePanel();
	void VidInit(void);

public://Subtitle interface
	void StartSubtitle(CDictionary *Dict, float flStartTime);
	void StartNextSubtitle(CDictionary *Dict);
	void AddLine(CDictionary *Dict, wchar_t *wszSentence, int nLength, float flStartTime, float flDuration, int nTextLength);
	void StartLine(CSubLine *Line);
	void ClearSubtitle(void);
	void QuerySubtitlePanelVars(SubtitlePanelVars_t *vars);
	void UpdateSubtitlePanelVars(SubtitlePanelVars_t *vars);
protected:
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PaintBackground(void);
	virtual void Paint(void);
	virtual void ApplySettings( KeyValues *inResourceData );

	MESSAGE_FUNC(OnTick, "Tick");

	CPanelAnimationVar( int, m_iCornorSize, "cornorsize", "8" );
	CPanelAnimationVar( int, m_iXSpace, "xspace", "12" );
	CPanelAnimationVar( int, m_iYSpace, "yspace", "8" );
	CPanelAnimationVar( int, m_iLineSpace, "linespace", "8" );
	CPanelAnimationVar( Color, m_PanelColor, "panelcolor", "SubtitleBG" );
	CPanelAnimationVar( float, m_flFadeIn, "fadein", "0.5" );
	CPanelAnimationVar( float, m_flFadeOut, "fadeout", "0.8" );
	CPanelAnimationVar( float, m_flHoldTime, "holdtime", "4.0" );
	CPanelAnimationVar( int, m_iPrefix, "prefix", "1" );
	CPanelAnimationVar( int, m_iWaitPlay, "waitplay", "1" );
	CPanelAnimationVar(int, m_iAntiSpam, "antispam", "1");
	CPanelAnimationVar( float, m_flStartTimeScale, "stimescale", "1" );
	CPanelAnimationVar( float, m_flHoldTimeScale, "htimescale", "1" );

public:
	//Some attributes
	textalign_t					m_iTextAlign;
	vgui::HFont					m_hTextFont;
	int							m_iFontTall;
	int							m_iScaledCornorSize;
	int							m_iScaledXSpace;
	int							m_iScaledYSpace;
	int							m_iScaledLineSpace;
	int							m_iMaxLines;
	int							m_iPanelTop;
private:
	char						m_szTextAlign[32];
	char						m_szTextFont[32];
	int							m_iPanelY;
	int							m_iPanelYEnd;
	int							m_iPanelAlpha;
	//For panel scaling animation
	int							m_iCurPanelY;
	int							m_iCurPanelYEnd;
	int							m_iRoundCornorMaterial[4];
	//The lines that displaying now
	CUtlVector<CSubLine *>		m_Lines;
	//The lines that preparing to be displayed
	CUtlVector<CSubLine *>		m_BackLines;
	//is it in level? if not, clear the subtitles.
	bool						m_bInLevel;
};

#endif