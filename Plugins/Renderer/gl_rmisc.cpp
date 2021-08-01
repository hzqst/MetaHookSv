#include "gl_local.h"

#define MAX_SAVESTACK 16

vec3_t save_vieworg[MAX_SAVESTACK];

vec3_t save_viewang[MAX_SAVESTACK];

int save_refdef_stack = 0;

gl_draw_context save_drawcontext[MAX_SAVESTACK];

int save_drawcontext_stack = 0;

GLint save_readframebuffer = 0;

GLint save_drawframebuffer = 0;

void GL_PushFrameBuffer(void)
{
	qglGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &save_readframebuffer);
	qglGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &save_drawframebuffer);
}

void GL_PopFrameBuffer(void)
{
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, save_readframebuffer);
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, save_drawframebuffer);
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
	//qglGetBooleanv(GL_POLYGON_OFFSET_FILL, &save_drawcontext[save_drawcontext_stack].polygon_offset_fill);
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

	if (save_drawcontext[save_drawcontext_stack].mtex && !(*mtexenabled))
		GL_EnableMultitexture();
	else if (!save_drawcontext[save_drawcontext_stack].mtex && (*mtexenabled))
		GL_DisableMultitexture();

	/*if(save_drawcontext[save_drawcontext_stack].polygon_offset_fill)
		qglEnable(GL_POLYGON_OFFSET_FILL);
	else
		qglDisable(GL_POLYGON_OFFSET_FILL);*/

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

void GL_UploadTextureArrayColorFormat(int texid, int w, int h, int levels, int iInternalFormat)
{
	qglBindTexture(GL_TEXTURE_2D_ARRAY, texid);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage3D(GL_TEXTURE_2D_ARRAY, 0, iInternalFormat, w, h, levels, 0, GL_RGBA, 
		(iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, NULL);
	qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

GLuint GL_GenTextureArrayColorFormat(int w, int h, int levels, int iInternalFormat)
{
	GLuint texid = GL_GenTexture();
	GL_UploadTextureArrayColorFormat(texid, w, h, levels, iInternalFormat);
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

	//qglTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);???

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

	//qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	//qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	qglTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

	qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
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

void GL_GenFrameBuffer(FBO_Container_t *s)
{
	qglGenFramebuffersEXT(1, &s->s_hBackBufferFBO);
	qglBindFramebufferEXT(GL_FRAMEBUFFER, s->s_hBackBufferFBO);
}

void GL_GenRenderBuffer(FBO_Container_t *s, int type)
{
	if (type == 1)
	{
		qglGenRenderbuffersEXT(1, &s->s_hBackBufferDB);
		qglBindRenderbufferEXT(GL_RENDERBUFFER, s->s_hBackBufferDB);
	}
	else
	{
		qglGenRenderbuffersEXT(1, &s->s_hBackBufferCB);
		qglBindRenderbufferEXT(GL_RENDERBUFFER, s->s_hBackBufferCB);
	}
}

void GL_RenderBufferStorage(FBO_Container_t *s, int type, GLuint iInternalFormat, qboolean multisample)
{
	if (multisample)
	{
		qglRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, gl_msaa_samples, iInternalFormat, s->iWidth, s->iHeight);
	}
	else
	{
		qglRenderbufferStorageEXT(GL_RENDERBUFFER, iInternalFormat, s->iWidth, s->iHeight);
	}

	if (type == 2)
		qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, s->s_hBackBufferDB);
	else if (type == 1)
		qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s->s_hBackBufferDB);
	else if (type == 0)
		qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, s->s_hBackBufferCB);
}

