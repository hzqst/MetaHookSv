#version 430

layout(binding = 0) uniform sampler2D baseTex; // 0
layout(location = 0) uniform float rt_w; // GeeXLab built-in
layout(location = 1) uniform float rt_h; // GeeXLab built-in
uniform float FXAA_SPAN_MAX = 8.0;
uniform float FXAA_REDUCE_MUL = 1.0/8.0;
uniform float FXAA_SUBPIX_SHIFT = 1.0/4.0;

in vec2 texCoord;

layout(location = 0) out vec4 out_Color;

#define FxaaInt2 ivec2
#define FxaaFloat2 vec2

vec3 FxaaPixelShader(
  vec2 pos, // Fragment texture coordinate
  sampler2D tex, // Input texture
  vec2 rcpFrame) // Constant {1.0/frameWidth, 1.0/frameHeight}
{
/*---------------------------------------------------------*/
    #define FXAA_REDUCE_MIN   (1.0/128.0)
    //#define FXAA_REDUCE_MUL   (1.0/8.0)
    //#define FXAA_SPAN_MAX     8.0
/*---------------------------------------------------------*/
    vec2 posPos_zw = pos - (rcpFrame * (0.5 + FXAA_SUBPIX_SHIFT));

    vec3 rgbNW = texture(tex, posPos_zw).xyz;
    vec3 rgbNE = texture(tex, posPos_zw + FxaaInt2(1,0) * rcpFrame.xy).xyz;
    vec3 rgbSW = texture(tex, posPos_zw + FxaaInt2(0,1) * rcpFrame.xy).xyz;
    vec3 rgbSE = texture(tex, posPos_zw + FxaaInt2(1,1) * rcpFrame.xy).xyz;
    vec3 rgbM  = texture(tex, pos).xyz;
/*---------------------------------------------------------*/
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
/*---------------------------------------------------------*/
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
/*---------------------------------------------------------*/
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
/*---------------------------------------------------------*/
    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(FxaaFloat2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
          max(FxaaFloat2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) * rcpFrame.xy;
/*--------------------------------------------------------*/
    vec3 rgbA = (1.0/2.0) * (
        texture(tex, pos + dir * (1.0/3.0 - 0.5)).xyz +
        texture(tex, pos + dir * (2.0/3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        texture(tex, pos + dir * (0.0/3.0 - 0.5)).xyz +
        texture(tex, pos + dir * (3.0/3.0 - 0.5)).xyz);
    float lumaB = dot(rgbB, luma);
    if((lumaB < lumaMin) || (lumaB > lumaMax)) return rgbA;
    return rgbB; }

vec4 PostFX(sampler2D tex, vec2 uv, float time)
{
  vec4 c = vec4(0.0);
  vec2 rcpFrame = vec2(1.0/rt_w, 1.0/rt_h);
  c.rgb = FxaaPixelShader(uv, tex, rcpFrame);
  c.a = 1.0;
  return c;
}

void main()
{
	out_Color = PostFX(baseTex, texCoord, 0.0);
}