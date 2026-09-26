// Exercise the production allocator and Renderer frame lifecycle without a GPU.
#include "../gl_local.h"
static void TestFinish() { assert(false); }
static GLenum TestGetError() { assert(false); return GL_NO_ERROR; }
#define glFinish TestFinish
#define glGetError TestGetError
#include "../gl_ringbuffer.cpp"
#undef glFinish
#undef glGetError
#include <cassert>
#include <cstdio>
#include <cstdarg>

cl_enginefunc_t gEngfuncs{};
IPMBRingBuffer* g_TriAPIVertexBuffer{};
IPMBRingBuffer* g_TriAPIIndexBuffer{};
IPMBRingBuffer* g_TexturedRectVertexBuffer{};
IPMBRingBuffer* g_FilledRectVertexBuffer{};
IPMBRingBuffer* g_RectInstanceBuffer{};
IPMBRingBuffer* g_RectIndexBuffer{};

static unsigned char mappedMemory[1024];
static unsigned warnings{}, fences{}, deletedFences{};
static GLenum waitResult = GL_ALREADY_SIGNALED;
static void ConsolePrint(const char*, ...) { ++warnings; }
GLuint GL_GenBuffer() { return 1; }
void GL_DeleteBuffer(GLuint) {}
void GL_BindVAO(GLuint) {}
static void GLAPIENTRY BindBuffer(GLenum, GLuint) {}
static void GLAPIENTRY BufferStorage(GLenum, GLsizeiptr, const void*, GLbitfield) {}
static void* GLAPIENTRY MapBufferRange(GLenum, GLintptr, GLsizeiptr, GLbitfield) { return mappedMemory; }
static GLboolean GLAPIENTRY UnmapBuffer(GLenum) { return GL_TRUE; }
static GLsync GLAPIENTRY FenceSync(GLenum, GLbitfield) { ++fences; return reinterpret_cast<GLsync>(1); }
static void GLAPIENTRY DeleteSync(GLsync) { ++deletedFences; }
static GLenum GLAPIENTRY ClientWaitSync(GLsync, GLbitfield, GLuint64) { return waitResult; }

PFNGLBINDBUFFERPROC __glewBindBuffer = BindBuffer;
PFNGLBUFFERSTORAGEPROC __glewBufferStorage = BufferStorage;
PFNGLMAPBUFFERRANGEPROC __glewMapBufferRange = MapBufferRange;
PFNGLUNMAPBUFFERPROC __glewUnmapBuffer = UnmapBuffer;
PFNGLOBJECTLABELPROC __glewObjectLabel = nullptr;
PFNGLFENCESYNCPROC __glewFenceSync = FenceSync;
PFNGLDELETESYNCPROC __glewDeleteSync = DeleteSync;
PFNGLCLIENTWAITSYNCPROC __glewClientWaitSync = ClientWaitSync;

int main()
{
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    gEngfuncs.Con_Printf = ConsolePrint;
    CPMBRingBufferAllocation allocation;

    // First draw creates the buffer after frame start (issue #880).
    R_BeginRingBufferFrame();
    g_FilledRectVertexBuffer = R_CreatePMBRingBuffer("FilledRectVertexBuffer", sizeof(mappedMemory), GL_ARRAY_BUFFER);
    assert(g_FilledRectVertexBuffer->Allocate(96, allocation));
    assert(0 == allocation.offset && 96 == allocation.size && allocation.valid);
    assert(0 == warnings);
    assert(g_FilledRectVertexBuffer->Allocate(96, allocation));
    assert(96 == allocation.offset && 192 == g_FilledRectVertexBuffer->GetUsedSize());
    R_EndRingBufferFrame();
    assert(1 == fences);
    assert(!g_FilledRectVertexBuffer->Allocate(96, allocation));
    assert(!allocation.valid && nullptr == allocation.ptr);
    assert(1 == warnings);

    // Keep the first frame pending: the next frame must not reuse its bytes.
    waitResult = GL_TIMEOUT_EXPIRED;
    R_BeginRingBufferFrame();
    assert(g_FilledRectVertexBuffer->Allocate(96, allocation));
    assert(192 == allocation.offset);
    R_EndRingBufferFrame();
    assert(2 == fences);
    waitResult = GL_ALREADY_SIGNALED;
    R_BeginRingBufferFrame();
    assert(0 == g_FilledRectVertexBuffer->GetUsedSize());
    assert(2 == deletedFences);
    assert(g_FilledRectVertexBuffer->Allocate(96, allocation));
    assert(0 == allocation.offset);
    R_EndRingBufferFrame();
    g_FilledRectVertexBuffer->Destroy();
    g_FilledRectVertexBuffer = nullptr;

    // Creation outside a frame must not silently authorize allocations.
    g_RectInstanceBuffer = R_CreatePMBRingBuffer("RectInstanceBuffer", sizeof(mappedMemory), GL_ARRAY_BUFFER);
    assert(!g_RectInstanceBuffer->Allocate(64, allocation));
    R_BeginRingBufferFrame();
    assert(g_RectInstanceBuffer->Allocate(64, allocation));
    // The public factory remains manually managed even during a Renderer frame.
    auto manual = GL_CreatePMBRingBuffer("Manual", sizeof(mappedMemory), GL_ARRAY_BUFFER);
    assert(!manual->Allocate(96, allocation));
    manual->BeginFrame();
    assert(manual->Allocate(96, allocation));
    manual->EndFrame();
    manual->Destroy();
    R_EndRingBufferFrame();
    g_RectInstanceBuffer->Destroy();
    g_RectInstanceBuffer = nullptr;

    // Every Renderer-owned slot must participate, including buffers first used
    // in a later frame. Creating another buffer must not reset existing usage.
    IPMBRingBuffer** slots[] = {
        &g_TriAPIVertexBuffer, &g_TriAPIIndexBuffer,
        &g_TexturedRectVertexBuffer, &g_FilledRectVertexBuffer,
        &g_RectInstanceBuffer, &g_RectIndexBuffer
    };
    R_BeginRingBufferFrame();
    R_EndRingBufferFrame();
    R_BeginRingBufferFrame();
    for (auto slot : slots)
    {
        *slot = R_CreatePMBRingBuffer("LateFirstUse", sizeof(mappedMemory), GL_ARRAY_BUFFER);
        assert((*slot)->Allocate(96, allocation));
    }
    assert(g_TriAPIVertexBuffer->Allocate(96, allocation));
    assert(96 == allocation.offset);
    R_EndRingBufferFrame();
    for (auto slot : slots)
        assert(!(*slot)->Allocate(96, allocation));
    R_BeginRingBufferFrame();
    for (auto slot : slots)
    {
        assert(0 == (*slot)->GetUsedSize());
        assert((*slot)->Allocate(96, allocation));
    }
    R_EndRingBufferFrame();
    for (auto slot : slots)
    {
        (*slot)->Destroy();
        *slot = nullptr;
    }
    assert(fences == deletedFences);
    std::puts("ringbuffer_tests: passed");
}
