#version 430

#include "common.h"

#extension GL_EXT_texture_array : require

#define GBUFFER_INDEX_DIFFUSE		0.0
#define GBUFFER_INDEX_LIGHTMAP		1.0
#define GBUFFER_INDEX_WORLDNORM		2.0
#define GBUFFER_INDEX_SPECULAR		3.0
#define GBUFFER_INDEX_ADDITIVE		4.0

layout(binding = 0) uniform sampler2DArray gbufferTex;
layout(binding = 1) uniform sampler2D depthTex;

#if defined(TEXTURE_VIEW_AVAILABLE)
layout(binding = 2) uniform usampler2D stencilTex;
#endif

layout(binding = 3) uniform sampler2D linearDepthTex;

uniform float u_ssrRayStep;
uniform int u_ssrIterCount;
uniform float u_ssrDistanceBias;
uniform vec2 u_ssrFade;

in vec2 texCoord;

layout(location = 0) out vec4 out_FragColor;

float random (vec2 uv) {
	return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453123); //simple random function
}

vec4 GenerateBasicColor(vec2 texcoord)
{
    vec4 diffuseColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_DIFFUSE));

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
            vec2 new_texcoord = texcoord + vec2((offset * i) / SceneUBO.viewport.x, (offset * j) / SceneUBO.viewport.y);
            vec4 color = GenerateBasicColor(new_texcoord);
            float weight = gauss[idx++];
            finalColor = finalColor + weight * color;
        }
    }
 
    return finalColor;
}

vec4 GenerateAdditiveColor(vec2 texcoord)
{
    vec4 additiveColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_ADDITIVE));

    vec4 resultColor = additiveColor;
    resultColor.a = 1.0;

    return resultColor;
}

vec3 GenerateViewPositionFromDepth(vec2 texcoord, float depth) {
    vec2 texcoord2 = vec2((texcoord.x - 0.5) * 2.0, (texcoord.y - 0.5) * 2.0);
	vec4 ndc = vec4(texcoord2.xy, depth, 1.0);
	vec4 inversed = SceneUBO.invProjMatrix * ndc;// going back from projected
	inversed /= inversed.w;
	return inversed.xyz;
}

vec2 GenerateProjectedPosition(vec3 pos){
	vec4 samplePosition = SceneUBO.projMatrix * vec4(pos, 1.0);
	samplePosition.xy = (samplePosition.xy / samplePosition.w) * 0.5 + 0.5;
	return samplePosition.xy;
}

vec3 GenerateWorldNormal(vec2 texcoord)
{
    vec4 worldnormColor = texture2DArray(gbufferTex, vec3(texcoord, GBUFFER_INDEX_WORLDNORM));
    vec3 normalworld = OctahedronToUnitVector(worldnormColor.xy);

    return normalworld;
}

vec3 GenerateViewNormal(vec2 texcoord)
{
    return normalize((SceneUBO.viewMatrix * vec4(GenerateWorldNormal(texcoord), 0.0) ).xyz);
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
		depthFromScreen = abs(GenerateViewPositionFromDepth(screenPosition, texture2D(depthTex, screenPosition).x).z);
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
			depthFromScreen = abs(GenerateViewPositionFromDepth(screenPosition, texture2D(depthTex, screenPosition).x).z);
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
    vec3 position = GenerateViewPositionFromDepth(texCoord.xy, texture2D(depthTex, texCoord.xy).x);
    vec3 viewnormal = GenerateViewNormal(texCoord.xy);

    vec3 reflectionDirection = normalize(reflect(position, viewnormal));

    return ScreenSpaceReflectionInternal(position, reflectionDirection);
}

float CalcShadowIntensityLumFadeout(vec4 lightmapColor, float intensity)
{
	float lightmapLum = 0.299 * lightmapColor.x + 0.587 * lightmapColor.y + 0.114 * lightmapColor.z;
	float shadowLerp = (lightmapLum - SceneUBO.shadowFade.w) / (SceneUBO.shadowFade.z - SceneUBO.shadowFade.w + 0.001);
	float shadowIntensity = intensity * clamp(shadowLerp, 0.0, 1.0);
	shadowIntensity *= SceneUBO.shadowColor.a;

	return shadowIntensity;
}

void main()
{
    vec4 diffuseColor = texture2DArray(gbufferTex, vec3(texCoord, GBUFFER_INDEX_DIFFUSE));
    vec4 lightmapColor = texture2DArray(gbufferTex, vec3(texCoord, GBUFFER_INDEX_LIGHTMAP));
	vec4 worldnormColor = texture2DArray(gbufferTex, vec3(texCoord, GBUFFER_INDEX_WORLDNORM));
    vec4 specularColor = texture2DArray(gbufferTex, vec3(texCoord, GBUFFER_INDEX_SPECULAR));

	float shadowIntensity = CalcShadowIntensityLumFadeout(lightmapColor, specularColor.z);
	lightmapColor.xyz *= (1.0 - shadowIntensity);

#ifdef SSR_ENABLED
    if(specularColor.g > 0.0)
    {
        vec4 ssr = ScreenSpaceReflection();

        diffuseColor.xyz = mix(diffuseColor.xyz, ssr.xyz, specularColor.g * ssr.a);
    }
#endif

    vec4 finalColor = diffuseColor * lightmapColor + GenerateAdditiveColor(texCoord);

#if !defined(SKY_FOG_ENABLED) && defined(TEXTURE_VIEW_AVAILABLE)

	uint stencilValue = texture(stencilTex, texCoord).r;

	if(stencilValue == 255)
		out_FragColor = finalColor;
	else
		out_FragColor = CalcFogWithDistance(finalColor, worldnormColor.z);

#else

    out_FragColor = CalcFogWithDistance(finalColor, worldnormColor.z);

#endif
}