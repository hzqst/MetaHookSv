#include "gl_local.h"
#include "MurmurHash2.h"
#include <ScopeExit/ScopeExit.h>
#include <FreeImage.h>
#include <strtools.h>

// Definitions for the translation and pixel arrays, presumably for texture processing.
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

gltexture_t *gltextures_get()
{
	return (gltextures_SvEngine) ? (*gltextures_SvEngine) : gltextures;
}

typedef struct
{
	const char *name;
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

int GL_GetAnsioValue()
{
	//Not registered yet?
	if (!gl_ansio)
		return gl_max_ansio;

	return max(min(gl_ansio->value, gl_max_ansio), 1);
}

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
			int iTextureTarget = GL_GetTextureTargetFromTextureEntry(&pgltextures[j]);

			if (iTextureTarget == GL_TEXTURE_2D_ARRAY)
			{
				glBindTexture(GL_TEXTURE_2D_ARRAY, pgltextures[j].texnum);

				if (pgltextures[j].mipmap)
				{
					glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, (*gl_filter_min));
					glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, (*gl_filter_max));
					glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY, GL_GetAnsioValue());
				}
				else
				{

					glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, (*gl_filter_max));
					glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, (*gl_filter_max));
					glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY, GL_GetAnsioValue());
				}

				glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			}
			else if (iTextureTarget == GL_TEXTURE_2D)
			{
				GL_Bind(pgltextures[j].texnum);

				if (pgltextures[j].mipmap)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (*gl_filter_min));
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (*gl_filter_max));
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, GL_GetAnsioValue());
				}
				else
				{

					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (*gl_filter_max));
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (*gl_filter_max));
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, GL_GetAnsioValue());
				}

				GL_Bind(0);
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
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, GL_GetAnsioValue());

			GL_Bind(0);
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
#define FORMAT_TEXTURE_IDENTIFIER(Ty, Name) if (textureType == Ty)\
	{\
		snprintf(hashedIdentifier, len, "@" Name "_%08X", MurmurHash2(identifier, strlen(identifier), textureType));\
		return;\
	}

	FORMAT_TEXTURE_IDENTIFIER(GLT_SYSTEM, "SYS");
	FORMAT_TEXTURE_IDENTIFIER(GLT_DECAL, "DCL");
	FORMAT_TEXTURE_IDENTIFIER(GLT_HUDSPRITE, "HSP");
	FORMAT_TEXTURE_IDENTIFIER(GLT_STUDIO, "MDL");
	FORMAT_TEXTURE_IDENTIFIER(GLT_WORLD, "BSP");
	FORMAT_TEXTURE_IDENTIFIER(GLT_SPRITE, "SPR");
	FORMAT_TEXTURE_IDENTIFIER(GLT_DETAIL, "DET");

#undef FORMAT_TEXTURE_IDENTIFIER

	//Default case
	strncpy(hashedIdentifier, identifier, len);
}

void GL_GenerateHashedTextureIndentifier2(const char* identifier, GL_TEXTURETYPE textureType, int width, int height, char * hashedIdentifier, size_t len)
{
#define FORMAT_TEXTURE_IDENTIFIER(Ty, Name) if (textureType == Ty)\
	{\
		snprintf(hashedIdentifier, len, "@" Name "_%08X_%04X_%04X", MurmurHash2(identifier, strlen(identifier), textureType), width, height);\
		return;\
	}

	FORMAT_TEXTURE_IDENTIFIER(GLT_SYSTEM, "SYS");
	FORMAT_TEXTURE_IDENTIFIER(GLT_DECAL, "DCL");
	FORMAT_TEXTURE_IDENTIFIER(GLT_HUDSPRITE, "HSP");
	FORMAT_TEXTURE_IDENTIFIER(GLT_STUDIO, "MDL");
	FORMAT_TEXTURE_IDENTIFIER(GLT_WORLD, "BSP");
	FORMAT_TEXTURE_IDENTIFIER(GLT_SPRITE, "SPR");
	FORMAT_TEXTURE_IDENTIFIER(GLT_DETAIL, "DET");
	
#undef FORMAT_TEXTURE_IDENTIFIER

	//Default case
	strncpy(hashedIdentifier, identifier, len);
}

void GL_GenerateHashedTextureIndentifier3(const char* identifier, GL_TEXTURETYPE textureType, int width, int height, int numframes, int frameduration, char* hashedIdentifier, size_t len)
{
#define FORMAT_TEXTURE_IDENTIFIER(Ty, Name) if (textureType == Ty)\
	{\
		snprintf(hashedIdentifier, len, "@" Name "_%08X_%04X_%04X_%04X_%04X", MurmurHash2(identifier, strlen(identifier), textureType), width, height, numframes, frameduration);\
		return;\
	}

	FORMAT_TEXTURE_IDENTIFIER(GLT_SYSTEM, "SYS");
	FORMAT_TEXTURE_IDENTIFIER(GLT_DECAL, "DCL");
	FORMAT_TEXTURE_IDENTIFIER(GLT_HUDSPRITE, "HSP");
	FORMAT_TEXTURE_IDENTIFIER(GLT_STUDIO, "MDL");
	FORMAT_TEXTURE_IDENTIFIER(GLT_WORLD, "BSP");
	FORMAT_TEXTURE_IDENTIFIER(GLT_SPRITE, "SPR");
	FORMAT_TEXTURE_IDENTIFIER(GLT_DETAIL, "DET");

#undef FORMAT_TEXTURE_IDENTIFIER

	//Default case
	strncpy(hashedIdentifier, identifier, len);
}

/*
	Purpose : Parse texture's identifier. Get textureType, and loadResult
*/

GL_TEXTURETYPE GL_ParseTextureIdentifier(const char* identifier, gl_loadtexture_result_t *loadResult)
{
	int hash = 0, width = 0, height = 0, numframes = 0, frameduration = 0;

	auto Ty = GL_GetTextureTypeFromTextureIdentifier(identifier);

	if (Ty != GLT_UNKNOWN)
	{
		auto len = strlen(identifier);

		if (len == sizeof("@SYS_12345678_1234_1234_1234_1234") - 1)
		{
			if (sscanf(identifier + sizeof("@SYS_") - 1, "%08X_%04X_%04X_%04X_%04X", &hash, &width, &height, &numframes, &frameduration) == 5)
			{
				loadResult->width = width;
				loadResult->height = height;
				loadResult->numframes = numframes;
				loadResult->frameduration = frameduration;
			}
		}
		else if (len == sizeof("@SYS_12345678_1234_1234") - 1)
		{
			if (sscanf(identifier + sizeof("@SYS_") - 1, "%08X_%04X_%04X", &hash, &width, &height) == 3)
			{
				loadResult->width = width;
				loadResult->height = height;
				loadResult->numframes = 0;
				loadResult->frameduration = 0;
			}
		}
	}
	
	return Ty;
}

GL_TEXTURETYPE GL_GetTextureTypeFromTextureIdentifier(const char *identifier)
{
	if (identifier[0] == '@')
	{
		if (identifier[1] == 'S' &&
			identifier[2] == 'Y' &&
			identifier[3] == 'S' &&
			identifier[4] == '_')
		{
			return GLT_SYSTEM;
		}

		if (identifier[1] == 'D' &&
			identifier[2] == 'C' &&
			identifier[3] == 'L' &&
			identifier[4] == '_')
		{
			return GLT_DECAL;
		}

		if (identifier[1] == 'S' &&
			identifier[2] == 'P' &&
			identifier[3] == 'H' &&
			identifier[4] == '_')
		{
			return GLT_HUDSPRITE;
		}

		if (identifier[1] == 'M' &&
			identifier[2] == 'D' &&
			identifier[3] == 'L' &&
			identifier[4] == '_')
		{
			return GLT_STUDIO;
		}

		if (identifier[1] == 'B' &&
			identifier[2] == 'S' &&
			identifier[3] == 'P' &&
			identifier[4] == '_')
		{
			return GLT_WORLD;
		}

		if (identifier[1] == 'S' &&
			identifier[2] == 'P' &&
			identifier[3] == 'R' &&
			identifier[4] == '_')
		{
			return GLT_SPRITE;
		}
	}

	return GLT_UNKNOWN;
}

int GL_GetTextureTargetFromTextureIdentifier(const char *identifier)
{
	int iTextureTarget = GL_TEXTURE_2D;

	//Make sure it's animated texture
	if (GL_GetTextureTypeFromTextureIdentifier(identifier) != GLT_UNKNOWN && strlen(identifier) == sizeof("@SYS_12345678_1234_1234_1234_1234") - 1)
	{
		iTextureTarget = GL_TEXTURE_2D_ARRAY;
	}

	return iTextureTarget;
}

int GL_GetTextureTargetFromTextureEntry(const gltexture_t* glt)
{
	return GL_GetTextureTargetFromTextureIdentifier(glt->identifier);
}

//GL Start

