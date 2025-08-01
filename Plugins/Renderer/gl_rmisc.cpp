#include "gl_local.h"

#define MAX_SAVESTACK 16

vec3_t save_vieworg[MAX_SAVESTACK] = { 0 };
vec3_t save_viewang[MAX_SAVESTACK] = { 0 };
int save_refdef_stack = 0;

typedef struct
{
	GLboolean cullface;
	GLboolean alphatest;
	GLboolean depthtest;
	GLboolean depthmask;
	GLboolean blend;
	int blendsrc;
	int blenddst;
	qboolean mtex;
}gl_draw_context;

gl_draw_context save_drawcontext[MAX_SAVESTACK] = {0};
int save_drawcontext_stack = 0;

GLint save_readframebuffer[MAX_SAVESTACK] = { 0 };
GLint save_drawframebuffer[MAX_SAVESTACK] = { 0 };
int save_framebuffer_stack = 0;

void GL_SetCurrentSceneFBO(FBO_Container_t* src)
{
	g_CurrentSceneFBO = src;
}

FBO_Container_t* GL_GetCurrentSceneFBO()
{
	return g_CurrentSceneFBO;
}

FBO_Container_t* GL_GetCurrentRenderingFBO()
{
	return g_CurrentRenderingFBO;
}

void GL_BindFrameBuffer(FBO_Container_t *fbo)
{
	g_CurrentRenderingFBO = fbo;

	if (fbo)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo->s_hBackBufferFBO);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void GL_BindFrameBufferWithTextures(FBO_Container_t *fbo, GLuint color, GLuint depth, GLuint depth_stencil, GLsizei width, GLsizei height)
{
	GL_BindFrameBuffer(fbo);

	if (color)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color, 0);
	}
	else
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0);
	}

	if (depth_stencil && !depth)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 0, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depth_stencil, 0);
	}
	else if (depth && !depth_stencil)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth, 0);
	}
	else if (!depth && !depth_stencil)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 0, 0);
	}
	else
	{
		g_pMetaHookAPI->SysError("GL_BindFrameBufferWithTextures: GL_DEPTH_STENCIL_ATTACHMENT and GL_DEPTH_ATTACHMENT can not be used together!");
	}

	if (width && height)
	{
		fbo->iWidth = width;
		fbo->iHeight = height;
	}
}

void GL_PushFrameBuffer(void)
{
	if (save_framebuffer_stack == MAX_SAVESTACK)
	{
		g_pMetaHookAPI->SysError("GL_PushFrameBuffer: MAX_SAVESTACK exceed");
		return;
	}
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &save_readframebuffer[save_framebuffer_stack]);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &save_drawframebuffer[save_framebuffer_stack]);
	save_framebuffer_stack++;
}

void GL_PopFrameBuffer(void)
{
	if (save_framebuffer_stack == 0)
	{
		g_pMetaHookAPI->SysError("GL_PopFrameBuffer: no framebuffer saved");
		return;
	}

	--save_framebuffer_stack;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, save_readframebuffer[save_framebuffer_stack]);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, save_drawframebuffer[save_framebuffer_stack]);
}

void GL_PushMatrix(void)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
}

void GL_PopMatrix(void)
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void GL_PushDrawState(void)
{
	if (save_drawcontext_stack == MAX_SAVESTACK)
	{
		g_pMetaHookAPI->SysError("GL_PushDrawState: MAX_SAVESTACK exceed");
		return;
	}
	glGetBooleanv(GL_CULL_FACE, &save_drawcontext[save_drawcontext_stack].cullface);
	glGetBooleanv(GL_ALPHA_TEST, &save_drawcontext[save_drawcontext_stack].alphatest);
	glGetBooleanv(GL_DEPTH_TEST, &save_drawcontext[save_drawcontext_stack].depthtest);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &save_drawcontext[save_drawcontext_stack].depthmask);
	glGetBooleanv(GL_BLEND, &save_drawcontext[save_drawcontext_stack].blend);
	if (save_drawcontext[save_drawcontext_stack].blend)
	{
		glGetIntegerv(GL_BLEND_SRC, &save_drawcontext[save_drawcontext_stack].blendsrc);
		glGetIntegerv(GL_BLEND_DST, &save_drawcontext[save_drawcontext_stack].blenddst);
	}

	save_drawcontext[save_drawcontext_stack].mtex = *mtexenabled;
	++save_drawcontext_stack;
}

