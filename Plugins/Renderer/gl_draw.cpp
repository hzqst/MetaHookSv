#include "gl_local.h"
#include "MurmurHash2.h"

extern "C"
{
#include "FreeImage/FreeImage.h"
};

#pragma comment(lib,"FreeImage/FreeImage.lib")

static byte texloader_buffer[4096 * 4096 * 4];

gltexture_t *gltextures = NULL;
gltexture_t **gltextures_SvEngine = NULL;//for SvEngine
int *maxgltextures_SvEngine = NULL;//for SvEngine
int *peakgltextures_SvEngine = NULL;//for SvEngine
int *numgltextures = NULL;
int *gHostSpawnCount = NULL;
int *currenttexture = NULL;
int *oldtarget = NULL;
cachewad_t **decal_wad = NULL;
qboolean* gfCustomBuild = NULL;
char (*szCustName)[10] = NULL;

int *gl_filter_min = NULL;
int *gl_filter_max = NULL;

int gl_loadtexture_format = GL_RGBA;
int gl_loadtexture_cubemap = 0;
bool gl_loadtexture_compressed = false;

std::vector<mipmap_texture_data_t> gl_loadtexture_mipmap;

gltexture_t *gltextures_get()
{
	return (gltextures_SvEngine) ? (*gltextures_SvEngine) : gltextures;
}

typedef struct
{
	char *name;
	int minimize, maximize;
}glmode_t;

static glmode_t gl_texture_modes[] =
{
	{ "GL_NEAREST", GL_NEAREST, GL_NEAREST },
	{ "GL_LINEAR", GL_LINEAR, GL_LINEAR },
	{ "GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR },
	{ "GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR }
};

void GL_Texturemode_internal(const char *value)
{
	int i;

	for (i = 0; i < 6; i++)
	{
		if (!stricmp(gl_texture_modes[i].name, value))
			break;
	}

	if (i == 6)
	{
		gEngfuncs.Con_Printf("GL_Texturemode: bad filter name. Only the following values are supported:\nGL_NEAREST\nGL_LINEAR\nGL_NEAREST_MIPMAP_NEAREST\nGL_LINEAR_MIPMAP_NEAREST\nGL_NEAREST_MIPMAP_LINEAR\nGL_LINEAR_MIPMAP_LINEAR\n");
		return;
	}

	*gl_filter_min = gl_texture_modes[i].minimize;
	*gl_filter_max = gl_texture_modes[i].maximize;

	R_FreeBindlessTexturesForWorld();

	auto pgltextures = gltextures_get();

	for (int j = 0; j < (*numgltextures); ++j)
	{
		if (pgltextures[j].texnum)
		{
			if (pgltextures[j].mipmap)
			{
				GL_Bind(pgltextures[j].texnum);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, *gl_filter_min);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max(min(gl_ansio->value, gl_max_ansio), 1));
			}
			else
			{
				GL_Bind(pgltextures[j].texnum);

				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, *gl_filter_max);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max(min(gl_ansio->value, gl_max_ansio), 1));
			}
		}
	}

	R_CreateBindlessTexturesForWorld();

	R_FreeBindlessTexturesForSkybox();

	for (int j = 0; j < 12; ++j)
	{
		if (r_wsurf.vSkyboxTextureId[j])
		{
			GL_Bind(r_wsurf.vSkyboxTextureId[j]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, *gl_filter_min);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max(min(gl_ansio->value, gl_max_ansio), 1));
		}
	}

	R_CreateBindlessTexturesForSkybox();
}

void GL_Texturemode_cb(cvar_t *pcvar)
{
	GL_Texturemode_internal(pcvar->string);
}

void GL_Texturemode_f(void)
{
	if (gEngfuncs.Cmd_Argc() >= 2)
		GL_Texturemode_internal(gEngfuncs.Cmd_Argv(1));
}

void GL_GenerateHashedTextureIndentifier(const char* identifier, GL_TEXTURETYPE textureType, char* hashedIdentifier, size_t len)
{
	if (textureType == GLT_SYSTEM)
	{
		snprintf(hashedIdentifier, len, "@SYS_%08X", MurmurHash2(identifier, strlen(identifier), textureType));
	}
	else if (textureType == GLT_DECAL)
	{
		snprintf(hashedIdentifier, len, "@DCL_%08X", MurmurHash2(identifier, strlen(identifier), textureType));
	}
	else if (textureType == GLT_HUDSPRITE)
	{
		snprintf(hashedIdentifier, len, "@SPH_%08X", MurmurHash2(identifier, strlen(identifier), textureType));
	}
	else if (textureType == GLT_STUDIO)
	{
		snprintf(hashedIdentifier, len, "@MDL_%08X", MurmurHash2(identifier, strlen(identifier), textureType));
	}
	else if (textureType == GLT_WORLD)
	{
		snprintf(hashedIdentifier, len, "@BSP_%08X", MurmurHash2(identifier, strlen(identifier), textureType));
	}
	else if (textureType == GLT_SPRITE)
	{
		snprintf(hashedIdentifier, len, "@SPR_%08X", MurmurHash2(identifier, strlen(identifier), textureType));
	}
	else if (textureType == GLT_DETAIL)
	{
		snprintf(hashedIdentifier, len, "@DET_%08X", MurmurHash2(identifier, strlen(identifier), textureType));
	}
	else
	{
		strncpy(hashedIdentifier, identifier, len);
	}
}

