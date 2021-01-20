#include <windows.h>

#include "interface.h"
#include <KeyValues.h>
#include "FileSystem.h"
#include <vstdlib/IKeyValuesSystem.h>

#include <VGUI/ISystem.h>
#include "vgui_internal.h"

#include <Color.h>
#include <assert.h>
#include <stdlib.h>
#include <direct.h>
#include "tier0/mem.h"
#include "utlvector.h"
#include "utlbuffer.h"
#include "strtools.h"

#include <tier0/memdbgon.h>

static char *s_LastFileLoadingFrom = "unknown";

#define KEYVALUES_TOKEN_SIZE 1024

KeyValues::KeyValues(const char *setName)
{
	Init();
	SetName(setName);
}

KeyValues::KeyValues(const char *setName, const char *firstKey, const char *firstValue)
{
	Init();
	SetName(setName);
	SetString(firstKey, firstValue);
}

KeyValues::KeyValues(const char *setName, const char *firstKey, const wchar_t *firstValue)
{
	Init();
	SetName(setName);
	SetWString(firstKey, firstValue);
}

KeyValues::KeyValues(const char *setName, const char *firstKey, int firstValue)
{
	Init();
	SetName(setName);
	SetInt(firstKey, firstValue);
}

KeyValues::KeyValues(const char *setName, const char *firstKey, const char *firstValue, const char *secondKey, const char *secondValue)
{
	Init();
	SetName(setName);
	SetString(firstKey, firstValue);
	SetString(secondKey, secondValue);
}

KeyValues::KeyValues(const char *setName, const char *firstKey, int firstValue, const char *secondKey, int secondValue)
{
	Init();
	SetName(setName);
	SetInt(firstKey, firstValue);
	SetInt(secondKey, secondValue);
}

void KeyValues::Init(void)
{
	m_iKeyName = INVALID_KEY_SYMBOL;
	m_iDataType = TYPE_NONE;

	m_pSub = NULL;
	m_pPeer = NULL;

	m_sValue = NULL;
	m_wsValue = NULL;
	m_pValue = NULL;

	m_pChain = NULL;
	m_bHasEscapeSequences = false;
}

KeyValues::~KeyValues(void)
{
	RemoveEverything();
}

void KeyValues::RemoveEverything(void)
{
	KeyValues *dat;
	KeyValues *datNext = NULL;

	for (dat = m_pSub; dat != NULL; dat = datNext)
	{
		datNext = dat->m_pPeer;
		dat->m_pPeer = NULL;

		if (dat->m_iDataType == TYPE_STRING || dat->m_iDataType == TYPE_WSTRING)
			delete dat->m_pValue;

		delete dat;
	}

	for (dat = m_pPeer; dat && dat != this; dat = datNext)
	{
		datNext = dat->m_pPeer;
		dat->m_pPeer = NULL;

		if (dat->m_iDataType == TYPE_STRING || dat->m_iDataType == TYPE_WSTRING)
			delete dat->m_pValue;

		delete dat;
	}

	Init();
}

void KeyValues::RecursiveSaveToFile(CUtlBuffer &buf, int indentLevel)
{
	WriteIndents(buf, indentLevel);
	buf.Printf("\"%s\"\n", GetName());
	WriteIndents(buf, indentLevel);
	buf.Printf("{\n");

	for (KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		if (dat->m_pSub)
		{
			dat->RecursiveSaveToFile(buf, indentLevel + 1);
		}
		else
		{
			if (dat->m_sValue && *(dat->m_sValue))
			{
				WriteIndents(buf, indentLevel + 1);
				buf.Printf("\"%s\"\t\t\"%s\"\n", dat->GetName(), dat->m_sValue);
			}
		}
	}

	WriteIndents(buf, indentLevel);
	buf.Printf("}\n");
}

void KeyValues::ChainKeyValue(KeyValues *pChain)
{
	m_pChain = pChain;
}

const char *KeyValues::GetName(void)const
{
	return KeyValuesSystem()->GetStringForSymbol(m_iKeyName);
}

int KeyValues::GetNameSymbol(void)const
{
	return m_iKeyName;
}

