#version 430

#ifdef LUMTEX_ENABLED

uniform sampler2D lumTex;

uniform float exposure;

out float lum;

#endif

out vec2 texCoord;

void main()
{
#ifdef LUMTEX_ENABLED
  #ifdef TONEMAP_ENABLED
  lum = exposure / max(0.001, texture(lumTex, vec2(0.5, 0.5)).x);
  #else
  lum = min(texture2D(lumTex, vec2(0.5, 0.5)).x, 0.8);
  #endif
#endif

  uint idx = gl_VertexID % 3; // allows rendering multiple fullscreen triangles
  vec4 pos =  vec4(
      (float( idx     &1U)) * 4.0 - 1.0,
      (float((idx>>1U)&1U)) * 4.0 - 1.0,
      0, 1.0);
  gl_Position = pos;
  texCoord = pos.xy * 0.5 + 0.5;
}