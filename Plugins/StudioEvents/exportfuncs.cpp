#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include <cl_entity.h>
#include <com_model.h>
#include <triangleapi.h>
#include <cvardef.h>
#include <entity_types.h>

#include <vector>
#include <algorithm>
#include <string>
#include <string_view>

#include "plugins.h"
#include "privatehook.h"
#include "exportfuncs.h"

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t** gpStudioInterface;

cvar_t* cl_studiosnd_anti_spam_diff = NULL;
cvar_t* cl_studiosnd_anti_spam_same = NULL;
cvar_t* cl_studiosnd_anti_spam_delay = NULL;
cvar_t* cl_studiosnd_block_player = NULL;
cvar_t* cl_studiosnd_debug = NULL;

typedef struct studio_event_sound_s
{
	studio_event_sound_s(const char* n, int e, int f, float t)
	{
		strcpy(name, n);
		entindex = e;
		frame = f;
		time = t;
	}
	char name[64];
	int entindex;
	int frame;
	float time;
}studio_event_sound_t;

static std::vector<studio_event_sound_t> g_StudioEventSoundPlayed;

static std::vector<studio_event_sound_t> g_StudioEventSoundDelayed;

static std::vector<std::string> g_StudioEventSoundWhitelist;
static std::vector<std::string> g_StudioEventSourceModelWhitelist;

