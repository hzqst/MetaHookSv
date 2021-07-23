#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"
#include "com_model.h"
#include "triangleapi.h"
#include "cvardef.h"
#include "exportfuncs.h"
#include "entity_types.h"
#include "privatehook.h"

#include <vector>

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

cvar_t *cl_studiosnd_anti_spam_diff = NULL;
cvar_t *cl_studiosnd_anti_spam_same = NULL;
cvar_t *cl_studiosnd_anti_spam_delay = NULL;
cvar_t *cl_studiosnd_debug = NULL;

typedef struct studio_event_sound_s
{
	studio_event_sound_s(const char *n, int e, int f, float t)
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

std::vector<studio_event_sound_t> g_StudioEventSoundPlayed;

std::vector<studio_event_sound_t> g_StudioEventSoundDelayed;

void Sys_ErrorEx(const char *fmt, ...)
{
	char msg[4096] = { 0 };

	va_list argptr;

	va_start(argptr, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	if (gEngfuncs.pfnClientCmd)
		gEngfuncs.pfnClientCmd("escape\n");

	MessageBox(NULL, msg, "Fatal Error", MB_ICONERROR);
	TerminateProcess((HANDLE)(-1), 0);
}

int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion)
{
	memcpy(&gEngfuncs, pEnginefuncs, sizeof(gEngfuncs));

	return gExportfuncs.Initialize(pEnginefuncs, iVersion);
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	cl_studiosnd_anti_spam_diff = gEngfuncs.pfnRegisterVariable("cl_studiosnd_anti_spam_diff", "0.5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cl_studiosnd_anti_spam_same = gEngfuncs.pfnRegisterVariable("cl_studiosnd_anti_spam_same", "1.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cl_studiosnd_anti_spam_delay = gEngfuncs.pfnRegisterVariable("cl_studiosnd_anti_spam_delay", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cl_studiosnd_debug = gEngfuncs.pfnRegisterVariable("cl_studiosnd_debug", "0", FCVAR_CLIENTDLL);
}

void HUD_Frame(double a1)
{
	gExportfuncs.HUD_Frame(a1);

	auto clientTime = gEngfuncs.GetClientTime();
	auto local = gEngfuncs.GetLocalPlayer();

	if (!local)
		return;

	auto itor = g_StudioEventSoundDelayed.begin();
	while (itor != g_StudioEventSoundDelayed.end())
	{
		if (clientTime > itor->time)
		{
			auto ent = gEngfuncs.GetEntityByIndex(itor->entindex);
			
			if (ent && ent->curstate.messagenum == local->curstate.messagenum)
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

void HUD_StudioEvent(const struct mstudioevent_s *ev, const struct cl_entity_s *ent)
{
	if (ev->event == 5004 && ev->options[0])
	{
		auto clientTime = gEngfuncs.GetClientTime();

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