void GL_GenerateHashedTextureIndentifier2(const char* identifier, GL_TEXTURETYPE textureType, int width, int height, char * hashedIdentifier, size_t len)
{
	if (textureType == GLT_SYSTEM)
	{
		snprintf(hashedIdentifier, len, "@SYS_%08X_%04X_%04X", MurmurHash2(identifier, strlen(identifier), textureType), width, height);
	}
	else if (textureType == GLT_DECAL)
	{
		snprintf(hashedIdentifier, len, "@DCL_%08X_%04X_%04X", MurmurHash2(identifier, strlen(identifier), textureType), width, height);
	}
	else if (textureType == GLT_HUDSPRITE)
	{
		snprintf(hashedIdentifier, len, "@SPH_%08X_%04X_%04X", MurmurHash2(identifier, strlen(identifier), textureType), width, height);
	}
	else if (textureType == GLT_STUDIO)
	{
		snprintf(hashedIdentifier, len, "@MDL_%08X_%04X_%04X", MurmurHash2(identifier, strlen(identifier), textureType), width, height);
	}
	else if (textureType == GLT_WORLD)
	{
		snprintf(hashedIdentifier, len, "@BSP_%08X_%04X_%04X", MurmurHash2(identifier, strlen(identifier), textureType), width, height);
	}
	else if (textureType == GLT_SPRITE)
	{
		snprintf(hashedIdentifier, len, "@SPR_%08X_%04X_%04X", MurmurHash2(identifier, strlen(identifier), textureType), width, height);
	}
	else if (textureType == GLT_DETAIL)
	{
		snprintf(hashedIdentifier, len, "@DET_%08X_%04X_%04X", MurmurHash2(identifier, strlen(identifier), textureType), width, height);
	}
	else
	{
		strncpy(hashedIdentifier, identifier, len);
	}
}

GL_TEXTURETYPE GL_GetTextureTypeFromGLTexture(gltexture_t* glt)
{
	if (glt->identifier[0] == '@')
	{
		if (glt->identifier[1] == 'S' &&
			glt->identifier[2] == 'Y' &&
			glt->identifier[3] == 'S' &&
			glt->identifier[4] == '_')
		{
			return GLT_SYSTEM;
		}

		if (glt->identifier[1] == 'D' &&
			glt->identifier[2] == 'C' &&
			glt->identifier[3] == 'L' &&
			glt->identifier[4] == '_')
		{
			return GLT_DECAL;
		}

		if (glt->identifier[1] == 'S' &&
			glt->identifier[2] == 'P' &&
			glt->identifier[3] == 'H' &&
			glt->identifier[4] == '_')
		{
			return GLT_HUDSPRITE;
		}

		if (glt->identifier[1] == 'M' &&
			glt->identifier[2] == 'D' &&
			glt->identifier[3] == 'L' &&
			glt->identifier[4] == '_')
		{
			return GLT_STUDIO;
		}

		if (glt->identifier[1] == 'B' &&
			glt->identifier[2] == 'S' &&
			glt->identifier[3] == 'P' &&
			glt->identifier[4] == '_')
		{
			return GLT_WORLD;
		}

		if (glt->identifier[1] == 'S' &&
			glt->identifier[2] == 'P' &&
			glt->identifier[3] == 'R' &&
			glt->identifier[4] == '_')
		{
			return GLT_SPRITE;
		}
	}

	return GLT_UNKNOWN;
}

//GL Start

GLuint GL_GenTexture(void)
{
	GLuint tex;
	glGenTextures(1, &tex);
	return tex;
}

GLuint GL_GenBuffer(void)
{
	GLuint buf;
	glGenBuffers(1, &buf);
	return buf;
}

GLuint GL_GenVAO(void)
{
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	return VAO;
}

void GL_DeleteBuffer(GLuint buf)
{
	glDeleteBuffers(1, &buf);
}

void GL_DeleteVAO(GLuint VAO)
{
	glDeleteVertexArrays(1, &VAO);
}

void GL_DeleteTexture(GLuint tex)
{
	glDeleteTextures(1, &tex);
}

void GL_BindVAO(GLuint VAO)
{
	glBindVertexArray(VAO);
}

