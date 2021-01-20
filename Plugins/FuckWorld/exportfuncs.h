extern cl_enginefunc_t gEngfuncs;

int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion);
int HUD_VidInit(void);
int HUD_Redraw(float time, int intermission);