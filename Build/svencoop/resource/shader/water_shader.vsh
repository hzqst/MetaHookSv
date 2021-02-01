//Water Vertex Shader by hzqst
varying vec3 worldpos;
varying vec4 projpos;

void main()
{
	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy / 128.0;
	projpos = gl_ModelViewProjectionMatrix * gl_Vertex;

	worldpos = gl_Vertex.xyz / gl_Vertex.w;
	gl_Position = ftransform();
}