void GL_UploadDataToVBO(GLuint VBO, size_t size, const void* data)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GL_UploadDataToEBO(GLuint EBO, size_t size, const void* data)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GL_BindStatesForVAO(GLuint VAO, GLuint VBO, GLuint EBO, void(*bind)(), void(*unbind)())
{
	GL_BindVAO(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	bind();

	GL_BindVAO(0);

	unbind();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GL_Bind(int texnum)
{
	gRefFuncs.GL_Bind(texnum);
}

void GL_SelectTexture(GLenum target)
{
	gRefFuncs.GL_SelectTexture(target);
}

void GL_DisableMultitexture(void)
{
	gRefFuncs.GL_DisableMultitexture();
}

void GL_EnableMultitexture(void)
{
	gRefFuncs.GL_EnableMultitexture();
}

void GL_UnloadTextureByIdentifier(const char* identifier, bool notify_callback)
{
	int i;
	gltexture_t* glt;

	if (identifier[0] == '@')
	{
		for (i = 0, glt = gltextures_get(); i < (*numgltextures); i++, glt++)
		{
			if (!strncmp(identifier, glt->identifier, sizeof("@SPR_DEADBEEF") - 1))
			{
				GL_FreeTexture(glt, notify_callback);
				return;
			}
		}
	}
	else
	{
		for (i = 0, glt = gltextures_get(); i < (*numgltextures); i++, glt++)
		{
			if (!strcmp(identifier, glt->identifier))
			{
				GL_FreeTexture(glt, notify_callback);
				return;
			}
		}
	}
}

void GL_UnloadTextures(void)
{
	int i;
	gltexture_t* glt;

	for (i = 0, glt = gltextures_get(); i < (*numgltextures); i++, glt++)
	{
		if (glt->servercount > 0 && glt->servercount != (*gHostSpawnCount))
		{
			GL_FreeTexture(glt, true);
		}
	}
}

void GL_UnloadTextureWithType(const char* identifier, GL_TEXTURETYPE textureType, bool notify_callback)
{
	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier(identifier, textureType, hashedIdentifier, sizeof(hashedIdentifier) - 1);

	GL_UnloadTextureByIdentifier(identifier, notify_callback);
}

void GL_UnloadTextureWithType(const char* identifier, GL_TEXTURETYPE textureType, int width, int height, bool notify_callback)
{
	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier2(identifier, textureType, width, height, hashedIdentifier, sizeof(hashedIdentifier) - 1);

	GL_UnloadTextureByIdentifier(identifier, notify_callback);
}

void GL_UnloadTextureByTextureId(int gltexturenum, bool notify_callback)
{
	int i;
	gltexture_t* glt;

	for (i = 0, glt = gltextures_get(); i < (*numgltextures); i++, glt++)
	{
		if (glt->texnum == gltexturenum)
		{
			GL_FreeTexture(glt, notify_callback);
			break;
		}
	}
}

void GL_UploadDXT(void *data, int width, int height, qboolean mipmap, qboolean ansio, int wrap)
{
	int iTextureTarget = GL_TEXTURE_2D;
	int iMipmapTextureTarget = GL_TEXTURE_2D;

	if (gl_loadtexture_cubemap)
	{
		iTextureTarget = GL_TEXTURE_CUBE_MAP;
		iMipmapTextureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + gl_loadtexture_cubemap - 1;
	}

	//Legacy
	glTexParameteri(iTextureTarget, GL_GENERATE_MIPMAP, GL_FALSE);

	if (mipmap)
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MIN_FILTER, *gl_filter_min);
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
	}
	else
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MIN_FILTER, *gl_filter_max);
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
	}

	if (ansio && gl_ansio)
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, max(min(gl_ansio->value, gl_max_ansio), 1));
	}
	else
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
	}

	glTexParameteri(iTextureTarget, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(iTextureTarget, GL_TEXTURE_WRAP_T, wrap);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmap ? (gl_loadtexture_mipmap.size() - 1) : 0);

	for (size_t i = 0; i < gl_loadtexture_mipmap.size(); ++i)
	{
		glCompressedTexImage2D(iMipmapTextureTarget, gl_loadtexture_mipmap[i].level, gl_loadtexture_format, gl_loadtexture_mipmap[i].width, gl_loadtexture_mipmap[i].height, 0, gl_loadtexture_mipmap[i].size, gl_loadtexture_mipmap[i].data);
		
		if (!mipmap)
			break;
	}

	if (gl_loadtexture_mipmap.size() == 1 && mipmap)
	{
		//available since OpenGL 3.0
		glGenerateMipmap(iTextureTarget);
	}

	gl_loadtexture_mipmap.clear();
	gl_loadtexture_format = 0;
}

void GL_UploadRGBA8(void *data, int width, int height, qboolean mipmap, qboolean ansio, int wrap)
{
	int iTextureTarget = GL_TEXTURE_2D;
	int iMipmapTextureTarget = GL_TEXTURE_2D;

	if (gl_loadtexture_cubemap)
	{
		iTextureTarget = GL_TEXTURE_CUBE_MAP;
		iMipmapTextureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + gl_loadtexture_cubemap - 1;
	}

	//Legacy
	glTexParameteri(iTextureTarget, GL_GENERATE_MIPMAP, GL_FALSE);

	if (mipmap)
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MIN_FILTER, *gl_filter_min);
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
	}
	else
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MIN_FILTER, *gl_filter_max);
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
	}

	if(ansio && gl_ansio)
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, max(min(gl_ansio->value, gl_max_ansio), 1));
	}
	else
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
	}

	glTexParameteri(iTextureTarget, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(iTextureTarget, GL_TEXTURE_WRAP_T, wrap);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmap ? (gl_loadtexture_mipmap.size() - 1) : 0);

	for (size_t i = 0; i < gl_loadtexture_mipmap.size(); ++i)
	{
		glTexImage2D(iMipmapTextureTarget, gl_loadtexture_mipmap[i].level, gl_loadtexture_format, gl_loadtexture_mipmap[i].width, gl_loadtexture_mipmap[i].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, gl_loadtexture_mipmap[i].data);
	
		if (!mipmap)
			break;
	}

	if (gl_loadtexture_mipmap.size() == 1 && mipmap)
	{
		//available since OpenGL 3.0
		glGenerateMipmap(iTextureTarget);
	}

	gl_loadtexture_mipmap.clear();
	gl_loadtexture_format = 0;
}

int GL_FindTextureByHashedIdentifier(const char *hashedIdentifier, GL_TEXTURETYPE textureType, int *width, int *height)
{
	int i;
	gltexture_t *slot;

	if (hashedIdentifier[0])
	{
		for (i = 0, slot = gltextures_get(); i < *numgltextures; i++, slot++)
		{
			if (!strncmp(hashedIdentifier, slot->identifier, sizeof("@SPR_DEADBEEF") - 1))
			{
				if (textureType == GLT_WORLD)
				{
					if (slot->servercount != *gHostSpawnCount)
						continue;
				}

				if (width)
					*width = slot->width;
				if (height)
					*height = slot->height;

				return slot->texnum;
			}
		}
	}
	else
	{
		gEngfuncs.Con_DPrintf("NULL Texture\n");
	}

	return 0;
}

int GL_FindTexture(const char* identifier, GL_TEXTURETYPE textureType, int* width, int* height)
{
	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier(identifier, textureType, hashedIdentifier, sizeof(hashedIdentifier) - 1);


	return GL_FindTextureByHashedIdentifier(hashedIdentifier, textureType, width, height);
}

