#include <Windows.h>
#include "steam_api.h"

int main(int argc, const char **argv)
{
	if (argc < 2)
		return 0;

	if (SteamAPI_Init())
	{
		if (SteamAPI_IsSteamRunning())
		{
			int appId = atoi(argv[1]);

			char szAppInstallDir[1024] = {0};
			if (SteamApps()->GetAppInstallDir(appId, szAppInstallDir, sizeof(szAppInstallDir)))
			{
				puts(szAppInstallDir);
			}
		}

		SteamAPI_Shutdown();
	}

	return 0;
}
