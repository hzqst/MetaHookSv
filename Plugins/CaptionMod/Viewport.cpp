#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/IEngineVGui.h>
#include <vgui/IGameUIFuncs.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include <IVGUI2Extension.h>
#include <IDpiManager.h>
#include "Viewport.h"
#include "SubtitlePanel.h"
#include "cstrikechatdialog.h"
#include "MemPool.h"
#include "message.h"
#include "privatefuncs.h"
#include "exportfuncs.h"
#include "util.h"

#include <libcsv/csv_document.h>

using namespace vgui;

ScClient_Sentence_t* ScClient_SoundEngine_GetSentenceByName(void* pSoundEngine, const char* name);

CViewport* g_pViewPort = NULL;

extern CHudMessage m_HudMessage;
extern CHudMenu m_HudMenu;
extern IGameUIFuncs* gameuifuncs;

CViewport::CViewport() : BaseClass(NULL, "CaptionViewport")
{
	int swide, stall;
	surface()->GetScreenSize(swide, stall);

	MakePopup(false, true);

	SetScheme2("CaptionScheme");
	SetBounds(0, 0, swide, stall);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
	SetProportional(true);
	m_pSubtitlePanel = NULL;
	m_pChatDialog = NULL;
	m_szLevelName[0] = 0;

	m_CurTime = 0;
}

CViewport::~CViewport(void)
{
	ClearDictionary();

	delete m_pSubtitlePanel;
	delete m_pChatDialog;
}

std::shared_ptr<CDictionary> CViewport::FindDictionaryCABI(const char* title)
{
	if (!m_Dictionary.size())
		return nullptr;

	std::string key(title);

	auto it = m_NamedDictionaryMap.find(key);
	if (it != m_NamedDictionaryMap.end())
	{
		return it->second;
	}

	return nullptr;
}

std::shared_ptr<CDictionary>CViewport::FindDictionaryCABI(const char* title, dict_t Type)
{
	if (!m_Dictionary.size())
		return nullptr;

	CTypedDictionaryHandle handle(title, Type);
	auto it = m_TypedDictionaryMap.find(handle);
	if (it != m_TypedDictionaryMap.end())
	{
		return it->second;
	}

	return nullptr;
}

std::shared_ptr<CDictionary> CViewport::FindDictionaryCXX(const std::string& title)
{
	if (!m_Dictionary.size())
		return nullptr;

	auto it = m_NamedDictionaryMap.find(title);
	if (it != m_NamedDictionaryMap.end())
	{
		return it->second;
	}

	return nullptr;
}

std::shared_ptr<CDictionary>CViewport::FindDictionaryCXX(const std::string& title, dict_t Type)
{
	if (!m_Dictionary.size())
		return nullptr;

	CTypedDictionaryHandle handle(title, Type);
	auto it = m_TypedDictionaryMap.find(handle);
	if (it != m_TypedDictionaryMap.end())
	{
		return it->second;
	}

	return nullptr;
}

std::shared_ptr<CDictionary>CViewport::FindDictionaryRegex(const std::string& str, dict_t Type, std::smatch& result)
{
	if (!m_Dictionary.size())
		return nullptr;

	for (size_t i = 0; i < m_Dictionary.size(); ++i)
	{
		if (m_Dictionary[i]->m_Type == Type && m_Dictionary[i]->m_pRegex)
		{
			if (std::regex_search(str, result, *m_Dictionary[i]->m_pRegex))
			{
				return m_Dictionary[i];
			}
		}
	}

	return NULL;
}

