uniform mat4 entitymatrix;
uniform vec4 eyepos;

varying vec4 projpos;
varying vec4 worldpos;

#ifdef DEPTH_ENABLED
varying mat4 invviewprojmatrix;
#endif

void main()
{
	projpos = gl_ModelViewProjectionMatrix * gl_Vertex;
	worldpos = entitymatrix * gl_Vertex;

#ifdef DEPTH_ENABLED
	invviewprojmatrix = inverse(gl_ModelViewProjectionMatrix) * entitymatrix;
#endif

	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy / 128.0;
	gl_Position = ftransform();
}