#ifndef KEYVALUES_H
#define KEYVALUES_H

#ifdef _WIN32
#pragma once
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#include "utlvector.h"
#include <filesystem.h>
#include <Color.h>
#include <IBaseKeyValues.h>

class IFileSystem;
class CUtlBuffer;
class Color;
typedef void *FileHandle_t;

class KeyValues : public IBaseKeyValues
{
public:
	//KeyValues(KeyValues &);
	KeyValues(const char *setName);
	KeyValues(const char *setName, const char *firstKey, const char *firstValue);
	KeyValues(const char *setName, const char *firstKey, const wchar_t *firstValue);
	KeyValues(const char *setName, const char *firstKey, int firstValue);
	KeyValues(const char *setName, const char *firstKey, const char *firstValue, const char *secondKey, const char *secondValue);
	KeyValues(const char *setName, const char *firstKey, int firstValue, const char *secondKey, int secondValue);
	~KeyValues(void);

public:

#if 0
	class AutoDelete
	{
	public:
		explicit inline AutoDelete(KeyValues *pKeyValues) : m_pKeyValues(pKeyValues)
		{
		}

		inline ~AutoDelete(void)
		{
			if (m_pKeyValues)
				m_pKeyValues->deleteThis();
		}

		inline void Assign(KeyValues *pKeyValues)
		{
			m_pKeyValues = pKeyValues;
		}

	private:
		AutoDelete(AutoDelete const &x);
		AutoDelete & operator = (AutoDelete const &x);
		KeyValues *m_pKeyValues;
	};
#endif

public:
	const char* GetName(void) const override;
	int GetNameSymbol(void) const override;
	bool LoadFromFile(IFileSystem *filesystem, const char *resourceName, const char *pathID = NULL) override;
	bool SaveToFile(IFileSystem *filesystem, const char *resourceName, const char *pathID = NULL) override;
	KeyValues* FindKey2(int keySymbol) const override;
	KeyValues *FindKey(const char *keyName, bool bCreate = false) override;
	KeyValues *CreateNewKey(void) override;
	void RemoveSubKey(KeyValues *subKey) override;
	KeyValues *GetFirstSubKey(void) override;
	KeyValues *GetNextKey(void) override;
	int GetInt(const char *keyName = NULL, int defaultValue = 0) override;
	float GetFloat(const char *keyName = NULL, float defaultValue = 0.0) override;
	const char *GetString(const char *keyName = NULL, const char *defaultValue = "") override;
	const wchar_t *GetWString(const char *keyName = NULL, const wchar_t *defaultValue = L"") override;
	void *GetPtr(const char *keyName = NULL, void *defaultValue = NULL) override;
	bool IsEmpty(const char *keyName = NULL) override;
	void SetWString(const char *keyName, const wchar_t *value) override;
	void SetString(const char *keyName, const char *value) override;
	void SetInt(const char *keyName, int value) override;
	void SetFloat(const char *keyName, float value) override;
	void SetPtr(const char *keyName, void *value) override;
	KeyValues *MakeCopy(void) const;
	void Clear(void) override;
	types_t GetDataType(const char *keyName = NULL) override;
	void deleteThis(void) override;

public:
	void SetName(const char *setName);
	void UsesEscapeSequences(bool state);
	bool LoadFromBuffer(char const *resourceName, const char *pBuffer, IFileSystem *pFileSystem = NULL, const char *pPathID = NULL);
	bool LoadFromBuffer(char const *resourceName, CUtlBuffer &buf, IFileSystem *pFileSystem = NULL, const char *pPathID = NULL);
	void AddSubKey(KeyValues *pSubkey);
	bool AddSubKeyAfter(KeyValues* pSubkey, KeyValues* pAfterKey);
	bool AddSubKeyBefore(KeyValues* pSubkey, KeyValues* pAfterKey);
	void SetNextKey(KeyValues *pDat);
	void ChainKeyValue(KeyValues *pChain);
	void RecursiveSaveToFile(CUtlBuffer &buf, int indentLevel);
	bool WriteAsBinary(CUtlBuffer &buffer);
	bool ReadAsBinary(CUtlBuffer &buffer);
	void SetStringValue(char const *strValue);
	void UnpackIntoStructure(struct KeyValuesUnpackStructure const *pUnpackTable, void *pDest);
	bool ProcessResolutionKeys(const char *pResString);
	uint64 GetUint64(const char *keyName = NULL, uint64 defaultValue = 0);
	void SetUint64(const char *keyName, uint64 value);
	int GetInt(int keySymbol, int defaultValue = 0);
	float GetFloat(int keySymbol, float defaultValue = 0.0f);
	const char *GetString(int keySymbol, const char *defaultValue = "");
	const wchar_t *GetWString(int keySymbol, const wchar_t *defaultValue = L"");
	void *GetPtr(int keySymbol, void *defaultValue = (void *)0);
	bool GetBool(const char *keyName = NULL, bool defaultValue = false);
	Color GetColor(const char *keyName = NULL);
	Color GetColor(int keySymbol);
	bool IsEmpty(int keySymbol);
	void SetBool(const char *keyName, bool value) { SetInt(keyName, value ? 1 : 0); }
	void SetColor(const char *keyName, Color value);
	void CopySubkeys(KeyValues *pParent) const;
	KeyValues *GetFirstTrueSubKey(void);
	KeyValues *GetNextTrueSubKey(void);
	KeyValues *GetFirstValue(void);
	KeyValues *GetNextValue(void);

public:
	void *operator new(unsigned int iAllfParseIncludedKeysocSize);
	void *operator new(unsigned int iAllocSize, int nBlockUse, const char *pFileName, int nLine);
	void operator delete(void *pMem);
	KeyValues &operator = (KeyValues &src);

private:
	KeyValues *CreateKey(const char *keyName);
	void RecursiveCopyKeyValues(KeyValues &src);
	void RemoveEverything(void);
	void RecursiveSaveToFile(IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int indentLevel);
	void WriteConvertedString(IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const char *pszString);
	void RecursiveLoadFromBuffer(char const *resourceName, CUtlBuffer &buf);
	void AppendIncludedKeys(CUtlVector<KeyValues *> &includedKeys);
	void ParseIncludedKeys(char const *resourceName, const char *filetoinclude, IFileSystem *pFileSystem, const char *pPathID, CUtlVector<KeyValues *> &includedKeys);
	void MergeBaseKeys(CUtlVector<KeyValues *> &baseKeys);
	void RecursiveMergeKeyValues(KeyValues *baseKV);
	void InternalWrite(IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const void *pData, int len);
	void Init(void);
	const char *ReadToken(CUtlBuffer &buf, bool &wasQuoted);
	void WriteIndents(IFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int indentLevel);
	void FreeAllocatedValue(void);
	void AllocateValueBlock(int size);

private:

