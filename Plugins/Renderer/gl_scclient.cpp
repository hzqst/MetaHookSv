#include "gl_local.h"

bool g_bIsSCClientParticleDrawing = false;
SCClientPortalLayout g_SCClientPortalLayout;
bool g_bIsRenderingPortalViews = false;

namespace
{
// Keep the GL bindings and Renderer's bookkeeping in sync, including when the
// engine's PushView/PopView bypass GL_BindFrameBuffer.
class SCClientFramebufferScope
{
public:
	SCClientFramebufferScope()
		: scene(GL_GetCurrentSceneFBO()), rendering(GL_GetCurrentRenderingFBO())
	{
		GL_PushFrameBuffer();
		glGetIntegerv(GL_VIEWPORT, viewport);
	}
	~SCClientFramebufferScope()
	{
		Restore();
	}
	void Restore()
	{
		if (restored)
			return;
		GL_SetCurrentSceneFBO(scene);
		GL_BindFrameBuffer(rendering);
		GL_PopFrameBuffer();
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
		restored = true;
	}
private:
	FBO_Container_t* scene;
	FBO_Container_t* rendering;
	GLint viewport[4];
	bool restored = false;
};

class SCClientPortalTargetScope
{
public:
	explicit SCClientPortalTargetScope(void* portal)
		: texture(ClientPortal_GetTextureId(portal)),
		width(ClientPortal_GetTextureWidth(portal)), height(ClientPortal_GetTextureHeight(portal))
	{
		auto cache = R_GetTextureCacheForPortalTexture(portal, width, height);
		GL_BeginDebugGroup("P_RenderPortals");
		GL_BindFrameBufferWithTextures(&s_PortalFBO, texture, 0,
			cache->depth_stencil, cache->width, cache->height);
		GL_SetCurrentSceneFBO(&s_PortalFBO);
	}
	~SCClientPortalTargetScope()
	{
		framebuffer.Restore();
		GL_EndDebugGroup();
	}
	GLuint texture;
	GLsizei width, height;
private:
	SCClientFramebufferScope framebuffer;
};

struct SCClientPortalViewsScope;
SCClientPortalViewsScope* currentPortalViews = nullptr;

struct SCClientPortalViewsScope
{
	SCClientFramebufferScope framebuffer;
	SCClientPortalViewsScope* previous = currentPortalViews;
	bool wasRendering = g_bIsRenderingPortalViews;
	void* previousManager = g_pClientPortalManager;
	void* previousPortal = g_pCurrentClientPortal;
	std::unique_ptr<SCClientPortalTargetScope> target;

	explicit SCClientPortalViewsScope(void* manager)
	{
		currentPortalViews = this;
		g_bIsRenderingPortalViews = true;
		g_pClientPortalManager = manager;
		g_pCurrentClientPortal = nullptr;
	}
	~SCClientPortalViewsScope()
	{
		target.reset();
		g_pCurrentClientPortal = previousPortal;
		g_pClientPortalManager = previousManager;
		g_bIsRenderingPortalViews = wasRendering;
		currentPortalViews = previous;
	}
};
}

void __fastcall ClientPortalManager_RenderPortals(void* pthis, int, ref_params_t* params)
{
	// PushView/PopView surround the entire list, while clear/copy surround each
	// portal. Save the outer state before PushView can change the real binding.
	SCClientPortalViewsScope scope(pthis);
	// Both Windows clients use thiscall with one stack argument (ret 4).
	gPrivateFuncs.ClientPortalManager_RenderPortals(pthis, 0, params);
}

void __stdcall SCClient_glClear(GLbitfield mask)
{
	if (g_bIsRenderingPortalViews && currentPortalViews && g_pCurrentClientPortal
		&& mask == GL_COLOR_BUFFER_BIT && !currentPortalViews->target)
	{
		currentPortalViews->target = std::make_unique<SCClientPortalTargetScope>(g_pCurrentClientPortal);
	}
	glClear(mask);
}

void __stdcall SCClient_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
	GLint x, GLint y, GLsizei width, GLsizei height)
{
	if (g_bIsRenderingPortalViews && currentPortalViews && currentPortalViews->target
		&& target == GL_TEXTURE_2D && level == 0 && xoffset == 0 && yoffset == 0
		&& width == currentPortalViews->target->width && height == currentPortalViews->target->height)
	{
		GLint texture = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
		if (static_cast<GLuint>(texture) == currentPortalViews->target->texture)
		{
			// The legacy source y is screenHeight - textureHeight. RenderView
			// already wrote the entire texture through the portal FBO, so no
			// screen-space source rectangle needs to be copied.
			currentPortalViews->target.reset();
			g_pCurrentClientPortal = nullptr;
			return;
		}
	}
	glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

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
