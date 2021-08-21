#include "gl_local.h"
#include "command.h"

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

int *gl_filter_min = NULL;
int *gl_filter_max = NULL;

cvar_callback_entry_t **cvar_callbacks = NULL;

int gl_loadtexture_format = GL_RGBA;
int gl_loadtexture_cubemap = 0;
int gl_loadtexture_size = 0;

gltexture_t *gltextures_get()
{
	return (gltextures_SvEngine) ? (*gltextures_SvEngine) : gltextures;
}

cvar_callback_t Cvar_HookCallback(const char *cvar_name, cvar_callback_t callback)
{
	auto cvar = gEngfuncs.pfnGetCvarPointer(cvar_name);
	auto v3 = (*cvar_callbacks);
	if (v3)
	{
		while (v3->pcvar != cvar)
		{
			v3 = v3->next;
			if (!v3)
				return NULL;
		}
		auto orig = v3->callback;
		v3->callback = callback;
		return orig;
	}

	return NULL;
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
		gEngfuncs.Con_Printf("bad filter name\n");
		return;
	}

	*gl_filter_min = gl_texture_modes[i].minimize;
	*gl_filter_max = gl_texture_modes[i].maximize;

	gltexture_t *pgltextures = gltextures_get();

	for (int j = 0; j < (*numgltextures); ++j)
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

	for (int j = 0; j < 6; ++j)
	{
		if (gSkyTexNumber[j])
		{
			GL_Bind(gSkyTexNumber[j]);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, *gl_filter_max);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max(min(gl_ansio->value, gl_max_ansio), 1));
		}
	}
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

void GL_DeleteBuffer(GLuint buf)
{
	glDeleteBuffers(1, &buf);
}

