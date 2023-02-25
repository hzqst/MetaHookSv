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
		return;

	std::string filename = mapname;

	filename = filename.substr(0, filename.length() - 4);

	filename = filename + ".dump.res";

	auto FileHandle = g_pFileSystem->Open(filename.c_str(), "wt");

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
				g_pFileSystem->Write(pResource->szFileName, strlen(pResource->szFileName), FileHandle);
				g_pFileSystem->Write("\n", 1, FileHandle);
				break;
			}

			case t_model:
			{
				if (pResource->szFileName[0] != '*')
				{
					g_pFileSystem->Write(pResource->szFileName, strlen(pResource->szFileName), FileHandle);
					g_pFileSystem->Write("\n", 1, FileHandle);
				}

				break;
			}
			case t_generic:
			{
				g_pFileSystem->Write(pResource->szFileName, strlen(pResource->szFileName), FileHandle);
				g_pFileSystem->Write("\n", 1, FileHandle);
				break;
			}
			}
		}
		pResource = next;
	}

	g_pFileSystem->Close(FileHandle);;
}

void HUD_Init(void)
{
	gExportfuncs.HUD_Init();

	gEngfuncs.pfnSetFilterMode(1);
	gEngfuncs.pfnSetFilterColor(0, 0.5f, 1);
	gEngfuncs.pfnSetFilterBrightness(1);

	gEngfuncs.pfnAddCommand("fs_dump_precaches", FS_Dump_Precaches);
}