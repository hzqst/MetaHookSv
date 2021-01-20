#include <metahook.h>
#include <net_api.h>
#include <cvardef.h>
#include "mempatchs.h"
#include "plugins.h"

void MemPatch_BlockSniperScope(void)
{
	if (g_dwEngineBuildnum >= 5953)
		return;

	unsigned char data[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC2, 0x04, 0x00 };
	DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)g_pMetaSave->pExportFuncs->Initialize, 0x100000, "\x83\xEC\x50\xA1\x2A\x2A\x2A\x2A\x89\x4C\x24\x00\x85\xC0", 14);

	if (!addr)
	{
		MessageBox(NULL, "BlockSniperScope patch failed!", "Warning", MB_ICONWARNING);
		return;
	}

	g_pMetaHookAPI->WriteMemory((void *)addr, data, sizeof(data));
}

void MemPatch_Start(MEMPATCH_STEP step)
{
	switch (step)
	{
		case MEMPATCH_STEP_LOADENGINE:
		{
			break;
		}

		case MEMPATCH_STEP_LOADCLIENT:
		{
			break;
		}

		case MEMPATCH_STEP_INITCLIENT:
		{
			MemPatch_BlockSniperScope();
			break;
		}
	}
}