#include <metahook.h>
#include <vgui/VGUI.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/IEngineVGui.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include "BaseUI.h"
#include "viewport.h"
#include "SubtitlePanel.h"
#include "MemPool.h"
#include "message.h"
#include "engfuncs.h"
#include "exportfuncs.h"
#include <stdexcept>

using namespace vgui;

CViewport *g_pViewPort = NULL;

//Dictionary hashtable
CMemoryPool m_HashItemMemPool(sizeof(hash_item_t), 64);

extern CHudMessage m_HudMessage;

CViewport::CViewport(void) : Panel(NULL, "CaptionViewport")
{
	int swide, stall;
	surface()->GetScreenSize(swide, stall);

	MakePopup(false, true);

	SetScheme("CaptionScheme");
	SetBounds(0, 0, swide, stall);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
	SetProportional(true);
	m_pSubtitle = NULL;
	m_szLevelName[0] = 0;
}

CViewport::~CViewport(void)
{
	for (int i = 0; i < m_Dictionary.Count(); ++i)
	{
		delete m_Dictionary[i];
	}

	m_Dictionary.RemoveAll();

	delete m_pSubtitle;
}

CDictionary *CViewport::FindDictionary(const char *szValue)
{
	if (!m_Dictionary.Count())
		return NULL;

	int hash = 0;
	hash_item_t *item;
	int count;

	hash = CaseInsensitiveHash(szValue, m_StringsHashTable.Count());
	count = m_StringsHashTable.Count();
	item = &m_StringsHashTable[hash];

	while (item->dict)
	{
		if (!Q_strcmp(item->dict->m_szTitle, szValue))
			break;

		hash = (hash + 1) % count;
		item = &m_StringsHashTable[hash];
	}

	if (!item->dict)
	{
		item->lastHash = NULL;
		return NULL;
	}

	m_StringsHashTable[hash].lastHash = item;
	return item->dict;
}

CDictionary *CViewport::FindDictionary(const char *szValue, dict_t Type)
{
	if (!m_Dictionary.Count())
		return NULL;

	int hash = 0;
	hash_item_t *item;
	int count;

	hash = CaseInsensitiveHash(szValue, m_StringsHashTable.Count());
	count = m_StringsHashTable.Count();
	item = &m_StringsHashTable[hash];

	while (item->dict)
	{
		if (!Q_strcmp(item->dict->m_szTitle, szValue) && item->dict->m_Type == Type)
			break;

		hash = (hash + 1) % count;
		item = &m_StringsHashTable[hash];
	}

	if (!item->dict)
	{
		item->lastHash = NULL;
		return NULL;
	}

	m_StringsHashTable[hash].lastHash = item;
	return item->dict;
}

CDictionary *CViewport::FindDictionaryRegex(const std::string &str, dict_t Type, std::smatch &result)
{
	if (!m_Dictionary.Count())
		return NULL;

	for(int i = 0; i < m_Dictionary.Count(); ++i)
	{
		if (m_Dictionary[i]->m_Type == Type)
		{
			std::regex pattern(m_Dictionary[i]->m_szTitle);

			if (std::regex_search(str, result, pattern)) 
			{
				return m_Dictionary[i];
			}
		}
	}

	return NULL;
}

int CViewport::CaseInsensitiveHash(const char *string, int iBounds)
{
	unsigned int hash = 0;

	if (!*string)
		return 0;

	while (*string)
	{
		if (*string < 'A' || *string > 'Z')
			hash = *string + 2 * hash;
		else
			hash = *string + 2 * hash + ' ';

		string++;
	}

	return (hash % iBounds);
}

void CViewport::EmptyDictionaryHash(void)
{
	int i;
	hash_item_t *item;
	hash_item_t *temp;
	hash_item_t *free;

	for (i = 0; i < m_StringsHashTable.Count(); i++)
	{
		item = &m_StringsHashTable[i];
		temp = item->next;
		item->dict = NULL;
		item->dictIndex = 0;
		item->lastHash = NULL;
		item->next = NULL;

		while (temp)
		{
			free = temp;
			temp = temp->next;
			m_HashItemMemPool.Free(free);
		}
	}
}

