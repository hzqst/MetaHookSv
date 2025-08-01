#version 430

#include "common.h"

layout(binding = DFINAL_BIND_DIFFUSE_TEXTURE) uniform sampler2D gbufferDiffuse;
layout(binding = DFINAL_BIND_LIGHTMAP_TEXTURE) uniform sampler2D gbufferLightmap;
layout(binding = DFINAL_BIND_WORLDNORM_TEXTURE) uniform sampler2D gbufferWorldNorm;
layout(binding = DFINAL_BIND_SPECULAR_TEXTURE) uniform sampler2D gbufferSpecular;
layout(binding = DFINAL_BIND_DEPTH_TEXTURE) uniform sampler2D depthTex;
layout(binding = DFINAL_BIND_STENCIL_TEXTURE) uniform usampler2D stencilTex;

uniform float u_ssrRayStep;
uniform int u_ssrIterCount;
uniform float u_ssrDistanceBias;
uniform vec2 u_ssrFade;

in vec2 texCoord;

layout(location = 0) out vec4 out_FragColor;

float random(vec2 uv) {
	return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec4 GenerateBasicColor(vec2 texcoord)
{
    vec4 diffuseColor = texture(gbufferDiffuse, texcoord);

    vec4 resultColor = diffuseColor;
    resultColor.a = 1.0;

    return resultColor;
}

const float gauss[] = {
    0.00000067, 0.00002292, 0.00019117, 0.00038771, 0.00019117, 0.00002292, 0.00000067,
    0.00002292, 0.00078633, 0.00655965, 0.01330373, 0.00655965, 0.00078633, 0.00002292,
    0.00019117, 0.00655965, 0.05472157, 0.11098164, 0.05472157, 0.00655965, 0.00019117,
    0.00038771, 0.01330373, 0.11098164, 0.22508352, 0.11098164, 0.01330373, 0.00038771,
    0.00019117, 0.00655965, 0.05472157, 0.11098164, 0.05472157, 0.00655965, 0.00019117,
    0.00002292, 0.00078633, 0.00655965, 0.01330373, 0.00655965, 0.00078633, 0.00002292,
    0.00000067, 0.00002292, 0.00019117, 0.00038771, 0.00019117, 0.00002292, 0.00000067
};

vec4 GenerateBasicColorBlur(vec2 texcoord, float offset)
{
	vec4 finalColor = vec4(0);
 
    int idx = 0;
    for(int i = -3;i <= 3;i++)
    {
        for(int j = -3; j <= 3;j++)
        {
            vec2 new_texcoord = texcoord + vec2((offset * i) / CameraUBO.viewport.x, (offset * j) / CameraUBO.viewport.y);
            vec4 color = GenerateBasicColor(new_texcoord);
            float weight = gauss[idx++];
            finalColor = finalColor + weight * color;
        }
    }
 
    return finalColor;
}

vec3 GenerateWorldNormal(vec2 texcoord)
{
    vec4 worldnormColor = texture(gbufferWorldNorm, texcoord);
    vec3 normalworld = OctahedronToUnitVector(worldnormColor.xy);

    return normalworld;
}

vec3 GenerateViewNormal(vec2 texcoord)
{
    return normalize((CameraUBO.viewMatrix * vec4(GenerateWorldNormal(texcoord), 0.0) ).xyz);
}

vec4 VignetteColor(vec4 c, vec2 win_bias)
{
    // convert window coord to [-1, 1] range
    vec2 wpos = 2.0*(win_bias - vec2(0.5, 0.5)); 

    // calculate distance from origin
    float r = length(wpos);
    r = 1.0 - smoothstep(u_ssrFade.x, u_ssrFade.y, r);
	
    c.a *= r;

    return c;
}

vec4 ScreenSpaceReflectionInternal(vec3 position, vec3 reflection)
{
	vec3 step = u_ssrRayStep * reflection;
	vec3 marchingPosition = position + step;
	float delta;
	float depthFromScreen;
	vec2 screenPosition;

    int i = 0;
	for (; i < u_ssrIterCount; i++) {
		screenPosition = GenerateProjectedPosition(marchingPosition);
		depthFromScreen = abs(GenerateViewPositionFromDepth(screenPosition, texture(depthTex, screenPosition).x).z);
		delta = abs(marchingPosition.z) - depthFromScreen;
		if (abs(delta) < u_ssrDistanceBias) {
            if(screenPosition.x < 0.0 || screenPosition.x > 1.0 || screenPosition.y < 0.0 || screenPosition.y > 1.0){
                return vec4(0.0);
            }

			return VignetteColor(GenerateBasicColorBlur(screenPosition, 5.0), screenPosition);
		}
        #ifdef SSR_BINARY_SEARCH_ENABLED
		if (delta > 0.0) {
			break;
		}
        #endif
		#ifdef SSR_ADAPTIVE_STEP_ENABLED
			float directionSign = sign(abs(marchingPosition.z) - depthFromScreen);
			//this is sort of adapting step, should prevent lining reflection by doing sort of iterative converging
			//some implementation doing it by binary search, but I found this idea more cheaty and way easier to implement
			step = step * (1.0 - u_ssrRayStep * max(directionSign, 0.0));
			marchingPosition += step * (-directionSign);
		#else
			marchingPosition += step;
		#endif
		#ifdef SSR_EXPONENTIAL_STEP_ENABLED
			step *= 1.05;
		#endif
    }
	#ifdef SSR_BINARY_SEARCH_ENABLED
		for(; i < u_ssrIterCount; i++){
			
			step *= 0.5;
			marchingPosition = marchingPosition - step * sign(delta);
			
			screenPosition = GenerateProjectedPosition(marchingPosition);
			depthFromScreen = abs(GenerateViewPositionFromDepth(screenPosition, texture(depthTex, screenPosition).x).z);
			delta = abs(marchingPosition.z) - depthFromScreen;
			
			if (abs(delta) < u_ssrDistanceBias) {
                if(screenPosition.x < 0.0 || screenPosition.x > 1.0 || screenPosition.y < 0.0 || screenPosition.y > 1.0){
                    return vec4(0.0);
                }

				return VignetteColor(GenerateBasicColorBlur(screenPosition, 5.0), screenPosition);
			}
		}
	#endif
	
    return vec4(0.0);
}

vec4 ScreenSpaceReflection()
{
    vec3 position = GenerateViewPositionFromDepth(texCoord.xy, texture(depthTex, texCoord.xy).x);
    vec3 viewnormal = GenerateViewNormal(texCoord.xy);

    vec3 reflectionDirection = normalize(reflect(position, viewnormal));

    return ScreenSpaceReflectionInternal(position, reflectionDirection);
}

float CalcShadowIntensityLumFadeout(vec4 lightmapColor, float intensity, uint stencilValue)
{
	if((stencilValue & STENCIL_MASK_NO_SHADOW) == STENCIL_MASK_NO_SHADOW)
		return 0;

	float lightmapLum = 0.299 * lightmapColor.x + 0.587 * lightmapColor.y + 0.114 * lightmapColor.z;
	float shadowLerp = (lightmapLum - SceneUBO.shadowFade.w) / (SceneUBO.shadowFade.z - SceneUBO.shadowFade.w + 0.001);
	float shadowIntensity = intensity * clamp(shadowLerp, 0.0, 1.0);
	shadowIntensity *= SceneUBO.shadowColor.a;

	return shadowIntensity;
}

void main()
{
    vec4 diffuseColor = texture(gbufferDiffuse, texCoord);
    vec4 lightmapColor = texture(gbufferLightmap, texCoord);
	vec4 worldnormColor = texture(gbufferWorldNorm, texCoord);
    vec4 specularColor = texture(gbufferSpecular, texCoord);
	uint stencilValue = texture(stencilTex, texCoord).r;

	float shadowIntensity = CalcShadowIntensityLumFadeout(lightmapColor, specularColor.z, stencilValue);
	lightmapColor.xyz *= (1.0 - shadowIntensity);

#if defined(SSR_ENABLED)
    if(specularColor.g > 0.0)
    {
        vec4 ssr = ScreenSpaceReflection();

        diffuseColor.xyz = mix(diffuseColor.xyz, ssr.xyz, specularColor.g * ssr.a);
    }
#endif

    vec4 finalColor = diffuseColor * lightmapColor;

    float flDistanceToFragment = worldnormColor.z;

#if !defined(SKY_FOG_ENABLED)

	if((stencilValue & STENCIL_MASK_HAS_FOG) == 0)
		out_FragColor = finalColor;
	else
		out_FragColor = CalcFog(finalColor, flDistanceToFragment);

#else

    out_FragColor = CalcFog(finalColor, flDistanceToFragment);

#endif
}