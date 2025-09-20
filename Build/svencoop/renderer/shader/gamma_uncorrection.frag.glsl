#version 430

#include "common.h"

in vec2 texCoord;

layout (location = 0) uniform float maxLambert = 1.0;

layout(binding = 0) uniform sampler2D baseTex;

layout(location = 0) out vec4 out_Color;

void main() {
  vec4 baseColor = texture(baseTex, texCoord);
  
  baseColor.r = clamp(baseColor.r, 0.0, maxLambert);
  baseColor.g = clamp(baseColor.g, 0.0, maxLambert);
  baseColor.b = clamp(baseColor.b, 0.0, maxLambert);

  baseColor = GammaToLinear(baseColor);

  out_Color = baseColor;
}