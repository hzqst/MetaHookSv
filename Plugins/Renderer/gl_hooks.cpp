
#include <metahook.h>
#include <capstone.h>
#include <set>
#include "gl_local.h"
#include <utlvector.h>
#include <SDL2/SDL_video.h>



#define R_DRAWPARTICLES_SIG_BLOB "\x83\xEC\x40\xA1\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xC0\x0B\x00\x00"


#define S_EXTRAUPDATE_SVENGINE "\xE8\x2A\x2A\x2A\x2A\x85\xC0\x75\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xD9\x05"
#define S_EXTRAUPDATE_BLOB "\xE8\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x2A\x2A\xE9\x2A\x2A\x2A\x2A\xC3"

#define R_ROTATEFORENTITY_SVENGINE "\x83\xEC\x2A\x8B\x2A\x24\x2A\x8B\x2A\x24\x2A\xD9\x00"
#define R_ROTATEFORENTITY_HL25     "\x55\x8B\xEC\x83\xEC\x20\x8B\x45\x08\x8B\x08\x8B\x50\x04\x8B\x40\x08"
#define R_ROTATEFORENTITY_NEW      "\x55\x8B\xEC\x83\xEC\x20\x8B\x45\x08\x8B\x08\x8B\x50\x04\x8B\x40\x08"
#define R_ROTATEFORENTITY_BLOB     "\x83\xEC\x20\x8B\x44\x24\x24\x8B\x08\x8B\x50\x04\x8B\x40\x08"

#define R_DECALSHOTINTERNAL_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\x8B\x54\x24\x2A\x8B\x4C\x24\x2A\x53\x8B\x5C\x24\x2A\x56\x69\xF2\xB8\x0B\x00\x00"


#define R_NEWMAP_SIG_COMMON    "\x55\x8B\xEC\x83\xEC\x2A\xC7\x45\xFC\x00\x00\x00\x00\x2A\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC"





#define R_MARKLEAVES_SIG_BLOB "\xB8\x00\x10\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\xA1"

#define R_CULLBOX_SIG_BLOB "\x53\x8B\x5C\x24\x08\x56\x57\x8B\x7C\x24\x14\xBE\x2A\x2A\x2A\x2A\x56\x57\x53\xE8"

#define R_SETUPFRAME_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x83\xEC\x18\x83\xF8\x01\x0F\x8E\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x2A\x2A\x68"
#define R_SETUPFRAME_SIG_BLOB2 "\x8B\x0D\x2A\x2A\x2A\x2A\x83\xEC\x18\x33\xC0\x83\xF9\x01\x0F\x9F\xC0\x50\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1"
#define R_SETUPFRAME_SIG_NEW "\x55\x8B\xEC\x83\xEC\x18\x8B\x0D\x2A\x2A\x2A\x2A\x33\xC0\x83\xF9\x01\x0F\x9F\xC0\x50\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1"
#define R_SETUPFRAME_SIG_NEW2 R_SETUPFRAME_SIG_BLOB2
#define R_SETUPFRAME_SIG_HL25 ""     //inlined
#define R_SETUPFRAME_SIG_SVENGINE "" //inlined




#define GL_SETMODE_SIG_BLOB "\x8B\x44\x24\x10\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\x85\xC0"

#define GL_SHUTDOWN_SIG_HL25 "\xFF\x35\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A\xFF\x30\xE8"

#define EMITWATERPOLYS_SIG_BLOB "\x83\xEC\x1C\x33\xD2\x55\x56\x8B\x74\x24\x28\x57\x8B\x46\x2C\x8B\x48\x24\x8B\x41\x44\x33\xC9\x8A\x50\x0C\x8A\x48\x0B\x52"
#define EMITWATERPOLYS_SIG_NEW2 EMITWATERPOLYS_SIG_BLOB
#define EMITWATERPOLYS_SIG_NEW "\x55\x8B\xEC\x83\xEC\x20\x56\x8B\x75\x08\x33\xD2\x57\x8B\x46\x2C\x8B\x48\x24\x8B\x41\x44\x33\xC9\x8A\x50\x0C\x8A\x48\x0B\x52\x51"
#define EMITWATERPOLYS_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x2A\xF3\x0F\x10\x2A\x2A\x2A\x2A\x2A\xF3\x0F\x10"
#define EMITWATERPOLYS_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x20\x53\x55\xD9\x05\x2A\x2A\x2A\x2A\x56\x57"

#define R_DRAWTENTITIESONLIST_SIG_BLOB "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x0F\x2A\x2A\x2A\x00\x00\x8B\x44\x24\x04"

#define R_SETUPGL_SIG_BLOB "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x10\x53\x55\x56\x57\x68\x01\x17\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xFF\x15"

#define R_DRAWSEQUENTIALPOLY_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x53\x55\x56\x8B\x88\xF8\x02\x00\x00\xBE\x01\x00\x00\x00"

#define R_DRAWBRUSHMODEL_SIG_BLOB "\x83\xEC\x4C\xC7\x05\x2A\x2A\x2A\x2A\xFF\xFF\xFF\xFF\x53\x55\x56\x57"

#define R_RECURSIVEWORLDNODE_SIG_BLOB "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x0C\x53\x56\x57\x8B\x7D\x08\x83\x3F\xFE"

#define R_DECALMPOLY_SIG "\xA1\x2A\x2A\x2A\x2A\x57\x50\xE8\x2A\x2A\x2A\x2A\x8B\x4C\x24\x10\x8B\x51\x18"
#define R_DECALMPOLY_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x57\x50\xE8\x2A\x2A\x2A\x2A\x8B\x4D\x0C\x8B\x51\x18\x52\xE8"

#define R_DRAWDECALS_SIG "\xB8\x0C\x00\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x85\xC0"
#define R_DRAWDECALS_SIG_NEW "\x55\x8B\xEC\xB8\x10\x00\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x84"
#define R_DRAWDECALS_SIG_SVENGINE "\xB8\x2A\x2A\x00\x00\xE8\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x84\x2A\x2A\x2A\x2A\x53\x8B\x1D"

//xref string "Failed to load custom decal for player"

#define DRAW_MIPTEXTEXTURE_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\x2A\x2A\x2A\x24\x40\x2A\x2A\x2A\x24\x40"
#define DRAW_MIPTEXTEXTURE_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x2A\x8B\x5D\x0C\x2A\x8B\x75\x08\x2A\x89\x75\xCC"
#define DRAW_MIPTEXTEXTURE_SIG_NEW "\x55\x8B\xEC\x83\xEC\x2A\x2A\x2A\x2A\x2A\x08\x2A\x83\x2A\x2A\x20"

#define R_RENDERVIEW_SIG_BLOB "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\x83\xEC\x14\xDF\xE0\xF6\xC4"

#define V_RENDERVIEW_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x81\xEC\x2A\x00\x00\x00\x2A\x2A\x33\x2A\x33\x2A\x2A\x2A\x89\x35\x2A\x2A\x2A\x2A\x89\x35"

#define VID_UPDATEWINDOWVARS_SIG_BLOB "\x56\x8B\x74\x24\x08\x8B\xC6\x8B\x08\x89\x0D\x2A\x2A\x2A\x2A\x8B\x50\x04\x89\x15"
#define VID_UPDATEWINDOWVARS_SIG_NEW2 VID_UPDATEWINDOWVARS_SIG_BLOB
#define VID_UPDATEWINDOWVARS_SIG_NEW "\x55\x8B\xEC\x51\x56\x8B\x75\x08\x8B\xC6\x8B\x08\x89\x0D\x2A\x2A\x2A\x2A\x8B\x50\x04\x89\x15"
#define VID_UPDATEWINDOWVARS_SIG_HL25 "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x83\xEC\x10\x85\xC0"
#define VID_UPDATEWINDOWVARS_SIG_SVENGINE "\x8b\xc7\x99\x2B\xC2\xD1\xF8\x03\x2A\x50"

#define R_FORCECVARS_SIG_SVENGINE "\x83\x7C\x24\x2A\x00\x2A\x2A\x2A\x2A\x00\x00\x81\x3D\x2A\x2A\x2A\x2A\xFF\x00\x00\x00"
#define R_FORCECVARS_SIG_NEW "\x55\x8B\xEC\x8B\x45\x08\x85\xC0\x0F\x84\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D"

//Studio Funcs
#define R_GLSTUDIODRAWPOINTS_SIG_BLOB2    "\x83\xEC\x48\x8B\x0D\x2A\x2A\x2A\x2A\x8B\x15\x2A\x2A\x2A\x2A\x53\x55\x8B\x41\x54\x8B\x59\x60"

#define R_STUDIOLIGHTING_SIG_BLOB "\x51\xDB\x05\x2A\x2A\x2A\x2A\x8A\x4C\x24\x10\xB8\x01\x00\x00\x00\x84\xC8\xD9\x5C\x24\x00"
#define R_STUDIOLIGHTING_SIG_NEW2 R_STUDIOLIGHTING_SIG_BLOB
#define R_STUDIOLIGHTING_SIG_NEW  "\x55\x8B\xEC\x51\xDB\x05\x2A\x2A\x2A\x2A\x8A\x4D\x10\xB8\x01\x00\x00\x00\x84\xC8\xD9\x5D\xFC"
#define R_STUDIOLIGHTING_SIG_HL25 "\x55\x8B\xEC\xF6\x45\x10\x01\x66\x0F\x2A\x2A\x2A\x2A\x2A\x2A\x0F\x5B\xDB"
#define R_STUDIOLIGHTING_SIG_SVENGINE "\xF6\x2A\x2A\x2A\x2A\xDB\x05\x2A\x2A\x2A\x2A\x74\x1C\x83\x3D"

#define R_LIGHTLAMBERT_SIG_BLOB      "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x2C\x8B\x0D\x2A\x2A\x2A\x2A\x53"
#define R_LIGHTLAMBERT_SIG_BLOB2     "\x8B\x0D\x2A\x2A\x2A\x2A\x83\xEC\x24\x53\x33\xDB\x56"
#define R_LIGHTLAMBERT_SIG_NEW2		 R_LIGHTLAMBERT_SIG_BLOB2
#define R_LIGHTLAMBERT_SIG_NEW       "\x55\x8B\xEC\x83\xEC\x24\x8B\x0D\x2A\x2A\x2A\x2A\x56\x57\x33\xFF\x3B\xCF\x75"
#define R_LIGHTLAMBERT_SIG_HL25      "\x55\x8B\xEC\x83\xE4\xF8\x8B\x15\x2A\x2A\x2A\x2A\x83\xEC\x18\x2A\x2A\x85\xD2\x0F\x2A\x2A\x2A\x2A\x2A\x0F\x57\x2A\x0F\x57\x2A\x33\xFF"
#define R_LIGHTLAMBERT_SIG_SVENGINE  "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\xD9\xE8\x8D\x4C\x24\x2A\x8B\x44\x24\x2A\x53"

#define BUILDNORMALINDEXTABLE_SIG_BLOB "\x8B\x15\x2A\x2A\x2A\x2A\x2A\x8B\x4A\x50\x85\xC9\x2A\x2A\x83\xC8\xFF"
#define BUILDNORMALINDEXTABLE_SIG_NEW "\x55\x8B\xEC\x51\x8B\x15\x2A\x2A\x2A\x2A\x57\x8B\x4A\x50\x85\xC9\x7E\x0A\x83\xC8\xFF\xBF"
#define BUILDNORMALINDEXTABLE_SIG_HL25 ""
#define BUILDNORMALINDEXTABLE_SIG_SVENGINE ""

#define R_STUDIOSETUPSKIN_SIG_BLOB "\xA0\x2A\x2A\x2A\x2A\x81\xEC\x0C\x01\x00\x00\xA8\x02\x53\x56\x57\x0F\x85"
#define R_STUDIOSETUPSKIN_SIG_NEW "\x55\x8B\xEC\x81\xEC\x04\x01\x00\x00\xA0\x2A\x2A\x2A\x2A\x53\x56\x57\xA8\x02\x0F\x85\x2A\x2A\x2A\x2A\x8B\x5D\x0C"
#define R_STUDIOSETUPSKIN_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\xF6\x05\x2A\x2A\x2A\x2A\x02\x8B\x45\x08"
#define R_STUDIOSETUPSKIN_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\xF6\x05"


#define R_DRAWSRPITEMODEL_SIG_BLOB "\x83\xEC\x40\x53\x56\x57\x8B\x7C\x24\x50\x8B\x87\x94\x0B\x00\x00"
#define R_DRAWSRPITEMODEL_SIG_NEW "\x55\x8B\xEC\x83\xEC\x44\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x2A\x2A\x2A\x00\x00\xD9\x2A\xE0\x02"
#define R_DRAWSRPITEMODEL_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\x2A\x2A\x2A\x8B\x75\x08\x2A\x8B\x2A\x94\x0B\x00\x00"
#define R_DRAWSRPITEMODEL_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\x53\x8B\x5C\x24\x2A\x55\x8B\x83\x2A\x0B\x00\x00"

#define R_LIGHTSTRENGTH_SIG_SVENGINE "\x8B\x15\x2A\x2A\x2A\x2A\x2A\x8B\x35\x2A\x2A\x2A\x2A\x2A\x8B\x7C\x24\x0C" //inlined
#define R_LIGHTSTRENGTH_SIG_SVENGINE_10152 "\x8B\x54\x24\x04\x2A\x8B\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x2A\x2A\x2A\x2A\x2A\x2A\x8D\x2A\x2A\x39\x2A\x2A\x2A\x2A\x2A\x2A\x0F" //inlined
#define R_LIGHTSTRENGTH_SIG_HL25   ""//inlined
#define R_LIGHTSTRENGTH_SIG_NEW   "\x55\x8B\xEC\x83\xEC\x0C\x8B\x4D\x08\x8B\x15\x2A\x2A\x2A\x2A\x2A\x8B\x04\x2A\x2A\x2A\x2A\x2A"
#define R_LIGHTSTRENGTH_SIG_NEW2  "\x8B\x4C\x24\x04\x8B\x15\x2A\x2A\x2A\x2A\x83\xEC\x0C\x8B\x04\x2A\x2A\x2A\x2A\x2A\x3B\xC2"
#define R_LIGHTSTRENGTH_SIG_BLOB  "\x8B\x0D\x2A\x2A\x2A\x2A\x83\xEC\x0C\x55\x8B\x2A\x24\x2A\x2A\x8B"


#define SCR_BEGIN_LOADING_PLAQUE "\x6A\x01\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x83\xC4\x04\x83\xF8\x03"

#define MOD_LOADSPRITEMODEL_BLOB		"\x53\x55\x56\x57\x8B\x7C\x24\x18\x8B\x47\x04\x50\xFF\x15"

#define R_INITPARTICLETEXTURE_BLOB "\xA1\x2A\x2A\x2A\x2A\x81\xEC\x2A\x2A\x00\x00\x8B\xC8\x40"
#define R_INITPARTICLETEXTURE_COMMON "\x68\x01\x14\x00\x00\x68\x08\x19\x00\x00\x6A\x00\x6A\x08\x6A\x08"

#define DRAWSTARTUPGRAPHIC_BLOB "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x2C\x53\x56\x57\x8B\xF9\x8B\x87\xA8\x01\x00\x00"


#define DRAW_SPRITEFRAMEHOLES_BLOB "\x68\xC0\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D"

#define DRAW_SPRITEFRAMEADDITIVE_BLOB "\x68\xE2\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x6A\x01\x6A\x01\xFF\x15\x2A\x2A\x2A\x2A\x8B\x44\x24\x14"
#define DRAW_SPRITEFRAMEADDITIVE_NEW "\x55\x8B\xEC\x68\xE2\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x6A\x01\x6A\x01\xFF\x15"

#define DRAW_SPRITEFRAMEGENERIC_BLOB "\x8B\x44\x24\x20\x8B\x4C\x24\x24\x2A\x2A\x8B\x74\x24\x0C\x2A\x68\xE2\x0B\x00\x00\x8B\x3E"

#define DRAW_FILLEDRGBA_BLOB "\x83\xEC\x08\x8D\x44\x24\x28\x8D\x4C\x24\x24\x50\x8D\x54\x24\x24\x51\x8D\x44\x24\x24\x52\x8D\x4C\x24\x24\x50\x8D\x54\x24\x24\x51\x8D\x44\x24\x24\x52\x8D\x4C\x24\x24\x50\x51\xFF\x15\x2A\x2A\x2A\x2A\x83\xC4\x20\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\xE2\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\x00\x00\x04\x46\x68\x00\x22\x00\x00\x68\x00\x23\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x6A\x01"

#define DRAW_FILLEDRGBABLEND_BLOB "\x83\xEC\x08\x8D\x44\x24\x28\x8D\x4C\x24\x24\x50\x8D\x54\x24\x24\x51\x8D\x44\x24\x24\x52\x8D\x4C\x24\x24\x50\x8D\x54\x24\x24\x51\x8D\x44\x24\x24\x52\x8D\x4C\x24\x24\x50\x51\xFF\x15\x2A\x2A\x2A\x2A\x83\xC4\x20\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\xE2\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\x00\x00\x04\x46\x68\x00\x22\x00\x00\x68\x00\x23\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\x03\x03\x00\x00"


#define D_FILLRECT_BLOB "\x83\xEC\x08\x2A\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\xE2\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\x00\x00\x04\x46\x68\x00\x22\x00\x00\x68\x00\x23\x00\x00"

#define DRAW_PIC_BLOB "\x51\x56\x8B\x74\x24\x14\x85\xF6\x0F\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\xE1\x0D\x00\x00"

static hook_t* g_phook_GL_Init = NULL;
static hook_t* g_phook_GL_SetMode_SvEngine = NULL;
static hook_t* g_phook_GL_SetMode_GoldSrc = NULL;
static hook_t* g_phook_GL_SetModeLegacy = NULL;
static hook_t* g_phook_GL_SelectPixelFormat = NULL;
static hook_t* g_phook_GL_Bind = NULL;
static hook_t* g_phook_GL_Set2D = NULL;
static hook_t* g_phook_GL_Finish2D = NULL;
static hook_t* g_phook_GL_BeginRendering = NULL;
static hook_t* g_phook_GL_EndRendering = NULL;
static hook_t* g_phook_R_RenderView_SvEngine = NULL;
static hook_t* g_phook_R_RenderView = NULL;
static hook_t* g_phook_R_LoadSkyBox_SvEngine = NULL;
static hook_t* g_phook_R_LoadSkys = NULL;
static hook_t* g_phook_R_NewMap = NULL;
static hook_t* g_phook_R_CullBox = NULL;
static hook_t* g_phook_R_ForceCVars = NULL;
static hook_t* g_phook_Mod_PointInLeaf = NULL;
static hook_t* g_phook_R_GLStudioDrawPoints = NULL;
static hook_t* g_phook_GL_UnloadTextures = NULL;
static hook_t* g_phook_GL_UnloadTexture = NULL;
static hook_t* g_phook_GL_LoadFilterTexture = NULL;
static hook_t* g_phook_GL_LoadTexture2 = NULL;
static hook_t* g_phook_GL_BuildLightmaps = NULL;
static hook_t* g_phook_LegacyMultiTextureInit = NULL;
static hook_t* g_phook_Mod_LoadStudioModel = NULL;
static hook_t* g_phook_Mod_LoadBrushModel = NULL;
static hook_t* g_phook_Mod_LoadSpriteModel = NULL;
static hook_t* g_phook_Mod_UnloadSpriteTextures = NULL;
static hook_t* g_phook_triapi_RenderMode = NULL;
static hook_t* g_phook_triapi_Begin = NULL;
static hook_t* g_phook_triapi_End = NULL;
static hook_t* g_phook_triapi_Color4f = NULL;
static hook_t* g_phook_triapi_Color4ub = NULL;
static hook_t* g_phook_triapi_TexCoord2f = NULL;
static hook_t* g_phook_triapi_Vertex3fv = NULL;
static hook_t* g_phook_triapi_Vertex3f = NULL;
static hook_t* g_phook_triapi_Brightness = NULL;
static hook_t* g_phook_triapi_Color4fRendermode = NULL;
static hook_t* g_phook_triapi_Fog = NULL;
static hook_t* g_phook_triapi_GetMatrix = NULL;
static hook_t* g_phook_Draw_MiptexTexture = NULL;
static hook_t* g_phook_BuildGammaTable = NULL;
static hook_t* g_phook_DLL_SetModKey = NULL;
static hook_t* g_phook_SDL_GL_SetAttribute = NULL;
static hook_t* g_phook_PVSNode = NULL;
static hook_t* g_phook_Host_ClearMemory = NULL;
static hook_t* g_phook_CVideoMode_Common_DrawStartupGraphic = NULL;
static hook_t* g_phook_CGame_DrawStartupVideo = NULL;
static hook_t* g_phook_Draw_Frame = NULL;
static hook_t* g_phook_Draw_SpriteFrameHoles = NULL;
static hook_t* g_phook_Draw_SpriteFrameHoles_SvEngine = NULL;
static hook_t* g_phook_Draw_SpriteFrameAdditive = NULL;
static hook_t* g_phook_Draw_SpriteFrameAdditive_SvEngine = NULL;
static hook_t* g_phook_Draw_SpriteFrameGeneric = NULL;
static hook_t* g_phook_Draw_SpriteFrameGeneric_SvEngine = NULL;
static hook_t* g_phook_Draw_FillRGBA = NULL;
static hook_t* g_phook_Draw_FillRGBABlend = NULL;
static hook_t* g_phook_Draw_FillRGBABuf = NULL;
static hook_t* g_phook_Draw_Pic = NULL;
static hook_t* g_phook_D_FillRect = NULL;
static hook_t* g_phook_R_GetSpriteFrame = NULL;

static hook_t* g_phook_ClientPortalManager_ResetAll = NULL;
static hook_t* g_phook_ClientPortalManager_DrawPortalSurface = NULL;
static hook_t* g_phook_ClientPortalManager_EnableClipPlane = NULL;
static hook_t* g_phook_ClientPortalManager_RenderPortals = NULL;
static hook_t* g_phook_UpdatePlayerPitch = NULL;

void Engine_FillAddress_HasOfficialFBOSupport(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs[] = "FBO backbuffer rendering disabled";
	auto FBO_String = Search_Pattern_Data(sigs, DllInfo);
	if (!FBO_String)
		FBO_String = Search_Pattern_Rdata(sigs, DllInfo);
	if (FBO_String)
	{
		g_bHasOfficialFBOSupport = true;
	}
}

void Engine_FillAddress_HasOfficialGLTexAllocSupport(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xA8\x16\x00\x00";
	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			typedef struct
			{
				bool bFoundPush;
				bool bFoundCall;
			}LoadSkys_SearchContext;

			LoadSkys_SearchContext ctx = { 0 };

			g_pMetaHookAPI->DisasmRanges(pFound + 4, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (LoadSkys_SearchContext*)context;

				if (instCount == 1 && pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG)
				{
					ctx->bFoundPush = true;
				}

				if (instCount == 2 && address[0] == 0xE8)
				{
					ctx->bFoundCall = true;
				}

				if (ctx->bFoundPush && ctx->bFoundCall)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

				}, 0, &ctx);

			if (ctx.bFoundPush && ctx.bFoundCall)
			{
				g_bHasOfficialGLTexAllocSupport = false;

				break;
			}

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void Engine_FillAddress_GL_Init(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_Init)
		return;

	gPrivateFuncs.GL_Init = (decltype(gPrivateFuncs.GL_Init))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_Init", MH_GAMESYMBOL_KIND_FUNCTION);

	// 在 GL_Init 中定位 gl_extensions
	// 模式: push 0x1F03 (GL_EXTENSIONS) -> call glGetString -> mov [imm32], eax
	if (!gl_extensions && !gPrivateFuncs.SDL_GL_GetProcAddress)
	{
		typedef struct GL_Init_ExtSearch_s
		{
			const mh_dll_info_t& RealDllInfo;
			PUCHAR push_GL_EXTENSIONS_address{};
			int push_GL_EXTENSIONS_instCount{};
		}GL_Init_ExtSearch;

		GL_Init_ExtSearch extCtx = { RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)gPrivateFuncs.GL_Init, 0x120, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (GL_Init_ExtSearch*)context;

			if (gl_extensions)
				return TRUE;

			// 检测 push 0x1F03 (GL_EXTENSIONS)
			if (pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				pinst->detail->x86.operands[0].imm == 0x1F03)
			{
				ctx->push_GL_EXTENSIONS_address = address;
				ctx->push_GL_EXTENSIONS_instCount = instCount;
			}

			// 检测 mov [mem], eax 在 push GL_EXTENSIONS 之后
			if (ctx->push_GL_EXTENSIONS_address &&
				address > ctx->push_GL_EXTENSIONS_address &&
				address < ctx->push_GL_EXTENSIONS_address + 0x30 &&
				instCount > ctx->push_GL_EXTENSIONS_instCount &&
				instCount < ctx->push_GL_EXTENSIONS_instCount + 10 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == X86_REG_EAX &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
			{
				gl_extensions = (decltype(gl_extensions))pinst->detail->x86.operands[0].mem.disp;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &extCtx);
	}
}

void Engine_FillAddress_GL_SetMode(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_SetMode_SvEngine || gPrivateFuncs.GL_SetMode_GoldSrc || gPrivateFuncs.GL_SetModeLegacy)
		return;

	//SvEngine uses the true 3-arg ABI, SDL GoldSrc/HL25 keep the six-arg ABI and
	//every other identity (legacy GoldSrc, blob builds, CoF) uses the legacy entry.
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.GL_SetMode_SvEngine = (decltype(gPrivateFuncs.GL_SetMode_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SetMode", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25 || gPrivateFuncs.SDL_GL_GetProcAddress)
	{
		gPrivateFuncs.GL_SetMode_GoldSrc = (decltype(gPrivateFuncs.GL_SetMode_GoldSrc))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SetMode", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else
	{
		gPrivateFuncs.GL_SetModeLegacy = (decltype(gPrivateFuncs.GL_SetModeLegacy))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SetModeLegacy", MH_GAMESYMBOL_KIND_FUNCTION);
	}

	//The patch record is the FF 15 call instruction to redirect; SvEngine never uses it.
	if (g_iEngineType != ENGINE_SVENGINE)
	{
		gPrivateFuncs.GL_SetMode_call_qwglCreateContext = (decltype(gPrivateFuncs.GL_SetMode_call_qwglCreateContext))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SetMode_call_qwglCreateContext", MH_GAMESYMBOL_KIND_PATCH);
	}

	if (gPrivateFuncs.SDL_GL_GetProcAddress &&
		(g_iEngineType == ENGINE_GOLDSRC || g_iEngineType == ENGINE_GOLDSRC_HL25))
	{
		gPrivateFuncs.SDL_InitGL = (decltype(gPrivateFuncs.SDL_InitGL))GamedataResolvePtr(RealDllInfo.ImageBase, "SDL_InitGL", MH_GAMESYMBOL_KIND_FUNCTION);
	}

	PVOID GL_SetMode_VA = gPrivateFuncs.GL_SetMode_SvEngine ? (PVOID)gPrivateFuncs.GL_SetMode_SvEngine :
		(gPrivateFuncs.GL_SetMode_GoldSrc ? (PVOID)gPrivateFuncs.GL_SetMode_GoldSrc : (PVOID)gPrivateFuncs.GL_SetModeLegacy);

	if (GL_SetMode_VA)
	{
		typedef struct GL_SetMode_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			PVOID base{};
			size_t max_insts{};
			int max_depth{};
			std::set<PVOID> code{};
			std::set<PVOID> branches{};
			std::vector<walk_context_t> walks{};
			// 用于定位 gl_extensions
			PUCHAR push_GL_EXTENSIONS_address{};
			int push_GL_EXTENSIONS_instCount{};
		} GL_SetMode_SearchContext;

		GL_SetMode_SearchContext ctx = { RealDllInfo, RealDllInfo };
		ctx.base = GL_SetMode_VA;
		ctx.max_insts = 500;
		ctx.max_depth = 16;
		ctx.walks.emplace_back(ctx.base, 0x500, 0);

		while (ctx.walks.size())
		{
			auto walk = ctx.walks[ctx.walks.size() - 1];
			ctx.walks.pop_back();

			g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
				auto pinst = (cs_insn*)inst;
				auto ctx = (GL_SetMode_SearchContext*)context;

				if (gl_extensions && (!gPrivateFuncs.GL_SetModeLegacy || vid_d3d))
					return TRUE;

				if (ctx->code.size() > ctx->max_insts)
					return TRUE;

				if (ctx->code.find(address) != ctx->code.end())
					return TRUE;

				ctx->code.emplace(address);

				// 定位 vid_d3d (仅 Legacy 引擎)
				if (instCount < 10 &&
					!vid_d3d &&
					gPrivateFuncs.GL_SetModeLegacy &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					((PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize) &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0x3F800000)
				{
					vid_d3d = (decltype(vid_d3d))pinst->detail->x86.operands[0].mem.disp;
				}

				// 定位 gl_extensions
				// 模式: push 0x1F03 -> call glGetString -> mov [imm32], eax
				if (!gl_extensions)
				{
					// 检测 push 0x1F03 (GL_EXTENSIONS)
					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						pinst->detail->x86.operands[0].imm == 0x1F03)
					{
						ctx->push_GL_EXTENSIONS_address = address;
						ctx->push_GL_EXTENSIONS_instCount = instCount;
					}

					// 检测 mov [mem], eax 在 push GL_EXTENSIONS 之后
					if (ctx->push_GL_EXTENSIONS_address &&
						address > ctx->push_GL_EXTENSIONS_address &&
						address < ctx->push_GL_EXTENSIONS_address + 0x30 &&
						instCount > ctx->push_GL_EXTENSIONS_instCount &&
						instCount < ctx->push_GL_EXTENSIONS_instCount + 10 &&
						pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						pinst->detail->x86.operands[0].mem.index == 0 &&
						pinst->detail->x86.operands[1].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].reg == X86_REG_EAX &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
					{
						gl_extensions = (decltype(gl_extensions))pinst->detail->x86.operands[0].mem.disp;
					}
				}

				// 分支处理
				if ((pinst->id == X86_INS_JMP || (pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS)) &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM)
				{
					PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;
					auto foundbranch = ctx->branches.find(imm);
					if (foundbranch == ctx->branches.end())
					{
						ctx->branches.emplace(imm);
						if (depth + 1 < ctx->max_depth)
						{
							ctx->walks.emplace_back(imm, 0x500, depth + 1);
						}
					}
					if (pinst->id == X86_INS_JMP)
						return TRUE;
				}

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
				}, walk.depth, &ctx);
		}

		Sig_VarNotFound(gl_extensions);
	}

	if (gPrivateFuncs.GL_SetModeLegacy)
	{
		gPrivateFuncs.GL_SelectPixelFormat = (decltype(gPrivateFuncs.GL_SelectPixelFormat))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SelectPixelFormat", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void Engine_FillAddress_R_PolyBlend(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_PolyBlend)
		return;

	gPrivateFuncs.R_PolyBlend = (decltype(gPrivateFuncs.R_PolyBlend))GamedataResolvePtr(RealDllInfo.ImageBase, "R_PolyBlend", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.V_FadeAlpha = (decltype(gPrivateFuncs.V_FadeAlpha))GamedataResolvePtr(RealDllInfo.ImageBase, "V_FadeAlpha", MH_GAMESYMBOL_KIND_FUNCTION);

	if (!cl_sf)
	{
		typedef struct R_PolyBlend_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}R_PolyBlend_SearchContext;

		R_PolyBlend_SearchContext ctx = { RealDllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)gPrivateFuncs.R_PolyBlend, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (R_PolyBlend_SearchContext*)context;

			if (!cl_sf &&
				pinst->id == X86_INS_TEST &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize &&
				pinst->detail->x86.operands[0].size == 1 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 2)
			{
				cl_sf = (decltype(cl_sf))((PUCHAR)pinst->detail->x86.operands[0].mem.disp - offsetof(screenfade_t, fadeFlags));
			}

			if (cl_sf)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		Sig_VarNotFound(cl_sf);
	}
}

