#version 430 core

#include "common.h"

layout(binding = STUDIO_BIND_TEXTURE_DIFFUSE) uniform sampler2D diffuseTex;
layout(binding = STUDIO_BIND_TEXTURE_NORMAL) uniform sampler2D normalTex;
layout(binding = STUDIO_BIND_TEXTURE_PARALLAX) uniform sampler2D parallaxTex;
layout(binding = STUDIO_BIND_TEXTURE_SPECULAR) uniform sampler2D specularTex;
layout(binding = STUDIO_BIND_TEXTURE_ANIMATED) uniform sampler2DArray animatedTexArray;
layout(binding = STUDIO_BIND_TEXTURE_STENCIL) uniform usampler2D stencilTex;
layout(binding = STUDIO_BIND_TEXTURE_MIX_DIFFUSE) uniform sampler2D mixDiffuseTex;

/* celshade */

uniform vec2 r_base_specular;
uniform vec4 r_celshade_specular;
uniform float r_celshade_midpoint;
uniform float r_celshade_softness;
uniform vec3 r_celshade_shadow_color;
uniform vec3 r_celshade_head_offset;
uniform vec2 r_celshade_lightdir_adjust;
uniform float r_rimlight_power;
uniform float r_rimlight_smooth;
uniform vec2 r_rimlight_smooth2;
uniform vec3 r_rimlight_color;
uniform float r_rimdark_power;
uniform float r_rimdark_smooth;
uniform vec2 r_rimdark_smooth2;
uniform vec3 r_rimdark_color;
uniform float r_hair_specular_exp;
uniform float r_hair_specular_exp2;
uniform vec3 r_hair_specular_intensity;
uniform vec3 r_hair_specular_intensity2;
uniform vec4 r_hair_specular_noise;
uniform vec4 r_hair_specular_noise2;
uniform vec2 r_hair_specular_smooth;
uniform float r_outline_dark;
uniform vec2 r_uvscale;
uniform float r_packed_stride;
uniform vec4 r_packed_index;
uniform vec2 r_framerate_numframes;
uniform vec2 r_nearplaneclip;

// Input from geometry shader (if enabled) or vertex shader (if not)
#if defined(MULTIVIEW_ENABLED)
	// When geometry shader is enabled, input comes from geometry shader with g_ prefix
	#define v_worldpos g_worldpos
	#define v_normal g_normal
	#define v_texcoord g_texcoord
	#define v_projpos g_projpos
	#define v_packedbone g_packedbone
	#define v_tangent g_tangent
	#define v_bitangent g_bitangent
	#define v_smoothnormal g_smoothnormal
	#define v_headfwd g_headfwd
	#define v_headup g_headup
	#define v_headorigin g_headorigin
	
	in vec3 g_worldpos;
	in vec3 g_normal;
	in vec2 g_texcoord;
	in vec4 g_projpos;
	flat in uint g_packedbone;
	in vec3 g_tangent;
	in vec3 g_bitangent;
	in vec3 g_smoothnormal;
	
	#if defined(STUDIO_NF_CELSHADE_FACE)
		in vec3 g_headfwd;
		in vec3 g_headup;
		in vec3 g_headorigin;
	#endif
#else
	// Standard path: input directly from vertex shader
	in vec3 v_worldpos;
	in vec3 v_normal;
	in vec2 v_texcoord;
	in vec4 v_projpos;
	flat in uint v_packedbone;
	in vec3 v_tangent;
	in vec3 v_bitangent;
	in vec3 v_smoothnormal;
	
	#if defined(STUDIO_NF_CELSHADE_FACE)
		in vec3 v_headfwd;
		in vec3 v_headup;
		in vec3 v_headorigin;
	#endif
#endif

layout(location = 0) out vec4 out_Diffuse;

#if defined(GBUFFER_ENABLED)
layout(location = 1) out vec4 out_Lightmap;
layout(location = 2) out vec4 out_WorldNorm;
layout(location = 3) out vec4 out_Specular;
#endif

#if defined(NORMALTEXTURE_ENABLED) || defined(PACKED_DIFFUSETEXTURE_ENABLED)