const char *KeyValues::ReadToken(char **buffer, bool &wasQuoted)
{
 	static char buf[KEYVALUES_TOKEN_SIZE];

	if ((*buffer) == NULL || (**buffer) == 0)
		return NULL;

	int bufC = 0;
	char c = 0;

	while (true)
	{
		do
		{
			c = **buffer;
			(*buffer)++;
		}
		while ((c > 0) && (c <= ' '));

		if (!c)
			return NULL;

		if (c != '/')
		{
			break;
		}
		else
		{
			c = **buffer;
			(*buffer)++;

			if (c != '/')
			{
				(*buffer)--;
				c = '/';
				break;
			}
			else
			{
				while (c > 0 && c != '\n')
				{
					c = **buffer;
					(*buffer)++;
				}
			}
		}
	}

	if (c == '\"')
	{
		wasQuoted = true;

		while (true)
		{
			c = **buffer;
			(*buffer)++;

			if (c == 0)
			{
				DevMsg(1, "KeyValues::ReadToken unexpected EOF in quoted string in file %s.\n", s_LastFileLoadingFrom);
				return NULL;
			}

			if (c == '\"')
				break;

			if (c == '\\' && m_bHasEscapeSequences)
			{
				c = **buffer;
				(*buffer)++;

				switch (c)
				{
					case 0:
					{
						DevMsg(1, "KeyValues::ReadToken unexpected EOF after \\ in file %s.\n", s_LastFileLoadingFrom);
						return NULL;
					}

					case 'n': c = '\n'; break;
					case '\\': c = '\\'; break;
					case 't': c = '\t'; break;
					case '\"': c = '\"'; break;

					default:
					{
						DevMsg(1, "KeyValues::ReadToken illegal character after \\ in file %s.\n", s_LastFileLoadingFrom);
						c = c;
						break;
					}
				}
			}

			if (bufC < (KEYVALUES_TOKEN_SIZE - 1))
				buf[bufC++] = c;
			else
				DevMsg(1, "KeyValues::ReadToken overflow (>%i) in file %s.\n", KEYVALUES_TOKEN_SIZE, s_LastFileLoadingFrom);
		}
	}
	else if (c == '{' || c == '}')
	{
		wasQuoted = false;
		buf[bufC++] = c;
	}
	else
	{
		wasQuoted = false;

		while (true)
		{
			if (c == 0)
				break;

			if (c == '"' || c == '{' || c == '}')
				break;

			if (c <= ' ')
				break;

			if (bufC < (KEYVALUES_TOKEN_SIZE - 1))
				buf[bufC++] = c;
			else
				DevMsg(1, "KeyValues::ReadToken overflow (>%i) in file %s.\n", KEYVALUES_TOKEN_SIZE, s_LastFileLoadingFrom);

			c = **buffer;
			(*buffer)++;
		}

		(*buffer)--;
	}

	if (bufC == 0)
		return "";

	buf[bufC] = 0;
	return buf;
}

void KeyValues::UsesEscapeSequences(bool state)
{
	m_bHasEscapeSequences = state;
}

bool KeyValues::LoadFromFile(IBaseFileSystem *filesystem, const char *resourceName, const char *pathID)
{
	assert(filesystem);
	assert(_heapchk() == _HEAPOK);

	FileHandle_t f = filesystem->Open(resourceName, "rb", pathID);

	if (!f)
		return false;

	s_LastFileLoadingFrom = (char*)resourceName;

	int fileSize = filesystem->Size(f);
	char *buffer = (char*)MemAllocScratch(fileSize + 1);

	Assert(buffer);
	filesystem->Read(buffer, fileSize, f);
	buffer[fileSize] = 0;
	filesystem->Close(f);

	bool retOK = LoadFromBuffer(resourceName, buffer, filesystem);
	MemFreeScratch();
	return retOK;
}

bool KeyValues::SaveToFile(IBaseFileSystem *filesystem, const char *resourceName, const char *pathID)
{
	FileHandle_t f = filesystem->Open(resourceName, "wb", pathID);

	if (!f)
	{
		const char *currentFileName = resourceName;
		char szBuf[KEYVALUES_TOKEN_SIZE] = "";

		while (1)
		{
			const char *fileSlash = strchr(currentFileName, '\\');

			if (!fileSlash)
				break;

			int pathSize = fileSlash - currentFileName;

			if (szBuf[0] != 0)
				Q_strncat(szBuf, "\\", sizeof(szBuf));

			Q_strncat(szBuf, currentFileName, pathSize);
			_mkdir(szBuf);
			currentFileName += (pathSize + 1);
		}

		f = filesystem->Open(resourceName, "wb", pathID);
	}

	if (!f)
		return false;

	RecursiveSaveToFile(filesystem, f, 0);
	filesystem->Close(f);
	return true;
}

void KeyValues::WriteIndents(IBaseFileSystem *filesystem, FileHandle_t f, int indentLevel)
{
	for (int i = 0; i < indentLevel; i++)
		filesystem->Write("\t", 1, f);
}

void KeyValues::WriteIndents(CUtlBuffer &buf, int indentLevel)
{
	for (int i = 0; i < indentLevel; i++)
		buf.Printf("\t");
}

void KeyValues::WriteConvertedString(IBaseFileSystem *filesystem, FileHandle_t f, const char *pszString)
{
	int len = Q_strlen(pszString);
	char *convertedString = (char *) _alloca ((len + 1) * sizeof(char) * 2);
	int j = 0;

	for (int i = 0; i <= len; i++)
	{
		if (pszString[i] == '\"')
		{
			convertedString[j] = '\\';
			j++;
		}

		convertedString[j] = pszString[i];
		j++;
	}

	filesystem->Write(convertedString, strlen(convertedString), f);
}

