
#include <metahook.h>
#include <capstone.h>
#include <set>
#include <map>
#include "gl_local.h"

//#define R_LOADSKYNAME_SIG_SVENGINE "\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x84\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x3D"

#define MOD_POINTINLEAF_SIG_SVENGINE "\x2A\x8B\x2A\x24\x2A\x85\x2A\x2A\x2A\x8B\x2A\xA4\x00\x00\x00"
#define MOD_POINTINLEAF_SIG_SVENGINE_10152 "\x8B\x54\x24\x08\x2A\x8D\x2A\xA4\x00\x00\x00\x85\xD2\x74\x2A\x83\x2A\x00"
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
#define R_SETUPGL_SIG_SVENGINE_10152 "\x51\x53\x56\x57\x68\x01\x17\x00\x00\xFF\x15"

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
#define R_RENDERDYNAMICLIGHTMAPS_SIG_SVENGINE_10152 "\x8B\x54\x24\x04\x83\xEC\x0C\xFF\x05\x2A\x2A\x2A\x2A\x57\x33\xFF"

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

#define R_LIGHTSTRENGTH_SIG_SVENGINE "\x8B\x15\x2A\x2A\x2A\x2A\x2A\x8B\x35\x2A\x2A\x2A\x2A\x2A\x8B\x7C\x24\x0C" //inlined
#define R_LIGHTSTRENGTH_SIG_SVENGINE_10152 "\x8B\x54\x24\x04\x2A\x8B\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x2A\x2A\x2A\x2A\x2A\x2A\x8D\x2A\x2A\x39\x2A\x2A\x2A\x2A\x2A\x2A\x0F" //inlined
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

#define MOD_LOADSPRITEMODEL_BLOB		"\x53\x55\x56\x57\x8B\x7C\x24\x18\x8B\x47\x04\x50\xFF\x15"
#define MOD_LOADSPRITEMODEL_NEW2		MOD_LOADSPRITEMODEL_BLOB
#define MOD_LOADSPRITEMODEL_NEW			"\x55\x8B\xEC\x51\x53\x56\x57\x8B\x7D\x0C\x8B\x47\x04\x50\xFF\x15"
#define MOD_LOADSPRITEMODEL_HL25		"\x55\x8B\xEC\x83\xEC\x08\x2A\x2A\x8B\x75\x0C\x2A\xFF\x2A\x04\xFF\x15"
#define MOD_LOADSPRITEMODEL_SVENGINE	"\x83\xEC\x08\x2A\x2A\x2A\x8B\x2A\x24\x1C\x2A\xFF\x2A\x04\xFF\x15"

#define R_INITPARTICLETEXTURE_BLOB "\xA1\x2A\x2A\x2A\x2A\x81\xEC\x2A\x2A\x00\x00\x8B\xC8\x40"
#define R_INITPARTICLETEXTURE_COMMON "\x68\x01\x14\x00\x00\x68\x08\x19\x00\x00\x6A\x00\x6A\x08\x6A\x08"

void R_FillAddress_EngineSurface(const mh_dll_info_t& DllInfo)
{
	auto engineFactory = g_pMetaHookAPI->GetEngineFactory();

	if (engineFactory)
	{
#define ENGINE_SURFACE_VERSION "EngineSurface007"
		void* engineSurface = (void*)engineFactory(ENGINE_SURFACE_VERSION, NULL);

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
.rdata:102C94B4 ??_7EngineSurface@@6B@ dd offset dtor_0 ; DATA XREF: dtor_0+2C��o
.rdata:102C94B4                                         ; sub_102285C0+30��o ...
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
			index_drawSetTextureFile++;
			index_isTextureIDValid++;
			index_drawFlushText++;
		}

		gPrivateFuncs.enginesurface_drawSetTextureRGBA = (decltype(gPrivateFuncs.enginesurface_drawSetTextureRGBA))engineSurface_vftable[index_drawSetTextureRGBA];
		gPrivateFuncs.enginesurface_drawSetTexture = (decltype(gPrivateFuncs.enginesurface_drawSetTexture))engineSurface_vftable[index_drawSetTexture];
		gPrivateFuncs.enginesurface_createNewTextureID = (decltype(gPrivateFuncs.enginesurface_createNewTextureID))engineSurface_vftable[index_createNewTextureID];
		gPrivateFuncs.enginesurface_drawSetTextureFile = (decltype(gPrivateFuncs.enginesurface_drawSetTextureFile))engineSurface_vftable[index_drawSetTextureFile];
		gPrivateFuncs.enginesurface_isTextureIDValid = (decltype(gPrivateFuncs.enginesurface_isTextureIDValid))engineSurface_vftable[index_isTextureIDValid];
		gPrivateFuncs.enginesurface_drawFlushText = (decltype(gPrivateFuncs.enginesurface_drawFlushText))engineSurface_vftable[index_drawFlushText];

		typedef struct
		{
			void* candicate{};
			int instCount{};
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
}

void R_FillAddress_HasOfficialFBOSupport(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

void R_FillAddress_HasOfficialGLTexAllocSupport(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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
			}LoadSkysSearchContext;

			LoadSkysSearchContext ctx = { 0 };

			g_pMetaHookAPI->DisasmRanges(pFound + 4, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (LoadSkysSearchContext*)context;

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

void R_FillAddress_GL_Init(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_Init)
		return;

	ULONG_PTR GL_Init_VA = 0;
	ULONG GL_Init_RVA = 0;

	{
		const char pattern[] = "\x68\x00\x1F\x00\x00\xFF";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				auto pCandidateFunction = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x80, [](PUCHAR Candidate) {

					//.text:01D5E540 83 EC 14                                            sub     esp, 14h
					//	.text : 01D5E543 56                                                  push    esi
					if ((Candidate[-1] == 0xCC || Candidate[-1] == 0x90 || Candidate[-1] == 0xC3) &&
						Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC &&
						Candidate[2] == 0x14 &&
						Candidate[3] >= 0x50 &&
						Candidate[3] <= 0x57)
						return TRUE;

					//.text : 01D82A50 55                                                  push    ebp
					//.text : 01D82A51 8B EC                                               mov     ebp, esp
					//.text:01D4BE83 83 EC 14                                            sub     esp, 14h
					if ((Candidate[-1] == 0xCC || Candidate[-1] == 0x90 || Candidate[-1] == 0xC3) &&
						Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC &&
						Candidate[3] == 0x83 &&
						Candidate[4] == 0xEC)
						return TRUE;

					if (Candidate[-1] == 0x90 &&
						Candidate[0] == 0x68 &&
						Candidate[1] == 0x00 &&
						Candidate[2] == 0x1F &&
						Candidate[3] == 0x00 &&
						Candidate[4] == 0x00)
						return TRUE;

					return FALSE;
					});

				if (pCandidateFunction)
				{
					typedef struct
					{
						const mh_dll_info_t& DllInfo;
						bool bFoundPushString{};
					}GL_InitSearchContext;

					GL_InitSearchContext ctx = { DllInfo };

					g_pMetaHookAPI->DisasmRanges(pCandidateFunction, 0x120, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

						auto pinst = (cs_insn*)inst;
						auto ctx = (GL_InitSearchContext*)context;

						if (pinst->id == X86_INS_PUSH &&
							pinst->detail->x86.op_count == 1 &&
							pinst->detail->x86.operands[0].type == X86_OP_IMM &&
							(//Check if imm is inside .data or .rdata
								((PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx->DllInfo.DataBase &&
									(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize) ||
								((PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx->DllInfo.RdataBase &&
									(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.RdataBase + ctx->DllInfo.RdataSize)
								))
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
						GL_Init_VA = (ULONG_PTR)pCandidateFunction;
						Convert_VA_to_RVA(GL_Init, DllInfo);
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

	if (GL_Init_RVA)
	{
		gPrivateFuncs.GL_Init = (decltype(gPrivateFuncs.GL_Init))VA_from_RVA(GL_Init, RealDllInfo);
	}

	Sig_FuncNotFound(GL_Init);
}

void R_FillAddress_R_PolyBlend(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_PolyBlend)
		return;

	ULONG_PTR R_PolyBlend_VA = 0;
	ULONG R_PolyBlend_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_PolyBlend_VA = (ULONG_PTR)Search_Pattern(R_POLYBLEND_SVENGINE, DllInfo);
		Convert_VA_to_RVA(R_PolyBlend, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_PolyBlend_VA = (ULONG_PTR)Search_Pattern(R_POLYBLEND_HL25, DllInfo);
		Convert_VA_to_RVA(R_PolyBlend, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_PolyBlend_VA = (ULONG_PTR)Search_Pattern(R_POLYBLEND_NEW, DllInfo);

		if (!R_PolyBlend_VA)
			R_PolyBlend_VA = (ULONG_PTR)Search_Pattern(R_POLYBLEND_NEW2, DllInfo);

		Convert_VA_to_RVA(R_PolyBlend, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_PolyBlend_VA = (ULONG_PTR)Search_Pattern(R_POLYBLEND_BLOB, DllInfo);
		Convert_VA_to_RVA(R_PolyBlend, DllInfo);
	}

	if (R_PolyBlend_RVA)
	{
		gPrivateFuncs.R_PolyBlend = (decltype(gPrivateFuncs.R_PolyBlend))VA_from_RVA(R_PolyBlend, RealDllInfo);
	}

	Sig_FuncNotFound(R_PolyBlend);
}

void R_FillAddress_S_ExtraUpdate(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.S_ExtraUpdate)
		return;

	ULONG_PTR S_ExtraUpdate_VA = 0;
	ULONG S_ExtraUpdate_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		S_ExtraUpdate_VA = (ULONG_PTR)Search_Pattern(S_EXTRAUPDATE_SVENGINE, DllInfo);
		Convert_VA_to_RVA(S_ExtraUpdate, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		S_ExtraUpdate_VA = (ULONG_PTR)Search_Pattern(S_EXTRAUPDATE_HL25, DllInfo);
		Convert_VA_to_RVA(S_ExtraUpdate, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		S_ExtraUpdate_VA = (ULONG_PTR)Search_Pattern(S_EXTRAUPDATE_NEW, DllInfo);

		if (!S_ExtraUpdate_VA)
			S_ExtraUpdate_VA = (ULONG_PTR)Search_Pattern(S_EXTRAUPDATE_NEW2, DllInfo);

		Convert_VA_to_RVA(S_ExtraUpdate, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		S_ExtraUpdate_VA = (ULONG_PTR)Search_Pattern(S_EXTRAUPDATE_BLOB, DllInfo);
		Convert_VA_to_RVA(S_ExtraUpdate, DllInfo);
	}

	if (S_ExtraUpdate_RVA)
	{
		gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))VA_from_RVA(S_ExtraUpdate, RealDllInfo);
	}

	Sig_FuncNotFound(S_ExtraUpdate);
}

void R_FillAddress_GL_Bind(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_Bind)
		return;

	ULONG_PTR GL_Bind_VA = 0;
	ULONG GL_Bind_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		GL_Bind_VA = (ULONG_PTR)Search_Pattern(GL_BIND_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(GL_Bind, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		GL_Bind_VA = (ULONG_PTR)Search_Pattern(GL_BIND_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(GL_Bind, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		GL_Bind_VA = (ULONG_PTR)Search_Pattern(GL_BIND_SIG_NEW, DllInfo);

		//try another signature
		if (!GL_Bind_VA)
			GL_Bind_VA = (ULONG_PTR)Search_Pattern(GL_BIND_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(GL_Bind, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		GL_Bind_VA = (ULONG_PTR)Search_Pattern(GL_BIND_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(GL_Bind, DllInfo);
	}

	if (GL_Bind_RVA)
	{
		gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))VA_from_RVA(GL_Bind, RealDllInfo);
	}

	Sig_FuncNotFound(GL_Bind);

	/*
//Global pointers that link into engine vars.
	int *currenttexture = NULL;
*/
	ULONG_PTR currenttexture_VA = 0;
	ULONG currenttexture_RVA = 0;
	{
		typedef struct
		{
			ULONG_PTR& currenttexture;
			const mh_dll_info_t& DllInfo;
		} GL_BindVars_SearchContext;

		GL_BindVars_SearchContext ctx = { currenttexture_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)GL_Bind_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (GL_BindVars_SearchContext*)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].reg == X86_REG_EAX)
				{//.text:01D3C0C2 A3 40 22 E4 01 mov     currenttexture, eax

					ctx->currenttexture = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}

				if (ctx->currenttexture)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(currenttexture, DllInfo);
	}

	if (currenttexture_RVA)
		currenttexture = (decltype(currenttexture))VA_from_RVA(currenttexture, RealDllInfo);

	Sig_VarNotFound(currenttexture);

}

void R_FillAddress_GL_SelectTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_SelectTexture)
		return;

	ULONG_PTR GL_SelectTexture_VA = 0;
	ULONG GL_SelectTexture_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		ULONG_PTR GL_Bind_VA = (ULONG_PTR)gPrivateFuncs.GL_Bind;
		ULONG GL_Bind_RVA = 0;

		Convert_VA_to_RVA(GL_Bind, RealDllInfo);
		Convert_RVA_to_VA(GL_Bind, DllInfo);

		GL_SelectTexture_VA = (ULONG_PTR)Search_Pattern_From(GL_Bind_VA, GL_SELECTTEXTURE_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(GL_SelectTexture, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		ULONG_PTR GL_Bind_VA = (ULONG_PTR)gPrivateFuncs.GL_Bind;
		ULONG GL_Bind_RVA = 0;

		Convert_VA_to_RVA(GL_Bind, RealDllInfo);
		Convert_RVA_to_VA(GL_Bind, DllInfo);

		GL_SelectTexture_VA = (ULONG_PTR)Search_Pattern_From(GL_Bind_VA, GL_SELECTTEXTURE_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(GL_SelectTexture, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		GL_SelectTexture_VA = (ULONG_PTR)Search_Pattern(GL_SELECTTEXTURE_SIG_NEW, DllInfo);
		Convert_VA_to_RVA(GL_SelectTexture, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		GL_SelectTexture_VA = (ULONG_PTR)Search_Pattern(GL_SELECTTEXTURE_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(GL_SelectTexture, DllInfo);
	}

	if (GL_SelectTexture_RVA)
	{
		gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))VA_from_RVA(GL_SelectTexture, RealDllInfo);
	}

	Sig_FuncNotFound(GL_SelectTexture);

	/*
  //Global pointers that link into engine vars.
	  int *oldtarget = NULL;
  */
	ULONG_PTR oldtarget_VA = 0;
	ULONG oldtarget_RVA = 0;

	{
		typedef struct
		{
			ULONG_PTR& oldtarget;
			const mh_dll_info_t& DllInfo;
		} GL_SelectTextureVars_SearchContext;

		GL_SelectTextureVars_SearchContext ctx = { oldtarget_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)GL_SelectTexture_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (GL_SelectTextureVars_SearchContext*)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].reg == X86_REG_ESI)
				{//.text:01D4FE37 89 35 08 82 ED 01 mov     oldtarget, esi

					ctx->oldtarget = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}

				if (ctx->oldtarget)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(oldtarget, DllInfo);
	}

	if (oldtarget_RVA)
		oldtarget = (decltype(oldtarget))VA_from_RVA(oldtarget, RealDllInfo);

	Sig_VarNotFound(oldtarget);
}

void R_FillAddress_GL_LoadTexture2(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_LoadTexture2)
		return;

	ULONG_PTR GL_LoadTexture2_RVA = 0;
	ULONG GL_LoadTexture2_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "NULL Texture\n";
		auto Error_String_VA = Search_Pattern_Data(sigs, DllInfo);
		if (!Error_String_VA)
			Error_String_VA = Search_Pattern_Rdata(sigs, DllInfo);
		if (Error_String_VA)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 1) = (DWORD)Error_String_VA;
			auto Error_Call_VA = Search_Pattern(pattern, DllInfo);
			if (Error_Call_VA)
			{
				auto GL_LoadTexture2_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Error_Call_VA, 0x500, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x51 &&
						Candidate[1] == 0x83)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					return FALSE;
					});

				if (GL_LoadTexture2_VA)
					GL_LoadTexture2_RVA = (ULONG_PTR)GL_LoadTexture2_VA - (ULONG_PTR)DllInfo.ImageBase;
			}
		}
	}
	else
	{
		const char sigs[] = "Texture Overflow: MAX_GLTEXTURES";
		auto Error_String_VA = Search_Pattern_Data(sigs, DllInfo);
		if (!Error_String_VA)
			Error_String_VA = Search_Pattern_Rdata(sigs, DllInfo);
		if (Error_String_VA)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
			*(DWORD*)(pattern + 1) = (DWORD)Error_String_VA;
			auto Error_Call_VA = Search_Pattern(pattern, DllInfo);
			if (Error_Call_VA)
			{
				auto GL_LoadTexture2_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Error_Call_VA, 0x500, [](PUCHAR Candidate) {

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

				Convert_VA_to_RVA(GL_LoadTexture2, DllInfo);
			}
		}
	}

	if (!GL_LoadTexture2_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			GL_LoadTexture2_VA = (ULONG_PTR)Search_Pattern(GL_LOADTEXTURE2_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(GL_LoadTexture2, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			GL_LoadTexture2_VA = (ULONG_PTR)Search_Pattern(GL_LOADTEXTURE2_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(GL_LoadTexture2, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			GL_LoadTexture2_VA = (ULONG_PTR)Search_Pattern(GL_LOADTEXTURE2_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(GL_LoadTexture2, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			GL_LoadTexture2_VA = (ULONG_PTR)Search_Pattern(GL_LOADTEXTURE2_SIG_BLOB, DllInfo);

			//try another signature
			if (!GL_LoadTexture2_VA)
				GL_LoadTexture2_VA = (ULONG_PTR)Search_Pattern(GL_LOADTEXTURE2_SIG_BLOB2, DllInfo);

			Convert_VA_to_RVA(GL_LoadTexture2, DllInfo);
		}
	}

	if (GL_LoadTexture2_RVA)
	{
		gPrivateFuncs.GL_LoadTexture2 = (decltype(gPrivateFuncs.GL_LoadTexture2))VA_from_RVA(GL_LoadTexture2, RealDllInfo);
	}

	Sig_FuncNotFound(GL_LoadTexture2);

	/*
		int *numgltextures = NULL;
		gltexture_t *gltextures = NULL;
		int *maxgltextures_SvEngine = NULL;
		gltexture_t **gltextures_SvEngine = NULL;
		int *peakgltextures_SvEngine = NULL;
		int *allocated_textures = NULL;
		int *gHostSpawnCount = NULL;
	*/

	ULONG_PTR numgltextures_VA = 0;
	ULONG numgltextures_RVA = 0;

	ULONG_PTR gltextures_VA = 0;
	ULONG gltextures_RVA = 0;

	ULONG_PTR maxgltextures_SvEngine_VA = 0;
	ULONG maxgltextures_SvEngine_RVA = 0;

	ULONG_PTR gltextures_SvEngine_VA = 0;
	ULONG gltextures_SvEngine_RVA = 0;

	ULONG_PTR peakgltextures_SvEngine_VA = 0;
	ULONG peakgltextures_SvEngine_RVA = 0;

	ULONG_PTR allocated_textures_VA = 0;
	ULONG allocated_textures_RVA = 0;

	ULONG_PTR gHostSpawnCount_VA = 0;
	ULONG gHostSpawnCount_RVA = 0;

	ULONG_PTR realloc_SvEngine_VA = 0;
	ULONG realloc_SvEngine_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char pattern[] = "\x8B\x15\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x2A\x8B\x1D";
		// GL_LoadTexture2
		//.text : 01D4EBF4 8B 15 F0 C5 0F 03                                   mov     edx, numgltextures
		//.text : 01D4EBFA 3B F2                                               cmp     esi, edx
		//.text : 01D4EBFC 7D 4D                                               jge     short loc_1D4EC4B
		//.text : 01D4EBFE 8B 1D E4 C5 0F 03                                   mov     ebx, gltextures

		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size((void*)GL_LoadTexture2_VA, 0x300, pattern, DllInfo);
		Sig_AddrNotFound(gltextures);
		numgltextures_VA = *(ULONG_PTR*)(addr + 2);
		gltextures_SvEngine_VA = *(ULONG_PTR*)(addr + 12);

		Convert_VA_to_RVA(numgltextures, DllInfo);
		Convert_VA_to_RVA(gltextures_SvEngine, DllInfo);

		const char pattern2[] = "\x6B\xC1\x54\x89\x0D";
		//  GL_LoadTexture2
		//.text:01D4ED66 6B C1 54                                            imul    eax, ecx, 54h; 'T'
		//.text:01D4ED69 89 0D F0 C6 0F 03                                   mov     maxgltextures, ecx

		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)GL_LoadTexture2_VA, 0x300, pattern2, DllInfo);
		Sig_AddrNotFound(maxgltextures);
		maxgltextures_SvEngine_VA = *(ULONG_PTR*)(addr + 5);

		Convert_VA_to_RVA(maxgltextures_SvEngine, DllInfo);

		const char pattern3[] = "\x51\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x50, pattern3, DllInfo);
		Sig_AddrNotFound(realloc);
		realloc_SvEngine_VA = (ULONG_PTR)GetCallAddress(addr + 1);

		Convert_VA_to_RVA(realloc_SvEngine, DllInfo);

		const char pattern4[] = "\x66\x8B\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x04";
		// GL_LoadTexture2
		//66 8B 0D E0 72 40 08                                mov     cx, word ptr gHostSpawnCount
		//66 89 4B 04                                         mov     [ebx+4], cx

		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)GL_LoadTexture2_VA, 0x200, pattern4, DllInfo);
		Sig_AddrNotFound(gHostSpawnCount);
		gHostSpawnCount_VA = *(ULONG_PTR*)(addr + 3);

		Convert_VA_to_RVA(gHostSpawnCount, DllInfo);

		const char pattern5[] = "\x03\x35\x2A\x2A\x2A\x2A\x3B\x15";
		// GL_LoadTexture2
		//.text:01D4EDE8 03 35 EC C6 0F 03                                   add     esi, gltextures
		//.text : 01D4EDEE 3B 15 00 C7 0F 03                                   cmp     edx, peakgltextures

		addr = (ULONG_PTR)g_pMetaHookAPI->SearchPattern((void*)GL_LoadTexture2_VA, 0x200, pattern5, Sig_Length(pattern5));
		Sig_AddrNotFound(peakgltextures);
		peakgltextures_SvEngine_VA = *(ULONG_PTR*)(addr + 8);

		Convert_VA_to_RVA(peakgltextures_SvEngine, DllInfo);
	}
	else
	{
		typedef struct
		{
			ULONG_PTR& gltextures;
			ULONG_PTR& numgltextures;
			ULONG_PTR& gHostSpawnCount;
			ULONG_PTR& allocated_textures;
			const mh_dll_info_t& DllInfo;

			int xor_exi_exi_instCount{};
			int xor_exi_exi_reg{};
			int inc_exx_instCount{};
			int inc_exx_reg{};
			int mov_mem_exx_instCount{};
			int mov_mem_exx_reg{};
		} GL_LoadTexture2_SearchContext;

		GL_LoadTexture2_SearchContext ctx = { gltextures_VA, numgltextures_VA, gHostSpawnCount_VA, allocated_textures_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)GL_LoadTexture2_VA, 0x200, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (GL_LoadTexture2_SearchContext*)context;

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
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
					)
				{//  mov     edi, offset gltextures
					ctx->gltextures = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
				}

				if (ctx->xor_exi_exi_instCount && pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					(pinst->detail->x86.operands[0].reg == X86_REG_ESI || pinst->detail->x86.operands[0].reg == X86_REG_EDI)
					&&
					pinst->detail->x86.operands[0].reg != ctx->xor_exi_exi_reg &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
					)
				{//  mov     edi, offset gltextures
					ctx->gltextures = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
				}

				if (pinst->id == X86_INS_INC &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG)
				{// inc     ecx

					ctx->inc_exx_instCount = instCount;
					ctx->inc_exx_reg = pinst->detail->x86.operands[0].reg;
				}

				if (ctx->xor_exi_exi_instCount && pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].reg == ctx->inc_exx_reg &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
					)
				{//  mov     numgltextures, ecx

					if (!g_bHasOfficialGLTexAllocSupport &&
						instCount > ctx->mov_mem_exx_instCount && instCount < ctx->mov_mem_exx_instCount + 5 &&
						pinst->detail->x86.operands[1].reg == ctx->mov_mem_exx_reg)
					{
						ctx->allocated_textures = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					}
					else
					{
						ctx->numgltextures = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					}
				}

				if (ctx->xor_exi_exi_instCount && (pinst->id == X86_INS_MOV || pinst->id == X86_INS_MOVZX) &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].size == 2 &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
					)
				{	//                        mov     ax, word ptr gHostSpawnCount
					//                        movzx   eax, word ptr gHostSpawnCount
					ctx->gHostSpawnCount = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
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
					if (ctx->gltextures && ctx->numgltextures && ctx->gHostSpawnCount && ctx->allocated_textures)
						return TRUE;
				}
				else
				{
					if (ctx->gltextures && ctx->numgltextures && ctx->gHostSpawnCount)
						return TRUE;
				}

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(gltextures, DllInfo);
		Convert_VA_to_RVA(numgltextures, DllInfo);
		Convert_VA_to_RVA(gHostSpawnCount, DllInfo);

		if (!g_bHasOfficialGLTexAllocSupport)
		{
			Convert_VA_to_RVA(allocated_textures, DllInfo);
		}
	}

	if (numgltextures_RVA)
		numgltextures = (decltype(numgltextures))VA_from_RVA(numgltextures, RealDllInfo);
	if (gltextures_RVA)
		gltextures = (decltype(gltextures))VA_from_RVA(gltextures, RealDllInfo);
	if (gltextures_SvEngine_RVA)
		gltextures_SvEngine = (decltype(gltextures_SvEngine))VA_from_RVA(gltextures_SvEngine, RealDllInfo);
	if (maxgltextures_SvEngine_RVA)
		maxgltextures_SvEngine = (decltype(maxgltextures_SvEngine))VA_from_RVA(maxgltextures_SvEngine, RealDllInfo);
	if (peakgltextures_SvEngine_RVA)
		peakgltextures_SvEngine = (decltype(peakgltextures_SvEngine))VA_from_RVA(peakgltextures_SvEngine, RealDllInfo);
	if (gHostSpawnCount_RVA)
		gHostSpawnCount = (decltype(gHostSpawnCount))VA_from_RVA(gHostSpawnCount, RealDllInfo);
	if (realloc_SvEngine_RVA)
		gPrivateFuncs.realloc_SvEngine = (decltype(gPrivateFuncs.realloc_SvEngine))VA_from_RVA(realloc_SvEngine, RealDllInfo);
	if (allocated_textures_RVA)
		allocated_textures = (decltype(allocated_textures))VA_from_RVA(allocated_textures, RealDllInfo);

	Sig_VarNotFound(gltextures);
	Sig_VarNotFound(numgltextures);
	Sig_VarNotFound(gHostSpawnCount);

	if (!g_bHasOfficialGLTexAllocSupport)
	{
		Sig_VarNotFound(allocated_textures);
	}
}