void Engine_FillAddress_GL_Bind(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_Bind)
		return;

	gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_Bind", MH_GAMESYMBOL_KIND_FUNCTION);

	currenttexture = (decltype(currenttexture))GamedataResolvePtr(RealDllInfo.ImageBase, "currenttexture", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_GL_LoadTexture2(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_LoadTexture2)
		return;

	PVOID GL_LoadTexture2_VA = (PVOID)GamedataResolvePtr(RealDllInfo.ImageBase, "GL_LoadTexture2", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.GL_LoadTexture2 = (decltype(gPrivateFuncs.GL_LoadTexture2))GL_LoadTexture2_VA;


	/*
		int *numgltextures = NULL;
		gltexture_t *gltextures = NULL;
		int *maxgltextures_SvEngine = NULL;
		gltexture_t **gltextures_SvEngine = NULL;
		int *peakgltextures_SvEngine = NULL;
		int *allocated_textures = NULL;
		int *gHostSpawnCount = NULL;
	*/

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		{
			const char pattern[] = "\x8B\x15\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x1D";
			// GL_LoadTexture2
			//.text : 01D4EBF4 8B 15 F0 C5 0F 03                                   mov     edx, numgltextures
			//.text : 01D4EBFA 3B F2                                               cmp     esi, edx
			//.text : 01D4EBFC 7D 4D                                               jge     short loc_1D4EC4B
			//.text : 01D4EBFE 8B 1D E4 C5 0F 03                                   mov     ebx, gltextures

			ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size((void*)GL_LoadTexture2_VA, 0x300, pattern);
			Sig_AddrNotFound(numgltextures);

			auto numgltextures_VA = *(PVOID*)(addr + 2);
			auto gltextures_SvEngine_VA = *(PVOID*)(addr + 12);

			numgltextures = (decltype(numgltextures))(numgltextures_VA);
			gltextures_SvEngine = (decltype(gltextures_SvEngine))(gltextures_SvEngine_VA);
		}

		{
			const char pattern2[] = "\x6B\xC1\x54\x89\x0D";
			//  GL_LoadTexture2
			//.text:01D4ED66 6B C1 54                                            imul    eax, ecx, 54h; 'T'
			//.text:01D4ED69 89 0D F0 C6 0F 03                                   mov     maxgltextures, ecx

			ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size((void*)GL_LoadTexture2_VA, 0x300, pattern2);
			Sig_AddrNotFound(maxgltextures_SvEngine);

			auto maxgltextures_SvEngine_VA = *(PVOID*)(addr + 5);
			maxgltextures_SvEngine = (decltype(maxgltextures_SvEngine))(maxgltextures_SvEngine_VA);
			Sig_VarNotFound(maxgltextures_SvEngine);

			const char pattern3[] = "\x51\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
			addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x50, pattern3);
			Sig_AddrNotFound(realloc_SvEngine);
			auto realloc_SvEngine_VA = GetCallAddress(addr + 1);
			gPrivateFuncs.realloc_SvEngine = (decltype(gPrivateFuncs.realloc_SvEngine))(realloc_SvEngine_VA);
			Sig_FuncNotFound(realloc_SvEngine);
		}

		{
			const char pattern4[] = "\x66\x8B\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x04";
			// GL_LoadTexture2
			//66 8B 0D E0 72 40 08                                mov     cx, word ptr gHostSpawnCount
			//66 89 4B 04                                         mov     [ebx+4], cx

			ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size((void*)GL_LoadTexture2_VA, 0x200, pattern4);
			Sig_AddrNotFound(gHostSpawnCount);
			auto gHostSpawnCount_VA = *(PVOID*)(addr + 3);

			gHostSpawnCount = (decltype(gHostSpawnCount))(gHostSpawnCount_VA);
			Sig_VarNotFound(gHostSpawnCount);
		}

		{
			const char pattern5[] = "\x03\x35\x2A\x2A\x2A\x2A\x3B\x15";
			// GL_LoadTexture2
			//.text:01D4EDE8 03 35 EC C6 0F 03                                   add     esi, gltextures
			//.text : 01D4EDEE 3B 15 00 C7 0F 03                                   cmp     edx, peakgltextures

			ULONG_PTR addr = (ULONG_PTR)g_pMetaHookAPI->SearchPattern((void*)GL_LoadTexture2_VA, 0x200, pattern5, Sig_Length(pattern5));
			Sig_AddrNotFound(peakgltextures);

			auto peakgltextures_SvEngine_VA = *(PVOID*)(addr + 8);

			peakgltextures_SvEngine = (decltype(peakgltextures_SvEngine))(peakgltextures_SvEngine_VA);
			Sig_VarNotFound(peakgltextures_SvEngine);
		}
	}
	else
	{
		typedef struct GL_LoadTexture2_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			
			int xor_exi_exi_instCount{};
			int xor_exi_exi_reg{};
			int inc_exx_instCount{};
			int inc_exx_reg{};
			int mov_mem_exx_instCount{};
			int mov_mem_exx_reg{};
		} GL_LoadTexture2_SearchContext;

		GL_LoadTexture2_SearchContext ctx = { RealDllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)GL_LoadTexture2_VA, 0x200, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			
			auto pinst = (cs_insn*)inst;
			auto ctx = (GL_LoadTexture2_SearchContext*)context;

			if (!ctx->xor_exi_exi_instCount && pinst->id == X86_INS_XOR &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				(pinst->detail->x86.operands[0].reg == X86_REG_ESI || pinst->detail->x86.operands[0].reg == X86_REG_EDI)
				&&
				pinst->detail->x86.operands[0].reg == pinst->detail->x86.operands[1].reg
				)
			{
				//  xor     esi, esi

				ctx->xor_exi_exi_instCount = instCount;
				ctx->xor_exi_exi_reg = pinst->detail->x86.operands[0].reg;
			}

			if (ctx->xor_exi_exi_instCount && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				(pinst->detail->x86.operands[0].reg == X86_REG_ESI || pinst->detail->x86.operands[0].reg == X86_REG_EDI)
				&&
				pinst->detail->x86.operands[0].reg != ctx->xor_exi_exi_reg &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize
				)
			{
				//  mov     edi, offset gltextures
				gltextures = (decltype(gltextures))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->RealDllInfo, ctx->RealDllInfo);
			}

			if (ctx->xor_exi_exi_instCount && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				(pinst->detail->x86.operands[0].reg == X86_REG_ESI || pinst->detail->x86.operands[0].reg == X86_REG_EDI)
				&&
				pinst->detail->x86.operands[0].reg != ctx->xor_exi_exi_reg &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize
				)
			{
				//  mov     edi, offset gltextures
				gltextures = (decltype(gltextures))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->RealDllInfo, ctx->RealDllInfo);
			}

			if (pinst->id == X86_INS_INC &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG)
			{
				// inc     ecx

				ctx->inc_exx_instCount = instCount;
				ctx->inc_exx_reg = pinst->detail->x86.operands[0].reg;
			}

			if (ctx->xor_exi_exi_instCount && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == ctx->inc_exx_reg &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize
				)
			{
				//  mov     numgltextures, ecx
				if (!g_bHasOfficialGLTexAllocSupport &&
					instCount > ctx->mov_mem_exx_instCount && instCount < ctx->mov_mem_exx_instCount + 5 &&
					pinst->detail->x86.operands[1].reg == ctx->mov_mem_exx_reg)
				{
					allocated_textures = (decltype(allocated_textures))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->RealDllInfo, ctx->RealDllInfo);
				}
				else
				{
					numgltextures = (decltype(numgltextures))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->RealDllInfo, ctx->RealDllInfo);
				}
			}

			if (ctx->xor_exi_exi_instCount && (pinst->id == X86_INS_MOV || pinst->id == X86_INS_MOVZX) &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].size == 2 &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize
				)
			{
				//                        mov     ax, word ptr gHostSpawnCount
				//                        movzx   eax, word ptr gHostSpawnCount
				gHostSpawnCount = (decltype(gHostSpawnCount))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->RealDllInfo, ctx->RealDllInfo);
			}

			if (!g_bHasOfficialGLTexAllocSupport)
			{
				if (ctx->xor_exi_exi_instCount && pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].mem.base != 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG
					)
				{// 89 06 mov     [esi], eax
					ctx->mov_mem_exx_instCount = instCount;
					ctx->mov_mem_exx_reg = pinst->detail->x86.operands[1].reg;
				}
			}

			if (!g_bHasOfficialGLTexAllocSupport)
			{
				if (gltextures && numgltextures && gHostSpawnCount && allocated_textures)
					return TRUE;
			}
			else
			{
				if (gltextures && numgltextures && gHostSpawnCount)
					return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);


		Sig_VarNotFound(gltextures);
		Sig_VarNotFound(numgltextures);
		Sig_VarNotFound(gHostSpawnCount);

		if (!g_bHasOfficialGLTexAllocSupport)
		{
			Sig_VarNotFound(allocated_textures);
		}
	}
}

void Engine_FillAddress_R_CullBox(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_CullBox)
		return;

	gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))GamedataResolvePtr(RealDllInfo.ImageBase, "R_CullBox", MH_GAMESYMBOL_KIND_FUNCTION);

	/*
		mplane_t *frustum = NULL;
		vec_t *vup = NULL;
		vec_t *vpn = NULL;
		vec_t *vright = NULL;
	*/
	frustum = (decltype(frustum))GamedataResolvePtr(RealDllInfo.ImageBase, "frustum", MH_GAMESYMBOL_KIND_GLOBAL);

	if (frustum)
	{
		auto frustum_VA = (PVOID)frustum;

		char pattern_Frustum[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern_Frustum + 11) = (DWORD)frustum_VA;

		auto addr = (ULONG_PTR)Search_Pattern(pattern_Frustum, RealDllInfo);
		Sig_AddrNotFound(pattern_Frustum);

		auto vpn_VA = *(PVOID *)(addr + 1);
		auto vup_VA = *(PVOID*)(addr + 6);

		vpn = (decltype(vpn))vpn_VA;
		vup = (decltype(vup))vup_VA;

		char pattern_Frustum2[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern_Frustum2 + 1) = (DWORD)vpn_VA;
		*(DWORD*)(pattern_Frustum2 + 11) = (DWORD)((PUCHAR)frustum_VA + 0x28);

		addr = (ULONG_PTR)Search_Pattern(pattern_Frustum2, RealDllInfo);
		Sig_AddrNotFound(pattern_Frustum2);

		auto vright_VA = *(PVOID*)(addr + 6);

		vright = (decltype(vup))vright_VA;
	}

	Sig_VarNotFound(frustum);
	Sig_VarNotFound(vpn);
	Sig_VarNotFound(vup);
	Sig_VarNotFound(vright);
}

void Engine_FillAddress_R_SetupFrame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_SetupFrame)
		return;

	PVOID R_SetupFrame_VA = 0;

	//The R_SetupFrame has been inlined into R_RenderScene in HL25 and SvEngine

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_SetupFrame_inlined = true;
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_SetupFrame_inlined = true;
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_SetupFrame_VA = Search_Pattern(R_SETUPFRAME_SIG_NEW, DllInfo);
		if (!R_SetupFrame_VA)
			R_SetupFrame_VA = Search_Pattern(R_SETUPFRAME_SIG_NEW2, DllInfo);
		gPrivateFuncs.R_SetupFrame = (decltype(gPrivateFuncs.R_SetupFrame))ConvertDllInfoSpace(R_SetupFrame_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_SetupFrame_VA = Search_Pattern(R_SETUPFRAME_SIG_BLOB, DllInfo);
		if(!R_SetupFrame_VA)
			R_SetupFrame_VA = Search_Pattern(R_SETUPFRAME_SIG_BLOB2, DllInfo);
		gPrivateFuncs.R_SetupFrame = (decltype(gPrivateFuncs.R_SetupFrame))ConvertDllInfoSpace(R_SetupFrame_VA, DllInfo, RealDllInfo);
	}

	if (!gPrivateFuncs.R_SetupFrame_inlined)
	{
		Sig_FuncNotFound(R_SetupFrame);
	}

	gPrivateFuncs.R_ForceCVars = (decltype(gPrivateFuncs.R_ForceCVars))GamedataResolvePtr(RealDllInfo.ImageBase, "R_ForceCVars", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_CheckVariables = (decltype(gPrivateFuncs.R_CheckVariables))GamedataResolvePtr(RealDllInfo.ImageBase, "R_CheckVariables", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_AnimateLight = (decltype(gPrivateFuncs.R_AnimateLight))GamedataResolvePtr(RealDllInfo.ImageBase, "R_AnimateLight", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_R_SetupGL(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_SetupGL)
		return;

	gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))GamedataResolvePtr(RealDllInfo.ImageBase, "R_SetupGL", MH_GAMESYMBOL_KIND_FUNCTION);

	PVOID R_SetupGL_VA = (PVOID)gPrivateFuncs.R_SetupGL;

	/*
		float *gWorldToScreen = NULL;
		float *gScreenToWorld = NULL;
	*/

	{
		const char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
		/*
		.text:01D45CB3 68 60 99 BC 02                                      push    offset flt_2BC9960 ;gScreenToWorld
		.text:01D45CB8 68 80 97 BC 02                                      push    offset flt_2BC9780 ;gWorldToScreen
		.text:01D45CBD E8 EE C2 01 00                                      call    sub_1D61FB0
		.text:01D45CC2 83 C4 08                                            add     esp, 8
		.text:01D45CC5 5F                                                  pop     edi
		.text:01D45CC6 5E                                                  pop     esi
		.text:01D45CC7 5B                                                  pop     ebx
		.text:01D45CC8 8B E5                                               mov     esp, ebp
		.text:01D45CCA 5D                                                  pop     ebp
		.text:01D45CCB C3                                                  retn
		.text:01D45CCB                                     R_SetupGL       endp
		*/

		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(R_SetupGL_VA, 0x700, pattern);

		if (addr)
		{
			auto gWorldToScreen_VA = (PVOID)(*(ULONG_PTR*)(addr + 6));
			auto gScreenToWorld_VA = (PVOID)(*(ULONG_PTR*)(addr + 1));
			gWorldToScreen = (decltype(gWorldToScreen))gWorldToScreen_VA;
			gScreenToWorld = (decltype(gScreenToWorld))gScreenToWorld_VA;
		}
	}

	Sig_VarNotFound(gWorldToScreen);
	Sig_VarNotFound(gScreenToWorld);

	/*
		float *r_world_matrix = NULL;
		float *r_projection_matrix = NULL;
		qboolean* vertical_fov_SvEngine = NULL;
	*/
	
	typedef struct R_SetupGL_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} R_SetupGL_SearchContext;

	if (1)
	{
		const char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\xA6\x0B\x00\x00";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size((void*)R_SetupGL_VA, 0x600, pattern);
		Sig_AddrNotFound(r_world_matrix);
		r_world_matrix = (decltype(r_world_matrix))((PVOID)(*(ULONG_PTR*)(addr + 1)));

		const char pattern2[] = "\x68\x2A\x2A\x2A\x2A\x68\xA7\x0B\x00\x00";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)R_SetupGL_VA, 0x500, pattern2);
		Sig_AddrNotFound(r_projection_matrix);
		r_projection_matrix = (decltype(r_projection_matrix))((PVOID)(*(ULONG_PTR*)(addr + 1)));
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char pattern3[] = "\x50\xFF\x15\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size((void*)R_SetupGL_VA, 0x120, pattern3);
		Sig_AddrNotFound(vertical_fov_SvEngine);
		vertical_fov_SvEngine = (decltype(vertical_fov_SvEngine))((PVOID)(*(ULONG_PTR*)(addr + 9)));
	}
	else
	{
		//no impl
	}

	Sig_VarNotFound(r_world_matrix);
	Sig_VarNotFound(r_projection_matrix);

	if (g_iEngineType == ENGINE_SVENGINE)
		Sig_VarNotFound(vertical_fov_SvEngine);
}

void Engine_FillAddress_R_RenderView(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_RenderView_SvEngine || gPrivateFuncs.R_RenderView)
		return;

	PVOID R_RenderView_VA = (PVOID)GamedataResolvePtr(RealDllInfo.ImageBase, "R_RenderView", MH_GAMESYMBOL_KIND_FUNCTION);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_RenderView_SvEngine = (decltype(gPrivateFuncs.R_RenderView_SvEngine))R_RenderView_VA;
	}
	else
	{
		gPrivateFuncs.R_RenderView = (decltype(gPrivateFuncs.R_RenderView))R_RenderView_VA;
	}


	//SvEngine publishes the alias counter under its own name, c_model_polys.
	c_alias_polys = (decltype(c_alias_polys))GamedataResolvePtr(RealDllInfo.ImageBase,
		(g_iEngineType == ENGINE_SVENGINE) ? "c_model_polys" : "c_alias_polys", MH_GAMESYMBOL_KIND_GLOBAL);
	c_brush_polys = (decltype(c_brush_polys))GamedataResolvePtr(RealDllInfo.ImageBase, "c_brush_polys", MH_GAMESYMBOL_KIND_GLOBAL);

	r_worldentity = (decltype(r_worldentity))GamedataResolvePtr(RealDllInfo.ImageBase, "r_worldentity", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_worldmodel = (decltype(cl_worldmodel))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_worldmodel", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_V_RenderView(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.V_RenderView)
		return;

	PVOID V_RenderView_VA = (PVOID)GamedataResolvePtr(RealDllInfo.ImageBase, "V_RenderView", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))V_RenderView_VA;


	{
		typedef struct V_RenderView_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;

			int MovClsStateInstCount{};
			int FldzInstCount{};
			int ZeroizedRegister[3]{};
			int ZeroizedRegisterCount{};
			PVOID ZeroizedCandidate[6]{};
			int ZeroizedCandidateCount{};
		}V_RenderView_SearchContext;

		V_RenderView_SearchContext ctx = { RealDllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)V_RenderView_VA, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (V_RenderView_SearchContext*)context;

			if (ctx->MovClsStateInstCount == 0 || instCount < ctx->MovClsStateInstCount + 6)
			{
				/*
				.text:102318B3                 push    esi
				.text:102318B4                 xor     esi, esi
				.text:102318B6                 mov     dword ptr r_soundOrigin+8, esi
				.text:102318BC                 mov     dword ptr r_soundOrigin+4, esi
				.text:102318C2                 mov     dword ptr r_soundOrigin, esi
				.text:102318C8                 mov     dword ptr r_soundOrigin+14h, esi
				.text:102318CE                 mov     dword ptr r_soundOrigin+10h, esi
				.text:102318D4                 mov     dword ptr r_soundOrigin+0Ch, esi
				*/

				if (ctx->ZeroizedRegisterCount < _ARRAYSIZE(ctx->ZeroizedRegister) &&
					pinst->id == X86_INS_XOR && pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == pinst->detail->x86.operands[1].reg)
				{
					ctx->ZeroizedRegister[ctx->ZeroizedRegisterCount] = pinst->detail->x86.operands[0].reg;
					ctx->ZeroizedRegisterCount++;
				}
				if (ctx->ZeroizedCandidateCount < 6 &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					(pinst->detail->x86.operands[1].reg == ctx->ZeroizedRegister[0] || pinst->detail->x86.operands[1].reg == ctx->ZeroizedRegister[1] || pinst->detail->x86.operands[1].reg == ctx->ZeroizedRegister[2]))
				{
					ctx->ZeroizedCandidate[ctx->ZeroizedCandidateCount] = (PVOID)pinst->detail->x86.operands[0].mem.disp;
					ctx->ZeroizedCandidateCount++;
				}

				if (ctx->FldzInstCount > 0 && instCount > ctx->FldzInstCount && instCount < ctx->FldzInstCount + 10)
				{
					/*
						.text:01DCDF74                 fldz
						.text:01DCDF76                 fst     flt_96F8790
						.text:01DCDF7C                 fst     flt_96F8794
						.text:01DCDF82                 fst     flt_96F8798
						.text:01DCDF88                 fst     flt_96F879C
						.text:01DCDF8E                 push    esi
						.text:01DCDF8F                 fst     flt_96F87A0
						.text:01DCDF95                 xor     esi, esi
						.text:01DCDF97                 cmp     dword_20D7D70, 5
						.text:01DCDF9E                 fstp    flt_96F87A4
					*/
					if (ctx->ZeroizedCandidateCount < 6 &&
						(pinst->id == X86_INS_FST || pinst->id == X86_INS_FSTP) &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0)
					{
						ctx->ZeroizedCandidate[ctx->ZeroizedCandidateCount] = (PVOID)pinst->detail->x86.operands[0].mem.disp;
						ctx->ZeroizedCandidateCount++;
					}
				}
			}

			if (!ctx->FldzInstCount && pinst->id == X86_INS_FLDZ)
			{
				ctx->FldzInstCount = instCount;
			}

			if (!cls_state &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 5)
			{
				//83 3D 30 9A 09 02 05                                cmp     cls_state, 5
				cls_state = (decltype(cls_state))((PVOID)pinst->detail->x86.operands[0].mem.disp);
				ctx->MovClsStateInstCount = instCount;
			}

			if (!cls_signon &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 2)
			{
				//83 3D D4 9F 0C 02 02                                cmp     cls_signon, 2
				cls_signon = (decltype(cls_signon))((PVOID)pinst->detail->x86.operands[0].mem.disp);
			}

			if (cls_state && cls_signon)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

		}, 0, &ctx);

		if (ctx.ZeroizedCandidateCount == 6)
		{
			std::qsort(ctx.ZeroizedCandidate, ctx.ZeroizedCandidateCount, sizeof(ctx.ZeroizedCandidate[0]), [](const void* a, const void* b) {
				return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
				});

			r_soundOrigin = (decltype(r_soundOrigin))((PVOID)ctx.ZeroizedCandidate[0]);
			r_playerViewportAngles = (decltype(r_playerViewportAngles))((PVOID)ctx.ZeroizedCandidate[3]);

		}

		Sig_VarNotFound(cls_state);
		Sig_VarNotFound(cls_signon);
		Sig_VarNotFound(r_soundOrigin);
		Sig_VarNotFound(r_playerViewportAngles);
	}
}

void Engine_FillAddress_R_RenderScene(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_RenderScene)
		return;

	PVOID R_RenderScene_VA = 0;

	PVOID R_RenderView_VA = (gPrivateFuncs.R_RenderView_SvEngine) ? (PVOID)gPrivateFuncs.R_RenderView_SvEngine : (PVOID)gPrivateFuncs.R_RenderView;

	PVOID R_SetupGL_VA = (PVOID)gPrivateFuncs.R_SetupGL;

	//SvEngine 10257 inlines R_RenderScene and publishes no catalog record; every
	//other identity, including svencoop-8948, resolves it.
	if (g_iEngineType == ENGINE_SVENGINE &&
		g_pMetaHookAPI->IsGameSymbolAvailable(RealDllInfo.ImageBase, "R_RenderScene") != MH_GAMESYMBOL_OK)
	{
		typedef struct R_RenderScene_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			PVOID &R_SetupGL_VA;
			PVOID &R_RenderScene_VA;
			bool bFoundCallSetupGL{};
		}R_RenderScene_SearchContext;

		R_RenderScene_SearchContext ctx = { RealDllInfo, RealDllInfo, R_SetupGL_VA, R_RenderScene_VA };

		g_pMetaHookAPI->DisasmRanges(R_RenderView_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (R_RenderScene_SearchContext*)context;

			if (address[0] == 0xE8)
			{
				PVOID target = (decltype(target))pinst->detail->x86.operands[0].imm;

				if (target == ctx->R_SetupGL_VA)
				{
					ctx->bFoundCallSetupGL = true;

					//as global var
					gPrivateFuncs.R_RenderScene_inlined = true;

					return TRUE;
				}
				else
				{
					//Do a copy here
					R_RenderScene_SearchContext ctx2 = *ctx;

					g_pMetaHookAPI->DisasmRanges((PVOID)target, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

						auto pinst = (cs_insn*)inst;
						auto ctx2 = (R_RenderScene_SearchContext*)context;

						PVOID target2 = (decltype(target2))pinst->detail->x86.operands[0].imm;

						if (target2 == ctx2->R_SetupGL_VA)
						{
							ctx2->bFoundCallSetupGL = true;
							return TRUE;
						}

						if (address[0] == 0xCC)
							return TRUE;

						if (pinst->id == X86_INS_RET)
							return TRUE;

						return FALSE;

						}, 0, &ctx2);

					if (ctx2.bFoundCallSetupGL)
					{
						ctx2.R_RenderScene_VA = target;
						return TRUE;
					}
				}
			}

			if (ctx->bFoundCallSetupGL)
				return TRUE;

			if (ctx->R_RenderScene_VA)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		if (R_RenderScene_VA)
		{
			gPrivateFuncs.R_RenderScene = (decltype(gPrivateFuncs.R_RenderScene))R_RenderScene_VA;
		}

		if (gPrivateFuncs.R_RenderScene_inlined)
			return;

		if (!R_RenderScene_VA)
		{
			char pattern[] = "\xDD\xD8\xDD\xD8\xE8";
			auto addr = (PUCHAR)Search_Pattern_From(R_RenderView_VA, pattern, RealDllInfo);
			Sig_AddrNotFound(R_RenderScene);
			R_RenderScene_VA = GetCallAddress(addr + 4);
			gPrivateFuncs.R_RenderScene = (decltype(gPrivateFuncs.R_RenderScene))R_RenderScene_VA;
		}

		Sig_FuncNotFound(R_RenderScene);
		return;
	}

	gPrivateFuncs.R_RenderScene = (decltype(gPrivateFuncs.R_RenderScene))GamedataResolvePtr(RealDllInfo.ImageBase, "R_RenderScene", MH_GAMESYMBOL_KIND_FUNCTION);

}

void Engine_FillAddress_R_NewMap(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_NewMap)
		return;

	gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))GamedataResolvePtr(RealDllInfo.ImageBase, "R_NewMap", MH_GAMESYMBOL_KIND_FUNCTION);

	PVOID R_NewMap_VA = (PVOID)gPrivateFuncs.R_NewMap;

	{
		//R_ClearParticles / R_DecalInit / V_InitLevel remain catalog-uncovered call targets.
		char pattern[] = "\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\xFF\xFF\xFF\xFF";
		auto addr = (ULONG_PTR)Search_Pattern_From_Size(R_NewMap_VA, 0x100, pattern);
		if (addr)
		{
			PVOID R_ClearParticles_VA = GetCallAddress(addr + 0);
			PVOID R_DecalInit_VA = GetCallAddress(addr + 5);
			PVOID V_InitLevel_VA = GetCallAddress(addr + 10);

			gPrivateFuncs.R_ClearParticles = (decltype(gPrivateFuncs.R_ClearParticles))R_ClearParticles_VA;
			gPrivateFuncs.R_DecalInit = (decltype(gPrivateFuncs.R_DecalInit))R_DecalInit_VA;
			gPrivateFuncs.V_InitLevel = (decltype(gPrivateFuncs.V_InitLevel))V_InitLevel_VA;
		}
	}

	gPrivateFuncs.GL_UnloadTextures = (decltype(gPrivateFuncs.GL_UnloadTextures))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_UnloadTextures", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_R_DrawSequentialPoly(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawSequentialPoly || gPrivateFuncs.R_DrawSequentialPoly_HL25)
		return;

	PVOID R_DrawSequentialPoly_VA = (PVOID)GamedataResolvePtr(RealDllInfo.ImageBase, "R_DrawSequentialPoly", MH_GAMESYMBOL_KIND_FUNCTION);

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_DrawSequentialPoly_HL25 = (decltype(gPrivateFuncs.R_DrawSequentialPoly_HL25))R_DrawSequentialPoly_VA;
	}
	else
	{
		gPrivateFuncs.R_DrawSequentialPoly = (decltype(gPrivateFuncs.R_DrawSequentialPoly))R_DrawSequentialPoly_VA;
	}


	/*
		//Global pointers that link into engine vars
		byte *lightmaps = NULL;
		int *gDecalSurfCount = NULL;
	*/
	lightmaps = (decltype(lightmaps))GamedataResolvePtr(RealDllInfo.ImageBase, "lightmaps", MH_GAMESYMBOL_KIND_GLOBAL);
	gDecalSurfCount = (decltype(gDecalSurfCount))GamedataResolvePtr(RealDllInfo.ImageBase, "gDecalSurfCount", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_RecursiveWorldNode(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_RecursiveWorldNode || gPrivateFuncs.R_RecursiveWorldNode_HL25)
		return;

	PVOID R_RecursiveWorldNode_VA = (PVOID)GamedataResolvePtr(RealDllInfo.ImageBase, "R_RecursiveWorldNode", MH_GAMESYMBOL_KIND_FUNCTION);

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_RecursiveWorldNode_HL25 = (decltype(gPrivateFuncs.R_RecursiveWorldNode_HL25))R_RecursiveWorldNode_VA;
	}
	else
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))R_RecursiveWorldNode_VA;
	}
}

