
#include <metahook.h>
#include <capstone.h>
#include <set>
#include <map>
#include "gl_local.h"

//#define R_LOADSKYNAME_SIG_SVENGINE "\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x84\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x3D"

#define MOD_POINTINLEAF_SIG_SVENGINE "\x2A\x8B\x2A\x24\x2A\x85\x2A\x2A\x2A\x8B\x2A\xA4\x00\x00\x00"
#define MOD_POINTINLEAF_SIG_HL25 "\x55\x8B\xEC\x56\x8B\x75\x0C\x85\x2A\x2A\x2A\x8B\x8E\xA4\x00\x00\x00\x85\xC9\x2A\x2A\x68"
#define MOD_POINTINLEAF_SIG_NEW "\x55\x8B\xEC\x2A\x8B\x2A\x0C\x85\x2A\x74\x2A\x8B\x2A\xA4\x00\x00\x00"
#define MOD_POINTINLEAF_SIG_BLOB "\x56\x8B\x74\x24\x0C\x85\xF6\x74\x2A\x8B\x86\xA4\x00\x00\x00\x85\xC0"

#define BUILDGAMMATABLE_SIG_SVENGINE "\x83\xEC\x2A\x6A\x05\xE8\x2A\x2A\x2A\x2A\xD9\xEE"
#define BUILDGAMMATABLE_SIG_HL25 "\x55\x8B\xEC\xF3\x0F\x10\x45\x08\x83\xEC\x10\x0F\x57\xDB\x0F\x2E\xC3\x9F"
#define BUILDGAMMATABLE_SIG_NEW "\x55\x8B\xEC\x83\xEC\x2A\xD9\x45\x08\xDC\x1D\x2A\x2A\x2A\x2A\xDF\xE0"
#define BUILDGAMMATABLE_SIG_BLOB "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x1C\x53\x56\x57\xE8\x2A\x2A\x2A\x2A\x85\xC0"

#define R_DRAWPARTICLES_SIG_SVENGINE "\x81\xEC\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x00\x00\x00\x2A\xFF\x35"
#define R_DRAWPARTICLES_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x2A\xFF\x35\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xC0\x0B\x00\x00"
#define R_DRAWPARTICLES_SIG_NEW "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xC0\x0B\x00\x00"
#define R_DRAWPARTICLES_SIG_NEW2 R_DRAWPARTICLES_SIG_BLOB
#define R_DRAWPARTICLES_SIG_BLOB "\x83\xEC\x40\xA1\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xC0\x0B\x00\x00"
#define R_DRAWPARTICLES_SIG_COMMON "\x83\xC4\x04\x68\xC0\x0B\x00\x00"

#define R_POLYBLEND_SVENGINE "\x55\x8B\xEC\x83\xE4\xC0\x83\xEC\x34\x2A\x2A\x2A\xE8"
#define R_POLYBLEND_HL25 "\x55\x8B\xEC\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B\xD8\x85\xDB\x0F\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\xC0\x0B\x00\x00"
#define R_POLYBLEND_NEW "\x55\x8B\xEC\x83\xEC\x0C\x2A\xE8\x2A\x2A\x2A\x2A\x8B\x2A\x85\x2A\x0F\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\xC0\x0B\x00\x00"
#define R_POLYBLEND_NEW2 "\x55\x8B\xEC\x83\x2A\x2A\x83\xEC\x0C\x2A\xE8\x2A\x2A\x2A\x2A\x8B\x2A\x85\x2A\x0F\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\xC0\x0B\x00\x00"
#define R_POLYBLEND_BLOB "\x55\x8B\xEC\x83\xE4\x2A\x83\xEC\x0C\x2A\xE8\x2A\x2A\x2A\x2A\x8B\x2A\x85\x2A\x0F"

#define S_EXTRAUPDATE_SVENGINE "\xE8\x2A\x2A\x2A\x2A\x85\xC0\x75\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xD9\x05"
#define S_EXTRAUPDATE_HL25 "\xE8\x2A\x2A\x2A\x2A\x85\xC0\x75\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xF3\x0F"
#define S_EXTRAUPDATE_NEW S_EXTRAUPDATE_SVENGINE
#define S_EXTRAUPDATE_NEW2 S_EXTRAUPDATE_BLOB
#define S_EXTRAUPDATE_BLOB "\xE8\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x2A\x2A\xE9\x2A\x2A\x2A\x2A\xC3"

#define R_ROTATEFORENTITY_SVENGINE "\x83\xEC\x2A\x8B\x2A\x24\x2A\x8B\x2A\x24\x2A\xD9\x00"
#define R_ROTATEFORENTITY_HL25     "\x55\x8B\xEC\x83\xEC\x20\x8B\x45\x08\x8B\x08\x8B\x50\x04\x8B\x40\x08"
#define R_ROTATEFORENTITY_NEW      "\x55\x8B\xEC\x83\xEC\x20\x8B\x45\x08\x8B\x08\x8B\x50\x04\x8B\x40\x08"
#define R_ROTATEFORENTITY_BLOB     "\x83\xEC\x20\x8B\x44\x24\x24\x8B\x08\x8B\x50\x04\x8B\x40\x08"
#define R_ROTATEFORENTITY_GOLDSRC  "\xFF\x15\x2A\x2A\x2A\x2A\x8D\x2A\x48\x0B\x00\x00\x2A\x2A\xE8"

#define R_DECALSHOTINTERNAL_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\x8B\x54\x24\x2A\x8B\x4C\x24\x2A\x53\x8B\x5C\x24\x2A\x56\x69\xF2\xB8\x0B\x00\x00"

#define R_RENDERSCENE_SIG_BLOB "\x51\xDB\x05\x2A\x2A\x2A\x2A\xD9\x5C\x24\x00\xD9\x05\x2A\x2A\x2A\x2A\xD8\x5C\x24\x00\xDF\xE0\xF6\xC4\x2A\x2A\x2A\xA1"
#define R_RENDERSCENE_SIG_NEW "\xE8\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\xE8\x2A\x2A\x2A\x2A\xE8"
#define R_RENDERSCENE_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\xE8\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x33\xC0"
#define R_RENDERSCENE_SIG_SVENGINE ""

#define R_NEWMAP_SIG_COMMON    "\x55\x8B\xEC\x83\xEC\x2A\xC7\x45\xFC\x00\x00\x00\x00\x2A\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC"
#define R_NEWMAP_SIG_BLOB      R_NEWMAP_SIG_COMMON
#define R_NEWMAP_SIG_NEW       R_NEWMAP_SIG_COMMON
#define R_NEWMAP_SIG_HL25      R_NEWMAP_SIG_COMMON
#define R_NEWMAP_SIG_SVENGINE "\x55\x8B\xEC\x51\xC7\x45\xFC\x00\x00\x00\x00\xEB\x2A\x8B\x45\xFC\x83\xC0\x01\x89\x45\xFC\x81\x7D\xFC\x00\x01\x00\x00"

#define GL_BUILDLIGHTMAPS_SIG_BLOB     "\x53\x56\x57\x68\x00\x80\x00\x00\x6A\x00\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1"
#define GL_BUILDLIGHTMAPS_SIG_NEW      "\x55\x8B\xEC\x2A\x2A\x2A\x2A\x68\x00\x80\x00\x00\x6A\x00\x68"
#define GL_BUILDLIGHTMAPS_SIG_HL25     "\x55\x8B\xEC\x83\xEC\x2A\xE8\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\xF3\x0F\x2A\x2A\x2A\x2A\x2A\x2A\x0F"
#define GL_BUILDLIGHTMAPS_SIG_SVENGINE "\x83\xEC\x24\x2A\x2A\x2A\x2A\x68\x00\x00\x08\x00\x6A\x00\x68"

#define GL_UNLOADTEXTURE_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x53\x56\x57\x33\xFF\xBE\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\x8B\x5D\x08"
#define GL_UNLOADTEXTURE_SIG_HL25 "\x55\x8B\xEC\x2A\x2A\x33\xFF\xBE\x2A\x2A\x2A\x2A\x39\x2A\x2A\x2A\x2A\x2A\x0F\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x2A\x08"
#define GL_UNLOADTEXTURE_SIG_SVENGINE "\x56\x33\xF6\x39\x35\x2A\x2A\x2A\x2A\x2A\x2A\x55\x8B\x2A\x2A\x0C\x57"

#define R_DRAWWORLD_SIG "\x81\xEC\xB8\x0B\x00\x00\x68\xB8\x0B\x00\x00\x8D\x44\x24\x04\x6A\x00\x50\xE8"
#define R_DRAWWORLD_SIG_NEW "\x55\x8B\xEC\x81\xEC\xB8\x0B\x00\x00\x68\xB8\x0B\x00\x00\x8D\x85\x48\xF4\xFF\xFF\x6A\x00\x50\xE8\x2A\x2A\x2A\x2A\x8B\x0D"
#define R_DRAWWORLD_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x2A\x0B\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x68\xB8\x0B\x00\x00"//valve's 9891 update fucked this
#define R_DRAWWORLD_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\xB8\x0B\x00\x00\xD9\x05"
#define R_DRAWWORLD_SIG_COMMON "\x68\xB8\x0B\x00\x00\x8D"

#define R_MARKLEAVES_SIG_BLOB "\xB8\x00\x10\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\xA1"
#define R_MARKLEAVES_SIG_NEW2 R_MARKLEAVES_SIG_BLOB
#define R_MARKLEAVES_SIG_NEW "\x55\x8B\xEC\xB8\x04\x10\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\xA1"
#define R_MARKLEAVES_SIG_HL25 "\x55\x8B\xEC\xB8\x2A\x2A\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x8B\x0D\x2A\x2A\x2A\x2A\x0F\x57\xC9"
#define R_MARKLEAVES_SIG_SVENGINE "\xB8\x2A\x2A\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\x8B\x0D\x2A\x2A\x2A\x2A\xD9\x05"

#define R_CULLBOX_SIG_BLOB "\x53\x8B\x5C\x24\x08\x56\x57\x8B\x7C\x24\x14\xBE\x2A\x2A\x2A\x2A\x56\x57\x53\xE8"
#define R_CULLBOX_SIG_NEW2 R_CULLBOX_SIG_BLOB
#define R_CULLBOX_SIG_NEW "\x55\x8B\xEC\x53\x8B\x5D\x08\x56\x57\x8B\x7D\x0C\xBE\x2A\x2A\x2A\x2A\x56\x57\x53"
#define R_CULLBOX_SIG_HL25 "\x55\x8B\xEC\x2A\x8B\x2A\x08\x2A\x2A\x8B\x2A\x0C\xBE\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C\x83\xF8\x02"
#define R_CULLBOX_SIG_SVENGINE "\x2A\x8B\x2A\x24\x08\x2A\x2A\x8B\x2A\x24\x14\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C\x83\xF8\x02"

#define R_SETUPFRAME_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x83\xEC\x18\x83\xF8\x01\x0F\x8E\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x2A\x2A\x68"
#define R_SETUPFRAME_SIG_BLOB2 "\x8B\x0D\x2A\x2A\x2A\x2A\x83\xEC\x18\x33\xC0\x83\xF9\x01\x0F\x9F\xC0\x50\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1"
#define R_SETUPFRAME_SIG_NEW "\x55\x8B\xEC\x83\xEC\x18\x8B\x0D\x2A\x2A\x2A\x2A\x33\xC0\x83\xF9\x01\x0F\x9F\xC0\x50\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1"
#define R_SETUPFRAME_SIG_HL25 ""     //inlined
#define R_SETUPFRAME_SIG_SVENGINE "" //inlined

#define GL_BIND_SIG_BLOB      "\x8B\x44\x24\x04\x8B\x0D\x2A\x2A\x2A\x2A\x2A\x8B\x2A\x2A\x2A\x10\x25\xFF\xFF\x00\x00"
#define GL_BIND_SIG_NEW2      GL_BIND_SIG_BLOB
#define GL_BIND_SIG_NEW       "\x55\x8B\xEC\x8B\x45\x08\x8B\x0D\x2A\x2A\x2A\x2A\x56\x8B\xF0\xC1\xFE\x10\x25\xFF\xFF\x00\x00\x4E\x3B\xC8"
#define GL_BIND_SIG_HL25      "\x55\x8B\xEC\x8B\x45\x08\x56\x8B\xF0\x0F\xB7\xC0\xC1\xFE\x10\x4E\x39"
#define GL_BIND_SIG_SVENGINE  "\x8B\x44\x24\x04\x39\x05\x2A\x2A\x2A\x2A\x2A\x2A\x50\x68\xE1\x0D\x00\x00\xA3\x2A\x2A\x2A\x2A\xFF\x15\x2A\x2A\x2A\x2A\xC3"

#define GL_SELECTTEXTURE_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x56\x85\xC0\x2A\x2A\x8B\x74\x24\x08\x56\xFF\x15\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x3B\xF0"
#define GL_SELECTTEXTURE_SIG_NEW2 GL_SELECTTEXTURE_SIG_BLOB
#define GL_SELECTTEXTURE_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x56\x85\xC0\x2A\x2A\x8B\x75\x08\x56\xFF\x15\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x3B\xF0"
#define GL_SELECTTEXTURE_SIG_HL25 "\x55\x8B\xEC\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x56\x8B\x75\x08\x56\xFF\x15"
#define GL_SELECTTEXTURE_SIG_SVENGINE "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x2A\x2A\x2A\x24\x08\x2A\xFF\x15\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\xA1"

#define GL_LOADTEXTURE_SIG "\xA1\x2A\x2A\x2A\x2A\x8B\x4C\x24\x20\x8B\x54\x24\x1C\x50\x8B\x44\x24\x1C\x51\x8B\x4C\x24\x1C\x52\x8B\x54\x24\x1C\x50\x8B\x44\x24\x1C"
#define GL_LOADTEXTURE_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x8B\x4D\x24\x8B\x55\x20\x50\x8B\x45\x1C\x51\x8B\x4D\x18\x52\x8B\x55\x14\x50\x8B\x45\x10\x51"

#define GL_LOADTEXTURE2_SIG_BLOB "\xB8\x0C\x40\x00\x00\xE8\x2A\x2A\x2A\x2A\x53\x8B\x9C\x24\x14\x40\x00\x00\x55\x56\x8A\x03\x33\xF6"
#define GL_LOADTEXTURE2_SIG_BLOB2 "\xB8\x0C\x40\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x84\x24\x10\x40\x00\x00\x53\x55\x33\xDB\x8A\x08\x56\x84\xC9\x57\x89\x5C\x24\x14"
#define GL_LOADTEXTURE2_SIG_NEW "\x55\x8B\xEC\xB8\x0C\x40\x00\x00\xE8\x2A\x2A\x2A\x2A\x8B\x45\x08\x53\x33\xDB\x56\x8A\x08\x57\x84\xC9\x89\x5D\xF4\x74\x2A\x33\xF6"
#define GL_LOADTEXTURE2_SIG_HL25 "\x55\x8B\xEC\xB8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x8B\x45\x08\x2A\x89\x85\x2A\x2A\x2A\x2A\x33\xDB\x8B\x45\x18"
#define GL_LOADTEXTURE2_SIG_SVENGINE "\x51\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x33\xC0\x59\xC3\x2A\x55\x8B\x6C\x24\x10"

#define GL_BEGINRENDERING_SIG_BLOB "\x8B\x44\x24\x08\x8B\x4C\x24\x04\x8B\x54\x24\x0C\xC7\x00\x00\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\xC7\x01\x00\x00\x00\x00\x8B\x0D\x2A\x2A\x2A\x2A\x2B\xC1"
#define GL_BEGINRENDERING_SIG_NEW2 GL_BEGINRENDERING_SIG_BLOB
#define GL_BEGINRENDERING_SIG_NEW "\x55\x8B\xEC\x8B\x45\x0C\x8B\x4D\x08\x56\x57\xC7\x00\x00\x00\x00\x00\xC7\x01\x00\x00\x00\x00\xE8"
#define GL_BEGINRENDERING_SIG_HL25 "\x55\x8B\xEC\x8B\x45\x0C\x2A\x2A\xC7\x00\x00\x00\x00\x00\x8B\x45\x08\xC7\x00\x00\x00\x00\x00"
#define GL_BEGINRENDERING_SIG_SVENGINE "\x8B\x44\x24\x08\x56\x57\xC7\x00\x00\x00\x00\x00\x8B\x44\x24\x0C\xC7\x00\x00\x00\x00\x00\xE8"

#define GL_ENDRENDERING_SIG_BLOB "\xFF\x25\x2A\x2A\x2A\x2A\x90"
#define GL_ENDRENDERING_SIG_NEW2 GL_ENDRENDERING_SIG_BLOB
#define GL_ENDRENDERING_SIG_NEW "\x55\x8B\xEC\x83\xEC\x34\xA1\x2A\x2A\x2A\x2A\x53\x33\xDB"
#define GL_ENDRENDERING_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x6A\x00"
#define GL_ENDRENDERING_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\xC0\x83\xEC\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00"
#define GL_ENDRENDERING_SIG_COMMON_GOLDSRC "\x68\x01\x26\x00\x00\x68\x00\x40\x00\x00"

#define EMITWATERPOLYS_SIG_BLOB "\x83\xEC\x1C\x33\xD2\x55\x56\x8B\x74\x24\x28\x57\x8B\x46\x2C\x8B\x48\x24\x8B\x41\x44\x33\xC9\x8A\x50\x0C\x8A\x48\x0B\x52"
#define EMITWATERPOLYS_SIG_NEW2 EMITWATERPOLYS_SIG_BLOB
#define EMITWATERPOLYS_SIG_NEW "\x55\x8B\xEC\x83\xEC\x20\x56\x8B\x75\x08\x33\xD2\x57\x8B\x46\x2C\x8B\x48\x24\x8B\x41\x44\x33\xC9\x8A\x50\x0C\x8A\x48\x0B\x52\x51"
#define EMITWATERPOLYS_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x2A\xF3\x0F\x10\x2A\x2A\x2A\x2A\x2A\xF3\x0F\x10"
#define EMITWATERPOLYS_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x20\x53\x55\xD9\x05\x2A\x2A\x2A\x2A\x56\x57"

#define R_DRAWTENTITIESONLIST_SIG_BLOB "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x2A\x0F\x2A\x2A\x2A\x00\x00\x8B\x44\x24\x04"
#define R_DRAWTENTITIESONLIST_SIG_NEW2 R_DRAWTENTITIESONLIST_SIG_BLOB
#define R_DRAWTENTITIESONLIST_SIG_NEW "\x55\x8B\xEC\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x44\x0F\x8B\x2A\x2A\x2A\x2A\x8B\x45\x08"
#define R_DRAWTENTITIESONLIST_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\xF3\x0F\x2A\x2A\x2A\x2A\x2A\x2A\x0F\x2E"
#define R_DRAWTENTITIESONLIST_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\x2A\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x00\x00\x00\xD9\x05\x2A\x2A\x2A\x2A\xD9\xEE"

#define R_SETUPGL_SIG_BLOB "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x10\x53\x55\x56\x57\x68\x01\x17\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xFF\x15"
#define R_SETUPGL_SIG_NEW2 R_SETUPGL_SIG_BLOB
#define R_SETUPGL_SIG_NEW "\x55\x8B\xEC\x83\xEC\x10\x53\x56\x57\x68\x01\x17\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xFF\x15\x2A\x2A\x2A\x2A\x8B\x1D"
#define R_SETUPGL_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x08\x2A\x2A\x68\x01\x17\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xFF\x15"
#define R_SETUPGL_SIG_HL25_9899 "\x55\x8B\xEC\x83\xEC\x08\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x68\x02\x17\x00\x00\xFF\x15"
#define R_SETUPGL_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\xC0\x83\xEC\x2A\x53\x56\x57\x68\x01\x17\x00\x00"

#define GL_DISABLEMULTITEXTURE_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x50\xE8"
#define GL_DISABLEMULTITEXTURE_SIG_NEW "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x50\xE8"
#define GL_DISABLEMULTITEXTURE_SIG_HL25 "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A\xE8"
#define GL_DISABLEMULTITEXTURE_SIG_SVENGINE "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x68\xC0\x84\x00\x00\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xC3"

#define GL_ENABLEMULTITEXTURE_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xE1\x0D\x00\x00\xFF\x15"
#define GL_ENABLEMULTITEXTURE_SIG_NEW "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xE1\x0D\x00\x00\xFF\x15"
#define GL_ENABLEMULTITEXTURE_SIG_HL25 "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xE1\x0D\x00\x00\xFF\x15"
#define GL_ENABLEMULTITEXTURE_SIG_SVENGINE "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x68\xC1\x84\x00\x00\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x01\x00\x00\x00\xC3"

#define R_DRAWSEQUENTIALPOLY_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x53\x55\x56\x8B\x88\xF8\x02\x00\x00\xBE\x01\x00\x00\x00"
#define R_DRAWSEQUENTIALPOLY_SIG_NEW2 R_DRAWSEQUENTIALPOLY_SIG_BLOB
#define R_DRAWSEQUENTIALPOLY_SIG_NEW "\x55\x8B\xEC\x51\xA1\x2A\x2A\x2A\x2A\x53\x56\x57\x83\xB8\xF8\x02\x00\x00\x01\x75\x2A\xE8"
#define R_DRAWSEQUENTIALPOLY_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x10\x2A\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x83\xB8\xF8\x02\x00\x00\x01"
#define R_DRAWSEQUENTIALPOLY_SIG_SVENGINE "\xA1\x2A\x2A\x2A\x2A\x83\xEC\x08\x83\xB8\x2A\x2A\x00\x00\x01"

#define R_DRAWBRUSHMODEL_SIG_BLOB "\x83\xEC\x4C\xC7\x05\x2A\x2A\x2A\x2A\xFF\xFF\xFF\xFF\x53\x55\x56\x57"
#define R_DRAWBRUSHMODEL_SIG_NEW2 R_DRAWBRUSHMODEL_SIG_BLOB
#define R_DRAWBRUSHMODEL_SIG_NEW "\x55\x8B\xEC\x83\xEC\x50\x53\x56\x57\x8B\x7D\x08\x89\x3D\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\xFF\xFF\xFF\xFF"
#define R_DRAWBRUSHMODEL_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x2A\x2A\x8B\x2A\x08\x0F\x57\xC9"
#define R_DRAWBRUSHMODEL_SIG_SVENGINE "\x83\xEC\x54\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x50\x53\x8B\x5C\x24\x5C\x55\x56\x57\x89\x1D\x2A\x2A\x2A\x2A\x8D\xBB"

#define R_RECURSIVEWORLDNODE_SIG_BLOB "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x0C\x53\x56\x57\x8B\x7D\x08\x83\x3F\xFE"
#define R_RECURSIVEWORLDNODE_SIG_NEW2 R_RECURSIVEWORLDNODE_SIG_BLOB
#define R_RECURSIVEWORLDNODE_SIG_NEW "\x55\x8B\xEC\x83\xEC\x08\x53\x56\x57\x8B\x7D\x08\x83\x3F\xFE\x0F\x2A\x2A\x2A\x2A\x2A\x8B\x47\x04"
#define R_RECURSIVEWORLDNODE_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x08\x2A\x8B\x5D\x08\x83\x3B\xFE\x0F"
#define R_RECURSIVEWORLDNODE_SIG_SVENGINE "\x83\xEC\x08\x53\x8B\x5C\x24\x10\x83\x3B\xFE"

#define R_TEXTUREANIMATION_SIG_BLOB "\x53\x55\x8B\x6C\x24\x0C\x56\x57\x8B\x45\x2C\x8B\x78\x24\xA1"
#define R_TEXTUREANIMATION_SIG_NEW2 R_TEXTUREANIMATION_SIG_BLOB
#define R_TEXTUREANIMATION_SIG_NEW "\x55\x8B\xEC\x8B\x45\x08\x53\x56\x57\x8B\x48\x2C\xA1\x2A\x2A\x2A\x2A\x85\xC0\x8B\x79\x24\x75\x2A\xBE"
#define R_TEXTUREANIMATION_SIG_HL25 "\x55\x8B\xEC\x83\x3D\x2A\x2A\x2A\x2A\x00\x8B\x55\x08\x2A\x2A\x2A\x8B\x42\x2C\x8B\x58\x24"
#define R_TEXTUREANIMATION_SIG_SVENGINE "\x83\x3D\x2A\x2A\x2A\x2A\x00\x8B\x54\x24\x2A\x53\x56\x57\x8B\x42\x2C"

#define R_RENDERDYNAMICLIGHTMAPS_SIG_BLOB "\x8B\x0D\x2A\x2A\x2A\x2A\x53\x41\x55\x89\x0D\x2A\x2A\x2A\x2A\x8B\x4C\x24\x0C\x56\x57"
#define R_RENDERDYNAMICLIGHTMAPS_SIG_NEW2 R_RENDERDYNAMICLIGHTMAPS_SIG_BLOB
#define R_RENDERDYNAMICLIGHTMAPS_SIG_NEW "\x55\x8B\xEC\x8B\x0D\x2A\x2A\x2A\x2A\x53\x41\x56\x89\x0D\x2A\x2A\x2A\x2A\x8B\x4D\x08\x57"
#define R_RENDERDYNAMICLIGHTMAPS_SIG_HL25 "\x55\x8B\xEC\x8B\x55\x08\x83\xEC\x08\xFF\x05\x2A\x2A\x2A\x2A\xF6\x2A\x08\x14"
#define R_RENDERDYNAMICLIGHTMAPS_SIG_SVENGINE "\x51\x8B\x54\x24\x08\xFF\x05\x2A\x2A\x2A\x2A\x57\x33\xFF\xF6\x42\x08\x14"

