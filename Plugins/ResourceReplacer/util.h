#pragma once

#include <stdint.h>
#include <string>

std::string TrimString(const std::string& str);
void RemoveFileExtension(std::string& filePath);
void COM_FixSlashes(char* pname);