void Engine_FillAddress_R_DrawWorld(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawWorld)
		return;

	gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))GamedataResolvePtr(RealDllInfo.ImageBase, "R_DrawWorld", MH_GAMESYMBOL_KIND_FUNCTION);

	modelorg = (decltype(modelorg))GamedataResolvePtr(RealDllInfo.ImageBase, "modelorg", MH_GAMESYMBOL_KIND_GLOBAL);

}

void Engine_FillAddress_R_DrawViewModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawViewModel)
		return;

	PVOID R_DrawViewModel_VA = (PVOID)GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "R_DrawViewModel", MH_GAMESYMBOL_KIND_FUNCTION);

	if (R_DrawViewModel_VA)
	{
		gPrivateFuncs.R_DrawViewModel = (decltype(gPrivateFuncs.R_DrawViewModel))R_DrawViewModel_VA;
	}
	else if (g_iEngineType != ENGINE_SVENGINE)
	{
		Sig_FuncNotFound(R_DrawViewModel);
	}


	envmap = (decltype(envmap))GamedataResolvePtr(RealDllInfo.ImageBase, "envmap", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_stats = (decltype(cl_stats))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_stats", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_weaponstarttime = (decltype(cl_weaponstarttime))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_weaponstarttime", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_weaponsequence = (decltype(cl_weaponsequence))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_weaponsequence", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_light_level = (decltype(cl_light_level))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_light_level", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_MarkLeaves(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_MarkLeaves)
		return;

	gPrivateFuncs.R_MarkLeaves = (decltype(gPrivateFuncs.R_MarkLeaves))GamedataResolvePtr(RealDllInfo.ImageBase, "R_MarkLeaves", MH_GAMESYMBOL_KIND_FUNCTION);

	PVOID R_MarkLeaves_VA = (PVOID)gPrivateFuncs.R_MarkLeaves;


	ULONG_PTR r_viewleaf_VA = 0;
	ULONG r_viewleaf_RVA = 0;

	ULONG_PTR r_oldviewleaf_VA = 0;
	ULONG r_oldviewleaf_RVA = 0;

	{
		typedef struct
		{
			ULONG_PTR& r_viewleaf;
			ULONG_PTR& r_oldviewleaf;
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		} R_MarkLeaves_SearchContext;

		R_MarkLeaves_SearchContext ctx = { r_viewleaf_VA, r_oldviewleaf_VA, RealDllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)R_MarkLeaves_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_MarkLeaves_SearchContext*)context;

				if (!ctx->r_viewleaf &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_ECX &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					pinst->detail->x86.operands[1].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
				{//8B 0D 2A 2A 2A 2A                                   mov     ecx, r_viewleaf
					ctx->r_viewleaf = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
				}

				if (!ctx->r_oldviewleaf &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].reg == X86_REG_ECX &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
				{//89 0D 2A 2A 2A 2A                                   mov     r_oldviewleaf, ecx
					ctx->r_oldviewleaf = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}

				if (ctx->r_viewleaf && ctx->r_oldviewleaf)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(r_viewleaf, RealDllInfo);
		Convert_VA_to_RVA(r_oldviewleaf, RealDllInfo);
	}

	if (r_viewleaf_RVA)
		r_viewleaf = (decltype(r_viewleaf))VA_from_RVA(r_viewleaf, RealDllInfo);
	if (r_oldviewleaf_RVA)
		r_oldviewleaf = (decltype(r_oldviewleaf))VA_from_RVA(r_oldviewleaf, RealDllInfo);

	Sig_VarNotFound(r_viewleaf);
	Sig_VarNotFound(r_oldviewleaf);
}

void Engine_FillAddress_EmitWaterPolys(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{

	if (gPrivateFuncs.EmitWaterPolys)
		return;

	ULONG_PTR EmitWaterPolys_VA = 0;
	ULONG EmitWaterPolys_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		EmitWaterPolys_VA = (ULONG_PTR)Search_Pattern(EMITWATERPOLYS_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(EmitWaterPolys, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		EmitWaterPolys_VA = (ULONG_PTR)Search_Pattern(EMITWATERPOLYS_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(EmitWaterPolys, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		EmitWaterPolys_VA = (ULONG_PTR)Search_Pattern(EMITWATERPOLYS_SIG_NEW, DllInfo);

		if (!EmitWaterPolys_VA)
			EmitWaterPolys_VA = (ULONG_PTR)Search_Pattern(EMITWATERPOLYS_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(EmitWaterPolys, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		EmitWaterPolys_VA = (ULONG_PTR)Search_Pattern(EMITWATERPOLYS_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(EmitWaterPolys, DllInfo);
	}

	if (EmitWaterPolys_RVA)
	{
		gPrivateFuncs.EmitWaterPolys = (decltype(gPrivateFuncs.EmitWaterPolys))VA_from_RVA(EmitWaterPolys, RealDllInfo);
	}

	Sig_FuncNotFound(EmitWaterPolys);
}

void Engine_FillAddress_VID_UpdateWindowVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.VID_UpdateWindowVars)
		return;

	ULONG_PTR VID_UpdateWindowVars_VA = 0;
	ULONG VID_UpdateWindowVars_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		auto addr = Search_Pattern(VID_UPDATEWINDOWVARS_SIG_SVENGINE, DllInfo);

		if (addr)
		{
			addr = Search_Pattern_From_Size(addr, 0x50, "\x50\xE8");

			if (addr)
			{
				VID_UpdateWindowVars_VA = (ULONG_PTR)GetCallAddress(addr + 1);
				Convert_VA_to_RVA(VID_UpdateWindowVars, DllInfo);
			}
		}
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		VID_UpdateWindowVars_VA = (ULONG_PTR)Search_Pattern(VID_UPDATEWINDOWVARS_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(VID_UpdateWindowVars, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		VID_UpdateWindowVars_VA = (ULONG_PTR)Search_Pattern(VID_UPDATEWINDOWVARS_SIG_NEW, DllInfo);

		if (!VID_UpdateWindowVars_VA)
			VID_UpdateWindowVars_VA = (ULONG_PTR)Search_Pattern(VID_UPDATEWINDOWVARS_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(VID_UpdateWindowVars, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		VID_UpdateWindowVars_VA = (ULONG_PTR)Search_Pattern(VID_UPDATEWINDOWVARS_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(VID_UpdateWindowVars, DllInfo);
	}

	if (VID_UpdateWindowVars_RVA)
	{
		gPrivateFuncs.VID_UpdateWindowVars = (decltype(gPrivateFuncs.VID_UpdateWindowVars))VA_from_RVA(VID_UpdateWindowVars, RealDllInfo);
	}

	Sig_FuncNotFound(VID_UpdateWindowVars);

	/*
	//Global pointers that link into engine vars
		RECT *window_rect = NULL;
	*/

	ULONG_PTR window_rect_VA = 0;
	ULONG window_rect_RVA = 0;

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		typedef struct
		{
			ULONG_PTR& window_rect;
			const mh_dll_info_t& DllInfo;
		} VID_UpdateWindowVars_SearchContext;

		VID_UpdateWindowVars_SearchContext ctx = { window_rect_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)VID_UpdateWindowVars_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (VID_UpdateWindowVars_SearchContext*)context;

				if (pinst->id == X86_INS_MOVUPS &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG)
				{//0F 11 05 F0 E9 9A 10                                movups  window_rect, xmm0

					ctx->window_rect = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}

				if (ctx->window_rect)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(window_rect, DllInfo);
	}
	else
	{
		typedef struct
		{
			ULONG_PTR& window_rect;
			const mh_dll_info_t& DllInfo;
		} VID_UpdateWindowVars_SearchContext;

		VID_UpdateWindowVars_SearchContext ctx = { window_rect_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)VID_UpdateWindowVars_VA, 0x40, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (VID_UpdateWindowVars_SearchContext*)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG)
				{//.text:01D5F436 A3 BC 66 00 08                                      mov     window_rect, eax

					ctx->window_rect = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}

				if (ctx->window_rect)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(window_rect, DllInfo);
	}

	if (window_rect_RVA)
		window_rect = (decltype(window_rect))VA_from_RVA(window_rect, RealDllInfo);

	Sig_VarNotFound(window_rect);
}

void Engine_FillAddress_BuildGammaTable(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.BuildGammaTable)
		return;

	gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))GamedataResolvePtr(RealDllInfo.ImageBase, "BuildGammaTable", MH_GAMESYMBOL_KIND_FUNCTION);

	PVOID BuildGammaTable_VA = (PVOID)gPrivateFuncs.BuildGammaTable;


	/*
	//Global pointers that link into engine vars
	byte *texgammatable = NULL;
	*/

	typedef struct BuildGammaTable_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} BuildGammaTable_SearchContext;

	BuildGammaTable_SearchContext ctx = { RealDllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(BuildGammaTable_VA, 0x250, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (BuildGammaTable_SearchContext*)context;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == X86_REG_ESI &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].size == 1)
		{
			texgammatable = (decltype(texgammatable))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->RealDllInfo, ctx->RealDllInfo);
		}

		if (texgammatable)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, &ctx);

	Sig_VarNotFound(texgammatable);
}

void Engine_FillAddress_R_DrawParticles(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawParticles)
		return;

	gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))GamedataResolvePtr(RealDllInfo.ImageBase, "R_DrawParticles", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_FreeDeadParticles = (decltype(gPrivateFuncs.R_FreeDeadParticles))GamedataResolvePtr(RealDllInfo.ImageBase, "R_FreeDeadParticles", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_TracerDraw = (decltype(gPrivateFuncs.R_TracerDraw))GamedataResolvePtr(RealDllInfo.ImageBase, "R_TracerDraw", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.R_BeamDrawList = (decltype(gPrivateFuncs.R_BeamDrawList))GamedataResolvePtr(RealDllInfo.ImageBase, "R_BeamDrawList", MH_GAMESYMBOL_KIND_FUNCTION);

	active_particles = (decltype(active_particles))GamedataResolvePtr(RealDllInfo.ImageBase, "active_particles", MH_GAMESYMBOL_KIND_GLOBAL);

	PVOID R_DrawParticles_VA = (PVOID)gPrivateFuncs.R_DrawParticles;

	//particletexture is catalog-uncovered and is still read from the resolved body.
	typedef struct R_DrawParticles_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} R_DrawParticles_SearchContext;

	R_DrawParticles_SearchContext ctx = { RealDllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(R_DrawParticles_VA, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (R_DrawParticles_SearchContext*)context;

		if (!particletexture &&
			pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			pinst->detail->x86.operands[0].mem.index == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
		{
			particletexture = (decltype(particletexture))((PVOID)pinst->detail->x86.operands[0].mem.disp);
		}

		if (!particletexture &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			pinst->detail->x86.operands[1].mem.index == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
		{
			//Skip this shit
			//.text:101EBCA6 A1 F4 36 32 10                                      mov     eax, ___security_cookie
			//.text:101EBCAB 33 C5 xor eax, ebp
			if (address[instLen] == 0x33 && address[instLen + 1] == 0xC5)
			{

			}
			else if (address[instLen] == 0x33 && address[instLen + 1] == 0xC4)
			{

			}
			else
			{
				particletexture = (decltype(particletexture))((PVOID)pinst->detail->x86.operands[1].mem.disp);
			}
		}

		if (particletexture)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(particletexture);


}

void Engine_FillAddress_CL_AllocDlight(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.CL_AllocDlight)
		return;

	gPrivateFuncs.CL_AllocDlight = (decltype(gPrivateFuncs.CL_AllocDlight))GamedataResolvePtr(RealDllInfo.ImageBase, "CL_AllocDlight", MH_GAMESYMBOL_KIND_FUNCTION);

	cl_dlights = (decltype(cl_dlights))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_dlights", MH_GAMESYMBOL_KIND_GLOBAL);

	//r_dlightactive is catalog-uncovered and is still read from the resolved body:
	//it is touched a few instructions after the memset(cl_dlights, 0, 0x28) call.
	typedef struct CL_AllocDlight_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		int push28_instcount{};
	} CL_AllocDlight_SearchContext;

	CL_AllocDlight_SearchContext ctx = { RealDllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)gPrivateFuncs.CL_AllocDlight, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (CL_AllocDlight_SearchContext*)context;

			if (pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				pinst->detail->x86.operands[0].imm == 0x28)
			{
				ctx->push28_instcount = instCount;
			}
			else if (ctx->push28_instcount &&
				instCount < ctx->push28_instcount + 8 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
			{//.text:01D18B16 8B 0D 78 9E BC 02 mov     ecx, r_dlightactive

				if (!r_dlightactive || (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp > (ULONG_PTR)r_dlightactive)
					r_dlightactive = (decltype(r_dlightactive))((PVOID)pinst->detail->x86.operands[1].mem.disp);
			}
			else if (!r_dlightactive &&
				pinst->id == X86_INS_OR &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize &&

				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 1)
			{//.text:01D18B16 8B 0D 78 9E BC 02 mov     ecx, r_dlightactive

				r_dlightactive = (decltype(r_dlightactive))((PVOID)pinst->detail->x86.operands[0].mem.disp);
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(r_dlightactive);
}

void Engine_FillAddress_CL_AllocElight(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.CL_AllocElight)
		return;

	gPrivateFuncs.CL_AllocElight = (decltype(gPrivateFuncs.CL_AllocElight))GamedataResolvePtr(RealDllInfo.ImageBase, "CL_AllocElight", MH_GAMESYMBOL_KIND_FUNCTION);

	cl_elights = (decltype(cl_elights))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_elights", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_R_StudioLighting(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_StudioLighting)
		return;

	PVOID R_StudioLighting_VA = 0;
	ULONG R_StudioLighting_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_StudioLighting_VA = Search_Pattern(R_STUDIOLIGHTING_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.R_StudioLighting = (decltype(gPrivateFuncs.R_StudioLighting))ConvertDllInfoSpace(R_StudioLighting_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_StudioLighting_VA = Search_Pattern(R_STUDIOLIGHTING_SIG_HL25, DllInfo);
		gPrivateFuncs.R_StudioLighting = (decltype(gPrivateFuncs.R_StudioLighting))ConvertDllInfoSpace(R_StudioLighting_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_StudioLighting_VA = Search_Pattern(R_STUDIOLIGHTING_SIG_NEW, DllInfo);

		if (!R_StudioLighting_VA)
			R_StudioLighting_VA = Search_Pattern(R_STUDIOLIGHTING_SIG_NEW2, DllInfo);

		gPrivateFuncs.R_StudioLighting = (decltype(gPrivateFuncs.R_StudioLighting))ConvertDllInfoSpace(R_StudioLighting_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_StudioLighting_VA = Search_Pattern(R_STUDIOLIGHTING_SIG_BLOB, DllInfo);
		gPrivateFuncs.R_StudioLighting = (decltype(gPrivateFuncs.R_StudioLighting))ConvertDllInfoSpace(R_StudioLighting_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(R_StudioLighting);

	/*
		float* r_shadelight = NULL;
		int* r_ambientlight = NULL;
		vec3_t* r_blightvec = NULL;
		vec3_t* r_plightvec = NULL;
		int* lightgammatable = NULL;
	*/

	typedef struct R_StudioLighting_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		PVOID base{};
		size_t max_insts{};
		int max_depth{};
		std::set<PVOID> code{};
		std::set<PVOID> branches{};
		std::vector<walk_context_t> walks{};
		int ambient_instcount{};
		int lightvec_start{};
		PVOID plightvec_cached{};
		ULONG_PTR blightvec_candidateVA{};
		int blightvec_instcount{};
		int blightvec_xmmreg{};
		ULONG_PTR plightvec_candidateVA{};
		int plightvec_instcount{};
		int plightvec_xmmreg{};
		PUCHAR mov_437F0000h_instaddr{};
		PUCHAR fld_255_instaddr{};
	} R_StudioLighting_SearchContext;

	R_StudioLighting_SearchContext ctx = { DllInfo, RealDllInfo };
	ctx.base = R_StudioLighting_VA;
	ctx.max_insts = 500;
	ctx.max_depth = 16;
	ctx.walks.emplace_back(ctx.base, 0x500, 0);

	while (ctx.walks.size())
	{
		auto walk = ctx.walks[ctx.walks.size() - 1];
		ctx.walks.pop_back();

		g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_StudioLighting_SearchContext*)context;

			if (r_ambientlight && r_shadelight && r_blightvec && r_plightvec && lightgammatable)
				return TRUE;

			if (ctx->code.size() > ctx->max_insts)
				return TRUE;

			if (ctx->code.find(address) != ctx->code.end())
				return TRUE;

			ctx->code.emplace(address);

			if (!r_ambientlight &&
				pinst->id == X86_INS_FILD &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D83B54 DB 05 E8 95 75 02                                   fild    r_ambientlight
				r_ambientlight = (decltype(r_ambientlight))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				ctx->ambient_instcount = instCount;
			}

			if (!r_ambientlight &&
				pinst->id == X86_INS_MOVD &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//   movd    xmm3, r_ambientlight
				r_ambientlight = (decltype(r_ambientlight))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				ctx->ambient_instcount = instCount;
			}

			if (!r_shadelight &&
				ctx->ambient_instcount &&
				instCount < ctx->ambient_instcount + 10 &&
				pinst->id == X86_INS_FLD &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D8A986 D9 05 60 61 52 08                                   fld     r_shadelight
				r_shadelight = (decltype(r_shadelight))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (!r_shadelight &&
				ctx->ambient_instcount &&
				instCount < ctx->ambient_instcount + 10 &&
				pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//   movss   xmm1, r_shadelight
				r_shadelight = (decltype(r_shadelight))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == -1)
			{//.text:01D8A9A1 83 F8 FF                                            cmp     eax, 0FFFFFFFFh
				ctx->lightvec_start = 1;
			}
			else if (ctx->plightvec_cached == address)
			{
				ctx->lightvec_start = 1;
				ctx->plightvec_cached = 0;
			}
			else if (
				ctx->lightvec_start &&
				address[0] == 0xDE &&
				address[1] == 0xC1)
			{//.text:01D8A9BC DE C1                                               faddp   st(1), st
				ctx->lightvec_start = 0;
			}
			else if (
				pinst->id == X86_INS_LEA &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[1].mem.base == X86_REG_EAX &&
				pinst->detail->x86.operands[1].mem.index == X86_REG_EAX &&
				pinst->detail->x86.operands[1].mem.scale == 2 &&
				pinst->detail->x86.operands[1].mem.disp == 0)
			{//.text:01D8A9A6 8D 04 40                                            lea     eax, [eax+eax*2]
				ctx->lightvec_start = 2;
			}

			if (!r_blightvec &&
				ctx->lightvec_start == 2 &&
				pinst->id == X86_INS_FLD &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D83BDC D9 80 48 04 79 02                                   fld     (r_blightvec+8)[eax]
				ctx->blightvec_candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				ctx->blightvec_instcount = instCount;
				ctx->blightvec_xmmreg = 0;
			}

			if (!r_blightvec &&
				ctx->lightvec_start == 2 &&
				pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{// F3 0F 10 04 85 28 CE 20 11                          movss   xmm0, (r_blightvec+8)[eax*4]
				ctx->blightvec_candidateVA = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
				ctx->blightvec_instcount = instCount;
				ctx->blightvec_xmmreg = pinst->detail->x86.operands[0].reg;
			}

			if (!r_blightvec &&
				ctx->lightvec_start == 2 &&
				ctx->blightvec_instcount &&
				instCount < ctx->blightvec_instcount + 2 &&
				pinst->id == X86_INS_FMUL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0)
			{//.text:01D83BE2 D8 49 08                                            fmul    dword ptr [ecx+8]
				r_blightvec = (decltype(r_blightvec))ConvertDllInfoSpace((PVOID)(ctx->blightvec_candidateVA - pinst->detail->x86.operands[0].mem.disp), ctx->DllInfo, ctx->RealDllInfo);
			}

			if (!r_blightvec &&
				ctx->lightvec_start == 2 &&
				ctx->blightvec_instcount &&
				instCount < ctx->blightvec_instcount + 2 &&
				pinst->id == X86_INS_MULSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == ctx->blightvec_xmmreg &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base != 0 &&
				pinst->detail->x86.operands[1].mem.disp == 0x8)
			{// F3 0F 59 41 08                                      mulss   xmm0, dword ptr [ecx+8]
				r_blightvec = (decltype(r_blightvec))ConvertDllInfoSpace((PVOID)(ctx->blightvec_candidateVA - pinst->detail->x86.operands[1].mem.disp), ctx->DllInfo, ctx->RealDllInfo);
			}

			if (!r_plightvec &&
				ctx->lightvec_start == 1 &&
				pinst->id == X86_INS_FLD &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D83BDC D9 80 48 04 79 02                                   fld     (r_blightvec+8)[eax]
				ctx->plightvec_candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				ctx->plightvec_instcount = instCount;
			}

			if (!r_plightvec &&
				ctx->lightvec_start == 1 &&
				ctx->plightvec_instcount &&
				instCount < ctx->plightvec_instcount + 15 &&
				pinst->id == X86_INS_FMUL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0)
			{//.text:01D83BE2 D8 49 08                                            fmul    dword ptr [ecx+8]
				r_plightvec = (decltype(r_plightvec))ConvertDllInfoSpace((PVOID)(ctx->plightvec_candidateVA - pinst->detail->x86.operands[0].mem.disp), ctx->DllInfo, ctx->RealDllInfo);
			}

			if (!r_plightvec &&
				ctx->lightvec_start >= 1 &&
				pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base != 0 &&
				pinst->detail->x86.operands[1].mem.disp == 0x8)
			{//F3 0F 10 41 08                                      movss   xmm0, dword ptr [ecx+8]
				ctx->plightvec_instcount = instCount;
				ctx->plightvec_xmmreg = pinst->detail->x86.operands[0].reg;
			}

			if (!r_plightvec &&
				ctx->lightvec_start >= 1 &&
				ctx->plightvec_instcount &&
				instCount < ctx->plightvec_instcount + 2 &&
				pinst->id == X86_INS_MULSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == ctx->plightvec_xmmreg &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{// F3 0F 59 05 A8 0F DC 10                             mulss   xmm0, r_plightvec+8
				r_plightvec = (decltype(r_plightvec))ConvertDllInfoSpace((PVOID)(pinst->detail->x86.operands[1].mem.disp - 8), ctx->DllInfo, ctx->RealDllInfo);
			}

			if (!ctx->mov_437F0000h_instaddr &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0x437F0000
				)
			{
				ctx->mov_437F0000h_instaddr = address;
			}

			if (!ctx->mov_437F0000h_instaddr &&
				pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.RdataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize
				)
			{
				if (*(DWORD*)pinst->detail->x86.operands[1].mem.disp == 0x437F0000)
				{
					ctx->mov_437F0000h_instaddr = address;
				}
			}

			if (!ctx->fld_255_instaddr &&
				pinst->id == X86_INS_FLD &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.RdataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize
				)
			{
				if (*(DWORD*)pinst->detail->x86.operands[0].mem.disp == 0x437F0000)
				{
					ctx->fld_255_instaddr = address;
				}
			}

			if (
				ctx->mov_437F0000h_instaddr &&
				address < ctx->mov_437F0000h_instaddr + 0x30 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				lightgammatable = (decltype(lightgammatable))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (
				ctx->fld_255_instaddr &&
				address < ctx->fld_255_instaddr + 0x80 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				lightgammatable = (decltype(lightgammatable))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->DllInfo, ctx->RealDllInfo);
			}

			if ((pinst->id == X86_INS_JMP || (pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS)) &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM)
			{
				PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;
				auto foundbranch = ctx->branches.find(imm);
				if (foundbranch == ctx->branches.end())
				{
					ctx->branches.emplace(imm);
					if (depth + 1 < ctx->max_depth)
					{
						ctx->walks.emplace_back(imm, 0x500, depth + 1);
						if (!ctx->plightvec_cached && ctx->lightvec_start == 1)
							ctx->plightvec_cached = imm;
					}
				}
				if (pinst->id == X86_INS_JMP)
					return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, walk.depth, &ctx);
	}

	Sig_VarNotFound(r_ambientlight);
	Sig_VarNotFound(r_shadelight);
	Sig_VarNotFound(r_blightvec);
	Sig_VarNotFound(r_plightvec);
	Sig_VarNotFound(lightgammatable);
}

void Engine_FillAddress_R_LightLambert(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_LightLambert)
		return;

	PVOID R_LightLambert_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_LightLambert_VA = Search_Pattern(R_LIGHTLAMBERT_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.R_LightLambert = (decltype(gPrivateFuncs.R_LightLambert))ConvertDllInfoSpace(R_LightLambert_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_LightLambert_VA = Search_Pattern(R_LIGHTLAMBERT_SIG_HL25, DllInfo);
		gPrivateFuncs.R_LightLambert = (decltype(gPrivateFuncs.R_LightLambert))ConvertDllInfoSpace(R_LightLambert_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_LightLambert_VA = Search_Pattern(R_LIGHTLAMBERT_SIG_NEW, DllInfo);

		if (!R_LightLambert_VA)
			R_LightLambert_VA = Search_Pattern(R_LIGHTLAMBERT_SIG_NEW2, DllInfo);

		gPrivateFuncs.R_LightLambert = (decltype(gPrivateFuncs.R_LightLambert))ConvertDllInfoSpace(R_LightLambert_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_LightLambert_VA = Search_Pattern(R_LIGHTLAMBERT_SIG_BLOB, DllInfo);
		gPrivateFuncs.R_LightLambert = (decltype(gPrivateFuncs.R_LightLambert))ConvertDllInfoSpace(R_LightLambert_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(R_LightLambert);
}

void Engine_FillAddress_R_StudioSetupSkin(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_StudioSetupSkin)
		return;

	PVOID R_StudioSetupSkin_VA = 0;

	if (1)
	{
		const char sigs[] = "DM_Base.bmp";
		auto DM_Base_String = Search_Pattern_Data(sigs, DllInfo);
		if (!DM_Base_String)
			DM_Base_String = Search_Pattern_Rdata(sigs, DllInfo);

		if (DM_Base_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\xC7\x44\x24\x2A\x2A\x00\x00\x00";
			*(DWORD*)(pattern + 1) = (DWORD)DM_Base_String;
			auto DM_Base_PushString = Search_Pattern(pattern, DllInfo);
			if (DM_Base_PushString)
			{
				R_StudioSetupSkin_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(DM_Base_PushString, 0x300, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x81 &&
						Candidate[1] == 0xEC &&
						Candidate[4] == 0x00 &&
						Candidate[5] == 0x00)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					if (Candidate[0] == 0xA0 &&
						Candidate[5] == 0x81 &&
						Candidate[6] == 0xEC)
						return TRUE;

					return FALSE;
					});
				gPrivateFuncs.R_StudioSetupSkin = (decltype(gPrivateFuncs.R_StudioSetupSkin))ConvertDllInfoSpace((PVOID)R_StudioSetupSkin_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!gPrivateFuncs.R_StudioSetupSkin)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_StudioSetupSkin_VA = Search_Pattern(R_STUDIOSETUPSKIN_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.R_StudioSetupSkin = (decltype(gPrivateFuncs.R_StudioSetupSkin))ConvertDllInfoSpace((PVOID)R_StudioSetupSkin_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_StudioSetupSkin_VA = Search_Pattern(R_STUDIOSETUPSKIN_SIG_HL25, DllInfo);
			gPrivateFuncs.R_StudioSetupSkin = (decltype(gPrivateFuncs.R_StudioSetupSkin))ConvertDllInfoSpace((PVOID)R_StudioSetupSkin_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_StudioSetupSkin_VA = Search_Pattern(R_STUDIOSETUPSKIN_SIG_NEW, DllInfo);
			gPrivateFuncs.R_StudioSetupSkin = (decltype(gPrivateFuncs.R_StudioSetupSkin))ConvertDllInfoSpace((PVOID)R_StudioSetupSkin_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_StudioSetupSkin_VA = Search_Pattern(R_STUDIOSETUPSKIN_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_StudioSetupSkin = (decltype(gPrivateFuncs.R_StudioSetupSkin))ConvertDllInfoSpace((PVOID)R_StudioSetupSkin_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_StudioSetupSkin);

	PVOID R_StudioGetSkin_VA = 0;
	PVOID GL_UnloadTexture_VA = 0;
	PVOID tmp_palette_VA = 0;

	{
		typedef struct R_StudioSetupSkin_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			ULONG_PTR candidateE8_VA{};
			int candidateE8_instCount{};
			int instCount_push300h{};
		} R_StudioSetupSkin_SearchContext;

		R_StudioSetupSkin_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)R_StudioSetupSkin_VA, 0x800, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_StudioSetupSkin_SearchContext*)context;

			if (address[0] == 0xE8 && instLen == 5)
			{
				ctx->candidateE8_VA = (ULONG_PTR)pinst->detail->x86.operands[0].imm;
				ctx->candidateE8_instCount = instCount;

				if (!gPrivateFuncs.R_StudioGetSkin)
				{
					typedef struct R_StudioSetupSkin_SearchCmp0B_ctx_s
					{
						bool bFoundCmp0B{};
					} R_StudioSetupSkin_SearchCmp0B_ctx;

					R_StudioSetupSkin_SearchCmp0B_ctx ctx2 = {};

					g_pMetaHookAPI->DisasmRanges((void*)ctx->candidateE8_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
						auto pinst = (cs_insn*)inst;
						auto ctx2 = (R_StudioSetupSkin_SearchCmp0B_ctx*)context;

						if (pinst->id == X86_INS_CMP &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[1].type == X86_OP_IMM &&
							pinst->detail->x86.operands[1].imm == 0xB)
						{
							ctx2->bFoundCmp0B = true;
							return TRUE;
						}

						if (address[0] == 0xCC)
							return TRUE;

						if (pinst->id == X86_INS_RET)
							return TRUE;

						return FALSE;

						}, 0, &ctx2);

					if (ctx2.bFoundCmp0B)
					{
						gPrivateFuncs.R_StudioGetSkin = (decltype(gPrivateFuncs.R_StudioGetSkin))ConvertDllInfoSpace((PVOID)ctx->candidateE8_VA, ctx->DllInfo, ctx->RealDllInfo);
					}
				}
			}

			if (!gPrivateFuncs.GL_UnloadTexture && ctx->candidateE8_VA)
			{
				if (instCount > ctx->candidateE8_instCount && instCount < ctx->candidateE8_instCount + 10)
				{
					//.text:01D8B20C FF B7 20 01 00 00                                   push    dword ptr [edi+120h]
					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base &&
						pinst->detail->x86.operands[0].mem.disp == 0x120)
					{
						gPrivateFuncs.GL_UnloadTexture = (decltype(gPrivateFuncs.GL_UnloadTexture))ConvertDllInfoSpace((PVOID)ctx->candidateE8_VA, ctx->DllInfo, ctx->RealDllInfo);
					}
					else if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].mem.base &&
						pinst->detail->x86.operands[1].mem.disp == 0x120)
					{
						gPrivateFuncs.GL_UnloadTexture = (decltype(gPrivateFuncs.GL_UnloadTexture))ConvertDllInfoSpace((PVOID)ctx->candidateE8_VA, ctx->DllInfo, ctx->RealDllInfo);
					}
				}
			}

			if (!tmp_palette)
			{
				if (!ctx->instCount_push300h &&
					pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					pinst->detail->x86.operands[0].imm == 0x300)
				{
					ctx->instCount_push300h = instCount;
				}

				if (ctx->instCount_push300h &&
					instCount > ctx->instCount_push300h &&
					instCount < ctx->instCount_push300h + 8 &&
					pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					(ULONG_PTR)pinst->detail->x86.operands[0].imm >(ULONG_PTR)ctx->DllInfo.DataBase &&
					(ULONG_PTR)pinst->detail->x86.operands[0].imm < (ULONG_PTR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					tmp_palette = (decltype(tmp_palette))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
				}
			}

			if (gPrivateFuncs.GL_UnloadTexture && gPrivateFuncs.R_StudioGetSkin && tmp_palette)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);
	}

	Sig_FuncNotFound(R_StudioGetSkin);
	Sig_FuncNotFound(GL_UnloadTexture);
	Sig_VarNotFound(tmp_palette);
}

void Engine_FillAddress_Cache_Alloc(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Cache_Alloc)
		return;

	gPrivateFuncs.Cache_Alloc = (decltype(gPrivateFuncs.Cache_Alloc))GamedataResolvePtr(RealDllInfo.ImageBase, "Cache_Alloc", MH_GAMESYMBOL_KIND_FUNCTION);

	cache_head = (decltype(cache_head))GamedataResolvePtr(RealDllInfo.ImageBase, "cache_head", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_Draw_MiptexTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_MiptexTexture)
		return;

	PVOID Draw_MiptexTexture_VA = 0;
	ULONG Draw_MiptexTexture_RVA = 0;

	{
		const char sigs[] = "Draw_MiptexTexture: Bad cached wad %s\n";
		auto Draw_MiptexTexture_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Draw_MiptexTexture_String)
			Draw_MiptexTexture_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (Draw_MiptexTexture_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8";
			*(DWORD*)(pattern + 1) = (DWORD)Draw_MiptexTexture_String;
			auto Draw_MiptexTexture_Call = Search_Pattern(pattern, DllInfo);
			if (Draw_MiptexTexture_Call)
			{
				Draw_MiptexTexture_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Draw_MiptexTexture_Call, 0x80, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC &&
						Candidate[3] == 0xA1)
						return TRUE;

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC &&
						Candidate[3] >= 0x50 &&
						Candidate[3] <= 0x57)
						return TRUE;

					return FALSE;
					});
				gPrivateFuncs.Draw_MiptexTexture = (decltype(gPrivateFuncs.Draw_MiptexTexture))ConvertDllInfoSpace(Draw_MiptexTexture_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!gPrivateFuncs.Draw_MiptexTexture)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			Draw_MiptexTexture_VA = Search_Pattern(DRAW_MIPTEXTEXTURE_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.Draw_MiptexTexture = (decltype(gPrivateFuncs.Draw_MiptexTexture))ConvertDllInfoSpace(Draw_MiptexTexture_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			Draw_MiptexTexture_VA = Search_Pattern(DRAW_MIPTEXTEXTURE_SIG_HL25, DllInfo);
			gPrivateFuncs.Draw_MiptexTexture = (decltype(gPrivateFuncs.Draw_MiptexTexture))ConvertDllInfoSpace(Draw_MiptexTexture_VA, DllInfo, RealDllInfo);
		}
		else
		{
			//GoldSrc_Blob and GoldSrc_New use the same signature
			Draw_MiptexTexture_VA = Search_Pattern(DRAW_MIPTEXTEXTURE_SIG_NEW, DllInfo);
			gPrivateFuncs.Draw_MiptexTexture = (decltype(gPrivateFuncs.Draw_MiptexTexture))ConvertDllInfoSpace(Draw_MiptexTexture_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(Draw_MiptexTexture);

	typedef struct Draw_MiptexTexture_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		int gfCustomBuild_instCount{};
		int push_0F_instCount{};
		ULONG_PTR gfCustomBuild_candidateVA{};
		int gfCustomBuild_candidate_reg{};
		int gfCustomBuild_candidate_instCount{};
	} Draw_MiptexTexture_SearchContext;

	Draw_MiptexTexture_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)Draw_MiptexTexture_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (Draw_MiptexTexture_SearchContext*)context;

		if (!gfCustomBuild &&
			pinst->id == X86_INS_CMP &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			pinst->detail->x86.operands[1].imm == 0)
		{
			gfCustomBuild = (decltype(gfCustomBuild))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			ctx->gfCustomBuild_instCount = instCount;
		}

		if (!gfCustomBuild &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			ctx->gfCustomBuild_candidateVA = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
			ctx->gfCustomBuild_candidate_reg = pinst->detail->x86.operands[0].reg;
			ctx->gfCustomBuild_candidate_instCount = instCount;
		}

		if (!gfCustomBuild &&
			instCount == ctx->gfCustomBuild_candidate_instCount + 1 &&
			pinst->id == X86_INS_TEST &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].reg == ctx->gfCustomBuild_candidate_reg)
		{
			gfCustomBuild = (decltype(gfCustomBuild))ConvertDllInfoSpace((PVOID)ctx->gfCustomBuild_candidateVA, ctx->DllInfo, ctx->RealDllInfo);
			ctx->gfCustomBuild_instCount = instCount;
		}

		if (!szCustName && instCount > ctx->gfCustomBuild_instCount && instCount < ctx->gfCustomBuild_instCount + 15)
		{
			if (!ctx->push_0F_instCount &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				pinst->detail->x86.operands[0].imm == 0xF)
			{
				ctx->push_0F_instCount = instCount;
			}

			if (instCount > ctx->push_0F_instCount &&
				instCount < ctx->push_0F_instCount + 5 &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[0].imm >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				szCustName = (decltype(szCustName))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
			}
		}

		if (gfCustomBuild && szCustName)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(gfCustomBuild);
	Sig_VarNotFound(szCustName);
}

void Engine_FillAddress_Draw_DecalTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_DecalTexture)
		return;

	gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_DecalTexture", MH_GAMESYMBOL_KIND_FUNCTION);

	PVOID Draw_DecalTexture_VA = (PVOID)gPrivateFuncs.Draw_DecalTexture;


	{
		typedef struct Draw_DecalTexture_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo; 
			
			PVOID base{};
			size_t max_insts{};
			int max_depth{};
			std::set<PVOID> code{};
			std::set<PVOID> branches{};
			std::vector<walk_context_t> walks{};

			int pushCount{};
			int pushAddrCount{};
		} Draw_DecalTexture_SearchContext;

		Draw_DecalTexture_SearchContext ctx = { RealDllInfo, RealDllInfo };

		ctx.base = Draw_DecalTexture_VA;
		ctx.max_insts = 300;
		ctx.max_depth = 16;
		ctx.walks.emplace_back(ctx.base, 0x3000, 0);

		while (ctx.walks.size())
		{
			auto walk = ctx.walks[ctx.walks.size() - 1];
			ctx.walks.pop_back();

			g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
				auto pinst = (cs_insn*)inst;
				auto ctx = (Draw_DecalTexture_SearchContext*)context;

				if (decal_wad && gPrivateFuncs.Draw_CustomCacheGet && gPrivateFuncs.Draw_CacheGet)
					return TRUE;

				if (ctx->code.size() > ctx->max_insts)
					return TRUE;

				if (ctx->code.find(address) != ctx->code.end())
					return TRUE;

				ctx->code.emplace(address);

				if (!decal_wad)
				{
					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						pinst->detail->x86.operands[0].mem.index == 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
					{
						decal_wad = (decltype(decal_wad))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->RealDllInfo, ctx->RealDllInfo);
					}
					else if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].mem.base == 0 &&
						pinst->detail->x86.operands[1].mem.index == 0 &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
					{
						decal_wad = (decltype(decal_wad))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->RealDllInfo, ctx->RealDllInfo);
					}
				}

				if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1)
				{
					ctx->pushCount++;
					ctx->pushAddrCount = instCount;
				}

				if (address[0] == 0xE8 && instLen == 5)
				{
					/*
					.text:01D2F8D6 52                                                  push    edx
					.text:01D2F8D7 50                                                  push    eax
					.text:01D2F8D8 56                                                  push    esi
					.text:01D2F8D9 57                                                  push    edi
					.text:01D2F8DA E8 D1 07 00 00                                      call    Draw_CustomCacheGet
					.text:01D2F8DF 83 C4 10                                            add     esp, 10h
					*/

					if (!gPrivateFuncs.Draw_CustomCacheGet && ctx->pushCount == 4 && instCount == ctx->pushAddrCount + 1 && !memcmp(address + instLen, "\x83\xC4\x10", 3))
					{
						PVOID target = (decltype(target))pinst->detail->x86.operands[0].imm;

						gPrivateFuncs.Draw_CustomCacheGet = (decltype(gPrivateFuncs.Draw_CustomCacheGet))
							ConvertDllInfoSpace(target, ctx->RealDllInfo, ctx->RealDllInfo);
					}
					/*
	.text:01D2F8EC 50                                                  push    eax
	.text:01D2F8ED 51                                                  push    ecx
	.text:01D2F8EE E8 AD 06 00 00                                      call    Draw_CacheGet
	.text:01D2F8F3 83 C4 08                                            add     esp, 8
					*/
					if (!gPrivateFuncs.Draw_CacheGet && ctx->pushCount == 2 && instCount == ctx->pushAddrCount + 1 && !memcmp(address + instLen, "\x83\xC4\x08", 3))
					{
						PVOID target = (decltype(target))pinst->detail->x86.operands[0].imm;

						gPrivateFuncs.Draw_CacheGet = (decltype(gPrivateFuncs.Draw_CacheGet))
							ConvertDllInfoSpace(target, ctx->RealDllInfo, ctx->RealDllInfo);
					}
					ctx->pushCount = 0;
					ctx->pushAddrCount = 0;
				}

				if (decal_wad && gPrivateFuncs.Draw_CustomCacheGet && gPrivateFuncs.Draw_CacheGet)
					return TRUE;

				if ((pinst->id == X86_INS_JMP || (pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS)) &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM)
				{
					PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;
					auto foundbranch = ctx->branches.find(imm);
					if (foundbranch == ctx->branches.end())
					{
						ctx->branches.emplace(imm);
						if (depth + 1 < ctx->max_depth)
							ctx->walks.emplace_back(imm, 0x300, depth + 1);
					}

					if (pinst->id == X86_INS_JMP)
						return TRUE;
				}

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

				}, walk.depth, &ctx);
		}
	}

	Sig_VarNotFound(decal_wad);
	//Sig_FuncNotFound(Draw_CustomCacheGet);
	//Sig_FuncNotFound(Draw_CacheGet);
}

void Engine_FillAddress_R_DrawSpriteModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawSpriteModel)
		return;

	PVOID R_DrawSpriteModel_VA = 0;

	{
		const char sigs[] = "R_DrawSpriteModel:  couldn";
		auto R_DrawSpriteModel_String = Search_Pattern_Data(sigs, DllInfo);
		if (!R_DrawSpriteModel_String)
			R_DrawSpriteModel_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (R_DrawSpriteModel_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)R_DrawSpriteModel_String;
			auto R_DrawSpriteModel_Call = Search_Pattern(pattern, DllInfo);
			if (R_DrawSpriteModel_Call)
			{
				R_DrawSpriteModel_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_DrawSpriteModel_Call, 0x300, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC &&
						Candidate[3] == 0xA1)
						return TRUE;

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC &&
						Candidate[3] >= 0x50 &&
						Candidate[3] <= 0x57)
						return TRUE;

					return FALSE;
					});
				gPrivateFuncs.R_DrawSpriteModel = (decltype(gPrivateFuncs.R_DrawSpriteModel))ConvertDllInfoSpace(R_DrawSpriteModel_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!gPrivateFuncs.R_DrawSpriteModel)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_DrawSpriteModel_VA = Search_Pattern(R_DRAWSRPITEMODEL_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.R_DrawSpriteModel = (decltype(gPrivateFuncs.R_DrawSpriteModel))ConvertDllInfoSpace(R_DrawSpriteModel_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_DrawSpriteModel_VA = Search_Pattern(R_DRAWSRPITEMODEL_SIG_HL25, DllInfo);
			gPrivateFuncs.R_DrawSpriteModel = (decltype(gPrivateFuncs.R_DrawSpriteModel))ConvertDllInfoSpace(R_DrawSpriteModel_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_DrawSpriteModel_VA = Search_Pattern(R_DRAWSRPITEMODEL_SIG_NEW, DllInfo);
			gPrivateFuncs.R_DrawSpriteModel = (decltype(gPrivateFuncs.R_DrawSpriteModel))ConvertDllInfoSpace(R_DrawSpriteModel_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_DrawSpriteModel_VA = Search_Pattern(R_DRAWSRPITEMODEL_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_DrawSpriteModel = (decltype(gPrivateFuncs.R_DrawSpriteModel))ConvertDllInfoSpace(R_DrawSpriteModel_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_DrawSpriteModel);
}

void Engine_FillAddress_R_LightStrength(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_LightStrength)
		return;

	PVOID R_LightStrength_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_LightStrength_VA = Search_Pattern(R_LIGHTSTRENGTH_SIG_SVENGINE, DllInfo);
		if (!R_LightStrength_VA)
			R_LightStrength_VA = Search_Pattern(R_LIGHTSTRENGTH_SIG_SVENGINE_10152, DllInfo);
		gPrivateFuncs.R_LightStrength = (decltype(gPrivateFuncs.R_LightStrength))ConvertDllInfoSpace(R_LightStrength_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_LightStrength_inlined = true;
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_LightStrength_VA = Search_Pattern(R_LIGHTSTRENGTH_SIG_NEW, DllInfo);
		if (!R_LightStrength_VA)
			R_LightStrength_VA = Search_Pattern(R_LIGHTSTRENGTH_SIG_NEW2, DllInfo);
		gPrivateFuncs.R_LightStrength = (decltype(gPrivateFuncs.R_LightStrength))ConvertDllInfoSpace(R_LightStrength_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_LightStrength_VA = Search_Pattern(R_LIGHTSTRENGTH_SIG_BLOB, DllInfo);
		gPrivateFuncs.R_LightStrength = (decltype(gPrivateFuncs.R_LightStrength))ConvertDllInfoSpace(R_LightStrength_VA, DllInfo, RealDllInfo);
	}

	if (gPrivateFuncs.R_LightStrength_inlined)
		return;

	Sig_FuncNotFound(R_LightStrength);
}

void Engine_FillAddress_R_RotateForEntity(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_RotateForEntity)
		return;

	PVOID R_RotateForEntity_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//no impl
	}
	else
	{
#define R_ROTATEFORENTITY_GOLDSRC  "\xFF\x15\x2A\x2A\x2A\x2A\x8D\x2A\x48\x0B\x00\x00\x2A\x2A\xE8"
		auto addr = Search_Pattern(R_ROTATEFORENTITY_GOLDSRC, DllInfo);
		if (addr)
		{
			R_RotateForEntity_VA = GetCallAddress((PUCHAR)addr + Sig_Length(R_ROTATEFORENTITY_GOLDSRC) - 1);
			gPrivateFuncs.R_RotateForEntity = (decltype(gPrivateFuncs.R_RotateForEntity))ConvertDllInfoSpace(R_RotateForEntity_VA, DllInfo, RealDllInfo);
		}
	}

	if (!gPrivateFuncs.R_RotateForEntity)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_RotateForEntity_VA = Search_Pattern(R_ROTATEFORENTITY_SVENGINE, DllInfo);
			gPrivateFuncs.R_RotateForEntity = (decltype(gPrivateFuncs.R_RotateForEntity))ConvertDllInfoSpace(R_RotateForEntity_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_RotateForEntity_VA = Search_Pattern(R_ROTATEFORENTITY_HL25, DllInfo);
			gPrivateFuncs.R_RotateForEntity = (decltype(gPrivateFuncs.R_RotateForEntity))ConvertDllInfoSpace(R_RotateForEntity_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_RotateForEntity_VA = Search_Pattern(R_ROTATEFORENTITY_NEW, DllInfo);
			gPrivateFuncs.R_RotateForEntity = (decltype(gPrivateFuncs.R_RotateForEntity))ConvertDllInfoSpace(R_RotateForEntity_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_RotateForEntity_VA = Search_Pattern(R_ROTATEFORENTITY_NEW, DllInfo);
			gPrivateFuncs.R_RotateForEntity = (decltype(gPrivateFuncs.R_RotateForEntity))ConvertDllInfoSpace(R_RotateForEntity_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_RotateForEntity);
}

void Engine_FillAddress_GlowBlend(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GlowBlend)
		return;

	//GlowBlend is inlined on SvEngine and HL25 and has no catalog record there.
	if (g_iEngineType == ENGINE_SVENGINE || g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		return;
	}

	gPrivateFuncs.GlowBlend = (decltype(gPrivateFuncs.GlowBlend))GamedataResolvePtr(RealDllInfo.ImageBase, "GlowBlend", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_SCR_BeginLoadingPlaque(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.SCR_BeginLoadingPlaque)
		return;

	PVOID SCR_BeginLoadingPlaque_VA = 0;

	//All engine use the same signature
	SCR_BeginLoadingPlaque_VA = Search_Pattern(SCR_BEGIN_LOADING_PLAQUE, DllInfo);

	gPrivateFuncs.SCR_BeginLoadingPlaque = (decltype(gPrivateFuncs.SCR_BeginLoadingPlaque))ConvertDllInfoSpace(SCR_BeginLoadingPlaque_VA, DllInfo, RealDllInfo);

	Sig_FuncNotFound(SCR_BeginLoadingPlaque);

	{
		typedef struct SCR_BeginLoadingPlaque_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		} SCR_BeginLoadingPlaque_SearchContext;

		SCR_BeginLoadingPlaque_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(SCR_BeginLoadingPlaque_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (SCR_BeginLoadingPlaque_SearchContext*)context;

			if (!scr_drawloading &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 1)
			{
				//C7 05 60 66 00 08 01 00 00 00                       mov     scr_drawloading, 1
				scr_drawloading = (decltype(scr_drawloading))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (scr_drawloading)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		Sig_VarNotFound(scr_drawloading);
	}
}

void Engine_FillAddress_Mod_LoadSpriteFrame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Mod_LoadSpriteFrame)
		return;

	gPrivateFuncs.Mod_LoadSpriteFrame = (decltype(gPrivateFuncs.Mod_LoadSpriteFrame))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_LoadSpriteFrame", MH_GAMESYMBOL_KIND_FUNCTION);


	{
		typedef struct Mod_LoadSpriteFrame_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			int MovRegMem_InstCount{};
			int MovRegMem_Reg{};
			PVOID MovRegMem_CandidateMem{};
		}Mod_LoadSpriteFrame_SearchContext;

		Mod_LoadSpriteFrame_SearchContext ctx = { RealDllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)ConvertDllInfoSpace(gPrivateFuncs.Mod_LoadSpriteFrame, RealDllInfo, RealDllInfo), 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (Mod_LoadSpriteFrame_SearchContext*)context;

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{
				gSpriteMipMap = (decltype(gSpriteMipMap))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->RealDllInfo, ctx->RealDllInfo);
				return TRUE;
			}

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
			{
				ctx->MovRegMem_Reg = pinst->detail->x86.operands[0].reg;
				ctx->MovRegMem_CandidateMem = (decltype(ctx->MovRegMem_CandidateMem))pinst->detail->x86.operands[1].mem.disp;
				ctx->MovRegMem_InstCount = instCount;
			}

			if (instCount == ctx->MovRegMem_InstCount + 1 &&
				pinst->id == X86_INS_TEST &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == ctx->MovRegMem_Reg &&
				pinst->detail->x86.operands[1].reg == ctx->MovRegMem_Reg)
			{
				gSpriteMipMap = (decltype(gSpriteMipMap))ConvertDllInfoSpace(ctx->MovRegMem_CandidateMem, ctx->RealDllInfo, ctx->RealDllInfo);
				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);
	}

	Sig_VarNotFound(gSpriteMipMap);
}

void Engine_FillAddress_R_AddTEntity(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_AddTEntity)
		return;

	ULONG_PTR R_AddTEntity_VA = 0;
	ULONG R_AddTEntity_RVA = 0;

	//though engine's R_AddTEntity is not used by Renderer anymore
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs1[] = "Can't add transparent entity. Too many";
		auto R_AddTEntity_String = Search_Pattern_Data(sigs1, DllInfo);
		if (!R_AddTEntity_String)
			R_AddTEntity_String = Search_Pattern_Rdata(sigs1, DllInfo);
		Sig_VarNotFound(R_AddTEntity_String);

		char pattern[] = "\x50\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern + 2) = (DWORD)R_AddTEntity_String;
		auto R_AddTEntity_Call = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(R_AddTEntity_Call);

		R_AddTEntity_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBegin(R_AddTEntity_Call, 0x50);
		Convert_VA_to_RVA(R_AddTEntity, DllInfo);
	}
	else
	{
		const char sigs1[] = "AddTentity: Too many objects";
		auto R_AddTEntity_String = Search_Pattern_Data(sigs1, DllInfo);
		if (!R_AddTEntity_String)
			R_AddTEntity_String = Search_Pattern_Rdata(sigs1, DllInfo);
		Sig_VarNotFound(R_AddTEntity_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)R_AddTEntity_String;
		auto R_AddTEntity_Call = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(R_AddTEntity_Call);

		R_AddTEntity_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBegin(R_AddTEntity_Call, 0x50);
		Convert_VA_to_RVA(R_AddTEntity, DllInfo);
	}

	if (R_AddTEntity_RVA)
	{
		gPrivateFuncs.R_AddTEntity = (decltype(gPrivateFuncs.R_AddTEntity))VA_from_RVA(R_AddTEntity, RealDllInfo);
	}

	Sig_FuncNotFound(R_AddTEntity);
}

