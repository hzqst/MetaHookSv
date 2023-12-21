#include <windows.h>
#include "IRegistry.h"

class CRegistry : public IRegistry
{
public:
	CRegistry(void);
	virtual ~CRegistry(void);

public:
	void Init(void);
	void Shutdown(void);
	int ReadInt(const char *key, int defaultValue = 0);
	void WriteInt(const char *key, int value);
	const char *ReadString(const char *key, const char *defaultValue = NULL);
	void WriteString(const char *key, const char *value);

private:
	bool m_bValid;
	HKEY m_hKey;
};

static CRegistry g_Registry;
IRegistry *registry = (IRegistry *)&g_Registry;

CRegistry::CRegistry(void)
{
	m_bValid = false;
	m_hKey = 0;
}

CRegistry::~CRegistry(void)
{
}

int CRegistry::ReadInt(const char *key, int defaultValue)
{
	LONG lResult;
	DWORD dwType;
	DWORD dwSize;

	int value;

	if (!m_bValid)
		return defaultValue;

	dwSize = sizeof(DWORD);
	lResult = RegQueryValueEx(m_hKey, key, 0, &dwType, (LPBYTE)&value, &dwSize);

	if (lResult != ERROR_SUCCESS)
		return defaultValue;

	if (dwType != REG_DWORD)
		return defaultValue;

	return value;
}

void CRegistry::WriteInt(const char *key, int value)
{
	DWORD dwSize;

	if (!m_bValid)
		return;

	dwSize = sizeof(DWORD);
	RegSetValueEx(m_hKey, key, 0, REG_DWORD, (LPBYTE)&value, dwSize);
}

const char *CRegistry::ReadString(const char *key, const char *defaultValue)
{
	LONG lResult;
	DWORD dwType;
	DWORD dwSize = 512;

	static char value[512];
	value[0] = 0;

	if (!m_bValid)
		return defaultValue;

	lResult = RegQueryValueEx(m_hKey, key, 0, &dwType, (unsigned char *)value, &dwSize);

	if (lResult != ERROR_SUCCESS)
		return defaultValue;

	if (dwType != REG_SZ)
		return defaultValue;

	return value;
}

void CRegistry::WriteString(const char *key, const char *value)
{
	DWORD dwSize;

	if (!m_bValid)
		return;

	dwSize = (DWORD)(strlen(value) + 1);
	RegSetValueEx(m_hKey, key, 0, REG_SZ, (LPBYTE)value, dwSize);
}

static char *GetPlatformName(void)
{
	return "Half-Life";
}

void CRegistry::Init(void)
{
	LONG lResult;
	DWORD dwDisposition;

	char szModelKey[1024];
	wsprintf(szModelKey, "Software\\Valve\\%s\\Settings\\", GetPlatformName());
	lResult = RegCreateKeyEx(HKEY_CURRENT_USER, szModelKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_hKey, &dwDisposition);

	if (lResult != ERROR_SUCCESS)
	{
		m_bValid = false;
		return;
	}

	m_bValid = true;
}

void CRegistry::Shutdown(void)
{
	if (!m_bValid)
		return;

	m_bValid = false;
	RegCloseKey(m_hKey);
}