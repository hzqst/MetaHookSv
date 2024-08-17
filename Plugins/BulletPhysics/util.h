#pragma once

#include <string>

qboolean UTIL_ParseStringAsVector1(const char* string, float* vec);
qboolean UTIL_ParseStringAsVector2(const char* string, float* vec);
qboolean UTIL_ParseStringAsVector3(const char* string, float* vec);
qboolean UTIL_ParseStringAsVector4(const char* string, float* vec);

void UTIL_RemoveFileExtension(std::string& filePath);
std::string trim(const std::string& str);

#define PROJECT_X(x, w) ((1.0f + (float)(x)) * (float)(w) * 0.5f)
#define PROJECT_Y(y, h) ((1.0f - (float)(y)) * (float)(h) * 0.5f)

#define UNPROJECT_X(x, w) ((float)(x) * 2.0f / (float)(w) - 1.0f)
#define UNPROJECT_Y(y, h) (1.0f - (float)(y) * 2.0f / (float)(h))