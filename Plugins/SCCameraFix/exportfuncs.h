#include <metahook.h>
#include <studio.h>
#include <r_studioint.h>
#include "cl_entity.h"

void CAM_Think(void);
void HUD_Init(void);
void V_CalcRefdef(struct ref_params_s* pparams);

void Client_FillAddress(void);
void Client_InstallHooks(void);
void Client_UninstallHooks(void);