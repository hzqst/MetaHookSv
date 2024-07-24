#pragma once

#include "bspfile.h"
#include "studio.h"
#include "com_model.h"

#define NL_PRESENT 0
#define NL_NEEDS_LOADED 1
#define NL_UNREFERENCED 2
#define NL_CLIENT 3

#define	MAX_QPATH 64

//wad

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

typedef void(*PFNCACHE)(struct cachewad_s *wad, byte *data);

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

//r_trans

typedef struct
{
	cl_entity_t *pEnt;
	float distance;
}transObjRef;

//gl_rsurf

typedef struct glRect_GoldSrc_s
{
	unsigned char l, t, w, h;
}glRect_GoldSrc_t;

typedef struct glRect_SvEngine_s
{
	int l, t, w, h;
}glRect_SvEngine_t;

typedef struct vrect_s
{
	int x, y, width, height;
	struct vrect_s *pnext;
}vrect_t;

typedef struct
{
	int r, g, b;
}mcolor24_t;

typedef struct refdef_s
{
	vec3_t vieworg;
	vec3_t viewangles;
	color24 ambientlight;
	qboolean onlyClientDraws;

	//Only available in SvEngine
	qboolean useCamera;
	vec3_t r_camera_origin;
}refdef_t;

typedef struct skybox_s
{
	float v[2][6];
}skybox_t;

//client

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

//gl_draw

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

//gl_studio

#define STUDIO_RENDER 1
#define STUDIO_EVENTS 2
#define STUDIO_RAGDOLL 4

// lighting options
#define STUDIO_NF_FLATSHADE		0x0001
#define STUDIO_NF_CHROME		0x0002
#define STUDIO_NF_FULLBRIGHT	0x0004
#define STUDIO_NF_NOMIPS        0x0008
#define STUDIO_NF_ALPHA         0x0010
#define STUDIO_NF_ADDITIVE      0x0020
#define STUDIO_NF_MASKED        0x0040

//gl_model

#include "modelgen.h"
#include "spritegn.h"

#define SIDE_FRONT 0
#define SIDE_BACK 1
#define SIDE_ON 2

#define SURF_PLANEBACK 2
#define SURF_DRAWSKY 4
#define SURF_DRAWSPRITE 8
#define SURF_DRAWTURB 0x10
#define SURF_DRAWTILED 0x20
#define SURF_DRAWBACKGROUND 0x40
#define SURF_UNDERWATER 0x80
#define SURF_DONTWARP 0x100

typedef struct decalcache_s
{
	int		decalIndex;
	float	decalVert[4][VERTEXSIZE];
} decalcache_t;

typedef struct mspriteframe_s
{
	int width;
	int height;
	float up, down, left, right;
	int gl_texturenum;
}
mspriteframe_t;

typedef struct
{
	int numframes;
	float *intervals;
	mspriteframe_t *frames[1];
}
mspritegroup_t;

typedef struct
{
	spriteframetype_t type;
	mspriteframe_t *frameptr;
}
mspriteframedesc_t;

typedef struct msprite_s
{
	short type;
	short texFormat;
	int maxwidth;
	int maxheight;
	int numframes;
	int paloffset;
	float beamlength;
	void *cachespot;
	mspriteframedesc_t frames[1];
}
msprite_t;

typedef struct
{
	int firstpose;
	int numposes;
	float interval;
	trivertx_t bboxmin;
	trivertx_t bboxmax;
	int frame;
	char name[16];
}
maliasframedesc_t;

typedef struct
{
	trivertx_t bboxmin;
	trivertx_t bboxmax;
	int frame;
}
maliasgroupframedesc_t;

typedef struct
{
	int numframes;
	int intervals;
	maliasgroupframedesc_t frames[1];
}
maliasgroup_t;

typedef struct mtriangle_s
{
	int facesfront;
	int vertindex[3];
}
mtriangle_t;

#define MAX_SKINS 32

