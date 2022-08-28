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

gltexture_t *gltextures_get();

//DXT

#define DDS_MAGIC 0x20534444

#define DDSD_CAPS                   0x00000001
#define DDSD_PIXELFORMAT            0x00001000
#define DDPF_FOURCC                 0x00000004
#define DDSCAPS_MIPMAP 0x400000

#define D3DFMT_DXT1     '1TXD'    //  DXT1 compression texture format 
#define D3DFMT_DXT3     '3TXD'    //  DXT5 compression texture format 
#define D3DFMT_DXT5     '5TXD'    //  DXT5 compression texture format 

#define DIB_HEADER_MARKER ((WORD)('M' << 8) | 'B')

#define SIZE_OF_DXT1(width, height)    ( max(1, ( (width + 3) >> 2 ) ) * max(1, ( (height + 3) >> 2 ) ) * 8 )
#define SIZE_OF_DXT2(width, height)    ( max(1, ( (width + 3) >> 2 ) ) * max(1, ( (height + 3) >> 2 ) ) * 16 )

typedef struct {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwFourCC;
	DWORD dwRGBBitCount;
	DWORD dwRBitMask;
	DWORD dwGBitMask;
	DWORD dwBBitMask;
	DWORD dwABitMask;
}DDS_PIXELFORMAT;

typedef struct
{
	DWORD           dwSize;
	DWORD           dwFlags;
	DWORD           dwHeight;
	DWORD           dwWidth;
	DWORD           dwPitchOrLinearSize;
	DWORD           dwDepth;
	DWORD           dwMipMapCount;
	DWORD           dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	DWORD           dwCaps;
	DWORD           dwCaps2;
	DWORD           dwCaps3;
	DWORD           dwCaps4;
	DWORD           dwReserved2;
}DDS_HEADER;

typedef struct
{
	DWORD        dwMagic;
	DDS_HEADER    Header;
} DDS_FILEHEADER;

int GL_AllocTexture(char *identifier, GL_TEXTURETYPE textureType, int width, int height, qboolean mipmap);
int GL_FindTexture(const char *identifier, GL_TEXTURETYPE textureType, int *width, int *height);
const char * V_GetFileExtension(const char * path);
const char * V_UnqualifiedFileName(const char * in);