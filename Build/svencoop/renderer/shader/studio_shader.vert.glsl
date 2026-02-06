#version 430

#include "common.h"

uniform vec3 r_celshade_head_offset;
uniform vec2 r_hair_shadow_offset;
uniform vec2 r_uvscale;
//uniform float r_viewmodel_scale;

layout(location = STUDIO_VA_POSITION) in vec3 in_vertex;
layout(location = STUDIO_VA_NORMAL) in vec3 in_normal;
layout(location = STUDIO_VA_TEXCOORD) in vec2 in_texcoord;
layout(location = STUDIO_VA_PACKEDBONE) in uint in_packedbone;
layout(location = STUDIO_VA_TANGENT) in vec3 in_tangent;
layout(location = STUDIO_VA_BITANGENT) in vec3 in_bitangent;
layout(location = STUDIO_VA_SMOOTHNORMAL) in vec3 in_smoothnormal;

out vec3 v_worldpos;
out vec3 v_normal;
out vec2 v_texcoord;
out vec4 v_projpos;
flat out uint v_packedbone;
out vec3 v_tangent;
out vec3 v_bitangent;
out vec3 v_smoothnormal;

#if defined(STUDIO_NF_CELSHADE_FACE)

	out vec3 v_headfwd;
	out vec3 v_headup;
	out vec3 v_headorigin;

	#if defined(DEBUG_ENABLED)

		out vec4 v_headorigin_proj;

	#endif
	
#endif

