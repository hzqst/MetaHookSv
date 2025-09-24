#version 430

#extension GL_EXT_texture_array : require

#include "common.h"

uniform float u_parallaxScale;

layout(location = WSURF_VA_POSITION) in vec3 in_vertex;
layout(location = WSURF_VA_NORMAL) in vec3 in_normal;
layout(location = WSURF_VA_S_TANGENT) in vec3 in_tangent;
layout(location = WSURF_VA_T_TANGENT) in vec3 in_bitangent;
layout(location = WSURF_VA_TEXCOORD) in vec3 in_diffusetexcoord;
layout(location = WSURF_VA_LIGHTMAP_TEXCOORD) in vec3 in_lightmaptexcoord;
layout(location = WSURF_VA_REPLACETEXTURE_TEXCOORD) in vec2 in_replacetexcoord;
layout(location = WSURF_VA_DETAILTEXTURE_TEXCOORD) in vec2 in_detailtexcoord;
layout(location = WSURF_VA_NORMALTEXTURE_TEXCOORD) in vec2 in_normaltexcoord;
layout(location = WSURF_VA_PARALLAXTEXTURE_TEXCOORD) in vec2 in_parallaxtexcoord;
layout(location = WSURF_VA_SPECULARTEXTURE_TEXCOORD) in vec2 in_speculartexcoord;

#if defined(SKYBOX_ENABLED)

#elif defined(DECAL_ENABLED)
	layout(location = WSURF_VA_STYLES) in uvec4 in_styles;
#else
	layout(location = WSURF_VA_STYLES) in uvec4 in_styles;
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

#if defined(SKYBOX_ENABLED)
	
#elif defined(DECAL_ENABLED)
	flat out uvec4 v_styles;
#else
	flat out uvec4 v_styles;
#endif

#if defined(SKYBOX_ENABLED)

void MakeSkyVec(float s, float t, int axis, float zFar, out vec3 position, out vec3 normal, out vec2 texCoord)
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
	const vec3 axis_to_normal[6] = 
    {
        vec3( -1, 0, 0 ),
        vec3( 1, 0, 0 ),

        vec3( 0, -1, 0 ),
        vec3( 0, 1, 0 ),

        vec3( 0, 0, -1 ),
        vec3( 0, 0, 1 ),
    };

	float width = zFar * flScale;
	vec3 b = vec3(s * width, t * width, width);
	vec3 v = CameraUBO.viewpos.xyz;

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
	normal = axis_to_normal[axis];
	texCoord = vec2(s, t);
}

#endif

void main(void)
{
#if defined(SKYBOX_ENABLED)

	int vertidx = gl_VertexID % 6;
	int quadidx = gl_VertexID / 6;

	const int indexedVertIds[] = {
        0, 1, 2, 2, 3, 0
    };

	vertidx = indexedVertIds[vertidx];

	const vec4 s_array = vec4(-1.0, -1.0, 1.0, 1.0);
	const vec4 t_array = vec4(-1.0, 1.0, 1.0, -1.0);

	vec3 vertex = vec3(0.0, 0.0, 0.0);
    vec3 normal = vec3(0.0, 0.0, 0.0);
	vec2 texcoord = vec2(0.0, 0.0);
	MakeSkyVec(s_array[vertidx], t_array[vertidx], quadidx, SceneUBO.z_far, vertex, normal, texcoord);

	vec4 normal4 = vec4(normal.xyz, 0.0);
	v_normal = normalize((normal4).xyz);

	#if defined(REVERT_NORMAL_ENABLED)
		v_normal = v_normal * -1.0;
	#endif

	vec4 worldpos4 = vec4(vertex, 1.0);

	worldpos4.xyz += v_normal.xyz * EntityUBO.scale;

    v_worldpos = worldpos4.xyz;

	v_diffusetexcoord = texcoord;

#else

	vec4 normal4 = vec4(in_normal.xyz, 0.0);
	v_normal = normalize((EntityUBO.entityMatrix * normal4).xyz);

	#if defined(REVERT_NORMAL_ENABLED)
		v_normal = v_normal * -1.0;
	#endif

	vec4 worldpos4 = EntityUBO.entityMatrix * vec4(in_vertex.xyz, 1.0);

	worldpos4.xyz += v_normal.xyz * EntityUBO.scale;

    v_worldpos = worldpos4.xyz;

	#ifdef DIFFUSE_ENABLED
		v_diffusetexcoord = vec2(in_diffusetexcoord.x + in_diffusetexcoord.z * EntityUBO.scrollSpeed, in_diffusetexcoord.y);
	#endif

#endif

#if defined(LIGHTMAP_ENABLED)
	v_lightmaptexcoord = in_lightmaptexcoord;
#endif

#if defined(REPLACETEXTURE_ENABLED)
	v_replacetexcoord = in_replacetexcoord;
#endif

#if defined(DETAILTEXTURE_ENABLED)
	v_detailtexcoord = in_detailtexcoord;
#endif

#if defined(NORMALTEXTURE_ENABLED) || defined(PARALLAXTEXTURE_ENABLED)

    vec4 tangent4 = vec4(in_tangent, 0.0);
	
	#if defined(REVERT_NORMAL_ENABLED)
		tangent4 = tangent4 * -1.0;
	#endif

    v_tangent = normalize((EntityUBO.entityMatrix * tangent4).xyz);

	vec4 bitangent4 = vec4(in_bitangent, 0.0);
	
	#if defined(REVERT_NORMAL_ENABLED)
		bitangent4 = bitangent4 * -1.0;
	#endif

    v_bitangent = normalize((EntityUBO.entityMatrix * bitangent4).xyz);

#endif

#if defined(NORMALTEXTURE_ENABLED)
	v_normaltexcoord = in_normaltexcoord;
#endif

#if defined(PARALLAXTEXTURE_ENABLED)
	v_parallaxtexcoord = in_parallaxtexcoord;
#endif

#if defined(SPECULARTEXTURE_ENABLED)
	v_speculartexcoord = in_speculartexcoord;
#endif

	#if defined(SKYBOX_ENABLED)
		
	#elif defined(DECAL_ENABLED)
		v_styles = in_styles;
	#else
		v_styles = in_styles;
	#endif	

	gl_Position = CameraUBO.projMatrix * CameraUBO.viewMatrix * worldpos4;

	v_projpos = gl_Position;
}