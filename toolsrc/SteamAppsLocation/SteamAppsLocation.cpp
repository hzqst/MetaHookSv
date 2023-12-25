#include <Windows.h>
#include <steam_api.h>
#include <detours.h>
#include <string>
#include <iostream>
#include <filesystem>

bool (S_CALLTYPE*IsSteamRunning)() = NULL;

bool S_CALLTYPE NewIsSteamRunning()
{
	return true;
}

bool ReadRegistryValue(const std::wstring& keyPath, const std::wstring& valueName, std::wstring& outValue) {
	HKEY hKey;
	// Open the key using the Unicode function RegOpenKeyExW
	if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		std::wcerr << L"[Error] Unable to open registry key." << std::endl;
		return false;
	}

	// Query the value using the Unicode function RegQueryValueExW
	wchar_t value[255];
	DWORD valueLength = sizeof(value);
	DWORD valueType;
	if (RegQueryValueExW(hKey, valueName.c_str(), NULL, &valueType, (LPBYTE)value, &valueLength) != ERROR_SUCCESS) {
		std::wcerr << L"[Error] Unable to read registry value." << std::endl;
		RegCloseKey(hKey);
		return false;
	}

	// Make sure it's a string type
	if (valueType != REG_SZ) {
		std::wcerr << L"[Error] The registry value is not a string." << std::endl;
		RegCloseKey(hKey);
		return false;
	}

	outValue.assign(value, valueLength / sizeof(wchar_t) - 1); // -1 to exclude the null terminator
	RegCloseKey(hKey);
	return true;
}

bool WriteRegistryValue(const std::wstring& keyPath, const std::wstring& valueName, const std::wstring& value) {
	HKEY hKey;
	// Open the key using the Unicode function RegOpenKeyExW
	if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
		std::wcerr << L"[Error] Unable to open registry key for writing." << std::endl;
		return false;
	}

	// Write the value using the Unicode function RegSetValueExW
	if (RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ,
		(const BYTE*)value.c_str(), (value.size() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
		std::wcerr << L"[Error] Unable to write registry value." << std::endl;
		RegCloseKey(hKey);
		return false;
	}

	RegCloseKey(hKey);
	return true;
}

int main(int argc, const char **argv)
{
	if (argc < 2)
	{
		std::wcerr << L"[Error] AppId must be specified." << std::endl;
		return -1;
	}
	
	auto steamapi = GetModuleHandleA("steam_api.dll");
	if (!steamapi)
	{
		std::wcerr << L"[Error] Failed to get steam_api." << std::endl;
		return -1;
	}

	IsSteamRunning = (decltype(IsSteamRunning))GetProcAddress(steamapi, "SteamAPI_IsSteamRunning");
	if (!IsSteamRunning)
	{
		std::wcerr << L"[Error] Failed to locate SteamAPI_IsSteamRunning." << std::endl;
		return -1;
	}

	std::wstring SteamPath;
	std::wstring SteamClientDll;
	if (!ReadRegistryValue(L"Software\\Valve\\Steam", L"SteamPath", SteamPath)) {
		std::wcerr << L"[Error] Failed to get SteamPath." << std::endl;
		return -1;
	}

	if (!ReadRegistryValue(L"Software\\Valve\\Steam\\ActiveProcess", L"SteamClientDll", SteamClientDll)) {
		std::wcerr << L"[Error] Failed to get SteamPath." << std::endl;
		return -1;
	}
	std::filesystem::path steamPathFs(SteamPath);
	std::filesystem::path steamClientDllFs(SteamClientDll);

	steamPathFs /= L"steamclient.dll";

	// Normalize the paths to ensure they can be compared correctly
	steamPathFs = std::filesystem::canonical(steamPathFs);
	steamClientDllFs = std::filesystem::canonical(steamClientDllFs);

	if (steamPathFs != steamClientDllFs) {

		std::wstring newSteamClientDllPath = steamPathFs.wstring();

		// Write the new path back to the registry
		if (!WriteRegistryValue(L"Software\\Valve\\Steam\\ActiveProcess", L"SteamClientDll", newSteamClientDllPath)) {
			std::wcerr << L"[Error] Failed to set SteamClientDll to the expected one." << std::endl;
			std::wcout << L"[Error] The SteamClientDll (" << steamClientDllFs << L") does not matches the expected one (" << steamClientDllFs << L"). You might be opening a cracked steam game before ? You can restart your steam client to fix this issue !" << std::endl;
			return -1;
		}
	}

	if (SteamAPI_Init())
	{
		int appId = atoi(argv[1]);

		char szAppInstallDir[1024] = { 0 };
		if (SteamApps()->GetAppInstallDir(appId, szAppInstallDir, sizeof(szAppInstallDir)))
		{
			std::cout << "[OK] " << szAppInstallDir << std::endl;
		}
		else
		{
			std::wcerr << L"[Error] Failed to GetAppInstallDir." << std::endl;
			return -1;
		}

		SteamAPI_Shutdown();
	}
	else
	{
		std::wcerr << L"[Error] Failed to SteamAPI_Init." << std::endl;
		return 0;
	}

	return 0;
}
