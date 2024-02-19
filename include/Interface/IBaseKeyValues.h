//========= Copyright ?1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BASE_KEYVALUES_H
#define BASE_KEYVALUES_H

#ifdef _WIN32
#pragma once
#endif

// #include <vgui/VGUI.h>

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

class IFileSystem;
class CUtlBuffer;
class Color;
typedef void* FileHandle_t;

/*
	Purpose: keep the consistency of vftable layout with GoldSrc
*/

class KeyValues;

class IBaseKeyValues
{
public:
	// Data type

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
		TYPE_NUMTYPES,
	};

public:

	virtual const char* GetName(void) const = 0;
	virtual int GetNameSymbol(void) const = 0;
	virtual bool LoadFromFile(IFileSystem* filesystem, const char* resourceName, const char* pathID) = 0;
	virtual bool SaveToFile(IFileSystem* filesystem, const char* resourceName, const char* pathID) = 0;
	virtual KeyValues* FindKey2(int keySymbol) const = 0;
	virtual KeyValues* FindKey(const char* keyName, bool bCreate = false) = 0;
	virtual KeyValues* CreateNewKey() = 0;
	virtual void RemoveSubKey(KeyValues* subKey) = 0;
	virtual KeyValues* GetFirstSubKey() = 0;
	virtual KeyValues* GetNextKey() = 0;
	virtual int GetInt(const char* keyName, int defaultValue = 0) = 0;
	virtual float GetFloat(const char* keyName, float defaultValue = 0.0f) = 0;
	virtual const char* GetString(const char* keyName, const char* defaultValue = "") = 0;
	virtual const wchar_t* GetWString(const char* keyName, const wchar_t* defaultValue = L"") = 0;
	virtual void* GetPtr(const char* keyName, void* defaultValue = nullptr) = 0;
	virtual bool IsEmpty(const char* keyName) = 0;
	virtual void SetWString(const char* keyName, const wchar_t* value) = 0;
	virtual void SetString(const char* keyName, const char* value) = 0;
	virtual void SetInt(const char* keyName, int value) = 0;
	virtual void SetFloat(const char* keyName, float value) = 0;
	virtual void SetPtr(const char* keyName, void* value) = 0;
	virtual KeyValues* MakeCopy(void) const = 0;
	virtual void Clear(void) = 0;
	virtual types_t GetDataType(const char* keyName) = 0;
	virtual void deleteThis() = 0;
};

#endif // BASE_KEYVALUES_H
