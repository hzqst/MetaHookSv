#version 430

#include "common.h"

layout(binding = DSHADE_BIND_DIFFUSE_TEXTURE) uniform sampler2D gbufferDiffuse;
layout(binding = DSHADE_BIND_LIGHTMAP_TEXTURE) uniform sampler2D gbufferLightmap;
layout(binding = DSHADE_BIND_WORLDNORM_TEXTURE) uniform sampler2D gbufferWorldNorm;
layout(binding = DSHADE_BIND_SPECULAR_TEXTURE) uniform sampler2D gbufferSpecular;
layout(binding = DSHADE_BIND_DEPTH_TEXTURE) uniform sampler2D depthTex;
layout(binding = DSHADE_BIND_STENCIL_TEXTURE) uniform usampler2D stencilTex;

#if defined(CONE_TEXTURE_ENABLED)
layout(binding = DSHADE_BIND_CONE_TEXTURE) uniform sampler2D coneTex;
#endif

#if defined(SHADOW_TEXTURE_ENABLED)
layout(binding = DSHADE_BIND_SHADOWMAP_TEXTURE) uniform sampler2DShadow shadowTex;
#endif

#if defined(CSM_ENABLED)
layout(binding = DSHADE_BIND_CSM_TEXTURE) uniform sampler2DArrayShadow csmTex;
#endif

#if defined(CUBEMAP_SHADOW_TEXTURE_ENABLED)
layout(binding = DSHADE_BIND_CUBEMAP_SHADOW_TEXTURE) uniform samplerCubeShadow cubemapShadowTex;
#endif

#if defined(SHADOW_TEXTURE_ENABLED)
uniform mat4 u_shadowmatrix;
uniform vec2 u_shadowtexel;
#endif

#if defined(CSM_ENABLED)
uniform mat4 u_csmMatrices[4];
uniform vec4 u_csmDistances;
uniform vec2 u_csmTexel;
#endif

#if defined(CUBEMAP_SHADOW_TEXTURE_ENABLED)
uniform float u_cubeShadowTexel;
#endif


#if defined(VOLUME_ENABLED)

uniform mat4 u_modelmatrix;

#endif

uniform vec3 u_lightpos;
uniform vec3 u_lightdir;
uniform vec3 u_lightright;
uniform vec3 u_lightup;
uniform vec3 u_lightcolor;
uniform vec2 u_lightcone;
uniform float u_lightSize;
uniform float u_lightradius;
uniform float u_lightambient;
uniform float u_lightdiffuse;
uniform float u_lightspecular;
uniform float u_lightspecularpow;

in vec3 v_fragpos;
in vec4 v_projpos;
in vec2 v_texcoord;

layout(location = 0) out vec4 out_FragColor;

#if defined(SHADOW_TEXTURE_ENABLED)

float ShadowCompareDepth(vec4 basecoord, vec2 floorcoord, vec2 offset, float texelSize)
{
    vec4 uv = basecoord;
    uv.xy = floorcoord.xy + basecoord.w * offset * texelSize;
    
    return textureProj(shadowTex, uv);
}