mat3 GenerateTBNMatrix(vec3 tangent, vec3 bitangent, vec3 normal)
{
   // Construct a tangent-bitangent-normal matrix
    return mat3(tangent, bitangent, normal);
}

vec4 SampleNormalTexture(vec2 baseTexcoord)
{
	#if defined(NORMALTEXTURE_ENABLED)

		vec4 normColor = texture(normalTex, baseTexcoord);

	#elif defined(PACKED_NORMALTEXTURE_ENABLED)

		vec4 normColor = texture(diffuseTex, vec2(baseTexcoord.x + r_packed_stride * r_packed_index.y, baseTexcoord.y));

	#endif

	return normColor;
}

vec3 NormalMapping(mat3 TBN, vec2 baseTexcoord)
{
    // Sample tangent space normal vector from normal map and remap it from [0, 1] to [-1, 1] range.
    vec3 n = SampleNormalTexture(baseTexcoord).xyz;

    n = normalize(n * 2.0 - 1.0);

    // Multiple normal by the TBN matrix to transform the normal from tangent space to world space.
    n = normalize(TBN * n);

    return n;
}

#endif

vec3 R_GetAdjustedLightDirection(vec3 vecLightDirection)
{
	vec3 vecAdjustedLightDirection = vecLightDirection;

#if defined(STUDIO_NF_CELSHADE_FACE)

		vecAdjustedLightDirection.z *= r_celshade_lightdir_adjust.y;

#elif defined(STUDIO_NF_CELSHADE)

		vecAdjustedLightDirection.z *= r_celshade_lightdir_adjust.x;

#endif

	return normalize(vecAdjustedLightDirection);
}

float R_StudioBaseLight_PhongSpecular(vec3 vWorldPos, vec3 vNormal, float specularMask)
{
	float illum = 0.0;

	vec3 vecVertexToEye = normalize(GetCameraViewPos(0) - vWorldPos.xyz);
	vec3 vecAdjustedLight = R_GetAdjustedLightDirection(StudioUBO.r_plightvec.xyz);
	vec3 vecLightReflect = normalize(reflect(vecAdjustedLight, vNormal.xyz));
	float flSpecularFactor = dot(vecVertexToEye, vecLightReflect);

	flSpecularFactor = clamp(flSpecularFactor, 0.0, 1.0);
	flSpecularFactor = pow(flSpecularFactor, r_base_specular.y) * r_base_specular.x;

	illum += StudioUBO.r_shadelight * flSpecularFactor * specularMask;

	return illum;
}

float GetSteppedValue(float low, float high, float value) {
    return mix(mix(0.0, value, step(low, value)), 1.0, step(high, value)) * value;
}

float R_StudioBaseLight_CelShadeSpecular(vec3 vWorldPos, vec3 vNormal, float specularMask)
{
	float illum = 0.0;

	vec3 vecVertexToEye = normalize(GetCameraViewPos(0) - vWorldPos.xyz);
	vec3 vecAdjustedLight = R_GetAdjustedLightDirection(StudioUBO.r_plightvec.xyz);
	vec3 vecLightReflect = normalize(reflect(vecAdjustedLight, vNormal.xyz));
	float flSpecularFactor = dot(vecVertexToEye, vecLightReflect);

	flSpecularFactor = clamp(flSpecularFactor, 0.0, 1.0);
	flSpecularFactor = pow(flSpecularFactor, r_celshade_specular.y) * r_celshade_specular.x;

	flSpecularFactor = GetSteppedValue(r_celshade_specular.z, r_celshade_specular.w, flSpecularFactor);
	
	illum += StudioUBO.r_shadelight * flSpecularFactor * specularMask;

	return illum;
}

float R_StudioBaseLight_FlatShading(vec3 vWorldPos, vec3 vNormal, float specularMask)
{
	float illum = 0.0;

	illum += StudioUBO.r_shadelight * 0.8;

	//Layer 1 Specular
	#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

		illum += R_StudioBaseLight_PhongSpecular(vWorldPos, vNormal, specularMask);

	#endif

	//Layer 2 Specular
	#if (defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)) && defined(STUDIO_NF_CELSHADE)

		illum += R_StudioBaseLight_CelShadeSpecular(vWorldPos, vNormal, specularMask);

	#endif

	return illum;
}

