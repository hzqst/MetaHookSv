#version 460

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_texture_array : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_ARB_gpu_shader5 : require

#define TEXTURE_SSBO_DIFFUSE 0
#define TEXTURE_SSBO_DETAIL 1
#define TEXTURE_SSBO_NORMAL 2
#define TEXTURE_SSBO_PARALLAX 3
#define TEXTURE_SSBO_SPECULAR 4

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
	float clipPlane;
	vec4 shadowDirection;
	vec4 shadowColor;
	vec4 shadowFade;
};

struct entity_ubo_t{
	mat4 entityMatrix;
	float scrollSpeed;
	float padding[3];
};

struct texture_ssbo_t{

#ifdef UINT64_ENABLE

	uint64_t handles[5 * 1];

#else

	uvec2 handles[5 * 1];

#endif
};

layout (std140, binding = 0) uniform SceneBlock
{
   scene_ubo_t SceneUBO;
};

layout (std140, binding = 1) uniform EntityBlock
{
   entity_ubo_t EntityUBO;
};

layout (std430, binding = 2) buffer TextureBlock
{
    texture_ssbo_t TextureSSBO;
};

uniform float u_parallaxScale;
uniform vec4 u_color;

#ifdef BINDLESS_ENABLED
layout(binding = 1) uniform sampler2DArray lightmapTexArray;
layout(binding = 6) uniform sampler2DArray shadowmapTexArray;
#else
layout(binding = 0) uniform sampler2D diffuseTex;
layout(binding = 1) uniform sampler2DArray lightmapTexArray;
layout(binding = 2) uniform sampler2D detailTex;
layout(binding = 3) uniform sampler2D normalTex;
layout(binding = 4) uniform sampler2D parallaxTex;
layout(binding = 5) uniform sampler2D specularTex;
layout(binding = 6) uniform sampler2DArray shadowmapTexArray;
#endif

in vec3 v_worldpos;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;
in vec4 v_color;
in vec2 v_diffusetexcoord;
in vec3 v_lightmaptexcoord;
in vec2 v_detailtexcoord;
in vec2 v_normaltexcoord;
in vec2 v_parallaxtexcoord;
in vec2 v_speculartexcoord;
in vec4 v_shadowcoord[3];
flat in int v_drawid;

layout(location = 0) out vec4 out_Diffuse;
layout(location = 1) out vec4 out_Lightmap;
layout(location = 2) out vec4 out_WorldNorm;
layout(location = 3) out vec4 out_Specular;
layout(location = 4) out vec4 out_Additive;

vec2 UnitVectorToHemiOctahedron(vec3 dir) {

	dir.y = max(dir.y, 0.0001);
	dir.xz /= dot(abs(dir), vec3(1.0));

	return clamp(0.5 * vec2(dir.x + dir.z, dir.x - dir.z) + 0.5, 0.0, 1.0);
}

vec3 HemiOctahedronToUnitVector(vec2 coord) {

	coord = 2.0 * coord - 1.0;
	coord = 0.5 * vec2(coord.x + coord.y, coord.x - coord.y);

	float y = 1.0 - dot(vec2(1.0), abs(coord));
	return normalize(vec3(coord.x, y + 0.0001, coord.y));
}

vec2 UnitVectorToOctahedron(vec3 dir) {

    dir.xz /= dot(abs(dir), vec3(1.0));

	// Lower hemisphere
	if (dir.y < 0.0) {
		vec2 orig = dir.xz;
		dir.x = (orig.x >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.y));
        dir.z = (orig.y >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.x));
	}

	return clamp(0.5 * vec2(dir.x, dir.z) + 0.5, 0.0, 1.0);
}

vec3 OctahedronToUnitVector(vec2 coord) {

	coord = 2.0 * coord - 1.0;
	float y = 1.0 - dot(abs(coord), vec2(1.0));

	// Lower hemisphere
	if (y < 0.0) {
		vec2 orig = coord;
		coord.x = (orig.x >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.y));
		coord.y = (orig.y >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.x));
	}

	return normalize(vec3(coord.x, y + 0.0001, coord.y));
}

