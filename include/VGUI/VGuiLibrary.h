#ifndef IVGUILIBRARY_H
#define IVGUILIBRARY_H

#pragma pack(1)

typedef struct
{
	const char* name;
	int ping;
	int packetloss;
	bool thisplayer;
	const char* teamname;
	int teamnumber;
	int frags;
	int deaths;
	int playerclass;
	int health;
	bool dead;
	uint64 m_nSteamID;
}
VGuiLibraryPlayer_t;

#pragma pack()

typedef struct
{
	const char* name;
	int frags;
	int deaths;
	int ping;
	int players;
	int packetloss;
	int teamnumber;
}
VGuiLibraryTeamInfo_t;

typedef int (*ININTERMISSION)(void);
typedef void (*SPECTATOR_FINDNEXTPLAYER)(bool);
typedef void (*SPECTATOR_FINDPLAYER)(const char*);
typedef int (*SPECTATOR_PIPINSETOFF)(void);
typedef void (*SPECTATOR_INSETVALUES)(int*, int*, int*, int*);
typedef void (*SPECTATOR_CHECKSETTINGS)(void);
typedef int (*SPECTATOR_NUMBER)(void);
typedef bool (*SPECTATOR_ISSPECTATEONLY)(void);
typedef float (*HUDTIME)(void);
typedef void (*MESSAGE_ADD)(void*);
typedef void (*MESSAGE_HUD)(const char*, int, void*);
typedef int (*TEAMPLAY)(void);
typedef void (*CLIENTCMD)(const char*);
typedef int (*TEAMNUMBER)(void);
typedef const char* (*GETLEVELNAME)(void);
typedef char* (*COMPARSEFILE)(char*, char*);
typedef void* (*COMLOADFILE)(char*, int, int*);
typedef void (*COMFREEFILE)(void*);
typedef void (*COMFILEBASE)(char*, char*);
typedef void (*CONDPRINTF)(char*, ...);
typedef VGuiLibraryPlayer_t(*GETPLAYERINFO)(int);
typedef int (*GETMAXPLAYERS)(void);
typedef bool (*SPECTATOR_ISSPECTATING)(void);
typedef int (*SPECTATOR_SPECTATORMODE)(void);
typedef int (*SPECTATOR_SPECTATORTARGET)(void);
typedef int (*GETLOCALPLAYERINDEX)(void);
typedef void (*VOICESTOPSQUELCH)(void);
typedef bool (*DEMOPLAYBACK)(void);
typedef float (*CVARGETFLOAT)(const char*);

typedef struct
{
	ININTERMISSION InIntermission;
	SPECTATOR_FINDNEXTPLAYER FindNextPlayer;
	SPECTATOR_FINDPLAYER FindPlayer;
	SPECTATOR_PIPINSETOFF PipInsetOff;
	SPECTATOR_INSETVALUES InsetValues;
	SPECTATOR_CHECKSETTINGS CheckSettings;
	SPECTATOR_NUMBER SpectatorNumber;
	SPECTATOR_ISSPECTATING IsSpectator;
	SPECTATOR_SPECTATORMODE SpectatorMode;
	SPECTATOR_SPECTATORTARGET SpectatorTarget;
	SPECTATOR_ISSPECTATEONLY IsSpectateOnly;
	HUDTIME HudTime;
	MESSAGE_ADD MessageAdd;
	MESSAGE_HUD MessageHud;
	TEAMPLAY TeamPlay;
	CLIENTCMD CallEnghudClientCmd;
	TEAMNUMBER TeamNumber;
	GETLEVELNAME GetLevelName;
	GETLOCALPLAYERINDEX GetLocalPlayerIndex;
	COMPARSEFILE COM_ParseFile;
	COMLOADFILE COM_LoadFile;
	COMFREEFILE COM_FreeFile;
	COMFILEBASE COM_FileBase;
	CONDPRINTF Con_DPrintf;
	GETPLAYERINFO CallEnghudGetPlayerInfo;
	GETMAXPLAYERS GetMaxPlayers;
	VOICESTOPSQUELCH GameVoice_StopSquelchMode;
	DEMOPLAYBACK IsDemoPlayingBack;
	CVARGETFLOAT CvarGetFloat;
}
VGuiLibraryInterface_t;

#endif