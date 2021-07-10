#pragma once

typedef struct
{
	void(*R_NewMap)(void);

	//Client Studio
	int (__fastcall *StudioDrawPlayer)(void *pthis, int, int flags, struct entity_state_s *pplayer);

	void(__fastcall *StudioSetupBones)(void *pthis, int);
}privte_funcs_t;

extern privte_funcs_t gPrivateFuncs;