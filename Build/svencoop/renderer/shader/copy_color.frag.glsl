#version 430

#include "common.h"

in vec2 texCoord;

layout(binding = 0) uniform sampler2D baseTex;

layout(location = 0) out vec4 out_Color;

void main() {
  vec4 baseColor = texture(baseTex, texCoord);
  
  #if defined(HALO_ADD_ENABLED)

    // Store max color component in alpha for alpha blend of one/invSrcAlpha
    float flLuminance = max( baseColor.r, max( baseColor.g, baseColor.b ) );
    baseColor.a = pow( flLuminance, 0.8f );

  #endif
  
  out_Color = baseColor;
}