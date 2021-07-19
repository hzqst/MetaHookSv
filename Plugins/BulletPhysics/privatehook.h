#pragma once

typedef struct
{
	void(*R_NewMap)(void);
	void(*R_RecursiveWorldNode)(void *node);

	//Client Studio
	int(__fastcall *StudioDrawModel)(void *pthis, int, int flags);
	int (__fastcall *StudioDrawPlayer)(void *pthis, int, int flags, struct entity_state_s *pplayer);

	void(__fastcall *StudioSetupBones)(void *pthis, int);
	void (*FirstPerson_f)(void);
	void (*ThreadPerson_f)(void);
}privte_funcs_t;

extern int *r_visframecount;
extern privte_funcs_t gPrivateFuncs;