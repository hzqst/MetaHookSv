#include "util.h"
#include "mathlib2.h"
#include <cvardef.h>

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

qboolean UTIL_ParseStringAsColor1(const char* string, float* vec)
{
	vec2_t vinput;
	if (sscanf(string, "%f", &vinput[0]) == 1)
	{
		vec[0] = math_clamp(vinput[0], 0, 255) / 255.0f;
		return true;
	}
	return false;
}

qboolean UTIL_ParseStringAsColor2(const char* string, float* vec)
{
	vec2_t vinput;
	if (sscanf(string, "%f %f", &vinput[0], &vinput[1]) == 2)
	{
		vec[0] = math_clamp(vinput[0], 0, 255) / 255.0f;
		vec[1] = math_clamp(vinput[1], 0, 255) / 255.0f;
		return true;
	}
	return false;
}

qboolean UTIL_ParseStringAsColor3(const char* string, float* vec)
{
	vec3_t vinput;
	if (sscanf(string, "%f %f %f", &vinput[0], &vinput[1], &vinput[2]) == 3)
	{
		vec[0] = math_clamp(vinput[0], 0, 255) / 255.0f;
		vec[1] = math_clamp(vinput[1], 0, 255) / 255.0f;
		vec[2] = math_clamp(vinput[2], 0, 255) / 255.0f;
		return true;
	}
	return false;
}

qboolean UTIL_ParseStringAsColor4(const char* string, float* vec)
{
	vec4_t vinput;
	if (sscanf(string, "%f %f %f %f", &vinput[0], &vinput[1], &vinput[2], &vinput[3]) == 4)
	{
		vec[0] = math_clamp(vinput[0], 0, 255) / 255.0f;
		vec[1] = math_clamp(vinput[1], 0, 255) / 255.0f;
		vec[2] = math_clamp(vinput[2], 0, 255) / 255.0f;
		vec[3] = math_clamp(vinput[3], 0, 255) / 255.0f;
		return true;
	}
	return false;
}

qboolean UTIL_ParseStringAsVector1(const char* string, float* vec)
{
	vec2_t vinput;
	if (sscanf(string, "%f", &vinput[0]) == 1)
	{
		vec[0] = vinput[0];
		return true;
	}
	return false;
}

qboolean UTIL_ParseStringAsVector2(const char* string, float* vec)
{
	vec2_t vinput;
	if (sscanf(string, "%f %f", &vinput[0], &vinput[1]) == 2)
	{
		vec[0] = vinput[0];
		vec[1] = vinput[1];
		return true;
	}
	return false;
}

qboolean UTIL_ParseStringAsVector3(const char* string, float* vec)
{
	vec3_t vinput;
	if (sscanf(string, "%f %f %f", &vinput[0], &vinput[1], &vinput[2]) == 3)
	{
		vec[0] = vinput[0];
		vec[1] = vinput[1];
		vec[2] = vinput[2];
		return true;
	}
	return false;
}

qboolean UTIL_ParseStringAsVector4(const char* string, float* vec)
{
	vec4_t vinput;
	if (sscanf(string, "%f %f %f %f", &vinput[0], &vinput[1], &vinput[2], &vinput[3]) == 4)
	{
		vec[0] = vinput[0];
		vec[1] = vinput[1];
		vec[2] = vinput[2];
		vec[3] = vinput[3];
		return true;
	}
	return false;
}

qboolean UTIL_ParseCvarAsColor1(cvar_t* cvar, float* vec)
{
	return UTIL_ParseStringAsColor1(cvar->string, vec);
}

qboolean UTIL_ParseCvarAsColor2(cvar_t* cvar, float* vec)
{
	return UTIL_ParseStringAsColor2(cvar->string, vec);
}

qboolean UTIL_ParseCvarAsColor3(cvar_t* cvar, float* vec)
{
	return UTIL_ParseStringAsColor3(cvar->string, vec);
}

qboolean UTIL_ParseCvarAsColor4(cvar_t* cvar, float* vec)
{
	return UTIL_ParseStringAsColor4(cvar->string, vec);
}

qboolean UTIL_ParseCvarAsVector1(cvar_t* cvar, float* vec)
{
	return UTIL_ParseStringAsVector1(cvar->string, vec);
}

qboolean UTIL_ParseCvarAsVector2(cvar_t* cvar, float* vec)
{
	return UTIL_ParseStringAsVector2(cvar->string, vec);
}

qboolean UTIL_ParseCvarAsVector3(cvar_t* cvar, float* vec)
{
	return UTIL_ParseStringAsVector3(cvar->string, vec);
}

qboolean UTIL_ParseCvarAsVector4(cvar_t* cvar, float* vec)
{
	return UTIL_ParseStringAsVector4(cvar->string, vec);
}