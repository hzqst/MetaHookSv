#version 430

out vec2 texCoord;

void main(void)
{
	uint idx = gl_VertexID % 4;

	vec2 vertices[4]= vec2[4](vec2(-1, -1), vec2(-1, 1), vec2(1, 1), vec2(1, -1));
	vec2 texcoords[4]= vec2[4](vec2(0, 0), vec2(0, 1), vec2(1, 1), vec2(1, 0));

	gl_Position = vec4(vertices[idx], 0, 1);

	texCoord = texcoords[idx];
}