void KeyValues::RecursiveSaveToFile(IBaseFileSystem *filesystem, FileHandle_t f, int indentLevel)
{
	WriteIndents(filesystem, f, indentLevel);
	filesystem->Write("\"", 1, f);
	filesystem->Write(GetName(), strlen(GetName()), f);
	filesystem->Write("\"\n", 2, f);
	WriteIndents(filesystem, f, indentLevel);
	filesystem->Write("{\n", 2, f);

	for (KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		if (dat->m_pSub)
		{
			dat->RecursiveSaveToFile(filesystem, f, indentLevel + 1);
		}
		else
		{
			switch (dat->m_iDataType)
			{
				case TYPE_STRING:
				{
					if (dat->m_sValue && *(dat->m_sValue))
					{
						WriteIndents(filesystem, f, indentLevel + 1);
						filesystem->Write("\"", 1, f);
						filesystem->Write(dat->GetName(), Q_strlen(dat->GetName()), f);
						filesystem->Write("\"\t\t\"", 4, f);

						WriteConvertedString(filesystem, f, dat->m_sValue);	

						filesystem->Write("\"\n", 2, f);
					}

					break;
				}

				case TYPE_WSTRING:
				{
					if (dat->m_wsValue)
					{
						assert(::WideCharToMultiByte(CP_UTF8, 0, dat->m_wsValue, -1, NULL, 0, NULL, NULL) < KEYVALUES_TOKEN_SIZE);

						static char buf[KEYVALUES_TOKEN_SIZE];
						int result = ::WideCharToMultiByte(CP_UTF8, 0, dat->m_wsValue, -1, buf, KEYVALUES_TOKEN_SIZE, NULL, NULL);

						if (result)
						{
							WriteIndents(filesystem, f, indentLevel + 1);
							filesystem->Write("\"", 1, f);
							filesystem->Write(dat->GetName(), Q_strlen(dat->GetName()), f);
							filesystem->Write("\"\t\t\"", 4, f);

							WriteConvertedString(filesystem, f, buf);

							filesystem->Write("\"\n", 2, f);
						}
					}

					break;
				}

				case TYPE_INT:
				{
					WriteIndents(filesystem, f, indentLevel + 1);
					filesystem->Write("\"", 1, f);
					filesystem->Write(dat->GetName(), Q_strlen(dat->GetName()), f);
					filesystem->Write("\"\t\t\"", 4, f);

					char buf[32];
					Q_snprintf(buf, 32, "%d", dat->m_iValue);

					filesystem->Write(buf, Q_strlen(buf), f);
					filesystem->Write("\"\n", 2, f);
					break;
				}

				case TYPE_FLOAT:
				{
					WriteIndents(filesystem, f, indentLevel + 1);
					filesystem->Write("\"", 1, f);
					filesystem->Write(dat->GetName(), Q_strlen(dat->GetName()), f);
					filesystem->Write("\"\t\t\"", 4, f);

					char buf[48];
					Q_snprintf(buf, 48, "%f", dat->m_flValue);

					filesystem->Write(buf, Q_strlen(buf), f);
					filesystem->Write("\"\n", 2, f);
					break;
				}

				case TYPE_COLOR:
				{
					DevMsg(1, "KeyValues::RecursiveSaveToFile: TODO, missing code for TYPE_COLOR.\n");
					break;
				}

				default: break;
			}
		}
	}

	WriteIndents(filesystem, f, indentLevel);
	filesystem->Write("}\n", 2, f);
}

KeyValues *KeyValues::FindKey(int keySymbol) const
{
	for (KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		if (dat->m_iKeyName == keySymbol)
			return dat;
	}

	return NULL;
}

KeyValues *KeyValues::FindKey(const char *keyName, bool bCreate)
{
	if (!keyName || !keyName[0])
		return this;

	char szBuf[256];
	const char *subStr = strchr(keyName, '/');
	const char *searchStr = keyName;

	if (subStr)
	{
		int size = subStr - keyName;
		Q_memcpy(szBuf, keyName, size);
		szBuf[size] = 0;
		searchStr = szBuf;
	}

	HKeySymbol iSearchStr = KeyValuesSystem()->GetSymbolForString(searchStr);
	KeyValues *lastItem = NULL;
	KeyValues *dat;

	for (dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		lastItem = dat;

		if (dat->m_iKeyName == iSearchStr)
			break;
	}

	if (!dat && m_pChain)
		dat = m_pChain->FindKey(keyName, false);

	if (!dat)
	{
		if (bCreate)
		{
			dat = new KeyValues(searchStr);

			if (lastItem)
				lastItem->m_pPeer = dat;
			else
				m_pSub = dat;

			dat->m_pPeer = NULL;
			m_iDataType = TYPE_NONE;
		}
		else
			return NULL;
	}

	if (subStr)
		return dat->FindKey(subStr + 1, bCreate);

	return dat;
}

KeyValues *KeyValues::CreateNewKey(void)
{
	int newID = 1;

	for (KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		int val = atoi(dat->GetName());

		if (newID <= val)
			newID = val + 1;
	}

	char buf[12];
	itoa(newID, buf, 10);
	return CreateKey(buf);
}

