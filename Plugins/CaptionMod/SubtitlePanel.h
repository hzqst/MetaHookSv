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

class CSubLineAnim : public IBaseInterface
{
public:
	CSubLineAnim(float StartTime, float AnimTime, CSubLine* Line) : m_pLine(Line)
	{
		m_StartTime = StartTime;
		m_AnimTime = AnimTime;
		m_Started = false;
	}
	virtual bool IsPlaying(void)
	{
		return (g_pViewPort->GetCurTime() >= m_StartTime && g_pViewPort->GetCurTime() < m_StartTime + m_AnimTime);
	}
	virtual bool IsDonePlay(void)
	{
		return (g_pViewPort->GetCurTime() >= m_StartTime + m_AnimTime);
	}
	virtual LineAnim_t GetType(void) = 0;
	virtual bool Update(void) = 0;

protected:
	float	m_StartTime{};
	float	m_AnimTime{};
	bool	m_Started{};
	CSubLine* m_pLine{};
};

class CAnimMovement : public CSubLineAnim
{
public:
	CAnimMovement(float StartTime, float AnimTime, int EndValue, CSubLine* pLine) : CSubLineAnim(StartTime, AnimTime, pLine)
	{
		m_EndValue = EndValue;
	}
	virtual LineAnim_t GetType(void){return ANIM_MOVEMENT;}
	virtual bool Update(void);
private:
	int m_StartValue{};
	int m_EndValue{};
};

class CAnimAlphaFade : public CSubLineAnim
{
public:
	CAnimAlphaFade(float StartTime, float AnimTime, int EndValue, CSubLine* pLine) : CSubLineAnim(StartTime, AnimTime, pLine)
	{
		m_EndValue = EndValue;
	}
	virtual LineAnim_t GetType(void){return ANIM_ALPHAFADE;}
	virtual bool Update(void);
private:
	int m_StartValue{};
	int m_EndValue{};
};

class CSubLine : public IBaseInterface
{
public:
	CSubLine(SubtitlePanel *Panel, const std::shared_ptr<CDictionary> &dict, const wchar_t *sentence, size_t length) : m_Panel(Panel), m_Dict(dict), m_Sentence(sentence, (size_t)length), m_Length(length)
	{
	
	}

	void Draw(int x, int w, int align);
	bool Update(void);
	void MoveTo(int Dist, float Time);
	void AlphaFade(int Alpha, float Time);
	bool ShouldRetire(void);
	bool ShouldStart(void);
	int CalcYPos(void);
	float GetYPosOutRate(void);

	SubtitlePanel* m_Panel{};//Subtitle panel
	std::shared_ptr<CDictionary> m_Dict;//Linked dictionary
	std::wstring	m_Sentence{};//The sentence to display
	int				m_Length{};
	float			m_EndTime{};
	float			m_StartTime{};
	float			m_Duration{};
	Color			m_Color{}; //Render color
	int				m_Alpha{};//Alpha multiplier
	int				m_YPos{};//Y position
	int				m_LineIndex{};//Line number, 0 means the bottom
	bool			m_Retired{};//Should this line be retired?
	float			m_FadeOut{};//Fadeout duration
	std::vector<std::shared_ptr<CSubLineAnim>> m_AnimList;//Animation list
	int				m_TextWide{};
	textalign_t		m_TextAlign{ ALIGN_DEFAULT };
	bool			m_bHasSenderName{};
	std::string		m_SenderName;
};

class SubtitlePanel : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE(SubtitlePanel, vgui::EditablePanel);

public:
	SubtitlePanel(Panel *parent);
	~SubtitlePanel();

	void VidInit(void);
	void ConnectToServer(const char* game, int IP, int port);
	void AdjustClock(double flAdjustment);

public://Subtitle interface

	void StartSubtitle(const std::shared_ptr<CDictionary>& dict, float flDurationTime, float flStartTime, const CStartSubtitleContext * pStartSubtitleContext);
	void StartNextSubtitle(const std::shared_ptr<CDictionary>& dict, const CStartSubtitleContext* pStartSubtitleContext);

	std::shared_ptr<CSubLine> AddLine(const std::shared_ptr<CDictionary>& dict, const CStartSubtitleContext* pStartSubtitleContext, const wchar_t* wszSentence, int nLength, float flStartTime, float flDuration, int nTextWide);
	void StartLine(const std::shared_ptr<CSubLine> &Line);
	void ClearSubtitle(void);

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
	//CPanelAnimationVar( float, m_flFadeIn, "fadein", "0.5" );
	//CPanelAnimationVar( float, m_flFadeOut, "fadeout", "0.8" );
	//CPanelAnimationVar( float, m_flHoldTime, "holdtime", "4.0" );
	//CPanelAnimationVar( int, m_iPrefix, "prefix", "1" );
	//CPanelAnimationVar( int, m_iWaitPlay, "waitplay", "1" );
	//CPanelAnimationVar(int, m_iAntiSpam, "antispam", "1");
	//CPanelAnimationVar( float, m_flStartTimeScale, "stimescale", "1" );
	//CPanelAnimationVar( float, m_flHoldTimeScale, "htimescale", "1" );

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
	float						m_flCurPanelY;
	float						m_flCurPanelYEnd;
	//Textures
	int							m_iRoundCornorMaterial[4];
	//The lines that displaying now
	std::list<std::shared_ptr<CSubLine>>		m_Lines;
	//The lines that preparing to be displayed
	std::list<std::shared_ptr<CSubLine>>		m_BackLines;
	//is it in level? if not, clear the subtitles.
	bool						m_bInLevel;
};

#endif