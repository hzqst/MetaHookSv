#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>

// The capture backend only needs dimensions and logging from its host.
// This standalone test adapter does not load or link the MetaHook framework.
using byte = unsigned char;

struct CaptureTestVideoAPI
{
    void GetVideoMode(int* width, int* height, void*, void*);
};

struct CaptureTestEngine
{
    void Con_Printf(const char* format, ...);
};

extern CaptureTestVideoAPI* g_pMetaHookAPI;
extern CaptureTestEngine gEngfuncs;