void Engine_FillAddress_Hunk_AllocName(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Hunk_AllocName)
		return;

	gPrivateFuncs.Hunk_AllocName = (decltype(gPrivateFuncs.Hunk_AllocName))GamedataResolvePtr(RealDllInfo.ImageBase, "Hunk_AllocName", MH_GAMESYMBOL_KIND_FUNCTION);

}

void Engine_FillAddress_GL_EndRenderingVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	//Global float pointers that link into engine vars or our global vars.
		float *s_fXMouseAspectAdjustment = NULL;
		float *s_fYMouseAspectAdjustment = NULL;

	//Global int pointers that link into engine vars (optional, can be nullptr)
		int *gl_msaa_fbo = NULL;
		int *gl_backbuffer_fbo = NULL;
	*/

	if (!g_bHasOfficialFBOSupport)
	{
		s_fXMouseAspectAdjustment = &s_fXMouseAspectAdjustment_Storage;
		s_fYMouseAspectAdjustment = &s_fYMouseAspectAdjustment_Storage;
		gl_msaa_fbo = nullptr;
		gl_backbuffer_fbo = nullptr;
		return;
	}

	PVOID GL_EndRendering_VA = ConvertDllInfoSpace(gPrivateFuncs.GL_EndRendering, RealDllInfo, DllInfo);

	typedef struct GL_EndRendering_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		int type{};
		int zero_register{};
		int load_zero_instcount{};
	} GL_EndRendering_SearchContext;

	GL_EndRendering_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(GL_EndRendering_VA, 0x350, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (GL_EndRendering_SearchContext*)context;

			//A1 40 77 7B 02			mov     eax, gl_backbuffer_fbo
			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{
				DWORD imm = pinst->detail->x86.operands[1].mem.disp;

				if (!gl_backbuffer_fbo && ctx->type == 0)
				{
					gl_backbuffer_fbo = (decltype(gl_backbuffer_fbo))ConvertDllInfoSpace((PVOID)imm, ctx->DllInfo, ctx->RealDllInfo);
					ctx->type = 1;
				}
				else if (!gl_msaa_fbo && ctx->type == 1)
				{
					gl_msaa_fbo = (decltype(gl_msaa_fbo))ConvertDllInfoSpace((PVOID)imm, ctx->DllInfo, ctx->RealDllInfo);
				}
			}
			//83 3D 94 66 00 08 00 cmp     gl_backbuffer_fbo, 0
			else if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				if (!gl_backbuffer_fbo && ctx->type == 0)
				{
					gl_backbuffer_fbo = (decltype(gl_backbuffer_fbo))ConvertDllInfoSpace((PVOID)imm, ctx->DllInfo, ctx->RealDllInfo);
					ctx->type = 2;
				}
				else if (!gl_msaa_fbo && ctx->type == 2)
				{
					gl_msaa_fbo = (decltype(gl_msaa_fbo))ConvertDllInfoSpace((PVOID)imm, ctx->DllInfo, ctx->RealDllInfo);
				}
			}
			//.text:01D4D4C0 A3 F4 78 E4 01 mov     videowindowaspect_0, eax
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == ctx->zero_register &&
				instCount < ctx->load_zero_instcount + 5)
			{
				if (!s_fYMouseAspectAdjustment)
				{
					s_fYMouseAspectAdjustment = (decltype(s_fYMouseAspectAdjustment))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}
				else if (!s_fXMouseAspectAdjustment)
				{
					s_fXMouseAspectAdjustment = (decltype(s_fXMouseAspectAdjustment))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0x3F800000)
			{//.text:01D4D4C0 A3 F4 78 E4 01 mov     videowindowaspect_0, eax

				if (!s_fYMouseAspectAdjustment)
					s_fYMouseAspectAdjustment = (decltype(s_fYMouseAspectAdjustment))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				else if (!s_fXMouseAspectAdjustment)
					s_fXMouseAspectAdjustment = (decltype(s_fXMouseAspectAdjustment))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}
			else if ((pinst->id == X86_INS_FST || pinst->id == X86_INS_FSTP) &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				instCount < ctx->load_zero_instcount + 5)
			{//.D9 15 E0 85 ED 01 fst     videowindowaspect

				if (!s_fYMouseAspectAdjustment)
					s_fYMouseAspectAdjustment = (decltype(s_fYMouseAspectAdjustment))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				else if (!s_fXMouseAspectAdjustment)
					s_fXMouseAspectAdjustment = (decltype(s_fXMouseAspectAdjustment))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0x3F800000)
			{//.text:01D4D4B8 B8 00 00 80 3F mov     eax, 3F800000h

				ctx->zero_register = pinst->detail->x86.operands[0].reg;
				ctx->load_zero_instcount = instCount;
			}
			else if (pinst->id == X86_INS_FLD1)
			{//.text:01D4D4B8 B8 00 00 80 3F mov     eax, 3F800000h

				ctx->zero_register = 0;
				ctx->load_zero_instcount = instCount;
			}

			if (gl_backbuffer_fbo && gl_msaa_fbo && s_fXMouseAspectAdjustment && s_fYMouseAspectAdjustment)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(gl_backbuffer_fbo);
	Sig_VarNotFound(gl_msaa_fbo);
	Sig_VarNotFound(s_fXMouseAspectAdjustment);
	Sig_VarNotFound(s_fYMouseAspectAdjustment);
}

void Engine_FillAddress_R_AllocTransObjectsVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars.
	int *numTransObjs = NULL;
	int *maxTransObjs = NULL;
	transObjRef **transObjects = NULL;
	*/
	ULONG_PTR numTransObjs_VA = 0;
	ULONG numTransObjs_RVA = 0;

	ULONG_PTR maxTransObjs_VA = 0;
	ULONG maxTransObjs_RVA = 0;

	ULONG_PTR transObjects_VA = 0;
	ULONG transObjects_RVA = 0;

	if (1)
	{
		const char sigs[] = "Transparent objects reallocate";
		auto R_AllocTransObjects_String = Search_Pattern_Data(sigs, DllInfo);
		if (!R_AllocTransObjects_String)
			R_AllocTransObjects_String = Search_Pattern_Rdata(sigs, DllInfo);
		Sig_VarNotFound(R_AllocTransObjects_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
		*(DWORD*)(pattern + 1) = (DWORD)R_AllocTransObjects_String;
		auto R_AllocTransObjects_PushString = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(R_AllocTransObjects_PushString);

		PVOID R_AllocTransObjects = (decltype(R_AllocTransObjects))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_AllocTransObjects_PushString, 0x50, [](PUCHAR Candidate) {
			//.text:01D920B0 83 3D 94 61 DF 08 00                                cmp     dword_8DF6194, 0
			if (Candidate[0] == 0x83 &&
				Candidate[1] == 0x3D &&
				Candidate[6] == 0x00)
				return TRUE;

			//  .text : 01D0B180 55                                                  push    ebp
			//	.text : 01D0B181 8B EC                                               mov     ebp, esp
			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC)
				return TRUE;

			if (Candidate[0] == 0xA1 &&
				Candidate[5] == 0x85 &&
				Candidate[6] == 0xC0)
			{
				return TRUE;
			}
			return FALSE;
			});

		Sig_VarNotFound(R_AllocTransObjects);

		typedef struct
		{
			ULONG_PTR& transObjects;
			ULONG_PTR& maxTransObjs;
		} R_AllocTransObjectsVars_SearchContext;

		R_AllocTransObjectsVars_SearchContext ctx = { transObjects_VA, maxTransObjs_VA };

		g_pMetaHookAPI->DisasmRanges(R_AllocTransObjects, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_AllocTransObjectsVars_SearchContext*)context;

			if (!ctx->transObjects && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == X86_REG_EAX)
			{//.text:01D9205D A3 8C 61 DF 08 mov     transObjects, eax
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				ctx->transObjects = (ULONG_PTR)imm;
			}

			else if (!ctx->maxTransObjs &&
				ctx->transObjects &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//.text:01D9205D A3 8C 61 DF 08 mov     transObjects, eax
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				if (imm != (DWORD)ctx->transObjects)
					ctx->maxTransObjs = (ULONG_PTR)imm;
			}

			if (ctx->transObjects && ctx->maxTransObjs)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		transObjects_VA = ctx.transObjects;
		maxTransObjs_VA = ctx.maxTransObjs;

		Convert_VA_to_RVA(transObjects, DllInfo);
		Convert_VA_to_RVA(maxTransObjs, DllInfo);

		//numTransObjs is always before maxTransObjs
		if (maxTransObjs_VA)
			numTransObjs_VA = maxTransObjs_VA - sizeof(int);

		Convert_VA_to_RVA(numTransObjs, DllInfo);
	}

	if (transObjects_RVA)
		transObjects = (decltype(transObjects))VA_from_RVA(transObjects, RealDllInfo);
	if (maxTransObjs_RVA)
		maxTransObjs = (decltype(maxTransObjs))VA_from_RVA(maxTransObjs, RealDllInfo);
	if (numTransObjs_RVA)
		numTransObjs = (decltype(numTransObjs))VA_from_RVA(numTransObjs, RealDllInfo);

	Sig_VarNotFound(transObjects);
	Sig_VarNotFound(maxTransObjs);
	Sig_VarNotFound(numTransObjs);
}