#define R_BUILDLIGHTMAP_SIG_BLOB "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\x83\xEC\x18\xDF\xE0\xF6\xC4"
#define R_BUILDLIGHTMAP_SIG_NEW "\x55\x8B\xEC\x83\xEC\x1C\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0"
#define R_BUILDLIGHTMAP_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\x0F\x57\xC9\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\xF3\x0F\x2A\x2A\x2A\x2A\x2A\x2A\x0F\x2E\xC1"
#define R_BUILDLIGHTMAP_SIG_SVENGINE "\xD9\x05\x2A\x2A\x2A\x2A\x83\xEC\x1C\xD9\xEE\xDD\xE1\xDF\xE0\x53\x55\x56\x57\xDD\xD9"

#define R_ADDDYNAMICLIGHTS_SIG_SVENGINE "\x83\xEC\x30\x2A\x2A\x8B\x2A\x24\x2A\x2A\x01\x00\x00\x00\x2A\x2A\xB9"
#define R_ADDDYNAMICLIGHTS_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\x8B\x45\x08\x2A\x2A\x2A\x01\x00\x00\x00\x0F\x2A\x50\x18\x0F\x2A\x48\x1A"
#define R_ADDDYNAMICLIGHTS_SIG_NEW "\x55\x8B\xEC\x83\xEC\x48\x2A\x8B\x2A\x08\x2A\x2A\x0F\x2A\x2A\x18"
#define R_ADDDYNAMICLIGHTS_SIG_BLOB "\x83\xEC\x44\x53\x55\x8B\x6C\x24\x50\x56\x57\x33\xC9\x0F\xBF\x5D\x18"

#define R_DECALMPOLY_SIG "\xA1\x2A\x2A\x2A\x2A\x57\x50\xE8\x2A\x2A\x2A\x2A\x8B\x4C\x24\x10\x8B\x51\x18"
#define R_DECALMPOLY_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x57\x50\xE8\x2A\x2A\x2A\x2A\x8B\x4D\x0C\x8B\x51\x18\x52\xE8"

#define R_DRAWDECALS_SIG "\xB8\x0C\x00\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x85\xC0"
#define R_DRAWDECALS_SIG_NEW "\x55\x8B\xEC\xB8\x10\x00\x00\x00\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x84"
#define R_DRAWDECALS_SIG_SVENGINE "\xB8\x2A\x2A\x00\x00\xE8\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x84\x2A\x2A\x2A\x2A\x53\x8B\x1D"

//TODO: use string "Failed to load custom decal for player"
#define DRAW_DECALTEXTURE_SIG_BLOB      "\x8B\x44\x24\x04\x56\x85\xC0\x57\x7D\x7B\x83\xC9\xFF\x2B\xC8\x8D\x04\x49"
#define DRAW_DECALTEXTURE_SIG_BLOB2     "\x8B\x4C\x24\x04\x56\x85\xC9\x57\x2A\x2A\x83\xC8\xFF\x2B\xC1"
#define DRAW_DECALTEXTURE_SIG_NEW       "\x55\x8B\xEC\x8B\x4D\x08\x56\x85\xC9\x57\x7D\x2A\x83\xC8\xFF\x2B\xC1\x8D\x0C\xC0"
#define DRAW_DECALTEXTURE_SIG_HL25      "\x55\x8B\xEC\x8B\x4D\x08\x85\xC9\x2A\x2A\xF7\xD1\x69\xD1\x50\x02\x00\x00"
#define DRAW_DECALTEXTURE_SIG_SVENGINE "\x8B\x4C\x24\x2A\x0F\xAE\xE8\x85\xC9\x2A\x2A\xF7\xD1"

#define DRAW_MIPTEXTEXTURE_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\x2A\x2A\x2A\x24\x40\x2A\x2A\x2A\x24\x40"
#define DRAW_MIPTEXTEXTURE_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x2A\x8B\x5D\x0C\x2A\x8B\x75\x08\x2A\x89\x75\xCC"
#define DRAW_MIPTEXTEXTURE_SIG_NEW "\x55\x8B\xEC\x83\xEC\x2A\x2A\x2A\x2A\x2A\x08\x2A\x83\x2A\x2A\x20"

//TODO: use string "R_RenderView: NULL worldmodel"
#define R_RENDERVIEW_SIG_BLOB "\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\x83\xEC\x14\xDF\xE0\xF6\xC4"
#define R_RENDERVIEW_SIG_NEW2 R_RENDERVIEW_SIG_BLOB
#define R_RENDERVIEW_SIG_NEW "\x55\x8B\xEC\x83\xEC\x14\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D\x2A\x2A\x2A\x2A\xDF\xE0\xF6\xC4\x44"
#define R_RENDERVIEW_SIG_HL25 "\x55\x8B\xEC\xF3\x0F\x10\x05\x2A\x2A\x2A\x2A\x83\xEC\x2A\x0F\x57\xC9\x0F\x2E\xC1\x9F\xF6"
#define R_RENDERVIEW_SIG_SVENGINE "\x55\x8B\xEC\x83\xE4\xC0\x83\xEC\x34\x53\x56\x57\x8B\x7D\x08\x85\xFF"

#define V_RENDERVIEW_SIG_BLOB "\xA1\x2A\x2A\x2A\x2A\x81\xEC\x2A\x00\x00\x00\x2A\x2A\x33\x2A\x33\x2A\x2A\x2A\x89\x35\x2A\x2A\x2A\x2A\x89\x35"
#define V_RENDERVIEW_SIG_NEW2 V_RENDERVIEW_SIG_BLOB
#define V_RENDERVIEW_SIG_NEW "\x55\x8B\xEC\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x2A\x2A\x33\x2A\x33"
#define V_RENDERVIEW_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x2A\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x2A\x33\x2A\x89\x35\x2A\x2A\x2A\x2A\x89\x35"
#define V_RENDERVIEW_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\xD9\xEE\xD9\x15"
 
#define VID_UPDATEWINDOWVARS_SIG_BLOB "\x56\x8B\x74\x24\x08\x8B\xC6\x8B\x08\x89\x0D\x2A\x2A\x2A\x2A\x8B\x50\x04\x89\x15"
#define VID_UPDATEWINDOWVARS_SIG_NEW2 VID_UPDATEWINDOWVARS_SIG_BLOB
#define VID_UPDATEWINDOWVARS_SIG_NEW "\x55\x8B\xEC\x51\x56\x8B\x75\x08\x8B\xC6\x8B\x08\x89\x0D\x2A\x2A\x2A\x2A\x8B\x50\x04\x89\x15"
#define VID_UPDATEWINDOWVARS_SIG_HL25 "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x83\xEC\x10\x85\xC0"
#define VID_UPDATEWINDOWVARS_SIG_SVENGINE "\x8b\xc7\x99\x2B\xC2\xD1\xF8\x03\x2A\x50"

#define R_FORCECVARS_SIG_SVENGINE "\x83\x7C\x24\x2A\x00\x2A\x2A\x2A\x2A\x00\x00\x81\x3D\x2A\x2A\x2A\x2A\xFF\x00\x00\x00"
#define R_FORCECVARS_SIG_NEW "\x55\x8B\xEC\x8B\x45\x08\x85\xC0\x0F\x84\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x1D"

#define CL_ALLOCDLIGHT_SIG_COMMON   "\x20\x74\x2A\x47\x83\xC6\x28"
#define CL_ALLOCDLIGHT_SIG_BLOB     "\x53\x8B\x5C\x24\x08\x56\x57\x85\x2A\x2A\x2A\xBE\x2A\x2A\x2A\x2A\x33\x2A\x39"
#define CL_ALLOCDLIGHT_SIG_NEW2     CL_ALLOCDLIGHT_SIG_BLOB
#define CL_ALLOCDLIGHT_SIG_NEW      "\x55\x8B\xEC\x2A\x8B\x5D\x08\x2A\x2A\x85\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x20"
#define CL_ALLOCDLIGHT_SIG_HL25     "\x55\x8B\xEC\x2A\x8B\x2A\x08\x2A\x2A\x85\x2A\x2A\x2A\xBE\x2A\x2A\x2A\x2A\x33\x2A\x39"
#define CL_ALLOCDLIGHT_SIG_SVENGINE "\x2A\x8B\x5C\x24\x2A\x2A\x2A\x85\x2A\x2A\x2A\xBE\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x20"

//Studio Funcs
#define R_GLSTUDIODRAWPOINTS_SIG_BLOB     "\x83\xEC\x44\xA1\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x53\x55\x56\x8B\x70\x54\x8B\x40\x60\x57"
#define R_GLSTUDIODRAWPOINTS_SIG_BLOB2    "\x83\xEC\x48\x8B\x0D\x2A\x2A\x2A\x2A\x8B\x15\x2A\x2A\x2A\x2A\x53\x55\x8B\x41\x54\x8B\x59\x60"
#define R_GLSTUDIODRAWPOINTS_SIG_NEW      "\x55\x8B\xEC\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x53\x56\x8B\x70\x54\x8B\x40\x60\x03\xC1"
#define R_GLSTUDIODRAWPOINTS_SIG_NEW2     R_GLSTUDIODRAWPOINTS_SIG_BLOB2
#define R_GLSTUDIODRAWPOINTS_SIG_HL25     "\x55\x8B\xEC\x81\xEC\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x8B\x0D\x2A\x2A\x2A\x2A\x2A\x8B\x35\x2A\x2A\x2A\x2A\x2A\x8B\x3D\x2A\x2A\x2A\x2A\xC7\x85"
#define R_GLSTUDIODRAWPOINTS_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\x53\x8B\x1D\x2A\x2A\x2A\x2A\x55\x56\x57\x8B\x3D\x2A\x2A\x2A\x2A\x8B\x4B\x54"
#define R_GLSTUDIODRAWPOINTS_SIG_COMMON "\x75\x2A\x68\x44\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A"

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

#define R_STUDIOCHROME_SIG_BLOB "\x83\xEC\x24\x8B\x0D\x2A\x2A\x2A\x2A\x53\x56\x8B\x74\x24\x34\x57\x8B\x04\xB5"
#define R_STUDIOCHROME_SIG_NEW2 R_STUDIOCHROME_SIG_BLOB
#define R_STUDIOCHROME_SIG_NEW "\x55\x8B\xEC\x83\xEC\x24\x8B\x0D\x2A\x2A\x2A\x2A\x53\x56\x8B\x75\x0C\x57\x8B\x04\xB5"
#define R_STUDIOCHROME_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x2A\x8B\x2A\x08\x2A\x8B\x2A\x0C\x8B\x04"
#define R_STUDIOCHROME_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\xD9\xE8\x53"

#define BUILDNORMALINDEXTABLE_SIG_BLOB "\x8B\x15\x2A\x2A\x2A\x2A\x2A\x8B\x4A\x50\x85\xC9\x2A\x2A\x83\xC8\xFF"
#define BUILDNORMALINDEXTABLE_SIG_NEW "\x55\x8B\xEC\x51\x8B\x15\x2A\x2A\x2A\x2A\x57\x8B\x4A\x50\x85\xC9\x7E\x0A\x83\xC8\xFF\xBF"
#define BUILDNORMALINDEXTABLE_SIG_HL25 ""
#define BUILDNORMALINDEXTABLE_SIG_SVENGINE ""

//TODO: use string "DM_Base.bmp"
#define R_STUDIOSETUPSKIN_SIG_BLOB "\xA0\x2A\x2A\x2A\x2A\x81\xEC\x0C\x01\x00\x00\xA8\x02\x53\x56\x57\x0F\x85"
#define R_STUDIOSETUPSKIN_SIG_NEW "\x55\x8B\xEC\x81\xEC\x04\x01\x00\x00\xA0\x2A\x2A\x2A\x2A\x53\x56\x57\xA8\x02\x0F\x85\x2A\x2A\x2A\x2A\x8B\x5D\x0C"
#define R_STUDIOSETUPSKIN_SIG_HL25 "\x55\x8B\xEC\x81\xEC\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\xF6\x05\x2A\x2A\x2A\x2A\x02\x8B\x45\x08"
#define R_STUDIOSETUPSKIN_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\xF6\x05"

#define R_GETSPRITEFRAME_SIG "\x56\x8B\x74\x24\x08\x57\x33\xFF\x85\xF6\x75\x2A\x68\x2A\x2A\x2A\x2A\xE8"
#define R_GETSPRITEFRAME_SIG_NEW "\x55\x8B\xEC\x56\x8B\x75\x08\x57\x33\xFF\x85\xF6\x75\x13\x68"

//TODO: use string "R_DrawSpriteModel:  couldn"
#define R_DRAWSRPITEMODEL_SIG_BLOB "\x83\xEC\x40\x53\x56\x57\x8B\x7C\x24\x50\x8B\x87\x94\x0B\x00\x00"
#define R_DRAWSRPITEMODEL_SIG_NEW "\x55\x8B\xEC\x83\xEC\x44\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x2A\x2A\x2A\x00\x00\xD9\x2A\xE0\x02"
#define R_DRAWSRPITEMODEL_SIG_HL25 "\x55\x8B\xEC\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\x2A\x2A\x2A\x8B\x75\x08\x2A\x8B\x2A\x94\x0B\x00\x00"
#define R_DRAWSRPITEMODEL_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x2A\x53\x8B\x5C\x24\x2A\x55\x8B\x83\x2A\x0B\x00\x00"

#define R_LIGHTSTRENGTH_SIG_SVENGINE "\x8B\x15\x2A\x2A\x2A\x2A\x2A\x8B\x35\x2A\x2A\x2A\x2A\x2A\x8B\x7C\x24\x0C"
#define R_LIGHTSTRENGTH_SIG_HL25   ""//inlined
#define R_LIGHTSTRENGTH_SIG_NEW   "\x55\x8B\xEC\x83\xEC\x0C\x8B\x4D\x08\x8B\x15\x2A\x2A\x2A\x2A\x2A\x8B\x04\x2A\x2A\x2A\x2A\x2A"
#define R_LIGHTSTRENGTH_SIG_NEW2  "\x8B\x4C\x24\x04\x8B\x15\x2A\x2A\x2A\x2A\x83\xEC\x0C\x8B\x04\x2A\x2A\x2A\x2A\x2A\x3B\xC2"
#define R_LIGHTSTRENGTH_SIG_BLOB  "\x8B\x0D\x2A\x2A\x2A\x2A\x83\xEC\x0C\x55\x8B\x2A\x24\x2A\x2A\x8B"

#define R_GLOW_BLEND_SIG_SVENGINE ""//inlined
#define R_GLOW_BLEND_SIG_HL25     ""//inlined
#define R_GLOW_BLEND_SIG_NEW      "\x55\x8B\xEC\x81\xEC\x2A\x00\x00\x00\xD9\x05\x2A\x2A\x2A\x2A\xD8\x25\x2A\x2A\x2A\x2A\x2A\x8D\x2A\x2A"
#define R_GLOW_BLEND_SIG_NEW2      "\x83\xEC\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD8\x25\x2A\x2A\x2A\x2A\x2A\x8D\x44\x24\x0C\x2A\x2A\xD9"
#define R_GLOW_BLEND_SIG_BLOB      "\x81\xEC\x2A\x2A\x00\x00\xD9\x05\x2A\x2A\x2A\x2A\xD8\x25\x2A\x2A\x2A\x2A\x2A\x8D"

//#define GL_UPLOAD16_SIG_SVENGINE "\x8B\x44\x24\x2A\x83\xEC\x08\x85\xC0\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04\x83\xC4\x08"

#define SCR_BEGIN_LOADING_PLAQUE "\x6A\x01\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x83\xC4\x04\x83\xF8\x03"

#define HOST_IS_SINGLE_PLAYER_GAME_BLOB      "\xA1\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x33\xC0"
#define HOST_IS_SINGLE_PLAYER_GAME_NEW3      HOST_IS_SINGLE_PLAYER_GAME_BLOB
#define HOST_IS_SINGLE_PLAYER_GAME_NEW2      "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x75\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x33\xC0"
#define HOST_IS_SINGLE_PLAYER_GAME_NEW       "\xA1\x2A\x2A\x2A\x2A\x85\xC0\xA1\x2A\x2A\x2A\x2A\x74\x05\xA1\x2A\x2A\x2A\x2A\xC3"
#define HOST_IS_SINGLE_PLAYER_GAME_HL25      "\x83\x3D\x2A\x2A\x2A\x2A\x00\xA1\x2A\x2A\x2A\x2A\x0F\x44\x05\x2A\x2A\x2A\x2A\xC3"
#define HOST_IS_SINGLE_PLAYER_GAME_SVENGINE  "\x33\xC0\x39\x05\x2A\x2A\x2A\x2A\x75\x2A\x83\x3D\x2A\x2A\x2A\x2A\x01\x0F\x94\xC0"

#define MOD_UNLOADSPRITETEXTURES_BLOB     "\x81\xEC\x2A\x2A\x00\x00\xB8\x01\x00\x00\x00\x2A\x8B\x2A\x24\x08\x01\x00\x00\x2A\x2A\x2A\x2A\x44"
#define MOD_UNLOADSPRITETEXTURES_NEW2     MOD_UNLOADSPRITETEXTURES_BLOB
#define MOD_UNLOADSPRITETEXTURES_NEW      "\x55\x8B\xEC\x81\xEC\x00\x01\x00\x00\x53\x8B\x5D\x08\xB8\x01\x00\x00\x00\x56\x8B\x4B\x44"
#define MOD_UNLOADSPRITETEXTURES_HL25     "\x55\x8B\xEC\x81\xEC\x04\x01\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC5\x89\x45\xFC\x57\x8B\x7D\x08\x83\x7F\x44\x01"
#define MOD_UNLOADSPRITETEXTURES_SVENGINE "\x81\xEC\x04\x01\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x00\x01\x00\x00\x57\x8B\xBC\x24\x0C\x01\x00\x00"

#define R_INITPARTICLETEXTURE_BLOB "\xA1\x2A\x2A\x2A\x2A\x81\xEC\x2A\x2A\x00\x00\x8B\xC8\x40"
#define R_INITPARTICLETEXTURE_COMMON "\x68\x01\x14\x00\x00\x68\x08\x19\x00\x00\x6A\x00\x6A\x08\x6A\x08"

