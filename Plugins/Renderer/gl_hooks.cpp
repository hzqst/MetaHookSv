
#include <metahook.h>
#include <capstone.h>
#include <set>
#include "gl_local.h"

typedef struct walk_context_s
{
	walk_context_s(PVOID a, size_t l, int d) : address(a), len(l), depth(d)
	{

	}
	PVOID address;
	size_t len;
	int depth;
}walk_context_t;

#define R_MARKLIGHTS_SVENGINE "\x83\xEC\x0C\x2A\x8B\x7C\x24\x2A\x83\x3F\x00"
#define R_MARKLIGHTS_NEW "\x55\x8B\xEC\x83\xEC\x18\x2A\x8B\x2A\x10\x83\x2A\x00"

#define R_POLYBLEND_SVENGINE "\x55\x8B\xEC\x83\xE4\xC0\x83\xEC\x34\x2A\x2A\x2A\xE8"
#define R_POLYBLEND_NEW "\x55\x8B\xEC\x83\xEC\x0C\x2A\xE8\x2A\x2A\x2A\x2A\x8B\x2A\x85\x2A\x0F\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\xC0\x0B\x00\x00"

#define S_EXTRAUPDATE_SVENGINE "\xE8\x2A\x2A\x2A\x2A\x85\xC0\x75\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xD9\x05"
#define S_EXTRAUPDATE_NEW S_EXTRAUPDATE_SVENGINE

#define R_ROTATEFORENTITY_SVENGINE "\x83\xEC\x2A\x8B\x2A\x24\x2A\x8B\x2A\x24\x2A\xD9\x00"
#define R_ROTATEFORENTITY_NEW "\x55\x8B\xEC\x83\xEC\x20\x8B\x45\x08\x8B\x08\x8B\x50\x04\x8B\x40\x08"

#define R_DECALSHOTINTERNAL_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\x8B\x54\x24\x2A\x8B\x4C\x24\x2A\x53\x8B\x5C\x24\x2A\x56\x69\xF2\xB8\x0B\x00\x00"

#define R_RENDERSCENE_SIG "\x51\xDB\x05\x2A\x2A\x2A\x2A\xD9\x5C\x24\x00\xD9\x05\x2A\x2A\x2A\x2A\xD8\x5C\x24\x00\xDF\xE0\xF6\xC4\x2A\x2A\x2A\xA1"
#define R_RENDERSCENE_SIG_NEW "\xE8\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\xE8\x2A\x2A\x2A\x2A\xE8"
#define R_RENDERSCENE_SIG_SVENGINE "\xDD\xD8\xDD\xD8\xE8"

#define R_NEWMAP_SIG "\x55\x8B\xEC\x83\xEC\x0C\xC7\x45\xFC\x00\x00\x00\x00\x2A\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC"
#define R_NEWMAP_SIG_NEW "\x55\x8B\xEC\x83\xEC\x08\xC7\x45\xFC\x00\x00\x00\x00\x2A\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC\x81\x7D\xFC\x00\x01\x00\x00\x2A\x2A\x8B\x4D\xFC"
#define R_NEWMAP_SIG_SVENGINE "\x55\x8B\xEC\x51\xC7\x45\xFC\x00\x00\x00\x00\xEB\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC\x81\x7D\xFC\x00\x01\x00\x00"

#define R_DRAWWORLD_SIG "\x81\xEC\xB8\x0B\x00\x00\x68\xB8\x0B\x00\x00\x8D\x44\x24\x04\x6A\x00\x50\xE8"
#define R_DRAWWORLD_SIG_NEW "\x55\x8B\xEC\x81\xEC\xB8\x0B\x00\x00\x68\xB8\x0B\x00\x00\x8D\x85\x48\xF4\xFF\xFF\x6A\x00\x50\xE8\x2A\x2A\x2A\x2A\x8B\x0D"
#define R_DRAWWORLD_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\xB8\x0B\x00\x00\xD9\x05"

#define R_MARKLEAVES_SIG "\xB8\x00\x10\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\xA1"
#define R_MARKLEAVES_SIG_NEW "\x55\x8B\xEC\xB8\x04\x10\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\xA1"
#define R_MARKLEAVES_SIG_SVENGINE "\xB8\x2A\x2A\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\x8B\x0D\x2A\x2A\x2A\x2A\xD9\x05"

#define R_CULLBOX_SIG "\x53\x8B\x5C\x24\x08\x56\x57\x8B\x7C\x24\x14\xBE\x2A\x2A\x2A\x2A\x56\x57\x53\xE8"
#define R_CULLBOX_SIG_NEW "\x55\x8B\xEC\x53\x8B\x5D\x08\x56\x57\x8B\x7D\x0C\xBE\x2A\x2A\x2A\x2A\x56\x57\x53"
#define R_CULLBOX_SIG_SVENGINE "\x2A\x8B\x2A\x24\x08\x2A\x2A\x8B\x2A\x24\x14\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C\x83\xF8\x02"

#define R_SETUPFRAME_SIG "\xA1\x2A\x2A\x2A\x2A\x83\xEC\x18\x83\xF8\x01\x0F\x8E\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x2A\x2A\x68"
#define R_SETUPFRAME_SIG2 "\x8B\x0D\x2A\x2A\x2A\x2A\x83\xEC\x18\x33\xC0\x83\xF9\x01\x0F\x9F\xC0\x50\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1"
#define R_SETUPFRAME_SIG_NEW "\x55\x8B\xEC\x83\xEC\x18\x8B\x0D\x2A\x2A\x2A\x2A\x33\xC0\x83\xF9\x01\x0F\x9F\xC0\x50\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1"

#define GL_BIND_SIG "\x8B\x44\x24\x04\x8B\x0D\x2A\x2A\x2A\x2A\x56\x8B\xF0\xC1\xFE\x10\x25\xFF\xFF\x00\x00\x4E\x3B\xC8"
#define GL_BIND_SIG_NEW "\x55\x8B\xEC\x8B\x45\x08\x8B\x0D\x2A\x2A\x2A\x2A\x56\x8B\xF0\xC1\xFE\x10\x25\xFF\xFF\x00\x00\x4E\x3B\xC8"
#define GL_BIND_SIG_SVENGINE "\x8B\x44\x24\x04\x39\x05\x2A\x2A\x2A\x2A\x2A\x2A\x50\x68\xE1\x0D\x00\x00\xA3\x2A\x2A\x2A\x2A\xFF\x15\x2A\x2A\x2A\x2A\xC3"

#define GL_SELECTTEXTURE_SIG "\xA1\x2A\x2A\x2A\x2A\x56\x85\xC0\x2A\x2A\x8B\x74\x24\x08\x56\xFF\x15\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x3B\xF0"
#define GL_SELECTTEXTURE_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x56\x85\xC0\x2A\x2A\x8B\x75\x08\x56\xFF\x15\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x3B\xF0"
#define GL_SELECTTEXTURE_SIG_SVENGINE "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x2A\x2A\x2A\x24\x08\x2A\xFF\x15\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xA1"

#define GL_LOADTEXTURE_SIG "\xA1\x2A\x2A\x2A\x2A\x8B\x4C\x24\x20\x8B\x54\x24\x1C\x50\x8B\x44\x24\x1C\x51\x8B\x4C\x24\x1C\x52\x8B\x54\x24\x1C\x50\x8B\x44\x24\x1C"
#define GL_LOADTEXTURE_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x8B\x4D\x24\x8B\x55\x20\x50\x8B\x45\x1C\x51\x8B\x4D\x18\x52\x8B\x55\x14\x50\x8B\x45\x10\x51"

#define GL_LOADTEXTURE2_SIG "\xB8\x0C\x40\x00\x00\xE8\x2A\x2A\x2A\x2A\x53\x8B\x9C\x24\x14\x40\x00\x00\x55\x56\x8A\x03\x33\xF6"
#define GL_LOADTEXTURE2_SIG2 "\xB8\x0C\x40\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x84\x24\x10\x40\x00\x00\x53\x55\x33\xDB\x8A\x08\x56\x84\xC9\x57\x89\x5C\x24\x14"
#define GL_LOADTEXTURE2_SIG_NEW "\x55\x8B\xEC\xB8\x0C\x40\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x45\x08\x53\x33\xDB\x56\x8A\x08\x57\x84\xC9\x89\x5D\xF4\x74\x2A\x33\xF6"
#define GL_LOADTEXTURE2_SIG_SVENGINE "\x51\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x33\xC0\x59\xC3\x2A\x55\x8B\x6C\x24\x10"

#define GL_BEGINRENDERING_SIG "\x8B\x44\x24\x08\x8B\x4C\x24\x04\x8B\x54\x24\x0C\xC7\x00\x00\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\xC7\x01\x00\x00\x00\x00\x8B\x0D\x2A\x2A\x2A\x2A\x2B\xC1"
#define GL_BEGINRENDERING_SIG_NEW "\x55\x8B\xEC\x8B\x45\x0C\x8B\x4D\x08\x56\x57\xC7\x00\x00\x00\x00\x00\xC7\x01\x00\x00\x00\x00\xE8"
#define GL_BEGINRENDERING_SIG_SVENGINE "\x8B\x44\x24\x08\x56\x57\xC7\x00\x00\x00\x00\x00\x8B\x44\x24\x0C\xC7\x00\x00\x00\x00\x00\xE8"

#define GL_ENDRENDERING_SIG "\xFF\x25\x2A\x2A\x2A\x2A\x90"
#define GL_ENDRENDERING_SIG_NEW "\x55\x8B\xEC\x83\xEC\x34\xA1\x2A\x2A\x2A\x2A\x53\x33\xDB"
#define GL_ENDRENDERING_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\xC0\x83\xEC\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00"

#define EMITWATERPOLYS_SIG "\x83\xEC\x1C\x33\xD2\x55\x56\x8B\x74\x24\x28\x57\x8B\x46\x2C\x8B\x48\x24\x8B\x41\x44\x33\xC9\x8A\x50\x0C\x8A\x48\x0B\x52"
#define EMITWATERPOLYS_SIG_NEW "\x55\x8B\xEC\x83\xEC\x20\x56\x8B\x75\x08\x33\xD2\x57\x8B\x46\x2C\x8B\x48\x24\x8B\x41\x44\x33\xC9\x8A\x50\x0C\x8A\x48\x0B\x52\x51"
#define EMITWATERPOLYS_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x20\x53\x55\xD9\x05\x2A\x2A\x2A\x2A\x56\x57"

#define R_DRAWTENTITIESONLIST_SIG "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x0F\x2A\x2A\x2A\x00\x00\x8B\x44\x24\x04"
#define R_DRAWTENTITIESONLIST_SIG_NEW "\x55\x8B\xEC\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x44\x0F\x8B\x2A\x2A\x2A\x2A\x8B\x45\x08"
#define R_DRAWTENTITIESONLIST_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\x2A\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x00\x00\x00\xD9\x05\x2A\x2A\x2A\x2A\xD9\xEE"

#define R_SETUPGL_SIG "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x10\x53\x55\x56\x57\x68\x01\x17\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xFF\x15"
#define R_SETUPGL_SIG_NEW "\x55\x8B\xEC\x83\xEC\x10\x53\x56\x57\x68\x01\x17\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xFF\x15\x2A\x2A\x2A\x2A\x8B\x1D"
#define R_SETUPGL_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\xC0\x83\xEC\x2A\x53\x56\x57\x68\x01\x17\x00\x00"

#define GL_DISABLEMULTITEXTURE_SIG "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x50\xE8"
#define GL_DISABLEMULTITEXTURE_SIG_NEW "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x50\xE8"
#define GL_DISABLEMULTITEXTURE_SIG_SVENGINE "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\xC0\x84\x00\x00\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xC3"

#define GL_ENABLEMULTITEXTURE_SIG "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xE1\x0D\x00\x00\xFF\x15"
#define GL_ENABLEMULTITEXTURE_SIG_NEW "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xE1\x0D\x00\x00\xFF\x15"
#define GL_ENABLEMULTITEXTURE_SIG_SVENGINE "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x68\xC1\x84\x00\x00\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x01\x00\x00\x00\xC3"

#define R_DRAWSEQUENTIALPOLY_SIG "\xA1\x2A\x2A\x2A\x2A\x53\x55\x56\x8B\x88\xF8\x02\x00\x00\xBE\x01\x00\x00\x00"
#define R_DRAWSEQUENTIALPOLY_SIG_NEW "\x55\x8B\xEC\x51\xA1\x2A\x2A\x2A\x2A\x53\x56\x57\x83\xB8\xF8\x02\x00\x00\x01\x75\x2A\xE8"
#define R_DRAWSEQUENTIALPOLY_SIG_SVENGINE "\xA1\x2A\x2A\x2A\x2A\x83\xEC\x08\x83\xB8\x2A\x2A\x00\x00\x01"