void GL_PopDrawState(void)
{
	if (save_drawcontext_stack == 0)
	{
		g_pMetaHookAPI->SysError("GL_PopDrawState: no drawcontext saved");
		return;
	}

	--save_drawcontext_stack;

	if (save_drawcontext[save_drawcontext_stack].mtex && !(*mtexenabled))
		GL_EnableMultitexture();
	else if (!save_drawcontext[save_drawcontext_stack].mtex && (*mtexenabled))
		GL_DisableMultitexture();

	if (save_drawcontext[save_drawcontext_stack].cullface)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	if(save_drawcontext[save_drawcontext_stack].alphatest)
		glEnable(GL_ALPHA_TEST);
	else
		glDisable(GL_ALPHA_TEST);

	if (save_drawcontext[save_drawcontext_stack].depthtest)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	if (save_drawcontext[save_drawcontext_stack].depthmask)
		glDepthMask(GL_TRUE);
	else
		glDepthMask(GL_FALSE);

	if (save_drawcontext[save_drawcontext_stack].blend)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	if (save_drawcontext[save_drawcontext_stack].blend)
	{
		glBlendFunc(save_drawcontext[save_drawcontext_stack].blendsrc, save_drawcontext[save_drawcontext_stack].blenddst);
	}
}

void *R_GetRefDef(void)
{
	return r_refdef_SvEngine ? (void *)r_refdef_SvEngine : (void *)r_refdef_GoldSrc;
}

void R_PushRefDef(void)
{
	if (save_refdef_stack == MAX_SAVESTACK)
	{
		g_pMetaHookAPI->SysError("R_PushRefDef: MAX_SAVESTACK exceed");
		return;
	}
	VectorCopy((*r_refdef.vieworg), save_vieworg[save_refdef_stack]);
	VectorCopy((*r_refdef.viewangles), save_viewang[save_refdef_stack]);
	++save_refdef_stack;
}

/*
	Purpose: Update r_origin, vpn, vright and vup according to r_refdef stuffs
*/

void R_UpdateRefDef(void)
{
	VectorCopy((*r_refdef.vieworg), r_origin);
	AngleVectors((*r_refdef.viewangles), vpn, vright, vup);
}

void R_PopRefDef(void)
{
	if (save_refdef_stack == 0)
	{
		g_pMetaHookAPI->SysError("R_PopRefDef: no refdef saved");
		return;
	}
	--save_refdef_stack;
	VectorCopy(save_vieworg[save_refdef_stack], (*r_refdef.vieworg));
	VectorCopy(save_viewang[save_refdef_stack], (*r_refdef.viewangles));
}

