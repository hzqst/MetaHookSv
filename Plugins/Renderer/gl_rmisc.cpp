#include "gl_local.h"

vec3_t save_vieworg[MAX_SAVEREFDEF_STACK];
vec3_t save_viewang[MAX_SAVEREFDEF_STACK];
int save_refdefstack = 0;

void R_PushFrameBuffer(void)
{
	if(gl_framebuffer_object)
	{
		qglGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (GLint *)&readframebuffer);
		qglGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint *)&drawframebuffer);
	}
}

void R_PopFrameBuffer(void)
{
	if(gl_framebuffer_object)
	{
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER, readframebuffer);
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, drawframebuffer);
	}
}

void R_PushMatrix(void)
{
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
}

void R_PopMatrix(void)
{
	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();

	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();
}

void R_GLBindFrameBuffer(GLenum target, GLuint framebuffer)
{
	if(gl_framebuffer_object)
	{
		qglBindFramebufferEXT(target, framebuffer);
	}
}

void R_GLUploadTextureColorFormat(int texid, int w, int h, int iInternalFormat)
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

GLuint R_GLGenTextureColorFormat(int w, int h, int iInternalFormat)
{
	GLuint texid = GL_GenTexture();
	R_GLUploadTextureColorFormat(texid, w, h, iInternalFormat);
	return texid;
}

GLuint R_GLGenTextureRGBA8(int w, int h)
{
	return R_GLGenTextureColorFormat(w, h, GL_RGBA8);
}

void R_GLUploadDepthTexture(int texId, int w, int h)
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

GLuint R_GLGenDepthTexture(int w, int h)
{
	GLuint texid = GL_GenTexture();
	R_GLUploadDepthTexture(texid, w, h);
	return texid;
}

void R_GLUploadShadowTexture(int texid, int w, int h)
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

GLuint R_GLGenShadowTexture(int w, int h)
{
	GLuint texid = GL_GenTexture();
	R_GLUploadShadowTexture(texid, w, h);
	return texid;
}

void R_PushRefDef(void)
{
	if(save_refdefstack == MAX_SAVEREFDEF_STACK)
	{
		Sys_ErrorEx("R_PushRefDef: MAX_SAVEREFDEF_STACK exceed\n");
		return;
	}
	VectorCopy(r_refdef->vieworg, save_vieworg[save_refdefstack]);
	VectorCopy(r_refdef->viewangles, save_viewang[save_refdefstack]);
	++ save_refdefstack;
}

void R_UpdateRefDef(void)
{
	VectorCopy(r_refdef->vieworg, r_origin);
	AngleVectors(r_refdef->viewangles, vpn, vright, vup);
}

float *R_GetSavedViewOrg(void)
{
	return save_vieworg[save_refdefstack-1];
}

void R_PopRefDef(void)
{
	if(save_refdefstack == 0)
	{
		Sys_ErrorEx("R_PushRefDef: no refdef is pushed\n");
		return;
	}
	-- save_refdefstack;
	VectorCopy(save_vieworg[save_refdefstack], r_refdef->vieworg);
	VectorCopy(save_viewang[save_refdefstack], r_refdef->viewangles);
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

void R_NewMap(void)
{
	r_worldentity = gEngfuncs.GetEntityByIndex(0);
	r_worldmodel = r_worldentity->model;

	gRefFuncs.R_NewMap();

	R_VidInitWSurf();
}

mleaf_t *Mod_PointInLeaf(vec3_t p, model_t *model)
{
	if (drawreflect && model == r_worldmodel && 0  == VectorCompare(p, r_refdef->vieworg))
	{
		return gRefFuncs.Mod_PointInLeaf(water_view, model);
	}

	return gRefFuncs.Mod_PointInLeaf(p, model);
}

float *R_GetAttachmentPoint(int entity, int attachment)
{
	cl_entity_t *pEntity;

	pEntity = gEngfuncs.GetEntityByIndex(entity);

	if (attachment)
		return pEntity->attachment[attachment - 1];

	return pEntity->origin;
}