
#include <metahook.h>
#include <capstone.h>
#include <set>
#include <map>
#include "gl_local.h"

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
#define R_SETUPFRAME_SIG_NEW2 R_SETUPFRAME_SIG_BLOB2
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

//xref string "Failed to load custom decal for player"
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

void Engine_FillAddress_EngineSurface(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto engineFactory = g_pMetaHookAPI->GetEngineFactory();

	if (engineFactory)
	{
#define ENGINE_SURFACE_VERSION "EngineSurface007"
		void* engineSurface = (void*)engineFactory(ENGINE_SURFACE_VERSION, NULL);

		auto engineSurface_vftable = *(PVOID**)engineSurface;

		int index_drawSetTextureRGBA = 8;
		int index_drawSetTexture = 9;
		int index_createNewTextureID = 11;
		int index_drawSetTextureFile = 18;
		int index_isTextureIDValid = 20;
		int index_drawFlushText = 22;

		engineSurface_vftable = (decltype(engineSurface_vftable))ConvertDllInfoSpace(engineSurface_vftable, RealDllInfo, DllInfo);

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

		gPrivateFuncs.enginesurface_drawSetTextureRGBA = (decltype(gPrivateFuncs.enginesurface_drawSetTextureRGBA))		GetVFunctionFromVFTable(engineSurface_vftable, index_drawSetTextureRGBA,	DllInfo, RealDllInfo, RealDllInfo);
		gPrivateFuncs.enginesurface_drawSetTexture = (decltype(gPrivateFuncs.enginesurface_drawSetTexture))				GetVFunctionFromVFTable(engineSurface_vftable, index_drawSetTexture,		DllInfo, RealDllInfo, RealDllInfo);
		gPrivateFuncs.enginesurface_createNewTextureID = (decltype(gPrivateFuncs.enginesurface_createNewTextureID))		GetVFunctionFromVFTable(engineSurface_vftable, index_createNewTextureID,	DllInfo, RealDllInfo, RealDllInfo);
		gPrivateFuncs.enginesurface_drawSetTextureFile = (decltype(gPrivateFuncs.enginesurface_drawSetTextureFile))		GetVFunctionFromVFTable(engineSurface_vftable, index_drawSetTextureFile,	DllInfo, RealDllInfo, RealDllInfo);
		gPrivateFuncs.enginesurface_isTextureIDValid = (decltype(gPrivateFuncs.enginesurface_isTextureIDValid))			GetVFunctionFromVFTable(engineSurface_vftable, index_isTextureIDValid,		DllInfo, RealDllInfo, RealDllInfo);
		gPrivateFuncs.enginesurface_drawFlushText = (decltype(gPrivateFuncs.enginesurface_drawFlushText))				GetVFunctionFromVFTable(engineSurface_vftable, index_drawFlushText,			DllInfo, RealDllInfo, RealDllInfo);

		auto enginesurface_isTextureIDValid_VA = ConvertDllInfoSpace(gPrivateFuncs.enginesurface_isTextureIDValid, RealDllInfo, DllInfo);

		if (!enginesurface_isTextureIDValid_VA)
		{
			//Someone just hooked enginesurface_isTextureIDValid ?
			Sig_NotFound(enginesurface_isTextureIDValid_VA);
		}

		typedef struct enginesurface_isTextureIDValid_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			void* candicate{};
			int instCount{};
		}enginesurface_isTextureIDValid_SearchContext;

		enginesurface_isTextureIDValid_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(enginesurface_isTextureIDValid_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

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
			gPrivateFuncs.staticGetTextureById = (decltype(gPrivateFuncs.staticGetTextureById))ConvertDllInfoSpace(ctx.candicate, DllInfo, RealDllInfo);
		}
	}
}

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
					typedef struct GL_Init_SearchContext_s
					{
						const mh_dll_info_t& DllInfo;
						const mh_dll_info_t& RealDllInfo;
						bool bFoundPushString{};
					}GL_Init_SearchContext;

					GL_Init_SearchContext ctx = { DllInfo, RealDllInfo };

					g_pMetaHookAPI->DisasmRanges(pCandidateFunction, 0x120, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

						auto pinst = (cs_insn*)inst;
						auto ctx = (GL_Init_SearchContext*)context;

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
						gPrivateFuncs.GL_Init = (decltype(gPrivateFuncs.GL_Init))ConvertDllInfoSpace(pCandidateFunction, DllInfo, RealDllInfo);
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
}

void Engine_FillAddress_R_PolyBlend(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_PolyBlend)
		return;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		auto R_PolyBlend_VA = Search_Pattern(R_POLYBLEND_SVENGINE, DllInfo);
		gPrivateFuncs.R_PolyBlend = (decltype(gPrivateFuncs.R_PolyBlend))ConvertDllInfoSpace(R_PolyBlend_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		auto R_PolyBlend_VA = Search_Pattern(R_POLYBLEND_HL25, DllInfo);
		gPrivateFuncs.R_PolyBlend = (decltype(gPrivateFuncs.R_PolyBlend))ConvertDllInfoSpace(R_PolyBlend_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		auto R_PolyBlend_VA = Search_Pattern(R_POLYBLEND_NEW, DllInfo);

		if (!R_PolyBlend_VA)
			R_PolyBlend_VA = Search_Pattern(R_POLYBLEND_NEW2, DllInfo);

		gPrivateFuncs.R_PolyBlend = (decltype(gPrivateFuncs.R_PolyBlend))ConvertDllInfoSpace(R_PolyBlend_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		auto R_PolyBlend_VA = Search_Pattern(R_POLYBLEND_BLOB, DllInfo);
		gPrivateFuncs.R_PolyBlend = (decltype(gPrivateFuncs.R_PolyBlend))ConvertDllInfoSpace(R_PolyBlend_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(R_PolyBlend);
}

void Engine_FillAddress_S_ExtraUpdate(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.S_ExtraUpdate)
		return;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		auto S_ExtraUpdate_VA = Search_Pattern(S_EXTRAUPDATE_SVENGINE, DllInfo);
		gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))ConvertDllInfoSpace(S_ExtraUpdate_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		auto S_ExtraUpdate_VA = Search_Pattern(S_EXTRAUPDATE_HL25, DllInfo);
		gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))ConvertDllInfoSpace(S_ExtraUpdate_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		auto S_ExtraUpdate_VA = Search_Pattern(S_EXTRAUPDATE_NEW, DllInfo);

		if (!S_ExtraUpdate_VA)
			S_ExtraUpdate_VA = Search_Pattern(S_EXTRAUPDATE_NEW2, DllInfo);

		gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))ConvertDllInfoSpace(S_ExtraUpdate_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		auto S_ExtraUpdate_VA = Search_Pattern(S_EXTRAUPDATE_BLOB, DllInfo);
		gPrivateFuncs.S_ExtraUpdate = (decltype(gPrivateFuncs.S_ExtraUpdate))ConvertDllInfoSpace(S_ExtraUpdate_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(S_ExtraUpdate);
}

void Engine_FillAddress_GL_Bind(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_Bind)
		return;

	PVOID GL_Bind_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		GL_Bind_VA = Search_Pattern(GL_BIND_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))ConvertDllInfoSpace((PVOID)GL_Bind_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		GL_Bind_VA = Search_Pattern(GL_BIND_SIG_HL25, DllInfo);
		gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))ConvertDllInfoSpace((PVOID)GL_Bind_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		GL_Bind_VA = Search_Pattern(GL_BIND_SIG_NEW, DllInfo);

		//try another signature
		if (!GL_Bind_VA)
			GL_Bind_VA = Search_Pattern(GL_BIND_SIG_NEW2, DllInfo);

		gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))ConvertDllInfoSpace((PVOID)GL_Bind_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		GL_Bind_VA = Search_Pattern(GL_BIND_SIG_BLOB, DllInfo);
		gPrivateFuncs.GL_Bind = (decltype(gPrivateFuncs.GL_Bind))ConvertDllInfoSpace((PVOID)GL_Bind_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(GL_Bind);

	/*
		//Global pointers that link into engine vars.
		int *currenttexture = NULL;
	*/

	typedef struct
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} GL_BindVars_SearchContext;

	GL_BindVars_SearchContext ctx = { DllInfo, RealDllInfo };

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

				currenttexture = (decltype(currenttexture))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (currenttexture)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(currenttexture);
}

void Engine_FillAddress_GL_SelectTexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_SelectTexture)
		return;

	PVOID GL_SelectTexture_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		PVOID GL_Bind_VA = ConvertDllInfoSpace(gPrivateFuncs.GL_Bind, RealDllInfo, DllInfo);

		GL_SelectTexture_VA = Search_Pattern_From(GL_Bind_VA, GL_SELECTTEXTURE_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))ConvertDllInfoSpace((PVOID)GL_SelectTexture_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		PVOID GL_Bind_VA = ConvertDllInfoSpace(gPrivateFuncs.GL_Bind, RealDllInfo, DllInfo);

		GL_SelectTexture_VA = Search_Pattern_From(GL_Bind_VA, GL_SELECTTEXTURE_SIG_HL25, DllInfo);
		gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))ConvertDllInfoSpace((PVOID)GL_SelectTexture_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		GL_SelectTexture_VA = Search_Pattern(GL_SELECTTEXTURE_SIG_NEW, DllInfo);
		if(!GL_SelectTexture_VA)
			GL_SelectTexture_VA = Search_Pattern(GL_SELECTTEXTURE_SIG_NEW2, DllInfo);

		gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))ConvertDllInfoSpace((PVOID)GL_SelectTexture_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		GL_SelectTexture_VA = Search_Pattern(GL_SELECTTEXTURE_SIG_BLOB, DllInfo);
		gPrivateFuncs.GL_SelectTexture = (decltype(gPrivateFuncs.GL_SelectTexture))ConvertDllInfoSpace((PVOID)GL_SelectTexture_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(GL_SelectTexture);

	/*
		//Global pointers that link into engine vars.
		  int *oldtarget = NULL;
	*/

	typedef struct GL_SelectTextureVars_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} GL_SelectTextureVars_SearchContext;

	GL_SelectTextureVars_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)GL_SelectTexture_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
		auto pinst = (cs_insn*)inst;
		auto ctx = (GL_SelectTextureVars_SearchContext*)context;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].reg == X86_REG_ESI)
		{//.text:01D4FE37 89 35 08 82 ED 01 mov     oldtarget, esi

			oldtarget = (decltype(oldtarget))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (oldtarget)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, &ctx);

	Sig_VarNotFound(oldtarget);
}

