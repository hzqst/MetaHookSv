#version 460

#define OIT_ALPHA_BLEND_ENABLED

#include "common.h"

in vec2 texCoord;

void main() {

    uint x = uint(gl_FragCoord.x);
    uint y = uint(gl_FragCoord.y);
    uint viewportW = SceneUBO.viewport.x;

    numFragmentsBuffer[addrGen(uvec2(x,y), viewportW)] = 0;
}