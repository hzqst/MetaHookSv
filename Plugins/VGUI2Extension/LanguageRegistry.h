#pragma once

#include <cstddef>
#include <cstring>

namespace LanguageRegistry
{
using Reader = void (__cdecl *)(const char*, const char*, char*, int, const char*);

inline bool IsSteamLanguage(const char* subKey, const char* element)
{
	// These five-argument engines use HKCU unless they strip an HKLM prefix.
	// Match the complete implicit-HKCU key; an explicit HKCU prefix is NOT stripped.
	return subKey && element &&
		!_stricmp(subKey, "Software\\Valve\\Steam") && !_stricmp(element, "Language");
}

inline void CopyBounded(char* destination, size_t capacity, const char* source, size_t sourceCapacity)
{
	if (!destination || !capacity)
		return;

	size_t length = 0;
	while (length < capacity - 1 && length < sourceCapacity && source[length])
	{
		destination[length] = source[length];
		++length;
	}
	destination[length] = '\0';
}

inline void Read(Reader original, const char* subKey, const char* element,
	char* output, int capacity, const char* defaultValue, const char* forcedLanguage,
	char* currentLanguage, size_t currentCapacity)
{
	// Preserve the original default and registry side effects; override only its output.
	original(subKey, element, output, capacity, defaultValue);
	if (!IsSteamLanguage(subKey, element) || !output || capacity <= 0)
		return;

	const auto outputCapacity = static_cast<size_t>(capacity);
	if (forcedLanguage && forcedLanguage[0])
		CopyBounded(output, outputCapacity, forcedLanguage, outputCapacity);

	// Capture the effective value, including truncation in the caller's buffer.
	CopyBounded(currentLanguage, currentCapacity, output, outputCapacity);
}
}
