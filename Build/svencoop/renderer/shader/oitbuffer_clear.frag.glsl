#version 430

#define OIT_BLEND_ENABLED

#include "common.h"

in vec2 texCoord;

void main() {

    float x = texCoord.x;
    float y = texCoord.y;

    vec4 viewport = GetCameraViewPort(0);
    float viewportW = viewport.z;
    float viewportH = viewport.w;

    uint index = uint(viewportW*y + x);
    
    numFragments[index] = -1;
}