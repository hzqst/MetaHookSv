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

#if defined(STATIC_SHADOW_TEXTURE_ENABLED)
layout(binding = DSHADE_BIND_STATIC_SHADOW_TEXTURE) uniform sampler2DShadow staticShadowTex;
#endif

#if defined(DYNAMIC_SHADOW_TEXTURE_ENABLED)
layout(binding = DSHADE_BIND_DYNAMIC_SHADOW_TEXTURE) uniform sampler2DShadow dynamicShadowTex;
#endif

#if defined(STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED)
layout(binding = DSHADE_BIND_STATIC_CUBEMAP_SHADOW_TEXTURE) uniform samplerCubeShadow staticCubemapShadowTex;
#endif

#if defined(DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED)
layout(binding = DSHADE_BIND_DYNAMIC_CUBEMAP_SHADOW_TEXTURE) uniform samplerCubeShadow dynamicCubemapShadowTex;
#endif

#if defined(CSM_SHADOW_TEXTURE_ENABLED)
layout(binding = DSHADE_BIND_CSM_TEXTURE) uniform sampler2DArrayShadow csmTex;
#endif

#if defined(STATIC_SHADOW_TEXTURE_ENABLED)
uniform mat4 u_staticShadowMatrix;
uniform vec2 u_staticShadowTexel;
#endif

#if defined(DYNAMIC_SHADOW_TEXTURE_ENABLED)
uniform mat4 u_dynamicShadowMatrix;
uniform vec2 u_dynamicShadowTexel;
#endif

#if defined(STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED)
uniform vec2 u_staticCubemapShadowTexel;
#endif

#if defined(DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED)
uniform vec2 u_dynamicCubemapShadowTexel;
#endif

#if defined(CSM_SHADOW_TEXTURE_ENABLED)
uniform mat4 u_csmMatrices[4];
uniform vec4 u_csmDistances;
uniform vec2 u_csmTexel;
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

#if defined(STATIC_SHADOW_TEXTURE_ENABLED) || defined(DYNAMIC_SHADOW_TEXTURE_ENABLED)

float ShadowCompareDepth(sampler2DShadow tex, vec3 sampleCoord, float compareDepth)
{
    return texture(tex, vec3(sampleCoord.xy, compareDepth));
}

// Linear depth version for Spotlight (uses linear depth like cubemap shadow)
float CalcShadowIntensityLinear(sampler2DShadow shadowTex, vec3 World, vec3 Norm, vec3 LightDirection, vec3 LightPos, float LightRadius, vec2 shadowTexel, mat4 shadowMatrix)
{
    // Transform world position to shadow space
    vec4 shadowCoords4 = shadowMatrix * vec4(World, 1.0);
    
    // Perspective divide to get NDC coordinates
    vec3 projCoords = shadowCoords4.xyz / shadowCoords4.w;
    
    // Calculate linear depth like cubemap shadow
    // Distance from light to world position
    vec3 lightToWorld = World - LightPos;
    float distanceLightToWorld = length(lightToWorld);
    
    // Normalize to [0, 1] range using light radius
    float zNear = 0.1;
    float zFar = LightRadius;
    float linearDepth = distanceLightToWorld / zFar;
    
    // Improved bias calculation matching cubemap shadow
    float NdotL = max(dot(Norm, -LightDirection), 0.0);
    
    // Slope-based bias: larger bias for surfaces at grazing angles
    float slopeBias = 0.002 * pow(1.0 - NdotL, 2.0);
    
    float invRes = shadowTexel.y;
    
    // Distance-based bias: account for texel size at different distances
    float texelWorldSize = distanceLightToWorld * invRes * 2.0;
    float distanceBias = texelWorldSize * 0.5;
    
    // Constant base bias
    float constBias = 0.0003;
    
    // Normal offset bias: push the sample point slightly along the normal
    float normalOffsetScale = sqrt(distanceLightToWorld) * invRes * 0.5;
    normalOffsetScale = min(normalOffsetScale, 0.1);
    
    vec3 offsetWorld = World + Norm * normalOffsetScale;
    vec3 offsetLightToWorld = offsetWorld - LightPos;
    float offsetDistance = length(offsetLightToWorld);
    float offsetLinearDepth = offsetDistance / zFar;
    
    // Combine all bias components
    float bias = constBias + slopeBias + distanceBias;
    bias = min(bias, 0.01);
    
    float compareDepth = offsetLinearDepth - bias;
    compareDepth = clamp(compareDepth, 0.0, 1.0);
    
    float visibility = 0.0;
    
    // PCF filtering with 3x3 kernel
    float texRes = shadowTexel.x;
    float pcfRadius = 1.0;
    int pcfSamples = 0;
    
    for(int x = -1; x <= 1; x++)
    {
        for(int y = -1; y <= 1; y++)
        {
            vec2 offset = vec2(float(x), float(y)) * pcfRadius * invRes;
            vec3 sampleCoord = vec3(projCoords.xy + offset, 0.0);
            visibility += ShadowCompareDepth(shadowTex, sampleCoord, compareDepth);
            pcfSamples++;
        }
    }
    
    visibility /= float(pcfSamples);
    
    return visibility;
}