#define R_DRAWBRUSHMODEL_SIG "\x83\xEC\x4C\xC7\x05\x2A\x2A\x2A\x2A\xFF\xFF\xFF\xFF\x53\x55\x56\x57"
#define R_DRAWBRUSHMODEL_SIG_NEW "\x55\x8B\xEC\x83\xEC\x50\x53\x56\x57\x8B\x7D\x08\x89\x3D\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\xFF\xFF\xFF\xFF"
#define R_DRAWBRUSHMODEL_SIG_SVENGINE "\x83\xEC\x54\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x50\x53\x8B\x5C\x24\x5C\x55\x56\x57\x89\x1D\x2A\x2A\x2A\x2A\x8D\xBB"

#define R_RECURSIVEWORLDNODE_SIG "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x0C\x53\x56\x57\x8B\x7D\x08\x83\x3F\xFE"
#define R_RECURSIVEWORLDNODE_SIG_NEW "\x55\x8B\xEC\x83\xEC\x08\x53\x56\x57\x8B\x7D\x08\x83\x3F\xFE\x0F\x2A\x2A\x2A\x2A\x2A\x8B\x47\x04"
#define R_RECURSIVEWORLDNODE_SIG_SVENGINE "\x83\xEC\x08\x53\x8B\x5C\x24\x10\x83\x3B\xFE"

#define R_TEXTUREANIMATION_SIG "\x53\x55\x8B\x6C\x24\x0C\x56\x57\x8B\x45\x2C\x8B\x78\x24\xA1"
#define R_TEXTUREANIMATION_SIG_NEW "\x55\x8B\xEC\x8B\x45\x08\x53\x56\x57\x8B\x48\x2C\xA1\x2A\x2A\x2A\x2A\x85\xC0\x8B\x79\x24\x75\x2A\xBE"
#define R_TEXTUREANIMATION_SIG_SVENGINE "\x83\x3D\x2A\x2A\x2A\x2A\x00\x8B\x54\x24\x2A\x53\x56\x57\x8B\x42\x2C"

#define R_RENDERDYNAMICLIGHTMAPS_SIG "\x8B\x0D\x2A\x2A\x2A\x2A\x53\x41\x55\x89\x0D\x2A\x2A\x2A\x2A\x8B\x4C\x24\x0C\x56\x57"
#define R_RENDERDYNAMICLIGHTMAPS_SIG_NEW "\x55\x8B\xEC\x8B\x0D\x2A\x2A\x2A\x2A\x53\x41\x56\x89\x0D\x2A\x2A\x2A\x2A\x8B\x4D\x08\x57"
#define R_RENDERDYNAMICLIGHTMAPS_SIG_SVENGINE "\x51\x8B\x54\x24\x08\xFF\x05\x2A\x2A\x2A\x2A\x57\x33\xFF\xF6\x42\x08\x14"

#define R_BUILDLIGHTMAP_SIG "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\x83\xEC\x18\xDF\xE0\xF6\xC4"
#define R_BUILDLIGHTMAP_SIG_NEW "\x55\x8B\xEC\x83\xEC\x1C\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0"
#define R_BUILDLIGHTMAP_SIG_SVENGINE "\xD9\x05\x2A\x2A\x2A\x2A\x83\xEC\x1C\xD9\xEE\xDD\xE1\xDF\xE0\x53\x55\x56\x57\xDD\xD9"

#define R_ADDDYNAMICLIGHTS_SIG_SVENGINE "\x83\xEC\x30\x2A\x2A\x8B\x2A\x24\x2A\x2A\x01\x00\x00\x00\x2A\x2A\xB9"
#define R_ADDDYNAMICLIGHTS_SIG_NEW "\x55\x8B\xEC\x83\xEC\x48\x2A\x8B\x2A\x08\x2A\x2A\x0F\x2A\x2A\x18"

#define R_DECALMPOLY_SIG "\xA1\x2A\x2A\x2A\x2A\x57\x50\xE8\x2A\x2A\x2A\x2A\x8B\x4C\x24\x10\x8B\x51\x18"
#define R_DECALMPOLY_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x57\x50\xE8\x2A\x2A\x2A\x2A\x8B\x4D\x0C\x8B\x51\x18\x52\xE8"

#define R_DRAWDECALS_SIG "\xB8\x0C\x00\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x85\xC0"
#define R_DRAWDECALS_SIG_NEW "\x55\x8B\xEC\xB8\x10\x00\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x84"
#define R_DRAWDECALS_SIG_SVENGINE "\xB8\x2A\x2A\x00\x00\xE8\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x84\x2A\x2A\x2A\x2A\x53\x8B\x1D"

#define R_DRAW_DECALTEXTURE_SIG "\x8B\x44\x24\x04\x56\x85\xC0\x57\x7D\x7B\x83\xC9\xFF\x2B\xC8\x8D\x04\x49"
#define R_DRAW_DECALTEXTURE_SIG2 "\x8B\x4C\x24\x04\x56\x85\xC9\x57\x2A\x2A\x83\xC8\xFF\x2B\xC1"
#define R_DRAW_DECALTEXTURE_SIG_NEW "\x55\x8B\xEC\x8B\x4D\x08\x56\x85\xC9\x57\x7D\x2A\x83\xC8\xFF\x2B\xC1\x8D\x0C\xC0"
#define R_DRAW_DECALTEXTURE_SIG_SVENGINE "\x8B\x4C\x24\x2A\x0F\xAE\xE8\x85\xC9\x2A\x2A\xF7\xD1"

#define R_RENDERVIEW_SIG "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\x83\xEC\x14\xDF\xE0\xF6\xC4"
#define R_RENDERVIEW_SIG_NEW "\x55\x8B\xEC\x83\xEC\x14\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x44"
#define R_RENDERVIEW_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\xC0\x83\xEC\x34\x53\x56\x57\x8B\x7D\x08\x85\xFF"

#define VID_UPDATEWINDOWVARS_SIG "\x56\x8B\x74\x24\x08\x8B\xC6\x8B\x08\x89\x0D\x2A\x2A\x2A\x2A\x8B\x50\x04\x89\x15"
#define VID_UPDATEWINDOWVARS_SIG_NEW "\x55\x8B\xEC\x51\x56\x8B\x75\x08\x8B\xC6\x8B\x08\x89\x0D\x2A\x2A\x2A\x2A\x8B\x50\x04\x89\x15"
#define VID_UPDATEWINDOWVARS_SIG_SVENGINE "\x8b\xc7\x99\x2B\xC2\xD1\xF8\x03\x2A\x50"

#define MOD_POINTINLEAF_SIG_SVENGINE "\x56\x8B\x74\x24\x0C\x85\xF6\x2A\x2A\x8B\x8E\xA4\x00\x00\x00"
#define MOD_POINTINLEAF_SIG_NEW "\x55\x8B\xEC\x2A\x8B\x2A\x0C\x85\x2A\x2A\x2A\x8B\x2A\xA4\x00\x00\x00"

#define R_FORCECVAR_SIG_SVENGINE "\x83\x7C\x24\x2A\x00\x2A\x2A\x2A\x2A\x00\x00\x81\x3D\x2A\x2A\x2A\x2A\xFF\x00\x00\x00"
#define R_FORCECVAR_SIG_NEW "\x55\x8B\xEC\x8B\x45\x08\x85\xC0\x0F\x84\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D"

#define CL_ALLOCDLIGHT_SIG_SVENGINE "\x2A\x8B\x5C\x24\x2A\x2A\x2A\x85\x2A\x2A\x2A\xBE\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x20"
#define CL_ALLOCDLIGHT_SIG_NEW "\x55\x8B\xEC\x2A\x8B\x5D\x08\x2A\x2A\x85\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x20"

//Studio Funcs
#define R_GLSTUDIODRAWPOINTS_SIG "\x83\xEC\x44\xA1\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x53\x55\x56\x8B\x70\x54\x8B\x40\x60\x57"
#define R_GLSTUDIODRAWPOINTS_SIG2 "\x83\xEC\x48\x8B\x0D\x2A\x2A\x2A\x2A\x8B\x15\x2A\x2A\x2A\x2A\x53\x55\x8B\x41\x54\x8B\x59\x60"
#define R_GLSTUDIODRAWPOINTS_SIG_NEW "\x55\x8B\xEC\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x53\x56\x8B\x70\x54\x8B\x40\x60\x03\xC1"
#define R_GLSTUDIODRAWPOINTS_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\x53\x8B\x1D\x2A\x2A\x2A\x2A\x55\x56\x57\x8B\x3D\x2A\x2A\x2A\x2A\x8B\x4B\x54"

#define R_STUDIOLIGHTING_SIG "\x51\xDB\x05\x2A\x2A\x2A\x2A\x8A\x4C\x24\x10\xB8\x01\x00\x00\x00\x84\xC8\xD9\x5C\x24\x00"
#define R_STUDIOLIGHTING_SIG_NEW "\x55\x8B\xEC\x51\xDB\x05\x2A\x2A\x2A\x2A\x8A\x4D\x10\xB8\x01\x00\x00\x00\x84\xC8\xD9\x5D\xFC"
#define R_STUDIOLIGHTING_SIG_SVENGINE "\xF6\x2A\x2A\x2A\x2A\xDB\x05\x2A\x2A\x2A\x2A\x74\x1C\x83\x3D"

#define R_LIGHTLAMBERT_SIG "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x2C\x8B\x0D\x2A\x2A\x2A\x2A\x53"
#define R_LIGHTLAMBERT_SIG2 "\x8B\x0D\x2A\x2A\x2A\x2A\x83\xEC\x24\x53\x33\xDB\x56"
#define R_LIGHTLAMBERT_SIG_NEW "\x55\x8B\xEC\x83\xEC\x24\x8B\x0D\x2A\x2A\x2A\x2A\x56\x57\x33\xFF\x3B\xCF\x75"
#define R_LIGHTLAMBERT_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\xD9\xE8\x8D\x4C\x24\x2A\x8B\x44\x24\x2A\x53"

#define R_STUDIOCHROME_SIG "\x83\xEC\x24\x8B\x0D\x2A\x2A\x2A\x2A\x53\x56\x8B\x74\x24\x34\x57\x8B\x04\xB5"
#define R_STUDIOCHROME_SIG_NEW "\x55\x8B\xEC\x83\xEC\x24\x8B\x0D\x2A\x2A\x2A\x2A\x53\x56\x8B\x75\x0C\x57\x8B\x04\xB5"
#define R_STUDIOCHROME_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\xD9\xE8\x53"

#define BUILDNORMALINDEXTABLE_SIG "\x8B\x15\x2A\x2A\x2A\x2A\x57\x8B\x4A\x50\x85\xC9\x7E\x2A\x83\xC8\xFF\xBF\x2A\x2A\x2A\x2A"
#define BUILDNORMALINDEXTABLE_SIG_NEW "\x55\x8B\xEC\x51\x8B\x15\x2A\x2A\x2A\x2A\x57\x8B\x4A\x50\x85\xC9\x7E\x0A\x83\xC8\xFF\xBF"
#define BUILDNORMALINDEXTABLE_SIG_SVENGINE ""

#define R_STUDIOSETUPSKIN_SIG "\xA0\x2A\x2A\x2A\x2A\x81\xEC\x0C\x01\x00\x00\xA8\x02\x53\x56\x57\x0F\x85"
#define R_STUDIOSETUPSKIN_SIG_NEW "\x55\x8B\xEC\x81\xEC\x04\x01\x00\x00\xA0\x2A\x2A\x2A\x2A\x53\x56\x57\xA8\x02\x0F\x85\x2A\x2A\x2A\x2A\x8B\x5D\x0C"
#define R_STUDIOSETUPSKIN_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\xF6\x05"

//Sprite Funcs
#define R_GETSPRITEFRAME_SIG "\x56\x8B\x74\x24\x08\x57\x33\xFF\x85\xF6\x75\x2A\x68\x2A\x2A\x2A\x2A\xE8"
#define R_GETSPRITEFRAME_SIG_NEW "\x55\x8B\xEC\x56\x8B\x75\x08\x57\x33\xFF\x85\xF6\x75\x13\x68"

#define R_DRAWSRPITEMODEL_SIG "\x83\xEC\x40\x53\x56\x57\x8B\x7C\x24\x50\x8B\x87\x94\x0B\x00\x00"
#define R_DRAWSRPITEMODEL_SIG_NEW "\x55\x8B\xEC\x83\xEC\x44\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x2A\x2A\x2A\x00\x00\xD9\x2A\xE0\x02"
#define R_DRAWSRPITEMODEL_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\x53\x8B\x5C\x24\x2A\x55\x8B\x83\x2A\x0B\x00\x00"