KeyValues *KeyValues::CreateKey(const char *keyName)
{
	KeyValues *dat = new KeyValues(keyName);

	dat->UsesEscapeSequences(m_bHasEscapeSequences);

	if (m_pSub == NULL)
	{
		m_pSub = dat;
	}
	else
	{
		KeyValues *pTempDat = m_pSub;

		while (pTempDat->GetNextKey() != NULL)
			pTempDat = pTempDat->GetNextKey();

		pTempDat->SetNextKey(dat);
	}

	return dat;
}

void KeyValues::AddSubKey(KeyValues *pSubkey)
{
	Assert(pSubkey->m_pPeer == NULL);

	if (m_pSub == NULL)
	{
		m_pSub = pSubkey;
	}
	else
	{
		KeyValues *pTempDat = m_pSub;

		while (pTempDat->GetNextKey() != NULL)
			pTempDat = pTempDat->GetNextKey();

		pTempDat->SetNextKey(pSubkey);
	}
}

void KeyValues::RemoveSubKey(KeyValues *subKey)
{
	if (!subKey)
		return;

	if (m_pSub == subKey)
	{
		m_pSub = subKey->m_pPeer;
	}
	else
	{
		KeyValues *kv = m_pSub;

		while (kv->m_pPeer)
		{
			if (kv->m_pPeer == subKey)
			{
				kv->m_pPeer = subKey->m_pPeer;
				break;
			}
			
			kv = kv->m_pPeer;
		}
	}

	subKey->m_pPeer = NULL;
}

KeyValues *KeyValues::GetFirstSubKey(void)
{
	return m_pSub;
}

KeyValues *KeyValues::GetNextKey(void)
{
	return m_pPeer;
}

void KeyValues::SetNextKey(KeyValues *pDat)
{
	m_pPeer = pDat;
}

int KeyValues::GetInt(const char *keyName, int defaultValue)
{
	KeyValues *dat = FindKey(keyName, false);

	if (dat)
	{
		switch (dat->m_iDataType)
		{
			case TYPE_STRING: return atoi(dat->m_sValue);
			case TYPE_WSTRING: return _wtoi(dat->m_wsValue);
			case TYPE_FLOAT: return (int)dat->m_flValue;
			case TYPE_UINT64: Assert(0); return 0;
			case TYPE_INT:
			case TYPE_PTR:
			default: return dat->m_iValue;
		}
	}

	return defaultValue;
}

uint64 KeyValues::GetUint64(const char *keyName, uint64 defaultValue)
{
	KeyValues *dat = FindKey(keyName, false);

	if (dat)
	{
		switch (dat->m_iDataType)
		{
			case TYPE_STRING: return atoi(dat->m_sValue);

			case TYPE_WSTRING:
			{
	#ifdef _WIN32
				return _wtoi(dat->m_wsValue);
	#else
				AssertFatal(0);
				return 0;
	#endif
			}

			case TYPE_FLOAT: return (int)dat->m_flValue;
			case TYPE_UINT64: return *((uint64 *)dat->m_sValue);
			case TYPE_INT:
			case TYPE_PTR:
			default: return dat->m_iValue;
		}
	}

	return defaultValue;
}

void *KeyValues::GetPtr(const char *keyName, void *defaultValue)
{
	KeyValues *dat = FindKey(keyName, false);

	if (dat)
	{
		switch (dat->m_iDataType)
		{
			case TYPE_PTR: return dat->m_pValue;
			case TYPE_WSTRING:
			case TYPE_STRING:
			case TYPE_FLOAT:
			case TYPE_INT:
			case TYPE_UINT64:
			default: return NULL;
		}
	}

	return defaultValue;
}

float KeyValues::GetFloat(const char *keyName, float defaultValue)
{
	KeyValues *dat = FindKey(keyName, false);

	if (dat)
	{
		switch (dat->m_iDataType)
		{
			case TYPE_STRING: return (float)atof(dat->m_sValue);
			case TYPE_WSTRING: return 0.0f;
			case TYPE_FLOAT: return dat->m_flValue;
			case TYPE_INT: return (float)dat->m_iValue;
			case TYPE_UINT64: (float)(*((uint64 *)dat->m_sValue));
			case TYPE_PTR:
			default: return 0.0f;
		}
	}

	return defaultValue;
}

