#ifdef VOLUME_ENABLED
varying vec4 projpos;
uniform mat4 modelmatrix;
#endif

uniform vec3 viewpos;

varying vec3 fragpos;

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
#ifdef VOLUME_ENABLED
	vec4 worldp = modelmatrix * gl_Vertex;
	gl_Position = gl_ModelViewProjectionMatrix * worldp;
	fragpos = worldp.xyz;
	projpos = gl_Position;
#else
	fragpos = gl_Color;
	gl_Position = ftransform();
#endif
}