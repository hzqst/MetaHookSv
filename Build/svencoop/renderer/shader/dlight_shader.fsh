#version 430

#include "common.h"

#extension GL_EXT_texture_array : require
#extension GL_EXT_gpu_shader4 : require

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

vec4 CalcLightInternal(vec3 World, vec3 LightDirection, vec3 Normal, vec2 vBaseTexCoord)
{
    vec4 AmbientColor = vec4(u_lightcolor, 1.0) * u_lightambient;
    vec4 DiffuseColor = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 SpecularColor = vec4(0.0, 0.0, 0.0, 0.0);

    uint stencilValue = texture(stencilTex, vBaseTexCoord).r;

    if((stencilValue & 2) == 0)
    {
        float DiffuseFactor = dot(Normal, -LightDirection);
    
        if (DiffuseFactor > 0.0) {
            DiffuseColor = vec4(u_lightcolor * u_lightdiffuse * DiffuseFactor, 1.0);
            vec3 VertexToEye = normalize(SceneUBO.viewpos.xyz - World);
            vec3 LightReflect = normalize(reflect(LightDirection, Normal));
            float SpecularFactor = dot(VertexToEye, LightReflect);
            if (SpecularFactor > 0.0) {

                float specularValue = texture2DArray(gbufferTex, vec3(vBaseTexCoord, GBUFFER_INDEX_SPECULAR)).r;

                SpecularFactor = pow(SpecularFactor, u_lightspecularpow);
                SpecularColor = vec4(u_lightcolor * u_lightspecular * SpecularFactor * specularValue, 1.0);
            }
        }
    }
    else
    {
        //flatshade
        DiffuseColor = vec4(u_lightcolor * u_lightdiffuse * 0.8, 1.0);  
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

#ifdef CONE_TEXTURE_ENABLED

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
#ifdef VOLUME_ENABLED
    vec2 vBaseTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;
#else
    vec2 vBaseTexCoord = v_texcoord.xy;
#endif

    vec4 worldnormColor = texture2DArray(gbufferTex, vec3(vBaseTexCoord, GBUFFER_INDEX_WORLDNORM));

    float depth = texture2D(depthTex, vBaseTexCoord).r;

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

//#endif

}