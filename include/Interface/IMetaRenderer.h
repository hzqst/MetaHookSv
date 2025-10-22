#pragma once

#ifndef META_RENDERER_INTERFACE_H
#define META_RENDERER_INTERFACE_H

#include <interface.h>
#include <stdint.h>

//You should #include "gl_common.h" to get those type definitions
typedef struct texturedrectvertex_s texturedrectvertex_t;
typedef struct filledrectvertex_s filledrectvertex_t;

class CCompileShaderArgs;

typedef struct FBO_Container_s FBO_Container_t;

//You should #include "gl_ringbuffer.h" to use IPMBRingBuffer
class IPMBRingBuffer;

class IMetaRendererCallbacks : public IBaseInterface
{
public:
    /*
        Called from GL_BeginRendering
    */
    virtual void OnBeginRendering(int* x, int* y, int* width, int* height) = 0;

    /*
        Called from GL_BeginRendering -> R_RenderFrameStart
    */
    virtual void OnRenderFrameStart() = 0;

    /*
        Called from GL_BeginRendering
    */
    virtual void OnBeginRenderingPost(int* x, int* y, int* width, int* height) = 0;

    /*
        Called from GL_FreeFrameBuffers
    */
    virtual void OnFreeFrameBuffers() = 0;

    /*
        Called from GL_GenerateFrameBuffers
    */
    virtual void OnGenerateFrameBuffers() = 0;

    /*
        Called from R_PreRenderView
    */
    virtual void OnPreRenderView() = 0;

    /*
        Called from R_PreRenderView, after switching to target FBO
    */
    virtual void OnPreRenderViewPost() = 0;

    /*
        Called from R_PostRenderView
    */
    virtual void OnPostRenderView() = 0;

    /*
        Called from R_PostRenderView, after executing all built-in post-processes
    */
    virtual void OnPostRenderViewPost() = 0;

    /*
        Called from R_RenderEndFrame
    */
    virtual void OnRenderEndFrame() = 0;
};

class IMetaRenderer : public IBaseInterface
{
public:

    /*
        Purpose: draw textured triangle list with OpenGL textureid: "gltexturenum", indicesCount must be multiple of 3, available programState can be found in "gl_common.h"
    */
    virtual void DrawTexturedRect(int gltexturenum, const texturedrectvertex_t* verticeBuffer, size_t verticeCount, const uint32_t* indices, size_t indicesCount, uint64_t programState, const char* debugMetadata) = 0;

    /*
        Purpose: draw colored triangle list with OpenGL textureid: "gltexturenum", indicesCount must be multiple of 3, available programState can be found in "gl_common.h"
    */
    virtual void DrawFilledRect(const filledrectvertex_t* verticeBuffer, size_t verticeCount, const uint32_t* indices, size_t indicesCount, uint64_t programState, const char* debugMetadata) = 0;

    /*
        Purpose: Simple wrapper around DrawTexturedRect, for drawing rectangle only.
    */
    virtual void DrawTexturedQuad(int gltexturenum, int x0, int y0, int x1, int y1, const float* color4v, uint64_t programState, const char* debugMetadata) = 0;

    /*
        Purpose: Simple wrapper around DrawFilledRect, for drawing rectangle only.
    */
    virtual void DrawFilledQuad(int x0, int y0, int x1, int y1, const float* color4v, uint64_t programState, const char* debugMetadata) = 0;

    /*
        Purpose: Load and compile vertex and fragment shader from FileSystem, and link them into a OpenGL program.
    */
    virtual uint32_t CompileShaderFile(const char* vsfile, const char* fsfile, const char* vsdefine = nullptr, const char* fsdefine = nullptr) = 0;
    
    /*
        Purpose: Load and compile vertex, fragment, and geometry shader (if specified) from FileSystem, and link them into a OpenGL program.
    */
    virtual uint32_t CompileShaderFileEx(const CCompileShaderArgs* args) = 0;

    /*
         Purpose: Create PMB RingBuffer
    */
    virtual IPMBRingBuffer* CreatePMBRingBuffer(const char* name, size_t bufferSize, int OpenGLBufferTarget) = 0;

