#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"
#include "com_model.h"
#include "triangleapi.h"
#include "cvardef.h"
#include "exportfuncs.h"
#include "entity_types.h"
#include "plugins.h"
#include <string>
#include <sstream>

cl_enginefunc_t gEngfuncs;
engine_studio_api_t IEngineStudio;
r_studio_interface_t **gpStudioInterface;

resource_t *cl_resourcesonhand = NULL;

void FS_Dump_Precaches(void)
{
	const char *mapname = gEngfuncs.pfnGetLevelName();

	if (!mapname || !mapname[0])
	{
		gEngfuncs.Con_Printf("FS_Dump_Precaches: Cannot dump precache resource list because you are not in an active map!\n");
		return;
	}

	std::string filename = mapname;

	filename = filename.substr(0, filename.length() - 4);

	filename = filename + ".dump.res";

	auto FileHandle = FILESYSTEM_ANY_OPEN(filename.c_str(), "wt");

	if (!FileHandle)
	{
		gEngfuncs.Con_Printf("FS_Dump_Precaches: Could not open \"%s\" for writing!\n", filename.c_str());
		return;
	}

	resource_t *pResource;
	resource_t *next;

	pResource = (*cl_resourcesonhand).pNext;

	while (pResource && pResource != cl_resourcesonhand)
	{
		next = pResource->pNext;

		if ((pResource->ucFlags & RES_PRECACHED))
		{
			switch (pResource->type)
			{
			case t_sound:
			{
				FILESYSTEM_ANY_WRITE(pResource->szFileName, strlen(pResource->szFileName), FileHandle);
				FILESYSTEM_ANY_WRITE("\n", 1, FileHandle);
				break;
			}

			case t_model:
			{
				if (pResource->szFileName[0] != '*')
				{
					FILESYSTEM_ANY_WRITE(pResource->szFileName, strlen(pResource->szFileName), FileHandle);
					FILESYSTEM_ANY_WRITE("\n", 1, FileHandle);
				}

				break;
			}
			case t_generic:
			{
				FILESYSTEM_ANY_WRITE(pResource->szFileName, strlen(pResource->szFileName), FileHandle);
				FILESYSTEM_ANY_WRITE("\n", 1, FileHandle);
				break;
			}
			}
		}
		pResource = next;
	}

	FILESYSTEM_ANY_CLOSE(FileHandle);

	gEngfuncs.Con_Printf("FS_Dump_Precaches: Precached resources dumpped into %s.\n", filename.c_str());
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	gEngfuncs.pfnAddCommand("fs_dump_precaches", FS_Dump_Precaches);
}