int GL_AllocTexture(const char *identifier, GL_TEXTURETYPE textureType, int width, int height, qboolean mipmap)
{
	int i;
	gltexture_t *glt;
	gltexture_t *slot;

	glt = NULL;

//tryagain:
	if (identifier[0])
	{
		for (i = 0, slot = gltextures_get(); i < *numgltextures; i++, slot++)
		{
			if (slot->servercount < 0)
			{
				if (!glt)
					glt = slot;

				break;
			}

			if (!strcmp(identifier, slot->identifier) && width == slot->width && height == slot->height)
			{
				if (slot->servercount > 0)
					slot->servercount = *gHostSpawnCount;

				return slot->texnum;
			}
		}
	}
	else
	{
		gEngfuncs.Con_DPrintf("NULL Texture\n");
	}

	if (!glt)
	{
		if (maxgltextures_SvEngine)
		{
			if ((*numgltextures) + 1 > (*maxgltextures_SvEngine))
			{
				//realloc just like SvEngine does.
				int v16 = (*numgltextures) - (*maxgltextures_SvEngine) + 1;
				if (/*dword_30FC6F4 >= 0 || */(*numgltextures) - (*maxgltextures_SvEngine) < 0)
				{
					if (v16 <= 0)
					{
						v16 = (*maxgltextures_SvEngine);
						if (!(*maxgltextures_SvEngine))
							v16 = 1;
					}
				}

				*maxgltextures_SvEngine += v16;
				*gltextures_SvEngine = (gltexture_t *)gRefFuncs.realloc_SvEngine((void *)(*gltextures_SvEngine), (*maxgltextures_SvEngine) * sizeof(gltexture_t));
			}
		}
		else
		{
			if ((*numgltextures) + 1 >= EngineGetMaxGLTextures())
			{
				g_pMetaHookAPI->SysError("Texture Overflow: MAX_GLTEXTURES\n");
				return 0;
			}
		}

		glt = &(gltextures_get()[(*numgltextures)]);
		(*numgltextures)++;
	}

	if (peakgltextures_SvEngine)
	{
		if ((*numgltextures) > (*peakgltextures_SvEngine))
			(*peakgltextures_SvEngine) = (*numgltextures);
	}

	if (maxgltextures_SvEngine)
	{
		glt->texnum = GL_GenTexture();
	}
	else
	{
		if (!glt->texnum)
			glt->texnum = GL_GenTexture();
	}

	strncpy(glt->identifier, identifier, sizeof(glt->identifier) - 1);
	glt->identifier[sizeof(glt->identifier) - 1] = 0;
	glt->width = width;
	glt->height = height;
	glt->mipmap = mipmap;
	glt->servercount = (textureType == GLT_WORLD) ? *gHostSpawnCount : 0;
	glt->paletteIndex = -1;

	return glt->texnum;
}

int GL_LoadTexture(const char* identifier, GL_TEXTURETYPE textureType, int width, int height, byte* data, qboolean mipmap, int iType, byte* pPal)
{
	return GL_LoadTexture2(identifier, textureType, width, height, data, mipmap, iType, pPal, (*gl_filter_max));
}

int GL_LoadTexture2(const char* identifier, GL_TEXTURETYPE textureType, int width, int height, byte* data, qboolean mipmap, int iType, byte* pPal, int filter)
{
	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier2(identifier, textureType, width, height, hashedIdentifier, sizeof(hashedIdentifier) - 1);

	int gltexturenum = gRefFuncs.GL_LoadTexture2(hashedIdentifier, textureType, width, height, data, mipmap, iType, pPal, filter);

	gEngfuncs.Con_DPrintf("GL_LoadTexture2: [%s] -> [%s] [%d]\n", identifier, hashedIdentifier, gltexturenum);

	return gltexturenum;
}

int GL_LoadTextureInternal(const char *identifier, GL_TEXTURETYPE textureType, int width, int height, void *data, qboolean mipmap, qboolean ansio)
{
	if (!gl_loadtexture_format)
	{
		g_pMetaHookAPI->SysError("GL_LoadTextureInternal: Invalid gl_loadtexture_format!");
		return 0;
	}

	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier2(identifier, textureType, width, height, hashedIdentifier, sizeof(hashedIdentifier) - 1);

	int gltexturenum = GL_AllocTexture(hashedIdentifier, textureType, width, height, mipmap);

	if (!gltexturenum)
	{
		gEngfuncs.Con_Printf("GL_LoadTextureInternal: Failed to allocate texture entry.\n");
		return 0;
	}

	gEngfuncs.Con_DPrintf("GL_LoadTextureInternal: [%s] -> [%s] [%d]\n", identifier, hashedIdentifier, gltexturenum);

	int iTextureTarget = GL_TEXTURE_2D;

	if (gl_loadtexture_cubemap)
		iTextureTarget = GL_TEXTURE_CUBE_MAP;

	int iWrap = GL_REPEAT;

	if (textureType == GLT_HUDSPRITE || textureType == GLT_SPRITE)
		iWrap = GL_CLAMP_TO_EDGE;

	glBindTexture(iTextureTarget, gltexturenum);
	(*currenttexture) = -1;

	if (gl_loadtexture_compressed)
	{
		GL_UploadDXT(data, width, height, mipmap, ansio, iWrap);
	}
	else
	{
		GL_UploadRGBA8(data, width, height, mipmap, ansio, iWrap);
	}

	glBindTexture(iTextureTarget, 0);
	(*currenttexture) = -1;

	return gltexturenum;
}

texture_t *Draw_DecalTexture(int index)
{
	texture_t *texture = gRefFuncs.Draw_DecalTexture(index);

	return texture;
}

int LittleLong(int l);
short LittleShort(short l);
float LittleFloat(float l);