GLuint GL_GenTexture(void)
{
	GLuint tex = 0;
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

void GL_UploadSubDataToUBO(GLuint UBO, size_t offset, size_t size, const void* data)
{
	if (glNamedBufferSubData)
	{
		glNamedBufferSubData(UBO, offset, size, data);
	}
	else
	{
		glBindBuffer(GL_UNIFORM_BUFFER, UBO);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}

void GL_UploadDataToVBOStaticDraw(GLuint VBO, size_t size, const void* data)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	if (glBufferStorage)
	{
		glBufferStorage(GL_ARRAY_BUFFER, size, data, 0);
	}
	else
	{
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GL_UploadDataToVBODynamicDraw(GLuint VBO, size_t size, const void* data)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GL_UploadSubDataToVBODynamicDraw(GLuint VBO, size_t offset, size_t size, const void* data)
{
	if (glNamedBufferSubData)
	{
		glNamedBufferSubData(VBO, offset, size, data);
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void GL_UploadDataToEBOStaticDraw(GLuint EBO, size_t size, const void* data)
{
	//TODO: what if size == 0 ?

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	if (glBufferStorage)
	{
		glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, size, data, 0);
	}
	else
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GL_UploadDataToEBODynamicDraw(GLuint EBO, size_t size, const void* data)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
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
	gPrivateFuncs.GL_Bind(texnum);
}

void GL_SelectTexture(GLenum target)
{
	gPrivateFuncs.GL_SelectTexture(target);
}

void GL_DisableMultitexture(void)
{
	gPrivateFuncs.GL_DisableMultitexture();
}

void GL_EnableMultitexture(void)
{
	gPrivateFuncs.GL_EnableMultitexture();
}

void GL_UnloadTextureByIdentifier(const char* identifier, bool notify_callback)
{
	int i;
	gltexture_t* glt;

	if (identifier[0] == '@')
	{
		for (i = 0, glt = gltextures_get(); i < (*numgltextures); i++, glt++)
		{
			if (glt->texnum > 0 && !strncmp(glt->identifier, identifier, sizeof("@SPR_DEADBEEF") - 1))
			{
				GL_FreeTextureEntry(glt, notify_callback);
				return;
			}
		}
	}
	else
	{
		for (i = 0, glt = gltextures_get(); i < (*numgltextures); i++, glt++)
		{
			if (glt->texnum > 0 && !strcmp(glt->identifier, identifier))
			{
				GL_FreeTextureEntry(glt, notify_callback);
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
			GL_FreeTextureEntry(glt, true);
		}
	}
}

void GL_UnloadTextureWithType(const char* identifier, GL_TEXTURETYPE textureType, bool notify_callback)
{
	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier(identifier, textureType, hashedIdentifier, sizeof(hashedIdentifier));

	GL_UnloadTextureByIdentifier(hashedIdentifier, notify_callback);
}

void GL_UnloadTextureWithType(const char* identifier, GL_TEXTURETYPE textureType, int width, int height, bool notify_callback)
{
	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier2(identifier, textureType, width, height, hashedIdentifier, sizeof(hashedIdentifier));

	GL_UnloadTextureByIdentifier(hashedIdentifier, notify_callback);
}

void GL_UnloadTextureByTextureId(int gltexturenum, bool notify_callback)
{
	int i;
	gltexture_t* glt;

	for (i = 0, glt = gltextures_get(); i < (*numgltextures); i++, glt++)
	{
		if (glt->texnum == gltexturenum)
		{
			GL_FreeTextureEntry(glt, notify_callback);
			break;
		}
	}
}

bool GL_IsSupportedInternalFormat(GLuint internalFormat)
{
	switch (internalFormat)
	{
	case GL_RGBA8:
		return true;
	case GL_RGB8:
		return true;
	case GL_RGBA32F:
		return true;
	case GL_RGB32F:
		return true;
	}

	return false;
}

GLuint GL_GetTextureFormatFromInternalFormat(GLuint internalFormat)
{
	switch (internalFormat)
	{
	case GL_RGBA8:
		return GL_RGBA;
	case GL_RGB8:
		return GL_RGB;
	case GL_RGBA32F:
		return GL_RGBA;
	case GL_RGB32F:
		return GL_RGB;
	}

	return 0;
}

GLuint GL_GetTextureColorTypeFromInternalFormat(GLuint internalFormat)
{
	switch (internalFormat)
	{
	case GL_RGBA8:
		return GL_UNSIGNED_BYTE;
	case GL_RGB8:
		return GL_UNSIGNED_BYTE;
	case GL_RGBA32F:
		return GL_FLOAT;
	case GL_RGB32F:
		return GL_FLOAT;
	}

	return 0;
}

int GL_GetTextureTargetFromLoadTextureContext(const gl_loadtexture_context_t* context)
{
	int iTextureTarget = GL_TEXTURE_2D;

	if (context->cubemap)
	{
		iTextureTarget = GL_TEXTURE_CUBE_MAP;
	}
	else if (context->numframes)
	{
		iTextureTarget = GL_TEXTURE_2D_ARRAY;
	}

	return iTextureTarget;
}

int GL_GetMipmapTextureTargetFromLoadTextureContext(const gl_loadtexture_context_t* context)
{
	int iMipmapTextureTarget = GL_TEXTURE_2D;

	if (context->cubemap)
	{
		iMipmapTextureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + context->cubemap - 1;
	}
	else if (context->numframes)
	{
		iMipmapTextureTarget = GL_TEXTURE_2D_ARRAY;
	}

	return iMipmapTextureTarget;
}

void GL_UploadTexturePreCommon(gl_loadtexture_context_t* context, int iTextureTarget)
{
	//Disable auto-mipmap-generation.
	glTexParameteri(iTextureTarget, GL_GENERATE_MIPMAP, GL_FALSE);

	if (context->mipmap)
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MIN_FILTER, (*gl_filter_min));
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAG_FILTER, (*gl_filter_max));
	}
	else if (context->filter)
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MIN_FILTER, context->filter);
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAG_FILTER, context->filter);
	}
	else
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MIN_FILTER, (*gl_filter_max));
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAG_FILTER, (*gl_filter_max));
	}

	if (context->wrap)
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_WRAP_S, context->wrap);
		glTexParameteri(iTextureTarget, GL_TEXTURE_WRAP_T, context->wrap);
	}

	glTexParameteri(iTextureTarget, GL_TEXTURE_MAX_ANISOTROPY, GL_GetAnsioValue());

	glTexParameteri(iTextureTarget, GL_TEXTURE_BASE_LEVEL, 0);

	if (context->mipmap)
	{
		//There are more than one mipmaps
		if (context->mipmaps.size() > 1)
		{
			glTexParameteri(iTextureTarget, GL_TEXTURE_MAX_LEVEL, context->mipmaps.size() - 1);
		}
		else
		{
			glTexParameteri(iTextureTarget, GL_TEXTURE_MAX_LEVEL, 1000);
		}
	}
	else
	{
		glTexParameteri(iTextureTarget, GL_TEXTURE_MAX_LEVEL, 0);
	}
}

void GL_UploadTexturePostCommon(gl_loadtexture_context_t* context, int iTextureTarget)
{
	if (context->numframes)
	{
		if (context->mipmap)
		{
			glGenerateMipmap(iTextureTarget);
		}
	}
	else
	{
		//Generate mipmaps if there isn't one
		if (context->mipmaps.size() == 1 && context->mipmap)
		{
			//available since OpenGL 3.0
			glGenerateMipmap(iTextureTarget);
		}
	}

	//Free containers owning mipmap data.
	if (context->mipmaps_dtor)
	{
		for (size_t i = 0; i < context->mipmaps.size(); ++i)
		{
			context->mipmaps_dtor(context->mipmaps[i].data_ctx, context->mipmaps[i].data, context->mipmaps[i].size);
		}
		context->mipmaps.clear();
	}
}

void GL_UploadCompressedTexture(gl_loadtexture_context_t * context, int iTextureTarget)
{
	GL_UploadTexturePreCommon(context, iTextureTarget);

	int iMipmapTextureTarget = GL_GetMipmapTextureTargetFromLoadTextureContext(context);

	if (context->numframes)
	{
		//TODO: Not supported yet
	}
	else
	{
		for (size_t i = 0; i < context->mipmaps.size(); ++i)
		{
			glCompressedTexImage2D(
				iMipmapTextureTarget, 
				context->mipmaps[i].level,
				context->internalformat,
				context->mipmaps[i].width, 
				context->mipmaps[i].height, 
				0, 
				context->mipmaps[i].size,
				context->mipmaps[i].data);

			if (!context->mipmap)
				break;
		}
	}

	GL_UploadTexturePostCommon(context, iTextureTarget);
}

void GL_UploadUncompressedTexture(gl_loadtexture_context_t* context, int iTextureTarget)
{
	GL_UploadTexturePreCommon(context, iTextureTarget);

	int iMipmapTextureTarget = GL_GetMipmapTextureTargetFromLoadTextureContext(context);

	if (context->numframes)
	{
		if (GL_IsSupportedInternalFormat(context->internalformat))
		{
			glTexStorage3D(iTextureTarget, 1, context->internalformat, context->width, context->height, context->numframes);

			for (size_t i = 0; i < context->mipmaps.size(); ++i)
			{
				glTexSubImage3D(
					iMipmapTextureTarget,
					context->mipmaps[i].level, //level
					0, //xoffset
					0, //yoffset
					i, //zoffset
					context->mipmaps[i].width,
					context->mipmaps[i].height,
					1,
					GL_GetTextureFormatFromInternalFormat(context->internalformat),
					GL_GetTextureColorTypeFromInternalFormat(context->internalformat),
					context->mipmaps[i].data);
			}
		}
		else
		{
			g_pMetaHookAPI->SysError("GL_UploadUncompressedTexture: bogus internalformat 0x%X.", context->internalformat);
		}
	}
	else
	{
		if (GL_IsSupportedInternalFormat(context->internalformat))
		{
			for (size_t i = 0; i < context->mipmaps.size(); ++i)
			{
				glTexImage2D(iMipmapTextureTarget,
					context->mipmaps[i].level,
					context->internalformat,
					context->mipmaps[i].width,
					context->mipmaps[i].height,
					0,
					GL_GetTextureFormatFromInternalFormat(context->internalformat),
					GL_GetTextureColorTypeFromInternalFormat(context->internalformat),
					context->mipmaps[i].data);

				//Only upload mipmap[0] if mipmap is off
				if (!context->mipmap)
					break;
			}
		}
		else
		{
			g_pMetaHookAPI->SysError("GL_UploadUncompressedTexture: bogus internalformat 0x%X.", context->internalformat);
		}
	}

	GL_UploadTexturePostCommon(context, iTextureTarget);
}

/*
	Search for glt and returns texnum, The identifier must be matched
*/