const char *KeyValues::GetString(const char *keyName, const char *defaultValue)
{
	KeyValues *dat = FindKey(keyName, false);

	if (dat)
	{
		char buf[64];

		switch (dat->m_iDataType)
		{
			case TYPE_FLOAT:
			{
				Q_snprintf(buf, 64, "%f", dat->m_flValue);
				SetString(keyName, buf);
				break;
			}

			case TYPE_INT:
			case TYPE_PTR:
			{
				Q_snprintf(buf, 64, "%d", dat->m_iValue);
				SetString(keyName, buf);
				break;
			}

			case TYPE_UINT64:
			{
				Q_snprintf(buf, sizeof(buf), "%I64i", *((uint64 *)(dat->m_sValue)));
				SetString(keyName, buf);
				break;
			}

			case TYPE_WSTRING:
			{
				static char buf[512];
				int result = ::WideCharToMultiByte(CP_UTF8, 0, dat->m_wsValue, -1, buf, 512, NULL, NULL);

				if (result)
					SetString(keyName, buf);
				else
					return defaultValue;

				break;
			}

			case TYPE_STRING: break;
			default: return defaultValue;
		}

		return dat->m_sValue;
	}

	return defaultValue;
}

const wchar_t *KeyValues::GetWString(const char *keyName, const wchar_t *defaultValue)
{
	KeyValues *dat = FindKey(keyName, false);

	if (dat)
	{
		wchar_t wbuf[64];

		switch (dat->m_iDataType)
		{
			case TYPE_FLOAT:
			{
				swprintf(wbuf, L"%f", dat->m_flValue);
				SetWString(keyName, wbuf);
				break;
			}

			case TYPE_INT:
			case TYPE_PTR:
			{
				swprintf(wbuf, L"%d", dat->m_iValue);
				SetWString(keyName, wbuf);
				break;
			}

			case TYPE_UINT64:
			{
				swprintf(wbuf, L"%I64i", *((uint64 *)(dat->m_sValue)));
				SetWString(keyName, wbuf);
				break;
			}

			case TYPE_WSTRING: break;

			case TYPE_STRING:
			{
				static wchar_t wbuftemp[512];
				int result = ::MultiByteToWideChar(CP_UTF8, 0, dat->m_sValue, -1, wbuftemp, 512);

				if (result)
					SetWString(keyName, wbuftemp);
				else
					return defaultValue;

				break;
			}

			default: return defaultValue;
		}

		return (const wchar_t *)dat->m_wsValue;
	}

	return defaultValue;
}

Color KeyValues::GetColor(const char *keyName)
{
	Color color(0, 0, 0, 0);
	KeyValues *dat = FindKey(keyName, false);

	if (dat)
	{
		if (dat->m_iDataType == TYPE_COLOR)
		{
			color[0] = dat->m_Color[0];
			color[1] = dat->m_Color[1];
			color[2] = dat->m_Color[2];
			color[3] = dat->m_Color[3];
		}
		else if (dat->m_iDataType == TYPE_FLOAT)
		{
			color[0] = dat->m_flValue;
		}
		else if (dat->m_iDataType == TYPE_INT)
		{
			color[0] = dat->m_iValue;
		}
		else if (dat->m_iDataType == TYPE_STRING)
		{
			float a, b, c, d;
			sscanf(dat->m_sValue, "%f %f %f %f", &a, &b, &c, &d);
			color[0] = (unsigned char)a;
			color[1] = (unsigned char)b;
			color[2] = (unsigned char)c;
			color[3] = (unsigned char)d;
		}
	}

	return color;
}

void KeyValues::SetColor(const char *keyName, Color value)
{
	KeyValues *dat = FindKey(keyName, true);

	if (dat)
	{
		dat->m_iDataType = TYPE_COLOR;
		dat->m_Color[0] = value[0];
		dat->m_Color[1] = value[1];
		dat->m_Color[2] = value[2];
		dat->m_Color[3] = value[3];
	}
}

void KeyValues::SetString(const char *keyName, const char *value)
{
	KeyValues *dat = FindKey(keyName, true);

	if (dat)
	{
		if (dat->m_iDataType == TYPE_STRING || dat->m_iDataType == TYPE_WSTRING)
			delete [] dat->m_pValue;

		if (!value)
			value = "";

		int len = Q_strlen(value);
		dat->m_sValue = new char[len + 1];
		Q_memcpy(dat->m_sValue, value, len + 1);

		dat->m_iDataType = TYPE_STRING;
	}
}

void KeyValues::SetWString(const char *keyName, const wchar_t *value)
{
	KeyValues *dat = FindKey(keyName, true);

	if (dat)
	{
		if (dat->m_iDataType == TYPE_STRING || dat->m_iDataType == TYPE_WSTRING)
			delete [] dat->m_pValue;

		if (!value)
			value = L"";

		int len = wcslen(value);
		dat->m_wsValue = new wchar_t[len + 1];
		Q_memcpy(dat->m_wsValue, value, (len + 1) * sizeof(wchar_t));

		dat->m_iDataType = TYPE_WSTRING;
	}
}

void KeyValues::SetInt(const char *keyName, int value)
{
	KeyValues *dat = FindKey(keyName, true);

	if (dat)
	{
		if (dat->m_iDataType == TYPE_STRING || dat->m_iDataType == TYPE_WSTRING)
			delete [] dat->m_pValue;

		dat->m_iValue = value;
		dat->m_iDataType = TYPE_INT;
	}
}

