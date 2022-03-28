#version 430

#include "common.h"

uniform vec2 r_hair_shadow_offset;

layout(location = 0) in vec3 in_vertex;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in ivec2 in_vertnormbone;

out vec3 v_worldpos;
out vec3 v_normal;
out vec4 v_color;
out vec2 v_texcoord;
out vec4 v_projpos;

void main(void)
{
	vec3 vert = in_vertex;
	vec3 norm = in_normal;

	int vertbone = in_vertnormbone.x;
	int normbone = in_vertnormbone.y;

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
	vec3 outnorm = vec3(
		dot(norm, normbone_matrix_0),
		dot(norm, normbone_matrix_1),
		dot(norm, normbone_matrix_2)
	);

	outnorm = normalize(outnorm);

	v_worldpos = outvert;
	v_normal = outnorm;

#ifndef SHADOW_CASTER_ENABLED

	#ifdef TRANSADDITIVE_ENABLED

		v_color = vec4(StudioUBO.r_blend, StudioUBO.r_blend, StudioUBO.r_blend, StudioUBO.r_blend);
		v_color = GammaToLinear(v_color);

	#else

		#ifdef STUDIO_NF_FULLBRIGHT

			v_color = vec4(1.0, 1.0, 1.0, StudioUBO.r_blend);
			v_color = GammaToLinear(v_color);

		#else

			float illum = StudioUBO.r_ambientlight;

			#ifdef STUDIO_NF_FLATSHADE

				illum += StudioUBO.r_shadelight * 0.8;

			#else

				float lightcos = dot(outnorm, StudioUBO.r_plightvec.xyz);

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

			#endif

			float lv = clamp(illum, 0.0, 255.0) / 255.0;

			lv = LightGammaToLinearInternal(lv);

			//lv = max(lv, 0.03);

			v_color = vec4(lv * StudioUBO.r_colormix.x, lv * StudioUBO.r_colormix.y, lv * StudioUBO.r_colormix.z, StudioUBO.r_blend);

		#endif

	#endif

	#if defined(OUTLINE_ENABLED)

		outvert = outvert + outnorm * StudioUBO.r_scale;
		v_worldpos = outvert;

	#elif defined(STUDIO_NF_CHROME)

		outvert = outvert + outnorm * StudioUBO.r_scale;
		v_worldpos = outvert;

		vec3 tmp = vec3(
			normbone_matrix[0][3] - StudioUBO.r_origin.x,
			normbone_matrix[1][3] - StudioUBO.r_origin.y,
			normbone_matrix[2][3] - StudioUBO.r_origin.z
		);
		tmp = normalize(tmp);

		vec3 chromeupvec = cross(tmp, SceneUBO.vright.xyz);
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

#if defined(HAIR_SHADOW_ENABLED) && defined(STUDIO_NF_CELSHADE_HAIR)
	vec3 vecLight = StudioUBO.r_plightvec.xyz;
	vecLight.z *= 0.0001;
	vecLight = normalize(vecLight);
	outvert = outvert + vecLight * r_hair_shadow_offset.x + vec3(0.0, 0.0, r_hair_shadow_offset.y);
#endif

	gl_Position = SceneUBO.projMatrix * SceneUBO.viewMatrix * vec4(outvert, 1.0);

	v_projpos = gl_Position;
}