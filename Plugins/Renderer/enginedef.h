#pragma once

#define	MAX_QPATH 64

#pragma pack(1)
typedef struct lumpinfo_s
{
	int filepos;
	int disksize;
	int size;
	char type;
	char compression;
	char pad1, pad2;
	char name[16];
}lumpinfo_t;
#pragma pack()

typedef struct
{
	char name[MAX_QPATH];
	cache_user_t cache;
}cacheentry_t;

typedef void (*PFNCACHE)(struct cachewad_s *wad, byte *data);

typedef struct cachewad_s
{
	char *name;
	cacheentry_t *cache;
	int cacheCount;
	int cacheMax;
	lumpinfo_t *lumps;
	int lumpCount;
	int cacheExtra;
	PFNCACHE pfnCacheBuild;
	int numpaths;
	char **basedirs;
	int *lumppathindices;
	qboolean tempWad;
}cachewad_t;

typedef struct
{
	cl_entity_t *pEnt;
	float distance;
}transObjRef;

typedef struct glRect_s
{
	unsigned char l, t, w, h;
}glRect_t;

typedef struct glRect_SvEngine_s
{
	unsigned int l, t, w, h;
}glRect_SvEngine_t;

typedef struct FBO_Container_s
{
	GLuint s_hBackBufferFBO;
	GLuint s_hBackBufferCB;
	GLuint s_hBackBufferDB;
	GLuint s_hBackBufferTex;
	GLuint s_hBackBufferTex2;
	GLuint s_hBackBufferTex3;
	GLuint s_hBackBufferTex4;
	GLuint s_hBackBufferDepthTex;
	int iWidth;
	int iHeight;
	int iTextureColorFormat;
}FBO_Container_t;

#define MAX_CLIENTS 32
#define MAX_WEAPONS 64

#define MULTIPLAYER_BACKUP 64

#define MAX_PACKET_ENTITIES 256

#include <entity_state.h>
#include <weaponinfo.h>

typedef struct
{
	int num_entities;
	unsigned char flags[32];
	entity_state_t *entities;
}
packet_entities_t;

typedef struct
{
	double receivedtime;
	double latency;
	qboolean invalid;
	qboolean choked;
	entity_state_t playerstate[MAX_CLIENTS];
	double time;
	clientdata_t clientdata;
	weapon_data_t weapondata[64];
	packet_entities_t packet_entities;
	unsigned short clientbytes;
	unsigned short playerinfobytes;
	unsigned short packetentitybytes;
	unsigned short tentitybytes;
	unsigned short soundbytes;
	unsigned short eventbytes;
	unsigned short usrbytes;
	unsigned short voicebytes;
	unsigned short msgbytes;
}frame_t;

#define TEX_TYPE_NONE	0
#define TEX_TYPE_ALPHA	1
#define TEX_TYPE_LUM	2
#define TEX_TYPE_ALPHA_GRADIENT 3
#define TEX_TYPE_RGBA	4

#define TEX_IS_ALPHA(type)		((type)==TEX_TYPE_ALPHA||(type)==TEX_TYPE_ALPHA_GRADIENT||(type)==TEX_TYPE_RGBA)
