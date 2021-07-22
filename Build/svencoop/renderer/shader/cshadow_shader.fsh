#version 130

uniform vec3 entitypos;

void main(void)
{
	gl_FragColor = vec4(entitypos.x, entitypos.y, entitypos.z, gl_FragCoord.z);
}