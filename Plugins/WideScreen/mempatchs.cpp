#include <metahook.h>
#include <net_api.h>
#include <cvardef.h>
#include "mempatchs.h"
#include "plugins.h"

void MemPatch_WideScreenLimit(void)
{
	if (g_dwEngineBuildnum >= 5953)
		return;

	DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, "\x8B\x51\x08\x8B\x41\x0C\x8B\x71\x54\x8B\xFA\xC1\xE7\x04", 14);

	if (!addr)
	{
		MessageBox(NULL, "WideScreenLimit patch failed!", "Warning", MB_ICONWARNING);
		return;
	}

	DWORD addr2 = addr + 11;
	DWORD addr3 =  (DWORD)g_pMetaHookAPI->SearchPattern((void *)addr, 0x60, "\xB1\x01\x8B\x7C\x24\x14", 6);

	if (addr3)
	{
		g_pMetaHookAPI->WriteNOP((void *)addr2, addr3 - addr2);
	}
}

void MemPatch_Start(MEMPATCH_STEP step)
{
	switch (step)
	{
		case MEMPATCH_STEP_LOADENGINE:
		{
			MemPatch_WideScreenLimit();
			break;
		}

		case MEMPATCH_STEP_LOADCLIENT:
		{
			break;
		}

		case MEMPATCH_STEP_INITCLIENT:
		{
			break;
		}
	}
}