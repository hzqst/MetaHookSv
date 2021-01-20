#include <metahook.h>
#include "plugins.h"
#include "steam_api.h"

HMODULE g_hSteamAPI = NULL;

ISteamUser *(*g_pfnSteamUser)(void) = NULL;
ISteamFriends *(*g_pfnSteamFriends)(void) = NULL;
ISteamUtils *(*g_pfnSteamUtils)(void) = NULL;
ISteamMatchmaking *(*g_pfnSteamMatchmaking)(void) = NULL;
ISteamUserStats *(*g_pfnSteamUserStats)(void) = NULL;
ISteamApps *(*g_pfnSteamApps)(void) = NULL;
ISteamNetworking *(*g_pfnSteamNetworking)(void) = NULL;
ISteamMatchmakingServers *(*g_pfnSteamMatchmakingServers)(void) = NULL;
ISteamRemoteStorage *(*g_pfnSteamRemoteStorage)(void) = NULL;
ISteamScreenshots *(*g_pfnSteamScreenshots)(void) = NULL;
ISteamHTTP *(*g_pfnSteamHTTP)(void) = NULL;
ISteamUnifiedMessages *(*g_pfnSteamUnifiedMessages)(void) = NULL;

void (*g_pfnSteamAPI_Shutdown)(void) = NULL;
int (*g_pfnSteamAPI_Init)(void) = NULL;
bool (*g_pfnSteamAPI_IsSteamRunning)(void) = NULL;
void (*g_pfnSteamAPI_RegisterCallback)(class CCallbackBase *pCallback, int iCallback) = NULL;
void (*g_pfnSteamAPI_UnregisterCallback)(class CCallbackBase *pCallback) = NULL;

void SteamAPI_Load(void)
{
	g_hSteamAPI = GetModuleHandle("steam_api.dll");
	g_pfnSteamAPI_Init = (int (*)(void))GetProcAddress(g_hSteamAPI, "SteamAPI_Init");
	g_pfnSteamAPI_Shutdown = (void (*)(void))GetProcAddress(g_hSteamAPI, "SteamAPI_Shutdown");
	g_pfnSteamAPI_IsSteamRunning = (bool (*)(void))GetProcAddress(g_hSteamAPI, "SteamAPI_IsSteamRunning");
	g_pfnSteamAPI_RegisterCallback = (void (*)(class CCallbackBase *, int))GetProcAddress(g_hSteamAPI, "SteamAPI_RegisterCallback");
	g_pfnSteamAPI_UnregisterCallback = (void (*)(class CCallbackBase *))GetProcAddress(g_hSteamAPI, "SteamAPI_UnregisterCallback");
	g_pfnSteamUnifiedMessages = (ISteamUnifiedMessages *(*)(void))GetProcAddress(g_hSteamAPI, "SteamUnifiedMessages");
	g_pfnSteamHTTP = (ISteamHTTP *(*)(void))GetProcAddress(g_hSteamAPI, "SteamHTTP");
	g_pfnSteamScreenshots = (ISteamScreenshots *(*)(void))GetProcAddress(g_hSteamAPI, "SteamScreenshots");
	g_pfnSteamRemoteStorage = (ISteamRemoteStorage *(*)(void))GetProcAddress(g_hSteamAPI, "SteamRemoteStorage");
	g_pfnSteamMatchmakingServers = (ISteamMatchmakingServers *(*)(void))GetProcAddress(g_hSteamAPI, "SteamMatchmakingServers");
	g_pfnSteamNetworking = (ISteamNetworking *(*)(void))GetProcAddress(g_hSteamAPI, "SteamNetworking");
	g_pfnSteamApps = (ISteamApps *(*)(void))GetProcAddress(g_hSteamAPI, "SteamApps");
	g_pfnSteamUserStats = (ISteamUserStats *(*)(void))GetProcAddress(g_hSteamAPI, "SteamUserStats");
	g_pfnSteamMatchmaking = (ISteamMatchmaking *(*)(void))GetProcAddress(g_hSteamAPI, "SteamMatchmaking");
	g_pfnSteamUtils = (ISteamUtils *(*)(void))GetProcAddress(g_hSteamAPI, "SteamUtils");
	g_pfnSteamFriends = (ISteamFriends *(*)(void))GetProcAddress(g_hSteamAPI, "SteamFriends");
	g_pfnSteamUser = (ISteamUser *(*)(void))GetProcAddress(g_hSteamAPI, "SteamUser");
}