void CDictionary::LoadFromRow(
	const char* szTitle,
	const char* szSentence,
	const char* szColor,
	const char* szDuration,
	const char* szSpeaker,
	const char* szNext,
	const char* szNextDelay,
	const char* szStyle,
	const Color& defaultColor,
	vgui::IScheme* ischeme)
{
	m_Color = defaultColor;
	m_bDefaultColor = true;

	m_szTitle = szTitle;

	//If title ended with .wav

	if (m_szTitle.length() > 4 && !Q_stricmp(&m_szTitle[m_szTitle.length() - 4], ".wav"))
	{
		m_Type = DICT_SOUND;
	}
	else if (m_szTitle.length() > 4 && !Q_stricmp(&m_szTitle[m_szTitle.length() - 4], ".ogg"))
	{
		m_Type = DICT_SOUND;
	}
	else if (m_szTitle.length() > 4 && !Q_stricmp(&m_szTitle[m_szTitle.length() - 4], ".wma"))
	{
		m_Type = DICT_SOUND;
	}
	else if (m_szTitle.length() > 4 && !Q_stricmp(&m_szTitle[m_szTitle.length() - 4], ".mp3"))
	{
		m_Type = DICT_SOUND;
	}

	//If it's a textmessage found in engine (gamedir/titles.txt)
	client_textmessage_t* textmsg = gPrivateFuncs.pfnTextMessageGet(m_szTitle[0] == '#' ? &m_szTitle[1] : &m_szTitle[0]);
	if (textmsg)
	{
		m_Type = DICT_MESSAGE;
	}
	else if (m_szTitle[0] == '%' && m_szTitle[1] == '!')
	{
		m_Type = DICT_SENDAUDIO;
	}
	else if (m_szTitle[0] == '!' || m_szTitle[0] == '#')
	{
		m_Type = DICT_SENTENCE;
	}

	if (g_bIsSvenCoop)
	{
		auto sentenceObject = ScClient_SoundEngine_GetSentenceByName(gPrivateFuncs.ScClient_soundengine(), m_szTitle[0] == '#' ? &m_szTitle[1] : &m_szTitle[0]);

		if (sentenceObject)
		{
			m_Type = DICT_SENTENCE;
		}
	}

	//2015-11-26 added to support NETMESSAGE:
	if (!Q_strncmp(m_szTitle.c_str(), "NETMESSAGE_REGEX:", sizeof("NETMESSAGE_REGEX:") - 1))
	{
		m_Type = DICT_NETMESSAGE;
		m_szTitle = m_szTitle.substr(sizeof("NETMESSAGE_REGEX:") - 1);
		m_bRegex = true;
	}
	else if (!Q_strncmp(m_szTitle.c_str(), "NETMESSAGE:", sizeof("NETMESSAGE:") - 1))
	{
		m_Type = DICT_NETMESSAGE;
		m_szTitle = m_szTitle.substr(sizeof("NETMESSAGE:") - 1);
		m_bRegex = false;
	}
	else if (!Q_strncmp(m_szTitle.c_str(), "MESSAGE:", sizeof("MESSAGE:") - 1))
	{
		m_Type = DICT_MESSAGE;
		m_szTitle = m_szTitle.substr(sizeof("MESSAGE:") - 1);
		m_bRegex = false;
	}
	else if (!Q_strncmp(m_szTitle.c_str(), "SENTENCE:", sizeof("SENTENCE:") - 1))
	{
		m_Type = DICT_SENTENCE;
		m_szTitle = m_szTitle.substr(sizeof("SENTENCE:") - 1);
		m_bRegex = false;
	}
	else if (!Q_strncmp(m_szTitle.c_str(), "SENDAUDIO:", sizeof("SENDAUDIO:") - 1))
	{
		m_Type = DICT_SENDAUDIO;
		m_szTitle = m_szTitle.substr(sizeof("SENDAUDIO:") - 1);
		m_bRegex = false;
	}

	//Translated text
	if (szSentence && szSentence[0])
	{
		wchar_t* pwszLocalized = nullptr;
		if (szSentence[0] == '#')
		{
			pwszLocalized = localize()->Find(szSentence);

			if (pwszLocalized)
			{
				m_szSentence = pwszLocalized;
			}
		}

		if (!pwszLocalized)
		{
			wchar_t wszSentence[1024] = { 0 };
			localize()->ConvertANSIToUnicode(szSentence, wszSentence, sizeof(wszSentence));

			m_szSentence = wszSentence;
		}

		if (m_Type == DICT_NETMESSAGE && !m_bRegex)
		{
			StringReplaceA(m_szTitle, "\\n", "\n");
			StringReplaceA(m_szTitle, "\\r", "\r");
		}

		StringReplaceW(m_szSentence, L"\\n", L"\n");
		StringReplaceW(m_szSentence, L"\\r", L"\r");
	}

	if (m_Type == DICT_NETMESSAGE && m_bRegex)
	{
		m_pRegex = std::make_unique<std::regex>(m_szTitle);
	}

	if (szColor && szColor[0])
	{
		CUtlVector<char*> splitColor;
		V_SplitString(szColor, " ", splitColor);

		if (splitColor.Size() >= 2)
		{
			if (splitColor[0][0])
			{
				m_Color1 = ischeme->GetColor(splitColor[0], defaultColor);
			}
			if (splitColor[1][0])
			{
				m_Color2 = ischeme->GetColor(splitColor[1], defaultColor);
			}

			m_bOverrideColor = true;

			m_bDefaultColor = false;
		}
		else
		{
			m_Color = ischeme->GetColor(szColor, defaultColor);

			m_bDefaultColor = false;
		}

		splitColor.PurgeAndDeleteElements();
	}

	if (szDuration && szDuration[0])
	{
		m_flDuration = Q_atof(szDuration);

		if (m_flDuration > 0)
			m_bOverrideDuration = true;
	}

	if (szSpeaker && szSpeaker[0])
	{
		wchar_t* pwszLocalized = nullptr;
		if (szSpeaker[0] == '#')
		{
			pwszLocalized = localize()->Find(szSpeaker);

			if (pwszLocalized)
			{
				m_szSpeaker = pwszLocalized;
			}
		}
		if (!pwszLocalized)
		{
			wchar_t wszSpeaker[1024] = { 0 };
			localize()->ConvertANSIToUnicode(szSpeaker, wszSpeaker, sizeof(wszSpeaker));
			m_szSpeaker = wszSpeaker;
		}
	}

	//Next dictionary
	if (szNext && szNext[0])
	{
		m_szNext = szNext;

		if (szNextDelay && szNextDelay[0])
		{
			m_flNextDelay = Q_atof(szNextDelay);
		}
	}

	//Style
	if (szStyle && szStyle[0])
	{
		std::string style = szStyle;

		std::regex reg(" ");
		std::vector<std::string> elems(std::sregex_token_iterator(style.begin(), style.end(), reg, -1), std::sregex_token_iterator());

		for (auto& e : elems)
		{
			if (e.size() > 0)
			{
				e.erase(0, e.find_first_not_of(_T(" \n\r\t")));
				e.erase(e.find_last_not_of(_T(" \n\r\t")) + 1);

				if (e.size() == 1 && (e[0] == 'R' || e[0] == 'r'))
				{
					m_iTextAlign = ALIGN_RIGHT;
				}
				else if (e.size() == 1 && (e[0] == 'C' || e[0] == 'c'))
				{
					m_iTextAlign = ALIGN_CENTER;
				}
				else if (e.size() == 1 && (e[0] == 'L' || e[0] == 'L'))
				{
					m_iTextAlign = ALIGN_LEFT;
				}
				else if (e == "ALIGN_RIGHT")
				{
					m_iTextAlign = ALIGN_RIGHT;
				}
				else if (e == "ALIGN_CENTER")
				{
					m_iTextAlign = ALIGN_CENTER;
				}
				else if (e == "ALIGN_LEFT")
				{
					m_iTextAlign = ALIGN_LEFT;
				}
				else if (e == "IGNORE_DISTANCE_LIMIT")
				{
					m_bIgnoreDistanceLimit = true;
				}
				else if (e == "IGNORE_VOLUME_LIMIT")
				{
					m_bIgnoreVolumeLimit = true;
				}
			}
		}
	}
}