void GL_DeleteTexture(GLuint tex)
{
	glDeleteTextures(1, &tex);
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

void GL_UploadDXT(byte *data, int width, int height, qboolean mipmap, qboolean ansio, int wrap)
{
	int iTextureTarget = GL_TEXTURE_2D;

	if (gl_loadtexture_cubemap)
		iTextureTarget = GL_TEXTURE_CUBE_MAP;

	glTexParameterf(iTextureTarget, GL_GENERATE_MIPMAP, (mipmap) ? GL_TRUE : GL_FALSE);

	if (gl_loadtexture_cubemap)
	{
		glCompressedTexImage2DARB(GL_TEXTURE_CUBE_MAP_POSITIVE_X + gl_loadtexture_cubemap - 1, 0, gl_loadtexture_format, width, height, 0, gl_loadtexture_size, data);
	}
	else
	{
		glCompressedTexImage2DARB(iTextureTarget, 0, gl_loadtexture_format, width, height, 0, gl_loadtexture_size, data);
	}

	if (mipmap)
	{
		glTexParameterf(iTextureTarget, GL_TEXTURE_MIN_FILTER, *gl_filter_min);
		glTexParameterf(iTextureTarget, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
	}
	else
	{
		glTexParameterf(iTextureTarget, GL_TEXTURE_MIN_FILTER, *gl_filter_max);
		glTexParameterf(iTextureTarget, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
	}

	if (ansio && gl_ansio)
	{
		glTexParameterf(iTextureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, max(min(gl_ansio->value, gl_max_ansio), 1));
	}
	else
	{
		glTexParameterf(iTextureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
}

void GL_Upload32(unsigned int *data, int width, int height, qboolean mipmap, qboolean ansio, int wrap)
{
	int iComponent, iFormat, iTextureTarget;

	int bbp = 0;
	g_pMetaHookAPI->GetVideoMode(NULL, NULL, &bbp, NULL);

	iFormat = GL_RGBA;
	iComponent = (bbp == 16) ? GL_RGB5_A1 : GL_RGBA8;
	iTextureTarget = GL_TEXTURE_2D;

	if (gl_loadtexture_cubemap)
		iTextureTarget = GL_TEXTURE_CUBE_MAP;

	glTexParameterf(iTextureTarget, GL_GENERATE_MIPMAP, (mipmap) ? GL_TRUE : GL_FALSE);

	if (gl_loadtexture_cubemap)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + gl_loadtexture_cubemap - 1, 0, iComponent, width, height, 0, iFormat, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		glTexImage2D(iTextureTarget, 0, iComponent, width, height, 0, iFormat, GL_UNSIGNED_BYTE, data);
	}

	if (mipmap)
	{
		glTexParameterf(iTextureTarget, GL_TEXTURE_MIN_FILTER, *gl_filter_min);
		glTexParameterf(iTextureTarget, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
	}
	else
	{
		glTexParameterf(iTextureTarget, GL_TEXTURE_MIN_FILTER, *gl_filter_max);
		glTexParameterf(iTextureTarget, GL_TEXTURE_MAG_FILTER, *gl_filter_max);
	}

	if(ansio && gl_ansio)
	{
		glTexParameterf(iTextureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, max(min(gl_ansio->value, gl_max_ansio), 1));
	}
	else
	{
		glTexParameterf(iTextureTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
}

void GL_Upload16(byte *data, int width, int height, qboolean mipmap, int iType, unsigned char *pPal, qboolean ansio, int wrap)
{
	static unsigned int *trans = NULL;//[640*480];
	static int transSize = 0;

	int			i, s;
	qboolean	noalpha;
	int			p;
	unsigned char *pb;

	s = width*height;

	if(trans == NULL)
	{
		transSize = max(s, 640*480);
		trans = new unsigned int[transSize];
	}
	else if(transSize < s)
	{
		delete [] trans;
		transSize = s;
		trans = new unsigned int[s];
	}

	if ( iType != TEX_TYPE_LUM )
	{
		if ( !pPal )
			return;

		//for ( i = 0; i < 768; i++ )
		//	pPal[i] = texgammatable[pPal[i]];
	}

	// if there are no transparent pixels, make it a 3 component
	// texture even if it was specified as otherwise
	if ( TEX_IS_ALPHA( iType ) )
	{
		noalpha = true;

		if ( iType == TEX_TYPE_ALPHA_GRADIENT )
		{
			for (i=0 ; i<s ; i++)
			{
				p = data[i];
				pb = (unsigned char *)&trans[i];
				pb[0] = pPal[765];
				pb[1] = pPal[766];
				pb[2] = pPal[767];
				pb[3] = p;
				noalpha = false;
			}
		}
		else if ( iType == TEX_TYPE_RGBA )
		{
			for (i=0 ; i<s ; i++)
			{
				p = data[i];
				pb = (unsigned char *)&trans[i];
				pb[0] = pPal[p * 3 + 0];
				pb[1] = pPal[p * 3 + 1];
				pb[2] = pPal[p * 3 + 2];
				pb[3] = p;
				noalpha = false;
			}
		}
		else if ( iType == TEX_TYPE_ALPHA )
		{
			for (i=0 ; i<s ; i++)
			{
				p = data[i];
				pb = (unsigned char *)&trans[i];

				if (p == 255)
				{
					noalpha = false;
					pb[0] = 0;
					pb[1] = 0;
					pb[2] = 0;
					pb[3] = 0;
				}
				else
				{
					pb[0] = pPal[p * 3 + 0];
					pb[1] = pPal[p * 3 + 1];
					pb[2] = pPal[p * 3 + 2];
					pb[3] = 255;
				}
			}
		}

		if (noalpha)
			iType = TEX_TYPE_NONE;
	}
	else if ( iType == TEX_TYPE_NONE )
	{
		if (s&3)
			Sys_ErrorEx("GL_Upload16: s&3");

		if ( gl_dither && gl_dither->value )
		{
			for (i=0 ; i<s ; i++)
			{
				unsigned char r, g, b, *ppix;

				p = data[i];
				pb = (unsigned char *)&trans[i];
				ppix = &pPal[p * 3];
				r = ppix[0] |= (ppix[0] >> 6); 
				g = ppix[1] |= (ppix[1] >> 6);
				b = ppix[2] |= (ppix[2] >> 6);

				pb[0] = r;
				pb[1] = g;
				pb[2] = b;
				pb[3] = 255;
			}
		}
		else
		{
			for (i=0 ; i<s ; i++)
			{
				p = data[i];
				pb = (unsigned char *)&trans[i];
				pb[0] = pPal[p * 3 + 0];
				pb[1] = pPal[p * 3 + 1];
				pb[2] = pPal[p * 3 + 2];
				pb[3] = 255;
			}
		}
	}
	else if ( iType == TEX_TYPE_LUM )
	{
		memcpy( trans, data, width * height );
	}
	else
	{
		gEngfuncs.Con_Printf( "Upload16:Bogus texture type!/n" );
	}

	GL_Upload32(trans, width, height, mipmap, ansio, wrap);
}

int GL_FindTexture(const char *identifier, GL_TEXTURETYPE textureType, int *width, int *height)
{
	int i;
	gltexture_t *slot;

	if (gltextures_SvEngine)
		gltextures = *gltextures_SvEngine;

	if (identifier[0])
	{
		for (i = 0, slot = gltextures; i < *numgltextures; i++, slot++)
		{
			if (!stricmp(identifier, slot->identifier))
			{
				if (textureType != GLT_SYSTEM && textureType != GLT_DECAL && textureType != GLT_HUDSPRITE)
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

int GL_AllocTexture(char *identifier, GL_TEXTURETYPE textureType, int width, int height, qboolean mipmap)
{
	int i;
	gltexture_t *glt;
	gltexture_t *slot;

	glt = NULL;

	if (gltextures_SvEngine)
		gltextures = *gltextures_SvEngine;

tryagain:
	if (identifier[0])
	{
		for (i = 0, slot = gltextures; i < *numgltextures; i++, slot++)
		{
			if (slot->servercount < 0)
			{
				if (!glt)
					glt = slot;

				break;
			}

			if (!strcmp(identifier, slot->identifier))
			{
				if (width != slot->width || height != slot->height)
				{
					identifier[3]++;
					goto tryagain;
				}

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
				gltextures = *gltextures_SvEngine;
			}
		}
		else
		{
			if ((*numgltextures) + 1 >= MAX_GLTEXTURES)
			{
				Sys_ErrorEx("Texture Overflow: MAX_GLTEXTURES\n");
				return 0;
			}
		}

		glt = &gltextures[(*numgltextures)];
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
	glt->servercount = (textureType != GLT_SYSTEM && textureType != GLT_DECAL && textureType != GLT_HUDSPRITE) ? *gHostSpawnCount : 0;
	glt->paletteIndex = -1;

	return glt->texnum;
}

int GL_LoadTexture2(char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal, int filter)
{
	if (textureType == GLT_STUDIO && iType == TEX_TYPE_NONE && pPal == tmp_palette)
	{
		iType = TEX_TYPE_ALPHA;
	}

	return gRefFuncs.GL_LoadTexture2(identifier, textureType, width, height, data, mipmap, iType, pPal, filter);
}

int GL_LoadTextureEx(const char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, qboolean ansio)
{
	int texnum = GL_AllocTexture((char *)identifier, textureType, width, height, mipmap);

	if(!texnum)
		return 0;

	int iTextureTarget = GL_TEXTURE_2D;

	if (gl_loadtexture_cubemap)
		iTextureTarget = GL_TEXTURE_CUBE_MAP;

	int iWrap = GL_REPEAT;

	if (textureType == GLT_HUDSPRITE || textureType == GLT_SPRITE)
		iWrap = GL_CLAMP_TO_EDGE;

	glBindTexture(iTextureTarget, texnum);
	(*currenttexture) = -1;

	if (gl_loadtexture_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT || gl_loadtexture_format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT || gl_loadtexture_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
	{
		GL_UploadDXT(data, width, height, mipmap, ansio, iWrap);
	}
	else
	{
		GL_Upload32((unsigned *)data, width, height, mipmap, ansio, iWrap);
	}

	glBindTexture(iTextureTarget, 0);

	return texnum;
}

texture_t *Draw_DecalTexture(int index)
{
	texture_t *t = gRefFuncs.Draw_DecalTexture(index);

	return t;
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

int LoadDDS(const char *filename, byte *buf, int bufsize, int *width, int *height)
{
	dds_header_t header;
	int fileSize;

	if (width)
		*width = 0;

	if (height)
		*height = 0;

	FileHandle_t fileHandle = g_pFileSystem->Open(filename, "rb");

	if(!fileHandle)
		return FALSE;

	fileSize = g_pFileSystem->Size(fileHandle);

	if(!g_pFileSystem->Read((void *)&header, sizeof(dds_header_t), fileHandle))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DXT image.\n", filename);
		g_pFileSystem->Close(fileHandle);
		return FALSE;
	}

	DWORD iFlags = ByteToUInt(header.bFlags);
	DWORD iMagic = ByteToUInt(header.bMagic);
	DWORD iFourCC = ByteToUInt(header.bPFFourCC);
	DWORD iPFFlags = ByteToUInt(header.bPFFlags);
	DWORD iLinSize = ByteToUInt(header.bPitchOrLinearSize);
	DWORD iSize = ByteToUInt(header.bSize);

	int w = ByteToUInt(header.bWidth);
	int h = ByteToUInt(header.bHeight);

	if(iMagic != DDS_MAGIC)
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DXT image.\n", filename);
		g_pFileSystem->Close(fileHandle);
		return FALSE;
	}

	if(iSize != 124)
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DXT image.\n", filename);
		g_pFileSystem->Close(fileHandle);
		return FALSE;
	}

	if(!(iFlags & DDSD_PIXELFORMAT))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DXT image.\n", filename);
		g_pFileSystem->Close(fileHandle);
		return FALSE;
	}

	if(!(iFlags & DDSD_CAPS))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DXT image.\n", filename);
		g_pFileSystem->Close(fileHandle);
		return FALSE;
	}

	if(!(iPFFlags & DDPF_FOURCC))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is not a DXT image.\n", filename);
		g_pFileSystem->Close(fileHandle);
		return FALSE;
	}

	if(iFourCC != D3DFMT_DXT1 && iFourCC != D3DFMT_DXT3 && iFourCC != D3DFMT_DXT5)
	{
		gEngfuncs.Con_Printf("LoadDDS: Texture %s is not DXT1/3/5 format!\n", filename);
		g_pFileSystem->Close(fileHandle);
		return FALSE;
	}

	if((int)iLinSize > bufsize)
	{
		gEngfuncs.Con_Printf("LoadDDS: Texture %s is too large!\n", filename);
		g_pFileSystem->Close(fileHandle);
		return FALSE;
	}

	g_pFileSystem->Seek(fileHandle, 128, FILESYSTEM_SEEK_HEAD);

	if(!g_pFileSystem->Read((void *)buf, iLinSize, fileHandle))
	{
		gEngfuncs.Con_Printf("LoadDDS: File %s is corrupted.\n", filename);
		g_pFileSystem->Close(fileHandle);
		return FALSE;
	}

	if(iFourCC == D3DFMT_DXT1)
		gl_loadtexture_format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
	else if(iFourCC == D3DFMT_DXT3)
		gl_loadtexture_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	else if(iFourCC == D3DFMT_DXT5)
		gl_loadtexture_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

	gl_loadtexture_size = iLinSize;

	if(width)
		*width = w;
	if(height)
		*height = h;

	g_pFileSystem->Close(fileHandle);
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
	if(g_pFileSystem->Read(buffer, size*count, handle))
		return count;
	return 0;
}

unsigned WINAPI FI_Write(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
	if(g_pFileSystem->Write(buffer, size*count, handle))
		return count;
	return 0;
}

int WINAPI FI_Seek(fi_handle handle, long offset, int origin)
{
	g_pFileSystem->Seek(handle, offset, (FileSystemSeek_t)origin);
	return 0;
}

long WINAPI FI_Tell(fi_handle handle)
{
	return g_pFileSystem->Tell(handle);
}

int LoadImageGeneric(const char *filename, byte *buf, int bufSize, int *width, int *height)
{
	FileHandle_t fileHandle = g_pFileSystem->Open(filename, "rb");

	if(!fileHandle)
		return FALSE;

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
		gEngfuncs.Con_Printf("LoadImageGeneric: %s Unsupported format.\n", filename);
		return FALSE;
    }

	if(!FreeImage_FIFSupportsReading(fiFormat))
    {
		gEngfuncs.Con_Printf("LoadImageGeneric: %s Unsupported format.\n", filename);
		return FALSE;
    }

	FIBITMAP *fiB = FreeImage_LoadFromHandle(fiFormat, &fiIO, (fi_handle)fileHandle);

	g_pFileSystem->Close(fileHandle);

	if(!fiB)
		return FALSE;

	int pos, w, h, blockSize;

	pos = 0;
	w = FreeImage_GetWidth(fiB);
	h = FreeImage_GetHeight(fiB);
	blockSize = FreeImage_GetLine(fiB) / w;

	if(w * h * 4 > bufSize)
	{
		FreeImage_Unload(fiB);
		return FALSE;
	}

	for( int y = 0; y < h; ++y )
	{
		BYTE *bits = FreeImage_GetScanLine(fiB, h-y-1);
		for( int x = 0; x < w; ++x )
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

	gl_loadtexture_format = GL_RGBA;
	gl_loadtexture_size = pos;

	FreeImage_Unload(fiB);
	return TRUE;
}

int SaveImageGeneric(const char *filename, int width, int height, byte *data)
{
	const char *extension = V_GetFileExtension(filename);

	FREE_IMAGE_FORMAT fiFormat = FreeImage_GetFIFFromFilename(filename);

	if(fiFormat == FIF_UNKNOWN)
	{
		gEngfuncs.Con_Printf("SaveImageGeneric: %s Unsupported format.\n", filename);
		return FALSE;  
	}

	if(!FreeImage_FIFSupportsWriting(fiFormat))
    {
		gEngfuncs.Con_Printf("SaveImageGeneric: %s Unsupported format.\n", filename);
		return FALSE;
    }

	FileHandle_t fileHandle = g_pFileSystem->Open(filename, "wb");

	if(!fileHandle)
    {
		gEngfuncs.Con_Printf("SaveImageGeneric: Cannot open %s for writing.\n", filename);
		return FALSE;  
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
		gEngfuncs.Con_Printf("SaveImageGeneric: Cannot save %s.\n", filename);
		g_pFileSystem->Close(fileHandle);
		FreeImage_Unload(fiB);
        return FALSE;
    }

	g_pFileSystem->Close(fileHandle);
	FreeImage_Unload(fiB);
	return TRUE;
}

int R_LoadTextureEx(const char *filepath, const char *name, int *width, int *height, GL_TEXTURETYPE type, qboolean mipmap, qboolean ansio)
{
	int w, h;

	const char *extension = V_GetFileExtension(filepath);

	if(!extension)
	{
		gEngfuncs.Con_Printf("R_LoadTextureEx: File %s has no extension.\n", filepath);
		return 0;
	}

	if(!stricmp(extension, "dds"))
	{
		if(LoadDDS(filepath, texloader_buffer, sizeof(texloader_buffer), &w, &h))
		{
			if(width)
				*width = w;
			if(height)
				*height = h;

			return GL_LoadTextureEx(name, type, w, h, texloader_buffer, mipmap, ansio);
		}
	}
	else if(LoadImageGeneric(filepath, texloader_buffer, sizeof(texloader_buffer), &w, &h))
	{
		if(width)
			*width = w;
		if(height)
			*height = h;

		return GL_LoadTextureEx(name, type, w, h, texloader_buffer, mipmap, ansio);
	}

	gEngfuncs.Con_Printf("R_LoadTextureEx: Cannot load texture %s.\n", filepath);
	return 0;
}