void R_FillAddress_R_CullBox(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_CullBox)
		return;

	ULONG_PTR R_CullBox_VA = 0;
	ULONG R_CullBox_RVA = 0;

	ULONG_PTR GL_LoadTexture2_VA = (ULONG_PTR)gPrivateFuncs.GL_LoadTexture2;
	ULONG GL_LoadTexture2_RVA = 0;

	Convert_VA_to_RVA(GL_LoadTexture2, RealDllInfo);
	Convert_RVA_to_VA(GL_LoadTexture2, DllInfo);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_CullBox_VA = (ULONG_PTR)Search_Pattern_From(GL_LoadTexture2_VA, R_CULLBOX_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(R_CullBox, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_CullBox_VA = (ULONG_PTR)Search_Pattern_From(GL_LoadTexture2_VA, R_CULLBOX_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(R_CullBox, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_CullBox_VA = (ULONG_PTR)Search_Pattern_From(GL_LoadTexture2_VA, R_CULLBOX_SIG_NEW, DllInfo);

		//try another signature
		if (!R_CullBox_VA)
			R_CullBox_VA = (ULONG_PTR)Search_Pattern_From(GL_LoadTexture2_VA, R_CULLBOX_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(R_CullBox, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_CullBox_VA = (ULONG_PTR)Search_Pattern_From(GL_LoadTexture2_VA, R_CULLBOX_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_CullBox, DllInfo);
	}

	if (R_CullBox_RVA)
	{
		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))VA_from_RVA(R_CullBox, RealDllInfo);
	}

	Sig_FuncNotFound(R_CullBox);

	/*
		mplane_t *frustum = NULL;
		vec_t *vup = NULL;
		vec_t *vpn = NULL;
		vec_t *vright = NULL;
	*/
	ULONG_PTR frustum_VA = 0;
	ULONG frustum_RVA = 0;

	ULONG_PTR vup_VA = 0;
	ULONG vup_RVA = 0;

	ULONG_PTR vpn_VA = 0;
	ULONG vpn_RVA = 0;

	ULONG_PTR vright_VA = 0;
	ULONG vright_RVA = 0;

	typedef struct
	{
		ULONG_PTR& frustum;
		const mh_dll_info_t& DllInfo;
	} R_CullBox_SearchContext;

	R_CullBox_SearchContext ctx = { frustum_VA, DllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)R_CullBox_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_CullBox_SearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_ESI &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
				)
			{//BE 80 98 BC 02                                      mov     esi, offset frustum

				ctx->frustum = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
			}

			if (ctx->frustum)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	Convert_VA_to_RVA(frustum, DllInfo);

	if (frustum_RVA)
	{
		char pattern_Frustum[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern_Frustum + 11) = (DWORD)frustum_VA;

		auto addr = (ULONG_PTR)Search_Pattern(pattern_Frustum, DllInfo);
		Sig_AddrNotFound(pattern_Frustum);

		vpn_VA = *(ULONG_PTR*)(addr + 1);
		vup_VA = *(ULONG_PTR*)(addr + 6);

		char pattern_Frustum2[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern_Frustum2 + 1) = (DWORD)vpn_VA;
		*(DWORD*)(pattern_Frustum2 + 11) = (DWORD)(frustum_VA + 0x28);

		addr = (ULONG_PTR)Search_Pattern(pattern_Frustum2, DllInfo);
		Sig_AddrNotFound(pattern_Frustum2);

		vright_VA = *(ULONG_PTR*)((PUCHAR)addr + 6);

		Convert_VA_to_RVA(vpn, DllInfo);
		Convert_VA_to_RVA(vup, DllInfo);
		Convert_VA_to_RVA(vright, DllInfo);
	}

	if (frustum_RVA)
		frustum = (decltype(frustum))VA_from_RVA(frustum, RealDllInfo);
	if (vpn_RVA)
		vpn = (decltype(vpn))VA_from_RVA(vpn, RealDllInfo);
	if (vup_RVA)
		vup = (decltype(vup))VA_from_RVA(vup, RealDllInfo);
	if (vright_RVA)
		vright = (decltype(vright))VA_from_RVA(vright, RealDllInfo);

	Sig_VarNotFound(frustum);
	Sig_VarNotFound(vpn);
	Sig_VarNotFound(vup);
	Sig_VarNotFound(vright);
}

void R_FillAddress_R_SetupFrame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_SetupFrame)
		return;

	ULONG_PTR R_SetupFrame_VA = 0;
	ULONG R_SetupFrame_RVA = 0;

	//The R_SetupFrame has been inlined into R_RenderScene in HL25 and SvEngine

	if (g_iEngineType == ENGINE_SVENGINE)
	{

	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{

	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		auto R_SetupFrame_VA = (ULONG_PTR)Search_Pattern(R_SETUPFRAME_SIG_NEW, DllInfo);
		Convert_VA_to_RVA(R_SetupFrame, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		auto R_SetupFrame_VA = (ULONG_PTR)Search_Pattern(R_SETUPFRAME_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_SetupFrame, DllInfo);
	}

	if (R_SetupFrame_RVA)
	{
		gPrivateFuncs.R_SetupFrame = (decltype(gPrivateFuncs.R_SetupFrame))VA_from_RVA(R_SetupFrame, RealDllInfo);
	}

	Sig_FuncNotFound(R_SetupFrame);
}

void R_FillAddress_R_SetupGL(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_SetupGL)
		return;

	ULONG_PTR R_SetupGL_VA = 0;
	ULONG R_SetupGL_RVA = 0;

	{
		const char pattern[] = "\x68\xE2\x0B\x00\x00\xFF\x2A\x68\xC0\x0B\x00\x00\xFF\x2A\x68\x71\x0B\x00\x00\xFF";

		/*
			.text:01D56353 68 E2 0B 00 00                                      push    0BE2h           ; cap
			.text:01D56358 FF D6                                               call    esi ; glDisable
			.text:01D5635A 68 C0 0B 00 00                                      push    0BC0h           ; cap
			.text:01D5635F FF D6                                               call    esi ; glDisable
			.text:01D56361 68 71 0B 00 00                                      push    0B71h           ; cap
			.text:01D56366 FF D7                                               call    edi ; glEnable
		*/

		auto SetupGL_Call = Search_Pattern(pattern, DllInfo);

		if (SetupGL_Call)
		{
			R_SetupGL_VA = (decltype(R_SetupGL_VA))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(SetupGL_Call, 0x600, [](PUCHAR Candidate) {

				if (Candidate[0] == 0x55 &&
					Candidate[1] == 0x8B &&
					Candidate[2] == 0xEC)
				{
					if (Search_Pattern_From_Size(Candidate, 15, "\x68\x01\x17\x00\x00") ||
						Search_Pattern_From_Size(Candidate, 15, "\x68\x02\x17\x00\x00"))
					{
						return TRUE;
					}
				}

				if (Candidate[-1] == 0xCC &&
					Candidate[0] >= 0x50 &&
					Candidate[0] <= 0x57)
				{
					if (Search_Pattern_From_Size(Candidate, 15, "\x68\x01\x17\x00\x00") ||
						Search_Pattern_From_Size(Candidate, 15, "\x68\x02\x17\x00\x00"))
					{
						return TRUE;
					}
				}

				return FALSE;
				});
			Convert_VA_to_RVA(R_SetupGL, DllInfo);
		}
		else
		{
			/*
				.text:01D45BE0 68 E2 0B 00 00                                      push    0BE2h
				.text:01D45BE5 FF 15 A4 1F 79 02                                   call    dword_2791FA4
				.text:01D45BEB 68 C0 0B 00 00                                      push    0BC0h
				.text:01D45BF0 FF 15 A4 1F 79 02                                   call    dword_2791FA4
				.text:01D45BF6 68 71 0B 00 00                                      push    0B71h
				.text:01D45BFB FF 15 3C 17 79 02                                   call    dword_279173C
			*/
			const char pattern2[] = "\x68\xE2\x0B\x00\x00\xFF\x2A\x68\xC0\x0B\x00\x00\xFF\x2A\x68\x71\x0B\x00\x00\xFF";
			SetupGL_Call = Search_Pattern(pattern2, DllInfo);

			if (SetupGL_Call)
			{
				R_SetupGL_VA = (decltype(R_SetupGL_VA))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(SetupGL_Call, 0x600, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
					{
						if (Search_Pattern_From_Size(Candidate, 15, "\x68\x01\x17\x00\x00") ||
							Search_Pattern_From_Size(Candidate, 15, "\x68\x02\x17\x00\x00"))
						{
							return TRUE;
						}
					}

					if (Candidate[-1] == 0xCC &&
						Candidate[0] >= 0x50 &&
						Candidate[0] <= 0x57)
					{
						if (Search_Pattern_From_Size(Candidate, 15, "\x68\x01\x17\x00\x00") ||
							Search_Pattern_From_Size(Candidate, 15, "\x68\x02\x17\x00\x00"))
						{
							return TRUE;
						}
					}

					return FALSE;
					});
				Convert_VA_to_RVA(R_SetupGL, DllInfo);
			}
		}
	}

	if (!R_SetupGL_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			if (g_dwEngineBuildnum >= 10152)
			{
				R_SetupGL_VA = (ULONG_PTR)Search_Pattern(R_SETUPGL_SIG_SVENGINE_10152, DllInfo);
			}
			else
			{
				R_SetupGL_VA = (ULONG_PTR)Search_Pattern(R_SETUPGL_SIG_SVENGINE, DllInfo);
			}
			Convert_VA_to_RVA(R_SetupGL, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			if (g_dwEngineBuildnum >= 9899)
			{
				R_SetupGL_VA = (ULONG_PTR)Search_Pattern(R_SETUPGL_SIG_HL25_9899, DllInfo);
			}
			else
			{
				R_SetupGL_VA = (ULONG_PTR)Search_Pattern(R_SETUPGL_SIG_HL25, DllInfo);
			}
			Convert_VA_to_RVA(R_SetupGL, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_SetupGL_VA = (ULONG_PTR)Search_Pattern(R_SETUPGL_SIG_NEW, DllInfo);

			//try another signature
			if (!R_SetupGL_VA)
				R_SetupGL_VA = (ULONG_PTR)Search_Pattern(R_SETUPGL_SIG_NEW2, DllInfo);

			Convert_VA_to_RVA(R_SetupGL, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_SetupGL_VA = (ULONG_PTR)Search_Pattern(R_SETUPGL_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(R_SetupGL, DllInfo);
		}
	}

	if (R_SetupGL_RVA)
	{
		gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))VA_from_RVA(R_SetupGL, RealDllInfo);
	}

	Sig_FuncNotFound(R_SetupGL);

	ULONG_PTR gWorldToScreen_VA = 0;
	ULONG gWorldToScreen_RVA = 0;

	ULONG_PTR gScreenToWorld_VA = 0;
	ULONG gScreenToWorld_RVA = 0;

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

		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size((void*)R_SetupGL_VA, 0x700, pattern, DllInfo);

		if (addr)
		{
			gWorldToScreen_VA = *(ULONG_PTR*)(addr + 6);
			gScreenToWorld_VA = *(ULONG_PTR*)(addr + 1);

			Convert_VA_to_RVA(gWorldToScreen, DllInfo);
			Convert_VA_to_RVA(gScreenToWorld, DllInfo);
		}
	}

	if (gWorldToScreen_RVA)
		gWorldToScreen = (decltype(gWorldToScreen))VA_from_RVA(gWorldToScreen, RealDllInfo);
	if (gScreenToWorld_RVA)
		gScreenToWorld = (decltype(gScreenToWorld))VA_from_RVA(gScreenToWorld, RealDllInfo);

	Sig_VarNotFound(gWorldToScreen);
	Sig_VarNotFound(gScreenToWorld);
}