#define R_LIGHTSTRENGTH_SIG_SVENGINE "\x8B\x15\x2A\x2A\x2A\x2A\x2A\x8B\x35\x2A\x2A\x2A\x2A\x2A\x8B\x7C\x24\x0C"
#define R_LIGHTSTRENGTH_SIG_NEW "\x55\x8B\xEC\x83\xEC\x0C\x8B\x4D\x08\x8B\x15\x2A\x2A\x2A\x2A\x2A\x8B\x04\x2A\x2A\x2A\x2A\x2A"

#define GLOW_BLEND_SVENGINE "\x55\x8B\xEC\x83\x2A\x2A\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\xA0\x00"
#define GLOW_BLEND_SIG_NEW ""

void Sys_ErrorEx(const char *fmt, ...);

void R_FillAddress(void)
{
	DWORD addr;

	auto engineFactory = Sys_GetFactory((HINTERFACEMODULE)g_dwEngineBase); 
#define ENGINE_SURFACE_VERSION "EngineSurface007"
	void *engineSurface = (void *)engineFactory(ENGINE_SURFACE_VERSION, NULL);

	gRefFuncs.enginesurface_drawFlushText = *(decltype(gRefFuncs.enginesurface_drawFlushText) *)(*(DWORD *)engineSurface + 0x58);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_MarkLights = (decltype(gRefFuncs.R_MarkLights))Search_Pattern(R_MARKLIGHTS_SVENGINE);
		Sig_FuncNotFound(R_MarkLights);
	}
	else
	{
		gRefFuncs.R_MarkLights = (decltype(gRefFuncs.R_MarkLights))Search_Pattern(R_MARKLIGHTS_NEW);
		Sig_FuncNotFound(R_MarkLights);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_PolyBlend = (decltype(gRefFuncs.R_PolyBlend))Search_Pattern(R_POLYBLEND_SVENGINE);
		Sig_FuncNotFound(R_PolyBlend);
	}
	else
	{
		gRefFuncs.R_PolyBlend = (decltype(gRefFuncs.R_PolyBlend))Search_Pattern(R_POLYBLEND_NEW);
		Sig_FuncNotFound(R_PolyBlend);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.S_ExtraUpdate = (decltype(gRefFuncs.S_ExtraUpdate))Search_Pattern(S_EXTRAUPDATE_SVENGINE);
		Sig_FuncNotFound(S_ExtraUpdate);
	}
	else
	{
		gRefFuncs.S_ExtraUpdate = (decltype(gRefFuncs.S_ExtraUpdate))Search_Pattern(S_EXTRAUPDATE_NEW);
		Sig_FuncNotFound(S_ExtraUpdate);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.GL_Bind = (void(*)(int))Search_Pattern(GL_BIND_SIG_SVENGINE);
		Sig_FuncNotFound(GL_Bind);
	}
	else
	{
		gRefFuncs.GL_Bind = (void(*)(int))Search_Pattern(GL_BIND_SIG_NEW);
		Sig_FuncNotFound(GL_Bind);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.GL_SelectTexture = (void(*)(GLenum))Search_Pattern_From(gRefFuncs.GL_Bind, GL_SELECTTEXTURE_SIG_SVENGINE);
		Sig_FuncNotFound(GL_SelectTexture);
	}
	else
	{
		gRefFuncs.GL_SelectTexture = (void(*)(GLenum))Search_Pattern(GL_SELECTTEXTURE_SIG_NEW);
		Sig_FuncNotFound(GL_SelectTexture);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.GL_LoadTexture2 = (int(*)(char *, int, int, int, byte *, qboolean, int, byte *, int))Search_Pattern(GL_LOADTEXTURE2_SIG_SVENGINE);
		Sig_FuncNotFound(GL_LoadTexture2);
	}
	else
	{
		gRefFuncs.GL_LoadTexture2 = (int(*)(char *, int, int, int, byte *, qboolean, int, byte *, int))Search_Pattern(GL_LOADTEXTURE2_SIG_NEW);
		Sig_FuncNotFound(GL_LoadTexture2);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_CullBox = (qboolean(*)(vec3_t, vec3_t))Search_Pattern_From(gRefFuncs.GL_LoadTexture2, R_CULLBOX_SIG_SVENGINE);
		Sig_FuncNotFound(R_CullBox);
	}
	else
	{
		gRefFuncs.R_CullBox = (qboolean(*)(vec3_t, vec3_t))Search_Pattern_From(gRefFuncs.GL_LoadTexture2, R_CULLBOX_SIG_NEW);
		Sig_FuncNotFound(R_CullBox);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_SetupFrame = NULL;//inlined
	}
	else
	{
		gRefFuncs.R_SetupFrame = (void(*)(void))Search_Pattern(R_SETUPFRAME_SIG_NEW);
		Sig_FuncNotFound(R_SetupFrame);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_SetupGL = (void(*)(void))Search_Pattern(R_SETUPGL_SIG_SVENGINE);
		Sig_FuncNotFound(R_SetupGL);
	}
	else
	{
		gRefFuncs.R_SetupGL = (void(*)(void))Search_Pattern_From(gRefFuncs.R_SetupFrame, R_SETUPGL_SIG_NEW);
		Sig_FuncNotFound(R_SetupGL);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_DrawSequentialPoly = (void(*)(msurface_t *, int))Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_SVENGINE);
		Sig_FuncNotFound(R_DrawSequentialPoly);
	}
	else
	{
		gRefFuncs.R_DrawSequentialPoly = (void(*)(msurface_t *, int))Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_NEW);
		Sig_FuncNotFound(R_DrawSequentialPoly);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_TextureAnimation = (texture_t *(*)(msurface_t *))Search_Pattern(R_TEXTUREANIMATION_SIG_SVENGINE);
		Sig_FuncNotFound(R_TextureAnimation);
	}
	else
	{
		gRefFuncs.R_TextureAnimation = (texture_t *(*)(msurface_t *))Search_Pattern(R_TEXTUREANIMATION_SIG_NEW);
		Sig_FuncNotFound(R_TextureAnimation);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_RenderView_SvEngine = (void(*)(int))Search_Pattern(R_RENDERVIEW_SIG_SVENGINE);
		Sig_FuncNotFound(R_RenderView_SvEngine);

		addr = (DWORD)Search_Pattern_From(gRefFuncs.R_RenderView_SvEngine, R_RENDERSCENE_SIG_SVENGINE);
		Sig_AddrNotFound(R_RenderScene);
		gRefFuncs.R_RenderScene = (void(*)(void))(addr + 5 + 4 + *(int *)(addr + 5));
	}
	else
	{
		gRefFuncs.R_RenderView = (void(*)(void))Search_Pattern(R_RENDERVIEW_SIG_NEW);
		Sig_FuncNotFound(R_RenderView);

		gRefFuncs.R_RenderScene = (void(*)(void))Search_Pattern(R_RENDERSCENE_SIG_NEW);
		Sig_FuncNotFound(R_RenderScene);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_NewMap = (void(*)(void))Search_Pattern(R_NEWMAP_SIG_SVENGINE);
		Sig_FuncNotFound(R_NewMap);
	}
	else
	{
		gRefFuncs.R_NewMap = (void(*)(void))Search_Pattern(R_NEWMAP_SIG_NEW);
		Sig_FuncNotFound(R_NewMap);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_BuildLightMap = (void(*)(msurface_t *, byte *, int))Search_Pattern(R_BUILDLIGHTMAP_SIG_SVENGINE);
		Sig_FuncNotFound(R_BuildLightMap);
	}
	else
	{
		gRefFuncs.R_BuildLightMap = (void(*)(msurface_t *, byte *, int))Search_Pattern(R_BUILDLIGHTMAP_SIG_NEW);
		Sig_FuncNotFound(R_BuildLightMap);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_AddDynamicLights = (void(*)(msurface_t *))Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_SVENGINE);
		Sig_FuncNotFound(R_AddDynamicLights);
	}
	else
	{
		gRefFuncs.R_AddDynamicLights = (void(*)(msurface_t *))Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_NEW);
		Sig_FuncNotFound(R_AddDynamicLights);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.GL_DisableMultitexture = (void(*)(void))Search_Pattern(GL_DISABLEMULTITEXTURE_SIG_SVENGINE);
		Sig_FuncNotFound(GL_DisableMultitexture);
	}
	else
	{
		gRefFuncs.GL_DisableMultitexture = (void(*)(void))Search_Pattern_From(gRefFuncs.R_NewMap, GL_DISABLEMULTITEXTURE_SIG_NEW);
		Sig_FuncNotFound(GL_DisableMultitexture);

	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.GL_EnableMultitexture = (void(*)(void))Search_Pattern(GL_ENABLEMULTITEXTURE_SIG_SVENGINE);
		Sig_FuncNotFound(GL_EnableMultitexture);
	}
	else
	{
		gRefFuncs.GL_EnableMultitexture = (void(*)(void))Search_Pattern_From(gRefFuncs.GL_DisableMultitexture, GL_ENABLEMULTITEXTURE_SIG_NEW);
		Sig_FuncNotFound(GL_EnableMultitexture);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_RenderDynamicLightmaps = (void(*)(msurface_t *))Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_SVENGINE);
		Sig_FuncNotFound(R_RenderDynamicLightmaps);
	}
	else
	{
		gRefFuncs.R_RenderDynamicLightmaps = (void(*)(msurface_t *))Search_Pattern_From(gRefFuncs.GL_EnableMultitexture, R_RENDERDYNAMICLIGHTMAPS_SIG_NEW);
		Sig_FuncNotFound(R_RenderDynamicLightmaps);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_DrawBrushModel = (void(*)(cl_entity_t *))Search_Pattern(R_DRAWBRUSHMODEL_SIG_SVENGINE);
		Sig_FuncNotFound(R_DrawBrushModel);
	}
	else
	{
		gRefFuncs.R_DrawBrushModel = (void(*)(cl_entity_t *))Search_Pattern_From(gRefFuncs.R_RenderDynamicLightmaps, R_DRAWBRUSHMODEL_SIG_NEW);
		Sig_FuncNotFound(R_DrawBrushModel);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_RecursiveWorldNode = (void(*)(mnode_t *))Search_Pattern(R_RECURSIVEWORLDNODE_SIG_SVENGINE);
		Sig_FuncNotFound(R_RecursiveWorldNode);
	}
	else
	{
		gRefFuncs.R_RecursiveWorldNode = (void(*)(mnode_t *))Search_Pattern_From(gRefFuncs.R_DrawBrushModel, R_RECURSIVEWORLDNODE_SIG_NEW);
		Sig_FuncNotFound(R_RecursiveWorldNode);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_DrawWorld = (void(*)(void))Search_Pattern(R_DRAWWORLD_SIG_SVENGINE);
		Sig_FuncNotFound(R_DrawWorld);
	}
	else
	{
		gRefFuncs.R_DrawWorld = (void(*)(void))Search_Pattern_From(gRefFuncs.R_RecursiveWorldNode, R_DRAWWORLD_SIG_NEW);
		Sig_FuncNotFound(R_DrawWorld);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_MarkLeaves = (void(*)(void))Search_Pattern(R_MARKLEAVES_SIG_SVENGINE);
		Sig_FuncNotFound(R_MarkLeaves);
	}
	else
	{
		gRefFuncs.R_MarkLeaves = (void(*)(void))Search_Pattern_From(gRefFuncs.R_DrawWorld, R_MARKLEAVES_SIG_NEW);
		Sig_FuncNotFound(R_MarkLeaves);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.GL_BeginRendering = (void(*)(int *, int *, int *, int *))Search_Pattern(GL_BEGINRENDERING_SIG_SVENGINE);
		Sig_FuncNotFound(GL_BeginRendering);
	}
	else
	{

		gRefFuncs.GL_BeginRendering = (void(*)(int *, int *, int *, int *))Search_Pattern(GL_BEGINRENDERING_SIG_NEW);
		Sig_FuncNotFound(GL_BeginRendering);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.GL_EndRendering = (void(*)(void))Search_Pattern(GL_ENDRENDERING_SIG_SVENGINE);
		Sig_FuncNotFound(GL_EndRendering);
	}
	else
	{
		gRefFuncs.GL_EndRendering = (void(*)(void))Search_Pattern_From(gRefFuncs.GL_BeginRendering, GL_ENDRENDERING_SIG_NEW);
		Sig_FuncNotFound(GL_EndRendering);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.EmitWaterPolys = (void(*)(msurface_t *, int))Search_Pattern(EMITWATERPOLYS_SIG_SVENGINE);
		Sig_FuncNotFound(EmitWaterPolys);
	}
	else
	{
		gRefFuncs.EmitWaterPolys = (void(*)(msurface_t *, int))Search_Pattern(EMITWATERPOLYS_SIG_NEW);
		Sig_FuncNotFound(EmitWaterPolys);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		addr = (DWORD)Search_Pattern(VID_UPDATEWINDOWVARS_SIG_SVENGINE);
		Sig_AddrNotFound(VID_UpdateWindowVars);

		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)addr, 0x50, "\x50\xE8", 2);
		Sig_AddrNotFound(VID_UpdateWindowVars);

		gRefFuncs.VID_UpdateWindowVars = (void(*)(RECT *prc, int x, int y))(addr + 2 + 4 + *(int *)(addr + 2));
		Sig_FuncNotFound(VID_UpdateWindowVars);
	}
	else
	{
		gRefFuncs.VID_UpdateWindowVars = (void(*)(RECT *prc, int x, int y))Search_Pattern(VID_UPDATEWINDOWVARS_SIG_NEW);
		Sig_FuncNotFound(VID_UpdateWindowVars);

	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.Mod_PointInLeaf = (mleaf_t *(*)(vec3_t, model_t *))Search_Pattern(MOD_POINTINLEAF_SIG_SVENGINE);
		Sig_FuncNotFound(Mod_PointInLeaf);
	}
	else
	{
		gRefFuncs.Mod_PointInLeaf = (mleaf_t *(*)(vec3_t, model_t *))Search_Pattern(MOD_POINTINLEAF_SIG_NEW);
		Sig_FuncNotFound(Mod_PointInLeaf);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_ForceCVars = (void(*)(qboolean))Search_Pattern(R_FORCECVAR_SIG_SVENGINE);
		Sig_FuncNotFound(R_ForceCVars);
	}
	else
	{
		gRefFuncs.R_ForceCVars = (void(*)(qboolean))Search_Pattern(R_FORCECVAR_SIG_NEW);
		Sig_FuncNotFound(R_ForceCVars);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_DrawTEntitiesOnList = (void(*)(int))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_SVENGINE);
		Sig_FuncNotFound(R_DrawTEntitiesOnList);
	}
	else
	{
		gRefFuncs.R_DrawTEntitiesOnList = (void(*)(int))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW);
		Sig_FuncNotFound(R_DrawTEntitiesOnList);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.CL_AllocDlight = (dlight_t *(*)(int))Search_Pattern(CL_ALLOCDLIGHT_SIG_SVENGINE);
		Sig_FuncNotFound(CL_AllocDlight);
	}
	else
	{
		gRefFuncs.CL_AllocDlight = (dlight_t *(*)(int))Search_Pattern(CL_ALLOCDLIGHT_SIG_NEW);
		Sig_FuncNotFound(CL_AllocDlight);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_GLStudioDrawPoints = (void(*)(void))Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_SVENGINE);
		Sig_FuncNotFound(R_GLStudioDrawPoints);
	}
	else
	{
		gRefFuncs.R_GLStudioDrawPoints = (void(*)(void))Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_NEW);
		Sig_FuncNotFound(R_GLStudioDrawPoints);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_StudioLighting = (void(*)(float *lv, int bone, int flags, vec3_t normal))Search_Pattern(R_STUDIOLIGHTING_SIG_SVENGINE);
		Sig_FuncNotFound(R_StudioLighting);
	}
	else
	{
		gRefFuncs.R_StudioLighting = (void(*)(float *, int, int, vec3_t))Search_Pattern(R_STUDIOLIGHTING_SIG_NEW);
		Sig_FuncNotFound(R_StudioLighting);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_StudioChrome = (void(*)(int *pchrome, int bone, vec3_t normal))Search_Pattern(R_STUDIOCHROME_SIG_SVENGINE);
		Sig_FuncNotFound(R_StudioChrome);
	}
	else
	{
		gRefFuncs.R_StudioChrome = (void(*)(int *, int, vec3_t))Search_Pattern(R_STUDIOCHROME_SIG_NEW);
		Sig_FuncNotFound(R_StudioChrome);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_LightLambert = (void(*)(float(*light)[4], float *normal, float *src, float *lambert))Search_Pattern(R_LIGHTLAMBERT_SIG_SVENGINE);
		Sig_FuncNotFound(R_LightLambert);
	}
	else
	{
		gRefFuncs.R_LightLambert = (void(*)(float(*)[4], float *, float *, float *))Search_Pattern(R_LIGHTLAMBERT_SIG_NEW);
		Sig_FuncNotFound(R_LightLambert);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_StudioSetupSkin = (void(*)(studiohdr_t *, int))Search_Pattern(R_STUDIOSETUPSKIN_SIG_SVENGINE);
		Sig_FuncNotFound(R_StudioSetupSkin);
	}
	else
	{
		gRefFuncs.R_StudioSetupSkin = (void(*)(studiohdr_t *, int))Search_Pattern(R_STUDIOSETUPSKIN_SIG_NEW);
		Sig_FuncNotFound(R_StudioSetupSkin);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.Draw_DecalTexture = (texture_t *(*)(int))Search_Pattern(R_DRAW_DECALTEXTURE_SIG_SVENGINE);
		Sig_FuncNotFound(Draw_DecalTexture);
	}
	else
	{
		gRefFuncs.Draw_DecalTexture = (texture_t *(*)(int))Search_Pattern(R_DRAW_DECALTEXTURE_SIG_NEW);
		Sig_FuncNotFound(Draw_DecalTexture);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_DrawSpriteModel = (void(*)(cl_entity_t *))Search_Pattern(R_DRAWSRPITEMODEL_SIG_SVENGINE);
		Sig_FuncNotFound(R_DrawSpriteModel);
	}
	else
	{
		gRefFuncs.R_DrawSpriteModel = (void(*)(cl_entity_t *))Search_Pattern(R_DRAWSRPITEMODEL_SIG_NEW);
		Sig_FuncNotFound(R_DrawSpriteModel);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_LightStrength = (decltype(gRefFuncs.R_LightStrength))Search_Pattern(R_LIGHTSTRENGTH_SIG_SVENGINE);
		Sig_FuncNotFound(R_LightStrength);
	}
	else
	{
		gRefFuncs.R_LightStrength = (decltype(gRefFuncs.R_LightStrength))Search_Pattern(R_LIGHTSTRENGTH_SIG_NEW);
		Sig_FuncNotFound(R_LightStrength);
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.R_RotateForEntity = (void(*)(float *, cl_entity_t *))Search_Pattern(R_ROTATEFORENTITY_SVENGINE);
		Sig_FuncNotFound(R_RotateForEntity);
	}
	else
	{
		gRefFuncs.R_RotateForEntity = (void(*)(float *, cl_entity_t *))Search_Pattern(R_ROTATEFORENTITY_NEW);
		Sig_FuncNotFound(R_RotateForEntity);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{

	}
	else
	{
		gRefFuncs.BuildNormalIndexTable = (void(*)(void))Search_Pattern(BUILDNORMALINDEXTABLE_SIG_NEW);
		Sig_FuncNotFound(BuildNormalIndexTable);
	}
	
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gRefFuncs.GlowBlend = (float(*)(cl_entity_t *))Search_Pattern(GLOW_BLEND_SVENGINE);
		Sig_FuncNotFound(GlowBlend);
	}
	else
	{
		
	}
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs1[] = "Can't add transparent entity. Too many";
		auto R_AddTEntity_String = Search_Pattern_Data(sigs1);
		if (!R_AddTEntity_String)
			R_AddTEntity_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(R_AddTEntity_String);
		char pattern[] = "\x50\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD *)(pattern + 2) = (DWORD)R_AddTEntity_String;
		auto R_AddTEntity_Call = Search_Pattern(pattern);
		Sig_VarNotFound(R_AddTEntity_Call);

		gRefFuncs.R_AddTEntity = (decltype(gRefFuncs.R_AddTEntity))g_pMetaHookAPI->ReverseSearchFunctionBegin(R_AddTEntity_Call, 0x50);
		Sig_FuncNotFound(R_AddTEntity);
	}
	else
	{
		const char sigs1[] = "AddTentity: Too many objects";
		auto R_AddTEntity_String = Search_Pattern_Data(sigs1);
		if(!R_AddTEntity_String)
			R_AddTEntity_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(R_AddTEntity_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD *)(pattern + 1) = (DWORD)R_AddTEntity_String;
		auto R_AddTEntity_Call = Search_Pattern(pattern);
		Sig_VarNotFound(R_AddTEntity_Call);

		gRefFuncs.R_AddTEntity = (decltype(gRefFuncs.R_AddTEntity))g_pMetaHookAPI->ReverseSearchFunctionBegin(R_AddTEntity_Call, 0x50);
		Sig_FuncNotFound(R_AddTEntity);
	}

	if (g_iEngineType == ENGINE_GOLDSRC)
	{
		typedef struct
		{
			int a1;
		}Cvar_Set_ctx;

		Cvar_Set_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(g_pMetaSave->pEngineFuncs->Cvar_Set, 0x150, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (Cvar_Set_ctx *)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{//A1 40 77 7B 02 mov     eax, gl_backbuffer_fbo
				DWORD imm = pinst->detail->x86.operands[1].mem.disp;

				if (!cvar_callbacks)
				{
					cvar_callbacks = (decltype(cvar_callbacks))imm;
				}
			}

			if (cvar_callbacks)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(cvar_callbacks);
	}

	if (1)
	{
		typedef struct
		{
			int type;
			int zero_register;
			int load_zero_instcount;
		}GL_EndRendering_ctx;

		GL_EndRendering_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gRefFuncs.GL_EndRendering, 0x350, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (GL_EndRendering_ctx *)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{//A1 40 77 7B 02 mov     eax, gl_backbuffer_fbo
				DWORD imm = pinst->detail->x86.operands[1].mem.disp;

				if (!gl_backbuffer_fbo && !ctx->type)
				{
					gl_backbuffer_fbo = (decltype(gl_backbuffer_fbo))imm;
					ctx->type = 1;
				}
				else if (ctx->type == 1 && !gl_msaa_fbo)
				{
					gl_msaa_fbo = (decltype(gl_msaa_fbo))imm;
				}
			}
			else if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{//83 3D 94 66 00 08 00 cmp     gl_backbuffer_fbo, 0
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				if (!gl_backbuffer_fbo && !ctx->type)
				{
					gl_backbuffer_fbo = (decltype(gl_backbuffer_fbo))imm;
					ctx->type = 2;
				}
				else if (ctx->type == 2 && !gl_msaa_fbo)
				{
					gl_msaa_fbo = (decltype(gl_msaa_fbo))imm;
				}
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == ctx->zero_register &&
				instCount < ctx->load_zero_instcount + 5)
			{//.text:01D4D4C0 A3 F4 78 E4 01 mov     videowindowaspect_0, eax

				if (!videowindowaspect)
					videowindowaspect = (decltype(videowindowaspect))pinst->detail->x86.operands[0].mem.disp;
				else if (!windowvideoaspect)
					windowvideoaspect = (decltype(windowvideoaspect))pinst->detail->x86.operands[0].mem.disp;
			}
			else if ((pinst->id == X86_INS_FST || pinst->id == X86_INS_FSTP) &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				instCount < ctx->load_zero_instcount + 5)
			{//.D9 15 E0 85 ED 01 fst     videowindowaspect

				if (!videowindowaspect)
					videowindowaspect = (decltype(videowindowaspect))pinst->detail->x86.operands[0].mem.disp;
				else if (!windowvideoaspect)
					windowvideoaspect = (decltype(windowvideoaspect))pinst->detail->x86.operands[0].mem.disp;
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

			if (gl_backbuffer_fbo && gl_msaa_fbo && videowindowaspect && windowvideoaspect)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(gl_backbuffer_fbo);
		Sig_VarNotFound(gl_msaa_fbo);
		Sig_VarNotFound(videowindowaspect);
		Sig_VarNotFound(windowvideoaspect);
	}

	if (1)
	{
		char pattern[] = "\x8B\x0D\x2A\x2A\x2A\x2A\x81\xF9\x00\x02\x00\x00";
		auto ClientDLL_AddEntity_Pattern = Search_Pattern(pattern);
		Sig_VarNotFound(ClientDLL_AddEntity_Pattern);

		cl_numvisedicts = *(decltype(cl_numvisedicts)*)((PUCHAR)ClientDLL_AddEntity_Pattern + 2);

		g_pMetaHookAPI->DisasmRanges(ClientDLL_AddEntity_Pattern, 0x150, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (!cl_visedicts &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == X86_REG_ECX &&
				pinst->detail->x86.operands[0].mem.scale == 4 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG )
			{
				//.text:01D198C9 89 04 8D 00 3A 6E 02                                mov     cl_visedicts[ecx*4], eax
				//.text:01D0C7C5 89 14 8D C0 F0 D5 02                                mov     cl_visedicts[ecx*4], edx
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				cl_visedicts = (decltype(cl_visedicts))imm;
			}

			if (cl_visedicts)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(cl_visedicts);
	}

	if (1)
	{
		const char sigs[] = "Transparent objects reallocate";
		auto R_AllocTransObjects_String = Search_Pattern_Data(sigs);
		if (!R_AllocTransObjects_String)
			R_AllocTransObjects_String = Search_Pattern_Rdata(sigs);
		Sig_VarNotFound(R_AllocTransObjects_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
		*(DWORD *)(pattern + 1) = (DWORD)R_AllocTransObjects_String;
		auto R_AllocTransObjects_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(R_AllocTransObjects_PushString);

		PVOID R_AllocTransObjects = (decltype(R_AllocTransObjects))g_pMetaHookAPI->ReverseSearchFunctionBegin(R_AllocTransObjects_PushString, 0x50);
		Sig_VarNotFound(R_AllocTransObjects);

		g_pMetaHookAPI->DisasmRanges(R_AllocTransObjects, 0x80, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (!transObjects && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == X86_REG_EAX)
			{//.text:01D9205D A3 8C 61 DF 08 mov     transObjects, eax
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				transObjects = (decltype(transObjects))imm;
			}

			else if (!maxTransObjs && 
				transObjects && 
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//.text:01D9205D A3 8C 61 DF 08 mov     transObjects, eax
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				if(imm != (DWORD)transObjects)
					maxTransObjs = (decltype(maxTransObjs))imm;
			}

			if (transObjects && maxTransObjs)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(transObjects);
		Sig_VarNotFound(maxTransObjs);

		numTransObjs = (decltype(numTransObjs))((PUCHAR)maxTransObjs - 4);
	}

	if (1)
	{
		typedef struct
		{
			int disableFog_instcount;
			int parsemod_instcount;
			int getskin_instcount;
		}R_DrawTEntitiesOnList_ctx;

		R_DrawTEntitiesOnList_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gRefFuncs.R_DrawTEntitiesOnList, 0x250, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (R_DrawTEntitiesOnList_ctx *)context;

			if (pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].imm == GL_FOG)
			{//.text:01D92330 68 60 0B 00 00 push    0B60h
				DWORD imm = pinst->detail->x86.operands[0].imm;

				ctx->disableFog_instcount = instCount;
			}
			else if (!r_blend && ctx->disableFog_instcount &&
				instCount < ctx->disableFog_instcount + 15 &&
				(pinst->id == X86_INS_FSTP || pinst->id == X86_INS_FST) &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0)
			{//.text:01D92355 D9 15 80 61 DF 08 fst     r_blend
				r_blend = (decltype(r_blend))pinst->detail->x86.operands[0].mem.disp;
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				//.text:01D923D9 A1 DC 72 ED 01                                      mov     eax, cl_parsemod
				//.text:01D88CBB A1 CC AF E3 01                                      mov     eax, cl_parsemod
				DWORD value = *(DWORD *)pinst->detail->x86.operands[1].mem.disp;
				if (value == 63)
				{
					ctx->parsemod_instcount = instCount;
				}
			}
			else if (!cl_parsecount && ctx->parsemod_instcount &&
				instCount < ctx->parsemod_instcount + 3 &&
				(pinst->id == X86_INS_MOV || pinst->id == X86_INS_AND) &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				//.text:01D923DE 23 05 AC D2 30 02                                   and     eax, cl_parsecount
				//.text:01D88CC0 8B 0D 04 AE D8 02                                   mov     ecx, cl_parsecount
				cl_parsecount = (decltype(cl_parsecount))pinst->detail->x86.operands[1].mem.disp;
			}
			else if (!cl_frames && ctx->parsemod_instcount &&
				instCount < ctx->parsemod_instcount + 20 &&
				pinst->id == X86_INS_LEA &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base != 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				//.text:01D923F0 8D 80 F4 D5 30 02                                   lea     eax, cl_frames[eax]
				//.text:01D88CE8 8D 84 CA 4C B1 D8 02                                lea     eax, cl_frames_1[edx+ecx*8]
				cl_frames = (decltype(cl_frames))pinst->detail->x86.operands[1].mem.disp;
			}
			else if (ctx->parsemod_instcount &&
				instCount < ctx->parsemod_instcount + 5 &&
				pinst->id == X86_INS_IMUL &&
				pinst->detail->x86.op_count == 3 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[2].type == X86_OP_IMM &&
				pinst->detail->x86.operands[2].imm > 0x4000 &&
				pinst->detail->x86.operands[2].imm < 0xF000 )
			{
				//.text:01D923E4 69 C8 D8 84 00 00                                   imul    ecx, eax, 84D8h
				size_of_frame = pinst->detail->x86.operands[2].imm;
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
			else if (!r_entorigin &&
				ctx->getskin_instcount &&
				instCount < ctx->getskin_instcount + 8 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				//.text:01D88C23 89 15 E0 98 BC 02                                   mov     r_entorigin, edx
				r_entorigin = (decltype(r_entorigin))pinst->detail->x86.operands[0].mem.disp;
			}
			else if (!r_entorigin &&
				ctx->getskin_instcount &&
				instCount < ctx->getskin_instcount + 8 &&
				pinst->id == X86_INS_FST &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
			//.text:01D88C23 89 15 E0 98 BC 02                                   mov     r_entorigin, edx
				r_entorigin = (decltype(r_entorigin))pinst->detail->x86.operands[0].mem.disp;
			}

			if (r_blend && cl_parsecount && cl_frames && r_entorigin)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(r_blend);
		Sig_VarNotFound(cl_frames);
		Sig_VarNotFound(cl_parsecount);
		Sig_VarNotFound(r_entorigin);
	}

	if (1)
	{
		typedef struct
		{
			int movexx_offset;
			int movexx_instcount;
			int movexx_register;
			int cmp_register;
			DWORD cmp_candidate;
			int test_cl_instcount;
			int test_cl_flag;
		}R_RecursiveWorldNode_ctx;

		R_RecursiveWorldNode_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gRefFuncs.R_RecursiveWorldNode, 0x500, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (R_RecursiveWorldNode_ctx *)context;

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
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				)
			{
				//.text:01D49238 8B 0D D4 98 BC 02                                   mov     ecx, r_visframecount

				ctx->cmp_register = pinst->detail->x86.operands[0].reg;
				ctx->cmp_candidate = (decltype(ctx->cmp_candidate))pinst->detail->x86.operands[1].mem.disp;
			}
			else if (ctx->movexx_instcount &&
				instCount < ctx->movexx_instcount + 3 &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				)
			{
				//.text:01D5A533 3B 05 7C 3F F5 03                                   cmp     eax, r_visframecount

				if (ctx->movexx_offset == 4 && !r_visframecount)
					r_visframecount = (decltype(r_visframecount))pinst->detail->x86.operands[1].mem.disp;
				else if (ctx->movexx_offset == 0 && !r_framecount)
					r_framecount = (decltype(r_framecount))pinst->detail->x86.operands[1].mem.disp;
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
						pinst->detail->x86.operands[0].reg == ctx->movexx_register)) )
			{
				//.text:01D49235 8B 47 04                                            mov     eax, [edi+4]
				//.text:01D49238 8B 0D D4 98 BC 02                                   mov     ecx, r_visframecount
				//.text:01D4923E 3B C1                                               cmp     eax, ecx

				//.text:01D4932E 8B 0E                                               mov     ecx, [esi]
				//.text:01D49330 A1 EC 97 BC 02                                      mov     eax, r_framecount
				//.text:01D49335 3B C8                                               cmp     ecx, eax
				if (ctx->movexx_offset == 4 && !r_visframecount)
					r_visframecount = (decltype(r_visframecount))ctx->cmp_candidate;
				else if (ctx->movexx_offset == 0 && !r_framecount)
					r_framecount = (decltype(r_framecount))ctx->cmp_candidate;
			}
			else if (
				pinst->id == X86_INS_TEST &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_CL &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm >= 4 &&
				pinst->detail->x86.operands[1].imm <= 0x10)
			{
				//.text:01D493A7 F6 C1 04                                            test    cl, 4

				ctx->test_cl_flag = pinst->detail->x86.operands[1].imm;
				ctx->test_cl_instcount = instCount;
			}
			else if (ctx->test_cl_instcount &&
				instCount < ctx->test_cl_instcount + 3 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				)
			{
				//.text:01D5A66D A1 C4 54 F5 03                                      mov     eax, skychain

				if (!skychain && ctx->test_cl_flag == 4)
					skychain = (decltype(skychain))pinst->detail->x86.operands[1].mem.disp;
				else if (!waterchain && ctx->test_cl_flag == 0x10)
					waterchain = (decltype(waterchain))pinst->detail->x86.operands[1].mem.disp;
			}

			if (r_visframecount && r_framecount && skychain && waterchain )
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(r_framecount);
		Sig_VarNotFound(r_visframecount);
		Sig_VarNotFound(skychain);
		Sig_VarNotFound(waterchain);
	}

	if (1)
	{
		const char sigs[] = "SKY: ";
		auto R_LoadSkys_String = Search_Pattern_Data(sigs);
		if(!R_LoadSkys_String)
			R_LoadSkys_String = Search_Pattern_Rdata(sigs);
		Sig_VarNotFound(R_LoadSkys_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xC7\x2A\x2A\x2A\x2A\x00\x00";
		*(DWORD *)(pattern + 1) = (DWORD)R_LoadSkys_String;
		auto R_LoadSkys_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(R_LoadSkys_PushString);

		gRefFuncs.R_LoadSkys = (decltype(gRefFuncs.R_LoadSkys))g_pMetaHookAPI->ReverseSearchFunctionBegin(R_LoadSkys_PushString, 0x600);
		Sig_FuncNotFound(R_LoadSkys);

		typedef struct
		{
			int candidate_register;
			int candidate_instcount;
			DWORD candidate_gSkyTexNumber;
		}R_LoadSkys_ctx;

		R_LoadSkys_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gRefFuncs.R_LoadSkys, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (R_LoadSkys_ctx *)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D4F9D8 BE 60 34 34 02 mov     esi, offset gSkyTexNumber
				DWORD imm = pinst->detail->x86.operands[1].imm;

				ctx->candidate_gSkyTexNumber = imm;
				ctx->candidate_register = pinst->detail->x86.operands[0].reg;
				ctx->candidate_instcount = instCount;
			}
			else if (ctx->candidate_gSkyTexNumber &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == ctx->candidate_register &&
				pinst->detail->x86.operands[0].mem.disp == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				instCount < ctx->candidate_instcount + 5)
			{//text:01D4F9DD 39 3E cmp     [esi], edi
				gSkyTexNumber = (decltype(gSkyTexNumber))ctx->candidate_gSkyTexNumber;
			}
			else if (pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base != 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D5FC00 FF B6 80 69 00 08                                   push    gSkyTexNumber[esi]
				gSkyTexNumber = (decltype(gSkyTexNumber))pinst->detail->x86.operands[0].mem.disp;
			}

			if (gSkyTexNumber)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(gSkyTexNumber);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gRefFuncs.GL_Bind, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == X86_REG_EAX)
			{//.text:01D3C0C2 A3 40 22 E4 01 mov     currenttexture, eax
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				currenttexture = (decltype(currenttexture))imm;
			}

			if (currenttexture)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(currenttexture);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gRefFuncs.GL_SelectTexture, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == X86_REG_ESI)
			{//.text:01D4FE37 89 35 08 82 ED 01 mov     oldtarget, esi
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				oldtarget = (decltype(oldtarget))imm;
			}

			if (oldtarget)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(oldtarget);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "\xDB\x05\x2A\x2A\x2A\x2A\x2A\xD9\x1C\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x2A\xDB\x05";
		addr = (DWORD)Search_Pattern(sigs);
		Sig_AddrNotFound(gl_filter_min);
		gl_filter_min = *(int **)(addr + 2);
		gl_filter_max = *(int **)(addr + sizeof(sigs) - 1);
	}
	else
	{
		const char sigs[] = "\xDB\x05\x2A\x2A\x2A\x2A\x83\xC4\x04\xD9\x2A\x2A\x2A\x2A\x2A\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xDB\x05";
		addr = (DWORD)Search_Pattern(sigs);
		Sig_AddrNotFound(gl_filter_min);
		gl_filter_min = *(int **)(addr + 2);
		gl_filter_max = *(int **)(addr + sizeof(sigs) - 1);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gRefFuncs.R_TextureAnimation, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (!rtable &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_ESI &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D58B66 BE F0 5B 00 08 mov     esi, offset rtable

				rtable = (decltype(rtable))pinst->detail->x86.operands[1].imm;
			}

			if (rtable)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(rtable);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "\x68\x2A\x2A\x2A\x2A\xD9\x1D\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68";
		addr = (DWORD)Search_Pattern(sigs);
		Sig_AddrNotFound(r_refdef);
		vup = (vec_t *)(*(DWORD *)(addr + 1));
		vright = (vec_t *)(*(DWORD *)(addr + 18));
		vpn = (vec_t *)(*(DWORD *)(addr + 23));
		auto r_refdef_viewangles = (vec_t *)(*(DWORD *)(addr + 28));
		r_refdef = (refdef_t *)((char *)r_refdef_viewangles - offsetof(refdef_t, viewangles));
	}
	else 
	{
		const char sigs[] = "\x40\x68\x2A\x2A\x2A\x2A\xA3\x2A\x2A\x2A\x2A\xA1";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_SetupFrame, 0x300, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(r_refdef);
		addr += 2;
		vup = (vec_t *)(*(DWORD *)addr);
		addr += 10;
		addr += 5;
		vright = (vec_t *)(*(DWORD *)addr);
		addr += 5;
		vpn = (vec_t *)(*(DWORD *)addr);
		addr += 5;
		r_refdef = (refdef_t *)(*(DWORD *)addr - offsetof(refdef_t, viewangles));
		addr += 5;
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "\xD9\x54\x24\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD9\xE8";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_SetupGL, 0x100, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(r_xfov);
		r_xfov = *(decltype(r_xfov)*)(addr + 6);
	}
	else
	{
		const char sigs[] = "\x8B\x15\x2A\x2A\x2A\x2A\xD9\x5D\x2A\xDB\x05";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_SetupGL, 0x100, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(r_xfov);
		r_xfov = *(decltype(r_xfov)*)(addr + 2);
	}

	if (1)
	{
		const char sigs[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_SetupGL, 0x600, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(gWorldToScreen);
		gWorldToScreen = *(decltype(gWorldToScreen)*)(addr + 6);
		gScreenToWorld = *(decltype(gScreenToWorld)*)(addr + 1);
	}

	if (1)
	{
		const char sigs[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_RenderScene, 0x50, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(r_refdef_vrect);
		r_refdef_vrect = *(decltype(r_refdef_vrect)*)(addr + 1);
	}

	if (1)
	{
		PVOID R_RenderFinalFog = NULL;

		if (g_iEngineType == ENGINE_SVENGINE)
		{
			const char sigs[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x68\x60\x0B\x00\x00";
			addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_RenderScene, 0x600, sigs, sizeof(sigs) - 1);
			Sig_AddrNotFound(R_RenderFinalFog);

			R_RenderFinalFog = (decltype(R_RenderFinalFog))addr;

			g_bUserFogOn = *(int **)(addr + 2);
		}
		else
		{
			const char sigs[] = "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x6A\x00";
			addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_RenderScene, 0x600, sigs, sizeof(sigs) - 1);
			Sig_AddrNotFound(g_bUserFogOn);

			R_RenderFinalFog = (decltype(R_RenderFinalFog))GetCallAddress(addr + 9);

			g_bUserFogOn = *(int **)(addr + 1);
		}

		typedef struct
		{
			int pushvalue;
			DWORD candidate;
		}R_RenderFinalFog_ctx;

		R_RenderFinalFog_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(R_RenderFinalFog, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (R_RenderFinalFog_ctx *)context;

			if (ctx->candidate &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				pinst->detail->x86.operands[0].imm > 0x100)
			{
				int pushvalue = pinst->detail->x86.operands[0].imm;

				if (!g_UserFogDensity && pushvalue == GL_FOG_DENSITY)
				{
					g_UserFogDensity = (decltype(g_UserFogDensity))ctx->candidate;
					ctx->candidate = NULL;
				}
				else if (!g_UserFogColor && pushvalue == GL_FOG_COLOR)
				{
					g_UserFogColor = (decltype(g_UserFogColor))ctx->candidate;
					ctx->candidate = NULL;
				}
				else if (!g_UserFogStart && pushvalue == GL_FOG_START)
				{
					g_UserFogStart = (decltype(g_UserFogStart))ctx->candidate;
					ctx->candidate = NULL;
				}
				else if (!g_UserFogEnd && pushvalue == GL_FOG_END)
				{
					g_UserFogEnd = (decltype(g_UserFogEnd))ctx->candidate;
					ctx->candidate = NULL;
				}
			}

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{//.text:01D88E84 A1 E0 23 73 02                                      mov     eax, flFogDensity

				ctx->candidate = pinst->detail->x86.operands[1].mem.disp;
			}
			else if (pinst->id == X86_INS_FLD &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0)
			{//.text:01D88E84 A1 E0 23 73 02                                      mov     eax, flFogDensity

				ctx->candidate = pinst->detail->x86.operands[0].mem.disp;
			}
			else if (pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				)
			{//.text:01D88E84 A1 E0 23 73 02                                      mov     eax, flFogDensity

				ctx->candidate = pinst->detail->x86.operands[0].imm;
			}

			if (g_UserFogDensity && g_UserFogColor && g_UserFogStart && g_UserFogEnd)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(g_UserFogDensity);
		Sig_VarNotFound(g_UserFogColor);
		Sig_VarNotFound(g_UserFogStart);
		Sig_VarNotFound(g_UserFogEnd);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gRefFuncs.R_RenderScene, 0x30, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (address[0] == 0xE8)
			{//.text:01D58B66 BE F0 5B 00 08 mov     esi, offset rtable

				gRefFuncs.CL_IsDevOverviewMode = (decltype(gRefFuncs.CL_IsDevOverviewMode))pinst->detail->x86.operands[0].imm;
			}

			if (gRefFuncs.CL_IsDevOverviewMode)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_FuncNotFound(CL_IsDevOverviewMode);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gRefFuncs.R_CullBox, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_ESI &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				
				)
			{//BE 80 98 BC 02                                      mov     esi, offset frustum

				frustum = (decltype(frustum))pinst->detail->x86.operands[1].imm;
			}

			if (frustum)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(frustum);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs1[] = "\x8B\x15\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x1D";
		//Search in GL_LoadTexture2
		//.text : 01D4EBF4 8B 15 F0 C5 0F 03                                   mov     edx, numgltextures
		//.text : 01D4EBFA 3B F2                                               cmp     esi, edx
		//.text : 01D4EBFC 7D 4D                                               jge     short loc_1D4EC4B
		//.text : 01D4EBFE 8B 1D E4 C5 0F 03                                   mov     ebx, gltextures
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.GL_LoadTexture2, 0x300, sigs1, sizeof(sigs1) - 1);
		Sig_AddrNotFound(gltextures);
		numgltextures = *(int **)(addr + 2);
		gltextures_SvEngine = *(gltexture_t ***)(addr + 12);

		const char sigs2[] = "\x6B\xC1\x54\x89\x0D";
		//Search in GL_LoadTexture2
		//.text:01D4ED66 6B C1 54                                            imul    eax, ecx, 54h; 'T'
		//.text:01D4ED69 89 0D F0 C6 0F 03                                   mov     maxgltextures, ecx
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.GL_LoadTexture2, 0x300, sigs2, sizeof(sigs2) - 1);
		Sig_AddrNotFound(maxgltextures);
		maxgltextures_SvEngine = *(int **)(addr + 5);

		const char sigs3[] = "\x51\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)addr, 0x50, sigs3, sizeof(sigs3) - 1);
		Sig_AddrNotFound(realloc);
		gRefFuncs.realloc_SvEngine = (decltype(gRefFuncs.realloc_SvEngine))(addr + 2 + 4 + *(int *)(addr + 2));

		const char sigs4[] = "\x66\x8B\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x04";
		//66 8B 0D E0 72 40 08                                mov     cx, word ptr gHostSpawnCount
		//66 89 4B 04                                         mov     [ebx+4], cx
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.GL_LoadTexture2, 0x200, sigs4, sizeof(sigs4) - 1);
		Sig_AddrNotFound(gHostSpawnCount);
		gHostSpawnCount = *(int **)(addr + 3);

		const char sigs5[] = "\x03\x35\x2A\x2A\x2A\x2A\x3B\x15";
		//Search in GL_LoadTexture2
		//.text:01D4EDE8 03 35 EC C6 0F 03                                   add     esi, gltextures
		//.text : 01D4EDEE 3B 15 00 C7 0F 03                                   cmp     edx, peakgltextures
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.GL_LoadTexture2, 0x200, sigs5, sizeof(sigs5) - 1);
		Sig_AddrNotFound(peakgltextures);
		peakgltextures_SvEngine = *(int **)(addr + 8);
	}
	else
	{
		const char sigs1[] = "\x33\xF6\xBF\x2A\x2A\x2A\x2A\x3B\x35\x2A\x2A\x2A\x2A\x7D";
		//Search in GL_LoadTexture2
		//xor     esi, esi
		//mov     edi, offset gltextures
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.GL_LoadTexture2, 0x100, sigs1, sizeof(sigs1) - 1);
		Sig_AddrNotFound(gltextures);
		gltextures = *(gltexture_t **)(addr + 3);
		numgltextures = *(int **)(addr + 9);

		const char sigs2[] = "\x66\x8B\x15\x2A\x2A\x2A\x2A\x66\x89\x57\x04";
		//mov     dx, word ptr gHostSpawnCount
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.GL_LoadTexture2, 0x200, sigs2, sizeof(sigs2) - 1);
		Sig_AddrNotFound(gHostSpawnCount);
		gHostSpawnCount = *(int **)(addr + 3);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gRefFuncs.VID_UpdateWindowVars, 0x40, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG )
			{//.text:01D5F436 A3 BC 66 00 08                                      mov     window_rect, eax

				window_rect = (decltype(window_rect))pinst->detail->x86.operands[0].mem.disp;
			}

			if (window_rect)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(window_rect);
	}

	if (1)
	{
		const char sigs[] = "\x68\x00\xC0\x01\x00\x6A\x00";
		auto R_DecalInit = Search_Pattern(sigs);
		Sig_VarNotFound(R_DecalInit);

		g_pMetaHookAPI->DisasmRanges(R_DecalInit, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (!gDecalPool && pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//68 B8 5C 32 02 push    offset gDecalPool

				gDecalPool = (decltype(gDecalPool))pinst->detail->x86.operands[0].imm;
			}
			else if (!gDecalCache && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D49DBE B8 C0 96 BB 02 mov     eax, offset gDecalCache

				gDecalCache = (decltype(gDecalCache))pinst->detail->x86.operands[1].imm;
			}

			if (gDecalPool && gDecalCache)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(gDecalPool);
		Sig_VarNotFound(gDecalCache);
	}

	if (1)
	{
		typedef struct
		{
			int candidate_count;
			DWORD candidate[10];
		}R_DrawWorld_ctx;

		R_DrawWorld_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gRefFuncs.R_DrawWorld, 0x130, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (R_DrawWorld_ctx *)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//.text:01D49479 89 15 C0 96 BC 02                                   mov     modelorg, edx

				if (!ctx->candidate[ctx->candidate_count] && ctx->candidate_count < 10)
				{
					ctx->candidate[ctx->candidate_count] = pinst->detail->x86.operands[0].mem.disp;
					ctx->candidate_count ++;
				}
			}
			else if (pinst->id == X86_INS_FSTP &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D49479 89 15 C0 96 BC 02                                   mov     modelorg, edx

				if (!ctx->candidate[ctx->candidate_count] && ctx->candidate_count < 10)
				{
					ctx->candidate[ctx->candidate_count] = pinst->detail->x86.operands[0].mem.disp;
					ctx->candidate_count++;
				}
			}
			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		if (ctx.candidate_count >= 3)
		{
			std::qsort(ctx.candidate, ctx.candidate_count, sizeof(int), [](const void*a, const void*b) {
				return (int)(*(LONG_PTR *)a - *(LONG_PTR *)b);
			});

			//other, other, other, modelorg[0], modelorg[1], modelorg[2]
			if (ctx.candidate[ctx.candidate_count - 3] + 4 == ctx.candidate[ctx.candidate_count - 2] &&
				ctx.candidate[ctx.candidate_count - 2] + 4 == ctx.candidate[ctx.candidate_count - 1])
			{
				modelorg = (decltype(modelorg))ctx.candidate[ctx.candidate_count - 3];
			}
			//modelorg[0], modelorg[1], modelorg[2], other, other, other
			else if (ctx.candidate[0] + 4 == ctx.candidate[1] &&
				ctx.candidate[1] + 4 == ctx.candidate[2])
			{
				modelorg = (decltype(modelorg))ctx.candidate[0];
			}
		}

		Sig_VarNotFound(modelorg);
	}

	if (1)
	{
		typedef struct
		{
			int cmp_al_FF_instcount;
		}R_RenderDynamicLightmaps_ctx;

		R_RenderDynamicLightmaps_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gRefFuncs.R_RenderDynamicLightmaps, 0x150, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (R_RenderDynamicLightmaps_ctx *)context;

			if (!lightmap_polys &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.index != 0 &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.scale == 4 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == X86_REG_EAX)
			{//.text:01D58422 89 04 8D C8 B8 F5 03 mov     lightmap_polys[ecx*4], eax

				lightmap_polys = (decltype(lightmap_polys))pinst->detail->x86.operands[0].mem.disp;
			}
			else if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_AL &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0xFF )
			{//.text:01D47F46 3C FF   cmp     al, 0FFh

				ctx->cmp_al_FF_instcount = instCount;
			}
			else if (!d_lightstylevalue &&
				ctx->cmp_al_FF_instcount &&
				instCount < ctx->cmp_al_FF_instcount + 5 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.index != 0 &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.scale == 4 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D47F4F 8B 04 85 20 9A BC 02                                mov     eax, d_lightstylevalue[eax*4]

				d_lightstylevalue = (decltype(d_lightstylevalue))pinst->detail->x86.operands[1].mem.disp;
			}
			else if (!lightmap_modified &&				
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.index != 0 &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.scale == 4 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 1)
			{//.text:01D58489 C7 04 85 C8 C8 F5 03 01 00 00 00                    mov     lightmap_modified[eax*4], 1

				lightmap_modified = (decltype(lightmap_modified))pinst->detail->x86.operands[0].mem.disp;
			}

			if (d_lightstylevalue && lightmap_polys && lightmap_modified)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(d_lightstylevalue);
		Sig_VarNotFound(lightmap_polys);
		Sig_VarNotFound(lightmap_modified);
	}

	if (1)
	{
		typedef struct
		{
			int push28_instcount;
		}CL_AllocDlight_ctx;

		CL_AllocDlight_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gRefFuncs.CL_AllocDlight, 0x150, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;
			auto ctx = (CL_AllocDlight_ctx *)context;

			if (pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				pinst->detail->x86.operands[0].imm == 0x28)
			{
				ctx->push28_instcount = instCount;
			}
			else if (!cl_dlights &&
				ctx->push28_instcount &&
				instCount < ctx->push28_instcount + 3 &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize )
			{//.text:01D18B06 68 E0 95 D5 02 push    offset cl_dlight

				cl_dlights = (decltype(cl_dlights))pinst->detail->x86.operands[0].imm;
			}
			else if (ctx->push28_instcount &&
				instCount < ctx->push28_instcount + 8 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D18B16 8B 0D 78 9E BC 02 mov     ecx, r_dlightactive

				if(!r_dlightactive || (DWORD)pinst->detail->x86.operands[1].mem.disp > (DWORD)r_dlightactive)
					r_dlightactive = (decltype(r_dlightactive))pinst->detail->x86.operands[1].mem.disp;
			}
			else if (!r_dlightactive && 
				pinst->id == X86_INS_OR &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&

				pinst->detail->x86.operands[1].type == X86_OP_IMM && 
				pinst->detail->x86.operands[1].imm == 1 )
			{//.text:01D18B16 8B 0D 78 9E BC 02 mov     ecx, r_dlightactive

				r_dlightactive = (decltype(r_dlightactive))pinst->detail->x86.operands[0].mem.disp;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(cl_dlights);
		Sig_VarNotFound(r_dlightactive);
	}

	if (1)
	{
		typedef struct
		{
			PVOID base;
			size_t max_insts;
			int max_depth;
			std::set<PVOID> code;
			std::set<PVOID> branches;
			std::vector<walk_context_t> walks;

			int mov_38_instcount;
			DWORD decalsurf_candidate;
			int decalsurf_instcount;
			int decalsurf_register;
		}R_DrawSequentialPoly_ctx;

		R_DrawSequentialPoly_ctx ctx = { 0 };

		ctx.base = gRefFuncs.R_DrawSequentialPoly;
		ctx.max_insts = 500;
		ctx.max_depth = 16;
		ctx.walks.emplace_back(ctx.base, 0x500, 0);

		while (ctx.walks.size())
		{
			auto walk = ctx.walks[ctx.walks.size() - 1];
			ctx.walks.pop_back();

			g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn *)inst;
				auto ctx = (R_DrawSequentialPoly_ctx *)context;

				if (lightmap_textures && lightmap_rectchange && lightmaps && gDecalSurfs && gDecalSurfCount)
					return TRUE;

				if (ctx->code.size() > ctx->max_insts)
					return TRUE;

				if (ctx->code.find(address) != ctx->code.end())
					return TRUE;

				ctx->code.emplace(address);

				if (
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base != 0 &&
					pinst->detail->x86.operands[1].mem.disp == 0x38)
				{//.text:01D47A28 8B 53 38 mov     edx, [ebx+38h]

					ctx->mov_38_instcount = instCount;
				}
				else if (!lightmap_textures &&
					ctx->mov_38_instcount &&
					instCount < ctx->mov_38_instcount + 3 &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.index != 0 &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					pinst->detail->x86.operands[1].mem.scale == 4 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//.text:01D47A2B 8B 04 95 00 32 34 02                                mov     eax, lightmap_textures[edx*4]

					if ((DWORD)pinst->detail->x86.operands[1].mem.disp != (DWORD)lightmap_modified)
						lightmap_textures = (decltype(lightmap_textures))pinst->detail->x86.operands[1].mem.disp;
				}
				else if (!lightmap_textures &&
					ctx->mov_38_instcount &&
					instCount < ctx->mov_38_instcount + 3 &&
					pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.index != 0 &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.scale == 4 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//.text:01D5918C FF 34 85 C0 44 F5 03                                push    lightmap_textures[eax*4] 

					lightmap_textures = (decltype(lightmap_textures))pinst->detail->x86.operands[0].mem.disp;
				}
				else if (!lightmap_rectchange &&
					ctx->mov_38_instcount &&
					instCount < ctx->mov_38_instcount + 15 &&
					pinst->id == X86_INS_ADD &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_ESI &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[1].imm >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//.text:01D591B8 81 C6 C8 D8 F5 03                                   add     esi, offset lightmap_rectchange

					lightmap_rectchange = (decltype(lightmap_rectchange))pinst->detail->x86.operands[1].imm;
				}
				else if (lightmap_rectchange && !lightmaps &&
					ctx->mov_38_instcount &&
					instCount < ctx->mov_38_instcount + 20 &&
					pinst->id == X86_INS_ADD &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[1].imm >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//.text:01D591C9 81 C1 C8 18 FE 03                                   add     ecx, offset lightmaps 

					lightmaps = (decltype(lightmaps))pinst->detail->x86.operands[1].imm;
				}
				else if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index != 0 &&
					pinst->detail->x86.operands[0].mem.scale == 4 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_REG  )
				{//89 3C 85 F8 8C FE 07 mov     gDecalSurfs[eax*4], edi

					ctx->decalsurf_candidate = (decltype(ctx->decalsurf_candidate))pinst->detail->x86.operands[0].mem.disp;
					ctx->decalsurf_instcount = instCount;
					ctx->decalsurf_register = pinst->detail->x86.operands[0].mem.index;
				}
				else if (!gDecalSurfs &&
					ctx->decalsurf_candidate &&
					instCount < ctx->decalsurf_instcount + 3 &&
					pinst->id == X86_INS_INC &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == ctx->decalsurf_register)
				{//.text:01D47C0F 40                                                  inc     eax

					gDecalSurfs = (decltype(gDecalSurfs))ctx->decalsurf_candidate;
				}
				else if (!gDecalSurfCount &&
					ctx->decalsurf_candidate &&
					instCount < ctx->decalsurf_instcount + 5 &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == 0 &&
					pinst->detail->x86.operands[0].mem.scale == 1 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].reg == ctx->decalsurf_register)
				{//.text:01D47C15 A3 98 26 34 02                                      mov     gDecalSurfCount, eax

					gDecalSurfCount = (decltype(gDecalSurfCount))pinst->detail->x86.operands[0].mem.disp;
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

		Sig_VarNotFound(lightmap_textures);
		Sig_VarNotFound(lightmap_rectchange);
		Sig_VarNotFound(lightmaps);
		Sig_VarNotFound(gDecalSurfs);
		Sig_VarNotFound(gDecalSurfCount);
	}

	if (1)
	{
		typedef struct
		{
			PVOID base;
			size_t max_insts;
			int max_depth;
			std::set<PVOID> code;
			std::set<PVOID> branches;
			std::vector<walk_context_t> walks;

			int ambient_instcount;
			int lightvec_start;
			PVOID plightvec_cached;
			DWORD lightvec_candidate;
			int lightvec_instcount;
		}R_StudioLighting_ctx;

		R_StudioLighting_ctx ctx = { 0 };

		ctx.base = gRefFuncs.R_StudioLighting;
		ctx.max_insts = 500;
		ctx.max_depth = 16;
		ctx.walks.emplace_back(ctx.base, 0x500, 0);

		while (ctx.walks.size())
		{
			auto walk = ctx.walks[ctx.walks.size() - 1];
			ctx.walks.pop_back();

			g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn *)inst;
				auto ctx = (R_StudioLighting_ctx *)context;

				if(r_ambientlight && r_shadelight && r_blightvec && r_plightvec && lightgammatable)
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
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//.text:01D83B54 DB 05 E8 95 75 02                                   fild    r_ambientlight

					r_ambientlight = (decltype(r_ambientlight))pinst->detail->x86.operands[0].mem.disp;
					ctx->ambient_instcount = instCount;
				}
				else if (!r_shadelight &&
					ctx->ambient_instcount &&
					instCount < ctx->ambient_instcount + 10 &&
					pinst->id == X86_INS_FLD &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//.text:01D8A986 D9 05 60 61 52 08                                   fld     r_shadelight

					r_shadelight = (decltype(r_shadelight))pinst->detail->x86.operands[0].mem.disp;
				}
				else if (
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
				else if (!r_blightvec &&
					ctx->lightvec_start == 2 &&
					pinst->id == X86_INS_FLD &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//.text:01D83BDC D9 80 48 04 79 02                                   fld     (r_blightvec+8)[eax]

					ctx->lightvec_candidate = pinst->detail->x86.operands[0].mem.disp;
					ctx->lightvec_instcount = instCount;
				}
				else if (!r_blightvec &&
					ctx->lightvec_start == 2 &&
					ctx->lightvec_instcount &&
					instCount < ctx->lightvec_instcount + 2 &&
					pinst->id == X86_INS_FMUL &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0)
				{//.text:01D83BE2 D8 49 08                                            fmul    dword ptr [ecx+8]
					r_blightvec = (decltype(r_blightvec))(ctx->lightvec_candidate - pinst->detail->x86.operands[0].mem.disp);
				}
				else if (!r_plightvec &&
					ctx->lightvec_start == 1 &&
					pinst->id == X86_INS_FLD &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//.text:01D83BDC D9 80 48 04 79 02                                   fld     (r_blightvec+8)[eax]

					ctx->lightvec_candidate = pinst->detail->x86.operands[0].mem.disp;
					ctx->lightvec_instcount = instCount;
				}
				else if (!r_plightvec &&
					ctx->lightvec_start == 1 &&
					ctx->lightvec_instcount &&
					instCount < ctx->lightvec_instcount + 2 &&
					pinst->id == X86_INS_FMUL &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0)
				{//.text:01D83BE2 D8 49 08                                            fmul    dword ptr [ecx+8]
					r_plightvec = (decltype(r_plightvec))(ctx->lightvec_candidate - pinst->detail->x86.operands[0].mem.disp);
				}
				else if (!memcmp(address, "\xC1\xE0\x02\xB9", 4))
				{
				//.text:01D8AA7C C1 E0 02                                            shl     eax, 2
				//.text : 01D8AA7F B9 B0 F7 67 09                                      mov     ecx, offset lightgammatable

					lightgammatable = *(decltype(lightgammatable) *)(address + 4);

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
							ctx->walks.emplace_back(imm, 0x300, depth + 1);

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

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define NORMALINDEX_SIG_SVENGINE "\x83\x3C\xB5\x2A\x2A\x2A\x2A\x00"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_GLStudioDrawPoints, 0x600, NORMALINDEX_SIG_SVENGINE, sizeof(NORMALINDEX_SIG_SVENGINE) - 1);
		Sig_AddrNotFound(g_NormalIndex);
		g_NormalIndex = *(decltype(g_NormalIndex) *)(addr + 3);
	}
	else
	{	
		//mov     edi, offset g_NormalIndex
		//rep stosd
#define G_NORMALINDEX_SIG "\xBF\x2A\x2A\x2A\x2A\xF3\xAB"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.BuildNormalIndexTable, 0x50, G_NORMALINDEX_SIG, sizeof(G_NORMALINDEX_SIG) - 1);
		Sig_AddrNotFound(g_NormalIndex);
		g_NormalIndex = *(int(**)[MAXSTUDIOVERTS])(addr + 1);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CHROMEAGE_SIG_SVENGINE "\xBF\x2A\x2A\x2A\x2A\xF3\xAB\x33\xFF\x39"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_GLStudioDrawPoints, 0x600, CHROMEAGE_SIG_SVENGINE, sizeof(CHROMEAGE_SIG_SVENGINE) - 1);
		Sig_AddrNotFound(chromeage);
		chromeage = *(decltype(chromeage) *)(addr + 1);
	}
	else
	{
#define CHROMEAGE_SIG "\x8B\x04\xB5\x2A\x2A\x2A\x2A\x3B\xC1"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_StudioChrome, 0x50, CHROMEAGE_SIG, sizeof(CHROMEAGE_SIG) - 1);
		Sig_AddrNotFound(chromeage);
		chromeage = *(int(**)[MAXSTUDIOBONES])((DWORD)addr + 3);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CHROME_SIG_SVENGINE "\xC1\xE8\x1F\x03\xC2\x8D\x04\xC5\x2A\x2A\x2A\x2A\x50\xE8"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_GLStudioDrawPoints, 0x1000, CHROME_SIG_SVENGINE, sizeof(CHROME_SIG_SVENGINE) - 1);
		Sig_AddrNotFound(chrome);
		chrome = *(decltype(chrome) *)(addr + 8);
	}
	else
	{
#define CHROME_SIG_NEW "\x8D\x0C\xD5\x2A\x2A\x2A\x2A\x51\xE8"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_GLStudioDrawPoints, 0x600, CHROME_SIG_NEW, sizeof(CHROME_SIG_NEW) - 1);
		Sig_AddrNotFound(chrome);
		chrome = *(decltype(chrome) *)(addr + 3);
	}

