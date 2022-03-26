#include "gl_local.h"

std::vector<MapConVar *> g_MapConVars;

MapConVar::MapConVar(char *cvar_name, char *default_value, int cvar_flags, int numargs, int flags)
{
	m_cvar = gEngfuncs.pfnRegisterVariable(cvar_name, default_value, FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_is_map_override = false;
	m_numargs = numargs;
	m_flags = flags;
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

float MapConVar::GetValue()
{
	return GetValues()[0];
}

StudioConVar::StudioConVar()
{
	m_cvar = NULL;
	m_is_override = false;
	m_numargs = 0;
	m_flags = 0;
	m_override_value[0] = 0;
	m_override_value[1] = 0;
	m_override_value[2] = 0;
	m_override_value[3] = 0;
}

StudioConVar::StudioConVar(cvar_t *cvar, int numargs, int flags)
{
	Init(cvar, numargs, flags);
}

StudioConVar::~StudioConVar()
{
	m_cvar = NULL;
}

void StudioConVar::Init(cvar_t *cvar, int numargs, int flags)
{
	m_cvar = cvar;
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
		if (m_numargs == 4 && R_ParseCvarAsColor4(m_cvar, vec))
			return true;
		if (m_numargs == 3 && R_ParseCvarAsColor3(m_cvar, vec))
			return true;
		if (m_numargs == 2 && R_ParseCvarAsColor2(m_cvar, vec))
			return true;
		if (m_numargs == 1 && R_ParseCvarAsColor1(m_cvar, vec))
			return true;
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

MapConVar *R_RegisterMapCvar(char *cvar_name, char *default_value, int cvar_flags, int numargs, int flags)
{
	auto mapcvar = new MapConVar(cvar_name, default_value, cvar_flags, numargs, flags);

	R_ParseMapCvarSetCvarValue(mapcvar, default_value);

	g_MapConVars.emplace_back(mapcvar);

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

void R_ParseMapCvarSetMapValue(MapConVar *mapcvar, char *value)
{
	if (!value)
		return;

	if (mapcvar->m_numargs == 4)
	{
		if (mapcvar->m_flags & ConVar_Color255)
		{
			if (R_ParseStringAsColor4(value, mapcvar->m_map_value))
			{
				mapcvar->m_is_map_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector4(value, mapcvar->m_map_value))
			{
				mapcvar->m_is_map_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
	}
	if (mapcvar->m_numargs == 3)
	{
		if (mapcvar->m_flags & ConVar_Color255)
		{
			if (R_ParseStringAsColor3(value, mapcvar->m_map_value))
			{
				mapcvar->m_is_map_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector3(value, mapcvar->m_map_value))
			{
				mapcvar->m_is_map_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
	}
	if (mapcvar->m_numargs == 2)
	{
		if (mapcvar->m_flags & ConVar_Color255)
		{
			if (R_ParseStringAsColor2(value, mapcvar->m_map_value))
			{
				mapcvar->m_is_map_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector2(value, mapcvar->m_map_value))
			{
				mapcvar->m_is_map_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
	}
	if (mapcvar->m_numargs == 1)
	{
		if (mapcvar->m_flags & ConVar_Color255)
		{
			if (R_ParseStringAsColor1(value, mapcvar->m_map_value))
			{
				mapcvar->m_is_map_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector1(value, mapcvar->m_map_value))
			{
				mapcvar->m_is_map_override = true;
			}
			else
			{
				gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
	}
}

void R_ParseMapCvarSetCvarValue(MapConVar *mapcvar, char *value)
{
	if (mapcvar->m_numargs == 4)
	{
		if (mapcvar->m_flags & ConVar_Color255)
		{
			if (R_ParseStringAsColor4(value, mapcvar->m_cvar_value))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector4(value, mapcvar->m_cvar_value))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
	}
	if (mapcvar->m_numargs == 3)
	{
		if (mapcvar->m_flags & ConVar_Color255)
		{
			if (R_ParseStringAsColor3(value, mapcvar->m_cvar_value))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector3(value, mapcvar->m_cvar_value))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
	}
	if (mapcvar->m_numargs == 2)
	{
		if (mapcvar->m_flags & ConVar_Color255)
		{
			if (R_ParseStringAsColor2(value, mapcvar->m_cvar_value))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector2(value, mapcvar->m_cvar_value))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
	}
	else if (mapcvar->m_numargs == 1)
	{
		if (mapcvar->m_flags & ConVar_Color255)
		{
			if (R_ParseStringAsColor1(value, mapcvar->m_cvar_value))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
		else
		{
			if (R_ParseStringAsVector1(value, mapcvar->m_cvar_value))
			{

			}
			else
			{
				gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
			}
		}
	}
}

void R_CvarSetMapCvar(cvar_t *cvar, char *value)
{
	for (size_t i = 0; i < g_MapConVars.size(); ++i)
	{
		auto mapcvar = g_MapConVars[i];
		if (mapcvar->m_cvar == cvar)
		{
			R_ParseMapCvarSetCvarValue(mapcvar, value);
			break;
		}
	}
}

void Cvar_DirectSet(cvar_t *var, char *value)
{
	gRefFuncs.Cvar_DirectSet(var, value);

	R_CvarSetMapCvar(var, value);
}