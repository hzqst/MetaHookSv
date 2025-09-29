#version 430

#define OIT_BLEND_ENABLED

#include "common.h"

in vec2 texCoord;

void main() {

    float x = gl_FragCoord.x;
    float y = gl_FragCoord.y;
    float viewportW = CameraUBO.viewport.z;
    float viewportH = CameraUBO.viewport.w;

    uint index = uint(viewportW*y + x);
    
    numFragments[index] = -1;
}