void Engine_FillAddress_R_RenderFinalFog(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *g_bUserFogOn = NULL;
		float *g_UserFogColor = NULL;
		float *g_UserFogDensity = NULL;
		float *g_UserFogStart = NULL;
		float *g_UserFogEnd = NULL;
	*/
	PVOID R_RenderFinalFog_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//SvEngine inlines R_RenderFinalFog into the render-view body and publishes
		//no catalog record, so the push 0B60h instruction stays the anchor.
		PVOID SearchBase_VA = gPrivateFuncs.R_RenderView_SvEngine ? (PVOID)gPrivateFuncs.R_RenderView_SvEngine : (PVOID)gPrivateFuncs.R_RenderView;

		typedef struct R_RenderFinalFog_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			PVOID& R_RenderFinalFog_VA;

			bool bFoundPushB60h{};
			bool bFoundPush801h{};
			bool bFoundPushB65h{};
		} R_RenderFinalFog_SearchContext;

		R_RenderFinalFog_SearchContext ctx = { RealDllInfo, RealDllInfo, R_RenderFinalFog_VA };

		g_pMetaHookAPI->DisasmRanges((void*)SearchBase_VA, 0x600, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_RenderFinalFog_SearchContext*)context;

				if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0)
				{
					g_bUserFogOn = (decltype(g_bUserFogOn))((PVOID)pinst->detail->x86.operands[0].mem.disp);
				}
				else if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					pinst->detail->x86.operands[0].imm == 0xB60)
				{
					ctx->bFoundPushB60h = true;
					ctx->R_RenderFinalFog_VA = (PVOID)address;
				}
				else if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					pinst->detail->x86.operands[0].imm == 0x801)
				{
					ctx->bFoundPush801h = true;
				}
				else if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					pinst->detail->x86.operands[0].imm == 0xB65)
				{
					ctx->bFoundPushB65h = true;
				}

				if (g_bUserFogOn && ctx->bFoundPushB60h && ctx->bFoundPush801h && ctx->bFoundPushB65h)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		gPrivateFuncs.R_RenderFinalFog = (decltype(gPrivateFuncs.R_RenderFinalFog))R_RenderFinalFog_VA;
	}
	else
	{
		gPrivateFuncs.R_RenderFinalFog = (decltype(gPrivateFuncs.R_RenderFinalFog))GamedataResolvePtr(RealDllInfo.ImageBase, "R_RenderFinalFog", MH_GAMESYMBOL_KIND_FUNCTION);
		R_RenderFinalFog_VA = (PVOID)gPrivateFuncs.R_RenderFinalFog;

		//g_bUserFogOn remains catalog-uncovered: locate the R_RenderFinalFog call
		//inside the render-view body on the real image and read the pushed slot.
		PVOID SearchBase_VA = gPrivateFuncs.R_RenderScene ? (PVOID)gPrivateFuncs.R_RenderScene : (PVOID)gPrivateFuncs.R_RenderView;

		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			const char sigs[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x60\x0B\x00\x00";
			ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(SearchBase_VA, 0x600, sigs);

			if (addr)
			{
				g_bUserFogOn = (decltype(g_bUserFogOn))((PVOID)(*(ULONG_PTR*)(addr + 2)));
			}
		}
		else
		{
			const char sigs[] = "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x6A\x00";
			ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(SearchBase_VA, 0x600, sigs);

			if (addr)
			{
				g_bUserFogOn = (decltype(g_bUserFogOn))((PVOID)(*(ULONG_PTR*)(addr + 1)));
			}
		}
	}

	if (R_RenderFinalFog_VA)
	{
		typedef struct R_RenderFinalFog_VarsContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			ULONG_PTR candidateVA{};
		} R_RenderFinalFog_VarsContext;

		R_RenderFinalFog_VarsContext ctx = { RealDllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(R_RenderFinalFog_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_RenderFinalFog_VarsContext*)context;

			if (ctx->candidateVA &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				pinst->detail->x86.operands[0].imm > 0x100)
			{
				int pushvalue = pinst->detail->x86.operands[0].imm;

				if (!g_UserFogDensity && pushvalue == GL_FOG_DENSITY)
				{
					g_UserFogDensity = (decltype(g_UserFogDensity))((PVOID)ctx->candidateVA);
					ctx->candidateVA = 0;
				}
				else if (!g_UserFogColor && pushvalue == GL_FOG_COLOR)
				{
					g_UserFogColor = (decltype(g_UserFogColor))((PVOID)ctx->candidateVA);
					ctx->candidateVA = 0;
				}
				else if (!g_UserFogStart && pushvalue == GL_FOG_START)
				{
					g_UserFogStart = (decltype(g_UserFogStart))((PVOID)ctx->candidateVA);
					ctx->candidateVA = 0;
				}
				else if (!g_UserFogEnd && pushvalue == GL_FOG_END)
				{
					g_UserFogEnd = (decltype(g_UserFogEnd))((PVOID)ctx->candidateVA);
					ctx->candidateVA = 0;
				}
			}

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{
				ctx->candidateVA = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
			}
			else if (pinst->id == X86_INS_FLD &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0)
			{
				ctx->candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			}
			else if (pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
			{
				ctx->candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].imm;
			}
			else if (pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
			{
				ctx->candidateVA = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
			}

			if (g_UserFogDensity && g_UserFogColor && g_UserFogStart && g_UserFogEnd)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);
	}

	Sig_VarNotFound(g_bUserFogOn);
	Sig_VarNotFound(g_UserFogDensity);
	Sig_VarNotFound(g_UserFogColor);
	Sig_VarNotFound(g_UserFogStart);
	Sig_VarNotFound(g_UserFogEnd);
}


void Engine_FillAddress_R_DrawTEntitiesOnListVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		float* r_blend = NULL;
		int *cl_parsecount = NULL;
	*/
	cl_parsecount = (decltype(cl_parsecount))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_parsecount", MH_GAMESYMBOL_KIND_GLOBAL);

	PVOID R_DrawTEntitiesOnList_VA = ConvertDllInfoSpace(gPrivateFuncs.R_DrawTEntitiesOnList, RealDllInfo, DllInfo);

	typedef struct R_DrawTEntitiesOnList_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		int disableFog_instcount{};
		int getskin_instcount{};
		int r_entorigin_candidate_count{};
		int push2300_instcount{};
		int ClientDLL_DrawTransparentTriangles_candidate_instcount{};
		ULONG_PTR r_entorigin_candidateVA[3]{};
	} R_DrawTEntitiesOnList_SearchContext;

	R_DrawTEntitiesOnList_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(R_DrawTEntitiesOnList_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
		
		auto pinst = (cs_insn*)inst;
		auto ctx = (R_DrawTEntitiesOnList_SearchContext*)context;

		if (pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].imm == GL_FOG)
		{//.text:01D92330 68 60 0B 00 00 push    0B60h

			ctx->disableFog_instcount = instCount;
		}

		if (address[0] == 0x6A && address[1] == 0x00 && address[2] == 0xE8)
		{
			//6A 00 push    0
			//E8 A3 13 05 00                                      call    GL_EnableDisableFog

			auto callTarget = GetCallAddress((address + 2));

			typedef struct GL_EnableDisableFog_SearchContext_s
			{
				bool bFoundGL_FOG{};
			} GL_EnableDisableFog_SearchContext;

			GL_EnableDisableFog_SearchContext ctx2 = { };

			g_pMetaHookAPI->DisasmRanges(callTarget, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx2 = (GL_EnableDisableFog_SearchContext*)context;

				if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].imm == GL_FOG)
				{//.text:01D92330 68 60 0B 00 00 push    0B60h

					ctx2->bFoundGL_FOG = instCount;
				}

				return FALSE;

				}, 0, &ctx2);

			if (ctx2.bFoundGL_FOG)
			{
				ctx->disableFog_instcount = instCount;
			}
		}

		if (!r_blend && ctx->disableFog_instcount &&
			instCount < ctx->disableFog_instcount + 15 &&
			(pinst->id == X86_INS_FSTP || pinst->id == X86_INS_FST) &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0)
		{
			//D9 15 80 61 DF 08 fst     r_blend
			r_blend = (decltype(r_blend))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}
		if (!r_blend && ctx->disableFog_instcount &&
			instCount < ctx->disableFog_instcount + 15 &&
			(pinst->id == X86_INS_MOVSS) &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG)
		{
			// F3 0F 11 05 54 40 1B 11                             movss   r_blend, xmm0
			r_blend = (decltype(r_blend))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (
			pinst->id == X86_INS_MOVSX &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].size == 4 &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].size == 2 &&
			pinst->detail->x86.operands[1].mem.base != 0 &&
			pinst->detail->x86.operands[1].mem.disp == 0x2E8)
		{
			//.text:01D924D9 0F BF 83 E8 02 00 00                                movsx   eax, word ptr [ebx+2E8h]
			ctx->getskin_instcount = instCount;
		}

		if (ctx->getskin_instcount &&
			instCount < ctx->getskin_instcount + 20 &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D88C23 89 15 E0 98 BC 02                                   mov     r_entorigin, edx
			auto candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			if (ctx->r_entorigin_candidate_count < 3)
			{
				bool bFound = false;
				for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
				{
					if (ctx->r_entorigin_candidateVA[k] == candidateVA)
						bFound = true;
				}
				if (!bFound)
				{
					ctx->r_entorigin_candidateVA[ctx->r_entorigin_candidate_count] = candidateVA;
					ctx->r_entorigin_candidate_count++;
				}
			}
		}

		if (ctx->getskin_instcount &&
			instCount < ctx->getskin_instcount + 20 &&
			pinst->id == X86_INS_FST &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D88C23 89 15 E0 98 BC 02                                   mov     r_entorigin, edx
			auto candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			if (ctx->r_entorigin_candidate_count < 3)
			{
				bool bFound = false;
				for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
				{
					if (ctx->r_entorigin_candidateVA[k] == candidateVA)
						bFound = true;
				}
				if (!bFound)
				{
					ctx->r_entorigin_candidateVA[ctx->r_entorigin_candidate_count] = candidateVA;
					ctx->r_entorigin_candidate_count++;
				}
			}
		}

		if (ctx->getskin_instcount &&
			instCount < ctx->getskin_instcount + 20 &&
			pinst->id == X86_INS_MOVSS &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:101FA69B F3 0F 10 00                                         movss   xmm0, dword ptr[eax]
			//.text : 101FA69F F3 0F 11 05 E0 02 DC 10                             movss   r_entorigin, xmm0
			auto candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			if (ctx->r_entorigin_candidate_count < 3)
			{
				bool bFound = false;
				for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
				{
					if (ctx->r_entorigin_candidateVA[k] == candidateVA)
						bFound = true;
				}
				if (!bFound)
				{
					ctx->r_entorigin_candidateVA[ctx->r_entorigin_candidate_count] = candidateVA;
					ctx->r_entorigin_candidate_count++;
				}
			}
		}

		if (!gPrivateFuncs.ClientDLL_DrawTransparentTriangles || !gPrivateFuncs.pfnDrawTransparentTriangles)
		{
			if (!ctx->push2300_instcount)
			{
				if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					pinst->detail->x86.operands[0].imm == 0x2300)
				{
					ctx->push2300_instcount = instCount;
				}
			}

			if (ctx->push2300_instcount > 0 && instCount > ctx->push2300_instcount)
			{
				if (!gPrivateFuncs.ClientDLL_DrawTransparentTriangles && address[0] == 0xE8 && instLen == 5)
				{
					gPrivateFuncs.ClientDLL_DrawTransparentTriangles = (decltype(gPrivateFuncs.ClientDLL_DrawTransparentTriangles))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
					ctx->ClientDLL_DrawTransparentTriangles_candidate_instcount = instCount;
				}

				if (!gPrivateFuncs.pfnDrawTransparentTriangles && gPrivateFuncs.ClientDLL_DrawTransparentTriangles && instCount == ctx->ClientDLL_DrawTransparentTriangles_candidate_instcount + 1)
				{
					/*
						.text:01D88E15 E8 96 2C F8 FF                                      call    ClientDLL_DrawTransparentTriangles
						.text:01D88E1A A1 E4 23 73 02                                      mov     eax, g_bUserFogOn
					*/
					PVOID g_bUserFogOn_VA = ConvertDllInfoSpace(g_bUserFogOn, ctx->RealDllInfo, ctx->DllInfo);

					if (pinst->id == X86_INS_CMP &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp == (PUCHAR)g_bUserFogOn_VA &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						pinst->detail->x86.operands[1].imm == 0)
					{
						// Keep the current value, no change needed
					}
					/*
						.text:01D88E15 E8 96 2C F8 FF                                      call    ClientDLL_DrawTransparentTriangles
						.text:01D88E1A A1 E4 23 73 02                                      mov     eax, g_bUserFogOn
					*/
					else if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].mem.base == 0 &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp == (PUCHAR)g_bUserFogOn_VA)
					{
						// Keep the current value, no change needed
					}
					else
					{
						// If neither condition matches, clear the candidate
						gPrivateFuncs.ClientDLL_DrawTransparentTriangles = nullptr;
					}

					if (gPrivateFuncs.ClientDLL_DrawTransparentTriangles)
					{
						PUCHAR code = (PUCHAR)ConvertDllInfoSpace(gPrivateFuncs.ClientDLL_DrawTransparentTriangles, ctx->RealDllInfo, ctx->DllInfo);

						if (code[0] == 0xA1)
						{
							PVOID pfnDrawTransparentTriangles_VA = *(PVOID*)(code + 1);
							gPrivateFuncs.pfnDrawTransparentTriangles = (decltype(gPrivateFuncs.pfnDrawTransparentTriangles))
								ConvertDllInfoSpace(pfnDrawTransparentTriangles_VA, ctx->DllInfo, ctx->RealDllInfo);
						}
					}
				}
			}
		}

		if (r_blend && cl_parsecount && ctx->r_entorigin_candidate_count >= 3 && gPrivateFuncs.ClientDLL_DrawTransparentTriangles && gPrivateFuncs.pfnDrawTransparentTriangles)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, &ctx);

	Sig_VarNotFound(r_blend);
	Sig_FuncNotFound(ClientDLL_DrawTransparentTriangles);
	Sig_FuncNotFound(pfnDrawTransparentTriangles);

	if (ctx.r_entorigin_candidate_count >= 2)
	{
		std::qsort(ctx.r_entorigin_candidateVA, ctx.r_entorigin_candidate_count, sizeof(ctx.r_entorigin_candidateVA[0]), [](const void* a, const void* b) {
			return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
		});

		if ((ULONG_PTR)ctx.r_entorigin_candidateVA[0] == (ULONG_PTR)ctx.r_entorigin_candidateVA[1] - sizeof(float))
		{
			r_entorigin = (decltype(r_entorigin))ConvertDllInfoSpace((PVOID)ctx.r_entorigin_candidateVA[0], ctx.DllInfo, ctx.RealDllInfo);
		}
	}
	else if (ctx.r_entorigin_candidate_count == 1)
	{
		r_entorigin = (decltype(r_entorigin))ConvertDllInfoSpace((PVOID)ctx.r_entorigin_candidateVA[0], ctx.DllInfo, ctx.RealDllInfo);
	}

	Sig_VarNotFound(r_entorigin);
}

void Engine_FillAddress_R_RecursiveWorldNodeVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars.
		int *r_framecount = NULL;
		int *r_visframecount = NULL;
	*/

	PVOID R_RecursiveWorldNode_VA = NULL;

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		R_RecursiveWorldNode_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RecursiveWorldNode_HL25, RealDllInfo, DllInfo);
	else
		R_RecursiveWorldNode_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RecursiveWorldNode, RealDllInfo, DllInfo);

	if (!R_RecursiveWorldNode_VA)
	{
		Sig_NotFound(R_RecursiveWorldNode_VA);
	}

	typedef struct R_RecursiveWorldNode_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		int movexx_offset{};
		int movexx_instcount{};
		int movexx_register{};
		int cmp_register{};
		ULONG_PTR cmp_candidateVA{};
	} R_RecursiveWorldNode_SearchContext;

	R_RecursiveWorldNode_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(R_RecursiveWorldNode_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
		auto pinst = (cs_insn*)inst;
		auto ctx = (R_RecursiveWorldNode_SearchContext*)context;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base != 0 &&
			pinst->detail->x86.operands[1].mem.index == 0 &&
			(pinst->detail->x86.operands[1].mem.disp == 0 || pinst->detail->x86.operands[1].mem.disp == 4)
			)
		{//.text:01D49235 8B 47 04                                            mov     eax, [edi+4]

			ctx->movexx_offset = pinst->detail->x86.operands[1].mem.disp;
			ctx->movexx_instcount = instCount;
			ctx->movexx_register = pinst->detail->x86.operands[0].reg;
		}
		else if (ctx->movexx_instcount &&
			instCount < ctx->movexx_instcount + 3 &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
			)
		{
			//.text:01D49238 8B 0D D4 98 BC 02                                   mov     ecx, r_visframecount

			ctx->cmp_register = pinst->detail->x86.operands[0].reg;
			ctx->cmp_candidateVA = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
		}
		else if (ctx->movexx_instcount &&
			instCount < ctx->movexx_instcount + 3 &&
			pinst->id == X86_INS_CMP &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
			)
		{
			//.text:01D5A533 3B 05 7C 3F F5 03                                   cmp     eax, r_visframecount

			if (ctx->movexx_offset == 4 && !r_visframecount)
				r_visframecount = (decltype(r_visframecount))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			else if (ctx->movexx_offset == 0 && !r_framecount)
				r_framecount = (decltype(r_framecount))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (ctx->movexx_instcount &&
			instCount < ctx->movexx_instcount + 3 &&
			pinst->id == X86_INS_CMP &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			((pinst->detail->x86.operands[0].reg == ctx->cmp_register &&
				pinst->detail->x86.operands[1].reg == ctx->movexx_register) ||
				(pinst->detail->x86.operands[1].reg == ctx->cmp_register &&
					pinst->detail->x86.operands[0].reg == ctx->movexx_register)))
		{
			//.text:01D49235 8B 47 04                                            mov     eax, [edi+4]
			//.text:01D49238 8B 0D D4 98 BC 02                                   mov     ecx, r_visframecount
			//.text:01D4923E 3B C1                                               cmp     eax, ecx

			//.text:01D4932E 8B 0E                                               mov     ecx, [esi]
			//.text:01D49330 A1 EC 97 BC 02                                      mov     eax, r_framecount
			//.text:01D49335 3B C8                                               cmp     ecx, eax
			if (ctx->movexx_offset == 4 && !r_visframecount)
				r_visframecount = (decltype(r_visframecount))ConvertDllInfoSpace((PVOID)ctx->cmp_candidateVA, ctx->DllInfo, ctx->RealDllInfo);
			else if (ctx->movexx_offset == 0 && !r_framecount)
				r_framecount = (decltype(r_framecount))ConvertDllInfoSpace((PVOID)ctx->cmp_candidateVA, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (r_visframecount && r_framecount)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, &ctx);

	Sig_VarNotFound(r_framecount);
	Sig_VarNotFound(r_visframecount);
}

void Engine_FillAddress_R_LoadSkybox(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_LoadSkyBox_SvEngine = (decltype(gPrivateFuncs.R_LoadSkyBox_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "R_LoadSkyBox_SvEngine", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else
	{
		gPrivateFuncs.R_LoadSkys = (decltype(gPrivateFuncs.R_LoadSkys))GamedataResolvePtr(RealDllInfo.ImageBase, "R_LoadSkys", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void Engine_FillAddress_GL_FilterMinMaxVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	//Global pointers that link into engine vars.
		int *gl_filter_min = NULL;
		int *gl_filter_max = NULL;
	*/

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define GL_FILTER_SIG_SVENGINE "\xDB\x05\x2A\x2A\x2A\x2A\x2A\xD9\x1C\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x2A\xDB\x05"
		/*
			.text:01D501E4                                     loc_1D501E4:                            ; CODE XREF: .text:01D501D5↑j
			.text:01D501E4 DB 05 F4 01 EE 01                                   fild    dword_1EE01F4
			.text:01D501EA 51                                                  push    ecx
			.text:01D501EB D9 1C 24                                            fstp    dword ptr [esp]
			.text:01D501EE 68 01 28 00 00                                      push    2801h
			.text:01D501F3 68 E1 0D 00 00                                      push    0DE1h
			.text:01D501F8 FF D3                                               call    ebx ; glTexParameterf
		*/

		auto addr = (ULONG_PTR)Search_Pattern(GL_FILTER_SIG_SVENGINE, DllInfo);
		if (addr)
		{
			PVOID gl_filter_min_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
			PVOID gl_filter_max_VA = (PVOID)(*(ULONG_PTR*)(addr + Sig_Length(GL_FILTER_SIG_SVENGINE)));

			gl_filter_min = (decltype(gl_filter_min))ConvertDllInfoSpace(gl_filter_min_VA, DllInfo, RealDllInfo);
			gl_filter_max = (decltype(gl_filter_max))ConvertDllInfoSpace(gl_filter_max_VA, DllInfo, RealDllInfo);
		}
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
#define GL_FILTER_SIG_GOLDSRC_HL25 "\x66\x0F\x6E\x05\x2A\x2A\x2A\x2A\x0F\x5B\xC0\x2A\xF3\x2A\x2A\x2A\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x66\x0F\x6E\x05"
		/*
			.text:1023AB74 66 0F 6E 05 28 E3 31 10                             movd    xmm0, dword_1031E328
			.text:1023AB7C 0F 5B C0                                            cvtdq2ps xmm0, xmm0
			.text:1023AB7F 51                                                  push    ecx
			.text:1023AB80 F3 0F 11 04 24                                      movss   [esp+10h+var_10], xmm0
			.text:1023AB85 68 01 28 00 00                                      push    2801h
			.text:1023AB8A 68 E1 0D 00 00                                      push    0DE1h
			.text:1023AB8F FF 15 6C 33 9B 10                                   call    dword_109B336C
		*/
		auto addr = (ULONG_PTR)Search_Pattern(GL_FILTER_SIG_GOLDSRC_HL25, DllInfo);
		if (addr)
		{
			PVOID gl_filter_min_VA = (PVOID)(*(ULONG_PTR*)(addr + 4));
			PVOID gl_filter_max_VA = (PVOID)(*(ULONG_PTR*)(addr + Sig_Length(GL_FILTER_SIG_GOLDSRC_HL25)));

			gl_filter_min = (decltype(gl_filter_min))ConvertDllInfoSpace(gl_filter_min_VA, DllInfo, RealDllInfo);
			gl_filter_max = (decltype(gl_filter_max))ConvertDllInfoSpace(gl_filter_max_VA, DllInfo, RealDllInfo);
		}
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
#define GL_FILTER_SIG_GOLDSRC_NEW "\xDB\x05\x2A\x2A\x2A\x2A\x83\xC4\x04\xD9\x2A\x2A\x2A\x2A\x2A\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xDB\x05"
#define GL_FILTER_SIG_GOLDSRC_NEW2 "\xDB\x05\x2A\x2A\x2A\x2A\x83\xC4\x04\xD9\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xDB\x05"

		/*
			.text:01D3C2DB DB 05 3C 15 E4 01                                   fild    dword_1E4153C
			.text:01D3C2E1 83 C4 04                                            add     esp, 4
			.text:01D3C2E4 D9 5D 08                                            fstp    [ebp+arg_0]
			.text:01D3C2E7 8B 4D 08                                            mov     ecx, [ebp+arg_0]
			.text:01D3C2EA 51                                                  push    ecx
			.text:01D3C2EB 68 01 28 00 00                                      push    2801h
			.text:01D3C2F0 68 E1 0D 00 00                                      push    0DE1h
			.text:01D3C2F5 FF 15 78 1B 79 02                                   call    dword_2791B78
		*/

		auto addr = (ULONG_PTR)Search_Pattern(GL_FILTER_SIG_GOLDSRC_NEW, DllInfo);
		if (addr)
		{
			PVOID gl_filter_min_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
			PVOID gl_filter_max_VA = (PVOID)(*(ULONG_PTR*)(addr + Sig_Length(GL_FILTER_SIG_GOLDSRC_NEW)));

			gl_filter_min = (decltype(gl_filter_min))ConvertDllInfoSpace(gl_filter_min_VA, DllInfo, RealDllInfo);
			gl_filter_max = (decltype(gl_filter_max))ConvertDllInfoSpace(gl_filter_max_VA, DllInfo, RealDllInfo);
		}
		else
		{
			addr = (ULONG_PTR)Search_Pattern(GL_FILTER_SIG_GOLDSRC_NEW2, DllInfo);
			if (addr)
			{
				PVOID gl_filter_min_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
				PVOID gl_filter_max_VA = (PVOID)(*(ULONG_PTR*)(addr + Sig_Length(GL_FILTER_SIG_GOLDSRC_NEW2)));

				gl_filter_min = (decltype(gl_filter_min))ConvertDllInfoSpace(gl_filter_min_VA, DllInfo, RealDllInfo);
				gl_filter_max = (decltype(gl_filter_max))ConvertDllInfoSpace(gl_filter_max_VA, DllInfo, RealDllInfo);
			}
		}
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
#define GL_FILTER_SIG_BLOB "\xDB\x05\x2A\x2A\x2A\x2A\x83\xC4\x04\xD9\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xDB\x05"
		/*
			.text:01D3CC13 DB 05 C4 20 EC 01                                   fild    dword_1EC20C4
			.text:01D3CC19 83 C4 04                                            add     esp, 4
			.text:01D3CC1C D9 5C 24 08                                         fstp    [esp+0Ch+var_4]
			.text:01D3CC20 8B 4C 24 08                                         mov     ecx, [esp+0Ch+var_4]
			.text:01D3CC24 51                                                  push    ecx
			.text:01D3CC25 68 01 28 00 00                                      push    2801h
			.text:01D3CC2A 68 E1 0D 00 00                                      push    0DE1h
			.text:01D3CC2F FF 15 3C 85 7E 02                                   call    dword_27E853C
		*/
		auto addr = (ULONG_PTR)Search_Pattern(GL_FILTER_SIG_BLOB, DllInfo);
		if (addr)
		{
			PVOID gl_filter_min_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
			PVOID gl_filter_max_VA = (PVOID)(*(ULONG_PTR*)(addr + Sig_Length(GL_FILTER_SIG_BLOB)));

			gl_filter_min = (decltype(gl_filter_min))ConvertDllInfoSpace(gl_filter_min_VA, DllInfo, RealDllInfo);
			gl_filter_max = (decltype(gl_filter_max))ConvertDllInfoSpace(gl_filter_max_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_VarNotFound(gl_filter_min);
	Sig_VarNotFound(gl_filter_max);
}

void Engine_FillAddress_ScrFov(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	//Global pointers that link into engine vars.
		float *scr_fov_value = NULL;
	*/

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		/*
		void sub_1D1A030()
		{
		  float v0; // [esp+0h] [ebp-48h]
		  float v1[8]; // [esp+4h] [ebp-44h] BYREF
		  _BYTE v2[32]; // [esp+24h] [ebp-24h] BYREF

		  if ( dword_20D7D70 == 5 && !dword_2108F6C )
		  {
			sub_1D3D7B0(v1, 0, 0x20u);
			v1[3] = flt_234B6B4;
			v1[4] = flt_234B6B8;
			v1[5] = flt_234B6BC;
			v1[0] = *((float *)dword_2723390 + 750 * dword_270AFBC + 722);
			v1[1] = *((float *)dword_2723390 + 750 * dword_270AFBC + 723);
			v1[2] = *((float *)dword_2723390 + 750 * dword_270AFBC + 724);
			v1[7] = scr_fov_value;
		*/

		const char pattern[] = "\xD9\x05\x2A\x2A\x2A\x2A\xD9\x5C\x24\x1C\x89\x44\x24\x18";
		auto addr = (ULONG_PTR)Search_Pattern(pattern, DllInfo);

		if (addr)
		{
			PVOID scrfov_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
			scr_fov_value = (decltype(scr_fov_value))ConvertDllInfoSpace(scrfov_VA, DllInfo, RealDllInfo);
		}
	}
	else
	{
		const char pattern[] = "\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x16\x43";
		auto addr = (ULONG_PTR)Search_Pattern(pattern, DllInfo);

		if (addr)
		{
			const char pattern2[] = "\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x20\x41";
			auto addr2 = (ULONG_PTR)Search_Pattern_From_Size(((PUCHAR)addr - 0x50), 0x50, pattern2);

			if (addr2)
			{
				PVOID scrfov_VA = (PVOID)(*(ULONG_PTR*)(addr2 + 2));
				scr_fov_value = (decltype(scr_fov_value))ConvertDllInfoSpace(scrfov_VA, DllInfo, RealDllInfo);
			}
		}
	}

	Sig_VarNotFound(scr_fov_value);
}

//Got CL_IsDevOverviewMode, CL_SetDevOverView and refdef here
void Engine_FillAddress_RenderSceneVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	 // We use our own struct and use pointer to link to actual engine refdef_t
	 // because SvEngine and GoldSrc have different refdef_t definitions.

		 typedef struct refdef_SvEngine_s
		{
			vrect_GoldSrc_t vrect;
			vec3_t vieworg;
			vec3_t viewangles;
			color24 ambientlight;
			byte padding;
			qboolean onlyClientDraws;
			qboolean useCamera;
			vec3_t r_camera_origin;
		}refdef_SvEngine_t;

		typedef struct refdef_GoldSrc_s
		{
			vrect_GoldSrc_t vrect;
			char padding[96];
			vec3_t vieworg;
			vec3_t viewangles;
			color24 ambientlight;
			byte padding2;
			qboolean onlyClientDraws;
		}refdef_GoldSrc_t;

		typedef struct refdef_s
		{
			vrect_GoldSrc_t *vrect;// link to &enginedll_refdef.vrect
			vec3_t *vieworg; // link to &enginedll_refdef.vieworg
			vec3_t *viewangles; // link to &enginedll_refdef.viewangles
			color24 *ambientlight;// link to &enginedll_refdef.ambientlight
			qboolean *onlyClientDraws;// link to &enginedll_refdef.onlyClientDraws
		}refdef_t;

		refdef_t r_refdef;
	*/

	PVOID SearchBase_VA = 0;
	SIZE_T SearchLength = 0;

	if (gPrivateFuncs.R_RenderScene)
	{
		SearchBase_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RenderScene, RealDllInfo, DllInfo);
		SearchLength = 0x100;
	}
	else if (gPrivateFuncs.R_RenderView_SvEngine)
	{
		SearchBase_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RenderView_SvEngine, RealDllInfo, DllInfo);
		SearchLength = 0x450;
	}
	else if (gPrivateFuncs.R_RenderView)
	{
		SearchBase_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RenderView, RealDllInfo, DllInfo);
		SearchLength = 0x450;
	}

	typedef struct R_RenderScene_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		int Call_Candidate_instCount{};
		PVOID Call_Candidate{};
	} R_RenderScene_SearchContext;

	R_RenderScene_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(SearchBase_VA, SearchLength, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
		
		auto pinst = (cs_insn*)inst;
		auto ctx = (R_RenderScene_SearchContext*)context;

		if (!gPrivateFuncs.CL_IsDevOverviewMode)
		{
			if (address[0] == 0xE8 && address[5] == 0x85 && address[6] == 0xC0)
			{
				ctx->Call_Candidate = (decltype(ctx->Call_Candidate))pinst->detail->x86.operands[0].imm;
				ctx->Call_Candidate_instCount = instCount;
			}

			if (address[0] == 0x68 && address[5] == 0xE8 && instCount > ctx->Call_Candidate_instCount && instCount <= ctx->Call_Candidate_instCount + 3)
			{
				gPrivateFuncs.CL_IsDevOverviewMode = (decltype(gPrivateFuncs.CL_IsDevOverviewMode))ConvertDllInfoSpace(ctx->Call_Candidate, ctx->DllInfo, ctx->RealDllInfo);
			}
		}

		if (gPrivateFuncs.CL_IsDevOverviewMode && !gPrivateFuncs.CL_SetDevOverView &&
			address[0] == 0xE8 && address[-5] == 0x68 && address[5] == 0x83)
		{
			if (g_iEngineType == ENGINE_SVENGINE)
			{
				r_refdef_SvEngine = (decltype(r_refdef_SvEngine))ConvertDllInfoSpace((PVOID)(*(ULONG_PTR*)(address - 4)), ctx->DllInfo, ctx->RealDllInfo);
				r_refdef.vrect = &r_refdef_SvEngine->vrect;
				r_refdef.vieworg = &r_refdef_SvEngine->vieworg;
				r_refdef.viewangles = &r_refdef_SvEngine->viewangles;
				r_refdef.ambientlight = &r_refdef_SvEngine->ambientlight;
				r_refdef.onlyClientDraws = &r_refdef_SvEngine->onlyClientDraws;
			}
			else
			{
				r_refdef_GoldSrc = (decltype(r_refdef_GoldSrc))ConvertDllInfoSpace((PVOID)(*(ULONG_PTR*)(address - 4)), ctx->DllInfo, ctx->RealDllInfo);
				r_refdef.vrect = &r_refdef_GoldSrc->vrect;
				r_refdef.vieworg = &r_refdef_GoldSrc->vieworg;
				r_refdef.viewangles = &r_refdef_GoldSrc->viewangles;
				r_refdef.ambientlight = &r_refdef_GoldSrc->ambientlight;
				r_refdef.onlyClientDraws = &r_refdef_GoldSrc->onlyClientDraws;
			}

			gPrivateFuncs.CL_SetDevOverView = (decltype(gPrivateFuncs.CL_SetDevOverView))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (gPrivateFuncs.CL_IsDevOverviewMode && gPrivateFuncs.CL_SetDevOverView)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);

	Sig_FuncNotFound(CL_IsDevOverviewMode);
	Sig_FuncNotFound(CL_SetDevOverView);
	
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Sig_VarNotFound(r_refdef_SvEngine);
	}
	else
	{
		Sig_VarNotFound(r_refdef_GoldSrc);
	}
}

//Got ClientDLL_DrawNormalTriangles_VA and gDevOverview here
void Engine_FillAddress_RenderSceneVars2(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	cl_waterlevel = (decltype(cl_waterlevel))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_waterlevel", MH_GAMESYMBOL_KIND_GLOBAL);

	PVOID SearchBase_VA = 0;
	SIZE_T SearchLength = 0;

	if (gPrivateFuncs.R_RenderScene)
	{
		SearchBase_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RenderScene, RealDllInfo, DllInfo);
		SearchLength = 0x600;
	}
	else if (gPrivateFuncs.R_RenderView_SvEngine)
	{
		SearchBase_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RenderView_SvEngine, RealDllInfo, DllInfo);
		SearchLength = 0x800;
	}
	else if (gPrivateFuncs.R_RenderView)
	{
		SearchBase_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RenderView, RealDllInfo, DllInfo);
		SearchLength = 0x800;
	}

	{
		typedef struct R_RenderScene_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		} R_RenderScene_SearchContext;

		R_RenderScene_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(SearchBase_VA, SearchLength, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_RenderScene_SearchContext*)context;

			if (address[0] == 0xE8 && instLen == 5)
			{
				auto candidate = (PUCHAR)pinst->detail->x86.operands[0].imm;

				//.text:01D1A4E0                                     ClientDLL_DrawNormalTriangles proc near ; CODE XREF: R_RenderScene:loc_1D566A7↑p
				//.text:01D1A4E0 A1 70 5B 04 02                                      mov     eax, pfnDrawNormalTriangles
				//.text:01D1A4E5 85 C0                                               test    eax, eax
				if (candidate[0] == 0xA1 && candidate[5] == 0x85 && candidate[6] == 0xC0)
				{
					auto pfnDrawNormalTriangles = *(PVOID*)(candidate + 1);
					auto pfnDrawNormalTriangles_RealDllBased = ConvertDllInfoSpace(pfnDrawNormalTriangles, ctx->DllInfo, ctx->RealDllInfo);
					if ((ULONG_PTR)pfnDrawNormalTriangles_RealDllBased == (ULONG_PTR)gPrivateFuncs.pfnDrawTransparentTriangles - sizeof(ULONG_PTR))
					{
						gPrivateFuncs.ClientDLL_DrawNormalTriangles = (decltype(gPrivateFuncs.ClientDLL_DrawNormalTriangles))ConvertDllInfoSpace((PVOID)candidate, ctx->DllInfo, ctx->RealDllInfo);
					}
				}
			}

			if (gPrivateFuncs.ClientDLL_DrawNormalTriangles)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);
	}

	Sig_FuncNotFound(ClientDLL_DrawNormalTriangles);

	PVOID CL_SetDevOverView_VA = ConvertDllInfoSpace(gPrivateFuncs.CL_SetDevOverView, RealDllInfo, DllInfo);

	{
		typedef struct OverviewZoom_SearchCotext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			int Push30_instCount{};
			ULONG_PTR CandidatesVA[6]{};
			int CandidateCount{};
		} OverviewZoom_SearchCotext;

		OverviewZoom_SearchCotext ctx2 = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(CL_SetDevOverView_VA, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto ctx = (OverviewZoom_SearchCotext*)context;
			auto pinst = (cs_insn*)inst;

			if (!ctx->Push30_instCount &&
				instCount > 100 &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				pinst->detail->x86.operands[0].imm == 0x30)
			{
				ctx->Push30_instCount = instCount;
			}

			if (ctx->CandidateCount < 6 && instCount > ctx->Push30_instCount)
			{
				if (pinst->id == X86_INS_FLD &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					ctx->CandidatesVA[ctx->CandidateCount] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->CandidateCount++;
				}
				if (pinst->id == X86_INS_MOVSS &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					ctx->CandidatesVA[ctx->CandidateCount] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
					ctx->CandidateCount++;
				}
			}

			if (ctx->CandidateCount >= 6)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx2);

		if (ctx2.CandidateCount > 0)
		{
			std::qsort(ctx2.CandidatesVA, ctx2.CandidateCount, sizeof(ctx2.CandidatesVA[0]), [](const void* a, const void* b) {
				return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
			});

			gDevOverview = (decltype(gDevOverview))ConvertDllInfoSpace((PVOID)ctx2.CandidatesVA[0], ctx2.DllInfo, ctx2.RealDllInfo);
		}
	}

	Sig_VarNotFound(gDevOverview);
}

