#version 430

#include "common.h"

layout(location=0) uniform vec2 texelsize;

layout(binding=0) uniform sampler2D baseTex;

in vec2 texCoord;

layout(location=0) out float out_Color;

const vec3 LUMINANCE_WEIGHTS = vec3(0.27, 0.67, 0.06);

void main() {

  float color = 0.0;

#if defined(LUMPASS_LOG)

  color += 0.25 * log( dot( texture( baseTex, texCoord.xy ).xyz, LUMINANCE_WEIGHTS) + 0.0001 );
  color += 0.25 * log( dot( texture( baseTex, texCoord.xy + vec2(texelsize.x, 0.0) ).xyz, LUMINANCE_WEIGHTS) + 0.0001 );
  color += 0.25 * log( dot( texture( baseTex, texCoord.xy + vec2(texelsize.x, texelsize.y) ).xyz, LUMINANCE_WEIGHTS) + 0.0001 );
  color += 0.25 * log( dot( texture( baseTex, texCoord.xy + vec2(0.0, texelsize.y) ).xyz, LUMINANCE_WEIGHTS) + 0.0001 );

#elif defined(LUMPASS_EXP)

  color += 0.25 * texture( baseTex, texCoord.xy ).x;
  color += 0.25 * texture( baseTex, texCoord.xy + vec2(texelsize.x, 0.0) ).x;
  color += 0.25 * texture( baseTex, texCoord.xy + vec2(texelsize.x, texelsize.y) ).x;
  color += 0.25 * texture( baseTex, texCoord.xy + vec2(0.0, texelsize.y) ).x;

  color = max(exp(color), 0.1);

#else

  color += 0.25 * texture( baseTex, texCoord.xy ).x;
  color += 0.25 * texture( baseTex, texCoord.xy + vec2(texelsize.x, 0.0) ).x;
  color += 0.25 * texture( baseTex, texCoord.xy + vec2(texelsize.x, texelsize.y) ).x;
  color += 0.25 * texture( baseTex, texCoord.xy + vec2(0.0, texelsize.y) ).x;

#endif

  out_Color = color;

}