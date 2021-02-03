#pragma once

#define MAX_GLTEXTURES 4800

#pragma pack(1)
typedef struct gltexture_s
{
	int texnum;
	short servercount;
	short paletteIndex;
	int width;
	int height;
	qboolean mipmap;
	char identifier[64];
}gltexture_t;
#pragma pack()

typedef enum
{
	GLT_SYSTEM,
	GLT_DECAL,
	GLT_HUDSPRITE,
	GLT_STUDIO,
	GLT_WORLD,
	GLT_SPRITE
}GL_TEXTURETYPE;

extern GLenum TEXTURE0_SGIS;
extern GLenum TEXTURE1_SGIS;
extern GLenum TEXTURE2_SGIS;
extern GLenum TEXTURE3_SGIS;

extern gltexture_t *gltextures;
extern gltexture_t **gltextures_SvEngine;
extern int *maxgltextures_SvEngine;
extern int *numgltextures;
extern int *gHostSpawnCount;
extern int *currenttexid;;
extern int *currenttexture;
extern gltexture_t *currentglt;

extern float gl_max_ansio;
extern float gl_force_ansio;

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