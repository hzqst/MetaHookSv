#include <metahook.h>
#include <cl_entity.h>
#include <event_api.h>
#include <cvardef.h>
#include <pm_defs.h>
#include <pm_shared.h>
#include <entity_types.h>
#include <ref_params.h>
#include <com_model.h>
#include "exportfuncs.h"
#include "mathlib2.h"
#include "plugins.h"
#include "privatehook.h"

cl_enginefunc_t gEngfuncs;

cvar_t* cl_chasedist = NULL;
vec3_t v_angles;
vec3_t v_cl_angles;
vec3_t v_sim_org;

static struct event_api_s s_ProxyEventAPI = { 0 };

static bool g_bIsCallingCAM_Think = false;
static bool g_bIsCallingCAM_Think_Post = false;

int EngineGetMaxPhysEnts()
{
	if (g_iEngineType == ENGINE_SVENGINE && g_dwEngineBuildnum >= 10152)
		return MAX_PHYSENTS_10152;

	return MAX_PHYSENTS;
}

void EV_PlayerTrace_Proxy(float* start, float* end, int traceFlags, int ignore_pe, struct pmtrace_s* tr)
{
	if (g_bIsCallingCAM_Think && traceFlags == (PM_STUDIO_BOX | PM_STUDIO_IGNORE))
	{
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(1, 1);
		gEngfuncs.pEventAPI->EV_PushPMStates();

		auto spectating_player = gEngfuncs.GetLocalPlayer();

		if (g_iUser1 && g_iUser2 && (*g_iUser1))
		{
			spectating_player = gEngfuncs.GetEntityByIndex((*g_iUser2));
		}

		if (spectating_player->player)
		{
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(spectating_player->index - 1);
		}
		else
		{
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
		}

		ignore_pe = -1;

		for (int i = 0; i < EngineGetMaxPhysEnts(); ++i)
		{
			auto PhysEnt = gEngfuncs.pEventAPI->EV_GetPhysent(i);

			if (!PhysEnt)
				break;

			if (PhysEnt->info == spectating_player->index)
			{
				ignore_pe = i;
			}
		}

		gEngfuncs.pEventAPI->EV_SetTraceHull(2);

		gEngfuncs.pEventAPI->EV_PlayerTrace(start, end, PM_STUDIO_BOX | PM_STUDIO_IGNORE, ignore_pe, tr);

		gEngfuncs.pEventAPI->EV_PopPMStates();

		g_bIsCallingCAM_Think_Post = true;

		return;
	}

	return gEngfuncs.pEventAPI->EV_PlayerTrace(start, end, traceFlags, ignore_pe, tr);
}

void EV_SetUpPlayerPrediction_Proxy(int dopred, int bIncludeLocalClient)
{
	if (g_bIsCallingCAM_Think && g_bIsCallingCAM_Think_Post && dopred == 1 && bIncludeLocalClient == 1)
	{
		return;
	}

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(dopred, bIncludeLocalClient);
}

void EV_SetSolidPlayers_Proxy(int playernum)
{
	if (g_bIsCallingCAM_Think && g_bIsCallingCAM_Think_Post && playernum == -1)
	{
		return;
	}

	gEngfuncs.pEventAPI->EV_SetSolidPlayers(playernum);
}

void CAM_Think(void)
{
	g_bIsCallingCAM_Think = true;
	g_bIsCallingCAM_Think_Post = false;

	gExportfuncs.CAM_Think();

	g_bIsCallingCAM_Think = false;
	g_bIsCallingCAM_Think_Post = false;
}

const vec3_t VEC_VIEW = { 0, 0, 28 };

int PM_GetPhysEntInfo(int i)
{
	auto physent = gEngfuncs.pEventAPI->EV_GetPhysent(i);
	if (physent)
		return physent->info;
	return -1;
}

