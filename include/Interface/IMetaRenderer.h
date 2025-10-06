#pragma once

#ifndef META_RENDERER_INTERFACE_H
#define META_RENDERER_INTERFACE_H

#include <interface.h>
#include <stdint.h>

typedef struct texturedrectvertex_s texturedrectvertex_t;
typedef struct filledrectvertex_s filledrectvertex_t;

class IMetaRenderer : public IBaseInterface
{
public:

    virtual void DrawTexturedRect(int gltexturenum, const texturedrectvertex_t* verticeBuffer, size_t verticeCount, const uint32_t* indices, size_t indicesCount, uint64_t programState, const char* debugMetadata);

    virtual void DrawFilledRect(const filledrectvertex_t* verticeBuffer, size_t verticeCount, const uint32_t* indices, size_t indicesCount, uint64_t programState, const char* debugMetadata);

};

IMetaRenderer* MetaRenderer();

#define METARENDERER_INTERFACE_VERSION "MetaRenderer_API_001"

#endif //META_RENDERER_INTERFACE_H