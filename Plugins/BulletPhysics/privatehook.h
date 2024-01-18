#pragma once

#include <studio.h>
#include <com_model.h>

typedef struct walk_context_s
{
	walk_context_s(PVOID a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	PVOID address;
	size_t len;
	int depth;
}walk_context_t;

typedef struct
{
	void(*R_NewMap)(void);
	void(*R_RecursiveWorldNode)(void *node);
	void(*R_DrawTEntitiesOnList)(int onlyClientDraw);

	//Client GameStudioRenderer
	void(__fastcall* GameStudioRenderer_StudioSetupBones)(void* pthis, int);
	void(__fastcall* GameStudioRenderer_StudioMergeBones)(void* pthis, int, model_t* pSubModel);
	int(__fastcall* GameStudioRenderer_StudioDrawModel)(void* pthis, int, int flags);
	int(__fastcall* GameStudioRenderer_StudioDrawPlayer)(void* pthis, int, int flags, struct entity_state_s* pplayer);
	int(__fastcall* GameStudioRenderer__StudioDrawPlayer)(void* pthis, int, int flags, struct entity_state_s* pplayer);
	void(__fastcall* GameStudioRenderer_StudioRenderModel)(void* pthis, int);
	void(__fastcall* GameStudioRenderer_StudioRenderFinal)(void* pthis, int);

	int GameStudioRenderer_StudioCalcAttachments_vftable_index;
	int GameStudioRenderer_StudioSetupBones_vftable_index;
	int GameStudioRenderer_StudioSaveBones_vftable_index;
	int GameStudioRenderer_StudioMergeBones_vftable_index;
	int GameStudioRenderer_StudioDrawModel_vftable_index;
	int GameStudioRenderer_StudioDrawPlayer_vftable_index;
	int GameStudioRenderer__StudioDrawPlayer_vftable_index;
	int GameStudioRenderer_StudioRenderModel_vftable_index;
	int GameStudioRenderer_StudioRenderFinal_vftable_index;

	//Engine StudioRenderer
	int (*R_StudioDrawModel)(int flags);
	int (*R_StudioDrawPlayer)(int flags, struct entity_state_s *pplayer);
	void (*R_StudioSetupBones)(void);

	void (*FirstPerson_f)(void);
	void (*ThreadPerson_f)(void);

	//Engine model managment
	void (*Mod_LoadStudioModel)(model_t* mod, void* buffer);

	//efxapi
	TEMPENTITY	*(*efxapi_R_TempModel)				(float *pos, float *dir, float *angles, float life, int modelIndex, int soundtype);
}private_funcs_t;

void R_NewMap(void);
TEMPENTITY *efxapi_R_TempModel(float *pos, float *dir, float *angles, float life, int modelIndex, int soundtype);

void Engine_FillAddreess(void);
void Client_FillAddress(void);
void Engine_InstallHook(void);
void Engine_UninstallHook(void);

extern studiohdr_t** pstudiohdr;
extern model_t** r_model;
extern void* g_pGameStudioRenderer;
extern float(*pbonetransform)[128][3][4];
extern float(*plighttransform)[128][3][4];

extern int* r_framecount;
extern int *r_visframecount;
extern int *cl_parsecount;
extern void *cl_frames;
extern int size_of_frame;
extern int *cl_viewentity;
extern cl_entity_t **currententity;
extern void *mod_known;
extern int *mod_numknown;
extern TEMPENTITY *gTempEnts;

extern int *g_iUser1;
extern int *g_iUser2;

extern private_funcs_t gPrivateFuncs;

extern bool g_bIsSvenCoop;
extern bool g_bIsCounterStrike;

extern ref_params_t r_params;

//For Counter-Strike
typedef struct extra_player_info_s
{
	short frags;//00000000 frags           dw ? ; XREF: CounterStrikeViewport::MsgFunc_TeamInfo(char const*, int, void *) + F8 / w
	short deaths;//00000002 deaths          dw ?
	short team_id; //00000004 team_id         dw ?
	short padding;//00000006                 db ? ; undefined
	int has_c4;//00000008 has_c4          dd ?
	int vip;//0000000C vip             dd ?
	vec3_t origin;//00000010 origin          Vector ?
	int radarflash;//0000001C radarflash      dd ?
	int radarflashon;//00000020 radarflashon    dd ?
	int radarflashes;//00000024 radarflashes    dd ?
	short playerclass;//00000028 playerclass     dw ?
	short teamnumber;//0000002A teamnumber      dw ?
	char teamname[16];//0000002C teamname        db 16 dup(? )
	char dead;//0000003C dead
	float showhealth;//0x40
	int health;//0x44
	char location[32];//0x48
	int sb_health;//0x68
	int sb_account;//0x6C
	int has_defuse_kit;//0x70
}extra_player_info_t;

static_assert(sizeof(extra_player_info_t) == 0x74, "Size check");

extern cvar_t *cl_minmodels;
extern cvar_t *cl_min_t;
extern cvar_t *cl_min_ct;
extern extra_player_info_t(*g_PlayerExtraInfo)[65];

int EngineGetMaxKnownModel();
int EngineGetNumKnownModel();
int EngineGetModelIndex(model_t* mod);
model_t* EngineGetModelByIndex(int index);