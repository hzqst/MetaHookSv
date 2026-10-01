#include <windows.h>

BOOL Sys_GetExecutableName(char *pszName, int nSize)
{
	return GetModuleFileName(GetModuleHandle(NULL), pszName, nSize) != 0;
}

char *Sys_GetLongPathName(void)
{
	static char szLongPath[MAX_PATH];

	char szShortPath[MAX_PATH];
	char *pszPath;

	szShortPath[0] = 0;
	szLongPath[0] = 0;

	if (GetModuleFileName(NULL, szShortPath, sizeof(szShortPath)))
	{
		GetLongPathName(szShortPath, szLongPath, sizeof(szLongPath));
		pszPath = strrchr(szLongPath, '\\');

		if (pszPath[0])
			pszPath[1] = 0;

		size_t len = strlen(szLongPath);

		if (len > 0)
		{
			if (szLongPath[len - 1] == '\\' || szLongPath[len - 1] == '/')
				szLongPath[len - 1] = 0;
		}
	}

	return szLongPath;
}