void GL_FrameBufferColorTexture(FBO_Container_t *s, GLuint iInternalFormat, qboolean multisample)
{
	int tex2D = multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

	s->s_hBackBufferTex = GL_GenTexture();
	qglBindTexture(tex2D, s->s_hBackBufferTex);
	qglTexParameteri(tex2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(tex2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(tex2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(tex2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	if (multisample)
	{
		qglTexStorage2DMultisample(tex2D, gl_msaa_samples, iInternalFormat, s->iWidth, s->iHeight, GL_FALSE);
	}
	else
	{
		if (iInternalFormat == GL_R32F)
		{
			qglTexStorage2D(tex2D, 1, iInternalFormat, s->iWidth, s->iHeight);
		}
		else
		{
			qglTexImage2D(tex2D, 0, iInternalFormat, s->iWidth, s->iHeight, 0, GL_RGBA,
				(iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, 0);
		}
	}
	s->iTextureColorFormat = iInternalFormat;

	qglFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s->s_hBackBufferTex, 0);
	qglBindTexture(tex2D, 0);
}

void GL_FrameBufferDepthTexture(FBO_Container_t *s, GLuint iInternalFormat, qboolean multisample)
{
	int tex2D = multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

	s->s_hBackBufferDepthTex = GL_GenTexture();
	qglBindTexture(tex2D, s->s_hBackBufferDepthTex);
	qglTexParameteri(tex2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(tex2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(tex2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(tex2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if (multisample)
	{
		qglTexStorage2DMultisample(tex2D, gl_msaa_samples, iInternalFormat, s->iWidth, s->iHeight, GL_FALSE);
	}
	else
	{
		if (iInternalFormat == GL_DEPTH24_STENCIL8 || iInternalFormat == GL_DEPTH24_STENCIL8_EXT)
		{
			qglTexStorage2D(tex2D, 1, iInternalFormat, s->iWidth, s->iHeight);

			//qglTexImage2D(tex2D, 0, iInternalFormat, s->iWidth, s->iHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
		}
		else
		{
			if (iInternalFormat == GL_RGBA || iInternalFormat == GL_RGBA8)
				qglTexImage2D(tex2D, 0, iInternalFormat, s->iWidth, s->iHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
			else
				qglTexImage2D(tex2D, 0, iInternalFormat, s->iWidth, s->iHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		}
	}

	if (iInternalFormat == GL_DEPTH24_STENCIL8 || iInternalFormat == GL_DEPTH24_STENCIL8_EXT)
	{
		qglFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, s->s_hBackBufferDepthTex, 0);
	}
	else
	{
		qglFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s->s_hBackBufferDepthTex, 0);
	}

	qglBindTexture(tex2D, 0);

	if (iInternalFormat == GL_DEPTH24_STENCIL8 || iInternalFormat == GL_DEPTH24_STENCIL8_EXT)
	{
		s->s_hBackBufferStencilView = GL_GenTexture();
		qglTextureView(s->s_hBackBufferStencilView, tex2D, s->s_hBackBufferDepthTex, GL_DEPTH24_STENCIL8, 0, 1, 0, 1);
		
		qglBindTexture(GL_TEXTURE_2D, s->s_hBackBufferStencilView);
		qglTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		qglBindTexture(GL_TEXTURE_2D, 0);
	}
}

int GL_GenColorTextureHBAO(int w, int h)
{
	GLint swizzle[4] = { GL_RED,GL_GREEN,GL_ZERO,GL_ZERO };

	int texId = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, texId);
	qglTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, w, h, 0, GL_RG, GL_FLOAT, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);

	return texId;
}

void GL_FrameBufferColorTextureHBAO(FBO_Container_t *s)
{
	GLint swizzle[4] = { GL_RED,GL_GREEN,GL_ZERO,GL_ZERO };

	s->s_hBackBufferTex = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex);
	qglTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, s->iWidth, s->iHeight);
	qglBindTexture(GL_TEXTURE_2D, 0);

	s->s_hBackBufferTex2 = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex2);
	qglTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, s->iWidth, s->iHeight);
	qglBindTexture(GL_TEXTURE_2D, 0);

	s->iTextureColorFormat = GL_RG16F;

	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s->s_hBackBufferTex, 0);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, s->s_hBackBufferTex2, 0);
}

void GL_FrameBufferColorTextureDeferred(FBO_Container_t *s, int iInternalColorFormat)
{
	s->s_hBackBufferTex = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D_ARRAY, s->s_hBackBufferTex);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexImage3D(GL_TEXTURE_2D_ARRAY, 0, iInternalColorFormat, s->iWidth, s->iHeight, 5, 0, GL_RGB, GL_FLOAT, NULL);
	qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	s->iTextureColorFormat = iInternalColorFormat;

	qglFramebufferTextureLayerEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s->s_hBackBufferTex, 0, GBUFFER_INDEX_DIFFUSE);
	qglFramebufferTextureLayerEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, s->s_hBackBufferTex, 0, GBUFFER_INDEX_LIGHTMAP);
	qglFramebufferTextureLayerEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, s->s_hBackBufferTex, 0, GBUFFER_INDEX_WORLD);
	qglFramebufferTextureLayerEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, s->s_hBackBufferTex, 0, GBUFFER_INDEX_NORMAL);
	qglFramebufferTextureLayerEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, s->s_hBackBufferTex, 0, GBUFFER_INDEX_ADDITIVE);
}

void GL_Begin2D(void)
{
	qglViewport(glx, gly, glwidth, glheight);

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0, glwidth, glheight, 0, -99999, 99999);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_ALPHA_TEST);
	qglDisable(GL_CULL_FACE);
}

void GL_Begin2DEx(int width, int height)
{
	qglViewport(glx, gly, width, height);

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0, width, height, 0, -99999, 99999);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();
}

void GL_End2D(void)
{
	qglViewport(r_viewport[0], r_viewport[1], r_viewport[2], r_viewport[3]);

	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf(r_projection_matrix);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadMatrixf(r_world_matrix);

	qglEnable(GL_DEPTH_TEST);
	qglEnable(GL_CULL_FACE);
}

void COM_FileBase(const char *in, char *out)
{
	int len, start, end;

	len = strlen(in);

	// scan backward for '.'
	end = len - 1;
	while (end && in[end] != '.' && in[end] != '/' && in[end] != '\\')
		end--;

	if (in[end] != '.')		// no '.', copy to end
		end = len - 1;
	else
		end--;					// Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len - 1;
	while (start >= 0 && in[start] != '/' && in[start] != '\\')
		start--;

	if (start < 0 || (in[start] != '/' && in[start] != '\\'))
		start = 0;
	else
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy(out, &in[start], len);
	out[len] = 0;
}