float R_StudioBaseLight_PhongShading(vec3 vWorldPos, vec3 vNormal, float specularMask)
{
	float illum = 0.0;
	float lightcos = dot(vNormal.xyz, StudioUBO.r_plightvec.xyz);

	if(SceneUBO.v_lambert < 1.0)
	{
		lightcos = (SceneUBO.v_lambert - lightcos) / (SceneUBO.v_lambert + 1.0); 
		illum += StudioUBO.r_shadelight * max(lightcos, 0.0); 			
	}
	else
	{
		illum += StudioUBO.r_shadelight;
		lightcos = (lightcos + SceneUBO.v_lambert - 1.0) / SceneUBO.v_lambert;
		illum -= StudioUBO.r_shadelight * max(lightcos, 0.0);
	}

	//Layer 1 Specular
	#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

		illum += R_StudioBaseLight_PhongSpecular(vWorldPos, vNormal, specularMask);

	#endif

	return illum;
}

vec3 R_StudioEntityLight_PhongSpecular(int i, vec3 vElightDirection, vec3 vWorldPos, vec3 vNormal, float specularMask, float ElightAttenuation)
{
	vec3 color = vec3(0.0, 0.0, 0.0);

	vec3 vecVertexToEye = normalize(GetCameraViewPos(0) - vWorldPos.xyz);
	vec3 vecLightReflect = normalize(reflect(vElightDirection, vNormal.xyz));
	
	float flSpecularFactor = dot(vecVertexToEye, vecLightReflect);
	flSpecularFactor = clamp(flSpecularFactor, 0.0, 1.0);
	flSpecularFactor = pow(flSpecularFactor, r_base_specular.y) * r_base_specular.x;

	vec3 ElightColor = ProcessOtherGammaColor3(StudioUBO.r_elight_color[i].xyz);
	
	color += ElightColor * flSpecularFactor * ElightAttenuation * specularMask;

	return color;
}

vec3 R_StudioEntityLight_FlatShading(int i, vec3 vWorldPos, vec3 vNormal, float specularMask)
{
	vec3 color = vec3(0.0, 0.0, 0.0);
	
	vec3 ElightDirection = StudioUBO.r_elight_origin_radius[i].xyz - vWorldPos.xyz;

	float ElightDot = dot(ElightDirection, ElightDirection);

	float r2 = StudioUBO.r_elight_origin_radius[i].w;

	r2 = r2 * r2;

    float MinStrength = 1.0;

    if (ElightDot > r2)
    {
        MinStrength = r2 / ElightDot;
    }
    
	float ElightCosine = 0.8;

	//if (MinStrength > 0.004 && ElightCosine > 0)
	if (ElightCosine > 0)
	{
		float ElightAttenuation = clamp(r2 / (ElightDot * sqrt(ElightDot)), 0.0, 1.0) * ElightCosine;

		vec3 ElightColor = ProcessOtherGammaColor3(StudioUBO.r_elight_color[i].xyz);
		
		color += ElightColor * ElightAttenuation;

		#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

			color += R_StudioEntityLight_PhongSpecular(i, normalize(ElightDirection), vWorldPos, vNormal, specularMask, ElightAttenuation);
		
		#endif
	}

	return color;
}

vec3 R_StudioEntityLight_PhongShading(int i, vec3 vWorldPos, vec3 vNormal, float specularMask)
{
	vec3 color = vec3(0.0, 0.0, 0.0);

	vec3 ElightDirection = StudioUBO.r_elight_origin_radius[i].xyz - vWorldPos.xyz;

	float ElightDot = dot(ElightDirection, ElightDirection);

	float r2 = StudioUBO.r_elight_origin_radius[i].w;
	
	r2 = r2 * r2;
	
    float MinStrength = 1.0;

    if (ElightDot > r2)
    {
        MinStrength = r2 / ElightDot;
    }
    
	float ElightCosine = dot(vNormal, normalize(ElightDirection));

	//if (MinStrength > 0.004 && ElightCosine > 0)
	if (ElightCosine > 0)
	{
		float ElightAttenuation = clamp(r2 / (ElightDot * sqrt(ElightDot)), 0.0, 1.0) * ElightCosine;

		vec3 ElightColor = ProcessOtherGammaColor3(StudioUBO.r_elight_color[i].xyz);
		
		color += ElightColor * ElightAttenuation;

		#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

			color += R_StudioEntityLight_PhongSpecular(i, normalize(ElightDirection), vWorldPos, vNormal, specularMask, ElightAttenuation);
		
		#endif
	}

	return color;
}

