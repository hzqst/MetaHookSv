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
#include "plugins.h"
#include "command.h"

#include "pm_defs.h"
#include "mathlib.h"
#include "phycorpse.h"
#include "physics.h"

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

cvar_t *bv_debug = NULL;
cvar_t *bv_simrate = NULL;
cvar_t *bv_scale = NULL;

studiohdr_t **pstudiohdr = NULL;
model_t **r_model = NULL;
model_t *r_worldmodel = NULL;
void *g_pGameStudioRenderer = NULL;
int *r_visframecount = NULL;

float(*pbonetransform)[MAXSTUDIOBONES][3][4] = NULL;
float(*plighttransform)[MAXSTUDIOBONES][3][4] = NULL;

bool IsEntityBarnacle(cl_entity_t* ent);
bool IsEntityCorpse(cl_entity_t* ent);
bool IsPlayerDeathAnimation(entity_state_t* entstate);
bool IsPlayerBarnacleAnimation(entity_state_t* entstate);

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

	gPhysicsManager.Init();

	Cmd_GetCmdBase = *(cmd_function_t *(**)(void))((DWORD)g_pMetaSave->pEngineFuncs + 0x198);

	return gExportfuncs.Initialize(pEnginefuncs, iVersion);
}

void __fastcall GameStudioRenderer_StudioSetupBones(void *pthis, int)
{
	auto currententity = IEngineStudio.GetCurrentEntity();

	if (IsEntityCorpse(currententity))
	{
		gPhysicsManager.SetupPlayerBones((*pstudiohdr), currententity->index);
		return;
	}

	gPrivateFuncs.GameStudioRenderer_StudioSetupBones(pthis, 0);

	if (IsEntityBarnacle(currententity))
	{
		auto player = gCorpseManager.FindPlayerForBarnacle(currententity->index);
		if (player)
		{
			gPhysicsManager.SetupBarnacleBones((*pstudiohdr), player->index);
		}
	}
}

int __fastcall GameStudioRenderer_StudioDrawModel(void *pthis, int dummy, int flags)
{
	auto currententity = IEngineStudio.GetCurrentEntity();

	if(IsEntityCorpse(currententity))
	{
		flags &= ~STUDIO_EVENTS;
	}

	return gPrivateFuncs.GameStudioRenderer_StudioDrawModel(pthis, 0, flags);
}

int __fastcall GameStudioRenderer_StudioDrawPlayer(void *pthis, int dummy, int flags, struct entity_state_s *pplayer)
{
	int result = 0;

	if (flags & STUDIO_RENDER)
	{
		auto currententity = IEngineStudio.GetCurrentEntity();

		bool bPlayerDeath = IsPlayerDeathAnimation(pplayer);
		bool bPlayerBarnacle = bPlayerDeath ? false : IsPlayerBarnacleAnimation(pplayer);
		if (bPlayerDeath || bPlayerBarnacle)
		{
			auto tempent = gCorpseManager.FindCorpseForEntity(pplayer->number);
			if (!tempent)
			{
				bool bCreatingRagdoll = false;

				//Create ragdoll only for dead players, not for corpse entities.
				if (currententity->player)
				{
					gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, 0, pplayer);

					auto cfg = gPhysicsManager.LoadRagdollConfig((*r_model));

					if (cfg)
					{
						auto itor = cfg->animcontrol.find(pplayer->sequence);

						if (itor != cfg->animcontrol.end() && pplayer->frame >= itor->second)
						{
							tempent = gCorpseManager.CreateCorpseForEntity(currententity, (*r_model));
							if (tempent)
							{
								vec3_t velocity = { 0 };
								if (bPlayerDeath && currententity->curstate.usehull != 1)
								{
									float frametime = currententity->curstate.animtime - currententity->latched.prevanimtime;
									if (frametime > 0)
									{
										VectorSubtract(currententity->curstate.origin, currententity->latched.prevorigin, velocity);
										velocity[0] /= frametime;
										velocity[1] /= frametime;
										velocity[2] /= frametime;
									}
								}

								cl_entity_t *barnacle = NULL;
								if (bPlayerBarnacle)
								{
									barnacle = gCorpseManager.FindBarnacleForPlayer(pplayer);
								}

								if (gPhysicsManager.CreateRagdoll(cfg, tempent->entity.index, (*r_model), (*pstudiohdr), pplayer->origin, velocity, bPlayerBarnacle, barnacle))
								{
									bCreatingRagdoll = true;
								}
							}
						}
					}
				}

				if (!bCreatingRagdoll)
					result = gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);
				else if (flags & STUDIO_EVENTS)
					result = gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, STUDIO_EVENTS, pplayer);
			}
			else
			{
				if (flags & STUDIO_EVENTS)
					result = gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, STUDIO_EVENTS, pplayer);

				tempent->entity.curstate.colormap = pplayer->colormap;
				tempent->entity.curstate.gaitsequence = pplayer->gaitsequence;
				tempent->entity.curstate.sequence = pplayer->sequence;
				tempent->entity.curstate.frame = pplayer->frame;
			}
			return result;
		}
		else
		{
			gCorpseManager.FreeCorpseForEntity(pplayer->number);
		}
	}

	return gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer(pthis, 0, flags, pplayer);
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	int result = gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio);

	DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)pstudio->StudioSetHeader, 0x10, "\xA3", 1);
	Sig_AddrNotFound(pstudiohdr);
	pstudiohdr = *(studiohdr_t ***)(addr + 1);

	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)pstudio->SetRenderModel, 0x10, "\xA3", 1);
	Sig_AddrNotFound(r_model);
	r_model = *(model_t ***)(addr + 1);

	pbonetransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetBoneTransform();
	plighttransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetLightTransform();

	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)(*ppinterface)->StudioDrawPlayer, 0x50, "\xFF\x74\x2A\x2A\xB9", sizeof("\xFF\x74\x2A\x2A\xB9") - 1);
	Sig_AddrNotFound(GameStudioRenderer);

	g_pGameStudioRenderer = *(void **)(addr + sizeof("\xFF\x74\x2A\x2A\xB9") - 1);

	DWORD *vftable = *(DWORD **)g_pGameStudioRenderer;

	gPrivateFuncs.GameStudioRenderer_StudioDrawModel = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawModel))vftable[2];
	gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer = (decltype(gPrivateFuncs.GameStudioRenderer_StudioDrawPlayer))vftable[3];
	gPrivateFuncs.GameStudioRenderer_StudioSetupBones = (decltype(gPrivateFuncs.GameStudioRenderer_StudioSetupBones))vftable[7];

	Install_InlineHook(GameStudioRenderer_StudioSetupBones);
	Install_InlineHook(GameStudioRenderer_StudioDrawPlayer);
	Install_InlineHook(GameStudioRenderer_StudioDrawModel);

	return result;
}