int GL_FindTextureEx(const char *identifier, GL_TEXTURETYPE textureType, int *width, int *height)
{
	int i;
	gltexture_t *slot;

	if (identifier[0])
	{
		if (identifier[0] == '@')
		{
			for (i = 0, slot = gltextures_get(); i < (*numgltextures); i++, slot++)
			{
				if (!strncmp(slot->identifier, identifier, sizeof("@SPR_DEADBEEF") - 1))
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
			for (i = 0, slot = gltextures_get(); i < (*numgltextures); i++, slot++)
			{
				if (!strcmp(identifier, slot->identifier))
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
	}

	return 0;
}

/*
	Search for glt and returns glt entry, The identifier must be matched
*/
gltexture_t* GL_FindTextureEntryEx(const char* identifier, GL_TEXTURETYPE textureType)
{
	int i;
	gltexture_t* slot;

	if (identifier[0])
	{
		if (identifier[0] == '@')
		{
			for (i = 0, slot = gltextures_get(); i < (*numgltextures); i++, slot++)
			{
				if (!strncmp(slot->identifier, identifier, sizeof("@SPR_DEADBEEF") - 1))
				{
					if (textureType == GLT_WORLD)
					{
						if (slot->servercount != *gHostSpawnCount)
							continue;
					}

					return slot;
				}
			}
		}
		else
		{
			for (i = 0, slot = gltextures_get(); i < (*numgltextures); i++, slot++)
			{
				if (!strcmp(identifier, slot->identifier))
				{
					if (textureType == GLT_WORLD)
					{
						if (slot->servercount != *gHostSpawnCount)
							continue;
					}

					return slot;
				}
			}
		}
	}

	return 0;
}

/*
	Search for the specified glt entry and returns glt->texnum, The identifier, textureType, width and height must be matched
*/
int GL_FindTextureEx2(const char* identifier, GL_TEXTURETYPE textureType, int width, int height)
{
	int i;
	gltexture_t* slot;

	if (identifier[0])
	{
		if (identifier[0] == '@')
		{
			for (i = 0, slot = gltextures_get(); i < (*numgltextures); i++, slot++)
			{
				if (!strncmp(identifier, slot->identifier, sizeof("@SPR_DEADBEEF") - 1) && width == slot->width && height == slot->height)
				{
					if (textureType == GLT_WORLD)
					{
						if (slot->servercount != *gHostSpawnCount)
							continue;
					}

					return slot->texnum;
				}
			}
		}
		else
		{
			for (i = 0, slot = gltextures_get(); i < (*numgltextures); i++, slot++)
			{
				if (!strcmp(identifier, slot->identifier) && width == slot->width && height == slot->height)
				{
					if (textureType == GLT_WORLD)
					{
						if (slot->servercount != *gHostSpawnCount)
							continue;
					}

					return slot->texnum;
				}
			}
		}
	}

	return 0;
}

/*
	Search for the specified glt entry and returns glt, The identifier, textureType, width and height must be matched
*/
gltexture_t * GL_FindTextureEntryEx2(const char* identifier, GL_TEXTURETYPE textureType, int width, int height)
{
	int i;
	gltexture_t* slot;

	if (identifier[0])
	{
		if (identifier[0] == '@')
		{
			for (i = 0, slot = gltextures_get(); i < (*numgltextures); i++, slot++)
			{
				if (!strncmp(slot->identifier, identifier, sizeof("@SPR_DEADBEEF") - 1) && width == slot->width && height == slot->height)
				{
					if (textureType == GLT_WORLD)
					{
						if (slot->servercount != *gHostSpawnCount)
							continue;
					}

					return slot;
				}
			}
		}
		else
		{
			for (i = 0, slot = gltextures_get(); i < (*numgltextures); i++, slot++)
			{
				if (!strcmp(identifier, slot->identifier) && width == slot->width && height == slot->height)
				{
					if (textureType == GLT_WORLD)
					{
						if (slot->servercount != *gHostSpawnCount)
							continue;
					}

					return slot;
				}
			}
		}
	}

	return 0;
}

/*
	Search for the specified glt entry and returns glt->texnum, The raw-identifier and textureType must be matched
*/

int GL_FindTexture(const char* identifier, GL_TEXTURETYPE textureType, int* width, int* height)
{
	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier(identifier, textureType, hashedIdentifier, sizeof(hashedIdentifier));

	auto foundTexture = GL_FindTextureEx(hashedIdentifier, textureType, width, height);

	return foundTexture;
}

/*
	Search for the specified glt entry and returns glt, The raw-identifier and textureType must be matched
*/

gltexture_t *GL_FindTextureEntry(const char* identifier, GL_TEXTURETYPE textureType)
{
	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier(identifier, textureType, hashedIdentifier, sizeof(hashedIdentifier));

	auto foundGLT = GL_FindTextureEntryEx(hashedIdentifier, textureType);

	return foundGLT;
}

/*
	Search for the specified glt entry and returns glt->texnum, The raw-identifier, textureType, width and height must be matched
*/

int GL_FindTexture2(const char* identifier, GL_TEXTURETYPE textureType, int width, int height)
{
	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier(identifier, textureType, hashedIdentifier, sizeof(hashedIdentifier));

	auto foundTexture = GL_FindTextureEx2(hashedIdentifier, textureType, width, height);

	return foundTexture;
}

/*
	Search for the specified glt entry, The raw-identifier, textureType, width and height must be matched
*/

gltexture_t*GL_FindTextureEntry2(const char* identifier, GL_TEXTURETYPE textureType, int width, int height)
{
	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier(identifier, textureType, hashedIdentifier, sizeof(hashedIdentifier));

	auto foundGLT = GL_FindTextureEntryEx2(hashedIdentifier, textureType, width, height);

	return foundGLT;
}

/*
	Allocate an empty glt entry, or returns an existing one if identifier matched
*/

gltexture_t * GL_AllocTextureEntry(const char* identifier, GL_TEXTURETYPE textureType, int width, int height, qboolean mipmap, bool *foundExisting)
{
	int i;
	gltexture_t* glt;
	gltexture_t* slot;

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

				if (foundExisting)
					*foundExisting = true;

				return slot;
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
				*gltextures_SvEngine = (gltexture_t*)gPrivateFuncs.realloc_SvEngine((void*)(*gltextures_SvEngine), (*maxgltextures_SvEngine) * sizeof(gltexture_t));
			}
		}
		else
		{
			if ((*numgltextures) + 1 >= EngineGetMaxGLTextures())
			{
				g_pMetaHookAPI->SysError("Texture Overflow: MAX_GLTEXTURES\n");
				return NULL;
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

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		glt->servercount = (textureType != GLT_SYSTEM && textureType != GLT_DECAL && textureType != GLT_HUDSPRITE) ? (*gHostSpawnCount) : 0;
	}
	else
	{
		glt->servercount = (textureType == GLT_WORLD) ? (*gHostSpawnCount) : 0;
	}

	glt->paletteIndex = -1;

	if (foundExisting)
		*foundExisting = false;

	return glt;
}

int GL_AllocTexture(const char* identifier, GL_TEXTURETYPE textureType, int width, int height, qboolean mipmap, bool* foundExisting)
{
	auto glt = GL_AllocTextureEntry(identifier, textureType, width, height, mipmap, foundExisting);

	if (!glt)
		return 0;

	return glt->texnum;
}

void GL_BoxFilter3x3(unsigned char* out, unsigned char* in, int w, int h, int x, int y)
{
	int				i, j;
	int				a = 0;
	int				r = 0, g = 0, b = 0;
	int				count = 0;
	int				acount = 0;
	int				u, v;
	unsigned char* pixel;

	for (i = 0; i < 3; i++)
	{
		u = (i - 1) + x;

		for (j = 0; j < 3; j++)
		{
			v = (j - 1) + y;

			if (u >= 0 && u < w && v >= 0 && v < h)
			{
				pixel = &in[(u + v * w) * 4];

				if (pixel[3] != 0)
				{
					r += pixel[0];
					g += pixel[1];
					b += pixel[2];
					a += pixel[3];
					acount++;
				}
			}
		}
	}

	if (acount == 0)
		acount = 1;

	out[0] = r / acount;
	out[1] = g / acount;
	out[2] = b / acount;
	out[3] = 0;
}

void GL_ProcessMipmap32(int iPalTextureType, gl_loadtexture_context_t* context)
{
	for (size_t i = 0; i < context->mipmaps.size(); ++i)
	{
		if (gl_spriteblend && gl_spriteblend->value)
		{
			if (g_iEngineType == ENGINE_SVENGINE)
			{
				if (iPalTextureType == TEX_TYPE_ALPHA_SVENGINE || iPalTextureType == TEX_TYPE_ALPHA_GRADIENT_SVENGINE || iPalTextureType == TEX_TYPE_RGBA_SVENGINE)
				{
					int width = context->mipmaps[i].width;
					int height = context->mipmaps[i].height;
					unsigned int* pData = (unsigned int*)context->mipmaps[i].data;

					for (int i = 0; i < width * height; i++)
					{
						if (pData[i] == 0)
						{
							GL_BoxFilter3x3((unsigned char*)&pData[i], (unsigned char*)pData, width, height, i % width, i / width);
						}
					}
				}
			}
			else
			{
				if (iPalTextureType == TEX_TYPE_ALPHA || iPalTextureType == TEX_TYPE_ALPHA_GRADIENT || iPalTextureType == TEX_TYPE_RGBA)
				{
					int width = context->mipmaps[i].width;
					int height = context->mipmaps[i].height;
					unsigned int* pData = (unsigned int*)context->mipmaps[i].data;

					for (int i = 0; i < width * height; i++)
					{
						if (pData[i] == 0)
						{
							GL_BoxFilter3x3((unsigned char*)&pData[i], (unsigned char*)pData, width, height, i % width, i / width);
						}
					}
				}
			}
		}
	}
}

void GL_Upload16ToMipmap(byte* pData, int width, int height, byte* pPal, int iPalTextureType, gl_loadtexture_context_t* context)
{
	// Calculate the size of the image data.
	int imageSize = width * height * sizeof(unsigned int);

	// Check if the image size exceeds the maximum allowed size.
	if (imageSize > sizeof(texloader_buffer)) {
		gEngfuncs.Con_Printf(
			"GL_Upload16ToMipmap: Texture too large! (%d x %d = %d B, max. size is %d B)\n",
			width, height, imageSize, sizeof(texloader_buffer));
		return;
	}

	auto pPalette = (const byte*)pPal;
	auto pInputBytes = (const byte*)pData;
	auto pOutputPixels = (unsigned int*)texloader_buffer;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		bool isTransparent = true;
		for (int i = 0; i < height * width; ++i) {

			switch (iPalTextureType) {
			case TEX_TYPE_NONE:
			{
				if (gl_dither && gl_dither->value)
				{
					auto index = pInputBytes[i];
					auto pb = (byte*)&pOutputPixels[i];
					auto ppix = &pPalette[index * 3];
					auto r = ppix[0] | (ppix[0] >> 6);
					auto g = ppix[1] | (ppix[1] >> 6);
					auto b = ppix[2] | (ppix[2] >> 6);

					pb[0] = r;
					pb[1] = g;
					pb[2] = b;
					pb[3] = 255;
				}
				else
				{
					auto index = pInputBytes[i];
					auto pb = (byte*)&pOutputPixels[i];
					pb[0] = pPal[index * 3 + 0];
					pb[1] = pPal[index * 3 + 1];
					pb[2] = pPal[index * 3 + 2];
					pb[3] = 255;
				}
				break;
			}
			case TEX_TYPE_ALPHA_SVENGINE:
			case TEX_TYPE_ALPHA_GRADIENT_SVENGINE:
			case TEX_TYPE_RGBA_SVENGINE:
			{
				byte index = pInputBytes[i];

				if (iPalTextureType == TEX_TYPE_ALPHA_GRADIENT_SVENGINE) {
					pOutputPixels[i] = (index << 24) | ((*(unsigned int*)&pPalette[765]) & 0xFFFFFF);
				}
				else if (iPalTextureType == TEX_TYPE_RGBA_SVENGINE) {
					pOutputPixels[i] = (index << 24) | ((*(unsigned int*)&pPalette[3 * index]) & 0xFFFFFF);
				}
				else if (index == 255) {
					pOutputPixels[i] = 0; // Fully transparent.
				}
				else {
					pOutputPixels[i] = (*(unsigned int*)&pPalette[3 * index]) | 0xFF000000; // Full alpha.
				}
				break;
			}
			default:
			{
				// If the type is not recognized, throw an error.
				g_pMetaHookAPI->SysError("GL_Upload16ToMipmap: Bogus texture type!\n");
				break;
			}
			}
		}
	}
	else
	{
		bool isTransparent = true;
		for (int i = 0; i < height * width; ++i) {

			switch (iPalTextureType) {
			case TEX_TYPE_NONE:
			{
				if (gl_dither && gl_dither->value)
				{
					auto index = pInputBytes[i];
					auto pb = (byte*)&pOutputPixels[i];
					auto ppix = &pPalette[index * 3];
					auto r = ppix[0] | (ppix[0] >> 6);
					auto g = ppix[1] | (ppix[1] >> 6);
					auto b = ppix[2] | (ppix[2] >> 6);

					pb[0] = r;
					pb[1] = g;
					pb[2] = b;
					pb[3] = 255;
				}
				else
				{
					auto index = pInputBytes[i];
					auto pb = (byte*)&pOutputPixels[i];
					pb[0] = pPal[index * 3 + 0];
					pb[1] = pPal[index * 3 + 1];
					pb[2] = pPal[index * 3 + 2];
					pb[3] = 255;
				}
				break;
			}
			case TEX_TYPE_ALPHA:
			case TEX_TYPE_ALPHA_GRADIENT:
			case TEX_TYPE_RGBA:
			{
				byte index = pInputBytes[i];

				if (iPalTextureType == TEX_TYPE_ALPHA_GRADIENT) {
					pOutputPixels[i] = (index << 24) | ((*(unsigned int*)&pPalette[765]) & 0xFFFFFF);
				}
				else if (iPalTextureType == TEX_TYPE_RGBA) {
					pOutputPixels[i] = (index << 24) | ((*(unsigned int*)&pPalette[3 * index]) & 0xFFFFFF);
				}
				else if (index == 255) {
					pOutputPixels[i] = 0; // Fully transparent.
				}
				else {
					pOutputPixels[i] = (*(unsigned int*)&pPalette[3 * index]) | 0xFF000000; // Full alpha.
				}
				break;
			}
			default:
			{
				// If the type is not recognized, throw an error.
				g_pMetaHookAPI->SysError("GL_Upload16ToMipmap: Bogus texture type!\n");
				break;
			}
			}
		}
	}

	context->width = width;
	context->height = height;

	context->mipmaps.emplace_back(0, pOutputPixels, imageSize, context->width, context->height);

	GL_ProcessMipmap32(iPalTextureType, context);
}

void GL_Upload32ToMipmap(byte* pData, int width, int height, int iPalTextureType, gl_loadtexture_context_t* context)
{
	auto imageSize = width * height * 4;

	context->mipmaps.emplace_back(0, pData, imageSize, width, height);

	GL_ProcessMipmap32(iPalTextureType, context);
}

void GL_UploadTexture(gltexture_t* glt, GL_TEXTURETYPE textureType, gl_loadtexture_context_t* context)
{
	int iTextureTarget = GL_GetTextureTargetFromLoadTextureContext(context);

	if (!context->wrap)
	{
		context->wrap = GL_REPEAT;

		if (textureType == GLT_HUDSPRITE || textureType == GLT_SPRITE)
			context->wrap = GL_CLAMP_TO_EDGE;
	}

	glBindTexture(iTextureTarget, glt->texnum);

	if (context->compressed)
	{
		GL_UploadCompressedTexture(context, iTextureTarget);
	}
	else
	{
		GL_UploadUncompressedTexture(context, iTextureTarget);
	}

	glBindTexture(iTextureTarget, 0);
}

int GL_LoadTexture2(char* identifier, GL_TEXTURETYPE textureType, int width, int height, byte* data, qboolean mipmap, int iPalTextureType, byte* pPal, int filter)
{
	char hashedIdentifier[64] = { 0 };
	GL_GenerateHashedTextureIndentifier2(identifier, textureType, width, height, hashedIdentifier, sizeof(hashedIdentifier));

	if (bUseLegacyTextureLoader)
	{
		int gltexturenum = gPrivateFuncs.GL_LoadTexture2(hashedIdentifier, textureType, width, height, data, mipmap, iPalTextureType, pPal, filter);

		gEngfuncs.Con_DPrintf("GL_LoadTexture2: Using legacy texture loader [%s] -> [%s] [%d]\n", identifier, hashedIdentifier, gltexturenum);

		return gltexturenum;
	}

	bool foundExisting = false;

	auto glt = GL_AllocTextureEntry(hashedIdentifier, textureType, width, height, mipmap, &foundExisting);

	if (!glt)
	{
		gEngfuncs.Con_Printf("GL_LoadTexture2: Failed to allocate texture entry for [%s] -> [%s].\n", identifier, hashedIdentifier);
		return 0;
	}

	if (foundExisting)
	{
		gEngfuncs.Con_DPrintf("GL_LoadTexture2: Found existing texture entry [%s] -> [%s] [%d]\n", identifier, hashedIdentifier, glt->texnum);
		return glt->texnum;
	}

	gEngfuncs.Con_DPrintf("GL_LoadTexture2: Using new texture loader [%s] -> [%s] [%d]\n", identifier, hashedIdentifier, glt->texnum);

	gl_loadtexture_context_t context;

	context.internalformat = GL_RGBA8;
	context.compressed = false;
	context.width = width;
	context.height = height;
	context.mipmap = mipmap;
	context.filter = filter;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		if (!pPal)
		{
			GL_Upload32ToMipmap(data, width, height, iPalTextureType, &context);
		}
		else
		{
			GL_Upload16ToMipmap(data, width, height, pPal, iPalTextureType, &context);
		}
	}
	else
	{
		if (iPalTextureType == TEX_TYPE_RGBA && textureType == GLT_SPRITE)
		{
			GL_Upload32ToMipmap(data, width, height, iPalTextureType, &context);
		}
		else
		{
			GL_Upload16ToMipmap(data, width, height, pPal, iPalTextureType, &context);
		}
	}

	GL_UploadTexture(glt, textureType, &context);

	return glt->texnum;
}

