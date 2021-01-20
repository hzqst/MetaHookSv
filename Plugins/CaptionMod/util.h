#ifndef UTIL_H
#define UTIL_H

#include <math.h>

#ifdef SSE
	#include <sselib.h>
#endif

//Vector
#ifndef M_PI
#define M_PI 3.141592653589
#endif

inline float DotProduct(float *x, float *y)
{
#ifndef SSE
	return (x[0]*y[0]+x[1]*y[1]+x[2]*y[2]);
#else
	float result;
	DotProductSSE(&result, x, y);
	return result;
#endif
}

#define VectorSubtract(a,b,c) {(c)[0]=(a)[0]-(b)[0];(c)[1]=(a)[1]-(b)[1];(c)[2]=(a)[2]-(b)[2];}
#define VectorAdd(a,b,c) {(c)[0]=(a)[0]+(b)[0];(c)[1]=(a)[1]+(b)[1];(c)[2]=(a)[2]+(b)[2];}
#define VectorCopy(a,b) {(b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2];}
#define VectorMultiply(a,b,c) {(c)[0]=(a)[0]*(b);(c)[1]=(a)[1]*(b);(c)[2]=(a)[2]*(b);}
#define VectorClear(a) {(a)[0]=0.0;(a)[1]=0.0;(a)[2]=0.0;}

inline void VectorMA(float *a, float scale, float *b, float *c)
{
#ifndef SSE
	c[0] = a[0] + scale * b[0];
	c[1] = a[1] + scale * b[1];
	c[2] = a[2] + scale * b[2];
#else
	VectorMASSE(a, scale, b, c);
#endif
}

#define CrossProduct(a,b,c) ((c)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1],(c)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2],(c)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0])

inline void VectorScale(float *a, float b)
{
#ifndef SSE
	a[0]=b*a[0];
	a[1]=b*a[1];
	a[2]=b*a[2];
#else
	return VectorScaleSSE(a, b);
#endif
}

inline float VectorLength(float *a)
{
#ifndef SSE
	return sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
#else
	return VectorLengthSSE(a);
#endif
}

inline void VectorNormalize(float *a)
{
#ifndef SSE
	float flLength = VectorLength(a);
	a[0]=a[0]/flLength;
	a[1]=a[1]/flLength;
	a[2]=a[2]/flLength;
#else
	return VectorNormalizeSSE(a);
#endif
}

inline float VectorAngle(vec3_t a, vec3_t b)
{
    float la = VectorLength(a);
	float lb = VectorLength(b);
	float lab = la*lb;
	if(lab==0.0)
		return 0.0;
	else
		return (double)(acos(DotProduct(a,b)/lab) * (180/M_PI));
}

wchar_t *UTF8ToUnicode( const char* str );
wchar_t *ANSIToUnicode( const char* str );
char *UnicodeToANSI( const wchar_t* str );
char *UnicodeToUTF8( const wchar_t* str );

char *UTIL_VarArgs(char *format, ...);

#define va UTIL_VarArgs

#define XX (g_x->value)
#define YY (g_y->value)
#define WW (g_w->value)
#define HH (g_h->value)

#define RANDOM_FLOAT gEngfuncs.pfnRandomFloat
#define RANDOM_LONG gEngfuncs.pfnRandomLong

#endif