void BV_Reload_f(void)
{
	gPhysicsManager.ReloadConfig();
}

void BV_ThreadPerson_f(void)
{
	auto localplayer = gEngfuncs.GetLocalPlayer();

	if (localplayer && localplayer->player)
	{
		if (IsPlayerBarnacleAnimation(&localplayer->curstate))
		{
			gEngfuncs.Con_Printf("Cannot change to thirdperson while playing barnacle animation.\n");
			return;
		}
	}

	gPrivateFuncs.ThreadPerson_f();
}

void BV_FirstPerson_f(void)
{
	auto localplayer = gEngfuncs.GetLocalPlayer();

	if (localplayer && localplayer->player)
	{
		if (IsPlayerBarnacleAnimation(&localplayer->curstate))
		{
			gEngfuncs.Con_Printf("Cannot change to firstperson while playing barnacle animation.\n");
			return;
		}
	}

	gPrivateFuncs.FirstPerson_f();
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	bv_debug = gEngfuncs.pfnRegisterVariable("bv_debug", "0", FCVAR_CLIENTDLL);
	bv_simrate = gEngfuncs.pfnRegisterVariable("bv_simrate", "64", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_scale = gEngfuncs.pfnRegisterVariable("bv_scale", "0.1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

	gEngfuncs.pfnAddCommand("bv_reload", BV_Reload_f);
	gPrivateFuncs.ThreadPerson_f = Cmd_HookCmd("thirdperson", BV_ThreadPerson_f);
	gPrivateFuncs.FirstPerson_f = Cmd_HookCmd("firstperson", BV_FirstPerson_f);
}

int HUD_AddEntity(int type, cl_entity_t *ent, const char *model)
{
	if (type == ET_NORMAL && ent->model)
	{
		if (ent->model->type == modtype_t::mod_brush && ent->curstate.solid == SOLID_BSP)
		{
			gPhysicsManager.CreateForBrushModel(ent);
		}

		if (IsEntityBarnacle(ent))
		{
			gPhysicsManager.CreateForBarnacle(ent);
			gCorpseManager.AddBarnacle(ent->index, 0);
		}
	}

	return gExportfuncs.HUD_AddEntity(type, ent, model);
}

void HUD_TempEntUpdate(
	double frametime,   // Simulation time
	double client_time, // Absolute time on client
	double cl_gravity,  // True gravity on client
	TEMPENTITY **ppTempEntFree,   // List of freed temporary ents
	TEMPENTITY **ppTempEntActive, // List 
	int(*Callback_AddVisibleEntity)(cl_entity_t *pEntity),
	void(*Callback_TempEntPlaySound)(TEMPENTITY *pTemp, float damp))
{
	auto levelname = gEngfuncs.pfnGetLevelName();
	if (levelname && levelname[0] && gCorpseManager.HasCorpse())
	{
		gPhysicsManager.SetGravity(cl_gravity);
		gPhysicsManager.UpdateTempEntity(ppTempEntActive, frametime, client_time);
		gPhysicsManager.StepSimulation(frametime);
	}

	return gExportfuncs.HUD_TempEntUpdate(frametime, client_time, cl_gravity, ppTempEntFree, ppTempEntActive, Callback_AddVisibleEntity, Callback_TempEntPlaySound);
}

void Vector3BulletToGoldSrc(btVector3& vec);

void V_CalcRefdef(struct ref_params_s *pparams)
{
	if (gExportfuncs.CL_IsThirdPerson())
	{
		auto local = gEngfuncs.GetLocalPlayer();

		if (local && local->player && (IsPlayerDeathAnimation(&local->curstate) || IsPlayerBarnacleAnimation(&local->curstate)))
		{
			gPhysicsManager.SyncView(local, pparams);
		}
	}

	gExportfuncs.V_CalcRefdef(pparams);
}

void HUD_DrawTransparentTriangles(void)
{
	gExportfuncs.HUD_DrawTransparentTriangles();

	gPhysicsManager.DebugDraw();
}