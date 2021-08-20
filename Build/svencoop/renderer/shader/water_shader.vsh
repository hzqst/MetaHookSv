#version 430

uniform vec4 u_watercolor;
uniform vec2 u_depthfactor;
uniform float u_fresnelfactor;
uniform float u_normfactor;

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

layout (std140, binding = 0) uniform SceneBlock
{
   scene_ubo_t SceneUBO;
};

layout (std140, binding = 1) uniform EntityBlock
{
   entity_ubo_t EntityUBO;
};

layout(location = 0) in vec3 in_vertex;
layout(location = 1) in vec3 in_normal;
layout(location = 4) in vec2 in_diffusetexcoord;

out vec4 v_projpos;
out vec3 v_worldpos;
out vec3 v_normal;
out vec2 v_diffusetexcoord;

void main()
{
	vec4 worldpos4 = /*EntityUBO.entityMatrix * */vec4(in_vertex, 1.0);
    v_worldpos = worldpos4.xyz;

	vec4 normal4 = vec4(in_normal, 0.0);
	v_normal = normalize((EntityUBO.entityMatrix * normal4).xyz);

	v_diffusetexcoord = in_diffusetexcoord.xy;
	gl_Position = SceneUBO.projMatrix * SceneUBO.viewMatrix * worldpos4;

	v_projpos = gl_Position;
}