void CViewport::LoadCustomDictionary(const char* dict_name)
{
	CSV::CSVDocument doc;
	CSV::CSVDocument::row_index_type row_count = 0;

	//Parse from the document

	try
	{
		row_count = doc.load_file(dict_name);
	}
	catch (const std::exception& err)
	{
		gEngfuncs.Con_DPrintf("LoadCustomDictionary: %s\n", err.what());
		return;
	}

	if (row_count < 2)
	{
		gEngfuncs.Con_Printf("LoadCustomDictionary: too few lines in the dictionary file.\n");
		return;
	}

	IScheme* ischeme = scheme()->GetIScheme(GetScheme());

	if (!ischeme)
		return;

	Color defaultColor = ischeme->GetColor("BaseText", Color(255, 255, 255, 200));

	int nRowCount = row_count;

	//parse the dictionary line by line...
	for (int i = 1; i < nRowCount; ++i)
	{
		CSV::CSVDocument::row_type row = doc.get_row(i);

		if (row.size() < 1)
			continue;

		std::string title = row[0];

		if (title.empty())
			continue;

		auto Dict = std::make_shared<CDictionary>();

		std::string sentence = (row.size() >= 2) ? row[1] : "";
		std::string color = (row.size() >= 3) ? row[2] : "";
		std::string duration = (row.size() >= 4) ? row[3] : "";
		std::string speaker = (row.size() >= 5) ? row[4] : "";
		std::string next = (row.size() >= 6) ? row[5] : "";
		std::string nextDelay = (row.size() >= 7) ? row[6] : "";
		std::string style = (row.size() >= 8) ? row[7] : "";

		Dict->LoadFromRow(title.c_str(), sentence.c_str(), color.c_str(), duration.c_str(), speaker.c_str(), next.c_str(), nextDelay.c_str(), style.c_str(), defaultColor, ischeme);

		m_Dictionary.push_back(Dict);

		m_NamedDictionaryMap[Dict->m_szTitle] = Dict;

		CTypedDictionaryHandle handle(Dict->m_szTitle, Dict->m_Type);
		m_TypedDictionaryMap[handle] = Dict;
	}

	gEngfuncs.Con_Printf("LoadCustomDictionary: %d lines are loaded.\n", nRowCount - 1);
}

