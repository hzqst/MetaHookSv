#version 430

#extension GL_EXT_texture_array : require

#ifdef BINDLESS_ENABLED
#extension GL_ARB_shader_draw_parameters : require
#endif

#include "common.h"

uniform float u_parallaxScale;

layout(location = 0) in vec3 in_vertex;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec3 in_bitangent;
layout(location = 4) in vec3 in_diffusetexcoord;
layout(location = 5) in vec3 in_lightmaptexcoord;
layout(location = 6) in vec2 in_replacetexcoord;
layout(location = 7) in vec2 in_detailtexcoord;
layout(location = 8) in vec2 in_normaltexcoord;
layout(location = 9) in vec2 in_parallaxtexcoord;
layout(location = 10) in vec2 in_speculartexcoord;

#if defined(SKYBOX_ENABLED)

#elif defined(DECAL_ENABLED)
	layout(location = 11) in int in_decalindex;
	layout(location = 12) in uvec4 in_styles;
#else
	layout(location = 11) in int in_texindex;
	layout(location = 12) in uvec4 in_styles;
#endif

out vec3 v_worldpos;
out vec3 v_normal;
out vec3 v_tangent;
out vec3 v_bitangent;
out vec2 v_diffusetexcoord;
out vec3 v_lightmaptexcoord;
out vec2 v_replacetexcoord;
out vec2 v_detailtexcoord;
out vec2 v_normaltexcoord;
out vec2 v_parallaxtexcoord;
out vec2 v_speculartexcoord;
out vec4 v_shadowcoord[3];
out vec4 v_projpos;

#ifdef BINDLESS_ENABLED

	#if defined(SKYBOX_ENABLED)
		flat out int v_drawid;
	#elif defined(DECAL_ENABLED)
		flat out int v_decalindex;
	#else
		flat out int v_texindex;
	#endif

#endif

#if defined(SKYBOX_ENABLED)
	
#elif defined(DECAL_ENABLED)
	flat out uvec4 v_styles;
#else
	flat out uvec4 v_styles;
#endif

#ifdef SKYBOX_ENABLED

void MakeSkyVec(float s, float t, int axis, float zFar, out vec3 position, out vec2 texCoord)
{
	const float flScale = 0.57735;
	const ivec3 st_to_vec[6] =
	{
		ivec3( 3, -1, 2 ),
		ivec3( -3, 1, 2 ),

		ivec3( 1, 3, 2 ),
		ivec3( -1, -3, 2 ),

		ivec3( -2, -1, 3 ),
		ivec3( 2, -1, -3 )
	};

	float width = zFar * flScale;
	vec3 b = vec3(s * width, t * width, width);
	vec3 v = SceneUBO.viewpos.xyz;

	for (int j = 0; j < 3; j++)
	{
		int k = st_to_vec[axis][j];
		float v_negetive = -b[-k - 1];
		float v_positive = b[k - 1];
		v[j] += mix(v_negetive, v_positive, float(step(0, k)) );
	}

	// avoid bilerp seam
	s = (s + 1)*0.5;
	t = (t + 1)*0.5;
	
	// AV - I'm commenting this out since our skyboxes aren't 512x512 and we don't
	//      modify the textures to deal with the border seam fixup correctly.
	//      The code below was causing seams in the skyboxes.
	s = clamp(s, 1.0 / 512.0, 511.0 / 512.0);
	t = clamp(t, 1.0 / 512.0, 511.0 / 512.0);
	
	t = 1.0 - t;

	position = v;
	texCoord = vec2(s, t);
}

#endif


void main(void)
{
#ifdef SKYBOX_ENABLED

	int vertidx = gl_VertexID % 4;
	int quadidx = gl_VertexID / 4;

	const vec4 s_array = vec4(-1.0, -1.0, 1.0, 1.0);
	const vec4 t_array = vec4(-1.0, 1.0, 1.0, -1.0);

	vec3 vertex = vec3(0.0);
	vec2 texcoord = vec2(0.0);
	MakeSkyVec(s_array[vertidx], t_array[vertidx], quadidx, SceneUBO.z_far, vertex, texcoord);

	vec4 worldpos4 = vec4(vertex, 1.0);
    v_worldpos = worldpos4.xyz;

	vec4 normal4 = vec4(in_normal.xyz, 0.0);
	v_normal = normalize((normal4).xyz);

	v_diffusetexcoord = texcoord;

#else

	vec4 worldpos4 = EntityUBO.entityMatrix * vec4(in_vertex.xyz, 1.0);
    v_worldpos = worldpos4.xyz;

	vec4 normal4 = vec4(in_normal.xyz, 0.0);
	v_normal = normalize((EntityUBO.entityMatrix * normal4).xyz);

	#ifdef DIFFUSE_ENABLED
		v_diffusetexcoord = vec2(in_diffusetexcoord.x + in_diffusetexcoord.z * EntityUBO.scrollSpeed, in_diffusetexcoord.y);
	#endif

#endif

#ifdef LIGHTMAP_ENABLED
	v_lightmaptexcoord = in_lightmaptexcoord;
#endif

#ifdef REPLACETEXTURE_ENABLED
	v_replacetexcoord = in_replacetexcoord;
#endif

#ifdef DETAILTEXTURE_ENABLED
	v_detailtexcoord = in_detailtexcoord;
#endif

#if defined(NORMALTEXTURE_ENABLED) || defined(PARALLAXTEXTURE_ENABLED)
    vec4 tangent4 = vec4(in_tangent, 0.0);
    v_tangent = normalize((EntityUBO.entityMatrix * tangent4).xyz);
	vec4 bitangent4 = vec4(in_bitangent, 0.0);
    v_bitangent = normalize((EntityUBO.entityMatrix * bitangent4).xyz);
#endif

#ifdef NORMALTEXTURE_ENABLED
	v_normaltexcoord = in_normaltexcoord;
#endif

#ifdef PARALLAXTEXTURE_ENABLED
	v_parallaxtexcoord = in_parallaxtexcoord;
#endif

#ifdef SPECULARTEXTURE_ENABLED
	v_speculartexcoord = in_speculartexcoord;
#endif

#ifdef SHADOWMAP_ENABLED

	#ifdef SHADOWMAP_HIGH_ENABLED
        v_shadowcoord[0] = SceneUBO.shadowMatrix[0] * vec4(v_worldpos, 1.0);
    #endif

    #ifdef SHADOWMAP_MEDIUM_ENABLED
        v_shadowcoord[1] = SceneUBO.shadowMatrix[1] * vec4(v_worldpos, 1.0);
    #endif

    #ifdef SHADOWMAP_LOW_ENABLED
        v_shadowcoord[2] = SceneUBO.shadowMatrix[2] * vec4(v_worldpos, 1.0);
    #endif

#endif

#ifdef BINDLESS_ENABLED

	#if defined(SKYBOX_ENABLED)
		v_drawid = quadidx;
	#elif defined(DECAL_ENABLED)
		v_decalindex = in_decalindex;
	#else
		v_texindex = in_texindex;
	#endif

#endif

	#if defined(SKYBOX_ENABLED)
		
	#elif defined(DECAL_ENABLED)
		v_styles = in_styles;
	#else
		v_styles = in_styles;
	#endif	

	gl_Position = SceneUBO.projMatrix * SceneUBO.viewMatrix * worldpos4;

	v_projpos = gl_Position;
}