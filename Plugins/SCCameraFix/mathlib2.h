#pragma once

#include <math.h>

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

#define SIDE_FRONT 0
#define SIDE_ON 2
#define SIDE_BACK 1
#define SIDE_CROSS -2

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define Q_PI 3.14159265358979323846

extern vec3_t vec3_origin;

#define EQUAL_EPSILON 0.001
#define COLINEAR_EPSILON 0.001

struct mplane_s;

extern int nanmask;

#define IS_NAN(x) (((*(int *)&x)&nanmask) == nanmask)

#define METERS_PER_INCH 0.0254f
#define METER2INCH(x) (float)(x * (1.0f / METERS_PER_INCH))
#define INCH2METER(x) (float)(x * (METERS_PER_INCH / 1.0f))

#define VectorSubtract(a, b, c) { (c)[0] = (a)[0] - (b)[0]; (c)[1] = (a)[1] - (b)[1]; (c)[2] = (a)[2] - (b)[2]; }
#define VectorAdd(a, b, c) { (c)[0] = (a)[0] + (b)[0]; (c)[1] = (a)[1] + (b)[1]; (c)[2] = (a)[2] + (b)[2]; }
#define VectorCopy(a, b) { (b)[0] = (a)[0]; (b)[1] = (a)[1]; (b)[2] = (a)[2]; }
#define VectorClear(a) { (a)[0] = 0.0; (a)[1] = 0.0; (a)[2] = 0.0; }
#define DotProduct(x, y) ((x)[0] * (y)[0] + (x)[1] * (y)[1] + (x)[2] * (y)[2])
#define DotProduct4(x, y) ((x)[0] * (y)[0] + (x)[1] * (y)[1] + (x)[2] * (y)[2]+ (x)[3] * (y)[3])
void MatrixCopy(float in[4][3], float out[4][3]);

void VectorMA(const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc);

vec_t _DotProduct(vec3_t v1, vec3_t v2);
void _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorCopy(vec3_t in, vec3_t out);

int VectorCompare(const vec3_t v1, const vec3_t v2);
double VectorDistance(const vec3_t v1, const vec3_t v2);
double VectorLength(vec3_t v);
void CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross);
float VectorNormalize(vec3_t v);
void VectorInverse(vec3_t v);
void VectorScale(const vec3_t in, vec_t scale, vec3_t out);
int Q_log2(int val);
void ClearBounds(vec3_t mins, vec3_t maxs);

void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);
void Matrix4x4_CreateFromEntity(float out[4][4], const vec3_t angles, const vec3_t origin, float scale);

void FloorDivMod(double numer, double denom, int *quotient, int *rem);
int GreatestCommonDivisor(int i1, int i2);
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);

void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void AngleVectorsTranspose(const vec3_t angles, vec3_t *forward, vec3_t *right, vec3_t *up);

#define AngleIVectors AngleVectorsTranspose

void AngleMatrix(const vec3_t angles, float (*matrix)[4]);
void AngleIMatrix(const vec3_t angles, float (*matrix)[4]);
void VectorTransform(const vec3_t in1, float in2[3][4], vec3_t out);

void NormalizeAngles(vec3_t angles);
void InterpolateAngles(vec3_t start, vec3_t end, vec3_t output, float frac);
float AngleBetweenVectors(const vec3_t v1, const vec3_t v2);

void VectorMatrix(vec3_t forward, vec3_t right, vec3_t up);
void VectorAngles(const vec3_t forward, vec3_t angles);

int InvertMatrix(const float * m, float *out);

void Matrix4x4_ConcatTransforms(float out[4][4], float in1[4][4], float in2[4][4]);
void Matrix4x4_Transpose(float out[4][4], float in1[4][4]);
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct mplane_s *plane);

void VectorRotate(const vec3_t in1, const float in2[3][4], vec3_t out);
void VectorIRotate(const vec3_t in1, const float in2[3][4], vec3_t out);

inline void SinCos(float radians, float* sine, float* cosine)
{
	*sine = sinf(radians);
	*cosine = cosf(radians);
}

inline float anglemod(float a)
{
	a = (360.f / 65536) * ((int)(a * (65536.f / 360.0f)) & 65535);
	return a;
}

#define BOX_ON_PLANE_SIDE(emins, emaxs, p) \
	(((p)->type < 3) ? \
	( \
		((p)->dist <= (emins)[(p)->type]) ? \
			1 \
		: \
		( \
			((p)->dist >= (emaxs)[(p)->type]) ? \
				2 \
			: \
				3 \
		) \
	) \
	: \
		BoxOnPlaneSide((emins), (emaxs), (p)))