#define GWATERCOLOR_SIG_SVENGINE "\xDB\x05\x2A\x2A\x2A\x2A\x68\x01\x26\x00\x00\x68\x65\x0B\x00\x00"
	auto gWaterColor_Pattern = Search_Pattern(GWATERCOLOR_SIG_SVENGINE);
	Sig_VarNotFound(gWaterColor_Pattern);
	gWaterColor = *(decltype(gWaterColor) *)((PUCHAR)gWaterColor_Pattern + 2);
	cshift_water = (decltype(cshift_water))((PUCHAR)gWaterColor + 12);

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gRefFuncs.GL_EnableMultitexture, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (!gl_mtexable &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{//01D57970 83 3D 80 66 00 08 00                                cmp     gl_mtexable, 0
				gl_mtexable = (decltype(gl_mtexable))pinst->detail->x86.operands[0].mem.disp;
			}
			else if (!gl_mtexable &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//.text:01D478D0 A1 44 34 34 02                                      mov     eax, gl_mtexable
				gl_mtexable = (decltype(gl_mtexable))pinst->detail->x86.operands[1].mem.disp;
			}
			else if (!mtexenabled &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 1)
			{//.text:01D478F2 C7 05 10 33 34 02 01 00 00 00                       mov     mtexenabled, 1
				mtexenabled = (decltype(mtexenabled))pinst->detail->x86.operands[0].mem.disp;
			}

			if (gl_mtexable && mtexenabled)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(gl_mtexable);
		Sig_VarNotFound(mtexenabled);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "\xFF\x35\x2A\x2A\x2A\x2A\xDC\x0D\x2A\x2A\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A\xE8";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_RenderView_SvEngine, 0x800, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(c_brush_polys);
		c_alias_polys = *(int **)(addr + 2);
		c_brush_polys = *(int **)(addr + 14);
	}
	else
	{
		const char sigs[] = "\xA1\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x50\x51";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_RenderView, 0x150, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(c_brush_polys);
		c_alias_polys = *(int **)(addr + 1);
		c_brush_polys = *(int **)(addr + 7);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "\xF6\xC4\x44\x0F\x2A\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_RenderView_SvEngine, 0x400, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(envmap);
		envmap = *(int **)(addr + 11);
		cl_stats = *(int **)(addr + 24);

		const char sigs3[] = "\xD9\x1D\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x89\x81\xDC\x02\x00\x00";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_RenderView_SvEngine, 0x600, sigs3, sizeof(sigs3) - 1);
		Sig_AddrNotFound(cl_weaponstarttime);
		cl_weaponstarttime = *(float **)(addr + 2);
		cl_weaponsequence = *(int **)(addr + 7);

		const char sigs4[] = "\xD1\xEA\x89\x15\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_RenderView_SvEngine, 0x600, sigs4, sizeof(sigs4) - 1);
		Sig_AddrNotFound(cl_light_level);
		cl_light_level = *(int **)(addr + 4);
	}
	else
	{
		const char sigs[] = "\x39\x3D\x2A\x2A\x2A\x2A\x0F\x2A\x2A\x2A\x2A\x2A\xD9\x05";
		addr = (DWORD)Search_Pattern(sigs);
		Sig_AddrNotFound(envmap);
		envmap = *(int **)(addr + 2);

		const char sigs2[] = "\x39\x3D\x2A\x2A\x2A\x2A\x0F\x2A\x2A\x2A\x2A\x2A\xA1";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)addr, 0x50, sigs2, sizeof(sigs2) - 1);
		Sig_AddrNotFound(cl_stats);
		cl_stats = *(int **)(addr + 2);

		const char sigs3[] = "\xD9\x1D\x2A\x2A\x2A\x2A\xA1";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)addr, 0x500, sigs3, sizeof(sigs3) - 1);
		Sig_AddrNotFound(cl_weaponstarttime);
		cl_weaponstarttime = *(float **)(addr + 2);
		cl_weaponsequence = *(int **)(addr + 7);

		const char sigs4[] = "\x6A\x01\x89\x15\x2A\x2A\x2A\x2A\x89";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)addr, 0x600, sigs4, sizeof(sigs4) - 1);
		Sig_AddrNotFound(cl_light_level);
		cl_light_level = *(int **)(addr + 4);

	}

