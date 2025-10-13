#pragma once

#ifndef META_RENDERER_INTERFACE_H
#define META_RENDERER_INTERFACE_H

#include <interface.h>
#include <stdint.h>

//You should #include "gl_common.h" to get those type definitions
typedef struct texturedrectvertex_s texturedrectvertex_t;
typedef struct filledrectvertex_s filledrectvertex_t;
class CCompileShaderArgs;

//You should #include "gl_ringbuffer.h" to use IPMBRingBuffer
class IPMBRingBuffer;

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
    void Bind(int gltexturenum);

    /*
        Purpose: bind gltexturenum at OpenGLTextureUnit, and revert back to texture unit 0.
    */
    virtual void BindTextureUnit(int OpenGLTextureUnit, int OpenGLTextureTarget, int gltexturenum) = 0;
};

IMetaRenderer* MetaRenderer();

#define METARENDERER_INTERFACE_VERSION "MetaRenderer_API_001"

#endif //META_RENDERER_INTERFACE_H