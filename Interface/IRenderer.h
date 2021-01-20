#ifndef IRENDERER_H
#define IRENDERER_H

#ifdef _WIN32
#pragma once
#endif

#ifndef RENDERER_EXPORTS
	#include "ref_int.h"
#else
	#include "ref_int_internal.h"
#endif

class IRenderer : public IBaseInterface
{
public:
	virtual void GetInterface(ref_export_t *pRefExports, const char *version);
};

#define RENDERER_API_VERSION "RENDER_API_VERSION001"

extern IRenderer *g_pRenderer;

#endif