void R_FillAddress(void)
{
	ULONG_PTR addr;

	auto hSDL2 = GetModuleHandleA("SDL2.dll");
	if (hSDL2)
	{
		gPrivateFuncs.SDL_GL_SetAttribute = (decltype(gPrivateFuncs.SDL_GL_SetAttribute))GetProcAddress(hSDL2, "SDL_GL_SetAttribute");
	}

	auto engineFactory = g_pMetaHookAPI->GetEngineFactory();

	if (engineFactory)
	{
#define ENGINE_SURFACE_VERSION "EngineSurface007"
		void *engineSurface = (void *)engineFactory(ENGINE_SURFACE_VERSION, NULL);

		auto engineSurface_vftable = *(ULONG_PTR**)engineSurface;

		int index_drawSetTextureRGBA = 8;
		int index_drawSetTexture = 9;
		int index_createNewTextureID = 11;
		int index_drawSetTextureFile = 18;
		int index_isTextureIDValid = 20;
		int index_drawFlushText = 22;

		if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			/*
.rdata:102C94B4 ; const EngineSurface::`vftable'
.rdata:102C94B4 ??_7EngineSurface@@6B@ dd offset dtor_0 ; DATA XREF: dtor_0+2C¡üo
.rdata:102C94B4                                         ; sub_102285C0+30¡üo ...
.rdata:102C94B8                 dd offset pushMakeCurrent //1
.rdata:102C94BC                 dd offset popMakeCurrent  //2
.rdata:102C94C0                 dd offset drawFilledRect  //3
.rdata:102C94C4                 dd offset drawOutlinedRect//4
.rdata:102C94C8                 dd offset drawLine			//5
.rdata:102C94CC                 dd offset drawPolyLine		//6
.rdata:102C94D0                 dd offset drawTexturedPolygon//7
.rdata:102C94D4                 dd offset drawSetTextureRGBA//8
.rdata:102C94D8                 dd offset drawSetTexture	//9
.rdata:102C94DC                 dd offset drawTexturedRect	//10
.rdata:102C94E0                 dd offset drawTexturedRectAdd//11
.rdata:102C94E4                 dd offset j_GL_GenTexture	//12
.rdata:102C94E8                 dd offset drawSetColor		//13
			*/
			index_createNewTextureID++;
			index_drawSetTextureFile ++;
			index_isTextureIDValid++;
			index_drawFlushText ++;
		}

		gPrivateFuncs.enginesurface_drawSetTextureRGBA = (decltype(gPrivateFuncs.enginesurface_drawSetTextureRGBA))engineSurface_vftable[index_drawSetTextureRGBA];
		gPrivateFuncs.enginesurface_drawSetTexture = (decltype(gPrivateFuncs.enginesurface_drawSetTexture))engineSurface_vftable[index_drawSetTexture];
		gPrivateFuncs.enginesurface_createNewTextureID = (decltype(gPrivateFuncs.enginesurface_createNewTextureID))engineSurface_vftable[index_createNewTextureID];
		gPrivateFuncs.enginesurface_drawSetTextureFile = (decltype(gPrivateFuncs.enginesurface_drawSetTextureFile))engineSurface_vftable[index_drawSetTextureFile];
		gPrivateFuncs.enginesurface_isTextureIDValid = (decltype(gPrivateFuncs.enginesurface_isTextureIDValid))engineSurface_vftable[index_isTextureIDValid];
		gPrivateFuncs.enginesurface_drawFlushText = (decltype(gPrivateFuncs.enginesurface_drawFlushText))engineSurface_vftable[index_drawFlushText];

		typedef struct
		{
			void* candicate;
			int instCount;
		}enginesurface_isTextureIDValid_SearchContext;

		enginesurface_isTextureIDValid_SearchContext ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.enginesurface_isTextureIDValid, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (enginesurface_isTextureIDValid_SearchContext*)context;

			if (instCount <= 8 && pinst->id == X86_INS_CALL &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM)
			{
				ctx->candicate = (decltype(ctx->candicate))pinst->detail->x86.operands[0].imm;
			}

			ctx->instCount = instCount;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

		}, 0, &ctx);

		if (ctx.candicate && ctx.instCount <= 20)
		{
			gPrivateFuncs.staticGetTextureById = (decltype(gPrivateFuncs.staticGetTextureById))ctx.candicate;
		}
	}

	gPrivateFuncs.triapi_RenderMode = gEngfuncs.pTriAPI->RenderMode;
	//gPrivateFuncs.triapi_Color4f = gEngfuncs.pTriAPI->Color4f;

	bHasOfficialFBOSupport = false;
	bHasOfficialGLTexAllocSupport = true;

	if (1)
	{
		const char sigs1[] = "FBO backbuffer rendering disabled";
		auto FBO_String = Search_Pattern_Data(sigs1);
		if (!FBO_String)
			FBO_String = Search_Pattern_Rdata(sigs1);
		if (FBO_String)
		{
			bHasOfficialFBOSupport = true;
		}
	}

	if (1)
	{
		const char pattern[] = "\xA8\x16\x00\x00";
		PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
		PUCHAR SearchLimit = (PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				typedef struct
				{
					bool bFoundPush;
					bool bFoundCall;
				}LoadSkysSearchContext;

				LoadSkysSearchContext ctx = { 0 };

				g_pMetaHookAPI->DisasmRanges(pFound + 4, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (LoadSkysSearchContext*)context;

					if (instCount == 1 && pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG )
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
					bHasOfficialGLTexAllocSupport = false;

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

	if (1)
	{
		const char pattern[] = "\x68\x00\x1F\x00\x00\xFF";
		PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
		PUCHAR SearchLimit = (PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				auto pCandidateFunction = g_pMetaHookAPI->ReverseSearchFunctionBegin(pFound, 0x80);
				if (pCandidateFunction)
				{
					typedef struct
					{
						bool bFoundPushString;
					}GL_InitSearchContext;

					GL_InitSearchContext ctx = { 0 };

					g_pMetaHookAPI->DisasmRanges(pCandidateFunction, 0x120, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

						auto pinst = (cs_insn*)inst;
						auto ctx = (GL_InitSearchContext*)context;

						if (pinst->id == X86_INS_PUSH &&
							pinst->detail->x86.op_count == 1 &&
							pinst->detail->x86.operands[0].type == X86_OP_IMM &&
							(
							((PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)g_dwEngineDataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize) ||
							((PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)g_dwEngineRdataBase &&
								(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineRdataBase + g_dwEngineRdataSize)
								) )
						{
							auto pString = (PCHAR)pinst->detail->x86.operands[0].imm;
							if (!memcmp(pString, "Failed to query GL vendor", sizeof("Failed to query GL vendor") - 1) ||
								!memcmp(pString, "Failed to query gl vendor", sizeof("Failed to query gl vendor") - 1) ||
								!memcmp(pString, "GL_VENDOR: %s", sizeof("GL_VENDOR: %s") - 1))
							{
								ctx->bFoundPushString = true;
							}
						}

						if (ctx->bFoundPushString)
							return TRUE;

						if (address[0] == 0xCC)
							return TRUE;

						if (pinst->id == X86_INS_RET)
							return TRUE;

						return FALSE;

						}, 0, &ctx);

					if (ctx.bFoundPushString)
					{
						gPrivateFuncs.GL_Init = (decltype(gPrivateFuncs.GL_Init))pCandidateFunction;

						break;
					}
				}

				SearchBegin = pFound + Sig_Length(pattern);
}
			else
			{
				break;
			}
		}
	}
	Sig_FuncNotFound(GL_Init);

#if 0//unused
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_MarkLights = (decltype(gPrivateFuncs.R_MarkLights))Search_Pattern(R_MARKLIGHTS_SVENGINE);
		Sig_FuncNotFound(R_MarkLights);
	}
	else
	{
		gPrivateFuncs.R_MarkLights = (decltype(gPrivateFuncs.R_MarkLights))Search_Pattern(R_MARKLIGHTS_NEW);
		Sig_FuncNotFound(R_MarkLights);
	}
#endif

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_PolyBlend = (decltype(gPrivateFuncs.R_PolyBlend))Search_Pattern(R_POLYBLEND_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_PolyBlend = (decltype(gPrivateFuncs.R_PolyBlend))Search_Pattern(R_POLYBLEND_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_PolyBlend = (decltype(gPrivateFuncs.R_PolyBlend))Search_Pattern(R_POLYBLEND_NEW);
		if(!gPrivateFuncs.R_PolyBlend)
			gPrivateFuncs.R_PolyBlend = (decltype(gPrivateFuncs.R_PolyBlend))Search_Pattern(R_POLYBLEND_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_PolyBlend = (decltype(gPrivateFuncs.R_PolyBlend))Search_Pattern(R_POLYBLEND_BLOB);
	}
	Sig_FuncNotFound(R_PolyBlend);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))Search_Pattern(S_EXTRAUPDATE_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))Search_Pattern(S_EXTRAUPDATE_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))Search_Pattern(S_EXTRAUPDATE_NEW);
		if(!gPrivateFuncs.S_ExtraUpdate)
			gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))Search_Pattern(S_EXTRAUPDATE_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))Search_Pattern(S_EXTRAUPDATE_BLOB);
	}

	Sig_FuncNotFound(S_ExtraUpdate);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))Search_Pattern(GL_BIND_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))Search_Pattern(GL_BIND_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))Search_Pattern(GL_BIND_SIG_NEW);
		if(!gPrivateFuncs.GL_Bind)
			gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))Search_Pattern(GL_BIND_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))Search_Pattern(GL_BIND_SIG_BLOB);
	}
	Sig_FuncNotFound(GL_Bind);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))Search_Pattern_From(gPrivateFuncs.GL_Bind, GL_SELECTTEXTURE_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))Search_Pattern_From(gPrivateFuncs.GL_Bind, GL_SELECTTEXTURE_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))Search_Pattern(GL_SELECTTEXTURE_SIG_NEW);
		if(!gPrivateFuncs.GL_SelectTexture)
			gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))Search_Pattern(GL_SELECTTEXTURE_SIG_NEW2);		
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))Search_Pattern(GL_SELECTTEXTURE_SIG_BLOB);
	}
	Sig_FuncNotFound(GL_SelectTexture);

	if (g_iEngineType != ENGINE_SVENGINE)
	{
		const char sigs1[] = "Texture Overflow: MAX_GLTEXTURES";
		auto Error_String = Search_Pattern_Data(sigs1);
		if (!Error_String)
			Error_String = Search_Pattern_Rdata(sigs1);
		if (Error_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
			*(DWORD*)(pattern + 1) = (DWORD)Error_String;
			auto Error_Call = Search_Pattern(pattern);
			if (Error_Call)
			{
				gPrivateFuncs.GL_LoadTexture2 = (decltype(gPrivateFuncs.GL_LoadTexture2))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Error_Call, 0x500, [](PUCHAR Candidate) {

					if (Candidate[0] == 0xB8 &&
						Candidate[3] == 0x00 &&
						Candidate[4] == 0x00 &&
						Candidate[5] == 0xE8)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					return FALSE;
					});
			}
		}
	}

	if (!gPrivateFuncs.GL_LoadTexture2)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.GL_LoadTexture2 = (decltype(gPrivateFuncs.GL_LoadTexture2))Search_Pattern(GL_LOADTEXTURE2_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.GL_LoadTexture2 = (decltype(gPrivateFuncs.GL_LoadTexture2))Search_Pattern(GL_LOADTEXTURE2_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.GL_LoadTexture2 = (decltype(gPrivateFuncs.GL_LoadTexture2))Search_Pattern(GL_LOADTEXTURE2_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.GL_LoadTexture2 = (decltype(gPrivateFuncs.GL_LoadTexture2))Search_Pattern(GL_LOADTEXTURE2_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(GL_LoadTexture2);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))Search_Pattern_From(gPrivateFuncs.GL_LoadTexture2, R_CULLBOX_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))Search_Pattern_From(gPrivateFuncs.GL_LoadTexture2, R_CULLBOX_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))Search_Pattern_From(gPrivateFuncs.GL_LoadTexture2, R_CULLBOX_SIG_NEW);
		if(!gPrivateFuncs.R_CullBox)
			gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))Search_Pattern_From(gPrivateFuncs.GL_LoadTexture2, R_CULLBOX_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))Search_Pattern_From(gPrivateFuncs.GL_LoadTexture2, R_CULLBOX_SIG_BLOB);
	}
	Sig_FuncNotFound(R_CullBox);

	//inlined in HL25 and SvEngine
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_SetupFrame = (decltype(gPrivateFuncs.R_SetupFrame))Search_Pattern(R_SETUPFRAME_SIG_NEW);
		Sig_FuncNotFound(R_CullBox);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_SetupFrame = (decltype(gPrivateFuncs.R_SetupFrame))Search_Pattern(R_SETUPFRAME_SIG_BLOB);
		Sig_FuncNotFound(R_SetupFrame);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))Search_Pattern(R_SETUPGL_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		if (g_dwEngineBuildnum >= 9899)
		{
			gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))Search_Pattern(R_SETUPGL_SIG_HL25_9899);
		}
		else
		{
			gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))Search_Pattern(R_SETUPGL_SIG_HL25);
		}
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))Search_Pattern(R_SETUPGL_SIG_NEW);
		if(!gPrivateFuncs.R_SetupGL)
			gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))Search_Pattern(R_SETUPGL_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))Search_Pattern(R_SETUPGL_SIG_BLOB);
	}

	Sig_FuncNotFound(R_SetupGL);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_DrawSequentialPoly = (decltype(gPrivateFuncs.R_DrawSequentialPoly))Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_DrawSequentialPoly = (decltype(gPrivateFuncs.R_DrawSequentialPoly))Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_DrawSequentialPoly = (decltype(gPrivateFuncs.R_DrawSequentialPoly))Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_NEW);
		if(!gPrivateFuncs.R_DrawSequentialPoly)
			gPrivateFuncs.R_DrawSequentialPoly = (decltype(gPrivateFuncs.R_DrawSequentialPoly))Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_DrawSequentialPoly = (decltype(gPrivateFuncs.R_DrawSequentialPoly))Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_BLOB);
	}
	Sig_FuncNotFound(R_DrawSequentialPoly);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_TextureAnimation = (decltype(gPrivateFuncs.R_TextureAnimation))Search_Pattern(R_TEXTUREANIMATION_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_TextureAnimation = (decltype(gPrivateFuncs.R_TextureAnimation))Search_Pattern(R_TEXTUREANIMATION_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_TextureAnimation = (decltype(gPrivateFuncs.R_TextureAnimation))Search_Pattern(R_TEXTUREANIMATION_SIG_NEW);
		if(!gPrivateFuncs.R_TextureAnimation)
			gPrivateFuncs.R_TextureAnimation = (decltype(gPrivateFuncs.R_TextureAnimation))Search_Pattern(R_TEXTUREANIMATION_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_TextureAnimation = (decltype(gPrivateFuncs.R_TextureAnimation))Search_Pattern(R_TEXTUREANIMATION_SIG_BLOB);
	}
	Sig_FuncNotFound(R_TextureAnimation);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))Search_Pattern(V_RENDERVIEW_SIG_SVENGINE);
		Sig_FuncNotFound(V_RenderView);

		gPrivateFuncs.R_RenderView_SvEngine = (decltype(gPrivateFuncs.R_RenderView_SvEngine))Search_Pattern(R_RENDERVIEW_SIG_SVENGINE);
		Sig_FuncNotFound(R_RenderView_SvEngine);

		char pattern[] = "\xDD\xD8\xDD\xD8\xE8";
		addr = (ULONG_PTR)Search_Pattern_From(gPrivateFuncs.R_RenderView_SvEngine, pattern);
		Sig_AddrNotFound(R_RenderScene);
		gPrivateFuncs.R_RenderScene = (decltype(gPrivateFuncs.R_RenderScene))GetCallAddress(addr + 4);
		Sig_FuncNotFound(R_RenderScene);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))Search_Pattern(V_RENDERVIEW_SIG_HL25);
		Sig_FuncNotFound(V_RenderView);

		gPrivateFuncs.R_RenderView = (decltype(gPrivateFuncs.R_RenderView))Search_Pattern(R_RENDERVIEW_SIG_HL25);
		Sig_FuncNotFound(R_RenderView);

		gPrivateFuncs.R_RenderScene = (decltype(gPrivateFuncs.R_RenderScene))Search_Pattern(R_RENDERSCENE_SIG_HL25);
		Sig_FuncNotFound(R_RenderScene);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))Search_Pattern(V_RENDERVIEW_SIG_NEW);
		if(!gPrivateFuncs.V_RenderView)
			gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))Search_Pattern(V_RENDERVIEW_SIG_NEW2);
		Sig_FuncNotFound(V_RenderView);

		gPrivateFuncs.R_RenderView = (decltype(gPrivateFuncs.R_RenderView))Search_Pattern(R_RENDERVIEW_SIG_NEW);
		if (!gPrivateFuncs.R_RenderView)
			gPrivateFuncs.R_RenderView = (decltype(gPrivateFuncs.R_RenderView))Search_Pattern(R_RENDERVIEW_SIG_NEW2);
		Sig_FuncNotFound(R_RenderView);

		gPrivateFuncs.R_RenderScene = (decltype(gPrivateFuncs.R_RenderScene))Search_Pattern(R_RENDERSCENE_SIG_NEW);
		Sig_FuncNotFound(R_RenderScene);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))Search_Pattern(V_RENDERVIEW_SIG_BLOB);
		Sig_FuncNotFound(V_RenderView);

		gPrivateFuncs.R_RenderView = (decltype(gPrivateFuncs.R_RenderView))Search_Pattern(R_RENDERVIEW_SIG_BLOB);
		Sig_FuncNotFound(R_RenderView);

		gPrivateFuncs.R_RenderScene = (decltype(gPrivateFuncs.R_RenderScene))Search_Pattern(R_RENDERSCENE_SIG_BLOB);
		Sig_FuncNotFound(R_RenderScene);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))Search_Pattern(R_NEWMAP_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))Search_Pattern(R_NEWMAP_SIG_HL25);
	}
	else if(g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))Search_Pattern(R_NEWMAP_SIG_NEW);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))Search_Pattern(R_NEWMAP_SIG_BLOB);
	}
	Sig_FuncNotFound(R_NewMap);

	if (1)
	{
		char pattern[] = "\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\xFF\xFF\xFF\xFF";
		addr = (ULONG_PTR)Search_Pattern_From_Size(gPrivateFuncs.R_NewMap, 0x100, pattern);
		if (addr)
		{
			gPrivateFuncs.R_ClearParticles = (decltype(gPrivateFuncs.R_ClearParticles))GetCallAddress(addr + 0);
			gPrivateFuncs.R_DecalInit = (decltype(gPrivateFuncs.R_DecalInit))GetCallAddress(addr + 5);
			gPrivateFuncs.V_InitLevel = (decltype(gPrivateFuncs.V_InitLevel))GetCallAddress(addr + 10);
			gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))GetCallAddress(addr + 15);
		}
	}

	if (!gPrivateFuncs.GL_BuildLightmaps)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))Search_Pattern(GL_BUILDLIGHTMAPS_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))Search_Pattern(GL_BUILDLIGHTMAPS_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))Search_Pattern(GL_BUILDLIGHTMAPS_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))Search_Pattern(GL_BUILDLIGHTMAPS_SIG_BLOB);
		}
		Sig_FuncNotFound(GL_BuildLightmaps);
	}

	if (1)
	{
		const char sigs1[] = "Error: lightmap for texture %s too large";
		auto Error_String = Search_Pattern_Data(sigs1);
		if (!Error_String)
			Error_String = Search_Pattern_Rdata(sigs1);
		if (Error_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x18";
			*(DWORD*)(pattern + 1) = (DWORD)Error_String;
			auto Error_Call = Search_Pattern(pattern);
			if (Error_Call)
			{
				gPrivateFuncs.R_BuildLightMap = (decltype(gPrivateFuncs.R_BuildLightMap))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Error_Call, 0x300, [](PUCHAR Candidate) {

					if (Candidate[0] == 0xD9 &&
						Candidate[1] == 0x05)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					return FALSE;
				});
			}
		}
	}

	if (!gPrivateFuncs.R_BuildLightMap)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_BuildLightMap = (decltype(gPrivateFuncs.R_BuildLightMap))Search_Pattern(R_BUILDLIGHTMAP_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.R_BuildLightMap = (decltype(gPrivateFuncs.R_BuildLightMap))Search_Pattern(R_BUILDLIGHTMAP_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_BuildLightMap = (decltype(gPrivateFuncs.R_BuildLightMap))Search_Pattern(R_BUILDLIGHTMAP_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.R_BuildLightMap = (decltype(gPrivateFuncs.R_BuildLightMap))Search_Pattern(R_BUILDLIGHTMAP_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(R_BuildLightMap);

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

		}R_BuildLightMap_ctx;

		R_BuildLightMap_ctx ctx = { 0 };

		ctx.base = gPrivateFuncs.R_BuildLightMap;
		ctx.max_insts = 1000;
		ctx.max_depth = 16;
		ctx.walks.emplace_back(ctx.base, 0x1000, 0);

		while (ctx.walks.size())
		{
			auto walk = ctx.walks[ctx.walks.size() - 1];
			ctx.walks.pop_back();

			g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
					auto pinst = (cs_insn*)inst;
					auto ctx = (R_BuildLightMap_ctx*)context;

					if (gPrivateFuncs.R_AddDynamicLights)
						return TRUE;

					if (ctx->code.size() > ctx->max_insts)
						return TRUE;

					if (ctx->code.find(address) != ctx->code.end())
						return TRUE;

					ctx->code.emplace(address);

					if (!gPrivateFuncs.R_AddDynamicLights &&
						pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG)
					{
						auto nextaddr = address + instLen;
						if (nextaddr[0] == 0xE8 &&
							nextaddr[5] == 0x83 &&
							nextaddr[6] == 0xC4 &&
							nextaddr[7] == 0x04)
						{
							gPrivateFuncs.R_AddDynamicLights = (decltype(gPrivateFuncs.R_AddDynamicLights))GetCallAddress(nextaddr);
						}
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

	if (!gPrivateFuncs.R_AddDynamicLights)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_AddDynamicLights = (decltype(gPrivateFuncs.R_AddDynamicLights))Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.R_AddDynamicLights = (decltype(gPrivateFuncs.R_AddDynamicLights))Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_AddDynamicLights = (decltype(gPrivateFuncs.R_AddDynamicLights))Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.R_AddDynamicLights = (decltype(gPrivateFuncs.R_AddDynamicLights))Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(R_AddDynamicLights);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.GL_DisableMultitexture = (decltype(gPrivateFuncs.GL_DisableMultitexture))Search_Pattern(GL_DISABLEMULTITEXTURE_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.GL_DisableMultitexture = (decltype(gPrivateFuncs.GL_DisableMultitexture))Search_Pattern_From(gPrivateFuncs.R_NewMap, GL_DISABLEMULTITEXTURE_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.GL_DisableMultitexture = (decltype(gPrivateFuncs.GL_DisableMultitexture))Search_Pattern_From(gPrivateFuncs.R_NewMap, GL_DISABLEMULTITEXTURE_SIG_NEW);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.GL_DisableMultitexture = (decltype(gPrivateFuncs.GL_DisableMultitexture))Search_Pattern_From(gPrivateFuncs.R_NewMap, GL_DISABLEMULTITEXTURE_SIG_BLOB);
	}
	Sig_FuncNotFound(GL_DisableMultitexture);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.GL_EnableMultitexture = (decltype(gPrivateFuncs.GL_EnableMultitexture))Search_Pattern(GL_ENABLEMULTITEXTURE_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.GL_EnableMultitexture = (decltype(gPrivateFuncs.GL_EnableMultitexture))Search_Pattern_From(gPrivateFuncs.GL_DisableMultitexture, GL_ENABLEMULTITEXTURE_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.GL_EnableMultitexture = (decltype(gPrivateFuncs.GL_EnableMultitexture))Search_Pattern_From(gPrivateFuncs.GL_DisableMultitexture, GL_ENABLEMULTITEXTURE_SIG_NEW);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.GL_EnableMultitexture = (decltype(gPrivateFuncs.GL_EnableMultitexture))Search_Pattern_From(gPrivateFuncs.GL_DisableMultitexture, GL_ENABLEMULTITEXTURE_SIG_BLOB);
	}
	Sig_FuncNotFound(GL_EnableMultitexture);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_RenderDynamicLightmaps = (decltype(gPrivateFuncs.R_RenderDynamicLightmaps))Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_RenderDynamicLightmaps = (decltype(gPrivateFuncs.R_RenderDynamicLightmaps))Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_RenderDynamicLightmaps = (decltype(gPrivateFuncs.R_RenderDynamicLightmaps))Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_NEW);
		if(!gPrivateFuncs.R_RenderDynamicLightmaps)
			gPrivateFuncs.R_RenderDynamicLightmaps = (decltype(gPrivateFuncs.R_RenderDynamicLightmaps))Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_RenderDynamicLightmaps = (decltype(gPrivateFuncs.R_RenderDynamicLightmaps))Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_BLOB);
	}
	Sig_FuncNotFound(R_RenderDynamicLightmaps);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_DrawBrushModel = (decltype(gPrivateFuncs.R_DrawBrushModel))Search_Pattern(R_DRAWBRUSHMODEL_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_DrawBrushModel = (decltype(gPrivateFuncs.R_DrawBrushModel))Search_Pattern(R_DRAWBRUSHMODEL_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_DrawBrushModel = (decltype(gPrivateFuncs.R_DrawBrushModel))Search_Pattern(R_DRAWBRUSHMODEL_SIG_NEW);
		if(!gPrivateFuncs.R_DrawBrushModel)
			gPrivateFuncs.R_DrawBrushModel = (decltype(gPrivateFuncs.R_DrawBrushModel))Search_Pattern(R_DRAWBRUSHMODEL_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_DrawBrushModel = (decltype(gPrivateFuncs.R_DrawBrushModel))Search_Pattern(R_DRAWBRUSHMODEL_SIG_BLOB);
	}
	Sig_FuncNotFound(R_DrawBrushModel);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern(R_RECURSIVEWORLDNODE_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern_From(gPrivateFuncs.R_DrawSequentialPoly, R_RECURSIVEWORLDNODE_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern_From(gPrivateFuncs.R_DrawBrushModel, R_RECURSIVEWORLDNODE_SIG_NEW);
		if(!gPrivateFuncs.R_RecursiveWorldNode)
			gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern_From(gPrivateFuncs.R_DrawBrushModel, R_RECURSIVEWORLDNODE_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))Search_Pattern_From(gPrivateFuncs.R_DrawBrushModel, R_RECURSIVEWORLDNODE_SIG_BLOB);
	}
	Sig_FuncNotFound(R_RecursiveWorldNode);

	if (1)
	{
		const char pattern[] = R_DRAWWORLD_SIG_COMMON;
		PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
		PUCHAR SearchLimit = (PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				typedef struct
				{
					bool bFoundLeaEax;
					bool bFoundPushZero;
				}R_DrawWorld_ctx1;

				R_DrawWorld_ctx1 ctx = { 0 };

				g_pMetaHookAPI->DisasmRanges(pFound + 5, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (R_DrawWorld_ctx1*)context;

					if (instCount == 1 && pinst->id == X86_INS_LEA && 
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						(pinst->detail->x86.operands[1].mem.base == X86_REG_EBP || pinst->detail->x86.operands[1].mem.base == X86_REG_ESP))
					{
						ctx->bFoundLeaEax = true;
					}

					if (instCount == 2 && 0 == memcmp(address, "\x6A\x00", 2))
					{
						ctx->bFoundPushZero = true;
					}

					if(ctx->bFoundLeaEax && ctx->bFoundPushZero)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					return TRUE;

				}, 0, &ctx);

				if (ctx.bFoundLeaEax && ctx.bFoundPushZero)
				{
					gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x300, [](PUCHAR Candidate) {

						if (Candidate[0] == 0x81 &&
							Candidate[1] == 0xEC &&
							Candidate[4] == 0 &&
							Candidate[5] == 0)
							return TRUE;

						if (Candidate[0] == 0x55 &&
							Candidate[1] == 0x8B &&
							Candidate[2] == 0xEC &&
							Candidate[3] == 0x83)
							return TRUE;


						if (Candidate[0] == 0x55 &&
							Candidate[1] == 0x8B &&
							Candidate[2] == 0xEC &&
							Candidate[3] == 0x81)
							return TRUE;

						return FALSE;
					});

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

	if (!gPrivateFuncs.R_DrawWorld)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))Search_Pattern(R_DRAWWORLD_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))Search_Pattern_From(gPrivateFuncs.R_DrawSequentialPoly, R_DRAWWORLD_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))Search_Pattern_From(gPrivateFuncs.R_RecursiveWorldNode, R_DRAWWORLD_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))Search_Pattern_From(gPrivateFuncs.R_RecursiveWorldNode, R_DRAWWORLD_SIG_NEW);
		}
	}
	Sig_FuncNotFound(R_DrawWorld);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//inlined
	}
	else
	{
		const char pattern[] = "\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
		PUCHAR SearchBegin = (PUCHAR)gPrivateFuncs.R_RenderView;
		PUCHAR SearchLimit = SearchBegin + 0x500;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				auto target1 = GetCallAddress(pFound + 0);
				auto target2 = GetCallAddress(pFound + 5);
				auto target3 = GetCallAddress(pFound + 10);

				if (target2 == gPrivateFuncs.R_PolyBlend && target3 == gPrivateFuncs.S_ExtraUpdate)
				{
					gPrivateFuncs.R_DrawViewModel = (decltype(gPrivateFuncs.R_DrawViewModel))target1;
					break;
				}

				SearchBegin = pFound + Sig_Length(pattern);
			}
			else
			{
				break;
			}
		}

		Sig_FuncNotFound(R_DrawViewModel);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_MarkLeaves = (decltype(gPrivateFuncs.R_MarkLeaves))Search_Pattern(R_MARKLEAVES_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_MarkLeaves = (decltype(gPrivateFuncs.R_MarkLeaves))Search_Pattern(R_MARKLEAVES_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_MarkLeaves = (decltype(gPrivateFuncs.R_MarkLeaves))Search_Pattern(R_MARKLEAVES_SIG_NEW);
		if(!gPrivateFuncs.R_MarkLeaves)
			gPrivateFuncs.R_MarkLeaves = (decltype(gPrivateFuncs.R_MarkLeaves))Search_Pattern(R_MARKLEAVES_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_MarkLeaves = (decltype(gPrivateFuncs.R_MarkLeaves))Search_Pattern(R_MARKLEAVES_SIG_BLOB);
	}
	Sig_FuncNotFound(R_MarkLeaves);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.GL_BeginRendering = (decltype(gPrivateFuncs.GL_BeginRendering))Search_Pattern(GL_BEGINRENDERING_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.GL_BeginRendering = (decltype(gPrivateFuncs.GL_BeginRendering))Search_Pattern(GL_BEGINRENDERING_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.GL_BeginRendering = (decltype(gPrivateFuncs.GL_BeginRendering))Search_Pattern(GL_BEGINRENDERING_SIG_NEW);
		if(!gPrivateFuncs.GL_BeginRendering)
			gPrivateFuncs.GL_BeginRendering = (decltype(gPrivateFuncs.GL_BeginRendering))Search_Pattern(GL_BEGINRENDERING_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.GL_BeginRendering = (decltype(gPrivateFuncs.GL_BeginRendering))Search_Pattern(GL_BEGINRENDERING_SIG_BLOB);
	}
	Sig_FuncNotFound(GL_BeginRendering);

	if (g_iEngineType == ENGINE_GOLDSRC || g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		auto addr = Search_Pattern(GL_ENDRENDERING_SIG_COMMON_GOLDSRC);

		if (addr)
		{
			gPrivateFuncs.GL_EndRendering = (decltype(gPrivateFuncs.GL_EndRendering))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(addr, 0x500, [](PUCHAR Candidate) {

				if (Candidate[0] == 0x55 &&
					Candidate[1] == 0x8B &&
					Candidate[2] == 0xEC &&
					Candidate[3] == 0x83)
					return TRUE;

				return FALSE;
			});
		}
	}

	if (!gPrivateFuncs.GL_EndRendering)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.GL_EndRendering = (decltype(gPrivateFuncs.GL_EndRendering))Search_Pattern(GL_ENDRENDERING_SIG_SVENGINE);
			Sig_FuncNotFound(GL_EndRendering);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.GL_EndRendering = (decltype(gPrivateFuncs.GL_EndRendering))Search_Pattern(GL_ENDRENDERING_SIG_HL25);
			Sig_FuncNotFound(GL_EndRendering);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			if (bHasOfficialFBOSupport)
			{
				gPrivateFuncs.GL_EndRendering = (decltype(gPrivateFuncs.GL_EndRendering))Search_Pattern_From_Size(gPrivateFuncs.GL_BeginRendering, 0x100, GL_ENDRENDERING_SIG_NEW);
			}
			else
			{
				gPrivateFuncs.GL_EndRendering = (decltype(gPrivateFuncs.GL_EndRendering))Search_Pattern_From_Size(gPrivateFuncs.GL_BeginRendering, 0x100, GL_ENDRENDERING_SIG_BLOB);
			}
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.GL_EndRendering = (decltype(gPrivateFuncs.GL_EndRendering))Search_Pattern_From_Size(gPrivateFuncs.GL_BeginRendering, 0x100, GL_ENDRENDERING_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(GL_EndRendering);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.EmitWaterPolys = (decltype(gPrivateFuncs.EmitWaterPolys))Search_Pattern(EMITWATERPOLYS_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.EmitWaterPolys = (decltype(gPrivateFuncs.EmitWaterPolys))Search_Pattern(EMITWATERPOLYS_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.EmitWaterPolys = (decltype(gPrivateFuncs.EmitWaterPolys))Search_Pattern(EMITWATERPOLYS_SIG_NEW);
		if(!gPrivateFuncs.EmitWaterPolys)
			gPrivateFuncs.EmitWaterPolys = (decltype(gPrivateFuncs.EmitWaterPolys))Search_Pattern(EMITWATERPOLYS_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.EmitWaterPolys = (decltype(gPrivateFuncs.EmitWaterPolys))Search_Pattern(EMITWATERPOLYS_SIG_BLOB);
	}
	Sig_FuncNotFound(EmitWaterPolys);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		addr = (ULONG_PTR)Search_Pattern(VID_UPDATEWINDOWVARS_SIG_SVENGINE);
		Sig_AddrNotFound(VID_UpdateWindowVars);

		addr = (ULONG_PTR)Search_Pattern_From_Size((void *)addr, 0x50, "\x50\xE8");
		Sig_AddrNotFound(VID_UpdateWindowVars);

		gPrivateFuncs.VID_UpdateWindowVars = (decltype(gPrivateFuncs.VID_UpdateWindowVars))GetCallAddress(addr + 1);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.VID_UpdateWindowVars = (decltype(gPrivateFuncs.VID_UpdateWindowVars))Search_Pattern(VID_UPDATEWINDOWVARS_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.VID_UpdateWindowVars = (decltype(gPrivateFuncs.VID_UpdateWindowVars))Search_Pattern(VID_UPDATEWINDOWVARS_SIG_NEW);
		if(!gPrivateFuncs.VID_UpdateWindowVars)
			gPrivateFuncs.VID_UpdateWindowVars = (decltype(gPrivateFuncs.VID_UpdateWindowVars))Search_Pattern(VID_UPDATEWINDOWVARS_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.VID_UpdateWindowVars = (decltype(gPrivateFuncs.VID_UpdateWindowVars))Search_Pattern(VID_UPDATEWINDOWVARS_SIG_BLOB);
	}

	Sig_FuncNotFound(VID_UpdateWindowVars);

	if (1)
	{
		const char sigs1[] = "Mod_PointInLeaf: bad model";
		auto BadModel_String = Search_Pattern_Data(sigs1);
		if (!BadModel_String)
			BadModel_String = Search_Pattern_Rdata(sigs1);
		if (BadModel_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
			*(DWORD*)(pattern + 1) = (DWORD)BadModel_String;
			auto BadModel_Call = Search_Pattern(pattern);
			if (BadModel_Call)
			{
				gPrivateFuncs.Mod_PointInLeaf = (decltype(gPrivateFuncs.Mod_PointInLeaf))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(BadModel_Call, 0x100, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x56 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0x74)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					return FALSE;
				});
			}
		}
	}

	if (!gPrivateFuncs.Mod_PointInLeaf)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.Mod_PointInLeaf = (decltype(gPrivateFuncs.Mod_PointInLeaf))Search_Pattern(MOD_POINTINLEAF_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.Mod_PointInLeaf = (decltype(gPrivateFuncs.Mod_PointInLeaf))Search_Pattern(MOD_POINTINLEAF_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.Mod_PointInLeaf = (decltype(gPrivateFuncs.Mod_PointInLeaf))Search_Pattern(MOD_POINTINLEAF_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.Mod_PointInLeaf = (decltype(gPrivateFuncs.Mod_PointInLeaf))Search_Pattern(MOD_POINTINLEAF_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(Mod_PointInLeaf);

	if (1)
	{
		const char sigs1[] = "Non-sprite set to glow";
		auto NonSprite_String = Search_Pattern_Data(sigs1);
		if (!NonSprite_String)
			NonSprite_String = Search_Pattern_Rdata(sigs1);
		if (NonSprite_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B";
			*(DWORD*)(pattern + 1) = (DWORD)NonSprite_String;
			auto NonSprite_Call = Search_Pattern(pattern);
			if (NonSprite_Call)
			{
				gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(NonSprite_Call, 0x500, [](PUCHAR Candidate) {

					if (Candidate[0] == 0xD9 &&
						Candidate[1] == 0x05 &&
						Candidate[6] == 0xD8)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					return FALSE;
				});
			}
		}
	}

	if (!gPrivateFuncs.R_DrawTEntitiesOnList)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW);
			if(!gPrivateFuncs.R_DrawTEntitiesOnList)
				gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW2);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))Search_Pattern(R_DRAWTENTITIESONLIST_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(R_DrawTEntitiesOnList);

	if (g_iEngineType != ENGINE_SVENGINE)
	{
		const char pattern[] = "\x00\x00\x20\x40\xE8";
		PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
		PUCHAR SearchLimit = SearchBegin + g_dwEngineTextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				auto calltarget = GetCallAddress(pFound + 4);

				if ((ULONG_PTR)calltarget > (ULONG_PTR)g_dwEngineTextBase && (ULONG_PTR)calltarget < (ULONG_PTR)g_dwEngineTextBase + g_dwEngineTextSize)
				{
					gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))calltarget;
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

	if (!gPrivateFuncs.BuildGammaTable)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))Search_Pattern(BUILDGAMMATABLE_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))Search_Pattern(BUILDGAMMATABLE_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))Search_Pattern(BUILDGAMMATABLE_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))Search_Pattern(BUILDGAMMATABLE_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(BuildGammaTable);

	if (1)
	{
		const char pattern[] = R_DRAWPARTICLES_SIG_COMMON;
		PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
		PUCHAR SearchLimit = (PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				typedef struct
				{
					bool bFound2200h;
					bool bFound2300h;
					bool bFound303h;
					bool bFound302h;
				}R_DrawParticleSearchContext;

				R_DrawParticleSearchContext ctx = { 0 };

				g_pMetaHookAPI->DisasmRanges(pFound, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (R_DrawParticleSearchContext*)context;

					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						pinst->detail->x86.operands[0].imm == 0x2200)
					{
						ctx->bFound2200h = true;
					}

					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						pinst->detail->x86.operands[0].imm == 0x2300)
					{
						ctx->bFound2300h = true;
					}

					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						pinst->detail->x86.operands[0].imm == 0x302)
					{
						ctx->bFound302h = true;
					}

					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						pinst->detail->x86.operands[0].imm == 0x303)
					{
						ctx->bFound303h = true;
					}

					if (ctx->bFound2200h && ctx->bFound2300h && ctx->bFound302h && ctx->bFound303h)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					return TRUE;

				}, 0, &ctx);

				if (ctx.bFound2200h && ctx.bFound2300h && ctx.bFound302h && ctx.bFound303h)
				{
					gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x100, [](PUCHAR Candidate) {

						if (Candidate[0] == 0x83 &&
							Candidate[1] == 0xEC &&
							Candidate[3] == 0xA1)
							return TRUE;

						if (Candidate[0] == 0x55 &&
							Candidate[1] == 0x8B &&
							Candidate[2] == 0xEC &&
							Candidate[3] == 0x83)
							return TRUE;


						if (Candidate[0] == 0x81 &&
							Candidate[1] == 0xEC &&
							Candidate[4] == 0 &&
							Candidate[5] == 0 &&
							Candidate[6] == 0xA1)
							return TRUE;

						return FALSE;
					});

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

	if (!gPrivateFuncs.R_DrawParticles)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))Search_Pattern(R_DRAWPARTICLES_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))Search_Pattern(R_DRAWPARTICLES_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))Search_Pattern(R_DRAWPARTICLES_SIG_NEW);
			if(!gPrivateFuncs.R_DrawParticles)
				gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))Search_Pattern(R_DRAWPARTICLES_SIG_NEW2);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))Search_Pattern(R_DRAWPARTICLES_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(R_DrawParticles);

	if (g_iEngineType != ENGINE_SVENGINE)
	{
		const char pattern[] = CL_ALLOCDLIGHT_SIG_COMMON;
		PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
		PUCHAR SearchLimit = (PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				gPrivateFuncs.CL_AllocDlight = (decltype(gPrivateFuncs.CL_AllocDlight))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x80, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x53 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0x5C)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					return FALSE;
				});

				if (gPrivateFuncs.CL_AllocDlight)
					break;

				SearchBegin = pFound + Sig_Length(pattern);
			}
			else
			{
				break;
			}
		}
	}

	if (!gPrivateFuncs.CL_AllocDlight)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.CL_AllocDlight = (decltype(gPrivateFuncs.CL_AllocDlight))Search_Pattern(CL_ALLOCDLIGHT_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.CL_AllocDlight = (decltype(gPrivateFuncs.CL_AllocDlight))Search_Pattern(CL_ALLOCDLIGHT_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.CL_AllocDlight = (decltype(gPrivateFuncs.CL_AllocDlight))Search_Pattern(CL_ALLOCDLIGHT_SIG_NEW);
			if(!gPrivateFuncs.CL_AllocDlight)
				gPrivateFuncs.CL_AllocDlight = (decltype(gPrivateFuncs.CL_AllocDlight))Search_Pattern(CL_ALLOCDLIGHT_SIG_NEW2);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.CL_AllocDlight = (decltype(gPrivateFuncs.CL_AllocDlight))Search_Pattern(CL_ALLOCDLIGHT_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(CL_AllocDlight);

	if (1)
	{
		PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
		PUCHAR SearchLimit = (PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, R_GLSTUDIODRAWPOINTS_SIG_COMMON);
			if (pFound)
			{
				typedef struct
				{
					bool bFoundMov1;
					bool bFound54;
					bool bFound60;
				}R_GLStudioDrawPointsSearchContext;

				R_GLStudioDrawPointsSearchContext ctx = { 0 };

				g_pMetaHookAPI->DisasmSingleInstruction(pFound + Sig_Length(R_GLSTUDIODRAWPOINTS_SIG_COMMON), [](void* inst, PUCHAR address, size_t instLen, PVOID context) {
					auto pinst = (cs_insn*)inst;
					auto ctx = (R_GLStudioDrawPointsSearchContext*)context;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						pinst->detail->x86.operands[1].imm == 1)
					{
						ctx->bFoundMov1 = true;
					}

				}, &ctx);
				
				if (ctx.bFoundMov1)
				{
					auto candidate = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x1000, [](PUCHAR Candidate) {

						if (Candidate[0] == 0x55 &&
							Candidate[1] == 0x8B &&
							Candidate[2] == 0xEC)
						{
							if (Candidate[3] == 0x83 && Candidate[4] == 0xEC)
								return TRUE;

							if (Candidate[3] == 0x81 && Candidate[4] == 0xEC)
								return TRUE;
						}

						if (Candidate[0] == 0x83 &&
							Candidate[1] == 0xEC &&
							Candidate[3] == 0xA1)
						{
							return TRUE;
						}

						if (Candidate[0] == 0x83 &&
							Candidate[1] == 0xEC &&
							Candidate[3] == 0x8B)
						{
							return TRUE;
						}

						if (Candidate[0] == 0x81 &&
							Candidate[1] == 0xEC &&
							Candidate[4] == 0x00 &&
							Candidate[5] == 0x00 &&
							Candidate[6] == 0xA1)
						{
							return TRUE;
						}

						return FALSE;
					});

					if (candidate)
					{
						g_pMetaHookAPI->DisasmRanges(candidate, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

							auto pinst = (cs_insn*)inst;
							auto ctx = (R_GLStudioDrawPointsSearchContext*)context;

							if (pinst->id == X86_INS_MOV &&
								pinst->detail->x86.op_count == 2 &&
								pinst->detail->x86.operands[0].type == X86_OP_REG &&
								pinst->detail->x86.operands[1].type == X86_OP_MEM &&
								pinst->detail->x86.operands[1].mem.disp == 0x54)
							{
								ctx->bFound54 = true;
							}

							if (pinst->id == X86_INS_MOV &&
								pinst->detail->x86.op_count == 2 &&
								pinst->detail->x86.operands[0].type == X86_OP_REG &&
								pinst->detail->x86.operands[1].type == X86_OP_MEM &&
								pinst->detail->x86.operands[1].mem.disp == 0x60)
							{
								ctx->bFound60 = true;
							}

							if (ctx->bFound54 && ctx->bFound60)
								return TRUE;

							if (address[0] == 0xCC)
								return TRUE;

							if (pinst->id == X86_INS_RET)
								return TRUE;

							return FALSE;

							return TRUE;

							}, 0, &ctx);
					}

					if (ctx.bFound54 && ctx.bFound60)
					{
						gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))candidate;
					}

					if(gPrivateFuncs.R_GLStudioDrawPoints)
						break;
				}

				SearchBegin = pFound + Sig_Length(R_GLSTUDIODRAWPOINTS_SIG_COMMON);
			}
			else
			{
				break;
			}
		}
	}

	if (!gPrivateFuncs.R_GLStudioDrawPoints)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_HL25);
		}
		else if(g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_NEW);
			if(!gPrivateFuncs.R_GLStudioDrawPoints)
				gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_NEW2);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(R_GLStudioDrawPoints);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_StudioLighting = (decltype(gPrivateFuncs.R_StudioLighting))Search_Pattern(R_STUDIOLIGHTING_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_StudioLighting = (decltype(gPrivateFuncs.R_StudioLighting))Search_Pattern(R_STUDIOLIGHTING_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_StudioLighting = (decltype(gPrivateFuncs.R_StudioLighting))Search_Pattern(R_STUDIOLIGHTING_SIG_NEW);
		if(!gPrivateFuncs.R_StudioLighting)
			gPrivateFuncs.R_StudioLighting = (decltype(gPrivateFuncs.R_StudioLighting))Search_Pattern(R_STUDIOLIGHTING_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_StudioLighting = (decltype(gPrivateFuncs.R_StudioLighting))Search_Pattern(R_STUDIOLIGHTING_SIG_BLOB);
	}
	Sig_FuncNotFound(R_StudioLighting);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_StudioChrome = (decltype(gPrivateFuncs.R_StudioChrome))Search_Pattern(R_STUDIOCHROME_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_StudioChrome = (decltype(gPrivateFuncs.R_StudioChrome))Search_Pattern(R_STUDIOCHROME_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_StudioChrome = (decltype(gPrivateFuncs.R_StudioChrome))Search_Pattern(R_STUDIOCHROME_SIG_NEW);
		if(!gPrivateFuncs.R_StudioChrome)
			gPrivateFuncs.R_StudioChrome = (decltype(gPrivateFuncs.R_StudioChrome))Search_Pattern(R_STUDIOCHROME_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_StudioChrome = (decltype(gPrivateFuncs.R_StudioChrome))Search_Pattern(R_STUDIOCHROME_SIG_BLOB);
	}
	Sig_FuncNotFound(R_StudioChrome);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_LightLambert = (decltype(gPrivateFuncs.R_LightLambert))Search_Pattern(R_LIGHTLAMBERT_SIG_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.R_LightLambert = (decltype(gPrivateFuncs.R_LightLambert))Search_Pattern(R_LIGHTLAMBERT_SIG_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_LightLambert = (decltype(gPrivateFuncs.R_LightLambert))Search_Pattern(R_LIGHTLAMBERT_SIG_NEW);
		if(!gPrivateFuncs.R_LightLambert)
			gPrivateFuncs.R_LightLambert = (decltype(gPrivateFuncs.R_LightLambert))Search_Pattern(R_LIGHTLAMBERT_SIG_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_LightLambert = (decltype(gPrivateFuncs.R_LightLambert))Search_Pattern(R_LIGHTLAMBERT_SIG_BLOB);
	}
	Sig_FuncNotFound(R_LightLambert);

	if (1)
	{
		const char sigs1[] = "DM_Base.bmp";
		auto DM_Base_String = Search_Pattern_Data(sigs1);
		if (!DM_Base_String)
			DM_Base_String = Search_Pattern_Rdata(sigs1);
		
		if (DM_Base_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\xC7\x44\x24\x2A\x2A\x00\x00\x00";
			*(DWORD*)(pattern + 1) = (DWORD)DM_Base_String;
			auto DM_Base_Push = Search_Pattern(pattern);
			if (DM_Base_Push)
			{
				gPrivateFuncs.R_StudioSetupSkin = (decltype(gPrivateFuncs.R_StudioSetupSkin))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(DM_Base_Push, 0x300, [](PUCHAR Candidate) {

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
			}
		}
	}

	if (!gPrivateFuncs.R_StudioSetupSkin)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_StudioSetupSkin = (decltype(gPrivateFuncs.R_StudioSetupSkin))Search_Pattern(R_STUDIOSETUPSKIN_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.R_StudioSetupSkin = (decltype(gPrivateFuncs.R_StudioSetupSkin))Search_Pattern(R_STUDIOSETUPSKIN_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_StudioSetupSkin = (decltype(gPrivateFuncs.R_StudioSetupSkin))Search_Pattern(R_STUDIOSETUPSKIN_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.R_StudioSetupSkin = (decltype(gPrivateFuncs.R_StudioSetupSkin))Search_Pattern(R_STUDIOSETUPSKIN_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(R_StudioSetupSkin);

	if (1)
	{
		const char sigs1[] = "Cache_Alloc: already allocated";
		auto Cache_Alloc_String = Search_Pattern_Data(sigs1);
		if (!Cache_Alloc_String)
			Cache_Alloc_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Cache_Alloc_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
		*(DWORD*)(pattern + 1) = (DWORD)Cache_Alloc_String;
		auto Cache_Alloc_Call = Search_Pattern(pattern);
		Sig_VarNotFound(Cache_Alloc_Call);

		gPrivateFuncs.Cache_Alloc = (decltype(gPrivateFuncs.Cache_Alloc))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Cache_Alloc_Call, 0x80, [](PUCHAR Candidate) {

			if (Candidate[0] == 0x53 &&
				Candidate[1] == 0x8B &&
				Candidate[4] == 0x08)
				return TRUE;

			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC)
				return TRUE;

			return FALSE;
		});
		Sig_FuncNotFound(Cache_Alloc);
	}

	if (1)
	{
		const char sigs1[] = "Draw_MiptexTexture: Bad cached wad %s\n";
		auto Draw_MiptexTexture_String = Search_Pattern_Data(sigs1);
		if (!Draw_MiptexTexture_String)
			Draw_MiptexTexture_String = Search_Pattern_Rdata(sigs1);
		if (Draw_MiptexTexture_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8";
			*(DWORD*)(pattern + 1) = (DWORD)Draw_MiptexTexture_String;
			auto Draw_MiptexTexture_Call = Search_Pattern(pattern);
			if (Draw_MiptexTexture_Call)
			{
				gPrivateFuncs.Draw_MiptexTexture = (decltype(gPrivateFuncs.Draw_MiptexTexture))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Draw_MiptexTexture_Call, 0x80, [](PUCHAR Candidate) {

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
			}
		}
	}
	
	if(!gPrivateFuncs.Draw_MiptexTexture)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.Draw_MiptexTexture = (decltype(gPrivateFuncs.Draw_MiptexTexture))Search_Pattern(DRAW_MIPTEXTEXTURE_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.Draw_MiptexTexture = (decltype(gPrivateFuncs.Draw_MiptexTexture))Search_Pattern(DRAW_MIPTEXTEXTURE_SIG_HL25);
		}
		else
		{
			gPrivateFuncs.Draw_MiptexTexture = (decltype(gPrivateFuncs.Draw_MiptexTexture))Search_Pattern(DRAW_MIPTEXTEXTURE_SIG_NEW);
		}
	}
	Sig_FuncNotFound(Draw_MiptexTexture);
	
	if (1)
	{
		const char sigs1[] = "Failed to load custom decal for player";
		auto Draw_DecalTexture_String = Search_Pattern_Data(sigs1);
		if (!Draw_DecalTexture_String)
			Draw_DecalTexture_String = Search_Pattern_Rdata(sigs1);
		if (Draw_DecalTexture_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C";
			*(DWORD*)(pattern + 1) = (DWORD)Draw_DecalTexture_String;
			auto Draw_DecalTexture_Call = Search_Pattern(pattern);
			if (Draw_DecalTexture_Call)
			{
				gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Draw_DecalTexture_Call, 0x300, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					if (Candidate[0] == 0x81 &&
						Candidate[1] == 0xEC &&
						Candidate[4] == 0x00 &&
						Candidate[5] == 0x00 &&
						Candidate[6] == 0xA1)
						return TRUE;

					if (Candidate[0] == 0x8B &&
						Candidate[1] == 0x44 &&
						Candidate[2] == 0x24)
						return TRUE;

					if (Candidate[0] == 0x8B &&
						Candidate[1] == 0x4C &&
						Candidate[2] == 0x24)
						return TRUE;

					return FALSE;
				});
			}
		}
	}

	if (!gPrivateFuncs.Draw_DecalTexture)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))Search_Pattern(DRAW_DECALTEXTURE_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))Search_Pattern(DRAW_DECALTEXTURE_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))Search_Pattern(DRAW_DECALTEXTURE_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))Search_Pattern(DRAW_DECALTEXTURE_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(Draw_DecalTexture);

	if (1)
	{
		const char sigs1[] = "R_DrawSpriteModel:  couldn";
		auto R_DrawSpriteModel_String = Search_Pattern_Data(sigs1);
		if (!R_DrawSpriteModel_String)
			R_DrawSpriteModel_String = Search_Pattern_Rdata(sigs1);
		if (R_DrawSpriteModel_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)R_DrawSpriteModel_String;
			auto R_DrawSpriteModel_Call = Search_Pattern(pattern);
			if (R_DrawSpriteModel_Call)
			{
				gPrivateFuncs.R_DrawSpriteModel = (decltype(gPrivateFuncs.R_DrawSpriteModel))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_DrawSpriteModel_Call, 0x300, [](PUCHAR Candidate) {

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
			}
		}
	}

	if (!gPrivateFuncs.R_DrawSpriteModel)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_DrawSpriteModel = (decltype(gPrivateFuncs.R_DrawSpriteModel))Search_Pattern(R_DRAWSRPITEMODEL_SIG_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.R_DrawSpriteModel = (decltype(gPrivateFuncs.R_DrawSpriteModel))Search_Pattern(R_DRAWSRPITEMODEL_SIG_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_DrawSpriteModel = (decltype(gPrivateFuncs.R_DrawSpriteModel))Search_Pattern(R_DRAWSRPITEMODEL_SIG_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.R_DrawSpriteModel = (decltype(gPrivateFuncs.R_DrawSpriteModel))Search_Pattern(R_DRAWSRPITEMODEL_SIG_BLOB);
		}
	}
	Sig_FuncNotFound(R_DrawSpriteModel);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_LightStrength = (decltype(gPrivateFuncs.R_LightStrength))Search_Pattern(R_LIGHTSTRENGTH_SIG_SVENGINE);
		Sig_FuncNotFound(R_LightStrength);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		//Inlined
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_LightStrength = (decltype(gPrivateFuncs.R_LightStrength))Search_Pattern(R_LIGHTSTRENGTH_SIG_NEW);
		if(!gPrivateFuncs.R_LightStrength)
			gPrivateFuncs.R_LightStrength = (decltype(gPrivateFuncs.R_LightStrength))Search_Pattern(R_LIGHTSTRENGTH_SIG_NEW2);
		Sig_FuncNotFound(R_LightStrength);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_LightStrength = (decltype(gPrivateFuncs.R_LightStrength))Search_Pattern(R_LIGHTSTRENGTH_SIG_BLOB);
		Sig_FuncNotFound(R_LightStrength);
	}

	if (g_iEngineType != ENGINE_SVENGINE)
	{
		auto addr = Search_Pattern(R_ROTATEFORENTITY_GOLDSRC);
		if (addr)
		{
			gPrivateFuncs.R_RotateForEntity = (decltype(gPrivateFuncs.R_RotateForEntity))GetCallAddress((PUCHAR)addr + sizeof(R_ROTATEFORENTITY_GOLDSRC) - 1 - 1);
		}
	}

	if (!gPrivateFuncs.R_RotateForEntity)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_RotateForEntity = (decltype(gPrivateFuncs.R_RotateForEntity))Search_Pattern(R_ROTATEFORENTITY_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.R_RotateForEntity = (decltype(gPrivateFuncs.R_RotateForEntity))Search_Pattern(R_ROTATEFORENTITY_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.R_RotateForEntity = (decltype(gPrivateFuncs.R_RotateForEntity))Search_Pattern(R_ROTATEFORENTITY_NEW);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.R_RotateForEntity = (decltype(gPrivateFuncs.R_RotateForEntity))Search_Pattern(R_ROTATEFORENTITY_NEW);
		}
	}
	Sig_FuncNotFound(R_RotateForEntity);

#if 0
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//Inlined
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		//Inlined
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.BuildNormalIndexTable = (decltype(gPrivateFuncs.BuildNormalIndexTable))Search_Pattern(BUILDNORMALINDEXTABLE_SIG_NEW);
		Sig_FuncNotFound(BuildNormalIndexTable);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.BuildNormalIndexTable = (decltype(gPrivateFuncs.BuildNormalIndexTable))Search_Pattern(BUILDNORMALINDEXTABLE_SIG_BLOB);
		Sig_FuncNotFound(BuildNormalIndexTable);
	}
#endif
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//Inlined
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		//Inlined
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.R_GlowBlend = (decltype(gPrivateFuncs.R_GlowBlend))Search_Pattern(R_GLOW_BLEND_SIG_NEW);
		if(!gPrivateFuncs.R_GlowBlend)
			gPrivateFuncs.R_GlowBlend = (decltype(gPrivateFuncs.R_GlowBlend))Search_Pattern(R_GLOW_BLEND_SIG_NEW2);
		Sig_FuncNotFound(R_GlowBlend);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.R_GlowBlend = (decltype(gPrivateFuncs.R_GlowBlend))Search_Pattern(R_GLOW_BLEND_SIG_BLOB);
		Sig_FuncNotFound(R_GlowBlend);
	}

	gPrivateFuncs.SCR_BeginLoadingPlaque = (decltype(gPrivateFuncs.SCR_BeginLoadingPlaque))Search_Pattern(SCR_BEGIN_LOADING_PLAQUE);
	Sig_FuncNotFound(SCR_BeginLoadingPlaque);

	if (!gPrivateFuncs.Host_IsSinglePlayerGame)
	{
		const char sigs1[] = "setpause;";
		auto setpause_String = Search_Pattern_Data(sigs1);
		if (!setpause_String)
			setpause_String = Search_Pattern_Rdata(sigs1);
		if (setpause_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)setpause_String;
			auto setpause_PushString = (PUCHAR)Search_Pattern(pattern);
			if (setpause_PushString)
			{
				auto setpause_Function = (PUCHAR)g_pMetaHookAPI->ReverseSearchFunctionBegin(setpause_PushString, 0x50);
				if (setpause_Function)
				{
					g_pMetaHookAPI->DisasmRanges(setpause_Function, setpause_PushString - setpause_Function, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
					{
						auto pinst = (cs_insn*)inst;

						if (address[0] == 0xE8 && instLen == 5 && address[5] == 0x85 && address[6] == 0xC0)
						{
							gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))pinst->detail->x86.operands[0].imm;
						}

						if (address[0] == 0xCC)
							return TRUE;

						if (pinst->id == X86_INS_RET)
							return TRUE;

						return FALSE;
					}, 0, NULL);
				}
			}
		}
	}

	if (!gPrivateFuncs.Host_IsSinglePlayerGame)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_SVENGINE);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_HL25);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_NEW);
			if (!gPrivateFuncs.Host_IsSinglePlayerGame)
				gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_NEW2);
			if (!gPrivateFuncs.Host_IsSinglePlayerGame)
				gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_NEW3);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_BLOB);
		}
	}
	Sig_FuncNotFound(Host_IsSinglePlayerGame);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))Search_Pattern(MOD_UNLOADSPRITETEXTURES_SVENGINE);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))Search_Pattern(MOD_UNLOADSPRITETEXTURES_HL25);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))Search_Pattern(MOD_UNLOADSPRITETEXTURES_NEW);
		if(!gPrivateFuncs.Mod_UnloadSpriteTextures)
			gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))Search_Pattern(MOD_UNLOADSPRITETEXTURES_NEW2);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))Search_Pattern(MOD_UNLOADSPRITETEXTURES_BLOB);
	}
	Sig_FuncNotFound(Mod_UnloadSpriteTextures);

	//Engine's R_AddTEntity is not used anymore
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

		gPrivateFuncs.R_AddTEntity = (decltype(gPrivateFuncs.R_AddTEntity))g_pMetaHookAPI->ReverseSearchFunctionBegin(R_AddTEntity_Call, 0x50);
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

		gPrivateFuncs.R_AddTEntity = (decltype(gPrivateFuncs.R_AddTEntity))g_pMetaHookAPI->ReverseSearchFunctionBegin(R_AddTEntity_Call, 0x50);
		Sig_FuncNotFound(R_AddTEntity);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs1[] = "Hunk_Alloc: bad size: %i";
		auto Hunk_Alloc_String = Search_Pattern_Data(sigs1);
		if (!Hunk_Alloc_String)
			Hunk_Alloc_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Hunk_Alloc_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x0F\xAE\xE8\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
		*(DWORD *)(pattern + 1) = (DWORD)Hunk_Alloc_String;
		auto Hunk_Alloc_Call = Search_Pattern(pattern);
		Sig_VarNotFound(Hunk_Alloc_Call);

		gPrivateFuncs.Hunk_AllocName = (decltype(gPrivateFuncs.Hunk_AllocName))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Hunk_Alloc_Call, 0x50, [](PUCHAR Candidate) {
			//.text : 01DD29B0 83 EC 08                                                     sub     esp, 8
			//.text : 01DD29B3 53                                                           push    ebx
			//.text : 01DD29B4 55                                                           push    ebp
			//.text : 01DD29B5 8B 6C 24 14                                                  mov     ebp, [esp + 10h + arg_0]
			if (Candidate[0] == 0x83 &&
				Candidate[1] == 0xEC &&
				Candidate[2] == 0x08 &&
				Candidate[4] == 0x55 &&
				Candidate[5] == 0x8B)
				return TRUE;

			return FALSE;
		});

		Sig_FuncNotFound(Hunk_AllocName);
	}
	else
	{
		const char sigs1[] = "Hunk_Alloc: bad size: %i";
		auto Hunk_Alloc_String = Search_Pattern_Data(sigs1);
		if (!Hunk_Alloc_String)
			Hunk_Alloc_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Hunk_Alloc_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
		*(DWORD *)(pattern + 1) = (DWORD)Hunk_Alloc_String;
		auto Hunk_Alloc_Call = Search_Pattern(pattern);
		Sig_VarNotFound(Hunk_Alloc_Call);

		gPrivateFuncs.Hunk_AllocName = (decltype(gPrivateFuncs.Hunk_AllocName))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Hunk_Alloc_Call, 0x50, [](PUCHAR Candidate) {

			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC)
			{
				if (Candidate[-1] == 0x90 || Candidate[-1] == 0xCC)
				{
					return TRUE;
				}
			}
			if (Candidate[0] == 0x83 &&
				Candidate[1] == 0xEC)
			{
				return TRUE;
			}

			if (Candidate[0] >= 0x53 &&
				Candidate[0] <= 0x57 &&
				Candidate[1] == 0x8B)
			{
				return TRUE;
			}
			return FALSE;
		});
		Sig_FuncNotFound(Hunk_AllocName);
	}