vec3 NormalMapping(vec3 T, vec3 B, vec3 N)
{
#ifdef BINDLESS_ENABLED
	sampler2D normalTex = sampler2D(TextureSSBO.handles[v_drawid * 5 + TEXTURE_SSBO_NORMAL]);
#endif

    // Create TBN matrix. from tangent to world space
    mat3 TBN = mat3(normalize(T), normalize(B), normalize(N));

	vec2 vNormTexcoord = vec2(v_diffusetexcoord.x * v_normaltexcoord.x, v_diffusetexcoord.y * v_normaltexcoord.y);

    // Sample tangent space normal vector from normal map and remap it from [0, 1] to [-1, 1] range.
    vec3 n = texture2D(normalTex, vNormTexcoord).xyz;
    n = normalize(n * 2.0 - 1.0);

    // Multiple normal by the TBN matrix to transform the normal from tangent space to world space.
    n = normalize(TBN * n);

    return n;
}

vec2 ParallaxMapping(vec3 T, vec3 B, vec3 N, vec3 viewDirWorld)
{
#ifdef BINDLESS_ENABLED
	sampler2D parallaxTex = sampler2D(TextureSSBO.handles[v_drawid * 5 + TEXTURE_SSBO_PARALLAX]);
#endif

    // Create TBN matrix.
    mat3 TBN = mat3(normalize(T), normalize(B), normalize(N));

	//Multiple viewDir by the TBN matrix to transform the normal from tangent space to world space.
	vec3 viewDir = normalize(transpose(TBN) * viewDirWorld);

	const float minLayers = 20;
	const float maxLayers = 40;	
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0f, 0.0f, 1.0f), viewDir)));

    float layerDepth = 1.0 / numLayers;

    float currentLayerDepth = 0.0;

    vec2 p = viewDir.xy / viewDir.z * u_parallaxScale;

    vec2 deltaTexCoords = p / numLayers;

	vec2 mainTexCoods = v_diffusetexcoord.xy;

    vec2 currentTexCoords = mainTexCoods;

	vec2 ddx = dFdx(mainTexCoods);
	vec2 ddy = dFdy(mainTexCoods);

    float currentDepthMapValue = 1.0 - textureGrad(parallaxTex, vec2(currentTexCoords.x * v_parallaxtexcoord.x, currentTexCoords.y * v_parallaxtexcoord.y), ddx, ddy ).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - textureGrad(parallaxTex, vec2(currentTexCoords.x * v_parallaxtexcoord.x, currentTexCoords.y * v_parallaxtexcoord.y), ddx, ddy ).r;
        currentLayerDepth += layerDepth;
    }

	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	// get depth after and before collision for linear interpolation
	float afterDepth  = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = 1.0 - textureGrad(parallaxTex, vec2(prevTexCoords.x * v_parallaxtexcoord.x, prevTexCoords.y * v_parallaxtexcoord.y), ddx, ddy ).r - currentLayerDepth + layerDepth;
	 
	// interpolation of texture coordinates
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = mix(currentTexCoords, prevTexCoords, weight);

	return finalTexCoords;
}

float ShadowCompareDepth(vec4 coord, vec2 off, float layer)
{
	vec4 newcoord = coord + vec4(off.x * SHADOW_TEXTURE_OFFSET, off.y * SHADOW_TEXTURE_OFFSET, 0.0, 0.0);

	float depth0 = texture2DArray(shadowmapTexArray, vec3(newcoord.xy / newcoord.w, layer) ).a;

	float depth1 = newcoord.z / newcoord.w;

	return depth0 < depth1 ? 0.0 : 1.0;
}

vec3 ShadowGetWorldPosition(vec4 coord, float layer)
{
	return texture2DArray(shadowmapTexArray, vec3(coord.xy / coord.w, layer) ).xyz;
}

float CalcShadowIntensityInternal(vec3 worldpos, float lightmapLum, float layer, float shadow_high, float shadow_medium, float shadow_low)
{
	float shadow_intensity = 1.0;

	vec3 scene = worldpos.xyz;
	
	int ilayer = int(layer);
	vec3 caster = ShadowGetWorldPosition(v_shadowcoord[ilayer], layer);

	float dist = distance(caster, scene);
	float distlerp = (dist - SceneUBO.shadowFade.x) / SceneUBO.shadowFade.y;
	shadow_intensity *= 1.0 - clamp(distlerp, 0.0, 1.0);
	
	float lumlerp = (lightmapLum - SceneUBO.shadowFade.w) / (SceneUBO.shadowFade.z - SceneUBO.shadowFade.w);
	shadow_intensity *= clamp(lumlerp, 0.0, 1.0);
	
	shadow_high = 1.0 - shadow_high;
	shadow_medium = 1.0 - shadow_medium;
	shadow_low = 1.0 - shadow_low;

	float shadow_final = shadow_high + shadow_medium + shadow_low;
	shadow_final = clamp(shadow_final, 0.0, 1.0) * shadow_intensity;

	return shadow_final;
}

