#include <metahook.h>
#include <capstone.h>
#include <cl_entity.h>
#include <event_api.h>
#include <triangleapi.h>
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
engine_studio_api_t IEngineStudio;
r_studio_interface_t** gpStudioInterface;

cvar_t* cl_chasedist = NULL;
cvar_t* cl_waterdist = NULL;

cvar_t* v_centermove = NULL;
cvar_t* v_centerspeed = NULL;
cvar_t* cl_forwardspeed = NULL;

cvar_t* cl_bob = NULL;
cvar_t* cl_bobcycle = NULL;
cvar_t* cl_bobup = NULL;

cvar_t* cl_vsmoothing = NULL;

cvar_t* cl_rollangle = NULL;
cvar_t* cl_rollspeed = NULL;

cvar_t* v_iroll_cycle = NULL;
cvar_t* v_ipitch_cycle = NULL;
cvar_t* v_iyaw_cycle = NULL;

cvar_t* v_iroll_level = NULL;
cvar_t* v_ipitch_level = NULL;
cvar_t* v_iyaw_level = NULL;

cvar_t* scr_ofsx = NULL;
cvar_t* scr_ofsy = NULL;
cvar_t* scr_ofsz = NULL;

float v_frametime = 0;
float v_lastDistance = 0;
vec3_t v_angles;
vec3_t v_cl_angles;
vec3_t v_sim_org;
vec3_t v_client_aimangles;
vec3_t v_crosshairangle;
bool v_resetCamera = true;
float v_idlescale = 0;

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

const vec3_t VEC_DUCK_HULL_MAX = { 16, 16, 18 };
const vec3_t VEC_DUCK_VIEW = { 0, 0, 12 };
const vec3_t VEC_DEAD_VIEW = { 0, 0, -8 };
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

	v_lastDistance = VectorDistance(origin, trace->endpos);
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

	v_resetCamera = false;
}

void V_ResetChaseCam()
{
	v_resetCamera = true;
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

	v_frametime = pparams->frametime;

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

#if 0

#define ORIGIN_BACKUP 64
#define ORIGIN_MASK (ORIGIN_BACKUP - 1)

struct viewinterp_t
{
	vec3_t Origins[ORIGIN_BACKUP];
	float OriginTime[ORIGIN_BACKUP];

	vec3_t Angles[ORIGIN_BACKUP];
	float AngleTime[ORIGIN_BACKUP];

	int CurrentOrigin;
	int CurrentAngle;
};

void V_StartPitchDrift()
{
	if (g_pitchdrift->laststop == gEngfuncs.GetClientTime())
	{
		return; // something else is keeping it from drifting
	}

	if (g_pitchdrift->nodrift || 0 == g_pitchdrift->pitchvel)
	{
		g_pitchdrift->pitchvel = v_centerspeed->value;
		g_pitchdrift->nodrift = false;
		g_pitchdrift->driftmove = 0;
	}
}

void V_DriftPitch(ref_params_t* pparams)
{
	float delta, move;

	if (0 != gEngfuncs.IsNoClipping() || 0 == pparams->onground || 0 != pparams->demoplayback || 0 != pparams->spectator)
	{
		g_pitchdrift->driftmove = 0;
		g_pitchdrift->pitchvel = 0;
		return;
	}

	// don't count small mouse motion
	if (g_pitchdrift->nodrift)
	{
		if (v_centermove->value > 0 && (in_mlook.state & 1) == 0)
		{
			// this is for lazy players. if they stopped, looked around and then continued
			// to move the view will be centered automatically if they move more than
			// v_centermove units.

			if (fabs(pparams->cmd->forwardmove) < cl_forwardspeed->value)
				g_pitchdrift->driftmove = 0;
			else
				g_pitchdrift->driftmove += pparams->frametime;

			if (g_pitchdrift->driftmove > v_centermove->value)
			{
				V_StartPitchDrift();
			}
			else
			{
				return; // player didn't move enough
			}
		}

		return; // don't drift view
	}

	delta = pparams->idealpitch - pparams->cl_viewangles[0];

	if (0 == delta)
	{
		g_pitchdrift->pitchvel = 0;
		return;
	}

	move = pparams->frametime * g_pitchdrift->pitchvel;

	g_pitchdrift->pitchvel *= (1.0f + (pparams->frametime * 0.25f)); // get faster by time

	if (delta > 0)
	{
		if (move > delta)
		{
			g_pitchdrift->pitchvel = 0;
			move = delta;
		}
		pparams->cl_viewangles[0] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			g_pitchdrift->pitchvel = 0;
			move = -delta;
		}
		pparams->cl_viewangles[0] -= move;
	}
}

