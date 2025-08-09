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
#include <sequence.h>
#include <regex>
#include <unordered_map>
#include "MurmurHash2.h"

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

class CDictionary : public IBaseInterface
{
public:
	void LoadFromRow(
		const char* szTitle,
		const char* szSentense,
		const char* szColor,
		const char* szDuration,
		const char* szSpeaker,
		const char* szNext,
		const char* szNextDelay,
		const char* szStyle,
		const Color& defaultColor,
		vgui::IScheme* ischeme,
		bool bUTF8BOM);

	void ProcessString(const std::wstring& input, const CStartSubtitleContext* pStartSubtitleContext, std::wstring& output);

	dict_t					m_Type{ DICT_CUSTOM };
	std::string				m_szTitle;
	std::wstring			m_szSentence;
	Color					m_Color{ Color(255, 255, 255, 255) };
	float					m_flDuration{};
	std::wstring			m_szSpeaker;
	float					m_flNextDelay{};
	std::string				m_szNext;
	std::weak_ptr<CDictionary> m_pNext;
	textalign_t				m_iTextAlign{ ALIGN_DEFAULT };
	bool					m_bIgnoreDistanceLimit{};
	bool					m_bIgnoreVolumeLimit{};
	bool					m_bRegex{};

	bool					m_bOverrideColor{};
	bool					m_bOverrideDuration{};
	Color					m_Color1{ Color(255, 255, 255, 255) };
	Color					m_Color2{ Color(255, 255, 255, 255) };

	bool					m_bDefaultColor{};
	std::unique_ptr<std::regex>	m_pRegex{};
};

class CTypedDictionaryHandle
{
public:
	CTypedDictionaryHandle(const std::string& title, dict_t type) :
		m_title(title), m_type(type)
	{
		
	}

	CTypedDictionaryHandle(const char* title, dict_t type) :
		m_title(title), m_type(type)
	{

	}

	bool operator == (const CTypedDictionaryHandle& a) const
	{
		return m_title == a.m_title && m_type == a.m_type;
	}

	std::string m_title;
	dict_t m_type{ DICT_CUSTOM };
};

class CTypedDictionaryHasher
{
public:
	std::size_t operator()(const CTypedDictionaryHandle& key) const
	{
		size_t seed = 0;

		seed ^= MurmurHash2(&key.m_type, sizeof(key.m_type), seed);
		seed ^= MurmurHash2(key.m_title.c_str(), key.m_title.size(), seed);

		return seed;
	}
};

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
	void LoadCustomDictionary(const char * fileName);
	void LinkDictionary(void);
	void ClearDictionary(void);

	//Subtitle Interface
	void StartSubtitle(const std::shared_ptr<CDictionary>& dict, float flDurationTime, const CStartSubtitleContext* pStartSubtitleContext);
	void StartNextSubtitle(const std::shared_ptr<CDictionary>& dict, const CStartSubtitleContext* pStartSubtitleContext);

	//Dictionary Interface
	std::shared_ptr<CDictionary> FindDictionaryCABI(const char * title);
	std::shared_ptr<CDictionary> FindDictionaryCABI(const char * title, dict_t Type);
	std::shared_ptr<CDictionary> FindDictionaryCXX(const std::string& title);
	std::shared_ptr<CDictionary> FindDictionaryCXX(const std::string& title, dict_t Type);
	std::shared_ptr<CDictionary> FindDictionaryRegex(const std::string & title, dict_t Type, std::smatch &result);

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
	std::vector<std::shared_ptr<CDictionary>> m_Dictionary;
	std::unordered_map<std::string, std::shared_ptr<CDictionary>> m_NamedDictionaryMap;
	std::unordered_map<CTypedDictionaryHandle, std::shared_ptr<CDictionary>, CTypedDictionaryHasher> m_TypedDictionaryMap;

	SubtitlePanel* m_pSubtitlePanel{};
	CCSChatDialog* m_pChatDialog{};
	char m_szLevelName[256]{};
	double m_CurTime{};
};

extern CViewport *g_pViewPort;