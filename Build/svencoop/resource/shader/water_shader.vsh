uniform mat4 entitymatrix;

varying vec4 projpos;
varying vec4 worldpos;

void main()
{
	projpos = gl_ModelViewProjectionMatrix * gl_Vertex;
	worldpos = entitymatrix * gl_Vertex;

	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy / 128.0;
	gl_Position = ftransform();
}