// Get the origin of the Observer based around the target's position and angles
void V_GetChaseOrigin(const float* angles, const float* origin, float distance, vec3_t& returnvec)
{
	vec3_t vecEnd;
	vec3_t forward, right, up;
	vec3_t vecStart;
	pmtrace_t* trace;
	int maxLoops = 8;

	int ignorePhysEntIndex = -1; // first, ignore no entity

	cl_entity_t* ent = nullptr;

	// Trace back from the target using the player's view angles
	AngleVectors(angles, forward, right, up);

	forward[0] = -forward[0];
	forward[1] = -forward[1];
	forward[2] = -forward[2];

	vecStart[0] = origin[0];
	vecStart[1] = origin[1];
	vecStart[2] = origin[2];

	vecEnd[0] = vecStart[0] + (distance * forward[0]);
	vecEnd[1] = vecStart[1] + (distance * forward[1]);
	vecEnd[2] = vecStart[2] + (distance * forward[2]);

	while (maxLoops > 0)
	{
		trace = gEngfuncs.PM_TraceLine(vecStart, vecEnd, PM_TRACELINE_PHYSENTSONLY, 2, ignorePhysEntIndex);

		// WARNING! trace->ent is is the number in physent list not the normal entity number

		if (trace->ent <= 0)
			break; // we hit the world or nothing, stop trace

		ent = gEngfuncs.GetEntityByIndex(PM_GetPhysEntInfo(trace->ent));

		if (ent == nullptr)
			break;

		// hit non-player solid BSP , stop here
		if (ent->curstate.solid == SOLID_BSP && 0 == ent->player)
			break;

		// if close enought to end pos, stop, otherwise continue trace
		if (VectorDistance(vecEnd, trace->endpos) < 1.0f)
		{
			break;
		}
		else
		{
			ignorePhysEntIndex = trace->ent; // ignore last hit entity
			vecStart[0] = trace->endpos[0];
			vecStart[1] = trace->endpos[1];
			vecStart[2] = trace->endpos[2];
		}

		maxLoops--;
	}

	returnvec[0] = trace->endpos[0] + (4 * trace->plane.normal[0]);
	returnvec[1] = trace->endpos[1] + (4 * trace->plane.normal[1]);
	returnvec[2] = trace->endpos[2] + (4 * trace->plane.normal[2]);
}

void V_GetChasePos(int target, vec3_t* cl_angles, vec3_t& origin, vec3_t& angles)
{
	cl_entity_t* ent = nullptr;

	if (0 != target)
	{
		ent = gEngfuncs.GetEntityByIndex(target);
	}

	if (!ent)
	{
		// just copy a save in-map position
		VectorCopy(gEngfuncs.GetLocalPlayer()->angles, angles);
		VectorCopy(gEngfuncs.GetLocalPlayer()->origin, origin);
		return;
	}

	if (cl_angles == nullptr) // no mouse angles given, use entity angles ( locked mode )
	{
		VectorCopy(ent->angles, angles);
		angles[0] *= -1;
	}
	else
	{
		VectorCopy((*cl_angles), angles);
	}

	VectorCopy(ent->origin, origin);
	VectorAdd(origin, VEC_VIEW, origin);

	V_GetChaseOrigin(angles, origin, cl_chasedist ? cl_chasedist->value : 128, origin);
}

/*
==================
V_CalcSpectatorRefdef
==================
*/
void V_CalcSpectatorRefdef(ref_params_t* pparams)
{
	pparams->onlyClientDraw = 0;

	// refresh position
	VectorCopy(pparams->simorg, v_sim_org);

	// get old values
	VectorCopy(pparams->cl_viewangles, v_cl_angles);
	VectorCopy(pparams->viewangles, v_angles);
	VectorCopy(pparams->vieworg, (*v_origin));

	switch ((*g_iUser1))
	{
	case OBS_SVEN_CHASE_FREE:
	{
		V_GetChasePos((*g_iUser2), &v_cl_angles, (*v_origin), v_angles);
		break;
	}
	case OBS_SVEN_ROAMING:
	{
		VectorCopy(v_cl_angles, v_angles);
		VectorCopy(v_sim_org, (*v_origin));
		break;
	}
	case OBS_SVEN_CHASE_LOCKED:
	{
		V_GetChasePos((*g_iUser2), nullptr, (*v_origin), v_angles);
		break;
	}
	}

	// Write back new values into pparams
	VectorCopy(v_cl_angles, pparams->cl_viewangles);
	VectorCopy(v_angles, pparams->viewangles);
	VectorCopy((*v_origin), pparams->vieworg);

	//For SoundEngine
	VectorCopy(pparams->viewangles, (*g_vVecViewangles));
}


void V_CalcNormalRefdef(ref_params_t* pparams)
{
	if (pparams->spectator || (*g_iUser1))
	{
		V_CalcSpectatorRefdef(pparams);
		return;
	}

	gPrivateFuncs.V_CalcNormalRefdef(pparams);
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	cl_chasedist = gEngfuncs.pfnGetCvarPointer("cl_chasedist");
	if (!cl_chasedist)
		cl_chasedist = gEngfuncs.pfnRegisterVariable("cl_chasedist", "128", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	memcpy(&s_ProxyEventAPI, gEngfuncs.pEventAPI, sizeof(s_ProxyEventAPI));

	s_ProxyEventAPI.EV_PlayerTrace = EV_PlayerTrace_Proxy;
	s_ProxyEventAPI.EV_SetUpPlayerPrediction = EV_SetUpPlayerPrediction_Proxy;
	s_ProxyEventAPI.EV_SetSolidPlayers = EV_SetSolidPlayers_Proxy;

	(*g_pClientDLLEventAPI) = &s_ProxyEventAPI;
}

void HUD_Shutdown(void)
{
	Client_UninstallHooks();

	gExportfuncs.HUD_Shutdown();
}