#define R_WORLD_MATRIX_SIG_SVENGINE "\x68\x2A\x2A\x2A\x2A\x68\xA6\x0B\x00\x00"
	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_SetupGL, 0x600, R_WORLD_MATRIX_SIG_SVENGINE, sizeof(R_WORLD_MATRIX_SIG_SVENGINE) - 1);
	Sig_AddrNotFound(r_world_matrix);
	r_world_matrix = *(float **)(addr + 1);

#define R_PROJ_MATRIX_SIG_SVENGINE "\x68\x2A\x2A\x2A\x2A\x68\xA7\x0B\x00\x00"
	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_SetupGL, 0x500, R_PROJ_MATRIX_SIG_SVENGINE, sizeof(R_PROJ_MATRIX_SIG_SVENGINE) - 1);
	Sig_AddrNotFound(r_projection_matrix);
	r_projection_matrix = *(float **)(addr + 1);

#define TMP_PALETTE_SVENGINE "\x68\x2A\x2A\x2A\x2A\x6A\x00\x6A\x00"
	addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gRefFuncs.R_StudioSetupSkin, 0x600, TMP_PALETTE_SVENGINE, sizeof(TMP_PALETTE_SVENGINE) - 1);
	Sig_AddrNotFound(tmp_palette);
	tmp_palette = *(void **)(addr + 1);

	//get cl. vars by offsets
	cl_waterlevel = cl_parsecount + 274;
	//cl_time = (double *)(cl_waterlevel + 11);
	//cl_oldtime = cl_time + 1;	