float CalcShadowIntensity(vec3 worldpos, vec3 lightmap, vec3 norm, vec3 lightdir)
{
	float shadow_final = 0.0;
	if(dot(norm.xyz, lightdir.xyz) < 0.0) 
	{
		float lightmapLum = 0.299 * lightmap.x + 0.587 * lightmap.y + 0.114 * lightmap.z;
		if(lightmapLum > SceneUBO.shadowFade.w)
		{
			float shadow_high = 1.0;

			#ifdef SHADOWMAP_HIGH_ENABLED
				shadow_high = 0.0;
				shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2(0.0,0.0), 0.0) * 0.25;
				shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( -1.0, -1.0), 0.0) * 0.0625;
				shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( -1.0, 0.0), 0.0) * 0.125;
				shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( -1.0, 1.0), 0.0) * 0.0625;
				shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( 0.0, -1.0), 0.0) * 0.125;
				shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( 0.0, 1.0), 0.0) * 0.125;
				shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( 1.0, -1.0), 0.0) * 0.0625;
				shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( 1.0, 0.0), 0.0) * 0.125;
				shadow_high += ShadowCompareDepth(v_shadowcoord[0], vec2( 1.0, 1.0), 0.0) * 0.0625;
			#endif

			float shadow_medium = 1.0;

			#ifdef SHADOWMAP_MEDIUM_ENABLED
				shadow_medium = 0.0;
				shadow_medium += ShadowCompareDepth(v_shadowcoord[1], vec2(0.0,0.0), 1.0);
				shadow_medium += ShadowCompareDepth(v_shadowcoord[1], vec2(0.035,0.0), 1.0);
				shadow_medium += ShadowCompareDepth(v_shadowcoord[1], vec2(-0.035,0.0), 1.0);
				shadow_medium += ShadowCompareDepth(v_shadowcoord[1], vec2(0.0,0.035), 1.0);
				shadow_medium += ShadowCompareDepth(v_shadowcoord[1], vec2(0.0,-0.035), 1.0);
				shadow_medium *= 0.2;
			#endif

			float shadow_low = 1.0;

			#ifdef SHADOWMAP_LOW_ENABLED
				shadow_low = 0.0;
				shadow_low += ShadowCompareDepth(v_shadowcoord[2], vec2(0.0,0.0), 2.0);
				shadow_low += ShadowCompareDepth(v_shadowcoord[2], vec2(0.035,0.0), 2.0);
				shadow_low += ShadowCompareDepth(v_shadowcoord[2], vec2(-0.035,0.0), 2.0);
				shadow_low += ShadowCompareDepth(v_shadowcoord[2], vec2(0.0,0.035), 2.0);
				shadow_low += ShadowCompareDepth(v_shadowcoord[2], vec2(0.0,-0.035), 2.0);
				shadow_low *= 0.2;
			#endif

			if(false)
			{
				//nothing here
			}
			
		#ifdef SHADOWMAP_HIGH_ENABLED
			else if(shadow_high < 0.95)
			{
				shadow_final = CalcShadowIntensityInternal(worldpos, lightmapLum, 0.0, shadow_high, shadow_medium, shadow_low);
			}
		#endif

		#ifdef SHADOWMAP_MEDIUM_ENABLED
			else if(shadow_medium < 0.95)
			{
				shadow_final = CalcShadowIntensityInternal(worldpos, lightmapLum, 1.0, shadow_high, shadow_medium, shadow_low);
			}
		#endif

		#ifdef SHADOWMAP_LOW_ENABLED
			else if(shadow_low < 0.95)
			{
				shadow_final = CalcShadowIntensityInternal(worldpos, lightmapLum, 2.0, shadow_high, shadow_medium, shadow_low);
			}
		#endif
		}
	}
	return shadow_final;
}

