#include "gl_local.h"

#ifdef _DEBUG
#pragma comment(lib, "opengl32.lib")
#endif

typedef HMODULE dllhandle_t;

typedef struct dllfunc_s
{
	const char *name;
	void **func;
}
dllfunc_t;

glState_t glState;
glConfig_t glConfig;

GLenum (APIENTRY *pglGetError)(void);
const GLcharARB *(APIENTRY *pglGetString)(GLenum name);

void (APIENTRY *pglAccum)(GLenum op, GLfloat value);
void (APIENTRY *pglAlphaFunc)(GLenum func, GLclampf ref);
void (APIENTRY *pglBegin)(GLenum mode);
void (APIENTRY *pglBindTexture)(GLenum target, GLuint texture);
void (APIENTRY *pglBitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void (APIENTRY *pglBlendFunc)(GLenum sfactor, GLenum dfactor);
void (APIENTRY *pglCallList)(GLuint list);
void (APIENTRY *pglCallLists)(GLsizei n, GLenum type, const GLvoid *lists);
void (APIENTRY *pglClear)(GLbitfield mask);
void (APIENTRY *pglClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void (APIENTRY *pglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void (APIENTRY *pglClearDepth)(GLclampd depth);
void (APIENTRY *pglClearIndex)(GLfloat c);
void (APIENTRY *pglClearStencil)(GLint s);
GLboolean (APIENTRY *pglIsEnabled)(GLenum cap);
GLboolean (APIENTRY *pglIsList)(GLuint list);
GLboolean (APIENTRY *pglIsTexture)(GLuint texture);
void (APIENTRY *pglClipPlane)(GLenum plane, const GLdouble *equation);
void (APIENTRY *pglColor3b)(GLbyte red, GLbyte green, GLbyte blue);
void (APIENTRY *pglColor3bv)(const GLbyte *v);
void (APIENTRY *pglColor3d)(GLdouble red, GLdouble green, GLdouble blue);
void (APIENTRY *pglColor3dv)(const GLdouble *v);
void (APIENTRY *pglColor3f)(GLfloat red, GLfloat green, GLfloat blue);
void (APIENTRY *pglColor3fv)(const GLfloat *v);
void (APIENTRY *pglColor3i)(GLint red, GLint green, GLint blue);
void (APIENTRY *pglColor3iv)(const GLint *v);
void (APIENTRY *pglColor3s)(GLshort red, GLshort green, GLshort blue);
void (APIENTRY *pglColor3sv)(const GLshort *v);
void (APIENTRY *pglColor3ub)(GLubyte red, GLubyte green, GLubyte blue);
void (APIENTRY *pglColor3ubv)(const GLubyte *v);
void (APIENTRY *pglColor3ui)(GLuint red, GLuint green, GLuint blue);
void (APIENTRY *pglColor3uiv)(const GLuint *v);
void (APIENTRY *pglColor3us)(GLushort red, GLushort green, GLushort blue);
void (APIENTRY *pglColor3usv)(const GLushort *v);
void (APIENTRY *pglColor4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
void (APIENTRY *pglColor4bv)(const GLbyte *v);
void (APIENTRY *pglColor4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void (APIENTRY *pglColor4dv)(const GLdouble *v);
void (APIENTRY *pglColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void (APIENTRY *pglColor4fv)(const GLfloat *v);
void (APIENTRY *pglColor4i)(GLint red, GLint green, GLint blue, GLint alpha);
void (APIENTRY *pglColor4iv)(const GLint *v);
void (APIENTRY *pglColor4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha);
void (APIENTRY *pglColor4sv)(const GLshort *v);
void (APIENTRY *pglColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void (APIENTRY *pglColor4ubv)(const GLubyte *v);
void (APIENTRY *pglColor4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha);
void (APIENTRY *pglColor4uiv)(const GLuint *v);
void (APIENTRY *pglColor4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha);
void (APIENTRY *pglColor4usv)(const GLushort *v);
void (APIENTRY *pglColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void (APIENTRY *pglColorMaterial)(GLenum face, GLenum mode);
void (APIENTRY *pglCopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void (APIENTRY *pglCopyTexImage1D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
void (APIENTRY *pglCopyTexImage2D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void (APIENTRY *pglCopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void (APIENTRY *pglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void (APIENTRY *pglCullFace)(GLenum mode);
void (APIENTRY *pglDeleteLists)(GLuint list, GLsizei range);
void (APIENTRY *pglDeleteTextures)(GLsizei n, const GLuint *textures);
void (APIENTRY *pglDepthFunc)(GLenum func);
void (APIENTRY *pglDepthMask)(GLboolean flag);
void (APIENTRY *pglDepthRange)(GLclampd zNear, GLclampd zFar);
void (APIENTRY *pglDisable)(GLenum cap);
void (APIENTRY *pglDisableClientState)(GLenum array);
void (APIENTRY *pglDrawArrays)(GLenum mode, GLint first, GLsizei count);
void (APIENTRY *pglDrawBuffer)(GLenum mode);
void (APIENTRY *pglDrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void (APIENTRY *pglEdgeFlag)(GLboolean flag);
void (APIENTRY *pglEdgeFlagPointer)(GLsizei stride, const GLvoid *pointer);
void (APIENTRY *pglEdgeFlagv)(const GLboolean *flag);
void (APIENTRY *pglEnable)(GLenum cap);
void (APIENTRY *pglEnableClientState)(GLenum array);
void (APIENTRY *pglEnd)(void);
void (APIENTRY *pglEndList)(void);
void (APIENTRY *pglEvalCoord1d)(GLdouble u);
void (APIENTRY *pglEvalCoord1dv)(const GLdouble *u);
void (APIENTRY *pglEvalCoord1f)(GLfloat u);
void (APIENTRY *pglEvalCoord1fv)(const GLfloat *u);
void (APIENTRY *pglEvalCoord2d)(GLdouble u, GLdouble v);
void (APIENTRY *pglEvalCoord2dv)(const GLdouble *u);
void (APIENTRY *pglEvalCoord2f)(GLfloat u, GLfloat v);
void (APIENTRY *pglEvalCoord2fv)(const GLfloat *u);
void (APIENTRY *pglEvalMesh1)(GLenum mode, GLint i1, GLint i2);
void (APIENTRY *pglEvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
void (APIENTRY *pglEvalPoint1)(GLint i);
void (APIENTRY *pglEvalPoint2)(GLint i, GLint j);
void (APIENTRY *pglFeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer);
void (APIENTRY *pglFinish)(void);
void (APIENTRY *pglFlush)(void);
void (APIENTRY *pglFogf)(GLenum pname, GLfloat param);
void (APIENTRY *pglFogfv)(GLenum pname, const GLfloat *params);
void (APIENTRY *pglFogi)(GLenum pname, GLint param);
void (APIENTRY *pglFogiv)(GLenum pname, const GLint *params);
void (APIENTRY *pglFrontFace)(GLenum mode);
void (APIENTRY *pglFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void (APIENTRY *pglGenTextures)(GLsizei n, GLuint *textures);
void (APIENTRY *pglGetBooleanv)(GLenum pname, GLboolean *params);
void (APIENTRY *pglGetClipPlane)(GLenum plane, GLdouble *equation);
void (APIENTRY *pglGetDoublev)(GLenum pname, GLdouble *params);
void (APIENTRY *pglGetFloatv)(GLenum pname, GLfloat *params);
void (APIENTRY *pglGetIntegerv)(GLenum pname, GLint *params);
void (APIENTRY *pglGetLightfv)(GLenum light, GLenum pname, GLfloat *params);
void (APIENTRY *pglGetLightiv)(GLenum light, GLenum pname, GLint *params);
void (APIENTRY *pglGetMapdv)(GLenum target, GLenum query, GLdouble *v);
void (APIENTRY *pglGetMapfv)(GLenum target, GLenum query, GLfloat *v);
void (APIENTRY *pglGetMapiv)(GLenum target, GLenum query, GLint *v);
void (APIENTRY *pglGetMaterialfv)(GLenum face, GLenum pname, GLfloat *params);
void (APIENTRY *pglGetMaterialiv)(GLenum face, GLenum pname, GLint *params);
void (APIENTRY *pglGetPixelMapfv)(GLenum map, GLfloat *values);
void (APIENTRY *pglGetPixelMapuiv)(GLenum map, GLuint *values);
void (APIENTRY *pglGetPixelMapusv)(GLenum map, GLushort *values);
void (APIENTRY *pglGetPointerv)(GLenum pname, GLvoid* *params);
void (APIENTRY *pglGetPolygonStipple)(GLubyte *mask);
void (APIENTRY *pglGetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params);
void (APIENTRY *pglGetTexEnviv)(GLenum target, GLenum pname, GLint *params);
void (APIENTRY *pglGetTexGendv)(GLenum coord, GLenum pname, GLdouble *params);
void (APIENTRY *pglGetTexGenfv)(GLenum coord, GLenum pname, GLfloat *params);
void (APIENTRY *pglGetTexGeniv)(GLenum coord, GLenum pname, GLint *params);
void (APIENTRY *pglGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
void (APIENTRY *pglGetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
void (APIENTRY *pglGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
void (APIENTRY *pglGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
void (APIENTRY *pglGetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
void (APIENTRY *pglHint)(GLenum target, GLenum mode);
void (APIENTRY *pglIndexMask)(GLuint mask);
void (APIENTRY *pglIndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
void (APIENTRY *pglIndexd)(GLdouble c);
void (APIENTRY *pglIndexdv)(const GLdouble *c);
void (APIENTRY *pglIndexf)(GLfloat c);
void (APIENTRY *pglIndexfv)(const GLfloat *c);
void (APIENTRY *pglIndexi)(GLint c);
void (APIENTRY *pglIndexiv)(const GLint *c);
void (APIENTRY *pglIndexs)(GLshort c);
void (APIENTRY *pglIndexsv)(const GLshort *c);
void (APIENTRY *pglIndexub)(GLubyte c);
void (APIENTRY *pglIndexubv)(const GLubyte *c);
void (APIENTRY *pglInitNames)(void);
void (APIENTRY *pglInterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer);
void (APIENTRY *pglLightModelf)(GLenum pname, GLfloat param);
void (APIENTRY *pglLightModelfv)(GLenum pname, const GLfloat *params);
void (APIENTRY *pglLightModeli)(GLenum pname, GLint param);
void (APIENTRY *pglLightModeliv)(GLenum pname, const GLint *params);
void (APIENTRY *pglLightf)(GLenum light, GLenum pname, GLfloat param);
void (APIENTRY *pglLightfv)(GLenum light, GLenum pname, const GLfloat *params);
void (APIENTRY *pglLighti)(GLenum light, GLenum pname, GLint param);
void (APIENTRY *pglLightiv)(GLenum light, GLenum pname, const GLint *params);
void (APIENTRY *pglLineStipple)(GLint factor, GLushort pattern);
void (APIENTRY *pglLineWidth)(GLfloat width);
void (APIENTRY *pglListBase)(GLuint base);
void (APIENTRY *pglLoadIdentity)(void);
void (APIENTRY *pglLoadMatrixd)(const GLdouble *m);
void (APIENTRY *pglLoadMatrixf)(const GLfloat *m);
void (APIENTRY *pglLoadName)(GLuint name);
void (APIENTRY *pglLogicOp)(GLenum opcode);
void (APIENTRY *pglMap1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
void (APIENTRY *pglMap1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
void (APIENTRY *pglMap2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
void (APIENTRY *pglMap2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
void (APIENTRY *pglMapGrid1d)(GLint un, GLdouble u1, GLdouble u2);
void (APIENTRY *pglMapGrid1f)(GLint un, GLfloat u1, GLfloat u2);
void (APIENTRY *pglMapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void (APIENTRY *pglMapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void (APIENTRY *pglMaterialf)(GLenum face, GLenum pname, GLfloat param);
void (APIENTRY *pglMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
void (APIENTRY *pglMateriali)(GLenum face, GLenum pname, GLint param);
void (APIENTRY *pglMaterialiv)(GLenum face, GLenum pname, const GLint *params);
void (APIENTRY *pglMatrixMode)(GLenum mode);
void (APIENTRY *pglMultMatrixd)(const GLdouble *m);
void (APIENTRY *pglMultMatrixf)(const GLfloat *m);
void (APIENTRY *pglNewList)(GLuint list, GLenum mode);
void (APIENTRY *pglNormal3b)(GLbyte nx, GLbyte ny, GLbyte nz);
void (APIENTRY *pglNormal3bv)(const GLbyte *v);
void (APIENTRY *pglNormal3d)(GLdouble nx, GLdouble ny, GLdouble nz);
void (APIENTRY *pglNormal3dv)(const GLdouble *v);
void (APIENTRY *pglNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
void (APIENTRY *pglNormal3fv)(const GLfloat *v);
void (APIENTRY *pglNormal3i)(GLint nx, GLint ny, GLint nz);
void (APIENTRY *pglNormal3iv)(const GLint *v);
void (APIENTRY *pglNormal3s)(GLshort nx, GLshort ny, GLshort nz);
void (APIENTRY *pglNormal3sv)(const GLshort *v);
void (APIENTRY *pglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void (APIENTRY *pglPassThrough)(GLfloat token);
void (APIENTRY *pglPixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat *values);
void (APIENTRY *pglPixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint *values);
void (APIENTRY *pglPixelMapusv)(GLenum map, GLsizei mapsize, const GLushort *values);
void (APIENTRY *pglPixelStoref)(GLenum pname, GLfloat param);
void (APIENTRY *pglPixelStorei)(GLenum pname, GLint param);
void (APIENTRY *pglPixelTransferf)(GLenum pname, GLfloat param);
void (APIENTRY *pglPixelTransferi)(GLenum pname, GLint param);
void (APIENTRY *pglPixelZoom)(GLfloat xfactor, GLfloat yfactor);
void (APIENTRY *pglPointSize)(GLfloat size);
void (APIENTRY *pglPolygonMode)(GLenum face, GLenum mode);
void (APIENTRY *pglPolygonOffset)(GLfloat factor, GLfloat units);
void (APIENTRY *pglPolygonStipple)(const GLubyte *mask);
void (APIENTRY *pglPopAttrib)(void);
void (APIENTRY *pglPopClientAttrib)(void);
void (APIENTRY *pglPopMatrix)(void);
void (APIENTRY *pglPopName)(void);
void (APIENTRY *pglPushAttrib)(GLbitfield mask);
void (APIENTRY *pglPushClientAttrib)(GLbitfield mask);
void (APIENTRY *pglPushMatrix)(void);
void (APIENTRY *pglPushName)(GLuint name);
void (APIENTRY *pglRasterPos2d)(GLdouble x, GLdouble y);
void (APIENTRY *pglRasterPos2dv)(const GLdouble *v);
void (APIENTRY *pglRasterPos2f)(GLfloat x, GLfloat y);
void (APIENTRY *pglRasterPos2fv)(const GLfloat *v);
void (APIENTRY *pglRasterPos2i)(GLint x, GLint y);
void (APIENTRY *pglRasterPos2iv)(const GLint *v);
void (APIENTRY *pglRasterPos2s)(GLshort x, GLshort y);
void (APIENTRY *pglRasterPos2sv)(const GLshort *v);
void (APIENTRY *pglRasterPos3d)(GLdouble x, GLdouble y, GLdouble z);
void (APIENTRY *pglRasterPos3dv)(const GLdouble *v);
void (APIENTRY *pglRasterPos3f)(GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *pglRasterPos3fv)(const GLfloat *v);
void (APIENTRY *pglRasterPos3i)(GLint x, GLint y, GLint z);
void (APIENTRY *pglRasterPos3iv)(const GLint *v);
void (APIENTRY *pglRasterPos3s)(GLshort x, GLshort y, GLshort z);
void (APIENTRY *pglRasterPos3sv)(const GLshort *v);
void (APIENTRY *pglRasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void (APIENTRY *pglRasterPos4dv)(const GLdouble *v);
void (APIENTRY *pglRasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void (APIENTRY *pglRasterPos4fv)(const GLfloat *v);
void (APIENTRY *pglRasterPos4i)(GLint x, GLint y, GLint z, GLint w);
void (APIENTRY *pglRasterPos4iv)(const GLint *v);
void (APIENTRY *pglRasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w);
void (APIENTRY *pglRasterPos4sv)(const GLshort *v);
void (APIENTRY *pglReadBuffer)(GLenum mode);
void (APIENTRY *pglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void (APIENTRY *pglRectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void (APIENTRY *pglRectdv)(const GLdouble *v1, const GLdouble *v2);
void (APIENTRY *pglRectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void (APIENTRY *pglRectfv)(const GLfloat *v1, const GLfloat *v2);
void (APIENTRY *pglRecti)(GLint x1, GLint y1, GLint x2, GLint y2);
void (APIENTRY *pglRectiv)(const GLint *v1, const GLint *v2);
void (APIENTRY *pglRects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void (APIENTRY *pglRectsv)(const GLshort *v1, const GLshort *v2);
void (APIENTRY *pglRotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void (APIENTRY *pglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *pglScaled)(GLdouble x, GLdouble y, GLdouble z);
void (APIENTRY *pglScalef)(GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *pglScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
void (APIENTRY *pglSelectBuffer)(GLsizei size, GLuint *buffer);
void (APIENTRY *pglShadeModel)(GLenum mode);
void (APIENTRY *pglStencilFunc)(GLenum func, GLint ref, GLuint mask);
void (APIENTRY *pglStencilMask)(GLuint mask);
void (APIENTRY *pglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
void (APIENTRY *pglTexCoord1d)(GLdouble s);
void (APIENTRY *pglTexCoord1dv)(const GLdouble *v);
void (APIENTRY *pglTexCoord1f)(GLfloat s);
void (APIENTRY *pglTexCoord1fv)(const GLfloat *v);
void (APIENTRY *pglTexCoord1i)(GLint s);
void (APIENTRY *pglTexCoord1iv)(const GLint *v);
void (APIENTRY *pglTexCoord1s)(GLshort s);
void (APIENTRY *pglTexCoord1sv)(const GLshort *v);
void (APIENTRY *pglTexCoord2d)(GLdouble s, GLdouble t);
void (APIENTRY *pglTexCoord2dv)(const GLdouble *v);
void (APIENTRY *pglTexCoord2f)(GLfloat s, GLfloat t);
void (APIENTRY *pglTexCoord2fv)(const GLfloat *v);
void (APIENTRY *pglTexCoord2i)(GLint s, GLint t);
void (APIENTRY *pglTexCoord2iv)(const GLint *v);
void (APIENTRY *pglTexCoord2s)(GLshort s, GLshort t);
void (APIENTRY *pglTexCoord2sv)(const GLshort *v);
void (APIENTRY *pglTexCoord3d)(GLdouble s, GLdouble t, GLdouble r);
void (APIENTRY *pglTexCoord3dv)(const GLdouble *v);
void (APIENTRY *pglTexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
void (APIENTRY *pglTexCoord3fv)(const GLfloat *v);
void (APIENTRY *pglTexCoord3i)(GLint s, GLint t, GLint r);
void (APIENTRY *pglTexCoord3iv)(const GLint *v);
void (APIENTRY *pglTexCoord3s)(GLshort s, GLshort t, GLshort r);
void (APIENTRY *pglTexCoord3sv)(const GLshort *v);
void (APIENTRY *pglTexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void (APIENTRY *pglTexCoord4dv)(const GLdouble *v);
void (APIENTRY *pglTexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void (APIENTRY *pglTexCoord4fv)(const GLfloat *v);
void (APIENTRY *pglTexCoord4i)(GLint s, GLint t, GLint r, GLint q);
void (APIENTRY *pglTexCoord4iv)(const GLint *v);
void (APIENTRY *pglTexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q);
void (APIENTRY *pglTexCoord4sv)(const GLshort *v);
void (APIENTRY *pglTexEnvf)(GLenum target, GLenum pname, GLfloat param);
void (APIENTRY *pglTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
void (APIENTRY *pglTexEnvi)(GLenum target, GLenum pname, GLint param);
void (APIENTRY *pglTexEnviv)(GLenum target, GLenum pname, const GLint *params);
void (APIENTRY *pglTexGend)(GLenum coord, GLenum pname, GLdouble param);
void (APIENTRY *pglTexGendv)(GLenum coord, GLenum pname, const GLdouble *params);
void (APIENTRY *pglTexGenf)(GLenum coord, GLenum pname, GLfloat param);
void (APIENTRY *pglTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);
void (APIENTRY *pglTexGeni)(GLenum coord, GLenum pname, GLint param);
void (APIENTRY *pglTexGeniv)(GLenum coord, GLenum pname, const GLint *params);
void (APIENTRY *pglTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void (APIENTRY *pglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void (APIENTRY *pglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
void (APIENTRY *pglTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
void (APIENTRY *pglTexParameteri)(GLenum target, GLenum pname, GLint param);
void (APIENTRY *pglTexParameteriv)(GLenum target, GLenum pname, const GLint *params);
void (APIENTRY *pglTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
void (APIENTRY *pglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void (APIENTRY *pglTranslated)(GLdouble x, GLdouble y, GLdouble z);
void (APIENTRY *pglTranslatef)(GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *pglVertex2d)(GLdouble x, GLdouble y);
void (APIENTRY *pglVertex2dv)(const GLdouble *v);
void (APIENTRY *pglVertex2f)(GLfloat x, GLfloat y);
void (APIENTRY *pglVertex2fv)(const GLfloat *v);
void (APIENTRY *pglVertex2i)(GLint x, GLint y);
void (APIENTRY *pglVertex2iv)(const GLint *v);
void (APIENTRY *pglVertex2s)(GLshort x, GLshort y);
void (APIENTRY *pglVertex2sv)(const GLshort *v);
void (APIENTRY *pglVertex3d)(GLdouble x, GLdouble y, GLdouble z);
void (APIENTRY *pglVertex3dv)(const GLdouble *v);
void (APIENTRY *pglVertex3f)(GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *pglVertex3fv)(const GLfloat *v);
void (APIENTRY *pglVertex3i)(GLint x, GLint y, GLint z);
void (APIENTRY *pglVertex3iv)(const GLint *v);
void (APIENTRY *pglVertex3s)(GLshort x, GLshort y, GLshort z);
void (APIENTRY *pglVertex3sv)(const GLshort *v);
void (APIENTRY *pglVertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void (APIENTRY *pglVertex4dv)(const GLdouble *v);
void (APIENTRY *pglVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void (APIENTRY *pglVertex4fv)(const GLfloat *v);
void (APIENTRY *pglVertex4i)(GLint x, GLint y, GLint z, GLint w);
void (APIENTRY *pglVertex4iv)(const GLint *v);
void (APIENTRY *pglVertex4s)(GLshort x, GLshort y, GLshort z, GLshort w);
void (APIENTRY *pglVertex4sv)(const GLshort *v);
void (APIENTRY *pglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
void (APIENTRY *pglPointParameterfEXT)(GLenum param, GLfloat value);
void (APIENTRY *pglPointParameterfvEXT)(GLenum param, const GLfloat *value);
void (APIENTRY *pglLockArraysEXT)(int , int);
void (APIENTRY *pglUnlockArraysEXT)(void);
void (APIENTRY *pglActiveTextureARB)(GLenum);
void (APIENTRY *pglClientActiveTextureARB)(GLenum);
void (APIENTRY *pglGetCompressedTexImage)(GLenum target, GLint lod, const void* data);
void (APIENTRY *pglDrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
void (APIENTRY *pglDrawRangeElementsEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
void (APIENTRY *pglDrawElements)(GLenum mode, GLsizei count, GLenum type, const void *indices);
void (APIENTRY *pglVertexPointer)(GLint size, GLenum type, GLsizei stride, const void *ptr);
void (APIENTRY *pglNormalPointer)(GLenum type, GLsizei stride, const void *ptr);
void (APIENTRY *pglColorPointer)(GLint size, GLenum type, GLsizei stride, const void *ptr);
void (APIENTRY *pglTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const void *ptr);
void (APIENTRY *pglArrayElement)(GLint i);
void (APIENTRY *pglMultiTexCoord1f)(GLenum, GLfloat);
void (APIENTRY *pglMultiTexCoord2f)(GLenum, GLfloat, GLfloat);
void (APIENTRY *pglMultiTexCoord3f)(GLenum, GLfloat, GLfloat, GLfloat);
void (APIENTRY *pglMultiTexCoord4f)(GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
void (APIENTRY *pglActiveTexture)(GLenum);
void (APIENTRY *pglClientActiveTexture)(GLenum);
void (APIENTRY *pglCompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
void (APIENTRY *pglCompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
void (APIENTRY *pglCompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data);
void (APIENTRY *pglCompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
void (APIENTRY *pglCompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
void (APIENTRY *pglCompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
void (APIENTRY *pglDeleteObjectARB)(GLhandleARB obj);
GLhandleARB (APIENTRY *pglGetHandleARB)(GLenum pname);
void (APIENTRY *pglDetachObjectARB)(GLhandleARB containerObj, GLhandleARB attachedObj);
GLhandleARB (APIENTRY *pglCreateShaderObjectARB)(GLenum shaderType);
void (APIENTRY *pglShaderSourceARB)(GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length);
void (APIENTRY *pglCompileShaderARB)(GLhandleARB shaderObj);
GLhandleARB (APIENTRY *pglCreateProgramObjectARB)(void);
void (APIENTRY *pglAttachObjectARB)(GLhandleARB containerObj, GLhandleARB obj);
void (APIENTRY *pglLinkProgramARB)(GLhandleARB programObj);
void (APIENTRY *pglUseProgramObjectARB)(GLhandleARB programObj);
void (APIENTRY *pglValidateProgramARB)(GLhandleARB programObj);
void (APIENTRY *pglBindProgramARB)(GLenum target, GLuint program);
void (APIENTRY *pglDeleteProgramsARB)(GLsizei n, const GLuint *programs);
void (APIENTRY *pglGenProgramsARB)(GLsizei n, GLuint *programs);
void (APIENTRY *pglProgramStringARB)(GLenum target, GLenum format, GLsizei len, const void *string);
void (APIENTRY *pglProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void (APIENTRY *pglProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void (APIENTRY *pglGetProgramivARB)(GLenum target, GLenum pname, GLint *params);
void (APIENTRY *pglUniform1fARB)(GLint location, GLfloat v0);
void (APIENTRY *pglUniform2fARB)(GLint location, GLfloat v0, GLfloat v1);
void (APIENTRY *pglUniform3fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void (APIENTRY *pglUniform4fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void (APIENTRY *pglUniform1iARB)(GLint location, GLint v0);
void (APIENTRY *pglUniform2iARB)(GLint location, GLint v0, GLint v1);
void (APIENTRY *pglUniform3iARB)(GLint location, GLint v0, GLint v1, GLint v2);
void (APIENTRY *pglUniform4iARB)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
void (APIENTRY *pglUniform1fvARB)(GLint location, GLsizei count, const GLfloat *value);
void (APIENTRY *pglUniform2fvARB)(GLint location, GLsizei count, const GLfloat *value);
void (APIENTRY *pglUniform3fvARB)(GLint location, GLsizei count, const GLfloat *value);
void (APIENTRY *pglUniform4fvARB)(GLint location, GLsizei count, const GLfloat *value);
void (APIENTRY *pglUniform1ivARB)(GLint location, GLsizei count, const GLint *value);
void (APIENTRY *pglUniform2ivARB)(GLint location, GLsizei count, const GLint *value);
void (APIENTRY *pglUniform3ivARB)(GLint location, GLsizei count, const GLint *value);
void (APIENTRY *pglUniform4ivARB)(GLint location, GLsizei count, const GLint *value);
void (APIENTRY *pglUniformMatrix2fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void (APIENTRY *pglUniformMatrix3fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void (APIENTRY *pglUniformMatrix4fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void (APIENTRY *pglGetObjectParameterfvARB)(GLhandleARB obj, GLenum pname, GLfloat *params);
void (APIENTRY *pglGetObjectParameterivARB)(GLhandleARB obj, GLenum pname, GLint *params);
void (APIENTRY *pglGetInfoLogARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
void (APIENTRY *pglGetAttachedObjectsARB)(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
GLint (APIENTRY *pglGetUniformLocationARB)(GLhandleARB programObj, const GLcharARB *name);
void (APIENTRY *pglGetActiveUniformARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
void (APIENTRY *pglGetUniformfvARB)(GLhandleARB programObj, GLint location, GLfloat *params);
void (APIENTRY *pglGetUniformivARB)(GLhandleARB programObj, GLint location, GLint *params);
void (APIENTRY *pglGetShaderSourceARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
void (APIENTRY *pglTexImage3D)(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
void (APIENTRY *pglTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
void (APIENTRY *pglCopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void (APIENTRY *pglBlendEquationEXT)(GLenum);
void (APIENTRY *pglStencilOpSeparate)(GLenum, GLenum, GLenum, GLenum);
void (APIENTRY *pglStencilFuncSeparate)(GLenum, GLenum, GLint, GLuint);
void (APIENTRY *pglActiveStencilFaceEXT)(GLenum);
void (APIENTRY *pglVertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
void (APIENTRY *pglEnableVertexAttribArrayARB)(GLuint index);
void (APIENTRY *pglDisableVertexAttribArrayARB)(GLuint index);
void (APIENTRY *pglBindAttribLocationARB)(GLhandleARB programObj, GLuint index, const GLcharARB *name);
void (APIENTRY *pglGetActiveAttribARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GLint (APIENTRY *pglGetAttribLocationARB)(GLhandleARB programObj, const GLcharARB *name);
void (APIENTRY *pglBindBufferARB)(GLenum target, GLuint buffer);
void (APIENTRY *pglDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
void (APIENTRY *pglGenBuffersARB)(GLsizei n, GLuint *buffers);
GLboolean (APIENTRY *pglIsBufferARB)(GLuint buffer);
void* (APIENTRY *pglMapBufferARB)(GLenum target, GLenum access);
GLboolean (APIENTRY *pglUnmapBufferARB)(GLenum target);
void (APIENTRY *pglBufferDataARB)(GLenum target, GLsizeiptrARB size, const void *data, GLenum usage);
void (APIENTRY *pglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void *data);
void (APIENTRY *pglGenQueriesARB)(GLsizei n, GLuint *ids);
void (APIENTRY *pglDeleteQueriesARB)(GLsizei n, const GLuint *ids);
GLboolean (APIENTRY *pglIsQueryARB)(GLuint id);
void (APIENTRY *pglBeginQueryARB)(GLenum target, GLuint id);
void (APIENTRY *pglEndQueryARB)(GLenum target);
void (APIENTRY *pglGetQueryivARB)(GLenum target, GLenum pname, GLint *params);
void (APIENTRY *pglGetQueryObjectivARB)(GLuint id, GLenum pname, GLint *params);
void (APIENTRY *pglGetQueryObjectuivARB)(GLuint id, GLenum pname, GLuint *params);
void (APIENTRY * pglSelectTextureSGIS)(GLenum);
void (APIENTRY * pglMTexCoord2fSGIS)(GLenum, GLfloat, GLfloat);
void (APIENTRY * pglSwapInterval)(int interval);
GLboolean (APIENTRY *pglIsRenderbuffer)(GLuint renderbuffer);
void (APIENTRY *pglBindRenderbuffer)(GLenum target, GLuint renderbuffer);
void (APIENTRY *pglDeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
void (APIENTRY *pglGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
void (APIENTRY *pglRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void (APIENTRY *pglRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
void (APIENTRY *pglGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint *params);
GLboolean (APIENTRY *pglIsFramebuffer)(GLuint framebuffer);
void (APIENTRY *pglBindFramebuffer)(GLenum target, GLuint framebuffer);
void (APIENTRY *pglDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
void (APIENTRY *pglGenFramebuffers)(GLsizei n, GLuint *framebuffers);
GLenum (APIENTRY *pglCheckFramebufferStatus)(GLenum target);
void (APIENTRY *pglFramebufferTexture1D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void (APIENTRY *pglFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void (APIENTRY *pglFramebufferTexture3D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer);
void (APIENTRY *pglFramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
void (APIENTRY *pglFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void (APIENTRY *pglGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
void (APIENTRY *pglBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
void (APIENTRY *pglGenerateMipmap)(GLenum target);
PROC (WINAPI *pwglGetProcAddress)(const char *);

static dllfunc_t opengl_110funcs[] =
{
	{ "glClearColor", (void **)&pglClearColor },
	{ "glClear", (void **)&pglClear },
	{ "glAlphaFunc", (void **)&pglAlphaFunc },
	{ "glBlendFunc", (void **)&pglBlendFunc },
	{ "glCullFace", (void **)&pglCullFace },
	{ "glDrawBuffer", (void **)&pglDrawBuffer },
	{ "glReadBuffer", (void **)&pglReadBuffer },
	{ "glEnable", (void **)&pglEnable },
	{ "glDisable", (void **)&pglDisable },
	{ "glEnableClientState", (void **)&pglEnableClientState },
	{ "glDisableClientState", (void **)&pglDisableClientState },
	{ "glGetBooleanv", (void **)&pglGetBooleanv },
	{ "glGetDoublev", (void **)&pglGetDoublev },
	{ "glGetFloatv", (void **)&pglGetFloatv },
	{ "glGetIntegerv", (void **)&pglGetIntegerv },
	{ "glGetError", (void **)&pglGetError },
	{ "glGetString", (void **)&pglGetString },
	{ "glFinish", (void **)&pglFinish },
	{ "glFlush", (void **)&pglFlush },
	{ "glClearDepth", (void **)&pglClearDepth },
	{ "glDepthFunc", (void **)&pglDepthFunc },
	{ "glDepthMask", (void **)&pglDepthMask },
	{ "glDepthRange", (void **)&pglDepthRange },
	{ "glFrontFace", (void **)&pglFrontFace },
	{ "glDrawElements", (void **)&pglDrawElements },
	{ "glColorMask", (void **)&pglColorMask },
	{ "glIndexPointer", (void **)&pglIndexPointer },
	{ "glVertexPointer", (void **)&pglVertexPointer },
	{ "glNormalPointer", (void **)&pglNormalPointer },
	{ "glColorPointer", (void **)&pglColorPointer },
	{ "glTexCoordPointer", (void **)&pglTexCoordPointer },
	{ "glArrayElement", (void **)&pglArrayElement },
	{ "glColor3f", (void **)&pglColor3f },
	{ "glColor3fv", (void **)&pglColor3fv },
	{ "glColor4f", (void **)&pglColor4f },
	{ "glColor4fv", (void **)&pglColor4fv },
	{ "glColor3ub", (void **)&pglColor3ub },
	{ "glColor4ub", (void **)&pglColor4ub },
	{ "glColor4ubv", (void **)&pglColor4ubv },
	{ "glTexCoord1f", (void **)&pglTexCoord1f },
	{ "glTexCoord2f", (void **)&pglTexCoord2f },
	{ "glTexCoord3f", (void **)&pglTexCoord3f },
	{ "glTexCoord4f", (void **)&pglTexCoord4f },
	{ "glTexCoord1fv", (void **)&pglTexCoord1fv },
	{ "glTexCoord2fv", (void **)&pglTexCoord2fv },
	{ "glTexCoord3fv", (void **)&pglTexCoord3fv },
	{ "glTexCoord4fv", (void **)&pglTexCoord4fv },
	{ "glTexGenf", (void **)&pglTexGenf },
	{ "glTexGenfv", (void **)&pglTexGenfv },
	{ "glTexGeni", (void **)&pglTexGeni },
	{ "glVertex2f", (void **)&pglVertex2f },
	{ "glVertex3f", (void **)&pglVertex3f },
	{ "glVertex3fv", (void **)&pglVertex3fv },
	{ "glNormal3f", (void **)&pglNormal3f },
	{ "glNormal3fv", (void **)&pglNormal3fv },
	{ "glBegin", (void **)&pglBegin },
	{ "glEnd", (void **)&pglEnd },
	{ "glLineWidth", (void**)&pglLineWidth },
	{ "glPointSize", (void**)&pglPointSize },
	{ "glMatrixMode", (void **)&pglMatrixMode },
	{ "glOrtho", (void **)&pglOrtho },
	{ "glRasterPos2f", (void **) &pglRasterPos2f },
	{ "glFrustum", (void **)&pglFrustum },
	{ "glViewport", (void **)&pglViewport },
	{ "glPushMatrix", (void **)&pglPushMatrix },
	{ "glPopMatrix", (void **)&pglPopMatrix },
	{ "glPushAttrib", (void **)&pglPushAttrib },
	{ "glPopAttrib", (void **)&pglPopAttrib },
	{ "glLoadIdentity", (void **)&pglLoadIdentity },
	{ "glLoadMatrixd", (void **)&pglLoadMatrixd },
	{ "glLoadMatrixf", (void **)&pglLoadMatrixf },
	{ "glMultMatrixd", (void **)&pglMultMatrixd },
	{ "glMultMatrixf", (void **)&pglMultMatrixf },
	{ "glRotated", (void **)&pglRotated },
	{ "glRotatef", (void **)&pglRotatef },
	{ "glScaled", (void **)&pglScaled },
	{ "glScalef", (void **)&pglScalef },
	{ "glTranslated", (void **)&pglTranslated },
	{ "glTranslatef", (void **)&pglTranslatef },
	{ "glReadPixels", (void **)&pglReadPixels },
	{ "glDrawPixels", (void **)&pglDrawPixels },
	{ "glStencilFunc", (void **)&pglStencilFunc },
	{ "glStencilMask", (void **)&pglStencilMask },
	{ "glStencilOp", (void **)&pglStencilOp },
	{ "glClearStencil", (void **)&pglClearStencil },
	{ "glIsEnabled", (void **)&pglIsEnabled },
	{ "glIsList", (void **)&pglIsList },
	{ "glIsTexture", (void **)&pglIsTexture },
	{ "glTexEnvf", (void **)&pglTexEnvf },
	{ "glTexEnvfv", (void **)&pglTexEnvfv },
	{ "glTexEnvi", (void **)&pglTexEnvi },
	{ "glTexParameterf", (void **)&pglTexParameterf },
	{ "glTexParameterfv", (void **)&pglTexParameterfv },
	{ "glTexParameteri", (void **)&pglTexParameteri },
	{ "glHint", (void **)&pglHint },
	{ "glPixelStoref", (void **)&pglPixelStoref },
	{ "glPixelStorei", (void **)&pglPixelStorei },
	{ "glGenTextures", (void **)&pglGenTextures },
	{ "glDeleteTextures", (void **)&pglDeleteTextures },
	{ "glBindTexture", (void **)&pglBindTexture },
	{ "glTexImage1D", (void **)&pglTexImage1D },
	{ "glTexImage2D", (void **)&pglTexImage2D },
	{ "glTexSubImage1D", (void **)&pglTexSubImage1D },
	{ "glTexSubImage2D", (void **)&pglTexSubImage2D },
	{ "glCopyTexImage1D", (void **)&pglCopyTexImage1D },
	{ "glCopyTexImage2D", (void **)&pglCopyTexImage2D },
	{ "glCopyTexSubImage1D", (void **)&pglCopyTexSubImage1D },
	{ "glCopyTexSubImage2D", (void **)&pglCopyTexSubImage2D },
	{ "glScissor", (void **)&pglScissor },
	{ "glGetTexEnviv", (void **)&pglGetTexEnviv },
	{ "glPolygonOffset", (void **)&pglPolygonOffset },
	{ "glPolygonMode", (void **)&pglPolygonMode },
	{ "glPolygonStipple", (void **)&pglPolygonStipple },
	{ "glClipPlane", (void **)&pglClipPlane },
	{ "glGetClipPlane", (void **)&pglGetClipPlane },
	{ "glShadeModel", (void **)&pglShadeModel },
	{ "glFogfv", (void **)&pglFogfv },
	{ "glFogf", (void **)&pglFogf },
	{ "glFogi", (void **)&pglFogi },
	{ NULL, NULL }
};

static dllfunc_t pointparametersfunc[] =
{
	{ "glPointParameterfEXT", (void **)&pglPointParameterfEXT },
	{ "glPointParameterfvEXT", (void **)&pglPointParameterfvEXT },
	{ NULL, NULL }
};

static dllfunc_t drawrangeelementsfuncs[] =
{
	{ "glDrawRangeElements", (void **)&pglDrawRangeElements },
	{ NULL, NULL }
};

static dllfunc_t drawrangeelementsextfuncs[] =
{
	{ "glDrawRangeElementsEXT", (void **)&pglDrawRangeElementsEXT },
	{ NULL, NULL }
};

static dllfunc_t sgis_multitexturefuncs[] =
{
	{ "glSelectTextureSGIS", (void **)&pglSelectTextureSGIS },
	{ "glMTexCoord2fSGIS", (void **)&pglMTexCoord2fSGIS },
	{ NULL, NULL }
};

static dllfunc_t multitexturefuncs[] =
{
	{ "glMultiTexCoord1fARB", (void **)&pglMultiTexCoord1f },
	{ "glMultiTexCoord2fARB", (void **)&pglMultiTexCoord2f },
	{ "glMultiTexCoord3fARB", (void **)&pglMultiTexCoord3f },
	{ "glMultiTexCoord4fARB", (void **)&pglMultiTexCoord4f },
	{ "glActiveTextureARB", (void **)&pglActiveTextureARB },
	{ "glClientActiveTextureARB", (void **)&pglClientActiveTexture },
	{ "glClientActiveTextureARB", (void **)&pglClientActiveTextureARB },
	{ NULL, NULL }
};

static dllfunc_t compiledvertexarrayfuncs[] =
{
	{ "glLockArraysEXT", (void **)&pglLockArraysEXT },
	{ "glUnlockArraysEXT", (void **)&pglUnlockArraysEXT },
	{ "glDrawArrays", (void **)&pglDrawArrays },
	{ NULL, NULL }
};

static dllfunc_t texture3dextfuncs[] =
{
	{ "glTexImage3DEXT", (void **)&pglTexImage3D },
	{ "glTexSubImage3DEXT", (void **)&pglTexSubImage3D },
	{ "glCopyTexSubImage3DEXT", (void **)&pglCopyTexSubImage3D },
	{ NULL, NULL }
};

static dllfunc_t atiseparatestencilfuncs[] =
{
	{ "glStencilOpSeparateATI", (void **)&pglStencilOpSeparate },
	{ "glStencilFuncSeparateATI", (void **)&pglStencilFuncSeparate },
	{ NULL, NULL }
};

static dllfunc_t gl2separatestencilfuncs[] =
{
	{ "glStencilOpSeparate", (void **)&pglStencilOpSeparate },
	{ "glStencilFuncSeparate", (void **)&pglStencilFuncSeparate },
	{ NULL, NULL }
};

static dllfunc_t stenciltwosidefuncs[] =
{
	{ "glActiveStencilFaceEXT", (void **)&pglActiveStencilFaceEXT },
	{ NULL, NULL }
};

static dllfunc_t blendequationfuncs[] =
{
	{ "glBlendEquationEXT", (void **)&pglBlendEquationEXT },
	{ NULL, NULL }
};

static dllfunc_t shaderobjectsfuncs[] =
{
	{ "glDeleteObjectARB", (void **)&pglDeleteObjectARB },
	{ "glGetHandleARB", (void **)&pglGetHandleARB },
	{ "glDetachObjectARB", (void **)&pglDetachObjectARB },
	{ "glCreateShaderObjectARB", (void **)&pglCreateShaderObjectARB },
	{ "glShaderSourceARB", (void **)&pglShaderSourceARB },
	{ "glCompileShaderARB", (void **)&pglCompileShaderARB },
	{ "glCreateProgramObjectARB", (void **)&pglCreateProgramObjectARB },
	{ "glAttachObjectARB", (void **)&pglAttachObjectARB },
	{ "glLinkProgramARB", (void **)&pglLinkProgramARB },
	{ "glUseProgramObjectARB", (void **)&pglUseProgramObjectARB },
	{ "glValidateProgramARB", (void **)&pglValidateProgramARB },
	{ "glUniform1fARB", (void **)&pglUniform1fARB },
	{ "glUniform2fARB", (void **)&pglUniform2fARB },
	{ "glUniform3fARB", (void **)&pglUniform3fARB },
	{ "glUniform4fARB", (void **)&pglUniform4fARB },
	{ "glUniform1iARB", (void **)&pglUniform1iARB },
	{ "glUniform2iARB", (void **)&pglUniform2iARB },
	{ "glUniform3iARB", (void **)&pglUniform3iARB },
	{ "glUniform4iARB", (void **)&pglUniform4iARB },
	{ "glUniform1fvARB", (void **)&pglUniform1fvARB },
	{ "glUniform2fvARB", (void **)&pglUniform2fvARB },
	{ "glUniform3fvARB", (void **)&pglUniform3fvARB },
	{ "glUniform4fvARB", (void **)&pglUniform4fvARB },
	{ "glUniform1ivARB", (void **)&pglUniform1ivARB },
	{ "glUniform2ivARB", (void **)&pglUniform2ivARB },
	{ "glUniform3ivARB", (void **)&pglUniform3ivARB },
	{ "glUniform4ivARB", (void **)&pglUniform4ivARB },
	{ "glUniformMatrix2fvARB", (void **)&pglUniformMatrix2fvARB },
	{ "glUniformMatrix3fvARB", (void **)&pglUniformMatrix3fvARB },
	{ "glUniformMatrix4fvARB", (void **)&pglUniformMatrix4fvARB },
	{ "glGetObjectParameterfvARB", (void **)&pglGetObjectParameterfvARB },
	{ "glGetObjectParameterivARB", (void **)&pglGetObjectParameterivARB },
	{ "glGetInfoLogARB", (void **)&pglGetInfoLogARB },
	{ "glGetAttachedObjectsARB", (void **)&pglGetAttachedObjectsARB },
	{ "glGetUniformLocationARB", (void **)&pglGetUniformLocationARB },
	{ "glGetActiveUniformARB", (void **) &pglGetActiveUniformARB },
	{ "glGetUniformfvARB", (void **)&pglGetUniformfvARB },
	{ "glGetUniformivARB", (void **)&pglGetUniformivARB },
	{ "glGetShaderSourceARB", (void **)&pglGetShaderSourceARB },
	{ "glVertexAttribPointerARB", (void **)&pglVertexAttribPointerARB },
	{ "glEnableVertexAttribArrayARB", (void **)&pglEnableVertexAttribArrayARB },
	{ "glDisableVertexAttribArrayARB", (void **)&pglDisableVertexAttribArrayARB },
	{ "glBindAttribLocationARB", (void **)&pglBindAttribLocationARB },
	{ "glGetActiveAttribARB", (void **)&pglGetActiveAttribARB },
	{ "glGetAttribLocationARB", (void **)&pglGetAttribLocationARB },
	{ NULL, NULL }
};

static dllfunc_t vertexshaderfuncs[] =
{
	{ "glVertexAttribPointerARB", (void **)&pglVertexAttribPointerARB },
	{ "glEnableVertexAttribArrayARB", (void **)&pglEnableVertexAttribArrayARB },
	{ "glDisableVertexAttribArrayARB", (void **)&pglDisableVertexAttribArrayARB },
	{ "glBindAttribLocationARB", (void **)&pglBindAttribLocationARB },
	{ "glGetActiveAttribARB", (void **)&pglGetActiveAttribARB },
	{ "glGetAttribLocationARB", (void **)&pglGetAttribLocationARB },
	{ NULL, NULL }
};

static dllfunc_t cgprogramfuncs[] =
{
	{ "glBindProgramARB", (void **)&pglBindProgramARB },
	{ "glDeleteProgramsARB", (void **)&pglDeleteProgramsARB },
	{ "glGenProgramsARB", (void **)&pglGenProgramsARB },
	{ "glProgramStringARB", (void **)&pglProgramStringARB },
	{ "glGetProgramivARB", (void **)&pglGetProgramivARB },
	{ "glProgramEnvParameter4fARB", (void **)&pglProgramEnvParameter4fARB },
	{ "glProgramLocalParameter4fARB", (void **)&pglProgramLocalParameter4fARB },
	{ NULL, NULL }
};

static dllfunc_t vbofuncs[] =
{
	{ "glBindBufferARB", (void **)&pglBindBufferARB },
	{ "glDeleteBuffersARB", (void **)&pglDeleteBuffersARB },
	{ "glGenBuffersARB", (void **)&pglGenBuffersARB },
	{ "glIsBufferARB", (void **)&pglIsBufferARB },
	{ "glMapBufferARB", (void **)&pglMapBufferARB },
	{ "glUnmapBufferARB", (void **)&pglUnmapBufferARB },
	{ "glBufferDataARB", (void **)&pglBufferDataARB },
	{ "glBufferSubDataARB", (void **)&pglBufferSubDataARB },
	{ NULL, NULL }
};

static dllfunc_t occlusionfunc[] =
{
	{ "glGenQueriesARB", (void **)&pglGenQueriesARB },
	{ "glDeleteQueriesARB", (void **)&pglDeleteQueriesARB },
	{ "glIsQueryARB", (void **)&pglIsQueryARB },
	{ "glBeginQueryARB", (void **)&pglBeginQueryARB },
	{ "glEndQueryARB", (void **)&pglEndQueryARB },
	{ "glGetQueryivARB", (void **)&pglGetQueryivARB },
	{ "glGetQueryObjectivARB", (void **)&pglGetQueryObjectivARB },
	{ "glGetQueryObjectuivARB", (void **)&pglGetQueryObjectuivARB },
	{ NULL, NULL }
};

static dllfunc_t texturecompressionfuncs[] =
{
	{ "glCompressedTexImage3DARB", (void **)&pglCompressedTexImage3DARB },
	{ "glCompressedTexImage2DARB", (void **)&pglCompressedTexImage2DARB },
	{ "glCompressedTexImage1DARB", (void **)&pglCompressedTexImage1DARB },
	{ "glCompressedTexSubImage3DARB", (void **)&pglCompressedTexSubImage3DARB },
	{ "glCompressedTexSubImage2DARB", (void **)&pglCompressedTexSubImage2DARB },
	{ "glCompressedTexSubImage1DARB", (void **)&pglCompressedTexSubImage1DARB },
	{ "glGetCompressedTexImageARB", (void **)&pglGetCompressedTexImage },
	{ NULL, NULL }
};

static dllfunc_t arbfbofuncs[] =
{
	{ "glIsRenderbuffer", (void **)&pglIsRenderbuffer },
	{ "glBindRenderbuffer", (void **)&pglBindRenderbuffer },
	{ "glDeleteRenderbuffers", (void **)&pglDeleteRenderbuffers },
	{ "glGenRenderbuffers", (void **)&pglGenRenderbuffers },
	{ "glRenderbufferStorage", (void **)&pglRenderbufferStorage },
	{ "glGetRenderbufferParameteriv", (void **)&pglGetRenderbufferParameteriv },
	{ "glIsFramebuffer", (void **)&pglIsFramebuffer },
	{ "glBindFramebuffer", (void **)&pglBindFramebuffer },
	{ "glDeleteFramebuffers", (void **)&pglDeleteFramebuffers },
	{ "glGenFramebuffers", (void **)&pglGenFramebuffers },
	{ "glCheckFramebufferStatus", (void **)&pglCheckFramebufferStatus },
	{ "glFramebufferTexture1D", (void **)&pglFramebufferTexture1D },
	{ "glFramebufferTexture2D", (void **)&pglFramebufferTexture2D },
	{ "glFramebufferTexture3D", (void **)&pglFramebufferTexture3D },
	{ "glFramebufferRenderbuffer", (void **)&pglFramebufferRenderbuffer },
	{ "glGetFramebufferAttachmentParameteriv", (void **)&pglGetFramebufferAttachmentParameteriv },
	{ "glGenerateMipmap", (void **)&pglGenerateMipmap },
	{ NULL, NULL }
};

static dllfunc_t wglproc_funcs[] =
{
	{ "wglGetProcAddress", (void **)&pwglGetProcAddress },
	{ NULL, NULL }
};

static dllhandle_t opengl_dll = NULL;

void *GL_GetProcAddress(const char *name)
{
	void *p = NULL;

	if (pwglGetProcAddress != NULL)
		p = (void *)pwglGetProcAddress(name);

	if (!p)
	{
		if (!opengl_dll)
			opengl_dll = GetModuleHandle("opengl32.dll");

		p = (void *)GetProcAddress(opengl_dll, name);
	}

	return p;
}

void GL_SetExtension(int r_ext, int enable)
{
	if (r_ext >= 0 && r_ext < R_EXTCOUNT)
		glConfig.extension[r_ext] = enable ? GL_TRUE : GL_FALSE;
	else
		gEngfuncs.Con_DPrintf("GL_SetExtension: invalid extension %d\n", r_ext);
}

bool GL_Support(int r_ext)
{
	if (r_ext >= 0 && r_ext < R_EXTCOUNT)
		return glConfig.extension[r_ext] ? true : false;

	gEngfuncs.Con_DPrintf("GL_Support: invalid extension %d\n", r_ext);
	return false;		
}

void GL_CheckExtension(const char *name, const dllfunc_t *funcs, int r_ext)
{
	const dllfunc_t	*func;

	gEngfuncs.Con_DPrintf("GL_CheckExtension: %s ", name);

	if ((name[2] == '_' || name[3] == '_') && !strstr(glConfig.extensions_string, name))
	{
		GL_SetExtension(r_ext, false);
		gEngfuncs.Con_DPrintf("- failed\n");
		return;
	}

	for (func = funcs; func && func->name; func++)
		*func->func = NULL;

	GL_SetExtension(r_ext, true);

	for (func = funcs; func && func->name != NULL; func++)
	{
		if (!(*func->func = (void *)GL_GetProcAddress(func->name)))
			GL_SetExtension(r_ext, false);
	}

	if (GL_Support(r_ext))
		gEngfuncs.Con_DPrintf("- enabled\n");
}

void GL_InitExtensions(void)
{
	GL_CheckExtension("OpenGL 1.1.0", opengl_110funcs, R_OPENGL_110);

	glConfig.vendor_string = pglGetString(GL_VENDOR);
	glConfig.renderer_string = pglGetString(GL_RENDERER);
	glConfig.version_string = pglGetString(GL_VERSION);
	glConfig.extensions_string = pglGetString(GL_EXTENSIONS);

	GL_CheckExtension("OpenGL ProcAddress", wglproc_funcs, R_WGL_PROCADDRESS);

	GL_CheckExtension("glDrawRangeElements", drawrangeelementsfuncs, R_DRAW_RANGEELEMENTS_EXT);

	if (!GL_Support(R_DRAW_RANGEELEMENTS_EXT))
		GL_CheckExtension("GL_EXT_draw_range_elements", drawrangeelementsextfuncs, R_DRAW_RANGEELEMENTS_EXT);

	glConfig.max_texture_units = 1;
	GL_CheckExtension("GL_ARB_multitexture", multitexturefuncs, R_ARB_MULTITEXTURE);

	if (GL_Support(R_ARB_MULTITEXTURE))
	{
		pglGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &glConfig.max_texture_units);
		GL_CheckExtension("GL_ARB_texture_env_combine", NULL, R_ENV_COMBINE_EXT);

		if (!GL_Support(R_ENV_COMBINE_EXT))
			GL_CheckExtension("GL_EXT_texture_env_combine", NULL, R_ENV_COMBINE_EXT);

		if (GL_Support(R_ENV_COMBINE_EXT))
			GL_CheckExtension("GL_ARB_texture_env_dot3", NULL, R_DOT3_ARB_EXT);
	}
	else
	{
		GL_CheckExtension("GL_SGIS_multitexture", sgis_multitexturefuncs, R_ARB_MULTITEXTURE);

		if (GL_Support(R_ARB_MULTITEXTURE))
			glConfig.max_texture_units = 2;
	}

	if (glConfig.max_texture_units == 1)
		GL_SetExtension(R_ARB_MULTITEXTURE, false);

	GL_CheckExtension("GL_EXT_texture3D", texture3dextfuncs, R_TEXTURE_3D_EXT);

	if (GL_Support(R_TEXTURE_3D_EXT))
	{
		pglGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &glConfig.max_3d_texture_size);

		if (glConfig.max_3d_texture_size < 32)
		{
			GL_SetExtension(R_TEXTURE_3D_EXT, false);
			gEngfuncs.Con_DPrintf("GL_EXT_texture3D reported bogus GL_MAX_3D_TEXTURE_SIZE, disabled\n");
		}
	}

	GL_CheckExtension("GL_SGIS_generate_mipmap", NULL, R_SGIS_MIPMAPS_EXT);
	GL_CheckExtension("GL_ARB_texture_cube_map", NULL, R_TEXTURECUBEMAP_EXT);

	if (GL_Support(R_TEXTURECUBEMAP_EXT))
		pglGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.max_cubemap_size);

	GL_CheckExtension("GL_EXT_point_parameters", pointparametersfunc, R_EXT_POINTPARAMETERS);
	GL_CheckExtension("GL_ARB_texture_non_power_of_two", NULL, R_ARB_TEXTURE_NPOT_EXT);
	GL_CheckExtension("GL_ARB_texture_compression", texturecompressionfuncs, R_TEXTURE_COMPRESSION_EXT);
	GL_CheckExtension("GL_EXT_compiled_vertex_array", compiledvertexarrayfuncs, R_CUSTOM_VERTEX_ARRAY_EXT);

	if (!GL_Support( R_CUSTOM_VERTEX_ARRAY_EXT))
		GL_CheckExtension("GL_SGI_compiled_vertex_array", compiledvertexarrayfuncs, R_CUSTOM_VERTEX_ARRAY_EXT);

	GL_CheckExtension("GL_EXT_texture_edge_clamp", NULL, R_CLAMPTOEDGE_EXT);

	if (!GL_Support(R_CLAMPTOEDGE_EXT))
		GL_CheckExtension("GL_SGIS_texture_edge_clamp", NULL, R_CLAMPTOEDGE_EXT);

	glConfig.max_texture_anisotropy = 0.0f;
	GL_CheckExtension("GL_EXT_texture_filter_anisotropic", NULL, R_ANISOTROPY_EXT);

	if (GL_Support(R_ANISOTROPY_EXT))
		pglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.max_texture_anisotropy);

	GL_CheckExtension("GL_EXT_texture_lod_bias", NULL, R_TEXTURE_LODBIAS);

	if (GL_Support(R_TEXTURE_LODBIAS))
		pglGetFloatv(GL_MAX_TEXTURE_LOD_BIAS_EXT, &glConfig.max_texture_lodbias);

	GL_CheckExtension("GL_ARB_texture_border_clamp", NULL, R_CLAMP_TEXBORDER_EXT);
	GL_CheckExtension("GL_EXT_blend_minmax", blendequationfuncs, R_BLEND_MINMAX_EXT);
	GL_CheckExtension("GL_EXT_blend_subtract", blendequationfuncs, R_BLEND_SUBTRACT_EXT);

	GL_CheckExtension("glStencilOpSeparate", gl2separatestencilfuncs, R_SEPARATESTENCIL_EXT);

	if (!GL_Support(R_SEPARATESTENCIL_EXT))
		GL_CheckExtension("GL_ATI_separate_stencil", atiseparatestencilfuncs, R_SEPARATESTENCIL_EXT);

	GL_CheckExtension("GL_EXT_stencil_two_side", stenciltwosidefuncs, R_STENCILTWOSIDE_EXT);
	GL_CheckExtension("GL_ARB_vertex_buffer_object", vbofuncs, R_ARB_VERTEX_BUFFER_OBJECT_EXT);

	if (pglDrawRangeElementsEXT == NULL)
		pglDrawRangeElementsEXT = pglDrawRangeElements;

	GL_CheckExtension("GL_ARB_vertex_program", cgprogramfuncs, R_VERTEX_PROGRAM_EXT);
	GL_CheckExtension("GL_ARB_fragment_program", cgprogramfuncs, R_FRAGMENT_PROGRAM_EXT);
	GL_CheckExtension("GL_ARB_texture_env_add", NULL, R_TEXTURE_ENV_ADD_EXT);

	GL_CheckExtension("GL_ARB_shader_objects", shaderobjectsfuncs, R_SHADER_OBJECTS_EXT);
	GL_CheckExtension("GL_ARB_shading_language_100", NULL, R_SHADER_GLSL100_EXT);
	GL_CheckExtension("GL_ARB_vertex_shader", vertexshaderfuncs, R_VERTEX_SHADER_EXT);
	GL_CheckExtension("GL_ARB_fragment_shader", NULL, R_FRAGMENT_SHADER_EXT);

	GL_CheckExtension("GL_ARB_depth_texture", NULL, R_DEPTH_TEXTURE);
	GL_CheckExtension("GL_ARB_shadow", NULL, R_SHADOW_EXT);

	GL_CheckExtension("GL_ARB_occlusion_query", occlusionfunc, R_OCCLUSION_QUERIES_EXT);

	if (strstr(glConfig.extensions_string, "GL_NV_texture_rectangle"))
	{
		glConfig.texRectangle = GL_TEXTURE_RECTANGLE_NV;
		pglGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &glConfig.max_2d_rectangle_size);
	}
	else if (strstr(glConfig.extensions_string, "GL_EXT_texture_rectangle"))
	{
		glConfig.texRectangle = GL_TEXTURE_RECTANGLE_EXT;
		pglGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &glConfig.max_2d_rectangle_size);
	}
	else
		glConfig.texRectangle = glConfig.max_2d_rectangle_size = 0;

	glConfig.max_2d_texture_size = 0;
	pglGetIntegerv(GL_MAX_TEXTURE_SIZE, &glConfig.max_2d_texture_size);

	if (glConfig.max_2d_texture_size <= 0)
		glConfig.max_2d_texture_size = 256;

	if (!GL_Support(R_SGIS_MIPMAPS_EXT))
		GL_SetExtension(R_ARB_TEXTURE_NPOT_EXT, false);

	GL_CheckExtension("GL_ARB_framebuffer_object", arbfbofuncs, R_FRAMEBUFFER_OBJECT);

	GL_CheckExtension("PARANOIA_HACKS_V1", NULL, R_PARANOIA_EXT);
}

void GL_SetDefaultState(void)
{
}