#include "gl_local.h"

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
	Sys_Error("NOT AVAILABLE");
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
}

void GL_PopMatrix(void)
{
	Sys_Error("NOT AVAILABLE");
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

void GL_CreateTextureColorFormat(int texid, int w, int h, int iInternalFormat, bool filter, float *borderColor, bool immutable)
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

	if (immutable)
	{
		glTexStorage2D(GL_TEXTURE_2D, 1, iInternalFormat, w, h);
	}
	else
	{
		//glTexStorage2D doesnt work with qglCopyTexImage2D so we use glTexImage2D here
		glTexImage2D(GL_TEXTURE_2D, 0, iInternalFormat, w, h, 0, GL_RGBA, (iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, 0);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenTextureColorFormat(int w, int h, int iInternalFormat, bool filter, float *borderColor, bool immutable)
{
	GLuint texid = GL_GenTexture();
	GL_CreateTextureColorFormat(texid, w, h, iInternalFormat, filter, borderColor, immutable);
	return texid;
}

void GL_CreateTextureArrayColorFormat(int texid, int w, int h, int levels, int iInternalFormat, bool filter, float *borderColor, bool immutable)
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
	
	if (immutable)
	{
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, iInternalFormat, w, h, levels);
	}
	else
	{
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, iInternalFormat, w, h, levels, 0, GL_RGBA, (iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, NULL);
	}

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

GLuint GL_GenTextureArrayColorFormat(int w, int h, int depth, int iInternalFormat, bool filter, float *borderColor, bool immutable)
{
	GLuint texid = GL_GenTexture();
	GL_CreateTextureArrayColorFormat(texid, w, h, depth, iInternalFormat, filter, borderColor, immutable);
	return texid;
}

GLuint GL_GenTextureRGBA8(int w, int h, bool immutable)
{
	return GL_GenTextureColorFormat(w, h, GL_RGBA8, true, nullptr, immutable);
}

void GL_CreateDepthTexture(int texId, int w, int h, bool immutable)
{
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	if (immutable)
	{
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, w, h);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenDepthTexture(int w, int h, bool immutable)
{
	GLuint texid = GL_GenTexture();
	GL_CreateDepthTexture(texid, w, h, immutable);
	return texid;
}

void GL_CreateDepthStencilTexture(int texId, int w, int h, bool immutable)
{
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	if (immutable)
	{
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, w, h);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, w, h, 0, GL_DEPTH_STENCIL, GL_FLOAT, 0);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenDepthStencilTexture(int w, int h, bool immutable)
{
	GLuint texid = GL_GenTexture();
	GL_CreateDepthStencilTexture(texid, w, h, immutable);
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

void GL_CreateShadowTexture(int texid, int w, int h, float *borderColor, bool immutable)
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

	//GL_DEPTH_TEXTURE_MODE has nothing to do with rendering to a depth texture. That's old fixed-function stuff from desktop OpenGL 2.1.
	//glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	if (immutable)
	{
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH32F_STENCIL8, w, h);
		//glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, w, h);
	}
	else
	{
		
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenShadowTexture(int w, int h, float *borderColor, bool immutable)
{
	GLuint texid = GL_GenTexture();
	GL_CreateShadowTexture(texid, w, h, borderColor, immutable);
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

void GL_GenFrameBuffer(FBO_Container_t *s, const char *szFrameBufferName)
{
	glGenFramebuffers(1, &s->s_hBackBufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, s->s_hBackBufferFBO);

	strncpy(s->szFrameBufferName, szFrameBufferName, sizeof(s->szFrameBufferName) - 1);
	s->szFrameBufferName[sizeof(s->szFrameBufferName) - 1] = 0;

	GL_SetFrameBufferDebugName(s->s_hBackBufferFBO, szFrameBufferName);
}

const char* GL_GetFrameBufferName(FBO_Container_t* s)
{
	return s->szFrameBufferName;
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

	GL_SetTextureDebugNameFormat(s->s_hBackBufferTex, "%s - s_hBackBufferTex", GL_GetFrameBufferName(s));
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

	if (iInternalFormat == GL_DEPTH24_STENCIL8 || iInternalFormat == GL_DEPTH32F_STENCIL8)
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

	if (iInternalFormat == GL_DEPTH24_STENCIL8 || iInternalFormat == GL_DEPTH32F_STENCIL8)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, s->s_hBackBufferDepthTex, 0);
	}
	else
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s->s_hBackBufferDepthTex, 0);
	}

	glBindTexture(tex2D, 0);

	GL_SetTextureDebugNameFormat(s->s_hBackBufferDepthTex, "%s - s_hBackBufferDepthTex", GL_GetFrameBufferName(s));

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

		GL_SetTextureDebugNameFormat(s->s_hBackBufferStencilView, "%s - s_hBackBufferStencilView", GL_GetFrameBufferName(s));
	}
	else if (iInternalFormat == GL_DEPTH32F_STENCIL8 && glTextureView)
	{
		s->s_hBackBufferStencilView = GL_GenTexture();
		glTextureView(s->s_hBackBufferStencilView, tex2D, s->s_hBackBufferDepthTex, GL_DEPTH32F_STENCIL8, 0, 1, 0, 1);

		glBindTexture(GL_TEXTURE_2D, s->s_hBackBufferStencilView);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_SetTextureDebugNameFormat(s->s_hBackBufferStencilView, "%s - s_hBackBufferStencilView", GL_GetFrameBufferName(s));
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

void GL_BindTextureUnit(int textureUnit, int target, int gltexturenum)
{
	// Texture unit 0 = GBuffer diffuse array
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(target, gltexturenum);
	glActiveTexture(GL_TEXTURE0 + 0);
}

void GL_ClearColor(vec4_t color)
{
	GL_BeginDebugGroup("GL_ClearColor");

	glClearColor(color[0], color[1], color[2], color[3]);
	glClear(GL_COLOR_BUFFER_BIT);

	GL_EndDebugGroup();
}

void GL_ClearDepth(float depth)
{
	GL_BeginDebugGroup("GL_ClearDepthStencil");

	glDepthMask(GL_TRUE);

	glClearDepth(depth);

	glClear(GL_DEPTH_BUFFER_BIT);

	GL_EndDebugGroup();
}

void GL_ClearDepthStencil(float depth, int stencilref, int stencilmask)
{
	GL_BeginDebugGroup("GL_ClearDepthStencil");

	glStencilMask(stencilmask);
	glDepthMask(GL_TRUE);

	glClearStencil(stencilref);
	glClearDepth(depth);

	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glStencilMask(0);

	GL_EndDebugGroup();
}

void GL_ClearColorDepthStencil(vec4_t color, float depth, int stencilref, int stencilmask)
{
	GL_BeginDebugGroup("GL_ClearColorDepthStencil");

	glStencilMask(stencilmask);
	glDepthMask(GL_TRUE);

	glClearColor(color[0], color[1], color[2], color[3]);
	glClearStencil(stencilref);
	glClearDepth(depth);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glStencilMask(0);

	GL_EndDebugGroup();
}

void GL_ClearStencil(int mask)
{
	GL_BeginDebugGroup("GL_ClearStencil");

	glStencilMask(mask);
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilMask(0);

	GL_EndDebugGroup();
}

void GL_BeginStencilCompareEqual(int ref, int mask)
{
	GL_BeginDebugGroup("GL_BeginStencilCompareEqual");

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_EQUAL, ref, mask);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_EndDebugGroup();
}

void GL_BeginStencilCompareNotEqual(int ref, int mask)
{
	GL_BeginDebugGroup("GL_BeginStencilCompareNotEqual");

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, ref, mask);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_EndDebugGroup();
}