#if 0
	if (1)
	{
		const char sigs1[] = "***PROTECTED***";
		auto Cvar_DirectSet_String = Search_Pattern_Data(sigs1);
		if (!Cvar_DirectSet_String)
			Cvar_DirectSet_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Cvar_DirectSet_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD *)(pattern + 1) = (DWORD)Cvar_DirectSet_String;
		auto Cvar_DirectSet_Call = Search_Pattern(pattern);
		Sig_VarNotFound(Cvar_DirectSet_Call);

		gPrivateFuncs.Cvar_DirectSet = (decltype(gPrivateFuncs.Cvar_DirectSet))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Cvar_DirectSet_Call, 0x500, [](PUCHAR Candidate) {
			//.text : 01D42120 81 EC 0C 04 00 00                                   sub     esp, 40Ch
			//.text : 01D42126 A1 E8 F0 ED 01                                      mov     eax, ___security_cookie
			//.text : 01D4212B 33 C4
			if (Candidate[0] == 0x81 &&
				Candidate[1] == 0xEC &&
				Candidate[4] == 0x00 &&
				Candidate[5] == 0x00 &&
				Candidate[6] == 0xA1 &&
				Candidate[11] == 0x33 &&
				Candidate[12] == 0xC4)
				return TRUE;

			//.text : 01D2E530 55                                                  push    ebp
			//.text : 01D2E531 8B EC                                               mov     ebp, esp
			//.text : 01D2E533 81 EC 00 04 00 00                                   sub     esp, 400h
			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC &&
				Candidate[3] == 0x81 &&
				Candidate[4] == 0xEC &&
				Candidate[7] == 0x00 &&
				Candidate[8] == 0x00)
				return TRUE;

			return FALSE;
		});
		Sig_FuncNotFound(Cvar_DirectSet);
	}
