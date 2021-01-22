#include "gl_local.h"
#include "command.h"

extern "C"
{
#include "FreeImage/FreeImage.h"
};

#pragma comment(lib,"FreeImage/FreeImage.lib")

GLenum TEXTURE0_SGIS;
GLenum TEXTURE1_SGIS;
GLenum TEXTURE2_SGIS;
GLenum TEXTURE3_SGIS;

xcommand_t gl_texturemode_function;

float current_ansio = -1.0;

static byte texloader_buffer[2048*2048*4];
gltexture_t *gltextures;
int *numgltextures;
int *gHostSpawnCount;
int *currenttexid;//for 3xxx~4xxx
int *currenttexture;
//for renderer
gltexture_t *currentglt;
static byte scaled_buffer[1024*1024*4];
int gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
int gl_filter_max = GL_LINEAR;

int gl_loadtexture_format = GL_RGBA;
int gl_loadtexture_size;

//GL Start

GLuint GL_GenTexture(void)
{
	GLuint tex;
	if(g_dwEngineBuildnum < 5953)
	{
		tex = (*currenttexid);
		(*currenttexid) ++;
	}
	else
	{
		qglGenTextures(1, &tex);
	}
	return tex;
}

void GL_DeleteTexture(GLuint tex)
{
	qglDeleteTextures(1, &tex);
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

typedef struct
{
	char *name;
	int minimize, maximize;
}glmode_t;

glmode_t modes[] =
{
	{ "GL_NEAREST", GL_NEAREST, GL_NEAREST },
	{ "GL_LINEAR", GL_LINEAR, GL_LINEAR },
	{ "GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR },
	{ "GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR }
};

void Draw_TextureMode_f(void)
{
	int i;

	if (gEngfuncs.Cmd_Argc() == 1)
	{
		for (i = 0; i < 6; i++)
		{
			if (gl_filter_min == modes[i].minimize)
			{
				gEngfuncs.Con_Printf("%s\n", modes[i].name);
				return;
			}
		}

		gEngfuncs.Con_Printf("current filter is unknown???\n");
		return;
	}

	for (i = 0; i < 6; i++)
	{
		if (!stricmp(modes[i].name, gEngfuncs.Cmd_Argv(1)))
			break;
	}

	if (i == 6)
	{
		gEngfuncs.Con_Printf("bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;
}

void Draw_UpdateAnsios(void)
{
	if (gl_ansio->value != current_ansio)
	{
		for (int i = 0; i < 6; i++)
		{
			if (gl_filter_min == modes[i].minimize)
			{
				char cmd[64];
				sprintf(cmd, "gl_texturemode %s\n", modes[i].name);
				gEngfuncs.pfnClientCmd(cmd);
				break;
			}
		}

		current_ansio = gl_ansio->value;
	}
}

void Draw_Init(void)
{
	gl_texturemode_function = Cmd_HookCmd("gl_texturemode", Draw_TextureMode_f);
	Cmd_HookCmd("screenshot", CL_ScreenShot_f);
}

byte *R_GetTexLoaderBuffer(int *bufsize)
{
	if(bufsize)
		*bufsize = sizeof(texloader_buffer);
	return texloader_buffer;
}

//Texture resampler

/*void GL_ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight)
{
	int i, j;
	unsigned *inrow, *inrow2;
	unsigned frac, fracstep;
	unsigned p1[1024], p2[1024];
	byte *pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth * 0x10000 / outwidth;
	frac = fracstep >> 2;

	for (i = 0; i < outwidth; i++)
	{
		p1[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	frac = 3 * (fracstep >> 2);

	for (i = 0; i < outwidth; i++)
	{
		p2[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	for (i = 0; i < outheight; i++, out += outwidth)
	{
		inrow = in + inwidth * (int)((i + 0.25) * inheight / outheight);
		inrow2 = in + inwidth * (int)((i + 0.75) * inheight / outheight);

		frac = fracstep >> 1;

		for (j = 0; j < outwidth; j++)
		{
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out + j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
			((byte *)(out + j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
			((byte *)(out + j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
			((byte *)(out + j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
		}
	}
}

void GL_ResampleTexturePoint(unsigned char *in, int inwidth, int inheight, unsigned char *out, int outwidth, int outheight)
{
	int i, j;
	unsigned ufrac, vfrac;
	unsigned ufracstep, vfracstep;
	unsigned char *src, *dest;

	src = in;
	dest = out;
	ufracstep = inwidth * 0x10000 / outwidth;
	vfracstep = inheight * 0x10000 / outheight;

	vfrac = vfracstep >> 2;

	for (i = 0; i < outheight; i++, out += outwidth)
	{
		ufrac = ufracstep >> 2;

		for (j = 0; j < outwidth; j++)
		{
			*dest = src[ufrac >> 16];
			ufrac += ufracstep;
			dest++;
		}

		vfrac += vfracstep;
		src += inwidth * (vfrac >> 16);
		vfrac = vfrac & 0xFFFF;
	}
}

void GL_MipMap(byte *in, int width, int height)
{
	int i, j;
	byte *out;

	width <<=2;
	height >>= 1;
	out = in;

	for (i = 0; i < height; i++, in += width)
	{
		for (j = 0; j < width; j += 8, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4] + in[width + 0] + in[width + 4]) >> 2;
			out[1] = (in[1] + in[5] + in[width + 1] + in[width + 5]) >> 2;
			out[2] = (in[2] + in[6] + in[width + 2] + in[width + 6]) >> 2;
			out[3] = (in[3] + in[7] + in[width + 3] + in[width + 7]) >> 2;
		}
	}
}

void ComputeScaledSize(int *wscale, int *hscale, int width, int height)
{
	int scaled_width, scaled_height;
	int max_size;

	max_size = max(128, (int)gl_max_size->value);

	for (scaled_width = 1; scaled_width < width; scaled_width <<= 1) {}

	if (gl_round_down->value > 0 && width < scaled_width && (gl_round_down->value == 1 || (scaled_width - width) > (scaled_width >> (int)gl_round_down->value)))
		scaled_width >>= 1;

	for (scaled_height = 1; scaled_height < height; scaled_height <<= 1) {}

	if (gl_round_down->value > 0 && height < scaled_height && (gl_round_down->value == 1 || (scaled_height - height) > (scaled_height >> (int)gl_round_down->value)))
		scaled_height >>= 1;

	if (wscale)
		*wscale = min(scaled_width >> (int)gl_picmip->value, max_size);

	if (hscale)
		*hscale = min(scaled_height >> (int)gl_picmip->value, max_size);
}*/

void GL_Upload32(unsigned int *data, int width, int height, qboolean mipmap, qboolean ansio)
{
	int iComponent, iFormat;
	//int scaled_width, scaled_height;

	//ComputeScaledSize(&scaled_width, &scaled_height, width, height);

	//if (scaled_height * scaled_width > sizeof(scaled_buffer) / 4)
	//	Sys_ErrorEx("GL_Upload32: Texture is too large!");

	//if(gl_loadtexture_format != GL_RGBA)
	//	Sys_ErrorEx("GL_Upload32: Only RGBA is supported!");

	iFormat = GL_RGBA;
	iComponent = (g_iBPP == 16) ? GL_RGB5_A1 : GL_RGBA8;

	qglTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, (mipmap) ? GL_TRUE : GL_FALSE);

	if (mipmap)
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		qglTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		qglTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	}

	if(ansio && gl_ansio)
	{
		if(gl_force_ansio)
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_force_ansio);
		else
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max(min(gl_ansio->value, gl_max_ansio), 1));
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
	}

	qglTexImage2D(GL_TEXTURE_2D, 0, iComponent, width, height, 0, iFormat, GL_UNSIGNED_BYTE, data);

	/*if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
			qglTexImage2D(GL_TEXTURE_2D, 0, iComponent, width, height, 0, iFormat, GL_UNSIGNED_BYTE, data);
			return;
		}

		memcpy(scaled_buffer, data, width * height * 4);
	}
	else
	{
		GL_ResampleTexture(data, width, height, (unsigned int *)scaled_buffer, scaled_width, scaled_height);
	}

	qglTexImage2D(GL_TEXTURE_2D, 0, iComponent, scaled_width, scaled_height, 0, iFormat, GL_UNSIGNED_BYTE, scaled_buffer);*/

	/*if (mipmap)
	{
		int miplevel = 0;

		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap((byte *)scaled_buffer, scaled_width, scaled_height);

			scaled_width >>= 1;
			scaled_height >>= 1;

			if (scaled_width < 1)
				scaled_width = 1;

			if (scaled_height < 1)
				scaled_height = 1;

			miplevel++;

			qglTexImage2D(GL_TEXTURE_2D, miplevel, iComponent, scaled_width, scaled_height, 0, iFormat, GL_UNSIGNED_BYTE, scaled_buffer);
		}
	}*/
}

void GL_Upload16(byte *data, int width, int height, qboolean mipmap, int iType, unsigned char *pPal, qboolean ansio)
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

	GL_Upload32(trans, width, height, mipmap, ansio);
}

