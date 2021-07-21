uniform mat4 entitymatrix;
uniform mat4 worldmatrix;
uniform vec4 eyepos;

varying vec4 projpos;
varying vec4 worldpos;
varying vec4 posToCameraDir;

#ifdef DEPTH_ENABLED
varying mat4 viewprojmatrix_inv;
#endif

void main()
{
	projpos = gl_ModelViewProjectionMatrix * gl_Vertex;
	worldpos = entitymatrix * gl_Vertex;

	posToCameraDir.xyz = eyepos.xyz - worldpos.xyz;
	posToCameraDir.a = 1.0;
#ifdef DEPTH_ENABLED
	viewprojmatrix_inv = inverse(gl_ModelViewProjectionMatrix) * entitymatrix;
#endif

	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy / 128.0;
	gl_Position = ftransform();
}