#version 430

#define OIT_BLEND_ENABLED

#include "common.h"

in vec2 texCoord;

void main() {

    float x = gl_FragCoord.x;
    float y = gl_FragCoord.y;
    float viewportW = SceneUBO.viewport.x;

    uint index = uint(viewportW*y + x);
    
    numFragments[index] = -1;
}