#endif

	if (!bHasOfficialFBOSupport)
	{
		s_fXMouseAspectAdjustment = &s_fXMouseAspectAdjustment_Storage;
		s_fYMouseAspectAdjustment = &s_fYMouseAspectAdjustment_Storage;
	}
	else
	{
		typedef struct
		{
			int type;
			int zero_register;
			int load_zero_instcount;
		}GL_EndRendering_ctx;

		GL_EndRendering_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.GL_EndRendering, 0x350, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (GL_EndRendering_ctx*)context;

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

					if (!s_fYMouseAspectAdjustment)
						s_fYMouseAspectAdjustment = (decltype(s_fYMouseAspectAdjustment))pinst->detail->x86.operands[0].mem.disp;
					else if (!s_fXMouseAspectAdjustment)
						s_fXMouseAspectAdjustment = (decltype(s_fXMouseAspectAdjustment))pinst->detail->x86.operands[0].mem.disp;
				}
				else if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0x3F800000)
				{//.text:01D4D4C0 A3 F4 78 E4 01 mov     videowindowaspect_0, eax

					if (!s_fYMouseAspectAdjustment)
						s_fYMouseAspectAdjustment = (decltype(s_fYMouseAspectAdjustment))pinst->detail->x86.operands[0].mem.disp;
					else if (!s_fXMouseAspectAdjustment)
						s_fXMouseAspectAdjustment = (decltype(s_fXMouseAspectAdjustment))pinst->detail->x86.operands[0].mem.disp;
				}
				else if ((pinst->id == X86_INS_FST || pinst->id == X86_INS_FSTP) &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					instCount < ctx->load_zero_instcount + 5)
				{//.D9 15 E0 85 ED 01 fst     videowindowaspect

					if (!s_fYMouseAspectAdjustment)
						s_fYMouseAspectAdjustment = (decltype(s_fYMouseAspectAdjustment))pinst->detail->x86.operands[0].mem.disp;
					else if (!s_fXMouseAspectAdjustment)
						s_fXMouseAspectAdjustment = (decltype(s_fXMouseAspectAdjustment))pinst->detail->x86.operands[0].mem.disp;
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
			}, 0, & ctx);

		Sig_VarNotFound(gl_backbuffer_fbo);
		Sig_VarNotFound(gl_msaa_fbo);
		Sig_VarNotFound(s_fXMouseAspectAdjustment);
		Sig_VarNotFound(s_fYMouseAspectAdjustment);
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

		numTransObjs = (decltype(numTransObjs))(maxTransObjs - 1);
	}

	if (1)
	{
		typedef struct
		{
			int disableFog_instcount;
			int parsemod_instcount;
			int getskin_instcount;
			PVOID r_entorigin_candidate[3];
			int r_entorigin_candidate_count;
		}R_DrawTEntitiesOnList_ctx;

		R_DrawTEntitiesOnList_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_DrawTEntitiesOnList, 0x500, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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

			if (address[0] == 0x6A && address[1] == 0x00 && address[2] == 0xE8)
			{
				//6A 00 push    0
				//E8 A3 13 05 00                                      call    GL_EnableDisableFog

				auto target = GetCallAddress((address + 2));

				typedef struct
				{
					bool bFoundGL_FOG;
				}GL_EnableDisableFog_ctx2;

				GL_EnableDisableFog_ctx2 ctx2 = { 0 };

				g_pMetaHookAPI->DisasmRanges(target, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
				{
					auto pinst = (cs_insn*)inst;
					auto ctx2 = (GL_EnableDisableFog_ctx2 *)context;

					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].imm == GL_FOG)
					{//.text:01D92330 68 60 0B 00 00 push    0B60h
						DWORD imm = pinst->detail->x86.operands[0].imm;

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
			{//D9 15 80 61 DF 08 fst     r_blend
				r_blend = (decltype(r_blend))pinst->detail->x86.operands[0].mem.disp;
			}
			if (!r_blend && ctx->disableFog_instcount &&
				instCount < ctx->disableFog_instcount + 15 &&
				(pinst->id == X86_INS_MOVSS) &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{// F3 0F 11 05 54 40 1B 11                             movss   r_blend, xmm0
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

			if (!r_entorigin &&
				ctx->getskin_instcount &&
				instCount < ctx->getskin_instcount + 20 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				//.text:01D88C23 89 15 E0 98 BC 02                                   mov     r_entorigin, edx
				auto candidate = (PVOID)pinst->detail->x86.operands[0].mem.disp;
				if (!ctx->r_entorigin_candidate[ctx->r_entorigin_candidate_count] && ctx->r_entorigin_candidate_count < 3)
				{
					bool bFound = false;
					for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
					{
						if (ctx->r_entorigin_candidate[k] == candidate)
							bFound = true;
					}
					if (!bFound)
					{
						ctx->r_entorigin_candidate[ctx->r_entorigin_candidate_count] = candidate;
						ctx->r_entorigin_candidate_count++;
					}
				}
			}

			if (!r_entorigin &&
				ctx->getskin_instcount &&
				instCount < ctx->getskin_instcount + 20 &&
				pinst->id == X86_INS_FST &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
			//.text:01D88C23 89 15 E0 98 BC 02                                   mov     r_entorigin, edx
				auto candidate = (PVOID)pinst->detail->x86.operands[0].mem.disp;
				if (!ctx->r_entorigin_candidate[ctx->r_entorigin_candidate_count] && ctx->r_entorigin_candidate_count < 3)
				{
					bool bFound = false;
					for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
					{
						if (ctx->r_entorigin_candidate[k] == candidate)
							bFound = true;
					}
					if (!bFound)
					{
						ctx->r_entorigin_candidate[ctx->r_entorigin_candidate_count] = candidate;
						ctx->r_entorigin_candidate_count++;
					}
				}
			}

			if (!r_entorigin &&
				ctx->getskin_instcount &&
				instCount < ctx->getskin_instcount + 20 &&
				pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 && 
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					//.text:101FA69B F3 0F 10 00                                         movss   xmm0, dword ptr[eax]
					//.text : 101FA69F F3 0F 11 05 E0 02 DC 10                             movss   r_entorigin, xmm0
					//r_entorigin = (decltype(r_entorigin))pinst->detail->x86.operands[1].mem.disp;

					auto candidate = (PVOID)pinst->detail->x86.operands[0].mem.disp;
					if (!ctx->r_entorigin_candidate[ctx->r_entorigin_candidate_count] && ctx->r_entorigin_candidate_count < 3)
					{
						bool bFound = false;
						for (auto k = 0; k < ctx->r_entorigin_candidate_count; ++k)
						{
							if (ctx->r_entorigin_candidate[k] == candidate)
								bFound = true;
						}
						if (!bFound)
						{
							ctx->r_entorigin_candidate[ctx->r_entorigin_candidate_count] = candidate;
							ctx->r_entorigin_candidate_count++;
						}
					}
				}

			if (r_blend && cl_parsecount && cl_frames && ctx->r_entorigin_candidate_count >= 3)
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

		if (ctx.r_entorigin_candidate_count >= 2)
		{
			std::qsort(ctx.r_entorigin_candidate, ctx.r_entorigin_candidate_count, sizeof(ctx.r_entorigin_candidate[0]), [](const void* a, const void* b) {
				return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
			});

			if ((PUCHAR)ctx.r_entorigin_candidate[0] == (PUCHAR)ctx.r_entorigin_candidate[1] - sizeof(float))
			{
				r_entorigin = (decltype(r_entorigin))ctx.r_entorigin_candidate[0];
			}
		}
		else if (ctx.r_entorigin_candidate_count == 1)
		{
			r_entorigin = (decltype(r_entorigin))ctx.r_entorigin_candidate[0];
		}

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

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_RecursiveWorldNode, 0x500, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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
				pinst->detail->x86.operands[0].size == 1 &&
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

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "SKY: ";
		auto R_LoadSkys_String = Search_Pattern_Data(sigs);
		if (!R_LoadSkys_String)
			R_LoadSkys_String = Search_Pattern_Rdata(sigs);
		Sig_VarNotFound(R_LoadSkys_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xC7\x2A\x2A\x2A\x2A\x00\x00";
		*(DWORD *)(pattern + 1) = (DWORD)R_LoadSkys_String;
		auto R_LoadSkys_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(R_LoadSkys_PushString);

		gPrivateFuncs.R_LoadSkyboxInt_SvEngine = (decltype(gPrivateFuncs.R_LoadSkyboxInt_SvEngine))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_LoadSkys_PushString, 0x600, [](PUCHAR Candidate) {
			//.text : 01D5FC10 81 EC 28 01 00 00                                   sub     esp, 128h
			//.text : 01D5FC16 A1 E8 F0 ED 01                                      mov     eax, ___security_cookie
			//.text : 01D5FC1B 33 C4 xor eax, esp
			if (Candidate[0] == 0x81 &&
				Candidate[1] == 0xEC &&
				Candidate[6] == 0xA1 &&
				Candidate[11] == 0x33 &&
				Candidate[12] == 0xC4)
				return TRUE;
			
			return FALSE;
		});

		Sig_FuncNotFound(R_LoadSkyboxInt_SvEngine);

		typedef struct
		{
			int candidate_register;
			int candidate_instcount;
			DWORD candidate_gSkyTexNumber;
		}R_LoadSkys_ctx;

		R_LoadSkys_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_LoadSkyboxInt_SvEngine, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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

		const char sigs2[] = "desert\0";
		auto R_LoadSkyBox_String = Search_Pattern_Rdata(sigs2);
		Sig_VarNotFound(R_LoadSkyBox_String);

		char pattern2[] = "\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C";
		*(DWORD *)(pattern2 + 6) = (DWORD)R_LoadSkyBox_String;
		auto R_LoadSkyBox_PushString = Search_Pattern(pattern2);
		Sig_VarNotFound(R_LoadSkyBox_PushString);


		gPrivateFuncs.R_LoadSkyBox_SvEngine = (decltype(gPrivateFuncs.R_LoadSkyBox_SvEngine))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_LoadSkyBox_PushString, 0x200, [](PUCHAR Candidate) {
			/*
.text:01D5FEF0                                     ; void __cdecl R_LoadSkyName(const char *name)
.text:01D5FEF0                                     R_LoadSkyName   proc near               ; CODE XREF: sub_1D042D0+E¡üp
.text:01D5FEF0                                                                             ; R_LoadSkys_0+5¡ýp ...
.text:01D5FEF0
.text:01D5FEF0                                     name            = dword ptr  4
.text:01D5FEF0
.text:01D5FEF0 83 3D 98 69 00 08 00                                cmp     r_loading_skybox, 0
			*/
			if (Candidate[0] == 0x83 &&
				Candidate[1] == 0x3D &&
				Candidate[6] == 0x00)
			{
				return TRUE;
			}

			return FALSE;
		});

		Sig_FuncNotFound(R_LoadSkyBox_SvEngine);

	}
	else
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

		gPrivateFuncs.R_LoadSkys = (decltype(gPrivateFuncs.R_LoadSkys))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_LoadSkys_PushString, 0x600, [](PUCHAR Candidate) {
			//.text : 01D5FC10 81 EC 28 01 00 00                                   sub     esp, 128h
			//.text : 01D5FC16 A1 E8 F0 ED 01                                      mov     eax, ___security_cookie
			//.text : 01D5FC1B 33 C4 xor eax, esp
			if (Candidate[0] == 0x81 &&
				Candidate[1] == 0xEC &&
				Candidate[6] == 0xA1 &&
				Candidate[11] == 0x33 &&
				Candidate[12] == 0xC4)
				return TRUE;

			//  .text : 01D4F9C2 55                                                  push    ebp
			//	.text : 01D4F9C3 8B EC                                               mov     ebp, esp
			//	.text : 01D4F9C5 83 EC 6C                                            sub     esp, 6Ch
			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC &&
				Candidate[3] == 0x83 &&
				Candidate[4] == 0xEC)
				return TRUE;

			if (Candidate[0] == 0x83 &&
				Candidate[1] == 0xEC &&
				Candidate[3] == 0xA1)
				return TRUE;

			return FALSE;
		});

		Sig_FuncNotFound(R_LoadSkys);

		if (bHasOfficialGLTexAllocSupport)
		{
			typedef struct
			{
				int candidate_register;
				int candidate_instcount;
				DWORD candidate_gSkyTexNumber;
			}R_LoadSkys_ctx;

			R_LoadSkys_ctx ctx = { 0 };

			g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_LoadSkys, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
					auto pinst = (cs_insn*)inst;
					auto ctx = (R_LoadSkys_ctx*)context;

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
					else if (ctx->candidate_gSkyTexNumber &&
						pinst->id == X86_INS_CMP &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == ctx->candidate_register &&
						pinst->detail->x86.operands[0].mem.disp == 0 &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						pinst->detail->x86.operands[1].imm == 0 &&
						instCount < ctx->candidate_instcount + 5)
					{//.text:102506E6 83 3E 00 cmp     dword ptr [esi], 0
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
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(g_iEngineType == ENGINE_SVENGINE ? (void *)gPrivateFuncs.R_LoadSkyBox_SvEngine : (void *)gPrivateFuncs.R_LoadSkys, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{
				DWORD imm = pinst->detail->x86.operands[1].mem.disp;

				r_loading_skybox = (decltype(r_loading_skybox))imm;
			}
			else if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{
				DWORD imm = pinst->detail->x86.operands[0].mem.disp;

				r_loading_skybox = (decltype(r_loading_skybox))imm;
			}

			if (r_loading_skybox)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(r_loading_skybox);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.GL_Bind, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.GL_SelectTexture, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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
#define GL_FILTER_SIG_SVENGINE "\xDB\x05\x2A\x2A\x2A\x2A\x2A\xD9\x1C\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x2A\xDB\x05"

		addr = (ULONG_PTR)Search_Pattern(GL_FILTER_SIG_SVENGINE);
		if (addr)
		{
			gl_filter_min = *(int**)(addr + 2);
			gl_filter_max = *(int**)(addr + Sig_Length(GL_FILTER_SIG_SVENGINE));
		}
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
#define GL_FILTER_SIG_GOLDSRC_HL25 "\x66\x0F\x6E\x05\x2A\x2A\x2A\x2A\x0F\x5B\xC0\x2A\xF3\x2A\x2A\x2A\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\x66\x0F\x6E\x05"
		addr = (ULONG_PTR)Search_Pattern(GL_FILTER_SIG_GOLDSRC_HL25);
		if (addr)
		{
			gl_filter_min = *(int**)(addr + 4);
			gl_filter_max = *(int**)(addr + Sig_Length(GL_FILTER_SIG_GOLDSRC_HL25));
		}
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
#define GL_FILTER_SIG_GOLDSRC_NEW "\xDB\x05\x2A\x2A\x2A\x2A\x83\xC4\x04\xD9\x2A\x2A\x2A\x2A\x2A\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xDB\x05"
#define GL_FILTER_SIG_GOLDSRC_NEW2 "\xDB\x05\x2A\x2A\x2A\x2A\x83\xC4\x04\xD9\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xDB\x05"

		addr = (ULONG_PTR)Search_Pattern(GL_FILTER_SIG_GOLDSRC_NEW);
		if (addr)
		{
			gl_filter_min = *(int**)(addr + 2);
			gl_filter_max = *(int**)(addr + Sig_Length(GL_FILTER_SIG_GOLDSRC_NEW));
		}
		else
		{
			addr = (ULONG_PTR)Search_Pattern(GL_FILTER_SIG_GOLDSRC_NEW2);
			if (addr)
			{
				gl_filter_min = *(int**)(addr + 2);
				gl_filter_max = *(int**)(addr + Sig_Length(GL_FILTER_SIG_GOLDSRC_NEW2));
			}
		}
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
#define GL_FILTER_SIG_BLOB "\xDB\x05\x2A\x2A\x2A\x2A\x83\xC4\x04\xD9\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x15\x2A\x2A\x2A\x2A\xDB\x05"
		addr = (ULONG_PTR)Search_Pattern(GL_FILTER_SIG_BLOB);
		if (addr)
		{
			gl_filter_min = *(int**)(addr + 2);
			gl_filter_max = *(int**)(addr + Sig_Length(GL_FILTER_SIG_BLOB));
		}
	}
	Sig_AddrNotFound(gl_filter_min);
	Sig_AddrNotFound(gl_filter_max);

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_TextureAnimation, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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
		const char sigs[] = "\xD9\x54\x24\x2A\xD9\x05\x2A\x2A\x2A\x2A\xD9\xE8";
		addr = (DWORD)Search_Pattern_From_Size((void *)gPrivateFuncs.R_SetupGL, 0x100, sigs);
		Sig_AddrNotFound(scrfov);
		scrfov = *(decltype(scrfov)*)(addr + 6);
	}
	else
	{
		const char sigs[] = "\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x16\x43";
		addr = (DWORD)Search_Pattern(sigs);
		Sig_AddrNotFound(scrfov);

		const char sigs2[] = "\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x20\x41";
		addr = (DWORD)Search_Pattern_From_Size(((PUCHAR)addr - 0x50), 0x50, sigs2);
		Sig_AddrNotFound(scrfov);
		scrfov = *(decltype(scrfov)*)(addr + 2);
	}

	if (1)
	{
		const char sigs[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
		addr = (DWORD)Search_Pattern_From_Size((void *)gPrivateFuncs.R_SetupGL, 0x700, sigs);
		Sig_AddrNotFound(gWorldToScreen);
		gWorldToScreen = *(decltype(gWorldToScreen)*)(addr + 6);
		gScreenToWorld = *(decltype(gScreenToWorld)*)(addr + 1);
	}

	if (1)
	{
		PVOID R_RenderFinalFog = NULL;

		if (g_iEngineType == ENGINE_SVENGINE)
		{
			const char sigs[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\x68\x60\x0B\x00\x00";
			addr = (DWORD)Search_Pattern_From_Size((void *)gPrivateFuncs.R_RenderScene, 0x600, sigs);
			Sig_AddrNotFound(R_RenderFinalFog);

			R_RenderFinalFog = (decltype(R_RenderFinalFog))addr;

			g_bUserFogOn = *(int **)(addr + 2);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			const char sigs[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x60\x0B\x00\x00";
			addr = (DWORD)Search_Pattern_From_Size((void *)gPrivateFuncs.R_RenderScene, 0x600, sigs);
			Sig_AddrNotFound(R_RenderFinalFog);

			R_RenderFinalFog = (decltype(R_RenderFinalFog))GetCallAddress(addr + 9);

			g_bUserFogOn = *(int **)(addr + 2);
		}
		else
		{
			const char sigs[] = "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x6A\x00";
			addr = (DWORD)Search_Pattern_From_Size((void *)gPrivateFuncs.R_RenderScene, 0x600, sigs);
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

			else if (pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//F3 0F 10 05 B4 ED 1A 11                             movss   xmm0, flFogDensity

				ctx->candidate = pinst->detail->x86.operands[1].mem.disp;
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
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_RenderScene, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (!gPrivateFuncs.CL_IsDevOverviewMode && address[0] == 0xE8 && address[5] == 0x85)
			{
				gPrivateFuncs.CL_IsDevOverviewMode = (decltype(gPrivateFuncs.CL_IsDevOverviewMode))pinst->detail->x86.operands[0].imm;
			}

			if (gPrivateFuncs.CL_IsDevOverviewMode && !gPrivateFuncs.CL_SetDevOverView && address[0] == 0xE8 && address[-5] == 0x68 && address[5] == 0x83)
			{
				if (g_iEngineType == ENGINE_SVENGINE)
				{
					r_refdef_SvEngine = *(decltype(r_refdef_SvEngine)*)(address - 4);
					r_refdef.vrect = &r_refdef_SvEngine->vrect;
					r_refdef.vieworg = &r_refdef_SvEngine->vieworg;
					r_refdef.viewangles = &r_refdef_SvEngine->viewangles;
					r_refdef.ambientlight = &r_refdef_SvEngine->ambientlight;
					r_refdef.onlyClientDraws = &r_refdef_SvEngine->onlyClientDraws;
				}
				else
				{
					r_refdef_GoldSrc = *(decltype(r_refdef_GoldSrc)*)(address - 4);
					r_refdef.vrect = &r_refdef_GoldSrc->vrect;
					r_refdef.vieworg = &r_refdef_GoldSrc->vieworg;
					r_refdef.viewangles = &r_refdef_GoldSrc->viewangles;
					r_refdef.ambientlight = &r_refdef_GoldSrc->ambientlight;
					r_refdef.onlyClientDraws = &r_refdef_GoldSrc->onlyClientDraws;
				}

				gPrivateFuncs.CL_SetDevOverView = (decltype(gPrivateFuncs.CL_SetDevOverView))pinst->detail->x86.operands[0].imm;
			}

			if (gPrivateFuncs.CL_IsDevOverviewMode && gPrivateFuncs.CL_SetDevOverView)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_FuncNotFound(CL_IsDevOverviewMode);
		Sig_FuncNotFound(CL_SetDevOverView);

		typedef struct
		{
			std::map<int, ULONG_PTR> candidate_disp;
			std::map<int, PVOID> candidate_addr;
		}R_RenderScene_ctx;

		R_RenderScene_ctx ctx2;

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_RenderScene, 0x600, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_RenderScene_ctx *)context;

			if (!cl_waterlevel && pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize&&
				pinst->detail->x86.operands[1].type == X86_OP_IMM && pinst->detail->x86.operands[1].imm == 2
				)
			{
				cl_waterlevel = (decltype(cl_waterlevel))pinst->detail->x86.operands[0].mem.disp;
			}

			if (!cl_waterlevel && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				)
			{
				ctx->candidate_addr[pinst->detail->x86.operands[0].reg] = address;
				ctx->candidate_disp[pinst->detail->x86.operands[0].reg] = pinst->detail->x86.operands[1].mem.disp;
			}

			if (!cl_waterlevel && pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm  == 2)
			{
				if (ctx->candidate_addr.find(pinst->detail->x86.operands[0].reg) != ctx->candidate_addr.end())
				{
					auto addr = (PUCHAR)ctx->candidate_addr[pinst->detail->x86.operands[0].reg];
					auto disp = ctx->candidate_disp[pinst->detail->x86.operands[0].reg];
					if (address > addr && address < addr + 0x30)
					{
						cl_waterlevel = (decltype(cl_waterlevel))disp;
					}
				}
			}

			if (cl_waterlevel)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx2);

		Sig_VarNotFound(cl_waterlevel);

		typedef struct
		{
			int Push30_instCount;
			PVOID Candidates[6];
			int CandidateCount;
		}OverviewZoomCotext_t;

		OverviewZoomCotext_t ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.CL_SetDevOverView, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto ctx = (OverviewZoomCotext_t*)context;
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
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					ctx->Candidates[ctx->CandidateCount] = (PVOID)pinst->detail->x86.operands[0].mem.disp;
					ctx->CandidateCount++;
				}
				if (pinst->id == X86_INS_MOVSS &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					ctx->Candidates[ctx->CandidateCount] = (PVOID)pinst->detail->x86.operands[1].mem.disp;
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
		}, 0, & ctx);

		std::qsort(ctx.Candidates, ctx.CandidateCount, sizeof(ctx.Candidates[0]), [](const void* a, const void* b) {
			return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
		});

		gDevOverview = (decltype(gDevOverview))ctx.Candidates[0];

		Sig_VarNotFound(gDevOverview);


	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_CullBox, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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

		char sigs_Frustum[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(sigs_Frustum + 11) = (DWORD)frustum;

		auto addr = Search_Pattern(sigs_Frustum);
		Sig_AddrNotFound(vpn);
		vpn = *(decltype(vpn)*)((PUCHAR)addr + 1);
		vup = *(decltype(vpn)*)((PUCHAR)addr + 6);

		char sigs_Frustum2[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(sigs_Frustum2 + 1) = (DWORD)vpn;
		*(DWORD*)(sigs_Frustum2 + 11) = (DWORD)frustum + 0x28;

		addr = Search_Pattern(sigs_Frustum2);
		Sig_AddrNotFound(vright);
		vright = *(decltype(vright)*)((PUCHAR)addr + 6);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs1[] = "\x8B\x15\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x1D";
		//Search in GL_LoadTexture2
		//.text : 01D4EBF4 8B 15 F0 C5 0F 03                                   mov     edx, numgltextures
		//.text : 01D4EBFA 3B F2                                               cmp     esi, edx
		//.text : 01D4EBFC 7D 4D                                               jge     short loc_1D4EC4B
		//.text : 01D4EBFE 8B 1D E4 C5 0F 03                                   mov     ebx, gltextures
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.GL_LoadTexture2, 0x300, sigs1, sizeof(sigs1) - 1);
		Sig_AddrNotFound(gltextures);
		numgltextures = *(int **)(addr + 2);
		gltextures_SvEngine = *(gltexture_t ***)(addr + 12);

		const char sigs2[] = "\x6B\xC1\x54\x89\x0D";
		//Search in GL_LoadTexture2
		//.text:01D4ED66 6B C1 54                                            imul    eax, ecx, 54h; 'T'
		//.text:01D4ED69 89 0D F0 C6 0F 03                                   mov     maxgltextures, ecx
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.GL_LoadTexture2, 0x300, sigs2, sizeof(sigs2) - 1);
		Sig_AddrNotFound(maxgltextures);
		maxgltextures_SvEngine = *(int **)(addr + 5);

		const char sigs3[] = "\x51\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)addr, 0x50, sigs3, sizeof(sigs3) - 1);
		Sig_AddrNotFound(realloc);
		gPrivateFuncs.realloc_SvEngine = (decltype(gPrivateFuncs.realloc_SvEngine))(addr + 2 + 4 + *(int *)(addr + 2));

		const char sigs4[] = "\x66\x8B\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x04";
		//66 8B 0D E0 72 40 08                                mov     cx, word ptr gHostSpawnCount
		//66 89 4B 04                                         mov     [ebx+4], cx
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.GL_LoadTexture2, 0x200, sigs4, sizeof(sigs4) - 1);
		Sig_AddrNotFound(gHostSpawnCount);
		gHostSpawnCount = *(int **)(addr + 3);

		const char sigs5[] = "\x03\x35\x2A\x2A\x2A\x2A\x3B\x15";
		//Search in GL_LoadTexture2
		//.text:01D4EDE8 03 35 EC C6 0F 03                                   add     esi, gltextures
		//.text : 01D4EDEE 3B 15 00 C7 0F 03                                   cmp     edx, peakgltextures
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.GL_LoadTexture2, 0x200, sigs5, sizeof(sigs5) - 1);
		Sig_AddrNotFound(peakgltextures);
		peakgltextures_SvEngine = *(int **)(addr + 8);
	}
	else
	{
		typedef struct
		{
			int xor_exi_exi_instCount;
			int xor_exi_exi_reg;
			int inc_exx_instCount;
			int inc_exx_reg;
			int mov_mem_exx_instCount;
			int mov_mem_exx_reg;
		}GL_LoadTexture2_Context;

		GL_LoadTexture2_Context ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.GL_LoadTexture2, 0x200, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;

			auto ctx = (GL_LoadTexture2_Context *)context;

			if (!ctx->xor_exi_exi_instCount && pinst->id == X86_INS_XOR &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				(pinst->detail->x86.operands[0].reg == X86_REG_ESI || pinst->detail->x86.operands[0].reg == X86_REG_EDI)
				&&
				pinst->detail->x86.operands[0].reg == pinst->detail->x86.operands[1].reg
				)
			{//  xor     esi, esi

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
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				)
			{//  mov     edi, offset gltextures
				gltextures = (decltype(gltextures))pinst->detail->x86.operands[1].mem.disp;
			
			}
			if (ctx->xor_exi_exi_instCount && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				(pinst->detail->x86.operands[0].reg == X86_REG_ESI || pinst->detail->x86.operands[0].reg == X86_REG_EDI)
				&&
				pinst->detail->x86.operands[0].reg != ctx->xor_exi_exi_reg &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				)
			{//  mov     edi, offset gltextures
				gltextures = (decltype(gltextures))pinst->detail->x86.operands[1].imm;
			}

			if (pinst->id == X86_INS_INC &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG )
			{// inc     ecx

				ctx->inc_exx_instCount = instCount;
				ctx->inc_exx_reg = pinst->detail->x86.operands[0].reg;
			}

			if (ctx->xor_exi_exi_instCount && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == ctx->inc_exx_reg &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				)
			{//  mov     numgltextures, ecx

				if (!bHasOfficialGLTexAllocSupport &&
					instCount > ctx->mov_mem_exx_instCount && instCount < ctx->mov_mem_exx_instCount + 5 &&
					pinst->detail->x86.operands[1].reg == ctx->mov_mem_exx_reg)
				{
					allocated_textures = (decltype(allocated_textures))pinst->detail->x86.operands[0].mem.disp;
				}
				else
				{
					numgltextures = (decltype(numgltextures))pinst->detail->x86.operands[0].mem.disp;
				}
			}

			if (ctx->xor_exi_exi_instCount && (pinst->id == X86_INS_MOV || pinst->id == X86_INS_MOVZX) &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].size == 2 &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize
				)
			{//                           mov     ax, word ptr gHostSpawnCount
				//                        movzx   eax, word ptr gHostSpawnCount
				gHostSpawnCount = (decltype(gHostSpawnCount))pinst->detail->x86.operands[1].mem.disp;

			}

			if (!bHasOfficialGLTexAllocSupport)
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

			if (!bHasOfficialGLTexAllocSupport)
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

		if (!bHasOfficialGLTexAllocSupport)
		{
			Sig_VarNotFound(allocated_textures);
		}
#if 0
		const char sigs1[] = "\x33\x2A\xBF\x2A\x2A\x2A\x2A\x3B\x35\x2A\x2A\x2A\x2A\x7D";
		//Search in GL_LoadTexture2
		//xor     esi, esi
		//mov     edi, offset gltextures
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.GL_LoadTexture2, 0x100, sigs1, sizeof(sigs1) - 1);
		Sig_AddrNotFound(gltextures);
		gltextures = *(gltexture_t **)(addr + 3);
		numgltextures = *(int **)(addr + 9);

		const char sigs2[] = "\x66\x8B\x15\x2A\x2A\x2A\x2A\x66\x89\x57\x04";
		//mov     dx, word ptr gHostSpawnCount
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.GL_LoadTexture2, 0x200, sigs2, sizeof(sigs2) - 1);
		Sig_AddrNotFound(gHostSpawnCount);
		gHostSpawnCount = *(int **)(addr + 3);
