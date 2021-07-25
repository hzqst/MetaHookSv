#include <metahook.h>
#include <gl/gl.h>
#include <gl/glext.h>
#include "gl_local.h"

#ifdef _DEBUG
#pragma comment(lib, "opengl32.lib")
#endif

extern "C"
{
	void (APIENTRY *qglAccum)(GLenum op, GLfloat value) = NULL;
	void (APIENTRY *qglAlphaFunc)(GLenum func, GLclampf ref) = NULL;
	GLboolean (APIENTRY *qglAreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences) = NULL;
	void (APIENTRY *qglArrayElement)(GLint i) = NULL;
	void (APIENTRY *qglBegin)(GLenum mode) = NULL;
	void (APIENTRY *qglBindTexture)(GLenum target, GLuint texture) = NULL;
	void (APIENTRY *qglBitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) = NULL;
	void (APIENTRY *qglBlendFunc)(GLenum sfactor, GLenum dfactor) = NULL;
	void (APIENTRY *qglCallList)(GLuint list) = NULL;
	void (APIENTRY *qglCallLists)(GLsizei n, GLenum type, const GLvoid *lists) = NULL;
	void (APIENTRY *qglClear)(GLbitfield mask) = NULL;
	void (APIENTRY *qglClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = NULL;
	void (APIENTRY *qglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) = NULL;
	void (APIENTRY *qglClearDepth)(GLclampd depth) = NULL;
	void (APIENTRY *qglClearIndex)(GLfloat c) = NULL;
	void (APIENTRY *qglClearStencil)(GLint s) = NULL;
	void (APIENTRY *qglClipPlane)(GLenum plane, const GLdouble *equation) = NULL;
	void (APIENTRY *qglColor3b)(GLbyte red, GLbyte green, GLbyte blue) = NULL;
	void (APIENTRY *qglColor3bv)(const GLbyte *v) = NULL;
	void (APIENTRY *qglColor3d)(GLdouble red, GLdouble green, GLdouble blue) = NULL;
	void (APIENTRY *qglColor3dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglColor3f)(GLfloat red, GLfloat green, GLfloat blue) = NULL;
	void (APIENTRY *qglColor3fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglColor3i)(GLint red, GLint green, GLint blue) = NULL;
	void (APIENTRY *qglColor3iv)(const GLint *v) = NULL;
	void (APIENTRY *qglColor3s)(GLshort red, GLshort green, GLshort blue) = NULL;
	void (APIENTRY *qglColor3sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglColor3ub)(GLubyte red, GLubyte green, GLubyte blue) = NULL;
	void (APIENTRY *qglColor3ubv)(const GLubyte *v) = NULL;
	void (APIENTRY *qglColor3ui)(GLuint red, GLuint green, GLuint blue) = NULL;
	void (APIENTRY *qglColor3uiv)(const GLuint *v) = NULL;
	void (APIENTRY *qglColor3us)(GLushort red, GLushort green, GLushort blue) = NULL;
	void (APIENTRY *qglColor3usv)(const GLushort *v) = NULL;
	void (APIENTRY *qglColor4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) = NULL;
	void (APIENTRY *qglColor4bv)(const GLbyte *v) = NULL;
	void (APIENTRY *qglColor4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) = NULL;
	void (APIENTRY *qglColor4dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = NULL;
	void (APIENTRY *qglColor4fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglColor4i)(GLint red, GLint green, GLint blue, GLint alpha) = NULL;
	void (APIENTRY *qglColor4iv)(const GLint *v) = NULL;
	void (APIENTRY *qglColor4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha) = NULL;
	void (APIENTRY *qglColor4sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) = NULL;
	void (APIENTRY *qglColor4ubv)(const GLubyte *v) = NULL;
	void (APIENTRY *qglColor4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha) = NULL;
	void (APIENTRY *qglColor4uiv)(const GLuint *v) = NULL;
	void (APIENTRY *qglColor4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha) = NULL;
	void (APIENTRY *qglColor4usv)(const GLushort *v) = NULL;
	void (APIENTRY *qglColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) = NULL;
	void (APIENTRY *qglColorMaterial)(GLenum face, GLenum mode) = NULL;
	void (APIENTRY *qglColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) = NULL;
	void (APIENTRY *qglCopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) = NULL;
	void (APIENTRY *qglCopyTexImage1D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border) = NULL;
	void (APIENTRY *qglCopyTexImage2D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) = NULL;
	void (APIENTRY *qglCopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) = NULL;
	void (APIENTRY *qglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
	void (APIENTRY *qglCullFace)(GLenum mode) = NULL;
	void (APIENTRY *qglDeleteLists)(GLuint list, GLsizei range) = NULL;
	void (APIENTRY *qglDeleteTextures)(GLsizei n, const GLuint *textures) = NULL;
	void (APIENTRY *qglDepthFunc)(GLenum func) = NULL;
	void (APIENTRY *qglDepthMask)(GLboolean flag) = NULL;
	void (APIENTRY *qglDepthRange)(GLclampd zNear, GLclampd zFar) = NULL;
	void (APIENTRY *qglDisable)(GLenum cap) = NULL;
	void (APIENTRY *qglDisableClientState)(GLenum array) = NULL;
	void (APIENTRY *qglDrawArrays)(GLenum mode, GLint first, GLsizei count) = NULL;
	void (APIENTRY *qglDrawBuffer)(GLenum mode) = NULL;
	void (APIENTRY *qglDrawBuffers)(GLsizei n, const GLenum *bufs) = NULL;
	void (APIENTRY *qglDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) = NULL;
	void (APIENTRY *qglDrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
	void (APIENTRY *qglEdgeFlag)(GLboolean flag) = NULL;
	void (APIENTRY *qglEdgeFlagPointer)(GLsizei stride, const GLvoid *pointer) = NULL;
	void (APIENTRY *qglEdgeFlagv)(const GLboolean *flag) = NULL;
	void (APIENTRY *qglEnable)(GLenum cap) = NULL;
	void (APIENTRY *qglEnableClientState)(GLenum array) = NULL;
	void (APIENTRY *qglEnd)(void) = NULL;
	void (APIENTRY *qglEndList)(void) = NULL;
	void (APIENTRY *qglEvalCoord1d)(GLdouble u) = NULL;
	void (APIENTRY *qglEvalCoord1dv)(const GLdouble *u) = NULL;
	void (APIENTRY *qglEvalCoord1f)(GLfloat u) = NULL;
	void (APIENTRY *qglEvalCoord1fv)(const GLfloat *u) = NULL;
	void (APIENTRY *qglEvalCoord2d)(GLdouble u, GLdouble v) = NULL;
	void (APIENTRY *qglEvalCoord2dv)(const GLdouble *u) = NULL;
	void (APIENTRY *qglEvalCoord2f)(GLfloat u, GLfloat v) = NULL;
	void (APIENTRY *qglEvalCoord2fv)(const GLfloat *u) = NULL;
	void (APIENTRY *qglEvalMesh1)(GLenum mode, GLint i1, GLint i2) = NULL;
	void (APIENTRY *qglEvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) = NULL;
	void (APIENTRY *qglEvalPoint1)(GLint i) = NULL;
	void (APIENTRY *qglEvalPoint2)(GLint i, GLint j) = NULL;
	void (APIENTRY *qglFeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer) = NULL;
	void (APIENTRY *qglFinish)(void) = NULL;
	void (APIENTRY *qglFlush)(void) = NULL;
	void (APIENTRY *qglFogf)(GLenum pname, GLfloat param) = NULL;
	void (APIENTRY *qglFogfv)(GLenum pname, const GLfloat *params) = NULL;
	void (APIENTRY *qglFogi)(GLenum pname, GLint param) = NULL;
	void (APIENTRY *qglFogiv)(GLenum pname, const GLint *params) = NULL;
	void (APIENTRY *qglFrontFace)(GLenum mode) = NULL;
	void (APIENTRY *qglFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) = NULL;
	GLuint (APIENTRY *qglGenLists)(GLsizei range) = NULL;
	void (APIENTRY *qglGenTextures)(GLsizei n, GLuint *textures) = NULL;
	void (APIENTRY *qglGetBooleanv)(GLenum pname, GLboolean *params) = NULL;
	void (APIENTRY *qglGetClipPlane)(GLenum plane, GLdouble *equation) = NULL;
	void (APIENTRY *qglGetDoublev)(GLenum pname, GLdouble *params) = NULL;
	GLenum (APIENTRY *qglGetError)(void) = NULL;
	void (APIENTRY *qglGetFloatv)(GLenum pname, GLfloat *params) = NULL;
	void (APIENTRY *qglGetIntegerv)(GLenum pname, GLint *params) = NULL;
	void (APIENTRY *qglGetLightfv)(GLenum light, GLenum pname, GLfloat *params) = NULL;
	void (APIENTRY *qglGetLightiv)(GLenum light, GLenum pname, GLint *params) = NULL;
	void (APIENTRY *qglGetMapdv)(GLenum target, GLenum query, GLdouble *v) = NULL;
	void (APIENTRY *qglGetMapfv)(GLenum target, GLenum query, GLfloat *v) = NULL;
	void (APIENTRY *qglGetMapiv)(GLenum target, GLenum query, GLint *v) = NULL;
	void (APIENTRY *qglGetMaterialfv)(GLenum face, GLenum pname, GLfloat *params) = NULL;
	void (APIENTRY *qglGetMaterialiv)(GLenum face, GLenum pname, GLint *params) = NULL;
	void (APIENTRY *qglGetPixelMapfv)(GLenum map, GLfloat *values) = NULL;
	void (APIENTRY *qglGetPixelMapuiv)(GLenum map, GLuint *values) = NULL;
	void (APIENTRY *qglGetPixelMapusv)(GLenum map, GLushort *values) = NULL;
	void (APIENTRY *qglGetPointerv)(GLenum pname, GLvoid* *params) = NULL;
	void (APIENTRY *qglGetPolygonStipple)(GLubyte *mask) = NULL;
	const GLubyte * (APIENTRY *qglGetString)(GLenum name) = NULL;
	void (APIENTRY *qglGetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params) = NULL;
	void (APIENTRY *qglGetTexEnviv)(GLenum target, GLenum pname, GLint *params) = NULL;
	void (APIENTRY *qglGetTexGendv)(GLenum coord, GLenum pname, GLdouble *params) = NULL;
	void (APIENTRY *qglGetTexGenfv)(GLenum coord, GLenum pname, GLfloat *params) = NULL;
	void (APIENTRY *qglGetTexGeniv)(GLenum coord, GLenum pname, GLint *params) = NULL;
	void (APIENTRY *qglGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels) = NULL;
	void (APIENTRY *qglGetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params) = NULL;
	void (APIENTRY *qglGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params) = NULL;
	void (APIENTRY *qglGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params) = NULL;
	void (APIENTRY *qglGetTexParameteriv)(GLenum target, GLenum pname, GLint *params) = NULL;
	void (APIENTRY *qglHint)(GLenum target, GLenum mode) = NULL;
	void (APIENTRY *qglIndexMask)(GLuint mask) = NULL;
	void (APIENTRY *qglIndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer) = NULL;
	void (APIENTRY *qglIndexd)(GLdouble c) = NULL;
	void (APIENTRY *qglIndexdv)(const GLdouble *c) = NULL;
	void (APIENTRY *qglIndexf)(GLfloat c) = NULL;
	void (APIENTRY *qglIndexfv)(const GLfloat *c) = NULL;
	void (APIENTRY *qglIndexi)(GLint c) = NULL;
	void (APIENTRY *qglIndexiv)(const GLint *c) = NULL;
	void (APIENTRY *qglIndexs)(GLshort c) = NULL;
	void (APIENTRY *qglIndexsv)(const GLshort *c) = NULL;
	void (APIENTRY *qglIndexub)(GLubyte c) = NULL;
	void (APIENTRY *qglIndexubv)(const GLubyte *c) = NULL;
	void (APIENTRY *qglInitNames)(void) = NULL;
	void (APIENTRY *qglInterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer) = NULL;
	GLboolean (APIENTRY *qglIsEnabled)(GLenum cap) = NULL;
	GLboolean (APIENTRY *qglIsList)(GLuint list) = NULL;
	GLboolean (APIENTRY *qglIsTexture)(GLuint texture) = NULL;
	void (APIENTRY *qglLightModelf)(GLenum pname, GLfloat param) = NULL;
	void (APIENTRY *qglLightModelfv)(GLenum pname, const GLfloat *params) = NULL;
	void (APIENTRY *qglLightModeli)(GLenum pname, GLint param) = NULL;
	void (APIENTRY *qglLightModeliv)(GLenum pname, const GLint *params) = NULL;
	void (APIENTRY *qglLightf)(GLenum light, GLenum pname, GLfloat param) = NULL;
	void (APIENTRY *qglLightfv)(GLenum light, GLenum pname, const GLfloat *params) = NULL;
	void (APIENTRY *qglLighti)(GLenum light, GLenum pname, GLint param) = NULL;
	void (APIENTRY *qglLightiv)(GLenum light, GLenum pname, const GLint *params) = NULL;
	void (APIENTRY *qglLineStipple)(GLint factor, GLushort pattern) = NULL;
	void (APIENTRY *qglLineWidth)(GLfloat width) = NULL;
	void (APIENTRY *qglListBase)(GLuint base) = NULL;
	void (APIENTRY *qglLoadIdentity)(void) = NULL;
	void (APIENTRY *qglLoadMatrixd)(const GLdouble *m) = NULL;
	void (APIENTRY *qglLoadMatrixf)(const GLfloat *m) = NULL;
	void (APIENTRY *qglLoadName)(GLuint name) = NULL;
	void (APIENTRY *qglLogicOp)(GLenum opcode) = NULL;
	void (APIENTRY *qglMap1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) = NULL;
	void (APIENTRY *qglMap1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) = NULL;
	void (APIENTRY *qglMap2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) = NULL;
	void (APIENTRY *qglMap2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) = NULL;
	void (APIENTRY *qglMapGrid1d)(GLint un, GLdouble u1, GLdouble u2) = NULL;
	void (APIENTRY *qglMapGrid1f)(GLint un, GLfloat u1, GLfloat u2) = NULL;
	void (APIENTRY *qglMapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) = NULL;
	void (APIENTRY *qglMapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) = NULL;
	void (APIENTRY *qglMaterialf)(GLenum face, GLenum pname, GLfloat param) = NULL;
	void (APIENTRY *qglMaterialfv)(GLenum face, GLenum pname, const GLfloat *params) = NULL;
	void (APIENTRY *qglMateriali)(GLenum face, GLenum pname, GLint param) = NULL;
	void (APIENTRY *qglMaterialiv)(GLenum face, GLenum pname, const GLint *params) = NULL;
	void (APIENTRY *qglMatrixMode)(GLenum mode) = NULL;
	void (APIENTRY *qglMultMatrixd)(const GLdouble *m) = NULL;
	void (APIENTRY *qglMultMatrixf)(const GLfloat *m) = NULL;
	void (APIENTRY *qglNewList)(GLuint list, GLenum mode) = NULL;
	void (APIENTRY *qglNormal3b)(GLbyte nx, GLbyte ny, GLbyte nz) = NULL;
	void (APIENTRY *qglNormal3bv)(const GLbyte *v) = NULL;
	void (APIENTRY *qglNormal3d)(GLdouble nx, GLdouble ny, GLdouble nz) = NULL;
	void (APIENTRY *qglNormal3dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz) = NULL;
	void (APIENTRY *qglNormal3fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglNormal3i)(GLint nx, GLint ny, GLint nz) = NULL;
	void (APIENTRY *qglNormal3iv)(const GLint *v) = NULL;
	void (APIENTRY *qglNormal3s)(GLshort nx, GLshort ny, GLshort nz) = NULL;
	void (APIENTRY *qglNormal3sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer) = NULL;
	void (APIENTRY *qglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) = NULL;
	void (APIENTRY *qglPassThrough)(GLfloat token) = NULL;
	void (APIENTRY *qglPixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat *values) = NULL;
	void (APIENTRY *qglPixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint *values) = NULL;
	void (APIENTRY *qglPixelMapusv)(GLenum map, GLsizei mapsize, const GLushort *values) = NULL;
	void (APIENTRY *qglPixelStoref)(GLenum pname, GLfloat param) = NULL;
	void (APIENTRY *qglPixelStorei)(GLenum pname, GLint param) = NULL;
	void (APIENTRY *qglPixelTransferf)(GLenum pname, GLfloat param) = NULL;
	void (APIENTRY *qglPixelTransferi)(GLenum pname, GLint param) = NULL;
	void (APIENTRY *qglPixelZoom)(GLfloat xfactor, GLfloat yfactor) = NULL;
	void (APIENTRY *qglPointSize)(GLfloat size) = NULL;
	void (APIENTRY *qglPolygonMode)(GLenum face, GLenum mode) = NULL;
	void (APIENTRY *qglPolygonOffset)(GLfloat factor, GLfloat units) = NULL;
	void (APIENTRY *qglPolygonStipple)(const GLubyte *mask) = NULL;
	void (APIENTRY *qglPopAttrib)(void) = NULL;
	void (APIENTRY *qglPopClientAttrib)(void) = NULL;
	void (APIENTRY *qglPopMatrix)(void) = NULL;
	void (APIENTRY *qglPopName)(void) = NULL;
	void (APIENTRY *qglPrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities) = NULL;
	void (APIENTRY *qglPushAttrib)(GLbitfield mask) = NULL;
	void (APIENTRY *qglPushClientAttrib)(GLbitfield mask) = NULL;
	void (APIENTRY *qglPushMatrix)(void) = NULL;
	void (APIENTRY *qglPushName)(GLuint name) = NULL;
	void (APIENTRY *qglRasterPos2d)(GLdouble x, GLdouble y) = NULL;
	void (APIENTRY *qglRasterPos2dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglRasterPos2f)(GLfloat x, GLfloat y) = NULL;
	void (APIENTRY *qglRasterPos2fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglRasterPos2i)(GLint x, GLint y) = NULL;
	void (APIENTRY *qglRasterPos2iv)(const GLint *v) = NULL;
	void (APIENTRY *qglRasterPos2s)(GLshort x, GLshort y) = NULL;
	void (APIENTRY *qglRasterPos2sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglRasterPos3d)(GLdouble x, GLdouble y, GLdouble z) = NULL;
	void (APIENTRY *qglRasterPos3dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglRasterPos3f)(GLfloat x, GLfloat y, GLfloat z) = NULL;
	void (APIENTRY *qglRasterPos3fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglRasterPos3i)(GLint x, GLint y, GLint z) = NULL;
	void (APIENTRY *qglRasterPos3iv)(const GLint *v) = NULL;
	void (APIENTRY *qglRasterPos3s)(GLshort x, GLshort y, GLshort z) = NULL;
	void (APIENTRY *qglRasterPos3sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglRasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w) = NULL;
	void (APIENTRY *qglRasterPos4dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglRasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w) = NULL;
	void (APIENTRY *qglRasterPos4fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglRasterPos4i)(GLint x, GLint y, GLint z, GLint w) = NULL;
	void (APIENTRY *qglRasterPos4iv)(const GLint *v) = NULL;
	void (APIENTRY *qglRasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w) = NULL;
	void (APIENTRY *qglRasterPos4sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglReadBuffer)(GLenum mode) = NULL;
	void (APIENTRY *qglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) = NULL;
	void (APIENTRY *qglRectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) = NULL;
	void (APIENTRY *qglRectdv)(const GLdouble *v1, const GLdouble *v2) = NULL;
	void (APIENTRY *qglRectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) = NULL;
	void (APIENTRY *qglRectfv)(const GLfloat *v1, const GLfloat *v2) = NULL;
	void (APIENTRY *qglRecti)(GLint x1, GLint y1, GLint x2, GLint y2) = NULL;
	void (APIENTRY *qglRectiv)(const GLint *v1, const GLint *v2) = NULL;
	void (APIENTRY *qglRects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2) = NULL;
	void (APIENTRY *qglRectsv)(const GLshort *v1, const GLshort *v2) = NULL;
	GLint (APIENTRY *qglRenderMode)(GLenum mode) = NULL;
	void (APIENTRY *qglRotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z) = NULL;
	void (APIENTRY *qglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) = NULL;
	void (APIENTRY *qglScaled)(GLdouble x, GLdouble y, GLdouble z) = NULL;
	void (APIENTRY *qglScalef)(GLfloat x, GLfloat y, GLfloat z) = NULL;
	void (APIENTRY *qglScissor)(GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
	void (APIENTRY *qglSelectBuffer)(GLsizei size, GLuint *buffer) = NULL;
	void (APIENTRY *qglShadeModel)(GLenum mode) = NULL;
	void (APIENTRY *qglStencilFunc)(GLenum func, GLint ref, GLuint mask) = NULL;
	void (APIENTRY *qglStencilMask)(GLuint mask) = NULL;
	void (APIENTRY *qglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass) = NULL;
	void (APIENTRY *qglTexCoord1d)(GLdouble s) = NULL;
	void (APIENTRY *qglTexCoord1dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglTexCoord1f)(GLfloat s) = NULL;
	void (APIENTRY *qglTexCoord1fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglTexCoord1i)(GLint s) = NULL;
	void (APIENTRY *qglTexCoord1iv)(const GLint *v) = NULL;
	void (APIENTRY *qglTexCoord1s)(GLshort s) = NULL;
	void (APIENTRY *qglTexCoord1sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglTexCoord2d)(GLdouble s, GLdouble t) = NULL;
	void (APIENTRY *qglTexCoord2dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglTexCoord2f)(GLfloat s, GLfloat t) = NULL;
	void (APIENTRY *qglTexCoord2fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglTexCoord2i)(GLint s, GLint t) = NULL;
	void (APIENTRY *qglTexCoord2iv)(const GLint *v) = NULL;
	void (APIENTRY *qglTexCoord2s)(GLshort s, GLshort t) = NULL;
	void (APIENTRY *qglTexCoord2sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglTexCoord3d)(GLdouble s, GLdouble t, GLdouble r) = NULL;
	void (APIENTRY *qglTexCoord3dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglTexCoord3f)(GLfloat s, GLfloat t, GLfloat r) = NULL;
	void (APIENTRY *qglTexCoord3fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglTexCoord3i)(GLint s, GLint t, GLint r) = NULL;
	void (APIENTRY *qglTexCoord3iv)(const GLint *v) = NULL;
	void (APIENTRY *qglTexCoord3s)(GLshort s, GLshort t, GLshort r) = NULL;
	void (APIENTRY *qglTexCoord3sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglTexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q) = NULL;
	void (APIENTRY *qglTexCoord4dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglTexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q) = NULL;
	void (APIENTRY *qglTexCoord4fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglTexCoord4i)(GLint s, GLint t, GLint r, GLint q) = NULL;
	void (APIENTRY *qglTexCoord4iv)(const GLint *v) = NULL;
	void (APIENTRY *qglTexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q) = NULL;
	void (APIENTRY *qglTexCoord4sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) = NULL;
	void (APIENTRY *qglTexEnvf)(GLenum target, GLenum pname, GLfloat param) = NULL;
	void (APIENTRY *qglTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params) = NULL;
	void (APIENTRY *qglTexEnvi)(GLenum target, GLenum pname, GLint param) = NULL;
	void (APIENTRY *qglTexEnviv)(GLenum target, GLenum pname, const GLint *params) = NULL;
	void (APIENTRY *qglTexGend)(GLenum coord, GLenum pname, GLdouble param) = NULL;
	void (APIENTRY *qglTexGendv)(GLenum coord, GLenum pname, const GLdouble *params) = NULL;
	void (APIENTRY *qglTexGenf)(GLenum coord, GLenum pname, GLfloat param) = NULL;
	void (APIENTRY *qglTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params) = NULL;
	void (APIENTRY *qglTexGeni)(GLenum coord, GLenum pname, GLint param) = NULL;
	void (APIENTRY *qglTexGeniv)(GLenum coord, GLenum pname, const GLint *params) = NULL;
	void (APIENTRY *qglTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
	void (APIENTRY *qglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
	void (APIENTRY *qglTexImage3D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * data) = NULL;
	void (APIENTRY *qglTexStorage2D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = NULL;
	void (APIENTRY *qglTexStorage3D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = NULL;
	void (APIENTRY *qglTexStorage2DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) = NULL;
	void (APIENTRY *qglTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) = NULL;
	void (APIENTRY *qglTextureView) (GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) = NULL;
	void (APIENTRY *qglCreateTextures) (GLenum target, GLsizei n, GLuint *textures) = NULL;
	void (APIENTRY *qglTexParameterf)(GLenum target, GLenum pname, GLfloat param) = NULL;
	void (APIENTRY *qglTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params) = NULL;
	void (APIENTRY *qglTexParameteri)(GLenum target, GLenum pname, GLint param) = NULL;
	void (APIENTRY *qglTexParameteriv)(GLenum target, GLenum pname, const GLint *params) = NULL;
	void (APIENTRY *qglTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
	void (APIENTRY *qglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) = NULL;
	void (APIENTRY *qglTranslated)(GLdouble x, GLdouble y, GLdouble z) = NULL;
	void (APIENTRY *qglTranslatef)(GLfloat x, GLfloat y, GLfloat z) = NULL;
	void (APIENTRY *qglVertex2d)(GLdouble x, GLdouble y) = NULL;
	void (APIENTRY *qglVertex2dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglVertex2f)(GLfloat x, GLfloat y) = NULL;
	void (APIENTRY *qglVertex2fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglVertex2i)(GLint x, GLint y) = NULL;
	void (APIENTRY *qglVertex2iv)(const GLint *v) = NULL;
	void (APIENTRY *qglVertex2s)(GLshort x, GLshort y) = NULL;
	void (APIENTRY *qglVertex2sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglVertex3d)(GLdouble x, GLdouble y, GLdouble z) = NULL;
	void (APIENTRY *qglVertex3dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglVertex3f)(GLfloat x, GLfloat y, GLfloat z) = NULL;
	void (APIENTRY *qglVertex3fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglVertex3i)(GLint x, GLint y, GLint z) = NULL;
	void (APIENTRY *qglVertex3iv)(const GLint *v) = NULL;
	void (APIENTRY *qglVertex3s)(GLshort x, GLshort y, GLshort z) = NULL;
	void (APIENTRY *qglVertex3sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglVertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w) = NULL;
	void (APIENTRY *qglVertex4dv)(const GLdouble *v) = NULL;
	void (APIENTRY *qglVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w) = NULL;
	void (APIENTRY *qglVertex4fv)(const GLfloat *v) = NULL;
	void (APIENTRY *qglVertex4i)(GLint x, GLint y, GLint z, GLint w) = NULL;
	void (APIENTRY *qglVertex4iv)(const GLint *v) = NULL;
	void (APIENTRY *qglVertex4s)(GLshort x, GLshort y, GLshort z, GLshort w) = NULL;
	void (APIENTRY *qglVertex4sv)(const GLshort *v) = NULL;
	void (APIENTRY *qglVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) = NULL;
	void (APIENTRY *qglVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer) = NULL;
	void (APIENTRY *qglVertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer) = NULL;
	void (APIENTRY *qglViewport)(GLint x, GLint y, GLsizei width, GLsizei height) = NULL;
	void (APIENTRY *qglSampleMaski)(GLuint maskNumber, GLbitfield mask) = NULL;
}

extern "C"
{
	int (WINAPI *qwglChoosePixelFormat)(HDC, CONST PIXELFORMATDESCRIPTOR *) = NULL;
	int (WINAPI *qwglDescribePixelFormat)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR) = NULL;
	int (WINAPI *qwglGetPixelFormat)(HDC) = NULL;
	BOOL (WINAPI *qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *) = NULL;
	BOOL (WINAPI *qwglSwapBuffers)(HDC) = NULL;

	BOOL (WINAPI *qwglCopyContext)(HGLRC, HGLRC, UINT) = NULL;
	HGLRC (WINAPI *qwglCreateContext)(HDC) = NULL;
	HGLRC (WINAPI *qwglCreateLayerContext)(HDC, int) = NULL;
	BOOL (WINAPI *qwglDeleteContext)(HGLRC) = NULL;
	HGLRC (WINAPI *qwglGetCurrentContext)(VOID) = NULL;
	HDC (WINAPI *qwglGetCurrentDC)(VOID) = NULL;
	PROC (WINAPI *qwglGetProcAddress)(LPCSTR) = NULL;
	BOOL (WINAPI *qwglMakeCurrent)(HDC, HGLRC) = NULL;
	BOOL (WINAPI *qwglShareLists)(HGLRC, HGLRC) = NULL;
	BOOL (WINAPI *qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD) = NULL;

	BOOL (WINAPI *qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT) = NULL;

	BOOL (WINAPI *qwglDescribeLayerPlane)(HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR) = NULL;
	int (WINAPI *qwglSetLayerPaletteEntries)(HDC, int, int, int, CONST COLORREF *) = NULL;
	int (WINAPI *qwglGetLayerPaletteEntries)(HDC, int, int, int, COLORREF *) = NULL;
	BOOL (WINAPI *qwglRealizeLayerPalette)(HDC, int, BOOL) = NULL;
	BOOL (WINAPI *qwglSwapLayerBuffers)(HDC, UINT) = NULL;

	BOOL (WINAPI *qwglSwapIntervalEXT)(int) = NULL;
}

PFNGLACTIVETEXTUREARBPROC qglActiveTextureARB = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC qglClientActiveTextureARB = NULL;
PFNGLMULTITEXCOORD2FARBPROC qglMultiTexCoord2fARB = NULL;
PFNGLMULTITEXCOORD3FARBPROC qglMultiTexCoord3fARB = NULL;
PFNGLBINDBUFFERARBPROC qglBindBufferARB = NULL;
PFNGLGENBUFFERSARBPROC qglGenBuffersARB = NULL;
PFNGLBUFFERDATAARBPROC qglBufferDataARB = NULL;
PFNGLDELETEBUFFERSARBPROC qglDeleteBuffersARB = NULL;
PFNGLLOCKARRAYSEXTPROC qglLockArraysEXT = NULL;
PFNGLUNLOCKARRAYSEXTPROC qglUnlockArraysEXT = NULL;
PFNGLTEXIMAGE3DEXTPROC qglTexImage3DEXT = NULL;
PFNGLGENPROGRAMSARBPROC qglGenProgramsARB = NULL;
PFNGLBINDPROGRAMARBPROC qglBindProgramARB = NULL;
PFNGLPROGRAMSTRINGARBPROC qglProgramStringARB = NULL;
PFNGLGETPROGRAMIVARBPROC qglGetProgramivARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC qglProgramLocalParameter4fARB = NULL;
PFNGLFOGCOORDPOINTEREXTPROC qglFogCoordPointer = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC qglCompressedTexImage2DARB = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC qglProgramEnvParameter4fARB = NULL;
PFNGLDELETEPROGRAMSARBPROC qglDeleteProgramsARB = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC qglDeleteFramebuffersEXT = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC qglDeleteRenderbuffersEXT = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC qglGenFramebuffersEXT = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC qglBindFramebufferEXT = NULL;
PFNGLGENRENDERBUFFERSEXTPROC qglGenRenderbuffersEXT = NULL;
PFNGLBINDRENDERBUFFEREXTPROC qglBindRenderbufferEXT = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC qglFramebufferRenderbufferEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC qglCheckFramebufferStatusEXT = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC qglRenderbufferStorageEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC qglFramebufferTexture2DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE3DEXTPROC qglFramebufferTexture3DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC qglFramebufferTextureLayerEXT = NULL;
PFNGLFRAMEBUFFERTEXTUREPROC qglFramebufferTexture = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC qglRenderbufferStorageMultisampleEXT = NULL;
PFNGLBLITFRAMEBUFFEREXTPROC qglBlitFramebufferEXT = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLECOVERAGENVPROC qglRenderbufferStorageMultisampleCoverageNV = NULL;
PFNGLGENVERTEXARRAYSPROC qglGenVertexArrays = NULL;
PFNGLBINDVERTEXARRAYPROC qglBindVertexArray = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC qglEnableVertexAttribArray = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC qglDisableVertexAttribArray = NULL;
PFNGLGENQUERIESPROC qglGenQueries = NULL;
PFNGLBEGINQUERYPROC qglBeginQuery = NULL;
PFNGLENDQUERYPROC qglEndQuery = NULL;
PFNGLBEGINCONDITIONALRENDERPROC qglBeginConditionalRender = NULL;
PFNGLENDCONDITIONALRENDERPROC qglEndConditionalRender = NULL;

//Shader ARB funcs
PFNGLCREATESHADEROBJECTARBPROC qglCreateShaderObjectARB = NULL;
PFNGLDETACHOBJECTARBPROC qglDetachObjectARB = NULL;
PFNGLDELETEOBJECTARBPROC qglDeleteObjectARB = NULL;
PFNGLSHADERSOURCEARBPROC qglShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC qglCompileShaderARB = NULL;
PFNGLCREATEPROGRAMOBJECTARBPROC qglCreateProgramObjectARB = NULL;
PFNGLATTACHOBJECTARBPROC qglAttachObjectARB = NULL;
PFNGLLINKPROGRAMARBPROC qglLinkProgramARB = NULL;
PFNGLUSEPROGRAMPROC qglUseProgram = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC qglUseProgramObjectARB = NULL;
PFNGLGETUNIFORMLOCATIONPROC qglGetUniformLocation = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC qglGetUniformLocationARB = NULL;
PFNGLGETATTRIBLOCATIONARBPROC qglGetAttribLocationARB = NULL;
PFNGLUNIFORM1IARBPROC qglUniform1iARB = NULL;
PFNGLUNIFORM2IARBPROC qglUniform2iARB = NULL;
PFNGLUNIFORM3IARBPROC qglUniform3iARB = NULL;
PFNGLUNIFORM4IARBPROC qglUniform4iARB = NULL;
PFNGLUNIFORM1FARBPROC qglUniform1fARB = NULL;
PFNGLUNIFORM2FARBPROC qglUniform2fARB = NULL;
PFNGLUNIFORM3FARBPROC qglUniform3fARB = NULL;
PFNGLUNIFORM4FARBPROC qglUniform4fARB = NULL;
PFNGLUNIFORM2FVARBPROC qglUniform2fvARB = NULL;
PFNGLUNIFORM3FVARBPROC qglUniform3fvARB = NULL;
PFNGLUNIFORM4FVARBPROC qglUniform4fvARB = NULL;
PFNGLUNIFORMMATRIX3FVARBPROC qglUniformMatrix3fvARB = NULL;
PFNGLUNIFORMMATRIX4FVARBPROC qglUniformMatrix4fvARB = NULL;
PFNGLUNIFORMMATRIX4FVARBPROC qglUniformMatrix3x4fvARB = NULL;
PFNGLUNIFORMMATRIX4FVARBPROC qglUniformMatrix4x3fvARB = NULL;
PFNGLVERTEXATTRIB3FPROC qglVertexAttrib3f = NULL;
PFNGLVERTEXATTRIB3FVPROC qglVertexAttrib3fv = NULL;
PFNGLGETSHADERIVPROC qglGetShaderiv = NULL;
PFNGLGETPROGRAMIVPROC qglGetProgramiv = NULL;
PFNGLGETSHADERINFOLOGPROC qglGetShaderInfoLog = NULL;
PFNGLGETPROGRAMINFOLOGPROC qglGetProgramInfoLog = NULL;
PFNGLGETINFOLOGARBPROC qglGetInfoLogARB = NULL;

void QGL_Init(void)
{
	HMODULE hOpenGL = GetModuleHandle("opengl32.dll");

	if (hOpenGL)
	{
		*(FARPROC *)&qglAccum = GetProcAddress(hOpenGL, "glAccum");
		*(FARPROC *)&qglAlphaFunc = GetProcAddress(hOpenGL, "glAlphaFunc");
		*(FARPROC *)&qglAreTexturesResident = GetProcAddress(hOpenGL, "glAreTexturesResident");
		*(FARPROC *)&qglArrayElement = GetProcAddress(hOpenGL, "glArrayElement");
		*(FARPROC *)&qglBegin = GetProcAddress(hOpenGL, "glBegin");
		*(FARPROC *)&qglBindTexture = GetProcAddress(hOpenGL, "glBindTexture");
		*(FARPROC *)&qglBitmap = GetProcAddress(hOpenGL, "glBitmap");
		*(FARPROC *)&qglBlendFunc = GetProcAddress(hOpenGL, "glBlendFunc");
		*(FARPROC *)&qglCallList = GetProcAddress(hOpenGL, "glCallList");
		*(FARPROC *)&qglCallLists = GetProcAddress(hOpenGL, "glCallLists");
		*(FARPROC *)&qglClear = GetProcAddress(hOpenGL, "glClear");
		*(FARPROC *)&qglClearAccum = GetProcAddress(hOpenGL, "glClearAccum");
		*(FARPROC *)&qglClearColor = GetProcAddress(hOpenGL, "glClearColor");
		*(FARPROC *)&qglClearDepth = GetProcAddress(hOpenGL, "glClearDepth");
		*(FARPROC *)&qglClearIndex = GetProcAddress(hOpenGL, "glClearIndex");
		*(FARPROC *)&qglClearStencil = GetProcAddress(hOpenGL, "glClearStencil");
		*(FARPROC *)&qglClipPlane = GetProcAddress(hOpenGL, "glClipPlane");
		*(FARPROC *)&qglColor3b = GetProcAddress(hOpenGL, "glColor3b");
		*(FARPROC *)&qglColor3bv = GetProcAddress(hOpenGL, "glColor3bv");
		*(FARPROC *)&qglColor3d = GetProcAddress(hOpenGL, "glColor3d");
		*(FARPROC *)&qglColor3dv = GetProcAddress(hOpenGL, "glColor3dv");
		*(FARPROC *)&qglColor3f = GetProcAddress(hOpenGL, "glColor3f");
		*(FARPROC *)&qglColor3fv = GetProcAddress(hOpenGL, "glColor3fv");
		*(FARPROC *)&qglColor3i = GetProcAddress(hOpenGL, "glColor3i");
		*(FARPROC *)&qglColor3iv = GetProcAddress(hOpenGL, "glColor3iv");
		*(FARPROC *)&qglColor3s = GetProcAddress(hOpenGL, "glColor3s");
		*(FARPROC *)&qglColor3sv = GetProcAddress(hOpenGL, "glColor3sv");
		*(FARPROC *)&qglColor3ub = GetProcAddress(hOpenGL, "glColor3ub");
		*(FARPROC *)&qglColor3ubv = GetProcAddress(hOpenGL, "glColor3ubv");
		*(FARPROC *)&qglColor3ui = GetProcAddress(hOpenGL, "glColor3ui");
		*(FARPROC *)&qglColor3uiv = GetProcAddress(hOpenGL, "glColor3uiv");
		*(FARPROC *)&qglColor3us = GetProcAddress(hOpenGL, "glColor3us");
		*(FARPROC *)&qglColor3usv = GetProcAddress(hOpenGL, "glColor3usv");
		*(FARPROC *)&qglColor4b = GetProcAddress(hOpenGL, "glColor4b");
		*(FARPROC *)&qglColor4bv = GetProcAddress(hOpenGL, "glColor4bv");
		*(FARPROC *)&qglColor4d = GetProcAddress(hOpenGL, "glColor4d");
		*(FARPROC *)&qglColor4dv = GetProcAddress(hOpenGL, "glColor4dv");
		*(FARPROC *)&qglColor4f = GetProcAddress(hOpenGL, "glColor4f");
		*(FARPROC *)&qglColor4fv = GetProcAddress(hOpenGL, "glColor4fv");
		*(FARPROC *)&qglColor4i = GetProcAddress(hOpenGL, "glColor4i");
		*(FARPROC *)&qglColor4iv = GetProcAddress(hOpenGL, "glColor4iv");
		*(FARPROC *)&qglColor4s = GetProcAddress(hOpenGL, "glColor4s");
		*(FARPROC *)&qglColor4sv = GetProcAddress(hOpenGL, "glColor4sv");
		*(FARPROC *)&qglColor4ub = GetProcAddress(hOpenGL, "glColor4ub");
		*(FARPROC *)&qglColor4ubv = GetProcAddress(hOpenGL, "glColor4ubv");
		*(FARPROC *)&qglColor4ui = GetProcAddress(hOpenGL, "glColor4ui");
		*(FARPROC *)&qglColor4uiv = GetProcAddress(hOpenGL, "glColor4uiv");
		*(FARPROC *)&qglColor4us = GetProcAddress(hOpenGL, "glColor4us");
		*(FARPROC *)&qglColor4usv = GetProcAddress(hOpenGL, "glColor4usv");
		*(FARPROC *)&qglColorMask = GetProcAddress(hOpenGL, "glColorMask");
		*(FARPROC *)&qglColorMaterial = GetProcAddress(hOpenGL, "glColorMaterial");
		*(FARPROC *)&qglColorPointer = GetProcAddress(hOpenGL, "glColorPointer");
		*(FARPROC *)&qglCopyPixels = GetProcAddress(hOpenGL, "glCopyPixels");
		*(FARPROC *)&qglCopyTexImage1D = GetProcAddress(hOpenGL, "glCopyTexImage1D");
		*(FARPROC *)&qglCopyTexImage2D = GetProcAddress(hOpenGL, "glCopyTexImage2D");
		*(FARPROC *)&qglCopyTexSubImage1D = GetProcAddress(hOpenGL, "glCopyTexSubImage1D");
		*(FARPROC *)&qglCopyTexSubImage2D = GetProcAddress(hOpenGL, "glCopyTexSubImage2D");
		*(FARPROC *)&qglCullFace = GetProcAddress(hOpenGL, "glCullFace");
		*(FARPROC *)&qglDeleteLists = GetProcAddress(hOpenGL, "glDeleteLists");
		*(FARPROC *)&qglDeleteTextures = GetProcAddress(hOpenGL, "glDeleteTextures");
		*(FARPROC *)&qglDepthFunc = GetProcAddress(hOpenGL, "glDepthFunc");
		*(FARPROC *)&qglDepthMask = GetProcAddress(hOpenGL, "glDepthMask");
		*(FARPROC *)&qglDepthRange = GetProcAddress(hOpenGL, "glDepthRange");
		*(FARPROC *)&qglDisable = GetProcAddress(hOpenGL, "glDisable");
		*(FARPROC *)&qglDisableClientState = GetProcAddress(hOpenGL, "glDisableClientState");
		*(FARPROC *)&qglDrawArrays = GetProcAddress(hOpenGL, "glDrawArrays");
		*(FARPROC *)&qglDrawBuffer = GetProcAddress(hOpenGL, "glDrawBuffer");
		*(FARPROC *)&qglDrawElements = GetProcAddress(hOpenGL, "glDrawElements");
		*(FARPROC *)&qglDrawPixels = GetProcAddress(hOpenGL, "glDrawPixels");
		*(FARPROC *)&qglEdgeFlag = GetProcAddress(hOpenGL, "glEdgeFlag");
		*(FARPROC *)&qglEdgeFlagPointer = GetProcAddress(hOpenGL, "glEdgeFlagPointer");
		*(FARPROC *)&qglEdgeFlagv = GetProcAddress(hOpenGL, "glEdgeFlagv");
		*(FARPROC *)&qglEnable = GetProcAddress(hOpenGL, "glEnable");
		*(FARPROC *)&qglEnableClientState = GetProcAddress(hOpenGL, "glEnableClientState");
		*(FARPROC *)&qglEnd = GetProcAddress(hOpenGL, "glEnd");
		*(FARPROC *)&qglEndList = GetProcAddress(hOpenGL, "glEndList");
		*(FARPROC *)&qglEvalCoord1d = GetProcAddress(hOpenGL, "glEvalCoord1d");
		*(FARPROC *)&qglEvalCoord1dv = GetProcAddress(hOpenGL, "glEvalCoord1dv");
		*(FARPROC *)&qglEvalCoord1f = GetProcAddress(hOpenGL, "glEvalCoord1f");
		*(FARPROC *)&qglEvalCoord1fv = GetProcAddress(hOpenGL, "glEvalCoord1fv");
		*(FARPROC *)&qglEvalCoord2d = GetProcAddress(hOpenGL, "glEvalCoord2d");
		*(FARPROC *)&qglEvalCoord2dv = GetProcAddress(hOpenGL, "glEvalCoord2dv");
		*(FARPROC *)&qglEvalCoord2f = GetProcAddress(hOpenGL, "glEvalCoord2f");
		*(FARPROC *)&qglEvalCoord2fv = GetProcAddress(hOpenGL, "glEvalCoord2fv");
		*(FARPROC *)&qglEvalMesh1 = GetProcAddress(hOpenGL, "glEvalMesh1");
		*(FARPROC *)&qglEvalMesh2 = GetProcAddress(hOpenGL, "glEvalMesh2");
		*(FARPROC *)&qglEvalPoint1 = GetProcAddress(hOpenGL, "glEvalPoint1");
		*(FARPROC *)&qglEvalPoint2 = GetProcAddress(hOpenGL, "glEvalPoint2");
		*(FARPROC *)&qglFeedbackBuffer = GetProcAddress(hOpenGL, "glFeedbackBuffer");
		*(FARPROC *)&qglFinish = GetProcAddress(hOpenGL, "glFinish");
		*(FARPROC *)&qglFlush = GetProcAddress(hOpenGL, "glFlush");
		*(FARPROC *)&qglFogf = GetProcAddress(hOpenGL, "glFogf");
		*(FARPROC *)&qglFogfv = GetProcAddress(hOpenGL, "glFogfv");
		*(FARPROC *)&qglFogi = GetProcAddress(hOpenGL, "glFogi");
		*(FARPROC *)&qglFogiv = GetProcAddress(hOpenGL, "glFogiv");
		*(FARPROC *)&qglFrontFace = GetProcAddress(hOpenGL, "glFrontFace");
		*(FARPROC *)&qglFrustum = GetProcAddress(hOpenGL, "glFrustum");
		*(FARPROC *)&qglGenLists = GetProcAddress(hOpenGL, "glGenLists");
		*(FARPROC *)&qglGenTextures = GetProcAddress(hOpenGL, "glGenTextures");
		*(FARPROC *)&qglGetBooleanv = GetProcAddress(hOpenGL, "glGetBooleanv");
		*(FARPROC *)&qglGetClipPlane = GetProcAddress(hOpenGL, "glGetClipPlane");
		*(FARPROC *)&qglGetDoublev = GetProcAddress(hOpenGL, "glGetDoublev");
		*(FARPROC *)&qglGetError = GetProcAddress(hOpenGL, "glGetError");
		*(FARPROC *)&qglGetFloatv = GetProcAddress(hOpenGL, "glGetFloatv");
		*(FARPROC *)&qglGetIntegerv = GetProcAddress(hOpenGL, "glGetIntegerv");
		*(FARPROC *)&qglGetLightfv = GetProcAddress(hOpenGL, "glGetLightfv");
		*(FARPROC *)&qglGetLightiv = GetProcAddress(hOpenGL, "glGetLightiv");
		*(FARPROC *)&qglGetMapdv = GetProcAddress(hOpenGL, "glGetMapdv");
		*(FARPROC *)&qglGetMapfv = GetProcAddress(hOpenGL, "glGetMapfv");
		*(FARPROC *)&qglGetMapiv = GetProcAddress(hOpenGL, "glGetMapiv");
		*(FARPROC *)&qglGetMaterialfv = GetProcAddress(hOpenGL, "glGetMaterialfv");
		*(FARPROC *)&qglGetMaterialiv = GetProcAddress(hOpenGL, "glGetMaterialiv");
		*(FARPROC *)&qglGetPixelMapfv = GetProcAddress(hOpenGL, "glGetPixelMapfv");
		*(FARPROC *)&qglGetPixelMapuiv = GetProcAddress(hOpenGL, "glGetPixelMapuiv");
		*(FARPROC *)&qglGetPixelMapusv = GetProcAddress(hOpenGL, "glGetPixelMapusv");
		*(FARPROC *)&qglGetPointerv = GetProcAddress(hOpenGL, "glGetPointerv");
		*(FARPROC *)&qglGetPolygonStipple = GetProcAddress(hOpenGL, "glGetPolygonStipple");
		*(FARPROC *)&qglGetString = GetProcAddress(hOpenGL, "glGetString");
		*(FARPROC *)&qglGetTexEnvfv = GetProcAddress(hOpenGL, "glGetTexEnvfv");
		*(FARPROC *)&qglGetTexEnviv = GetProcAddress(hOpenGL, "glGetTexEnviv");
		*(FARPROC *)&qglGetTexGendv = GetProcAddress(hOpenGL, "glGetTexGendv");
		*(FARPROC *)&qglGetTexGenfv = GetProcAddress(hOpenGL, "glGetTexGenfv");
		*(FARPROC *)&qglGetTexGeniv = GetProcAddress(hOpenGL, "glGetTexGeniv");
		*(FARPROC *)&qglGetTexImage = GetProcAddress(hOpenGL, "glGetTexImage");
		*(FARPROC *)&qglGetTexLevelParameterfv = GetProcAddress(hOpenGL, "glGetLevelParameterfv");
		*(FARPROC *)&qglGetTexLevelParameteriv = GetProcAddress(hOpenGL, "glGetLevelParameteriv");
		*(FARPROC *)&qglGetTexParameterfv = GetProcAddress(hOpenGL, "glGetTexParameterfv");
		*(FARPROC *)&qglGetTexParameteriv = GetProcAddress(hOpenGL, "glGetTexParameteriv");
		*(FARPROC *)&qglHint = GetProcAddress(hOpenGL, "glHint");
		*(FARPROC *)&qglIndexMask = GetProcAddress(hOpenGL, "glIndexMask");
		*(FARPROC *)&qglIndexPointer = GetProcAddress(hOpenGL, "glIndexPointer");
		*(FARPROC *)&qglIndexd = GetProcAddress(hOpenGL, "glIndexd");
		*(FARPROC *)&qglIndexdv = GetProcAddress(hOpenGL, "glIndexdv");
		*(FARPROC *)&qglIndexf = GetProcAddress(hOpenGL, "glIndexf");
		*(FARPROC *)&qglIndexfv = GetProcAddress(hOpenGL, "glIndexfv");
		*(FARPROC *)&qglIndexi = GetProcAddress(hOpenGL, "glIndexi");
		*(FARPROC *)&qglIndexiv = GetProcAddress(hOpenGL, "glIndexiv");
		*(FARPROC *)&qglIndexs = GetProcAddress(hOpenGL, "glIndexs");
		*(FARPROC *)&qglIndexsv = GetProcAddress(hOpenGL, "glIndexsv");
		*(FARPROC *)&qglIndexub = GetProcAddress(hOpenGL, "glIndexub");
		*(FARPROC *)&qglIndexubv = GetProcAddress(hOpenGL, "glIndexubv");
		*(FARPROC *)&qglInitNames = GetProcAddress(hOpenGL, "glInitNames");
		*(FARPROC *)&qglInterleavedArrays = GetProcAddress(hOpenGL, "glInterleavedArrays");
		*(FARPROC *)&qglIsEnabled = GetProcAddress(hOpenGL, "glIsEnabled");
		*(FARPROC *)&qglIsList = GetProcAddress(hOpenGL, "glIsList");
		*(FARPROC *)&qglIsTexture = GetProcAddress(hOpenGL, "glIsTexture");
		*(FARPROC *)&qglLightModelf = GetProcAddress(hOpenGL, "glLightModelf");
		*(FARPROC *)&qglLightModelfv = GetProcAddress(hOpenGL, "glLightModelfv");
		*(FARPROC *)&qglLightModeli = GetProcAddress(hOpenGL, "glLightModeli");
		*(FARPROC *)&qglLightModeliv = GetProcAddress(hOpenGL, "glLightModeliv");
		*(FARPROC *)&qglLightf = GetProcAddress(hOpenGL, "glLightf");
		*(FARPROC *)&qglLightfv = GetProcAddress(hOpenGL, "glLightfv");
		*(FARPROC *)&qglLighti = GetProcAddress(hOpenGL, "glLighti");
		*(FARPROC *)&qglLightiv = GetProcAddress(hOpenGL, "glLightiv");
		*(FARPROC *)&qglLineStipple = GetProcAddress(hOpenGL, "glLineStipple");
		*(FARPROC *)&qglLineWidth = GetProcAddress(hOpenGL, "glLineWidth");
		*(FARPROC *)&qglListBase = GetProcAddress(hOpenGL, "glListBase");
		*(FARPROC *)&qglLoadIdentity = GetProcAddress(hOpenGL, "glLoadIdentity");
		*(FARPROC *)&qglLoadMatrixd = GetProcAddress(hOpenGL, "glLoadMatrixd");
		*(FARPROC *)&qglLoadMatrixf = GetProcAddress(hOpenGL, "glLoadMatrixf");
		*(FARPROC *)&qglLoadName = GetProcAddress(hOpenGL, "glLoadName");
		*(FARPROC *)&qglLogicOp = GetProcAddress(hOpenGL, "glLogicOp");
		*(FARPROC *)&qglMap1d = GetProcAddress(hOpenGL, "glMap1d");
		*(FARPROC *)&qglMap1f = GetProcAddress(hOpenGL, "glMap1f");
		*(FARPROC *)&qglMap2d = GetProcAddress(hOpenGL, "glMap2d");
		*(FARPROC *)&qglMap2f = GetProcAddress(hOpenGL, "glMap2f");
		*(FARPROC *)&qglMapGrid1d = GetProcAddress(hOpenGL, "glMapGrid1d");
		*(FARPROC *)&qglMapGrid1f = GetProcAddress(hOpenGL, "glMapGrid1f");
		*(FARPROC *)&qglMapGrid2d = GetProcAddress(hOpenGL, "glMapGrid2d");
		*(FARPROC *)&qglMapGrid2f = GetProcAddress(hOpenGL, "glMapGrid2f");
		*(FARPROC *)&qglMaterialf = GetProcAddress(hOpenGL, "glMaterialf");
		*(FARPROC *)&qglMaterialfv = GetProcAddress(hOpenGL, "glMaterialfv");
		*(FARPROC *)&qglMateriali = GetProcAddress(hOpenGL, "glMateriali");
		*(FARPROC *)&qglMaterialiv = GetProcAddress(hOpenGL, "glMaterialiv");
		*(FARPROC *)&qglMatrixMode = GetProcAddress(hOpenGL, "glMatrixMode");
		*(FARPROC *)&qglMultMatrixd = GetProcAddress(hOpenGL, "glMultMatrixd");
		*(FARPROC *)&qglMultMatrixf = GetProcAddress(hOpenGL, "glMultMatrixf");
		*(FARPROC *)&qglNewList = GetProcAddress(hOpenGL, "glNewList");
		*(FARPROC *)&qglNormal3b = GetProcAddress(hOpenGL, "glNormal3b");
		*(FARPROC *)&qglNormal3bv = GetProcAddress(hOpenGL, "glNormal3bv");
		*(FARPROC *)&qglNormal3d = GetProcAddress(hOpenGL, "glNormal3d");
		*(FARPROC *)&qglNormal3dv = GetProcAddress(hOpenGL, "glNormal3dv");
		*(FARPROC *)&qglNormal3f = GetProcAddress(hOpenGL, "glNormal3f");
		*(FARPROC *)&qglNormal3fv = GetProcAddress(hOpenGL, "glNormal3fv");
		*(FARPROC *)&qglNormal3i = GetProcAddress(hOpenGL, "glNormal3i");
		*(FARPROC *)&qglNormal3iv = GetProcAddress(hOpenGL, "glNormal3iv");
		*(FARPROC *)&qglNormal3s = GetProcAddress(hOpenGL, "glNormal3s");
		*(FARPROC *)&qglNormal3sv = GetProcAddress(hOpenGL, "glNormal3sv");
		*(FARPROC *)&qglNormalPointer = GetProcAddress(hOpenGL, "glNormalPointer");
		*(FARPROC *)&qglOrtho = GetProcAddress(hOpenGL, "glOrtho");
		*(FARPROC *)&qglPassThrough = GetProcAddress(hOpenGL, "glPassThrough");
		*(FARPROC *)&qglPixelMapfv = GetProcAddress(hOpenGL, "glPixelMapfv");
		*(FARPROC *)&qglPixelMapuiv = GetProcAddress(hOpenGL, "glPixelMapuiv");
		*(FARPROC *)&qglPixelMapusv = GetProcAddress(hOpenGL, "glPixelMapusv");
		*(FARPROC *)&qglPixelStoref = GetProcAddress(hOpenGL, "glPixelStoref");
		*(FARPROC *)&qglPixelStorei = GetProcAddress(hOpenGL, "glPixelStorei");
		*(FARPROC *)&qglPixelTransferf = GetProcAddress(hOpenGL, "glPixelTransferf");
		*(FARPROC *)&qglPixelTransferi = GetProcAddress(hOpenGL, "glPixelTransferi");
		*(FARPROC *)&qglPixelZoom = GetProcAddress(hOpenGL, "glPixelZoom");
		*(FARPROC *)&qglPointSize = GetProcAddress(hOpenGL, "glPointSize");
		*(FARPROC *)&qglPolygonMode = GetProcAddress(hOpenGL, "glPolygonMode");
		*(FARPROC *)&qglPolygonOffset = GetProcAddress(hOpenGL, "glPolygonOffset");
		*(FARPROC *)&qglPolygonStipple = GetProcAddress(hOpenGL, "glPolygonStipple");
		*(FARPROC *)&qglPopAttrib = GetProcAddress(hOpenGL, "glPopAttrib");
		*(FARPROC *)&qglPopClientAttrib = GetProcAddress(hOpenGL, "glPopClientAttrib");
		*(FARPROC *)&qglPopMatrix = GetProcAddress(hOpenGL, "glPopMatrix");
		*(FARPROC *)&qglPopName = GetProcAddress(hOpenGL, "glPopName");
		*(FARPROC *)&qglPrioritizeTextures = GetProcAddress(hOpenGL, "glPrioritizeTextures");
		*(FARPROC *)&qglPushAttrib = GetProcAddress(hOpenGL, "glPushAttrib");
		*(FARPROC *)&qglPushClientAttrib = GetProcAddress(hOpenGL, "glPushClientAttrib");
		*(FARPROC *)&qglPushMatrix = GetProcAddress(hOpenGL, "glPushMatrix");
		*(FARPROC *)&qglPushName = GetProcAddress(hOpenGL, "glPushName");
		*(FARPROC *)&qglRasterPos2d = GetProcAddress(hOpenGL, "glRasterPos2d");
		*(FARPROC *)&qglRasterPos2dv = GetProcAddress(hOpenGL, "glRasterPos2dv");
		*(FARPROC *)&qglRasterPos2f = GetProcAddress(hOpenGL, "glRasterPos2f");
		*(FARPROC *)&qglRasterPos2fv = GetProcAddress(hOpenGL, "glRasterPos2fv");
		*(FARPROC *)&qglRasterPos2i = GetProcAddress(hOpenGL, "glRasterPos2i");
		*(FARPROC *)&qglRasterPos2iv = GetProcAddress(hOpenGL, "glRasterPos2iv");
		*(FARPROC *)&qglRasterPos2s = GetProcAddress(hOpenGL, "glRasterPos2s");
		*(FARPROC *)&qglRasterPos2sv = GetProcAddress(hOpenGL, "glRasterPos2sv");
		*(FARPROC *)&qglRasterPos3d = GetProcAddress(hOpenGL, "glRasterPos3d");
		*(FARPROC *)&qglRasterPos3dv = GetProcAddress(hOpenGL, "glRasterPos3dv");
		*(FARPROC *)&qglRasterPos3f = GetProcAddress(hOpenGL, "glRasterPos3f");
		*(FARPROC *)&qglRasterPos3fv = GetProcAddress(hOpenGL, "glRasterPos3fv");
		*(FARPROC *)&qglRasterPos3i = GetProcAddress(hOpenGL, "glRasterPos3i");
		*(FARPROC *)&qglRasterPos3iv = GetProcAddress(hOpenGL, "glRasterPos3iv");
		*(FARPROC *)&qglRasterPos3s = GetProcAddress(hOpenGL, "glRasterPos3s");
		*(FARPROC *)&qglRasterPos3sv = GetProcAddress(hOpenGL, "glRasterPos3sv");
		*(FARPROC *)&qglRasterPos4d = GetProcAddress(hOpenGL, "glRasterPos4d");
		*(FARPROC *)&qglRasterPos4dv = GetProcAddress(hOpenGL, "glRasterPos4dv");
		*(FARPROC *)&qglRasterPos4f = GetProcAddress(hOpenGL, "glRasterPos4f");
		*(FARPROC *)&qglRasterPos4fv = GetProcAddress(hOpenGL, "glRasterPos4fv");
		*(FARPROC *)&qglRasterPos4i = GetProcAddress(hOpenGL, "glRasterPos4i");
		*(FARPROC *)&qglRasterPos4iv = GetProcAddress(hOpenGL, "glRasterPos4iv");
		*(FARPROC *)&qglRasterPos4s = GetProcAddress(hOpenGL, "glRasterPos4s");
		*(FARPROC *)&qglRasterPos4sv = GetProcAddress(hOpenGL, "glRasterPos4sv");
		*(FARPROC *)&qglReadBuffer = GetProcAddress(hOpenGL, "glReadBuffer");
		*(FARPROC *)&qglReadPixels = GetProcAddress(hOpenGL, "glReadPixels");
		*(FARPROC *)&qglRectd = GetProcAddress(hOpenGL, "glRectd");
		*(FARPROC *)&qglRectdv = GetProcAddress(hOpenGL, "glRectdv");
		*(FARPROC *)&qglRectf = GetProcAddress(hOpenGL, "glRectf");
		*(FARPROC *)&qglRectfv = GetProcAddress(hOpenGL, "glRectfv");
		*(FARPROC *)&qglRecti = GetProcAddress(hOpenGL, "glRecti");
		*(FARPROC *)&qglRectiv = GetProcAddress(hOpenGL, "glRectiv");
		*(FARPROC *)&qglRects = GetProcAddress(hOpenGL, "glRects");
		*(FARPROC *)&qglRectsv = GetProcAddress(hOpenGL, "glRectsv");
		*(FARPROC *)&qglRenderMode = GetProcAddress(hOpenGL, "glRenderMode");
		*(FARPROC *)&qglRotated = GetProcAddress(hOpenGL, "glRotated");
		*(FARPROC *)&qglRotatef = GetProcAddress(hOpenGL, "glRotatef");
		*(FARPROC *)&qglScaled = GetProcAddress(hOpenGL, "glScaled");
		*(FARPROC *)&qglScalef = GetProcAddress(hOpenGL, "glScalef");
		*(FARPROC *)&qglScissor = GetProcAddress(hOpenGL, "glScissor");
		*(FARPROC *)&qglSelectBuffer = GetProcAddress(hOpenGL, "glSelectBuffer");
		*(FARPROC *)&qglShadeModel = GetProcAddress(hOpenGL, "glShadeModel");
		*(FARPROC *)&qglStencilFunc = GetProcAddress(hOpenGL, "glStencilFunc");
		*(FARPROC *)&qglStencilMask = GetProcAddress(hOpenGL, "glStencilMask");
		*(FARPROC *)&qglStencilOp = GetProcAddress(hOpenGL, "glStencilOp");
		*(FARPROC *)&qglTexCoord1d = GetProcAddress(hOpenGL, "glTexCoord1d");
		*(FARPROC *)&qglTexCoord1dv = GetProcAddress(hOpenGL, "glTexCoord1dv");
		*(FARPROC *)&qglTexCoord1f = GetProcAddress(hOpenGL, "glTexCoord1f");
		*(FARPROC *)&qglTexCoord1fv = GetProcAddress(hOpenGL, "glTexCoord1fv");
		*(FARPROC *)&qglTexCoord1i = GetProcAddress(hOpenGL, "glTexCoord1i");
		*(FARPROC *)&qglTexCoord1iv = GetProcAddress(hOpenGL, "glTexCoord1iv");
		*(FARPROC *)&qglTexCoord1s = GetProcAddress(hOpenGL, "glTexCoord1s");
		*(FARPROC *)&qglTexCoord1sv = GetProcAddress(hOpenGL, "glTexCoord1sv");
		*(FARPROC *)&qglTexCoord2d = GetProcAddress(hOpenGL, "glTexCoord2d");
		*(FARPROC *)&qglTexCoord2dv = GetProcAddress(hOpenGL, "glTexCoord2dv");
		*(FARPROC *)&qglTexCoord2f = GetProcAddress(hOpenGL, "glTexCoord2f");
		*(FARPROC *)&qglTexCoord2fv = GetProcAddress(hOpenGL, "glTexCoord2fv");
		*(FARPROC *)&qglTexCoord2i = GetProcAddress(hOpenGL, "glTexCoord2i");
		*(FARPROC *)&qglTexCoord2iv = GetProcAddress(hOpenGL, "glTexCoord2iv");
		*(FARPROC *)&qglTexCoord2s = GetProcAddress(hOpenGL, "glTexCoord2s");
		*(FARPROC *)&qglTexCoord2sv = GetProcAddress(hOpenGL, "glTexCoord2sv");
		*(FARPROC *)&qglTexCoord3d = GetProcAddress(hOpenGL, "glTexCoord3d");
		*(FARPROC *)&qglTexCoord3dv = GetProcAddress(hOpenGL, "glTexCoord3dv");
		*(FARPROC *)&qglTexCoord3f = GetProcAddress(hOpenGL, "glTexCoord3f");
		*(FARPROC *)&qglTexCoord3fv = GetProcAddress(hOpenGL, "glTexCoord3fv");
		*(FARPROC *)&qglTexCoord3i = GetProcAddress(hOpenGL, "glTexCoord3i");
		*(FARPROC *)&qglTexCoord3iv = GetProcAddress(hOpenGL, "glTexCoord3iv");
		*(FARPROC *)&qglTexCoord3s = GetProcAddress(hOpenGL, "glTexCoord3s");
		*(FARPROC *)&qglTexCoord3sv = GetProcAddress(hOpenGL, "glTexCoord3sv");
		*(FARPROC *)&qglTexCoord4d = GetProcAddress(hOpenGL, "glTexCoord4d");
		*(FARPROC *)&qglTexCoord4dv = GetProcAddress(hOpenGL, "glTexCoord4dv");
		*(FARPROC *)&qglTexCoord4f = GetProcAddress(hOpenGL, "glTexCoord4f");
		*(FARPROC *)&qglTexCoord4fv = GetProcAddress(hOpenGL, "glTexCoord4fv");
		*(FARPROC *)&qglTexCoord4i = GetProcAddress(hOpenGL, "glTexCoord4i");
		*(FARPROC *)&qglTexCoord4iv = GetProcAddress(hOpenGL, "glTexCoord4iv");
		*(FARPROC *)&qglTexCoord4s = GetProcAddress(hOpenGL, "glTexCoord4s");
		*(FARPROC *)&qglTexCoord4sv = GetProcAddress(hOpenGL, "glTexCoord4sv");
		*(FARPROC *)&qglTexCoordPointer = GetProcAddress(hOpenGL, "glTexCoordPointer");
		*(FARPROC *)&qglTexEnvf = GetProcAddress(hOpenGL, "glTexEnvf");
		*(FARPROC *)&qglTexEnvfv = GetProcAddress(hOpenGL, "glTexEnvfv");
		*(FARPROC *)&qglTexEnvi = GetProcAddress(hOpenGL, "glTexEnvi");
		*(FARPROC *)&qglTexEnviv = GetProcAddress(hOpenGL, "glTexEnviv");
		*(FARPROC *)&qglTexGend = GetProcAddress(hOpenGL, "glTexGend");
		*(FARPROC *)&qglTexGendv = GetProcAddress(hOpenGL, "glTexGendv");
		*(FARPROC *)&qglTexGenf = GetProcAddress(hOpenGL, "glTexGenf");
		*(FARPROC *)&qglTexGenfv = GetProcAddress(hOpenGL, "glTexGenfv");
		*(FARPROC *)&qglTexGeni = GetProcAddress(hOpenGL, "glTexGeni");
		*(FARPROC *)&qglTexGeniv = GetProcAddress(hOpenGL, "glTexGeniv");
		*(FARPROC *)&qglTexImage1D = GetProcAddress(hOpenGL, "glTexImage1D");
		*(FARPROC *)&qglTexImage2D = GetProcAddress(hOpenGL, "glTexImage2D");
		*(FARPROC *)&qglTexParameterf = GetProcAddress(hOpenGL, "glTexParameterf");
		*(FARPROC *)&qglTexParameterfv = GetProcAddress(hOpenGL, "glTexParameterfv");
		*(FARPROC *)&qglTexParameteri = GetProcAddress(hOpenGL, "glTexParameteri");
		*(FARPROC *)&qglTexParameteriv = GetProcAddress(hOpenGL, "glTexParameteriv");
		*(FARPROC *)&qglTexSubImage1D = GetProcAddress(hOpenGL, "glTexSubImage1D");
		*(FARPROC *)&qglTexSubImage2D = GetProcAddress(hOpenGL, "glTexSubImage2D");
		*(FARPROC *)&qglTranslated = GetProcAddress(hOpenGL, "glTranslated");
		*(FARPROC *)&qglTranslatef = GetProcAddress(hOpenGL, "glTranslatef");
		*(FARPROC *)&qglVertex2d = GetProcAddress(hOpenGL, "glVertex2d");
		*(FARPROC *)&qglVertex2dv = GetProcAddress(hOpenGL, "glVertex2dv");
		*(FARPROC *)&qglVertex2f = GetProcAddress(hOpenGL, "glVertex2f");
		*(FARPROC *)&qglVertex2fv = GetProcAddress(hOpenGL, "glVertex2fv");
		*(FARPROC *)&qglVertex2i = GetProcAddress(hOpenGL, "glVertex2i");
		*(FARPROC *)&qglVertex2iv = GetProcAddress(hOpenGL, "glVertex2iv");
		*(FARPROC *)&qglVertex2s = GetProcAddress(hOpenGL, "glVertex2s");
		*(FARPROC *)&qglVertex2sv = GetProcAddress(hOpenGL, "glVertex2sv");
		*(FARPROC *)&qglVertex3d = GetProcAddress(hOpenGL, "glVertex3d");
		*(FARPROC *)&qglVertex3dv = GetProcAddress(hOpenGL, "glVertex3dv");
		*(FARPROC *)&qglVertex3f = GetProcAddress(hOpenGL, "glVertex3f");
		*(FARPROC *)&qglVertex3fv = GetProcAddress(hOpenGL, "glVertex3fv");
		*(FARPROC *)&qglVertex3i = GetProcAddress(hOpenGL, "glVertex3i");
		*(FARPROC *)&qglVertex3iv = GetProcAddress(hOpenGL, "glVertex3iv");
		*(FARPROC *)&qglVertex3s = GetProcAddress(hOpenGL, "glVertex3s");
		*(FARPROC *)&qglVertex3sv = GetProcAddress(hOpenGL, "glVertex3sv");
		*(FARPROC *)&qglVertex4d = GetProcAddress(hOpenGL, "glVertex4d");
		*(FARPROC *)&qglVertex4dv = GetProcAddress(hOpenGL, "glVertex4dv");
		*(FARPROC *)&qglVertex4f = GetProcAddress(hOpenGL, "glVertex4f");
		*(FARPROC *)&qglVertex4fv = GetProcAddress(hOpenGL, "glVertex4fv");
		*(FARPROC *)&qglVertex4i = GetProcAddress(hOpenGL, "glVertex4i");
		*(FARPROC *)&qglVertex4iv = GetProcAddress(hOpenGL, "glVertex4iv");
		*(FARPROC *)&qglVertex4s = GetProcAddress(hOpenGL, "glVertex4s");
		*(FARPROC *)&qglVertex4sv = GetProcAddress(hOpenGL, "glVertex4sv");
		*(FARPROC *)&qglVertexPointer = GetProcAddress(hOpenGL, "glVertexPointer");
		*(FARPROC *)&qglViewport = GetProcAddress(hOpenGL, "glViewport");

		*(FARPROC *)&qwglCopyContext = GetProcAddress(hOpenGL, "wglCopyContext");
		*(FARPROC *)&qwglCreateContext = GetProcAddress(hOpenGL, "wglCreateContext");
		*(FARPROC *)&qwglCreateLayerContext = GetProcAddress(hOpenGL, "wglCreateLayerContext");
		*(FARPROC *)&qwglDeleteContext = GetProcAddress(hOpenGL, "wglDeleteContext");
		*(FARPROC *)&qwglDescribeLayerPlane = GetProcAddress(hOpenGL, "wglDescribeLayerPlane");
		*(FARPROC *)&qwglGetCurrentContext = GetProcAddress(hOpenGL, "wglGetCurrentContext");
		*(FARPROC *)&qwglGetCurrentDC = GetProcAddress(hOpenGL, "wglGetCurrentDC");
		*(FARPROC *)&qwglGetLayerPaletteEntries = GetProcAddress(hOpenGL, "wglGetLayerPaletteEntries");
		*(FARPROC *)&qwglGetProcAddress = GetProcAddress(hOpenGL, "wglGetProcAddress");
		*(FARPROC *)&qwglMakeCurrent = GetProcAddress(hOpenGL, "wglMakeCurrent");
		*(FARPROC *)&qwglRealizeLayerPalette = GetProcAddress(hOpenGL, "wglRealizeLayerPalette");
		*(FARPROC *)&qwglSetLayerPaletteEntries = GetProcAddress(hOpenGL, "wglSetLayerPaletteEntries");
		*(FARPROC *)&qwglShareLists = GetProcAddress(hOpenGL, "wglShareLists");
		*(FARPROC *)&qwglSwapLayerBuffers = GetProcAddress(hOpenGL, "wglSwapLayerBuffers");
		*(FARPROC *)&qwglUseFontBitmaps = GetProcAddress(hOpenGL, "wglUseFontBitmapsA");
		*(FARPROC *)&qwglUseFontOutlines = GetProcAddress(hOpenGL, "wglUseFontOutlinesA");

		*(FARPROC *)&qwglChoosePixelFormat = GetProcAddress(hOpenGL, "wglChoosePixelFormat");
		*(FARPROC *)&qwglDescribePixelFormat = GetProcAddress(hOpenGL, "wglDescribePixelFormat");
		*(FARPROC *)&qwglGetPixelFormat = GetProcAddress(hOpenGL, "wglGetPixelFormat");
		*(FARPROC *)&qwglSetPixelFormat = GetProcAddress(hOpenGL, "wglSetPixelFormat");
		*(FARPROC *)&qwglSwapBuffers = GetProcAddress(hOpenGL, "wglSwapBuffers");

		*(FARPROC *)&qwglSwapIntervalEXT = qwglGetProcAddress("wglSwapIntervalEXT");
		*(FARPROC *)&qglTexSubImage3D = qwglGetProcAddress("glTexSubImage3D");
		*(FARPROC *)&qglSampleMaski = qwglGetProcAddress("glSampleMaski");
	}

	QGL_InitExtension();
}

void QGL_InitExtension(void)
{
	const char *extension = (const char *)qglGetString(GL_EXTENSIONS);

	gl_max_texture_size = 128;
	qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size);

	gl_max_ansio = 1;
	if (strstr(extension, "GL_EXT_texture_filter_anisotropic"))
	{
		qglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max_ansio);
	}

	if (strstr(extension, "GL_ARB_texture_view"))
	{
		*(FARPROC *)&qglCreateTextures = qwglGetProcAddress("glCreateTextures");
		*(FARPROC *)&qglTextureView = qwglGetProcAddress("glTextureView");
	}

	if (strstr(extension, "GL_EXT_texture_storage"))
	{
		*(FARPROC *)&qglTexStorage2D = qwglGetProcAddress("glTexStorage2D");
		*(FARPROC *)&qglTexStorage3D = qwglGetProcAddress("glTexStorage3D");
		*(FARPROC *)&qglTexStorage2DMultisample = qwglGetProcAddress("glTexStorage2DMultisample");
		*(FARPROC *)&qglTexImage3D = qwglGetProcAddress("glTexImage3D");
	}

	if (strstr(extension, "GL_ARB_multitexture"))
	{
		qglActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)qwglGetProcAddress("glActiveTextureARB");
		qglClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC)qwglGetProcAddress("glClientActiveTextureARB");
		qglMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)qwglGetProcAddress("glMultiTexCoord2fARB");
		qglMultiTexCoord3fARB = (PFNGLMULTITEXCOORD3FARBPROC)qwglGetProcAddress("glMultiTexCoord3fARB");

		TEXTURE0_SGIS = GL_TEXTURE0;
		TEXTURE1_SGIS = GL_TEXTURE1;
		TEXTURE2_SGIS = GL_TEXTURE2;
		TEXTURE3_SGIS = GL_TEXTURE3;
	}

	if (strstr(extension, "GL_ARB_vertex_buffer_object"))
	{
		qglBindBufferARB = (PFNGLBINDBUFFERARBPROC)qwglGetProcAddress("glBindBufferARB");
		qglGenBuffersARB = (PFNGLGENBUFFERSARBPROC)qwglGetProcAddress("glGenBuffersARB");
		qglBufferDataARB = (PFNGLBUFFERDATAARBPROC)qwglGetProcAddress("glBufferDataARB");
		qglDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)qwglGetProcAddress("glDeleteBuffersARB");
		*(FARPROC *)&qglVertexAttribPointer = qwglGetProcAddress("glVertexAttribPointer");
		*(FARPROC *)&qglVertexAttribIPointer = qwglGetProcAddress("glVertexAttribIPointer");
	}

	if (strstr(extension, "GL_ARB_vertex_array_object"))
	{
		*(FARPROC *)&qglGenVertexArrays = qwglGetProcAddress("glGenVertexArrays");
		*(FARPROC *)&qglBindVertexArray = qwglGetProcAddress("glBindVertexArray");
		*(FARPROC *)&qglEnableVertexAttribArray = qwglGetProcAddress("glEnableVertexAttribArray");
		*(FARPROC *)&qglDisableVertexAttribArray = qwglGetProcAddress("glDisableVertexAttribArray");
	}

	if (strstr(extension, "GL_EXT_texture3D"))
	{
		qglTexImage3DEXT = (PFNGLTEXIMAGE3DEXTPROC)qwglGetProcAddress("glTexImage3DEXT");
	}

	if (strstr(extension, "GL_ARB_vertex_program") && strstr(extension, "GL_ARB_fragment_program"))
	{
		qglGenProgramsARB = (PFNGLGENPROGRAMSARBPROC)qwglGetProcAddress("glGenProgramsARB");
		qglBindProgramARB = (PFNGLBINDPROGRAMARBPROC)qwglGetProcAddress("glBindProgramARB");
		qglProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)qwglGetProcAddress("glProgramStringARB");
		qglGetProgramivARB = (PFNGLGETPROGRAMIVARBPROC)qwglGetProcAddress("glGetProgramivARB");
		qglProgramLocalParameter4fARB = (PFNGLPROGRAMLOCALPARAMETER4FARBPROC)qwglGetProcAddress("glProgramLocalParameter4fARB");
		qglProgramEnvParameter4fARB = (PFNGLPROGRAMENVPARAMETER4FARBPROC)qwglGetProcAddress("glProgramEnvParameter4fARB");
		qglDeleteProgramsARB = (PFNGLDELETEPROGRAMSARBPROC)qwglGetProcAddress("glDeleteProgramsARB");

		gl_program_support = true;
	}

	if (strstr(extension, "GL_ARB_shader_objects") && strstr(extension, "GL_ARB_vertex_shader") && strstr(extension, "GL_ARB_fragment_shader"))
	{
		qglCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)qwglGetProcAddress("glCreateShaderObjectARB");
		qglDetachObjectARB = (PFNGLDETACHOBJECTARBPROC)qwglGetProcAddress("glDetachObjectARB");
		qglDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC)qwglGetProcAddress("glDeleteObjectARB");
		qglShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)qwglGetProcAddress("glShaderSourceARB");
		qglCompileShaderARB = (PFNGLCOMPILESHADERARBPROC)qwglGetProcAddress("glCompileShaderARB");
		qglCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glCreateProgramObjectARB");
		qglAttachObjectARB = (PFNGLATTACHOBJECTARBPROC)qwglGetProcAddress("glAttachObjectARB");
		qglLinkProgramARB = (PFNGLLINKPROGRAMARBPROC)qwglGetProcAddress("glLinkProgramARB");
		qglUseProgram = (PFNGLUSEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glUseProgram");
		qglUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glUseProgramObjectARB");
		qglGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)qwglGetProcAddress("glGetUniformLocationARB");
		qglGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)qwglGetProcAddress("glGetUniformLocation");

		qglUniform1iARB = (PFNGLUNIFORM1IARBPROC)qwglGetProcAddress("glUniform1iARB");
		qglUniform2iARB = (PFNGLUNIFORM2IARBPROC)qwglGetProcAddress("glUniform2iARB");
		qglUniform3iARB = (PFNGLUNIFORM3IARBPROC)qwglGetProcAddress("glUniform3iARB");
		qglUniform4iARB = (PFNGLUNIFORM4IARBPROC)qwglGetProcAddress("glUniform4iARB");
		qglUniform1fARB = (PFNGLUNIFORM1FARBPROC)qwglGetProcAddress("glUniform1fARB");
		qglUniform2fARB = (PFNGLUNIFORM2FARBPROC)qwglGetProcAddress("glUniform2fARB");
		qglUniform3fARB = (PFNGLUNIFORM3FARBPROC)qwglGetProcAddress("glUniform3fARB");
		qglUniform4fARB = (PFNGLUNIFORM4FARBPROC)qwglGetProcAddress("glUniform4fARB");
		qglUniform2fvARB = (PFNGLUNIFORM2FVARBPROC)qwglGetProcAddress("glUniform2fvARB");
		qglUniform3fvARB = (PFNGLUNIFORM3FVARBPROC)qwglGetProcAddress("glUniform3fvARB");
		qglUniform4fvARB = (PFNGLUNIFORM4FVARBPROC)qwglGetProcAddress("glUniform4fvARB");
		qglUniformMatrix3fvARB = (PFNGLUNIFORMMATRIX3FVARBPROC)qwglGetProcAddress("glUniformMatrix3fvARB");
		qglUniformMatrix4fvARB = (PFNGLUNIFORMMATRIX4FVARBPROC)qwglGetProcAddress("glUniformMatrix4fvARB");
		qglUniformMatrix4x3fvARB = (PFNGLUNIFORMMATRIX4FVARBPROC)qwglGetProcAddress("glUniformMatrix4x3fv");
		qglUniformMatrix3x4fvARB = (PFNGLUNIFORMMATRIX4FVARBPROC)qwglGetProcAddress("glUniformMatrix3x4fv");

		qglGetShaderiv = (PFNGLGETSHADERIVPROC)qwglGetProcAddress("glGetShaderiv");
		qglGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)qwglGetProcAddress("glGetShaderInfoLog");
		qglGetProgramiv = (PFNGLGETPROGRAMIVPROC)qwglGetProcAddress("glGetProgramiv");
		qglGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)qwglGetProcAddress("glGetProgramInfoLog");
		qglGetInfoLogARB = (PFNGLGETINFOLOGARBPROC)qwglGetProcAddress("glGetInfoLogARB");
		qglGetAttribLocationARB = (PFNGLGETATTRIBLOCATIONARBPROC)qwglGetProcAddress("glGetAttribLocationARB");
		qglVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)qwglGetProcAddress("glVertexAttrib3f");
		qglVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)qwglGetProcAddress("glVertexAttrib3fv");

		gl_shader_support = true;
	}

	if (strstr(extension, "GL_ARB_texture_compression"))
	{
		qglCompressedTexImage2DARB = (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)qwglGetProcAddress("glCompressedTexImage2DARB");
	}

	if (strstr(extension, "GL_EXT_framebuffer_object"))
	{
		qglDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)qwglGetProcAddress("glDeleteFramebuffersEXT");
		qglDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)qwglGetProcAddress("glDeleteRenderbuffersEXT");
		qglGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)qwglGetProcAddress("glGenFramebuffersEXT");
		qglBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)qwglGetProcAddress("glBindFramebufferEXT");
		qglGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)qwglGetProcAddress("glGenRenderbuffersEXT");
		qglBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)qwglGetProcAddress("glBindRenderbufferEXT");
		qglFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)qwglGetProcAddress("glFramebufferRenderbufferEXT");
		qglCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)qwglGetProcAddress("glCheckFramebufferStatusEXT");
		qglRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)qwglGetProcAddress("glRenderbufferStorageEXT");
		qglFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)qwglGetProcAddress("glFramebufferTexture2DEXT");
		qglFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)qwglGetProcAddress("glFramebufferTexture3DEXT");
		qglFramebufferTextureLayerEXT = (PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC)qwglGetProcAddress("glFramebufferTextureLayerEXT");
		qglFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)qwglGetProcAddress("glFramebufferTexture");
		qglDrawBuffers = (PFNGLDRAWBUFFERSPROC)qwglGetProcAddress("glDrawBuffers");

		gl_framebuffer_object = true;
	}

	if (strstr(extension, "GL_EXT_framebuffer_multisample"))
	{
		qglRenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)qwglGetProcAddress("glRenderbufferStorageMultisampleEXT");
	
		gl_msaa_support = true;
	}

	if (strstr(extension, "GL_EXT_framebuffer_blit"))
	{
		qglBlitFramebufferEXT = (PFNGLBLITFRAMEBUFFEREXTPROC)qwglGetProcAddress("glBlitFramebufferEXT");

		gl_blit_support = true;
	}

	if( strstr(extension, "GL_ARB_texture_float") || strstr(extension, "GL_NV_float_buffer") || strstr(extension, "GL_ATI_texture_float"))
	{
		gl_float_buffer_support = true;;
	}

	if (strstr(extension, "GL_EXT_texture_compression_s3tc"))
	{
		gl_s3tc_compression_support = true;
	}
}