void CViewport::ClearDictionary(void)
{
	m_Dictionary.clear();
	m_NamedDictionaryMap.clear();
	m_TypedDictionaryMap.clear();
}

void CViewport::LinkDictionary(void)
{
	for (size_t i = 0; i < m_Dictionary.size(); ++i)
	{
		const auto& Dict = m_Dictionary[i];

		if (Dict->m_szNext.size() > 0)
		{
			Dict->m_pNext = FindDictionaryCXX(Dict->m_szNext);
		}
	}
}

void CViewport::LoadBaseDictionary(void)
{
	CSV::CSVDocument doc;
	CSV::CSVDocument::row_index_type row_count = 0;

	//Parse from the document

	try
	{
		row_count = doc.load_file("captionmod/dictionary.csv");
	}
	catch (const std::exception& err)
	{
		g_pMetaHookAPI->SysError("LoadBaseDictionary: %s\n", err.what());
	}

	if (row_count < 2)
	{
		gEngfuncs.Con_Printf("LoadBaseDictionary: too few lines in the dictionary file.\n");
		return;
	}

	IScheme* ischeme = scheme()->GetIScheme(GetScheme());

	if (!ischeme)
		return;

	Color defaultColor = ischeme->GetColor("BaseText", Color(255, 255, 255, 200));

	int nRowCount = row_count;

	//parse the dictionary line by line...
	for (int i = 1; i < nRowCount; ++i)
	{
		CSV::CSVDocument::row_type row = doc.get_row(i);

		if (row.size() < 1)
			continue;

		const char* title = row[0].c_str();

		if (!title || !title[0])
			continue;

		auto Dict = std::make_shared<CDictionary>();

		std::string sentence = (row.size() >= 2) ? row[1] : "";
		std::string color = (row.size() >= 3) ? row[2] : "";
		std::string duration = (row.size() >= 4) ? row[3] : "";
		std::string speaker = (row.size() >= 5) ? row[4] : "";
		std::string next = (row.size() >= 6) ? row[5] : "";
		std::string nextDelay = (row.size() >= 7) ? row[6] : "";
		std::string style = (row.size() >= 8) ? row[7] : "";

		Dict->LoadFromRow(title, sentence.c_str(), color.c_str(), duration.c_str(), speaker.c_str(), next.c_str(), nextDelay.c_str(), style.c_str(), defaultColor, ischeme);

		m_Dictionary.push_back(Dict);

		m_NamedDictionaryMap[Dict->m_szTitle] = Dict;

		CTypedDictionaryHandle handle(Dict->m_szTitle, Dict->m_Type);
		m_TypedDictionaryMap[handle] = Dict;
	}

	gEngfuncs.Con_Printf("LoadBaseDictionary: %d lines are loaded.\n", nRowCount - 1);
}