    /*
         Purpose: Setup worldmatrix & projmatrix for rendering in 2D, the viewport is the whole screen
    */
    virtual void Set2D() = 0;

    /*
         Purpose: Setup worldmatrix & projmatrix for rendering in 2D, the viewport is specified in the parameters.
    */
    virtual void Set2DEx(int x, int y, int width, int height) = 0;

    /*
         Purpose: Revert worldmatrix & projmatrix back to 2D
    */
    virtual void Finish2D() = 0;

    /*
         Purpose: bind gltexturenum at texture unit 0.
     */
    virtual void Bind(int gltexturenum) = 0;

    /*
        Purpose: bind gltexturenum at OpenGLTextureUnit, and revert back to texture unit 0.
    */
    virtual void BindTextureUnit(int OpenGLTextureUnit, int OpenGLTextureTarget, int gltexturenum) = 0;

    /*
        Purpose: Bind OpenGL program
    */
    virtual void UseProgram(int glprogram) = 0;

    /*
        Purpose: Set the current scene framebuffer object
    */
    virtual void SetCurrentSceneFBO(FBO_Container_t* src) = 0;

    /*
        Purpose: Get the current scene framebuffer object
    */
    virtual FBO_Container_t* GetCurrentSceneFBO() = 0;

    /*
        Purpose: Get the current rendering framebuffer object
    */
    virtual FBO_Container_t* GetCurrentRenderingFBO() = 0;

    /*
        Purpose: Bind a framebuffer object for rendering
    */
    virtual void BindFrameBuffer(FBO_Container_t* fbo) = 0;

    /*
        Purpose: Bind a framebuffer object with specified color, depth, and depth-stencil textures
    */
    virtual void BindFrameBufferWithTextures(FBO_Container_t* fbo, unsigned int color, unsigned int depth, unsigned int depth_stencil, int width, int height) = 0;

    /*
        Purpose: Push current framebuffer binding state onto the stack
    */
    virtual void PushFrameBuffer() = 0;

    /*
        Purpose: Pop framebuffer binding state from the stack
    */
    virtual void PopFrameBuffer() = 0;

    /*
        Purpose: Get the rendering definition structure pointer
    */
    virtual void* GetRefDef() = 0;

    /*
        Purpose: Push current view origin and view angles onto the stack
    */
    virtual void PushRefDef() = 0;

    /*
        Purpose: Update rendering origin and view vectors according to refdef
    */
    virtual void UpdateRefDef() = 0;

    /*
        Purpose: Pop view origin and view angles from the stack
    */
    virtual void PopRefDef() = 0;

    /*
        Purpose: Generate a texture with specified color format
    */
    virtual unsigned int GenTextureColorFormat(int w, int h, int iInternalFormat, bool filter, float* borderColor, bool immutable) = 0;

    /*
        Purpose: Generate a texture array with specified color format
    */
    virtual unsigned int GenTextureArrayColorFormat(int w, int h, int depth, int iInternalFormat, bool filter, float* borderColor, bool immutable) = 0;

    /*
        Purpose: Generate a RGBA8 format texture
    */
    virtual unsigned int GenTextureRGBA8(int w, int h, bool immutable) = 0;

    /*
        Purpose: Generate a depth texture
    */
    virtual unsigned int GenDepthTexture(int w, int h, bool immutable) = 0;

    /*
        Purpose: Generate a depth-stencil texture
    */
    virtual unsigned int GenDepthStencilTexture(int w, int h, bool immutable) = 0;

    /*
        Purpose: Create a depth view for an existing depth texture
    */
    virtual unsigned int CreateDepthViewForDepthTexture(int texId) = 0;

    /*
        Purpose: Create a stencil view for an existing depth texture
    */
    virtual unsigned int CreateStencilViewForDepthTexture(int texId) = 0;

    /*
        Purpose: Generate a framebuffer object with the specified name
    */
    virtual void GenFrameBuffer(FBO_Container_t* s, const char* szFrameBufferName) = 0;

