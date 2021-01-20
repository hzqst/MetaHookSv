#ifndef KEYVALUES_H
#define KEYVALUES_H

#ifdef _WIN32
#pragma once
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

#include "utlvector.h"
#include "FileSystem.h"

class IBaseFileSystem;
class CUtlBuffer;
class Color;
typedef void *FileHandle_t;

class KeyValues
{
public:
	KeyValues(KeyValues &);
	KeyValues(const char *setName);
	KeyValues(const char *setName, const char *firstKey, const char *firstValue);
	KeyValues(const char *setName, const char *firstKey, const wchar_t *firstValue);
	KeyValues(const char *setName, const char *firstKey, int firstValue);
	KeyValues(const char *setName, const char *firstKey, const char *firstValue, const char *secondKey, const char *secondValue);
	KeyValues(const char *setName, const char *firstKey, int firstValue, const char *secondKey, int secondValue);
	~KeyValues(void);

public:
	enum types_t
	{
		TYPE_NONE,
		TYPE_STRING,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_PTR,
		TYPE_WSTRING,
		TYPE_COLOR,
		TYPE_UINT64,
	};

public:
	virtual const char *GetName(void)const;
	virtual int GetNameSymbol(void)const;
	virtual bool LoadFromFile(IFileSystem *filesystem, const char *resourceName, const char *pathID = NULL);
	virtual bool SaveToFile(IFileSystem *filesystem, const char *resourceName, const char *pathID = NULL);
	virtual KeyValues *FindKey(const char *keyName, bool bCreate = false);
	virtual KeyValues *FindKey(int keySymbol) const;
	virtual KeyValues *CreateNewKey(void);
	virtual void RemoveSubKey(KeyValues *subKey);
	virtual KeyValues *GetFirstSubKey(void);
	virtual KeyValues *GetNextKey(void);
	virtual int GetInt(const char *keyName = NULL, int defaultValue = 0);
	virtual float GetFloat(const char *keyName = NULL, float defaultValue = 0.0);
	virtual const char *GetString(const char *keyName = NULL, const char *defaultValue = "");
	virtual const wchar_t *GetWString(const char *keyName = NULL, const wchar_t *defaultValue = L"");
	virtual void *GetPtr(const char *keyName = NULL, void *defaultValue = NULL);
	Color GetColor(const char *keyName = NULL);
	virtual bool IsEmpty(const char *keyName = NULL);
	virtual void SetWString(const char *keyName, const wchar_t *value);
	virtual void SetString(const char *keyName, const char *value);
	virtual void SetInt(const char *keyName, int value);
	virtual void SetFloat(const char *keyName, float value);
	virtual void SetPtr(const char *keyName, void *value);
	void SetColor(const char *keyName, Color value);
	virtual KeyValues *MakeCopy(void)const;
	virtual void Clear(void);
	virtual types_t GetDataType(const char *keyName = NULL);
	virtual void deleteThis(void);

public:
	void SetName(const char *setName);
	void UsesEscapeSequences(bool state);
	bool LoadFromBuffer(char const *resourceName, const char *pBuffer, IBaseFileSystem* pFileSystem = NULL, const char *pPathID = NULL);
	void AddSubKey(KeyValues *pSubkey);
	void SetNextKey(KeyValues *pDat);
	void ChainKeyValue(KeyValues *pChain);
	void RecursiveSaveToFile(CUtlBuffer &buf, int indentLevel);
	bool ProcessResolutionKeys(const char *pResString);
	uint64 GetUint64(const char *keyName = NULL, uint64 defaultValue = 0);
	void SetUint64(const char *keyName, uint64 value);
	void CopySubkeys(KeyValues *pParent) const;

public:
	void *operator new(unsigned int iAllocSize);
	void *operator new(unsigned int iAllocSize, int nBlockUse, const char *pFileName, int nLine);
	void operator delete(void *pMem);
	KeyValues &operator = (KeyValues &src);

private:
	KeyValues *CreateKey(const char *keyName);	
	void RecursiveCopyKeyValues(KeyValues &src);
	void RemoveEverything(void);
	void RecursiveSaveToFile(IBaseFileSystem *filesystem, FileHandle_t f, int indentLevel);
	void WriteConvertedString(IBaseFileSystem *filesystem, FileHandle_t f, const char *pszString);
	void RecursiveLoadFromBuffer( char const *resourceName, char **pfile );
	void AppendIncludedKeys(CUtlVector<KeyValues *> &includedKeys);
	void ParseIncludedKeys(char const *resourceName, const char *filetoinclude, IBaseFileSystem *pFileSystem, const char *pPathID, CUtlVector<KeyValues *> &includedKeys);
	void Init(void);
	const char *ReadToken(char **buffer, bool &wasQuoted);
	void WriteIndents(IBaseFileSystem *filesystem, FileHandle_t f, int indentLevel);
	void WriteIndents(CUtlBuffer &buf, int indentLevel);

private:
	int m_iKeyName;

	union
	{
		int m_iValue;
		float m_flValue;
		void *m_pValue;
		unsigned char m_Color[4];
		char *m_sValue;
		wchar_t *m_wsValue;
	};

	types_t m_iDataType;
	KeyValues *m_pPeer;
	KeyValues *m_pSub;
	KeyValues *m_pChain;
	bool m_bHasEscapeSequences;
};

bool EvaluateConditional(const char *str);
#endif