void GL_BeginStencilWrite(int ref, int write_mask)
{
	GL_BeginDebugGroup("GL_BeginStencilWrite");

	glEnable(GL_STENCIL_TEST);
	glStencilMask(write_mask);
	glStencilFunc(GL_ALWAYS, ref, write_mask);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	GL_EndDebugGroup();
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

static mat4 r_pushed_world_matrix[MAX_SAVESTACK];
static size_t r_pushed_world_matrix_count{};

float* R_GetWorldMatrix()
{
	return r_world_matrix;
}

void R_PushWorldMatrix()
{
	if (r_pushed_world_matrix_count == MAX_SAVESTACK)
	{
		Sys_Error("R_PushWorldMatrix: max stack excceeded!");
		return;
	}

	memcpy(r_pushed_world_matrix[r_pushed_world_matrix_count], r_world_matrix, sizeof(mat4));
	r_pushed_world_matrix_count++;
}

void R_PopWorldMatrix()
{
	if (r_pushed_world_matrix_count == 0)
	{
		Sys_Error("R_PopWorldMatrix: empty r_pushed_world_matrix!");
		return;
	}

	auto& matrix = r_pushed_world_matrix[r_pushed_world_matrix_count - 1];

	memcpy(r_world_matrix, matrix, sizeof(mat4));

	r_pushed_world_matrix_count--;
}

void R_LoadIdentityForWorldMatrix()
{
	const float r_identity_matrix[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f}
	};

	memcpy(r_world_matrix, r_identity_matrix, sizeof(r_identity_matrix));
}

