#include "gl_local.h"

typedef struct
{
	GLboolean polygon_offset_fill;
	GLboolean cullface;
	GLboolean alphatest;
	GLboolean depthtest;
	GLboolean depthmask;
	GLboolean blend;
	int blendsrc;
	int blenddst;
	qboolean mtex;
}gl_draw_context;

vec3_t save_vieworg[MAX_SAVESTACK];

vec3_t save_viewang[MAX_SAVESTACK];

int save_refdef_stack = 0;

gl_draw_context save_drawcontext[MAX_SAVESTACK];

int save_drawcontext_stack = 0;

GLint save_readframebuffer = 0;

GLint save_drawframebuffer = 0;

bool GL_CanUseMSAAFrameBuffer(void)
{
	return s_MSAAFBO.s_hBackBufferFBO;
}

bool GL_CanUseBackFrameBuffer(void)
{
	return s_BackBufferFBO.s_hBackBufferFBO;
}

void GL_PushFrameBuffer(void)
{
	if(gl_framebuffer_object && GL_CanUseBackFrameBuffer())
	{
		qglGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &save_readframebuffer);
		qglGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &save_drawframebuffer);
	}
}

void GL_PopFrameBuffer(void)
{
	if(gl_framebuffer_object && GL_CanUseBackFrameBuffer())
	{
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, save_readframebuffer);
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, save_drawframebuffer);
	}
}

void GL_PushMatrix(void)
{
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
}

void GL_PopMatrix(void)
{
	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();

	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();
}

void GL_PushDrawState(void)
{
	if (save_drawcontext_stack == MAX_SAVESTACK)
	{
		Sys_ErrorEx("GL_PushDrawState: MAX_SAVESTACK exceed");
		return;
	}
	qglGetBooleanv(GL_POLYGON_OFFSET_FILL, &save_drawcontext[save_drawcontext_stack].polygon_offset_fill);
	qglGetBooleanv(GL_CULL_FACE, &save_drawcontext[save_drawcontext_stack].cullface);
	qglGetBooleanv(GL_ALPHA_TEST, &save_drawcontext[save_drawcontext_stack].alphatest);
	qglGetBooleanv(GL_DEPTH_TEST, &save_drawcontext[save_drawcontext_stack].depthtest);
	qglGetBooleanv(GL_DEPTH_WRITEMASK, &save_drawcontext[save_drawcontext_stack].depthmask);
	qglGetBooleanv(GL_BLEND, &save_drawcontext[save_drawcontext_stack].blend);
	if (save_drawcontext[save_drawcontext_stack].blend)
	{
		qglGetIntegerv(GL_BLEND_SRC, &save_drawcontext[save_drawcontext_stack].blendsrc);
		qglGetIntegerv(GL_BLEND_DST, &save_drawcontext[save_drawcontext_stack].blenddst);
	}
	save_drawcontext[save_drawcontext_stack].mtex = *mtexenabled;

	++save_drawcontext_stack;
}

void GL_PopDrawState(void)
{
	if (save_drawcontext_stack == 0)
	{
		Sys_ErrorEx("GL_PopDrawState: no drawcontext saved");
		return;
	}
	--save_drawcontext_stack;

	if(save_drawcontext[save_drawcontext_stack].polygon_offset_fill)
		qglEnable(GL_POLYGON_OFFSET_FILL);
	else
		qglDisable(GL_POLYGON_OFFSET_FILL);

	if (save_drawcontext[save_drawcontext_stack].cullface)
		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

	if(save_drawcontext[save_drawcontext_stack].alphatest)
		qglEnable(GL_ALPHA_TEST);
	else
		qglDisable(GL_ALPHA_TEST);

	if (save_drawcontext[save_drawcontext_stack].depthtest)
		qglEnable(GL_DEPTH_TEST);
	else
		qglDisable(GL_DEPTH_TEST);

	if (save_drawcontext[save_drawcontext_stack].depthmask)
		qglDepthMask(GL_TRUE);
	else
		qglDepthMask(GL_FALSE);

	if (save_drawcontext[save_drawcontext_stack].blend)
		qglEnable(GL_BLEND);
	else
		qglDisable(GL_BLEND);

	if (save_drawcontext[save_drawcontext_stack].blend)
	{
		qglBlendFunc(save_drawcontext[save_drawcontext_stack].blendsrc, save_drawcontext[save_drawcontext_stack].blenddst);
	}

	if (save_drawcontext[save_drawcontext_stack].mtex && !(*mtexenabled))
		GL_EnableMultitexture();
	else if (!save_drawcontext[save_drawcontext_stack].mtex && (*mtexenabled))
		GL_DisableMultitexture();
}

refdef_t *R_GetRefDef(void)
{
	return r_refdef;
}

void R_PushRefDef(void)
{
	if (save_refdef_stack == MAX_SAVESTACK)
	{
		Sys_ErrorEx("R_PushRefDef: MAX_SAVESTACK exceed");
		return;
	}
	VectorCopy(r_refdef->vieworg, save_vieworg[save_refdef_stack]);
	VectorCopy(r_refdef->viewangles, save_viewang[save_refdef_stack]);
	++save_refdef_stack;
}

void R_UpdateRefDef(void)
{
	VectorCopy(r_refdef->vieworg, r_origin);
	AngleVectors(r_refdef->viewangles, vpn, vright, vup);
}

void R_PopRefDef(void)
{
	if (save_refdef_stack == 0)
	{
		Sys_ErrorEx("R_PopRefDef: no refdef saved");
		return;
	}
	--save_refdef_stack;
	VectorCopy(save_vieworg[save_refdef_stack], r_refdef->vieworg);
	VectorCopy(save_viewang[save_refdef_stack], r_refdef->viewangles);
}

void GL_UploadTextureColorFormat(int texid, int w, int h, int iInternalFormat)
{
	qglBindTexture(GL_TEXTURE_2D, texid);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//glTexStorage2D doesnt work with qglCopyTexImage2D so we use glTexImage2D here
	qglTexImage2D(GL_TEXTURE_2D, 0, iInternalFormat, w, h, 0, GL_RGBA,
		(iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, 0);

	qglBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenTextureColorFormat(int w, int h, int iInternalFormat)
{
	GLuint texid = GL_GenTexture();
	GL_UploadTextureColorFormat(texid, w, h, iInternalFormat);
	return texid;
}

GLuint GL_GenTextureRGBA8(int w, int h)
{
	return GL_GenTextureColorFormat(w, h, GL_RGBA8);
}

void GL_UploadDepthTexture(int texId, int w, int h)
{
	qglBindTexture(GL_TEXTURE_2D, texId);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexStorage2D doesnt work with qglCopyTexImage2D so we use glTexImage2D here
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenDepthTexture(int w, int h)
{
	GLuint texid = GL_GenTexture();
	GL_UploadDepthTexture(texid, w, h);
	return texid;
}

void GL_UploadShadowTexture(int texid, int w, int h)
{
	qglBindTexture(GL_TEXTURE_2D, texid);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	qglTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
}

GLuint GL_GenShadowTexture(int w, int h)
{
	GLuint texid = GL_GenTexture();
	GL_UploadShadowTexture(texid, w, h);
	return texid;
}

void GL_FreeTexture(gltexture_t *glt)
{
	GLuint texnum;

	texnum = glt->texnum;
	qglDeleteTextures(1, &texnum);

	memset(glt, 0, sizeof(*glt));
	glt->servercount = -1;
}

void R_InitTextures(void)
{
}

void R_FreeTextures(void)
{
}