vec3 R_StudioDynamicLight_PhongSpecular(int i, vec3 vElightDirection, vec3 vWorldPos, vec3 vNormal, float specularMask, float DlightAttenuation)
{
	vec3 color = vec3(0.0, 0.0, 0.0);

	vec3 vecVertexToEye = normalize(GetCameraViewPos(0) - vWorldPos.xyz);
	vec3 vecLightReflect = normalize(reflect(vElightDirection, vNormal.xyz));
	
	float flSpecularFactor = dot(vecVertexToEye, vecLightReflect);
	flSpecularFactor = clamp(flSpecularFactor, 0.0, 1.0);
	flSpecularFactor = pow(flSpecularFactor, r_base_specular.y) * r_base_specular.x;

	color.x += DLightUBO.color_minlight[i].x * flSpecularFactor * DlightAttenuation * specularMask;
	color.y += DLightUBO.color_minlight[i].y * flSpecularFactor * DlightAttenuation * specularMask;
	color.z += DLightUBO.color_minlight[i].z * flSpecularFactor * DlightAttenuation * specularMask;

	return color;
}

vec3 R_StudioDynamicLight_FlatShading(int i, vec3 vWorldPos, vec3 vNormal, float specularMask)
{
	vec3 color = vec3(0.0, 0.0, 0.0);
	
	vec3 DlightDirection = DLightUBO.origin_radius[i].xyz - vWorldPos.xyz;
	
	float DlightDistance = length(DlightDirection);

	float DlightDot = dot(DlightDirection, DlightDirection);
	
	float r = DLightUBO.origin_radius[i].w;

	float DlightCosine = 0.8;

	if (DlightCosine > 0)
	{
		float DlightAttenuation = clamp(((r - DlightDistance) / r), 0.0, 1.0) * DlightCosine;

		color.x += DLightUBO.color_minlight[i].x * DlightAttenuation;
		color.y += DLightUBO.color_minlight[i].y * DlightAttenuation;
		color.z += DLightUBO.color_minlight[i].z * DlightAttenuation;

		#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

			color += R_StudioDynamicLight_PhongSpecular(i, normalize(DlightDirection), vWorldPos, vNormal, specularMask, DlightAttenuation);
		
		#endif
	}

	return color;
}

vec3 R_StudioDynamicLight_PhongShading(int i, vec3 vWorldPos, vec3 vNormal, float specularMask)
{
	vec3 color = vec3(0.0, 0.0, 0.0);

	vec3 DlightDirection = DLightUBO.origin_radius[i].xyz - vWorldPos.xyz;

	float DlightDistance = length(DlightDirection);

	float r = DLightUBO.origin_radius[i].w;

	float DlightCosine = dot(vNormal, normalize(DlightDirection));
	
	if (DlightCosine > 0)
	{
		float DlightAttenuation = clamp(((r - DlightDistance) / r), 0.0, 1.0) * DlightCosine;

		color.x += DLightUBO.color_minlight[i].x * DlightAttenuation;
		color.y += DLightUBO.color_minlight[i].y * DlightAttenuation;
		color.z += DLightUBO.color_minlight[i].z * DlightAttenuation;

		#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

			color += R_StudioDynamicLight_PhongSpecular(i, normalize(DlightDirection), vWorldPos, vNormal, specularMask, DlightAttenuation);
		
		#endif
	}

	return color;
}

//The output is always in linear space

