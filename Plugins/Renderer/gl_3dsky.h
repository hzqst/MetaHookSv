#pragma once

typedef struct
{
	vec3_t maxs;
	vec3_t mins;
	vec3_t camera;
	vec3_t center;
	qboolean enable;
	float scale;
}r_3dsky_parm_t;

extern qboolean draw3dsky;
extern vec3_t _3dsky_view;
extern float _3dsky_mvmatrix[16];
extern mplane_t _3dsky_frustum[4];
extern r_3dsky_parm_t r_3dsky_parm;

extern cvar_t *r_3dsky;
extern cvar_t *r_3dsky_debug;

void R_Init3DSky(void);
void R_Clear3DSky(void);
void R_Render3DSky(void);
void R_ViewOriginFor3DSky(float *org);
void R_Draw3DSkyEntities(void);
void R_Add3DSkyEntity(cl_entity_t *ent);
void R_Setup3DSkyModel(void);
void R_Finish3DSkyModel(void);