float CalcShadowIntensity(vec3 World, vec3 Norm, vec3 LightDirection)
{
    vec4 shadowCoords = u_shadowmatrix * vec4(World, 1.0);

	float visibility = 0.0;

    {
        // Depth bias to reduce light leaking and shadow acne
        float NdotL = max(dot(Norm, -LightDirection), 0.0);
        float slopeBias = 0.001 * (1.0 - NdotL);
        float constBias = 0.5 * u_shadowtexel.y; // scale with texel size
        shadowCoords.z += max(slopeBias, constBias);

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

#if defined(CSM_ENABLED)

float CalcCSMShadowIntensity(vec3 World, vec3 Norm, vec3 LightDirection, vec2 vBaseTexCoord)
{
    float distanceFromCamera = length(World - GetCameraViewPos(0));

    // Determine which cascade to use
    int cascadeIndex = CSM_LEVELS - 1; // Default to furthest cascade
    int nextCascadeIndex = -1;
    float cascadeBlendFactor = 0.0;

    for (int i = 0; i < CSM_LEVELS; ++i)
    {
        if (distanceFromCamera < u_csmDistances[i])
        {
            cascadeIndex = i;

            // Check if we need to blend with next cascade
            if (i < CSM_LEVELS - 1)
            {
                float blendDistance = (u_csmDistances[i+1] - u_csmDistances[i]) * 0.1; // 10% blend region
                float distToNextBoundary = u_csmDistances[i+1] - distanceFromCamera;

                if (distToNextBoundary < blendDistance)
                {
                    nextCascadeIndex = i + 1;
                    cascadeBlendFactor = 1.0 - (distToNextBoundary / blendDistance);
                }
            }
            break;
        }
    }

    // Calculate shadow coordinates for selected cascade
    vec4 shadowCoords = u_csmMatrices[cascadeIndex] * vec4(World, 1.0);

    shadowCoords.z += 0.001 * (cascadeIndex * 0.5 + 0.5);

    float texRes = u_csmTexel.x;
    float invRes = u_csmTexel.y;
    float pcfRadius = 1.0;

    float visibility = 0.0;

    // Check if we're outside the shadow map bounds
    // Perspective divide
    vec3 projCoords = shadowCoords.xyz / shadowCoords.w;

    {
        // Improved PCF filtering with more samples for smoother shadows
        int pcfSamples = 0;

        for(int x = -1; x <= 1; x++)
        {
            for(int y = -1; y <= 1; y++)
            {
                vec2 offset = vec2(float(x), float(y)) * pcfRadius * invRes;
                vec4 sampleCoord = vec4(projCoords.xy + offset, float(cascadeIndex), projCoords.z);
                visibility += texture(csmTex, sampleCoord);
                pcfSamples++;
            }
        }

        visibility /= float(pcfSamples);
    }

    // Blend with next cascade if needed for smooth transitions
    if (nextCascadeIndex != -1 && cascadeBlendFactor > 0.0)
    {
        vec4 nextShadowCoords = u_csmMatrices[nextCascadeIndex] * vec4(World, 1.0);

        float bias = 0.001 * (nextCascadeIndex * 0.5 + 0.5);

        nextShadowCoords.z += bias;

        float nextVisibility = 0.0;

        vec3 nextProjCoords = nextShadowCoords.xyz / nextShadowCoords.w;

        {
            // Simple PCF for next cascade
            int nextPcfSamples = 0;
            for(int x = -1; x <= 1; x++)
            {
                for(int y = -1; y <= 1; y++)
                {
                    vec2 offset = vec2(float(x), float(y)) * pcfRadius * invRes;
                    vec4 sampleCoord = vec4(nextProjCoords.xy + offset, float(nextCascadeIndex), nextProjCoords.z);
                    nextVisibility += texture(csmTex, sampleCoord);
                    nextPcfSamples++;
                }
            }
            nextVisibility /= float(nextPcfSamples);
        }

        // Blend between current and next cascade
        visibility = mix(visibility, nextVisibility, cascadeBlendFactor);
    }

    return visibility;
}

#endif

#if defined(CUBEMAP_SHADOW_TEXTURE_ENABLED)

float CubeShadowCompareDepth(vec4 basecoord, vec2 offset, float texelSize)
{
    vec4 uv = basecoord;
    
    uv.xy += basecoord.w * offset * texelSize;
    
    return texture(cubemapShadowTex, uv);
}

float CalcCubemapShadowIntensity(vec3 World, vec3 LightPos, vec3 Normal, vec3 LightDirection)
{
    vec3 lightToWorld = World - LightPos;
    float distanceLightToWorld = length(lightToWorld);
    vec3 lightToWorldDir = lightToWorld / distanceLightToWorld;
    
    // Shadow parameters matching the shadow pass
    float zNear = 0.1;
    float zFar = u_lightradius;
    
    //We store linearDepth in cubeShadowTex
    float linearDepth = distanceLightToWorld / zFar;

    // Calculate bias based on surface angle
    float NdotL = max(dot(Normal, -LightDirection), 0.0);
    float slopeBias = 0.0002 * (1.0 - NdotL);
    float constBias = 0.0004;
    float bias = max(slopeBias, constBias);
    
    // Sample cubemap shadow texture
    // For cubemap, we use the direction + computed depth
    vec4 cubeSampleCoord = vec4(lightToWorldDir, linearDepth - bias);

    float visibility = CubeShadowCompareDepth(cubeSampleCoord, vec2(0.0, 0.0), u_cubeShadowTexel);
    
    return visibility;
}

#endif

void CalcLightInternal(vec3 World, vec3 LightDirection, vec3 Normal, vec2 vBaseTexCoord, out vec4 AmbientColor, out vec4 DiffuseSpecularColor)
{
    AmbientColor = vec4(u_lightcolor, 1.0) * u_lightambient;
    vec4 DiffuseColor = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 SpecularColor = vec4(0.0, 0.0, 0.0, 0.0);

    uint stencilValue = texture(stencilTex, vBaseTexCoord).r;

    if((stencilValue & STENCIL_MASK_HAS_FLATSHADE) == STENCIL_MASK_HAS_FLATSHADE)
    {
        //flatshade
        DiffuseColor = vec4(u_lightcolor * u_lightdiffuse * 0.8, 1.0);
    }
    else if((stencilValue & STENCIL_MASK_NO_LIGHTING) == STENCIL_MASK_NO_LIGHTING)
    {
        AmbientColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
    else
    {
        float DiffuseFactor = dot(Normal, -LightDirection);

        if (DiffuseFactor > 0.0) {
            DiffuseColor = vec4(u_lightcolor * u_lightdiffuse * DiffuseFactor, 1.0);
            vec3 VertexToEye = normalize(GetCameraViewPos(0) - World);
            vec3 LightReflect = normalize(reflect(LightDirection, Normal));
            float SpecularFactor = dot(VertexToEye, LightReflect);
            if (SpecularFactor > 0.0) {

                float specularValue = texture(gbufferSpecular, vBaseTexCoord).r;

                SpecularFactor = pow(SpecularFactor, u_lightspecularpow);
                SpecularColor = vec4(u_lightcolor * u_lightspecular * SpecularFactor * specularValue, 1.0);
            }
        }
    }
    DiffuseSpecularColor = DiffuseColor + SpecularColor;
}

vec4 CalcPointLight(vec3 World, vec3 Normal, vec2 vBaseTexCoord)
{
    vec3 LightDirection = World - u_lightpos.xyz;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);

    vec4 AmbientColor, DiffuseSpecularColor;
    CalcLightInternal(World, LightDirection, Normal, vBaseTexCoord, AmbientColor, DiffuseSpecularColor);

    vec4 Color = AmbientColor + DiffuseSpecularColor;

    float r2 = u_lightradius * u_lightradius;
    float Attenuation = clamp(( r2 - (Distance * Distance)) / r2, 0.0, 1.0);

#if defined(CUBEMAP_SHADOW_TEXTURE_ENABLED)
    float flShadowIntensity = CalcCubemapShadowIntensity(World, u_lightpos.xyz, Normal, LightDirection);
    Color.r *= flShadowIntensity;
    Color.g *= flShadowIntensity;
    Color.b *= flShadowIntensity;
#endif

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

vec4 CalcDirectionalLight(vec3 World, vec3 Normal, vec2 vBaseTexCoord)
{
    vec3 LightDirection = u_lightdir.xyz;

    vec4 AmbientColor, DiffuseSpecularColor;
    CalcLightInternal(World, LightDirection, Normal, vBaseTexCoord, AmbientColor, DiffuseSpecularColor);

    float flShadowIntensity = 1.0;

#if defined(SHADOW_TEXTURE_ENABLED)

    flShadowIntensity = CalcShadowIntensity(World, Normal, u_lightdir.xyz);

#endif

#if defined(CSM_ENABLED)

    float flCSMShadowIntensity = CalcCSMShadowIntensity(World, Normal, LightDirection, vBaseTexCoord);

    flShadowIntensity = min(flShadowIntensity, flCSMShadowIntensity);

#endif

    vec4 Color = AmbientColor + DiffuseSpecularColor;
    Color.r *= flShadowIntensity;
    Color.g *= flShadowIntensity;
    Color.b *= flShadowIntensity;

    return Color;
}

void main()
{
#if defined(VOLUME_ENABLED)
    vec2 vBaseTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;
#else
    vec2 vBaseTexCoord = v_texcoord.xy;
#endif

    vec4 worldnormColor = texture(gbufferWorldNorm, vBaseTexCoord);

    vec3 normal = OctahedronToUnitVector(worldnormColor.xy);

    vec3 worldpos = GetCameraViewPos(0) + normalize( v_fragpos.xyz - GetCameraViewPos(0) ) * worldnormColor.z;

#if defined(SPOT_ENABLED)
    out_FragColor = CalcSpotLight(worldpos, normal, vBaseTexCoord);
#elif defined(POINT_ENABLED)
    out_FragColor = CalcPointLight(worldpos, normal, vBaseTexCoord);
#elif defined(DIRECTIONAL_ENABLED)
    out_FragColor = CalcDirectionalLight(worldpos, normal, vBaseTexCoord);
#endif

}