//const char* GetSenderName();

//KeyBinding Name(jump) -> Key Name(SPACE)
const char* PrimaryKey_ForBinding(const CStartSubtitleContext* pStartSubtitleContext, const char* binding)
{
	if (!strcmp(binding, "sender") && pStartSubtitleContext->m_pszSenderName)
	{
		return pStartSubtitleContext->m_pszSenderName;
	}

	if (binding[0] == '+')
		binding++;

	for (int i = 255; i >= 0; --i)
	{
		const char* found = gameuifuncs->Key_BindingForKey(i);

		if (found && found[0])
		{
			if (found[0] == '+')
				found++;
			if (!Q_stricmp(found, binding))
			{
				const char* key = gameuifuncs->Key_NameForKey(i);
				if (key && key[0])
				{
					return key;
				}
			}
		}
	}
	return "<not bound>";
}

void CDictionary::ProcessString(const std::wstring& input, const CStartSubtitleContext* pStartSubtitleContext, std::wstring& output)
{
	auto finalize = input;

	static std::wregex pattern(L"(<([A-Za-z_]+)>)");
	std::wsmatch result;
	std::regex_search(output, result, pattern);

	std::wstring skipped;

	std::wstring::const_iterator searchStart(finalize.cbegin());

	while (std::regex_search(searchStart, finalize.cend(), result, pattern) && result.size() > 2)
	{
		std::wstring prefix = result.prefix();
		std::wstring suffix = result.suffix();

		auto wkeybind = result[2].str();

		char akeybind[256] = { 0 };
		localize()->ConvertUnicodeToANSI(wkeybind.c_str(), akeybind, sizeof(akeybind) - 1);
		const char* pszBinding = PrimaryKey_ForBinding(pStartSubtitleContext, akeybind);

		if (pszBinding)
		{
			wchar_t wbinding[256] = { 0 };
			Q_UTF8ToUnicode(pszBinding, wbinding, sizeof(wbinding));

			if (searchStart != finalize.cbegin())
			{
				finalize = skipped + prefix;
			}
			else
			{
				finalize = prefix;
			}
			finalize += wbinding;

			auto currentLength = finalize.length();

			finalize += suffix;

			skipped = finalize.substr(0, currentLength);
			searchStart = finalize.cbegin() + currentLength;
			continue;
		}

		searchStart = result.suffix().first;
	}

	output = finalize;
}

void CViewport::Start(void)
{
	m_pSubtitlePanel = new SubtitlePanel(this);
	m_pChatDialog = new CCSChatDialog(this, PANEL_CHAT);

	SetVisible(false);
}

void CViewport::SetParent(VPANEL vPanel)
{
	BaseClass::SetParent(vPanel);

	if (g_iEngineType != ENGINE_GOLDSRC_HL25 && DpiManager()->IsHighDpiSupportEnabled())
	{
		SetProportional(true);
	}
}

void CViewport::Think(void)
{
	if ((*cl_time) > 1)
	{
		if ((*cl_time) < m_CurTime)
		{
			if (m_pSubtitlePanel)
				m_pSubtitlePanel->AdjustClock((*cl_time) - m_CurTime);
		}

		m_CurTime = (*cl_time);
	}
}

