#version 430

uniform vec4 u_watercolor;
uniform vec2 u_depthfactor;
uniform float u_fresnelfactor;
uniform float u_normfactor;

layout(binding = 0) uniform sampler2D normalTex;
layout(binding = 1) uniform sampler2D refractTex;
layout(binding = 2) uniform sampler2D reflectTex;
layout(binding = 3) uniform sampler2D depthTex;

struct scene_ubo_t{
	mat4 viewMatrix;
	mat4 projMatrix;
	mat4 invViewMatrix;
	mat4 invProjMatrix;
	mat4 shadowMatrix[3];
	vec4 viewpos;
	vec4 fogColor;
	float fogStart;
	float fogEnd;
	float time;
	float padding;
};

struct entity_ubo_t{
	mat4 entityMatrix;
	float scrollSpeed;
	float padding[3];
};

layout (std140, binding = 0) uniform SceneBlock
{
   scene_ubo_t SceneUBO;
};

layout (std140, binding = 1) uniform EntityBlock
{
   entity_ubo_t EntityUBO;
};

in vec4 v_projpos;
in vec3 v_worldpos;
in vec3 v_normal;
in vec2 v_diffusetexcoord;

layout(location = 0) out vec4 out_Diffuse;
layout(location = 1) out vec4 out_Lightmap;
layout(location = 2) out vec4 out_WorldNorm;
layout(location = 3) out vec4 out_Specular;
layout(location = 4) out vec4 out_Additive;

vec3 GenerateWorldPositionFromDepth(vec2 texCoord)
{
	vec4 clipSpaceLocation;	
	clipSpaceLocation.xy = texCoord * 2.0-1.0;
	clipSpaceLocation.z  = texture2D(depthTex, texCoord).x * 2.0-1.0;
	clipSpaceLocation.w  = 1.0;
	vec4 homogenousLocation = SceneUBO.invViewMatrix * SceneUBO.invProjMatrix * clipSpaceLocation;
	return homogenousLocation.xyz / homogenousLocation.w;
}

void main()
{
	float flWaterColorAlpha = clamp(u_watercolor.a, 0.01, 0.99);
	vec4 vWaterColor = vec4(u_watercolor.xyz, 1.0);

	//calculate the normal texcoord and sample the normal vector from texture
	vec2 vNormTexCoord1 = vec2(0.2, 0.15) * SceneUBO.time + v_diffusetexcoord.xy; 
	vec2 vNormTexCoord2 = vec2(-0.13, 0.11) * SceneUBO.time + v_diffusetexcoord.xy;
	vec2 vNormTexCoord3 = vec2(-0.14, -0.16) * SceneUBO.time + v_diffusetexcoord.xy;
	vec2 vNormTexCoord4 = vec2(0.17, 0.15) * SceneUBO.time + v_diffusetexcoord.xy;
	vec4 vNorm1 = texture2D(normalTex, vNormTexCoord1);
	vec4 vNorm2 = texture2D(normalTex, vNormTexCoord2);
	vec4 vNorm3 = texture2D(normalTex, vNormTexCoord3);
	vec4 vNorm4 = texture2D(normalTex, vNormTexCoord4);
	vec4 vNormal = vNorm1 + vNorm2 + vNorm3 + vNorm4;
	vNormal = (vNormal * 0.25) * 2.0 - 1.0;

	//calculate texcoord
	vec2 vBaseTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;

	vec3 vEyeVect = SceneUBO.viewpos.xyz - v_worldpos.xyz;
	float dist = length(vEyeVect);
	float sinX = abs(vEyeVect.z) / (dist + 0.001);
	float flFresnel = asin(sinX) / (0.5 * 3.14159);
	
	float flOffsetFactor = clamp(flFresnel, 0.0, 1.0) * u_normfactor;

	flFresnel = 1.0 - flFresnel;

	vec2 vOffsetTexCoord = normalize(vNormal.xyz).xy * flOffsetFactor;

#ifdef REFRACT_ENABLED

	vec2 vRefractTexCoord = vBaseTexCoord + vOffsetTexCoord;
	vec4 vRefractColor = texture2D(refractTex, vRefractTexCoord);
	vRefractColor.a = 1.0;

	vRefractColor = mix(vRefractColor, vWaterColor, flWaterColorAlpha);

#else

	vec4 vRefractColor = vWaterColor;

#endif

#ifdef UNDERWATER_ENABLED

	vec4 vFinalColor = vRefractColor;

	#ifdef GBUFFER_ENABLED

		vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

		float flDistanceToFragment = distance(v_worldpos.xyz, SceneUBO.viewpos.xyz);

		vFinalColor.a = 1.0;
	    out_Diffuse = vFinalColor;
		out_Lightmap = vec4(1.0, 1.0, 1.0, 1.0);
		out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
		out_Specular = vec4(0.0);
		out_Additive = vec4(0.0);
	#else

		out_Diffuse = vFinalColor;

	#endif

#else

	#ifdef DEPTH_ENABLED

		vec3 worldScene = GenerateWorldPositionFromDepth(vBaseTexCoord);
		float flDiffZ = v_worldpos.z - worldScene.z;
		float flWaterBlendAlpha = clamp( clamp( u_depthfactor.x * flDiffZ, 0.0, 1.0 ) + u_depthfactor.y, 0.0, 1.0 );

	#else

		float flWaterBlendAlpha = 1.0;

	#endif

		//Sample the reflect color (texcoord inverted)
		vec2 vBaseTexCoord2 = vec2(v_projpos.x, -v_projpos.y) / v_projpos.w * 0.5 + 0.5;

		vec2 vReflectTexCoord = vBaseTexCoord2 + vOffsetTexCoord;
		vec4 vReflectColor = texture2D(reflectTex, vReflectTexCoord);
		vReflectColor.a = 1.0;

		if(vReflectColor.x == u_watercolor.x && vReflectColor.y == u_watercolor.y && vReflectColor.z == u_watercolor.z)
		{
			vReflectColor = u_watercolor;
		}

		float flRefractFactor = clamp(flFresnel * u_fresnelfactor, 0.0, 1.0);

		vec4 vFinalColor = vRefractColor + vReflectColor * flRefractFactor;

		vFinalColor.a = flWaterBlendAlpha;

		//todo, blend with fog ?
		float flFogFactor = 0.0;//CalcPixelFogFactor( PIXELFOGTYPE, g_PixelFogParams, g_EyePos, i.worldPos, i.vProjPos.z );

	#ifdef GBUFFER_ENABLED

		vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

		float flDistanceToFragment = distance(v_worldpos.xyz, SceneUBO.viewpos.xyz);

		vFinalColor.a = 1.0;
	    out_Diffuse = vFinalColor;
		out_Lightmap = vec4(1.0, 1.0, 1.0, 1.0);
		out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
		out_Specular = vec4(0.0);
		out_Additive = vec4(0.0);
	#else
		out_Diffuse = vFinalColor;
	#endif

#endif
}