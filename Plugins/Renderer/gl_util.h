inline void Cvar_DirectSet(cvar_t *var, char *value)
{
	gEngfuncs.Cvar_Set(var->name, value);
}