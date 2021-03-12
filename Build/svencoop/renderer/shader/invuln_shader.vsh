//Invuln Vertex Shader by hzqst

void main()
{
	gl_TexCoord[1].xy = gl_MultiTexCoord0.xy / 128.0;
	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;
	gl_Position = ftransform();
}