#include "gl_local.h"

std::vector<MapConVar *> g_MapConVars;

MapConVar::MapConVar(char *cvar_name, char *default_value, int cvar_flags, int numargs, int flags)
{
	m_cvar = gEngfuncs.pfnRegisterVariable(cvar_name, default_value, FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_is_map_override = false;
	m_numargs = numargs;
	m_flags = flags;
	memset(m_map_value, 0, sizeof(m_map_value));
	memset(m_cvar_value, 0, sizeof(m_cvar_value));
}

MapConVar::~MapConVar()
{
	m_cvar = NULL;
}

float *MapConVar::GetValues()
{
	if (m_is_map_override)
		return m_map_value;

	return m_cvar_value;
}

void MapConVar::FetchValues(float *values)
{
	memcpy(values, GetValues(), sizeof(float) * m_numargs);
}

float MapConVar::GetValue()
{
	return GetValues()[0];
}

int MapConVar::GetNumArgs() const
{
	return m_numargs;
}

int MapConVar::GetFlags() const
{
	return m_flags;
}

bool MapConVar::IsMapOverride() const
{
	return m_is_map_override;
}

void MapConVar::SetMapOverride(bool bIsMapOverride)
{
	m_is_map_override = bIsMapOverride;
}

float* MapConVar::GetRawMapValues()
{
	return m_map_value;
}

float* MapConVar::GetRawCvarValues()
{
	return m_cvar_value;
}

cvar_t* MapConVar::GetRawCvarPointer()
{
	return m_cvar;
}

StudioConVar::~StudioConVar()
{
	m_mapcvar = NULL;
	m_cvar = NULL;
}

StudioConVar::StudioConVar()
{
	m_mapcvar = NULL;
	m_cvar = NULL;
	m_is_override = false;
	m_numargs = 0;
	m_flags = 0;
	m_override_value[0] = 0;
	m_override_value[1] = 0;
	m_override_value[2] = 0;
	m_override_value[3] = 0;
}

StudioConVar::StudioConVar(MapConVar* mapcvar, int numargs, int flags)
{
	Init(mapcvar, numargs, flags);
}

StudioConVar::StudioConVar(cvar_t *cvar, int numargs, int flags)
{
	Init(cvar, numargs, flags);
}

void StudioConVar::Init(cvar_t *cvar, int numargs, int flags)
{
	m_mapcvar = NULL;
	m_cvar = cvar;
	m_is_override = false;
	m_numargs = numargs;
	m_flags = flags;
	m_override_value[0] = 0;
	m_override_value[1] = 0;
	m_override_value[2] = 0;
	m_override_value[3] = 0;
}

void StudioConVar::Init(MapConVar* mapcvar, int numargs, int flags)
{
	m_mapcvar = mapcvar;
	m_cvar = NULL;
	m_is_override = false;
	m_numargs = numargs;
	m_flags = flags;
	m_override_value[0] = 0;
	m_override_value[1] = 0;
	m_override_value[2] = 0;
	m_override_value[3] = 0;
}

bool StudioConVar::GetValues(float *vec)
{
	if (m_is_override)
	{
		memcpy(vec, m_override_value, sizeof(float) * m_numargs);
		return true;
	}

	if (m_flags & ConVar_Color255)
	{
		if (m_mapcvar)
		{
			if (m_numargs >= 1 && m_numargs <= 4)
			{
				m_mapcvar->FetchValues(vec);
				return true;
			}
		}
		else if (m_cvar)
		{
			if (m_numargs == 4 && R_ParseCvarAsColor4(m_cvar, vec))
				return true;
			if (m_numargs == 3 && R_ParseCvarAsColor3(m_cvar, vec))
				return true;
			if (m_numargs == 2 && R_ParseCvarAsColor2(m_cvar, vec))
				return true;
			if (m_numargs == 1 && R_ParseCvarAsColor1(m_cvar, vec))
				return true;
		}
	}
	else
	{
		if (m_mapcvar)
		{
			if (m_numargs >= 1 && m_numargs <= 4)
			{
				m_mapcvar->FetchValues(vec);
				return true;
			}
		}
		else
		{
			if (m_numargs == 4 && R_ParseCvarAsVector4(m_cvar, vec))
				return true;
			if (m_numargs == 3 && R_ParseCvarAsVector3(m_cvar, vec))
				return true;
			if (m_numargs == 2 && R_ParseCvarAsVector2(m_cvar, vec))
				return true;
			if (m_numargs == 1 && R_ParseCvarAsVector1(m_cvar, vec))
				return true;
		}
	}
	return false;
}

float StudioConVar::GetValue()
{
	if (m_is_override)
	{
		return m_override_value[0];
	}

	return m_cvar->value;
}


bool StudioConVar::IsOverride()const
{
	return m_is_override;
}

void StudioConVar::SetOverride(bool bIsOverride)
{
	m_is_override = bIsOverride;
}

void StudioConVar::SetOverrideValues(const float* values)
{
	memcpy(m_override_value, values, sizeof(float) * m_numargs);
}

void R_CvarSetMapCvar(cvar_t *cvar)
{
	for (size_t i = 0; i < g_MapConVars.size(); ++i)
	{
		auto mapcvar = g_MapConVars[i];

		if (mapcvar->GetRawCvarPointer() == cvar)
		{
			R_ParseMapCvarSetCvarValue(mapcvar, cvar->string);
			break;
		}
	}
}

MapConVar *R_RegisterMapCvar(char *cvar_name, char *default_value, int cvar_flags, int numargs, int flags)
{
	auto mapcvar = new MapConVar(cvar_name, default_value, cvar_flags, numargs, flags);

	R_ParseMapCvarSetCvarValue(mapcvar, default_value);

	g_MapConVars.emplace_back(mapcvar);

	g_pMetaHookAPI->RegisterCvarCallback(cvar_name, R_CvarSetMapCvar, NULL);

	return mapcvar;
}

void R_FreeMapCvars(void)
{
	for (size_t i = 0; i < g_MapConVars.size(); ++i)
	{
		delete g_MapConVars[i];
		g_MapConVars[i] = NULL;
	}
	g_MapConVars.clear();
}

void R_ParseMapCvarSetMapValue(MapConVar *mapcvar, const char *value)
{
	if (!value)
		return;

	if (mapcvar->GetNumArgs() == 4)
	{
		if (mapcvar->GetFlags() & ConVar_Color255)
		{
			if (R_ParseStringAsColor4(value, mapcvar->GetRawMapValues()))
			{
				mapcvar->SetMapOverride(true);
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector4(value, mapcvar->GetRawMapValues()))
			{
				mapcvar->SetMapOverride(true);
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
	}
	if (mapcvar->GetNumArgs() == 3)
	{
		if (mapcvar->GetFlags() & ConVar_Color255)
		{
			if (R_ParseStringAsColor3(value, mapcvar->GetRawMapValues()))
			{
				mapcvar->SetMapOverride(true);
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector3(value, mapcvar->GetRawMapValues()))
			{
				mapcvar->SetMapOverride(true);
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
	}
	if (mapcvar->GetNumArgs() == 2)
	{
		if (mapcvar->GetFlags() & ConVar_Color255)
		{
			if (R_ParseStringAsColor2(value, mapcvar->GetRawMapValues()))
			{
				mapcvar->SetMapOverride(true);
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector2(value, mapcvar->GetRawMapValues()))
			{
				mapcvar->SetMapOverride(true);
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
	}
	if (mapcvar->GetNumArgs() == 1)
	{
		if (mapcvar->GetFlags() & ConVar_Color255)
		{
			if (R_ParseStringAsColor1(value, mapcvar->GetRawMapValues()))
			{
				mapcvar->SetMapOverride(true);
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector1(value, mapcvar->GetRawMapValues()))
			{
				mapcvar->SetMapOverride(true);
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
	}
}

void R_ParseMapCvarSetCvarValue(MapConVar *mapcvar, const char *value)
{
	if (mapcvar->GetNumArgs() == 4)
	{
		if (mapcvar->GetFlags() & ConVar_Color255)
		{
			if (R_ParseStringAsColor4(value, mapcvar->GetRawCvarValues()))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector4(value, mapcvar->GetRawCvarValues()))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
	}
	if (mapcvar->GetNumArgs() == 3)
	{
		if (mapcvar->GetFlags() & ConVar_Color255)
		{
			if (R_ParseStringAsColor3(value, mapcvar->GetRawCvarValues()))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector3(value, mapcvar->GetRawCvarValues()))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
	}
	if (mapcvar->GetNumArgs() == 2)
	{
		if (mapcvar->GetFlags() & ConVar_Color255)
		{
			if (R_ParseStringAsColor2(value, mapcvar->GetRawCvarValues()))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector2(value, mapcvar->GetRawCvarValues()))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
	}
	else if (mapcvar->GetNumArgs() == 1)
	{
		if (mapcvar->GetFlags() & ConVar_Color255)
		{
			if (R_ParseStringAsColor1(value, mapcvar->GetRawCvarValues()))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector1(value, mapcvar->GetRawCvarValues()))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->GetRawCvarPointer()->name);
			}
		}
	}
}