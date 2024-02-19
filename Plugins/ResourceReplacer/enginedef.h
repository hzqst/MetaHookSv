#pragma once

#include <com_model.h>

#define	MAX_QPATH		64			// max length of a quake game pathname

typedef struct sfx_s
{
	char 	name[MAX_QPATH];
	cache_user_t	cache;
	int		servercount;
}sfx_t;

typedef struct sfxcache_s
{
	int 	length;
	int 	loopstart;
	int 	speed;
	int 	width;
	int 	stereo;
	byte	data[1];		// variable sized
} sfxcache_t;

typedef struct
{
	sfx_t* sfx;			// sfx number
	int		leftvol;		// 0-255 volume
	int		rightvol;		// 0-255 volume
	int		end;			// end time in global paintsamples
	int 	pos;			// sample position in sfx
	int		looping;		// where to loop, -1 = no looping
	int		entnum;			// to allow overriding a specific sound
	int		entchannel;		//
	vec3_t	origin;			// origin of sound effect
	vec_t	dist_mult;		// distance multiplier (attenuation/clipK)
	int		master_vol;		// 0-255 master volume
	int		isentence;
	int		iword;
	int		pitch;			// real-time pitch after any modulation or shift by dynamic data
} channel_t;