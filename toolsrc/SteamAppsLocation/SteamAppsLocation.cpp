#include <Windows.h>
#include <steam_api.h>
#include <detours.h>

bool (S_CALLTYPE*IsSteamRunning)() = NULL;

bool S_CALLTYPE NewIsSteamRunning()
{
	return true;
}

int main(int argc, const char **argv)
{
	if (argc < 2)
		return 0;
	
	auto steamapi = GetModuleHandleA("steam_api.dll");
	if (!steamapi)
		return 0;

	IsSteamRunning = (decltype(IsSteamRunning))GetProcAddress(steamapi, "SteamAPI_IsSteamRunning");
	if (!IsSteamRunning)
		return 0;

	DetourTransactionBegin();
	DetourAttach(&(void *&)IsSteamRunning, NewIsSteamRunning);
	DetourTransactionCommit();

	if (SteamAPI_Init())
	{
		int appId = atoi(argv[1]);

		char szAppInstallDir[1024] = { 0 };
		if (SteamApps()->GetAppInstallDir(appId, szAppInstallDir, sizeof(szAppInstallDir)))
		{
			puts(szAppInstallDir);
		}

		SteamAPI_Shutdown();
	}

	return 0;
}