void R_RotateWorldMatrix(float angle, float x, float y, float z)
{
	// Apply rotate translation to "r_world_matrix" like glRotatef
	
	// Convert angle from degrees to radians
	float radian = (angle * M_PI) / 180.0f;
	
	// Normalize the axis vector
	float len = sqrtf(x * x + y * y + z * z);
	if (len == 0.0f)
		return;
	
	x /= len;
	y /= len;
	z /= len;
	
	// Calculate sin and cos
	float s, c;
	SinCos(radian, &s, &c);
	float one_minus_c = 1.0f - c;
	
	// Create rotation matrix using Rodrigues' rotation formula
	// This is equivalent to glRotatef implementation
	float rotation_matrix[16] = {
		// Column 0
		c + x * x * one_minus_c,
		y * x * one_minus_c + z * s,
		z * x * one_minus_c - y * s,
		0.0f,
		
		// Column 1
		x * y * one_minus_c - z * s,
		c + y * y * one_minus_c,
		z * y * one_minus_c + x * s,
		0.0f,
		
		// Column 2
		x * z * one_minus_c + y * s,
		y * z * one_minus_c - x * s,
		c + z * z * one_minus_c,
		0.0f,
		
		// Column 3
		0.0f,
		0.0f,
		0.0f,
		1.0f
	};
	
	// Multiply current world matrix with rotation matrix
	// result = r_world_matrix * rotation_matrix
	float result[16];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result[j * 4 + i] = 0.0f;
			for (int k = 0; k < 4; k++) {
				result[j * 4 + i] += r_world_matrix[k * 4 + i] * rotation_matrix[j * 4 + k];
			}
		}
	}
	
	// Copy result back to world matrix
	memcpy(r_world_matrix, result, sizeof(float) * 16);
}

void R_TranslateWorldMatrix(float x, float y, float z)
{
	// Apply translation to the world matrix (equivalent to glTranslatef)
	// In a 4x4 matrix, translation values go in the last column (indices 12, 13, 14)
	r_world_matrix[12] += r_world_matrix[0] * x + r_world_matrix[4] * y + r_world_matrix[8] * z;
	r_world_matrix[13] += r_world_matrix[1] * x + r_world_matrix[5] * y + r_world_matrix[9] * z;
	r_world_matrix[14] += r_world_matrix[2] * x + r_world_matrix[6] * y + r_world_matrix[10] * z;
}

void R_SetupPlayerViewWorldMatrix(const vec3_t origin, const vec3_t viewangles)
{
	R_RotateWorldMatrix(-90, 1, 0, 0);
	R_RotateWorldMatrix(90, 0, 0, 1);
	R_RotateWorldMatrix(-viewangles[2], 1, 0, 0); // roll
	R_RotateWorldMatrix(-viewangles[0], 0, 1, 0); // pitch
	R_RotateWorldMatrix(-viewangles[1], 0, 0, 1); // yaw
	R_TranslateWorldMatrix(-origin[0], -origin[1], -origin[2]);
}