int GL_LoadTexture(char* identifier, GL_TEXTURETYPE textureType, int width, int height, byte* data, qboolean mipmap, int iPalTextureType, byte* pPal)
{
	return GL_LoadTexture2(identifier, textureType, width, height, data, mipmap, iPalTextureType, pPal, (*gl_filter_max));
}

gltexture_t *GL_LoadTextureEx(const char *identifier, GL_TEXTURETYPE textureType, gl_loadtexture_context_t *context)
{
	if (!context->mipmaps.size())
	{
		gEngfuncs.Con_Printf("GL_LoadTextureEx: no mipmap data for %s.\n", identifier);
		return NULL;
	}

	char hashedIdentifier[64];

	if (context->numframes > 0)
	{
		GL_GenerateHashedTextureIndentifier3(identifier, textureType, context->width, context->height, context->numframes, context->frameduration, hashedIdentifier, sizeof(hashedIdentifier));
	}
	else
	{
		GL_GenerateHashedTextureIndentifier2(identifier, textureType, context->width, context->height, hashedIdentifier, sizeof(hashedIdentifier));
	}

	bool foundExisting = false;

	//Find or Allocate a legacy texture entry for this texture.
	auto glt = GL_AllocTextureEntry(hashedIdentifier, textureType, context->width, context->height, context->mipmap, &foundExisting);

	if (!glt)
	{
		gEngfuncs.Con_Printf("GL_LoadTextureEx: Failed to allocate texture entry for [%s] -> [%s].\n", identifier, hashedIdentifier);
		return NULL;
	}

	if (foundExisting)
	{
		gEngfuncs.Con_DPrintf("GL_LoadTextureEx: Found existing texture entry [%s] -> [%s] [%d]\n", identifier, hashedIdentifier, glt->texnum);
		return glt;
	}

	gEngfuncs.Con_DPrintf("GL_LoadTextureEx: [%s] -> [%s] [%d]\n", identifier, hashedIdentifier, glt->texnum);

	//Upload texture data to GPU.
	GL_UploadTexture(glt, textureType, context);

	return glt;
}

texture_t *Draw_DecalTexture(int index)
{
	texture_t *texture = gPrivateFuncs.Draw_DecalTexture(index);

	return texture;
}

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

		int iTexType = (g_iEngineType == ENGINE_SVENGINE) ? TEX_TYPE_ALPHA_SVENGINE : TEX_TYPE_ALPHA;

		tex->gl_texturenum = GL_LoadTexture(tex->name, GLT_DECAL, tex->width, tex->height, bitmap, true, iTexType, pal);
	}
	else
	{
		tex->name[0] = '}';

		if ((*gfCustomBuild))
			GL_UnloadTextureWithType(tex->name, GLT_DECAL, true);

		//Why'th fuck 2 in SvEngine?
		int iTexType = (g_iEngineType == ENGINE_SVENGINE) ? TEX_TYPE_ALPHA_GRADIENT_SVENGINE : TEX_TYPE_ALPHA_GRADIENT;

		tex->gl_texturenum = GL_LoadTexture(tex->name, GLT_DECAL, tex->width, tex->height, bitmap, true, iTexType, pal);
	}
}

