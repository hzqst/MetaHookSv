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