void Engine_FillAddress_CL_IsDevOverviewModeVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		// int* allow_cheats = NULL;
		allow_cheats = (decltype(allow_cheats))GamedataResolvePtr(RealDllInfo.ImageBase, "allow_cheats", MH_GAMESYMBOL_KIND_GLOBAL);
	}
	else
	{
		// GoldSrc doesn't have such "int *allow_cheats;"
	}

}

void Engine_FillAddress_R_DecalInit(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		decal_t *gDecalPool = NULL;
		decalcache_t *gDecalCache = NULL;
	*/
	PVOID R_DecalInit_VA = 0;

	const char pattern[] = "\x68\x00\xC0\x01\x00\x6A\x00";
	R_DecalInit_VA = Search_Pattern(pattern, DllInfo);

	if (!R_DecalInit_VA)
	{
		Sig_NotFound(R_DecalInit);
	}

	typedef struct R_DecalInit_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} R_DecalInit_SearchContext;

	R_DecalInit_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(R_DecalInit_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (R_DecalInit_SearchContext*)context;

		if (pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_IMM &&
			(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		
		{
			//68 B8 5C 32 02 push    offset gDecalPool

			gDecalPool = (decltype(gDecalPool))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D49DBE B8 C0 96 BB 02 mov     eax, offset gDecalCache

			gDecalCache = (decltype(gDecalCache))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (gDecalPool && gDecalCache)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, &ctx);

	Sig_VarNotFound(gDecalPool);
	Sig_VarNotFound(gDecalCache);
}

void Engine_FillAddress_LightstyleVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	d_lightstylevalue = (decltype(d_lightstylevalue))GamedataResolvePtr(RealDllInfo.ImageBase, "d_lightstylevalue", MH_GAMESYMBOL_KIND_GLOBAL);
}

void Engine_FillAddress_WaterVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	cshift_water = (decltype(cshift_water))GamedataResolvePtr(RealDllInfo.ImageBase, "cshift_water", MH_GAMESYMBOL_KIND_GLOBAL);

	//gWaterColor remains catalog-uncovered and is still read from the V_CalcRefdef water-cshift call site.
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
#define GWATERCOLOR_SIG_HL25 "\x66\x0F\x6E\x05\x2A\x2A\x2A\x2A\xF2\x0F\x10\x0D\x2A\x2A\x2A\x2A\xF3\x0F\xE6\xC0\x68\x01\x26\x00\x00"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(GWATERCOLOR_SIG_HL25, RealDllInfo);
		Sig_AddrNotFound(gWaterColor);
		PVOID gWaterColor_VA = *(PVOID*)((PUCHAR)addr + 4);
		gWaterColor = (decltype(gWaterColor))(gWaterColor_VA);
	}
	else
	{
#define GWATERCOLOR_SIG "\xDB\x05\x2A\x2A\x2A\x2A\x68\x01\x26\x00\x00\x68\x65\x0B\x00\x00"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(GWATERCOLOR_SIG, RealDllInfo);
		Sig_AddrNotFound(gWaterColor);
		PVOID gWaterColor_VA = *(PVOID*)((PUCHAR)addr + 2);
		gWaterColor = (decltype(gWaterColor))(gWaterColor_VA);
	}

	Sig_VarNotFound(gWaterColor);
}

void Engine_FillAddress_BasePalette(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		.text:101D2EF5 68 D0 EC 2B 10                                      push    offset aPaletteLmp ; "palette.lmp"
		.text:101D2EFA 68 00 08 00 00                                      push    800h
		.text:101D2EFF E8 AC 53 06 00                                      call    sub_102382B0
		.text:101D2F04 83 C4 08                                            add     esp, 8
	*/

	char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x00\x08\x00\x00\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\xA3";
	auto palette_Call = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(palette_Call);

	PVOID host_basepal_VA = *(PVOID*)((PUCHAR)palette_Call + Sig_Length(pattern));
	host_basepal = (decltype(host_basepal))ConvertDllInfoSpace(host_basepal_VA, DllInfo, RealDllInfo);

	Sig_VarNotFound(host_basepal);
}

void Engine_FillAddress_R_LightStrengthVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		vec3_t *locallight = NULL;
		int *numlights = NULL;
	*/

	if (gPrivateFuncs.R_LightStrength)
	{
		// Convert R_LightStrength to DllInfo-based
		PVOID R_LightStrength_VA = ConvertDllInfoSpace(gPrivateFuncs.R_LightStrength, RealDllInfo, DllInfo);

		typedef struct R_LightStrength_Context_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;

			int MovZeroEbp_InstCount{};
			int MovZeroEbp_Disp{};
			int MovZeroEbp_Reg{};
			int ZerodReg{};
			int ZerodReg_InstCount{};
			int Candidate_locallight_InstCount{};
			ULONG_PTR Candidate_locallight{};
			int Candidate_numlights_instCount{};
			int Candidate_numlights_reg{};
			ULONG_PTR Candidate_numlights{};
			int Candidate_locallight2_InstCount{};
			int Candidate_locallight2_reg{};
			ULONG_PTR Candidate_locallight2{};
		}R_LightStrength_Context;

		R_LightStrength_Context ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(R_LightStrength_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (R_LightStrength_Context*)context;

			if (
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.disp != 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{
				ctx->MovZeroEbp_InstCount = instCount;
				ctx->MovZeroEbp_Disp = pinst->detail->x86.operands[0].mem.disp;
			}

			if (ctx->MovZeroEbp_InstCount &&
				instCount > ctx->MovZeroEbp_InstCount &&
				instCount < ctx->MovZeroEbp_InstCount + 20 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.disp == ctx->MovZeroEbp_Disp)
			{
				ctx->ZerodReg_InstCount = instCount;
				ctx->ZerodReg = pinst->detail->x86.operands[0].reg;
			}

			if (!ctx->ZerodReg &&
				!ctx->ZerodReg_InstCount &&
				pinst->id == X86_INS_XOR &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == pinst->detail->x86.operands[1].reg)
			{
				ctx->ZerodReg = pinst->detail->x86.operands[0].reg;
				ctx->ZerodReg_InstCount = instCount;
			}

			if (ctx->ZerodReg &&
				ctx->ZerodReg_InstCount &&
				instCount > ctx->ZerodReg_InstCount &&
				instCount < ctx->ZerodReg_InstCount + 20 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.scale == 4 &&
				pinst->detail->x86.operands[1].mem.index == ctx->ZerodReg &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				//.text:01D83D28 8B 04 BD C4 A0 37 02                                mov     eax, locallight[edi*4]
				ctx->Candidate_locallight = (decltype(ctx->Candidate_locallight))pinst->detail->x86.operands[1].mem.disp;
				ctx->Candidate_locallight_InstCount = instCount;
			}

			if (!locallight &&
				ctx->ZerodReg &&
				ctx->ZerodReg_InstCount &&
				ctx->Candidate_locallight_InstCount &&
				instCount > ctx->Candidate_locallight_InstCount &&
				instCount < ctx->Candidate_locallight_InstCount + 40 &&
				pinst->id == X86_INS_INC &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == ctx->ZerodReg)
			{
				locallight = (decltype(locallight))ConvertDllInfoSpace((PVOID)ctx->Candidate_locallight, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				//.text:01D99694 BB 8C AB 40 02                                      mov     ebx, offset locallight
				ctx->Candidate_locallight2 = (decltype(ctx->Candidate_locallight2))pinst->detail->x86.operands[1].imm;
				ctx->Candidate_locallight2_InstCount = instCount;
				ctx->Candidate_locallight2_reg = pinst->detail->x86.operands[0].reg;
			}

			if (!locallight && ctx->Candidate_locallight2_InstCount &&
				instCount > ctx->Candidate_locallight2_InstCount &&
				instCount < ctx->Candidate_locallight2_InstCount + 40 &&
				pinst->id == X86_INS_ADD &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == ctx->Candidate_locallight2_reg &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 4)
			{
				locallight = (decltype(locallight))ConvertDllInfoSpace((PVOID)ctx->Candidate_locallight2, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (!numlights &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[0].type == X86_OP_REG)
			{
				ctx->Candidate_numlights = (decltype(ctx->Candidate_numlights))pinst->detail->x86.operands[1].mem.disp;
				ctx->Candidate_numlights_instCount = instCount;
				ctx->Candidate_numlights_reg = pinst->detail->x86.operands[0].reg;
			}

			if (!numlights &&
				ctx->Candidate_numlights &&
				ctx->Candidate_numlights_instCount &&
				instCount > ctx->Candidate_numlights_instCount &&
				instCount < ctx->Candidate_numlights_instCount + 30 &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				((pinst->detail->x86.operands[0].reg == ctx->Candidate_numlights_reg &&
					pinst->detail->x86.operands[1].reg == ctx->ZerodReg) ||
					(pinst->detail->x86.operands[1].reg == ctx->Candidate_numlights_reg &&
						pinst->detail->x86.operands[0].reg == ctx->ZerodReg)))
			{
				numlights = (decltype(numlights))ConvertDllInfoSpace((PVOID)ctx->Candidate_numlights, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (!numlights &&
				ctx->Candidate_numlights &&
				ctx->Candidate_numlights_instCount &&
				instCount > ctx->Candidate_numlights_instCount &&
				instCount < ctx->Candidate_numlights_instCount + 30 &&
				pinst->id == X86_INS_TEST &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == ctx->Candidate_numlights_reg)
			{
				numlights = (decltype(numlights))ConvertDllInfoSpace((PVOID)ctx->Candidate_numlights, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (locallight && numlights)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	}
	else
	{
		PVOID R_GLStudioDrawPoints_VA = ConvertDllInfoSpace(gPrivateFuncs.R_GLStudioDrawPoints, RealDllInfo, DllInfo);

		typedef struct R_LightStrength_Context_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;

			PVOID base{};
			size_t max_insts{};
			int max_depth{};
			std::set<PVOID> code{};
			std::set<PVOID> branches{};
			std::vector<walk_context_t> walks{};
			int MovZeroEbp_InstCount{};
			int MovZeroEbp_Disp{};
			int MovZeroEbp_Reg{};
			int ZerodReg{};
			int ZerodReg_InstCount{};
			int Candidate_InstCount{};
			ULONG_PTR Candidate_locallight{};
			int Candidate_numlights_instCount{};
			int Candidate_numlights_reg{};
			ULONG_PTR Candidate_numlights{};
		}R_LightStrength_Context;

		R_LightStrength_Context ctx = { DllInfo, RealDllInfo };

		ctx.base = R_GLStudioDrawPoints_VA;
		ctx.max_insts = 1000;
		ctx.max_depth = 16;
		ctx.walks.emplace_back(ctx.base, 0x1000, 0);

		while (ctx.walks.size())
		{
			auto walk = ctx.walks[ctx.walks.size() - 1];
			ctx.walks.pop_back();

			g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_LightStrength_Context*)context;

				if (locallight && numlights)
					return TRUE;

				if (ctx->code.size() > ctx->max_insts)
					return TRUE;

				if (ctx->code.find(address) != ctx->code.end())
					return TRUE;

				ctx->code.emplace(address);

				if (
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0 &&
					pinst->detail->x86.operands[0].mem.disp != 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0)
				{
					ctx->MovZeroEbp_InstCount = instCount;
					ctx->MovZeroEbp_Disp = pinst->detail->x86.operands[0].mem.disp;
				}

				if (ctx->MovZeroEbp_InstCount &&
					instCount > ctx->MovZeroEbp_InstCount &&
					instCount < ctx->MovZeroEbp_InstCount + 20 &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.disp == ctx->MovZeroEbp_Disp)
				{
					ctx->ZerodReg_InstCount = instCount;
					ctx->ZerodReg = pinst->detail->x86.operands[0].reg;
				}

				if (!ctx->ZerodReg &&
					!ctx->ZerodReg_InstCount &&
					pinst->id == X86_INS_XOR &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == pinst->detail->x86.operands[1].reg)
				{
					ctx->ZerodReg = pinst->detail->x86.operands[0].reg;
					ctx->ZerodReg_InstCount = instCount;
				}

				if (ctx->ZerodReg &&
					ctx->ZerodReg_InstCount &&
					instCount > ctx->ZerodReg_InstCount &&
					instCount < ctx->ZerodReg_InstCount + 20 &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					pinst->detail->x86.operands[1].mem.scale == 4 &&
					pinst->detail->x86.operands[1].mem.index == ctx->ZerodReg &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					//.text:01D83D28 8B 04 BD C4 A0 37 02                                mov     eax, locallight[edi*4]
					ctx->Candidate_locallight = (decltype(ctx->Candidate_locallight))pinst->detail->x86.operands[1].mem.disp;
					ctx->Candidate_InstCount = instCount;
				}

				if (ctx->ZerodReg &&
					ctx->ZerodReg_InstCount &&
					ctx->Candidate_InstCount &&
					instCount > ctx->Candidate_InstCount &&
					instCount < ctx->Candidate_InstCount + 40 &&
					pinst->id == X86_INS_INC &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == ctx->ZerodReg)
				{
					locallight = (decltype(locallight))ConvertDllInfoSpace((PVOID)ctx->Candidate_locallight, ctx->DllInfo, ctx->RealDllInfo);
				}

				if (!numlights &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					pinst->detail->x86.operands[1].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
					pinst->detail->x86.operands[0].type == X86_OP_REG)
				{
					ctx->Candidate_numlights = (decltype(ctx->Candidate_numlights))pinst->detail->x86.operands[1].mem.disp;
					ctx->Candidate_numlights_instCount = instCount;
					ctx->Candidate_numlights_reg = pinst->detail->x86.operands[0].reg;
				}

				if (!numlights &&
					ctx->Candidate_numlights &&
					ctx->Candidate_numlights_instCount &&
					instCount > ctx->Candidate_numlights_instCount &&
					instCount < ctx->Candidate_numlights_instCount + 30 &&
					pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					((pinst->detail->x86.operands[0].reg == ctx->Candidate_numlights_reg &&
						pinst->detail->x86.operands[1].reg == ctx->ZerodReg) ||
						(pinst->detail->x86.operands[1].reg == ctx->Candidate_numlights_reg &&
							pinst->detail->x86.operands[0].reg == ctx->ZerodReg)))
				{
					numlights = (decltype(numlights))ConvertDllInfoSpace((PVOID)ctx->Candidate_numlights, ctx->DllInfo, ctx->RealDllInfo);
				}

				if ((pinst->id == X86_INS_JMP || (pinst->id >= X86_INS_JAE && pinst->id <= X86_INS_JS)) &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM)
				{
					PVOID imm = (PVOID)pinst->detail->x86.operands[0].imm;
					auto foundbranch = ctx->branches.find(imm);
					if (foundbranch == ctx->branches.end())
					{
						ctx->branches.emplace(imm);
						if (depth + 1 < ctx->max_depth)
							ctx->walks.emplace_back(imm, 0x300, depth + 1);
					}

					if (pinst->id == X86_INS_JMP)
						return TRUE;
				}

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, walk.depth, &ctx);
		}

	}

	Sig_VarNotFound(locallight);
	Sig_VarNotFound(numlights);
}

void Engine_FillAddress_MoveVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		/*
.text:01DA3890                                     CL_ParseMovevars proc near              ; CODE XREF: sub_1DA0DE0+3D8↑p
.text:01DA3890                                                                             ; .text:01DA3F01↓p ...
.text:01DA3890
.text:01DA3890                                     var_8           = dword ptr -8
.text:01DA3890                                     arg_0           = dword ptr  4
.text:01DA3890
.text:01DA3890 56                                                  push    esi
.text:01DA3891 8B 74 24 08                                         mov     esi, [esp+4+arg_0]
.text:01DA3895 6A 2C                                               push    2Ch ; ','
.text:01DA3897 56                                                  push    esi
.text:01DA3898 E8 F3 87 F9 FF                                      call    MSG_ReadFloat
.text:01DA389D D9 05 C8 B4 55 08                                   fld     movevars_gravity
		*/

		const char pattern[] = "\x56\x8B\x74\x24\x08\x6A\x2C\x56\xE8\x2A\x2A\x2A\x2A\xD9\x05";

		auto addr = (DWORD)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(pmovevars);

		PVOID pmovevars_VA = *(PVOID *)(addr + Sig_Length(pattern));
		pmovevars = (decltype(pmovevars))ConvertDllInfoSpace(pmovevars_VA, DllInfo, RealDllInfo);
	}
	else
	{
		/*
		.text:101A74F0                                     CL_ParseMovevars    proc near               ; CODE XREF: sub_101AA080↓j
		.text:101A74F0
		.text:101A74F0                                     var_4           = dword ptr -4
		.text:101A74F0
		.text:101A74F0 E8 7B 27 01 00                                      call    MSG_ReadFloat
		.text:101A74F5 D9 1D 20 47 22 11                                   fstp    movevars.gravity
		.text:101A74FB E8 70 27 01 00                                      call    MSG_ReadFloat
		.text:101A7500 D9 1D 24 47 22 11                                   fstp    movevars.stopspeed
		.text:101A7506 E8 65 27 01 00                                      call    MSG_ReadFloat
		.text:101A750B D9 1D 28 47 22 11                                   fstp    movevars.maxspeed
		*/
		const char pattern[] = "\xE8\x2A\x2A\x2A\x2A\xD9\x1D\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xD9\x1D\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xD9\x1D\x2A\x2A\x2A\x2A";

		auto addr = (DWORD)Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(pmovevars);

		PVOID pmovevars_VA = *(PVOID*)(addr + 7);
		pmovevars = (decltype(pmovevars))ConvertDllInfoSpace(pmovevars_VA, DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(pmovevars);
}

void Engine_FillAddress_MissingTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		/*
.text:01D56BC6 6A 00                                               push    0
.text:01D56BC8 68 14 66 E6 01                                      push    offset aMissing ; "**missing**"
.text:01D56BCD E8 AE 80 FF FF                                      call    GL_LoadTexture
		*/
		const char sigs[] = "**missing**\0";
		auto Missing_String = Search_Pattern_NoWildCard_Data(sigs, DllInfo);
		if (!Missing_String)
			Missing_String = Search_Pattern_NoWildCard_Rdata(sigs, DllInfo);
		if (Missing_String)
		{
			char pattern[] = "\x6A\x00\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 3) = (DWORD)Missing_String;
			auto Missing_Call = Search_Pattern(pattern, DllInfo);
			if (Missing_Call)
			{
				typedef struct MissingTexture_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
				} MissingTexture_SearchContext;

				MissingTexture_SearchContext ctx = { DllInfo, RealDllInfo };

				g_pMetaHookAPI->DisasmRanges((PUCHAR)Missing_Call + Sig_Length(pattern), 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (MissingTexture_SearchContext*)context;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
					{
						r_missingtexture = (decltype(r_missingtexture))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}

					if (r_missingtexture)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
					}, 0, &ctx);
			}
		}
		Sig_VarNotFound(r_missingtexture);
	}
	else
	{
		//r_missingtexture is only available in SvEngine. not a thing in GoldSrc.
	}

	if (1)
	{
		/*
.text:01D56BC6 6A 00                                               push    0
.text:01D56BC8 68 14 66 E6 01                                      push    offset aMissing ; "**empty**"
.text:01D56BCD E8 AE 80 FF FF                                      call    GL_LoadTexture
		*/
		const char sigs[] = "**empty**\0";
		auto Empty_String = Search_Pattern_NoWildCard_Data(sigs, DllInfo);
		if (!Empty_String)
			Empty_String = Search_Pattern_NoWildCard_Rdata(sigs, DllInfo);
		if (Empty_String)
		{
			char pattern[] = "\x6A\x00\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 3) = (DWORD)Empty_String;
			auto Empty_Call = Search_Pattern(pattern, DllInfo);
			if (Empty_Call)
			{
				typedef struct EmptyTexture_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
				} EmptyTexture_SearchContext;

				EmptyTexture_SearchContext ctx = { DllInfo, RealDllInfo };

				g_pMetaHookAPI->DisasmRanges((PUCHAR)Empty_Call + Sig_Length(pattern), 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
					auto pinst = (cs_insn*)inst;
					auto ctx = (EmptyTexture_SearchContext*)context;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
					{
						r_notexture_mip = (decltype(r_notexture_mip))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}

					if (r_notexture_mip)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
					}, 0, &ctx);
			}
		}
		Sig_VarNotFound(r_notexture_mip);
	}
}