bool LoadDDS(const char* filename, const char* pathId, gl_loadtexture_context_t * context)
{
	byte* buf = texloader_buffer;
	size_t bufsize = sizeof(texloader_buffer);

	DDS_FILEHEADER10 fileHeader10;

	FileHandle_t fileHandle = FILESYSTEM_ANY_OPEN(filename, "rb", pathId);

	if (!fileHandle)
	{
		return false;
	}

	SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(fileHandle); };

	if (sizeof(DDS_FILEHEADER) != FILESYSTEM_ANY_READ(&fileHeader10, sizeof(DDS_FILEHEADER), fileHandle))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);	
		return false;
	}

	if (fileHeader10.dwMagic != DDS_MAGIC)
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
		return false;
	}

	if (fileHeader10.Header.dwSize != 124)
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
		return false;
	}

	if (!(fileHeader10.Header.dwFlags & DDSD_PIXELFORMAT))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
		return false;
	}

	if (!(fileHeader10.Header.dwFlags & DDSD_CAPS))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
		return false;
	}

	if (!(fileHeader10.Header.ddspf.dwFlags & DDPF_FOURCC))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
		return false;
	}

	if (fileHeader10.Header.ddspf.dwFourCC != D3DFMT_DXT1 &&
		fileHeader10.Header.ddspf.dwFourCC != D3DFMT_DXT3 &&
		fileHeader10.Header.ddspf.dwFourCC != D3DFMT_DXT5 &&
		fileHeader10.Header.ddspf.dwFourCC != D3DFMT_BC7 &&
		fileHeader10.Header.ddspf.dwFourCC != D3DFMT_DX10)
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s has unsupported compressed texture format! Only DXT1/3/5 and BC7 supported!\n", filename);
		return false;
	}

	if (fileHeader10.Header.ddspf.dwFourCC == D3DFMT_DX10)
	{
		if (sizeof(DDS_HEADER_DXT10) != FILESYSTEM_ANY_READ(&fileHeader10.Header10, sizeof(DDS_HEADER_DXT10), fileHandle))
		{
			gEngfuncs.Con_Printf("LoadDDS: File %s is not a DDS image.\n", filename);
			return false;
		}

		FILESYSTEM_ANY_SEEK(fileHandle, sizeof(DDS_FILEHEADER10), FILESYSTEM_SEEK_HEAD);
	}
	else
	{
		FILESYSTEM_ANY_SEEK(fileHandle, sizeof(DDS_FILEHEADER), FILESYSTEM_SEEK_HEAD);
	}

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
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					return false;
				}

				context->internalformat = GL_COMPRESSED_RGBA_BPTC_UNORM;
				context->compressed = true;
				context->mipmaps.emplace_back(i, buf + offset, size, w, h);

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
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					return false;
				}

				context->internalformat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
				context->compressed = true;
				context->mipmaps.emplace_back(i, buf + offset, size, w, h);

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
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					return false;
				}

				context->internalformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				context->compressed = true;
				context->mipmaps.emplace_back(i, buf + offset, size, w, h);

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
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					return false;
				}

				context->internalformat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
				context->compressed = true;
				context->mipmaps.emplace_back(i, buf + offset, size, w, h);

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
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					return false;
				}

				context->internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				context->compressed = true;
				context->mipmaps.emplace_back(i, buf + offset, size, w, h);

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
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					return false;
				}

				context->internalformat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
				context->compressed = true;
				context->mipmaps.emplace_back(i, buf + offset, size, w, h);

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
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					return false;
				}

				context->internalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				context->compressed = true;
				context->mipmaps.emplace_back(i, buf + offset, size, w, h);

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
					return false;
				}

				if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
				{
					gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
					return false;
				}

				context->internalformat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
				context->compressed = true;
				context->mipmaps.emplace_back(i, buf + offset, size, w, h);

				offset += size;
				w = w >> 1;
				h = h >> 1;
			}
			else
			{
				gEngfuncs.Con_Printf("LoadDDS: File %s has unsupported compressed texture format! Only DXT1/3/5 and BC7 supported!\n", filename);
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
				return false;
			}

			if (size != FILESYSTEM_ANY_READ((void*)(buf + offset), size, fileHandle))
			{
				gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
				return false;
			}

			context->internalformat = GL_COMPRESSED_RGBA_BPTC_UNORM;
			context->compressed = true;
			context->mipmaps.emplace_back(i, buf + offset, size, w, h);

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
				return false;
			}

			if (size != FILESYSTEM_ANY_READ((void *)(buf + offset), size, fileHandle))
			{
				gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
				return false;
			}

			context->internalformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			context->compressed = true;
			context->mipmaps.emplace_back(i, buf + offset, size, w, h);

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
				return false;
			}

			if (size != FILESYSTEM_ANY_READ((void *)(buf + offset), size, fileHandle))
			{
				gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
				return false;
			}

			context->internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			context->compressed = true;
			context->mipmaps.emplace_back(i, buf + offset, size, w, h);

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
				return false;
			}
			
			if (size != FILESYSTEM_ANY_READ((void *)(buf + offset), size, fileHandle))
			{
				gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
				return false;
			}

			context->internalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			context->compressed = true;
			context->mipmaps.emplace_back(i, buf + offset, size, w, h);

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

	context->width = fileHeader10.Header.dwWidth;
	context->height = fileHeader10.Header.dwHeight;

	return context->callback(context);
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

bool LoadImageGenericRGBA32F(const char* filename, FIBITMAP* fiB, gl_loadtexture_context_t* context)
{
	size_t pos = 0;
	size_t w = FreeImage_GetWidth(fiB);
	size_t h = FreeImage_GetHeight(fiB);
	size_t imageSize = w * h * sizeof(vec4_t);

	auto imageData = FreeImage_GetBits(fiB);

	size_t rowSize = w * sizeof(vec4_t);

	FreeImage_FlipVertical(fiB);

	//Assume the texture to be SRGB
	float r_texgamma = 1.0f / v_texgamma->value;

	for (unsigned int y = 0; y < h; ++y) {
		for (unsigned int x = 0; x < w; ++x) {
			float* row = (float*)(imageData + y * rowSize + x * sizeof(vec4_t));

			row[0] = pow(row[0], r_texgamma);
			row[1] = pow(row[1], r_texgamma);
			row[2] = pow(row[2], r_texgamma);
		}
	}

	context->internalformat = GL_RGBA32F;
	context->compressed = false;
	context->width = w;
	context->height = h;
	context->mipmaps.emplace_back(0, imageData, pos, w, h);

	return context->callback(context);
}

bool LoadImageGenericRGB32F(const char* filename, FIBITMAP* fiB, gl_loadtexture_context_t* context)
{
	size_t pos = 0;
	size_t w = FreeImage_GetWidth(fiB);
	size_t h = FreeImage_GetHeight(fiB);
	size_t imageSize = w * h * sizeof(vec3_t);

	byte* imageData = FreeImage_GetBits(fiB);

	size_t rowSize = w * sizeof(vec3_t);

	FreeImage_FlipVertical(fiB);

	//Assume the texture to be SRGB
	float r_texgamma = 1.0f / v_gamma->value;

	for (unsigned int y = 0; y < h; ++y) {
		for (unsigned int x = 0; x < w; ++x) {
			float* row = (float*)(imageData + y * rowSize + x * sizeof(vec3_t));

			row[0] = pow(row[0], r_texgamma);
			row[1] = pow(row[1], r_texgamma);
			row[2] = pow(row[2], r_texgamma);
		}
	}

	context->internalformat = GL_RGB32F;
	context->compressed = false;
	context->width = w;
	context->height = h;
	context->mipmaps.emplace_back(0, imageData, pos, w, h);

	return context->callback(context);
}

bool LoadImageGenericBGRA8(const char *filename, FIBITMAP* fiB, gl_loadtexture_context_t* context)
{
	size_t pos = 0;
	size_t w = FreeImage_GetWidth(fiB);
	size_t h = FreeImage_GetHeight(fiB);

	byte* imageData = FreeImage_GetBits(fiB);

	FreeImage_FlipVertical(fiB);

	for (unsigned y = 0; y < h; ++y) {
		// Get a pointer to the start of the pixel row.
		BYTE* bits = FreeImage_GetScanLine(fiB, y);
		for (unsigned x = 0; x < w; ++x) {
			// Swap the red and blue bytes.
			// Assuming the format is BGRA, where the bytes are in the order (B, G, R, A).
			BYTE temp = bits[FI_RGBA_RED];
			bits[FI_RGBA_RED] = bits[FI_RGBA_BLUE];
			bits[FI_RGBA_BLUE] = temp;

			// Move to the next pixel (4 bytes per pixel).
			bits += 4;
		}
	}

	context->internalformat = GL_RGBA8;
	context->compressed = false;
	context->width = w;
	context->height = h;
	context->mipmaps.emplace_back(0, imageData, pos, w, h);

	return context->callback(context);
}

