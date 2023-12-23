#version 430

#include "common.h"

layout(binding = STUDIO_DIFFUSE_TEXTURE) uniform sampler2D diffuseTex;
layout(binding = STUDIO_NORMAL_TEXTURE) uniform sampler2D normalTex;
layout(binding = STUDIO_PARALLAX_TEXTURE) uniform sampler2D parallaxTex;
layout(binding = STUDIO_SPECULAR_TEXTURE) uniform sampler2D specularTex;
layout(binding = 6) uniform usampler2D stencilTex;

/* celshade */

uniform float r_celshade_midpoint;
uniform float r_celshade_softness;
uniform vec3 r_celshade_shadow_color;
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

in vec3 v_worldpos;
in vec3 v_normal;
in vec2 v_texcoord;
in vec4 v_projpos;

#if defined(STUDIO_NF_CELSHADE)
in mat4 v_bonematrix;
in mat4 v_invbonematrix;
#endif

layout(location = 0) out vec4 out_Diffuse;

#if defined(GBUFFER_ENABLED)
layout(location = 1) out vec4 out_Lightmap;
layout(location = 2) out vec4 out_WorldNorm;
layout(location = 3) out vec4 out_Specular;
#endif

#if defined(NORMALTEXTURE_ENABLED)

mat3 CalcTBNMatrix()
{
  // Calculate the TBN matrix
    vec3 dp1 = dFdx(v_worldpos);
    vec3 dp2 = dFdy(v_worldpos);
    vec2 duv1 = dFdx(v_texcoord);
    vec2 duv2 = dFdy(v_texcoord);

    // Solve the linear system
    vec3 dp2perp = cross(dp2, v_normal);
    vec3 dp1perp = cross(v_normal, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // Construct a tangent-bitangent-normal matrix
    return mat3(normalize(T), normalize(B), v_normal);
}

vec3 NormalMapping(mat3 TBN, vec2 baseTexcoord)
{
	vec2 vNormTexcoord = vec2(baseTexcoord.x, baseTexcoord.y);

    // Sample tangent space normal vector from normal map and remap it from [0, 1] to [-1, 1] range.
    vec3 n = texture(normalTex, vNormTexcoord).xyz;
    n = normalize(n * 2.0 - 1.0);

    // Multiple normal by the TBN matrix to transform the normal from tangent space to world space.
    n = normalize(TBN * n);

    return n;
}

#endif

//The output is in Linear Space
vec3 R_StudioLightingLinear(vec3 vWorldPos, vec3 vNormal, float specularMask)
{	
	#if defined(STUDIO_NF_DOUBLE_FACE)
		if (!gl_FrontFacing) {
			vNormal = vNormal * -1.0;
		}
	#endif

	#if defined(INVERT_NORMAL_ENABLED)
		vNormal = vNormal * -1.0;
	#endif

	float illum = StudioUBO.r_ambientlight;

	#if defined(STUDIO_NF_FULLBRIGHT)

		return vec3(1.0, 1.0, 1.0);

	#elif defined(STUDIO_NF_FLATSHADE) || defined(STUDIO_NF_CELSHADE)

		illum += StudioUBO.r_shadelight * 0.8;

	#else

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

		#if defined(STUDIO_NF_FLATSHADE) || defined(STUDIO_NF_CELSHADE)

		#else

			#if defined(SPECULARTEXTURE_ENABLED)

				vec3 VertexToEye = normalize(SceneUBO.viewpos.xyz - vWorldPos.xyz);
				vec3 LightReflect = normalize(reflect(StudioUBO.r_plightvec.xyz, vNormal.xyz));
				float SpecularFactor = dot(VertexToEye, LightReflect);
				SpecularFactor = clamp(SpecularFactor, 0.0, 1.0);
				SpecularFactor = pow(SpecularFactor * SceneUBO.r_studio_shade_specular,
				SceneUBO.r_studio_shade_specularpow);

				illum += StudioUBO.r_shadelight * SpecularFactor * specularMask;

			#endif

		#endif

	#endif

	//Really need to clamp?

	float lv = clamp(illum, 0.0, 255.0) / 255.0;

	lv = LightGammaToLinearInternal(lv);

	vec3 color = vec3(lv, lv, lv);		

	for(int i = 0; i < StudioUBO.r_numelight.x; ++i)
	{
		vec3 ElightDirection = StudioUBO.r_elight_origin[i].xyz - vWorldPos.xyz;
		
		#if defined(STUDIO_NF_FLATSHADE) || defined(STUDIO_NF_CELSHADE)
		
			float ElightCosine = 0.8;
		
		#else

			float ElightCosine = clamp(dot(vNormal, normalize(ElightDirection)), 0.0, 1.0);

		#endif

		float ElightDistance = length(ElightDirection);
		float ElightDot = dot(ElightDirection, ElightDirection);

		float r2 = StudioUBO.r_elight_radius[i];
		
		r2 = r2 * r2;

		float ElightAttenuation = clamp(r2 / (ElightDot * ElightDistance), 0.0, 1.0);

		color.x += StudioUBO.r_elight_color[i].x * ElightCosine;
		color.y += StudioUBO.r_elight_color[i].y * ElightCosine;
		color.z += StudioUBO.r_elight_color[i].z * ElightCosine;

		#if defined(STUDIO_NF_FLATSHADE) || defined(STUDIO_NF_CELSHADE)

		#else

			#if defined(SPECULARTEXTURE_ENABLED)

				vec3 EVertexToEye = normalize(SceneUBO.viewpos.xyz - vWorldPos.xyz);
				vec3 ELightReflect = normalize(reflect(ElightDirection, vNormal.xyz));
				float ESpecularFactor = dot(EVertexToEye, ELightReflect);
				ESpecularFactor = clamp(ESpecularFactor, 0.0, 1.0);
				ESpecularFactor = pow(ESpecularFactor * SceneUBO.r_studio_shade_specular,
				SceneUBO.r_studio_shade_specularpow);
				color.x += StudioUBO.r_elight_color[i].x * ESpecularFactor * specularMask;
				color.y += StudioUBO.r_elight_color[i].y * ESpecularFactor * specularMask;
				color.z += StudioUBO.r_elight_color[i].z * ESpecularFactor * specularMask;

			#endif

		#endif
	}

	return color;
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

vec3 R_StudioCelShade(vec3 v_color, vec3 normalWS, vec3 lightdirWS, float specularMask)
{
	vec3 N = normalWS;

	#if defined(STUDIO_NF_DOUBLE_FACE)
		if (!gl_FrontFacing) {
			N = N * -1.0;
		}
	#endif

    vec3 L = lightdirWS;
	vec3 V = normalize(v_worldpos.xyz - SceneUBO.viewpos.xyz);
	vec3 UP = vec3(0.0, 0.0, -1.0);
	vec3 BiT = cross(N, UP);
	vec3 T = cross(N, BiT);

	vec4 lightdirLS = v_invbonematrix * vec4(L, 0.0);
	
#if defined(STUDIO_NF_CELSHADE_FACE)
	lightdirLS.z = 0;
#else
	lightdirLS.z *= 0.01;
#endif

	lightdirLS.xyz = normalize(lightdirLS.xyz);

	vec4 lightdirWS_transformed = lightdirLS * v_bonematrix;

	L = lightdirWS_transformed.xyz;

    float NoL = dot(-N,L);

    // N dot L
    float litOrShadowArea = smoothstep(r_celshade_midpoint - r_celshade_softness, r_celshade_midpoint + r_celshade_softness, NoL);

#if defined(STUDIO_NF_CELSHADE_FACE)

	litOrShadowArea = mix(0.5, 1.0, litOrShadowArea);

	vec2 screenTexCoord = v_projpos.xy / v_projpos.w * 0.5 + 0.5;
	uint stencilValue = texture(stencilTex, screenTexCoord).r;

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
	
	vec3 V2 = SceneUBO.vpn.xyz;

    vec3 HforStrandSpecular = normalize(L + vec3(0.01, 0.0, 0.0) + V2);
    kajiyaSpecular += r_hair_specular_intensity * StrandSpecular(shiftedTangent1, HforStrandSpecular, r_hair_specular_exp);
    kajiyaSpecular += r_hair_specular_intensity2 * StrandSpecular(shiftedTangent2, HforStrandSpecular, r_hair_specular_exp2);

	kajiyaSpecular.x = kajiyaSpecular.x * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.x);
	kajiyaSpecular.y = kajiyaSpecular.y * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.y);
	kajiyaSpecular.z = kajiyaSpecular.z * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.z);