vec3 R_StudioLighting(vec3 vWorldPos, vec3 vNormal, float specularMask)
{	
	float illum = StudioUBO.r_ambientlight;

	#if defined(STUDIO_NF_FULLBRIGHT)

		return vec3(1.0, 1.0, 1.0);

	#elif defined(STUDIO_NF_FLATSHADE) || defined(STUDIO_NF_CELSHADE)

		illum += R_StudioBaseLight_FlatShading(vWorldPos, vNormal, specularMask);

	#else

		illum += R_StudioBaseLight_PhongShading(vWorldPos, vNormal, specularMask);

	#endif

	float lv = illum / 255.0;

	float linearLv = LightGammaToLinearInternal(lv);

	vec3 linearColor = vec3(linearLv, linearLv, linearLv);

	linearColor *= StudioUBO.r_color.xyz;

	#if defined(LEGACY_ELIGHT_ENABLED)
		for(int i = 0; i < StudioUBO.r_numelight; ++i)
		{
		
		#if defined(STUDIO_NF_FLATSHADE) || defined(STUDIO_NF_CELSHADE)

			linearColor += R_StudioEntityLight_FlatShading(i, vWorldPos, vNormal, specularMask);

		#else

			linearColor += R_StudioEntityLight_PhongShading(i, vWorldPos, vNormal, specularMask);

		#endif
		}
	#endif
	
	#if defined(LEGACY_DLIGHT_ENABLED)
		for(int i = 0; i < DLightUBO.active_dlights; ++i)
		{
		#if defined(STUDIO_NF_FLATSHADE) || defined(STUDIO_NF_CELSHADE)

			linearColor += R_StudioDynamicLight_FlatShading(i, vWorldPos, vNormal, specularMask);

		#else

			linearColor += R_StudioDynamicLight_PhongShading(i, vWorldPos, vNormal, specularMask);

		#endif
		}
	#endif

	return linearColor;
}

vec3 ProjectVectorOntoPlane(vec3 v, vec3 normal) {

    vec3 projectionOntoNormal = dot(v, normal) * normal;
    vec3 projectionOntoPlane = v - projectionOntoNormal;
    return normalize(projectionOntoPlane);
}

#if defined(STUDIO_NF_CELSHADE)

vec3 ShiftTangent(vec3 T, vec3 N, float uvX, vec4 noise)
{
    return normalize(T + N * (sin(uvX * noise.x) * noise.z + sin(uvX * noise.y + 1.2) * noise.w));
}

float StrandSpecular(vec3 T, vec3 H, float exponent)
{
	float dotTH = dot(T, H);
    float sinTH = max(0.01, sqrt(1.0 - dotTH * dotTH));
    float dirAtten = smoothstep(-1.0, 0.0, dotTH);
    return dirAtten * pow(sinTH, exponent);
}