#endif
	}

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.VID_UpdateWindowVars, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;

			if (pinst->id == X86_INS_MOVUPS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//0F 11 05 F0 E9 9A 10                                movups  window_rect, xmm0

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
	else
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.VID_UpdateWindowVars, 0x40, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_DrawWorld, 0x130, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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
			else if (pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{// movss   dword ptr modelorg, xmm0

				if (!ctx->candidate[ctx->candidate_count] && ctx->candidate_count < 10)
				{
					ctx->candidate[ctx->candidate_count] = pinst->detail->x86.operands[0].mem.disp;
					ctx->candidate_count++;
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

			if (pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				pinst->detail->x86.operands[0].imm == 0x100 
				)
			{
				return TRUE;
			}

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		if (ctx.candidate_count >= 3)
		{
			std::qsort(ctx.candidate, ctx.candidate_count, sizeof(ctx.candidate[0]), [](const void*a, const void*b) {
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
		else if (ctx.candidate_count == 1)
		{
			modelorg = (decltype(modelorg))ctx.candidate[0];
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

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_RenderDynamicLightmaps, 0x150, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.CL_AllocDlight, 0x150, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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

		ctx.base = gPrivateFuncs.R_DrawSequentialPoly;
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
				else if (!lightmap_textures &&
					ctx->mov_38_instcount &&
					instCount < ctx->mov_38_instcount + 3 &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//8B 3D 40 FB C0 02                                   mov     edi, lightmap_textures

					lightmap_textures = (decltype(lightmap_textures))pinst->detail->x86.operands[1].mem.disp;
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
			DWORD blightvec_candidate;
			int blightvec_instcount;
			int blightvec_xmmreg;
			DWORD plightvec_candidate;
			int plightvec_instcount;
			int plightvec_xmmreg;
			PUCHAR mov_437F0000h_instaddr;
			PUCHAR fld_255_instaddr;
		}R_StudioLighting_ctx;

		R_StudioLighting_ctx ctx = { 0 };

		ctx.base = gPrivateFuncs.R_StudioLighting;
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

				if (!r_ambientlight &&
					pinst->id == X86_INS_MOVD &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//   movd    xmm3, r_ambientlight

					r_ambientlight = (decltype(r_ambientlight))pinst->detail->x86.operands[1].mem.disp;

					ctx->ambient_instcount = instCount;
				}

				if (!r_shadelight &&
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

				if (!r_shadelight &&
					ctx->ambient_instcount &&
					instCount < ctx->ambient_instcount + 10 &&
					pinst->id == X86_INS_MOVSS &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//   movss   xmm1, r_shadelight

					r_shadelight = (decltype(r_shadelight))pinst->detail->x86.operands[1].mem.disp;
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
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//.text:01D83BDC D9 80 48 04 79 02                                   fld     (r_blightvec+8)[eax]

					ctx->blightvec_candidate = pinst->detail->x86.operands[0].mem.disp;
					ctx->blightvec_instcount = instCount;
					ctx->blightvec_xmmreg = 0;
				}

				if (!r_blightvec &&
					ctx->lightvec_start == 2 &&
					pinst->id == X86_INS_MOVSS &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{// F3 0F 10 04 85 28 CE 20 11                          movss   xmm0, (r_blightvec+8)[eax*4]

					ctx->blightvec_candidate = pinst->detail->x86.operands[1].mem.disp;
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
					r_blightvec = (decltype(r_blightvec))(ctx->blightvec_candidate - pinst->detail->x86.operands[0].mem.disp);
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
					r_blightvec = (decltype(r_blightvec))(ctx->blightvec_candidate - pinst->detail->x86.operands[1].mem.disp);
				}

				if (!r_plightvec &&
					ctx->lightvec_start == 1 &&
					pinst->id == X86_INS_FLD &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{//.text:01D83BDC D9 80 48 04 79 02                                   fld     (r_blightvec+8)[eax]

					ctx->plightvec_candidate = pinst->detail->x86.operands[0].mem.disp;
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
					r_plightvec = (decltype(r_plightvec))(ctx->plightvec_candidate - pinst->detail->x86.operands[0].mem.disp);
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
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{// F3 0F 59 05 A8 0F DC 10                             mulss   xmm0, r_plightvec+8
					r_plightvec = (decltype(r_plightvec))(pinst->detail->x86.operands[1].mem.disp - 8);
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
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineRdataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineRdataBase + g_dwEngineRdataSize
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
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineRdataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineRdataBase + g_dwEngineRdataSize
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
					(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize )
				{

					lightgammatable = (decltype(lightgammatable))pinst->detail->x86.operands[1].imm;

				}


				if (
					ctx->fld_255_instaddr &&
					address < ctx->fld_255_instaddr + 0x80 &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[1].imm >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{

					lightgammatable = (decltype(lightgammatable))pinst->detail->x86.operands[1].imm;

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

	if (1)
	{
		typedef struct
		{
			int unused;
		}BuildGammaTable_ctx;

		BuildGammaTable_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.BuildGammaTable, 0x250, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (BuildGammaTable_ctx*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == X86_REG_ESI &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].size == 1)
			{
				texgammatable = (decltype(texgammatable))pinst->detail->x86.operands[0].mem.disp;
			}

			if(texgammatable)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, & ctx);

		Sig_VarNotFound(texgammatable);

#if 0
#define TEXGAMMATABLE_SIG "\x88\x86\x2A\x2A\x2A\x2A\x46"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.BuildGammaTable, 0x300, TEXGAMMATABLE_SIG, sizeof(TEXGAMMATABLE_SIG) - 1);
		Sig_AddrNotFound(texgammatable);
		texgammatable = *(decltype(texgammatable) *)(addr + 2);
#endif
	}

#if 0
	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define NORMALINDEX_SIG_SVENGINE "\x83\x3C\xB5\x2A\x2A\x2A\x2A\x00"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_GLStudioDrawPoints, 0x600, NORMALINDEX_SIG_SVENGINE, sizeof(NORMALINDEX_SIG_SVENGINE) - 1);
		Sig_AddrNotFound(g_NormalIndex);
		g_NormalIndex = *(decltype(g_NormalIndex) *)(addr + 3);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		//.text : 01D821E5 BF B0 64 37 02                                      mov     edi, offset g_NormalIndex
		//.text : 01D821EA F3 AB                                               rep stosd
#define NORMALINDEX_SIG_HL25 "\xBF\x2A\x2A\x2A\x2A\x83\xC8\xFF\xF3\xAB"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.R_GLStudioDrawPoints, 0x200, NORMALINDEX_SIG_HL25, sizeof(NORMALINDEX_SIG_HL25) - 1);
		Sig_AddrNotFound(g_NormalIndex);
		g_NormalIndex = *(decltype(g_NormalIndex)*)(addr + 1);
	}
	else
	{	
		//mov     edi, offset g_NormalIndex
		//rep stosd
#define G_NORMALINDEX_SIG "\xBF\x2A\x2A\x2A\x2A\xF3\xAB"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.BuildNormalIndexTable, 0x50, G_NORMALINDEX_SIG, sizeof(G_NORMALINDEX_SIG) - 1);
		Sig_AddrNotFound(g_NormalIndex);
		g_NormalIndex = *(int(**)[MAXSTUDIOVERTS])(addr + 1);
	}
#endif

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CHROMEAGE_SIG_SVENGINE "\xBF\x2A\x2A\x2A\x2A\xF3\xAB\x33\xFF\x39"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_GLStudioDrawPoints, 0x600, CHROMEAGE_SIG_SVENGINE, sizeof(CHROMEAGE_SIG_SVENGINE) - 1);
		Sig_AddrNotFound(chromeage);
		chromeage = *(decltype(chromeage) *)(addr + 1);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
#define CHROMEAGE_SIG_HL25 "\x33\xC0\xBF\x2A\x2A\x2A\x2A\xF3\xAB\x8B"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.R_GLStudioDrawPoints, 0x600, CHROMEAGE_SIG_HL25, sizeof(CHROMEAGE_SIG_HL25) - 1);
		Sig_AddrNotFound(chromeage);
		chromeage = *(decltype(chromeage)*)(addr + 3);
	}
	else
	{
#define CHROMEAGE_SIG "\x8B\x04\xB5\x2A\x2A\x2A\x2A\x3B\xC1"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_StudioChrome, 0x50, CHROMEAGE_SIG, sizeof(CHROMEAGE_SIG) - 1);
		Sig_AddrNotFound(chromeage);
		chromeage = *(int(**)[MAXSTUDIOBONES])((DWORD)addr + 3);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CHROME_SIG_SVENGINE "\xC1\xE8\x1F\x03\xC2\x8D\x04\xC5\x2A\x2A\x2A\x2A\x50\xE8"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_GLStudioDrawPoints, 0x1000, CHROME_SIG_SVENGINE, sizeof(CHROME_SIG_SVENGINE) - 1);
		Sig_AddrNotFound(chrome);
		chrome = *(decltype(chrome) *)(addr + 8);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
#define CHROME_SIG_HL25 "\x8D\x04\xFD\x2A\x2A\x2A\x2A\xFF\xB5\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.R_GLStudioDrawPoints, 0x1000, CHROME_SIG_HL25, sizeof(CHROME_SIG_HL25) - 1);
		Sig_AddrNotFound(chrome);
		chrome = *(decltype(chrome)*)(addr + 3);
	}
	else
	{
#define CHROME_SIG_NEW "\x8D\x0C\xD5\x2A\x2A\x2A\x2A\x51\xE8"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_GLStudioDrawPoints, 0x600, CHROME_SIG_NEW, sizeof(CHROME_SIG_NEW) - 1);
		Sig_AddrNotFound(chrome);
		chrome = *(decltype(chrome) *)(addr + 3);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CL_VIEWENTITY_SIG_SVENGINE "\x68\x2A\x2A\x2A\x2A\x50\x6A\x06\xFF\x35\x2A\x2A\x2A\x2A\xE8"
		DWORD addr = (DWORD)Search_Pattern(CL_VIEWENTITY_SIG_SVENGINE);
		Sig_AddrNotFound(cl_viewentity);
		cl_viewentity = *(decltype(cl_viewentity)*)(addr + 10);
	}
	else
	{
#define CL_VIEWENTITY_SIG_GOLDSRC "\xA1\x2A\x2A\x2A\x2A\x48\x3B\x2A"
		DWORD addr = (DWORD)Search_Pattern(CL_VIEWENTITY_SIG_GOLDSRC);
		Sig_AddrNotFound(cl_viewentity);

		typedef struct
		{
			bool found_cmp_200;
		}CL_ViewEntity_ctx;

		CL_ViewEntity_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges((PVOID)addr, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (CL_ViewEntity_ctx*)context;

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0x200)
			{
				ctx->found_cmp_200 = true;
			}

			if (ctx->found_cmp_200)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, & ctx);

		if (ctx.found_cmp_200)
		{
			cl_viewentity = *(decltype(cl_viewentity)*)(addr + 1);
		}

		Sig_VarNotFound(cl_viewentity);
#if 0
#define CL_VIEWENTITY_SIG_NEW "\x8B\x0D\x2A\x2A\x2A\x2A\x6A\x64\x6A\x00\x68\x00\x00\x80\x3F\x68\x00\x00\x80\x3F\x68\x2A\x2A\x2A\x2A\x50"
		DWORD addr = (DWORD)Search_Pattern(CL_VIEWENTITY_SIG_NEW);
		Sig_AddrNotFound(cl_viewentity);
		cl_viewentity = *(decltype(cl_viewentity)*)(addr + 2);
#endif
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CL_MAXEDICTS_SIG_SVENGINE "\x69\xC0\xB8\x0B\x00\x00\x50\xE8\x2A\x2A\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A\xA3\x2A\x2A\x2A\x2A\xE8"
		DWORD addr = (DWORD)Search_Pattern(CL_MAXEDICTS_SIG_SVENGINE);
		Sig_AddrNotFound(cl_max_edicts);
		cl_max_edicts = *(decltype(cl_max_edicts)*)(addr + 14);
		cl_entities = *(decltype(cl_entities)*)(addr + 19);

#define GTEMPENTS_SIG_SVENGINE "\x68\x00\xE0\x5F\x00\x6A\x00\x68\x2A\x2A\x2A\x2A\xA3"
		if (1)
		{
			DWORD addr = (DWORD)Search_Pattern(GTEMPENTS_SIG_SVENGINE);
			Sig_AddrNotFound(gTempEnts);
			gTempEnts = *(decltype(gTempEnts)*)(addr + 8);
		}

	}
	else
	{
		//Seach "CL_Reallocate cl_entities"
		const char sigs1[] = "CL_Reallocate cl_entities\n";
		auto CL_Reallocate_String = Search_Pattern_Data(sigs1);
		if (!CL_Reallocate_String)
			CL_Reallocate_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(CL_Reallocate_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1";
		*(DWORD*)(pattern + 1) = (DWORD)CL_Reallocate_String;
		auto CL_Reallocate_Call = Search_Pattern(pattern);
		Sig_VarNotFound(CL_Reallocate_Call);

		auto CL_ReallocateDynamicData = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(CL_Reallocate_Call, 0x100, [](PUCHAR Candidate) {

			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC)
			{
				return TRUE;
			}

			if (Candidate[0] == 0x8B && 
				Candidate[1] == 0x44 &&
				Candidate[2] == 0x24)
			{
				return TRUE;
			}

			return FALSE;
		});

		Sig_VarNotFound(CL_ReallocateDynamicData);

		typedef struct
		{
			PVOID CL_Reallocate_Call;
		}CL_ReallocateDynamicData_ctx;

		CL_ReallocateDynamicData_ctx ctx = { 0 };

		ctx.CL_Reallocate_Call = CL_Reallocate_Call;

		g_pMetaHookAPI->DisasmRanges(CL_ReallocateDynamicData, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (CL_ReallocateDynamicData_ctx*)context;

			if (!cl_max_edicts && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				// mov     eax, cl_max_edicts
				// add     esp, 4

				if (0 == memcmp(address + instLen, "\x83\xC4\x04", 3))
				{
					cl_max_edicts = (decltype(cl_max_edicts))pinst->detail->x86.operands[1].mem.disp;
				}
			}

			if (!cl_entities && address > ctx->CL_Reallocate_Call && pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				// mov     eax, cl_max_edicts
				// add     esp, 4
				cl_entities = (decltype(cl_entities))pinst->detail->x86.operands[0].mem.disp;
			}

			if (cl_entities && cl_max_edicts)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(cl_max_edicts);
		Sig_VarNotFound(cl_entities);

#if 0
#define CL_MAXEDICTS_SIG_NEW "\xC1\xE1\x03\x51\xE8\x2A\x2A\x2A\x2A\x8B\x15\x2A\x2A\x2A\x2A\xA3"
		DWORD addr = (DWORD)Search_Pattern(CL_MAXEDICTS_SIG_NEW);
		Sig_AddrNotFound(cl_max_edicts);
		cl_max_edicts = *(decltype(cl_max_edicts)*)(addr + 11);
		cl_entities = *(decltype(cl_entities)*)(addr + 16);
#endif

#define GTEMPENTS_SIG_NEW "\x68\x30\x68\x17\x00\x6A\x00\x68\x2A\x2A\x2A\x2A\xE8"
		if (1)
		{
			DWORD addr = (DWORD)Search_Pattern(GTEMPENTS_SIG_NEW);
			Sig_AddrNotFound(gTempEnts);
			gTempEnts = *(decltype(gTempEnts)*)(addr + 8);
		}
	}

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
#define GWATERCOLOR_SIG_HL25 "\x66\x0F\x6E\x05\x2A\x2A\x2A\x2A\xF2\x0F\x10\x0D\x2A\x2A\x2A\x2A\xF3\x0F\xE6\xC0\x68\x01\x26\x00\x00"
		auto gWaterColor_Pattern = Search_Pattern(GWATERCOLOR_SIG_HL25);
		Sig_VarNotFound(gWaterColor_Pattern);
		gWaterColor = *(decltype(gWaterColor)*)((PUCHAR)gWaterColor_Pattern + 4);
		cshift_water = (decltype(cshift_water))((PUCHAR)gWaterColor + 12);
	}
	else
	{
#define GWATERCOLOR_SIG "\xDB\x05\x2A\x2A\x2A\x2A\x68\x01\x26\x00\x00\x68\x65\x0B\x00\x00"
		auto gWaterColor_Pattern = Search_Pattern(GWATERCOLOR_SIG);
		Sig_VarNotFound(gWaterColor_Pattern);
		gWaterColor = *(decltype(gWaterColor)*)((PUCHAR)gWaterColor_Pattern + 2);
		cshift_water = (decltype(cshift_water))((PUCHAR)gWaterColor + 12);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.GL_EnableMultitexture, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_RenderView_SvEngine, 0x800, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(c_brush_polys);
		c_alias_polys = *(int **)(addr + 2);
		c_brush_polys = *(int **)(addr + 14);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		const char sigs[] = "\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.R_RenderView, 0x100, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(c_brush_polys);
		c_brush_polys = *(int**)(addr + 2);
		c_alias_polys = *(int**)(addr + 12);
	}
	else
	{
		const char sigs[] = "\xA1\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x50\x51";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_RenderView, 0x150, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(c_brush_polys);
		c_alias_polys = *(int **)(addr + 1);
		c_brush_polys = *(int **)(addr + 7);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "\xF6\xC4\x44\x0F\x2A\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F";
		addr = (ULONG_PTR)Search_Pattern_From_Size(gPrivateFuncs.R_RenderView_SvEngine, 0x500, sigs);
		Sig_AddrNotFound(envmap);
		envmap = *(int **)(addr + 11);
		cl_stats = *(int **)(addr + 24);

		const char sigs3[] = "\xD9\x1D\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x89\x81\xDC\x02\x00\x00";
		addr = (ULONG_PTR)Search_Pattern_From_Size(addr, 0x300, sigs3);
		Sig_AddrNotFound(cl_weaponstarttime);
		cl_weaponstarttime = *(float **)(addr + 2);
		cl_weaponsequence = *(int **)(addr + 7);

		const char sigs4[] = "\xD1\xEA\x89\x15\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
		addr = (ULONG_PTR)Search_Pattern_From_Size(addr, 0x300, sigs4);
		Sig_AddrNotFound(cl_light_level);
		cl_light_level = *(int **)(addr + 4);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		const char sigs[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\xF3\x0F\x10\x05";
		addr = (ULONG_PTR)Search_Pattern_From_Size(gPrivateFuncs.R_DrawViewModel, 0x100, sigs);
		Sig_AddrNotFound(envmap);
		envmap = *(int**)(addr + 2);

		const char sigs2[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x83\xB9\x94\x0B\x00\x00\x00";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x50, sigs2);
		Sig_AddrNotFound(cl_stats);
		cl_stats = *(int**)(addr + 2);

		const char sigs3[] = "\xF3\x0F\x11\x05\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x89\x81\xDC\x02\x00\x00";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x500, sigs3);
		Sig_AddrNotFound(cl_weaponstarttime);
		cl_weaponstarttime = *(float**)(addr + 4);
		cl_weaponsequence = *(int**)(addr + 9);

		const char sigs4[] = "\xD1\xEA\x89\x15\x2A\x2A\x2A\x2A\x8B\x40\x04";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x600, sigs4);
		Sig_AddrNotFound(cl_light_level);
		cl_light_level = *(int**)(addr + 4);
	}
	else
	{
		const char sigs[] = "\x39\x3D\x2A\x2A\x2A\x2A\x0F\x2A\x2A\x2A\x2A\x2A\xD9\x05";
		addr = (ULONG_PTR)Search_Pattern_From_Size(gPrivateFuncs.R_DrawViewModel, 0x100, sigs);
		Sig_AddrNotFound(envmap);
		envmap = *(int **)(addr + 2);

		const char sigs2[] = "\x39\x3D\x2A\x2A\x2A\x2A\x0F\x2A\x2A\x2A\x2A\x2A\xA1";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void *)addr, 0x50, sigs2);
		Sig_AddrNotFound(cl_stats);
		cl_stats = *(int **)(addr + 2);

		const char sigs3[] = "\xD9\x1D\x2A\x2A\x2A\x2A\xA1";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void *)addr, 0x500, sigs3);
		Sig_AddrNotFound(cl_weaponstarttime);
		cl_weaponstarttime = *(float **)(addr + 2);
		cl_weaponsequence = *(int **)(addr + 7);

		const char sigs4[] = "\x89\x15\x2A\x2A\x2A\x2A\x89\x2A\x2A\x2A\x2A\x2A\xFF";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void *)addr, 0x600, sigs4);
		Sig_AddrNotFound(cl_light_level);
		cl_light_level = *(int **)(addr + 2);

	}

	if (1)
	{
#define R_WORLD_MATRIX_SIG "\x68\x2A\x2A\x2A\x2A\x68\xA6\x0B\x00\x00"
		addr = (DWORD)Search_Pattern_From_Size((void*)gPrivateFuncs.R_SetupGL, 0x600, R_WORLD_MATRIX_SIG);
		Sig_AddrNotFound(r_world_matrix);
		r_world_matrix = *(float**)(addr + 1);

#define R_PROJ_MATRIX_SIG "\x68\x2A\x2A\x2A\x2A\x68\xA7\x0B\x00\x00"
		addr = (DWORD)Search_Pattern_From_Size((void*)gPrivateFuncs.R_SetupGL, 0x500, R_PROJ_MATRIX_SIG);
		Sig_AddrNotFound(r_projection_matrix);
		r_projection_matrix = *(float**)(addr + 1);

#define TMP_PALETTE_SIG "\x68\x2A\x2A\x2A\x2A\x6A\x00\x6A\x00"
		addr = (DWORD)Search_Pattern_From_Size((void*)gPrivateFuncs.R_StudioSetupSkin, 0x600, TMP_PALETTE_SIG);
		Sig_AddrNotFound(tmp_palette);
		tmp_palette = *(void**)(addr + 1);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define VERTICAL_FOV_SIG_SVENGINE "\x50\xFF\x15\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00"
		addr = (DWORD)Search_Pattern_From_Size((void*)gPrivateFuncs.R_SetupGL, 0x120, VERTICAL_FOV_SIG_SVENGINE);
		Sig_AddrNotFound(vertical_fov_SvEngine);
		vertical_fov_SvEngine = *(decltype(vertical_fov_SvEngine)*)(addr + 9);
	}

	if (1)
	{
		typedef struct
		{
			PVOID candidate_E8;
		}R_StudioSetupSkin_ctx;

		R_StudioSetupSkin_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_StudioSetupSkin, 0x800, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_StudioSetupSkin_ctx*)context;

			if (address[0] == 0xE8 && instLen == 5)
			{
				ctx->candidate_E8 = (decltype(ctx->candidate_E8))pinst->detail->x86.operands[0].imm;

				if (!gPrivateFuncs.R_StudioGetSkin)
				{
					typedef struct
					{
						bool bFoundCmp0B;
					}R_StudioSetupSkin_SearchCmp0B_ctx;

					R_StudioSetupSkin_SearchCmp0B_ctx ctx2 = { 0 };

					g_pMetaHookAPI->DisasmRanges(ctx->candidate_E8, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
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
						gPrivateFuncs.R_StudioGetSkin = (decltype(gPrivateFuncs.R_StudioGetSkin))ctx->candidate_E8;
					}
				}
			}

			if (!gPrivateFuncs.GL_UnloadTexture && ctx->candidate_E8)
			{
				if (address[0] == 0x6A && address[1] == 0x00 && address[2] == 0x6A && address[3] == 0x00)
				{
					gPrivateFuncs.GL_UnloadTexture = (decltype(gPrivateFuncs.GL_UnloadTexture))ctx->candidate_E8;
					return TRUE;
				}
			}

			if (gPrivateFuncs.GL_UnloadTexture && gPrivateFuncs.R_StudioGetSkin)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_FuncNotFound(R_StudioGetSkin);
		Sig_FuncNotFound(GL_UnloadTexture);
	}

