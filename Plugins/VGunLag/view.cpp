#include <metahook.h>
#include <cvardef.h>
#include "mathlib.h"

extern cvar_t *cl_vgunlag;

void (*g_pfnV_CalcGunAngle)(ref_params_s *pparams);
vec3_t v_lastFacing;

void V_CalcGunlLag(struct ref_params_s *pparams)
{
	float speed;
	float diff;
	vec3_t newOrigin;
	vec3_t vecDiff;
	vec3_t forward, right, up;
	float pitch;

	AngleVectors(pparams->viewangles, forward, right, up);

	if (pparams->frametime != 0.0f)
	{
		VectorSubtract(forward, v_lastFacing, vecDiff);

		speed = 5.0f;
		diff = VectorLength(vecDiff);

		if ((diff > cl_vgunlag->value) && (cl_vgunlag->value > 0.0f))
			speed *= diff * cl_vgunlag->value;

		v_lastFacing[0] += vecDiff[0] * (speed * pparams->frametime);
		v_lastFacing[1] += vecDiff[1] * (speed * pparams->frametime);
		v_lastFacing[2] += vecDiff[2] * (speed * pparams->frametime);

		VectorNormalize(v_lastFacing);

		newOrigin[0] = pparams->vieworg[0] + (vecDiff[0] * -1.0f) * speed;
		newOrigin[1] = pparams->vieworg[1] + (vecDiff[1] * -1.0f) * speed;
		newOrigin[2] = pparams->vieworg[2] + (vecDiff[2] * -1.0f) * speed;
	}

	if (cl_vgunlag->value > 0.0f)
	{
		pitch = pparams->viewangles[2];

		if (pitch > 180.0f)
			pitch -= 360.0f;
		else if (pitch < -180.0f)
			pitch += 360.0f;

		VectorScale(forward, -pitch * 0.035f, forward);
		VectorScale(right, -pitch * 0.03f, right);
		VectorScale(up, -pitch * 0.02f, up);
		VectorAdd(newOrigin, forward, newOrigin);
		VectorAdd(newOrigin, right, newOrigin);
		VectorAdd(newOrigin, up, newOrigin);
		VectorCopy(newOrigin, pparams->vieworg);
	}
}

void V_CalcGunAngle(ref_params_s *pparams)
{
	g_pfnV_CalcGunAngle(pparams);

	V_CalcGunlLag(pparams);
}