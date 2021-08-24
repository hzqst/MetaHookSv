#version 460

#define OIT_BLEND_ENABLED

#include "common.h"

in vec2 texCoord;

void main() {
    int viewportW = SceneUBO.viewport.x;
    int x = int(gl_FragCoord.x);
    int y = int(gl_FragCoord.y);
    int index = viewportW * y + x;
    
    OITStartOffsetSSBO[index] = -1;
}