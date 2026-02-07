#include <Windows.h>
#include <steam_api.h>
#include <string>
#include <iostream>
#include <filesystem>

bool ReadRegistryValue(const std::wstring& keyPath, const std::wstring& valueName, std::wstring& outValue) {
	HKEY hKey;
	// Open the key using the Unicode function RegOpenKeyExW
	if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		std::wcerr << L"[Error] Unable to open registry key." << std::endl;
		return false;
	}

	// First call: query the required buffer size and type
	DWORD valueLength = 0;
	DWORD valueType;
	LSTATUS status = RegQueryValueExW(hKey, valueName.c_str(), NULL, &valueType, NULL, &valueLength);
	if (status != ERROR_SUCCESS) {
		std::wcerr << L"[Error] Unable to query registry value size." << std::endl;
		RegCloseKey(hKey);
		return false;
	}

	// Make sure it's a string type
	if (valueType != REG_SZ) {
		std::wcerr << L"[Error] The registry value is not a string." << std::endl;
		RegCloseKey(hKey);
		return false;
	}

	// Allocate buffer and read the actual value
	outValue.resize(valueLength / sizeof(wchar_t));
	status = RegQueryValueExW(hKey, valueName.c_str(), NULL, NULL, (LPBYTE)outValue.data(), &valueLength);
	if (status != ERROR_SUCCESS) {
		std::wcerr << L"[Error] Unable to read registry value." << std::endl;
		RegCloseKey(hKey);
		return false;
	}

	// Remove trailing null terminator(s) if present
	while (!outValue.empty() && outValue.back() == L'\0') {
		outValue.pop_back();
	}

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
		return 1;
	}

	std::wstring SteamPath;
	std::wstring SteamClientDll;
	if (!ReadRegistryValue(L"Software\\Valve\\Steam", L"SteamPath", SteamPath)) {
		std::wcerr << L"[Error] Failed to get SteamPath." << std::endl;
		return 4;
	}

	if (!ReadRegistryValue(L"Software\\Valve\\Steam\\ActiveProcess", L"SteamClientDll", SteamClientDll)) {
		std::wcerr << L"[Error] Failed to get SteamPath." << std::endl;
		return 5;
	}
	std::filesystem::path steamPathFs(SteamPath);
	std::filesystem::path steamClientDllFs(SteamClientDll);

	steamPathFs /= L"steamclient.dll";

	// Normalize the paths to ensure they can be compared correctly
	std::error_code ec;
	steamPathFs = std::filesystem::canonical(steamPathFs, ec);
	if (ec) {
		std::wcerr << L"[Error] Failed to resolve steamclient.dll path: " << steamPathFs << L" (" << ec.message().c_str() << L")" << std::endl;
		return 9;
	}
	steamClientDllFs = std::filesystem::canonical(steamClientDllFs, ec);
	if (ec) {
		std::wcerr << L"[Error] Failed to resolve SteamClientDll path: " << steamClientDllFs << L" (" << ec.message().c_str() << L")" << std::endl;
		return 10;
	}

	if (steamPathFs != steamClientDllFs) {

		std::wstring newSteamClientDllPath = steamPathFs.wstring();

		// Write the new path back to the registry
		if (!WriteRegistryValue(L"Software\\Valve\\Steam\\ActiveProcess", L"SteamClientDll", newSteamClientDllPath)) {
			std::wcerr << L"[Error] Failed to set SteamClientDll to the expected one." << std::endl;
			std::wcerr << L"[Error] The SteamClientDll (" << steamClientDllFs << L") does not match the expected one (" << steamPathFs << L"). You might be opening a cracked steam game before? You can restart your Steam client to fix this issue!" << std::endl;
			return 6;
		}
	}

	if (SteamAPI_Init())
	{
		int appId = atoi(argv[1]);

		char szAppInstallDir[1024] = { 0 };
		if (SteamApps()->GetAppInstallDir(appId, szAppInstallDir, sizeof(szAppInstallDir)))
		{
			std::cout << szAppInstallDir << std::endl;
		}
		else
		{
			std::wcerr << L"[Error] Failed to GetAppInstallDir." << std::endl;
			return 7;
		}

		SteamAPI_Shutdown();
	}
	else
	{
		std::wcerr << L"[Error] Failed to SteamAPI_Init." << std::endl;
		return 8;
	}

	return 0;
}