static void LoadWhitelist(std::vector<std::string>& whitelist, const char* filePath, const char* whitelistName)
{
	auto hFileHandle = FILESYSTEM_ANY_OPEN(filePath, "rt");
	if (hFileHandle)
	{
		whitelist.clear();
		
		char szReadLine[256];
		while (!FILESYSTEM_ANY_EOF(hFileHandle))
		{
			FILESYSTEM_ANY_READLINE(szReadLine, sizeof(szReadLine) - 1, hFileHandle);
			szReadLine[sizeof(szReadLine) - 1] = 0;
			
			// Parse line and extract name
			bool quoted = false;
			char token[256];
			char* p = szReadLine;
			
			p = FILESYSTEM_ANY_PARSEFILE(p, token, &quoted);
			if (token[0])
			{
				std::string itemName(token);
				
				// Avoid duplicates
				const auto &itor = std::find_if(whitelist.begin(), whitelist.end(), [&itemName](const std::string& item) {
					return item.compare(itemName) == 0;
				});
				if (itor == whitelist.end())
				{
					whitelist.emplace_back(itemName);
				}
			}
		}
		
		FILESYSTEM_ANY_CLOSE(hFileHandle);
		
		gEngfuncs.Con_DPrintf("[StudioEvents] Loaded %d %s from \"%s\".\n", whitelist.size(), whitelistName, filePath);
	}
	else
	{
		gEngfuncs.Con_DPrintf("[StudioEvents] %s file \"%s\" not found.\n", whitelistName, filePath);
	}
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	cl_studiosnd_anti_spam_diff = gEngfuncs.pfnRegisterVariable("cl_studiosnd_anti_spam_diff", "0.5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cl_studiosnd_anti_spam_same = gEngfuncs.pfnRegisterVariable("cl_studiosnd_anti_spam_same", "1.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cl_studiosnd_anti_spam_delay = gEngfuncs.pfnRegisterVariable("cl_studiosnd_anti_spam_delay", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cl_studiosnd_block_player = gEngfuncs.pfnRegisterVariable("cl_studiosnd_block_player", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cl_studiosnd_debug = gEngfuncs.pfnRegisterVariable("cl_studiosnd_debug", "0", FCVAR_CLIENTDLL);

	g_StudioEventSoundPlayed.clear();
	
	LoadWhitelist(g_StudioEventSoundWhitelist, "studioevents/sound_whitelist.txt", "whitelisted sounds");
	LoadWhitelist(g_StudioEventSourceModelWhitelist, "studioevents/sourcemodel_whitelist.txt", "whitelisted source models");
}

int HUD_VidInit(void)
{
	g_StudioEventSoundPlayed.clear();
	g_StudioEventSoundDelayed.clear();

	return gExportfuncs.HUD_VidInit();
}

void HUD_Frame(double a1)
{
	gExportfuncs.HUD_Frame(a1);

	auto local = gEngfuncs.GetLocalPlayer();

	if (!local)
	{
		return;
	}

	auto clientTime = gEngfuncs.GetClientTime();

	auto itor = g_StudioEventSoundDelayed.begin();
	while (itor != g_StudioEventSoundDelayed.end())
	{
		if (clientTime > itor->time)
		{
			auto ent = gEngfuncs.GetEntityByIndex(itor->entindex);

			if (ent && ent->curstate.messagenum == local->curstate.messagenum)//TODO: change to cl_parsecount? player not emitted will get incorrect messagenum
			{
				mstudioevent_s ev;
				ev.event = 5004;
				ev.frame = itor->frame;
				strcpy(ev.options, itor->name);
				ev.type = 0;

				HUD_StudioEvent(&ev, ent);
			}

			itor = g_StudioEventSoundDelayed.erase(itor);
		}
		else
		{
			itor++;
		}
	}
}

const char* EngineStudio_GetCurrentRenderModelName()
{
	return (*r_model) ? (*r_model)->name : "";
}

void HUD_StudioEvent(const struct mstudioevent_s* ev, const struct cl_entity_s* ent)
{
	if (ev->event == 5004 && ev->options[0])
	{
		// Check whitelist first - whitelisted sounds or source models bypass all checks

		std::string_view soundSourceModelName(EngineStudio_GetCurrentRenderModelName());
		std::string_view soundName(ev->options);
		
		// Check sound name whitelist
		const auto &soundWhitelistItor = std::find_if(g_StudioEventSoundWhitelist.begin(), g_StudioEventSoundWhitelist.end(), [&soundName](const std::string& item) {
			return item.compare(soundName) == 0;
		});
		if (soundWhitelistItor != g_StudioEventSoundWhitelist.end())
		{
			if (cl_studiosnd_debug->value)
				gEngfuncs.Con_Printf("[StudioEvents] Sound whitelisted: %s\n", ev->options);
			
			gExportfuncs.HUD_StudioEvent(ev, ent);
			return;
		}
		
		// Check source model whitelist
		const auto &modelWhitelistItor = std::find_if(g_StudioEventSourceModelWhitelist.begin(), g_StudioEventSourceModelWhitelist.end(), [&soundSourceModelName](const std::string& item) {
			return item.compare(soundSourceModelName) == 0;
		});
		if (modelWhitelistItor != g_StudioEventSourceModelWhitelist.end())
		{
			if (cl_studiosnd_debug->value)
				gEngfuncs.Con_Printf("[StudioEvents] Source model whitelisted: %s (sound: %s)\n", soundSourceModelName.data(), ev->options);
			
			gExportfuncs.HUD_StudioEvent(ev, ent);
			return;
		}
		
		if (cl_studiosnd_block_player->value > 0 && ent->player)
		{
			if (cl_studiosnd_debug->value)
				gEngfuncs.Con_Printf("[StudioEvents] Blocked %s\n", ev->options);

			return;
		}

		float clientTime = (float)gEngfuncs.GetClientTime();

		bool bFound = false;
		float max_time = 0;

		auto max_duration = max(cl_studiosnd_anti_spam_diff->value, cl_studiosnd_anti_spam_same->value);

		auto itor = g_StudioEventSoundPlayed.begin();
		while (itor != g_StudioEventSoundPlayed.end())
		{
			if (clientTime > itor->time + max_duration)
			{
				itor = g_StudioEventSoundPlayed.erase(itor);
			}
			else
			{
				//Is same sound as before ?
				if (itor->entindex == ent->index && itor->frame == ev->frame && !strcmp(itor->name, ev->options))
				{
					if (clientTime < itor->time + cl_studiosnd_anti_spam_same->value)
					{
						bFound = true;

						if (itor->time + cl_studiosnd_anti_spam_same->value > max_time)
							max_time = itor->time + cl_studiosnd_anti_spam_same->value;
					}
				}
				else
				{
					if (clientTime < itor->time + cl_studiosnd_anti_spam_diff->value)
					{
						bFound = true;

						if (itor->time + cl_studiosnd_anti_spam_diff->value > max_time)
							max_time = itor->time + cl_studiosnd_anti_spam_diff->value;
					}
				}

				itor++;
			}
		}

		if (bFound)
		{
			if (cl_studiosnd_anti_spam_delay->value)
			{
				//blocked
				if (cl_studiosnd_debug->value)
					gEngfuncs.Con_Printf("[StudioEvents] Delayed %s\n", ev->options);

				g_StudioEventSoundDelayed.emplace_back(ev->options, ent->index, ev->frame, max_time);
			}
			else
			{
				//blocked
				if (cl_studiosnd_debug->value)
					gEngfuncs.Con_Printf("[StudioEvents] Blocked %s\n", ev->options);
			}

			return;
		}

		//blocked
		if (cl_studiosnd_debug->value)
			gEngfuncs.Con_Printf("[StudioEvents] Played %s\n", ev->options);

		g_StudioEventSoundPlayed.emplace_back(ev->options, ent->index, ev->frame, clientTime);
	}

	gExportfuncs.HUD_StudioEvent(ev, ent);
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio)
{
	EngineStudio_FillAddress(pstudio, g_MirrorEngineDLLInfo.ImageBase ? g_MirrorEngineDLLInfo : g_EngineDLLInfo, g_EngineDLLInfo);
	EngineStudio_InstalHooks();

	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	int result = gExportfuncs.HUD_GetStudioModelInterface ? gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio) : 1;

	return result;
}