void CViewport::AddDictionaryHash(CDictionary *dict, const char *value)
{
	int count;
	hash_item_t *item;
	hash_item_t *next;
	hash_item_t *temp;
	hash_item_t *newp;
	unsigned int hash = 0;
	int dictIndex;
	CDictionary *dictTemp;

	if (!dict->m_szTitle[0])
		return;

	count = m_StringsHashTable.Count();
	hash = CaseInsensitiveHash(value, count);
	dictIndex = dict - m_Dictionary[0];

	item = &m_StringsHashTable[hash];

	while (item->dict)
	{
		if (!Q_strcmp(item->dict->m_szTitle, dict->m_szTitle))
			break;

		hash = (hash + 1) % count;
		item = &m_StringsHashTable[hash];
	}

	if (item->dict)
	{
		next = item->next;

		while (next)
		{
			if (item->dict == dict)
				break;

			if (item->dictIndex >= dictIndex)
				break;

			item = next;
			next = next->next;
		}

		if (dictIndex < item->dictIndex)
		{
			dictTemp = item->dict;
			item->dict = dict;
			item->lastHash = NULL;
			item->dictIndex = dictIndex;
			dictIndex = dictTemp - m_Dictionary[0];
		}
		else
			dictTemp = dict;

		if (item->dict != dictTemp)
		{
			temp = item->next;
			newp = (hash_item_t *)m_HashItemMemPool.Alloc(sizeof(hash_item_t));
			item->next = newp;
			newp->dict = dictTemp;
			newp->lastHash = NULL;
			newp->dictIndex = dictIndex;

			if (temp)
				newp->next = temp;
			else
				newp->next = NULL;
		}
	}
	else
	{
		item->dict = dict;
		item->lastHash = NULL;
		item->dictIndex = dict - m_Dictionary[0];
	}
}

void CViewport::RemoveDictionaryHash(CDictionary *dict, const char *value)
{
	int hash = 0;
	hash_item_t *item;
	hash_item_t *last;
	int dictIndex;
	int count;

	count = m_StringsHashTable.Count();
	hash = CaseInsensitiveHash(value, count);
	dictIndex = dict - m_Dictionary[0];


	hash = hash % count;
	item = &m_StringsHashTable[hash];

	while (item->dict)
	{
		if (!Q_strcmp(item->dict->m_szTitle, dict->m_szTitle))
			break;

		hash = (hash + 1) % count;
		item = &m_StringsHashTable[hash];
	}

	if (item->dict)
	{
		last = item;

		while (item->next)
		{
			if (item->dict == dict)
				break;

			last = item;
			item = item->next;
		}

		if (item->dict == dict)
		{
			if (last == item)
			{
				if (item->next)
				{
					item->dict = item->next->dict;
					item->dictIndex = item->next->dictIndex;
					item->lastHash = NULL;
					item->next = item->next->next;
				}
				else
				{
					item->dict = NULL;
					item->lastHash = NULL;
					item->dictIndex = 0;
				}
			}
			else
			{
				if (m_StringsHashTable[hash].lastHash == item)
					m_StringsHashTable[hash].lastHash = NULL;

				last->next = item->next;
				m_HashItemMemPool.Free(item);
			}
		}
	}
}

CDictionary::CDictionary()
{
	m_Type = DICT_CUSTOM;
	m_szTitle[0] = 0;
	m_Color = Color(255, 255, 255, 255);
	m_flDuration = 0;
	m_flNextDelay = 0;
	m_szNext[0] = 0;
	m_pNext = NULL;
	m_pTextMessage = NULL;
	m_iTextAlign = ALIGN_DEFAULT;
	m_bKeyReplaced = false;
	m_bPrefixAdded = false;
	m_bRegex = false;
	m_szSentence.RemoveAll();
	m_szSentence.AddToTail(L'\0');
	m_szSpeaker.RemoveAll();
	m_szSpeaker.AddToTail(L'\0');
}

CDictionary::~CDictionary()
{
	if(m_pTextMessage)
	{
		if(m_pTextMessage->pMessage)
			delete m_pTextMessage->pMessage;
		delete m_pTextMessage;

		m_pTextMessage = NULL;
	}
}

