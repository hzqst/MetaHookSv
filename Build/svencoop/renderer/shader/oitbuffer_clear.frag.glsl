#version 430

#define OIT_BLEND_ENABLED

#include "common.h"

in vec2 texCoord;

void main() {

    uint x = uint(gl_FragCoord.x);
    uint y = uint(gl_FragCoord.y);
    uint viewportW = SceneUBO.viewport.x;

    uint index = viewportW*y + x;
    
    numFragments[index] = -1;
}