    /*
        Purpose: Get the name of a framebuffer object
    */
    virtual const char* GetFrameBufferName(FBO_Container_t* s) = 0;

    /*
        Purpose: Attach a color texture to the framebuffer object
    */
    virtual void FrameBufferColorTexture(FBO_Container_t* s, unsigned int iInternalFormat) = 0;

    /*
        Purpose: Attach a depth texture to the framebuffer object
    */
    virtual void FrameBufferDepthTexture(FBO_Container_t* s, unsigned int iInternalFormat) = 0;

    /*
        Purpose: Clear color buffer with specified color
    */
    virtual void ClearColor(float* color) = 0;

    /*
        Purpose: Clear depth buffer with specified depth value
    */
    virtual void ClearDepth(float depth) = 0;

    /*
        Purpose: Clear depth and stencil buffers
    */
    virtual void ClearDepthStencil(float depth, int stencilref, int stencilmask) = 0;

    /*
        Purpose: Clear color, depth, and stencil buffers
    */
    virtual void ClearColorDepthStencil(float* color, float depth, int stencilref, int stencilmask) = 0;

    /*
        Purpose: Clear stencil buffer
    */
    virtual void ClearStencil(int mask) = 0;

    /*
        Purpose: Begin stencil test with equal comparison
    */
    virtual void BeginStencilCompareEqual(int ref, int mask) = 0;

    /*
        Purpose: Begin stencil test with not-equal comparison
    */
    virtual void BeginStencilCompareNotEqual(int ref, int mask) = 0;

    /*
        Purpose: Begin stencil write operation
    */
    virtual void BeginStencilWrite(int ref, int write_mask) = 0;

    /*
        Purpose: End stencil operation
    */
    virtual void EndStencil() = 0;

    /*
        Purpose: Clear a framebuffer object
    */
    virtual void ClearFBO(FBO_Container_t* s) = 0;

    /*
        Purpose: Free a framebuffer object and its associated resources
    */
    virtual void FreeFBO(FBO_Container_t* s) = 0;

    /*
        Purpose: Get the world matrix pointer
    */
    virtual float* GetWorldMatrix() = 0;

    /*
        Purpose: Push current world matrix onto the stack
    */
    virtual void PushWorldMatrix() = 0;

    /*
        Purpose: Pop world matrix from the stack
    */
    virtual void PopWorldMatrix() = 0;

    /*
        Purpose: Load identity matrix as the world matrix
    */
    virtual void LoadIdentityForWorldMatrix() = 0;

    /*
        Purpose: Rotate the world matrix by specified angle and axis
    */
    virtual void RotateWorldMatrix(float angle, float x, float y, float z) = 0;

    /*
        Purpose: Translate the world matrix by specified vector
    */
    virtual void TranslateWorldMatrix(float x, float y, float z) = 0;

    /*
        Purpose: Setup world matrix based on player view origin and view angles
    */
    virtual void SetupPlayerViewWorldMatrix(const float* origin, const float* viewangles) = 0;

    /*
        Purpose: Push current projection matrix onto the stack
    */
    virtual void PushProjectionMatrix() = 0;

    /*
        Purpose: Pop projection matrix from the stack
    */
    virtual void PopProjectionMatrix() = 0;

    /*
        Purpose: Get the projection matrix pointer
    */
    virtual float* GetProjectionMatrix() = 0;

    /*
        Purpose: Load identity matrix as the projection matrix
    */
    virtual void LoadIdentityForProjectionMatrix() = 0;

    /*
        Purpose: Setup frustum projection matrix (perspective projection)
    */
    virtual void SetupFrustumProjectionMatrix(float left, float right, float bottom, float top, float zNear, float zFar) = 0;

    /*
        Purpose: Setup orthographic projection matrix
    */
    virtual void SetupOrthoProjectionMatrix(float left, float right, float bottom, float top, float zNear, float zFar, bool NegativeOneToOneZ) = 0;

    /*
        Purpose: Begin a debug group for OpenGL debugging tools
    */
    virtual void BeginDebugGroup(const char* name) = 0;