void CDictionary::Load(CSV::CSVDocument::row_type &row, Color &defaultColor, IScheme *ischeme)
{
	m_Color = defaultColor;

	const char *title = row[0].c_str();

	Q_memset(m_szTitle, 0, sizeof(m_szTitle));
	Q_strncpy(m_szTitle, title, sizeof(m_szTitle) - 1);

	//If title ended with .wav
	int titlelen = strlen(title);
	if (!Q_stricmp(&title[titlelen - 4], ".wav"))
	{
		m_Type = DICT_SOUND;
	}

	//2015-11-26 added to support !SENTENCE and #SENTENCE
	if (title[0] == '!' || title[0] == '#')
	{
		m_Type = DICT_SENTENCE;
	}

	//If it's a textmessage found in engine (gamedir/titles.txt)
	client_textmessage_t *textmsg = gEngfuncs.pfnTextMessageGet(title);
	if (textmsg)
	{
		m_Type = DICT_MESSAGE;
		m_pTextMessage = new client_textmessage_t;
		memcpy(m_pTextMessage, textmsg, sizeof(client_textmessage_t));
		m_pTextMessage->pMessage = (const char *)new char[HUDMESSAGE_MAXLENGTH];
	}

	//2015-11-26 added to support NETMESSAGE:
	if (!Q_strncmp(title, "NETMESSAGE:", sizeof("NETMESSAGE:") - 1))
	{
		m_Type = DICT_NETMESSAGE;
		memcpy(m_szTitle, title + sizeof("NETMESSAGE:") - 1, titlelen - (sizeof("NETMESSAGE:") - 1));
		m_szTitle[titlelen - (sizeof("NETMESSAGE:") - 1)] = 0;
	}

	//Translated text
	const char *sentence = row[1].c_str();
	wchar_t *pLocalized = NULL;
	int localizedLength;
	if (sentence[0] == '#')
	{
		pLocalized = localize()->Find(sentence);
		if (pLocalized)
		{
			localizedLength = Q_wcslen(pLocalized);
			m_szSentence.SetSize(localizedLength + 1);
			Q_wcsncpy(&m_szSentence[0], pLocalized, (localizedLength + 1) * sizeof(wchar_t));
		}
	}
	if (!pLocalized)
	{
		localizedLength = MultiByteToWideChar(CP_ACP, 0, sentence, -1, NULL, 0);
		m_szSentence.SetSize(localizedLength + 1);
		MultiByteToWideChar(CP_ACP, 0, sentence, -1, &m_szSentence[0], localizedLength);
		m_szSentence[localizedLength] = L'0';
	}

	//There is no <pattern> in text, marked as replaced

	if (!wcschr(&m_szSentence[0], L'<'))
	{
		m_bKeyReplaced = true;
	}

	if (m_Type == DICT_NETMESSAGE)
	{
		if (strstr(m_szTitle, "(") && strstr(m_szTitle, ")"))
		{
			if (wcschr(&m_szSentence[0], L'{') && wcschr(&m_szSentence[0], L'}'))
			{
				m_bRegex = true;
			}
		}
	}

	ReplaceReturn();

	const char *color = row[2].c_str();
	if(color[0])
	{
		m_Color = ischeme->GetColor(color, defaultColor);

		if(m_pTextMessage)
		{
			char szColor1[16];
			char szColor2[16];
			sscanf(color, "%s %s", szColor1, szColor2);
			if(szColor2[0])
			{
				Color clrColor2 = ischeme->GetColor(szColor2, defaultColor);
				m_pTextMessage->r2 = clrColor2.r();
				m_pTextMessage->g2 = clrColor2.g();
				m_pTextMessage->b2 = clrColor2.b();
			}
			if(szColor1[0])
			{
				Color clrColor1 = ischeme->GetColor(szColor1, defaultColor);
				m_pTextMessage->r1 = clrColor1.r();
				m_pTextMessage->g1 = clrColor1.g();
				m_pTextMessage->b1 = clrColor1.b();
			}
		}
	}

	const char *duration = row[3].c_str();
	if(duration[0])
	{
		m_flDuration = Q_atof(duration);

		if(m_pTextMessage)
		{
			m_pTextMessage->holdtime = m_flDuration;
		}
	}

	const char *speaker = row[4].c_str();
	if(speaker[0])
	{
		if(speaker[0] == '#')
		{
			pLocalized = localize()->Find(speaker);
			if(pLocalized)
			{
				localizedLength = Q_wcslen(pLocalized);
				m_szSpeaker.SetSize(localizedLength + 1);
				Q_wcsncpy(&m_szSpeaker[0], pLocalized, (localizedLength + 1) * sizeof(wchar_t));
			}
		}
		if(!pLocalized)
		{
			localizedLength = MultiByteToWideChar(CP_ACP, 0, speaker, -1, NULL, 0);
			m_szSpeaker.SetSize(localizedLength + 1);
			MultiByteToWideChar(CP_ACP, 0, speaker, -1, &m_szSpeaker[0], localizedLength);
			m_szSpeaker[localizedLength] = L'0';
		}
	}

	if(m_pTextMessage)
	{
		//Covert the sentence text to UTF8
		std::string sentence;
		sentence.resize(HUDMESSAGE_MAXLENGTH);

		int finalLength = localize()->ConvertUnicodeToANSI(m_szSentence.Base(), (char *)sentence.data(), sentence.length());

		sentence.resize(finalLength);

		V_strncpy((char *)m_pTextMessage->pMessage, sentence.data(), HUDMESSAGE_MAXLENGTH - 1);
		((char *)m_pTextMessage->pMessage)[HUDMESSAGE_MAXLENGTH - 1] = 0;
	}

	//Next dictionary
	if(row.size() >= 7)
	{
		const char *next = row[5].c_str();
		if(next[0])
		{
			Q_strncpy(m_szNext, next, sizeof(m_szNext));
		}

		const char *nextdelay = row[6].c_str();
		if(nextdelay[0])
		{
			m_flNextDelay = Q_atof(nextdelay);
		}
	}

	//Text alignment
	if(row.size() >= 8)
	{
		const char *textalign = row[7].c_str();
		if(textalign[0] == 'R' || textalign[0] == 'r')
			m_iTextAlign = ALIGN_RIGHT;
		else if(textalign[0] == 'C' || textalign[0] == 'c')
			m_iTextAlign = ALIGN_CENTER;
		if(textalign[0] == 'L' || textalign[0] == 'l')
			m_iTextAlign = ALIGN_LEFT;
	}
}

