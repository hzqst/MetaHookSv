#pragma once

#include <metahook.h>
#include <stdint.h>
#include <string>

#define math_clamp(value, mi, ma) min(max(value, mi), ma)

void RemoveFileExtension(std::string& filePath);
void COM_FixSlashes(char* pname);

qboolean UTIL_ParseStringAsColor1(const char* string, float* vec);
qboolean UTIL_ParseStringAsColor2(const char* string, float* vec);
qboolean UTIL_ParseStringAsColor3(const char* string, float* vec);
qboolean UTIL_ParseStringAsColor4(const char* string, float* vec);
qboolean UTIL_ParseStringAsVector1(const char* string, float* vec);
qboolean UTIL_ParseStringAsVector2(const char* string, float* vec);
qboolean UTIL_ParseStringAsVector3(const char* string, float* vec);
qboolean UTIL_ParseStringAsVector4(const char* string, float* vec);
qboolean UTIL_ParseCvarAsColor1(cvar_t* cvar, float* vec);
qboolean UTIL_ParseCvarAsColor2(cvar_t* cvar, float* vec);
qboolean UTIL_ParseCvarAsColor3(cvar_t* cvar, float* vec);
qboolean UTIL_ParseCvarAsColor4(cvar_t* cvar, float* vec);
qboolean UTIL_ParseCvarAsVector1(cvar_t* cvar, float* vec);
qboolean UTIL_ParseCvarAsVector2(cvar_t* cvar, float* vec);
qboolean UTIL_ParseCvarAsVector3(cvar_t* cvar, float* vec);
qboolean UTIL_ParseCvarAsVector4(cvar_t* cvar, float* vec);