void CViewport::VidInit(void)
{
	m_szLevelName[0] = 0;

	if (!g_bIsSvenCoop)
	{
		LoadBaseDictionary();
		LinkDictionary();
	}

	m_pChatDialog->VidInit();
	m_pSubtitlePanel->VidInit();
	m_HudMessage.VidInit();
	m_HudMenu.VidInit();
}

void CViewport::Init(void)
{
	m_HudMessage.Init();
	m_HudMenu.Init();
}

void CViewport::StartSubtitle(const std::shared_ptr<CDictionary>& dict, float flDurationTime, const CStartSubtitleContext* pStartSubtitleContext)
{
	if (cap_enabled && cap_enabled->value) {
		m_pSubtitlePanel->StartSubtitle(dict, flDurationTime, g_pViewPort->GetCurTime(), pStartSubtitleContext);
	}
}

void CViewport::StartNextSubtitle(const std::shared_ptr<CDictionary>& dict, const CStartSubtitleContext* pStartSubtitleContext)
{
	if (cap_enabled && cap_enabled->value) {
		m_pSubtitlePanel->StartNextSubtitle(dict, pStartSubtitleContext);
	}
}

void CViewport::ConnectToServer(const char* game, int IP, int port)
{
	auto szLevelName = gEngfuncs.pfnGetLevelName();

	if (!szLevelName || !szLevelName[0])
		return;

	if (0 != strcmp(szLevelName, m_szLevelName))
	{
		if (!g_bIsSvenCoop)
		{
			std::string name = szLevelName;
			RemoveFileExtension(name);
			name += "_dictionary.csv";

			LoadCustomDictionary(name.c_str());

			if (0 != strcmp(VGUI2Extension()->GetCurrentLanguage(), "english"))
			{
				name = szLevelName;
				RemoveFileExtension(name);
				name += "_dictionary_";
				name += VGUI2Extension()->GetCurrentLanguage();
				name += ".csv";

				LoadCustomDictionary(name.c_str());
			}

			LinkDictionary();
		}

		strncpy(m_szLevelName, szLevelName, sizeof(m_szLevelName) - 1);
		m_szLevelName[sizeof(m_szLevelName) - 1] = 0;
	}

	if (m_pSubtitlePanel)
		m_pSubtitlePanel->ConnectToServer(game, IP, port);
}

void CViewport::ActivateClientUI(void)
{
	SetVisible(true);
}

void CViewport::HideClientUI(void)
{
	SetVisible(false);
}

double CViewport::GetCurTime(void) const
{
	return (*cl_time);
}

double CViewport::GetFrameTime(void) const
{
	return (*cl_time) - (*cl_oldtime);
}

void CViewport::Paint(void)
{
	BaseClass::Paint();

	m_HudMessage.Draw();
	m_HudMenu.Draw();
}

bool CViewport::AllowedToPrintText(void)
{
	if (gPrivateFuncs.GameViewport_AllowedToPrintText)
		return gPrivateFuncs.GameViewport_AllowedToPrintText(GameViewport, 0);

	return true;
}

bool CViewport::IsChatBlocked(int clientIndex)
{
	if (clientIndex >= 1 && clientIndex <= 32)
	{
		if (GetVoiceBanMask() & (1 << clientIndex))
			return true;
	}

	return false;
}

bool CViewport::IsScoreBoardVisible(void)
{
	if (gPrivateFuncs.GameViewport_IsScoreBoardVisible)
		return gPrivateFuncs.GameViewport_IsScoreBoardVisible(GameViewport, 0);

	return true;
}

bool CViewport::IsChatDialogOpened(void)
{
	return m_pChatDialog->IsVisible();
}

void CViewport::StopMessageMode(void)
{
	m_pChatDialog->StopMessageMode();
}

void CViewport::StartMessageMode(void)
{
	m_pChatDialog->StartMessageMode(MM_SAY);
}

void CViewport::StartMessageMode2(void)
{
	m_pChatDialog->StartMessageMode(MM_SAY_TEAM);
}

void CViewport::ChatPrintf(int iPlayerIndex, const wchar_t* buffer)
{
	m_pChatDialog->ChatPrintf(iPlayerIndex, buffer);
}