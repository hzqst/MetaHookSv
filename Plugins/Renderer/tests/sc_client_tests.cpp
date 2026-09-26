// Exercise the client hooks with real object memory and no GL context.
#include "../gl_local.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>

static void TestClear(GLbitfield mask);
static void TestCopy(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
static void TestGetInteger(GLenum name, GLint* values);
static void TestViewport(GLint x, GLint y, GLsizei width, GLsizei height);
#define glClear TestClear
#define glCopyTexSubImage2D TestCopy
#define glGetIntegerv TestGetInteger
#define glViewport TestViewport
#include "../gl_scclient.cpp"
#undef glClear
#undef glCopyTexSubImage2D
#undef glGetIntegerv
#undef glViewport

private_funcs_t gPrivateFuncs{};
void* g_pClientPortalManager{};
void* g_pCurrentClientPortal{};

FBO_Container_t s_PortalFBO{};
static FBO_Container_t parentFBO{};
static FBO_Container_t* sceneFBO = &parentFBO;
static FBO_Container_t* renderingFBO = &parentFBO;
static GLint readFBO = 17, drawFBO = 19, boundTexture{};
static GLint viewport[4] = { 3, 5, 800, 600 };
static std::vector<std::pair<GLint, GLint>> framebufferStack;
static int groups{}, portalGroups{}, clears{}, copies{};
static GLbitfield lastClear{};
static int renderMode{};
static void* sources[2]{};

FBO_Container_t* GL_GetCurrentSceneFBO() { return sceneFBO; }
FBO_Container_t* GL_GetCurrentRenderingFBO() { return renderingFBO; }
void GL_SetCurrentSceneFBO(FBO_Container_t* fbo) { sceneFBO = fbo; }
void GL_BindFrameBuffer(FBO_Container_t* fbo)
{
    renderingFBO = fbo;
    readFBO = drawFBO = fbo ? fbo->s_hBackBufferFBO : 0;
}
void GL_PushFrameBuffer() { framebufferStack.emplace_back(readFBO, drawFBO); }
void GL_PopFrameBuffer()
{
    assert(!framebufferStack.empty());
    readFBO = framebufferStack.back().first;
    drawFBO = framebufferStack.back().second;
    framebufferStack.pop_back();
}
void GL_BindFrameBufferWithTextures(FBO_Container_t* fbo, GLuint color, GLuint, GLuint, GLsizei width, GLsizei height)
{
    assert(ClientPortal_GetTextureId(g_pCurrentClientPortal) == color);
    GL_BindFrameBuffer(fbo);
    fbo->iWidth = width;
    fbo->iHeight = height;
}
void GL_BeginDebugGroup(const char* name)
{
    assert(0 == strcmp("P_RenderPortals", name));
    ++groups;
    ++portalGroups;
}
void GL_EndDebugGroup() { assert(groups > 0); --groups; }
std::shared_ptr<CPortalTextureCache> R_GetTextureCacheForPortalTexture(void*, int width, int height)
{
    auto cache = std::make_shared<CPortalTextureCache>();
    cache->width = width;
    cache->height = height;
    cache->depth_stencil = 101;
    return cache;
}
static void TestClear(GLbitfield mask) { ++clears; lastClear = mask; }
static void TestCopy(GLenum target, GLint level, GLint xo, GLint yo, GLint x, GLint y, GLsizei width, GLsizei height)
{
    assert(GL_TEXTURE_2D == target && 0 == level && 0 == xo && 0 == yo && 0 == x && 344 == y);
    assert(512 == width && 256 == height);
    ++copies;
}
static void TestGetInteger(GLenum name, GLint* values)
{
    if (name == GL_VIEWPORT) memcpy(values, viewport, sizeof(viewport));
    else { assert(GL_TEXTURE_BINDING_2D == name); *values = boundTexture; }
}
static void TestViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    viewport[0] = x; viewport[1] = y; viewport[2] = width; viewport[3] = height;
}
static void CopyPortal() { SCClient_glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 344, 512, 256); }
static void __fastcall RenderPortals(void* self, int)
{
    assert(g_bIsRenderingPortalViews && g_pClientPortalManager == self);
    if (renderMode == 0)
    {
        CopyPortal(); // Being inside RenderPortals alone must not swallow copies.
        return; // Empty list never calls PushView.
    }
    // Simulate the engine PushView changing the real binding without updating Renderer.
    readFBO = drawFBO = 0;
    for (auto source : sources)
    {
        g_pCurrentClientPortal = source;
        SCClient_glClear(GL_COLOR_BUFFER_BIT);
        assert(&s_PortalFBO == sceneFBO && &s_PortalFBO == renderingFBO);
        assert(1 == groups && GL_COLOR_BUFFER_BIT == lastClear);
        TestViewport(0, 0, 512, 256);
        if (renderMode == 2) return; // Cleanup even without the copy boundary.
        if (renderMode == 3) throw std::runtime_error("portal failure");
        boundTexture = 999; // An unrelated copy must still reach GL.
        CopyPortal();
        assert(1 == groups);
        boundTexture = ClientPortal_GetTextureId(source);
        CopyPortal();
        assert(0 == groups && nullptr == g_pCurrentClientPortal);
        assert(&parentFBO == sceneFBO && &parentFBO == renderingFBO);
    }
    // Simulate PopView changing the bindings and viewport after the last copy.
    readFBO = drawFBO = 27;
    TestViewport(0, 0, 1920, 1080);
}