void main()
{
#ifdef DIFFUSE_ENABLED

	#ifdef BINDLESS_ENABLED
		sampler2D diffuseTex = sampler2D(TextureSSBO.handles[v_drawid * 5 + TEXTURE_SSBO_DIFFUSE]);
	#endif

	#ifdef PARALLAXTEXTURE_ENABLED

		vec3 viewDir = normalize(v_worldpos.xyz - SceneUBO.viewpos.xyz);

		vec4 diffuseColor = texture2D(diffuseTex, ParallaxMapping(v_tangent, v_bitangent, v_normal, viewDir));

	#else

		vec4 diffuseColor = texture2D(diffuseTex, v_diffusetexcoord.xy);

	#endif
#else

	vec4 diffuseColor = v_color;

#endif

#ifdef LIGHTMAP_ENABLED

	vec4 lightmapColor = texture2DArray(lightmapTexArray, v_lightmaptexcoord.xyz);

#else

	vec4 lightmapColor = vec4(1.0, 1.0, 1.0, 1.0);

#endif

#ifdef NORMALTEXTURE_ENABLED

	vec3 vNormal = NormalMapping(v_tangent, v_bitangent, v_normal);

#else

	vec3 vNormal = normalize(v_normal.xyz);

#endif

#ifdef SHADOWMAP_ENABLED

	lightmapColor.xyz = mix(lightmapColor.xyz, SceneUBO.shadowColor.xyz, CalcShadowIntensity(v_worldpos, lightmapColor.xyz, vNormal, SceneUBO.shadowDirection.xyz) * SceneUBO.shadowColor.a);

#endif


#ifdef DETAILTEXTURE_ENABLED

#ifdef BINDLESS_ENABLED
	sampler2D detailTex = sampler2D(TextureSSBO.handles[v_drawid * 5 + TEXTURE_SSBO_DETAIL]);
#endif

	vec2 detailTexCoord = vec2(v_diffusetexcoord.x * v_detailtexcoord.x, v_diffusetexcoord.y * v_detailtexcoord.y);
	vec4 detailColor = texture2D(detailTex, detailTexCoord);
    detailColor.xyz *= 2.0;
    detailColor.a = 1.0;

#else

	vec4 detailColor = vec4(1.0, 1.0, 1.0, 1.0);

#endif

#ifdef CLIP_ABOVE_ENABLED
	if (v_worldpos.z > SceneUBO.clipPlane)
		discard;
#endif

#ifdef CLIP_UNDER_ENABLED
	if (v_worldpos.z < SceneUBO.clipPlane)
		discard;
#endif

#ifdef SHADOW_CASTER_ENABLED

	out_Diffuse.xyz = v_worldpos.xyz;
	out_Diffuse.w = gl_FragCoord.z;

#else

	#ifdef GBUFFER_ENABLED

		vec2 vOctNormal = UnitVectorToOctahedron(vNormal);

		float flDistanceToFragment = distance(v_worldpos.xyz, SceneUBO.viewpos.xyz);

	#ifdef SPECULARTEXTURE_ENABLED
	
		#ifdef BINDLESS_ENABLED
			sampler2D specularTex = sampler2D(TextureSSBO.handles[v_drawid * 5 + TEXTURE_SSBO_SPECULAR]);
		#endif
		vec2 specularTexCoord = vec2(v_diffusetexcoord.x * v_speculartexcoord.x, v_diffusetexcoord.y * v_speculartexcoord.y);
		vec4 specularColor = texture2D(specularTex, specularTexCoord);
	#else
		vec4 specularColor = vec4(0.0);
	#endif

		out_Diffuse = diffuseColor * detailColor;
		out_Lightmap = lightmapColor;
		out_WorldNorm = vec4(vOctNormal.x, vOctNormal.y, flDistanceToFragment, 0.0);
		out_Specular = specularColor;
		out_Additive = vec4(0.0);

	#else

		#ifdef TRANSPARENT_ENABLED

			out_Diffuse = diffuseColor * lightmapColor * detailColor * v_color;

		#else

			out_Diffuse = diffuseColor * lightmapColor * detailColor;

		#endif

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