bool LoadWEBPFrame(FIMULTIBITMAP * fiBMulti, int page, gl_loadtexture_context_t* context)
{
	bool bLoaded = false;

	auto fiBPage = FreeImage_LockPage(fiBMulti, page);

	if (fiBPage)
	{
		auto colorType = FreeImage_GetColorType(fiBPage);

		if (colorType == FIC_RGBALPHA || colorType == FIC_RGB)
		{
			auto fiBGRA8 = FreeImage_ConvertTo32Bits(fiBPage);

			if (fiBGRA8)
			{
				FreeImage_FlipVertical(fiBGRA8);

				auto w = FreeImage_GetWidth(fiBGRA8);
				auto h = FreeImage_GetHeight(fiBGRA8);
				auto bpp = FreeImage_GetBPP(fiBGRA8);

				for (unsigned int y = 0; y < h; ++y) {
					// Get a pointer to the start of the pixel row.
					BYTE* bits = FreeImage_GetScanLine(fiBGRA8, y);
					for (unsigned int x = 0; x < w; ++x) {
						// Swap the red and blue bytes.
						// Assuming the format is BGRA, where the bytes are in the order (B, G, R, A).
						BYTE temp = bits[FI_RGBA_RED];
						bits[FI_RGBA_RED] = bits[FI_RGBA_BLUE];
						bits[FI_RGBA_BLUE] = temp;

						// Move to the next pixel (4 bytes per pixel).
						bits += 4;
					}
				}

				if (page > 0)
				{
					auto& previousMipmap = context->mipmaps[page - 1];

					auto fiPreviousBitmap = (FIBITMAP*)previousMipmap.data_ctx;

					auto fiSrcBitmap = fiBGRA8;
					auto fiDstBitmap = FreeImage_Clone(fiPreviousBitmap);

					int XOffset = 0;
					int YOffset = 0;

					FITAG* tagX = NULL;
					if (FreeImage_GetMetadata(FIMD_ANIMATION, fiBPage, "FrameLeft", &tagX))
					{
						XOffset = *(int*)FreeImage_GetTagValue(tagX);
					}

					FITAG* tagY = NULL;
					if (FreeImage_GetMetadata(FIMD_ANIMATION, fiBPage, "FrameTop", &tagY))
					{
						YOffset = *(int*)FreeImage_GetTagValue(tagY);
					}

					if (fiDstBitmap)
					{
						for (unsigned int y = 0; y < h; ++y) {

							auto srcBits = FreeImage_GetScanLine(fiSrcBitmap, y);
							auto dstBits = FreeImage_GetScanLine(fiDstBitmap, y + YOffset);

							for (unsigned int x = 0; x < w; ++x) {
								// Assuming 32-bit images (4 bytes per pixel: BGRA)
								// Composite src pixel onto dst pixel
								// Simple alpha blending: out = src_alpha * src + (1 - src_alpha) * dst

								float srcAlpha = srcBits[x * 4 + 3] / 255.0f; // Normalize the alpha

								dstBits[(x + XOffset) * 4 + 0] = (BYTE)(srcBits[x * 4 + 0] * srcAlpha + dstBits[(x + XOffset) * 4 + 0] * (1.0f - srcAlpha));
								dstBits[(x + XOffset) * 4 + 1] = (BYTE)(srcBits[x * 4 + 1] * srcAlpha + dstBits[(x + XOffset) * 4 + 1] * (1.0f - srcAlpha));
								dstBits[(x + XOffset) * 4 + 2] = (BYTE)(srcBits[x * 4 + 2] * srcAlpha + dstBits[(x + XOffset) * 4 + 2] * (1.0f - srcAlpha));
								//dstBits[(x + XOffset) * 4 + 3] = 255;
								//dstBits[(x + XOffset) * 4 + 3] = srcBits[x * 4 + 3];
							}
						}

						context->mipmaps.emplace_back(0, FreeImage_GetBits(fiDstBitmap), context->width * context->height * 4, context->width, context->height, fiDstBitmap);
						context->numframes ++;

						bLoaded = true;
					}

					FreeImage_Unload(fiBGRA8);
				}
				else
				{
					// duration of the frame (in milliseconds).
					int duration = 0;

					FITAG* tagFrameRate = NULL;
					if (FreeImage_GetMetadata(FIMD_ANIMATION, fiBPage, "FrameTime", &tagFrameRate))
					{
						duration = *(int*)FreeImage_GetTagValue(tagFrameRate);
					}

					context->mipmaps.emplace_back(0, FreeImage_GetBits(fiBGRA8), w * h * 4, w, h, fiBGRA8);
					context->width = w;
					context->height = h;
					context->numframes ++;
					context->frameduration = duration;

					bLoaded = true;
				}
			}
		}

		FreeImage_UnlockPage(fiBMulti, fiBPage, FALSE);
	}

	return bLoaded;
}

bool LoadWEBP(const char* filename, const char* pathId, gl_loadtexture_context_t* context)
{
	auto fileHandle = FILESYSTEM_ANY_OPEN(filename, "rb", pathId);

	if (!fileHandle)
	{
		return false;
	}

	SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(fileHandle); };

	FreeImageIO fiIO;
	fiIO.read_proc = FI_Read;
	fiIO.write_proc = FI_Write;
	fiIO.seek_proc = FI_Seek;
	fiIO.tell_proc = FI_Tell;

	auto fiFormat = FreeImage_GetFileTypeFromHandle(&fiIO, (fi_handle)fileHandle);

	if (fiFormat != FIF_WEBP)
	{
		gEngfuncs.Con_Printf("LoadWEBP: Could not load %s, Invalid format.\n", filename);
		return false;
	}

	if (!FreeImage_FIFSupportsReading(fiFormat))
	{
		gEngfuncs.Con_Printf("LoadWEBP: Could not load %s, Unsupported format.\n", filename);
		return false;
	}

	auto fiBMulti = FreeImage_OpenMultiBitmapFromHandle(fiFormat, &fiIO, (fi_handle)fileHandle, WEBP_LOAD_FRAME);

	if (!fiBMulti)
	{
		gEngfuncs.Con_Printf("LoadWEBP: Could not load %s, FreeImage_OpenMultiBitmapFromHandle failed.\n", filename);
		return false;
	}

	SCOPE_EXIT{ FreeImage_CloseMultiBitmap(fiBMulti); };

	int count = FreeImage_GetPageCount(fiBMulti);

	for (int i = 0; i < count; ++i)
	{
		if (!LoadWEBPFrame(fiBMulti, i, context))
		{
			gEngfuncs.Con_Printf("LoadWEBP: Could not load %s, LoadWEBPFrame failed.\n", filename);
			return false;
		}
	}

	context->internalformat = GL_RGBA8;
	context->compressed = false;

	context->mipmaps_dtor = [](void* data_ctx, const void* data, int size) {

		auto fi = (FIBITMAP *)data_ctx;

		FreeImage_Unload(fi);

	};

#if 0
	for (size_t i = 0; i < context->mipmaps.size(); ++i)
	{
		char test[256];
		sprintf(test, "test_%d.png", i);
		SaveImageGenericRGBA8(test, NULL, context->mipmaps[i].width, context->mipmaps[i].height, context->mipmaps[i].data);
	}
#endif

	return context->callback(context);
}

bool LoadImageGeneric(const char *filename, const char* pathId, gl_loadtexture_context_t * context)
{
	auto fileHandle = FILESYSTEM_ANY_OPEN(filename, "rb", pathId);

	if (!fileHandle)
	{
		return false;
	}

	SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(fileHandle); };

	FreeImageIO fiIO;
	fiIO.read_proc = FI_Read;
	fiIO.write_proc = FI_Write;
	fiIO.seek_proc = FI_Seek;
	fiIO.tell_proc = FI_Tell;

	auto fiFormat = FreeImage_GetFileTypeFromHandle(&fiIO, (fi_handle)fileHandle);

	if (fiFormat == FIF_UNKNOWN)
	{
		fiFormat = FreeImage_GetFIFFromFilename(filename);
	}

	if(fiFormat == FIF_UNKNOWN)
	{
		gEngfuncs.Con_Printf("LoadImageGeneric: Could not load %s, Unknown format.\n", filename);
		return false;
	}

	if(!FreeImage_FIFSupportsReading(fiFormat))
	{
		gEngfuncs.Con_Printf("LoadImageGeneric: Could not load %s, Unsupported format.\n", filename);
		return false;
	}

	int flags = 0;
	if (context->ignore_direction_LoadTGA && fiFormat == FIF_TARGA)
	{
		flags |= TARGA_IGNORE_DIRECTION;
	}

	auto fiB = FreeImage_LoadFromHandle(fiFormat, &fiIO, (fi_handle)fileHandle, flags);

	if (!fiB)
	{
		gEngfuncs.Con_Printf("LoadImageGeneric: Could not load %s, FreeImage_LoadFromHandle failed.\n", filename);
		return false;
	}

	SCOPE_EXIT{ FreeImage_Unload(fiB); };

	if (fiFormat == FIF_HDR)
	{
		auto colorType = FreeImage_GetColorType(fiB);

		if (colorType == FIC_RGBALPHA)
		{
			auto fiBFloat = FreeImage_ConvertToRGBAF(fiB);

			if (!fiBFloat)
			{
				gEngfuncs.Con_Printf("LoadImageGeneric: Could not load %s, FreeImage_ConvertToRGBAF failed.\n", filename);
				return false;
			}

			SCOPE_EXIT{ FreeImage_Unload(fiBFloat); };

			if (LoadImageGenericRGBA32F(filename, fiBFloat, context))
			{
				return true;
			}
		}
		else if (colorType == FIC_RGB)
		{
			auto fiBFloat = FreeImage_ConvertToRGBF(fiB);

			if (!fiBFloat)
			{
				gEngfuncs.Con_Printf("LoadImageGeneric: Could not load %s, FreeImage_ConvertToRGBF failed.\n", filename);
				return false;
			}

			SCOPE_EXIT{ FreeImage_Unload(fiBFloat); };

			if (LoadImageGenericRGB32F(filename, fiBFloat, context))
			{
				return true;
			}
		}
	}
	else
	{
		auto colorType = FreeImage_GetColorType(fiB);

		if (colorType == FIC_RGBALPHA || colorType == FIC_RGB || colorType == FIC_PALETTE)
		{
			auto fiBGRA8 = FreeImage_ConvertTo32Bits(fiB);

			if (!fiBGRA8)
			{
				gEngfuncs.Con_Printf("LoadImageGeneric: Could not load %s, FreeImage_ConvertTo32Bits failed.\n", filename);
				return false;
			}

			SCOPE_EXIT{ FreeImage_Unload(fiBGRA8); };

			if (LoadImageGenericBGRA8(filename, fiBGRA8, context))
			{
				return true;
			}
		}
	}

	return false;
}

bool SaveImageGenericRGBA8(const char* filename, const char* pathId, int width, int height, const void* data)
{
	const char* extension = V_GetFileExtension(filename);

	FREE_IMAGE_FORMAT fiFormat = FreeImage_GetFIFFromFilename(filename);

	if (fiFormat == FIF_UNKNOWN)
	{
		gEngfuncs.Con_Printf("SaveImageGenericRGBA8: Could not save %s, Unsupported format.\n", filename);
		return false;
	}

	if (!FreeImage_FIFSupportsWriting(fiFormat))
	{
		gEngfuncs.Con_Printf("SaveImageGenericRGBA8: Could not save %s, Unsupported format.\n", filename);
		return false;
	}

	FileHandle_t fileHandle = FILESYSTEM_ANY_OPEN(filename, "wb", pathId);

	if (!fileHandle)
	{
		gEngfuncs.Con_Printf("SaveImageGenericRGBA8: Could not open %s for writing.\n", filename);
		return false;
	}

	SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(fileHandle); };

	FreeImageIO fiIO;
	fiIO.read_proc = FI_Read;
	fiIO.write_proc = FI_Write;
	fiIO.seek_proc = FI_Seek;
	fiIO.tell_proc = FI_Tell;

	auto fiB = FreeImage_Allocate(width, height, 32);

	SCOPE_EXIT{ FreeImage_Unload(fiB); };

	auto src = (byte*)data;
	int pos = 0;

	int blockSize = FreeImage_GetLine(fiB) / width;

	for (int y = 0; y < height; ++y)
	{
		BYTE* bits = FreeImage_GetScanLine(fiB, y);
		for (int x = 0; x < width; ++x)
		{
			bits[FI_RGBA_RED] = src[pos++];
			bits[FI_RGBA_GREEN] = src[pos++];
			bits[FI_RGBA_BLUE] = src[pos++];
			bits[FI_RGBA_ALPHA] = src[pos++];
			bits += blockSize;
		}
	}

	if (FALSE == FreeImage_SaveToHandle(fiFormat, fiB, &fiIO, (fi_handle)fileHandle))
	{
		gEngfuncs.Con_Printf("SaveImageGenericRGBA8: Could not save %s, FreeImage_SaveToHandle failed.\n", filename);
		return false;
	}

	return true;
}