    /*
        Purpose: Begin a debug group for OpenGL debugging tools
    */
    virtual void BeginDebugGroupFormat(const char* fmt, ...) = 0;

    /*
        Purpose: End the current debug group
    */
    virtual void EndDebugGroup() = 0;

    /*
        Purpose: Set a debug name for a texture object
    */
    virtual void SetTextureDebugName(unsigned int textureId, const char* name) = 0;

    /*
        Purpose: Set a debug name for a framebuffer object
    */
    virtual void SetFrameBufferDebugName(unsigned int framebufferId, const char* name) = 0;

    /*
        Purpose: Generate a new OpenGL texture object
    */
    virtual unsigned int GenTexture() = 0;

    /*
        Purpose: Generate a new OpenGL buffer object
    */
    virtual unsigned int GenBuffer() = 0;

    /*
        Purpose: Generate a new OpenGL Vertex Array Object (VAO)
    */
    virtual unsigned int GenVAO() = 0;

    /*
        Purpose: Delete an OpenGL buffer object
    */
    virtual void DeleteBuffer(unsigned int buf) = 0;

    /*
        Purpose: Delete an OpenGL Vertex Array Object (VAO)
    */
    virtual void DeleteVAO(unsigned int VAO) = 0;

    /*
        Purpose: Delete an OpenGL texture object
    */
    virtual void DeleteTexture(unsigned int texid) = 0;

    /*
        Purpose: Bind a Vertex Array Object (VAO)
    */
    virtual void BindVAO(unsigned int VAO) = 0;

    /*
        Purpose: Bind an Indirect Draw Attribute Buffer Object (ABO)
    */
    virtual void BindABO(unsigned int ABO) = 0;

    /*
        Purpose: Upload data to Uniform Buffer Object (UBO) with dynamic draw usage
    */
    virtual void UploadDataToUBODynamicDraw(unsigned int UBO, size_t size, const void* data) = 0;

    /*
        Purpose: Upload partial data to Uniform Buffer Object (UBO)
    */
    virtual void UploadSubDataToUBO(unsigned int UBO, size_t offset, size_t size, const void* data) = 0;

    /*
        Purpose: Upload data to Vertex Buffer Object (VBO) with static draw usage
    */
    virtual void UploadDataToVBOStaticDraw(unsigned int VBO, size_t size, const void* data) = 0;

    /*
        Purpose: Upload data to Vertex Buffer Object (VBO) with dynamic draw usage
    */
    virtual void UploadDataToVBODynamicDraw(unsigned int VBO, size_t size, const void* data) = 0;

    /*
        Purpose: Upload data to Vertex Buffer Object (VBO) with stream draw usage
    */
    virtual void UploadDataToVBOStreamDraw(unsigned int VBO, size_t size, const void* data) = 0;

    /*
        Purpose: Upload data to Vertex Buffer Object (VBO) using mapped memory
    */
    virtual void UploadDataToVBOStreamMap(unsigned int VBO, size_t size, const void* data) = 0;

    /*
        Purpose: Upload partial data to Vertex Buffer Object (VBO)
    */
    virtual void UploadSubDataToVBO(unsigned int VBO, size_t offset, size_t size, const void* data) = 0;

    /*
        Purpose: Upload data to Element Buffer Object (EBO/Index Buffer) with static draw usage
    */
    virtual void UploadDataToEBOStaticDraw(unsigned int EBO, size_t size, const void* data) = 0;

    /*
        Purpose: Upload data to Element Buffer Object (EBO/Index Buffer) with dynamic draw usage
    */
    virtual void UploadDataToEBODynamicDraw(unsigned int EBO, size_t size, const void* data) = 0;

    /*
        Purpose: Upload data to Element Buffer Object (EBO/Index Buffer) with stream draw usage
    */
    virtual void UploadDataToEBOStreamDraw(unsigned int EBO, size_t size, const void* data) = 0;

    /*
        Purpose: Upload data to Element Buffer Object (EBO/Index Buffer) using mapped memory
    */
    virtual void UploadDataToEBOStreamMap(unsigned int EBO, size_t size, const void* data) = 0;