void main(void)
{
	vec3 vert = in_vertex;
	vec3 norm = in_normal;

	uint vertbone = UnpackStudioBoneAsVertBone(in_packedbone);
	uint normbone = UnpackStudioBoneAsNormBone(in_packedbone);

	mat3x4 vertbone_matrix = StudioUBO.bonematrix[vertbone];
    vec3 vertbone_matrix_0 = vec3(vertbone_matrix[0][0], vertbone_matrix[0][1], vertbone_matrix[0][2]);
  	vec3 vertbone_matrix_1 = vec3(vertbone_matrix[1][0], vertbone_matrix[1][1], vertbone_matrix[1][2]);
    vec3 vertbone_matrix_2 = vec3(vertbone_matrix[2][0], vertbone_matrix[2][1], vertbone_matrix[2][2]);
	vec3 outvert = vec3(
		dot(vert, vertbone_matrix_0) + vertbone_matrix[0][3],
		dot(vert, vertbone_matrix_1) + vertbone_matrix[1][3],
		dot(vert, vertbone_matrix_2) + vertbone_matrix[2][3]
	);

	mat3x4 normbone_matrix = StudioUBO.bonematrix[normbone];
	vec3 normbone_matrix_0 = vec3(normbone_matrix[0][0], normbone_matrix[0][1], normbone_matrix[0][2]);
    vec3 normbone_matrix_1 = vec3(normbone_matrix[1][0], normbone_matrix[1][1], normbone_matrix[1][2]);
    vec3 normbone_matrix_2 = vec3(normbone_matrix[2][0], normbone_matrix[2][1], normbone_matrix[2][2]);

	v_worldpos = outvert;
	v_normal = normalize(vec3(
		dot(norm, normbone_matrix_0),
		dot(norm, normbone_matrix_1),
		dot(norm, normbone_matrix_2)
	));
	v_packedbone = in_packedbone;
	v_tangent = normalize(vec3(
		dot(in_tangent, normbone_matrix_0),
		dot(in_tangent, normbone_matrix_1),
		dot(in_tangent, normbone_matrix_2)
	));
	v_bitangent = normalize(vec3(
		dot(in_bitangent, normbone_matrix_0),
		dot(in_bitangent, normbone_matrix_1),
		dot(in_bitangent, normbone_matrix_2)
	));
	v_smoothnormal = normalize(vec3(
		dot(in_smoothnormal, normbone_matrix_0),
		dot(in_smoothnormal, normbone_matrix_1),
		dot(in_smoothnormal, normbone_matrix_2)
	));

	outvert = outvert + v_smoothnormal * StudioUBO.r_scale;
	v_worldpos = outvert;

#if !defined(SHADOW_CASTER_ENABLED)

	#if defined(STUDIO_NF_CHROME)

		vec3 tmp = vec3(
			normbone_matrix[0][3] - StudioUBO.r_origin.x,
			normbone_matrix[1][3] - StudioUBO.r_origin.y,
			normbone_matrix[2][3] - StudioUBO.r_origin.z
		);
		tmp = normalize(tmp);

		vec3 chromeupvec = cross(tmp, GetCameraVRight(0));
		chromeupvec = normalize(chromeupvec);

		vec3 chromerightvec = cross(tmp, chromeupvec);
		chromerightvec = normalize(chromerightvec);

		//VectorIRotate
		vec3 chromeup = vec3(
			chromeupvec.x * normbone_matrix[0][0] + chromeupvec.y * normbone_matrix[1][0] + chromeupvec.z * normbone_matrix[2][0],
			chromeupvec.x * normbone_matrix[0][1] + chromeupvec.y * normbone_matrix[1][1] + chromeupvec.z * normbone_matrix[2][1],
			chromeupvec.x * normbone_matrix[0][2] + chromeupvec.y * normbone_matrix[1][2] + chromeupvec.z * normbone_matrix[2][2]
		);

		//VectorIRotate
		vec3 chromeright = vec3(
			chromerightvec.x * normbone_matrix[0][0] + chromerightvec.y * normbone_matrix[1][0] + chromerightvec.z * normbone_matrix[2][0],
			chromerightvec.x * normbone_matrix[0][1] + chromerightvec.y * normbone_matrix[1][1] + chromerightvec.z * normbone_matrix[2][1],
			chromerightvec.x * normbone_matrix[0][2] + chromerightvec.y * normbone_matrix[1][2] + chromerightvec.z * normbone_matrix[2][2]
		);
		
		vec2 texcoord = vec2(
			(dot(norm, chromeright) + 1.0) * 1024.0,
			(dot(norm, chromeup) + 1.0) * 1024.0
		);

		v_texcoord = texcoord.xy;

	#else

		v_texcoord = in_texcoord.xy;

	#endif

#endif

	//Offset the hair geometry a little bit, but no the face geometry
#if defined(HAIR_SHADOW_ENABLED) && defined(STUDIO_NF_CELSHADE_HAIR)
	vec3 vecLight = StudioUBO.r_plightvec.xyz;
	vecLight.z *= 0.0001;
	vecLight = normalize(vecLight);
	outvert = outvert + vecLight * r_hair_shadow_offset.x + vec3(0.0, 0.0, r_hair_shadow_offset.y);
	v_worldpos = outvert;
#endif

	#if defined(STUDIO_NF_CELSHADE_FACE)

		v_headfwd = vec3(
			dot(vec3(0.0, 1.0, 0.0), vertbone_matrix_0),
			dot(vec3(0.0, 1.0, 0.0), vertbone_matrix_1),
			dot(vec3(0.0, 1.0, 0.0), vertbone_matrix_2)
		);
		v_headfwd = normalize(v_headfwd);

		v_headup = vec3(
			dot(vec3(1.0, 0.0, 0.0), vertbone_matrix_0),
			dot(vec3(1.0, 0.0, 0.0), vertbone_matrix_1),
			dot(vec3(1.0, 0.0, 0.0), vertbone_matrix_2)
		);
		v_headup = normalize(v_headup);

		v_headorigin = vec3(
			dot(r_celshade_head_offset, vertbone_matrix_0) + vertbone_matrix[0][3],
			dot(r_celshade_head_offset, vertbone_matrix_1) + vertbone_matrix[1][3],
			dot(r_celshade_head_offset, vertbone_matrix_2) + vertbone_matrix[2][3]
		);

		#if defined(DEBUG_ENABLED)

			v_headorigin_proj = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * vec4(v_headorigin, 1.0);

		#endif

	#endif

	gl_Position = GetCameraProjMatrix(0) * GetCameraWorldMatrix(0) * vec4(outvert, 1.0);

	v_projpos = gl_Position;
	v_texcoord = v_texcoord * r_uvscale;
}