void Engine_FillAddress_GL_LoadTexture2(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_LoadTexture2)
		return;

	PVOID GL_LoadTexture2_VA = 0;
	ULONG_PTR GL_LoadTexture2_RVA = 0;

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
				GL_LoadTexture2_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Error_Call_VA, 0x500, [](PUCHAR Candidate) {

					if (Candidate[0] == 0x51 &&
						Candidate[1] == 0x83)
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
				GL_LoadTexture2_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Error_Call_VA, 0x500, [](PUCHAR Candidate) {

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
			GL_LoadTexture2_VA = Search_Pattern(GL_LOADTEXTURE2_SIG_SVENGINE, DllInfo);
			Convert_VA_to_RVA(GL_LoadTexture2, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			GL_LoadTexture2_VA = Search_Pattern(GL_LOADTEXTURE2_SIG_HL25, DllInfo);
			Convert_VA_to_RVA(GL_LoadTexture2, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			GL_LoadTexture2_VA = Search_Pattern(GL_LOADTEXTURE2_SIG_NEW, DllInfo);
			Convert_VA_to_RVA(GL_LoadTexture2, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			GL_LoadTexture2_VA = Search_Pattern(GL_LOADTEXTURE2_SIG_BLOB, DllInfo);

			if (!GL_LoadTexture2_VA)
				GL_LoadTexture2_VA = Search_Pattern(GL_LOADTEXTURE2_SIG_BLOB2, DllInfo);

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

			numgltextures = (decltype(numgltextures))ConvertDllInfoSpace(numgltextures_VA, DllInfo, RealDllInfo);
			gltextures_SvEngine = (decltype(gltextures_SvEngine))ConvertDllInfoSpace(gltextures_SvEngine_VA, DllInfo, RealDllInfo);
		}

		{
			const char pattern2[] = "\x6B\xC1\x54\x89\x0D";
			//  GL_LoadTexture2
			//.text:01D4ED66 6B C1 54                                            imul    eax, ecx, 54h; 'T'
			//.text:01D4ED69 89 0D F0 C6 0F 03                                   mov     maxgltextures, ecx

			ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size((void*)GL_LoadTexture2_VA, 0x300, pattern2);
			Sig_AddrNotFound(maxgltextures_SvEngine);

			auto maxgltextures_SvEngine_VA = *(PVOID*)(addr + 5);
			maxgltextures_SvEngine = (decltype(maxgltextures_SvEngine))ConvertDllInfoSpace(maxgltextures_SvEngine_VA, DllInfo, RealDllInfo);
			Sig_VarNotFound(maxgltextures_SvEngine);

			const char pattern3[] = "\x51\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08";
			addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x50, pattern3);
			Sig_AddrNotFound(realloc_SvEngine);
			auto realloc_SvEngine_VA = GetCallAddress(addr + 1);
			gPrivateFuncs.realloc_SvEngine = (decltype(gPrivateFuncs.realloc_SvEngine))ConvertDllInfoSpace(realloc_SvEngine_VA, DllInfo, RealDllInfo);
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

			gHostSpawnCount = (decltype(gHostSpawnCount))ConvertDllInfoSpace(gHostSpawnCount_VA, DllInfo, RealDllInfo);
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

			peakgltextures_SvEngine = (decltype(peakgltextures_SvEngine))ConvertDllInfoSpace(peakgltextures_SvEngine_VA, DllInfo, RealDllInfo);
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

		GL_LoadTexture2_SearchContext ctx = { DllInfo, RealDllInfo };

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
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
				)
			{
				//  mov     edi, offset gltextures
				gltextures = (decltype(gltextures))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
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
			{
				//  mov     edi, offset gltextures
				gltextures = (decltype(gltextures))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->DllInfo, ctx->RealDllInfo);
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
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize
				)
			{
				//  mov     numgltextures, ecx
				if (!g_bHasOfficialGLTexAllocSupport &&
					instCount > ctx->mov_mem_exx_instCount && instCount < ctx->mov_mem_exx_instCount + 5 &&
					pinst->detail->x86.operands[1].reg == ctx->mov_mem_exx_reg)
				{
					allocated_textures = (decltype(allocated_textures))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}
				else
				{
					numgltextures = (decltype(numgltextures))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
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
			{
				//                        mov     ax, word ptr gHostSpawnCount
				//                        movzx   eax, word ptr gHostSpawnCount
				gHostSpawnCount = (decltype(gHostSpawnCount))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
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

	PVOID GL_LoadTexture2 = ConvertDllInfoSpace(gPrivateFuncs.GL_LoadTexture2, RealDllInfo, DllInfo);
	PVOID R_CullBox_VA = NULL;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_CullBox_VA = Search_Pattern_From(GL_LoadTexture2, R_CULLBOX_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))ConvertDllInfoSpace(R_CullBox_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_CullBox_VA = Search_Pattern_From(GL_LoadTexture2, R_CULLBOX_SIG_HL25, DllInfo);
		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))ConvertDllInfoSpace(R_CullBox_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_CullBox_VA = Search_Pattern_From(GL_LoadTexture2, R_CULLBOX_SIG_NEW, DllInfo);

		if (!R_CullBox_VA)
			R_CullBox_VA = Search_Pattern_From(GL_LoadTexture2, R_CULLBOX_SIG_NEW2, DllInfo);

		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))ConvertDllInfoSpace(R_CullBox_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_CullBox_VA = Search_Pattern_From(GL_LoadTexture2, R_CULLBOX_SIG_BLOB, DllInfo);
		gPrivateFuncs.R_CullBox = (decltype(gPrivateFuncs.R_CullBox))ConvertDllInfoSpace(R_CullBox_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(R_CullBox);

	/*
		mplane_t *frustum = NULL;
		vec_t *vup = NULL;
		vec_t *vpn = NULL;
		vec_t *vright = NULL;
	*/
	typedef struct R_CullBox_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} R_CullBox_SearchContext;

	R_CullBox_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(R_CullBox_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
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

				frustum = (decltype(frustum))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (frustum)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	if (frustum)
	{
		auto frustum_VA = ConvertDllInfoSpace(frustum, RealDllInfo, DllInfo);

		char pattern_Frustum[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8";
		*(DWORD*)(pattern_Frustum + 11) = (DWORD)frustum_VA;

		auto addr = (ULONG_PTR)Search_Pattern(pattern_Frustum, DllInfo);
		Sig_AddrNotFound(pattern_Frustum);

		auto vpn_VA = *(PVOID *)(addr + 1);
		auto vup_VA = *(PVOID*)(addr + 6);

		vpn = (decltype(vpn))ConvertDllInfoSpace(vpn_VA, DllInfo, RealDllInfo);
		vup = (decltype(vup))ConvertDllInfoSpace(vup_VA, DllInfo, RealDllInfo);

		char pattern_Frustum2[] = "\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern_Frustum2 + 1) = (DWORD)vpn_VA;
		*(DWORD*)(pattern_Frustum2 + 11) = (DWORD)((PUCHAR)frustum_VA + 0x28);

		addr = (ULONG_PTR)Search_Pattern(pattern_Frustum2, DllInfo);
		Sig_AddrNotFound(pattern_Frustum2);

		auto vright_VA = *(PVOID*)(addr + 6);

		vright = (decltype(vup))ConvertDllInfoSpace(vright_VA, DllInfo, RealDllInfo);
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

	if (gPrivateFuncs.R_SetupFrame_inlined)
	{
		//do nothing
	}
	else
	{
		Sig_FuncNotFound(R_SetupFrame);
	}

	{
		PVOID R_ForceCVars_VA = 0;
		PVOID R_CheckVariables_VA = 0;
		PVOID R_AnimateLight_VA = 0;

#define R_SETUPFRAME_CALL_SIG "\x0F\x9F\xC0\x50\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8"
		if (R_SetupFrame_VA)
		{
			auto addr = (ULONG_PTR)Search_Pattern_From(R_SetupFrame_VA, R_SETUPFRAME_CALL_SIG, DllInfo);
			if (addr)
			{
				R_ForceCVars_VA = GetCallAddress(addr + 3 + 1);
				R_CheckVariables_VA = GetCallAddress(addr + 3 + 1 + 5);
				R_AnimateLight_VA = GetCallAddress(addr + 3 + 1 + 5 + 5);

				gPrivateFuncs.R_ForceCVars = (decltype(gPrivateFuncs.R_ForceCVars))ConvertDllInfoSpace(R_ForceCVars_VA, DllInfo, RealDllInfo);
				gPrivateFuncs.R_CheckVariables = (decltype(gPrivateFuncs.R_CheckVariables))ConvertDllInfoSpace(R_CheckVariables_VA, DllInfo, RealDllInfo);
				gPrivateFuncs.R_AnimateLight = (decltype(gPrivateFuncs.R_AnimateLight))ConvertDllInfoSpace(R_AnimateLight_VA, DllInfo, RealDllInfo);
			}
			else
			{
#define R_SETUPFRAME_CALL_SIG2 "\x83\xC4\x08\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xA1"
				addr = (ULONG_PTR)Search_Pattern_From(R_SetupFrame_VA, R_SETUPFRAME_CALL_SIG2, DllInfo);

				if (addr)
				{
					R_CheckVariables_VA = GetCallAddress(addr + 3);
					R_AnimateLight_VA = GetCallAddress(addr + 3 + 5);

					//R_ForceCVars has been inlined into R_SetupFrame
					gPrivateFuncs.R_ForceCVars_inlined = true;
					gPrivateFuncs.R_CheckVariables = (decltype(gPrivateFuncs.R_CheckVariables))ConvertDllInfoSpace(R_CheckVariables_VA, DllInfo, RealDllInfo);
					gPrivateFuncs.R_AnimateLight = (decltype(gPrivateFuncs.R_AnimateLight))ConvertDllInfoSpace(R_AnimateLight_VA, DllInfo, RealDllInfo);
				}
			}
		}
		else
		{
			auto addr = (ULONG_PTR)Search_Pattern(R_SETUPFRAME_CALL_SIG, DllInfo);
			if (addr)
			{
				R_ForceCVars_VA = GetCallAddress(addr + 3 + 1);
				R_CheckVariables_VA = GetCallAddress(addr + 3 + 1 + 5);
				R_AnimateLight_VA = GetCallAddress(addr + 3 + 1 + 5 + 5);

				gPrivateFuncs.R_ForceCVars = (decltype(gPrivateFuncs.R_ForceCVars))ConvertDllInfoSpace(R_ForceCVars_VA, DllInfo, RealDllInfo);
				gPrivateFuncs.R_CheckVariables = (decltype(gPrivateFuncs.R_CheckVariables))ConvertDllInfoSpace(R_CheckVariables_VA, DllInfo, RealDllInfo);
				gPrivateFuncs.R_AnimateLight = (decltype(gPrivateFuncs.R_AnimateLight))ConvertDllInfoSpace(R_AnimateLight_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!gPrivateFuncs.R_ForceCVars_inlined)
		Sig_FuncNotFound(R_ForceCVars);

	Sig_FuncNotFound(R_CheckVariables);
	Sig_FuncNotFound(R_AnimateLight);
}

void Engine_FillAddress_R_SetupGL(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_SetupGL)
		return;

	PVOID R_SetupGL_VA = 0;

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
			gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))ConvertDllInfoSpace(R_SetupGL_VA, DllInfo, RealDllInfo);
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
				gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))ConvertDllInfoSpace(R_SetupGL_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!gPrivateFuncs.R_SetupGL)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			if (g_dwEngineBuildnum >= 10152)
			{
				R_SetupGL_VA = Search_Pattern(R_SETUPGL_SIG_SVENGINE_10152, DllInfo);
			}
			else
			{
				R_SetupGL_VA = Search_Pattern(R_SETUPGL_SIG_SVENGINE, DllInfo);
			}
			gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))ConvertDllInfoSpace(R_SetupGL_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			if (g_dwEngineBuildnum >= 9899)
			{
				R_SetupGL_VA = Search_Pattern(R_SETUPGL_SIG_HL25_9899, DllInfo);
			}
			else
			{
				R_SetupGL_VA = Search_Pattern(R_SETUPGL_SIG_HL25, DllInfo);
			}
			gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))ConvertDllInfoSpace(R_SetupGL_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_SetupGL_VA = Search_Pattern(R_SETUPGL_SIG_NEW, DllInfo);

			//try another signature
			if (!R_SetupGL_VA)
				R_SetupGL_VA = Search_Pattern(R_SETUPGL_SIG_NEW2, DllInfo);

			gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))ConvertDllInfoSpace(R_SetupGL_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_SetupGL_VA = Search_Pattern(R_SETUPGL_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_SetupGL = (decltype(gPrivateFuncs.R_SetupGL))ConvertDllInfoSpace(R_SetupGL_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_SetupGL);

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
			gWorldToScreen = (decltype(gWorldToScreen))ConvertDllInfoSpace(gWorldToScreen_VA, DllInfo, RealDllInfo);
			gScreenToWorld = (decltype(gScreenToWorld))ConvertDllInfoSpace(gScreenToWorld_VA, DllInfo, RealDllInfo);
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
		r_world_matrix = (decltype(r_world_matrix))ConvertDllInfoSpace((PVOID)(*(ULONG_PTR*)(addr + 1)), DllInfo, RealDllInfo);

		const char pattern2[] = "\x68\x2A\x2A\x2A\x2A\x68\xA7\x0B\x00\x00";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)R_SetupGL_VA, 0x500, pattern2);
		Sig_AddrNotFound(r_projection_matrix);
		r_projection_matrix = (decltype(r_projection_matrix))ConvertDllInfoSpace((PVOID)(*(ULONG_PTR*)(addr + 1)), DllInfo, RealDllInfo);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char pattern3[] = "\x50\xFF\x15\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size((void*)R_SetupGL_VA, 0x120, pattern3);
		Sig_AddrNotFound(vertical_fov_SvEngine);
		vertical_fov_SvEngine = (decltype(vertical_fov_SvEngine))ConvertDllInfoSpace((PVOID)(*(ULONG_PTR*)(addr + 9)), DllInfo, RealDllInfo);
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

	PVOID R_RenderView_VA = 0;

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
			}
		}
	}

	if (!R_RenderView_VA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_RenderView_VA = (decltype(R_RenderView_VA))Search_Pattern(R_RENDERVIEW_SIG_SVENGINE, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_RenderView_VA = (decltype(R_RenderView_VA))Search_Pattern(R_RENDERVIEW_SIG_HL25, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_RenderView_VA = (decltype(R_RenderView_VA))Search_Pattern(R_RENDERVIEW_SIG_NEW, DllInfo);

			if (!R_RenderView_VA)
				R_RenderView_VA = (decltype(R_RenderView_VA))Search_Pattern(R_RENDERVIEW_SIG_NEW2, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_RenderView_VA = (decltype(R_RenderView_VA))Search_Pattern(R_RENDERVIEW_SIG_BLOB, DllInfo);
		}
	}

	if (R_RenderView_VA)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			gPrivateFuncs.R_RenderView_SvEngine = (decltype(gPrivateFuncs.R_RenderView_SvEngine))ConvertDllInfoSpace(R_RenderView_VA, DllInfo, RealDllInfo);
		}
		else
		{
			gPrivateFuncs.R_RenderView = (decltype(gPrivateFuncs.R_RenderView))ConvertDllInfoSpace(R_RenderView_VA, DllInfo, RealDllInfo);
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

	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			const char pattern[] = "\xFF\x35\x2A\x2A\x2A\x2A\xDC\x0D\x2A\x2A\x2A\x2A\xFF\x35\x2A\x2A\x2A\x2A\xE8";

			size_t SearchLength = 0x800;
			if (gPrivateFuncs.R_RenderScene_inlined)
				SearchLength = 0x1000;

			ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(R_RenderView_VA, SearchLength, pattern);
			Sig_AddrNotFound(c_brush_polys);
			PVOID c_alias_polys_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
			PVOID c_brush_polys_VA = (PVOID)(*(ULONG_PTR*)(addr + 14));
			c_alias_polys = (decltype(c_alias_polys))ConvertDllInfoSpace(c_alias_polys_VA, DllInfo, RealDllInfo);
			c_brush_polys = (decltype(c_brush_polys))ConvertDllInfoSpace(c_brush_polys_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			/*
.text:1024466F C7 05 B0 55 DC 10 00 00 00 00                       mov     c_brush_polys, 0
.text:10244679 C7 05 AC 55 DC 10 00 00 00 00                       mov     c_alias_polys, 0
.text:10244683
.text:10244683                                     loc_10244683:                           ; CODE XREF: R_RenderView+4F↑j
.text:10244683 33 C0                                               xor     eax, eax
.text:10244685 C7 05 E4 50 DC 10 00 00 00 00                       mov     dword_10DC50E4, 0
.text:1024468F 83 3D 44 CC 3F 11 01                                cmp     dword_113FCC44, 1
			*/
			const char pattern[] = "\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00\xC7\x05\x2A\x2A\x2A\x2A\x00\x00\x00\x00";
			ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(R_RenderView_VA, 0x100, pattern);
			Sig_AddrNotFound(c_brush_polys);
			PVOID c_brush_polys_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
			PVOID c_alias_polys_VA = (PVOID)(*(ULONG_PTR*)(addr + 12));
			c_brush_polys = (decltype(c_brush_polys))ConvertDllInfoSpace(c_brush_polys_VA, DllInfo, RealDllInfo);
			c_alias_polys = (decltype(c_alias_polys))ConvertDllInfoSpace(c_alias_polys_VA, DllInfo, RealDllInfo);
		}
		else
		{
			const char pattern[] = "\xA1\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x50\x51";
			ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(R_RenderView_VA, 0x150, pattern);
			Sig_AddrNotFound(c_brush_polys);
			PVOID c_alias_polys_VA = (PVOID)(*(ULONG_PTR*)(addr + 1));
			PVOID c_brush_polys_VA = (PVOID)(*(ULONG_PTR*)(addr + 7));
			c_alias_polys = (decltype(c_alias_polys))ConvertDllInfoSpace(c_alias_polys_VA, DllInfo, RealDllInfo);
			c_brush_polys = (decltype(c_brush_polys))ConvertDllInfoSpace(c_brush_polys_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_VarNotFound(c_alias_polys);
	Sig_VarNotFound(c_brush_polys);

	{
		typedef struct R_RenderView_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			PVOID Candidate[2]{ };
			int CandidateCount{};
		}R_RenderView_SearchContext;

		R_RenderView_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(R_RenderView_VA, 0x60, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (R_RenderView_SearchContext*)context;

			if (ctx->CandidateCount < 2 &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{
				ctx->Candidate[ctx->CandidateCount] = ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				ctx->CandidateCount++;
			}

			if (ctx->CandidateCount < 2 &&
				pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == X86_REG_EAX &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				if (0 == memcmp(address + instLen, "\x85\xC0", 2))
				{
					ctx->Candidate[ctx->CandidateCount] = ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					ctx->CandidateCount++;
				}
			}

			if (instCount >= 30)
				return TRUE;

			if(ctx->CandidateCount >= 2)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

		}, 0, & ctx);

		if (ctx.CandidateCount == 2)
		{
			r_worldentity = (decltype(r_worldentity))((PUCHAR)ctx.Candidate[0] - offsetof(cl_entity_t, model));
			cl_worldmodel = (decltype(cl_worldmodel))ctx.Candidate[1];
		}
	}
	Sig_VarNotFound(r_worldentity);
	Sig_VarNotFound(cl_worldmodel);
}

void Engine_FillAddress_V_RenderView(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.V_RenderView)
		return;

	PVOID V_RenderView_VA = 0;

	PVOID R_RenderView_VA = ConvertDllInfoSpace(
		(gPrivateFuncs.R_RenderView_SvEngine) ? (PVOID)gPrivateFuncs.R_RenderView_SvEngine : (PVOID)gPrivateFuncs.R_RenderView,
		RealDllInfo, DllInfo);

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
				typedef struct V_RenderView_SearchContext_s
				{
					PVOID R_RenderView_VA;
					bool bFoundCallRenderView{};
				}V_RenderView_SearchContext;

				V_RenderView_SearchContext ctx = { R_RenderView_VA };

				g_pMetaHookAPI->DisasmRanges(pFound + 5, 0x120, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto pinst = (cs_insn*)inst;
					auto ctx = (V_RenderView_SearchContext*)context;

					if (address[0] == 0xE8)
					{
						PVOID callTarget = (PVOID)pinst->detail->x86.operands[0].imm;

						if (callTarget == ctx->R_RenderView_VA)
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

					gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))ConvertDllInfoSpace(V_RenderView_VA, DllInfo, RealDllInfo);

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

	if (!gPrivateFuncs.V_RenderView)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			V_RenderView_VA = (PVOID)Search_Pattern(V_RENDERVIEW_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))ConvertDllInfoSpace(V_RenderView_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			V_RenderView_VA = (PVOID)Search_Pattern(V_RENDERVIEW_SIG_HL25, DllInfo);
			gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))ConvertDllInfoSpace(V_RenderView_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			V_RenderView_VA = (PVOID)Search_Pattern(V_RENDERVIEW_SIG_NEW, DllInfo);

			if (!V_RenderView_VA)
				V_RenderView_VA = (PVOID)Search_Pattern(V_RENDERVIEW_SIG_NEW2, DllInfo);

			gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))ConvertDllInfoSpace(V_RenderView_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			V_RenderView_VA = (PVOID)Search_Pattern(V_RENDERVIEW_SIG_BLOB, DllInfo);
			gPrivateFuncs.V_RenderView = (decltype(gPrivateFuncs.V_RenderView))ConvertDllInfoSpace(V_RenderView_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(V_RenderView);

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

		V_RenderView_SearchContext ctx = { DllInfo, RealDllInfo };

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
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 5)
			{
				//83 3D 30 9A 09 02 05                                cmp     cls_state, 5
				cls_state = (decltype(cls_state))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				ctx->MovClsStateInstCount = instCount;
			}

			if (!cls_signon &&
				pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 2)
			{
				//83 3D D4 9F 0C 02 02                                cmp     cls_signon, 2
				cls_signon = (decltype(cls_signon))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
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

			r_soundOrigin = (decltype(r_soundOrigin))ConvertDllInfoSpace((PVOID)ctx.ZeroizedCandidate[0], DllInfo, RealDllInfo);
			r_playerViewportAngles = (decltype(r_playerViewportAngles))ConvertDllInfoSpace((PVOID)ctx.ZeroizedCandidate[3], DllInfo, RealDllInfo);

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
		typedef struct R_RenderScene_SearchContext_s
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

void Engine_FillAddress_R_NewMap(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_NewMap)
		return;

	{
		//Setting up renderer...
		const char sigs[] = "Setting up renderer...\n";
		auto SettingUpRenderer_String = Search_Pattern_Data(sigs, DllInfo);
		if (!SettingUpRenderer_String)
			SettingUpRenderer_String = Search_Pattern_Rdata(sigs, DllInfo);
		Sig_VarNotFound(SettingUpRenderer_String);

		char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
		*(DWORD*)(pattern + 1) = (DWORD)SettingUpRenderer_String;
		auto SettingUpRenderer_PushString = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(SettingUpRenderer_PushString);

		typedef struct SettingUpRenderer_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}SettingUpRenderer_SearchContext;

		SettingUpRenderer_SearchContext ctx = { DllInfo , RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((PUCHAR)SettingUpRenderer_PushString + Sig_Length(pattern), 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (SettingUpRenderer_SearchContext*)context;

			if (address[0] == 0xE8)
			{
				gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

		}, 0, & ctx);
	}

	if (!gPrivateFuncs.R_NewMap)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			PVOID R_NewMap_VA = Search_Pattern(R_NEWMAP_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))ConvertDllInfoSpace(R_NewMap_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			PVOID R_NewMap_VA = Search_Pattern(R_NEWMAP_SIG_HL25, DllInfo);
			gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))ConvertDllInfoSpace(R_NewMap_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			PVOID R_NewMap_VA = Search_Pattern(R_NEWMAP_SIG_NEW, DllInfo);
			gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))ConvertDllInfoSpace(R_NewMap_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			PVOID R_NewMap_VA = Search_Pattern(R_NEWMAP_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_NewMap = (decltype(gPrivateFuncs.R_NewMap))ConvertDllInfoSpace(R_NewMap_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_NewMap);

	PVOID R_NewMap_VA = ConvertDllInfoSpace(gPrivateFuncs.R_NewMap, RealDllInfo, DllInfo);

	{

		char pattern[] = "\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\xFF\xFF\xFF\xFF";
		auto addr = (ULONG_PTR)Search_Pattern_From_Size(R_NewMap_VA, 0x100, pattern);
		if (addr)
		{
			PVOID R_ClearParticles_VA = GetCallAddress(addr + 0);
			PVOID R_DecalInit_VA = GetCallAddress(addr + 5);
			PVOID V_InitLevel_VA = GetCallAddress(addr + 10);
			PVOID GL_BuildLightmaps_VA = GetCallAddress(addr + 15);

			gPrivateFuncs.R_ClearParticles = (decltype(gPrivateFuncs.R_ClearParticles))ConvertDllInfoSpace(R_ClearParticles_VA, DllInfo, RealDllInfo);
			gPrivateFuncs.R_DecalInit = (decltype(gPrivateFuncs.R_DecalInit))ConvertDllInfoSpace(R_DecalInit_VA, DllInfo, RealDllInfo);
			gPrivateFuncs.V_InitLevel = (decltype(gPrivateFuncs.V_InitLevel))ConvertDllInfoSpace(V_InitLevel_VA, DllInfo, RealDllInfo);
			gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))ConvertDllInfoSpace(GL_BuildLightmaps_VA, DllInfo, RealDllInfo);
		}
	}

	{
		typedef struct R_NewMap_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			ULONG_PTR candidateE8_VA{};
			bool bFoundRet{};
		} R_NewMap_SearchContext;

		R_NewMap_SearchContext ctx = { DllInfo };

		g_pMetaHookAPI->DisasmRanges(R_NewMap_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
	
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_NewMap_SearchContext*)context;

			if (address[0] == 0xE8 && instLen == 5)
			{
				ctx->candidateE8_VA = (ULONG_PTR)pinst->detail->x86.operands[0].imm;
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

		if (ctx.bFoundRet && ctx.candidateE8_VA)
		{
			gPrivateFuncs.GL_UnloadTextures = (decltype(gPrivateFuncs.GL_UnloadTextures))ConvertDllInfoSpace((PVOID)ctx.candidateE8_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(GL_UnloadTextures);
}

void Engine_FillAddress_GL_BuildLightmaps(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_BuildLightmaps)
		return;

	PVOID GL_BuildLightmaps_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		GL_BuildLightmaps_VA = Search_Pattern(GL_BUILDLIGHTMAPS_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))ConvertDllInfoSpace((PVOID)GL_BuildLightmaps_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		GL_BuildLightmaps_VA = Search_Pattern(GL_BUILDLIGHTMAPS_SIG_HL25, DllInfo);
		gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))ConvertDllInfoSpace((PVOID)GL_BuildLightmaps_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		GL_BuildLightmaps_VA = Search_Pattern(GL_BUILDLIGHTMAPS_SIG_NEW, DllInfo);
		gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))ConvertDllInfoSpace((PVOID)GL_BuildLightmaps_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		GL_BuildLightmaps_VA = Search_Pattern(GL_BUILDLIGHTMAPS_SIG_BLOB, DllInfo);
		gPrivateFuncs.GL_BuildLightmaps = (decltype(gPrivateFuncs.GL_BuildLightmaps))ConvertDllInfoSpace((PVOID)GL_BuildLightmaps_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(GL_BuildLightmaps);
}

void Engine_FillAddress_R_BuildLightMap(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_BuildLightMap)
		return;

	PVOID R_BuildLightMap_VA = 0;

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
				R_BuildLightMap_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Error_Call, 0x300, [](PUCHAR Candidate) {

					if (Candidate[0] == 0xD9 &&
						Candidate[1] == 0x05)
						return TRUE;

					if (Candidate[0] == 0x55 &&
						Candidate[1] == 0x8B &&
						Candidate[2] == 0xEC)
						return TRUE;

					return FALSE;
					});

				gPrivateFuncs.R_BuildLightMap = (decltype(gPrivateFuncs.R_BuildLightMap))ConvertDllInfoSpace((PVOID)R_BuildLightMap_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!gPrivateFuncs.R_BuildLightMap)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_BuildLightMap_VA = Search_Pattern(R_BUILDLIGHTMAP_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.R_BuildLightMap = (decltype(gPrivateFuncs.R_BuildLightMap))ConvertDllInfoSpace((PVOID)R_BuildLightMap_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_BuildLightMap_VA = Search_Pattern(R_BUILDLIGHTMAP_SIG_HL25, DllInfo);
			gPrivateFuncs.R_BuildLightMap = (decltype(gPrivateFuncs.R_BuildLightMap))ConvertDllInfoSpace((PVOID)R_BuildLightMap_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_BuildLightMap_VA = Search_Pattern(R_BUILDLIGHTMAP_SIG_NEW, DllInfo);
			gPrivateFuncs.R_BuildLightMap = (decltype(gPrivateFuncs.R_BuildLightMap))ConvertDllInfoSpace((PVOID)R_BuildLightMap_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_BuildLightMap_VA = Search_Pattern(R_BUILDLIGHTMAP_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_BuildLightMap = (decltype(gPrivateFuncs.R_BuildLightMap))ConvertDllInfoSpace((PVOID)R_BuildLightMap_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_BuildLightMap);
}

void Engine_FillAddress_R_AddDynamicLights(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_AddDynamicLights)
		return;

	{
		PVOID R_BuildLightMap_VA = ConvertDllInfoSpace(gPrivateFuncs.R_BuildLightMap, RealDllInfo, DllInfo);

		if (!R_BuildLightMap_VA)
		{
			Sig_NotFound(R_BuildLightMap_VA);
		}

		typedef struct R_AddDynamicLights_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			PVOID base{};
			size_t max_insts{};
			int max_depth{};
			std::set<PVOID> code{};
			std::set<PVOID> branches{};
			std::vector<walk_context_t> walks{};
		}R_AddDynamicLights_SearchContext;

		R_AddDynamicLights_SearchContext ctx = { DllInfo, RealDllInfo };

		ctx.base = R_BuildLightMap_VA;
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
						auto nextaddr_VA = GetCallAddress(nextaddr);
						gPrivateFuncs.R_AddDynamicLights = (decltype(gPrivateFuncs.R_AddDynamicLights))ConvertDllInfoSpace(nextaddr_VA, ctx->DllInfo, ctx->RealDllInfo);
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
			PVOID R_AddDynamicLights_VA = Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.R_AddDynamicLights = (decltype(gPrivateFuncs.R_AddDynamicLights))ConvertDllInfoSpace((PVOID)R_AddDynamicLights_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			PVOID R_AddDynamicLights_VA = Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_HL25, DllInfo);
			gPrivateFuncs.R_AddDynamicLights = (decltype(gPrivateFuncs.R_AddDynamicLights))ConvertDllInfoSpace((PVOID)R_AddDynamicLights_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			PVOID R_AddDynamicLights_VA = Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_NEW, DllInfo);
			gPrivateFuncs.R_AddDynamicLights = (decltype(gPrivateFuncs.R_AddDynamicLights))ConvertDllInfoSpace((PVOID)R_AddDynamicLights_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			PVOID R_AddDynamicLights_VA = Search_Pattern(R_ADDDYNAMICLIGHTS_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_AddDynamicLights = (decltype(gPrivateFuncs.R_AddDynamicLights))ConvertDllInfoSpace((PVOID)R_AddDynamicLights_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_AddDynamicLights);
}

void Engine_FillAddress_GL_DisableMultitexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_DisableMultitexture)
		return;

	PVOID GL_DisableMultitexture_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		GL_DisableMultitexture_VA = Search_Pattern(GL_DISABLEMULTITEXTURE_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.GL_DisableMultitexture = (decltype(gPrivateFuncs.GL_DisableMultitexture))ConvertDllInfoSpace((PVOID)GL_DisableMultitexture_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		PVOID R_NewMap_VA = ConvertDllInfoSpace(gPrivateFuncs.R_NewMap, RealDllInfo, DllInfo);

		GL_DisableMultitexture_VA = Search_Pattern_From(R_NewMap_VA, GL_DISABLEMULTITEXTURE_SIG_HL25, DllInfo);
		gPrivateFuncs.GL_DisableMultitexture = (decltype(gPrivateFuncs.GL_DisableMultitexture))ConvertDllInfoSpace((PVOID)GL_DisableMultitexture_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		PVOID R_NewMap_VA = ConvertDllInfoSpace(gPrivateFuncs.R_NewMap, RealDllInfo, DllInfo);

		GL_DisableMultitexture_VA = Search_Pattern_From(R_NewMap_VA, GL_DISABLEMULTITEXTURE_SIG_NEW, DllInfo);
		gPrivateFuncs.GL_DisableMultitexture = (decltype(gPrivateFuncs.GL_DisableMultitexture))ConvertDllInfoSpace((PVOID)GL_DisableMultitexture_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		PVOID R_NewMap_VA = ConvertDllInfoSpace(gPrivateFuncs.R_NewMap, RealDllInfo, DllInfo);

		GL_DisableMultitexture_VA = Search_Pattern_From(R_NewMap_VA, GL_DISABLEMULTITEXTURE_SIG_BLOB, DllInfo);
		gPrivateFuncs.GL_DisableMultitexture = (decltype(gPrivateFuncs.GL_DisableMultitexture))ConvertDllInfoSpace((PVOID)GL_DisableMultitexture_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(GL_DisableMultitexture);
}

void Engine_FillAddress_GL_EnableMultitexture(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.GL_EnableMultitexture)
		return;

	PVOID GL_EnableMultitexture_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		GL_EnableMultitexture_VA = Search_Pattern(GL_ENABLEMULTITEXTURE_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.GL_EnableMultitexture = (decltype(gPrivateFuncs.GL_EnableMultitexture))ConvertDllInfoSpace((PVOID)GL_EnableMultitexture_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		PVOID GL_DisableMultitexture_VA = ConvertDllInfoSpace(gPrivateFuncs.GL_DisableMultitexture, RealDllInfo, DllInfo);

		GL_EnableMultitexture_VA = Search_Pattern_From(GL_DisableMultitexture_VA, GL_ENABLEMULTITEXTURE_SIG_HL25, DllInfo);
		gPrivateFuncs.GL_EnableMultitexture = (decltype(gPrivateFuncs.GL_EnableMultitexture))ConvertDllInfoSpace((PVOID)GL_EnableMultitexture_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		PVOID GL_DisableMultitexture_VA = ConvertDllInfoSpace(gPrivateFuncs.GL_DisableMultitexture, RealDllInfo, DllInfo);

		GL_EnableMultitexture_VA = Search_Pattern_From(GL_DisableMultitexture_VA, GL_ENABLEMULTITEXTURE_SIG_NEW, DllInfo);
		gPrivateFuncs.GL_EnableMultitexture = (decltype(gPrivateFuncs.GL_EnableMultitexture))ConvertDllInfoSpace((PVOID)GL_EnableMultitexture_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		PVOID GL_DisableMultitexture_VA = ConvertDllInfoSpace(gPrivateFuncs.GL_DisableMultitexture, RealDllInfo, DllInfo);

		GL_EnableMultitexture_VA = Search_Pattern_From(GL_DisableMultitexture_VA, GL_ENABLEMULTITEXTURE_SIG_BLOB, DllInfo);
		gPrivateFuncs.GL_EnableMultitexture = (decltype(gPrivateFuncs.GL_EnableMultitexture))ConvertDllInfoSpace((PVOID)GL_EnableMultitexture_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(GL_EnableMultitexture);

	typedef struct GL_EnableMultitexture_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} GL_EnableMultitexture_SearchContext;

	GL_EnableMultitexture_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(GL_EnableMultitexture_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
		auto pinst = (cs_insn*)inst;
		auto ctx = (GL_EnableMultitexture_SearchContext*)context;

		if (!gl_mtexable &&
			pinst->id == X86_INS_CMP &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			pinst->detail->x86.operands[0].mem.index == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			pinst->detail->x86.operands[1].imm == 0)
		{
			//01D57970 83 3D 80 66 00 08 00                                cmp     gl_mtexable, 0
			gl_mtexable = (decltype(gl_mtexable))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (!gl_mtexable &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			pinst->detail->x86.operands[1].mem.index == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D478D0 A1 44 34 34 02                                      mov     eax, gl_mtexable
			gl_mtexable = (decltype(gl_mtexable))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (!mtexenabled &&
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
			//.text:01D478F2 C7 05 10 33 34 02 01 00 00 00                       mov     mtexenabled, 1
			mtexenabled = (decltype(mtexenabled))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (gl_mtexable && mtexenabled)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, &ctx);

	Sig_VarNotFound(gl_mtexable);
	Sig_VarNotFound(mtexenabled);
}

void Engine_FillAddress_R_DrawSequentialPoly(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

	typedef struct R_DrawSequentialPoly_SearchContext_s
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

				typedef struct R_DrawSequentialPoly_FindRenderDynamicLightsContext_s
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

void Engine_FillAddress_R_TextureAnimation(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

void Engine_FillAddress_R_DrawBrushModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

void Engine_FillAddress_R_RecursiveWorldNode(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_RecursiveWorldNode)
		return;

	PVOID R_RecursiveWorldNode_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_RecursiveWorldNode_VA = Search_Pattern(R_RECURSIVEWORLDNODE_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))ConvertDllInfoSpace(R_RecursiveWorldNode_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		PVOID R_DrawSequentialPoly_VA = ConvertDllInfoSpace(gPrivateFuncs.R_DrawSequentialPoly, RealDllInfo, DllInfo);

		if (!R_DrawSequentialPoly_VA)
		{
			Sig_NotFound(R_DrawSequentialPoly_VA);
		}

		R_RecursiveWorldNode_VA = Search_Pattern_From(R_DrawSequentialPoly_VA, R_RECURSIVEWORLDNODE_SIG_HL25, DllInfo);

		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))ConvertDllInfoSpace(R_RecursiveWorldNode_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		PVOID R_DrawBrushModel_VA = ConvertDllInfoSpace(gPrivateFuncs.R_DrawBrushModel, RealDllInfo, DllInfo);

		if (!R_DrawBrushModel_VA)
		{
			Sig_NotFound(R_DrawBrushModel_VA);
		}

		R_RecursiveWorldNode_VA = Search_Pattern_From(R_DrawBrushModel_VA, R_RECURSIVEWORLDNODE_SIG_NEW, DllInfo);

		//try another signature
		if (!R_RecursiveWorldNode_VA)
			R_RecursiveWorldNode_VA = Search_Pattern_From(R_DrawBrushModel_VA, R_RECURSIVEWORLDNODE_SIG_NEW2, DllInfo);

		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))ConvertDllInfoSpace(R_RecursiveWorldNode_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		PVOID R_DrawBrushModel_VA = ConvertDllInfoSpace(gPrivateFuncs.R_DrawBrushModel, RealDllInfo, DllInfo);

		if (!R_DrawBrushModel_VA)
		{
			Sig_NotFound(R_DrawBrushModel_VA);
		}

		R_RecursiveWorldNode_VA = Search_Pattern_From(R_DrawBrushModel_VA, R_RECURSIVEWORLDNODE_SIG_BLOB, DllInfo);
		gPrivateFuncs.R_RecursiveWorldNode = (decltype(gPrivateFuncs.R_RecursiveWorldNode))ConvertDllInfoSpace(R_RecursiveWorldNode_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(R_RecursiveWorldNode);
}

void Engine_FillAddress_R_DrawWorld(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawWorld)
		return;

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
				typedef struct R_DrawWorld_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					bool bFoundLeaEax{};
					bool bFoundPushZero{};
				}R_DrawWorld_SearchContext;

				R_DrawWorld_SearchContext ctx = { DllInfo, RealDllInfo };

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
					auto R_DrawWorld_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(pFound, 0x300, [](PUCHAR Candidate) {

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

					gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))ConvertDllInfoSpace(R_DrawWorld_VA, DllInfo, RealDllInfo);
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
			auto R_DrawWorld_VA = Search_Pattern(R_DRAWWORLD_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))ConvertDllInfoSpace(R_DrawWorld_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			PVOID R_DrawSequentialPoly_VA = ConvertDllInfoSpace(gPrivateFuncs.R_DrawSequentialPoly, RealDllInfo, DllInfo);

			auto R_DrawWorld_VA = Search_Pattern_From(R_DrawSequentialPoly_VA, R_DRAWWORLD_SIG_HL25, DllInfo);
			gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))ConvertDllInfoSpace(R_DrawWorld_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			PVOID R_RecursiveWorldNode_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RecursiveWorldNode, RealDllInfo, DllInfo);

			auto R_DrawWorld_VA = Search_Pattern_From(R_RecursiveWorldNode_VA, R_DRAWWORLD_SIG_NEW, DllInfo);
			gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))ConvertDllInfoSpace(R_DrawWorld_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			PVOID R_RecursiveWorldNode_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RecursiveWorldNode, RealDllInfo, DllInfo);

			auto R_DrawWorld_VA = Search_Pattern_From(R_RecursiveWorldNode_VA, R_DRAWWORLD_SIG_NEW, DllInfo);
			gPrivateFuncs.R_DrawWorld = (decltype(gPrivateFuncs.R_DrawWorld))ConvertDllInfoSpace(R_DrawWorld_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_DrawWorld);

	{
		auto R_DrawWorld_VA = ConvertDllInfoSpace(gPrivateFuncs.R_DrawWorld, RealDllInfo, DllInfo);

		/*
			 //Global pointers that link into engine vars
			 vec_t *modelorg = NULL;
		 */

		typedef struct R_DrawWorld_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			int candidate_count{};
			ULONG_PTR candidateVA[10]{};
		} R_DrawWorld_SearchContext;

		R_DrawWorld_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(R_DrawWorld_VA, 0x130, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

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
				modelorg = (decltype(modelorg)) ConvertDllInfoSpace((PVOID)ctx.candidateVA[ctx.candidate_count - 3], DllInfo, RealDllInfo);
			}
			//modelorg[0], modelorg[1], modelorg[2], other, other, other
			else if (ctx.candidateVA[0] + 4 == ctx.candidateVA[1] &&
				ctx.candidateVA[1] + 4 == ctx.candidateVA[2])
			{
				modelorg = (decltype(modelorg))ConvertDllInfoSpace((PVOID)ctx.candidateVA[0], DllInfo, RealDllInfo);
			}
		}
		else if (ctx.candidate_count == 1)
		{
			modelorg = (decltype(modelorg))ConvertDllInfoSpace((PVOID)ctx.candidateVA[0], DllInfo, RealDllInfo);
		}

		Sig_VarNotFound(modelorg);
	}
}

void Engine_FillAddress_R_DrawViewModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_DrawViewModel)
		return;

	PVOID R_DrawViewModel_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//inlined
	}
	else
	{
		PVOID R_RenderView_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RenderView, RealDllInfo, DllInfo);
		PVOID R_PolyBlend_VA = ConvertDllInfoSpace(gPrivateFuncs.R_PolyBlend, RealDllInfo, DllInfo);
		PVOID S_ExtraUpdate_VA = ConvertDllInfoSpace(gPrivateFuncs.S_ExtraUpdate, RealDllInfo, DllInfo);

		if (!R_RenderView_VA)
		{
			Sig_NotFound(R_RenderView_VA);
		}

		if (!R_PolyBlend_VA)
		{
			Sig_NotFound(R_PolyBlend_VA);
		}

		if (!S_ExtraUpdate_VA)
		{
			Sig_NotFound(S_ExtraUpdate_VA);
		}

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

				if ((ULONG_PTR)target2 == (ULONG_PTR)R_PolyBlend_VA && (ULONG_PTR)target3 == (ULONG_PTR)S_ExtraUpdate_VA)
				{
					R_DrawViewModel_VA = target1;
					gPrivateFuncs.R_DrawViewModel = (decltype(gPrivateFuncs.R_DrawViewModel))ConvertDllInfoSpace(R_DrawViewModel_VA, DllInfo, RealDllInfo);
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

	/*
		//Global pointers that link into engine vars
		int *envmap = NULL;
		int *cl_stats = NULL;
		float *cl_weaponstarttime = NULL;
		int *cl_weaponsequence = NULL;
		int *cl_light_level = NULL;
	*/

	typedef struct R_DrawViewModel_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} R_DrawViewModel_SearchContext;

	R_DrawViewModel_SearchContext ctx = { DllInfo, RealDllInfo };

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char pattern1[] = "\xF6\xC4\x44\x0F\x2A\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(pattern1, DllInfo);
		Sig_AddrNotFound(envmap);
		PVOID envmap_VA = (PVOID)(*(ULONG_PTR*)(addr + 11));
		PVOID cl_stats_VA = (PVOID)(*(ULONG_PTR*)(addr + 24));
		envmap = (decltype(envmap))ConvertDllInfoSpace(envmap_VA, DllInfo, RealDllInfo);
		cl_stats = (decltype(cl_stats))ConvertDllInfoSpace(cl_stats_VA, DllInfo, RealDllInfo);

		//SvEngine 10182, R_PreDrawViewModel not inlined
		const char pattern2[] = "\xD9\x2A\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x89\x81\xDC\x02\x00\x00";
		addr = (ULONG_PTR)Search_Pattern(pattern2, DllInfo);
		Sig_AddrNotFound(cl_weaponstarttime);
		PVOID cl_weaponstarttime_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
		PVOID cl_weaponsequence_VA = (PVOID)(*(ULONG_PTR*)(addr + 7));
		cl_weaponstarttime = (decltype(cl_weaponstarttime))ConvertDllInfoSpace(cl_weaponstarttime_VA, DllInfo, RealDllInfo);
		cl_weaponsequence = (decltype(cl_weaponsequence))ConvertDllInfoSpace(cl_weaponsequence_VA, DllInfo, RealDllInfo);

		const char pattern3[] = "\xD1\xEA\x89\x15\x2A\x2A\x2A\x2A";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x300, pattern3);
		Sig_AddrNotFound(cl_light_level);
		PVOID cl_light_level_VA = (PVOID)(*(ULONG_PTR*)(addr + 4));
		cl_light_level = (decltype(cl_light_level))ConvertDllInfoSpace(cl_light_level_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		const char pattern1[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\xF3\x0F\x10\x05";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size((void*)R_DrawViewModel_VA, 0x100, pattern1);
		Sig_AddrNotFound(envmap);
		PVOID envmap_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
		envmap = (decltype(envmap))ConvertDllInfoSpace(envmap_VA, DllInfo, RealDllInfo);

		const char pattern2[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x0F\x2A\x2A\x2A\x2A\x2A\x8B\x0D\x2A\x2A\x2A\x2A\x83\xB9\x94\x0B\x00\x00\x00";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x50, pattern2);
		Sig_AddrNotFound(cl_stats);
		PVOID cl_stats_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
		cl_stats = (decltype(cl_stats))ConvertDllInfoSpace(cl_stats_VA, DllInfo, RealDllInfo);

		const char pattern3[] = "\xF3\x0F\x11\x05\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x89\x81\xDC\x02\x00\x00";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x500, pattern3);
		Sig_AddrNotFound(cl_weaponstarttime);
		PVOID cl_weaponstarttime_VA = (PVOID)(*(ULONG_PTR*)(addr + 4));
		PVOID cl_weaponsequence_VA = (PVOID)(*(ULONG_PTR*)(addr + 9));
		cl_weaponstarttime = (decltype(cl_weaponstarttime))ConvertDllInfoSpace(cl_weaponstarttime_VA, DllInfo, RealDllInfo);
		cl_weaponsequence = (decltype(cl_weaponsequence))ConvertDllInfoSpace(cl_weaponsequence_VA, DllInfo, RealDllInfo);

		const char pattern4[] = "\xD1\xEA\x89\x15\x2A\x2A\x2A\x2A\x8B\x40\x04";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x600, pattern4);
		Sig_AddrNotFound(cl_light_level);
		PVOID cl_light_level_VA = (PVOID)(*(ULONG_PTR*)(addr + 4));
		cl_light_level = (decltype(cl_light_level))ConvertDllInfoSpace(cl_light_level_VA, DllInfo, RealDllInfo);
	}
	else
	{
		const char pattern1[] = "\x39\x3D\x2A\x2A\x2A\x2A\x0F\x2A\x2A\x2A\x2A\x2A\xD9\x05";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size((void*)R_DrawViewModel_VA, 0x100, pattern1);
		Sig_AddrNotFound(envmap);
		PVOID envmap_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
		envmap = (decltype(envmap))ConvertDllInfoSpace(envmap_VA, DllInfo, RealDllInfo);

		const char pattern2[] = "\x39\x3D\x2A\x2A\x2A\x2A\x0F\x2A\x2A\x2A\x2A\x2A\xA1";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x50, pattern2);
		Sig_AddrNotFound(cl_stats);
		PVOID cl_stats_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
		cl_stats = (decltype(cl_stats))ConvertDllInfoSpace(cl_stats_VA, DllInfo, RealDllInfo);

		const char pattern3[] = "\xD9\x1D\x2A\x2A\x2A\x2A\xA1";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x500, pattern3);
		Sig_AddrNotFound(cl_weaponstarttime);
		PVOID cl_weaponstarttime_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
		PVOID cl_weaponsequence_VA = (PVOID)(*(ULONG_PTR*)(addr + 7));
		cl_weaponstarttime = (decltype(cl_weaponstarttime))ConvertDllInfoSpace(cl_weaponstarttime_VA, DllInfo, RealDllInfo);
		cl_weaponsequence = (decltype(cl_weaponsequence))ConvertDllInfoSpace(cl_weaponsequence_VA, DllInfo, RealDllInfo);

		const char pattern4[] = "\x89\x15\x2A\x2A\x2A\x2A\x89\x2A\x2A\x2A\x2A\x2A\xFF";
		addr = (ULONG_PTR)Search_Pattern_From_Size((void*)addr, 0x600, pattern4);
		Sig_AddrNotFound(cl_light_level);
		PVOID cl_light_level_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
		cl_light_level = (decltype(cl_light_level))ConvertDllInfoSpace(cl_light_level_VA, DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(envmap);
	Sig_VarNotFound(cl_stats);
	Sig_VarNotFound(cl_weaponstarttime);
	Sig_VarNotFound(cl_weaponsequence);
	Sig_VarNotFound(cl_light_level);
}

void Engine_FillAddress_R_MarkLeaves(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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
		} R_MarkLeaves_SearchContext;

		R_MarkLeaves_SearchContext ctx = { r_viewleaf_VA, r_oldviewleaf_VA, DllInfo };

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
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//01D57970 83 3D 80 66 00 08 00                                cmp     gl_mtexable, 0
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
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//01D57970 83 3D 80 66 00 08 00                                cmp     gl_mtexable, 0
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

		Convert_VA_to_RVA(r_viewleaf, DllInfo);
		Convert_VA_to_RVA(r_oldviewleaf, DllInfo);
	}

	if (r_viewleaf_RVA)
		r_viewleaf = (decltype(r_viewleaf))VA_from_RVA(r_viewleaf, RealDllInfo);
	if (r_oldviewleaf_RVA)
		r_oldviewleaf = (decltype(r_oldviewleaf))VA_from_RVA(r_oldviewleaf, RealDllInfo);

	Sig_VarNotFound(r_viewleaf);
	Sig_VarNotFound(r_oldviewleaf);
}

