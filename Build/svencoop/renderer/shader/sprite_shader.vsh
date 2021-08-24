#version 460

#include "common.h"

out vec3 v_worldpos;
out vec3 v_normal;
out vec4 v_color;
out vec2 v_texcoord;
flat out int v_frameindex;

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
	float use_oriented = abs(sign(angles.z));

	vec3 oriented_forward;
	vec3 oriented_right;
	vec3 oriented_up;
	R_GetSpriteAxes_ParallelOriented(angles, oriented_forward, oriented_right, oriented_up);

	up = mix(SceneUBO.vup.xyz, oriented_up, use_oriented);
	right = mix(SceneUBO.vright.xyz, oriented_right, use_oriented);
	forward = mix(SceneUBO.vpn.xyz, oriented_forward, use_oriented);
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
	vec4 in_color = SpriteEntrySSBO.entries[gl_InstanceID].color;
	vec4 in_origin = SpriteEntrySSBO.entries[gl_InstanceID].origin;
	vec3 in_angles = SpriteEntrySSBO.entries[gl_InstanceID].angles;
	int in_frameindex = SpriteEntrySSBO.entries[gl_InstanceID].frameindex;

	int type = SpriteFrameSSBO.frames[in_frameindex].type;
	int width = SpriteFrameSSBO.frames[in_frameindex].width;
	int height = SpriteFrameSSBO.frames[in_frameindex].height;
	float up = SpriteFrameSSBO.frames[in_frameindex].up;
	float down = SpriteFrameSSBO.frames[in_frameindex].down;
	float left = SpriteFrameSSBO.frames[in_frameindex].left;
	float right = SpriteFrameSSBO.frames[in_frameindex].right;

	float scale = in_origin.w;

	uint idx = gl_VertexID % 4;

	vec3 vForwardArray[5];
	vec3 vRightArray[5];
	vec3 vUpArray[5];

	R_GetSpriteAxes_ParallelUpright(in_angles, vForwardArray[SPR_VP_PARALLEL_UPRIGHT], vRightArray[SPR_VP_PARALLEL_UPRIGHT], vUpArray[SPR_VP_PARALLEL_UPRIGHT]);
	R_GetSpriteAxes_FacingUpright(in_angles, vForwardArray[SPR_FACING_UPRIGHT], vRightArray[SPR_FACING_UPRIGHT], vUpArray[SPR_FACING_UPRIGHT]);
	R_GetSpriteAxes_Parallel(in_angles, vForwardArray[SPR_VP_PARALLEL], vRightArray[SPR_VP_PARALLEL], vUpArray[SPR_VP_PARALLEL]);
	R_GetSpriteAxes_Oriented(in_angles, vForwardArray[SPR_ORIENTED], vRightArray[SPR_ORIENTED], vUpArray[SPR_ORIENTED]);
	R_GetSpriteAxes_ParallelOriented(in_angles, vForwardArray[SPR_VP_PARALLEL_ORIENTED], vRightArray[SPR_VP_PARALLEL_ORIENTED], vUpArray[SPR_VP_PARALLEL_ORIENTED]);

	vec3 vForward = vForwardArray[type];
	vec3 vRight = vRightArray[type];
	vec3 vUp = vUpArray[type];

	vec3 vertex[4];

	vertex[0] = in_origin.xyz + (scale * down) * vUp;
	vertex[0] = vertex[0] + (scale * left) * vRight;

	vertex[1] = in_origin.xyz + (scale * up) * vUp;
	vertex[1] = vertex[1] + (scale * left) * vRight;
	
	vertex[2] = in_origin.xyz + (scale * up) * vUp;
	vertex[2] = vertex[2] + (scale * right) * vRight;

	vertex[3] = in_origin.xyz + (scale * down) * vUp;
	vertex[3] = vertex[3] + (scale * right) * vRight;

	vec3 outvert = vertex[idx];

	gl_Position = SceneUBO.projMatrix * SceneUBO.viewMatrix * vec4(outvert, 1.0);

	v_worldpos = outvert;
	v_normal = normalize(vForward);
	v_color = in_color;
	v_texcoord = vec2( texcoord_s[idx], texcoord_t[idx] );
	v_frameindex = in_frameindex;
}