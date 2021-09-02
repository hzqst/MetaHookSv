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
	}
}

void R_ParseMapCvarSetMapValue(MapConVar *mapcvar, char *value)
{
	if (!value)
		return;

	if (mapcvar->m_numargs == 4)
	{
		int result = sscanf_s(value, "%f %f %f %f", &mapcvar->m_map_value[0], &mapcvar->m_map_value[1], &mapcvar->m_map_value[2], &mapcvar->m_map_value[3]);

		if (result != 4)
		{
			gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
		}
		else
		{
			mapcvar->m_is_map_override = true;

			if (mapcvar->m_flags & MapConVar_Color255)
			{
				mapcvar->m_map_value[0] = clamp(mapcvar->m_map_value[0] / 255.0, 0, 1);
				mapcvar->m_map_value[1] = clamp(mapcvar->m_map_value[1] / 255.0, 0, 1);
				mapcvar->m_map_value[2] = clamp(mapcvar->m_map_value[2] / 255.0, 0, 1);
				mapcvar->m_map_value[3] = clamp(mapcvar->m_map_value[3] / 255.0, 0, 1);
			}
		}
	}
	else if (mapcvar->m_numargs == 3)
	{
		int result = sscanf_s(value, "%f %f %f", &mapcvar->m_map_value[0], &mapcvar->m_map_value[1], &mapcvar->m_map_value[2]);
		mapcvar->m_map_value[3] = 0;

		if (result != 3)
		{
			gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
		}
		else
		{
			mapcvar->m_is_map_override = true;

			if (mapcvar->m_flags & MapConVar_Color255)
			{
				mapcvar->m_map_value[0] = clamp(mapcvar->m_map_value[0] / 255.0, 0, 1);
				mapcvar->m_map_value[1] = clamp(mapcvar->m_map_value[1] / 255.0, 0, 1);
				mapcvar->m_map_value[2] = clamp(mapcvar->m_map_value[2] / 255.0, 0, 1);
			}
		}
	}
	else if (mapcvar->m_numargs == 2)
	{
		int result = sscanf_s(value, "%f %f", &mapcvar->m_map_value[0], &mapcvar->m_map_value[1]);
		mapcvar->m_map_value[2] = 0;
		mapcvar->m_map_value[3] = 0;

		if (result != 2)
		{
			gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
		}
		else
		{
			mapcvar->m_is_map_override = true;

			if (mapcvar->m_flags & MapConVar_Color255)
			{
				mapcvar->m_map_value[0] = clamp(mapcvar->m_map_value[0] / 255.0, 0, 1);
				mapcvar->m_map_value[1] = clamp(mapcvar->m_map_value[1] / 255.0, 0, 1);
			}
		}
	}
	else if (mapcvar->m_numargs == 1)
	{
		int result = sscanf_s(value, "%f", &mapcvar->m_map_value[0]);
		mapcvar->m_map_value[1] = 0;
		mapcvar->m_map_value[2] = 0;
		mapcvar->m_map_value[3] = 0;

		if (result != 1)
		{
			gEngfuncs.Con_Printf("R_ParseMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
		}
		else
		{
			mapcvar->m_is_map_override = true;

			if (mapcvar->m_flags & MapConVar_Color255)
			{
				mapcvar->m_map_value[0] = clamp(mapcvar->m_map_value[0] / 255.0, 0, 1);
			}
		}
	}
}

void R_ParseMapCvarSetCvarValue(MapConVar *mapcvar, char *value)
{
	if (mapcvar->m_numargs == 4)
	{
		int result = sscanf_s(value, "%f %f %f %f", &mapcvar->m_cvar_value[0], &mapcvar->m_cvar_value[1], &mapcvar->m_cvar_value[2], &mapcvar->m_cvar_value[3]);
		if (result != 4)
		{
			gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
		}
		else
		{
			if (mapcvar->m_flags & MapConVar_Color255)
			{
				mapcvar->m_cvar_value[0] = clamp(mapcvar->m_cvar_value[0] / 255.0, 0, 1);
				mapcvar->m_cvar_value[1] = clamp(mapcvar->m_cvar_value[1] / 255.0, 0, 1);
				mapcvar->m_cvar_value[2] = clamp(mapcvar->m_cvar_value[2] / 255.0, 0, 1);
				mapcvar->m_cvar_value[3] = clamp(mapcvar->m_cvar_value[3] / 255.0, 0, 1);
			}
		}
	}
	else if (mapcvar->m_numargs == 3)
	{
		int result = sscanf_s(value, "%f %f %f", &mapcvar->m_cvar_value[0], &mapcvar->m_cvar_value[1], &mapcvar->m_cvar_value[2]);
		mapcvar->m_cvar_value[3] = 0;
		if (result != 3)
		{
			gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
		}
		else
		{
			if (mapcvar->m_flags & MapConVar_Color255)
			{
				mapcvar->m_cvar_value[0] = clamp(mapcvar->m_cvar_value[0] / 255.0, 0, 1);
				mapcvar->m_cvar_value[1] = clamp(mapcvar->m_cvar_value[1] / 255.0, 0, 1);
				mapcvar->m_cvar_value[2] = clamp(mapcvar->m_cvar_value[2] / 255.0, 0, 1);
			}
		}
	}
	else if (mapcvar->m_numargs == 2)
	{
		int result = sscanf_s(value, "%f %f", &mapcvar->m_cvar_value[0], &mapcvar->m_cvar_value[1]);
		mapcvar->m_cvar_value[2] = 0;
		mapcvar->m_cvar_value[3] = 0;
		if (result != 2)
		{
			gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
		}
		else
		{
			if (mapcvar->m_flags & MapConVar_Color255)
			{
				mapcvar->m_cvar_value[0] = clamp(mapcvar->m_cvar_value[0] / 255.0, 0, 1);
				mapcvar->m_cvar_value[1] = clamp(mapcvar->m_cvar_value[1] / 255.0, 0, 1);
			}
		}
	}
	else if (mapcvar->m_numargs == 1)
	{
		int result = sscanf_s(value, "%f", &mapcvar->m_cvar_value[0]);
		mapcvar->m_cvar_value[1] = 0;
		mapcvar->m_cvar_value[2] = 0;
		mapcvar->m_cvar_value[3] = 0;
		if (result != 1)
		{
			gEngfuncs.Con_Printf("R_CvarSetMapCvar: Failed to parse map cvar for %s\n", mapcvar->m_cvar->name);
		}
		else
		{
			if (mapcvar->m_flags & MapConVar_Color255)
			{
				mapcvar->m_cvar_value[0] = clamp(mapcvar->m_cvar_value[0] / 255.0, 0, 1);
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