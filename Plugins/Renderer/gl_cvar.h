#pragma once

#include <cvardef.h>

class MapConVar
{
public:
	MapConVar(char *cvar_name, char *default_value, int cvar_flags, int numargs, int flags);
	~MapConVar();
	float GetValue();
	float *GetValues();

	cvar_t *m_cvar;
	bool m_is_map_override;
	int m_numargs;
	int m_flags;
	float m_map_value[4];
	float m_cvar_value[4];
};

class StudioConVar
{
public:
	StudioConVar();
	StudioConVar(cvar_t *cvar, int numargs, int flags);
	~StudioConVar();

	void Init(cvar_t *cvar, int numargs, int flags);

	float GetValue();
	bool GetValues(float *vec);

	cvar_t *m_cvar;
	bool m_is_override;
	int m_numargs;
	int m_flags;
	float m_override_value[4];
};

#define ConVar_Color255 1

MapConVar *R_RegisterMapCvar(char *cvar_name, char *default_value, int cvar_flags, int numargs = 1, int flags = 0);
//void R_CvarSetMapCvar(cvar_t *cvar, char *value);
void R_ParseMapCvarSetMapValue(MapConVar *mapcvar, char *value);
void R_ParseMapCvarSetCvarValue(MapConVar *mapcvar, char *value);
void R_FreeMapCvars(void);
//void Cvar_DirectSet(cvar_t *var, char *value);