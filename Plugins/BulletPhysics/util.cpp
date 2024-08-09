#include <metahook.h>
#include "util.h"
#include "mathlib2.h"

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