// Non-linear depth version for Directional Light (traditional shadow mapping)
float CalcShadowIntensity(sampler2DShadow shadowTex, vec3 World, vec3 Norm, vec3 LightDirection, vec2 shadowTexel, mat4 shadowMatrix)
{
    vec4 shadowCoords = shadowMatrix * vec4(World, 1.0);

	float visibility = 0.0;

    {
        // Depth bias to reduce light leaking and shadow acne
        float NdotL = max(dot(Norm, -LightDirection), 0.0);
        float slopeBias = 0.001 * (1.0 - NdotL);
        float constBias = 0.5 * shadowTexel.y; // scale with texel size
        shadowCoords.z += max(slopeBias, constBias);

        float texRes = shadowTexel.x;
        float invRes = shadowTexel.y;

        vec2 uv = shadowCoords.xy * texRes;

        vec2 flooredUV = vec2(floor(uv.x), floor(uv.y));

        float s = fract(uv.x);
        float t = fract(uv.y);

        flooredUV *= invRes;

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

        visibility += uw0 * vw0 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u0, v0) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw1 * vw0 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u1, v0) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw2 * vw0 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u2, v0) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw3 * vw0 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u3, v0) * invRes, shadowCoords.z, shadowCoords.w));

        visibility += uw0 * vw1 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u0, v1) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw1 * vw1 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u1, v1) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw2 * vw1 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u2, v1) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw3 * vw1 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u3, v1) * invRes, shadowCoords.z, shadowCoords.w));

        visibility += uw0 * vw2 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u0, v2) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw1 * vw2 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u1, v2) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw2 * vw2 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u2, v2) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw3 * vw2 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u3, v2) * invRes, shadowCoords.z, shadowCoords.w));

        visibility += uw0 * vw3 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u0, v3) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw1 * vw3 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u1, v3) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw2 * vw3 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u2, v3) * invRes, shadowCoords.z, shadowCoords.w));
        visibility += uw3 * vw3 * textureProj(shadowTex, vec4(flooredUV + shadowCoords.w * vec2(u3, v3) * invRes, shadowCoords.z, shadowCoords.w));

        visibility /= 2704.0;
    }

    return visibility;
}

#endif

#if defined(CSM_SHADOW_TEXTURE_ENABLED)

