#version 460

#include "common.h"

layout (location = 0) uniform ivec2 width_height;
layout (location = 1) uniform vec4 up_down_left_right;
layout (location = 2) uniform vec4 in_color;
layout (location = 3) uniform vec3 in_origin;
layout (location = 4) uniform vec3 in_angles;
layout (location = 5) uniform float in_scale;

layout(binding = 0) uniform sampler2D baseTex;

in vec3 v_worldpos;
in vec3 v_normal;
in vec4 v_color;
in vec2 v_texcoord;

#if defined(OIT_BLEND_ENABLED)

//No color output

#elif defined(GBUFFER_ENABLED)

layout(location = 0) out vec4 out_Diffuse;
layout(location = 1) out vec4 out_Lightmap;
layout(location = 2) out vec4 out_WorldNorm;
layout(location = 3) out vec4 out_Specular;
layout(location = 4) out vec4 out_Additive;

#else

layout(location = 0) out vec4 out_Diffuse;

#endif

void main(void)
{
	vec4 baseColor = texture2D(baseTex, v_texcoord);

	vec3 vNormal = normalize(v_normal.xyz);

#ifdef CLIP_ENABLED
	vec4 clipVec = vec4(v_worldpos.xyz, 1);
	vec4 clipPlane = SceneUBO.clipPlane;
	if(dot(clipVec, clipPlane) < 0)
		discard;

	clipPlane.w += 32.0;
	if(dot(clipVec, clipPlane) < 0 && dot(v_normal.xyz, -clipPlane.xyz) > 0.866)
		discard;
#endif

#ifdef GBUFFER_ENABLED

	vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

	float flDistanceToFragment = distance(v_worldpos.xyz, SceneUBO.viewpos.xyz);

	out_Diffuse = baseColor;
	out_Lightmap = v_color;
	out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
	out_Specular = vec4(0.0);
	out_Additive = vec4(0.0);

#else

	vec4 color = CalcFog(baseColor * v_color);

	#if defined(OIT_ALPHA_BLEND_ENABLED) || defined(OIT_ADDITIVE_BLEND_ENABLED) 
		
		GatherFragment(color);

	#endif

	out_Diffuse = color;	

#endif
}