void Draw_MiptexTexture(cachewad_t *wad, byte *data)
{
	texture_t* tex;
	miptex_t* mip, tmp;
	int i, pix, paloffset, palettesize;
	byte* pal, * bitmap;

	if (wad->cacheExtra != 32)
	{
		gEngfuncs.Con_Printf("Draw_MiptexTexture: Bad cached wad %s\n", wad->name);
		return;
	}

	tmp = *(miptex_t*)(data + wad->cacheExtra);
	tex = (texture_t*)data;
	mip = &tmp;

	memcpy(tex->name, mip->name, sizeof(tex->name));
	tex->width = LittleLong(mip->width);
	tex->height = LittleLong(mip->height);
	tex->anim_max = 0;
	tex->anim_min = 0;
	tex->anim_total = 0;
	tex->alternate_anims = NULL;
	tex->anim_next = NULL;

	for (i = 0; i < MIPLEVELS; i++)
		tex->offsets[i] = LittleLong(mip->offsets[i]) + wad->cacheExtra;

	pix = tex->width * tex->height;
	paloffset = 0;

	for (i = 0; i < MIPLEVELS; i++, pix >>= 2)
		paloffset += pix;

	bitmap = (byte*)(data + tex->offsets[0]);
	pal = (byte*)(data + tex->offsets[0] + paloffset + sizeof(short));
	palettesize = *(unsigned short*)(data + sizeof(miptex_t) + pix);

	if ((*gfCustomBuild))
	{
		strncpy(tex->name, (*szCustName), sizeof(tex->name) - 1);
		tex->name[sizeof(tex->name) - 1] = 0;
	}

	if (pal[765] == 0 && pal[766] == 0 && pal[767] == 255)
	{
		tex->name[0] = '{';
		tex->gl_texturenum = GL_LoadTexture(tex->name, GLT_DECAL, tex->width, tex->height, bitmap, true, TEX_TYPE_ALPHA, pal);
	}
	else
	{
		tex->name[0] = '}';

		if ((*gfCustomBuild))
			GL_UnloadTextureWithType(tex->name, GLT_DECAL, true);

		//Why'th fuck 2 in SvEngine?
		int iTexType = (g_iEngineType == ENGINE_SVENGINE) ? 2 : TEX_TYPE_ALPHA_GRADIENT;

		tex->gl_texturenum = GL_LoadTexture(tex->name, GLT_DECAL, tex->width, tex->height, bitmap, true, iTexType, pal);
	}
}

DWORD ByteToUInt( byte *byte )
{
	DWORD iValue = byte[0];
	iValue += (byte[1]<<8);
	iValue += (byte[2]<<16);
	iValue += (byte[3]<<24);

	return iValue;
}

qboolean PowerOfTwo(int iWidth,int iHeight)
{
	int iWidthT = iWidth;
	while(iWidthT != 1)
	{
		if((iWidthT % 2) != 0) return false;
		iWidthT /=2;
	}

	int iHeightT = iHeight;
	while(iHeightT != 1)
	{
		if((iHeightT % 2) != 0) return false;
		iHeightT /=2;
	}
	return true;
}

