#ifndef ILOCALIZE_H
#define ILOCALIZE_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

class KeyValues;

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

namespace vgui
{

typedef unsigned long StringIndex_t;
const unsigned long INVALID_STRING_INDEX = (unsigned long)-1;

class ILocalize : public IBaseInterface
{
public:
	virtual bool AddFile(IFileSystem *fileSystem, const char *fileName) = 0;
	virtual void RemoveAll(void) = 0;
	virtual wchar_t *Find(char const *tokenName) = 0;
	virtual int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSizeInBytes) = 0;
	virtual int ConvertUnicodeToANSI(const wchar_t *unicode, char *ansi, int ansiBufferSize) = 0;
	virtual StringIndex_t FindIndex(const char *tokenName) = 0;
	virtual void ConstructString(wchar_t *unicodeOutput, int unicodeBufferSizeInBytes, wchar_t *formatString, int numFormatParameters, ...) = 0;
	virtual const char *GetNameByIndex(StringIndex_t index) = 0;
	virtual wchar_t *GetValueByIndex(StringIndex_t index) = 0;
	virtual StringIndex_t GetFirstStringIndex(void) = 0;
	virtual StringIndex_t GetNextStringIndex(StringIndex_t index) = 0;
	virtual void AddString(const char *tokenName, wchar_t *unicodeString, const char *fileName) = 0;
	virtual void SetValueByIndex(StringIndex_t index, wchar_t *newValue) = 0;
	virtual bool SaveToFile(IFileSystem *fileSystem, const char *fileName) = 0;
	virtual int GetLocalizationFileCount(void) = 0;
	virtual const char *GetLocalizationFileName(int index) = 0;
	virtual const char *GetFileNameByIndex(StringIndex_t index) = 0;
	virtual void ReloadLocalizationFiles(void) = 0;
	virtual void ConstructString(wchar_t *unicodeOutput, int unicodeBufferSizeInBytes, const char *tokenName, KeyValues *localizationVariables) = 0;
	virtual void ConstructString(wchar_t *unicodeOutput, int unicodeBufferSizeInBytes, StringIndex_t unlocalizedTextSymbol, KeyValues *localizationVariables) = 0;
};
}

#define VGUI_LOCALIZE_INTERFACE_VERSION "VGUI_Localize003"

#endif