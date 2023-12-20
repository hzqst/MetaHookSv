#version 430

#extension GL_EXT_texture_array : require
#extension GL_EXT_gpu_shader4 : require

#include "common.h"

#define GBUFFER_INDEX_DIFFUSE		0.0
#define GBUFFER_INDEX_LIGHTMAP		1.0
#define GBUFFER_INDEX_WORLDNORM		2.0
#define GBUFFER_INDEX_SPECULAR		3.0
#define GBUFFER_INDEX_ADDITIVE		4.0

layout(binding = 0) uniform sampler2DArray gbufferTex;
layout(binding = 1) uniform sampler2D depthTex;
layout(binding = 2) uniform usampler2D stencilTex;

#ifdef CONE_TEXTURE_ENABLED
layout(binding = 3) uniform sampler2D coneTex;
#endif

#ifdef SHADOW_TEXTURE_ENABLED
layout(binding = 4) uniform sampler2DShadow shadowTex;
#endif

#ifdef SHADOW_TEXTURE_ENABLED

uniform mat4 u_shadowmatrix;
uniform vec2 u_shadowtexel;

#endif

#ifdef VOLUME_ENABLED

uniform mat4 u_modelmatrix;

#endif

uniform vec3 u_lightpos;
uniform vec3 u_lightdir;
uniform vec3 u_lightright;
uniform vec3 u_lightup;
uniform vec3 u_lightcolor;
uniform vec2 u_lightcone;
uniform float u_lightradius;
uniform float u_lightambient;
uniform float u_lightdiffuse;
uniform float u_lightspecular;
uniform float u_lightspecularpow;

in vec3 v_fragpos;
in vec4 v_projpos;
in vec2 v_texcoord;

layout(location = 0) out vec4 out_FragColor;

vec3 GenerateViewPositionFromDepth(vec2 texcoord, float depth) {
    vec2 texcoord2 = vec2((texcoord.x - 0.5) * 2.0, (texcoord.y - 0.5) * 2.0);
	vec4 ndc = vec4(texcoord2.xy, depth, 1.0);
	vec4 inversed = SceneUBO.invProjMatrix * ndc;// going back from projected
	inversed /= inversed.w;
	return inversed.xyz;
}

vec3 GenerateWorldPositionFromDepth(vec2 texcoord, float depth) {
   vec2 texcoord2 = vec2((texcoord.x - 0.5) * 2.0, (texcoord.y - 0.5) * 2.0);
	vec4 ndc = vec4(texcoord2.xy, depth, 1.0);
	vec4 inversed = SceneUBO.invViewMatrix * SceneUBO.invProjMatrix * ndc;// going back from projected
	inversed /= inversed.w;
	return inversed.xyz;
}

#if defined(SHADOW_TEXTURE_ENABLED)

float ShadowCompareDepth(vec4 basecoord, vec2 floorcoord, vec2 offset, float texelSize)
{
    vec4 uv = basecoord;
    uv.xy = floorcoord.xy + offset * texelSize * basecoord.w;
    
    return textureProj(shadowTex, uv);
}

