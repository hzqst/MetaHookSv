#version 430

#include "common.h"

uniform vec4 u_watercolor;
uniform vec3 u_depthfactor;
uniform vec4 u_fresnelfactor;
uniform float u_normfactor;
uniform float u_scale;
uniform float u_speed;

layout(binding = WATER_BIND_BASE_TEXTURE) uniform sampler2D baseTex;
layout(binding = WATER_BIND_NORMAL_TEXTURE) uniform sampler2D normalTex;
layout(binding = WATER_BIND_REFRACT_TEXTURE) uniform sampler2D refractTex;
layout(binding = WATER_BIND_REFRACT_DEPTH_TEXTURE) uniform sampler2D refractDepthTex;
layout(binding = WATER_BIND_REFLECT_TEXTURE) uniform sampler2D reflectTex;
layout(binding = WATER_BIND_REFLECT_DEPTH_TEXTURE) uniform sampler2D reflectDepthTex;

in vec4 v_projpos;
in vec3 v_worldpos;
in vec3 v_normal;
in vec2 v_diffusetexcoord;

layout(location = 0) out vec4 out_Diffuse;

#if defined(GBUFFER_ENABLED)
layout(location = 1) out vec4 out_Lightmap;
layout(location = 2) out vec4 out_WorldNorm;
layout(location = 3) out vec4 out_Specular;
#endif

void main()
{
	vec3 vNormal = normalize(v_normal);

	ClipPlaneTest(v_worldpos.xyz, vNormal.xyz);

	vec4 vFinalColor = vec4(0.0);
	
	float flWaterColorAlpha = clamp(u_watercolor.a, 0.0, 1.0);
	vec4 vWaterColor = vec4(u_watercolor.xyz, 1.0);

	vWaterColor = ProcessOtherGammaColor(vWaterColor);

#if defined(LEGACY_ENABLED)

	vFinalColor.xyz = texture(baseTex, v_diffusetexcoord.xy).xyz;
	//vFinalColor.xyz = vec3(1, 1, 1);//test
	vFinalColor.a = flWaterColorAlpha;

	//The basetexture of water is in TexGamme Space and will need to convert to Linear Space
	vFinalColor = ProcessDiffuseColor(vFinalColor);

#else

	//calculate the normal texcoord and sample the normal vector from texture
	vec2 vNormTexCoord1 = vec2(0.2, 0.15) * SceneUBO.cl_time + v_diffusetexcoord.xy; 
	vec2 vNormTexCoord2 = vec2(-0.13, 0.11) * SceneUBO.cl_time + v_diffusetexcoord.xy;
	vec2 vNormTexCoord3 = vec2(-0.14, -0.16) * SceneUBO.cl_time + v_diffusetexcoord.xy;
	vec2 vNormTexCoord4 = vec2(0.17, 0.15) * SceneUBO.cl_time + v_diffusetexcoord.xy;
	vec4 vNorm1 = texture(normalTex, vNormTexCoord1);
	vec4 vNorm2 = texture(normalTex, vNormTexCoord2);
	vec4 vNorm3 = texture(normalTex, vNormTexCoord3);
	vec4 vNorm4 = texture(normalTex, vNormTexCoord4);
	vNormal = vNorm1.xyz + vNorm2.xyz + vNorm3.xyz + vNorm4.xyz;
	vNormal = (vNormal * 0.25) * 2.0 - 1.0;

	//calculate texcoord
	vec2 vBaseTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;

	vec3 vEyeVect = CameraUBO.viewpos.xyz - v_worldpos.xyz;
	float flHeight = abs(vEyeVect.z);
	float dist = length(vEyeVect);
	float sinX = flHeight / (dist + 0.001);
	float flFresnel = asin(sinX) / (0.5 * 3.14159);
	
	float flOffsetFactor = clamp(flFresnel, 0.0, 1.0) * u_normfactor;

	flFresnel = 1.0 - flFresnel;

	//flFresnel = clamp(flFresnel + smoothstep(u_fresnelfactor.x, u_fresnelfactor.y, flHeight), 0.0, 1.0);

	vec2 vOffsetTexCoord = normalize(vNormal).xy * flOffsetFactor;

	#if defined(REFRACT_ENABLED)

		vec2 vRefractTexCoord = vBaseTexCoord + vOffsetTexCoord;
		vec4 vRefractColor = texture(refractTex, vRefractTexCoord);
		vRefractColor.a = 1.0;
		vRefractColor = ProcessOtherLinearColor(vRefractColor);

		vRefractColor = mix(vRefractColor, vWaterColor, flWaterColorAlpha);

	#else

		vec4 vRefractColor = vWaterColor;

	#endif

	#if defined(UNDERWATER_ENABLED)

		vFinalColor = vRefractColor;

	#else

		#if defined(DEPTH_ENABLED)

			float sceneDepthValue = texture(refractDepthTex, vBaseTexCoord).x;
			vec3 sceneWorldPos = GenerateWorldPositionFromDepth(vBaseTexCoord, sceneDepthValue);

			float flDistanceBetweenWaterSurfaceAndScene = distance(sceneWorldPos.xyz, CameraUBO.viewpos.xyz) - distance(v_worldpos.xyz, CameraUBO.viewpos.xyz);
			float flRefractEdgeFeathering = clamp(flDistanceBetweenWaterSurfaceAndScene / u_depthfactor.z, 0.0, 1.0);
			vOffsetTexCoord *= (flRefractEdgeFeathering * flRefractEdgeFeathering);

		#endif

		float flWaterBlendAlpha = 1.0;

		#if defined(DEPTH_ENABLED) && defined(REFRACT_ENABLED)

			float flDiffZ = v_worldpos.z - sceneWorldPos.z;

			flWaterBlendAlpha = clamp( clamp( u_depthfactor.x * flDiffZ, 0.0, 1.0 ) + u_depthfactor.y, 0.0, 1.0 );

		#endif

		//Sample the reflect color (texcoord inverted)
		vec2 vBaseTexCoord2 = vec2(v_projpos.x, -v_projpos.y) / v_projpos.w * 0.5 + 0.5;

		vec2 vReflectTexCoord = vBaseTexCoord2 + vOffsetTexCoord;
		vec4 vReflectColor = texture(reflectTex, vReflectTexCoord);
		vReflectColor.a = 1.0;
		vReflectColor = ProcessOtherLinearColor(vReflectColor);

		float flReflectFactor = clamp(pow(flFresnel, u_fresnelfactor.z), 0.0, u_fresnelfactor.w);
		
		#if defined(DEPTH_ENABLED)

			float reflectSceneDepthValue = texture(reflectDepthTex, vReflectTexCoord).x;

			if(reflectSceneDepthValue == 1)
				flReflectFactor = 0;

		#endif

		vFinalColor = mix(vRefractColor, vReflectColor, flReflectFactor);

		vFinalColor.a = flWaterBlendAlpha;

	#endif

#endif

	float flDistanceToFragment = distance(v_worldpos.xyz, CameraUBO.viewpos.xyz);

#if defined(GBUFFER_ENABLED)

	vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

	out_Diffuse = vFinalColor;
	out_Lightmap = vec4(1.0, 1.0, 1.0, 1.0);
	out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
	out_Specular = vec4(0.0);

#else

	vec4 color = CalcFog(
		ProcessLinearBlendShift(vFinalColor),
		flDistanceToFragment
	);

	GatherFragment(color);

	out_Diffuse = color;

#endif
}