void Engine_FillAddress_NoTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID r_notexture_mip_VA = 0;
	ULONG r_notexture_mip_RVA = 0;

	if (1)
	{
		/*
.text:01D56BC6 6A 00                                               push    0
.text:01D56BC8 68 14 66 E6 01                                      push    offset aMissing ; "**empty**"
.text:01D56BCD E8 AE 80 FF FF                                      call    GL_LoadTexture
		*/
		const char sigs[] = "**empty**\0";
		auto Empty_String = Search_Pattern_NoWildCard_Data(sigs, DllInfo);
		if (!Empty_String)
			Empty_String = Search_Pattern_NoWildCard_Rdata(sigs, DllInfo);
		if (Empty_String)
		{
			char pattern[] = "\x6A\x00\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 3) = (DWORD)Empty_String;
			auto Empty_Call = Search_Pattern(pattern, DllInfo);
			if (Empty_Call)
			{
				typedef struct EmptyTexture_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
				} EmptyTexture_SearchContext;

				EmptyTexture_SearchContext ctx = { DllInfo, RealDllInfo };

				g_pMetaHookAPI->DisasmRanges((PUCHAR)Empty_Call + Sig_Length(pattern), 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
					auto pinst = (cs_insn*)inst;
					auto ctx = (EmptyTexture_SearchContext*)context;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
					{
						r_notexture_mip = (decltype(r_notexture_mip))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}

					if (r_notexture_mip)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
					}, 0, &ctx);
			}
		}
		Sig_VarNotFound(r_notexture_mip);
	}
}

void Engine_FillAddress_LegacyMultiTextureInit(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	detTexSupported = (decltype(detTexSupported))GamedataResolvePtr(RealDllInfo.ImageBase, "detTexSupported", MH_GAMESYMBOL_KIND_GLOBAL);

	//We only need to neuter one function per engine: the legacy multitexture probe is the
	//sole caller of DT_Initialize wherever both exist, so hooking the probe covers both.
	//HL/CoF/blob call it CheckMultiTextureExtensions, SvEngine calls it InitMultitexturing.
	//HL25 and SvEngine inline the probe into GL_Init on Windows and publish no record for
	//it, so those fall back to the standalone DT_Initialize that GL_Init still calls.
	static const char* s_LegacyMultiTextureInitNames[] = {
		"CheckMultiTextureExtensions",
		"InitMultitexturing",
		"DT_Initialize",
	};

	for (auto name : s_LegacyMultiTextureInitNames)
	{
		gPrivateFuncs.LegacyMultiTextureInit = (decltype(gPrivateFuncs.LegacyMultiTextureInit))GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, name, MH_GAMESYMBOL_KIND_FUNCTION);

		if (gPrivateFuncs.LegacyMultiTextureInit)
			break;
	}

	Sig_FuncNotFound(LegacyMultiTextureInit);
}

void Engine_FillAddress_DrawStartupGraphic(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.CVideoMode_Common_DrawStartupGraphic)
		return;

	gPrivateFuncs.CVideoMode_Common_DrawStartupGraphic = (decltype(gPrivateFuncs.CVideoMode_Common_DrawStartupGraphic))GamedataResolvePtr(RealDllInfo.ImageBase, "CVideoMode_Common_DrawStartupGraphic", MH_GAMESYMBOL_KIND_FUNCTION);

	PVOID DrawStartupGraphic_VA = (PVOID)gPrivateFuncs.CVideoMode_Common_DrawStartupGraphic;


	{
		typedef struct DrawStartupGraphic_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			int xor_reg{};
			int xor_instCount{};
			int mov_reg{};
			int mov_instCount{};
			int mov_candidateOffset{};
		} DrawStartupGraphic_SearchContext;

		DrawStartupGraphic_SearchContext ctx = { RealDllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(DrawStartupGraphic_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (DrawStartupGraphic_SearchContext*)context;

			if (!ctx->xor_reg &&
				pinst->id == X86_INS_XOR &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == pinst->detail->x86.operands[1].reg)
			{
				ctx->xor_instCount = instCount;
				ctx->xor_reg = pinst->detail->x86.operands[0].reg;
			}

			if (ctx->xor_reg &&
				instCount > ctx->xor_instCount && 
				instCount < ctx->xor_instCount + 5 &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0 &&
				pinst->detail->x86.operands[0].mem.disp >= 0x100 && pinst->detail->x86.operands[0].mem.disp < 0x400 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG&&
				pinst->detail->x86.operands[1].reg == ctx->xor_reg)
			{
				gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size = pinst->detail->x86.operands[0].mem.disp;
				gPrivateFuncs.offset_CVideoMode_Common_m_ImageID = gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size - sizeof(CUtlMemory<bimage_t>);
				gPrivateFuncs.offset_CVideoMode_Common_m_iBaseResX = gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size + 8;
				gPrivateFuncs.offset_CVideoMode_Common_m_iBaseResY = gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size + 12;
			}

			if (!ctx->mov_reg &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base != 0 &&
				pinst->detail->x86.operands[1].mem.disp >= 0x100 && pinst->detail->x86.operands[0].mem.disp < 0x400)
			{
				ctx->mov_instCount = instCount;
				ctx->mov_reg = pinst->detail->x86.operands[0].reg;
				ctx->mov_candidateOffset = pinst->detail->x86.operands[1].mem.disp;
			}

			if (ctx->mov_reg &&
				instCount > ctx->mov_instCount &&
				instCount < ctx->mov_instCount + 3 &&
				pinst->id == X86_INS_TEST &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == pinst->detail->x86.operands[1].reg &&
				pinst->detail->x86.operands[0].reg == ctx->mov_reg)
			{
				gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size = ctx->mov_candidateOffset;
				gPrivateFuncs.offset_CVideoMode_Common_m_ImageID = gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size - sizeof(CUtlMemory<bimage_t>);
				gPrivateFuncs.offset_CVideoMode_Common_m_iBaseResX = gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size + 8;
				gPrivateFuncs.offset_CVideoMode_Common_m_iBaseResY = gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size + 12;
			}

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0 &&
				pinst->detail->x86.operands[0].mem.disp >= 0x100 && pinst->detail->x86.operands[0].mem.disp < 0x400 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0 )
			{
				gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size = pinst->detail->x86.operands[0].mem.disp;
				gPrivateFuncs.offset_CVideoMode_Common_m_ImageID = gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size - sizeof(CUtlMemory<bimage_t>);
				gPrivateFuncs.offset_CVideoMode_Common_m_iBaseResX = gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size + 8;
				gPrivateFuncs.offset_CVideoMode_Common_m_iBaseResY = gPrivateFuncs.offset_CVideoMode_Common_m_ImageID_Size + 12;
			}

			if (gPrivateFuncs.offset_CVideoMode_Common_m_ImageID)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);
	}

	Sig_FuncNotFound(offset_CVideoMode_Common_m_ImageID);
}

void Engine_FillAddress_DrawStartupVideo(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.CGame_DrawStartupVideo)
		return;

	//Only available in HL25
	if (g_iEngineType != ENGINE_GOLDSRC_HL25)
		return;

	gPrivateFuncs.CGame_DrawStartupVideo = (decltype(gPrivateFuncs.CGame_DrawStartupVideo))GamedataResolvePtr(RealDllInfo.ImageBase, "CGame_DrawStartupVideo", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_Draw_Frame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_Frame)
		return;

	gPrivateFuncs.Draw_Frame = (decltype(gPrivateFuncs.Draw_Frame))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_Frame", MH_GAMESYMBOL_KIND_FUNCTION);

	PVOID Draw_Frame_VA = (PVOID)gPrivateFuncs.Draw_Frame;


	{
		typedef struct Draw_Frame_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			int test_instCount{ };
			int test_reg{};
			int xor_instCount{};
			int xor_reg{};
			PVOID test_CandidateVA{};
			int Cmp_instCount{};
			ULONG_PTR CandidatesVA[6]{};
			int CandidateCount{};
		} Draw_Frame_SearchContext;

		Draw_Frame_SearchContext ctx = { RealDllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(Draw_Frame_VA, 0x120, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (Draw_Frame_SearchContext*)context;

			if (!ctx->xor_reg &&
				pinst->id == X86_INS_XOR &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == pinst->detail->x86.operands[1].reg)
			{
				ctx->xor_instCount = instCount;
				ctx->xor_reg = pinst->detail->x86.operands[0].reg;
			}

			if (!giScissorTest &&
				!ctx->test_reg &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
			{
				ctx->test_reg = pinst->detail->x86.operands[0].reg;
				ctx->test_instCount = instCount;
				ctx->test_CandidateVA = (decltype(ctx->test_CandidateVA))pinst->detail->x86.operands[1].mem.disp;
			}

			if (!giScissorTest &&
				ctx->test_reg &&
				instCount > ctx->test_instCount &&
				instCount < ctx->test_instCount + 3 &&
				pinst->id == X86_INS_TEST &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == ctx->test_reg &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == ctx->test_reg)
			{
				giScissorTest = (decltype(giScissorTest))ConvertDllInfoSpace((PVOID)ctx->test_CandidateVA, ctx->RealDllInfo, ctx->RealDllInfo);
				ctx->Cmp_instCount = instCount;
			}

			if (!giScissorTest &&
				!ctx->Cmp_instCount &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{
				giScissorTest = (decltype(giScissorTest))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->RealDllInfo, ctx->RealDllInfo);
				ctx->Cmp_instCount = instCount;
			}

			if (!giScissorTest &&
				!ctx->Cmp_instCount &&
				instCount > ctx->xor_instCount &&
				instCount < ctx->xor_instCount + 15 &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == ctx->xor_reg)
			{
				giScissorTest = (decltype(giScissorTest))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->RealDllInfo, ctx->RealDllInfo);
				ctx->Cmp_instCount = instCount;
			}

			if (ctx->Cmp_instCount > 0 && ctx->CandidateCount < 4 && instCount > ctx->Cmp_instCount && instCount < ctx->Cmp_instCount + 15)
			{
				if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
				{
					ctx->CandidatesVA[ctx->CandidateCount] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->CandidateCount++;
				}
				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->RealDllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->RealDllInfo.DataBase + ctx->RealDllInfo.DataSize)
				{
					ctx->CandidatesVA[ctx->CandidateCount] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
					ctx->CandidateCount++;
				}
			}

			if (giScissorTest && ctx->CandidateCount >= 4)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		if (ctx.CandidateCount > 0)
		{
			std::qsort(ctx.CandidatesVA, ctx.CandidateCount, sizeof(ctx.CandidatesVA[0]), [](const void* a, const void* b) {
				return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
				});

			scissor_x = (decltype(scissor_x))ConvertDllInfoSpace((PVOID)ctx.CandidatesVA[0], ctx.DllInfo, ctx.RealDllInfo);
			scissor_y = (decltype(scissor_y))ConvertDllInfoSpace((PVOID)ctx.CandidatesVA[1], ctx.DllInfo, ctx.RealDllInfo);
			scissor_width = (decltype(scissor_width))ConvertDllInfoSpace((PVOID)ctx.CandidatesVA[2], ctx.DllInfo, ctx.RealDllInfo);
			scissor_height = (decltype(scissor_height))ConvertDllInfoSpace((PVOID)ctx.CandidatesVA[3], ctx.DllInfo, ctx.RealDllInfo);
		}
	}

	Sig_VarNotFound(giScissorTest);
	Sig_VarNotFound(scissor_x);
	Sig_VarNotFound(scissor_y);
	Sig_VarNotFound(scissor_width);
	Sig_VarNotFound(scissor_height);
}