float CalcShadowIntensity(vec3 World, vec3 Norm, vec3 LightDirection)
{
    vec4 shadowCoords = u_shadowmatrix * vec4(World, 1.0);

	float visibility = 0.0;

    if(shadowCoords.z / shadowCoords.w > 1.0)
    {
        visibility = 1.0;
    }
    else
    {
        float texRes = u_shadowtexel.x;
        float invRes = u_shadowtexel.y;

        vec2 uv = shadowCoords.xy * texRes;

        vec2 flooredUV = vec2(floor(uv.x), floor(uv.y));

        float s = fract(uv.x);
        float t = fract(uv.y);

        flooredUV *= invRes;

        /*float uw0 = (4.0 - 3.0 * s);
        float uw1 = 7.0;
        float uw2 = (1.0 + 3.0 * s);

        float u0 = (3.0 - 2.0 * s) / uw0 - 2.0;
        float u1 = (3.0 + s) / uw1;
        float u2 = s / uw2 + 2.0;

        float vw0 = (4.0 - 3.0 * t);
        float vw1 = 7.0;
        float vw2 = (1.0 + 3.0 * t);

        float v0 = (3.0 - 2.0 * t) / vw0 - 2.0;
        float v1 = (3.0 + t) / vw1;
        float v2 = t / vw2 + 2.0;

        visibility += uw0 * vw0 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u0, v0), invRes);
        visibility += uw1 * vw0 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u1, v0), invRes);
        visibility += uw2 * vw0 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u2, v0), invRes);

        visibility += uw0 * vw1 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u0, v1), invRes);
        visibility += uw1 * vw1 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u1, v1), invRes);
        visibility += uw2 * vw1 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u2, v1), invRes);

        visibility += uw0 * vw2 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u0, v2), invRes);
        visibility += uw1 * vw2 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u1, v2), invRes);
        visibility += uw2 * vw2 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u2, v2), invRes);

        visibility /= 144.0;*/

        float uw0 = (5.0 * s - 6.0);
        float uw1 = (11.0 * s - 28.0);
        float uw2 = -(11.0 * s + 17.0);
        float uw3 = -(5.0 * s + 1.0);

        float u0 = (4.0 * s - 5.0) / uw0 - 3.0;
        float u1 = (4.0 * s - 16.0) / uw1 - 1.0;
        float u2 = -(7.0 * s + 5.0) / uw2 + 1.0;
        float u3 = -s / uw3 + 3.0;

        float vw0 = (5.0 * t - 6.0);
        float vw1 = (11.0 * t - 28.0);
        float vw2 = -(11.0 * t + 17.0);
        float vw3 = -(5.0 * t + 1.0);

        float v0 = (4.0 * t - 5.0) / vw0 - 3.0;
        float v1 = (4.0 * t - 16.0) / vw1 - 1.0;
        float v2 = -(7.0 * t + 5.0) / vw2 + 1.0;
        float v3 = -t / vw3 + 3.0;

        visibility += uw0 * vw0 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u0, v0), invRes);
        visibility += uw1 * vw0 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u1, v0), invRes);
        visibility += uw2 * vw0 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u2, v0), invRes);
        visibility += uw3 * vw0 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u3, v0), invRes);

        visibility += uw0 * vw1 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u0, v1), invRes);
        visibility += uw1 * vw1 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u1, v1), invRes);
        visibility += uw2 * vw1 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u2, v1), invRes);
        visibility += uw3 * vw1 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u3, v1), invRes);

        visibility += uw0 * vw2 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u0, v2), invRes);
        visibility += uw1 * vw2 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u1, v2), invRes);
        visibility += uw2 * vw2 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u2, v2), invRes);
        visibility += uw3 * vw2 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u3, v2), invRes);

        visibility += uw0 * vw3 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u0, v3), invRes);
        visibility += uw1 * vw3 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u1, v3), invRes);
        visibility += uw2 * vw3 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u2, v3), invRes);
        visibility += uw3 * vw3 * ShadowCompareDepth(shadowCoords, flooredUV, vec2(u3, v3), invRes);

        visibility /= 2704.0;
    }

    return visibility;
}

#endif

vec4 CalcLightInternal(vec3 World, vec3 LightDirection, vec3 Normal, vec2 vBaseTexCoord)
{
    vec4 AmbientColor = vec4(u_lightcolor, 1.0) * u_lightambient;
    vec4 DiffuseColor = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 SpecularColor = vec4(0.0, 0.0, 0.0, 0.0);

    uint stencilValue = texture(stencilTex, vBaseTexCoord).r;

    if((stencilValue & STENCIL_MASK_HAS_FLATSHADE) == STENCIL_MASK_HAS_FLATSHADE)
    {
        //flatshade
        DiffuseColor = vec4(u_lightcolor * u_lightdiffuse * 0.8, 1.0);
    }
    else
    {
        float DiffuseFactor = dot(Normal, -LightDirection);
    
        if (DiffuseFactor > 0.0) {
            DiffuseColor = vec4(u_lightcolor * u_lightdiffuse * DiffuseFactor, 1.0);
            vec3 VertexToEye = normalize(SceneUBO.viewpos.xyz - World);
            vec3 LightReflect = normalize(reflect(LightDirection, Normal));
            float SpecularFactor = dot(VertexToEye, LightReflect);
            if (SpecularFactor > 0.0) {

                float specularValue = texture(gbufferTex, vec3(vBaseTexCoord, GBUFFER_INDEX_SPECULAR)).r;

                SpecularFactor = pow(SpecularFactor, u_lightspecularpow);
                SpecularColor = vec4(u_lightcolor * u_lightspecular * SpecularFactor * specularValue, 1.0);
            }
        }
    }
    return (AmbientColor + DiffuseColor + SpecularColor);
}