float V_CalcRoll(vec3_t angles, vec3_t velocity, float rollangle, float rollspeed)
{
	float sign;
	float side;
	float value;
	vec3_t forward, right, up;

	AngleVectors(angles, forward, right, up);

	side = DotProduct(velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);

	value = rollangle;
	if (side < rollspeed)
	{
		side = side * value / rollspeed;
	}
	else
	{
		side = value;
	}

	return side * sign;
}

void V_CalcViewRoll(ref_params_t* pparams)
{
	float side;

	auto viewentity = gEngfuncs.GetEntityByIndex(pparams->viewentity);

	if (!viewentity)
		return;

	float rollangle = cl_rollangle->value;
	float rollspeed = cl_rollspeed->value;

	bool bIsSpectating = (pparams->spectator || (*g_iIsSpectator) || (*g_iUser1));

	if (gExportfuncs.CL_IsThirdPerson())
	{
		bIsSpectating = false;

		rollangle = 0;
		rollspeed = 0;
	}

	side = V_CalcRoll(viewentity->angles, pparams->simvel, cl_rollangle->value, cl_rollspeed->value);

	if (bIsSpectating)
	{
		side = 0;
		rollangle = 0;
		rollspeed = 0;
		return;
	}

	pparams->viewangles[2] += side;

	if (pparams->health <= 0 && (pparams->viewheight[2] != 0))
	{
		// only roll the view if the player is dead and the viewheight[2] is nonzero
		// this is so deadcam in multiplayer will work.
		pparams->viewangles[2] = 80; // dead view angle
		return;
	}
}

void V_AddIdle(ref_params_t* pparams)
{
	pparams->viewangles[2] += v_idlescale * sin(pparams->time * v_iroll_cycle->value) * v_iroll_level->value;
	pparams->viewangles[0] += v_idlescale * sin(pparams->time * v_ipitch_cycle->value) * v_ipitch_level->value;
	pparams->viewangles[1] += v_idlescale * sin(pparams->time * v_iyaw_cycle->value) * v_iyaw_level->value;
}

float V_CalcBob(ref_params_t* pparams)
{
	static double bobtime = 0;
	static float bob = 0;
	float cycle;
	static float lasttime = 0;
	vec3_t vel;


	if (pparams->onground == -1 ||
		pparams->time == lasttime)
	{
		// just use old value
		return bob;
	}

	lasttime = pparams->time;

	// TODO: bobtime will eventually become a value so large that it will no longer behave properly.
	// Consider resetting the variable if a level change is detected (pparams->time < lasttime might do the trick).
	bobtime += pparams->frametime;
	cycle = bobtime - (int)(bobtime / cl_bobcycle->value) * cl_bobcycle->value;
	cycle /= cl_bobcycle->value;

	if (cycle < cl_bobup->value)
	{
		cycle = M_PI * cycle / cl_bobup->value;
	}
	else
	{
		cycle = M_PI + M_PI * (cycle - cl_bobup->value) / (1.0 - cl_bobup->value);
	}

	// bob is proportional to simulated velocity in the xy plane
	// (don't count Z, or jumping messes it up)
	VectorCopy(pparams->simvel, vel);
	vel[2] = 0;

	bob = sqrt(vel[0] * vel[0] + vel[1] * vel[1]) * cl_bob->value;
	bob = bob * 0.3 + bob * 0.7 * sin(cycle);
	bob = min(bob, 4.f);
	bob = max(bob, -7.f);
	return bob;
}