void Engine_FillAddress_GL_BeginRendering(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

void Engine_FillAddress_GL_EndRendering(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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
				GL_EndRendering_VA = (ULONG_PTR)Search_Pattern_From_Size(GL_BeginRendering_VA, 0x100, GL_ENDRENDERING_SIG_NEW);
			}
			else
			{
				GL_EndRendering_VA = (ULONG_PTR)Search_Pattern_From_Size(GL_BeginRendering_VA, 0x100, GL_ENDRENDERING_SIG_BLOB);
			}
			Convert_VA_to_RVA(GL_EndRendering, DllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			ULONG_PTR GL_BeginRendering_VA = (ULONG_PTR)gPrivateFuncs.GL_BeginRendering;
			ULONG GL_BeginRendering_RVA = 0;

			Convert_VA_to_RVA(GL_BeginRendering, RealDllInfo);
			Convert_RVA_to_VA(GL_BeginRendering, DllInfo);

			GL_EndRendering_VA = (ULONG_PTR)Search_Pattern_From_Size(GL_BeginRendering_VA, 0x100, GL_ENDRENDERING_SIG_BLOB);
			Convert_VA_to_RVA(GL_EndRendering, DllInfo);
		}
	}

	if (GL_EndRendering_RVA)
	{
		gPrivateFuncs.GL_EndRendering = (decltype(gPrivateFuncs.GL_EndRendering))VA_from_RVA(GL_EndRendering, RealDllInfo);
	}

	Sig_FuncNotFound(GL_EndRendering);
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

