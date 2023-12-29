#pragma once

#include <cvardef.h>

class MapConVar
{
public:
	MapConVar(char *cvar_name, char *default_value, int cvar_flags, int numargs, int flags);
	~MapConVar();
	float GetValue();
	float *GetValues();
	void FetchValues(float* values);
	int GetNumArgs() const;
	int GetFlags() const;
	float* GetRawMapValues();
	float* GetRawCvarValues();
	cvar_t* GetRawCvarPointer();
	bool IsMapOverride() const;
	void SetMapOverride(bool bIsMapOverride);
private:
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
	~StudioConVar();
	StudioConVar();
	StudioConVar(cvar_t *cvar, int numargs, int flags); 
	StudioConVar(MapConVar* mapcvar, int numargs, int flags);

	void Init(cvar_t *cvar, int numargs, int flags);
	void Init(MapConVar* mapcvar, int numargs, int flags);

	float GetValue();
	bool GetValues(float *vec);

	bool IsOverride()const;
	void SetOverride(bool bIsOverride);

	void SetOverrideValues(const float* values);
private:
	MapConVar* m_mapcvar;
	cvar_t *m_cvar;
	bool m_is_override;
	int m_numargs;
	int m_flags;
	float m_override_value[4];
};

#define ConVar_None 0
#define ConVar_Color255 1

MapConVar *R_RegisterMapCvar(char *cvar_name, char *default_value, int cvar_flags, int numargs = 1, int flags = 0);
void R_ParseMapCvarSetMapValue(MapConVar *mapcvar, const char *value);
void R_ParseMapCvarSetCvarValue(MapConVar *mapcvar, const char *value);
void R_FreeMapCvars(void);