void V_CalcNormalRefdef_Refactor(ref_params_t* pparams)
{
	static viewinterp_t ViewInterp;
	static float oldz = 0;
	static float lasttime = 0;

	cl_entity_t* ent, * view;
	vec3_t angles;
	float bob, waterOffset;
	vec3_t camAngles, camForward, camRight, camUp;
	cl_entity_t* pwater;

	// don't allow cheats in multiplayer
	if (pparams->maxclients > 0)
	{
		scr_ofsx->value = 0;
		scr_ofsy->value = 0;
		scr_ofsz->value = 0;
	}

	V_DriftPitch(pparams);

	if (0 != gEngfuncs.IsSpectateOnly())
	{
		ent = gEngfuncs.GetEntityByIndex((*g_iUser2));
	}
	else
	{
		// ent is the player model ( visible when out of body )
		ent = gEngfuncs.GetLocalPlayer();
	}

	// view is the weapon model (only visible from inside body )
	view = gEngfuncs.GetViewModel();

	// transform the view offset by the model's matrix to get the offset from
	// model origin for the view
	bob = V_CalcBob(pparams);

	// refresh position
	VectorCopy(pparams->simorg, pparams->vieworg);

	pparams->vieworg[2] += (bob);

	VectorAdd(pparams->vieworg, pparams->viewheight, pparams->vieworg);

	VectorCopy(pparams->cl_viewangles, pparams->viewangles);

	gEngfuncs.V_CalcShake();
	gEngfuncs.V_ApplyShake(pparams->vieworg, pparams->viewangles, 1.0f);

	// never let view origin sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// FIXME, we send origin at 1/128 now, change this?
	// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis

	pparams->vieworg[0] += 1.0f / 32;
	pparams->vieworg[1] += 1.0f / 32;
	pparams->vieworg[2] += 1.0f / 32;

	// Check for problems around water, move the viewer artificially if necessary
	// -- this prevents drawing errors in GL due to waves

	waterOffset = 0;
	if (pparams->waterlevel >= 2)
	{
		int contents, waterDist, waterEntity;

		vec3_t point;

		waterDist = cl_waterdist->value;

		if (pparams->hardware)
		{
			waterEntity = gEngfuncs.PM_WaterEntity(pparams->simorg);
			if (waterEntity >= 0 && waterEntity < pparams->max_entities)
			{
				pwater = gEngfuncs.GetEntityByIndex(waterEntity);
				if (pwater && (pwater->model != nullptr))
				{
					waterDist += (pwater->curstate.scale * 16); // Add in wave height
				}
			}
		}
		else
		{
			waterEntity = 0; // Don't need this in software
		}

		VectorCopy(pparams->vieworg, point);

		// Eyes are above water, make sure we're above the waves
		if (pparams->waterlevel == 2)
		{
			point[2] -= waterDist;
			for (int i = 0; i < waterDist; i++)
			{
				contents = gEngfuncs.PM_PointContents(point, nullptr);
				if (contents > CONTENTS_WATER)
					break;
				point[2] += 1;
			}
			waterOffset = (point[2] + waterDist) - pparams->vieworg[2];
		}
		else
		{
			// eyes are under water.  Make sure we're far enough under
			point[2] += waterDist;

			for (int i = 0; i < waterDist; i++)
			{
				contents = gEngfuncs.PM_PointContents(point, nullptr);
				if (contents <= CONTENTS_WATER)
					break;
				point[2] -= 1;
			}
			waterOffset = (point[2] - waterDist) - pparams->vieworg[2];
		}
	}

	pparams->vieworg[2] += waterOffset;

	V_CalcViewRoll(pparams);

	V_AddIdle(pparams);

	// offsets
	VectorCopy(pparams->cl_viewangles, angles);

	AngleVectors(angles, pparams->forward, pparams->right, pparams->up);

	// don't allow cheats in multiplayer
	if (pparams->maxclients <= 1)
	{
		for (int i = 0; i < 3; i++)
		{
			pparams->vieworg[i] += scr_ofsx->value * pparams->forward[i] + scr_ofsy->value * pparams->right[i] + scr_ofsz->value * pparams->up[i];
		}
	}

	// Treating cam_ofs[2] as the distance
	if (gExportfuncs.CL_IsThirdPerson())
	{
		vec3_t ofs = {0, 0, 0};

		gExportfuncs.CL_CameraOffset(ofs);

		vec3_t camAngles;
		
		VectorCopy(ofs, camAngles);

		camAngles[2] = 0;

		AngleVectors(camAngles, camForward, camRight, camUp);

		for (int i = 0; i < 3; i++)
		{
			pparams->vieworg[i] += -ofs[2] * camForward[i];
		}
	}

	// Give gun our viewangles
	VectorCopy(pparams->cl_viewangles, view->angles);

	// set up gun position
	V_CalcGunAngle(pparams);

	// Use predicted origin as view origin.
	view->origin = pparams->simorg;
	view->origin[2] += (waterOffset);
	view->origin = view->origin + pparams->viewheight;

	// Let the viewmodel shake at about 10% of the amplitude
	gEngfuncs.V_ApplyShake(view->origin, view->angles, 0.9);

	for (int i = 0; i < 3; i++)
	{
		view->origin[i] += bob * 0.4 * pparams->forward[i];
	}
	view->origin[2] += bob;

	// throw in a little tilt.
	view->angles[1] -= bob * 0.5;
	view->angles[2] -= bob * 1;
	view->angles[0] -= bob * 0.3;

	if (0 != cl_bobtilt->value)
	{
		view->curstate.angles = view->angles;
	}

	// pushing the view origin down off of the same X/Z plane as the ent's origin will give the
	// gun a very nice 'shifting' effect when the player looks up/down. If there is a problem
	// with view model distortion, this may be a cause. (SJB).
	view->origin[2] -= 1;

	// fudge position around to keep amount of weapon visible
	// roughly equal with different FOV
	if (pparams->viewsize == 110)
	{
		view->origin[2] += 1;
	}
	else if (pparams->viewsize == 100)
	{
		view->origin[2] += 2;
	}
	else if (pparams->viewsize == 90)
	{
		view->origin[2] += 1;
	}
	else if (pparams->viewsize == 80)
	{
		view->origin[2] += 0.5;
	}

	// Add in the punchangle, if any
	pparams->viewangles = pparams->viewangles + pparams->punchangle;

	// Include client side punch, too
	pparams->viewangles = pparams->viewangles + ev_punchangle;

	V_DropPunchAngle(pparams->frametime, ev_punchangle);

	// smooth out stair step ups
#if 1
	if (0 == pparams->smoothing && 0 != pparams->onground && pparams->simorg[2] - oldz > 0)
	{
		float steptime;

		steptime = pparams->time - lasttime;
		if (steptime < 0)
			// FIXME		I_Error ("steptime < 0");
			steptime = 0;

		oldz += steptime * 150;
		if (oldz > pparams->simorg[2])
			oldz = pparams->simorg[2];
		if (pparams->simorg[2] - oldz > 18)
			oldz = pparams->simorg[2] - 18;
		pparams->vieworg[2] += oldz - pparams->simorg[2];
		view->origin[2] += oldz - pparams->simorg[2];
	}
	else
	{
		oldz = pparams->simorg[2];
	}
#endif

	if(1)
	{
		static vec3_t lastorg;
		vec3_t delta;

		VectorSubtract(pparams->simorg, lastorg, delta);

		if (VectorLength(delta) != 0.0)
		{
			VectorCopy(pparams->simorg, ViewInterp.Origins[ViewInterp.CurrentOrigin & ORIGIN_MASK]);
			ViewInterp.OriginTime[ViewInterp.CurrentOrigin & ORIGIN_MASK] = pparams->time;
			ViewInterp.CurrentOrigin++;

			VectorCopy(pparams->simorg, lastorg);
		}
	}

	// Smooth out whole view in multiplayer when on trains, lifts
	if (cl_vsmoothing && 0 != cl_vsmoothing->value &&
		(0 != pparams->smoothing && (pparams->maxclients > 1)))
	{
		int foundidx;
		float t;

		if (cl_vsmoothing->value < 0.0)
		{
			gEngfuncs.Cvar_SetValue("cl_vsmoothing", 0.0);
		}

		t = pparams->time - cl_vsmoothing->value;

		int i;
		for (i = 1; i < ORIGIN_MASK; i++)
		{
			foundidx = ViewInterp.CurrentOrigin - 1 - i;
			if (ViewInterp.OriginTime[foundidx & ORIGIN_MASK] <= t)
				break;
		}

		if (i < ORIGIN_MASK && ViewInterp.OriginTime[foundidx & ORIGIN_MASK] != 0.0)
		{
			// Interpolate
			vec3_t delta;
			double frac;
			double dt;
			vec3_t neworg;

			dt = ViewInterp.OriginTime[(foundidx + 1) & ORIGIN_MASK] - ViewInterp.OriginTime[foundidx & ORIGIN_MASK];
			if (dt > 0.0)
			{
				frac = (t - ViewInterp.OriginTime[foundidx & ORIGIN_MASK]) / dt;
				frac = min(1.0, frac);

				delta[0] = ViewInterp.Origins[(foundidx + 1) & ORIGIN_MASK][0] - ViewInterp.Origins[foundidx & ORIGIN_MASK][0];
				delta[1] = ViewInterp.Origins[(foundidx + 1) & ORIGIN_MASK][1] - ViewInterp.Origins[foundidx & ORIGIN_MASK][1];
				delta[2] = ViewInterp.Origins[(foundidx + 1) & ORIGIN_MASK][2] - ViewInterp.Origins[foundidx & ORIGIN_MASK][2];

				neworg[0] = ViewInterp.Origins[foundidx & ORIGIN_MASK][0] + (frac * delta[0]);
				neworg[1] = ViewInterp.Origins[foundidx & ORIGIN_MASK][1] + (frac * delta[1]);
				neworg[2] = ViewInterp.Origins[foundidx & ORIGIN_MASK][2] + (frac * delta[2]);

				// Dont interpolate large changes
				if (VectorLength(delta) < 64)

					VectorSubtract(neworg, pparams->simorg, delta);

					VectorAdd(pparams->simorg, delta, pparams->simorg);
					VectorAdd(pparams->vieworg, delta, pparams->vieworg);
					VectorAdd(view->origin, delta, view->origin);
				}
			}
		}
	}

	// Store off v_angles before munging for third person
	VectorCopy(pparams->viewangles, v_angles);
	VectorCopy(pparams->cl_viewangles, v_client_aimangles);
	VectorCopy(pparams->viewangles, v_lastAngles);

	//	v_cl_angles = pparams->cl_viewangles;	// keep old user mouse angles !
	if (0 != CL_IsThirdPerson())
	{
		pparams->viewangles = camAngles;
	}

	// Apply this at all times
	{
		float pitch = pparams->viewangles[0];

		// Normalize angles
		if (pitch > 180)
			pitch -= 360.0;
		else if (pitch < -180)
			pitch += 360;

		// Player pitch is inverted
		pitch /= -3.0;

		// Slam local player's pitch value
		ent->angles[0] = pitch;
		ent->curstate.angles[0] = pitch;
		ent->prevstate.angles[0] = pitch;
		ent->latched.prevangles[0] = pitch;
	}

	// override all previous settings if the viewent isn't the client
	if (pparams->viewentity > pparams->maxclients)
	{
		cl_entity_t* viewentity;
		viewentity = gEngfuncs.GetEntityByIndex(pparams->viewentity);
		if (viewentity)
		{
			pparams->vieworg = viewentity->origin;
			pparams->viewangles = viewentity->angles;

			// Store off overridden viewangles
			v_angles = pparams->viewangles;
		}
	}

	lasttime = pparams->time;

	v_origin = pparams->vieworg;
	v_crosshairangle = pparams->crosshairangle;
}

