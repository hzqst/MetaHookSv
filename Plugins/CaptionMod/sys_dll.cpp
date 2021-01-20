#include <windows.h>
#include "tier1/strtools.h"

void Sys_GetRegKeyValueUnderRoot(const char *pszSubKey, const char *pszElement, char *pszReturnString, int nReturnLength, const char *pszDefaultValue)
{
	HKEY hKey;
	char szBuff[128];
	DWORD dwDisposition;
	DWORD dwType;
	DWORD dwSize;

	Q_snprintf(pszReturnString, nReturnLength, "%s", pszDefaultValue);
	pszReturnString[nReturnLength - 1] = '\0';

	if (RegCreateKeyExA(HKEY_CURRENT_USER, pszSubKey, 0, "String", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
		return;

	if (dwDisposition == REG_CREATED_NEW_KEY)
	{
		RegSetValueEx(hKey, pszElement, 0, REG_SZ, (CONST BYTE *)pszDefaultValue, strlen(pszDefaultValue) + 1);
	}
	else
	{
		dwSize = nReturnLength;

		if (RegQueryValueEx(hKey, pszElement, 0, &dwType, (unsigned char *)szBuff, &dwSize) == ERROR_SUCCESS)
		{
			if (dwType == REG_SZ)
			{
				Q_strncpy(pszReturnString, szBuff, nReturnLength);
				pszReturnString[nReturnLength - 1] = '\0';
			}
		}
		else
		{
			RegSetValueEx(hKey, pszElement, 0, REG_SZ, (CONST BYTE *)pszDefaultValue, strlen(pszDefaultValue) + 1);
		}
	}

	RegCloseKey(hKey);
}

void Sys_SetRegKeyValueUnderRoot(const char *pszSubKey, const char *pszElement, const char *pszValue)
{
	HKEY hKey;
	DWORD dwDisposition;

	if (RegCreateKeyExA(HKEY_CURRENT_USER, pszSubKey, 0, "String", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
		return;

	RegSetValueEx(hKey, pszElement, 0, REG_SZ, (CONST BYTE *)pszValue, strlen(pszValue) + 1);
	RegCloseKey(hKey);
}