void KeyValues::SetUint64(const char *keyName, uint64 value)
{
	KeyValues *dat = FindKey(keyName, true);

	if (dat)
	{
		if (dat->m_iDataType == TYPE_STRING || dat->m_iDataType == TYPE_WSTRING)
			delete [] dat->m_pValue;

		dat->m_sValue = new char[sizeof(uint64)];
		*((uint64 *)dat->m_sValue) = value;
		dat->m_iDataType = TYPE_UINT64;
	}
}

void KeyValues::SetFloat(const char *keyName, float value)
{
	KeyValues *dat = FindKey(keyName, true);

	if (dat)
	{
		if (dat->m_iDataType == TYPE_STRING || dat->m_iDataType == TYPE_WSTRING)
			delete [] dat->m_pValue;

		dat->m_flValue = value;
		dat->m_iDataType = TYPE_FLOAT;
	}
}

void KeyValues::SetName(const char *setName)
{
	m_iKeyName = KeyValuesSystem()->GetSymbolForString(setName);
}

void KeyValues::SetPtr(const char *keyName, void *value)
{
	KeyValues *dat = FindKey(keyName, true);

	if (dat)
	{
		if (dat->m_iDataType == TYPE_STRING || dat->m_iDataType == TYPE_WSTRING)
			delete [] dat->m_pValue;

		dat->m_pValue = value;
		dat->m_iDataType = TYPE_PTR;
	}
}

void KeyValues::RecursiveCopyKeyValues(KeyValues &src)
{
	m_iKeyName = src.GetNameSymbol();

	if (!src.m_pSub)
	{
		m_iDataType = src.m_iDataType;

		char buf[256];

		switch (src.m_iDataType)
		{
			case TYPE_NONE: break;

			case TYPE_STRING:
			{
				if (src.m_sValue)
				{
					m_sValue = new char[Q_strlen(src.m_sValue) + 1];
					Q_strcpy(m_sValue, src.m_sValue);
				}

				break;
			}

			case TYPE_INT:
			{
				m_iValue = src.m_iValue;
				Q_snprintf(buf, sizeof(buf), "%d", m_iValue);
				m_sValue = new char[strlen(buf) + 1];
				Q_strcpy(m_sValue, buf);
				break;
			}

			case TYPE_FLOAT:
			{
				m_flValue = src.m_flValue;
				Q_snprintf(buf, sizeof(buf), "%f", m_flValue);
				m_sValue = new char[strlen(buf) + 1];
				Q_strcpy(m_sValue, buf);
				break;
			}

			case TYPE_PTR:
			{
				m_pValue = src.m_pValue;
				break;
			}

			case TYPE_UINT64:
			{
				m_sValue = new char[sizeof(uint64)];
				Q_memcpy(m_sValue, src.m_sValue, sizeof(uint64));
				break;
			}

			case TYPE_COLOR:
			{
				m_Color[0] = src.m_Color[0];
				m_Color[1] = src.m_Color[1];
				m_Color[2] = src.m_Color[2];
				m_Color[3] = src.m_Color[3];
				break;
			}

			default:
			{
				Assert(0);
				break;
			}
		}
	}

	if (src.m_pSub)
	{
		m_pSub = new KeyValues(NULL);
		m_pSub->RecursiveCopyKeyValues(*src.m_pSub);
	}

	if (src.m_pPeer)
	{
		m_pPeer = new KeyValues(NULL);
		m_pPeer->RecursiveCopyKeyValues(*src.m_pPeer);
	}
}

KeyValues &KeyValues::operator = (KeyValues &src)
{
	RemoveEverything();
	RecursiveCopyKeyValues(src);
	return *this;
}

void KeyValues::CopySubkeys(KeyValues *pParent) const
{
	KeyValues *pPrev = NULL;

	for (KeyValues *sub = m_pSub; sub != NULL; sub = sub->m_pPeer)
	{
		KeyValues *dat = sub->MakeCopy();

		if (pPrev)
			pPrev->m_pPeer = dat;
		else
			pParent->m_pSub = dat;

		dat->m_pPeer = NULL;
		pPrev = dat;
	}
}