bool SaveImageGenericRGB8(const char *filename, const char* pathId, int width, int height, const void *data)
{
	const char *extension = V_GetFileExtension(filename);

	FREE_IMAGE_FORMAT fiFormat = FreeImage_GetFIFFromFilename(filename);

	if(fiFormat == FIF_UNKNOWN)
	{
		gEngfuncs.Con_Printf("SaveImageGenericRGB8: Could not save %s, Unsupported format.\n", filename);
		return false;  
	}

	if(!FreeImage_FIFSupportsWriting(fiFormat))
    {
		gEngfuncs.Con_Printf("SaveImageGenericRGB8: Could not save %s, Unsupported format.\n", filename);
		return false;
    }

	FileHandle_t fileHandle = FILESYSTEM_ANY_OPEN(filename, "wb", pathId);

	if(!fileHandle)
    {
		gEngfuncs.Con_Printf("SaveImageGenericRGB8: Could not open %s for writing.\n", filename);
		return false;  
    }

	SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(fileHandle); };

	FreeImageIO fiIO;
	fiIO.read_proc = FI_Read;
	fiIO.write_proc = FI_Write;
	fiIO.seek_proc = FI_Seek;
	fiIO.tell_proc = FI_Tell;

	auto fiB = FreeImage_Allocate(width, height, 24);

	SCOPE_EXIT{ FreeImage_Unload(fiB); };

	auto src = (byte*)data;
	int pos = 0;
	int blockSize = FreeImage_GetLine(fiB) / width;

	for(int y = 0; y < height; ++y)
	{
		BYTE *bits = FreeImage_GetScanLine(fiB, y);
		for(int x = 0; x < width; ++x)
		{
			bits[FI_RGBA_RED] = src[pos++];
			bits[FI_RGBA_GREEN] = src[pos++];
			bits[FI_RGBA_BLUE] = src[pos++];
			bits += blockSize;
		}
	}

	if(FALSE == FreeImage_SaveToHandle(fiFormat, fiB, &fiIO, (fi_handle)fileHandle))
    {
		gEngfuncs.Con_Printf("SaveImageGenericRGB8: Could not save %s, FreeImage_SaveToHandle failed.\n", filename);
        return false;
    }

	return true;
}

void GL_FillLoadTextureResultFromTextureEntry(const gltexture_t* textureEntry, gl_loadtexture_result_t* result)
{
	result->gltexturenum = textureEntry->texnum;
	result->width = textureEntry->width;
	result->height = textureEntry->height;

	GL_ParseTextureIdentifier(textureEntry->identifier, result);
}

void GL_FillLoadTextureResultFromLoadTextureContext(const gltexture_t* textureEntry, const gl_loadtexture_context_t* context, gl_loadtexture_result_t* result)
{
	result->gltexturenum = textureEntry->texnum;
	result->width = context->width;
	result->height = context->height;
	result->numframes = context->numframes;
	result->frameduration = context->frameduration;
}

int R_LoadRGBA8TextureFromMemory(const char* identifier, const void* data, int width, int height, GL_TEXTURETYPE textureType, bool mipmap)
{
	gl_loadtexture_context_t context;

	context.internalformat = GL_RGBA8;
	context.compressed = false;
	context.width = width;
	context.height = height;
	context.mipmap = mipmap;
	context.mipmaps.emplace_back(0, data, 0, width, height);

	auto textureEntry = GL_LoadTextureEx(identifier, textureType, &context);

	if (textureEntry)
	{
		return textureEntry->texnum;
	}

	return 0;
}

#if 0

int AVCodec_Read(void* opaque, uint8_t* buf, int buf_size) {

	FileHandle_t fileHandle = (FileHandle_t)opaque;

	return FILESYSTEM_ANY_READ(buf, buf_size, fileHandle);
}

int64_t AVCodec_Seek(void* opaque, int64_t offset, int whence) {

	FileHandle_t fileHandle = (FileHandle_t)opaque;
	int64_t ret = -1;

	switch (whence)
	{
	case AVSEEK_SIZE:
	{
		return FILESYSTEM_ANY_SIZE(fileHandle);
	}

	case SEEK_END:
	{

		break;
	}
	case SEEK_CUR:
	{
		
		break;
	}
	case SEEK_SET:
	{
		FILESYSTEM_ANY_SEEK(fileHandle, offset, FileSystemSeek_t::FILESYSTEM_SEEK_HEAD);
		ret = FILESYSTEM_ANY_TELL(fileHandle);

		break;
	}
	}

	return ret;
}

int process_frame(AVFormatContext* fmt_ctx,
	AVCodecContext* dec_ctx,
	AVCodecParameters* par,
	AVFrame* frame,
	AVPacket* pkt,
	int* packet_new) {
	int ret = 0, got_frame = 0;

	if (dec_ctx && dec_ctx->codec) {
		switch (par->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
		case AVMEDIA_TYPE_AUDIO:
			if (*packet_new) {
				ret = avcodec_send_packet(dec_ctx, pkt);
				if (ret == AVERROR(EAGAIN)) {
					ret = 0;
				}
				else if (ret >= 0 || ret == AVERROR_EOF) {
					ret = 0;
					*packet_new = 0;
				}
			}
			if (ret >= 0) {
				ret = avcodec_receive_frame(dec_ctx, frame);
				if (ret >= 0) {
					got_frame = 1;
				}
				else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					ret = 0;
				}
			}
			break;

		case AVMEDIA_TYPE_SUBTITLE:
			*packet_new = 0;
			break;
		default:
			*packet_new = 0;
		}
	}
	else {
		*packet_new = 0;
	}

	if (ret < 0) {
		return ret;
	}

	return got_frame || *packet_new;
}

bool LoadVideoFrames(const char* filename, const char* pathId, gl_loadtexture_context_t *state)
{
	FileHandle_t fileHandle = FILESYSTEM_ANY_OPEN(filename, "rb", pathId);

	if (!fileHandle)
	{
		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to open %s.\n", filename);
		return false;
	}

	SCOPE_EXIT{ FILESYSTEM_ANY_CLOSE(fileHandle); };

	size_t io_buffer_size = 0x10000;

	unsigned char* io_buffer = (unsigned char*)av_malloc(io_buffer_size);

	if (!io_buffer)
	{
		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to av_malloc for %s.\n", filename);
		return false;
	}

	auto avio_ctx = avio_alloc_context(io_buffer, io_buffer_size, 0, fileHandle, AVCodec_Read, NULL, AVCodec_Seek);

	if (!avio_ctx)
	{
		av_free(io_buffer);

		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to avio_alloc_context for %s.\n", filename);
		return false;
	}

	SCOPE_EXIT{
		av_free(avio_ctx->buffer);
		avio_context_free(&avio_ctx); 
	};

	auto formatContext = avformat_alloc_context();

	if(!formatContext)
	{
		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to avformat_alloc_context for %s.\n", filename);
		return false;
	}

	SCOPE_EXIT
	{ 
		if (formatContext) {
			avformat_free_context(formatContext); 
			formatContext = NULL;
		}
	};
	
	formatContext->pb = avio_ctx;

	if (avformat_open_input(&formatContext, NULL, NULL, NULL) < 0)
	{
		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to avformat_open_input for %s.\n", filename);
		return false;
	}

	SCOPE_EXIT{ if(formatContext) avformat_close_input(&formatContext); };

	if (avformat_find_stream_info(formatContext, NULL) <  0)
	{
		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to avformat_find_stream_info for %s.\n", filename);
		return false;
	}

	// Find the video stream
	AVCodecParameters* codecParameters = NULL;

	auto videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

	if (videoStreamIndex >= 0)
	{
		codecParameters = formatContext->streams[videoStreamIndex]->codecpar;
	}

	if (!codecParameters)
	{
		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to av_find_best_stream for %s.\n", filename);
		return false;
	}

	auto codec = avcodec_find_decoder(codecParameters->codec_id);

	if (!codec)
	{
		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to avcodec_find_decoder for %s.\n", filename);
		return false;
	}

	auto codecContext = avcodec_alloc_context3(codec);

	if (!codecContext)
	{
		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to avcodec_alloc_context3 for %s.\n", filename);
		return false;
	}

	SCOPE_EXIT{ if(codecContext) avcodec_free_context(&codecContext); };

	if (avcodec_parameters_to_context(codecContext, codecParameters) < 0)
	{
		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to avcodec_parameters_to_context for %s.\n", filename);
		return false;
	}

	if (avcodec_open2(codecContext, codec, NULL) < 0)
	{
		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to avcodec_open2 for %s.\n", filename);
		return false;
	}

	SCOPE_EXIT{ if (codecContext) avcodec_close(codecContext); };

	auto frame = av_frame_alloc();

	if (!frame)
	{
		gEngfuncs.Con_Printf("LoadVideoFrames: Failed to av_frame_alloc for %s.\n", filename);
		return false;
	}

	SCOPE_EXIT{ if (frame) av_frame_free(&frame); };

	auto packet = av_packet_alloc();

	while (av_read_frame(formatContext, packet) == 0)
	{
		if (packet->stream_index == videoStreamIndex)
		{
			int packet_new = 1;
			while (process_frame(formatContext, codecContext, codecParameters, frame, packet, &packet_new) > 0)
			{
				auto swsCtx = sws_getContext(
					frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
					codecParameters->width, codecParameters->height, AV_PIX_FMT_RGBA,
					SWS_BILINEAR, NULL, NULL, NULL
				);

				if (!swsCtx)
				{
					gEngfuncs.Con_Printf("LoadVideoFrames: Failed to sws_getContext for %s.\n", filename);
					return false;
				}

				SCOPE_EXIT{ if (swsCtx) sws_freeContext(swsCtx); };

				AVFrame* rgbFrame = av_frame_alloc();

				if (!rgbFrame)
				{
					gEngfuncs.Con_Printf("LoadVideoFrames: Failed to av_frame_alloc for %s.\n", filename);
					return false;
				}

				rgbFrame->width = codecParameters->width;
				rgbFrame->height = codecParameters->height;
				rgbFrame->format = AV_PIX_FMT_RGBA;

				// Allocate the buffer for the frame data
				if (av_frame_get_buffer(rgbFrame, 0) < 0)
				{
					gEngfuncs.Con_Printf("LoadVideoFrames: Could not allocate frame data.\n");
					av_frame_free(&rgbFrame);
					return false;
				}

				// Make sure the frame data is writable
				if (av_frame_make_writable(rgbFrame) < 0)
				{
					gEngfuncs.Con_Printf("LoadVideoFrames: Frame not writable.\n");
					av_frame_free(&rgbFrame);
					return false;
				}

				if (sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, rgbFrame->data, rgbFrame->linesize) == 0)
				{
					gEngfuncs.Con_Printf("LoadVideoFrames: Failed to sws_scale for %s.\n", filename);
					av_frame_free(&rgbFrame);
					return false;
				}

				state->mipmaps.emplace_back(0, rgbFrame->data[0], codecParameters->width * codecParameters->height * 4, codecParameters->width, codecParameters->height, rgbFrame);

				frameIndex++;
			}
		}

		av_packet_unref(packet);
	}

	if (1)
	{
		int packet_new = 1;
		packet->stream_index = videoStreamIndex;
		while (process_frame(formatContext, codecContext, codecParameters, frame, packet, &packet_new) > 0)
		{
			auto swsCtx = sws_getContext(
				frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
				codecParameters->width, codecParameters->height, AV_PIX_FMT_RGBA,
				SWS_BILINEAR, NULL, NULL, NULL
			);

			if (!swsCtx)
			{
				gEngfuncs.Con_Printf("LoadVideoFrames: Failed to sws_getContext for %s.\n", filename);
				return false;
			}

			SCOPE_EXIT{ if (swsCtx) sws_freeContext(swsCtx); };

			AVFrame* rgbFrame = av_frame_alloc();

			if (!rgbFrame)
			{
				gEngfuncs.Con_Printf("LoadVideoFrames: Failed to av_frame_alloc for %s.\n", filename);
				return false;
			}

			rgbFrame->width = codecParameters->width;
			rgbFrame->height = codecParameters->height;
			rgbFrame->format = AV_PIX_FMT_RGBA;

			// Allocate the buffer for the frame data
			if (av_frame_get_buffer(rgbFrame, 0) < 0)
			{
				gEngfuncs.Con_Printf("LoadVideoFrames: Could not allocate frame data.\n");
				av_frame_free(&rgbFrame);
				return false;
			}

			// Make sure the frame data is writable
			if (av_frame_make_writable(rgbFrame) < 0)
			{
				gEngfuncs.Con_Printf("LoadVideoFrames: Frame not writable.\n");
				av_frame_free(&rgbFrame);
				return false;
			}

			if (sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, rgbFrame->data, rgbFrame->linesize) == 0)
			{
				gEngfuncs.Con_Printf("LoadVideoFrames: Failed to sws_scale for %s.\n", filename);
				av_frame_free(&rgbFrame);
				return false;
			}

			state->mipmaps.emplace_back(0, rgbFrame->data[0], codecParameters->width * codecParameters->height * 4, codecParameters->width, codecParameters->height, rgbFrame);
			state->numframes++;
		}

		avcodec_flush_buffers(codecContext);
	}

	state->internalformat = GL_RGBA8;
	state->compressed = false;
	state->width = codecParameters->width;
	state->height = codecParameters->height;

	state->mipmaps_dtor = [](void* data_ctx, void *data, int size) {

		AVFrame* rgbFrame = (AVFrame*)data_ctx;

		av_frame_free(&rgbFrame);

	};

	return state->callback(state);
}

