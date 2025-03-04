#pragma once

#include <studio.h>
#include <com_model.h>

typedef struct walk_context_s
{
	walk_context_s(void *a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	void* address;
	size_t len;
	int depth;
}walk_context_t;

typedef struct
{
	//engine stuffs
	void(*R_NewMap)(void);
	void(*R_RecursiveWorldNode)(void *node);
	void(*R_DrawTEntitiesOnList)(int onlyClientDraw);
	//void(*V_SetRefParams)(ref_params_t *);
	void (*V_RenderView)(void);
	void (*R_RenderView)(void);
	void (*R_RenderView_SvEngine)(int viewIdx);
	qboolean(*R_CullBox)(vec3_t mins, vec3_t maxs);

	//client stuffs
	//void (*V_CalcNormalRefdef)(ref_params_t*);

	//Client GameStudioRenderer
	void(__fastcall* GameStudioRenderer_StudioSetupBones)(void* pthis, int);
	void(__fastcall* GameStudioRenderer_StudioMergeBones)(void* pthis, int, model_t* pSubModel);
	void(__fastcall* GameStudioRenderer_StudioSaveBones)(void* pthis, int);
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
	void (*R_StudioMergeBones)(void);
	void (*R_StudioSaveBones)(void);
	void (*R_StudioRenderModel)(void);
	void (*R_StudioRenderFinal)(void);

	//IEngineStudio
	int (*studioapi_StudioCheckBBox)(void);

	void (*FirstPerson_f)(void);
	void (*ThreadPerson_f)(void);

	//Engine model managment
	// unused
	//void (*Mod_LoadStudioModel)(model_t* mod, void* buffer);

	//efxapi
	TEMPENTITY	*(*efxapi_R_TempModel)				(float *pos, float *dir, float *angles, float life, int modelIndex, int soundtype);
}private_funcs_t;

void R_NewMap(void);
void R_RenderView_SvEngine(int viewIdx);
void R_RenderView();

TEMPENTITY* efxapi_R_TempModel(float* pos, float* dir, float* angles, float life, int modelIndex, int soundtype);

void Engine_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Engine_InstallHook(void);
void Engine_UninstallHook(void);
void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo);
void Client_InstallHooks(void);
void ClientStudio_InstallHooks();
void EngineStudio_InstallHooks(void);
void ClientStudio_UninstallHooks(void);
void EngineStudio_UninstallHooks(void);

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo);
PVOID GetVFunctionFromVFTable(PVOID* vftable, int index, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo, const mh_dll_info_t& OutputDllInfo);

extern studiohdr_t** pstudiohdr;
extern model_t** r_model;
extern void* g_pGameStudioRenderer;
extern float(*pbonetransform)[128][3][4];
extern float(*plighttransform)[128][3][4];

//extern int* r_framecount;
//extern int *r_visframecount;
extern int *cl_parsecount;
extern void *cl_frames;
extern int size_of_frame;
extern int *cl_viewentity;
extern cl_entity_t **currententity;
extern void *mod_known;
extern int *mod_numknown;
extern TEMPENTITY *gTempEnts;

extern int* allow_cheats;

extern int* g_iWaterLevel;
extern bool* g_bRenderingPortals_SCClient;
extern int* g_ViewEntityIndex_SCClient;

struct pitchdrift_t
{
	float pitchvel;
	bool nodrift;
	float driftmove;
	double laststop;
};

extern struct pitchdrift_t* g_pitchdrift;

extern int *g_iUser1;
extern int *g_iUser2;

extern private_funcs_t gPrivateFuncs;

extern bool g_bIsSvenCoop;
extern bool g_bIsCounterStrike;
extern bool g_bIsDayOfDefeat;

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
	float radarflash;//0000001C radarflash      dd ?
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

typedef struct extra_player_info_czds_s 
{                      
    short frags;//0
	short deaths;   //2
	short playerclass;//4
	short health;//6
	char dead;//8
	char padding;
	short teamnumber;//0xA
    char teamname[16];//0xC
}extra_player_info_czds_t;

static_assert(sizeof(extra_player_info_czds_t) == 0x1C, "Size check");

 typedef struct team_info_s  // sizeof=0x28
 {                                       // XREF: CClientScoreBoardDialog::UpdateTeamInfo(void)+8F/o
                                         // TeamFortressViewport::MsgFunc_TeamNames(char const*,int,void *)+31/o ...
     char name[16];
     short frags;
     short deaths;                     // XREF: CClientScoreBoardDialog::UpdateTeamInfo(void)+17B/o
     short ping;                       // XREF: CClientScoreBoardDialog::UpdateTeamInfo(void)+135/o
     short packetloss;
     short ownteam;
     short players;
     int already_drawn;                  // XREF: CClientScoreBoardDialog::UpdateTeamInfo(void)+1EB/w
     int scores_overriden;               // XREF: CClientScoreBoardDialog::UpdateTeamInfo(void)+1C/o
     int teamnumber;
 } team_info_t;

 static_assert(sizeof(team_info_t) == 0x28, "Size check");

extern cvar_t *cl_minmodels;
extern cvar_t *cl_min_t;
extern cvar_t *cl_min_ct;
extern extra_player_info_t(*g_PlayerExtraInfo)[65];
extern extra_player_info_czds_t(*g_PlayerExtraInfo_CZDS)[65];

int EngineGetMaxKnownModel();
int EngineGetNumKnownModel();
int EngineGetModelIndex(model_t* mod);
model_t* EngineGetModelByIndex(int index);
model_t* EngineFindWorldModelBySubModel(model_t* psubmodel);