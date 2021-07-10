#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"
#include "com_model.h"
#include "triangleapi.h"
#include "cvardef.h"
#include "exportfuncs.h"
#include "enghook.h"

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

float(*pbonetransform)[MAXSTUDIOBONES][3][4] = NULL;
float(*plighttransform)[MAXSTUDIOBONES][3][4] = NULL;

bool IsEntityCorpse(cl_entity_t* ent);

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

	return gExportfuncs.Initialize(pEnginefuncs, iVersion);
}

void __fastcall StudioSetupBones(void *pthis, int)
{
	if(IsEntityCorpse(IEngineStudio.GetCurrentEntity()))
	{
		gPhysicsManager.SetupBones((*pstudiohdr), IEngineStudio.GetCurrentEntity()->index);
		return;
	}
	gPrivateFuncs.StudioSetupBones(pthis, 0);
}

int StudioDrawPlayer(int flags, struct entity_state_s *pplayer)
{
	int result = 0;

	if (flags & STUDIO_RENDER)
	{
		if (gCorpseManager.IsPlayerDeathAnimation(pplayer))
		{
			auto currententity = IEngineStudio.GetCurrentEntity();
			auto tempent = gCorpseManager.FindCorpseForEntity(currententity);
			if (!tempent)
			{
				bool bRagdoll = false;

				gPrivateFuncs.StudioDrawPlayer(0, pplayer);

				std::string modelname((*r_model)->name);

				auto cfg = gPhysicsManager.LoadRagdollConfig(modelname);

				if (cfg)
				{
					auto itor = cfg->animcontrol.find(pplayer->sequence);

					if (itor == cfg->animcontrol.end() || (itor != cfg->animcontrol.end() && pplayer->frame >= itor->second))
					{
						tempent = gCorpseManager.CreateCorpseForEntity(currententity, (*r_model));
						if (tempent)
						{
							float frametime = currententity->curstate.animtime - currententity->latched.prevanimtime;

							vec3_t velocity;
							VectorSubtract(currententity->curstate.origin, currententity->latched.prevorigin, velocity);
							velocity[0] /= frametime;
							velocity[1] /= frametime;
							velocity[2] /= frametime;

							if (gPhysicsManager.CreateRagdoll(cfg, tempent->entity.index, (*r_model), (*pstudiohdr), velocity))
							{
								bRagdoll = true;
							}
						}
					}
				}

				if(!bRagdoll)
					result = gPrivateFuncs.StudioDrawPlayer(flags, pplayer);
			}
			else
			{
				tempent->entity.curstate.colormap = pplayer->colormap;
			}
			return result;
		}
		else
		{
			gCorpseManager.FreeCorpseForEntity(IEngineStudio.GetCurrentEntity());
		}
	}

	return gPrivateFuncs.StudioDrawPlayer(flags, pplayer);
}

/*int StudioCheckBBox(void)
{
	auto currententity = IEngineStudio.GetCurrentEntity();
	if (gCorpseManager.IsEntityCorpse(currententity))
	{

		return true;
	}

	return IEngineStudio.StudioCheckBBox();
}*/

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	int result = gExportfuncs.HUD_GetStudioModelInterface(version, ppinterface, pstudio);

	DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)pstudio->StudioSetHeader, 0x10, "\xA3", 1);
	Sig_AddrNotFound("pstudiohdr");
	pstudiohdr = *(studiohdr_t ***)(addr + 1);

	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)pstudio->SetRenderModel, 0x10, "\xA3", 1);
	Sig_AddrNotFound("r_model");
	r_model = *(model_t ***)(addr + 1);

	pbonetransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetBoneTransform();
	plighttransform = (float(*)[MAXSTUDIOBONES][3][4])pstudio->StudioGetLightTransform();

	g_pMetaHookAPI->InlineHook(gPrivateFuncs.StudioSetupBones, StudioSetupBones, (void *&)gPrivateFuncs.StudioSetupBones);
	//g_pMetaHookAPI->InlineHook(IEngineStudio.StudioCheckBBox, StudioCheckBBox, (void *&)IEngineStudio.StudioCheckBBox);

	gPrivateFuncs.StudioDrawPlayer = (*ppinterface)->StudioDrawPlayer;
	(*ppinterface)->StudioDrawPlayer = StudioDrawPlayer;

	return result;
}

void BV_Reload_f(void)
{
	gPhysicsManager.ReloadConfig();
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	bv_debug = gEngfuncs.pfnRegisterVariable("bv_debug", "0", FCVAR_CLIENTDLL);
	bv_simrate = gEngfuncs.pfnRegisterVariable("bv_simrate", "64", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	bv_scale = gEngfuncs.pfnRegisterVariable("bv_scale", "0.1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	gEngfuncs.pfnAddCommand("bv_reload", BV_Reload_f);
}

int HUD_AddEntity(int type, cl_entity_t *ent, const char *model)
{
	if (type == 0 && ent->model && ent->model->type == modtype_t::mod_brush && ent->curstate.solid == SOLID_BSP)
	{
		gPhysicsManager.CreateForBrushModel(ent);
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
		gPhysicsManager.StepSimulation(frametime);
		gPhysicsManager.SynchronizeTempEntntity(ppTempEntActive);
	}

	return gExportfuncs.HUD_TempEntUpdate(frametime, client_time, cl_gravity, ppTempEntFree, ppTempEntActive, Callback_AddVisibleEntity, Callback_TempEntPlaySound);
}

void HUD_DrawTransparentTriangles(void)
{
	gExportfuncs.HUD_DrawTransparentTriangles();

	gPhysicsManager.DebugDraw();
}