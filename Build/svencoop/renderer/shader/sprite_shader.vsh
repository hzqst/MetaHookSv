#version 430

#include "common.h"

layout (location = 0) uniform ivec2 width_height;
layout (location = 1) uniform vec4 up_down_left_right;
layout (location = 2) uniform vec4 in_color;
layout (location = 3) uniform vec3 in_origin;
layout (location = 4) uniform vec3 in_angles;
layout (location = 5) uniform float in_scale;

out vec3 v_worldpos;
out vec3 v_normal;
out vec4 v_color;
out vec2 v_texcoord;

const vec4 texcoord_s = vec4(0.0, 0.0, 1.0, 1.0);
const vec4 texcoord_t = vec4(1.0, 0.0, 0.0, 1.0);

void R_GetSpriteAxes_ParallelUpright(vec3 angles, inout vec3 forward, inout vec3 right, inout vec3 up)
{
	up = vec3(0.0, 0.0, 1.0);

	right = vec3(SceneUBO.vpn.y, -SceneUBO.vpn.x, 0.0);

	right = normalize(right);

	forward = vec3(-right.y, right.x, 0.0);
}

void R_GetSpriteAxes_FacingUpright(vec3 angles, inout vec3 forward, inout vec3 right, inout vec3 up)
{
	vec3 tvec = -SceneUBO.viewpos.xyz;

	tvec = normalize(tvec);

	up = vec3(0.0, 0.0, 1.0);
	right = vec3(tvec.y, -tvec.x, 0.0);

	right = normalize(right);

	forward = vec3(-right.y, right.x, 0.0);
}

void R_GetSpriteAxes_ParallelOriented(vec3 angles, inout vec3 forward, inout vec3 right, inout vec3 up)
{
	float angle = angles.z * (3.141593 * 2.0 / 360.0);
	float sr = sin(angle);
	float cr = cos(angle);

	forward = SceneUBO.vpn.xyz;
	right = vec3(SceneUBO.vright.x * cr + SceneUBO.vup.x * sr, SceneUBO.vright.y * cr + SceneUBO.vup.y * sr, SceneUBO.vright.z * cr + SceneUBO.vup.z * sr);
	up = vec3(SceneUBO.vright.x * -sr + SceneUBO.vup.x * cr, SceneUBO.vright.y * -sr + SceneUBO.vup.y * cr, SceneUBO.vright.z * -sr + SceneUBO.vup.z * cr);
}

void R_GetSpriteAxes_Parallel(vec3 angles, inout vec3 forward, inout vec3 right, inout vec3 up)
{
	up = SceneUBO.vup.xyz;
	right = SceneUBO.vright.xyz;
	forward = SceneUBO.vpn.xyz;
}

void AngleVectors(vec3 angles, inout vec3 forward, inout vec3 right, inout vec3 up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	
	angle = angles.y * (3.141593 * 2.0 / 360.0);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles.x * (3.141593 * 2.0 / 360.0);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles.z * (3.141593 * 2.0 / 360.0);
	sr = sin(angle);
	cr = cos(angle);

	forward = vec3(cp*cy, cp*sy, -sp);

	right = vec3((-1*sr*sp*cy+-1*cr*-sy), (-1*sr*sp*sy+-1*cr*cy), -1*sr*cp);

	up = vec3((cr*sp*cy+-sr*-sy), (cr*sp*sy+-sr*cy), cr*cp);
}

void R_GetSpriteAxes_Oriented(vec3 angles, inout vec3 forward, inout vec3 right, inout vec3 up)
{
	AngleVectors(angles, forward, right, up);
}

void main()
{
	int width = width_height.x;
	int height = width_height.y;
	float up = up_down_left_right.x;
	float down = up_down_left_right.y;
	float left = up_down_left_right.z;
	float right = up_down_left_right.w;

	uint idx = gl_VertexID % 4;

	vec3 vForward, vRight, vUp;
	
#ifdef PARALLEL_UPRIGHT_ENABLED
	R_GetSpriteAxes_ParallelUpright(in_angles, vForward, vRight, vUp);
#endif
	
#ifdef FACING_UPRIGHT_ENABLED
	R_GetSpriteAxes_FacingUpright(in_angles, vForward, vRight, vUp);
#endif

#ifdef PARALLEL_ENABLED
	R_GetSpriteAxes_Parallel(in_angles, vForward, vRight, vUp);
#endif

#ifdef ORIENTED_ENABLED
	R_GetSpriteAxes_Oriented(in_angles, vForward, vRight, vUp);
#endif

#ifdef PARALLEL_ORIENTED_ENABLED
	R_GetSpriteAxes_ParallelOriented(in_angles, vForward, vRight, vUp);
#endif

	vec3 vertexArray[4];

	vertexArray[0] = in_origin + (in_scale * down) * vUp;
	vertexArray[0] = vertexArray[0] + (in_scale * left) * vRight;

	vertexArray[1] = in_origin + (in_scale * up) * vUp;
	vertexArray[1] = vertexArray[1] + (in_scale * left) * vRight;
	
	vertexArray[2] = in_origin + (in_scale * up) * vUp;
	vertexArray[2] = vertexArray[2] + (in_scale * right) * vRight;

	vertexArray[3] = in_origin + (in_scale * down) * vUp;
	vertexArray[3] = vertexArray[3] + (in_scale * right) * vRight;

	vec3 outvert = vertexArray[idx];

	gl_Position = SceneUBO.projMatrix * SceneUBO.viewMatrix * vec4(outvert, 1.0);
	v_worldpos = outvert;
	v_normal = normalize(vForward);
	v_color = in_color;
	v_texcoord = vec2( texcoord_s[idx], texcoord_t[idx] );
}