KeyValues *KeyValues::MakeCopy(void)const
{
	KeyValues *newKeyValue = new KeyValues(GetName());
	newKeyValue->m_iDataType = m_iDataType;

	switch (m_iDataType)
	{
		case TYPE_STRING:
		{
			if (m_sValue)
			{
				int len = Q_strlen(m_sValue);
				assert(!newKeyValue->m_sValue);
				newKeyValue->m_sValue = new char[len + 1];
				Q_memcpy(newKeyValue->m_sValue, m_sValue, len + 1);
			}

			break;
		}

		case TYPE_WSTRING:
		{
			if (m_wsValue)
			{
				int len = wcslen(m_wsValue);
				newKeyValue->m_wsValue = new wchar_t[len + 1];
				Q_memcpy( newKeyValue->m_wsValue, m_wsValue, (len + 1) * sizeof(wchar_t));
			}

			break;
		}

		case TYPE_INT:
		{
			newKeyValue->m_iValue = m_iValue;
			break;
		}

		case TYPE_FLOAT:
		{
			newKeyValue->m_flValue = m_flValue;
			break;
		}

		case TYPE_PTR:
		{
			newKeyValue->m_pValue = m_pValue;
			break;
		}

		case TYPE_COLOR:
		{
			newKeyValue->m_Color[0] = m_Color[0];
			newKeyValue->m_Color[1] = m_Color[1];
			newKeyValue->m_Color[2] = m_Color[2];
			newKeyValue->m_Color[3] = m_Color[3];
			break;
		}

		case TYPE_UINT64:
		{
			newKeyValue->m_sValue = new char[sizeof(uint64)];
			Q_memcpy(newKeyValue->m_sValue, m_sValue, sizeof(uint64));
			break;
		}
	}

	KeyValues *pPrev = NULL;

	for (KeyValues *sub = m_pSub; sub != NULL; sub = sub->m_pPeer)
	{
		KeyValues *dat = sub->MakeCopy();

		if (pPrev)
			pPrev->m_pPeer = dat;
		else
			newKeyValue->m_pSub = dat;

		dat->m_pPeer = NULL;
		pPrev = dat;
	}

	return newKeyValue;
}

bool KeyValues::IsEmpty(const char *keyName)
{
	KeyValues *dat = FindKey(keyName, false);

	if (!dat)
		return true;

	if (dat->m_iDataType == TYPE_NONE && dat->m_pSub == NULL)
		return true;

	return false;
}

void KeyValues::Clear(void)
{
	delete m_pSub;
	m_pSub = NULL;
	m_iDataType = TYPE_NONE;
}

KeyValues::types_t KeyValues::GetDataType(const char *keyName)
{
	KeyValues *dat = FindKey(keyName, false);

	if (dat)
		return dat->m_iDataType;

	return TYPE_NONE;
}

void KeyValues::deleteThis(void)
{
	delete this;
}

void KeyValues::AppendIncludedKeys(CUtlVector<KeyValues *> &includedKeys)
{
	int includeCount = includedKeys.Count();

	for (int i = 0; i < includeCount; i++)
	{
		KeyValues *kv = includedKeys[i];
		Assert(kv);

		KeyValues *insertSpot = this;

		while (insertSpot->GetNextKey())
			insertSpot = insertSpot->GetNextKey();

		insertSpot->SetNextKey(kv);
	}
}

void KeyValues::ParseIncludedKeys(char const *resourceName, const char *filetoinclude, IBaseFileSystem *pFileSystem, const char *pPathID, CUtlVector<KeyValues *> &includedKeys)
{
	Assert(resourceName);
	Assert(filetoinclude);
	Assert(pFileSystem);

	if (!pFileSystem)
		return;

	char fullpath[512];
	Q_strncpy(fullpath, resourceName, sizeof(fullpath));

	bool done = false;
	int len = Q_strlen(fullpath);

	while (!done)
	{
		if (len <= 0)
			break;

		if (fullpath[len - 1] == '\\' || fullpath[len - 1] == '/')
			break;

		fullpath[len - 1] = 0;
		--len;
	}

	Q_strcat(fullpath, filetoinclude, sizeof(fullpath));

	KeyValues *newKV = new KeyValues(fullpath);

	newKV->UsesEscapeSequences(m_bHasEscapeSequences);

	if (newKV->LoadFromFile(pFileSystem, fullpath, pPathID))
	{
		includedKeys.AddToTail(newKV);
	}
	else
	{
		DevMsg("KeyValues::ParseIncludedKeys: Couldn't load included keyvalue file %s\n", fullpath);
		newKV->deleteThis();
	}
}

bool KeyValues::LoadFromBuffer(char const *resourceName, const char *pBuffer, IBaseFileSystem *pFileSystem , const char *pPathID)
{
	char *pfile = const_cast<char *>(pBuffer);

	KeyValues *pPreviousKey = NULL;
	KeyValues *pCurrentKey = this;
	CUtlVector<KeyValues *> includedKeys;
	bool wasQuoted;

	while (true)
	{
		const char *s = ReadToken(&pfile, wasQuoted);
		
		if (!pfile || !s || *s == 0)
			break;

		if (!Q_stricmp(s, "#include"))
		{
			s = ReadToken(&pfile, wasQuoted);

			if (!s || *s == 0)
				DevMsg("KeyValues::LoadFromBuffer: #include is NULL in file %s\n", resourceName);
			else
				ParseIncludedKeys(resourceName, s, pFileSystem, pPathID, includedKeys);

			continue;
		}

		if (!pCurrentKey)
		{
			pCurrentKey = new KeyValues(s);
			pCurrentKey->UsesEscapeSequences(m_bHasEscapeSequences);

			if (pPreviousKey)
				pPreviousKey->SetNextKey(pCurrentKey);
		}
		else
			pCurrentKey->SetName(s);

		s = ReadToken(&pfile, wasQuoted);

		if (s && *s == '{' && !wasQuoted)
			pCurrentKey->RecursiveLoadFromBuffer(resourceName, &pfile);
		else
			DevMsg("KeyValues::LoadFromBuffer: missing { in %s\n", resourceName);

		pPreviousKey = pCurrentKey;
		pCurrentKey = NULL;
	}

	AppendIncludedKeys(includedKeys);
	return true;
}

