#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"

void IN_ActivateMouse(void);

void __fastcall CClient_SoundEngine_FlushCache(int pthis, int dummy, qboolean including_local);

int NewClientCmd(const char *szCmdString);

typedef struct
{
	xcommand_t Connect_f;

	xcommand_t Retry_f;

	void(__fastcall *CClient_SoundEngine_FlushCache)(int pthis, int dummy, qboolean including_local);

	int(*pfnClientCmd)(const char *szCmdString);

}private_funcs_t;

extern private_funcs_t gPrivateFuncs;