#if defined (_WIN32) && !defined (_X360)
#include <windows.h>
#elif defined (_LINUX)
#include <wchar.h>
#define _alloca alloca
#endif

#include "KeyValues.h"
#include <filesystem.h>
#include <vstdlib/IKeyValuesSystem.h>

#include <vgui/ISystem.h>

#include <Color.h>
#include <assert.h>
#include <stdlib.h>
#include <direct.h>
#include <tier0/mem.h>
#include "utlvector.h"
#include "utlbuffer.h"

static char *s_LastFileLoadingFrom = "unknown";

#define KEYVALUES_TOKEN_SIZE 1024
static char s_pTokenBuf[KEYVALUES_TOKEN_SIZE];

#define INTERNALWRITE(pData, len) InternalWrite(filesystem, f, pBuf, pData, len)

const int MAX_ERROR_STACK = 64;

class CKeyValuesErrorStack
{
public:
	CKeyValuesErrorStack(void) : m_pFilename("NULL"), m_errorIndex(0), m_maxErrorIndex(0) {}

	void SetFilename(const char *pFilename)
	{
		m_pFilename = pFilename;
		m_maxErrorIndex = 0;
	}

	int Push(int symName)
	{
		if (m_errorIndex < MAX_ERROR_STACK)
			m_errorStack[m_errorIndex] = symName;

		m_errorIndex++;
		m_maxErrorIndex = max(m_maxErrorIndex, (m_errorIndex - 1));
		return m_errorIndex - 1;
	}

	void Pop(void)
	{
		m_errorIndex--;
		Assert(m_errorIndex >= 0);
	}

	void Reset(int stackLevel, int symName)
	{
		Assert(stackLevel >= 0 && stackLevel < m_errorIndex);
		m_errorStack[stackLevel] = symName;
	}

	void ReportError(const char *pError)
	{
		Warning("KeyValues Error: %s in file %s\n", pError, m_pFilename);

		for (int i = 0; i < m_maxErrorIndex; i++)
		{
			if (m_errorStack[i] != INVALID_KEY_SYMBOL)
			{
				if (i < m_errorIndex)
				{
					Warning("%s, ", KeyValuesSystem()->GetStringForSymbol(m_errorStack[i]));
				}
				else
				{
					Warning("(*%s*), ", KeyValuesSystem()->GetStringForSymbol(m_errorStack[i]));
				}
			}
		}

		Warning("\n");
	}

private:
	int m_errorStack[MAX_ERROR_STACK];
	const char *m_pFilename;
	int m_errorIndex;
	int m_maxErrorIndex;
}
g_KeyValuesErrorStack;

class CKeyErrorContext
{
public:
	CKeyErrorContext(KeyValues *pKv)
	{
		Init(pKv->GetNameSymbol());
	}

	~CKeyErrorContext(void)
	{
		g_KeyValuesErrorStack.Pop();
	}

	CKeyErrorContext(int symName)
	{
		Init(symName);
	}

	void Reset(int symName)
	{
		g_KeyValuesErrorStack.Reset(m_stackLevel, symName);
	}

private:
	void Init(int symName)
	{
		m_stackLevel = g_KeyValuesErrorStack.Push(symName);
	}

	int m_stackLevel;
};

#ifdef LEAKTRACK

class CLeakTrack
{
public:
	CLeakTrack(void)
	{
	}

	~CLeakTrack(void)
	{
		if (keys.Count() != 0)
		{
			Assert(0);
		}
	}

	struct kve
	{
		KeyValues *kv;
		char name[256];
	};

	void AddKv(KeyValues *kv, char const *name)
	{
		kve k;
		Q_strncpy(k.name, name ? name : "NULL", sizeof(k.name));
		k.kv = kv;

		keys.AddToTail(k);
	}

	void RemoveKv(KeyValues *kv)
	{
		int c = keys.Count();

		for (int i = 0; i < c; i++)
		{
			if (keys[i].kv == kv)
			{
				keys.Remove(i);
				break;
			}
		}
	}

	CUtlVector<kve> keys;
};

static CLeakTrack track;

#define TRACK_KV_ADD(ptr, name) track.AddKv(ptr, name)
#define TRACK_KV_REMOVE(ptr) track.RemoveKv(ptr)

#else

#define TRACK_KV_ADD(ptr, name)
#define TRACK_KV_REMOVE(ptr)

#endif

