#version 430

#include "common.h"

layout(binding = STUDIO_DIFFUSE_TEXTURE) uniform sampler2D diffuseTex;
layout(binding = STUDIO_NORMAL_TEXTURE) uniform sampler2D normalTex;
layout(binding = STUDIO_PARALLAX_TEXTURE) uniform sampler2D parallaxTex;
layout(binding = STUDIO_SPECULAR_TEXTURE) uniform sampler2D specularTex;
layout(binding = STUDIO_RESERVED_TEXTURE_STENCIL) uniform usampler2D stencilTex;
layout(binding = STUDIO_RESERVED_TEXTURE_ANIMATED) uniform sampler2DArray animatedTexArray;

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

in vec3 v_worldpos;
in vec3 v_normal;
in vec2 v_texcoord;
in vec4 v_projpos;
flat in ivec2 v_vertnormbone;

#if defined(STUDIO_NF_CELSHADE_FACE)

	in vec3 v_headfwd;
	in vec3 v_headup;
	in vec3 v_headorigin;

	#if defined(STUDIO_DEBUG_ENABLED)

		in vec4 v_headorigin_proj;

	#endif

#endif

layout(location = 0) out vec4 out_Diffuse;

#if defined(GBUFFER_ENABLED)
layout(location = 1) out vec4 out_Lightmap;
layout(location = 2) out vec4 out_WorldNorm;
layout(location = 3) out vec4 out_Specular;
#endif

#if defined(NORMALTEXTURE_ENABLED) || defined(PACKED_DIFFUSETEXTURE_ENABLED)

mat3 GenerateTBNMatrix()
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

	vec3 vecVertexToEye = normalize(CameraUBO.viewpos.xyz - vWorldPos.xyz);
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

	vec3 vecVertexToEye = normalize(CameraUBO.viewpos.xyz - vWorldPos.xyz);
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

vec3 R_StudioEntityLight_PhongSpecular(int i, vec3 vElightDirection, vec3 vWorldPos, vec3 vNormal, float specularMask)
{
	vec3 color = vec3(0.0, 0.0, 0.0);

	vec3 vecVertexToEye = normalize(CameraUBO.viewpos.xyz - vWorldPos.xyz);
	vec3 vecLightReflect = normalize(reflect(vElightDirection, vNormal.xyz));
	
	float flSpecularFactor = dot(vecVertexToEye, vecLightReflect);
	flSpecularFactor = clamp(flSpecularFactor, 0.0, 1.0);
	flSpecularFactor = pow(flSpecularFactor, r_base_specular.y) * r_base_specular.x;

	color.x += StudioUBO.r_elight_color[i].x * flSpecularFactor * specularMask;
	color.y += StudioUBO.r_elight_color[i].y * flSpecularFactor * specularMask;
	color.z += StudioUBO.r_elight_color[i].z * flSpecularFactor * specularMask;

	return color;
}

vec3 R_StudioEntityLight_FlatShading(int i, vec3 vWorldPos, vec3 vNormal, float specularMask)
{
	vec3 color = vec3(0.0, 0.0, 0.0);
	
	vec3 ElightDirection = StudioUBO.r_elight_origin[i].xyz - vWorldPos.xyz;

	float ElightCosine = 0.8;

	float ElightDistance = length(ElightDirection);
	float ElightDot = dot(ElightDirection, ElightDirection);

	float r2 = StudioUBO.r_elight_radius[i];

	r2 = r2 * r2;

	float ElightAttenuation = clamp(r2 / (ElightDot * ElightDistance), 0.0, 1.0);

	color.x += StudioUBO.r_elight_color[i].x * ElightAttenuation;
	color.y += StudioUBO.r_elight_color[i].y * ElightAttenuation;
	color.z += StudioUBO.r_elight_color[i].z * ElightAttenuation;

	#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

		color += R_StudioEntityLight_PhongSpecular(i, ElightDirection, vWorldPos, vNormal, specularMask);
		
	#endif

	return color;
}

vec3 R_StudioEntityLight_PhongShading(int i, vec3 vWorldPos, vec3 vNormal, float specularMask)
{
	vec3 color = vec3(0.0, 0.0, 0.0);

	vec3 ElightDirection = StudioUBO.r_elight_origin[i].xyz - vWorldPos.xyz;
		
	float ElightCosine = clamp(dot(vNormal, normalize(ElightDirection)), 0.0, 1.0);
	float ElightDistance = length(ElightDirection);
	float ElightDot = dot(ElightDirection, ElightDirection);

	float r2 = StudioUBO.r_elight_radius[i];
	
	r2 = r2 * r2;

	float ElightAttenuation = clamp(r2 / (ElightDot * ElightDistance), 0.0, 1.0);

	color.x += StudioUBO.r_elight_color[i].x * ElightAttenuation;
	color.y += StudioUBO.r_elight_color[i].y * ElightAttenuation;
	color.z += StudioUBO.r_elight_color[i].z * ElightAttenuation;

	#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

		color += R_StudioEntityLight_PhongSpecular(i, ElightDirection, vWorldPos, vNormal, specularMask);
		
	#endif

	return color;
}

//The output is in linear space
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

	#if defined(STUDIO_NF_OVERBRIGHT)

		lv = LightGammaToLinearOverBrightInternal(lv);
	#else

		lv = LightGammaToLinearInternal(lv);

	#endif

	vec3 color = vec3(lv, lv, lv);		

	for(int i = 0; i < StudioUBO.r_numelight.x; ++i)
	{
		
	#if defined(STUDIO_NF_FLATSHADE) || defined(STUDIO_NF_CELSHADE)

		color += R_StudioEntityLight_FlatShading(i, vWorldPos, vNormal, specularMask);

	#else

		color += R_StudioEntityLight_PhongShading(i, vWorldPos, vNormal, specularMask);

	#endif

	}

	return color;
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