int GL_AllocTexture(char *identifier, GL_TEXTURETYPE textureType, int width, int height, qboolean mipmap)
{
	int i;
	gltexture_t *glt;
	gltexture_t *slot;

	glt = NULL;

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

				currentglt = slot;
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
		glt = &gltextures[(*numgltextures)];
		(*numgltextures)++;

		if (*numgltextures >= MAX_GLTEXTURES)
		{
			gEngfuncs.Con_Printf("Texture Overflow: MAX_GLTEXTURES\n");
			return 0;
		}
	}

	if (!glt->texnum)
		glt->texnum = GL_GenTexture();

	strncpy(glt->identifier, identifier, sizeof(glt->identifier) - 1);
	glt->identifier[sizeof(glt->identifier) - 1] = 0;
	glt->width = width;
	glt->height = height;
	glt->mipmap = mipmap;
	glt->servercount = (textureType == GLT_WORLD) ? *gHostSpawnCount : 0;
	glt->paletteIndex = -1;

	currentglt = glt;

	return glt->texnum;
}

int GL_LoadTexture2(char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal, int filter)
{
	if(!qglTexParameterf) 
	{
		return gRefFuncs.GL_LoadTexture2(identifier, textureType, width, height, data, mipmap, iType, pPal, filter);
	}
	/*if (!mipmap || textureType != GLT_WORLD)
	{
		if(qglTexParameterf)//just in case of crashing when called before initalize QGL
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
		return gRefFuncs.GL_LoadTexture2(identifier, textureType, width, height, data, mipmap, iType, pPal, filter);
	}

	Draw_UpdateAnsios();

	if(qglTexParameterf)
	{
		if(gl_force_ansio)
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_force_ansio);
		else
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max(min(gl_ansio->value, gl_max_ansio), 1));
	}*/

	int texnum = GL_AllocTexture((char *)identifier, textureType, width, height, mipmap);

	if(!texnum)
		return 0;

	GL_Bind(texnum);
	(*currenttexture) = -1;

	qboolean ansio = (textureType == GLT_WORLD || textureType == GLT_SPRITE || textureType == GLT_STUDIO) ? true : false;

	if ( iType == TEX_TYPE_RGBA && textureType == GLT_SPRITE )
		GL_Upload32( (unsigned *)data, width, height, mipmap, ansio );
	else
		GL_Upload16( data, width, height, mipmap, iType, pPal, ansio );

	return texnum;

	//return gRefFuncs.GL_LoadTexture2(identifier, textureType, width, height, data, mipmap, iType, pPal, filter);
}

