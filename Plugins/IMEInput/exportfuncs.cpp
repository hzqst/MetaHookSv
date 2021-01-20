#include <metahook.h>
#include "IMEInput.h"
#include "UnicodeVoice.h"

cl_enginefunc_t gEngfuncs;

int Initialize(struct cl_enginefuncs_s *pEnginefuncs, int iVersion)
{
	memcpy(&gEngfuncs, pEnginefuncs, sizeof(gEngfuncs));

	pEnginefuncs->pfnGetPlayerInfo = GetPlayerInfo;

	INEIN_InstallHook();

	return gExportfuncs.Initialize(pEnginefuncs, iVersion);
}