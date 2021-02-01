//Shadow Vertex Shader by hzqst

varying vec4 ShadowCoord;
varying vec3 worldpos;

void main()
{
	worldpos = gl_Vertex.xyz / gl_Vertex.w;
  	ShadowCoord = gl_TextureMatrix[0] * gl_ModelViewMatrix * gl_Vertex;
	gl_Position = ftransform();
}