#if defined(SPECULARTEXTURE_ENABLED)

	specularColor += kajiyaSpecular * litOrShadowColor * specularMask;

#else

	specularColor += kajiyaSpecular * litOrShadowColor;

#endif

#elif defined(STUDIO_NF_CELSHADE_HAIR_H)
	
	vec3 kajiyaSpecular = vec3(0.0);
    vec3 shiftedTangent1 = ShiftTangent(BiT, N, v_texcoord.y, r_hair_specular_noise);
    vec3 shiftedTangent2 = ShiftTangent(BiT, N, v_texcoord.y, r_hair_specular_noise2);

	vec3 V2 = SceneUBO.vpn.xyz;

    vec3 HforStrandSpecular = normalize(-L + vec3(0.01, 0.0, 0.0) + V2);
    kajiyaSpecular += r_hair_specular_intensity * StrandSpecular(shiftedTangent1, HforStrandSpecular, r_hair_specular_exp);
    kajiyaSpecular += r_hair_specular_intensity2 * StrandSpecular(shiftedTangent2, HforStrandSpecular, r_hair_specular_exp2);

	kajiyaSpecular.x = kajiyaSpecular.x * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.x);
	kajiyaSpecular.y = kajiyaSpecular.y * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.y);
	kajiyaSpecular.z = kajiyaSpecular.z * smoothstep(r_hair_specular_smooth.x, r_hair_specular_smooth.y, v_color.z);

