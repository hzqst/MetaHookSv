#include "util.h"

void RemoveFileExtension(std::string& filePath)
{
	// Find the last occurrence of '.'
	size_t lastDotPosition = filePath.find_last_of(".");

	// Check if the dot is part of a directory component rather than an extension
	size_t lastPathSeparator = filePath.find_last_of("/\\");

	if (lastDotPosition != std::string::npos) {
		// Ensure the dot is after the last path separator
		if (lastPathSeparator != std::string::npos && lastDotPosition < lastPathSeparator) {
			return; // Dot is part of a directory name, not an extension
		}
		// Return the substring from the beginning to the dot
		filePath = filePath.substr(0, lastDotPosition);
	}

	// No extension found, return the original path
}

void COM_FixSlashes(char* pname)
{
#ifdef _WIN32
	while (*pname) {
		if (*pname == '/')
			*pname = '\\';
		pname++;
	}
#else
	while (*pname) {
		if (*pname == '\\')
			*pname = '/';
		pname++;
	}
#endif
}