void Engine_FillAddress_Mod_PointInLeaf(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

void Engine_FillAddress_R_DrawTEntitiesOnList(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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
				gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))ConvertDllInfoSpace((PVOID)R_DrawTEntitiesOnList_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!gPrivateFuncs.R_DrawTEntitiesOnList)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))ConvertDllInfoSpace((PVOID)R_DrawTEntitiesOnList_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_HL25, DllInfo);
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))ConvertDllInfoSpace((PVOID)R_DrawTEntitiesOnList_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW, DllInfo);

			if (!R_DrawTEntitiesOnList_VA)
				R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_NEW2, DllInfo);

			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))ConvertDllInfoSpace((PVOID)R_DrawTEntitiesOnList_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_DrawTEntitiesOnList_VA = (ULONG_PTR)Search_Pattern(R_DRAWTENTITIESONLIST_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_DrawTEntitiesOnList = (decltype(gPrivateFuncs.R_DrawTEntitiesOnList))ConvertDllInfoSpace((PVOID)R_DrawTEntitiesOnList_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_DrawTEntitiesOnList);
}

void Engine_FillAddress_BuildGammaTable(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.BuildGammaTable)
		return;

	PVOID BuildGammaTable_VA = 0;

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
					BuildGammaTable_VA = calltarget;
					gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))ConvertDllInfoSpace(BuildGammaTable_VA, DllInfo, RealDllInfo);
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
			BuildGammaTable_VA = Search_Pattern(BUILDGAMMATABLE_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))ConvertDllInfoSpace(BuildGammaTable_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			BuildGammaTable_VA = Search_Pattern(BUILDGAMMATABLE_SIG_HL25, DllInfo);
			gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))ConvertDllInfoSpace(BuildGammaTable_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			BuildGammaTable_VA = Search_Pattern(BUILDGAMMATABLE_SIG_NEW, DllInfo);
			gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))ConvertDllInfoSpace(BuildGammaTable_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			BuildGammaTable_VA = Search_Pattern(BUILDGAMMATABLE_SIG_BLOB, DllInfo);
			gPrivateFuncs.BuildGammaTable = (decltype(gPrivateFuncs.BuildGammaTable))ConvertDllInfoSpace(BuildGammaTable_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(BuildGammaTable);

	/*
	//Global pointers that link into engine vars
	byte *texgammatable = NULL;
	*/

	typedef struct BuildGammaTable_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} BuildGammaTable_SearchContext;

	BuildGammaTable_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(BuildGammaTable_VA, 0x250, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

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
			texgammatable = (decltype(texgammatable))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
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

	PVOID R_DrawParticles_VA = 0;

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
				typedef struct R_DrawParticle_SearchContext_s
				{
					bool bFound2200h{};
					bool bFound2300h{};
					bool bFound303h{};
					bool bFound302h{};
				}R_DrawParticle_SearchContext;

				R_DrawParticle_SearchContext ctx = { };

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

					gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))ConvertDllInfoSpace(R_DrawParticles_VA, DllInfo, RealDllInfo);
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
			R_DrawParticles_VA = Search_Pattern(R_DRAWPARTICLES_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))ConvertDllInfoSpace(R_DrawParticles_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_DrawParticles_VA = Search_Pattern(R_DRAWPARTICLES_SIG_HL25, DllInfo);
			gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))ConvertDllInfoSpace(R_DrawParticles_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_DrawParticles_VA = Search_Pattern(R_DRAWPARTICLES_SIG_NEW, DllInfo);

			if (!R_DrawParticles_VA)
				R_DrawParticles_VA = Search_Pattern(R_DRAWPARTICLES_SIG_NEW2, DllInfo);

			gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))ConvertDllInfoSpace(R_DrawParticles_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_DrawParticles_VA = Search_Pattern(R_DRAWPARTICLES_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_DrawParticles = (decltype(gPrivateFuncs.R_DrawParticles))ConvertDllInfoSpace(R_DrawParticles_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_DrawParticles);

	typedef struct R_DrawParticles_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} R_DrawParticles_SearchContext;

	R_DrawParticles_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(R_DrawParticles_VA, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (R_DrawParticles_SearchContext*)context;

		if (!particletexture &&
			pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			pinst->detail->x86.operands[0].mem.index == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			particletexture = (decltype(particletexture))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (!particletexture &&
			pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].mem.base == 0 &&
			pinst->detail->x86.operands[1].mem.index == 0 &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
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
				particletexture = (decltype(particletexture))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
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
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			if (address[-5] == 0xE8)
			{
				PVOID R_FreeDeadParticles_VA = GetCallAddress(address - 5);

				gPrivateFuncs.R_FreeDeadParticles = (decltype(gPrivateFuncs.R_FreeDeadParticles))ConvertDllInfoSpace(R_FreeDeadParticles_VA, ctx->DllInfo, ctx->RealDllInfo);

				active_particles = (decltype(active_particles))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}
		}

		if (particletexture && active_particles)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(active_particles);
	Sig_VarNotFound(particletexture);

	{
#define R_TRACERDRAW_SIG "\xFF\x15\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x00\xE8\x2A\x2A\x2A\x2A"
		auto addr = (ULONG_PTR)Search_Pattern_From_Size((void*)R_DrawParticles_VA, 0x800, R_TRACERDRAW_SIG);
		Sig_AddrNotFound(R_TracerDraw);

		PVOID R_TracerDraw_VA = GetCallAddress(addr + 6);
		gPrivateFuncs.R_TracerDraw = (decltype(gPrivateFuncs.R_TracerDraw))ConvertDllInfoSpace(R_TracerDraw_VA, DllInfo, RealDllInfo);

		PVOID R_BeamDrawList_VA = GetCallAddress(addr + 11);
		gPrivateFuncs.R_BeamDrawList = (decltype(gPrivateFuncs.R_BeamDrawList))ConvertDllInfoSpace(R_BeamDrawList_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(R_TracerDraw);
	Sig_FuncNotFound(R_BeamDrawList);
}

void Engine_FillAddress_CL_AllocDlight(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

	typedef struct CL_AllocDlight_SearchContext_s
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

void Engine_FillAddress_R_GLStudioDrawPoints(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_GLStudioDrawPoints)
		return;

	PVOID R_GLStudioDrawPoints_VA = 0;
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
				typedef struct R_GLStudioDrawPoints_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					bool bFoundMov1{};
					bool bFound54{};
					bool bFound60{};
				}R_GLStudioDrawPoints_SearchContext;

				R_GLStudioDrawPoints_SearchContext ctx = { DllInfo, RealDllInfo };

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
						gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))ConvertDllInfoSpace((PVOID)CandidateAddress, DllInfo, RealDllInfo);
					}

					if (gPrivateFuncs.R_GLStudioDrawPoints)
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

	if (!gPrivateFuncs.R_GLStudioDrawPoints)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			R_GLStudioDrawPoints_VA = (PVOID)Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))ConvertDllInfoSpace(R_GLStudioDrawPoints_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			R_GLStudioDrawPoints_VA = (PVOID)Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_HL25, DllInfo);
			gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))ConvertDllInfoSpace(R_GLStudioDrawPoints_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			R_GLStudioDrawPoints_VA = (PVOID)Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_NEW, DllInfo);

			if (!R_GLStudioDrawPoints_VA)
				R_GLStudioDrawPoints_VA = (PVOID)Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_NEW2, DllInfo);

			gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))ConvertDllInfoSpace(R_GLStudioDrawPoints_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			R_GLStudioDrawPoints_VA = (PVOID)Search_Pattern(R_GLSTUDIODRAWPOINTS_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_GLStudioDrawPoints = (decltype(gPrivateFuncs.R_GLStudioDrawPoints))ConvertDllInfoSpace(R_GLStudioDrawPoints_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(R_GLStudioDrawPoints);
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

void Engine_FillAddress_R_StudioChrome(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_StudioChrome)
		return;

	PVOID R_StudioChrome_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		R_StudioChrome_VA = Search_Pattern(R_STUDIOCHROME_SIG_SVENGINE, DllInfo);
		gPrivateFuncs.R_StudioChrome = (decltype(gPrivateFuncs.R_StudioChrome))ConvertDllInfoSpace(R_StudioChrome_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		R_StudioChrome_VA = Search_Pattern(R_STUDIOCHROME_SIG_HL25, DllInfo);
		gPrivateFuncs.R_StudioChrome = (decltype(gPrivateFuncs.R_StudioChrome))ConvertDllInfoSpace(R_StudioChrome_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		R_StudioChrome_VA = Search_Pattern(R_STUDIOCHROME_SIG_NEW, DllInfo);

		if (!R_StudioChrome_VA)
			R_StudioChrome_VA = Search_Pattern(R_STUDIOCHROME_SIG_NEW2, DllInfo);

		gPrivateFuncs.R_StudioChrome = (decltype(gPrivateFuncs.R_StudioChrome))ConvertDllInfoSpace(R_StudioChrome_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_StudioChrome_VA = Search_Pattern(R_STUDIOCHROME_SIG_BLOB, DllInfo);
		gPrivateFuncs.R_StudioChrome = (decltype(gPrivateFuncs.R_StudioChrome))ConvertDllInfoSpace(R_StudioChrome_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(R_StudioChrome);
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

	PVOID Cache_Alloc_VA = 0;

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

		Cache_Alloc_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Cache_Alloc_Call, 0x80, [](PUCHAR Candidate) {

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

		gPrivateFuncs.Cache_Alloc = (decltype(gPrivateFuncs.Cache_Alloc))ConvertDllInfoSpace(Cache_Alloc_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(Cache_Alloc);

	typedef struct cache_head_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} cache_head_SearchContext;

	cache_head_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((void*)Cache_Alloc_VA, 0x500, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (cache_head_SearchContext*)context;

		if (!cache_head &&
			pinst->id == X86_INS_CMP &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			(PUCHAR)pinst->detail->x86.operands[1].imm > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			cache_head = (decltype(cache_head))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (!cache_head &&
			pinst->id == X86_INS_CMP &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			(PUCHAR)pinst->detail->x86.operands[1].imm >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			cache_head = (decltype(cache_head))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].imm, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (cache_head)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(cache_head);
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

	PVOID Draw_DecalTexture_VA = 0;

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
				Draw_DecalTexture_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Draw_DecalTexture_Call, 0x300, [](PUCHAR Candidate) {

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

				gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))ConvertDllInfoSpace(Draw_DecalTexture_VA, DllInfo, RealDllInfo);
			}
		}
	}

	if (!gPrivateFuncs.Draw_DecalTexture)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			Draw_DecalTexture_VA = Search_Pattern(DRAW_DECALTEXTURE_SIG_SVENGINE, DllInfo);
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))ConvertDllInfoSpace(Draw_DecalTexture_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			Draw_DecalTexture_VA = Search_Pattern(DRAW_DECALTEXTURE_SIG_HL25, DllInfo);
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))ConvertDllInfoSpace(Draw_DecalTexture_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			Draw_DecalTexture_VA = Search_Pattern(DRAW_DECALTEXTURE_SIG_NEW, DllInfo);
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))ConvertDllInfoSpace(Draw_DecalTexture_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			Draw_DecalTexture_VA = Search_Pattern(DRAW_DECALTEXTURE_SIG_BLOB, DllInfo);
			gPrivateFuncs.Draw_DecalTexture = (decltype(gPrivateFuncs.Draw_DecalTexture))ConvertDllInfoSpace(Draw_DecalTexture_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(Draw_DecalTexture);

	{
		typedef struct Draw_DecalTexture_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			int pushCount{};
			int pushAddrCount{};
		} Draw_DecalTexture_SearchContext;

		Draw_DecalTexture_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(Draw_DecalTexture_VA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (Draw_DecalTexture_SearchContext*)context;

			if (pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				decal_wad = (decltype(decal_wad))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}
			else if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				decal_wad = (decltype(decal_wad))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (pinst->id == X86_INS_PUSH &&
				pinst->detail->x86.op_count == 1)
			{
				ctx->pushCount ++;
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
						ConvertDllInfoSpace(target, ctx->DllInfo, ctx->RealDllInfo);
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
						ConvertDllInfoSpace(target, ctx->DllInfo, ctx->RealDllInfo);
				}
				ctx->pushCount = 0;
				ctx->pushAddrCount = 0;
			}

			if (decal_wad && gPrivateFuncs.Draw_CustomCacheGet && gPrivateFuncs.Draw_CacheGet)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
			}, 0, &ctx);
	}

	Sig_VarNotFound(decal_wad);
	Sig_FuncNotFound(Draw_CustomCacheGet);
	Sig_FuncNotFound(Draw_CacheGet);
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

void Engine_FillAddress_R_GlowBlend(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.R_GlowBlend)
		return;

	PVOID R_GlowBlend_VA = 0;

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
		R_GlowBlend_VA = Search_Pattern(R_GLOW_BLEND_SIG_NEW, DllInfo);
		if (!R_GlowBlend_VA)
			R_GlowBlend_VA = Search_Pattern(R_GLOW_BLEND_SIG_NEW2, DllInfo);
		gPrivateFuncs.R_GlowBlend = (decltype(gPrivateFuncs.R_GlowBlend))ConvertDllInfoSpace(R_GlowBlend_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		R_GlowBlend_VA = Search_Pattern(R_GLOW_BLEND_SIG_BLOB, DllInfo);
		gPrivateFuncs.R_GlowBlend = (decltype(gPrivateFuncs.R_GlowBlend))ConvertDllInfoSpace(R_GlowBlend_VA, DllInfo, RealDllInfo);
	}

	if (!gPrivateFuncs.R_GlowBlend_inlined)
	{
		Sig_FuncNotFound(R_GlowBlend);
	}
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

void Engine_FillAddress_Host_IsSinglePlayerGame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Host_IsSinglePlayerGame)
		return;

	PVOID Host_IsSinglePlayerGame_VA = 0;

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
					typedef struct Host_IsSinglePlayerGame_SearchContext_s
					{
						const mh_dll_info_t& DllInfo;
						const mh_dll_info_t& RealDllInfo;
					} Host_IsSinglePlayerGame_SearchContext;

					Host_IsSinglePlayerGame_SearchContext ctx = { DllInfo, RealDllInfo };

					g_pMetaHookAPI->DisasmRanges(setpause_Function, setpause_PushString - setpause_Function, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
						auto pinst = (cs_insn*)inst;
						auto ctx = (Host_IsSinglePlayerGame_SearchContext*)context;

						if (address[0] == 0xE8 && instLen == 5 &&
							address[5] == 0x85 && address[6] == 0xC0)
						{
							gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
						}

						if (address[0] == 0xCC)
							return TRUE;

						if (pinst->id == X86_INS_RET)
							return TRUE;

						return FALSE;
					}, 0, &ctx);
				}
			}
		}
	}

	if (!gPrivateFuncs.Host_IsSinglePlayerGame)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			Host_IsSinglePlayerGame_VA = Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_SVENGINE, DllInfo);
			gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))ConvertDllInfoSpace(Host_IsSinglePlayerGame_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			Host_IsSinglePlayerGame_VA = Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_HL25, DllInfo);
			gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))ConvertDllInfoSpace(Host_IsSinglePlayerGame_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			Host_IsSinglePlayerGame_VA = Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_NEW, DllInfo);
			if (!Host_IsSinglePlayerGame_VA)
				Host_IsSinglePlayerGame_VA = Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_NEW2, DllInfo);
			if (!Host_IsSinglePlayerGame_VA)
				Host_IsSinglePlayerGame_VA = Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_NEW3, DllInfo);
			gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))ConvertDllInfoSpace(Host_IsSinglePlayerGame_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			Host_IsSinglePlayerGame_VA = Search_Pattern(HOST_IS_SINGLE_PLAYER_GAME_BLOB, DllInfo);
			gPrivateFuncs.Host_IsSinglePlayerGame = (decltype(gPrivateFuncs.Host_IsSinglePlayerGame))ConvertDllInfoSpace(Host_IsSinglePlayerGame_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(Host_IsSinglePlayerGame);
}

void Engine_FillAddress_Mod_UnloadSpriteTextures(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Mod_UnloadSpriteTextures)
		return;

	PVOID Mod_UnloadSpriteTextures_VA = 0;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		Mod_UnloadSpriteTextures_VA = Search_Pattern(MOD_UNLOADSPRITETEXTURES_SVENGINE, DllInfo);
		gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))ConvertDllInfoSpace(Mod_UnloadSpriteTextures_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		Mod_UnloadSpriteTextures_VA = Search_Pattern(MOD_UNLOADSPRITETEXTURES_HL25, DllInfo);
		gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))ConvertDllInfoSpace(Mod_UnloadSpriteTextures_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC)
	{
		Mod_UnloadSpriteTextures_VA = Search_Pattern(MOD_UNLOADSPRITETEXTURES_NEW, DllInfo);
		if (!Mod_UnloadSpriteTextures_VA)
			Mod_UnloadSpriteTextures_VA = Search_Pattern(MOD_UNLOADSPRITETEXTURES_NEW2, DllInfo);
		gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))ConvertDllInfoSpace(Mod_UnloadSpriteTextures_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
	{
		Mod_UnloadSpriteTextures_VA = Search_Pattern(MOD_UNLOADSPRITETEXTURES_BLOB, DllInfo);
		gPrivateFuncs.Mod_UnloadSpriteTextures = (decltype(gPrivateFuncs.Mod_UnloadSpriteTextures))ConvertDllInfoSpace(Mod_UnloadSpriteTextures_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(Mod_UnloadSpriteTextures);
}

void Engine_FillAddress_Mod_LoadSpriteModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Mod_LoadSpriteModel)
		return;

	PVOID Mod_LoadSpriteModel_VA = 0;
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
				typedef struct Mod_LoadSpriteModel_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
				} Mod_LoadSpriteModel_SearchContext;

				Mod_LoadSpriteModel_SearchContext ctx = { DllInfo, RealDllInfo };

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
					Mod_LoadSpriteModel_VA = (PVOID)Sprite_Function;
					gPrivateFuncs.Mod_LoadSpriteModel = (decltype(gPrivateFuncs.Mod_LoadSpriteModel))ConvertDllInfoSpace(Mod_LoadSpriteModel_VA, DllInfo, RealDllInfo);
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
				typedef struct Mod_LoadSpriteModel_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
				} Mod_LoadSpriteModel_SearchContext;

				Mod_LoadSpriteModel_SearchContext ctx = { DllInfo, RealDllInfo };

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
					Mod_LoadSpriteModel_VA = (PVOID)Sprite_Function;
					gPrivateFuncs.Mod_LoadSpriteModel = (decltype(gPrivateFuncs.Mod_LoadSpriteModel))ConvertDllInfoSpace(Mod_LoadSpriteModel_VA, DllInfo, RealDllInfo);
				}
			}
		}
	}

	if (!gPrivateFuncs.Mod_LoadSpriteModel)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			Mod_LoadSpriteModel_VA = (PVOID)Search_Pattern(MOD_LOADSPRITEMODEL_SVENGINE, DllInfo);
			gPrivateFuncs.Mod_LoadSpriteModel = (decltype(gPrivateFuncs.Mod_LoadSpriteModel))ConvertDllInfoSpace(Mod_LoadSpriteModel_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			Mod_LoadSpriteModel_VA = (PVOID)Search_Pattern(MOD_LOADSPRITEMODEL_HL25, DllInfo);
			gPrivateFuncs.Mod_LoadSpriteModel = (decltype(gPrivateFuncs.Mod_LoadSpriteModel))ConvertDllInfoSpace(Mod_LoadSpriteModel_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			Mod_LoadSpriteModel_VA = (PVOID)Search_Pattern(MOD_LOADSPRITEMODEL_NEW, DllInfo);
			if (!Mod_LoadSpriteModel_VA)
				Mod_LoadSpriteModel_VA = (PVOID)Search_Pattern(MOD_LOADSPRITEMODEL_NEW2, DllInfo);
			gPrivateFuncs.Mod_LoadSpriteModel = (decltype(gPrivateFuncs.Mod_LoadSpriteModel))ConvertDllInfoSpace(Mod_LoadSpriteModel_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			Mod_LoadSpriteModel_VA = (PVOID)Search_Pattern(MOD_LOADSPRITEMODEL_BLOB, DllInfo);
			gPrivateFuncs.Mod_LoadSpriteModel = (decltype(gPrivateFuncs.Mod_LoadSpriteModel))ConvertDllInfoSpace(Mod_LoadSpriteModel_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_FuncNotFound(Mod_LoadSpriteModel);
}

void Engine_FillAddress_Mod_LoadSpriteFrame(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (gPrivateFuncs.Mod_LoadSpriteFrame)
		return;

	if (1)
	{
		PVOID Mod_LoadSpriteModel_VA = ConvertDllInfoSpace(gPrivateFuncs.Mod_LoadSpriteModel, RealDllInfo, DllInfo);

		typedef struct Mod_LoadSpriteFrame_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			bool bFoundPush300h{};
			PVOID callTarget{};
		}Mod_LoadSpriteFrame_SearchContext;

		Mod_LoadSpriteFrame_SearchContext ctx = { DllInfo, RealDllInfo };

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
					gPrivateFuncs.Mod_LoadSpriteFrame = (decltype(gPrivateFuncs.Mod_LoadSpriteFrame))ConvertDllInfoSpace(ctx->callTarget, ctx->DllInfo, ctx->RealDllInfo);
					return TRUE;
				}
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);
	}

	Sig_FuncNotFound(Mod_LoadSpriteFrame);

	{
		typedef struct Mod_LoadSpriteFrame_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			int MovRegMem_InstCount{};
			int MovRegMem_Reg{};
			PVOID MovRegMem_CandidateMem{};
		}Mod_LoadSpriteFrame_SearchContext;

		Mod_LoadSpriteFrame_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)ConvertDllInfoSpace(gPrivateFuncs.Mod_LoadSpriteFrame, RealDllInfo, DllInfo), 0x300, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

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
				gSpriteMipMap = (decltype(gSpriteMipMap))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
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
				gSpriteMipMap = (decltype(gSpriteMipMap))ConvertDllInfoSpace(ctx->MovRegMem_CandidateMem, ctx->DllInfo, ctx->RealDllInfo);
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