#if defined(SPECULARTEXTURE_ENABLED)

	specularColor += kajiyaSpecular * litOrShadowColor * specularMask;

#else

	specularColor += kajiyaSpecular * litOrShadowColor;

#endif

#endif

#if 0

	vec3 halfVec = normalize(L + vec3(0.01, 0.0, 0.0) + SceneUBO.vpn.xyz);

	float specular = dot(N, halfVec);

	vec3 specularColorMasked = vec3(0.0);
	vec3 specularColorSmooth = vec3(0.0);

	specularColorMasked.x = pow(v_color.x * specular, 0.8);
	specularColorMasked.y = pow(v_color.y * specular, 0.8);
	specularColorMasked.z = pow(v_color.z * specular, 0.8);

	specularColorSmooth.x = smoothstep(0, 0.01 * 1, specularColorMasked.x);
	specularColorSmooth.y = smoothstep(0, 0.01 * 1, specularColorMasked.y);
	specularColorSmooth.z = smoothstep(0, 0.01 * 1, specularColorMasked.z);

	specularColor += specularColorSmooth;

#endif

	return v_color.xyz * litOrShadowColor + rimLightColor + rimDarkColor + specularColor;
}

#endif

void main(void)
{
#if !defined(SHADOW_CASTER_ENABLED) && !defined(HAIR_SHADOW_ENABLED)

	vec3 vWorldPos = v_worldpos.xyz;

	vec4 specularColor = vec4(0.0);

#if defined(NORMALTEXTURE_ENABLED)

	mat3 TBN = CalcTBNMatrix();
	vec3 vNormal = NormalMapping(TBN, v_texcoord);

#else

	vec3 vNormal = normalize(v_normal.xyz);

#endif

	ClipPlaneTest(v_worldpos.xyz, vNormal);

	vec2 texcoord = v_texcoord;

	vec4 diffuseColor = texture(diffuseTex, texcoord);

	#if defined(STUDIO_NF_MASKED)
		if(diffuseColor.a < 0.5)
			discard;
	#endif

	diffuseColor = ProcessDiffuseColor(diffuseColor);

	#if defined(SPECULARTEXTURE_ENABLED)

		vec2 specularTexCoord = vec2(v_texcoord.x, v_texcoord.y);
		specularColor.xy = texture(specularTex, specularTexCoord).xy;

	#endif

	#if defined(ADDITIVE_RENDER_MODE_ENABLED)

		vec4 lightmapColor = ProcessOtherGammaColor(StudioUBO.r_color);
		
	#elif defined(GLOW_SHELL_ENABLED)

		vec4 lightmapColor = ProcessOtherGammaColor(StudioUBO.r_color);

	#else

		vec4 lightmapColor = ProcessOtherGammaColor(StudioUBO.r_color);

		vec3 lightColorLinear = R_StudioLightingLinear(vWorldPos, vNormal, specularColor.x);

		#if defined(STUDIO_NF_CELSHADE)
			lightColorLinear = R_StudioCelShade(lightColorLinear, vNormal, StudioUBO.r_plightvec.xyz, specularColor.x);
		#endif

		#if defined(GAMMA_BLEND_ENABLED)
			lightmapColor.rgb *= LinearToGamma3(lightColorLinear);
		#else
			lightmapColor.rgb *= lightColorLinear;
		#endif

	#endif

	#if defined(OUTLINE_ENABLED)
		lightmapColor.rgb *= ProcessOtherGammaColor3(vec3(r_outline_dark));
	#endif

#endif

#if defined(SHADOW_CASTER_ENABLED)

	//Position output

	#if defined(STUDIO_NF_MASKED)
		vec4 diffuseColorMask = texture(diffuseTex, v_texcoord);

		if(diffuseColorMask.a < 0.5)
			discard;
	#endif

	out_Diffuse = vec4(StudioUBO.entity_origin.x, StudioUBO.entity_origin.y, StudioUBO.entity_origin.z, gl_FragCoord.z);

#elif defined(HAIR_SHADOW_ENABLED)

	//No color output

	out_Diffuse = vec4(1.0, 1.0, 1.0, 1.0);

#else

	//Normal color output

	#if defined(GBUFFER_ENABLED)

		vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

		float flDistanceToFragment = distance(v_worldpos.xyz, SceneUBO.viewpos.xyz);

		out_Diffuse = diffuseColor;
		out_Lightmap = lightmapColor;
		out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
		out_Specular = specularColor;

	#else

		vec4 finalColor = CalcFog(diffuseColor * lightmapColor);

		GatherFragment(finalColor);

		out_Diffuse = finalColor;

	#endif

#endif
}