#include <interface.h>

#include <vstdlib/IKeyValuesSystem.h>

#include <tier1/KeyValues.h>
#include <tier1/mempool.h>
#include <tier1/utlsymbol.h>
#include <tier0/threadtools.h>
#include <tier1/memstack.h>

#include <vgui/ILocalize.h>

#include <tier0/memdbgon.h>

using namespace vgui;

#if 0

#ifdef NO_SBH
#define KEYVALUES_USE_POOL 1
#endif

class CKeyValuesSystem : public IKeyValuesSystem
{
public:
	CKeyValuesSystem(void);
	~CKeyValuesSystem(void);

public:
	void RegisterSizeofKeyValues(int size);

	void *AllocKeyValuesMemory(int size);
	void FreeKeyValuesMemory(void *pMem);

	HKeySymbol GetSymbolForString(const char *name);
	const char *GetStringForSymbol(HKeySymbol symbol);

	void GetLocalizedFromANSI(const char *ansi, wchar_t *outBuf, int unicodeBufferSizeInBytes);
	void GetANSIFromLocalized(const wchar_t *wchar, char *outBuf, int ansiBufferSizeInBytes);

	void AddKeyValuesToMemoryLeakList(void *pMem, HKeySymbol name);
	void RemoveKeyValuesFromMemoryLeakList(void *pMem);

private:
#ifdef KEYVALUES_USE_POOL
	CMemoryPool *m_pMemPool;
#endif
	int m_iMaxKeyValuesSize;

	CMemoryStack m_Strings;

	struct hash_item_t
	{
		int stringIndex;
		hash_item_t *next;
	};

	CMemoryPool m_HashItemMemPool;
	CUtlVector<hash_item_t> m_HashTable;

	int CaseInsensitiveHash(const char *string, int iBounds);

	struct MemoryLeakTracker_t
	{
		int nameIndex;
		void *pMem;
	};

	static bool MemoryLeakTrackerLessFunc(const MemoryLeakTracker_t &lhs, const MemoryLeakTracker_t &rhs)
	{
		return lhs.pMem < rhs.pMem;
	}

	CUtlRBTree<MemoryLeakTracker_t, int> m_KeyValuesTrackingList;

	CThreadFastMutex m_mutex;
};

//EXPOSE_SINGLE_INTERFACE(CKeyValuesSystem, IKeyValuesSystem, KEYVALUES_INTERFACE_VERSION);

static CKeyValuesSystem g_KeyValuesSystem;

IKeyValuesSystem *KeyValuesSystem(void)
{
	return &g_KeyValuesSystem;
}

CKeyValuesSystem::CKeyValuesSystem(void) : m_HashItemMemPool(sizeof(hash_item_t), 64, CMemoryPool::GROW_FAST, "CKeyValuesSystem::m_HashItemMemPool"), m_KeyValuesTrackingList(0, 0, MemoryLeakTrackerLessFunc)
{
	m_HashTable.AddMultipleToTail(2047);

	for (int i = 0; i < m_HashTable.Count(); i++)
	{
		m_HashTable[i].stringIndex = 0;
		m_HashTable[i].next = NULL;
	}

	m_Strings.Init(4 * 1024 * 1024, 64 * 1024, 0, 4);

	char *pszEmpty = ((char *)m_Strings.Alloc(1));
	*pszEmpty = 0;

#ifdef KEYVALUES_USE_POOL
	m_pMemPool = NULL;
#endif
	m_iMaxKeyValuesSize = sizeof(KeyValues);
}

CKeyValuesSystem::~CKeyValuesSystem(void)
{
#ifdef KEYVALUES_USE_POOL
#ifdef _DEBUG
	if (m_pMemPool && m_pMemPool->Count() > 0)
	{
		DevMsg("Leaked KeyValues blocks: %d\n", m_pMemPool->Count());
	}

	for (int i = 0; i < m_KeyValuesTrackingList.MaxElement(); i++)
	{
		if (m_KeyValuesTrackingList.IsValidIndex(i))
		{
			DevMsg("\tleaked KeyValues(%s)\n", &m_Strings[m_KeyValuesTrackingList[i].nameIndex]);
		}
	}
#endif

	delete m_pMemPool;
#endif
}