	//GoldSrc stuffs
	int m_iKeyName;//+4

	union
	{
		int m_iValue;
		float m_flValue;
		void *m_pValue;
		unsigned char m_Color[4];
		char *m_sValue;
		wchar_t *m_wsValue;
	};//+8h

	types_t m_iDataType;//+Ch

	KeyValues *m_pPeer;//+10h
	KeyValues *m_pSub;//+14h

	//Source stuffs
	KeyValues *m_pChain;

	char m_bHasEscapeSequences;
};

enum KeyValuesUnpackDestinationTypes_t
{
	UNPACK_TYPE_FLOAT,
	UNPACK_TYPE_VECTOR,
	UNPACK_TYPE_VECTOR_COLOR,
	UNPACK_TYPE_STRING,
	UNPACK_TYPE_INT,
	UNPACK_TYPE_FOUR_FLOATS,
	UNPACK_TYPE_TWO_FLOATS,
};

#define UNPACK_FIXED(kname, kdefault, dtype, ofs) { kname, kdefault, dtype, ofs, 0 }
#define UNPACK_VARIABLE(kname, kdefault, dtype, ofs, sz) { kname, kdefault, dtype, ofs, sz }
#define UNPACK_END_MARKER { NULL, NULL, UNPACK_TYPE_FLOAT, 0 }

struct KeyValuesUnpackStructure
{
	char const *m_pKeyName;
	char const *m_pKeyDefault;
	KeyValuesUnpackDestinationTypes_t m_eDataType;
	size_t m_nFieldOffset;
	size_t m_nFieldSize;
};

inline int KeyValues::GetInt(int keySymbol, int defaultValue)
{
	KeyValues *dat = FindKey2(keySymbol);
	return dat ? dat->GetInt((const char *)NULL, defaultValue) : defaultValue;
}

inline float KeyValues::GetFloat(int keySymbol, float defaultValue)
{
	KeyValues *dat = FindKey2(keySymbol);
	return dat ? dat->GetFloat((const char *)NULL, defaultValue) : defaultValue;
}

inline const char *KeyValues::GetString(int keySymbol, const char *defaultValue)
{
	KeyValues *dat = FindKey2(keySymbol);
	return dat ? dat->GetString((const char *)NULL, defaultValue) : defaultValue;
}

inline const wchar_t *KeyValues::GetWString(int keySymbol, const wchar_t *defaultValue)
{
	KeyValues *dat = FindKey2(keySymbol);
	return dat ? dat->GetWString((const char *)NULL, defaultValue) : defaultValue;
}

inline void *KeyValues::GetPtr(int keySymbol, void *defaultValue)
{
	KeyValues *dat = FindKey2(keySymbol);
	return dat ? dat->GetPtr((const char *)NULL, defaultValue) : defaultValue;
}

inline Color KeyValues::GetColor(int keySymbol)
{
	Color defaultValue(0, 0, 0, 0);
	KeyValues *dat = FindKey2(keySymbol);
	return dat ? dat->GetColor() : defaultValue;
}

inline bool KeyValues::IsEmpty(int keySymbol)
{
	KeyValues *dat = FindKey2(keySymbol);
	return dat ? dat->IsEmpty() : true;
}

bool EvaluateConditional( const char *str );

#endif