void GL_UploadTextureColorFormat(int texid, int w, int h, int iInternalFormat, bool filter, float *borderColor)
{
	glBindTexture(GL_TEXTURE_2D, texid);

	if (borderColor)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);

	//glTexStorage2D doesnt work with qglCopyTexImage2D so we use glTexImage2D here
	glTexImage2D(GL_TEXTURE_2D, 0, iInternalFormat, w, h, 0, GL_RGBA,
		(iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenTextureColorFormat(int w, int h, int iInternalFormat, bool filter, float *borderColor)
{
	GLuint texid = GL_GenTexture();
	GL_UploadTextureColorFormat(texid, w, h, iInternalFormat, filter, borderColor);
	return texid;
}

void GL_UploadTextureArrayColorFormat(int texid, int w, int h, int levels, int iInternalFormat, bool filter, float *borderColor)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, texid);

	if (borderColor)
	{
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, iInternalFormat, w, h, levels, 0, GL_RGBA, 
		(iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

GLuint GL_GenTextureArrayColorFormat(int w, int h, int levels, int iInternalFormat, bool filter, float *borderColor)
{
	GLuint texid = GL_GenTexture();
	GL_UploadTextureArrayColorFormat(texid, w, h, levels, iInternalFormat, filter, borderColor);
	return texid;
}

GLuint GL_GenTextureRGBA8(int w, int h)
{
	return GL_GenTextureColorFormat(w, h, GL_RGBA8, true, NULL);
}

void GL_UploadDepthTexture(int texId, int w, int h)
{
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenDepthTexture(int w, int h)
{
	GLuint texid = GL_GenTexture();
	GL_UploadDepthTexture(texid, w, h);
	return texid;
}

void GL_UploadDepthStencilTexture(int texId, int w, int h)
{
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, w, h);
	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenDepthStencilTexture(int w, int h)
{
	GLuint texid = GL_GenTexture();
	GL_UploadDepthStencilTexture(texid, w, h);
	return texid;
}

GLuint GL_CreateDepthViewForDepthTexture(int texId)
{
	GLuint depthviewtexid = GL_GenTexture();
	glTextureView(depthviewtexid, GL_TEXTURE_2D, texId, GL_DEPTH24_STENCIL8, 0, 1, 0, 1);

	glBindTexture(GL_TEXTURE_2D, depthviewtexid);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	return depthviewtexid;
}

GLuint GL_CreateStencilViewForDepthTexture(int texId)
{
	GLuint stencilviewtexid = GL_GenTexture();
	glTextureView(stencilviewtexid, GL_TEXTURE_2D, texId, GL_DEPTH24_STENCIL8, 0, 1, 0, 1);

	glBindTexture(GL_TEXTURE_2D, stencilviewtexid);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	return stencilviewtexid;
}

void GL_UploadShadowTexture(int texid, int w, int h, float *borderColor)
{
	glBindTexture(GL_TEXTURE_2D, texid);

	if (borderColor)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, w, h);

	//glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH32F_STENCIL8, w, h);

	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenShadowTexture(int w, int h, float *borderColor)
{
	GLuint texid = GL_GenTexture();
	GL_UploadShadowTexture(texid, w, h, borderColor);
	return texid;
}

void GL_FreeTextureEntryInternal(gltexture_t* glt)
{
	GL_DeleteTexture(glt->texnum);
	memset(glt, 0, sizeof(*glt));
	glt->servercount = -1;

	//The texnum can be deleted before and will never be allocated again by engine in this situation.
	//We have to allocate a new texture manually to fix this issue.
	//Until we rewrite GL_LoadTexture2
#if 1
	if (!g_bHasOfficialGLTexAllocSupport)
	{
		glt->texnum = GL_GenTexture();
	}
#endif
	/*
		SvEngine remove the entire glt struct from CUtlVector, instead of simply zeroing the struct
	*/
#if 1
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		/*
			void __cdecl GL_UnloadTexture(char *a1)
			{
			  int gltindex; // esi
			  int i; // edi
			  const GLuint *v3; // ebx
			  int previous_numgltextures; // eax

			  gltindex = 0;
			  if ( numgltextures > 0 )
			  {
				for ( i = 0; ; i += 84 )
				{
				  v3 = (const GLuint *)((char *)gltextures + i);
				  if ( !strcmp(a1, (_BYTE *)gltextures + i + 20) )
					break;
				  if ( ++gltindex >= numgltextures )
					return;
				}
				glDeleteTextures(1, v3);
				previous_numgltextures = numgltextures;
				if ( numgltextures - gltindex - 1 > 0 )
				{
				  memmove(
					(char *)gltextures + 84 * gltindex,
					(char *)gltextures + 84 * gltindex + 84,
					84 * (numgltextures - gltindex - 1));
				  previous_numgltextures = numgltextures;
				}
				numgltextures = previous_numgltextures - 1;
			  }
			}
		*/

		int gltindex = glt - gltextures_get();
		if ((*numgltextures) - gltindex - 1 > 0)
		{
			memmove(
				glt,
				glt + 1,
				sizeof(gltexture_t) * ((*numgltextures) - gltindex - 1));
		}
		(*numgltextures)--;
	}
#endif
}

void GL_FreeTextureEntry(gltexture_t *glt)
{
	if (glt->texnum <= 0)
	{
		gEngfuncs.Con_DPrintf("GL_FreeTextureEntry: Bogus texture entry [%s] texid[%d] ?\n", glt->identifier, glt->texnum);
		return;
	}

	gEngfuncs.Con_DPrintf("GL_FreeTextureEntry: [%s] texid[%d], server[%d].\n", glt->identifier, glt->texnum, (int)glt->servercount);

	GL_FreeTextureEntryInternal(glt);
}

void GL_GenFrameBuffer(FBO_Container_t *s)
{
	glGenFramebuffers(1, &s->s_hBackBufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, s->s_hBackBufferFBO);
}

void GL_GenRenderBuffer(FBO_Container_t *s, int type)
{
	if (type == 1)
	{
		glGenRenderbuffers(1, &s->s_hBackBufferDB);
		glBindRenderbuffer(GL_RENDERBUFFER, s->s_hBackBufferDB);
	}
	else
	{
		glGenRenderbuffers(1, &s->s_hBackBufferCB);
		glBindRenderbuffer(GL_RENDERBUFFER, s->s_hBackBufferCB);
	}
}

void GL_RenderBufferStorage(FBO_Container_t *s, int type, GLuint iInternalFormat)
{
	glRenderbufferStorage(GL_RENDERBUFFER, iInternalFormat, s->iWidth, s->iHeight);

	if (type == 2)
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, s->s_hBackBufferDB);
	else if (type == 1)
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s->s_hBackBufferDB);
	else if (type == 0)
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, s->s_hBackBufferCB);
}

