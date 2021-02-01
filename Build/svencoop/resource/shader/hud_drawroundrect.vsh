//Vertex Shader by hzqst

varying vec2 worldpos;

void main()
{
	worldpos = gl_Vertex.xy / gl_Vertex.w;
	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;
	gl_FrontColor = gl_Color;
	gl_Position = ftransform();
}