void GL_UploadDXT(byte *data, int width, int height, qboolean mipmap, qboolean ansio)
{
	if (mipmap)
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		qglTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		qglTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	}

	if(ansio)
	{
		if(gl_force_ansio)
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_force_ansio);
		else
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max(min(gl_ansio->value, gl_max_ansio), 1));
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
	}		

	qglCompressedTexImage2DARB(GL_TEXTURE_2D, 0, gl_loadtexture_format, width, height, 0, gl_loadtexture_size, data);
}

int GL_LoadTextureEx(const char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, qboolean ansio)
{
	int scaled_width, scaled_height;
	qboolean rescale;
	byte *pTexture;
	int texnum;

	texnum = GL_AllocTexture((char *)identifier, textureType, width, height, mipmap);

	if(!texnum)
		return 0;

	GL_Bind(texnum);
	(*currenttexture) = -1;

	if(gl_s3tc_compression_support && (gl_loadtexture_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT || gl_loadtexture_format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT || gl_loadtexture_format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT))
	{
		GL_UploadDXT(data, width, height, mipmap, ansio);
		return texnum;
	}

	//ComputeScaledSize(&scaled_width, &scaled_height, width, height);
	//rescale = (scaled_width == width && scaled_height == height) ? false : true;

	//pTexture = data;
	//if (!mipmap && rescale && scaled_width <= 128 && scaled_height <= 128)
	//{
	//	GL_ResampleTexturePoint(data, width, height, scaled_buffer, scaled_width, scaled_height);
	//	pTexture = scaled_buffer;
	//}

	//if (pTexture)
	//{
		GL_Upload32((unsigned *)data, width, height, mipmap, ansio);
	//}

	return texnum;
}

gltexture_t *R_GetCurrentGLTexture(void)
{
	return currentglt;
}

extern cvar_t *r_wsurf_decal;

texture_t *Draw_DecalTexture(int index)
{
	texture_t *t = gRefFuncs.Draw_DecalTexture(index);
	if(index < 0)
		return t;
	if(t->anim_next && r_wsurf_decal->value)
		return t->anim_next;

	return t;
}

void Draw_MiptexTexture(cachewad_t *wad, byte *data)
{
	gRefFuncs.Draw_MiptexTexture(wad, data);

	texture_t *t = (texture_t *)data;
	R_LinkDecalTexture(t);
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
		gl_loadtexture_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
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

	if(!stricmp(extension, "dds") && gl_s3tc_compression_support)
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