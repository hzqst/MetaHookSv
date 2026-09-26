// Exercise the client hooks with real object memory and no GL context.
#include "../gl_local.h"
#include "../gl_scclient.cpp"
#include <cassert>
#include <cstdio>
#include <stdexcept>

private_funcs_t gPrivateFuncs{};
void* g_pClientPortalManager{};

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
    puts("SC client compatibility tests passed");
}