KeyValues::KeyValues(const char *setName)
{
	TRACK_KV_ADD(this, setName);

	Init();
	SetName(setName);
}

KeyValues::KeyValues(const char *setName, const char *firstKey, const char *firstValue)
{
	TRACK_KV_ADD(this, setName);

	Init();
	SetName(setName);
	SetString(firstKey, firstValue);
}

KeyValues::KeyValues(const char *setName, const char *firstKey, const wchar_t *firstValue)
{
	TRACK_KV_ADD(this, setName);

	Init();
	SetName(setName);
	SetWString(firstKey, firstValue);
}

KeyValues::KeyValues(const char *setName, const char *firstKey, int firstValue)
{
	TRACK_KV_ADD(this, setName);

	Init();
	SetName(setName);
	SetInt(firstKey, firstValue);
}

KeyValues::KeyValues(const char *setName, const char *firstKey, const char *firstValue, const char *secondKey, const char *secondValue)
{
	TRACK_KV_ADD(this, setName);

	Init();
	SetName(setName);
	SetString(firstKey, firstValue);
	SetString(secondKey, secondValue);
}

KeyValues::KeyValues(const char *setName, const char *firstKey, int firstValue, const char *secondKey, int secondValue)
{
	TRACK_KV_ADD(this, setName);

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
	TRACK_KV_REMOVE(this);

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
	RecursiveSaveToFile(NULL, FILESYSTEM_INVALID_HANDLE, &buf, indentLevel);
}

void KeyValues::ChainKeyValue(KeyValues *pChain)
{
	m_pChain = pChain;
}

const char *KeyValues::GetName(void) const
{
	return KeyValuesSystem()->GetStringForSymbol(m_iKeyName);
}

int KeyValues::GetNameSymbol(void) const
{
	return m_iKeyName;
}

const char *KeyValues::ReadToken(CUtlBuffer &buf, bool &wasQuoted)
{
	wasQuoted = false;

	if (!buf.IsValid())
		return NULL; 

	while (true)
	{
		buf.EatWhiteSpace();

		if (!buf.IsValid())
			return NULL;

		if (!buf.EatCPPComment())
			break;
	}

	const char *c = (const char *)buf.PeekGet(sizeof(char), 0);

	if (!c)
		return NULL;

	if (*c == '\"')
	{
		wasQuoted = true;
		buf.GetDelimitedString(m_bHasEscapeSequences ? GetCStringCharConversion() : GetNoEscCharConversion(), s_pTokenBuf, KEYVALUES_TOKEN_SIZE);
		return s_pTokenBuf;
	}

	if (*c == '{' || *c == '}')
	{
		s_pTokenBuf[0] = *c;
		s_pTokenBuf[1] = 0;
		buf.SeekGet(CUtlBuffer::SEEK_CURRENT, 1);
		return s_pTokenBuf;
	}

	bool bReportedError = false;
	int nCount = 0;

	while (c = (const char *)buf.PeekGet(sizeof(char), 0))
	{
		if (*c == 0)
			break;

		if (*c == '"' || *c == '{' || *c == '}' || *c == '[' || *c == ']')
			break;

		if (isspace(*c))
			break;

		if (nCount < (KEYVALUES_TOKEN_SIZE - 1))
		{
			s_pTokenBuf[nCount++] = *c;
		}
		else if (!bReportedError)
		{
			bReportedError = true;
			g_KeyValuesErrorStack.ReportError(" ReadToken overflow");
		}

		buf.SeekGet(CUtlBuffer::SEEK_CURRENT, 1);
	}

	s_pTokenBuf[nCount] = 0;
	return s_pTokenBuf;
}

void KeyValues::UsesEscapeSequences(bool state)
{
	m_bHasEscapeSequences = state;
}

bool KeyValues::LoadFromFile(IFileSystem *filesystem, const char *resourceName, const char *pathID)
{
	assert(filesystem);
	assert(IsX360() || (IsPC() && _heapchk() == _HEAPOK));

	FileHandle_t f = filesystem->Open(resourceName, "rb", pathID);

	if (!f)
		return false;

	s_LastFileLoadingFrom = (char *)resourceName;

	int fileSize = filesystem->Size(f);
	char *buffer = (char *)MemAllocScratch(fileSize + 1);
	Assert(buffer);

	bool bRetOK = (filesystem->Read(buffer, fileSize, f) != 0);
	filesystem->Close(f);

	if (bRetOK)
	{
		buffer[fileSize] = 0;
		bRetOK = LoadFromBuffer(resourceName, buffer, filesystem);
	}

	MemFreeScratch();
	return bRetOK;
}

