#pragma once

#include <stdint.h>
#include <string>

void RemoveFileExtension(std::string& filePath);
void COM_FixSlashes(char* pname);
void StringReplaceW(std::wstring& strBase, const std::wstring& strSrc, const std::wstring& strDst);
void StringReplaceA(std::string& strBase, const std::string& strSrc, const std::string& strDst);