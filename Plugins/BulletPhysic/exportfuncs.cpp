#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"
#include "com_model.h"
#include "triangleapi.h"
#include "cvardef.h"
#include "exportfuncs.h"
#include "enghook.h"

#include "mathlib.h"
#include "phycorpse.h"
#include "physics.h"

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

cvar_t *bv_debug = NULL;
studiohdr_t **pstudiohdr = NULL;
model_t **r_model = NULL;
float(*pbonetransform)[MAXSTUDIOBONES][3][4] = NULL;
float(*plighttransform)[MAXSTUDIOBONES][3][4] = NULL;

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
	if(gCorpseManager.IsEntityCorpse(IEngineStudio.GetCurrentEntity()))
	{
		gPhysicsManager.SetupBones((*pstudiohdr), IEngineStudio.GetCurrentEntity()->index);
		return;
	}
	gPrivateFuncs.StudioSetupBones(pthis, 0);
}

int StudioDrawPlayer(int flags, struct entity_state_s *pplayer)
{
	//Default death animation
	if (gCorpseManager.IsPlayingDeathAnimation(pplayer))
	{
		if (!gCorpseManager.FindCorpseForEntity(IEngineStudio.GetCurrentEntity()))
		{
			TEMPENTITY* tempent = gCorpseManager.CreateCorpseForEntity(IEngineStudio.GetCurrentEntity());
			if (tempent)
			{
				gPrivateFuncs.StudioDrawPlayer(0, pplayer);

				tempent->entity.model = (*r_model);

				gPhysicsManager.CreateRagdoll(tempent->entity.index, tempent->entity.model, (*pstudiohdr), pplayer->velocity);
			}
		}
		return 0;
	}
	else
	{
		gCorpseManager.FreeCorpseForEntity(IEngineStudio.GetCurrentEntity());
	}

	return gPrivateFuncs.StudioDrawPlayer(flags, pplayer);
}

int HUD_GetStudioModelInterface(int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio)
{
	//Save Studio API
	memcpy(&IEngineStudio, pstudio, sizeof(IEngineStudio));
	gpStudioInterface = ppinterface;

	//InitPhysicsInterface(NULL);
	//gPhysics.InitSystem("svencoop", &IEngineStudio);

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
	gEngfuncs.pfnAddCommand("bv_reload", BV_Reload_f);
}

int HUD_AddEntity(int type, cl_entity_t *ent, const char *model)
{
	if (ent->index && ent->index < 512)
	{
		if (ent->model->type == modtype_t::mod_brush)
		{
			//gPhysics.AddCollider(ent);
		}
		else if (ent->model->type == modtype_t::mod_studio)
		{
			//gPhysics.AddCollider(ent);
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
	if (levelname && levelname[0])
	{
		gPhysicsManager.SetGravity(cl_gravity);
		gPhysicsManager.StepSimulation(frametime);
	}

	return gExportfuncs.HUD_TempEntUpdate(frametime, client_time, cl_gravity, ppTempEntFree, ppTempEntActive, Callback_AddVisibleEntity, Callback_TempEntPlaySound);
}

void HUD_DrawTransparentTriangles(void)
{
	gExportfuncs.HUD_DrawTransparentTriangles();

	if(bv_debug->value)
		gPhysicsManager.DebugDraw();
}