void Engine_FillAddress_VisEdicts(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars.
		int *cl_numvisedicts = NULL;
		cl_entity_t **cl_visedicts = NULL;
	*/

	PVOID cl_numvisedicts_VA = 0;
	PVOID cl_visedicts_VA = 0;

	{
		/*
		 //CL_CreateVisibleEntity
			.text:01D0C7AF 8B 0D 50 F9 F0 02                                   mov     ecx, dword_2F0F950
			.text:01D0C7B5 81 F9 00 02 00 00                                   cmp     ecx, 200h
		*/
		char pattern[] = "\x8B\x0D\x2A\x2A\x2A\x2A\x81\xF9\x00\x2A\x00\x00";
		auto CL_CreateVisibleEntity_Pattern = Search_Pattern(pattern, DllInfo);
		Sig_VarNotFound(CL_CreateVisibleEntity_Pattern);

		cl_numvisedicts_VA = *(PVOID*)((PUCHAR)CL_CreateVisibleEntity_Pattern + 2);
		cl_numvisedicts = (decltype(cl_numvisedicts))ConvertDllInfoSpace(cl_numvisedicts_VA, DllInfo, RealDllInfo);

		typedef struct VisEdicts_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		} VisEdicts_SearchContext;

		VisEdicts_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(CL_CreateVisibleEntity_Pattern, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (VisEdicts_SearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == X86_REG_ECX &&
				pinst->detail->x86.operands[0].mem.scale == 4 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{
				//.text:01D198C9 89 04 8D 00 3A 6E 02                                mov     cl_visedicts[ecx*4], eax
				//.text:01D0C7C5 89 14 8D C0 F0 D5 02                                mov     cl_visedicts[ecx*4], edx

				cl_visedicts = (decltype(cl_visedicts))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (cl_visedicts)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);
	}

	Sig_VarNotFound(cl_visedicts);
	Sig_VarNotFound(cl_numvisedicts);
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
	ULONG R_RenderFinalFog_RVA = 0;

	PVOID SearchBase_VA = 0;

	if (gPrivateFuncs.R_RenderScene)
		SearchBase_VA = gPrivateFuncs.R_RenderScene;
	else if (gPrivateFuncs.R_RenderView_SvEngine)
		SearchBase_VA = gPrivateFuncs.R_RenderView_SvEngine;
	else if (gPrivateFuncs.R_RenderView)
		SearchBase_VA = gPrivateFuncs.R_RenderView;

	// Convert SearchBase_VA to DllInfo-based
	if (SearchBase_VA)
	{
		ULONG SearchBase_RVA = 0;
		Convert_VA_to_RVA(SearchBase, RealDllInfo);
		Convert_RVA_to_VA(SearchBase, DllInfo);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		typedef struct R_RenderFinalFog_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			PVOID& R_RenderFinalFog_VA;

			bool bFoundPushB60h{};
			bool bFoundPush801h{};
			bool bFoundPushB65h{};
		} R_RenderFinalFog_SearchContext;

		R_RenderFinalFog_SearchContext ctx = { DllInfo, RealDllInfo, R_RenderFinalFog_VA };

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
					g_bUserFogOn = (decltype(g_bUserFogOn))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
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

		Convert_VA_to_RVA(R_RenderFinalFog, DllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		const char sigs[] = "\x83\x3D\x2A\x2A\x2A\x2A\x00\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x60\x0B\x00\x00";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(SearchBase_VA, 0x600, sigs);

		if (addr)
		{
			g_bUserFogOn = (decltype(g_bUserFogOn))ConvertDllInfoSpace((PVOID)(*(ULONG_PTR*)(addr + 2)), DllInfo, RealDllInfo);

			R_RenderFinalFog_VA = (PVOID)GetCallAddress(addr + 9);
			Convert_VA_to_RVA(R_RenderFinalFog, DllInfo);
		}
	}
	else
	{
		const char sigs[] = "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x6A\x00";
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(SearchBase_VA, 0x600, sigs);

		if (addr)
		{
			R_RenderFinalFog_VA = (PVOID)GetCallAddress(addr + 9);
			g_bUserFogOn = (decltype(g_bUserFogOn))ConvertDllInfoSpace((PVOID)(*(ULONG_PTR*)(addr + 1)), DllInfo, RealDllInfo);

			Convert_VA_to_RVA(R_RenderFinalFog, DllInfo);
		}
	}

	if (R_RenderFinalFog_RVA)
	{
		typedef struct R_RenderFinalFog_VarsContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			ULONG_PTR candidateVA{};
		} R_RenderFinalFog_VarsContext;

		R_RenderFinalFog_VarsContext ctx = { DllInfo, RealDllInfo };

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
					g_UserFogDensity = (decltype(g_UserFogDensity))ConvertDllInfoSpace((PVOID)ctx->candidateVA, ctx->DllInfo, ctx->RealDllInfo);
					ctx->candidateVA = 0;
				}
				else if (!g_UserFogColor && pushvalue == GL_FOG_COLOR)
				{
					g_UserFogColor = (decltype(g_UserFogColor))ConvertDllInfoSpace((PVOID)ctx->candidateVA, ctx->DllInfo, ctx->RealDllInfo);
					ctx->candidateVA = 0;
				}
				else if (!g_UserFogStart && pushvalue == GL_FOG_START)
				{
					g_UserFogStart = (decltype(g_UserFogStart))ConvertDllInfoSpace((PVOID)ctx->candidateVA, ctx->DllInfo, ctx->RealDllInfo);
					ctx->candidateVA = 0;
				}
				else if (!g_UserFogEnd && pushvalue == GL_FOG_END)
				{
					g_UserFogEnd = (decltype(g_UserFogEnd))ConvertDllInfoSpace((PVOID)ctx->candidateVA, ctx->DllInfo, ctx->RealDllInfo);
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

			if (g_UserFogDensity && g_UserFogColor && g_UserFogStart && g_UserFogEnd)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		gPrivateFuncs.R_RenderFinalFog = (decltype(gPrivateFuncs.R_RenderFinalFog))VA_from_RVA(R_RenderFinalFog, RealDllInfo);
	}

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
		void *cl_frames = NULL;
		int *cl_parsecount = NULL;

		//Global vars
		int size_of_frame = sizeof(frame_t);
	*/
	PVOID R_DrawTEntitiesOnList_VA = ConvertDllInfoSpace(gPrivateFuncs.R_DrawTEntitiesOnList, RealDllInfo, DllInfo);

	if (g_dwEngineBuildnum <= 8684)
	{
		size_of_frame = 0x42B8;
	}

	typedef struct R_DrawTEntitiesOnList_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		int disableFog_instcount{};
		int parsemod_instcount{};
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
		else if (!cl_parsecount && ctx->parsemod_instcount &&
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
			cl_parsecount = (decltype(cl_parsecount))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (!cl_frames && ctx->parsemod_instcount &&
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
			cl_frames = (decltype(cl_frames))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
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

		if (r_blend && cl_parsecount && cl_frames && ctx->r_entorigin_candidate_count >= 3 && gPrivateFuncs.ClientDLL_DrawTransparentTriangles && gPrivateFuncs.pfnDrawTransparentTriangles)
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
	Sig_VarNotFound(size_of_frame);
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
		msurface_t **skychain = NULL;
		msurface_t **waterchain = NULL;
	*/

	PVOID R_RecursiveWorldNode_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RecursiveWorldNode, RealDllInfo, DllInfo);

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
		int test_cl_instcount{};
		int test_cl_flag{};
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

			if (!skychain && ctx->test_cl_flag == 4)
				skychain = (decltype(skychain))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			else if (!waterchain && ctx->test_cl_flag == 0x10)
				waterchain = (decltype(waterchain))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (r_visframecount && r_framecount && skychain && waterchain)
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

void Engine_FillAddress_R_LoadSkybox(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *gSkyTexNumber = NULL;
	*/

	PVOID R_LoadSkyboxInt_SvEngine_VA = 0;
	PVOID R_LoadSkyBox_SvEngine_VA = 0;
	PVOID R_LoadSkys_VA = 0;

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

		R_LoadSkyboxInt_SvEngine_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_LoadSkys_PushString, 0x600, [](PUCHAR Candidate) {
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

		//SvEngine always has g_bHasOfficialGLTexAllocSupport == true

		typedef struct R_LoadSkys_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			int candidate_register{};
			int candidate_instcount{};
			ULONG_PTR candidate_gSkyTexNumberVA{};
		} R_LoadSkys_SearchContext;

		R_LoadSkys_SearchContext ctx = { DllInfo, RealDllInfo };

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
					gSkyTexNumber = (decltype(gSkyTexNumber))ConvertDllInfoSpace((PVOID)ctx->candidate_gSkyTexNumberVA, ctx->DllInfo, ctx->RealDllInfo);
				}
				else if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base != 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{//.text:01D5FC00 FF B6 80 69 00 08                                   push    gSkyTexNumber[esi]
					gSkyTexNumber = (decltype(gSkyTexNumber))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}

				if (gSkyTexNumber)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

		const char sigs2[] = "desert\0";
		auto R_LoadSkyBox_String = Search_Pattern_Rdata(sigs2, DllInfo);
		Sig_VarNotFound(R_LoadSkyBox_String);

		char pattern2[] = "\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C";
		*(DWORD*)(pattern2 + 6) = (DWORD)R_LoadSkyBox_String;
		auto R_LoadSkyBox_PushString = Search_Pattern(pattern2, DllInfo);
		Sig_VarNotFound(R_LoadSkyBox_PushString);

		R_LoadSkyBox_SvEngine_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_LoadSkyBox_PushString, 0x200, [](PUCHAR Candidate) {
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

		R_LoadSkys_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(R_LoadSkys_PushString, 0x600, [](PUCHAR Candidate) {
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

			//.text:01D5AC7C 83 EC 6C                                            sub     esp, 6Ch
			//.text:01D5AC7F A1 D8 33 38 02                                      mov     eax, dword_23833D8
			if (Candidate[0] == 0x83 &&
				Candidate[1] == 0xEC &&
				Candidate[3] == 0xA1)
				return TRUE;

			return FALSE;
		});

		if (g_bHasOfficialGLTexAllocSupport)
		{
			typedef struct R_LoadSkys_SearchContext_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;
				int candidate_register{};
				int candidate_instcount{};
				ULONG_PTR candidate_gSkyTexNumberVA{};
			} R_LoadSkys_SearchContext;

			R_LoadSkys_SearchContext ctx = { DllInfo, RealDllInfo };

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
						gSkyTexNumber = (decltype(gSkyTexNumber))ConvertDllInfoSpace((PVOID)ctx->candidate_gSkyTexNumberVA, ctx->DllInfo, ctx->RealDllInfo);
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
						gSkyTexNumber = (decltype(gSkyTexNumber))ConvertDllInfoSpace((PVOID)ctx->candidate_gSkyTexNumberVA, ctx->DllInfo, ctx->RealDllInfo);
					}
					else if (pinst->id == X86_INS_PUSH &&
						pinst->detail->x86.op_count == 1 &&
						pinst->detail->x86.operands[0].type == X86_OP_MEM &&
						pinst->detail->x86.operands[0].mem.base != 0 &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
						(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
					{//.text:01D5FC00 FF B6 80 69 00 08                                   push    gSkyTexNumber[esi]
						gSkyTexNumber = (decltype(gSkyTexNumber))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
					}

					if (gSkyTexNumber)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;
				}, 0, &ctx);
		}
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.R_LoadSkyboxInt_SvEngine = (decltype(gPrivateFuncs.R_LoadSkyboxInt_SvEngine))ConvertDllInfoSpace(R_LoadSkyboxInt_SvEngine_VA, DllInfo, RealDllInfo);
		gPrivateFuncs.R_LoadSkyBox_SvEngine = (decltype(gPrivateFuncs.R_LoadSkyBox_SvEngine))ConvertDllInfoSpace(R_LoadSkyBox_SvEngine_VA, DllInfo, RealDllInfo);

		Sig_FuncNotFound(R_LoadSkyboxInt_SvEngine);
		Sig_FuncNotFound(R_LoadSkyBox_SvEngine);
	}
	else
	{
		gPrivateFuncs.R_LoadSkys = (decltype(gPrivateFuncs.R_LoadSkys))ConvertDllInfoSpace(R_LoadSkys_VA, DllInfo, RealDllInfo);
		Sig_FuncNotFound(R_LoadSkys);
	}

	if (g_bHasOfficialGLTexAllocSupport)
	{
		Sig_VarNotFound(gSkyTexNumber);
	}

	/*
  //Global pointers that link into engine vars.
	  int *r_loading_skybox = NULL;
  */

	{
		PVOID SearchBase_VA = 0;

		if (g_iEngineType == ENGINE_SVENGINE)
			SearchBase_VA = R_LoadSkyBox_SvEngine_VA;
		else
			SearchBase_VA = R_LoadSkys_VA;

		typedef struct R_LoadSkyboxVars_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		} R_LoadSkyboxVars_SearchContext;

		R_LoadSkyboxVars_SearchContext ctx = { DllInfo, RealDllInfo };

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
					r_loading_skybox = (decltype(r_loading_skybox))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}
				else if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0)
				{
					r_loading_skybox = (decltype(r_loading_skybox))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				}

				if (r_loading_skybox)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);
	}

	Sig_VarNotFound(r_loading_skybox);
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
		float *scrfov = NULL;
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
			v1[7] = scrfov;
		*/

		const char pattern[] = "\xD9\x05\x2A\x2A\x2A\x2A\xD9\x5C\x24\x1C\x89\x44\x24\x18";
		auto addr = (ULONG_PTR)Search_Pattern(pattern, DllInfo);

		if (addr)
		{
			PVOID scrfov_VA = (PVOID)(*(ULONG_PTR*)(addr + 2));
			scrfov = (decltype(scrfov))ConvertDllInfoSpace(scrfov_VA, DllInfo, RealDllInfo);
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
				scrfov = (decltype(scrfov))ConvertDllInfoSpace(scrfov_VA, DllInfo, RealDllInfo);
			}
		}
	}

	Sig_VarNotFound(scrfov);
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