void R_FillAddress_R_RenderView(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_RenderView_SvEngine || gPrivateFuncs.R_RenderView)
		return;

	ULONG_PTR R_RenderView_VA = 0;
	ULONG R_RenderView_RVA = 0;

	{
		const char sig[] = "R_RenderView: NULL worldmodel";
		auto R_RenderView_String = Search_Pattern_Data(sig, DllInfo);
		if (!R_RenderView_String)
			R_RenderView_String = Search_Pattern_Rdata(sig, DllInfo);
		if (R_RenderView_String)
		{
			char pattern[] = "\x75\x2A\x68\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 3) = (DWORD)R_RenderView_String;
			auto R_RenderView_PushString = Search_Pattern(pattern, DllInfo);
			if (R_RenderView_PushString)
			{
				R_RenderView_VA = (decltype(R_RenderView_VA))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_RenderView_PushString, 0x100, [](PUCHAR Candidate) {

					if (Candidate[0] == 0xD9 &&
						Candidate[1] == 0x05)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					//SvEngine 10182
					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC)
						return TRUE;

					return FALSE;
					});

				Convert_VA_to_RVA(R_RenderView, DllInfo);
			}
		}
	}

	if (!R_RenderView_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_RenderView_VA = (ULONG_PTR)Search_Pattern(R_RENDERVIEW_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(R_RenderView, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_RenderView_VA = (ULONG_PTR)Search_Pattern(R_RENDERVIEW_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(R_RenderView, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_RenderView_VA = (ULONG_PTR)Search_Pattern(R_RENDERVIEW_SIG_NEW, DllInfo);

			if (!R_RenderView_VA)
				R_RenderView_VA = (ULONG_PTR)Search_Pattern(R_RENDERVIEW_SIG_NEW2, DllInfo);

			Convert_VA_to_RVA(R_RenderView, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_RenderView_VA = (ULONG_PTR)Search_Pattern(R_RENDERVIEW_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(R_RenderView, DllInfo);
		}
	}

	if (R_RenderView_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_RenderView_SvEngine = (decltype(gPrivateFuncs.R_RenderView_SvEngine))VA_from_RVA(R_RenderView, RealDllInfo);
		}
		else
		{
			gPrivateFuncs.R_RenderView = (decltype(gPrivateFuncs.R_RenderView))VA_from_RVA(R_RenderView, RealDllInfo);
		}
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Sig_FuncNotFound(R_RenderView_SvEngine);
	}
	else
	{
		Sig_FuncNotFound(R_RenderView);
	}
}

void R_FillAddress_V_RenderView(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.V_RenderView)
		return;

	ULONG_PTR V_RenderView_VA = 0;
	ULONG V_RenderView_RVA = 0;

	ULONG_PTR R_RenderView_VA = (gPrivateFuncs.R_RenderView_SvEngine) ? (ULONG_PTR)gPrivateFuncs.R_RenderView_SvEngine : (ULONG_PTR)gPrivateFuncs.R_RenderView;
	ULONG R_RenderView_RVA = 0;

	//Convert R_RenderView_VA to DllInfo-based.
	Convert_VA_to_RVA(R_RenderView, RealDllInfo);
	Convert_RVA_to_VA(R_RenderView, DllInfo);

	{
		const char pattern[] = "\x68\x00\x40\x00\x00\xFF";
		/*
			.text:01DCDF5C 68 00 40 00 00                                      push    4000h           ; mask
			.text:01DCDF61 FF D3                                               call    ebx ; glClear
		*/
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				typedef struct
				{
					bool bFoundCallRenderView;
					ULONG_PTR R_RenderView_VA;
				}V_RenderView_SearchContext;

				V_RenderView_SearchContext ctx = { 0 };

				ctx.R_RenderView_VA = R_RenderView_VA;

				g_pMetaHookAPI->DisasmRanges(pFound + 5, 0x120, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (V_RenderView_SearchContext*)context;

					if (address[0] == 0xE8)
					{
						ULONG_PTR target = (decltype(target))pinst->detail->x86.operands[0].imm;

						if (target == ctx->R_RenderView_VA)
						{
							ctx->bFoundCallRenderView = true;
							return TRUE;
						}
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					}, 0, &ctx);

				if (ctx.bFoundCallRenderView)
				{
					V_RenderView_VA = (decltype(V_RenderView_VA))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x300, [](PUCHAR Candidate) {

						if (Candidate[0] == 0x81 &&
							Candidate[1] == 0xEC &&
							Candidate[4] == 0 &&
							Candidate[5] == 0)
							return TRUE;

						if (Candidate[0] == 0x55 &&
							Candidate[1] == 0x8B &&
							Candidate[2] == 0xEC)
							return TRUE;

						if (Candidate[0] == 0xA1 &&
							Candidate[5] == 0x81 &&
							Candidate[6] == 0xEC)
							return TRUE;

						return FALSE;
						});

					Convert_VA_to_RVA(V_RenderView, DllInfo);

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

	if (!V_RenderView_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			V_RenderView_VA = (ULONG_PTR)Search_Pattern(V_RENDERVIEW_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(V_RenderView, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			V_RenderView_VA = (ULONG_PTR)Search_Pattern(V_RENDERVIEW_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(V_RenderView, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			V_RenderView_VA = (ULONG_PTR)Search_Pattern(V_RENDERVIEW_SIG_NEW, DllInfo);

			if (!V_RenderView_VA)
				V_RenderView_VA = (ULONG_PTR)Search_Pattern(V_RENDERVIEW_SIG_NEW2, DllInfo);

			Convert_VA_to_RVA(V_RenderView, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			V_RenderView_VA = (ULONG_PTR)Search_Pattern(V_RENDERVIEW_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(V_RenderView, DllInfo);
		}
	}

	if (V_RenderView_RVA)
	{
		gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))VA_from_RVA(R_RenderView, RealDllInfo);
	}

	Sig_FuncNotFound(V_RenderView);
}

void R_FillAddress_R_RenderScene(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_RenderScene)
		return;

	ULONG_PTR R_RenderScene_VA = 0;
	ULONG R_RenderScene_RVA = 0;

	ULONG_PTR R_RenderView_VA = (gPrivateFuncs.R_RenderView_SvEngine) ? (ULONG_PTR)gPrivateFuncs.R_RenderView_SvEngine : (ULONG_PTR)gPrivateFuncs.R_RenderView;
	ULONG R_RenderView_RVA = 0;

	//Convert R_RenderView_VA to DllInfo-based.
	Convert_VA_to_RVA(R_RenderView, RealDllInfo);
	Convert_RVA_to_VA(R_RenderView, DllInfo);

	ULONG_PTR R_SetupGL_VA = (ULONG_PTR)gPrivateFuncs.R_SetupGL;
	ULONG R_SetupGL_RVA = 0;

	//Convert R_SetupGL_VA to DllInfo-based.
	Convert_VA_to_RVA(R_SetupGL, RealDllInfo);
	Convert_RVA_to_VA(R_SetupGL, DllInfo);

	{
		typedef struct
		{
			ULONG_PTR& R_RenderScene;
			ULONG_PTR R_SetupGL_VA{};
			bool bFoundCallSetupGL{};
		}R_RenderScene_SearchContext;

		R_RenderScene_SearchContext ctx = { R_RenderScene_VA, R_SetupGL_VA };

		g_pMetaHookAPI->DisasmRanges((PVOID)R_RenderView_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (R_RenderScene_SearchContext*)context;

			if (address[0] == 0xE8)
			{
				ULONG_PTR target = (decltype(target))pinst->detail->x86.operands[0].imm;

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

						ULONG_PTR target2 = (decltype(target2))pinst->detail->x86.operands[0].imm;

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
						ctx2.R_RenderScene = target;
						return TRUE;
					}
				}
			}

			if (ctx->bFoundCallSetupGL)
				return TRUE;

			if (ctx->R_RenderScene)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		Convert_VA_to_RVA(R_RenderScene, DllInfo);
	}

	if (gPrivateFuncs.R_RenderScene_inlined)
		return;

	if (!R_RenderScene_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			char pattern[] = "\xDD\xD8\xDD\xD8\xE8";
			auto addr = (ULONG_PTR)Search_Pattern_From(R_RenderView_VA, pattern, DllInfo);
			Sig_AddrNotFound(R_RenderScene);
			R_RenderScene_VA = (ULONG_PTR)GetCallAddress(addr + 4);
			Convert_VA_to_RVA(R_RenderScene, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_RenderScene_VA = (ULONG_PTR)Search_Pattern(R_RENDERSCENE_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(R_RenderScene, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_RenderScene_VA = (ULONG_PTR)Search_Pattern(R_RENDERSCENE_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(R_RenderScene, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_RenderScene_VA = (ULONG_PTR)Search_Pattern(R_RENDERSCENE_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(R_RenderScene, DllInfo);
		}
	}

	if (R_RenderScene_RVA)
	{
		gPrivateFuncs.R_RenderScene = (decltype(gPrivateFuncs.R_RenderScene))VA_from_RVA(R_RenderScene, RealDllInfo);
	}

	Sig_FuncNotFound(R_RenderScene);
}

void R_FillAddress_R_NewMap(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_NewMap)
		return;

	ULONG_PTR R_NewMap_VA = 0;
	ULONG R_NewMap_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_NewMap_VA = (ULONG_PTR)Search_Pattern(R_NEWMAP_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(R_NewMap, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_NewMap_VA = (ULONG_PTR)Search_Pattern(R_NEWMAP_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(R_NewMap, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_NewMap_VA = (ULONG_PTR)Search_Pattern(R_NEWMAP_SIG_NEW, DllInfo);
		Convert_VA_to_RVA(R_NewMap, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_NewMap_VA = (ULONG_PTR)Search_Pattern(R_NEWMAP_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_NewMap, DllInfo);
	}

	if (R_NewMap_RVA)
	{
		gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))VA_from_RVA(R_NewMap, RealDllInfo);
	}

	if (R_NewMap_RVA)
	{
		char pattern[] = "\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\xFF\xFF\xFF\xFF";
		auto addr = (ULONG_PTR)Search_Pattern_From_Size(R_NewMap_VA, 0x100, pattern);
		if (addr)
		{
			ULONG_PTR R_ClearParticles_VA = (decltype(R_ClearParticles_VA))GetCallAddress(addr + 0);
			ULONG_PTR R_DecalInit_VA = (decltype(R_DecalInit_VA))GetCallAddress(addr + 5);
			ULONG_PTR V_InitLevel_VA = (decltype(V_InitLevel_VA))GetCallAddress(addr + 10);
			ULONG_PTR GL_BuildLightmaps_VA = (decltype(GL_BuildLightmaps_VA))GetCallAddress(addr + 15);

			ULONG R_ClearParticles_RVA = 0;
			ULONG R_DecalInit_RVA = 0;
			ULONG V_InitLevel_RVA = 0;
			ULONG GL_BuildLightmaps_RVA = 0;

			Convert_VA_to_RVA(R_ClearParticles, DllInfo);
			Convert_VA_to_RVA(R_DecalInit, DllInfo);
			Convert_VA_to_RVA(V_InitLevel, DllInfo);
			Convert_VA_to_RVA(GL_BuildLightmaps, DllInfo);

			if (R_ClearParticles_RVA) gPrivateFuncs.R_ClearParticles = (decltype(gPrivateFuncs.R_ClearParticles))VA_from_RVA(R_ClearParticles, RealDllInfo);
			if (R_DecalInit_RVA) gPrivateFuncs.R_DecalInit = (decltype(gPrivateFuncs.R_DecalInit))VA_from_RVA(R_DecalInit, RealDllInfo);
			if (V_InitLevel_RVA) gPrivateFuncs.V_InitLevel = (decltype(gPrivateFuncs.V_InitLevel))VA_from_RVA(V_InitLevel, RealDllInfo);
			if (GL_BuildLightmaps_RVA) gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))VA_from_RVA(GL_BuildLightmaps, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_NewMap);
}

void R_FillAddress_GL_BuildLightmaps(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_BuildLightmaps)
		return;

	ULONG_PTR GL_BuildLightmaps_VA = 0;
	ULONG GL_BuildLightmaps_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		GL_BuildLightmaps_VA = (ULONG_PTR)Search_Pattern(GL_BUILDLIGHTMAPS_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(GL_BuildLightmaps, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		GL_BuildLightmaps_VA = (ULONG_PTR)Search_Pattern(GL_BUILDLIGHTMAPS_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(GL_BuildLightmaps, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		GL_BuildLightmaps_VA = (ULONG_PTR)Search_Pattern(GL_BUILDLIGHTMAPS_SIG_NEW, DllInfo);
		Convert_VA_to_RVA(GL_BuildLightmaps, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		GL_BuildLightmaps_VA = (ULONG_PTR)Search_Pattern(GL_BUILDLIGHTMAPS_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(GL_BuildLightmaps, DllInfo);
	}

	if (GL_BuildLightmaps_RVA)
	{
		gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))VA_from_RVA(GL_BuildLightmaps, RealDllInfo);
	}

	Sig_FuncNotFound(GL_BuildLightmaps);
}

void R_FillAddress_R_BuildLightMap(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_BuildLightMap)
		return;

	ULONG_PTR R_BuildLightMap_VA = 0;
	ULONG R_BuildLightMap_RVA = 0;

	{
		const char sigs[] = "Error: lightmap for texture %s too large";
		auto Error_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Error_String)
			Error_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (Error_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x18";
			*(DWORD*)(pattern + 1) = (DWORD)Error_String;
			auto Error_Call = Search_Pattern(pattern, DllInfo);
			if (Error_Call)
			{
				R_BuildLightMap_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Error_Call, 0x300, [](PUCHAR Candidate) {

					if (Candidate[0] == 0xD9 &&
						Candidate[1] == 0x05)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					return FALSE;
					});

				Convert_VA_to_RVA(R_BuildLightMap, DllInfo);
			}
		}
	}

	if (!R_BuildLightMap_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_BuildLightMap_VA = (ULONG_PTR)Search_Pattern(R_BUILDLIGHTMAP_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(R_BuildLightMap, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_BuildLightMap_VA = (ULONG_PTR)Search_Pattern(R_BUILDLIGHTMAP_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(R_BuildLightMap, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_BuildLightMap_VA = (ULONG_PTR)Search_Pattern(R_BUILDLIGHTMAP_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(R_BuildLightMap, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_BuildLightMap_VA = (ULONG_PTR)Search_Pattern(R_BUILDLIGHTMAP_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(R_BuildLightMap, DllInfo);
		}
	}

	if (R_BuildLightMap_RVA)
	{
		gPrivateFuncs.R_BuildLightMap = (decltype(gPrivateFuncs.R_BuildLightMap))VA_from_RVA(R_BuildLightMap, RealDllInfo);
	}

	Sig_FuncNotFound(R_BuildLightMap);
}

void R_FillAddress_R_AddDynamicLights(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_AddDynamicLights)
		return;

	ULONG_PTR	R_AddDynamicLights_VA = 0;
	ULONG		R_AddDynamicLights_RVA = 0;

	{
		typedef struct
		{
			ULONG_PTR& R_AddDynamicLights;
			PVOID base{};
			size_t max_insts{};
			int max_depth{};
			std::set<PVOID> code;
			std::set<PVOID> branches;
			std::vector<walk_context_t> walks;
		}R_AddDynamicLights_SearchContext;

		R_AddDynamicLights_SearchContext ctx = { R_AddDynamicLights_VA };

		ctx.base = gPrivateFuncs.R_BuildLightMap;
		ctx.max_insts = 1000;
		ctx.max_depth = 16;
		ctx.walks.emplace_back(ctx.base, 0x1000, 0);

		while (ctx.walks.size())
		{
			auto walk = ctx.walks[ctx.walks.size() - 1];
			ctx.walks.pop_back();

			g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_AddDynamicLights_SearchContext*)context;

				if (ctx->R_AddDynamicLights)
					return TRUE;

				if (ctx->code.size() > ctx->max_insts)
					return TRUE;

				if (ctx->code.find(address) != ctx->code.end())
					return TRUE;

				ctx->code.emplace(address);

				if (!ctx->R_AddDynamicLights &&
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
						ctx->R_AddDynamicLights = (ULONG_PTR)GetCallAddress(nextaddr);
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

		Convert_VA_to_RVA(R_AddDynamicLights, DllInfo);
	}

	if (!R_AddDynamicLights_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_AddDynamicLights_VA = (ULONG_PTR)Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(R_AddDynamicLights, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_AddDynamicLights_VA = (ULONG_PTR)Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(R_AddDynamicLights, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_AddDynamicLights_VA = (ULONG_PTR)Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(R_AddDynamicLights, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_AddDynamicLights_VA = (ULONG_PTR)Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(R_AddDynamicLights, DllInfo);
		}
	}

	if (R_AddDynamicLights_RVA)
	{
		gPrivateFuncs.R_AddDynamicLights = (decltype(gPrivateFuncs.R_AddDynamicLights))VA_from_RVA(R_AddDynamicLights, RealDllInfo);
	}

	Sig_FuncNotFound(R_AddDynamicLights);
}

void R_FillAddress_GL_DisableMultitexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_DisableMultitexture)
		return;

	ULONG_PTR GL_DisableMultitexture_VA = 0;
	ULONG GL_DisableMultitexture_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		GL_DisableMultitexture_VA = (ULONG_PTR)Search_Pattern(GL_DISABLEMULTITEXTURE_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(GL_DisableMultitexture, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		ULONG_PTR R_NewMap_VA = (ULONG_PTR)gPrivateFuncs.R_NewMap;
		ULONG R_NewMap_RVA = 0;

		Convert_VA_to_RVA(R_NewMap, RealDllInfo);
		Convert_RVA_to_VA(R_NewMap, DllInfo);

		GL_DisableMultitexture_VA = (ULONG_PTR)Search_Pattern_From(R_NewMap_VA, GL_DISABLEMULTITEXTURE_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(GL_DisableMultitexture, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		ULONG_PTR R_NewMap_VA = (ULONG_PTR)gPrivateFuncs.R_NewMap;
		ULONG R_NewMap_RVA = 0;

		Convert_VA_to_RVA(R_NewMap, RealDllInfo);
		Convert_RVA_to_VA(R_NewMap, DllInfo);

		GL_DisableMultitexture_VA = (ULONG_PTR)Search_Pattern_From(R_NewMap_VA, GL_DISABLEMULTITEXTURE_SIG_NEW, DllInfo);
		Convert_VA_to_RVA(GL_DisableMultitexture, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		ULONG_PTR R_NewMap_VA = (ULONG_PTR)gPrivateFuncs.R_NewMap;
		ULONG R_NewMap_RVA = 0;

		Convert_VA_to_RVA(R_NewMap, RealDllInfo);
		Convert_RVA_to_VA(R_NewMap, DllInfo);

		GL_DisableMultitexture_VA = (ULONG_PTR)Search_Pattern_From(R_NewMap_VA, GL_DISABLEMULTITEXTURE_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(GL_DisableMultitexture, DllInfo);
	}

	if (GL_DisableMultitexture_RVA)
	{
		gPrivateFuncs.GL_DisableMultitexture = (decltype(gPrivateFuncs.GL_DisableMultitexture))VA_from_RVA(GL_DisableMultitexture, RealDllInfo);
	}

	Sig_FuncNotFound(GL_DisableMultitexture);
}

void R_FillAddress_GL_EnableMultitexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_EnableMultitexture)
		return;

	ULONG_PTR GL_EnableMultitexture_VA = 0;
	ULONG GL_EnableMultitexture_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		GL_EnableMultitexture_VA = (ULONG_PTR)Search_Pattern(GL_ENABLEMULTITEXTURE_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(GL_EnableMultitexture, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		ULONG_PTR GL_DisableMultitexture_VA = (ULONG_PTR)gPrivateFuncs.GL_DisableMultitexture;
		ULONG GL_DisableMultitexture_RVA = 0;

		Convert_VA_to_RVA(GL_DisableMultitexture, RealDllInfo);
		Convert_RVA_to_VA(GL_DisableMultitexture, DllInfo);

		GL_EnableMultitexture_VA = (ULONG_PTR)Search_Pattern_From(GL_DisableMultitexture_VA, GL_ENABLEMULTITEXTURE_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(GL_EnableMultitexture, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		ULONG_PTR GL_DisableMultitexture_VA = (ULONG_PTR)gPrivateFuncs.GL_DisableMultitexture;
		ULONG GL_DisableMultitexture_RVA = 0;

		Convert_VA_to_RVA(GL_DisableMultitexture, RealDllInfo);
		Convert_RVA_to_VA(GL_DisableMultitexture, DllInfo);

		GL_EnableMultitexture_VA = (ULONG_PTR)Search_Pattern_From(GL_DisableMultitexture_VA, GL_ENABLEMULTITEXTURE_SIG_NEW, DllInfo);
		Convert_VA_to_RVA(GL_EnableMultitexture, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		ULONG_PTR GL_DisableMultitexture_VA = (ULONG_PTR)gPrivateFuncs.GL_DisableMultitexture;
		ULONG GL_DisableMultitexture_RVA = 0;

		Convert_VA_to_RVA(GL_DisableMultitexture, RealDllInfo);
		Convert_RVA_to_VA(GL_DisableMultitexture, DllInfo);

		GL_EnableMultitexture_VA = (ULONG_PTR)Search_Pattern_From(GL_DisableMultitexture_VA, GL_ENABLEMULTITEXTURE_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(GL_EnableMultitexture, DllInfo);
	}

	if (GL_EnableMultitexture_RVA)
	{
		gPrivateFuncs.GL_EnableMultitexture = (decltype(gPrivateFuncs.GL_EnableMultitexture))VA_from_RVA(GL_EnableMultitexture, RealDllInfo);
	}

	Sig_FuncNotFound(GL_EnableMultitexture);
}

void R_FillAddress_R_DrawSequentialPoly(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawSequentialPoly)
		return;

	ULONG_PTR R_DrawSequentialPoly_VA = 0;
	ULONG R_DrawSequentialPoly_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_DrawSequentialPoly_VA = (ULONG_PTR)Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(R_DrawSequentialPoly, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_DrawSequentialPoly_VA = (ULONG_PTR)Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(R_DrawSequentialPoly, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_DrawSequentialPoly_VA = (ULONG_PTR)Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_NEW, DllInfo);

		//try another signature
		if (!R_DrawSequentialPoly_VA)
			R_DrawSequentialPoly_VA = (ULONG_PTR)Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(R_DrawSequentialPoly, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_DrawSequentialPoly_VA = (ULONG_PTR)Search_Pattern(R_DRAWSEQUENTIALPOLY_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_DrawSequentialPoly, DllInfo);
	}

	if (R_DrawSequentialPoly_RVA)
	{
		gPrivateFuncs.R_DrawSequentialPoly = (decltype(gPrivateFuncs.R_DrawSequentialPoly))VA_from_RVA(R_DrawSequentialPoly, RealDllInfo);
	}

	Sig_FuncNotFound(R_DrawSequentialPoly);

	/*
		//Global pointers that link into engine vars
		byte *lightmaps = NULL;
		int *lightmap_textures = NULL;
		void *lightmap_rectchange = NULL;
		int *gDecalSurfCount = NULL;
		msurface_t **gDecalSurfs = NULL;
	*/
	ULONG_PTR lightmaps_VA = 0;
	ULONG lightmaps_RVA = 0;

	ULONG_PTR lightmap_textures_VA = 0;
	ULONG lightmap_textures_RVA = 0;

	ULONG_PTR lightmap_rectchange_VA = 0;
	ULONG lightmap_rectchange_RVA = 0;

	ULONG_PTR gDecalSurfCount_VA = 0;
	ULONG gDecalSurfCount_RVA = 0;

	ULONG_PTR gDecalSurfs_VA = 0;
	ULONG gDecalSurfs_RVA = 0;

	typedef struct
	{
		ULONG_PTR& lightmaps;
		ULONG_PTR& lightmap_textures;
		ULONG_PTR& lightmap_rectchange;
		ULONG_PTR& gDecalSurfs;
		ULONG_PTR& gDecalSurfCount;
		ULONG_PTR& R_RenderDynamicLightmaps;
		const mh_dll_info_t& DllInfo;

		std::set<PVOID> code{};
		std::set<PVOID> branches{};
		std::vector<walk_context_t> walks{};

		int mov_38_instcount{};
		ULONG_PTR decalsurf_candidateVA{};
		int decalsurf_instcount{};
		int decalsurf_register{};
	} R_DrawSequentialPoly_SearchContext;

	ULONG_PTR R_RenderDynamicLightmaps_VA = 0;
	ULONG R_RenderDynamicLightmaps_RVA = 0;

	R_DrawSequentialPoly_SearchContext ctx = {
		lightmaps_VA, lightmap_textures_VA, lightmap_rectchange_VA,
		gDecalSurfs_VA, gDecalSurfCount_VA, R_RenderDynamicLightmaps_VA, DllInfo
	};

	ctx.walks.emplace_back((void*)R_DrawSequentialPoly_VA, 0x1000, 0);

	while (ctx.walks.size())
	{
		auto walk = ctx.walks[ctx.walks.size() - 1];
		ctx.walks.pop_back();

		g_pMetaHookAPI->DisasmRanges(walk.address, walk.len, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (R_DrawSequentialPoly_SearchContext*)context;

			if (ctx->lightmap_textures && ctx->lightmap_rectchange && ctx->lightmaps && ctx->gDecalSurfs && ctx->gDecalSurfCount && ctx->R_RenderDynamicLightmaps)
				return TRUE;

			if (ctx->code.size() > 1000)
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
			else if (!ctx->lightmap_textures &&
				ctx->mov_38_instcount &&
				instCount < ctx->mov_38_instcount + 3 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.index != 0 &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.scale == 4 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D47A2B 8B 04 95 00 32 34 02                                mov     eax, lightmap_textures[edx*4]

				if ((ULONG_PTR)pinst->detail->x86.operands[1].mem.disp != (ULONG_PTR)lightmap_modified)
					ctx->lightmap_textures = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
			}
			else if (!ctx->lightmap_textures &&
				ctx->mov_38_instcount &&
				instCount < ctx->mov_38_instcount + 3 &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.index != 0 &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.scale == 4 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D5918C FF 34 85 C0 44 F5 03                                push    lightmap_textures[eax*4] 

				ctx->lightmap_textures = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			}
			else if (!ctx->lightmap_textures &&
				ctx->mov_38_instcount &&
				instCount < ctx->mov_38_instcount + 3 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//8B 3D 40 FB C0 02                                   mov     edi, lightmap_textures

				ctx->lightmap_textures = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
			}
			else if (!ctx->lightmap_rectchange &&
				ctx->mov_38_instcount &&
				instCount < ctx->mov_38_instcount + 15 &&
				pinst->id == X86_INS_ADD &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_ESI &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D591B8 81 C6 C8 D8 F5 03                                   add     esi, offset lightmap_rectchange

				ctx->lightmap_rectchange = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
			}
			else if (ctx->lightmap_rectchange && !ctx->lightmaps &&
				ctx->mov_38_instcount &&
				instCount < ctx->mov_38_instcount + 20 &&
				pinst->id == X86_INS_ADD &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[1].imm >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D591C9 81 C1 C8 18 FE 03                                   add     ecx, offset lightmaps 

				ctx->lightmaps = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index != 0 &&
				pinst->detail->x86.operands[0].mem.scale == 4 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//89 3C 85 F8 8C FE 07 mov     gDecalSurfs[eax*4], edi

				ctx->decalsurf_candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				ctx->decalsurf_instcount = instCount;
				ctx->decalsurf_register = pinst->detail->x86.operands[0].mem.index;
			}
			else if (!ctx->gDecalSurfs &&
				ctx->decalsurf_candidateVA &&
				instCount < ctx->decalsurf_instcount + 3 &&
				pinst->id == X86_INS_INC &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == ctx->decalsurf_register)
			{//.text:01D47C0F 40                                                  inc     eax

				ctx->gDecalSurfs = (ULONG_PTR)ctx->decalsurf_candidateVA;
			}
			else if (!ctx->gDecalSurfCount &&
				ctx->decalsurf_candidateVA &&
				instCount < ctx->decalsurf_instcount + 5 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				pinst->detail->x86.operands[0].mem.scale == 1 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == ctx->decalsurf_register)
			{//.text:01D47C15 A3 98 26 34 02                                      mov     gDecalSurfCount, eax

				ctx->gDecalSurfCount = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			}
			if (address[0] == 0xE8)
			{
				PVOID target = (decltype(target))pinst->detail->x86.operands[0].imm;

				typedef struct
				{
					bool bFoundImm14h{};
					bool bFoundPush200{};
				} R_DrawSequentialPoly_FindRenderDynamicLightsContext;

				R_DrawSequentialPoly_FindRenderDynamicLightsContext ctx2 = {};

				g_pMetaHookAPI->DisasmRanges(target, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx2 = (R_DrawSequentialPoly_FindRenderDynamicLightsContext*)context;

					if (!ctx2->bFoundImm14h)
					{
						if (pinst->detail->x86.op_count >= 2 &&
							pinst->detail->x86.operands[pinst->detail->x86.op_count - 1].type == X86_OP_IMM &&
							pinst->detail->x86.operands[pinst->detail->x86.op_count - 1].imm == 0x14)
						{
							ctx2->bFoundImm14h = true;
						}
					}

					if (!ctx2->bFoundPush200)
					{
						if (pinst->id == X86_INS_PUSH &&
							pinst->detail->x86.op_count == 1 &&
							pinst->detail->x86.operands[0].type == X86_OP_IMM &&
							pinst->detail->x86.operands[0].imm == 0x200)
						{
							ctx2->bFoundPush200 = true;
						}
					}

					if (ctx2->bFoundImm14h && ctx2->bFoundPush200)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					}, 0, &ctx2);

				if (ctx2.bFoundImm14h && ctx2.bFoundPush200)
				{
					ctx->R_RenderDynamicLightmaps = (ULONG_PTR)target;
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
					if (depth + 1 < 16)
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

	Convert_VA_to_RVA(lightmap_textures, DllInfo);
	Convert_VA_to_RVA(lightmap_rectchange, DllInfo);
	Convert_VA_to_RVA(lightmaps, DllInfo);
	Convert_VA_to_RVA(gDecalSurfs, DllInfo);
	Convert_VA_to_RVA(gDecalSurfCount, DllInfo);
	Convert_VA_to_RVA(R_RenderDynamicLightmaps, DllInfo);

	if (lightmap_textures_RVA)
		lightmap_textures = (decltype(lightmap_textures))VA_from_RVA(lightmap_textures, RealDllInfo);
	if (lightmap_rectchange_RVA)
		lightmap_rectchange = (decltype(lightmap_rectchange))VA_from_RVA(lightmap_rectchange, RealDllInfo);
	if (lightmaps_RVA)
		lightmaps = (decltype(lightmaps))VA_from_RVA(lightmaps, RealDllInfo);
	if (gDecalSurfs_RVA)
		gDecalSurfs = (decltype(gDecalSurfs))VA_from_RVA(gDecalSurfs, RealDllInfo);
	if (gDecalSurfCount_RVA)
		gDecalSurfCount = (decltype(gDecalSurfCount))VA_from_RVA(gDecalSurfCount, RealDllInfo);
	if (R_RenderDynamicLightmaps_RVA)
		gPrivateFuncs.R_RenderDynamicLightmaps = (decltype(gPrivateFuncs.R_RenderDynamicLightmaps))VA_from_RVA(R_RenderDynamicLightmaps, RealDllInfo);

	Sig_VarNotFound(lightmap_textures);
	Sig_VarNotFound(lightmap_rectchange);
	Sig_VarNotFound(lightmaps);
	Sig_VarNotFound(gDecalSurfs);
	Sig_VarNotFound(gDecalSurfCount);
}

void R_FillAddress_R_TextureAnimation(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_TextureAnimation)
		return;

	ULONG_PTR R_TextureAnimation_VA = 0;
	ULONG R_TextureAnimation_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_TextureAnimation_VA = (ULONG_PTR)Search_Pattern(R_TEXTUREANIMATION_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(R_TextureAnimation, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_TextureAnimation_VA = (ULONG_PTR)Search_Pattern(R_TEXTUREANIMATION_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(R_TextureAnimation, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_TextureAnimation_VA = (ULONG_PTR)Search_Pattern(R_TEXTUREANIMATION_SIG_NEW, DllInfo);

		//try another signature
		if (!R_TextureAnimation_VA)
			R_TextureAnimation_VA = (ULONG_PTR)Search_Pattern(R_TEXTUREANIMATION_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(R_TextureAnimation, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_TextureAnimation_VA = (ULONG_PTR)Search_Pattern(R_TEXTUREANIMATION_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_TextureAnimation, DllInfo);
	}

	if (R_TextureAnimation_RVA)
	{
		gPrivateFuncs.R_TextureAnimation = (decltype(gPrivateFuncs.R_TextureAnimation))VA_from_RVA(R_TextureAnimation, RealDllInfo);
	}

	Sig_FuncNotFound(R_TextureAnimation);

	//int(*rtable)[20][20] = NULL;

	ULONG_PTR rtable_VA = 0;
	ULONG rtable_RVA = 0;
	{
		typedef struct
		{
			ULONG_PTR& rtable;
			const mh_dll_info_t& DllInfo;
		} R_TextureAnimation_SearchContext;

		R_TextureAnimation_SearchContext ctx = { rtable_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)R_TextureAnimation_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_TextureAnimation_SearchContext*)context;

				if (!ctx->rtable &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_ESI &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//.text:01D58B66 BE F0 5B 00 08 mov     esi, offset rtable

					ctx->rtable = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
				}

				if (ctx->rtable)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(rtable, DllInfo);
	}

	if (rtable_RVA)
		rtable = (decltype(rtable))VA_from_RVA(rtable, RealDllInfo);

	Sig_VarNotFound(rtable);
}

void R_FillAddress_R_DrawBrushModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawBrushModel)
		return;

	ULONG_PTR R_DrawBrushModel_VA = 0;
	ULONG R_DrawBrushModel_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_DrawBrushModel_VA = (ULONG_PTR)Search_Pattern(R_DRAWBRUSHMODEL_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(R_DrawBrushModel, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_DrawBrushModel_VA = (ULONG_PTR)Search_Pattern(R_DRAWBRUSHMODEL_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(R_DrawBrushModel, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_DrawBrushModel_VA = (ULONG_PTR)Search_Pattern(R_DRAWBRUSHMODEL_SIG_NEW, DllInfo);

		//try another signature
		if (!R_DrawBrushModel_VA)
			R_DrawBrushModel_VA = (ULONG_PTR)Search_Pattern(R_DRAWBRUSHMODEL_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(R_DrawBrushModel, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_DrawBrushModel_VA = (ULONG_PTR)Search_Pattern(R_DRAWBRUSHMODEL_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_DrawBrushModel, DllInfo);
	}

	if (R_DrawBrushModel_RVA)
	{
		gPrivateFuncs.R_DrawBrushModel = (decltype(gPrivateFuncs.R_DrawBrushModel))VA_from_RVA(R_DrawBrushModel, RealDllInfo);
	}

	Sig_FuncNotFound(R_DrawBrushModel);
}

void R_FillAddress_R_RecursiveWorldNode(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_RecursiveWorldNode)
		return;

	ULONG_PTR R_RecursiveWorldNode_VA = 0;
	ULONG R_RecursiveWorldNode_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_RecursiveWorldNode_VA = (ULONG_PTR)Search_Pattern(R_RECURSIVEWORLDNODE_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(R_RecursiveWorldNode, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_RecursiveWorldNode_VA = (ULONG_PTR)Search_Pattern_From(gPrivateFuncs.R_DrawSequentialPoly, R_RECURSIVEWORLDNODE_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(R_RecursiveWorldNode, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_RecursiveWorldNode_VA = (ULONG_PTR)Search_Pattern_From(gPrivateFuncs.R_DrawBrushModel, R_RECURSIVEWORLDNODE_SIG_NEW, DllInfo);

		//try another signature
		if (!R_RecursiveWorldNode_VA)
			R_RecursiveWorldNode_VA = (ULONG_PTR)Search_Pattern_From(gPrivateFuncs.R_DrawBrushModel, R_RECURSIVEWORLDNODE_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(R_RecursiveWorldNode, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_RecursiveWorldNode_VA = (ULONG_PTR)Search_Pattern_From(gPrivateFuncs.R_DrawBrushModel, R_RECURSIVEWORLDNODE_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_RecursiveWorldNode, DllInfo);
	}

	if (R_RecursiveWorldNode_RVA)
	{
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))VA_from_RVA(R_RecursiveWorldNode, RealDllInfo);
	}

	Sig_FuncNotFound(R_RecursiveWorldNode);
}

void R_FillAddress_R_DrawWorld(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawWorld)
		return;

	ULONG_PTR R_DrawWorld_VA = 0;
	ULONG R_DrawWorld_RVA = 0;

	{
		/*
		68 b8 0b 00 00          push   BB8h // imm = 3000
		*/
		const char pattern[] = "\x68\xB8\x0B\x00\x00\x8D";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);

			if (pFound)
			{
				typedef struct
				{
					bool bFoundLeaEax{};
					bool bFoundPushZero{};
				}R_DrawWorld_SearchContext;

				R_DrawWorld_SearchContext ctx = { 0 };

				g_pMetaHookAPI->DisasmRanges(pFound + 5, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (R_DrawWorld_SearchContext*)context;

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

					if (ctx->bFoundLeaEax && ctx->bFoundPushZero)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					}, 0, &ctx);

				if (ctx.bFoundLeaEax && ctx.bFoundPushZero)
				{
					R_DrawWorld_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x300, [](PUCHAR Candidate) {

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

					Convert_VA_to_RVA(R_DrawWorld, DllInfo);
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

	if (!R_DrawWorld_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_DrawWorld_VA = (ULONG_PTR)Search_Pattern(R_DRAWWORLD_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(R_DrawWorld, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			ULONG_PTR R_DrawSequentialPoly_VA = (ULONG_PTR)gPrivateFuncs.R_DrawSequentialPoly;
			ULONG R_DrawSequentialPoly_RVA = 0;

			// Convert R_DrawSequentialPoly_VA to DllInfo-based
			Convert_VA_to_RVA(R_DrawSequentialPoly, RealDllInfo);
			Convert_RVA_to_VA(R_DrawSequentialPoly, DllInfo);

			R_DrawWorld_VA = (ULONG_PTR)Search_Pattern_From(R_DrawSequentialPoly_VA, R_DRAWWORLD_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(R_DrawWorld, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			ULONG_PTR R_RecursiveWorldNode_VA = (ULONG_PTR)gPrivateFuncs.R_RecursiveWorldNode;
			ULONG R_RecursiveWorldNode_RVA = 0;

			// Convert R_RecursiveWorldNode_VA to DllInfo-based
			Convert_VA_to_RVA(R_RecursiveWorldNode, RealDllInfo);
			Convert_RVA_to_VA(R_RecursiveWorldNode, DllInfo);

			R_DrawWorld_VA = (ULONG_PTR)Search_Pattern_From(R_RecursiveWorldNode_VA, R_DRAWWORLD_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(R_DrawWorld, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			ULONG_PTR R_RecursiveWorldNode_VA = (ULONG_PTR)gPrivateFuncs.R_RecursiveWorldNode;
			ULONG R_RecursiveWorldNode_RVA = 0;

			// Convert R_RecursiveWorldNode_VA to DllInfo-based
			Convert_VA_to_RVA(R_RecursiveWorldNode, RealDllInfo);
			Convert_RVA_to_VA(R_RecursiveWorldNode, DllInfo);

			R_DrawWorld_VA = (ULONG_PTR)Search_Pattern_From(R_RecursiveWorldNode_VA, R_DRAWWORLD_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(R_DrawWorld, DllInfo);
		}
	}

	if (R_DrawWorld_RVA)
	{
		gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))VA_from_RVA(R_DrawWorld, RealDllInfo);
	}

	Sig_FuncNotFound(R_DrawWorld);

	/*
		 //Global pointers that link into engine vars
		 vec_t *modelorg = NULL;
	 */
	ULONG_PTR modelorg_VA = 0;
	ULONG modelorg_RVA = 0;

	typedef struct
	{
		const mh_dll_info_t& DllInfo;
		int candidate_count{};
		ULONG_PTR candidateVA[10]{};
	} R_DrawWorld_SearchContext;

	R_DrawWorld_SearchContext ctx = { DllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)R_DrawWorld_VA, 0x130, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_DrawWorld_SearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//.text:01D49479 89 15 C0 96 BC 02                                   mov     modelorg, edx

				if (!ctx->candidateVA[ctx->candidate_count] && ctx->candidate_count < 10)
				{
					ctx->candidateVA[ctx->candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->candidate_count++;
				}
			}
			else if (pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{// movss   dword ptr modelorg, xmm0

				if (!ctx->candidateVA[ctx->candidate_count] && ctx->candidate_count < 10)
				{
					ctx->candidateVA[ctx->candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->candidate_count++;
				}
			}
			else if (pinst->id == X86_INS_FSTP &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D49479 89 15 C0 96 BC 02                                   mov     modelorg, edx

				if (!ctx->candidateVA[ctx->candidate_count] && ctx->candidate_count < 10)
				{
					ctx->candidateVA[ctx->candidate_count] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
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
		std::qsort(ctx.candidateVA, ctx.candidate_count, sizeof(ctx.candidateVA[0]), [](const void* a, const void* b) {
			return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
			});

		//other, other, other, modelorg[0], modelorg[1], modelorg[2]
		if (ctx.candidateVA[ctx.candidate_count - 3] + 4 == ctx.candidateVA[ctx.candidate_count - 2] &&
			ctx.candidateVA[ctx.candidate_count - 2] + 4 == ctx.candidateVA[ctx.candidate_count - 1])
		{
			modelorg_VA = ctx.candidateVA[ctx.candidate_count - 3];
		}
		//modelorg[0], modelorg[1], modelorg[2], other, other, other
		else if (ctx.candidateVA[0] + 4 == ctx.candidateVA[1] &&
			ctx.candidateVA[1] + 4 == ctx.candidateVA[2])
		{
			modelorg_VA = ctx.candidateVA[0];
		}
	}
	else if (ctx.candidate_count == 1)
	{
		modelorg_VA = ctx.candidateVA[0];
	}

	Convert_VA_to_RVA(modelorg, DllInfo);

	if (modelorg_RVA)
		modelorg = (decltype(modelorg))VA_from_RVA(modelorg, RealDllInfo);

	Sig_VarNotFound(modelorg);
}

void R_FillAddress_R_DrawViewModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawViewModel)
		return;

	ULONG_PTR R_DrawViewModel_VA = 0;
	ULONG R_DrawViewModel_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//inlined
	}
	else
	{
		ULONG_PTR R_RenderView_VA = (ULONG_PTR)gPrivateFuncs.R_RenderView;
		ULONG R_RenderView_RVA = 0;

		// Convert R_RenderView_VA to DllInfo-based
		Convert_VA_to_RVA(R_RenderView, RealDllInfo);
		Convert_RVA_to_VA(R_RenderView, DllInfo);

		ULONG_PTR R_PolyBlend_VA = (ULONG_PTR)gPrivateFuncs.R_PolyBlend;
		ULONG R_PolyBlend_RVA = 0;

		// Convert R_PolyBlend_VA to DllInfo-based
		Convert_VA_to_RVA(R_PolyBlend, RealDllInfo);
		Convert_RVA_to_VA(R_PolyBlend, DllInfo);

		ULONG_PTR S_ExtraUpdate_VA = (ULONG_PTR)gPrivateFuncs.S_ExtraUpdate;
		ULONG S_ExtraUpdate_RVA = 0;

		// Convert S_ExtraUpdate_VA to DllInfo-based
		Convert_VA_to_RVA(S_ExtraUpdate, RealDllInfo);
		Convert_RVA_to_VA(S_ExtraUpdate, DllInfo);

		const char pattern[] = "\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
		PUCHAR SearchBegin = (PUCHAR)R_RenderView_VA;
		PUCHAR SearchLimit = SearchBegin + 0x500;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				auto target1 = GetCallAddress(pFound + 0);
				auto target2 = GetCallAddress(pFound + 5);
				auto target3 = GetCallAddress(pFound + 10);

				if ((ULONG_PTR)target2 == R_PolyBlend_VA && (ULONG_PTR)target3 == S_ExtraUpdate_VA)
				{
					R_DrawViewModel_VA = (ULONG_PTR)target1;
					Convert_VA_to_RVA(R_DrawViewModel, DllInfo);
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

	if (R_DrawViewModel_RVA)
	{
		gPrivateFuncs.R_DrawViewModel = (decltype(gPrivateFuncs.R_DrawViewModel))VA_from_RVA(R_DrawViewModel, RealDllInfo);
	}

	Sig_FuncNotFound(R_DrawViewModel);
}

void R_FillAddress_R_MarkLeaves(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_MarkLeaves)
		return;

	ULONG_PTR R_MarkLeaves_VA = 0;
	ULONG R_MarkLeaves_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_MarkLeaves_VA = (ULONG_PTR)Search_Pattern(R_MARKLEAVES_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(R_MarkLeaves, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_MarkLeaves_VA = (ULONG_PTR)Search_Pattern(R_MARKLEAVES_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(R_MarkLeaves, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_MarkLeaves_VA = (ULONG_PTR)Search_Pattern(R_MARKLEAVES_SIG_NEW, DllInfo);

		if (!R_MarkLeaves_VA)
			R_MarkLeaves_VA = (ULONG_PTR)Search_Pattern(R_MARKLEAVES_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(R_MarkLeaves, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_MarkLeaves_VA = (ULONG_PTR)Search_Pattern(R_MARKLEAVES_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_MarkLeaves, DllInfo);
	}

	if (R_MarkLeaves_RVA)
	{
		gPrivateFuncs.R_MarkLeaves = (decltype(gPrivateFuncs.R_MarkLeaves))VA_from_RVA(R_MarkLeaves, RealDllInfo);
	}

	Sig_FuncNotFound(R_MarkLeaves);
}

void R_FillAddress_GL_BeginRendering(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_BeginRendering)
		return;

	ULONG_PTR GL_BeginRendering_VA = 0;
	ULONG GL_BeginRendering_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		GL_BeginRendering_VA = (ULONG_PTR)Search_Pattern(GL_BEGINRENDERING_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(GL_BeginRendering, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		GL_BeginRendering_VA = (ULONG_PTR)Search_Pattern(GL_BEGINRENDERING_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(GL_BeginRendering, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		GL_BeginRendering_VA = (ULONG_PTR)Search_Pattern(GL_BEGINRENDERING_SIG_NEW, DllInfo);

		if (!GL_BeginRendering_VA)
			GL_BeginRendering_VA = (ULONG_PTR)Search_Pattern(GL_BEGINRENDERING_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(GL_BeginRendering, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		GL_BeginRendering_VA = (ULONG_PTR)Search_Pattern(GL_BEGINRENDERING_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(GL_BeginRendering, DllInfo);
	}

	if (GL_BeginRendering_RVA)
	{
		gPrivateFuncs.GL_BeginRendering = (decltype(gPrivateFuncs.GL_BeginRendering))VA_from_RVA(GL_BeginRendering, RealDllInfo);
	}

	Sig_FuncNotFound(GL_BeginRendering);
}

void R_FillAddress_GL_EndRendering(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_EndRendering)
		return;

	ULONG_PTR GL_EndRendering_VA = 0;
	ULONG GL_EndRendering_RVA = 0;

	if (g_iEngineType == ENGINE_GOLDSRC || g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		auto addr = Search_Pattern(GL_ENDRENDERING_SIG_COMMON_GOLDSRC, DllInfo);

		if (addr)
		{
			GL_EndRendering_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(addr, 0x500, [](PUCHAR Candidate) {

				if (Candidate[0] == 0x55 &&
					Candidate[1] == 0x8B &&
					Candidate[2] == 0xEC &&
					Candidate[3] == 0x83)
					return TRUE;

				return FALSE;
				});
			Convert_VA_to_RVA(GL_EndRendering, DllInfo);
		}
	}

	if (!GL_EndRendering_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			GL_EndRendering_VA = (ULONG_PTR)Search_Pattern(GL_ENDRENDERING_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(GL_EndRendering, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			GL_EndRendering_VA = (ULONG_PTR)Search_Pattern(GL_ENDRENDERING_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(GL_EndRendering, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			ULONG_PTR GL_BeginRendering_VA = (ULONG_PTR)gPrivateFuncs.GL_BeginRendering;
			ULONG GL_BeginRendering_RVA = 0;

			Convert_VA_to_RVA(GL_BeginRendering, RealDllInfo);
			Convert_RVA_to_VA(GL_BeginRendering, DllInfo);

			if (g_bHasOfficialFBOSupport)
			{
				GL_EndRendering_VA = (ULONG_PTR)Search_Pattern_From_Size(GL_BeginRendering_VA, 0x100, GL_ENDRENDERING_SIG_NEW, DllInfo);
			}
			else
			{
				GL_EndRendering_VA = (ULONG_PTR)Search_Pattern_From_Size(GL_BeginRendering_VA, 0x100, GL_ENDRENDERING_SIG_BLOB, DllInfo);
			}
			Convert_VA_to_RVA(GL_EndRendering, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			ULONG_PTR GL_BeginRendering_VA = (ULONG_PTR)gPrivateFuncs.GL_BeginRendering;
			ULONG GL_BeginRendering_RVA = 0;

			Convert_VA_to_RVA(GL_BeginRendering, RealDllInfo);
			Convert_RVA_to_VA(GL_BeginRendering, DllInfo);

			GL_EndRendering_VA = (ULONG_PTR)Search_Pattern_From_Size(GL_BeginRendering_VA, 0x100, GL_ENDRENDERING_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(GL_EndRendering, DllInfo);
		}
	}

	if (GL_EndRendering_RVA)
	{
		gPrivateFuncs.GL_EndRendering = (decltype(gPrivateFuncs.GL_EndRendering))VA_from_RVA(GL_EndRendering, RealDllInfo);
	}

	Sig_FuncNotFound(GL_EndRendering);
}

void R_FillAddress_EmitWaterPolys(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

void R_FillAddress_VID_UpdateWindowVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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
			addr = Search_Pattern_From_Size(addr, 0x50, "\x50\xE8", DllInfo);

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

void R_FillAddress_Mod_PointInLeaf(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Mod_PointInLeaf)
		return;

	ULONG_PTR Mod_PointInLeaf_VA = 0;
	ULONG Mod_PointInLeaf_RVA = 0;

	{
		const char sigs[] = "Mod_PointInLeaf: bad model\0";
		auto BadModel_String = Search_Pattern_Data(sigs, DllInfo);
		if (!BadModel_String)
			BadModel_String = Search_Pattern_Rdata(sigs, DllInfo);

		if (BadModel_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
			*(DWORD*)(pattern + 1) = (DWORD)BadModel_String;
			auto BadModel_Call = Search_Pattern(pattern, DllInfo);

			if (BadModel_Call)
			{
				Mod_PointInLeaf_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(BadModel_Call, 0x100, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x56 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0x74)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					if (Candidate[0] == 0x8B &&
						Candidate[2] == 0x24)
						return TRUE;

					return FALSE;
					});
				Convert_VA_to_RVA(Mod_PointInLeaf, DllInfo);
			}
		}
	}

	if (!Mod_PointInLeaf_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			Mod_PointInLeaf_VA = (ULONG_PTR)Search_Pattern(MOD_POINTINLEAF_SIG_SVENGINE, DllInfo);

			if (!Mod_PointInLeaf_VA)
				Mod_PointInLeaf_VA = (ULONG_PTR)Search_Pattern(MOD_POINTINLEAF_SIG_SVENGINE_10152, DllInfo);

			Convert_VA_to_RVA(Mod_PointInLeaf, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			Mod_PointInLeaf_VA = (ULONG_PTR)Search_Pattern(MOD_POINTINLEAF_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(Mod_PointInLeaf, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			Mod_PointInLeaf_VA = (ULONG_PTR)Search_Pattern(MOD_POINTINLEAF_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(Mod_PointInLeaf, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			Mod_PointInLeaf_VA = (ULONG_PTR)Search_Pattern(MOD_POINTINLEAF_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(Mod_PointInLeaf, DllInfo);
		}
	}

	if (Mod_PointInLeaf_RVA)
	{
		gPrivateFuncs.Mod_PointInLeaf = (decltype(gPrivateFuncs.Mod_PointInLeaf))VA_from_RVA(Mod_PointInLeaf, RealDllInfo);
	}

	Sig_FuncNotFound(Mod_PointInLeaf);
}

void R_FillAddress_R_DrawTEntitiesOnList(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawTEntitiesOnList)
		return;

	ULONG_PTR R_DrawTEntitiesOnList_VA = 0;
	ULONG R_DrawTEntitiesOnList_RVA = 0;

	{
		const char sigs[] = "Non-sprite set to glow";
		auto NonSprite_String = Search_Pattern_Data(sigs, DllInfo);
		if (!NonSprite_String)
			NonSprite_String = Search_Pattern_Rdata(sigs, DllInfo);

		if (NonSprite_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B";
			*(DWORD*)(pattern + 1) = (DWORD)NonSprite_String;
			auto NonSprite_Call = Search_Pattern(pattern, DllInfo);

			if (NonSprite_Call)
			{
				R_DrawTEntitiesOnList_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(NonSprite_Call, 0x500, [](PUCHAR Candidate) {

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
				Convert_VA_to_RVA(R_DrawTEntitiesOnList, DllInfo);
			}
		}
	}

	if (!R_DrawTEntitiesOnList_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(R_DrawTEntitiesOnList, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(R_DrawTEntitiesOnList, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW, DllInfo);

			if (!R_DrawTEntitiesOnList_VA)
				R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW2, DllInfo);

			Convert_VA_to_RVA(R_DrawTEntitiesOnList, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(R_DrawTEntitiesOnList, DllInfo);
		}
	}

	if (R_DrawTEntitiesOnList_RVA)
	{
		gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))VA_from_RVA(R_DrawTEntitiesOnList, RealDllInfo);
	}

	Sig_FuncNotFound(R_DrawTEntitiesOnList);
}

void R_FillAddress_BuildGammaTable(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.BuildGammaTable)
		return;

	ULONG_PTR BuildGammaTable_VA = 0;
	ULONG BuildGammaTable_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//no impl
	}
	else
	{
		/*
		.text:01DC2840 68 00 00 20 40                                      push    40200000h       ; float
		.text:01DC2845 E8 26 EF FF FF                                      call    sub_1DC1770
		*/
		const char pattern[] = "\x00\x00\x20\x40\xE8\x2A\x2A\x2A\x2A";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = SearchBegin + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				auto calltarget = GetCallAddress(pFound + 4);

				if ((ULONG_PTR)calltarget > (ULONG_PTR)DllInfo.TextBase && (ULONG_PTR)calltarget < (ULONG_PTR)DllInfo.TextBase + DllInfo.TextSize)
				{
					BuildGammaTable_VA = (ULONG_PTR)calltarget;
					Convert_VA_to_RVA(BuildGammaTable, DllInfo);
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

	if (!BuildGammaTable_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			BuildGammaTable_VA = (ULONG_PTR)Search_Pattern(BUILDGAMMATABLE_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(BuildGammaTable, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			BuildGammaTable_VA = (ULONG_PTR)Search_Pattern(BUILDGAMMATABLE_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(BuildGammaTable, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			BuildGammaTable_VA = (ULONG_PTR)Search_Pattern(BUILDGAMMATABLE_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(BuildGammaTable, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			BuildGammaTable_VA = (ULONG_PTR)Search_Pattern(BUILDGAMMATABLE_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(BuildGammaTable, DllInfo);
		}
	}

	if (BuildGammaTable_RVA)
	{
		gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))VA_from_RVA(BuildGammaTable, RealDllInfo);
	}

	Sig_FuncNotFound(BuildGammaTable);

	/*
	//Global pointers that link into engine vars
	byte *texgammatable = NULL;
	*/
	ULONG_PTR texgammatable_VA = 0;
	ULONG texgammatable_RVA = 0;

	if (1)
	{
		typedef struct
		{
			ULONG_PTR& texgammatable;
			const mh_dll_info_t& DllInfo;
		} BuildGammaTable_SearchContext;

		BuildGammaTable_SearchContext ctx = { texgammatable_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.BuildGammaTable, 0x250, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (BuildGammaTable_SearchContext*)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == X86_REG_ESI &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].size == 1)
				{
					ctx->texgammatable = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}

				if (ctx->texgammatable)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(texgammatable, DllInfo);
	}

	if (texgammatable_RVA)
		texgammatable = (decltype(texgammatable))VA_from_RVA(texgammatable, RealDllInfo);

	Sig_VarNotFound(texgammatable);
}

void R_FillAddress_R_DrawParticles(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawParticles)
		return;

	ULONG_PTR R_DrawParticles_VA = 0;
	ULONG R_DrawParticles_RVA = 0;

	{
		/*
			.text:01D82DA6 83 C4 04                                            add     esp, 4
			.text:01D82DA9 68 C0 0B 00 00                                      push    0BC0h           ; cap
		*/
		const char pattern[] = "\x83\xC4\x04\x68\xC0\x0B\x00\x00";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				typedef struct
				{
					bool bFound2200h{};
					bool bFound2300h{};
					bool bFound303h{};
					bool bFound302h{};
				}R_DrawParticle_SearchContext;

				R_DrawParticle_SearchContext ctx = { 0 };

				g_pMetaHookAPI->DisasmRanges(pFound, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (R_DrawParticle_SearchContext*)context;

					/*
						.text:01D82DC1 68 00 22 00 00                                      push    2200h           ; pname
						.text:01D82DC6 68 00 23 00 00                                      push    2300h           ; target
						.text:01D82DCB FF 15 20 13 E6 01                                   call    ds:glTexEnvf
					*/

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

					/*
						.text:01D82DD1 68 03 03 00 00                                      push    303h            ; dfactor
						.text:01D82DD6 68 02 03 00 00                                      push    302h            ; sfactor
						.text:01D82DDB FF 15 18 13 E6 01                                   call    ds:glBlendFunc
					*/

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

					}, 0, &ctx);

				if (ctx.bFound2200h && ctx.bFound2300h && ctx.bFound302h && ctx.bFound303h)
				{
					R_DrawParticles_VA = (decltype(R_DrawParticles_VA))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x100, [](PUCHAR Candidate) {

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

					Convert_VA_to_RVA(R_DrawParticles, DllInfo);
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

	if (!R_DrawParticles_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_DrawParticles_VA = (ULONG_PTR)Search_Pattern(R_DRAWPARTICLES_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(R_DrawParticles, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_DrawParticles_VA = (ULONG_PTR)Search_Pattern(R_DRAWPARTICLES_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(R_DrawParticles, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_DrawParticles_VA = (ULONG_PTR)Search_Pattern(R_DRAWPARTICLES_SIG_NEW, DllInfo);

			if (!R_DrawParticles_VA)
				R_DrawParticles_VA = (ULONG_PTR)Search_Pattern(R_DRAWPARTICLES_SIG_NEW2, DllInfo);

			Convert_VA_to_RVA(R_DrawParticles, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_DrawParticles_VA = (ULONG_PTR)Search_Pattern(R_DRAWPARTICLES_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(R_DrawParticles, DllInfo);
		}
	}

	if (R_DrawParticles_RVA)
	{
		gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))VA_from_RVA(R_DrawParticles, RealDllInfo);
	}

	Sig_FuncNotFound(R_DrawParticles);
}

void R_FillAddress_CL_AllocDlight(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.CL_AllocDlight)
		return;

	ULONG_PTR CL_AllocDlight_VA = 0;
	ULONG CL_AllocDlight_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//no impl
	}
	else
	{
		/*
			.text:01D18AD4 39 5E 20                                            cmp     [esi+20h], ebx
			.text:01D18AD7 74 6B                                               jz      short loc_1D18B44
			.text:01D18AD9 47                                                  inc     edi
			.text:01D18ADA 83 C6 28                                            add     esi, 28h ; '('
			.text:01D18ADD 83 FF 20                                            cmp     edi, 20h ; ' '
		*/

		const char pattern[] = "\x20\x74\x2A\x47\x83\xC6\x28";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				CL_AllocDlight_VA = (decltype(CL_AllocDlight_VA))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x80, [](PUCHAR Candidate) {

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

				if (CL_AllocDlight_VA)
				{
					Convert_VA_to_RVA(CL_AllocDlight, DllInfo);
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

	if (!CL_AllocDlight_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			CL_AllocDlight_VA = (ULONG_PTR)Search_Pattern(CL_ALLOCDLIGHT_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(CL_AllocDlight, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			CL_AllocDlight_VA = (ULONG_PTR)Search_Pattern(CL_ALLOCDLIGHT_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(CL_AllocDlight, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			CL_AllocDlight_VA = (ULONG_PTR)Search_Pattern(CL_ALLOCDLIGHT_SIG_NEW, DllInfo);

			if (!CL_AllocDlight_VA)
				CL_AllocDlight_VA = (ULONG_PTR)Search_Pattern(CL_ALLOCDLIGHT_SIG_NEW2, DllInfo);

			Convert_VA_to_RVA(CL_AllocDlight, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			CL_AllocDlight_VA = (ULONG_PTR)Search_Pattern(CL_ALLOCDLIGHT_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(CL_AllocDlight, DllInfo);
		}
	}

	if (CL_AllocDlight_RVA)
	{
		gPrivateFuncs.CL_AllocDlight = (decltype(gPrivateFuncs.CL_AllocDlight))VA_from_RVA(CL_AllocDlight, RealDllInfo);
	}

	Sig_FuncNotFound(CL_AllocDlight);

	/*
		//Global pointers that link into engine vars
		dlight_t *cl_dlights = NULL;
		int *r_dlightactive = NULL;
	*/
	ULONG_PTR cl_dlights_VA = 0;
	ULONG cl_dlights_RVA = 0;

	ULONG_PTR r_dlightactive_VA = 0;
	ULONG r_dlightactive_RVA = 0;

	typedef struct
	{
		ULONG_PTR& cl_dlights;
		ULONG_PTR& r_dlightactive;
		const mh_dll_info_t& DllInfo;
		int push28_instcount{};
	} CL_AllocDlight_SearchContext;

	CL_AllocDlight_SearchContext ctx = { cl_dlights_VA, r_dlightactive_VA, DllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)CL_AllocDlight_VA, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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
			else if (!ctx->cl_dlights &&
				ctx->push28_instcount &&
				instCount < ctx->push28_instcount + 3 &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[0].imm >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D18B06 68 E0 95 D5 02 push    offset cl_dlight

				ctx->cl_dlights = (ULONG_PTR)pinst->detail->x86.operands[0].imm;
			}
			else if (ctx->push28_instcount &&
				instCount < ctx->push28_instcount + 8 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//.text:01D18B16 8B 0D 78 9E BC 02 mov     ecx, r_dlightactive

				if (!ctx->r_dlightactive || (DWORD)pinst->detail->x86.operands[1].mem.disp > (DWORD)ctx->r_dlightactive)
					ctx->r_dlightactive = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
			}
			else if (!ctx->r_dlightactive &&
				pinst->id == X86_INS_OR &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&

				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 1)
			{//.text:01D18B16 8B 0D 78 9E BC 02 mov     ecx, r_dlightactive

				ctx->r_dlightactive = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	Convert_VA_to_RVA(cl_dlights, DllInfo);
	Convert_VA_to_RVA(r_dlightactive, DllInfo);

	if (cl_dlights_RVA)
		cl_dlights = (decltype(cl_dlights))VA_from_RVA(cl_dlights, RealDllInfo);
	if (r_dlightactive_RVA)
		r_dlightactive = (decltype(r_dlightactive))VA_from_RVA(r_dlightactive, RealDllInfo);

	Sig_VarNotFound(cl_dlights);
	Sig_VarNotFound(r_dlightactive);
}

void R_FillAddress_R_GLStudioDrawPoints(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_GLStudioDrawPoints)
		return;

	ULONG_PTR R_GLStudioDrawPoints_VA = 0;
	ULONG R_GLStudioDrawPoints_RVA = 0;

	{
		/*
			.text:01D86D01 75 12                                               jnz     short loc_1D86D15
			.text:01D86D03 68 44 0B 00 00                                      push    0B44h
			.text:01D86D08 FF 15 A4 1F 79 02                                   call    dword_2791FA4
		*/

		const char pattern[] = "\x75\x2A\x68\x44\x0B\x00\x00\xFF\x15\x2A\x2A\x2A\x2A";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				typedef struct
				{
					bool bFoundMov1{};
					bool bFound54{};
					bool bFound60{};
				}R_GLStudioDrawPoints_SearchContext;

				R_GLStudioDrawPoints_SearchContext ctx = { 0 };

				g_pMetaHookAPI->DisasmSingleInstruction(pFound + Sig_Length(pattern), [](void* inst, PUCHAR address, size_t instLen, PVOID context) {
					auto pinst = (cs_insn*)inst;
					auto ctx = (R_GLStudioDrawPoints_SearchContext*)context;

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
					auto CandidateAddress = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x1000, [](PUCHAR Candidate) {

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

					if (CandidateAddress)
					{
						g_pMetaHookAPI->DisasmRanges(CandidateAddress, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

							auto pinst = (cs_insn*)inst;
							auto ctx = (R_GLStudioDrawPoints_SearchContext*)context;

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

							}, 0, &ctx);
					}

					if (ctx.bFound54 && ctx.bFound60)
					{
						R_GLStudioDrawPoints_VA = (decltype(R_GLStudioDrawPoints_VA))CandidateAddress;
						Convert_VA_to_RVA(R_GLStudioDrawPoints, DllInfo);
					}

					if (R_GLStudioDrawPoints_RVA)
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

	if (!R_GLStudioDrawPoints_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_GLStudioDrawPoints_VA = (ULONG_PTR)Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(R_GLStudioDrawPoints, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_GLStudioDrawPoints_VA = (ULONG_PTR)Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(R_GLStudioDrawPoints, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_GLStudioDrawPoints_VA = (ULONG_PTR)Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_NEW, DllInfo);

			if (!R_GLStudioDrawPoints_VA)
				R_GLStudioDrawPoints_VA = (ULONG_PTR)Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_NEW2, DllInfo);

			Convert_VA_to_RVA(R_GLStudioDrawPoints, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_GLStudioDrawPoints_VA = (ULONG_PTR)Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(R_GLStudioDrawPoints, DllInfo);
		}
	}

	if (R_GLStudioDrawPoints_RVA)
	{
		gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))VA_from_RVA(R_GLStudioDrawPoints, RealDllInfo);
	}

	Sig_FuncNotFound(R_GLStudioDrawPoints);
}

void R_FillAddress_R_StudioLighting(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_StudioLighting)
		return;

	ULONG_PTR R_StudioLighting_VA = 0;
	ULONG R_StudioLighting_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_StudioLighting_VA = (ULONG_PTR)Search_Pattern(R_STUDIOLIGHTING_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(R_StudioLighting, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_StudioLighting_VA = (ULONG_PTR)Search_Pattern(R_STUDIOLIGHTING_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(R_StudioLighting, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_StudioLighting_VA = (ULONG_PTR)Search_Pattern(R_STUDIOLIGHTING_SIG_NEW, DllInfo);

		if (!R_StudioLighting_VA)
			R_StudioLighting_VA = (ULONG_PTR)Search_Pattern(R_STUDIOLIGHTING_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(R_StudioLighting, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_StudioLighting_VA = (ULONG_PTR)Search_Pattern(R_STUDIOLIGHTING_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_StudioLighting, DllInfo);
	}

	if (R_StudioLighting_RVA)
	{
		gPrivateFuncs.R_StudioLighting = (decltype(gPrivateFuncs.R_StudioLighting))VA_from_RVA(R_StudioLighting, RealDllInfo);
	}

	Sig_FuncNotFound(R_StudioLighting);

/*
	float* r_shadelight = NULL;
	int* r_ambientlight = NULL;
	vec3_t* r_blightvec = NULL;
	vec3_t* r_plightvec = NULL;
	int* lightgammatable = NULL;
*/
	ULONG_PTR r_shadelight_VA = 0;
	ULONG r_shadelight_RVA = 0;
	ULONG_PTR r_ambientlight_VA = 0;
	ULONG r_ambientlight_RVA = 0;
	ULONG_PTR r_blightvec_VA = 0;
	ULONG r_blightvec_RVA = 0;
	ULONG_PTR r_plightvec_VA = 0;
	ULONG r_plightvec_RVA = 0;
	ULONG_PTR lightgammatable_VA = 0;
	ULONG lightgammatable_RVA = 0;

	{
		typedef struct
		{
			ULONG_PTR& r_ambientlight;
			ULONG_PTR& r_shadelight;
			ULONG_PTR& r_blightvec;
			ULONG_PTR& r_plightvec;
			ULONG_PTR& lightgammatable;
			const mh_dll_info_t& DllInfo;
			ULONG_PTR base{};
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

		R_StudioLighting_SearchContext ctx = { r_ambientlight_VA, r_shadelight_VA, r_blightvec_VA, r_plightvec_VA, lightgammatable_VA, DllInfo };
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

				if (ctx->r_ambientlight && ctx->r_shadelight && ctx->r_blightvec && ctx->r_plightvec && ctx->lightgammatable)
					return TRUE;

				if (ctx->code.size() > ctx->max_insts)
					return TRUE;

				if (ctx->code.find(address) != ctx->code.end())
					return TRUE;

				ctx->code.emplace(address);

				if (!ctx->r_ambientlight &&
					pinst->id == X86_INS_FILD &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//.text:01D83B54 DB 05 E8 95 75 02                                   fild    r_ambientlight
					ctx->r_ambientlight = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->ambient_instcount = instCount;
				}

				if (!ctx->r_ambientlight &&
					pinst->id == X86_INS_MOVD &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//   movd    xmm3, r_ambientlight
					ctx->r_ambientlight = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
					ctx->ambient_instcount = instCount;
				}

				if (!ctx->r_shadelight &&
					ctx->ambient_instcount &&
					instCount < ctx->ambient_instcount + 10 &&
					pinst->id == X86_INS_FLD &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//.text:01D8A986 D9 05 60 61 52 08                                   fld     r_shadelight
					ctx->r_shadelight = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}

				if (!ctx->r_shadelight &&
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
					ctx->r_shadelight = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
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

				if (!ctx->r_blightvec &&
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

				if (!ctx->r_blightvec &&
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

				if (!ctx->r_blightvec &&
					ctx->lightvec_start == 2 &&
					ctx->blightvec_instcount &&
					instCount < ctx->blightvec_instcount + 2 &&
					pinst->id == X86_INS_FMUL &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0)
				{//.text:01D83BE2 D8 49 08                                            fmul    dword ptr [ecx+8]
					ctx->r_blightvec = (ULONG_PTR)(ctx->blightvec_candidateVA - pinst->detail->x86.operands[0].mem.disp);
				}

				if (!ctx->r_blightvec &&
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
					ctx->r_blightvec = (ULONG_PTR)(ctx->blightvec_candidateVA - pinst->detail->x86.operands[1].mem.disp);
				}

				if (!ctx->r_plightvec &&
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

				if (!ctx->r_plightvec &&
					ctx->lightvec_start == 1 &&
					ctx->plightvec_instcount &&
					instCount < ctx->plightvec_instcount + 15 &&
					pinst->id == X86_INS_FMUL &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0)
				{//.text:01D83BE2 D8 49 08                                            fmul    dword ptr [ecx+8]
					ctx->r_plightvec = (ULONG_PTR)(ctx->plightvec_candidateVA - pinst->detail->x86.operands[0].mem.disp);
				}

				if (!ctx->r_plightvec &&
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

				if (!ctx->r_plightvec &&
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
					ctx->r_plightvec = (ULONG_PTR)(pinst->detail->x86.operands[1].mem.disp - 8);
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
					ctx->lightgammatable = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
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
					ctx->lightgammatable = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
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

		Convert_VA_to_RVA(r_ambientlight, DllInfo);
		Convert_VA_to_RVA(r_shadelight, DllInfo);
		Convert_VA_to_RVA(r_blightvec, DllInfo);
		Convert_VA_to_RVA(r_plightvec, DllInfo);
		Convert_VA_to_RVA(lightgammatable, DllInfo);
	}

	if (r_ambientlight_RVA)
		r_ambientlight = (decltype(r_ambientlight))VA_from_RVA(r_ambientlight, RealDllInfo);
	if (r_shadelight_RVA)
		r_shadelight = (decltype(r_shadelight))VA_from_RVA(r_shadelight, RealDllInfo);
	if (r_blightvec_RVA)
		r_blightvec = (decltype(r_blightvec))VA_from_RVA(r_blightvec, RealDllInfo);
	if (r_plightvec_RVA)
		r_plightvec = (decltype(r_plightvec))VA_from_RVA(r_plightvec, RealDllInfo);
	if (lightgammatable_RVA)
		lightgammatable = (decltype(lightgammatable))VA_from_RVA(lightgammatable, RealDllInfo);

	Sig_VarNotFound(r_ambientlight);
	Sig_VarNotFound(r_shadelight);
	Sig_VarNotFound(r_blightvec);
	Sig_VarNotFound(r_plightvec);
	Sig_VarNotFound(lightgammatable);
}

void R_FillAddress_R_StudioChrome(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_StudioChrome)
		return;

	ULONG_PTR R_StudioChrome_VA = 0;
	ULONG R_StudioChrome_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_StudioChrome_VA = (ULONG_PTR)Search_Pattern(R_STUDIOCHROME_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(R_StudioChrome, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_StudioChrome_VA = (ULONG_PTR)Search_Pattern(R_STUDIOCHROME_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(R_StudioChrome, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_StudioChrome_VA = (ULONG_PTR)Search_Pattern(R_STUDIOCHROME_SIG_NEW, DllInfo);

		if (!R_StudioChrome_VA)
			R_StudioChrome_VA = (ULONG_PTR)Search_Pattern(R_STUDIOCHROME_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(R_StudioChrome, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_StudioChrome_VA = (ULONG_PTR)Search_Pattern(R_STUDIOCHROME_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_StudioChrome, DllInfo);
	}

	if (R_StudioChrome_RVA)
	{
		gPrivateFuncs.R_StudioChrome = (decltype(gPrivateFuncs.R_StudioChrome))VA_from_RVA(R_StudioChrome, RealDllInfo);
	}

	Sig_FuncNotFound(R_StudioChrome);
}

void R_FillAddress_R_LightLambert(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_LightLambert)
		return;

	ULONG_PTR R_LightLambert_VA = 0;
	ULONG R_LightLambert_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_LightLambert_VA = (ULONG_PTR)Search_Pattern(R_LIGHTLAMBERT_SIG_SVENGINE, DllInfo);
		Convert_VA_to_RVA(R_LightLambert, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_LightLambert_VA = (ULONG_PTR)Search_Pattern(R_LIGHTLAMBERT_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(R_LightLambert, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_LightLambert_VA = (ULONG_PTR)Search_Pattern(R_LIGHTLAMBERT_SIG_NEW, DllInfo);

		if (!R_LightLambert_VA)
			R_LightLambert_VA = (ULONG_PTR)Search_Pattern(R_LIGHTLAMBERT_SIG_NEW2, DllInfo);

		Convert_VA_to_RVA(R_LightLambert, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_LightLambert_VA = (ULONG_PTR)Search_Pattern(R_LIGHTLAMBERT_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_LightLambert, DllInfo);
	}

	if (R_LightLambert_RVA)
	{
		gPrivateFuncs.R_LightLambert = (decltype(gPrivateFuncs.R_LightLambert))VA_from_RVA(R_LightLambert, RealDllInfo);
	}

	Sig_FuncNotFound(R_LightLambert);
}

void R_FillAddress_R_StudioSetupSkin(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_StudioSetupSkin)
		return;

	ULONG_PTR R_StudioSetupSkin_VA = 0;
	ULONG R_StudioSetupSkin_RVA = 0;

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
				R_StudioSetupSkin_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(DM_Base_PushString, 0x300, [](PUCHAR Candidate) {

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
				Convert_VA_to_RVA(R_StudioSetupSkin, DllInfo);
			}
		}
	}

	if (!R_StudioSetupSkin_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_StudioSetupSkin_VA = (ULONG_PTR)Search_Pattern(R_STUDIOSETUPSKIN_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(R_StudioSetupSkin, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_StudioSetupSkin_VA = (ULONG_PTR)Search_Pattern(R_STUDIOSETUPSKIN_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(R_StudioSetupSkin, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_StudioSetupSkin_VA = (ULONG_PTR)Search_Pattern(R_STUDIOSETUPSKIN_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(R_StudioSetupSkin, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_StudioSetupSkin_VA = (ULONG_PTR)Search_Pattern(R_STUDIOSETUPSKIN_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(R_StudioSetupSkin, DllInfo);
		}
	}

	if (R_StudioSetupSkin_RVA)
	{
		gPrivateFuncs.R_StudioSetupSkin = (decltype(gPrivateFuncs.R_StudioSetupSkin))VA_from_RVA(R_StudioSetupSkin, RealDllInfo);
	}

	Sig_FuncNotFound(R_StudioSetupSkin);
}

void R_FillAddress_Cache_Alloc(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Cache_Alloc)
		return;

	ULONG_PTR Cache_Alloc_VA = 0;
	ULONG Cache_Alloc_RVA = 0;

	if (1)
	{
		const char sigs[] = "Cache_Alloc: already allocated";
		auto Cache_Alloc_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Cache_Alloc_String)
			Cache_Alloc_String = Search_Pattern_Rdata(sigs, DllInfo);
		Sig_VarNotFound(Cache_Alloc_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04";
		*(DWORD*)(pattern + 1) = (DWORD)Cache_Alloc_String;
		auto Cache_Alloc_Call = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(Cache_Alloc_Call);

		Cache_Alloc_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Cache_Alloc_Call, 0x80, [](PUCHAR Candidate) {

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
		Convert_VA_to_RVA(Cache_Alloc, DllInfo);
	}

	if (Cache_Alloc_RVA)
	{
		gPrivateFuncs.Cache_Alloc = (decltype(gPrivateFuncs.Cache_Alloc))VA_from_RVA(Cache_Alloc, RealDllInfo);
	}

	Sig_FuncNotFound(Cache_Alloc);
}

void R_FillAddress_Draw_MiptexTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_MiptexTexture)
		return;

	ULONG_PTR Draw_MiptexTexture_VA = 0;
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
				Draw_MiptexTexture_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Draw_MiptexTexture_Call, 0x80, [](PUCHAR Candidate) {

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
				Convert_VA_to_RVA(Draw_MiptexTexture, DllInfo);
			}
		}
	}

	if (!Draw_MiptexTexture_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			Draw_MiptexTexture_VA = (ULONG_PTR)Search_Pattern(DRAW_MIPTEXTEXTURE_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(Draw_MiptexTexture, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			Draw_MiptexTexture_VA = (ULONG_PTR)Search_Pattern(DRAW_MIPTEXTEXTURE_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(Draw_MiptexTexture, DllInfo);
		}
		else
		{
			//GoldSrc_Blob and GoldSrc_New use the same signature
			Draw_MiptexTexture_VA = (ULONG_PTR)Search_Pattern(DRAW_MIPTEXTEXTURE_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(Draw_MiptexTexture, DllInfo);
		}
	}

	if (Draw_MiptexTexture_RVA)
	{
		gPrivateFuncs.Draw_MiptexTexture = (decltype(gPrivateFuncs.Draw_MiptexTexture))VA_from_RVA(Draw_MiptexTexture, RealDllInfo);
	}

	Sig_FuncNotFound(Draw_MiptexTexture);
}

void R_FillAddress_Draw_DecalTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Draw_DecalTexture)
		return;

	ULONG_PTR Draw_DecalTexture_VA = 0;
	ULONG Draw_DecalTexture_RVA = 0;

	{
		const char sigs[] = "Failed to load custom decal for player";
		auto Draw_DecalTexture_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Draw_DecalTexture_String)
			Draw_DecalTexture_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (Draw_DecalTexture_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C";
			*(DWORD*)(pattern + 1) = (DWORD)Draw_DecalTexture_String;
			auto Draw_DecalTexture_Call = Search_Pattern(pattern, DllInfo);
			if (Draw_DecalTexture_Call)
			{
				Draw_DecalTexture_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Draw_DecalTexture_Call, 0x300, [](PUCHAR Candidate) {

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
				Convert_VA_to_RVA(Draw_DecalTexture, DllInfo);
			}
		}
	}

	if (!Draw_DecalTexture_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			Draw_DecalTexture_VA = (ULONG_PTR)Search_Pattern(DRAW_DECALTEXTURE_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(Draw_DecalTexture, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			Draw_DecalTexture_VA = (ULONG_PTR)Search_Pattern(DRAW_DECALTEXTURE_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(Draw_DecalTexture, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			Draw_DecalTexture_VA = (ULONG_PTR)Search_Pattern(DRAW_DECALTEXTURE_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(Draw_DecalTexture, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			Draw_DecalTexture_VA = (ULONG_PTR)Search_Pattern(DRAW_DECALTEXTURE_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(Draw_DecalTexture, DllInfo);
		}
	}

	if (Draw_DecalTexture_RVA)
	{
		gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))VA_from_RVA(Draw_DecalTexture, RealDllInfo);
	}

	Sig_FuncNotFound(Draw_DecalTexture);
}

void R_FillAddress_R_DrawSpriteModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawSpriteModel)
		return;

	ULONG_PTR R_DrawSpriteModel_VA = 0;
	ULONG R_DrawSpriteModel_RVA = 0;

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
				R_DrawSpriteModel_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_DrawSpriteModel_Call, 0x300, [](PUCHAR Candidate) {

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
				Convert_VA_to_RVA(R_DrawSpriteModel, DllInfo);
			}
		}
	}

	if (!R_DrawSpriteModel_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_DrawSpriteModel_VA = (ULONG_PTR)Search_Pattern(R_DRAWSRPITEMODEL_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(R_DrawSpriteModel, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_DrawSpriteModel_VA = (ULONG_PTR)Search_Pattern(R_DRAWSRPITEMODEL_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(R_DrawSpriteModel, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_DrawSpriteModel_VA = (ULONG_PTR)Search_Pattern(R_DRAWSRPITEMODEL_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(R_DrawSpriteModel, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_DrawSpriteModel_VA = (ULONG_PTR)Search_Pattern(R_DRAWSRPITEMODEL_SIG_BLOB, DllInfo);
			Convert_VA_to_RVA(R_DrawSpriteModel, DllInfo);
		}
	}

	if (R_DrawSpriteModel_RVA)
	{
		gPrivateFuncs.R_DrawSpriteModel = (decltype(gPrivateFuncs.R_DrawSpriteModel))VA_from_RVA(R_DrawSpriteModel, RealDllInfo);
	}

	Sig_FuncNotFound(R_DrawSpriteModel);
}

void R_FillAddress_R_LightStrength(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_LightStrength)
		return;

	ULONG_PTR R_LightStrength_VA = 0;
	ULONG R_LightStrength_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_LightStrength_VA = (ULONG_PTR)Search_Pattern(R_LIGHTSTRENGTH_SIG_SVENGINE, DllInfo);
		if (!R_LightStrength_VA)
			R_LightStrength_VA = (ULONG_PTR)Search_Pattern(R_LIGHTSTRENGTH_SIG_SVENGINE_10152, DllInfo);
		Convert_VA_to_RVA(R_LightStrength, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		//Inlined
		gPrivateFuncs.R_LightStrength_inlined = true;
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_LightStrength_VA = (ULONG_PTR)Search_Pattern(R_LIGHTSTRENGTH_SIG_NEW, DllInfo);
		if (!R_LightStrength_VA)
			R_LightStrength_VA = (ULONG_PTR)Search_Pattern(R_LIGHTSTRENGTH_SIG_NEW2, DllInfo);
		Convert_VA_to_RVA(R_LightStrength, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_LightStrength_VA = (ULONG_PTR)Search_Pattern(R_LIGHTSTRENGTH_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_LightStrength, DllInfo);
	}

	if (gPrivateFuncs.R_LightStrength_inlined)
		return;

	if (R_LightStrength_RVA)
	{
		gPrivateFuncs.R_LightStrength = (decltype(gPrivateFuncs.R_LightStrength))VA_from_RVA(R_LightStrength, RealDllInfo);
	}

	Sig_FuncNotFound(R_LightStrength);
}

void R_FillAddress_R_RotateForEntity(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_RotateForEntity)
		return;

	ULONG_PTR R_RotateForEntity_VA = 0;
	ULONG R_RotateForEntity_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//no impl
	}
	else
	{
		auto addr = Search_Pattern(R_ROTATEFORENTITY_GOLDSRC, DllInfo);
		if (addr)
		{
			R_RotateForEntity_VA = (ULONG_PTR)GetCallAddress((PUCHAR)addr + Sig_Length(R_ROTATEFORENTITY_GOLDSRC) - 1);
			Convert_VA_to_RVA(R_RotateForEntity, DllInfo);
		}
	}

	if (!R_RotateForEntity_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_RotateForEntity_VA = (ULONG_PTR)Search_Pattern(R_ROTATEFORENTITY_SVENGINE, DllInfo);
			Convert_VA_to_RVA(R_RotateForEntity, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_RotateForEntity_VA = (ULONG_PTR)Search_Pattern(R_ROTATEFORENTITY_HL25, DllInfo);
			Convert_VA_to_RVA(R_RotateForEntity, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_RotateForEntity_VA = (ULONG_PTR)Search_Pattern(R_ROTATEFORENTITY_NEW, DllInfo);
			Convert_VA_to_RVA(R_RotateForEntity, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_RotateForEntity_VA = (ULONG_PTR)Search_Pattern(R_ROTATEFORENTITY_NEW, DllInfo);
			Convert_VA_to_RVA(R_RotateForEntity, DllInfo);
		}
	}

	if (R_RotateForEntity_RVA)
	{
		gPrivateFuncs.R_RotateForEntity = (decltype(gPrivateFuncs.R_RotateForEntity))VA_from_RVA(R_RotateForEntity, RealDllInfo);
	}

	Sig_FuncNotFound(R_RotateForEntity);
}

void R_FillAddress_R_GlowBlend(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_GlowBlend)
		return;

	ULONG_PTR R_GlowBlend_VA = 0;
	ULONG R_GlowBlend_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//Inlined
		gPrivateFuncs.R_GlowBlend_inlined = true;
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		//Inlined
		gPrivateFuncs.R_GlowBlend_inlined = true;
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_GlowBlend_VA = (ULONG_PTR)Search_Pattern(R_GLOW_BLEND_SIG_NEW, DllInfo);
		if (!R_GlowBlend_VA)
			R_GlowBlend_VA = (ULONG_PTR)Search_Pattern(R_GLOW_BLEND_SIG_NEW2, DllInfo);
		Convert_VA_to_RVA(R_GlowBlend, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_GlowBlend_VA = (ULONG_PTR)Search_Pattern(R_GLOW_BLEND_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_GlowBlend, DllInfo);
	}

	if (R_GlowBlend_RVA)
	{
		gPrivateFuncs.R_GlowBlend = (decltype(gPrivateFuncs.R_GlowBlend))VA_from_RVA(R_GlowBlend, RealDllInfo);
	}

	if (!gPrivateFuncs.R_GlowBlend_inlined)
	{
		Sig_FuncNotFound(R_GlowBlend);
	}
}

void R_FillAddress_SCR_BeginLoadingPlaque(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.SCR_BeginLoadingPlaque)
		return;

	ULONG_PTR SCR_BeginLoadingPlaque_VA = 0;
	ULONG SCR_BeginLoadingPlaque_RVA = 0;

	//All engine use the same signature
	SCR_BeginLoadingPlaque_VA = (ULONG_PTR)Search_Pattern(SCR_BEGIN_LOADING_PLAQUE, DllInfo);
	Convert_VA_to_RVA(SCR_BeginLoadingPlaque, DllInfo);

	if (SCR_BeginLoadingPlaque_RVA)
	{
		gPrivateFuncs.SCR_BeginLoadingPlaque = (decltype(gPrivateFuncs.SCR_BeginLoadingPlaque))VA_from_RVA(SCR_BeginLoadingPlaque, RealDllInfo);
	}

	Sig_FuncNotFound(SCR_BeginLoadingPlaque);
}

void R_FillAddress_Host_IsSinglePlayerGame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Host_IsSinglePlayerGame)
		return;

	ULONG_PTR Host_IsSinglePlayerGame_VA = 0;
	ULONG Host_IsSinglePlayerGame_RVA = 0;

	if (1)
	{
		const char sigs[] = "setpause;";
		auto setpause_String = Search_Pattern_Data(sigs, DllInfo);
		if (!setpause_String)
			setpause_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (setpause_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)setpause_String;
			auto setpause_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
			if (setpause_PushString)
			{
				auto setpause_Function = (PUCHAR)g_pMetaHookAPI->ReverseSearchFunctionBegin(setpause_PushString, 0x50);
				if (setpause_Function)
				{
					typedef struct
					{
						ULONG_PTR& Host_IsSinglePlayerGame;
					}Host_IsSinglePlayerGame_SearchContext;

					Host_IsSinglePlayerGame_SearchContext ctx = { Host_IsSinglePlayerGame_VA };

					g_pMetaHookAPI->DisasmRanges(setpause_Function, setpause_PushString - setpause_Function, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
						auto pinst = (cs_insn*)inst;
						auto ctx = (Host_IsSinglePlayerGame_SearchContext*)context;

						if (address[0] == 0xE8 && instLen == 5 &&
							address[5] == 0x85 && address[6] == 0xC0)
						{
							ctx->Host_IsSinglePlayerGame = (ULONG_PTR)pinst->detail->x86.operands[0].imm;
						}

						if (address[0] == 0xCC)
							return TRUE;

						if (pinst->id == X86_INS_RET)
							return TRUE;

						return FALSE;
						}, 0, NULL);

					Convert_VA_to_RVA(Host_IsSinglePlayerGame, DllInfo);
				}
			}
		}
	}

	if (!Host_IsSinglePlayerGame_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			Host_IsSinglePlayerGame_VA = (ULONG_PTR)Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_SVENGINE, DllInfo);
			Convert_VA_to_RVA(Host_IsSinglePlayerGame, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			Host_IsSinglePlayerGame_VA = (ULONG_PTR)Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_HL25, DllInfo);
			Convert_VA_to_RVA(Host_IsSinglePlayerGame, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			Host_IsSinglePlayerGame_VA = (ULONG_PTR)Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_NEW, DllInfo);
			if (!Host_IsSinglePlayerGame_VA)
				Host_IsSinglePlayerGame_VA = (ULONG_PTR)Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_NEW2, DllInfo);
			if (!Host_IsSinglePlayerGame_VA)
				Host_IsSinglePlayerGame_VA = (ULONG_PTR)Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_NEW3, DllInfo);
			Convert_VA_to_RVA(Host_IsSinglePlayerGame, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			Host_IsSinglePlayerGame_VA = (ULONG_PTR)Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_BLOB, DllInfo);
			Convert_VA_to_RVA(Host_IsSinglePlayerGame, DllInfo);
		}
	}

	if (Host_IsSinglePlayerGame_RVA)
	{
		gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))VA_from_RVA(Host_IsSinglePlayerGame, RealDllInfo);
	}

	Sig_FuncNotFound(Host_IsSinglePlayerGame);
}

void R_FillAddress_Mod_UnloadSpriteTextures(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Mod_UnloadSpriteTextures)
		return;

	ULONG_PTR Mod_UnloadSpriteTextures_VA = 0;
	ULONG Mod_UnloadSpriteTextures_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Mod_UnloadSpriteTextures_VA = (ULONG_PTR)Search_Pattern(MOD_UNLOADSPRITETEXTURES_SVENGINE, DllInfo);
		Convert_VA_to_RVA(Mod_UnloadSpriteTextures, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		Mod_UnloadSpriteTextures_VA = (ULONG_PTR)Search_Pattern(MOD_UNLOADSPRITETEXTURES_HL25, DllInfo);
		Convert_VA_to_RVA(Mod_UnloadSpriteTextures, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		Mod_UnloadSpriteTextures_VA = (ULONG_PTR)Search_Pattern(MOD_UNLOADSPRITETEXTURES_NEW, DllInfo);
		if (!Mod_UnloadSpriteTextures_VA)
			Mod_UnloadSpriteTextures_VA = (ULONG_PTR)Search_Pattern(MOD_UNLOADSPRITETEXTURES_NEW2, DllInfo);
		Convert_VA_to_RVA(Mod_UnloadSpriteTextures, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		Mod_UnloadSpriteTextures_VA = (ULONG_PTR)Search_Pattern(MOD_UNLOADSPRITETEXTURES_BLOB, DllInfo);
		Convert_VA_to_RVA(Mod_UnloadSpriteTextures, DllInfo);
	}

	if (Mod_UnloadSpriteTextures_RVA)
	{
		gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))VA_from_RVA(Mod_UnloadSpriteTextures, RealDllInfo);
	}

	Sig_FuncNotFound(Mod_UnloadSpriteTextures);
}

void R_FillAddress_Mod_LoadSpriteModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Mod_LoadSpriteModel)
		return;

	ULONG_PTR Mod_LoadSpriteModel_VA = 0;
	ULONG Mod_LoadSpriteModel_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "Sprite \"%s\" has wrong version number";
		auto Sprite_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Sprite_String)
			Sprite_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (Sprite_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)Sprite_String;
			auto Sprite_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
			if (Sprite_PushString)
			{
				auto Sprite_Function = (PUCHAR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Sprite_PushString, 0x100, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC &&
						Candidate[2] == 0x08)
						return TRUE;

					if (Candidate[0] == 0xA1 &&
						Candidate[5] == 0x83 &&
						Candidate[6] == 0xEC &&
						Candidate[7] == 0x08)
						return TRUE;

					return FALSE;
					});

				if (Sprite_Function)
				{
					Mod_LoadSpriteModel_VA = (ULONG_PTR)Sprite_Function;
					Convert_VA_to_RVA(Mod_LoadSpriteModel, DllInfo);
				}
			}
		}
	}
	else
	{
		const char sigs[] = "Mod_LoadSpriteModel: Invalid # of frame";
		auto Sprite_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Sprite_String)
			Sprite_String = Search_Pattern_Rdata(sigs, DllInfo);
		if (Sprite_String)
		{
			char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
			*(DWORD*)(pattern + 1) = (DWORD)Sprite_String;
			auto Sprite_PushString = (PUCHAR)Search_Pattern(pattern, DllInfo);
			if (Sprite_PushString)
			{
				auto Sprite_Function = (PUCHAR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Sprite_PushString, 0x300, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					if (Candidate[0] == 0x83 &&
						Candidate[1] == 0xEC &&
						Candidate[2] == 0x08)
						return TRUE;

					if (Candidate[-1] == 0x90 &&
						Candidate[0] >= 0x50 &&
						Candidate[0] <= 0x57 &&
						Candidate[1] >= 0x50 &&
						Candidate[1] <= 0x57 &&
						Candidate[2] >= 0x50 &&
						Candidate[2] <= 0x57 &&
						Candidate[3] >= 0x50 &&
						Candidate[3] <= 0x57)
						return TRUE;

					return FALSE;
					});

				if (Sprite_Function)
				{
					Mod_LoadSpriteModel_VA = (ULONG_PTR)Sprite_Function;
					Convert_VA_to_RVA(Mod_LoadSpriteModel, DllInfo);
				}
			}
		}
	}

	if (!Mod_LoadSpriteModel_RVA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			Mod_LoadSpriteModel_VA = (ULONG_PTR)Search_Pattern(MOD_LOADSPRITEMODEL_SVENGINE, DllInfo);
			Convert_VA_to_RVA(Mod_LoadSpriteModel, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			Mod_LoadSpriteModel_VA = (ULONG_PTR)Search_Pattern(MOD_LOADSPRITEMODEL_HL25, DllInfo);
			Convert_VA_to_RVA(Mod_LoadSpriteModel, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			Mod_LoadSpriteModel_VA = (ULONG_PTR)Search_Pattern(MOD_LOADSPRITEMODEL_NEW, DllInfo);
			if (!Mod_LoadSpriteModel_VA)
				Mod_LoadSpriteModel_VA = (ULONG_PTR)Search_Pattern(MOD_LOADSPRITEMODEL_NEW2, DllInfo);
			Convert_VA_to_RVA(Mod_LoadSpriteModel, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			Mod_LoadSpriteModel_VA = (ULONG_PTR)Search_Pattern(MOD_LOADSPRITEMODEL_BLOB, DllInfo);
			Convert_VA_to_RVA(Mod_LoadSpriteModel, DllInfo);
		}
	}

	if (Mod_LoadSpriteModel_RVA)
	{
		gPrivateFuncs.Mod_LoadSpriteModel = (decltype(gPrivateFuncs.Mod_LoadSpriteModel))VA_from_RVA(Mod_LoadSpriteModel, RealDllInfo);
	}

	Sig_FuncNotFound(Mod_LoadSpriteModel);
}

void R_FillAddress_Mod_LoadSpriteFrame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Mod_LoadSpriteFrame)
		return;

	ULONG_PTR Mod_LoadSpriteFrame_VA = 0;
	ULONG Mod_LoadSpriteFrame_RVA = 0;

	if (1)
	{
		ULONG_PTR Mod_LoadSpriteModel_VA = (ULONG_PTR)gPrivateFuncs.Mod_LoadSpriteModel;
		ULONG Mod_LoadSpriteModel_RVA = 0;

		Convert_VA_to_RVA(Mod_LoadSpriteModel, RealDllInfo);
		Convert_RVA_to_VA(Mod_LoadSpriteModel, DllInfo);

		typedef struct
		{
			ULONG_PTR& Mod_LoadSpriteFrame;
			bool bFoundPush300h{};
			PVOID callTarget{};
		}Mod_LoadSpriteFrame_SearchContext;

		Mod_LoadSpriteFrame_SearchContext ctx = { Mod_LoadSpriteFrame_VA };

		g_pMetaHookAPI->DisasmRanges((void*)Mod_LoadSpriteModel_VA, 0x240, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (Mod_LoadSpriteFrame_SearchContext*)context;

			if (address[0] == 0xE8)
			{
				PVOID target = (decltype(target))pinst->detail->x86.operands[0].imm;

				ctx->callTarget = target;

				g_pMetaHookAPI->DisasmRanges(target, 0x60, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (Mod_LoadSpriteFrame_SearchContext*)context;

					if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_IMM &&
						pinst->detail->x86.operands[0].imm == 0x300)
					{
						ctx->bFoundPush300h = true;
						return TRUE;
					}

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					}, 0, ctx);

				if (ctx->bFoundPush300h)
				{
					ctx->Mod_LoadSpriteFrame = (ULONG_PTR)ctx->callTarget;
					return TRUE;
				}
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		if (Mod_LoadSpriteFrame_VA)
		{
			Convert_VA_to_RVA(Mod_LoadSpriteFrame, DllInfo);
		}
	}

	if (Mod_LoadSpriteFrame_RVA)
	{
		gPrivateFuncs.Mod_LoadSpriteFrame = (decltype(gPrivateFuncs.Mod_LoadSpriteFrame))VA_from_RVA(Mod_LoadSpriteFrame, RealDllInfo);
	}

	Sig_FuncNotFound(Mod_LoadSpriteFrame);

	ULONG_PTR gSpriteMipMap_VA = 0;
	ULONG gSpriteMipMap_RVA = 0;

	{
		typedef struct
		{
			ULONG_PTR& gSpriteMipMap;
			const mh_dll_info_t& DllInfo;
			int MovRegMem_InstCount{};
			int MovRegMem_Reg{};
			PVOID MovRegMem_CandidateMem{};
		}Mod_LoadSpriteFrame_SearchContext;

		Mod_LoadSpriteFrame_SearchContext ctx = { gSpriteMipMap_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)Mod_LoadSpriteFrame_VA, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (Mod_LoadSpriteFrame_SearchContext*)context;

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{
				ctx->gSpriteMipMap = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				return TRUE;
			}

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
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
				ctx->gSpriteMipMap = (ULONG_PTR)ctx->MovRegMem_CandidateMem;
				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		if (gSpriteMipMap_VA)
		{
			Convert_VA_to_RVA(gSpriteMipMap, DllInfo);
		}
	}

	gSpriteMipMap = (decltype(gSpriteMipMap))VA_from_RVA(gSpriteMipMap, RealDllInfo);

	Sig_VarNotFound(gSpriteMipMap);
}

void R_FillAddress_R_AddTEntity(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

void R_FillAddress_Hunk_AllocName(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Hunk_AllocName)
		return;

	ULONG_PTR Hunk_AllocName_VA = 0;
	ULONG Hunk_AllocName_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "Hunk_Alloc: bad size: %i";
		auto Hunk_Alloc_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Hunk_Alloc_String)
			Hunk_Alloc_String = Search_Pattern_Rdata(sigs, DllInfo);
		Sig_VarNotFound(Hunk_Alloc_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x0F\xAE\xE8\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
		*(DWORD*)(pattern + 1) = (DWORD)Hunk_Alloc_String;
		auto Hunk_Alloc_Call = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(Hunk_Alloc_Call);

		Hunk_AllocName_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Hunk_Alloc_Call, 0x50, [](PUCHAR Candidate) {
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
		Convert_VA_to_RVA(Hunk_AllocName, DllInfo);
	}
	else
	{
		const char sigs[] = "Hunk_Alloc: bad size: %i";
		auto Hunk_Alloc_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Hunk_Alloc_String)
			Hunk_Alloc_String = Search_Pattern_Rdata(sigs, DllInfo);
		Sig_VarNotFound(Hunk_Alloc_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
		*(DWORD*)(pattern + 1) = (DWORD)Hunk_Alloc_String;
		auto Hunk_Alloc_Call = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(Hunk_Alloc_Call);

		Hunk_AllocName_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Hunk_Alloc_Call, 0x50, [](PUCHAR Candidate) {

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
		Convert_VA_to_RVA(Hunk_AllocName, DllInfo);
	}

	if (Hunk_AllocName_RVA)
	{
		gPrivateFuncs.Hunk_AllocName = (decltype(gPrivateFuncs.Hunk_AllocName))VA_from_RVA(Hunk_AllocName, RealDllInfo);
	}

	Sig_FuncNotFound(Hunk_AllocName);
}

void R_FillAddress_GL_EndRenderingVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

	ULONG_PTR	s_fXMouseAspectAdjustment_VA = 0;
	ULONG		s_fXMouseAspectAdjustment_RVA = 0;

	ULONG_PTR	s_fYMouseAspectAdjustment_VA = 0;
	ULONG		s_fYMouseAspectAdjustment_RVA = 0;

	ULONG_PTR	gl_msaa_fbo_VA = 0;
	ULONG		gl_msaa_fbo_RVA = 0;

	ULONG_PTR	gl_backbuffer_fbo_VA = 0;
	ULONG		gl_backbuffer_fbo_RVA = 0;

	typedef struct
	{
		ULONG_PTR& s_fXMouseAspectAdjustment;
		ULONG_PTR& s_fYMouseAspectAdjustment;
		ULONG_PTR& gl_msaa_fbo;
		ULONG_PTR& gl_backbuffer_fbo;
		int type{};
		int zero_register{};
		int load_zero_instcount{};
	}GL_EndRendering_SearchContext;

	GL_EndRendering_SearchContext ctx = { s_fXMouseAspectAdjustment_VA, s_fYMouseAspectAdjustment_VA , gl_msaa_fbo_VA , gl_backbuffer_fbo_VA };

	g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.GL_EndRendering, 0x350, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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

				if (!ctx->gl_backbuffer_fbo && ctx->type == 0)
				{
					ctx->gl_backbuffer_fbo = (ULONG_PTR)imm;
					ctx->type = 1;
				}
				else if (!ctx->gl_msaa_fbo && ctx->type == 1)
				{
					ctx->gl_msaa_fbo = (ULONG_PTR)imm;
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

				if (!ctx->gl_backbuffer_fbo && ctx->type == 0)
				{
					ctx->gl_backbuffer_fbo = (ULONG_PTR)imm;
					ctx->type = 2;
				}
				else if (!ctx->gl_msaa_fbo && ctx->type == 2)
				{
					ctx->gl_msaa_fbo = (ULONG_PTR)imm;
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
				if (!ctx->s_fYMouseAspectAdjustment)
				{
					ctx->s_fYMouseAspectAdjustment = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}
				else if (!ctx->s_fXMouseAspectAdjustment)
				{
					ctx->s_fXMouseAspectAdjustment = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0x3F800000)
			{//.text:01D4D4C0 A3 F4 78 E4 01 mov     videowindowaspect_0, eax

				if (!ctx->s_fYMouseAspectAdjustment)
					ctx->s_fYMouseAspectAdjustment = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				else if (!ctx->s_fXMouseAspectAdjustment)
					ctx->s_fXMouseAspectAdjustment = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			}
			else if ((pinst->id == X86_INS_FST || pinst->id == X86_INS_FSTP) &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				instCount < ctx->load_zero_instcount + 5)
			{//.D9 15 E0 85 ED 01 fst     videowindowaspect

				if (!ctx->s_fYMouseAspectAdjustment)
					ctx->s_fYMouseAspectAdjustment = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				else if (!ctx->s_fXMouseAspectAdjustment)
					ctx->s_fXMouseAspectAdjustment = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
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

			if (ctx->gl_backbuffer_fbo && ctx->gl_msaa_fbo && ctx->s_fXMouseAspectAdjustment && ctx->s_fYMouseAspectAdjustment)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	Convert_VA_to_RVA(gl_backbuffer_fbo, DllInfo);
	Convert_VA_to_RVA(gl_msaa_fbo, DllInfo);
	Convert_VA_to_RVA(s_fXMouseAspectAdjustment, DllInfo);
	Convert_VA_to_RVA(s_fYMouseAspectAdjustment, DllInfo);

	if (gl_backbuffer_fbo_RVA)
		gl_backbuffer_fbo = (decltype(gl_backbuffer_fbo))VA_from_RVA(gl_backbuffer_fbo, RealDllInfo);
	if (gl_msaa_fbo_RVA)
		gl_msaa_fbo = (decltype(gl_msaa_fbo))VA_from_RVA(gl_msaa_fbo, RealDllInfo);
	if (s_fXMouseAspectAdjustment_RVA)
		s_fXMouseAspectAdjustment = (decltype(s_fXMouseAspectAdjustment))VA_from_RVA(s_fXMouseAspectAdjustment, RealDllInfo);
	if (s_fYMouseAspectAdjustment_RVA)
		s_fYMouseAspectAdjustment = (decltype(s_fYMouseAspectAdjustment))VA_from_RVA(s_fYMouseAspectAdjustment, RealDllInfo);

	Sig_VarNotFound(gl_backbuffer_fbo);
	Sig_VarNotFound(gl_msaa_fbo);
	Sig_VarNotFound(s_fXMouseAspectAdjustment);
	Sig_VarNotFound(s_fYMouseAspectAdjustment);
}

void R_FillAddress_R_AllocTransObjectsVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

void R_FillAddress_VisEdicts(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars.
		int *cl_numvisedicts = NULL;
		cl_entity_t **cl_visedicts = NULL;
	*/

	ULONG_PTR cl_numvisedicts_VA = 0;
	ULONG cl_numvisedicts_RVA = 0;

	ULONG_PTR cl_visedicts_VA = 0;
	ULONG cl_visedicts_RVA = 0;

	{
		/*
			.text:01D0C7AF 8B 0D 50 F9 F0 02                                   mov     ecx, dword_2F0F950
			.text:01D0C7B5 81 F9 00 02 00 00                                   cmp     ecx, 200h
		*/
		char pattern[] = "\x8B\x0D\x2A\x2A\x2A\x2A\x81\xF9\x00\x2A\x00\x00";
		auto ClientDLL_AddEntity_Pattern = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(ClientDLL_AddEntity_Pattern);

		cl_numvisedicts_VA = *(ULONG_PTR*)((PUCHAR)ClientDLL_AddEntity_Pattern + 2);

		typedef struct
		{
			ULONG_PTR& cl_numvisedicts;
			ULONG_PTR& cl_visedicts;
		} VisEdicts_SearchContext;

		VisEdicts_SearchContext ctx = { cl_numvisedicts_VA, cl_visedicts_VA };

		g_pMetaHookAPI->DisasmRanges(ClientDLL_AddEntity_Pattern, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (VisEdicts_SearchContext*)context;

			if (ctx->cl_visedicts == 0 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == X86_REG_ECX &&
				pinst->detail->x86.operands[0].mem.scale == 4 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{
				//.text:01D198C9 89 04 8D 00 3A 6E 02                                mov     cl_visedicts[ecx*4], eax
				//.text:01D0C7C5 89 14 8D C0 F0 D5 02                                mov     cl_visedicts[ecx*4], edx

				ctx->cl_visedicts = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			}

			if (ctx->cl_visedicts)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(cl_numvisedicts, DllInfo);
		Convert_VA_to_RVA(cl_visedicts, DllInfo);
	}

	if (cl_numvisedicts_RVA)
		cl_numvisedicts = (decltype(cl_numvisedicts))VA_from_RVA(cl_numvisedicts, RealDllInfo);
	if (cl_visedicts_RVA)
		cl_visedicts = (decltype(cl_visedicts))VA_from_RVA(cl_visedicts, RealDllInfo);

	Sig_VarNotFound(cl_visedicts);
	Sig_VarNotFound(cl_numvisedicts);
}

void R_FillAddress_R_RenderFinalFog(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *g_bUserFogOn = NULL;
		float *g_UserFogColor = NULL;
		float *g_UserFogDensity = NULL;
		float *g_UserFogStart = NULL;
		float *g_UserFogEnd = NULL;
	*/
	ULONG_PTR g_bUserFogOn_VA = 0;
	ULONG g_bUserFogOn_RVA = 0;

	ULONG_PTR g_UserFogColor_VA = 0;
	ULONG g_UserFogColor_RVA = 0;

	ULONG_PTR g_UserFogDensity_VA = 0;
	ULONG g_UserFogDensity_RVA = 0;

	ULONG_PTR g_UserFogStart_VA = 0;
	ULONG g_UserFogStart_RVA = 0;

	ULONG_PTR g_UserFogEnd_VA = 0;
	ULONG g_UserFogEnd_RVA = 0;

	ULONG_PTR R_RenderFinalFog_VA = 0;
	ULONG R_RenderFinalFog_RVA = 0;

	ULONG_PTR SearchBase_VA = 0;

	if (gPrivateFuncs.R_RenderScene)
		SearchBase_VA = (ULONG_PTR)gPrivateFuncs.R_RenderScene;
	else if (gPrivateFuncs.R_RenderView_SvEngine)
		SearchBase_VA = (ULONG_PTR)gPrivateFuncs.R_RenderView_SvEngine;
	else if (gPrivateFuncs.R_RenderView)
		SearchBase_VA = (ULONG_PTR)gPrivateFuncs.R_RenderView;

	// Convert SearchBase_VA to DllInfo-based
	if (SearchBase_VA)
	{
		ULONG SearchBase_RVA = 0;
		Convert_VA_to_RVA(SearchBase, RealDllInfo);
		Convert_RVA_to_VA(SearchBase, DllInfo);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		typedef struct
		{
			ULONG_PTR& g_bUserFogOn;
			ULONG_PTR& R_RenderFinalFog;
			const mh_dll_info_t& DllInfo;
			bool bFoundPushB60h{};
			bool bFoundPush801h{};
			bool bFoundPushB65h{};
		} R_RenderFinalFog_SearchContext;

		R_RenderFinalFog_SearchContext ctx = { g_bUserFogOn_VA, R_RenderFinalFog_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)SearchBase_VA, 0x600, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_RenderFinalFog_SearchContext*)context;

				if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0)
				{
					DWORD imm = pinst->detail->x86.operands[0].mem.disp;

					ctx->g_bUserFogOn = (ULONG_PTR)imm;
				}

				else if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					pinst->detail->x86.operands[0].imm == 0xB60)
				{
					ctx->bFoundPushB60h = true;
					ctx->R_RenderFinalFog = (ULONG_PTR)address;
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

				if (ctx->g_bUserFogOn && ctx->bFoundPushB60h && ctx->bFoundPush801h && ctx->bFoundPushB65h)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(g_bUserFogOn, DllInfo);
		Convert_VA_to_RVA(R_RenderFinalFog, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		const char sigs[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x60\x0B\x00\x00";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(SearchBase_VA, 0x600, sigs, DllInfo);

		if (addr)
		{
			R_RenderFinalFog_VA = (ULONG_PTR)GetCallAddress(addr + 9);
			g_bUserFogOn_VA = *(ULONG_PTR*)(addr + 2);

			Convert_VA_to_RVA(R_RenderFinalFog, DllInfo);
			Convert_VA_to_RVA(g_bUserFogOn, DllInfo);
		}
	}
	else
	{
		const char sigs[] = "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x6A\x00";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(SearchBase_VA, 0x600, sigs, DllInfo);

		if (addr)
		{
			R_RenderFinalFog_VA = (ULONG_PTR)GetCallAddress(addr + 9);
			g_bUserFogOn_VA = *(ULONG_PTR*)(addr + 1);

			Convert_VA_to_RVA(R_RenderFinalFog, DllInfo);
			Convert_VA_to_RVA(g_bUserFogOn, DllInfo);
		}
	}

	if (R_RenderFinalFog_RVA)
	{
		typedef struct
		{
			ULONG_PTR& g_UserFogDensity;
			ULONG_PTR& g_UserFogColor;
			ULONG_PTR& g_UserFogStart;
			ULONG_PTR& g_UserFogEnd;
			const mh_dll_info_t& DllInfo;

			ULONG_PTR candidateVA{};

		} R_RenderFinalFog_VarsContext;

		R_RenderFinalFog_VarsContext ctx = { g_UserFogDensity_VA, g_UserFogColor_VA, g_UserFogStart_VA, g_UserFogEnd_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)R_RenderFinalFog_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_RenderFinalFog_VarsContext*)context;

			if (ctx->candidateVA &&
				pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				pinst->detail->x86.operands[0].imm > 0x100)
			{
				int pushvalue = pinst->detail->x86.operands[0].imm;

				if (!ctx->g_UserFogDensity && pushvalue == GL_FOG_DENSITY)
				{
					ctx->g_UserFogDensity = (ULONG_PTR)ctx->candidateVA;
					ctx->candidateVA = 0;
				}
				else if (!ctx->g_UserFogColor && pushvalue == GL_FOG_COLOR)
				{
					ctx->g_UserFogColor = (ULONG_PTR)ctx->candidateVA;
					ctx->candidateVA = 0;
				}
				else if (!ctx->g_UserFogStart && pushvalue == GL_FOG_START)
				{
					ctx->g_UserFogStart = (ULONG_PTR)ctx->candidateVA;
					ctx->candidateVA = 0;
				}
				else if (!ctx->g_UserFogEnd && pushvalue == GL_FOG_END)
				{
					ctx->g_UserFogEnd = (ULONG_PTR)ctx->candidateVA;
					ctx->candidateVA = 0;
				}
			}

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{//.text:01D88E84 A1 E0 23 73 02                                      mov     eax, flFogDensity

				ctx->candidateVA = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
			}
			else if (pinst->id == X86_INS_FLD &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0)
			{//.text:01D88E84 A1 E0 23 73 02                                      mov     eax, flFogDensity

				ctx->candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			}
			else if (pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_IMM &&
				(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
				)
			{//.text:01D88E84 A1 E0 23 73 02                                      mov     eax, flFogDensity

				ctx->candidateVA = (ULONG_PTR)pinst->detail->x86.operands[0].imm;
			}

			else if (pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{//F3 0F 10 05 B4 ED 1A 11                             movss   xmm0, flFogDensity

				ctx->candidateVA = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
			}

			if (ctx->g_UserFogDensity && ctx->g_UserFogColor && ctx->g_UserFogStart && ctx->g_UserFogEnd)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(g_UserFogDensity, DllInfo);
		Convert_VA_to_RVA(g_UserFogColor, DllInfo);
		Convert_VA_to_RVA(g_UserFogStart, DllInfo);
		Convert_VA_to_RVA(g_UserFogEnd, DllInfo);

		gPrivateFuncs.R_RenderFinalFog = (decltype(gPrivateFuncs.R_RenderFinalFog))VA_from_RVA(R_RenderFinalFog, RealDllInfo);
	}

	if (g_bUserFogOn_RVA)
		g_bUserFogOn = (decltype(g_bUserFogOn))VA_from_RVA(g_bUserFogOn, RealDllInfo);
	if (g_UserFogDensity_RVA)
		g_UserFogDensity = (decltype(g_UserFogDensity))VA_from_RVA(g_UserFogDensity, RealDllInfo);
	if (g_UserFogStart_RVA)
		g_UserFogStart = (decltype(g_UserFogStart))VA_from_RVA(g_UserFogStart, RealDllInfo);
	if (g_UserFogEnd_RVA)
		g_UserFogEnd = (decltype(g_UserFogEnd))VA_from_RVA(g_UserFogEnd, RealDllInfo);

	Sig_VarNotFound(g_UserFogDensity);
	Sig_VarNotFound(g_UserFogColor);
	Sig_VarNotFound(g_UserFogStart);
	Sig_VarNotFound(g_UserFogEnd);
}

void R_FillAddress_R_DrawTEntitiesOnListVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		float* r_blend = NULL;
		void *cl_frames = NULL;
		int *cl_parsecount = NULL;

		//Global vars
		int size_of_frame = sizeof(frame_t);
	*/
	ULONG_PTR r_blend_VA = 0;
	ULONG r_blend_RVA = 0;

	ULONG_PTR cl_frames_VA = 0;
	ULONG cl_frames_RVA = 0;

	ULONG_PTR cl_parsecount_VA = 0;
	ULONG cl_parsecount_RVA = 0;

	ULONG_PTR r_entorigin_VA = 0;
	ULONG r_entorigin_RVA = 0;

	ULONG_PTR ClientDLL_DrawTransparentTriangles_VA = 0;
	ULONG ClientDLL_DrawTransparentTriangles_RVA = 0;

	ULONG_PTR pfnDrawTransparentTriangles_VA = 0;
	ULONG pfnDrawTransparentTriangles_RVA = 0;

	if (g_dwEngineBuildnum <= 8684)
	{
		size_of_frame = 0x42B8;
	}
	ULONG_PTR R_DrawTEntitiesOnList_VA = (ULONG_PTR)gPrivateFuncs.R_DrawTEntitiesOnList;
	ULONG R_DrawTEntitiesOnList_RVA = 0;

	// Convert to DllInfo space
	Convert_VA_to_RVA(R_DrawTEntitiesOnList, RealDllInfo);
	Convert_RVA_to_VA(R_DrawTEntitiesOnList, DllInfo);

	typedef struct
	{
		ULONG_PTR& r_blend;
		ULONG_PTR& cl_frames;
		ULONG_PTR& cl_parsecount;
		ULONG_PTR& pfnDrawTransparentTriangles;
		ULONG_PTR& ClientDLL_DrawTransparentTriangles;
		const mh_dll_info_t& DllInfo;

		int disableFog_instcount{};
		int parsemod_instcount{};
		int getskin_instcount{};
		int r_entorigin_candidate_count{};
		int push2300_instcount{};
		int ClientDLL_DrawTransparentTriangles_candidate_instcount{};
		ULONG_PTR r_entorigin_candidateVA[3];
	} R_DrawTEntitiesOnList_SearchContext;

	R_DrawTEntitiesOnList_SearchContext ctx = {
		r_blend_VA,
		cl_frames_VA,
		cl_parsecount_VA,
		pfnDrawTransparentTriangles_VA,
		ClientDLL_DrawTransparentTriangles_VA,
		DllInfo
	};

	g_pMetaHookAPI->DisasmRanges((void*)R_DrawTEntitiesOnList_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
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

			typedef struct
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

		if (!ctx->r_blend && ctx->disableFog_instcount &&
			instCount < ctx->disableFog_instcount + 15 &&
			(pinst->id == X86_INS_FSTP || pinst->id == X86_INS_FST) &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0)
		{//D9 15 80 61 DF 08 fst     r_blend
			ctx->r_blend = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
		}
		if (!ctx->r_blend && ctx->disableFog_instcount &&
			instCount < ctx->disableFog_instcount + 15 &&
			(pinst->id == X86_INS_MOVSS) &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG)
		{// F3 0F 11 05 54 40 1B 11                             movss   r_blend, xmm0
			ctx->r_blend = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
		}
		else if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D923D9 A1 DC 72 ED 01                                      mov     eax, cl_parsemod
			//.text:01D88CBB A1 CC AF E3 01                                      mov     eax, cl_parsemod
			DWORD value = *(DWORD*)pinst->detail->x86.operands[1].mem.disp;
			if (value == 63)
			{
				ctx->parsemod_instcount = instCount;
			}
		}
		else if (!ctx->cl_parsecount && ctx->parsemod_instcount &&
			instCount < ctx->parsemod_instcount + 3 &&
			(pinst->id == X86_INS_MOV || pinst->id == X86_INS_AND) &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D923DE 23 05 AC D2 30 02                                   and     eax, cl_parsecount
			//.text:01D88CC0 8B 0D 04 AE D8 02                                   mov     ecx, cl_parsecount
			ctx->cl_parsecount = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
		}
		else if (!ctx->cl_frames && ctx->parsemod_instcount &&
			instCount < ctx->parsemod_instcount + 20 &&
			pinst->id == X86_INS_LEA &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base != 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D923F0 8D 80 F4 D5 30 02                                   lea     eax, cl_frames[eax]
			//.text:01D88CE8 8D 84 CA 4C B1 D8 02                                lea     eax, cl_frames_1[edx+ecx*8]
			ctx->cl_frames = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
		}
		else if (ctx->parsemod_instcount &&
			instCount < ctx->parsemod_instcount + 5 &&
			pinst->id == X86_INS_IMUL &&
			pinst->detail->x86.op_count == 3 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[2].type == X86_OP_IMM &&
			pinst->detail->x86.operands[2].imm > 0x4000 &&
			pinst->detail->x86.operands[2].imm < 0xF000)
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

		if (!ctx->ClientDLL_DrawTransparentTriangles)
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
				if (address[0] == 0xE8 && instLen == 5)
				{
					ctx->ClientDLL_DrawTransparentTriangles = (ULONG_PTR)pinst->detail->x86.operands[0].imm;
					ctx->ClientDLL_DrawTransparentTriangles_candidate_instcount = instCount;
				}

				if (ctx->ClientDLL_DrawTransparentTriangles && instCount == ctx->ClientDLL_DrawTransparentTriangles_candidate_instcount + 1)
				{
					/*
						.text:01D88E15 E8 96 2C F8 FF                                      call    ClientDLL_DrawTransparentTriangles
						.text:01D88E1A A1 E4 23 73 02                                      mov     eax, g_bUserFogOn
					*/
					ULONG_PTR g_bUserFogOn_VA = 0;
					if (g_bUserFogOn)
					{
						g_bUserFogOn_VA = (ULONG_PTR)g_bUserFogOn;
					}

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
						ctx->ClientDLL_DrawTransparentTriangles = 0;
					}

					if (ctx->ClientDLL_DrawTransparentTriangles)
					{
						PUCHAR code = (PUCHAR)ctx->ClientDLL_DrawTransparentTriangles;

						if (code[0] == 0xA1)
						{
							ctx->pfnDrawTransparentTriangles = *(ULONG_PTR*)(code + 1);
						}
					}
				}
			}
		}

		if (ctx->r_blend && ctx->cl_parsecount && ctx->cl_frames && ctx->r_entorigin_candidate_count >= 3 && ctx->ClientDLL_DrawTransparentTriangles)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
		}, 0, &ctx);

	Convert_VA_to_RVA(r_blend, DllInfo);
	Convert_VA_to_RVA(cl_frames, DllInfo);
	Convert_VA_to_RVA(cl_parsecount, DllInfo);
	Convert_VA_to_RVA(ClientDLL_DrawTransparentTriangles, DllInfo);
	Convert_VA_to_RVA(pfnDrawTransparentTriangles, DllInfo);

	if (r_blend_RVA)
		r_blend = (decltype(r_blend))VA_from_RVA(r_blend, RealDllInfo);
	if (cl_frames_RVA)
		cl_frames = (decltype(cl_frames))VA_from_RVA(cl_frames, RealDllInfo);
	if (cl_parsecount_RVA)
		cl_parsecount = (decltype(cl_parsecount))VA_from_RVA(cl_parsecount, RealDllInfo);
	if (ClientDLL_DrawTransparentTriangles_RVA)
		gPrivateFuncs.ClientDLL_DrawTransparentTriangles = (decltype(gPrivateFuncs.ClientDLL_DrawTransparentTriangles))VA_from_RVA(ClientDLL_DrawTransparentTriangles, RealDllInfo);
	if (pfnDrawTransparentTriangles_RVA)
		gPrivateFuncs.pfnDrawTransparentTriangles = (decltype(gPrivateFuncs.pfnDrawTransparentTriangles))VA_from_RVA(pfnDrawTransparentTriangles, RealDllInfo);

	Sig_VarNotFound(r_blend);
	Sig_VarNotFound(cl_frames);
	Sig_VarNotFound(cl_parsecount);
	Sig_VarNotFound(size_of_frame);
	Sig_FuncNotFound(ClientDLL_DrawTransparentTriangles);

	if (ctx.r_entorigin_candidate_count >= 2)
	{
		std::qsort(ctx.r_entorigin_candidateVA, ctx.r_entorigin_candidate_count, sizeof(ctx.r_entorigin_candidateVA[0]), [](const void* a, const void* b) {
			return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
			});

		if ((ULONG_PTR)ctx.r_entorigin_candidateVA[0] == (ULONG_PTR)ctx.r_entorigin_candidateVA[1] - sizeof(float))
		{
			r_entorigin_VA = ctx.r_entorigin_candidateVA[0];
			Convert_VA_to_RVA(r_entorigin, DllInfo);
			if (r_entorigin_RVA)
				r_entorigin = (decltype(r_entorigin))VA_from_RVA(r_entorigin, RealDllInfo);
		}
	}
	else if (ctx.r_entorigin_candidate_count == 1)
	{
		r_entorigin_VA = ctx.r_entorigin_candidateVA[0];
		Convert_VA_to_RVA(r_entorigin, DllInfo);
		if (r_entorigin_RVA)
			r_entorigin = (decltype(r_entorigin))VA_from_RVA(r_entorigin, RealDllInfo);
	}

	Sig_VarNotFound(r_entorigin);
}

void R_FillAddress_R_RecursiveWorldNodeVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars.
		int *r_framecount = NULL;
		int *r_visframecount = NULL;
		msurface_t **skychain = NULL;
		msurface_t **waterchain = NULL;
	*/
	ULONG_PTR r_framecount_VA = 0;
	ULONG r_framecount_RVA = 0;

	ULONG_PTR r_visframecount_VA = 0;
	ULONG r_visframecount_RVA = 0;

	ULONG_PTR skychain_VA = 0;
	ULONG skychain_RVA = 0;

	ULONG_PTR waterchain_VA = 0;
	ULONG waterchain_RVA = 0;

	ULONG_PTR R_RecursiveWorldNode_VA = (ULONG_PTR)gPrivateFuncs.R_RecursiveWorldNode;
	ULONG R_RecursiveWorldNode_RVA = 0;

	// Convert to DllInfo space
	if (R_RecursiveWorldNode_VA)
	{
		Convert_VA_to_RVA(R_RecursiveWorldNode, RealDllInfo);
		Convert_RVA_to_VA(R_RecursiveWorldNode, DllInfo);
	}

	typedef struct
	{
		ULONG_PTR& r_framecount;
		ULONG_PTR& r_visframecount;
		ULONG_PTR& skychain;
		ULONG_PTR& waterchain;
		const mh_dll_info_t& DllInfo;

		int movexx_offset{};
		int movexx_instcount{};
		int movexx_register{};
		int cmp_register{};
		ULONG_PTR cmp_candidateVA{};
		int test_cl_instcount{};
		int test_cl_flag{};
	} R_RecursiveWorldNode_ctx;

	R_RecursiveWorldNode_ctx ctx = { r_framecount_VA, r_visframecount_VA, skychain_VA, waterchain_VA, DllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)R_RecursiveWorldNode_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_RecursiveWorldNode_ctx*)context;

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

				if (ctx->movexx_offset == 4 && !ctx->r_visframecount)
					ctx->r_visframecount = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
				else if (ctx->movexx_offset == 0 && !ctx->r_framecount)
					ctx->r_framecount = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
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
				if (ctx->movexx_offset == 4 && !ctx->r_visframecount)
					ctx->r_visframecount = (ULONG_PTR)ctx->cmp_candidateVA;
				else if (ctx->movexx_offset == 0 && !ctx->r_framecount)
					ctx->r_framecount = (ULONG_PTR)ctx->cmp_candidateVA;
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
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
				)
			{
				//.text:01D5A66D A1 C4 54 F5 03                                      mov     eax, skychain

				if (!ctx->skychain && ctx->test_cl_flag == 4)
					ctx->skychain = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
				else if (!ctx->waterchain && ctx->test_cl_flag == 0x10)
					ctx->waterchain = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
			}

			if (ctx->r_visframecount && ctx->r_framecount && ctx->skychain && ctx->waterchain)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	Convert_VA_to_RVA(r_framecount, DllInfo);
	Convert_VA_to_RVA(r_visframecount, DllInfo);
	Convert_VA_to_RVA(skychain, DllInfo);
	Convert_VA_to_RVA(waterchain, DllInfo);

	if (r_framecount_RVA)
		r_framecount = (decltype(r_framecount))VA_from_RVA(r_framecount, RealDllInfo);
	if (r_visframecount_RVA)
		r_visframecount = (decltype(r_visframecount))VA_from_RVA(r_visframecount, RealDllInfo);
	if (skychain_RVA)
		skychain = (decltype(skychain))VA_from_RVA(skychain, RealDllInfo);
	if (waterchain_RVA)
		waterchain = (decltype(waterchain))VA_from_RVA(waterchain, RealDllInfo);

	Sig_VarNotFound(r_framecount);
	Sig_VarNotFound(r_visframecount);
	Sig_VarNotFound(skychain);
	Sig_VarNotFound(waterchain);
}

void R_FillAddress_R_LoadSkybox(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *gSkyTexNumber = NULL;
	*/
	ULONG_PTR gSkyTexNumber_VA = 0;
	ULONG gSkyTexNumber_RVA = 0;

	ULONG_PTR R_LoadSkyboxInt_SvEngine_VA = 0;
	ULONG R_LoadSkyboxInt_SvEngine_RVA = 0;

	ULONG_PTR R_LoadSkyBox_SvEngine_VA = 0;
	ULONG R_LoadSkyBox_SvEngine_RVA = 0;

	ULONG_PTR R_LoadSkys_VA = 0;
	ULONG R_LoadSkys_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "SKY: ";
		auto R_LoadSkys_String = Search_Pattern_Data(sigs, DllInfo);
		if (!R_LoadSkys_String)
			R_LoadSkys_String = Search_Pattern_Rdata(sigs, DllInfo);
		Sig_VarNotFound(R_LoadSkys_String);

		char pattern[] = "\x75\x2A\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 3) = (DWORD)R_LoadSkys_String;
		auto R_LoadSkys_PushString = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(R_LoadSkys_PushString);

		R_LoadSkyboxInt_SvEngine_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_LoadSkys_PushString, 0x600, [](PUCHAR Candidate) {
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

		Convert_VA_to_RVA(R_LoadSkyboxInt_SvEngine, DllInfo);

		//SvEngine always has g_bHasOfficialGLTexAllocSupport == true

		typedef struct
		{
			ULONG_PTR& gSkyTexNumber;
			const mh_dll_info_t& DllInfo;
			int candidate_register{};
			int candidate_instcount{};
			ULONG_PTR candidate_gSkyTexNumberVA{};
		} R_LoadSkys_SearchContext;

		R_LoadSkys_SearchContext ctx = { gSkyTexNumber_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)R_LoadSkyboxInt_SvEngine_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_LoadSkys_SearchContext*)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//.text:01D4F9D8 BE 60 34 34 02 mov     esi, offset gSkyTexNumber

					ctx->candidate_gSkyTexNumberVA = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
					ctx->candidate_register = pinst->detail->x86.operands[0].reg;
					ctx->candidate_instcount = instCount;
				}
				else if (ctx->candidate_gSkyTexNumberVA &&
					pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == ctx->candidate_register &&
					pinst->detail->x86.operands[0].mem.disp == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					instCount < ctx->candidate_instcount + 5)
				{//text:01D4F9DD 39 3E cmp     [esi], edi
					ctx->gSkyTexNumber = (ULONG_PTR)ctx->candidate_gSkyTexNumberVA;
				}
				else if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//.text:01D5FC00 FF B6 80 69 00 08                                   push    gSkyTexNumber[esi]
					ctx->gSkyTexNumber = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}

				if (ctx->gSkyTexNumber)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(gSkyTexNumber, DllInfo);

		const char sigs2[] = "desert\0";
		auto R_LoadSkyBox_String = Search_Pattern_Rdata(sigs2, DllInfo);
		Sig_VarNotFound(R_LoadSkyBox_String);

		char pattern2[] = "\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C";
		*(DWORD*)(pattern2 + 6) = (DWORD)R_LoadSkyBox_String;
		auto R_LoadSkyBox_PushString = Search_Pattern(pattern2, DllInfo);
		Sig_VarNotFound(R_LoadSkyBox_PushString);

		R_LoadSkyBox_SvEngine_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_LoadSkyBox_PushString, 0x200, [](PUCHAR Candidate) {
			/*
				.text:01D5FEF0                                     ; void __cdecl R_LoadSkyName(const char *name)
				.text:01D5FEF0                                     R_LoadSkyName   proc near               ; CODE XREF: sub_1D042D0+E��p
				.text:01D5FEF0                                                                             ; R_LoadSkys_0+5��p ...
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

		Convert_VA_to_RVA(R_LoadSkyBox_SvEngine, DllInfo);
	}
	else
	{
		const char sigs[] = "SKY: ";
		auto R_LoadSkys_String = Search_Pattern_Data(sigs, DllInfo);
		if (!R_LoadSkys_String)
			R_LoadSkys_String = Search_Pattern_Rdata(sigs, DllInfo);
		Sig_VarNotFound(R_LoadSkys_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xC7\x2A\x2A\x2A\x2A\x00\x00";
		*(DWORD*)(pattern + 1) = (DWORD)R_LoadSkys_String;
		auto R_LoadSkys_PushString = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(R_LoadSkys_PushString);

		R_LoadSkys_VA = (ULONG_PTR)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_LoadSkys_PushString, 0x600, [](PUCHAR Candidate) {
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

		Convert_VA_to_RVA(R_LoadSkys, DllInfo);

		if (g_bHasOfficialGLTexAllocSupport)
		{
			typedef struct
			{
				ULONG_PTR& gSkyTexNumber;
				const mh_dll_info_t& DllInfo;
				int candidate_register{};
				int candidate_instcount{};
				ULONG_PTR candidate_gSkyTexNumberVA{};
			} R_LoadSkys_SearchContext;

			R_LoadSkys_SearchContext ctx = { gSkyTexNumber_VA, DllInfo };

			g_pMetaHookAPI->DisasmRanges((void*)R_LoadSkys_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
				{
					auto pinst = (cs_insn*)inst;
					auto ctx = (R_LoadSkys_SearchContext*)context;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
					{//.text:01D4F9D8 BE 60 34 34 02 mov     esi, offset gSkyTexNumber

						ctx->candidate_gSkyTexNumberVA = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
						ctx->candidate_register = pinst->detail->x86.operands[0].reg;
						ctx->candidate_instcount = instCount;
					}
					else if (ctx->candidate_gSkyTexNumberVA &&
						pinst->id == X86_INS_CMP &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == ctx->candidate_register &&
						pinst->detail->x86.operands[0].mem.disp == 0 &&
						pinst->detail->x86.operands[1].type == X86_OP_REG &&
						instCount < ctx->candidate_instcount + 5)
					{//text:01D4F9DD 39 3E cmp     [esi], edi
						ctx->gSkyTexNumber = (ULONG_PTR)ctx->candidate_gSkyTexNumberVA;
					}
					else if (ctx->candidate_gSkyTexNumberVA &&
						pinst->id == X86_INS_CMP &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base == ctx->candidate_register &&
						pinst->detail->x86.operands[0].mem.disp == 0 &&
						pinst->detail->x86.operands[1].type == X86_OP_IMM &&
						pinst->detail->x86.operands[1].imm == 0 &&
						instCount < ctx->candidate_instcount + 5)
					{//.text:102506E6 83 3E 00 cmp     dword ptr [esi], 0
						ctx->gSkyTexNumber = (ULONG_PTR)ctx->candidate_gSkyTexNumberVA;
					}
					else if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base != 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
					{//.text:01D5FC00 FF B6 80 69 00 08                                   push    gSkyTexNumber[esi]
						ctx->gSkyTexNumber = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					}

					if (ctx->gSkyTexNumber)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
				}, 0, &ctx);

			Convert_VA_to_RVA(gSkyTexNumber, DllInfo);
		}
	}

	if (R_LoadSkyboxInt_SvEngine_RVA)
		gPrivateFuncs.R_LoadSkyboxInt_SvEngine = (decltype(gPrivateFuncs.R_LoadSkyboxInt_SvEngine))VA_from_RVA(R_LoadSkyboxInt_SvEngine, RealDllInfo);

	if (R_LoadSkyBox_SvEngine_RVA)
		gPrivateFuncs.R_LoadSkyBox_SvEngine = (decltype(gPrivateFuncs.R_LoadSkyBox_SvEngine))VA_from_RVA(R_LoadSkyBox_SvEngine, RealDllInfo);

	if (R_LoadSkys_RVA)
		gPrivateFuncs.R_LoadSkys = (decltype(gPrivateFuncs.R_LoadSkys))VA_from_RVA(R_LoadSkys, RealDllInfo);

	if (gSkyTexNumber_RVA)
		gSkyTexNumber = (decltype(gSkyTexNumber))VA_from_RVA(gSkyTexNumber, RealDllInfo);

	Sig_FuncNotFound(R_LoadSkyboxInt_SvEngine);
	Sig_FuncNotFound(R_LoadSkyBox_SvEngine);
	Sig_FuncNotFound(R_LoadSkys);
	Sig_VarNotFound(gSkyTexNumber);

	/*
  //Global pointers that link into engine vars.
	  int *r_loading_skybox = NULL;
  */
	ULONG_PTR r_loading_skybox_VA = 0;
	ULONG r_loading_skybox_RVA = 0;

	{
		ULONG_PTR SearchBase_VA = 0;

		if (g_iEngineType == ENGINE_SVENGINE)
			SearchBase_VA = (ULONG_PTR)R_LoadSkyBox_SvEngine_VA;
		else
			SearchBase_VA = (ULONG_PTR)R_LoadSkys_VA;

		typedef struct
		{
			ULONG_PTR& r_loading_skybox;
			const mh_dll_info_t& DllInfo;
		} R_LoadSkyboxVars_SearchContext;

		R_LoadSkyboxVars_SearchContext ctx = { r_loading_skybox_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)SearchBase_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_LoadSkyboxVars_SearchContext*)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0)
				{
					ctx->r_loading_skybox = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
				}
				else if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0)
				{
					ctx->r_loading_skybox = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}

				if (ctx->r_loading_skybox)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(r_loading_skybox, DllInfo);
	}

	if (r_loading_skybox_RVA)
		r_loading_skybox = (decltype(r_loading_skybox))VA_from_RVA(r_loading_skybox, RealDllInfo);

	Sig_VarNotFound(r_loading_skybox);
}

void R_FillAddress_GL_FilterMinMaxVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	//Global pointers that link into engine vars.
		int *gl_filter_min = NULL;
		int *gl_filter_max = NULL;
	*/
	ULONG_PTR gl_filter_min_VA = 0;
	ULONG gl_filter_min_RVA = 0;

	ULONG_PTR gl_filter_max_VA = 0;
	ULONG gl_filter_max_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define GL_FILTER_SIG_SVENGINE "\xDB\x05\x2A\x2A\x2A\x2A\x2A\xD9\x1C\x2A\x68\x01\x28\x00\x00\x68\xE1\x0D\x00\x00\xFF\x2A\xDB\x05"
		/*
			.text:01D501E4                                     loc_1D501E4:                            ; CODE XREF: .text:01D501D5��j
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
			gl_filter_min_VA = *(ULONG_PTR*)(addr + 2);
			gl_filter_max_VA = *(ULONG_PTR*)(addr + Sig_Length(GL_FILTER_SIG_SVENGINE));

			Convert_VA_to_RVA(gl_filter_min, DllInfo);
			Convert_VA_to_RVA(gl_filter_max, DllInfo);
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
			gl_filter_min_VA = *(ULONG_PTR*)(addr + 4);
			gl_filter_max_VA = *(ULONG_PTR*)(addr + Sig_Length(GL_FILTER_SIG_GOLDSRC_HL25));

			Convert_VA_to_RVA(gl_filter_min, DllInfo);
			Convert_VA_to_RVA(gl_filter_max, DllInfo);
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
			gl_filter_min_VA = *(ULONG_PTR*)(addr + 2);
			gl_filter_max_VA = *(ULONG_PTR*)(addr + Sig_Length(GL_FILTER_SIG_GOLDSRC_NEW));

			Convert_VA_to_RVA(gl_filter_min, DllInfo);
			Convert_VA_to_RVA(gl_filter_max, DllInfo);
		}
		else
		{
			addr = (ULONG_PTR)Search_Pattern(GL_FILTER_SIG_GOLDSRC_NEW2, DllInfo);
			if (addr)
			{
				gl_filter_min_VA = *(ULONG_PTR*)(addr + 2);
				gl_filter_max_VA = *(ULONG_PTR*)(addr + Sig_Length(GL_FILTER_SIG_GOLDSRC_NEW2));

				Convert_VA_to_RVA(gl_filter_min, DllInfo);
				Convert_VA_to_RVA(gl_filter_max, DllInfo);
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
			gl_filter_min_VA = *(ULONG_PTR*)(addr + 2);
			gl_filter_max_VA = *(ULONG_PTR*)(addr + Sig_Length(GL_FILTER_SIG_BLOB));

			Convert_VA_to_RVA(gl_filter_min, DllInfo);
			Convert_VA_to_RVA(gl_filter_max, DllInfo);
		}
	}

	if (gl_filter_min_RVA)
		gl_filter_min = (decltype(gl_filter_min))VA_from_RVA(gl_filter_min, RealDllInfo);
	if (gl_filter_max_RVA)
		gl_filter_max = (decltype(gl_filter_max))VA_from_RVA(gl_filter_max, RealDllInfo);

	Sig_VarNotFound(gl_filter_min);
	Sig_VarNotFound(gl_filter_max);
}

void R_FillAddress_ScrFov(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	//Global pointers that link into engine vars.
		float *scrfov = NULL;
	*/
	ULONG_PTR scrfov_VA = 0;
	ULONG scrfov_RVA = 0;

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
			v1[7] = scrfov;
		*/

		const char pattern[] = "\xD9\x05\x2A\x2A\x2A\x2A\xD9\x5C\x24\x1C\x89\x44\x24\x18";
		auto addr = (ULONG_PTR)Search_Pattern(pattern, DllInfo);

		if (addr)
		{
			scrfov_VA = *(ULONG_PTR*)(addr + 2);
			Convert_VA_to_RVA(scrfov, DllInfo);
		}
	}
	else
	{
		const char pattern[] = "\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x16\x43";
		auto addr = (ULONG_PTR)Search_Pattern(pattern, DllInfo);

		if (addr)
		{
			const char pattern2[] = "\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x20\x41";
			auto addr2 = (ULONG_PTR)Search_Pattern_From_Size(((PUCHAR)addr - 0x50), 0x50, pattern2, DllInfo);

			if (addr2)
			{
				scrfov_VA = *(ULONG_PTR*)(addr2 + 2);
				Convert_VA_to_RVA(scrfov, DllInfo);
			}
		}
	}

	if (scrfov_RVA)
		scrfov = (decltype(scrfov))VA_from_RVA(scrfov, RealDllInfo);

	Sig_VarNotFound(scrfov);
}

//Got CL_IsDevOverviewMode, CL_SetDevOverView and refdef here
void R_FillAddress_RenderSceneVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

	ULONG_PTR CL_IsDevOverviewMode_VA = 0;
	ULONG CL_IsDevOverviewMode_RVA = 0;

	ULONG_PTR CL_SetDevOverView_VA = 0;
	ULONG CL_SetDevOverView_RVA = 0;

	ULONG_PTR r_refdef_SvEngine_VA = 0;
	ULONG r_refdef_SvEngine_RVA = 0;

	ULONG_PTR r_refdef_GoldSrc_VA = 0;
	ULONG r_refdef_GoldSrc_RVA = 0;

	ULONG_PTR SearchBase_VA = 0;
	SIZE_T SearchLength = 0;

	if (gPrivateFuncs.R_RenderScene)
	{
		SearchBase_VA = (ULONG_PTR)gPrivateFuncs.R_RenderScene;
		SearchLength = 0x100;
	}
	else if (gPrivateFuncs.R_RenderView_SvEngine)
	{
		SearchBase_VA = (ULONG_PTR)gPrivateFuncs.R_RenderView_SvEngine;
		SearchLength = 0x450;
	}
	else if (gPrivateFuncs.R_RenderView)
	{
		SearchBase_VA = (ULONG_PTR)gPrivateFuncs.R_RenderView;
		SearchLength = 0x450;
	}

	// Convert SearchBase_VA to DllInfo-based
	if (SearchBase_VA)
	{
		ULONG SearchBase_RVA = 0;
		Convert_VA_to_RVA(SearchBase, RealDllInfo);
		Convert_RVA_to_VA(SearchBase, DllInfo);
	}

	typedef struct
	{
		ULONG_PTR& CL_IsDevOverviewMode;
		ULONG_PTR& CL_SetDevOverView;
		ULONG_PTR& r_refdef_SvEngine;
		ULONG_PTR& r_refdef_GoldSrc;
		const mh_dll_info_t& DllInfo;

		int Call_Candidate_instCount{};
		PVOID Call_Candidate{};
	} R_RenderScene_SearchContext;

	R_RenderScene_SearchContext ctx = { CL_IsDevOverviewMode_VA, CL_SetDevOverView_VA, r_refdef_SvEngine_VA, r_refdef_GoldSrc_VA, DllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)SearchBase_VA, SearchLength, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
		auto pinst = (cs_insn*)inst;
		auto ctx = (R_RenderScene_SearchContext*)context;

		if (!ctx->CL_IsDevOverviewMode)
		{
			if (address[0] == 0xE8 && address[5] == 0x85 && address[6] == 0xC0)
			{
				ctx->Call_Candidate = (decltype(ctx->Call_Candidate))pinst->detail->x86.operands[0].imm;
				ctx->Call_Candidate_instCount = instCount;
			}

			if (address[0] == 0x68 && address[5] == 0xE8 && instCount > ctx->Call_Candidate_instCount && instCount <= ctx->Call_Candidate_instCount + 3)
			{
				ctx->CL_IsDevOverviewMode = (ULONG_PTR)ctx->Call_Candidate;
			}
		}

		if (ctx->CL_IsDevOverviewMode && !ctx->CL_SetDevOverView &&
			address[0] == 0xE8 && address[-5] == 0x68 && address[5] == 0x83)
		{
			if (g_iEngineType == ENGINE_SVENGINE)
			{
				ctx->r_refdef_SvEngine = *(ULONG_PTR*)(address - 4);
			}
			else
			{
				ctx->r_refdef_GoldSrc = *(ULONG_PTR*)(address - 4);
			}

			ctx->CL_SetDevOverView = (ULONG_PTR)pinst->detail->x86.operands[0].imm;
		}

		if (ctx->CL_IsDevOverviewMode && ctx->CL_SetDevOverView)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

		}, 0, &ctx);

	Convert_VA_to_RVA(CL_IsDevOverviewMode, DllInfo);
	Convert_VA_to_RVA(CL_SetDevOverView, DllInfo);

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Convert_VA_to_RVA(r_refdef_SvEngine, DllInfo);
	}
	else
	{
		Convert_VA_to_RVA(r_refdef_GoldSrc, DllInfo);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		if (r_refdef_SvEngine_RVA)
		{
			r_refdef_SvEngine = (decltype(r_refdef_SvEngine))VA_from_RVA(r_refdef_SvEngine, RealDllInfo);
			r_refdef.vrect = &r_refdef_SvEngine->vrect;
			r_refdef.vieworg = &r_refdef_SvEngine->vieworg;
			r_refdef.viewangles = &r_refdef_SvEngine->viewangles;
			r_refdef.ambientlight = &r_refdef_SvEngine->ambientlight;
			r_refdef.onlyClientDraws = &r_refdef_SvEngine->onlyClientDraws;
		}

		Sig_VarNotFound(r_refdef_SvEngine);
	}
	else
	{
		if (r_refdef_GoldSrc_RVA)
		{
			r_refdef_GoldSrc = (decltype(r_refdef_GoldSrc))VA_from_RVA(r_refdef_GoldSrc, RealDllInfo);
			r_refdef.vrect = &r_refdef_GoldSrc->vrect;
			r_refdef.vieworg = &r_refdef_GoldSrc->vieworg;
			r_refdef.viewangles = &r_refdef_GoldSrc->viewangles;
			r_refdef.ambientlight = &r_refdef_GoldSrc->ambientlight;
			r_refdef.onlyClientDraws = &r_refdef_GoldSrc->onlyClientDraws;
		}

		Sig_VarNotFound(r_refdef_GoldSrc);
	}

	if (CL_IsDevOverviewMode_RVA)
		gPrivateFuncs.CL_IsDevOverviewMode = (decltype(gPrivateFuncs.CL_IsDevOverviewMode))VA_from_RVA(CL_IsDevOverviewMode, RealDllInfo);

	if (CL_SetDevOverView_RVA)
		gPrivateFuncs.CL_SetDevOverView = (decltype(gPrivateFuncs.CL_SetDevOverView))VA_from_RVA(CL_SetDevOverView, RealDllInfo);

	Sig_FuncNotFound(CL_IsDevOverviewMode);
	Sig_FuncNotFound(CL_SetDevOverView);
}

//Got ClientDLL_DrawNormalTriangles_VA, cl_waterlevel and gDevOverview here
void R_FillAddress_RenderSceneVars2(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	ULONG_PTR cl_waterlevel_VA = 0;
	ULONG cl_waterlevel_RVA = 0;

	ULONG_PTR ClientDLL_DrawNormalTriangles_VA = 0;
	ULONG ClientDLL_DrawNormalTriangles_RVA = 0;

	ULONG_PTR gDevOverview_VA = 0;
	ULONG gDevOverview_RVA = 0;

	ULONG_PTR SearchBase_VA = 0;
	SIZE_T SearchLength = 0;

	if (gPrivateFuncs.R_RenderScene)
	{
		SearchBase_VA = (ULONG_PTR)gPrivateFuncs.R_RenderScene;
		SearchLength = 0x600;
	}
	else if (gPrivateFuncs.R_RenderView_SvEngine)
	{
		SearchBase_VA = (ULONG_PTR)gPrivateFuncs.R_RenderView_SvEngine;
		SearchLength = 0x800;
	}
	else if (gPrivateFuncs.R_RenderView)
	{
		SearchBase_VA = (ULONG_PTR)gPrivateFuncs.R_RenderView;
		SearchLength = 0x800;
	}

	// Convert SearchBase_VA to DllInfo-based
	if (SearchBase_VA)
	{
		ULONG SearchBase_RVA = 0;
		Convert_VA_to_RVA(SearchBase, RealDllInfo);
		Convert_RVA_to_VA(SearchBase, DllInfo);
	}
	{
		typedef struct
		{
			ULONG_PTR& cl_waterlevel;
			ULONG_PTR& ClientDLL_DrawNormalTriangles;
			const mh_dll_info_t& DllInfo;
			std::map<int, ULONG_PTR> candidate_disp;
			std::map<int, ULONG_PTR> candidate_addr;
		} R_RenderScene_SearchContext;

		R_RenderScene_SearchContext ctx = { cl_waterlevel_VA, ClientDLL_DrawNormalTriangles_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)SearchBase_VA, SearchLength, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_RenderScene_SearchContext*)context;

				if (!ctx->cl_waterlevel && pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM && pinst->detail->x86.operands[1].imm == 2
					)
				{
					ctx->cl_waterlevel = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}

				if (!ctx->cl_waterlevel && pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
					)
				{
					ctx->candidate_addr[pinst->detail->x86.operands[0].reg] = (ULONG_PTR)address;
					ctx->candidate_disp[pinst->detail->x86.operands[0].reg] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
				}

				if (!ctx->cl_waterlevel && pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 2)
				{
					if (ctx->candidate_addr.find(pinst->detail->x86.operands[0].reg) != ctx->candidate_addr.end())
					{
						auto addr = (PUCHAR)ctx->candidate_addr[pinst->detail->x86.operands[0].reg];
						auto disp = ctx->candidate_disp[pinst->detail->x86.operands[0].reg];
						if (address > addr && address < addr + 0x30)
						{
							ctx->cl_waterlevel = (ULONG_PTR)disp;
						}
					}
				}

				if (!ctx->ClientDLL_DrawNormalTriangles)
				{
					if (address[0] == 0xE8 && instLen == 5)
					{
						auto candidate = (PUCHAR)pinst->detail->x86.operands[0].imm;

						//.text:01D1A4E0                                     ClientDLL_DrawNormalTriangles proc near ; CODE XREF: R_RenderScene:loc_1D566A7��p
						//.text:01D1A4E0 A1 70 5B 04 02                                      mov     eax, pfnDrawNormalTriangles
						//.text:01D1A4E5 85 C0                                               test    eax, eax
						if (candidate[0] == 0xA1 && candidate[5] == 0x85 && candidate[6] == 0xC0)
						{
							auto pfnDrawNormalTriangles = *(ULONG_PTR*)(candidate + 1);

							if (pfnDrawNormalTriangles == (ULONG_PTR)gPrivateFuncs.pfnDrawTransparentTriangles - sizeof(ULONG_PTR))
							{
								ctx->ClientDLL_DrawNormalTriangles = (ULONG_PTR)candidate;
							}
						}
					}
				}

				if (ctx->cl_waterlevel && ctx->ClientDLL_DrawNormalTriangles)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(cl_waterlevel, DllInfo);
		Convert_VA_to_RVA(ClientDLL_DrawNormalTriangles, DllInfo);
	}

	if (cl_waterlevel_RVA)
		cl_waterlevel = (decltype(cl_waterlevel))VA_from_RVA(cl_waterlevel, RealDllInfo);

	if (ClientDLL_DrawNormalTriangles_RVA)
		gPrivateFuncs.ClientDLL_DrawNormalTriangles = (decltype(gPrivateFuncs.ClientDLL_DrawNormalTriangles))VA_from_RVA(ClientDLL_DrawNormalTriangles, RealDllInfo);

	Sig_VarNotFound(cl_waterlevel);
	Sig_FuncNotFound(ClientDLL_DrawNormalTriangles);

	ULONG_PTR CL_SetDevOverView_VA = 0;

	if (gPrivateFuncs.CL_SetDevOverView)
		CL_SetDevOverView_VA = (ULONG_PTR)gPrivateFuncs.CL_SetDevOverView;

	// Convert CL_SetDevOverView_VA to DllInfo-based
	if (CL_SetDevOverView_VA)
	{
		ULONG CL_SetDevOverView_RVA = 0;
		Convert_VA_to_RVA(CL_SetDevOverView, RealDllInfo);
		Convert_RVA_to_VA(CL_SetDevOverView, DllInfo);
	}
	{
		typedef struct
		{
			ULONG_PTR& gDevOverview;
			const mh_dll_info_t& DllInfo;
			int Push30_instCount{};
			ULONG_PTR CandidatesVA[6]{};
			int CandidateCount{};
		} OverviewZoomCotext_t;

		OverviewZoomCotext_t ctx2 = { gDevOverview_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)CL_SetDevOverView_VA, 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
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

			gDevOverview_VA = ctx2.CandidatesVA[0];
			Convert_VA_to_RVA(gDevOverview, DllInfo);
		}
	}

	if (gDevOverview_RVA)
		gDevOverview = (decltype(gDevOverview))VA_from_RVA(gDevOverview, RealDllInfo);

	Sig_VarNotFound(gDevOverview);
}

void R_FillAddress_CL_IsDevOverviewModeVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	//Sven Co-op only
		int* allow_cheats = NULL;
	*/

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		ULONG_PTR allow_cheats_VA = 0;
		ULONG allow_cheats_RVA = 0;

		ULONG_PTR CL_IsDevOverviewMode_VA = 0;

		if (gPrivateFuncs.CL_IsDevOverviewMode)
			CL_IsDevOverviewMode_VA = (ULONG_PTR)gPrivateFuncs.CL_IsDevOverviewMode;

		// Convert CL_IsDevOverviewMode_VA to DllInfo-based
		if (CL_IsDevOverviewMode_VA)
		{
			ULONG CL_IsDevOverviewMode_RVA = 0;
			Convert_VA_to_RVA(CL_IsDevOverviewMode, RealDllInfo);
			Convert_RVA_to_VA(CL_IsDevOverviewMode, DllInfo);
		}

		typedef struct
		{
			ULONG_PTR& allow_cheats;
			const mh_dll_info_t& DllInfo;
		} CL_IsDevOverviewMode_SearchContext;

		CL_IsDevOverviewMode_SearchContext ctx = { allow_cheats_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)CL_IsDevOverviewMode_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (CL_IsDevOverviewMode_SearchContext*)context;

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{
				ctx->allow_cheats = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
			}

			if (ctx->allow_cheats)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(allow_cheats, DllInfo);

		if (allow_cheats_RVA)
			allow_cheats = (decltype(allow_cheats))VA_from_RVA(allow_cheats, RealDllInfo);

		Sig_VarNotFound(allow_cheats);
	}
	else
	{
		//GoldSrc doesn't have such "int *allow_cheats;"
	}
}

void R_FillAddress_R_DecalInit(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		decal_t *gDecalPool = NULL;
		decalcache_t *gDecalCache = NULL;
	*/
	ULONG_PTR gDecalPool_VA = 0;
	ULONG gDecalPool_RVA = 0;

	ULONG_PTR gDecalCache_VA = 0;
	ULONG gDecalCache_RVA = 0;

	ULONG_PTR R_DecalInit_VA = 0;

	const char pattern[] = "\x68\x00\xC0\x01\x00\x6A\x00";
	ULONG_PTR addr = (ULONG_PTR)Search_Pattern(pattern, DllInfo);

	if (addr)
	{
		R_DecalInit_VA = addr;

		typedef struct
		{
			ULONG_PTR& gDecalPool;
			ULONG_PTR& gDecalCache;
			const mh_dll_info_t& DllInfo;
		} R_DecalInit_SearchContext;

		R_DecalInit_SearchContext ctx = { gDecalPool_VA, gDecalCache_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)R_DecalInit_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_DecalInit_SearchContext*)context;

				if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//68 B8 5C 32 02 push    offset gDecalPool

					ctx->gDecalPool = (ULONG_PTR)pinst->detail->x86.operands[0].imm;
				}
				else if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//.text:01D49DBE B8 C0 96 BB 02 mov     eax, offset gDecalCache

					ctx->gDecalCache = (ULONG_PTR)pinst->detail->x86.operands[1].imm;
				}

				if (ctx->gDecalPool && ctx->gDecalCache)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(gDecalPool, DllInfo);
		Convert_VA_to_RVA(gDecalCache, DllInfo);
	}

	Sig_AddrNotFound(R_DecalInit);

	if (gDecalPool_RVA)
		gDecalPool = (decltype(gDecalPool))VA_from_RVA(gDecalPool, RealDllInfo);
	if (gDecalCache_RVA)
		gDecalCache = (decltype(gDecalCache))VA_from_RVA(gDecalCache, RealDllInfo);

	Sig_VarNotFound(gDecalPool);
	Sig_VarNotFound(gDecalCache);
}

void R_FillAddress_R_RenderDynamicLightmaps(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_RenderDynamicLightmaps)
		return;

	ULONG_PTR R_RenderDynamicLightmaps_VA = 0;
	ULONG R_RenderDynamicLightmaps_RVA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_RenderDynamicLightmaps_VA = (ULONG_PTR)Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_SVENGINE, DllInfo);
		if (!R_RenderDynamicLightmaps_VA)
			R_RenderDynamicLightmaps_VA = (ULONG_PTR)Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_SVENGINE_10152, DllInfo);
		Convert_VA_to_RVA(R_RenderDynamicLightmaps, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_RenderDynamicLightmaps_VA = (ULONG_PTR)Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_HL25, DllInfo);
		Convert_VA_to_RVA(R_RenderDynamicLightmaps, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_RenderDynamicLightmaps_VA = (ULONG_PTR)Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_NEW, DllInfo);
		if (!R_RenderDynamicLightmaps_VA)
			R_RenderDynamicLightmaps_VA = (ULONG_PTR)Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_NEW2, DllInfo);
		Convert_VA_to_RVA(R_RenderDynamicLightmaps, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_RenderDynamicLightmaps_VA = (ULONG_PTR)Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_BLOB, DllInfo);
		Convert_VA_to_RVA(R_RenderDynamicLightmaps, DllInfo);
	}

	if (R_RenderDynamicLightmaps_RVA)
	{
		gPrivateFuncs.R_RenderDynamicLightmaps = (decltype(gPrivateFuncs.R_RenderDynamicLightmaps))VA_from_RVA(R_RenderDynamicLightmaps, RealDllInfo);
	}

	Sig_FuncNotFound(R_RenderDynamicLightmaps);

	/*
		 int *d_lightstylevalue = NULL;
		 int *lightmap_modified = NULL;
		 glpoly_t **lightmap_polys = NULL;
	 */
	ULONG_PTR d_lightstylevalue_VA = 0;
	ULONG d_lightstylevalue_RVA = 0;

	ULONG_PTR lightmap_modified_VA = 0;
	ULONG lightmap_modified_RVA = 0;

	ULONG_PTR lightmap_polys_VA = 0;
	ULONG lightmap_polys_RVA = 0;

	{

		typedef struct
		{
			ULONG_PTR& d_lightstylevalue;
			ULONG_PTR& lightmap_polys;
			ULONG_PTR& lightmap_modified;
			const mh_dll_info_t& DllInfo;

			int cmp_al_FF_instcount{};
		} R_RenderDynamicLightmaps_SearchContext;

		R_RenderDynamicLightmaps_SearchContext ctx = { d_lightstylevalue_VA, lightmap_polys_VA, lightmap_modified_VA, DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)R_RenderDynamicLightmaps_VA, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_RenderDynamicLightmaps_SearchContext*)context;

				if (!ctx->lightmap_polys &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.index != 0 &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.scale == 4 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].reg == X86_REG_EAX)
				{//.text:01D58422 89 04 8D C8 B8 F5 03 mov     lightmap_polys[ecx*4], eax

					ctx->lightmap_polys = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}
				else if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_AL &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0xFF)
				{//.text:01D47F46 3C FF   cmp     al, 0FFh

					ctx->cmp_al_FF_instcount = instCount;
				}
				else if (!ctx->d_lightstylevalue &&
					ctx->cmp_al_FF_instcount &&
					instCount < ctx->cmp_al_FF_instcount + 5 &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.index != 0 &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					pinst->detail->x86.operands[1].mem.scale == 4 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//.text:01D47F4F 8B 04 85 20 9A BC 02                                mov     eax, d_lightstylevalue[eax*4]

					ctx->d_lightstylevalue = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
				}
				else if (!ctx->lightmap_modified &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.index != 0 &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[0].mem.scale == 4 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 1)
				{//.text:01D58489 C7 04 85 C8 C8 F5 03 01 00 00 00                    mov     lightmap_modified[eax*4], 1

					ctx->lightmap_modified = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
				}

				if (ctx->d_lightstylevalue && ctx->lightmap_polys && ctx->lightmap_modified)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		Convert_VA_to_RVA(d_lightstylevalue, DllInfo);
		Convert_VA_to_RVA(lightmap_polys, DllInfo);
		Convert_VA_to_RVA(lightmap_modified, DllInfo);
	}

	if (d_lightstylevalue_RVA)
		d_lightstylevalue = (decltype(d_lightstylevalue))VA_from_RVA(d_lightstylevalue, RealDllInfo);
	if (lightmap_polys_RVA)
		lightmap_polys = (decltype(lightmap_polys))VA_from_RVA(lightmap_polys, RealDllInfo);
	if (lightmap_modified_RVA)
		lightmap_modified = (decltype(lightmap_modified))VA_from_RVA(lightmap_modified, RealDllInfo);

	Sig_VarNotFound(d_lightstylevalue);
	Sig_VarNotFound(lightmap_polys);
	Sig_VarNotFound(lightmap_modified);
}

void R_FillAddress(void)
{
	ULONG_PTR addr;

	auto hSDL2 = GetModuleHandleA("SDL2.dll");

	if (hSDL2)
	{
		gPrivateFuncs.SDL_GL_SetAttribute = (decltype(gPrivateFuncs.SDL_GL_SetAttribute))GetProcAddress(hSDL2, "SDL_GL_SetAttribute");
	}

	R_FillAddress_EngineSurface(g_EngineDLLInfo);

	gPrivateFuncs.triapi_RenderMode = gEngfuncs.pTriAPI->RenderMode;
	gPrivateFuncs.triapi_GetMatrix = gEngfuncs.pTriAPI->GetMatrix;
	gPrivateFuncs.triapi_BoxInPVS = gEngfuncs.pTriAPI->BoxInPVS;
	gPrivateFuncs.triapi_Fog = gEngfuncs.pTriAPI->Fog;
	//gPrivateFuncs.triapi_Color4f = gEngfuncs.pTriAPI->Color4f;

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_HasOfficialFBOSupport(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_HasOfficialFBOSupport(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_HasOfficialGLTexAllocSupport(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_HasOfficialGLTexAllocSupport(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_GL_Init(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_GL_Init(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_PolyBlend(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_PolyBlend(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_S_ExtraUpdate(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_S_ExtraUpdate(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_GL_Bind(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_GL_Bind(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_GL_SelectTexture(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_GL_SelectTexture(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_GL_LoadTexture2(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_GL_LoadTexture2(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_CullBox(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_CullBox(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_SetupFrame(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_SetupFrame(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_SetupGL(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_SetupGL(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_RenderView(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_RenderView(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_V_RenderView(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_V_RenderView(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_RenderScene(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_RenderScene(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_NewMap(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_NewMap(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_GL_BuildLightmaps(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_GL_BuildLightmaps(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_BuildLightMap(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_BuildLightMap(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_AddDynamicLights(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_AddDynamicLights(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_GL_DisableMultitexture(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_GL_DisableMultitexture(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_GL_EnableMultitexture(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_GL_EnableMultitexture(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_DrawSequentialPoly(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_DrawSequentialPoly(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_TextureAnimation(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_TextureAnimation(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_DrawBrushModel(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_DrawBrushModel(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_RecursiveWorldNode(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_RecursiveWorldNode(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_DrawWorld(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_DrawWorld(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_DrawViewModel(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_DrawViewModel(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_MarkLeaves(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_MarkLeaves(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_GL_BeginRendering(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_GL_BeginRendering(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_GL_EndRendering(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_GL_EndRendering(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_EmitWaterPolys(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_EmitWaterPolys(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_VID_UpdateWindowVars(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_VID_UpdateWindowVars(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_Mod_PointInLeaf(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_Mod_PointInLeaf(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_DrawTEntitiesOnList(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_DrawTEntitiesOnList(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_BuildGammaTable(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_BuildGammaTable(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_DrawParticles(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_DrawParticles(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_CL_AllocDlight(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_CL_AllocDlight(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_GLStudioDrawPoints(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_GLStudioDrawPoints(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_StudioLighting(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_StudioLighting(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_StudioChrome(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_StudioChrome(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_LightLambert(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_LightLambert(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_StudioSetupSkin(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_StudioSetupSkin(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_Cache_Alloc(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_Cache_Alloc(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_Draw_MiptexTexture(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_Draw_MiptexTexture(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_Draw_DecalTexture(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_Draw_DecalTexture(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_DrawSpriteModel(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_DrawSpriteModel(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_LightStrength(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_LightStrength(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_RotateForEntity(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_RotateForEntity(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_GlowBlend(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_GlowBlend(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_SCR_BeginLoadingPlaque(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_SCR_BeginLoadingPlaque(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_Host_IsSinglePlayerGame(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_Host_IsSinglePlayerGame(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_Mod_UnloadSpriteTextures(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_Mod_UnloadSpriteTextures(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_Mod_LoadSpriteModel(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_Mod_LoadSpriteModel(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_Mod_LoadSpriteFrame(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_Mod_LoadSpriteFrame(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_AddTEntity(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_AddTEntity(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_Hunk_AllocName(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_Hunk_AllocName(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_GL_EndRenderingVars(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_GL_EndRenderingVars(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_VisEdicts(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_VisEdicts(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_AllocTransObjectsVars(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_AllocTransObjectsVars(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_DrawTEntitiesOnListVars(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_DrawTEntitiesOnListVars(g_EngineDLLInfo, g_EngineDLLInfo);
	}


	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_RecursiveWorldNodeVars(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_RecursiveWorldNodeVars(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_LoadSkybox(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_LoadSkybox(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_GL_FilterMinMaxVars(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_GL_FilterMinMaxVars(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_ScrFov(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_ScrFov(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	//Got CL_IsDevOverviewMode, CL_SetDevOverView and refdef here
	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_RenderSceneVars(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_RenderSceneVars(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	//Got ClientDLL_DrawNormalTriangles_VA, cl_waterlevel and gDevOverview here
	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_RenderSceneVars2(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_RenderSceneVars2(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_CL_IsDevOverviewModeVars(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_CL_IsDevOverviewModeVars(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_DecalInit(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_DecalInit(g_EngineDLLInfo, g_EngineDLLInfo);
	}

	if (g_MirroredEngineDLLInfo.ImageBase)
	{
		R_FillAddress_R_RenderDynamicLightmaps(g_MirroredEngineDLLInfo, g_EngineDLLInfo);
	}
	else
	{
		R_FillAddress_R_RenderDynamicLightmaps(g_EngineDLLInfo, g_EngineDLLInfo);
	}

#if 0
	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define NORMALINDEX_SIG_SVENGINE "\x83\x3C\xB5\x2A\x2A\x2A\x2A\x00"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.R_GLStudioDrawPoints, 0x600, NORMALINDEX_SIG_SVENGINE, sizeof(NORMALINDEX_SIG_SVENGINE) - 1);
		Sig_AddrNotFound(g_NormalIndex);
		g_NormalIndex = *(decltype(g_NormalIndex)*)(addr + 3);
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
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.BuildNormalIndexTable, 0x50, G_NORMALINDEX_SIG, sizeof(G_NORMALINDEX_SIG) - 1);
		Sig_AddrNotFound(g_NormalIndex);
		g_NormalIndex = *(int(**)[MAXSTUDIOVERTS])(addr + 1);
	}
#endif

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CHROMEAGE_SIG_SVENGINE "\xBF\x2A\x2A\x2A\x2A\xF3\xAB\x33\xFF\x39"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.R_GLStudioDrawPoints, 0x600, CHROMEAGE_SIG_SVENGINE, sizeof(CHROMEAGE_SIG_SVENGINE) - 1);
		Sig_AddrNotFound(chromeage);
		chromeage = *(decltype(chromeage)*)(addr + 1);
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
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.R_StudioChrome, 0x50, CHROMEAGE_SIG, sizeof(CHROMEAGE_SIG) - 1);
		Sig_AddrNotFound(chromeage);
		chromeage = *(int(**)[MAXSTUDIOBONES])((DWORD)addr + 3);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CHROME_SIG_SVENGINE "\xC1\xE8\x1F\x03\xC2\x8D\x04\xC5\x2A\x2A\x2A\x2A\x50\xE8"
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.R_GLStudioDrawPoints, 0x1000, CHROME_SIG_SVENGINE, sizeof(CHROME_SIG_SVENGINE) - 1);
		Sig_AddrNotFound(chrome);
		chrome = *(decltype(chrome)*)(addr + 8);
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
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.R_GLStudioDrawPoints, 0x600, CHROME_SIG_NEW, sizeof(CHROME_SIG_NEW) - 1);
		Sig_AddrNotFound(chrome);
		chrome = *(decltype(chrome)*)(addr + 3);
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
			}, 0, &ctx);

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

	if (1)
	{
		//Seach "CL_Reallocate cl_entities"
		const char sigs1[] = "CL_Reallocate cl_entities\n";
		auto CL_Reallocate_String = Search_Pattern_Data(sigs1);
		if (!CL_Reallocate_String)
			CL_Reallocate_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(CL_Reallocate_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
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

			if (Candidate[0] == 0xFF &&
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

				if (!cl_max_edicts && pinst->id == X86_INS_IMUL &&
					pinst->detail->x86.op_count == 3 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[2].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					cl_max_edicts = (decltype(cl_max_edicts))pinst->detail->x86.operands[1].mem.disp;
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
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#if 0
#define CL_MAXEDICTS_SIG_SVENGINE "\x69\xC0\xB8\x0B\x00\x00\x50\xE8\x2A\x2A\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A\xA3\x2A\x2A\x2A\x2A\xE8"
		DWORD addr = (DWORD)Search_Pattern(CL_MAXEDICTS_SIG_SVENGINE);
		Sig_AddrNotFound(cl_max_edicts);
		cl_max_edicts = *(decltype(cl_max_edicts)*)(addr + 14);
		cl_entities = *(decltype(cl_entities)*)(addr + 19);
#endif
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
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.GL_EnableMultitexture, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

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

		int SearchLength = 0x800;
		if (R_RenderScene_inlined)
			SearchLength = 0x1000;

		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)gPrivateFuncs.R_RenderView_SvEngine, SearchLength, sigs);
		Sig_AddrNotFound(c_brush_polys);
		c_alias_polys = *(int**)(addr + 2);
		c_brush_polys = *(int**)(addr + 14);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		const char sigs[] = "\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)gPrivateFuncs.R_RenderView, 0x100, sigs);
		Sig_AddrNotFound(c_brush_polys);
		c_brush_polys = *(int**)(addr + 2);
		c_alias_polys = *(int**)(addr + 12);
	}
	else
	{
		const char sigs[] = "\xA1\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x50\x51";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)gPrivateFuncs.R_RenderView, 0x150, sigs);
		Sig_AddrNotFound(c_brush_polys);
		c_alias_polys = *(int**)(addr + 1);
		c_brush_polys = *(int**)(addr + 7);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "\xF6\xC4\x44\x0F\x2A\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F";
		addr = (ULONG_PTR)Search_Pattern(sigs);
		Sig_AddrNotFound(envmap);
		envmap = *(int**)(addr + 11);
		cl_stats = *(int**)(addr + 24);

		//SvEngine 10182, R_PreDrawViewModel not inlined
		const char sigs3[] = "\xD9\x2A\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x89\x81\xDC\x02\x00\x00";
		addr = (ULONG_PTR)Search_Pattern(sigs3);
		Sig_AddrNotFound(cl_weaponstarttime);
		cl_weaponstarttime = *(float**)(addr + 2);
		cl_weaponsequence = *(int**)(addr + 7);

		const char sigs4[] = "\xD1\xEA\x89\x15\x2A\x2A\x2A\x2A";
		addr = (ULONG_PTR)Search_Pattern_From_Size(addr, 0x300, sigs4);
		Sig_AddrNotFound(cl_light_level);
		cl_light_level = *(int**)(addr + 4);
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
		envmap = *(int**)(addr + 2);

		const char sigs2[] = "\x39\x3D\x2A\x2A\x2A\x2A\x0F\x2A\x2A\x2A\x2A\x2A\xA1";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x50, sigs2);
		Sig_AddrNotFound(cl_stats);
		cl_stats = *(int**)(addr + 2);

		const char sigs3[] = "\xD9\x1D\x2A\x2A\x2A\x2A\xA1";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x500, sigs3);
		Sig_AddrNotFound(cl_weaponstarttime);
		cl_weaponstarttime = *(float**)(addr + 2);
		cl_weaponsequence = *(int**)(addr + 7);

		const char sigs4[] = "\x89\x15\x2A\x2A\x2A\x2A\x89\x2A\x2A\x2A\x2A\x2A\xFF";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x600, sigs4);
		Sig_AddrNotFound(cl_light_level);
		cl_light_level = *(int**)(addr + 2);

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

	}

	if (0)
	{
		//68 00 03 00 00                                      push    300h


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
			int candidate_E8_instCount;
			int instCount_push300h;
		}R_StudioSetupSkin_ctx;

		R_StudioSetupSkin_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_StudioSetupSkin, 0x800, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (R_StudioSetupSkin_ctx*)context;

				if (address[0] == 0xE8 && instLen == 5)
				{
					ctx->candidate_E8 = (decltype(ctx->candidate_E8))pinst->detail->x86.operands[0].imm;
					ctx->candidate_E8_instCount = instCount;

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
					if (instCount > ctx->candidate_E8_instCount && instCount < ctx->candidate_E8_instCount + 10)
					{
						//.text:01D8B20C FF B7 20 01 00 00                                   push    dword ptr [edi+120h]
						if (pinst->id == X86_INS_PUSH &&
							pinst->detail->x86.op_count == 1 &&
							pinst->detail->x86.operands[0].type == X86_OP_MEM &&
							pinst->detail->x86.operands[0].mem.base &&
							pinst->detail->x86.operands[0].mem.disp == 0x120)
						{
							gPrivateFuncs.GL_UnloadTexture = (decltype(gPrivateFuncs.GL_UnloadTexture))ctx->candidate_E8;
						}
						else if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_REG &&
							pinst->detail->x86.operands[1].type == X86_OP_MEM &&
							pinst->detail->x86.operands[1].mem.base &&
							pinst->detail->x86.operands[1].mem.disp == 0x120)
						{
							gPrivateFuncs.GL_UnloadTexture = (decltype(gPrivateFuncs.GL_UnloadTexture))ctx->candidate_E8;
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
						(ULONG_PTR)pinst->detail->x86.operands[0].imm >(ULONG_PTR)g_dwEngineBase &&
						(ULONG_PTR)pinst->detail->x86.operands[0].imm < (ULONG_PTR)g_dwEngineBase + (ULONG_PTR)g_dwEngineSize)
					{
						tmp_palette = (decltype(tmp_palette)*)pinst->detail->x86.operands[0].imm;
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

		Sig_FuncNotFound(R_StudioGetSkin);
		Sig_FuncNotFound(GL_UnloadTexture);
		Sig_VarNotFound(tmp_palette);
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

	if (1)
	{
#define MOD_KNOWN_SIG "\xB8\x9D\x82\x97\x53\x81\xE9"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(MOD_KNOWN_SIG);
		Sig_AddrNotFound(mod_known);
		mod_known = *(model_t**)(addr + 7);
	}

	if (1)
	{
		const char sigs1[] = "Cached models:\n";
		auto Mod_Print_String = Search_Pattern_Data(sigs1);
		if (!Mod_Print_String)
			Mod_Print_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Mod_Print_String);
		char pattern[] = "\x57\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern + 2) = (DWORD)Mod_Print_String;
		auto Mod_Print_Call = Search_Pattern(pattern);
		Sig_VarNotFound(Mod_Print_Call);

		g_pMetaHookAPI->DisasmRanges(Mod_Print_Call, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

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

	if (1)
	{
		const char sigs1[] = "bogus\0";
		auto Bogus_String = Search_Pattern_Data(sigs1);
		if (!Bogus_String)
			Bogus_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Bogus_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)Bogus_String;
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
				Candidate[8] == 0x00)
			{
				return TRUE;
			}

			return FALSE;
			});
		Sig_FuncNotFound(Mod_LoadStudioModel);
	}

	if (1)
	{
		const char sigs1[] = "Mod_LoadBrushModel: %s has wrong version number";
		auto Bogus_String = Search_Pattern_Data(sigs1);
		if (!Bogus_String)
			Bogus_String = Search_Pattern_Rdata(sigs1);
		Sig_VarNotFound(Bogus_String);
		char pattern[] = "\x68\x2A\x2A\x2A\x2A\x6A\x01\xE8";
		*(DWORD*)(pattern + 1) = (DWORD)Bogus_String;
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

	/*
	 //Global popinters that link into engine vars.
		char (*loadname)[64] = NULL;
		model_t **loadmodel = NULL;
	*/

	if (1)
	{
		PVOID Mod_LoadModel_String = NULL;

		if (g_iEngineType == ENGINE_SVENGINE)
		{
			const char sigs[] = "Loading '%s'\n";
			Mod_LoadModel_String = Search_Pattern_Data(MOD_LOADMODEL_STRING_SVENGINE);
			if (!Mod_LoadModel_String)
				Mod_LoadModel_String = Search_Pattern_Rdata(MOD_LOADMODEL_STRING_SVENGINE);
		}
		else
		{
			const char sigs[] = "loading %s\n";
			Mod_LoadModel_String = Search_Pattern_Data(MOD_LOADMODEL_STRING);
			if (!Mod_LoadModel_String)
				Mod_LoadModel_String = Search_Pattern_Rdata(MOD_LOADMODEL_STRING);
		}

		Sig_VarNotFound(Mod_LoadModel_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
		*(DWORD*)(pattern + 1) = (DWORD)Mod_LoadModel_String;
		auto Mod_LoadModel_PushString = Search_Pattern(pattern);
		Sig_VarNotFound(Mod_LoadModel_PushString);

		gPrivateFuncs.Mod_LoadModel = (decltype(gPrivateFuncs.Mod_LoadModel))g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Mod_LoadModel_PushString, 0x600, [](PUCHAR Candidate) {

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

		typedef struct
		{
			PVOID loadname_nextaddr{};
		}Mod_LoadModel_SearchContext;

		Mod_LoadModel_SearchContext ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges((PUCHAR)Mod_LoadModel_PushString + 5, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;
				auto ctx = (Mod_LoadModel_SearchContext*)context;

				if (!loadname &&
					pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_IMM &&
					(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
				{
					loadname = (decltype(loadname))pinst->detail->x86.operands[0].imm;
					ctx->loadname_nextaddr = address + instLen;
				}

				if (loadname)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

			}, 0, &ctx);

		Sig_VarNotFound(loadname);

		g_pMetaHookAPI->DisasmRanges(ctx.loadname_nextaddr, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

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

		host_basepal = (decltype(host_basepal)) * (void**)((PUCHAR)palette_Call + sizeof(pattern) - 1);
		Sig_VarNotFound(host_basepal);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.Draw_DecalTexture, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

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
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_DrawParticles, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

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
					else if (address[instLen] == 0x33 && address[instLen + 1] == 0xC4)
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
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.R_DrawParticles, 0x800, R_TRACERDRAW_SIG, sizeof(R_TRACERDRAW_SIG) - 1);
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
		gDevOverview = (decltype(gDevOverview))(*(DWORD*)(addr + 9) - 0xC);
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
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_MarkLeaves, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto pinst = (cs_insn*)inst;

				if (!r_viewleaf &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_ECX &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					pinst->detail->x86.operands[1].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
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
		vup = (vec_t*)(*(DWORD*)(addr + 1));
		vright = (vec_t*)(*(DWORD*)(addr + 18));
		vpn = (vec_t*)(*(DWORD*)(addr + 23));
	}
	else
	{
		const char sigs[] = "\x40\x68\x2A\x2A\x2A\x2A\xA3\x2A\x2A\x2A\x2A\xA1";
		addr = (DWORD)g_pMetaHookAPI->SearchPattern((void*)gPrivateFuncs.R_SetupFrame, 0x300, sigs, sizeof(sigs) - 1);
		Sig_AddrNotFound(vright);
		addr += 2;
		vup = (vec_t*)(*(DWORD*)addr);
		addr += 10;
		addr += 5;
		vright = (vec_t*)(*(DWORD*)addr);
		addr += 5;
		vpn = (vec_t*)(*(DWORD*)addr);
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
		*(DWORD*)(pattern + 1) = (DWORD)UrlInfo_String;
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
		typedef struct
		{
			int MovClsStateInstCount;
			int FldzInstCount;
			int ZeroizedRegister[3];
			int ZeroizedRegisterCount;
			PVOID ZeroizedCandidate[6];
			int ZeroizedCandidateCount;
		}V_RenderView_ctx;

		V_RenderView_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.V_RenderView, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (V_RenderView_ctx*)context;

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
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 5)
			{
				//83 3D 30 9A 09 02 05                                cmp     cls_state, 5
				cls_state = (decltype(cls_state))pinst->detail->x86.operands[0].mem.disp;
				ctx->MovClsStateInstCount = instCount;
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

			}, 0, &ctx);

		Sig_VarNotFound(cls_state);
		Sig_VarNotFound(cls_signon);

		if (ctx.ZeroizedCandidateCount == 6)
		{
			std::qsort(ctx.ZeroizedCandidate, ctx.ZeroizedCandidateCount, sizeof(ctx.ZeroizedCandidate[0]), [](const void* a, const void* b) {
				return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
				});

			r_soundOrigin = (decltype(r_soundOrigin))ctx.ZeroizedCandidate[0];
			r_playerViewportAngles = (decltype(r_playerViewportAngles))ctx.ZeroizedCandidate[3];
		}
		Sig_VarNotFound(r_soundOrigin);
		Sig_VarNotFound(r_playerViewportAngles);
	}

	if (1)
	{
		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.SCR_BeginLoadingPlaque, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;

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
			int Candidate_numlights_instCount;
			int Candidate_numlights_reg;
			PVOID Candidate_numlights;

			int Candidate_locallight2_InstCount;
			int Candidate_locallight2_reg;
			PVOID Candidate_locallight2;
		}R_LightStrength_Context;

		R_LightStrength_Context ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gPrivateFuncs.R_LightStrength, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto ctx = (R_LightStrength_Context*)context;
			auto pinst = (cs_insn*)inst;

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
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)g_dwEngineDataBase &&
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
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
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

			if (!numlights &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
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
				numlights = (decltype(numlights))ctx->Candidate_numlights;
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
				numlights = (decltype(numlights))ctx->Candidate_numlights;
			}

			if (locallight && numlights)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);

		Sig_VarNotFound(locallight);
		Sig_VarNotFound(numlights);
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
			int Candidate_numlights_instCount;
			int Candidate_numlights_reg;
			PVOID Candidate_numlights;
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

				if (!numlights &&
					pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					pinst->detail->x86.operands[1].mem.index == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize &&
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
					numlights = (decltype(numlights))ctx->Candidate_numlights;
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
		Sig_VarNotFound(numlights);
	}

	g_pMetaHookAPI->DisasmRanges(gEngfuncs.pfnSetFilterMode, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;

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

	g_pMetaHookAPI->DisasmRanges(gEngfuncs.pfnSetFilterColor, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto ctx = (FilterColorCotext_t*)context;
		auto pinst = (cs_insn*)inst;

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

	std::qsort(ctx.Candidates, ctx.CandidateCount, sizeof(ctx.Candidates[0]), [](const void* a, const void* b) {
		return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
		});

	filterColorRed = (decltype(filterColorRed))ctx.Candidates[0];
	filterColorGreen = (decltype(filterColorGreen))ctx.Candidates[1];
	filterColorBlue = (decltype(filterColorBlue))ctx.Candidates[2];

	Sig_VarNotFound(filterColorRed);
	Sig_VarNotFound(filterColorGreen);
	Sig_VarNotFound(filterColorBlue);

	g_pMetaHookAPI->DisasmRanges(gEngfuncs.pfnSetFilterBrightness, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;

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

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		/*
.text:01D56BC6 6A 00                                               push    0
.text:01D56BC8 68 14 66 E6 01                                      push    offset aMissing ; "**missing**"
.text:01D56BCD E8 AE 80 FF FF                                      call    GL_LoadTexture
		*/
		const char missing_Pattern[] = "**missing**\0";
		auto Missing_String = Search_Pattern_NoWildCard_Data(missing_Pattern);
		if (!Missing_String)
			Missing_String = Search_Pattern_NoWildCard_Rdata(missing_Pattern);
		if (Missing_String)
		{
			char pattern[] = "\x6A\x00\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 3) = (DWORD)Missing_String;
			auto Missing_Call = Search_Pattern(pattern);
			if (Missing_Call)
			{
				g_pMetaHookAPI->DisasmRanges((PUCHAR)Missing_Call + Sig_Length(pattern), 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
					{
						r_missingtexture = (decltype(r_missingtexture))pinst->detail->x86.operands[1].mem.disp;
					}

					if (r_missingtexture)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
					}, 0, NULL);
			}
		}
		Sig_VarNotFound(r_missingtexture);
	}

	if (1)
	{
		/*
.text:01D56BC6 6A 00                                               push    0
.text:01D56BC8 68 14 66 E6 01                                      push    offset aMissing ; "**empty**"
.text:01D56BCD E8 AE 80 FF FF                                      call    GL_LoadTexture
		*/
		const char empty_Pattern[] = "**empty**\0";
		auto Empty_String = Search_Pattern_NoWildCard_Data(empty_Pattern);
		if (!Empty_String)
			Empty_String = Search_Pattern_NoWildCard_Rdata(empty_Pattern);
		if (Empty_String)
		{
			char pattern[] = "\x6A\x00\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
			*(DWORD*)(pattern + 3) = (DWORD)Empty_String;
			auto Empty_Call = Search_Pattern(pattern);
			if (Empty_Call)
			{
				g_pMetaHookAPI->DisasmRanges((PUCHAR)Empty_Call + Sig_Length(pattern), 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;

					if (pinst->id == X86_INS_MOV &&
						pinst->detail->x86.op_count == 2 &&
						pinst->detail->x86.operands[0].type == X86_OP_REG &&
						pinst->detail->x86.operands[1].type == X86_OP_MEM &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwEngineDataBase &&
						(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwEngineDataBase + g_dwEngineDataSize)
					{
						r_notexture_mip = (decltype(r_notexture_mip))pinst->detail->x86.operands[1].mem.disp;
					}

					if (r_notexture_mip)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
					}, 0, NULL);
			}
		}
		Sig_VarNotFound(r_notexture_mip);
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
				(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)g_dwEngineDataBase &&
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
				(PUCHAR)pinst->detail->x86.operands[1].imm >(PUCHAR)g_dwEngineDataBase &&
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
				pinst->detail->x86.operands[1].imm == 0)
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

static hook_t* g_phook_GL_Init = NULL;
static hook_t* g_phook_GL_BeginRendering = NULL;
static hook_t* g_phook_GL_EndRendering = NULL;
static hook_t* g_phook_R_RenderView_SvEngine = NULL;
static hook_t* g_phook_R_RenderView = NULL;
static hook_t* g_phook_R_LoadSkyBox_SvEngine = NULL;
static hook_t* g_phook_R_LoadSkys = NULL;
static hook_t* g_phook_R_NewMap = NULL;
static hook_t* g_phook_R_CullBox = NULL;
static hook_t* g_phook_Mod_PointInLeaf = NULL;
static hook_t* g_phook_R_BuildLightMap = NULL;
static hook_t* g_phook_R_AddDynamicLights = NULL;
static hook_t* g_phook_R_GLStudioDrawPoints = NULL;
static hook_t* g_phook_GL_UnloadTextures = NULL;
static hook_t* g_phook_GL_UnloadTexture = NULL;
static hook_t* g_phook_GL_LoadTexture2 = NULL;
static hook_t* g_phook_GL_BuildLightmaps = NULL;
static hook_t* g_phook_enginesurface_createNewTextureID = NULL;
static hook_t* g_phook_enginesurface_drawSetTextureFile = NULL;
static hook_t* g_phook_enginesurface_drawFlushText = NULL;
static hook_t* g_phook_Mod_LoadStudioModel = NULL;
static hook_t* g_phook_Mod_LoadBrushModel = NULL;
static hook_t* g_phook_Mod_LoadSpriteModel = NULL;
static hook_t* g_phook_Mod_UnloadSpriteTextures = NULL;
static hook_t* g_phook_triapi_RenderMode = NULL;
static hook_t* g_phook_triapi_BoxInPVS = NULL;
static hook_t* g_phook_triapi_Fog = NULL;
static hook_t* g_phook_triapi_GetMatrix = NULL;
//static hook_t *g_phook_triapi_Color4f = NULL;
static hook_t* g_phook_Draw_MiptexTexture = NULL;
static hook_t* g_phook_BuildGammaTable = NULL;
static hook_t* g_phook_DLL_SetModKey = NULL;
static hook_t* g_phook_SDL_GL_SetAttribute = NULL;

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

	if (!g_bHasOfficialGLTexAllocSupport)
	{
		Uninstall_Hook(enginesurface_createNewTextureID);
	}

	Uninstall_Hook(enginesurface_drawSetTextureFile);
	Uninstall_Hook(enginesurface_drawFlushText);
	Uninstall_Hook(Mod_LoadStudioModel);
	Uninstall_Hook(Mod_LoadSpriteModel);
	Uninstall_Hook(Mod_UnloadSpriteTextures);
	Uninstall_Hook(triapi_RenderMode);
	Uninstall_Hook(triapi_BoxInPVS);
	//Uninstall_Hook(triapi_Fog);
	Uninstall_Hook(triapi_GetMatrix);
	Uninstall_Hook(Draw_MiptexTexture);
	Uninstall_Hook(BuildGammaTable);
	Uninstall_Hook(R_CullBox);

	if (gPrivateFuncs.SDL_GL_SetAttribute)
	{
		Uninstall_Hook(SDL_GL_SetAttribute);
	}
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

	if (!g_bHasOfficialGLTexAllocSupport)
	{
		Install_InlineHook(enginesurface_createNewTextureID);
	}

	Install_InlineHook(enginesurface_drawSetTextureFile);
	Install_InlineHook(enginesurface_drawFlushText);
	Install_InlineHook(Mod_LoadStudioModel);
	Install_InlineHook(Mod_LoadSpriteModel);
	Install_InlineHook(Mod_UnloadSpriteTextures);
	Install_InlineHook(triapi_RenderMode);
	Install_InlineHook(triapi_BoxInPVS);
	//Install_InlineHook(triapi_Fog);
	Install_InlineHook(triapi_GetMatrix);
	Install_InlineHook(Draw_MiptexTexture);
	Install_InlineHook(BuildGammaTable);
	Install_InlineHook(R_CullBox);

	//OpenGL4.2 was forced by HL25 engine which might ruin the renderer features.
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
	if (g_bHasOfficialGLTexAllocSupport)
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

void R_PatchResetLatched(void)
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		return;

	const char pattern[] = "\x6A\x01\x2A\x2A\x2A\x08\x03\x00\x00";

	PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
	PUCHAR SearchLimit = (PUCHAR)g_dwEngineTextBase + g_dwEngineTextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			typedef struct
			{
				bool bFoundMov308h;
				bool bFoundResetLatched;
			}PatchResetLatchedContext;

			PatchResetLatchedContext ctx = { 0 };

			g_pMetaHookAPI->DisasmRanges(pFound, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (PatchResetLatchedContext*)context;

				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0 &&
					pinst->detail->x86.operands[0].mem.disp == 0x308 &&
					(pinst->detail->x86.operands[1].type == X86_OP_REG || pinst->detail->x86.operands[1].type == X86_OP_IMM))
				{
					ctx->bFoundMov308h = true;
					return FALSE;
				}

				if (ctx->bFoundMov308h && address[0] == 0xE8 && instLen == 5)
				{
					ctx->bFoundResetLatched = true;
					gPrivateFuncs.R_ResetLatched = (decltype(gPrivateFuncs.R_ResetLatched))GetCallAddress(address);

					g_pMetaHookAPI->InlinePatchRedirectBranch(address, R_ResetLatched_Patched, NULL);

					return TRUE;
				}

				if (ctx->bFoundResetLatched)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;

				}, 0, &ctx);

			if (ctx.bFoundResetLatched)
			{
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

void Client_FillAddress(void)
{

}