vec3 R_StudioCelShade(vec3 v_color, vec3 normalWS, vec3 lightdirWS, float specularMask, vec2 screenTexCoord)
{
	vec3 N = normalWS;

    vec3 L = lightdirWS;
	vec3 V = normalize(v_worldpos.xyz - GetCameraViewPos(0));
	vec3 UP = vec3(0.0, 0.0, -1.0);
	vec3 BiT = cross(N, UP);
	vec3 T = cross(N, BiT);

	#if defined(STUDIO_NF_CELSHADE_FACE)

		vec3 vecForward = v_headfwd;
		vec3 vecUp = v_headup;

		L = R_GetAdjustedLightDirection(L);

		float flFaceCosine = abs(vecForward.z);
		flFaceCosine = pow(flFaceCosine, 10.0);

		//Check the direction
		float flFaceDot = dot(vecForward, -lightdirWS);

		//Use vecForward when flFaceCosine getting close to 1 or -1
		L = mix(L, vecForward * sign(flFaceDot), flFaceCosine);

	#else

		L = R_GetAdjustedLightDirection(L);

	#endif

    float NoL = dot(-N,L);

    // N dot L
    float litOrShadowArea = smoothstep(r_celshade_midpoint - r_celshade_softness, r_celshade_midpoint + r_celshade_softness, NoL);

#if defined(STUDIO_NF_CELSHADE_FACE) && defined(STENCIL_TEXTURE_ENABLED)

	litOrShadowArea = mix(0.5, 1.0, litOrShadowArea);

	uint stencilValue = LoadStencilValueFromStencilTexture(stencilTex, screenTexCoord);

   	if((stencilValue & STENCIL_MASK_HAS_SHADOW) == STENCIL_MASK_HAS_SHADOW)
    {
		litOrShadowArea = 0.0;
	}

#endif

    vec3 litOrShadowColor = mix(r_celshade_shadow_color.xyz, vec3(1.0, 1.0, 1.0), litOrShadowArea);

	vec3 rimLightColor = vec3(0.0);
	vec3 rimDarkColor = vec3(0.0);
	vec3 specularColor = vec3(0.0);

#if !defined(STUDIO_NF_CELSHADE_FACE)
	//Rim light
	float lambertD = max(0, -NoL);
    float lambertF = max(0, NoL);
    float rim = 1.0 - clamp(dot(V, -N), 0.0, 1.0);

	float rimDot = pow(rim, r_rimlight_power.x);
	rimDot = lambertF * rimDot;
	float rimIntensity = smoothstep(0, r_rimlight_smooth, rimDot);
	rimLightColor = pow(rimIntensity, 5.0) * r_rimlight_color.xyz;

	rimLightColor.x = rimLightColor.x * smoothstep(r_rimlight_smooth2.x, r_rimlight_smooth2.y, v_color.x);
	rimLightColor.y = rimLightColor.y * smoothstep(r_rimlight_smooth2.x, r_rimlight_smooth2.y, v_color.y);
	rimLightColor.z = rimLightColor.z * smoothstep(r_rimlight_smooth2.x, r_rimlight_smooth2.y, v_color.z);

	rimDot = pow(rim, r_rimdark_power.x);
    rimDot = lambertD * rimDot;
	rimIntensity = smoothstep(0, r_rimdark_smooth, rimDot);
    rimDarkColor = pow(rimIntensity, 5.0) * r_rimdark_color.xyz;

	rimDarkColor.x = rimDarkColor.x * smoothstep(r_rimdark_smooth2.x, r_rimdark_smooth2.y, v_color.x);
	rimDarkColor.y = rimDarkColor.y * smoothstep(r_rimdark_smooth2.x, r_rimdark_smooth2.y, v_color.y);
	rimDarkColor.z = rimDarkColor.z * smoothstep(r_rimdark_smooth2.x, r_rimdark_smooth2.y, v_color.z);
#endif

#if defined(STUDIO_NF_CELSHADE_HAIR)
	
	vec3 kajiyaSpecular = vec3(0.0);
    vec3 shiftedTangent1 = ShiftTangent(T, N, v_texcoord.x, r_hair_specular_noise);
    vec3 shiftedTangent2 = ShiftTangent(T, N, v_texcoord.x, r_hair_specular_noise2);
	
	vec3 V2 = GetCameraVForward(0);

    vec3 HforStrandSpecular = normalize(L + vec3(0.01, 0.0, 0.0) + V2);
    kajiyaSpecular += r_hair_specular_intensity * StrandSpecular(shiftedTangent1, HforStrandSpecular, r_hair_specular_exp);
    kajiyaSpecular += r_hair_specular_intensity2 * StrandSpecular(shiftedTangent2, HforStrandSpecular, r_hair_specular_exp2);

	kajiyaSpecular.x = kajiyaSpecular.x * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.x);
	kajiyaSpecular.y = kajiyaSpecular.y * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.y);
	kajiyaSpecular.z = kajiyaSpecular.z * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.z);

#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

	specularColor += kajiyaSpecular * litOrShadowColor * specularMask;

#else

	specularColor += kajiyaSpecular * litOrShadowColor;

#endif

#endif    //defined(STUDIO_NF_CELSHADE_HAIR)

	return v_color.xyz * litOrShadowColor + rimLightColor + rimDarkColor + specularColor;

}	//R_StudioCelShade

#endif  //defined(STUDIO_NF_CELSHADE)

vec3 R_GenerateSimplifiedNormal()
{
	vec3 vNormal = normalize(v_normal);

	#if defined(STUDIO_NF_DOUBLE_FACE)
		if (gl_FrontFacing) {
			vNormal = vNormal * -1.0;
		}
	#endif

	#if defined(REVERT_NORMAL_ENABLED)
		vNormal = vNormal * -1.0;
	#endif

	return vNormal;
}

