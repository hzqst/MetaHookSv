#version 430

#define OIT_BLEND_ENABLED

#include "common.h"

#define MAX_NUM_FRAGS 128

uint colorList[MAX_NUM_FRAGS];
float depthList[MAX_NUM_FRAGS];

in vec2 texCoord;

layout(binding = 0) uniform sampler2D screenTex;

out vec4 fragColor;

void swapFrags(uint i, uint j) {
    uint cTemp = colorList[i];
    colorList[i] = colorList[j];
    colorList[j] = cTemp;
    float dTemp = depthList[i];
    depthList[i] = depthList[j];
    depthList[j] = dTemp;
}

vec4 blendBackToFront(uint fragsCount, vec4 screenColor)
{
    vec4 color = screenColor;

    for (uint i = 0; i < fragsCount; i++) {

        // Blend the accumulated color with the color of the fragment node
        vec4 colorSrc = unpackUnorm4x8(colorList[i]);

        //AlphaBlend
        float useAlphaBlend = step(0.0, depthList[i]);

        vec4 alphaBlendColor = vec4(color.rgb * (1.0 - colorSrc.a) + colorSrc.rgb * colorSrc.a, 1.0);
        vec4 additiveBlendColor = vec4(color.rgb + colorSrc.rgb, 1.0);

        color = mix(additiveBlendColor, alphaBlendColor, useAlphaBlend);
    }

    return color;
}

void maxHeapSink(uint x, uint fragsCount)
{
    uint c; // Child
    while((c = 2 * x + 1) < fragsCount) {
        // While children exist
        if(c + 1 < fragsCount && abs(depthList[c]) > abs(depthList[c+1]) ) {
            // Find the biggest of both
            ++c;
        }

        if( abs(depthList[x]) <= abs(depthList[c]) ) {
            // Does it have to sink
            return;
        } else {
            swapFrags(x, c);
            x = c; // Swap and sink again
        }
    }
}

vec4 heapSort(uint fragsCount, vec4 screenColor)
{
    uint i;
    for (i = (fragsCount + 1)/2 ; i > 0 ; --i) {
        // Bring it to heap structure
        maxHeapSink(i-1, fragsCount); // Sink all inner nodes
    }
    // Heap => Sorted List
    for (i=1;i<fragsCount;++i) {
        swapFrags(0, fragsCount-i); // Swap max to List End
        maxHeapSink(0, fragsCount-i); // Sink the max to obtain correct heap
    }

    return blendBackToFront(fragsCount, screenColor);
}

void main() {

    vec4 screenColor = texture2D(screenTex, texCoord);

    float x = gl_FragCoord.x;
    float y = gl_FragCoord.y;
    float viewportW = CameraUBO.viewport.z;
    float viewportH = CameraUBO.viewport.w;
    uint pixelIndex = uint(viewportW*y + x);

    // Get start offset from array
    uint fragOffset = numFragments[pixelIndex];

    int numFrags = 0;
    FragmentNode fragment;
    for (int i = 0; i < MAX_NUM_FRAGS; i++)
    {
        if (fragOffset == -1) {
            // End of list reached
            break;
        }

        fragment = nodes[fragOffset];
        fragOffset = fragment.next;

        colorList[i] = fragment.color;
        depthList[i] = fragment.depth;

        numFrags++;
    }

    if (numFrags == 0) {
        discard;
    }

    fragColor = heapSort(numFrags, screenColor);
}