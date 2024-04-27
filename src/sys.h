#pragma once

#include <Windows.h>

BOOL Sys_GetExecutableName(char *pszName, int nSize);
char *Sys_GetLongPathName(void);