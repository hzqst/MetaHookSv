#include "gl_local.h"

bool g_bIsSCClientParticleDrawing = false;
SCClientPortalLayout g_SCClientPortalLayout;

void __fastcall CParticleSystem_ParticleDraw(void* pthis, int, void* particle)
{
	// Restore the outer scope as well as the normal non-particle state.
	struct ParticleDrawingScope
	{
		bool previous = g_bIsSCClientParticleDrawing;
		ParticleDrawingScope() { g_bIsSCClientParticleDrawing = true; }
		~ParticleDrawingScope() { g_bIsSCClientParticleDrawing = previous; }
	} scope;
	gPrivateFuncs.CParticleSystem_ParticleDraw(pthis, 0, particle);
}

void __stdcall CoreProfile_glBegin(int GLPrimitiveCode)
{
	if (g_bIsSCClientParticleDrawing)
		triapi_glBegin(GLPrimitiveCode);
}

void __stdcall CoreProfile_glEnd()
{
	if (g_bIsSCClientParticleDrawing)
		triapi_glEnd();
}

void __stdcall CoreProfile_glNormal3f(float, float, float)
{
}

void __fastcall ClientPortalManager_InitShader(void* pthis, int)
{
	// Installed before client initialization, so the legacy program is never
	// created. Both inlined UseProgram branches test this byte after InitShader.
	*((byte*)pthis + gPrivateFuncs.offset_ClientPortalManager_m_bShadersAvailable) = 0;
}

int ClientPortal_GetIndex(void* pClientPortal)
{
	auto vectorBase = *(void***)((byte*)g_pClientPortalManager + g_SCClientPortalLayout.vectorBegin);
	auto vectorEnd = *(void***)((byte*)g_pClientPortalManager + g_SCClientPortalLayout.vectorEnd);
	int index = 0;
	for (auto it = vectorBase; it != vectorEnd; ++it, ++index)
	{
		if (*it == pClientPortal)
			return index;
	}
	return -1;
}

int ClientPortal_GetTextureId(void* pClientPortal)
{
	return *(int*)((byte*)pClientPortal + g_SCClientPortalLayout.textureId);
}

int ClientPortal_GetTextureWidth(void* pClientPortal)
{
	return *(int*)((byte*)pClientPortal + g_SCClientPortalLayout.textureWidth);
}

int ClientPortal_GetTextureHeight(void* pClientPortal)
{
	return *(int*)((byte*)pClientPortal + g_SCClientPortalLayout.textureHeight);
}
