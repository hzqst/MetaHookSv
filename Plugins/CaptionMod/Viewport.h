#pragma once

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <VGUI_controls/Controls.h>
#include <VGUI_controls/Panel.h>
#include <VGUI_controls/Frame.h>
#include "libcsv/csv_document.h"
#include <sequence.h>
#include <regex>

#define HUDMESSAGE_MAXLENGTH 2048

class CCSChatDialog;
class SubtitlePanel;
class CHudMessage;
class ISchemel;

enum dict_t
{
	DICT_CUSTOM,
	DICT_SOUND,
	DICT_MESSAGE,
	DICT_SENTENCE,
	DICT_NETMESSAGE,
	DICT_SENDAUDIO,
};

enum textalign_t
{
	ALIGN_DEFAULT,
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT,
};

typedef struct QuerySubtitlePanelVars_s
{
	int m_iYPos;
	int m_iWidth;
	int m_iHeight;
	int m_iPrefix;
	int m_iWaitPlay;
	int m_iAntiSpam;
	float m_flFadeIn;
	float m_flFadeOut;
	float m_flHoldTime;
	float m_flStartTimeScale;
	float m_flHoldTimeScale;
}SubtitlePanelVars_t;

class CStartSubtitleContext
{
public:
	CStartSubtitleContext()
	{
		m_pszSenderName = nullptr;
		m_pCurrentTextMessage = nullptr;
	}


	const char* m_pszSenderName;
	client_textmessage_t* m_pCurrentTextMessage;
};

class CDictionary
{
public:
	CDictionary();
	virtual ~CDictionary();

	void Load(const CSV::CSVDocument::row_type &row, const Color &defaultColor, vgui::IScheme *ischeme);
	void ProcessString(const std::wstring& input, const CStartSubtitleContext* pStartSubtitleContext, std::wstring& output);

	dict_t					m_Type;
	std::string				m_szTitle;
	std::wstring			m_szSentence;
	Color					m_Color;
	float					m_flDuration;
	std::wstring			m_szSpeaker;
	float					m_flNextDelay;
	std::string				m_szNext;
	CDictionary				*m_pNext;
	textalign_t				m_iTextAlign;
	bool					m_bIgnoreDistanceLimit;
	bool					m_bIgnoreVolumeLimit;
	bool					m_bRegex;

	bool					m_bOverrideColor;
	bool					m_bOverrideDuration;
	Color					m_Color1;
	Color					m_Color2;

	bool					m_bDefaultColor;
	std::regex				*m_pRegex;
};

typedef struct hash_item_s
{
	CDictionary *dict;
	struct hash_item_s *next;
	struct hash_item_s *lastHash;
	int dictIndex;
}hash_item_t;

class CViewport : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CViewport, vgui::Panel);

public:
	CViewport();
	virtual ~CViewport(void);

public:
	//ClientVGUI Interface
	void Start(void);
	void Init(void);
	void VidInit(void);
	void Think(void);
	void Paint(void);
	void SetParent(vgui::VPANEL vPanel);
	void ConnectToServer(const char* game, int IP, int port);
	void ActivateClientUI(void);
	void HideClientUI(void);
	void LoadBaseDictionary(void);
	void LoadCustomDictionary(const char *dict_name);
	void LinkDictionary(void);

	//Subtitle Interface
	void StartSubtitle(CDictionary *dict, float flDurationTime, const CStartSubtitleContext* pStartSubtitleContext);
	void StartNextSubtitle(CDictionary *dict, const CStartSubtitleContext* pStartSubtitleContext);

	//Dictionary Hashtable
	CDictionary *FindDictionary(const char *szValue);
	CDictionary *FindDictionary(const char *szValue, dict_t Type);
	CDictionary *FindDictionaryRegex(const std::string &str, dict_t Type, std::smatch &result);
	int CaseInsensitiveHash(const char *string, int iBounds);
	void EmptyDictionaryHash(void);
	void AddDictionaryHash(CDictionary *dict, const char *value);
	void RemoveDictionaryHash(CDictionary *dict, const char *value);

	bool IsChatBlocked(int clientIndex);
	bool AllowedToPrintText(void);
	bool IsScoreBoardVisible(void);
	bool IsChatDialogOpened(void);
	void StopMessageMode(void);
	void StartMessageMode(void);
	void StartMessageMode2(void);
	void ChatPrintf(int iPlayerIndex, const wchar_t *buffer);
	void QuerySubtitlePanelVars(SubtitlePanelVars_t *vars);
	void UpdateSubtitlePanelVars(SubtitlePanelVars_t *vars);
	double GetCurTime(void) const;
	double GetFrameTime(void) const;
private:
	SubtitlePanel *m_pSubtitlePanel;
	CCSChatDialog *m_pChatDialog;
	CUtlVector<CDictionary *> m_Dictionary;	
	CUtlVector<hash_item_t> m_StringsHashTable;
	char m_szLevelName[256];
	double m_CurTime;
};

extern CViewport *g_pViewPort;