void CViewport::LoadCustomDictionary(const char *dict_name)
{
	CSV::CSVDocument doc;
	CSV::CSVDocument::row_index_type row_count = 0;

	//Parse from the document

	try
	{
		row_count = doc.load_file(dict_name);
	}
	catch (std::exception &err)
	{
		gEngfuncs.Con_Printf("LoadCustomDictionary: %s", err.what());
	}

	if (row_count < 2)
		return;

	IScheme *ischeme = scheme()->GetIScheme(GetScheme());

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

		const char *title = row[0].c_str();

		if (!title || !title[0])
			continue;

		CDictionary *Dict = new CDictionary;

		Dict->Load(row, defaultColor, ischeme);

		m_Dictionary.AddToTail(Dict);

		AddDictionaryHash(Dict, Dict->m_szTitle);
	}
}

void CViewport::LinkDictionary(void)
{
	for (int i = 0; i < m_Dictionary.Count(); ++i)
	{
		CDictionary *Dict = m_Dictionary[i];
		if (Dict->m_szNext[0])
		{
			Dict->m_pNext = FindDictionary(Dict->m_szNext);
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
	catch(std::exception &err)
	{
		Sys_ErrorEx("LoadBaseDictionary: %s", err.what());
	}

	if(row_count < 2)
		return;

	IScheme *ischeme = scheme()->GetIScheme(GetScheme());

	if(!ischeme)
		return;

	Color defaultColor = ischeme->GetColor("BaseText", Color(255, 255, 255, 200));
	
	//Initialize the dictionary hashtable
	m_StringsHashTable.SetSize(2048);

	for (int i = 0; i < m_StringsHashTable.Count(); i++)
		m_StringsHashTable[i].next = NULL;

	EmptyDictionaryHash();

	int nRowCount = row_count;
	
	//parse the dictionary line by line...
	for (int i = 1;i < nRowCount; ++i)
	{
		CSV::CSVDocument::row_type row = doc.get_row(i);

		if(row.size() < 1)
			continue;

		const char *title = row[0].c_str();

		if(!title || !title[0])
			continue;

		CDictionary *Dict = new CDictionary;

		Dict->Load(row, defaultColor, ischeme);

		m_Dictionary.AddToTail(Dict);

		AddDictionaryHash(Dict, Dict->m_szTitle);
	}
	
}

//KeyBinding Name(jump) -> Key Name(SPACE)
const char *PrimaryKey_ForBinding(const char *binding)
{
	if(binding[0] == '+')
		binding ++;

	for (int i = 255; i >= 0; --i)
	{
		const char *found = gameuifuncs->Key_BindingForKey(i);

		if(found && found[0])
		{
			if(found[0] == '+')
				found ++;
			if(!Q_stricmp(found, binding))
			{
				const char *key = gameuifuncs->Key_NameForKey(i);
				if(key && key[0])
				{
					return key;
				}
			}
		}
	}
	return "<not bound>";
}

void CDictionary::AddPrefix(void)
{
	if(m_bPrefixAdded)
		return;

	m_bPrefixAdded = true;

	int nSentenceLength = Q_wcslen(&m_szSentence[0]);
	int nSpeakerLength = Q_wcslen(&m_szSpeaker[0]);

	if(!nSentenceLength || !nSpeakerLength)
		return;

	//No enough space for new sentence?
	int nCharToAdd = (nSentenceLength + nSpeakerLength + 1) - m_szSentence.Count();

	if(nCharToAdd > 0)
		m_szSentence.AddMultipleToTail(nCharToAdd);

	memcpy(&m_szSentence[0] + nSpeakerLength, &m_szSentence[0], sizeof(wchar_t) * nSentenceLength);
	memcpy(&m_szSentence[0], &m_szSpeaker[0], sizeof(wchar_t) * nSpeakerLength);
	m_szSentence[nSentenceLength + nSpeakerLength] = L'\0';
}

void CDictionary::ReplaceRegex(void)
{

}

void CDictionary::ReplaceKey(void)
{
	wchar_t *p = &m_szSentence[0];
	wchar_t *pLeftQuote = NULL;
	wchar_t *pRightQuote = NULL;
	const char *pszBinding = NULL;
	wchar_t wszKey[32];
	char szKey[32];

	if(m_bKeyReplaced)
		return;

	m_bKeyReplaced = true;

	bool bReplaced = false;

	while(*p++)
	{
		if(!pRightQuote && *p == L'<')
			pLeftQuote = p;
		if(pLeftQuote && *p == L'>')
			pRightQuote = p;

		if(pLeftQuote && pRightQuote)
		{
			int maxBytes = min(sizeof(wchar_t)*(pRightQuote-pLeftQuote), sizeof(wszKey));
			Q_wcsncpy(wszKey, pLeftQuote + 1, maxBytes);

			if(!wszKey[0])
				continue;

			int OldKeyLength = Q_wcslen(wszKey);

			WideCharToMultiByte(CP_ACP, 0, wszKey, -1, szKey, sizeof(szKey), NULL, NULL);
			szKey[sizeof(szKey)-1] = '\0';

			pszBinding = PrimaryKey_ForBinding(szKey);

			if(pszBinding)
			{
				bReplaced = true;
				MultiByteToWideChar(CP_ACP, 0, pszBinding, -1, wszKey, sizeof(wszKey)/sizeof(wchar_t));

				//make it upper case
				wcsupr(wszKey);

				int NewKeyLength = Q_wcslen(wszKey);

				if(NewKeyLength != OldKeyLength + 2)
				{
					//The new key is too long, we need to extend the vector!
					if(NewKeyLength > OldKeyLength + 2)
					{
						int nExtendLength = NewKeyLength - OldKeyLength + 2;

						//save the relative position of all pointers
						int Position_p = p - &m_szSentence[0];
						int Position_pLeftQuote = pLeftQuote - &m_szSentence[0];
						int Position_pRightQuote = pRightQuote - &m_szSentence[0];
						//The memory is reallocated to another place
						m_szSentence.AddMultipleToTail(nExtendLength);
						//So we need to fix all pointers
						p = &m_szSentence[0] + Position_p;
						pLeftQuote = &m_szSentence[0] + Position_pLeftQuote;
						pRightQuote = &m_szSentence[0] + Position_pRightQuote;
					}

					//Move the right part of text to new place
					int nRightLength = Q_wcslen(pRightQuote + 1);
					memcpy(pLeftQuote + NewKeyLength, pRightQuote + 1, sizeof(wchar_t) * (nRightLength + 1));

					//reset the pointer to position after the key
					p = pLeftQuote + NewKeyLength;
				}
				else
				{
					//Do nothing since the total length did not change, skip the <pattern>
					p = pRightQuote + 1;
				}
				
				//Paste the key
				memcpy(pLeftQuote, wszKey, sizeof(wchar_t) * NewKeyLength);
			}

			//Find next left/right quote
			pLeftQuote = NULL;
			pRightQuote = NULL;
		}
	}

	if(bReplaced && m_pTextMessage)
	{
		if(m_pTextMessage->pMessage)
			delete m_pTextMessage->pMessage;

		//Covert the text to UTF8
		int utf8Length = WideCharToMultiByte(CP_UTF8, 0, &m_szSentence[0], -1, NULL, 0, NULL, NULL);
		char *utf8Text = new char[utf8Length + 1];
		WideCharToMultiByte(CP_UTF8, 0, &m_szSentence[0], -1, utf8Text, utf8Length, NULL, NULL);
		utf8Text[utf8Length] = '\0';
		m_pTextMessage->pMessage = utf8Text;
	}
}

//2015-11-27 added
//Purpose: replace all "\r" "\n" to '\r' '\n'
void CDictionary::ReplaceReturn(void)
{
	wchar_t *p = &m_szSentence[0];

	wchar_t *pBackSlash = NULL;

	//empty sentence?
	if(!p[0])
		return;

	//make sure we have at least two characters
	while(*p && *(p + 1))
	{
		if(*p == L'\\')
		{
			int bMove = false;
			if(*(p + 1) == L'r')
			{
				*p = L'\r';
				bMove = true;
			}
			else if(*(p + 1) == L'n')
			{
				*p = L'\n';
				bMove = true;
			}
			if(bMove)
			{
				int nCharsToMove = Q_wcslen(p + 2);
				memcpy(p + 1, p + 2, (nCharsToMove + 1) * sizeof(wchar_t));
			}
		}

		p ++;
	}
}

void CViewport::Start(void)
{
	m_pSubtitle = new SubtitlePanel(NULL);

	SetVisible(false);
}

void CViewport::SetParent(VPANEL vPanel)
{
	BaseClass::SetParent(vPanel);

	m_pSubtitle->SetParent(this);
}

void CViewport::Think(void)
{
	if (!gEngfuncs.pfnGetLevelName() || !gEngfuncs.pfnGetLevelName()[0])
		return;

	if (0 != strcmp(gEngfuncs.pfnGetLevelName(), m_szLevelName))
	{
		std::string name = gEngfuncs.pfnGetLevelName();
		name = name.substr(0, name.length() - 4);
		name += "_dictionary.csv";

		LoadCustomDictionary(name.c_str());
		LinkDictionary();

		strcpy(m_szLevelName, gEngfuncs.pfnGetLevelName());
	}
}

void CViewport::VidInit(void)
{
	m_szLevelName[0] = 0;
	LoadBaseDictionary();
	LinkDictionary();

	m_HudMessage.VidInit();
}

void CViewport::Init(void)
{
	m_HudMessage.Init();
}

void CViewport::StartSubtitle(CDictionary *dict)
{
	if(cap_enabled->value)
		m_pSubtitle->StartSubtitle(dict, cl_time);
}

void CViewport::StartNextSubtitle(CDictionary *dict)
{
	if (cap_enabled->value)
		m_pSubtitle->StartNextSubtitle(dict);
}

void CViewport::ActivateClientUI(void)
{
	SetVisible(true);
}

void CViewport::HideClientUI(void)
{
	SetVisible(false);
}

void CViewport::Paint(void)
{
	BaseClass::Paint();

	m_HudMessage.Draw();
}