static mat4 r_pushed_projection_matrix[MAX_SAVESTACK];
static size_t r_pushed_projection_matrix_count{};

void R_PushProjectionMatrix()
{
	if (r_pushed_projection_matrix_count == MAX_SAVESTACK)
	{
		Sys_Error("R_PushProjectionMatrix: max stack excceeded!");
		return;
	}
	memcpy(r_pushed_projection_matrix[r_pushed_projection_matrix_count], r_projection_matrix, sizeof(mat4));
	r_pushed_projection_matrix_count++;
}

void R_PopProjectionMatrix()
{
	if (r_pushed_projection_matrix_count == 0)
	{
		Sys_Error("R_PopProjectionMatrix: empty r_pushed_projection_matrix!");
		return;
	}

	auto& matrix = r_pushed_projection_matrix[r_pushed_projection_matrix_count - 1];

	memcpy(r_projection_matrix, matrix, sizeof(mat4));

	r_pushed_projection_matrix_count--;
}

float* R_GetProjectionMatrix()
{
	return r_projection_matrix;
}

void R_LoadIdentityForProjectionMatrix()
{
	const float r_identity_matrix[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f}
	};

	memcpy(r_projection_matrix, r_identity_matrix, sizeof(r_identity_matrix));
}

/*

	// All graphics APIs except for OpenGL use [0, 1] as NDC Z range.
	// OpenGL uses [-1, 1] unless glClipControl is used to change it.
	// Use IRenderDevice::GetDeviceInfo().NDC to get the NDC Z range.
	// See https://github.com/DiligentGraphics/DiligentCore/blob/master/doc/CoordinateSystem.md
	void SetNearFarClipPlanes(T zNear, T zFar, bool NegativeOneToOneZ)
	{
		if (_44 == 0)
		{
			// Perspective projection
			if (NegativeOneToOneZ)
			{
				// https://www.opengl.org/sdk/docs/man2/xhtml/gluPerspective.xml
				// http://www.terathon.com/gdc07_lengyel.pdf
				// Note that OpenGL uses right-handed coordinate system, where
				// camera is looking in negative z direction:
				//   OO
				//  |__|<--------------------
				//         -z             +z
				// Consequently, OpenGL projection matrix given by these two
				// references inverts z axis.

				// We do not need to do this, because we use DX coordinate
				// system for the camera space. Thus we need to invert the
				// sign of the values in the third column in the matrix
				// from the references:

				_33 = -(-(zFar + zNear) / (zFar - zNear));
				_43 = -2 * zNear * zFar / (zFar - zNear);
				_34 = -(-1);
			}
			else
			{
				_33 = zFar / (zFar - zNear);
				_43 = -zNear * zFar / (zFar - zNear);
				_34 = 1;
			}
		}
		else
		{
			// Orthographic projection
			_33 = (NegativeOneToOneZ ? 2 : 1) / (zFar - zNear);
			_43 = (NegativeOneToOneZ ? zNear + zFar : zNear) / (zNear - zFar);
		}
	}
	static Matrix4x4 OrthoOffCenter(T left, T right, T bottom, T top, T zNear, T zFar, bool NegativeOneToOneZ) // Left-handed ortho projection
	{
		// clang-format off
		Matrix4x4 Proj
			{
						 2   / (right - left),                                 0,   0,    0,
											0,                2 / (top - bottom),   0,    0,
											0,                                 0,   0,    0,
				(left + right)/(left - right),   (top + bottom) / (bottom - top),   0,    1
			};
		// clang-format on
		Proj.SetNearFarClipPlanes(zNear, zFar, NegativeOneToOneZ);
		return Proj;
	}
*/