vec3 R_GenerateAdjustedNormal(vec3 vWorldPos, float flNormalMask)
{
	vec3 vNormal = normalize(v_normal);

#if defined(NORMALTEXTURE_ENABLED) || defined(PACKED_NORMALTEXTURE_ENABLED)

	vec3 vTangent = normalize(v_tangent);
	vec3 vBitangent = normalize(v_bitangent);
	mat3 TBN = GenerateTBNMatrix(vTangent, vBitangent, vNormal);
	vNormal = NormalMapping(TBN, v_texcoord);

#endif

	#if defined(STUDIO_NF_DOUBLE_FACE)
		if (gl_FrontFacing) {
			vNormal = vNormal * -1.0;
		}
	#endif

	#if defined(REVERT_NORMAL_ENABLED)
		vNormal = vNormal * -1.0;
	#endif

	#if defined(STUDIO_NF_CELSHADE)

		#if defined(STUDIO_NF_CELSHADE_FACE)

			vec3 vSphereizedNormal = vWorldPos - v_headorigin;

			vSphereizedNormal = normalize(vSphereizedNormal);

			vNormal = mix(vNormal, vSphereizedNormal, flNormalMask);

		#endif

	#endif

	return vNormal;
}

vec4 SampleDiffuseTexture(vec2 baseTexcoord)
{
	#if defined(ANIMATED_TEXTURE_ENABLED)

		float layer = mod(floor(SceneUBO.cl_time * r_framerate_numframes.x), r_framerate_numframes.y);

		vec4 diffuseColor = texture(animatedTexArray, vec3(baseTexcoord.x, baseTexcoord.y, layer ));

	#elif defined(PACKED_DIFFUSETEXTURE_ENABLED)

		vec4 diffuseColor = texture(diffuseTex, vec2(baseTexcoord.x + r_packed_stride * r_packed_index.x, baseTexcoord.y));

	#else

		vec4 diffuseColor = texture(diffuseTex, baseTexcoord);

	#endif

	return diffuseColor;
}

#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

vec4 SampleRawSpecularTexture(vec2 baseTexcoord)
{
	#if defined(SPECULARTEXTURE_ENABLED)

		vec4 rawSpecularColor = texture(specularTex, baseTexcoord);

	#elif defined(PACKED_SPECULARTEXTURE_ENABLED)

		vec4 rawSpecularColor = texture(diffuseTex, vec2(baseTexcoord.x + r_packed_stride * r_packed_index.w, baseTexcoord.y));

	#endif

	return rawSpecularColor;
}

#endif

#if defined(CLIP_BONE_ENABLED)

bool IsBoneClipped(uint boneindex)
{
	uint slot = boneindex / 32;
	uint index = boneindex - slot * 4;
	
	if((StudioUBO.r_clipbone[slot] & (1 << index)) != 0)
		return true;

	return false;
}

#endif