#define MOD_KNOWN_SIG "\xB8\x9D\x82\x97\x53\x81\xE9"

	{
		DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)g_dwEngineTextBase, g_dwEngineTextSize, MOD_KNOWN_SIG, sizeof(MOD_KNOWN_SIG) - 1);
		Sig_AddrNotFound(mod_known);
		mod_known = *(model_t **)(addr + 7);
	}

	{
		const char sigs1[] = "Cached models:\n";
		auto Mod_Print_String = Search_Pattern_Data(sigs1);
		if (!Mod_Print_String)
			Mod_Print_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Mod_Print_String);
		char pattern[] = "\x57\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD *)(pattern + 2) = (DWORD)Mod_Print_String;
		auto Mod_Print_Call = Search_Pattern(pattern);
		Sig_VarNotFound(Mod_Print_Call);

		g_pMetaHookAPI->DisasmRanges(Mod_Print_Call, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{//A1 84 5C 32 02 mov     eax, mod_numknown
				DWORD imm = pinst->detail->x86.operands[1].mem.disp;

				mod_numknown = (decltype(mod_numknown))imm;
			}
			else if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//39 3D 44 32 90 03 cmp     mod_numknown, edi
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				mod_numknown = (decltype(mod_numknown))imm;
			}

			if (mod_numknown)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);
	}

	{
		const char sigs1[] = "bogus\0";
		auto Bogus_String = Search_Pattern_Data(sigs1);
		if (!Bogus_String)
			Bogus_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Bogus_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\xE8";
		*(DWORD *)(pattern + 1) = (DWORD)Bogus_String;
		auto Bogus_Call = Search_Pattern(pattern);
		Sig_VarNotFound(Bogus_Call);

		gRefFuncs.Mod_LoadStudioModel = (decltype(gRefFuncs.Mod_LoadStudioModel))g_pMetaHookAPI->ReverseSearchFunctionBegin(Bogus_Call, 0x50);
		Sig_FuncNotFound(Mod_LoadStudioModel);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gRefFuncs.Draw_DecalTexture, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (!decal_wad &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				decal_wad = (decltype(decal_wad))pinst->detail->x86.operands[0].mem.disp;
			}
			else if (!decal_wad &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				decal_wad = (decltype(decal_wad))pinst->detail->x86.operands[1].mem.disp;
			}
			if (decal_wad)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(decal_wad);
	}

	//Allocate 32bytes instead of 28 bytes for mspriteframe_t