void CKeyValuesSystem::RegisterSizeofKeyValues(int size)
{
	if (size > m_iMaxKeyValuesSize)
	{
		m_iMaxKeyValuesSize = size;
	}
}

static void KVLeak(char const *fmt, ...)
{
	va_list argptr; 
	char data[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(data, sizeof(data), fmt, argptr);
	va_end(argptr);

	Msg(data);
}

void *CKeyValuesSystem::AllocKeyValuesMemory(int size)
{
#ifdef KEYVALUES_USE_POOL
	if (!m_pMemPool)
	{
		m_pMemPool = new CMemoryPool(m_iMaxKeyValuesSize, 1024, CMemoryPool::GROW_FAST, "CKeyValuesSystem::m_pMemPool");
		m_pMemPool->SetErrorReportFunc(KVLeak);
	}

	return m_pMemPool->Alloc(size);
#else
	return malloc(size);
#endif
}

void CKeyValuesSystem::FreeKeyValuesMemory(void *pMem)
{
#ifdef KEYVALUES_USE_POOL
	m_pMemPool->Free(pMem);
#else
	free(pMem);
#endif
}

HKeySymbol CKeyValuesSystem::GetSymbolForString(const char *name)
{
	if (!name)
	{
		return (-1);
	}

	AUTO_LOCK(m_mutex);

	int hash = CaseInsensitiveHash(name, m_HashTable.Count());
	int i = 0;
	hash_item_t *item = &m_HashTable[hash];

	while (1)
	{
		if (!stricmp(name, (char *)m_Strings.GetBase() + item->stringIndex))
			return (HKeySymbol)item->stringIndex;

		i++;

		if (item->next == NULL)
		{
			if (item->stringIndex != 0)
			{
				item->next = (hash_item_t *)m_HashItemMemPool.Alloc(sizeof(hash_item_t));
				item = item->next;
			}

			item->next = NULL;

			char *pString = (char *)m_Strings.Alloc(strlen(name) + 1);

			if (!pString)
			{
				Error("Out of keyvalue string space");
				return -1;
			}

			item->stringIndex = pString - (char *)m_Strings.GetBase();
			strcpy(pString, name);
			return (HKeySymbol)item->stringIndex;
		}

		item = item->next;
	}

	Assert(0);
	return (-1);
}

const char *CKeyValuesSystem::GetStringForSymbol(HKeySymbol symbol)
{
	if (symbol == -1)
		return "";

	return ((char *)m_Strings.GetBase() + (size_t)symbol);
}

void CKeyValuesSystem::GetLocalizedFromANSI(const char *ansi, wchar_t *outBuf, int unicodeBufferSizeInBytes)
{
//	g_pLocalize->ConvertANSIToUnicode(ansi, outBuf, unicodeBufferSizeInBytes);
}

void CKeyValuesSystem::GetANSIFromLocalized(const wchar_t *wchar, char *outBuf, int ansiBufferSizeInBytes)
{
//	g_pLocalize->ConvertUnicodeToANSI(wchar, outBuf, ansiBufferSizeInBytes);
}

void CKeyValuesSystem::AddKeyValuesToMemoryLeakList(void *pMem, HKeySymbol name)
{
#ifdef _DEBUG
	MemoryLeakTracker_t item = { name, pMem };
	m_KeyValuesTrackingList.Insert(item);
#endif
}

void CKeyValuesSystem::RemoveKeyValuesFromMemoryLeakList(void *pMem)
{
#ifdef _DEBUG
	MemoryLeakTracker_t item = { 0, pMem };
	int index = m_KeyValuesTrackingList.Find(item);
	m_KeyValuesTrackingList.RemoveAt(index);
#endif
}

int CKeyValuesSystem::CaseInsensitiveHash(const char *string, int iBounds)
{
	unsigned int hash = 0;

	for ( ; *string != 0; string++)
	{
		if (*string >= 'A' && *string <= 'Z')
		{
			hash = (hash << 1) + (*string - 'A' + 'a');
		}
		else
		{
			hash = (hash << 1) + *string;
		}
	}

	return hash % iBounds;
}

#endif

extern IKeyValuesSystem *g_pKeyValuesSystem;

IKeyValuesSystem *KeyValuesSystem(void)
{
	return g_pKeyValuesSystem;
}