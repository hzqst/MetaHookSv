#pragma once

#define CVOXWORDMAX 32
#define CVOXSENTENCEMAX 16
#define CVOXZEROSCANMAX 255

#define CVOXFILESENTENCEMAX 1536

#define SND_VOLUME		(1<<0)		// a byte 1
#define SND_ATTENUATION	(1<<1)		// a byte 2
#define SND_LARGE_INDEX	(1<<2)		// a long 4
#define SND_PITCH		(1<<3)		//8
#define SND_SENTENCE	(1<<4)		//0x10
#define SND_STOP		(1<<5)		//0x20
#define SND_CHANGE_VOL	(1<<6)		//0x40
#define SND_CHANGE_PITCH	(1<<7)	//0x80
#define SND_SPAWNING	(1<<8)		//0x100

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
	sfx_t	*sfx;			// sfx number
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

//Meta Audio
typedef struct
{
	//wave info
	int 	length;
	int 	loopstart;
	int 	speed;
	int 	width;
	int 	channels;
	int		dataofs;
	int		bitrate;
	int		blockalign;
	//for OpenAL buffer
	int		alformat;
	unsigned int	albuffer;
	//for Stream sound
	FileHandle_t file;
	int		filesize;
	//data chunk so we could do some magic change on the raw sound data
	int		datalen;
	byte	data[1];
}aud_sfxcache_t;

typedef struct
{
	//wave info
	unsigned long long length;
	unsigned long long loopstart;
	unsigned long long loopend;
	unsigned long samplerate;
	bool looping;
	bool force_streaming;
}aud_sfxcache_LAGonauta_t;

typedef struct voxword
{
	int volume;
	int pitch;
	int start;
	int end;
	int cbtrim;
	int fKeepCached;
	int samplefrac;
	int timecompress;
	sfx_t *sfx;
}
voxword_t;