float CalcCSMShadowIntensity(sampler2DArrayShadow shadowTex, vec3 World, vec3 Norm, vec3 LightDirection, vec2 vBaseTexCoord, vec2 shadowTexel, mat4 csmMatrix[4],  vec4 csmDistance)
{
    float distanceFromCamera = length(World - GetCameraViewPos(0));

    // Determine which cascade to use
    int cascadeIndex = CSM_LEVELS - 1; // Default to furthest cascade
    int nextCascadeIndex = -1;
    float cascadeBlendFactor = 0.0;

    for (int i = 0; i < CSM_LEVELS; ++i)
    {
        if (distanceFromCamera < csmDistance[i])
        {
            cascadeIndex = i;

            // Check if we need to blend with next cascade
            if (i < CSM_LEVELS - 1)
            {
                float blendDistance = (csmDistance[i+1] - csmDistance[i]) * 0.1; // 10% blend region
                float distToNextBoundary = csmDistance[i+1] - distanceFromCamera;

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
    vec4 shadowCoords = csmMatrix[cascadeIndex] * vec4(World, 1.0);

    shadowCoords.z += 0.001 * (cascadeIndex * 0.5 + 0.5);

    float texRes = shadowTexel.x;
    float invRes = shadowTexel.y;
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
                visibility += texture(shadowTex, sampleCoord);
                pcfSamples++;
            }
        }

        visibility /= float(pcfSamples);
    }

    // Blend with next cascade if needed for smooth transitions
    if (nextCascadeIndex != -1 && cascadeBlendFactor > 0.0)
    {
        vec4 nextShadowCoords = csmMatrix[nextCascadeIndex] * vec4(World, 1.0);

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
                    nextVisibility += texture(shadowTex, sampleCoord);
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

#if defined(STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED) || defined(DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED)

// Poisson disk sampling pattern for PCF
const vec3 POISSON_DISK_SAMPLES[20] = vec3[](
    vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
    vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 1, 0), vec3(0, -1, 0),
    vec3(0, 0, 1), vec3(0, 0, -1),
    vec3(0.707, 0.707, 0), vec3(-0.707, 0.707, 0),
    vec3(0.707, -0.707, 0), vec3(-0.707, -0.707, 0),
    vec3(0, 0.707, 0.707), vec3(0, -0.707, -0.707)
);

float CubemapShadowCompareDepth(samplerCubeShadow shadowTex, vec3 sampleDir, float compareDepth)
{
    return texture(shadowTex, vec4(sampleDir, compareDepth));
}

// PCF filtering with variable kernel size
float CubemapShadowPCF(samplerCubeShadow shadowTex, vec3 lightToWorldDir, float compareDepth, float filterRadius)
{
    float visibility = 0.0;
    int sampleCount = 20;
    
    for (int i = 0; i < sampleCount; i++)
    {
        vec3 sampleDir = normalize(lightToWorldDir + POISSON_DISK_SAMPLES[i] * filterRadius);
        visibility += CubemapShadowCompareDepth(shadowTex, sampleDir, compareDepth);
    }
    
    return visibility / float(sampleCount);
}

float CalcCubemapShadowIntensity(samplerCubeShadow shadowTex, vec3 World, vec3 LightPos, vec3 Normal, vec3 LightDirection, vec2 shadowTexel)
{
    vec3 lightToWorld = World - LightPos;
    float distanceLightToWorld = length(lightToWorld);
    vec3 lightToWorldDir = lightToWorld / distanceLightToWorld;
    
    // Shadow parameters matching the shadow pass
    float zNear = 0.1;
    float zFar = u_lightradius;
    
    // We store linearDepth in cubeShadowTex
    float linearDepth = distanceLightToWorld / zFar;

    // Improved bias calculation to eliminate wave artifacts
    // Use both normal-based and distance-based bias
    float NdotL = max(dot(Normal, -LightDirection), 0.0);
    
    // Slope-based bias: larger bias for surfaces at grazing angles
    // Use smoother falloff to avoid sudden changes
    float slopeBias = 0.002 * pow(1.0 - NdotL, 2.0);

    float invRes = shadowTexel.y;
    
    // Distance-based bias: account for cubemap texel size at different distances
    // At distance, each texel covers more world space
    float texelWorldSize = distanceLightToWorld * invRes * 2.0;
    float distanceBias = texelWorldSize * 0.5;
    
    // Constant base bias
    float constBias = 0.0003;
    
    // Normal offset bias: push the sample point slightly along the normal
    // Use square root scaling to reduce offset at far distances
    float normalOffsetScale = sqrt(distanceLightToWorld) * invRes * 0.5;
    // Clamp offset to prevent excessive values
    normalOffsetScale = min(normalOffsetScale, 0.1);
    
    vec3 offsetWorld = World + Normal * normalOffsetScale;
    vec3 offsetLightToWorld = offsetWorld - LightPos;
    float offsetDistance = length(offsetLightToWorld);
    vec3 offsetDir = offsetLightToWorld / offsetDistance;
    float offsetLinearDepth = offsetDistance / zFar;
    
    // Combine all bias components
    float bias = constBias + slopeBias + distanceBias;
    // Clamp bias to prevent over-correction
    bias = min(bias, 0.01);
    
    float compareDepth = offsetLinearDepth - bias;
    
    // Clamp depth to valid range
    compareDepth = clamp(compareDepth, 0.0, 1.0);
    
    float visibility = 0.0;

#if defined(PCF_ENABLED)
    // Standard PCF with fixed kernel size
    float filterRadius = invRes * 1.2;
    visibility = CubemapShadowPCF(shadowTex, offsetDir, compareDepth, filterRadius);
#else
    // No filtering, single sample (fastest but lowest quality)
    visibility = CubemapShadowCompareDepth(shadowTex, offsetDir, compareDepth);
#endif
    
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
    return AmbientColor + DiffuseColor + SpecularColor;
}

vec4 CalcPointLight(vec3 World, vec3 Normal, vec2 vBaseTexCoord)
{
    vec3 LightDirection = World - u_lightpos.xyz;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);

    vec4 Color = CalcLightInternal(World, LightDirection, Normal, vBaseTexCoord);

    float r2 = u_lightradius * u_lightradius;
    float Attenuation = clamp(( r2 - (Distance * Distance)) / r2, 0.0, 1.0);

    Color = Color * Attenuation;

    float flStaticShadowIntensity = 1.0;
    float flDynamicShadowIntensity = 1.0;

#if defined(STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED)
    flStaticShadowIntensity = CalcCubemapShadowIntensity(staticCubemapShadowTex, World, u_lightpos.xyz, Normal, LightDirection, u_staticCubemapShadowTexel);
#endif

#if defined(DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED)
    flDynamicShadowIntensity = CalcCubemapShadowIntensity(dynamicCubemapShadowTex, World, u_lightpos.xyz, Normal, LightDirection, u_dynamicCubemapShadowTexel);
#endif

#if defined(STATIC_CUBEMAP_SHADOW_TEXTURE_ENABLED) || defined(DYNAMIC_CUBEMAP_SHADOW_TEXTURE_ENABLED)
    float flShadowIntensity = min(flStaticShadowIntensity, flDynamicShadowIntensity);

    Color = Color * flShadowIntensity;
#endif

    return Color;
}

