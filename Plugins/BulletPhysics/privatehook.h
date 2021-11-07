#pragma once

typedef struct
{
	void(*R_NewMap)(void);
	void(*R_RecursiveWorldNode)(void *node);

	//Client GameStudioRenderer
	int(__fastcall *GameStudioRenderer_StudioDrawModel)(void *pthis, int, int flags);
	int (__fastcall *GameStudioRenderer_StudioDrawPlayer)(void *pthis, int, int flags, struct entity_state_s *pplayer);
	void(__fastcall *GameStudioRenderer_StudioSetupBones)(void *pthis, int);

	//Engine StudioRenderer
	int (*R_StudioDrawModel)(int flags);
	int (*R_StudioDrawPlayer)(int flags, struct entity_state_s *pplayer);
	void (*R_StudioSetupBones)(void);

	void (*FirstPerson_f)(void);
	void (*ThreadPerson_f)(void);
}privte_funcs_t;

extern int *r_visframecount;
extern int *cl_parsecount;
extern void *mod_known;
extern int *mod_numknown;
extern privte_funcs_t gPrivateFuncs;