bool KeyValues::SaveToFile(IFileSystem *filesystem, const char *resourceName, const char *pathID)
{
	FileHandle_t f = filesystem->Open(resourceName, "wb", pathID);

	if (f == FILESYSTEM_INVALID_HANDLE)
	{
		DevMsg(1, "KeyValues::SaveToFile: couldn't open file \"%s\" in path \"%s\".\n", resourceName ? resourceName : "NULL", pathID ? pathID : "NULL");
		return false;
	}

	RecursiveSaveToFile(filesystem, f, NULL, 0);
	filesystem->Close(f);
	return true;
}

void KeyValues::WriteIndents(IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int indentLevel)
{
	for (int i = 0; i < indentLevel; i++)
		INTERNALWRITE("\t", 1);
}

void KeyValues::WriteConvertedString(IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const char *pszString)
{
	int len = Q_strlen(pszString);
	char *convertedString = (char *)_alloca ((len + 1) * sizeof(char) * 2);
	int j = 0;

	for (int i = 0; i <= len; i++)
	{
		if (pszString[i] == '\"')
		{
			convertedString[j] = '\\';
			j++;
		}
		else if (m_bHasEscapeSequences && pszString[i] == '\\')
		{
			convertedString[j] = '\\';
			j++;
		}

		convertedString[j] = pszString[i];
		j++;
	}

	INTERNALWRITE(convertedString, strlen(convertedString));
}

void KeyValues::InternalWrite(IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const void *pData, int len)
{
	if (filesystem)
	{
		filesystem->Write(pData, len, f);
	}

	if (pBuf)
	{
		pBuf->Put(pData, len);
	}
} 