#endif

void V_CalcNormalRefdef(ref_params_t* pparams)
{
	if (pparams->spectator || (*g_iUser1))
	{
		V_CalcSpectatorRefdef(pparams);
		return;
	}

	gPrivateFuncs.V_CalcNormalRefdef(pparams);
}

void V_CalcRefdef(struct ref_params_s* pparams)
{
#if 0
	(*g_iWaterLevel) = pparams->waterlevel;
	if (!pparams->nextView && !(*g_bRenderingPortals_SCClient))
	{
		pparams->onlyClientDraw = 0;

		if (pparams->intermission)
		{
			//TODO: refactor this shit.
			//V_CalcIntermissionRefdef(pparams);
			gPrivateFuncs.V_CalcNormalRefdef(pparams);
		}
		else if (pparams->spectator || 0 != (*g_iUser1))
		{
			V_CalcSpectatorRefdef(pparams);
		}
		else if (!pparams->paused)
		{
			//TODO: refactor this shit.
			gPrivateFuncs.V_CalcNormalRefdef(pparams);
		}

	}

	pparams->nextView = 0;
	AngleVectors(pparams->viewangles, pparams->forward, pparams->right, pparams->up);

#endif

#if 0
	if (!pparams->intermission && (pparams->spectator || 0 != (*g_iUser1)))
	{
		pparams->nextView = 1;

		gExportfuncs.V_CalcRefdef(pparams);

		pparams->nextView = 0;

		V_CalcSpectatorRefdef(pparams);

		AngleVectors(pparams->viewangles, pparams->forward, pparams->right, pparams->up);
		return;
	}

	gExportfuncs.V_CalcRefdef(pparams);

#endif
	gExportfuncs.V_CalcRefdef(pparams);

#if 0
	vec3_t fogColor;
	fogColor[0] = g_iFogColor_SCClient[0] / 255.0f;
	fogColor[1] = g_iFogColor_SCClient[1] / 255.0f;
	fogColor[2] = g_iFogColor_SCClient[2] / 255.0f;
	gEngfuncs.pTriAPI->Fog(fogColor, (*g_iStartDist_SCClient), (*g_iEndDist_SCClient), true);
#endif

}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	cl_chasedist = gEngfuncs.pfnGetCvarPointer("cl_chasedist");
	if (!cl_chasedist)
		cl_chasedist = gEngfuncs.pfnRegisterVariable("cl_chasedist", "128", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	cl_waterdist = gEngfuncs.pfnGetCvarPointer("cl_waterdist");

	v_centermove = gEngfuncs.pfnGetCvarPointer("v_centermove");
	v_centerspeed = gEngfuncs.pfnGetCvarPointer("v_centerspeed");
	cl_forwardspeed = gEngfuncs.pfnGetCvarPointer("cl_forwardspeed");

	cl_bob = gEngfuncs.pfnGetCvarPointer("cl_bob");
	cl_bobcycle = gEngfuncs.pfnGetCvarPointer("cl_bobcycle");
	cl_bobup = gEngfuncs.pfnGetCvarPointer("cl_bobup");

	cl_vsmoothing = gEngfuncs.pfnGetCvarPointer("cl_vsmoothing");

	cl_rollangle = gEngfuncs.pfnGetCvarPointer("cl_rollangle");
	cl_rollspeed = gEngfuncs.pfnGetCvarPointer("cl_rollspeed");

	v_iroll_cycle = gEngfuncs.pfnGetCvarPointer("v_iroll_cycle");
	v_ipitch_cycle = gEngfuncs.pfnGetCvarPointer("v_ipitch_cycle");
	v_iyaw_cycle = gEngfuncs.pfnGetCvarPointer("v_iyaw_cycle");

	v_iroll_level = gEngfuncs.pfnGetCvarPointer("v_iroll_level");
	v_ipitch_level = gEngfuncs.pfnGetCvarPointer("v_ipitch_level");
	v_iyaw_level = gEngfuncs.pfnGetCvarPointer("v_iyaw_level");

	scr_ofsx = gEngfuncs.pfnGetCvarPointer("scr_ofsx");
	scr_ofsy = gEngfuncs.pfnGetCvarPointer("scr_ofsy");
	scr_ofsz = gEngfuncs.pfnGetCvarPointer("scr_ofsz");

	memcpy(&s_ProxyEventAPI, gEngfuncs.pEventAPI, sizeof(s_ProxyEventAPI));

	s_ProxyEventAPI.EV_PlayerTrace = EV_PlayerTrace_Proxy;
	s_ProxyEventAPI.EV_SetUpPlayerPrediction = EV_SetUpPlayerPrediction_Proxy;
	s_ProxyEventAPI.EV_SetSolidPlayers = EV_SetSolidPlayers_Proxy;

	(*g_pClientDLLEventAPI) = &s_ProxyEventAPI;
}