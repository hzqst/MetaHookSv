#version 460

#include "common.h"

#ifndef BINDLESS_ENABLED
layout(binding = 0) uniform sampler2D baseTex;
#endif

in vec3 v_worldpos;
in vec3 v_normal;
in vec4 v_color;
in vec2 v_texcoord;
flat in int v_frameindex;

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
	#ifdef BINDLESS_ENABLED
		sampler2D baseTex = sampler2D(SpriteFrameSSBO.frames[v_frameindex].texturehandle);
	#endif

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

	#ifdef OIT_BLEND_ENABLED

		vec4 color = baseColor * v_color;
		
		GatherFragment(color);

	#else

		out_Diffuse = baseColor * v_color;

		#ifdef LINEAR_FOG_ENABLED

			float z = gl_FragCoord.z / gl_FragCoord.w;
			float fogFactor = ( SceneUBO.fogEnd - z ) / ( SceneUBO.fogEnd - SceneUBO.fogStart );
			fogFactor = clamp(fogFactor, 0.0, 1.0);

			vec3 finalColor = out_Diffuse.xyz;

			out_Diffuse.xyz = mix(SceneUBO.fogColor.xyz, finalColor, fogFactor );

		#endif

	#endif

#endif
}