vec4 CalcPointLight(vec3 World, vec3 Normal, vec2 vBaseTexCoord)
{
    vec3 LightDirection = World - u_lightpos.xyz;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);
 
    vec4 Color = CalcLightInternal(World, LightDirection, Normal, vBaseTexCoord);

    float r2 = u_lightradius * u_lightradius;
    float Attenuation = clamp(( r2 - (Distance * Distance)) / r2, 0.0, 1.0);
 
    return Color * Attenuation;
}

vec4 CalcSpotLight(vec3 World, vec3 Normal, vec2 vBaseTexCoord)
{
    vec3 LightToPixel = normalize(World - u_lightpos.xyz);
    float SpotCosine = dot(LightToPixel, u_lightdir.xyz);
    float LimitCosine = u_lightcone.x;
    float LimitSine = u_lightcone.y;
    if (SpotCosine > LimitCosine) {
        vec4 Color = CalcPointLight(World, Normal, vBaseTexCoord);

#if defined(SHADOW_TEXTURE_ENABLED)

        float flShadowIntensity = CalcShadowIntensity(World, Normal, u_lightdir.xyz);
        Color.r *= flShadowIntensity;
        Color.g *= flShadowIntensity;
        Color.b *= flShadowIntensity;

#endif

#if defined(CONE_TEXTURE_ENABLED)

        float flConeProjX = dot(u_lightright, LightToPixel);
        float flConeProjY = dot(u_lightup, LightToPixel);

        //map from (-LimitSine, LimitSine) to (0, 1)
        float flConeProjU = (flConeProjX * 1.0 / LimitSine + 1.0) * 0.5;
        float flConeProjV = (flConeProjY * 1.0 / LimitSine + 1.0) * 0.5;

        vec4 vConeColor = texture(coneTex, vec2(flConeProjU, flConeProjV));

        return Color * vConeColor;
#else

        float flConeFactor = (SpotCosine - LimitCosine) * (1.0 / LimitSine);

        return Color * flConeFactor;
#endif

    }
    else {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
}

void main()
{
#if defined(VOLUME_ENABLED)
    vec2 vBaseTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;
#else
    vec2 vBaseTexCoord = v_texcoord.xy;
#endif

    vec4 worldnormColor = texture(gbufferTex, vec3(vBaseTexCoord, GBUFFER_INDEX_WORLDNORM));

    float depth = texture(depthTex, vBaseTexCoord).r;

    //vec3 worldpos = GenerateWorldPositionFromDepth(vBaseTexCoord, depth);

    vec3 normal = OctahedronToUnitVector(worldnormColor.xy);

    vec3 worldpos = SceneUBO.viewpos.xyz + normalize(v_fragpos.xyz - SceneUBO.viewpos.xyz) * worldnormColor.z;

//#ifndef VOLUME_ENABLED
    //out_FragColor = vec4(vBaseTexCoord.x, vBaseTexCoord.y, 0.0, 0.0); 
//#else

#if defined(SPOT_ENABLED)
    out_FragColor = CalcSpotLight(worldpos, normal, vBaseTexCoord);
#elif defined(POINT_ENABLED)
    out_FragColor = CalcPointLight(worldpos, normal, vBaseTexCoord);
#endif

}