void R_SetupFrustumProjectionMatrix(float left, float right, float bottom, float top, float zNear, float zFar)
{
	memset(r_projection_matrix, 0, sizeof(float) * 16);

	// 透视投影矩阵 (按照glFrustum实现)
	// 参考OpenGL规范中的glFrustum矩阵公式
	float rl = right - left;
	float tb = top - bottom;
	float fn = zFar - zNear;

	r_projection_matrix[0] = (2.0f * zNear) / rl;                    // _11
	r_projection_matrix[5] = (2.0f * zNear) / tb;                    // _22
	r_projection_matrix[8] = (right + left) / rl;                    // _31
	r_projection_matrix[9] = (top + bottom) / tb;                    // _32
	r_projection_matrix[10] = -(zFar + zNear) / fn;                  // _33
	r_projection_matrix[11] = -1.0f;                                 // _34
	r_projection_matrix[14] = -(2.0f * zFar * zNear) / fn;          // _43
}

void R_SetupOrthoProjectionMatrix(float left, float right, float bottom, float top, float zNear, float zFar, bool NegativeOneToOneZ)
{
	memset(r_projection_matrix, 0, sizeof(float) * 16);

	// 基础正交投影矩阵布局 (按照OrthoOffCenter实现)
	r_projection_matrix[0] = 2.0f / (right - left);                     // _11
	r_projection_matrix[5] = 2.0f / (top - bottom);                     // _22
	r_projection_matrix[12] = (left + right) / (left - right);          // _41
	r_projection_matrix[13] = (top + bottom) / (bottom - top);          // _42
	r_projection_matrix[15] = 1.0f;                                     // _44

	// 设置近远裁剪平面 (按照SetNearFarClipPlanes实现)
	if (NegativeOneToOneZ)
	{
		// OpenGL风格 [-1, 1] NDC Z范围
		r_projection_matrix[10] = 2.0f / (zFar - zNear);               // _33
		r_projection_matrix[14] = (zNear + zFar) / (zNear - zFar);     // _43
	}
	else
	{
		// DirectX风格 [0, 1] NDC Z范围
		r_projection_matrix[10] = 1.0f / (zFar - zNear);               // _33
		r_projection_matrix[14] = zNear / (zNear - zFar);              // _43
	}
}

void GL_BeginDebugGroup(const char *name)
{
#if defined(_DEBUG)
	if (glPushDebugGroup)
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
#endif
}

void GL_BeginDebugGroupFormat(const char* fmt, ...)
{
#if defined(_DEBUG)
	char buf[256]{};

	va_list argptr;

	va_start(argptr, fmt);
	_vsnprintf(buf, sizeof(buf) - 1, fmt, argptr);
	va_end(argptr);

	buf[sizeof(buf) - 1] = 0;

	if (glPushDebugGroup)
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, buf);
#endif
}

void GL_EndDebugGroup()
{
#if defined(_DEBUG)
	if (glPopDebugGroup)
		glPopDebugGroup();
#endif
}

void GL_SetTextureDebugName(GLuint textureId, const char* name)
{
	if (glObjectLabel)
	{
		glObjectLabel(GL_TEXTURE, textureId, -1, name);
	}
}

void GL_SetTextureDebugNameFormat(GLuint textureId, const char* fmt, ...)
{
	char buf[256]{};

	va_list argptr;

	va_start(argptr, fmt);
	_vsnprintf(buf, sizeof(buf) - 1, fmt, argptr);
	va_end(argptr);

	buf[sizeof(buf) - 1] = 0;

	if (glObjectLabel)
	{
		glObjectLabel(GL_TEXTURE, textureId, -1, buf);
	}
}

void GL_SetFrameBufferDebugName(GLuint framebufferId, const char* name)
{
	if (glObjectLabel)
	{
		glObjectLabel(GL_FRAMEBUFFER, framebufferId, -1, name);
	}
}

void GL_SetFrameBufferDebugNameFormat(GLuint framebufferId, const char* fmt, ...)
{
	char buf[256]{};

	va_list argptr;

	va_start(argptr, fmt);
	_vsnprintf(buf, sizeof(buf) - 1, fmt, argptr);
	va_end(argptr);

	buf[sizeof(buf) - 1] = 0;

	if (glObjectLabel)
	{
		glObjectLabel(GL_FRAMEBUFFER, framebufferId, -1, buf);
	}
}