#endif

bool R_LoadTextureFromFile(const char *filename, const char * identifier, GL_TEXTURETYPE textureType, bool mipmap, gl_loadtexture_result_t* result)
{
	auto textureEntry = GL_FindTextureEntry(identifier, textureType);

	if (textureEntry)
	{
		GL_FillLoadTextureResultFromTextureEntry(textureEntry, result);

		return true;
	}

	const char *szExtension = V_GetFileExtension(filename);

	if(!szExtension)
	{
		gEngfuncs.Con_Printf("R_LoadTextureFromFile: File %s has no extension.\n", filename);
		return false;
	}

	gl_loadtexture_context_t context;

	context.mipmap = mipmap;
	context.callback = [&textureEntry, identifier, textureType, result](gl_loadtexture_context_t *ctx) {

		textureEntry = GL_LoadTextureEx(identifier, textureType, ctx);

		if (textureEntry)
		{
			GL_FillLoadTextureResultFromLoadTextureContext(textureEntry, ctx, result);
			return true;
		}

		return false;
	};

#if 0
	if (!stricmp(extension, "webm") || !stricmp(extension, "mp4") || !stricmp(extension, "avi") || !stricmp(extension, "mkv") || !stricmp(extension, "flv") || !stricmp(extension, "mov") || !stricmp(extension, "m3u8"))
	{
		if (LoadVideoFrames(filename, NULL, &context))
		{
			return foundTexture;
		}
	}
#endif

	if (!stricmp(szExtension, "webp"))
	{
		if (LoadWEBP(filename, NULL, &context))
		{
			return true;
		}
	}

#if 0
	else if (!stricmp(extension, "gif"))
	{
		if (LoadGIF(filename, NULL, &context))
		{
			return true;
		}
	}
#endif

	if(!stricmp(szExtension, "dds"))
	{
		if(LoadDDS(filename, NULL, &context))
		{
			return true;
		}
	}

	if(LoadImageGeneric(filename, NULL, &context))
	{
		return true;
	}
	
	return false;
}

void BuildGammaTable(float g)
{
	gPrivateFuncs.BuildGammaTable(g);

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

void __fastcall enginesurface_drawSetTextureFile(void* pthis, int dummy, int textureId, const char* filename, qboolean hardwareFilter, bool forceReload)
{
	bool bLoaded = false;
	char filepath[1024];

	if (gPrivateFuncs.staticGetTextureById)
	{
		auto texture = gPrivateFuncs.staticGetTextureById(textureId);

		if (texture && !forceReload)
		{
			gPrivateFuncs.enginesurface_drawSetTexture(pthis, dummy, textureId);
			return;
		}
	}
	else
	{
		if (gPrivateFuncs.enginesurface_isTextureIDValid(pthis, dummy, textureId) && !forceReload)
		{
			gPrivateFuncs.enginesurface_drawSetTexture(pthis, dummy, textureId);
			return;
		}
	}

	gl_loadtexture_context_t context;
	context.wrap = GL_CLAMP_TO_EDGE;
	context.filter = hardwareFilter ? GL_LINEAR : GL_NEAREST;
	context.ignore_direction_LoadTGA = true;

	context.callback = [pthis, textureId, hardwareFilter](gl_loadtexture_context_t* ctx) {

		if (ctx->mipmaps.size() > 0)
		{
			if (ctx->compressed)
			{
				//We have to call drawSetTextureRGBA with dummy shit to allocate a new enginesurface_Texture for us
				gPrivateFuncs.enginesurface_drawSetTextureRGBA(pthis, 0, textureId, (const char*)texloader_buffer, 16, 16, hardwareFilter, true);

				(*currenttexture) = -1;
				GL_Bind(textureId);
				GL_UploadCompressedTexture(ctx, GL_TEXTURE_2D);

				return true;
			}
			else
			{
				gPrivateFuncs.enginesurface_drawSetTextureRGBA(pthis, 0, textureId,
					(const char*)ctx->mipmaps[0].data, ctx->mipmaps[0].width, ctx->mipmaps[0].height,
					hardwareFilter, true);

				return true;
			}
		}

		return false;
	};

	if (1)
	{
		snprintf(filepath, sizeof(filepath), "%s.dds", filename);
		if (g_iEngineType == ENGINE_SVENGINE && !bLoaded && LoadDDS(filepath, "UI", &context))
		{
			bLoaded = true;
		}
		if (!bLoaded && LoadDDS(filepath, NULL, &context))
		{
			bLoaded = true;
		}
	}
	if (1)
	{
		snprintf(filepath, sizeof(filepath), "%s.png", filename);
		if (g_iEngineType == ENGINE_SVENGINE && !bLoaded && LoadImageGeneric(filepath, "UI", &context))
		{
			bLoaded = true;
		}
		if (!bLoaded && LoadImageGeneric(filepath, NULL, &context))
		{
			bLoaded = true;
		}
	}
	if (1)
	{
		snprintf(filepath, sizeof(filepath), "%s.tga", filename);
		if (g_iEngineType == ENGINE_SVENGINE && !bLoaded && LoadImageGeneric(filepath, "UI", &context))
		{
			bLoaded = true;
		}
		if (!bLoaded && LoadImageGeneric(filepath, NULL, &context))
		{
			bLoaded = true;
		}
	}
	if (1)
	{
		snprintf(filepath, sizeof(filepath), "%s.bmp", filename);
		if (g_iEngineType == ENGINE_SVENGINE && !bLoaded && LoadImageGeneric(filepath, "UI", &context))
		{
			bLoaded = true;
		}
		if (!bLoaded && LoadImageGeneric(filepath, NULL, &context))
		{
			bLoaded = true;
		}
	}

	if (bLoaded)
	{
		if (gPrivateFuncs.staticGetTextureById)
		{
			auto texture = gPrivateFuncs.staticGetTextureById(textureId);

			if (texture)
			{
				gPrivateFuncs.enginesurface_drawSetTexture(pthis, dummy, textureId);
				return;
			}
		}
		else
		{
			if (gPrivateFuncs.enginesurface_isTextureIDValid(pthis, dummy, textureId))
			{
				gPrivateFuncs.enginesurface_drawSetTexture(pthis, dummy, textureId);
				return;
			}
		}
	}
}

int __fastcall enginesurface_createNewTextureID(void* pthis, int dummy)
{
	// allocated_surface_texture = 5810;
	return (int)GL_GenTexture();
}

void __fastcall enginesurface_drawFlushText(void *pthis, int dummy)
{
	gPrivateFuncs.enginesurface_drawFlushText(pthis, dummy);

	//Valve called glEnableClientState(GL_VERTEX_ARRAY) and forgot to disable it.
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}