    /*
        Purpose: Upload partial data to Element Buffer Object (EBO/Index Buffer)
    */
    virtual void UploadSubDataToEBO(unsigned int EBO, size_t offset, size_t size, const void* data) = 0;

    /*
        Purpose: Upload data to Indirect Draw Attribute Buffer Object (ABO) with static draw usage
    */
    virtual void UploadDataToABOStaticDraw(unsigned int ABO, size_t size, const void* data) = 0;

    /*
        Purpose: Upload data to Indirect Draw Attribute Buffer Object (ABO) with dynamic draw usage
    */
    virtual void UploadDataToABODynamicDraw(unsigned int ABO, size_t size, const void* data) = 0;

    /*
        Purpose: Upload data to Shader Storage Buffer Object (SSBO) with static draw usage
    */
    virtual void UploadDataToSSBOStaticDraw(unsigned int SSBO, size_t size, const void* data) = 0;

    /*
        Purpose: Get the empty VAO for rendering without vertex attributes
    */
    virtual unsigned int GetEmptyVAO() = 0;

    /*
        Purpose: Copy color buffer from source FBO to destination FBO
    */
    virtual void CopyColor(FBO_Container_t* src, FBO_Container_t* dst) = 0;

    /*
        Purpose: Apply Gaussian blur pass to framebuffer
    */
    virtual void BlurPass(FBO_Container_t* src, FBO_Container_t* dst, float scale, bool vertical) = 0;

    /*
        Purpose: Apply Fast Approximate Anti-Aliasing (FXAA) post-processing
    */
    virtual void FXAA(FBO_Container_t* src, FBO_Container_t* dst) = 0;

    /*
        Purpose: Downsample framebuffer with optional 2x2 filter and stencil filter
    */
    virtual void DownSample(FBO_Container_t* src_color, FBO_Container_t* src_stencil, FBO_Container_t* dst, bool bUseFilter2x2, bool bUseStencilFilter) = 0;

    /*
        Purpose: Blit depth and stencil buffers from source FBO to destination FBO
    */
    virtual void BlitFrameBufferToFrameBufferDepthStencil(FBO_Container_t* src, FBO_Container_t* dst) = 0;

    /*
        Purpose: Blit stencil buffer only from source FBO to destination FBO
    */
    virtual void BlitFrameBufferToFrameBufferStencilOnly(FBO_Container_t* src, FBO_Container_t* dst) = 0;

    /*
        Purpose: Blit depth buffer only from source FBO to destination FBO
    */
    virtual void BlitFrameBufferToFrameBufferDepthOnly(FBO_Container_t* src, FBO_Container_t* dst) = 0;

    /*
        Purpose: Blit color, depth and stencil buffers from source FBO to destination FBO
    */
    virtual void BlitFrameBufferToFrameBufferColorDepthStencil(FBO_Container_t* src, FBO_Container_t* dst) = 0;

    /*
        Purpose: Blit color and depth buffers from source FBO to destination FBO
    */
    virtual void BlitFrameBufferToFrameBufferColorDepth(FBO_Container_t* src, FBO_Container_t* dst) = 0;

    /*
        Purpose: Blit color buffer only from source FBO to destination FBO
    */
    virtual void BlitFrameBufferToFrameBufferColorOnly(FBO_Container_t* src, FBO_Container_t* dst) = 0;

    /*
        Purpose: Blit framebuffer to screen (default framebuffer)
    */
    virtual void BlitFrameBufferToScreen(FBO_Container_t* src) = 0;

    /*
        Purpose: Register render callbacks
    */
    virtual void RegisterRenderCallbacks(IMetaRendererCallbacks* RenderCallbacks) = 0;
    /*
        Purpose: Unregister render callbacks
    */
    virtual void UnregisterRenderCallbacks(IMetaRendererCallbacks* RenderCallbacks) = 0;
};

IMetaRenderer* MetaRenderer();

#define METARENDERER_INTERFACE_VERSION "MetaRenderer_API_002"

#endif //META_RENDERER_INTERFACE_H