ISteamUser *SteamUser(void)
{
	if (!g_hSteamAPI)
		return NULL;

	if (!g_pfnSteamUser)
		return NULL;

	return g_pfnSteamUser();
}

ISteamFriends *SteamFriends(void)
{
	if (!g_hSteamAPI)
		return NULL;

	if (!g_pfnSteamFriends)
		return NULL;

	return g_pfnSteamFriends();
}

ISteamUtils *SteamUtils(void)
{
	if (!g_hSteamAPI)
		return NULL;

	if (!g_pfnSteamUtils)
		return NULL;

	return g_pfnSteamUtils();
}

ISteamMatchmaking *SteamMatchmaking(void)
{
	if (!g_hSteamAPI)
		return NULL;

	if (!g_pfnSteamMatchmaking)
		return NULL;

	return g_pfnSteamMatchmaking();
}

ISteamUserStats *SteamUserStats(void)
{
	if (!g_hSteamAPI)
		return NULL;	

	if (!g_pfnSteamUserStats)
		return NULL;

	return g_pfnSteamUserStats();
}

ISteamApps *SteamApps(void)
{
	if (!g_hSteamAPI)
		return NULL;

	if (!g_pfnSteamApps)
		return NULL;

	return g_pfnSteamApps();
}

ISteamNetworking *SteamNetworking(void)
{
	if (!g_hSteamAPI)
		return NULL;

	if (!g_pfnSteamNetworking)
		return NULL;

	return g_pfnSteamNetworking();
}

ISteamMatchmakingServers *SteamMatchmakingServers(void)
{
	if (!g_hSteamAPI)
		return NULL;	

	if (!g_pfnSteamMatchmakingServers)
		return NULL;

	return g_pfnSteamMatchmakingServers();
}

ISteamRemoteStorage *SteamRemoteStorage(void)
{
	if (!g_hSteamAPI)
		return NULL;

	if (!g_pfnSteamRemoteStorage)
		return NULL;

	return g_pfnSteamRemoteStorage();
}

ISteamScreenshots *SteamScreenshots(void)
{
	if (!g_hSteamAPI)
		return NULL;

	if (!g_pfnSteamScreenshots)
		return NULL;

	return g_pfnSteamScreenshots();
}

ISteamHTTP *SteamHTTP(void)
{
	if (!g_hSteamAPI)
		return NULL;

	if (!g_pfnSteamHTTP)
		return NULL;

	return g_pfnSteamHTTP();
}

ISteamUnifiedMessages *SteamUnifiedMessages(void)
{
	if (!g_hSteamAPI)
		return NULL;

	if (!g_pfnSteamUnifiedMessages)
		return NULL;

	return g_pfnSteamUnifiedMessages();
}

bool SteamAPI_Init(void)
{
	if (!g_bIsUseSteam)
		return false;

	if (!g_hSteamAPI)
		return false;

	if (!g_pfnSteamAPI_Init)
		return false;

	return g_pfnSteamAPI_Init() != 0;
}

void SteamAPI_Shutdown(void)
{
	if (!g_bIsUseSteam)
		return;

	if (!g_hSteamAPI)
		return;

	if (!g_pfnSteamAPI_Shutdown)
		return;

	g_pfnSteamAPI_Shutdown();
}

bool SteamAPI_IsSteamRunning(void)
{
	if (!g_bIsUseSteam)
		return false;

	if (!g_hSteamAPI)
		return false;

	if (!g_pfnSteamAPI_IsSteamRunning)
		return false;

	return g_pfnSteamAPI_IsSteamRunning();
}

void SteamAPI_RegisterCallback(class CCallbackBase *pCallback, int iCallback)
{
	if (!g_bIsUseSteam)
		return;

	if (!g_hSteamAPI)
		return;

	if (!g_pfnSteamAPI_RegisterCallback)
		return;

	return g_pfnSteamAPI_RegisterCallback(pCallback, iCallback);
}

void SteamAPI_UnregisterCallback(class CCallbackBase *pCallback)
{
	if (!g_bIsUseSteam)
		return;

	if (!g_hSteamAPI)
		return;

	if (!g_pfnSteamAPI_UnregisterCallback)
		return;

	return g_pfnSteamAPI_UnregisterCallback(pCallback);
}