vec4 CalcSpotLight(vec3 World, vec3 Normal, vec2 vBaseTexCoord)
{
    vec3 LightToPixel = normalize(World - u_lightpos.xyz);
    float SpotCosine = dot(LightToPixel, u_lightdir.xyz);
    float LimitCosine = u_lightcone.x;
    float LimitSine = u_lightcone.y;

    if (SpotCosine > LimitCosine) {

        vec4 Color = CalcPointLight(World, Normal, vBaseTexCoord);

        float flStaticShadowIntensity = 1.0;
        float flDynamicShadowIntensity = 1.0;

        #if defined(STATIC_SHADOW_TEXTURE_ENABLED)

            flStaticShadowIntensity = CalcShadowIntensityLinear(staticShadowTex, World, Normal, u_lightdir.xyz, u_lightpos.xyz, u_lightradius, u_staticShadowTexel, u_staticShadowMatrix);

        #endif

        #if defined(DYNAMIC_SHADOW_TEXTURE_ENABLED)

            flDynamicShadowIntensity = CalcShadowIntensityLinear(dynamicShadowTex, World, Normal, u_lightdir.xyz, u_lightpos.xyz, u_lightradius, u_dynamicShadowTexel, u_dynamicShadowMatrix);

        #endif

#if defined(STATIC_SHADOW_TEXTURE_ENABLED) || defined(DYNAMIC_SHADOW_TEXTURE_ENABLED)
        float flShadowIntensity = min(flStaticShadowIntensity, flDynamicShadowIntensity);

        Color = Color * flShadowIntensity;
#endif

#if defined(CONE_TEXTURE_ENABLED)

        float flConeProjX = dot(u_lightright, LightToPixel);
        float flConeProjY = dot(u_lightup, LightToPixel);

        //map from (-LimitSine, LimitSine) to (0, 1)
        float flConeProjU = (flConeProjX * 1.0 / LimitSine + 1.0) * 0.5;
        float flConeProjV = (flConeProjY * 1.0 / LimitSine + 1.0) * 0.5;

        vec4 vConeColor = texture(coneTex, vec2(flConeProjU, flConeProjV));

        Color = Color * vConeColor;
#else

        float flConeFactor = (SpotCosine - LimitCosine) * (1.0 / LimitSine);

        Color = Color * flConeFactor;

#endif
        return Color;

    }
    else {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
}

vec4 CalcDirectionalLight(vec3 World, vec3 Normal, vec2 vBaseTexCoord)
{
    vec3 LightDirection = u_lightdir.xyz;

    vec4 Color = CalcLightInternal(World, LightDirection, Normal, vBaseTexCoord);

    float flStaticShadowIntensity = 1.0;
    float flCSMShadowIntensity = 1.0;

#if defined(STATIC_SHADOW_TEXTURE_ENABLED)

    flStaticShadowIntensity = CalcShadowIntensity(staticShadowTex, World, Normal, u_lightdir.xyz, u_staticShadowTexel, u_staticShadowMatrix);

#endif

#if defined(CSM_SHADOW_TEXTURE_ENABLED)

    flCSMShadowIntensity = CalcCSMShadowIntensity(csmTex, World, Normal, LightDirection, vBaseTexCoord, u_csmTexel, u_csmMatrices, u_csmDistances);

#endif

#if defined(STATIC_SHADOW_TEXTURE_ENABLED) || defined(CSM_SHADOW_TEXTURE_ENABLED)
    float flShadowIntensity = min(flStaticShadowIntensity, flCSMShadowIntensity);

    Color = Color * flShadowIntensity;
#endif
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