qboolean LoadDDS(const char* filename, byte* buf, size_t bufsize, size_t* width, size_t* height, qboolean throw_warning_on_missing)
{
	DDS_FILEHEADER10 fileHeader10;

	if (width)
		*width = 0;

	if (height)
		*height = 0;

	FileHandle_t fileHandle = FILESYSTEM_ANY_OPEN(filename, "rb");

	if (!fileHandle)
	{
		if (throw_warning_on_missing)
		{
			gEngfuncs.Con_Printf("LoadDDS: Could not open %s.\n", filename);
		}
		return false;
	}

	if (sizeof(DDS_FILEHEADER) != FILESYSTEM_ANY_READ(&fileHeader10, sizeof(DDS_FILEHEADER), fileHandle))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
		FILESYSTEM_ANY_CLOSE(fileHandle);
		return false;
	}

	if (fileHeader10.dwMagic != DDS_MAGIC)
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
		FILESYSTEM_ANY_CLOSE(fileHandle);
		return false;
	}

	if (fileHeader10.Header.dwSize != 124)
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
		FILESYSTEM_ANY_CLOSE(fileHandle);
		return false;
	}

	if (!(fileHeader10.Header.dwFlags & DDSD_PIXELFORMAT))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
		FILESYSTEM_ANY_CLOSE(fileHandle);
		return false;
	}

	if (!(fileHeader10.Header.dwFlags & DDSD_CAPS))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
		FILESYSTEM_ANY_CLOSE(fileHandle);
		return false;
	}

	if (!(fileHeader10.Header.ddspf.dwFlags & DDPF_FOURCC))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
		FILESYSTEM_ANY_CLOSE(fileHandle);
		return false;
	}

	if (fileHeader10.Header.ddspf.dwFourCC != D3DFMT_DXT1 &&
		fileHeader10.Header.ddspf.dwFourCC != D3DFMT_DXT3 &&
		fileHeader10.Header.ddspf.dwFourCC != D3DFMT_DXT5 &&
		fileHeader10.Header.ddspf.dwFourCC != D3DFMT_BC7 &&
		fileHeader10.Header.ddspf.dwFourCC != D3DFMT_DX10)
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s has unsupported compressed texture format! Only DXT1/3/5 and BC7 supported!\n", filename);
		FILESYSTEM_ANY_CLOSE(fileHandle);
		return false;
	}

	if (fileHeader10.Header.ddspf.dwFourCC == D3DFMT_DX10)
	{
		if (sizeof(DDS_HEADER_DXT10) != FILESYSTEM_ANY_READ(&fileHeader10.Header10, sizeof(DDS_HEADER_DXT10), fileHandle))
		{
			gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
			FILESYSTEM_ANY_CLOSE(fileHandle);
			return false;
		}

		FILESYSTEM_ANY_SEEK(fileHandle, sizeof(DDS_FILEHEADER10), FILESYSTEM_SEEK_HEAD);
	}
	else
	{
		FILESYSTEM_ANY_SEEK(fileHandle, sizeof(DDS_FILEHEADER), FILESYSTEM_SEEK_HEAD);
	}

	gl_loadtexture_mipmap.clear();

	size_t offset = 0;
	size_t w = fileHeader10.Header.dwWidth;
	size_t h = fileHeader10.Header.dwHeight;

	for (size_t i = 0; i < fileHeader10.Header.dwMipMapCount; ++i)
	{
		switch (fileHeader10.Header.ddspf.dwFourCC)
		{
		case D3DFMT_DX10:
		{
			if (fileHeader10.Header10.dxgiFormat == DXGI_FORMAT_BC7_UNORM && fileHeader10.Header10.resourceDimension == D3D10_RESOURCE_DIMENSION::D3D10_RESOURCE_DIMENSION_TEXTURE2D)
			{
				size_t size = SIZE_OF_BC7(w, h);

				if (offset + size > bufsize)
				{
					gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				gl_loadtexture_format = GL_COMPRESSED_RGBA_BPTC_UNORM;
				gl_loadtexture_compressed = true;
				gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

				offset += size;
				w = w >> 1;
				h = h >> 1;
			}
			else if (fileHeader10.Header10.dxgiFormat == DXGI_FORMAT_BC7_UNORM_SRGB && fileHeader10.Header10.resourceDimension == D3D10_RESOURCE_DIMENSION::D3D10_RESOURCE_DIMENSION_TEXTURE2D)
			{
				size_t size = SIZE_OF_BC7(w, h);

				if (offset + size > bufsize)
				{
					gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				gl_loadtexture_format = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
				gl_loadtexture_compressed = true;
				gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

				offset += size;
				w = w >> 1;
				h = h >> 1;
			}
			else if (fileHeader10.Header10.dxgiFormat == DXGI_FORMAT_BC1_UNORM && fileHeader10.Header10.resourceDimension == D3D10_RESOURCE_DIMENSION::D3D10_RESOURCE_DIMENSION_TEXTURE2D)
			{
				size_t size = SIZE_OF_DXT1(w, h);

				if (offset + size > bufsize)
				{
					gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				gl_loadtexture_format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				gl_loadtexture_compressed = true;
				gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

				offset += size;
				w = w >> 1;
				h = h >> 1;
			}
			else if (fileHeader10.Header10.dxgiFormat == DXGI_FORMAT_BC1_UNORM_SRGB && fileHeader10.Header10.resourceDimension == D3D10_RESOURCE_DIMENSION::D3D10_RESOURCE_DIMENSION_TEXTURE2D)
			{
				size_t size = SIZE_OF_DXT1(w, h);

				if (offset + size > bufsize)
				{
					gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				gl_loadtexture_format = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
				gl_loadtexture_compressed = true;
				gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

				offset += size;
				w = w >> 1;
				h = h >> 1;
			}
			else if (fileHeader10.Header10.dxgiFormat == DXGI_FORMAT_BC2_UNORM && fileHeader10.Header10.resourceDimension == D3D10_RESOURCE_DIMENSION::D3D10_RESOURCE_DIMENSION_TEXTURE2D)
			{
				size_t size = SIZE_OF_DXT2(w, h);

				if (offset + size > bufsize)
				{
					gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				gl_loadtexture_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				gl_loadtexture_compressed = true;
				gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

				offset += size;
				w = w >> 1;
				h = h >> 1;
			}
			else if (fileHeader10.Header10.dxgiFormat == DXGI_FORMAT_BC2_UNORM_SRGB && fileHeader10.Header10.resourceDimension == D3D10_RESOURCE_DIMENSION::D3D10_RESOURCE_DIMENSION_TEXTURE2D)
			{
				size_t size = SIZE_OF_DXT2(w, h);

				if (offset + size > bufsize)
				{
					gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				gl_loadtexture_format = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
				gl_loadtexture_compressed = true;
				gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

				offset += size;
				w = w >> 1;
				h = h >> 1;
			}
			else if (fileHeader10.Header10.dxgiFormat == DXGI_FORMAT_BC3_UNORM && fileHeader10.Header10.resourceDimension == D3D10_RESOURCE_DIMENSION::D3D10_RESOURCE_DIMENSION_TEXTURE2D)
			{
				size_t size = SIZE_OF_DXT2(w, h);

				if (offset + size > bufsize)
				{
					gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				gl_loadtexture_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				gl_loadtexture_compressed = true;
				gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

				offset += size;
				w = w >> 1;
				h = h >> 1;
			}
			else if (fileHeader10.Header10.dxgiFormat == DXGI_FORMAT_BC3_UNORM_SRGB && fileHeader10.Header10.resourceDimension == D3D10_RESOURCE_DIMENSION::D3D10_RESOURCE_DIMENSION_TEXTURE2D)
			{
				size_t size = SIZE_OF_DXT2(w, h);

				if (offset + size > bufsize)
				{
					gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					FILESYSTEM_ANY_CLOSE(fileHandle);
					return false;
				}

				gl_loadtexture_format = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
				gl_loadtexture_compressed = true;
				gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

				offset += size;
				w = w >> 1;
				h = h >> 1;
			}
			else
			{
				gEngfuncs.Con_Printf("LoadDDS: File %s has unsupported compressed texture format! Only DXT1/3/5 and BC7 supported!\n", filename);
				FILESYSTEM_ANY_CLOSE(fileHandle);
				return false;
			}
			break;
		}
		case D3DFMT_BC7:
		{
			size_t size = SIZE_OF_BC7(w, h);

			if (offset + size > bufsize)
			{
				gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
				FILESYSTEM_ANY_CLOSE(fileHandle);
				return false;
			}

			if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
			{
				gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
				FILESYSTEM_ANY_CLOSE(fileHandle);
				return false;
			}

			gl_loadtexture_format = GL_COMPRESSED_RGBA_BPTC_UNORM;
			gl_loadtexture_compressed = true;
			gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

			offset += size;
			w = w >> 1;
			h = h >> 1;

			break;
		}
		case D3DFMT_DXT1:
		{
			size_t size = SIZE_OF_DXT1(w, h);

			if (offset + size > bufsize)
			{
				gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
				FILESYSTEM_ANY_CLOSE(fileHandle);
				return false;
			}

			if (size != FILESYSTEM_ANY_READ((void *)(buf + offset), size, fileHandle))
			{
				gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
				FILESYSTEM_ANY_CLOSE(fileHandle);
				return false;
			}

			gl_loadtexture_format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			gl_loadtexture_compressed = true;
			gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

			offset += size;
			w = w >> 1;
			h = h >> 1;

			break;
		}
		case D3DFMT_DXT3:
		{
			size_t size = SIZE_OF_DXT2(w, h);

			if (offset + size > bufsize)
			{
				gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
				FILESYSTEM_ANY_CLOSE(fileHandle);
				return false;
			}

			if (size != FILESYSTEM_ANY_READ((void *)(buf + offset), size, fileHandle))
			{
				gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
				FILESYSTEM_ANY_CLOSE(fileHandle);
				return false;
			}

			gl_loadtexture_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			gl_loadtexture_compressed = true;
			gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

			offset += size;
			w = w >> 1;
			h = h >> 1;

			break;
		}
		case D3DFMT_DXT5:
		{
			size_t size = SIZE_OF_DXT2(w, h);
			if (offset + size > bufsize)
			{
				gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
				FILESYSTEM_ANY_CLOSE(fileHandle);
				return false;
			}
			
			if (size != FILESYSTEM_ANY_READ((void *)(buf + offset), size, fileHandle))
			{
				gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
				FILESYSTEM_ANY_CLOSE(fileHandle);
				return false;
			}

			gl_loadtexture_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			gl_loadtexture_compressed = true;
			gl_loadtexture_mipmap.emplace_back(i, buf + offset, size, w, h);

			offset += size;
			w = w >> 1;
			h = h >> 1;

			break;
		}
		}

		if (w == 0 || h == 0)
		{
			break;
		}
		if (FILESYSTEM_ANY_EOF(fileHandle))
		{
			break;
		}

	}

	if(width)
		*width = fileHeader10.Header.dwWidth;
	if(height)
		*height = fileHeader10.Header.dwHeight;

	FILESYSTEM_ANY_CLOSE(fileHandle);

	return TRUE;
}

#define PATHSEPARATOR(c) ((c) == '\\' || (c) == '/')
//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the beginning of the unqualified file name 
//			(no path information)
// Input:	in - file name (may be unqualified, relative or absolute path)
// Output:	pointer to unqualified file name
//-----------------------------------------------------------------------------
const char * V_UnqualifiedFileName(const char * in)
{
	// back up until the character after the first path separator we find,
	// or the beginning of the string
	const char * out = in + strlen(in) - 1;
	while ((out > in) && (!PATHSEPARATOR(*(out - 1))))
		out--;
	return out;
}

const char * V_GetFileExtension( const char * path )
{
	const char    *src;

	src = path + strlen(path) - 1;

	//
	// back up until a . or the start
	//
	while (src != path && *(src-1) != '.' )
		src--;

	// check to see if the '.' is part of a pathname
	if (src == path || *src == '\\' || *src == '/' )
	{		
		return NULL;  // no extension
	}

	return src;
}

unsigned WINAPI FI_Read(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
	if(FILESYSTEM_ANY_READ(buffer, size*count, handle))
		return count;
	return 0;
}

unsigned WINAPI FI_Write(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
	if(FILESYSTEM_ANY_WRITE(buffer, size*count, handle))
		return count;
	return 0;
}

int WINAPI FI_Seek(fi_handle handle, long offset, int origin)
{
	FILESYSTEM_ANY_SEEK(handle, offset, (FileSystemSeek_t)origin);
	return 0;
}

long WINAPI FI_Tell(fi_handle handle)
{
	return FILESYSTEM_ANY_TELL(handle);
}

qboolean LoadImageGeneric(const char *filename, byte *buf, size_t bufSize, size_t *width, size_t *height, qboolean throw_warning_on_missing)
{
	FileHandle_t fileHandle = FILESYSTEM_ANY_OPEN(filename, "rb");

	if (!fileHandle)
	{
		if (throw_warning_on_missing)
		{
			gEngfuncs.Con_Printf("LoadImageGeneric: Could not open %s.\n", filename);
		}
		return false;
	}

	FreeImageIO fiIO;
	fiIO.read_proc = FI_Read;
	fiIO.write_proc = FI_Write;
	fiIO.seek_proc = FI_Seek;
	fiIO.tell_proc = FI_Tell;

	FREE_IMAGE_FORMAT fiFormat = FreeImage_GetFileTypeFromHandle(&fiIO, (fi_handle)fileHandle);

	if(fiFormat == FIF_UNKNOWN)
		fiFormat = FreeImage_GetFIFFromFilename(filename);

	if(fiFormat == FIF_UNKNOWN)
    {
		gEngfuncs.Con_Printf("LoadImageGeneric: Could not load %s, Unsupported format.\n", filename);
		FILESYSTEM_ANY_CLOSE(fileHandle);
		return false;
    }

	if(!FreeImage_FIFSupportsReading(fiFormat))
    {
		gEngfuncs.Con_Printf("LoadImageGeneric: Could not load %s, Unsupported format.\n", filename);
		FILESYSTEM_ANY_CLOSE(fileHandle);
		return false;
    }

	FIBITMAP *fiB = FreeImage_LoadFromHandle(fiFormat, &fiIO, (fi_handle)fileHandle);

	FILESYSTEM_ANY_CLOSE(fileHandle);

	if (!fiB)
	{
		gEngfuncs.Con_Printf("LoadImageGeneric: Could not load %s, FreeImage_LoadFromHandle failed.\n", filename);
		return false;
	}

	gl_loadtexture_mipmap.clear();

	size_t pos = 0;
	size_t w = FreeImage_GetWidth(fiB);
	size_t h = FreeImage_GetHeight(fiB);
	size_t blockSize = FreeImage_GetLine(fiB) / w;

	if(w * h * 4 > bufSize)
	{
		FreeImage_Unload(fiB);
		return false;
	}

	for(size_t y = 0; y < h; ++y )
	{
		BYTE *bits = FreeImage_GetScanLine(fiB, h-y-1);
		for( size_t x = 0; x < w; ++x )
		{
			buf[pos++] = bits[FI_RGBA_RED];//B
			buf[pos++] = bits[FI_RGBA_GREEN];//G
			buf[pos++] = bits[FI_RGBA_BLUE];//R
			if(blockSize == 4)
				buf[pos++] = bits[FI_RGBA_ALPHA];//Alpha
			else
				buf[pos++] = 255;
			bits += blockSize;
		}
	}

	if(width)
		*width = w;

	if(height)
		*height = h;

	gl_loadtexture_format = GL_RGBA8;
	gl_loadtexture_compressed = false;

	gl_loadtexture_mipmap.emplace_back(0, buf, pos, w, h);

	FreeImage_Unload(fiB);

	return true;
}

qboolean SaveImageGeneric(const char *filename, int width, int height, byte *data)
{
	const char *extension = V_GetFileExtension(filename);

	FREE_IMAGE_FORMAT fiFormat = FreeImage_GetFIFFromFilename(filename);

	if(fiFormat == FIF_UNKNOWN)
	{
		gEngfuncs.Con_Printf("SaveImageGeneric: Could not save %s, Unsupported format.\n", filename);
		return false;  
	}

	if(!FreeImage_FIFSupportsWriting(fiFormat))
    {
		gEngfuncs.Con_Printf("SaveImageGeneric: Could not save %s, Unsupported format.\n", filename);
		return false;
    }

	FileHandle_t fileHandle = FILESYSTEM_ANY_OPEN(filename, "wb");

	if(!fileHandle)
    {
		gEngfuncs.Con_Printf("SaveImageGeneric: Could not open %s for writing.\n", filename);
		return false;  
    }

	FreeImageIO fiIO;
	fiIO.read_proc = FI_Read;
	fiIO.write_proc = FI_Write;
	fiIO.seek_proc = FI_Seek;
	fiIO.tell_proc = FI_Tell;

	FIBITMAP *fiB = FreeImage_Allocate(width, height, 24);

	int pos = 0;
	int blockSize = FreeImage_GetLine(fiB) / width;

	for(int y = 0; y < height; ++y)
	{
		BYTE *bits = FreeImage_GetScanLine(fiB, y);
		for(int x = 0; x < width; ++x)
		{
			bits[FI_RGBA_RED] = data[pos++];
			bits[FI_RGBA_GREEN] = data[pos++];
			bits[FI_RGBA_BLUE] = data[pos++];
			bits += blockSize;
		}
	}

	if(FALSE == FreeImage_SaveToHandle(fiFormat, fiB, &fiIO, (fi_handle)fileHandle))
    {
		gEngfuncs.Con_Printf("SaveImageGeneric: Could not save %s, FreeImage_SaveToHandle failed.\n", filename);
		FILESYSTEM_ANY_CLOSE(fileHandle);
		FreeImage_Unload(fiB);
        return false;
    }

	FILESYSTEM_ANY_CLOSE(fileHandle);
	FreeImage_Unload(fiB);
	return true;
}

int R_LoadRGBATextureFromMemory(const char* name, int width, int height, void* data, GL_TEXTURETYPE type, qboolean mipmap, qboolean ansio)
{
	gl_loadtexture_format = GL_RGBA8;
	gl_loadtexture_compressed = false;
	gl_loadtexture_mipmap.emplace_back(0, data, 0, width, height);

	return GL_LoadTextureInternal(name, type, width, height, data, mipmap, ansio);
}

int R_LoadTextureFromFile(const char *filepath, const char *name, int *width, int *height, GL_TEXTURETYPE type, qboolean mipmap, qboolean ansio, qboolean throw_warning_on_missing)
{
	size_t w = 0, h = 0;

	const char *extension = V_GetFileExtension(filepath);

	if(!extension)
	{
		if (throw_warning_on_missing)
		{
			gEngfuncs.Con_Printf("R_LoadTextureFromFile: File %s has no extension.\n", filepath);
		}
		return 0;
	}

	if(!stricmp(extension, "dds"))
	{
		if(LoadDDS(filepath, texloader_buffer, sizeof(texloader_buffer), &w, &h, throw_warning_on_missing))
		{
			if(width)
				*width = w;
			if(height)
				*height = h;

			return GL_LoadTextureInternal(name, type, w, h, texloader_buffer, mipmap, ansio);
		}
	}
	else if(LoadImageGeneric(filepath, texloader_buffer, sizeof(texloader_buffer), &w, &h, throw_warning_on_missing))
	{
		if(width)
			*width = w;
		if(height)
			*height = h;

		return GL_LoadTextureInternal(name, type, w, h, texloader_buffer, mipmap, ansio);
	}
	
	return 0;
}

void BuildGammaTable(float g)
{
	gRefFuncs.BuildGammaTable(g);

	//Don't do texgamma space to gamma space convertion
	for (int i = 0; i < 256; i++)
	{
		texgammatable[i] = i;
	}

	//Don't do lightgamma space to gamma space convertion
	for (int i = 0; i < 1024; i++)
	{
		lightgammatable[i] = i;
	}
}

//Valve called glEnableClientState(GL_VERTEX_ARRAY) and forgot to disable it.

void __fastcall enginesurface_drawFlushText(void *pthis, int dummy)
{
	gRefFuncs.enginesurface_drawFlushText(pthis, dummy);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}