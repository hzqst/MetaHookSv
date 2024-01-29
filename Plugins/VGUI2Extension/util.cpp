#include <metahook.h>
#include "exportfuncs.h"
#include "engfuncs.h"
#include "util.h"
#include "studio_util.h"

char *UTIL_VarArgs(char *format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

BOOL UTIL_IsHullDefaultEx(vec3_t vecOrigin, float size, float height)
{
	vec3_t traceEnds[8];

	traceEnds[0][0] = vecOrigin[0] - size;
	traceEnds[0][1] = vecOrigin[1] - size;
	traceEnds[0][2] = vecOrigin[2] - height;

	traceEnds[1][0] = vecOrigin[0] - size;
	traceEnds[1][1] = vecOrigin[1] - size;
	traceEnds[1][2] = vecOrigin[2] + height;

	traceEnds[2][0] = vecOrigin[0] + size;
	traceEnds[2][1] = vecOrigin[1] - size;
	traceEnds[2][2] = vecOrigin[2] + height;

	traceEnds[3][0] = vecOrigin[0] + size;
	traceEnds[3][1] = vecOrigin[1] - size;
	traceEnds[3][2] = vecOrigin[2] - height;

	traceEnds[4][0] = vecOrigin[0] - size;
	traceEnds[4][1] = vecOrigin[1] + size;
	traceEnds[4][2] = vecOrigin[2] - height;

	traceEnds[5][0] = vecOrigin[0] - size;
	traceEnds[5][1] = vecOrigin[1] + size;
	traceEnds[5][2] = vecOrigin[2] + height;

	traceEnds[6][0] = vecOrigin[0] + size;
	traceEnds[6][1] = vecOrigin[1] + size;
	traceEnds[6][2] = vecOrigin[2] + height;

	traceEnds[7][0] = vecOrigin[0] + size;
	traceEnds[7][1] = vecOrigin[1] + size;
	traceEnds[7][2] = vecOrigin[2] - height;

	for (int i = 0; i < 8; i++)
	{
		int contents = gEngfuncs.PM_PointContents(traceEnds[i], NULL);
		if (contents != CONTENTS_EMPTY && contents != CONTENT_WATER)
			return FALSE;

		/*tr = (*cl_ppmove)->PM_PlayerTraceEx(vecOrigin, traceEnds[i], PM_NORMAL, T_BluePrint_IgnoreEnt);
		if (tr.fraction != 1)
			return FALSE;
		if (traceEnds[i][0] != tr.endpos[0])
			return FALSE;
		if (traceEnds[i][1] != tr.endpos[1])
			return FALSE;
		if (traceEnds[i][2] != tr.endpos[2])
			return FALSE;*/
	}
	return TRUE;
}

BOOL UTIL_IsHullInZone(physent_t *pe, vec3_t vecMins, vec3_t vecMaxs)
{
	if(vecMaxs[0] < pe->mins[0]
	|| vecMaxs[1] < pe->mins[1]
	|| vecMaxs[2] < pe->mins[2]
	|| vecMins[0] > pe->maxs[0]
	|| vecMins[1] > pe->maxs[1]
	|| vecMins[2] > pe->maxs[2])
		return FALSE;
	hull_t *hull = &(pe->model->hulls[0]);
	vec3_t vecStart;

	for(int i = 0; i < 8; ++i)
	{
		vecStart[0] = (i & 1) ? vecMins[0] : vecMaxs[0];
		vecStart[1] = (i & 2) ? vecMins[1] : vecMaxs[1];
		vecStart[2] = (i & 4) ? vecMins[2] : vecMaxs[2];
		if((*cl_ppmove)->PM_HullPointContents(hull, hull->firstclipnode, vecStart) == CONTENT_SOLID)
			return TRUE;
	}
	return FALSE;
}

void ProjectPointOnPlane(vec3_t dst, vec3_t p, vec3_t normal)
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct(normal, normal);

	d = DotProduct(normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

void PerpendicularVector(vec3_t dst, vec3_t src)
{
	int pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	for (pos = 0, i = 0; i < 3; i++)
	{
		if (fabs(src[i]) < minelem)
		{
			pos = i;
			minelem = fabs(src[i]);
		}
	}

	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	ProjectPointOnPlane(dst, tempvec, src);

	VectorNormalize(dst);
}

void ClipVelocity(vec_t *in, vec_t *normal, vec_t *out, float overbounce)
{
	float backoff;
	float change;
	float angle;
	int i;

	angle = normal[2];

	backoff = DotProduct(in, normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;

		if (out[i] > -0.1 && out[i] < 0.1)
			out[i] = 0;
	}
}