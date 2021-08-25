#version 460

#define OIT_ALPHA_BLEND_ENABLED

#include "common.h"

in vec2 texCoord;

layout(binding = 0) uniform sampler2D screenTex;

out vec4 fragColor;

void main() {

    uint x = uint(gl_FragCoord.x);
    uint y = uint(gl_FragCoord.y);
    uint viewportW = SceneUBO.viewport.x;
    uint pixelIndex = addrGen(uvec2(x,y), viewportW);

    vec4 color = texture2D(screenTex, texCoord);

    int offset = int(numFragmentsBuffer[pixelIndex])-1;
    int index = MAX_NUM_NODES * int(pixelIndex) + offset;
    for (int i = offset; i >= 0; i--)
    {
        // Blend the accumulated color with the color of the fragment node
        vec4 colorSrc = unpackUnorm4x8(nodes[index].color);

        float useAlphaBlend = step(0.0, nodes[index].depth);

        vec4 alphaBlendColor = vec4(colorSrc.rgb * colorSrc.a + color.rgb * (1.0 - colorSrc.a), 1.0);
        vec4 additiveBlendColor = vec4(color.rgb + colorSrc.rgb, 1.0);

        color = mix(additiveBlendColor, alphaBlendColor, useAlphaBlend);

        index--;
    }

    fragColor = color;
}