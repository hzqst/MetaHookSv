#version 430

in vec2 texCoord;

uniform vec2 texelsize;

layout(binding=0) uniform sampler2D baseTex;

layout(location=0) out vec4 out_Color;

void main() {
  vec4 color = vec4(0.0);
#if defined(DOWNSAMPLE_2X2)
  color += 0.25 * texture( baseTex, texCoord.xy );
  color += 0.25 * texture( baseTex, texCoord.xy + vec2(texelsize.x, 0.0) );
  color += 0.25 * texture( baseTex, texCoord.xy + vec2(texelsize.x, texelsize.y) );
  color += 0.25 * texture( baseTex, texCoord.xy + vec2(0.0, texelsize.y) );
#else
  color += texture( baseTex, texCoord.xy );
#endif
  out_Color = color;
}