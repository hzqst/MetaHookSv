#version 430

#include "common.h"

in vec2 texCoord;

layout(binding = 0) uniform sampler2D baseTex;

layout(location=0) out vec4 out_Color;

void main() {
  vec4 baseColor = texture(baseTex, texCoord);
  
  baseColor = LinearToGamma(baseColor);

  out_Color = baseColor;
}