typedef struct
{
	int ident;
	int version;
	vec3_t scale;
	vec3_t scale_origin;
	float boundingradius;
	vec3_t eyeposition;
	int numskins;
	int skinwidth;
	int skinheight;
	int numverts;
	int numtris;
	int numframes;
	synctype_t synctype;
	int flags;
	float size;
	int numposes;
	int poseverts;
	int posedata;
	int commands;
	int gl_texturenum[MAX_SKINS];
	maliasframedesc_t frames[1];
}
aliashdr_t;

#define MAXALIASVERTS 1024
#define MAXALIASFRAMES 256
#define MAXALIASTRIS 2048

extern aliashdr_t *pheader;
extern stvert_t stverts[MAXALIASVERTS];
extern mtriangle_t triangles[MAXALIASTRIS];
extern trivertx_t *poseverts[MAXALIASFRAMES];

#define PhyCorpseFlag (1145141919)

#define ENTINDEX_TEMPENTITY (1000000)

typedef enum
{
	ACT_RESET,
	ACT_IDLE,
	ACT_GUARD,
	ACT_WALK,
	ACT_RUN,
	ACT_FLY,
	ACT_SWIM,
	ACT_HOP,
	ACT_LEAP,
	ACT_FALL,
	ACT_LAND,
	ACT_STRAFE_LEFT,
	ACT_STRAFE_RIGHT,
	ACT_ROLL_LEFT,
	ACT_ROLL_RIGHT,
	ACT_TURN_LEFT,
	ACT_TURN_RIGHT,
	ACT_CROUCH,
	ACT_CROUCHIDLE,
	ACT_STAND,
	ACT_USE,
	ACT_SIGNAL1,
	ACT_SIGNAL2,
	ACT_SIGNAL3,
	ACT_TWITCH,
	ACT_COWER,
	ACT_SMALL_FLINCH,
	ACT_BIG_FLINCH,
	ACT_RANGE_ATTACK1,
	ACT_RANGE_ATTACK2,
	ACT_MELEE_ATTACK1,
	ACT_MELEE_ATTACK2,
	ACT_RELOAD,
	ACT_ARM,
	ACT_DISARM,
	ACT_EAT,
	ACT_DIESIMPLE,
	ACT_DIEBACKWARD,
	ACT_DIEFORWARD,
	ACT_DIEVIOLENT,
	ACT_BARNACLE_HIT,
	ACT_BARNACLE_PULL,
	ACT_BARNACLE_CHOMP,
	ACT_BARNACLE_CHEW,
	ACT_SLEEP,
	ACT_INSPECT_FLOOR,
	ACT_INSPECT_WALL,
	ACT_IDLE_ANGRY,
	ACT_WALK_HURT,
	ACT_RUN_HURT,
	ACT_HOVER,
	ACT_GLIDE,
	ACT_FLY_LEFT,
	ACT_FLY_RIGHT,
	ACT_DETECT_SCENT,
	ACT_SNIFF,
	ACT_BITE,
	ACT_THREAT_DISPLAY,
	ACT_FEAR_DISPLAY,
	ACT_EXCITED,
	ACT_SPECIAL_ATTACK1,
	ACT_SPECIAL_ATTACK2,
	ACT_COMBAT_IDLE,
	ACT_WALK_SCARED,
	ACT_RUN_SCARED,
	ACT_VICTORY_DANCE,
	ACT_DIE_HEADSHOT,
	ACT_DIE_CHESTSHOT,
	ACT_DIE_GUTSHOT,
	ACT_DIE_BACKSHOT,
	ACT_FLINCH_HEAD,
	ACT_FLINCH_CHEST,
	ACT_FLINCH_STOMACH,
	ACT_FLINCH_LEFTARM,
	ACT_FLINCH_RIGHTARM,
	ACT_FLINCH_LEFTLEG,
	ACT_FLINCH_RIGHTLEG,
	ACT_FLINCH_SMALL,
	ACT_FLINCH_LARGE,
	ACT_HOLDBOMB
}activity_e;