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

GLuint R_GLGenTexture(int w, int h)
{
	GLuint texid = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, texid);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
	return texid;
}

GLuint R_GLGenTextureColorFormat(int w, int h, int iInternalFormat)
{
	GLuint texid = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, texid);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//glTexStorage2D doesnt work with qglCopyTexImage2D so we use glTexImage2D here
	qglTexImage2D(GL_TEXTURE_2D, 0, iInternalFormat, w, h, 0, GL_RGBA,
		(iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, 0);

	qglBindTexture(GL_TEXTURE_2D, 0);
	return texid;
}

GLuint R_GLUploadTextureColorFormat(int texid, int w, int h, int iInternalFormat)
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
	return texid;
}

GLuint R_GLGenDepthTexture(int w, int h)
{
	GLuint texid = GL_GenTexture();
	qglBindTexture(GL_TEXTURE_2D, texid);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexStorage2D doesnt work with qglCopyTexImage2D so we use glTexImage2D here
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
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
	mnode_t *node;
	float d;
	mplane_t *plane;

	if (!model || !model->nodes)
		Sys_ErrorEx("Mod_PointInLeaf: bad model");

	node = model->nodes;

	//Don't clip bsp nodes when rendering refract or reflect view for non-transparent water.
	/*if (drawrefract)
	{
		if (curwater && curwater->color.a == 255)
		{
			if (node->contents < 0)
				return (mleaf_t *)node;
		}
	}
	else if (drawreflect)
	{
		if (node->contents < 0)
			return (mleaf_t *)node;
	}*/

	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t *)node;

		plane = node->plane;
		d = DotProduct(p,plane->normal) - plane->dist;

		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;
}

float *R_GetAttachmentPoint(int entity, int attachment)
{
	cl_entity_t *pEntity;

	pEntity = gEngfuncs.GetEntityByIndex(entity);

	if (attachment)
		return pEntity->attachment[attachment - 1];

	return pEntity->origin;
}