#if 0//unused
	if (1)
	{
		typedef struct
		{
			PVOID unused;
		}R_StudioGetSkin_ctx;

		R_StudioGetSkin_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_StudioGetSkin, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_StudioGetSkin_ctx*)context;

			if (!pDM_RemapSkin &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.scale == 4 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				pDM_RemapSkin = (decltype(pDM_RemapSkin))pinst->detail->x86.operands[1].mem.disp;
			}

			if (!r_remapindex &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{
				r_remapindex = (decltype(r_remapindex))pinst->detail->x86.operands[0].mem.disp;
			}

			if (!DM_RemapSkin &&
				pinst->id == X86_INS_ADD &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				DM_RemapSkin = (decltype(DM_RemapSkin))pinst->detail->x86.operands[1].imm;
			}

			if (!DM_RemapSkin &&
				pinst->id == X86_INS_LEA &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				DM_RemapSkin = (decltype(DM_RemapSkin))pinst->detail->x86.operands[1].mem.disp;
			}

			if (pDM_RemapSkin && DM_RemapSkin && r_remapindex)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(pDM_RemapSkin);
		Sig_VarNotFound(DM_RemapSkin);
		Sig_VarNotFound(r_remapindex);
	}
#endif
	if (1)
	{
		typedef struct
		{
			PVOID candidate_E8;
			bool bFoundRet;
		}R_NewMap_ctx;

		R_NewMap_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_NewMap, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_NewMap_ctx*)context;

			if (address[0] == 0xE8 && instLen == 5)
			{
				ctx->candidate_E8 = (decltype(ctx->candidate_E8))pinst->detail->x86.operands[0].imm;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
			{
				ctx->bFoundRet = true;
				return TRUE;
			}

			return FALSE;
		}, 0, &ctx);

		if (ctx.bFoundRet && ctx.candidate_E8)
		{
			gPrivateFuncs.GL_UnloadTextures = (decltype(gPrivateFuncs.GL_UnloadTextures))ctx.candidate_E8;
		}

		Sig_FuncNotFound(GL_UnloadTextures);
	}

	if(1)
	{
#define MOD_KNOWN_SIG "\xB8\x9D\x82\x97\x53\x81\xE9"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(MOD_KNOWN_SIG);
		Sig_AddrNotFound(mod_known);
		mod_known = *(model_t **)(addr + 7);
	}

	if(1)
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

		Sig_VarNotFound(mod_numknown);

	}

	if(1)
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

		gPrivateFuncs.Mod_LoadStudioModel = (decltype(gPrivateFuncs.Mod_LoadStudioModel))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Bogus_Call, 0x50, [](PUCHAR Candidate) {

			//  .text : 01D71630 81 EC 10 01 00 00                                   sub     esp, 110h
			if (Candidate[0] == 0x81 &&
				Candidate[1] == 0xEC &&
				Candidate[4] == 0x00 &&
				Candidate[5] == 0x00)
			{
				return TRUE;
			}
			//  .text : 01D61AD0 55                                                  push    ebp
			//  .text : 01D61AD1 8B EC                                               mov     ebp, esp
			//	.text : 01D61AD3 81 EC 0C 01 00 00                                   sub     esp, 10Ch
			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC &&
				Candidate[3] == 0x81 &&
				Candidate[4] == 0xEC &&
				Candidate[7] == 0x00 &&
				Candidate[8] == 0x00 )
			{
				return TRUE;
			}

			return FALSE;
		});
		Sig_FuncNotFound(Mod_LoadStudioModel);
	}

	{
		const char sigs1[] = "Mod_LoadBrushModel: %s has wrong version number";
		auto Bogus_String = Search_Pattern_Data(sigs1);
		if (!Bogus_String)
			Bogus_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Bogus_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x6A\x01\xE8";
		*(DWORD *)(pattern + 1) = (DWORD)Bogus_String;
		auto Bogus_Call = Search_Pattern(pattern);
		if (!Bogus_Call) {
			char pattern2[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83";
			*(DWORD*)(pattern2 + 1) = (DWORD)Bogus_String;
			Bogus_Call = Search_Pattern(pattern2);
		}
		Sig_VarNotFound(Bogus_Call);

		gPrivateFuncs.Mod_LoadBrushModel = (decltype(gPrivateFuncs.Mod_LoadStudioModel))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Bogus_Call, 0x150, [](PUCHAR Candidate) {

			//.text:01D078D0 83 EC 34                                            sub     esp, 34h
			//.text : 01D078D3 A1 E8 F0 ED 01                                      mov     eax, ___security_cookie
			//	.text : 01D078D8 33 C4                                               xor     eax, esp
			if (Candidate[0] == 0x83 &&
				Candidate[1] == 0xEC &&
				Candidate[3] == 0xA1)
			{
				return TRUE;
			}
			//  .text : 01D61AD0 55                                                  push    ebp
			//  .text : 01D61AD1 8B EC                                               mov     ebp, esp
			//	.text : 01D61AD3 83 EC ??											sub     esp, ??
			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC &&
				Candidate[3] == 0x83 &&
				Candidate[4] == 0xEC)
			{
				return TRUE;
			}

			return FALSE;
		});
		Sig_FuncNotFound(Mod_LoadBrushModel);
	}

	if(1)
	{
		const char sigs1[] = "\xFF\x15\x2A\x2A\x2A\x2A\x83\xC4\x0C\x3D\x49\x44\x50\x4F";
		auto Mod_LoadModel_Pattern = Search_Pattern(sigs1);
		Sig_VarNotFound(Mod_LoadModel_Pattern);

		gPrivateFuncs.Mod_LoadModel = (decltype(gPrivateFuncs.Mod_LoadModel))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Mod_LoadModel_Pattern, 0x600, [](PUCHAR Candidate) {

			//81 EC ?? 01 00 00 A1 ?? ?? ?? ?? 33 C4
			/*
.text:01D51990 81 EC 50 01 00 00                                            sub     esp, 150h
			*/
			if (Candidate[0] == 0x81 &&
				Candidate[1] == 0xEC &&
				Candidate[3] == 0x01 &&
				Candidate[4] == 0x00 &&
				Candidate[5] == 0x00)
			{
				return TRUE;
			}

			//.text : 01D40030 55                                                  push    ebp
			//.text : 01D40031 8B EC                                               mov     ebp, esp
			//.text : 01D40033 81 EC 0C 01 00 00                                   sub     esp, 10Ch
			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC &&
				Candidate[3] == 0x81 &&
				Candidate[4] == 0xEC &&
				Candidate[6] == 0x01 &&
				Candidate[7] == 0x00 &&
				Candidate[8] == 0x00)
			{
				return TRUE;
			}

			return FALSE;
		});
		Sig_FuncNotFound(Mod_LoadModel);

		const char sigs2[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x68";
		auto Mod_LoadModel_Pattern2 = g_pMetaHookAPI->SearchPattern((PUCHAR)Mod_LoadModel_Pattern - 0x50, 0x50, sigs2, Sig_Length(sigs2));
		Sig_VarNotFound(Mod_LoadModel_Pattern2);

		loadname = *(decltype(loadname)*)((PUCHAR)Mod_LoadModel_Pattern2 + sizeof(sigs2) - 1);

		g_pMetaHookAPI->DisasmRanges((PUCHAR)Mod_LoadModel_Pattern2 + sizeof(sigs2) - 1 + 4, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn *)inst;

				if (!loadmodel &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					loadmodel = (decltype(loadmodel))pinst->detail->x86.operands[0].mem.disp;
				}

				if (loadmodel)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, NULL);

		Sig_VarNotFound(loadmodel);
	}


	{
		/*const char sigs1[] = "palette.lmp\0";
		auto palette_String = Search_Pattern_Data(sigs1);
		if (!palette_String)
			palette_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(palette_String);*/

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x68\x00\x08\x00\x00\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\xA3";
		//*(DWORD *)(pattern + 1) = (DWORD)palette_String;
		auto palette_Call = Search_Pattern(pattern);
		Sig_VarNotFound(palette_Call);

		host_basepal = (decltype(host_basepal))*(void **)((PUCHAR)palette_Call + sizeof(pattern) - 1);
		Sig_VarNotFound(host_basepal);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.Draw_DecalTexture, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_DrawParticles, 0x150, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (!particletexture &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				particletexture = (decltype(particletexture))pinst->detail->x86.operands[0].mem.disp;
			}

			if (!particletexture &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				//Skip this shit
				//.text:101EBCA6 A1 F4 36 32 10                                      mov     eax, ___security_cookie
				//.text:101EBCAB 33 C5 xor eax, ebp
				if (address[instLen] == 0x33 && address[instLen + 1] == 0xC5)
				{

				}
				else
				{
					particletexture = (decltype(particletexture))pinst->detail->x86.operands[1].mem.disp;
				}
			}

			if (!active_particles &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_ESI &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				if (address[-5] == 0xE8)
				{
					gPrivateFuncs.R_FreeDeadParticles = (decltype(gPrivateFuncs.R_FreeDeadParticles))GetCallAddress(address - 5);
					active_particles = (decltype(active_particles))pinst->detail->x86.operands[1].mem.disp;
				}
			}

			if (particletexture && active_particles)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(particletexture);
	}

	if (1)
	{
#define R_TRACERDRAW_SIG "\xFF\x15\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x00\xE8\x2A\x2A\x2A\x2A"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_DrawParticles, 0x800, R_TRACERDRAW_SIG, sizeof(R_TRACERDRAW_SIG) - 1);
		Sig_AddrNotFound(R_TracerDraw);
		gPrivateFuncs.R_TracerDraw = (decltype(gPrivateFuncs.R_TracerDraw))GetCallAddress(addr + 6);
		gPrivateFuncs.R_BeamDrawList = (decltype(gPrivateFuncs.R_BeamDrawList))GetCallAddress(addr + 11);
	}

	if (0)
	{
#define OVERVIEW_ZOOM_STRING " Overview: Zoom %.2f"
		auto OverviewZoom_String = Search_Pattern_Data(OVERVIEW_ZOOM_STRING);
		if (!OverviewZoom_String)
			OverviewZoom_String = Search_Pattern_Rdata(OVERVIEW_ZOOM_STRING);
		Sig_VarNotFound(OverviewZoom_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x38";
		*(DWORD*)(pattern + 1) = (DWORD)OverviewZoom_String;
		auto OverviewZoom_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(OverviewZoom_PushString);

		gPrivateFuncs.CL_SetDevOverView = (decltype(gPrivateFuncs.CL_SetDevOverView))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(OverviewZoom_PushString, 0x100, [](PUCHAR Candidate) {

			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC)
			{
				return TRUE;
			}

			return FALSE;
		});
		Sig_FuncNotFound(CL_SetDevOverView);

#if 0
#define DEVOVERVIEW_SIG "\x83\xEC\x30\xDD\x5C\x24\x2A\xD9\x05"
		addr = (DWORD)Search_Pattern(DEVOVERVIEW_SIG);
		Sig_AddrNotFound(gDevOverview);
		gDevOverview = (decltype(gDevOverview))(*(DWORD *)(addr + 9) - 0xC);
#endif
	}