void GL_FrameBufferColorTexture(FBO_Container_t *s, GLuint iInternalFormat)
{
	int tex2D = GL_TEXTURE_2D;

	s->s_hBackBufferTex = GL_GenTexture();
	glBindTexture(tex2D, s->s_hBackBufferTex);
	glTexParameteri(tex2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(tex2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(tex2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(tex2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	if (iInternalFormat == GL_R32F)
	{
		glTexStorage2D(tex2D, 1, iInternalFormat, s->iWidth, s->iHeight);
	}
	else
	{
		glTexImage2D(tex2D, 0, iInternalFormat, s->iWidth, s->iHeight, 0, GL_RGBA,
			(iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, 0);
	}

	s->iTextureColorFormat = iInternalFormat;

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s->s_hBackBufferTex, 0);
	glBindTexture(tex2D, 0);
}

void GL_FrameBufferDepthTexture(FBO_Container_t *s, GLuint iInternalFormat)
{
	int tex2D = GL_TEXTURE_2D;

	s->s_hBackBufferDepthTex = GL_GenTexture();
	glBindTexture(tex2D, s->s_hBackBufferDepthTex);
	glTexParameteri(tex2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(tex2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(tex2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(tex2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	if (iInternalFormat == GL_DEPTH24_STENCIL8)
	{
		glTexStorage2D(tex2D, 1, iInternalFormat, s->iWidth, s->iHeight);
	}
	else
	{
		if (iInternalFormat == GL_RGBA || iInternalFormat == GL_RGBA8)
			glTexImage2D(tex2D, 0, iInternalFormat, s->iWidth, s->iHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
		else
			glTexImage2D(tex2D, 0, iInternalFormat, s->iWidth, s->iHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	}

	if (iInternalFormat == GL_DEPTH24_STENCIL8)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, s->s_hBackBufferDepthTex, 0);
	}
	else
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s->s_hBackBufferDepthTex, 0);
	}

	glBindTexture(tex2D, 0);

	s->iTextureDepthFormat = iInternalFormat;

	if (iInternalFormat == GL_DEPTH24_STENCIL8 && glTextureView)
	{
		s->s_hBackBufferStencilView = GL_GenTexture();
		glTextureView(s->s_hBackBufferStencilView, tex2D, s->s_hBackBufferDepthTex, GL_DEPTH24_STENCIL8, 0, 1, 0, 1);
		
		glBindTexture(GL_TEXTURE_2D, s->s_hBackBufferStencilView);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		//Texture view not supported!
		s->s_hBackBufferStencilView = 0;
	}
}

int GL_GenColorTextureHBAO(int w, int h)
{
	GLint swizzle[4] = { GL_RED,GL_GREEN,GL_ZERO,GL_ZERO };

	int texId = GL_GenTexture();
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, w, h, 0, GL_RG, GL_FLOAT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texId;
}

void GL_FrameBufferColorTextureHBAO(FBO_Container_t *s)
{
	GLint swizzle[4] = { GL_RED,GL_GREEN,GL_ZERO,GL_ZERO };

	s->s_hBackBufferTex = GL_GenTexture();
	glBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex);
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, s->iWidth, s->iHeight);
	glBindTexture(GL_TEXTURE_2D, 0);

	s->s_hBackBufferTex2 = GL_GenTexture();
	glBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex2);
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG16F, s->iWidth, s->iHeight);
	glBindTexture(GL_TEXTURE_2D, 0);

	s->iTextureColorFormat = GL_RG16F;

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s->s_hBackBufferTex, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, s->s_hBackBufferTex2, 0);
}

void GL_FrameBufferColorTextureDeferred(
	FBO_Container_t *s, 
	GLuint iInternalColorFormat, 
	GLuint iInternalColorFormat2,
	GLuint iInternalColorFormat3,
	GLuint iInternalColorFormat4)
{
	s->s_hBackBufferTex = GL_GenTexture();
	glBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, iInternalColorFormat, s->iWidth, s->iHeight);
	glBindTexture(GL_TEXTURE_2D, 0);

	s->s_hBackBufferTex2 = GL_GenTexture();
	glBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, iInternalColorFormat2, s->iWidth, s->iHeight);
	glBindTexture(GL_TEXTURE_2D, 0);

	s->s_hBackBufferTex3 = GL_GenTexture();
	glBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex3);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, iInternalColorFormat3, s->iWidth, s->iHeight);
	glBindTexture(GL_TEXTURE_2D, 0);

	s->s_hBackBufferTex4 = GL_GenTexture();
	glBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, iInternalColorFormat4, s->iWidth, s->iHeight);
	glBindTexture(GL_TEXTURE_2D, 0);

	s->iTextureColorFormat = iInternalColorFormat;

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s->s_hBackBufferTex, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, s->s_hBackBufferTex2, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, s->s_hBackBufferTex3, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, s->s_hBackBufferTex4, 0);
}

