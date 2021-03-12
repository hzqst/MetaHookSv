//Airblast Vertex Shader by hzqst
varying vec4 projpos;

void main()
{
	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;
	projpos = gl_ModelViewProjectionMatrix * gl_Vertex;

	gl_FrontColor = gl_Color;
	gl_Position = ftransform();
}