void KeyValues::RecursiveSaveToFile(IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int indentLevel)
{
	WriteIndents(filesystem, f, pBuf, indentLevel);
	INTERNALWRITE("\"", 1);
	WriteConvertedString(filesystem, f, pBuf, GetName());
	INTERNALWRITE("\"\n", 2);
	WriteIndents(filesystem, f, pBuf, indentLevel);
	INTERNALWRITE("{\n", 2);

	for (KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		if (dat->m_pSub)
		{
			dat->RecursiveSaveToFile(filesystem, f, pBuf, indentLevel + 1);
		}
		else
		{
			switch (dat->m_iDataType)
			{
				case TYPE_STRING:
				{
					if (dat->m_sValue && *(dat->m_sValue))
					{
						WriteIndents(filesystem, f, pBuf, indentLevel + 1);
						INTERNALWRITE("\"", 1);
						WriteConvertedString(filesystem, f, pBuf, dat->GetName());
						INTERNALWRITE("\"\t\t\"", 4);

						WriteConvertedString(filesystem, f, pBuf, dat->m_sValue);

						INTERNALWRITE("\"\r\n", 3);
					}

					break;
				}
#ifdef _WIN32
				case TYPE_WSTRING:
				{
					if (dat->m_wsValue)
					{
						static char buf[KEYVALUES_TOKEN_SIZE];

						assert(::WideCharToMultiByte(CP_UTF8, 0, dat->m_wsValue, -1, NULL, 0, NULL, NULL) < KEYVALUES_TOKEN_SIZE);
						int result = ::WideCharToMultiByte(CP_UTF8, 0, dat->m_wsValue, -1, buf, KEYVALUES_TOKEN_SIZE, NULL, NULL);

						if (result)
						{
							WriteIndents(filesystem, f, pBuf, indentLevel + 1);
							INTERNALWRITE("\"", 1);
							INTERNALWRITE(dat->GetName(), Q_strlen(dat->GetName()));
							INTERNALWRITE("\"\t\t\"", 4);

							WriteConvertedString(filesystem, f, pBuf, buf);

							INTERNALWRITE("\"\r\n", 3);
						}
					}
#endif
					break;
				}

				case TYPE_INT:
				{
					WriteIndents(filesystem, f, pBuf, indentLevel + 1);
					INTERNALWRITE("\"", 1);
					INTERNALWRITE(dat->GetName(), Q_strlen(dat->GetName()));
					INTERNALWRITE("\"\t\t\"", 4);

					char buf[32];
					Q_snprintf(buf, 32, "%d", dat->m_iValue);

					INTERNALWRITE(buf, Q_strlen(buf));
					INTERNALWRITE("\"\r\n", 3);
					break;
				}

				case TYPE_UINT64:
				{
					WriteIndents(filesystem, f, pBuf, indentLevel + 1);
					INTERNALWRITE("\"", 1);
					INTERNALWRITE(dat->GetName(), Q_strlen(dat->GetName()));
					INTERNALWRITE("\"\t\t\"", 4);

					char buf[32];
					Q_snprintf(buf, sizeof(buf), "0x%016I64X", *((uint64 *)dat->m_sValue));

					INTERNALWRITE(buf, Q_strlen(buf));
					INTERNALWRITE("\"\r\n", 3);
					break;
				}

				case TYPE_FLOAT:
				{
					WriteIndents(filesystem, f, pBuf, indentLevel + 1);
					INTERNALWRITE("\"", 1);
					INTERNALWRITE(dat->GetName(), Q_strlen(dat->GetName()));
					INTERNALWRITE("\"\t\t\"", 4);

					char buf[48];
					Q_snprintf(buf, 48, "%f", dat->m_flValue);

					INTERNALWRITE(buf, Q_strlen(buf));
					INTERNALWRITE("\"\r\n", 3);
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

	WriteIndents(filesystem, f, pBuf, indentLevel);
	INTERNALWRITE("}\r\n", 3);
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

	if (iSearchStr == INVALID_KEY_SYMBOL)
	{
		return NULL;
	}

	KeyValues *lastItem = NULL;
	KeyValues *dat;

	for (dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		lastItem = dat;

		if (dat->m_iKeyName == iSearchStr)
		{
			break;
		}
	}

	if (!dat && m_pChain)
	{
		dat = m_pChain->FindKey(keyName, false);
	}

	if (!dat)
	{
		if (bCreate)
		{
			dat = new KeyValues(searchStr);

			if (lastItem)
			{
				lastItem->m_pPeer = dat;
			}
			else
			{
				m_pSub = dat;
			}

			dat->m_pPeer = NULL;

			m_iDataType = TYPE_NONE;
		}
		else
		{
			return NULL;
		}
	}

	if (subStr)
	{
		return dat->FindKey(subStr + 1, bCreate);
	}

	return dat;
}

KeyValues *KeyValues::CreateNewKey(void)
{
	int newID = 1;

	for (KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		int val = atoi(dat->GetName());

		if (newID <= val)
		{
			newID = val + 1;
		}
	}

	char buf[12];
	Q_snprintf(buf, sizeof(buf), "%d", newID);

	return CreateKey(buf);
}

KeyValues *KeyValues::CreateKey(const char *keyName)
{
	KeyValues *dat = new KeyValues(keyName);

	dat->UsesEscapeSequences(m_bHasEscapeSequences != 0);

	AddSubKey(dat);

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

KeyValues *KeyValues::GetFirstTrueSubKey(void)
{
	KeyValues *pRet = m_pSub;

	while (pRet && pRet->m_iDataType != TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

KeyValues *KeyValues::GetNextTrueSubKey(void)
{
	KeyValues *pRet = m_pPeer;

	while (pRet && pRet->m_iDataType != TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

KeyValues *KeyValues::GetFirstValue(void)
{
	KeyValues *pRet = m_pSub;

	while (pRet && pRet->m_iDataType == TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

KeyValues *KeyValues::GetNextValue(void)
{
	KeyValues *pRet = m_pPeer;

	while (pRet && pRet->m_iDataType == TYPE_NONE)
		pRet = pRet->m_pPeer;

	return pRet;
}

int KeyValues::GetInt(const char *keyName, int defaultValue)
{
	KeyValues *dat = FindKey(keyName, false);

	if (dat)
	{
		switch (dat->m_iDataType)
		{
			case TYPE_STRING:
			{
				return atoi(dat->m_sValue);
			}

			case TYPE_WSTRING:
			{
#ifdef _WIN32
				return _wtoi(dat->m_wsValue);
#else
			DevMsg("TODO: implement _wtoi\n");
			return 0;
#endif
			}

			case TYPE_FLOAT:
			{
				return (int)dat->m_flValue;
			}

			case TYPE_UINT64:
			{
				Assert(0);
				return 0;
			}

			case TYPE_INT:
			case TYPE_PTR:
			default:
			{
				return dat->m_iValue;
			}
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
			case TYPE_STRING:
			{
				return atoi(dat->m_sValue);
			}

			case TYPE_WSTRING:
			{
#ifdef _WIN32
				return _wtoi(dat->m_wsValue);
#else
				AssertFatal(0);
				return 0;
#endif
			}

			case TYPE_FLOAT:
			{
				return (int)dat->m_flValue;
			}

			case TYPE_UINT64:
			{
				return *((uint64 *)dat->m_sValue);
			}

			case TYPE_INT:
			case TYPE_PTR:

			default:
			{
				return dat->m_iValue;
			}
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
			case TYPE_PTR:
			{
				return dat->m_pValue;
			}

			case TYPE_WSTRING:
			case TYPE_STRING:
			case TYPE_FLOAT:
			case TYPE_INT:
			case TYPE_UINT64:
			default:
			{
				return NULL;
			}
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
			case TYPE_STRING:
			{
				return (float)atof(dat->m_sValue);
			}

			case TYPE_WSTRING:
			{
				return 0.0f;
			}

			case TYPE_FLOAT:
			{
				return dat->m_flValue;
			}

			case TYPE_INT:
			{
				return (float)dat->m_iValue;
			}

			case TYPE_UINT64:
			{
				return (float)(*((uint64 *)dat->m_sValue));
			}

			case TYPE_PTR:
			default:
			{
				return 0.0f;
			}
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
#ifdef _WIN32
				static char buf[512];
				int result = ::WideCharToMultiByte(CP_UTF8, 0, dat->m_wsValue, -1, buf, 512, NULL, NULL);

				if (result)
				{
					SetString(keyName, buf);
				}
				else
				{
					return defaultValue;
				}
#endif
				break;
			}

			case TYPE_STRING:
			{
				break;
			}

			default:
			{
				return defaultValue;
			}
		}

		return dat->m_sValue;
	}

	return defaultValue;
}

const wchar_t *KeyValues::GetWString(const char *keyName, const wchar_t *defaultValue)
{
	KeyValues *dat = FindKey(keyName, false);
#ifdef _WIN32
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

			case TYPE_WSTRING:
			{
				break;
			}

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
#else
	DevMsg("TODO: implement wide char functions\n");
#endif
	return defaultValue;
}

bool KeyValues::GetBool(const char *keyName, bool defaultValue)
{
	if (FindKey(keyName))
		return 0 != GetInt(keyName, 0);

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

void KeyValues::SetStringValue(char const *strValue)
{
	if (m_iDataType == TYPE_STRING || m_iDataType == TYPE_WSTRING)
		delete [] m_pValue;

	if (!strValue)
		strValue = "";

	int len = Q_strlen(strValue);
	m_sValue = new char[len + 1];
	Q_memcpy(m_sValue, strValue, len + 1);

	m_iDataType = TYPE_STRING;
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
			case TYPE_NONE:
			{
				break;
			}

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
	Init();
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

KeyValues *KeyValues::MakeCopy(void) const
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
				Q_memcpy(newKeyValue->m_wsValue, m_wsValue, (len + 1) * sizeof(wchar_t));
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

	CopySubkeys(newKeyValue);
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
		{
			insertSpot = insertSpot->GetNextKey();
		}

		insertSpot->SetNextKey(kv);
	}
}

void KeyValues::ParseIncludedKeys(char const *resourceName, const char *filetoinclude, IFileSystem *pFileSystem, const char *pPathID, CUtlVector<KeyValues *> &includedKeys)
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

	Q_strncat(fullpath, filetoinclude, sizeof(fullpath), COPY_ALL_CHARACTERS);

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

void KeyValues::MergeBaseKeys(CUtlVector<KeyValues *> &baseKeys)
{
	int includeCount = baseKeys.Count();

	for (int i = 0; i < includeCount; i++)
	{
		KeyValues *kv = baseKeys[i];
		Assert(kv);

		RecursiveMergeKeyValues(kv);
	}
}

void KeyValues::RecursiveMergeKeyValues(KeyValues *baseKV)
{
	for (KeyValues *baseChild = baseKV->m_pSub; baseChild != NULL; baseChild = baseChild->m_pPeer)
	{
		bool bFoundMatch = false;

		for (KeyValues *newChild = m_pSub; newChild != NULL; newChild = newChild->m_pPeer)
		{
			if (!Q_strcmp(baseChild->GetName(), newChild->GetName()))
			{
				newChild->RecursiveMergeKeyValues(baseChild);
				bFoundMatch = true;
				break;
			}
		}

		if (!bFoundMatch)
		{
			KeyValues *dat = baseChild->MakeCopy();
			Assert(dat);
			AddSubKey(dat);
		}
	}
}

bool KeyValues::LoadFromBuffer(char const *resourceName, CUtlBuffer &buf, IFileSystem *pFileSystem, const char *pPathID)
{
	KeyValues *pPreviousKey = NULL;
	KeyValues *pCurrentKey = this;
	CUtlVector<KeyValues *> includedKeys;
	CUtlVector<KeyValues *> baseKeys;
	bool wasQuoted;

	g_KeyValuesErrorStack.SetFilename(resourceName);

	do
	{
		const char *s = ReadToken(buf, wasQuoted);

		if (!buf.IsValid() || !s || *s == 0)
			break;

		if (!Q_stricmp(s, "#include"))
		{
			s = ReadToken(buf, wasQuoted);

			if (!s || *s == 0)
			{
				g_KeyValuesErrorStack.ReportError("#include is NULL ");
			}
			else
			{
				ParseIncludedKeys(resourceName, s, pFileSystem, pPathID, includedKeys);
			}

			continue;
		}
		else if (!Q_stricmp(s, "#base"))
		{
			s = ReadToken(buf, wasQuoted);

			if (!s || *s == 0)
			{
				g_KeyValuesErrorStack.ReportError("#base is NULL ");
			}
			else
			{
				ParseIncludedKeys(resourceName, s, pFileSystem, pPathID, baseKeys);
			}

			continue;
		}

		if (!pCurrentKey)
		{
			pCurrentKey = new KeyValues(s);
			Assert(pCurrentKey);

			pCurrentKey->UsesEscapeSequences(m_bHasEscapeSequences != 0);

			if (pPreviousKey)
			{
				pPreviousKey->SetNextKey(pCurrentKey);
			}
		}
		else
		{
			pCurrentKey->SetName(s);
		}

		s = ReadToken(buf, wasQuoted);

		if (s && *s == '{' && !wasQuoted)
		{
			pCurrentKey->RecursiveLoadFromBuffer(resourceName, buf);
		}
		else
		{
			g_KeyValuesErrorStack.ReportError("LoadFromBuffer: missing {");
		}

		pPreviousKey = pCurrentKey;
		pCurrentKey = NULL;

	}
	while (buf.IsValid());

	AppendIncludedKeys(includedKeys);
	{
		for (int i = includedKeys.Count() - 1; i > 0; i--)
		{
			KeyValues *kv = includedKeys[i];
			kv->deleteThis();
		}
	}

	MergeBaseKeys(baseKeys);
	{
		for (int i = baseKeys.Count() - 1; i >= 0; i--)
		{
			KeyValues *kv = baseKeys[i];
			kv->deleteThis();
		}
	}

	g_KeyValuesErrorStack.SetFilename("");
	return true;
}

bool KeyValues::LoadFromBuffer(char const *resourceName, const char *pBuffer, IFileSystem* pFileSystem, const char *pPathID)
{
	if (!pBuffer)
		return true;

	int nLen = Q_strlen(pBuffer);
	CUtlBuffer buf(pBuffer, nLen, CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER);
	return LoadFromBuffer(resourceName, buf, pFileSystem, pPathID);
}

void KeyValues::RecursiveLoadFromBuffer(char const *resourceName, CUtlBuffer &buf)
{
	CKeyErrorContext errorReport(this);
	bool wasQuoted;
	CKeyErrorContext errorKey(INVALID_KEY_SYMBOL);

	while (1)
	{
		const char *name = ReadToken(buf, wasQuoted);

		if (!name)
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got EOF instead of keyname");
			break;
		}

		if (!*name)
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got empty keyname");
			break;
		}

		if (*name == '}' && !wasQuoted )
			break;

		KeyValues *dat = CreateKey(name);

		errorKey.Reset(dat->GetNameSymbol());

		const char *value = ReadToken(buf, wasQuoted);

		if (!value)
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got NULL key");
			break;
		}

		if (*value == '}' && !wasQuoted)
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got } in key");
			break;
		}

		if (*value == '{' && !wasQuoted)
		{
			errorKey.Reset(INVALID_KEY_SYMBOL);
			dat->RecursiveLoadFromBuffer(resourceName, buf);
		}
		else 
		{
			if (dat->m_sValue)
			{
				delete [] dat->m_sValue;
				dat->m_sValue = NULL;
			}

			int len = Q_strlen(value);

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

				dat->m_sValue = new char [sizeof(uint64)];
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
			{
				dat->m_iDataType = TYPE_STRING;
			}

			if (dat->m_iDataType == TYPE_STRING)
			{
				dat->m_sValue = new char[len + 1];
				Q_memcpy(dat->m_sValue, value, len + 1);
			}
		}
	}
}

bool KeyValues::WriteAsBinary(CUtlBuffer &buffer)
{
	if (buffer.IsText())
		return false;

	if (!buffer.IsValid())
		return false;

	for (KeyValues *dat = this; dat != NULL; dat = dat->m_pPeer)
	{
		buffer.PutUnsignedChar(dat->m_iDataType);
		buffer.PutString(dat->GetName());

		switch (dat->m_iDataType)
		{
			case TYPE_NONE:
			{
				dat->m_pSub->WriteAsBinary(buffer);
				break;
			}

			case TYPE_STRING:
			{
				if (dat->m_sValue && *(dat->m_sValue))
				{
					buffer.PutString(dat->m_sValue);
				}
				else
				{
					buffer.PutString("");
				}

				break;
			}

			case TYPE_WSTRING:
			{
				Assert(!"TYPE_WSTRING");
				break;
			}

			case TYPE_INT:
			{
				buffer.PutInt(dat->m_iValue);
				break;
			}

			case TYPE_UINT64:
			{
				buffer.PutDouble(*((double *)dat->m_sValue));
				break;
			}

			case TYPE_FLOAT:
			{
				buffer.PutFloat(dat->m_flValue);
				break;
			}

			case TYPE_COLOR:
			{
				buffer.PutUnsignedChar(dat->m_Color[0]);
				buffer.PutUnsignedChar(dat->m_Color[1]);
				buffer.PutUnsignedChar(dat->m_Color[2]);
				buffer.PutUnsignedChar(dat->m_Color[3]);
				break;
			}

			case TYPE_PTR:
			{
				buffer.PutUnsignedInt((int)dat->m_pValue);
			}

			default:
			{
				break;
			}
		}
	}

	buffer.PutUnsignedChar(TYPE_NUMTYPES); 
	return buffer.IsValid();
}

bool KeyValues::ReadAsBinary(CUtlBuffer &buffer)
{
	if (buffer.IsText())
		return false;

	if (!buffer.IsValid())
		return false;

	RemoveEverything();
	Init();

	char token[KEYVALUES_TOKEN_SIZE];
	KeyValues *dat = this;
	types_t type = (types_t)buffer.GetUnsignedChar();

	while (true)
	{
		if (type == TYPE_NUMTYPES)
			break;

		dat->m_iDataType = type;

		buffer.GetString(token, KEYVALUES_TOKEN_SIZE - 1);
		token[KEYVALUES_TOKEN_SIZE - 1] = 0;

		dat->SetName(token);

		switch (type)
		{
			case TYPE_NONE:
			{
				dat->m_pSub = new KeyValues("");
				dat->m_pSub->ReadAsBinary(buffer);
				break;
			}

			case TYPE_STRING:
			{
				buffer.GetString(token, KEYVALUES_TOKEN_SIZE - 1);
				token[KEYVALUES_TOKEN_SIZE - 1] = 0;

				int len = Q_strlen(token);
				dat->m_sValue = new char[len + 1];
				Q_memcpy(dat->m_sValue, token, len + 1);

				break;
			}

			case TYPE_WSTRING:
			{
				Assert(!"TYPE_WSTRING");
				break;
			}

			case TYPE_INT:
			{
				dat->m_iValue = buffer.GetInt();
				break;
			}

			case TYPE_UINT64:
			{
				dat->m_sValue = new char[sizeof(uint64)];
				*((double *)dat->m_sValue) = buffer.GetDouble();
			}

			case TYPE_FLOAT:
			{
				dat->m_flValue = buffer.GetFloat();
				break;
			}

			case TYPE_COLOR:
			{
				dat->m_Color[0] = buffer.GetUnsignedChar();
				dat->m_Color[1] = buffer.GetUnsignedChar();
				dat->m_Color[2] = buffer.GetUnsignedChar();
				dat->m_Color[3] = buffer.GetUnsignedChar();
				break;
			}

			case TYPE_PTR:
			{
				dat->m_pValue = (void *)buffer.GetUnsignedInt();
			}

			default:
			{
				break;
			}
		}

		if (!buffer.IsValid())
			return false;

		type = (types_t)buffer.GetUnsignedChar();

		if (type == TYPE_NUMTYPES)
			break;

		dat->m_pPeer = new KeyValues("");
		dat = dat->m_pPeer;
	}

	return buffer.IsValid();
}

#include <tier0/memdbgoff.h>

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

void KeyValues::UnpackIntoStructure(KeyValuesUnpackStructure const *pUnpackTable, void *pDest)
{
	uint8 *dest = (uint8 *) pDest;

	while (pUnpackTable->m_pKeyName)
	{
		uint8 *dest_field = dest + pUnpackTable->m_nFieldOffset;
		KeyValues *find_it = FindKey(pUnpackTable->m_pKeyName);

		switch (pUnpackTable->m_eDataType)
		{
			case UNPACK_TYPE_FLOAT:
			{
				float default_value = (pUnpackTable->m_pKeyDefault) ? atof(pUnpackTable->m_pKeyDefault) : 0.0;
				*((float *) dest_field) = GetFloat(pUnpackTable->m_pKeyName, default_value);
				break;
			}

			case UNPACK_TYPE_VECTOR:
			{
				float *dest_v = (float *)dest_field;
				char const *src_string = GetString(pUnpackTable->m_pKeyName, pUnpackTable->m_pKeyDefault);

				if ((!src_string) || (sscanf(src_string, "%f %f %f", &(dest_v[0]), &(dest_v[1]), &(dest_v[2])) != 3))
					memset(dest_v, 0, 3 * sizeof(float));

				break;
			}

			case UNPACK_TYPE_FOUR_FLOATS:
			{
				float *dest_f = (float *)dest_field;
				char const *src_string = GetString(pUnpackTable->m_pKeyName, pUnpackTable->m_pKeyDefault);

				if ((!src_string) || (sscanf(src_string, "%f %f %f %f", dest_f, dest_f + 1, dest_f + 2, dest_f + 3)) != 4)
					memset(dest_f, 0, 4 * sizeof(float));

				break;
			}

			case UNPACK_TYPE_TWO_FLOATS:
			{
				float *dest_f = (float *)dest_field;
				char const *src_string = GetString(pUnpackTable->m_pKeyName, pUnpackTable->m_pKeyDefault);

				if ((!src_string) || (sscanf(src_string,"%f %f", dest_f, dest_f + 1)) != 2)
					memset(dest_f, 0, 2 * sizeof(float));

				break;
			}

			case UNPACK_TYPE_STRING:
			{
				char *dest_s = (char *)dest_field;
				strncpy(dest_s, GetString(pUnpackTable->m_pKeyName, pUnpackTable->m_pKeyDefault), pUnpackTable->m_nFieldSize);
				break;
			}

			case UNPACK_TYPE_INT:
			{
				int *dest_i = (int *)dest_field;
				int default_int = 0;

				if (pUnpackTable->m_pKeyDefault)
					default_int = atoi(pUnpackTable->m_pKeyDefault);

				*(dest_i) = GetInt(pUnpackTable->m_pKeyName, default_int);
				break;
			}

			case UNPACK_TYPE_VECTOR_COLOR:
			{
				float *dest_v = (float *)dest_field;

				if (find_it)
				{
					Color c = GetColor(pUnpackTable->m_pKeyName);
					dest_v[0] = c.r();
					dest_v[1] = c.g();
					dest_v[2] = c.b();
				}
				else
				{
					if (pUnpackTable->m_pKeyDefault)
						sscanf(pUnpackTable->m_pKeyDefault, "%f %f %f", &(dest_v[0]), &(dest_v[1]), &(dest_v[2]));
					else
						memset(dest_v, 0, 3 * sizeof(float));
				}

				*(dest_v) *= (1.0 / 255);
			}
		}

		pUnpackTable++;
	}
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
				{
					RemoveSubKey(pKey);
				}

				pSubKey->SetName(normalKeyName);
			}
		}
	}

	return true;
}

bool EvaluateConditional( const char *str )
{
	bool bResult = false;
	bool bXboxUI = IsX360();

	if ( bXboxUI )
	{
		bResult = !Q_stricmp( "[$X360]", str );
	}
	else
	{
		bResult = !Q_stricmp( "[$WIN32]", str );
	}

	return bResult;
}