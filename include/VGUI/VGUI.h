#ifndef VGUI_H
#define VGUI_H

#ifdef _WIN32
#pragma once
#endif

#define null 0L

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

#pragma warning(disable:4800)
#pragma warning(disable:4786)
#pragma warning(disable:4355)
#pragma warning(disable:4097)
#pragma warning(disable:4514)
#pragma warning(disable:4100)
#pragma warning(disable:4127)

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#ifndef _WCHAR_T_DEFINED
#if !defined(__x86_64__) && !defined(__WCHAR_TYPE__ )
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif

namespace vgui
{
typedef unsigned int VPANEL;
typedef unsigned long HScheme;
typedef unsigned long HTexture;
typedef unsigned long HCursor;
typedef unsigned long HPanel;
const HPanel INVALID_PANEL = 0xffffffff;
typedef unsigned long HFont;
const HFont INVALID_FONT = 0;
}

#endif