#pragma once

extern gltexture_t *gltextures;
extern gltexture_t **gltextures_SvEngine;
extern int *maxgltextures_SvEngine;
extern int *peakgltextures_SvEngine;
extern int *numgltextures;
extern int *gHostSpawnCount;
extern int *currenttexid;
extern int *currenttexture;
extern int *oldtarget;
extern int *gl_filter_min;
extern int *gl_filter_max;
extern cachewad_t **decal_wad;

extern float gl_max_ansio;

extern int gl_loadtexture_format;
extern int gl_loadtexture_cubemap;
extern int gl_loadtexture_size;

gltexture_t *gltextures_get();

//DXT

#define DDS_MAGIC 0x20534444

#define DDSD_CAPS                   0x00000001
#define DDSD_PIXELFORMAT            0x00001000
#define DDPF_FOURCC                 0x00000004

#define D3DFMT_DXT1     '1TXD'    //  DXT1 compression texture format 
#define D3DFMT_DXT3     '3TXD'    //  DXT5 compression texture format 
#define D3DFMT_DXT5     '5TXD'    //  DXT5 compression texture format 

#define DIB_HEADER_MARKER ((WORD)('M' << 8) | 'B')

typedef struct
{
	byte bMagic[4];
	byte bSize[4];
	byte bFlags[4];
	byte bHeight[4];
	byte bWidth[4];
	byte bPitchOrLinearSize[4];

	byte bPad1[52];

	byte bPFSize[4];
	byte bPFFlags[4];
	byte bPFFourCC[4];
}dds_header_t;

int GL_AllocTexture(char *identifier, GL_TEXTURETYPE textureType, int width, int height, qboolean mipmap);
int GL_FindTexture(const char *identifier, GL_TEXTURETYPE textureType, int *width, int *height);
const char * V_GetFileExtension(const char * path);
const char * V_UnqualifiedFileName(const char * in);