#define R_SETUPFRAME_CALL_SIG "\x0F\x9F\xC0\x50\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8"
	if (gPrivateFuncs.R_SetupFrame)
	{
		addr = (ULONG_PTR)Search_Pattern_From(gPrivateFuncs.R_SetupFrame, R_SETUPFRAME_CALL_SIG);
		if (addr)
		{
			gPrivateFuncs.R_ForceCVars = (decltype(gPrivateFuncs.R_ForceCVars))GetCallAddress(addr + 3 + 1);
			gPrivateFuncs.R_CheckVariables = (decltype(gPrivateFuncs.R_CheckVariables))GetCallAddress(addr + 3 + 1 + 5);
			gPrivateFuncs.R_AnimateLight = (decltype(gPrivateFuncs.R_AnimateLight))GetCallAddress(addr + 3 + 1 + 5 + 5);
		}
		else
		{
#define R_SETUPFRAME_CALL_SIG2 "\x83\xC4\x08\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1"
			addr = (ULONG_PTR)Search_Pattern_From(gPrivateFuncs.R_SetupFrame, R_SETUPFRAME_CALL_SIG2);
			//R_ForceCVars has been inlined into R_SetupFrame
			if (addr)
			{
				gPrivateFuncs.R_CheckVariables = (decltype(gPrivateFuncs.R_CheckVariables))GetCallAddress(addr + 3);
				gPrivateFuncs.R_AnimateLight = (decltype(gPrivateFuncs.R_AnimateLight))GetCallAddress(addr + 3 + 5);
			}
		}
	}
	else
	{
		addr = (ULONG_PTR)Search_Pattern(R_SETUPFRAME_CALL_SIG);
		if (addr)
		{
			gPrivateFuncs.R_ForceCVars = (decltype(gPrivateFuncs.R_ForceCVars))GetCallAddress(addr + 3 + 1);
			gPrivateFuncs.R_CheckVariables = (decltype(gPrivateFuncs.R_CheckVariables))GetCallAddress(addr + 3 + 1 + 5);
			gPrivateFuncs.R_AnimateLight = (decltype(gPrivateFuncs.R_AnimateLight))GetCallAddress(addr + 3 + 1 + 5 + 5);
		}
	}
	Sig_FuncNotFound(R_CheckVariables);
	Sig_FuncNotFound(R_AnimateLight);

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_MarkLeaves, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn *)inst;

			if (!r_viewleaf &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_ECX &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize )
			{//01D57970 83 3D 80 66 00 08 00                                cmp     gl_mtexable, 0
				r_viewleaf = (decltype(r_viewleaf))pinst->detail->x86.operands[1].mem.disp;
			}
			
			if (!r_oldviewleaf &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == X86_REG_ECX &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{//01D57970 83 3D 80 66 00 08 00                                cmp     gl_mtexable, 0
				r_oldviewleaf = (decltype(r_viewleaf))pinst->detail->x86.operands[0].mem.disp;
			}

			if (r_viewleaf && r_oldviewleaf)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(r_viewleaf);
		Sig_VarNotFound(r_oldviewleaf);
	}

#if 0
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "\x68\x2A\x2A\x2A\x2A\xD9\x1D\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68";
		addr = (DWORD)Search_Pattern(sigs);
		Sig_AddrNotFound(vright);
		vup = (vec_t *)(*(DWORD *)(addr + 1));
		vright = (vec_t *)(*(DWORD *)(addr + 18));
		vpn = (vec_t *)(*(DWORD *)(addr + 23));
	}
	else
	{
		const char sigs[] = "\x40\x68\x2A\x2A\x2A\x2A\xA3\x2A\x2A\x2A\x2A\xA1";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void *)gPrivateFuncs.R_SetupFrame, 0x300, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(vright);
		addr += 2;
		vup = (vec_t *)(*(DWORD *)addr);
		addr += 10;
		addr += 5;
		vright = (vec_t *)(*(DWORD *)addr);
		addr += 5;
		vpn = (vec_t *)(*(DWORD *)addr);
		addr += 5;
	}
#endif

#if 0
	if (1)
	{
#define URL_INFO_STRING "url_info"
		auto UrlInfo_String = Search_Pattern_Data(URL_INFO_STRING);
		if (!UrlInfo_String)
			UrlInfo_String = Search_Pattern_Rdata(URL_INFO_STRING);
		Sig_VarNotFound(UrlInfo_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x56\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
		*(DWORD *)(pattern + 1) = (DWORD)UrlInfo_String;
		auto UrlInfo_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(UrlInfo_PushString);
		gPrivateFuncs.DLL_SetModKey = (decltype(gPrivateFuncs.DLL_SetModKey))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(UrlInfo_PushString, 0x300, [](PUCHAR Candidate) {

			//.text : 01D0A6D0 56                                                  push    esi
			//.text : 01D0A6D1 8B 74 24 0C                                         mov     esi, [esp + 4 + arg_4]
			//.text : 01D0A6D5 68 F0 B5 E5 01                                      push    offset aGamedll; "gamedll"
			if (Candidate[1] == 0x8B &&
				Candidate[3] == 0x24 &&
				Candidate[4] == 0x0C &&
				Candidate[5] == 0x68)
			{
				return TRUE;
			}

			//.text : 01DAB080 55                                                  push    ebp
			//.text : 01DAB081 8B EC                                               mov     ebp, esp
			if (Candidate[0] == 0x55 &&
				Candidate[1] == 0x8B &&
				Candidate[2] == 0xEC)
			{
				return TRUE;
			}

			return FALSE;
		});
		Sig_FuncNotFound(DLL_SetModKey);

	}
#endif

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.V_RenderView, 0x150, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
				auto pinst = (cs_insn *)inst;

				if (!cls_state &&
					pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 5)
				{
					//83 3D 30 9A 09 02 05                                cmp     cls_state, 5
					cls_state = (decltype(cls_state))pinst->detail->x86.operands[0].mem.disp;
				}
				
				if (!cls_signon &&
					pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 2)
				{
					//83 3D D4 9F 0C 02 02                                cmp     cls_signon, 2
					cls_signon = (decltype(cls_signon))pinst->detail->x86.operands[0].mem.disp;
				}

				if (cls_state && cls_signon)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, NULL);

		Sig_VarNotFound(cls_state);
		Sig_VarNotFound(cls_signon);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.SCR_BeginLoadingPlaque, 0x100, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn *)inst;

			if (!scr_drawloading &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 1)
			{
				//C7 05 60 66 00 08 01 00 00 00                       mov     scr_drawloading, 1
				scr_drawloading = (decltype(scr_drawloading))pinst->detail->x86.operands[0].mem.disp;
			}

			if (scr_drawloading)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, NULL);

		Sig_VarNotFound(scr_drawloading);
	}

	if (gPrivateFuncs.R_LightStrength)
	{
		typedef struct
		{
			int MovZeroEbp_InstCount;
			int MovZeroEbp_Disp;
			int MovZeroEbp_Reg;
			int ZerodReg;
			int ZerodReg_InstCount;
			int Candidate_locallight_InstCount;
			PVOID Candidate_locallight;
			int Candidate_numlight_instCount;
			int Candidate_numlight_reg;
			PVOID Candidate_numlight;

			int Candidate_locallight2_InstCount;
			int Candidate_locallight2_reg;
			PVOID Candidate_locallight2;
		}R_LightStrength_Context;

		R_LightStrength_Context ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_LightStrength, 0x500, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto ctx = (R_LightStrength_Context *)context;
			auto pinst = (cs_insn *)inst;

			if (
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.disp != 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0 )
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
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
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
				locallight = (decltype(locallight))ctx->Candidate_locallight;
			}

			if (
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
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
				locallight = (decltype(locallight))ctx->Candidate_locallight2;
			}

			if (!numlight &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[0].type == X86_OP_REG)
			{
				ctx->Candidate_numlight = (decltype(ctx->Candidate_numlight))pinst->detail->x86.operands[1].mem.disp;
				ctx->Candidate_numlight_instCount = instCount;
				ctx->Candidate_numlight_reg = pinst->detail->x86.operands[0].reg;
			}

			if (!numlight &&
				ctx->Candidate_numlight &&
				ctx->Candidate_numlight_instCount &&
				instCount > ctx->Candidate_numlight_instCount &&
				instCount < ctx->Candidate_numlight_instCount + 30 &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				((pinst->detail->x86.operands[0].reg == ctx->Candidate_numlight_reg && 
				pinst->detail->x86.operands[1].reg == ctx->ZerodReg) ||
				(pinst->detail->x86.operands[1].reg == ctx->Candidate_numlight_reg &&
					pinst->detail->x86.operands[0].reg == ctx->ZerodReg)) )
			{
				numlight = (decltype(numlight))ctx->Candidate_numlight;
			}

			if (!numlight &&
				ctx->Candidate_numlight &&
				ctx->Candidate_numlight_instCount &&
				instCount > ctx->Candidate_numlight_instCount &&
				instCount < ctx->Candidate_numlight_instCount + 30 &&
				pinst->id == X86_INS_TEST &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == ctx->Candidate_numlight_reg)
			{
				numlight = (decltype(numlight))ctx->Candidate_numlight;
			}

			if (locallight && numlight)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(locallight);
		Sig_VarNotFound(numlight);
	}
	else
	{
		typedef struct
		{
			PVOID base;
			size_t max_insts;
			int max_depth;
			std::set<PVOID> code;
			std::set<PVOID> branches;
			std::vector<walk_context_t> walks;

			int MovZeroEbp_InstCount;
			int MovZeroEbp_Disp;
			int MovZeroEbp_Reg;
			int ZerodReg;
			int ZerodReg_InstCount;
			int Candidate_InstCount;
			PVOID Candidate_locallight;
			int Candidate_numlight_instCount;
			int Candidate_numlight_reg;
			PVOID Candidate_numlight;
		}R_LightStrength_Context;

		R_LightStrength_Context ctx = { 0 };

		ctx.base = gPrivateFuncs.R_GLStudioDrawPoints;
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

				if (locallight && numlight)
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
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
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
					locallight = (decltype(locallight))ctx->Candidate_locallight;
				}

				if (!numlight &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					pinst->detail->x86.operands[1].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
					pinst->detail->x86.operands[0].type == X86_OP_REG)
				{
					ctx->Candidate_numlight = (decltype(ctx->Candidate_numlight))pinst->detail->x86.operands[1].mem.disp;
					ctx->Candidate_numlight_instCount = instCount;
					ctx->Candidate_numlight_reg = pinst->detail->x86.operands[0].reg;
				}

				if (!numlight &&
					ctx->Candidate_numlight &&
					ctx->Candidate_numlight_instCount &&
					instCount > ctx->Candidate_numlight_instCount &&
					instCount < ctx->Candidate_numlight_instCount + 30 &&
					pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					((pinst->detail->x86.operands[0].reg == ctx->Candidate_numlight_reg &&
						pinst->detail->x86.operands[1].reg == ctx->ZerodReg) ||
						(pinst->detail->x86.operands[1].reg == ctx->Candidate_numlight_reg &&
							pinst->detail->x86.operands[0].reg == ctx->ZerodReg)))
				{
					numlight = (decltype(numlight))ctx->Candidate_numlight;
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

		Sig_VarNotFound(locallight);
		Sig_VarNotFound(numlight);
	}

	g_pMetaHookAPI->DisasmRanges(gEngfuncs.pfnSetFilterMode, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn *)inst;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
		{
			//.text:01D1B114 A3 AC 44 F5 03                                      mov     filterMode, eax
			filterMode = (decltype(filterMode))pinst->detail->x86.operands[0].mem.disp;
		}

		if (filterMode)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, NULL);

	Sig_VarNotFound(filterMode);

	typedef struct
	{
		PVOID Candidates[3];
		int CandidateCount;
	}FilterColorCotext_t;

	FilterColorCotext_t ctx = { 0 };

	g_pMetaHookAPI->DisasmRanges(gEngfuncs.pfnSetFilterColor, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto ctx = (FilterColorCotext_t *)context;
		auto pinst = (cs_insn *)inst;

		if (ctx->CandidateCount < 3)
		{
			if (g_iEngineType == ENGINE_SVENGINE)
			{
				if (pinst->id == X86_INS_FSTP &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					//  .text : 01D1B120 D9 44 24 04                                         fld[esp + arg_0]
					//	.text : 01D1B124 D9 1D 90 83 ED 01                                   fstp    r_filter_color
					//	.text : 01D1B12A D9 44 24 08                                         fld[esp + arg_4]
					//	.text : 01D1B12E D9 1D 94 83 ED 01                                   fstp    r_filter_color + 4
					//	.text : 01D1B134 D9 44 24 0C                                         fld[esp + arg_8]
					//	.text : 01D1B138 D9 1D 98 83 ED 01                                   fstp    r_filter_color + 8
					//	.text : 01D1B13E C3                                                  retn
					ctx->Candidates[ctx->CandidateCount] = (PVOID)pinst->detail->x86.operands[0].mem.disp;
					ctx->CandidateCount++;
				}
			}
			else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
			{
				if (pinst->id == X86_INS_MOVSS &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					//  .text : 01D1B120 D9 44 24 04                                         fld[esp + arg_0]
					//	.text : 01D1B124 D9 1D 90 83 ED 01                                   fstp    r_filter_color
					//	.text : 01D1B12A D9 44 24 08                                         fld[esp + arg_4]
					//	.text : 01D1B12E D9 1D 94 83 ED 01                                   fstp    r_filter_color + 4
					//	.text : 01D1B134 D9 44 24 0C                                         fld[esp + arg_8]
					//	.text : 01D1B138 D9 1D 98 83 ED 01                                   fstp    r_filter_color + 8
					//	.text : 01D1B13E C3                                                  retn
					ctx->Candidates[ctx->CandidateCount] = (PVOID)pinst->detail->x86.operands[0].mem.disp;
					ctx->CandidateCount++;
				}
			}
			else
			{
				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					//.text:01D115C1 A3 C4 71 E4 01                                      mov     filterColorRed, eax
					ctx->Candidates[ctx->CandidateCount] = (PVOID)pinst->detail->x86.operands[0].mem.disp;
					ctx->CandidateCount++;
				}
			}
		}

		if (ctx->CandidateCount >= 3)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, &ctx);

	std::qsort(ctx.Candidates, ctx.CandidateCount, sizeof(ctx.Candidates[0]), [](const void*a, const void*b) {
		return (int)(*(LONG_PTR *)a - *(LONG_PTR *)b);
	});

	filterColorRed = (decltype(filterColorRed))ctx.Candidates[0];
	filterColorGreen = (decltype(filterColorGreen))ctx.Candidates[1];
	filterColorBlue = (decltype(filterColorBlue))ctx.Candidates[2];

	Sig_VarNotFound(filterColorRed);
	Sig_VarNotFound(filterColorGreen);
	Sig_VarNotFound(filterColorBlue);

	g_pMetaHookAPI->DisasmRanges(gEngfuncs.pfnSetFilterBrightness, 0x50, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn *)inst;

		if (g_iEngineType == ENGINE_SVENGINE)
		{
			if (pinst->id == X86_INS_FSTP &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				//.text:01D1B114 A3 AC 44 F5 03                                      mov     filterMode, eax
				filterBrightness = (decltype(filterBrightness))pinst->detail->x86.operands[0].mem.disp;
			}
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			if (pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				filterBrightness = (decltype(filterBrightness))pinst->detail->x86.operands[0].mem.disp;
			}
		}
		else
		{
			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				filterBrightness = (decltype(filterBrightness))pinst->detail->x86.operands[0].mem.disp;
			}
		}

		if (filterBrightness)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, NULL);

	Sig_VarNotFound(filterBrightness);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define pmovevars_Signature_SvEngine "\x56\x8B\x74\x24\x08\x6A\x2C\x56\xE8\x2A\x2A\x2A\x2A\xD9\x05"

		addr = (DWORD)Search_Pattern(pmovevars_Signature_SvEngine);
		Sig_AddrNotFound(pmovevars);

		pmovevars = *(decltype(pmovevars)*)(addr + sizeof(pmovevars_Signature_SvEngine) - 1);
	}
	else
	{
#define pmovevars_Signature_GoldSrc "\xE8\x2A\x2A\x2A\x2A\xD9\x1D\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xD9\x1D\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xD9\x1D\x2A\x2A\x2A\x2A"

		addr = (DWORD)Search_Pattern(pmovevars_Signature_GoldSrc);
		Sig_AddrNotFound(pmovevars);

		pmovevars = *(decltype(pmovevars)*)(addr + 7);
	}

	if (1)
	{
#define detTexSupported_Signature "\x68\x73\x85\x00\x00\x68\x00\x23\x00\x00\xFF"

		auto detTexSupportedPattern = (PUCHAR)Search_Pattern(detTexSupported_Signature);
		Sig_AddrNotFound(detTexSupported);

		gPrivateFuncs.DT_Initialize = (decltype(gPrivateFuncs.DT_Initialize))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(detTexSupportedPattern, 0x100, [](PUCHAR Candidate) {

			if (Candidate[-1] == 0xC3 && 
				Candidate[0] == 0x56 &&
				Candidate[1] == 0x68)
				return TRUE;

			if (Candidate[0] == 0x68 &&
				Candidate[5] == 0xE8)
				return TRUE;

			return FALSE;
		});

		Sig_FuncNotFound(DT_Initialize);

		g_pMetaHookAPI->DisasmRanges(detTexSupportedPattern, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].size == 1 &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				detTexSupported = (decltype(detTexSupported))pinst->detail->x86.operands[0].mem.disp;
			}

			if (detTexSupported)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(detTexSupported);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.Cache_Alloc, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;

			if (!cache_head &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm >(PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				cache_head = (decltype(cache_head))pinst->detail->x86.operands[1].imm;
			}
			else if (!cache_head &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				cache_head = (decltype(cache_head))pinst->detail->x86.operands[1].imm;
			}
			if (cache_head)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, NULL);

		Sig_VarNotFound(cache_head);
	}

	if (1)
	{
		typedef struct
		{
			int gfCustomBuild_instCount;
			int push_0F_instCount;

			PVOID gfCustomBuild_candidate;
			int gfCustomBuild_candidate_reg;
			int gfCustomBuild_candidate_instCount;
		}Draw_MiptexTexture_ctx;

		Draw_MiptexTexture_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.Draw_MiptexTexture, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (Draw_MiptexTexture_ctx*)context;

			if (!gfCustomBuild &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0 )
			{
				gfCustomBuild = (decltype(gfCustomBuild))pinst->detail->x86.operands[0].mem.disp;

				ctx->gfCustomBuild_instCount = instCount;
			}

			if (!gfCustomBuild &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
			{
				ctx->gfCustomBuild_candidate = (decltype(ctx->gfCustomBuild_candidate))pinst->detail->x86.operands[1].mem.disp;
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
				gfCustomBuild = (decltype(gfCustomBuild))ctx->gfCustomBuild_candidate;
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
					(PUCHAR)pinst->detail->x86.operands[0].imm >(PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					szCustName = (decltype(szCustName))pinst->detail->x86.operands[0].imm;
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
}

hook_t* g_phook_GL_Init = NULL;
hook_t *g_phook_GL_BeginRendering = NULL;
hook_t *g_phook_GL_EndRendering = NULL;
hook_t *g_phook_R_RenderView_SvEngine = NULL;
hook_t *g_phook_R_RenderView = NULL;
hook_t *g_phook_R_LoadSkyBox_SvEngine = NULL;
hook_t *g_phook_R_LoadSkys = NULL;
hook_t *g_phook_R_NewMap = NULL;
hook_t *g_phook_R_CullBox = NULL;
hook_t *g_phook_Mod_PointInLeaf = NULL;
hook_t *g_phook_R_BuildLightMap = NULL;
hook_t *g_phook_R_AddDynamicLights = NULL;
hook_t *g_phook_R_GLStudioDrawPoints = NULL;
hook_t *g_phook_GL_UnloadTextures = NULL;
hook_t *g_phook_GL_UnloadTexture = NULL;
hook_t *g_phook_GL_LoadTexture2 = NULL;
hook_t *g_phook_GL_BuildLightmaps = NULL;
hook_t *g_phook_enginesurface_createNewTextureID = NULL;
hook_t *g_phook_enginesurface_drawSetTextureFile = NULL;
hook_t *g_phook_enginesurface_drawFlushText = NULL;
hook_t *g_phook_Mod_LoadStudioModel = NULL;
hook_t *g_phook_Mod_LoadBrushModel = NULL;
hook_t *g_phook_Mod_UnloadSpriteTextures = NULL;
hook_t *g_phook_triapi_RenderMode = NULL;
//hook_t *g_phook_triapi_Color4f = NULL;
hook_t *g_phook_Draw_MiptexTexture = NULL;
hook_t *g_phook_BuildGammaTable = NULL;
hook_t *g_phook_DLL_SetModKey = NULL;
hook_t *g_phook_SDL_GL_SetAttribute = NULL;

void R_UninstallHooksForEngineDLL(void)
{
	//Engine
	Uninstall_Hook(GL_Init);
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

	Uninstall_Hook(R_NewMap);
	Uninstall_Hook(Mod_PointInLeaf);
	Uninstall_Hook(R_GLStudioDrawPoints);
	Uninstall_Hook(GL_UnloadTextures);
	Uninstall_Hook(GL_LoadTexture2);
	Uninstall_Hook(GL_BuildLightmaps);

	if (!bHasOfficialGLTexAllocSupport)
	{
		Uninstall_Hook(enginesurface_createNewTextureID);
	}

	Uninstall_Hook(enginesurface_drawSetTextureFile);
	Uninstall_Hook(enginesurface_drawFlushText);
	Uninstall_Hook(Mod_LoadStudioModel);
	Uninstall_Hook(Mod_UnloadSpriteTextures);
	Uninstall_Hook(triapi_RenderMode);
	Uninstall_Hook(Draw_MiptexTexture);
	Uninstall_Hook(BuildGammaTable);
	Uninstall_Hook(R_CullBox);

	if (gPrivateFuncs.SDL_GL_SetAttribute)
	{
		Uninstall_Hook(SDL_GL_SetAttribute);
	}

	Uninstall_Hook(studioapi_StudioDynamicLight);
	Uninstall_Hook(studioapi_StudioCheckBBox);
	Uninstall_Hook(CL_FxBlend);
}

void R_InstallHooks(void)
{
	Install_InlineHook(GL_Init);
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

	Install_InlineHook(R_NewMap);
	Install_InlineHook(Mod_PointInLeaf);
	Install_InlineHook(R_GLStudioDrawPoints);
	Install_InlineHook(GL_UnloadTextures);
	Install_InlineHook(GL_LoadTexture2);
	Install_InlineHook(GL_BuildLightmaps);

	if (!bHasOfficialGLTexAllocSupport)
	{
		Install_InlineHook(enginesurface_createNewTextureID);
	}

	Install_InlineHook(enginesurface_drawSetTextureFile);
	Install_InlineHook(enginesurface_drawFlushText);
	Install_InlineHook(Mod_LoadStudioModel);
	Install_InlineHook(Mod_UnloadSpriteTextures);
	Install_InlineHook(triapi_RenderMode);
	Install_InlineHook(Draw_MiptexTexture);
	Install_InlineHook(BuildGammaTable);
	Install_InlineHook(R_CullBox);

	//OpenGL4.2 was forced by HL25 engine which might ruin the renderer features.
	/*
	      if ( a1 )
		  {
			SDL_GL_SetAttribute(17, 4);
			SDL_GL_SetAttribute(18, 2);
			result = SDL_GL_SetAttribute(21, 2);
		  }
	*/
	if (gPrivateFuncs.SDL_GL_SetAttribute)
	{
		Install_InlineHook(SDL_GL_SetAttribute);
	}
}

int WINAPI GL_RedirectedGenTexture(void)
{
	return GL_GenTexture();
}

/*
Purpose: Redirect all "mov eax, allocated_textures" to "call GL_RedirectedGenTexture" for legacy engine
*/

void R_RedirectLegacyOpenGLTextureAllocation(void)
{
	if (bHasOfficialGLTexAllocSupport)
		return;
	
	const char pattern[] = "\xA1\x2A\x2A\x2A\x2A";
	*(ULONG_PTR*)(pattern + 1) = (ULONG_PTR)allocated_textures;

	PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
	PUCHAR SearchLimit = (PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			typedef struct
			{
				bool bFoundWriteBack;
				bool bFoundGL_Bind;
			}RedirectBlobEngineOpenGLTextureContext;

			RedirectBlobEngineOpenGLTextureContext ctx = { 0 };

			g_pMetaHookAPI->DisasmRanges(pFound, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (RedirectBlobEngineOpenGLTextureContext*)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp == (PUCHAR)allocated_textures &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].reg == X86_REG_EAX)
				{
					ctx->bFoundWriteBack = true;
				}

				if (address[0] == 0xE8 && 
					(PUCHAR)pinst->detail->x86.operands[0].imm == (PUCHAR)gPrivateFuncs.GL_Bind)
				{
					ctx->bFoundGL_Bind = true;
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
				char redirect[] = "\xE8\x2A\x2A\x2A\x2A";
				*(int*)(redirect + 1) = (PUCHAR)GL_RedirectedGenTexture - (pFound + 5);
				g_pMetaHookAPI->WriteMemory(pFound, redirect, sizeof(redirect) - 1);
			}

			SearchBegin = pFound + Sig_Length(pattern);
		}
		else
		{
			break;
		}
	}
}