void GL_FrameBufferColorTextureOITBlend(FBO_Container_t *s)
{
	s->s_hBackBufferTex = GL_GenTexture();	
	glBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, s->iWidth, s->iHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	s->s_hBackBufferTex2 = GL_GenTexture();
	glBindTexture(GL_TEXTURE_2D, s->s_hBackBufferTex2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, s->iWidth, s->iHeight, 0, GL_RED, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	s->iTextureColorFormat = GL_RGBA16F;

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s->s_hBackBufferTex, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, s->s_hBackBufferTex2, 0);
}

void GL_BeginFullScreenQuad(bool enableDepthTest)
{
	if (enableDepthTest)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}
	glDisable(GL_CULL_FACE);
}

void GL_EndFullScreenQuad(void)
{
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}

void GL_Begin2D(void)
{
	glViewport(glx, gly, glwidth, glheight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, glwidth, glheight, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);
}

void GL_Begin2DEx(int width, int height)
{
	glViewport(glx, gly, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void GL_End2D(void)
{
	glViewport(r_viewport[0], r_viewport[1], r_viewport[2], r_viewport[3]);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(r_projection_matrix);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(r_world_matrix);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void GL_ClearColor(vec4_t color)
{
	glClearColor(color[0], color[1], color[2], color[3]);
	glClear(GL_COLOR_BUFFER_BIT);
}

void GL_ClearDepthStencil(float depth, int stencilref, int stencilmask)
{
	glStencilMask(stencilmask);
	glDepthMask(GL_TRUE);

	glClearStencil(stencilref);
	glClearDepth(depth);

	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glStencilMask(0);
}

void GL_ClearColorDepthStencil(vec4_t color, float depth, int stencilref, int stencilmask)
{
	glStencilMask(stencilmask);
	glDepthMask(GL_TRUE);

	glClearColor(color[0], color[1], color[2], color[3]);
	glClearStencil(stencilref);
	glClearDepth(depth);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glStencilMask(0);
}

void GL_ClearStencil(int mask)
{
	glStencilMask(mask);
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilMask(0);
}

void GL_BeginStencilCompareEqual(int ref, int mask)
{
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_EQUAL, ref, mask);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void GL_BeginStencilCompareNotEqual(int ref, int mask)
{
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, ref, mask);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void GL_BeginStencilWrite(int ref, int write_mask)
{
	glEnable(GL_STENCIL_TEST);
	glStencilMask(write_mask);
	glStencilFunc(GL_ALWAYS, ref, write_mask);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void GL_BeginStencilCompareEqualWrite(int ref, int compare_mask, int write_mask)
{
	glEnable(GL_STENCIL_TEST);
	glStencilMask(write_mask);
	glStencilFunc(GL_EQUAL, ref, compare_mask);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void GL_BeginStencilCompareNotEqualWrite(int ref, int compare_mask, int write_mask)
{
	glEnable(GL_STENCIL_TEST);
	glStencilMask(write_mask);
	glStencilFunc(GL_NOTEQUAL, ref, compare_mask);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void GL_EndStencil()
{
	glStencilMask(0);
	glDisable(GL_STENCIL_TEST);
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

void GL_ClearFBO(FBO_Container_t* s)
{
	s->s_hBackBufferFBO = 0;
	s->s_hBackBufferCB = 0;
	s->s_hBackBufferDB = 0;
	s->s_hBackBufferTex = 0;
	s->s_hBackBufferTex2 = 0;
	s->s_hBackBufferDepthTex = 0;
	s->iWidth = s->iHeight = s->iTextureColorFormat = 0;
}

void GL_FreeFBO(FBO_Container_t* s)
{
	if (s->s_hBackBufferFBO)
		glDeleteFramebuffers(1, &s->s_hBackBufferFBO);

	if (s->s_hBackBufferCB)
		glDeleteRenderbuffers(1, &s->s_hBackBufferCB);

	if (s->s_hBackBufferDB)
		glDeleteRenderbuffers(1, &s->s_hBackBufferDB);

	if (s->s_hBackBufferTex)
		glDeleteTextures(1, &s->s_hBackBufferTex);

	if (s->s_hBackBufferTex2)
		glDeleteTextures(1, &s->s_hBackBufferTex2);

	if (s->s_hBackBufferTex3)
		glDeleteTextures(1, &s->s_hBackBufferTex3);

	if (s->s_hBackBufferTex4)
		glDeleteTextures(1, &s->s_hBackBufferTex4);

	if (s->s_hBackBufferDepthTex)
		glDeleteTextures(1, &s->s_hBackBufferDepthTex);

	if (s->s_hBackBufferStencilView)
		glDeleteTextures(1, &s->s_hBackBufferStencilView);

	GL_ClearFBO(s);
}

float GetFrameRateFromFrameDuration(int frameduration)
{
	if (frameduration > 0)
		return 1000.0f / frameduration;

	return 0;
}