#define Mod_LoadSpriteFrame_Sig "\x6A\x1C\x89\x44\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x6A\x1C"
	PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
	PUCHAR SearchLimit = (PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize;
	while (SearchBegin < SearchLimit)
	{
		auto pFound = (PUCHAR)g_pMetaHookAPI->SearchPattern(SearchBegin, SearchLimit - SearchBegin, Mod_LoadSpriteFrame_Sig, sizeof(Mod_LoadSpriteFrame_Sig) - 1);
		if (pFound)
		{
			g_pMetaHookAPI->WriteBYTE(pFound + 1, 0x20);
			g_pMetaHookAPI->WriteBYTE(pFound + 12, 0x20);

			SearchBegin = pFound + sizeof(Mod_LoadSpriteFrame_Sig) - 1;
		}
		else
		{
			break;
		}
	}
}

void R_InstallHook(void)
{
	Install_InlineHook(GL_BeginRendering);
	Install_InlineHook(GL_EndRendering);

	if (gRefFuncs.R_RenderView_SvEngine)
	{
		Install_InlineHook(R_RenderView_SvEngine);
	}
	else
	{
		Install_InlineHook(R_RenderView);
	}

	Install_InlineHook(R_DrawWorld);
	Install_InlineHook(R_DrawSpriteModel);	
	Install_InlineHook(R_NewMap);
	Install_InlineHook(R_SetupGL);
	Install_InlineHook(R_ForceCVars);
	Install_InlineHook(R_CullBox);
	Install_InlineHook(Mod_PointInLeaf);
	Install_InlineHook(R_BuildLightMap);
	Install_InlineHook(R_AddDynamicLights);
	Install_InlineHook(R_GLStudioDrawPoints);
	Install_InlineHook(R_DrawBrushModel);
	Install_InlineHook(R_DrawTEntitiesOnList);
	Install_InlineHook(R_AddTEntity);
	Install_InlineHook(GL_LoadTexture2);
	Install_InlineHook(enginesurface_drawFlushText);
	Install_InlineHook(Mod_LoadStudioModel);


}