//Got ClientDLL_DrawNormalTriangles_VA, cl_waterlevel and gDevOverview here
void Engine_FillAddress_RenderSceneVars2(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
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
			std::map<int, ULONG_PTR> candidate_disp{};
			std::map<int, ULONG_PTR> candidate_addr{};
		} R_RenderScene_SearchContext;

		R_RenderScene_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(SearchBase_VA, SearchLength, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (R_RenderScene_SearchContext*)context;

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM && pinst->detail->x86.operands[1].imm == 2
				)
			{
				cl_waterlevel = (decltype(cl_waterlevel))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (!cl_waterlevel && pinst->id == X86_INS_MOV &&
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

			if (!cl_waterlevel && pinst->id == X86_INS_CMP &&
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
						cl_waterlevel = (decltype(cl_waterlevel))ConvertDllInfoSpace((PVOID)disp, ctx->DllInfo, ctx->RealDllInfo);
					}
				}
			}

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

			if (cl_waterlevel && gPrivateFuncs.ClientDLL_DrawNormalTriangles)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);
	}

	Sig_VarNotFound(cl_waterlevel);
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
	/*
	//Sven Co-op only
		int* allow_cheats = NULL;
	*/

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		PVOID CL_IsDevOverviewMode_VA = ConvertDllInfoSpace(gPrivateFuncs.CL_IsDevOverviewMode, RealDllInfo, DllInfo);

		typedef struct CL_IsDevOverviewMode_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		} CL_IsDevOverviewMode_SearchContext;

		CL_IsDevOverviewMode_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(CL_IsDevOverviewMode_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
		
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
				allow_cheats = (decltype(allow_cheats))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (allow_cheats)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(allow_cheats);
	}
	else
	{
		//GoldSrc doesn't have such "int *allow_cheats;"
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

void Engine_FillAddress_R_RenderDynamicLightmaps(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (!gPrivateFuncs.R_RenderDynamicLightmaps)
	{
		if (g_iEngineType == ENGINE_SVENGINE)
		{
			PVOID R_RenderDynamicLightmaps_VA = Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_SVENGINE, DllInfo);
			if (!R_RenderDynamicLightmaps_VA)
				R_RenderDynamicLightmaps_VA = Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_SVENGINE_10152, DllInfo);
			gPrivateFuncs.R_RenderDynamicLightmaps = (decltype(gPrivateFuncs.R_RenderDynamicLightmaps))ConvertDllInfoSpace(R_RenderDynamicLightmaps_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			PVOID R_RenderDynamicLightmaps_VA = Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_HL25, DllInfo);
			gPrivateFuncs.R_RenderDynamicLightmaps = (decltype(gPrivateFuncs.R_RenderDynamicLightmaps))ConvertDllInfoSpace(R_RenderDynamicLightmaps_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC)
		{
			PVOID R_RenderDynamicLightmaps_VA = Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_NEW, DllInfo);
			if (!R_RenderDynamicLightmaps_VA)
				R_RenderDynamicLightmaps_VA = Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_NEW2, DllInfo);
			gPrivateFuncs.R_RenderDynamicLightmaps = (decltype(gPrivateFuncs.R_RenderDynamicLightmaps))ConvertDllInfoSpace(R_RenderDynamicLightmaps_VA, DllInfo, RealDllInfo);
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_BLOB)
		{
			PVOID R_RenderDynamicLightmaps_VA = Search_Pattern(R_RENDERDYNAMICLIGHTMAPS_SIG_BLOB, DllInfo);
			gPrivateFuncs.R_RenderDynamicLightmaps = (decltype(gPrivateFuncs.R_RenderDynamicLightmaps))ConvertDllInfoSpace(R_RenderDynamicLightmaps_VA, DllInfo, RealDllInfo);
		}

		Sig_FuncNotFound(R_RenderDynamicLightmaps);
	}

	PVOID R_RenderDynamicLightmaps_VA = ConvertDllInfoSpace(gPrivateFuncs.R_RenderDynamicLightmaps, RealDllInfo, DllInfo);

	/*
		 int *d_lightstylevalue = NULL;
		 int *lightmap_modified = NULL;
		 glpoly_t **lightmap_polys = NULL;
	 */

	typedef struct R_RenderDynamicLightmaps_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		int cmp_al_FF_instcount{};
	} R_RenderDynamicLightmaps_SearchContext;

	R_RenderDynamicLightmaps_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(R_RenderDynamicLightmaps_VA, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (R_RenderDynamicLightmaps_SearchContext*)context;

		if (!lightmap_polys &&
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
		{
			//.text:01D58422 89 04 8D C8 B8 F5 03 mov     lightmap_polys[ecx*4], eax

			lightmap_polys = (decltype(lightmap_polys))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (pinst->id == X86_INS_CMP &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].reg == X86_REG_AL &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			pinst->detail->x86.operands[1].imm == 0xFF)
		{
			//.text:01D47F46 3C FF   cmp     al, 0FFh

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
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp >(PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D47F4F 8B 04 85 20 9A BC 02                                mov     eax, d_lightstylevalue[eax*4]

			d_lightstylevalue = (decltype(d_lightstylevalue))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}
		else if (!lightmap_modified &&
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
		{
			//.text:01D58489 C7 04 85 C8 C8 F5 03 01 00 00 00                    mov     lightmap_modified[eax*4], 1

			lightmap_modified = (decltype(lightmap_modified))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
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

void Engine_FillAddress_R_StudioChromeVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int (*chromeage)[MAXSTUDIOBONES] = NULL;
		int (*chrome)[MAXSTUDIOBONES] = NULL;
	*/
	PVOID R_GLStudioDrawPoints_VA = 0;
	PVOID R_StudioChrome_VA = 0;

	if (gPrivateFuncs.R_GLStudioDrawPoints)
	{
		R_GLStudioDrawPoints_VA = ConvertDllInfoSpace(gPrivateFuncs.R_GLStudioDrawPoints, RealDllInfo, DllInfo);
	}

	if (gPrivateFuncs.R_StudioChrome)
	{
		R_StudioChrome_VA = ConvertDllInfoSpace(gPrivateFuncs.R_StudioChrome, RealDllInfo, DllInfo);
	}

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		//R_StudioChrome has been inlined into R_GLStudioDrawPoints
#define CHROMEAGE_SIG_SVENGINE "\xBF\x2A\x2A\x2A\x2A\xF3\xAB\x33\xFF\x39"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(R_GLStudioDrawPoints_VA, 0x600, CHROMEAGE_SIG_SVENGINE);
		Sig_AddrNotFound(chromeage);
		PVOID chromeage_VA = *(PVOID*)(addr + 1);
		chromeage = (decltype(chromeage))ConvertDllInfoSpace(chromeage_VA, DllInfo, RealDllInfo);

#define CHROME_SIG_SVENGINE "\xC1\xE8\x1F\x03\xC2\x8D\x04\xC5\x2A\x2A\x2A\x2A\x50\xE8"
		addr = (ULONG_PTR)Search_Pattern_From_Size(R_GLStudioDrawPoints_VA, 0x1000, CHROME_SIG_SVENGINE);
		Sig_AddrNotFound(chrome);
		PVOID chrome_VA = *(PVOID*)(addr + 8);
		chrome = (decltype(chrome))ConvertDllInfoSpace(chrome_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		//R_StudioChrome has been inlined into R_GLStudioDrawPoints
#define CHROMEAGE_SIG_HL25 "\x33\xC0\xBF\x2A\x2A\x2A\x2A\xF3\xAB\x8B"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(R_GLStudioDrawPoints_VA, 0x600, CHROMEAGE_SIG_HL25);
		Sig_AddrNotFound(chromeage);
		PVOID chromeage_VA = *(PVOID*)(addr + 3);
		chromeage = (decltype(chromeage))ConvertDllInfoSpace(chromeage_VA, DllInfo, RealDllInfo);

#define CHROME_SIG_HL25 "\x8D\x04\xFD\x2A\x2A\x2A\x2A\xFF\xB5\x2A\x2A\x2A\x2A\x50\xE8\x2A\x2A\x2A\x2A\x83\xC4\x0C"
		addr = (ULONG_PTR)Search_Pattern_From_Size(R_GLStudioDrawPoints_VA, 0x1000, CHROME_SIG_HL25);
		Sig_AddrNotFound(chrome);
		PVOID chrome_VA = *(PVOID*)(addr + 3);
		chrome = (decltype(chrome))ConvertDllInfoSpace(chrome_VA, DllInfo, RealDllInfo);
	}
	else
	{
#define CHROMEAGE_SIG "\x8B\x04\xB5\x2A\x2A\x2A\x2A\x3B\xC1"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(R_StudioChrome_VA, 0x50, CHROMEAGE_SIG);
		Sig_AddrNotFound(chromeage);
		PVOID chromeage_VA = *(PVOID*)(addr + 3);
		chromeage = (decltype(chromeage))ConvertDllInfoSpace(chromeage_VA, DllInfo, RealDllInfo);

#define CHROME_SIG_NEW "\x8D\x0C\xD5\x2A\x2A\x2A\x2A\x51\xE8"
		addr = (ULONG_PTR)Search_Pattern_From_Size(R_GLStudioDrawPoints_VA, 0x600, CHROME_SIG_NEW);
		Sig_AddrNotFound(chrome);
		PVOID chrome_VA = *(PVOID*)(addr + 3);
		chrome = (decltype(chrome))ConvertDllInfoSpace(chrome_VA, DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(chromeage);
	Sig_VarNotFound(chrome);
}

void Engine_FillAddress_CL_SimOrgVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		/*
.text:01D24BAE                                     loc_1D24BAE:                            ; CODE XREF: sub_1D249E0+179↑j
.text:01D24BAE D9 05 D8 B6 34 02                                   fld     cl_simorg
.text:01D24BB4 D9 9E 48 0B 00 00                                   fstp    dword ptr [esi+0B48h]
.text:01D24BBA D9 05 DC B6 34 02                                   fld     flt_234B6DC
.text:01D24BC0 D9 9E 4C 0B 00 00                                   fstp    dword ptr [esi+0B4Ch]
.text:01D24BC6 D9 05 E0 B6 34 02                                   fld     flt_234B6E0
.text:01D24BCC D9 9E 50 0B 00 00                                   fstp    dword ptr [esi+0B50h]
		*/
		//D9 05 ?? ?? ?? ?? D9 9E 48 0B 00 00 D9 05 ?? ?? ?? ?? D9 9E 4C 0B 00 00 D9 05
#define CL_SIMORG_SIG_SVENGINE "\xD9\x05\x2A\x2A\x2A\x2A\xD9\x9E\x48\x0B\x00\x00\xD9\x05\x2A\x2A\x2A\x2A\xD9\x9E\x4C\x0B\x00\x00\xD9\x05"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(CL_SIMORG_SIG_SVENGINE, DllInfo);
		Sig_AddrNotFound(cl_simorg);
		PVOID cl_simorg_VA = *(PVOID*)(addr + 2);
		cl_simorg = (decltype(cl_simorg))ConvertDllInfoSpace(cl_simorg_VA, DllInfo, RealDllInfo);
	}
	else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
		/*
		* F3 0F 10 05 ?? ?? ?? ?? F3 0F 11 87 48 0B 00 00 F3 0F 10 05 ?? ?? ?? ?? F3 0F 11 87 4C 0B 00 00
.text:1019E7D5                                     loc_1019E7D5:                           ; CODE XREF: CL_LinkPlayers+1A0↑j
.text:1019E7D5 F3 0F 10 05 68 2B 28 11                             movss   xmm0, cl_simorg
.text:1019E7DD F3 0F 11 87 48 0B 00 00                             movss   dword ptr [edi+0B48h], xmm0
.text:1019E7E5 F3 0F 10 05 6C 2B 28 11                             movss   xmm0, dword_11282B6C
.text:1019E7ED F3 0F 11 87 4C 0B 00 00                             movss   dword ptr [edi+0B4Ch], xmm0
.text:1019E7F5 F3 0F 10 05 70 2B 28 11                             movss   xmm0, dword_11282B70
.text:1019E7FD F3 0F 11 87 50 0B 00 00                             movss   dword ptr [edi+0B50h], xmm0
.text:1019E805
*/
#define CL_SIMORG_SIG_HL25 "\xF3\x0F\x10\x05\x2A\x2A\x2A\x2A\xF3\x0F\x11\x87\x48\x0B\x00\x00\xF3\x0F\x10\x05\x2A\x2A\x2A\x2A\xF3\x0F\x11\x87\x4C\x0B\x00\x00"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(CL_SIMORG_SIG_HL25, DllInfo);
		Sig_AddrNotFound(cl_simorg);
		PVOID cl_simorg_VA = *(PVOID*)(addr + 4);
		cl_simorg = (decltype(cl_simorg))ConvertDllInfoSpace(cl_simorg_VA, DllInfo, RealDllInfo);
	}
	else
	{
		/*
		* 
		* 8B ?? ?? ?? ?? ?? A1 ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? 89 96 48 0B 00 00 89 86 4C 0B 00 00
		* 
.text:01D140AE                                     loc_1D140AE:                            ; CODE XREF: sub_1D13ED0+189↑j
.text:01D140AE 8B 15 E8 AE D8 02                                   mov     edx, cl_simorg
.text:01D140B4 A1 EC AE D8 02                                      mov     eax, dword_2D8AEEC
.text:01D140B9 8B 0D F0 AE D8 02                                   mov     ecx, dword_2D8AEF0
.text:01D140BF 89 96 48 0B 00 00                                   mov     [esi+0B48h], edx
.text:01D140C5 89 86 4C 0B 00 00                                   mov     [esi+0B4Ch], eax
.text:01D140CB 89 8E 50 0B 00 00                                   mov     [esi+0B50h], ecx
		*/

#define CL_SIMORG_SIG "\x8B\x2A\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x8B\x2A\x2A\x2A\x2A\x2A\x89\x96\x48\x0B\x00\x00\x89\x86\x4C\x0B\x00\x00"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(CL_SIMORG_SIG, DllInfo);
		Sig_AddrNotFound(cl_simorg);
		PVOID cl_simorg_VA = *(PVOID*)(addr + 2);
		cl_simorg = (decltype(cl_simorg))ConvertDllInfoSpace(cl_simorg_VA, DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(cl_simorg);
}

void Engine_FillAddress_CL_ViewEntityVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *cl_viewentity = NULL;
	*/

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define CL_VIEWENTITY_SIG_SVENGINE "\x68\x2A\x2A\x2A\x2A\x50\x6A\x06\xFF\x35\x2A\x2A\x2A\x2A\xE8"
		auto addr = (PUCHAR)Search_Pattern_From_Size((void*)DllInfo.TextBase, DllInfo.TextSize, CL_VIEWENTITY_SIG_SVENGINE);
		Sig_AddrNotFound(cl_viewentity);
		PVOID cl_viewentity_VA = *(PVOID*)(addr + 10);
		cl_viewentity = (decltype(cl_viewentity))ConvertDllInfoSpace(cl_viewentity_VA, DllInfo, RealDllInfo);
	}
	else
	{
#define CL_VIEWENTITY_SIG_GOLDSRC "\xA1\x2A\x2A\x2A\x2A\x48\x3B\x2A"
		auto addr = (PUCHAR)Search_Pattern_From_Size((void*)DllInfo.TextBase, DllInfo.TextSize, CL_VIEWENTITY_SIG_GOLDSRC);
		Sig_AddrNotFound(cl_viewentity);

		typedef struct CL_ViewEntity_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			bool found_cmp_200{};
		} CL_ViewEntity_SearchContext;

		CL_ViewEntity_SearchContext ctx = { DllInfo };

		g_pMetaHookAPI->DisasmRanges((void*)addr, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (CL_ViewEntity_SearchContext*)context;

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
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
			PVOID cl_viewentity_VA = *(PVOID*)(addr + 1);
			cl_viewentity = (decltype(cl_viewentity))ConvertDllInfoSpace(cl_viewentity_VA, DllInfo, RealDllInfo);
		}
	}

	Sig_VarNotFound(cl_viewentity);
}

void Engine_FillAddress_CL_ReallocateDynamicData(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *cl_max_edicts = NULL;
		cl_entity_t *cl_entities = NULL;
	*/

	//Search "CL_Reallocate cl_entities"
	const char sigs[] = "CL_Reallocate cl_entities\n";
	auto CL_Reallocate_String = Search_Pattern_Data(sigs, DllInfo);
	if (!CL_Reallocate_String)
		CL_Reallocate_String = Search_Pattern_Rdata(sigs, DllInfo);
	Sig_VarNotFound(CL_Reallocate_String);

	char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A";
	*(DWORD*)(pattern + 1) = (DWORD)CL_Reallocate_String;
	PVOID CL_Reallocate_Call = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(CL_Reallocate_Call);

	PVOID CL_ReallocateDynamicData_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(CL_Reallocate_Call, 0x100, [](PUCHAR Candidate) {
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

	Sig_VarNotFound(CL_ReallocateDynamicData_VA);

	typedef struct CL_ReallocateDynamicData_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		PVOID CL_Reallocate_Call{};
	} CL_ReallocateDynamicData_SearchContext;

	CL_ReallocateDynamicData_SearchContext ctx = { DllInfo, RealDllInfo, CL_Reallocate_Call };

	g_pMetaHookAPI->DisasmRanges(CL_ReallocateDynamicData_VA, 0x150, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (CL_ReallocateDynamicData_SearchContext*)context;

		if (!cl_max_edicts && pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			// mov     eax, cl_max_edicts
			// add     esp, 4
			if (0 == memcmp(address + instLen, "\x83\xC4\x04", 3))
			{
				cl_max_edicts = (decltype(cl_max_edicts))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}
		}

		if (!cl_max_edicts && pinst->id == X86_INS_IMUL &&
			pinst->detail->x86.op_count == 3 &&
			pinst->detail->x86.operands[0].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].type == X86_OP_MEM &&
			pinst->detail->x86.operands[2].type == X86_OP_IMM &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			cl_max_edicts = (decltype(cl_max_edicts))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (!cl_entities && address > (PUCHAR)ctx->CL_Reallocate_Call && pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[1].reg == X86_REG_EAX &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			cl_entities = (decltype(cl_entities))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
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

void Engine_FillAddress_TempEntsVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		TEMPENTITY *gTempEnts = NULL;
	*/

	if (g_iEngineType == ENGINE_SVENGINE)
	{
#define GTEMPENTS_SIG_SVENGINE "\x68\x00\xE0\x5F\x00\x6A\x00\x68\x2A\x2A\x2A\x2A\xA3"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(GTEMPENTS_SIG_SVENGINE, DllInfo);
		Sig_AddrNotFound(gTempEnts);
		PVOID gTempEnts_VA = *(PVOID*)(addr + 8);
		gTempEnts = (decltype(gTempEnts))ConvertDllInfoSpace(gTempEnts_VA, DllInfo, RealDllInfo);
	}
	else
	{
#define GTEMPENTS_SIG_NEW "\x68\x30\x68\x17\x00\x6A\x00\x68\x2A\x2A\x2A\x2A\xE8"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(GTEMPENTS_SIG_NEW, DllInfo);
		Sig_AddrNotFound(gTempEnts);
		PVOID gTempEnts_VA = *(PVOID*)(addr + 8);
		gTempEnts = (decltype(gTempEnts))ConvertDllInfoSpace(gTempEnts_VA, DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(gTempEnts);
}

void Engine_FillAddress_WaterVars(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		float *gWaterColor = NULL;
		float *cshift_water = NULL;
	*/

	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
	{
#define GWATERCOLOR_SIG_HL25 "\x66\x0F\x6E\x05\x2A\x2A\x2A\x2A\xF2\x0F\x10\x0D\x2A\x2A\x2A\x2A\xF3\x0F\xE6\xC0\x68\x01\x26\x00\x00"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(GWATERCOLOR_SIG_HL25, DllInfo);
		Sig_AddrNotFound(gWaterColor);
		PVOID gWaterColor_VA = *(PVOID*)((PUCHAR)addr + 4);
		PVOID cshift_water_VA = (PVOID)((ULONG_PTR)gWaterColor_VA + 12);
		gWaterColor = (decltype(gWaterColor))ConvertDllInfoSpace(gWaterColor_VA, DllInfo, RealDllInfo);
		cshift_water = (decltype(cshift_water))ConvertDllInfoSpace(cshift_water_VA, DllInfo, RealDllInfo);
	}
	else
	{
#define GWATERCOLOR_SIG "\xDB\x05\x2A\x2A\x2A\x2A\x68\x01\x26\x00\x00\x68\x65\x0B\x00\x00"
		ULONG_PTR addr = (ULONG_PTR)Search_Pattern(GWATERCOLOR_SIG, DllInfo);
		Sig_AddrNotFound(gWaterColor);
		PVOID gWaterColor_VA = *(PVOID*)((PUCHAR)addr + 2);
		PVOID cshift_water_VA = (PVOID)((ULONG_PTR)gWaterColor_VA + 12);
		gWaterColor = (decltype(gWaterColor))ConvertDllInfoSpace(gWaterColor_VA, DllInfo, RealDllInfo);
		cshift_water = (decltype(cshift_water))ConvertDllInfoSpace(cshift_water_VA, DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(gWaterColor);
	Sig_VarNotFound(cshift_water);
}

void Engine_FillAddress_ModKnown(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	//Global pointers that link into engine vars
		model_t *mod_known = NULL;
	*/

	const char pattern[] = "\xB8\x9D\x82\x97\x53\x81\xE9";
	ULONG_PTR addr = (ULONG_PTR)Search_Pattern(pattern, DllInfo);
	Sig_AddrNotFound(mod_known);

	PVOID mod_known_VA = *(PVOID*)(addr + 7);
	mod_known = (decltype(mod_known))ConvertDllInfoSpace(mod_known_VA, DllInfo, RealDllInfo);

	Sig_VarNotFound(mod_known);
}

void Engine_FillAddress_Mod_NumKnown(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		//Global pointers that link into engine vars
		int *mod_numknown = NULL;
	*/

	typedef struct Mod_NumKnown_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} Mod_NumKnown_SearchContext;

	const char sigs[] = "Cached models:\n";
	auto Mod_Print_String = Search_Pattern_Data(sigs, DllInfo);
	if (!Mod_Print_String)
		Mod_Print_String = Search_Pattern_Rdata(sigs, DllInfo);
	Sig_VarNotFound(Mod_Print_String);

	char pattern[] = "\x57\x68\x2A\x2A\x2A\x2A\xE8";
	*(DWORD*)(pattern + 2) = (DWORD)Mod_Print_String;
	auto Mod_Print_Call = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(Mod_Print_Call);

	Mod_NumKnown_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(Mod_Print_Call, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {
			auto pinst = (cs_insn*)inst;
			auto ctx = (Mod_NumKnown_SearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0)
			{//A1 84 5C 32 02 mov     eax, mod_numknown
				mod_numknown = (decltype(mod_numknown))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}
			else if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG)
			{//39 3D 44 32 90 03 cmp     mod_numknown, edi
				mod_numknown = (decltype(mod_numknown))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (mod_numknown)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(mod_numknown);
}

void Engine_FillAddress_Mod_LoadStudioModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs[] = "bogus\0";
	auto Bogus_String = Search_Pattern_Data(sigs, DllInfo);
	if (!Bogus_String)
		Bogus_String = Search_Pattern_Rdata(sigs, DllInfo);
	Sig_VarNotFound(Bogus_String);

	char pattern[] = "\x68\x2A\x2A\x2A\x2A\x2A\xE8";
	*(DWORD*)(pattern + 1) = (DWORD)Bogus_String;
	auto Bogus_Call = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(Bogus_Call);

	typedef struct Mod_LoadStudioModel_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} Mod_LoadStudioModel_SearchContext;

	Mod_LoadStudioModel_SearchContext ctx = { DllInfo, RealDllInfo };

	PVOID Mod_LoadStudioModel_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Bogus_Call, 0x50, [](PUCHAR Candidate) {
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

	gPrivateFuncs.Mod_LoadStudioModel = (decltype(gPrivateFuncs.Mod_LoadStudioModel))ConvertDllInfoSpace(Mod_LoadStudioModel_VA, DllInfo, RealDllInfo);

	Sig_FuncNotFound(Mod_LoadStudioModel);
}

void Engine_FillAddress_Mod_LoadBrushModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	const char sigs[] = "Mod_LoadBrushModel: %s has wrong version number";
	auto Bogus_String = Search_Pattern_Data(sigs, DllInfo);
	if (!Bogus_String)
		Bogus_String = Search_Pattern_Rdata(sigs, DllInfo);
	Sig_VarNotFound(Bogus_String);

	char pattern[] = "\x68\x2A\x2A\x2A\x2A\x6A\x01\xE8";
	*(DWORD*)(pattern + 1) = (DWORD)Bogus_String;
	auto Bogus_Call = Search_Pattern(pattern, DllInfo);
	if (!Bogus_Call) {
		char pattern2[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83";
		*(DWORD*)(pattern2 + 1) = (DWORD)Bogus_String;
		Bogus_Call = Search_Pattern(pattern2, DllInfo);
	}
	Sig_VarNotFound(Bogus_Call);

	typedef struct Mod_LoadBrushModel_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} Mod_LoadBrushModel_SearchContext;

	Mod_LoadBrushModel_SearchContext ctx = { DllInfo, RealDllInfo };

	PVOID Mod_LoadBrushModel_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Bogus_Call, 0x150, [](PUCHAR Candidate) {
		//.text:01D078D0 83 EC 34												sub     esp, 34h
		//.text : 01D078D3 A1 E8 F0 ED 01										mov     eax, ___security_cookie
		//	.text : 01D078D8 33 C4                                              xor     eax, esp
		if (Candidate[0] == 0x83 &&
			Candidate[1] == 0xEC &&
			Candidate[3] == 0xA1)
		{
			return TRUE;
		}

		//  .text : 01D61AD0 55                                                  push    ebp
		//  .text : 01D61AD1 8B EC                                               mov     ebp, esp
		//	.text : 01D61AD3 83 EC ??											 sub     esp, ??
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

	gPrivateFuncs.Mod_LoadBrushModel = (decltype(gPrivateFuncs.Mod_LoadBrushModel))ConvertDllInfoSpace(Mod_LoadBrushModel_VA, DllInfo,  RealDllInfo);

	Sig_FuncNotFound(Mod_LoadBrushModel);
}

void Engine_FillAddress_Mod_LoadModel(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	 //Global pointers that link into engine vars.
		char (*loadname)[64] = NULL;
		model_t **loadmodel = NULL;
	*/

	PVOID Mod_LoadModel_String = NULL;

	if (g_iEngineType == ENGINE_SVENGINE)
	{
		const char sigs[] = "Loading '%s'\n";
		Mod_LoadModel_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Mod_LoadModel_String)
			Mod_LoadModel_String = Search_Pattern_Rdata(sigs, DllInfo);
	}
	else
	{
		const char sigs[] = "loading %s\n";
		Mod_LoadModel_String = Search_Pattern_Data(sigs, DllInfo);
		if (!Mod_LoadModel_String)
			Mod_LoadModel_String = Search_Pattern_Rdata(sigs, DllInfo);
	}

	Sig_VarNotFound(Mod_LoadModel_String);

	char pattern[] = "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4";
	*(DWORD*)(pattern + 1) = (DWORD)Mod_LoadModel_String;
	auto Mod_LoadModel_PushString = Search_Pattern(pattern, DllInfo);
	Sig_VarNotFound(Mod_LoadModel_PushString);

	PVOID Mod_LoadModel_VA = (PVOID)g_pMetaHookAPI->ReverseSearchFunctionBeginEx(Mod_LoadModel_PushString, 0x600, [](PUCHAR Candidate) {
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

	gPrivateFuncs.Mod_LoadModel = (decltype(gPrivateFuncs.Mod_LoadModel))ConvertDllInfoSpace(Mod_LoadModel_VA, DllInfo, RealDllInfo);
	Sig_FuncNotFound(Mod_LoadModel);

	typedef struct Mod_LoadModel_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		PVOID loadname_nextaddr{};
	} Mod_LoadModel_SearchContext;

	Mod_LoadModel_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges((PUCHAR)Mod_LoadModel_PushString + 5, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (Mod_LoadModel_SearchContext*)context;

		if (pinst->id == X86_INS_PUSH &&
			pinst->detail->x86.op_count == 1 &&
			pinst->detail->x86.operands[0].type == X86_OP_IMM &&
			(PUCHAR)pinst->detail->x86.operands[0].imm > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].imm < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			loadname = (decltype(loadname))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].imm, ctx->DllInfo, ctx->RealDllInfo);
			ctx->loadname_nextaddr = address + instLen;
			return TRUE;
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

	typedef struct Mod_LoadModel_SearchContext2_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} Mod_LoadModel_SearchContext2;

	Mod_LoadModel_SearchContext2 ctx2 = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(ctx.loadname_nextaddr, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (Mod_LoadModel_SearchContext2*)context;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			pinst->detail->x86.operands[0].mem.index == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			loadmodel = (decltype(loadmodel))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			return TRUE;
		}

		if (loadmodel)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, &ctx2);

	Sig_VarNotFound(loadmodel);
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

void Engine_FillAddress_SetFilterMode(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		int *filterMode = NULL;
	*/

	PVOID pfnSetFilterMode_VA = ConvertDllInfoSpace(gEngfuncs.pfnSetFilterMode, RealDllInfo, DllInfo);

	if (!pfnSetFilterMode_VA)
	{
		Sig_NotFound(pfnSetFilterMode_VA);
	}

	typedef struct
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} SetFilterMode_SearchContext;

	SetFilterMode_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(pfnSetFilterMode_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (SetFilterMode_SearchContext*)context;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[1].type == X86_OP_REG &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[0].mem.base == 0 &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
		{
			//.text:01D1B114 A3 AC 44 F5 03                                      mov     filterMode, eax
			filterMode = (decltype(filterMode))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
		}

		if (filterMode)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
		}, 0, &ctx);

	Sig_VarNotFound(filterMode);
}

void Engine_FillAddress_SetFilterColor(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		float *filterColorRed = NULL;
		float *filterColorGreen = NULL;
		float *filterColorBlue = NULL;
	*/

	PVOID pfnSetFilterColor_VA = ConvertDllInfoSpace(gEngfuncs.pfnSetFilterColor, RealDllInfo, DllInfo);

	if (!pfnSetFilterColor_VA)
	{
		Sig_NotFound(pfnSetFilterColor_VA);
	}

	typedef struct SetFilterColor_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
		ULONG_PTR CandidateVA[3];
		int CandidateCount{};
	}SetFilterColor_SearchContext;

	SetFilterColor_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(pfnSetFilterColor_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (SetFilterColor_SearchContext*)context;

		if (ctx->CandidateCount < 3)
		{
			if (g_iEngineType == ENGINE_SVENGINE)
			{
				if (pinst->id == X86_INS_FSTP &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					//  .text : 01D1B120 D9 44 24 04                                         fld[esp + arg_0]
					//	.text : 01D1B124 D9 1D 90 83 ED 01                                   fstp    r_filter_color
					//	.text : 01D1B12A D9 44 24 08                                         fld[esp + arg_4]
					//	.text : 01D1B12E D9 1D 94 83 ED 01                                   fstp    r_filter_color + 4
					//	.text : 01D1B134 D9 44 24 0C                                         fld[esp + arg_8]
					//	.text : 01D1B138 D9 1D 98 83 ED 01                                   fstp    r_filter_color + 8
					//	.text : 01D1B13E C3                                                  retn
					ctx->CandidateVA[ctx->CandidateCount] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->CandidateCount++;
				}
			}
			else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
			{
				if (pinst->id == X86_INS_MOVSS &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					//  .text : 01D1B120 D9 44 24 04                                         fld[esp + arg_0]
					//	.text : 01D1B124 D9 1D 90 83 ED 01                                   fstp    r_filter_color
					//	.text : 01D1B12A D9 44 24 08                                         fld[esp + arg_4]
					//	.text : 01D1B12E D9 1D 94 83 ED 01                                   fstp    r_filter_color + 4
					//	.text : 01D1B134 D9 44 24 0C                                         fld[esp + arg_8]
					//	.text : 01D1B138 D9 1D 98 83 ED 01                                   fstp    r_filter_color + 8
					//	.text : 01D1B13E C3                                                  retn
					ctx->CandidateVA[ctx->CandidateCount] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
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
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					//.text:01D115C1 A3 C4 71 E4 01                                      mov     filterColorRed, eax
					ctx->CandidateVA[ctx->CandidateCount] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
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

	if (ctx.CandidateCount >= 3)
	{
		std::qsort(ctx.CandidateVA, ctx.CandidateCount, sizeof(ctx.CandidateVA[0]), [](const void* a, const void* b) {
			return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
		});

		filterColorRed = (decltype(filterColorRed))ConvertDllInfoSpace((PVOID)ctx.CandidateVA[0], DllInfo, RealDllInfo);
		filterColorGreen = (decltype(filterColorGreen))ConvertDllInfoSpace((PVOID)ctx.CandidateVA[1], DllInfo, RealDllInfo);
		filterColorBlue = (decltype(filterColorBlue))ConvertDllInfoSpace((PVOID)ctx.CandidateVA[2], DllInfo, RealDllInfo);
	}

	Sig_VarNotFound(filterColorRed);
	Sig_VarNotFound(filterColorGreen);
	Sig_VarNotFound(filterColorBlue);
}

void Engine_FillAddress_SetFilterBrightness(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		float *filterBrightness = NULL;
	*/

	PVOID pfnSetFilterBrightness_VA = ConvertDllInfoSpace(gEngfuncs.pfnSetFilterBrightness, RealDllInfo, DllInfo);

	if (!pfnSetFilterBrightness_VA)
	{
		Sig_NotFound(pfnSetFilterBrightness_VA);
	}

	typedef struct SetFilterBrightness_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	} SetFilterBrightness_SearchContext;

	SetFilterBrightness_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(pfnSetFilterBrightness_VA, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (SetFilterBrightness_SearchContext*)context;

		if (g_iEngineType == ENGINE_SVENGINE)
		{
			if (pinst->id == X86_INS_FSTP &&
				pinst->detail->x86.op_count == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				//.text:01D1B114 A3 AC 44 F5 03                                      mov     filterMode, eax
				filterBrightness = (decltype(filterBrightness))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}
		}
		else if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		{
			if (pinst->id == X86_INS_MOVSS &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				filterBrightness = (decltype(filterBrightness))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}
		}
		else
		{
			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				filterBrightness = (decltype(filterBrightness))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}
		}

		if (filterBrightness)
			return TRUE;

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;
	}, 0, &ctx);

	Sig_VarNotFound(filterBrightness);
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

void Engine_FillAddress_DT_Initialize(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID detTexSupportedCallVA = 0;
	{
		/*
	.text:01D4780A 68 73 85 00 00                                      push    8573h           ; pname
	.text:01D4780F 68 00 23 00 00                                      push    2300h           ; target
	.text:01D47814 FF D6                                               call    esi ; glTexEnvf
			*/
		const char pattern[] = "\x68\x73\x85\x00\x00\x68\x00\x23\x00\x00\xFF";

		auto addr = Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(detTexSupportedCallVA);

		detTexSupportedCallVA = addr;

		PVOID DT_Initialize_VA = g_pMetaHookAPI->ReverseSearchFunctionBeginEx((void*)detTexSupportedCallVA, 0x100, [](PUCHAR Candidate) {

			if (Candidate[-1] == 0xC3 &&
				Candidate[0] == 0x56 &&
				Candidate[1] == 0x68)
				return TRUE;

			if (Candidate[0] == 0x68 &&
				Candidate[5] == 0xE8)
				return TRUE;

			return FALSE;
		});
		gPrivateFuncs.DT_Initialize = (decltype(gPrivateFuncs.DT_Initialize))ConvertDllInfoSpace(DT_Initialize_VA, DllInfo, RealDllInfo);
	}

	Sig_FuncNotFound(DT_Initialize);

	{
		typedef struct DT_Initialize_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		} DT_Initialize_SearchContext;

		DT_Initialize_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(detTexSupportedCallVA, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (DT_Initialize_SearchContext*)context;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 1 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].size == 1 &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				detTexSupported = (decltype(detTexSupported))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			}

			if (detTexSupported)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);
	}

	Sig_VarNotFound(detTexSupported);
}

void Engine_FillAddress(const mh_dll_info_t &DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto hSDL2 = GetModuleHandleA("SDL2.dll");

	if (hSDL2)
	{
		gPrivateFuncs.SDL_GL_SetAttribute = (decltype(gPrivateFuncs.SDL_GL_SetAttribute))GetProcAddress(hSDL2, "SDL_GL_SetAttribute");
	}

	Engine_FillAddress_EngineSurface(DllInfo, RealDllInfo);

	gPrivateFuncs.triapi_RenderMode = gEngfuncs.pTriAPI->RenderMode;
	gPrivateFuncs.triapi_GetMatrix = gEngfuncs.pTriAPI->GetMatrix;
	gPrivateFuncs.triapi_BoxInPVS = gEngfuncs.pTriAPI->BoxInPVS;
	gPrivateFuncs.triapi_Fog = gEngfuncs.pTriAPI->Fog;
	//gPrivateFuncs.triapi_Color4f = gEngfuncs.pTriAPI->Color4f;

	Engine_FillAddress_HasOfficialFBOSupport(DllInfo, RealDllInfo);

	Engine_FillAddress_HasOfficialGLTexAllocSupport(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_Init(DllInfo, RealDllInfo);

	Engine_FillAddress_R_PolyBlend(DllInfo, RealDllInfo);

	Engine_FillAddress_S_ExtraUpdate(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_Bind(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_SelectTexture(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_LoadTexture2(DllInfo, RealDllInfo);

	Engine_FillAddress_R_CullBox(DllInfo, RealDllInfo);

	Engine_FillAddress_R_SetupFrame(DllInfo, RealDllInfo);

	Engine_FillAddress_R_SetupGL(DllInfo, RealDllInfo);

	Engine_FillAddress_R_RenderView(DllInfo, RealDllInfo);

	Engine_FillAddress_V_RenderView(DllInfo, RealDllInfo);

	Engine_FillAddress_R_RenderScene(DllInfo, RealDllInfo);

	Engine_FillAddress_R_NewMap(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_BuildLightmaps(DllInfo, RealDllInfo);

	Engine_FillAddress_R_BuildLightMap(DllInfo, RealDllInfo);

	Engine_FillAddress_R_AddDynamicLights(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_DisableMultitexture(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_EnableMultitexture(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawSequentialPoly(DllInfo, RealDllInfo);

	Engine_FillAddress_R_TextureAnimation(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawBrushModel(DllInfo, RealDllInfo);

	Engine_FillAddress_R_RecursiveWorldNode(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawWorld(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawViewModel(DllInfo, RealDllInfo);

	Engine_FillAddress_R_MarkLeaves(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_BeginRendering(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_EndRendering(DllInfo, RealDllInfo);

	Engine_FillAddress_EmitWaterPolys(DllInfo, RealDllInfo);

	Engine_FillAddress_VID_UpdateWindowVars(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_PointInLeaf(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawTEntitiesOnList(DllInfo, RealDllInfo);

	Engine_FillAddress_BuildGammaTable(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawParticles(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_AllocDlight(DllInfo, RealDllInfo);

	Engine_FillAddress_R_GLStudioDrawPoints(DllInfo, RealDllInfo);

	Engine_FillAddress_R_StudioLighting(DllInfo, RealDllInfo);

	Engine_FillAddress_R_StudioChrome(DllInfo, RealDllInfo);

	Engine_FillAddress_R_LightLambert(DllInfo, RealDllInfo);

	Engine_FillAddress_R_StudioSetupSkin(DllInfo, RealDllInfo);

	Engine_FillAddress_Cache_Alloc(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_MiptexTexture(DllInfo, RealDllInfo);

	Engine_FillAddress_Draw_DecalTexture(DllInfo, RealDllInfo);

	Engine_FillAddress_R_DrawSpriteModel(DllInfo, RealDllInfo);

	Engine_FillAddress_R_LightStrength(DllInfo, RealDllInfo);

	Engine_FillAddress_R_RotateForEntity(DllInfo, RealDllInfo);

	Engine_FillAddress_R_GlowBlend(DllInfo, RealDllInfo);

	Engine_FillAddress_SCR_BeginLoadingPlaque(DllInfo, RealDllInfo);

	Engine_FillAddress_Host_IsSinglePlayerGame(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_UnloadSpriteTextures(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_LoadSpriteModel(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_LoadSpriteFrame(DllInfo, RealDllInfo);

	Engine_FillAddress_R_AddTEntity(DllInfo, RealDllInfo);

	Engine_FillAddress_Hunk_AllocName(DllInfo, RealDllInfo);

	Engine_FillAddress_GL_EndRenderingVars(DllInfo, RealDllInfo);

	Engine_FillAddress_VisEdicts(DllInfo, RealDllInfo);

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

	Engine_FillAddress_R_RenderDynamicLightmaps(DllInfo, RealDllInfo);

	Engine_FillAddress_R_StudioChromeVars(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_SimOrgVars(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_ViewEntityVars(DllInfo, RealDllInfo);

	Engine_FillAddress_CL_ReallocateDynamicData(DllInfo, RealDllInfo);

	Engine_FillAddress_TempEntsVars(DllInfo, RealDllInfo);

	Engine_FillAddress_WaterVars(DllInfo, RealDllInfo);

	Engine_FillAddress_ModKnown(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_NumKnown(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_LoadStudioModel(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_LoadBrushModel(DllInfo, RealDllInfo);

	Engine_FillAddress_Mod_LoadModel(DllInfo, RealDllInfo);

	Engine_FillAddress_BasePalette(DllInfo, RealDllInfo);

	Engine_FillAddress_R_LightStrengthVars(DllInfo, RealDllInfo);

	Engine_FillAddress_SetFilterMode(DllInfo, RealDllInfo);

	Engine_FillAddress_SetFilterColor(DllInfo, RealDllInfo);

	Engine_FillAddress_SetFilterBrightness(DllInfo, RealDllInfo);

	Engine_FillAddress_MoveVars(DllInfo, RealDllInfo);

	Engine_FillAddress_MissingTexture(DllInfo, RealDllInfo);

	Engine_FillAddress_NoTexture(DllInfo, RealDllInfo);

	Engine_FillAddress_DT_Initialize(DllInfo, RealDllInfo);
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
//static hook_t* g_phook_Draw_CustomCacheGet = NULL;
//static hook_t* g_phook_Draw_CacheGet = NULL;
static hook_t* g_phook_BuildGammaTable = NULL;
static hook_t* g_phook_DLL_SetModKey = NULL;
static hook_t* g_phook_SDL_GL_SetAttribute = NULL;

static hook_t* g_phook_ClientPortalManager_ResetAll = NULL;
static hook_t* g_phook_ClientPortalManager_DrawPortalSurface = NULL;
static hook_t* g_phook_ClientPortalManager_EnableClipPlane = NULL;
static hook_t* g_phook_UpdatePlayerPitch = NULL;

#if 0
#include <intrin.h>
void __stdcall NewglDeleteTextures(GLsizei n, const GLuint* textures)
{
	gEngfuncs.Con_DPrintf("glDeleteTextures: n=%d, texid=[%d], retaddr=%p.\n", n, textures[0], _ReturnAddress());

	gPrivateFuncs.glDeleteTextures(n, textures);
}
#endif

void Engine_InstallHooks(void)
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
	//Install_InlineHook(Draw_MiptexTexture);
	//Install_InlineHook(Draw_CustomCacheGet);
	//Install_InlineHook(Draw_CacheGet);
	Install_InlineHook(BuildGammaTable);
	Install_InlineHook(R_CullBox);

	//OpenGL4.2 was forced by HL25 engine which might ruin the renderer features.
	if (gPrivateFuncs.SDL_GL_SetAttribute)
	{
		Install_InlineHook(SDL_GL_SetAttribute);
	}
#if 0
	gPrivateFuncs.glDeleteTextures = glDeleteTextures;

	g_pMetaHookAPI->InlineHook(gPrivateFuncs.glDeleteTextures, NewglDeleteTextures, (void**)&gPrivateFuncs.glDeleteTextures);
#endif
}

void Engine_UninstallHooks(void)
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
	//Uninstall_Hook(Draw_CustomCacheGet);
	//Uninstall_Hook(Draw_CacheGet);
	Uninstall_Hook(BuildGammaTable);
	Uninstall_Hook(R_CullBox);

	if (gPrivateFuncs.SDL_GL_SetAttribute)
	{
		Uninstall_Hook(SDL_GL_SetAttribute);
	}
}

int WINAPI GL_RedirectedGenTexture(void)
{
	return GL_GenTexture();
}

/*
Purpose: Redirect all "mov eax, allocated_textures" to "call GL_RedirectedGenTexture" for legacy engine
*/

void R_RedirectLegacyOpenGLTextureAllocation(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
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

void R_PatchResetLatched(const mh_dll_info_t &DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_iEngineType == ENGINE_GOLDSRC_HL25)
		return;

	const char pattern[] = "\x6A\x01\x2A\x2A\x2A\x08\x03\x00\x00";

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
	while (SearchBegin < SearchLimit)
	{
		PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
		if (pFound)
		{
			typedef struct PatchResetLatched_SearchContext_s
			{
				const mh_dll_info_t& DllInfo;
				const mh_dll_info_t& RealDllInfo;
				bool bFoundMov308h{};
				bool bFoundResetLatched{};
			}PatchResetLatched_SearchContext;

			PatchResetLatched_SearchContext ctx = { DllInfo, RealDllInfo };

			g_pMetaHookAPI->DisasmRanges(pFound, 0x50, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

				auto pinst = (cs_insn*)inst;
				auto ctx = (PatchResetLatched_SearchContext*)context;

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

					PVOID R_ResetLatched_VA = GetCallAddress(address);

					gPrivateFuncs.R_ResetLatched = (decltype(gPrivateFuncs.R_ResetLatched))ConvertDllInfoSpace(R_ResetLatched_VA, ctx->DllInfo, ctx->RealDllInfo);

					auto Call_R_ResetLatched_RealDllBased = ConvertDllInfoSpace(address, ctx->DllInfo, ctx->RealDllInfo);

					g_pMetaHookAPI->InlinePatchRedirectBranch(Call_R_ResetLatched_RealDllBased, R_ResetLatched_Patched, NULL);

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

void Client_FillAddress_ClientPortalManager_ResetAll(const mh_dll_info_t &DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		.text:1004ADE3 ;   } // starts at 1004ADCD
		.text:1004ADE5                 mov     [ebp+var_4], 0FFFFFFFFh
		.text:1004ADEC                 mov     dword_1063C808, eax
		.text:1004ADF1                 call    ClientPortalManager_ResetAll
		.text:1004ADF6                 mov     ecx, dword_1063C808
		.text:1004ADFC
		.text:1004ADFC loc_1004ADFC:                           ; CODE XREF: HUD_DrawNormalTriangles_0+2B↑j
		.text:1004ADFC                 call    sub_1004F010
		.text:1004AE01                 mov     ecx, [ebp+var_C]
		.text:1004AE04                 mov     large fs:0, ecx
		.text:1004AE0B                 pop     ecx
		.text:1004AE0C                 mov     esp, ebp
		.text:1004AE0E                 pop     ebp
		.text:1004AE0F                 retn
		.text:1004AE0F ; } // starts at 1004AD90
		.text:1004AE0F HUD_DrawNormalTriangles_0 endp
	*/

	PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
	PUCHAR SearchLimit = SearchBegin + DllInfo.TextSize;

	const char pattern[] = "\xC7\x45\x2A\xFF\xFF\xFF\xFF\xA3\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x8B\x0D";

	ULONG_PTR addr = (ULONG_PTR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);

	Sig_AddrNotFound(ClientPortalManager_ResetAll);

	auto ClientPortalManager_ResetAll_VA = GetCallAddress(addr + 12);

	gPrivateFuncs.ClientPortalManager_ResetAll = (decltype(gPrivateFuncs.ClientPortalManager_ResetAll))ConvertDllInfoSpace(ClientPortalManager_ResetAll_VA, DllInfo, RealDllInfo);

	Sig_FuncNotFound(ClientPortalManager_ResetAll);
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
	/*
.text:1004F870                                     ClientPortalManager_EnableClipPlane proc near
.text:1004F870 83 EC 58                                            sub     esp, 58h
.text:1004F873 A1 40 E2 1B 10                                      mov     eax, ___security_cookie
.text:1004F878 33 C4                                               xor     eax, esp
.text:1004F87A 89 44 24 54                                         mov     [esp+58h+var_4], eax
.text:1004F87E 8B 4C 24 68                                         mov     ecx, [esp+58h+arg_C]
.text:1004F882 8D 54 24 24                                         lea     edx, [esp+58h+var_34]
.text:1004F886 8B 44 24 60                                         mov     eax, [esp+58h+arg_4]
	*/
	const char pattern[] = "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x2A\x44\x24\x2A\x2A\x2A\x24\x2A\x2A\x2A\x24\x2A\x2A\x44\x24\x2A\xF3\x0F";
	auto addr = Search_Pattern(pattern, DllInfo);

	Sig_AddrNotFound(ClientPortalManager_EnableClipPlane);

	gPrivateFuncs.ClientPortalManager_EnableClipPlane = (decltype(gPrivateFuncs.ClientPortalManager_EnableClipPlane))ConvertDllInfoSpace(addr, DllInfo, RealDllInfo);
}

void Client_FillAddress_UpdatePlayerPitch(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
		FF 73 40 E8 ?? ?? ?? ?? 83 C4 08 80 3D ?? ?? ?? ?? 00

		.text:10056F86 FF 73 40                                            push    dword ptr [ebx+40h] ; a1
		.text:10056F89 E8 62 B6 01 00                                      call    UpdatePlayerPitch
		.text:10056F8E 83 C4 08                                            add     esp, 8
		.text:10056F91 80 3D 05 C8 63 10 00                                cmp     g_bIsRenderingPortals, 0
	*/
	const char pattern[] = "\xFF\x73\x40\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x80\x3D\x2A\x2A\x2A\x2A\x00";
	auto addr = Search_Pattern(pattern, DllInfo);

	Sig_AddrNotFound(UpdatePlayerPitch);

	PVOID ClientPortalManager_EnableClipPlane_VA = GetCallAddress(addr + 3);

	gPrivateFuncs.UpdatePlayerPitch = (decltype(gPrivateFuncs.UpdatePlayerPitch))ConvertDllInfoSpace(ClientPortalManager_EnableClipPlane_VA, DllInfo, RealDllInfo);
}

void Client_FillAddress_RenderingPortals(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
.text:1004EAA8 6A 00                                               push    0
.text:1004EAAA 6A 00                                               push    0
.text:1004EAAC 6A 00                                               push    0
.text:1004EAAE 8B 01                                               mov     eax, [ecx]
.text:1004EAB0 FF 50 2C                                            call    dword ptr [eax+2Ch]
.text:1004EAB3 8B 35 A4 A1 11 10                                   mov     esi, ds:glDisable
.text:1004EAB9 33 D2                                               xor     edx, edx
.text:1004EABB C6 05 0D C8 63 10 01                                mov     g_bRenderingPortals, 1
	*/

	const char pattern[] = "\x6A\x00\x6A\x00\x6A\x00\x8B\x2A\xFF\x50\x2A";
	auto addr = Search_Pattern(pattern, DllInfo);
	Sig_AddrNotFound(g_bRenderingPortals);

	typedef struct RenderingPortals_SearchContext_s
	{
		const mh_dll_info_t& DllInfo;
		const mh_dll_info_t& RealDllInfo;
	}RenderingPortals_SearchContext;

	RenderingPortals_SearchContext ctx = { DllInfo, RealDllInfo };

	g_pMetaHookAPI->DisasmRanges(addr, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

		auto pinst = (cs_insn*)inst;
		auto ctx = (RenderingPortals_SearchContext*)context;

		if (pinst->id == X86_INS_MOV &&
			pinst->detail->x86.op_count == 2 &&
			pinst->detail->x86.operands[0].type == X86_OP_MEM &&
			pinst->detail->x86.operands[1].type == X86_OP_IMM &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
			(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
			pinst->detail->x86.operands[1].imm == 1)
		{
			g_bRenderingPortals_SCClient = (decltype(g_bRenderingPortals_SCClient))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[0].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
			return TRUE;
		}

		if (address[0] == 0xCC)
			return TRUE;

		if (pinst->id == X86_INS_RET)
			return TRUE;

		return FALSE;

	}, 0, &ctx);

	Sig_VarNotFound(g_bRenderingPortals_SCClient);
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

void Client_FillAddress_ViewEntityIndex(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	if (g_dwEngineBuildnum >= 10182)
	{
		const char pattern[] = "\xFF\x15\x2A\x2A\x2A\x2A\x85\xC0\x2A\x2A\x8B\x00\x2A\x05";
		auto addr = Search_Pattern(pattern, DllInfo);
		Sig_AddrNotFound(g_ViewEntityIndex_SCClient);

		typedef struct ViewEntityIndex_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
		}ViewEntityIndex_SearchContext;

		ViewEntityIndex_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(addr, 0x80, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto pinst = (cs_insn*)inst;
			auto ctx = (ViewEntityIndex_SearchContext*)context;

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
			{
				g_ViewEntityIndex_SCClient = (decltype(g_ViewEntityIndex_SCClient))ConvertDllInfoSpace((PVOID)pinst->detail->x86.operands[1].mem.disp, ctx->DllInfo, ctx->RealDllInfo);
				return TRUE;
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

			}, 0, &ctx);

		Sig_VarNotFound(g_ViewEntityIndex_SCClient);
	}
}

void Client_FillAddress_SCClient(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory)
	{
		auto SCClient001 = pfnClientFactory("SCClientDLL001", 0);

		if (SCClient001)
		{
			Client_FillAddress_ClientPortalManager_ResetAll(DllInfo, RealDllInfo);
			Client_FillAddress_ClientPortalManager_GetOriginalSurfaceTexture_DrawPortalSurface(DllInfo, RealDllInfo);
			Client_FillAddress_ClientPortalManager_EnableClipPlane(DllInfo, RealDllInfo);
			Client_FillAddress_UpdatePlayerPitch(DllInfo, RealDllInfo);
			Client_FillAddress_RenderingPortals(DllInfo, RealDllInfo);
			Client_FillAddress_ViewEntityIndex(DllInfo, RealDllInfo);
			Client_FillAddress_WaterLevel(DllInfo, RealDllInfo);
			Client_FillAddress_FogParams(DllInfo, RealDllInfo);

			g_bIsSvenCoop = true;
		}
	}
}

void Client_FillAddress_CL_IsThirdPerson(const mh_dll_info_t &DllInfo, const mh_dll_info_t& RealDllInfo)
{
	PVOID CL_IsThirdPerson = ConvertDllInfoSpace((void*)g_pMetaSave->pExportFuncs->CL_IsThirdPerson, RealDllInfo, DllInfo);

	if (!CL_IsThirdPerson)
	{
		if (g_pMetaHookAPI->GetClientModule())
		{
			CL_IsThirdPerson = ConvertDllInfoSpace(GetProcAddress(g_pMetaHookAPI->GetClientModule(), "CL_IsThirdPerson"), RealDllInfo, DllInfo);
		}
	}

	if (CL_IsThirdPerson)
	{
		typedef struct CL_IsThirdPerson_SearchContext_s
		{
			const mh_dll_info_t& DllInfo;
			const mh_dll_info_t& RealDllInfo;
			ULONG_PTR CandidateVA[16]{};
			int iNumCandidates{};
		}CL_IsThirdPerson_SearchContext;

		CL_IsThirdPerson_SearchContext ctx = { DllInfo, RealDllInfo };

		g_pMetaHookAPI->DisasmRanges(CL_IsThirdPerson, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

			auto ctx = (CL_IsThirdPerson_SearchContext*)context;
			auto pinst = (cs_insn*)inst;

			if (ctx->iNumCandidates < 16)
			{
				if (pinst->id == X86_INS_MOV &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					(
						pinst->detail->x86.operands[0].reg == X86_REG_EAX ||
						pinst->detail->x86.operands[0].reg == X86_REG_EBX ||
						pinst->detail->x86.operands[0].reg == X86_REG_ECX ||
						pinst->detail->x86.operands[0].reg == X86_REG_EDX ||
						pinst->detail->x86.operands[0].reg == X86_REG_ESI ||
						pinst->detail->x86.operands[0].reg == X86_REG_EDI
						) &&
					pinst->detail->x86.operands[1].type == X86_OP_MEM &&
					pinst->detail->x86.operands[1].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					ctx->CandidateVA[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[1].mem.disp;
					ctx->iNumCandidates++;
				}
			}

			if (ctx->iNumCandidates < 16)
			{
				if (pinst->id == X86_INS_CMP &&
					pinst->detail->x86.op_count == 2 &&
					pinst->detail->x86.operands[1].type == X86_OP_IMM &&
					pinst->detail->x86.operands[1].imm == 0 &&
					pinst->detail->x86.operands[0].type == X86_OP_MEM &&
					pinst->detail->x86.operands[0].mem.base == 0 &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
					(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize)
				{
					ctx->CandidateVA[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
					ctx->iNumCandidates++;
				}
			}

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;

		}, 0, &ctx);

		if (ctx.iNumCandidates >= 3 && ctx.CandidateVA[ctx.iNumCandidates - 1] == ctx.CandidateVA[ctx.iNumCandidates - 2] + sizeof(int))
		{
			g_iUser1 = (decltype(g_iUser1))ConvertDllInfoSpace((PVOID)ctx.CandidateVA[ctx.iNumCandidates - 2], DllInfo, RealDllInfo);
			g_iUser2 = (decltype(g_iUser2))ConvertDllInfoSpace((PVOID)ctx.CandidateVA[ctx.iNumCandidates - 1], DllInfo, RealDllInfo);
		}
	}
}

void Client_FillAddress_PlayerExtraInfo(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	/*
	 //Global pointers that link into client dll vars.
	extra_player_info_t(*g_PlayerExtraInfo)[65] = NULL;
	extra_player_info_czds_t(*g_PlayerExtraInfo_CZDS)[65] = NULL;
	*/

	//66 85 C0 66 89 ?? ?? ?? ?? ?? 66 89 ?? ?? ?? ?? ?? 66 89 ?? ?? ?? ?? ?? 66 89 ?? ?? ?? ?? ??
	/*
	.text:019A4575 66 85 C0                                            test    ax, ax
	.text:019A4578 66 89 99 20 F4 A2 01                                mov     word_1A2F420[ecx], bx
	.text:019A457F 66 89 A9 22 F4 A2 01                                mov     word_1A2F422[ecx], bp
	.text:019A4586 66 89 91 48 F4 A2 01                                mov     word_1A2F448[ecx], dx
	.text:019A458D 66 89 81 4A F4 A2 01                                mov     word_1A2F44A[ecx], ax
	*/
	if (1)
	{
		char pattern[] = "\x66\x89\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				typedef struct MsgFunc_ScoreInfo_SearchContext_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					ULONG_PTR Candidates[4]{};
					int iNumCandidates{};
				} MsgFunc_ScoreInfo_SearchContext;

				MsgFunc_ScoreInfo_SearchContext ctx = { DllInfo, RealDllInfo };

				g_pMetaHookAPI->DisasmRanges((void*)pFound, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto ctx = (MsgFunc_ScoreInfo_SearchContext*)context;
					auto pinst = (cs_insn*)inst;

					if (ctx->iNumCandidates < 4)
					{
						if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_MEM &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
							pinst->detail->x86.operands[1].type == X86_OP_REG &&
							pinst->detail->x86.operands[1].size == 2)
						{
							if (ctx->Candidates[0] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[1] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[2] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[3] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp
								)
							{
								ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
								ctx->iNumCandidates++;
							}
						}
						if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_MEM &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
							pinst->detail->x86.operands[1].type == X86_OP_IMM &&
							pinst->detail->x86.operands[1].size == 2 &&
							pinst->detail->x86.operands[1].imm == 0)
						{
							if (ctx->Candidates[0] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[1] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[2] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[3] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp
								)
							{
								ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
								ctx->iNumCandidates++;
							}
						}
					}

					if (ctx->iNumCandidates == 4)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

				}, 0, &ctx);

				if (ctx.iNumCandidates >= 3)
				{
					std::qsort(ctx.Candidates, ctx.iNumCandidates, sizeof(ctx.Candidates[0]), [](const void* a, const void* b) {
						return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
						});

					if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
					{
						if (ctx.Candidates[ctx.iNumCandidates - 2] +
							(offsetof(extra_player_info_czds_t, teamnumber) - offsetof(extra_player_info_czds_t, playerclass))
							==
							ctx.Candidates[ctx.iNumCandidates - 1])
						{
							PVOID playerExtraInfo_VA = (PVOID)(ctx.Candidates[ctx.iNumCandidates - 1] - offsetof(extra_player_info_czds_t, teamnumber));
							g_PlayerExtraInfo_CZDS = (decltype(g_PlayerExtraInfo_CZDS))ConvertDllInfoSpace(playerExtraInfo_VA, ctx.DllInfo, ctx.RealDllInfo);
							break;
						}
					}
					else
					{
						if (ctx.Candidates[ctx.iNumCandidates - 2] +
							(offsetof(extra_player_info_t, teamnumber) - offsetof(extra_player_info_t, playerclass))
							== ctx.Candidates[ctx.iNumCandidates - 1])
						{
							PVOID playerExtraInfo_VA = (PVOID)(ctx.Candidates[ctx.iNumCandidates - 1] - offsetof(extra_player_info_t, teamnumber));
							g_PlayerExtraInfo = (decltype(g_PlayerExtraInfo))ConvertDllInfoSpace(playerExtraInfo_VA, ctx.DllInfo, ctx.RealDllInfo);
							break;
						}
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

	if (!g_PlayerExtraInfo)
	{
		//For HL25
		char pattern[] = "\x66\x89\x2A\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A\x2A\x66\x89\x2A\x2A\x2A\x2A\x2A\x2A";
		PUCHAR SearchBegin = (PUCHAR)DllInfo.TextBase;
		PUCHAR SearchLimit = (PUCHAR)DllInfo.TextBase + DllInfo.TextSize;
		while (SearchBegin < SearchLimit)
		{
			PUCHAR pFound = (PUCHAR)Search_Pattern_From_Size(SearchBegin, SearchLimit - SearchBegin, pattern);
			if (pFound)
			{
				typedef struct MsgFunc_ScoreInfo_ctx_s
				{
					const mh_dll_info_t& DllInfo;
					const mh_dll_info_t& RealDllInfo;
					ULONG_PTR Candidates[4]{};
					int iNumCandidates{};
				} MsgFunc_ScoreInfo_ctx;

				MsgFunc_ScoreInfo_ctx ctx = { DllInfo, RealDllInfo };

				g_pMetaHookAPI->DisasmRanges((void*)pFound, 0x100, [](void* inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context) {

					auto ctx = (MsgFunc_ScoreInfo_ctx*)context;
					auto pinst = (cs_insn*)inst;

					if (ctx->iNumCandidates < 4)
					{
						if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_MEM &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
							pinst->detail->x86.operands[1].type == X86_OP_REG &&
							pinst->detail->x86.operands[1].size == 2)
						{
							if (ctx->Candidates[0] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[1] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[2] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[3] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp
								)
							{
								ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
								ctx->iNumCandidates++;
							}
						}
						if (pinst->id == X86_INS_MOV &&
							pinst->detail->x86.op_count == 2 &&
							pinst->detail->x86.operands[0].type == X86_OP_MEM &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)ctx->DllInfo.DataBase &&
							(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)ctx->DllInfo.DataBase + ctx->DllInfo.DataSize &&
							pinst->detail->x86.operands[1].type == X86_OP_IMM &&
							pinst->detail->x86.operands[1].size == 2 &&
							pinst->detail->x86.operands[1].imm == 0)
						{
							if (ctx->Candidates[0] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[1] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[2] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp &&
								ctx->Candidates[3] != (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp
								)
							{
								ctx->Candidates[ctx->iNumCandidates] = (ULONG_PTR)pinst->detail->x86.operands[0].mem.disp;
								ctx->iNumCandidates++;
							}
						}
					}

					if (ctx->iNumCandidates == 4)
						return TRUE;

					if (address[0] == 0xCC)
						return TRUE;

					if (pinst->id == X86_INS_RET)
						return TRUE;

					return FALSE;

					}, 0, &ctx);

				if (ctx.iNumCandidates >= 3)
				{
					std::qsort(ctx.Candidates, ctx.iNumCandidates, sizeof(ctx.Candidates[0]), [](const void* a, const void* b) {
						return (int)(*(LONG_PTR*)a - *(LONG_PTR*)b);
						});

					if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
					{
						if (ctx.Candidates[ctx.iNumCandidates - 2] +
							(offsetof(extra_player_info_czds_t, teamnumber) - offsetof(extra_player_info_czds_t, playerclass))
							==
							ctx.Candidates[ctx.iNumCandidates - 1])
						{
							PVOID playerExtraInfo_VA = (PVOID)(ctx.Candidates[ctx.iNumCandidates - 1] - offsetof(extra_player_info_czds_t, teamnumber));
							g_PlayerExtraInfo_CZDS = (decltype(g_PlayerExtraInfo_CZDS))ConvertDllInfoSpace(playerExtraInfo_VA, ctx.DllInfo, ctx.RealDllInfo);
							break;
						}
					}
					else
					{
						if (ctx.Candidates[ctx.iNumCandidates - 2] +
							(offsetof(extra_player_info_t, teamnumber) - offsetof(extra_player_info_t, playerclass))
							== ctx.Candidates[ctx.iNumCandidates - 1])
						{
							PVOID playerExtraInfo_VA = (PVOID)(ctx.Candidates[ctx.iNumCandidates - 1] - offsetof(extra_player_info_t, teamnumber));
							g_PlayerExtraInfo = (decltype(g_PlayerExtraInfo))ConvertDllInfoSpace(playerExtraInfo_VA, ctx.DllInfo, ctx.RealDllInfo);
							break;
						}
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
	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror")) {
		Sig_VarNotFound(g_PlayerExtraInfo_CZDS);
	}
	else {
		Sig_VarNotFound(g_PlayerExtraInfo);
	}
}

void Client_FillAddress(const mh_dll_info_t& DllInfo, const mh_dll_info_t& RealDllInfo)
{
	Client_FillAddress_CL_IsThirdPerson(DllInfo, RealDllInfo);
	Client_FillAddress_SCClient(DllInfo, RealDllInfo);

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "cstrike") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czero") || !strcmp(gEngfuncs.pfnGetGameDirectory(), "czeror"))
	{
		g_bIsCounterStrike = true;

		Client_FillAddress_PlayerExtraInfo(DllInfo, RealDllInfo);
	}

	if (!strcmp(gEngfuncs.pfnGetGameDirectory(), "aomdc"))
	{
		g_bIsAoMDC = true;
	}
}

void Client_InstallHooks()
{
	if (gPrivateFuncs.ClientPortalManager_ResetAll)
	{
		Install_InlineHook(ClientPortalManager_ResetAll);
	}

	if (gPrivateFuncs.ClientPortalManager_DrawPortalSurface)
	{
		Install_InlineHook(ClientPortalManager_DrawPortalSurface);
	}

	if (gPrivateFuncs.ClientPortalManager_EnableClipPlane)
	{
		Install_InlineHook(ClientPortalManager_EnableClipPlane);
	}

	if (gPrivateFuncs.UpdatePlayerPitch)
	{
		Install_InlineHook(UpdatePlayerPitch);
	}
}

void Client_UninstallHooks()
{
	Uninstall_Hook(ClientPortalManager_ResetAll);
	Uninstall_Hook(ClientPortalManager_DrawPortalSurface);
	Uninstall_Hook(ClientPortalManager_EnableClipPlane);
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