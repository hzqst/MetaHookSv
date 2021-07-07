#pragma once

typedef struct
{
	void(*R_NewMap)(void);

	//Client Studio
	int(*StudioDrawPlayer)(int flags, struct entity_state_s *pplayer);
	int(*StudioDrawModel)(int flags);
	void(__fastcall *StudioSetupBones)(void *pthis, int);
}privte_funcs_t;

extern privte_funcs_t gPrivateFuncs;