void KeyValues::RecursiveLoadFromBuffer(char const *resourceName, char **pfile)
{
	bool wasQuoted;

	while (1)
	{
		const char *name = ReadToken(pfile, wasQuoted);

		if (!name)
			break;

		if (!*name)
		{
			Msg("KeyValues::RecursiveLoadFromBuffer:  got empty keyname in section %s of %s\n", GetName() ? GetName() : "NULL", resourceName ? resourceName : "???");
			break;
		}

		if (*name == '}' && !wasQuoted)
			break;

		KeyValues *dat = CreateKey(name);
		const char *value = ReadToken(pfile, wasQuoted);

		if (!value)
		{
			Msg("KeyValues::RecursiveLoadFromBuffer:  expecting value, got NULL in key %s of section %s in %s\n", m_pSub ? m_pSub->GetName() : "NULL", GetName() ? GetName() : "NULL", resourceName ? resourceName : "???");
			break;
		}

		if (*value == '}' && !wasQuoted)
		{
			Msg("KeyValues::RecursiveLoadFromBuffer:  expecting value, got } in key %s of section %s in %s\n", m_pSub ? m_pSub->GetName() : "NULL", GetName() ? GetName() : "NULL", resourceName ? resourceName : "???");
			break;
		}

		if (*value == '{' && !wasQuoted)
		{
			dat->RecursiveLoadFromBuffer(resourceName, pfile);
		}
		else 
		{
			if (dat->m_sValue)
				delete [] dat->m_sValue;

			int len = Q_strlen(value);
			dat->m_sValue = new char[len + 1];
			Q_memcpy(dat->m_sValue, value, len + 1);

			char *pIEnd;
			char *pFEnd;
			const char *pSEnd = value + len;

			int ival = strtol(value, &pIEnd, 10);
			float fval = (float)strtod(value, &pFEnd);

			if (*value == 0)
			{
				dat->m_iDataType = TYPE_STRING;	
			}
			else if ((18 == len) && (value[0] == '0') && (value[1] == 'x'))
			{
				int64 retVal = 0;

				for (int i = 2; i < 2 + 16; i++)
				{
					char digit = value[i];

					if (digit >= 'a')
						digit -= 'a' - ('9' + 1);
					else if (digit >= 'A')
						digit -= 'A' - ('9' + 1);

					retVal = (retVal * 16) + (digit - '0');
				}

				dat->m_sValue = new char[sizeof(uint64)];
				*((uint64 *)dat->m_sValue) = retVal;
				dat->m_iDataType = TYPE_UINT64;
			}
			else if ((pFEnd > pIEnd) && (pFEnd == pSEnd))
			{
				dat->m_flValue = fval; 
				dat->m_iDataType = TYPE_FLOAT;
			}
			else if (pIEnd == pSEnd)
			{
				dat->m_iValue = ival; 
				dat->m_iDataType = TYPE_INT;
			}
			else
				dat->m_iDataType = TYPE_STRING;
		}
	}
}

void *KeyValues::operator new(unsigned int iAllocSize)
{
	return KeyValuesSystem()->AllocKeyValuesMemory(iAllocSize);
}

void *KeyValues::operator new(unsigned int iAllocSize, int nBlockUse, const char *pFileName, int nLine)
{
	return KeyValuesSystem()->AllocKeyValuesMemory(iAllocSize);
}

void KeyValues::operator delete(void *pMem)
{
	KeyValuesSystem()->FreeKeyValuesMemory(pMem);
}

bool KeyValues::ProcessResolutionKeys(const char *pResString)
{
	if (!pResString)
		return false;

	KeyValues *pSubKey = GetFirstSubKey();

	if (!pSubKey)
		return false;

	for ( ; pSubKey != NULL; pSubKey = pSubKey->GetNextKey())
	{
		pSubKey->ProcessResolutionKeys(pResString);

		if (Q_stristr(pSubKey->GetName(), pResString) != NULL)
		{
			char normalKeyName[128];
			V_strncpy(normalKeyName, pSubKey->GetName(), sizeof(normalKeyName));

			char *pString = Q_stristr(normalKeyName, pResString);

			if (pString && !Q_stricmp(pString, pResString))
			{
				*pString = '\0';

				KeyValues *pKey = FindKey(normalKeyName);

				if (pKey)
					RemoveSubKey(pKey);

				pSubKey->SetName(normalKeyName);
			}
		}
	}

	return true;
}

bool EvaluateConditional(const char *str)
{
	return !Q_stricmp("[$X360]", str);
}