vec3 R_StudioCelShade(vec3 v_color, vec3 normalWS, vec3 lightdirWS, float specularMask)
{
	vec3 N = normalWS;

    vec3 L = lightdirWS;
	vec3 V = normalize(v_worldpos.xyz - CameraUBO.viewpos.xyz);
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
	
	vec3 V2 = CameraUBO.vpn.xyz;

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

#elif defined(STUDIO_NF_CELSHADE_HAIR_H)
	
	vec3 kajiyaSpecular = vec3(0.0);
    vec3 shiftedTangent1 = ShiftTangent(BiT, N, v_texcoord.y, r_hair_specular_noise);
    vec3 shiftedTangent2 = ShiftTangent(BiT, N, v_texcoord.y, r_hair_specular_noise2);

	vec3 V2 = CameraUBO.vpn.xyz;

    vec3 HforStrandSpecular = normalize(-L + vec3(0.01, 0.0, 0.0) + V2);
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
}         //R_StudioCelShade

#endif  //defined(STUDIO_NF_CELSHADE)

#if defined(STUDIO_NF_CELSHADE_FACE) && defined(STUDIO_DEBUG_ENABLED) 

vec4 R_RenderDebugPoint(vec4 baseColor)
{ 
	// Convert to normalized device coordinates (NDC) by dividing by the w component
    vec3 point_ndc = v_headorigin_proj.xyz / v_headorigin_proj.w;
    vec3 vertex_ndc = v_projpos.xyz / v_projpos.w;

	point_ndc.x = point_ndc.x * CameraUBO.viewport.x / CameraUBO.viewport.y;
	vertex_ndc.x = vertex_ndc.x * CameraUBO.viewport.x / CameraUBO.viewport.y;

	if(distance(point_ndc.xy, vertex_ndc.xy) < 0.01)
	{
		return vec4(1.0, 0.0, 0.0, 1.0);
	}

	return baseColor;
}

#endif

vec3 R_GenerateSimplifiedNormal()
{
	vec3 vNormal = normalize(v_normal.xyz);

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
#if defined(NORMALTEXTURE_ENABLED) || defined(PACKED_NORMALTEXTURE_ENABLED)

	mat3 TBN = GenerateTBNMatrix();
	vec3 vNormal = NormalMapping(TBN, v_texcoord);

#else

	vec3 vNormal = normalize(v_normal.xyz);

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

bool IsBoneClipped(int boneindex)
{
	int slot = boneindex / 32;
	int index = boneindex - slot * 4;
	
	if((StudioUBO.r_clipbone[slot] & (1 << index)) != 0)
		return true;

	return false;
}

#endif

void main(void)
{
	#if defined(CLIP_BONE_ENABLED)

		int vertbone = v_vertnormbone.x;

		if(IsBoneClipped(vertbone))
			discard;

	#endif

#if !defined(SHADOW_CASTER_ENABLED) && !defined(HAIR_SHADOW_ENABLED)

	vec3 vWorldPos = v_worldpos.xyz;

	vec3 vSimpleNormal = R_GenerateSimplifiedNormal();

	vec4 specularColor = vec4(0.0);

	float flNormalMask = 0.0;

	ClipPlaneTest(v_worldpos.xyz, vSimpleNormal);

	vec4 diffuseColor = SampleDiffuseTexture(v_texcoord);

	#if defined(STUDIO_NF_MASKED)
		if(diffuseColor.a < 0.5)
			discard;
	#endif

	diffuseColor = ProcessDiffuseColor(diffuseColor);

	#if defined(SPECULARTEXTURE_ENABLED) || defined(PACKED_SPECULARTEXTURE_ENABLED)

		vec4 rawSpecularColor = SampleRawSpecularTexture(v_texcoord);
		specularColor.xy = rawSpecularColor.xy;
		flNormalMask = rawSpecularColor.z;

	#endif

	//Some meshes has STUDIO_NF_CELSHADE_FACE but has no STUDIO_NF_CELSHADE, why ???

	#if defined(STUDIO_DEBUG_ENABLED)  && defined(STUDIO_NF_CELSHADE_FACE)

		diffuseColor.b = flNormalMask;

		diffuseColor = R_RenderDebugPoint(diffuseColor);

	#endif

	vec3 vNormal = R_GenerateAdjustedNormal(vWorldPos, flNormalMask);

	#if defined(ADDITIVE_RENDER_MODE_ENABLED)

		vec4 lightmapColor = ProcessOtherGammaColor(StudioUBO.r_color);
		
	#elif defined(GLOW_SHELL_ENABLED)

		vec4 lightmapColor = ProcessOtherGammaColor(StudioUBO.r_color);
		
	#elif defined(STUDIO_NF_FULLBRIGHT)

		vec4 fullbrightColor = vec4(1.0, 1.0, 1.0, 1.0);
		vec4 lightmapColor = ProcessOtherGammaColor(fullbrightColor);

		vec3 lightColorLinear = R_StudioLighting(vWorldPos, vNormal, specularColor.x);

		#if defined(GAMMA_BLEND_ENABLED)
			lightmapColor.rgb *= LinearToGamma3(lightColorLinear);
		#else
			lightmapColor.rgb *= lightColorLinear;
		#endif

	#else

		vec4 lightmapColor = ProcessOtherGammaColor(StudioUBO.r_color);

		vec3 lightColorLinear = R_StudioLighting(vWorldPos, vNormal, specularColor.x);

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
		vec4 diffuseColorMask = SampleDiffuseTexture(v_texcoord);

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

		float flDistanceToFragment = distance(v_worldpos.xyz, CameraUBO.viewpos.xyz);

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