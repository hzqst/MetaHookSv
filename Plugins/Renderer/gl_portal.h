#pragma once

#ifndef GL_PORTAL_H
#define GL_PORTAL_H

#include <set>
#include "mathlib2.h"

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

class CWorldPortalModelHash
{
public:
	CWorldPortalModelHash(void *a1, GLuint a2, GLuint a3)
	{
		ClientPortal = a1;
		overlayTextureId = a2;
		textureId = a3;
	}
	void *ClientPortal;
	GLuint overlayTextureId;
	GLuint textureId;

	bool operator==(const CWorldPortalModelHash& p) const
	{
		return ClientPortal == p.ClientPortal && overlayTextureId == p.overlayTextureId && textureId == p.textureId;
	}
};

class CWorldPortalModelHasher
{
public:
	size_t operator()(const CWorldPortalModelHash& p) const {
		size_t seed = 0;
		hash_combine(seed, p.ClientPortal);
		hash_combine(seed, p.overlayTextureId);
		hash_combine(seed, p.textureId);
		return seed;
	}
};

class CWorldPortalModel
{
public:
	~CWorldPortalModel();

	GLuint hABO{};

	std::weak_ptr<CWorldSurfaceWorldModel> m_pWorldModel{};
	mtexinfo_t* texinfo{};
	std::set<int> SurfaceSet;
	uint32_t drawCount{};
	uint32_t polyCount{};
	std::vector<CDrawIndexAttrib> vDrawAttribBuffer;
};

typedef struct
{
	int program;
	int u_entityMatrix;
}portal_program_t;

typedef struct portal_texture_s
{
	struct portal_texture_s *next;
	struct portal_texture_s *prev;
	GLuint gl_texturenum1;
	GLuint gl_texturenum2;
}portal_texture_t;

void __fastcall ClientPortalManager_ResetAll(void * pthis, int);
mtexinfo_t * __fastcall ClientPortalManager_GetOriginalSurfaceTexture(void * pthis, int dummy, msurface_t *surf);
void __fastcall ClientPortalManager_DrawPortalSurface(void * pthis, int dummy, void * ClientPortal, msurface_t *surf, GLuint texture);
void __fastcall ClientPortalManager_RenderPortals(void* pthis, int dummy);

void __fastcall ClientPortalManager_EnableClipPlane(void* pthis, int dummy, int index, vec3_t viewangles, vec3_t view, vec4_t plane);
void ClientPortalManager_AngleVectors(const float* a1, float* a2, float* a3, float* a4);
int ClientPortal_GetIndex(void* pClientPortal);
bool ClientPortal_GetPortalTransform(void* pClientPortal, float* outOrigin, float* outAngles);
int ClientPortal_GetPortalMode(void* pClientPortal);

void R_LoadPortalProgramStates(void);
void R_SavePortalProgramStates(void);
void R_FreePortalResouces(void);
void R_ShutdownPortal(void);
void R_InitPortal(void);

int ClientPortal_GetTextureId(void* pClientPortal);
int ClientPortal_GetTextureWidth(void* pClientPortal);
int ClientPortal_GetTextureHeight(void* pClientPortal);

extern void* g_pCurrentClientPortal;
extern void* g_pClientPortalManager;

class CPortalTextureCache
{
public:
	int color{};
	int depth_stencil{};
	int width{};
	int height{};
};

class CPortalTextureCacheHash
{
public:
	CPortalTextureCacheHash(void* pClientPortal, int w, int h) : m_width(w), m_height(h)
	{
		m_index = ClientPortal_GetIndex(pClientPortal);
		m_mode = ClientPortal_GetPortalMode(pClientPortal);
	}
	int m_index{};
	int m_mode{};
	int m_width{};
	int m_height{};

	bool operator==(const CPortalTextureCacheHash& p) const
	{
		return m_index == p.m_index && m_mode == p.m_mode && m_width == p.m_width && m_height == p.m_height;
	}
};

class CPortalTextureCacheHasher
{
public:
	size_t operator()(const CPortalTextureCacheHash& p) const {
		size_t seed = 0;
		hash_combine(seed, p.m_index);
		hash_combine(seed, p.m_mode);
		hash_combine(seed, p.m_width);
		hash_combine(seed, p.m_height);
		return seed;
	}
};

std::shared_ptr<CPortalTextureCache> R_GetTextureCacheForPortalTexture(void* pClientPortal, int width, int height);

#endif //GL_PORTAL_H