void Engine_FillAddress_Draw_SpriteFrameHoles(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_SpriteFrameHoles || gPrivateFuncs.Draw_SpriteFrameHoles_SvEngine)
		return;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.Draw_SpriteFrameHoles_SvEngine = (decltype(gPrivateFuncs.Draw_SpriteFrameHoles_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameHoles_SvEngine", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else
	{
		gPrivateFuncs.Draw_SpriteFrameHoles = (decltype(gPrivateFuncs.Draw_SpriteFrameHoles))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameHoles", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void Engine_FillAddress_Draw_SpriteFrameAdditive(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_SpriteFrameAdditive || gPrivateFuncs.Draw_SpriteFrameAdditive_SvEngine)
		return;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.Draw_SpriteFrameAdditive_SvEngine = (decltype(gPrivateFuncs.Draw_SpriteFrameAdditive_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameAdditive_SvEngine", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else
	{
		gPrivateFuncs.Draw_SpriteFrameAdditive = (decltype(gPrivateFuncs.Draw_SpriteFrameAdditive))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameAdditive", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

void Engine_FillAddress_Draw_SpriteFrameGeneric(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_SpriteFrameGeneric || gPrivateFuncs.Draw_SpriteFrameGeneric_SvEngine)
		return;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.Draw_SpriteFrameGeneric_SvEngine = (decltype(gPrivateFuncs.Draw_SpriteFrameGeneric_SvEngine))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameGeneric_SvEngine", MH_GAMESYMBOL_KIND_FUNCTION);
	}
	else
	{
		gPrivateFuncs.Draw_SpriteFrameGeneric = (decltype(gPrivateFuncs.Draw_SpriteFrameGeneric))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_SpriteFrameGeneric", MH_GAMESYMBOL_KIND_FUNCTION);
	}
}

//SvEngine replaced the immediate-mode netgraph rectangle with a buffered one that takes
//eight integers (x, y, w, h, r, g, b, a) and appends four vertices to a 1024-entry vertex
//buffer. The catalog used to publish that body under the wrong name NET_DrawRect; it is
//now published as Draw_FillRGBABuf with no compatibility alias, so the old name no longer
//resolves. No other engine identity carries it.
void Engine_FillAddress_Draw_FillRGBABuf(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_FillRGBABuf)
		return;

	if (g_iEngineType != ENGINE_SVENGINE)
		return;

	gPrivateFuncs.Draw_FillRGBABuf = (decltype(gPrivateFuncs.Draw_FillRGBABuf))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_FillRGBABuf", MH_GAMESYMBOL_KIND_FUNCTION);
}

//Every engine identity publishes Mod_UnloadSpriteTextures as a standalone body that
//SPR_Shutdown still calls out of line, SvEngine and HL25 included, so the inline hook
//on it covers the HUD sprite list too and there is no branch left to keep.
//take it carefully with linux build ! Mod_UnloadSpriteTextures can be inlined into SPR_Shutdown in linux build !
void Engine_FillAddress_Mod_UnloadSpriteTextures(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Mod_UnloadSpriteTextures)
		return;

	gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_UnloadSpriteTextures", MH_GAMESYMBOL_KIND_FUNCTION);
}

//SvEngine keeps the cl_enginefuncs slots 11 and 130, but the entries there only forward to
//the real drawing bodies, so the catalog publishes the bodies for every engine identity and
//the dispatch has no branch left to keep.
void Engine_FillAddress_Draw_FillRGBA(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_FillRGBA)
		return;

	gPrivateFuncs.Draw_FillRGBA = (decltype(gPrivateFuncs.Draw_FillRGBA))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_FillRGBA", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress_Draw_FillRGBABlend(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_FillRGBABlend)
		return;

	gPrivateFuncs.Draw_FillRGBABlend = (decltype(gPrivateFuncs.Draw_FillRGBABlend))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_FillRGBABlend", MH_GAMESYMBOL_KIND_FUNCTION);
}

//SvEngine has no body with the legacy (vrect_t*, color*) interface: its connection message
//fills the rectangle through the eight-integer Draw_FillRGBABlend instead, which is hooked
//on its own, so there is nothing left to resolve or hook here on that engine.
void Engine_FillAddress_D_FillRect(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.D_FillRect)
		return;

	if (g_iEngineType == ENGINE_SVENGINE)
		return;

	gPrivateFuncs.D_FillRect = (decltype(gPrivateFuncs.D_FillRect))GamedataResolvePtr(RealDllInfo.ImageBase, "D_FillRect", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_FillAddress(const mh_dll_info_t &DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto hSDL2 = GetModuleHandleA("SDL2.dll");

	if (hSDL2)
	{
		gPrivateFuncs.SDL_GetWindowPosition = (decltype(gPrivateFuncs.SDL_GetWindowPosition))GetProcAddress(hSDL2, "SDL_GetWindowPosition");
		gPrivateFuncs.SDL_GL_SetAttribute = (decltype(gPrivateFuncs.SDL_GL_SetAttribute))GetProcAddress(hSDL2, "SDL_GL_SetAttribute");
		gPrivateFuncs.SDL_GetWindowSize = (decltype(gPrivateFuncs.SDL_GetWindowSize))GetProcAddress(hSDL2, "SDL_GetWindowSize");
		gPrivateFuncs.SDL_GL_SwapWindow = (decltype(gPrivateFuncs.SDL_GL_SwapWindow))GetProcAddress(hSDL2, "SDL_GL_SwapWindow");
		gPrivateFuncs.SDL_GL_GetProcAddress = (decltype(gPrivateFuncs.SDL_GL_GetProcAddress))GetProcAddress(hSDL2, "SDL_GL_GetProcAddress");
		gPrivateFuncs.SDL_CreateWindow = (decltype(gPrivateFuncs.SDL_CreateWindow))GetProcAddress(hSDL2, "SDL_CreateWindow");
		//Fuck Sniber
		gPrivateFuncs.SDL_GL_ExtensionSupported = (decltype(gPrivateFuncs.SDL_GL_ExtensionSupported))GetProcAddress(hSDL2, "SDL_GL_ExtensionSupported");
	}

	auto engineFactory = g_pMetaHookAPI->GetEngineFactory();

	if (engineFactory("SCEngineClient002", nullptr))
	{
		gPrivateFuncs.SvEngine_glewInit = (decltype(gPrivateFuncs.SvEngine_glewInit))GetProcAddress(g_pMetaHookAPI->GetEngineModule(), "_glewInit@0");
	}

	gPrivateFuncs.triapi_RenderMode = gEngfuncs.pTriAPI->RenderMode;
	gPrivateFuncs.triapi_Begin = gEngfuncs.pTriAPI->Begin;
	gPrivateFuncs.triapi_End = gEngfuncs.pTriAPI->End;
	gPrivateFuncs.triapi_Color4f = gEngfuncs.pTriAPI->Color4f;
	gPrivateFuncs.triapi_Color4ub = gEngfuncs.pTriAPI->Color4ub;
	gPrivateFuncs.triapi_TexCoord2f = gEngfuncs.pTriAPI->TexCoord2f;
	gPrivateFuncs.triapi_Vertex3fv = gEngfuncs.pTriAPI->Vertex3fv;
	gPrivateFuncs.triapi_Vertex3f = gEngfuncs.pTriAPI->Vertex3f;
	gPrivateFuncs.triapi_Brightness = gEngfuncs.pTriAPI->Brightness;
	gPrivateFuncs.triapi_Color4fRendermode = gEngfuncs.pTriAPI->Color4fRendermode;
	gPrivateFuncs.triapi_GetMatrix = gEngfuncs.pTriAPI->GetMatrix;
	gPrivateFuncs.triapi_BoxInPVS = gEngfuncs.pTriAPI->BoxInPVS;
	gPrivateFuncs.triapi_Fog = gEngfuncs.pTriAPI->Fog;
	gPrivateFuncs.triapi_FogParams = gEngfuncs.pTriAPI->FogParams;
	gPrivateFuncs.triapi_SpriteTexture = gEngfuncs.pTriAPI->SpriteTexture;

	EngineSurface_FillAddress(DllInfo, RealDllInfo);

	VideoMode_FillAddress(DllInfo, RealDllInfo);

	Engine_FillAddress_HasOfficialFBOSupport(DllInfo, RealDllInfo);

	Engine_FillAddress_HasOfficialGLTexAllocSupport(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_Init(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_SetMode(DllInfo, RealDllInfo);

	gPrivateFuncs.GL_Shutdown = (decltype(gPrivateFuncs.GL_Shutdown))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_Shutdown", MH_GAMESYMBOL_KIND_FUNCTION);
	gPrivateFuncs.Sys_ShutdownGame_call_GL_Shutdown = (decltype(gPrivateFuncs.Sys_ShutdownGame_call_GL_Shutdown))GamedataResolvePtr(RealDllInfo.ImageBase, "Sys_ShutdownGame_to_GL_Shutdown_callsite_0", MH_GAMESYMBOL_KIND_PATCH);

	Engine_FillAddress_R_PolyBlend(DllInfo, RealDllInfo);

	gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))GamedataResolvePtr(RealDllInfo.ImageBase, "S_ExtraUpdate", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_GL_Bind(DllInfo, RealDllInfo);

	gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_SelectTexture", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_GL_LoadTexture2(DllInfo, RealDllInfo);

	Engine_FillAddress_R_CullBox(DllInfo, RealDllInfo);

	Engine_FillAddress_R_SetupFrame(DllInfo, RealDllInfo);

	Engine_FillAddress_R_SetupGL(DllInfo, RealDllInfo);

	Engine_FillAddress_R_RenderView(DllInfo, RealDllInfo);

	Engine_FillAddress_V_RenderView(DllInfo, RealDllInfo);

	Engine_FillAddress_R_RenderScene(DllInfo, RealDllInfo);

	Engine_FillAddress_R_NewMap(DllInfo, RealDllInfo);

	gPrivateFuncs.GL_LoadFilterTexture = (decltype(gPrivateFuncs.GL_LoadFilterTexture))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_LoadFilterTexture", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_BuildLightmaps", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_R_DrawSequentialPoly(DllInfo, RealDllInfo);

	gPrivateFuncs.R_TextureAnimation = (decltype(gPrivateFuncs.R_TextureAnimation))GamedataResolvePtr(RealDllInfo.ImageBase, "R_TextureAnimation", MH_GAMESYMBOL_KIND_FUNCTION);
	rtable = (decltype(rtable))GamedataResolvePtr(RealDllInfo.ImageBase, "rtable", MH_GAMESYMBOL_KIND_GLOBAL);

	gPrivateFuncs.R_DrawBrushModel = (decltype(gPrivateFuncs.R_DrawBrushModel))GamedataResolvePtr(RealDllInfo.ImageBase, "R_DrawBrushModel", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_R_RecursiveWorldNode(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawWorld(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawViewModel(DllInfo, RealDllInfo);

	Engine_FillAddress_R_MarkLeaves(DllInfo, RealDllInfo);

	gPrivateFuncs.GL_Set2D = (decltype(gPrivateFuncs.GL_Set2D))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_Set2D", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.GL_Finish2D = (decltype(gPrivateFuncs.GL_Finish2D))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_Finish2D", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.GL_BeginRendering = (decltype(gPrivateFuncs.GL_BeginRendering))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_BeginRendering", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.GL_EndRendering = (decltype(gPrivateFuncs.GL_EndRendering))GamedataResolvePtr(RealDllInfo.ImageBase, "GL_EndRendering", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_EmitWaterPolys(DllInfo, RealDllInfo);

	Engine_FillAddress_VID_UpdateWindowVars(DllInfo, RealDllInfo);

	gPrivateFuncs.Mod_PointInLeaf = (decltype(gPrivateFuncs.Mod_PointInLeaf))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_PointInLeaf", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))GamedataResolvePtr(RealDllInfo.ImageBase, "R_DrawTEntitiesOnList", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_BuildGammaTable(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawParticles(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_AllocDlight(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_AllocElight(DllInfo, RealDllInfo);

	gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))GamedataResolvePtr(RealDllInfo.ImageBase, "R_GLStudioDrawPoints", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_R_StudioLighting(DllInfo, RealDllInfo);

	Engine_FillAddress_R_LightLambert(DllInfo, RealDllInfo);

	Engine_FillAddress_R_StudioSetupSkin(DllInfo, RealDllInfo);

	gPrivateFuncs.Host_ClearMemory = (decltype(gPrivateFuncs.Host_ClearMemory))GamedataResolvePtr(RealDllInfo.ImageBase, "Host_ClearMemory", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_Cache_Alloc(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_MiptexTexture(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_DecalTexture(DllInfo, RealDllInfo);

	gPrivateFuncs.R_GetSpriteFrame = (decltype(gPrivateFuncs.R_GetSpriteFrame))GamedataResolvePtr(RealDllInfo.ImageBase, "R_GetSpriteFrame", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_R_DrawSpriteModel(DllInfo, RealDllInfo);

	Engine_FillAddress_R_LightStrength(DllInfo, RealDllInfo);

	Engine_FillAddress_R_RotateForEntity(DllInfo, RealDllInfo);

	Engine_FillAddress_GlowBlend(DllInfo, RealDllInfo);

	Engine_FillAddress_SCR_BeginLoadingPlaque(DllInfo, RealDllInfo);

	gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))GamedataResolvePtr(RealDllInfo.ImageBase, "Host_IsSinglePlayerGame", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_Mod_UnloadSpriteTextures(DllInfo, RealDllInfo);

	gPrivateFuncs.Mod_LoadSpriteModel = (decltype(gPrivateFuncs.Mod_LoadSpriteModel))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_LoadSpriteModel", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_Mod_LoadSpriteFrame(DllInfo, RealDllInfo);

	Engine_FillAddress_R_AddTEntity(DllInfo, RealDllInfo);

	Engine_FillAddress_Hunk_AllocName(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_EndRenderingVars(DllInfo, RealDllInfo);

	cl_numvisedicts = (decltype(cl_numvisedicts))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_numvisedicts", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_visedicts = (decltype(cl_visedicts))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_visedicts", MH_GAMESYMBOL_KIND_GLOBAL);

	Engine_FillAddress_R_AllocTransObjectsVars(DllInfo, RealDllInfo);

	Engine_FillAddress_R_RenderFinalFog(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawTEntitiesOnListVars(DllInfo, RealDllInfo);

	Engine_FillAddress_R_RecursiveWorldNodeVars(DllInfo, RealDllInfo);

	Engine_FillAddress_R_LoadSkybox(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_FilterMinMaxVars(DllInfo, RealDllInfo);

	Engine_FillAddress_ScrFov(DllInfo, RealDllInfo);

	//Got CL_IsDevOverviewMode, CL_SetDevOverView and refdef here
	Engine_FillAddress_RenderSceneVars(DllInfo, RealDllInfo);

	//Got ClientDLL_DrawNormalTriangles_VA, cl_waterlevel and gDevOverview here
	Engine_FillAddress_RenderSceneVars2(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_IsDevOverviewModeVars(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DecalInit(DllInfo, RealDllInfo);

	Engine_FillAddress_LightstyleVars(DllInfo, RealDllInfo);

	cl_simorg = (decltype(cl_simorg))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_simorg", MH_GAMESYMBOL_KIND_GLOBAL);

	cl_viewentity = (decltype(cl_viewentity))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_viewentity", MH_GAMESYMBOL_KIND_GLOBAL);

	cl_max_edicts = (decltype(cl_max_edicts))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_max_edicts", MH_GAMESYMBOL_KIND_GLOBAL);
	cl_entities = (decltype(cl_entities))GamedataResolvePtr(RealDllInfo.ImageBase, "cl_entities", MH_GAMESYMBOL_KIND_GLOBAL);

	gTempEnts = (decltype(gTempEnts))GamedataResolvePtr(RealDllInfo.ImageBase, "gTempEnts", MH_GAMESYMBOL_KIND_GLOBAL);

	Engine_FillAddress_WaterVars(DllInfo, RealDllInfo);

	mod_known = (decltype(mod_known))GamedataResolvePtr(RealDllInfo.ImageBase, "mod_known", MH_GAMESYMBOL_KIND_GLOBAL);
	mod_numknown = (decltype(mod_numknown))GamedataResolvePtr(RealDllInfo.ImageBase, "mod_numknown", MH_GAMESYMBOL_KIND_GLOBAL);

	gPrivateFuncs.Mod_LoadStudioModel = (decltype(gPrivateFuncs.Mod_LoadStudioModel))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_LoadStudioModel", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.Mod_LoadBrushModel = (decltype(gPrivateFuncs.Mod_LoadBrushModel))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_LoadBrushModel", MH_GAMESYMBOL_KIND_FUNCTION);

	gPrivateFuncs.Mod_LoadModel = (decltype(gPrivateFuncs.Mod_LoadModel))GamedataResolvePtr(RealDllInfo.ImageBase, "Mod_LoadModel", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_BasePalette(DllInfo, RealDllInfo);

	Engine_FillAddress_R_LightStrengthVars(DllInfo, RealDllInfo);

	filterMode = (decltype(filterMode))GamedataResolvePtr(RealDllInfo.ImageBase, "filterMode", MH_GAMESYMBOL_KIND_GLOBAL);
	filterColorRed = (decltype(filterColorRed))GamedataResolvePtr(RealDllInfo.ImageBase, "filterColorRed", MH_GAMESYMBOL_KIND_GLOBAL);
	filterColorGreen = (decltype(filterColorGreen))GamedataResolvePtr(RealDllInfo.ImageBase, "filterColorGreen", MH_GAMESYMBOL_KIND_GLOBAL);
	filterColorBlue = (decltype(filterColorBlue))GamedataResolvePtr(RealDllInfo.ImageBase, "filterColorBlue", MH_GAMESYMBOL_KIND_GLOBAL);
	filterBrightness = (decltype(filterBrightness))GamedataResolvePtr(RealDllInfo.ImageBase, "filterBrightness", MH_GAMESYMBOL_KIND_GLOBAL);

	Engine_FillAddress_MoveVars(DllInfo, RealDllInfo);

	Engine_FillAddress_MissingTexture(DllInfo, RealDllInfo);

	Engine_FillAddress_NoTexture(DllInfo, RealDllInfo);

	Engine_FillAddress_LegacyMultiTextureInit(DllInfo, RealDllInfo);

	gPrivateFuncs.PVSNode = (decltype(gPrivateFuncs.PVSNode))GamedataResolvePtr(RealDllInfo.ImageBase, "PVSNode", MH_GAMESYMBOL_KIND_FUNCTION);

	Engine_FillAddress_DrawStartupGraphic(DllInfo, RealDllInfo);

	Engine_FillAddress_DrawStartupVideo(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_Frame(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_SpriteFrameHoles(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_SpriteFrameAdditive(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_SpriteFrameGeneric(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_FillRGBA(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_FillRGBABlend(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_FillRGBABuf(DllInfo, RealDllInfo);

	Engine_FillAddress_D_FillRect(DllInfo, RealDllInfo);

	gPrivateFuncs.Draw_Pic = (decltype(gPrivateFuncs.Draw_Pic))GamedataResolvePtr(RealDllInfo.ImageBase, "Draw_Pic", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Engine_InstallHooks(void)
{
	Install_InlineHook(GL_Init); 

	if (gPrivateFuncs.GL_SetModeLegacy)
	{
		Install_InlineHook(GL_SetModeLegacy);
		Install_InlineHook(GL_SelectPixelFormat);
	}
	else if (gPrivateFuncs.GL_SetMode_GoldSrc)
	{
		Install_InlineHook(GL_SetMode_GoldSrc);
	}
	else
	{
		Install_InlineHook(GL_SetMode_SvEngine);
	}

	g_pMetaHookAPI->InlinePatchRedirectBranch(gPrivateFuncs.Sys_ShutdownGame_call_GL_Shutdown, GL_Shutdown, NULL);

	Install_InlineHook(GL_Bind);
	Install_InlineHook(GL_Set2D);
	Install_InlineHook(GL_Finish2D);
	Install_InlineHook(GL_BeginRendering);
	Install_InlineHook(GL_EndRendering);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Install_InlineHook(R_RenderView_SvEngine);
		Install_InlineHook(R_LoadSkyBox_SvEngine);
	}
	else
	{
		Install_InlineHook(R_RenderView);
		Install_InlineHook(R_LoadSkys);
	}

	//For Sven
	Install_InlineHook(R_ForceCVars);
	Install_InlineHook(R_NewMap);
	Install_InlineHook(Mod_PointInLeaf);
	Install_InlineHook(R_GLStudioDrawPoints);
	Install_InlineHook(GL_UnloadTextures);
	Install_InlineHook(GL_LoadFilterTexture);
	Install_InlineHook(GL_LoadTexture2);
	Install_InlineHook(GL_BuildLightmaps);

	Install_InlineHook(LegacyMultiTextureInit);

	Install_InlineHook(Mod_LoadStudioModel);
	Install_InlineHook(Mod_LoadSpriteModel);
	Install_InlineHook(Mod_UnloadSpriteTextures);

	gEngfuncs.pTriAPI->RenderMode = triapi_RenderMode;
	gEngfuncs.pTriAPI->Begin = triapi_Begin;
	gEngfuncs.pTriAPI->End = triapi_End;
	gEngfuncs.pTriAPI->Color4f = triapi_Color4f;
	gEngfuncs.pTriAPI->Color4ub = triapi_Color4ub;
	gEngfuncs.pTriAPI->TexCoord2f = triapi_TexCoord2f;
	gEngfuncs.pTriAPI->Vertex3fv = triapi_Vertex3fv;
	gEngfuncs.pTriAPI->Vertex3f = triapi_Vertex3f;
	gEngfuncs.pTriAPI->Brightness = triapi_Brightness;
	gEngfuncs.pTriAPI->Color4fRendermode = triapi_Color4fRendermode;

	gEngfuncs.pTriAPI->GetMatrix = triapi_GetMatrix;
	gEngfuncs.pTriAPI->BoxInPVS = triapi_BoxInPVS;
	gEngfuncs.pTriAPI->Fog = triapi_Fog;
	gEngfuncs.pTriAPI->FogParams = triapi_FogParams;
	gEngfuncs.pTriAPI->SpriteTexture = triapi_SpriteTexture;

	Install_InlineHook(BuildGammaTable);
	Install_InlineHook(R_CullBox);
	Install_InlineHook(PVSNode);
	Install_InlineHook(Host_ClearMemory);
	Install_InlineHook(CVideoMode_Common_DrawStartupGraphic);
	Install_InlineHook(CGame_DrawStartupVideo);
	Install_InlineHook(Draw_Frame);
	Install_InlineHook(Draw_SpriteFrameHoles);
	Install_InlineHook(Draw_SpriteFrameHoles_SvEngine);
	Install_InlineHook(Draw_SpriteFrameAdditive);
	Install_InlineHook(Draw_SpriteFrameAdditive_SvEngine);
	Install_InlineHook(Draw_SpriteFrameGeneric);
	Install_InlineHook(Draw_SpriteFrameGeneric_SvEngine);
	Install_InlineHook(Draw_FillRGBA);
	Install_InlineHook(Draw_FillRGBABlend);
	Install_InlineHook(Draw_FillRGBABuf);
	Install_InlineHook(Draw_Pic);
	Install_InlineHook(D_FillRect);
	Install_InlineHook(R_GetSpriteFrame);
}

void Engine_UninstallHooks(void)
{
	//Engine
	Uninstall_Hook(GL_Init);
	if (gPrivateFuncs.GL_SetModeLegacy)
	{
		Uninstall_Hook(GL_SetModeLegacy);
		Uninstall_Hook(GL_SelectPixelFormat);
	}
	else if (gPrivateFuncs.GL_SetMode_GoldSrc)
	{
		Uninstall_Hook(GL_SetMode_GoldSrc);
	}
	else
	{
		Uninstall_Hook(GL_SetMode_SvEngine);
	}
	Uninstall_Hook(GL_Bind);
	Uninstall_Hook(GL_Set2D);
	Uninstall_Hook(GL_Finish2D);
	Uninstall_Hook(GL_BeginRendering);
	Uninstall_Hook(GL_EndRendering);

	if (gPrivateFuncs.R_RenderView_SvEngine)
	{
		Uninstall_Hook(R_RenderView_SvEngine);
		Uninstall_Hook(R_LoadSkyBox_SvEngine);
	}
	else
	{
		Uninstall_Hook(R_RenderView);
		Uninstall_Hook(R_LoadSkys);
	}

	Uninstall_Hook(R_ForceCVars);
	Uninstall_Hook(R_NewMap);
	Uninstall_Hook(Mod_PointInLeaf);
	Uninstall_Hook(R_GLStudioDrawPoints);
	Uninstall_Hook(GL_UnloadTextures);
	Uninstall_Hook(GL_LoadFilterTexture);
	Uninstall_Hook(GL_LoadTexture2);
	Uninstall_Hook(GL_BuildLightmaps);
	Uninstall_Hook(LegacyMultiTextureInit);

	Uninstall_Hook(Mod_LoadStudioModel);
	Uninstall_Hook(Mod_LoadSpriteModel);
	Uninstall_Hook(Mod_UnloadSpriteTextures);
	//Uninstall_Hook(Draw_MiptexTexture);
	Uninstall_Hook(BuildGammaTable);
	Uninstall_Hook(R_CullBox);
	Uninstall_Hook(PVSNode);
	Uninstall_Hook(CVideoMode_Common_DrawStartupGraphic);
	Uninstall_Hook(CGame_DrawStartupVideo);
	Uninstall_Hook(Draw_Frame);
	Uninstall_Hook(Draw_SpriteFrameHoles);
	Uninstall_Hook(Draw_SpriteFrameHoles_SvEngine);
	Uninstall_Hook(Draw_SpriteFrameAdditive);
	Uninstall_Hook(Draw_SpriteFrameAdditive_SvEngine);
	Uninstall_Hook(Draw_SpriteFrameGeneric);
	Uninstall_Hook(Draw_SpriteFrameGeneric_SvEngine);
	Uninstall_Hook(Draw_FillRGBA);
	Uninstall_Hook(Draw_FillRGBABlend);
	Uninstall_Hook(Draw_FillRGBABuf);
	Uninstall_Hook(Draw_Pic);
	Uninstall_Hook(D_FillRect);
	Uninstall_Hook(R_GetSpriteFrame);
}

int WINAPI GL_RedirectedGenTexture(void)
{
	return GL_GenTexture();
}

/*
	Purpose: Redirect all "mov eax, allocated_textures" to "call GL_RedirectedGenTexture" for legacy engine
*/

void R_RedirectEngineLegacyOpenGLTextureAllocation(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_bHasOfficialGLTexAllocSupport)
		return;

	auto allocated_textures_VA = ConvertDllInfoSpace(allocated_textures, RealDllInfo, DllInfo);

	const char pattern[] = "\xA1\x2A\x2A\x2A\x2A";
	*(ULONG_PTR*)(pattern + 1) = (ULONG_PTR)allocated_textures_VA;

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			typedef struct RedirectBlobEngineOpenGLTexture_SearchContext_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;
				bool bFoundWriteBack{};
				bool bFoundGL_Bind{};
			}RedirectBlobEngineOpenGLTexture_SearchContext;

			RedirectBlobEngineOpenGLTexture_SearchContext ctx = { DllInfo, RealDllInfo };

			g_pMetaHookAPI->DisasmRanges(pFound, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (RedirectBlobEngineOpenGLTexture_SearchContext*)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&

					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.ImageBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.ImageBase + ctx->DllInfo.ImageSize &&

					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].reg == X86_REG_EAX)
				{
					auto ConvertedImm = ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);

					if (ConvertedImm == allocated_textures)
					{
						ctx->bFoundWriteBack = true;
					}
				}

				if (address[0] == 0xE8)
				{
					auto ConvertedImm = ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);

					if (ConvertedImm == gPrivateFuncs.GL_Bind)
					{
						ctx->bFoundGL_Bind = true;
					}
				}

				if (ctx->bFoundWriteBack || ctx->bFoundGL_Bind)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

				}, 0, &ctx);

			if (ctx.bFoundWriteBack || ctx.bFoundGL_Bind)
			{
				auto pFound_RealDllBase = (PUCHAR)ConvertDllInfoSpace(pFound, ctx.DllInfo, ctx.RealDllInfo);

				char redirectCode[] = "\xE8\x2A\x2A\x2A\x2A";
				*(int*)(redirectCode + 1) = (PUCHAR)GL_RedirectedGenTexture - (pFound_RealDllBase + 5);
				g_pMetaHookAPI->WriteMemory(pFound_RealDllBase, redirectCode, sizeof(redirectCode) - 1);
			}

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectEngineLegacyOpenGLCallAPI(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glGetString", CoreProfile_glGetString, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glAlphaFunc", CoreProfile_glAlphaFunc, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glEnable", CoreProfile_glEnable, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glDisable", CoreProfile_glDisable, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glIsEnabled", CoreProfile_glIsEnabled, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glShadeModel", CoreProfile_glShadeModel, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glTexEnvf", CoreProfile_glTexEnvf, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glTexParameterf", CoreProfile_glTexParameterf, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glBegin", CoreProfile_glBegin, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glColor4f", CoreProfile_glColor4f, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "opengl32.dll", "glColor4ub", CoreProfile_glColor4ub, NULL);

		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_GL_ExtensionSupported", CoreProfile_SDL_GL_ExtensionSupported, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_GL_SetAttribute", CoreProfile_GL_SetAttribute, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_CreateWindow", CoreProfile_SDL_CreateWindow, NULL);
	}
	else if(gPrivateFuncs.SDL_GL_GetProcAddress)
	{ 
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_GL_GetProcAddress", CoreProfile_SDL_GL_GetProcAddress, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_GL_SetAttribute", CoreProfile_GL_SetAttribute, NULL);
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "SDL2.dll", "SDL_CreateWindow", CoreProfile_SDL_CreateWindow, NULL);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		g_pMetaHookAPI->BlobIATHook(g_pMetaHookAPI->GetBlobEngineModule(), "kernel32.dll", "GetProcAddress", CoreProfile_GetProcAddress, NULL);
	}
	else
	{
		//non-SDL
		g_pMetaHookAPI->IATHook(g_pMetaHookAPI->GetEngineModule(), "kernel32.dll", "GetProcAddress", CoreProfile_GetProcAddress, NULL);

	}

	if (gPrivateFuncs.GL_SetMode_call_qwglCreateContext)
	{
		g_pMetaHookAPI->InlinePatchRedirectBranch(gPrivateFuncs.GL_SetMode_call_qwglCreateContext, CoreProfile_qwglCreateContext, NULL);
	}
}

void R_RedirectEngineLegacyOpenGLCall(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	R_RedirectEngineLegacyOpenGLTextureAllocation(DllInfo, RealDllInfo);
	R_RedirectEngineLegacyOpenGLCallAPI(DllInfo, RealDllInfo);
}

void R_RedirectSCClientLegacyOpenGLCall_glTexEnvf(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\x68\x01\x85\x00\x00\x68\x00\x85\x00\x00\xFF\x15";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 10, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glTexEnvf, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glBegin(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\x6A\x06\xFF\x15\x2A\x2A\x2A\x2A\xF6\x87\x88\x00\x00\x00\x80";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 2, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, triapi_glBegin, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glEnd(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xFF\x15\x2A\x2A\x2A\x2A\xF7\x87\x88\x00\x00\x00\x00\x01\x00\x00";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 0, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, triapi_glEnd, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glColor4f_DrawParticle(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xF3\x0F\x11\x0C\x24\xFF\x15\x2A\x2A\x2A\x2A\x8D\x45\x98";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 5, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glColor4f, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glColor4f_DrawPortal(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xC7\x44\x24\x08\x00\x00\x80\x3F\xC7\x44\x24\x04\x00\x00\x80\x3F\xC7\x04\x24\x00\x00\x80\x3F\xFF\x15\x2A\x2A\x2A\x2A\x68";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound +
				Sig_Length("\xC7\x44\x24\x08\x00\x00\x80\x3F\xC7\x44\x24\x04\x00\x00\x80\x3F\xC7\x04\x24\x00\x00\x80\x3F")
				, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glColor4f, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glEnable_GenerateInvisibleTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xFF\x75\x00\x68\xE1\x0D\x00\x00\xFF\x15";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 5, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glEnable, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glEnable_GeneratePortalTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\x6A\x01\xFF\x15\x2A\x2A\x2A\x2A\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xFF\x36\x68";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 13, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glEnable, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glDisable_FOG(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	// glDisable(GL_FOG);
	const char pattern[] = "\x68\x60\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xA1";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + 5, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glDisable, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

static void* g_glDisable_ClipPlane = nullptr;

void R_RedirectSCClientLegacyOpenGLCall_glDisable_ClipPlane(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	g_glDisable_ClipPlane = CoreProfile_glDisable;

	// glDisable(GL_CLIPPLANE0);
	const char pattern[] = "\x8B\x35\x2A\x2A\x2A\x2A\x33\xD2\xC6\x05";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PUCHAR pRealCall = (PUCHAR)ConvertDllInfoSpace(pFound, DllInfo, RealDllInfo);

			g_pMetaHookAPI->WriteDWORD(pRealCall + 2, (ULONG_PTR)&g_glDisable_ClipPlane);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glCopyTexSubImage2D_RenderPortals(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	// glDisable(GL_FOG);
	const char pattern[] = "\x50\x6A\x00\x6A\x00\x6A\x00\x6A\x00\x68\xE1\x0D\x00\x00\xFF\x15";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + Sig_Length(pattern) - 2, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glCopyTexSubImage2D_RenderPortals, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientLegacyOpenGLCall_glClear_ClipPlane(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xC7\x44\x24\x0C\x00\x00\x00\x00\xC7\x44\x24\x08\x00\x00\x80\x3F\xC7\x44\x24\x04\x00\x00\x00\x00\xC7\x04\x24\x00\x00\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\x00\x40\x00\x00\xFF\x15";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + Sig_Length(pattern) - 2, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, CoreProfile_glClear_RenderPortals, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectSCClientRenderPortalAngleVectors(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\xC7\x82\xE8\x00\x00\x00\x01\x00\x00\x00\x50\x8D\x42\x18\x50\x51\xE8";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			PVOID pRealCall = ConvertDllInfoSpace(pFound + Sig_Length(pattern) - 1, DllInfo, RealDllInfo);

			g_pMetaHookAPI->InlinePatchRedirectBranch(pRealCall, ClientPortalManager_AngleVectors, nullptr);

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}

void R_RedirectClientLegacyOpenGLCall(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_bIsSvenCoop)
	{
		//Sniber NMSL
		R_RedirectSCClientLegacyOpenGLCall_glTexEnvf(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glBegin(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glEnd(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glColor4f_DrawParticle(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glColor4f_DrawPortal(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glEnable_GenerateInvisibleTexture(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glEnable_GeneratePortalTexture(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glDisable_FOG(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glDisable_ClipPlane(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glCopyTexSubImage2D_RenderPortals(DllInfo, RealDllInfo);
		R_RedirectSCClientLegacyOpenGLCall_glClear_ClipPlane(DllInfo, RealDllInfo);
		R_RedirectSCClientRenderPortalAngleVectors(DllInfo, RealDllInfo);
	}
}

void R_PatchResetLatched(const mh_dll_info_t &DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		return;

	//CL_LinkPacketEntities calls R_ResetLatched at two consecutive call sites; the
	//interpolation fix must cover both, so every numbered call-site record is
	//redirected and the resolved function itself feeds R_ResetLatched_Patched.
	char symbolName[96];

	for (int index = 0; ; ++index)
	{
		snprintf(symbolName, sizeof(symbolName), "CL_LinkPacketEntities_to_R_ResetLatched_callsite_%d", index);

		PVOID callsite = GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, symbolName, MH_GAMESYMBOL_KIND_PATCH);

		if (!callsite)
			break;

		g_pMetaHookAPI->InlinePatchRedirectBranch(callsite, R_ResetLatched_Patched, NULL);
	}

	gPrivateFuncs.R_ResetLatched = (decltype(gPrivateFuncs.R_ResetLatched))GamedataResolvePtr(RealDllInfo.ImageBase, "R_ResetLatched", MH_GAMESYMBOL_KIND_FUNCTION);
}

void Client_FillAddress_ClientPortalManager_ResetAll(const mh_dll_info_t &DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.ClientPortalManager_ResetAll)
		return;

	gPrivateFuncs.ClientPortalManager_ResetAll = (decltype(gPrivateFuncs.ClientPortalManager_ResetAll))GamedataResolvePtr(RealDllInfo.ImageBase, "ClientPortalManager_ResetAll", MH_GAMESYMBOL_KIND_FUNCTION);

}

void Client_FillAddress_ClientPortalManager_GetOriginalSurfaceTexture_DrawPortalSurface(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		.text:1004EED7 6A 01                                               push    1               ; alpha
		.text:1004EED9 6A 01                                               push    1               ; blue
		.text:1004EEDB 6A 01                                               push    1               ; green
		.text:1004EEDD 6A 01                                               push    1               ; red
		.text:1004EEDF FF 15 14 92 11 10                                   call    ds:glColorMask
		.text:1004EEE5 68 E1 0D 00 00                                      push    0DE1h           ; cap
		.text:1004EEEA FF 15 08 92 11 10                                   call    ds:glEnable
		.text:1004EEF0
		.text:1004EEF0                                     loc_1004EEF0:                           ; CODE XREF: sub_1004EA40+173↑j
		.text:1004EEF0 8B 4C 24 20                                         mov     ecx, [esp+30h+var_10]
		.text:1004EEF4 56                                                  push    esi
		.text:1004EEF5 E8 66 EB FF FF                                      call    ClientPortalManager__GetOriginalSurfaceTexture
	*/

	const char pattern[] = "\x6A\x01\x6A\x01\x6A\x01\x6A\x01\xFF\x15\x2A\x2A\x2A\x2A\x68\xE1\x0D\x00\x00";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = SearchBegin + DllInfo.TextSize;

	typedef struct ClientPortalManager_GetOriginalSurfaceTexture_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	}ClientPortalManager_GetOriginalSurfaceTexture_SearchContext;

	ClientPortalManager_GetOriginalSurfaceTexture_SearchContext ctx = { DllInfo, RealDllInfo };

	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			g_pMetaHookAPI->DisasmRanges(pFound + 4, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (ClientPortalManager_GetOriginalSurfaceTexture_SearchContext*)context;

				if (address[0] == 0xE8)
				{
					gPrivateFuncs.ClientPortalManager_GetOriginalSurfaceTexture = (decltype(gPrivateFuncs.ClientPortalManager_GetOriginalSurfaceTexture))
						ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);

					return TRUE;
				}

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

			}, 0, &ctx);

			if (gPrivateFuncs.ClientPortalManager_GetOriginalSurfaceTexture)
			{
				PVOID ClientPortalManager_DrawPortalSurface_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x600, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC)
						return TRUE;

					return FALSE;
				});

				if (ClientPortalManager_DrawPortalSurface_VA)
				{
					gPrivateFuncs.ClientPortalManager_DrawPortalSurface = (decltype(gPrivateFuncs.ClientPortalManager_DrawPortalSurface))
						ConvertDllInfoSpace(ClientPortalManager_DrawPortalSurface_VA, DllInfo, RealDllInfo);
				}

				break;
			}

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}

	Sig_FuncNotFound(ClientPortalManager_GetOriginalSurfaceTexture);
	Sig_FuncNotFound(ClientPortalManager_DrawPortalSurface);
}

void Client_FillAddress_ClientPortalManager_EnableClipPlane(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.ClientPortalManager_EnableClipPlane)
		return;

	gPrivateFuncs.ClientPortalManager_EnableClipPlane = (decltype(gPrivateFuncs.ClientPortalManager_EnableClipPlane))GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "ClientPortalManager_EnableClipPlane", MH_GAMESYMBOL_KIND_FUNCTION);

}

void Client_FillAddress_ClientPortalManager_RenderPoratals(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.ClientPortalManager_RenderPortals)
		return;

	gPrivateFuncs.ClientPortalManager_RenderPortals = (decltype(gPrivateFuncs.ClientPortalManager_RenderPortals))GamedataResolvePtr(RealDllInfo.ImageBase, "ClientPortalManager_RenderPortals", MH_GAMESYMBOL_KIND_FUNCTION);

}

void Client_FillAddress_UpdatePlayerPitch(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.UpdatePlayerPitch)
		return;

	gPrivateFuncs.UpdatePlayerPitch = (decltype(gPrivateFuncs.UpdatePlayerPitch))GamedataResolvePtr(RealDllInfo.ImageBase, "UpdatePlayerPitch", MH_GAMESYMBOL_KIND_FUNCTION);

}

void Client_FillAddress_WaterLevel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
.text:10072350                                     V_CalcRefdef    proc near               ; DATA XREF: .rdata:off_10170E88↓o
.text:10072350
.text:10072350                                     var_10          = dword ptr -10h
.text:10072350                                     var_8           = dword ptr -8
.text:10072350                                     var_4           = dword ptr -4
.text:10072350                                     a1              = dword ptr  4
.text:10072350
.text:10072350 83 EC 08                                            sub     esp, 8
.text:10072353 56                                                  push    esi
.text:10072354 8B 74 24 10                                         mov     esi, [esp+0Ch+a1]
.text:10072358 8B 46 54                                            mov     eax, [esi+54h]
.text:1007235B A3 74 35 60 10                                      mov     g_iWaterLevel, eax
.text:10072360 83 BE E0 00 00 00 00                                cmp     dword ptr [esi+0E0h], 0
.text:10072367 0F 85 A7 01 00 00                                   jnz     loc_10072514
.text:1007236D 80 3D 0D C8 63 10 00                                cmp     g_bRenderingPortals, 0
.text:10072374 0F 85 9A 01 00 00                                   jnz     loc_10072514
.text:1007237A 83 7E 44 00                                         cmp     dword ptr [esi+44h], 0
.text:1007237E C7 86 E4 00 00 00 00 00 00 00                       mov     dword ptr [esi+0E4h], 0
.text:10072388 0F 84 77 01 00 00                                   jz      loc_10072505
.text:1007238E 83 BE AC 00 00 00 01                                cmp     dword ptr [esi+0ACh], 1
				*/
	const char pattern[] = "\xA3\x2A\x2A\x2A\x2A\x83\x2A\xE0\x00\x00\x00\x00\x0F\x85\x2A\x2A\x2A\x2A\x80\x3D\x2A\x2A\x2A\x2A\x00";
	auto addr = (PUCHAR)Search_Pattern(pattern, DllInfo);
	Sig_AddrNotFound(g_iWaterLevel);

	auto g_iWaterLevel_VA = *(PVOID*)(addr + 1);

	g_iWaterLevel = (decltype(g_iWaterLevel))ConvertDllInfoSpace(g_iWaterLevel_VA, DllInfo, RealDllInfo);

	Sig_VarNotFound(g_iWaterLevel);
}

void Client_FillAddress_FogParams(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char pattern[] = "\x68\x01\x26\x00\x00\x68\x65\x0B\x00\x00";

	PVOID addr = Search_Pattern(pattern, DllInfo);

	Sig_AddrNotFound(g_iFogColor);

	typedef struct V_CalcNormalRefdef_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		ULONG_PTR Candidates[16]{};
		int iNumCandidates{};
	}V_CalcNormalRefdef_SearchContext;

	V_CalcNormalRefdef_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(addr, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto ctx = (V_CalcNormalRefdef_SearchContext*)context;
		auto pinst = (cs_insn*)inst;

		if (ctx->iNumCandidates < 16)
		{
			if (pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_XMM0 &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.ImageBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.ImageBase + ctx->DllInfo.ImageSize)
			{
				ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
				ctx->iNumCandidates++;
			}
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, &ctx);

	if (ctx.iNumCandidates >= 5 &&
		ctx.Candidates[ctx.iNumCandidates - 1] == ctx.Candidates[ctx.iNumCandidates - 2] + sizeof(int) &&
		ctx.Candidates[ctx.iNumCandidates - 2] == ctx.Candidates[ctx.iNumCandidates - 3] + sizeof(int) &&
		ctx.Candidates[ctx.iNumCandidates - 3] == ctx.Candidates[ctx.iNumCandidates - 4] + sizeof(int))
	{
		g_iFogColor_SCClient = (decltype(g_iFogColor_SCClient))ConvertDllInfoSpace((PVOID)ctx.Candidates[0], DllInfo, RealDllInfo);
		g_iStartDist_SCClient = (decltype(g_iStartDist_SCClient))ConvertDllInfoSpace((PVOID)ctx.Candidates[3], DllInfo, RealDllInfo);
		g_iEndDist_SCClient = (decltype(g_iEndDist_SCClient))ConvertDllInfoSpace((PVOID)ctx.Candidates[4], DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(g_iFogColor_SCClient);
	Sig_VarNotFound(g_iStartDist_SCClient);
	Sig_VarNotFound(g_iEndDist_SCClient);
}

void Client_FillAddress_SCClient(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory)
	{
		auto SCClient001 = pfnClientFactory("SCClientDLL001", 0);

		if (SCClient001)
		{
			gPrivateFuncs.SCClientDLL_glewInit = (decltype(gPrivateFuncs.SCClientDLL_glewInit))GetProcAddress(g_pMetaHookAPI->GetClientModule(), "_glewInit@0");

			Client_FillAddress_ClientPortalManager_ResetAll(DllInfo, RealDllInfo);
			Client_FillAddress_ClientPortalManager_GetOriginalSurfaceTexture_DrawPortalSurface(DllInfo, RealDllInfo);
			Client_FillAddress_ClientPortalManager_EnableClipPlane(DllInfo, RealDllInfo);
			Client_FillAddress_ClientPortalManager_RenderPoratals(DllInfo, RealDllInfo);
			Client_FillAddress_UpdatePlayerPitch(DllInfo, RealDllInfo);
			Client_FillAddress_WaterLevel(DllInfo, RealDllInfo);
			Client_FillAddress_FogParams(DllInfo, RealDllInfo);

			g_bRenderingPortals_SCClient = (decltype(g_bRenderingPortals_SCClient))GamedataResolvePtr(RealDllInfo.ImageBase, "g_bRenderingPortals_SCClient", MH_GAMESYMBOL_KIND_GLOBAL);
			//Only svencoop-10257 publishes this slot; 8948 keeps it null and the
			//studio view-entity save/restore is skipped there.
			g_ViewEntityIndex_SCClient = (decltype(g_ViewEntityIndex_SCClient))GamedataResolvePtrIfAvailable(RealDllInfo.ImageBase, "g_ViewEntityIndex_SCClient", MH_GAMESYMBOL_KIND_GLOBAL);

			g_bIsSvenCoop = true;
		}
	}
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{

	Client_FillAddress_SCClient(DllInfo, RealDllInfo);

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
			g_PlayerExtraInfo_CZDS = (decltype(g_PlayerExtraInfo_CZDS))GamedataResolvePtr(RealDllInfo.ImageBase, "g_PlayerExtraInfo_CZDS", MH_GAMESYMBOL_KIND_GLOBAL);
		else
			g_PlayerExtraInfo = (decltype(g_PlayerExtraInfo))GamedataResolvePtr(RealDllInfo.ImageBase, "g_PlayerExtraInfo", MH_GAMESYMBOL_KIND_GLOBAL);
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "aomdc"))
	{
		g_bIsAoMDC = true;
	}

}

void Client_InstallHooks()
{
	//Install_InlineHook(ClientPortalManager_ResetAll);
	Install_InlineHook(ClientPortalManager_DrawPortalSurface);
	Install_InlineHook(ClientPortalManager_EnableClipPlane);
	Install_InlineHook(ClientPortalManager_RenderPortals);
	Install_InlineHook(UpdatePlayerPitch);

	//Fuck Sniber
	if(gPrivateFuncs.SCClientDLL_glewInit)
		gPrivateFuncs.SCClientDLL_glewInit();
}

void Client_UninstallHooks()
{
	//Uninstall_Hook(ClientPortalManager_ResetAll);
	Uninstall_Hook(ClientPortalManager_DrawPortalSurface);
	Uninstall_Hook(ClientPortalManager_EnableClipPlane);
	Uninstall_Hook(ClientPortalManager_RenderPortals);
	Uninstall_Hook(UpdatePlayerPitch);
}

PVOID ConvertDllInfoSpace(PVOID addr, const mh_dll_info_t& SrcDllInfo, const mh_dll_info_t& TargetDllInfo)
{
	if ((ULONG_PTR)addr > (ULONG_PTR)SrcDllInfo.ImageBase && (ULONG_PTR)addr < (ULONG_PTR)SrcDllInfo.ImageBase + SrcDllInfo.ImageSize)
	{
		auto addr_VA = (ULONG_PTR)addr;
		auto addr_RVA = RVA_from_VA(addr, SrcDllInfo);

		return (PVOID)VA_from_RVA(addr, TargetDllInfo);
	}

	return nullptr;
}

PVOID GetVFunctionFromVFTable(PVOID* vftable, int index, const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo, const mh_dll_info_t& OutputDllInfo)
{
	if ((ULONG_PTR)vftable > (ULONG_PTR)RealDllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)RealDllInfo.ImageBase + RealDllInfo.ImageSize)
	{
		ULONG_PTR vftable_VA = (ULONG_PTR)vftable;
		ULONG vftable_RVA = RVA_from_VA(vftable, RealDllInfo);
		auto vftable_DllInfo = (decltype(vftable))VA_from_RVA(vftable, DllInfo);

		auto vf_VA = (ULONG_PTR)vftable_DllInfo[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}
	else if ((ULONG_PTR)vftable > (ULONG_PTR)DllInfo.ImageBase && (ULONG_PTR)vftable < (ULONG_PTR)DllInfo.ImageBase + DllInfo.ImageSize)
	{
		auto vf_VA = (ULONG_PTR)vftable[index];
		ULONG vf_RVA = RVA_from_VA(vf, DllInfo);

		return (PVOID)VA_from_RVA(vf, OutputDllInfo);
	}

	return vftable[index];
}