void main(void)
{
	#if defined(CLIP_BONE_ENABLED)

		uint vertbone = UnpackStudioBoneAsVertBone(v_packedbone);

		if (IsBoneClipped(vertbone))
			discard;

	#endif
	
	float flDistanceToFragment = distance(v_worldpos.xyz, GetCameraViewPos(0));

	vec3 vWorldPos = v_worldpos.xyz;

	vec3 vSimpleNormal = R_GenerateSimplifiedNormal();

	vec4 specularColor = vec4(0.0, 0.0, 0.0, 1.0);

	float flNormalAdjustment = 0.0;
	
	ClipPlaneTest(v_worldpos.xyz, vSimpleNormal);

	#if defined(CLIP_NEARPLANE_ENABLED)
	ClipNearPlaneTest(flDistanceToFragment, r_nearplaneclip.x, r_nearplaneclip.y);
	#endif

	vec2 screenTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;

	vec4 diffuseColor = SampleDiffuseTexture(v_texcoord);

	#if defined(STUDIO_NF_MASKED)
		if(diffuseColor.a < 0.5)
			discard;
	#endif

#if defined(SHADOW_CASTER_ENABLED) //only depth output

	#if defined(LINEAR_DEPTH_ENABLED)
		float newDepth = flDistanceToFragment / GetCameraZFar(GetCameraViewIndex());

		gl_FragDepth = newDepth;
	#endif

	out_Diffuse = vec4(flDistanceToFragment, 0.0, 0.0, 1.0);

#elif defined(GLOW_COLOR_ENABLED) //only color output

	out_Diffuse = vec4(StudioUBO.r_color.xyz, 1.0);

#elif defined(HAIR_SHADOW_ENABLED) //only depth output

	out_Diffuse = vec4(1.0, 1.0, 1.0, 1.0);

#elif defined(HAIR_FACE_COLOR_MIX_ENABLED) //only color output, diffuse.a will be mixed with specular.a

	#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

		vec4 rawSpecularColor = SampleRawSpecularTexture(v_texcoord);

		diffuseColor.a *= rawSpecularColor.a;

	#endif

	out_Diffuse = diffuseColor;

#else

	#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

		vec4 rawSpecularColor = SampleRawSpecularTexture(v_texcoord);
		specularColor = rawSpecularColor;
		specularColor.b = 0.0;//Don't write to GBuffer
		
		flNormalAdjustment = rawSpecularColor.b;

	#endif

	//Mix hair color with face color
	#if defined(STUDIO_NF_CELSHADE_HAIR) && defined(MIX_DIFFUSE_TEXTURE_ENABLED)
		
		vec4 mixDiffuseColor = texture(mixDiffuseTex, screenTexCoord);

		diffuseColor.rgb = mix(mixDiffuseColor.rgb, diffuseColor.rgb, mixDiffuseColor.a);

	#endif

	diffuseColor = ProcessDiffuseColor(diffuseColor);

	vec3 vNormal = R_GenerateAdjustedNormal(vWorldPos, flNormalAdjustment);

	#if defined(ADDITIVE_RENDER_MODE_ENABLED) || defined(GLOW_SHELL_ENABLED)

		vec4 lightmapColor = ProcessOtherGammaColor(StudioUBO.r_color);
		
	#elif defined(STUDIO_NF_FULLBRIGHT)

		vec4 fullbrightColor = vec4(1.0, 1.0, 1.0, 1.0);
		vec4 lightmapColor = ProcessOtherGammaColor(fullbrightColor);

		vec3 linearLightColor = R_StudioLighting(vWorldPos, vNormal, specularColor.x);

		#if defined(GAMMA_BLEND_ENABLED)
			lightmapColor.rgb *= LinearToGamma3(linearLightColor);
		#else
			lightmapColor.rgb *= linearLightColor;
		#endif

	#else

		vec4 lightmapColor = ProcessOtherGammaColor(StudioUBO.r_color);

		vec3 linearLightColor = R_StudioLighting(vWorldPos, vNormal, specularColor.x);

		#if defined(STUDIO_NF_CELSHADE)
			linearLightColor = R_StudioCelShade(linearLightColor, vNormal, StudioUBO.r_plightvec.xyz, specularColor.x, screenTexCoord);
		#endif

		#if defined(GAMMA_BLEND_ENABLED)
			lightmapColor.rgb *= LinearToGamma3(linearLightColor);
		#else
			lightmapColor.rgb *= linearLightColor;
		#endif

	#endif

	#if defined(OUTLINE_ENABLED)
		lightmapColor.rgb *= ProcessOtherGammaColor3(vec3(r_outline_dark));
	#endif

	//lightmapColor.x = pow(lightmapColor.x, SceneUBO.r_lightmap_pow);
	//lightmapColor.y = pow(lightmapColor.y, SceneUBO.r_lightmap_pow);
	//lightmapColor.z = pow(lightmapColor.z, SceneUBO.r_lightmap_pow);
	
	lightmapColor *= SceneUBO.r_lightmap_scale;

	#if defined(GBUFFER_ENABLED)

		vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

		out_Diffuse = diffuseColor;
		out_Lightmap = lightmapColor;
		out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
		out_Specular = specularColor;

	#else

		vec4 finalColor = CalcFog(
			ProcessLinearBlendShift(diffuseColor * lightmapColor), 
			flDistanceToFragment
			);

		GatherFragment(finalColor);

		out_Diffuse = finalColor;

	#endif

#endif
}