static void TestPortalScopes(void* manager, void* source)
{
    alignas(int) unsigned char secondSource[216];
    memcpy(secondSource, source, sizeof(secondSource));
    *reinterpret_cast<int*>(secondSource + g_SCClientPortalLayout.textureId) = 38;
    sources[0] = source;
    sources[1] = secondSource;
    gPrivateFuncs.ClientPortalManager_RenderPortals = RenderPortals;
    s_PortalFBO.s_hBackBufferFBO = 23;
    g_pCurrentClientPortal = source; // The boolean gate also protects a stale pointer.
    boundTexture = ClientPortal_GetTextureId(source);
    SCClient_glClear(GL_DEPTH_BUFFER_BIT);
    CopyPortal();
    g_pCurrentClientPortal = nullptr;
    assert(1 == clears && 1 == copies && 0 == portalGroups);
    int outerManager{};
    g_pClientPortalManager = &outerManager;
    for (renderMode = 0; renderMode < 4; ++renderMode)
    {
        try { ClientPortalManager_RenderPortals(manager, 0); }
        catch (const std::runtime_error&) { assert(3 == renderMode); }
        assert(!g_bIsRenderingPortalViews && nullptr == g_pCurrentClientPortal);
        assert(&outerManager == g_pClientPortalManager);
        assert(&parentFBO == sceneFBO && &parentFBO == renderingFBO);
        assert(17 == readFBO && 19 == drawFBO);
        assert(3 == viewport[0] && 5 == viewport[1] && 800 == viewport[2] && 600 == viewport[3]);
        assert(0 == groups && framebufferStack.empty());
    }
    assert(4 == portalGroups && 5 == clears && 4 == copies);
}

static int begins{}, ends{}, primitive{}, nesting{};
static bool throwFromParticle{};
static int particle;
static int particleSystem;

void __stdcall triapi_glBegin(int mode) { ++begins; primitive = mode; }
void __stdcall triapi_glEnd() { ++ends; }

static void __fastcall DrawParticle(void* self, int, void* unit)
{
    assert(&particleSystem == self && &particle == unit);
    assert(g_bIsSCClientParticleDrawing);
    if (throwFromParticle)
        throw std::runtime_error("particle failure");
    if (!nesting++)
    {
        CParticleSystem_ParticleDraw(self, 0, unit);
        assert(g_bIsSCClientParticleDrawing);
    }
    --nesting;
    CoreProfile_glBegin(GL_TRIANGLE_FAN);
    CoreProfile_glNormal3f(0, 0, 1);
    CoreProfile_glEnd();
}

int main()
{
    CoreProfile_glBegin(GL_TRIANGLES);
    CoreProfile_glEnd();
    assert(0 == begins && 0 == ends);
    gPrivateFuncs.CParticleSystem_ParticleDraw = DrawParticle;
    CParticleSystem_ParticleDraw(&particleSystem, 0, &particle);
    assert(2 == begins && 2 == ends && GL_TRIANGLE_FAN == primitive);
    assert(!g_bIsSCClientParticleDrawing);
    throwFromParticle = true;
    try { CParticleSystem_ParticleDraw(&particleSystem, 0, &particle); }
    catch (const std::runtime_error&) {}
    assert(!g_bIsSCClientParticleDrawing);

    // A byte write must leave the adjacent program and manager state intact.
    unsigned char manager[512];
    memset(manager, 0xA5, sizeof(manager));
    gPrivateFuncs.offset_ClientPortalManager_m_bShadersAvailable = 480;
    ClientPortalManager_InitShader(manager, 0);
    for (size_t i = 0; i < sizeof(manager); ++i)
        assert((i == 480 ? 0 : 0xA5) == manager[i]);

    g_pClientPortalManager = manager;
    g_SCClientPortalLayout.vectorBegin = 140;
    g_SCClientPortalLayout.vectorEnd = 144;
    int first{}, second{}, missing{};
    void* portals[] = { &first, &second };
    *reinterpret_cast<void***>(manager + 140) = portals;
    *reinterpret_cast<void***>(manager + 144) = portals + 2;
    assert(0 == ClientPortal_GetIndex(&first));
    assert(1 == ClientPortal_GetIndex(&second));
    assert(-1 == ClientPortal_GetIndex(&missing));
    *reinterpret_cast<void***>(manager + 144) = portals;
    assert(-1 == ClientPortal_GetIndex(&first));

    // Both Windows layouts publish these offsets; older builds must read them too.
    alignas(int) unsigned char source[216]{};
    g_SCClientPortalLayout.textureId = 204;
    g_SCClientPortalLayout.textureWidth = 208;
    g_SCClientPortalLayout.textureHeight = 212;
    *reinterpret_cast<int*>(source + 204) = 37;
    *reinterpret_cast<int*>(source + 208) = 512;
    *reinterpret_cast<int*>(source + 212) = 256;
    assert(37 == ClientPortal_GetTextureId(source));
    assert(512 == ClientPortal_GetTextureWidth(source));
    assert(256 == ClientPortal_GetTextureHeight(source));
    TestPortalScopes(manager, source);
    puts("SC client compatibility tests passed");
}
