#version 460

#define OIT_BLEND_ENABLED

#include "common.h"

in vec2 texCoord;

uint colorList[MAX_NUM_FRAGS];
float depthList[MAX_NUM_FRAGS];

out vec4 fragColor;

void swapFrags(uint i, uint j) {
    uint cTemp = colorList[i];
    colorList[i] = colorList[j];
    colorList[j] = cTemp;
    float dTemp = depthList[i];
    depthList[i] = depthList[j];
    depthList[j] = dTemp;
}

vec4 blendFTB(uint fragsCount)
{
    vec4 color = vec4(0.0);
    for (uint i = 0; i < fragsCount; i++) {
        // Front-to-Back (FTB) blending
        // Blend the accumulated color with the color of the fragment node
        vec4 colorSrc = unpackUnorm4x8(colorList[i]);
        float alphaSrc = colorSrc.a;
        color.rgb = color.rgb + (1.0 - color.a) * alphaSrc * colorSrc.rgb;
        color.a = color.a + (1.0 - color.a) * alphaSrc;
    }
    return vec4(color.rgb / color.a, color.a);
}

void maxHeapSink(uint x, uint fragsCount)
{
    uint c; // Child
    while((c = 2 * x + 1) < fragsCount) {
        // While children exist
        if(c + 1 < fragsCount && depthList[c] < depthList[c+1]) {
            // Find the biggest of both
            ++c;
        }

        if(depthList[x] >= depthList[c]) {
            // Does it have to sink
            return;
        } else {
            swapFrags(x, c);
            x = c; // Swap and sink again
        }
    }
}

vec4 heapSort(uint fragsCount)
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

    return blendFTB(fragsCount);
}

void main() {

    int viewportW = SceneUBO.viewport.x;
    int x = int(gl_FragCoord.x);
    int y = int(gl_FragCoord.y);
    int pixelIndex = viewportW*y + x;

    // Get start offset from array
    uint fragOffset = OITStartOffsetSSBO[pixelIndex];

    // Collect all fragments for this pixel
    int numFrags = 0;
    LinkedListFragmentNode fragment;
    for (int i = 0; i < MAX_NUM_FRAGS; i++)
    {
        if (fragOffset == -1) {
            // End of list reached
            break;
        }

        fragment = OITFragmentSSBO[fragOffset];
        fragOffset = fragment.next;

        colorList[i] = fragment.color;
        depthList[i] = fragment.depth;

        numFrags++;
    }

    if (numFrags == 0) {
        discard;
    }

    fragColor = heapSort(numFrags);
}