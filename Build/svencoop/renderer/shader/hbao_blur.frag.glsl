#version 430

const float KERNEL_RADIUS = 3;

layout(location=0) uniform float g_Sharpness;
layout(location=1) uniform vec2 g_InvResolutionDirection; // either set x to 1/width or y to 1/height

layout(binding=0) uniform sampler2D texSource;


in vec2 texCoord;

layout(location=0,index=0) out vec4 out_Color;

float BlurFunction(vec2 uv, float r, float center_c, float center_d, inout float w_total)
{
  vec2  aoz = texture2D( texSource, uv ).xy;
  float c = aoz.x;
  float d = aoz.y;
  
  const float BlurSigma = KERNEL_RADIUS * 0.5;
  const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);
  
  float ddiff = (d - center_d) * g_Sharpness;
  float w = exp2(-r*r*BlurFalloff - ddiff*ddiff);
  w_total += w;

  return c*w;
}

void main()
{
  vec2  aoz = texture2D( texSource, texCoord ).xy;
  float center_c = aoz.x;
  float center_d = aoz.y;
  
  float c_total = center_c;
  float w_total = 1.0;
  
  for (float r = 1; r <= KERNEL_RADIUS; ++r)
  {
    vec2 uv = texCoord + g_InvResolutionDirection * r;
    c_total += BlurFunction(uv, r, center_c, center_d, w_total);  
  }
  
  for (float r = 1; r <= KERNEL_RADIUS; ++r)
  {
    vec2 uv = texCoord - g_InvResolutionDirection * r;
    c_total += BlurFunction(uv, r, center_c, center_d, w_total);  
  }
  
#ifdef AO_BLUR_PRESENT

  out_Color = vec4(c_total/w_total);

#else

  out_Color = vec4(c_total/w_total, center_d, 0, 0);

#endif

}
