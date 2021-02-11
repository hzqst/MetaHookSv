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
};

enum textalign_t
{
	ALIGN_DEFAULT,
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT,
};

class CDictionary
{
public:
	CDictionary();
	~CDictionary();
	void Load(CSV::CSVDocument::row_type &row, Color &defaultColor, vgui::IScheme *ischeme);
	void ReplaceKey(void);
	void ReplaceReturn(void);
	void AddPrefix(void);
	void ReplaceRegex(void);

	dict_t					m_Type;
	char					m_szTitle[256];
	CUtlVector<wchar_t>		m_szSentence;
	Color					m_Color;
	float					m_flDuration;
	CUtlVector<wchar_t>		m_szSpeaker;
	float					m_flNextDelay;
	char					m_szNext[64];
	CDictionary				*m_pNext;
	client_textmessage_t	*m_pTextMessage;
	textalign_t				m_iTextAlign;
	bool					m_bKeyReplaced;
	bool					m_bPrefixAdded;
	bool					m_bRegex;
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
	CViewport(void);
	virtual ~CViewport(void);

public:
	//ClientVGUI Interface
	void Start(void);
	void Init(void);
	void VidInit(void);
	void Think(void);
	void Paint(void);
	void SetParent(vgui::VPANEL vPanel);
	void ActivateClientUI(void);
	void HideClientUI(void);
	void LoadDictionary(void);

	//Subtitle Interface
	void StartSubtitle(CDictionary *dict);
	void StartNextSubtitle(CDictionary *dict);

	//Dictionary Hashtable
	CDictionary *FindDictionary(const char *szValue);
	CDictionary *FindDictionary(const char *szValue, dict_t Type);
	CDictionary *FindDictionaryRegex(const std::string &str, dict_t Type, std::smatch &result);
	int CaseInsensitiveHash(const char *string, int iBounds);
	void EmptyDictionaryHash(void);
	void AddDictionaryHash(CDictionary *dict, const char *value);
	void RemoveDictionaryHash(CDictionary *dict, const char *value);

private:
	SubtitlePanel *m_pSubtitle;
	CUtlVector<CDictionary *> m_Dictionary;	
	CUtlVector<hash_item_t> m_StringsHashTable;
};

extern CViewport *g_pViewPort;